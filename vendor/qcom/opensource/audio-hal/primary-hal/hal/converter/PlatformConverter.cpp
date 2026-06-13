/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AHAL_PlatformConverter_QTI"

#include <android-base/logging.h>
#include <media/stagefright/foundation/MediaDefs.h>
#include <qti-audio/PlatformConverter.h>

// { SEC_AUDIO_COMMON
#include <system/audio.h>
// } SEC_AUDIO_COMMON

using ::aidl::android::media::audio::common::AudioChannelLayout;
using ::aidl::android::media::audio::common::AudioDeviceAddress;
using ::aidl::android::media::audio::common::AudioDeviceDescription;
using ::aidl::android::media::audio::common::AudioDeviceType;
using ::aidl::android::media::audio::common::AudioFormatDescription;
using ::aidl::android::media::audio::common::AudioFormatType;
using ::aidl::android::media::audio::common::AudioOutputFlags;
using ::aidl::android::media::audio::common::PcmType;

// clang-format off
namespace {
__attribute__((no_sanitize("unsigned-integer-overflow")))
static void hash_combiner(std::size_t& seed, const std::size_t& v) {
    // see boost::hash_combine
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
}  // namespace
// clang-format on

namespace std {
template <>
struct hash<::aidl::android::media::audio::common::AudioDeviceDescription> {
    std::size_t operator()(const ::aidl::android::media::audio::common::AudioDeviceDescription& add)
            const noexcept {
        std::size_t seed = 0;
        hash_combiner(seed, std::hash<::aidl::android::media::audio::common::AudioDeviceType>{}(
                                    add.type));
        hash_combiner(seed, std::hash<std::string>{}(add.connection));
        return seed;
    }
};

template <>
struct hash<::aidl::android::media::audio::common::AudioFormatDescription> {
    std::size_t operator()(const ::aidl::android::media::audio::common::AudioFormatDescription& aft)
            const noexcept {
        std::size_t seed = 0;
        hash_combiner(seed, std::hash<::aidl::android::media::audio::common::AudioFormatType>{}(
                                    aft.type));
        hash_combiner(seed, std::hash<::aidl::android::media::audio::common::PcmType>{}(aft.pcm));
        hash_combiner(seed, std::hash<std::string>{}(aft.encoding));
        return seed;
    }
};
} // namespace std

namespace qti::audio {

AudioDeviceDescription makeAudioDeviceDescription(AudioDeviceType type,
                                                  const std::string& connection = "") {
    AudioDeviceDescription result;
    result.type = type;
    result.connection = connection;
    return result;
}

using DevicePair = std::pair<AudioDeviceDescription, pal_device_id_t>;
using DevicePairs = std::vector<DevicePair>;

using outputFlagsStreamtypeMap =
        std::unordered_map<int32_t, pal_stream_type_t>;

// conversions
DevicePairs getDevicePairs() {
    // No dupicates on first entry
    DevicePairs pairs = {
            {AudioDeviceDescription{}, PAL_DEVICE_NONE},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_DEFAULT), PAL_DEVICE_OUT_SPEAKER},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_SPEAKER_EARPIECE),
             PAL_DEVICE_OUT_HANDSET},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_SPEAKER), PAL_DEVICE_OUT_SPEAKER},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_HEADPHONE,
                                        AudioDeviceDescription::CONNECTION_ANALOG),
             PAL_DEVICE_OUT_WIRED_HEADPHONE},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_DEVICE,
                                        AudioDeviceDescription::CONNECTION_BT_SCO),
             PAL_DEVICE_OUT_BLUETOOTH_SCO},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_CARKIT,
                                        AudioDeviceDescription::CONNECTION_BT_SCO),
             PAL_DEVICE_OUT_BLUETOOTH_SCO},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_TELEPHONY_TX), PAL_DEVICE_NONE},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_LINE_AUX), PAL_DEVICE_OUT_AUX_LINE},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_SPEAKER_SAFE), PAL_DEVICE_OUT_SPEAKER},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_SPEAKER,
                                        AudioDeviceDescription::CONNECTION_BT_LE),
             PAL_DEVICE_OUT_BLUETOOTH_BLE},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_BROADCAST,
                                        AudioDeviceDescription::CONNECTION_BT_LE),
             PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST},
            {makeAudioDeviceDescription(AudioDeviceType::IN_DEFAULT), PAL_DEVICE_IN_HANDSET_MIC},
            {makeAudioDeviceDescription(AudioDeviceType::IN_MICROPHONE), PAL_DEVICE_IN_HANDSET_MIC},
            {makeAudioDeviceDescription(AudioDeviceType::IN_MICROPHONE_BACK),
             PAL_DEVICE_IN_SPEAKER_MIC},
