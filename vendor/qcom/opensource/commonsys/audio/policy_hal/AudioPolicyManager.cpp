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

#define LOG_TAG "AudioPolicyManagerCustom"
//#define LOG_NDEBUG 0

//#define VERY_VERBOSE_LOGGING
#ifdef VERY_VERBOSE_LOGGING
#define ALOGVV ALOGV
#else
#define ALOGVV(a...) do { } while(0)
#endif

// A device mask for all audio output devices that are considered "remote" when evaluating
// active output devices in isStreamActiveRemotely()
#define APM_AUDIO_OUT_DEVICE_REMOTE_ALL  AUDIO_DEVICE_OUT_REMOTE_SUBMIX
// A device mask for all audio input and output devices where matching inputs/outputs on device
// type alone is not enough: the address must match too
#define APM_AUDIO_DEVICE_MATCH_ADDRESS_ALL (AUDIO_DEVICE_IN_REMOTE_SUBMIX | \
                                            AUDIO_DEVICE_OUT_REMOTE_SUBMIX)
#define SAMPLE_RATE_8000 8000
#include <inttypes.h>
#include <math.h>

#include <cutils/properties.h>
#include <utils/Log.h>
#include <hardware/audio.h>
#include <hardware/audio_effect.h>
#include <media/AudioParameter.h>
#include "AudioPolicyManager.h"
#include <policy.h>

