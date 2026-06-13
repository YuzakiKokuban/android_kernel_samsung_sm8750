// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <memory>
#include <string>
#include <vector>
#include <android-base/logging.h>
#include <fuzzer/FuzzedDataProvider.h>

#include "base.h"
#include "provider.h"
#include "gen.h"

#include <qti-audio-core/Module.h>
#include <qti-audio-core/ModulePrimary.h>
#include <qti-audio-core/Parameters.h>

using qti::audio::core::Module;
using qti::audio::core::ModulePrimary;

// data that persists between LLVMFuzzerTestOneInput() calls
struct AudioPersistData {
    std::vector<int> portIds;
    std::vector<int> portConfigIds;
    std::vector<std::shared_ptr<IStreamIn>> inStreams;
    std::vector<std::shared_ptr<IStreamOut>> outStreams;
};

class AudioDataProvider : public DataProvider {
public:
    AudioDataProvider(const char *data, size_t size, AudioPersistData *persistData):
            DataProvider(data, size), persistData(persistData) {}
    AudioPersistData *getPersistData() { return persistData; }

    virtual void genAudioPort(AudioPort &out) override {
        DataProvider::genAudioPort(out);
        pickPortId(out.id);
    }

    virtual void genAudioPortConfig(AudioPortConfig &out) override {
        DataProvider::genAudioPortConfig(out);
        pickPortConfigId(out.id);
        pickPortId(out.portId);
    }

    virtual void genOpenInputStreamArguments(IModule::OpenInputStreamArguments &out) override {
        DataProvider::genOpenInputStreamArguments(out);
        pickPortConfigId(out.portConfigId);
    }

    virtual void genOpenOutputStreamArguments(IModule::OpenOutputStreamArguments &out) override {
        DataProvider::genOpenOutputStreamArguments(out);
        pickPortConfigId(out.portConfigId);
    }

    virtual void genVendorParameter(VendorParameter &out) override {
        out.id = pickVendorParameter();
    }

    std::string pickVendorParameter() {
        using namespace qti::audio::core::Parameters;
        std::vector<std::string> parameters = {
            kHdrRecord,
            kWnr,
            kAns,
            kOrientation,
            kInverted,
            kHdrChannelCount,
            kHdrSamplingRate,
            kFacing,
            kVoiceCallState,
            kVoiceCallType,
            kVoiceVSID,
            kVoiceCRSCall,
            kVoiceCRSVolume,
            kVolumeBoost,
            kVoiceSlowTalk,
            kVoiceHDVoice,
            kVoiceDeviceMute,
            kVoiceDirection,
            kInCallMusic,
            kUHQA,
            kFbspCfgWaitTime,
            kFbspFTMWaitTime,
            kFbspValiWaitTime,
            kFbspValiValiTime,
            kTriggerSpeakerCall,
            kWfdChannelMap,
            kHapticsVolume,
            kHapticsIntensity
        };
        return pick(parameters);
    }

private:
    // most of the time, pick a valid port id
    void pickPortId(int &id) {
        const auto &portIds = persistData->portIds;
        auto v = uint();
        if (v < 8) {
            id = 0;
        } else if (v < 250 && !portIds.empty()) {
            id = pick(portIds);
        }
    }

    // most of the time, pick a valid port config id
    void pickPortConfigId(int &id) {
        const auto &portConfigIds = persistData->portConfigIds;
        auto v = uint();
        if (v < 8) {
            id = 0;
        } else if (v < 250 && !portConfigIds.empty()) {
            id = pick(portConfigIds);
        }
    }

    AudioPersistData *persistData;
};

struct AudioStreamCommonFuzzer: public IStreamCommonFuzzer<AudioDataProvider, IStreamCommon> {
    bool closed = false;

    AudioStreamCommonFuzzer(AudioDataProvider *provider, IStreamCommon *stream): IStreamCommonFuzzer(provider, stream) {}

    void fuzz_close() override {
        IStreamCommonFuzzer<AudioDataProvider, IStreamCommon>::fuzz_close();
        closed = true;
    }
};

struct AudioStreamInFuzzer : public IStreamInFuzzer<AudioDataProvider, IStreamIn> {
    AudioStreamInFuzzer(AudioDataProvider *provider, IStreamIn *stream): IStreamInFuzzer(provider, stream) {}

    std::shared_ptr<IStreamCommon> getStreamCommon() {
        std::shared_ptr<IStreamCommon> stream;
        target->getStreamCommon(&stream);
        return std::move(stream);
    }
};

struct AudioStreamOutFuzzer : public IStreamOutFuzzer<AudioDataProvider, IStreamOut> {
    AudioStreamOutFuzzer(AudioDataProvider *provider, IStreamOut *stream): IStreamOutFuzzer(provider, stream) {}

    std::shared_ptr<IStreamCommon> getStreamCommon() {
        std::shared_ptr<IStreamCommon> stream;
        target->getStreamCommon(&stream);
        return std::move(stream);
    }
};

struct AudioModuleFuzzer : public IModuleFuzzer<AudioDataProvider, Module> {
    AudioModuleFuzzer(AudioDataProvider *provider, Module *mod): IModuleFuzzer(provider, mod) {}

    std::optional<IModule::OpenInputStreamReturn> fuzz_openInputStream() override {
        auto ret = IModuleFuzzer<AudioDataProvider, Module>::fuzz_openInputStream();
        if (ret) {
            provider->getPersistData()->inStreams.push_back(ret.value().stream);
        }
        return ret;
    }

