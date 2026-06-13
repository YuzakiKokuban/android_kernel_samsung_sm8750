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
 */

#ifndef STREAMCONTEXTPROXY_H_
#define STREAMCONTEXTPROXY_H_

#include "StreamCommon.h"

class ResourceManager;
class Device;
class Session;

class StreamContextProxy : public StreamCommon
{
public:
   StreamContextProxy(const struct pal_stream_attributes *sattr, struct pal_device *dattr __unused,
             const uint32_t no_of_devices __unused,
             const struct modifier_kv *modifiers __unused, const uint32_t no_of_modifiers __unused,
             const std::shared_ptr<ResourceManager> rm); //make this just pass parameters to Stream and avoid duplicating code between StreamPCM and StreamCompress
   //StreamContextProxy();
   virtual ~StreamContextProxy();
   int32_t setParameters(uint32_t param_id, void *payload);
   int32_t setVolume( struct pal_volume_data *volume __unused) {return 0;}
private:
   void ParseASPSEventPayload(uint32_t event_id,
                              uint32_t event_size, void *data);
   static void HandleCallBack(uint64_t hdl, uint32_t event_id, void *data,
                              uint32_t event_size);
};

#endif//STREAMCONTEXTPROXY_H_
