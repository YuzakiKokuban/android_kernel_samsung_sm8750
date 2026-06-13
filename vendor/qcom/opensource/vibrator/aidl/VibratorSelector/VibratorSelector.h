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

#ifndef VIBRATOR_SELECTOR_H
#define VIBRATOR_SELECTOR_H

#include <vector>
#include <expat.h>

typedef enum {
    TAG_HAPTICS_POLICY_XML_ROOT,
    TAG_HAPTICS_ON_API,
    TAG_HAPTICS_PERFORM_API,
    TAG_HAPTICS_COMPOSE_API,
    TAG_HAPTICS_COMPOSE_PWLE_API,
} haptics_policy_xml_tag;

typedef enum {
    VIB_TYPE_OL,
    VIB_TYPE_CL,
} vibrator_type;

typedef struct haptics_policy_xml_data {
    char data_buf[1024];
    size_t offs;
    haptics_policy_xml_tag hapticstag;
} haptics_policy_xml_data;

class VibratorSelector
{
public:
    VibratorSelector();
    ~VibratorSelector();
    static int XmlParser(std::string xmlFile);
    static void endTag(void *userdata, const XML_Char *tag_name);
    static void startTag(void *userdata, const XML_Char *tag_name, const XML_Char **attr);
    static void handleData(void *userdata, const char *s, int len);
    static void resetDataBuf(haptics_policy_xml_data *data);
    static void process_xml_info(haptics_policy_xml_data *data, const XML_Char *tag_name);
    vibrator_type getVibForOnApi(int32_t timeoutMs);
    vibrator_type getVibForComposeApi();
    vibrator_type getVibForPerformApi(int effect_id);
    vibrator_type getVibForPwleApi();
    static int init();
    static std::shared_ptr<VibratorSelector> GetInstance();
private:
    static uint32_t maxTimeoutOnConfig;
    static std::vector <int> effectIDPerformConfig;
    static vibrator_type vibForComposition;
    static vibrator_type vibForPwle;
    static std::shared_ptr <VibratorSelector> me_;
};
#endif