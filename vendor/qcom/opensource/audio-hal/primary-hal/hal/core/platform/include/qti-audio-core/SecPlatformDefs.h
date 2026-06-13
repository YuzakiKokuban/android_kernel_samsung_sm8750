/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

// pal device direction for voice
enum {
    PAL_RX = 0,
    PAL_TX,
    PAL_TX_RX,
};

typedef enum {
    VOICE_DEVICE_INVALID   = -1,
    VOICE_DEVICE_ETC       = 0,
    VOICE_DEVICE_SPEAKER,
    VOICE_DEVICE_EARPHONE,
    VOICE_DEVICE_BLUETOOTH,
    VOICE_DEVICE_RECEIVER,
    VOICE_DEVICE_MAX
} voice_device_type;

#ifdef SEC_AUDIO_CALL_HAC
enum {
    HAC_MODE_MIC = 0,
    HAC_MODE_TCOIL,
    HAC_MODE_MAX
};
#endif

#ifdef SEC_AUDIO_CALL_TRANSLATION
enum {
    TRANSLATION_MODE_OFF = 0,
    TRANSLATION_MODE_HANDSET,
    TRANSLATION_MODE_SPEAKER,
    TRANSLATION_MODE_EARPHONE,
    TRANSLATION_MODE_BLUETOOTH,
};
#endif

enum {
    EFFECTS_MICMODE_STANDARD    = 0,  /* default */
    EFFECTS_MICMODE_VOICE_FOCUS = 1,
    EFFECTS_MICMODE_ALL_SOUND   = 2,
    EFFECTS_TRANSLATION   = 3,
    EFFECTS_MICMODE_DEFAULT     = 100,
};

enum {
    CALLMEMO_ON        = 0x00000001, /* need to enable music device */
    CALLMEMO_OFF       = 0x00000002,
    CALLMEMO_REC       = 0x00000010, /* need to disable music mixer path, and keep music device */
    CALLMEMO_INIT      = 0x10000000,
};

#ifdef SEC_AUDIO_CALL_VOIP
// refer to vendor/qcom/proprietary/mm-audio/ar-acdb/acdbdata/inc/kvh2xml.h
const std::map<uint32_t, uint32_t> getVoipMicMode {
    {EFFECTS_MICMODE_STANDARD,    1 /* STANDARD_MODE */},
    {EFFECTS_MICMODE_VOICE_FOCUS, 2 /* VOICE_FOCUS_MODE */},
    {EFFECTS_MICMODE_ALL_SOUND,   3 /* ALL_SOUND_MODE */},
    {EFFECTS_TRANSLATION,   4 /* TRANSLATION_MODE */},
};

const std::map<uint32_t, uint32_t> getVoipSampleRate {
    {8000,     0 /* VOIP_SR_NB */},
    {16000,    1 /* VOIP_SR_WB */},
    {32000,    2 /* VOIP_SR_SWB */},
    {48000,    3 /* VOIP_SR_FB */},
};
#endif


#ifdef SEC_AUDIO_ENFORCED_AUDIBLE
enum {
    NOT_MUTE = 0,
    MUTE_CALL,
    MUTE_CALL_AND_REC // For Camcording
};
#endif

#if defined(SEC_AUDIO_DUAL_SPEAKER) || defined(SEC_AUDIO_MULTI_SPEAKER)
// (TODO) check : need to move AudioEffect?
enum {
    TOP_UP = 0,
    RIGHT_UP,
    BUTTOM_UP,
    LEFT_UP
};

enum {
    FLATMOTION_NOT_FLAT = 0,
    FLATMOTION_FLAT
};
#endif

#ifdef SEC_AUDIO_ADAPT_SOUND
#define MAX_DHA_DATA_SIZE 40
enum {
    DHA_RESET = 0,   /* for CP call stop case, reset mixer as dha off */
    DHA_SET,         /* for CP call start case, set dha mixer */
    DHA_UPDATE      /* update dha param, and only call/wfc/vt case set dha mixer */
};
#endif

#ifdef SEC_AUDIO_FMRADIO
// defined in FM.cpp
const static std::string kHandleFM{"handle_fm"};
const static std::string kFMVolume{"fm_volume"};
const static std::string kFMMute{"fm_mute"};
const static std::string kFMRouting{"fm_routing"};

struct fmradio_config_t {
    bool on;
    audio_devices_t device;
    float volume;
    bool mute;
};
#endif

#ifdef SEC_AUDIO_SUPPORT_FLIP_CALL
enum {
    FOLDER_CLOSE = 0,
    FOLDER_OPEN = 1,
    FOLDER_FLEX_ON, // fold open/close
    FOLDER_FLEX_OFF // fold open
};
#endif
