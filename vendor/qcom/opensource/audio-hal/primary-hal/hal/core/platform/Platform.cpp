/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AHAL_Platform_QTI"

#include <Utils.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <cutils/str_parms.h>
#include <hardware/audio.h>
#include <qti-audio-core/AudioUsecase.h>
#include <qti-audio-core/MicrophoneInfoParser.h>
#include <qti-audio-core/Platform.h>
#include <qti-audio-core/PlatformUtils.h>
#include <qti-audio/PlatformConverter.h>
#include <qti-audio-core/Utils.h>
#include <aidl/qti/audio/core/VString.h>
#include <cutils/properties.h>
#include <dlfcn.h>
#include <extensions/AudioExtension.h>
#include <unistd.h>
#if defined(SEC_AUDIO_OFFLOAD_COMPRESSED_OPUS) && defined(SEC_AUDIO_OFFLOAD_SOUNDSPEED)
#include <media/stagefright/foundation/MediaDefs.h>
#endif

#define LC3_SWB_CODEC_CONFIG_INDEX 4
#define LC3_BROADCAST_TRANSIT_MODE 1
#define LC3_HFP_TRANSIT_MODE 3

using ::aidl::android::media::audio::common::AudioChannelLayout;
using ::aidl::android::media::audio::common::AudioDevice;
using ::aidl::android::media::audio::common::AudioDeviceAddress;
using ::aidl::android::media::audio::common::AudioDeviceDescription;
using ::aidl::android::media::audio::common::AudioDeviceType;
using ::aidl::android::media::audio::common::AudioFormatDescription;
using ::aidl::android::media::audio::common::AudioFormatType;
using ::aidl::android::media::audio::common::AudioIoFlags;
using ::aidl::android::media::audio::common::AudioOutputFlags;
using ::aidl::android::media::audio::common::AudioPort;
using ::aidl::android::media::audio::common::AudioPortConfig;
using ::aidl::android::media::audio::common::AudioPortDeviceExt;
using ::aidl::android::media::audio::common::AudioPortExt;
using ::aidl::android::media::audio::common::AudioProfile;
using ::aidl::android::media::audio::common::PcmType;
#ifdef SEC_AUDIO_COMMON
using ::aidl::android::media::audio::common::AudioSource;
using ::aidl::android::hardware::audio::common::getPcmSampleSizeInBytes;
#endif

using ::aidl::android::hardware::audio::common::getChannelCount;
using ::aidl::android::hardware::audio::common::getFrameSizeInBytes;
using ::aidl::android::hardware::audio::common::isBitPositionFlagSet;
using ::aidl::android::hardware::audio::core::IModule;
using aidl::android::media::audio::common::MicrophoneDynamicInfo;
using aidl::android::media::audio::common::MicrophoneInfo;

#ifdef ENABLE_TAS_SPK_PROT
const char *AUDIO_PARAM_TI_HDR[] = {
    AUDIO_PARAM_TI_SMARTPA_CH,
    AUDIO_PARAM_TI_SMARTPA_IDX,
    AUDIO_PARAM_TI_SMARTPA_LEN,
    AUDIO_PARAM_TI_SMARTPA_GET
};
const char *AUDIO_PARAM_TI_VIDX[] = {
    "ti_v0",
    "ti_v1",
    "ti_v2",
    "ti_v3",
    "ti_v4",
    "ti_v5",
    "ti_v6",
    "ti_v7",
    "ti_v8",
    "ti_v9",
    "ti_v10"
};
#endif

