// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2012-2021, The Linux Foundation. All rights reserved.
 */

#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/version.h>
#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinctrl.h>
#endif
#include "dp_power.h"
#include "dp_catalog.h"
#include "dp_debug.h"
#include "dp_pll.h"

#if defined(CONFIG_SECDP)
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include "secdp.h"

#if IS_ENABLED(CONFIG_SBU_SWITCH_CONTROL)// && IS_ENABLED(CONFIG_IF_CB_MANAGER)
#include <linux/usb/typec/manager/if_cb_manager.h>
#endif

#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PTN36502)
#include <linux/combo_redriver/ptn36502.h>
#elif IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
#include <linux/combo_redriver/ps5169.h>
#endif

#define DP_LINK_BW_RBR		0x06
#define DP_LINK_BW_HBR		0x0a
#define DP_LINK_BW_HBR2		0x14    /* 1.2 */
#define DP_LINK_BW_HBR3		0x1e    /* 1.4 */

#endif/*CONFIG_SECDP*/

#define DP_CLIENT_NAME_SIZE	20
#define XO_CLK_KHZ	19200

struct dp_power_private {
	struct dp_parser *parser;
	struct dp_pll *pll;
	struct platform_device *pdev;
	struct clk *pixel_clk_rcg;
	struct clk *pixel_parent;
	struct clk *pixel1_clk_rcg;
	struct clk *xo_clk;
	struct clk *link_clk_rcg;
	struct clk *link_parent;

	struct dp_power dp_power;

	bool core_clks_on;
	bool link_clks_on;
	bool strm0_clks_on;
	bool strm1_clks_on;
	bool strm0_clks_parked;
	bool strm1_clks_parked;
	bool link_clks_parked;
#if defined(CONFIG_SECDP)
	bool aux_pullup_on;
	struct mutex dp_clk_lock;
#if IS_ENABLED(CONFIG_SBU_SWITCH_CONTROL)
	enum sbu_switch_status sbu_status;
#endif

	void (*redrv_onoff)(struct dp_power_private *power,
					bool enable, int lane);
	void (*redrv_aux_ctrl)(struct dp_power_private *power, int cross);
	void (*redrv_notify_linkinfo)(struct dp_power_private *power,
					u32 bw_code, u8 v_level, u8 p_level);
#endif
};

#if defined(CONFIG_SECDP)
#define DP_ENUM_STR(x)	#x

enum redriver_switch_t {
	REDRIVER_SWITCH_UNKNOWN = -1,
	REDRIVER_SWITCH_RESET   =  0,
	REDRIVER_SWITCH_CROSS,
	REDRIVER_SWITCH_THROU,
};
#endif

static int dp_power_regulator_init(struct dp_power_private *power)
{
	int rc = 0, i = 0, j = 0;
	struct platform_device *pdev;
	struct dp_parser *parser;

	parser = power->parser;
	pdev = power->pdev;

	for (i = DP_CORE_PM; !rc && (i < DP_MAX_PM); i++) {
		rc = msm_dss_get_vreg(&pdev->dev,
			parser->mp[i].vreg_config,
			parser->mp[i].num_vreg, 1);
		if (rc) {
			DP_ERR("failed to init vregs for %s\n",
				dp_parser_pm_name(i));
			for (j = i - 1; j >= DP_CORE_PM; j--) {
				msm_dss_get_vreg(&pdev->dev,
				parser->mp[j].vreg_config,
				parser->mp[j].num_vreg, 0);
			}

			goto error;
		}
	}
error:
	return rc;
}

static void dp_power_regulator_deinit(struct dp_power_private *power)
{
	int rc = 0, i = 0;
	struct platform_device *pdev;
	struct dp_parser *parser;

	parser = power->parser;
	pdev = power->pdev;

	for (i = DP_CORE_PM; (i < DP_MAX_PM); i++) {
		rc = msm_dss_get_vreg(&pdev->dev,
			parser->mp[i].vreg_config,
			parser->mp[i].num_vreg, 0);
		if (rc)
			DP_ERR("failed to deinit vregs for %s\n",
				dp_parser_pm_name(i));
	}
}

#if defined(CONFIG_SECDP)
/* factory use only
 * ref: qusb_phy_enable_power()
 */
static int secdp_aux_pullup_vreg_enable(struct dp_power_private *power, bool on)
{
	struct regulator *aux_pu_vreg;
	int rc = 0;

	if (!power || !power->parser) {
		DP_ERR("error! power is null\n");
		goto exit;
	}

	aux_pu_vreg = power->parser->aux_pullup_vreg;
	if (!aux_pu_vreg) {
		DP_ERR("error! vdda33 is null\n");
		goto exit;
	}

	DP_ENTER("on:%d\n", on);

#define QUSB2PHY_3P3_VOL_MIN		3104000 /* uV */
#define QUSB2PHY_3P3_VOL_MAX		3105000 /* uV */
#define QUSB2PHY_3P3_HPM_LOAD		30000	/* uA */

	if (on) {
		if (power->aux_pullup_on) {
			DP_INFO("already on\n");
			goto exit;
		}

		rc = regulator_set_load(aux_pu_vreg, QUSB2PHY_3P3_HPM_LOAD);
		if (rc < 0) {
			DP_ERR("Unable to set HPM of vdda33:%d\n", rc);
			goto exit;
		}

		rc = regulator_set_voltage(aux_pu_vreg, QUSB2PHY_3P3_VOL_MIN,
					QUSB2PHY_3P3_VOL_MAX);
		if (rc) {
			DP_ERR("Unable to set voltage for vdda33:%d\n", rc);
			goto put_vdda33_lpm;
		}

		rc = regulator_enable(aux_pu_vreg);
		if (rc) {
			DP_ERR("Unable to enable vdda33:%d\n", rc);
			goto unset_vdd33;
		}

		DP_INFO("[AUX_PU] on success\n");
		power->aux_pullup_on = true;
	} else {

		rc = regulator_disable(aux_pu_vreg);
		if (rc)
			DP_ERR("Unable to disable vdda33:%d\n", rc);

unset_vdd33:
		rc = regulator_set_voltage(aux_pu_vreg, 0,
				QUSB2PHY_3P3_VOL_MAX);
		if (rc)
			DP_ERR("Unable to set 0 voltage for vdda33:%d\n", rc);

put_vdda33_lpm:
		rc = regulator_set_load(aux_pu_vreg, 0);
		if (!rc)
			DP_INFO("[AUX_PU] off success\n");
		else
			DP_ERR("Unable to set 0 HPM of vdda33:%d\n", rc);

		power->aux_pullup_on = false;
	}

exit:
	return rc;
}
#endif

