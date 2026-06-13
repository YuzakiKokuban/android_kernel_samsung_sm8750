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
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef STREAMINCALL_H_
#define STREAMINCALL_H_

#include "Stream.h"

class ResourceManager;
class Device;
class Session;

class StreamInCall : public Stream
{
public:
   StreamInCall(const struct pal_stream_attributes *sattr, struct pal_device *dattr,
             const uint32_t no_of_devices,
             const struct modifier_kv *modifiers, const uint32_t no_of_modifiers,
             const std::shared_ptr<ResourceManager> rm); //make this just pass parameters to Stream and avoid duplicating code between StreamInCall and StreamCompress
   //StreamInCall();
   virtual ~StreamInCall();
   int32_t open() override;
   int32_t close() override;
   int32_t start() override;
   int32_t stop() override;
   int32_t prepare() override;
   int32_t setStreamAttributes( struct pal_stream_attributes *sattr) override;
   int32_t setVolume( struct pal_volume_data *volume) override;
   int32_t mute(bool state) override;
   int32_t mute_l(bool state) override;
   int32_t getDeviceMute(pal_stream_direction_t dir __unused, bool *state __unused) override {return 0;}
   int32_t setDeviceMute(pal_stream_direction_t dir __unused, bool state __unused) override {return 0;}
   int32_t pause() override;
   int32_t pause_l() override;
   int32_t resume() override;
   int32_t resume_l() override;
   int32_t flush();
   int32_t addRemoveEffect(pal_audio_effect_t effect, bool enable) override;
   int32_t read(struct pal_buffer *buf) override;
   int32_t write(struct pal_buffer *buf) override;
   int32_t registerCallBack(pal_stream_callback cb, uint64_t cookie) override;
   int32_t getCallBack(pal_stream_callback *cb) override;
   int32_t getParameters(uint32_t param_id, void **payload) override;
   int32_t setParameters(uint32_t param_id, void *payload) override;
   static int32_t isSampleRateSupported(uint32_t sampleRate);
   static int32_t isChannelSupported(uint32_t numChannels);
   static int32_t isBitWidthSupported(uint32_t bitWidth);
   int32_t setECRef(std::shared_ptr<Device> dev, bool is_enable) override;
   int32_t setECRef_l(std::shared_ptr<Device> dev, bool is_enable) override;
   int32_t ssrDownHandler() override;
   int32_t ssrUpHandler() override;
   int32_t reconfigureModule(uint32_t tagID, const char* BE, struct sessionToPayloadParam  *data);
};

#endif//StreamInCall_H_
