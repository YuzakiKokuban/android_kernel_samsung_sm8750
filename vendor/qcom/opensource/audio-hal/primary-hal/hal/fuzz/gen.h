// GENERATED. DO NOT EDIT.
// Using: out/host/linux-x86/bin/aidl --lang=cpp-fuzzer -h /dev/null -o /dev/null --structured --stability vintf -Nhardware/interfaces/audio/aidl -Nsystem/hardware/interfaces/media/aidl -Nhardware/interfaces/common/aidl -Nhardware/interfaces/common/fmq/aidl hardware/interfaces/audio/aidl/android/hardware/audio/core/IBluetoothA2dp.aidl hardware/interfaces/audio/aidl/android/hardware/audio/core/IBluetooth.aidl hardware/interfaces/audio/aidl/android/hardware/audio/core/IBluetoothLe.aidl hardware/interfaces/audio/aidl/android/hardware/audio/core/IConfig.aidl hardware/interfaces/audio/aidl/android/hardware/audio/core/IModule.aidl hardware/interfaces/audio/aidl/android/hardware/audio/core/IStreamCallback.aidl hardware/interfaces/audio/aidl/android/hardware/audio/core/IStreamCommon.aidl hardware/interfaces/audio/aidl/android/hardware/audio/core/IStreamIn.aidl hardware/interfaces/audio/aidl/android/hardware/audio/core/IStreamOut.aidl hardware/interfaces/audio/aidl/android/hardware/audio/core/IStreamOutEventCallback.aidl hardware/interfaces/audio/aidl/android/hardware/audio/core/ITelephony.aidl

#include "aidl/android/hardware/audio/common/AudioOffloadMetadata.h"
#include "aidl/android/hardware/audio/common/PlaybackTrackMetadata.h"
#include "aidl/android/hardware/audio/common/RecordTrackMetadata.h"
#include "aidl/android/hardware/audio/common/SinkMetadata.h"
#include "aidl/android/hardware/audio/common/SourceMetadata.h"
#include "aidl/android/hardware/audio/core/AudioPatch.h"
#include "aidl/android/hardware/audio/core/IStreamCallback.h"
#include "aidl/android/hardware/audio/core/IStreamOutEventCallback.h"
#include "aidl/android/hardware/audio/core/ModuleDebug.h"
#include "aidl/android/hardware/audio/core/VendorParameter.h"
#include "aidl/android/hardware/audio/effect/IEffect.h"
#include "aidl/android/media/audio/common/AudioChannelLayout.h"
#include "aidl/android/media/audio/common/AudioConfigBase.h"
#include "aidl/android/media/audio/common/AudioContentType.h"
#include "aidl/android/media/audio/common/AudioDevice.h"
#include "aidl/android/media/audio/common/AudioDeviceAddress.h"
#include "aidl/android/media/audio/common/AudioDeviceDescription.h"
#include "aidl/android/media/audio/common/AudioDeviceType.h"
#include "aidl/android/media/audio/common/AudioDualMonoMode.h"
#include "aidl/android/media/audio/common/AudioEncapsulationMode.h"
#include "aidl/android/media/audio/common/AudioEncapsulationType.h"
#include "aidl/android/media/audio/common/AudioFormatDescription.h"
#include "aidl/android/media/audio/common/AudioFormatType.h"
#include "aidl/android/media/audio/common/AudioGain.h"
#include "aidl/android/media/audio/common/AudioGainConfig.h"
#include "aidl/android/media/audio/common/AudioIoFlags.h"
#include "aidl/android/media/audio/common/AudioLatencyMode.h"
#include "aidl/android/media/audio/common/AudioMMapPolicyType.h"
#include "aidl/android/media/audio/common/AudioMode.h"
#include "aidl/android/media/audio/common/AudioOffloadInfo.h"
#include "aidl/android/media/audio/common/AudioPlaybackRate.h"
#include "aidl/android/media/audio/common/AudioPort.h"
#include "aidl/android/media/audio/common/AudioPortConfig.h"
#include "aidl/android/media/audio/common/AudioPortDeviceExt.h"
#include "aidl/android/media/audio/common/AudioPortExt.h"
#include "aidl/android/media/audio/common/AudioPortMixExt.h"
#include "aidl/android/media/audio/common/AudioPortMixExtUseCase.h"
#include "aidl/android/media/audio/common/AudioProfile.h"
#include "aidl/android/media/audio/common/AudioSource.h"
#include "aidl/android/media/audio/common/AudioStandard.h"
#include "aidl/android/media/audio/common/AudioStreamType.h"
#include "aidl/android/media/audio/common/AudioUsage.h"
#include "aidl/android/media/audio/common/ExtraAudioDescriptor.h"
#include "aidl/android/media/audio/common/PcmType.h"
#include "aidl/android/hardware/audio/core/IBluetooth.h"
#include "aidl/android/hardware/audio/core/IBluetoothA2dp.h"
#include "aidl/android/hardware/audio/core/IBluetoothLe.h"
#include "aidl/android/hardware/audio/core/IConfig.h"
#include "aidl/android/hardware/audio/core/IModule.h"
#include "aidl/android/hardware/audio/core/IStreamCallback.h"
#include "aidl/android/hardware/audio/core/IStreamCommon.h"
#include "aidl/android/hardware/audio/core/IStreamIn.h"
#include "aidl/android/hardware/audio/core/IStreamOut.h"
#include "aidl/android/hardware/audio/core/IStreamOutEventCallback.h"
#include "aidl/android/hardware/audio/core/ITelephony.h"

using aidl::android::hardware::audio::common::AudioOffloadMetadata;
using aidl::android::hardware::audio::common::PlaybackTrackMetadata;
using aidl::android::hardware::audio::common::RecordTrackMetadata;
using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::hardware::audio::core::AudioPatch;
using aidl::android::hardware::audio::core::IStreamCallback;
using aidl::android::hardware::audio::core::IStreamOutEventCallback;
using aidl::android::hardware::audio::core::ModuleDebug;
using aidl::android::hardware::audio::core::VendorParameter;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioConfigBase;
using aidl::android::media::audio::common::AudioContentType;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioDualMonoMode;
using aidl::android::media::audio::common::AudioEncapsulationMode;
using aidl::android::media::audio::common::AudioEncapsulationType;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioGain;
using aidl::android::media::audio::common::AudioGainConfig;
using aidl::android::media::audio::common::AudioIoFlags;
using aidl::android::media::audio::common::AudioLatencyMode;
using aidl::android::media::audio::common::AudioMMapPolicyType;
using aidl::android::media::audio::common::AudioMode;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::AudioPlaybackRate;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortConfig;
using aidl::android::media::audio::common::AudioPortDeviceExt;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::AudioPortMixExt;
using aidl::android::media::audio::common::AudioPortMixExtUseCase;
using aidl::android::media::audio::common::AudioProfile;
using aidl::android::media::audio::common::AudioSource;
using aidl::android::media::audio::common::AudioStandard;
using aidl::android::media::audio::common::AudioStreamType;
using aidl::android::media::audio::common::AudioUsage;
using aidl::android::media::audio::common::ExtraAudioDescriptor;
using aidl::android::media::audio::common::PcmType;
using aidl::android::hardware::audio::core::IBluetooth;
using aidl::android::hardware::audio::core::IBluetoothA2dp;
using aidl::android::hardware::audio::core::IBluetoothLe;
using aidl::android::hardware::audio::core::IConfig;
using aidl::android::hardware::audio::core::IModule;
using aidl::android::hardware::audio::core::IStreamCallback;
using aidl::android::hardware::audio::core::IStreamCommon;
using aidl::android::hardware::audio::core::IStreamIn;
using aidl::android::hardware::audio::core::IStreamOut;
using aidl::android::hardware::audio::core::IStreamOutEventCallback;
using aidl::android::hardware::audio::core::ITelephony;

struct DataProvider : public DataProviderBase {
  DataProvider(const char *data, size_t size): DataProviderBase(data, size) {}
  virtual ~DataProvider() {}