static void dp_power_phy_gdsc(struct dp_power *dp_power, bool on)
{
	int rc = 0;

	if (IS_ERR_OR_NULL(dp_power->dp_phy_gdsc))
		return;

	DP_ENTER("on:%d\n", on);

	if (on)
		rc = regulator_enable(dp_power->dp_phy_gdsc);
	else
		rc = regulator_disable(dp_power->dp_phy_gdsc);

	if (rc)
		DP_ERR("Fail to %s dp_phy_gdsc regulator ret =%d\n",
				on ? "enable" : "disable", rc);

	DP_LEAVE("rc:%d\n", rc);
}

static int dp_power_regulator_ctrl(struct dp_power_private *power, bool enable)
{
	int rc = 0, i = 0, j = 0;
	struct dp_parser *parser;

	parser = power->parser;

	for (i = DP_CORE_PM; i < DP_MAX_PM; i++) {
		/*
		 * The DP_PLL_PM regulator is controlled by dp_display based
		 * on the link configuration.
		 */
		if (i == DP_PLL_PM) {
			/* DP GDSC vote is needed for new chipsets, define gdsc phandle if needed */
			dp_power_phy_gdsc(&power->dp_power, enable);
			DP_DEBUG("skipping: '%s' vregs for %s\n",
					enable ? "enable" : "disable",
					dp_parser_pm_name(i));
			continue;
		}

		rc = msm_dss_enable_vreg(
			parser->mp[i].vreg_config,
			parser->mp[i].num_vreg, enable);
		if (rc) {
			DP_ERR("failed to '%s' vregs for %s\n",
					enable ? "enable" : "disable",
					dp_parser_pm_name(i));
			if (enable) {
				for (j = i-1; j >= DP_CORE_PM; j--) {
					msm_dss_enable_vreg(
					parser->mp[j].vreg_config,
					parser->mp[j].num_vreg, 0);
				}
			}
			goto error;
		}
	}

#if defined(CONFIG_SECDP)
	secdp_aux_pullup_vreg_enable(power, enable);
#endif

error:
	return rc;
}

static int dp_power_pinctrl_set(struct dp_power_private *power, bool active)
{
	int rc = -EFAULT;
	struct pinctrl_state *pin_state;
	struct dp_parser *parser;

#if IS_ENABLED(CONFIG_SBU_SWITCH_CONTROL)
	return 0;
#endif

	parser = power->parser;

	if (IS_ERR_OR_NULL(parser->pinctrl.pin))
		return 0;

	DP_DEBUG("pinctrl:%d\n", active);

	pin_state = active ? parser->pinctrl.state_active
				: parser->pinctrl.state_suspend;
	if (!IS_ERR_OR_NULL(pin_state)) {
		rc = pinctrl_select_state(parser->pinctrl.pin,
				pin_state);
		if (rc)
			DP_ERR("can not set %s pins\n",
			       active ? "dp_active"
			       : "dp_sleep");
		else
			DP_DEBUG("%s success\n", active ? "dp_active" : "dp_sleep");
	} else {
		DP_ERR("invalid '%s' pinstate\n",
		       active ? "dp_active"
		       : "dp_sleep");
	}

	return rc;
}

static void dp_power_clk_put(struct dp_power_private *power)
{
	enum dp_pm_type module;

	for (module = DP_CORE_PM; module < DP_MAX_PM; module++) {
		struct dss_module_power *pm = &power->parser->mp[module];

		if (!pm->num_clk)
			continue;

		msm_dss_mmrm_deregister(&power->pdev->dev, pm);

		msm_dss_put_clk(pm->clk_config, pm->num_clk);
	}
}

static int dp_power_clk_init(struct dp_power_private *power, bool enable)
{
	int rc = 0;
	struct device *dev;
	enum dp_pm_type module;

	dev = &power->pdev->dev;

	if (enable) {
		for (module = DP_CORE_PM; module < DP_MAX_PM; module++) {
			struct dss_module_power *pm =
				&power->parser->mp[module];

			if (!pm->num_clk)
				continue;

			rc = msm_dss_get_clk(dev, pm->clk_config, pm->num_clk);
			if (rc) {
				DP_ERR("failed to get %s clk. err=%d\n",
					dp_parser_pm_name(module), rc);
				goto exit;
			}
		}

		power->pixel_clk_rcg = clk_get(dev, "pixel_clk_rcg");
		if (IS_ERR(power->pixel_clk_rcg)) {
			DP_ERR("Unable to get DP pixel clk RCG: %ld\n",
					PTR_ERR(power->pixel_clk_rcg));
			rc = PTR_ERR(power->pixel_clk_rcg);
			power->pixel_clk_rcg = NULL;
			goto err_pixel_clk_rcg;
		}

		power->pixel_parent = clk_get(dev, "pixel_parent");
		if (IS_ERR(power->pixel_parent)) {
			DP_ERR("Unable to get DP pixel RCG parent: %ld\n",
					PTR_ERR(power->pixel_parent));
			rc = PTR_ERR(power->pixel_parent);
			power->pixel_parent = NULL;
			goto err_pixel_parent;
		}

		power->xo_clk = clk_get(dev, "rpmh_cxo_clk");
		if (IS_ERR(power->xo_clk)) {
			DP_ERR("Unable to get XO clk: %ld\n", PTR_ERR(power->xo_clk));
			rc = PTR_ERR(power->xo_clk);
			power->xo_clk = NULL;
			goto err_xo_clk;
		}

		if (power->parser->has_mst) {
			power->pixel1_clk_rcg = clk_get(dev, "pixel1_clk_rcg");
			if (IS_ERR(power->pixel1_clk_rcg)) {
				DP_ERR("Unable to get DP pixel1 clk RCG: %ld\n",
						PTR_ERR(power->pixel1_clk_rcg));
				rc = PTR_ERR(power->pixel1_clk_rcg);
				power->pixel1_clk_rcg = NULL;
				goto err_pixel1_clk_rcg;
			}
		}

		power->link_clk_rcg = clk_get(dev, "link_clk_src");
		if (IS_ERR(power->link_clk_rcg)) {
			DP_ERR("Unable to get DP link clk RCG: %ld\n",
					PTR_ERR(power->link_clk_rcg));
			rc = PTR_ERR(power->link_clk_rcg);
			power->link_clk_rcg = NULL;
			goto err_link_clk_rcg;
		}

		/* If link_parent node is available, convert clk rates to HZ for byte2 ops */
		power->pll->clk_factor = 1000;
		power->link_parent = clk_get(dev, "link_parent");
		if (IS_ERR(power->link_parent)) {
			DP_WARN("Unable to get DP link parent: %ld\n",
					PTR_ERR(power->link_parent));
			power->link_parent = NULL;
			power->pll->clk_factor = 1;
		}
	} else {
		if (power->pixel1_clk_rcg)
			clk_put(power->pixel1_clk_rcg);

		if (power->pixel_parent)
			clk_put(power->pixel_parent);

		if (power->pixel_clk_rcg)
			clk_put(power->pixel_clk_rcg);

		if (power->link_parent)
			clk_put(power->link_parent);

		if (power->link_clk_rcg)
			clk_put(power->link_clk_rcg);

		dp_power_clk_put(power);
	}

	return rc;
err_link_clk_rcg:
	if (power->pixel1_clk_rcg)
		clk_put(power->pixel1_clk_rcg);
err_pixel1_clk_rcg:
	clk_put(power->xo_clk);
err_xo_clk:
	clk_put(power->pixel_parent);
err_pixel_parent:
	clk_put(power->pixel_clk_rcg);
err_pixel_clk_rcg:
	dp_power_clk_put(power);
exit:
	return rc;
}