namespace qti::audio::core {

btsco_lc3_cfg_t Platform::btsco_lc3_cfg = {};
#ifdef ENABLE_TAS_SPK_PROT
pal_tispk_prot_param_t Platform::tiSpkProtParam = {};
#endif

size_t Platform::getFrameCount(const AudioPortConfig& mixPortConfig, Usecase const& inTag) {
    const auto& tag = (inTag == Usecase::INVALID ? getUsecaseTag(mixPortConfig) : inTag);
    size_t numFrames = 0;

    if (mUsecaseOpMap.find(tag) != mUsecaseOpMap.end()) {
        numFrames = mUsecaseOpMap[tag].getFrameCount(mixPortConfig);
    } else {
        LOG(ERROR) << __func__ << "usecase not found " << getName(tag);
    }

    LOG(VERBOSE) << __func__ << " frames: " << numFrames << " for " << getName(tag);
    return numFrames;
}

struct BufferConfig Platform::getBufferConfig(const AudioPortConfig& mixPortConfig,
                                              Usecase const& inTag) {
    const auto& tag = (inTag == Usecase::INVALID ? getUsecaseTag(mixPortConfig) : inTag);
    struct BufferConfig config {};
    if (mUsecaseOpMap.find(tag) != mUsecaseOpMap.end()) {
        config = mUsecaseOpMap[tag].getBufferConfig(mixPortConfig);
    } else {
        LOG(ERROR) << __func__ << "usecase not found " << getName(tag);
    }

    return config;
}
#ifdef SEC_AUDIO_SUPPORT_UHQ
struct BufferConfig Platform::getBufferConfig(const AudioPortConfig& mixPortConfig,
                                              std::optional<std::pair<PcmType, pal_uhqa_state>>& uhqConfig,
                                              Usecase const& inTag) {
    const auto& tag = (inTag == Usecase::INVALID ? getUsecaseTag(mixPortConfig) : inTag);
    struct BufferConfig config;
    if (mUsecaseOpMap.find(tag) != mUsecaseOpMap.end()) {
        config = mUsecaseOpMap[tag].getBufferConfig(mixPortConfig);

        if (tag == Usecase::DEEP_BUFFER_PLAYBACK && uhqConfig.has_value()) {
            size_t formatRatio = getPcmSampleSizeInBytes(uhqConfig.value().first) /
                                           getPcmSampleSizeInBytes(mixPortConfig.format.value().pcm);
            size_t sampleRatio = (uint32_t)uhqConfig.value().second / mixPortConfig.sampleRate.value().value;
            config.bufferSize = config.bufferSize * formatRatio * sampleRatio;
            LOG(INFO) << __func__ << "newBufferSize for uhq " << config.bufferSize ;
        }
    } else {
        LOG(ERROR) << __func__ << "usecase not found " << getName(tag);
    }

    return config;
}
#endif

int32_t Platform::getLatencyMs(const AudioPortConfig& mixPortConfig, Usecase const& inTag) {
    if (mixPortConfig.ext.getTag() != AudioPortExt::Tag::mix) {
        LOG(ERROR) << __func__
                   << ": cannot deduce latency for port config which is not a mix port, "
                   << mixPortConfig.toString();
        return 0;
    }

    int32_t latencyMs = 0;

    const auto& tag = (inTag == Usecase::INVALID ? getUsecaseTag(mixPortConfig) : inTag);

    if (mUsecaseOpMap.find(tag) != mUsecaseOpMap.end()) {
        latencyMs = mUsecaseOpMap[tag].getLatency();
    } else {
        LOG(ERROR) << __func__ << "usecase not found " << getName(tag);
    }

    LOG(VERBOSE) << __func__ << ": latency" << latencyMs << " for " << getName(tag);

    return latencyMs;
}

size_t Platform::getMinimumStreamSizeFrames(const std::vector<AudioPortConfig*>& sources,
                                            const std::vector<AudioPortConfig*>& sinks) {
    if (sources.size() > 1) {
        LOG(WARNING) << __func__ << " unable to decide the minimum stream size for sources "
                                    "more than one; actual size:"
                     << sources.size();
        return 0;
    }
    // choose the mix port
    auto isMixPortConfig = [](const auto& audioPortConfig) {
        return audioPortConfig.ext.getTag() == AudioPortExt::Tag::mix;
    };

    const auto& mixPortConfig = isMixPortConfig(*sources.at(0)) ? *(sources.at(0)) : *(sinks.at(0));
#ifdef SEC_AUDIO_SAMSUNGRECORD
    if(getUnconfiguredFlagsReceived() == (AUDIO_INPUT_FLAG_FAST | AUDIO_INPUT_FLAG_RAW)) {
        return getFrameCount(mixPortConfig, Usecase::ULTRA_FAST_RECORD);
    }
#endif
    return getFrameCount(mixPortConfig);
}

std::unique_ptr<pal_stream_attributes> Platform::getPalStreamAttributes(
        const AudioPortConfig& portConfig, const bool isInput
#ifdef SEC_AUDIO_SUPPORT_UHQ
        , std::optional<std::pair<PcmType, pal_uhqa_state>> uhqConfig
#endif
        ) const {
    const auto& audioFormat = portConfig.format.value();
    const auto palFormat = PlatformConverter::getPalFormatId(audioFormat);
    if (palFormat == PAL_AUDIO_FMT_COMPRESSED_RANGE_END) {
        return nullptr;
    }

    const auto& audioChannelLayout = portConfig.channelMask.value();
    auto palChannelInfo = PlatformConverter::getPalChannelInfoForChannelCount(
            getChannelCount(audioChannelLayout));
    if (palChannelInfo == nullptr) {
        LOG(ERROR) << __func__ << " failed to find corresponding pal channel info for "
                   << audioChannelLayout.toString();
        return nullptr;
    }
    const auto sampleRate = portConfig.sampleRate.value().value;
    if (!sampleRate) {
        LOG(ERROR) << __func__ << " invalid sample rate " << std::to_string(sampleRate);
        return nullptr;
    }

    auto attributes = std::make_unique<pal_stream_attributes>();
    auto bitWidth = PlatformConverter::getBitWidthForAidlPCM(audioFormat);
    bitWidth == 0 ? (void)(bitWidth = kDefaultPCMBidWidth) : (void)0;

    if (!isInput) {
        attributes->direction = PAL_AUDIO_OUTPUT;
        attributes->out_media_config.sample_rate = sampleRate;
        attributes->out_media_config.aud_fmt_id = palFormat;
        attributes->out_media_config.ch_info = *(palChannelInfo);
        attributes->out_media_config.bit_width = bitWidth;
#ifdef SEC_AUDIO_SUPPORT_UHQ
        const auto& tag = getUsecaseTag(portConfig);
        if (tag == Usecase::DEEP_BUFFER_PLAYBACK && uhqConfig.has_value()) {
            AudioFormatDescription uhqFormat =
                          AudioFormatDescription{.type = AudioFormatType::PCM, .pcm = uhqConfig.value().first};
            attributes->out_media_config.sample_rate = (uint32_t)uhqConfig.value().second;
            attributes->out_media_config.aud_fmt_id = PlatformConverter::getPalFormatId(uhqFormat);
            attributes->out_media_config.bit_width = PlatformConverter::getBitWidthForAidlPCM(uhqFormat);
        }
#endif
    } else {
        attributes->direction = PAL_AUDIO_INPUT;
        attributes->in_media_config.sample_rate = sampleRate;
        attributes->in_media_config.aud_fmt_id = palFormat;
        attributes->in_media_config.ch_info = *(palChannelInfo);
        attributes->in_media_config.bit_width = bitWidth;
    }

    return std::move(attributes);
}

std::unique_ptr<pal_stream_attributes> Platform::getDefaultTelephonyAttributes() const {
    auto attributes = std::make_unique<pal_stream_attributes>();
    auto inChannelInfo = PlatformConverter::getPalChannelInfoForChannelCount(1);
    auto outChannelInfo = PlatformConverter::getPalChannelInfoForChannelCount(2);
    attributes->type = PAL_STREAM_VOICE_CALL;
    attributes->direction = PAL_AUDIO_INPUT_OUTPUT;
    attributes->in_media_config.sample_rate = kDefaultOutputSampleRate;
    attributes->in_media_config.ch_info = *inChannelInfo;
    attributes->in_media_config.bit_width = kDefaultPCMBidWidth;
    attributes->in_media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
    attributes->out_media_config.sample_rate = kDefaultOutputSampleRate;
    attributes->out_media_config.ch_info = *outChannelInfo;
    attributes->out_media_config.bit_width = kDefaultPCMBidWidth;
    attributes->out_media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
    return std::move(attributes);
}

std::unique_ptr<pal_stream_attributes> Platform::getDefaultCRSTelephonyAttributes() const {
    auto attributes = std::make_unique<pal_stream_attributes>();
    auto outChannelInfo = PlatformConverter::getPalChannelInfoForChannelCount(2);
    attributes->type = PAL_STREAM_LOOPBACK;
    attributes->info.opt_stream_info.loopback_type = PAL_STREAM_LOOPBACK_PLAYBACK_ONLY;
    attributes->direction = PAL_AUDIO_OUTPUT;
    attributes->out_media_config.sample_rate = kDefaultOutputSampleRate;
    attributes->out_media_config.ch_info = *outChannelInfo;
    attributes->out_media_config.bit_width = kDefaultPCMBidWidth;
    attributes->out_media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
    return std::move(attributes);
}

void Platform::configurePalDevicesCustomKey(std::vector<pal_device>& palDevices,
                                            const std::string& customKey) const {
    for (auto& palDevice : palDevices) {
        setPalDeviceCustomKey(palDevice, customKey);
    }
}

bool Platform::getMicMuteStatus() {
    return mMicMuted;
}

void Platform::setMicMuteStatus(bool mute) {
    mMicMuted = mute;
}

bool Platform::setStreamMicMute(pal_stream_handle_t* streamHandlePtr, const bool muted) {
    if (int32_t ret = ::pal_stream_set_mute(streamHandlePtr, muted); ret) {
        return false;
    }
    return true;
}

#ifdef SEC_AUDIO_SPK_AMP_MUTE
bool Platform::getSpeakerMuteStatus() const noexcept {
    return mSpeakerMuted;
}

void Platform::setSpeakerMute(const bool mute) noexcept {
    LOG(INFO) << __func__ << " : mute " << mute << ", mSpeakerMuted " << mSpeakerMuted;

    if (mute != mSpeakerMuted) {
        pal_param_speaker_status_t speakerStatus;
        speakerStatus.mute_status = mute ? PAL_DEVICE_SPEAKER_MUTE : PAL_DEVICE_SPEAKER_UNMUTE;
        if (int32_t ret = ::pal_set_param(PAL_PARAM_ID_SPEAKER_STATUS, &speakerStatus,
                                        sizeof(pal_param_speaker_status_t));
            ret) {
            LOG(ERROR) << __func__ << ": PAL_PARAM_ID_SPEAKER_STATUS failed";
            return;
        }
        mSpeakerMuted = mute;
    }
}
#endif

bool Platform::updateScreenState(const bool isTurnedOn) noexcept {
    mIsScreenTurnedOn = isTurnedOn;
    pal_param_screen_state_t screenState{.screen_state = mIsScreenTurnedOn};
    if (int32_t ret = ::pal_set_param(PAL_PARAM_ID_SCREEN_STATE, &screenState,
                                      sizeof(pal_param_screen_state_t));
        ret) {
        LOG(ERROR) << __func__ << ": PAL_PARAM_ID_SCREEN_STATE failed";
        return false;
    }
    return true;
}

bool Platform::isScreenTurnedOn() const noexcept {
    return mIsScreenTurnedOn;
}

void Platform::configurePalDevicesForHIFIPCMFilter(
        std::vector<pal_device>& palDevices) const noexcept {
    if (palDevices.size() == 0) {
        return;
    }

    bool isEnabled = false;

    auto getStatus = [&]() -> bool {
        bool status = false;
        bool* payLoad = &status;
        size_t payLoadSize = 0;
        if (int32_t ret =
                    ::pal_get_param(PAL_PARAM_ID_HIFI_PCM_FILTER,
                                    reinterpret_cast<void**>(&payLoad), &payLoadSize, nullptr);
            ret) {
            LOG(ERROR) << ": failed to get PAL_PARAM_ID_HIFI_PCM_FILTER status";
            return false;
        }
        return status;
    };

    for (auto& palDevice : palDevices) {
        if ((palDevice.id == PAL_DEVICE_OUT_WIRED_HEADSET ||
             palDevice.id == PAL_DEVICE_OUT_WIRED_HEADPHONE)) {
            if (!isEnabled) {
                isEnabled = getStatus();
            }
            if (isEnabled) {
                setPalDeviceCustomKey(palDevice, "hifi-filter_custom_key");
            }
        }
    }
}

void Platform::customizePalDevices(const AudioPortConfig& mixPortConfig, const Usecase& tag,
                                   std::vector<pal_device>& palDevices) const noexcept {
    const auto& sampleRate = getSampleRate(mixPortConfig);
    if (sampleRate && sampleRate.value() != 384000 && sampleRate.value() != 352800) {
        configurePalDevicesForHIFIPCMFilter(palDevices);
    }

    if (mIsHACEnabled && hasOutputVoipRxFlag(mixPortConfig.flags.value())) {
        auto itr = std::find_if(palDevices.begin(), palDevices.end(), [](const auto& palDevice) {
            return palDevice.id == PAL_DEVICE_OUT_HANDSET;
        });
        setPalDeviceCustomKey(*itr, "HAC");
    }
}

std::vector<pal_device> Platform::convertToPalDevices(
        const std::vector<AudioDevice>& devices) const noexcept {
    if (devices.size() == 0) {
        LOG(ERROR) << __func__ << " the set devices is empty";
        return {};
    }
    std::vector<pal_device> palDevices{devices.size()};

    size_t i = 0;
    for (auto& device : devices) {
        const auto palDeviceId = PlatformConverter::getPalDeviceId(device.type);
        if (palDeviceId == PAL_DEVICE_OUT_MIN) {
            return {};
        }
        palDevices[i].id = palDeviceId;

        /* Todo map each AIDL device type to alteast one PAL device */
        if (palDevices[i].id == PAL_DEVICE_OUT_SPEAKER &&
            device.type.type == AudioDeviceType::OUT_SPEAKER_SAFE) {
            setPalDeviceCustomKey(palDevices[i], "speaker-safe");
        } else if (palDevices[i].id == PAL_DEVICE_OUT_SPEAKER &&
                   device.type.type == AudioDeviceType::OUT_SPEAKER) {
            const auto isMSPPEnabled =
                    ::android::base::GetBoolProperty("vendor.audio.mspp.enable", false);
            if (isMSPPEnabled) {
                setPalDeviceCustomKey(palDevices[i], "mspp");
            }
        }

        palDevices[i].config.sample_rate = kDefaultOutputSampleRate;
        palDevices[i].config.bit_width = kDefaultPCMBidWidth;
        palDevices[i].config.aud_fmt_id = kDefaultPalPCMFormat;

        if (isUsbDevice(device)) {
            const auto& deviceAddress = device.address;
            if (deviceAddress.getTag() != AudioDeviceAddress::Tag::alsa) {
                LOG(ERROR) << __func__ << " failed to find alsa address for given usb device "
                           << device.toString();
                return {};
            }
            const auto& deviceAddressAlsa = deviceAddress.get<AudioDeviceAddress::Tag::alsa>();
            if (!isValidAlsaAddr(deviceAddressAlsa))
                return {};
            palDevices[i].address.card_id = deviceAddressAlsa[0];
            palDevices[i].address.device_num = deviceAddressAlsa[1];
        } else if (isHdmiDevice(device)) {
            if (auto result = getHdmiParameters(device)) {
                palDevices[i].id = result->deviceId;
            } else {
                return {};
            }
        }
        i++;
    }
    if (devices.size() == 2 && isHdmiDevice(devices[0]) && isHdmiDevice(devices[1])) {
        LOG(INFO) << __func__ << " Send latest DP device in the Pal list " << palDevices[1].id;
        return {palDevices[1]};
    }
    return palDevices;
}

std::vector<pal_device> Platform::getDummyPalDevices(const AudioPortConfig& mixPortConfig) const {
    struct pal_device dummyDevice = {};

    dummyDevice.config.sample_rate = Platform::kDefaultOutputSampleRate;
    dummyDevice.config.bit_width = Platform::kDefaultPCMBidWidth;
    dummyDevice.config.aud_fmt_id = Platform::kDefaultPalPCMFormat;
    dummyDevice.config.ch_info.channels = 2;

    if (isInputMixPortConfig(mixPortConfig)) {
        dummyDevice.id = PAL_DEVICE_IN_DUMMY;
    } else {
        dummyDevice.id = PAL_DEVICE_OUT_DUMMY;
    }

    return {dummyDevice};
}

/**
 * API is common for both Output and input streams
 */
std::vector<pal_device> Platform::configureAndFetchPalDevices(
        const AudioPortConfig& mixPortConfig, const Usecase& tag,
        const std::vector<AudioDevice>& devices, const bool dummyDevice) const {
    if (devices.empty()) {
        if (dummyDevice) {
            return getDummyPalDevices(mixPortConfig);
        } else {
            LOG(ERROR) << __func__ << " the set devices is empty";
            return {};
        }
    }
    auto palDevices = convertToPalDevices(devices);

    customizePalDevices(mixPortConfig, tag, palDevices);

    return palDevices;
}

#ifdef SEC_AUDIO_COMMON
std::vector<pal_device> Platform::configureSecPalDevicesForTelephony(
                                   const std::vector<AudioDevice>& devices) noexcept {
    if (devices.size() == 0) {
        LOG(ERROR) << __func__ << " the set devices is empty";
        return {};
    }
    auto palDevices = convertToPalDevices(devices);

    int ck_id = CUSTOM_KEY_INVALID;
    int key_dir = PAL_RX;
    size_t i = 0;
    for (auto& device : devices) {
        const auto palDeviceId = PlatformConverter::getPalDeviceId(device.type);
        if (palDeviceId == PAL_DEVICE_OUT_MIN) {
            return {};
        }
        palDevices[i].id = palDeviceId;

        // reset
        setPalDeviceCustomKey(palDevices[i], "");

        /* Case 1: call tx
        *  - rcv  : FLIP
        *  - bt   : BT_HEADSET_NREC
        *  - spk  : (volte vt) : VIDEO_CALL / VIDEO_CALL_FLEX / VIDEO_CALL_FLIP
        *           (comm) : DEX / FLIP
        */
        if (palDevices[i].id > PAL_DEVICE_IN_MIN) {
            key_dir = PAL_TX;
#if 0//def SEC_AUDIO_CALL_TTY//TEMP_FOR_SETUP_V
            for (int idx = 0; idx < MAX_VOICE_SESSIONS; idx++) {
                int cur_tty = avoice->voice_.session[idx].tty_mode;
                if ((cur_tty != PAL_TTY_OFF)
                        && avoice->voice_.session[idx].vsid == avoice->sec_voice_->cur_vsid) {
                    if (palDevices[i].id == PAL_DEVICE_IN_WIRED_HEADSET
                            || palDevices[i].id == PAL_DEVICE_IN_USB_HEADSET) {
                        if (cur_tty == PAL_TTY_FULL) {
                            ck_id = CUSTOM_KEY_TTY_FULL_MIC;
                        } else {
                            ck_id = CUSTOM_KEY_TTY;
                        }
                    } else if (cur_tty == PAL_TTY_VCO) {
                        if (palDevices[i].id == PAL_DEVICE_IN_HANDSET_MIC
                                || palDevices[i].id == PAL_DEVICE_IN_SPEAKER_MIC) {
                            ck_id = CUSTOM_KEY_TTY_VCO_MIC;
                        }
                    }
                }
            }
#endif
            if (palDevices[i].id == PAL_DEVICE_IN_HANDSET_MIC) {
                if (palDevices[1-i].id == PAL_DEVICE_OUT_HEARING_AID) {
                    ck_id = CUSTOM_KEY_HEARING_AID;
                } else if (palDevices[1-i].id == PAL_DEVICE_OUT_WIRED_HEADPHONE) {
                    ck_id = CUSTOM_KEY_HEADPHONE_MIC;
                } else if (palDevices[1-i].id == PAL_DEVICE_OUT_USB_HEADSET
                        && !mUSBCapEnable) {
                    ck_id = CUSTOM_KEY_USB_HEADPHONE_MIC;
                }
#ifdef SEC_AUDIO_SUPPORT_RCV_FLIP_CALL
                else if (!mHacIncall && mFolderclosed) {
                    ck_id = CUSTOM_KEY_FLIP;
                }
#endif
            } else if (palDevices[i].id == PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET) {
                if (!bt_nrec) {
                    ck_id = CUSTOM_KEY_BT_HEADSET_NREC;
                }
            } else if (palDevices[i].id == PAL_DEVICE_IN_SPEAKER_MIC) {
#ifdef SEC_AUDIO_SUPPORT_PERSONAL_VIDEOCALL
                if (mIsVolteVT) {
                    ck_id = CUSTOM_KEY_VIDEO_CALL;
                    if (mFlexmode) {
                        ck_id = CUSTOM_KEY_VIDEO_CALL_FLEX;
                    } else if (mFolderclosed) {
                        ck_id = CUSTOM_KEY_VIDEO_CALL_FLIP;
                    }
                } else
#endif
                {
                    if (mDexConnected) {
                        ck_id = CUSTOM_KEY_DEX;
                    }
                    else if (mFolderclosed) {
                        ck_id = CUSTOM_KEY_FLIP;
                    }
                }
            }
        }
        /* Case 2: call rx
        *  - headphone : HEADPHONE_MIC / USB_HEADPHONE_MIC
        *  - spk  : (volte vt) : VIDEO_CALL / VIDEO_CALL_FLEX / VIDEO_CALL_FLIP
        *           (comm) : DEX / FLIP
        *  - rev  : HAC / FLIP
        */
        else {
            key_dir = PAL_RX;
#if 0//def SEC_AUDIO_CALL_TTY//TEMP_FOR_SETUP_V
            for (int idx = 0; idx < MAX_VOICE_SESSIONS; idx++) {
                int cur_tty = avoice->voice_.session[idx].tty_mode;
                if ((cur_tty != PAL_TTY_OFF)
                        && avoice->voice_.session[idx].vsid == avoice->sec_voice_->cur_vsid) {
                    if (palDevices[i].id == PAL_DEVICE_OUT_WIRED_HEADPHONE
                            || palDevices[i].id == PAL_DEVICE_OUT_WIRED_HEADSET
                            || palDevices[i].id == PAL_DEVICE_OUT_USB_HEADSET) {
                        ck_id = CUSTOM_KEY_TTY;
                    } else if (cur_tty == PAL_TTY_HCO) {
                        if (palDevices[i].id == PAL_DEVICE_OUT_HANDSET
                                || palDevices[i].id == PAL_DEVICE_OUT_SPEAKER) {
                            ck_id = CUSTOM_KEY_TTY;
                        }
                    }
                }
            }
#endif
            if (palDevices[i].id == PAL_DEVICE_OUT_SPEAKER) {
#ifdef SEC_AUDIO_SUPPORT_PERSONAL_VIDEOCALL
                if (mIsVolteVT) {
                    ck_id = CUSTOM_KEY_VIDEO_CALL;
                    if (mFlexmode) {
                        ck_id = CUSTOM_KEY_VIDEO_CALL_FLEX;
                    } else if (mFolderclosed) {
                        ck_id = CUSTOM_KEY_VIDEO_CALL_FLIP;
                    }
                } else
#endif
                {
                    if (mDexConnected) {
                        ck_id = CUSTOM_KEY_DEX;
                    } else if (mFolderclosed) {
                        ck_id = CUSTOM_KEY_FLIP;
                    }
                }
            } else if (palDevices[i].id == PAL_DEVICE_OUT_HANDSET) {
#ifdef SEC_AUDIO_CALL_HAC
                if (mHacIncall) {
                    ck_id = GetHacCustomKeyId();
                }
#endif
#ifdef SEC_AUDIO_SUPPORT_RCV_FLIP_CALL
                else if (mFolderclosed) {
                    ck_id = CUSTOM_KEY_FLIP;
                }
#endif
            }
        }

        if (ck_id != CUSTOM_KEY_INVALID) {
            setPalDeviceCustomKey(palDevices[i], ck_table[ck_id]);
            ck_id = CUSTOM_KEY_INVALID;
            LOG(INFO) << __func__ << ": Setting custom key voice_" << (i == PAL_TX ? "tx " : "rx ")
                      << palDevices[i].custom_config.custom_key;
        }

        i++;
    }
    return palDevices;
}

std::vector<pal_device> Platform::configureSecPalDevicesForPlayback(const AudioPortConfig& mixPortConfig, const Usecase& tag,
                                   const std::vector<AudioDevice>& devices) noexcept {
    if (devices.size() == 0) {
        LOG(ERROR) << __func__ << " the set devices is empty";
        return {};
    }
    auto palDevices = convertToPalDevices(devices);

    int ck_id = CUSTOM_KEY_INVALID;
    size_t i = 0;
    for (auto& device : devices) {
        const auto palDeviceId = PlatformConverter::getPalDeviceId(device.type);
        if (palDeviceId == PAL_DEVICE_OUT_MIN) {
            return {};
        }
        palDevices[i].id = palDeviceId;

        // reset
        setPalDeviceCustomKey(palDevices[i], "");

        /* Case 1: usb+spk dual path */
        if (palDevices[i].id == PAL_DEVICE_OUT_USB_HEADSET &&
            ((devices.size() == 2)
#ifdef SEC_AUDIO_USB_GAIN_CONTROL
                || (tag == Usecase::DEEP_BUFFER_PLAYBACK && getUSBGainForCombo())
#endif
            )) {
            ck_id = CUSTOM_KEY_SPEAKER_AND_USB_HEADPHONES;
        }
        /* Case 2: wifi call
        *  - default : VOWIFI
        *  - rcv     : VOWIFI_HAC / VOWIFI_FLIP
        *  - spk     : VOWIFI_DEX / VOWIFI_FLIP
        */
        else if (mIsVoWiFi) {
            ck_id = CUSTOM_KEY_VOWIFI;
            if (palDevices[i].id == PAL_DEVICE_OUT_HANDSET) {
#ifdef SEC_AUDIO_CALL_HAC
                if (mHacIncall) {
                    ck_id = GetVoWifiHacCustomKeyId();
                }
#endif
#ifdef SEC_AUDIO_SUPPORT_RCV_FLIP_CALL
                else if (mFolderclosed) {
                    ck_id = CUSTOM_KEY_VOWIFI_FLIP;
                }
#endif
            }

            else if (palDevices[i].id == PAL_DEVICE_OUT_SPEAKER) {
                if (mDexConnected) {
                    ck_id = CUSTOM_KEY_VOWIFI_DEX;
                }
                else if (mFolderclosed) {
                    ck_id = CUSTOM_KEY_VOWIFI_FLIP;
                }
            }
        }
        /* Case 3: video call (SUPPORT_PERSONAL_VIDEOCALL model only)
        *  - default : VIDEO_CALL
        *  - spk     : VIDEO_CALL_FLIP/ VIDEO_CALL_FLEX
        */
#ifdef SEC_AUDIO_SUPPORT_PERSONAL_VIDEOCALL
        else if (mIsVolteVT) {
            ck_id = CUSTOM_KEY_VIDEO_CALL;
            if (palDevices[i].id == PAL_DEVICE_OUT_SPEAKER)  {
                if (mFolderclosed) {
                    ck_id = CUSTOM_KEY_VIDEO_CALL_FLIP;
                } else if (mFlexmode) {
                    ck_id = CUSTOM_KEY_VIDEO_CALL_FLEX;
                }
            }
        }
#endif
        /* Case 4: voip comm call
        *  - default : VOIP_COMM
        *  - rcv     : HAC / FLIP
        *  - spk     : DEX / VOIP_TV / VOIP_GAMING_FLIP / VOIP_GAMING / FLIP / FLEX
        */
        else if (mCallMode == AUDIO_MODE_IN_COMMUNICATION) {
            if (tag == Usecase::VOIP_PLAYBACK) {
                ck_id = CUSTOM_KEY_VOIP_COMM;
                if (palDevices[i].id == PAL_DEVICE_OUT_HANDSET) {
#ifdef SEC_AUDIO_CALL_HAC
                    if (mHacIncall) {
                        ck_id = GetHacCustomKeyId();
                    }
#endif
                    else if (mFolderclosed) {
                        ck_id = CUSTOM_KEY_FLIP;
                    }
                } else if (palDevices[i].id == PAL_DEVICE_OUT_SPEAKER) {
                    if (mDexConnected) {
                        ck_id = CUSTOM_KEY_DEX;
                    }
#ifdef SEC_AUDIO_SCREEN_MIRRORING // { SUPPORT_VOIP_VIA_SMART_VIEW
                    /*else*/ if (mVoipViaSmartView) {
                        ck_id = CUSTOM_KEY_VOIP_TV;
                    }
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW
#ifdef SEC_AUDIO_SUPPORT_GAMECHAT_SPK_AEC
                    else if (mGamechatMode) {
                        if (mFolderclosed)
                            ck_id = CUSTOM_KEY_VOIP_GAMING_FLIP;
                        else
                            ck_id = CUSTOM_KEY_VOIP_GAMING;
                    }
#endif
                    else if (mFolderclosed) {
                        ck_id = CUSTOM_KEY_FLIP;
                    }
#ifdef SEC_AUDIO_SUPPORT_PERSONAL_VIDEOCALL
                    else if (mFlexmode) {
                        ck_id = CUSTOM_KEY_FLEX;
                    }
#endif
                }
            }
        }
        /* Case 5: karaoke */
#ifdef SEC_AUDIO_KARAOKE
        else if (is_karaoke_on && palDevices[i].id == PAL_DEVICE_OUT_SPEAKER) {
            if (isKaraokeUsecases(tag)) {
                ck_id = CUSTOM_KEY_KARAOKE;
            }
        }
#endif
        /* Case 6: dual spk ampL off */
#if defined(SEC_AUDIO_DUAL_SPEAKER) && defined(SEC_AUDIO_ENHANCED_DUAL_SPEAKER)  && \
    !defined(SEC_AUDIO_FACTORY_TEST_MODE)
        else if (isSpeakerLeftAmpOff()
                && (devices.size() == 1)
                && (palDevices[i].id == PAL_DEVICE_OUT_SPEAKER)) {
            LOG(INFO) << __func__ << ": set custom key for SPEAKER_LEFT_AMP_OFF";
            ck_id = CUSTOM_KEY_SPEAKER_LEFT_AMP_OFF;
        }
#endif

        if (ck_id != CUSTOM_KEY_INVALID) {
            setPalDeviceCustomKey(palDevices[i], ck_table[ck_id]);
            LOG(INFO) << __func__ << ": Setting custom key as " << palDevices[i].custom_config.custom_key;
        }

        i++;
    }

#ifdef SEC_AUDIO_CALL_VOIP
    if (tag == Usecase::VOIP_PLAYBACK) {
        mPalDevicesOnVoipRx = palDevices;
    }
#endif

    return palDevices;
}

std::vector<pal_device> Platform::configureSecPalDevicesForCapture(const AudioPortConfig& mixPortConfig, const Usecase& tag,
                                   const std::vector<AudioDevice>& devices) const noexcept {
    if (devices.size() == 0) {
        LOG(ERROR) << __func__ << " the set devices is empty";
        return {};
    }
    auto palDevices = convertToPalDevices(devices);

    int ck_id = CUSTOM_KEY_INVALID;
    size_t i = 0;
    auto attr = getPalStreamAttributes(mixPortConfig, true);
    const auto& source = getMixPortAudioSource(mixPortConfig);

    auto isDeviceAvailable = [&](AudioDevice d) {
        return (std::find(devices.begin(), devices.end(), d) != devices.end());
    };

#ifdef SEC_AUDIO_CALL_VOIP
    auto isDeviceAvailableOnVoipOut = [&](pal_device_id_t device) {
        return (std::find_if(mPalDevicesOnVoipRx.begin(), mPalDevicesOnVoipRx.end(),
                [&](const auto& d) { return d.id == device; }) != mPalDevicesOnVoipRx.end());
    };
#endif

    if (!attr) {
        LOG(ERROR) << __func__ << " no pal attributes";
        return {};
    }
    for (auto& device : devices) {
        const auto palDeviceId = PlatformConverter::getPalDeviceId(device.type);
        if (palDeviceId == PAL_DEVICE_OUT_MIN) {
            return {};
        }
        palDevices[i].id = palDeviceId;

        // reset
        setPalDeviceCustomKey(palDevices[i], "");

#ifdef SEC_AUDIO_COMPRESS_CAPTURE
        if (tag == Usecase::COMPRESS_CAPTURE) {
            LOG(DEBUG) << __func__ << " Compress capture doesn't need custom key";
            return {};
        }
#endif

        /* Case 1: camcorder */
        if (source && source.value() == AudioSource::CAMCORDER) {
#if SEC_AUDIO_MULTI_MIC >= 3 || defined (SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO)
            if (GetRecMultiMic(mixPortConfig, devices, tag)) {
#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO
                if (IsBtForMultiDevice(devices) && multidevice_rec)
                    ck_id = CUSTOM_KEY_CAMCORDER_MULTI_AND_BT_MIC;
                else
#endif
                ck_id = CUSTOM_KEY_CAMCORDER_MULTI_MIC;
            } else
#endif
            if (isDeviceAvailable(AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE_MULTI})) {
                if (attr->type == PAL_STREAM_ULTRA_LOW_LATENCY)
                    ck_id = CUSTOM_KEY_REC_STEREO_MIC;
                else
                    ck_id = CUSTOM_KEY_CAMCORDER_MULTI_MIC;
            } else {
                ck_id = CUSTOM_KEY_CAMCORDER;
            }
        }
        /* Case 2: beamforming */
        else if (source && source.value() == AudioSource::SEC_VOICENOTE_BEAMFORMING) {
            ck_id = CUSTOM_KEY_REC_INTERVIEW;
        }
        /* Case 3: voip tx
        *  (1): wifi call
        *  - default   : VOWIFI
        *  - headphone : VOWIFI_HEADPHONE_MIC / VOWIFI_USB_HEADPHONE_MIC
        *  - bt        : VOWIFI_BT_HEADSET_NREC
        *  - rcv       : VOWIFI_FLIP
        *  - spk       : VOWIFI_DEX / VOWIFI_FLIP
        * (2) video call (SUPPORT_PERSONAL_VIDEOCALL model only)
        *  - default   : VIDEO_CALL
        *  - spk       : VIDEO_CALL_FLIP/ VIDEO_CALL_FLEX
        * (3) voip comm call
        *  - headphone : HEADPHONE_MIC / USB_HEADPHONE_MIC
        *  - bt        : BT_HEADSET_NREC
        *  - rcv       : FLIP
        *  - spk       : DEX / VOIP_TV / VOIP_GAMING_FLIP / VOIP_GAMING / FLIP / FLEX
        */
        else if (tag == Usecase::VOIP_RECORD) {
            if (mIsVoWiFi) {
                ck_id = CUSTOM_KEY_VOWIFI;
                if (isDeviceAvailableOnVoipOut(PAL_DEVICE_OUT_WIRED_HEADPHONE)
                        && device.type.type == AudioDeviceType::IN_MICROPHONE) {
                    ck_id = CUSTOM_KEY_VOWIFI_HEADPHONE_MIC;
                } else if (isDeviceAvailableOnVoipOut(PAL_DEVICE_OUT_USB_HEADSET)
                        && device.type.type == AudioDeviceType::IN_MICROPHONE) {
                    ck_id = CUSTOM_KEY_VOWIFI_USB_HEADPHONE_MIC;
                } else if (palDevices[i].id == PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET) {
                    if (!bt_nrec) {
                        ck_id = CUSTOM_KEY_VOWIFI_BT_HEADSET_NREC;
                    }
                } else if (palDevices[i].id == PAL_DEVICE_IN_HANDSET_MIC) {
#ifdef SEC_AUDIO_SUPPORT_RCV_FLIP_CALL
                    if (!mHacIncall && mFolderclosed) {
                        ck_id = CUSTOM_KEY_VOWIFI_FLIP;
                    }
#endif
                } else if (palDevices[i].id == PAL_DEVICE_IN_SPEAKER_MIC) {
                    if (mDexConnected) {
                        ck_id = CUSTOM_KEY_VOWIFI_DEX;
                    } else if (mFolderclosed) {
                        ck_id = CUSTOM_KEY_VOWIFI_FLIP;
                    }
                }
            }
#ifdef SEC_AUDIO_SUPPORT_PERSONAL_VIDEOCALL
            else if (avoice->sec_voice_->volte_vt) {
                ck_id = CUSTOM_KEY_VIDEO_CALL;
                else if (palDevices[i].id == PAL_DEVICE_IN_SPEAKER_MIC) {
                    if (mFolderclosed) {
                        ck_id = CUSTOM_KEY_VIDEO_CALL_FLIP;
                    } else if (mFlexmode) {
                        ck_id = CUSTOM_KEY_VIDEO_CALL_FLEX;
                    }
                }
            }
#endif
            else {
                if (isDeviceAvailableOnVoipOut(PAL_DEVICE_OUT_WIRED_HEADPHONE)
                        && device.type.type == AudioDeviceType::IN_MICROPHONE) {
                    ck_id = CUSTOM_KEY_HEADPHONE_MIC;
                } else if (isDeviceAvailableOnVoipOut(PAL_DEVICE_OUT_USB_HEADSET)
                        && device.type.type == AudioDeviceType::IN_MICROPHONE) {
                    ck_id = CUSTOM_KEY_USB_HEADPHONE_MIC;
                } else if (palDevices[i].id == PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET) {
                    if (!bt_nrec) {
                        ck_id = CUSTOM_KEY_BT_HEADSET_NREC;
                    }
                } else if (palDevices[i].id == PAL_DEVICE_IN_HANDSET_MIC) {
#ifdef SEC_AUDIO_SUPPORT_RCV_FLIP_CALL
                    if (mFolderclosed) {
                        ck_id = CUSTOM_KEY_FLIP;
                    }
#endif
                }
#ifdef SEC_AUDIO_HDMI // { SUPPORT_VOIP_VIA_SMART_MONITOR
                else if (isDeviceAvailableOnVoipOut(PAL_DEVICE_OUT_AUX_DIGITAL) &&
                        (palDevices[i].id == PAL_DEVICE_IN_SPEAKER_MIC)) {
                    ck_id = CUSTOM_KEY_VOIP_SMONITOR;
                }
#endif // } SUPPORT_VOIP_VIA_SMART_MONITOR
                else if (palDevices[i].id == PAL_DEVICE_IN_SPEAKER_MIC) {
                    if (mDexConnected) {
                        ck_id = CUSTOM_KEY_DEX;
                    }
#ifdef SEC_AUDIO_SCREEN_MIRRORING // { SUPPORT_VOIP_VIA_SMART_VIEW
                    else if (mVoipViaSmartView) {
                        ck_id = CUSTOM_KEY_VOIP_TV;
                    }
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW
#ifdef SEC_AUDIO_SUPPORT_GAMECHAT_SPK_AEC
                    else if (mGamechatMode) {
                        if (mFolderclosed)
                            ck_id = CUSTOM_KEY_VOIP_GAMING_FLIP;
                        else
                            ck_id = CUSTOM_KEY_VOIP_GAMING;
                    }
#endif
                    else if (mFolderclosed) {
                        ck_id = CUSTOM_KEY_FLIP;
                    }
#ifdef SEC_AUDIO_SUPPORT_PERSONAL_VIDEOCALL
                    else if (mFlexmode) {
                        ck_id = CUSTOM_KEY_FLEX;
                    }
#endif
                }
            }
        }
        /* Case 4: voice recognition */
        else if (source && source.value() == AudioSource::VOICE_RECOGNITION) {
            if (tag == Usecase::ULTRA_FAST_RECORD
                && getChannelCount(mixPortConfig.channelMask.value()) == 2) {
                ck_id = CUSTOM_KEY_VR_DUAL;
            } else
                ck_id = CUSTOM_KEY_VR;
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
            if (register_voice_keyword) {
                ck_id = CUSTOM_KEY_BIXBY_ENROLL;
            } else if (tag == Usecase::HOTWORD_RECORD &&
                        palDevices[i].id == PAL_DEVICE_IN_HANDSET_MIC) {
                ck_id = CUSTOM_KEY_VR_LOWPOWER;
            } else
#endif
#ifdef SEC_AUDIO_KARAOKE
            if (is_karaoke_on &&
                    palDevices[i].id == PAL_DEVICE_IN_SPEAKER_MIC) {
                ck_id = CUSTOM_KEY_KARAOKE;
            }
#endif
        }
        /* Case 5: sec voice recognition */
        else if (source && source.value() == AudioSource::SEC_VOICE_RECOGNITION) {
            ck_id = CUSTOM_KEY_BARGEIN_TTS;
            if ((mCallMode != AUDIO_MODE_IN_CALL && mCallMode != AUDIO_MODE_IN_COMMUNICATION) &&
                    (palDevices[i].id == PAL_DEVICE_IN_HANDSET_MIC))
                palDevices[i].id = PAL_DEVICE_IN_SPEAKER_MIC;
        }
        /* Case 6: 2mic svoice driving */
        else if (source && source.value() == AudioSource::SEC_2MIC_SVOICE_DRIVING) {
            ck_id = CUSTOM_KEY_VR_FARFIELD;
        }
        /* Case 7: 2mic svoice */
        else if (source && source.value() == AudioSource::SEC_2MIC_SVOICE_NORMAL) {
            if (palDevices[i].id == PAL_DEVICE_IN_SPEAKER_MIC) {
                ck_id = CUSTOM_KEY_VR_DUAL;
            } else {
                ck_id = CUSTOM_KEY_VR;
            }
        }
        /* Case 8: bargeing driving */
        else if (source && source.value() == AudioSource::SEC_BARGEIN_DRIVING) {
#ifdef SEC_AUDIO_INTERPRETER_MODE
            if (interpreter_mode == INTERPRETER_CONVERSATION) {
                if (device.type.type == AudioDeviceType::IN_HEADSET &&
                    device.type.connection == AudioDeviceDescription::CONNECTION_USB) {
                    ck_id = CUSTOM_KEY_INTERPRETER_CONVERSATION_USB;
                } else {
                    ck_id = CUSTOM_KEY_INTERPRETER_CONVERSATION_SPK;
                    palDevices[i].id = PAL_DEVICE_IN_SPEAKER_MIC;
                }
            } else if (interpreter_mode == INTERPRETER_LISTENING) {
                palDevices[i].id = PAL_DEVICE_IN_SPEAKER_MIC;
                ck_id = CUSTOM_KEY_INTERPRETER_LISTENING_SPK;
                if (hasBluetoothDevice(mOutDeepDevices)) {
                    ck_id = CUSTOM_KEY_INTERPRETER_LISTENING_BT;
                } else if (hasUsbHeadsetDevice(mOutDeepDevices)) {
                    ck_id = CUSTOM_KEY_INTERPRETER_LISTENING_USB;
                }
            }
            else
#endif
            {
                ck_id = CUSTOM_KEY_BARGEIN_AEC;
                auto palTelephonyDevices = convertToPalDevices(getTelephonyDevices());
                if (palDevices[i].id == PAL_DEVICE_IN_HANDSET_MIC &&
                    !((mCallMode == AUDIO_MODE_IN_CALL &&
                            palTelephonyDevices.size() > 0 && palTelephonyDevices[0].id == PAL_DEVICE_OUT_HANDSET) ||
                        (mCallMode == AUDIO_MODE_IN_COMMUNICATION &&
                            isDeviceAvailableOnVoipOut(PAL_DEVICE_OUT_HANDSET)))) {
                        palDevices[i].id = PAL_DEVICE_IN_SPEAKER_MIC;
                }
            }
        }
        /* Case 9: 2mic rec */
        else if (isDeviceAvailable(AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE_MULTI})) {
            ck_id = CUSTOM_KEY_REC_STEREO_MIC;
        }
        /* Case 10: karaoke */
#ifdef SEC_AUDIO_KARAOKE
        else if (is_karaoke_on &&
                    palDevices[i].id == PAL_DEVICE_IN_SPEAKER_MIC) {
            ck_id = CUSTOM_KEY_KARAOKE;
        }
#endif
        else if (source &&
            (source.value() == AudioSource::UNPROCESSED || source.value() == AudioSource::VOICE_PERFORMANCE)) {
            if (getChannelCount(mixPortConfig.channelMask.value()) == 2)
                ck_id = CUSTOM_KEY_VR_DUAL;
            else
                ck_id = CUSTOM_KEY_VR;
        }