  virtual void genAudioOffloadMetadata(AudioOffloadMetadata &out) {
    gen(out.sampleRate);
    genAudioChannelLayout(out.channelMask);
    gen(out.averageBitRatePerSecond);
    gen(out.delayFrames);
    gen(out.paddingFrames);
  }

  virtual void genPlaybackTrackMetadata(PlaybackTrackMetadata &out) {
    genAudioUsage(out.usage);
    genAudioContentType(out.contentType);
    gen(out.gain);
    genAudioChannelLayout(out.channelMask);
    gen(out.sourceDevice, [this](auto &v) { genAudioDevice(v); });
    gen(out.tags, [this](auto &v) { gen(v); });
  }

  virtual void genRecordTrackMetadata(RecordTrackMetadata &out) {
    genAudioSource(out.source);
    gen(out.gain);
    gen(out.destinationDevice, [this](auto &v) { genAudioDevice(v); });
    genAudioChannelLayout(out.channelMask);
    gen(out.tags, [this](auto &v) { gen(v); });
  }

  virtual void genSinkMetadata(SinkMetadata &out) {
    gen(out.tracks, [this](auto &v) { genRecordTrackMetadata(v); });
  }

  virtual void genSourceMetadata(SourceMetadata &out) {
    gen(out.tracks, [this](auto &v) { genPlaybackTrackMetadata(v); });
  }

  virtual void genAudioPatch(AudioPatch &out) {
    gen(out.id);
    gen(out.sourcePortConfigIds, [this](auto &v) { gen(v); });
    gen(out.sinkPortConfigIds, [this](auto &v) { gen(v); });
    gen(out.minimumStreamBufferSizeFrames);
    gen(out.latenciesMs, [this](auto &v) { gen(v); });
  }

  virtual void genHfpConfig(IBluetooth::HfpConfig &out) {
    gen(out.isEnabled, [this](auto &v) { genBoolean(v); });
    gen(out.sampleRate, [this](auto &v) { genInt(v); });
    gen(out.volume, [this](auto &v) { genFloat(v); });
  }

  virtual void genScoConfig(IBluetooth::ScoConfig &out) {
    gen(out.isEnabled, [this](auto &v) { genBoolean(v); });
    gen(out.isNrecEnabled, [this](auto &v) { genBoolean(v); });
    genMode(out.mode);
    gen(out.debugName, [this](auto &v) { gen(v); });
  }

  virtual void genMode(IBluetooth::ScoConfig::Mode &out) {
    const IBluetooth::ScoConfig::Mode values[] = {
      IBluetooth::ScoConfig::Mode::UNSPECIFIED,
      IBluetooth::ScoConfig::Mode::SCO,
      IBluetooth::ScoConfig::Mode::SCO_WB,
      IBluetooth::ScoConfig::Mode::SCO_SWB,
    };
    out = pick(values);
  }

  virtual void genOpenInputStreamArguments(IModule::OpenInputStreamArguments &out) {
    gen(out.portConfigId);
    genSinkMetadata(out.sinkMetadata);
    gen(out.bufferSizeFrames);
  }

  virtual void genOpenOutputStreamArguments(IModule::OpenOutputStreamArguments &out) {
    gen(out.portConfigId);
    genSourceMetadata(out.sourceMetadata);
    gen(out.offloadInfo, [this](auto &v) { genAudioOffloadInfo(v); });
    gen(out.bufferSizeFrames);
    // NOTE: `out.callback' is an interface
    // NOTE: `out.eventCallback' is an interface
  }

  virtual void genScreenRotation(IModule::ScreenRotation &out) {
    const IModule::ScreenRotation values[] = {
      IModule::ScreenRotation::DEG_0,
      IModule::ScreenRotation::DEG_90,
      IModule::ScreenRotation::DEG_180,
      IModule::ScreenRotation::DEG_270,
    };
    out = pick(values);
  }


  virtual void genMicrophoneDirection(IStreamIn::MicrophoneDirection &out) {
    const IStreamIn::MicrophoneDirection values[] = {
      IStreamIn::MicrophoneDirection::UNSPECIFIED,
      IStreamIn::MicrophoneDirection::FRONT,
      IStreamIn::MicrophoneDirection::BACK,
      IStreamIn::MicrophoneDirection::EXTERNAL,
    };
    out = pick(values);
  }


  virtual void genTelecomConfig(ITelephony::TelecomConfig &out) {
    gen(out.voiceVolume, [this](auto &v) { genFloat(v); });
    genTtyMode(out.ttyMode);
    gen(out.isHacEnabled, [this](auto &v) { genBoolean(v); });
  }

  virtual void genTtyMode(ITelephony::TelecomConfig::TtyMode &out) {
    const ITelephony::TelecomConfig::TtyMode values[] = {
      ITelephony::TelecomConfig::TtyMode::UNSPECIFIED,
      ITelephony::TelecomConfig::TtyMode::OFF,
      ITelephony::TelecomConfig::TtyMode::FULL,
      ITelephony::TelecomConfig::TtyMode::HCO,
      ITelephony::TelecomConfig::TtyMode::VCO,
    };
    out = pick(values);
  }

  virtual void genModuleDebug(ModuleDebug &out) {
    gen(out.simulateDeviceConnections);
    gen(out.streamTransientStateDelayMs);
  }

  virtual void genVendorParameter(VendorParameter &out) {
    gen(out.id);
    // NOTE: `out.ext' is a ParcelableHolder
  }


  virtual void genAudioChannelLayout(AudioChannelLayout &out) {
    int tag = p.ConsumeIntegralInRange<int>(0, 4);
    switch (tag) {
      case 0: {
        int32_t val;
        gen(val);
        out.set<AudioChannelLayout::Tag::none>(val);
        break;
      }
      case 1: {
        int32_t val;
        gen(val);
        out.set<AudioChannelLayout::Tag::invalid>(val);
        break;
      }
      case 2: {
        int32_t val;
        gen(val);
        out.set<AudioChannelLayout::Tag::indexMask>(val);
        break;
      }
      case 3: {
        int32_t val;
        gen(val);
        out.set<AudioChannelLayout::Tag::layoutMask>(val);
        break;
      }
      case 4: {
        int32_t val;
        gen(val);
        out.set<AudioChannelLayout::Tag::voiceMask>(val);
        break;
      }
    }
  }

  virtual void genAudioConfigBase(AudioConfigBase &out) {
    gen(out.sampleRate);
    genAudioChannelLayout(out.channelMask);
    genAudioFormatDescription(out.format);
  }

  virtual void genAudioContentType(AudioContentType &out) {
    const AudioContentType values[] = {
      AudioContentType::UNKNOWN,
      AudioContentType::SPEECH,
      AudioContentType::MUSIC,
      AudioContentType::MOVIE,
      AudioContentType::SONIFICATION,
      AudioContentType::ULTRASOUND,
    };
    out = pick(values);
  }

  virtual void genAudioDevice(AudioDevice &out) {
    genAudioDeviceDescription(out.type);
    genAudioDeviceAddress(out.address);
  }

  virtual void genAudioDeviceAddress(AudioDeviceAddress &out) {
    int tag = p.ConsumeIntegralInRange<int>(0, 4);
    switch (tag) {
      case 0: {
        ::std::string val;
        gen(val);
        out.set<AudioDeviceAddress::Tag::id>(val);
        break;
      }
      case 1: {
        ::std::vector<uint8_t> val;
        gen(val, [this](auto &v) { gen(v); });
        out.set<AudioDeviceAddress::Tag::mac>(val);
        break;
      }
      case 2: {
        ::std::vector<uint8_t> val;
        gen(val, [this](auto &v) { gen(v); });
        out.set<AudioDeviceAddress::Tag::ipv4>(val);
        break;
      }
      case 3: {
        ::std::vector<int32_t> val;
        gen(val, [this](auto &v) { gen(v); });
        out.set<AudioDeviceAddress::Tag::ipv6>(val);
        break;
      }
      case 4: {
        ::std::vector<int32_t> val;
        gen(val, [this](auto &v) { gen(v); });
        out.set<AudioDeviceAddress::Tag::alsa>(val);
        break;
      }
    }
  }

  virtual void genAudioDeviceDescription(AudioDeviceDescription &out) {
    genAudioDeviceType(out.type);
    gen(out.connection);
  }

