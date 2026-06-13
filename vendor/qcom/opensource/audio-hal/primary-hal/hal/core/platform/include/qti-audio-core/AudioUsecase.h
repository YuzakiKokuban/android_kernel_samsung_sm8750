/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <PalDefs.h>
#include <Utils.h>
#include <aidl/android/hardware/audio/common/AudioOffloadMetadata.h>
#include <aidl/android/hardware/audio/common/SinkMetadata.h>
#include <aidl/android/hardware/audio/common/SourceMetadata.h>
#include <aidl/android/hardware/audio/core/IStreamCallback.h>
#include <aidl/android/hardware/audio/core/IStreamOutEventCallback.h>
#include <aidl/android/hardware/audio/core/VendorParameter.h>
#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <aidl/android/media/audio/common/AudioDevice.h>
#include <aidl/android/media/audio/common/AudioFormatDescription.h>
#include <aidl/android/media/audio/common/AudioOffloadInfo.h>
#include <aidl/android/media/audio/common/AudioPortConfig.h>
#include <aidl/android/media/audio/common/Int.h>
#include <android/binder_auto_utils.h>

#include <algorithm>
#include <numeric>
#include <unordered_set>

#define DIV_ROUND_UP(x, y) (((x) + (y) - 1) / (y))
#define ALIGN(x, y) ((y) * DIV_ROUND_UP((x), (y)))
#define DEFAULT_SAMPLE_RATE 48000
namespace qti::audio::core {

// forward declaration
struct PlatformStreamCallback;

enum class Usecase : uint16_t {
    INVALID = 0,
    PRIMARY_PLAYBACK,
    LOW_LATENCY_PLAYBACK,
    DEEP_BUFFER_PLAYBACK,
    ULL_PLAYBACK,
    MMAP_PLAYBACK,
    COMPRESS_OFFLOAD_PLAYBACK,
    PCM_OFFLOAD_PLAYBACK,
    VOIP_PLAYBACK,
    HAPTICS_PLAYBACK,
    SPATIAL_PLAYBACK,
    IN_CALL_MUSIC,
    PCM_RECORD, // Start of record usecases
    FAST_RECORD,
    ULTRA_FAST_RECORD,
    MMAP_RECORD,
    COMPRESS_CAPTURE,
    VOIP_RECORD,
    VOICE_CALL_RECORD,
    HOTWORD_RECORD,
};

#ifdef SEC_AUDIO_COMMON
std::optional<aidl::android::media::audio::common::AudioSource> getMixPortAudioSource(
        const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig) noexcept;
#endif

Usecase getUsecaseTag(const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

std::string getName(const Usecase tag);

/*
 * Equivalent to getPeriodSize/getPeriodCount.
 * where bufferSize = frameCount * frameSize;
 * Single API getBufferConfig can be queried by Stream's to fetch Info about periodsize/count.
 */
struct BufferConfig {
    size_t bufferSize;
    size_t bufferCount;
};

/*
* Each usecase class should provide getLatency, getFrameCount, getBufferConfig which are
* used by Platform.cpp to map to respective usecase.
* getBufferConfig internally needs getBufferSize API.

* Earlier in Hidl, getBufferSize was used by framework and to configure pal streams.
* In AIDL when a patch is created framecount is queried, so instead of buffer size
* frameCount is expected to be returned, so for FWK calls getFrameCount is used,
* and for internal pal setups getBufferConfig is used.
*
* Ideally for pcm formats totalBytes (bufferSizeInBytes) = framecount * framesize.
* while framesize = (channelCount * sizeof(audio_format)).

* To summerize : getBufferSize = getFrameCount(portConfig) * getFrameSizeInBytes()
*
* However, for compress usecase, frameSize is 1, so bufferSize = getFrameSize.
*
* UsecaseConfig template is helpful to declare getBufferConfig and getBufferSize for
* each usecase, each new usecase can extend UsecaseConfig.
* As stated before, framesizes are different for pcm and compress types.
* UsecaseConfig by default uses pcm config, to use a pcm usecase extend like this
* class PcmUsecase : public UsecaseConfig <PcmUsecase>
* To define a compress usecase 1 can use as below:
* class CompressUsecase : public UsecaseConfig <CompressUsecase, false >
*/

template <typename Usecase, bool IsPcm = true>
class UsecaseConfig {
  public:
    /*
     * brief create getBufferConfig definition for the usecases.
     * BufferConfig publishes the bufferCount and bufferSize needed to configure the pal streams.
     * To utilize this API, properly configure kPeriodCount and getBufferSize in the usecase.
     */
    static BufferConfig getBufferConfig(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig) {
        BufferConfig config;
        config.bufferCount = Usecase::kPeriodCount;
        config.bufferSize = Usecase::getBufferSize(mixPortConfig);
        return config;
    }

    /*
     * brief create getBufferSize definition based on if usecase is pcm or non pcm.
     * For pcm case frameSize is calculated based on channel count and format.
     * for compress usecases frameSize is used as 1.
     */
    static size_t getBufferSize(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig) {
        size_t frameCount = Usecase::getFrameCount(mixPortConfig);
        size_t frameSizeInBytes = 1;
        if (IsPcm) {
            frameSizeInBytes = ::aidl::android::hardware::audio::common::getFrameSizeInBytes(
                    mixPortConfig.format.value(), mixPortConfig.channelMask.value());
        }
        return frameCount * frameSizeInBytes;
    }
};

/**
 * This port is opened by default and receives routing, audio mode and volume
 * controls related to voice calls
 **/

class PrimaryPlayback : public UsecaseConfig<PrimaryPlayback> {
  public:
#ifdef SEC_AUDIO_COMMON
    constexpr static size_t kPeriodCount = 4;
    constexpr static size_t kPeriodDurationMs = 20;
#else
    constexpr static size_t kPeriodCount = 2;
    constexpr static size_t kPeriodDurationMs = 40;
#endif
    constexpr static size_t kPlatformDelayMs = 29;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kPeriodDurationMs * kPeriodCount + kPlatformDelayMs; }
};

class DeepBufferPlayback : public UsecaseConfig<DeepBufferPlayback> {
  public:
#ifdef SEC_AUDIO_COMMON
    constexpr static size_t kPeriodCount = 4;
    constexpr static size_t kPeriodDurationMs = 20;
#else
    constexpr static size_t kPeriodCount = 2;
    constexpr static size_t kPeriodDurationMs = 40;
#endif
    constexpr static size_t kPlatformDelayMs = 49;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kPeriodDurationMs * kPeriodCount + kPlatformDelayMs; }
};

