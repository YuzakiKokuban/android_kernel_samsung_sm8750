/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "sthal_AidlToPalConverter"

#include <android-base/macros.h>
#include <log/log.h>
#include <utils/AidlToPalConverter.h>
#include <utils/CoreUtils.h>
#include <utils/SharedMemoryWrapper.h>

#include "PalApi.h"

#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
#include <cutils/str_parms.h>

struct sound_trigger_ss_va_config
{
    int32_t ctrl_SVT_MODE;      // 0 : SVT OFF, 1 : Voice Trigger w/ LPSD , 2 : Sound Detect, 3 : Voice Trigger w/o LPSD
    int32_t ctrl_LPSD;          // 0 : LPSD OFF, 1: LPSD ON
    int32_t ctrl_VT;            // 0 : Voice Trigger OFF, 1 : Voice Trigger ON
    int32_t ctrl_hist_buf_time; // 0 ~ 2000ms
};

static void ParseUserData(const char *keyValuePairs)
{
    ALOGD("%s : %s", __func__, keyValuePairs);
    struct str_parms *parms = str_parms_create_str(keyValuePairs);
    int value;
    int ret;

    if (!parms) {
        ALOGE("%s: str_params NULL", __func__);
        return;
    }

    ret = str_parms_get_int(parms, "voice_trig", &value);
    if (ret >= 0) {
        ALOGV("voice_trigger = (%d)", value);
        aidl::android::hardware::soundtrigger3::AidlToPalConverter::backlog_size = value;
        str_parms_del(parms, "voice_trig");
    }
    ret = str_parms_get_int(parms, "wakeup_mode", &value);
    if (ret >= 0) {
        ALOGV("wakeup_mode = (%d)", value);
        aidl::android::hardware::soundtrigger3::AidlToPalConverter::amodel_mode = value;
        str_parms_del(parms, "wakeup_mode");
    }
    str_parms_destroy(parms);
}
namespace aidl::android::hardware::soundtrigger3 {
using namespace ::aidl::android::media::audio::common;
int AidlToPalConverter::backlog_size = 0;
int AidlToPalConverter::amodel_mode = 0;
}
#endif

namespace aidl::android::hardware::soundtrigger3 {
using namespace ::aidl::android::media::audio::common;

void AidlToPalConverter::sharedMemoryToArray(const SoundModel &aidlModel,
                                             struct pal_st_sound_model *palSoundModel)
{
    SharedMemoryWrapper memWrapper(aidlModel.data.get(), palSoundModel->data_size);
    const uint8_t *src = memWrapper.data();
    uint8_t *dst = reinterpret_cast<uint8_t *>(palSoundModel) + palSoundModel->data_offset;

    memcpy(dst, src, palSoundModel->data_size);
}

void AidlToPalConverter::sharedMemoryToArray(const PhraseSoundModel &aidlModel,
                                             struct pal_st_phrase_sound_model *palPhraseSoundModel)
{
    sharedMemoryToArray(aidlModel.common, &(palPhraseSoundModel->common));
}

void AidlToPalConverter::convertPhrase(const Phrase &aidlPhrase,
                                       struct pal_st_phrase *palPhrase)
{
    palPhrase->id = aidlPhrase.id;
    palPhrase->recognition_mode = aidlPhrase.recognitionModes;
    palPhrase->num_users = aidlPhrase.users.size();
    for (int i = 0; i < palPhrase->num_users; i++)
        palPhrase->users[i] = aidlPhrase.users[i];

    strlcpy(palPhrase->locale, aidlPhrase.locale.c_str(), PAL_SOUND_TRIGGER_MAX_LOCALE_LEN);
    strlcpy(palPhrase->text, aidlPhrase.text.c_str(), PAL_SOUND_TRIGGER_MAX_STRING_LEN);
}

void AidlToPalConverter::convertPhraseSoundModelInternal(
                          const PhraseSoundModel &aidlModel,
                          struct pal_st_phrase_sound_model *palPhraseSoundModel)
{
    palPhraseSoundModel->common.type = (pal_st_sound_model_type_t)aidlModel.common.type;

    CoreUtils::convertStringtoUuid(aidlModel.common.uuid, &(palPhraseSoundModel->common.uuid));
    CoreUtils::convertStringtoUuid(aidlModel.common.vendorUuid,
                                    &(palPhraseSoundModel->common.vendor_uuid));

    palPhraseSoundModel->common.data_size = aidlModel.common.dataSize;
    palPhraseSoundModel->common.data_offset = sizeof(struct pal_st_phrase_sound_model);

    palPhraseSoundModel->num_phrases = aidlModel.phrases.size(); // TODO change num_phrase range.

    for (int i = 0; i < palPhraseSoundModel->num_phrases; i++) {
        convertPhrase(aidlModel.phrases[i], &palPhraseSoundModel->phrases[i]);
    }

    sharedMemoryToArray(aidlModel, palPhraseSoundModel);
}

void AidlToPalConverter::convertSoundModelInternal(const SoundModel &aidlModel,
                                                   struct pal_st_sound_model *palSoundModel)
{
    palSoundModel->type = (pal_st_sound_model_type_t)aidlModel.type;
    CoreUtils::convertStringtoUuid(aidlModel.uuid, &(palSoundModel->uuid));
    CoreUtils::convertStringtoUuid(aidlModel.vendorUuid, &(palSoundModel->vendor_uuid));
    palSoundModel->data_size = aidlModel.dataSize;
    palSoundModel->data_offset = sizeof(struct pal_st_sound_model);
    sharedMemoryToArray(aidlModel, palSoundModel);
}

void AidlToPalConverter::convertPhraseRecognitionExtra(
                          const PhraseRecognitionExtra &aidlExtra,
                          struct pal_st_phrase_recognition_extra *palExtra)
{
    palExtra->id = aidlExtra.id;
    palExtra->recognition_modes = aidlExtra.recognitionModes;
    palExtra->confidence_level = aidlExtra.confidenceLevel;
    palExtra->num_levels = aidlExtra.levels.size(); // TODO range check for num_levels