#ifdef SEC_AUDIO_COMMON
            {makeAudioDeviceDescription(AudioDeviceType::IN_MICROPHONE_MULTI), // AUDIO_DEVICE_IN_2MIC
             PAL_DEVICE_IN_SPEAKER_MIC},
            {makeAudioDeviceDescription(AudioDeviceType::IN_MICROPHONE_MULTI, // AUDIO_DEVICE_IN_MIC3
                                        AudioDeviceDescription::CONNECTION_BUILTIN_MIC3),
             PAL_DEVICE_IN_SPEAKER_MIC},
            {makeAudioDeviceDescription(AudioDeviceType::IN_MICROPHONE_MULTI, // AUDIO_DEVICE_IN_MIC4
                                        AudioDeviceDescription::CONNECTION_BUILTIN_MIC4),
             PAL_DEVICE_IN_SPEAKER_MIC},
            {makeAudioDeviceDescription(AudioDeviceType::IN_MICROPHONE_MULTI, // AUDIO_DEVICE_IN_MIC3_MIC4
                                        AudioDeviceDescription::CONNECTION_BUILTIN_MIC3_MIC4),
             PAL_DEVICE_IN_SPEAKER_MIC},
            {makeAudioDeviceDescription(AudioDeviceType::IN_MICROPHONE_MULTI, // AUDIO_DEVICE_IN_MULTI_MIC
                                        AudioDeviceDescription::CONNECTION_BUILTIN_MULTI_MIC),
             PAL_DEVICE_IN_SPEAKER_MIC},
#endif
            {makeAudioDeviceDescription(AudioDeviceType::IN_TELEPHONY_RX),
             PAL_DEVICE_IN_TELEPHONY_RX},

            {makeAudioDeviceDescription(AudioDeviceType::IN_ECHO_REFERENCE),
             PAL_DEVICE_IN_ECHO_REF},
            {makeAudioDeviceDescription(AudioDeviceType::IN_HEADSET,
                                        AudioDeviceDescription::CONNECTION_ANALOG),
             PAL_DEVICE_IN_WIRED_HEADSET},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_HEADSET,
                                        AudioDeviceDescription::CONNECTION_ANALOG),
             PAL_DEVICE_OUT_WIRED_HEADSET},
            {makeAudioDeviceDescription(AudioDeviceType::IN_HEADSET,
                                        AudioDeviceDescription::CONNECTION_BT_SCO),
             PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_HEADSET,
                                        AudioDeviceDescription::CONNECTION_BT_SCO),
             PAL_DEVICE_OUT_BLUETOOTH_SCO},
            {makeAudioDeviceDescription(AudioDeviceType::IN_DEVICE,
                                        AudioDeviceDescription::CONNECTION_HDMI),
             PAL_DEVICE_IN_HDMI},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_DEVICE,
                                        AudioDeviceDescription::CONNECTION_HDMI),
             PAL_DEVICE_OUT_AUX_DIGITAL},
            {makeAudioDeviceDescription(AudioDeviceType::IN_ACCESSORY,
                                        AudioDeviceDescription::CONNECTION_USB),
             PAL_DEVICE_IN_USB_ACCESSORY},
