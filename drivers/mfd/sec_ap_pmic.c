/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/sec_class.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/irq.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/sec_debug.h>
#include <linux/seq_file.h>
#include <linux/mfd/sec_ap_pmic.h>
#include <trace/events/power.h>
#include <linux/suspend.h>
#if IS_ENABLED(CONFIG_SEC_PM_LOG)
#include <linux/sec_pm_log.h>
#endif

#define WS_LOG_PERIOD	5
#define MAX_WAKE_SOURCES_LEN	256

static struct device *sec_ap_pmic_dev;
static struct sec_ap_pmic_info *sec_ap_pmic_data;

extern void pm_get_active_wakeup_sources(char *pending_wakeup_source, size_t max);

static void wake_sources_print_acquired(void)
{
	char wake_sources_acquired[MAX_WAKE_SOURCES_LEN];

	pm_get_active_wakeup_sources(wake_sources_acquired, MAX_WAKE_SOURCES_LEN);
	pr_info("PM: %s\n", wake_sources_acquired);
}

static void wake_sources_print_acquired_work(struct work_struct *work)
{
	struct sec_ap_pmic_info *info = container_of(to_delayed_work(work),
			struct sec_ap_pmic_info, ws_work);

	wake_sources_print_acquired();
	schedule_delayed_work(&info->ws_work, info->ws_log_period * HZ);
}

static ssize_t manual_reset_show(struct device *in_dev,
				struct device_attribute *attr, char *buf)
{
	int ret = sec_get_s2_reset(SEC_PON_KPDPWR_RESIN);

	pr_info("%s: ret=%d\n", __func__, ret);
	return sprintf(buf, "%d\n", !ret);
}

static ssize_t manual_reset_store(struct device *in_dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	int onoff = 0;

	if (kstrtoint(buf, 10, &onoff))
		return -EINVAL;

	pr_info("%s: onoff=%d\n", __func__, onoff);
#if IS_ENABLED(CONFIG_SEC_CRASHKEY_LONG)
	if (onoff)
		sec_crashkey_long_connect_to_input_evnet();
	else
		sec_crashkey_long_disconnect_from_input_event();
#endif

	return len;
}
static DEVICE_ATTR_RW(manual_reset);

static ssize_t wake_enabled_show(struct device *in_dev,
				struct device_attribute *attr, char *buf)
{
	int en = (sec_get_pm_key_wk_init(SEC_PON_KPDPWR) &&
				sec_get_pm_key_wk_init(SEC_PON_RESIN)) ? 1 : 0;

	return sprintf(buf, "%d\n", en);
}

static ssize_t wake_enabled_store(struct device *in_dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	int onoff;
	int ret;

	if (kstrtoint(buf, 10, &onoff) < 0)
		return -EINVAL;

	pr_info("%s: onoff=%d\n", __func__, onoff);

	ret = sec_set_pm_key_wk_init(SEC_PON_KPDPWR, onoff);
	pr_info("%s: PWR ret=%d\n", __func__, ret);

	ret = sec_set_pm_key_wk_init(SEC_PON_RESIN, onoff);
	pr_info("%s: RESIN ret=%d\n", __func__, ret);

	return len;
}
static DEVICE_ATTR_RW(wake_enabled);

#if IS_ENABLED(CONFIG_SEC_GPIO_DUMP)
static ssize_t gpio_dump_show(struct device *in_dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", (gpio_dump_enabled) ? 1 : 0);
}

static ssize_t gpio_dump_store(struct device *in_dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	int onoff;

	if (kstrtoint(buf, 10, &onoff) < 0)
		return -EINVAL;

	pr_info("%s: onoff=%d\n", __func__, onoff);
	gpio_dump_enabled = (onoff) ? true : false;

	return len;
}
static DEVICE_ATTR_RW(gpio_dump);
#endif

/* VDD/IDDQ info */
#define PARAM0_IVALID	1
#define PARAM0_LESS_THAN_0	2

#define DEFAULT_LEN_STR	1023

