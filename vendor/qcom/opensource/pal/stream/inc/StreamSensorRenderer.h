/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef STREAMSENSORRENDERER_H_
#define STREAMSENSORRENDERER_H_

#include "StreamCommon.h"
#include "ResourceManager.h"
#include "Device.h"
#include "Session.h"

class StreamSensorRenderer : public StreamCommon
{
public:
    StreamSensorRenderer(const struct pal_stream_attributes *sattr, struct pal_device *dattr,
                     const uint32_t no_of_devices, const struct modifier_kv *modifiers,
                     const uint32_t no_of_modifiers, const std::shared_ptr<ResourceManager> rm);
    ~StreamSensorRenderer();
};

#endif//STREAMSENSORRENDERER_H_