class LowLatencyPlayback : public UsecaseConfig<LowLatencyPlayback> {
  public:
    constexpr static size_t kPeriodCount = 2;
    constexpr static size_t kPlatformDelayMs = 13;
    constexpr static size_t kPeriodDurationMs = 4;
    static std::unordered_set<size_t> kSupportedFrameSizes;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kPeriodDurationMs * kPeriodCount + kPlatformDelayMs; }
};

class UllPlayback : public UsecaseConfig<UllPlayback> {
  public:
    constexpr static size_t kPlatformDelayMs = 4;
    constexpr static uint32_t kPeriodCount = 512;
    constexpr static size_t kPeriodDurationMs = 3;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kPeriodDurationMs + kPlatformDelayMs; }
};

class MmapUsecaseBase {
  public:
    virtual ~MmapUsecaseBase() {}
    virtual void setPalHandle(pal_stream_handle_t* handle);
    virtual int32_t createMMapBuffer(int64_t frameSize, int32_t* fd, int64_t* burstSizeFrames,
                                     int32_t* flags, int32_t* bufferSizeFrames);
    virtual int32_t getMMapPosition(int64_t* frames, int64_t* timeNs);
    virtual int32_t start();
    virtual int32_t stop();
  protected:
    pal_stream_handle_t* mPalHandle{nullptr};
    bool mIsStarted = false;
};

class MMapPlayback : public MmapUsecaseBase, public UsecaseConfig<MMapPlayback> {
  public:
    constexpr static size_t kPeriodDurationMs = 1;
    constexpr static size_t kPlatformDelayMs = 3;
    constexpr static uint32_t kPeriodCount = 512;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kPeriodDurationMs + kPlatformDelayMs; }
};