#define default_scnprintf(buf, offset, fmt, ...)	\
do {	\
	offset += scnprintf(&(buf)[offset], DEFAULT_LEN_STR - (size_t)offset, \
			fmt, ##__VA_ARGS__);	\
} while (0)

static void check_format(char *buf, ssize_t *size, int max_len_str)
{
	int i = 0, cnt = 0, pos = 0;

	if (!buf || *size <= 0)
		return;

	if (*size >= max_len_str)
		*size = max_len_str - 1;

	while (i < *size && buf[i]) {
		if (buf[i] == '"') {
			cnt++;
			pos = i;
		}

		if ((buf[i] < 0x20) || (buf[i] == 0x5C) || (buf[i] > 0x7E))
			buf[i] = ' ';
		i++;
	}

	if (cnt % 2) {
		if (pos == *size - 1) {
			buf[*size - 1] = '\0';
		} else {
			buf[*size - 1] = '"';
			buf[*size] = '\0';
		}
	}
}

static int get_param0(const char *name)
{
	struct device_node *np = of_find_node_by_path("/soc/sec_ap_param");
	u32 val;
	int ret;

	if (!np) {
		pr_err("No sec_avi_data found\n");
		return -PARAM0_IVALID;
	}

	ret = of_property_read_u32(np, name, &val);
	if (ret) {
		pr_err("failed to get %s from node\n", name);
		return -PARAM0_LESS_THAN_0;
	}

	return val;
}

#define GET_V(A)	((A) / 1000)
#define GET_MV(A)	((A) % 1000)
static ssize_t show_ap_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;

	/* currently, support only for GC_OPV */
	default_scnprintf(buf, info_size, "\"GC_OPV_3\":\"%d.%03d\"", GET_V(get_param0("go")), GET_MV(get_param0("go")));
	default_scnprintf(buf, info_size, ",\"GC_PRM\":\"%d\"",	get_param0("gi"));
	default_scnprintf(buf, info_size, ",\"DOUR\":\"%d\"", get_param0("dour"));
	default_scnprintf(buf, info_size, ",\"DOUB\":\"%d\"", get_param0("doub"));

	check_format(buf, &info_size, DEFAULT_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(ap_info, 0440, show_ap_info, NULL);

#define PBGT_PHOT_TYPE	0
#define PBGT_PHOT_LVL	0
static ssize_t show_phot_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;

	default_scnprintf(buf, info_size, "\"TYPE\":\"%d\"",	PBGT_PHOT_TYPE);
	default_scnprintf(buf, info_size, ",\"COUNT\":\"%d\"",
		sec_ap_pmic_data->ocpw_cnt - sec_ap_pmic_data->ocpw_cnt_reset_offset);
	default_scnprintf(buf, info_size, ",\"TIME\":\"%d\"",
		sec_ap_pmic_data->ocpw_time - sec_ap_pmic_data->ocpw_time_reset_offset);
	default_scnprintf(buf, info_size, ",\"LEVEL\":\"%d\"", PBGT_PHOT_LVL);

	check_format(buf, &info_size, DEFAULT_LEN_STR);

	sec_ap_pmic_data->ocpw_time_reset_offset = sec_ap_pmic_data->ocpw_time;
	sec_ap_pmic_data->ocpw_cnt_reset_offset = sec_ap_pmic_data->ocpw_cnt;

	return info_size;
}
static DEVICE_ATTR(phot_info, 0440, show_phot_info, NULL);

static struct attribute *sec_ap_pmic_attributes[] = {
#if IS_ENABLED(CONFIG_SEC_GPIO_DUMP)
	&dev_attr_gpio_dump.attr,
#endif
	&dev_attr_phot_info.attr,
	&dev_attr_ap_info.attr,
	&dev_attr_manual_reset.attr,
	&dev_attr_wake_enabled.attr,
	NULL,
};

static struct attribute_group sec_ap_pmic_attr_group = {
	.attrs = sec_ap_pmic_attributes,
};

#if IS_ENABLED(CONFIG_SEC_GPIO_DUMP)
static void gpio_state_debug_suspend_trace_probe(void *unused,
					const char *action, int val, bool start)
{
	/* SUSPEND: start(1), val(1), action(machine_suspend) */
	if (gpio_dump_enabled && start && val > 0 && !strcmp("machine_suspend", action)) {
		sec_ap_gpio_debug_print();
		sec_pmic_gpio_debug_print();
	}
}
#endif

static irqreturn_t ocp_warn_irq_thread(int irq, void *irq_data)
{
	struct sec_ap_pmic_info *info = irq_data;
	int warn_state = gpio_get_value(info->ocp_warn_gpio);

#if IS_ENABLED(CONFIG_SEC_PM_LOG)
	if (warn_state == 1) {
		info->ocpw_start_time = ktime_get();
		ss_thermal_print("ocp_warn: %d, %d\n", warn_state, ++(info->ocpw_cnt));
	} else 	if (info->ocpw_start_time) {
		info->ocpw_time += ktime_to_ms(ktime_get() - info->ocpw_start_time);
		ss_thermal_print("ocp_warn: %d, accu(%d ms)\n", warn_state, info->ocpw_time);
		info->ocpw_start_time = 0;
	}
#endif

	return IRQ_HANDLED;
}

static int suspend_resume_pm_event(struct notifier_block *notifier,
		unsigned long pm_event, void *unused)
{
	struct sec_ap_pmic_info *info = container_of(notifier,
			struct sec_ap_pmic_info, sec_pm_debug_nb);

	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		cancel_delayed_work_sync(&info->ws_work);
		break;
	case PM_POST_SUSPEND:
		schedule_delayed_work(&info->ws_work, info->ws_log_period * HZ);
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

static int sec_ap_pmic_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct sec_ap_pmic_info *info;
	int err;

	if (!node) {
		dev_err(&pdev->dev, "device-tree data is missing\n");
		return -ENXIO;
	}

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info) {
		dev_err(&pdev->dev, "%s: Fail to alloc info\n", __func__);
		return -ENOMEM;
	}

	info->ocp_warn_gpio = of_get_named_gpio(node, "sec_pm_debug,ocp_warn_irq", 0);
	if (info->ocp_warn_gpio < 0) {
		pr_err("%s: Error reading irq from dt = %d\n", __func__, info->ocp_warn_gpio);
		return -ENOMEM;
	}

	info->ocp_warn_irq = gpio_to_irq(info->ocp_warn_gpio);
	if (info->ocp_warn_irq > 0) {
		err = request_threaded_irq(info->ocp_warn_irq,
				NULL, ocp_warn_irq_thread,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				"ocp-warn-irq", info);
		if (err) {
			pr_info("%s: Failed to request ocp_warn_irq: %d\n", __func__, err);
			goto err_device_create;
		}
	} else {
		pr_info("%s: Failed to gpio_to_irq: %d\n", __func__, info->ocp_warn_irq);
		err = -1;
		goto err_device_create;
	}

	info->ocpw_cnt = 0;

	platform_set_drvdata(pdev, info);
	info->dev = &pdev->dev;
	sec_ap_pmic_data = info;

#if IS_ENABLED(CONFIG_SEC_CLASS)
	sec_ap_pmic_dev = sec_device_create(NULL, "ap_pmic");

	if (unlikely(IS_ERR(sec_ap_pmic_dev))) {
		pr_err("%s: Failed to create ap_pmic device\n", __func__);
		err = PTR_ERR(sec_ap_pmic_dev);
		goto err_device_create;
	}

	err = sysfs_create_group(&sec_ap_pmic_dev->kobj,
				&sec_ap_pmic_attr_group);
	if (err < 0) {
		pr_err("%s: Failed to create sysfs group\n", __func__);
		goto err_device_create;
	}
#endif

#if IS_ENABLED(CONFIG_SEC_GPIO_DUMP)
	/* Register callback for cheking subsystem stats */
	err = register_trace_suspend_resume(
		gpio_state_debug_suspend_trace_probe, NULL);
	if (err) {
		pr_err("%s: Failed to register suspend trace callback, ret=%d\n",
			__func__, err);
	}
#endif

	/* Set to default logging period (5s) */
	info->ws_log_period = WS_LOG_PERIOD;

	/* Register PM notifier */
	info->sec_pm_debug_nb.notifier_call = suspend_resume_pm_event;
	err = register_pm_notifier(&info->sec_pm_debug_nb);
	if (err) {
		dev_err(info->dev, "%s: failed to register PM notifier(%d)\n",
				__func__, err);
		return err;
	}

	INIT_DELAYED_WORK(&info->ws_work, wake_sources_print_acquired_work);
	schedule_delayed_work(&info->ws_work, info->ws_log_period * HZ);

	pr_info("%s: ap_pmic successfully inited.\n", __func__);

	return 0;

#if IS_ENABLED(CONFIG_SEC_CLASS)
err_device_create:
	sec_device_destroy(sec_ap_pmic_dev->devt);
	return err;
#endif
}