        if (ck_id != CUSTOM_KEY_INVALID) {
            setPalDeviceCustomKey(palDevices[i], ck_table[ck_id]);
            LOG(INFO) << __func__ << ": Setting custom key as " << palDevices[i].custom_config.custom_key;
        }

        // SEC_AUDIO_FACTORY

        i++;
    }
    return palDevices;
}
#endif
#ifdef SEC_AUDIO_CALL_HAC
int Platform::GetHacCustomKeyId() {
    int ck_id = CUSTOM_KEY_HAC;

    switch (mHacMode) {
    case HAC_MODE_MIC:
        ck_id = CUSTOM_KEY_HAC;
        break;
    case HAC_MODE_TCOIL:
        ck_id = CUSTOM_KEY_TCOIL_HAC;
        break;
    }

    return ck_id;
}

int Platform::GetVoWifiHacCustomKeyId() {
    int ck_id = CUSTOM_KEY_VOWIFI_HAC;

    switch (mHacMode) {
    case HAC_MODE_MIC:
        ck_id = CUSTOM_KEY_VOWIFI_HAC;
        break;
    case HAC_MODE_TCOIL:
        ck_id = CUSTOM_KEY_VOWIFI_TCOIL_HAC;
        break;
    }

    return ck_id;
}
#endif

