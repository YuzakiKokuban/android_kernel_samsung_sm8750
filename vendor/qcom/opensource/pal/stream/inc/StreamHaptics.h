/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef STREAMHAPTICS_H_
#define STREAMHAPTICS_H_

#include "StreamPCM.h"
#include "ResourceManager.h"
#include "Device.h"
#include "Session.h"

class StreamHaptics : public StreamPCM
{
public:
    StreamHaptics(const struct pal_stream_attributes *sattr, struct pal_device *dattr,
                     const uint32_t no_of_devices, const struct modifier_kv *modifiers,
                     const uint32_t no_of_modifiers, const std::shared_ptr<ResourceManager> rm);
    ~StreamHaptics();
    uint64_t cookie_;
    pal_stream_callback callback_= 0;
    int32_t setParameters(uint32_t param_id, void *payload);
    int32_t start();
    int32_t ssrDownHandler() override;
    int32_t ssrUpHandler() override;
    int32_t registerCallBack(pal_stream_callback cb, uint64_t cookie) override;
private:
    static void HandleCallBack(uint64_t hdl, uint32_t event_id,
                               void *data, uint32_t event_size);
    void HandleEvent(uint32_t event_id, void *data, uint32_t event_size);
    int32_t HandleHapticsConcurrency(struct pal_stream_attributes *sattr);
};

#endif//STREAMHAPTICS_H_