static int dp_power_park_module(struct dp_power_private *power, enum dp_pm_type module)
{
	struct dss_module_power *mp;
	struct clk *clk = NULL;
	int rc = 0;
	bool *parked;

	mp = &power->parser->mp[module];

	if (module == DP_STREAM0_PM) {
		clk = power->pixel_clk_rcg;
		parked = &power->strm0_clks_parked;
	} else if (module == DP_STREAM1_PM) {
		clk = power->pixel1_clk_rcg;
		parked = &power->strm1_clks_parked;
	} else if (module == DP_LINK_PM) {
		clk = power->link_clk_rcg;
		parked = &power->link_clks_parked;
	} else {
		goto exit;
	}

	if (!clk) {
		DP_WARN("clk type %d not supported\n", module);
		rc = -EINVAL;
		goto exit;
	}

	if (!power->xo_clk) {
		rc = -EINVAL;
		goto exit;
	}

	if (*parked)
		goto exit;

	rc = clk_set_parent(clk, power->xo_clk);
	if (rc) {
		DP_ERR("unable to set xo parent on clk %d\n", module);
		goto exit;
	}

	mp->clk_config->rate = XO_CLK_KHZ * 1000;
	rc = msm_dss_clk_set_rate(mp->clk_config, mp->num_clk);
	if (rc) {
		DP_ERR("failed to set clk rate.\n");
		goto exit;
	}

	*parked = true;

exit:
	return rc;
}


static int dp_power_clk_set_rate(struct dp_power_private *power,
		enum dp_pm_type module, bool enable)
{
	int rc = 0;
	struct dss_module_power *mp;
#if defined(CONFIG_SECDP)
	static bool prev[DP_MAX_PM];

	mutex_lock(&power->dp_clk_lock);
#endif
	if (!power) {
		DP_ERR("invalid power data\n");
		rc = -EINVAL;
		goto exit;
	}

	mp = &power->parser->mp[module];

#if defined(CONFIG_SECDP)
	if (prev[module] == enable) {
		DP_DEBUG("%d clk already %s\n", module, enable ? "enabled" : "disabled");
		goto exit;
	}
#endif

	if (enable) {
		rc = msm_dss_clk_set_rate(mp->clk_config, mp->num_clk);
		if (rc) {
			DP_ERR("failed to set clks rate.\n");
			goto exit;
		}

		rc = msm_dss_enable_clk(mp->clk_config, mp->num_clk, 1);
		if (rc) {
			DP_ERR("failed to enable clks\n");
			goto exit;
		}
	} else {
		rc = msm_dss_enable_clk(mp->clk_config, mp->num_clk, 0);
		if (rc) {
			DP_ERR("failed to disable clks\n");
				goto exit;
		}

		dp_power_park_module(power, module);
	}

#if defined(CONFIG_SECDP)
	prev[module] = enable;
#endif
exit:
#if defined(CONFIG_SECDP)
	mutex_unlock(&power->dp_clk_lock);
#endif
	return rc;
}

static bool dp_power_clk_status(struct dp_power *dp_power, enum dp_pm_type pm_type)
{
	struct dp_power_private *power;

	if (!dp_power) {
		DP_ERR("invalid power data\n");
		return false;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);

	if (pm_type == DP_LINK_PM)
		return power->link_clks_on;
	else if (pm_type == DP_CORE_PM)
		return power->core_clks_on;
	else if (pm_type == DP_STREAM0_PM)
		return power->strm0_clks_on;
	else if (pm_type == DP_STREAM1_PM)
		return power->strm1_clks_on;
	else
		return false;
}

static int dp_power_clk_enable(struct dp_power *dp_power,
		enum dp_pm_type pm_type, bool enable)
{
	int rc = 0;
	struct dss_module_power *mp;
	struct dp_power_private *power;

	if (!dp_power) {
		DP_ERR("invalid power data\n");
		rc = -EINVAL;
		goto error;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);

	mp = &power->parser->mp[pm_type];

	if (pm_type >= DP_MAX_PM) {
		DP_ERR("unsupported power module: %s\n",
				dp_parser_pm_name(pm_type));
		return -EINVAL;
	}

