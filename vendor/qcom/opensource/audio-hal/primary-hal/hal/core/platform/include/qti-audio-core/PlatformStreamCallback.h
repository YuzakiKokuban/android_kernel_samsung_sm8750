/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

namespace qti::audio::core {

struct PlatformStreamCallback {
    virtual ~PlatformStreamCallback() = default;
    virtual void onTransferReady() = 0;
    virtual void onDrainReady() = 0;
    virtual void onError() = 0;
};

}  // namespace qti::audio::core