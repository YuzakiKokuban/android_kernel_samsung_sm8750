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

#include "AudioHapticsInterface.h"

#define LOG_TAG "PAL: AudioHapticsInterface"
#define HAPTICS_XML_FILE "/vendor/etc/Hapticsconfig.xml"
#include "rx_haptics_api.h"
#include "wsa_haptics_vi_api.h"

std::shared_ptr<AudioHapticsInterface> AudioHapticsInterface::me_ = nullptr;
std::vector<haptics_wave_designer_config_t> AudioHapticsInterface::compose_haptics_info;
std::vector<haptics_wave_designer_config_t> AudioHapticsInterface::predefined_haptics_info;
std::vector<haptics_wave_designer_config_t> AudioHapticsInterface::oneshot_haptics_info;
int AudioHapticsInterface::ringtone_haptics_wave_design_mode;

AudioHapticsInterface::AudioHapticsInterface()
{

}

AudioHapticsInterface::~AudioHapticsInterface()
{

}

std::shared_ptr<AudioHapticsInterface> AudioHapticsInterface::GetInstance()
{
    if (!me_)
        me_ = std::shared_ptr<AudioHapticsInterface>(new AudioHapticsInterface);
    return me_;
}

int AudioHapticsInterface::init()
{
    int ret = 0;
    int bytes_read;
    predefined_haptics_info.clear();
    oneshot_haptics_info.clear();

    ret = AudioHapticsInterface::XmlParser(HAPTICS_XML_FILE);
    if (ret) {
        PAL_ERR(LOG_TAG, "error in haptics xml parsing ret %d", ret);
        throw std::runtime_error("error in haptics xml parsing");
    }
    return ret;
}

void AudioHapticsInterface::startTag(void *userdata, const XML_Char *tag_name,
    const XML_Char **attr)
{
    struct haptics_xml_data *data = ( struct haptics_xml_data *)userdata;
    resetDataBuf(data);
    if (!strcmp(tag_name, "haptics_param_values")) {
        data->hapticstag = TAG_HAPTICSCNFGXML_ROOT;
    } else if (!strcmp(tag_name, "predefined_effect")) {
        data->hapticstag = TAG_PREDEFINED_EFFECT;
    } else if (!strcmp(tag_name, "oneshot_effect")) {
        data->hapticstag = TAG_ONESHOT_EFFECT;
    } else if (!strcmp(tag_name, "ringtone_effect")) {
        data->hapticstag = TAG_RINGTONE_EFFECT;
    } else if (!strcmp(tag_name, "compose_effect")) {
        data->hapticstag = TAG_COMPOSE_EFFECT;
    } else {
        PAL_INFO(LOG_TAG, "No matching Tag found");
    }
}

void AudioHapticsInterface::endTag(void *userdata, const XML_Char *tag_name)
{
    struct haptics_xml_data *data = ( struct haptics_xml_data *)userdata;
    int size = -1;

    process_haptics_info(data, tag_name);
    resetDataBuf(data);
    return;
}

void AudioHapticsInterface::resetDataBuf(struct haptics_xml_data *data)
{
     data->offs = 0;
     data->data_buf[data->offs] = '\0';
}

void AudioHapticsInterface::handleData(void *userdata, const char *s, int len)
{
   struct haptics_xml_data *data = (struct haptics_xml_data *)userdata;
   if (len + data->offs >= sizeof(data->data_buf) ) {
       data->offs += len;
       /* string length overflow, return */
       return;
   } else {
        memcpy(data->data_buf + data->offs, s, len);
         data->offs += len;
   }
}