class CompressPlayback : public UsecaseConfig<CompressPlayback, false /*IsPcm*/> {
  public:
    static constexpr size_t kPeriodSize = 32 * 1024;
    static constexpr size_t kPeriodCount = 4;
    static constexpr int32_t kLatencyMs = 50;
    static constexpr size_t kPlatformDelayMs = 30;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kLatencyMs; }

    class Flac final {
      public:
        static constexpr size_t kPeriodSize = 256 * 1024;
        inline const static std::string kMinBlockSize{"music_offload_flac_min_blk_size"};
        inline const static std::string kMaxBlockSize{"music_offload_flac_max_blk_size"};
        inline const static std::string kMinFrameSize{"music_offload_flac_min_frame_size"};
        inline const static std::string kMaxFrameSize{"music_offload_flac_max_frame_size"};
    };
    class Alac final {
      public:
        inline const static std::string kFrameLength{"music_offload_alac_frame_length"};
        inline const static std::string kCompatVer{"music_offload_alac_compatible_version"};
        inline const static std::string kBitDepth{"music_offload_alac_bit_depth"};
        inline const static std::string kPb{"music_offload_alac_pb"};
        inline const static std::string kMb{"music_offload_alac_mb"};
        inline const static std::string kKb{"music_offload_alac_kb"};
        inline const static std::string kNumChannels{"music_offload_alac_num_channels"};
        inline const static std::string kMaxRun{"music_offload_alac_max_run"};
        inline const static std::string kMaxFrameBytes{"music_offload_alac_max_frame_bytes"};
        inline const static std::string kBitRate{"music_offload_alac_avg_bit_rate"};
        inline const static std::string kSamplingRate{"music_offload_alac_sampling_rate"};
        inline const static std::string kChannelLayoutTag{"music_offload_alac_channel_layout_tag"};
    };
    class Vorbis final {
      public:
        inline const static std::string kBitStreamFormat{"music_offload_vorbis_bitstream_fmt"};
    };
    class Ape final {
      public:
        inline const static std::string kCompatibleVersion{"music_offload_ape_compatible_version"};
        inline const static std::string kCompressionLevel{"music_offload_ape_compression_level"};
        inline const static std::string kFormatFlags{"music_offload_ape_format_flags"};
        inline const static std::string kBlocksPerFrame{"music_offload_ape_blocks_per_frame"};
        inline const static std::string kFinalFrameBlocks{"music_offload_ape_final_frame_blocks"};
        inline const static std::string kTotalFrames{"music_offload_ape_total_frames"};
        inline const static std::string kBitsPerSample{"music_offload_ape_bits_per_sample"};
        inline const static std::string kNumChannels{"music_offload_ape_num_channels"};
        inline const static std::string kSampleRate{"music_offload_ape_sample_rate"};
        inline const static std::string kSeekTablePresent{"music_offload_seek_table_present"};
    };
    class Wma final {
      public:
        inline const static std::string kFormatTag{"music_offload_wma_format_tag"};
        inline const static std::string kBlockAlign{"music_offload_wma_block_align"};
        inline const static std::string kBitPerSample{"music_offload_wma_bit_per_sample"};
        inline const static std::string kChannelMask{"music_offload_wma_channel_mask"};
        inline const static std::string kEncodeOption{"music_offload_wma_encode_option"};
        inline const static std::string kEncodeOption1{"music_offload_wma_encode_option1"};
        inline const static std::string kEncodeOption2{"music_offload_wma_encode_option2"};
    };
    class Opus final {
      public:
        inline const static std::string kBitStreamFormat{"music_offload_opus_bitstream_format"};
        inline const static std::string kPayloadType{"music_offload_opus_payload_type"};
        inline const static std::string kVersion{"music_offload_opus_version"};
        inline const static std::string kNumChannels{"music_offload_opus_num_channels"};
        inline const static std::string kPreSkip{"music_offload_opus_pre_skip"};
        inline const static std::string kOutputGain{"music_offload_opus_output_gain"};
        inline const static std::string kMappingFamily{"music_offload_opus_mapping_family"};
        inline const static std::string kStreamCount{"music_offload_opus_stream_count"};
        inline const static std::string kCoupledCount{"music_offload_opus_coupled_count"};
        inline const static std::string kChannelMap0{"music_offload_opus_channel_map0"};
        inline const static std::string kChannelMap1{"music_offload_opus_channel_map1"};
        inline const static std::string kChannelMap2{"music_offload_opus_channel_map2"};
        inline const static std::string kChannelMap3{"music_offload_opus_channel_map3"};
        inline const static std::string kChannelMap4{"music_offload_opus_channel_map4"};
        inline const static std::string kChannelMap5{"music_offload_opus_channel_map5"};
        inline const static std::string kChannelMap6{"music_offload_opus_channel_map6"};
        inline const static std::string kChannelMap7{"music_offload_opus_channel_map7"};
    };

    static int32_t palCallback(pal_stream_handle_t* palHandle, uint32_t eventId,
                               uint32_t* eventData, uint32_t eventSize, uint64_t cookie);

    explicit CompressPlayback(
            const ::aidl::android::media::audio::common::AudioOffloadInfo& offloadInfo,
            PlatformStreamCallback* const platformStreamCallback,
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);
    /* To reconfigure the codec, gapless info */
    void setAndConfigureCodecInfo(pal_stream_handle_t* handle);
    void configureGapless(pal_stream_handle_t* handle);
    ndk::ScopedAStatus getVendorParameters(
            const std::vector<std::string>& in_ids,
            std::vector<::aidl::android::hardware::audio::core::VendorParameter>* _aidl_return);
    ndk::ScopedAStatus setVendorParameters(
            const std::vector<::aidl::android::hardware::audio::core::VendorParameter>&
                    in_parameters,
            bool in_async);
    void updateOffloadMetadata(
            const ::aidl::android::hardware::audio::common::AudioOffloadMetadata& offloadMetaData);
    void updateSourceMetadata(
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetaData);
    int64_t getPositionInFrames(pal_stream_handle_t* palHandle);
    void onFlush();
    bool isGaplessConfigured() const noexcept { return mIsGaplessConfigured; }

  protected:
    void configureDefault();
    // configure the codec info which is cached already
    bool configureCodecInfo() const;
    // configure the gapless info which is cached already
    bool configureGapLessMetadata();

  protected:
    // dynamic compress info
    ::aidl::android::hardware::audio::common::AudioOffloadMetadata mOffloadMetadata{};
    const ::aidl::android::hardware::audio::common::SourceMetadata* mSourceMetadata{nullptr};
    // this is static info at the stream creation, for dynamic info check AudioOffloadMetadata
    const ::aidl::android::media::audio::common::AudioOffloadInfo& mOffloadInfo;
    uint16_t mCompressBitWidth{0};
    pal_stream_handle_t* mCompressPlaybackHandle{nullptr};
    pal_snd_dec_t mPalSndDec{};
    int32_t mSampleRate;
    ::aidl::android::media::audio::common::AudioFormatDescription mCompressFormat;
    ::aidl::android::media::audio::common::AudioChannelLayout mChannelLayout;
    int32_t mBitWidth;
    int64_t mTotalDSPFrames{0};
    int64_t mPrevFrames{0};
    const ::aidl::android::media::audio::common::AudioPortConfig& mMixPortConfig;
    PlatformStreamCallback * const mPlatformStreamCallback;
    std::atomic<bool> mIsGaplessConfigured = false;
};

