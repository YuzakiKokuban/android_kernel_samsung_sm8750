/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <any>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <vector>

#include <aidl/android/hardware/audio/effect/BnFactory.h>
#include <android-base/thread_annotations.h>
#include "EffectConfig.h"

using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::BnFactory;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::Processing;
namespace aidl::qti::effects {

class Factory : public BnFactory {
  public:
    explicit Factory(const std::string& file);
    /**
     * @brief Get identity of all effects supported by the device, with the optional filter by type
     * and/or by instance UUID.
     *
     * @param in_type Type UUID.
     * @param in_instance Instance UUID.
     * @param in_proxy Proxy UUID.
     * @param out_descriptor List of Descriptors.
     * @return ndk::ScopedAStatus
     */
    ndk::ScopedAStatus queryEffects(
            const std::optional<::aidl::android::media::audio::common::AudioUuid>& in_type,
            const std::optional<::aidl::android::media::audio::common::AudioUuid>& in_instance,
            const std::optional<::aidl::android::media::audio::common::AudioUuid>& in_proxy,
            std::vector<Descriptor>* out_descriptor) override;

    /**
     * @brief Query list of defined processing, with the optional filter by AudioStreamType
     *
     * @param in_type Type of processing, could be AudioStreamType or AudioSource. Optional.
     * @param _aidl_return List of processing filtered by in_type.
     * @return ndk::ScopedAStatus
     */
    ndk::ScopedAStatus queryProcessing(const std::optional<Processing::Type>& in_type,
                                       std::vector<Processing>* _aidl_return) override;

    /**
     * @brief Create an effect instance for a certain implementation (identified by UUID).
     *
     * @param in_impl_uuid Effect implementation UUID.
     * @param _aidl_return A pointer to created effect instance.
     * @return ndk::ScopedAStatus
     */
    ndk::ScopedAStatus createEffect(
            const ::aidl::android::media::audio::common::AudioUuid& in_impl_uuid,
            std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>* _aidl_return)
            override;

    /**
     * @brief Destroy an effect instance.
     *
     * @param in_handle Effect instance handle.
     * @return ndk::ScopedAStatus
     */
    ndk::ScopedAStatus destroyEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_handle)
            override;

  private:
    const EffectConfig mConfig;
    ~Factory();

    std::mutex mMutex;
    // Set of effect descriptors supported by the devices.
    std::set<Descriptor> mDescSet GUARDED_BY(mMutex);
    std::set<Descriptor::Identity> mIdentitySet GUARDED_BY(mMutex);

    static constexpr int kMapEntryHandleIndex = 0;
    static constexpr int kMapEntryInterfaceIndex = 1;
    static constexpr int kMapEntryLibNameIndex = 2;
    typedef std::tuple<std::unique_ptr<void, std::function<void(void*)>> /* dlHandle */,
                       std::unique_ptr<struct effect_dl_interface_s> /* interfaces */,
                       std::string /* library name */>
            DlEntry;

    std::map<aidl::android::media::audio::common::AudioUuid /* implUUID */, DlEntry> mEffectLibMap
            GUARDED_BY(mMutex);

    typedef std::pair<aidl::android::media::audio::common::AudioUuid, ndk::SpAIBinder> EffectEntry;
    std::map<std::weak_ptr<IEffect>, EffectEntry, std::owner_less<>> mEffectMap GUARDED_BY(mMutex);

    ndk::ScopedAStatus destroyEffectImpl_l(const std::shared_ptr<IEffect>& in_handle)
            REQUIRES(mMutex);
    void cleanupEffectMap_l() REQUIRES(mMutex);
    bool openEffectLibrary(const ::aidl::android::media::audio::common::AudioUuid& impl,
                           const std::string& path);
    void createIdentityWithConfig(
            const EffectConfig::Library& configLib,
            const ::aidl::android::media::audio::common::AudioUuid& typeUuidStr,
            const std::optional<::aidl::android::media::audio::common::AudioUuid> proxyUuid);

    ndk::ScopedAStatus getDescriptorWithUuid_l(
            const aidl::android::media::audio::common::AudioUuid& uuid, Descriptor* desc)
            REQUIRES(mMutex);

    void loadEffectLibs();
    /* Get effect_dl_interface_s from library handle */
    void getDlSyms_l(DlEntry& entry) REQUIRES(mMutex);
};

} // namespace aidl::qti::effects