  virtual void genAudioDeviceType(AudioDeviceType &out) {
    const AudioDeviceType values[] = {
      AudioDeviceType::NONE,
      AudioDeviceType::IN_DEFAULT,
      AudioDeviceType::IN_ACCESSORY,
      AudioDeviceType::IN_AFE_PROXY,
      AudioDeviceType::IN_DEVICE,
      AudioDeviceType::IN_ECHO_REFERENCE,
      AudioDeviceType::IN_FM_TUNER,
      AudioDeviceType::IN_HEADSET,
      AudioDeviceType::IN_LOOPBACK,
      AudioDeviceType::IN_MICROPHONE,
      AudioDeviceType::IN_MICROPHONE_BACK,
      AudioDeviceType::IN_SUBMIX,
      AudioDeviceType::IN_TELEPHONY_RX,
      AudioDeviceType::IN_TV_TUNER,
      AudioDeviceType::IN_DOCK,
      AudioDeviceType::IN_BUS,
      AudioDeviceType::OUT_DEFAULT,
      AudioDeviceType::OUT_ACCESSORY,
      AudioDeviceType::OUT_AFE_PROXY,
      AudioDeviceType::OUT_CARKIT,
      AudioDeviceType::OUT_DEVICE,
      AudioDeviceType::OUT_ECHO_CANCELLER,
      AudioDeviceType::OUT_FM,
      AudioDeviceType::OUT_HEADPHONE,
      AudioDeviceType::OUT_HEADSET,
      AudioDeviceType::OUT_HEARING_AID,
      AudioDeviceType::OUT_LINE_AUX,
      AudioDeviceType::OUT_SPEAKER,
      AudioDeviceType::OUT_SPEAKER_EARPIECE,
      AudioDeviceType::OUT_SPEAKER_SAFE,
      AudioDeviceType::OUT_SUBMIX,
      AudioDeviceType::OUT_TELEPHONY_TX,
      AudioDeviceType::OUT_DOCK,
      AudioDeviceType::OUT_BROADCAST,
      AudioDeviceType::OUT_BUS,
    };
    out = pick(values);
  }

  virtual void genAudioDualMonoMode(AudioDualMonoMode &out) {
    const AudioDualMonoMode values[] = {
      AudioDualMonoMode::OFF,
      AudioDualMonoMode::LR,
      AudioDualMonoMode::LL,
      AudioDualMonoMode::RR,
    };
    out = pick(values);
  }

  virtual void genAudioEncapsulationMode(AudioEncapsulationMode &out) {
    const AudioEncapsulationMode values[] = {
      AudioEncapsulationMode::INVALID,
      AudioEncapsulationMode::NONE,
      AudioEncapsulationMode::ELEMENTARY_STREAM,
      AudioEncapsulationMode::HANDLE,
    };
    out = pick(values);
  }

  virtual void genAudioEncapsulationType(AudioEncapsulationType &out) {
    const AudioEncapsulationType values[] = {
      AudioEncapsulationType::NONE,
      AudioEncapsulationType::IEC61937,
      AudioEncapsulationType::PCM,
    };
    out = pick(values);
  }

  virtual void genAudioFormatDescription(AudioFormatDescription &out) {
    genAudioFormatType(out.type);
    genPcmType(out.pcm);
    gen(out.encoding);
  }

  virtual void genAudioFormatType(AudioFormatType &out) {
    const AudioFormatType values[] = {
      AudioFormatType::DEFAULT,
      AudioFormatType::NON_PCM,
      AudioFormatType::PCM,
      AudioFormatType::SYS_RESERVED_INVALID,
    };
    out = pick(values);
  }

  virtual void genAudioGain(AudioGain &out) {
    gen(out.mode);
    genAudioChannelLayout(out.channelMask);
    gen(out.minValue);
    gen(out.maxValue);
    gen(out.defaultValue);
    gen(out.stepValue);
    gen(out.minRampMs);
    gen(out.maxRampMs);
    gen(out.useForVolume);
  }

  virtual void genAudioGainConfig(AudioGainConfig &out) {
    gen(out.index);
    gen(out.mode);
    genAudioChannelLayout(out.channelMask);
    gen(out.values, [this](auto &v) { gen(v); });
    gen(out.rampDurationMs);
  }

  virtual void genAudioIoFlags(AudioIoFlags &out) {
    int tag = p.ConsumeIntegralInRange<int>(0, 1);
    switch (tag) {
      case 0: {
        int32_t val;
        gen(val);
        out.set<AudioIoFlags::Tag::input>(val);
        break;
      }
      case 1: {
        int32_t val;
        gen(val);
        out.set<AudioIoFlags::Tag::output>(val);
        break;
      }
    }
  }

  virtual void genAudioLatencyMode(AudioLatencyMode &out) {
    const AudioLatencyMode values[] = {
      AudioLatencyMode::FREE,
      AudioLatencyMode::LOW,
      AudioLatencyMode::DYNAMIC_SPATIAL_AUDIO_SOFTWARE,
      AudioLatencyMode::DYNAMIC_SPATIAL_AUDIO_HARDWARE,
    };
    out = pick(values);
  }

  virtual void genAudioMMapPolicyType(AudioMMapPolicyType &out) {
    const AudioMMapPolicyType values[] = {
      AudioMMapPolicyType::DEFAULT,
      AudioMMapPolicyType::EXCLUSIVE,
    };
    out = pick(values);
  }

  virtual void genAudioMode(AudioMode &out) {
    const AudioMode values[] = {
      AudioMode::SYS_RESERVED_INVALID,
      AudioMode::SYS_RESERVED_CURRENT,
      AudioMode::NORMAL,
      AudioMode::RINGTONE,
      AudioMode::IN_CALL,
      AudioMode::IN_COMMUNICATION,
      AudioMode::CALL_SCREEN,
      AudioMode::SYS_RESERVED_CALL_REDIRECT,
      AudioMode::SYS_RESERVED_COMMUNICATION_REDIRECT,
    };
    out = pick(values);
  }

  virtual void genAudioOffloadInfo(AudioOffloadInfo &out) {
    genAudioConfigBase(out.base);
    genAudioStreamType(out.streamType);
    gen(out.bitRatePerSecond);
    gen(out.durationUs);
    gen(out.hasVideo);
    gen(out.isStreaming);
    gen(out.bitWidth);
    gen(out.offloadBufferSize);
    genAudioUsage(out.usage);
    genAudioEncapsulationMode(out.encapsulationMode);
    gen(out.contentId);
    gen(out.syncId);
  }

  virtual void genAudioPlaybackRate(AudioPlaybackRate &out) {
    gen(out.speed);
    gen(out.pitch);
    genTimestretchMode(out.timestretchMode);
    genTimestretchFallbackMode(out.fallbackMode);
  }

  virtual void genTimestretchFallbackMode(AudioPlaybackRate::TimestretchFallbackMode &out) {
    const AudioPlaybackRate::TimestretchFallbackMode values[] = {
      AudioPlaybackRate::TimestretchFallbackMode::SYS_RESERVED_CUT_REPEAT,
      AudioPlaybackRate::TimestretchFallbackMode::SYS_RESERVED_DEFAULT,
      AudioPlaybackRate::TimestretchFallbackMode::MUTE,
      AudioPlaybackRate::TimestretchFallbackMode::FAIL,
    };
    out = pick(values);
  }

  virtual void genTimestretchMode(AudioPlaybackRate::TimestretchMode &out) {
    const AudioPlaybackRate::TimestretchMode values[] = {
      AudioPlaybackRate::TimestretchMode::DEFAULT,
      AudioPlaybackRate::TimestretchMode::VOICE,
    };
    out = pick(values);
  }

  virtual void genAudioPort(AudioPort &out) {
    gen(out.id);
    gen(out.name);
    gen(out.profiles, [this](auto &v) { genAudioProfile(v); });
    genAudioIoFlags(out.flags);
    gen(out.extraAudioDescriptors, [this](auto &v) { genExtraAudioDescriptor(v); });
    gen(out.gains, [this](auto &v) { genAudioGain(v); });
    genAudioPortExt(out.ext);
  }

