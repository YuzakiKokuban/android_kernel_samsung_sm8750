/*
 * =============================================================================
 * Copyright (c) 2016  Texas Instruments Inc.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.See the GNU General Public License for more details.
 *
 * File:
 *     tas25xx-codec.c
 *
 * Description:
 *     ALSA SoC driver for Texas Instruments TAS25XX High Performance 4W Smart
 *     Amplifier
 *
 * =============================================================================
 */

#define DEBUG 5
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/version.h>

#include "../inc/tas25xx.h"
#include "../inc/tas25xx-ext.h"
#include "../inc/tas25xx-device.h"
#include "../inc/tas25xx-logic.h"
#include "../inc/tas25xx-regmap.h"
#include "../inc/tas25xx-regbin-parser.h"
#if IS_ENABLED(CONFIG_TAS25XX_ALGO)
#include "../algo/inc/tas_smart_amp_v2.h"
#include "../algo/inc/tas25xx-calib.h"
#endif /*CONFIG_TAS25XX_ALGO*/
#if defined(CONFIG_SEC_AUDIO_DEBUG)
#include "../../../sec_audio_debug.h"
#endif

#define TAS25XX_FW_UPDATE_SUCCESS 1

static const char *irq_gpio_label[2] = {
	"TAS25XX-IRQ", "TAS25XX-IRQ2"
};

static struct tas25xx_priv *s_tas25xx;

static int tas25xx_start_fw_load(struct tas25xx_priv *p_tas25xx, int retry_count);
static bool fw_load_required(struct tas25xx_priv *p_tas25xx);

static unsigned int tas25xx_codec_read(struct snd_soc_component *codec,
		unsigned int reg)
{
	unsigned int value = 0, dev_reg;
	struct tas25xx_priv *p_tas25xx = snd_soc_component_get_drvdata(codec);
	int ret = -1, channel;
	struct linux_platform *plat_data =
		(struct linux_platform *) p_tas25xx->platform_data;

	switch (reg) {
	case TAS25XX_SWITCH:
		dev_dbg(plat_data->dev, "%s: reg=0x%08x TAS25XX_SWITCH",
			__func__, reg);
		value = p_tas25xx->device_used;
		break;

	default:
		dev_reg = reg & 0xffffff;
		channel = (reg >> 24) & 0x3;
		if (channel >= p_tas25xx->ch_count)
			return -EINVAL;
		dev_dbg(plat_data->dev, "%s: ch=%d dev_reg=0x%06x, default read",
			__func__, channel, dev_reg);
		ret = p_tas25xx->read(p_tas25xx, channel, dev_reg, &value);
		break;
	}

	dev_dbg(plat_data->dev, "%s, reg=0x%08x, value=0x%x",
		__func__, reg, value);

	if (ret == 0)
		return value;
	else
		return ret;
}

static int tas25xx_codec_write(struct snd_soc_component *codec,
	unsigned int reg, unsigned int value)
{
	struct tas25xx_priv *p_tas25xx = snd_soc_component_get_drvdata(codec);
	struct linux_platform *plat_data =
		(struct linux_platform *) p_tas25xx->platform_data;
	int ret = 0;

	switch (reg) {
	case TAS25XX_SWITCH:
		dev_dbg(plat_data->dev, "%s: reg=0x%08x, value=0x%x TAS25XX_SWITCH",
			__func__, reg, value);
		p_tas25xx->device_used = value;
		break;

	default:
		ret = -EINVAL;
		dev_err(plat_data->dev, "%s: reg=0x%08x, value=0x%x UNIMPLEMENTED",
			__func__, reg, value);
		break;
	}

	return ret;
}

#if IS_ENABLED(CODEC_PM)
static int tas25xx_codec_suspend(struct snd_soc_component *codec)
{
	struct tas25xx_priv *p_tas25xx = snd_soc_component_get_drvdata(codec);
	struct linux_platform *plat_data =
		(struct linux_platform *) p_tas25xx->platform_data;
	int ret = -1;

	dev_dbg(plat_data->dev, "%s\n", __func__);

	mutex_lock(&p_tas25xx->codec_lock);
	ret = plat_data->runtime_suspend(p_tas25xx);
	mutex_unlock(&p_tas25xx->codec_lock);

	return ret;
}