#ifdef SEC_AUDIO_CALL_FORWARDING
bool Platform::isCallForwarding()
{
    if (mCallForwarding || mCallMemo == CALLMEMO_ON) {
        return true;
    }
    return false;
}
#endif

#ifdef SEC_AUDIO_CALL
int Platform::GetDeviceType(pal_device_id_t rx_device_id) {
    int device_type = VOICE_DEVICE_ETC;

    if (rx_device_id == PAL_DEVICE_OUT_SPEAKER)
        device_type = VOICE_DEVICE_SPEAKER;
    else if (rx_device_id == PAL_DEVICE_OUT_WIRED_HEADSET ||
                rx_device_id == PAL_DEVICE_OUT_WIRED_HEADPHONE)
        device_type = VOICE_DEVICE_EARPHONE;
    else if (rx_device_id == PAL_DEVICE_OUT_BLUETOOTH_SCO)
        device_type = VOICE_DEVICE_BLUETOOTH;
    else if (rx_device_id == PAL_DEVICE_OUT_HANDSET)
        device_type = VOICE_DEVICE_RECEIVER;

    LOG(INFO) << __func__ << " device id " << rx_device_id<< " device_type "  << device_type;
    return device_type;
}
#endif

void Platform::getPositionInFrames(pal_stream_handle_t* palHandle, int32_t const& sampleRate,
                                         int64_t* const dspFrames) const {
    pal_session_time tstamp;
    if (int32_t ret = ::pal_get_timestamp(palHandle, &tstamp); ret) {
        LOG(ERROR) << __func__ << " pal_get_timestamp failure, ret:" << ret;
        return;
    }

    uint64_t sessionTimeUs =
            ((static_cast<decltype(sessionTimeUs)>(tstamp.session_time.value_msw)) << 32 |
             tstamp.session_time.value_lsw);
    // sessionTimeUs to frames
    *dspFrames = static_cast<int64_t>((sessionTimeUs / 1000) * (sampleRate / 1000));
    LOG(VERBOSE) << __func__ << " dsp frames consumed:" << *dspFrames;
    return;
}

int Platform::setVolume(pal_stream_handle_t* handle, const std::vector<float>& volumes) const {
    auto data = makePalVolumes(volumes);
    if (data.empty()) {
        LOG(ERROR) << __func__ << ": failed to configure volume";
        return -1;
    }
    auto palVolumeData = reinterpret_cast<pal_volume_data*>(data.data());

    return ::pal_stream_set_volume(handle, palVolumeData);
}

#ifdef SEC_AUDIO_COMMON
std::vector<uint8_t> Platform::getPalVolume(const std::vector<float>& volumes) {
    return makePalVolumes(volumes);
}
#endif

std::unique_ptr<pal_buffer_config_t> Platform::getPalBufferConfig(const size_t bufferSize,
                                                                  const size_t bufferCount) const {
    auto palBufferConfig = std::make_unique<pal_buffer_config_t>();
    palBufferConfig->buf_size = bufferSize;
    palBufferConfig->buf_count = bufferCount;
    return std::move(palBufferConfig);
}

std::vector<::aidl::android::media::audio::common::AudioProfile> Platform::getUsbProfiles(
        const AudioPort& port) const {
    const auto& devicePortExt = port.ext.get<AudioPortExt::Tag::device>();
    auto& audioDeviceDesc = devicePortExt.device.type;
    const auto palDeviceId = PlatformConverter::getPalDeviceId(audioDeviceDesc);
    if (palDeviceId == PAL_DEVICE_OUT_MIN) {
        return {};
    }

    const auto& addressTag = devicePortExt.device.address.getTag();
    if (addressTag != AudioDeviceAddress::Tag::alsa) {
        LOG(ERROR) << __func__ << ": no alsa address provided for the AudioPort" << port.toString();
        return {};
    }
    const auto& deviceAddressAlsa =
            devicePortExt.device.address.get<AudioDeviceAddress::Tag::alsa>();
    if (!isValidAlsaAddr(deviceAddressAlsa))
        return {};
    const auto cardId = deviceAddressAlsa[0];
    const auto deviceId = deviceAddressAlsa[1];

    // get capability from device of USB
    auto deviceCapability = std::make_unique<pal_param_device_capability_t>();
    if (!deviceCapability) {
        LOG(ERROR) << __func__ << ": allocation failed ";
        return {};
    }

    auto dynamicMediaConfig = std::make_unique<dynamic_media_config_t>();
    if (!dynamicMediaConfig) {
        LOG(ERROR) << __func__ << ": allocation failed ";
        return {};
    }

    size_t payloadSize = 0;
    deviceCapability->addr.card_id = cardId;
    deviceCapability->addr.device_num = deviceId;
    deviceCapability->config = dynamicMediaConfig.get();
    if (isOutputDevice(devicePortExt.device)) {
        deviceCapability->id = palDeviceId;
        deviceCapability->is_playback = true;
    } else {
        deviceCapability->id = PAL_DEVICE_IN_USB_HEADSET;
        deviceCapability->is_playback = false;
    }

    void* deviceCapabilityPtr = deviceCapability.get();
    if (int32_t ret = pal_get_param(PAL_PARAM_ID_DEVICE_CAPABILITY, &deviceCapabilityPtr,
                                    &payloadSize, nullptr);
        ret != 0) {
        LOG(ERROR) << __func__ << " PAL get param failed for PAL_PARAM_ID_DEVICE_CAPABILITY" << ret;
        return {};
    }
    if (!dynamicMediaConfig->jack_status) {
        LOG(ERROR) << __func__ << " false usb jack status ";
        return {};
    }
    if (!deviceCapability->is_playback) {
        if ((dynamicMediaConfig.get()->sample_rate[0] == 0 && dynamicMediaConfig.get()->format[0] == 0 &&
             dynamicMediaConfig.get()->mask[0] == 0) || (dynamicMediaConfig->jack_status == false)) {
            mUSBCapEnable = false;
        } else {
            mUSBCapEnable = true;
        }
    }

    return getSupportedAudioProfiles(deviceCapability.get(), "usb");
}

std::vector<AudioProfile> Platform::getDynamicProfiles(
        const AudioPort& dynamicDeviceAudioPort) const {
    const auto& deviceExtTag = dynamicDeviceAudioPort.ext.getTag();
    if (deviceExtTag != AudioPortExt::Tag::device) {
        LOG(ERROR) << __func__ << ": provided AudioPort is not device port"
                   << dynamicDeviceAudioPort.toString();
        return {};
    }

    LOG(VERBOSE) << __func__ << ": fetching dynamic profiles for "
                 << dynamicDeviceAudioPort.toString();

    const auto& devicePortExt = dynamicDeviceAudioPort.ext.get<AudioPortExt::Tag::device>();

    if (isUsbDevice(devicePortExt.device)) {
        return getUsbProfiles(dynamicDeviceAudioPort);
    }

    LOG(VERBOSE) << __func__ << " unsupported " << dynamicDeviceAudioPort.toString();
    return {};
}

std::optional<struct HdmiParameters> Platform::getHdmiParameters(
        const ::aidl::android::media::audio::common::AudioDevice& device) const {
#ifdef SEC_AUDIO_HDMI
    int controller = 0;
    int stream = 0;
#else
    const auto& addressTag = device.address.getTag();
    if (addressTag != AudioDeviceAddress::Tag::id ||
        device.address.get<AudioDeviceAddress::Tag::id>().empty()) {
        LOG(ERROR) << __func__ << ": no hdmi address controller/stream provided for the device"
                   << device.toString();
        return std::nullopt;
    }
    const auto hdmiAddress = device.address.get<AudioDeviceAddress::Tag::id>();
    int controller = -1;
    int stream = -1;

    int status = std::sscanf(hdmiAddress.c_str(), "controller=%d;stream=%d", &controller, &stream);
    if (status != 2) {
        LOG(ERROR) << __func__ << ": failed to extract HDMI parameter from device"
                   << device.toString();
        return std::nullopt;
    }
#endif
    pal_device_id_t deviceId = PAL_DEVICE_OUT_AUX_DIGITAL;
    LOG(DEBUG) << __func__ << " controller " << controller << " stream " << stream;
    if (stream) {
        deviceId = PAL_DEVICE_OUT_AUX_DIGITAL_1;
        LOG(DEBUG) << __func__ << " override palDevice with PAL_DEVICE_OUT_AUX_DIGITAL_1";
    }
    struct HdmiParameters hdmiParam = {
            .controller = controller, .stream = stream, .deviceId = deviceId};
    return hdmiParam;
}

int Platform::handleDeviceConnectionChange(const AudioPort& deviceAudioPort,
                                            const bool isConnect) const {
    const auto& devicePortExt = deviceAudioPort.ext.get<AudioPortExt::Tag::device>();

    auto& audioDeviceDesc = devicePortExt.device.type;
    const auto palDeviceId = PlatformConverter::getPalDeviceId(audioDeviceDesc);
    if (palDeviceId == PAL_DEVICE_OUT_MIN) {
        return -EINVAL;
    }

    void* v = nullptr;
    const auto deviceConnection = std::make_unique<pal_param_device_connection_t>();
    if (!deviceConnection) {
        LOG(ERROR) << __func__ << ": allocation failed ";
        return -EINVAL;
    }

    deviceConnection->connection_state = isConnect;
    deviceConnection->id = palDeviceId;

    if (isUsbDevice(devicePortExt.device)) {
        const auto& addressTag = devicePortExt.device.address.getTag();
        if (addressTag != AudioDeviceAddress::Tag::alsa) {
            LOG(ERROR) << __func__ << ": no alsa address provided for the AudioPort"
                       << deviceAudioPort.toString();
            return -EINVAL;
        }
        const auto& deviceAddressAlsa =
                devicePortExt.device.address.get<AudioDeviceAddress::Tag::alsa>();
        if (!isValidAlsaAddr(deviceAddressAlsa)) {
            return -EINVAL;
        }
        const auto cardId = deviceAddressAlsa[0];
        const auto deviceId = deviceAddressAlsa[1];
        deviceConnection->device_config.usb_addr.card_id = cardId;
        deviceConnection->device_config.usb_addr.device_num = deviceId;
#ifdef SEC_AUDIO_SUPPORT_USB_OFFLOAD
        AudioExtensionBase::setUSBCardConfig(deviceConnection->device_config.usb_addr);
        if (!isConnect && isInputDevice(devicePortExt.device)) {
            mUSBCapEnable = false;
        }
#endif
    } else if (isHdmiDevice(devicePortExt.device)) {
        if (auto result = getHdmiParameters(devicePortExt.device)) {
            deviceConnection->device_config.dp_config.controller = result->controller;
            deviceConnection->device_config.dp_config.stream = result->stream;
            deviceConnection->id = result->deviceId;
        } else {
            return -EINVAL;
        }
    }  else if (isIPDevice(devicePortExt.device)) {
           if (!isIPAsProxyDeviceConnected()) {
                return -EINVAL;
           }
    }

#ifdef SEC_AUDIO_BT_OFFLOAD
    if (isBluetoothA2dpDevice(devicePortExt.device)) {
#ifdef SEC_AUDIO_BT_MULTIPLE_PROFILE
        if (!audio_is_bt_offload_format(bt_a2dp_format)) {
            LOG(WARNING) << __func__ << " offload state change when only offload format";
            return 0;
        }
#endif
        bool isBtOffload = isConnect ?
                           audio_is_bt_offload_format(bt_a2dp_format) : false;
        deviceConnection->is_bt_offload_enabled = isBtOffload;
        LOG(INFO) << __func__ << "bt_offload state is"
                  << (isBtOffload ? " enabled" : " disabled");
    }
#endif

    v = deviceConnection.get();
    if (int32_t ret = ::pal_set_param(PAL_PARAM_ID_DEVICE_CONNECTION, v,
                                      sizeof(pal_param_device_connection_t));
        ret != 0) {
        LOG(ERROR) << __func__ << ": pal_set_param failed for PAL_PARAM_ID_DEVICE_CONNECTION for "
                   << audioDeviceDesc.toString();
        return ret;
    }
    LOG(INFO) << __func__ << devicePortExt.device.toString()
              << (isConnect ? ": connected" : "disconnected");

    return 0;
}

void Platform::setWFDProxyChannels(const uint32_t numProxyChannels) noexcept {
    mWFDProxyChannels = numProxyChannels;
    pal_param_proxy_channel_config_t paramProxyChannelConfig{.num_proxy_channels =
                                                                     mWFDProxyChannels};
    if (int32_t ret = ::pal_set_param(PAL_PARAM_ID_PROXY_CHANNEL_CONFIG, &paramProxyChannelConfig,
                                      sizeof(pal_param_proxy_channel_config_t));
        ret) {
        LOG(ERROR) << __func__ << ": PAL_PARAM_ID_PROXY_CHANNEL_CONFIG failed: " << ret;
        return;
    }
}

void Platform::setProxyRecordFMQSize(const size_t& FMQSize) noexcept {
    mProxyRecordFMQSize = FMQSize;
}

