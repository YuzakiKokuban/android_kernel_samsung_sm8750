/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024, The Linux Foundation. All rights reserved.
 */
/*
 *  sec_audio_debug.c
 *
 *  Copyright (c) 2024 Samsung Electronics
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#include <linux/init.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/printk.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/samsung/snd_debug_proc.h>
#include "codecs/lpass-cdc/lpass-cdc.h"
#include "codecs/lpass-cdc/lpass-cdc-registers.h"
#include "codecs/lpass-cdc/lpass-cdc-clk-rsc.h"
#include "codecs/lpass-cdc/internal.h"
#include "sec_audio_debug.h"

static int boot_complete;
struct _tx_debug_info tx_debug_info = {0,};
struct _rx_debug_info *rx_debug_info = NULL;

struct lpass_cdc_priv *tx_priv;

struct soc_multi_mixer_control {
	int min, max, platform_max, count;
	unsigned int reg, rreg, shift, rshift, invert;
};

static int snd_soc_info_multi_ext(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_info *ucontrol)
{
	struct soc_multi_mixer_control *mc =
		(struct soc_multi_mixer_control *)kcontrol->private_value;

	ucontrol->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	ucontrol->count = mc->count;
	ucontrol->value.integer.min = 0;
	ucontrol->value.integer.max = mc->platform_max;
	return 0;
}

#define SOC_SINGLE_MULTI_EXT(xname, xreg, xshift, xmax, xinvert, xcount,\
	xhandler_get, xhandler_put) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = snd_soc_info_multi_ext, \
	.get = xhandler_get, .put = xhandler_put, \
	.private_value = (unsigned long)&(struct soc_multi_mixer_control) \
		{.reg = xreg, .shift = xshift, .rshift = xshift, .max = xmax, \
		.count = xcount, .platform_max = xmax, .invert = xinvert} }

void sec_audio_debug_lpass_cdc_reg_print(int startReg, int endReg) 
{
	int i, ret;
	int j = 0;
	int reg_val;
	char str[STRING_BUFFER_SIZE+1];

	if (!tx_priv) {
		pr_err("%s: tx_priv is NULL\n", __func__);
		return;
	}

	if (tx_priv->regmap == NULL) {
		pr_err("%s: regmap is NULL\n", __func__);
		return;
	}

	sdp_info_print("%s\n", (startReg == TX_START_OFFSET) ? ("TX REG") : ("VA REG"));

	for (i = startReg; i <= endReg; i += REGDUMP_PRINT_STRIDE) {
		ret = regmap_read(tx_priv->regmap, i, &reg_val);
		if (ret <0)
			continue;
		else {
			sprintf(&str[j*8], "%04X:%02X ", i, reg_val);

			if (j == LOG_COUNT-1) {
				sdp_info_print("%s\n", str);
				j=0;
				str[0] = '\0';
			} else {
				j++;
			}

			if (i == endReg)
				sdp_info_print("%s\n", str);
		}
	}
}

int sec_audio_debug_tx_status_check(void)
{
	int dmic_cfg, i;
	int dmic_ctl[MAX_CHANNEL_NUM];

	if (!tx_priv) {
		pr_err("%s: tx_priv is NULL\n", __func__);
		return -EINVAL;
	}

	if (tx_priv->regmap == NULL) {
		pr_err("%s: regmap is NULL\n", __func__);
		return -EINVAL;
	}

	for(i=0; i < tx_debug_info.num_mic; i++)
		regmap_read(tx_priv->regmap, LPASS_CDC_VA_TOP_CSR_DMIC0_CTL + (i*4) , &dmic_ctl[i]);

	regmap_read(tx_priv->regmap, LPASS_CDC_VA_TOP_CSR_DMIC_CFG , &dmic_cfg);

	tx_debug_info.config_status = dmic_cfg;

	if (dmic_cfg == 0x80)
		tx_debug_info.config_status = OFF_STATUS;
	else if (dmic_cfg == 0x0)
		tx_debug_info.config_status = NORMAL_STATUS;
	else
		tx_debug_info.config_status = ABNORMAL_STATUS;

	if (tx_debug_info.config_status == NORMAL_STATUS) {
		for(i = 0; i < tx_debug_info.num_mic; i++) {
			if ((dmic_ctl[i] == 0x3) || (dmic_ctl[i] == 0xb))
				tx_debug_info.dmic_status[i] = NORMAL_STATUS;
			else if (!dmic_ctl[i])
				tx_debug_info.dmic_status[i] = OFF_STATUS;
			else
				tx_debug_info.dmic_status[i] = ABNORMAL_STATUS;
		}
	} else if (tx_debug_info.config_status == OFF_STATUS) {
		for(i = 0; i < tx_debug_info.num_mic; i++) {
			tx_debug_info.dmic_status[i] = OFF_STATUS;
		}
	} else {
		for(i = 0; i < tx_debug_info.num_mic; i++) {
			tx_debug_info.dmic_status[i] = ABNORMAL_STATUS;
		}
 	}

	if (tx_debug_info.num_mic == 2)
		tx_debug_info.dmic_status[2] = NA_STATUS;

	if (tx_debug_info.config_status == OFF_STATUS || 
		(tx_debug_info.dmic_status[0] == OFF_STATUS && tx_debug_info.dmic_status[1] == OFF_STATUS
		&& tx_debug_info.dmic_status[2] == OFF_STATUS))
		sdp_info_print("Codec Status OFF\n");
	else if (tx_debug_info.config_status == ABNORMAL_STATUS || tx_debug_info.dmic_status[0] == ABNORMAL_STATUS || 
		tx_debug_info.dmic_status[1] == ABNORMAL_STATUS || tx_debug_info.dmic_status[2] == ABNORMAL_STATUS) {
		sdp_info_print("Codec Status NG\n");
		sdp_info_print("Codec Status Value - DMIC1 : %d DMIC2 : %d DMIC3 : %d DMIC CONFIG : %d\n",
				tx_debug_info.dmic_status[0], tx_debug_info.dmic_status[1],
				tx_debug_info.dmic_status[2], tx_debug_info.config_status);
	} else
		sdp_info_print("Codec Status OK\n");

	return 0;
}

static int sec_audio_debug_tx_config_info_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	int i;

	if (boot_complete != 0) {
		sdp_info_print("Micbias : %s\n", (tx_debug_info.bias_enable == 1) ? ("ON") : ("OFF"));

		for(i = 0; i < tx_debug_info.num_mic; i++)
			sdp_info_print("DMIC%d clk %s\n", i+1, (tx_debug_info.dmic_clk_enable[i] == 1) ? ("ON") : ("OFF"));

		sec_audio_debug_tx_status_check();

		ucontrol->value.integer.value[0] = tx_debug_info.bias_enable;
		ucontrol->value.integer.value[1] = tx_debug_info.dmic_clk_enable[0];
		ucontrol->value.integer.value[2] = tx_debug_info.dmic_clk_enable[1];
		ucontrol->value.integer.value[3] = tx_debug_info.dmic_clk_enable[2];
		ucontrol->value.integer.value[4] = tx_debug_info.dmic_status[0];
		ucontrol->value.integer.value[5] = tx_debug_info.dmic_status[1];
		ucontrol->value.integer.value[6] = tx_debug_info.dmic_status[2];
	} else {
		pr_debug("skip the mixer control init routine for unnecessary log print\n");
		boot_complete = 1;
	}
	return 0;
}

static int sec_audio_debug_tx_config_info_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_audio_debug_rx_config_info_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	if (!rx_debug_info) {
		pr_err("%s: tx_priv is NULL\n", __func__);
		return -EINVAL;
	}

	if (boot_complete == 2) {
		
		sec_audio_debug_rx_status_check(rx_debug_info);

		ucontrol->value.integer.value[0] = rx_debug_info->amp_status[0];
		ucontrol->value.integer.value[1] = rx_debug_info->amp_status[1];
	} else {
		pr_debug("skip the mixer control init routine for unnecessary log print\n");
		boot_complete = 2;
	}

	return 0;
}

static int sec_audio_debug_rx_config_info_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_audio_debug_tx_reg_dump_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_audio_debug_tx_reg_dump_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	sdp_info_print("TX Silence Detection Event raised\n");
	sec_audio_debug_lpass_cdc_reg_print(TX_START_OFFSET, TX_MAX_OFFSET);
	sec_audio_debug_lpass_cdc_reg_print(VA_START_OFFSET, VA_MAX_OFFSET);

	return 0;
}

static int sec_audio_debug_rx_reg_dump_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_audio_debug_rx_reg_dump_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	if (!rx_debug_info) {
		pr_err("%s: tx_priv is NULL\n", __func__);
		return -EINVAL;
	}

	sdp_info_print("RX Silence Detection Event raised\n");
	sec_audio_debug_rx_reg_print(rx_debug_info);
	return 0;
}

static const struct snd_kcontrol_new sec_audio_debug_mixer_controls[] = {
	SOC_SINGLE_EXT("TX_REG_DUMP", SND_SOC_NOPM, 0, 65535, 0,
		sec_audio_debug_tx_reg_dump_get, sec_audio_debug_tx_reg_dump_put),
	SOC_SINGLE_EXT("RX_REG_DUMP", SND_SOC_NOPM, 0, 65535, 0,
		sec_audio_debug_rx_reg_dump_get, sec_audio_debug_rx_reg_dump_put),
	SOC_SINGLE_MULTI_EXT("TX_CONFIG_INFO", SND_SOC_NOPM, 0, 65535, 0, TX_INFO_PARAM_NUM,
		sec_audio_debug_tx_config_info_get, sec_audio_debug_tx_config_info_put),
	SOC_SINGLE_MULTI_EXT("RX_CONFIG_INFO", SND_SOC_NOPM, 0, 65535, 0, RX_INFO_PARAM_NUM,
		sec_audio_debug_rx_config_info_get, sec_audio_debug_rx_config_info_put),
};

void sec_audio_debug_codec_micbias_enable(bool enable)
{
	tx_debug_info.bias_enable = enable;
}
EXPORT_SYMBOL(sec_audio_debug_codec_micbias_enable);

void sec_audio_debug_codec_mic_clk_enable(bool enable, int dmic)
{
	switch (dmic) {
	case 1:
		tx_debug_info.dmic_clk_enable[0] = enable;
		break;
	case 3:
		tx_debug_info.dmic_clk_enable[1] = enable;
		break;
	case 5:
		tx_debug_info.dmic_clk_enable[2] = enable;
		break;
	}
}
EXPORT_SYMBOL(sec_audio_debug_codec_mic_clk_enable);

void sec_audio_debug_rx_mute_control_info(int stream, int mute)
{
	if (!rx_debug_info) {
		pr_err("%s: tx_priv is NULL\n", __func__);
		return;
	}

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		rx_debug_info->rx_playback_mute = mute;
	else
		rx_debug_info->rx_feedback_mute = mute;
	
	sdp_info_print("Rx %s %s\n",
		(stream == SNDRV_PCM_STREAM_PLAYBACK) ? ("Playback") : ("Feedback"),
		(mute == 1) ? ("Mute") : ("Unmute"));
}
EXPORT_SYMBOL(sec_audio_debug_rx_mute_control_info);

int sec_audio_debug_get_tx_component(struct snd_soc_component *component)
{
	pr_info("%s\n", __func__);

	if (!component)
		return -EINVAL;

	tx_priv = snd_soc_component_get_drvdata(component);
	if (!tx_priv)
		return -EINVAL;

	if (!(of_device_is_compatible(tx_priv->dev->of_node, "qcom,lpass-cdc"))) {
		pr_err("%s: invalid codec\n", __func__);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(sec_audio_debug_get_tx_component);

static int sec_audio_debug_platform_probe(struct snd_soc_component *component)
{
	pr_info("%s\n", __func__);

	snd_soc_add_component_controls(component,
				sec_audio_debug_mixer_controls,
			ARRAY_SIZE(sec_audio_debug_mixer_controls));
	
	return 0;
}

static const struct snd_soc_component_driver sec_audio_debug = {
	.name		= "sec-audio-debug",
	.probe		= sec_audio_debug_platform_probe,
};

static int sec_audio_debug_probe(struct platform_device *pdev)
{
	pr_info("%s: dev name %s\n", __func__, dev_name(&pdev->dev));

	rx_debug_info = devm_kzalloc(&pdev->dev, sizeof(struct _rx_debug_info),
			    GFP_KERNEL);
	if (!rx_debug_info)
		return -ENOMEM;

	of_property_read_u32(pdev->dev.of_node,
			"debug,num-mic", &tx_debug_info.num_mic);

	of_property_read_u32(pdev->dev.of_node,
			"debug,num-amp", &rx_debug_info->num_amp);

	return snd_soc_register_component(&pdev->dev,
		&sec_audio_debug, NULL, 0);
}

static int sec_audio_debug_remove(struct platform_device *pdev)
{
	pr_debug("%s\n", __func__);
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

static const struct of_device_id sec_audio_debug_dt_match[] = {
	{.compatible = "samsung,sec-audio-debug"},
	{}
};
MODULE_DEVICE_TABLE(of, sec_audio_debug_dt_match);

static struct platform_driver sec_audio_debug_driver = {
	.driver = {
		.name = "samsung-audio-debug",
		.owner = THIS_MODULE,
		.of_match_table = sec_audio_debug_dt_match,
	},
	.probe = sec_audio_debug_probe,
	.remove = sec_audio_debug_remove,
};

module_platform_driver(sec_audio_debug_driver);

MODULE_DESCRIPTION("Samsung Electronics Audio Debug Driver");
MODULE_LICENSE("GPL");
