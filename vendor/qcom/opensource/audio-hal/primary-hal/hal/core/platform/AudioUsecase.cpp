/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#define LOG_TAG "AHAL_Usecase_QTI"


#include <aidl/qti/audio/core/VString.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <media/stagefright/foundation/MediaDefs.h>
#include <qti-audio-core/AudioUsecase.h>
#include <qti-audio-core/Platform.h>
#include <qti-audio-core/PlatformUtils.h>
#include <qti-audio-core/PlatformStreamCallback.h>
#include <qti-audio-core/Utils.h>

using ::aidl::android::media::audio::common::AudioIoFlags;
using ::aidl::android::media::audio::common::AudioInputFlags;
using ::aidl::android::media::audio::common::AudioOutputFlags;
using ::aidl::android::media::audio::common::AudioSource;
using ::aidl::android::media::audio::common::AudioStreamType;
using ::aidl::android::hardware::audio::common::isBitPositionFlagSet;
using ::aidl::android::hardware::audio::common::getChannelCount;
using ::aidl::android::hardware::audio::common::getFrameSizeInBytes;
using ::aidl::android::hardware::audio::common::getPcmSampleSizeInBytes;
using ::aidl::android::media::audio::common::AudioPortConfig;
using ::aidl::android::media::audio::common::AudioPortExt;
using ::aidl::android::media::audio::common::AudioPortMixExtUseCase;
using ::aidl::android::hardware::audio::core::VendorParameter;
using ::aidl::android::media::audio::common::AudioChannelLayout;
using ::aidl::android::hardware::audio::common::AudioOffloadMetadata;

namespace qti::audio::core {

#ifdef SEC_AUDIO_COMMON
std::optional<AudioSource> getMixPortAudioSource(
        const AudioPortConfig& mixPortconfig) noexcept {
    if (mixPortconfig.ext.getTag() != AudioPortExt::Tag::mix) {
        LOG(ERROR) << __func__ << ": not a mix port, " << mixPortconfig.toString();
        return std::nullopt;
    }
    if (mixPortconfig.ext.get<AudioPortExt::Tag::mix>().usecase.getTag() !=
        AudioPortMixExtUseCase::Tag::source) {
        LOG(ERROR) << __func__ << ": no source provided, " << mixPortconfig.toString();
        return std::nullopt;
    }
    return mixPortconfig.ext.get<AudioPortExt::Tag::mix>()
            .usecase.get<AudioPortMixExtUseCase::Tag::source>();
}
#endif

Usecase getUsecaseTag(const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig) {
    Usecase tag = Usecase::INVALID;
    if (!mixPortConfig.flags || mixPortConfig.ext.getTag() != AudioPortExt::Tag::mix) {
        LOG(ERROR) << __func__ << " cannot determine usecase, no flags set for mix port "
                                  "config or it isn't mix port, "
                   << mixPortConfig.toString();
        return tag;
    }

    if (!(mixPortConfig.sampleRate) || (mixPortConfig.sampleRate.value().value == 0)) {
        LOG(ERROR) << __func__ << ": mix port config missing sample rate!!!";
        return tag;
    }

    const auto& streamSampleRate = mixPortConfig.sampleRate.value().value;
    const auto& mixUsecase = mixPortConfig.ext.get<AudioPortExt::Tag::mix>().usecase;
    const auto mixUsecaseTag = mixUsecase.getTag();

    const auto& flagsTag = mixPortConfig.flags.value().getTag();
    constexpr auto flagCastToint = [](auto flag) { return static_cast<int32_t>(flag); };

    const auto& channelLayout = mixPortConfig.channelMask.value();

    constexpr int32_t noneFlags = 0;
    constexpr auto primaryPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::PRIMARY));
    constexpr auto deepBufferPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::DEEP_BUFFER));
    constexpr auto compressOffloadPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::DIRECT) |
                                 1 << flagCastToint(AudioOutputFlags::COMPRESS_OFFLOAD) |
                                 1 << flagCastToint(AudioOutputFlags::NON_BLOCKING) |
                                 1 << flagCastToint(AudioOutputFlags::GAPLESS_OFFLOAD));
    constexpr auto fastRecordFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioInputFlags::FAST));
    constexpr auto ullRecordFlags = static_cast<int32_t>(
        1 << flagCastToint(AudioInputFlags::FAST)| 1 << flagCastToint(AudioInputFlags::RAW));
    constexpr auto compressCaptureFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioInputFlags::DIRECT));
#ifdef SEC_AUDIO_COMMON
    constexpr auto lowLatencyPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::FAST)
#ifdef SEC_AUDIO_SUPPORT_LOWLATENCY_MEDIA
                                | 1 << flagCastToint(AudioOutputFlags::MEDIA)
#endif
                                );
#else
    constexpr auto lowLatencyPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::PRIMARY) |
                                 1 << flagCastToint(AudioOutputFlags::FAST));