static int tas25xx_codec_resume(struct snd_soc_component *codec)
{
	struct tas25xx_priv *p_tas25xx = snd_soc_component_get_drvdata(codec);
	struct linux_platform *plat_data =
		(struct linux_platform *) p_tas25xx->platform_data;
	int ret = 0;

	mutex_lock(&p_tas25xx->codec_lock);

	dev_dbg(plat_data->dev, "%s\n", __func__);
	ret = plat_data->runtime_resume(p_tas25xx);

	mutex_unlock(&p_tas25xx->codec_lock);
	return ret;
}
#endif

static int tas25xx_dac_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = snd_soc_dapm_to_component(w->dapm);
	struct tas25xx_priv *p_tas25xx = snd_soc_component_get_drvdata(codec);
	struct linux_platform *plat_data =
		(struct linux_platform *) p_tas25xx->platform_data;
	int ret = -1;

	mutex_lock(&p_tas25xx->codec_lock);
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		p_tas25xx->dac_power = 1;
		dev_info(plat_data->dev, "SND_SOC_DAPM_POST_PMU\n");
		ret = tas25xx_set_power_state(p_tas25xx, TAS_POWER_ACTIVE, 0xffff);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		p_tas25xx->dac_power = 0;
		dev_info(plat_data->dev, "SND_SOC_DAPM_PRE_PMD\n");
		if (p_tas25xx->m_power_state != TAS_POWER_SHUTDOWN)
			ret = tas25xx_set_power_state(p_tas25xx, TAS_POWER_SHUTDOWN, 0xffff);
		else
			ret = 0;
		break;
	}
	mutex_unlock(&p_tas25xx->codec_lock);

	return ret;
}

static const struct snd_kcontrol_new dapm_switch =
	SOC_DAPM_SINGLE("Switch", TAS25XX_SWITCH, 0, 1, 0);

static const struct snd_soc_dapm_widget tas25xx_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN("ASI1", "ASI1 Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_SWITCH("TAS25XX ASI", SND_SOC_NOPM, 0, 0, &dapm_switch),
	SND_SOC_DAPM_AIF_OUT("Voltage Sense", "ASI1 Capture",  1,
		SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("Current Sense", "ASI1 Capture",  0,
		SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC_E("DAC", NULL, SND_SOC_NOPM, 0, 0, tas25xx_dac_event,
		SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_OUTPUT("OUT"),
	SND_SOC_DAPM_SIGGEN("VMON"),
	SND_SOC_DAPM_SIGGEN("IMON")
};

static const struct snd_soc_dapm_route tas25xx_audio_map[] = {
	{"DAC", NULL, "ASI1"},
	{"TAS25XX ASI", "Switch", "DAC"},
	{"OUT", NULL, "TAS25XX ASI"},
	{"Voltage Sense", NULL, "VMON"},
	{"Current Sense", NULL, "IMON"},
};

static bool fw_load_required(struct tas25xx_priv *p_tas25xx)
{
	return (atomic_read(&p_tas25xx->fw_state) == TAS25XX_DSP_FW_LOAD_FAIL);
}

static int tas25xx_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	int i, ret = 0;
	struct snd_soc_component *codec = dai->component;
	struct tas25xx_priv *p_tas25xx
		= snd_soc_component_get_drvdata(codec);
	struct linux_platform *plat_data =
		(struct linux_platform *) p_tas25xx->platform_data;
	int bitwidth = 16;
	unsigned int channels = params_channels(params);

	if (fw_load_required(p_tas25xx)) {
		dev_warn(plat_data->dev, "%s, firmware is not loaded, retry", __func__);
		ret = tas25xx_start_fw_load(p_tas25xx, 3);
		if (ret < 0) {
			dev_err(plat_data->dev, "%s fw load failed", __func__);
			return ret;
		}
	}

	mutex_lock(&p_tas25xx->codec_lock);

	dev_dbg(plat_data->dev, "%s, stream %s format: %d\n", __func__,
		(substream->stream ==
			SNDRV_PCM_STREAM_PLAYBACK) ? ("Playback") : ("Capture"),
		params_format(params));

	if (substream->stream != SNDRV_PCM_STREAM_PLAYBACK)
		p_tas25xx->tx_ch_count = channels;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		bitwidth = 16;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		bitwidth = 24;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
	default:
		bitwidth = 32;
		break;
	}

	ret = tas25xx_set_bitwidth(p_tas25xx,
			bitwidth, substream->stream);
	if (ret < 0) {
		dev_info(plat_data->dev, "set bitwidth failed, %d\n", ret);
		goto ret;
	}

	dev_info(plat_data->dev, "%s, stream %s sample rate: %d\n", __func__,
		(substream->stream ==
			SNDRV_PCM_STREAM_PLAYBACK) ? ("Playback") : ("Capture"),
		params_rate(params));

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		for (i = 0; i < p_tas25xx->ch_count; i++) {
			if (is_error_on_ch(p_tas25xx->amp_i2c_err, i))
				continue;
			ret = tas25xx_set_sample_rate(p_tas25xx, i, params_rate(params));
			if (ret)
				set_ch_ignore(p_tas25xx->amp_i2c_err, i);
		}
	}

	if (is_err_on_all_ch(p_tas25xx->amp_i2c_err, p_tas25xx->ch_count)) {
		tas25xx_log_i2cerr_stats(p_tas25xx);
		ret = -EIO;
	}

ret:
	mutex_unlock(&p_tas25xx->codec_lock);
	return ret;
}