	if (enable) {
		if (dp_power_clk_status(dp_power, pm_type)) {
			DP_DEBUG("%s clks already enabled\n", dp_parser_pm_name(pm_type));
			return 0;
		}

		if ((pm_type == DP_CTRL_PM) && (!power->core_clks_on)) {
			DP_DEBUG("Need to enable core clks before link clks\n");

			rc = dp_power_clk_set_rate(power, pm_type, enable);
			if (rc) {
				DP_ERR("failed to enable clks: %s. err=%d\n",
					dp_parser_pm_name(DP_CORE_PM), rc);
				goto error;
			} else {
				power->core_clks_on = true;
			}
		}

		if (pm_type == DP_LINK_PM && power->link_parent) {
			rc = clk_set_parent(power->link_clk_rcg, power->link_parent);
			if (rc) {
				DP_ERR("failed to set link parent\n");
				goto error;
			}
		}
	}

	rc = dp_power_clk_set_rate(power, pm_type, enable);
	if (rc) {
		DP_ERR("failed to '%s' clks for: %s. err=%d\n",
			enable ? "enable" : "disable",
			dp_parser_pm_name(pm_type), rc);
			goto error;
	}

	if (pm_type == DP_CORE_PM)
		power->core_clks_on = enable;
	else if (pm_type == DP_STREAM0_PM)
		power->strm0_clks_on = enable;
	else if (pm_type == DP_STREAM1_PM)
		power->strm1_clks_on = enable;
	else if (pm_type == DP_LINK_PM)
		power->link_clks_on = enable;

	if (pm_type == DP_STREAM0_PM)
		power->strm0_clks_parked = false;
	if (pm_type == DP_STREAM1_PM)
		power->strm1_clks_parked = false;
	if (pm_type == DP_LINK_PM)
		power->link_clks_parked = false;

	/*
	 * This log is printed only when user connects or disconnects
	 * a DP cable. As this is a user-action and not a frequent
	 * usecase, it is not going to flood the kernel logs. Also,
	 * helpful in debugging the NOC issues.
	 */
	DP_INFO("core:%s link:%s strm0:%s strm1:%s\n",
		power->core_clks_on ? "on" : "off",
		power->link_clks_on ? "on" : "off",
		power->strm0_clks_on ? "on" : "off",
		power->strm1_clks_on ? "on" : "off");
error:
	return rc;
}

static int dp_power_request_gpios(struct dp_power_private *power)
{
	int rc = 0, i;
	struct device *dev;
	struct dss_module_power *mp;
	static const char * const gpio_names[] = {
		"aux_enable", "aux_sel", "usbplug_cc",
		"edp_vcc_enable", "edp_backlight_pwr", "edp_pwm_en", "edp_backlight_en",
	};

	if (!power) {
		DP_ERR("invalid power data\n");
		return -EINVAL;
	}

	DP_ENTER("\n");

	dev = &power->pdev->dev;
	mp = &power->parser->mp[DP_CORE_PM];

	for (i = 0; i < ARRAY_SIZE(gpio_names); i++) {
		unsigned int gpio = mp->gpio_config[i].gpio;

		if (gpio_is_valid(gpio)) {
			rc = gpio_request(gpio, gpio_names[i]);
			if (rc) {
				DP_ERR("request %s gpio failed, rc=%d\n",
					       gpio_names[i], rc);
#if defined(CONFIG_SECDP)
				if (!strncmp(gpio_names[i], "usbplug_cc",
						strlen(gpio_names[i]))) {
					DP_ERR("keep going\n");
					continue;
				}
#endif
				goto error;
			}
		}
	}
	DP_LEAVE("\n");
	return 0;
error:
	for (i = 0; i < ARRAY_SIZE(gpio_names); i++) {
		unsigned int gpio = mp->gpio_config[i].gpio;

		if (gpio_is_valid(gpio))
			gpio_free(gpio);
	}
	return rc;
}

static bool dp_power_find_gpio(const char *gpio1, const char *gpio2)
{
	return !!strnstr(gpio1, gpio2, strlen(gpio1));
}

#if !defined(CONFIG_SECDP)
static void dp_power_set_gpio(struct dp_power_private *power, bool flip)
{
	int i;
	struct dss_module_power *mp = &power->parser->mp[DP_CORE_PM];
	struct dss_gpio *config = mp->gpio_config;

	for (i = 0; i <= DP_GPIO_CMN_MAX; i++) {
		if (dp_power_find_gpio(config->gpio_name, "aux-sel"))
			config->value = flip;

		if (gpio_is_valid(config->gpio)) {
			DP_DEBUG("gpio %s, value %d\n", config->gpio_name,
				config->value);

			if (dp_power_find_gpio(config->gpio_name, "aux-en") ||
			    dp_power_find_gpio(config->gpio_name, "aux-sel"))
				gpio_direction_output(config->gpio,
					config->value);
			else
				gpio_set_value(config->gpio, config->value);

		}
		config++;
	}
}
#else
int secdp_power_request_gpios(struct dp_power *dp_power)
{
	int rc;
	struct dp_power_private *power;

	if (!dp_power) {
		DP_ERR("invalid power data\n");
		rc = -EINVAL;
		goto exit;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);
	rc = dp_power_request_gpios(power);

exit:
	return rc;
}

#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PTN36502) || IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
static inline char *secdp_redriver_switch_to_string(int event)
{
	switch (event) {
	case REDRIVER_SWITCH_UNKNOWN:
		return DP_ENUM_STR(REDRIVER_SWITCH_UNKNOWN);
	case REDRIVER_SWITCH_RESET:
		return DP_ENUM_STR(REDRIVER_SWITCH_RESET);
	case REDRIVER_SWITCH_CROSS:
		return DP_ENUM_STR(REDRIVER_SWITCH_CROSS);
	case REDRIVER_SWITCH_THROU:
		return DP_ENUM_STR(REDRIVER_SWITCH_THROU);
	default:
		return "unknown";
	}
}
#endif

#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PTN36502)
static void secdp_ptn36502_aux_ctrl(struct dp_power_private *power, int cross)
{
	DP_DEBUG("cross:%s\n", secdp_redriver_switch_to_string(cross));

	switch (cross) {
	case REDRIVER_SWITCH_CROSS:
		ptn36502_config(AUX_CROSS_MODE, 0);
		break;
	case REDRIVER_SWITCH_THROU:
		ptn36502_config(AUX_THRU_MODE, 0);
		break;
	case REDRIVER_SWITCH_RESET:
		ptn36502_config(SAFE_STATE, 0);
		break;
	default:
		DP_INFO("unknown: %d\n", cross);
		break;
	}
}