    for (int i = 0; i < palExtra->num_levels; i++) {
        palExtra->levels[i].user_id = aidlExtra.levels[i].userId;
        palExtra->levels[i].level = aidlExtra.levels[i].levelPercent;
    }
}

void AidlToPalConverter::convertRecognitionConfigInternal(
                          const RecognitionConfig &aidlConfig,
                          struct pal_st_recognition_config *palRecognitionConfig)
{
    palRecognitionConfig->capture_requested = aidlConfig.captureRequested;
    palRecognitionConfig->num_phrases = aidlConfig.phraseRecognitionExtras.size(); // range check

    for (int i = 0; i < palRecognitionConfig->num_phrases; i++) {
        convertPhraseRecognitionExtra(aidlConfig.phraseRecognitionExtras[i],
                                            &palRecognitionConfig->phrases[i]);
    }

#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
    if (amodel_mode > 0)
        palRecognitionConfig->data_size = sizeof(struct sound_trigger_ss_va_config);
    else
#endif
    palRecognitionConfig->data_size = aidlConfig.data.size();
    palRecognitionConfig->data_offset = sizeof(struct pal_st_recognition_config);

    const uint8_t *src = aidlConfig.data.data();
    uint8_t *dst = reinterpret_cast<uint8_t *>(palRecognitionConfig) +
                                                palRecognitionConfig->data_offset;
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
    if (amodel_mode > 0) {
        sound_trigger_ss_va_config *ss_param = (sound_trigger_ss_va_config *)dst;
        ss_param->ctrl_SVT_MODE = amodel_mode;
        ss_param->ctrl_LPSD = 0;
        ss_param->ctrl_VT = 0;
        ss_param->ctrl_hist_buf_time = backlog_size;
        palRecognitionConfig->capture_handle = AUDIO_IO_HANDLE_NONE;
        ALOGD("%s: ss_param->ctrl_SVT_MODE %d, ss_param->ctrl_hist_buf_time %d, palRecognitionConfig->capture_handle = %d",
            __func__, ss_param->ctrl_SVT_MODE, ss_param->ctrl_hist_buf_time, palRecognitionConfig->capture_handle);
    } else
#endif
    memcpy(dst, src, palRecognitionConfig->data_size);
}

void AidlToPalConverter::convertPhraseSoundModel(const PhraseSoundModel &aidlModel,
                                                 std::vector<uint8_t> & payload)
{
    size_t payloadSize = sizeof(struct pal_st_phrase_sound_model) + aidlModel.common.dataSize;
    size_t allocSize = sizeof(pal_param_payload) + payloadSize;
    // resize will throw bad_alloc in case of failure, no need for further nullptr checks
    payload.resize(allocSize);

    pal_param_payload *paramPayload = reinterpret_cast<pal_param_payload *> (payload.data());
    paramPayload->payload_size = payloadSize;

    struct pal_st_phrase_sound_model *palPhraseSoundModel =
                               (struct pal_st_phrase_sound_model *)paramPayload->payload;
    convertPhraseSoundModelInternal(aidlModel, palPhraseSoundModel);
}

void AidlToPalConverter::convertSoundModel(const SoundModel &aidlModel,
                                           std::vector<uint8_t> & payload)
{
    size_t payloadSize = sizeof(struct pal_st_sound_model) + aidlModel.dataSize;
    size_t allocSize = sizeof(pal_param_payload) + payloadSize;
    payload.resize(allocSize);

    pal_param_payload *paramPayload = reinterpret_cast<pal_param_payload *> (payload.data());
    paramPayload->payload_size = payloadSize;

    struct pal_st_sound_model *palSoundModel = (struct pal_st_sound_model *)paramPayload->payload;
    convertSoundModelInternal(aidlModel, palSoundModel);
}

void AidlToPalConverter::convertRecognitionConfig(const RecognitionConfig &config,
                                                  std::vector<uint8_t> & payload)
{
    size_t payloadSize = sizeof(struct pal_st_recognition_config) + config.data.size();
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
    amodel_mode = 0;
    backlog_size = 0;
    int param_size = config.data.size();
    if (param_size > 0) {
        char *params = (char*)config.data.data();
        if (params) {
            char *paramsEndWithNull = (char*)malloc(param_size + 1);
            if (paramsEndWithNull) {
                memset(paramsEndWithNull, 0, param_size + 1);
                memcpy(paramsEndWithNull, params, param_size);
                ALOGI("%s params(%d) : %s", __func__, param_size, paramsEndWithNull);
                ParseUserData(paramsEndWithNull);
                free (paramsEndWithNull);
            } else {
                ALOGI("%s params(%d) : %s", __func__, param_size, params);
                ParseUserData(params);
            }
        }
    }

    if (amodel_mode > 0) {
        payloadSize = sizeof(struct pal_st_recognition_config) +
            sizeof(struct sound_trigger_ss_va_config);
    }
#endif
    size_t allocSize = sizeof(pal_param_payload) + payloadSize;
    payload.resize(allocSize);

    pal_param_payload *recognitionParamPayload = reinterpret_cast<pal_param_payload *> (payload.data());

    recognitionParamPayload->payload_size = payloadSize;
    struct pal_st_recognition_config *palRecognitionConfig =
                            (struct pal_st_recognition_config *)recognitionParamPayload->payload;
    convertRecognitionConfigInternal(config, palRecognitionConfig);
}

} // namespace aidl::android::hardware::soundtrigger3