static int tas25xx_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	int ret = 0;
	struct snd_soc_component *codec = dai->component;
	struct tas25xx_priv *p_tas25xx = snd_soc_component_get_drvdata(codec);
	struct linux_platform *plat_data =
		(struct linux_platform *) p_tas25xx->platform_data;

	if (fw_load_required(p_tas25xx)) {
		dev_warn(plat_data->dev, "%s, firmware is not loaded, retry", __func__);
		ret = tas25xx_start_fw_load(p_tas25xx, 3);
		if (ret < 0) {
			dev_err(plat_data->dev, "%s fw load failed", __func__);
			return ret;
		}
	}

	dev_info(plat_data->dev, "%s, format=0x%x\n", __func__, fmt);

	p_tas25xx->mn_fmt = fmt;
	ret = tas25xx_set_dai_fmt_for_fmt(p_tas25xx, fmt, 0xffff);

	if (is_err_on_all_ch(p_tas25xx->amp_i2c_err, p_tas25xx->ch_count)) {
		tas25xx_log_i2cerr_stats(p_tas25xx);
		ret = -EIO;
	}

	return ret;
}

static int tas25xx_set_dai_tdm_slot(struct snd_soc_dai *dai,
	unsigned int tx_mask, unsigned int rx_mask,
	int slots, int slot_width)
{
	int ret = 0;
	struct snd_soc_component *codec = dai->component;
	struct tas25xx_priv *p_tas25xx = snd_soc_component_get_drvdata(codec);
	struct linux_platform *plat_data =
		(struct linux_platform *) p_tas25xx->platform_data;

	if (fw_load_required(p_tas25xx)) {
		dev_warn(plat_data->dev, "%s, firmware is not loaded, retry", __func__);
		ret = tas25xx_start_fw_load(p_tas25xx, 3);
		if (ret < 0) {
			dev_err(plat_data->dev, "%s fw load failed", __func__);
			return ret;
		}
	}

	dev_dbg(plat_data->dev, "%s, tx_mask:%d, rx_mask:%d",
		__func__, tx_mask, rx_mask);
	dev_dbg(plat_data->dev, "%s, slots:%d,slot_width:%d",
		__func__, slots, slot_width);

	p_tas25xx->slot_width = slot_width;
	p_tas25xx->mn_slots = slots;

	return ret;
}

static int tas25xx_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
	int ret = 0;
	struct snd_soc_component *codec = dai->component;
	struct tas25xx_priv *p_tas25xx = snd_soc_component_get_drvdata(codec);
	struct linux_platform *plat_data =
		(struct linux_platform *) p_tas25xx->platform_data;

	if (fw_load_required(p_tas25xx)) {
		dev_warn(plat_data->dev, "%s, firmware is not loaded, retry", __func__);
		ret = tas25xx_start_fw_load(p_tas25xx, 3);
		if (ret < 0) {
			dev_err(plat_data->dev, "%s fw load failed", __func__);
			return ret;
		}
	}

	dev_dbg(plat_data->dev, "%s, stream %s mute %d\n", __func__,
		(stream == SNDRV_PCM_STREAM_PLAYBACK) ? ("Playback") : ("Capture"),
		mute);
#if defined(CONFIG_SEC_AUDIO_DEBUG)
	sec_audio_debug_rx_mute_control_info(stream, mute);