#endif
    constexpr auto pcmOffloadPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::DIRECT));
    constexpr auto voipPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::VOIP_RX));
    constexpr auto spatialPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::SPATIALIZER));
    constexpr auto recordVoipFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioInputFlags::VOIP_TX));
    constexpr auto ullPlaybackFlags = static_cast<int32_t>(
            1 << flagCastToint(AudioOutputFlags::FAST) | 1 << flagCastToint(AudioOutputFlags::RAW));
    constexpr auto mmapPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::DIRECT) |
                                 1 << flagCastToint(AudioOutputFlags::MMAP_NOIRQ));
    constexpr auto mmapRecordFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioInputFlags::MMAP_NOIRQ));
    constexpr auto inCallMusicFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::INCALL_MUSIC));
    constexpr auto hotWordRecordFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioInputFlags::HW_HOTWORD));

    if (flagsTag == AudioIoFlags::Tag::output) {
        auto& outFlags = mixPortConfig.flags.value().get<AudioIoFlags::Tag::output>();
        if (channelLayout.getTag() == AudioChannelLayout::Tag::layoutMask &&
                   channelLayout.get<AudioChannelLayout::Tag::layoutMask>() ==
                           AudioChannelLayout::LAYOUT_STEREO_HAPTIC_A) {
            tag = Usecase::HAPTICS_PLAYBACK;
        } else if (outFlags == primaryPlaybackFlags) {
            tag = Usecase::PRIMARY_PLAYBACK;
        } else if (outFlags == deepBufferPlaybackFlags || (outFlags == noneFlags)) {
            tag = Usecase::DEEP_BUFFER_PLAYBACK;
        } else if (outFlags == lowLatencyPlaybackFlags) {
            tag = Usecase::LOW_LATENCY_PLAYBACK;
        } else if (outFlags == compressOffloadPlaybackFlags) {
            tag = Usecase::COMPRESS_OFFLOAD_PLAYBACK;
        } else if (outFlags == pcmOffloadPlaybackFlags) {
            tag = Usecase::PCM_OFFLOAD_PLAYBACK;
        } else if (outFlags == voipPlaybackFlags) {
            tag = Usecase::VOIP_PLAYBACK;
        } else if (outFlags == spatialPlaybackFlags) {
            tag = Usecase::SPATIAL_PLAYBACK;
        } else if (outFlags == ullPlaybackFlags) {
            tag = Usecase::ULL_PLAYBACK;
        } else if (outFlags == mmapPlaybackFlags) {
            tag = Usecase::MMAP_PLAYBACK;
        } else if (outFlags == inCallMusicFlags) {
            tag = Usecase::IN_CALL_MUSIC;
        }
    } else if (flagsTag == AudioIoFlags::Tag::input) {
        auto& inFlags = mixPortConfig.flags.value().get<AudioIoFlags::Tag::input>();
        tag = Usecase::PCM_RECORD;
        if (inFlags == noneFlags) {
            tag = Usecase::PCM_RECORD;
            if (mixUsecaseTag == AudioPortMixExtUseCase::source) {
                auto& source = mixUsecase.get<AudioPortMixExtUseCase::source>();
                if (source == AudioSource::VOICE_UPLINK || source == AudioSource::VOICE_DOWNLINK ||
                    source == AudioSource::VOICE_CALL) {
                    tag = Usecase::VOICE_CALL_RECORD;
                }
            }
        } else if (inFlags == fastRecordFlags) {
            tag = Usecase::FAST_RECORD;
        } else if (inFlags == ullRecordFlags) {
            tag = Usecase::ULTRA_FAST_RECORD;
        } else if (inFlags == compressCaptureFlags) {
            tag = Usecase::COMPRESS_CAPTURE;
#if SEC_AUDIO_CALL_VOIP
        } else if (inFlags == recordVoipFlags && mixUsecaseTag == AudioPortMixExtUseCase::source &&
                   ((mixUsecase.get<AudioPortMixExtUseCase::source>() ==
                           AudioSource::VOICE_COMMUNICATION) ||
                   (mixUsecase.get<AudioPortMixExtUseCase::source>() ==
                           AudioSource::MIC))) {

#else
        } else if (inFlags == recordVoipFlags && mixUsecaseTag == AudioPortMixExtUseCase::source &&
                   mixUsecase.get<AudioPortMixExtUseCase::source>() ==
                           AudioSource::VOICE_COMMUNICATION) {
#endif
            tag = Usecase::VOIP_RECORD;
        } else if (inFlags == mmapRecordFlags) {
            tag = Usecase::MMAP_RECORD;
        } else if (inFlags == hotWordRecordFlags) {
            tag = Usecase::HOTWORD_RECORD;
        }
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
        auto& platform = Platform::getInstance();
        const auto& source = getAudioSource(mixPortConfig);
        if (inFlags != hotWordRecordFlags && platform.isSeamlessEnabled() && source && source.value() == AudioSource::VOICE_RECOGNITION) {
            tag = Usecase::HOTWORD_RECORD;
        }
#endif
    }
    LOG(VERBOSE) << __func__ << " choosen " << getName(tag) << " for mix port config "
                 << mixPortConfig.toString();
    return tag;
}

std::string getName(const Usecase tag) {
    switch (tag) {
        case Usecase::INVALID:
            return "INVALID";
        case Usecase::PRIMARY_PLAYBACK:
            return "PRIMARY_PLAYBACK";
        case Usecase::DEEP_BUFFER_PLAYBACK:
            return "DEEP_BUFFER_PLAYBACK";
        case Usecase::LOW_LATENCY_PLAYBACK:
            return "LOW_LATENCY_PLAYBACK";
        case Usecase::PCM_RECORD:
            return "PCM_RECORD";
        case Usecase::COMPRESS_OFFLOAD_PLAYBACK:
            return "COMPRESS_OFFLOAD_PLAYBACK";
        case Usecase::COMPRESS_CAPTURE:
            return "COMPRESS_CAPTURE";
        case Usecase::PCM_OFFLOAD_PLAYBACK:
            return "PCM_OFFLOAD_PLAYBACK";
        case Usecase::VOIP_PLAYBACK:
            return "VOIP_PLAYBACK";
        case Usecase::SPATIAL_PLAYBACK:
            return "SPATIAL_PLAYBACK";
        case Usecase::VOIP_RECORD:
            return "VOIP_RECORD";
        case Usecase::ULL_PLAYBACK:
            return "ULL_PLAYBACK";
        case Usecase::MMAP_PLAYBACK:
            return "MMAP_PLAYBACK";
        case Usecase::MMAP_RECORD:
            return "MMAP_RECORD";
        case Usecase::VOICE_CALL_RECORD:
            return "VOICE_CALL_RECORD";
        case Usecase::IN_CALL_MUSIC:
            return "IN_CALL_MUSIC";
        case Usecase::FAST_RECORD:
            return "FAST_RECORD";
        case Usecase::ULTRA_FAST_RECORD:
            return "ULTRA_FAST_RECORD";
        case Usecase::HOTWORD_RECORD:
            return "HOTWORD_RECORD";
        case Usecase::HAPTICS_PLAYBACK:
            return "HAPTICS_PLAYBACK";
        default:
            return std::to_string(static_cast<uint16_t>(tag));
    }
}

auto getIntValueFromVString = [](
        const std::vector<::aidl::android::hardware::audio::core::VendorParameter>& parameters,
        const std::string& searchKey) -> std::optional<int32_t> {
    std::optional<::aidl::qti::audio::core::VString> parcel;
    for (const auto& p : parameters) {
        if (p.id == searchKey && p.ext.getParcelable(&parcel) == ::android::OK &&
            parcel.has_value()) {
            int32_t value = strtol(parcel.value().value.c_str(), nullptr, 10);
            return value;
        }
    }
    return std::nullopt;
};

// [LowLatencyPlayback Start]
std::unordered_set<size_t> LowLatencyPlayback::kSupportedFrameSizes = {160, 192, 240, 320, 480};

size_t LowLatencyPlayback::getFrameCount(const AudioPortConfig& mixPortConfig) {
    const std::string kPeriodSizeProp = "vendor.audio_hal.period_size";
    size_t periodSize = kPeriodDurationMs * getSampleRate(mixPortConfig).value() / 1000;
    auto frameSize = ::android::base::GetUintProperty<size_t>(kPeriodSizeProp, periodSize);
    if (kSupportedFrameSizes.count(frameSize)) {
        return frameSize;
    }
    return periodSize;
}