class PcmOffloadPlayback : public UsecaseConfig<PcmOffloadPlayback> {
  public:
    explicit PcmOffloadPlayback(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig)
        : mMixPortConfig(mixPortConfig) {}
    constexpr static size_t kPeriodDurationMs = 80;
    constexpr static size_t kPeriodCount = 2;
    constexpr static size_t kPlatformDelayMs = 30;
    constexpr static size_t kMinPeriodSize = 512;
    constexpr static size_t kMaxPeriodSize = 240 * 1024;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kPeriodDurationMs * kPeriodCount + kPlatformDelayMs; }

    int64_t getPositionInFrames(pal_stream_handle_t* palHandle);
    void onFlush();

  private:
    int64_t mTotalDSPFrames{0};
    int64_t mPrevFrames{0};
    const ::aidl::android::media::audio::common::AudioPortConfig& mMixPortConfig;
};

class VoipPlayback : public UsecaseConfig<VoipPlayback> {
  public:
    constexpr static size_t kPeriodDurationMs = 20;
    constexpr static size_t kPeriodCount = 2;
    constexpr static size_t kPlatformDelayMs = 30;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kPeriodDurationMs * kPeriodCount + kPlatformDelayMs; }
};

class SpatialPlayback : public UsecaseConfig<SpatialPlayback> {
  public:
    constexpr static size_t kPeriodDurationMs = 10;
    constexpr static size_t kPeriodCount = 2;
    constexpr static size_t kPlatformDelayMs = 13;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kPeriodDurationMs * kPeriodCount + kPlatformDelayMs; }
};

