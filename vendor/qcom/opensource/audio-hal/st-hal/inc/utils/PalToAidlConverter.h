/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/android/hardware/soundtrigger3/BnSoundTriggerHw.h>
#include <aidl/android/hardware/soundtrigger3/BnSoundTriggerHwCallback.h>
#include <aidl/android/hardware/soundtrigger3/BnSoundTriggerHwGlobalCallback.h>
#include <string>

#include "PalDefs.h"

namespace aidl::android::hardware::soundtrigger3 {
using namespace ::aidl::android::media::soundtrigger;
using namespace ::aidl::android::media::audio::common;

struct PalToAidlConverter {
    /*
     * @brief convertProperties convert properties from pal to aidl format
     * @param palProperties properties from PAL
     * @param aidlProperties to be copied from palProperties
     */
    static int convertProperties(pal_st_properties *palProperties,
                                 Properties &aidlProperties);

    /*
     * @brief convertRecognitionEvent convert pal_st_recognition_event from
     * PAL to AIDL RecognitionEvent
     * @param palEvent pal_st_recognition_event of PAL format
     * @param aidlEvent to be copied from PAL pal_st_recognition_event
     */
    static void convertRecognitionEvent(pal_st_recognition_event *palEvent,
                                        RecognitionEvent &aidlEvent);

    /*
     * @brief convertPhraseRecognitionEvent convert pal_st_phrase_recognition_event
     * from PAL to AIDL PhraseRecognitionEvent
     * @param palEvent pal_st_phrase_recognition_event of PAL format
     * @param palPhrase to be copied from PAL pal_st_phrase_recognition_event
     */
    static void convertPhraseRecognitionEvent(
        pal_st_phrase_recognition_event *palEvent,
        PhraseRecognitionEvent &event);

    /*
     * @brief convertPhraseRecognitionExtra convert pal_st_phrase_recognition_extra
     * from PAL to AIDL PhraseRecognitionExtra
     * @param palExtra
     * @param aidlExtra
     */
    static void convertPhraseRecognitionExtra(
        const struct pal_st_phrase_recognition_extra *palExtra,
        PhraseRecognitionExtra &aidlExtra);

    /*
     * @brief createAudioConfig creates AudioConfig for a palEvent
     * @param palEvent callback event
     * @return AudioConfig
     */
    static AudioConfig createAudioConfig(const pal_st_recognition_event *palEvent);
};

} // namespace aidl::android::hardware::soundtrigger3