// [LowLatencyPlayback End]

// [Deep Buffer Start]

size_t DeepBufferPlayback::getFrameCount(const AudioPortConfig& mixPortConfig) {
    return kPeriodDurationMs * getSampleRate(mixPortConfig).value() / 1000;
}

// [Deep Buffer End]
size_t PrimaryPlayback::getFrameCount(const AudioPortConfig& mixPortConfig) {
    return kPeriodDurationMs * getSampleRate(mixPortConfig).value() / 1000;
}

// [ULLPlayback Start]
size_t UllPlayback::getFrameCount(const AudioPortConfig& mixPortConfig) {
    return kPeriodDurationMs * getSampleRate(mixPortConfig).value() / 1000;
}

// [ULLPlayback End]

// [MmapUsecaseBase Start]

void MmapUsecaseBase::setPalHandle(pal_stream_handle_t* handle) {
    mPalHandle = handle;
}

int32_t MmapUsecaseBase::createMMapBuffer(int64_t frameSize, int32_t* fd, int64_t* burstSizeFrames,
                                          int32_t* flags, int32_t* bufferSizeFrames) {
    if (!mPalHandle) {
        LOG(ERROR) << __func__ << ": pal stream handle is null";
        return -EINVAL;
    }
    struct pal_mmap_buffer palMMapBuf;
    if (int32_t ret = pal_stream_create_mmap_buffer(mPalHandle, frameSize, &palMMapBuf); ret) {
        LOG(ERROR) << __func__ << ": pal stream create mmap buffer failed "
                   << "returned " << ret;
        return ret;
    }
    *fd = palMMapBuf.fd;
    *burstSizeFrames = palMMapBuf.burst_size_frames;
    *flags = palMMapBuf.flags;
    *bufferSizeFrames = palMMapBuf.buffer_size_frames;
    LOG(DEBUG) << __func__ << " burstSizeFrames " << *burstSizeFrames << " flags " << *flags
               << " bufferSizeFrames " << *bufferSizeFrames << " fd " << *fd;
    return 0;
}

int32_t MmapUsecaseBase::getMMapPosition(int64_t* frames, int64_t* timeNs) {
    if (!mPalHandle) {
        LOG(ERROR) << __func__ << ": pal stream handle is null";
        return -EINVAL;
    }
    struct pal_mmap_position pal_mmap_pos;
    if (int32_t ret = pal_stream_get_mmap_position(mPalHandle, &pal_mmap_pos); ret) {
        LOG(ERROR) << __func__ << ": failed to get mmap positon "
                   << "returned " << ret;
        return ret;
    }
    *timeNs = pal_mmap_pos.time_nanoseconds;
    *frames = pal_mmap_pos.position_frames;
    LOG(VERBOSE) << __func__ << ": frames:" << *frames << ", timeNs:" << *timeNs;
    return 0;
}

int32_t MmapUsecaseBase::start() {
    if (!mPalHandle) {
        LOG(ERROR) << __func__ << ": pal stream handle is null";
        return -EINVAL;
    }

    if (mIsStarted) {
        LOG(VERBOSE) << __func__ << ": MMAP already started";
        return 0;
    }

    if (int32_t ret = ::pal_stream_start(mPalHandle); ret) {
        LOG(ERROR) << __func__ << " pal stream start failed, ret:" << ret;
        return ret;
    }

    mIsStarted = true;
    LOG(VERBOSE) << __func__ << ": MMAP start success";

    return 0;
}

int32_t MmapUsecaseBase::stop() {
    if (!mPalHandle) {
        LOG(ERROR) << __func__ << ": pal stream handle is null";
        return -EINVAL;
    }

    if (!mIsStarted) {
        LOG(VERBOSE) << __func__ << ": MMAP already stopped";
        return 0;
    }

    if (int32_t ret = ::pal_stream_stop(mPalHandle); ret) {
        LOG(ERROR) << __func__ << " pal stream stop failed, ret:" << ret;
        return -EINVAL;
    }

    mIsStarted = false;
    LOG(VERBOSE) << __func__ << ": MMAP stop success";

    return 0;
}

// [MmapUsecaseBase End]
// [MMapPlayback Start]
size_t MMapPlayback::getFrameCount(const AudioPortConfig& mixPortConfig) {
    return kPeriodDurationMs * getSampleRate(mixPortConfig).value() / 1000;
}

// [MMapPlayback End]

// [CompressPlayback Start]
size_t CompressPlayback::getFrameCount(const AudioPortConfig& mixPortConfig) {
    auto format = mixPortConfig.format.value();
    size_t periodSize = kPeriodSize;
    if (format.encoding == ::android::MEDIA_MIMETYPE_AUDIO_FLAC) {
        periodSize = Flac::kPeriodSize;
    }

    const std::string kCompressPeriodSizeProp{"vendor.audio.offload.buffer.size.kb"};
    auto propPeriodSize =
            ::android::base::GetUintProperty<size_t>(kCompressPeriodSizeProp, 0) * 1024;

    if (propPeriodSize > periodSize) {
        periodSize = propPeriodSize;
    }
    return periodSize;
}

CompressPlayback::CompressPlayback(
        const ::aidl::android::media::audio::common::AudioOffloadInfo& offloadInfo,
        PlatformStreamCallback* const callback,
        const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig)
    : mOffloadInfo(offloadInfo), mPlatformStreamCallback(callback), mMixPortConfig(mixPortConfig) {
    configureDefault();
}

void CompressPlayback::configureDefault() {
    mSampleRate = mOffloadInfo.base.sampleRate;
    mCompressFormat = mOffloadInfo.base.format;
    mChannelLayout = mOffloadInfo.base.channelMask;
    mBitWidth = mOffloadInfo.bitWidth;

    if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_MP4 ||
        mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADIF ||
        mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS ||
#ifdef SEC_AUDIO_OFFLOAD
        mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_LC ||
#endif
        mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC ||
        mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_HE_V1 ||
        mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_HE_V2 ||
        mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_LC ||
        mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_LC) {
        mPalSndDec.aac_dec.audio_obj_type = 29;
        mPalSndDec.aac_dec.pce_bits_size = 0;
    }

    LOG(INFO) << __func__ << ": " << mOffloadInfo.toString();
    return;
}

void CompressPlayback::setAndConfigureCodecInfo(pal_stream_handle_t* handle) {
    mCompressPlaybackHandle = handle;
    if (mCompressPlaybackHandle == nullptr) {
        return;
    }
    configureCodecInfo();
}

