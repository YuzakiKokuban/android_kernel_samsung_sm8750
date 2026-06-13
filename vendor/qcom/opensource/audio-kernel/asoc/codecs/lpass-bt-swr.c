// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <soc/swr-common.h>
#include <asoc/msm-cdc-pinctrl.h>
#include <dsp/digital-cdc-rsc-mgr.h>
#include <soc/swr-wcd.h>
#include <soc/snd_event.h>

#define DRV_NAME "lpass-bt-swr"

/* pm runtime auto suspend timer in msecs */
#define LPASS_BT_SWR_AUTO_SUSPEND_DELAY          100 /* delay in msec */

#define LPASS_BT_SWR_STRING_LEN 80

#define LPASS_BT_SWR_CHILD_DEVICES_MAX 1

/* Hold instance to soundwire platform device */
struct lpass_bt_swr_ctrl_data {
	struct platform_device *lpass_bt_swr_pdev;
};

struct lpass_bt_swr_ctrl_platform_data {
	void *handle; /* holds parent private data */
	int (*read)(void *handle, int reg);
	int (*write)(void *handle, int reg, int val);
	int (*bulk_write)(void *handle, u32 *reg, u32 *val, size_t len);
	int (*clk)(void *handle, bool enable);
	int (*core_vote)(void *handle, bool enable);
	int (*handle_irq)(void *handle,
			  irqreturn_t (*swrm_irq_handler)(int irq,
							  void *data),
			  void *swrm_handle,
			  int action);
};

struct lpass_bt_swr_priv {
	struct device *dev;
	struct mutex vote_lock;
	struct mutex swr_clk_lock;
	struct mutex ssr_lock;
	bool dev_up;
	bool initial_boot;

	struct clk *lpass_core_hw_vote;
	struct clk *lpass_audio_hw_vote;
	int core_hw_vote_count;
	int core_audio_vote_count;
	int swr_clk_users;
	struct clk *clk_handle;
	struct clk *clk_handle_2x;

	struct lpass_bt_swr_ctrl_data *swr_ctrl_data;
	struct lpass_bt_swr_ctrl_platform_data swr_plat_data;
	struct work_struct lpass_bt_swr_add_child_devices_work;
	struct platform_device *pdev_child_devices
			[LPASS_BT_SWR_CHILD_DEVICES_MAX];
	int child_count;

	struct device_node *bt_swr_gpio_p;

	/* Entry for version info */
	struct snd_info_entry *entry;
	struct snd_info_entry *version_entry;

	struct blocking_notifier_head notifier;
	struct device *clk_dev;
};

static struct lpass_bt_swr_priv *lpass_bt_priv;