  virtual void genAudioPortConfig(AudioPortConfig &out) {
    gen(out.id);
    gen(out.portId);
    gen(out.sampleRate, [this](auto &v) { genInt(v); });
    gen(out.channelMask, [this](auto &v) { genAudioChannelLayout(v); });
    gen(out.format, [this](auto &v) { genAudioFormatDescription(v); });
    gen(out.gain, [this](auto &v) { genAudioGainConfig(v); });
    gen(out.flags, [this](auto &v) { genAudioIoFlags(v); });
    genAudioPortExt(out.ext);
  }

  virtual void genAudioPortDeviceExt(AudioPortDeviceExt &out) {
    genAudioDevice(out.device);
    gen(out.flags);
    gen(out.encodedFormats, [this](auto &v) { genAudioFormatDescription(v); });
    gen(out.encapsulationModes);
    gen(out.encapsulationMetadataTypes);
  }

  virtual void genAudioPortExt(AudioPortExt &out) {
    int tag = p.ConsumeIntegralInRange<int>(0, 3);
    switch (tag) {
      case 0: {
        bool val;
        gen(val);
        out.set<AudioPortExt::Tag::unspecified>(val);
        break;
      }
      case 1: {
        aidl::android::media::audio::common::AudioPortDeviceExt val;
        genAudioPortDeviceExt(val);
        out.set<AudioPortExt::Tag::device>(val);
        break;
      }
      case 2: {
        aidl::android::media::audio::common::AudioPortMixExt val;
        genAudioPortMixExt(val);
        out.set<AudioPortExt::Tag::mix>(val);
        break;
      }
      case 3: {
        int32_t val;
        gen(val);
        out.set<AudioPortExt::Tag::session>(val);
        break;
      }
    }
  }

  virtual void genAudioPortMixExt(AudioPortMixExt &out) {
    gen(out.handle);
    genAudioPortMixExtUseCase(out.usecase);
    gen(out.maxOpenStreamCount);
    gen(out.maxActiveStreamCount);
    gen(out.recommendedMuteDurationMs);
  }

  virtual void genAudioPortMixExtUseCase(AudioPortMixExtUseCase &out) {
    int tag = p.ConsumeIntegralInRange<int>(0, 2);
    switch (tag) {
      case 0: {
        bool val;
        gen(val);
        out.set<AudioPortMixExtUseCase::Tag::unspecified>(val);
        break;
      }
      case 1: {
        aidl::android::media::audio::common::AudioStreamType val;
        genAudioStreamType(val);
        out.set<AudioPortMixExtUseCase::Tag::stream>(val);
        break;
      }
      case 2: {
        aidl::android::media::audio::common::AudioSource val;
        genAudioSource(val);
        out.set<AudioPortMixExtUseCase::Tag::source>(val);
        break;
      }
    }
  }

  virtual void genAudioProfile(AudioProfile &out) {
    gen(out.name);
    genAudioFormatDescription(out.format);
    gen(out.channelMasks, [this](auto &v) { genAudioChannelLayout(v); });
    gen(out.sampleRates, [this](auto &v) { gen(v); });
    genAudioEncapsulationType(out.encapsulationType);
  }

  virtual void genAudioSource(AudioSource &out) {
    const AudioSource values[] = {
      AudioSource::SYS_RESERVED_INVALID,
      AudioSource::DEFAULT,
      AudioSource::MIC,
      AudioSource::VOICE_UPLINK,
      AudioSource::VOICE_DOWNLINK,
      AudioSource::VOICE_CALL,
      AudioSource::CAMCORDER,
      AudioSource::VOICE_RECOGNITION,
      AudioSource::VOICE_COMMUNICATION,
      AudioSource::REMOTE_SUBMIX,
      AudioSource::UNPROCESSED,
      AudioSource::VOICE_PERFORMANCE,
      AudioSource::ECHO_REFERENCE,
      AudioSource::FM_TUNER,
      AudioSource::HOTWORD,
      AudioSource::ULTRASOUND,
    };
    out = pick(values);
  }

  virtual void genAudioStandard(AudioStandard &out) {
    const AudioStandard values[] = {
      AudioStandard::NONE,
      AudioStandard::EDID,
      AudioStandard::SADB,
      AudioStandard::VSADB,
    };
    out = pick(values);
  }

  virtual void genAudioStreamType(AudioStreamType &out) {
    const AudioStreamType values[] = {
      AudioStreamType::INVALID,
      AudioStreamType::SYS_RESERVED_DEFAULT,
      AudioStreamType::VOICE_CALL,
      AudioStreamType::SYSTEM,
      AudioStreamType::RING,
      AudioStreamType::MUSIC,
      AudioStreamType::ALARM,
      AudioStreamType::NOTIFICATION,
      AudioStreamType::BLUETOOTH_SCO,
      AudioStreamType::ENFORCED_AUDIBLE,
      AudioStreamType::DTMF,
      AudioStreamType::TTS,
      AudioStreamType::ACCESSIBILITY,
      AudioStreamType::ASSISTANT,
      AudioStreamType::SYS_RESERVED_REROUTING,
      AudioStreamType::SYS_RESERVED_PATCH,
      AudioStreamType::CALL_ASSISTANT,
    };
    out = pick(values);
  }

  virtual void genAudioUsage(AudioUsage &out) {
    const AudioUsage values[] = {
      AudioUsage::INVALID,
      AudioUsage::UNKNOWN,
      AudioUsage::MEDIA,
      AudioUsage::VOICE_COMMUNICATION,
      AudioUsage::VOICE_COMMUNICATION_SIGNALLING,
      AudioUsage::ALARM,
      AudioUsage::NOTIFICATION,
      AudioUsage::NOTIFICATION_TELEPHONY_RINGTONE,
      AudioUsage::SYS_RESERVED_NOTIFICATION_COMMUNICATION_REQUEST,
      AudioUsage::SYS_RESERVED_NOTIFICATION_COMMUNICATION_INSTANT,
      AudioUsage::SYS_RESERVED_NOTIFICATION_COMMUNICATION_DELAYED,
      AudioUsage::NOTIFICATION_EVENT,
      AudioUsage::ASSISTANCE_ACCESSIBILITY,
      AudioUsage::ASSISTANCE_NAVIGATION_GUIDANCE,
      AudioUsage::ASSISTANCE_SONIFICATION,
      AudioUsage::GAME,
      AudioUsage::VIRTUAL_SOURCE,
      AudioUsage::ASSISTANT,
      AudioUsage::CALL_ASSISTANT,
      AudioUsage::EMERGENCY,
      AudioUsage::SAFETY,
      AudioUsage::VEHICLE_STATUS,
      AudioUsage::ANNOUNCEMENT,
    };
    out = pick(values);
  }

  virtual void genExtraAudioDescriptor(ExtraAudioDescriptor &out) {
    genAudioStandard(out.standard);
    gen(out.audioDescriptor, [this](auto &v) { gen(v); });
    genAudioEncapsulationType(out.encapsulationType);
  }

  virtual void genPcmType(PcmType &out) {
    const PcmType values[] = {
      PcmType::DEFAULT,
      PcmType::UINT_8_BIT,
      PcmType::INT_16_BIT,
      PcmType::INT_32_BIT,
      PcmType::FIXED_Q_8_24,
      PcmType::FLOAT_32_BIT,
      PcmType::INT_24_BIT,
    };
    out = pick(values);
  }

};
template <typename P, typename T>
struct IBluetoothFuzzer: public FuzzerBase<P, T> {
  IBluetoothFuzzer() {}
  IBluetoothFuzzer(P *provider, T *target): FuzzerBase<P, T>(provider, target) {}
  virtual ~IBluetoothFuzzer() {}