static int sec_ap_pmic_remove(struct platform_device *pdev)
{
#if IS_ENABLED(CONFIG_SEC_GPIO_DUMP)
	int ret;

	ret = unregister_trace_suspend_resume(
		gpio_state_debug_suspend_trace_probe, NULL);
#endif

#if IS_ENABLED(CONFIG_SEC_CLASS)
	if (sec_ap_pmic_dev) {
		sec_device_destroy(sec_ap_pmic_dev->devt);
	}
#endif

	return 0;
}

static const struct of_device_id sec_ap_pmic_match_table[] = {
	{ .compatible = "samsung,sec-ap-pmic" },
	{}
};

static struct platform_driver sec_ap_pmic_driver = {
	.driver = {
		.name = "samsung,sec-ap-pmic",
		.of_match_table = sec_ap_pmic_match_table,
	},
	.probe = sec_ap_pmic_probe,
	.remove = sec_ap_pmic_remove,
};

module_platform_driver(sec_ap_pmic_driver);

MODULE_DESCRIPTION("sec_ap_pmic driver");
MODULE_SOFTDEP("pre: sec_class");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: sec_crashkey_long");
MODULE_SOFTDEP("pre: pm8941-pwrkey");
MODULE_AUTHOR("Jiman Cho <jiman85.cho@samsung.com");
MODULE_LICENSE("GPL");