size_t Platform::getProxyRecordFMQSize() const noexcept {
    return mProxyRecordFMQSize;
}

uint32_t Platform::getWFDProxyChannels() const noexcept {
    return mWFDProxyChannels;
}

std::string Platform::IsProxyRecordActive()  const noexcept{
    int ret = 0;
    size_t size = 0;
    char proxy_record_state[6] = "false";
    ret = pal_get_param(PAL_PARAM_ID_PROXY_RECORD_SESSION, (void **)&proxy_record_state, &size,
                            nullptr);
    if (!ret && size > 0) {
        LOG(INFO) << __func__ << " proxyRecordActive = " << proxy_record_state;
    } else {
        LOG(ERROR) << __func__ << " : PAL_PARAM_ID_PROXY_RECORD_SESSION failed: " << ret;
    }
    return std::string(proxy_record_state);
}

void Platform::updateUHQA(const bool enable) noexcept {
    mIsUHQAEnabled = enable;
#ifndef SEC_AUDIO_SUPPORT_UHQ
    pal_param_uhqa_t paramUHQAFlags{.uhqa_state = mIsUHQAEnabled};
    if (int32_t ret =
                ::pal_set_param(PAL_PARAM_ID_UHQA_FLAG, &paramUHQAFlags, sizeof(pal_param_uhqa_t));
        ret) {
        LOG(ERROR) << __func__ << ": PAL_PARAM_ID_UHQA_FLAG failed: " << ret;
        return;
    }
#endif
    return;
}

#ifdef SEC_AUDIO_SUPPORT_UHQ
void Platform::updateUHQA(const pal_uhqa_state sample) noexcept {
    pal_param_uhqa_t paramInfo{.state = sample };
    mIsUHQAEnabled = (sample > PAL_UHQ_STATE_NORMAL) ? true : false;

    if (int32_t ret =
                ::pal_set_param(PAL_PARAM_ID_UHQA_FLAG, &paramInfo, sizeof(pal_param_uhqa_t));
        ret) {
        LOG(ERROR) << __func__ << ": PAL_PARAM_ID_UHQA_FLAG failed: " << ret;
        return;
    }
}
#endif

bool Platform::isUHQAEnabled() const noexcept {
    return mIsUHQAEnabled;
}

#ifdef SEC_AUDIO_SPEAKER_CALIBRATION
void Platform::triggerSpeakerCalibration() const noexcept {
    pal_param_cal_trigger_t param_cal_trigger{.enable = true};
    if (int32_t ret = ::pal_set_param(PAL_PARAM_ID_SEPAKER_AMP_RUN_CAL, &param_cal_trigger,
                                      sizeof(pal_param_cal_trigger_t));
        ret) {
        LOG(ERROR) << __func__ << ": PAL_PARAM_ID_SEPAKER_AMP_RUN_CAL failed: " << ret;
        return;
    }
}
#endif

void Platform::setFTMSpeakerProtectionMode(uint32_t const heatUpTime, uint32_t const runTime,
                                           bool const isFactoryTest, bool const isValidationMode,
                                           bool const isDynamicCalibration) const noexcept {
    pal_spkr_prot_payload spPayload{
            .spkrHeatupTime = heatUpTime, .operationModeRunTime = runTime,
    };

    if (isFactoryTest)
        spPayload.operationMode = PAL_SP_MODE_FACTORY_TEST;
    else if (isValidationMode)
        spPayload.operationMode = PAL_SP_MODE_V_VALIDATION;
    else if (isDynamicCalibration)
        spPayload.operationMode = PAL_SP_MODE_DYNAMIC_CAL;
    else
        return;

    if (int32_t ret =
                ::pal_set_param(PAL_PARAM_ID_SP_MODE, &spPayload, sizeof(pal_spkr_prot_payload));
        ret) {
        LOG(ERROR) << ": PAL_PARAM_ID_SP_MODE failed, ret:" << ret;
        return;
    }
}

std::optional<std::string> Platform::getFTMResult() const noexcept {
    char ftmValue[255];
    size_t dataSize = 0;
    if (int32_t ret = ::pal_get_param(PAL_PARAM_ID_SP_MODE, reinterpret_cast<void**>(&ftmValue),
                                      &dataSize, nullptr);
        (ret || dataSize <= 0)) {
        LOG(ERROR) << __func__ << ": PAL_PARAM_ID_SP_MODE failed, ret:" << ret
                   << ", data size:" << dataSize;
        return std::nullopt;
    }
    return std::string(ftmValue, dataSize);
}

std::optional<std::string> Platform::getSpeakerCalibrationResult() const noexcept {
    char calValue[255];
    size_t dataSize = 0;
    if (int32_t ret = ::pal_get_param(PAL_PARAM_ID_SP_GET_CAL, reinterpret_cast<void**>(&calValue),
                                      &dataSize, nullptr);
        (ret || dataSize <= 0)) {
        LOG(ERROR) << __func__ << ": PAL_PARAM_ID_SP_GET_CAL failed, ret:" << ret
                   << ", data size:" << dataSize;
        return std::nullopt;
    }
    return std::string(calValue, dataSize);
}

#ifdef ENABLE_TAS_SPK_PROT
std::optional<std::string> Platform::getSpeakerProtectionResult() const noexcept {
    std::string kvpairs = ";";
    if (!tiSpkProtParam.hdr[AUDIO_PARAM_TI_SMARTPA_GET_IDX]) {
        LOG(ERROR) << __func__ << " TI-SmartPA: get_without_set";
        return std::nullopt;
    } else {
        LOG(INFO) << __func__ << " TI-SmartPA: get ch=" << tiSpkProtParam.hdr[0]
                                << ", idx=0x" << std::hex << tiSpkProtParam.hdr[1]
                                << ", len=" << tiSpkProtParam.hdr[2];
        pal_tispk_prot_param_t *pTiSpkProtParam = &tiSpkProtParam;
        size_t size = 0;
        int32_t ret = pal_get_param(PAL_TISA_PARAM_GEN_GETPARAM,
                                    (void**)&pTiSpkProtParam, &size, nullptr);
        if (!ret) {
            std::string tiSpkProtParamStr;
            for (int i = 0; i < AUDIO_PARAM_HDR_LEN - 1; i++) {
                tiSpkProtParamStr = AUDIO_PARAM_TI_HDR[i];
                kvpairs += tiSpkProtParamStr + "="
                        + std::to_string(tiSpkProtParam.hdr[i]) + ";";
            }
            for (int i = 0; i < tiSpkProtParam.hdr[AUDIO_PARAM_TI_SMARTPA_LEN_IDX]; i++) {
                tiSpkProtParamStr = AUDIO_PARAM_TI_VIDX[i];
                kvpairs += tiSpkProtParamStr + "="
                        + std::to_string(tiSpkProtParam.data[i]) + ";";
            }
        }
    }
    memset(&tiSpkProtParam, 0, sizeof(tiSpkProtParam));
    return kvpairs;
}
#endif

void Platform::updateScreenRotation(const IModule::ScreenRotation in_rotation) noexcept {
    pal_param_device_rotation_t paramDeviceRotation{};

    auto notifyDeviceRotation = [&]() -> void {
        if (int32_t ret = ::pal_set_param(PAL_PARAM_ID_DEVICE_ROTATION, &paramDeviceRotation,
                                          sizeof(pal_param_device_rotation_t));
            ret) {
            LOG(ERROR) << ": PAL_PARAM_ID_DEVICE_ROTATION failed";
        }
        LOG(INFO) << ": updated screen rotation from "
                  << ::aidl::android::hardware::audio::core::toString(mCurrentScreenRotation)
                  << " to "
                  << ::aidl::android::hardware::audio::core::toString(
                             in_rotation); // validation log
    };

    if (in_rotation == IModule::ScreenRotation::DEG_270 &&
        mCurrentScreenRotation != IModule::ScreenRotation::DEG_270) {
        /* Device rotated from normal position to inverted landscape. */
        paramDeviceRotation.rotation_type = PAL_SPEAKER_ROTATION_RL;
        notifyDeviceRotation();
    } else if (in_rotation != IModule::ScreenRotation::DEG_270 &&
               mCurrentScreenRotation == IModule::ScreenRotation::DEG_270) {
        /* Phone was in inverted landspace and now is changed to portrait or inverted portrait. */
        paramDeviceRotation.rotation_type = PAL_SPEAKER_ROTATION_LR;
        notifyDeviceRotation();
    }

    // set for hdr params
    if (in_rotation == IModule::ScreenRotation::DEG_90 ||
        in_rotation == IModule::ScreenRotation::DEG_270) {
        setOrientation("landscape");
    } else {
        setOrientation("portrait");
    }

    if (in_rotation == IModule::ScreenRotation::DEG_270 ||
        in_rotation == IModule::ScreenRotation::DEG_180) {
        setInverted(true);
    } else {
        setInverted(false);
    }

    mCurrentScreenRotation = in_rotation;
}

IModule::ScreenRotation Platform::getCurrentScreenRotation() const noexcept {
    return mCurrentScreenRotation;
}

void Platform::setHapticsVolume(const float hapticsVolume) const noexcept {
    auto data = makePalVolumes({hapticsVolume});
    if (data.empty()) {
        LOG(ERROR) << __func__ << ": failed to configure haptics volume";
        return;
    }
    auto payloadPtr = reinterpret_cast<pal_volume_data*>(data.data());
    if (int32_t ret = ::pal_set_param(PAL_PARAM_ID_HAPTICS_VOLUME, payloadPtr, data.size()); ret) {
        LOG(ERROR) << __func__ << ": PAL_PARAM_ID_HAPTICS_VOLUME failed: " << ret;
        return;
    }
}

void Platform::setHapticsIntensity(const int hapticsIntensity) const noexcept {
    pal_param_haptics_intensity_t paramHapticsIntensity{.intensity = hapticsIntensity};
    if (int32_t ret = ::pal_set_param(PAL_PARAM_ID_HAPTICS_INTENSITY, &paramHapticsIntensity,
                                      sizeof(pal_param_haptics_intensity_t));
        ret) {
        LOG(ERROR) << __func__ << ": PAL_PARAM_ID_HAPTICS_INTENSITY failed: " << ret;
        return;
    }
}

#ifdef SEC_AUDIO_SUPPORT_HAPTIC_PLAYBACK
void Platform::setHapticsSource(const int hapticsSource) const noexcept {
    pal_param_haptic_source_t paramHapticsSource{.haptic_source = (haptic_source_t)hapticsSource};
    if (int32_t ret = ::pal_set_param(PAL_PARAM_ID_HAPTIC_SOURCE, (void*)&paramHapticsSource,
                                      sizeof(pal_param_haptic_source_t));
        ret) {
        LOG(ERROR) << __func__ << ": PAL_PARAM_ID_HAPTIC_SOURCE failed: " << ret;
        return;
    }
}
#endif

bool Platform::setVendorParameters(
        const std::vector<::aidl::android::hardware::audio::core::VendorParameter>& in_parameters,
        bool in_async) {
    std::string kvpairs = getkvPairsForVendorParameter(in_parameters);
    if (!kvpairs.empty()) {
        setBluetoothParameters(kvpairs.c_str());
#ifdef ENABLE_TAS_SPK_PROT
        setSpeakerProtectionParameters(kvpairs.c_str());
#endif
    }
    return true;
}

#ifdef ENABLE_TAS_SPK_PROT
bool Platform::setSpeakerProtectionParameters(const char* kvpairs) {
    struct str_parms* parms = NULL;
    int ret = 0, val = 0;
    char value[256];
    LOG(VERBOSE) << __func__ << "kvpairs " << kvpairs;
    parms = str_parms_create_str(kvpairs);
    ret = str_parms_get_str(parms, AUDIO_PARAM_TI_SMARTPA, value, sizeof(value));
    if (ret >= 0) {
        int i;
        int get = 0;
        int param_len;

        memset(&tiSpkProtParam, 0, sizeof(tiSpkProtParam));

        for (i = 0; i < AUDIO_PARAM_HDR_LEN; i++) {
            ret = str_parms_get_int(parms, AUDIO_PARAM_TI_HDR[i],
                    &tiSpkProtParam.hdr[i]);
            if (ret)
                break;
        }

        if (i < (AUDIO_PARAM_HDR_LEN - 1)) {
            ret = -EINVAL;
        } else if (i == (AUDIO_PARAM_HDR_LEN - 1)) {
            tiSpkProtParam.hdr[AUDIO_PARAM_TI_SMARTPA_GET_IDX] = 0;
            ret = 0;
        }

        if (!ret) {
            get = tiSpkProtParam.hdr[AUDIO_PARAM_TI_SMARTPA_GET_IDX];
            param_len = tiSpkProtParam.hdr[AUDIO_PARAM_TI_SMARTPA_LEN_IDX];

            LOG(INFO) << __func__
                      << " TI-SmartPA: setparam ch=" << tiSpkProtParam.hdr[0]
                      << ", idx=" << tiSpkProtParam.hdr[1]
                      << ", len=" << param_len
                      << ", get=" << (get ? 1 : 0);

            if (!get) {
                if (param_len > AUDIO_PARAM_MAX_LEN) {
                    ret = -EINVAL;
                } else {
                    for (i = 0; i < param_len; i++) {
                        ret = str_parms_get_int(parms, AUDIO_PARAM_TI_VIDX[i],
                                &tiSpkProtParam.data[i]);
                        if (ret)
                            break;
                    }
                    if (i == param_len) {
                        /* success */
                        ret = pal_set_param(PAL_TISA_PARAM_GEN_SETPARAM,
                                (void*)&tiSpkProtParam, sizeof(tiSpkProtParam));
                    } else {
                        LOG(ERROR) << __func__
                                   << " TI-SmartPA: Unable to extract all params";
                    }
                }
            }
        } else {
            LOG(INFO) << __func__
                      << " TI-SmartPA: invalid params, kvparis=" << kvpairs;
        }
    }
    if (parms)
        str_parms_destroy(parms);
    return true;
}
#endif

bool Platform::setBluetoothParameters(const char* kvpairs) {
    struct str_parms* parms = NULL;
    int ret = 0, val = 0;
    char value[256];
    LOG(VERBOSE) << __func__ << "kvpairs " << kvpairs;
    parms = str_parms_create_str(kvpairs);
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_RECONFIG_A2DP, value, sizeof(value));
    if (ret >= 0) {
        pal_param_bta2dp_t param_bt_a2dp;
        param_bt_a2dp.reconfig = true;

        LOG(VERBOSE) << __func__ << " BT A2DP Reconfig command received";
        pal_set_param(PAL_PARAM_ID_BT_A2DP_RECONFIG, (void*)&param_bt_a2dp,
                            sizeof(pal_param_bta2dp_t));
    }
    ret = str_parms_get_str(parms, "A2dpSuspended", value, sizeof(value));
    if (ret >= 0) {
        pal_param_bta2dp_t param_bt_a2dp;
        param_bt_a2dp.is_suspend_setparam = true;

        if (strncmp(value, "true", 4) == 0)
            param_bt_a2dp.a2dp_suspended = true;
        else
            param_bt_a2dp.a2dp_suspended = false;

        param_bt_a2dp.dev_id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;

        param_bt_a2dp.is_in_call = (mCallMode != AUDIO_MODE_NORMAL);

#ifdef SEC_AUDIO_BT_OFFLOAD
        param_bt_a2dp.is_bt_offload_enabled = audio_is_bt_offload_format(bt_a2dp_format);
#endif

        LOG(VERBOSE) << __func__ << " BT A2DP Suspended = " << value;
        std::unique_lock<std::mutex> guard(AudioExtension::reconfig_wait_mutex_);
        pal_set_param(PAL_PARAM_ID_BT_A2DP_SUSPENDED, (void*)&param_bt_a2dp,
                            sizeof(pal_param_bta2dp_t));
    }
    ret = str_parms_get_str(parms, "TwsChannelConfig", value, sizeof(value));
    if (ret >= 0) {
        pal_param_bta2dp_t param_bt_a2dp;

        LOG(VERBOSE) << __func__ << " Setting tws channel mode to = " << value;
        if (!(strncmp(value, "mono", strlen(value))))
            param_bt_a2dp.is_tws_mono_mode_on = true;
        else if (!(strncmp(value, "dual-mono", strlen(value))))
            param_bt_a2dp.is_tws_mono_mode_on = false;
        pal_set_param(PAL_PARAM_ID_BT_A2DP_TWS_CONFIG, (void*)&param_bt_a2dp,
                            sizeof(pal_param_bta2dp_t));
    }
    ret = str_parms_get_str(parms, "LEAMono", value, sizeof(value));
    if (ret >= 0) {
        pal_param_bta2dp_t param_bt_a2dp;

        LOG(VERBOSE) << __func__ << " Setting LC3 channel mode to = " << value;
        if (!(strncmp(value, "true", strlen(value))))
            param_bt_a2dp.is_lc3_mono_mode_on = true;
        else
            param_bt_a2dp.is_lc3_mono_mode_on = false;
        pal_set_param(PAL_PARAM_ID_BT_A2DP_LC3_CONFIG, (void*)&param_bt_a2dp,
                            sizeof(pal_param_bta2dp_t));
    }