  virtual std::optional<::aidl::android::hardware::audio::core::IBluetooth::ScoConfig> fuzz_setScoConfig() {
    ::aidl::android::hardware::audio::core::IBluetooth::ScoConfig in_config;
    this->provider->genScoConfig(in_config);
    ::aidl::android::hardware::audio::core::IBluetooth::ScoConfig _aidl_return;

    if (this->target->setScoConfig(in_config, &_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<::aidl::android::hardware::audio::core::IBluetooth::HfpConfig> fuzz_setHfpConfig() {
    ::aidl::android::hardware::audio::core::IBluetooth::HfpConfig in_config;
    this->provider->genHfpConfig(in_config);
    ::aidl::android::hardware::audio::core::IBluetooth::HfpConfig _aidl_return;

    if (this->target->setHfpConfig(in_config, &_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz() {
    char id = this->provider->inner().template ConsumeIntegral<char>();
    switch (id) {
      case 0: {
        fuzz_setScoConfig();
        break;
      }
      case 1: {
        fuzz_setHfpConfig();
        break;
      }
    }
  }
};

template <typename P, typename T>
struct IBluetoothA2dpFuzzer: public FuzzerBase<P, T> {
  IBluetoothA2dpFuzzer() {}
  IBluetoothA2dpFuzzer(P *provider, T *target): FuzzerBase<P, T>(provider, target) {}
  virtual ~IBluetoothA2dpFuzzer() {}

  virtual std::optional<bool> fuzz_isEnabled() {
    bool _aidl_return;

    if (this->target->isEnabled(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setEnabled() {
    bool in_enabled;
    this->provider->gen(in_enabled);

    this->target->setEnabled(in_enabled);
  }

  virtual std::optional<bool> fuzz_supportsOffloadReconfiguration() {
    bool _aidl_return;

    if (this->target->supportsOffloadReconfiguration(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_reconfigureOffload() {
    std::vector<::aidl::android::hardware::audio::core::VendorParameter> in_parameters;
    this->provider->gen(in_parameters, [this](auto &v) { this->provider->genVendorParameter(v); });

    this->target->reconfigureOffload(in_parameters);
  }

  virtual void fuzz() {
    char id = this->provider->inner().template ConsumeIntegral<char>();
    switch (id) {
      case 0: {
        fuzz_isEnabled();
        break;
      }
      case 1: {
        fuzz_setEnabled();
        break;
      }
      case 2: {
        fuzz_supportsOffloadReconfiguration();
        break;
      }
      case 3: {
        fuzz_reconfigureOffload();
        break;
      }
    }
  }
};

template <typename P, typename T>
struct IBluetoothLeFuzzer: public FuzzerBase<P, T> {
  IBluetoothLeFuzzer() {}
  IBluetoothLeFuzzer(P *provider, T *target): FuzzerBase<P, T>(provider, target) {}
  virtual ~IBluetoothLeFuzzer() {}

  virtual std::optional<bool> fuzz_isEnabled() {
    bool _aidl_return;

    if (this->target->isEnabled(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setEnabled() {
    bool in_enabled;
    this->provider->gen(in_enabled);

    this->target->setEnabled(in_enabled);
  }

  virtual std::optional<bool> fuzz_supportsOffloadReconfiguration() {
    bool _aidl_return;

    if (this->target->supportsOffloadReconfiguration(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_reconfigureOffload() {
    std::vector<::aidl::android::hardware::audio::core::VendorParameter> in_parameters;
    this->provider->gen(in_parameters, [this](auto &v) { this->provider->genVendorParameter(v); });

    this->target->reconfigureOffload(in_parameters);
  }

  virtual void fuzz() {
    char id = this->provider->inner().template ConsumeIntegral<char>();
    switch (id) {
      case 0: {
        fuzz_isEnabled();
        break;
      }
      case 1: {
        fuzz_setEnabled();
        break;
      }
      case 2: {
        fuzz_supportsOffloadReconfiguration();
        break;
      }
      case 3: {
        fuzz_reconfigureOffload();
        break;
      }
    }
  }
};

template <typename P, typename T>
struct IConfigFuzzer: public FuzzerBase<P, T> {
  IConfigFuzzer() {}
  IConfigFuzzer(P *provider, T *target): FuzzerBase<P, T>(provider, target) {}
  virtual ~IConfigFuzzer() {}

  virtual std::optional<::aidl::android::hardware::audio::core::SurroundSoundConfig> fuzz_getSurroundSoundConfig() {
    ::aidl::android::hardware::audio::core::SurroundSoundConfig _aidl_return;

    if (this->target->getSurroundSoundConfig(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<::aidl::android::media::audio::common::AudioHalEngineConfig> fuzz_getEngineConfig() {
    ::aidl::android::media::audio::common::AudioHalEngineConfig _aidl_return;

    if (this->target->getEngineConfig(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz() {
    char id = this->provider->inner().template ConsumeIntegral<char>();
    switch (id) {
      case 0: {
        fuzz_getSurroundSoundConfig();
        break;
      }
      case 1: {
        fuzz_getEngineConfig();
        break;
      }
    }
  }
};

template <typename P, typename T>
struct IModuleFuzzer: public FuzzerBase<P, T> {
  IModuleFuzzer() {}
  IModuleFuzzer(P *provider, T *target): FuzzerBase<P, T>(provider, target) {}
  virtual ~IModuleFuzzer() {}

  virtual void fuzz_setModuleDebug() {
    ::aidl::android::hardware::audio::core::ModuleDebug in_debug;
    this->provider->genModuleDebug(in_debug);

    this->target->setModuleDebug(in_debug);
  }

  virtual std::optional<std::shared_ptr<::aidl::android::hardware::audio::core::ITelephony>> fuzz_getTelephony() {
    std::shared_ptr<::aidl::android::hardware::audio::core::ITelephony> _aidl_return;

    if (this->target->getTelephony(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<std::shared_ptr<::aidl::android::hardware::audio::core::IBluetooth>> fuzz_getBluetooth() {
    std::shared_ptr<::aidl::android::hardware::audio::core::IBluetooth> _aidl_return;

    if (this->target->getBluetooth(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<std::shared_ptr<::aidl::android::hardware::audio::core::IBluetoothA2dp>> fuzz_getBluetoothA2dp() {
    std::shared_ptr<::aidl::android::hardware::audio::core::IBluetoothA2dp> _aidl_return;

    if (this->target->getBluetoothA2dp(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<std::shared_ptr<::aidl::android::hardware::audio::core::IBluetoothLe>> fuzz_getBluetoothLe() {
    std::shared_ptr<::aidl::android::hardware::audio::core::IBluetoothLe> _aidl_return;

    if (this->target->getBluetoothLe(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<::aidl::android::media::audio::common::AudioPort> fuzz_connectExternalDevice() {
    ::aidl::android::media::audio::common::AudioPort in_templateIdAndAdditionalData;
    this->provider->genAudioPort(in_templateIdAndAdditionalData);
    ::aidl::android::media::audio::common::AudioPort _aidl_return;

    if (this->target->connectExternalDevice(in_templateIdAndAdditionalData, &_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_disconnectExternalDevice() {
    int32_t in_portId;
    this->provider->gen(in_portId);

    this->target->disconnectExternalDevice(in_portId);
  }

  virtual std::optional<std::vector<::aidl::android::hardware::audio::core::AudioPatch>> fuzz_getAudioPatches() {
    std::vector<::aidl::android::hardware::audio::core::AudioPatch> _aidl_return;

    if (this->target->getAudioPatches(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<::aidl::android::media::audio::common::AudioPort> fuzz_getAudioPort() {
    int32_t in_portId;
    this->provider->gen(in_portId);
    ::aidl::android::media::audio::common::AudioPort _aidl_return;

    if (this->target->getAudioPort(in_portId, &_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<std::vector<::aidl::android::media::audio::common::AudioPortConfig>> fuzz_getAudioPortConfigs() {
    std::vector<::aidl::android::media::audio::common::AudioPortConfig> _aidl_return;

    if (this->target->getAudioPortConfigs(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<std::vector<::aidl::android::media::audio::common::AudioPort>> fuzz_getAudioPorts() {
    std::vector<::aidl::android::media::audio::common::AudioPort> _aidl_return;

    if (this->target->getAudioPorts(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<std::vector<::aidl::android::hardware::audio::core::AudioRoute>> fuzz_getAudioRoutes() {
    std::vector<::aidl::android::hardware::audio::core::AudioRoute> _aidl_return;

    if (this->target->getAudioRoutes(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<std::vector<::aidl::android::hardware::audio::core::AudioRoute>> fuzz_getAudioRoutesForAudioPort() {
    int32_t in_portId;
    this->provider->gen(in_portId);
    std::vector<::aidl::android::hardware::audio::core::AudioRoute> _aidl_return;

    if (this->target->getAudioRoutesForAudioPort(in_portId, &_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<::aidl::android::hardware::audio::core::IModule::OpenInputStreamReturn> fuzz_openInputStream() {
    ::aidl::android::hardware::audio::core::IModule::OpenInputStreamArguments in_args;
    this->provider->genOpenInputStreamArguments(in_args);
    ::aidl::android::hardware::audio::core::IModule::OpenInputStreamReturn _aidl_return;

    if (this->target->openInputStream(in_args, &_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<::aidl::android::hardware::audio::core::IModule::OpenOutputStreamReturn> fuzz_openOutputStream() {
    ::aidl::android::hardware::audio::core::IModule::OpenOutputStreamArguments in_args;
    this->provider->genOpenOutputStreamArguments(in_args);
    ::aidl::android::hardware::audio::core::IModule::OpenOutputStreamReturn _aidl_return;

    if (this->target->openOutputStream(in_args, &_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<::aidl::android::hardware::audio::core::IModule::SupportedPlaybackRateFactors> fuzz_getSupportedPlaybackRateFactors() {
    ::aidl::android::hardware::audio::core::IModule::SupportedPlaybackRateFactors _aidl_return;

    if (this->target->getSupportedPlaybackRateFactors(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<::aidl::android::hardware::audio::core::AudioPatch> fuzz_setAudioPatch() {
    ::aidl::android::hardware::audio::core::AudioPatch in_requested;
    this->provider->genAudioPatch(in_requested);
    ::aidl::android::hardware::audio::core::AudioPatch _aidl_return;

    if (this->target->setAudioPatch(in_requested, &_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<bool> fuzz_setAudioPortConfig() {
    ::aidl::android::media::audio::common::AudioPortConfig in_requested;
    this->provider->genAudioPortConfig(in_requested);
    ::aidl::android::media::audio::common::AudioPortConfig out_suggested;
    bool _aidl_return;

    if (this->target->setAudioPortConfig(in_requested, &out_suggested, &_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_resetAudioPatch() {
    int32_t in_patchId;
    this->provider->gen(in_patchId);

    this->target->resetAudioPatch(in_patchId);
  }

  virtual void fuzz_resetAudioPortConfig() {
    int32_t in_portConfigId;
    this->provider->gen(in_portConfigId);

    this->target->resetAudioPortConfig(in_portConfigId);
  }

  virtual std::optional<bool> fuzz_getMasterMute() {
    bool _aidl_return;

    if (this->target->getMasterMute(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setMasterMute() {
    bool in_mute;
    this->provider->gen(in_mute);

    this->target->setMasterMute(in_mute);
  }

  virtual std::optional<float> fuzz_getMasterVolume() {
    float _aidl_return;

    if (this->target->getMasterVolume(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setMasterVolume() {
    float in_volume;
    this->provider->gen(in_volume);

    this->target->setMasterVolume(in_volume);
  }

  virtual std::optional<bool> fuzz_getMicMute() {
    bool _aidl_return;

    if (this->target->getMicMute(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setMicMute() {
    bool in_mute;
    this->provider->gen(in_mute);

    this->target->setMicMute(in_mute);
  }

  virtual std::optional<std::vector<::aidl::android::media::audio::common::MicrophoneInfo>> fuzz_getMicrophones() {
    std::vector<::aidl::android::media::audio::common::MicrophoneInfo> _aidl_return;

    if (this->target->getMicrophones(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_updateAudioMode() {
    ::aidl::android::media::audio::common::AudioMode in_mode;
    this->provider->genAudioMode(in_mode);

    this->target->updateAudioMode(in_mode);
  }

  virtual void fuzz_updateScreenRotation() {
    ::aidl::android::hardware::audio::core::IModule::ScreenRotation in_rotation;
    this->provider->genScreenRotation(in_rotation);

    this->target->updateScreenRotation(in_rotation);
  }

  virtual void fuzz_updateScreenState() {
    bool in_isTurnedOn;
    this->provider->gen(in_isTurnedOn);

    this->target->updateScreenState(in_isTurnedOn);
  }

  virtual std::optional<std::shared_ptr<::aidl::android::hardware::audio::core::sounddose::ISoundDose>> fuzz_getSoundDose() {
    std::shared_ptr<::aidl::android::hardware::audio::core::sounddose::ISoundDose> _aidl_return;

    if (this->target->getSoundDose(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<int32_t> fuzz_generateHwAvSyncId() {
    int32_t _aidl_return;

    if (this->target->generateHwAvSyncId(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<std::vector<::aidl::android::hardware::audio::core::VendorParameter>> fuzz_getVendorParameters() {
    std::vector<std::string> in_ids;
    this->provider->gen(in_ids, [this](auto &v) { this->provider->gen(v); });
    std::vector<::aidl::android::hardware::audio::core::VendorParameter> _aidl_return;

    if (this->target->getVendorParameters(in_ids, &_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setVendorParameters() {
    std::vector<::aidl::android::hardware::audio::core::VendorParameter> in_parameters;
    this->provider->gen(in_parameters, [this](auto &v) { this->provider->genVendorParameter(v); });
    bool in_async;
    this->provider->gen(in_async);

    this->target->setVendorParameters(in_parameters, in_async);
  }

  virtual void fuzz_addDeviceEffect() {
    int32_t in_portConfigId;
    this->provider->gen(in_portConfigId);
    std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect> in_effect;
    // NOTE: `in_effect' is an interface

    this->target->addDeviceEffect(in_portConfigId, in_effect);
  }

  virtual void fuzz_removeDeviceEffect() {
    int32_t in_portConfigId;
    this->provider->gen(in_portConfigId);
    std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect> in_effect;
    // NOTE: `in_effect' is an interface

    this->target->removeDeviceEffect(in_portConfigId, in_effect);
  }

  virtual std::optional<std::vector<::aidl::android::media::audio::common::AudioMMapPolicyInfo>> fuzz_getMmapPolicyInfos() {
    ::aidl::android::media::audio::common::AudioMMapPolicyType in_mmapPolicyType;
    this->provider->genAudioMMapPolicyType(in_mmapPolicyType);
    std::vector<::aidl::android::media::audio::common::AudioMMapPolicyInfo> _aidl_return;

    if (this->target->getMmapPolicyInfos(in_mmapPolicyType, &_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<bool> fuzz_supportsVariableLatency() {
    bool _aidl_return;

    if (this->target->supportsVariableLatency(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<int32_t> fuzz_getAAudioMixerBurstCount() {
    int32_t _aidl_return;

    if (this->target->getAAudioMixerBurstCount(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<int32_t> fuzz_getAAudioHardwareBurstMinUsec() {
    int32_t _aidl_return;

    if (this->target->getAAudioHardwareBurstMinUsec(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_prepareToDisconnectExternalDevice() {
    int32_t in_portId;
    this->provider->gen(in_portId);

    this->target->prepareToDisconnectExternalDevice(in_portId);
  }

  virtual void fuzz() {
    char id = this->provider->inner().template ConsumeIntegral<char>();
    switch (id) {
      case 0: {
        fuzz_setModuleDebug();
        break;
      }
      case 1: {
        fuzz_getTelephony();
        break;
      }
      case 2: {
        fuzz_getBluetooth();
        break;
      }
      case 3: {
        fuzz_getBluetoothA2dp();
        break;
      }
      case 4: {
        fuzz_getBluetoothLe();
        break;
      }
      case 5: {
        fuzz_connectExternalDevice();
        break;
      }
      case 6: {
        fuzz_disconnectExternalDevice();
        break;
      }
      case 7: {
        fuzz_getAudioPatches();
        break;
      }
      case 8: {
        fuzz_getAudioPort();
        break;
      }
      case 9: {
        fuzz_getAudioPortConfigs();
        break;
      }
      case 10: {
        fuzz_getAudioPorts();
        break;
      }
      case 11: {
        fuzz_getAudioRoutes();
        break;
      }
      case 12: {
        fuzz_getAudioRoutesForAudioPort();
        break;
      }
      case 13: {
        fuzz_openInputStream();
        break;
      }
      case 14: {
        fuzz_openOutputStream();
        break;
      }
      case 15: {
        fuzz_getSupportedPlaybackRateFactors();
        break;
      }
      case 16: {
        fuzz_setAudioPatch();
        break;
      }
      case 17: {
        fuzz_setAudioPortConfig();
        break;
      }
      case 18: {
        fuzz_resetAudioPatch();
        break;
      }
      case 19: {
        fuzz_resetAudioPortConfig();
        break;
      }
      case 20: {
        fuzz_getMasterMute();
        break;
      }
      case 21: {
        fuzz_setMasterMute();
        break;
      }
      case 22: {
        fuzz_getMasterVolume();
        break;
      }
      case 23: {
        fuzz_setMasterVolume();
        break;
      }
      case 24: {
        fuzz_getMicMute();
        break;
      }
      case 25: {
        fuzz_setMicMute();
        break;
      }
      case 26: {
        fuzz_getMicrophones();
        break;
      }
      case 27: {
        fuzz_updateAudioMode();
        break;
      }
      case 28: {
        fuzz_updateScreenRotation();
        break;
      }
      case 29: {
        fuzz_updateScreenState();
        break;
      }
      case 30: {
        fuzz_getSoundDose();
        break;
      }
      case 31: {
        fuzz_generateHwAvSyncId();
        break;
      }
      case 32: {
        fuzz_getVendorParameters();
        break;
      }
      case 33: {
        fuzz_setVendorParameters();
        break;
      }
      case 34: {
        fuzz_addDeviceEffect();
        break;
      }
      case 35: {
        fuzz_removeDeviceEffect();
        break;
      }
      case 36: {
        fuzz_getMmapPolicyInfos();
        break;
      }
      case 37: {
        fuzz_supportsVariableLatency();
        break;
      }
      case 38: {
        fuzz_getAAudioMixerBurstCount();
        break;
      }
      case 39: {
        fuzz_getAAudioHardwareBurstMinUsec();
        break;
      }
      case 40: {
        fuzz_prepareToDisconnectExternalDevice();
        break;
      }
    }
  }
};

template <typename P, typename T>
struct IStreamCallbackFuzzer: public FuzzerBase<P, T> {
  IStreamCallbackFuzzer() {}
  IStreamCallbackFuzzer(P *provider, T *target): FuzzerBase<P, T>(provider, target) {}
  virtual ~IStreamCallbackFuzzer() {}

  virtual void fuzz_onTransferReady() {

    this->target->onTransferReady();
  }

  virtual void fuzz_onError() {

    this->target->onError();
  }

  virtual void fuzz_onDrainReady() {

    this->target->onDrainReady();
  }

  virtual void fuzz() {
    char id = this->provider->inner().template ConsumeIntegral<char>();
    switch (id) {
      case 0: {
        fuzz_onTransferReady();
        break;
      }
      case 1: {
        fuzz_onError();
        break;
      }
      case 2: {
        fuzz_onDrainReady();
        break;
      }
    }
  }
};

template <typename P, typename T>
struct IStreamCommonFuzzer: public FuzzerBase<P, T> {
  IStreamCommonFuzzer() {}
  IStreamCommonFuzzer(P *provider, T *target): FuzzerBase<P, T>(provider, target) {}
  virtual ~IStreamCommonFuzzer() {}

  virtual void fuzz_close() {

    this->target->close();
  }

  virtual void fuzz_prepareToClose() {

    this->target->prepareToClose();
  }

  virtual void fuzz_updateHwAvSyncId() {
    int32_t in_hwAvSyncId;
    this->provider->gen(in_hwAvSyncId);

    this->target->updateHwAvSyncId(in_hwAvSyncId);
  }

  virtual std::optional<std::vector<::aidl::android::hardware::audio::core::VendorParameter>> fuzz_getVendorParameters() {
    std::vector<std::string> in_ids;
    this->provider->gen(in_ids, [this](auto &v) { this->provider->gen(v); });
    std::vector<::aidl::android::hardware::audio::core::VendorParameter> _aidl_return;

    if (this->target->getVendorParameters(in_ids, &_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setVendorParameters() {
    std::vector<::aidl::android::hardware::audio::core::VendorParameter> in_parameters;
    this->provider->gen(in_parameters, [this](auto &v) { this->provider->genVendorParameter(v); });
    bool in_async;
    this->provider->gen(in_async);

    this->target->setVendorParameters(in_parameters, in_async);
  }

  virtual void fuzz_addEffect() {
    std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect> in_effect;
    // NOTE: `in_effect' is an interface

    this->target->addEffect(in_effect);
  }

  virtual void fuzz_removeEffect() {
    std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect> in_effect;
    // NOTE: `in_effect' is an interface

    this->target->removeEffect(in_effect);
  }

  virtual void fuzz() {
    char id = this->provider->inner().template ConsumeIntegral<char>();
    switch (id) {
      case 0: {
        fuzz_close();
        break;
      }
      case 1: {
        fuzz_prepareToClose();
        break;
      }
      case 2: {
        fuzz_updateHwAvSyncId();
        break;
      }
      case 3: {
        fuzz_getVendorParameters();
        break;
      }
      case 4: {
        fuzz_setVendorParameters();
        break;
      }
      case 5: {
        fuzz_addEffect();
        break;
      }
      case 6: {
        fuzz_removeEffect();
        break;
      }
    }
  }
};

template <typename P, typename T>
struct IStreamInFuzzer: public FuzzerBase<P, T> {
  IStreamInFuzzer() {}
  IStreamInFuzzer(P *provider, T *target): FuzzerBase<P, T>(provider, target) {}
  virtual ~IStreamInFuzzer() {}

  virtual std::optional<std::shared_ptr<::aidl::android::hardware::audio::core::IStreamCommon>> fuzz_getStreamCommon() {
    std::shared_ptr<::aidl::android::hardware::audio::core::IStreamCommon> _aidl_return;

    if (this->target->getStreamCommon(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>> fuzz_getActiveMicrophones() {
    std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo> _aidl_return;

    if (this->target->getActiveMicrophones(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual std::optional<::aidl::android::hardware::audio::core::IStreamIn::MicrophoneDirection> fuzz_getMicrophoneDirection() {
    ::aidl::android::hardware::audio::core::IStreamIn::MicrophoneDirection _aidl_return;

    if (this->target->getMicrophoneDirection(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setMicrophoneDirection() {
    ::aidl::android::hardware::audio::core::IStreamIn::MicrophoneDirection in_direction;
    this->provider->genMicrophoneDirection(in_direction);

    this->target->setMicrophoneDirection(in_direction);
  }

  virtual std::optional<float> fuzz_getMicrophoneFieldDimension() {
    float _aidl_return;

    if (this->target->getMicrophoneFieldDimension(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setMicrophoneFieldDimension() {
    float in_zoom;
    this->provider->gen(in_zoom);

    this->target->setMicrophoneFieldDimension(in_zoom);
  }

  virtual void fuzz_updateMetadata() {
    ::aidl::android::hardware::audio::common::SinkMetadata in_sinkMetadata;
    this->provider->genSinkMetadata(in_sinkMetadata);

    this->target->updateMetadata(in_sinkMetadata);
  }

  virtual std::optional<std::vector<float>> fuzz_getHwGain() {
    std::vector<float> _aidl_return;

    if (this->target->getHwGain(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setHwGain() {
    std::vector<float> in_channelGains;
    this->provider->gen(in_channelGains, [this](auto &v) { this->provider->gen(v); });

    this->target->setHwGain(in_channelGains);
  }

  virtual void fuzz() {
    char id = this->provider->inner().template ConsumeIntegral<char>();
    switch (id) {
      case 0: {
        fuzz_getStreamCommon();
        break;
      }
      case 1: {
        fuzz_getActiveMicrophones();
        break;
      }
      case 2: {
        fuzz_getMicrophoneDirection();
        break;
      }
      case 3: {
        fuzz_setMicrophoneDirection();
        break;
      }
      case 4: {
        fuzz_getMicrophoneFieldDimension();
        break;
      }
      case 5: {
        fuzz_setMicrophoneFieldDimension();
        break;
      }
      case 6: {
        fuzz_updateMetadata();
        break;
      }
      case 7: {
        fuzz_getHwGain();
        break;
      }
      case 8: {
        fuzz_setHwGain();
        break;
      }
    }
  }
};

template <typename P, typename T>
struct IStreamOutFuzzer: public FuzzerBase<P, T> {
  IStreamOutFuzzer() {}
  IStreamOutFuzzer(P *provider, T *target): FuzzerBase<P, T>(provider, target) {}
  virtual ~IStreamOutFuzzer() {}

  virtual std::optional<std::shared_ptr<::aidl::android::hardware::audio::core::IStreamCommon>> fuzz_getStreamCommon() {
    std::shared_ptr<::aidl::android::hardware::audio::core::IStreamCommon> _aidl_return;

    if (this->target->getStreamCommon(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_updateMetadata() {
    ::aidl::android::hardware::audio::common::SourceMetadata in_sourceMetadata;
    this->provider->genSourceMetadata(in_sourceMetadata);

    this->target->updateMetadata(in_sourceMetadata);
  }

  virtual void fuzz_updateOffloadMetadata() {
    ::aidl::android::hardware::audio::common::AudioOffloadMetadata in_offloadMetadata;
    this->provider->genAudioOffloadMetadata(in_offloadMetadata);

    this->target->updateOffloadMetadata(in_offloadMetadata);
  }

  virtual std::optional<std::vector<float>> fuzz_getHwVolume() {
    std::vector<float> _aidl_return;

    if (this->target->getHwVolume(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setHwVolume() {
    std::vector<float> in_channelVolumes;
    this->provider->gen(in_channelVolumes, [this](auto &v) { this->provider->gen(v); });

    this->target->setHwVolume(in_channelVolumes);
  }

  virtual std::optional<float> fuzz_getAudioDescriptionMixLevel() {
    float _aidl_return;

    if (this->target->getAudioDescriptionMixLevel(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setAudioDescriptionMixLevel() {
    float in_leveldB;
    this->provider->gen(in_leveldB);

    this->target->setAudioDescriptionMixLevel(in_leveldB);
  }

  virtual std::optional<::aidl::android::media::audio::common::AudioDualMonoMode> fuzz_getDualMonoMode() {
    ::aidl::android::media::audio::common::AudioDualMonoMode _aidl_return;

    if (this->target->getDualMonoMode(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setDualMonoMode() {
    ::aidl::android::media::audio::common::AudioDualMonoMode in_mode;
    this->provider->genAudioDualMonoMode(in_mode);

    this->target->setDualMonoMode(in_mode);
  }

  virtual std::optional<std::vector<::aidl::android::media::audio::common::AudioLatencyMode>> fuzz_getRecommendedLatencyModes() {
    std::vector<::aidl::android::media::audio::common::AudioLatencyMode> _aidl_return;

    if (this->target->getRecommendedLatencyModes(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setLatencyMode() {
    ::aidl::android::media::audio::common::AudioLatencyMode in_mode;
    this->provider->genAudioLatencyMode(in_mode);

    this->target->setLatencyMode(in_mode);
  }

  virtual std::optional<::aidl::android::media::audio::common::AudioPlaybackRate> fuzz_getPlaybackRateParameters() {
    ::aidl::android::media::audio::common::AudioPlaybackRate _aidl_return;

    if (this->target->getPlaybackRateParameters(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_setPlaybackRateParameters() {
    ::aidl::android::media::audio::common::AudioPlaybackRate in_playbackRate;
    this->provider->genAudioPlaybackRate(in_playbackRate);

    this->target->setPlaybackRateParameters(in_playbackRate);
  }

  virtual void fuzz_selectPresentation() {
    int32_t in_presentationId;
    this->provider->gen(in_presentationId);
    int32_t in_programId;
    this->provider->gen(in_programId);

    this->target->selectPresentation(in_presentationId, in_programId);
  }

  virtual void fuzz() {
    char id = this->provider->inner().template ConsumeIntegral<char>();
    switch (id) {
      case 0: {
        fuzz_getStreamCommon();
        break;
      }
      case 1: {
        fuzz_updateMetadata();
        break;
      }
      case 2: {
        fuzz_updateOffloadMetadata();
        break;
      }
      case 3: {
        fuzz_getHwVolume();
        break;
      }
      case 4: {
        fuzz_setHwVolume();
        break;
      }
      case 5: {
        fuzz_getAudioDescriptionMixLevel();
        break;
      }
      case 6: {
        fuzz_setAudioDescriptionMixLevel();
        break;
      }
      case 7: {
        fuzz_getDualMonoMode();
        break;
      }
      case 8: {
        fuzz_setDualMonoMode();
        break;
      }
      case 9: {
        fuzz_getRecommendedLatencyModes();
        break;
      }
      case 10: {
        fuzz_setLatencyMode();
        break;
      }
      case 11: {
        fuzz_getPlaybackRateParameters();
        break;
      }
      case 12: {
        fuzz_setPlaybackRateParameters();
        break;
      }
      case 13: {
        fuzz_selectPresentation();
        break;
      }
    }
  }
};

template <typename P, typename T>
struct IStreamOutEventCallbackFuzzer: public FuzzerBase<P, T> {
  IStreamOutEventCallbackFuzzer() {}
  IStreamOutEventCallbackFuzzer(P *provider, T *target): FuzzerBase<P, T>(provider, target) {}
  virtual ~IStreamOutEventCallbackFuzzer() {}

  virtual void fuzz_onCodecFormatChanged() {
    std::vector<uint8_t> in_audioMetadata;
    this->provider->gen(in_audioMetadata, [this](auto &v) { this->provider->gen(v); });

    this->target->onCodecFormatChanged(in_audioMetadata);
  }

  virtual void fuzz_onRecommendedLatencyModeChanged() {
    std::vector<::aidl::android::media::audio::common::AudioLatencyMode> in_modes;
    this->provider->gen(in_modes, [this](auto &v) { this->provider->genAudioLatencyMode(v); });

    this->target->onRecommendedLatencyModeChanged(in_modes);
  }

  virtual void fuzz() {
    char id = this->provider->inner().template ConsumeIntegral<char>();
    switch (id) {
      case 0: {
        fuzz_onCodecFormatChanged();
        break;
      }
      case 1: {
        fuzz_onRecommendedLatencyModeChanged();
        break;
      }
    }
  }
};

template <typename P, typename T>
struct ITelephonyFuzzer: public FuzzerBase<P, T> {
  ITelephonyFuzzer() {}
  ITelephonyFuzzer(P *provider, T *target): FuzzerBase<P, T>(provider, target) {}
  virtual ~ITelephonyFuzzer() {}

  virtual std::optional<std::vector<::aidl::android::media::audio::common::AudioMode>> fuzz_getSupportedAudioModes() {
    std::vector<::aidl::android::media::audio::common::AudioMode> _aidl_return;

    if (this->target->getSupportedAudioModes(&_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz_switchAudioMode() {
    ::aidl::android::media::audio::common::AudioMode in_mode;
    this->provider->genAudioMode(in_mode);

    this->target->switchAudioMode(in_mode);
  }

  virtual std::optional<::aidl::android::hardware::audio::core::ITelephony::TelecomConfig> fuzz_setTelecomConfig() {
    ::aidl::android::hardware::audio::core::ITelephony::TelecomConfig in_config;
    this->provider->genTelecomConfig(in_config);
    ::aidl::android::hardware::audio::core::ITelephony::TelecomConfig _aidl_return;

    if (this->target->setTelecomConfig(in_config, &_aidl_return).isOk()) {
      return std::move(_aidl_return);
    } else {
      return std::nullopt;
    }
  }

  virtual void fuzz() {
    char id = this->provider->inner().template ConsumeIntegral<char>();
    switch (id) {
      case 0: {
        fuzz_getSupportedAudioModes();
        break;
      }
      case 1: {
        fuzz_switchAudioMode();
        break;
      }
      case 2: {
        fuzz_setTelecomConfig();
        break;
      }
    }
  }
};