void CompressPlayback::configureGapless(pal_stream_handle_t* handle) {
    mCompressPlaybackHandle = handle;
    if (mCompressPlaybackHandle == nullptr) {
        return;
    }
    configureGapLessMetadata();
}

ndk::ScopedAStatus CompressPlayback::getVendorParameters(
        const std::vector<std::string>& in_ids,
        std::vector<::aidl::android::hardware::audio::core::VendorParameter>* _aidl_return) {
    return ndk::ScopedAStatus::ok();
}

// static
int32_t CompressPlayback::palCallback(pal_stream_handle_t* palHandle, uint32_t eventId,
                                      uint32_t* eventData, uint32_t eventSize, uint64_t cookie) {
    auto compressPlayback = reinterpret_cast<CompressPlayback*>(cookie);

    switch (eventId) {
        case PAL_STREAM_CBK_EVENT_WRITE_READY: {
            compressPlayback->mPlatformStreamCallback->onTransferReady();
        } break;
        case PAL_STREAM_CBK_EVENT_DRAIN_READY: {
            compressPlayback->mPlatformStreamCallback->onDrainReady();
        } break;
        case PAL_STREAM_CBK_EVENT_PARTIAL_DRAIN_READY: {
            compressPlayback->mPlatformStreamCallback->onDrainReady();
            // gapless resets in PAL, when partial drain is received,
            compressPlayback->mIsGaplessConfigured = false;
        } break;
        case PAL_STREAM_CBK_EVENT_ERROR: {
            compressPlayback->mPlatformStreamCallback->onError();
        } break;
        default:
            LOG(ERROR) << __func__ << " invalid!!! event id:" << eventId;
            return -EINVAL;
    }
    return 0;
}

ndk::ScopedAStatus CompressPlayback::setVendorParameters(
        const std::vector<::aidl::android::hardware::audio::core::VendorParameter>& in_parameters,
        bool in_async) {
    LOG(VERBOSE) << __func__ << ": parameter count:" << in_parameters.size() << " parsing for "
                 << mCompressFormat.encoding;
    bool isCompressMetadataAvail = false;
    if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_FLAC) {
        if (auto value = getIntValueFromVString(in_parameters, Flac::kMinBlockSize); value) {
            mPalSndDec.flac_dec.min_blk_size = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Flac::kMaxBlockSize); value) {
            mPalSndDec.flac_dec.max_blk_size = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Flac::kMinFrameSize); value) {
            mPalSndDec.flac_dec.min_frame_size = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Flac::kMaxFrameSize); value) {
            mPalSndDec.flac_dec.max_frame_size = value.value();
            isCompressMetadataAvail = true;
        }
        // exception
        mPalSndDec.flac_dec.sample_size = (mBitWidth == 32) ? 24 : mBitWidth;
    } else if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_ALAC) {
        if (auto value = getIntValueFromVString(in_parameters, Alac::kFrameLength); value) {
            mPalSndDec.alac_dec.frame_length = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Alac::kCompatVer); value) {
            mPalSndDec.alac_dec.compatible_version = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Alac::kBitDepth); value) {
            mPalSndDec.alac_dec.bit_depth = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Alac::kPb); value) {
            mPalSndDec.alac_dec.pb = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Alac::kMb); value) {
            mPalSndDec.alac_dec.mb = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Alac::kKb); value) {
            mPalSndDec.alac_dec.kb = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Alac::kNumChannels); value) {
            mPalSndDec.alac_dec.num_channels = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Alac::kMaxRun); value) {
            mPalSndDec.alac_dec.max_run = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Alac::kMaxFrameBytes); value) {
            mPalSndDec.alac_dec.max_frame_bytes = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Alac::kBitRate); value) {
            mPalSndDec.alac_dec.avg_bit_rate = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Alac::kSamplingRate); value) {
            mPalSndDec.alac_dec.sample_rate = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Alac::kChannelLayoutTag); value) {
            mPalSndDec.alac_dec.channel_layout_tag = value.value();
            isCompressMetadataAvail = true;
        }
    } else if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_MP4 ||
               mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADIF ||
               mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS ||
               mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC) {
        mPalSndDec.aac_dec.audio_obj_type = 29;
        mPalSndDec.aac_dec.pce_bits_size = 0;
    } else if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_VORBIS) {
        if (auto value = getIntValueFromVString(in_parameters, Vorbis::kBitStreamFormat); value) {
            mPalSndDec.vorbis_dec.bit_stream_fmt = value.value();
            isCompressMetadataAvail = true;
        }
    } else if (mCompressFormat.encoding == "audio/x-ape") {
        if (auto value = getIntValueFromVString(in_parameters, Ape::kCompatibleVersion); value) {
            mPalSndDec.ape_dec.compatible_version = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Ape::kCompressionLevel); value) {
            mPalSndDec.ape_dec.compression_level = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Ape::kFormatFlags); value) {
            mPalSndDec.ape_dec.format_flags = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Ape::kBlocksPerFrame); value) {
            mPalSndDec.ape_dec.blocks_per_frame = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Ape::kFinalFrameBlocks); value) {
            mPalSndDec.ape_dec.final_frame_blocks = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Ape::kTotalFrames); value) {
            mPalSndDec.ape_dec.total_frames = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Ape::kBitsPerSample); value) {
            mPalSndDec.ape_dec.bits_per_sample = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Ape::kNumChannels); value) {
            mPalSndDec.ape_dec.num_channels = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Ape::kSampleRate); value) {
            mPalSndDec.ape_dec.sample_rate = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Ape::kSeekTablePresent); value) {
            mPalSndDec.ape_dec.seek_table_present = value.value();
            isCompressMetadataAvail = true;
        }
    } else if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_WMA ||
               mCompressFormat.encoding == "audio/x-ms-wma.pro") {
        if (auto value = getIntValueFromVString(in_parameters, Wma::kFormatTag); value) {
            mPalSndDec.wma_dec.fmt_tag = value.value();
            isCompressMetadataAvail = true;
        }
        mPalSndDec.wma_dec.avg_bit_rate = mOffloadMetadata.averageBitRatePerSecond;

        LOG(VERBOSE) << __func__ << ": averageBitRatePerSecond "
                                 << mOffloadMetadata.averageBitRatePerSecond;
        if (auto value = getIntValueFromVString(in_parameters, Wma::kBlockAlign); value) {
            mPalSndDec.wma_dec.super_block_align = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Wma::kBitPerSample); value) {
            mPalSndDec.wma_dec.bits_per_sample = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Wma::kChannelMask); value) {
            mPalSndDec.wma_dec.channelmask = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Wma::kEncodeOption); value) {
            mPalSndDec.wma_dec.encodeopt = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Wma::kEncodeOption1); value) {
            mPalSndDec.wma_dec.encodeopt1 = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Wma::kEncodeOption2); value) {
            mPalSndDec.wma_dec.encodeopt2 = value.value();
            isCompressMetadataAvail = true;
        }
    } else if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_OPUS) {
        if (auto value = getIntValueFromVString(in_parameters, Opus::kBitStreamFormat); value) {
            mPalSndDec.opus_dec.bitstream_format = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kPayloadType); value) {
            mPalSndDec.opus_dec.payload_type = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kVersion); value) {
            mPalSndDec.opus_dec.version = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kNumChannels); value) {
            mPalSndDec.opus_dec.num_channels = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kPreSkip); value) {
            mPalSndDec.opus_dec.pre_skip = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kOutputGain); value) {
            mPalSndDec.opus_dec.output_gain = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kMappingFamily); value) {
            mPalSndDec.opus_dec.mapping_family = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kStreamCount); value) {
            mPalSndDec.opus_dec.stream_count = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kCoupledCount); value) {
            mPalSndDec.opus_dec.coupled_count = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kChannelMap0); value) {
            mPalSndDec.opus_dec.channel_map[0] = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kChannelMap1); value) {
            mPalSndDec.opus_dec.channel_map[1] = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kChannelMap2); value) {
            mPalSndDec.opus_dec.channel_map[2] = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kChannelMap3); value) {
            mPalSndDec.opus_dec.channel_map[3] = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kChannelMap4); value) {
            mPalSndDec.opus_dec.channel_map[4] = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kChannelMap5); value) {
            mPalSndDec.opus_dec.channel_map[5] = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kChannelMap6); value) {
            mPalSndDec.opus_dec.channel_map[6] = value.value();
            isCompressMetadataAvail = true;
        }
        if (auto value = getIntValueFromVString(in_parameters, Opus::kChannelMap7); value) {
            mPalSndDec.opus_dec.channel_map[7] = value.value();
            isCompressMetadataAvail = true;
        }
        mPalSndDec.opus_dec.sample_rate = mSampleRate;
    }
    if(mCompressPlaybackHandle == nullptr){
        return ndk::ScopedAStatus::ok();
    }
    LOG(VERBOSE) << __func__ << ": trying for on-the-fly codec configuration";
    if (isCompressMetadataAvail)
        configureCodecInfo();
    return ndk::ScopedAStatus::ok();
}

