/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "sthal_PalToAidlConverter"
#include "PalApi.h"
#include <log/log.h>
#include <utils/PalToAidlConverter.h>
#include <utils/CoreUtils.h>

#include <android-base/macros.h>
#include <cutils/ashmem.h>
#include <sys/mman.h>

#include <aidl/android/media/audio/common/AudioChannelLayout.h>

namespace aidl::android::hardware::soundtrigger3 {
using namespace ::aidl::android::media::audio::common;

int PalToAidlConverter::convertProperties(pal_st_properties *palProperties,
                                          Properties &aidlProperties)
{
    aidlProperties.implementor = reinterpret_cast<char *>(palProperties->implementor);
    aidlProperties.description = reinterpret_cast<char *>(palProperties->description);
    aidlProperties.version = palProperties->version;
    aidlProperties.uuid = CoreUtils::convertUuidToString(palProperties->uuid);
    aidlProperties.maxSoundModels = palProperties->max_sound_models;
    aidlProperties.maxKeyPhrases = palProperties->max_key_phrases;
    aidlProperties.maxUsers = palProperties->max_users;
    aidlProperties.recognitionModes = palProperties->recognition_modes;
    aidlProperties.captureTransition = palProperties->capture_transition;
    aidlProperties.maxBufferMs = palProperties->max_buffer_ms;
    aidlProperties.concurrentCapture = palProperties->concurrent_capture;
    aidlProperties.triggerInEvent = palProperties->trigger_in_event;
    aidlProperties.powerConsumptionMw = palProperties->power_consumption_mw;
    aidlProperties.audioCapabilities = 0;
    return STATUS_OK;
}

void PalToAidlConverter::convertPhraseRecognitionExtra(
    const struct pal_st_phrase_recognition_extra *palExtra,
    PhraseRecognitionExtra &aidlExtra)
{
    aidlExtra.id = palExtra->id;
    aidlExtra.recognitionModes = palExtra->recognition_modes;
    aidlExtra.confidenceLevel = palExtra->confidence_level;
    aidlExtra.levels.resize(palExtra->num_levels);
    for (int j = 0; j < palExtra->num_levels; j++) {
        aidlExtra.levels[j].userId = palExtra->levels[j].user_id;
        aidlExtra.levels[j].levelPercent = palExtra->levels[j].level;
    }
}

AudioConfig PalToAidlConverter::createAudioConfig(const pal_st_recognition_event *palEvent) {
    media::audio::common::AudioConfig aidlAudioConfig;
    aidlAudioConfig.base.sampleRate = palEvent->media_config.sample_rate;
    int channels = palEvent->media_config.ch_info.channels;
#define INPUT_LAYOUT(n)                                                        \
                AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>( \
                AudioChannelLayout::LAYOUT_##n)                                \

    if (channels == 1) {
        aidlAudioConfig.base.channelMask = INPUT_LAYOUT(MONO);
    } else if (channels == 2) {
        aidlAudioConfig.base.channelMask = INPUT_LAYOUT(STEREO);
    }
    aidlAudioConfig.base.format.type = AudioFormatType::PCM;
    aidlAudioConfig.base.format.pcm = PcmType::INT_16_BIT;
    return aidlAudioConfig;
}

void PalToAidlConverter::convertRecognitionEvent(pal_st_recognition_event *palEvent,
                                                 RecognitionEvent &aidlEvent)
{
    CoreUtils::printPalRecognitionEvent(palEvent);
    aidlEvent.status = static_cast<RecognitionStatus>(palEvent->status);
    aidlEvent.type = static_cast<SoundModelType>(palEvent->type);
    aidlEvent.captureAvailable = palEvent->capture_available;
    aidlEvent.captureDelayMs = palEvent->capture_delay_ms;
    aidlEvent.capturePreambleMs = palEvent->capture_preamble_ms;
    aidlEvent.triggerInData = palEvent->trigger_in_data;

    aidlEvent.audioConfig = createAudioConfig(palEvent);

    const uint8_t *src = reinterpret_cast<const uint8_t *>(palEvent) + palEvent->data_offset;
    aidlEvent.data.resize(palEvent->data_size);
    memcpy(&aidlEvent.data[0], src, palEvent->data_size);

    aidlEvent.recognitionStillActive = (aidlEvent.status == RecognitionStatus::FORCED); // TODO
}

void PalToAidlConverter::convertPhraseRecognitionEvent(pal_st_phrase_recognition_event *palEvent,
                                                       PhraseRecognitionEvent &aidlEvent)
{
    convertRecognitionEvent(&palEvent->common, aidlEvent.common);

    aidlEvent.phraseExtras.resize(palEvent->num_phrases);
    for (int i = 0; i < palEvent->num_phrases; i++) {
        convertPhraseRecognitionExtra(&palEvent->phrase_extras[i],
                                             aidlEvent.phraseExtras[i]);
    }
}

} // namespace aidl::android::hardware::soundtrigger3
