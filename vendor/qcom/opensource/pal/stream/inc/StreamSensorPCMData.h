/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
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

#ifndef StreamSensorPCMData_H_
#define StreamSensorPCMData_H_

#include "Device.h"
#include "Session.h"
#include "StreamCommon.h"
#include "ACDPlatformInfo.h"

class StreamSensorPCMData : public StreamCommon
{
public:
    StreamSensorPCMData(const struct pal_stream_attributes *sattr __unused,
                        struct pal_device *dattr __unused,
                        const uint32_t no_of_devices __unused,
                        const struct modifier_kv *modifiers __unused,
                        const uint32_t no_of_modifiers __unused,
                        const std::shared_ptr<ResourceManager> rm);
    ~StreamSensorPCMData();
    std::shared_ptr<CaptureProfile> GetCurrentCaptureProfile();
    int32_t addRemoveEffect(pal_audio_effect_t effect, bool enable);
    int32_t open() override;
    int32_t start() override;
    int32_t stop() override;
    int32_t close() override;
    int32_t Resume(bool is_internal __unused) override;
    int32_t Pause(bool is_internal __unused) override;
    int32_t HandleConcurrentStream(bool active) override;
    int32_t DisconnectDevice(pal_device_id_t device_id) override;
    int32_t ConnectDevice(pal_device_id_t device_id) override;
    pal_device_id_t GetAvailCaptureDevice();
    struct st_uuid GetVendorUuid();

private:
    void GetUUID(class SoundTriggerUUID *uuid, const struct st_uuid *vendor_uuid);
    int32_t SetupStreamConfig(const struct st_uuid *vendor_uuid);
    int32_t DisconnectDevice_l(pal_device_id_t device_id);
    int32_t ConnectDevice_l(pal_device_id_t device_id);
    int32_t setECRef(std::shared_ptr<Device> dev, bool is_enable);
    int32_t setECRef_l(std::shared_ptr<Device> dev, bool is_enable);
    int32_t setParameters(uint32_t param_id, void *payload);
    std::shared_ptr<ACDStreamConfig> sm_cfg_;
    std::shared_ptr<ACDPlatformInfo> acd_info_;
    std::shared_ptr<CaptureProfile> cap_prof_;
    uint32_t pcm_data_stream_effect;
    uint32_t pcm_data_buffering;
    bool paused_;
};

#endif//StreamSensorPCMData_H_