static void lpass_bt_swr_add_child_devices(struct work_struct *work)
{
	struct lpass_bt_swr_priv *priv;
	struct platform_device *pdev;
	struct device_node *node;
	struct lpass_bt_swr_ctrl_data *swr_ctrl_data = NULL, *temp;
	int ret;
	u16 count = 0, ctrl_num = 0;
	struct lpass_bt_swr_ctrl_platform_data *platdata;
	char plat_dev_name[LPASS_BT_SWR_STRING_LEN];

	priv = container_of(work, struct lpass_bt_swr_priv,
			     lpass_bt_swr_add_child_devices_work);
	if (!priv) {
		pr_err("%s: Memory for priv does not exist\n",
			__func__);
		return;
	}
	if (!priv->dev || !priv->dev->of_node) {
		dev_err(priv->dev,
			"%s: DT node for priv does not exist\n", __func__);
		return;
	}

	platdata = &priv->swr_plat_data;
	priv->child_count = 0;

	for_each_available_child_of_node(priv->dev->of_node, node) {
		if (strnstr(node->name, "bt_swr_mstr",
				strlen("bt_swr_mstr")) != NULL)
			strscpy(plat_dev_name, "bt_swr_mstr",
				(LPASS_BT_SWR_STRING_LEN - 1));
		else
			continue;

		pdev = platform_device_alloc(plat_dev_name, -1);
		if (!pdev) {
			dev_err(priv->dev, "%s: pdev memory alloc failed\n",
				__func__);
			ret = -ENOMEM;
			return;
		}
		pdev->dev.parent = priv->dev;
		pdev->dev.of_node = node;

		ret = platform_device_add_data(pdev, platdata,
					       sizeof(*platdata));
		if (ret) {
			dev_err(&pdev->dev,
				"%s: cannot add plat data ctrl:%d\n",
				__func__, ctrl_num);
			goto fail_pdev_add;
		}

		temp = krealloc(swr_ctrl_data,
				(ctrl_num + 1) * sizeof(
				struct lpass_bt_swr_ctrl_data),
				GFP_KERNEL);
		if (!temp) {
			dev_err(&pdev->dev, "out of memory\n");
			ret = -ENOMEM;
			goto fail_pdev_add;
		}
		swr_ctrl_data = temp;
		swr_ctrl_data[ctrl_num].lpass_bt_swr_pdev = pdev;
		ctrl_num++;

		dev_dbg(&pdev->dev, "%s: Adding soundwire ctrl device(s)\n",
			__func__);
		priv->swr_ctrl_data = swr_ctrl_data;

		ret = platform_device_add(pdev);
		if (ret) {
			dev_err(&pdev->dev,
				"%s: Cannot add platform device\n",
				__func__);
			goto fail_pdev_add;
		}

		if (priv->child_count < LPASS_BT_SWR_CHILD_DEVICES_MAX)
			priv->pdev_child_devices[
					priv->child_count++] = pdev;
		else
			return;
	}

	return;
fail_pdev_add:
	for (count = 0; count < priv->child_count; count++)
		platform_device_put(priv->pdev_child_devices[count]);
}


bool lpass_bt_swr_check_core_votes(struct lpass_bt_swr_priv *priv)
{
	bool ret = true;

	mutex_lock(&priv->vote_lock);
	if (!priv->dev_up ||
		(priv->lpass_core_hw_vote && !priv->core_hw_vote_count) ||
		(priv->lpass_audio_hw_vote && !priv->core_audio_vote_count))
		ret = false;
	mutex_unlock(&priv->vote_lock);

	return ret;
}

static int lpass_bt_swr_core_vote(void *handle, bool enable)
{
	int rc = 0;
	struct lpass_bt_swr_priv *priv = (struct lpass_bt_swr_priv *) handle;

	if (priv == NULL) {
		pr_err_ratelimited("%s: priv data is NULL\n", __func__);
		return -EINVAL;
	}

	if (!priv->dev_up && enable) {
		pr_err("%s: adsp is not up\n", __func__);
		return -EINVAL;
	}

	if (enable) {
		pm_runtime_get_sync(priv->dev);
		if (lpass_bt_swr_check_core_votes(priv))
			rc = 0;
		else
			rc = -ENOTSYNC;
	} else {
		pm_runtime_put_autosuspend(priv->dev);
		pm_runtime_mark_last_busy(priv->dev);
	}
	return rc;
}

static int lpass_bt_swr_mclk_enable(
				struct lpass_bt_swr_priv *priv,
				bool mclk_enable)
{
	int ret = 0;

	dev_dbg(priv->dev, "%s: mclk_enable = %u\n",
		__func__, mclk_enable);

	if (mclk_enable) {
		ret = clk_prepare_enable(priv->clk_handle);
		if (ret < 0) {
			dev_err_ratelimited(priv->dev,
				"%s: bt_swr_clk enable failed\n", __func__);
			goto error;
		}

		if (priv->clk_handle_2x) {
			ret = clk_prepare_enable(priv->clk_handle_2x);
			if (ret < 0) {
				dev_err_ratelimited(priv->dev,
				  "%s: bt_swr_2x_clk enable failed\n", __func__);
				clk_disable_unprepare(priv->clk_handle);
			}
		}
	} else {
		clk_disable_unprepare(priv->clk_handle);
		if (priv->clk_handle_2x)
			clk_disable_unprepare(priv->clk_handle_2x);
	}

error:
	return ret;
}