int AudioHapticsInterface::XmlParser(std::string xmlFile) {
    XML_Parser parser;
    FILE *file = NULL;
    int ret = 0;
    int bytes_read;
    void *buf = NULL;
    struct haptics_xml_data data;
    memset(&data, 0, sizeof(data));

    PAL_INFO(LOG_TAG, "XML parsing started %s", xmlFile.c_str());
    file = fopen(xmlFile.c_str(), "r");
    if (!file) {
        PAL_ERR(LOG_TAG, "Failed to open xml");
        ret = -EINVAL;
        goto done;
    }

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        PAL_ERR(LOG_TAG, "Failed to create XML");
        goto closeFile;
    }
    XML_SetUserData(parser,&data);
    XML_SetElementHandler(parser, startTag, endTag);
    XML_SetCharacterDataHandler(parser, handleData);

while (1) {
        buf = XML_GetBuffer(parser, 1024);
        if (buf == NULL) {
            PAL_ERR(LOG_TAG, "XML_Getbuffer failed");
            ret = -EINVAL;
            goto freeParser;
        }

        bytes_read = fread(buf, 1, 1024, file);
        if (bytes_read < 0) {
            PAL_ERR(LOG_TAG, "fread failed");
            ret = -EINVAL;
            goto freeParser;
        }

        if (XML_ParseBuffer(parser, bytes_read, bytes_read == 0) == XML_STATUS_ERROR) {
            PAL_ERR(LOG_TAG, "XML ParseBuffer failed ");
            ret = -EINVAL;
            goto freeParser;
        }
        if (bytes_read == 0)
            break;
    }

freeParser:
    XML_ParserFree(parser);
closeFile:
    fclose(file);
done:
    return ret;
}