#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    ret = str_parms_get_str(parms, "g_a2dp_delay_report", value, sizeof(value));
    if (ret >= 0) {
        pal_param_bta2dp_delay_report_t param_bt_a2dp_delay_report;
        param_bt_a2dp_delay_report.delay_report = atoi(value);
		LOG(VERBOSE) << __func__ << " BT A2DP delay report = " << param_bt_a2dp_delay_report.delay_report << ", command received";
        pal_set_param(PAL_PARAM_ID_BT_A2DP_DELAY_REPORT, (void *)&param_bt_a2dp_delay_report,
                        sizeof(pal_param_bta2dp_delay_report_t));
    }
#endif

    /* SCO parameters */
    ret = str_parms_get_str(parms, "BT_SCO", value, sizeof(value));
    if (ret >= 0) {
        pal_param_btsco_t param_bt_sco;
        memset(&param_bt_sco, 0, sizeof(pal_param_btsco_t));
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0) {
            param_bt_sco.bt_sco_on = true;
        } else {
            param_bt_sco.bt_sco_on = false;
        }

        LOG(VERBOSE) << __func__ << " BTSCO on = " << param_bt_sco.bt_sco_on;
        pal_set_param(PAL_PARAM_ID_BT_SCO, (void*)&param_bt_sco, sizeof(pal_param_btsco_t));
#ifdef SEC_AUDIO_BLUETOOTH
        setBtScoState(param_bt_sco.bt_sco_on);
#endif

#if 0
        if (param_bt_sco.bt_sco_on == true) {
            if (crs_device.size() == 0) {
                crs_device.insert(AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET);
                voice_->RouteStream(crs_device);
            } else {
                pos = std::find(crs_device.begin(), crs_device.end(), AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET);
                if (pos != crs_device.end()) {
                    AHAL_INFO("same device has added");
                } else {
                    crs_device.insert(AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET);
                    voice_->RouteStream({AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET});
                }
            }
        } else if (param_bt_sco.bt_sco_on == false) {
            pos = std::find(crs_device.begin(), crs_device.end(), AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET);
            if (pos != crs_device.end()) {
                crs_device.erase(pos);
                if (crs_device.size() >= 1) {
                    voice_->RouteStream(crs_device);
                    AHAL_INFO("route to device 0x%x", AudioExtn::get_device_types(crs_device));
                } else {
                    crs_device.clear();
                    voice_->RouteStream({AUDIO_DEVICE_OUT_SPEAKER});
                }
            }
        }
#endif
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_BT_SCO_WB, value, sizeof(value));
    if (ret >= 0) {
        pal_param_btsco_t param_bt_sco = {};
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0)
            param_bt_sco.bt_wb_speech_enabled = true;
        else
            param_bt_sco.bt_wb_speech_enabled = false;

        LOG(VERBOSE) << __func__ << " BTSCO WB mode = " << param_bt_sco.bt_wb_speech_enabled;
        pal_set_param(PAL_PARAM_ID_BT_SCO_WB, (void*)&param_bt_sco,
                            sizeof(pal_param_btsco_t));
    }
    ret = str_parms_get_str(parms, "bt_swb", value, sizeof(value));
    if (ret >= 0) {
        pal_param_btsco_t param_bt_sco = {};

        val = atoi(value);
        param_bt_sco.bt_swb_speech_mode = val;
        LOG(VERBOSE) << __func__ << " BTSCO SWB mode = " << val;
        pal_set_param(PAL_PARAM_ID_BT_SCO_SWB, (void*)&param_bt_sco,
                            sizeof(pal_param_btsco_t));
    }

    ret = str_parms_get_str(parms, "bt_ble", value, sizeof(value));
    if (ret >= 0) {
        pal_param_btsco_t param_bt_sco = {};
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0) {
            bt_lc3_speech_enabled = true;

            // turn off wideband, super-wideband
            param_bt_sco.bt_wb_speech_enabled = false;
            pal_set_param(PAL_PARAM_ID_BT_SCO_WB, (void*)&param_bt_sco,
                                sizeof(pal_param_btsco_t));

            param_bt_sco.bt_swb_speech_mode = 0xFFFF;
            pal_set_param(PAL_PARAM_ID_BT_SCO_SWB, (void*)&param_bt_sco,
                                sizeof(pal_param_btsco_t));
        } else {
            bt_lc3_speech_enabled = false;
            param_bt_sco.bt_lc3_speech_enabled = false;
            pal_set_param(PAL_PARAM_ID_BT_SCO_LC3, (void*)&param_bt_sco,
                                sizeof(pal_param_btsco_t));

            // clear btsco_lc3_cfg to avoid stale and partial cfg being used in next round
            memset(&btsco_lc3_cfg, 0, sizeof(btsco_lc3_cfg_t));
        }
        LOG(VERBOSE) << __func__ << " BTSCO LC3 mode = " << bt_lc3_speech_enabled;
    }

    ret = str_parms_get_str(parms, "bt_lc3_swb", value, sizeof(value));
    if (ret >= 0) {
        pal_param_btsco_t param_bt_sco_swb = {};
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0) {
            // turn off wideband, super-wideband
            param_bt_sco_swb.bt_wb_speech_enabled = false;
            pal_set_param(PAL_PARAM_ID_BT_SCO_WB, (void*)&param_bt_sco_swb,
                                sizeof(pal_param_btsco_t));

            param_bt_sco_swb.bt_swb_speech_mode = 0xFFFF;
            pal_set_param(PAL_PARAM_ID_BT_SCO_SWB, (void*)&param_bt_sco_swb,
                                sizeof(pal_param_btsco_t));

            char streamMap[PAL_LC3_MAX_STRING_LEN] = "(0, 0, M, 0, 1, M)";
            char vendor[PAL_LC3_MAX_STRING_LEN] = "00,00,00,00,00,00,00,00,00,02,00,00,00,0A,00,00";
            param_bt_sco_swb.bt_lc3_speech_enabled = true;
            param_bt_sco_swb.lc3_cfg.num_blocks = 1;
            param_bt_sco_swb.lc3_cfg.rxconfig_index = LC3_SWB_CODEC_CONFIG_INDEX;
            param_bt_sco_swb.lc3_cfg.txconfig_index = LC3_SWB_CODEC_CONFIG_INDEX;
            param_bt_sco_swb.lc3_cfg.api_version = 21;
            param_bt_sco_swb.lc3_cfg.mode = LC3_HFP_TRANSIT_MODE;
            strlcpy(param_bt_sco_swb.lc3_cfg.streamMap, streamMap, PAL_LC3_MAX_STRING_LEN);
            strlcpy(param_bt_sco_swb.lc3_cfg.vendor, vendor, PAL_LC3_MAX_STRING_LEN);

            LOG(VERBOSE) << __func__ << " BTSCO LC3 SWB mode = on, sending..";
            pal_set_param(PAL_PARAM_ID_BT_SCO_LC3, (void*)&param_bt_sco_swb,
                                sizeof(pal_param_btsco_t));
        } else {
            param_bt_sco_swb.bt_lc3_speech_enabled = false;

            LOG(VERBOSE) << __func__ << " BTSCO LC3 SWB mode = off, sending..";
            pal_set_param(PAL_PARAM_ID_BT_SCO_LC3, (void*)&param_bt_sco_swb,
                                sizeof(pal_param_btsco_t));
        }
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_BT_NREC, value, sizeof(value));
    if (ret >= 0) {
        pal_param_btsco_t param_bt_sco = {};
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0) {
            LOG(VERBOSE) << __func__ << " BTSCO NREC mode = ON";
            param_bt_sco.bt_sco_nrec = true;
        } else {
            LOG(VERBOSE) << __func__ << " BTSCO NREC mode = OFF";
            param_bt_sco.bt_sco_nrec = false;
        }
        pal_set_param(PAL_PARAM_ID_BT_SCO_NREC, (void*)&param_bt_sco,
                            sizeof(pal_param_btsco_t));
#ifdef SEC_AUDIO_BLUETOOTH
        setBtNrecState(param_bt_sco.bt_sco_nrec);
#endif
    }

    for (auto& key : lc3_reserved_params) {
        ret = str_parms_get_str(parms, key, value, sizeof(value));
        if (ret < 0) continue;

        if (!strcmp(key, "Codec") && (!strcmp(value, "LC3"))) {
            btsco_lc3_cfg.fields_map |= LC3_CODEC_BIT;
        } else if (!strcmp(key, "StreamMap")) {
            strlcpy(btsco_lc3_cfg.streamMap, value, PAL_LC3_MAX_STRING_LEN);
            btsco_lc3_cfg.fields_map |= LC3_STREAM_MAP_BIT;
        } else if (!strcmp(key, "FrameDuration")) {
            btsco_lc3_cfg.frame_duration = atoi(value);
            btsco_lc3_cfg.fields_map |= LC3_FRAME_DURATION_BIT;
        } else if (!strcmp(key, "Blocks_forSDU")) {
            btsco_lc3_cfg.num_blocks = atoi(value);
            btsco_lc3_cfg.fields_map |= LC3_BLOCKS_FORSDU_BIT;
        } else if (!strcmp(key, "rxconfig_index")) {
            btsco_lc3_cfg.rxconfig_index = atoi(value);
            btsco_lc3_cfg.fields_map |= LC3_RXCFG_IDX_BIT;
        } else if (!strcmp(key, "txconfig_index")) {
            btsco_lc3_cfg.txconfig_index = atoi(value);
            btsco_lc3_cfg.fields_map |= LC3_TXCFG_IDX_BIT;
        } else if (!strcmp(key, "version")) {
            btsco_lc3_cfg.api_version = atoi(value);
            btsco_lc3_cfg.fields_map |= LC3_VERSION_BIT;
        } else if (!strcmp(key, "vendor")) {
            strlcpy(btsco_lc3_cfg.vendor, value, PAL_LC3_MAX_STRING_LEN);
            btsco_lc3_cfg.fields_map |= LC3_VENDOR_BIT;
        }
    }

    if (((btsco_lc3_cfg.fields_map & LC3_BIT_MASK) == LC3_BIT_VALID) &&
        (bt_lc3_speech_enabled == true)) {
        pal_param_btsco_t param_bt_sco = {};
        param_bt_sco.bt_lc3_speech_enabled = bt_lc3_speech_enabled;
        param_bt_sco.lc3_cfg.frame_duration = btsco_lc3_cfg.frame_duration;
        param_bt_sco.lc3_cfg.num_blocks = btsco_lc3_cfg.num_blocks;
        param_bt_sco.lc3_cfg.rxconfig_index = btsco_lc3_cfg.rxconfig_index;
        param_bt_sco.lc3_cfg.txconfig_index = btsco_lc3_cfg.txconfig_index;
        param_bt_sco.lc3_cfg.api_version = btsco_lc3_cfg.api_version;
        param_bt_sco.lc3_cfg.mode = LC3_BROADCAST_TRANSIT_MODE;
        strlcpy(param_bt_sco.lc3_cfg.streamMap, btsco_lc3_cfg.streamMap, PAL_LC3_MAX_STRING_LEN);
        strlcpy(param_bt_sco.lc3_cfg.vendor, btsco_lc3_cfg.vendor, PAL_LC3_MAX_STRING_LEN);

        LOG(VERBOSE) << __func__ << " BTSCO LC3 mode = on, sending..";
        pal_set_param(PAL_PARAM_ID_BT_SCO_LC3, (void*)&param_bt_sco,
                            sizeof(pal_param_btsco_t));

        memset(&btsco_lc3_cfg, 0, sizeof(btsco_lc3_cfg_t));
    }
    ret = str_parms_get_str(parms, "A2dpCaptureSuspend", value, sizeof(value));
    if (ret >= 0) {
        pal_param_bta2dp_t param_bt_a2dp;
        param_bt_a2dp.is_suspend_setparam = true;

        if (strncmp(value, "true", 4) == 0)
            param_bt_a2dp.a2dp_capture_suspended = true;
        else
            param_bt_a2dp.a2dp_capture_suspended = false;

        param_bt_a2dp.dev_id = PAL_DEVICE_IN_BLUETOOTH_A2DP;

        param_bt_a2dp.is_in_call = (mCallMode != AUDIO_MODE_NORMAL);

        LOG(VERBOSE) << __func__ << " BT A2DP Capture Suspended " << value << "command received";
        std::unique_lock<std::mutex> guard(AudioExtension::reconfig_wait_mutex_);
        pal_set_param(PAL_PARAM_ID_BT_A2DP_CAPTURE_SUSPENDED, (void*)&param_bt_a2dp,
                            sizeof(pal_param_bta2dp_t));
    }
    ret = str_parms_get_str(parms, "LeAudioSuspended", value, sizeof(value));
    if (ret >= 0) {
        pal_param_bta2dp_t param_bt_a2dp;
        param_bt_a2dp.is_suspend_setparam = true;

        if (strcmp(value, "true") == 0) {
            param_bt_a2dp.a2dp_suspended = true;
            param_bt_a2dp.a2dp_capture_suspended = true;
        } else {
            param_bt_a2dp.a2dp_suspended = false;
            param_bt_a2dp.a2dp_capture_suspended = false;
        }

        param_bt_a2dp.is_in_call = (mCallMode != AUDIO_MODE_NORMAL);

        LOG(INFO) << __func__ << " BT LEA Suspended = ," << value << " command received";
        // Synchronize the suspend/resume calls from setparams and reconfig_cb
        std::unique_lock<std::mutex> guard(AudioExtension::reconfig_wait_mutex_);
        param_bt_a2dp.dev_id = PAL_DEVICE_OUT_BLUETOOTH_BLE;
        pal_set_param(PAL_PARAM_ID_BT_A2DP_SUSPENDED, (void*)&param_bt_a2dp,
                            sizeof(pal_param_bta2dp_t));

        param_bt_a2dp.dev_id = PAL_DEVICE_IN_BLUETOOTH_BLE;
        pal_set_param(PAL_PARAM_ID_BT_A2DP_CAPTURE_SUSPENDED, (void*)&param_bt_a2dp,
                            sizeof(pal_param_bta2dp_t));
        param_bt_a2dp.dev_id = PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST;
        pal_set_param(PAL_PARAM_ID_BT_A2DP_SUSPENDED, (void*)&param_bt_a2dp,
                            sizeof(pal_param_bta2dp_t));
    }