static void secdp_ptn36502_onoff(struct dp_power_private *power, bool enable, int lane)
{
	DP_DEBUG("en:%d, lane:%d\n", enable, lane);

	if (enable) {
		int val = -1;

		if (lane == 2)
			ptn36502_config(DP2_LANE_USB3_MODE, 1);
		else if (lane == 4)
			ptn36502_config(DP4_LANE_MODE, 1);
		else {
			DP_ERR("error! unknown lane: %d\n", lane);
			goto exit;
		}

		val = ptn36502_i2c_read(Chip_ID);
		DP_INFO("Chip_ID:  0x%x\n", val);
		val = ptn36502_i2c_read(Chip_Rev);
		DP_INFO("Chip_Rev: 0x%x\n", val);
	} else {
		ptn36502_config(SAFE_STATE, 0);
	}

exit:
	return;
}

static void secdp_ptn36502_notify_linkinfo(struct dp_power_private *power, u32 bw_code, u8 v_level, u8 p_level)
{
	DP_ENTER("0x%x,%d,%d, do nothing!\n", bw_code, v_level, p_level);

	//.TODO:
}
#elif IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
static void secdp_ps5169_aux_ctrl(struct dp_power_private *power, int cross)
{
	/*
	 * ps5169 does not support AUX switching function.
	 * It needs to be done by AUX switch IC
	 */
	DP_ENTER("cross: %s, do nothing!\n",
		secdp_redriver_switch_to_string(cross));
}

static void secdp_ps5169_onoff(struct dp_power_private *power, bool enable, int lane)
{
	DP_DEBUG("en:%d, lane:%d\n", enable, lane);

	if (enable) {
		if (lane == 2)
			ps5169_config(DP2_LANE_USB_MODE, 1);
		else if (lane == 4)
			ps5169_config(DP_ONLY_MODE, 1);
		else {
			DP_ERR("error! unknown lane: %d\n", lane);
			goto exit;
		}

		DP_INFO("Chip_ID1:  0x%x, Chip_Rev1: 0x%x\n",
			ps5169_i2c_read(Chip_ID1), ps5169_i2c_read(Chip_Rev1));
		DP_INFO("Chip_ID2:  0x%x, Chip_Rev2: 0x%x\n",
			ps5169_i2c_read(Chip_ID2), ps5169_i2c_read(Chip_Rev2));
	} else {
		ps5169_config(CLEAR_STATE, 0);
	}

exit:
	return;
}

static void secdp_ps5169_notify_linkinfo(struct dp_power_private *power,
					u32 bw_code, u8 v_level, u8 p_level)
{
	struct dp_parser *parser = power->parser;
	u8 eq0, eq1;

	switch (bw_code) {
	case DP_LINK_BW_RBR:
		eq0 = parser->ps5169_rbr_eq0[v_level][p_level];
		eq1 = parser->ps5169_rbr_eq1[v_level][p_level];
		break;
	case DP_LINK_BW_HBR:
		eq0 = parser->ps5169_hbr_eq0[v_level][p_level];
		eq1 = parser->ps5169_hbr_eq1[v_level][p_level];
		break;
	case DP_LINK_BW_HBR2:
		eq0 = parser->ps5169_hbr2_eq0[v_level][p_level];
		eq1 = parser->ps5169_hbr2_eq1[v_level][p_level];
		break;
	case DP_LINK_BW_HBR3:
	default:
		eq0 = parser->ps5169_hbr3_eq0[v_level][p_level];
		eq1 = parser->ps5169_hbr3_eq1[v_level][p_level];
		break;
	}

	DP_DEBUG("bw:0x%x, v:%d, p:%d, eq0:0x%x, eq1:0x%x\n",
				bw_code, v_level, p_level, eq0, eq1);
	ps5169_notify_dplink(eq0, eq1);
}
#endif

void secdp_redriver_onoff(struct dp_power *dp_power,
					bool enable, int lane)
{
	struct dp_power_private *power;

	power = container_of(dp_power, struct dp_power_private, dp_power);
	if (power && power->redrv_onoff)
		power->redrv_onoff(power, enable, lane);
}

void secdp_redriver_linkinfo(struct dp_power *dp_power,
					u32 rate, u8 v_level, u8 p_level)
{
	struct dp_power_private *power;

	power = container_of(dp_power, struct dp_power_private, dp_power);
	if (power && power->redrv_notify_linkinfo)
		power->redrv_notify_linkinfo(power, rate, v_level, p_level);
}

static void secdp_redriver_register(struct dp_power_private *power)
{
	int use_redrv;

	if (!power || !power->parser) {
		DP_ERR("invalid power!\n");
		goto end;
	}

	use_redrv = power->parser->use_redrv;
	DP_DEBUG("++ use_redrv(%d)\n", use_redrv);

	if (!use_redrv) {
		DP_INFO("nothing registered!\n");
		goto end;
	}

#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PTN36502)
	power->redrv_onoff = secdp_ptn36502_onoff;
	power->redrv_aux_ctrl = secdp_ptn36502_aux_ctrl;
	power->redrv_notify_linkinfo = secdp_ptn36502_notify_linkinfo;
	DP_INFO("ptn36502 API registered!\n");
#elif IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
	power->redrv_onoff = secdp_ps5169_onoff;
	power->redrv_aux_ctrl = secdp_ps5169_aux_ctrl;
	power->redrv_notify_linkinfo = secdp_ps5169_notify_linkinfo;
	DP_INFO("ps5169 API registered!\n");
#endif

end:
	return;
}

#if !IS_ENABLED(CONFIG_SBU_SWITCH_CONTROL)
/* turn on EDP_AUX switch
 * ===================================================
 * | usbplug-cc(dir) | orientation | flip  | aux-sel |
 * ===================================================
 * |        0        |     CC1     | false |    0    |
 * |        1        |     CC2     | true  |    1    |
 * ===================================================
 */
