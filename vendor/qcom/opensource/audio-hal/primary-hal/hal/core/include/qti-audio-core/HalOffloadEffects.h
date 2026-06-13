/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace qti::audio::core {

#ifdef __LP64__
#define OFFLOAD_EFFECTS_BUNDLE_LIBRARY_PATH "/vendor/lib64/soundfx/libqcompostprocbundle.so"
#define VISUALIZER_LIBRARY_PATH "/vendor/lib64/soundfx/libqcomvisualizer.so"
#else
#define OFFLOAD_EFFECTS_BUNDLE_LIBRARY_PATH "/vendor/lib/soundfx/libqcompostprocbundle.so"
#define VISUALIZER_LIBRARY_PATH "/vendor/lib/soundfx/libqcomvisualizer.so"
#endif

// { SEC_AUDIO_SUPPORT_AIDL_EFFECT
#ifdef __LP64__
#define OFFLOAD_EFFECT_LIBRARY_PATH "/vendor/lib64/soundfx/libaudioeffectoffload_aidl.so"
#else
#define OFFLOAD_EFFECT_LIBRARY_PATH "/vendor/lib/soundfx/libaudioeffectoffload_aidl.so"
#endif
// } SEC_AUDIO_SUPPORT_AIDL_EFFECT

/*
* All libraries implementing post processing effect should expose
* startEffect(int, pal_stream_handle_t*)
* stopEffect(int)
*/
using pal_stream_handle_t = uint64_t;

using StartEffectFptr = void (*)(int, uint64_t*);
using StopEffectFptr = void (*)(int);
// { SEC_AUDIO_VOLUME_MONITOR
using UpdateEffectFptr = void (*)(uint64_t*, int);
// } SEC_AUDIO_VOLUME_MONITOR
struct OffloadEffectLibIntf {
    StartEffectFptr mStartEffect;
    StopEffectFptr mStopEffect;
// { SEC_AUDIO_VOLUME_MONITOR
    UpdateEffectFptr mUpdateEffect;
// } SEC_AUDIO_VOLUME_MONITOR
};

/* Pair of libhandle, and function pointers*/
using EffectLibInfo = std::pair<std::unique_ptr<void, std::function<void(void*)>>,
                                std::unique_ptr<struct OffloadEffectLibIntf>>;

class HalOffloadEffects {
  private:
#ifdef __LP64__
    const std::string kOffloadVisualizerPath = "/vendor/lib64/soundfx/libqcomvisualizer.so";
    const std::string kOffloadPostProcBundlePath = "/vendor/lib64/soundfx/libqcompostprocbundle.so";
#else
    const std::string kOffloadVisualizerPath = "/vendor/lib/soundfx/libqcomvisualizer.so";
    const std::string kOffloadPostProcBundlePath = "/vendor/lib/soundfx/libqcompostprocbundle.so";
#endif

// { SEC_AUDIO_SUPPORT_AIDL_EFFECT
#ifdef __LP64__
    const std::string kSecOffloadEffectLibraryPath = "/vendor/lib64/soundfx/libaudioeffectoffload_aidl.so";
#else
    const std::string kSecOffloadEffectLibraryPath = "/vendor/lib/soundfx/libaudioeffectoffload_aidl.so";
#endif
// } SEC_AUDIO_SUPPORT_AIDL_EFFECT

    std::vector<EffectLibInfo> mEffects;

    HalOffloadEffects();
    void loadLibrary(std::string path);

  public:
    static HalOffloadEffects& getInstance() {
        static HalOffloadEffects halEffects;
        return halEffects;
    }

    void startEffect(int ioHandle, pal_stream_handle_t* palHandle);
    void stopEffect(int ioHandle);
// { SEC_AUDIO_VOLUME_MONITOR
    void updateEffect(pal_stream_handle_t* palHandle, int updateType);
// } SEC_AUDIO_VOLUME_MONITOR
};

} // qti::audio::core