static int lpass_bt_swrm_clock(void *handle, bool enable)
{
	struct lpass_bt_swr_priv *priv = (struct lpass_bt_swr_priv *) handle;
	int ret = 0;

	mutex_lock(&priv->swr_clk_lock);

	dev_dbg(priv->dev, "%s: swrm clock %s\n",
		__func__, (enable ? "enable" : "disable"));
	if (enable) {
		pm_runtime_get_sync(priv->dev);
		if (priv->swr_clk_users == 0) {
			ret = msm_cdc_pinctrl_select_active_state(
						priv->bt_swr_gpio_p);
			if (ret < 0) {
				dev_err_ratelimited(priv->dev,
					"%s: bt swr pinctrl enable failed\n",
					__func__);
				pm_runtime_mark_last_busy(priv->dev);
				pm_runtime_put_autosuspend(priv->dev);
				goto exit;
			}
			ret = lpass_bt_swr_mclk_enable(priv, true);
			if (ret < 0) {
				msm_cdc_pinctrl_select_sleep_state(
						priv->bt_swr_gpio_p);
				dev_err_ratelimited(priv->dev,
					"%s: lpass bt swr request clock enable failed\n",
					__func__);
				pm_runtime_mark_last_busy(priv->dev);
				pm_runtime_put_autosuspend(priv->dev);
				goto exit;
			}
		}
		priv->swr_clk_users++;
		pm_runtime_mark_last_busy(priv->dev);
		pm_runtime_put_autosuspend(priv->dev);
	} else {
		if (priv->swr_clk_users <= 0) {
			dev_err_ratelimited(priv->dev, "%s: clock already disabled\n",
			__func__);
			priv->swr_clk_users = 0;
			goto exit;
		}
		priv->swr_clk_users--;
		if (priv->swr_clk_users == 0) {
			lpass_bt_swr_mclk_enable(priv, false);
			ret = msm_cdc_pinctrl_select_sleep_state(
						priv->bt_swr_gpio_p);
			if (ret < 0) {
				dev_err_ratelimited(priv->dev,
					"%s: bt swr pinctrl disable failed\n",
					__func__);
				goto exit;
			}
		}
	}
	dev_dbg(priv->dev, "%s: swrm clock users %d\n",
		__func__, priv->swr_clk_users);
exit:
	mutex_unlock(&priv->swr_clk_lock);
	return ret;
}

static void lpass_bt_swr_ssr_disable(struct device *dev, void *data)
{
	struct lpass_bt_swr_priv *priv = data;

	if (!priv->dev_up) {
		dev_err_ratelimited(priv->dev,
				    "%s: already disabled\n", __func__);
		return;
	}

	mutex_lock(&priv->ssr_lock);
	priv->dev_up = false;
	mutex_unlock(&priv->ssr_lock);

	swrm_wcd_notify(priv->swr_ctrl_data->lpass_bt_swr_pdev,
				 SWR_DEVICE_SSR_DOWN, NULL);

}