#ifdef SEC_AUDIO_FMRADIO
            {makeAudioDeviceDescription(AudioDeviceType::IN_FM_TUNER,
                                        AudioDeviceDescription::VX_SEC_CONNECTION_FM),
             PAL_DEVICE_IN_FM_TUNER},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_FM,
                                        AudioDeviceDescription::VX_SEC_CONNECTION_FM),
             PAL_DEVICE_OUT_FM},
#else
            {makeAudioDeviceDescription(AudioDeviceType::IN_FM_TUNER), PAL_DEVICE_IN_FM_TUNER},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_FM), PAL_DEVICE_OUT_FM},
#endif

            {makeAudioDeviceDescription(AudioDeviceType::IN_DEVICE,
                                        AudioDeviceDescription::CONNECTION_ANALOG),
             PAL_DEVICE_IN_LINE},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_DEVICE,
                                        AudioDeviceDescription::CONNECTION_ANALOG),
             PAL_DEVICE_OUT_WIRED_HEADPHONE},

            {makeAudioDeviceDescription(AudioDeviceType::IN_DEVICE,
                                        AudioDeviceDescription::CONNECTION_SPDIF),
             PAL_DEVICE_IN_SPDIF},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_DEVICE,
                                        AudioDeviceDescription::CONNECTION_SPDIF),
             PAL_DEVICE_OUT_SPDIF},

            {makeAudioDeviceDescription(AudioDeviceType::IN_DEVICE,
                                        AudioDeviceDescription::CONNECTION_BT_A2DP),
             PAL_DEVICE_IN_BLUETOOTH_A2DP},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_DEVICE,
                                        AudioDeviceDescription::CONNECTION_BT_A2DP),
             PAL_DEVICE_OUT_BLUETOOTH_A2DP},

            {makeAudioDeviceDescription(AudioDeviceType::IN_AFE_PROXY,
                                        AudioDeviceDescription::CONNECTION_VIRTUAL),
             PAL_DEVICE_IN_PROXY},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_AFE_PROXY,
                                        AudioDeviceDescription::CONNECTION_VIRTUAL),
             PAL_DEVICE_OUT_PROXY},

            {makeAudioDeviceDescription(AudioDeviceType::IN_DEVICE,
                                        AudioDeviceDescription::CONNECTION_IP_V4),
             PAL_DEVICE_IN_RECORD_PROXY},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_DEVICE,
                                        AudioDeviceDescription::CONNECTION_IP_V4),
             PAL_DEVICE_OUT_RECORD_PROXY},

            {makeAudioDeviceDescription(AudioDeviceType::OUT_DEVICE,
                                        AudioDeviceDescription::CONNECTION_USB),
             PAL_DEVICE_OUT_USB_DEVICE},
            {makeAudioDeviceDescription(AudioDeviceType::IN_HEADSET,
                                        AudioDeviceDescription::CONNECTION_USB),
             PAL_DEVICE_IN_USB_HEADSET},
            {makeAudioDeviceDescription(AudioDeviceType::IN_DEVICE,
                                        AudioDeviceDescription::CONNECTION_USB),
             PAL_DEVICE_IN_USB_HEADSET},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_HEADSET,
                                        AudioDeviceDescription::CONNECTION_USB),
             PAL_DEVICE_OUT_USB_HEADSET},

            {makeAudioDeviceDescription(AudioDeviceType::IN_HEADSET,
                                        AudioDeviceDescription::CONNECTION_BT_LE),
             PAL_DEVICE_IN_BLUETOOTH_BLE},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_HEADSET,
                                        AudioDeviceDescription::CONNECTION_BT_LE),
             PAL_DEVICE_OUT_BLUETOOTH_BLE},
            {makeAudioDeviceDescription(AudioDeviceType::OUT_HEARING_AID,
                                        AudioDeviceDescription::CONNECTION_WIRELESS),
             PAL_DEVICE_OUT_HEARING_AID}};
    return pairs;
}

// AudioFormat Conversions
AudioFormatDescription make_AudioFormatDescription(AudioFormatType type) {
    AudioFormatDescription result;
    result.type = type;
    return result;
}