class InCallMusic : public UsecaseConfig<InCallMusic> {
  public:
    constexpr static size_t kPeriodCount = 4;
    constexpr static size_t kPlatformDelayMs = 0;
    constexpr static size_t kPeriodDurationMs = 20;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return PrimaryPlayback::getLatency(); }
};

class HapticsPlayback : public UsecaseConfig<HapticsPlayback> {
  public:
#ifdef SEC_AUDIO_SUPPORT_HAPTIC_PLAYBACK // Set the same as Generic
    constexpr static size_t kPeriodCount = 4;
    constexpr static size_t kPlatformDelayMs = 30;
    constexpr static size_t kPeriodDurationMs = 20;
#else
    constexpr static size_t kPeriodCount = 2;
    constexpr static size_t kPlatformDelayMs = 30;
    constexpr static size_t kPeriodDurationMs = 4;
#endif

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kPeriodDurationMs * kPeriodCount + kPlatformDelayMs; }
};

class PcmRecord : public UsecaseConfig<PcmRecord> {
  public:
    constexpr static uint32_t kCaptureDurationMs = 20;
    constexpr static uint32_t kPeriodCount = 4;
    constexpr static size_t kFMQMinFrameSize = 160;
    constexpr static size_t kPlatformDelayMs = 20;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kCaptureDurationMs * kPeriodCount + kPlatformDelayMs; }
};

class FastRecord : public UsecaseConfig<FastRecord> {
  public:
    constexpr static uint32_t kCaptureDurationMs = 5;
    constexpr static size_t kPeriodCount = 4;
    constexpr static size_t kPlatformDelayMs = 5;
    bool mIsWFDCapture{false};
    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kPlatformDelayMs; }
};

class UltraFastRecord : public UsecaseConfig<UltraFastRecord> {
  public:
#ifdef SEC_AUDIO_SAMSUNGRECORD
    constexpr static int32_t kSampleRate = 48000;
#endif
    constexpr static uint32_t kCaptureDurationMs = 2;
    // The below values at the moment are not generic, TODO make generic
    constexpr static size_t kPeriodCount = 512;
    constexpr static size_t kPlatformDelayMs = 2;
    bool mIsWFDCapture{false};

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kPlatformDelayMs; }
};

class MMapRecord : public MmapUsecaseBase, public UsecaseConfig<MMapRecord> {
  public:
    constexpr static uint32_t kCaptureDurationMs = 2;
    constexpr static size_t kPeriodCount = 512;
    constexpr static size_t kPlatformDelayMs = 4;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kPlatformDelayMs; }
};

class HotwordRecord : public UsecaseConfig<HotwordRecord> {
  public:
    constexpr static uint32_t kPeriodCount = 4;
    constexpr static size_t kPlatformDelayMs = 0;
    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    // use same as pcm record
    static int32_t getLatency() { return PcmRecord::getLatency(); }
    pal_stream_handle_t* getPalHandle(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    bool isStRecord() { return mIsStRecord; }
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
    void enableSeamless(bool enable) { mIsSeamless = enable; }
#endif

  private:
    bool mIsStRecord{false};
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
    bool mIsSeamless{false};
#endif
};

class VoipRecord : public UsecaseConfig<VoipRecord> {
  public:
    constexpr static uint32_t kCaptureDurationMs = 20;
    constexpr static uint32_t kPeriodCount = 4;
    constexpr static size_t kPlatformDelayMs = 0;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return PcmRecord::getLatency(); }
};

