// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/regulator/driver.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/component.h>
#include <sound/soc.h>
#include <asoc/wcd-mbhc-v2.h>
#include "wcd939x.h"

#define WCD939X_MAX_MICBIAS 4

struct wcd_reg_priv {
	struct device *dev;
	struct regulator_dev *wcd_reg_rdev[WCD939X_MAX_MICBIAS];
	bool mb_enabled_count[WCD939X_MAX_MICBIAS];
};

static inline bool is_valid_micb(int micb)
{
	if ((micb == MIC_BIAS_1) || (micb == MIC_BIAS_2) ||
			(micb == MIC_BIAS_3) || (micb == MIC_BIAS_4))
		return true;

	return false;
}

static int get_micb_num(struct regulator_dev *rdev)
{
	struct wcd_reg_priv *priv = rdev_get_drvdata(rdev);
	int i = 0;

	for (i = 0; i < WCD939X_MAX_MICBIAS; ++i) {
		if (rdev == priv->wcd_reg_rdev[i])
			return (i + 1);
	}
	return -ENODEV;
}

static int wcd_reg_enable_mb(struct regulator_dev *rdev)
{
	struct wcd_reg_priv *priv = rdev_get_drvdata(rdev);
	int micb = 0, ret;

	micb = get_micb_num(rdev);
	if (is_valid_micb(micb)) {
		ret = wcd939x_micb_external_event(priv->dev, micb, true);
		if (!ret)
			priv->mb_enabled_count[micb - 1] = true;

		return ret;
	}

	return -ENODEV;
}

static int wcd_reg_disable_mb(struct regulator_dev *rdev)
{
	struct wcd_reg_priv *priv = rdev_get_drvdata(rdev);
	int micb = 0;

	micb = get_micb_num(rdev);
	if (is_valid_micb(micb)) {
		wcd939x_micb_external_event(priv->dev, micb, false);
		priv->mb_enabled_count[micb - 1] = false;
	}

	return 0;
}

static int wcd_reg_is_enabled_mb(struct regulator_dev *rdev)
{
	struct wcd_reg_priv *priv = rdev_get_drvdata(rdev);
	int micb = 0;

	micb = get_micb_num(rdev);
	if (is_valid_micb(micb))
		return priv->mb_enabled_count[micb - 1];

	return false;
}


static const struct regulator_ops wcd_reg_mb_ops = {
	.enable		= wcd_reg_enable_mb,
	.disable	= wcd_reg_disable_mb,
	.is_enabled	= wcd_reg_is_enabled_mb,
};

static struct regulator_desc wcd_reg_mb_rdesc[] = {
	{
		.owner		= THIS_MODULE,
		.of_match	= "qcom,wcd-mb1-reg",
		.name		= "wcd-mb1-reg",
		.type		= REGULATOR_VOLTAGE,
		.ops		= &wcd_reg_mb_ops,
	},
	{
		.owner		= THIS_MODULE,
		.of_match	= "qcom,wcd-mb2-reg",
		.name		= "wcd-mb2-reg",
		.type		= REGULATOR_VOLTAGE,
		.ops		= &wcd_reg_mb_ops,
	},
	{
		.owner		= THIS_MODULE,
		.of_match	= "qcom,wcd-mb3-reg",
		.name		= "wcd-mb3-reg",
		.type		= REGULATOR_VOLTAGE,
		.ops		= &wcd_reg_mb_ops,
	},
	{
		.owner		= THIS_MODULE,
		.of_match	= "qcom,wcd-mb4-reg",
		.name		= "wcd-mb4-reg",
		.type		= REGULATOR_VOLTAGE,
		.ops		= &wcd_reg_mb_ops,
	},
};


int wcd_init_mb_regulator(struct device *dev)
{
	struct regulator_config cfg = {};
	struct wcd_reg_priv *wcd_regltr = NULL;
	int rc = 0;
	int i;

	wcd_regltr = devm_kzalloc(dev, sizeof(*wcd_regltr), GFP_KERNEL);
	if (!wcd_regltr)
		return -ENOMEM;

	wcd_regltr->dev = dev;
	cfg.dev = dev;
	cfg.driver_data = wcd_regltr;

	for (i = 0; i < WCD939X_MAX_MICBIAS; i++) {
		wcd_regltr->mb_enabled_count[i] = false;
		wcd_regltr->wcd_reg_rdev[i] = devm_regulator_register(wcd_regltr->dev,
				&wcd_reg_mb_rdesc[i], &cfg);
		if (IS_ERR(wcd_regltr->wcd_reg_rdev[i])) {
			rc = PTR_ERR(wcd_regltr->wcd_reg_rdev[i]);
			wcd_regltr->wcd_reg_rdev[i] = NULL;
		}
		if (rc && rc != -EPROBE_DEFER)
			dev_err(dev, "register wcd-reg-mb%d regulator failed, rc=%d\n",
					i + 1, rc);
	}

	return rc;
}
