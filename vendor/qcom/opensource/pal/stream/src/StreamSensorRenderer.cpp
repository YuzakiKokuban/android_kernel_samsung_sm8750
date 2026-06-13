/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: StreamSensorRenderer"

#include "StreamSensorRenderer.h"
#include "Session.h"
#include "SessionAlsaPcm.h"
#include "ResourceManager.h"
#include "Device.h"
#include "us_detect_api.h"
#include <unistd.h>

StreamSensorRenderer::StreamSensorRenderer(const struct pal_stream_attributes *sattr __unused, struct pal_device *dattr __unused,
                    const uint32_t no_of_devices __unused, const struct modifier_kv *modifiers __unused,
                    const uint32_t no_of_modifiers __unused, const std::shared_ptr<ResourceManager> rm):
                  StreamCommon(sattr,dattr,no_of_devices,modifiers,no_of_modifiers,rm)
{
    rm->registerStream(this);
}

StreamSensorRenderer::~StreamSensorRenderer()
{
    rm->resetStreamInstanceID(this);
    rm->deregisterStream(this);
}