void AudioHapticsInterface::process_haptics_info(struct haptics_xml_data *data,
                                           const XML_Char *tag_name)
{
    int size = 0;
    struct haptics_wave_designer_config_t HapticsCnfg = {};

    if (data->hapticstag == TAG_PREDEFINED_EFFECT) {
        if (!strcmp(tag_name, "num_channels")) {
            HapticsCnfg.num_channels = atoi(data->data_buf);
            predefined_haptics_info.push_back(HapticsCnfg);
        } else if (!strcmp(tag_name, "channel_mask")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].channel_mask =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "wave_design_mode")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].wave_design_mode =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "auto_overdrive_brake_en")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].auto_overdrive_brake_en =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "f0_tracking_en")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].f0_tracking_en =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "f0_tracking_param_reset_flag")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].f0_tracking_param_reset_flag =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "override_flag")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].override_flag =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "tracked_freq_warmup_time_ms")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].tracked_freq_warmup_time_ms =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "settling_time_ms")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].settling_time_ms =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "delay_time_ms")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].delay_time_ms =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "wavegen_fstart_hz_q20")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].wavegen_fstart_hz_q20 =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "repetition_count")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].repetition_count =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "repetition_period_ms")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].repetition_period_ms =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "pilot_tone_en")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].pilot_tone_en =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "low_pulse_intensity")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].low_pulse_intensity =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "mid_pulse_intensity")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].mid_pulse_intensity =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "high_pulse_intensity")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].high_pulse_intensity =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "pulse_width_ms")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].pulse_width_ms =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "pulse_sharpness")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].pulse_sharpness =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "num_pwl")) {
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].num_pwl = atoi(data->data_buf);
        } else if (!strcmp(tag_name, "pwl_time")) {
            int j = 0;
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].pwl_time = (int32_t *) calloc(predefined_haptics_info[size].num_pwl,
                                                            sizeof(int32_t));
            for (int i=0; i<predefined_haptics_info[size].num_pwl ;i++) {
                 predefined_haptics_info[size].pwl_time[i] = atoi(&data->data_buf[j]);
                 PAL_DBG(LOG_TAG, "pwltime :%d", predefined_haptics_info[size].pwl_time[i]);
                 for (; j<strlen(data->data_buf) ;j++) {
                    if (data->data_buf[j] == ',') {
                        j = j+1;
                        break;
                    }
                 }
            }
        } else if (!strcmp(tag_name, "pwl_acc")) {
            int j = 0;
            size = predefined_haptics_info.size() - 1;
            predefined_haptics_info[size].pwl_acc = (int32_t *) calloc(predefined_haptics_info[size].num_pwl,
                                                            sizeof(int32_t));
            for (int i=0; i<predefined_haptics_info[size].num_pwl ;i++) {
                 predefined_haptics_info[size].pwl_acc[i] = atoi(&data->data_buf[j]);
                 PAL_DBG(LOG_TAG, "pwlacc :%d", predefined_haptics_info[size].pwl_acc[i]);
                 for (; j<strlen(data->data_buf) ;j++) {
                    if (data->data_buf[j] == ',') {
                        j = j+1;
                        break;
                    }
                }
            }
        }
    }

    if (data->hapticstag == TAG_COMPOSE_EFFECT) {
        if (!strcmp(tag_name, "num_channels")) {
            HapticsCnfg.num_channels = atoi(data->data_buf);
            compose_haptics_info.push_back(HapticsCnfg);
        }
        else if (!strcmp(tag_name, "channel_mask")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].channel_mask = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "wave_design_mode")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].wave_design_mode = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "auto_overdrive_brake_en")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].auto_overdrive_brake_en = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "f0_tracking_en")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].f0_tracking_en = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "f0_tracking_param_reset_flag")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].f0_tracking_param_reset_flag = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "override_flag")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].override_flag = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "tracked_freq_warmup_time_ms")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].tracked_freq_warmup_time_ms = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "settling_time_ms")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].settling_time_ms = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "delay_time_ms")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].delay_time_ms = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "wavegen_fstart_hz_q20")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].wavegen_fstart_hz_q20 = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "repetition_count")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].repetition_count = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "repetition_period_ms")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].repetition_period_ms = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "pilot_tone_en")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].pilot_tone_en = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "low_pulse_intensity")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].low_pulse_intensity = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "mid_pulse_intensity")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].mid_pulse_intensity = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "high_pulse_intensity")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].high_pulse_intensity = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "pulse_width_ms")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].pulse_width_ms = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "pulse_sharpness")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].pulse_sharpness = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "num_pwl")) {
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].num_pwl = atoi(data->data_buf);
        }
        else if (!strcmp(tag_name, "pwl_time")) {
            int j = 0;
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].pwl_time = (int32_t*)calloc(compose_haptics_info[size].num_pwl,
                sizeof(int32_t));
            for (int i = 0; i < compose_haptics_info[size].num_pwl; i++) {
                compose_haptics_info[size].pwl_time[i] = atoi(&data->data_buf[j]);
                PAL_DBG(LOG_TAG, "pwltime :%d", compose_haptics_info[size].pwl_time[i]);
                for (; j < strlen(data->data_buf); j++) {
                    if (data->data_buf[j] == ',') {
                        j = j + 1;
                        break;
                    }
                }
            }
        }
        else if (!strcmp(tag_name, "pwl_acc")) {
            int j = 0;
            size = compose_haptics_info.size() - 1;
            compose_haptics_info[size].pwl_acc = (int32_t*)calloc(compose_haptics_info[size].num_pwl,
                sizeof(int32_t));
            for (int i = 0; i < compose_haptics_info[size].num_pwl; i++) {
                compose_haptics_info[size].pwl_acc[i] = atoi(&data->data_buf[j]);
                PAL_DBG(LOG_TAG, "pwlacc :%d", compose_haptics_info[size].pwl_acc[i]);
                for (; j < strlen(data->data_buf); j++) {
                    if (data->data_buf[j] == ',') {
                        j = j + 1;
                        break;
                    }
                }
            }
        }
    }

    if (data->hapticstag == TAG_ONESHOT_EFFECT) {
        if (!strcmp(tag_name, "num_channels")) {
            HapticsCnfg.num_channels = atoi(data->data_buf);
            oneshot_haptics_info.push_back(HapticsCnfg);
        } else if (!strcmp(tag_name, "channel_mask")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].channel_mask =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "wave_design_mode")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].wave_design_mode =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "auto_overdrive_brake_en")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].auto_overdrive_brake_en =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "f0_tracking_en")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].f0_tracking_en =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "f0_tracking_param_reset_flag")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].f0_tracking_param_reset_flag =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "override_flag")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].override_flag =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "wavegen_fstart_hz_q20")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].wavegen_fstart_hz_q20 =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "tracked_freq_warmup_time_ms")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].tracked_freq_warmup_time_ms =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "settling_time_ms")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].settling_time_ms =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "delay_time_ms")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].delay_time_ms =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "repetition_count")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].repetition_count =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "repetition_period_ms")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].repetition_period_ms =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "pilot_tone_en")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].pilot_tone_en =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "low_pulse_intensity")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].low_pulse_intensity =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "pulse_width_ms")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].pulse_width_ms =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "pulse_sharpness")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].pulse_sharpness =  atoi(data->data_buf);
        } else if (!strcmp(tag_name, "num_pwl")) {
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].num_pwl = atoi(data->data_buf);
        } else if (!strcmp(tag_name, "pwl_time")) {
            int j = 0;
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].pwl_time = (int32_t *) calloc(oneshot_haptics_info[size].num_pwl,
                                                            sizeof(int32_t));
            for (int i=0; i < oneshot_haptics_info[size].num_pwl ;i++) {
                 oneshot_haptics_info[size].pwl_time[i] = atoi(&data->data_buf[j]);
                 PAL_DBG(LOG_TAG, "pwltime :%d", oneshot_haptics_info[size].pwl_time[i]);
                 for (; j<strlen(data->data_buf) ;j++) {
                    if (data->data_buf[j] == ',') {
                        j = j+1;
                        break;
                    }
                 }
            }
        } else if (!strcmp(tag_name, "pwl_acc")) {
            int j = 0;
            size = oneshot_haptics_info.size() - 1;
            oneshot_haptics_info[size].pwl_acc = (int32_t *) calloc(oneshot_haptics_info[size].num_pwl,
                                                            sizeof(int32_t));
            for (int i=0; i < oneshot_haptics_info[size].num_pwl ;i++) {
                 oneshot_haptics_info[size].pwl_acc[i] = atoi(&data->data_buf[j]);
                 PAL_DBG(LOG_TAG, "pwlacc :%d", oneshot_haptics_info[size].pwl_acc[i]);
                 for (; j<strlen(data->data_buf) ;j++) {
                    if (data->data_buf[j] == ',') {
                        j = j+1;
                        break;
                    }
                }
            }
        }
    }

    if (data->hapticstag == TAG_RINGTONE_EFFECT) {
        if (!strcmp(tag_name, "wave_design_mode")) {
            ringtone_haptics_wave_design_mode =  atoi(data->data_buf);
        }
    }
    PAL_ERR(LOG_TAG, "%s \n", data->data_buf);
}