static void _secdp_power_set_gpio(struct dp_power_private *power, bool flip)
{
	int i, rc = 0;
	/*int dir = (flip == false) ? 0 : 1;*/
	struct dss_module_power *mp = &power->parser->mp[DP_CORE_PM];
	struct dss_gpio *config = mp->gpio_config;
	struct dp_parser *parser = power->parser;
	bool sel_val = false;

//	DP_DEBUG("flip:%d, aux_inv:%d, redrv:%d\n",
//		flip, parser->aux_sel_inv, parser->use_redrv);

	if (parser->aux_sel_inv)
		sel_val = true;

	for (i = 0; i <= DP_GPIO_CMN_MAX; i++) {
		if (gpio_is_valid(config->gpio)) {
			if (dp_power_find_gpio(config->gpio_name, "aux-sel")) {
				if (parser->use_redrv == SECDP_REDRV_PTN36502) {
					rc = gpio_direction_output(config->gpio, 0);
				} else {
					/* SECDP_REDRV_PS5169 or SECDP_REDRV_NONE */
					bool val = (bool)gpio_get_value(config->gpio);

					if ((!flip && (val == sel_val)) ||
							(flip && (val == !sel_val))) {
						DP_DEBUG("%s: already %d %d, skip\n",
							config->gpio_name, flip, val);
						break;
					}
					rc = gpio_direction_output(config->gpio,
						(!flip ? sel_val : !sel_val));
				}
				usleep_range(100, 120);
				DP_INFO("[aux-sel] set %d (f:%d,i:%d,r:%d) %d\n",
					gpio_get_value(config->gpio),
					flip, parser->aux_sel_inv, parser->use_redrv, rc);
				break;
			}
		}
		config++;
	}

	usleep_range(100, 120);
	config = mp->gpio_config;
	for (i = 0; i <= DP_GPIO_CMN_MAX; i++) {
		if (gpio_is_valid(config->gpio)) {
			if (dp_power_find_gpio(config->gpio_name, "aux-en")) {
				if (!gpio_get_value(config->gpio)) {
					DP_DEBUG("%s: already enabled, skip\n",
						config->gpio_name);
					break;
				}
				rc = gpio_direction_output(config->gpio, 0);
				usleep_range(100, 120);
				DP_INFO("[aux-en] set %d (f:%d,i:%d,r:%d) %d\n",
					gpio_get_value(config->gpio),
					flip, parser->aux_sel_inv, parser->use_redrv, rc);
				break;
			}
		}
		config++;
	}
}

/* turn off EDP_AUX switch */
static void _secdp_power_unset_gpio(struct dp_power_private *power)
{
	int i, rc = 0;
	struct dss_module_power *mp = &power->parser->mp[DP_CORE_PM];
	struct dss_gpio *config = mp->gpio_config;

	DP_ENTER("\n");

	for (i = 0; i < mp->num_gpio; i++) {
		if (gpio_is_valid(config->gpio)) {
			if (dp_power_find_gpio(config->gpio_name, "aux-en")) {
				if (gpio_get_value(config->gpio)) {
					DP_DEBUG("%s: already disabled, skip\n",
						config->gpio_name);
					break;
				}
				rc = gpio_direction_output(config->gpio, 1);
				usleep_range(100, 120);
				DP_INFO("[aux-en] set %d, %d\n",
					gpio_get_value(config->gpio), rc);
				break;
			}
		}
		config++;
	}

	config = mp->gpio_config;
	for (i = 0; i < mp->num_gpio; i++) {
		if (gpio_is_valid(config->gpio)) {
			if (dp_power_find_gpio(config->gpio_name, "aux-sel")) {
				if (!gpio_get_value(config->gpio)) {
					DP_DEBUG("%s: already 0, skip\n",
						config->gpio_name);
					break;
				}
				rc = gpio_direction_output(config->gpio, 0);
				usleep_range(100, 120);
				DP_INFO("[aux-sel] set %d, %d\n",
					gpio_get_value(config->gpio), rc);
				break;
			}
		}
		config++;
	}
}
#else/*CONFIG_SBU_SWITCH_CONTROL*/
static void _secdp_sbu_switch_on(struct dp_power_private *power, bool flip)
{
	int cc_sbu = !flip ? CLOSE_SBU_CC1_ACTIVE : CLOSE_SBU_CC2_ACTIVE;

	if (power->sbu_status == cc_sbu)
		return;

	usbpd_sbu_switch_control(cc_sbu);
	power->sbu_status = cc_sbu;
}

static void _secdp_sbu_switch_off(struct dp_power_private *power)
{
	if (power->sbu_status == OPEN_SBU)
		return;

	usbpd_sbu_switch_control(OPEN_SBU);
	power->sbu_status = OPEN_SBU;
}
#endif

void secdp_power_set_gpio(struct dp_power *dp_power, bool flip)
{
	struct dp_power_private *power;

	power = container_of(dp_power, struct dp_power_private, dp_power);

#if !IS_ENABLED(CONFIG_SBU_SWITCH_CONTROL)
	_secdp_power_set_gpio(power, flip);
#else
	_secdp_sbu_switch_on(power, flip);
#endif
}

void secdp_power_unset_gpio(struct dp_power *dp_power)
{
	struct dp_power_private *power;

	power = container_of(dp_power, struct dp_power_private, dp_power);

#if !IS_ENABLED(CONFIG_SBU_SWITCH_CONTROL)
	_secdp_power_unset_gpio(power);
#else
	_secdp_sbu_switch_off(power);
#endif
}

#if defined(CONFIG_SECDP_FACTORY_DPSWITCH_TEST)
static void secdp_redriver_aux_ctrl(struct dp_power_private *power,
					int cross)
{
	if (power && power->redrv_aux_ctrl)
		power->redrv_aux_ctrl(power, cross);
}

/*
 * @aux_sel : 1 or 0
 */
void secdp_config_gpios_factory(struct dp_power *dp_power, int aux_sel, bool on)
{
	struct dp_power_private *power;

	power = container_of(dp_power, struct dp_power_private, dp_power);

	DP_DEBUG("sel:%d, on:%d\n", aux_sel, on);

	if (on) {
		secdp_aux_pullup_vreg_enable(power, true);
		secdp_power_set_gpio(dp_power, aux_sel);

		if (aux_sel == 1)
			secdp_redriver_aux_ctrl(power, REDRIVER_SWITCH_CROSS);
		else if (aux_sel == 0)
			secdp_redriver_aux_ctrl(power, REDRIVER_SWITCH_THROU);
		else
			DP_ERR("unknown <%d>\n", aux_sel);
	} else {
		secdp_redriver_aux_ctrl(power, REDRIVER_SWITCH_RESET);
		secdp_power_unset_gpio(dp_power);
		secdp_aux_pullup_vreg_enable(power, false);
	}
}
#endif