AudioFormatDescription make_AudioFormatDescription(PcmType pcm) {
    auto result = make_AudioFormatDescription(AudioFormatType::PCM);
    result.pcm = pcm;
    return result;
}

AudioFormatDescription make_AudioFormatDescription(const std::string& encoding) {
    AudioFormatDescription result;
    result.type = ::aidl::android::media::audio::common::AudioFormatType::NON_PCM;
    result.encoding = encoding;
    return result;
}

AudioFormatDescription make_AudioFormatDescription(PcmType transport, const std::string& encoding) {
    auto result = make_AudioFormatDescription(encoding);
    result.pcm = transport;
    return result;
}

using FormatPair = std::pair<pal_audio_fmt_t, AudioFormatDescription>;
using FormatPairs = std::vector<FormatPair>;

FormatPairs getFormatPairs() {
    // No duplicates on second entry
    FormatPairs pairs = {{
            {PAL_AUDIO_FMT_PCM_S16_LE, make_AudioFormatDescription(PcmType::INT_16_BIT)},
            {PAL_AUDIO_FMT_PCM_S8, make_AudioFormatDescription(PcmType::UINT_8_BIT)},
            {PAL_AUDIO_FMT_PCM_S32_LE, make_AudioFormatDescription(PcmType::INT_32_BIT)},
            {PAL_AUDIO_FMT_PCM_S24_LE, make_AudioFormatDescription(PcmType::FIXED_Q_8_24)},
            {PAL_AUDIO_FMT_PCM_S32_LE, make_AudioFormatDescription(PcmType::FLOAT_32_BIT)},
            {PAL_AUDIO_FMT_PCM_S24_3LE, make_AudioFormatDescription(PcmType::INT_24_BIT)},
            {PAL_AUDIO_FMT_AAC,
             make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_AAC_MP4)},
            {PAL_AUDIO_FMT_AAC_LATM,
             make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_AAC)},
            {PAL_AUDIO_FMT_AAC_ADTS,
             make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS)},
            {PAL_AUDIO_FMT_AAC_ADIF,
             make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_AAC_ADIF)},
            {PAL_AUDIO_FMT_AAC,
             make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_AAC_LC)},
#ifdef SEC_AUDIO_OFFLOAD             
            {PAL_AUDIO_FMT_AAC_ADTS,
             make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_LC)},
#else
             make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_LC)},
#endif             
            {PAL_AUDIO_FMT_AAC,
             make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_HE_V1)},
            {PAL_AUDIO_FMT_AAC,
             make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_AAC_ADTS_HE_V2)},
            {PAL_AUDIO_FMT_AAC,
             make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_AAC_HE_V1)},
            {PAL_AUDIO_FMT_AAC,
             make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_AAC_HE_V2)},
            {PAL_AUDIO_FMT_MP3, make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_MPEG)},
            {PAL_AUDIO_FMT_FLAC, make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_FLAC)},
            {PAL_AUDIO_FMT_VORBIS,
             make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_VORBIS)},
            {PAL_AUDIO_FMT_ALAC, make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_ALAC)},
            {PAL_AUDIO_FMT_WMA_STD,
             make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_WMA)},
            {PAL_AUDIO_FMT_APE, make_AudioFormatDescription("audio/x-ape")},
            {PAL_AUDIO_FMT_WMA_PRO, make_AudioFormatDescription("audio/x-ms-wma.pro")},
            {PAL_AUDIO_FMT_OPUS, make_AudioFormatDescription(::android::MEDIA_MIMETYPE_AUDIO_OPUS)},
    }};
    return pairs;
}

template <typename S, typename T>
std::map<S, T> make_DirectMap(const std::vector<std::pair<S, T>>& v) {
    std::map<S, T> result(v.begin(), v.end());
    if (result.size() != v.size()) {
        LOG(FATAL) << __func__ << "Duplicate key elements detected";
    }
    return result;
}

