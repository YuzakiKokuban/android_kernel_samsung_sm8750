// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024, The Linux Foundation. All rights reserved.
 */

#include <sound/soc.h>
#include <sound/samsung/sec_audio_sysfs.h>
#include <sound/samsung/snd_debug_proc.h>
#if IS_ENABLED(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif
#if defined(CONFIG_SND_SOC_TAS25XX)
#include "codecs/tas25xx/inc/tas25xx-ext.h"
#endif
#include "machine_defect_detector.h"

static int defer_count;

/*
 * change codec component to dummy in dailinks
 * when soundcard does not registered several times.
*/
static void change_snd_card_dailinks(struct snd_soc_card *card)
{
	static const struct snd_soc_dai_link_component dummy_compononet = {
		.name = "snd-soc-dummy",
		.dai_name = "snd-soc-dummy-dai",
	};
	struct snd_soc_dai *dai = NULL;
	int i, j = 0;

	sdp_boot_print("%s\n", __func__);

	for (i = 0; i < card->num_links; i++) {
		for (j = 0; j < card->dai_link[i].num_codecs; j++) {
			dai = snd_soc_find_dai(&card->dai_link[i].codecs[j]);
			if(!dai) {
				sdp_boot_print("%s: CANNOT find dai %s\n",
					__func__, card->dai_link[i].codecs[j].name);
				if(card->dai_link[i].codecs[j].name == NULL)
					card->num_of_dapm_routes=0;
				card->dai_link[i].codecs[j].of_node = NULL;
				card->dai_link[i].codecs[j].name =
							dummy_compononet.name;
				card->dai_link[i].codecs[j].dai_name =
							dummy_compononet.dai_name;
				card->dai_link[i].init = NULL;
				pr_info("%s: Change codec_dai of %s to DUMMY\n",
					__func__, card->dai_link[i].name);
			}
		}
	}
}

static bool is_defer_count_expired(int max_defer_count)
{
	if (++defer_count >= max_defer_count)
		return true;

	return false;
}

void check_snd_component(struct snd_soc_card *card, int max_defer_count)
{
	if (is_defer_count_expired(max_defer_count))
		change_snd_card_dailinks(card);
}

static int get_audio_amp_ready(enum amp_id id)
{
	pr_info("%s(%d)\n", __func__, id);
#if defined(CONFIG_SND_SOC_TAS25XX)
	if (tas25xx_get_state(id) > 0)
		return INIT_SUCCESS;
	else
		return INIT_FAIL;
#endif

	return NOT_SUPPORTED;
}

static void report_amp_i2c_fail(uint32_t i2caddr)
{
	pr_info("%s(%x)\n", __func__, i2caddr);
#if IS_ENABLED(CONFIG_SEC_ABC)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
	sec_abc_send_event("MODULE=audio@INFO=spk_amp");
#else
	sec_abc_send_event("MODULE=audio@WARN=spk_amp");
#endif
#endif
}

inline static void i2c_error_callback(void (*i2c_err_cb)(uint32_t))
{
	/* dummy callback function */
	return;
}

int register_amp_callback(void)
{
#if defined(CONFIG_SND_SOC_TAS25XX)
	tas25xx_register_i2c_error_callback(report_amp_i2c_fail);
#else
	i2c_error_callback(report_amp_i2c_fail);
#endif
	audio_register_ready_cb(get_audio_amp_ready);

	return 0;
}

