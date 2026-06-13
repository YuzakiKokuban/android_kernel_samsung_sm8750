/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.

 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <log/log.h>
#include <string>
#include "VibratorSelector.h"

#define HAPTICS_XML_FILE "/vendor/etc/HapticsPolicy.xml"

std::shared_ptr<VibratorSelector> VibratorSelector::me_ = nullptr;
uint32_t VibratorSelector::maxTimeoutOnConfig;
std::vector <int> VibratorSelector::effectIDPerformConfig;
vibrator_type VibratorSelector::vibForComposition;
vibrator_type VibratorSelector::vibForPwle;

VibratorSelector::VibratorSelector()
{

}

VibratorSelector::~VibratorSelector()
{

}

std::shared_ptr<VibratorSelector> VibratorSelector::GetInstance()
{
    if (!me_)
        me_ = std::shared_ptr<VibratorSelector>(new VibratorSelector);
    return me_;
}

int VibratorSelector::init()
{
    int ret;
    int bytes_read;

    ret = VibratorSelector::XmlParser(HAPTICS_XML_FILE);
    if (ret) {
        ALOGE("Error in haptics xml parsing ret %d", ret);
    }
    return ret;
}

void VibratorSelector::resetDataBuf(haptics_policy_xml_data* data)
{
    data->offs = 0;
    data->data_buf[data->offs] = '\0';
}

void VibratorSelector::startTag(void *userdata, const XML_Char *tag_name,
    const XML_Char **attr)
{
    haptics_policy_xml_data *data = (haptics_policy_xml_data *)userdata;
    resetDataBuf(data);
    if (!strcmp(tag_name, "hapticsPolicyConfiguration")) {
        data->hapticstag = TAG_HAPTICS_POLICY_XML_ROOT;
    } else if (!strcmp(tag_name, "hapticsONAPI")) {
        data->hapticstag = TAG_HAPTICS_ON_API;
    } else if (!strcmp(tag_name, "hapticsPerformAPI")) {
        data->hapticstag = TAG_HAPTICS_PERFORM_API;
    } else if (!strcmp(tag_name, "hapticsComposeAPI")) {
        data->hapticstag = TAG_HAPTICS_COMPOSE_API;
    } else if (!strcmp(tag_name, "hapticsComposePwleAPI")) {
        data->hapticstag = TAG_HAPTICS_COMPOSE_PWLE_API;
    } else {
        ALOGE("No matching Tag found");
    }
}

void VibratorSelector::endTag(void *userdata, const XML_Char *tag_name)
{
    haptics_policy_xml_data *data = (haptics_policy_xml_data *)userdata;
    int size = -1;

    process_xml_info(data, tag_name);
    resetDataBuf(data);
    return;
}

void VibratorSelector::handleData(void *userdata, const char *s, int len)
{
   haptics_policy_xml_data *data = (haptics_policy_xml_data *)userdata;
   if (len + data->offs >= sizeof(data->data_buf) ) {
       data->offs += len;
       /* string length overflow, return */
       return;
   } else {
        memcpy(data->data_buf + data->offs, s, len);
        data->offs += len;
   }
}

int VibratorSelector::XmlParser(std::string xmlFile) {
    XML_Parser parser;
    FILE *file = NULL;
    int ret = 0;
    int bytes_read;
    void *buf = NULL;
    haptics_policy_xml_data data;

    memset(&data, 0, sizeof(data));

    ALOGI("XML parsing started %s", xmlFile.c_str());
    file = fopen(xmlFile.c_str(), "r");
    if (!file) {
        ALOGE("Failed to open xml");
        ret = -EINVAL;
        goto done;
    }

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        ALOGE("Failed to create XML");
        goto closeFile;
    }
    XML_SetUserData(parser,&data);
    XML_SetElementHandler(parser, startTag, endTag);
    XML_SetCharacterDataHandler(parser, handleData);

    while (1) {
        buf = XML_GetBuffer(parser, 1024);
        if (buf == NULL) {
            ALOGE("XML_Getbuffer failed");
            ret = -EINVAL;
            goto freeParser;
        }

        bytes_read = fread(buf, 1, 1024, file);
        if (bytes_read < 0) {
            ALOGE("fread failed");
            ret = -EINVAL;
            goto freeParser;
        }

        if (XML_ParseBuffer(parser, bytes_read, bytes_read == 0) == XML_STATUS_ERROR) {
            ALOGE("XML ParseBuffer failed ");
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

void VibratorSelector::process_xml_info(haptics_policy_xml_data *data,
                                           const XML_Char *tag_name)
{
    int size = 0;

    if (data->hapticstag == TAG_HAPTICS_ON_API) {
        if (!strcmp(tag_name, "maxMs")) {
            maxTimeoutOnConfig = atoi(data->data_buf);
            ALOGI("maxMs: %d", maxTimeoutOnConfig);
        }
    }

    if (data->hapticstag == TAG_HAPTICS_PERFORM_API) {
       if (!strcmp(tag_name, "effect_id")) {
            int j = 0;
            int data_len = strlen(data->data_buf);
            while (j < data_len) {
                effectIDPerformConfig.push_back(atoi(&data->data_buf[j]));
                for (; j < data_len; j++) {
                    if (data->data_buf[j] == ',') {
                        j = j + 1;
                        break;
                    }
                }
            }

            for (int i = 0; i < effectIDPerformConfig.size(); i++) {
                ALOGI("Effect ID: %d", effectIDPerformConfig[i]);
            }
       }
    }

    if (data->hapticstag == TAG_HAPTICS_COMPOSE_API) {
        if (!strcmp(tag_name, "SupportCompose")) {
		ALOGE("support value for Composition: %s", data->data_buf);
            if (strstr(data->data_buf, "False")) {
                vibForComposition = VIB_TYPE_CL;
            } else {
                vibForComposition = VIB_TYPE_OL;
            }
            ALOGI("support value for Composition: %d", vibForComposition);
        }
    }

    if (data->hapticstag == TAG_HAPTICS_COMPOSE_PWLE_API) {
        if (!strcmp(tag_name, "SupportComposePWLE")) {
            if (strstr(data->data_buf, "False")) {
                vibForPwle = VIB_TYPE_CL;
            } else {
                vibForPwle = VIB_TYPE_OL;
            }
            ALOGI("support value for Compose PWL: %d", vibForPwle);
        }
    }
}

vibrator_type VibratorSelector::getVibForOnApi(int32_t timeout)
{
    vibrator_type vibType = VIB_TYPE_OL;

    vibType = timeout <= maxTimeoutOnConfig ? VIB_TYPE_OL : VIB_TYPE_CL;

    return vibType;
}

vibrator_type VibratorSelector::getVibForPerformApi(int effect_id)
{
    vibrator_type vibType = VIB_TYPE_OL;

    if (std::find(effectIDPerformConfig.begin(), effectIDPerformConfig.end(),
        effect_id) == effectIDPerformConfig.end()) {
            vibType = VIB_TYPE_CL;
    }

    return vibType;
}

vibrator_type VibratorSelector::getVibForComposeApi()
{
    return vibForComposition;
}

vibrator_type VibratorSelector::getVibForPwleApi()
{
    return vibForPwle;
}