static int lpass_bt_swr_ssr_enable(struct device *dev, void *data)
{
	struct lpass_bt_swr_priv *priv = data;
	int ret;

	if (priv->initial_boot) {
		priv->initial_boot = false;
		return 0;
	}

	mutex_lock(&priv->ssr_lock);
	priv->dev_up = true;
	mutex_unlock(&priv->ssr_lock);

	mutex_lock(&priv->swr_clk_lock);

	dev_dbg(priv->dev, "%s: swrm clock users %d\n",
		__func__, priv->swr_clk_users);

	if (priv->swr_clk_users > 0) {
		lpass_bt_swr_mclk_enable(priv, false);
		ret = msm_cdc_pinctrl_select_sleep_state(
					priv->bt_swr_gpio_p);
		if (ret < 0) {
			dev_err_ratelimited(priv->dev,
				"%s: bt swr pinctrl disable failed\n",
				__func__);
		}

		lpass_bt_swr_mclk_enable(priv, true);
		ret = msm_cdc_pinctrl_select_active_state(
					priv->bt_swr_gpio_p);
		if (ret < 0) {
			dev_err_ratelimited(priv->dev,
				"%s: bt swr pinctrl enable failed\n",
				__func__);
		}
	}
	mutex_unlock(&priv->swr_clk_lock);

	swrm_wcd_notify(priv->swr_ctrl_data->lpass_bt_swr_pdev,
				 SWR_DEVICE_SSR_UP, NULL);

	return 0;
}

static const struct snd_event_ops lpass_bt_swr_ssr_ops = {
	.enable = lpass_bt_swr_ssr_enable,
	.disable = lpass_bt_swr_ssr_disable,
};

