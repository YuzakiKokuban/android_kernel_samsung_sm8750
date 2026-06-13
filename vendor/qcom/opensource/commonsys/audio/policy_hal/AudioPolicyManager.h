/*
 * Copyright (c) 2013-2020 The Linux Foundation. All rights reserved.
 * Not a contribution.
 *
 * Copyright (C) 2009 The Android Open Source Project
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


#include <audiopolicy/managerdefault/AudioPolicyManager.h>
#include <Volume.h>
#include "APMConfigHelper.h"


namespace android {
#ifndef AUDIO_EXTN_FORMATS_ENABLED
#define AUDIO_FORMAT_WMA 0x12000000UL
#define AUDIO_FORMAT_WMA_PRO 0x13000000UL
#define AUDIO_FORMAT_FLAC 0x1B000000UL
#define AUDIO_FORMAT_ALAC 0x1C000000UL
#define AUDIO_FORMAT_APE 0x1D000000UL
#endif

#define WMA_STD_NUM_FREQ     7
#define WMA_STD_NUM_CHANNELS 2
static uint32_t wmaStdSampleRateTbl[WMA_STD_NUM_FREQ] =
{
    8000, 11025, 16000, 22050, 32000, 44100, 48000
};

static uint32_t wmaStdMinAvgByteRateTbl[WMA_STD_NUM_FREQ][WMA_STD_NUM_CHANNELS] =
{
    {128, 12000},
    {8016, 8016},
    {10000, 16000},
    {16016, 20008},
    {20000, 24000},
    {20008, 31960},
    {63000, 63000}
};

static uint32_t wmaStdMaxAvgByteRateTbl[WMA_STD_NUM_FREQ][WMA_STD_NUM_CHANNELS] =
{
    {8000, 12000},
    {10168, 10168},
    {16000, 20000},
    {20008, 32048},
    {20000, 48000},
    {48024, 320032},
    {256008, 256008}
};

#define MAX_BITRATE_WMA_PRO      1536000
#define MAX_BITRATE_WMA_LOSSLESS 1152000

#ifndef AAC_ADTS_OFFLOAD_ENABLED
#define AUDIO_FORMAT_AAC_ADTS 0x1E000000UL
#endif

#ifndef AUDIO_EXTN_AFE_PROXY_ENABLED
#define AUDIO_DEVICE_OUT_PROXY 0x1000000
#endif

// ----------------------------------------------------------------------------

class AudioPolicyManagerCustom: public AudioPolicyManager
{

public:
        AudioPolicyManagerCustom(AudioPolicyClientInterface *clientInterface);

        virtual ~AudioPolicyManagerCustom() {}

        status_t setDeviceConnectionStateInt(audio_devices_t device,
                                          audio_policy_dev_state_t state,
                                          const char *device_address,
                                          const char *device_name,
                                          audio_format_t encodedFormat);
        virtual void setPhoneState(audio_mode_t state);
        virtual void setForceUse(audio_policy_force_use_t usage,
                                 audio_policy_forced_cfg_t config);

        virtual bool isOffloadSupported(const audio_offload_info_t& offloadInfo);

        virtual status_t getInputForAttr(const audio_attributes_t *attr,
                                         audio_io_handle_t *input,
                                         audio_unique_id_t riid,
                                         audio_session_t session,
                                         uid_t uid,
                                         const audio_config_base_t *config,
                                         audio_input_flags_t flags,
                                         audio_port_handle_t *selectedDeviceId,
                                         input_type_t *inputType,
                                         audio_port_handle_t *portId);
        /* count active capture sessions (that are not sound trigger) using one of
           the specified devices. Ignore devices if AUDIO_DEVICE_IN_DEFAULT is passed */
        uint32_t activeNonSoundTriggerInputsCountOnDevices(
            audio_devices_t devices = AUDIO_DEVICE_IN_DEFAULT) const;
        // indicates to the audio policy manager that the input starts being used.
        virtual status_t startInput(audio_port_handle_t portId);
        // indicates to the audio policy manager that the input stops being used.
        virtual status_t stopInput(audio_port_handle_t portId);

        status_t dump(int fd) override;
        static sp<APMConfigHelper> mApmConfigs;

protected:
        // check that volume change is permitted, compute and send new volume to audio hardware
        virtual status_t checkAndSetVolume(IVolumeCurves &curves,
                                           VolumeSource volumeSource, int index,
                                           const sp<AudioOutputDescriptor>& outputDesc,
                                           DeviceTypeSet deviceTypes,
                                           int delayMs = 0, bool force = false);

        // avoid invalidation for active music stream on  previous outputs
        // which is supported on the new device.

        bool isInvalidationOfMusicStreamNeeded(const audio_attributes_t &attr);

        // Must be called before updateDevicesAndOutputs()
        void checkOutputForAttributes(const audio_attributes_t &attr);

        // if argument "device" is different from AUDIO_DEVICE_NONE,  startSource() will force
        // the re-evaluation of the output device.
        status_t startSource(const sp<SwAudioOutputDescriptor>& outputDesc,
                             const sp<TrackClientDescriptor>& client,
                             uint32_t *delayMs);
        status_t stopSource(const sp<SwAudioOutputDescriptor>& outputDesc,
                            const sp<TrackClientDescriptor>& client);
        // event is one of STARTING_OUTPUT, STARTING_BEACON, STOPPING_OUTPUT, STOPPING_BEACON
        // returns 0 if no mute/unmute event happened, the largest latency of the device where
        //   the mute/unmute happened

        uint32_t handleEventForBeacon(int){return 0;}
        uint32_t setBeaconMute(bool){return 0;}
        static audio_output_flags_t getFallBackPath();
        int mFallBackflag;
        //parameter indicates of HDMI speakers disabled
        bool mHdmiAudioDisabled;
        //parameter indicates if HDMI plug in/out detected
        bool mHdmiAudioEvent;

private:
        // internal method to return the output handle for the given device and format
        audio_io_handle_t getOutputForDevices(
                const DeviceVector &devices,
                audio_session_t session,
                audio_stream_type_t stream,
                const audio_config_t *config,
                audio_output_flags_t *flags,
                bool forceMutingHaptic = false);

        // internal method to fill offload info in case of Direct PCM
        status_t getOutputForAttr(const audio_attributes_t *attr,
                audio_io_handle_t *output,
                audio_session_t session,
                audio_stream_type_t *stream,
                uid_t uid,
                const audio_config_t *config,
                audio_output_flags_t *flags,
                audio_port_handle_t *selectedDeviceId,
                audio_port_handle_t *portId,
                std::vector<audio_io_handle_t> *secondaryOutputs,
                output_type_t *outputType);




        // internal method to query hal for whether display-port is connected
        // and can be used for voip/voice call
        void chkDpConnAndAllowedForVoice();
        // Used for voip + voice concurrency usecase
        int mPrevPhoneState;
        int mvoice_call_state;
        // Used for record + playback concurrency
        bool mIsInputRequestOnProgress;
};
};
