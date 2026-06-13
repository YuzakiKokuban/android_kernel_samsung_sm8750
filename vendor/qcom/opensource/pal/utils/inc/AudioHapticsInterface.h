/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIO_HAPTICS_INTERFACE_H
#define AUDIO_HAPTICS_INTERFACE_H

#include <errno.h>
#include <stdint.h>
#include <map>
#include <expat.h>
#include <vector>
#include <memory>
#include <string>
#include "PalDefs.h"
#include "PalCommon.h"
#include "kvh2xml.h"

typedef enum {
    TAG_HAPTICSCNFGXML_ROOT,
    TAG_PREDEFINED_EFFECT,
    TAG_HAPTICS_EFFECT,
    TAG_ONESHOT_EFFECT,
    TAG_RINGTONE_EFFECT,
    TAG_COMPOSE_EFFECT,
} haptics_xml_tag;

struct haptics_wave_designer_config_t {
    // Waveform designer mode parameters
    int8_t num_channels;
    int8_t channel_mask;
    int8_t wave_design_mode;
    int32_t auto_overdrive_brake_en;
    int32_t f0_tracking_en;
    int32_t f0_tracking_param_reset_flag;
    uint32_t override_flag;
    int32_t tracked_freq_warmup_time_ms;
    int32_t settling_time_ms;
    int32_t delay_time_ms;
    int32_t wavegen_fstart_hz_q20;
    int32_t repetition_count;
    int32_t repetition_period_ms;
    uint32_t pilot_tone_en;
    int32_t low_pulse_intensity;
    int32_t mid_pulse_intensity;
    int32_t high_pulse_intensity;
    int32_t pulse_width_ms;
    int32_t pulse_sharpness;
    int32_t num_pwl;
    int32_t *pwl_time;
    int32_t *pwl_acc;
};

struct haptics_xml_data{
    char data_buf[1024];
    size_t offs;
    haptics_xml_tag hapticstag;
};

class AudioHapticsInterface
{
public:
    AudioHapticsInterface();
    ~AudioHapticsInterface();
    static int XmlParser(std::string xmlFile);
    static void endTag(void *userdata, const XML_Char *tag_name);
    static void startTag(void *userdata, const XML_Char *tag_name, const XML_Char **attr);
    static void handleData(void *userdata, const char *s, int len);
    static void resetDataBuf(struct haptics_xml_data *data);
    static void process_haptics_info(struct haptics_xml_data *data, const XML_Char *tag_name);
    void getTouchHapticsEffectConfiguration(int effect_id, bool isCompose, haptics_wave_designer_config_t **HConfig);
    int getRingtoneHapticsEffectConfiguration() {return ringtone_haptics_wave_design_mode;}
    static int init();
    static std::shared_ptr<AudioHapticsInterface> GetInstance();
private:
    static std::vector<haptics_wave_designer_config_t> predefined_haptics_info;
    static std::vector<haptics_wave_designer_config_t> compose_haptics_info;
    static std::vector<haptics_wave_designer_config_t> oneshot_haptics_info;
    static std::shared_ptr<AudioHapticsInterface> me_;
    static int ringtone_haptics_wave_design_mode;
};
#endif