bool CompressPlayback::configureGapLessMetadata() {
    const auto payloadSize = sizeof(pal_param_payload);
    const auto kGapLessSize = sizeof(pal_compr_gapless_mdata);
    auto dataPtr = std::make_unique<uint8_t[]>(payloadSize + kGapLessSize);
    auto payloadPtr = reinterpret_cast<pal_param_payload*>(dataPtr.get());
    payloadPtr->payload_size = kGapLessSize;
    auto gapLessPtr = reinterpret_cast<pal_compr_gapless_mdata*>(dataPtr.get() + payloadSize);
    gapLessPtr->encoderDelay = mOffloadMetadata.delayFrames;
    gapLessPtr->encoderPadding = mOffloadMetadata.paddingFrames;

    if (mCompressPlaybackHandle) {
        if (int32_t ret = ::pal_stream_set_param(mCompressPlaybackHandle, PAL_PARAM_ID_GAPLESS_MDATA,
                                             payloadPtr);
            ret) {
            LOG(ERROR) << __func__ << ": failed PAL_PARAM_ID_GAPLESS_MDATA!! ret:" << ret;
            return false;
        }
        mIsGaplessConfigured = true;
        LOG(VERBOSE) << __func__ << ": encoderDelay:" << gapLessPtr->encoderDelay
                 << ", encoderPadding:" << gapLessPtr->encoderPadding;
        return true;
    }
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    LOG(INFO) << __func__ << " PAL stream handle is NULL!";
#else
    LOG(ERROR) << __func__ << " PAL stream handle is NULL!";
#endif
    return false;
}

void CompressPlayback::updateOffloadMetadata(
        const ::aidl::android::hardware::audio::common::AudioOffloadMetadata& offloadMetaData) {
    mOffloadMetadata = offloadMetaData;
    mSampleRate = mOffloadMetadata.sampleRate;
    mChannelLayout = mOffloadMetadata.channelMask;
    configureGapLessMetadata();
    return;
}

void CompressPlayback::updateSourceMetadata(
        const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetaData) {
    mSourceMetadata = &sourceMetaData;
    // TODO check for any pal update
    LOG(INFO) << __func__ << ": " << mSourceMetadata->toString();
    return;
}

bool CompressPlayback::configureCodecInfo() const {
    auto dataPtr = std::make_unique<uint8_t[]>(sizeof(pal_param_payload) + sizeof(pal_snd_dec_t));
    auto palParamPayload = reinterpret_cast<pal_param_payload*>(dataPtr.get());
    palParamPayload->payload_size = sizeof(pal_snd_dec_t);
    auto palSndDecPtr = reinterpret_cast<pal_snd_dec_t*>(dataPtr.get() + sizeof(pal_param_payload));
    *palSndDecPtr = mPalSndDec;
    if (mCompressPlaybackHandle) {
        if (int32_t ret =
                ::pal_stream_set_param(mCompressPlaybackHandle, PAL_PARAM_ID_CODEC_CONFIGURATION,
                                       reinterpret_cast<pal_param_payload*>(dataPtr.get()));
        ret) {
            LOG(ERROR) << __func__ << " PAL_PARAM_ID_CODEC_CONFIGURATION failed, ret:" << ret;
            return false;
        }
        LOG(VERBOSE) << __func__ << " PAL_PARAM_ID_CODEC_CONFIGURATION successful";
        return true;
    }
    LOG(ERROR) << __func__ << " PAL stream handle is NULL!";
    return false;
}

int64_t CompressPlayback::getPositionInFrames(pal_stream_handle_t* palHandle) {
    if (palHandle == nullptr) {
        return mTotalDSPFrames + mPrevFrames;
    }

    pal_session_time tstamp;
    if (int32_t ret = ::pal_get_timestamp(palHandle, &tstamp); ret) {
        LOG(ERROR) << __func__ << " pal_get_timestamp failure, returning previous" << ret;
        return mTotalDSPFrames + mPrevFrames;
    }

    uint64_t sessionTimeUs =
            ((static_cast<decltype(sessionTimeUs)>(tstamp.session_time.value_msw)) << 32 |
             tstamp.session_time.value_lsw);
    const auto& sampleRate = getSampleRate(mMixPortConfig).value();
    // sessionTimeUs to frames
    // try to convert the session to frames without loss of precision.
    mPrevFrames = static_cast<int64_t>((sessionTimeUs / 1000) * sampleRate / 1000);
    LOG(VERBOSE) << __func__ << " dsp frames consumed: (" << mTotalDSPFrames << "+" << mPrevFrames
                 << ") = " << mTotalDSPFrames + mPrevFrames;
    return mTotalDSPFrames + mPrevFrames;
}

