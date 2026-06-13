/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "STHAL: CoreUtils"
#include <log/log.h>
#include <utils/CoreUtils.h>
#include <soundtriggerhw/SoundTriggerCommon.h>

#include "PalApi.h"

const std::string acdUuid = "4e93281b-296e-4d73-9833-2710c3c7c1db";

namespace aidl::android::hardware::soundtrigger3 {

pal_stream_type_t CoreUtils::getStreamType(const SoundModel &aidlModel)
{
    pal_stream_type_t stream = PAL_STREAM_VOICE_UI;

    if (aidlModel.vendorUuid == acdUuid)
        stream = PAL_STREAM_ACD;
    return stream;
}

pal_stream_type_t CoreUtils::getStreamType(const PhraseSoundModel &aidlModel)
{
    return getStreamType(aidlModel.common);
}

bool CoreUtils::isAcdStream(const SoundModel &model)
{
    return getStreamType(model) == PAL_STREAM_ACD;
}

bool CoreUtils::isValidSoundModel(const SoundModel &aidlModel)
{
    if (!isAcdStream(aidlModel) && (aidlModel.dataSize == 0)) {
        STHAL_ERR(LOG_TAG, "Invalid sound model %s", aidlModel.toString().c_str());
        return false;
    }
    return true;
}

bool CoreUtils::isValidPhraseSoundModel(const PhraseSoundModel &aidlModel)
{
    if ((aidlModel.common.dataSize == 0) ||
        (aidlModel.common.type != SoundModelType::KEYPHRASE) ||
        (aidlModel.phrases.size() == 0)) {
        STHAL_ERR(LOG_TAG, "Invalid phrase sound model %s", aidlModel.toString().c_str());
        return false;
    }
    return true;
}

std::string CoreUtils::convertUuidToString(const st_uuid &uuid)
{
    char str[64];

    snprintf(str, sizeof(str), "%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
             uuid.timeLow, uuid.timeMid, uuid.timeHiAndVersion, uuid.clockSeq,
             uuid.node[0], uuid.node[1], uuid.node[2], uuid.node[3], uuid.node[4],
             uuid.node[5]);
    return std::string(str);
}

void CoreUtils::convertStringtoUuid(const std::string &aidlUuid, st_uuid *palUuid)
{
    int temp[10];
    const char *inUuid = aidlUuid.c_str();

    if (inUuid == NULL || (sscanf(inUuid, "%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
                                  temp, temp + 1, temp + 2, temp + 3, temp + 4, temp + 5,
                                  temp + 6, temp + 7, temp + 8, temp + 9) < 10)) {
        return;
    }
    palUuid->timeLow = (uint32_t)temp[0];
    palUuid->timeMid = (uint16_t)temp[1];
    palUuid->timeHiAndVersion = (uint16_t)temp[2];
    palUuid->clockSeq = (uint16_t)temp[3];
    palUuid->node[0] = (uint8_t)temp[4];
    palUuid->node[1] = (uint8_t)temp[5];
    palUuid->node[2] = (uint8_t)temp[6];
    palUuid->node[3] = (uint8_t)temp[7];
    palUuid->node[4] = (uint8_t)temp[8];
    palUuid->node[5] = (uint8_t)temp[9];
}

void CoreUtils::printPalRecognitionEvent(struct pal_st_recognition_event *palEvent)
{
    std::ostringstream os;

    os << "pal_st_recognition_event {";
    os << "status: " << palEvent->status;
    os << ", type: " << palEvent->type;
    os << ", st_handle: " << palEvent->st_handle;
    os << ", capture_available: " << palEvent->capture_available;
    os << ", capture_session: " << palEvent->capture_session;
    os << ", capture_delay_ms: " << palEvent->capture_delay_ms;
    os << ", capture_preamble_ms: " << palEvent->capture_preamble_ms;
    os << ", trigger_in_data: " << palEvent->trigger_in_data;
    os << ", media_config {: ";
    os << ", sample_rate: " << palEvent->media_config.sample_rate;
    os << ", bit_width : " << palEvent->media_config.bit_width;
    os << ", aud_fmt_id {: " << palEvent->media_config.aud_fmt_id;
    os << ", ch_info {: ";
    os << ", channels :" << palEvent->media_config.ch_info.channels;
    for (int i = 0; i < palEvent->media_config.ch_info.channels; i++)
        os << ", " << palEvent->media_config.ch_info.ch_map[i];
    os << "}";
    os << "}";
    STHAL_VERBOSE(LOG_TAG, "%s ", os.str().c_str());
}

void CoreUtils::printPalStRecognitionConfig(
    struct pal_st_recognition_config *palRecognitionConfig)
{
    std::ostringstream os;

    os << "pal_st_recognition_config{";
    os << "capture_handle: " << palRecognitionConfig->capture_handle;
    os << ", capture_device: " << palRecognitionConfig->capture_device;
    os << ", capture_requested: " << palRecognitionConfig->capture_requested;
    os << ", num_phrases: " << palRecognitionConfig->num_phrases;
//    for (int i = 0; i < palRecognitionConfig->num_phrases; i++) {
//        os << ", Phrases: " << palRecognitionConfig->phrases[i].toString();
//    }
    os << ", pal_st_recognition_callback_t: " << palRecognitionConfig->callback;
    os << ", data_size: " << palRecognitionConfig->data_size;
    os << ", data_offset: " << palRecognitionConfig->data_offset;
    uint8_t *dst = reinterpret_cast<uint8_t *>(palRecognitionConfig) +
                                                    palRecognitionConfig->data_offset;
    os << " Data [";

    for (int i = 0; i < palRecognitionConfig->data_size; i++) {
        if (i == 0)
            os << std::to_string(dst[i]);
        else
            os << ", " << std::to_string(dst[i]);
    }
    os << "]";
    os << "}";
    STHAL_VERBOSE(LOG_TAG, "%s ", os.str().c_str());
}

void CoreUtils::printPalStUuid(struct st_uuid *uuid)
{
    std::ostringstream os;
    char str[64];

    snprintf(str, sizeof(str), "%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
            uuid->timeLow, uuid->timeMid, uuid->timeHiAndVersion, uuid->clockSeq,
            uuid->node[0], uuid->node[1], uuid->node[2], uuid->node[3], uuid->node[4],
            uuid->node[5]);
    os << "pal_st_uuid {" << str << "}";
    STHAL_VERBOSE(LOG_TAG, "%s ", os.str().c_str());
}

Status CoreUtils::convertHalErrorCodeToAidlStatus(int halError)
{
    switch(halError) {
        case -EBUSY:
            return Status::RESOURCE_CONTENTION;
        case -ENOSYS:
            return Status::OPERATION_NOT_SUPPORTED;
        case -EPERM:
            return Status::TEMPORARY_PERMISSION_DENIED;
        case -EPIPE:
            return Status::DEAD_OBJECT;
        default:
            return Status::INTERNAL_ERROR;
    }
}

::ndk::ScopedAStatus CoreUtils::halErrorToAidl(int halError)
{
    if (halError == STATUS_OK)
        return ndk::ScopedAStatus::ok();
    auto status = convertHalErrorCodeToAidlStatus(halError);
    return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
        static_cast<int32_t>(status), toString(status).c_str()));
}

} // namespace aidl::android::hardware::soundtrigger3