enum dp_hpd_plug_orientation secdp_get_plug_orientation(struct dp_power *dp_power)
{
	struct dp_power_private *power;
	struct dp_parser *parser;
	struct dss_module_power *mp;
	struct dss_gpio *config;
	int i, dir;

	power = container_of(dp_power, struct dp_power_private, dp_power);
	parser = power->parser;
	mp = &power->parser->mp[DP_CORE_PM];
	config = mp->gpio_config;

	for (i = 0; i < mp->num_gpio; i++) {
		if (gpio_is_valid(config->gpio)) {
			if (dp_power_find_gpio(config->gpio_name,
					"usbplug-cc")) {
				dir = gpio_get_value(config->gpio);
				if (parser->cc_dir_inv)
					dir = !dir;
				DP_INFO("cc_dir_inv:%d, orientation:%s\n",
					parser->cc_dir_inv, !dir ? "CC1" : "CC2");
				if (dir == 0)
					return ORIENTATION_CC1;
				else /* if (dir == 1) */
					return ORIENTATION_CC2;
			}
		}
		config++;
	}

	/*cannot be here*/
	return ORIENTATION_NONE;
}
#endif

static int dp_power_config_gpios(struct dp_power_private *power, bool flip,
					bool enable)
{
#if !defined(CONFIG_SECDP)
	int rc = 0, i;
	struct dss_module_power *mp;
	struct dss_gpio *config;

	mp = &power->parser->mp[DP_CORE_PM];
	config = mp->gpio_config;

	if (enable) {
		rc = dp_power_request_gpios(power);
		if (rc) {
			DP_ERR("gpio request failed\n");
			return rc;
		}

		dp_power_set_gpio(power, flip);
	} else {
		for (i = 0; i < mp->num_gpio; i++) {
			if (gpio_is_valid(config[i].gpio)) {
				gpio_set_value(config[i].gpio, 0);
				gpio_free(config[i].gpio);
			}
		}
	}
#else
	struct dp_power *dp_power = &power->dp_power;

	if (enable)
		secdp_power_set_gpio(dp_power, flip);
	else
		secdp_power_unset_gpio(dp_power);
#endif

	return 0;
}

static int dp_power_mmrm_init(struct dp_power *dp_power, struct sde_power_handle *phandle, void *dp,
	int (*dp_display_mmrm_callback)(struct mmrm_client_notifier_data *notifier_data))
{
	int rc = 0;
	enum dp_pm_type module;
	struct dp_power_private *power = container_of(dp_power, struct dp_power_private, dp_power);
	struct device *dev = &power->pdev->dev;

	for (module = DP_CORE_PM; module < DP_MAX_PM; module++) {
		struct dss_module_power *pm = &power->parser->mp[module];
		if (!pm->num_clk)
			continue;

		rc = msm_dss_mmrm_register(dev, pm, dp_display_mmrm_callback,
					dp, &phandle->mmrm_enable);
		if (rc)
			DP_ERR("mmrm register failed rc=%d\n", rc);
	}

	return rc;
}

static int dp_power_client_init(struct dp_power *dp_power,
	struct sde_power_handle *phandle, struct drm_device *drm_dev)
{
	int rc = 0;
	struct dp_power_private *power;

	if (!drm_dev) {
		DP_ERR("invalid drm_dev\n");
		return -EINVAL;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);

	rc = dp_power_regulator_init(power);
	if (rc) {
		DP_ERR("failed to init regulators\n");
		goto error_power;
	}

	rc = dp_power_clk_init(power, true);
	if (rc) {
		DP_ERR("failed to init clocks\n");
		goto error_clk;
	}
	dp_power->phandle = phandle;
	dp_power->drm_dev = drm_dev;

#if defined(CONFIG_SECDP)
	rc = dp_power_pinctrl_set(power, false);
	if (rc) {
		DP_ERR("failed to set pinctrl state\n");
		goto error_client;
	}
#endif
	return 0;

#if defined(CONFIG_SECDP)
error_client:
	dp_power_clk_init(power, false);
#endif
error_clk:
	dp_power_regulator_deinit(power);
error_power:
	return rc;
}

static void dp_power_client_deinit(struct dp_power *dp_power)
{
	struct dp_power_private *power;

	if (!dp_power) {
		DP_ERR("invalid power data\n");
		return;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);

	dp_power_clk_init(power, false);
	dp_power_regulator_deinit(power);
}

static int dp_power_park_clocks(struct dp_power *dp_power)
{
	int rc = 0;
	struct dp_power_private *power;

	if (!dp_power) {
		DP_ERR("invalid power data\n");
		return -EINVAL;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);

	rc = dp_power_park_module(power, DP_STREAM0_PM);
	if (rc) {
		DP_ERR("failed to park stream 0. err=%d\n", rc);
		goto error;
	}

	rc = dp_power_park_module(power, DP_STREAM1_PM);
	if (rc) {
		DP_ERR("failed to park stream 1. err=%d\n", rc);
		goto error;
	}

	rc = dp_power_park_module(power, DP_LINK_PM);
	if (rc) {
		DP_ERR("failed to park link clock. err=%d\n", rc);
		goto error;
	}

error:
	return rc;
}

static int dp_power_set_pixel_clk_parent(struct dp_power *dp_power, u32 strm_id)
{
	int rc = 0;
	struct dp_power_private *power;

	if (!dp_power || strm_id >= DP_STREAM_MAX) {
		DP_ERR("invalid power data. stream %d\n", strm_id);
		rc = -EINVAL;
		goto exit;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);

	if (strm_id == DP_STREAM_0) {
		if (power->pixel_clk_rcg && power->pixel_parent)
			rc = clk_set_parent(power->pixel_clk_rcg,
					power->pixel_parent);
		else
			DP_WARN("skipped for strm_id=%d\n", strm_id);
	} else if (strm_id == DP_STREAM_1) {
		if (power->pixel1_clk_rcg && power->pixel_parent)
			rc = clk_set_parent(power->pixel1_clk_rcg,
					power->pixel_parent);
		else
			DP_WARN("skipped for strm_id=%d\n", strm_id);
	}

	if (rc)
		DP_ERR("failed. strm_id=%d, rc=%d\n", strm_id, rc);
exit:
	return rc;
}