void CompressPlayback::onFlush() {
    // on flush SPR module is reset to 0. Hence, we cache the DSP frames
    mTotalDSPFrames = mTotalDSPFrames + mPrevFrames;
    mPrevFrames = 0;
}

// [CompressPlayback End]

// [PcmOffloadPlayback Start]

size_t PcmOffloadPlayback::getFrameCount(
        const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig) {
    const auto frameSize =
            getFrameSizeInBytes(mixPortConfig.format.value(), mixPortConfig.channelMask.value());
    if (frameSize == 0) {
        LOG(ERROR) << __func__ << ": could not calculate frame size for port config "
                   << mixPortConfig.toString();
        return 0;
    }

    size_t periodSize =
            (mixPortConfig.sampleRate.value().value * kPeriodDurationMs * frameSize) / 1000;

    if (periodSize < kMinPeriodSize) {
        periodSize = kMinPeriodSize;
    } else if (periodSize > kMaxPeriodSize) {
        periodSize = kMaxPeriodSize;
    }

    periodSize = ALIGN(periodSize, (frameSize * 32));

    if (auto res = Platform::requiresBufferReformat(mixPortConfig)) {
        audio_format_t inFormat = res->first;
        audio_format_t outFormat = res->second;
        periodSize =
                (periodSize * audio_bytes_per_sample(inFormat)) / audio_bytes_per_sample(outFormat);
    }

    return periodSize / frameSize;
}

int64_t PcmOffloadPlayback::getPositionInFrames(pal_stream_handle_t* palHandle) {
    if (palHandle == nullptr) {
        return mTotalDSPFrames + mPrevFrames;
    }

    // if sound card not up, then cache position
    auto& platform = Platform::getInstance();
    if (!platform.isSoundCardUp()) {
        mTotalDSPFrames = mTotalDSPFrames + mPrevFrames;
        return mTotalDSPFrames;
    }

    pal_session_time tstamp;
    if (int32_t ret = ::pal_get_timestamp(palHandle, &tstamp); ret) {
        LOG(ERROR) << __func__ << " pal_get_timestamp failure, returning previous" << ret;
        return mTotalDSPFrames + mPrevFrames;
    }

    uint64_t sessionTimeUs =
            ((static_cast<decltype(sessionTimeUs)>(tstamp.session_time.value_msw)) << 32 |
             tstamp.session_time.value_lsw);
    const auto& sampleRate = getSampleRate(mMixPortConfig).value();
    // sessionTimeUs to frames
    // try to convert the session to frames without loss of precision.
    mPrevFrames = static_cast<int64_t>((sessionTimeUs / 1000) * sampleRate / 1000);
    LOG(VERBOSE) << __func__ << " dsp frames consumed: (" << mTotalDSPFrames << "+" << mPrevFrames
                 << ") = " << mTotalDSPFrames + mPrevFrames;
    return mTotalDSPFrames + mPrevFrames;
}

void PcmOffloadPlayback::onFlush() {
    // on flush SPR module is reset to 0. Hence, we cache the DSP frames
    mTotalDSPFrames = mTotalDSPFrames + mPrevFrames;
    mPrevFrames = 0;
}

// [PcmOffloadPlayback End]

// [SpatialPlayback Start]
size_t SpatialPlayback::getFrameCount(const AudioPortConfig& mixPortConfig) {
    return kPeriodDurationMs * getSampleRate(mixPortConfig).value() / 1000;
}

// [SpatialPlayback End]

// [InCallMusic Start]
size_t InCallMusic::getFrameCount(const AudioPortConfig& mixPortConfig) {
    return kPeriodDurationMs * getSampleRate(mixPortConfig).value() / 1000;
}

// [InCallMusic End]

// [VoipPlayback Start]
size_t VoipPlayback::getFrameCount(const AudioPortConfig& mixPortConfig) {
    return kPeriodDurationMs * getSampleRate(mixPortConfig).value() / 1000;
}

// [VoipPlayback End]

// [HapticPlayback Start]
size_t HapticsPlayback::getFrameCount(const AudioPortConfig& mixPortConfig) {
    return kPeriodDurationMs * getSampleRate(mixPortConfig).value() / 1000;
}

// [HapticsPlayback End]
// [PcmRecord Start]
size_t PcmRecord::getFrameCount(const AudioPortConfig& mixPortConfig) {
    size_t frameCount = kCaptureDurationMs * getSampleRate(mixPortConfig).value() / 1000;
    frameCount = getNearestMultiple(
            frameCount, std::lcm(32, getPcmSampleSizeInBytes(mixPortConfig.format.value().pcm)));
    // Adjusting to frameCount as atleast kFMQMinFrameSize (160).
    // Todo check the sanity of this requirement in the VTS test.
    frameCount = std::max(frameCount, kFMQMinFrameSize);
    return frameCount;
}

// [PcmRecord End]

// [FastRecord Start]
size_t FastRecord::getFrameCount(const AudioPortConfig& mixPortConfig) {
    /**
     * Some clients which directly uses AHAL service for Fast Record like
     * proxy capture
     **/
    auto& platform = Platform::getInstance();
    if (const auto& propFrameSize = platform.getProxyRecordFMQSize(); propFrameSize > 0) {
        LOG(VERBOSE) << __func__ << ": client applied FMQSize in Frames:" << propFrameSize;
        return propFrameSize;
    }

    size_t periodSize = (kCaptureDurationMs * getSampleRate(mixPortConfig).value()) / 1000;
    size_t frameSize =
            getFrameSizeInBytes(mixPortConfig.format.value(), mixPortConfig.channelMask.value());
    if (frameSize == 0) {
        LOG(ERROR) << __func__ << ": could not calculate frame size for port config "
                   << mixPortConfig.toString();
        return 0;
    }
    size_t size = periodSize * frameSize;
    size = getNearestMultiple(size, std::lcm(32, frameSize));
    return size / frameSize;
}
// [FastRecord End]

// [UltraFastRecord Start]
size_t UltraFastRecord::getFrameCount(const AudioPortConfig& mixPortConfig) {
    /**
     * Some clients which directly uses AHAL service for ULL Record like
     * proxy capture
     **/
    auto& platform = Platform::getInstance();
    if (const auto& propFrameSize = platform.getProxyRecordFMQSize(); propFrameSize > 0) {
        LOG(VERBOSE) << __func__ << ": client applied FMQSize in Frames:" << propFrameSize;
        return propFrameSize;
    }

    // return default period size for ULL
    return kCaptureDurationMs * getSampleRate(mixPortConfig).value() / 1000;
}

