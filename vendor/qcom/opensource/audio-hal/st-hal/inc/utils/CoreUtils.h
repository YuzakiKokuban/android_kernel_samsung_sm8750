/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/android/hardware/soundtrigger3/BnSoundTriggerHw.h>
#include <aidl/android/hardware/soundtrigger3/BnSoundTriggerHwCallback.h>
#include <aidl/android/hardware/soundtrigger3/BnSoundTriggerHwGlobalCallback.h>
#include <aidl/android/media/soundtrigger/Status.h>

#include <string>
#include "PalDefs.h"

namespace aidl::android::hardware::soundtrigger3 {
using namespace ::aidl::android::media::soundtrigger;
using namespace ::aidl::android::media::audio::common;

struct CoreUtils {
    /*
     * @brief getStreamType returns stream of SoundModel from AIDL
     * @param aidlModel SoundModel of AIDL format
     * @return stream type
     */
    static pal_stream_type_t getStreamType(const SoundModel &aidlModel);

    /*
     * @brief getStreamType returns stream of PhraseSoundModel from AIDL
     * @param aidlModel PhraseSoundModel of AIDL format
     * @return stream type
     */
    static pal_stream_type_t getStreamType(const PhraseSoundModel &aidlModel);

    /*
     * @brief isAcdStream checks the stream of SoundModel from AIDL
     * @param aidlModel SoundModel of AIDL format
     * @return true if ACD stream
     */
    static bool isAcdStream(const SoundModel &aidlModel);

    /*
     * @brief isValidSoundModel checks if SoundModel of AIDL is valid
     * @param aidlModel SoundModel of AIDL format
     * @return true if SoundModel is valid
     */
    static bool isValidSoundModel(const SoundModel &model);

    /*
     * @brief isValidPhraseSoundModel checks if PhraseSoundModel of AIDL is valid
     * @param aidlModel PhraseSoundModel of AIDL format
     * @return true if PhraseSoundModel is valid
     */
    static bool isValidPhraseSoundModel(const PhraseSoundModel &model);

    /*
     * @brief convertUuidToString convert PAL st_uuid to std::string uuid format
     * @param uuid input uuid in format of st_uuid
     * @return uuid is std::string
     */
    static std::string convertUuidToString(const st_uuid &uuid);

    /*
     * @brief convertStringtoUuid convert uuid in format of uuid to PAL's st_uuid
     * @param uuid input
     * @param palUuid to be converted from input uuid
     */
    static void convertStringtoUuid(const std::string &uuid, st_uuid *palUuid);

    /*
     * @brief printPalRecognitionEvent prints pal_st_recognition_event
     * @param palEvent which needs to be printed
     */
    static void printPalRecognitionEvent(struct pal_st_recognition_event *palEvent);

    /*
     * @brief printPalStRecognitionConfig prints pal_st_recognition_config
     * @param palRecognitionConfig which needs to be printed
     */
    static void printPalStRecognitionConfig(
        struct pal_st_recognition_config *palRecognitionConfig);

    /*
     * @brief printPalStUuid print uuid
     * @param uuid which needs to be printed
     */
    static void printPalStUuid(struct st_uuid *uuid);

    static Status convertHalErrorCodeToAidlStatus(int halError);
    static ::ndk::ScopedAStatus halErrorToAidl(int halError);
};

} // namespace aidl::android::hardware::soundtrigger3