static int lpass_bt_swr_probe(struct platform_device *pdev)
{
	struct lpass_bt_swr_priv *priv;
	int ret;
	struct clk *lpass_core_hw_vote = NULL;
	struct clk *lpass_audio_hw_vote = NULL;
	struct clk *bt_swr_clk = NULL;
	struct clk *bt_swr_2x_clk = NULL;

	priv = devm_kzalloc(&pdev->dev, sizeof(struct lpass_bt_swr_priv),
			    GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	BLOCKING_INIT_NOTIFIER_HEAD(&priv->notifier);
	priv->dev = &pdev->dev;
	priv->dev_up = true;
	priv->core_hw_vote_count = 0;
	priv->core_audio_vote_count = 0;

	dev_set_drvdata(&pdev->dev, priv);
	mutex_init(&priv->vote_lock);
	mutex_init(&priv->swr_clk_lock);
	mutex_init(&priv->ssr_lock);

	priv->bt_swr_gpio_p = of_parse_phandle(pdev->dev.of_node,
					"qcom,bt-swr-gpios", 0);
	if (!priv->bt_swr_gpio_p) {
		dev_err(&pdev->dev, "%s: swr_gpios handle not provided!\n",
			__func__);
		return -EINVAL;
	}
	if (msm_cdc_pinctrl_get_state(priv->bt_swr_gpio_p) < 0) {
		dev_info(&pdev->dev, "%s: failed to get swr pin state\n",
			__func__);
		return -EPROBE_DEFER;
	}

	/* Register LPASS core hw vote */
	lpass_core_hw_vote = devm_clk_get(&pdev->dev, "lpass_core_hw_vote");
	if (IS_ERR(lpass_core_hw_vote)) {
		ret = PTR_ERR(lpass_core_hw_vote);
		dev_dbg(&pdev->dev, "%s: clk get %s failed %d\n",
			__func__, "lpass_core_hw_vote", ret);
		lpass_core_hw_vote = NULL;
		ret = 0;
	}
	priv->lpass_core_hw_vote = lpass_core_hw_vote;

	/* Register LPASS audio hw vote */
	lpass_audio_hw_vote = devm_clk_get(&pdev->dev, "lpass_audio_hw_vote");
	if (IS_ERR(lpass_audio_hw_vote)) {
		ret = PTR_ERR(lpass_audio_hw_vote);
		dev_dbg(&pdev->dev, "%s: clk get %s failed %d\n",
			__func__, "lpass_audio_hw_vote", ret);
		lpass_audio_hw_vote = NULL;
		ret = 0;
	}
	priv->lpass_audio_hw_vote = lpass_audio_hw_vote;

	/* Register bt swr clk vote */
	bt_swr_clk = devm_clk_get(&pdev->dev, "bt_swr_mclk_clk");
	if (IS_ERR(bt_swr_clk)) {
		ret = PTR_ERR(bt_swr_clk);
		dev_err(&pdev->dev, "%s: clk get %s failed %d\n",
			__func__, "bt_swr_clk", ret);
		return -EINVAL;
	}
	priv->clk_handle = bt_swr_clk;

	/* Register bt swr 2x clk vote */
	bt_swr_2x_clk = devm_clk_get(&pdev->dev, "bt_swr_mclk_clk_2x");
	if (IS_ERR(bt_swr_2x_clk)) {
		ret = PTR_ERR(bt_swr_2x_clk);
		dev_dbg(&pdev->dev, "%s: clk get %s failed %d\n",
			__func__, "bt_swr_2x_clk", ret);
		bt_swr_2x_clk = NULL;
		ret = 0;
	}
	priv->clk_handle_2x = bt_swr_2x_clk;

	/* Add soundwire child devices. */
	INIT_WORK(&priv->lpass_bt_swr_add_child_devices_work,
		 lpass_bt_swr_add_child_devices);

	priv->swr_plat_data.handle = (void *)priv;
	priv->swr_plat_data.read = NULL;
	priv->swr_plat_data.write = NULL;
	priv->swr_plat_data.bulk_write = NULL;
	priv->swr_plat_data.clk = lpass_bt_swrm_clock;
	priv->swr_plat_data.core_vote = lpass_bt_swr_core_vote;
	priv->swr_plat_data.handle_irq = NULL;

	lpass_bt_priv = priv;

	pm_runtime_set_autosuspend_delay(&pdev->dev, LPASS_BT_SWR_AUTO_SUSPEND_DELAY);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
	pm_suspend_ignore_children(&pdev->dev, true);
	pm_runtime_enable(&pdev->dev);

	/* call scheduler to add child devices. */
	schedule_work(&priv->lpass_bt_swr_add_child_devices_work);

	priv->initial_boot = true;
	ret = snd_event_client_register(priv->dev, &lpass_bt_swr_ssr_ops, priv);
	if (!ret) {
		snd_event_notify(priv->dev, SND_EVENT_UP);
		dev_err(&pdev->dev, "%s: Registered SSR ops\n", __func__);
	} else {
		dev_err(&pdev->dev,
			"%s: Registration with SND event FWK failed ret = %d\n",
			__func__, ret);
	}

	return 0;
}

static int lpass_bt_swr_remove(struct platform_device *pdev)
{
	struct lpass_bt_swr_priv *priv = dev_get_drvdata(&pdev->dev);

	if (!priv)
		return -EINVAL;

	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
	of_platform_depopulate(&pdev->dev);
	mutex_destroy(&priv->vote_lock);
	mutex_destroy(&priv->swr_clk_lock);
	mutex_destroy(&priv->ssr_lock);

	return 0;
}

#ifdef CONFIG_PM
int lpass_bt_swr_runtime_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct lpass_bt_swr_priv *priv = platform_get_drvdata(pdev);
	int ret = 0;

	dev_dbg(dev, "%s, enter\n", __func__);
	mutex_lock(&priv->vote_lock);
	if (priv->lpass_core_hw_vote == NULL) {
		dev_dbg(dev, "%s: Invalid lpass core hw node\n", __func__);
		goto audio_vote;
	}

	if (priv->core_hw_vote_count == 0) {
		ret = digital_cdc_rsc_mgr_hw_vote_enable(priv->lpass_core_hw_vote, dev);
		if (ret < 0) {
			dev_err_ratelimited(dev, "%s:lpass core hw enable failed\n",
				__func__);
			goto audio_vote;
		}
	}
	priv->core_hw_vote_count++;

audio_vote:
	if (priv->lpass_audio_hw_vote == NULL) {
		dev_dbg(dev, "%s: Invalid lpass audio hw node\n", __func__);
		goto done;
	}

	if (priv->core_audio_vote_count == 0) {
		ret = digital_cdc_rsc_mgr_hw_vote_enable(priv->lpass_audio_hw_vote, dev);
		if (ret < 0) {
			dev_err_ratelimited(dev, "%s:lpass audio hw enable failed\n",
				__func__);
			goto done;
		}
	}
	priv->core_audio_vote_count++;

done:
	mutex_unlock(&priv->vote_lock);
	dev_dbg(dev, "%s, leave, hw_vote %d, audio_vote %d\n", __func__,
			priv->core_hw_vote_count, priv->core_audio_vote_count);
	pm_runtime_set_autosuspend_delay(priv->dev, LPASS_BT_SWR_AUTO_SUSPEND_DELAY);

	return 0;
}