#ifdef SEC_AUDIO_COMMON
    if (parms)
        str_parms_destroy(parms);
#endif
    return true;
}

bool Platform::setParameter(const std::string& key, const std::string& value) {
    // Todo check for validity of key
    const auto & [ first, second ] = mParameters.insert_or_assign(key, value);
    LOG(VERBOSE) << __func__ << " platform parameter with key:" << key << " "
                 << (second ? "inserted" : "re-assigned") << " with value:" << value;
    return true;
}

std::string Platform::getParameter(const std::string& key) const {
    if (mParameters.find(key) != mParameters.cend()) {
        return mParameters.at(key);
    }
    return "";
}

#ifdef SEC_AUDIO_BT_OFFLOAD
bool Platform::isBluetoothA2dpDevice(const AudioDevice& d) const noexcept {
    if (d.type.connection == AudioDeviceDescription::CONNECTION_BT_A2DP) {
        return true;
    }
    return false;
}
#endif

bool Platform::isSoundCardUp() const noexcept {
    if (mSndCardStatus == CARD_STATUS_ONLINE) {
        return true;
    }
    return false;
}

bool Platform::isSoundCardDown() const noexcept {
    if (mSndCardStatus == CARD_STATUS_OFFLINE || mSndCardStatus == CARD_STATUS_STANDBY) {
        return true;
    }
    return false;
}


#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO
bool Platform::IsBtForMultiDevice(const std::vector<AudioDevice>& devices) const noexcept {
#ifdef SEC_AUDIO_BLUETOOTH
    if(bt_sco_on) {
        return true;
    }
#endif
#ifdef SEC_AUDIO_BLUETOOTH
    for (auto& device : devices) {
        const auto palDeviceId = PlatformConverter::getPalDeviceId(device.type);
        if (palDeviceId == PAL_DEVICE_IN_BLUETOOTH_BLE) {
            return true;
        }
    }
#endif
    return false;
}
#endif

uint32_t Platform::getBluetoothLatencyMs(const std::vector<AudioDevice>& bluetoothDevices) {
    pal_param_bta2dp_t btConfig{};
    pal_param_bta2dp_t *param_bt_a2dp_ptr = &btConfig;

    for (const auto& device : bluetoothDevices) {
        size_t payloadSize = 0;
        param_bt_a2dp_ptr->dev_id = PlatformConverter::getPalDeviceId(device.type);
        // first bluetooth device
        if (param_bt_a2dp_ptr->dev_id == PAL_DEVICE_OUT_BLUETOOTH_A2DP ||
            param_bt_a2dp_ptr->dev_id == PAL_DEVICE_OUT_BLUETOOTH_BLE ||
            param_bt_a2dp_ptr->dev_id == PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST) {
            if (int32_t ret = ::pal_get_param(PAL_PARAM_ID_BT_A2DP_ENCODER_LATENCY,
                             reinterpret_cast<void**>(&param_bt_a2dp_ptr), &payloadSize, nullptr);
                ret) {
                LOG(ERROR) << __func__ << " failure in PARAM_ID_BT_A2DP_ENCODER_LATENCY: " << ret;
                continue;
            }
            if (payloadSize == 0) {
                LOG(ERROR) << __func__ << " empty payload size!!!";
                continue;
            }
        }
    }
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    LOG(VERBOSE) << __func__ << " bt latency: " << param_bt_a2dp_ptr->latency;
#else
    LOG(DEBUG) << __func__ << " bt latency: " << param_bt_a2dp_ptr->latency;
#endif
    return param_bt_a2dp_ptr->latency;
}

bool Platform::isA2dpSuspended() {
    int ret = 0;
    size_t bt_param_size = 0;
    pal_param_bta2dp_t *param_bt_a2dp_ptr, param_bt_a2dp;
    param_bt_a2dp_ptr = &param_bt_a2dp;
    param_bt_a2dp_ptr->dev_id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
    ret = pal_get_param(PAL_PARAM_ID_BT_A2DP_SUSPENDED, (void**)&param_bt_a2dp_ptr, &bt_param_size,
                        nullptr);
    if (!ret && bt_param_size && param_bt_a2dp_ptr && !param_bt_a2dp_ptr->a2dp_suspended) {
        LOG(DEBUG) << __func__ << " A2dp suspended " << param_bt_a2dp_ptr->a2dp_suspended;
        return param_bt_a2dp_ptr->a2dp_suspended;
    }
    return true;
}

PlaybackRateStatus Platform::setPlaybackRate(
        pal_stream_handle_t* handle, const Usecase& tag,
        const ::aidl::android::media::audio::common::AudioPlaybackRate& playbackRate) {
    if (!isValidPlaybackRate(playbackRate)) {
        return PlaybackRateStatus::ILLEGAL_ARGUMENT;
    }

    if (!usecaseSupportsOffloadSpeed(tag)) {
        return PlaybackRateStatus::UNSUPPORTED;
    }

    if (!handle) {
        LOG(DEBUG) << __func__ << " stream inactive ";
        return PlaybackRateStatus::SUCCESS;
    }

    auto allocSize = sizeof(pal_param_payload) + sizeof(pal_param_playback_rate_t);
    auto payload =
            VALUE_OR_EXIT(allocate<pal_param_payload>(allocSize), PlaybackRateStatus::UNSUPPORTED);
    pal_param_payload* payloadPtr = payload.get();
    payloadPtr->payload_size = sizeof(pal_param_playback_rate_t);

    auto palPlaybackRatePtr = reinterpret_cast<pal_param_playback_rate_t*>(payloadPtr->payload);
    palPlaybackRatePtr->speed = playbackRate.speed;
    palPlaybackRatePtr->pitch = playbackRate.pitch;

    if (auto ret = pal_stream_set_param(handle, PAL_PARAM_ID_TIMESTRETCH_PARAMS, payloadPtr); ret) {
        LOG(ERROR) << __func__ << " failed to set " << playbackRate.toString();
        return PlaybackRateStatus::UNSUPPORTED;
    }
    return PlaybackRateStatus::SUCCESS;
}

int Platform::getRecommendedLatencyModes(
          std::vector<::aidl::android::media::audio::common::AudioLatencyMode>* _aidl_return) {

     size_t size;
     int ret = 0;
     auto palLatencyModeInfo = std::make_unique<pal_param_latency_mode_t>();
     if (!palLatencyModeInfo) {
         LOG(ERROR) << __func__ << ": allocation failed ";
         return -ENOMEM;
     }

     palLatencyModeInfo->dev_id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
     palLatencyModeInfo->num_modes = PAL_MAX_LATENCY_MODES;
     void *palLatencyModeInfoPtr = palLatencyModeInfo.get();

     ret = pal_get_param(PAL_PARAM_ID_LATENCY_MODE,
                        (void **)&palLatencyModeInfoPtr, &size, nullptr);
     if (ret) {
         LOG(ERROR) << __func__ << " get param latency mode failed";
         return ret;
     }

     LOG(VERBOSE) << __func__ << " actual modes returned: " << palLatencyModeInfo->num_modes;

     for (int count = 0; count < palLatencyModeInfo->num_modes; count++)
     {
        _aidl_return->push_back(
         (::aidl::android::media::audio::common::AudioLatencyMode)palLatencyModeInfo->modes[count]);
     }

     return ret;
}

bool Platform::isHDRARMenabled() {
    const auto& platform = Platform::getInstance();
    const std::string kHdrArmProperty{"vendor.audio.hdr.record.enable"};
    const bool isArmEnabled = ::android::base::GetBoolProperty(kHdrArmProperty, false);
    const bool isHdrSetOnPlatform = platform.isHDREnabled();
    if (isArmEnabled && isHdrSetOnPlatform) {
        return true;
    }
    return false;

}
bool Platform::isHDRSPFEnabled() {
    const std::string kHdrSpfProperty{"vendor.audio.hdr.spf.record.enable"};
    const bool isSPFEnabled = ::android::base::GetBoolProperty(kHdrSpfProperty, false);
    if (isSPFEnabled) {
        return true;
    }
    return false;
}

void Platform::setHdrOnPalDevice(pal_device* palDeviceIn) {
    const auto& platform = Platform::getInstance();
    const bool isOrientationLandscape = platform.getOrientation() == "landscape";
    const bool isInverted = platform.isInverted();

    LOG(ERROR) << __func__ << " platform.getOrientation():" << std::string(platform.getOrientation());

    if (isOrientationLandscape && !isInverted) {
        setPalDeviceCustomKey(*palDeviceIn, "unprocessed-hdr-mic-landscape");
    } else if (!isOrientationLandscape && !isInverted) {
        setPalDeviceCustomKey(*palDeviceIn, "unprocessed-hdr-mic-portrait");
    } else if (isOrientationLandscape && isInverted) {
        setPalDeviceCustomKey(*palDeviceIn, "unprocessed-hdr-mic-inverted-landscape");
    } else if (!isOrientationLandscape && isInverted) {
        setPalDeviceCustomKey(*palDeviceIn, "unprocessed-hdr-mic-inverted-portrait");
    }
    LOG(DEBUG) << __func__
               << " setting custom config:" << std::string(palDeviceIn->custom_config.custom_key);

}

void Platform::configurePalDevices(
        const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig,
        std::vector<pal_device>& palDevices) {
    const auto& mixUsecase =
            mixPortConfig.ext.get<::aidl::android::media::audio::common::AudioPortExt::Tag::mix>()
                    .usecase;
    if (mixUsecase.getTag() !=
        ::aidl::android::media::audio::common::AudioPortMixExtUseCase::Tag::source) {
        LOG(ERROR) << __func__ << " expected mix usecase as source instead found, "
                   << mixUsecase.toString();
        return;
    }
    const auto& sampleRate = mixPortConfig.sampleRate.value().value;
    const auto& channelLayout = mixPortConfig.channelMask.value();
    const ::aidl::android::media::audio::common::AudioSource& audioSourceType = mixUsecase.get<
            ::aidl::android::media::audio::common::AudioPortMixExtUseCase::Tag::source>();
    const bool isSourceUnprocessed =
            audioSourceType == ::aidl::android::media::audio::common::AudioSource::UNPROCESSED;
    const bool isSourceCamCorder =
            audioSourceType == ::aidl::android::media::audio::common::AudioSource::CAMCORDER;
    const bool isMic = audioSourceType == ::aidl::android::media::audio::common::AudioSource::MIC;
    const bool isHdrArmEnable = isHDRARMenabled();
    const bool isHdrSpfEnable = isHDRSPFEnabled();
    if ((isSourceUnprocessed && sampleRate == 48000 && getChannelCount(channelLayout) == 4 &&
         isHdrArmEnable) ||
        (isHdrArmEnable) || (isHdrSpfEnable && (isSourceCamCorder || isMic))) {
        std::for_each(palDevices.begin(), palDevices.end(),
                      [&](auto& palDevice) { this->setHdrOnPalDevice(&palDevice); });
    }
}

int Platform::setLatencyMode(uint32_t mode) {

     int ret = 0;
     auto palLatencyModeInfo = std::make_unique<pal_param_latency_mode_t>();
     if (!palLatencyModeInfo) {
         LOG(ERROR) << __func__ << ": allocation failed ";
         return -ENOMEM;
     }

     palLatencyModeInfo->dev_id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
     palLatencyModeInfo->num_modes = 1;
     palLatencyModeInfo->modes[0] = (uint32_t)mode;

     ret = pal_set_param(PAL_PARAM_ID_LATENCY_MODE,
              (void *)palLatencyModeInfo.get(), sizeof(pal_param_latency_mode_t));

     return ret;
}

std::optional<std::pair<audio_format_t, audio_format_t>> Platform::requiresBufferReformat(
        const AudioPortConfig& portConfig) {
    const auto& audioFormat = portConfig.format.value();

    if (audioFormat.pcm == PcmType::FLOAT_32_BIT) {
        return std::make_pair(AUDIO_FORMAT_PCM_FLOAT, AUDIO_FORMAT_PCM_32_BIT);
    }
    return std::nullopt;
}

std::string Platform::toString() const {
    std::ostringstream os;
    os << " === platform start ===" << std::endl;
    os << "sound card status: " << mSndCardStatus << std::endl;
    for (const auto & [ key, value ] : mParameters) {
        os << key << "=>" << value << std::endl;
    }
    os << PlatformConverter::toString() << std::endl;
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    os << toStringSec();
#endif
    os << " === platform end ===" << std::endl;
    return os.str();
}

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
std::string Platform::toStringSec() const {
    std::ostringstream os;
#ifdef SEC_AUDIO_BLUETOOTH
    os << "  bt_sco_on : " << (bt_sco_on? "on" : "off") << std::endl;
#endif
#ifdef SEC_AUDIO_CALL
    os << "  mIsVoWiFi : " << (mIsVoWiFi? "on" : "off");
#ifdef SEC_AUDIO_WB_AMR
    os << ", mCallBand : " << mCallBand << std::endl;
#endif
    os << "  mRingbacktone : " << (mRingbacktone? "on" : "off") << std::endl;
    os << "  mNbQuality : " << (mNbQuality? "on" : "off") << std::endl;
    if (mCallMode == AUDIO_MODE_IN_CALL) {
        os << "  Mute Voice : RX " << (mVoiceMuteState[PAL_RX]? "mute":"unmute") <<
                           ", TX " << (mVoiceMuteState[PAL_TX]? "mute":"unmute ") << std::endl;
    }
    os << "  mMicMuted : " << (mMicMuted? "on" : "off") << std::endl;
#endif
#ifdef SEC_AUDIO_ALL_SOUND_MUTE
    os << "  mAllSoundMute : " << (mAllSoundMute? "on" : "off") << std::endl;
#endif
#ifdef SEC_AUDIO_CALL_FORWARDING
    os << "  mCallMemo : " << mCallMemo << std::endl;
    os << "  mCallForwarding : " << (mCallForwarding? "on" : "off") << std::endl;
#endif
#ifdef SEC_AUDIO_VOICE_TX_FOR_INCALL_MUSIC
    os << "  mScreenCall : " << (mScreenCall? "on" : "off") << std::endl;
#endif
#ifdef SEC_AUDIO_CALL_TRANSLATION
    os << "  mCallTranslation : " << (mCallTranslation? "on" : "off") << std::endl;
    os << "  mVoiceRxControl : " << mVoiceRxControl << std::endl;
#endif
#ifdef SEC_AUDIO_ENFORCED_AUDIBLE
    os << "  mEnforcePlaybackState : " <<  mEnforcePlaybackState << std::endl;
#endif
#ifdef SEC_AUDIO_CALL_HAC
    os << "  mHacIncall : " << (mHacIncall? "on" : "off");
    os << ", mHacMode : " << mHacMode << std::endl;
#endif
#ifdef SEC_AUDIO_SCREEN_MIRRORING // { SUPPORT_VOIP_VIA_SMART_VIEW
    os << "  mVoipViaSmartView : " << (mVoipViaSmartView? "on" : "off") << std::endl;
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW
#ifdef SEC_AUDIO_FMRADIO
    os << "  mFM : " << (mFM.on? "on" : "off") << (mFM.mute? " (mute)" : "") << std::endl;
#endif
#ifdef SEC_AUDIO_SUPPORT_NSRI
    os << "  is_NSRI_secure : " << (is_NSRI_secure? "on" : "off") << std::endl;
#endif
#ifdef SEC_AUDIO_SUPPORT_REMOTE_MIC
    os << "  mAasEnabled : " << (mAasEnabled? "on" : "off") << std::endl;
#endif
    os << std::endl;
    return os.str();
}

void Platform::dump(int fd) const {
    pal_dump(fd);
}
#endif