#endif

	if (stream != SNDRV_PCM_STREAM_PLAYBACK)
		return ret;

	/*
	mutex_lock(&p_tas25xx->codec_lock);
	if (mute)
		ret = tas25xx_set_power_state(p_tas25xx, TAS_POWER_MUTE,
			(0xf & tas25xx_get_drv_channel_opmode()));
	mutex_unlock(&p_tas25xx->codec_lock);
	*/

	if (mute) {
#if IS_ENABLED(CONFIG_TAS25XX_ALGO)
		tas25xx_stop_algo_processing();
#endif /* CONFIG_TAS25XX_ALGO */

#if IS_ENABLED(CONFIG_TAS25XX_IRQ_BD)
		tas25xx_log_interrupt_stats(p_tas25xx);
#endif /* CONFIG_TAS25XX_IRQ_BD */
		tas25xx_log_i2cerr_stats(p_tas25xx);
	} else {
#if IS_ENABLED(CONFIG_TAS25XX_ALGO)
		tas25xx_start_algo_processing(p_tas25xx->curr_mn_iv_width,
			p_tas25xx->curr_mn_vbat);
#endif /* CONFIG_TAS25XX_ALGO */
#if IS_ENABLED(CONFIG_TAS25XX_IRQ_BD)
		tas25xx_clear_interrupt_stats(p_tas25xx);
#endif /* CONFIG_TAS25XX_IRQ_BD */
	}

	return ret;
}

static struct snd_soc_dai_ops tas25xx_dai_ops = {
	.hw_params  = tas25xx_hw_params,
	.set_fmt    = tas25xx_set_dai_fmt,
	.set_tdm_slot = tas25xx_set_dai_tdm_slot,
	.mute_stream = tas25xx_mute_stream,
};

#define TAS25XX_FORMATS (SNDRV_PCM_FMTBIT_S16_LE |\
						SNDRV_PCM_FMTBIT_S20_3LE |\
						SNDRV_PCM_FMTBIT_S24_LE |\
						SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_driver tas25xx_dai_driver[] = {
	{
		.name = "tas25xx ASI1",
		.id = 0,
		.playback = {
			.stream_name    = "ASI1 Playback",
			.channels_min   = 1,
			.channels_max   = 8,
			.rates      = SNDRV_PCM_RATE_8000_192000,
			.formats    = TAS25XX_FORMATS,
		},
		.capture = {
			.stream_name    = "ASI1 Capture",
			.channels_min   = 1,
			.channels_max   = 8,
			.rates          = SNDRV_PCM_RATE_8000_192000,
			.formats    = TAS25XX_FORMATS,
		},
		.ops = &tas25xx_dai_ops,
		.symmetric_rate = 1,
	},
};

static irqreturn_t tas25xx_irq_handler(int irq, void *dev_id)
{
	struct tas25xx_priv *p_tas25xx = (struct tas25xx_priv *)dev_id;

	if (p_tas25xx != s_tas25xx)
		return IRQ_NONE;

	schedule_delayed_work(&p_tas25xx->irq_work,
		msecs_to_jiffies(p_tas25xx->intr_data[0].processing_delay));
	return IRQ_HANDLED;
}

static int tas25xx_setup_irq(struct tas25xx_priv *p_tas25xx)
{
	int i, ret = 0;
	struct linux_platform *plat_data =
		(struct linux_platform *) p_tas25xx->platform_data;

	if (!plat_data)
		return -EINVAL;

	/* register for interrupts */
	for (i = 0; i < p_tas25xx->ch_count; i++) {
		if (gpio_is_valid(p_tas25xx->devs[i]->irq_gpio)) {
			ret = gpio_request(p_tas25xx->devs[i]->irq_gpio,
					irq_gpio_label[i]);
			if (ret) {
				dev_err(plat_data->dev,
					"%s:%u: ch 0x%02x: GPIO %d request error\n",
					__func__, __LINE__,
					p_tas25xx->devs[i]->mn_addr,
					p_tas25xx->devs[i]->irq_gpio);
				continue;
			}
			gpio_direction_input(p_tas25xx->devs[i]->irq_gpio);

			p_tas25xx->devs[i]->irq_no =
				gpio_to_irq(p_tas25xx->devs[i]->irq_gpio);
			dev_info(plat_data->dev, "irq = %d\n",
				p_tas25xx->devs[i]->irq_no);

			if (p_tas25xx->nested_irq) {
				ret = devm_request_threaded_irq(plat_data->dev,
						p_tas25xx->devs[i]->irq_no, NULL, tas25xx_irq_handler,
						IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_SHARED,
						"tas25xx", p_tas25xx);
			} else {
				ret = devm_request_threaded_irq(plat_data->dev,
						p_tas25xx->devs[i]->irq_no, tas25xx_irq_handler, NULL,
						IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_SHARED,
						"tas25xx", p_tas25xx);
			}
			if (ret) {
				dev_err(plat_data->dev, "request_irq failed, error=%d\n", ret);
			} else {
				p_tas25xx->irq_enabled[i] = 1;
				dev_info(plat_data->dev, "Interrupt registration successful!!!");
			}
		}
	}

	return ret;
}

