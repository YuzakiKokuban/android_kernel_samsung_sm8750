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
 * ​​​​​Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <set>

#include <aidl/android/hardware/audio/core/BnModule.h>
#include <extensions/AudioExtension.h>
#include <qti-audio-core/ChildInterface.h>
#include <qti-audio-core/ModuleConfig.h>
#include <qti-audio-core/Stream.h>
#include <qti-audio-core/Telephony.h>

namespace qti::audio::core {

class Module : public ::aidl::android::hardware::audio::core::BnModule,
               public std::enable_shared_from_this<Module> {
  public:
    enum Type : int { DEFAULT, R_SUBMIX, STUB, USB };

    explicit Module(Type type);

    // #################### start of overriding APIs from IModule ####################
    ndk::ScopedAStatus setModuleDebug(
            const ::aidl::android::hardware::audio::core::ModuleDebug& in_debug) override;
    ndk::ScopedAStatus getTelephony(
            std::shared_ptr<::aidl::android::hardware::audio::core::ITelephony>* _aidl_return)
            override;
    ndk::ScopedAStatus getBluetooth(
            std::shared_ptr<::aidl::android::hardware::audio::core::IBluetooth>* _aidl_return)
            override;
    ndk::ScopedAStatus getBluetoothA2dp(
            std::shared_ptr<::aidl::android::hardware::audio::core::IBluetoothA2dp>* _aidl_return)
            override;
    ndk::ScopedAStatus getBluetoothLe(
            std::shared_ptr<::aidl::android::hardware::audio::core::IBluetoothLe>* _aidl_return)
            override;
    ndk::ScopedAStatus prepareToDisconnectExternalDevice(int32_t in_portId) override;
    ndk::ScopedAStatus connectExternalDevice(
            const ::aidl::android::media::audio::common::AudioPort& in_templateIdAndAdditionalData,
            ::aidl::android::media::audio::common::AudioPort* _aidl_return) override;
    ndk::ScopedAStatus disconnectExternalDevice(int32_t in_portId) override;
    ndk::ScopedAStatus getAudioPatches(
            std::vector<::aidl::android::hardware::audio::core::AudioPatch>* _aidl_return) override;
    ndk::ScopedAStatus getAudioPort(
            int32_t in_portId,
            ::aidl::android::media::audio::common::AudioPort* _aidl_return) override;
    ndk::ScopedAStatus getAudioPortConfigs(
            std::vector<::aidl::android::media::audio::common::AudioPortConfig>* _aidl_return)
            override;
    ndk::ScopedAStatus getAudioPorts(
            std::vector<::aidl::android::media::audio::common::AudioPort>* _aidl_return) override;
    ndk::ScopedAStatus getAudioRoutes(
            std::vector<::aidl::android::hardware::audio::core::AudioRoute>* _aidl_return) override;
    ndk::ScopedAStatus getAudioRoutesForAudioPort(
            int32_t in_portId,
            std::vector<::aidl::android::hardware::audio::core::AudioRoute>* _aidl_return) override;
    ndk::ScopedAStatus openInputStream(
            const ::aidl::android::hardware::audio::core::IModule::OpenInputStreamArguments&
                    in_args,
            ::aidl::android::hardware::audio::core::IModule::OpenInputStreamReturn* _aidl_return)
            override;
    ndk::ScopedAStatus openOutputStream(
            const ::aidl::android::hardware::audio::core::IModule::OpenOutputStreamArguments&
                    in_args,
            ::aidl::android::hardware::audio::core::IModule::OpenOutputStreamReturn* _aidl_return)
            override;
    ndk::ScopedAStatus getSupportedPlaybackRateFactors(
            SupportedPlaybackRateFactors* _aidl_return) override;
    ndk::ScopedAStatus setAudioPatch(
            const ::aidl::android::hardware::audio::core::AudioPatch& in_requested,
            ::aidl::android::hardware::audio::core::AudioPatch* _aidl_return) override;
    ndk::ScopedAStatus setAudioPortConfig(
            const ::aidl::android::media::audio::common::AudioPortConfig& in_requested,
            ::aidl::android::media::audio::common::AudioPortConfig* out_suggested,
            bool* _aidl_return) override;
    ndk::ScopedAStatus resetAudioPatch(int32_t in_patchId) override;
    ndk::ScopedAStatus resetAudioPortConfig(int32_t in_portConfigId) override;
    ndk::ScopedAStatus getMasterMute(bool* _aidl_return) override;
    ndk::ScopedAStatus setMasterMute(bool in_mute) override;
    ndk::ScopedAStatus getMasterVolume(float* _aidl_return) override;
    ndk::ScopedAStatus setMasterVolume(float in_volume) override;
    ndk::ScopedAStatus getMicMute(bool* _aidl_return) override;
    ndk::ScopedAStatus setMicMute(bool in_mute) override;
    ndk::ScopedAStatus getMicrophones(
            std::vector<::aidl::android::media::audio::common::MicrophoneInfo>* _aidl_return)
            override;
    ndk::ScopedAStatus updateAudioMode(
            ::aidl::android::media::audio::common::AudioMode in_mode) override;
    ndk::ScopedAStatus updateScreenRotation(
            ::aidl::android::hardware::audio::core::IModule::ScreenRotation in_rotation) override;
    ndk::ScopedAStatus updateScreenState(bool in_isTurnedOn) override;
    ndk::ScopedAStatus getSoundDose(
            std::shared_ptr<::aidl::android::hardware::audio::core::sounddose::ISoundDose>*
                    _aidl_return) override;
    ndk::ScopedAStatus generateHwAvSyncId(int32_t* _aidl_return) override;
    ndk::ScopedAStatus getVendorParameters(
            const std::vector<std::string>& in_ids,
            std::vector<::aidl::android::hardware::audio::core::VendorParameter>* _aidl_return)
            override;
    ndk::ScopedAStatus setVendorParameters(
            const std::vector<::aidl::android::hardware::audio::core::VendorParameter>&
                    in_parameters,
            bool in_async) override;
    ndk::ScopedAStatus addDeviceEffect(
            int32_t in_portConfigId,
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override;
    ndk::ScopedAStatus removeDeviceEffect(
            int32_t in_portConfigId,
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override;
    ndk::ScopedAStatus getMmapPolicyInfos(
            ::aidl::android::media::audio::common::AudioMMapPolicyType mmapPolicyType,
            std::vector<::aidl::android::media::audio::common::AudioMMapPolicyInfo>* _aidl_return)
            override;
    ndk::ScopedAStatus supportsVariableLatency(bool* _aidl_return) override;
    ndk::ScopedAStatus getAAudioMixerBurstCount(int32_t* _aidl_return) override;
    ndk::ScopedAStatus getAAudioHardwareBurstMinUsec(int32_t* _aidl_return) override;
    // #################### end of overriding APIs from IModule ####################

    // This value is used for all AudioPatches.
    static constexpr int32_t kMinimumStreamBufferSizeFrames = 48;
    // The maximum stream buffer size is 1 GiB = 2 ** 30 bytes;
    static constexpr int32_t kMaximumStreamBufferSizeBytes = 1 << 30;

  protected:
    struct VendorDebug {
        static const std::string kForceTransientBurstName;
        static const std::string kForceSynchronousDrainName;
        bool forceTransientBurst = false;
        bool forceSynchronousDrain = false;
    };

    // ids of device ports created at runtime via 'connectExternalDevice'.
    // Also stores a list of ids of mix ports with dynamic profiles that were populated from
    // the connected port. This list can be empty, thus an int->int multimap can't be used.
    using ConnectedDevicePorts = std::map<int32_t, std::set<int32_t>>;
    // Maps port ids and port config ids to patch ids.
    // Multimap because both ports and configs can be used by multiple patches.
    using Patches = std::multimap<int32_t, int32_t>;

    const Type mType;
    std::unique_ptr<ModuleConfig> mConfig;
    ::aidl::android::hardware::audio::core::ModuleDebug mDebug;
    VendorDebug mVendorDebug;
    ConnectedDevicePorts mConnectedDevicePorts;
    Streams mStreams;
    Patches mPatches;
    bool mMasterMute = false;
    float mMasterVolume = 1.0f;
    ChildInterface<::aidl::android::hardware::audio::core::sounddose::ISoundDose> mSoundDose;
    std::optional<bool> mIsMmapSupported;

  protected:
    // #################### start of virtual APIs to be implemented by children ####################
    virtual ndk::ScopedAStatus createInputStream(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones,
            std::shared_ptr<StreamIn>* result) = 0;
    virtual ndk::ScopedAStatus createOutputStream(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
            const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                    offloadInfo,
            std::shared_ptr<StreamOut>* result) = 0;
    virtual std::vector<::aidl::android::media::audio::common::AudioProfile> getDynamicProfiles(
            const ::aidl::android::media::audio::common::AudioPort& audioPort);

    virtual void onNewPatchCreation(
            const std::vector<::aidl::android::media::audio::common::AudioPortConfig*>& sources,
            const std::vector<::aidl::android::media::audio::common::AudioPortConfig*>& sinks,
            ::aidl::android::hardware::audio::core::AudioPatch& newPatch);
    virtual void onPrepareToDisconnectExternalDevice(
            const ::aidl::android::media::audio::common::AudioPort& audioPort);

    virtual void setAudioPatchTelephony(
            const std::vector<::aidl::android::media::audio::common::AudioPortConfig*>& sources,
            const std::vector<::aidl::android::media::audio::common::AudioPortConfig*>& sinks,
            const ::aidl::android::hardware::audio::core::AudioPatch& patch);
    virtual void resetAudioPatchTelephony(
            const ::aidl::android::hardware::audio::core::AudioPatch&);
    virtual std::string toStringInternal() { return std::string("No-op implementation"); }
    /**
     * Call this API only for debugging purpose
    **/
    virtual void dumpInternal(const std::string& identifier = "no_id"){};

    // If the module is unable to populate the connected device port correctly,
    // the returned error code must correspond to the errors of
    // `IModule.connectedExternalDevice` method.
    virtual ndk::ScopedAStatus populateConnectedDevicePort(
            ::aidl::android::media::audio::common::AudioPort* connectedDevicePort,
            const int32_t templateDevicePortId);
    // If the module finds that the patch endpoints configurations are not
    // matched, the returned error code must correspond to the errors of
    // `IModule.setAudioPatch` method.
    virtual ndk::ScopedAStatus checkAudioPatchEndpointsMatch(
            const std::vector<::aidl::android::media::audio::common::AudioPortConfig*>& sources,
            const std::vector<::aidl::android::media::audio::common::AudioPortConfig*>& sinks);
    virtual int onExternalDeviceConnectionChanged(
            const ::aidl::android::media::audio::common::AudioPort& audioPort, bool connected);
    virtual ndk::ScopedAStatus onMasterMuteChanged(bool mute);
    virtual ndk::ScopedAStatus onMasterVolumeChanged(float volume);
    virtual std::unique_ptr<ModuleConfig> initializeConfig();
    /* fetch the nominal latency for the given mix port config */
    virtual int32_t getNominalLatencyMs(
            const ::aidl::android::media::audio::common::AudioPortConfig&);
    // #################### end of virtual APIs to be implemented by children ####################

    // Utility and helper functions accessible to subclasses.
    void cleanUpPatch(int32_t patchId);
    ndk::ScopedAStatus createStreamContext(
            int32_t in_portConfigId, int64_t in_bufferSizeFrames,
            std::shared_ptr<::aidl::android::hardware::audio::core::IStreamCallback> asyncCallback,
            std::shared_ptr<::aidl::android::hardware::audio::core::IStreamOutEventCallback>
                    outEventCallback,
            StreamContext* out_context);
    std::vector<::aidl::android::media::audio::common::AudioDevice> findConnectedDevices(
            int32_t portConfigId);
    std::set<int32_t> findConnectedPortConfigIds(int32_t portConfigId);
    ndk::ScopedAStatus findPortIdForNewStream(
            int32_t in_portConfigId, ::aidl::android::media::audio::common::AudioPort** port);
    ModuleConfig& getConfig();
    const ConnectedDevicePorts& getConnectedDevicePorts() const { return mConnectedDevicePorts; }
    bool getMasterMute() const { return mMasterMute; }
    bool getMasterVolume() const { return mMasterVolume; }
    const Patches& getPatches() const { return mPatches; }
    const Streams& getStreams() const { return mStreams; }
    Type getType() const { return mType; }
    bool isMmapSupported();
    void populateConnectedProfiles();
    template <typename C>
    std::set<int32_t> portIdsFromPortConfigIds(C portConfigIds);

    // helper functions to print human readable string for portconfig names and routes
    std::string portNameFromPortConfigIds(int portConfigId);
    std::string getPatchDetails(const ::aidl::android::hardware::audio::core::AudioPatch& patch);

    void registerPatch(const ::aidl::android::hardware::audio::core::AudioPatch& patch);
    ndk::ScopedAStatus updateStreamsConnectedState(
            const ::aidl::android::hardware::audio::core::AudioPatch& oldPatch,
            const ::aidl::android::hardware::audio::core::AudioPatch& newPatch);

    ChildInterface<Telephony> mTelephony;
};

std::ostream& operator<<(std::ostream& os, Module::Type t);

} // namespace qti::audio::core