class VoiceCallRecord : public UsecaseConfig<VoiceCallRecord> {
  public:
    constexpr static size_t kCaptureDurationMs = 20;
    constexpr static size_t kPeriodCount = 2;
    constexpr static size_t kPlatformDelayMs = 0;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    pal_incall_record_direction getRecordDirection(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);
    static int32_t getLatency() { return PcmRecord::getLatency(); }
};

class CompressCapture : public UsecaseConfig<CompressCapture, false /*IsPcm*/> {
  public:
    constexpr static size_t kPlatformDelayMs = 20;

    static size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);

    static int32_t getLatency() { return kPlatformDelayMs; }
    class Aac final {
      public:
        inline static const std::string kDSPAacBitRate{"dsp_aac_audio_bitrate"};
        inline static const std::string kDSPAacGlobalCutoffFrequency{
                "dsp_aac_audio_global_cutoff_frequency"};
        enum EncodingMode {
            LC = 0x02,
            SBR = 0x05,
            PS = 0x1D,
        };
        enum EncodingFormat {
            ADTS = 0x00,
            LOAS = 0x01,
            RAW = 0x03,
            LATM = 0x04,
        };

        constexpr static uint32_t kAacLcPCMSamplesPerFrame = 1024;
        constexpr static uint32_t kHeAacPCMSamplesPerFrame = 2048;
        constexpr static int32_t kAacLcMonoMinSupportedBitRate = 8000;
        constexpr static int32_t kAacLcStereoMinSupportedBitRate = 16000;
        constexpr static int32_t kHeAacMonoMinSupportedBitRate1 = 10000;
        constexpr static int32_t kHeAacMonoMinSupportedBitRate2 = 12000;
        constexpr static int32_t kHeAacStereoMinSupportedBitRate1 = 18000;
        constexpr static int32_t kHeAacStereoMinSupportedBitRate2 = 24000;
        constexpr static int32_t kHeAacPsStereoMinSupportedBitRate1 = 10000;
        constexpr static int32_t kHeAacPsStereoMinSupportedBitRate2 = 12000;

        constexpr static int32_t kAacLcMonoMaxSupportedBitRate = 192000;
        constexpr static int32_t kAacLcStereoMaxSupportedBitRate = 384000;
        constexpr static int32_t kHeAacMonoMaxSupportedBitRate = 192000;
        constexpr static int32_t kHeAacStereoMaxSupportedBitRate = 192000;
        constexpr static int32_t kHeAacPstereoMaxSupportedBitRate = 192000;
        static const uint32_t KAacMaxOutputSize = 2048;  // bytes
        static const int32_t kAacDefaultBitrate = 36000; // bps
    };

    constexpr static size_t kPeriodCount = 4;
    explicit CompressCapture(
            const ::aidl::android::media::audio::common::AudioFormatDescription& format,
            int32_t sampleRate,
            const ::aidl::android::media::audio::common::AudioChannelLayout& channelLayout);
    void setPalHandle(pal_stream_handle_t* handle);
    ndk::ScopedAStatus setVendorParameters(
            const std::vector<::aidl::android::hardware::audio::core::VendorParameter>&
                    in_parameters,
            bool in_async);
    size_t getLatencyMs();
    ndk::ScopedAStatus getVendorParameters(
            const std::vector<std::string>& in_ids,
            std::vector<::aidl::android::hardware::audio::core::VendorParameter>* _aidl_return);
    bool configureCodecInfo();
    int32_t getAACMinBitrateValue();

    int32_t getAACMaxBitrateValue();

    uint32_t getAACMaxBufferSize();
    void setAACDSPBitRate();
    void advanceReadCount();
    int64_t getPositionInFrames();

    private:
    const ::aidl::android::media::audio::common::AudioFormatDescription& mCompressFormat;
    const ::aidl::android::media::audio::common::AudioChannelLayout& mChannelLayout;
    int32_t mSampleRate{};
    size_t mPCMSamplesPerFrame{0};
    pal_stream_handle_t* mCompressHandle{nullptr};
    size_t mNumReadCalls{0};
    pal_snd_enc_t mPalSndEnc{};
};

} // namespace qti::audio::core