    std::optional<IModule::OpenOutputStreamReturn> fuzz_openOutputStream() override {
        auto ret = IModuleFuzzer<AudioDataProvider, Module>::fuzz_openOutputStream();
        if (ret) {
            provider->getPersistData()->outStreams.push_back(ret.value().stream);
        }
        return ret;
    }

    // avoid assertion in connectExternalDevice call in get<Tag::device>()
    std::optional<AudioPort> fuzz_connectExternalDevice() override {
        AudioPort inData;
        provider->genAudioPort(inData);

        if (inData.ext.getTag() == AudioPortExt::Tag::device) {
            AudioPort _aidl_return;
            if (target->connectExternalDevice(inData, &_aidl_return).isOk()) {
                return _aidl_return;
            }
        }
        return std::nullopt;
    }

    // record port IDs in persist data
    virtual std::optional<std::vector<AudioPort>> fuzz_getAudioPorts() override {
        std::vector<AudioPort> ports;
        if (target->getAudioPorts(&ports).isOk()) {
            auto &portIds = provider->getPersistData()->portIds;
            portIds.clear();
            for (const auto &port : ports) {
                portIds.push_back(port.id);
            }
            return std::move(ports);
        }
        return std::nullopt;
    }

    // record port config IDs in persist data
    virtual std::optional<std::vector<AudioPortConfig>> fuzz_getAudioPortConfigs() override {
        std::vector<AudioPortConfig> configs;
        if (target->getAudioPortConfigs(&configs).isOk()) {
            auto &portConfigIds = provider->getPersistData()->portConfigIds;
            portConfigIds.clear();
            for (const auto &config : configs) {
                portConfigIds.push_back(config.id);
            }
            return std::move(configs);
        } else {
            return std::nullopt;
        }
    }

    virtual std::optional<bool> fuzz_setAudioPortConfig() override {
        AudioPortConfig config;
        provider->genAudioPortConfig(config);
        if (provider->getPersistData()->portConfigIds.size() < 4) {
            // if there are not many port configs, let's try to generate some legitimate data that
            // can pass checks in setAudioPortConfig()
            const auto &portIds = provider->getPersistData()->portIds;
            if (portIds.empty()) {
                return std::nullopt;
            }
            config.portId = provider->pick(portIds);
            config.id = 0;

            AudioPort port;
            if (!target->getAudioPort(config.portId, &port).isOk()) {
                return std::nullopt;
            }
            config.flags = port.flags;

            if (port.profiles.empty()) {
                return std::nullopt;
            }
            const auto &profile = provider->pick(port.profiles);
            config.format = profile.format;

            if (profile.channelMasks.empty() || profile.sampleRates.empty()) {
                return std::nullopt;
            }
            config.channelMask = provider->pick(profile.channelMasks);
            auto sampleRate = provider->pick(profile.sampleRates);
            config.sampleRate = aidl::android::media::audio::common::Int(sampleRate);
        }

        AudioPortConfig out_suggested;
        bool _aidl_return;

        if (target->setAudioPortConfig(config, &out_suggested, &_aidl_return).isOk()) {
            return std::move(_aidl_return);
        } else {
            return std::nullopt;
        }
    }

    virtual std::optional<std::vector<VendorParameter>> fuzz_getVendorParameters() override {
        std::vector<std::string> in_ids;
        provider->gen(in_ids, [this](auto &v) { v = provider->pickVendorParameter(); });
        std::vector<::aidl::android::hardware::audio::core::VendorParameter> _aidl_return;

        if (target->getVendorParameters(in_ids, &_aidl_return).isOk()) {
        return std::move(_aidl_return);
        } else {
        return std::nullopt;
        }
    }
};

template <typename T, typename F>
void fuzzStream(AudioDataProvider *provider, std::vector<std::shared_ptr<T>> &streams) {
    if (!streams.empty()) {
        auto index = provider->inner().ConsumeIntegralInRange<size_t>(0, streams.size() - 1);
        auto stream = streams[index];

        auto fuzzer = F(provider, stream.get());
        fuzzer.fuzz();

        std::shared_ptr<IStreamCommon> common = fuzzer.getStreamCommon();
        AudioStreamCommonFuzzer commonFuzzer(provider, common.get());
        commonFuzzer.fuzz();
        if (commonFuzzer.closed) {
            commonFuzzer.closed = false;
            streams.erase(streams.begin() + index);
        }
    }
}

extern "C" binder_status_t registerService(void);

class AudioFuzzer {
public:
    void setProvider(AudioDataProvider *provider) {
        this->provider = provider;
    }

    void fuzz() {
        if (!modulePrimary) {
            // register agm service
            registerService();
            modulePrimary = ndk::SharedRefBase::make<ModulePrimary>();
        }
        moduleFuzzer = std::make_unique<AudioModuleFuzzer>(provider, modulePrimary.get());
        moduleFuzzer->fuzz();

        fuzzStream<IStreamIn, AudioStreamInFuzzer>(provider, provider->getPersistData()->inStreams);
        fuzzStream<IStreamOut, AudioStreamOutFuzzer>(provider, provider->getPersistData()->outStreams);
    }

private:
    AudioDataProvider *provider = nullptr;
    std::shared_ptr<Module> modulePrimary = nullptr;
    std::unique_ptr<AudioModuleFuzzer> moduleFuzzer = nullptr;
};

extern "C" int LLVMFuzzerTestOneInput(const char *data, size_t size) {
    static AudioFuzzer fuzzer;
    static AudioPersistData persistData;

    AudioDataProvider provider(data, size, &persistData);
    fuzzer.setProvider(&provider);

    fuzzer.fuzz();
    return 0;
}