// static
int Platform::palGlobalCallback(uint32_t event_id, uint32_t* event_data, uint64_t cookie) {
    auto platform = reinterpret_cast<Platform*>(cookie);
    switch (event_id) {
        case PAL_SND_CARD_STATE:
            platform->mSndCardStatus = static_cast<card_status_t>(*event_data);
            LOG(INFO) << __func__ << " card status changed to " << platform->mSndCardStatus;
            break;
        default:
            LOG(ERROR) << __func__ << " invalid event id" << event_id;
            return -EINVAL;
    }
    return 0;
}

void Platform::initUsecaseOpMap() {
    mUsecaseOpMap[Usecase::PRIMARY_PLAYBACK] = makeUsecaseOps<PrimaryPlayback>();
    mUsecaseOpMap[Usecase::LOW_LATENCY_PLAYBACK] = makeUsecaseOps<LowLatencyPlayback>();
    mUsecaseOpMap[Usecase::DEEP_BUFFER_PLAYBACK] = makeUsecaseOps<DeepBufferPlayback>();
    mUsecaseOpMap[Usecase::ULL_PLAYBACK] = makeUsecaseOps<UllPlayback>();
    mUsecaseOpMap[Usecase::MMAP_PLAYBACK] = makeUsecaseOps<MMapPlayback>();
    mUsecaseOpMap[Usecase::COMPRESS_OFFLOAD_PLAYBACK] = makeUsecaseOps<CompressPlayback>();
    mUsecaseOpMap[Usecase::PCM_OFFLOAD_PLAYBACK] = makeUsecaseOps<PcmOffloadPlayback>();
    mUsecaseOpMap[Usecase::VOIP_PLAYBACK] = makeUsecaseOps<VoipPlayback>();
    mUsecaseOpMap[Usecase::HAPTICS_PLAYBACK] = makeUsecaseOps<HapticsPlayback>();
    mUsecaseOpMap[Usecase::SPATIAL_PLAYBACK] = makeUsecaseOps<SpatialPlayback>();
    mUsecaseOpMap[Usecase::IN_CALL_MUSIC] = makeUsecaseOps<InCallMusic>();

    // Record usecases
    mUsecaseOpMap[Usecase::PCM_RECORD] = makeUsecaseOps<PcmRecord>();
    mUsecaseOpMap[Usecase::FAST_RECORD] = makeUsecaseOps<FastRecord>();
    mUsecaseOpMap[Usecase::ULTRA_FAST_RECORD] = makeUsecaseOps<UltraFastRecord>();
    mUsecaseOpMap[Usecase::MMAP_RECORD] = makeUsecaseOps<MMapRecord>();
    mUsecaseOpMap[Usecase::COMPRESS_CAPTURE] = makeUsecaseOps<CompressCapture>();
    mUsecaseOpMap[Usecase::VOIP_RECORD] = makeUsecaseOps<VoipRecord>();
    mUsecaseOpMap[Usecase::VOICE_CALL_RECORD] = makeUsecaseOps<VoiceCallRecord>();
    mUsecaseOpMap[Usecase::HOTWORD_RECORD] = makeUsecaseOps<HotwordRecord>();
}

std::vector<MicrophoneDynamicInfo> Platform::getMicrophoneDynamicInfo(
        const std::vector<AudioDevice>& devices) {
    auto palDevices = convertToPalDevices(devices);
    std::vector<MicrophoneDynamicInfo> result;
    for (const auto& palDevice : palDevices) {
        auto id = palDevice.id;
        if (mMicrophoneDynamicInfoMap.count(id) != 0) {
            auto dynamicInfo = mMicrophoneDynamicInfoMap[id];
            result.insert(result.end(), dynamicInfo.begin(), dynamicInfo.end());
        }
    }
    return result;
}

#ifdef SEC_AUDIO_SAMSUNGRECORD
int Platform::match_device_enums(const AudioDevice& device) const noexcept {
    int ret = 0;
    if (device.type.type == AudioDeviceType::IN_MICROPHONE) { // AUDIO_DEVICE_IN_BUILTIN_MIC
        ret = AUDIO_DEVICE_IN_BUILTIN_MIC;
    } else if (device.type.type == AudioDeviceType::IN_MICROPHONE_BACK) { // AUDIO_DEVICE_IN_BACK_MIC
        ret = AUDIO_DEVICE_IN_BACK_MIC;
    } else if (device.type.type == AudioDeviceType::IN_HEADSET &&
                device.type.connection == AudioDeviceDescription::CONNECTION_BT_SCO) { // AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET
        ret = AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
    } else if (device.type.type == AudioDeviceType::IN_MICROPHONE_MULTI) { // AUDIO_DEVICE_IN_2MIC
        ret = AUDIO_DEVICE_IN_2MIC;
    } else if (device.type.type == AudioDeviceType::IN_HEADSET &&
                device.type.connection == AudioDeviceDescription::CONNECTION_ANALOG) { // AUDIO_DEVICE_IN_WIRED_HEADSET
        ret = AUDIO_DEVICE_IN_WIRED_HEADSET;
    }else if (device.type.type == AudioDeviceType::IN_ACCESSORY &&
                device.type.connection == AudioDeviceDescription::CONNECTION_USB) { // AUDIO_DEVICE_IN_USB_ACCESSORY
        ret = AUDIO_DEVICE_IN_USB_ACCESSORY;
    } else if (device.type.type == AudioDeviceType::IN_DEVICE &&
                device.type.connection == AudioDeviceDescription::CONNECTION_USB) { // AUDIO_DEVICE_IN_USB_DEVICE
        ret = AUDIO_DEVICE_IN_USB_DEVICE;
    } else if (device.type.type == AudioDeviceType::IN_HEADSET &&
                device.type.connection == AudioDeviceDescription::CONNECTION_USB) { // AUDIO_DEVICE_IN_USB_HEADSET
        ret = AUDIO_DEVICE_IN_USB_HEADSET;
    } else if (device.type.type == AudioDeviceType::IN_HEADSET &&
                device.type.connection == AudioDeviceDescription::CONNECTION_BT_LE) { // AUDIO_DEVICE_IN_BLE_HEADSET
        ret = AUDIO_DEVICE_IN_BLE_HEADSET;
    } else {
        ret = 0;
    }
    return ret;
}

int Platform::get_device_types(const std::vector<AudioDevice>& devices) const noexcept {
    int device = 0;
    for(auto itr = devices.cbegin(); itr < devices.cend(); itr++) {
        device |= match_device_enums(*itr);
    }
    return device;
}

bool Platform::GetRecMultiMic(const AudioPortConfig& mixPortConfig, const std::vector<AudioDevice>& connectedDevices, Usecase tag) const noexcept {
    bool state = false;
    audio_devices_t devices = static_cast<audio_devices_t>(get_device_types(connectedDevices));
    // 4CH Supported Devices :
    //  2mic(or buildin)
    //  BTMIX (2mic+bt)
    //  switching BTMIX : usb->btmix, ble->btmix
    if (!audio_is_subset_device(devices, AUDIO_DEVICE_IN_2MIC)
#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO
        && !(multidevice_rec
            && (audio_is_subset_device(devices, AUDIO_DEVICE_IN_BLE_HEADSET)
#if (SEC_AUDIO_MULTI_MIC == 0)
                || audio_is_usb_in_device(devices)
#endif
            ))
#endif
        ) {
        return state;
    }

#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO
    // not support releasing BTMIX : btmix(2mic+ble) -> no btmix
    if (!multidevice_rec
        && audio_is_subset_device(devices, AUDIO_DEVICE_IN_BLE_HEADSET)
        && audio_is_subset_device(devices, AUDIO_DEVICE_IN_2MIC)) {
        return state;
    }
#endif

    const auto& source = getAudioSource(mixPortConfig);

    // Supported Scenario
    if (IsSupportPreprocess(mixPortConfig, tag)
        && tag == Usecase::PCM_RECORD
        && !is_karaoke_on
        && mIsLoopBackOff
#if (SEC_AUDIO_MULTI_MIC == 0) && defined(SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO)
        && multidevice_rec
#endif
        && source && (source.value() == AudioSource::CAMCORDER)) {
        state = true;
    }
    return state;
}

bool Platform::IsSupportPreprocess(const AudioPortConfig& mixPortConfig, Usecase tag) const noexcept {
    bool ret = false;

    if (!SelectPreProcessSolutions(mixPortConfig)) {
        return ret;
    }
    const auto& sampleRate = getSampleRate(mixPortConfig);
    if (!(sampleRate && sampleRate.value() == 48000
            && (mixPortConfig.flags && hasInputFastFlag(mixPortConfig.flags.value())) != 0)
        && !mIsRmsTestMode
        && tag != Usecase::VOIP_RECORD) {
        ret = true;
    }
    return ret;
}

uint32_t Platform::SelectPreProcessSolutions(const AudioPortConfig& mixPortConfig) const noexcept {
    uint32_t solutions = S_NONE;
    const auto& source = getAudioSource(mixPortConfig);
    auto& flags = mixPortConfig.flags.value();

    if ((mCallMode == AUDIO_MODE_IN_CALL)
        || (mCallMode== AUDIO_MODE_IN_COMMUNICATION)
        || (mCallMode == AUDIO_MODE_CALL_SCREEN)) {
        return solutions;
    }

#ifdef SEC_AUDIO_COMPRESS_CAPTURE
    if (hasInputDirectFlag(flags)) {
        return solutions;
    }
#endif

    if (source) {
        int src = static_cast<int>(source.value());
        if ((src == static_cast<int>(AudioSource::MIC))
            || (src == static_cast<int>(AudioSource::CAMCORDER))
            || (src == static_cast<int>(AudioSource::VOICE_PERFORMANCE) + 7)) { // SEC_CAMCORDER
            solutions |= preprocess_eq_enables;
        } else if (src == static_cast<int>(AudioSource::VOICE_PERFORMANCE) + 8) { // SEC_BEAMFORMING
            solutions |= S_REC_BF;
        }
    }
#ifdef SEC_AUDIO_RECORDALIVE_ON_REMOTE_MIC
    //remote mic solution should be attached on recordalive supported source
    if (hasInputRemoteMicFlag(flags) && solutions) {
        solutions |= S_REC_REMOTE_MIC;
    }
#endif

    return solutions;
}

uint32_t Platform::GetBufferSize(const AudioPortConfig& mixPortConfig) {
    const auto& sampleRate = mixPortConfig.sampleRate.value().value;
    const auto& frameSize = getFrameSizeInBytes(mixPortConfig.format.value(), mixPortConfig.channelMask.value());
    uint32_t ret = (mixPortConfig.sampleRate.value().value / 1000) * AUDIO_CAPTURE_PERIOD_DURATION_MSEC *
                    frameSize;
    LOG(DEBUG) << __func__ << " update buffer size : " << ret;
    return ret;
}

int Platform::GetRecFormat(const AudioPortConfig& mixPortConfig, const std::vector<AudioDevice>& connectedDevices, Usecase tag) {
    const auto& source = getAudioSource(mixPortConfig);
    int formats = 0;
    audio_devices_t devices = static_cast<audio_devices_t>(get_device_types(connectedDevices));
    // 24bit Supported Devices :
    //  2mic(or buildin), usb, 3.5pi
    //  BTMIX (2mic+bt)
    //  switching BTMIX : ble->btmix
    if (!audio_is_usb_in_device(devices)
        && !audio_is_subset_device(devices, AUDIO_DEVICE_IN_WIRED_HEADSET)
        && !audio_is_subset_device(devices, AUDIO_DEVICE_IN_2MIC)
#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO
        && !(multidevice_rec
            && audio_is_subset_device(devices, AUDIO_DEVICE_IN_BLE_HEADSET))
#endif
        ) {
        return formats;
    }

#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO
    // not support releasing BTMIX : btmix(2mic+ble) -> no btmix
    if (!multidevice_rec
        && audio_is_subset_device(devices, AUDIO_DEVICE_IN_BLE_HEADSET)
        && audio_is_subset_device(devices, AUDIO_DEVICE_IN_2MIC)) {
        return formats;
    }
#endif

    // Supported Scenario
    if(isLoopBackOff()) {
        if (mixPortConfig.format.value().pcm == PcmType::INT_16_BIT
            && IsSupportPreprocess(mixPortConfig, tag)
            && tag == Usecase::PCM_RECORD
            && source && (source.value() == AudioSource::MIC || source.value() == AudioSource::CAMCORDER)) {
            formats = AUDIO_SUPPORTED_FORMAT_24;
        }
    }
    return formats;
}
#endif

#ifdef SEC_AUDIO_KARAOKE
bool Platform::isKaraokeUsecases(const Usecase& tag) {
    switch (tag) {
        case Usecase::LOW_LATENCY_PLAYBACK:
        case Usecase::DEEP_BUFFER_PLAYBACK:
        case Usecase::ULL_PLAYBACK:
            return true;
        default:
            return false;
    }
}
#endif

#if defined(SEC_AUDIO_OFFLOAD_COMPRESSED_OPUS) && defined(SEC_AUDIO_OFFLOAD_SOUNDSPEED)
PlaybackRateStatus Platform::setSecPlaybackRate(pal_stream_handle_t* handle, const Usecase& tag,
        const ::aidl::android::media::audio::common::AudioPlaybackRate& playbackRate,
        const std::optional< ::aidl::android::media::audio::common::AudioOffloadInfo>& offloadInfo
        ,std::function<void(const float&)> sendSpeed) {
    if (!isValidPlaybackRate(playbackRate)) {
        return PlaybackRateStatus::ILLEGAL_ARGUMENT;
    }

    if (!usecaseSupportsOffloadSpeed(tag)) {
        return PlaybackRateStatus::UNSUPPORTED;
    }

    if (!isSecSupportsOffloadSpeed(offloadInfo)) {
        return PlaybackRateStatus::UNSUPPORTED;
    }

    if (!handle) {
        LOG(DEBUG) << __func__ << " stream inactive ";
        return PlaybackRateStatus::SUCCESS;
    }

    sendSpeed(playbackRate.speed);

    return PlaybackRateStatus::SUCCESS;

}

bool Platform::isSecSupportsOffloadSpeed(const std::optional< ::aidl::android::media::audio::common::AudioOffloadInfo>& offloadInfo) {
    if (!offloadInfo.has_value()) {
        return false;
    }

    if (offloadInfo.has_value() && offloadInfo.value().base.format.encoding != ::android::MEDIA_MIMETYPE_AUDIO_OPUS) {
        return false;
    }

    return true;
}
#endif

Platform::Platform() {
    initUsecaseOpMap();
    if (int32_t ret = pal_init(); ret) {
        LOG(ERROR) << __func__ << "pal_init failed, ret:" << ret;
        return;
    }
    LOG(VERBOSE) << __func__ << " pal_init successful";
    if (int32_t ret =
                pal_register_global_callback(&palGlobalCallback, reinterpret_cast<uint64_t>(this));
        ret) {
        LOG(ERROR) << __func__ << "pal register global callback failed, ret:" << ret;
        return;
    }
    mSndCardStatus = CARD_STATUS_ONLINE;
    LOG(VERBOSE) << __func__ << " pal register global callback successful";
    mOffloadSpeedSupported = property_get_bool("vendor.audio.offload.playspeed", true);
    MicrophoneInfoParser micInfoParser;
    mMicrophoneInfo = micInfoParser.getMicrophoneInfo();
    mMicrophoneDynamicInfoMap = micInfoParser.getMicrophoneDynamicInfoMap();
}

// static
Platform& Platform::getInstance() {
    static const auto kPlatform = []() {
        std::unique_ptr<Platform> platform{new Platform()};
        return std::move(platform);
    }();
    return *(kPlatform.get());
}

} // namespace qti::audio::core