// [UltraFastRecord End]

// [MMapRecord Start]

size_t MMapRecord::getFrameCount(const AudioPortConfig& mixPortConfig) {
    return kCaptureDurationMs * getSampleRate(mixPortConfig).value() / 1000;
}

// [MMapRecord End]

// [HotwordRecord Start]
size_t HotwordRecord::getFrameCount(const AudioPortConfig& mixPortConfig) {
    return PcmRecord::getFrameCount(mixPortConfig);
}

pal_stream_handle_t* HotwordRecord::getPalHandle(
        const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig) {
    size_t payloadSize = 0;
    pal_param_st_capture_info_t stCaptureInfo{0, nullptr};

    auto& ioHandle = mixPortConfig.ext.get<AudioPortExt::Tag::mix>().handle;
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
    if (mIsSeamless)
        stCaptureInfo.capture_handle = AUDIO_IO_HANDLE_NONE;
    else
#endif
    stCaptureInfo.capture_handle = ioHandle;

    int32_t ret = pal_get_param(PAL_PARAM_ID_ST_CAPTURE_INFO, (void**)&stCaptureInfo, &payloadSize,
                                nullptr);
    if (ret || !stCaptureInfo.pal_handle) {
        LOG(ERROR) << __func__ << ": sound trigger handle not found, status " << ret;
        return nullptr;
    }

    if (!mIsStRecord) {
        mIsStRecord = true;
        LOG(DEBUG) << __func__ << ": sound trigger pal handle " << stCaptureInfo.pal_handle
                << " for IOHandle  " << ioHandle;
    }

    return stCaptureInfo.pal_handle;
}
// [HotwordRecord End]

// [VoipRecord Start]
size_t VoipRecord::getFrameCount(const AudioPortConfig& mixPortConfig) {
    return (kCaptureDurationMs * mixPortConfig.sampleRate.value().value) / 1000;
}

// [VoipRecord End]

// [VoiceCallRecord Start]
size_t VoiceCallRecord::getFrameCount(const AudioPortConfig& mixPortConfig) {
    return kCaptureDurationMs * (mixPortConfig.sampleRate.value().value / 1000);
}

pal_incall_record_direction VoiceCallRecord::getRecordDirection(
        const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig) {
    auto& source = mixPortConfig.ext.get<AudioPortExt::Tag::mix>()
                           .usecase.get<AudioPortMixExtUseCase::source>();
    if (source == AudioSource::VOICE_UPLINK) {
        return INCALL_RECORD_VOICE_UPLINK;
    } else if (source == AudioSource::VOICE_DOWNLINK) {
        return INCALL_RECORD_VOICE_DOWNLINK;
    } else if (source == AudioSource::VOICE_CALL) {
        return INCALL_RECORD_VOICE_UPLINK_DOWNLINK;
    }
    LOG(ERROR) << __func__ << ": Invalid source for VoiceCallRecord" << static_cast<int>(source);
    return static_cast<pal_incall_record_direction>(0);
}

// [VoiceCallRecord End]

// [CompressCapture Start]
size_t CompressCapture::getFrameCount(const AudioPortConfig& mixPortConfig) {
    auto format = mixPortConfig.format.value();
    if (format.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_LC ||
        format.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_LC ||
        format.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_HE_V1 ||
        format.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_HE_V2) {
        return Aac::KAacMaxOutputSize;
    }
    return 0;
}

CompressCapture::CompressCapture(
        const ::aidl::android::media::audio::common::AudioFormatDescription& format,
        const int32_t sampleRate,
        const ::aidl::android::media::audio::common::AudioChannelLayout& channelLayout)
    : mCompressFormat(format), mSampleRate(sampleRate), mChannelLayout(channelLayout) {
    if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_LC ||
        mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_LC) {
        mPalSndEnc.aac_enc.enc_cfg.aac_enc_mode = Aac::EncodingMode::LC;
        mPalSndEnc.aac_enc.enc_cfg.aac_fmt_flag = Aac::EncodingFormat::ADTS;
        mPalSndEnc.aac_enc.aac_bit_rate = Aac::kAacDefaultBitrate;
    } else if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_HE_V1) {
        mPalSndEnc.aac_enc.enc_cfg.aac_enc_mode = Aac::EncodingMode::SBR;
        mPalSndEnc.aac_enc.enc_cfg.aac_fmt_flag = Aac::EncodingFormat::ADTS;
        mPalSndEnc.aac_enc.aac_bit_rate = Aac::kAacDefaultBitrate;
    } else if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_HE_V2) {
        mPalSndEnc.aac_enc.enc_cfg.aac_enc_mode = Aac::EncodingMode::PS;
        mPalSndEnc.aac_enc.enc_cfg.aac_fmt_flag = Aac::EncodingFormat::ADTS;
        mPalSndEnc.aac_enc.aac_bit_rate = Aac::kAacDefaultBitrate;
    }
    mPCMSamplesPerFrame = (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_LC ||
                           mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_LC)
                                  ? Aac::kAacLcPCMSamplesPerFrame
                                  : Aac::kHeAacPCMSamplesPerFrame;
}

void CompressCapture::setPalHandle(pal_stream_handle_t* handle) {
    mCompressHandle = handle;
}

size_t CompressCapture::getLatencyMs() {
    constexpr size_t kMilliSeconds = 1000;
    return mPCMSamplesPerFrame * kMilliSeconds / mSampleRate;
}
void CompressCapture::advanceReadCount() {
    mNumReadCalls++;
}

int64_t CompressCapture::getPositionInFrames() {
    return (mNumReadCalls * mPCMSamplesPerFrame);
}

bool CompressCapture::configureCodecInfo(){
    /* check for global cut-off frequency*/
    if (mPalSndEnc.aac_enc.global_cutoff_freq <= 0 /* not configured*/) {
        const std::string kAACCutOffFrequencyProp{"vendor.audio.compress_capture.aac.cut_off_freq"};
        mPalSndEnc.aac_enc.global_cutoff_freq =
                ::android::base::GetIntProperty<int32_t>(kAACCutOffFrequencyProp, 0);
    }

    auto dataPtr = std::make_unique<uint8_t[]>(sizeof(pal_param_payload) + sizeof(pal_snd_enc_t));
    auto palParamPayload = reinterpret_cast<pal_param_payload*>(dataPtr.get());
    palParamPayload->payload_size = sizeof(pal_snd_enc_t);
    auto payloadPtr = reinterpret_cast<pal_snd_enc_t*>(dataPtr.get() + sizeof(pal_param_payload));
    *payloadPtr = mPalSndEnc;
    if (mCompressHandle) {
        if (int32_t ret = ::pal_stream_set_param(mCompressHandle, PAL_PARAM_ID_CODEC_CONFIGURATION,
                                             palParamPayload); ret) {
            LOG(ERROR) << __func__ << " PAL_PARAM_ID_CODEC_CONFIGURATION failed!!! ret:" << ret;
            return false;
        }

        LOG(VERBOSE) << __func__ << " PAL_PARAM_ID_CODEC_CONFIGURATION configured";
        return true;
    }
    LOG(ERROR) << __func__ << " PAL stream handle is NULL!";
    return false;
}

