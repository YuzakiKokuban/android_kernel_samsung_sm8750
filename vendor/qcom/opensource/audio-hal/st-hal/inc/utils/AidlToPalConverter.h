/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/android/hardware/soundtrigger3/BnSoundTriggerHw.h>
#include <aidl/android/hardware/soundtrigger3/BnSoundTriggerHwCallback.h>
#include <aidl/android/hardware/soundtrigger3/BnSoundTriggerHwGlobalCallback.h>

#include <string>
#include <vector>
#include "PalDefs.h"

namespace aidl::android::hardware::soundtrigger3 {
using namespace ::aidl::android::media::soundtrigger;
using namespace ::aidl::android::media::audio::common;

struct AidlToPalConverter {
    /*
     * @brief convertPhraseSoundModel convert PhraseSoundModel from AIDL to uint8_t
     * memory, where clients can typecast to pal_param_payload for further usage.
     * Clients need not to deallocate casted memory, that will be taken care by
     * lifetime of vector.
     * @param aidlModel PhraseSoundModel of AIDL format
     * @param empty payload vector passed by client, converter calculates the needed
     * allocation and fills the vector as per pal_param_payload and needful PAL types
     * client can cast payload to pal_param_payload and use for pal_set_param or other
     * PAL APIs.
     */
    static void convertPhraseSoundModel(const PhraseSoundModel &aidlModel,
                                        std::vector<uint8_t> &payload);

    /*
     * @brief convertSoundModel convert SoundModel from AIDL to uint8_t memory,
     * where clients can typecast to pal_param_payload for further usage.
     * clients need not to deallocate casted memory, that will be taken care by
     * lifetime of vector.
     * @param aidlModel SoundModel of AIDL format
     * @param empty payload vector passed by client, converter calculates the needed
     * allocation and fills the vector as per pal_param_payload and needful PAL types
     * client can cast payload to pal_param_payload and use for pal_set_param or other
     * PAL APIs.
     */
    static void convertSoundModel(const SoundModel &aidlModel,
                                  std::vector<uint8_t> &payload);

    /*
     * @brief convertRecognitionConfig convert RecognitionConfig from AIDL to
     * memory, where clients can typecast to pal_param_payload for further usage.
     * Clients need not to dellocate casted memory, that will be taken care by
     * lifetime of vector.
     * @param aidlConfig RecognitionConfig of AIDL format
     * @param empty payload vector passed by client, converter calculates the needed
     * allocation and fills the vector as per pal_param_payload and needful PAL types
     * client can cast payload to pal_param_payload and use for pal_set_param or other
     * PAL APIs.
     */
    static void convertRecognitionConfig(const RecognitionConfig &aidlConfig,
                                         std::vector<uint8_t> &payload);
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
    static int backlog_size;
    static int amodel_mode;
#endif
private:
    /*
     * @brief sharedMemoryToArray converts sharedmemory contained in aidlModel to opaque array
     * @param aidlModel containing shared memory
     * @param palPhraseSoundModel after reading from aidlModel's sharedMemory data will be
     * copied at data_offset of palPhraseSoundModel
     */
    static void sharedMemoryToArray(
        const PhraseSoundModel &aidlModel,
        struct pal_st_phrase_sound_model *palPhraseSoundModel);

    /*
     * @brief sharedMemoryToArray converts sharedmemory contained in aidlModel to opaque array
     * @param aidlModel containing shared memory
     * @param palSoundModel after reading from aidlModel's sharedMemory data will be
     * copied at data_offset of palSoundModel
     */
    static void sharedMemoryToArray(const SoundModel &aidlModel,
                                    struct pal_st_sound_model *palSoundModel);

    /*
     * @brief convertRecognitionConfigInternal convert RecognitionConfig from
     * AIDL to PAL format pal_st_recognition_config
     * @param aidlConfig RecognitionConfig of AIDL format
     * @param palRecognitionConfig which is to be copied from AIDL RecognitionConfig
     */
    static void convertRecognitionConfigInternal(
        const RecognitionConfig &aidlConfig,
        struct pal_st_recognition_config *palRecognitionConfig);

    /*
     * @brief convertSoundModelInternal convert SoundModel from AIDL to PAL format
     * @param aidlModel SoundModel of AIDL format
     * @param palSoundModel to be copied from AIDL SoundModel
     */
    static void convertSoundModelInternal(
        const SoundModel &aidlModel,
        struct pal_st_sound_model *palSoundModel);

    /*
     * @brief convertPhraseSoundModelInternal convert PhraseSoundModel from AIDL to PAL format
     * @param aidlModel PhraseSoundModel of AIDL format
     * @param palPhraseSoundModel to be copied from AIDL PhraseSoundModel
     */
    static void convertPhraseSoundModelInternal(
        const PhraseSoundModel &aidlModel,
        struct pal_st_phrase_sound_model *palPhraseSoundModel);

    /*
     * @brief convertPhrase convert Phrase from AIDL to PAL pal_st_phrase
     * @param aidlPhrase Phrase of AIDL format
     * @param palPhrase to be copied from AIDL Phrase
     */
    static void convertPhrase(const Phrase &aidlPhrase, struct pal_st_phrase *palPhrase);

    /*
     * @brief convertPhraseRecognitionExtra convert PhraseRecognitionExtra from
     * AIDL to PAL pal_st_phrase_recognition_extra
     * @param aidlExtra
     * @param palExtra
     */
    static void convertPhraseRecognitionExtra(
        const PhraseRecognitionExtra &aidlExtra,
        struct pal_st_phrase_recognition_extra *palExtra);
};

} // namespace aidl::android::hardware::soundtrigger3