static u64 dp_power_clk_get_rate(struct dp_power *dp_power, char *clk_name)
{
	size_t i;
	enum dp_pm_type j;
	struct dss_module_power *mp;
	struct dp_power_private *power;
	bool clk_found = false;
	u64 rate = 0;

	if (!clk_name) {
		DP_ERR("invalid pointer for clk_name\n");
		return 0;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);
	mp = &dp_power->phandle->mp;
	for (i = 0; i < mp->num_clk; i++) {
		if (!strcmp(mp->clk_config[i].clk_name, clk_name)) {
			rate = clk_get_rate(mp->clk_config[i].clk);
			clk_found = true;
			break;
		}
	}

	for (j = DP_CORE_PM; j < DP_MAX_PM && !clk_found; j++) {
		mp = &power->parser->mp[j];
		for (i = 0; i < mp->num_clk; i++) {
			if (!strcmp(mp->clk_config[i].clk_name, clk_name)) {
				rate = clk_get_rate(mp->clk_config[i].clk);
				clk_found = true;
				break;
			}
		}
	}

	return rate;
}

static int dp_power_init(struct dp_power *dp_power, bool flip)
{
	int rc = 0;
	struct dp_power_private *power;

	if (!dp_power) {
		DP_ERR("invalid power data\n");
		rc = -EINVAL;
		goto exit;
	}

	DP_ENTER("\n");

	power = container_of(dp_power, struct dp_power_private, dp_power);

	rc = dp_power_regulator_ctrl(power, true);
	if (rc) {
		DP_ERR("failed to enable regulators\n");
		goto exit;
	}

	rc = dp_power_pinctrl_set(power, true);
	if (rc) {
		DP_ERR("failed to set pinctrl state\n");
		goto err_pinctrl;
	}

	rc = dp_power_config_gpios(power, flip, true);
	if (rc) {
		DP_ERR("failed to enable gpios\n");
		goto err_gpio;
	}

	rc = pm_runtime_resume_and_get(dp_power->drm_dev->dev);
	if (rc < 0) {
		DP_ERR("failed to enable power resource %d\n", rc);
		goto err_sde_power;
	}

	rc = dp_power_clk_enable(dp_power, DP_CORE_PM, true);
	if (rc) {
		DP_ERR("failed to enable DP core clocks\n");
		goto err_clk;
	}

	return 0;

err_clk:
	pm_runtime_put_sync(dp_power->drm_dev->dev);
err_sde_power:
	dp_power_config_gpios(power, flip, false);
err_gpio:
	dp_power_pinctrl_set(power, false);
err_pinctrl:
	dp_power_regulator_ctrl(power, false);
exit:
	return rc;
}

static int dp_power_deinit(struct dp_power *dp_power)
{
	int rc = 0;
	struct dp_power_private *power;

	if (!dp_power) {
		DP_ERR("invalid power data\n");
		rc = -EINVAL;
		goto exit;
	}

	DP_ENTER("\n");

	power = container_of(dp_power, struct dp_power_private, dp_power);

	if (power->link_clks_on)
		dp_power_clk_enable(dp_power, DP_LINK_PM, false);

	dp_power_clk_enable(dp_power, DP_CORE_PM, false);
	pm_runtime_put_sync(dp_power->drm_dev->dev);

	dp_power_config_gpios(power, false, false);
	dp_power_pinctrl_set(power, false);
	dp_power_regulator_ctrl(power, false);
exit:
	return rc;
}

static int dp_power_edp_panel_set_gpio(struct dp_power *dp_power,
		enum dp_pin_states pin_state, bool enable)
{
	int rc = 0;
	struct dp_power_private *power;
	struct dss_module_power *mp;
	struct dss_gpio *config;

	if (!dp_power) {
		DP_ERR("invalid power data\n");
		return -EINVAL;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);

	mp = &power->parser->mp[DP_CORE_PM];
	config = mp->gpio_config;

	if (config == NULL)
		return -EINVAL;

	if ((pin_state >= DP_GPIO_EDP_MIN) && (pin_state < DP_GPIO_EDP_MAX)) {
		gpio_direction_output(config[pin_state].gpio, enable);
	} else {
		DP_ERR("Invalid GPIO call with pin state: %d\n", pin_state);
		return -EINVAL;
	}

	return rc;
}

struct dp_power *dp_power_get(struct dp_parser *parser, struct dp_pll *pll)
{
	int rc = 0;
	struct dp_power_private *power;
	struct dp_power *dp_power;
	struct device *dev;

	if (!parser || !pll) {
		DP_ERR("invalid input\n");
		rc = -EINVAL;
		goto error;
	}

	power = kzalloc(sizeof(*power), GFP_KERNEL);
	if (!power) {
		rc = -ENOMEM;
		goto error;
	}

	power->parser = parser;
	power->pll = pll;
	power->pdev = parser->pdev;

	dp_power = &power->dp_power;
	dev = &power->pdev->dev;

	dp_power->init = dp_power_init;
	dp_power->deinit = dp_power_deinit;
	dp_power->clk_enable = dp_power_clk_enable;
	dp_power->clk_status = dp_power_clk_status;
	dp_power->set_pixel_clk_parent = dp_power_set_pixel_clk_parent;
	dp_power->park_clocks = dp_power_park_clocks;
	dp_power->clk_get_rate = dp_power_clk_get_rate;
	dp_power->power_client_init = dp_power_client_init;
	dp_power->power_client_deinit = dp_power_client_deinit;
	dp_power->power_mmrm_init = dp_power_mmrm_init;
	dp_power->edp_panel_set_gpio = dp_power_edp_panel_set_gpio;

	dp_power->dp_phy_gdsc = devm_regulator_get(dev, "dp_phy_gdsc");
	if (IS_ERR(dp_power->dp_phy_gdsc)) {
		dp_power->dp_phy_gdsc = NULL;
		DP_DEBUG("Optional GDSC regulator is missing\n");
	}

#if defined(CONFIG_SECDP)
	secdp_redriver_register(power);
	mutex_init(&power->dp_clk_lock);
#endif

	return dp_power;
error:
	return ERR_PTR(rc);
}

void dp_power_put(struct dp_power *dp_power)
{
	struct dp_power_private *power = NULL;

	if (!dp_power)
		return;

	power = container_of(dp_power, struct dp_power_private, dp_power);

	kfree(power);
}
