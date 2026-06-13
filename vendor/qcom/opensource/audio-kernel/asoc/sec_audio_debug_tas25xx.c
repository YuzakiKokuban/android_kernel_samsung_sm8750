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
#include "sec_audio_debug.h"
#if defined(CONFIG_SND_SOC_TAS25XX)
#include "sec_audio_debug_tas25xx.h"
#include "codecs/tas25xx/inc/tas25xx-regmap.h"
#include "codecs/tas25xx/inc/tas25xx.h"

struct tas25xx_priv *rx_priv;

int sec_audio_debug_rx_status_check(struct _rx_debug_info *debug_info)
{
	struct _rx_debug_info *rx_debug_info;
	int amp_power[MAX_CHANNEL_NUM], amp_mode[MAX_CHANNEL_NUM], amp_init[MAX_CHANNEL_NUM];
	int j;

	rx_debug_info = debug_info;

	for (j = 0; j < rx_debug_info->num_amp; j++) {
		if (rx_priv->platform_data->regmap[j] == NULL) {
			pr_err("%s: regmap is NULL\n", __func__);
			return -EINVAL;
		}
	}

	for (j = 0; j < rx_debug_info->num_amp; j++) {
		rx_priv->read(rx_priv, j, TAS25XX_REG_POWER, &amp_power[j]);
		rx_priv->read(rx_priv, j, TAS25XX_REG_MODE, &amp_mode[j]);
		rx_priv->read(rx_priv, j, TAS25XX_REG_INIT, &amp_init[j]);
	}


	for (j = 0; j < rx_debug_info->num_amp; j++) {
		if (amp_power[j] == 0x0 && amp_init[j] == 0x84) {
			if(amp_mode[j] == 0xc0) {
				rx_debug_info->amp_status[j] = NORMAL_STATUS;
				sdp_info_print("%s AMP OK\n", (j == 0) ? ("RCV") : ("SPK"));
			} else {
				rx_debug_info->amp_status[j]= ABNORMAL_STATUS;
				sdp_info_print("%s AMP NG\n", (j == 0) ? ("RCV") : ("SPK"));
			}
		} else if (amp_power[j] == 0x1 || amp_power[j] == 0x2) {
			rx_debug_info->amp_status[j] = OFF_STATUS;
			sdp_info_print("%s AMP OFF\n", (j == 0) ? ("RCV") : ("SPK"));
		} else {
			rx_debug_info->amp_status[j] = ABNORMAL_STATUS;
			sdp_info_print("%s AMP NG\n", (j == 0) ? ("RCV") : ("SPK"));
		}
	}
	return 0;
}
EXPORT_SYMBOL(sec_audio_debug_rx_status_check);

int sec_audio_debug_rx_reg_print(struct _rx_debug_info *debug_info)
{
	struct _rx_debug_info *rx_debug_info;
	int i, j;
	int k = 0;
	int reg_val;
	char str[STRING_BUFFER_SIZE+1];

	rx_debug_info = debug_info;

	for (j = 0; j < rx_debug_info->num_amp; j++) {
		if (rx_priv->platform_data->regmap[j] == NULL) {
			pr_err("%s: regmap is NULL\n", __func__);
			return -EINVAL;
		}
	}

	for (j = 0 ; j < rx_debug_info->num_amp; j++) {
		sdp_info_print("AMP:REG:CHANNEL : %d\n",j);
		rx_priv->read(rx_priv, j, TAS25XX_REG_POWER, &reg_val);
		sprintf(&str[k*8], "%04X:%02X ", TAS25XX_REG_POWER, reg_val);
		k++;
		rx_priv->read(rx_priv, j, TAS25XX_REG_MODE, &reg_val);
		sprintf(&str[k*8], "%04X:%02X ", TAS25XX_REG_MODE, reg_val);
		k++;
		rx_priv->read(rx_priv, j, TAS25XX_REG_INIT, &reg_val);
		sprintf(&str[k*8], "%04X:%02X ", TAS25XX_REG_INIT, reg_val);
		k++;
		rx_priv->read(rx_priv, j, TAS25XX_REG_TDM, &reg_val);
		sprintf(&str[k*8], "%04X:%02X ", TAS25XX_REG_TDM, reg_val);
		k++;		
		for (i = 0; i < TAS25XX_REG_ADC_INDEX; i++) {
		rx_priv->read(rx_priv, j, i+TAS25XX_REG_ADC, &reg_val);
		sprintf(&str[k*8], "%04X:%02X ", i+TAS25XX_REG_ADC, reg_val);
			k++;
		}
		rx_priv->read(rx_priv, j, i+TAS25XX_REG_NG_LP, &reg_val);
		sprintf(&str[k*8], "%04X:%02X ", TAS25XX_REG_NG_LP, reg_val);
		sdp_info_print("%s\n", str);
		k=0;
		str[0] = '\0';
	}

	return 0;
}
EXPORT_SYMBOL(sec_audio_debug_rx_reg_print);

int sec_audio_debug_get_rx_component(struct snd_soc_component *component)
{
	pr_info("%s\n", __func__);

	if (!component)
		return -EINVAL;

	rx_priv = snd_soc_component_get_drvdata(component);
	if (!rx_priv)
		return -EINVAL;

	return 0;
}
EXPORT_SYMBOL(sec_audio_debug_get_rx_component);
#endif

MODULE_DESCRIPTION("Samsung Electronics Audio Debug Driver");
MODULE_LICENSE("GPL");