template <typename S, typename T>
std::map<T, S> make_ReverseMap(const std::vector<std::pair<S, T>>& v) {
    std::map<T, S> result;
    std::transform(v.begin(), v.end(), std::inserter(result, result.begin()),
                   [](const std::pair<S, T>& p) { return std::make_pair(p.second, p.first); });
    if (result.size() != v.size()) {
        LOG(FATAL) << __func__ << "Duplicate key elements detected";
    }
    return result;
}

using AidlToPalDeviceMap =
        std::map<::aidl::android::media::audio::common::AudioDeviceDescription, pal_device_id_t>;
using AidlToPalAudioFormatMap =
        std::map<::aidl::android::media::audio::common::AudioFormatDescription, pal_audio_fmt_t>;

const static AidlToPalDeviceMap kAidlToPalDeviceMap =
        make_DirectMap<AudioDeviceDescription, pal_device_id_t>(getDevicePairs());
const static AidlToPalAudioFormatMap kAidlToPalAudioFormatMap =
        make_ReverseMap<pal_audio_fmt_t, AudioFormatDescription>(getFormatPairs());

// static
std::unique_ptr<pal_channel_info> PlatformConverter::getPalChannelInfoForChannelCount(
        int count) noexcept {
    auto channelInfo = std::make_unique<pal_channel_info>();
    channelInfo->channels = count;
    if (count == 1) {
        channelInfo->ch_map[0] = PAL_CHMAP_CHANNEL_FL;
    } else if (count == 2) {
        channelInfo->ch_map[0] = PAL_CHMAP_CHANNEL_FL;
        channelInfo->ch_map[1] = PAL_CHMAP_CHANNEL_FR;
    } else if (count == 3) {
        channelInfo->ch_map[0] = PAL_CHMAP_CHANNEL_FL;
        channelInfo->ch_map[1] = PAL_CHMAP_CHANNEL_FR;
        channelInfo->ch_map[2] = PAL_CHMAP_CHANNEL_C;
    } else if (count == 4) {
        channelInfo->ch_map[0] = PAL_CHMAP_CHANNEL_FL;
        channelInfo->ch_map[1] = PAL_CHMAP_CHANNEL_FR;
        channelInfo->ch_map[2] = PAL_CHMAP_CHANNEL_C;
        channelInfo->ch_map[3] = PAL_CHMAP_CHANNEL_LFE;
    } else if (count == 5) {
        channelInfo->ch_map[0] = PAL_CHMAP_CHANNEL_FL;
        channelInfo->ch_map[1] = PAL_CHMAP_CHANNEL_FR;
        channelInfo->ch_map[2] = PAL_CHMAP_CHANNEL_C;
        channelInfo->ch_map[3] = PAL_CHMAP_CHANNEL_LFE;
        channelInfo->ch_map[4] = PAL_CHMAP_CHANNEL_RC;
    } else if (count == 6) {
        channelInfo->ch_map[0] = PAL_CHMAP_CHANNEL_FL;
        channelInfo->ch_map[1] = PAL_CHMAP_CHANNEL_FR;
        channelInfo->ch_map[2] = PAL_CHMAP_CHANNEL_C;
        channelInfo->ch_map[3] = PAL_CHMAP_CHANNEL_LFE;
        channelInfo->ch_map[4] = PAL_CHMAP_CHANNEL_LB;
        channelInfo->ch_map[5] = PAL_CHMAP_CHANNEL_RB;
    } else if (count == 7) {
        channelInfo->ch_map[0] = PAL_CHMAP_CHANNEL_FL;
        channelInfo->ch_map[1] = PAL_CHMAP_CHANNEL_FR;
        channelInfo->ch_map[2] = PAL_CHMAP_CHANNEL_C;
        channelInfo->ch_map[3] = PAL_CHMAP_CHANNEL_LFE;
        channelInfo->ch_map[4] = PAL_CHMAP_CHANNEL_LB;
        channelInfo->ch_map[5] = PAL_CHMAP_CHANNEL_RB;
        channelInfo->ch_map[6] = PAL_CHMAP_CHANNEL_LS;
    } else if (count == 8) {
        channelInfo->ch_map[0] = PAL_CHMAP_CHANNEL_FL;
        channelInfo->ch_map[1] = PAL_CHMAP_CHANNEL_FR;
        channelInfo->ch_map[2] = PAL_CHMAP_CHANNEL_C;
        channelInfo->ch_map[3] = PAL_CHMAP_CHANNEL_LFE;
        channelInfo->ch_map[4] = PAL_CHMAP_CHANNEL_LB;
        channelInfo->ch_map[5] = PAL_CHMAP_CHANNEL_RB;
        channelInfo->ch_map[6] = PAL_CHMAP_CHANNEL_LS;
        channelInfo->ch_map[7] = PAL_CHMAP_CHANNEL_RS;
    } else {
        LOG(ERROR) << __func__ << "channel map not found for channels" << count;
    }
    return std::move(channelInfo);
}