void AudioHapticsInterface::getTouchHapticsEffectConfiguration(int effect_id, bool isCompose, haptics_wave_designer_config_t **HConfig)
{
    if (effect_id >= 0) {
        if (*HConfig == NULL) {
            if (isCompose) {
                *HConfig = (haptics_wave_designer_config_t*)calloc(1, sizeof(compose_haptics_info[effect_id]));
                if (*HConfig)
                    memcpy(*HConfig, &compose_haptics_info[effect_id],
                        sizeof(compose_haptics_info[effect_id]));
            } else {
                *HConfig = (haptics_wave_designer_config_t*)calloc(1, sizeof(predefined_haptics_info[effect_id]));
                if (*HConfig)
                    memcpy(*HConfig, &predefined_haptics_info[effect_id],
                        sizeof(predefined_haptics_info[effect_id]));
            }
        }
    } else {
        if (*HConfig == NULL) {
            *HConfig = (haptics_wave_designer_config_t *) calloc(1, sizeof(oneshot_haptics_info[0]));
            if (*HConfig)
                memcpy(*HConfig, &oneshot_haptics_info[0],
                                             sizeof(oneshot_haptics_info[0]));
        }
    }
    PAL_DBG(LOG_TAG, "getTouchHapticsEffectConfiguration exit\n");
}