static int init_dev_with_fw_data(struct tas25xx_priv *p_tas25xx)
{
	int ret, i, ch_count;
	struct linux_platform *plat_data = NULL;

	plat_data = (struct linux_platform *) p_tas25xx->platform_data;
	ch_count = p_tas25xx->ch_count;

	/* software reset and initial writes */
	for (i = 0; i < ch_count; i++) {
		if (is_error_on_ch(p_tas25xx->amp_i2c_err, i))
			continue;

		ret = tas25xx_software_reset(p_tas25xx, i);
		set_ch_ignore_on_i2c_err(ret, p_tas25xx->amp_i2c_err, i);
		if (ret)
			dev_err(plat_data->dev,
				"ch=%d swreset failed, err=%d", i, ret);
	}

	if (is_err_on_all_ch(p_tas25xx->amp_i2c_err, ch_count)) {
		tas25xx_log_i2cerr_stats(p_tas25xx);
		ret = -EIO;
		goto post_fw_load_work_done;
	}

	ret = tas25xx_probe(p_tas25xx);
	if (ret) {
		dev_err(plat_data->dev, "Failed to initialize, error=%d", ret);
		goto post_fw_load_work_done;
	}

	ret = tas25xx_setup_irq(p_tas25xx);
	if (ret)
		dev_err(plat_data->dev, "failed to initialize irq=%d", ret);

post_fw_load_work_done:
	return ret;
}

static void fw_load_work_routine(struct work_struct *work)
{
	int ret, i, ch_count;
	struct linux_platform *plat_data = NULL;
	struct tas25xx_priv *p_tas25xx =
		container_of(work, struct tas25xx_priv, fw_load_work.work);

	plat_data = (struct linux_platform *) p_tas25xx->platform_data;
	ch_count = p_tas25xx->ch_count;

	ret = tas25xx_load_firmware(p_tas25xx, p_tas25xx->fw_load_retry_count);
	dev_info(plat_data->dev, "%s FW loading %s", __func__,
		!ret ? "success" : "fail");
	if (ret) {
		for (i = 0; i < ch_count; i++)
			p_tas25xx->ti_amp_state[i] = TAS_AMP_ERR_FW_LOAD;
	} else {
		ret = init_dev_with_fw_data(p_tas25xx);
		if (ret) {
			dev_err(plat_data->dev,
				"%s fw dnld to device error=%d", __func__, ret);
			for (i = 0; i < ch_count; i++)
				p_tas25xx->ti_amp_state[i] = TAS_AMP_ERR_I2C;
		} else {
			for (i = 0; i < ch_count; i++) {
				if (is_error_on_ch(p_tas25xx->amp_i2c_err, i))
					p_tas25xx->ti_amp_state[i] = TAS_AMP_ERR_I2C;
				else
					p_tas25xx->ti_amp_state[i] = TAS_AMP_STATE_FW_LOAD_SUCCESS;
			}
			ret = TAS25XX_FW_UPDATE_SUCCESS;
		}
	}

	atomic_set(&p_tas25xx->dev_init_status, ret);
	wake_up(&p_tas25xx->dev_init_wait);
}