namespace android {
/*audio policy: workaround for truncated touch sounds*/
//FIXME: workaround for truncated touch sounds
// to be removed when the problem is handled by system UI
#define TOUCH_SOUND_FIXED_DELAY_MS 100

sp<APMConfigHelper> AudioPolicyManagerCustom::mApmConfigs = new APMConfigHelper();

audio_output_flags_t AudioPolicyManagerCustom::getFallBackPath()
{
    audio_output_flags_t flag = AUDIO_OUTPUT_FLAG_FAST;
    std::string fallback_path = mApmConfigs->getVoiceConcFallbackPath();

    if (strlen(fallback_path.c_str()) > 0) {
        if (!strncmp(fallback_path.c_str(), "deep-buffer", 11)) {
            flag = AUDIO_OUTPUT_FLAG_DEEP_BUFFER;
        }
        else if (!strncmp(fallback_path.c_str(), "fast", 4)) {
            flag = AUDIO_OUTPUT_FLAG_FAST;
        }
        else {
            ALOGD("voice_conc:not a recognised path(%s) in prop vendor.voice.conc.fallbackpath",
                 fallback_path.c_str());
        }
    }
    else {
        ALOGD("voice_conc:prop vendor.voice.conc.fallbackpath not set");
    }

    ALOGD("voice_conc:picked up flag(0x%x) from prop vendor.voice.conc.fallbackpath",
        flag);

    return flag;
}

template <typename T>
bool operator== (const SortedVector<T> &left, const SortedVector<T> &right)
{
    if (left.size() != right.size()) {
        return false;
    }
    for (size_t index = 0; index < right.size(); index++) {
        if (left[index] != right[index]) {
            return false;
        }
    }
    return true;
}

template <typename T>
bool operator!= (const SortedVector<T> &left, const SortedVector<T> &right)
{
    return !(left == right);
}

// ----------------------------------------------------------------------------
// AudioPolicyInterface implementation
// ----------------------------------------------------------------------------
extern "C" AudioPolicyInterface* createAudioPolicyManager(
         AudioPolicyClientInterface *clientInterface)
{
     AudioPolicyManagerCustom *apm = new AudioPolicyManagerCustom(clientInterface);
     status_t status = apm->initialize();
     if (status != NO_ERROR) {
         delete apm;
         apm = nullptr;
     }
     return apm;
}

extern "C" void destroyAudioPolicyManager(AudioPolicyInterface *interface)
{
     delete interface;
}

status_t AudioPolicyManagerCustom::setDeviceConnectionStateInt(audio_devices_t deviceType,
                                                         audio_policy_dev_state_t state,
                                                         const char *device_address,
                                                         const char *device_name,
                                                         audio_format_t encodedFormat)
{
    ALOGD("setDeviceConnectionStateInt() device: 0x%X, state %d, address %s name %s format 0x%X",
            deviceType, state, device_address, device_name, encodedFormat);

    // connect/disconnect only 1 device at a time
    if (!audio_is_output_device(deviceType) && !audio_is_input_device(deviceType)) return BAD_VALUE;

    sp<DeviceDescriptor> device =
            mHwModules.getDeviceDescriptor(deviceType, device_address, device_name, encodedFormat,
                                           state == AUDIO_POLICY_DEVICE_STATE_AVAILABLE);
    if (device == 0) {
        return INVALID_OPERATION;
    }

    // handle output devices
    if (audio_is_output_device(deviceType)) {
        SortedVector <audio_io_handle_t> outputs;

        ssize_t index = mAvailableOutputDevices.indexOf(device);

        // save a copy of the opened output descriptors before any output is opened or closed
        // by checkOutputsForDevice(). This will be needed by checkOutputForAllStrategies()
        mPreviousOutputs = mOutputs;
        switch (state)
        {
        // handle output device connection
        case AUDIO_POLICY_DEVICE_STATE_AVAILABLE: {
            if (index >= 0) {
                if (mApmConfigs->isHDMISpkEnabled() &&
                        (popcount(deviceType) == 1) && (deviceType & AUDIO_DEVICE_OUT_AUX_DIGITAL)) {
                   if (!strncmp(device_address, "hdmi_spkr", 9)) {
                        mHdmiAudioDisabled = false;
                    } else {
                        mHdmiAudioEvent = true;
                    }
                }
                ALOGW("setDeviceConnectionState() device already connected: %x", deviceType);
                return INVALID_OPERATION;
            }
            ALOGV("%s() connecting device %s format %x",
                    __func__, device->toString().c_str(), encodedFormat);

            // register new device as available
            if (mAvailableOutputDevices.add(device) < 0) {
                return NO_MEMORY;
            }
            if (mApmConfigs->isHDMISpkEnabled() &&
                    (popcount(deviceType) == 1) && (deviceType & AUDIO_DEVICE_OUT_AUX_DIGITAL)) {
                if (!strncmp(device_address, "hdmi_spkr", 9)) {
                    mHdmiAudioDisabled = false;
                } else {
                    mHdmiAudioEvent = true;
                }
                if (mHdmiAudioDisabled || !mHdmiAudioEvent) {
                    mAvailableOutputDevices.remove(device);
                    ALOGW("HDMI sink not connected, do not route audio to HDMI out");
                    return INVALID_OPERATION;
                }
            }

            // Before checking outputs, broadcast connect event to allow HAL to retrieve dynamic
            // parameters on newly connected devices (instead of opening the outputs...)
            broadcastDeviceConnectionState(device, state);

            if (checkOutputsForDevice(device, state, outputs) != NO_ERROR) {
                mAvailableOutputDevices.remove(device);

                mHwModules.cleanUpForDevice(device);

                broadcastDeviceConnectionState(device, AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE);
                return INVALID_OPERATION;
            }
            if (deviceType == AUDIO_DEVICE_OUT_AUX_DIGITAL) {
                chkDpConnAndAllowedForVoice();
            }

            // outputs should never be empty here
            ALOG_ASSERT(outputs.size() != 0, "setDeviceConnectionState():"
                    "checkOutputsForDevice() returned no outputs but status OK");
            ALOGV("%s() checkOutputsForDevice() returned %zu outputs", __func__, outputs.size());

            } break;
        // handle output device disconnection
        case AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE: {
            if (index < 0) {
                if (mApmConfigs->isHDMISpkEnabled() &&
                        (popcount(deviceType) == 1) && (deviceType & AUDIO_DEVICE_OUT_AUX_DIGITAL)) {
                    if (!strncmp(device_address, "hdmi_spkr", 9)) {
                        mHdmiAudioDisabled = true;
                    } else {
                        mHdmiAudioEvent = false;
                    }
                }
                ALOGW("setDeviceConnectionState() device not connected: %x", deviceType);
                return INVALID_OPERATION;
            }

            ALOGV("%s() disconnecting output device %s", __func__, device->toString().c_str());

            // Send Disconnect to HALs
            broadcastDeviceConnectionState(device, state);

            // remove device from available output devices
            mAvailableOutputDevices.remove(device);

            mOutputs.clearSessionRoutesForDevice(device);
            if (mApmConfigs->isHDMISpkEnabled() &&
                    (popcount(deviceType) == 1) && (deviceType & AUDIO_DEVICE_OUT_AUX_DIGITAL)) {
                if (!strncmp(device_address, "hdmi_spkr", 9)) {
                    mHdmiAudioDisabled = true;
                } else {
                    mHdmiAudioEvent = false;
                }
            }
            checkOutputsForDevice(device, state, outputs);

            // Reset active device codec
            device->setEncodedFormat(AUDIO_FORMAT_DEFAULT);

            if (deviceType == AUDIO_DEVICE_OUT_AUX_DIGITAL) {
                mEngine->setDpConnAndAllowedForVoice(false);
            }
            } break;

        default:
            ALOGE("%s() invalid state: %x", __func__, state);
            return BAD_VALUE;
        }

        // Propagate device availability to Engine
        setEngineDeviceConnectionState(device, state);

        if (!outputs.isEmpty()) {
            for (size_t i = 0; i < outputs.size(); i++) {
                sp<SwAudioOutputDescriptor> desc = mOutputs.valueFor(outputs[i]);
                // close voip output before track invalidation to allow creation of
                // new voip stream from restoreTrack
                if ((desc->mFlags == (AUDIO_OUTPUT_FLAG_DIRECT | AUDIO_OUTPUT_FLAG_VOIP_RX)) != 0) {
                    closeOutput(outputs[i]);
                    outputs.remove(outputs[i]);
                }
            }
        }

        // No need to evaluate playback routing when connecting a remote submix
        // output device used by a dynamic policy of type recorder as no
        // playback use case is affected.
        bool doCheckForDeviceAndOutputChanges = true;
        if (device->type() == AUDIO_DEVICE_OUT_REMOTE_SUBMIX
                && strncmp(device_address, "0", AUDIO_DEVICE_MAX_ADDRESS_LEN) != 0) {
            for (audio_io_handle_t output : outputs) {
                sp<SwAudioOutputDescriptor> desc = mOutputs.valueFor(output);
                sp<AudioPolicyMix> policyMix = desc->mPolicyMix.promote();
                if (policyMix != nullptr
                        && policyMix->mMixType == MIX_TYPE_RECORDERS
                        && strncmp(device_address,
                                   policyMix->mDeviceAddress.string(),
                                   AUDIO_DEVICE_MAX_ADDRESS_LEN) == 0) {
                    doCheckForDeviceAndOutputChanges = false;
                    break;
                }
            }
        }

        auto checkCloseOutputs = [&]() {
            // outputs must be closed after checkOutputForAllStrategies() is executed
            if (!outputs.isEmpty()) {
                for (audio_io_handle_t output : outputs) {
                    sp<SwAudioOutputDescriptor> desc = mOutputs.valueFor(output);
                    // close unused outputs after device disconnection or direct outputs that have
                    // been opened by checkOutputsForDevice() to query dynamic parameters
                    if ((state == AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE) ||
                            (((desc->mFlags & AUDIO_OUTPUT_FLAG_DIRECT) != 0) &&
                                (desc->mDirectOpenCount == 0))) {
                        closeOutput(output);
                    }
                }
                // check A2DP again after closing A2DP output to reset mA2dpSuspended if needed
                return true;
            }
            return false;
        };

        if (doCheckForDeviceAndOutputChanges) {
            checkForDeviceAndOutputChanges(checkCloseOutputs);
        } else {
            checkCloseOutputs();
        }

        if (mEngine->getPhoneState() == AUDIO_MODE_IN_CALL && hasPrimaryOutput()) {
            DeviceVector newDevices = getNewOutputDevices(mPrimaryOutput, false /*fromCache*/);
            updateCallRouting(newDevices);
        }
        const DeviceVector msdOutDevices = getMsdAudioOutDevices();
        for (size_t i = 0; i < mOutputs.size(); i++) {
            sp<SwAudioOutputDescriptor> desc = mOutputs.valueAt(i);
            if (desc->isActive() && ((mEngine->getPhoneState() != AUDIO_MODE_IN_CALL) ||
                (desc != mPrimaryOutput))) {
                DeviceVector newDevices = getNewOutputDevices(desc, true /*fromCache*/);
                // do not force device change on duplicated output because if device is 0, it will
                // also force a device 0 for the two outputs it is duplicated to which may override
                // a valid device selection on those outputs.
                bool force = (msdOutDevices.isEmpty() || msdOutDevices != desc->devices())
                        && !desc->isDuplicated()
                        && (!device_distinguishes_on_address(deviceType)
                                // always force when disconnecting (a non-duplicated device)
                                || (state == AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE));
                setOutputDevices(desc, newDevices, force, 0);
            }
        }

        if (state == AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE) {
            cleanUpForDevice(device);
        }

        mpClientInterface->onAudioPortListUpdate();
        return NO_ERROR;
    }  // end if is output device

    // handle input devices
    if (audio_is_input_device(deviceType)) {
        ssize_t index = mAvailableInputDevices.indexOf(device);
        switch (state)
        {
        // handle input device connection
        case AUDIO_POLICY_DEVICE_STATE_AVAILABLE: {
            if (index >= 0) {
                ALOGW("setDeviceConnectionState() device already connected: %d", deviceType);
                return INVALID_OPERATION;
            }

            if (mAvailableInputDevices.add(device) < 0) {
                return NO_MEMORY;
            }

            // Before checking intputs, broadcast connect event to allow HAL to retrieve dynamic
            // parameters on newly connected devices (instead of opening the inputs...)
            broadcastDeviceConnectionState(device, state);

            if (checkInputsForDevice(device, state) != NO_ERROR) {
                mAvailableInputDevices.remove(device);

                broadcastDeviceConnectionState(device, AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE);

                mHwModules.cleanUpForDevice(device);

                return INVALID_OPERATION;
            }

        } break;

        // handle input device disconnection
        case AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE: {
            if (index < 0) {
                ALOGW("setDeviceConnectionState() device not connected: %d", deviceType);
                return INVALID_OPERATION;
            }

            ALOGV("setDeviceConnectionState() disconnecting input device %x", deviceType);

            // Set Disconnect to HALs
            broadcastDeviceConnectionState(device, state);

            mAvailableInputDevices.remove(device);

            checkInputsForDevice(device, state);

        } break;

        default:
            ALOGE("setDeviceConnectionState() invalid state: %x", state);
            return BAD_VALUE;
        }

        // Propagate device availability to Engine
        setEngineDeviceConnectionState(device, state);

        checkCloseInputs();
        /*audio policy: fix call volume over USB*/
        // As the input device list can impact the output device selection, update
        // getDeviceForStrategy() cache
        updateDevicesAndOutputs();

        if (mEngine->getPhoneState() == AUDIO_MODE_IN_CALL && hasPrimaryOutput()) {
            DeviceVector newDevices = getNewOutputDevices(mPrimaryOutput, false /*fromCache*/);
            updateCallRouting(newDevices);
        }

        if (state == AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE) {
            cleanUpForDevice(device);
        }

        mpClientInterface->onAudioPortListUpdate();
        return NO_ERROR;
    } // end if is input device

    ALOGW("setDeviceConnectionState() invalid device: %x", deviceType);
    return BAD_VALUE;
}



void AudioPolicyManagerCustom::chkDpConnAndAllowedForVoice()
{
    String8 value;
    bool connAndAllowed = false;
    String8 valueStr = mpClientInterface->getParameters((audio_io_handle_t)0,
                                                        String8("dp_for_voice"));

    AudioParameter result = AudioParameter(valueStr);
    if (result.get(String8("dp_for_voice"), value) == NO_ERROR) {
        connAndAllowed = value.contains("true");
    }
    mEngine->setDpConnAndAllowedForVoice(connAndAllowed);
}

bool AudioPolicyManagerCustom::isInvalidationOfMusicStreamNeeded(const audio_attributes_t &attr)
{
    if (followsSameRouting(attr, attributes_initializer(AUDIO_USAGE_MEDIA))) {
        for (size_t i = 0; i < mOutputs.size(); i++) {
            sp<SwAudioOutputDescriptor> newOutputDesc = mOutputs.valueAt(i);
            if (newOutputDesc->getFormat() == AUDIO_FORMAT_DSD)
                return false;
        }
    }
    return true;
}


void AudioPolicyManagerCustom::checkOutputForAttributes(const audio_attributes_t &attr)
{

    DeviceVector oldDevices = mEngine->getOutputDevicesForAttributes(attr, 0, true /*fromCache*/);
    DeviceVector newDevices = mEngine->getOutputDevicesForAttributes(attr, 0, false /*fromCache*/);
    SortedVector<audio_io_handle_t> srcOutputs = getOutputsForDevices(oldDevices, mPreviousOutputs);
    SortedVector<audio_io_handle_t> dstOutputs = getOutputsForDevices(newDevices, mOutputs);

    // also take into account external policy-related changes: add all outputs which are
    // associated with policies in the "before" and "after" output vectors
    ALOGVV("%s(): policy related outputs", __func__);
    for (size_t i = 0 ; i < mPreviousOutputs.size() ; i++) {
        const sp<SwAudioOutputDescriptor> desc = mPreviousOutputs.valueAt(i);
        if (desc != 0 && desc->mPolicyMix != NULL) {
            srcOutputs.add(desc->mIoHandle);
            ALOGVV(" previous outputs: adding %d", desc->mIoHandle);
        }
    }
    for (size_t i = 0 ; i < mOutputs.size() ; i++) {
        const sp<SwAudioOutputDescriptor> desc = mOutputs.valueAt(i);
        if (desc != 0 && desc->mPolicyMix != NULL) {
            dstOutputs.add(desc->mIoHandle);
            ALOGVV(" new outputs: adding %d", desc->mIoHandle);
        }
    }

    if ((srcOutputs != dstOutputs) && isInvalidationOfMusicStreamNeeded(attr)) {
        AudioPolicyManager::checkOutputForAttributes(attr);
    }
}

// This function checks for the parameters which can be offloaded.
// This can be enhanced depending on the capability of the DSP and policy
// of the system.
bool AudioPolicyManagerCustom::isOffloadSupported(const audio_offload_info_t& offloadInfo)
{
    DeviceVector primaryInputDevices = availablePrimaryModuleInputDevices();
    ALOGV("isOffloadSupported: SR=%u, CM=0x%x, Format=0x%x, StreamType=%d,"
     " BitRate=%u, duration=%" PRId64 " us, has_video=%d",
     offloadInfo.sample_rate, offloadInfo.channel_mask,
     offloadInfo.format,
     offloadInfo.stream_type, offloadInfo.bit_rate, offloadInfo.duration_us,
     offloadInfo.has_video);

     if (mMasterMono) {
        return false; // no offloading if mono is set.
     }

    if (mApmConfigs->isVoiceConcEnabled()) {
        if (mApmConfigs->isVoicePlayConcDisabled() && isInCall()) {
            ALOGD("\n copl: blocking  compress offload on call mode\n");
            return false;
        }
    }
    if (mApmConfigs->isVoiceDSDConcDisabled() &&
        isInCall() &&  (offloadInfo.format == AUDIO_FORMAT_DSD)) {
        ALOGD("blocking DSD compress offload on call mode");
        return false;
    }
    if (mApmConfigs->isRecPlayConcEnabled()) {
        if (mApmConfigs->isRecPlayConcDisabled() &&
             ((true == mIsInputRequestOnProgress) ||
             (mInputs.activeInputsCountOnDevices(primaryInputDevices) > 0))) {
            ALOGD("copl: blocking  compress offload for record concurrency");
            return false;
        }
    }
    // Check if stream type is music, then only allow offload as of now.
    if (offloadInfo.stream_type != AUDIO_STREAM_MUSIC)
    {
        ALOGV("isOffloadSupported: stream_type != MUSIC, returning false");
        return false;
    }

    // Check if offload has been disabled
    bool offloadDisabled = mApmConfigs->isAudioOffloadDisabled();
    if (offloadDisabled) {
        ALOGI("offload disabled by audio.offload.disable=%d", offloadDisabled);
        return false;
    }

    //check if it's multi-channel AAC (includes sub formats) and FLAC format
    if ((popcount(offloadInfo.channel_mask) > 2) &&
        (((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_AAC) ||
        ((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_VORBIS))) {
        ALOGD("offload disabled for multi-channel AAC,FLAC and VORBIS format");
        return false;
    }

    if (mApmConfigs->isExtnFormatsEnabled()) {
        //check if it's multi-channel FLAC/ALAC/WMA format with sample rate > 48k
        if ((popcount(offloadInfo.channel_mask) > 2) &&
            (((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_FLAC) ||
            (((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_ALAC) && (offloadInfo.sample_rate > 48000)) ||
            (((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_WMA) && (offloadInfo.sample_rate > 48000)) ||
            (((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_WMA_PRO) && (offloadInfo.sample_rate > 48000)) ||
            ((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_AAC_ADTS))) {
                ALOGD("offload disabled for multi-channel FLAC/ALAC/WMA/AAC_ADTS clips with sample rate > 48kHz");
            return false;
        }

        // check against wma std bit rate restriction
        if ((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_WMA) {
            int32_t sr_id = -1;
            uint32_t min_bitrate, max_bitrate;
            for (int i = 0; i < WMA_STD_NUM_FREQ; i++) {
                if (offloadInfo.sample_rate == wmaStdSampleRateTbl[i]) {
                    sr_id = i;
                    break;
                }
            }
            if ((sr_id < 0) || (popcount(offloadInfo.channel_mask) > 2)
                    || (popcount(offloadInfo.channel_mask) <= 0)) {
                ALOGE("invalid sample rate or channel count");
                return false;
            }

            min_bitrate = wmaStdMinAvgByteRateTbl[sr_id][popcount(offloadInfo.channel_mask) - 1];
            max_bitrate = wmaStdMaxAvgByteRateTbl[sr_id][popcount(offloadInfo.channel_mask) - 1];
            if ((offloadInfo.bit_rate > max_bitrate) || (offloadInfo.bit_rate < min_bitrate)) {
                ALOGD("offload disabled for WMA clips with unsupported bit rate");
                ALOGD("bit_rate %d, max_bitrate %d, min_bitrate %d", offloadInfo.bit_rate, max_bitrate, min_bitrate);
                return false;
            }
        }

        // Safely choose the min bitrate as threshold and leave the restriction to NT decoder as we can't distinguish wma pro and wma lossless here.
        if ((((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_WMA_PRO) && (offloadInfo.bit_rate > MAX_BITRATE_WMA_PRO)) ||
            (((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_WMA_PRO) && (offloadInfo.bit_rate > MAX_BITRATE_WMA_LOSSLESS))) {
            ALOGD("offload disabled for WMA_PRO/WMA_LOSSLESS clips with bit rate over maximum supported value");
            return false;
        }
    }

    //TODO: enable audio offloading with video when ready
    if (offloadInfo.has_video && !mApmConfigs->isAudioOffloadVideoEnabled()) {
        ALOGV("isOffloadSupported: has_video == true, returning false");
        return false;
    }

    if (offloadInfo.has_video && offloadInfo.is_streaming &&
            !mApmConfigs->isAVStreamingOffloadEnabled()) {
        ALOGW("offload disabled by vendor.audio.av.streaming.offload.enable %d",
               mApmConfigs->isAVStreamingOffloadEnabled());
        return false;
    }

    //If duration is less than minimum value defined in property, return false
    if (mApmConfigs->getAudioOffloadMinDuration() > 0) {
        if (offloadInfo.duration_us < (mApmConfigs->getAudioOffloadMinDuration() * 1000000 )) {
            ALOGV("Offload denied by duration < audio.offload.min.duration.secs(=%u)", mApmConfigs->getAudioOffloadMinDuration());
            return false;
        }
    } else if (offloadInfo.duration_us < OFFLOAD_DEFAULT_MIN_DURATION_SECS * 1000000) {
        ALOGV("Offload denied by duration < default min(=%u)", OFFLOAD_DEFAULT_MIN_DURATION_SECS);
        //duration checks only valid for MP3/AAC/ formats,
        //do not check duration for other audio formats, e.g. AAC/AC3 and amrwb+ formats
        if ((offloadInfo.format == AUDIO_FORMAT_MP3) ||
            ((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_AAC) ||
            ((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_FLAC) ||
            ((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_VORBIS))
            return false;

        if (mApmConfigs->isExtnFormatsEnabled()) {
            if (((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_WMA) ||
                ((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_WMA_PRO) ||
                ((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_ALAC) ||
                ((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_APE) ||
                ((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_DSD) ||
                ((offloadInfo.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_AAC_ADTS))
                return false;
        }
    }

    // Do not allow offloading if one non offloadable effect is enabled. This prevents from
    // creating an offloaded track and tearing it down immediately after start when audioflinger
    // detects there is an active non offloadable effect.
    // FIXME: We should check the audio session here but we do not have it in this context.
    // This may prevent offloading in rare situations where effects are left active by apps
    // in the background.
    if (mEffects.isNonOffloadableEffectEnabled()) {
        return false;
    }

    // See if there is a profile to support this.
    // AUDIO_DEVICE_NONE
    sp<IOProfile> profile = getProfileForOutput(DeviceVector() /*ignore device */,
                                            offloadInfo.sample_rate,
                                            offloadInfo.format,
                                            offloadInfo.channel_mask,
                                            AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD,
                                            true /*directOnly*/);
    ALOGV("isOffloadSupported() profile %sfound", profile != 0 ? "" : "NOT ");
    return (profile != 0);
}

void AudioPolicyManagerCustom::setPhoneState(audio_mode_t state)
{
    ALOGD("setPhoneState() state %d", state);
    // store previous phone state for management of sonification strategy below
    int oldState = mEngine->getPhoneState();

    if (mEngine->setPhoneState(state) != NO_ERROR) {
        ALOGW("setPhoneState() invalid or same state %d", state);
        return;
    }
    /// Opens: can these line be executed after the switch of volume curves???
    if (isStateInCall(oldState)) {
        ALOGV("setPhoneState() in call state management: new state is %d", state);

        // force reevaluating accessibility routing when call stops
        mpClientInterface->invalidateStream(AUDIO_STREAM_ACCESSIBILITY);
    }

    /**
     * Switching to or from incall state or switching between telephony and VoIP lead to force
     * routing command.
     */
    bool force = ((is_state_in_call(oldState) != is_state_in_call(state))
                  || (is_state_in_call(state) && (state != oldState)));

    // check for device and output changes triggered by new phone state
    checkForDeviceAndOutputChanges();

    sp<SwAudioOutputDescriptor> hwOutputDesc = mPrimaryOutput;
    if (mApmConfigs->isVoiceConcEnabled()) {
        bool prop_playback_enabled = mApmConfigs->isVoicePlayConcDisabled();
        bool prop_rec_enabled = mApmConfigs->isVoiceRecConcDisabled();
        bool prop_voip_enabled = mApmConfigs->isVoiceVOIPConcDisabled();

        if ((AUDIO_MODE_IN_CALL != oldState) && (AUDIO_MODE_IN_CALL == state)) {
            ALOGD("voice_conc:Entering to call mode oldState :: %d state::%d ",
                oldState, state);
            mvoice_call_state = state;
            if (prop_rec_enabled) {
                //Close all active inputs
                Vector<sp <AudioInputDescriptor> > activeInputs = mInputs.getActiveInputs();
                if (activeInputs.size() != 0) {
                   for (size_t i = 0; i <  activeInputs.size(); i++) {
                       sp<AudioInputDescriptor> activeInput = activeInputs[i];
                       switch(activeInput->source()) {
                           case AUDIO_SOURCE_VOICE_UPLINK:
                           case AUDIO_SOURCE_VOICE_DOWNLINK:
                           case AUDIO_SOURCE_VOICE_CALL:
                               ALOGD("voice_conc:FOUND active input during call active: %d",
                                       activeInput->source());
                           break;

                           case  AUDIO_SOURCE_VOICE_COMMUNICATION:
                                if (prop_voip_enabled) {
                                    ALOGD("voice_conc:CLOSING VoIP input source on call setup :%d ",
                                            activeInput->source());
                                    RecordClientVector activeClients = activeInput->clientsList(true /*activeOnly*/);
                                    for (const auto& activeClient : activeClients) {
                                        closeClient(activeClient->portId());
                                    }
                                }
                           break;

                           default:
                               ALOGD("voice_conc:CLOSING input on call setup  for inputSource: %d",
                                       activeInput->source());
                               RecordClientVector activeClients = activeInput->clientsList(true /*activeOnly*/);
                               for (const auto& activeClient : activeClients) {
                                   closeClient(activeClient->portId());
                               }
                           break;
                       }
                   }
               }
            } else if (prop_voip_enabled) {
                Vector<sp <AudioInputDescriptor> > activeInputs = mInputs.getActiveInputs();
                if (activeInputs.size() != 0) {
                    for (size_t i = 0; i <  activeInputs.size(); i++) {
                        sp<AudioInputDescriptor> activeInput = activeInputs[i];
                        if (AUDIO_SOURCE_VOICE_COMMUNICATION == activeInput->source()) {
                            ALOGD("voice_conc:CLOSING VoIP on call setup : %d",activeInput->source());
                            RecordClientVector activeClients = activeInput->clientsList(true /*activeOnly*/);
                            for (const auto& activeClient : activeClients) {
                                closeClient(activeClient->portId());
                            }
                        }
                    }
                }
            }
            if (prop_playback_enabled) {
                // Move tracks associated to this strategy from previous output to new output
                for (int i = AUDIO_STREAM_SYSTEM; i < AUDIO_STREAM_FOR_POLICY_CNT; i++) {
                    ALOGV("voice_conc:Invalidate on call mode for stream :: %d ", i);
                    if (AUDIO_OUTPUT_FLAG_DEEP_BUFFER == mFallBackflag) {
                        if ((AUDIO_STREAM_MUSIC == i) ||
                            (AUDIO_STREAM_VOICE_CALL == i) ) {
                            ALOGD("voice_conc:Invalidate stream type %d", i);
                            mpClientInterface->invalidateStream((audio_stream_type_t)i);
                        }
                    } else if (AUDIO_OUTPUT_FLAG_FAST == mFallBackflag) {
                        ALOGD("voice_conc:Invalidate stream type %d", i);
                        mpClientInterface->invalidateStream((audio_stream_type_t)i);
                    }
                }
            }

            for (size_t i = 0; i < mOutputs.size(); i++) {
                sp<SwAudioOutputDescriptor> outputDesc = mOutputs.valueAt(i);
                if ( (outputDesc == NULL) || (outputDesc->mProfile == NULL)) {
                   ALOGD("voice_conc:ouput desc / profile is NULL");
                   continue;
                }

                bool isFastFallBackNeeded =
                   ((AUDIO_OUTPUT_FLAG_DEEP_BUFFER | AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD | AUDIO_OUTPUT_FLAG_DIRECT_PCM) & outputDesc->mProfile->getFlags());

                if ((AUDIO_OUTPUT_FLAG_FAST == mFallBackflag) && isFastFallBackNeeded) {
                    if (((!outputDesc->isDuplicated() && outputDesc->mProfile->getFlags() & AUDIO_OUTPUT_FLAG_PRIMARY))
                                && prop_playback_enabled) {
                        ALOGD("voice_conc:calling suspendOutput on call mode for primary output");
                        mpClientInterface->suspendOutput(mOutputs.keyAt(i));
                    } //Close compress all sessions
                    else if ((outputDesc->mProfile->getFlags() & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD)
                                    &&  prop_playback_enabled) {
                        ALOGD("voice_conc:calling closeOutput on call mode for COMPRESS output");
                        closeOutput(mOutputs.keyAt(i));
                    }
                    else if ((outputDesc->mProfile->getFlags() & AUDIO_OUTPUT_FLAG_VOIP_RX)
                                    && prop_voip_enabled) {
                        ALOGD("voice_conc:calling closeOutput on call mode for DIRECT  output");
                        closeOutput(mOutputs.keyAt(i));
                    }
                } else if (AUDIO_OUTPUT_FLAG_DEEP_BUFFER == mFallBackflag) {
                    if (outputDesc->mProfile->getFlags() & AUDIO_OUTPUT_FLAG_VOIP_RX) {
                        if (prop_voip_enabled) {
                            ALOGD("voice_conc:calling closeOutput on call mode for DIRECT  output");
                            closeOutput(mOutputs.keyAt(i));
                        }
                    }
                    else if (prop_playback_enabled
                               && (outputDesc->mProfile->getFlags() & AUDIO_OUTPUT_FLAG_DIRECT)) {
                        ALOGD("voice_conc:calling closeOutput on call mode for COMPRESS output");
                        closeOutput(mOutputs.keyAt(i));
                    }
                }
            }
        }

        if ((AUDIO_MODE_IN_CALL == oldState || AUDIO_MODE_IN_COMMUNICATION == oldState) &&
           (AUDIO_MODE_NORMAL == state) && prop_playback_enabled && mvoice_call_state) {
            ALOGD("voice_conc:EXITING from call mode oldState :: %d state::%d \n",oldState, state);
            mvoice_call_state = 0;
            if (AUDIO_OUTPUT_FLAG_FAST == mFallBackflag) {
                //restore PCM (deep-buffer) output after call termination
                for (size_t i = 0; i < mOutputs.size(); i++) {
                    sp<SwAudioOutputDescriptor> outputDesc = mOutputs.valueAt(i);
                    if ( (outputDesc == NULL) || (outputDesc->mProfile == NULL)) {
                       ALOGD("voice_conc:ouput desc / profile is NULL");
                       continue;
                    }
                    if (!outputDesc->isDuplicated() && outputDesc->mProfile->getFlags() & AUDIO_OUTPUT_FLAG_PRIMARY) {
                        ALOGD("voice_conc:calling restoreOutput after call mode for primary output");
                        mpClientInterface->restoreOutput(mOutputs.keyAt(i));
                    }
               }
            }
           //call invalidate tracks so that any open streams can fall back to deep buffer/compress path from ULL
            for (int i = AUDIO_STREAM_SYSTEM; i < AUDIO_STREAM_FOR_POLICY_CNT; i++) {
                ALOGV("voice_conc:Invalidate on call mode for stream :: %d ", i);
                if (AUDIO_OUTPUT_FLAG_DEEP_BUFFER == mFallBackflag) {
                    if ((AUDIO_STREAM_MUSIC == i) ||
                        (AUDIO_STREAM_VOICE_CALL == i) ) {
                        mpClientInterface->invalidateStream((audio_stream_type_t)i);
                    }
                } else if (AUDIO_OUTPUT_FLAG_FAST == mFallBackflag) {
                    mpClientInterface->invalidateStream((audio_stream_type_t)i);
                }
            }
        }
    }

    sp<SwAudioOutputDescriptor> outputDesc = NULL;
    for (size_t i = 0; i < mOutputs.size(); i++) {
        outputDesc = mOutputs.valueAt(i);
        if ((outputDesc == NULL) || (outputDesc->mProfile == NULL)) {
            ALOGD("voice_conc:ouput desc / profile is NULL");
            continue;
        }

        if (mApmConfigs->isVoiceDSDConcDisabled() &&
            (outputDesc->mFlags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) &&
            (outputDesc->getFormat() == AUDIO_FORMAT_DSD)) {
            ALOGD("voice_conc:calling closeOutput on call mode for DSD COMPRESS output");
            closeOutput(mOutputs.keyAt(i));
            // call invalidate for music, so that DSD compress will fallback to deep-buffer.
            mpClientInterface->invalidateStream(AUDIO_STREAM_MUSIC);
        }

    }

    if (mApmConfigs->isRecPlayConcEnabled()) {
        if (mApmConfigs->isRecPlayConcDisabled()) {
            if (AUDIO_MODE_IN_COMMUNICATION == mEngine->getPhoneState()) {
                ALOGD("phone state changed to MODE_IN_COMM invlaidating music and voice streams");
                // call invalidate for voice streams, so that it can use deepbuffer with VoIP out device from HAL
                mpClientInterface->invalidateStream(AUDIO_STREAM_VOICE_CALL);
                // call invalidate for music, so that compress will fallback to deep-buffer with VoIP out device
                mpClientInterface->invalidateStream(AUDIO_STREAM_MUSIC);

                // close compress output to make sure session will be closed before timeout(60sec)
                for (size_t i = 0; i < mOutputs.size(); i++) {

                    sp<SwAudioOutputDescriptor> outputDesc = mOutputs.valueAt(i);
                    if ((outputDesc == NULL) || (outputDesc->mProfile == NULL)) {
                       ALOGD("ouput desc / profile is NULL");
                       continue;
                    }

                    if (outputDesc->mProfile->getFlags() & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) {
                        ALOGD("calling closeOutput on call mode for COMPRESS output");
                        closeOutput(mOutputs.keyAt(i));
                    }
                }
            } else if ((oldState == AUDIO_MODE_IN_COMMUNICATION) &&
                        (mEngine->getPhoneState() == AUDIO_MODE_NORMAL)) {
                // call invalidate for music so that music can fallback to compress
                mpClientInterface->invalidateStream(AUDIO_STREAM_MUSIC);
            }
        }
    }
    mPrevPhoneState = oldState;
    int delayMs = 0;
    if (isStateInCall(state)) {
        nsecs_t sysTime = systemTime();
        auto musicStrategy = streamToStrategy(AUDIO_STREAM_MUSIC);
        auto sonificationStrategy = streamToStrategy(AUDIO_STREAM_ALARM);

        for (size_t i = 0; i < mOutputs.size(); i++) {
            sp<SwAudioOutputDescriptor> desc = mOutputs.valueAt(i);
            // mute media and sonification strategies and delay device switch by the largest
            // latency of any output where either strategy is active.
            // This avoid sending the ring tone or music tail into the earpiece or headset.
            if ((desc->isStrategyActive(musicStrategy,
                                  SONIFICATION_HEADSET_MUSIC_DELAY,
                                  sysTime) ||
                 desc->isStrategyActive(sonificationStrategy,
                                  SONIFICATION_HEADSET_MUSIC_DELAY,
                                  sysTime)) &&
                    (delayMs < (int)desc->latency()*2)) {
                delayMs = desc->latency()*2;
            }
            setStrategyMute(musicStrategy, true, desc);
            setStrategyMute(musicStrategy, false, desc, MUTE_TIME_MS,
                mEngine->getOutputDevicesForAttributes(attributes_initializer(AUDIO_USAGE_MEDIA),
                                                       nullptr, true /*fromCache*/).types());
            setStrategyMute(sonificationStrategy, true, desc);
            setStrategyMute(sonificationStrategy, false, desc, MUTE_TIME_MS,
                 mEngine->getOutputDevicesForAttributes(attributes_initializer(AUDIO_USAGE_ALARM),
                                                       nullptr, true /*fromCache*/).types());
        }
    }

    if (hasPrimaryOutput()) {
        // Note that despite the fact that getNewOutputDevice() is called on the primary output,
        // the device returned is not necessarily reachable via this output
        DeviceVector rxDevices = getNewOutputDevices(mPrimaryOutput, false /*fromCache*/);
        // force routing command to audio hardware when ending call
        // even if no device change is needed
        if (isStateInCall(oldState) && rxDevices.isEmpty()) {
            rxDevices = mPrimaryOutput->devices();
        }

        if (state == AUDIO_MODE_IN_CALL) {
            updateCallRouting(rxDevices, delayMs);
        } else if (oldState == AUDIO_MODE_IN_CALL) {
            if (mCallRxPatch != 0) {
                mpClientInterface->releaseAudioPatch(mCallRxPatch->getAfHandle(), 0);
                mCallRxPatch.clear();
            }
            if (mCallTxPatch != 0) {
                mpClientInterface->releaseAudioPatch(mCallTxPatch->getAfHandle(), 0);
                mCallTxPatch.clear();
            }
            setOutputDevices(mPrimaryOutput, rxDevices, force, 0);
        } else {
            setOutputDevices(mPrimaryOutput, rxDevices, force, 0);
        }
    }

    // reevaluate routing on all outputs in case tracks have been started during the call
    for (size_t i = 0; i < mOutputs.size(); i++) {
        sp<SwAudioOutputDescriptor> desc = mOutputs.valueAt(i);
        DeviceVector newDevices = getNewOutputDevices(desc, true /*fromCache*/);
        if (state != AUDIO_MODE_IN_CALL || desc != mPrimaryOutput) {
            setOutputDevices(desc, newDevices, !newDevices.isEmpty(), 0 /*delayMs*/);
        }
    }
    if (isStateInCall(state)) {
        ALOGV("setPhoneState() in call state management: new state is %d", state);

       // force reevaluating accessibility routing when call starts
       mpClientInterface->invalidateStream(AUDIO_STREAM_ACCESSIBILITY);
    }

    // Flag that ringtone volume must be limited to music volume until we exit MODE_RINGTONE
    mLimitRingtoneVolume = (state == AUDIO_MODE_RINGTONE &&
                            isStreamActive(AUDIO_STREAM_MUSIC, SONIFICATION_HEADSET_MUSIC_DELAY));
}

void AudioPolicyManagerCustom::setForceUse(audio_policy_force_use_t usage,
                                         audio_policy_forced_cfg_t config)
{
    ALOGD("setForceUse() usage %d, config %d, mPhoneState %d", usage, config, mEngine->getPhoneState());
    if (config == mEngine->getForceUse(usage)) {
        return;
    }

    if (mEngine->setForceUse(usage, config) != NO_ERROR) {
        ALOGW("setForceUse() could not set force cfg %d for usage %d", config, usage);
        return;
    }
    bool forceVolumeReeval = (usage == AUDIO_POLICY_FORCE_FOR_COMMUNICATION) ||
            (usage == AUDIO_POLICY_FORCE_FOR_DOCK) ||
            (usage == AUDIO_POLICY_FORCE_FOR_SYSTEM);

    // check for device and output changes triggered by new force usage
    checkForDeviceAndOutputChanges();

     // force client reconnection to reevaluate flag AUDIO_FLAG_AUDIBILITY_ENFORCED
     if (usage == AUDIO_POLICY_FORCE_FOR_SYSTEM) {
         mpClientInterface->invalidateStream(AUDIO_STREAM_SYSTEM);
         mpClientInterface->invalidateStream(AUDIO_STREAM_ENFORCED_AUDIBLE);
     }

    //FIXME: workaround for truncated touch sounds
    // to be removed when the problem is handled by system UI
    uint32_t delayMs = 0;
    uint32_t waitMs = 0;
    if (usage == AUDIO_POLICY_FORCE_FOR_COMMUNICATION) {
        delayMs = TOUCH_SOUND_FIXED_DELAY_MS;
    }
    if (mEngine->getPhoneState() == AUDIO_MODE_IN_CALL && hasPrimaryOutput()) {
        DeviceVector newDevices = getNewOutputDevices(mPrimaryOutput, true /*fromCache*/);
        if (forceVolumeReeval && !newDevices.isEmpty()) {
            applyStreamVolumes(mPrimaryOutput, newDevices.types(), delayMs, true);
        }
        waitMs = updateCallRouting(newDevices, delayMs);
    }
    // Use reverse loop to make sure any low latency usecases (generally tones)
    // are not routed before non LL usecases (generally music).
    // We can safely assume that LL output would always have lower index,
    // and use this work-around to avoid routing of output with music stream
    // from the context of short lived LL output.
    // Note: in case output's share backend(HAL sharing is implicit) all outputs
    //       gets routing update while processing first output itself.
    for (size_t i = mOutputs.size(); i > 0; i--) {
        sp<SwAudioOutputDescriptor> outputDesc = mOutputs.valueAt(i-1);
        DeviceVector newDevices = getNewOutputDevices(outputDesc, true /*fromCache*/);
        if (outputDesc->isActive() && ((mEngine->getPhoneState() != AUDIO_MODE_IN_CALL) ||
            (outputDesc != mPrimaryOutput))) {
            waitMs = setOutputDevices(outputDesc, newDevices, !newDevices.isEmpty(),
                                     delayMs);

            if (forceVolumeReeval && !newDevices.isEmpty()) {
                applyStreamVolumes(outputDesc, newDevices.types(), waitMs, true);
            }
         }
    }

    for (const auto& activeDesc : mInputs.getActiveInputs()) {
        // Skip for hotword recording as the input device switch
        // is handled within sound trigger HAL
        if (activeDesc->isSoundTrigger() &&
            activeDesc->source() == AUDIO_SOURCE_HOTWORD) {
            continue;
        }
        auto newDevice = getNewInputDevice(activeDesc);
        // Force new input selection if the new device can not be reached via current input
        if (activeDesc->mProfile->getSupportedDevices().contains(newDevice)) {
            setInputDevice(activeDesc->mIoHandle, newDevice);
        } else {
            closeInput(activeDesc->mIoHandle);
        }
    }
}

status_t AudioPolicyManagerCustom::stopSource(const sp<SwAudioOutputDescriptor>& outputDesc,
                                              const sp<TrackClientDescriptor>& client)
{
    // always handle stream stop, check which stream type is stopping
    audio_stream_type_t stream = client->stream();
    auto clientVolSrc = client->volumeSource();

    if (stream < 0 || stream >= AUDIO_STREAM_CNT) {
        ALOGW("stopSource() invalid stream %d", stream);
        return INVALID_OPERATION;
    }
    handleEventForBeacon(stream == AUDIO_STREAM_TTS ? STOPPING_BEACON : STOPPING_OUTPUT);

    if (outputDesc->getActivityCount(clientVolSrc) > 0) {
        if (outputDesc->getActivityCount(clientVolSrc) == 1) {
            // Automatically disable the remote submix input when output is stopped on a
            // re routing mix of type MIX_TYPE_RECORDERS
            sp<AudioPolicyMix> policyMix = outputDesc->mPolicyMix.promote();
            if (isSingleDeviceType(outputDesc->devices().types(), &audio_is_remote_submix_device) &&
                policyMix != NULL &&
                policyMix->mMixType == MIX_TYPE_RECORDERS) {
                setDeviceConnectionStateInt(AUDIO_DEVICE_IN_REMOTE_SUBMIX,
                                            AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE,
                                            policyMix->mDeviceAddress,
                                            "remote-submix", AUDIO_FORMAT_DEFAULT);
            }
        }
        bool forceDeviceUpdate = false;
        if (client->hasPreferredDevice(true)) {
            checkStrategyRoute(client->strategy(), AUDIO_IO_HANDLE_NONE);
            forceDeviceUpdate = true;
        }

        // decrement usage count of this stream on the output
        outputDesc->setClientActive(client, false);

        // store time at which the stream was stopped - see isStreamActive()
        if (outputDesc->getActivityCount(clientVolSrc) == 0 || forceDeviceUpdate) {
            outputDesc->setStopTime(client, systemTime());
            DeviceVector prevDevices = outputDesc->devices();
            DeviceVector newDevices = getNewOutputDevices(outputDesc, false /*fromCache*/);
            // delay the device switch by twice the latency because stopOutput() is executed when
            // the track stop() command is received and at that time the audio track buffer can
            // still contain data that needs to be drained. The latency only covers the audio HAL
            // and kernel buffers. Also the latency does not always include additional delay in the
            // audio path (audio DSP, CODEC ...)
            setOutputDevices(outputDesc, newDevices, false, outputDesc->latency()*2);

            // force restoring the device selection on other active outputs if it differs from the
            // one being selected for this output
            for (size_t i = 0; i < mOutputs.size(); i++) {
                audio_io_handle_t curOutput = mOutputs.keyAt(i);
                sp<SwAudioOutputDescriptor> desc = mOutputs.valueAt(i);
                if (desc != outputDesc &&
                        desc->isActive() &&
                        outputDesc->sharesHwModuleWith(desc) &&
                        (newDevices != desc->devices())) {
                        DeviceVector dev = getNewOutputDevices(mOutputs.valueFor(curOutput), false /*fromCache*/);
                        bool force = desc->devices() != dev;
                        uint32_t delayMs;
                        if (dev == prevDevices) {
                            delayMs = 0;
                        } else {
                            delayMs = outputDesc->latency()*2;
                        }
                        setOutputDevices(desc,
                                    dev,
                                    force,
                                    delayMs);
                     // re-apply device specific volume if not done by setOutputDevice()
                     if (!force) {
                         applyStreamVolumes(desc, dev.types(), delayMs);
                     }
                }
            }
            // update the outputs if stopping one with a stream that can affect notification routing
            handleNotificationRoutingForStream(stream);
        }

        if (stream == AUDIO_STREAM_ENFORCED_AUDIBLE &&
                mEngine->getForceUse(AUDIO_POLICY_FORCE_FOR_SYSTEM) == AUDIO_POLICY_FORCE_SYSTEM_ENFORCED) {
            setStrategyMute(streamToStrategy(AUDIO_STREAM_ALARM), false, outputDesc);
        }

        if (followsSameRouting(client->attributes(), attributes_initializer(AUDIO_USAGE_MEDIA))) {
            selectOutputForMusicEffects();
        }
        return NO_ERROR;
    } else {
        ALOGW("stopOutput() refcount is already 0");
        return INVALID_OPERATION;
    }
}

status_t AudioPolicyManagerCustom::startSource(const sp<SwAudioOutputDescriptor>& outputDesc,
                                               const sp<TrackClientDescriptor>& client,
                                               uint32_t *delayMs)
{
    // cannot start playback of STREAM_TTS if any other output is being used
    uint32_t beaconMuteLatency = 0;
    audio_stream_type_t stream = client->stream();
    auto clientVolSrc = client->volumeSource();
    auto clientStrategy = client->strategy();
    auto clientAttr = client->attributes();

    if (stream < 0 || stream >= AUDIO_STREAM_CNT) {
        ALOGW("startSource() invalid stream %d", stream);
        return INVALID_OPERATION;
    }

    *delayMs = 0;
    if (stream == AUDIO_STREAM_TTS) {
        ALOGV("\t found BEACON stream");
        if (!mTtsOutputAvailable && mOutputs.isAnyOutputActive(
                    toVolumeSource(AUDIO_STREAM_TTS) /*sourceToIgnore*/)) {
            return INVALID_OPERATION;
        } else {
            beaconMuteLatency = handleEventForBeacon(STARTING_BEACON);
        }
    } else {
        // some playback other than beacon starts
        beaconMuteLatency = handleEventForBeacon(STARTING_OUTPUT);
    }

    // force device change if the output is inactive and no audio patch is already present.
    // check active before incrementing usage count
    bool force = !outputDesc->isActive() &&
            (outputDesc->getPatchHandle() == AUDIO_PATCH_HANDLE_NONE);

    DeviceVector devices;
    sp<AudioPolicyMix> policyMix = outputDesc->mPolicyMix.promote();
    const char *address = NULL;
    if (policyMix != NULL) {
        audio_devices_t newDeviceType;
        address = policyMix->mDeviceAddress.string();
        if ((policyMix->mRouteFlags & MIX_ROUTE_FLAG_LOOP_BACK) == MIX_ROUTE_FLAG_LOOP_BACK) {
            newDeviceType = AUDIO_DEVICE_OUT_REMOTE_SUBMIX;
        } else {
            newDeviceType = policyMix->mDeviceType;
        }
        sp device = mAvailableOutputDevices.getDevice(newDeviceType, String8(address),
                                                        AUDIO_FORMAT_DEFAULT);
        ALOG_ASSERT(device, "%s: no device found t=%u, a=%s", __func__, newDeviceType, address);
        devices.add(device);
    }

    // requiresMuteCheck is false when we can bypass mute strategy.
    // It covers a common case when there is no materially active audio
    // and muting would result in unnecessary delay and dropped audio.
    const uint32_t outputLatencyMs = outputDesc->latency();
    bool requiresMuteCheck = outputDesc->isActive(outputLatencyMs * 2);  // account for drain

    // increment usage count for this stream on the requested output:
    // NOTE that the usage count is the same for duplicated output and hardware output which is
    // necessary for a correct control of hardware output routing by startOutput() and stopOutput()
    outputDesc->setClientActive(client, true);

    if (client->hasPreferredDevice(true)) {
        if (outputDesc->clientsList(true /*activeOnly*/).size() == 1 &&
                client->isPreferredDeviceForExclusiveUse()) {
            // Preferred device may be exclusive, use only if no other active clients on this output
            devices = DeviceVector(
                    mAvailableOutputDevices.getDeviceFromId(client->preferredDeviceId()));
        } else {
            devices = getNewOutputDevices(outputDesc, false /*fromCache*/);
        }
        if (devices != outputDesc->devices()) {
            checkStrategyRoute(clientStrategy, outputDesc->mIoHandle);
        }
    }

    if (followsSameRouting(clientAttr, attributes_initializer(AUDIO_USAGE_MEDIA))) {
        selectOutputForMusicEffects();
    }

    if (outputDesc->getActivityCount(clientVolSrc) == 1 || !devices.isEmpty()) {
        // starting an output being rerouted?
        if (devices.isEmpty()) {
            devices = getNewOutputDevices(outputDesc, false /*fromCache*/);
        }
        bool shouldWait =
            (followsSameRouting(clientAttr, attributes_initializer(AUDIO_USAGE_ALARM)) ||
             followsSameRouting(clientAttr, attributes_initializer(AUDIO_USAGE_NOTIFICATION)) ||
             (beaconMuteLatency > 0));
        uint32_t waitMs = beaconMuteLatency;
        for (size_t i = 0; i < mOutputs.size(); i++) {
            sp<SwAudioOutputDescriptor> desc = mOutputs.valueAt(i);
            if (desc != outputDesc) {
                // An output has a shared device if
                // - managed by the same hw module
                // - supports the currently selected device
                const bool sharedDevice = outputDesc->sharesHwModuleWith(desc)
                    && (!desc->filterSupportedDevices(devices).isEmpty());

                // force a device change if any other output is:
                // - managed by the same hw module
                // - supports currently selected device
                // - has a current device selection that differs from selected device.
                // - has an active audio patch
                // In this case, the audio HAL must receive the new device selection so that it can
                // change the device currently selected by the other output.
                if (sharedDevice &&
                        desc->devices() != devices &&
                        desc->getPatchHandle() != AUDIO_PATCH_HANDLE_NONE) {
                    force = true;
                }
                // wait for audio on other active outputs to be presented when starting
                // a notification so that audio focus effect can propagate, or that a mute/unmute
                // event occurred for beacon
                const uint32_t latencyMs = desc->latency();
                const bool isActive = desc->isActive(latencyMs * 2);  // account for drain

                if (shouldWait && isActive && (waitMs < latencyMs)) {
                    waitMs = latencyMs;
                }

                // Require mute check if another output is on a shared device
                // and currently active to have proper drain and avoid pops.
                // Note restoring AudioTracks onto this output needs to invoke
                // a volume ramp if there is no mute.
                requiresMuteCheck |= sharedDevice && isActive;
            }
        }
        const uint32_t muteWaitMs =
                setOutputDevices(outputDesc, devices, force, 0, NULL, requiresMuteCheck);

        // apply volume rules for current stream and device if necessary
        auto &curves = getVolumeCurves(client->attributes());
        checkAndSetVolume(curves, client->volumeSource(),
                          getVolumeCurves(stream).getVolumeIndex(outputDesc->devices().types()),
                          outputDesc,
                          outputDesc->devices().types());

        // update the outputs if starting an output with a stream that can affect notification
        // routing
        handleNotificationRoutingForStream(stream);

        // force reevaluating accessibility routing when ringtone or alarm starts
        if (followsSameRouting(clientAttr, attributes_initializer(AUDIO_USAGE_ALARM))) {
            mpClientInterface->invalidateStream(AUDIO_STREAM_ACCESSIBILITY);
        }
        if (waitMs > muteWaitMs) {
            *delayMs = waitMs - muteWaitMs;
        }

    }

    if (stream == AUDIO_STREAM_ENFORCED_AUDIBLE &&
            mEngine->getForceUse(AUDIO_POLICY_FORCE_FOR_SYSTEM) == AUDIO_POLICY_FORCE_SYSTEM_ENFORCED) {
        setStrategyMute(streamToStrategy(AUDIO_STREAM_ALARM), true, outputDesc);
    }

    // Automatically enable the remote submix input when output is started on a re routing mix
    // of type MIX_TYPE_RECORDERS
    if (isSingleDeviceType(devices.types(), &audio_is_remote_submix_device) &&
        policyMix != NULL &&
        policyMix->mMixType == MIX_TYPE_RECORDERS) {
        setDeviceConnectionStateInt(AUDIO_DEVICE_IN_REMOTE_SUBMIX,
                                    AUDIO_POLICY_DEVICE_STATE_AVAILABLE,
                                    address,
                                    "remote-submix",
                                    AUDIO_FORMAT_DEFAULT);
    }

    return NO_ERROR;
}

status_t AudioPolicyManagerCustom::checkAndSetVolume(IVolumeCurves &curves,
                                               VolumeSource volumeSource,
                                               int index,
                                               const sp<AudioOutputDescriptor>& outputDesc,
                                               DeviceTypeSet deviceTypes,
                                               int delayMs,
                                               bool force)
{
    // do not change actual stream volume if the stream is muted
    if (outputDesc->isMuted(volumeSource)) {
        ALOGVV("%s: volume source %d muted count %d active=%d", __func__, volumeSource,
               outputDesc->getMuteCount(volumeSource), outputDesc->isActive(volumeSource));
        return NO_ERROR;
    }

    VolumeSource callVolSrc = toVolumeSource(AUDIO_STREAM_VOICE_CALL);
    VolumeSource btScoVolSrc = toVolumeSource(AUDIO_STREAM_BLUETOOTH_SCO);
    bool isVoiceVolSrc = callVolSrc == volumeSource;
    bool isBtScoVolSrc = btScoVolSrc == volumeSource;

    audio_policy_forced_cfg_t forceUseForComm =
            mEngine->getForceUse(AUDIO_POLICY_FORCE_FOR_COMMUNICATION);
    // do not change in call volume if bluetooth is connected and vice versa
    if ((callVolSrc != btScoVolSrc) &&
            ((isVoiceVolSrc && forceUseForComm == AUDIO_POLICY_FORCE_BT_SCO) ||
             (isBtScoVolSrc && forceUseForComm != AUDIO_POLICY_FORCE_BT_SCO))) {
        ALOGV("checkAndSetVolume() cannot set stream %d volume with force use = %d for comm",
             volumeSource, forceUseForComm);
        return INVALID_OPERATION;
    }

    if (deviceTypes.empty()) {
        deviceTypes = outputDesc->devices().types();
    }

    float volumeDb = computeVolume(curves, volumeSource, index, deviceTypes);
    if (outputDesc->isFixedVolume(deviceTypes)||
            // Force VoIP volume to max for bluetooth SCO

            ((isVoiceVolSrc || isBtScoVolSrc) &&
                    isSingleDeviceType(deviceTypes, audio_is_bluetooth_out_sco_device))) {
        volumeDb = 0.0f;
    }

    outputDesc->setVolume(volumeDb, volumeSource, curves.getStreamTypes(), deviceTypes, delayMs, force);

      if (isVoiceVolSrc || isBtScoVolSrc) {
        float voiceVolume;
        // Force voice volume to max or mute for Bluetooth SCO as other attenuations are managed by the headset
        if (isVoiceVolSrc) {
            voiceVolume = (float)index/(float)curves.getVolumeIndexMax();
        } else {
            voiceVolume = index == 0 ? 0.0 : 1.0;
        }

        if (voiceVolume != mLastVoiceVolume) {
            mpClientInterface->setVoiceVolume(voiceVolume, delayMs);
            mLastVoiceVolume = voiceVolume;
        }
    }
    return NO_ERROR;
}

bool static tryForDirectPCM(audio_output_flags_t flags)
{
    bool trackDirectPCM = false;  // Output request for track created by other apps

    if (flags == AUDIO_OUTPUT_FLAG_NONE) {
        if (AudioPolicyManagerCustom::mApmConfigs != NULL)
            trackDirectPCM = AudioPolicyManagerCustom::mApmConfigs->isAudioTrackOffloadEnabled();
    }
    return trackDirectPCM;
}

status_t AudioPolicyManagerCustom::getOutputForAttr(const audio_attributes_t *attr,
                                                    audio_io_handle_t *output,
                                                    audio_session_t session,
                                                    audio_stream_type_t *stream,
                                                    uid_t uid,
                                                    const audio_config_t *config,
                                                    audio_output_flags_t *flags,
                                                    audio_port_handle_t *selectedDeviceId,
                                                    audio_port_handle_t *portId,
                                                    std::vector<audio_io_handle_t> *secondaryOutputs,
                                                    output_type_t *outputType)
{
    audio_offload_info_t tOffloadInfo = AUDIO_INFO_INITIALIZER;
    audio_config_t tConfig;

    uint32_t bitWidth = (audio_bytes_per_sample(config->format) * 8);

    memcpy(&tConfig, config, sizeof(audio_config_t));
    if ((*flags == AUDIO_OUTPUT_FLAG_DIRECT || tryForDirectPCM(*flags)) &&
        (!memcmp(&config->offload_info, &tOffloadInfo, sizeof(audio_offload_info_t)))) {
        tConfig.offload_info.sample_rate  = config->sample_rate;
        tConfig.offload_info.channel_mask = config->channel_mask;
        tConfig.offload_info.format = config->format;
        tConfig.offload_info.stream_type = *stream;
        tConfig.offload_info.bit_width = bitWidth;
        if (attr != NULL) {
            ALOGV("found attribute .. setting usage %d ", attr->usage);
            tConfig.offload_info.usage = attr->usage;
        } else {
            ALOGI("%s:: attribute is NULL .. no usage set", __func__);
        }
    }

    return AudioPolicyManager::getOutputForAttr(attr, output, session, stream,
                                                (uid_t)uid, &tConfig,
                                                flags,
                                                (audio_port_handle_t*)selectedDeviceId,
                                                portId,
                                                secondaryOutputs,
                                                outputType);
}

audio_io_handle_t AudioPolicyManagerCustom::getOutputForDevices(
                const DeviceVector &devices,
                audio_session_t session,
                audio_stream_type_t stream,
                const audio_config_t *config,
                audio_output_flags_t *flags,
                bool forceMutingHaptic)
{
    audio_io_handle_t output = AUDIO_IO_HANDLE_NONE;
    status_t status;

    // Discard haptic channel mask when forcing muting haptic channels.
    audio_channel_mask_t channelMask = forceMutingHaptic
            ? (config->channel_mask & ~AUDIO_CHANNEL_HAPTIC_ALL) : config->channel_mask;

    if (stream < AUDIO_STREAM_MIN || stream >= AUDIO_STREAM_CNT) {
        ALOGE("%s: invalid stream %d", __func__, stream);
        return AUDIO_IO_HANDLE_NONE;
    }

    if (((*flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) != 0) &&
            (stream != AUDIO_STREAM_MUSIC)) {
        // compress should not be used for non-music streams
        ALOGE("Offloading only allowed with music stream");
        return 0;
    }

     /*
      * Check for VOIP Flag override for voice streams using linear pcm,
      * but not when intended for uplink device(i.e. Telephony Tx)
      */
     if (stream == AUDIO_STREAM_VOICE_CALL &&
        audio_is_linear_pcm(config->format) &&
        !devices.onlyContainsDevicesWithType(AUDIO_DEVICE_OUT_TELEPHONY_TX)) {
        if (mApmConfigs->isCompressVOIPEnabled()) {
            // let voice stream to go with primary output by default
            // in case direct voip is bypassed
            bool use_primary_out = true;

            if ((channelMask == 1) &&
                    (config->sample_rate == 8000 || config->sample_rate == 16000 ||
                    config->sample_rate == 32000 || config->sample_rate == 48000)) {
                // Allow Voip direct output only if:
                // audio mode is MODE_IN_COMMUNCATION; AND
                // voip output is not opened already; AND
                // requested sample rate matches with that of voip input stream (if opened already)
                int value = 0;
                uint32_t voipOutCount = 1, voipSampleRate = 1;

                String8 valueStr = mpClientInterface->getParameters((audio_io_handle_t)0,
                                                  String8("voip_out_stream_count"));
                AudioParameter result = AudioParameter(valueStr);
                if (result.getInt(String8("voip_out_stream_count"), value) == NO_ERROR) {
                    voipOutCount = value;
                }

                valueStr = mpClientInterface->getParameters((audio_io_handle_t)0,
                                                  String8("voip_sample_rate"));
                result = AudioParameter(valueStr);
                if (result.getInt(String8("voip_sample_rate"), value) == NO_ERROR) {
                    voipSampleRate = value;
                }

                if ((voipOutCount == 0) &&
                    ((voipSampleRate == 0) || (voipSampleRate == config->sample_rate))) {
                    if (mApmConfigs->useVoicePathForPCMVOIP()
                            && (config->format == AUDIO_FORMAT_PCM_16_BIT)) {
                        *flags = (audio_output_flags_t)(AUDIO_OUTPUT_FLAG_VOIP_RX |
                                                     AUDIO_OUTPUT_FLAG_DIRECT);
                        ALOGD("Set VoIP and Direct output flags for PCM format");
                        use_primary_out = false;
                    }
                }
            }

            if (use_primary_out) {
                *flags = (audio_output_flags_t)(AUDIO_OUTPUT_FLAG_FAST|AUDIO_OUTPUT_FLAG_PRIMARY);
            }
        } else if ((config->channel_mask == 1 || config->channel_mask == 3) &&
                   (config->sample_rate == 8000 || config->sample_rate == 16000 ||
                    config->sample_rate == 32000 || config->sample_rate == 48000)) {
            //check if VoIP output is not opened already
            bool voip_pcm_already_in_use = false;
            for (size_t i = 0; i < mOutputs.size(); i++) {
                 sp<SwAudioOutputDescriptor> desc = mOutputs.valueAt(i);
                 if (desc->mFlags == (AUDIO_OUTPUT_FLAG_VOIP_RX | AUDIO_OUTPUT_FLAG_DIRECT)) {
                     //close voip output if currently open by the same client with different device
                     if (desc->mDirectClientSession == session &&
                         desc->devices() != devices) {
                         closeOutput(desc->mIoHandle);
                     } else {
                         voip_pcm_already_in_use = true;
                         ALOGD("VoIP PCM already in use");
                     }
                     break;
                 }
            }

            if (!voip_pcm_already_in_use) {
                *flags = (audio_output_flags_t)(AUDIO_OUTPUT_FLAG_VOIP_RX |
                                               AUDIO_OUTPUT_FLAG_DIRECT);
                ALOGV("Set VoIP and Direct output flags for PCM format");
            }
        }
    } /* voip flag override block end */

    //IF VOIP is going to be started at the same time as when
    //vr is enabled, get VOIP to fallback to low latency
    String8 vr_value;
    String8 value_Str;
    bool is_vr_mode_on = false;
    AudioParameter ret;

    value_Str =  mpClientInterface->getParameters((audio_io_handle_t)0,
                                          String8("vr_audio_mode_on"));
    ret = AudioParameter(value_Str);
    if (ret.get(String8("vr_audio_mode_on"), vr_value) == NO_ERROR) {
        is_vr_mode_on = vr_value.contains("true");
        ALOGI("VR mode is %d, switch to primary output if request is for fast|raw",
            is_vr_mode_on);
    }

    if (is_vr_mode_on) {
         //check the flags being requested for, and clear FAST|RAW
        *flags = (audio_output_flags_t)(*flags &
            (~(AUDIO_OUTPUT_FLAG_FAST|AUDIO_OUTPUT_FLAG_RAW)));

    }

    if (mApmConfigs->isVoiceConcEnabled()) {
        bool prop_play_enabled = false, prop_voip_enabled = false;
        prop_play_enabled = mApmConfigs->isVoicePlayConcDisabled();
        prop_voip_enabled = mApmConfigs->isVoiceVOIPConcDisabled();

        bool isDeepBufferFallBackNeeded =
            ((AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD | AUDIO_OUTPUT_FLAG_DIRECT_PCM) & *flags);
        bool isFastFallBackNeeded =
            ((AUDIO_OUTPUT_FLAG_DEEP_BUFFER | AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD | AUDIO_OUTPUT_FLAG_DIRECT_PCM) & *flags);

        if (prop_play_enabled && mvoice_call_state) {
            //check if voice call is active  / running in background
            if ((AUDIO_MODE_IN_CALL == mEngine->getPhoneState()) ||
                 ((AUDIO_MODE_IN_CALL == mPrevPhoneState)
                    && (AUDIO_MODE_IN_COMMUNICATION == mEngine->getPhoneState())))
            {
                if (AUDIO_OUTPUT_FLAG_VOIP_RX  & *flags) {
                    if (prop_voip_enabled) {
                       ALOGD("voice_conc:getoutput:IN call mode return no o/p for VoIP %x",
                            *flags );
                       return 0;
                    }
                }
                else {
                    if (isFastFallBackNeeded &&
                        (AUDIO_OUTPUT_FLAG_FAST == mFallBackflag)) {
                        ALOGD("voice_conc:IN call mode adding ULL flags .. flags: %x ", *flags );
                        *flags = AUDIO_OUTPUT_FLAG_FAST;
                    } else if (isDeepBufferFallBackNeeded &&
                               (AUDIO_OUTPUT_FLAG_DEEP_BUFFER == mFallBackflag)) {
                        if (AUDIO_STREAM_MUSIC == stream) {
                            *flags = AUDIO_OUTPUT_FLAG_DEEP_BUFFER;
                            ALOGD("voice_conc:IN call mode adding deep-buffer flags %x ", *flags );
                        }
                        else {
                            *flags = AUDIO_OUTPUT_FLAG_FAST;
                            ALOGD("voice_conc:IN call mode adding fast flags %x ", *flags );
                        }
                    }
                }
            }
        } else if (prop_voip_enabled && mvoice_call_state) {
            //check if voice call is active  / running in background
            //some of VoIP apps(like SIP2SIP call) supports resume of VoIP call when call in progress
            //return only ULL ouput
            if ((AUDIO_MODE_IN_CALL == mEngine->getPhoneState()) ||
                 ((AUDIO_MODE_IN_CALL == mPrevPhoneState)
                    && (AUDIO_MODE_IN_COMMUNICATION == mEngine->getPhoneState())))
            {
                if (AUDIO_OUTPUT_FLAG_VOIP_RX  & *flags) {
                        ALOGD("voice_conc:getoutput:IN call mode return no o/p for VoIP %x",
                            *flags );
                   return 0;
                }
            }
        }
    }
    if (mApmConfigs->isRecPlayConcEnabled()) {
        bool prop_rec_play_enabled = mApmConfigs->isRecPlayConcDisabled();
        DeviceVector primaryInputDevices = availablePrimaryModuleInputDevices();
        if ((prop_rec_play_enabled) &&
                ((true == mIsInputRequestOnProgress) ||
                (mInputs.activeInputsCountOnDevices(primaryInputDevices) > 0))) {
            if (AUDIO_MODE_IN_COMMUNICATION == mEngine->getPhoneState()) {
                if (AUDIO_OUTPUT_FLAG_VOIP_RX & *flags) {
                    // allow VoIP using voice path
                    // Do nothing
                } else if ((*flags & AUDIO_OUTPUT_FLAG_FAST) == 0) {
                    ALOGD("voice_conc:MODE_IN_COMM is setforcing deep buffer output for non ULL... flags: %x", *flags);
                    // use deep buffer path for all non ULL outputs
                    *flags = AUDIO_OUTPUT_FLAG_DEEP_BUFFER;
                }
            } else if ((*flags & AUDIO_OUTPUT_FLAG_FAST) == 0) {
                ALOGD("voice_conc:Record mode is on forcing deep buffer output for non ULL... flags: %x ", *flags);
                // use deep buffer path for all non ULL outputs
                *flags = AUDIO_OUTPUT_FLAG_DEEP_BUFFER;
            }
        }
        if (prop_rec_play_enabled &&
                (stream == AUDIO_STREAM_ENFORCED_AUDIBLE)) {
               ALOGD("Record conc is on forcing ULL output for ENFORCED_AUDIBLE");
               *flags = AUDIO_OUTPUT_FLAG_FAST;
        }
    }

    /*
    * WFD audio routes back to target speaker when starting a ringtone playback.
    * This is because primary output is reused for ringtone, so output device is
    * updated based on SONIFICATION strategy for both ringtone and music playback.
    * The same issue is not seen on remoted_submix HAL based WFD audio because
    * primary output is not reused and a new output is created for ringtone playback.
    * Issue is fixed by updating output flag to AUDIO_OUTPUT_FLAG_FAST when there is
    * a non-music stream playback on WFD, so primary output is not reused for ringtone.
    */
    if (mApmConfigs->isAFEProxyEnabled()) {
        DeviceTypeSet availableOutputDeviceTypes = mAvailableOutputDevices.types();
        if ((deviceTypesToBitMask(availableOutputDeviceTypes) & AUDIO_DEVICE_OUT_PROXY)
              && (stream != AUDIO_STREAM_MUSIC)) {
            ALOGD("WFD audio: use OUTPUT_FLAG_FAST for non music stream. flags:%x", *flags );
            //For voip paths
            if (*flags & AUDIO_OUTPUT_FLAG_DIRECT)
                *flags = AUDIO_OUTPUT_FLAG_DIRECT;
            else //route every thing else to ULL path
                *flags = AUDIO_OUTPUT_FLAG_FAST;
        }
    }

    // open a direct output if required by specified parameters
    // force direct flag if offload flag is set: offloading implies a direct output stream
    // and all common behaviors are driven by checking only the direct flag
    // this should normally be set appropriately in the policy configuration file
    if ((*flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) != 0) {
        *flags = (audio_output_flags_t)(*flags | AUDIO_OUTPUT_FLAG_DIRECT);
    }
    if ((*flags & AUDIO_OUTPUT_FLAG_HW_AV_SYNC) != 0) {
        *flags = (audio_output_flags_t)(*flags | AUDIO_OUTPUT_FLAG_DIRECT);
    }

    // Do internal direct magic here
    bool offload_disabled = mApmConfigs->isAudioOffloadDisabled();
    if ((*flags == AUDIO_OUTPUT_FLAG_NONE) &&
        (stream == AUDIO_STREAM_MUSIC) &&
        ( !offload_disabled) &&
        ((config->offload_info.usage == AUDIO_USAGE_MEDIA) ||
        (config->offload_info.usage == AUDIO_USAGE_GAME))) {
        audio_output_flags_t old_flags = *flags;
        *flags = (audio_output_flags_t)(AUDIO_OUTPUT_FLAG_DIRECT);
        ALOGD("Force Direct Flag .. old flags(0x%x)", old_flags);
    } else if (*flags == AUDIO_OUTPUT_FLAG_DIRECT &&
                (offload_disabled || stream != AUDIO_STREAM_MUSIC)) {
        ALOGD("Offloading is disabled or Stream is not music --> Force Remove Direct Flag");
        *flags = (audio_output_flags_t)(AUDIO_OUTPUT_FLAG_NONE);
    }

    // check if direct output for pcm/track offload or compress offload already exist
    bool direct_pcm_already_in_use = false;
    bool compress_offload_already_in_use = false;
    if (*flags & AUDIO_OUTPUT_FLAG_DIRECT) {
        for (size_t i = 0; i < mOutputs.size(); i++) {
            sp<SwAudioOutputDescriptor> desc = mOutputs.valueAt(i);
            if (desc->mFlags == AUDIO_OUTPUT_FLAG_DIRECT) {
                direct_pcm_already_in_use = true;
                ALOGD("Direct PCM already in use");
            }
            if (desc->mFlags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) {
                compress_offload_already_in_use = true;
                ALOGD("Compress Offload already in use");
            }
        }
        // prevent direct pcm for non-music stream blindly if direct pcm already in use
        // for other music stream concurrency is handled after checking direct ouput usage
        // and checking client
        if (direct_pcm_already_in_use == true && stream != AUDIO_STREAM_MUSIC &&
            !(*flags & AUDIO_OUTPUT_FLAG_VOIP_RX)) {
            ALOGD("disabling offload for non music stream as direct pcm is already in use");
            *flags = (audio_output_flags_t)(AUDIO_OUTPUT_FLAG_NONE);
        }
    }

    bool forced_deep = false;
    // only allow deep buffering for music stream type
    if (stream != AUDIO_STREAM_MUSIC) {
        *flags = (audio_output_flags_t)(*flags & ~AUDIO_OUTPUT_FLAG_DEEP_BUFFER);
    } else if (/* stream == AUDIO_STREAM_MUSIC && */
            (*flags == AUDIO_OUTPUT_FLAG_NONE || *flags == AUDIO_OUTPUT_FLAG_DIRECT ||
            (*flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD)) &&
            mApmConfigs->isAudioDeepbufferMediaEnabled() && !isInCall()) {
            forced_deep = true;
    }

    if (stream == AUDIO_STREAM_TTS) {
        *flags = AUDIO_OUTPUT_FLAG_TTS;
    } else if (devices.onlyContainsDevicesWithType(AUDIO_DEVICE_OUT_TELEPHONY_TX) &&
        stream == AUDIO_STREAM_MUSIC &&
        audio_is_linear_pcm(config->format) &&
        isInCall()) {
        *flags = (audio_output_flags_t)AUDIO_OUTPUT_FLAG_INCALL_MUSIC;
    }

    sp<IOProfile> profile;

    // skip direct output selection if the request can obviously be attached to a mixed output
    // and not explicitly requested
    if (((*flags & AUDIO_OUTPUT_FLAG_DIRECT) == 0) &&
            audio_is_linear_pcm(config->format) && config->sample_rate <= SAMPLE_RATE_HZ_MAX &&
            audio_channel_count_from_out_mask(channelMask) <= 2) {
        goto non_direct_output;
    }

    // Do not allow offloading if one non offloadable effect is enabled or MasterMono is enabled.
    // This prevents creating an offloaded track and tearing it down immediately after start
    // when audioflinger detects there is an active non offloadable effect.
    // FIXME: We should check the audio session here but we do not have it in this context.
    // This may prevent offloading in rare situations where effects are left active by apps
    // in the background.
    //
    // Supplementary annotation:
    // For sake of track offload introduced, we need a rollback for both compress offload
    // and track offload use cases.
    if ((*flags & (AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD|AUDIO_OUTPUT_FLAG_DIRECT)) &&
                (mEffects.isNonOffloadableEffectEnabled() || mMasterMono)) {
        ALOGD("non offloadable effect is enabled, try with non direct output");
        goto non_direct_output;
    }

    profile = getProfileForOutput(devices,
                                        config->sample_rate,
                                        config->format,
                                        channelMask,
                                        (audio_output_flags_t)*flags,
                                        true /* directOnly */);

    if (profile != 0) {

        if (!(*flags & AUDIO_OUTPUT_FLAG_DIRECT) &&
             (profile->getFlags() & AUDIO_OUTPUT_FLAG_DIRECT)) {
            ALOGI("got Direct without requesting ... reject ");
            profile = NULL;
            goto non_direct_output;
        }

        sp<SwAudioOutputDescriptor> outputDesc = NULL;
        // if multiple concurrent offload decode is supported
        // do no check for reuse and also don't close previous output if its offload
        // previous output will be closed during track destruction
        if (!(mApmConfigs->isAudioMultipleOffloadEnable() &&
                ((*flags & AUDIO_OUTPUT_FLAG_DIRECT) != 0) &&
                ((*flags & AUDIO_OUTPUT_FLAG_MMAP_NOIRQ) == 0))) {
            for (size_t i = 0; i < mOutputs.size(); i++) {
                sp<SwAudioOutputDescriptor> desc = mOutputs.valueAt(i);
                if (!desc->isDuplicated() && (profile == desc->mProfile)) {
                    outputDesc = desc;
                    // reuse direct output if currently open by the same client
                    // and configured with same parameters
                    if ((config->sample_rate == desc->getSamplingRate()) &&
                        (config->format == desc->getFormat()) &&
                        (channelMask == desc->getChannelMask()) &&
                        (session == desc->mDirectClientSession)) {
                        desc->mDirectOpenCount++;
                        ALOGV("getOutputForDevice() reusing direct output %d for session %d",
                            mOutputs.keyAt(i), session);
                        return mOutputs.keyAt(i);
                    }
                }
            }
            if (outputDesc != NULL) {
                if ((((*flags == AUDIO_OUTPUT_FLAG_DIRECT) && direct_pcm_already_in_use) ||
                    ((*flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) &&
                     compress_offload_already_in_use)) &&
                     session != outputDesc->mDirectClientSession) {
                     ALOGV("getOutput() do not reuse direct pcm output because current client (%d) "
                           "is not the same as requesting client (%d) for different output conf",
                     outputDesc->mDirectClientSession, session);
                     goto non_direct_output;
                }
                closeOutput(outputDesc->mIoHandle);
            }
        }
        if (!profile->canOpenNewIo()) {
            goto non_direct_output;
        }

        outputDesc =
                new SwAudioOutputDescriptor(profile, mpClientInterface);
        DeviceVector outputDevices = mAvailableOutputDevices.getDevicesFromTypes(devices.types());
        String8 address = outputDevices.size() > 0 ? String8(outputDevices.itemAt(0)->address().data())
                : String8("");
        status = outputDesc->open(config, devices, stream, *flags, &output);

        // only accept an output with the requested parameters
        if (status != NO_ERROR ||
            (config->sample_rate != 0 && config->sample_rate != outputDesc->getSamplingRate()) ||
            (config->format != AUDIO_FORMAT_DEFAULT && config->format != outputDesc->getFormat()) ||
            (channelMask != 0 && channelMask != outputDesc->getChannelMask())) {
            ALOGV("getOutputForDevice() failed opening direct output: output %d sample rate %d %d,"
                    "format %d %d, channel mask %04x %04x", output, config->sample_rate,
                    outputDesc->getSamplingRate(), config->format, outputDesc->getFormat(),
                    channelMask, outputDesc->getChannelMask());
            //Only close o/p descriptor if successfully opened
            if (status == NO_ERROR) {
                outputDesc->close();
            }
            // fall back to mixer output if possible when the direct output could not be open
            if (audio_is_linear_pcm(config->format) && config->sample_rate <= SAMPLE_RATE_HZ_MAX) {
                goto non_direct_output;
            }
            return AUDIO_IO_HANDLE_NONE;
        }
        outputDesc->mDirectOpenCount = 1;
        outputDesc->mDirectClientSession = session;

        addOutput(output, outputDesc);
        mPreviousOutputs = mOutputs;
        ALOGV("getOutputForDevice() returns new direct output %d", output);
        mpClientInterface->onAudioPortListUpdate();
        return output;
    }

non_direct_output:

    // A request for HW A/V sync cannot fallback to a mixed output because time
    // stamps are embedded in audio data
    if ((*flags & (AUDIO_OUTPUT_FLAG_HW_AV_SYNC | AUDIO_OUTPUT_FLAG_MMAP_NOIRQ)) != 0) {
        return AUDIO_IO_HANDLE_NONE;
    }

    // ignoring channel mask due to downmix capability in mixer

    // open a non direct output

    // for non direct outputs, only PCM is supported
    if (audio_is_linear_pcm(config->format)) {
        // get which output is suitable for the specified stream. The actual
        // routing change will happen when startOutput() will be called
        SortedVector<audio_io_handle_t> outputs = getOutputsForDevices(devices, mOutputs);

        // at this stage we should ignore the DIRECT flag as no direct output could be found earlier
        *flags = (audio_output_flags_t)(*flags & ~AUDIO_OUTPUT_FLAG_DIRECT);

        if (forced_deep) {
            *flags = (audio_output_flags_t)(AUDIO_OUTPUT_FLAG_DEEP_BUFFER);
            ALOGI("setting force DEEP buffer now ");
        } else if (*flags == AUDIO_OUTPUT_FLAG_NONE) {
            // no deep buffer playback is requested hence fallback to primary
            *flags = (audio_output_flags_t)(AUDIO_OUTPUT_FLAG_PRIMARY);
            ALOGI("FLAG None hence request for a primary output");
        }

        output = selectOutput(outputs, *flags, config->format, channelMask);
    }

    ALOGW_IF((output == 0), "getOutputForDevice() could not find output for stream %d, "
            "sampling rate %d, format %#x, channels %#x, flags %#x",
            stream, config->sample_rate, config->format, channelMask, *flags);

    ALOGV("getOutputForDevice() returns output %d", output);

    return output;
}

status_t AudioPolicyManagerCustom::getInputForAttr(const audio_attributes_t *attr,
                                         audio_io_handle_t *input,
                                         audio_unique_id_t riid,
                                         audio_session_t session,
                                         uid_t uid,
                                         const audio_config_base_t *config,
                                         audio_input_flags_t flags,
                                         audio_port_handle_t *selectedDeviceId,
                                         input_type_t *inputType,
                                         audio_port_handle_t *portId)
{
    audio_source_t inputSource;
    inputSource = attr->source;

    if (mApmConfigs->isVoiceConcEnabled()) {
        bool prop_rec_enabled = false, prop_voip_enabled = false;
        prop_rec_enabled = mApmConfigs->isVoiceRecConcDisabled();
        prop_voip_enabled = mApmConfigs->isVoiceVOIPConcDisabled();

        if (prop_rec_enabled && mvoice_call_state) {
             //check if voice call is active  / running in background
             //some of VoIP apps(like SIP2SIP call) supports resume of VoIP call when call in progress
             //Need to block input request
            if ((AUDIO_MODE_IN_CALL == mEngine->getPhoneState()) ||
               ((AUDIO_MODE_IN_CALL == mPrevPhoneState) &&
                 (AUDIO_MODE_IN_COMMUNICATION == mEngine->getPhoneState())))
            {
                switch(inputSource) {
                    case AUDIO_SOURCE_VOICE_UPLINK:
                    case AUDIO_SOURCE_VOICE_DOWNLINK:
                    case AUDIO_SOURCE_VOICE_CALL:
                        ALOGD("voice_conc:Creating input during incall mode for inputSource: %d",
                            inputSource);
                    break;

                    case AUDIO_SOURCE_VOICE_COMMUNICATION:
                        if (prop_voip_enabled) {
                           ALOGD("voice_conc:BLOCK VoIP requst incall mode for inputSource: %d",
                            inputSource);
                           return NO_INIT;
                        }
                    break;
                    default:
                        ALOGD("voice_conc:BLOCK VoIP requst incall mode for inputSource: %d",
                            inputSource);
                    return NO_INIT;
                }
            }
        }//check for VoIP flag
        else if (prop_voip_enabled && mvoice_call_state) {
             //check if voice call is active  / running in background
             //some of VoIP apps(like SIP2SIP call) supports resume of VoIP call when call in progress
             //Need to block input request
            if ((AUDIO_MODE_IN_CALL == mEngine->getPhoneState()) ||
               ((AUDIO_MODE_IN_CALL == mPrevPhoneState) &&
                 (AUDIO_MODE_IN_COMMUNICATION == mEngine->getPhoneState())))
            {
                if (inputSource == AUDIO_SOURCE_VOICE_COMMUNICATION) {
                    ALOGD("BLOCKING VoIP request during incall mode for inputSource: %d ",inputSource);
                    return NO_INIT;
                }
            }
        }
    }

    // This workaround prevents Sound Trigger capture streams from
    // starting on USB headset. Sound Trigger is not supported on
    // USB headset, so the capture streams should not select USB
    // headset device. Temporarily remove USB headset devices from
    // the available devices list while selecting device.
    bool isSoundTrigger = inputSource == AUDIO_SOURCE_HOTWORD &&
        mSoundTriggerSessions.indexOfKey(session) >= 0;
    DeviceVector USBDevices = mAvailableInputDevices.getDevicesFromType(
                AUDIO_DEVICE_IN_USB_HEADSET);

    if (isSoundTrigger) {
        for (size_t i = 0; i < USBDevices.size(); i++) {
            mAvailableInputDevices.remove(USBDevices[i]);
        }
    }

    status_t status = AudioPolicyManager::getInputForAttr(attr,
                                                          input,
                                                          riid,
                                                          session,
                                                          uid,
                                                          config,
                                                          flags,
                                                          selectedDeviceId,
                                                          inputType,
                                                          portId);

    if (isSoundTrigger) {
        for (size_t i = 0; i < USBDevices.size(); i++) {
            mAvailableInputDevices.add(USBDevices[i]);
        }
    }

    return status;
}

uint32_t AudioPolicyManagerCustom::activeNonSoundTriggerInputsCountOnDevices(audio_devices_t devices) const
{
    uint32_t count = 0;
    for (size_t i = 0; i < mInputs.size(); i++) {
        const sp<AudioInputDescriptor>  inputDescriptor = mInputs.valueAt(i);
        if (inputDescriptor->isActive() && !inputDescriptor->isSoundTrigger() &&
                ((devices == AUDIO_DEVICE_IN_DEFAULT) ||
                 ((inputDescriptor->getDeviceType() & devices & ~AUDIO_DEVICE_BIT_IN) != 0))) {
            count++;
        }
    }
    return count;
}

status_t AudioPolicyManagerCustom::startInput(audio_port_handle_t portId)
{
    ALOGV("%s portId %d", __FUNCTION__, portId);

    sp<AudioInputDescriptor> inputDesc = mInputs.getInputForClient(portId);
    if (inputDesc == 0) {
        ALOGW("%s no input for client %d", __FUNCTION__, portId);
        return BAD_VALUE;
    }

    audio_io_handle_t input = inputDesc->mIoHandle;
    sp<RecordClientDescriptor> client = inputDesc->getClient(portId);
    if (client == NULL) {
        ALOGW("%s invalid client desc for %d", __FUNCTION__, portId);
        return BAD_VALUE;
    }

    if (client->active()) {
        ALOGW("%s input %d client %d already started", __FUNCTION__, input, client->portId());
        return INVALID_OPERATION;
    }

    audio_session_t session = client->session();

    ALOGV("%s input:%d, session:%d)", __FUNCTION__, input, session);


// FIXME: disable concurrent capture until UI is ready
#if 0
    if (!isConcurentCaptureAllowed(inputDesc, audioSession)) {
        ALOGW("startInput(%d) failed: other input already started", input);
        return INVALID_OPERATION;
    }

    if (isInCall()) {
        *concurrency |= API_INPUT_CONCURRENCY_CALL;
    }

    if (mInputs.activeInputsCountOnDevices() != 0) {
        *concurrency |= API_INPUT_CONCURRENCY_CAPTURE;
    }
#endif

    if (mApmConfigs->isRecPlayConcEnabled()) {
        mIsInputRequestOnProgress = true;
        DeviceVector primaryInputDevices = availablePrimaryModuleInputDevices();
        if (mApmConfigs->isRecPlayConcDisabled() && (mInputs.activeInputsCountOnDevices(primaryInputDevices) == 0)) {
            // send update to HAL on record playback concurrency
            AudioParameter param = AudioParameter();
            param.add(String8("rec_play_conc_on"), String8("true"));
            ALOGD("startInput() setParameters rec_play_conc is setting to ON ");
            mpClientInterface->setParameters(0, param.toString());

            // Call invalidate to reset all opened non ULL audio tracks
            // Move tracks associated to this strategy from previous output to new output
            for (int i = AUDIO_STREAM_SYSTEM; i < AUDIO_STREAM_FOR_POLICY_CNT; i++) {
                // Do not call invalidate for ENFORCED_AUDIBLE (otherwise pops are seen for camcorder)
                if (i != AUDIO_STREAM_ENFORCED_AUDIBLE) {
                   ALOGD("Invalidate on releaseInput for stream :: %d ", i);
                   //FIXME see fixme on name change
                   mpClientInterface->invalidateStream((audio_stream_type_t)i);
                }
            }
            // close compress tracks
            for (size_t i = 0; i < mOutputs.size(); i++) {
                sp<SwAudioOutputDescriptor> outputDesc = mOutputs.valueAt(i);
                if ((outputDesc == NULL) || (outputDesc->mProfile == NULL)) {
                   ALOGD("ouput desc / profile is NULL");
                   continue;
                }
                if (outputDesc->mProfile->getFlags()
                                & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) {
                    // close compress  sessions
                    ALOGD("calling closeOutput on record conc for COMPRESS output");
                    closeOutput(mOutputs.keyAt(i));
                }
            }
        }
    }
    status_t status = inputDesc->start();
    if (status != NO_ERROR) {
        return status;
    }

    // increment activity count before calling getNewInputDevice() below as only active sessions
    // are considered for device selection
    inputDesc->setClientActive(client, true);

    // indicate active capture to sound trigger service if starting capture from a mic on
    // primary HW module
    sp<DeviceDescriptor> device = getNewInputDevice(inputDesc);
    setInputDevice(input, device, true /* force */);

    if (inputDesc->activeCount()  == 1) {
        sp<AudioPolicyMix> policyMix = inputDesc->mPolicyMix.promote();
        // if input maps to a dynamic policy with an activity listener, notify of state change
        if ((policyMix != NULL)
                && ((policyMix->mCbFlags & AudioMix::kCbFlagNotifyActivity) != 0)) {
            mpClientInterface->onDynamicPolicyMixStateUpdate(policyMix->mDeviceAddress,
                    MIX_STATE_MIXING);
        }

        DeviceVector primaryInputDevices = availablePrimaryModuleInputDevices();
        if ((primaryInputDevices.contains(device) && (device->type() & ~AUDIO_DEVICE_BIT_IN)) != 0) {
            if (mApmConfigs->isVAConcEnabled()) {
                if (activeNonSoundTriggerInputsCountOnDevices(deviceTypesToBitMask(primaryInputDevices.types())) == 1)
                    mpClientInterface->setSoundTriggerCaptureState(true);
            } else if (mInputs.activeInputsCountOnDevices(primaryInputDevices) == 1)
                mpClientInterface->setSoundTriggerCaptureState(true);
        }

        // automatically enable the remote submix output when input is started if not
        // used by a policy mix of type MIX_TYPE_RECORDERS
        // For remote submix (a virtual device), we open only one input per capture request.
        if (audio_is_remote_submix_device(inputDesc->getDeviceType())) {
            String8 address = String8("");
            if (policyMix == NULL) {
                address = String8("0");
            } else if (policyMix->mMixType == MIX_TYPE_PLAYERS) {
                address = policyMix->mDeviceAddress;
            }
            if (address != "") {
                setDeviceConnectionStateInt(AUDIO_DEVICE_OUT_REMOTE_SUBMIX,
                        AUDIO_POLICY_DEVICE_STATE_AVAILABLE,
                        address, "remote-submix", AUDIO_FORMAT_DEFAULT);
            }
        }
    }

    ALOGV("%s input %d source = %d exit", __FUNCTION__, input, client->source());

    if (mApmConfigs->isRecPlayConcEnabled())
        mIsInputRequestOnProgress = false;
    return NO_ERROR;
}

status_t AudioPolicyManagerCustom::stopInput(audio_port_handle_t portId)
{
    status_t status;
    status = AudioPolicyManager::stopInput(portId);
    sp<AudioInputDescriptor> inputDesc = mInputs.getInputForClient(portId);
    if (inputDesc == 0) {
        ALOGW("stopInput() no input for client %d", portId);
        return BAD_VALUE;
    }
    sp<RecordClientDescriptor> client = inputDesc->getClient(portId);
    audio_io_handle_t input = inputDesc->mIoHandle;

    ALOGV("stopInput() input %d", input);
    DeviceVector primaryInputDevices = availablePrimaryModuleInputDevices();
    if (mApmConfigs->isVAConcEnabled()) {
        sp<AudioInputDescriptor> inputDesc = mInputs.getInputForClient(portId);
        if ((primaryInputDevices.contains(inputDesc->getDevice()) &&
                activeNonSoundTriggerInputsCountOnDevices(deviceTypesToBitMask(primaryInputDevices.types()))) == 0) {
                mpClientInterface->setSoundTriggerCaptureState(false);
        }
    }
    if (mApmConfigs->isRecPlayConcEnabled()) {
        if (mApmConfigs->isRecPlayConcDisabled() &&
                (mInputs.activeInputsCountOnDevices(primaryInputDevices) == 0)) {
            //send update to HAL on record playback concurrency
            AudioParameter param = AudioParameter();
            param.add(String8("rec_play_conc_on"), String8("false"));
            ALOGD("stopInput() setParameters rec_play_conc is setting to OFF ");
            mpClientInterface->setParameters(0, param.toString());

            //call invalidate tracks so that any open streams can fall back to deep buffer/compress path from ULL
            for (int i = AUDIO_STREAM_SYSTEM; i < (int)AUDIO_STREAM_CNT; i++) {
                //Do not call invalidate for ENFORCED_AUDIBLE (otherwise pops are seen for camcorder stop tone)
                if ((i != AUDIO_STREAM_ENFORCED_AUDIBLE) && (i != AUDIO_STREAM_PATCH)) {
                   ALOGD(" Invalidate on stopInput for stream :: %d ", i);
                   //FIXME see fixme on name change
                   mpClientInterface->invalidateStream((audio_stream_type_t)i);
                }
            }
        }
    }
    return status;
}

AudioPolicyManagerCustom::AudioPolicyManagerCustom(AudioPolicyClientInterface *clientInterface)
    : AudioPolicyManager(clientInterface),
      mFallBackflag(AUDIO_OUTPUT_FLAG_NONE),
      mHdmiAudioDisabled(false),
      mHdmiAudioEvent(false),
      mPrevPhoneState(0),
      mIsInputRequestOnProgress(false)
{
    if (mApmConfigs->useXMLAudioPolicyConf())
        ALOGD("USE_XML_AUDIO_POLICY_CONF is TRUE");
    else
        ALOGD("USE_XML_AUDIO_POLICY_CONF is FALSE");

    if (mApmConfigs->isVoiceConcEnabled())
        mFallBackflag = getFallBackPath();
}

status_t AudioPolicyManagerCustom::dump(int fd)
{
    AudioPolicyManager::dump(fd);
    String8 result;
    mApmConfigs->dump(&result);
    write(fd, result.string(), result.size());
    return NO_ERROR;
}

}