ndk::ScopedAStatus CompressCapture::setVendorParameters(
        const std::vector<::aidl::android::hardware::audio::core::VendorParameter>& in_parameters,
        bool in_async) {
    LOG(VERBOSE) << __func__ << " parsing for " << mCompressFormat.encoding;
    if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_LC ||
        mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_HE_V1 ||
        mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_HE_V2 ||
        mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_LC) {
        if (auto value = getIntValueFromVString(in_parameters, Aac::kDSPAacBitRate); value) {
            auto requested = value.value();
            const auto min = getAACMinBitrateValue();
            const auto max = getAACMaxBitrateValue();
            mPalSndEnc.aac_enc.aac_bit_rate =
                    requested < min ? min : (requested > max ? max : requested);
            mCompressHandle != nullptr ? (void)setAACDSPBitRate() : (void)0;
        }
        if (auto value = getIntValueFromVString(in_parameters, Aac::kDSPAacGlobalCutoffFrequency);
            value) {
            if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_LC) {
                mPalSndEnc.aac_enc.global_cutoff_freq = value.value();
            }
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CompressCapture::getVendorParameters(
        const std::vector<std::string>& in_ids, std::vector<VendorParameter>* _aidl_return) {
    std::vector<VendorParameter> result;
    for (const auto& id : in_ids) {
        if (id == Aac::kDSPAacBitRate) {
            result.emplace_back(
                    makeVendorParameter(id, std::to_string(mPalSndEnc.aac_enc.aac_bit_rate)));
        } else if (id == Aac::kDSPAacGlobalCutoffFrequency) {
            result.emplace_back(makeVendorParameter(
                    id, std::to_string(mPalSndEnc.aac_enc.global_cutoff_freq)));
        }
    }
    *_aidl_return = result;
    return ndk::ScopedAStatus::ok();
}

void CompressCapture::setAACDSPBitRate() {
    const auto palSndEncSize = sizeof(pal_snd_enc_t);
    auto payload = std::make_unique<uint8_t[]>(sizeof(pal_param_payload) + palSndEncSize);
    auto paramPayload = (pal_param_payload*)payload.get();
    paramPayload->payload_size = palSndEncSize;
    memcpy(paramPayload->payload, &mPalSndEnc, paramPayload->payload_size);
    if (mCompressHandle) {
        if (int32_t ret = ::pal_stream_set_param(mCompressHandle, PAL_PARAM_ID_RECONFIG_ENCODER,
                                             paramPayload);
        ret) {
            LOG(ERROR) << __func__ << "pal set param PAL_PARAM_ID_RECONFIG_ENCODER failed:" << ret;
        }
    } else {
        LOG(ERROR) << __func__ << "PAL stream handle is NULL!";
    }

}

int32_t CompressCapture::getAACMinBitrateValue() {
    const auto channelCount =
            ::aidl::android::hardware::audio::common::getChannelCount(mChannelLayout);
    if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_LC ||
        mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_LC) {
        if (channelCount == 1) {
            return Aac::kAacLcMonoMinSupportedBitRate;
        } else {
            return Aac::kAacLcStereoMinSupportedBitRate;
        }
    } else if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_HE_V1) {
        if (channelCount == 1) {
            return (mSampleRate == 24000 || mSampleRate == 32000)
                           ? Aac::kHeAacMonoMinSupportedBitRate1
                           : Aac::kHeAacMonoMinSupportedBitRate2;
        } else {
            return (mSampleRate == 24000 || mSampleRate == 32000)
                           ? Aac::kHeAacStereoMinSupportedBitRate1
                           : Aac::kHeAacStereoMinSupportedBitRate2;
        }
    } else {
        // AUDIO_FORMAT_AAC_ADTS_HE_V2
        return (mSampleRate == 24000 || mSampleRate == 32000)
                       ? Aac::kHeAacPsStereoMinSupportedBitRate1
                       : Aac::kHeAacPsStereoMinSupportedBitRate2;
    }
}

int32_t CompressCapture::getAACMaxBitrateValue() {
    const auto channelCount =
            ::aidl::android::hardware::audio::common::getChannelCount(mChannelLayout);
    if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_LC ||
        mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_LC) {
        if (channelCount == 1) {
            return std::min(Aac::kAacLcMonoMaxSupportedBitRate, 6 * mSampleRate);
        } else {
            return std::min(Aac::kAacLcStereoMaxSupportedBitRate, 12 * mSampleRate);
        }
    } else if (mCompressFormat.encoding == ::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_HE_V1) {
        if (channelCount == 1) {
            return std::min(Aac::kHeAacMonoMaxSupportedBitRate, 6 * mSampleRate);
        } else {
            return std::min(Aac::kHeAacStereoMaxSupportedBitRate, 12 * mSampleRate);
        }
    } else {
        // AUDIO_FORMAT_AAC_ADTS_HE_V2
        return std::min(Aac::kHeAacPstereoMaxSupportedBitRate, 6 * mSampleRate);
    }
}

uint32_t CompressCapture::getAACMaxBufferSize() {
    int32_t maxBitRate = getAACMaxBitrateValue();
    /**
     * AAC Encoder 1024 PCM samples => 1 compress AAC frame;
     * 1 compress AAC frame => max possible length => max-bitrate bits;
     * let's take example of 48K HZ;
     * 1 second ==> 384000 bits ; 1 second ==> 48000 PCM samples;
     * 1 AAC frame ==> 1024 PCM samples;
     * Max buffer size possible;
     * 48000/1024 = (8/375) seconds ==> ( 8/375 ) * 384000 bits
     *     ==> ( (8/375) * 384000 / 8 ) bytes;
     **/
    return (uint32_t)(
            (((((double)mPCMSamplesPerFrame) / mSampleRate) * ((uint32_t)(maxBitRate))) / 8) +
            /* Just in case; not to miss precision */ 1);
}

}  // namespace qti::audio::core