static int tas25xx_start_fw_load(struct tas25xx_priv *p_tas25xx, int retry_count)
{
	int ret, ch_count;
	struct linux_platform *plat_data = NULL;

	atomic_set(&p_tas25xx->dev_init_status, 0);
	atomic_set(&p_tas25xx->fw_state, TAS25XX_DSP_FW_TRYLOAD);
	p_tas25xx->fw_load_retry_count = retry_count;
	plat_data = (struct linux_platform *) p_tas25xx->platform_data;
	ch_count = p_tas25xx->ch_count;

	tas25xx_check_last_i2c_error_n_reset();
	INIT_DELAYED_WORK(&p_tas25xx->fw_load_work, fw_load_work_routine);
	schedule_delayed_work(&p_tas25xx->fw_load_work, msecs_to_jiffies(0));

	wait_event_interruptible(p_tas25xx->dev_init_wait,
		atomic_read(&p_tas25xx->dev_init_status) != 0);

	/* set -ve errno or success 1*/
	ret = atomic_read(&p_tas25xx->dev_init_status);
	if (ret == TAS25XX_FW_UPDATE_SUCCESS)
		ret = 0;

	return ret;
}

static int tas25xx_codec_probe(struct snd_soc_component *codec)
{
	int ret = -1, i = 0;
	struct snd_soc_dapm_context *dapm = snd_soc_component_get_dapm(codec);
	struct tas25xx_priv *p_tas25xx = snd_soc_component_get_drvdata(codec);
	struct linux_platform *plat_data =
		(struct linux_platform *) p_tas25xx->platform_data;
	char *w_name[4] = {NULL};
	const char *prefix = codec->name_prefix;
	int w_count = 0;

	if (plat_data)
		plat_data->codec = codec;

	s_tas25xx = p_tas25xx;

	/*Moved from machine driver to codec*/
	if (prefix) {
		w_name[0] = kasprintf(GFP_KERNEL, "%s %s",
				prefix, "ASI1 Playback");
		w_name[1] = kasprintf(GFP_KERNEL, "%s %s",
				prefix, "ASI1 Capture");
		w_name[2] = kasprintf(GFP_KERNEL, "%s %s",
			prefix, "OUT");
		w_count = 3;
	} else {
		w_name[0] = kasprintf(GFP_KERNEL, "%s", "ASI1 Playback");
		w_name[1] = kasprintf(GFP_KERNEL, "%s", "ASI1 Capture");
		w_name[2] = kasprintf(GFP_KERNEL, "%s", "OUT");
		w_count = 3;
	}

	for (i = 0; i < w_count; i++) {
		snd_soc_dapm_ignore_suspend(dapm, w_name[i]);
		kfree(w_name[i]);
	}

	snd_soc_dapm_sync(dapm);

	ret = tas25xx_start_fw_load(p_tas25xx, 800);
	if (ret == -ENOENT)
		ret = 0;

	dev_info(plat_data->dev, "%s returning ret=%d\n",
		__func__, ret);

	return ret;
}

static void tas25xx_codec_remove(struct snd_soc_component *codec)
{
	struct tas25xx_priv *p_tas25xx = snd_soc_component_get_drvdata(codec);

	tas25xx_remove(p_tas25xx);
	s_tas25xx = NULL;
}

static struct snd_soc_component_driver soc_codec_driver_tas25xx = {
	.name			= "tas25xx",
	.probe			= tas25xx_codec_probe,
	.remove			= tas25xx_codec_remove,
	.read			= tas25xx_codec_read,
	.write			= tas25xx_codec_write,
#if IS_ENABLED(CODEC_PM)
	.suspend		= tas25xx_codec_suspend,
	.resume			= tas25xx_codec_resume,
#endif
	.dapm_widgets		= tas25xx_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(tas25xx_dapm_widgets),
	.dapm_routes		= tas25xx_audio_map,
	.num_dapm_routes	= ARRAY_SIZE(tas25xx_audio_map),
};

int tas25xx_register_codec(struct tas25xx_priv *p_tas25xx)
{
	int ret = -1;
	struct linux_platform *plat_data =
		(struct linux_platform *) p_tas25xx->platform_data;

	dev_info(plat_data->dev, "%s, enter\n", __func__);

	ret = devm_snd_soc_register_component(plat_data->dev,
		&soc_codec_driver_tas25xx,
		tas25xx_dai_driver, ARRAY_SIZE(tas25xx_dai_driver));

	return ret;
}

int tas25xx_deregister_codec(struct tas25xx_priv *p_tas25xx)
{
	struct linux_platform *plat_data =
		(struct linux_platform *) p_tas25xx->platform_data;

	snd_soc_unregister_component(plat_data->dev);

	return 0;
}

MODULE_AUTHOR("Texas Instruments Inc.");
MODULE_DESCRIPTION("TAS25XX ALSA SOC Smart Amplifier driver");
MODULE_LICENSE("GPL v2");

