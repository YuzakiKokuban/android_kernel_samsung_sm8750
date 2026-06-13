/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AHAL_HalOffloadEffects_QTI"

#include <android-base/logging.h>
#include <dlfcn.h>
#include <qti-audio-core/HalOffloadEffects.h>

namespace qti::audio::core {

HalOffloadEffects::HalOffloadEffects() {
    loadLibrary(kOffloadPostProcBundlePath);
    loadLibrary(kOffloadVisualizerPath);
// { SEC_AUDIO_SUPPORT_AIDL_EFFECT
    loadLibrary(kSecOffloadEffectLibraryPath);
// } SEC_AUDIO_SUPPORT_AIDL_EFFECT
}

void HalOffloadEffects::loadLibrary(std::string path) {
    // dlopen library and dlsym fptr.
    std::function<void(void *)> dlClose = [](void *handle) -> void {
        if (handle && dlclose(handle)) {
            LOG(ERROR) << "dlclose failed " << dlerror();
        }
    };

    auto libHandle =
            std::unique_ptr<void, decltype(dlClose)>{dlopen(path.c_str(), RTLD_LAZY), dlClose};
    if (!libHandle) {
        LOG(ERROR) << __func__ << ": dlopen failed for " << path << " " << dlerror();
        return;
    }

    // std::unique_ptr<struct OffloadEffectLibIntf> effectIntf;
    auto effectIntf = new OffloadEffectLibIntf{nullptr, nullptr};
    effectIntf->mStartEffect = (StartEffectFptr)dlsym(libHandle.get(), "startEffect");
    if (!effectIntf->mStartEffect) {
        LOG(ERROR) << "startEffect is missing in " << path << dlerror();
        return;
    }
    effectIntf->mStopEffect = (StopEffectFptr)dlsym(libHandle.get(), "stopEffect");
    if (!effectIntf->mStopEffect) {
        LOG(ERROR) << "stopEffect is missing in " << path << dlerror();
        return;
    }
// { SEC_AUDIO_VOLUME_MONITOR
    effectIntf->mUpdateEffect = (UpdateEffectFptr)dlsym(libHandle.get(), "updateEffect");
    if (!effectIntf->mUpdateEffect) {
        LOG(ERROR) << "updateEffect is missing in " << path << dlerror();
        return;
    }
// } SEC_AUDIO_VOLUME_MONITOR
    LOG(DEBUG) << "found post proc library" << path;
    mEffects.emplace_back(std::make_pair(std::move(libHandle),
                                         std::unique_ptr<struct OffloadEffectLibIntf>(effectIntf)));
}

void HalOffloadEffects::startEffect(int ioHandle, pal_stream_handle_t *palHandle) {
    for (const auto &effect : mEffects) {
        effect.second->mStartEffect(ioHandle, palHandle);
    }
}

void HalOffloadEffects::stopEffect(int ioHandle) {
    for (const auto &effect : mEffects) {
        effect.second->mStopEffect(ioHandle);
    }
}
// { SEC_AUDIO_VOLUME_MONITOR
void HalOffloadEffects::updateEffect(pal_stream_handle_t * palHandle, int updateType) {
    for (const auto &effect : mEffects) {
        effect.second->mUpdateEffect(palHandle, updateType);
    }
}
// } SEC_AUDIO_VOLUME_MONITOR
} // namespace qti::audio::core