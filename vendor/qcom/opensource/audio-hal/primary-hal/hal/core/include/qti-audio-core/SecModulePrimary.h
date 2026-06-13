/*
 * Copyright (C) 2023 The Android Open Source Project
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

#pragma once

#include <string>
#include <system/audio.h>
#include <hardware/audio.h>

#include "AudioEffect.h"
#include "SecFTM.h"
#include <qti-audio-core/SecParameters.h>

using ::aidl::android::hardware::audio::core::VendorParameter;
using ::aidl::qti::audio::core::VString;

namespace qti::audio::core {

class SecModulePrimary {
    public:
        SecModulePrimary();

        static SecModulePrimary& getInstance();
        // For set parameters
        using SetHandler = std::function<void(
                SecModulePrimary*,
                const std::vector<::aidl::android::hardware::audio::core::VendorParameter>&)>;
        using SetParameterToFeatureMap = std::map<std::string, SecParameters::Feature_SEC>;
        using FeatureToSetHandlerMap = std::map<SecParameters::Feature_SEC, SetHandler>;

        static SetParameterToFeatureMap fillSetParameterToFeatureMap();
        static FeatureToSetHandlerMap fillFeatureToSetHandlerMap();
        using FeatureToVendorParametersMap =
                std::map<SecParameters::Feature_SEC, std::vector<::aidl::android::hardware::audio::core::VendorParameter>>;

        // For get parameters
        using GetHandler =
                std::function<std::vector<::aidl::android::hardware::audio::core::VendorParameter>(
                    SecModulePrimary*, const std::vector<std::string>&)>;
        using GetParameterToFeatureMap = std::map<std::string, SecParameters::Feature_SEC>;
        using FeatureToGetHandlerMap = std::map<SecParameters::Feature_SEC, GetHandler>;
        static GetParameterToFeatureMap fillGetParameterToFeatureMap();
        static FeatureToGetHandlerMap fillFeatureToGetHandlerMap();
        using FeatureToStringMap = std::map<SecParameters::Feature_SEC, std::vector<std::string>>;
#ifdef SEC_AUDIO_SPEAKER_CALIBRATION
        using CalValueToGetParameterMap = std::map<std::string, std::string>;
        static CalValueToGetParameterMap fillCalValueToGetParameterMap();
#endif
        // start of module parameters handling
        bool processSetVendorParameters(const std::vector<VendorParameter>& parameters);
#ifdef SEC_AUDIO_SPEAKER_CALIBRATION
        void processCalibrationParameters(const std::vector<VendorParameter>& parameters);
        std::string getCalibrationResults(std::string calValue, std::string ampType);
#endif
#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_PROVIDEO
        void SetProVideoState(int mode);
#endif
#ifdef SEC_AUDIO_SUPPORT_REMOTE_MIC
        pal_device_id_t getPrimaryOutPalDeviceId();
#endif
#ifdef SEC_AUDIO_COMMON
        static bool CheckComboDevice();
#endif
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
        static bool isListenbackDevice(pal_device_id_t deviceId);
        static bool isListenbackUsecase(Usecase usecase);
        static int getListenbackOutputCount();
        static void updateStreamListenbackMode(bool enable);
#endif
#ifdef SEC_AUDIO_USB_GAIN_CONTROL
        static void updateUsbAudioGain();
#endif
#ifdef SEC_AUDIO_KARAOKE
        void setKaraokeDevice();
#endif
        // setHandler for Samsung Generic
        void onSetSECGenericParameters(const std::vector<VendorParameter>& parameters);
        // SetHandler For Samsung Telephony
        void onSetSECTelephonyParameters(const std::vector<VendorParameter>& parameters);
        // SetHandler For Samsung Factory
        void onSetSECFTMParameters(const std::vector<VendorParameter>& parameters);
        // SetHandler For Samsung Parameters using Subkey
        void onSetSECSubkeyParameters(const std::vector<VendorParameter>& parameters);

        std::vector<VendorParameter> processGetVendorParameters(const std::vector<std::string>&);
        // GetHandler for Samsung Generic
        std::vector<VendorParameter> onGetSECGenericParameters(const std::vector<std::string>&);
        // GetHandler For Samsung Telephony
        std::vector<VendorParameter> onGetSECTelephonyParameters(const std::vector<std::string>&);
        // GetHandler For Samsung Factory
        std::vector<VendorParameter> onGetSECFTMParameters(const std::vector<std::string>&);

        static ndk::ScopedAStatus ForceSetOutDevices(
                const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices,
                bool force = false);
        static ndk::ScopedAStatus ForceSetInDevices(
                const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices);
        static std::shared_ptr<StreamOut> GetStreamOut(Usecase tag);
        static std::shared_ptr<StreamIn> GetStreamIn(Usecase tag);
#ifdef SEC_AUDIO_SAMSUNGRECORD
        static std::shared_ptr<StreamIn> getHighestPriorityStreamIn();
#endif
        void setTelephony(ChildInterface<Telephony> tel) { mTelephony = tel; }

        std::string toString() const {
            std::ostringstream os;
            os << std::endl << " --- SecModulePrimary ---" << std::endl;
            os << mSecFTM.toString();
            os << " --- SecModulePrimary end ---" << std::endl << std::endl;
            return os.str();
        }

    protected:
        ChildInterface<Telephony> mTelephony;

        const SetParameterToFeatureMap mSetParameterToFeatureMap_SEC{fillSetParameterToFeatureMap()};
        const FeatureToSetHandlerMap mFeatureToSetHandlerMap{fillFeatureToSetHandlerMap()};
        const GetParameterToFeatureMap mGetParameterToFeatureMap{fillGetParameterToFeatureMap()};
        const FeatureToGetHandlerMap mFeatureToGetHandlerMap{fillFeatureToGetHandlerMap()};
#ifdef SEC_AUDIO_SPEAKER_CALIBRATION
        const CalValueToGetParameterMap mCalValueToGetParameterMap{fillCalValueToGetParameterMap()};
#endif

#ifdef SEC_AUDIO_CALL_VOIP
        ndk::ScopedAStatus RerouteForVoip();
#endif
#ifdef SEC_AUDIO_SCREEN_MIRRORING // { SUPPORT_VOIP_VIA_SMART_VIEW
        ndk::ScopedAStatus UpdateSmartViewState(bool newVoipViaSmartView);
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW
#if defined(SEC_AUDIO_SUPPORT_FLIP_CALL) || defined(SEC_AUDIO_SUPPORT_SOUNDBOOSTER_FOLD_PARAM_ON_DSP)
        void SetFolderState(int state);
#endif
#ifdef SEC_AUDIO_BLE_OFFLOAD
        void UpdateSCOdeviceState();
#endif

        Platform& mPlatform{Platform::getInstance()};
        AudioExtension& mAudExt{AudioExtension::getInstance()};
        AudioEffect& mAudioEffect{AudioEffect::getInstance()};
        SecFTM& mSecFTM{SecFTM::getInstance()};

    };
} // namespace qti::audio::core::SecModulePrimary