// static
uint16_t PlatformConverter::getBitWidthForAidlPCM(
        const AudioFormatDescription& aidlFormat) noexcept {
    if (aidlFormat.type != AudioFormatType::PCM) {
        return 0;
    }
    if (aidlFormat.pcm == PcmType::UINT_8_BIT) {
        return 8;
    } else if (aidlFormat.pcm == PcmType::INT_16_BIT) {
        return 16;
    } else if (aidlFormat.pcm == PcmType::INT_24_BIT) {
        return 24;
    } else if (aidlFormat.pcm == PcmType::INT_32_BIT) {
        return 32;
    } else if (aidlFormat.pcm == PcmType::FIXED_Q_8_24) {
        return 32;
    } else if (aidlFormat.pcm == PcmType::FLOAT_32_BIT) {
        return 32;
    }
    return 0;
}

#ifdef SEC_AUDIO_SAMSUNGRECORD
uint32_t PlatformConverter::getAudioFormatForAidlPCM(
        const AudioFormatDescription& aidlFormat) noexcept {
    if (aidlFormat.type != AudioFormatType::PCM) {
        return 0xFFFFFFFFu;
    }
    if (aidlFormat.pcm == PcmType::UINT_8_BIT) { // AUDIO_FORMAT_PCM_8_BIT
        return 0x2u;
    } else if (aidlFormat.pcm == PcmType::INT_16_BIT) { // AUDIO_FORMAT_PCM_16_BIT
        return 0x1u;
    } else if (aidlFormat.pcm == PcmType::INT_24_BIT) { // AUDIO_FORMAT_PCM_24_BIT_PACKED
        return 0x6u;
    } else if (aidlFormat.pcm == PcmType::INT_32_BIT) { // AUDIO_FORMAT_PCM_32_BIT
        return 0x3u; 
    } else if (aidlFormat.pcm == PcmType::FIXED_Q_8_24) { // AUDIO_FORMAT_PCM_8_24_BIT
        return 0x4u;
    } else if (aidlFormat.pcm == PcmType::FLOAT_32_BIT) { // AUDIO_FORMAT_PCM_FLOAT
        return 0x5u;
    }
    return 0xFFFFFFFFu;
}
#endif

// static
pal_audio_fmt_t PlatformConverter::getPalFormatId(
        const ::aidl::android::media::audio::common::AudioFormatDescription&
                formatDescription) noexcept {
    auto element = kAidlToPalAudioFormatMap.find(formatDescription);
    if (element == kAidlToPalAudioFormatMap.cend()) {
        LOG(ERROR) << __func__ << " failed to find corressponding pal format for "
                   << formatDescription.toString();
        // no format found hence return range end;
        // Todo have PAL_AUDIO_FMT_INVALID as 0
        return PAL_AUDIO_FMT_COMPRESSED_RANGE_END;
    }
    return element->second;
}

// static
pal_device_id_t PlatformConverter::getPalDeviceId(
        const ::aidl::android::media::audio::common::AudioDeviceDescription&
                deviceDescription) noexcept {
    auto element = kAidlToPalDeviceMap.find(deviceDescription);
    if (element == kAidlToPalDeviceMap.cend()) {
        LOG(ERROR) << __func__ << " failed to find corressponding pal device for "
                   << deviceDescription.toString();
        // no device found hence return 0;
        return PAL_DEVICE_OUT_MIN;
    }
    return element->second;
}