int lpass_bt_swr_runtime_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct lpass_bt_swr_priv *priv = platform_get_drvdata(pdev);

	dev_dbg(dev, "%s, enter\n", __func__);
	mutex_lock(&priv->vote_lock);
	if (priv->lpass_core_hw_vote != NULL) {
		if (--priv->core_hw_vote_count == 0)
			digital_cdc_rsc_mgr_hw_vote_disable(
					priv->lpass_core_hw_vote, dev);
		if (priv->core_hw_vote_count < 0)
			priv->core_hw_vote_count = 0;
	} else {
		dev_dbg(dev, "%s: Invalid lpass core hw node\n",
			__func__);
	}

	if (priv->lpass_audio_hw_vote != NULL) {
		if (--priv->core_audio_vote_count == 0)
			digital_cdc_rsc_mgr_hw_vote_disable(
					priv->lpass_audio_hw_vote, dev);
		if (priv->core_audio_vote_count < 0)
			priv->core_audio_vote_count = 0;
	} else {
		dev_dbg(dev, "%s: Invalid lpass audio hw node\n",
			__func__);
	}

	mutex_unlock(&priv->vote_lock);
	dev_dbg(dev, "%s, leave, hw_vote %d, audio_vote %d\n", __func__,
		priv->core_hw_vote_count, priv->core_audio_vote_count);

	return 0;
}
#endif /* CONFIG_PM */

static const struct of_device_id lpass_bt_swr_dt_match[] = {
	{.compatible = "qcom,lpass-bt-swr"},
	{}
};
MODULE_DEVICE_TABLE(of, lpass_bt_swr_dt_match);

static const struct dev_pm_ops lpass_bt_swr_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(
		pm_runtime_force_suspend,
		pm_runtime_force_resume
	)
	SET_RUNTIME_PM_OPS(
		lpass_bt_swr_runtime_suspend,
		lpass_bt_swr_runtime_resume,
		NULL
	)
};

static struct platform_driver lpass_bt_swr_drv = {
	.driver = {
		.name = "lpass-bt-swr",
		.pm = &lpass_bt_swr_pm_ops,
		.of_match_table = lpass_bt_swr_dt_match,
		.suppress_bind_attrs = true,
	},
	.probe = lpass_bt_swr_probe,
	.remove = lpass_bt_swr_remove,
};

static int lpass_bt_swr_drv_init(void)
{
	return platform_driver_register(&lpass_bt_swr_drv);
}

static void lpass_bt_swr_drv_exit(void)
{
	platform_driver_unregister(&lpass_bt_swr_drv);
}

static int __init lpass_bt_swr_init(void)
{
	lpass_bt_swr_drv_init();
	return 0;
}
module_init(lpass_bt_swr_init);

static void __exit lpass_bt_swr_exit(void)
{
	lpass_bt_swr_drv_exit();
}
module_exit(lpass_bt_swr_exit);

MODULE_SOFTDEP("pre: bt_fm_swr");
MODULE_DESCRIPTION("LPASS BT SWR driver");
MODULE_LICENSE("GPL");