outputFlagsStreamtypeMap populatemOutputFlagsStreamtypeMap() {
    outputFlagsStreamtypeMap result;
    constexpr auto flagCastToint = [](auto flag) { return static_cast<int32_t>(flag); };
    constexpr auto PrimaryPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::PRIMARY));
#ifdef SEC_AUDIO_SUPPORT_MEDIA_OUTPUT
    result[PrimaryPlaybackFlags] = PAL_STREAM_GENERIC;
#else // qc orig.
    result[PrimaryPlaybackFlags] = PAL_STREAM_DEEP_BUFFER;
#endif
    constexpr auto deepBufferPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::DEEP_BUFFER));
    result[deepBufferPlaybackFlags] = PAL_STREAM_DEEP_BUFFER;
    constexpr auto compressOffloadPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::DIRECT) |
                                 1 << flagCastToint(AudioOutputFlags::COMPRESS_OFFLOAD) |
                                 1 << flagCastToint(AudioOutputFlags::NON_BLOCKING) |
                                 1 << flagCastToint(AudioOutputFlags::GAPLESS_OFFLOAD));
    result[compressOffloadPlaybackFlags] = PAL_STREAM_COMPRESSED;
    constexpr auto lowLatencyPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::PRIMARY) |
                                 1 << flagCastToint(AudioOutputFlags::FAST));
    result[lowLatencyPlaybackFlags] = PAL_STREAM_LOW_LATENCY;
    constexpr auto pcmOffloadPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::DIRECT));
    result[pcmOffloadPlaybackFlags] = PAL_STREAM_PCM_OFFLOAD;
    constexpr auto voipPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::VOIP_RX));
    result[voipPlaybackFlags] =  PAL_STREAM_VOIP_RX;
    constexpr auto spatialPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::SPATIALIZER));
    result[spatialPlaybackFlags] = PAL_STREAM_SPATIAL_AUDIO;
    constexpr auto ullPlaybackFlags = static_cast<int32_t>(
            1 << flagCastToint(AudioOutputFlags::FAST) | 1 << flagCastToint(AudioOutputFlags::RAW));
    result[ullPlaybackFlags] = PAL_STREAM_ULTRA_LOW_LATENCY;
    constexpr auto mmapPlaybackFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::DIRECT) |
                                 1 << flagCastToint(AudioOutputFlags::MMAP_NOIRQ));
    result[mmapPlaybackFlags] = PAL_STREAM_ULTRA_LOW_LATENCY;
    constexpr auto inCallMusicFlags =
            static_cast<int32_t>(1 << flagCastToint(AudioOutputFlags::INCALL_MUSIC));
    result[inCallMusicFlags] = PAL_STREAM_VOICE_CALL_MUSIC;
    return result;
}

const static outputFlagsStreamtypeMap kOutputFlagsStreamtypeMap =
        populatemOutputFlagsStreamtypeMap();
pal_stream_type_t PlatformConverter::getPalStreamTypeId(int32_t outputFlag) noexcept {
    return kOutputFlagsStreamtypeMap.at(outputFlag);
}

// static
std::string PlatformConverter::toString() noexcept {
    std::ostringstream os;
    os << "### platform conversion start ###" << std::endl;
    os << "devices: Aidl to PAL" << std::endl;
    for (const auto& [key, value] : kAidlToPalDeviceMap) {
        os << key.toString() << " => " << deviceNameLUT.at(value).c_str() << std::endl;
    }
    os << std::endl << "formats: Aidl to PAL " << std::endl;
    for (const auto& [key, value] : kAidlToPalAudioFormatMap) {
        os << key.toString() << " => "
           << "pal format: 0x" << std::hex << value << std::endl;
    }
    os << "### platform conversion end ###" << std::endl;
    return os.str();
}

} // namespace qti::audio
