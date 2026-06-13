/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: ResourceManager"
#include <agm/agm_api.h>
#include <cutils/properties.h>
#include <unistd.h>
#include <dlfcn.h>
#include <mutex>
#include <iostream>
#include <fstream>
#include <sys/ioctl.h>
#include "ResourceManager.h"
#include "Session.h"
#include "Device.h"
#include "Stream.h"
#include "StreamPCM.h"
#include "StreamCompress.h"
#include "StreamSoundTrigger.h"
#include "StreamACD.h"
#include "StreamASR.h"
#include "StreamInCall.h"
#include "StreamACDB.h"
#include "StreamContextProxy.h"
#include "StreamUltraSound.h"
#include "StreamSensorPCMData.h"
#include "StreamCommonProxy.h"
#include "StreamHaptics.h"
#include "StreamSensorRenderer.h"
#include "gsl_intf.h"
#include "Headphone.h"
#include "PayloadBuilder.h"
#include "Bluetooth.h"
#include "SpeakerMic.h"
#include "Speaker.h"
#include "SpeakerProtection.h"
#include "USBAudio.h"
#include "HeadsetMic.h"
#include "HandsetMic.h"
#include "DisplayPort.h"
#include "Handset.h"
#include "SndCardMonitor.h"
#include "UltrasoundDevice.h"
#include "ECRefDevice.h"
#include "HapticsDev.h"
#include "HapticsDevProtection.h"
#include "AudioHapticsInterface.h"
#include "VUIInterfaceProxy.h"
#include "kvh2xml.h"

#include "PerfLock.h"
#ifdef ENABLE_TAS_SPK_PROT
#include "TISpeakerProtDefs.h"
#endif

#ifndef FEATURE_IPQ_OPENWRT
#include <cutils/str_parms.h>
#endif

#ifdef SEC_AUDIO_COMMON
#include <time.h>
#endif
#ifdef SEC_AUDIO_BOOT_ON_ERR
#include <cutils/properties.h>
#endif
#ifdef SEC_AUDIO_MUTE_DETECTION
#include <audioInfo.h>
#endif
#define XML_PATH_EXTN_MAX_SIZE 80
#define XML_FILE_DELIMITER "_"
#define XML_FILE_EXT ".xml"
#define XML_PATH_MAX_LENGTH 100
#define HW_INFO_ARRAY_MAX_SIZE 32

#define VBAT_BCL_SUFFIX "-vbat"
#define SPKR_PROT_SUFFIX "-prot"

#if defined(FEATURE_IPQ_OPENWRT) || defined(LINUX_ENABLED)
#define SNDPARSER "/etc/card-defs.xml"
#else
#define SNDPARSER "/vendor/etc/card-defs.xml"
#endif

#if defined(ADSP_SLEEP_MONITOR)
#include <adsp_sleepmon.h>
#endif

#if LINUX_ENABLED
#if defined(__LP64__)
#define CL_LIBRARY_PATH "/usr/lib64/libaudiochargerlistener.so"
#else
#define CL_LIBRARY_PATH "/usr/lib/libaudiochargerlistener.so"
#endif
#else
#define CL_LIBRARY_PATH "libaudiochargerlistener.so"
#endif

#define MIXER_XML_BASE_STRING_NAME "mixer_paths"
#define RMNGR_XMLFILE_BASE_STRING_NAME "resourcemanager"

#ifdef SEC_AUDIO_BOOT_ON_ERR
#define MAX_DUMMY_CARD_LENGTH 256
#endif

#define MAX_RETRY_CNT 20
#define LOWLATENCY_PCM_DEVICE 15
#define DEEP_BUFFER_PCM_DEVICE 0
#define DEVICE_NAME_MAX_SIZE 128

#define SND_CARD_VIRTUAL 100
#define SND_CARD_HW      0        // This will be used to intialize the sound card,
                                  // actual will be updated during init_audio

#define DEFAULT_BIT_WIDTH 16
#define DEFAULT_SAMPLE_RATE 48000
#define DEFAULT_CHANNELS 2
#define DEFAULT_FORMAT 0x00000000u
// TODO: double check and confirm actual
// values for max sessions number
#define MAX_SESSIONS_LOW_LATENCY 8
#define MAX_SESSIONS_ULTRA_LOW_LATENCY 8
#define MAX_SESSIONS_DEEP_BUFFER 3
#define MAX_SESSIONS_COMPRESSED 10
#define MAX_SESSIONS_GENERIC 2
#define MAX_SESSIONS_PCM_OFFLOAD 2
#define MAX_SESSIONS_VOICE_UI 8
#define MAX_SESSIONS_ASR 1
#define MAX_SESSIONS_RAW 1
#define MAX_SESSIONS_ACD 8
#define MAX_SESSIONS_PROXY 8
#define DEFAULT_MAX_SESSIONS 8
#define MAX_SESSIONS_INCALL_MUSIC 2
#define MAX_SESSIONS_INCALL_RECORD 2
#define MAX_SESSIONS_NON_TUNNEL 4
#define MAX_SESSIONS_HAPTICS 2
#define MAX_SESSIONS_ULTRASOUND 1
#define MAX_SESSIONS_VOICE_RECOGNITION 1
#define MAX_SESSIONS_SPATIAL_AUDIO 1
#define MAX_SESSIONS_SENSOR_PLAYBACK 1

#define WAKE_LOCK_NAME "audio_pal_wl"
#define WAKE_LOCK_PATH "/sys/power/wake_lock"
#define WAKE_UNLOCK_PATH "/sys/power/wake_unlock"
#define MAX_WAKE_LOCK_LENGTH 1024

#define CLOCK_SRC_DEFAULT 1

#define WAIT_LL_PB 4
#define WAIT_RECOVER_FET 150000

#define NLPI_LPI_SWITCH_DELAY_SEC 5
#define NLPI_LPI_SWITCH_SLEEP_INTERVAL_SEC 1

/*this can be over written by the config file settings*/
uint32_t pal_log_lvl = (PAL_LOG_ERR|PAL_LOG_INFO);

static struct str_parms *configParamKVPairs;

char rmngr_xml_file[XML_PATH_MAX_LENGTH] = {0};
char rmngr_xml_file_wo_variant[XML_PATH_MAX_LENGTH] = {0};

char vendor_config_path[VENDOR_CONFIG_PATH_MAX_LENGTH] = {0};

const std::vector<int> gSignalsOfInterest = {
    DEBUGGER_SIGNAL,
    SIGABRT,
    SIGSEGV,
};

// default properties which will be updated based on platform configuration
static struct pal_st_properties qst_properties = {
        "QUALCOMM Technologies, Inc",  // implementor
        "Sound Trigger HAL",  // description
        1,  // version
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
        { 0xa1757552, 0xaf10, 0x11e7, 0x985a,
         { 0xa3, 0x75, 0x40, 0x10, 0xba, 0xa2 } }, // samsung uuid
#else
        { 0x68ab2d40, 0xe860, 0x11e3, 0x95ef,
         { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } },  // uuid
#endif
        8,  // max_sound_models
        10,  // max_key_phrases
        10,  // max_users
        PAL_RECOGNITION_MODE_VOICE_TRIGGER |
        PAL_RECOGNITION_MODE_GENERIC_TRIGGER,  // recognition_modes
        true,  // capture_transition
        0,  // max_capture_ms
        false,  // concurrent_capture
        false,  // trigger_in_event
        0  // power_consumption_mw
};

/*
pcm device id is directly related to device,
using legacy design for alsa
*/
// Will update actual value when numbers got for VT

std::vector<std::pair<int32_t, std::string>> ResourceManager::deviceLinkName {
    {PAL_DEVICE_OUT_MIN,                  {std::string{ "none" }}},
    {PAL_DEVICE_NONE,                     {std::string{ "none" }}},
    {PAL_DEVICE_OUT_HANDSET,              {std::string{ "" }}},
    {PAL_DEVICE_OUT_SPEAKER,              {std::string{ "" }}},
    {PAL_DEVICE_OUT_WIRED_HEADSET,        {std::string{ "" }}},
    {PAL_DEVICE_OUT_WIRED_HEADPHONE,      {std::string{ "" }}},
    {PAL_DEVICE_OUT_LINE,                 {std::string{ "" }}},
    {PAL_DEVICE_OUT_BLUETOOTH_SCO,        {std::string{ "" }}},
    {PAL_DEVICE_OUT_BLUETOOTH_A2DP,       {std::string{ "" }}},
    {PAL_DEVICE_OUT_BLUETOOTH_BLE,        {std::string{ "" }}},
    {PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST,{std::string{ "" }}},
    {PAL_DEVICE_OUT_AUX_DIGITAL,          {std::string{ "" }}},
    {PAL_DEVICE_OUT_HDMI,                 {std::string{ "" }}},
    {PAL_DEVICE_OUT_USB_DEVICE,           {std::string{ "" }}},
    {PAL_DEVICE_OUT_USB_HEADSET,          {std::string{ "" }}},
    {PAL_DEVICE_OUT_SPDIF,                {std::string{ "" }}},
    {PAL_DEVICE_OUT_FM,                   {std::string{ "" }}},
    {PAL_DEVICE_OUT_AUX_LINE,             {std::string{ "" }}},
    {PAL_DEVICE_OUT_PROXY,                {std::string{ "" }}},
    {PAL_DEVICE_OUT_RECORD_PROXY,         {std::string{ "" }}},
    {PAL_DEVICE_OUT_AUX_DIGITAL_1,        {std::string{ "" }}},
    {PAL_DEVICE_OUT_HEARING_AID,          {std::string{ "" }}},
    {PAL_DEVICE_OUT_HAPTICS_DEVICE,       {std::string{ "" }}},
    {PAL_DEVICE_OUT_ULTRASOUND,           {std::string{ "" }}},
    {PAL_DEVICE_OUT_ULTRASOUND_DEDICATED, {std::string{ "" }}},
    {PAL_DEVICE_OUT_DUMMY,                {std::string{ "" }}},
#ifdef SEC_AUDIO_CALL_SATELLITE
    {PAL_DEVICE_OUT_EXT_MODEM,            {std::string{ "" }}},
#endif
    {PAL_DEVICE_OUT_MAX,                  {std::string{ "none" }}},

    {PAL_DEVICE_IN_HANDSET_MIC,           {std::string{ "tdm-pri" }}},
    {PAL_DEVICE_IN_SPEAKER_MIC,           {std::string{ "tdm-pri" }}},
    {PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET, {std::string{ "" }}},
    {PAL_DEVICE_IN_WIRED_HEADSET,         {std::string{ "" }}},
    {PAL_DEVICE_IN_AUX_DIGITAL,           {std::string{ "" }}},
    {PAL_DEVICE_IN_HDMI,                  {std::string{ "" }}},
    {PAL_DEVICE_IN_USB_ACCESSORY,         {std::string{ "" }}},
    {PAL_DEVICE_IN_USB_DEVICE,            {std::string{ "" }}},
    {PAL_DEVICE_IN_USB_HEADSET,           {std::string{ "" }}},
    {PAL_DEVICE_IN_FM_TUNER,              {std::string{ "" }}},
    {PAL_DEVICE_IN_LINE,                  {std::string{ "" }}},
    {PAL_DEVICE_IN_SPDIF,                 {std::string{ "" }}},
    {PAL_DEVICE_IN_PROXY,                 {std::string{ "" }}},
    {PAL_DEVICE_IN_HANDSET_VA_MIC,        {std::string{ "" }}},
    {PAL_DEVICE_IN_BLUETOOTH_A2DP,        {std::string{ "" }}},
    {PAL_DEVICE_IN_HEADSET_VA_MIC,        {std::string{ "" }}},
    {PAL_DEVICE_IN_VI_FEEDBACK,           {std::string{ "" }}},
    {PAL_DEVICE_IN_TELEPHONY_RX,          {std::string{ "" }}},
    {PAL_DEVICE_IN_ULTRASOUND_MIC,        {std::string{ "" }}},
    {PAL_DEVICE_IN_EXT_EC_REF,            {std::string{ "none" }}},
    {PAL_DEVICE_IN_ECHO_REF,              {std::string{ "" }}},
    {PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK,   {std::string{ "" }}},
    {PAL_DEVICE_IN_BLUETOOTH_BLE,         {std::string{ "" }}},
    {PAL_DEVICE_IN_CPS_FEEDBACK,          {std::string{ "" }}},
    {PAL_DEVICE_IN_DUMMY,                 {std::string{ "" }}},
    {PAL_DEVICE_IN_CPS2_FEEDBACK,         {std::string{ "" }}},
    {PAL_DEVICE_IN_RECORD_PROXY,          {std::string{ "" }}},
#ifdef SEC_AUDIO_CALL_SATELLITE
    {PAL_DEVICE_IN_EXT_MODEM_MIC,         {std::string{ "" }}},
#endif
    {PAL_DEVICE_IN_MAX,                   {std::string{ "" }}},
};

std::vector<std::pair<int32_t, int32_t>> ResourceManager::devicePcmId {
    {PAL_DEVICE_OUT_MIN,                  0},
    {PAL_DEVICE_NONE,                     0},
    {PAL_DEVICE_OUT_HANDSET,              1},
    {PAL_DEVICE_OUT_SPEAKER,              1},
    {PAL_DEVICE_OUT_WIRED_HEADSET,        1},
    {PAL_DEVICE_OUT_WIRED_HEADPHONE,      1},
    {PAL_DEVICE_OUT_LINE,                 0},
    {PAL_DEVICE_OUT_BLUETOOTH_SCO,        0},
    {PAL_DEVICE_OUT_BLUETOOTH_A2DP,       0},
    {PAL_DEVICE_OUT_BLUETOOTH_BLE,        0},
    {PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST, 0},
    {PAL_DEVICE_OUT_AUX_DIGITAL,          0},
    {PAL_DEVICE_OUT_HDMI,                 0},
    {PAL_DEVICE_OUT_USB_DEVICE,           0},
    {PAL_DEVICE_OUT_USB_HEADSET,          0},
    {PAL_DEVICE_OUT_SPDIF,                0},
    {PAL_DEVICE_OUT_FM,                   0},
    {PAL_DEVICE_OUT_AUX_LINE,             0},
    {PAL_DEVICE_OUT_PROXY,                0},
    {PAL_DEVICE_OUT_RECORD_PROXY,         0},
    {PAL_DEVICE_OUT_AUX_DIGITAL_1,        0},
    {PAL_DEVICE_OUT_HEARING_AID,          0},
    {PAL_DEVICE_OUT_HAPTICS_DEVICE,       0},
    {PAL_DEVICE_OUT_ULTRASOUND,           1},
    {PAL_DEVICE_OUT_ULTRASOUND_DEDICATED, 1},
    {PAL_DEVICE_OUT_DUMMY,                0},
#ifdef SEC_AUDIO_CALL_SATELLITE
    {PAL_DEVICE_OUT_EXT_MODEM,            0},
#endif
    {PAL_DEVICE_OUT_MAX,                  0},

    {PAL_DEVICE_IN_HANDSET_MIC,           0},
    {PAL_DEVICE_IN_SPEAKER_MIC,           0},
    {PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET, 0},
    {PAL_DEVICE_IN_WIRED_HEADSET,         0},
    {PAL_DEVICE_IN_AUX_DIGITAL,           0},
    {PAL_DEVICE_IN_HDMI,                  0},
    {PAL_DEVICE_IN_USB_ACCESSORY,         0},
    {PAL_DEVICE_IN_USB_DEVICE,            0},
    {PAL_DEVICE_IN_USB_HEADSET,           0},
    {PAL_DEVICE_IN_FM_TUNER,              0},
    {PAL_DEVICE_IN_LINE,                  0},
    {PAL_DEVICE_IN_SPDIF,                 0},
    {PAL_DEVICE_IN_PROXY,                 0},
    {PAL_DEVICE_IN_HANDSET_VA_MIC,        0},
    {PAL_DEVICE_IN_BLUETOOTH_A2DP,        0},
    {PAL_DEVICE_IN_HEADSET_VA_MIC,        0},
    {PAL_DEVICE_IN_VI_FEEDBACK,           0},
    {PAL_DEVICE_IN_TELEPHONY_RX,          0},
    {PAL_DEVICE_IN_ULTRASOUND_MIC,        0},
    {PAL_DEVICE_IN_EXT_EC_REF,            0},
    {PAL_DEVICE_IN_ECHO_REF,              0},
    {PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK,   0},
    {PAL_DEVICE_IN_BLUETOOTH_BLE,         0},
    {PAL_DEVICE_IN_CPS_FEEDBACK,          0},
    {PAL_DEVICE_IN_DUMMY,                 0},
    {PAL_DEVICE_IN_CPS2_FEEDBACK,         0},
    {PAL_DEVICE_IN_RECORD_PROXY,          0},
#ifdef SEC_AUDIO_CALL_SATELLITE
    {PAL_DEVICE_IN_EXT_MODEM_MIC,         0},
#endif
    {PAL_DEVICE_IN_MAX,                   0},
};

// To be defined in detail
std::vector<std::pair<int32_t, std::string>> ResourceManager::sndDeviceNameLUT {
    {PAL_DEVICE_OUT_MIN,                  {std::string{ "" }}},
    {PAL_DEVICE_NONE,                     {std::string{ "none" }}},
    {PAL_DEVICE_OUT_HANDSET,              {std::string{ "" }}},
    {PAL_DEVICE_OUT_SPEAKER,              {std::string{ "" }}},
    {PAL_DEVICE_OUT_WIRED_HEADSET,        {std::string{ "" }}},
    {PAL_DEVICE_OUT_WIRED_HEADPHONE,      {std::string{ "" }}},
    {PAL_DEVICE_OUT_LINE,                 {std::string{ "" }}},
    {PAL_DEVICE_OUT_BLUETOOTH_SCO,        {std::string{ "" }}},
    {PAL_DEVICE_OUT_BLUETOOTH_A2DP,       {std::string{ "" }}},
    {PAL_DEVICE_OUT_BLUETOOTH_BLE,        {std::string{ "" }}},
    {PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST, {std::string{ "" }}},
    {PAL_DEVICE_OUT_AUX_DIGITAL,          {std::string{ "" }}},
    {PAL_DEVICE_OUT_HDMI,                 {std::string{ "" }}},
    {PAL_DEVICE_OUT_USB_DEVICE,           {std::string{ "" }}},
    {PAL_DEVICE_OUT_USB_HEADSET,          {std::string{ "" }}},
    {PAL_DEVICE_OUT_SPDIF,                {std::string{ "" }}},
    {PAL_DEVICE_OUT_FM,                   {std::string{ "" }}},
    {PAL_DEVICE_OUT_AUX_LINE,             {std::string{ "" }}},
    {PAL_DEVICE_OUT_PROXY,                {std::string{ "" }}},
    {PAL_DEVICE_OUT_RECORD_PROXY,         {std::string{ "" }}},
    {PAL_DEVICE_OUT_AUX_DIGITAL_1,        {std::string{ "" }}},
    {PAL_DEVICE_OUT_HEARING_AID,          {std::string{ "" }}},
    {PAL_DEVICE_OUT_HAPTICS_DEVICE,       {std::string{ "" }}},
    {PAL_DEVICE_OUT_ULTRASOUND,           {std::string{ "" }}},
    {PAL_DEVICE_OUT_ULTRASOUND_DEDICATED, {std::string{ "" }}},
    {PAL_DEVICE_OUT_DUMMY,                {std::string{ "" }}},
#ifdef SEC_AUDIO_CALL_SATELLITE
    {PAL_DEVICE_OUT_EXT_MODEM,            {std::string{ "" }}},
#endif
    {PAL_DEVICE_OUT_MAX,                  {std::string{ "" }}},

    {PAL_DEVICE_IN_HANDSET_MIC,           {std::string{ "" }}},
    {PAL_DEVICE_IN_SPEAKER_MIC,           {std::string{ "" }}},
    {PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET, {std::string{ "" }}},
    {PAL_DEVICE_IN_WIRED_HEADSET,         {std::string{ "" }}},
    {PAL_DEVICE_IN_AUX_DIGITAL,           {std::string{ "" }}},
    {PAL_DEVICE_IN_HDMI,                  {std::string{ "" }}},
    {PAL_DEVICE_IN_USB_ACCESSORY,         {std::string{ "" }}},
    {PAL_DEVICE_IN_USB_DEVICE,            {std::string{ "" }}},
    {PAL_DEVICE_IN_USB_HEADSET,           {std::string{ "" }}},
    {PAL_DEVICE_IN_FM_TUNER,              {std::string{ "" }}},
    {PAL_DEVICE_IN_LINE,                  {std::string{ "" }}},
    {PAL_DEVICE_IN_SPDIF,                 {std::string{ "" }}},
    {PAL_DEVICE_IN_PROXY,                 {std::string{ "" }}},
    {PAL_DEVICE_IN_HANDSET_VA_MIC,        {std::string{ "" }}},
    {PAL_DEVICE_IN_BLUETOOTH_A2DP,        {std::string{ "" }}},
    {PAL_DEVICE_IN_HEADSET_VA_MIC,        {std::string{ "" }}},
    {PAL_DEVICE_IN_VI_FEEDBACK,           {std::string{ "" }}},
    {PAL_DEVICE_IN_TELEPHONY_RX,          {std::string{ "" }}},
    {PAL_DEVICE_IN_ULTRASOUND_MIC,        {std::string{ "" }}},
    {PAL_DEVICE_IN_EXT_EC_REF,            {std::string{ "none" }}},
    {PAL_DEVICE_IN_ECHO_REF,              {std::string{ "" }}},
    {PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK,   {std::string{ "" }}},
    {PAL_DEVICE_IN_BLUETOOTH_BLE,         {std::string{ "" }}},
    {PAL_DEVICE_IN_CPS_FEEDBACK,          {std::string{ "" }}},
    {PAL_DEVICE_IN_DUMMY,                 {std::string{ "" }}},
    {PAL_DEVICE_IN_CPS2_FEEDBACK,         {std::string{ "" }}},
    {PAL_DEVICE_IN_RECORD_PROXY,          {std::string{ "" }}},
#ifdef SEC_AUDIO_CALL_SATELLITE
    {PAL_DEVICE_IN_EXT_MODEM_MIC,         {std::string{ "" }}},
#endif
    {PAL_DEVICE_IN_MAX,                   {std::string{ "" }}},
};

#ifdef SEC_AUDIO_COMMON
const std::map<uint32_t, uint32_t> streamPriorityLUT {
    {PAL_STREAM_LOW_LATENCY,        4},
    {PAL_STREAM_DEEP_BUFFER,        4},
    {PAL_STREAM_COMPRESSED,         4},
    {PAL_STREAM_VOIP,               2},
    {PAL_STREAM_VOIP_RX,            2},
    {PAL_STREAM_VOIP_TX,            2},
    {PAL_STREAM_VOICE_CALL_MUSIC,   2},
    {PAL_STREAM_GENERIC,            4},
    {PAL_STREAM_RAW,                4},
    {PAL_STREAM_VOICE_RECOGNITION,  4},
    {PAL_STREAM_VOICE_CALL_RECORD,  2},
    {PAL_STREAM_VOICE_CALL_TX,      1},
    {PAL_STREAM_VOICE_CALL_RX_TX,   1},
    {PAL_STREAM_VOICE_CALL,         1},
    {PAL_STREAM_LOOPBACK,           3},
    {PAL_STREAM_TRANSCODE,          4},
    {PAL_STREAM_VOICE_UI,           5},
    {PAL_STREAM_PCM_OFFLOAD,        4},
    {PAL_STREAM_ULTRA_LOW_LATENCY,  4},
    {PAL_STREAM_PROXY,              4},
    {PAL_STREAM_NON_TUNNEL,         4},
    {PAL_STREAM_HAPTICS,            4},
    {PAL_STREAM_ACD,                4},
    {PAL_STREAM_CONTEXT_PROXY,      4},
    {PAL_STREAM_SENSOR_PCM_DATA,    4},
    {PAL_STREAM_ULTRASOUND,         5},
    {PAL_STREAM_SPATIAL_AUDIO,      4},
    {PAL_STREAM_SENSOR_PCM_RENDERER,5},
};
#else
const std::map<uint32_t, uint32_t> streamPriorityLUT {
    {PAL_STREAM_LOW_LATENCY,        3},
    {PAL_STREAM_DEEP_BUFFER,        3},
    {PAL_STREAM_COMPRESSED,         3},
    {PAL_STREAM_VOIP,               2},
    {PAL_STREAM_VOIP_RX,            2},
    {PAL_STREAM_VOIP_TX,            2},
    {PAL_STREAM_VOICE_CALL_MUSIC,   2},
    {PAL_STREAM_GENERIC,            3},
    {PAL_STREAM_RAW,                3},
    {PAL_STREAM_VOICE_RECOGNITION,  3},
    {PAL_STREAM_VOICE_CALL_RECORD,  2},
    {PAL_STREAM_VOICE_CALL_TX,      1},
    {PAL_STREAM_VOICE_CALL_RX_TX,   1},
    {PAL_STREAM_VOICE_CALL,         1},
    {PAL_STREAM_LOOPBACK,           3},
    {PAL_STREAM_TRANSCODE,          3},
    {PAL_STREAM_VOICE_UI,           4},
    {PAL_STREAM_PCM_OFFLOAD,        3},
    {PAL_STREAM_ULTRA_LOW_LATENCY,  3},
    {PAL_STREAM_PROXY,              3},
    {PAL_STREAM_NON_TUNNEL,         3},
    {PAL_STREAM_HAPTICS,            3},
    {PAL_STREAM_ACD,                3},
    {PAL_STREAM_ASR,                4},
    {PAL_STREAM_CONTEXT_PROXY,      3},
    {PAL_STREAM_SENSOR_PCM_DATA,    3},
    {PAL_STREAM_ULTRASOUND,         4},
    {PAL_STREAM_SPATIAL_AUDIO,      3},
    {PAL_STREAM_SENSOR_PCM_RENDERER,4},
};
#endif

const std::map<std::string, sidetone_mode_t> sidetoneModetoId {
    {std::string{ "OFF" }, SIDETONE_OFF},
    {std::string{ "HW" },  SIDETONE_HW},
    {std::string{ "SW" },  SIDETONE_SW},
};

const std::map<uint32_t, pal_audio_fmt_t> bitWidthToFormat {
    {BITWIDTH_16, PAL_AUDIO_FMT_PCM_S16_LE},
    {BITWIDTH_24, PAL_AUDIO_FMT_PCM_S24_LE},
    {BITWIDTH_32, PAL_AUDIO_FMT_PCM_S32_LE},
};

bool isPalPCMFormat(uint32_t fmt_id)
{
    switch (fmt_id) {
        case PAL_AUDIO_FMT_PCM_S32_LE:
        case PAL_AUDIO_FMT_PCM_S8:
        case PAL_AUDIO_FMT_PCM_S24_3LE:
        case PAL_AUDIO_FMT_PCM_S24_LE:
        case PAL_AUDIO_FMT_PCM_S16_LE:
            return true;
        default:
            return false;
    }
}

bool ResourceManager::isBitWidthSupported(uint32_t bitWidth)
{
    bool rc = false;
    PAL_VERBOSE(LOG_TAG, "bitWidth %u", bitWidth);
    switch (bitWidth) {
        case BITWIDTH_16:
        case BITWIDTH_24:
        case BITWIDTH_32:
            rc = true;
            break;
        default:
            PAL_ERR(LOG_TAG, "bit width not supported %d rc %d", bitWidth, rc);
            break;
    }
    return rc;
}

std::shared_ptr<ResourceManager> ResourceManager::rm = nullptr;
std::vector <int> ResourceManager::streamTag = {0};
std::vector <int> ResourceManager::streamPpTag = {0};
std::vector <int> ResourceManager::mixerTag = {0};
std::vector <int> ResourceManager::devicePpTag = {0};
std::vector <int> ResourceManager::deviceTag = {0};
std::mutex ResourceManager::mResourceManagerMutex;
std::mutex ResourceManager::mChargerBoostMutex;
std::mutex ResourceManager::mGraphMutex;
std::mutex ResourceManager::mActiveStreamMutex;
std::mutex ResourceManager::mSleepMonitorMutex;
std::mutex ResourceManager::mListFrontEndsMutex;
std::vector <int> ResourceManager::listAllFrontEndIds = {0};
std::vector <int> ResourceManager::listFreeFrontEndIds = {0};
std::vector <int> ResourceManager::listAllPcmPlaybackFrontEnds = {0};
std::vector <int> ResourceManager::listAllPcmRecordFrontEnds = {0};
std::vector <int> ResourceManager::listAllPcmHostlessRxFrontEnds = {0};
std::vector <int> ResourceManager::listAllPcmHostlessTxFrontEnds = {0};
std::vector <int> ResourceManager::listAllPcmExtEcTxFrontEnds = {0};
std::vector <int> ResourceManager::listAllCompressPlaybackFrontEnds = {0};
std::vector <int> ResourceManager::listAllCompressRecordFrontEnds = {0};
std::vector <int> ResourceManager::listAllPcmVoice1RxFrontEnds = {0};
std::vector <int> ResourceManager::listAllPcmVoice1TxFrontEnds = {0};
std::vector <int> ResourceManager::listAllPcmVoice2RxFrontEnds = {0};
std::vector <int> ResourceManager::listAllPcmVoice2TxFrontEnds = {0};
std::vector <int> ResourceManager::listAllPcmInCallRecordFrontEnds = {0};
std::vector <int> ResourceManager::listAllPcmInCallMusicFrontEnds = {0};
std::vector <int> ResourceManager::listAllNonTunnelSessionIds = {0};
std::vector <int> ResourceManager::listAllPcmContextProxyFrontEnds = {0};
std::vector <std::string> ResourceManager::usb_vendor_uuid_list = {""};
struct audio_mixer* ResourceManager::audio_virt_mixer = NULL;
struct audio_mixer* ResourceManager::audio_hw_mixer = NULL;
struct audio_route* ResourceManager::audio_route = NULL;
int ResourceManager::snd_virt_card = SND_CARD_VIRTUAL;
int ResourceManager::snd_hw_card = SND_CARD_HW;
std::vector<deviceCap> ResourceManager::devInfo;
static struct nativeAudioProp na_props;
static bool isHifiFilterEnabled = false;
SndCardMonitor* ResourceManager::sndmon = NULL;
void* ResourceManager::cl_lib_handle = NULL;
cl_init_t ResourceManager::cl_init = NULL;
cl_deinit_t ResourceManager::cl_deinit = NULL;
cl_set_boost_state_t ResourceManager::cl_set_boost_state = NULL;

void* ResourceManager::vui_dmgr_lib_handle = NULL;
vui_dmgr_init_t ResourceManager::vui_dmgr_init = NULL;
vui_dmgr_deinit_t ResourceManager::vui_dmgr_deinit = NULL;

void* ResourceManager::feature_stats_handle = NULL;
afs_init_t ResourceManager::feature_stats_init = NULL;
afs_deinit_t ResourceManager::feature_stats_deinit = NULL;

std::mutex ResourceManager::cvMutex;
std::queue<card_status_t> ResourceManager::msgQ;
std::condition_variable ResourceManager::cv;
std::thread ResourceManager::workerThread;
std::thread ResourceManager::mixerEventTread;
bool ResourceManager::mixerClosed = false;
int ResourceManager::mixerEventRegisterCount = 0;
int ResourceManager::TxconcurrencyEnableCount = 0;
int ResourceManager::concurrencyEnableCount = 0;
int ResourceManager::concurrencyDisableCount = 0;
int ResourceManager::ACDConcurrencyEnableCount = 0;
int ResourceManager::ACDConcurrencyDisableCount = 0;
int ResourceManager::ASRConcurrencyEnableCount = 0;
int ResourceManager::ASRConcurrencyDisableCount = 0;
int ResourceManager::SNSPCMDataConcurrencyEnableCount = 0;
int ResourceManager::SNSPCMDataConcurrencyDisableCount = 0;
defer_switch_state_t ResourceManager::deferredSwitchState = NO_DEFER;
std::thread ResourceManager::vui_deferred_switch_thread_;
std::condition_variable ResourceManager::vui_switch_cv_;
std::mutex ResourceManager::vui_switch_mutex_;
bool ResourceManager::vui_switch_thread_exit_ = false;
int ResourceManager::deferred_switch_cnt_ = -1;
int ResourceManager::wake_lock_fd = -1;
int ResourceManager::wake_unlock_fd = -1;
uint32_t ResourceManager::wake_lock_cnt = 0;
static int max_session_num;
bool ResourceManager::isQmpEnabled = false;
bool ResourceManager::isSpeakerProtectionEnabled = false;
bool ResourceManager::isHandsetProtectionEnabled = false;
bool ResourceManager::isHapticsProtectionEnabled = false;
bool ResourceManager::isChargeConcurrencyEnabled = false;
int ResourceManager::cpsMode = 0;
bool ResourceManager::isVbatEnabled = false;
static int max_nt_sessions;
bool ResourceManager::isRasEnabled = false;
bool ResourceManager::isMainSpeakerRight;
int ResourceManager::spQuickCalTime;
bool ResourceManager::isGaplessEnabled = false;
bool ResourceManager::isDualMonoEnabled = false;
#ifdef SEC_AUDIO_SUPPORT_UHQ
pal_uhqa_state ResourceManager::stateUHQA = PAL_UHQ_STATE_NORMAL;
#else
bool ResourceManager::isUHQAEnabled = false;
#endif
bool ResourceManager::isContextManagerEnabled = false;
bool ResourceManager::isVIRecordStarted;
bool ResourceManager::lpi_logging_ = false;
bool ResourceManager::isUpdDedicatedBeEnabled = false;
bool ResourceManager::isDeviceMuxConfigEnabled = false;
bool ResourceManager::isUpdDutyCycleEnabled = false;
bool ResourceManager::isUPDVirtualPortEnabled = false;
bool ResourceManager::isCPEnabled = false;
bool ResourceManager::isDummyDevEnabled = false;
bool ResourceManager::isProxyRecordActive = false;
pal_audio_event_callback ResourceManager::callback_event = nullptr;
bool ResourceManager::isSilenceDetectionEnabledPcm = false;
bool ResourceManager::isSilenceDetectionEnabledVoice = false;
uint32_t ResourceManager::silenceDetectionDuration = 3000;
int ResourceManager::max_voice_vol = -1;     /* Variable to store max volume index for voice call */
bool ResourceManager::isSignalHandlerEnabled = false;
static int haptics_priority;
bool ResourceManager::isHapticsthroughWSA = false;
bool ResourceManager::isCRSCallEnabled = false;
#ifdef SOC_PERIPHERAL_PROT
std::thread ResourceManager::socPerithread;
bool ResourceManager::isTZSecureZone = false;
void * ResourceManager::tz_handle = NULL;
void * ResourceManager::socPeripheralLibHdl = NULL;
getPeripheralStatusFnPtr ResourceManager::mGetPeripheralState = nullptr;
registerPeripheralCBFnPtr ResourceManager::mRegisterPeripheralCb = nullptr;
deregisterPeripheralCBFnPtr ResourceManager::mDeregisterPeripheralCb = nullptr;
#define PRPHRL_REGSTR_RETRY_COUNT 10
#endif
#ifdef SEC_AUDIO_BARGEIN_MODE
int ResourceManager::bargein_mode = 0;
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
int ResourceManager::interpreter_mode = 0;
#endif
#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO
bool ResourceManager::isBtmixRecord = false;
#endif
//TODO:Needs to define below APIs so that functionality won't break
#ifdef FEATURE_IPQ_OPENWRT
int str_parms_get_str(struct str_parms *str_parms, const char *key,
                      char *out_val, int len){return 0;}
char *str_parms_to_str(struct str_parms *str_parms){return NULL;}
int str_parms_add_str(struct str_parms *str_parms, const char *key,
                      const char *value){return 0;}
struct str_parms *str_parms_create(void){return NULL;}
void str_parms_del(struct str_parms *str_parms, const char *key){return;}
void str_parms_destroy(struct str_parms *str_parms){return;}

#endif

std::vector<vote_type_t> ResourceManager::sleep_monitor_vote_type_(PAL_STREAM_MAX, NLPI_VOTE);
std::vector<deviceIn> ResourceManager::deviceInfo;
std::vector<tx_ecinfo> ResourceManager::txEcInfo;
std::vector <uint32_t> sndCardStandbySupportedStreams_;
struct vsid_info ResourceManager::vsidInfo;
struct volume_set_param_info ResourceManager::volumeSetParamInfo_;
struct disable_lpm_info ResourceManager::disableLpmInfo_;
std::vector<struct pal_amp_db_and_gain_table> ResourceManager::gainLvlMap;
std::map<std::pair<uint32_t, std::string>, std::string> ResourceManager::btCodecMap;
std::map<int, std::string> ResourceManager::spkrTempCtrlsMap;
std::map<uint32_t, uint32_t> ResourceManager::btSlimClockSrcMap;

std::shared_ptr<group_dev_config_t> ResourceManager::activeGroupDevConfig = nullptr;
group_dev_config_t ResourceManager::currentGroupDevConfig = {};
std::map<group_dev_config_idx_t, std::shared_ptr<group_dev_config_t>> ResourceManager::groupDevConfigMap;
std::vector<int> ResourceManager::spViChannelMapCfg = {};

#define MAKE_STRING_FROM_ENUM(string) { {#string}, string }
std::map<std::string, uint32_t> ResourceManager::btFmtTable = {
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_AAC),
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_SBC),
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_APTX),
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_APTX_HD),
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_APTX_DUAL_MONO),
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_LDAC),
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_CELT),
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_APTX_AD),
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_APTX_AD_SPEECH),
// SS_BT_HFP - H_127 : RVP
#ifdef SEC_AUDIO_BLUETOOTH
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_RVP),
#endif
// SS_BT_HFP - H_127 end
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_LC3),
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_PCM),
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_SSC),
#else
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_PCM),
#endif
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_APTX_AD_QLEA),
    MAKE_STRING_FROM_ENUM(CODEC_TYPE_APTX_AD_R4)
};

std::map<std::string, int> ResourceManager::spkrPosTable = {
    MAKE_STRING_FROM_ENUM(SPKR_RIGHT),
    MAKE_STRING_FROM_ENUM(SPKR_LEFT)
};

std::vector<std::pair<int32_t, std::string>> ResourceManager::listAllBackEndIds {
    {PAL_DEVICE_OUT_MIN,                  {std::string{ "" }}},
    {PAL_DEVICE_NONE,                     {std::string{ "" }}},
    {PAL_DEVICE_OUT_HANDSET,              {std::string{ "" }}},
    {PAL_DEVICE_OUT_SPEAKER,              {std::string{ "none" }}},
    {PAL_DEVICE_OUT_WIRED_HEADSET,        {std::string{ "" }}},
    {PAL_DEVICE_OUT_WIRED_HEADPHONE,      {std::string{ "" }}},
    {PAL_DEVICE_OUT_LINE,                 {std::string{ "" }}},
    {PAL_DEVICE_OUT_BLUETOOTH_SCO,        {std::string{ "" }}},
    {PAL_DEVICE_OUT_BLUETOOTH_A2DP,       {std::string{ "" }}},
    {PAL_DEVICE_OUT_BLUETOOTH_BLE,        {std::string{ "" }}},
    {PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST, {std::string{ "" }}},
    {PAL_DEVICE_OUT_AUX_DIGITAL,          {std::string{ "" }}},
    {PAL_DEVICE_OUT_HDMI,                 {std::string{ "" }}},
    {PAL_DEVICE_OUT_USB_DEVICE,           {std::string{ "" }}},
    {PAL_DEVICE_OUT_USB_HEADSET,          {std::string{ "" }}},
    {PAL_DEVICE_OUT_SPDIF,                {std::string{ "" }}},
    {PAL_DEVICE_OUT_FM,                   {std::string{ "" }}},
    {PAL_DEVICE_OUT_AUX_LINE,             {std::string{ "" }}},
    {PAL_DEVICE_OUT_PROXY,                {std::string{ "" }}},
    {PAL_DEVICE_OUT_RECORD_PROXY,         {std::string{ "" }}},
    {PAL_DEVICE_OUT_AUX_DIGITAL_1,        {std::string{ "" }}},
    {PAL_DEVICE_OUT_HEARING_AID,          {std::string{ "" }}},
    {PAL_DEVICE_OUT_HAPTICS_DEVICE,       {std::string{ "" }}},
    {PAL_DEVICE_OUT_ULTRASOUND,           {std::string{ "" }}},
    {PAL_DEVICE_OUT_ULTRASOUND_DEDICATED, {std::string{ "" }}},
    {PAL_DEVICE_OUT_DUMMY,                {std::string{ "" }}},
#ifdef SEC_AUDIO_CALL_SATELLITE
    {PAL_DEVICE_OUT_EXT_MODEM,            {std::string{ "" }}},
#endif
    {PAL_DEVICE_OUT_MAX,                  {std::string{ "" }}},

    {PAL_DEVICE_IN_HANDSET_MIC,           {std::string{ "none" }}},
    {PAL_DEVICE_IN_SPEAKER_MIC,           {std::string{ "none" }}},
    {PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET, {std::string{ "" }}},
    {PAL_DEVICE_IN_WIRED_HEADSET,         {std::string{ "" }}},
    {PAL_DEVICE_IN_AUX_DIGITAL,           {std::string{ "" }}},
    {PAL_DEVICE_IN_HDMI,                  {std::string{ "" }}},
    {PAL_DEVICE_IN_USB_ACCESSORY,         {std::string{ "" }}},
    {PAL_DEVICE_IN_USB_DEVICE,            {std::string{ "" }}},
    {PAL_DEVICE_IN_USB_HEADSET,           {std::string{ "" }}},
    {PAL_DEVICE_IN_FM_TUNER,              {std::string{ "" }}},
    {PAL_DEVICE_IN_LINE,                  {std::string{ "" }}},
    {PAL_DEVICE_IN_SPDIF,                 {std::string{ "" }}},
    {PAL_DEVICE_IN_PROXY,                 {std::string{ "" }}},
    {PAL_DEVICE_IN_HANDSET_VA_MIC,        {std::string{ "none" }}},
    {PAL_DEVICE_IN_BLUETOOTH_A2DP,        {std::string{ "" }}},
    {PAL_DEVICE_IN_HEADSET_VA_MIC,        {std::string{ "none" }}},
    {PAL_DEVICE_IN_VI_FEEDBACK,           {std::string{ "" }}},
    {PAL_DEVICE_IN_TELEPHONY_RX,          {std::string{ "" }}},
    {PAL_DEVICE_IN_ULTRASOUND_MIC,        {std::string{ "none" }}},
    {PAL_DEVICE_IN_EXT_EC_REF,            {std::string{ "none" }}},
    {PAL_DEVICE_IN_ECHO_REF,              {std::string{ "" }}},
    {PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK,   {std::string{ "" }}},
    {PAL_DEVICE_IN_BLUETOOTH_BLE,         {std::string{ "" }}},
    {PAL_DEVICE_IN_CPS_FEEDBACK,          {std::string{ "" }}},
    {PAL_DEVICE_IN_DUMMY,                 {std::string{ "" }}},
    {PAL_DEVICE_IN_CPS2_FEEDBACK,         {std::string{ "" }}},
    {PAL_DEVICE_IN_RECORD_PROXY,          {std::string{ "" }}},
#ifdef SEC_AUDIO_CALL_SATELLITE
    {PAL_DEVICE_IN_EXT_MODEM_MIC,         {std::string{ "" }}},
#endif
    {PAL_DEVICE_IN_MAX,                   {std::string{ "" }}},
};

void agmServiceCrashHandler(uint64_t cookie __unused)
{
    PAL_ERR(LOG_TAG,"AGM service crashed :( ");
    _exit(1);
}

pal_device_id_t ResourceManager::getDeviceId(std::string device_name)
{
   pal_device_id_t type =  (pal_device_id_t )deviceIdLUT.at(device_name);
   return type;
}

pal_stream_type_t ResourceManager::getStreamType(std::string stream_name)
{
    pal_stream_type_t type = (pal_stream_type_t )usecaseIdLUT.at(stream_name);
    return type;
}

#ifdef SOC_PERIPHERAL_PROT
int32_t ResourceManager::secureZoneEventCb(const uint32_t peripheral,
                                           const uint8_t secureState) {
    struct mixer_ctl *ctl;
    int ret = 0;

    PAL_INFO(LOG_TAG,"Received Notification from TZ... secureState: %d", secureState);

    ctl = mixer_get_ctl_by_name(audio_hw_mixer, "VOTE Against Sleep");
    if (!ctl) {
       PAL_ERR(LOG_TAG, "Invalid mixer control: VOTE Against Sleep");
       return -ENOENT;
    }

    switch (secureState) {
        case STATE_SECURE:
            ResourceManager::isTZSecureZone = true;
            PAL_DBG(LOG_TAG, "Enter Secure zone successfully, vote for LPASS core");
            ret = mixer_ctl_set_enum_by_string(ctl, "Enable");
            if (ret)
                PAL_ERR(LOG_TAG, "Could not Enable ctl for mixer cmd - %s ret %d\n",
                        "VOTE Against Sleep", ret);
            break;
        case STATE_POST_CHANGE:
            PAL_DBG(LOG_TAG, "Entered Secure zone successfully, unvote for LPASS core");
            ret = mixer_ctl_set_enum_by_string(ctl, "Disable");
            if (ret)
                PAL_ERR(LOG_TAG, "Could not Disable ctl for mixer cmd - %s ret %d\n",
                        "VOTE Against Sleep", ret);
            break;
        case STATE_PRE_CHANGE:
            PAL_DBG(LOG_TAG, "Before the exit from secure zone, vote for LPASS core");
            ret = mixer_ctl_set_enum_by_string(ctl, "Enable");
            if (ret)
                PAL_ERR(LOG_TAG, "Could not Enable ctl for mixer cmd - %s ret %d\n",
                        "VOTE Against Sleep", ret);
            break;
        case STATE_NONSECURE:
            ResourceManager::isTZSecureZone = false;
            PAL_DBG(LOG_TAG, "Exited Secure zone successfully, unvote for LPASS core");
            ret = mixer_ctl_set_enum_by_string(ctl, "Disable");
            if (ret)
                PAL_ERR(LOG_TAG, "Could not Disable ctl for mixer cmd - %s ret %d\n",
                        "VOTE Against Sleep", ret);
            break;
        case STATE_RESET_CONNECTION:
            /* Handling the state where connection got broken to get
                state change notification */
            PAL_INFO(LOG_TAG, "ssgtzd link got broken..re-registering to TZ");
            ret = registertoPeripheral(CPeripheralAccessControl_AUDIO_UID);
            break;
        default :
            PAL_ERR(LOG_TAG, "Invalid secureState = %d", secureState);
            return -EINVAL;
    }
    return ret;
}
#endif

uint32_t ResourceManager::getNTPathForStreamAttr(
                              const pal_stream_attributes &attr)
{
    uint32_t streamInputFormat = attr.out_media_config.aud_fmt_id;
    if (streamInputFormat == PAL_AUDIO_FMT_PCM_S16_LE ||
            streamInputFormat == PAL_AUDIO_FMT_PCM_S8 ||
            streamInputFormat == PAL_AUDIO_FMT_PCM_S24_3LE ||
            streamInputFormat == PAL_AUDIO_FMT_PCM_S24_LE ||
            streamInputFormat == PAL_AUDIO_FMT_PCM_S32_LE) {
        return NT_PATH_ENCODE;
    }
    return NT_PATH_DECODE;
}

ssize_t ResourceManager::getAvailableNTStreamInstance(
                              const pal_stream_attributes &attr)
{
    uint32_t pathIdx = getNTPathForStreamAttr(attr);
    auto NTStreamInstancesMap = mNTStreamInstancesList[pathIdx];
    for (int inst = INSTANCE_1; inst <= max_nt_sessions; ++inst) {
         auto it = NTStreamInstancesMap->find(inst);
         if (it == NTStreamInstancesMap->end()) {
             NTStreamInstancesMap->emplace(inst, true);
             return inst;
         } else if (!NTStreamInstancesMap->at(inst)) {
             NTStreamInstancesMap->at(inst) = true;
             return inst;
         } else {
             PAL_DBG(LOG_TAG, "Path %d, instanceId %d is in use", pathIdx, inst);
         }
     }
     return -EINVAL;
}

void ResourceManager::getFileNameExtn(const char *in_snd_card_name, char* file_name_extn,
                                      char* file_name_extn_wo_variant)
{
    /* Sound card name follows below mentioned convention:
       <target name>-<form factor>-<variant>-snd-card.
    */
    char *snd_card_name = NULL;
    char *tmp = NULL;
    char *card_sub_str = NULL;

    snd_card_name = strdup(in_snd_card_name);
    if (snd_card_name == NULL) {
        goto err;
    }

    card_sub_str = strtok_r(snd_card_name, "-", &tmp);
    if (card_sub_str == NULL) {
        PAL_ERR(LOG_TAG,"called on invalid snd card name");
        goto err;
    }
    strlcat(file_name_extn, card_sub_str, XML_PATH_EXTN_MAX_SIZE);

    while ((card_sub_str = strtok_r(NULL, "-", &tmp))) {
        if (strncmp(card_sub_str, "snd", strlen("snd"))) {
            strlcpy(file_name_extn_wo_variant, file_name_extn, XML_PATH_EXTN_MAX_SIZE);
            strlcat(file_name_extn, XML_FILE_DELIMITER, XML_PATH_EXTN_MAX_SIZE);
            strlcat(file_name_extn, card_sub_str, XML_PATH_EXTN_MAX_SIZE);
        }
        else
            break;
    }
    PAL_DBG(LOG_TAG,"file path extension(%s)", file_name_extn);
    PAL_DBG(LOG_TAG,"file path extension without variant(%s)", file_name_extn_wo_variant);

err:
    if (snd_card_name)
        free(snd_card_name);
}

void ResourceManager::sendCrashSignal(int signal, pid_t pid, uid_t uid)
{
    PAL_DBG(LOG_TAG, "%s: signal %d, pid %u, uid %u", __func__, signal, pid, uid);
    int32_t ret = memLoggerDumpAllToFile();
    if (ret)
    {
        PAL_ERR(LOG_TAG, "Error in dumping queues: %d", ret);
    }
    struct agm_dump_info dump_info = {signal, (uint32_t)pid, (uint32_t)uid};
    agm_dump(&dump_info);
}

ResourceManager::ResourceManager()
{
    PAL_INFO(LOG_TAG, "Enter: %p", this);
    int ret = 0;
    // Init audio_route and audio_mixer
    na_props.rm_na_prop_enabled = false;
    na_props.ui_na_prop_enabled = false;
    na_props.na_mode = NATIVE_AUDIO_MODE_INVALID;
    max_session_num = DEFAULT_MAX_SESSIONS;
    max_nt_sessions = DEFAULT_NT_SESSION_TYPE_COUNT;
    //TODO: parse the tag and populate in the tags
    streamTag.clear();
    deviceTag.clear();
    btCodecMap.clear();
    btSlimClockSrcMap.clear();
    usb_vendor_uuid_list.clear();

    vsidInfo.loopback_delay = 0;

    //Initialize class members in the construct
    bOverwriteFlag = false;
    cookie = 0;
    memset(&this->linear_gain, 0, sizeof(pal_param_mspp_linear_gain_t));
    memset(&this->mSpkrProtModeValue, 0, sizeof(pal_spkr_prot_payload));
    mHighestPriorityActiveStream = nullptr;
    mPriorityHighestPriorityActiveStream = 0;

    ret = memLoggerInitQ(PAL_STATE_Q, MEMLOG_CFG_FILE); //initializes the queue for the debug logger

    if (ret) {
        PAL_ERR(LOG_TAG, "error in initializing memory queue %d", ret);
    }

    ret = memLoggerInitQ(KPI_Q, MEMLOG_CFG_FILE); //initializes the queue for the debug logger

    if (ret) {
        PAL_ERR(LOG_TAG, "error in initializing KPI queue %d", ret);
    }

    ret = ResourceManager::XmlParser(SNDPARSER);
    if (ret) {
        PAL_ERR(LOG_TAG, "error in snd xml parsing ret %d", ret);
    }

    ret = ResourceManager::init_audio();
    if (ret) {
        PAL_ERR(LOG_TAG, "error in init audio route and audio mixer ret %d", ret);
        throw std::runtime_error("error in init audio route and audio mixer");
    }

    cardState = CARD_STATUS_ONLINE;
    ret = ResourceManager::XmlParser(rmngr_xml_file);
    if (ret == -ENOENT) // try resourcemanager xml without variant name
        ret = ResourceManager::XmlParser(rmngr_xml_file_wo_variant);
    if (ret) {
        PAL_ERR(LOG_TAG, "error in resource xml parsing ret %d", ret);
        throw std::runtime_error("error in resource xml parsing");
    }

    if (IsVirtualPortForUPDEnabled()) {
        updateVirtualBackendName();
        updateVirtualBESndName();
    }

    if (isHifiFilterEnabled)
        audio_route_apply_and_update_path(audio_route, "hifi-filter-coefficients");

    char propValue[PROPERTY_VALUE_MAX];
    bool isBuildDebuggable = false;
    property_get("ro.debuggable", propValue, "0");
    if(atoi(propValue) == 1) {
        isBuildDebuggable = true;
    }

    if (isSignalHandlerEnabled) {
        mSigHandler = SignalHandler::getInstance();
        if (mSigHandler) {
            std::function<void(int, pid_t, uid_t)> crashSignalCb = sendCrashSignal;
            SignalHandler::setClientCallback(crashSignalCb);
            SignalHandler::setBuildDebuggable(isBuildDebuggable);
            mSigHandler->registerSignalHandler(gSignalsOfInterest);
        } else {
            PAL_INFO(LOG_TAG, "Failed to create signal handler");
        }
    }

#if defined(ADSP_SLEEP_MONITOR)
    lpi_counter_ = 0;
    nlpi_counter_ = 0;
    sleepmon_fd_ = -1;
    sleepmon_fd_ = open(ADSPSLEEPMON_DEVICE_NAME, O_RDWR);
    if (sleepmon_fd_ == -1)
        PAL_ERR(LOG_TAG, "Failed to open ADSP sleep monitor file");
#endif
    listAllFrontEndIds.clear();
    listFreeFrontEndIds.clear();
    listAllPcmPlaybackFrontEnds.clear();
    listAllPcmRecordFrontEnds.clear();
    listAllPcmHostlessRxFrontEnds.clear();
    listAllNonTunnelSessionIds.clear();
    listAllPcmHostlessTxFrontEnds.clear();
    listAllCompressPlaybackFrontEnds.clear();
    listAllCompressRecordFrontEnds.clear();
    listAllPcmVoice1RxFrontEnds.clear();
    listAllPcmVoice1TxFrontEnds.clear();
    listAllPcmVoice2RxFrontEnds.clear();
    listAllPcmVoice2TxFrontEnds.clear();
    listAllPcmInCallRecordFrontEnds.clear();
    listAllPcmInCallMusicFrontEnds.clear();
    listAllPcmContextProxyFrontEnds.clear();
    listAllPcmExtEcTxFrontEnds.clear();
    memset(stream_instances, 0, PAL_STREAM_MAX * sizeof(uint64_t));
    memset(in_stream_instances, 0, PAL_STREAM_MAX * sizeof(uint64_t));

    for (int i=0; i < devInfo.size(); i++) {

        if (devInfo[i].type == PCM) {
            if (devInfo[i].sess_mode == HOSTLESS && devInfo[i].playback == 1) {
                listAllPcmHostlessRxFrontEnds.push_back(devInfo[i].deviceId);
            } else if (devInfo[i].sess_mode == HOSTLESS && devInfo[i].record == 1) {
                listAllPcmHostlessTxFrontEnds.push_back(devInfo[i].deviceId);
            } else if (devInfo[i].playback == 1 && devInfo[i].sess_mode == DEFAULT) {
                listAllPcmPlaybackFrontEnds.push_back(devInfo[i].deviceId);
            } else if (devInfo[i].record == 1 && devInfo[i].sess_mode == DEFAULT) {
                listAllPcmRecordFrontEnds.push_back(devInfo[i].deviceId);
            } else if (devInfo[i].sess_mode == NON_TUNNEL && devInfo[i].record == 1) {
                listAllPcmInCallRecordFrontEnds.push_back(devInfo[i].deviceId);
            } else if (devInfo[i].sess_mode == NON_TUNNEL && devInfo[i].playback == 1) {
                listAllPcmInCallMusicFrontEnds.push_back(devInfo[i].deviceId);
            } else if (devInfo[i].sess_mode == NO_CONFIG && devInfo[i].record == 1) {
                listAllPcmContextProxyFrontEnds.push_back(devInfo[i].deviceId);
            }
        } else if (devInfo[i].type == COMPRESS) {
            if (devInfo[i].playback == 1) {
                listAllCompressPlaybackFrontEnds.push_back(devInfo[i].deviceId);
            } else if (devInfo[i].record == 1) {
                listAllCompressRecordFrontEnds.push_back(devInfo[i].deviceId);
            }
        } else if (devInfo[i].type == VOICE1) {
            if (devInfo[i].sess_mode == HOSTLESS && devInfo[i].playback == 1) {
                listAllPcmVoice1RxFrontEnds.push_back(devInfo[i].deviceId);
            }
            if (devInfo[i].sess_mode == HOSTLESS && devInfo[i].record == 1) {
                listAllPcmVoice1TxFrontEnds.push_back(devInfo[i].deviceId);
            }
        } else if (devInfo[i].type == VOICE2) {
            if (devInfo[i].sess_mode == HOSTLESS && devInfo[i].playback == 1) {
                listAllPcmVoice2RxFrontEnds.push_back(devInfo[i].deviceId);
            }
            if (devInfo[i].sess_mode == HOSTLESS && devInfo[i].record == 1) {
                listAllPcmVoice2TxFrontEnds.push_back(devInfo[i].deviceId);
            }
        } else if (devInfo[i].type == ExtEC) {
            if (devInfo[i].sess_mode == HOSTLESS && devInfo[i].record == 1) {
                listAllPcmExtEcTxFrontEnds.push_back(devInfo[i].deviceId);
            }
        }
        /*We create a master list of all the frontends*/
        listAllFrontEndIds.push_back(devInfo[i].deviceId);
    }
    /*
     *Arrange all the FrontendIds in descending order, this gives the
     *largest deviceId being used for ALSA usecases.
     *For NON-TUNNEL usecases the sessionIds to be used are formed by incrementing the largest used deviceID
     *with number of non-tunnel sessions supported on a platform. This way we avoid any conflict of deviceIDs.
     */
     sort(listAllFrontEndIds.rbegin(), listAllFrontEndIds.rend());
     int maxDeviceIdInUse = listAllFrontEndIds.at(0);
     for (int i = 0; i < max_nt_sessions; i++)
          listAllNonTunnelSessionIds.push_back(maxDeviceIdInUse + i);

#ifndef LINUX_ENABLED
    // Get AGM service handle
    ret = agm_register_service_crash_callback(&agmServiceCrashHandler,
                                               (uint64_t)this);
    if (ret) {
        PAL_ERR(LOG_TAG, "AGM service not up%d", ret);
    }
#endif

    auto encodeMap = std::make_shared<std::unordered_map<uint32_t, bool>>();
    auto decodeMap = std::make_shared<std::unordered_map<uint32_t, bool>>();
    mNTStreamInstancesList[NT_PATH_ENCODE] = encodeMap;
    mNTStreamInstancesList[NT_PATH_DECODE] = decodeMap;

    ResourceManager::loadAdmLib();
    ResourceManager::initWakeLocks();
    ret = PayloadBuilder::init();
    if (ret) {
        throw std::runtime_error("Failed to parse usecase manager xml");
    } else {
        PAL_INFO(LOG_TAG, "usecase manager xml parsing successful");
    }

    if (ResourceManager::isHapticsthroughWSA) {
        ret = AudioHapticsInterface::init();
        if (ret) {
            throw std::runtime_error("Failed to parse hapticsconfig xml");
        } else {
            PAL_INFO(LOG_TAG, "hapticsconfig xml parsing successful");
        }
    }

    PAL_DBG(LOG_TAG, "Creating ContextManager");
    ctxMgr = new ContextManager();
    if (!ctxMgr) {
        throw std::runtime_error("Failed to allocate ContextManager");

    }

    // init use_lpi_ flag
    use_lpi_ = IsLPISupported();


#ifdef SOC_PERIPHERAL_PROT
    socPerithread = std::thread(loadSocPeripheralLib);
#endif
    PAL_INFO(LOG_TAG, "Exit: %p", this);
}
ResourceManager::~ResourceManager()
{
    // Dump memory logger queues
    int ret = memLoggerDumpAllToFile();
    if (ret)
    {
        PAL_ERR(LOG_TAG, "error in dumping queues: %d", ret);
    }
    ret = memLoggerDeinitQ(PAL_STATE_Q);
    if (ret)
    {
        PAL_ERR(LOG_TAG, "error in deinitializing memory queue %d", ret);
    }
    ret = memLoggerDeinitQ(KPI_Q);

    if (ret)
    {
        PAL_ERR(LOG_TAG, "error in deinitializing KPI queue %d", ret);
    }

    streamTag.clear();
    streamPpTag.clear();
    mixerTag.clear();
    devicePpTag.clear();
    deviceTag.clear();

    listAllFrontEndIds.clear();
    listAllPcmPlaybackFrontEnds.clear();
    listAllPcmRecordFrontEnds.clear();
    listAllPcmHostlessRxFrontEnds.clear();
    listAllPcmHostlessTxFrontEnds.clear();
    listAllCompressPlaybackFrontEnds.clear();
    listAllCompressRecordFrontEnds.clear();
    listFreeFrontEndIds.clear();
    listAllPcmVoice1RxFrontEnds.clear();
    listAllPcmVoice1TxFrontEnds.clear();
    listAllPcmVoice2RxFrontEnds.clear();
    listAllPcmVoice2TxFrontEnds.clear();
    listAllNonTunnelSessionIds.clear();
    listAllPcmExtEcTxFrontEnds.clear();
    usb_vendor_uuid_list.clear();
    devInfo.clear();
    txEcInfo.clear();

    STInstancesLists.clear();
    devicePcmId.clear();
    PCMDataInstances.clear();

    if (admLibHdl) {
        if (admDeInitFn)
            admDeInitFn(admData);
        dlclose(admLibHdl);
    }

    ResourceManager::deInitWakeLocks();
    if (ctxMgr) {
        delete ctxMgr;
    }
#ifdef ADSP_SLEEP_MONITOR
    if (sleepmon_fd_ >= 0)
        close(sleepmon_fd_);
#endif

#ifdef SOC_PERIPHERAL_PROT
     deregPeripheralCb(tz_handle);
#endif
}

#ifdef SOC_PERIPHERAL_PROT
int ResourceManager::registertoPeripheral(uint32_t pUID)
{
    int retry = PRPHRL_REGSTR_RETRY_COUNT;
    int state = PRPHRL_SUCCESS;

    if (mRegisterPeripheralCb) {
        do {
            /* register callback function with TZ service to get notifications of state change */
            tz_handle = mRegisterPeripheralCb(pUID, secureZoneEventCb);
            if (tz_handle != NULL) {
                PAL_INFO(LOG_TAG, "registered call back for audio peripheral[0x%x] to TZ", pUID);
                break;
            }
            retry--;
            usleep(1000);
        } while(retry);
    }

    if (retry == 0)
    {
        PAL_INFO(LOG_TAG, "Failed to register call back for audio peripheral to TZ");
        state = PRPHRL_ERROR;
        return state;
    }

    /** Getting current peripheral state after connection */
    if (mGetPeripheralState) {
        state = mGetPeripheralState(tz_handle);
        if (state == PRPHRL_ERROR) {
            PAL_ERR(LOG_TAG, "Failed to get Peripheral state from TZ");
            state = PRPHRL_ERROR;
            return state;
        } else if (state == STATE_SECURE) {
            ResourceManager::isTZSecureZone = true;
        }
    }
    PAL_DBG(LOG_TAG, "Soc peripheral thread exit");
    return state;
}

int ResourceManager::deregPeripheralCb(void *tz_handle)
{
    if (tz_handle && mDeregisterPeripheralCb) {
        mDeregisterPeripheralCb(tz_handle);
        mRegisterPeripheralCb = nullptr;
        mDeregisterPeripheralCb = nullptr;
        dlclose(socPeripheralLibHdl);
        socPeripheralLibHdl = nullptr;
    }

    return -1;
}

void ResourceManager::loadSocPeripheralLib()
{
    if (access(SOC_PERIPHERAL_LIBRARY_PATH, R_OK) == 0) {
        socPeripheralLibHdl = dlopen(SOC_PERIPHERAL_LIBRARY_PATH, RTLD_NOW);
        if (socPeripheralLibHdl == NULL) {
            PAL_ERR(LOG_TAG, "DLOPEN failed for %s %s", SOC_PERIPHERAL_LIBRARY_PATH, dlerror());
        } else {
            PAL_VERBOSE(LOG_TAG, "DLOPEN successful for %s", SOC_PERIPHERAL_LIBRARY_PATH);
            mRegisterPeripheralCb = (registerPeripheralCBFnPtr)
                dlsym(socPeripheralLibHdl, "registerPeripheralCB");
            const char *dlsym_error = dlerror();
            if (dlsym_error) {
                 PAL_ERR(LOG_TAG, "cannot find registerPeripheralCB symbol");
            }

            mGetPeripheralState = (getPeripheralStatusFnPtr)
                dlsym(socPeripheralLibHdl, "getPeripheralState");
            dlsym_error = dlerror();
            if (dlsym_error) {
                 PAL_ERR(LOG_TAG, "cannot find getPeripheralState symbol");
            }

            mDeregisterPeripheralCb = (deregisterPeripheralCBFnPtr)
                dlsym(socPeripheralLibHdl, "deregisterPeripheralCB");
            dlsym_error = dlerror();
            if (dlsym_error) {
                 PAL_ERR(LOG_TAG, "cannot find deregisterPeripheralCB symbol");
            }
            registertoPeripheral(CPeripheralAccessControl_AUDIO_UID);
        }
    }
}
#endif

void ResourceManager::loadAdmLib()
{
    if (access(ADM_LIBRARY_PATH, R_OK) == 0) {
        admLibHdl = dlopen(ADM_LIBRARY_PATH, RTLD_NOW);
        if (admLibHdl == NULL) {
            PAL_ERR(LOG_TAG, "DLOPEN failed for %s %s", ADM_LIBRARY_PATH, dlerror());
        } else {
            PAL_VERBOSE(LOG_TAG, "DLOPEN successful for %s", ADM_LIBRARY_PATH);
            admInitFn = (adm_init_t)
                dlsym(admLibHdl, "adm_init");
            admDeInitFn = (adm_deinit_t)
                dlsym(admLibHdl, "adm_deinit");
            admRegisterInputStreamFn = (adm_register_input_stream_t)
                dlsym(admLibHdl, "adm_register_input_stream");
            admRegisterOutputStreamFn = (adm_register_output_stream_t)
                dlsym(admLibHdl, "adm_register_output_stream");
            admDeregisterStreamFn = (adm_deregister_stream_t)
                dlsym(admLibHdl, "adm_deregister_stream");
            admRequestFocusFn = (adm_request_focus_t)
                dlsym(admLibHdl, "adm_request_focus");
            admAbandonFocusFn = (adm_abandon_focus_t)
                dlsym(admLibHdl, "adm_abandon_focus");
            admSetConfigFn = (adm_set_config_t)
                dlsym(admLibHdl, "adm_set_config");
            admRequestFocusV2Fn = (adm_request_focus_v2_t)
                dlsym(admLibHdl, "adm_request_focus_v2");
            admOnRoutingChangeFn = (adm_on_routing_change_t)
                dlsym(admLibHdl, "adm_on_routing_change");
            admRequestFocus_v2_1Fn = (adm_request_focus_v2_1_t)
                dlsym(admLibHdl, "adm_request_focus_v2_1");

            dlerror(); // clear error during dlsym, if any.
            if (admInitFn)
                admData = admInitFn();
        }
    }
}

int ResourceManager::initWakeLocks(void) {

    char buf[MAX_WAKE_LOCK_LENGTH] = {};
    int size = 0, ret = 0;

    wake_lock_fd = ::open(WAKE_LOCK_PATH, O_RDWR|O_APPEND);
    if (wake_lock_fd < 0) {
        PAL_ERR(LOG_TAG, "Unable to open %s, err:%s",
            WAKE_LOCK_PATH, strerror(errno));
        if (errno == ENOENT) {
            PAL_INFO(LOG_TAG, "No wake lock support");
            return -ENOENT;
        }
        return -EINVAL;
    }
    wake_unlock_fd = ::open(WAKE_UNLOCK_PATH, O_WRONLY|O_APPEND);
    if (wake_unlock_fd < 0) {
        PAL_ERR(LOG_TAG, "Unable to open %s, err:%s",
            WAKE_UNLOCK_PATH, strerror(errno));
        ::close(wake_lock_fd);
        wake_lock_fd = -1;
        return -EINVAL;
    }

    size = ::read(wake_lock_fd, buf, sizeof(buf) - 1);
    buf[MAX_WAKE_LOCK_LENGTH - 1] = '\0';
    if (size >= 0) {
        if (strstr(buf, WAKE_LOCK_NAME)) {
            PAL_INFO(LOG_TAG, "Clean up wake lock after restart");
            ret = ::write(wake_unlock_fd, WAKE_LOCK_NAME, strlen(WAKE_LOCK_NAME));
            if (ret < 0) {
                PAL_ERR(LOG_TAG, "Failed to release wakelock %d %s",
                    ret, strerror(errno));
                return ret;
            }
        }
    }
    return 0;
}

void ResourceManager::deInitWakeLocks(void) {
    if (wake_lock_fd >= 0) {
        ::close(wake_lock_fd);
        wake_lock_fd = -1;
    }
    if (wake_unlock_fd >= 0) {
        ::close(wake_unlock_fd);
        wake_unlock_fd = -1;
    }
}

void ResourceManager::acquireWakeLock() {
    int ret = 0;

    mResourceManagerMutex.lock();
    if (wake_lock_fd < 0) {
        PAL_ERR(LOG_TAG, "Invalid fd %d", wake_lock_fd);
        goto exit;
    }

    PAL_DBG(LOG_TAG, "wake lock count: %d", wake_lock_cnt);
    if (++wake_lock_cnt == 1) {
        PAL_INFO(LOG_TAG, "Acquiring wake lock %s", WAKE_LOCK_NAME);
        ret = ::write(wake_lock_fd, WAKE_LOCK_NAME, strlen(WAKE_LOCK_NAME));
        if (ret < 0)
            PAL_ERR(LOG_TAG, "Failed to acquire wakelock %d %s",
                ret, strerror(errno));
    }

exit:
    mResourceManagerMutex.unlock();
}

void ResourceManager::releaseWakeLock() {
    int ret = 0;

    mResourceManagerMutex.lock();
    if (wake_unlock_fd < 0) {
        PAL_ERR(LOG_TAG, "Invalid fd %d", wake_unlock_fd);
        goto exit;
    }

    PAL_DBG(LOG_TAG, "wake lock count: %d", wake_lock_cnt);
    if (wake_lock_cnt > 0 && --wake_lock_cnt == 0) {
        PAL_INFO(LOG_TAG, "Releasing wake lock %s", WAKE_LOCK_NAME);
        ret = ::write(wake_unlock_fd, WAKE_LOCK_NAME, strlen(WAKE_LOCK_NAME));
        if (ret < 0)
            PAL_ERR(LOG_TAG, "Failed to release wakelock %d %s",
                ret, strerror(errno));
    }

exit:
     mResourceManagerMutex.unlock();
}

bool ResourceManager::isSsrDownFeasible(std::shared_ptr<ResourceManager> rm,
                                        int type)
{
    bool do_ssr = true;

    /* Check only for down cases */
    switch (rm->cardState) {
    case CARD_STATUS_STANDBY:
        if (rm->isStreamSupportedInsndCardStandy(type))
            do_ssr = false;
        break;
    case CARD_STATUS_OFFLINE:
    default:
        break;
    }

    return do_ssr;
}

void ResourceManager::ssrHandlingLoop(std::shared_ptr<ResourceManager> rm)
{
    card_status_t state;
    card_status_t prevState = CARD_STATUS_ONLINE;
    std::unique_lock<std::mutex> lock(rm->cvMutex);
    int32_t ret = 0;
    uint32_t eventData;
    pal_global_callback_event_t event;
    pal_stream_type_t type;

    PAL_VERBOSE(LOG_TAG,"ssr Handling thread started");

    while(1) {
        if (rm->msgQ.empty())
            rm->cv.wait(lock);
        if (!rm->msgQ.empty()) {
            state = rm->msgQ.front();
            rm->msgQ.pop();
            lock.unlock();
            PAL_INFO(LOG_TAG, "state %d, prev state %d size %zu",
                               state, prevState, rm->mActiveStreams.size());
            if (state == CARD_STATUS_NONE)
                break;

            mActiveStreamMutex.lock();
            rm->cardState = state;
            if (state != prevState) {
                if (rm->globalCb) {
                    PAL_DBG(LOG_TAG, "Notifying client about sound card state %d global cb %pK",
                                      rm->cardState, rm->globalCb);
                    eventData = (int)rm->cardState;
                    event = PAL_SND_CARD_STATE;
                    PAL_DBG(LOG_TAG, "eventdata %d", eventData);
                    rm->globalCb(event, &eventData, cookie);
                }
            }

#ifdef SEC_AUDIO_COMMON
            if (state == CARD_STATUS_OFFLINE) {
                rm->ssrStarted = true;
                //get Currnte local time
                time_t rawtime;
                time (&rawtime);
                ssrTimeinfo = localtime(&rawtime);
                PAL_INFO(LOG_TAG, "Ssr Current local time and date: %s", asctime(ssrTimeinfo));
            }
#endif

            if (rm->mActiveStreams.empty()) {
                /*
                 * Context manager closes its streams on down, so empty list may still
                 * require CM up handling
                 */
                if (state == CARD_STATUS_ONLINE) {
                    if (isContextManagerEnabled) {
                        mActiveStreamMutex.unlock();
                        ret = ctxMgr->ssrUpHandler();
                        if (0 != ret) {
                            PAL_ERR(LOG_TAG, "Ssr up handling failed for ContextManager ret %d", ret);
                        }
                        mActiveStreamMutex.lock();
                    }
                }

                PAL_INFO(LOG_TAG, "Idle SSR : No streams registered yet.");
                prevState = state;
            } else if (state == prevState) {
                PAL_INFO(LOG_TAG, "%d state already handled", state);
            } else if (PAL_CARD_STATUS_DOWN(state)) {
                for (auto str: rm->mActiveStreams) {
                    ret = increaseStreamUserCounter(str);
                    if (0 != ret) {
                        PAL_ERR(LOG_TAG, "Error incrementing the stream counter for the stream handle: %pK", str);
                        continue;
                    }
                    ret = str->ssrDownHandler();
                    if (0 != ret) {
                        PAL_ERR(LOG_TAG, "Ssr down handling failed for %pK ret %d",
                                          str, ret);
                    }
                    ret = str->getStreamType(&type);
                    if (type == PAL_STREAM_NON_TUNNEL) {
                        ret = voteSleepMonitor(str, false);
                        if (ret)
                            PAL_DBG(LOG_TAG, "Failed to unvote for stream type %d", type);
                    }
                    ret = decreaseStreamUserCounter(str);
                    if (0 != ret) {
                        PAL_ERR(LOG_TAG, "Error decrementing the stream counter for the stream handle: %pK", str);
                    }
                }
                if (isContextManagerEnabled) {
                    mActiveStreamMutex.unlock();
                    ret = ctxMgr->ssrDownHandler();
                    if (0 != ret) {
                        PAL_ERR(LOG_TAG, "Ssr down handling failed for ContextManager ret %d", ret);
                    }
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
                    else {
                        PAL_INFO(LOG_TAG, "Ssr down handling for ContextManager");
                    }
#endif
                    mActiveStreamMutex.lock();
                }
                prevState = state;
            } else if (PAL_CARD_STATUS_UP(state)) {
                if (isContextManagerEnabled) {
                    mActiveStreamMutex.unlock();
                    ret = ctxMgr->ssrUpHandler();
                    if (0 != ret) {
                        PAL_ERR(LOG_TAG, "Ssr up handling failed for ContextManager ret %d", ret);
                    }
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
                    else {
                        PAL_INFO(LOG_TAG, "Ssr up handling for ContextManager");
                    }
#endif
                    mActiveStreamMutex.lock();
                }

                mResourceManagerMutex.lock();
                SoundTriggerCaptureProfile = GetCaptureProfileByPriority(nullptr, "va_macro");
                TXMacroCaptureProfile = GetCaptureProfileByPriority(nullptr, "tx_macro");
                mResourceManagerMutex.unlock();
                for (auto str: rm->mActiveStreams) {
                    ret = increaseStreamUserCounter(str);
                    if (0 != ret) {
                        PAL_ERR(LOG_TAG, "Error incrementing the stream counter for the stream handle: %pK", str);
                        continue;
                    }
                    ret = str->ssrUpHandler();
                    if (0 != ret) {
                        PAL_ERR(LOG_TAG, "Ssr up handling failed for %pK ret %d",
                                          str, ret);
                    }
                    ret = decreaseStreamUserCounter(str);
                    if (0 != ret) {
                        PAL_ERR(LOG_TAG, "Error decrementing the stream counter for the stream handle: %pK", str);
                    }
                }
                prevState = state;
            } else {
                PAL_ERR(LOG_TAG, "Invalid state. state %d", state);
            }
            mActiveStreamMutex.unlock();
            lock.lock();
        }
    }
    PAL_INFO(LOG_TAG, "ssr Handling thread ended");
}

int ResourceManager::initSndMonitor()
{
    int ret = 0;
    workerThread = std::thread(&ResourceManager::ssrHandlingLoop, this, rm);
    sndmon = new SndCardMonitor(snd_hw_card);
    if (!sndmon) {
        ret = -EINVAL;
        PAL_ERR(LOG_TAG, "Sound monitor creation failed, ret %d", ret);
    }
    return ret;
}

void ResourceManager::ssrHandler(card_status_t state)
{
    PAL_DBG(LOG_TAG, "Enter. state %d", state);
    cvMutex.lock();
    msgQ.push(state);
    cvMutex.unlock();
    cv.notify_all();
    PAL_DBG(LOG_TAG, "Exit. state %d", state);
    return;
}

char* ResourceManager::getDeviceNameFromID(uint32_t id)
{
    for (int i=0; i < devInfo.size(); i++) {
        if (devInfo[i].deviceId == id) {
            PAL_DBG(LOG_TAG, "pcm id name is %s ", devInfo[i].name);
            return devInfo[i].name;
        }
    }

    return NULL;
}

int ResourceManager::init_audio()
{
    int retry = 0;
    int status = 0;
    bool snd_card_found = false;

    char *snd_card_name = NULL;
    FILE *file = NULL;
    char mixer_xml_file[XML_PATH_MAX_LENGTH] = {0};
    char mixer_xml_file_wo_variant[XML_PATH_MAX_LENGTH] = {0};
    char file_name_extn[XML_PATH_EXTN_MAX_SIZE] = {0};
    char file_name_extn_wo_variant[XML_PATH_EXTN_MAX_SIZE] = {0};

    PAL_DBG(LOG_TAG, "Enter.");

#ifdef SEC_FACTORY_INTERPOSER
    property_set("vendor.audio.use.primary.default", "true");
    PAL_ERR(LOG_TAG, "skip audio mixer open on secondary bin (not support audio)");
#endif
#ifdef SEC_AUDIO_BOOT_ON_ERR
    bool use_dummy_snd_card = false;
    if (property_get_bool("vendor.audio.use.primary.default", false)) {
        // skip mixer open
        PAL_ERR(LOG_TAG, "init audio hal with dummy snd card (audio not functional)");
        use_dummy_snd_card = true;
        goto update_snd_card_name;
    }
#endif

    do {
        /* Look for only default codec sound card */
        /* Ignore USB sound card if detected */
        snd_hw_card = SND_CARD_HW;
        while (snd_hw_card < MAX_SND_CARD) {
            struct audio_mixer* tmp_mixer = NULL;
            tmp_mixer = mixer_open(snd_hw_card);
            if (tmp_mixer) {
                snd_card_name = strdup(mixer_get_name(tmp_mixer));
                if (!snd_card_name) {
                    PAL_ERR(LOG_TAG, "failed to allocate memory for snd_card_name");
                    mixer_close(tmp_mixer);
                    status = -EINVAL;
                    goto exit;
                }
                PAL_INFO(LOG_TAG, "mixer_open success. snd_card_num = %d, snd_card_name %s",
                snd_hw_card, snd_card_name);

                /* TODO: Needs to extend for new targets */
                if (strstr(snd_card_name, "kona") ||
                    strstr(snd_card_name, "sm8150") ||
                    strstr(snd_card_name, "sdx")||
                    strstr(snd_card_name, "lahaina") ||
                    strstr(snd_card_name, "waipio") ||
                    strstr(snd_card_name, "kalama") ||
                    strstr(snd_card_name, "pineapple") ||
                    strstr(snd_card_name, "cliffs") ||
                    strstr(snd_card_name, "anorak") ||
                    strstr(snd_card_name, "diwali") ||
                    strstr(snd_card_name, "bengal") ||
                    strstr(snd_card_name, "monaco") ||
                    strstr(snd_card_name, "sun")) {
                    PAL_VERBOSE(LOG_TAG, "Found Codec sound card");
                    snd_card_found = true;
                    audio_hw_mixer = tmp_mixer;
                    break;
                } else {
                    if (snd_card_name) {
                        free(snd_card_name);
                        snd_card_name = NULL;
                    }
                    mixer_close(tmp_mixer);
                }
            }
            snd_hw_card++;
        }

        if (!snd_card_found) {
            PAL_INFO(LOG_TAG, "No audio mixer, retry %d", retry++);
            sleep(1);
        }
    } while (!snd_card_found && retry <= MAX_RETRY_CNT);

    if (snd_hw_card >= MAX_SND_CARD || !audio_hw_mixer) {
        PAL_ERR(LOG_TAG, "audio mixer open failure");

#ifdef SEC_AUDIO_BOOT_ON_ERR
        property_set("vendor.audio.use.primary.default", "true");
        LOG_ALWAYS_FATAL("sound card err, vendor.audio.use.primary.default as true");
#endif
        status = -EINVAL;
        goto exit;
    }

#ifdef SEC_AUDIO_BOOT_ON_ERR
update_snd_card_name:
    if (use_dummy_snd_card) {
        // load dummy snd card
        char dummy_snd_card_name[MAX_DUMMY_CARD_LENGTH];
        char vendor_sku[PROPERTY_VALUE_MAX] = {'\0'};
        if (property_get("ro.boot.product.vendor.sku", vendor_sku, "") <= 0) {
            // (Todo) : need to change this vendor_sku name for each chipset
            PAL_ERR(LOG_TAG, "ro.boot.product.vendor.sku not defined, use sun");
            strlcpy(vendor_sku, "sun", PROPERTY_VALUE_MAX);
        }
        snprintf(dummy_snd_card_name, MAX_DUMMY_CARD_LENGTH, "%s%s", vendor_sku, "-qrd-snd-card");
        PAL_ERR(LOG_TAG, "snd card not available, set card to %s (default hal)", dummy_snd_card_name);

        getFileNameExtn(dummy_snd_card_name, file_name_extn, file_name_extn_wo_variant);
    } else
#endif
    {
        audio_virt_mixer = mixer_open(snd_virt_card);
        if(!audio_virt_mixer) {
            PAL_ERR(LOG_TAG, "Error: %d virtual audio mixer open failure", -EIO);
            if (snd_card_name)
                free(snd_card_name);
            mixer_close(audio_hw_mixer);
            return -EIO;
        }

        getFileNameExtn(snd_card_name, file_name_extn, file_name_extn_wo_variant);
    }

    getVendorConfigPath(vendor_config_path, sizeof(vendor_config_path));

    /* Get path for platorm_info_xml_path_name in vendor */
    snprintf(mixer_xml_file, sizeof(mixer_xml_file),
            "%s/%s", vendor_config_path, MIXER_XML_BASE_STRING_NAME);

    snprintf(rmngr_xml_file, sizeof(rmngr_xml_file),
            "%s/%s", vendor_config_path, RMNGR_XMLFILE_BASE_STRING_NAME);

#ifdef SEC_AUDIO_BOOT_ON_ERR
    if (use_dummy_snd_card)
#endif
    {
        strlcat(mixer_xml_file, XML_FILE_DELIMITER, XML_PATH_MAX_LENGTH);
        strlcat(mixer_xml_file_wo_variant, mixer_xml_file, XML_PATH_MAX_LENGTH);
        strlcat(mixer_xml_file, file_name_extn, XML_PATH_MAX_LENGTH);
        strlcat(mixer_xml_file_wo_variant, file_name_extn_wo_variant, XML_PATH_MAX_LENGTH);
        strlcat(rmngr_xml_file, XML_FILE_DELIMITER, XML_PATH_MAX_LENGTH);
        strlcpy(rmngr_xml_file_wo_variant, rmngr_xml_file, XML_PATH_MAX_LENGTH);
        strlcat(rmngr_xml_file, file_name_extn, XML_PATH_MAX_LENGTH);
        strlcat(rmngr_xml_file_wo_variant, file_name_extn_wo_variant, XML_PATH_MAX_LENGTH);
    }

    strlcat(mixer_xml_file, XML_FILE_EXT, XML_PATH_MAX_LENGTH);
    strlcat(rmngr_xml_file, XML_FILE_EXT, XML_PATH_MAX_LENGTH);
    strlcat(rmngr_xml_file_wo_variant, XML_FILE_EXT, XML_PATH_MAX_LENGTH);
    strlcat(mixer_xml_file_wo_variant, XML_FILE_EXT, XML_PATH_MAX_LENGTH);

#ifdef SEC_AUDIO_BOOT_ON_ERR
    // skip to audio_route_init
    if (use_dummy_snd_card) {
        goto exit;
    }
#endif

    audio_route = audio_route_init(snd_hw_card, mixer_xml_file);
    PAL_INFO(LOG_TAG, "audio route %pK, mixer path %s", audio_route, mixer_xml_file);
    if (!audio_route) {
        PAL_ERR(LOG_TAG, "audio route init failed trying with mixer without variant name");
        audio_route = audio_route_init(snd_hw_card, mixer_xml_file_wo_variant);
        PAL_INFO(LOG_TAG, "audio route %pK, mixer path %s", audio_route, mixer_xml_file_wo_variant);
        if (!audio_route) {
            PAL_ERR(LOG_TAG, "audio route init failed ");
            mixer_close(audio_virt_mixer);
            mixer_close(audio_hw_mixer);
            status = -EINVAL;
        }
    }
    // audio_route init success
exit:
    PAL_DBG(LOG_TAG, "Exit, status %d. audio route init with card %d mixer path %s", status,
            snd_hw_card, mixer_xml_file);
    if (snd_card_name) {
        free(snd_card_name);
        snd_card_name = NULL;
    }

    return status;
}

template <class T>
void getMatchingStStreams(std::list<T> &active_streams, std::vector<Stream*> &st_streams, vui_dmgr_uuid_t &uuid)
{
    int ret = 0;
    struct st_uuid st_uuid;

    for (auto st : active_streams) {
        if (NULL != st) {
            st_uuid = st->GetVendorUuid();
            if (!memcmp(&st_uuid, &uuid, sizeof(uuid))) {
                PAL_INFO(LOG_TAG, "vendor uuid matched");
                st_streams.push_back(static_cast<Stream*>(st));
            }
        }
    }
}

int32_t ResourceManager::voiceuiDmgrRestartUseCases(vui_dmgr_param_restart_usecases_t *uc_info)
{
    int status = 0;
    std::vector<Stream*> st_streams;
    pal_stream_type_t st_type;

    for (int i = 0; i < uc_info->num_usecases; i++) {
        if (uc_info->usecases[i].stream_type == PAL_STREAM_VOICE_UI && active_streams_st.size()) {
            PAL_INFO(LOG_TAG, "get matching streams for VoiceUI");
            getMatchingStStreams(active_streams_st, st_streams, uc_info->usecases[i].vendor_uuid);
        }
        else if (uc_info->usecases[i].stream_type == PAL_STREAM_ACD && active_streams_acd.size()) {
            PAL_INFO(LOG_TAG, "get matching streams for acd");
            getMatchingStStreams(active_streams_acd, st_streams, uc_info->usecases[i].vendor_uuid);
        }
        else if (uc_info->usecases[i].stream_type == PAL_STREAM_ASR && active_streams_asr.size()) {
            PAL_INFO(LOG_TAG, "get matching streams for asr");
            getMatchingStStreams(active_streams_asr, st_streams, uc_info->usecases[i].vendor_uuid);
        }
        else if (uc_info->usecases[i].stream_type == PAL_STREAM_SENSOR_PCM_DATA && active_streams_sensor_pcm_data.size()) {
            PAL_INFO(LOG_TAG, "get matching streams for sensor");
            getMatchingStStreams(active_streams_sensor_pcm_data, st_streams, uc_info->usecases[i].vendor_uuid);
        }
    }

    // Reuse SSR mechanism for stream teardown and bring up.
    PAL_INFO(LOG_TAG, "restart %d streams", st_streams.size());
    for (auto &st : st_streams) {
        st->getStreamType(&st_type);
        status = st->ssrDownHandler();
        if (status) {
            PAL_ERR(LOG_TAG, "stream teardown failed %d", st_type);
        }
        status = st->ssrUpHandler();
        if (status) {
            PAL_ERR(LOG_TAG, "strem bring up failed %d", st_type);
        }
    }
    return status;
}

int32_t ResourceManager::voiceuiDmgrPalCallback(int32_t param_id, void *payload, size_t payload_size)
{
    int status = 0;

    PAL_DBG(LOG_TAG, "Enter param id: %d", param_id);
    if (!payload) {
        PAL_ERR(LOG_TAG, "Null payload");
        return -EINVAL;
    }
    if (!rm) {
        PAL_ERR(LOG_TAG, "null resource manager");
        return -EINVAL;
    }

    switch (param_id) {
        case VUI_DMGR_PARAM_ID_RESTART_USECASES:
        {
            vui_dmgr_param_restart_usecases_t *uc_info = (vui_dmgr_param_restart_usecases_t *)payload;
            if (payload_size != sizeof(vui_dmgr_param_restart_usecases_t)) {
                PAL_ERR(LOG_TAG, "Incorrect payload size %zu", payload_size);
                status = -EINVAL;
                break;
            }
            if (rm) {
                mActiveStreamMutex.lock();
                rm->voiceuiDmgrRestartUseCases(uc_info);
                mActiveStreamMutex.unlock();
            }
        }
        break;
        default:
            PAL_ERR(LOG_TAG, "Unknown param id: %d", param_id);
            break;
    }

    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

void ResourceManager::voiceuiDmgrManagerInit()
{
    int status = 0;

    vui_dmgr_lib_handle = dlopen(VUI_DMGR_LIB_PATH, RTLD_NOW);

    if (!vui_dmgr_lib_handle) {
        PAL_ERR(LOG_TAG, "dlopen failed for voiceui dmgr %s", dlerror());
        return;
    }

    vui_dmgr_init = (vui_dmgr_init_t)dlsym(vui_dmgr_lib_handle, "vui_dmgr_init");
    if (!vui_dmgr_init) {
        PAL_ERR(LOG_TAG, "dlsym for vui_dmgr_init failed %s", dlerror());
        goto exit;
    }
    vui_dmgr_deinit = (vui_dmgr_deinit_t)dlsym(vui_dmgr_lib_handle, "vui_dmgr_deinit");
    if (!vui_dmgr_deinit) {
        PAL_ERR(LOG_TAG, "dlsym for voiceui dmgr failed %s", dlerror());
        goto exit;
    }
    status = vui_dmgr_init(voiceuiDmgrPalCallback);
    if (status) {
        PAL_DBG(LOG_TAG, "voiceui dmgr failed to initialize, status %d", status);
        goto exit;
    }
    PAL_INFO(LOG_TAG, "voiceui dgmr initialized");
    return;

exit:
    if (vui_dmgr_lib_handle) {
        dlclose(vui_dmgr_lib_handle);
        vui_dmgr_lib_handle = NULL;
    }
    vui_dmgr_init = NULL;
    vui_dmgr_deinit = NULL;
}

void ResourceManager::checkQVAAppPresence(afs_param_payload_t *payload)
{
    std::ifstream fp;
    std::string qva_version = "";
    fp.open(QVA_VERSION);
    if (!fp) {
        PAL_ERR(LOG_TAG, "File operation for QVA App failed");
    } else {
        std::getline(fp, qva_version);
        if (qva_version.compare(0, 3, "qva") == 0) {
            strlcpy(payload->qva_version, qva_version.c_str(),
                    sizeof(payload->qva_version));
        }
    }
    fp.close();
    return;
}

pal_param_payload *ResourceManager::AFSWakeUpAlgoDetection()
{
    int32_t rc = 0;
    struct pal_stream_attributes stream_attr;
    pal_param_payload *payload = nullptr;

    memset(&stream_attr, 0, sizeof(struct pal_stream_attributes));
    stream_attr.type = PAL_STREAM_COMMON_PROXY;
    stream_attr.direction = PAL_AUDIO_INPUT;

    rc = pal_stream_open(&stream_attr, 0, NULL, 0, NULL,
                        NULL, 0, &afs_stream_handle);
    if (rc) {
        PAL_ERR(LOG_TAG, "Failed to open pal stream, ret = %d", rc);
        goto close_stream;
    }

    rc = pal_stream_start(afs_stream_handle);
    if (rc) {
        PAL_ERR(LOG_TAG, "Failed to start pal stream, ret = %d", rc);
        goto close_stream;
    }

    rc = pal_stream_get_param(afs_stream_handle, PAL_PARAM_ID_SVA_WAKEUP_MODULE_VERSION, &payload);
    if (rc) {
        PAL_ERR(LOG_TAG, "Failed to get pal stream attributes, ret = %d", rc);
        rc = pal_stream_stop(afs_stream_handle);
        if (rc) {
            PAL_ERR(LOG_TAG, "Failed to stop pal stream, ret = %d", rc);
        }
        goto close_stream;
    }

    if (payload) {
        rc = pal_stream_stop(afs_stream_handle);
        if (rc) {
            PAL_ERR(LOG_TAG, "Failed to stop pal stream, ret = %d", rc);
        }
        rc = pal_stream_close(afs_stream_handle);
        if (rc) {
            PAL_ERR(LOG_TAG, "Failed to close pal stream, ret = %d", rc);
        }
        PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
        return payload;
    }

close_stream:
    if (afs_stream_handle) {
        pal_stream_close(afs_stream_handle);
        afs_stream_handle = NULL;
    }
    if (payload)
        free(payload);
    return NULL;
}

int ResourceManager::AudioFeatureStatsGetInfo(void **afs_payload,
                                               size_t *afs_payload_size)
{
    afs_param_payload_t *afs_param_payload = nullptr;
    struct amdb_module_version_info_payload_t *module_version_payload = nullptr;
    std::shared_ptr<ResourceManager> rm = nullptr;

    afs_param_payload = (afs_param_payload_t *)calloc(1, sizeof(afs_param_payload_t));
    if (!afs_param_payload){
       PAL_ERR(LOG_TAG,"failed to allocate memory for the Feature Stats");
       *afs_payload = NULL;
       *afs_payload_size = 0;
       return -EINVAL;
    }

    rm = ResourceManager::getInstance();
    if (!rm) {
        PAL_ERR(LOG_TAG, "Null Resource Manager");
        *afs_payload = NULL;
        *afs_payload_size = 0;
        free(afs_param_payload);
        return -EINVAL;
    }
    rm->checkQVAAppPresence(afs_param_payload);
    pal_param_payload *payload = rm->AFSWakeUpAlgoDetection();
    if (payload) {
        module_version_payload = (amdb_module_version_info_payload_t*)((uint8_t *)payload +
                                  sizeof(amdb_param_id_module_version_info_t));
        if (module_version_payload->is_present != 1) {
            PAL_ERR(LOG_TAG,"Is Present is set to : %d", module_version_payload->is_present);
            *afs_payload = NULL;
            *afs_payload_size = 0;
            free(payload);
            free(afs_param_payload);
            return -EINVAL;
        }
        afs_param_payload->is_present = module_version_payload->is_present;
        afs_param_payload->module_version_major = module_version_payload->module_version_major;
        afs_param_payload->module_version_minor = module_version_payload->module_version_minor;
        afs_param_payload->build_ts = module_version_payload->build_ts;
    } else {
        PAL_ERR(LOG_TAG," Empty payload");
        *afs_payload = NULL;
        *afs_payload_size = 0;
        free(afs_param_payload);
        return -EINVAL;
    }

    *afs_payload = afs_param_payload;
    *afs_payload_size = sizeof(afs_param_payload_t);

    if (payload)
        free(payload);
    return 0;
}

void ResourceManager::AudioFeatureStatsInit()
{
    int status = 0;

    feature_stats_handle = dlopen(AFS_LIB_PATH, RTLD_NOW);
    if (!feature_stats_handle) {
        PAL_ERR(LOG_TAG, "dlopen failed for Feature Stats");
        return;
    }

    feature_stats_init = (afs_init_t)dlsym(feature_stats_handle, "AudioFeatureStatsInit");
    if (!feature_stats_init) {
        PAL_ERR(LOG_TAG, "dlsym for Feature Stats Init failed %s", dlerror());
        goto exit;
    }
    feature_stats_deinit = (afs_deinit_t)dlsym(feature_stats_handle, "AudioFeatureStatsDeInit");
    if (!feature_stats_deinit) {
        PAL_ERR(LOG_TAG, "dlsym for Feature Stats De-Init failed %s", dlerror());
        goto exit;
    }
    status = feature_stats_init(&AudioFeatureStatsGetInfo);
    if (status) {
        PAL_DBG(LOG_TAG, "Audio Feature Stats failed to initialize, status %d", status);
        goto exit;
    }
    PAL_INFO(LOG_TAG, "Audio Feature Stats initialized");
    return;

exit:
    if (feature_stats_handle) {
        dlclose(feature_stats_handle);
        feature_stats_handle = NULL;
    }
    feature_stats_init = NULL;
    feature_stats_deinit = NULL;
}

void ResourceManager::AudioFeatureStatsDeInit()
{
    if (feature_stats_deinit)
        feature_stats_deinit();

    if (feature_stats_handle) {
        dlclose(feature_stats_handle);
        feature_stats_handle = NULL;
    }
    feature_stats_init = NULL;
    feature_stats_deinit = NULL;
}

void ResourceManager::voiceuiDmgrManagerDeInit()
{
    if (vui_dmgr_deinit)
        vui_dmgr_deinit();

    if (vui_dmgr_lib_handle) {
        dlclose(vui_dmgr_lib_handle);
        vui_dmgr_lib_handle = NULL;
    }
    vui_dmgr_init = NULL;
    vui_dmgr_deinit = NULL;
}

int ResourceManager::initContextManager()
{
    int ret = 0;

    PAL_VERBOSE(LOG_TAG," isContextManagerEnabled: %s", isContextManagerEnabled? "true":"false");
    if (isContextManagerEnabled) {
        ret = ctxMgr->Init();
        if (ret != 0) {
            PAL_ERR(LOG_TAG, "ContextManager init failed :%d", ret);
        }
    }

    return ret;
}

void ResourceManager::deInitContextManager()
{
    if (isContextManagerEnabled) {
        ctxMgr->DeInit();
    }
}

int ResourceManager::init()
{
    std::shared_ptr<Device> dev = nullptr;
    std::shared_ptr<HapticsDev> Hapdev = nullptr;

    // Initialize Speaker Protection calibration mode
    struct pal_device dattr;

    mixerEventTread = std::thread(mixerEventWaitThreadLoop, rm);
    if (rm && rm->IsLowLatencyBargeinSupported()) {
        vui_deferred_switch_thread_ = std::thread(
            ResourceManager::voiceUIDeferredSwitchLoop, rm);
    }

    //Initialize audio_charger_listener
    if (rm && isChargeConcurrencyEnabled)
        rm->chargerListenerFeatureInit();

    // Get the speaker instance and activate speaker protection
    dattr.id = PAL_DEVICE_OUT_SPEAKER;
    dev = std::dynamic_pointer_cast<Device>(Device::getInstance(&dattr , rm));
    if (dev) {
        PAL_DBG(LOG_TAG, "Speaker instance created");
    }
    else
        PAL_DBG(LOG_TAG, "Speaker instance not created");

    if (ResourceManager::isHapticsthroughWSA) {
        dattr.id = PAL_DEVICE_OUT_HAPTICS_DEVICE;
        Hapdev = std::dynamic_pointer_cast<HapticsDev>(Device::getInstance(&dattr , rm));
        if (Hapdev) {
            PAL_DBG(LOG_TAG, "HapticsDev instance created");
        }
        else
           PAL_INFO(LOG_TAG, "HapticsDev instance not created");
    }

    PAL_INFO(LOG_TAG, "Initialize voiceui dmgr");
    voiceuiDmgrManagerInit();

    PAL_INFO(LOG_TAG, "Initialize Audio Feature Stats");
    AudioFeatureStatsInit();

    return 0;
}

bool ResourceManager::isLpiLoggingEnabled()
{
    char value[256] = {0};
    bool lpi_logging_prop = false;

#ifndef FEATURE_IPQ_OPENWRT
    property_get("vendor.audio.lpi_logging", value, "");
    if (!strncmp("true", value, sizeof("true"))) {
        lpi_logging_prop = true;
    }
#endif
    return (lpi_logging_prop | lpi_logging_);
}

#if defined(ADSP_SLEEP_MONITOR)
int32_t ResourceManager::voteSleepMonitor(Stream *str, bool vote, bool force_nlpi_vote)
{

    int32_t ret = 0;
    int fd = 0;
    pal_stream_type_t type;
    bool lpi_stream = false;
    struct adspsleepmon_ioctl_audio monitor_payload;

    if (sleepmon_fd_ == -1) {
        PAL_ERR(LOG_TAG, "ioctl device is not open");
        return -EINVAL;
    }

    monitor_payload.version = ADSPSLEEPMON_IOCTL_AUDIO_VER_1;
    ret = str->getStreamType(&type);
    if (ret != 0) {
        PAL_ERR(LOG_TAG, "getStreamType failed with status : %d", ret);
        return ret;
    }
    PAL_VERBOSE(LOG_TAG, "Enter for stream type %d", type);

    if (sleep_monitor_vote_type_[type] == AVOID_VOTE) {
        PAL_INFO(LOG_TAG, "Avoiding vote/unvote for stream type : %d", type);
        return ret;
    }

    if (sleep_monitor_vote_type_[type] == LPI_VOTE) {
        lpi_stream = (!force_nlpi_vote && str->ConfigSupportLPI() &&
                      !IsTransitToNonLPIOnChargingSupported());
    }

    mSleepMonitorMutex.lock();
    if (vote) {
        if (lpi_stream) {
            if (++lpi_counter_ == 1) {
                monitor_payload.command = ADSPSLEEPMON_AUDIO_ACTIVITY_LPI_START;
                mSleepMonitorMutex.unlock();
                ret = ioctl(sleepmon_fd_, ADSPSLEEPMON_IOCTL_AUDIO_ACTIVITY, &monitor_payload);
                mSleepMonitorMutex.lock();
            }
        } else {
            if (++nlpi_counter_ == 1) {
                monitor_payload.command = ADSPSLEEPMON_AUDIO_ACTIVITY_START;
                mSleepMonitorMutex.unlock();
                ret = ioctl(sleepmon_fd_, ADSPSLEEPMON_IOCTL_AUDIO_ACTIVITY, &monitor_payload);
                mSleepMonitorMutex.lock();
            }
        }
    } else {
        if (lpi_stream) {
            if (--lpi_counter_ == 0) {
                monitor_payload.command = ADSPSLEEPMON_AUDIO_ACTIVITY_LPI_STOP;
                mSleepMonitorMutex.unlock();
                ret = ioctl(sleepmon_fd_, ADSPSLEEPMON_IOCTL_AUDIO_ACTIVITY, &monitor_payload);
                mSleepMonitorMutex.lock();
            } else if (lpi_counter_ < 0) {
                PAL_ERR(LOG_TAG,
                  "LPI vote count is negative, number of unvotes is more than number of votes");
                lpi_counter_ = 0;
            }
        } else {
            if (--nlpi_counter_ == 0) {
                monitor_payload.command = ADSPSLEEPMON_AUDIO_ACTIVITY_STOP;
                mSleepMonitorMutex.unlock();
                ret = ioctl(sleepmon_fd_, ADSPSLEEPMON_IOCTL_AUDIO_ACTIVITY, &monitor_payload);
                mSleepMonitorMutex.lock();
            } else if(nlpi_counter_ < 0) {
                PAL_ERR(LOG_TAG,
                 "NLPI vote count is negative, number of unvotes is more than number of votes");
                nlpi_counter_ = 0;
            }
        }
    }
    if (ret) {
        PAL_ERR(LOG_TAG, "Failed to %s for %s use case", vote ? "vote" : "unvote",
                         lpi_stream ? "lpi" : "nlpi");
    } else {
        PAL_INFO(LOG_TAG, "%s done for %s use case, lpi votes %d, nlpi votes : %d",
        vote ? "Voting" : "Unvoting", lpi_stream ? "lpi" : "nlpi", lpi_counter_, nlpi_counter_);
    }

    mSleepMonitorMutex.unlock();
    return ret;
}
#else
int32_t ResourceManager::voteSleepMonitor(Stream *str, bool vote, bool force_nlpi_vote)
{
    return 0;
}
#endif

/*
  Playback is going on and charger Insertion occurs, Below
  steps to smooth recovery of FET which avoid its fault.
  1. Wait for 4s to honour usb insertion notification.
  2. Force Device switch from spkr->spkr, to disable PA.
  3. Audio notifies charger driver about concurrency.
  4. StreamDevConnect to enable PA.
  5. Config ICL for low gain for SPKR.
*/
int ResourceManager::handlePBChargerInsertion(Stream *stream)
{
    int status = 0;
    struct pal_device newDevAttr;
    std::shared_ptr<Device> dev = nullptr;

    PAL_DBG(LOG_TAG, "Enter. charger status %d", is_charger_online_);

    if (!stream) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Stream dont exists, status %d", status);
        goto exit;
    }

    /*
     Use below Lock for
      a. Avoid notify PMIC and eventually FET fault when insertion notif.
         comes from HAL thread of USB insertion in mid of this process.
      b. Update charger_online to false which will avoid notifying  PMIC.
    */
    mChargerBoostMutex.lock();
    //TODO handle below varaiable when dispatcher thread comes into picture.
    if (is_charger_online_)
        is_charger_online_ = false;

    //Unlock HAL thread to setparam for enabling insertion notif. Playback.
    mChargerBoostMutex.unlock();

    // Wait for 4s to honour low latency playback.
    sleep(WAIT_LL_PB);

    mChargerBoostMutex.lock();
    //Retain charger_online status to true after notif. PB
    if (!is_charger_online_)
        is_charger_online_ = true;

    newDevAttr.id = PAL_DEVICE_OUT_SPEAKER;
    dev = Device::getInstance(&newDevAttr, rm);

    if (!dev)
        goto unlockChargerBoostMutex;

    status = dev->getDeviceAttributes(&newDevAttr);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Failed to get Device Attribute with status %d", status);
        goto unlockChargerBoostMutex;
    }

    status = forceDeviceSwitch(dev, &newDevAttr);
    if (0 != status)
        PAL_ERR(LOG_TAG, "Failed to do Force Device switch %d", status);

unlockChargerBoostMutex:
    mChargerBoostMutex.unlock();
exit:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

/*
  Playback is going on and charger Removal occurs, Below
  steps avoid HW fault in FET.
  1. Force Device Switch from spkr->spkr to Unvote and Vote for Boost vdd.
*/

int ResourceManager::handlePBChargerRemoval(Stream *stream)
{
    int status = 0;
    struct pal_device newDevAttr;
    std::shared_ptr<Device> dev = nullptr;

    PAL_DBG(LOG_TAG, "Enter. ");

    if (!stream) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Stream dont exists, status %d", status);
        goto exit;
    }

    newDevAttr.id = PAL_DEVICE_OUT_SPEAKER;
    dev = Device::getInstance(&newDevAttr, rm);

    if (!dev)
        goto exit;

    status = dev->getDeviceAttributes(&newDevAttr);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Failed to get Device Attribute with status %d", status);
        goto exit;
    }

    status = forceDeviceSwitch(dev, &newDevAttr);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Failed to do Force Device switch %d", status);
        goto exit;
    }

    status = rm->chargerListenerSetBoostState(false, PB_ON_CHARGER_REMOVE);
exit:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int ResourceManager::handleChargerEvent(Stream *stream, int tag)
{
    int status = 0;
    PAL_DBG(LOG_TAG, "Enter: tag %s", tag == CHARGE_CONCURRENCY_ON_TAG ?
    "CHARGE_CONCURRENCY_ON_TAG" : "CHARGE_CONCURRENCY_OFF_TAG");

    if (!stream) {
        PAL_ERR(LOG_TAG, "No Stream opened");
        status = -EINVAL;
        goto exit;
    }

    if (!is_concurrent_boost_state_ && tag == CHARGE_CONCURRENCY_ON_TAG)
        status = handlePBChargerInsertion(stream);
    else if (is_concurrent_boost_state_ && tag == CHARGE_CONCURRENCY_OFF_TAG)
        status = handlePBChargerRemoval(stream);
    else
        PAL_DBG(LOG_TAG, "Concurrency state unchanged");

    if (0 != status)
        PAL_ERR(LOG_TAG, "Failed to notify PMIC: %d", status);

exit:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int ResourceManager::setSessionParamConfig(uint32_t param_id, Stream *stream, int tag)
{
    int status = 0;
    Session *session = nullptr;
    struct audio_route *audioRoute = nullptr;

    PAL_DBG(LOG_TAG, "Enter param id: %d with tag: %d", param_id, tag);

    if (!stream) {
        PAL_ERR(LOG_TAG, "No Stream opened");
        status = -EINVAL;
        goto exit;
    }
    stream->getAssociatedSession(&session);
    if (!session) {
        PAL_ERR(LOG_TAG, "No associated session for stream exist");
        status = -EINVAL;
        goto exit;
    }

    switch (param_id) {
        case PAL_PARAM_ID_CHARGER_STATE:
        {
            if (!is_concurrent_boost_state_) goto exit;

            status = session->setConfig(stream, MODULE, tag);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to setConfig with status %d", status);
                goto exit;
            }
            is_ICL_config_ = (tag == CHARGE_CONCURRENCY_ON_TAG) ? true : false ;
        }
        break;
        default:
            PAL_DBG(LOG_TAG, "Unknown ParamID:%d", param_id);
            break;
    }

exit:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

void ResourceManager::onChargerListenerStatusChanged(int event_type, int status,
                                                       bool concurrent_state)
{
    int result = 0;
    pal_param_charger_state_t charger_state;
    std::shared_ptr<ResourceManager> rm = nullptr;

    PAL_DBG(LOG_TAG, "Enter: Event: %s, status: %s and concurrent_state: %s",
            event_type ? "Battery": "Charger", status ? "online" : "Offline",
            concurrent_state ? "True" : "False");

    switch (event_type) {
        case CHARGER_EVENT:
            charger_state.is_charger_online =  status ? true : false;
            PAL_DBG(LOG_TAG, "charger status is %s", status ? "Online" : "Offline");
            charger_state.is_concurrent_boost_enable = concurrent_state;
            rm = ResourceManager::getInstance();
            if (rm) {
                result = rm->setParameter(PAL_PARAM_ID_CHARGER_STATE,(void*)&charger_state,
                                          sizeof(pal_param_charger_state_t));
                if (0 != result) {
                    PAL_DBG(LOG_TAG, "Failed to enable audio limiter before charging  %d\n",
                            result);
                }
            }
            break;
        case BATTERY_EVENT:
            break;
        default:
            PAL_ERR(LOG_TAG, "Invalid Uevent_type");
            break;
    }
    PAL_DBG(LOG_TAG, "Exit: call back executed for event: %s is %s",
            event_type ? "Battery": "Charger", result ? "Failed" : "Success");
}

void ResourceManager::chargerListenerInit(charger_status_change_fn_t fn)
{
    if (!fn)
        return;

    cl_lib_handle = dlopen(CL_LIBRARY_PATH, RTLD_NOW);

    if (!cl_lib_handle) {
        PAL_ERR(LOG_TAG, "dlopen for charger_listener failed %s", dlerror());
        return;
    }

    cl_init = (cl_init_t)dlsym(cl_lib_handle, "chargerPropertiesListenerInit");
    cl_deinit = (cl_deinit_t)dlsym(cl_lib_handle, "chargerPropertiesListenerDeinit");
    cl_set_boost_state = (cl_set_boost_state_t)dlsym(cl_lib_handle,
                                 "chargerPropertiesListenerSetBoostState");
    if (!cl_init || !cl_deinit || !cl_set_boost_state) {
        PAL_ERR(LOG_TAG, "dlsym for charger_listener failed");
        goto feature_disabled;
    }
    cl_init(fn);
    return;

feature_disabled:
    if (cl_lib_handle) {
        dlclose(cl_lib_handle);
        cl_lib_handle = NULL;
    }

    cl_init = NULL;
    cl_deinit = NULL;
    cl_set_boost_state = NULL;
    PAL_INFO(LOG_TAG, "---- Feature charger_listener is disabled ----");
}

void ResourceManager::chargerListenerDeinit()
{
    if (cl_deinit)
        cl_deinit();
    if (cl_lib_handle) {
        dlclose(cl_lib_handle);
        cl_lib_handle = NULL;
    }
    cl_init = NULL;
    cl_deinit = NULL;
    cl_set_boost_state = NULL;
}

int ResourceManager::chargerListenerSetBoostState(bool state, charger_boost_mode_t mode)
{
    int status = 0;

    PAL_DBG(LOG_TAG, "Enter: setting concurrency state: %s for usecase mode: %d", state ?
            "True" : "False", mode);

    if (cl_set_boost_state) {
        switch (mode) {
            case CHARGER_ON_PB_STARTS:
                if (!current_concurrent_state_)
                    goto notify_charger;
            break;
            case PB_ON_CHARGER_INSERT:
                if (state && current_concurrent_state_ == 0 &&
                    is_concurrent_boost_state_ == 0) {
                    goto notify_charger;
                }
            break;
            case PB_ON_CHARGER_REMOVE:
                if (!state && current_concurrent_state_ == 0 &&
                    is_concurrent_boost_state_ == 1) {
                    goto notify_charger;
                }
            break;
            case CONCURRENCY_PB_STOPS:
                if (!state && is_concurrent_boost_state_) {
                     is_ICL_config_ = false;
                     goto notify_charger;
                 }
            break;
            default:
                PAL_ERR(LOG_TAG, "Invalid mode to Notify charger");
            break;
        }
        goto exit;
notify_charger:
        status = cl_set_boost_state(state);
        if (0 == status)
            is_concurrent_boost_state_ = state;
        PAL_DBG(LOG_TAG, "Updated Concurrent Boost state is: %s with Setting status: %d",
                 is_concurrent_boost_state_ ? "True" : "False", status);
    }
exit:
    PAL_DBG(LOG_TAG, "Exit:  status %d", status);
    return status;
}

void ResourceManager::chargerListenerFeatureInit()
{
    is_concurrent_boost_state_ = false;
    is_ICL_config_ = false;
    ResourceManager::chargerListenerInit(onChargerListenerStatusChanged);
}

bool ResourceManager::getEcRefStatus(pal_stream_type_t tx_streamtype,pal_stream_type_t rx_streamtype)
{
    bool ecref_status = true;
    if (tx_streamtype == PAL_STREAM_LOW_LATENCY) {
       PAL_DBG(LOG_TAG, "no need to enable ec for tx stream %d", tx_streamtype);
       return false;
    }
    for (int i = 0; i < txEcInfo.size(); i++) {
        if (tx_streamtype == txEcInfo[i].tx_stream_type) {
            for (auto rx_type = txEcInfo[i].disabled_rx_streams.begin();
                  rx_type != txEcInfo[i].disabled_rx_streams.end(); rx_type++) {
               if (rx_streamtype == *rx_type) {
                   ecref_status = false;
                   PAL_DBG(LOG_TAG, "given rx %d disabled %d status %d",rx_streamtype, *rx_type, ecref_status);
                   break;
               }
            }
        }
    }
    return ecref_status;
}

void ResourceManager::getDeviceInfo(pal_device_id_t deviceId, pal_stream_type_t type, std::string key, struct pal_device_info *devinfo)
{
    bool found = false;

    for (int32_t i = 0; i < deviceInfo.size(); i++) {
        if (deviceId == deviceInfo[i].deviceId) {
            devinfo->max_channels = deviceInfo[i].max_channel;
            devinfo->channels = deviceInfo[i].channel;
            devinfo->sndDevName = deviceInfo[i].sndDevName;
            devinfo->samplerate = deviceInfo[i].samplerate;
            devinfo->isExternalECRefEnabledFlag = deviceInfo[i].isExternalECRefEnabled;
            devinfo->isUSBUUIdBasedTuningEnabledFlag = deviceInfo[i].isUSBUUIdBasedTuningEnabled;
            devinfo->bit_width = deviceInfo[i].bit_width;
            devinfo->bitFormatSupported = deviceInfo[i].bitFormatSupported;
            devinfo->channels_overwrite = false;
            devinfo->samplerate_overwrite = false;
            devinfo->sndDevName_overwrite = false;
            devinfo->bit_width_overwrite = false;
            devinfo->fractionalSRSupported = deviceInfo[i].fractionalSRSupported;

            if ((type >= PAL_STREAM_LOW_LATENCY) && (type < PAL_STREAM_MAX))
                devinfo->priority = streamPriorityLUT.at(type);
            else
                devinfo->priority = MIN_USECASE_PRIORITY;

            for (int32_t j = 0; j < deviceInfo[i].usecase.size(); j++) {
                if (type == deviceInfo[i].usecase[j].type) {
                    if (deviceInfo[i].usecase[j].channel) {
                        devinfo->channels = deviceInfo[i].usecase[j].channel;
                        devinfo->channels_overwrite = true;
                        PAL_VERBOSE(LOG_TAG, "getting overwritten channels %d for usecase %d for dev %s",
                                devinfo->channels,
                                type,
                                deviceNameLUT.at(deviceId).c_str());
                    }
                    if (deviceInfo[i].usecase[j].samplerate) {
                        devinfo->samplerate = deviceInfo[i].usecase[j].samplerate;
                        devinfo->samplerate_overwrite = true;
                        PAL_VERBOSE(LOG_TAG, "getting overwritten samplerate %d for usecase %d for dev %s",
                                devinfo->samplerate,
                                type,
                                deviceNameLUT.at(deviceId).c_str());
                    }
                    if (!(deviceInfo[i].usecase[j].sndDevName).empty()) {
                        devinfo->sndDevName = deviceInfo[i].usecase[j].sndDevName;
                        devinfo->sndDevName_overwrite = true;
                        PAL_VERBOSE(LOG_TAG, "getting overwritten snd device name %s for usecase %d for dev %s",
                                devinfo->sndDevName.c_str(),
                                type,
                                deviceNameLUT.at(deviceId).c_str());
                    }
                    if (deviceInfo[i].usecase[j].priority &&
                        deviceInfo[i].usecase[j].priority != MIN_USECASE_PRIORITY) {
                        devinfo->priority = deviceInfo[i].usecase[j].priority;
                        PAL_VERBOSE(LOG_TAG, "getting priority %d for usecase %d for dev %s",
                                devinfo->priority,
                                type,
                                deviceNameLUT.at(deviceId).c_str());
                    }
                    if (deviceInfo[i].usecase[j].bit_width) {
                        devinfo->bit_width = deviceInfo[i].usecase[j].bit_width;
                        devinfo->bit_width_overwrite = true;
                        PAL_VERBOSE(LOG_TAG, "getting overwritten bit width %d for usecase %d for dev %s",
                                devinfo->bit_width,
                                type,
                                deviceNameLUT.at(deviceId).c_str());
                    }
                    /*parse custom config if there*/
                    for (int32_t k = 0; k < deviceInfo[i].usecase[j].config.size(); k++) {
                        if (!deviceInfo[i].usecase[j].config[k].key.compare(key)) {
                            /*overwrite the channels if needed*/
                            if (deviceInfo[i].usecase[j].config[k].channel) {
                                devinfo->channels = deviceInfo[i].usecase[j].config[k].channel;
                                devinfo->channels_overwrite = true;
                                PAL_VERBOSE(LOG_TAG, "got overwritten channels %d for custom key %s usecase %d for dev %s",
                                        devinfo->channels,
                                        key.c_str(),
                                        type,
                                        deviceNameLUT.at(deviceId).c_str());
                            }
                            if (deviceInfo[i].usecase[j].config[k].samplerate) {
                                devinfo->samplerate = deviceInfo[i].usecase[j].config[k].samplerate;
                                devinfo->samplerate_overwrite = true;
                                PAL_VERBOSE(LOG_TAG, "got overwritten samplerate %d for custom key %s usecase %d for dev %s",
                                        devinfo->samplerate,
                                        key.c_str(),
                                        type,
                                        deviceNameLUT.at(deviceId).c_str());
                            }
                            if (!(deviceInfo[i].usecase[j].config[k].sndDevName).empty()) {
                                devinfo->sndDevName = deviceInfo[i].usecase[j].config[k].sndDevName;
                                devinfo->sndDevName_overwrite = true;
                                PAL_VERBOSE(LOG_TAG, "got overwitten snd dev %s for custom key %s usecase %d for dev %s",
                                        devinfo->sndDevName.c_str(),
                                        key.c_str(),
                                        type,
                                        deviceNameLUT.at(deviceId).c_str());
                            }
                            if (deviceInfo[i].usecase[j].config[k].priority &&
                                deviceInfo[i].usecase[j].config[k].priority != MIN_USECASE_PRIORITY) {
                                devinfo->priority = deviceInfo[i].usecase[j].config[k].priority;
                                PAL_VERBOSE(LOG_TAG, "got priority %d for custom key %s usecase %d for dev %s",
                                        devinfo->priority,
                                        key.c_str(),
                                        type,
                                        deviceNameLUT.at(deviceId).c_str());
                            }
                            if (deviceInfo[i].usecase[j].config[k].bit_width) {
                                devinfo->bit_width = deviceInfo[i].usecase[j].config[k].bit_width;
                                devinfo->bit_width_overwrite = true;
                                PAL_VERBOSE(LOG_TAG, "got overwritten bit width %d for custom key %s usecase %d for dev %s",
                                        devinfo->bit_width,
                                        key.c_str(),
                                        type,
                                        deviceNameLUT.at(deviceId).c_str());
                            }
                            found = true;
                            break;
                        }
                    }
                }
            }
        }
    }
}

int32_t ResourceManager::getSidetoneMode(pal_device_id_t deviceId,
                                         pal_stream_type_t type,
                                         sidetone_mode_t *mode){
    int32_t status = 0;

    *mode = SIDETONE_OFF;
    for (int32_t size1 = 0; size1 < deviceInfo.size(); size1++) {
        if (deviceId == deviceInfo[size1].deviceId) {
            for (int32_t size2 = 0; size2 < deviceInfo[size1].usecase.size(); size2++) {
                if (type == deviceInfo[size1].usecase[size2].type) {
                    *mode = deviceInfo[size1].usecase[size2].sidetoneMode;
                    PAL_DBG(LOG_TAG, "found sidetoneMode %d for dev %d", *mode, deviceId);
                    break;
                }
            }
        }
    }
    return status;
}

int32_t ResourceManager::getVolumeSetParamInfo(struct volume_set_param_info *volinfo)
{
    if (!volinfo)
       return 0;

    volinfo->isVolumeUsingSetParam = volumeSetParamInfo_.isVolumeUsingSetParam;

    for (int size = 0; size < volumeSetParamInfo_.streams_.size(); size++) {
        volinfo->streams_.push_back(volumeSetParamInfo_.streams_[size]);
    }

    return 0;
}

int32_t ResourceManager::getDisableLpmInfo(struct disable_lpm_info *lpminfo)
{
    if (!lpminfo)
       return 0;

    lpminfo->isDisableLpm = disableLpmInfo_.isDisableLpm;

    for (int size = 0; size < disableLpmInfo_.streams_.size(); size++) {
        lpminfo->streams_.push_back(disableLpmInfo_.streams_[size]);
    }

    return 0;
}

int32_t ResourceManager::getVsidInfo(struct vsid_info  *info) {
    int status = 0;
    struct vsid_modepair modePair = {};

    info->vsid = vsidInfo.vsid;
#ifdef SEC_AUDIO_LOOPBACK_TEST
    info->loopback_delay = vsidInfo.loopback_mode_delay[vsidInfo.loopback_mode];
#else
    info->loopback_delay = vsidInfo.loopback_delay;
#endif
    for (int size = 0; size < vsidInfo.modepair.size(); size++) {
        modePair.key = vsidInfo.modepair[size].key;
        modePair.value = vsidInfo.modepair[size].value;
        info->modepair.push_back(modePair);
    }
    return status;

}

int ResourceManager::getMaxVoiceVol() {
    return max_voice_vol;
}



void ResourceManager::getChannelMap(uint8_t *channel_map, int channels)
{
    switch (channels) {
    case CHANNELS_1:
       channel_map[0] = PAL_CHMAP_CHANNEL_C;
       break;
    case CHANNELS_2:
       channel_map[0] = PAL_CHMAP_CHANNEL_FL;
       channel_map[1] = PAL_CHMAP_CHANNEL_FR;
       break;
    case CHANNELS_3:
       channel_map[0] = PAL_CHMAP_CHANNEL_FL;
       channel_map[1] = PAL_CHMAP_CHANNEL_FR;
       channel_map[2] = PAL_CHMAP_CHANNEL_C;
       break;
    case CHANNELS_4:
       channel_map[0] = PAL_CHMAP_CHANNEL_FL;
       channel_map[1] = PAL_CHMAP_CHANNEL_FR;
       channel_map[2] = PAL_CHMAP_CHANNEL_LB;
       channel_map[3] = PAL_CHMAP_CHANNEL_RB;
       break;
    case CHANNELS_5:
       channel_map[0] = PAL_CHMAP_CHANNEL_FL;
       channel_map[1] = PAL_CHMAP_CHANNEL_FR;
       channel_map[2] = PAL_CHMAP_CHANNEL_C;
       channel_map[3] = PAL_CHMAP_CHANNEL_LB;
       channel_map[4] = PAL_CHMAP_CHANNEL_RB;
       break;
    case CHANNELS_6:
       channel_map[0] = PAL_CHMAP_CHANNEL_FL;
       channel_map[1] = PAL_CHMAP_CHANNEL_FR;
       channel_map[2] = PAL_CHMAP_CHANNEL_C;
       channel_map[3] = PAL_CHMAP_CHANNEL_LFE;
       channel_map[4] = PAL_CHMAP_CHANNEL_LB;
       channel_map[5] = PAL_CHMAP_CHANNEL_RB;
       break;
    case CHANNELS_7:
       channel_map[0] = PAL_CHMAP_CHANNEL_FL;
       channel_map[1] = PAL_CHMAP_CHANNEL_FR;
       channel_map[2] = PAL_CHMAP_CHANNEL_C;
       channel_map[3] = PAL_CHMAP_CHANNEL_LFE;
       channel_map[4] = PAL_CHMAP_CHANNEL_LB;
       channel_map[5] = PAL_CHMAP_CHANNEL_RB;
       channel_map[6] = PAL_CHMAP_CHANNEL_RC;
       break;
    case CHANNELS_8:
       channel_map[0] = PAL_CHMAP_CHANNEL_FL;
       channel_map[1] = PAL_CHMAP_CHANNEL_FR;
       channel_map[2] = PAL_CHMAP_CHANNEL_C;
       channel_map[3] = PAL_CHMAP_CHANNEL_LFE;
       channel_map[4] = PAL_CHMAP_CHANNEL_LB;
       channel_map[5] = PAL_CHMAP_CHANNEL_RB;
       channel_map[6] = PAL_CHMAP_CHANNEL_LS;
       channel_map[7] = PAL_CHMAP_CHANNEL_RS;
       break;
   }
}

pal_audio_fmt_t ResourceManager::getAudioFmt(uint32_t bitWidth)
{
    return bitWidthToFormat.at(bitWidth);
}

int32_t ResourceManager::getDeviceConfig(struct pal_device *deviceattr,
                                         struct pal_stream_attributes *sAttr)
{
    int32_t status = 0;
    struct pal_channel_info dev_ch_info;
    bool is_wfd_in_progress = false;
    struct pal_stream_attributes tx_attr;
    struct pal_device_info devinfo = {};

    if (!deviceattr) {
        PAL_ERR(LOG_TAG, "Invalid deviceattr");
        return -EINVAL;
    }

    if (sAttr != NULL)
        getDeviceInfo(deviceattr->id, sAttr->type,
                      deviceattr->custom_config.custom_key, &devinfo);
    else
        /* For NULL sAttr set default samplerate */
        getDeviceInfo(deviceattr->id, (pal_stream_type_t)0,
                      deviceattr->custom_config.custom_key, &devinfo);

    /* set snd device name */
    strlcpy(deviceattr->sndDevName, devinfo.sndDevName.c_str(), DEVICE_NAME_MAX_SIZE);

    /*set channels*/
    if (devinfo.channels == 0 || devinfo.channels > devinfo.max_channels) {
        PAL_ERR(LOG_TAG, "Invalid num channels[%d], max channels[%d] failed to create stream",
                    devinfo.channels,
                    devinfo.max_channels);
        status = -EINVAL;
        goto exit;
    }
    dev_ch_info.channels = devinfo.channels;
    getChannelMap(&(dev_ch_info.ch_map[0]), devinfo.channels);
    deviceattr->config.ch_info = dev_ch_info;

    /*set proper sample rate*/
    if (devinfo.samplerate) {
        deviceattr->config.sample_rate = devinfo.samplerate;
    } else {
        deviceattr->config.sample_rate = ((sAttr == NULL) ?  SAMPLINGRATE_48K :
                    (sAttr->direction == PAL_AUDIO_INPUT) ? sAttr->in_media_config.sample_rate : sAttr->out_media_config.sample_rate);
    }
    /*set proper bit width*/
    if (devinfo.bit_width) {
        deviceattr->config.bit_width = devinfo.bit_width;
    } /*if default is not set in resourcemanager.xml use from stream*/
    else {
        deviceattr->config.bit_width = ((sAttr == NULL) ?  BITWIDTH_16 :
                    (sAttr->direction == PAL_AUDIO_INPUT) ? sAttr->in_media_config.bit_width : sAttr->out_media_config.bit_width);
        if (deviceattr->config.bit_width == BITWIDTH_32 && deviceattr->id == PAL_DEVICE_OUT_SPEAKER) {
            if (devinfo.bitFormatSupported != PAL_AUDIO_FMT_PCM_S32_LE) {
                PAL_DBG(LOG_TAG, "32 bit is not supported, hence update with supported bit format");
                deviceattr->config.aud_fmt_id = devinfo.bitFormatSupported;
                deviceattr->config.bit_width = palFormatToBitwidthLookup(devinfo.bitFormatSupported);
            }
        }
    }
    if (!isBitWidthSupported(deviceattr->config.bit_width))
        deviceattr->config.bit_width = BITWIDTH_16;

    deviceattr->config.aud_fmt_id = bitWidthToFormat.at(deviceattr->config.bit_width);

#ifdef SEC_AUDIO_COMMON
    /*special case if PAL_AUDIO_FMT_PCM_S24_3LE is requested*/
    if (devinfo.bitFormatSupported == PAL_AUDIO_FMT_PCM_S24_3LE) {
        deviceattr->config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S24_3LE;
    }
#endif

    if ((sAttr != NULL) && (sAttr->direction == PAL_AUDIO_INPUT) &&
            (deviceattr->config.bit_width == BITWIDTH_32)) {
        PAL_INFO(LOG_TAG, "update i/p bitwidth stream from 32b to max supported 24b");
        deviceattr->config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S24_LE;
        deviceattr->config.bit_width = BITWIDTH_24;
    }

    /*special cases to update attrs for hot plug devices*/
    switch (deviceattr->id) {
        case PAL_DEVICE_IN_WIRED_HEADSET:
            status = (HeadsetMic::checkAndUpdateSampleRate(&deviceattr->config.sample_rate));
            if (status) {
                PAL_ERR(LOG_TAG, "failed to update samplerate/bitwidth");
                status = -EINVAL;
            }
            break;
        case PAL_DEVICE_OUT_WIRED_HEADPHONE:
        case PAL_DEVICE_OUT_WIRED_HEADSET:
            status = (Headphone::checkAndUpdateBitWidth(&deviceattr->config.bit_width) |
                Headphone::checkAndUpdateSampleRate(&deviceattr->config.sample_rate));
            if (status) {
                PAL_ERR(LOG_TAG, "failed to update samplerate/bitwidth");
                status = -EINVAL;
            }
            break;
        case PAL_DEVICE_OUT_BLUETOOTH_A2DP:
        case PAL_DEVICE_IN_BLUETOOTH_A2DP:
        case PAL_DEVICE_OUT_BLUETOOTH_BLE:
        case PAL_DEVICE_IN_BLUETOOTH_BLE:
        case PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST:
            /*overwride format for a2dp*/
            deviceattr->config.aud_fmt_id = PAL_AUDIO_FMT_DEFAULT_COMPRESSED;
            break;
        case PAL_DEVICE_OUT_BLUETOOTH_SCO:
        case PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET:
            {
                std::shared_ptr<BtSco> scoDev;
                scoDev = std::dynamic_pointer_cast<BtSco>(BtSco::getInstance(deviceattr, rm));
                if (!scoDev) {
                    PAL_ERR(LOG_TAG, "failed to get BtSco singleton object.");
                    return -EINVAL;
                }
                // update device sample rate based on sco mode
                scoDev->updateSampleRate(&deviceattr->config.sample_rate);
                PAL_DBG(LOG_TAG, "BT SCO device samplerate %d, bitwidth %d",
                      deviceattr->config.sample_rate, deviceattr->config.bit_width);
            }
            break;
        case PAL_DEVICE_OUT_USB_DEVICE:
        case PAL_DEVICE_OUT_USB_HEADSET:
            {
                if (!sAttr) {
                    PAL_ERR(LOG_TAG, "Invalid parameter.");
                    return -EINVAL;
                }
                // config.ch_info memory is allocated in selectBestConfig below
                std::shared_ptr<USB> USB_out_device;
                USB_out_device = std::dynamic_pointer_cast<USB>(USB::getInstance(deviceattr, rm));
                if (!USB_out_device) {
                    PAL_ERR(LOG_TAG, "failed to get USB singleton object.");
                    return -EINVAL;
                }
                status = USB_out_device->selectBestConfig(deviceattr, sAttr, true, &devinfo);
                deviceattr->config.aud_fmt_id = bitWidthToFormat.at(deviceattr->config.bit_width);
                if (deviceattr->config.bit_width == BITWIDTH_24) {
                    if (devinfo.bitFormatSupported == PAL_AUDIO_FMT_PCM_S24_LE)
                        deviceattr->config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S24_LE;
                    else
                        deviceattr->config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S24_3LE;
                }
            }
            break;
        case PAL_DEVICE_IN_USB_DEVICE:
        case PAL_DEVICE_IN_USB_HEADSET:
            {
                if (!sAttr) {
                    PAL_ERR(LOG_TAG, "Invalid parameter.");
                    return -EINVAL;
                }
                std::shared_ptr<USB> USB_in_device;
                USB_in_device = std::dynamic_pointer_cast<USB>(USB::getInstance(deviceattr, rm));
                if (!USB_in_device) {
                    PAL_ERR(LOG_TAG, "failed to get USB singleton object.");
                    return -EINVAL;
                }
                USB_in_device->selectBestConfig(deviceattr, sAttr, false, &devinfo);
                /*Update aud_fmt_id based on the selected bitwidth*/
                deviceattr->config.aud_fmt_id = bitWidthToFormat.at(deviceattr->config.bit_width);
                if (deviceattr->config.bit_width == BITWIDTH_24) {
                    if (devinfo.bitFormatSupported == PAL_AUDIO_FMT_PCM_S24_LE)
                        deviceattr->config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S24_LE;
                    else
                        deviceattr->config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S24_3LE;
                }
            }
            break;
        case PAL_DEVICE_IN_PROXY:
        case PAL_DEVICE_IN_FM_TUNER:
        case PAL_DEVICE_IN_RECORD_PROXY:
            {
            /* For PAL_DEVICE_IN_FM_TUNER/PAL_DEVICE_IN_PROXY, copy all config from stream attributes */
            if (!sAttr) {
                PAL_ERR(LOG_TAG, "Invalid parameter.");
                return -EINVAL;
            }

            struct pal_media_config *candidateConfig = &sAttr->in_media_config;
            PAL_DBG(LOG_TAG, "sattr chn=0x%x fmt id=0x%x rate = 0x%x width=0x%x",
                sAttr->in_media_config.ch_info.channels,
                sAttr->in_media_config.aud_fmt_id,
                sAttr->in_media_config.sample_rate,
                sAttr->in_media_config.bit_width);


            if (!ifVoiceorVoipCall(sAttr->type) && isDeviceAvailable(PAL_DEVICE_OUT_PROXY)) {
                PAL_DBG(LOG_TAG, "This is NOT voice call. out proxy is available");
                std::shared_ptr<Device> devOut = nullptr;
                struct pal_device proxyOut_dattr;
                proxyOut_dattr.id = PAL_DEVICE_OUT_PROXY;
                devOut = Device::getInstance(&proxyOut_dattr, rm);
                if (devOut) {
                    status = devOut->getDeviceAttributes(&proxyOut_dattr);
                    if (status) {
                        PAL_ERR(LOG_TAG, "getDeviceAttributes for OUT_PROXY failed %d", status);
                        break;
                    }

                    if (proxyOut_dattr.config.ch_info.channels &&
                            proxyOut_dattr.config.sample_rate) {
                        PAL_INFO(LOG_TAG, "proxy out attr is used");
                        candidateConfig = &proxyOut_dattr.config;
                    }
                }
            }

            deviceattr->config.ch_info = candidateConfig->ch_info;
            if (isPalPCMFormat(candidateConfig->aud_fmt_id))
                deviceattr->config.bit_width =
                          palFormatToBitwidthLookup(candidateConfig->aud_fmt_id);
            else
                deviceattr->config.bit_width = candidateConfig->bit_width;

            deviceattr->config.aud_fmt_id = candidateConfig->aud_fmt_id;
            deviceattr->config.sample_rate = candidateConfig->sample_rate;

            PAL_INFO(LOG_TAG, "in proxy chn=0x%x fmt id=0x%x rate = 0x%x width=0x%x",
                        deviceattr->config.ch_info.channels,
                        deviceattr->config.aud_fmt_id,
                        deviceattr->config.sample_rate,
                        deviceattr->config.bit_width);
            }
            break;
        case PAL_DEVICE_OUT_PROXY:
        case PAL_DEVICE_OUT_RECORD_PROXY:
            {
            if (!sAttr) {
                PAL_ERR(LOG_TAG, "Invalid parameter.");
                return -EINVAL;
            }
            // check if wfd session in progress
            for (auto& tx_str: mActiveStreams) {
                tx_str->getStreamAttributes(&tx_attr);
                if (tx_attr.direction == PAL_AUDIO_INPUT &&
                    tx_attr.info.opt_stream_info.tx_proxy_type == PAL_STREAM_PROXY_TX_WFD) {
                    is_wfd_in_progress = true;
                    break;
                }
            }

            if (is_wfd_in_progress)
            {
                PAL_INFO(LOG_TAG, "wfd TX is in progress");
                std::shared_ptr<Device> dev = nullptr;
                struct pal_device proxyIn_dattr;
                proxyIn_dattr.id = PAL_DEVICE_IN_PROXY;
                dev = Device::getInstance(&proxyIn_dattr, rm);
                if (dev) {
                    status = dev->getDeviceAttributes(&proxyIn_dattr);
                    if (status) {
                        PAL_ERR(LOG_TAG, "OUT_PROXY getDeviceAttributes failed %d", status);
                        break;
                    }
                    deviceattr->config.ch_info = proxyIn_dattr.config.ch_info;
                    deviceattr->config.sample_rate = proxyIn_dattr.config.sample_rate;
                    if (isPalPCMFormat(proxyIn_dattr.config.aud_fmt_id))
                        deviceattr->config.bit_width =
                              palFormatToBitwidthLookup(proxyIn_dattr.config.aud_fmt_id);
                    else
                        deviceattr->config.bit_width = proxyIn_dattr.config.bit_width;

                    deviceattr->config.aud_fmt_id = proxyIn_dattr.config.aud_fmt_id;
                }
            }
            else
            {
                PAL_INFO(LOG_TAG, "wfd TX is not in progress");
                if (rm->num_proxy_channels) {
                    PAL_INFO(LOG_TAG, "proxy device channel number: %d", rm->num_proxy_channels);
                    deviceattr->config.ch_info.channels = rm->num_proxy_channels;
                }
            }
            PAL_INFO(LOG_TAG, "PAL_DEVICE_OUT_PROXY sample rate %d bitwidth %d ch:%d",
                    deviceattr->config.sample_rate, deviceattr->config.bit_width,
                    deviceattr->config.ch_info.channels);
            }
            break;
        case PAL_DEVICE_OUT_AUX_DIGITAL:
        case PAL_DEVICE_OUT_AUX_DIGITAL_1:
        case PAL_DEVICE_OUT_HDMI:
            {
                std::shared_ptr<DisplayPort> dp_device;
                dp_device = std::dynamic_pointer_cast<DisplayPort>
                                    (DisplayPort::getInstance(deviceattr, rm));
                if (!dp_device) {
                    PAL_ERR(LOG_TAG, "Failed to get DisplayPort object.");
                    return -EINVAL;
                }

                if (!sAttr) {
                    PAL_ERR(LOG_TAG, "Invalid parameter.");
                    return -EINVAL;
                }
                /**
                 * Comparision of stream channel and device supported max channel.
                 * If stream channel is less than or equal to device supported
                 * channel then the channel of stream is taken othewise it is of
                 * device
                 */
                int channels = dp_device->getMaxChannel();

                if (channels > sAttr->out_media_config.ch_info.channels)
                    channels = sAttr->out_media_config.ch_info.channels;

                /**
                 * According to HDMI spec CEA-861-E, 1 channel is not
                 * supported, thus converting 1 channel to 2 channels.
                 */
                if (channels == 1)
                    channels = 2;

                dev_ch_info.channels = channels;

                getChannelMap(&(dev_ch_info.ch_map[0]), channels);
                deviceattr->config.ch_info = dev_ch_info;
#ifdef SEC_AUDIO_SUPPORT_UHQ
                int target_sample_rate = deviceattr->config.sample_rate;
                if ((sAttr->type == PAL_STREAM_DEEP_BUFFER)
                        || (sAttr->type == PAL_STREAM_COMPRESSED)) {
                    target_sample_rate = rm->stateUHQA;
                }
                if (dp_device->isSupportedSR(NULL, target_sample_rate)) {
                    deviceattr->config.sample_rate = target_sample_rate;
                } else
#else
                if (!dp_device->isSupportedSR(NULL,
                            deviceattr->config.sample_rate))
#endif
                {
                    deviceattr->config.sample_rate = dp_device->getHighestSupportedSR();

                    if (sAttr->out_media_config.sample_rate < SAMPLINGRATE_32K &&
                        (sAttr->out_media_config.sample_rate % 11025) == 0 &&
                        dp_device->isSupportedSR(NULL,SAMPLINGRATE_44K)) {
                            deviceattr->config.sample_rate = SAMPLINGRATE_44K;
                    }
                }

                if (DisplayPort::isBitWidthSupported(
                            deviceattr->config.bit_width) != 0) {
                    int bps = dp_device->getHighestSupportedBps();
                    if (sAttr->out_media_config.bit_width > bps)
                        deviceattr->config.bit_width = bps;
                    else
                        deviceattr->config.bit_width = BITWIDTH_16;
                }
                if ((deviceattr->config.bit_width == BITWIDTH_32) &&
                            (devinfo.bitFormatSupported != PAL_AUDIO_FMT_PCM_S32_LE)) {
                    PAL_DBG(LOG_TAG, "32 bit is not supported; update with supported bit format");
                    deviceattr->config.aud_fmt_id = devinfo.bitFormatSupported;
                    deviceattr->config.bit_width =
                            palFormatToBitwidthLookup(devinfo.bitFormatSupported);
                } else {
                    if (deviceattr->config.bit_width == 32) {
                        deviceattr->config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S32_LE;
                    } else if (deviceattr->config.bit_width == 24) {
                        if (sAttr->out_media_config.aud_fmt_id == PAL_AUDIO_FMT_PCM_S24_LE)
                            deviceattr->config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S24_LE;
                        else
                            deviceattr->config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S24_3LE;
                    } else {
                        deviceattr->config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
                    }
                }

                PAL_DBG(LOG_TAG, "devcie %d sample rate %d bitwidth %d",
                        deviceattr->id, deviceattr->config.sample_rate,
                        deviceattr->config.bit_width);
            }
            break;
        case PAL_DEVICE_OUT_HAPTICS_DEVICE:
        case PAL_DEVICE_OUT_HEARING_AID:
            /* For PAL_DEVICE_OUT_HAPTICS_DEVICE, copy all config from stream attributes */
            if (!sAttr) {
                PAL_ERR(LOG_TAG, "Invalid parameter.");
                return -EINVAL;
            }
            if (deviceattr->id == PAL_DEVICE_OUT_HAPTICS_DEVICE &&
                                 ResourceManager::isHapticsthroughWSA)
                goto exit;

            deviceattr->config.ch_info = sAttr->out_media_config.ch_info;
            deviceattr->config.bit_width = sAttr->out_media_config.bit_width;
            deviceattr->config.aud_fmt_id = sAttr->out_media_config.aud_fmt_id;
            PAL_DBG(LOG_TAG, "devcie %d sample rate %d bitwidth %d",
                    deviceattr->id,deviceattr->config.sample_rate,
                    deviceattr->config.bit_width);
            break;
        case PAL_DEVICE_OUT_ULTRASOUND:
        case PAL_DEVICE_OUT_ULTRASOUND_DEDICATED:
            /* copy all config from stream attributes for sensor renderer */
            if (!sAttr) {
                PAL_ERR(LOG_TAG, "Invalid parameter.");
                return -EINVAL;
            }
            if (sAttr->type != PAL_STREAM_SENSOR_PCM_RENDERER)
                goto exit;

            deviceattr->config.sample_rate = sAttr->out_media_config.sample_rate;
            deviceattr->config.ch_info = sAttr->out_media_config.ch_info;
            deviceattr->config.bit_width = sAttr->out_media_config.bit_width;
            deviceattr->config.aud_fmt_id = bitWidthToFormat.at(sAttr->out_media_config.bit_width);
            PAL_DBG(LOG_TAG, "devcie %d sample rate %d bitwidth %d",
                    deviceattr->id, deviceattr->config.sample_rate,
                    deviceattr->config.bit_width);
            break;
#ifdef SEC_AUDIO_DSM_AMP
        case PAL_DEVICE_IN_VI_FEEDBACK:
#ifdef SEC_AUDIO_VI_FEEDBACK
            deviceattr->config.bit_width = BITWIDTH_24;
            deviceattr->config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S24_LE;
#else
            deviceattr->config.bit_width = BITWIDTH_16;
            deviceattr->config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
#endif
            break;
#endif
        default:
            //do nothing for rest of the devices
            break;
    }
exit:
    PAL_DBG(LOG_TAG, "device id 0x%x channels %d samplerate %d, bitwidth %d format %d SndDev %s priority 0x%x",
            deviceattr->id, deviceattr->config.ch_info.channels, deviceattr->config.sample_rate,
            deviceattr->config.bit_width, deviceattr->config.aud_fmt_id,
            devinfo.sndDevName.c_str(), devinfo.priority);
    return status;
}

bool ResourceManager::isStreamSupported(struct pal_stream_attributes *attributes,
                                        struct pal_device *devices, int no_of_devices)
{
    bool result = false;
    uint16_t channels;
    uint32_t samplerate, bitwidth;
    uint32_t rc;
    size_t cur_sessions = 0;
    size_t max_sessions = 0;

    if (!attributes || ((no_of_devices > 0) && !devices && (attributes->type != PAL_STREAM_VOICE_CALL_MUSIC)
                         && (attributes->type != PAL_STREAM_VOICE_CALL_RECORD))) {
        PAL_ERR(LOG_TAG, "Invalid input parameter attr %p, noOfDevices %d devices %p",
                attributes, no_of_devices, devices);
        return result;
    }

    // check if stream type is supported
    // and new stream session is allowed
    pal_stream_type_t type = attributes->type;
    PAL_DBG(LOG_TAG, "Enter. type %d", type);
    switch (type) {
        case PAL_STREAM_LOW_LATENCY:
        case PAL_STREAM_VOIP:
        case PAL_STREAM_VOIP_RX:
        case PAL_STREAM_VOIP_TX:
            cur_sessions = active_streams_ll.size();
            max_sessions = MAX_SESSIONS_LOW_LATENCY;
            break;
        case PAL_STREAM_ULTRA_LOW_LATENCY:
            cur_sessions = active_streams_ull.size();
            max_sessions = MAX_SESSIONS_ULTRA_LOW_LATENCY;
            break;
        case PAL_STREAM_DEEP_BUFFER:
            cur_sessions = active_streams_db.size();
            max_sessions = MAX_SESSIONS_DEEP_BUFFER;
            break;
        case PAL_STREAM_SPATIAL_AUDIO:
            cur_sessions = active_streams_sa.size();
            max_sessions = MAX_SESSIONS_SPATIAL_AUDIO;
            break;
        case PAL_STREAM_COMPRESSED:
            cur_sessions = active_streams_comp.size();
            max_sessions = MAX_SESSIONS_COMPRESSED;
            break;
        case PAL_STREAM_GENERIC:
            cur_sessions = active_streams_ulla.size();
            max_sessions = MAX_SESSIONS_GENERIC;
            break;
        case PAL_STREAM_RAW:
            cur_sessions = active_streams_raw.size();
            max_sessions = MAX_SESSIONS_RAW;
            break;
        case PAL_STREAM_VOICE_RECOGNITION:
            cur_sessions = active_streams_voice_rec.size();
            max_sessions = MAX_SESSIONS_VOICE_RECOGNITION;
            break;
        case PAL_STREAM_LOOPBACK:
        case PAL_STREAM_TRANSCODE:
        case PAL_STREAM_VOICE_UI:
            cur_sessions = active_streams_st.size();
            max_sessions = MAX_SESSIONS_VOICE_UI;
            break;
        case PAL_STREAM_ACD:
            cur_sessions = active_streams_acd.size();
            max_sessions = MAX_SESSIONS_ACD;
            break;
        case PAL_STREAM_ASR:
            cur_sessions = active_streams_asr.size();
            max_sessions = MAX_SESSIONS_ASR;
            break;
        case PAL_STREAM_PCM_OFFLOAD:
            cur_sessions = active_streams_po.size();
            max_sessions = MAX_SESSIONS_PCM_OFFLOAD;
            break;
        case PAL_STREAM_PROXY:
            cur_sessions = active_streams_proxy.size();
            max_sessions = MAX_SESSIONS_PROXY;
            break;
         case PAL_STREAM_VOICE_CALL:
            break;
        case PAL_STREAM_VOICE_CALL_MUSIC:
            cur_sessions = active_streams_incall_music.size();
            max_sessions = MAX_SESSIONS_INCALL_MUSIC;
            break;
        case PAL_STREAM_VOICE_CALL_RECORD:
            cur_sessions = active_streams_incall_record.size();
            max_sessions = MAX_SESSIONS_INCALL_RECORD;
            break;
        case PAL_STREAM_NON_TUNNEL:
            cur_sessions = active_streams_non_tunnel.size();
            max_sessions = max_nt_sessions;
            break;
        case PAL_STREAM_HAPTICS:
            cur_sessions = active_streams_haptics.size();
            max_sessions = MAX_SESSIONS_HAPTICS;
            break;
        case PAL_STREAM_CONTEXT_PROXY:
        case PAL_STREAM_SENSOR_PCM_DATA:
        case PAL_STREAM_COMMON_PROXY:
            return true;
        case PAL_STREAM_ULTRASOUND:
            cur_sessions = active_streams_ultrasound.size();
            max_sessions = MAX_SESSIONS_ULTRASOUND;
            break;
        case PAL_STREAM_SENSOR_PCM_RENDERER:
            cur_sessions = active_streams_sensor_renderer.size();
            max_sessions = MAX_SESSIONS_SENSOR_PLAYBACK;
            break;
        default:
            PAL_ERR(LOG_TAG, "Invalid stream type = %d", type);
        return result;
    }
    if (cur_sessions == max_sessions && type != PAL_STREAM_VOICE_CALL) {
        if (type == PAL_STREAM_VOICE_RECOGNITION &&
            active_streams_db.size() < MAX_SESSIONS_DEEP_BUFFER) {
                attributes->type = PAL_STREAM_DEEP_BUFFER;
                type = PAL_STREAM_DEEP_BUFFER;
        } else {
            PAL_ERR(LOG_TAG, "no new session allowed for stream %d", type);
            return result;
        }
    }

    if (type == PAL_STREAM_HAPTICS) {
        struct pal_device hapticsDattr;
        std::shared_ptr<Device> hapticsDev = nullptr;
        std::vector <Stream *> activeStreams;
        struct pal_stream_attributes ActivesAttr;
        Stream *stream = NULL;
        hapticsDattr.id = PAL_DEVICE_OUT_HAPTICS_DEVICE;
        hapticsDev = Device::getInstance(&hapticsDattr, rm);
        getActiveStream_l(activeStreams, hapticsDev);
        for (int i = 0;i < activeStreams.size(); i++) {
             stream = static_cast<Stream *>(activeStreams[i]);
             stream->getStreamAttributes(&ActivesAttr);
             if (ActivesAttr.info.opt_stream_info.haptics_type == PAL_STREAM_HAPTICS_RINGTONE) {
                 PAL_INFO(LOG_TAG, "Ringtone Haptics is Active skipping Touch Haptics");
                 return result;
             }
        }
    }

    // check if param supported by audio configruation
    switch (type) {
        case PAL_STREAM_VOICE_CALL_RECORD:
        case PAL_STREAM_LOW_LATENCY:
        case PAL_STREAM_ULTRA_LOW_LATENCY:
        case PAL_STREAM_DEEP_BUFFER:
        case PAL_STREAM_SPATIAL_AUDIO:
        case PAL_STREAM_GENERIC:
        case PAL_STREAM_VOIP:
        case PAL_STREAM_VOIP_RX:
        case PAL_STREAM_VOIP_TX:
        case PAL_STREAM_PCM_OFFLOAD:
        case PAL_STREAM_LOOPBACK:
        case PAL_STREAM_PROXY:
        case PAL_STREAM_VOICE_CALL_MUSIC:
        case PAL_STREAM_VOICE_RECOGNITION:
            if (attributes->direction == PAL_AUDIO_INPUT) {
                channels = attributes->in_media_config.ch_info.channels;
                samplerate = attributes->in_media_config.sample_rate;
                bitwidth = attributes->in_media_config.bit_width;
            } else {
                channels = attributes->out_media_config.ch_info.channels;
                samplerate = attributes->out_media_config.sample_rate;
                bitwidth = attributes->out_media_config.bit_width;
            }
            rc = (StreamPCM::isBitWidthSupported(bitwidth) |
                  StreamPCM::isSampleRateSupported(samplerate) |
                  StreamPCM::isChannelSupported(channels));
            if (0 != rc) {
               PAL_ERR(LOG_TAG, "config not supported rc %d", rc);
               return result;
            }
            PAL_VERBOSE(LOG_TAG, "config suppported");
            result = true;
            break;
        case PAL_STREAM_RAW:
            if (attributes->direction == PAL_AUDIO_INPUT) {
               channels = attributes->in_media_config.ch_info.channels;
               samplerate = attributes->in_media_config.sample_rate;
               bitwidth = attributes->in_media_config.bit_width;
            } else {
                channels = attributes->out_media_config.ch_info.channels;
                samplerate = attributes->out_media_config.sample_rate;
                bitwidth = attributes->out_media_config.bit_width;
            }
            rc = (StreamPCM::isBitWidthSupported(bitwidth) |
                  StreamPCM::isSampleRateSupported(samplerate) |
                  StreamPCM::isChannelSupported(channels));
            if (0 != rc) {
               PAL_ERR(LOG_TAG, "config not supported rc %d", rc);
               return result;
            }
            PAL_VERBOSE(LOG_TAG, "config suppported");
            result = true;
            break;
        case PAL_STREAM_COMPRESSED:
            if (attributes->direction == PAL_AUDIO_INPUT) {
               channels = attributes->in_media_config.ch_info.channels;
               samplerate = attributes->in_media_config.sample_rate;
               bitwidth = attributes->in_media_config.bit_width;
            } else {
               channels = attributes->out_media_config.ch_info.channels;
               samplerate = attributes->out_media_config.sample_rate;
               bitwidth = attributes->out_media_config.bit_width;
            }
            rc = (StreamCompress::isBitWidthSupported(bitwidth) |
                  StreamCompress::isSampleRateSupported(samplerate) |
                  StreamCompress::isChannelSupported(channels));
            if (0 != rc) {
               PAL_ERR(LOG_TAG, "config not supported rc %d", rc);
               return result;
            }
            result = true;
            break;
        case PAL_STREAM_VOICE_UI:
            if (attributes->direction == PAL_AUDIO_INPUT) {
               channels = attributes->in_media_config.ch_info.channels;
               samplerate = attributes->in_media_config.sample_rate;
               bitwidth = attributes->in_media_config.bit_width;
            } else {
               channels = attributes->out_media_config.ch_info.channels;
               samplerate = attributes->out_media_config.sample_rate;
               bitwidth = attributes->out_media_config.bit_width;
            }
            rc = (StreamSoundTrigger::isBitWidthSupported(bitwidth) |
                  StreamSoundTrigger::isSampleRateSupported(samplerate) |
                  StreamSoundTrigger::isChannelSupported(channels));
            if (0 != rc) {
               PAL_ERR(LOG_TAG, "config not supported rc %d", rc);
               return result;
            }
            PAL_VERBOSE(LOG_TAG, "config suppported");
            result = true;
            break;
        case PAL_STREAM_ASR:
            if (attributes->direction != PAL_AUDIO_INPUT) {
                return result;
            }
            channels = attributes->in_media_config.ch_info.channels;
            samplerate = attributes->in_media_config.sample_rate;
            bitwidth = attributes->in_media_config.bit_width;

            rc = (StreamASR::isBitWidthSupported(bitwidth) |
                  StreamASR::isSampleRateSupported(samplerate) |
                  StreamASR::isChannelSupported(channels));
            if (0 != rc) {
                PAL_ERR(LOG_TAG, "Config not supported rc: %d", rc);
                return result;
            }
            PAL_VERBOSE(LOG_TAG, "config suppported");
            result = true;
            break;
        case PAL_STREAM_VOICE_CALL:
            channels = attributes->out_media_config.ch_info.channels;
            samplerate = attributes->out_media_config.sample_rate;
            bitwidth = attributes->out_media_config.bit_width;
            rc = (StreamPCM::isBitWidthSupported(bitwidth) |
                  StreamPCM::isSampleRateSupported(samplerate) |
                  StreamPCM::isChannelSupported(channels));
            if (0 != rc) {
               PAL_ERR(LOG_TAG, "config not supported rc %d", rc);
               return result;
            }
            PAL_VERBOSE(LOG_TAG, "config suppported");
            result = true;
            break;
        case PAL_STREAM_NON_TUNNEL:
            if (attributes->direction != PAL_AUDIO_INPUT_OUTPUT) {
               PAL_ERR(LOG_TAG, "config dir %d not supported", attributes->direction);
               return result;
            }
            PAL_VERBOSE(LOG_TAG, "config suppported");
            result = true;
            break;
        case PAL_STREAM_ACD:
        case PAL_STREAM_ULTRASOUND:
        case PAL_STREAM_SENSOR_PCM_DATA:
        case PAL_STREAM_SENSOR_PCM_RENDERER:
             result = true;
             break;
        case PAL_STREAM_HAPTICS:
            if (attributes->info.opt_stream_info.haptics_type == PAL_STREAM_HAPTICS_TOUCH)
                result = true;
            else {
                channels = attributes->out_media_config.ch_info.channels;
                samplerate = attributes->out_media_config.sample_rate;
                bitwidth = attributes->out_media_config.bit_width;
                rc = (StreamPCM::isBitWidthSupported(bitwidth) |
                      StreamPCM::isSampleRateSupported(samplerate) |
                      StreamPCM::isChannelSupported(channels));
                if (0 != rc) {
                    PAL_ERR(LOG_TAG, "config not supported rc %d", rc);
                    return result;
                }
                PAL_VERBOSE(LOG_TAG, "config suppported");
                result = true;
            }
            break;
        default:
            PAL_ERR(LOG_TAG, "unknown type");
            return false;
    }
    PAL_DBG(LOG_TAG, "Exit. result %d", result);
    return result;
}

template <class T>
int registerstream(T s, std::list<T> &streams)
{
    int ret = 0;

    if(NULL != s)
        streams.push_back(s);
    else
        ret = -EINVAL;

    return ret;
}

int ResourceManager::registerStream(Stream *s)
{
    int ret = 0;
    pal_stream_type_t type;
    PAL_DBG(LOG_TAG, "Enter. stream %pK", s);
    ret = s->getStreamType(&type);
    if (ret != 0)
    {
        PAL_ERR(LOG_TAG, "getStreamType failed with status = %d", ret);
        return ret;
    }
    PAL_DBG(LOG_TAG, "stream type %d", type);

    mActiveStreamMutex.lock();
    switch (type) {
        case PAL_STREAM_LOW_LATENCY:
        case PAL_STREAM_VOIP_RX:
        case PAL_STREAM_VOIP_TX:
        case PAL_STREAM_VOICE_CALL:
        {
            StreamPCM* sPCM = dynamic_cast<StreamPCM*>(s);
            ret = registerstream(sPCM, active_streams_ll);
            break;
        }
        case PAL_STREAM_PCM_OFFLOAD:
        case PAL_STREAM_LOOPBACK:
        {
            StreamPCM* sPCM = dynamic_cast<StreamPCM*>(s);
            ret = registerstream(sPCM, active_streams_po);
            break;
        }
        case PAL_STREAM_DEEP_BUFFER:
        {
            StreamPCM* sDB = dynamic_cast<StreamPCM*>(s);
            ret = registerstream(sDB, active_streams_db);
            break;
        }
        case PAL_STREAM_SPATIAL_AUDIO:
        {
            StreamPCM* sSA = dynamic_cast<StreamPCM*>(s);
            ret = registerstream(sSA, active_streams_sa);
            break;
        }
        case PAL_STREAM_COMPRESSED:
        {
            StreamCompress* sComp = dynamic_cast<StreamCompress*>(s);
            ret = registerstream(sComp, active_streams_comp);
            break;
        }
        case PAL_STREAM_GENERIC:
        {
            StreamPCM* sULLA = dynamic_cast<StreamPCM*>(s);
            ret = registerstream(sULLA, active_streams_ulla);
            break;
        }
        case PAL_STREAM_VOICE_UI:
        {
            if (active_streams_st.size() == 0)
                onVUIStreamRegistered();
            StreamSoundTrigger* sST = dynamic_cast<StreamSoundTrigger*>(s);
            ret = registerstream(sST, active_streams_st);
            break;
        }
        case PAL_STREAM_ASR:
        {
            StreamASR* sASR = dynamic_cast<StreamASR*>(s);
            ret = registerstream(sASR, active_streams_asr);
            break;
        }
        case PAL_STREAM_ULTRA_LOW_LATENCY:
        {
            StreamPCM* sULL = dynamic_cast<StreamPCM*>(s);
            ret = registerstream(sULL, active_streams_ull);
            break;
        }
        case PAL_STREAM_PROXY:
        {
            StreamPCM* sProxy = dynamic_cast<StreamPCM*>(s);
            ret = registerstream(sProxy, active_streams_proxy);
            break;
        }
        case PAL_STREAM_VOICE_CALL_MUSIC:
        {
            StreamInCall* sPCM = dynamic_cast<StreamInCall*>(s);
            ret = registerstream(sPCM, active_streams_incall_music);
            break;
        }
        case PAL_STREAM_VOICE_CALL_RECORD:
        {
            StreamInCall* sPCM = dynamic_cast<StreamInCall*>(s);
            ret = registerstream(sPCM, active_streams_incall_record);
            break;
        }
        case PAL_STREAM_NON_TUNNEL:
        {
            StreamNonTunnel* sNonTunnel = dynamic_cast<StreamNonTunnel*>(s);
            ret = registerstream(sNonTunnel, active_streams_non_tunnel);
            break;
        }
        case PAL_STREAM_HAPTICS:
        {
            StreamPCM* sHap = dynamic_cast<StreamPCM*>(s);
            ret = registerstream(sHap, active_streams_haptics);
            break;
        }
        case PAL_STREAM_ACD:
        {
            StreamACD* sAcd = dynamic_cast<StreamACD*>(s);
            ret = registerstream(sAcd, active_streams_acd);
            break;
        }
        case PAL_STREAM_ULTRASOUND:
        {
            StreamUltraSound* sUPD = dynamic_cast<StreamUltraSound*>(s);
            ret = registerstream(sUPD, active_streams_ultrasound);
            break;
        }
        case PAL_STREAM_RAW:
        {
            StreamPCM* sRaw = dynamic_cast<StreamPCM*>(s);
            ret = registerstream(sRaw, active_streams_raw);
            break;
        }
        case PAL_STREAM_SENSOR_PCM_DATA:
        {
            StreamSensorPCMData* sPCM = dynamic_cast<StreamSensorPCMData*>(s);
            ret = registerstream(sPCM, active_streams_sensor_pcm_data);
            break;
        }
        case PAL_STREAM_CONTEXT_PROXY:
        {
            StreamContextProxy* sCtxt = dynamic_cast<StreamContextProxy*>(s);
            ret = registerstream(sCtxt, active_streams_context_proxy);
            break;
        }
        case PAL_STREAM_VOICE_RECOGNITION:
        {
            StreamPCM* sVR = dynamic_cast<StreamPCM*>(s);
            ret = registerstream(sVR, active_streams_voice_rec);
            break;
        }
        case PAL_STREAM_COMMON_PROXY:
        {
            StreamCommonProxy* sAFS = dynamic_cast<StreamCommonProxy*>(s);
            ret = registerstream(sAFS, active_streams_afs);
            break;
        }
        case PAL_STREAM_SENSOR_PCM_RENDERER:
        {
            StreamSensorRenderer* sPCM = dynamic_cast<StreamSensorRenderer*>(s);
            ret = registerstream(sPCM, active_streams_sensor_renderer);
            break;
        }
        default:
            ret = -EINVAL;
            PAL_ERR(LOG_TAG, "Invalid stream type = %d ret %d", type, ret);
            break;
    }
    mActiveStreams.push_back(s);

#if 0
    s->getStreamAttributes(&incomingStreamAttr);
    int incomingPriority = getStreamAttrPriority(incomingStreamAttr);
    if (incomingPriority > mPriorityHighestPriorityActiveStream) {
        PAL_INFO(LOG_TAG, "Added new stream with highest priority %d", incomingPriority);
        mPriorityHighestPriorityActiveStream = incomingPriority;
        mHighestPriorityActiveStream = s;
    }
    calculalte priority and store in Stream

    mAllActiveStreams.push_back(s);
#endif

    mActiveStreamMutex.unlock();
    if (ret)
        PAL_ERR(LOG_TAG, "Failed to register stream type: %d, ret %d", type, ret);

    PAL_DBG(LOG_TAG, "Exit. ret %d", ret);
    return ret;
}

///private functions


// template function to deregister stream
template <class T>
int deregisterstream(T s, std::list<T> &streams)
{
    int ret = 0;
    typename std::list<T>::iterator iter = std::find(streams.begin(), streams.end(), s);
    if (iter != streams.end())
        streams.erase(iter);
    else
        ret = -ENOENT;
    return ret;
}

int ResourceManager::deregisterStream(Stream *s)
{
    struct pal_state_queue que;
    int ret = 0;
    pal_stream_type_t type;
    PAL_DBG(LOG_TAG, "Enter. stream %pK", s);
    ret = s->getStreamType(&type);
    if (0 != ret)
    {
        PAL_ERR(LOG_TAG, "getStreamType failed with status = %d", ret);
        goto exit;
    }

    PAL_INFO(LOG_TAG, "stream type %d", type);
    mActiveStreamMutex.lock();
    switch (type) {
        case PAL_STREAM_LOW_LATENCY:
        case PAL_STREAM_VOIP_RX:
        case PAL_STREAM_VOIP_TX:
        case PAL_STREAM_VOICE_CALL:
        {
            StreamPCM* sPCM = dynamic_cast<StreamPCM*>(s);
            ret = deregisterstream(sPCM, active_streams_ll);
            break;
        }
        case PAL_STREAM_PCM_OFFLOAD:
        case PAL_STREAM_LOOPBACK:
        {
            StreamPCM* sPCM = dynamic_cast<StreamPCM*>(s);
            ret = deregisterstream(sPCM, active_streams_po);
            break;
        }
        case PAL_STREAM_DEEP_BUFFER:
        {
            StreamPCM* sDB = dynamic_cast<StreamPCM*>(s);
            ret = deregisterstream(sDB, active_streams_db);
            break;
        }
        case PAL_STREAM_SPATIAL_AUDIO:
        {
            StreamPCM* sSA = dynamic_cast<StreamPCM*>(s);
            ret = deregisterstream(sSA, active_streams_sa);
            break;
        }
        case PAL_STREAM_COMPRESSED:
        {
            StreamCompress* sComp = dynamic_cast<StreamCompress*>(s);
            ret = deregisterstream(sComp, active_streams_comp);
            break;
        }
        case PAL_STREAM_GENERIC:
        {
            StreamPCM* sULLA = dynamic_cast<StreamPCM*>(s);
            ret = deregisterstream(sULLA, active_streams_ulla);
            break;
        }
        case PAL_STREAM_VOICE_UI:
        {
            StreamSoundTrigger* sST = dynamic_cast<StreamSoundTrigger*>(s);
            ret = deregisterstream(sST, active_streams_st);
            // reset concurrency count when all st streams deregistered
            if (active_streams_st.size() == 0) {
                onVUIStreamDeregistered();
            }
            break;
        }
        case PAL_STREAM_ASR:
        {
            StreamASR* sASR = dynamic_cast<StreamASR*>(s);
            ret = deregisterstream(sASR, active_streams_asr);
            break;
        }
        case PAL_STREAM_ULTRA_LOW_LATENCY:
        {
            StreamPCM* sULL = dynamic_cast<StreamPCM*>(s);
            ret = deregisterstream(sULL, active_streams_ull);
            break;
        }
        case PAL_STREAM_PROXY:
        {
            StreamPCM* sProxy = dynamic_cast<StreamPCM*>(s);
            ret = deregisterstream(sProxy, active_streams_proxy);
            break;
        }
        case PAL_STREAM_VOICE_CALL_MUSIC:
        {
            StreamInCall* sPCM = dynamic_cast<StreamInCall*>(s);
            ret = deregisterstream(sPCM, active_streams_incall_music);
            break;
        }
        case PAL_STREAM_VOICE_CALL_RECORD:
        {
            StreamInCall* sPCM = dynamic_cast<StreamInCall*>(s);
            ret = deregisterstream(sPCM, active_streams_incall_record);
            break;
        }
        case PAL_STREAM_NON_TUNNEL:
        {
            StreamNonTunnel* sNonTunnel = dynamic_cast<StreamNonTunnel*>(s);
            ret = deregisterstream(sNonTunnel, active_streams_non_tunnel);
            break;
        }
        case PAL_STREAM_HAPTICS:
        {
            StreamPCM* sHap = dynamic_cast<StreamPCM*>(s);
            ret = deregisterstream(sHap, active_streams_haptics);
            break;
        }
        case PAL_STREAM_ACD:
        {
            StreamACD* sAcd = dynamic_cast<StreamACD*>(s);
            ret = deregisterstream(sAcd, active_streams_acd);
            break;
        }
        case PAL_STREAM_ULTRASOUND:
        {
            StreamUltraSound* sUPD = dynamic_cast<StreamUltraSound*>(s);
            ret = deregisterstream(sUPD, active_streams_ultrasound);
            break;
        }
        case PAL_STREAM_RAW:
        {
            StreamPCM* sRaw = dynamic_cast<StreamPCM*>(s);
            ret = deregisterstream(sRaw, active_streams_raw);
            break;
        }
        case PAL_STREAM_SENSOR_PCM_DATA:
        {
            StreamSensorPCMData* sPCM = dynamic_cast<StreamSensorPCMData*>(s);
            ret = deregisterstream(sPCM, active_streams_sensor_pcm_data);
            break;
        }
        case PAL_STREAM_CONTEXT_PROXY:
        {
            StreamContextProxy* sCtxt = dynamic_cast<StreamContextProxy*>(s);
            ret = deregisterstream(sCtxt, active_streams_context_proxy);
            break;
        }
        case PAL_STREAM_VOICE_RECOGNITION:
        {
            StreamPCM* sVR = dynamic_cast<StreamPCM*>(s);
            ret = deregisterstream(sVR, active_streams_voice_rec);
            break;
        }
        case PAL_STREAM_COMMON_PROXY:
        {
            StreamCommonProxy* sAFS = dynamic_cast<StreamCommonProxy*>(s);
            ret = deregisterstream(sAFS, active_streams_afs);
            break;
        }
        case PAL_STREAM_SENSOR_PCM_RENDERER:
        {
            StreamSensorRenderer* sPCM = dynamic_cast<StreamSensorRenderer*>(s);
            ret = deregisterstream(sPCM, active_streams_sensor_renderer);
            break;
        }

        default:
            ret = -EINVAL;
            PAL_ERR(LOG_TAG, "Invalid stream type = %d ret %d", type, ret);
            break;
    }

    deregisterstream(s, mActiveStreams);

    mActiveStreamMutex.unlock();
exit:
    if (ret)
        PAL_ERR(LOG_TAG, "Failed to deregister stream type: %d, ret %d", type, ret);

    PAL_DBG(LOG_TAG, "Exit. ret %d", ret);
    return ret;
}

template <class T>
bool isStreamActive(T s, std::list<T> &streams)
{
    bool ret = false;

    PAL_DBG(LOG_TAG, "Enter.");
    typename std::list<T>::iterator iter =
        std::find(streams.begin(), streams.end(), s);
    if (iter != streams.end()) {
        ret = true;
    }

    PAL_DBG(LOG_TAG, "Exit, ret %d", ret);
    return ret;
}

int ResourceManager::isActiveStream(pal_stream_handle_t *handle) {
    for (auto &s : mActiveStreams) {
        if (handle == reinterpret_cast<uint64_t *>(s)) {
            return true;
        }
    }
    return false;
}

int ResourceManager::initStreamUserCounter(Stream *s)
{
    lockActiveStream();
    mActiveStreamUserCounter.insert(std::make_pair(s, std::make_pair(0, true)));
    s->initStreamSmph();
    unlockActiveStream();
    return 0;
}

int ResourceManager::deactivateStreamUserCounter(Stream *s)
{
    std::map<Stream*, std::pair<uint32_t, bool>>::iterator it;
    lockActiveStream();
    printStreamUserCounter(s);
    it = mActiveStreamUserCounter.find(s);
    if (it != mActiveStreamUserCounter.end() && it->second.second == true) {
        PAL_DBG(LOG_TAG, "stream %p is to be deactivated.", s);
        it->second.second = false;
        unlockActiveStream();
        s->waitStreamSmph();
        PAL_DBG(LOG_TAG, "stream %p is inactive.", s);
        s->deinitStreamSmph();
        return 0;
    } else {
        PAL_ERR(LOG_TAG, "stream %p is not found or inactive", s);
        unlockActiveStream();
        return -EINVAL;
    }
}

int ResourceManager::eraseStreamUserCounter(Stream *s)
{
    std::map<Stream*, std::pair<uint32_t, bool>>::iterator it;
    lockActiveStream();
    it = mActiveStreamUserCounter.find(s);
    if (it != mActiveStreamUserCounter.end()) {
        mActiveStreamUserCounter.erase(it);
        PAL_DBG(LOG_TAG, "stream counter for %p is erased.", s);
        unlockActiveStream();
        return 0;
    } else {
        PAL_ERR(LOG_TAG, "stream counter for %p is not found.", s);
        unlockActiveStream();
        return -EINVAL;
    }
}

int ResourceManager::increaseStreamUserCounter(Stream* s)
{
    std::map<Stream*, std::pair<uint32_t, bool>>::iterator it;
    printStreamUserCounter(s);
    it = mActiveStreamUserCounter.find(s);
    if (it != mActiveStreamUserCounter.end() &&
        it->second.second) {
        if (0 == it->second.first) {
            s->waitStreamSmph();
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
            PAL_VERBOSE(LOG_TAG, "stream %p in use", s);
#else
            PAL_DBG(LOG_TAG, "stream %p in use", s);
#endif
        }
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
        PAL_VERBOSE(LOG_TAG, "stream %p counter was %d", s, it->second.first);
#else
        PAL_DBG(LOG_TAG, "stream %p counter was %d", s, it->second.first);
#endif
        it->second.first = it->second.first + 1;
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
        PAL_VERBOSE(LOG_TAG, "stream %p counter increased to %d", s, it->second.first);
#else
        PAL_DBG(LOG_TAG, "stream %p counter increased to %d", s, it->second.first);
#endif
        return 0;
    } else {
        PAL_ERR(LOG_TAG, "stream %p is not found or inactive.", s);
        return -EINVAL;
    }
}

int ResourceManager::decreaseStreamUserCounter(Stream* s)
{
    std::map<Stream*, std::pair<uint32_t, bool>>::iterator it;
    printStreamUserCounter(s);
    it = mActiveStreamUserCounter.find(s);
    if (it != mActiveStreamUserCounter.end()) {
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
        PAL_VERBOSE(LOG_TAG, "stream %p counter was %d", s, it->second.first);
#else
        PAL_DBG(LOG_TAG, "stream %p counter was %d", s, it->second.first);
#endif
        if (0 == it->second.first) {
            PAL_ERR(LOG_TAG, "counter of stream %p has already been 0.", s);
            return -EINVAL;
        }

        it->second.first = it->second.first - 1;
        if (0 == it->second.first) {
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
            PAL_VERBOSE(LOG_TAG, "stream %p not in use", s);
#else
            PAL_DBG(LOG_TAG, "stream %p not in use", s);
#endif
            s->postStreamSmph();
        }
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
        PAL_VERBOSE(LOG_TAG, "stream %p counter decreased to %d", s, it->second.first);
#else
        PAL_DBG(LOG_TAG, "stream %p counter decreased to %d", s, it->second.first);
#endif
        return 0;
    } else {
        PAL_ERR(LOG_TAG, "stream %p is not found.", s);
        return -EINVAL;
    }
}

int ResourceManager::getStreamUserCounter(Stream *s)
{
    std::map<Stream*, std::pair<uint32_t, bool>>::iterator it;
    printStreamUserCounter(s);
    it = mActiveStreamUserCounter.find(s);
    if (it != mActiveStreamUserCounter.end()) {
        return it->second.first;
    } else {
        PAL_ERR(LOG_TAG, "stream %p is not found.", s);
        return -EINVAL;
    }
}

int ResourceManager::printStreamUserCounter(Stream *s)
{
    std::map<Stream*, std::pair<uint32_t, bool>>::iterator it;
    for (it = mActiveStreamUserCounter.begin();
            it != mActiveStreamUserCounter.end(); it++) {
        PAL_VERBOSE(LOG_TAG, "stream = %p count = %d active = %d",
                    it->first, it->second.first, it->second.second);
    }

    return 0;
}

// check if any of the ec device supports external ec
bool ResourceManager::isExternalECSupported(std::shared_ptr<Device> tx_dev) {
    bool is_supported = false;
    int i = 0;
    int tx_dev_id = 0;
    int rx_dev_id = 0;
    std::vector<pal_device_id_t>::iterator iter;

    if (!tx_dev) {
        PAL_ERR(LOG_TAG, "Invalid tx_dev");
        goto exit;
    }

    tx_dev_id = tx_dev->getSndDeviceId();
    for (i = 0; i < deviceInfo.size(); i++) {
        if (tx_dev_id == deviceInfo[i].deviceId) {
            break;
        }
    }

    if (i == deviceInfo.size()) {
        PAL_ERR(LOG_TAG, "Tx device %d not found", tx_dev_id);
        goto exit;
    }

    for (iter = deviceInfo[i].rx_dev_ids.begin();
        iter != deviceInfo[i].rx_dev_ids.end(); iter++) {
        rx_dev_id = *iter;
        is_supported = isExternalECRefEnabled(rx_dev_id);
        if (is_supported)
            break;
    }

exit:
    return is_supported;
}

bool ResourceManager::isExternalECRefEnabled(int rx_dev_id)
{
    bool is_enabled = false;

    for (int i = 0; i < deviceInfo.size(); i++) {
        if (rx_dev_id == deviceInfo[i].deviceId) {
            is_enabled = deviceInfo[i].isExternalECRefEnabled;
            break;
        }
    }

    return is_enabled;
}

// NOTE: this api should be called with mActiveStreamMutex locked
void ResourceManager::disableInternalECRefs(Stream *s)
{
    int32_t status = 0;
    std::shared_ptr<Device> dev = nullptr;
    std::shared_ptr<Device> rx_dev = nullptr;
    struct pal_stream_attributes sAttr;
    std::vector<std::shared_ptr<Device>> associatedDevices;

    PAL_DBG(LOG_TAG, "Enter");
    for (auto str: mActiveStreams) {
        associatedDevices.clear();
        if (!str)
            continue;

        status = str->getStreamAttributes(&sAttr);
        if (status != 0) {
            PAL_ERR(LOG_TAG,"stream get attributes failed");
            continue;
        } else if (sAttr.direction != PAL_AUDIO_INPUT) {
            continue;
        }

        status = str->getAssociatedDevices(associatedDevices);
        if ((0 != status) || associatedDevices.empty()) {
            PAL_ERR(LOG_TAG, "getAssociatedDevices Failed or Empty");
            continue;
        }

        // Tx stream should have one device
        for (int i = 0; i < associatedDevices.size(); i++) {
            dev = associatedDevices[i];
            if (isExternalECSupported(dev)) {
                rx_dev = clearInternalECRefCounts(str, dev);
                if (rx_dev && !checkStreamMatch(str, s)) {
                    if (isDeviceSwitch)
                        status = str->setECRef_l(rx_dev, false);
                    else
                        status = str->setECRef(rx_dev, false);
                }
            }
        }
    }

    PAL_DBG(LOG_TAG, "Exit");
}

// NOTE: this api should be called with mActiveStreamMutex locked
void ResourceManager::restoreInternalECRefs()
{
    int32_t status = 0;
    std::shared_ptr<Device> dev = nullptr;
    struct pal_stream_attributes sAttr;
    std::vector<std::shared_ptr<Device>> associatedDevices;

    PAL_DBG(LOG_TAG, "Enter");
    for (auto str: mActiveStreams) {
        associatedDevices.clear();
        if (!str)
            continue;

        status = str->getStreamAttributes(&sAttr);
        if (status != 0) {
            PAL_ERR(LOG_TAG,"stream get attributes failed");
            continue;
        } else if (sAttr.direction != PAL_AUDIO_INPUT) {
            continue;
        }

        status = str->getAssociatedDevices(associatedDevices);
        if ((0 != status) || associatedDevices.empty()) {
            PAL_ERR(LOG_TAG, "getAssociatedDevices Failed or Empty");
            continue;
        }

        // Tx stream should have one device
        for (int i = 0; i < associatedDevices.size(); i++) {
            dev = associatedDevices[i];
            mResourceManagerMutex.lock();
            if (isDeviceActive_l(dev, str))
                checkandEnableEC_l(dev, str, true);
            mResourceManagerMutex.unlock();
        }
    }

    PAL_DBG(LOG_TAG, "Exit");
}

bool ResourceManager::checkStreamMatch(Stream *target, Stream *ref) {
    int32_t status = 0;
    bool is_match = false;
    struct pal_stream_attributes target_attr;
    struct pal_stream_attributes ref_attr;
    StreamSoundTrigger *st_target = nullptr;
    StreamSoundTrigger *st_ref = nullptr;

    if (target == ref)
        return true;

    status = target->getStreamAttributes(&target_attr);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"stream get attributes failed");
        goto exit;
    }

    status = ref->getStreamAttributes(&ref_attr);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"stream get attributes failed");
        goto exit;
    }

    if (target_attr.type != PAL_STREAM_VOICE_UI ||
        ref_attr.type != PAL_STREAM_VOICE_UI)
        goto exit;

    st_target = dynamic_cast<StreamSoundTrigger *>(target);
    st_ref = dynamic_cast<StreamSoundTrigger *>(ref);

    if (st_target->GetGSLEngine() == st_ref->GetGSLEngine()) {
        PAL_DBG(LOG_TAG, "Voice UI stream match because same gsl engine used");
        is_match = true;
    }

exit:
    return is_match;
}

int ResourceManager::getECEnableSetting(std::shared_ptr<Device> tx_dev,
                                        Stream* streamHandle, bool *ec_enable)
{
    int status = 0;
    struct pal_device DevDattr;
    pal_device_id_t deviceId;
    std::string key = "";
    struct pal_stream_attributes curStrAttr;

    if (tx_dev == nullptr || ec_enable == nullptr || streamHandle == nullptr) {
        PAL_ERR(LOG_TAG, "invalid input");
        status = -EINVAL;
        goto exit;
    }

    streamHandle->getStreamAttributes(&curStrAttr);
    *ec_enable = true;
    status = tx_dev->getDeviceAttributes(&DevDattr, streamHandle);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getDeviceAttributes Failed");
        goto exit;
    } else if (strlen(DevDattr.custom_config.custom_key)) {
        key = DevDattr.custom_config.custom_key;
    }
    deviceId = (pal_device_id_t)tx_dev->getSndDeviceId();

    PAL_DBG(TAG_LOG, "stream type: %d, deviceid: %d, custom key: %s",
                      curStrAttr.type, deviceId, key.c_str());
    for (auto devInfo : deviceInfo) {
        if (deviceId != devInfo.deviceId)
            continue;
        *ec_enable = devInfo.ec_enable;
        for (auto usecaseInfo : devInfo.usecase) {
            if (curStrAttr.type != usecaseInfo.type)
                continue;
            *ec_enable = usecaseInfo.ec_enable;
            for (auto custom_config : usecaseInfo.config) {
                PAL_DBG(TAG_LOG,"existing custom config key = %s", custom_config.key.c_str());
                if (!custom_config.key.compare(key)) {
                    *ec_enable = custom_config.ec_enable;
                    break;
                }
            }
            break;
        }
        break;
    }
exit:
    PAL_DBG(TAG_LOG,"ec_enable_setting:%d, status:%d", ec_enable ? *ec_enable : 0, status);
    return status;
}

int ResourceManager::checkandEnableECForTXStream_l(std::shared_ptr<Device> tx_dev,
                                                   Stream *tx_stream, bool ec_on)
{
    int status = 0;
    struct pal_stream_attributes sAttr;
    std::shared_ptr<Device> rx_dev = nullptr;
    std::vector <Stream *> activeStreams;
    struct pal_stream_attributes rx_attr;
    int rxdevcount = 0;
    bool ec_enable_setting = false;

    if (!tx_dev || !tx_stream) {
        PAL_ERR(LOG_TAG, "invalid input.");
        status = -EINVAL;
        goto exit;
    }
    status = tx_stream->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed.");
        status = -EINVAL;
        goto exit;
    }
    PAL_DBG(LOG_TAG, "Enter: setting EC[%s] for usecase %d of device %d.",
                      ec_on ? "ON" : "OFF", sAttr.type, tx_dev->getSndDeviceId());

    status = getECEnableSetting(tx_dev, tx_stream, &ec_enable_setting);
    if (status !=0) {
        PAL_ERR(LOG_TAG, "getECEnableSetting failed.");
        goto exit;
    } else if (!ec_enable_setting) {
        PAL_ERR(LOG_TAG, "EC is disabled for usecase %d of device %d.",
                          sAttr.type, tx_dev->getSndDeviceId());
        goto exit;
    }

    if (ec_on) {
        rx_dev = getActiveEchoReferenceRxDevices_l(tx_stream);
        if (!rx_dev) {
            PAL_VERBOSE(LOG_TAG, "EC device not found, skip EC set");
            goto exit;
        }
        getActiveStream_l(activeStreams, rx_dev);
        for (auto& rx_str: activeStreams) {
            if (!isDeviceActive_l(rx_dev, rx_str) ||
                !(rx_str->getCurState() == STREAM_STARTED ||
                  rx_str->getCurState() == STREAM_PAUSED))
                continue;
            rx_str->getStreamAttributes(&rx_attr);
            if (rx_attr.direction != PAL_AUDIO_INPUT) {
                if (getEcRefStatus(sAttr.type, rx_attr.type)) {
                    rxdevcount++;
                } else {
                    PAL_DBG(LOG_TAG, "rx stream is disabled for ec ref %d.", rx_attr.type);
                    continue;
                }
            }
        }
    }
    rxdevcount = updateECDeviceMap(rx_dev, tx_dev, tx_stream, rxdevcount, !ec_on);
    if (rxdevcount <= 0 && ec_on) {
        PAL_DBG(LOG_TAG, "No need to enable EC ref");
    } else {
        mResourceManagerMutex.unlock();
        status = tx_stream->setECRef_l(rx_dev, ec_on);
        mResourceManagerMutex.lock();
        if (status == -ENODEV) {
            PAL_VERBOSE(LOG_TAG, "operation is not supported by device, error: %d.", status);
            status = 0;
        } else if (status && ec_on) {
            // reset ec map if set ec failed for tx device
            updateECDeviceMap(rx_dev, tx_dev, tx_stream, 0, ec_on);
        }
    }
exit:
    PAL_DBG(LOG_TAG, "Exit. status: %d", status);
    return status;
}

int ResourceManager::checkandEnableECForRXStream_l(std::shared_ptr<Device> rx_dev,
                                                   Stream *rx_stream, bool ec_on)
{
    int status = 0;
    struct pal_stream_attributes sAttr;
    std::vector<Stream*> tx_streams_list;
    std::shared_ptr<Device> tx_dev = nullptr;
    std::vector<std::shared_ptr<Device>> tx_devices;
    int ec_map_rx_dev_count = 0;
    int rxdevcount = 0;
    bool ec_enable_setting = false;

    if (!rx_dev || !rx_stream) {
        PAL_ERR(LOG_TAG, "invalid input");
        status = -EINVAL;
        goto exit;
    }
    status = rx_stream->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        goto exit;
    }

    PAL_DBG(LOG_TAG, "Enter: setting EC[%s] for usecase %d of device %d.",
                      ec_on ? "ON" : "OFF", sAttr.type, rx_dev->getSndDeviceId());

    tx_streams_list = getConcurrentTxStream_l(rx_stream, rx_dev);
    for (auto tx_stream: tx_streams_list) {
        tx_devices.clear();
        if (!tx_stream || !isStreamActive(tx_stream, mActiveStreams)) {
            PAL_ERR(LOG_TAG, "TX Stream Empty or is not active\n");
            continue;
        }
        tx_stream->getAssociatedDevices(tx_devices);
        if (tx_devices.empty()) {
            PAL_ERR(LOG_TAG, "TX devices Empty\n");
            continue;
        }
        status = tx_stream->getStreamAttributes(&sAttr);
        if (status) {
            PAL_ERR(LOG_TAG, "stream get attributes failed");
            status = -EINVAL;
            continue;
        }
        // TODO: add support for stream with multi Tx devices
        tx_dev = tx_devices[0];
        status = getECEnableSetting(tx_dev, tx_stream, &ec_enable_setting);
        if (status != 0) {
            PAL_DBG(LOG_TAG, "getECEnableSetting failed.");
            continue;
        } else if (!ec_enable_setting) {
            PAL_ERR(LOG_TAG, "EC is disabled for usecase %d of device %d",
                              sAttr.type, tx_dev->getSndDeviceId());
            continue;
        }
        ec_map_rx_dev_count = ec_on ? 1 : 0;
        rxdevcount = updateECDeviceMap(rx_dev, tx_dev, tx_stream, ec_map_rx_dev_count, false);
        if (rxdevcount != ec_map_rx_dev_count) {
            PAL_DBG(LOG_TAG, "Invalid device pair or no need, rxdevcount =%d", rxdevcount);
            continue;
        }
        mResourceManagerMutex.unlock();
        if (isDeviceSwitch && tx_stream->isMutexLockedbyRm())
            status = tx_stream->setECRef_l(rx_dev, ec_on);
        else
            status = tx_stream->setECRef(rx_dev, ec_on);
        mResourceManagerMutex.lock();
        if (status != 0 && ec_on) {
            if (status == -ENODEV) {
                status = 0;
                PAL_VERBOSE(LOG_TAG, "operation is not supported by device, error: %d", status);
            }
            // decrease ec ref count if ec ref set failure
            updateECDeviceMap(rx_dev, tx_devices[0], tx_stream, 0, false);
        }
    }

exit:
    PAL_DBG(LOG_TAG, "Exit. status: %d", status);
    return status;
}

int ResourceManager::checkandEnableEC_l(std::shared_ptr<Device> d, Stream *s, bool enable)
{
    int status = 0;
    struct pal_stream_attributes sAttr;
    std::vector<std::shared_ptr<Device>> tx_devices;

    if (!d || !s) {
        status = -EINVAL;
        goto exit;
    }
    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        goto exit;
    }
    if (sAttr.type == PAL_STREAM_PROXY ||
        ((sAttr.type == PAL_STREAM_ULTRA_LOW_LATENCY ||
        sAttr.type == PAL_STREAM_GENERIC) && sAttr.direction == PAL_AUDIO_INPUT)) {
        PAL_DBG(LOG_TAG, "stream type %d is not supported for setting EC", sAttr.type);
        goto exit;
    }

    PAL_DBG(LOG_TAG, "Enter: setting to enable[%s] for stream %d.", enable ? "ON" : "OFF", sAttr.type);
    if (sAttr.direction == PAL_AUDIO_INPUT) {
        status = checkandEnableECForTXStream_l(d, s, enable);
    } else if (sAttr.direction == PAL_AUDIO_OUTPUT) {
        status = checkandEnableECForRXStream_l(d, s, enable);
    } else if (sAttr.direction == PAL_AUDIO_INPUT_OUTPUT) {
        if (d->getSndDeviceId() < PAL_DEVICE_OUT_MAX) {
            if (sAttr.type == PAL_STREAM_VOICE_CALL) {
                status = s->setECRef_l(d, enable);
                s->getAssociatedDevices(tx_devices);
                if (status || tx_devices.empty()) {
                    PAL_ERR(LOG_TAG, "Failed to set EC Ref with status %d"
                            "or tx_devices with size %zu", status, tx_devices.size());
                    if (status == -ENODEV) {
                        status = 0;
                        PAL_VERBOSE(LOG_TAG, "Failed to enable EC Ref because of -ENODEV");
                    }
                } else {
                    for (auto& tx_device: tx_devices) {
                        if (tx_device->getSndDeviceId() > PAL_DEVICE_IN_MIN &&
                            tx_device->getSndDeviceId() < PAL_DEVICE_IN_MAX) {
                            updateECDeviceMap(d, tx_device, s, enable ? 1 : 0, false);
                        }
                    }
                }
            }
            status = checkandEnableECForRXStream_l(d, s, enable);
        }
    }

exit:
    PAL_DBG(LOG_TAG, "Exit. status: %d", status);
    return status;
}

int ResourceManager::registerDevice_l(std::shared_ptr<Device> d, Stream *s)
{
    int ret = 0;
    PAL_DBG(LOG_TAG, "Enter.");

    auto iter = std::find(active_devices.begin(),
        active_devices.end(), std::make_pair(d, s));
    if (iter == active_devices.end())
        active_devices.push_back(std::make_pair(d, s));
    else
        ret = -EINVAL;
    PAL_DBG(LOG_TAG, "Exit.");
    return ret;
}

int ResourceManager::registerDevice(std::shared_ptr<Device> d, Stream *s)
{
    PAL_DBG(LOG_TAG, "Enter. dev id: %d", d->getSndDeviceId());

    mResourceManagerMutex.lock();
    if (registerDevice_l(d, s)) {
        PAL_DBG(LOG_TAG, "device %d is already registered for stream %pK",
            d->getSndDeviceId(), s);
    } else {
        checkandEnableEC_l(d, s, true);
    }
    mResourceManagerMutex.unlock();

    PAL_DBG(LOG_TAG, "Exit.");
    return 0;
}

int ResourceManager::deregisterDevice_l(std::shared_ptr<Device> d, Stream *s)
{
    int ret = 0;
    PAL_VERBOSE(LOG_TAG, "Enter.");

    auto iter = std::find(active_devices.begin(),
        active_devices.end(), std::make_pair(d, s));
    if (iter != active_devices.end())
        active_devices.erase(iter);
    else {
        ret = -ENOENT;
        PAL_ERR(LOG_TAG, "no device %d found in active device list ret %d",
                d->getSndDeviceId(), ret);
    }
    PAL_VERBOSE(LOG_TAG, "Exit. ret %d", ret);
    return ret;
}

int ResourceManager::deregisterDevice(std::shared_ptr<Device> d, Stream *s)
{
    PAL_DBG(LOG_TAG, "Enter. dev id: %d", d->getSndDeviceId());

    mResourceManagerMutex.lock();
    if (deregisterDevice_l(d, s)) {
        PAL_DBG(LOG_TAG, "Device %d not found for stream %pK, skip EC handling",
            d->getSndDeviceId(), s);
    } else {
        checkandEnableEC_l(d, s, false);
    }
    mResourceManagerMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit.");
    return 0;
}

bool ResourceManager::isDeviceActive(pal_device_id_t deviceId)
{
    bool is_active = false;
    int candidateDeviceId;
    PAL_DBG(LOG_TAG, "Enter.");

    mResourceManagerMutex.lock();
    for (int i = 0; i < active_devices.size(); i++) {
        candidateDeviceId = active_devices[i].first->getSndDeviceId();
        if (deviceId == candidateDeviceId) {
            is_active = true;
            PAL_INFO(LOG_TAG, "deviceid of %d is active", deviceId);
            break;
        }
    }

    mResourceManagerMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit.");
    return is_active;
}

bool ResourceManager::isDeviceActive(std::shared_ptr<Device> d, Stream *s)
{
    bool is_active = false;

    PAL_DBG(LOG_TAG, "Enter.");
    mResourceManagerMutex.lock();
    is_active = isDeviceActive_l(d, s);
    mResourceManagerMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit.");
    return is_active;
}

bool ResourceManager::isDeviceActive_l(std::shared_ptr<Device> d, Stream *s)
{
    bool is_active = false;
    int deviceId = d->getSndDeviceId();

    PAL_DBG(LOG_TAG, "Enter.");
    auto iter = std::find(active_devices.begin(),
        active_devices.end(), std::make_pair(d, s));
    if (iter != active_devices.end()) {
        is_active = true;
    }

    PAL_DBG(LOG_TAG, "Exit. device %d is active %d", deviceId, is_active);
    return is_active;
}

int ResourceManager::addPlugInDevice(std::shared_ptr<Device> d,
                            pal_param_device_connection_t connection_state)
{
    int ret = 0;

    ret = d->init(connection_state);
    if (ret && ret != -ENOENT) {
        PAL_ERR(LOG_TAG, "failed to init deivce.");
        return ret;
    }

    if (ret != -ENOENT)
        plugin_devices_.push_back(d);
    return ret;
}

int ResourceManager::removePlugInDevice(pal_device_id_t device_id,
                            pal_param_device_connection_t connection_state)
{
    int ret = 0;
    PAL_DBG(LOG_TAG, "Enter.");
    typename std::vector<std::shared_ptr<Device>>::iterator iter;

    for (iter = plugin_devices_.begin(); iter != plugin_devices_.end(); iter++) {
        if ((*iter)->getSndDeviceId() == device_id)
            break;
    }

    if (iter != plugin_devices_.end()) {
        (*iter)->deinit(connection_state);
        plugin_devices_.erase(iter);
    } else {
        ret = -ENOENT;
        PAL_ERR(LOG_TAG, "no device %d found in plugin device list ret %d",
                device_id, ret);
    }
    PAL_DBG(LOG_TAG, "Exit. ret %d", ret);
    return ret;
}

int ResourceManager::getActiveDevices(std::vector<std::shared_ptr<Device>> &deviceList)
{
    int ret = 0;
    mResourceManagerMutex.lock();
    for (int i = 0; i < active_devices.size(); i++)
        deviceList.push_back(active_devices[i].first);
    mResourceManagerMutex.unlock();
    return ret;
}

int ResourceManager::getAudioRoute(struct audio_route** ar)
{
    if (!audio_route) {
        PAL_ERR(LOG_TAG, "no audio route found");
        return -ENOENT;
    }
    *ar = audio_route;
    PAL_DBG(LOG_TAG, "ar %pK audio_route %pK", ar, audio_route);
    return 0;
}

int ResourceManager::getVirtualAudioMixer(struct audio_mixer ** am)
{
    if (!audio_virt_mixer || !am) {
        PAL_ERR(LOG_TAG, "no audio mixer found");
        return -ENOENT;
    }
    *am = audio_virt_mixer;
    PAL_DBG(LOG_TAG, "ar %pK audio_virt_mixer %pK", am, audio_virt_mixer);
    return 0;
}

int ResourceManager::getHwAudioMixer(struct audio_mixer ** am)
{
    if (!audio_hw_mixer || !am) {
        PAL_ERR(LOG_TAG, "no audio mixer found");
        return -ENOENT;
    }
    *am = audio_hw_mixer;
    PAL_DBG(LOG_TAG, "ar %pK audio_hw_mixer %pK", am, audio_hw_mixer);
    return 0;
}

void ResourceManager::GetVoiceUIProperties(struct pal_st_properties *qstp)
{
    std::shared_ptr<VoiceUIPlatformInfo> vui_info =
        VoiceUIPlatformInfo::GetInstance();

    if (!qstp) {
        return;
    }

    memcpy(qstp, &qst_properties, sizeof(struct pal_st_properties));

    if (vui_info) {
        qstp->version = vui_info->GetVersion();
        qstp->concurrent_capture = vui_info->GetConcurrentCaptureEnable();
    }
}

bool ResourceManager::isNLPISwitchSupported() {

    std::shared_ptr<SoundTriggerPlatformInfo> st_info =
                             SoundTriggerPlatformInfo::GetInstance();
    return st_info != nullptr ? st_info->GetSupportNLPISwitch() : false;
}

bool ResourceManager::IsLPISupported() {

    std::shared_ptr<SoundTriggerPlatformInfo> st_info =
                             SoundTriggerPlatformInfo::GetInstance();
    return st_info != nullptr ? st_info->GetLpiEnable() : false;
}

bool ResourceManager::IsDedicatedBEForUPDEnabled()
{
    return ResourceManager::isUpdDedicatedBeEnabled;
}

bool ResourceManager::IsDutyCycleForUPDEnabled()
{
    return ResourceManager::isUpdDutyCycleEnabled;
}

bool ResourceManager::IsVirtualPortForUPDEnabled()
{
    return ResourceManager::isUPDVirtualPortEnabled;
}

uint32_t ResourceManager::getHapticsPriority()
{
    return haptics_priority;
}

bool ResourceManager::IsHapticsThroughWSA()
{
    return ResourceManager::isHapticsthroughWSA;

}

void ResourceManager::GetSoundTriggerConcurrencyCount(
    pal_stream_type_t type,
    int32_t *enable_count, int32_t *disable_count) {
    mActiveStreamMutex.lock();
    GetSoundTriggerConcurrencyCount_l(type, enable_count, disable_count);
    mActiveStreamMutex.unlock();
}

// this should only be called when LPI supported by platform
void ResourceManager::GetSoundTriggerConcurrencyCount_l(
    pal_stream_type_t type,
    int32_t *enable_count, int32_t *disable_count) {

    pal_stream_attributes st_attr;
    bool voice_conc_enable = IsVoiceCallConcurrencySupported();
    bool voip_conc_enable = IsVoipConcurrencySupported();
    bool audio_capture_conc_enable =
        IsAudioCaptureConcurrencySupported();
    bool low_latency_bargein_enable = IsLowLatencyBargeinSupported();

    if (type == PAL_STREAM_ACD) {
        *enable_count = ACDConcurrencyEnableCount;
        *disable_count = ACDConcurrencyDisableCount;
    } else if (type == PAL_STREAM_VOICE_UI) {
        *enable_count = concurrencyEnableCount;
        *disable_count = concurrencyDisableCount;
    } else if (type == PAL_STREAM_ASR) {
        *enable_count = ASRConcurrencyEnableCount;
        *disable_count = ASRConcurrencyDisableCount;
    } else if (type == PAL_STREAM_SENSOR_PCM_DATA) {
        *enable_count = SNSPCMDataConcurrencyEnableCount;
        *disable_count = SNSPCMDataConcurrencyDisableCount;
    } else {
        PAL_ERR(LOG_TAG, "Error:%d Invalid stream type %d", -EINVAL, type);
        return;
    }

    PAL_INFO(LOG_TAG, "conc enable cnt %d, conc disable count %d",
        *enable_count, *disable_count);
}

bool ResourceManager::IsLowLatencyBargeinSupported() {
    std::shared_ptr<SoundTriggerPlatformInfo> st_info =
                SoundTriggerPlatformInfo::GetInstance();
    return st_info != nullptr ? st_info->GetLowLatencyBargeinEnable() : false;
}

bool ResourceManager::IsAudioCaptureConcurrencySupported() {
    std::shared_ptr<SoundTriggerPlatformInfo> st_info =
                SoundTriggerPlatformInfo::GetInstance();
    return st_info != nullptr ? st_info->GetConcurrentCaptureEnable() : false;
}

bool ResourceManager::IsVoiceCallConcurrencySupported() {
    std::shared_ptr<SoundTriggerPlatformInfo> st_info =
                SoundTriggerPlatformInfo::GetInstance();

    /* if CRS call allow concurrency*/
    if (isCRSCallEnabled){
        PAL_INFO(LOG_TAG, "In CRS call, allow voice concurrency");
        return true;
    }

    return st_info != nullptr ? st_info->GetConcurrentVoiceCallEnable() : false;
}

bool ResourceManager::IsVoipConcurrencySupported() {
    std::shared_ptr<SoundTriggerPlatformInfo> st_info =
                SoundTriggerPlatformInfo::GetInstance();
    return st_info != nullptr ? st_info->GetConcurrentVoipCallEnable() : false;
}

bool ResourceManager::IsTransitToNonLPIOnChargingSupported() {
    std::shared_ptr<VoiceUIPlatformInfo> vui_info =
        VoiceUIPlatformInfo::GetInstance();

    if (vui_info)
        return vui_info->GetTransitToNonLpiOnCharging();

    return false;
}

bool ResourceManager::CheckForForcedTransitToNonLPI() {
    if (IsTransitToNonLPIOnChargingSupported() && charging_state_)
      return true;

    return false;
}

std::shared_ptr<CaptureProfile> ResourceManager::GetASRCaptureProfileByPriority(
    StreamASR *s, std::shared_ptr<CaptureProfile> cap_prof_priority,
    std::string backend) {
    std::shared_ptr<CaptureProfile> cap_prof = nullptr;

    for (auto& str: active_streams_asr) {
        if (str == s) {
            continue;
        }

        if (!str->isActive())
            continue;

        cap_prof = str->GetCurrentCaptureProfile();
        if (!cap_prof) {
            PAL_ERR(LOG_TAG, "Failed to get capture profile");
            continue;
        } else if (cap_prof->GetBackend().compare(backend) != 0) {
            continue;
        } else if (cap_prof->ComparePriority(cap_prof_priority) >=
                   CAPTURE_PROFILE_PRIORITY_HIGH) {
            cap_prof_priority = cap_prof;
        }
    }

    return cap_prof_priority;
}

std::shared_ptr<CaptureProfile> ResourceManager::GetACDCaptureProfileByPriority(
    StreamACD *s, std::shared_ptr<CaptureProfile> cap_prof_priority,
    std::string backend) {
    std::shared_ptr<CaptureProfile> cap_prof = nullptr;

       for (auto& str: active_streams_acd) {
       // NOTE: input param s can be nullptr here
        if (str == s) {
            continue;
        }

        if (!str->isActive())
            continue;

        cap_prof = str->GetCurrentCaptureProfile();
        if (!cap_prof) {
            PAL_ERR(LOG_TAG, "Failed to get capture profile");
            continue;
        } else if (cap_prof->GetBackend().compare(backend) != 0) {
            continue;
        } else if (cap_prof->ComparePriority(cap_prof_priority) >=
                   CAPTURE_PROFILE_PRIORITY_HIGH) {
            cap_prof_priority = cap_prof;
        }
    }

    return cap_prof_priority;
}

std::shared_ptr<CaptureProfile> ResourceManager::GetSVACaptureProfileByPriority(
    StreamSoundTrigger *s, std::shared_ptr<CaptureProfile> cap_prof_priority,
    std::string backend) {
    std::shared_ptr<CaptureProfile> cap_prof = nullptr;

    for (auto& str: active_streams_st) {
        // NOTE: input param s can be nullptr here
        if (str == s) {
            continue;
        }

        /*
         * Ignore capture profile for streams in below states:
         * 1. sound model loaded but not started by sthal
         * 2. stop recognition called by sthal
         */
        if (!str->isStarted())
            continue;

        cap_prof = str->GetCurrentCaptureProfile();
        if (!cap_prof) {
            PAL_ERR(LOG_TAG, "Failed to get capture profile");
            continue;
        } else if ((cap_prof->GetDevId() == PAL_DEVICE_IN_HANDSET_MIC) ||
            (cap_prof->GetDevId() == PAL_DEVICE_IN_SPEAKER_MIC) ||
            (cap_prof->GetDevId() == PAL_DEVICE_IN_WIRED_HEADSET)) {
            continue;
        } else if (cap_prof->GetBackend().compare(backend) != 0) {
            continue;
        } else if (cap_prof->ComparePriority(cap_prof_priority) >=
                   CAPTURE_PROFILE_PRIORITY_HIGH) {
            cap_prof_priority = cap_prof;
        }
    }

    return cap_prof_priority;
}

std::shared_ptr<CaptureProfile> ResourceManager::GetSPDCaptureProfileByPriority(
    StreamSensorPCMData *s, std::shared_ptr<CaptureProfile> cap_prof_priority,
    std::string backend) {
    std::shared_ptr<CaptureProfile> cap_prof = nullptr;

    for (auto& str: active_streams_sensor_pcm_data) {
        // NOTE: input param s can be nullptr here
        if (str == s) {
            continue;
        }

        if (NULL != str) {
            cap_prof = str->GetCurrentCaptureProfile();
            if (!cap_prof) {
                PAL_ERR(LOG_TAG, "Failed to get capture profile");
                continue;
            } else if (cap_prof->GetBackend().compare(backend) != 0) {
                continue;
            } else if (cap_prof->ComparePriority(cap_prof_priority) >=
                       CAPTURE_PROFILE_PRIORITY_HIGH) {
                cap_prof_priority = cap_prof;
            }
        }
    }

    return cap_prof_priority;
}

std::shared_ptr<CaptureProfile> ResourceManager::GetCaptureProfileByPriority(
    Stream *s, std::string backend)
{
    struct pal_stream_attributes sAttr;
    StreamSoundTrigger *st_st = nullptr;
    StreamACD *st_acd = nullptr;
    StreamASR *st_asr = nullptr;
    StreamSensorPCMData *st_sns_pcm_data = nullptr;
    std::shared_ptr<CaptureProfile> cap_prof_priority = nullptr;
    int32_t status = 0;

    if (!s)
        goto get_priority;

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        return nullptr;
    }

    if (sAttr.type == PAL_STREAM_VOICE_UI)
        st_st = dynamic_cast<StreamSoundTrigger*>(s);
    else if (sAttr.type == PAL_STREAM_ACD)
        st_acd = dynamic_cast<StreamACD*>(s);
    else if (sAttr.type == PAL_STREAM_ASR)
        st_asr = dynamic_cast<StreamASR*>(s);
    else
        st_sns_pcm_data = dynamic_cast<StreamSensorPCMData*>(s);

get_priority:
    cap_prof_priority = GetSVACaptureProfileByPriority(st_st, cap_prof_priority, backend);
    cap_prof_priority = GetACDCaptureProfileByPriority(st_acd, cap_prof_priority, backend);
    cap_prof_priority = GetASRCaptureProfileByPriority(st_asr, cap_prof_priority, backend);
    return GetSPDCaptureProfileByPriority(st_sns_pcm_data, cap_prof_priority, backend);
}

bool ResourceManager::UpdateSoundTriggerCaptureProfile(Stream *s, bool is_active) {

    bool backend_update = false;
    std::shared_ptr<CaptureProfile> cap_prof = nullptr;
    std::shared_ptr<CaptureProfile> cap_prof_priority = nullptr;
    std::shared_ptr<CaptureProfile> common_cap_prof = nullptr;
    struct pal_stream_attributes sAttr;
    StreamSoundTrigger *st_st = nullptr;
    StreamACD *st_acd = nullptr;
    StreamASR *st_asr = nullptr;
    StreamSensorPCMData *st_sns_pcm_data = nullptr;
    int32_t status = 0;

    if (!s) {
        PAL_ERR(LOG_TAG, "Invalid stream");
        return false;
    }

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        return false;
    }

    if (sAttr.type == PAL_STREAM_VOICE_UI)
        st_st = dynamic_cast<StreamSoundTrigger*>(s);
    else if (sAttr.type == PAL_STREAM_ACD)
        st_acd = dynamic_cast<StreamACD*>(s);
    else if (sAttr.type == PAL_STREAM_ASR)
        st_asr = dynamic_cast<StreamASR*>(s);
    else if (sAttr.type == PAL_STREAM_SENSOR_PCM_DATA)
        st_sns_pcm_data = dynamic_cast<StreamSensorPCMData*>(s);
    else {
        PAL_ERR(LOG_TAG, "Error:%d Invalid stream type", -EINVAL);
        return false;
    }
    mResourceManagerMutex.lock();
    // backend config update
    if (is_active) {
        if (sAttr.type == PAL_STREAM_VOICE_UI)
            cap_prof = st_st->GetCurrentCaptureProfile();
        else if (sAttr.type == PAL_STREAM_ACD)
            cap_prof = st_acd->GetCurrentCaptureProfile();
        else if (sAttr.type == PAL_STREAM_ASR)
            cap_prof = st_asr->GetCurrentCaptureProfile();
        else
            cap_prof = st_sns_pcm_data->GetCurrentCaptureProfile();

        if (!cap_prof) {
            PAL_ERR(LOG_TAG, "Failed to get capture profile");
            mResourceManagerMutex.unlock();
            return false;
        }

        common_cap_prof = (cap_prof->GetBackend().compare("tx_macro") != 0) ?
                          SoundTriggerCaptureProfile : TXMacroCaptureProfile;
        if (!common_cap_prof) {
            common_cap_prof = cap_prof;
        } else if (cap_prof->ComparePriority(common_cap_prof) >=
                   CAPTURE_PROFILE_PRIORITY_HIGH){
            common_cap_prof = cap_prof;
            backend_update = true;
        }
        ((cap_prof->GetBackend().compare("tx_macro") != 0) ?
                          SoundTriggerCaptureProfile : TXMacroCaptureProfile) = common_cap_prof;
    } else {
        /* Updating SoundTriggerCaptureProfile for streams use VA Macro capture profiles */
        cap_prof_priority = GetCaptureProfileByPriority(s, "va_macro");
        if (!cap_prof_priority) {
            PAL_DBG(LOG_TAG, "No session active, reset VA Macro common capture profiles");
            SoundTriggerCaptureProfile = nullptr;
        } else if (cap_prof_priority->ComparePriority(SoundTriggerCaptureProfile) >=
                CAPTURE_PROFILE_PRIORITY_HIGH) {
                SoundTriggerCaptureProfile = cap_prof_priority;
                backend_update = true;
        }

        /* Updating TXMacroCaptureProfile for streams use TX Macro capture profiles */
        cap_prof_priority = GetCaptureProfileByPriority(s, "tx_macro");
        if (!cap_prof_priority) {
            PAL_DBG(LOG_TAG, "No session active, reset TX Macro common capture profiles");
            TXMacroCaptureProfile = nullptr;
        } else if (cap_prof_priority->ComparePriority(TXMacroCaptureProfile) >=
                CAPTURE_PROFILE_PRIORITY_HIGH) {
                TXMacroCaptureProfile = cap_prof_priority;
                backend_update = true;
        }
    }
    mResourceManagerMutex.unlock();

    return backend_update;
}

void ResourceManager::SwitchSoundTriggerDevices(bool connect_state,
                                                pal_device_id_t st_device) {
    pal_device_id_t device_to_disconnect;
    pal_device_id_t device_to_connect;
    std::vector<pal_stream_type_t> st_streams;
    std::shared_ptr<CaptureProfile> cap_prof_priority = nullptr;
    std::shared_ptr<SoundTriggerPlatformInfo> st_info = nullptr;

    PAL_DBG(LOG_TAG, "Enter");

    /*
     * Voice UI, ACD, ASR and Sensor PCM Data(SPD)
     * share the sound trigger platform info.
     */
    st_info = SoundTriggerPlatformInfo::GetInstance();
    if (st_info && (false == st_info->GetSupportDevSwitch())) {
        PAL_INFO(LOG_TAG, "Device switch not supported");
        goto exit;
    }

    /* Updating SoundTriggerCaptureProfile for streams use VA Macro capture profiles */
    SoundTriggerCaptureProfile = nullptr;
    cap_prof_priority = GetCaptureProfileByPriority(nullptr, "va_macro");
    if (!cap_prof_priority) {
        PAL_DBG(LOG_TAG, "No ST session active, reset VA Macro capture profile");
        SoundTriggerCaptureProfile = nullptr;
    } else if (cap_prof_priority->ComparePriority(SoundTriggerCaptureProfile) >=
            CAPTURE_PROFILE_PRIORITY_HIGH) {
        SoundTriggerCaptureProfile = cap_prof_priority;
    }

    /* Updating TXMacroCaptureProfile for streams use TX Macro capture profiles */
    TXMacroCaptureProfile = nullptr;
    cap_prof_priority = GetCaptureProfileByPriority(nullptr, "tx_macro");
    if (!cap_prof_priority) {
        PAL_DBG(LOG_TAG, "No ST session active, reset TX Macro capture profile");
        TXMacroCaptureProfile = nullptr;
    } else if (cap_prof_priority->ComparePriority(TXMacroCaptureProfile) >=
            CAPTURE_PROFILE_PRIORITY_HIGH) {
        TXMacroCaptureProfile = cap_prof_priority;
    }

    if (true == connect_state) {
        device_to_connect = st_device;
        device_to_disconnect = PAL_DEVICE_IN_HANDSET_VA_MIC;
    } else {
        device_to_connect = PAL_DEVICE_IN_HANDSET_VA_MIC;
        device_to_disconnect = st_device;
    }

    /* This is called from mResourceManagerMutex lock, unlock before calling
     * HandleDetectionStreamAction */
    mResourceManagerMutex.unlock();
    mActiveStreamMutex.lock();
    if (active_streams_st.size())
        st_streams.push_back(PAL_STREAM_VOICE_UI);
    if (active_streams_acd.size())
        st_streams.push_back(PAL_STREAM_ACD);
    if (active_streams_asr.size())
        st_streams.push_back(PAL_STREAM_ASR);
    if (active_streams_sensor_pcm_data.size())
        st_streams.push_back(PAL_STREAM_SENSOR_PCM_DATA);

    for (pal_stream_type_t st_stream_type : st_streams) {
        /* Disconnect device for all active sound trigger streams */
        HandleDetectionStreamAction(st_stream_type, ST_HANDLE_DISCONNECT_DEVICE,
                                    (void *)&device_to_disconnect);
    }

    for (pal_stream_type_t st_stream_type : st_streams) {
        /* Connect device for all active sound trigger streams */
        HandleDetectionStreamAction(st_stream_type, ST_HANDLE_CONNECT_DEVICE,
                                    (void *)&device_to_connect);
    }
    mActiveStreamMutex.unlock();
    mResourceManagerMutex.lock();

exit:
    PAL_DBG(LOG_TAG, "Exit");
}

/* NOTE: there should be only one callback for each pcm id
 * so when new different callback register with same pcm id
 * older one will be overwritten
 */
int ResourceManager::registerMixerEventCallback(const std::vector<int> &DevIds,
                                                session_callback callback,
                                                uint64_t cookie,
                                                bool is_register) {
    int status = 0;
    std::map<int, std::pair<session_callback, uint64_t>>::iterator it;

    if (!callback || DevIds.size() <= 0) {
        PAL_ERR(LOG_TAG, "Invalid callback or pcm ids");
        return -EINVAL;
    }

    mResourceManagerMutex.lock();
    if (mixerEventRegisterCount == 0 && !is_register) {
        PAL_ERR(LOG_TAG, "Cannot deregister unregistered callback");
        mResourceManagerMutex.unlock();
        return -EINVAL;
    }

    if (is_register) {
        for (int i = 0; i < DevIds.size(); i++) {
            it = mixerEventCallbackMap.find(DevIds[i]);
            if (it != mixerEventCallbackMap.end()) {
                PAL_DBG(LOG_TAG, "callback exists for pcm id %d, overwrite",
                    DevIds[i]);
                mixerEventCallbackMap.erase(it);
            }
            mixerEventCallbackMap.insert(std::make_pair(DevIds[i],
                std::make_pair(callback, cookie)));

        }
        mixerEventRegisterCount++;
    } else {
        for (int i = 0; i < DevIds.size(); i++) {
            it = mixerEventCallbackMap.find(DevIds[i]);
            if (it != mixerEventCallbackMap.end()) {
                PAL_DBG(LOG_TAG, "callback found for pcm id %d, remove",
                    DevIds[i]);
                if (callback == it->second.first) {
                    mixerEventCallbackMap.erase(it);
                } else {
                    PAL_ERR(LOG_TAG, "No matching callback found for pcm id %d",
                        DevIds[i]);
                }
            } else {
                PAL_ERR(LOG_TAG, "No callback found for pcm id %d", DevIds[i]);
            }
        }
        mixerEventRegisterCount--;
    }

    mResourceManagerMutex.unlock();
    return status;
}

void ResourceManager::mixerEventWaitThreadLoop(
    std::shared_ptr<ResourceManager> rm) {
    int ret = 0;
    struct ctl_event mixer_event = {0, {.data8 = {0}}};
    struct mixer *mixer = nullptr;

    ret = rm->getVirtualAudioMixer(&mixer);
    if (ret) {
        PAL_ERR(LOG_TAG, "Failed to get audio mxier");
        return;
    }

    PAL_VERBOSE(LOG_TAG, "subscribing for event");
    mixer_subscribe_events(mixer, 1);

    while (1) {
        PAL_VERBOSE(LOG_TAG, "going to wait for event");
        ret = mixer_wait_event(mixer, -1);
        PAL_VERBOSE(LOG_TAG, "mixer_wait_event returns %d", ret);
        if (ret <= 0) {
            PAL_DBG(LOG_TAG, "mixer_wait_event err! ret = %d", ret);
        } else if (ret > 0) {
            ret = mixer_read_event(mixer, &mixer_event);
            if (ret >= 0) {
                if (strstr((char *)mixer_event.data.elem.id.name, (char *)"event")) {
                    PAL_INFO(LOG_TAG, "Event Received %s",
                             mixer_event.data.elem.id.name);
                    ret = rm->handleMixerEvent(mixer,
                        (char *)mixer_event.data.elem.id.name);
                } else
                    PAL_VERBOSE(LOG_TAG, "Unwanted event, Skipping");
            } else {
                PAL_DBG(LOG_TAG, "mixer_read failed, ret = %d", ret);
            }
        }
        if (ResourceManager::mixerClosed) {
            PAL_INFO(LOG_TAG, "mixerClosed, closed mixerEventWaitThreadLoop");
            return;
        }
    }
    PAL_VERBOSE(LOG_TAG, "unsubscribing for event");
    mixer_subscribe_events(mixer, 0);
}

int ResourceManager::handleMixerEvent(struct mixer *mixer, char *mixer_str) {
    int status = 0;
    int pcm_id = 0;
    uint64_t cookie = 0;
    session_callback session_cb = nullptr;
    std::string event_str(mixer_str);
    // TODO: hard code in common defs
    std::string pcm_prefix = "PCM";
    std::string compress_prefix = "COMPRESS";
    std::string event_suffix = "event";
    size_t prefix_idx = 0;
    size_t suffix_idx = 0;
    size_t length = 0;
    struct mixer_ctl *ctl = nullptr;
    char *buf = nullptr;
    unsigned int num_values;
    struct agm_event_cb_params *params = nullptr;
    std::map<int, std::pair<session_callback, uint64_t>>::iterator it;

    PAL_DBG(LOG_TAG, "Enter");
    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s", mixer_str);
        status = -EINVAL;
        goto exit;
    }

    // parse event payload
    num_values = mixer_ctl_get_num_values(ctl);
    PAL_VERBOSE(LOG_TAG, "num_values: %d", num_values);
    buf = (char *)calloc(1, num_values);
    if (!buf) {
        PAL_ERR(LOG_TAG, "Failed to allocate buf");
        status = -ENOMEM;
        goto exit;
    }

    status = mixer_ctl_get_array(ctl, buf, num_values);
    if (status < 0) {
        PAL_ERR(LOG_TAG, "Failed to mixer_ctl_get_array");
        goto exit;
    }

    params = (struct agm_event_cb_params *)buf;
    PAL_DBG(LOG_TAG, "source module id %x, event id %d, payload size %d",
            params->source_module_id, params->event_id,
            params->event_payload_size);

    if (!params->source_module_id) {
        PAL_ERR(LOG_TAG, "Invalid source module id");
        goto exit;
    }

    // NOTE: event we get should be in format like "PCM100 event"
    prefix_idx = event_str.find(pcm_prefix);
    if (prefix_idx == event_str.npos) {
        prefix_idx = event_str.find(compress_prefix);
        if (prefix_idx == event_str.npos) {
            /* search for Events with VoiceModel.. pattern */
            std::string voicemodel_searched =  event_str.substr(0,(event_str.size()-event_suffix.size()-1));
            for (std::vector<deviceCap>::iterator it = devInfo.begin() ; it != devInfo.end(); ++it){
                if (!strcmp(it->name, voicemodel_searched.c_str())) {
                    pcm_id = it->deviceId;
                    goto acquire_event_callback;
                }
            }
            PAL_ERR(LOG_TAG, "Invalid mixer event");
            status = -EINVAL;
            goto exit;
        } else {
            prefix_idx += compress_prefix.length();
        }
    } else {
        prefix_idx += pcm_prefix.length();
    }

    suffix_idx = event_str.find(event_suffix);
    if (suffix_idx == event_str.npos || suffix_idx - prefix_idx <= 1) {
        PAL_ERR(LOG_TAG, "Invalid mixer event");
        status = -EINVAL;
        goto exit;
    }

    length = suffix_idx - prefix_idx;
    pcm_id = std::stoi(event_str.substr(prefix_idx, length));

acquire_event_callback:
    // acquire callback/cookie with pcm dev id
    it = mixerEventCallbackMap.find(pcm_id);
    if (it != mixerEventCallbackMap.end()) {
        session_cb = it->second.first;
        cookie = it->second.second;
    }

    if (!session_cb) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid session callback");
        goto exit;
    }

    // callback
    if (params->event_id == AGM_EVENT_EARLY_EOS ||
        params->event_id == AGM_EVENT_EARLY_EOS_INTERNAL) {
         PAL_DBG(LOG_TAG, "Event will be handled by offload Thread loop");
    } else {
        session_cb(cookie, params->event_id, (void *)params->event_payload,
                 params->event_payload_size);
    }

#ifdef SEC_AUDIO_MUTE_DETECTION
    if (params->event_id == EVENT_ID_SILENCE_DETECTION) {
        //print and dump kernel info.
        rm->getKernelConfigInfo(TX_CONFIG_INFO);
        rm->getKernelConfigInfo(RX_CONFIG_INFO);
        rm->setKernelRegDump(TX_REG_DUMP);
    }
#endif

exit:
    if (buf)
        free(buf);
    PAL_DBG(LOG_TAG, "Exit, status %d", status);

    return status;
}

// NOTE: This api should be called with mActiveStreamMutex locked
int ResourceManager::HandleDetectionStreamAction(pal_stream_type_t type, int32_t action, void *data)
{
    int status = 0;
    pal_stream_attributes st_attr;

    if ((type == PAL_STREAM_VOICE_UI &&
         !active_streams_st.size()) ||
        (type == PAL_STREAM_ACD &&
         !active_streams_acd.size()) ||
        (type == PAL_STREAM_ASR &&
         !active_streams_asr.size()) ||
        (type == PAL_STREAM_SENSOR_PCM_DATA &&
         !active_streams_sensor_pcm_data.size())) {
        PAL_VERBOSE(LOG_TAG, "No active stream for type %d, skip action", type);
        return 0;
    }

    PAL_DBG(LOG_TAG, "Enter");
    for (auto& str: mActiveStreams) {
        if (!isStreamActive(str, mActiveStreams))
            continue;

        str->getStreamAttributes(&st_attr);
        if (st_attr.type != type)
            continue;

        switch (action) {
            case ST_PAUSE:
                if (str != (Stream *)data) {
                    status = str->Pause();
                    if (status)
                        PAL_ERR(LOG_TAG, "Failed to pause stream");
                }
                break;
            case ST_RESUME:
                if (str != (Stream *)data) {
                    status = str->Resume();
                    if (status)
                        PAL_ERR(LOG_TAG, "Failed to do resume stream");
                }
                break;
            case ST_HANDLE_CONCURRENT_STREAM: {
                bool enable = *(bool *)data;
                status = str->HandleConcurrentStream(enable);
                if (status)
                    PAL_ERR(LOG_TAG, "Failed to stop/unload stream");
                }
                break;
            case ST_HANDLE_DISCONNECT_DEVICE: {
                pal_device_id_t device_to_disconnect = *(pal_device_id_t*)data;
                status = str->DisconnectDevice(device_to_disconnect);
                if (status)
                    PAL_ERR(LOG_TAG, "Failed to disconnect device %d",
                            device_to_disconnect);
                }
                break;
            case ST_HANDLE_CONNECT_DEVICE: {
                pal_device_id_t device_to_connect = *(pal_device_id_t*)data;
                status = str->ConnectDevice(device_to_connect);
                if (status)
                    PAL_ERR(LOG_TAG, "Failed to connect device %d",
                            device_to_connect);
                }
                break;
            case ST_INTERNAL_PAUSE:
                if (str != (Stream *)data) {
                    status = str->Pause(true);
                    if (status)
                        PAL_ERR(LOG_TAG, "Internal pause stream failed");
                }
                break;
            case ST_INTERNAL_RESUME:
                if (str != (Stream *)data) {
                    status = str->Resume(true);
                    if (status)
                        PAL_ERR(LOG_TAG, "Internal resume stream failed");
                }
                break;
            default:
                break;
        }
    }
    PAL_DBG(LOG_TAG, "Exit, status %d", status);

    return status;
}

int ResourceManager::StopOtherDetectionStreams(void *st) {
    HandleDetectionStreamAction(PAL_STREAM_VOICE_UI, ST_INTERNAL_PAUSE, st);
    HandleDetectionStreamAction(PAL_STREAM_ACD, ST_PAUSE, st);
    HandleDetectionStreamAction(PAL_STREAM_ASR, ST_PAUSE, st);
    HandleDetectionStreamAction(PAL_STREAM_SENSOR_PCM_DATA, ST_PAUSE, st);
    return 0;
}

int ResourceManager::StartOtherDetectionStreams(void *st) {
    HandleDetectionStreamAction(PAL_STREAM_VOICE_UI, ST_INTERNAL_RESUME, st);
    HandleDetectionStreamAction(PAL_STREAM_ACD, ST_RESUME, st);
    HandleDetectionStreamAction(PAL_STREAM_ASR, ST_RESUME, st);
    HandleDetectionStreamAction(PAL_STREAM_SENSOR_PCM_DATA, ST_RESUME, st);
    return 0;
}

void ResourceManager::GetConcurrencyInfo(pal_stream_type_t st_type,
                         pal_stream_type_t in_type, pal_stream_direction_t dir,
                         bool *rx_conc, bool *tx_conc, bool *conc_en)
{
    bool voice_conc_enable = IsVoiceCallConcurrencySupported();
    bool voip_conc_enable = IsVoipConcurrencySupported();
    bool low_latency_bargein_enable = IsLowLatencyBargeinSupported();
    bool audio_capture_conc_enable = IsAudioCaptureConcurrencySupported();

    if (dir == PAL_AUDIO_OUTPUT) {
        if (in_type == PAL_STREAM_LOW_LATENCY && !low_latency_bargein_enable) {
            PAL_VERBOSE(LOG_TAG, "Ignore low latency playback stream");
        } else if (in_type == PAL_STREAM_SENSOR_PCM_RENDERER) {
            PAL_VERBOSE(LOG_TAG, "Ignore sensor renderer stream");
        } else {
            *rx_conc = true;
        }
    }

    /*
     * Generally voip/voice rx stream comes with related tx streams,
     * so there's no need to switch to NLPI for voip/voice rx stream
     * if corresponding voip/voice tx stream concurrency is not supported.
     * Also note that capture concurrency has highest proirity that
     * when capture concurrency is disabled then concurrency for voip
     * and voice call should also be disabled even voice_conc_enable
     * or voip_conc_enable is set to true.
     */
    if (in_type == PAL_STREAM_VOICE_CALL) {
        *tx_conc = true;
        *rx_conc = true;
        if (!audio_capture_conc_enable || !voice_conc_enable) {
            PAL_DBG(LOG_TAG, "pause on voice concurrency");
            *conc_en = false;
        }
    } else if (in_type == PAL_STREAM_VOIP_TX) {
        *tx_conc = true;
        if (!audio_capture_conc_enable || !voip_conc_enable) {
            PAL_DBG(LOG_TAG, "pause on voip concurrency");
            *conc_en = false;
        }
    } else if (dir == PAL_AUDIO_INPUT &&
               (in_type != PAL_STREAM_ACD &&
                in_type != PAL_STREAM_SENSOR_PCM_DATA &&
                in_type != PAL_STREAM_CONTEXT_PROXY  &&
                in_type != PAL_STREAM_VOICE_UI &&
                in_type != PAL_STREAM_ASR)) {
        *tx_conc = true;
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
        if (!audio_capture_conc_enable && in_type != PAL_STREAM_PROXY && in_type != PAL_STREAM_HOTWORD) {
            PAL_DBG(LOG_TAG, "pause on audio capture concurrency");
            *conc_en = false;
        }
#else
        if (!audio_capture_conc_enable && in_type != PAL_STREAM_PROXY) {
            PAL_DBG(LOG_TAG, "pause on audio capture concurrency");
            *conc_en = false;
        }
#endif
    } else if (in_type == PAL_STREAM_LOOPBACK){
        *tx_conc = true;
        *rx_conc = true;
        if (!audio_capture_conc_enable) {
            PAL_DBG(LOG_TAG, "pause on LOOPBACK concurrency");
            *conc_en = false;
        }
    } else if (in_type == PAL_STREAM_ULTRASOUND){
        *rx_conc = true;
    }

    PAL_INFO(LOG_TAG, "stream type %d Tx conc %d, Rx conc %d, concurrency%s allowed",
        in_type, *tx_conc, *rx_conc, *conc_en? "" : " not");
}

void ResourceManager::HandleStreamPauseResume(pal_stream_type_t st_type, bool active)
{
    int32_t *local_dis_count;

    if (st_type == PAL_STREAM_ACD)
        local_dis_count = &ACDConcurrencyDisableCount;
    else if (st_type == PAL_STREAM_ASR)
        local_dis_count = &ASRConcurrencyDisableCount;
    else if (st_type == PAL_STREAM_VOICE_UI)
        local_dis_count = &concurrencyDisableCount;
    else if (st_type == PAL_STREAM_SENSOR_PCM_DATA)
        local_dis_count = &SNSPCMDataConcurrencyDisableCount;
    else
        return;

    if (active) {
        if (++(*local_dis_count) == 1) {
            // pause all sva/acd streams
            HandleDetectionStreamAction(st_type, ST_PAUSE, NULL);
        }
    } else {
        if ((*local_dis_count) < 0) {
            (*local_dis_count) = 0;
        } else if ((*local_dis_count) > 0 && --(*local_dis_count) == 0) {
            // resume all sva/acd streams
            HandleDetectionStreamAction(st_type, ST_RESUME, NULL);
        }
    }
}

void ResourceManager::forceSwitchSoundTriggerStreams(bool active) {

    if (!PAL_CARD_STATUS_DOWN(cardState))
        std::lock_guard<std::mutex> lock(mActiveStreamMutex);

    std::vector<pal_stream_type_t> st_streams;

    if (active_streams_st.size())
        st_streams.push_back(PAL_STREAM_VOICE_UI);
    if (active_streams_acd.size())
        st_streams.push_back(PAL_STREAM_ACD);
    if (active_streams_sensor_pcm_data.size())
        st_streams.push_back(PAL_STREAM_SENSOR_PCM_DATA);

    if (checkAndUpdateDeferSwitchState(active)) {
        PAL_DBG(LOG_TAG, "Switch is deferred");
    } else {
        handleConcurrentStreamSwitch(st_streams);
    }
}

/* This function should be called with mActiveStreamMutex lock acquired */
void ResourceManager::handleConcurrentStreamSwitch(std::vector<pal_stream_type_t>& st_streams)
{
    std::shared_ptr<CaptureProfile> cap_prof_priority = nullptr;

    // update common capture profile after use_lpi_ updated for all streams
    if (st_streams.size()) {
        mResourceManagerMutex.lock();
        /* Updating SoundTriggerCaptureProfile for streams use VA Macro capture profiles */
        SoundTriggerCaptureProfile = nullptr;
        cap_prof_priority = GetCaptureProfileByPriority(nullptr, "va_macro");
        if (!cap_prof_priority) {
            PAL_DBG(LOG_TAG, "No session active, reset VA Macro common capture profile");
            SoundTriggerCaptureProfile = nullptr;
        } else if (cap_prof_priority->ComparePriority(SoundTriggerCaptureProfile) >=
                CAPTURE_PROFILE_PRIORITY_HIGH) {
            SoundTriggerCaptureProfile = cap_prof_priority;
        }

        /* Updating TXMacroCaptureProfile for streams use TX Macro capture profiles */
        TXMacroCaptureProfile = nullptr;
        cap_prof_priority = GetCaptureProfileByPriority(nullptr, "tx_macro");
        if (!cap_prof_priority) {
            PAL_DBG(LOG_TAG, "No session active, reset TX Macro common capture profile");
            TXMacroCaptureProfile = nullptr;
        } else if (cap_prof_priority->ComparePriority(TXMacroCaptureProfile) >=
                CAPTURE_PROFILE_PRIORITY_HIGH) {
            TXMacroCaptureProfile = cap_prof_priority;
        }
        mResourceManagerMutex.unlock();
    }

    for (pal_stream_type_t st_stream_type_to_stop : st_streams) {
        // stop/unload SVA/ACD/Sensor PCM Data streams
        bool action = false;
        PAL_DBG(LOG_TAG, "stop/unload stream type %d", st_stream_type_to_stop);
        HandleDetectionStreamAction(st_stream_type_to_stop,
            ST_HANDLE_CONCURRENT_STREAM, (void *)&action);
    }

    for (pal_stream_type_t st_stream_type_to_start : st_streams) {
        // load/start SVA/ACD/Sensor PCM Data streams
        bool action = true;
        PAL_DBG(LOG_TAG, "load/start stream type %d", st_stream_type_to_start);
        HandleDetectionStreamAction(st_stream_type_to_start,
            ST_HANDLE_CONCURRENT_STREAM, (void *)&action);
    }
}

bool ResourceManager::checkAndUpdateDeferSwitchState(bool stream_active)
{
    std::unique_lock<std::mutex> lck(vui_switch_mutex_);

    /*
     * When switching from NLPI to LPI:
     * 1. If low latency bargein is enabled, nlpi to lpi switch
     *    will be delayed by 5s until sleep is done in thread
     *    voiceUIDeferredSwitchLoop.
     * 2. Else if there's any VoiceUI stream in buffering, delay
     *    switch until buffering is done.
     *
     * When switching from LPI to NLPI:
     * 1. If there's any VoiceUI stream in buffering, delay switch
     *    until buffering is done.
     * 2. Additionally if low latency bargein is enabled and there's
     *    pending NLPI to LPI switch, then skip the pending switch
     *    and exit the sleep in voiceUIDeferredSwitchLoop.
     */
    if (!stream_active) {
        if (IsLowLatencyBargeinSupported()) {
            deferredSwitchState =
                (deferredSwitchState == DEFER_LPI_NLPI_SWITCH) ? NO_DEFER :
                 DEFER_NLPI_LPI_SWITCH;
            PAL_INFO(LOG_TAG,
                "Low latency bargein enabled, defer NLPI->LPI switch, deferred state:%d",
                deferredSwitchState);
            deferred_switch_cnt_ = NLPI_LPI_SWITCH_DELAY_SEC;
            vui_switch_cv_.notify_all();
            return true;
        } else if (isAnyVUIStreamBuffering()) {
            deferredSwitchState =
                (deferredSwitchState == DEFER_LPI_NLPI_SWITCH) ? NO_DEFER :
                 DEFER_NLPI_LPI_SWITCH;
            PAL_INFO(LOG_TAG,
                "VUI stream in buffering, defer NLPI->LPI switch, deferred state:%d",
                deferredSwitchState);
            return true;
        }
    } else {
        if (isAnyVUIStreamBuffering()) {
            deferredSwitchState =
                (deferredSwitchState == DEFER_NLPI_LPI_SWITCH) ? NO_DEFER :
                 DEFER_LPI_NLPI_SWITCH;
            PAL_INFO(LOG_TAG,
                "VUI stream in buffering, defer LPI->NLPI switch, deferred state:%d,"
                " LPI will be used until buffering done, hence EC won't be applied",
                deferredSwitchState);
            return true;
        }
        if (IsLowLatencyBargeinSupported() &&
            deferredSwitchState == DEFER_NLPI_LPI_SWITCH) {
            deferredSwitchState = NO_DEFER;
            deferred_switch_cnt_ = -1;
            PAL_INFO(LOG_TAG,
                "Cancel pending NLPI to LPI switch as new concurrency coming");
            return true;
        }
    }

    return false;
}

void ResourceManager::voiceUIDeferredSwitchLoop(std::shared_ptr<ResourceManager> rm)
{
    bool is_wake_lock_acquired = false;
    PAL_INFO(LOG_TAG, "Enter");
    std::unique_lock<std::mutex> lck(rm->vui_switch_mutex_);

    while (!rm->vui_switch_thread_exit_) {
        if (deferred_switch_cnt_ < 0) {
            if (is_wake_lock_acquired) {
                rm->releaseWakeLock();
                is_wake_lock_acquired = false;
            }
            rm->vui_switch_cv_.wait(lck);
            rm->acquireWakeLock();
            is_wake_lock_acquired = true;
        }

        if (rm->vui_switch_thread_exit_) {
            if (is_wake_lock_acquired)
                rm->releaseWakeLock();
            break;
        }

        if (deferred_switch_cnt_ > 0) {
            deferred_switch_cnt_--;
            lck.unlock();
            sleep(NLPI_LPI_SWITCH_SLEEP_INTERVAL_SEC);
            lck.lock();
        }

        if (deferred_switch_cnt_ == 0) {
            lck.unlock();
            rm->handleDeferredSwitch();
            lck.lock();
            if (deferred_switch_cnt_ == 0)
                deferred_switch_cnt_ = -1;
        }
    }
}

void ResourceManager::handleDeferredSwitch()
{
    int32_t status = 0;
    bool active = false;
    std::vector<pal_stream_type_t> st_streams;
    do {
        status = mActiveStreamMutex.try_lock();
    } while (!status && cardState == CARD_STATUS_ONLINE);

    if (cardState != CARD_STATUS_ONLINE) {
        if (status)
            mActiveStreamMutex.unlock();
        PAL_DBG(LOG_TAG, "Sound card is offline");
        return;
    }

    PAL_DBG(LOG_TAG, "enter, isAnyVUIStreambuffering:%d deferred state:%d",
        isAnyVUIStreamBuffering(), deferredSwitchState);

    if (!isAnyVUIStreamBuffering() && deferredSwitchState != NO_DEFER) {
        if (deferredSwitchState == DEFER_LPI_NLPI_SWITCH)
            active = true;
        else if (deferredSwitchState == DEFER_NLPI_LPI_SWITCH)
            active = false;

        use_lpi_ = !active;

        if (active_streams_st.size())
            st_streams.push_back(PAL_STREAM_VOICE_UI);
        if (active_streams_acd.size())
            st_streams.push_back(PAL_STREAM_ACD);
        if (active_streams_asr.size())
            st_streams.push_back(PAL_STREAM_ASR);
        if (active_streams_sensor_pcm_data.size())
            st_streams.push_back(PAL_STREAM_SENSOR_PCM_DATA);

        handleConcurrentStreamSwitch(st_streams);
        // reset the defer switch state after handling LPI/NLPI switch
        deferredSwitchState = NO_DEFER;
    }
    mActiveStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit");
}

bool ResourceManager::isAnyVUIStreamBuffering()
{
    for (auto& str: active_streams_st) {
        if (str->IsStreamInBuffering())
            return true;
    }
    return false;
}

void ResourceManager::ConcurrentStreamStatus(pal_stream_type_t type,
                                             pal_stream_direction_t dir,
                                             bool active)
{
    HandleConcurrencyForSoundTriggerStreams(type, dir, active);
}

void ResourceManager::HandleConcurrencyForSoundTriggerStreams(pal_stream_type_t type,
                                                             pal_stream_direction_t dir,
                                                             bool active)
{
    std::vector<pal_stream_type_t> st_streams;
    bool do_st_stream_switch = false;
    bool use_lpi_temp = use_lpi_;
    bool st_stream_conc_en = true;
    bool notify_resources_available = false;

    mActiveStreamMutex.lock();
    PAL_DBG(LOG_TAG, "Enter, stream type %d, direction %d, active %d", type, dir, active);

    if (deferredSwitchState == DEFER_LPI_NLPI_SWITCH) {
        use_lpi_temp = false;
    } else if (deferredSwitchState == DEFER_NLPI_LPI_SWITCH) {
        use_lpi_temp = true;
    }

    st_streams.push_back(PAL_STREAM_VOICE_UI);
    st_streams.push_back(PAL_STREAM_ACD);
    st_streams.push_back(PAL_STREAM_ASR);
    st_streams.push_back(PAL_STREAM_SENSOR_PCM_DATA);

    for (pal_stream_type_t st_stream_type : st_streams) {
        bool st_stream_tx_conc = false;
        bool st_stream_rx_conc = false;

        GetConcurrencyInfo(st_stream_type, type, dir,
                           &st_stream_rx_conc, &st_stream_tx_conc, &st_stream_conc_en);

        if (!st_stream_conc_en) {
            if (st_stream_type == PAL_STREAM_VOICE_UI &&
                concurrencyDisableCount == 1 && !active)
                notify_resources_available = true;
            HandleStreamPauseResume(st_stream_type, active);
            continue;
        }
        if (st_stream_tx_conc) {
            if (active)
                TxconcurrencyEnableCount++;
            else if (TxconcurrencyEnableCount > 0)
                TxconcurrencyEnableCount--;
        }
        if (st_stream_conc_en && (st_stream_tx_conc || st_stream_rx_conc)) {
            if (!IsLPISupported() || !isNLPISwitchSupported()) {
                PAL_INFO(LOG_TAG,
                         "Skip switch as st_stream %d LPI disabled/NLPI switch disabled", st_stream_type);
            } else if (active) {
                if ((PAL_STREAM_VOICE_UI == st_stream_type && ++concurrencyEnableCount == 1) ||
                    (PAL_STREAM_ACD == st_stream_type && ++ACDConcurrencyEnableCount == 1) ||
                    (PAL_STREAM_ASR == st_stream_type && ++ASRConcurrencyEnableCount == 1) ||
                    (PAL_STREAM_SENSOR_PCM_DATA == st_stream_type && ++SNSPCMDataConcurrencyEnableCount == 1)) {
                    if (use_lpi_temp) {
                        do_st_stream_switch = true;
                        use_lpi_temp = false;
                    }
                }
            } else {
                if ((PAL_STREAM_VOICE_UI == st_stream_type && --concurrencyEnableCount == 0) ||
                    (PAL_STREAM_ACD == st_stream_type && --ACDConcurrencyEnableCount == 0) ||
                    (PAL_STREAM_ASR == st_stream_type && --ASRConcurrencyEnableCount == 0) ||
                    (PAL_STREAM_SENSOR_PCM_DATA == st_stream_type && --SNSPCMDataConcurrencyEnableCount == 0)) {
                    if (!(active_streams_st.size() && charging_state_ && IsTransitToNonLPIOnChargingSupported())) {
                        do_st_stream_switch = true;
                        use_lpi_temp = true;
                    }
                }
            }
        }
    }

    /* Reset enable counts to 0 if they are negative */
    if (concurrencyEnableCount < 0)
        concurrencyEnableCount = 0;
    if (ACDConcurrencyEnableCount < 0)
        ACDConcurrencyEnableCount = 0;
    if (ASRConcurrencyEnableCount < 0)
        ASRConcurrencyEnableCount = 0;
    if (SNSPCMDataConcurrencyEnableCount < 0)
        SNSPCMDataConcurrencyEnableCount = 0;

    if (do_st_stream_switch) {
        if (checkAndUpdateDeferSwitchState(active)) {
            PAL_DBG(LOG_TAG, "Switch is deferred");
        } else {
            use_lpi_ = use_lpi_temp;
            handleConcurrentStreamSwitch(st_streams);
        }
    }

    mActiveStreamMutex.unlock();

    /*
     * The usecases using ST framework register the onResourcesAvailable callback.
     * Notify the framework upon concurrency is inactive.
     */
    if (onResourceAvailCb && notify_resources_available) {
        onResourceAvailCb(onResourceAvailCookie);
    }

    PAL_DBG(LOG_TAG, "Exit");
}

std::shared_ptr<Device> ResourceManager::getActiveEchoReferenceRxDevices_l(
    Stream *tx_str)
{
    int status = 0;
    int deviceId = 0;
    std::shared_ptr<Device> rx_device = nullptr;
    std::shared_ptr<Device> tx_device = nullptr;
    struct pal_stream_attributes tx_attr;
    struct pal_stream_attributes rx_attr;
    std::vector <std::shared_ptr<Device>> tx_device_list;
    std::vector <std::shared_ptr<Device>> rx_device_list;

    PAL_DBG(LOG_TAG, "Enter");

    // check stream direction
    status = tx_str->getStreamAttributes(&tx_attr);
    if (status) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        goto exit;
    }
    if (tx_attr.direction != PAL_AUDIO_INPUT) {
        PAL_ERR(LOG_TAG, "invalid stream direction %d", tx_attr.direction);
        status = -EINVAL;
        goto exit;
    }

    // get associated device list
    status = tx_str->getAssociatedDevices(tx_device_list);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to get associated device, status %d", status);
        goto exit;
    }

    for (auto& rx_str: mActiveStreams) {
        rx_str->getStreamAttributes(&rx_attr);
        rx_device_list.clear();
        if (rx_attr.direction != PAL_AUDIO_INPUT) {
            if (!getEcRefStatus(tx_attr.type, rx_attr.type)) {
                PAL_DBG(LOG_TAG, "No need to enable ec ref for rx %d tx %d",
                        rx_attr.type, tx_attr.type);
                continue;
            }
            rx_str->getAssociatedDevices(rx_device_list);
            for (int i = 0; i < rx_device_list.size(); i++) {
                if (!isDeviceActive_l(rx_device_list[i], rx_str) ||
                    !(rx_str->getCurState() == STREAM_STARTED ||
                      rx_str->getCurState() == STREAM_PAUSED))
                    continue;
                deviceId = rx_device_list[i]->getSndDeviceId();
                if (deviceId > PAL_DEVICE_OUT_MIN &&
                    deviceId < PAL_DEVICE_OUT_MAX)
                    rx_device = rx_device_list[i];
                else
                    rx_device = nullptr;
                for (int j = 0; j < tx_device_list.size(); j++) {
                    tx_device = tx_device_list[j];
                    if (checkECRef(rx_device, tx_device))
                        goto exit;
                }
            }
            rx_device = nullptr;
        } else {
            continue;
        }
    }

exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return rx_device;
}

std::shared_ptr<Device> ResourceManager::getActiveEchoReferenceRxDevices(
    Stream *tx_str)
{
    std::shared_ptr<Device> rx_device = nullptr;
    PAL_DBG(LOG_TAG, "Enter.");
    mResourceManagerMutex.lock();
    rx_device = getActiveEchoReferenceRxDevices_l(tx_str);
    mResourceManagerMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit.");
    return rx_device;
}

std::vector<Stream*> ResourceManager::getConcurrentTxStream_l(
    Stream *rx_str,
    std::shared_ptr<Device> rx_device)
{
    int deviceId = 0;
    int status = 0;
    std::vector<Stream*> tx_stream_list;
    struct pal_stream_attributes tx_attr;
    struct pal_stream_attributes rx_attr;
    std::shared_ptr<Device> tx_device = nullptr;
    std::vector <std::shared_ptr<Device>> tx_device_list;

    // check stream direction
    status = rx_str->getStreamAttributes(&rx_attr);
    if (status) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        goto exit;
    }
    if (!(rx_attr.direction == PAL_AUDIO_OUTPUT ||
          rx_attr.direction == PAL_AUDIO_INPUT_OUTPUT)) {
        PAL_ERR(LOG_TAG, "Invalid stream direction %d", rx_attr.direction);
        status = -EINVAL;
        goto exit;
    }

    for (auto& tx_str: mActiveStreams) {
        tx_device_list.clear();
        tx_str->getStreamAttributes(&tx_attr);
        if (tx_attr.type == PAL_STREAM_PROXY ||
            tx_attr.type == PAL_STREAM_ULTRA_LOW_LATENCY ||
            tx_attr.type == PAL_STREAM_GENERIC)
            continue;
        if (tx_attr.direction == PAL_AUDIO_INPUT) {
            if (!getEcRefStatus(tx_attr.type, rx_attr.type)) {
                PAL_DBG(LOG_TAG, "No need to enable ec ref for rx %d tx %d",
                        rx_attr.type, tx_attr.type);
                continue;
            }
            tx_str->getAssociatedDevices(tx_device_list);
            for (int i = 0; i < tx_device_list.size(); i++) {
                if (!isDeviceActive_l(tx_device_list[i], tx_str))
                    continue;
                deviceId = tx_device_list[i]->getSndDeviceId();
                if (deviceId > PAL_DEVICE_IN_MIN &&
                    deviceId < PAL_DEVICE_IN_MAX)
                    tx_device = tx_device_list[i];
                else
                    tx_device = nullptr;

                if (checkECRef(rx_device, tx_device)) {
                    tx_stream_list.push_back(tx_str);
                    break;
                }
            }
        }
    }
exit:
    return tx_stream_list;
}

std::vector<Stream*> ResourceManager::getConcurrentTxStream(
    Stream *rx_str,
    std::shared_ptr<Device> rx_device)
{
    std::vector<Stream*> tx_stream_list;
    PAL_DBG(LOG_TAG, "Enter.");
    mResourceManagerMutex.lock();
    tx_stream_list = getConcurrentTxStream_l(rx_str, rx_device);
    mResourceManagerMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit.");
    return tx_stream_list;
}

bool ResourceManager::checkECRef(std::shared_ptr<Device> rx_dev,
                                 std::shared_ptr<Device> tx_dev)
{
    bool result = false;
    int rx_dev_id = 0;
    int tx_dev_id = 0;

    if (!rx_dev || !tx_dev)
        return result;

    rx_dev_id = rx_dev->getSndDeviceId();
    tx_dev_id = tx_dev->getSndDeviceId();

    for (int i = 0; i < deviceInfo.size(); i++) {
        if (tx_dev_id == deviceInfo[i].deviceId) {
            for (int j = 0; j < deviceInfo[i].rx_dev_ids.size(); j++) {
                if (rx_dev_id == deviceInfo[i].rx_dev_ids[j]) {
                    result = true;
                    break;
                }
            }
        }
        if (result)
            break;
    }

    PAL_DBG(LOG_TAG, "EC Ref: %d, rx dev: %d, tx dev: %d",
        result, rx_dev_id, tx_dev_id);

    return result;
}

int ResourceManager::updateECDeviceMap_l(std::shared_ptr<Device> rx_dev,
    std::shared_ptr<Device> tx_dev, Stream *tx_str, int count, bool is_txstop)
{
    int status = 0;
    mResourceManagerMutex.lock();
    status = updateECDeviceMap(rx_dev, tx_dev, tx_str, count, is_txstop);
    mResourceManagerMutex.unlock();

    return status;
}


int ResourceManager::updateECDeviceMap(std::shared_ptr<Device> rx_dev,
    std::shared_ptr<Device> tx_dev, Stream *tx_str, int count, bool is_txstop)
{
    int rx_dev_id = 0;
    int tx_dev_id = 0;
    int ec_count = 0;
    int i = 0, j = 0;
    bool tx_stream_found = false;
    std::vector<std::pair<Stream *, int>>::iterator iter;
    std::map<int, std::vector<std::pair<Stream *, int>>>::iterator map_iter;

    if ((!rx_dev && !is_txstop) || !tx_dev || !tx_str) {
        PAL_ERR(LOG_TAG, "Invalid operation");
        return -EINVAL;
    }

    tx_dev_id = tx_dev->getSndDeviceId();
    for (i = 0; i < deviceInfo.size(); i++) {
        if (tx_dev_id == deviceInfo[i].deviceId) {
            break;
        }
    }

    if (i == deviceInfo.size()) {
        PAL_ERR(LOG_TAG, "Tx device %d not found", tx_dev_id);
        return -EINVAL;
    }

    if (is_txstop) {
        for (map_iter = deviceInfo[i].ec_ref_count_map.begin();
            map_iter != deviceInfo[i].ec_ref_count_map.end(); map_iter++) {
            rx_dev_id = (*map_iter).first;
            if (rx_dev && rx_dev->getSndDeviceId() != rx_dev_id)
                continue;
            for (iter = deviceInfo[i].ec_ref_count_map[rx_dev_id].begin();
                iter != deviceInfo[i].ec_ref_count_map[rx_dev_id].end(); iter++) {
                if ((*iter).first == tx_str) {
                    tx_stream_found = true;
                    deviceInfo[i].ec_ref_count_map[rx_dev_id].erase(iter);
                    ec_count = 0;
                    break;
                }
            }
            if (tx_stream_found && rx_dev)
                break;
        }
    } else {
        // rx_dev cannot be null if is_txstop is false
        rx_dev_id = rx_dev->getSndDeviceId();

        for (iter = deviceInfo[i].ec_ref_count_map[rx_dev_id].begin();
            iter != deviceInfo[i].ec_ref_count_map[rx_dev_id].end(); iter++) {
            if ((*iter).first == tx_str) {
                tx_stream_found = true;
                if (count > 0) {
                    (*iter).second += count;
                    ec_count = (*iter).second;
                } else if (count == 0) {
                    if ((*iter).second > 0) {
                        (*iter).second--;
                    }
                    ec_count = (*iter).second;
                    if ((*iter).second == 0) {
                        deviceInfo[i].ec_ref_count_map[rx_dev_id].erase(iter);
                    }
                }
                break;
            }
        }
    }

    if (!tx_stream_found) {
        if (count == 0) {
            PAL_ERR(LOG_TAG, "Cannot reset as ec ref not present");
            return -EINVAL;
        } else if (count > 0) {
            deviceInfo[i].ec_ref_count_map[rx_dev_id].push_back(
                std::make_pair(tx_str, count));
            ec_count = count;
        }
    }

    PAL_DBG(LOG_TAG, "EC ref count for stream device pair (%pK %d, %d) is %d",
        tx_str, tx_dev_id, rx_dev_id, ec_count);
    return ec_count;
}

std::shared_ptr<Device> ResourceManager::clearInternalECRefCounts(Stream *tx_str,
    std::shared_ptr<Device> tx_dev)
{
    int i = 0;
    int rx_dev_id = 0;
    int tx_dev_id = 0;
    struct pal_device palDev;
    std::shared_ptr<Device> rx_dev = nullptr;
    std::vector<std::pair<Stream *, int>>::iterator iter;
    std::map<int, std::vector<std::pair<Stream *, int>>>::iterator map_iter;

    if (!tx_str || !tx_dev) {
        PAL_ERR(LOG_TAG, "Invalid operation");
        goto exit;
    }

    tx_dev_id = tx_dev->getSndDeviceId();
    for (i = 0; i < deviceInfo.size(); i++) {
        if (tx_dev_id == deviceInfo[i].deviceId) {
            break;
        }
    }

    if (i == deviceInfo.size()) {
        PAL_ERR(LOG_TAG, "Tx device %d not found", tx_dev_id);
        goto exit;
    }

    for (map_iter = deviceInfo[i].ec_ref_count_map.begin();
        map_iter != deviceInfo[i].ec_ref_count_map.end(); map_iter++) {
        rx_dev_id = (*map_iter).first;
        if (isExternalECRefEnabled(rx_dev_id))
            continue;
        for (iter = deviceInfo[i].ec_ref_count_map[rx_dev_id].begin();
            iter != deviceInfo[i].ec_ref_count_map[rx_dev_id].end(); iter++) {
            if ((*iter).first == tx_str) {
                if ((*iter).second > 0) {
                    palDev.id = (pal_device_id_t)rx_dev_id;
                    rx_dev = Device::getInstance(&palDev, rm);
                }
                deviceInfo[i].ec_ref_count_map[rx_dev_id].erase(iter);
                break;
            }
        }
    }

exit:
    return rx_dev;
}

//TBD: test this piece later, for concurrency
#if 1
template <class T>
void ResourceManager::getHigherPriorityActiveStreams(const int inComingStreamPriority, std::vector<Stream*> &activestreams,
                      std::vector<T> sourcestreams)
{
    int existingStreamPriority = 0;
    pal_stream_attributes sAttr;


    typename std::vector<T>::iterator iter = sourcestreams.begin();


    for(iter; iter != sourcestreams.end(); iter++) {
        (*iter)->getStreamAttributes(&sAttr);

        existingStreamPriority = getStreamAttrPriority(&sAttr);
        if (existingStreamPriority > inComingStreamPriority)
        {
            activestreams.push_back(*iter);
        }
    }
}
#endif


template <class T>
void getActiveStreams(std::shared_ptr<Device> d, std::vector<Stream*> &activestreams,
                      std::list<T> sourcestreams)
{
    for (typename std::list<T>::iterator iter = sourcestreams.begin();
                 iter != sourcestreams.end(); iter++) {
        std::vector <std::shared_ptr<Device>> devices;

        if (NULL != *iter) {
            (*iter)->getAssociatedDevices(devices);
            if (d == NULL) {
                 if((*iter)->isAlive() && !devices.empty())
                    activestreams.push_back(*iter);
            } else {
                typename std::vector<std::shared_ptr<Device>>::iterator result =
                         std::find(devices.begin(), devices.end(), d);
                if ((result != devices.end()) && (*iter)->isAlive())
                    activestreams.push_back(*iter);
            }
        } else {
            // remove element from the list if it's a NULL pointer
            sourcestreams.erase(iter);
        }
    }
}


int ResourceManager::getActiveStream_l(std::vector<Stream*> &activestreams,
                                       std::shared_ptr<Device> d)
{
    int ret = 0;

    activestreams.clear();

    // merge all types of active streams into activestreams
    getActiveStreams(d, activestreams, active_streams_ll);
    getActiveStreams(d, activestreams, active_streams_ull);
    getActiveStreams(d, activestreams, active_streams_ulla);
    getActiveStreams(d, activestreams, active_streams_db);
    getActiveStreams(d, activestreams, active_streams_sa);
    getActiveStreams(d, activestreams, active_streams_raw);
    getActiveStreams(d, activestreams, active_streams_comp);
    getActiveStreams(d, activestreams, active_streams_st);
    getActiveStreams(d, activestreams, active_streams_acd);
    getActiveStreams(d, activestreams, active_streams_po);
    getActiveStreams(d, activestreams, active_streams_proxy);
    getActiveStreams(d, activestreams, active_streams_incall_record);
    getActiveStreams(d, activestreams, active_streams_non_tunnel);
    getActiveStreams(d, activestreams, active_streams_incall_music);
    getActiveStreams(d, activestreams, active_streams_haptics);
    getActiveStreams(d, activestreams, active_streams_ultrasound);
    getActiveStreams(d, activestreams, active_streams_sensor_pcm_data);
    getActiveStreams(d, activestreams, active_streams_voice_rec);
    getActiveStreams(d, activestreams, active_streams_sensor_renderer);

    if (activestreams.empty()) {
        ret = -ENOENT;
        if (d) {
            PAL_INFO(LOG_TAG, "no active streams found for device %d ret %d", d->getSndDeviceId(), ret);
        } else {
            PAL_INFO(LOG_TAG, "no active streams found ret %d", ret);
        }
    }

    return ret;
}

int ResourceManager::getActiveStream(std::vector<Stream*> &activestreams,
                                     std::shared_ptr<Device> d)
{
    int ret = 0;
    PAL_DBG(LOG_TAG, "Enter.");
    mResourceManagerMutex.lock();
    ret = getActiveStream_l(activestreams, d);
    mResourceManagerMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit. ret %d", ret);
    return ret;
}

template <class T>
void getOrphanStreams(std::vector<Stream*> &orphanstreams,
                      std::vector<Stream*> &retrystreams,
                      std::list<T> sourcestreams)
{
    for (typename std::list<T>::iterator iter = sourcestreams.begin();
                 iter != sourcestreams.end(); iter++) {
        std::vector <std::shared_ptr<Device>> devices;

        if (NULL != *iter) {
            (*iter)->getAssociatedDevices(devices);
            if (devices.empty())
                orphanstreams.push_back(*iter);

            if (((*iter)->suspendedOutDevIds.size() > 0) ||
                    ((*iter)->suspendedInDevIds.size() > 0))
                retrystreams.push_back(*iter);
        } else {
            // remove element from the list if it's a NULL pointer
            sourcestreams.erase(iter);
        }
    }
}

int ResourceManager::getOrphanStream_l(std::vector<Stream*> &orphanstreams,
                                       std::vector<Stream*> &retrystreams)
{
    int ret = 0;

    orphanstreams.clear();
    retrystreams.clear();

    getOrphanStreams(orphanstreams, retrystreams, active_streams_ll);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_ull);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_ulla);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_db);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_sa);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_comp);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_st);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_acd);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_po);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_proxy);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_incall_record);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_non_tunnel);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_incall_music);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_haptics);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_ultrasound);
    getOrphanStreams(orphanstreams, retrystreams, active_streams_sensor_renderer);

    if (orphanstreams.empty() && retrystreams.empty()) {
        ret = -ENOENT;
        PAL_INFO(LOG_TAG, "no orphan streams found");
    }

    return ret;
}

int ResourceManager::getOrphanStream(std::vector<Stream*> &orphanstreams,
                                     std::vector<Stream*> &retrystreams)
{
    int ret = 0;
    PAL_DBG(LOG_TAG, "Enter.");
    mResourceManagerMutex.lock();
    ret = getOrphanStream_l(orphanstreams, retrystreams);
    mResourceManagerMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit. ret %d", ret);
    return ret;
}

/*blsUpdated - to specify if the config is updated by rm*/
int ResourceManager::checkAndGetDeviceConfig(struct pal_device *device, bool* blsUpdated)
{
    int ret = -EINVAL;
    if (!device || !blsUpdated) {
        PAL_ERR(LOG_TAG, "Invalid input parameter ret %d", ret);
        return ret;
    }
    //TODO:check if device config is supported
    bool dev_supported = false;
    *blsUpdated = false;
    uint16_t channels = device->config.ch_info.channels;
    uint32_t samplerate = device->config.sample_rate;
    uint32_t bitwidth = device->config.bit_width;

    PAL_DBG(LOG_TAG, "Enter.");
    //TODO: check and rewrite params if needed
    // only compare with default value for now
    // because no config file parsed in init
    if (channels != DEFAULT_CHANNELS) {
        if (bOverwriteFlag) {
            device->config.ch_info.channels = DEFAULT_CHANNELS;
            *blsUpdated = true;
        }
    } else if (samplerate != DEFAULT_SAMPLE_RATE) {
        if (bOverwriteFlag) {
            device->config.sample_rate = DEFAULT_SAMPLE_RATE;
            *blsUpdated = true;
        }
    } else if (bitwidth != DEFAULT_BIT_WIDTH) {
        if (bOverwriteFlag) {
            device->config.bit_width = DEFAULT_BIT_WIDTH;
            *blsUpdated = true;
        }
    } else {
        ret = 0;
        dev_supported = true;
    }
    PAL_DBG(LOG_TAG, "Exit. ret %d", ret);
    return ret;
}

/* check if headset sample rate needs to be updated for haptics concurrency */
void ResourceManager::checkHapticsConcurrency(struct pal_device *deviceattr,
        const struct pal_stream_attributes *sAttr, std::vector<Stream*> &streamsToSwitch,
        struct pal_device *curDevAttr)
{
    std::vector <std::tuple<Stream *, uint32_t>> sharedBEStreamDev;
    std::vector <Stream *> activeHapticsStreams;

    if (!deviceattr) {
        PAL_ERR(LOG_TAG, "Invalid device attribute");
        return;
    }

    // if headset is coming, check if haptics is already active
    // and then update same sample rate for headset device
    if (deviceattr->id == PAL_DEVICE_OUT_WIRED_HEADSET ||
        deviceattr->id == PAL_DEVICE_OUT_WIRED_HEADPHONE) {
        struct pal_device hapticsDattr;
        std::shared_ptr<Device> hapticsDev = nullptr;
        std::shared_ptr<Device>  hsDev = nullptr;

        hapticsDattr.id = PAL_DEVICE_OUT_HAPTICS_DEVICE;
        hapticsDev = Device::getInstance(&hapticsDattr, rm);
        hsDev = Device::getInstance(deviceattr, rm);
        if (!hapticsDev || !hsDev) {
            PAL_ERR(LOG_TAG, "Getting Device instance failed");
            return;
        }
        getActiveStream_l(activeHapticsStreams, hapticsDev);
        if (activeHapticsStreams.size() != 0) {
            hapticsDev->getDeviceAttributes(&hapticsDattr);
            if ((deviceattr->config.sample_rate % SAMPLINGRATE_44K == 0) &&
                (hapticsDattr.config.sample_rate % SAMPLINGRATE_44K != 0)) {
                deviceattr->config.sample_rate = hapticsDattr.config.sample_rate;
                hsDev->setSampleRate(hapticsDattr.config.sample_rate);
                deviceattr->config.bit_width = hapticsDattr.config.bit_width;
                deviceattr->config.aud_fmt_id =  bitWidthToFormat.at(deviceattr->config.bit_width);
                PAL_DBG(LOG_TAG, "headset is coming, update headset to sr: %d bw: %d ",
                    deviceattr->config.sample_rate, deviceattr->config.bit_width);
            }
        } else {
               hsDev->setSampleRate(0);
        }
    } else if (deviceattr->id == PAL_DEVICE_OUT_HAPTICS_DEVICE) {
        // if haptics is coming, update headset sample rate if needed
        getSharedBEActiveStreamDevs(sharedBEStreamDev, PAL_DEVICE_OUT_WIRED_HEADSET);
        if (sharedBEStreamDev.size() > 0) {
            for (const auto &elem : sharedBEStreamDev) {
                bool switchNeeded = false;
                Stream *sharedStream = std::get<0>(elem);
                std::shared_ptr<Device> curDev = nullptr;

                if (switchNeeded)
                    streamsToSwitch.push_back(sharedStream);

                curDevAttr->id = (pal_device_id_t)std::get<1>(elem);
                curDev = Device::getInstance(curDevAttr, rm);
                if (!curDev) {
                    PAL_ERR(LOG_TAG, "Getting Device instance failed");
                    continue;
                }
                curDev->getDeviceAttributes(curDevAttr);
                if ((curDevAttr->config.sample_rate % SAMPLINGRATE_44K == 0) &&
                    (sAttr->out_media_config.sample_rate % SAMPLINGRATE_44K != 0)) {
                    curDevAttr->config.sample_rate = sAttr->out_media_config.sample_rate;
                    curDevAttr->config.bit_width = sAttr->out_media_config.bit_width;
                    curDevAttr->config.aud_fmt_id = bitWidthToFormat.at(deviceattr->config.bit_width);
                    switchNeeded = true;
                    streamsToSwitch.push_back(sharedStream);
                    PAL_DBG(LOG_TAG, "haptics is coming, update headset to sr: %d bw: %d ",
                        curDevAttr->config.sample_rate, curDevAttr->config.bit_width);
                }
            }
        }
    }
}

/* check if group dev configuration exists for a given group device */
bool ResourceManager::isGroupConfigAvailable(group_dev_config_idx_t idx)
{
    std::map<group_dev_config_idx_t, std::shared_ptr<group_dev_config_t>>::iterator it;

    it = groupDevConfigMap.find(idx);
    if (it != groupDevConfigMap.end())
        return true;

    return false;
}

/* this if for setting group device config for device with virtual port */
int ResourceManager::checkAndUpdateGroupDevConfig(struct pal_device *deviceattr, const struct pal_stream_attributes *sAttr,
        std::vector<Stream*> &streamsToSwitch, struct pal_device *streamDevAttr, bool streamEnable)
{
    struct pal_device activeDevattr;
    std::shared_ptr<Device> dev = nullptr;
    std::string backEndName;
    std::vector<Stream*> activeStream;
    std::map<group_dev_config_idx_t, std::shared_ptr<group_dev_config_t>>::iterator it;
    std::vector<Stream*>::iterator sIter;
    group_dev_config_idx_t group_cfg_idx = GRP_DEV_CONFIG_INVALID;

    if (!deviceattr) {
        PAL_ERR(LOG_TAG, "Invalid deviceattr");
        return -EINVAL;
    }

    if (!sAttr) {
        PAL_ERR(LOG_TAG, "Invalid stream attr");
        return -EINVAL;
    }

    /* handle special case for UPD device with virtual port:
     * 1. Enable
     *   1) if upd is coming and there's any active stream on speaker or handset,
     *      upadate group device config and disconnect and connect current stream;
     *   2) if stream on speaker or handset is coming and upd is already active,
     *      update group config disconnect and connect upd stream;
     * 2. Disable (restore device config)
     *   1) if upd goes away, and stream on speaker or handset is active, need to
     *      restore group config to speaker or handset standalone;
     *   2) if stream on speaker/handset goes away, and upd is still active, need to restore
     *      restore group config to upd standalone
     */
    if (getBackendName(deviceattr->id, backEndName) == 0 &&
            strstr(backEndName.c_str(), "-VIRT-")) {
        PAL_DBG(LOG_TAG, "virtual port enabled for device %d", deviceattr->id);

        /* check for UPD comming or goes away */
        if (deviceattr->id == PAL_DEVICE_OUT_ULTRASOUND) {
            group_cfg_idx = GRP_UPD_RX;
            // check if stream active on speaker or handset exists
            // update group config and stream to streamsToSwitch to switch device for current stream if needed
            pal_device_id_t conc_dev[] = {PAL_DEVICE_OUT_SPEAKER, PAL_DEVICE_OUT_HANDSET};
            for (int i = 0; i < sizeof(conc_dev)/sizeof(conc_dev[0]); i++) {
                activeDevattr.id = conc_dev[i];
                dev = Device::getInstance(&activeDevattr, rm);
                if (!dev)
                    continue;
                getActiveStream_l(activeStream, dev);
                if (activeStream.empty())
                    continue;
                for (sIter = activeStream.begin(); sIter != activeStream.end(); sIter++) {
                    pal_stream_type_t type;
                    (*sIter)->getStreamType(&type);
                    switch (conc_dev[i]) {
                        case PAL_DEVICE_OUT_SPEAKER:
                            if (streamEnable) {
                                PAL_DBG(LOG_TAG, "upd is coming, found stream %d active on speaker", type);
                                if (isGroupConfigAvailable(GRP_UPD_RX_SPEAKER)) {
                                    PAL_DBG(LOG_TAG, "concurrency config exists, update active group config to upd_speaker");
                                    group_cfg_idx = GRP_UPD_RX_SPEAKER;
                                    streamsToSwitch.push_back(*sIter);
                                } else {
                                    PAL_DBG(LOG_TAG, "concurrency config doesn't exist, update active group config to upd");
                                    group_cfg_idx = GRP_UPD_RX;
                                }
                            } else {
                                PAL_DBG(LOG_TAG, "upd goes away, stream %d active on speaker", type);
                                if (isGroupConfigAvailable(GRP_UPD_RX_SPEAKER)) {
                                    PAL_DBG(LOG_TAG, "concurrency config exists, update active group config to speaker");
                                    streamsToSwitch.push_back(*sIter);
                                    if (type == PAL_STREAM_VOICE_CALL &&
                                        isGroupConfigAvailable(GRP_SPEAKER_VOICE)) {
                                        PAL_DBG(LOG_TAG, "voice stream active, set to speaker voice cfg");
                                        group_cfg_idx = GRP_SPEAKER_VOICE;
                                    } else {
                                        // if voice usecase is active, always use voice config
                                        if (group_cfg_idx != GRP_SPEAKER_VOICE)
                                            group_cfg_idx = GRP_SPEAKER;
                                    }
                                }
                            }
                        break;
                        case PAL_DEVICE_OUT_HANDSET:
                            if (streamEnable) {
                                PAL_DBG(LOG_TAG, "upd is coming, stream %d active on handset", type);
                                if (isGroupConfigAvailable(GRP_UPD_RX_HANDSET)) {
                                    PAL_DBG(LOG_TAG, "concurrency config exists, update active group config to upd_handset");
                                    group_cfg_idx = GRP_UPD_RX_HANDSET;
                                    streamsToSwitch.push_back(*sIter);
                                } else {
                                    PAL_DBG(LOG_TAG, "concurrency config doesn't exist, update active group config to upd");
                                    group_cfg_idx = GRP_UPD_RX;
                                }
                            } else {
                                PAL_DBG(LOG_TAG, "upd goes away, stream %d active on handset", type);
                                if (isGroupConfigAvailable(GRP_UPD_RX_HANDSET)) {
                                    PAL_DBG(LOG_TAG, "concurrency config exists, update active group config to handset");
                                    streamsToSwitch.push_back(*sIter);
                                    group_cfg_idx = GRP_HANDSET;
                                }
                            }
                            break;
                        default:
                            break;
                    }
                }
                dev->getDeviceAttributes(streamDevAttr);
                activeStream.clear();
            }
            it = groupDevConfigMap.find(group_cfg_idx);
            if (it != groupDevConfigMap.end()) {
                ResourceManager::activeGroupDevConfig = it->second;
            } else {
                PAL_ERR(LOG_TAG, "group config for %d is missing", group_cfg_idx);
                return -EINVAL;
            }
        /* check for streams on speaker or handset comming/goes away */
        } else if (deviceattr->id == PAL_DEVICE_OUT_SPEAKER ||
                    deviceattr->id == PAL_DEVICE_OUT_HANDSET) {
            if (streamEnable) {
                PAL_DBG(LOG_TAG, "stream on device:%d is coming, update group config", deviceattr->id);
                if (deviceattr->id == PAL_DEVICE_OUT_SPEAKER)
                    group_cfg_idx = GRP_SPEAKER;
                else
                    group_cfg_idx = GRP_HANDSET;
            } // else {} do nothing if stream on handset or speaker goes away without active upd
            activeDevattr.id = PAL_DEVICE_OUT_ULTRASOUND;
            dev = Device::getInstance(&activeDevattr, rm);
            if (dev) {
                getActiveStream_l(activeStream, dev);
                if (!activeStream.empty()) {
                    sIter = activeStream.begin();
                    dev->getDeviceAttributes(streamDevAttr);
                    if (streamEnable) {
                        PAL_DBG(LOG_TAG, "upd is already active, stream on device:%d is coming", deviceattr->id);
                        if (deviceattr->id == PAL_DEVICE_OUT_SPEAKER) {
                            if (isGroupConfigAvailable(GRP_UPD_RX_SPEAKER)) {
                                PAL_DBG(LOG_TAG, "concurrency config exists, update active group config to upd_speaker");
                                group_cfg_idx = GRP_UPD_RX_SPEAKER;
                                streamsToSwitch.push_back(*sIter);
                            } else {
                                PAL_DBG(LOG_TAG, "concurrency config doesn't exist, update active group config to speaker");
                                group_cfg_idx = GRP_SPEAKER;
                            }
                        } else if (deviceattr->id == PAL_DEVICE_OUT_HANDSET){
                            if (isGroupConfigAvailable(GRP_UPD_RX_HANDSET)) {
                                PAL_DBG(LOG_TAG, "concurrency config exists, update active group config to upd_handset");
                                group_cfg_idx = GRP_UPD_RX_HANDSET;
                                streamsToSwitch.push_back(*sIter);
                            } else {
                                PAL_DBG(LOG_TAG, "concurrency config doesn't exist, update active group config to handset");
                                group_cfg_idx = GRP_HANDSET;
                            }
                        }
                    } else {
                        PAL_DBG(LOG_TAG, "upd is still active, stream on device:%d goes away", deviceattr->id);
                        if (deviceattr->id == PAL_DEVICE_OUT_SPEAKER) {
                            if (isGroupConfigAvailable(GRP_UPD_RX_SPEAKER)) {
                                PAL_DBG(LOG_TAG, "concurrency config exists, update active group config to upd");
                                streamsToSwitch.push_back(*sIter);
                            }
                        } else {
                            if (isGroupConfigAvailable(GRP_UPD_RX_HANDSET)) {
                                PAL_DBG(LOG_TAG, "concurrency config exists, update active group config to upd");
                                streamsToSwitch.push_back(*sIter);
                            }
                        }
                        group_cfg_idx = GRP_UPD_RX;
                    }
                } else {
                    // it could be mono speaker when voice call is coming without UPD
                    if (streamEnable) {
                        PAL_DBG(LOG_TAG, "upd is not active, stream type %d on device:%d is coming",
                                    sAttr->type, deviceattr->id);
                        if (deviceattr->id == PAL_DEVICE_OUT_SPEAKER &&
                            sAttr->type == PAL_STREAM_VOICE_CALL) {
                            if (isGroupConfigAvailable(GRP_SPEAKER_VOICE)) {
                                PAL_DBG(LOG_TAG, "set to speaker voice cfg");
                                group_cfg_idx = GRP_SPEAKER_VOICE;
                            }
                        // if coming usecase is not voice call but voice call already active
                        // still set group config for speaker as voice speaker
                        } else {
                            pal_stream_type_t type;
                            for (auto& str: mActiveStreams) {
                                str->getStreamType(&type);
                                if (type == PAL_STREAM_VOICE_CALL) {
                                    group_cfg_idx = GRP_SPEAKER_VOICE;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            it = groupDevConfigMap.find(group_cfg_idx);
            if (it != groupDevConfigMap.end()) {
                ResourceManager::activeGroupDevConfig = it->second;
            } else {
                PAL_ERR(LOG_TAG, "group config for %d is missing", group_cfg_idx);
                return -EINVAL;
            }
        }

        // update snd device name so all concurrent stream can apply the same mixer path
        if (ResourceManager::activeGroupDevConfig) {
            // first update incoming device name
            dev = Device::getInstance(deviceattr, rm);
            if (dev) {
                if (!ResourceManager::activeGroupDevConfig->snd_dev_name.empty()) {
                    dev->setSndName(ResourceManager::activeGroupDevConfig->snd_dev_name);
                } else {
                    dev->clearSndName();
                }
            }
            // then update current active device name
            dev = Device::getInstance(streamDevAttr, rm);
            if (dev) {
                if (!ResourceManager::activeGroupDevConfig->snd_dev_name.empty()) {
                    dev->setSndName(ResourceManager::activeGroupDevConfig->snd_dev_name);
                } else {
                    dev->clearSndName();
                }
            }
        }
    }

    return 0;
}

void ResourceManager::checkAndSetDutyCycleParam()
{
    pal_stream_attributes StrAttr;
    std::shared_ptr<Device> dev = nullptr;
    struct pal_device DevAttr;
    std::string backEndName;
    Stream *UPDStream = nullptr;
    bool is_upd_active = false;
    bool is_wsa_upd = false;
    bool enable_duty = false;
    std::vector<Stream*> activeStream;
    std::vector <std::shared_ptr<Device>> aDevices;
    pal_device_id_t conc_dev[] = {PAL_DEVICE_OUT_SPEAKER, PAL_DEVICE_OUT_HANDSET};

    if (!IsDutyCycleForUPDEnabled()) {
        PAL_DBG(LOG_TAG, "duty cycle for UPD not enabled");
        return;
    }

    // check if UPD is already active
    for (auto& str: mActiveStreams) {
        str->getStreamAttributes(&StrAttr);
        if ((StrAttr.type == PAL_STREAM_ULTRASOUND ||
             StrAttr.type == PAL_STREAM_SENSOR_PCM_RENDERER) &&
            str->isActive()) {
            is_upd_active = true;
            // enable duty by default, this may change based on concurrency.
            // during device switch, upd stream can active, but RX device is
            // disabled, so do not enable duty
            str->getAssociatedDevices(aDevices);
            if (aDevices.size() == 2)
                enable_duty = true;
            UPDStream = str;
            break;
        } else {
            continue;
        }
    }

    if (!is_upd_active) {
        PAL_DBG(LOG_TAG, "no active UPD stream found");
        return;
    }

    for (int i = 0; i < sizeof(conc_dev)/sizeof(conc_dev[0]); i++) {
        activeStream.clear();
        DevAttr.id = conc_dev[i];
        dev = Device::getInstance(&DevAttr, rm);
        if (!dev)
            continue;
        getActiveStream_l(activeStream, dev);
        if (activeStream.empty())
            continue;
        // if virtual port is enabled, if handset or speaker is active, disable duty cycle
        if (rm->activeGroupDevConfig) {
            enable_duty = false;
            PAL_DBG(LOG_TAG, "upd on virtual port, dev %d active", dev->getSndDeviceId());
        } else if (IsDedicatedBEForUPDEnabled()) {
            // for dedicated upd backend, check if upd on wsa or wcd
            if (getBackendName(PAL_DEVICE_OUT_ULTRASOUND_DEDICATED, backEndName) == 0){
                bool is_same_codec = false;
                if (strstr(backEndName.c_str(), "CODEC_DMA-LPAIF_WSA-RX"))
                    is_wsa_upd = true;
                if (getBackendName(dev->getSndDeviceId(), backEndName) == 0) {
                    if (strstr(backEndName.c_str(), "CODEC_DMA-LPAIF_WSA-RX")) {
                        if (is_wsa_upd) {
                            PAL_DBG(LOG_TAG, "upd and audio device %d on wsa", dev->getSndDeviceId());
                            is_same_codec = true;
                        }
                    } else {
                        if (!is_wsa_upd) {
                            PAL_DBG(LOG_TAG, "upd and audio device %d on wcd", dev->getSndDeviceId());
                            is_same_codec = true;
                        }
                    }
                    if (is_same_codec) {
                        if (dev->getDeviceCount() > 0)
                            enable_duty = false;
                        else
                            enable_duty = true;
                    }
                }
            }
        } else {
            // upd shares backend with handset, check if device count > 1
            aDevices.clear();
            UPDStream->getAssociatedDevices(aDevices);
            for (auto &upd_dev : aDevices) {
                if (ResourceManager::isOutputDevId(upd_dev->getSndDeviceId())) {
                    if (upd_dev->getDeviceCount() > 1) {
                        enable_duty = false;
                        PAL_DBG(LOG_TAG, "upd and audio stream active on %d", dev->getSndDeviceId());
                    } else {
                        enable_duty = true;
                        PAL_DBG(LOG_TAG, "only upd is active on %d, enable duty", dev->getSndDeviceId());
                    }
                }
            }
        }
    }

    if (UPDStream->getDutyCycleEnable() != enable_duty) {
        Session *session = NULL;
        UPDStream->getAssociatedSession(&session);
        if (session != NULL) {
            UPDStream->setDutyCycleEnable(enable_duty);
            session->setParameters(UPDStream, 0, PAL_PARAM_ID_SET_UPD_DUTY_CYCLE, &enable_duty);
            PAL_DBG(LOG_TAG, "set duty cycling: %d", enable_duty);
        }
    }
}

std::shared_ptr<ResourceManager> ResourceManager::getInstance()
{
    if(!rm) {
        std::lock_guard<std::mutex> lock(ResourceManager::mResourceManagerMutex);
        if (!rm) {
            std::shared_ptr<ResourceManager> sp(new ResourceManager());
            rm = sp;
        }
    }
    return rm;
}

int ResourceManager::getVirtualSndCard()
{
    return snd_virt_card;
}

int ResourceManager::getHwSndCard()
{
    return snd_hw_card;
}

int ResourceManager::getSndDeviceName(int deviceId, char *device_name)
{
    std::string backEndName;
    if (isValidDevId(deviceId)) {
        strlcpy(device_name, sndDeviceNameLUT[deviceId].second.c_str(), DEVICE_NAME_MAX_SIZE);
        if (isVbatEnabled && (deviceId == PAL_DEVICE_OUT_SPEAKER ||
                              deviceId == PAL_DEVICE_OUT_ULTRASOUND) &&
                                !strstr(device_name, VBAT_BCL_SUFFIX)) {
            if (deviceId == PAL_DEVICE_OUT_ULTRASOUND) {
                getBackendName(deviceId, backEndName);
                if (!(strstr(backEndName.c_str(), "CODEC_DMA-LPAIF_WSA-RX")))
                    return 0;
            }
            strlcat(device_name, VBAT_BCL_SUFFIX, DEVICE_NAME_MAX_SIZE);
        }
        if (isSpeakerProtectionEnabled && deviceId == PAL_DEVICE_OUT_SPEAKER)
            strlcat(device_name, SPKR_PROT_SUFFIX, DEVICE_NAME_MAX_SIZE);
        if (deviceId == PAL_DEVICE_OUT_ULTRASOUND_DEDICATED &&
             rm->IsDedicatedBEForUPDEnabled())
            strlcat(device_name, "-ultrasound", DEVICE_NAME_MAX_SIZE);
    } else {
        strlcpy(device_name, "", DEVICE_NAME_MAX_SIZE);
        PAL_ERR(LOG_TAG, "Invalid device id %d", deviceId);
        return -EINVAL;
    }
    return 0;
}

int ResourceManager::getDeviceEpName(int deviceId, std::string &epName)
{
    if (isValidDevId(deviceId)) {
        epName.assign(deviceLinkName[deviceId].second);
    } else {
        PAL_ERR(LOG_TAG, "Invalid device id %d", deviceId);
        return -EINVAL;
    }
    return 0;
}

// TODO: Should pcm device be related to usecases used(ll/db/comp/ulla)?
// Use Low Latency as default by now
int ResourceManager::getPcmDeviceId(int deviceId)
{
    int pcm_device_id = -1;
    if (!isValidDevId(deviceId)) {
        PAL_ERR(LOG_TAG, "Invalid device id %d", deviceId);
        return -EINVAL;
    }

    pcm_device_id = devicePcmId[deviceId].second;
    return pcm_device_id;
}

void ResourceManager::deinit()
{
    card_status_t state = CARD_STATUS_NONE;

    mixerClosed = true;
    mixer_close(audio_virt_mixer);
    mixer_close(audio_hw_mixer);
    if (audio_route) {
       audio_route_free(audio_route);
    }
    if (mixerEventTread.joinable()) {
        mixerEventTread.join();
    }
    PAL_DBG(LOG_TAG, "Mixer event thread joined");
    if (rm && rm->IsLowLatencyBargeinSupported()) {
        vui_switch_mutex_.lock();
        vui_switch_thread_exit_ = true;
        vui_switch_cv_.notify_all();
        vui_switch_mutex_.unlock();
        if (vui_deferred_switch_thread_.joinable())
            vui_deferred_switch_thread_.join();
        PAL_DBG(LOG_TAG, "VoiceUI deferred switch thread joined");
    }
    if (sndmon)
        delete sndmon;

   if (isChargeConcurrencyEnabled)
       chargerListenerDeinit();

    voiceuiDmgrManagerDeInit();
    AudioFeatureStatsDeInit();

    cvMutex.lock();
    msgQ.push(state);
    cvMutex.unlock();
    cv.notify_all();

    workerThread.join();
    while (!msgQ.empty())
        msgQ.pop();

#ifdef SOC_PERIPHERAL_PROT
    if (socPerithread.joinable()) {
        socPerithread.join();
    }
#endif
    deviceInfo.clear();
    listAllBackEndIds.clear();
    sndDeviceNameLUT.clear();
    deviceLinkName.clear();
    rm = nullptr;
}

int ResourceManager::getStreamTag(std::vector <int> &tag)
{
    int status = 0;
    for (int i=0; i < streamTag.size(); i++) {
        tag.push_back(streamTag[i]);
    }
    return status;
}

int ResourceManager::getStreamPpTag(std::vector <int> &tag)
{
    int status = 0;
    for (int i=0; i < streamPpTag.size(); i++) {
        tag.push_back(streamPpTag[i]);
    }
    return status;
}

int ResourceManager::getMixerTag(std::vector <int> &tag)
{
    int status = 0;
    for (int i=0; i < mixerTag.size(); i++) {
        tag.push_back(mixerTag[i]);
    }
    return status;
}

int ResourceManager::getDeviceTag(std::vector <int> &tag)
{
    int status = 0;
    for (int i=0; i < deviceTag.size(); i++) {
        tag.push_back(deviceTag[i]);
    }
    return status;
}

int ResourceManager::getDevicePpTag(std::vector <int> &tag)
{
    int status = 0;
    for (int i=0; i < devicePpTag.size(); i++) {
        tag.push_back(devicePpTag[i]);
    }
    return status;
}

int ResourceManager::getDeviceDirection(uint32_t beDevId)
{
    int dir = -EINVAL;

    if (beDevId < PAL_DEVICE_OUT_MAX)
        dir = PAL_AUDIO_OUTPUT;
    else if (beDevId < PAL_DEVICE_IN_MAX)
        dir = PAL_AUDIO_INPUT;

    return dir;
}

void ResourceManager::getSpViChannelMapCfg(int32_t *channelMap, uint32_t numOfChannels)
{
    int i = 0;

    if (!channelMap)
        return;

    if (!spViChannelMapCfg.empty() && (spViChannelMapCfg.size() == numOfChannels)) {
        for (i = 0; i < spViChannelMapCfg.size(); i++) {
            channelMap[i] = spViChannelMapCfg[i];
        }
        return;
    }

    PAL_DBG(LOG_TAG, "sp_vi_ch_map info is not updated from Rm.xml");
    for (i = 0; i < numOfChannels; i++) {
        channelMap[i] = i+1;
    }
}

int ResourceManager::getNumFEs(const pal_stream_type_t sType) const
{
    int n = 1;

    switch (sType) {
        case PAL_STREAM_LOOPBACK:
        case PAL_STREAM_TRANSCODE:
            n = 1;
            break;
        default:
            n = 1;
            break;
    }

    return n;
}

const std::vector<int> ResourceManager::allocateFrontEndExtEcIds()
{
    std::vector<int> f;
    f.clear();
    const int howMany = 1;
    int id = 0;
    std::vector<int>::iterator it;
    if (howMany > listAllPcmExtEcTxFrontEnds.size()) {
        PAL_ERR(LOG_TAG, "allocateFrontEndExtEcIds: requested for %d external ec front ends, have only %zu error",
                        howMany, listAllPcmExtEcTxFrontEnds.size());
        return f;
    }
    id = (listAllPcmExtEcTxFrontEnds.size() - 1);
    it =  (listAllPcmExtEcTxFrontEnds.begin() + id);
    for (int i = 0; i < howMany; i++) {
        f.push_back(listAllPcmExtEcTxFrontEnds.at(id));
        listAllPcmExtEcTxFrontEnds.erase(it);
        PAL_INFO(LOG_TAG, "allocateFrontEndExtEcIds: front end %d", f[i]);
        it -= 1;
        id -= 1;
    }
    return f;
}

void ResourceManager::freeFrontEndEcTxIds(const std::vector<int> frontend)
{
    for (int i = 0; i < frontend.size(); i++) {
        PAL_INFO(LOG_TAG, "freeing ext ec dev %d\n", frontend.at(i));
        listAllPcmExtEcTxFrontEnds.push_back(frontend.at(i));
    }
    return;
}

template <typename T>
void removeDuplicates(std::vector<T> &vec)
{
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
    return;
}

const std::vector<int> ResourceManager::allocateFrontEndIds(const struct pal_stream_attributes &sAttr, int lDirection)
{
    //TODO: lock resource manager
    std::vector<int> f;
    f.clear();
    const int howMany = getNumFEs(sAttr.type);
    int id = 0;
    std::vector<int>::iterator it;

    mListFrontEndsMutex.lock();
    switch(sAttr.type) {
        case PAL_STREAM_NON_TUNNEL:
            if (howMany > listAllNonTunnelSessionIds.size()) {
                PAL_ERR(LOG_TAG, "allocateFrontEndIds: requested for %d front ends, have only %zu error",
                                howMany, listAllPcmRecordFrontEnds.size());
                 goto error;
            }
            id = (listAllNonTunnelSessionIds.size() - 1);
            it =  (listAllNonTunnelSessionIds.begin() + id);
            for (int i = 0; i < howMany; i++) {
                f.push_back(listAllNonTunnelSessionIds.at(id));
                listAllNonTunnelSessionIds.erase(it);
                PAL_INFO(LOG_TAG, "allocateFrontEndIds: front end %d", f[i]);
                it -= 1;
                id -= 1;
            }
            break;
        case PAL_STREAM_LOW_LATENCY:
        case PAL_STREAM_ULTRA_LOW_LATENCY:
        case PAL_STREAM_GENERIC:
        case PAL_STREAM_DEEP_BUFFER:
        case PAL_STREAM_SPATIAL_AUDIO:
        case PAL_STREAM_VOIP:
        case PAL_STREAM_VOIP_RX:
        case PAL_STREAM_VOIP_TX:
        case PAL_STREAM_VOICE_UI:
        case PAL_STREAM_ACD:
        case PAL_STREAM_ASR:
        case PAL_STREAM_PCM_OFFLOAD:
        case PAL_STREAM_LOOPBACK:
        case PAL_STREAM_PROXY:
        case PAL_STREAM_HAPTICS:
        case PAL_STREAM_ULTRASOUND:
        case PAL_STREAM_RAW:
        case PAL_STREAM_SENSOR_PCM_DATA:
        case PAL_STREAM_VOICE_RECOGNITION:
        case PAL_STREAM_SENSOR_PCM_RENDERER:
            switch (sAttr.direction) {
                case PAL_AUDIO_INPUT:
                    if (lDirection == TX_HOSTLESS) {
                        if (howMany > listAllPcmHostlessTxFrontEnds.size()) {
                            PAL_ERR(LOG_TAG, "allocateFrontEndIds: requested for %d front ends, have only %zu error",
                                              howMany, listAllPcmHostlessTxFrontEnds.size());
                            goto error;
                        }
                        id = (listAllPcmHostlessTxFrontEnds.size() - 1);
                        it = (listAllPcmHostlessTxFrontEnds.begin() + id);
                        for (int i = 0; i < howMany; i++) {
                           f.push_back(listAllPcmHostlessTxFrontEnds.at(id));
                           listAllPcmHostlessTxFrontEnds.erase(it);
                           PAL_INFO(LOG_TAG, "allocateFrontEndIds: front end %d", f[i]);
                           it -= 1;
                           id -= 1;
                        }
                    } else {
                        if (howMany > listAllPcmRecordFrontEnds.size()) {
                            PAL_ERR(LOG_TAG, "allocateFrontEndIds: requested for %d front ends, have only %zu error",
                                              howMany, listAllPcmRecordFrontEnds.size());
                            goto error;
                        }
                        id = (listAllPcmRecordFrontEnds.size() - 1);
                        it = (listAllPcmRecordFrontEnds.begin() + id);
                        for (int i = 0; i < howMany; i++) {
                            f.push_back(listAllPcmRecordFrontEnds.at(id));
                            listAllPcmRecordFrontEnds.erase(it);
                            PAL_INFO(LOG_TAG, "allocateFrontEndIds: front end %d", f[i]);
                            it -= 1;
                            id -= 1;
                        }
                    }
                    break;
                case PAL_AUDIO_OUTPUT:
                    if (lDirection == RX_HOSTLESS) {
                        if (howMany > listAllPcmHostlessRxFrontEnds.size()) {
                            PAL_ERR(LOG_TAG, "allocateFrontEndIds: requested for %d front ends, have only %zu error",
                            howMany, listAllPcmHostlessRxFrontEnds.size());
                            goto error;
                        }
                        id = (listAllPcmHostlessRxFrontEnds.size() - 1);
                        it = (listAllPcmHostlessRxFrontEnds.begin() + id);
                        for (int i = 0; i < howMany; i++) {
                            f.push_back(listAllPcmHostlessRxFrontEnds.at(id));
                            listAllPcmHostlessRxFrontEnds.erase(it);
                            PAL_INFO(LOG_TAG, "allocateFrontEndIds: front end %d", f[i]);
                            it -= 1;
                            id -= 1;
                        }
                    } else {
                        if (howMany > listAllPcmPlaybackFrontEnds.size()) {
                            PAL_ERR(LOG_TAG, "allocateFrontEndIds: requested for %d front ends, have only %zu error",
                            howMany, listAllPcmPlaybackFrontEnds.size());
                            goto error;
                        }
                        if (!listAllPcmPlaybackFrontEnds.size()) {
                            PAL_ERR(LOG_TAG, "allocateFrontEndIds: requested for %d front ends, but we dont have any (%zu) !!!!!!! ",
                                    howMany, listAllPcmPlaybackFrontEnds.size());
                            goto error;
                        }
                        id = (int)(((int)listAllPcmPlaybackFrontEnds.size()) - 1);
                        if (id < 0) {
                            PAL_ERR(LOG_TAG, "allocateFrontEndIds: negative iterator id %d !!!!! ", id);
                            goto error;
                        }
                        it =  (listAllPcmPlaybackFrontEnds.begin() + id);
                        for (int i = 0; i < howMany; i++) {
                            f.push_back(listAllPcmPlaybackFrontEnds.at(id));
                            listAllPcmPlaybackFrontEnds.erase(it);
                            PAL_INFO(LOG_TAG, "allocateFrontEndIds: front end %d", f[i]);
                            it -= 1;
                            id -= 1;
                        }
                    }
                    break;
                case PAL_AUDIO_INPUT | PAL_AUDIO_OUTPUT:
                    if (lDirection == RX_HOSTLESS) {
                        if (howMany > listAllPcmHostlessRxFrontEnds.size()) {
                            PAL_ERR(LOG_TAG, "allocateFrontEndIds: requested for %d front ends, have only %zu error",
                                              howMany, listAllPcmHostlessRxFrontEnds.size());
                            goto error;
                        }
                        id = (listAllPcmHostlessRxFrontEnds.size() - 1);
                        it = (listAllPcmHostlessRxFrontEnds.begin() + id);
                        for (int i = 0; i < howMany; i++) {
                           f.push_back(listAllPcmHostlessRxFrontEnds.at(id));
                           listAllPcmHostlessRxFrontEnds.erase(it);
                           PAL_INFO(LOG_TAG, "allocateFrontEndIds: front end %d", f[i]);
                           it -= 1;
                           id -= 1;
                        }
                    } else {
                        if (howMany > listAllPcmHostlessTxFrontEnds.size()) {
                            PAL_ERR(LOG_TAG, "allocateFrontEndIds: requested for %d front ends, have only %zu error",
                                              howMany, listAllPcmHostlessTxFrontEnds.size());
                            goto error;
                        }
                        id = (listAllPcmHostlessTxFrontEnds.size() - 1);
                        it = (listAllPcmHostlessTxFrontEnds.begin() + id);
                        for (int i = 0; i < howMany; i++) {
                           f.push_back(listAllPcmHostlessTxFrontEnds.at(id));
                           listAllPcmHostlessTxFrontEnds.erase(it);
                           PAL_INFO(LOG_TAG, "allocateFrontEndIds: front end %d", f[i]);
                           it -= 1;
                           id -= 1;
                        }
                    }
                    break;
                default:
                    PAL_ERR(LOG_TAG,"direction unsupported");
                    break;
            }
            break;
        case PAL_STREAM_COMPRESSED:
            switch (sAttr.direction) {
                case PAL_AUDIO_INPUT:
                    if (howMany > listAllCompressRecordFrontEnds.size()) {
                        PAL_ERR(LOG_TAG, "allocateFrontEndIds: requested for %d front ends, have only %zu error",
                                          howMany, listAllCompressRecordFrontEnds.size());
                        goto error;
                    }
                    id = (listAllCompressRecordFrontEnds.size() - 1);
                    it = (listAllCompressRecordFrontEnds.begin() + id);
                    for (int i = 0; i < howMany; i++) {
                        f.push_back(listAllCompressRecordFrontEnds.at(id));
                        listAllCompressRecordFrontEnds.erase(it);
                        PAL_INFO(LOG_TAG, "allocateFrontEndIds: front end %d", f[i]);
                        it -= 1;
                        id -= 1;
                    }
                    break;
                case PAL_AUDIO_OUTPUT:
                    if (howMany > listAllCompressPlaybackFrontEnds.size()) {
                        PAL_ERR(LOG_TAG, "allocateFrontEndIds: requested for %d front ends, have only %zu error",
                                          howMany, listAllCompressPlaybackFrontEnds.size());
                        goto error;
                    }
                    id = (listAllCompressPlaybackFrontEnds.size() - 1);
                    it = (listAllCompressPlaybackFrontEnds.begin() + id);
                    for (int i = 0; i < howMany; i++) {
                        f.push_back(listAllCompressPlaybackFrontEnds.at(id));
                        listAllCompressPlaybackFrontEnds.erase(it);
                        PAL_INFO(LOG_TAG, "allocateFrontEndIds: front end %d", f[i]);
                        it -= 1;
                        id -= 1;
                    }
                    break;
                default:
                    PAL_ERR(LOG_TAG,"direction unsupported");
                    break;
                }
                break;
        case PAL_STREAM_VOICE_CALL:
            switch (sAttr.direction) {
              case PAL_AUDIO_INPUT | PAL_AUDIO_OUTPUT:
                    if (lDirection == RX_HOSTLESS) {
                        if (sAttr.info.voice_call_info.VSID == VOICEMMODE1 ||
                            sAttr.info.voice_call_info.VSID == VOICELBMMODE1) {
                            f = allocateVoiceFrontEndIds(listAllPcmVoice1RxFrontEnds, howMany);
                        } else if(sAttr.info.voice_call_info.VSID == VOICEMMODE2 ||
                            sAttr.info.voice_call_info.VSID == VOICELBMMODE2){
                            f = allocateVoiceFrontEndIds(listAllPcmVoice2RxFrontEnds, howMany);
                        } else {
                            PAL_ERR(LOG_TAG,"invalid VSID 0x%x provided",
                                    sAttr.info.voice_call_info.VSID);
                        }
                    } else {
                        if (sAttr.info.voice_call_info.VSID == VOICEMMODE1 ||
                            sAttr.info.voice_call_info.VSID == VOICELBMMODE1) {
                            f = allocateVoiceFrontEndIds(listAllPcmVoice1TxFrontEnds, howMany);
                        } else if(sAttr.info.voice_call_info.VSID == VOICEMMODE2 ||
                            sAttr.info.voice_call_info.VSID == VOICELBMMODE2){
                            f = allocateVoiceFrontEndIds(listAllPcmVoice2TxFrontEnds, howMany);
                        } else {
                            PAL_ERR(LOG_TAG,"invalid VSID 0x%x provided",
                                    sAttr.info.voice_call_info.VSID);
                        }
                    }
                    break;
              default:
                  PAL_ERR(LOG_TAG,"direction unsupported voice must be RX and TX");
                  break;
            }
            break;
        case PAL_STREAM_VOICE_CALL_RECORD:
            if (howMany > listAllPcmInCallRecordFrontEnds.size()) {
                    PAL_ERR(LOG_TAG, "allocateFrontEndIds: requested for %d front ends, have only %zu error",
                                      howMany, listAllPcmInCallRecordFrontEnds.size());
                    goto error;
                }
            id = (listAllPcmInCallRecordFrontEnds.size() - 1);
            it = (listAllPcmInCallRecordFrontEnds.begin() + id);
            for (int i = 0; i < howMany; i++) {
                f.push_back(listAllPcmInCallRecordFrontEnds.at(id));
                listAllPcmInCallRecordFrontEnds.erase(it);
                PAL_ERR(LOG_TAG, "allocateFrontEndIds: front end %d", f[i]);
                it -= 1;
                id -= 1;
            }
            break;
        case PAL_STREAM_VOICE_CALL_MUSIC:
            if (howMany > listAllPcmInCallMusicFrontEnds.size()) {
                    PAL_ERR(LOG_TAG, "allocateFrontEndIds: requested for %d front ends, have only %zu error",
                                      howMany, listAllPcmInCallMusicFrontEnds.size());
                    goto error;
                }
            id = (listAllPcmInCallMusicFrontEnds.size() - 1);
            it = (listAllPcmInCallMusicFrontEnds.begin() + id);
            for (int i = 0; i < howMany; i++) {
                f.push_back(listAllPcmInCallMusicFrontEnds.at(id));
                listAllPcmInCallMusicFrontEnds.erase(it);
                PAL_ERR(LOG_TAG, "allocateFrontEndIds: front end %d", f[i]);
                it -= 1;
                id -= 1;
            }
            break;
       case PAL_STREAM_CONTEXT_PROXY:
       case PAL_STREAM_COMMON_PROXY:
            if (howMany > listAllPcmContextProxyFrontEnds.size()) {
                    PAL_ERR(LOG_TAG, "allocateFrontEndIds: requested for %d front ends, have only %zu error",
                                      howMany, listAllPcmContextProxyFrontEnds.size());
                    goto error;
                }
            id = (listAllPcmContextProxyFrontEnds.size() - 1);
            it = (listAllPcmContextProxyFrontEnds.begin() + id);
            for (int i = 0; i < howMany; i++) {
                f.push_back(listAllPcmContextProxyFrontEnds.at(id));
                listAllPcmContextProxyFrontEnds.erase(it);
                PAL_ERR(LOG_TAG, "allocateFrontEndIds: front end %d", f[i]);
                it -= 1;
                id -= 1;
            }
            break;
        default:
            break;
    }

error:
    mListFrontEndsMutex.unlock();
    return f;
}


const std::vector<int> ResourceManager::allocateVoiceFrontEndIds(std::vector<int> listAllPcmVoiceFrontEnds, const int howMany)
{
    std::vector<int> f;
    f.clear();
    if ( howMany > listAllPcmVoiceFrontEnds.size()) {
        PAL_ERR(LOG_TAG, "allocate voice FrontEndIds: requested for %d front ends, have only %zu error",
                howMany, listAllPcmVoiceFrontEnds.size());
        return f;
    }
    for (int i = 0; i < howMany; i++) {
        f.push_back(listAllPcmVoiceFrontEnds.back());
        listAllPcmVoiceFrontEnds.pop_back();
        PAL_INFO(LOG_TAG, "allocate VoiceFrontEndIds: front end %d", f[i]);
    }

    return f;
}
void ResourceManager::freeFrontEndIds(const std::vector<int> frontend,
                                      const struct pal_stream_attributes &sAttr,
                                      int lDirection)
{
    mListFrontEndsMutex.lock();
    if (frontend.size() <= 0) {
        PAL_ERR(LOG_TAG,"frontend size is invalid");
        mListFrontEndsMutex.unlock();
        return;
    }
    PAL_INFO(LOG_TAG, "stream type %d, freeing %d\n", sAttr.type,
             frontend.at(0));

    switch(sAttr.type) {
        case PAL_STREAM_NON_TUNNEL:
            for (int i = 0; i < frontend.size(); i++) {
                 listAllNonTunnelSessionIds.push_back(frontend.at(i));
            }
            removeDuplicates(listAllNonTunnelSessionIds);
            break;
        case PAL_STREAM_LOW_LATENCY:
        case PAL_STREAM_ULTRA_LOW_LATENCY:
        case PAL_STREAM_GENERIC:
        case PAL_STREAM_PROXY:
        case PAL_STREAM_DEEP_BUFFER:
        case PAL_STREAM_SPATIAL_AUDIO:
        case PAL_STREAM_VOIP:
        case PAL_STREAM_VOIP_RX:
        case PAL_STREAM_VOIP_TX:
        case PAL_STREAM_VOICE_UI:
        case PAL_STREAM_LOOPBACK:
        case PAL_STREAM_ACD:
        case PAL_STREAM_ASR:
        case PAL_STREAM_PCM_OFFLOAD:
        case PAL_STREAM_HAPTICS:
        case PAL_STREAM_ULTRASOUND:
        case PAL_STREAM_SENSOR_PCM_DATA:
        case PAL_STREAM_RAW:
        case PAL_STREAM_VOICE_RECOGNITION:
        case PAL_STREAM_SENSOR_PCM_RENDERER:
            switch (sAttr.direction) {
                case PAL_AUDIO_INPUT:
                    if (lDirection == TX_HOSTLESS) {
                        for (int i = 0; i < frontend.size(); i++) {
                            listAllPcmHostlessTxFrontEnds.push_back(frontend.at(i));
                        }
                        removeDuplicates(listAllPcmHostlessTxFrontEnds);
                    } else {
                        for (int i = 0; i < frontend.size(); i++) {
                            listAllPcmRecordFrontEnds.push_back(frontend.at(i));
                        }
                        removeDuplicates(listAllPcmRecordFrontEnds);
                    }
                    break;
                case PAL_AUDIO_OUTPUT:
                    if (lDirection == RX_HOSTLESS) {
                        for (int i = 0; i < frontend.size(); i++) {
                            listAllPcmHostlessRxFrontEnds.push_back(frontend.at(i));
                        }
                        removeDuplicates(listAllPcmHostlessRxFrontEnds);
                    } else {
                        for (int i = 0; i < frontend.size(); i++) {
                             listAllPcmPlaybackFrontEnds.push_back(frontend.at(i));
                        }
                        removeDuplicates(listAllPcmPlaybackFrontEnds);
                    }
                    break;
                case PAL_AUDIO_INPUT | PAL_AUDIO_OUTPUT:
                    if (lDirection == RX_HOSTLESS) {
                        for (int i = 0; i < frontend.size(); i++) {
                            listAllPcmHostlessRxFrontEnds.push_back(frontend.at(i));
                        }
                        removeDuplicates(listAllPcmHostlessRxFrontEnds);
                    } else {
                        for (int i = 0; i < frontend.size(); i++) {
                            listAllPcmHostlessTxFrontEnds.push_back(frontend.at(i));
                        }
                        removeDuplicates(listAllPcmHostlessTxFrontEnds);
                    }
                    break;
                default:
                    PAL_ERR(LOG_TAG,"direction unsupported");
                    break;
            }
            break;

        case PAL_STREAM_VOICE_CALL:
            if (lDirection == RX_HOSTLESS) {
                for (int i = 0; i < frontend.size(); i++) {
                    if (sAttr.info.voice_call_info.VSID == VOICEMMODE1 ||
                        sAttr.info.voice_call_info.VSID == VOICELBMMODE1) {
                        listAllPcmVoice1RxFrontEnds.push_back(frontend.at(i));
                    } else {
                        listAllPcmVoice2RxFrontEnds.push_back(frontend.at(i));
                    }

                }
                removeDuplicates(listAllPcmVoice1RxFrontEnds);
                removeDuplicates(listAllPcmVoice2RxFrontEnds);
            } else {
                for (int i = 0; i < frontend.size(); i++) {
                    if (sAttr.info.voice_call_info.VSID == VOICEMMODE1 ||
                        sAttr.info.voice_call_info.VSID == VOICELBMMODE1) {
                        listAllPcmVoice1TxFrontEnds.push_back(frontend.at(i));
                    } else {
                        listAllPcmVoice2TxFrontEnds.push_back(frontend.at(i));
                    }
                }
                removeDuplicates(listAllPcmVoice1TxFrontEnds);
                removeDuplicates(listAllPcmVoice2TxFrontEnds);
            }
            break;

        case PAL_STREAM_COMPRESSED:
            switch (sAttr.direction) {
                case PAL_AUDIO_INPUT:
                    for (int i = 0; i < frontend.size(); i++) {
                        listAllCompressRecordFrontEnds.push_back(frontend.at(i));
                    }
                    removeDuplicates(listAllCompressRecordFrontEnds);
                    break;
                case PAL_AUDIO_OUTPUT:
                    for (int i = 0; i < frontend.size(); i++) {
                        listAllCompressPlaybackFrontEnds.push_back(frontend.at(i));
                    }
                    removeDuplicates(listAllCompressPlaybackFrontEnds);
                    break;
                default:
                    PAL_ERR(LOG_TAG,"direction unsupported");
                    break;
                }
            break;
        case PAL_STREAM_VOICE_CALL_RECORD:
        case PAL_STREAM_VOICE_CALL_MUSIC:
            switch (sAttr.direction) {
              case PAL_AUDIO_INPUT:
                for (int i = 0; i < frontend.size(); i++) {
                    listAllPcmInCallRecordFrontEnds.push_back(frontend.at(i));
                }
                removeDuplicates(listAllPcmInCallRecordFrontEnds);
                break;
              case PAL_AUDIO_OUTPUT:
                for (int i = 0; i < frontend.size(); i++) {
                    listAllPcmInCallMusicFrontEnds.push_back(frontend.at(i));
                }
                removeDuplicates(listAllPcmInCallMusicFrontEnds);
                break;
              default:
                break;
            }
            break;
       case PAL_STREAM_CONTEXT_PROXY:
       case PAL_STREAM_COMMON_PROXY:
            for (int i = 0; i < frontend.size(); i++) {
                 listAllPcmContextProxyFrontEnds.push_back(frontend.at(i));
            }
            removeDuplicates(listAllPcmContextProxyFrontEnds);
            break;
        default:
            break;
    }
    mListFrontEndsMutex.unlock();
    return;
}

void ResourceManager::getSharedBEActiveStreamDevs(std::vector <std::tuple<Stream *, uint32_t>> &activeStreamsDevices,
                                                  int dev_id)
{
    std::string backEndName;
    std::shared_ptr<Device> dev;
    std::vector <Stream *> activeStreams;
    std::vector <std::tuple<Stream *, uint32_t>>::iterator sIter;
    bool dup = false;

    if (isValidDevId(dev_id) && (dev_id != PAL_DEVICE_NONE))
        backEndName = listAllBackEndIds[dev_id].second;
    for (int i = PAL_DEVICE_OUT_MIN; i < PAL_DEVICE_IN_MAX; i++) {
        if (backEndName == listAllBackEndIds[i].second) {
            dev = Device::getObject((pal_device_id_t) i);
            if(dev) {
                std::list<Stream*>::iterator it;
                for(it = mActiveStreams.begin(); it != mActiveStreams.end(); it++) {
                    std::vector <std::shared_ptr<Device>> devices;
                    (*it)->getAssociatedDevices(devices);
                    typename std::vector<std::shared_ptr<Device>>::iterator result =
                             std::find(devices.begin(), devices.end(), dev);
                    if (result != devices.end())
                        activeStreams.push_back(*it);
                }
                PAL_DBG(LOG_TAG, "got dev %d active streams on dev is %zu", i, activeStreams.size() );
                for (int j=0; j < activeStreams.size(); j++) {
                    /*do not add if this is a dup*/
                    for (sIter = activeStreamsDevices.begin(); sIter != activeStreamsDevices.end(); sIter++) {
                        if ((std::get<0>(*sIter)) == activeStreams[j] &&
                            (std::get<1>(*sIter)) == dev->getSndDeviceId()){
                            dup = true;
                        }
                    }
                    if (!dup) {
                        activeStreamsDevices.push_back({activeStreams[j], dev->getSndDeviceId()});
                        PAL_DBG(LOG_TAG, "found shared BE stream %pK with dev %d", activeStreams[j], dev->getSndDeviceId() );
                    }
                    dup = false;
                }

            }
            activeStreams.clear();
        }
    }
}

bool ResourceManager::compareSharedBEStreamDevAttr(std::vector <std::tuple<Stream *, uint32_t>> &sharedBEStreamDev,
                                                  pal_device *newDevAttr, bool enable)
{
    int status = 0;
    pal_device curDevAttr;
    std::shared_ptr<Device> curDev, newDev = nullptr;
    uint32_t curDevPrio, newDevPrio;
    bool switchStreams = false;

    /* get current active device for shared BE streams */
    curDevAttr.id = (pal_device_id_t)std::get<1>(sharedBEStreamDev[0]);
    curDev = Device::getInstance(&curDevAttr, rm);
    if (!curDev) {
        PAL_ERR(LOG_TAG, "Getting Device instance failed");
        return switchStreams;
    }

    newDev = Device::getInstance(newDevAttr, rm);
    if (!newDev) {
        PAL_ERR(LOG_TAG, "Getting Device instance failed");
        return switchStreams;
    }

    if (enable){
        status = newDev->getTopPriorityDeviceAttr(newDevAttr, &newDevPrio);
        if (status == 0) {
            if (curDevAttr.id == newDevAttr->id) {
                /* if incoming device is the same as running device, compare devAttr */
                curDev->getDeviceAttributes(&curDevAttr);
                if (doDevAttrDiffer(newDevAttr, &curDevAttr))
                    switchStreams = true;
            } else {
                /*
                 * if incoming device is different from running device, check stream priority
                 * for example: voice call is currently active on handset, and later voip call
                 * is setting output to speaker, as voip has lower prioriy, no device switch is needed
                 */
                status = curDev->getTopPriorityDeviceAttr(&curDevAttr, &curDevPrio);
                if (status == 0) {
                    if (newDevPrio <= curDevPrio) {
                        PAL_DBG(LOG_TAG, "incoming dev: %d priority: 0x%x has same or higher priority than cur dev:%d priority: 0x%x",
                                            newDevAttr->id, newDevPrio, curDevAttr.id, curDevPrio);
                        switchStreams = true;
                    } else if (isBtA2dpDevice(newDevAttr->id) && isBtScoDevice(curDevAttr.id) &&
                               !curDev->isDeviceReady()) {
                        /* At the time of VOIP call end, it might happen that Voip Rx stream
                         * will go to standby after a delay. After SCO is disabled, APM will
                         * send routing for streams to A2DP device. At this time due to high
                         * priority stream being active on SCO, routing to A2DP will be ignored.
                         * Special handling to handle such scenarios and route all existing SCO
                         * streams to A2DP as well.
                         */
                        switchStreams = true;
                    } else {
                        PAL_DBG(LOG_TAG, "incoming dev: %d priority: 0x%x has lower priority than cur dev:%d priority: 0x%x,"
                                        " switching incoming stream to cur dev",
                                            newDevAttr->id, newDevPrio, curDevAttr.id, curDevPrio);
                        newDevAttr->id = curDevAttr.id;
                        /*
                         * when switching incoming stream to current device, check if dev atrr differs, like:
                         * UPD stream starts with 96K on upd device , but music is already active on speaker,
                         * UDP needs to align to speaker, to update speaker sample rate to 96KHz, so need to
                         * switch current active streams on speaker to 96K to apply new dev attr
                         */
                        newDev = Device::getInstance(newDevAttr, rm);
                        if (!newDev) {
                            PAL_ERR(LOG_TAG, "Getting Device instance failed");
                            return switchStreams;
                        }
                        curDev->getDeviceAttributes(&curDevAttr);
                        status = newDev->getTopPriorityDeviceAttr(newDevAttr, &newDevPrio);
                        if ((status == 0) && doDevAttrDiffer(newDevAttr, &curDevAttr))
                            switchStreams = true;
                    }
                } else {
                    PAL_DBG(LOG_TAG, "last entry removed from cur dev, switch to new dev");
                    switchStreams = true;
                }
            }
        }
    } else {
        /* during restoreDevice, need to figure out which stream-device attr has highest priority */
        Stream *sharedStream = nullptr;
        std::vector<std::shared_ptr<Device>> palDevices;
        std::multimap<uint32_t, struct pal_device *> streamDevAttr;
        pal_device *sharedBEDevAttr;
        uint32_t sharedBEStreamPrio;

        std::string backEndName_in = listAllBackEndIds[newDevAttr->id].second;
        for (const auto &elem : sharedBEStreamDev) {
            sharedStream = std::get<0>(elem);
            sharedStream->getPalDevices(palDevices);
            /* sort shared BE device attr into map */
            for (int i = 0; i < palDevices.size(); i++) {
                std::string backEndName = listAllBackEndIds[palDevices[i]->getSndDeviceId()].second;
                if(backEndName_in == backEndName) {
                    sharedBEDevAttr = (struct pal_device *) calloc(1, sizeof(struct pal_device));
                    if (!sharedBEDevAttr) {
                        PAL_ERR(LOG_TAG, "failed to allocate memory for pal device");
                        for (auto it = streamDevAttr.begin(); it != streamDevAttr.end(); it++)
                            free((*it).second);
                        return switchStreams;
                    }
                    status = palDevices[i]->getTopPriorityDeviceAttr(sharedBEDevAttr, &sharedBEStreamPrio);
                    if (status == 0)
                        streamDevAttr.insert(std::make_pair(sharedBEStreamPrio, sharedBEDevAttr));
                    else
                        free(sharedBEDevAttr);
                }
            }
            palDevices.clear();
        }
        if (!streamDevAttr.empty()) {
            auto it = streamDevAttr.begin();
            bool skipDevAttrDiffer = false;
            ar_mem_cpy(newDevAttr, sizeof(struct pal_device),
                    (*it).second, sizeof(struct pal_device));
           /*
            * If there're two or more streams on different devices but with same priority,
            * take below scenario for example:
            * <curDev - Speaker>
            * <StreamDevAttr(devices to be restored)>
              ===================================
              |   Stream   | Device  | priority |
              -----------------------------------
              | LowLatency | Handset |    3     |
              -----------------------------------
              | Deepbuffer | Speaker |    3     |
              -----------------------------------
            * In this case, restore device based on below criterion:
            * Check if any active stream is still on curDev, then keep on curDev
            * instead of switching to the first device in map(here it's handset).
            */
            if (streamDevAttr.count((*it).first) > 1) {
                for (auto dev_it = streamDevAttr.begin();
                          dev_it != streamDevAttr.end(); dev_it++) {
                     if ((*dev_it).second->id == curDev->getSndDeviceId()) {
                         PAL_DBG(LOG_TAG, "found remaining stream active on cur dev: %d",
                                 curDev->getSndDeviceId());
                         ar_mem_cpy(newDevAttr, sizeof(struct pal_device),
                                    (*dev_it).second, sizeof(struct pal_device));
                         break;
                     }
                }
            }

            curDev->getDeviceAttributes(&curDevAttr);
            /*
             * special case for speaker, avoid restoredevice if only bit-width is different and
             * the closed stream has higher bit-width.
             * TODO: remove it when we confirm there is no any impact to add 24bit limit in RM.xml.
             */
            if (curDevAttr.id == newDevAttr->id &&
                curDevAttr.id == PAL_DEVICE_OUT_SPEAKER &&
                newDevAttr->config.bit_width < curDevAttr.config.bit_width &&
                newDevAttr->config.sample_rate == curDevAttr.config.sample_rate &&
                newDevAttr->config.ch_info.channels == curDevAttr.config.ch_info.channels) {
                switchStreams = false;
                skipDevAttrDiffer = true;
            }

            if (!skipDevAttrDiffer && doDevAttrDiffer(newDevAttr, &curDevAttr))
                switchStreams = true;

            for (auto it = streamDevAttr.begin(); it != streamDevAttr.end(); it++)
                free((*it).second);
        }
    }

    PAL_INFO(LOG_TAG, "switchStreams is %d", switchStreams);

    return switchStreams;
}

const std::vector<std::string> ResourceManager::getBackEndNames(
        const std::vector<std::shared_ptr<Device>> &deviceList) const
{
    std::vector<std::string> backEndNames;
    std::string epname;
    backEndNames.clear();

    int dev_id;

    for (int i = 0; i < deviceList.size(); i++) {
        dev_id = deviceList[i]->getSndDeviceId();
        PAL_VERBOSE(LOG_TAG, "device id %d", dev_id);
        if (isValidDevId(dev_id)) {
            epname.assign(listAllBackEndIds[dev_id].second);
            backEndNames.push_back(epname);
        } else {
            PAL_ERR(LOG_TAG, "Invalid device id %d", dev_id);
        }
    }

    for (int i = 0; i < backEndNames.size(); i++) {
        PAL_DBG(LOG_TAG, "getBackEndNames: going to return %s", backEndNames[i].c_str());
    }

    return backEndNames;
}

void ResourceManager::getBackEndNames(
        const std::vector<std::shared_ptr<Device>> &deviceList,
        std::vector<std::pair<int32_t, std::string>> &rxBackEndNames,
        std::vector<std::pair<int32_t, std::string>> &txBackEndNames) const
{
    std::string epname;
    rxBackEndNames.clear();
    txBackEndNames.clear();

    int dev_id;

    for (int i = 0; i < deviceList.size(); i++) {
        dev_id = deviceList[i]->getSndDeviceId();
        if (dev_id > PAL_DEVICE_OUT_MIN && dev_id < PAL_DEVICE_OUT_MAX) {
            epname.assign(listAllBackEndIds[dev_id].second);
            rxBackEndNames.push_back(std::make_pair(dev_id, epname));
        } else if (dev_id > PAL_DEVICE_IN_MIN && dev_id < PAL_DEVICE_IN_MAX) {
            epname.assign(listAllBackEndIds[dev_id].second);
            txBackEndNames.push_back(std::make_pair(dev_id, epname));
        } else {
            PAL_ERR(LOG_TAG, "Invalid device id %d", dev_id);
        }
    }

    for (int i = 0; i < rxBackEndNames.size(); i++)
        PAL_DBG(LOG_TAG, "getBackEndNames (RX): %s", rxBackEndNames[i].second.c_str());
    for (int i = 0; i < txBackEndNames.size(); i++)
        PAL_DBG(LOG_TAG, "getBackEndNames (TX): %s", txBackEndNames[i].second.c_str());
}

bool ResourceManager::isValidDeviceSwitchForStream(Stream *s, pal_device_id_t newDeviceId)
{
    struct pal_stream_attributes sAttr;
    int status;
    bool ret = true;

    if (s == NULL || !isValidDevId(newDeviceId)) {
        PAL_ERR(LOG_TAG, "Invalid input\n");
        return false;
    }

    status = s->getStreamAttributes(&sAttr);
    if (status) {
        PAL_ERR(LOG_TAG,"getStreamAttributes Failed \n");
        return false;
    }

    switch (sAttr.type) {
    case PAL_STREAM_ULTRASOUND:
    case PAL_STREAM_SENSOR_PCM_RENDERER:
        switch (newDeviceId) {
        case PAL_DEVICE_OUT_HANDSET:
        case PAL_DEVICE_OUT_SPEAKER:
            ret = true;
            /*
             * if upd shares BE with handset, while handset doesn't
             * share BE with speaker, then during device switch
             * between handset and speaker, upd should still stay
             * on handset
             */
            if (listAllBackEndIds[PAL_DEVICE_OUT_HANDSET].second !=
                listAllBackEndIds[PAL_DEVICE_OUT_SPEAKER].second)
                ret = false;
            break;
        default:
            ret = false;
            break;
        }
        break;
    default:
        if (!isValidStreamId(sAttr.type)) {
            PAL_DBG(LOG_TAG, "Invalid stream type\n");
            return false;
        }
        ret = true;
        break;
    }

    if (!ret) {
        PAL_DBG(LOG_TAG, "Skip switching stream %d (%s) to device %d (%s)\n",
                sAttr.type, streamNameLUT.at(sAttr.type).c_str(),
                newDeviceId, deviceNameLUT.at(newDeviceId).c_str());
    }

    return ret;
}

/*
 * Due to compatibility challenges, some streams may not switch to the new device
 * during device switching.
 * Example: In the shared backend configuration, the UPD stream prevents switching
 * to devices other than the handset or speaker.
 * In this function, we compare the list of streams in streamDevDisconnectList
 * to the streams that are currently active on that device.
 * The streamsSkippingSwitch list consists of any stream that is present in the
 * active streams list but missing from the list of disconnecting streams.
 */
int ResourceManager::findActiveStreamsNotInDisconnectList(
        std::vector <std::tuple<Stream *, uint32_t>> &streamDevDisconnectList,
        std::vector <std::tuple<Stream *, uint32_t>> &streamsSkippingSwitch)
{
    std::set<Stream *> disconnectingStreams;
    std::vector <std::tuple<Stream *, uint32_t>>::iterator iter;
    std::shared_ptr<Device> devObj = nullptr;
    struct pal_device dAttr;
    std::vector<Stream*> activeStreams;
    std::vector<Stream*>::iterator sIter;
    int ret = 0;

    if (streamDevDisconnectList.size() == 0) {
        PAL_DBG(LOG_TAG, "Empty stream disconnect list");
        return ret;
    }

    /*
     * To facilitate comparisons against the list of active streams simpler,
     * construct a set of streams that are disconnecting from streamDevDisconnectList
     */
    for (iter = streamDevDisconnectList.begin();
            iter != streamDevDisconnectList.end(); iter++) {
        if (std::get<0>(*iter) != NULL)
            disconnectingStreams.insert(std::get<0>(*iter));
    }

    /*
     * Instance of a device object from which the active stream list is retrieved.
     * It's the same device from which streams in 'disconnectingStreams' are
     * disconnecting.
     */
    dAttr.id = (pal_device_id_t)std::get<1>(streamDevDisconnectList[0]);
    devObj = Device::getInstance(&dAttr, rm);
    if (devObj == nullptr) {
        PAL_DBG(LOG_TAG, "Error getting device ( %s ) instance",
                deviceNameLUT.at(dAttr.id).c_str());
        return ret;
    }

    mActiveStreamMutex.lock();

    rm->getActiveStream_l(activeStreams, devObj);

    PAL_DBG(LOG_TAG, "activeStreams size = %d, device: %s", activeStreams.size(),
            deviceNameLUT.at((pal_device_id_t)devObj->getSndDeviceId()).c_str());

    for (sIter = activeStreams.begin(); sIter != activeStreams.end(); sIter++) {
        if (disconnectingStreams.find(*sIter) != disconnectingStreams.end())
            continue;

        /*
         * Stream that is present in the active streams list but not in the list
         * of disconnecting streams, contribute to the streamsSkippingSwitch list.
         */
        streamsSkippingSwitch.push_back({(*sIter), dAttr.id});
    }

done:
    mActiveStreamMutex.unlock();
    return ret;
}

/*
 * For shared backend setup, it's crucial to restore the correct device configuration
 * for the UPD stream.
 *
 * Example 1: As music is being played on the speaker, the UPD stream begins (on
 * the speaker). The Music stream switches to the new device when a wired headset
 * or a BT device is connected. In this scenario, the UPD stream will be the only
 * active stream on Speaker device. Thus, we must ensure to switch it from Speaker
 * (NLPI) to Handset(LPI).
 *
 * Example 2: During a voice call concurrency with the UPD stream, when a wired
 * headset or a BT device is connected, The voice call stream switches to the new
 * device and the UPD continues to run on the handset device. In this case, we
 * configure the handset device to make it compatible to UPD only stream.
 */
int ResourceManager::restoreDeviceConfigForUPD(
        std::vector <std::tuple<Stream *, uint32_t>> &streamDevDisconnect,
        std::vector <std::tuple<Stream *, struct pal_device *>> &StreamDevConnect,
        std::vector <std::tuple<Stream *, uint32_t>> &streamsSkippingSwitch)
{
    int ret = 0;
    static struct pal_device dAttr; //struct reference is passed in StreamDevConnect.
    struct pal_device curDevAttr = {};
    std::shared_ptr<Device> hs_dev = nullptr;
    uint32_t devId;
    Stream *s;
    struct pal_stream_attributes sAttr;
    std::vector<std::shared_ptr<Device>> palDevices;

    if (rm->IsDedicatedBEForUPDEnabled() || rm->IsVirtualPortForUPDEnabled()) {
        PAL_DBG(LOG_TAG, "This UPD config requires no restoration");
        return ret;
    }

    if (streamDevDisconnect.size() == 0) {
        PAL_DBG(LOG_TAG, "Empty disconnect streams list. UPD backend needs no update");
        return ret;
    }

    /*
     * The streams that are skipping the switch to the new device are obtained
     * using the findActiveStreamsNotInDisconnectList method if not obtained via
     * the isValidDevicSwitchForStream path.
     */
    if (streamsSkippingSwitch.size() == 0) {
        ret = findActiveStreamsNotInDisconnectList(streamDevDisconnect,
                                                   streamsSkippingSwitch);
        if (ret)
            goto exit_on_error;
    }

    /*
     * The UPD stream is allowed to continue running on the device it is configured
     * for if the streamsSkippingSwitch size is greater than 1.
     */
    if (streamsSkippingSwitch.size() == 0 || streamsSkippingSwitch.size() > 1) {
        PAL_DBG(LOG_TAG, "UPD backend needs no update");
        return ret;
    }

    s = std::get<0>(streamsSkippingSwitch[0]);
    devId = (pal_device_id_t)std::get<1>(streamsSkippingSwitch[0]);

    if (s == NULL) {
        PAL_DBG(LOG_TAG, "Invalid stream pointer");
        ret = -EINVAL;
        goto exit_on_error;
    }

    ret = s->getStreamAttributes(&sAttr);
    if (ret)
        goto exit_on_error;

    if (sAttr.type != PAL_STREAM_ULTRASOUND &&
        sAttr.type != PAL_STREAM_SENSOR_PCM_RENDERER) {
        PAL_DBG(LOG_TAG, "Not a UPD stream. No UPD backend to update");
        return ret;
    }

    memset(&dAttr, 0, sizeof(struct pal_device));

    s->getPalDevices(palDevices);
    if (palDevices.size() == 0) {
        PAL_ERR(LOG_TAG, "Stream doesn't have pal device attached");
        ret = -EINVAL;
        goto exit_on_error;
    }
    dAttr.id = (pal_device_id_t)palDevices[0]->getSndDeviceId();

    ret = rm->getDeviceConfig(&dAttr, &sAttr);
    if (ret) {
        PAL_ERR(LOG_TAG, "Error getting deviceConfig");
        goto exit_on_error;
    }

    if (sAttr.type == PAL_STREAM_ULTRASOUND &&
        devId == PAL_DEVICE_OUT_HANDSET) {
        hs_dev = Device::getObject(PAL_DEVICE_OUT_HANDSET);
        if (hs_dev)
            hs_dev->getDeviceAttributes(&curDevAttr);

        if (!doDevAttrDiffer(&dAttr, &curDevAttr)) {
            PAL_DBG(LOG_TAG, "No need to update device attr for UPD");
            return ret;
        }
    }

    /*
     * We restore the device configuration appropriately if UPD is the only stream
     * skipping the switch to the new device and will be the only active stream
     * still present on the current device.
     *
     * If the current device is Speaker, the UPD stream is switched from speaker
     * (NLPI) to handset (LPI).
     *
     * If the current device is handset, the device is configured to limit it's
     * compatibility to UPD stream only.
     * For sensor renderer stream, always restore from handset/speaker to upd
     * dedicated device attribute for NLPI to LPI switch.
     */
    PAL_DBG(LOG_TAG, "Restoring UPD stream device from cur dev:%d to new dev: %d",
                     devId, dAttr.id );

    streamDevDisconnect.push_back(streamsSkippingSwitch[0]);
    StreamDevConnect.push_back({s, &dAttr});

    return ret;

exit_on_error:
    PAL_ERR(LOG_TAG, "Error updating UPD backend");
    return ret;
}

int32_t ResourceManager::streamDevDisconnect(std::vector <std::tuple<Stream *, uint32_t>> streamDevDisconnectList){
    int status = 0;
    std::vector <std::tuple<Stream *, uint32_t>>::iterator sIter;

    PAL_DBG(LOG_TAG, "Enter");

    /* disconnect active list from the current devices they are attached to */
    for (sIter = streamDevDisconnectList.begin(); sIter != streamDevDisconnectList.end(); sIter++) {
        if ((std::get<0>(*sIter) != NULL) && isStreamActive(std::get<0>(*sIter), mActiveStreams)) {
#ifdef SEC_AUDIO_AMP_BIGDATA
            struct pal_device dattr = {};
            std::shared_ptr<Device> dev = nullptr;
            dattr.id = (pal_device_id_t)std::get<1>(*sIter);
            if (dattr.id == PAL_DEVICE_OUT_SPEAKER) {
                dev = Device::getInstance(&dattr, rm);
                if (dev) {
                    PAL_DBG(LOG_TAG, "Got Speaker instance for Feedback stream");
                    dev->setParameter(PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_STOP, nullptr);
                } else {
                    PAL_DBG(LOG_TAG, "Unable to get speaker instance");
                }
            }
#endif
            status = (std::get<0>(*sIter))->disconnectStreamDevice(std::get<0>(*sIter), (pal_device_id_t)std::get<1>(*sIter));
            if (status) {
                PAL_ERR(LOG_TAG, "failed to disconnect stream %pK from device %d",
                        std::get<0>(*sIter), std::get<1>(*sIter));
                goto error;
            } else {
                PAL_DBG(LOG_TAG, "disconnect stream %pK from device %d",
                       std::get<0>(*sIter), std::get<1>(*sIter));
            }
        }
    }
error:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int32_t ResourceManager::streamDevConnect(std::vector <std::tuple<Stream *, struct pal_device *>> streamDevConnectList){
    int status = 0;
    std::vector <std::tuple<Stream *, struct pal_device *>>::iterator sIter;

    PAL_DBG(LOG_TAG, "Enter");
    /* connect active list from the current devices they are attached to */
    for (sIter = streamDevConnectList.begin(); sIter != streamDevConnectList.end(); sIter++) {
        if ((std::get<0>(*sIter) != NULL) && isStreamActive(std::get<0>(*sIter), mActiveStreams)) {
            status = std::get<0>(*sIter)->connectStreamDevice(std::get<0>(*sIter), std::get<1>(*sIter));
            if (status) {
                PAL_ERR(LOG_TAG,"failed to connect stream %pK from device %d",
                        std::get<0>(*sIter), (std::get<1>(*sIter))->id);
                goto error;
            } else {
                PAL_DBG(LOG_TAG,"connected stream %pK from device %d",
                        std::get<0>(*sIter), (std::get<1>(*sIter))->id);
            }
#ifdef SEC_AUDIO_AMP_BIGDATA
            struct pal_device dattr = {};
            std::shared_ptr<Device> dev = nullptr;
            dattr.id = (std::get<1>(*sIter))->id;
            if (dattr.id == PAL_DEVICE_OUT_SPEAKER) {
                dev = Device::getInstance(&dattr, rm);
                if (dev) {
                    PAL_DBG(LOG_TAG, "Got Speaker instance for Feedback stream");
                    dev->setParameter(PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_START, nullptr);
                } else {
                    PAL_DBG(LOG_TAG, "Unable to get speaker instance");
                }
            }
#endif
        }
    }
error:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int32_t ResourceManager::streamDevDisconnect_l(std::vector <std::tuple<Stream *, uint32_t>> streamDevDisconnectList){
    int status = 0;
    std::vector <std::tuple<Stream *, uint32_t>>::iterator sIter;

    PAL_DBG(LOG_TAG, "Enter");

    /* disconnect active list from the current devices they are attached to */
    for (sIter = streamDevDisconnectList.begin(); sIter != streamDevDisconnectList.end(); sIter++) {
        if ((std::get<0>(*sIter) != NULL) && isStreamActive(std::get<0>(*sIter), mActiveStreams)) {
#ifdef SEC_AUDIO_AMP_BIGDATA
            struct pal_device dattr = {};
            std::shared_ptr<Device> dev = nullptr;
            dattr.id = (pal_device_id_t)std::get<1>(*sIter);
            if (dattr.id == PAL_DEVICE_OUT_SPEAKER) {
                dev = Device::getInstance(&dattr, rm);
                if (dev) {
                    PAL_DBG(LOG_TAG, "Got Speaker instance for Feedback stream");
                    dev->setParameter(PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_STOP, nullptr);
                } else {
                    PAL_DBG(LOG_TAG, "Unable to get speaker instance");
                }
            }
#endif
            status = (std::get<0>(*sIter))->disconnectStreamDevice_l(std::get<0>(*sIter), (pal_device_id_t)std::get<1>(*sIter));
            if (status) {
                PAL_ERR(LOG_TAG, "failed to disconnect stream %pK from device %d",
                        std::get<0>(*sIter), std::get<1>(*sIter));
                goto error;
            } else {
                PAL_DBG(LOG_TAG, "disconnect stream %pK from device %d",
                       std::get<0>(*sIter), std::get<1>(*sIter));
            }
#ifdef SEC_AUDIO_MUTE_DETECTION
            pal_device_id_t device = (pal_device_id_t)std::get<1>(*sIter);
            if (device == PAL_DEVICE_OUT_SPEAKER || device == PAL_DEVICE_OUT_HANDSET) {
                struct pal_device dattr = {};
                dattr.id = device;
                std::shared_ptr<Device> dev = Device::getInstance(&dattr, rm);
                if (dev && dev->getDeviceCount() == 0) {
                    getKernelConfigInfo(RX_CONFIG_INFO); // after disconnect device.
                }
            }
#endif
        }
    }
error:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int32_t ResourceManager::streamDevConnect_l(std::vector <std::tuple<Stream *, struct pal_device *>> streamDevConnectList){
    int status = 0;
    std::vector <std::tuple<Stream *, struct pal_device *>>::iterator sIter;

    PAL_DBG(LOG_TAG, "Enter");
    /* connect active list from the current devices they are attached to */
    for (sIter = streamDevConnectList.begin(); sIter != streamDevConnectList.end(); sIter++) {
        if ((std::get<0>(*sIter) != NULL) && isStreamActive(std::get<0>(*sIter), mActiveStreams)) {
            status = std::get<0>(*sIter)->connectStreamDevice_l(std::get<0>(*sIter), std::get<1>(*sIter));
            if (status) {
                PAL_ERR(LOG_TAG,"failed to connect stream %pK from device %d",
                        std::get<0>(*sIter), (std::get<1>(*sIter))->id);
            } else {
                PAL_DBG(LOG_TAG,"connected stream %pK from device %d",
                        std::get<0>(*sIter), (std::get<1>(*sIter))->id);
            }
#ifdef SEC_AUDIO_AMP_BIGDATA
            struct pal_device dattr = {};
            std::shared_ptr<Device> dev = nullptr;
            dattr.id = (std::get<1>(*sIter))->id;
            if (dattr.id == PAL_DEVICE_OUT_SPEAKER) {
                dev = Device::getInstance(&dattr, rm);
                if (dev) {
                    PAL_DBG(LOG_TAG, "Got Speaker instance for Feedback stream");
                    dev->setParameter(PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_START, nullptr);
                } else {
                    PAL_DBG(LOG_TAG, "Unable to get speaker instance");
                }
            }
#endif
#ifdef SEC_AUDIO_MUTE_DETECTION
            pal_device_id_t device = (std::get<1>(*sIter))->id;
            if (device == PAL_DEVICE_OUT_SPEAKER || device == PAL_DEVICE_OUT_HANDSET) {
                struct pal_device dattr = {};
                dattr.id = device;
                std::shared_ptr<Device> dev = Device::getInstance(&dattr, rm);
                if (dev && dev->getDeviceCount() == 1) {
                    getKernelConfigInfo(RX_CONFIG_INFO); // after connect device.
                }
            }
#endif
            std::get<0>(*sIter)->unlockStreamMutex();
        }
    }

    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}


template <class T>
void SortAndUnique(std::vector<T> &streams)
{
    std::sort(streams.begin(), streams.end());
    typename std::vector<T>::iterator iter =
        std::unique(streams.begin(), streams.end());
    streams.erase(iter, streams.end());
    return;
}

int32_t ResourceManager::streamDevSwitch(std::vector <std::tuple<Stream *, uint32_t>> streamDevDisconnectList,
                                         std::vector <std::tuple<Stream *, struct pal_device *>> streamDevConnectList)
{
    int status = 0;
    std::vector <Stream*>::iterator sIter;
    std::vector <struct pal_device *>::iterator dIter;
    std::vector <std::tuple<Stream *, uint32_t>>::iterator sIter1;
    std::vector <std::tuple<Stream *, struct pal_device *>>::iterator sIter2;
    std::vector <Stream*> uniqueStreamsList;
    std::vector <struct pal_device *> uniqueDevConnectionList;
    pal_stream_attributes sAttr;

#ifdef SEC_AUDIO_USB_PROFILING
    struct pal_device DisconnectDev, ConnectDev;
#endif
    PAL_INFO(LOG_TAG, "Enter");

    mActiveStreamMutex.lock();

#ifdef SEC_AUDIO_USB_PROFILING
    for (const auto &elem : streamDevDisconnectList) {
        DisconnectDev.id = (pal_device_id_t)std::get<1>(elem);
    }
    for (const auto &elem : streamDevConnectList) {
        ConnectDev.id = (std::get<1>(elem))->id;
    }

    if ((DisconnectDev.id == PAL_DEVICE_OUT_SPEAKER) && (ConnectDev.id == PAL_DEVICE_OUT_USB_HEADSET))
        PAL_INFO(LOG_TAG, "USB_Profiling_routing_request");
#endif

    SortAndUnique(streamDevDisconnectList);
    SortAndUnique(streamDevConnectList);

    /* Need to lock all streams that are involved in devSwitch
     * When we are doing Switch to avoid any stream specific calls to happen.
     * We want to avoid stream close or any other control operations to happen when we are in the
     * middle of the switch
     */
    for (sIter1 = streamDevDisconnectList.begin(); sIter1 != streamDevDisconnectList.end(); sIter1++) {
        if ((std::get<0>(*sIter1) != NULL) && isStreamActive(std::get<0>(*sIter1), mActiveStreams)) {
            uniqueStreamsList.push_back(std::get<0>(*sIter1));
            PAL_VERBOSE(LOG_TAG, "streamDevDisconnectList stream %pK", std::get<0>(*sIter1));
        }
    }

    for (sIter2 = streamDevConnectList.begin(); sIter2 != streamDevConnectList.end(); sIter2++) {
        if ((std::get<0>(*sIter2) != NULL) && isStreamActive(std::get<0>(*sIter2), mActiveStreams)) {
            uniqueStreamsList.push_back(std::get<0>(*sIter2));
            PAL_VERBOSE(LOG_TAG, "streamDevConnectList stream %pK", std::get<0>(*sIter2));
            uniqueDevConnectionList.push_back(std::get<1>(*sIter2));
        }
    }

    // Find and Removedup elements between streamDevDisconnectList && streamDevConnectList and add to the list.
    SortAndUnique(uniqueStreamsList);
    SortAndUnique(uniqueDevConnectionList);

    // handle scenario where BT device is not ready
    for (dIter = uniqueDevConnectionList.begin(); dIter != uniqueDevConnectionList.end(); dIter++) {
        if ((uniqueDevConnectionList.size() == 1) &&
                ((((*dIter)->id == PAL_DEVICE_OUT_BLUETOOTH_A2DP) &&
                !isDeviceReady(PAL_DEVICE_OUT_BLUETOOTH_A2DP)) ||
                (((*dIter)->id == PAL_DEVICE_OUT_BLUETOOTH_BLE) &&
                !isDeviceReady(PAL_DEVICE_OUT_BLUETOOTH_BLE)) ||
                (((*dIter)->id == PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST) &&
                !isDeviceReady(PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST)))) {
            PAL_ERR(LOG_TAG, "a2dp/ble device is not ready for connection, skip device switch");
            status = -ENODEV;
            mActiveStreamMutex.unlock();
            goto exit_no_unlock;
        }
    }

    // lock all stream mutexes
    for (sIter = uniqueStreamsList.begin(); sIter != uniqueStreamsList.end(); sIter++) {
        PAL_DBG(LOG_TAG, "uniqueStreamsList stream %pK lock", (*sIter));
        (*sIter)->lockStreamMutex();
    }
    isDeviceSwitch = true;

    for (sIter = uniqueStreamsList.begin(); sIter != uniqueStreamsList.end(); sIter++) {
        status = (*sIter)->getStreamAttributes(&sAttr);
        Session *session = NULL;
        if (status != 0) {
            PAL_ERR(LOG_TAG,"stream get attributes failed");
            continue;
        }
        if (sAttr.direction == PAL_AUDIO_OUTPUT && sAttr.type == PAL_STREAM_ULTRA_LOW_LATENCY) {
            (*sIter)->getAssociatedSession(&session);
            if ((session != NULL) && (*sIter)->isActive())
                session->AdmRoutingChange((*sIter));
        }
    }

    status = streamDevDisconnect_l(streamDevDisconnectList);
    if (status) {
        PAL_ERR(LOG_TAG, "disconnect failed");
        goto exit;
    }

#ifdef SEC_AUDIO_USB_PROFILING
    if ((DisconnectDev.id == PAL_DEVICE_OUT_SPEAKER) && (ConnectDev.id == PAL_DEVICE_OUT_USB_HEADSET))
        PAL_INFO(LOG_TAG, "USB_Profiling_disabled_speaker_and_enable_usb");
#endif

    status = streamDevConnect_l(streamDevConnectList);
    if (status) {
        PAL_ERR(LOG_TAG, "Connect failed");
    }

    for (sIter2 = streamDevConnectList.begin(); sIter2 != streamDevConnectList.end(); sIter2++) {
        if ((std::get<0>(*sIter2) != NULL) && isStreamActive(std::get<0>(*sIter2), mActiveStreams)) {
            for (sIter = uniqueStreamsList.begin(); sIter != uniqueStreamsList.end(); sIter++) {
                if (*sIter == std::get<0>(*sIter2)) {
                    uniqueStreamsList.erase(sIter);
                    PAL_VERBOSE(LOG_TAG, "already unlocked, remove stream %pK from list",
                                std::get<0>(*sIter2));
                    break;
                }
            }
        }
    }

#ifdef SEC_AUDIO_USB_PROFILING
    if ((DisconnectDev.id == PAL_DEVICE_OUT_SPEAKER) && (ConnectDev.id == PAL_DEVICE_OUT_USB_HEADSET))
        PAL_INFO(LOG_TAG, "USB_Profiling_enabled_usb_headset");
#endif

exit:
    // unlock all stream mutexes
    for (sIter = uniqueStreamsList.begin(); sIter != uniqueStreamsList.end(); sIter++) {
        PAL_DBG(LOG_TAG, "uniqueStreamsList stream %pK unlock", (*sIter));
        (*sIter)->unlockStreamMutex();
    }
    isDeviceSwitch = false;
    mActiveStreamMutex.unlock();
exit_no_unlock:
    PAL_INFO(LOG_TAG, "Exit status: %d", status);
    return status;
}

void ResourceManager::handleA2dpBleConcurrency(std::shared_ptr<Device> *inDev,
        struct pal_device *inDevAttr, struct pal_device &dummyDevAttr,
        std::vector <std::tuple<Stream *, uint32_t>> &streamDevDisconnect,
        std::vector <std::tuple<Stream *, struct pal_device *>> &streamDevConnect)
{
    struct pal_device devAttr = {};
    std::shared_ptr<Device> dev = nullptr;
    std::vector <Stream *> streams;
    std::vector <Stream *>::iterator sIter;

    if (inDevAttr->id == PAL_DEVICE_IN_BLUETOOTH_BLE) {
        devAttr.id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
        dev = Device::getInstance(&devAttr, rm);
        if (!dev) {
            PAL_ERR(LOG_TAG, "getting a2dp/ble device instance failed");
            return;
        }
        getActiveStream_l(streams, dev);
        if (streams.size() == 0) {
            return;
        }
        dummyDevAttr.id = PAL_DEVICE_OUT_DUMMY;
        if (getDeviceConfig(&dummyDevAttr, NULL)) {
            PAL_ERR(LOG_TAG, "getDeviceConfig failed for out_dummy device");
            return;
        }
        for (sIter = streams.begin(); sIter != streams.end(); sIter++) {
            streamDevDisconnect.push_back({(*sIter), PAL_DEVICE_OUT_BLUETOOTH_A2DP});
            streamDevConnect.push_back({(*sIter), &dummyDevAttr});
        }
    } else if (inDevAttr->id == PAL_DEVICE_OUT_BLUETOOTH_A2DP) {
        devAttr.id = PAL_DEVICE_IN_BLUETOOTH_BLE;
        dev = Device::getInstance(&devAttr, rm);
        getActiveStream_l(streams, dev);
        if (streams.size() > 0) {
            inDevAttr->id = PAL_DEVICE_OUT_DUMMY;
            if (getDeviceConfig(inDevAttr, NULL)) {
                PAL_ERR(LOG_TAG, "getDeviceConfig failed for out_dummy device");
                inDevAttr->id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
                return;
            }
            *inDev = Device::getInstance(inDevAttr , rm);
        }
    }
}


/* when returning from this function, the device config will be updated with
 * the device config of the highest priority stream
 * TBD: manage re-routing of existing lower priority streams if incoming
 * stream is a higher priority stream. Priority defined in ResourceManager.h
 * (details below)
 */
bool ResourceManager::updateDeviceConfig(std::shared_ptr<Device> *inDev,
           struct pal_device *inDevAttr, const pal_stream_attributes* inStrAttr)
{
    bool isDeviceSwitch = false;
    int status = 0;
    std::vector <std::tuple<Stream *, uint32_t>> streamDevDisconnect, sharedBEStreamDev;
    std::vector <std::tuple<Stream *, struct pal_device *>> streamDevConnect;
    std::string ck;
    bool VoiceorVoip_call_active = false;
    struct pal_device_info inDeviceInfo;
    struct pal_device dummyDevAttr = {};
    std::vector <Stream *> streamsToSwitch;
    std::vector <Stream*>::iterator sIter;
    struct pal_device streamDevAttr;

    PAL_DBG(LOG_TAG, "Enter");

    if (!inDev || !inDevAttr || !inStrAttr) {
        PAL_ERR(LOG_TAG, "invalid input parameters");
        goto error;
    }

    /* Soundtrigger stream device attributes is updated via capture profile.
     * PAL_STREAM_SENSOR_PCM_DATA device attributes will be updated here due
     * to it receives device configurations from Sensor clients.
     */
    if (inStrAttr->type == PAL_STREAM_ACD ||
        inStrAttr->type == PAL_STREAM_VOICE_UI ||
        inStrAttr->type == PAL_STREAM_ASR)
        goto error;

    if (strlen(inDevAttr->custom_config.custom_key))
        ck.assign(inDevAttr->custom_config.custom_key);

    rm->getDeviceInfo(inDevAttr->id, inStrAttr->type,
                      inDevAttr->custom_config.custom_key, &inDeviceInfo);

    mActiveStreamMutex.lock();
    /* handle headphone and haptics concurrency */
    if (!ResourceManager::isHapticsthroughWSA)
        checkHapticsConcurrency(inDevAttr, inStrAttr, streamsToSwitch, &streamDevAttr);

    for (sIter = streamsToSwitch.begin(); sIter != streamsToSwitch.end(); sIter++) {
        streamDevDisconnect.push_back({(*sIter), streamDevAttr.id});
        streamDevConnect.push_back({(*sIter), &streamDevAttr});
    }
    streamsToSwitch.clear();

    /* handle IN_BLE and A2DP concurrency */
    handleA2dpBleConcurrency(inDev, inDevAttr, dummyDevAttr,
                             streamDevDisconnect, streamDevConnect);

    // check if device has virtual port enabled, update the active group devcie config
    // if streams has same virtual backend, it will be handled in shared backend case
    status = checkAndUpdateGroupDevConfig(inDevAttr, inStrAttr, streamsToSwitch, &streamDevAttr, true);
    if (status) {
        PAL_ERR(LOG_TAG, "no valid group device config found");
        streamsToSwitch.clear();
    }

    /* get the active streams on the device
     * if higher priority stream exists on any of the incoming device, update the config of incoming device
     * based on device config of higher priority stream
     * check if there are shared backends
     * if yes add them to streams to device switch
     */
    getSharedBEActiveStreamDevs(sharedBEStreamDev, inDevAttr->id);
    if (sharedBEStreamDev.size() > 0) {
        /* check to see if attrs changed, inDevAttr will be updated if switch is needed */
        bool switchStreams = compareSharedBEStreamDevAttr(sharedBEStreamDev, inDevAttr, true/* enable device */);
        for (const auto &elem : sharedBEStreamDev) {
            if (switchStreams && isDeviceReady(inDevAttr->id)) {
                streamDevDisconnect.push_back(elem);
                streamDevConnect.push_back({std::get<0>(elem), inDevAttr});
                isDeviceSwitch = true;
            }
        }
        // update the dev instance in case the incoming device is changed to the running device
        *inDev = Device::getInstance(inDevAttr , rm);
    } else {
        // if there is no shared backend just updated the snd device name
        updateSndName(inDevAttr->id, inDeviceInfo.sndDevName);

        /* handle special case for UPD with virtual backend */
        if (!streamsToSwitch.empty()) {
            for (sIter = streamsToSwitch.begin(); sIter != streamsToSwitch.end(); sIter++) {
                streamDevDisconnect.push_back({(*sIter), streamDevAttr.id});
                streamDevConnect.push_back({(*sIter), &streamDevAttr});
            }
        }
    }
    mActiveStreamMutex.unlock();

    // if device switch is needed, perform it
    if (streamDevDisconnect.size()) {
        status = streamDevSwitch(streamDevDisconnect, streamDevConnect);
        if (status) {
            PAL_ERR(LOG_TAG, "deviceswitch failed with %d", status);
        }
    }
    (*inDev)->setDeviceAttributes(*inDevAttr);

error:
    PAL_DBG(LOG_TAG, "Exit");
    return isDeviceSwitch;
}

int32_t ResourceManager::forceDeviceSwitch(std::shared_ptr<Device> inDev,
                                           struct pal_device *newDevAttr)
{
    int status = 0;
    std::vector <Stream *> activeStreams;
    std::vector <std::tuple<Stream *, uint32_t>> streamDevDisconnect, streamsSkippingSwitch;
    std::vector <std::tuple<Stream *, struct pal_device *>> streamDevConnect;
    std::vector<Stream*>::iterator sIter;

    if (!inDev || !newDevAttr) {
        PAL_ERR(LOG_TAG, "invalid input parameters");
        return -EINVAL;
    }

    // get active streams on the device
    mActiveStreamMutex.lock();
    getActiveStream_l(activeStreams, inDev);
    if (activeStreams.size() == 0) {
        PAL_ERR(LOG_TAG, "no other active streams found");
        mActiveStreamMutex.unlock();
        goto done;
    }

    // create dev switch vectors
    for (sIter = activeStreams.begin(); sIter != activeStreams.end(); sIter++) {
        if (!isValidDeviceSwitchForStream((*sIter), newDevAttr->id)) {
            if (*sIter != NULL)
                streamsSkippingSwitch.push_back({(*sIter), inDev->getSndDeviceId()});
            continue;
        }

        streamDevDisconnect.push_back({(*sIter), inDev->getSndDeviceId()});
        streamDevConnect.push_back({(*sIter), newDevAttr});
    }

    mActiveStreamMutex.unlock();

    status = rm->restoreDeviceConfigForUPD(streamDevDisconnect, streamDevConnect,
                                           streamsSkippingSwitch);
    if (status) {
        PAL_DBG(LOG_TAG, "Error restoring device config for UPD");
        return status;
    }

    status = streamDevSwitch(streamDevDisconnect, streamDevConnect);
    if (!status) {
        mActiveStreamMutex.lock();
        for (sIter = activeStreams.begin(); sIter != activeStreams.end(); sIter++) {
            if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
                (*sIter)->lockStreamMutex();
                if (ResourceManager::isDummyDevEnabled) {
                    (*sIter)->removePalDevice(*sIter, inDev->getSndDeviceId());
                } else {
                    (*sIter)->clearOutPalDevices(*sIter);
                }
                (*sIter)->addPalDevice(*sIter, newDevAttr);
                (*sIter)->unlockStreamMutex();
            }
        }
        mActiveStreamMutex.unlock();
    } else {
        PAL_ERR(LOG_TAG, "forceDeviceSwitch failed %d", status);
    }

done:
    return 0;
}

int32_t ResourceManager::forceDeviceSwitch(std::shared_ptr<Device> inDev,
                                           struct pal_device *newDevAttr,
                                           std::vector<Stream*> prevActiveStreams)
{
    int status = 0;
    std::vector <std::tuple<Stream *, uint32_t>> streamDevDisconnect, streamsSkippingSwitch;
    std::vector <std::tuple<Stream *, struct pal_device *>> streamDevConnect;
    std::vector<Stream*>::iterator sIter;

    if (!inDev || !newDevAttr) {
        PAL_ERR(LOG_TAG, "invalid input parameters");
        return -EINVAL;
    }

    // create dev switch vectors
    mActiveStreamMutex.lock();
    for (sIter = prevActiveStreams.begin(); sIter != prevActiveStreams.end(); sIter++) {
        if (((*sIter) != NULL) && isStreamActive((*sIter), mActiveStreams)) {
            if (!isValidDeviceSwitchForStream((*sIter), newDevAttr->id)) {
                if (*sIter != NULL)
                    streamsSkippingSwitch.push_back({(*sIter), inDev->getSndDeviceId()});
                continue;
            }

            streamDevDisconnect.push_back({(*sIter), inDev->getSndDeviceId()});
            streamDevConnect.push_back({(*sIter), newDevAttr});
        }
    }

    mActiveStreamMutex.unlock();

    status = rm->restoreDeviceConfigForUPD(streamDevDisconnect, streamDevConnect,
                                           streamsSkippingSwitch);
    if (status) {
        PAL_DBG(LOG_TAG, "Error restoring device config for UPD");
        return status;
    }

    status = streamDevSwitch(streamDevDisconnect, streamDevConnect);
    if (!status) {
        mActiveStreamMutex.lock();
        for (sIter = prevActiveStreams.begin(); sIter != prevActiveStreams.end(); sIter++) {
            if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
                (*sIter)->lockStreamMutex();
                if (ResourceManager::isDummyDevEnabled) {
                    (*sIter)->removePalDevice(*sIter, inDev->getSndDeviceId());
                } else {
                    (*sIter)->clearOutPalDevices(*sIter);
                }
                (*sIter)->addPalDevice(*sIter, newDevAttr);
                (*sIter)->unlockStreamMutex();
            }
        }
        mActiveStreamMutex.unlock();
    } else {
        PAL_ERR(LOG_TAG, "forceDeviceSwitch failed %d", status);
    }

    return 0;
}

const std::string ResourceManager::getPALDeviceName(const pal_device_id_t id) const
{
    PAL_DBG(LOG_TAG, "id %d", id);
    if (isValidDevId(id)) {
        return deviceNameLUT.at(id);
    } else {
        PAL_ERR(LOG_TAG, "Invalid device id %d", id);
        return std::string("");
    }
}

int ResourceManager::getBackendName(int deviceId, std::string &backendName)
{
    if (isValidDevId(deviceId) && (deviceId != PAL_DEVICE_NONE)) {
        backendName.assign(listAllBackEndIds[deviceId].second);
    } else {
        PAL_ERR(LOG_TAG, "Invalid device id %d", deviceId);
        return -EINVAL;
    }
    return 0;
}

void ResourceManager::updateVirtualBackendName()
{
    std::string PrevBackendName;
    pal_device_id_t virtual_dev[] = {PAL_DEVICE_OUT_ULTRASOUND, PAL_DEVICE_OUT_SPEAKER, PAL_DEVICE_OUT_HANDSET};

    if (getBackendName(PAL_DEVICE_OUT_HANDSET, PrevBackendName) != 0) {
        PAL_ERR(LOG_TAG, "Error retrieving BE name");
        return;
    }

    for (int i = 0; i < sizeof(virtual_dev) / sizeof(virtual_dev[0]); i++) {
        std::string backendName(PrevBackendName);

        switch(virtual_dev[i]) {
            case PAL_DEVICE_OUT_ULTRASOUND:
                backendName.append("-VIRT-1");
                break;
            case PAL_DEVICE_OUT_SPEAKER:
            case PAL_DEVICE_OUT_HANDSET:
                backendName.append("-VIRT-0");
                break;
            default:
                break;
        }

        listAllBackEndIds[virtual_dev[i]].second.assign(backendName);
    }
}

void ResourceManager::updateVirtualBESndName()
{
    std::map<group_dev_config_idx_t, std::shared_ptr<group_dev_config_t>>::iterator it;
    std::shared_ptr<group_dev_config_t> group_device_config = NULL;
    group_dev_config_idx_t grp_dev_cfgs[] = {GRP_UPD_RX_HANDSET, GRP_UPD_RX_SPEAKER};

    if (!isVbatEnabled)
        return;

    for (int i = 0; i < sizeof(grp_dev_cfgs) / sizeof(grp_dev_cfgs[0]); i++) {
        it = groupDevConfigMap.find(grp_dev_cfgs[i]);
        if (it == groupDevConfigMap.end())
            continue;

        group_device_config = it->second;
        if (group_device_config) {
            if (!strcmp(group_device_config->snd_dev_name.c_str(), "speaker"))
                group_device_config->snd_dev_name.assign("speaker-vbat");
        }
    }
}

bool ResourceManager::isValidDevId(int deviceId)
{
    if (((deviceId >= PAL_DEVICE_NONE) && (deviceId < PAL_DEVICE_OUT_MAX))
        || ((deviceId > PAL_DEVICE_IN_MIN) && (deviceId < PAL_DEVICE_IN_MAX)))
        return true;

    return false;
}

bool ResourceManager::isValidStreamId(int streamId)
{
    if (streamId < PAL_STREAM_LOW_LATENCY || streamId >= PAL_STREAM_MAX)
        return false;

    return true;
}

bool ResourceManager::isOutputDevId(int deviceId)
{
    if ((deviceId > PAL_DEVICE_NONE) && (deviceId < PAL_DEVICE_OUT_MAX))
        return true;

    return false;
}

bool ResourceManager::isInputDevId(int deviceId)
{
    if ((deviceId > PAL_DEVICE_IN_MIN) && (deviceId < PAL_DEVICE_IN_MAX))
        return true;

    return false;
}

bool ResourceManager::matchDevDir(int devId1, int devId2)
{
    if (isOutputDevId(devId1) && isOutputDevId(devId2))
        return true;
    if (isInputDevId(devId1) && isInputDevId(devId2))
        return true;

    return false;
}

bool ResourceManager::isNonALSACodec(const struct pal_device * /*device*/) const
{

    //return false on our target, move configuration to xml

    return false;
}

bool ResourceManager::ifVoiceorVoipCall (const pal_stream_type_t streamType) const {

   bool voiceOrVoipCall = false;

   switch (streamType) {
       case PAL_STREAM_VOIP:
       case PAL_STREAM_VOIP_RX:
       case PAL_STREAM_VOIP_TX:
       case PAL_STREAM_VOICE_CALL:
           voiceOrVoipCall = true;
           break;
       default:
           voiceOrVoipCall = false;
           break;
    }

    return voiceOrVoipCall;
}

int ResourceManager::getCallPriority(bool ifVoiceCall) const {

//TBD: replace this with XML based priorities
    if (ifVoiceCall) {
        return 100;
    } else {
        return 0;
    }
}

int ResourceManager::getStreamAttrPriority (const pal_stream_attributes* sAttr) const {
    int priority = 0;

    if (!sAttr)
        goto exit;


    priority = getCallPriority(ifVoiceorVoipCall(sAttr->type));


    //44.1 or multiple or 24 bit

    if ((sAttr->in_media_config.sample_rate % 44100) == 0) {
        priority += 50;
    }

    if (sAttr->in_media_config.bit_width == 24) {
        priority += 25;
    }

exit:
    return priority;
}

int ResourceManager::getNativeAudioSupport()
{
    int ret = NATIVE_AUDIO_MODE_INVALID;
    if (na_props.rm_na_prop_enabled &&
        na_props.ui_na_prop_enabled) {
        ret = na_props.na_mode;
    }
    PAL_ERR(LOG_TAG,"napb: ui Prop enabled(%d) mode(%d)",
           na_props.ui_na_prop_enabled, na_props.na_mode);
    return ret;
}

int ResourceManager::setNativeAudioSupport(int na_mode)
{
    if (NATIVE_AUDIO_MODE_SRC == na_mode || NATIVE_AUDIO_MODE_TRUE_44_1 == na_mode
        || NATIVE_AUDIO_MODE_MULTIPLE_MIX_IN_CODEC == na_mode
        || NATIVE_AUDIO_MODE_MULTIPLE_MIX_IN_DSP == na_mode) {
        na_props.rm_na_prop_enabled = na_props.ui_na_prop_enabled = true;
        na_props.na_mode = na_mode;
        PAL_DBG(LOG_TAG,"napb: native audio playback enabled in (%s) mode",
              ((na_mode == NATIVE_AUDIO_MODE_SRC)?"SRC":
               (na_mode == NATIVE_AUDIO_MODE_TRUE_44_1)?"True":
               (na_mode == NATIVE_AUDIO_MODE_MULTIPLE_MIX_IN_CODEC)?"Multiple_Mix_Codec":"Multiple_Mix_DSP"));
    }
    else {
        na_props.rm_na_prop_enabled = false;
        na_props.na_mode = NATIVE_AUDIO_MODE_INVALID;
        PAL_VERBOSE(LOG_TAG,"napb: native audio playback disabled");
    }

    return 0;
}

void ResourceManager::getNativeAudioParams(struct str_parms *query,
                             struct str_parms *reply,
                             char *value, int len)
{
    int ret;
    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_NATIVE_AUDIO,
                            value, len);
    if (ret >= 0) {
        if (na_props.rm_na_prop_enabled) {
            str_parms_add_str(reply, AUDIO_PARAMETER_KEY_NATIVE_AUDIO,
                          na_props.ui_na_prop_enabled ? "true" : "false");
            PAL_VERBOSE(LOG_TAG,"napb: na_props.ui_na_prop_enabled: %d",
                  na_props.ui_na_prop_enabled);
        } else {
            str_parms_add_str(reply, AUDIO_PARAMETER_KEY_NATIVE_AUDIO,
                              "false");
            PAL_VERBOSE(LOG_TAG,"napb: native audio not supported: %d",
                  na_props.rm_na_prop_enabled);
        }
    }
}

int ResourceManager::setConfigParams(struct str_parms *parms)
{
    char *value=NULL;
    int len;
    int ret = 0;
    char *kv_pairs = str_parms_to_str(parms);

    PAL_DBG(LOG_TAG,"Enter: %s", kv_pairs);
    if(kv_pairs == NULL) {
        ret = -ENOMEM;
        PAL_ERR(LOG_TAG," key-value pair is NULL");
        goto exit;
    }

    len = strlen(kv_pairs);
    value = (char*)calloc(len, sizeof(char));
    if(value == NULL) {
        ret = -ENOMEM;
        PAL_ERR(LOG_TAG,"failed to allocate memory");
        goto exit;
    }
    ret = setNativeAudioParams(parms, value, len);

    ret = setLoggingLevelParams(parms, value, len);

    ret = setContextManagerEnableParam(parms, value, len);

    ret = setUpdDedicatedBeEnableParam(parms, value, len);
    ret = setDualMonoEnableParam(parms, value, len);
    ret = setSignalHandlerEnableParam(parms, value, len);
    ret = setMuxconfigEnableParam(parms, value, len);
    ret = setUpdDutyCycleEnableParam(parms, value, len);
    ret = setUpdVirtualPortParam(parms, value, len);
    setConnectivityProxyEnableParam(parms, value, len);
    setDummyDevEnableParam(parms, value, len);

    ret = setHapticsPriorityParam(parms, value, len);
    ret = setHapticsDrivenParam(parms, value, len);

    /* Not checking return value as this is optional */
    setLpiLoggingParams(parms, value, len);

exit:
    PAL_DBG(LOG_TAG,"Exit, status %d", ret);
    if(value != NULL)
        free(value);
    return ret;
}

int ResourceManager::setLpiLoggingParams(struct str_parms *parms,
                                          char *value, int len)
{
    int ret = -EINVAL;

    if (!value || !parms)
        return ret;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_LPI_LOGGING,
                                value, len);
    if (ret >= 0) {
        if (value && !strncmp(value, "true", sizeof("true")))
            lpi_logging_ =  true;
        PAL_INFO(LOG_TAG, "LPI logging is set to %d", lpi_logging_);
        ret = 0;
    }
    return ret;
}

int ResourceManager::setLoggingLevelParams(struct str_parms *parms,
                                          char *value, int len)
{
    int ret = -EINVAL;

    if (!value || !parms)
        return ret;

#ifdef SEC_AUDIO_COMMON
    char debug_level[PROPERTY_VALUE_MAX] = {'\0'};
    ret = property_get(LOG_DEBUG_LEVEL_PROP, debug_level, "");

    if (!strncmp(LOG_DEBUG_LEVEL_LOW, debug_level, sizeof(LOG_DEBUG_LEVEL_LOW))) {
        pal_log_lvl = PAL_LOG_LEVEL_LOW;
        PAL_INFO(LOG_TAG, "pal logging level is set to 0x%x for debug level low",
                pal_log_lvl);
        ret = 0;
    } else {
        ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_LOG_LEVEL,
                                    value, len);
        if (ret >= 0) {
            pal_log_lvl = std::stoi(value,0,16);
#ifdef SEC_AUDIO_DUMP
            if (pal_log_lvl < PAL_LOG_LEVEL_DEFAULT) {
                pal_log_lvl = PAL_LOG_LEVEL_DEFAULT;
            }
#endif
            ret = 0;
            PAL_INFO(LOG_TAG, "pal logging level is set to 0x%x",
                    pal_log_lvl);
        }
    }
#else
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_LOG_LEVEL,
                                value, len);
    if (ret >= 0) {
        pal_log_lvl = std::stoi(value,0,16);
        PAL_VERBOSE(LOG_TAG, "pal logging level is set to 0x%x",
                 pal_log_lvl);
        ret = 0;
    }
#endif
    return ret;
}

int ResourceManager::setContextManagerEnableParam(struct str_parms *parms,
                                          char *value, int len)
{
    int ret = -EINVAL;

    if (!value || !parms)
        return ret;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_CONTEXT_MANAGER_ENABLE,
                            value, len);
    PAL_VERBOSE(LOG_TAG," value %s", value);

    if (ret >= 0) {
        if (value && !strncmp(value, "true", sizeof("true")))
            isContextManagerEnabled = true;

        str_parms_del(parms, AUDIO_PARAMETER_KEY_CONTEXT_MANAGER_ENABLE);
    }

    return ret;
}

int ResourceManager::setUpdDedicatedBeEnableParam(struct str_parms *parms,
                                 char *value, int len)
{
    int ret = -EINVAL;

    if (!value || !parms)
        return ret;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_UPD_DEDICATED_BE,
                            value, len);
    PAL_VERBOSE(LOG_TAG," value %s", value);

    if (ret >= 0) {
        if (value && !strncmp(value, "true", sizeof("true")))
            ResourceManager::isUpdDedicatedBeEnabled = true;

        str_parms_del(parms, AUDIO_PARAMETER_KEY_UPD_DEDICATED_BE);
    }

    return ret;

}

int ResourceManager::setHapticsPriorityParam(struct str_parms *parms,
                                 char *value, int len)
{
    int ret = -EINVAL;

    if (!value || !parms)
        return ret;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_HAPTICS_PRIORITY,
                            value, len);
    PAL_VERBOSE(LOG_TAG," value %s", value);

    if (ret >= 0) {
        if (value && !strncmp(value, "touch_haptics", sizeof("touch_haptics"))) {
            haptics_priority = HAPTICS_MODE_TOUCH;
        } else if (value && !strncmp(value, "ringtone_haptics", sizeof("ringtone_haptics"))) {
            haptics_priority = HAPTICS_MODE_RINGTONE;
        } else {
            haptics_priority = HAPTICS_MODE_INVALID;
        }
        str_parms_del(parms, AUDIO_PARAMETER_KEY_HAPTICS_PRIORITY);
    }

    return ret;
}

int ResourceManager::setHapticsDrivenParam(struct str_parms *parms,
                                              char *value, int len)
{
    int ret = -EINVAL;

    if (!value || !parms)
        return ret;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_WSA_HAPTICS,
                                                     value, len);
    PAL_VERBOSE(LOG_TAG," value %s", value);

    if (ret >= 0) {
        if (value && !strncmp(value, "true", sizeof("true")))
            ResourceManager::isHapticsthroughWSA = true;

        str_parms_del(parms, AUDIO_PARAMETER_KEY_WSA_HAPTICS);
    }

    return ret;
}

int ResourceManager::setMuxconfigEnableParam(struct str_parms *parms,
                                 char *value, int len)
{
    int ret = -EINVAL;

    if (!value || !parms)
        return ret;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_DEVICE_MUX,
                                value, len);
    PAL_VERBOSE(LOG_TAG," value %s", value);

    if (ret >= 0) {
        if (value && !strncmp(value, "true", sizeof("true")))
            isDeviceMuxConfigEnabled = true;

        str_parms_del(parms, AUDIO_PARAMETER_KEY_DEVICE_MUX);
    }

    return ret;
}

int ResourceManager::setUpdDutyCycleEnableParam(struct str_parms *parms,
                                 char *value, int len)
{
    int ret = -EINVAL;

    if (!value || !parms)
        return ret;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_UPD_DUTY_CYCLE,
                            value, len);
    PAL_VERBOSE(LOG_TAG," value %s", value);

    if (ret >= 0) {
        if (value && !strncmp(value, "true", sizeof("true")))
            ResourceManager::isUpdDutyCycleEnabled = true;

        str_parms_del(parms, AUDIO_PARAMETER_KEY_UPD_DUTY_CYCLE);
    }

    return ret;
}

int ResourceManager::setUpdVirtualPortParam(struct str_parms *parms, char *value, int len)
{
    int ret = -EINVAL;

    if (!value || !parms)
        return ret;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_UPD_VIRTUAL_PORT,
                            value, len);

    PAL_VERBOSE(LOG_TAG," value %s", value);

    if (ret >= 0) {
        if (value && !strncmp(value, "true", sizeof("true")))
            ResourceManager::isUPDVirtualPortEnabled = true;

        str_parms_del(parms, AUDIO_PARAMETER_KEY_UPD_VIRTUAL_PORT);
    }

    return ret;
}

void ResourceManager::setConnectivityProxyEnableParam(struct str_parms *parms, char *value, int len)
{
    int ret = -EINVAL;

    if (!value || !parms)
        return;

    ret = str_parms_get_str(parms, "connectivity_proxy_enabled",
                            value, len);
    PAL_VERBOSE(LOG_TAG," value %s", value);

    if (ret >= 0) {
        if (value && !strncmp(value, "true", sizeof("true")))
            ResourceManager::isCPEnabled = true;

        str_parms_del(parms, "connectivity_proxy_enabled");
    }
}

void ResourceManager::setDummyDevEnableParam(struct str_parms *parms, char *value, int len)
{
    int ret = -EINVAL;

    if (!value || !parms)
        return;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_DUMMY_DEV_ENABLE,
                            value, len);

    if (ret >= 0) {
        PAL_VERBOSE(LOG_TAG," value %s", value);

        if (value && !strncmp(value, "true", sizeof("true")))
            ResourceManager::isDummyDevEnabled = true;

        str_parms_del(parms, AUDIO_PARAMETER_KEY_DUMMY_DEV_ENABLE);
    }
}


int ResourceManager::setDualMonoEnableParam(struct str_parms *parms,
                                 char *value, int len)
{
    int ret = -EINVAL;

    if (!value || !parms)
        return ret;

    PAL_VERBOSE(LOG_TAG, "dual mono enabled was=%x", isDualMonoEnabled);
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_DUAL_MONO,
                                value, len);
    PAL_VERBOSE(LOG_TAG," value %s", value);
    if (ret >= 0) {
        if (value && !strncmp(value, "true", sizeof("true")))
            isDualMonoEnabled= true;

        str_parms_del(parms, AUDIO_PARAMETER_KEY_DUAL_MONO);
    }

    PAL_VERBOSE(LOG_TAG, "dual mono enabled is=%x", isDualMonoEnabled);

    return ret;
}

int ResourceManager::setSignalHandlerEnableParam(struct str_parms *parms,
                                 char *value, int len)
{
    int ret = -EINVAL;

    if (!value || !parms)
        return ret;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SIGNAL_HANDLER,
                                value, len);
    PAL_VERBOSE(LOG_TAG," value %s", value);
    if (ret >= 0) {
        if (value && !strncmp(value, "true", sizeof("true")))
            isSignalHandlerEnabled = true;

        str_parms_del(parms, AUDIO_PARAMETER_KEY_SIGNAL_HANDLER);
    }

    PAL_VERBOSE(LOG_TAG, "Signal handler enabled is=%x", isSignalHandlerEnabled);

    return ret;
}

int ResourceManager::setNativeAudioParams(struct str_parms *parms,
                                          char *value, int len)
{
    int ret = -EINVAL;
    int mode = NATIVE_AUDIO_MODE_INVALID;

    if (!value || !parms)
        return ret;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_MAX_SESSIONS,
                                value, len);
    if (ret >= 0) {
        max_session_num = std::stoi(value);
        PAL_VERBOSE(LOG_TAG, "Max sessions supported for each stream type are %d",
                 max_session_num);

    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_MAX_NT_SESSIONS,
                                value, len);
    if (ret >= 0) {
        max_nt_sessions = std::stoi(value);
        PAL_VERBOSE(LOG_TAG, "Max sessions supported for NON_TUNNEL stream type are %d",
                 max_nt_sessions);

    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_NATIVE_AUDIO_MODE,
                             value, len);
    PAL_VERBOSE(LOG_TAG," value %s", value);
    if (ret >= 0) {
        if (value && !strncmp(value, "src", sizeof("src")))
            mode = NATIVE_AUDIO_MODE_SRC;
        else if (value && !strncmp(value, "true", sizeof("true")))
            mode = NATIVE_AUDIO_MODE_TRUE_44_1;
        else if (value && !strncmp(value, "multiple_mix_codec", sizeof("multiple_mix_codec")))
            mode = NATIVE_AUDIO_MODE_MULTIPLE_MIX_IN_CODEC;
        else if (value && !strncmp(value, "multiple_mix_dsp", sizeof("multiple_mix_dsp")))
            mode = NATIVE_AUDIO_MODE_MULTIPLE_MIX_IN_DSP;
        else {
            mode = NATIVE_AUDIO_MODE_INVALID;
            PAL_ERR(LOG_TAG,"napb:native_audio_mode in RM xml,invalid mode(%s) string", value);
        }
        PAL_VERBOSE(LOG_TAG,"napb: updating mode (%d) from XML", mode);
        setNativeAudioSupport(mode);
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_HIFI_FILTER,
                             value, len);
    PAL_VERBOSE(LOG_TAG," value %s", value);
    if (ret >= 0) {
        if (!strncmp("true", value, sizeof("true"))) {
            isHifiFilterEnabled = true;
            PAL_VERBOSE(LOG_TAG,"HIFI filter enabled from XML");
        }
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_NATIVE_AUDIO,
                             value, len);
    PAL_VERBOSE(LOG_TAG," value %s", value);
    if (ret >= 0) {
        if (na_props.rm_na_prop_enabled) {
            if (!strncmp("true", value, sizeof("true"))) {
                na_props.ui_na_prop_enabled = true;
                PAL_VERBOSE(LOG_TAG,"napb: native audio feature enabled from UI");
            } else {
                na_props.ui_na_prop_enabled = false;
                PAL_VERBOSE(LOG_TAG,"napb: native audio feature disabled from UI");
            }

            str_parms_del(parms, AUDIO_PARAMETER_KEY_NATIVE_AUDIO);
            //TO-DO
            // Update the concurrencies
        } else {
              PAL_VERBOSE(LOG_TAG,"napb: native audio cannot be enabled from UI");
        }
    }
    return ret;
}
void ResourceManager::updatePcmId(int32_t deviceId, int32_t pcmId)
{
    if (isValidDevId(deviceId)) {
        devicePcmId[deviceId].second = pcmId;
    } else {
        PAL_ERR(LOG_TAG, "Invalid device id %d", deviceId);
    }
}

void ResourceManager::updateLinkName(int32_t deviceId, std::string linkName)
{
    if (isValidDevId(deviceId)) {
        deviceLinkName[deviceId].second = linkName;
    } else {
        PAL_ERR(LOG_TAG, "Invalid device id %d", deviceId);
    }
}

void ResourceManager::updateSndName(int32_t deviceId, std::string sndName)
{
    if (isValidDevId(deviceId)) {
        sndDeviceNameLUT[deviceId].second = sndName;
        PAL_DBG(LOG_TAG, "Updated snd device to %s for device %s",
                sndName.c_str(), deviceNameLUT.at(deviceId).c_str());
    } else {
        PAL_ERR(LOG_TAG, "Invalid device id %d", deviceId);
    }
}

void ResourceManager::updateBackEndName(int32_t deviceId, std::string backEndName)
{
    if (isValidDevId(deviceId) && deviceId < listAllBackEndIds.size()) {
        listAllBackEndIds[deviceId].second = backEndName;
    } else {
        PAL_ERR(LOG_TAG, "Invalid device id %d", deviceId);
    }
}

int ResourceManager::convertCharToHex(std::string num)
{
    uint64_t hexNum = 0;
    uint32_t base = 1;
    const char * charNum = num.c_str();
    int32_t len = strlen(charNum);
    for (int i = len-1; i>=2; i--) {
        if (charNum[i] >= '0' && charNum[i] <= '9') {
            hexNum += (charNum[i] - 48) * base;
            base = base << 4;
        } else if (charNum[i] >= 'A' && charNum[i] <= 'F') {
            hexNum += (charNum[i] - 55) * base;
            base = base << 4;
        } else if (charNum[i] >= 'a' && charNum[i] <= 'f') {
            hexNum += (charNum[i] - 87) * base;
            base = base << 4;
        }
    }
    return (int32_t) hexNum;
}

int32_t ResourceManager::a2dpReconfig()
{
    int status = 0;
    uint32_t latencyMs = 0, maxLatencyMs = 0;
    std::shared_ptr<Device> a2dpDev = nullptr;
    struct pal_device a2dpDattr;
    std::vector <Stream*> activeA2dpStreams;
    std::vector <Stream*> activeStreams;
    std::vector <Stream*>::iterator sIter;
    struct pal_volume_data* volume = NULL;

    PAL_DBG(LOG_TAG, "enter");
    volume = (struct pal_volume_data*)calloc(1, (sizeof(uint32_t) +
        (sizeof(struct pal_channel_vol_kv) * (0xFFFF))));
    if (!volume) {
        status = -ENOMEM;
        goto exit;
    }

    mActiveStreamMutex.lock();

    a2dpDattr.id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
    a2dpDev = Device::getInstance(&a2dpDattr, rm);
    if (!a2dpDev) {
        PAL_ERR(LOG_TAG, "Getting a2dp/ble device instance failed");
        mActiveStreamMutex.unlock();
        goto exit;
    }

    status = a2dpDev->getDeviceAttributes(&a2dpDattr);
    if (status) {
        PAL_ERR(LOG_TAG, "Switch DevAttributes Query Failed");
        mActiveStreamMutex.unlock();
        goto exit;
    }

    getActiveStream_l(activeA2dpStreams, a2dpDev);
    if (activeA2dpStreams.size() == 0) {
        PAL_DBG(LOG_TAG, "no active streams found on a2dp device");
        mActiveStreamMutex.unlock();
        goto exit;
    }
    for (sIter = activeA2dpStreams.begin(); sIter != activeA2dpStreams.end(); sIter++) {
        if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
            (*sIter)->lockStreamMutex();
            if (!((*sIter)->a2dpMuted)) {
                struct pal_stream_attributes sAttr;
                (*sIter)->getStreamAttributes(&sAttr);
                if (((sAttr.type == PAL_STREAM_COMPRESSED) ||
                    (sAttr.type == PAL_STREAM_PCM_OFFLOAD))) {
                    /* First mute & then pause
                     * This is to ensure DSP has enough ramp down period in volume module.
                     * If pause is issued firstly, then there's no enough data for processing.
                     * As a result, ramp down will not happen and will only occur after resume,
                     * which is perceived as audio leakage.
                     */
                    (*sIter)->mute_l(true);
                    (*sIter)->a2dpMuted = true;
                    // Pause only if the stream is not explicitly paused.
                    // In some scenarios, stream might have already paused prior to a2dpsuspend.
                    if (((*sIter)->isPaused) == false) {
                        (*sIter)->pause_l();
                        (*sIter)->a2dpPaused = true;
                    }
                } else {
                    latencyMs = (*sIter)->getLatency();
                    if (maxLatencyMs < latencyMs)
                        maxLatencyMs = latencyMs;
                    // Mute
                    (*sIter)->mute_l(true);
                    (*sIter)->a2dpMuted = true;
                }
            }
            (*sIter)->unlockStreamMutex();
        }
    }

    mActiveStreamMutex.unlock();

    // wait for stale pcm drained before switching to speaker
    if (maxLatencyMs > 0) {
        // multiplication factor applied to latency when calculating a safe mute delay
        // TODO: It's not needed if latency is accurate.
        const int latencyMuteFactor = 2;
        usleep(maxLatencyMs * 1000 * latencyMuteFactor);
    }

    forceDeviceSwitch(a2dpDev, &a2dpDattr);

    mActiveStreamMutex.lock();
    for (sIter = activeA2dpStreams.begin(); sIter != activeA2dpStreams.end(); sIter++) {
        if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
            (*sIter)->lockStreamMutex();
            struct pal_stream_attributes sAttr;
            (*sIter)->getStreamAttributes(&sAttr);
            if (((sAttr.type == PAL_STREAM_COMPRESSED) ||
                (sAttr.type == PAL_STREAM_PCM_OFFLOAD)) &&
                (!(*sIter)->isActive())) {
                /* Resume only when it was paused during a2dpSuspend.
                 * This is to avoid resuming during regular pause.
                 */
                if (((*sIter)->a2dpPaused) == true) {
                    (*sIter)->resume_l();
                    (*sIter)->a2dpPaused = false;
                }
            }
            status = (*sIter)->getVolumeData(volume);
            if (status) {
                PAL_ERR(LOG_TAG, "getVolumeData failed %d", status);
                (*sIter)->unlockStreamMutex();
                continue;
            }
            /* set a2dpMuted to false so that volume can be applied
             * volume gets cached if a2dpMuted is set to true
             */
            (*sIter)->a2dpMuted = false;
            status = (*sIter)->setVolume(volume);
            if (status) {
                PAL_ERR(LOG_TAG, "setVolume failed %d", status);
                (*sIter)->a2dpMuted = true;
                (*sIter)->unlockStreamMutex();
                continue;
            }
            // set a2dpMuted to true in case unmute failed
            if ((*sIter)->mute_l(false))
                (*sIter)->a2dpMuted = true;
            (*sIter)->unlockStreamMutex();
        }
    }
    mActiveStreamMutex.unlock();

exit:
    PAL_DBG(LOG_TAG, "exit status: %d", status);
    if (volume) {
        free(volume);
    }
    return status;
}

int32_t ResourceManager::a2dpSuspendToDummy(pal_device_id_t dev_id)
{
    int status = 0;
    std::shared_ptr<Device> a2dpDev = nullptr;
    struct pal_device a2dpDattr = {};
    std::shared_ptr<Device> switchDev = nullptr;
    struct pal_device switchDevDattr = {};
    std::vector <Stream*> activeA2dpStreams;
    std::vector <Stream*>::iterator sIter;
    std::vector <std::shared_ptr<Device>> associatedDevices;
    std::vector <std::tuple<Stream*, uint32_t>> streamDevDisconnect;
    std::vector <std::tuple<Stream*, struct pal_device *>> streamDevConnect;

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_INFO(LOG_TAG, "enter. (device : %d)", dev_id);
#else
    PAL_DBG(LOG_TAG, "enter");
#endif

    a2dpDattr.id = dev_id;
    a2dpDev = Device::getInstance(&a2dpDattr, rm);
    if (!a2dpDev) {
        PAL_ERR(LOG_TAG, "getting a2dp/ble device instance failed");
        goto exit;
    }

    switchDevDattr.id = PAL_DEVICE_OUT_DUMMY;
    status = getDeviceConfig(&switchDevDattr, NULL);
    if (status) {
        PAL_ERR(LOG_TAG, "getDeviceConfig failed for out_dummy device");
        goto exit;
    }

    mActiveStreamMutex.lock();
    getActiveStream_l(activeA2dpStreams, a2dpDev);
    if (activeA2dpStreams.size() == 0) {
        PAL_DBG(LOG_TAG, "no active streams found");
        mActiveStreamMutex.unlock();
        goto exit;
    }

    for (sIter = activeA2dpStreams.begin(); sIter != activeA2dpStreams.end(); sIter++) {
        if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
            (*sIter)->lockStreamMutex();
            associatedDevices.clear();
            status = (*sIter)->getAssociatedOutDevices(associatedDevices);
            if ((0 != status) ||
                !(rm->isDeviceAvailable(associatedDevices, a2dpDattr.id))) {
                PAL_ERR(LOG_TAG, "error: stream %pK is not associated with a2dp/ble device", *sIter);
                (*sIter)->unlockStreamMutex();
                continue;
            }
            (*sIter)->suspendedOutDevIds.clear();
            (*sIter)->suspendedOutDevIds.push_back(a2dpDattr.id);
            streamDevDisconnect.push_back({*sIter, a2dpDattr.id});
            if (associatedDevices.size() == 1) {
                // perform mute for non combo use-case, if it is not already muted
                if (!((*sIter)->a2dpMuted) && !((*sIter)->mute_l(true))) {
                    (*sIter)->a2dpMuted = true;
                }
                streamDevConnect.push_back({*sIter, &switchDevDattr});
            } else {
                (*sIter)->suspendedOutDevIds.push_back(switchDevDattr.id);
            }
            (*sIter)->unlockStreamMutex();
        }
    }
    mActiveStreamMutex.unlock();

    PAL_INFO(LOG_TAG, "switching a2dp/ble stream to out_dummy device"); //SEC_AUDIO_ADD_FOR_DEBUG : PAL_DBG -> PAL_INFO
    status = streamDevSwitch(streamDevDisconnect, streamDevConnect);

    if (status) {
        PAL_ERR(LOG_TAG, " streamDevSwitch failed %d", status);
        goto exit;
    }

    mActiveStreamMutex.lock();
    for (sIter = activeA2dpStreams.begin(); sIter != activeA2dpStreams.end(); sIter++) {
        if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
            (*sIter)->lockStreamMutex();
            (*sIter)->removePalDevice(*sIter, a2dpDattr.id);
            if ((*sIter)->suspendedOutDevIds.size() == 1) {
                (*sIter)->addPalDevice(*sIter, &switchDevDattr);
            }
            (*sIter)->unlockStreamMutex();
        }
    }
    mActiveStreamMutex.unlock();

exit:
    PAL_INFO(LOG_TAG, "exit status: %d", status); //SEC_AUDIO_ADD_FOR_DEBUG : PAL_DBG -> PAL_INFO
    return status;
}

int32_t ResourceManager::a2dpResumeFromDummy(pal_device_id_t dev_id)
{
    int status = 0;
    std::shared_ptr<Device> activeDev = nullptr;
    struct pal_device activeDattr = {};
    struct pal_device a2dpDattr = {};
    struct pal_volume_data *volume = NULL;
    std::vector <Stream*>::iterator sIter;
    std::vector <Stream*> activeStreams;
    std::vector <Stream*> orphanStreams;
    std::vector <Stream*> retryStreams;
    std::vector <Stream*> restoredStreams;
    std::vector <std::tuple<Stream*, uint32_t>> streamDevDisconnect;
    std::vector <std::tuple<Stream*, struct pal_device *>> streamDevConnect;

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_INFO(LOG_TAG, "enter. (device : %d)", dev_id);
#else
    PAL_DBG(LOG_TAG, "enter");
#endif

    volume = (struct pal_volume_data *)calloc(1, (sizeof(uint32_t) +
                     (sizeof(struct pal_channel_vol_kv) * (0xFFFF))));
    if (!volume) {
        status = -ENOMEM;
        goto exit;
    }

    activeDattr.id = PAL_DEVICE_OUT_DUMMY;
    activeDev = Device::getInstance(&activeDattr, rm);
    if (!activeDev) {
        PAL_ERR(LOG_TAG, "getting out_dummy device instance failed");
        goto exit;
    }

    a2dpDattr.id = dev_id;
    status = getDeviceConfig(&a2dpDattr, NULL);
    if (status) {
        PAL_ERR(LOG_TAG, "getDeviceConfig failed for a2dp/ble device");
        goto exit;
    }

    mActiveStreamMutex.lock();
    getActiveStream_l(activeStreams, activeDev);
    getOrphanStream_l(orphanStreams, retryStreams);
    if (activeStreams.empty() && orphanStreams.empty() && retryStreams.empty()) {
        PAL_DBG(LOG_TAG, "no active streams found");
        mActiveStreamMutex.unlock();
        goto exit;
    }

    /* Check all active streams associated with out_dummy device.
     * Disconnect out_dummy device and connect BT device for active streams.
     */
    for (sIter = activeStreams.begin(); sIter != activeStreams.end(); sIter++) {
        (*sIter)->lockStreamMutex();
        if (std::find((*sIter)->suspendedOutDevIds.begin(),
                (*sIter)->suspendedOutDevIds.end(), a2dpDattr.id)
                != (*sIter)->suspendedOutDevIds.end()) {
            restoredStreams.push_back(*sIter);
            streamDevDisconnect.push_back({*sIter, activeDattr.id});
            streamDevConnect.push_back({*sIter, &a2dpDattr});
        }
        (*sIter)->unlockStreamMutex();
    }

    // Retry all orphan streams which failed to restore previously.
    for (sIter = orphanStreams.begin(); sIter != orphanStreams.end(); sIter++) {
        (*sIter)->lockStreamMutex();
        if (std::find((*sIter)->suspendedOutDevIds.begin(),
                (*sIter)->suspendedOutDevIds.end(), a2dpDattr.id)
                != (*sIter)->suspendedOutDevIds.end()) {
            restoredStreams.push_back(*sIter);
            streamDevConnect.push_back({*sIter, &a2dpDattr});
        }
        (*sIter)->unlockStreamMutex();
    }

    /* Restore combo streams as well as streams which failed to switch
     * to desired device previously.
     */
    for (sIter = retryStreams.begin(); sIter != retryStreams.end(); sIter++) {
        (*sIter)->lockStreamMutex();
        if (std::find((*sIter)->suspendedOutDevIds.begin(),
                (*sIter)->suspendedOutDevIds.end(), a2dpDattr.id)
                != (*sIter)->suspendedOutDevIds.end()) {
            std::vector<std::shared_ptr<Device>> devices;
            (*sIter)->getAssociatedOutDevices(devices);
            if ((devices.size() > 0) &&
                ((*sIter)->suspendedOutDevIds.size() == 1 /* non combo */)) {
                for (auto device: devices) {
                    streamDevDisconnect.push_back({*sIter, device->getSndDeviceId()});
                }
            }
            restoredStreams.push_back(*sIter);
            streamDevConnect.push_back({*sIter, &a2dpDattr});
        } else if (std::find((*sIter)->suspendedOutDevIds.begin(),
                (*sIter)->suspendedOutDevIds.end(), PAL_DEVICE_OUT_BLUETOOTH_SCO)
                != (*sIter)->suspendedOutDevIds.end()) {
            std::vector<std::shared_ptr<Device>> devices;
            (*sIter)->getAssociatedOutDevices(devices);
            if (devices.size() > 0) {
                for (auto device: devices) {
                    if (device->getSndDeviceId() == PAL_DEVICE_OUT_BLUETOOTH_SCO) {
                        streamDevDisconnect.push_back({(*sIter), PAL_DEVICE_OUT_BLUETOOTH_SCO});
                        break;
                    }
                }
            }
            restoredStreams.push_back((*sIter));
            streamDevConnect.push_back({(*sIter), &a2dpDattr});
        }
        (*sIter)->unlockStreamMutex();
    }

    if (restoredStreams.empty()) {
        PAL_DBG(LOG_TAG, "no streams to be restored");
        mActiveStreamMutex.unlock();
        goto exit;
    }
    SortAndUnique(restoredStreams);
    mActiveStreamMutex.unlock();

    /* In case of SSR down event if a2dpSuspended = false sets, since sound card state is offline
     * StreamDevSwitch() operation will be skipped. Due to this mDevices will remain as speaker
     * and when SSR is up, active streams will route to the speaker only.
     * Thus in this corner scenario, update the mDevices as BT A2DP so that when SSR is up
     * active streams will be routed to BT properly.
     */
    if (PAL_CARD_STATUS_DOWN(cardState)) {
        PAL_ERR(LOG_TAG, "Sound card offline");
        mActiveStreamMutex.lock();
        for (sIter = restoredStreams.begin(); sIter != restoredStreams.end(); sIter++) {
            if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
                (*sIter)->lockStreamMutex();
                if (std::find((*sIter)->suspendedOutDevIds.begin(),
                        (*sIter)->suspendedOutDevIds.end(), a2dpDattr.id)
                        != (*sIter)->suspendedOutDevIds.end()) {
                    if ((*sIter)->suspendedOutDevIds.size() == 1 /* non-combo */) {
                        (*sIter)->removemDevice(activeDattr.id);
                        (*sIter)->removePalDevice(*sIter, activeDattr.id);
                    }
                } else if (std::find((*sIter)->suspendedOutDevIds.begin(),
                        (*sIter)->suspendedOutDevIds.end(), PAL_DEVICE_OUT_BLUETOOTH_SCO)
                        != (*sIter)->suspendedOutDevIds.end()) {
                    (*sIter)->removemDevice(PAL_DEVICE_OUT_BLUETOOTH_SCO);
                    (*sIter)->removePalDevice(*sIter, PAL_DEVICE_OUT_BLUETOOTH_SCO);
                }
                (*sIter)->addmDevice(&a2dpDattr);
                (*sIter)->addPalDevice(*sIter, &a2dpDattr);
                (*sIter)->a2dpMuted = false;
                (*sIter)->unlockStreamMutex();
            }
        }
        mActiveStreamMutex.unlock();
        goto exit;
    } else {
        PAL_INFO(LOG_TAG, "restoring a2dp/ble stream"); //SEC_AUDIO_ADD_FOR_DEBUG : PAL_DBG -> PAL_INFO
        status = streamDevSwitch(streamDevDisconnect, streamDevConnect);
        if (status) {
            PAL_ERR(LOG_TAG, "streamDevSwitch failed %d", status);
            goto exit;
        }
    }

    mActiveStreamMutex.lock();
    for (sIter = restoredStreams.begin(); sIter != restoredStreams.end(); sIter++) {
        if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
            (*sIter)->lockStreamMutex();
            // update PAL devices for the restored streams
            if (std::find((*sIter)->suspendedOutDevIds.begin(),
                    (*sIter)->suspendedOutDevIds.end(), a2dpDattr.id)
                    != (*sIter)->suspendedOutDevIds.end()) {
                if ((*sIter)->suspendedOutDevIds.size() == 1 /* non-combo */)
                    (*sIter)->removePalDevice(*sIter, activeDattr.id);
            } else if (std::find((*sIter)->suspendedOutDevIds.begin(),
                    (*sIter)->suspendedOutDevIds.end(), PAL_DEVICE_OUT_BLUETOOTH_SCO)
                    != (*sIter)->suspendedOutDevIds.end()) {
                (*sIter)->removePalDevice(*sIter, PAL_DEVICE_OUT_BLUETOOTH_SCO);
            }
            (*sIter)->addPalDevice(*sIter, &a2dpDattr);
            (*sIter)->suspendedOutDevIds.clear();
            // unmute the streams which were muted during a2dpSuspend
            if ((*sIter)->a2dpMuted) {
                status = (*sIter)->getVolumeData(volume);
                if (status) {
                    PAL_ERR(LOG_TAG, "getVolumeData failed %d", status);
                    (*sIter)->unlockStreamMutex();
                    continue;
                }
                /* set a2dpMuted to false so that volume can be applied
                 * volume gets cached if a2dpMuted is set to true
                 */
                (*sIter)->a2dpMuted = false;
                status = (*sIter)->setVolume(volume);
                if (status) {
                    PAL_ERR(LOG_TAG, "setVolume failed %d", status);
                    (*sIter)->a2dpMuted = true;
                    (*sIter)->unlockStreamMutex();
                    continue;
                }
                // set a2dpMuted to true in case unmute failed
                if ((*sIter)->mute_l(false))
                    (*sIter)->a2dpMuted = true;
            }
            (*sIter)->unlockStreamMutex();
        }
    }
    mActiveStreamMutex.unlock();

exit:
    PAL_INFO(LOG_TAG, "exit status: %d", status); //SEC_AUDIO_ADD_FOR_DEBUG : PAL_DBG -> PAL_INFO
    if (volume) {
        free(volume);
    }
    return status;
}

int32_t ResourceManager::a2dpCaptureSuspendToDummy(pal_device_id_t dev_id)
{
    int status = 0;
    std::shared_ptr<Device> a2dpDev = nullptr;
    struct pal_device a2dpDattr = {};
    struct pal_device switchDevDattr = {};
    std::vector <Stream*> activeA2dpStreams;
    std::shared_ptr<Device> switchDev = nullptr;
    std::vector <Stream*>::iterator sIter;
    std::vector <std::tuple<Stream*, uint32_t>> streamDevDisconnect;
    std::vector <std::tuple<Stream*, struct pal_device *>> streamDevConnect;

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_INFO(LOG_TAG, "enter. (device : %d)", dev_id);
#else
    PAL_DBG(LOG_TAG, "enter");
#endif

    a2dpDattr.id = dev_id;
    a2dpDev = Device::getInstance(&a2dpDattr, rm);
    if (!a2dpDev) {
        PAL_ERR(LOG_TAG, "getting a2dp/ble device instance failed");
        goto exit;
    }

    switchDevDattr.id = PAL_DEVICE_IN_DUMMY;
    status = getDeviceConfig(&switchDevDattr, NULL);
    if (status) {
        PAL_ERR(LOG_TAG, "getDeviceConfig failed for in_dummy device");
        goto exit;
    }

    mActiveStreamMutex.lock();
    getActiveStream_l(activeA2dpStreams, a2dpDev);
    if (activeA2dpStreams.size() == 0) {
        PAL_DBG(LOG_TAG, "no active streams found");
        mActiveStreamMutex.unlock();
        goto exit;
    }

    for (sIter = activeA2dpStreams.begin(); sIter != activeA2dpStreams.end(); sIter++) {
        if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
            (*sIter)->lockStreamMutex();
            if (!((*sIter)->a2dpMuted) && !((*sIter)->mute_l(true))) {
                (*sIter)->a2dpMuted = true;
            }
            (*sIter)->suspendedInDevIds.clear();
            (*sIter)->suspendedInDevIds.push_back(a2dpDattr.id);
            streamDevDisconnect.push_back({*sIter, a2dpDattr.id});
            streamDevConnect.push_back({*sIter, &switchDevDattr});
            (*sIter)->unlockStreamMutex();
        }
    }
    mActiveStreamMutex.unlock();

    PAL_INFO(LOG_TAG, "switching capture streams to in_dummy device"); //SEC_AUDIO_ADD_FOR_DEBUG : PAL_DBG -> PAL_INFO
    status = streamDevSwitch(streamDevDisconnect, streamDevConnect);
    if (status) {
        PAL_ERR(LOG_TAG, " streamDevSwitch failed %d", status);
        goto exit;
    }

    mActiveStreamMutex.lock();
    for (sIter = activeA2dpStreams.begin(); sIter != activeA2dpStreams.end(); sIter++) {
        if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
            (*sIter)->lockStreamMutex();
            (*sIter)->removePalDevice(*sIter, a2dpDattr.id);
            (*sIter)->addPalDevice(*sIter, &switchDevDattr);
            (*sIter)->unlockStreamMutex();
        }
    }
    mActiveStreamMutex.unlock();

exit:
    PAL_INFO(LOG_TAG, "exit status: %d", status); //SEC_AUDIO_ADD_FOR_DEBUG : PAL_DBG -> PAL_INFO
    return status;
}

int32_t ResourceManager::a2dpCaptureResumeFromDummy(pal_device_id_t dev_id)
{
    int status = 0;
    std::shared_ptr<Device> activeDev = nullptr;
    struct pal_device activeDattr = {};
    struct pal_device a2dpDattr = {};
    std::vector <Stream*>::iterator sIter;
    std::vector <Stream*> activeStreams;
    std::vector <Stream*> orphanStreams;
    std::vector <Stream*> retryStreams;
    std::vector <Stream*> restoredStreams;
    std::vector <std::tuple<Stream*, uint32_t>> streamDevDisconnect;
    std::vector <std::tuple<Stream*, struct pal_device*>> streamDevConnect;

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_INFO(LOG_TAG, "enter. (device : %d)", dev_id);
#else
    PAL_DBG(LOG_TAG, "enter");
#endif

    activeDattr.id = PAL_DEVICE_IN_DUMMY;
    activeDev = Device::getInstance(&activeDattr, rm);
    if (!activeDev) {
        PAL_ERR(LOG_TAG, "getting out_dummy device instance failed");
        goto exit;
    }

    a2dpDattr.id = dev_id;
    status = getDeviceConfig(&a2dpDattr, NULL);
    if (status) {
        PAL_ERR(LOG_TAG, "getDeviceConfig failed for a2dp/BLE device");
        goto exit;
    }

    mActiveStreamMutex.lock();
    getActiveStream_l(activeStreams, activeDev);
    getOrphanStream_l(orphanStreams, retryStreams);
    if (activeStreams.empty() && orphanStreams.empty() && retryStreams.empty()) {
        PAL_DBG(LOG_TAG, "no active streams found");
        mActiveStreamMutex.unlock();
        goto exit;
    }

    // check all active streams associated with in_dummy device.
    // Istore into stream vector for device switch.
    for (sIter = activeStreams.begin(); sIter != activeStreams.end(); sIter++) {
        if (std::find((*sIter)->suspendedInDevIds.begin(),
                (*sIter)->suspendedInDevIds.end(), a2dpDattr.id)
                != (*sIter)->suspendedInDevIds.end()) {
            restoredStreams.push_back((*sIter));
            streamDevDisconnect.push_back({(*sIter), activeDattr.id });
            streamDevConnect.push_back({(*sIter), &a2dpDattr });
        }
    }

    // retry all orphan streams which failed to restore previously.
    for (sIter = orphanStreams.begin(); sIter != orphanStreams.end(); sIter++) {
        if (std::find((*sIter)->suspendedInDevIds.begin(),
                (*sIter)->suspendedInDevIds.end(), a2dpDattr.id)
                != (*sIter)->suspendedInDevIds.end()) {
            restoredStreams.push_back((*sIter));
            streamDevConnect.push_back({(*sIter), &a2dpDattr });
        }
    }

    // retry all streams which failed to switch to desired device previously.
    for (sIter = retryStreams.begin(); sIter != retryStreams.end(); sIter++) {
        (*sIter)->lockStreamMutex();
        if (std::find((*sIter)->suspendedInDevIds.begin(),
            (*sIter)->suspendedInDevIds.end(), a2dpDattr.id)
            != (*sIter)->suspendedInDevIds.end()) {
            std::vector<std::shared_ptr<Device>> devices;
            (*sIter)->getAssociatedInDevices(devices);
            if (devices.size() > 0) {
                for (auto device : devices) {
                    streamDevDisconnect.push_back({ (*sIter), device->getSndDeviceId() });
                }
            }
#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO
            if (isBtmixRecord && a2dpDattr.id == PAL_DEVICE_IN_BLUETOOTH_BLE) {
                struct pal_device dattr2 = {};
                dattr2.id = PAL_DEVICE_IN_SPEAKER_MIC;
                PAL_INFO(LOG_TAG, "add spk_mic for btmix multidevice recording");
                streamDevConnect.push_back({ (*sIter), &dattr2 });
                strcpy(a2dpDattr.custom_config.custom_key, ck_table[CUSTOM_KEY_CAMCORDER_MULTI_AND_BT_MIC]);
            }
#endif
            restoredStreams.push_back((*sIter));
            streamDevConnect.push_back({ (*sIter), &a2dpDattr });
        }
        (*sIter)->unlockStreamMutex();
    }

    if (restoredStreams.empty()) {
        PAL_DBG(LOG_TAG, "no streams to be restored");
        mActiveStreamMutex.unlock();
        goto exit;
    }
    mActiveStreamMutex.unlock();

    PAL_INFO(LOG_TAG, "restoring a2dp/ble streams"); //SEC_AUDIO_ADD_FOR_DEBUG : PAL_DBG -> PAL_INFO
    status = streamDevSwitch(streamDevDisconnect, streamDevConnect);
    if (status) {
        PAL_ERR(LOG_TAG, "streamDevSwitch failed %d", status);
        goto exit;
    }

    mActiveStreamMutex.lock();
    for (sIter = restoredStreams.begin(); sIter != restoredStreams.end(); sIter++) {
        if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
            (*sIter)->lockStreamMutex();
            (*sIter)->suspendedInDevIds.clear();
            (*sIter)->removePalDevice(*sIter, activeDattr.id);
            (*sIter)->addPalDevice(*sIter, &a2dpDattr);
            if ((*sIter)->a2dpMuted) {
                (*sIter)->mute_l(false);
                (*sIter)->a2dpMuted = false;
            }
            (*sIter)->unlockStreamMutex();
        }
    }
    mActiveStreamMutex.unlock();

exit:
    PAL_DBG(LOG_TAG, "exit status: %d", status);
    return status;
}

int32_t ResourceManager::a2dpSuspend(pal_device_id_t dev_id)
{
    int status = 0;
    uint32_t latencyMs = 0, maxLatencyMs = 0;
    std::shared_ptr<Device> a2dpDev = nullptr;
    struct pal_device a2dpDattr;
    struct pal_device switchDevDattr;
    std::shared_ptr<Device> spkrDev = nullptr;
    std::shared_ptr<Device> handsetDev = nullptr;
    struct pal_device spkrDattr;
    struct pal_device handsetDattr;
    std::vector <Stream *> activeA2dpStreams;
    std::vector <Stream *> activeStreams;
    std::vector <Stream*>::iterator sIter;
    std::vector <std::shared_ptr<Device>> associatedDevices;

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_DBG(LOG_TAG, "enter. (device : %d)", dev_id);
#else
    PAL_DBG(LOG_TAG, "enter");
#endif

    a2dpDattr.id = dev_id;
    a2dpDev = Device::getInstance(&a2dpDattr, rm);
    if (!a2dpDev) {
        PAL_ERR(LOG_TAG, "Getting a2dp/ble device instance failed");
        goto exit;
    }

    spkrDattr.id = PAL_DEVICE_OUT_SPEAKER;
    spkrDev = Device::getInstance(&spkrDattr, rm);
    if (!spkrDev) {
        PAL_ERR(LOG_TAG, "Getting speaker device instance failed");
        goto exit;
    }

    mActiveStreamMutex.lock();
    getActiveStream_l(activeA2dpStreams, a2dpDev);
    if (activeA2dpStreams.size() == 0) {
        PAL_DBG(LOG_TAG, "no active streams found");
        mActiveStreamMutex.unlock();
        goto exit;
    }

    /* Check whether there's active stream associated with handset or speaker
     * when a2dp suspend is called.
     * - Device selected to switch by default is speaker.
     * - Check handset as well if no stream on speaker.
     */
    switchDevDattr.id = PAL_DEVICE_OUT_SPEAKER;
    getActiveStream_l(activeStreams, spkrDev);
    if (activeStreams.empty()) {
        // no stream active on speaker, then check handset as well
        handsetDattr.id = PAL_DEVICE_OUT_HANDSET;
        handsetDev = Device::getInstance(&handsetDattr, rm);
        if (handsetDev) {
            getActiveStream_l(activeStreams, handsetDev);
        } else {
            PAL_ERR(LOG_TAG, "Getting handset device instance failed");
            mActiveStreamMutex.unlock();
            goto exit;
        }

        if (activeStreams.size() != 0) {
            // active streams found on handset
            switchDevDattr.id = PAL_DEVICE_OUT_HANDSET;
            status = handsetDev->getDeviceAttributes(&switchDevDattr);
        } else {
            // no active stream found on speaker or handset, get the deafult
            pal_device_info devInfo;
            memset(&devInfo, 0, sizeof(pal_device_info));
            status = getDeviceConfig(&switchDevDattr, NULL);
            if (!status) {
                // get the default device info and update snd name
                getDeviceInfo(switchDevDattr.id, (pal_stream_type_t)0,
                    switchDevDattr.custom_config.custom_key, &devInfo);
                updateSndName(switchDevDattr.id, devInfo.sndDevName);
            }
        }
    } else {
        // activeStreams found on speaker
        status = spkrDev->getDeviceAttributes(&switchDevDattr);
    }
    if (status) {
        PAL_ERR(LOG_TAG, "Switch DevAttributes Query Failed");
        mActiveStreamMutex.unlock();
        goto exit;
    }

    PAL_DBG(LOG_TAG, "selecting active device_id[%d] and muting streams",
        switchDevDattr.id);

    for (sIter = activeA2dpStreams.begin(); sIter != activeA2dpStreams.end(); sIter++) {
        if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
            associatedDevices.clear();
            status = (*sIter)->getAssociatedOutDevices(associatedDevices);
            if ((0 != status) ||
                !(rm->isDeviceAvailable(associatedDevices, a2dpDattr.id))) {
                PAL_ERR(LOG_TAG, "Error: stream %pK is not associated with A2DP/BLE device", *sIter);
                continue;
            }
            /* For a2dp + spkr or handset combo use case,
             * add speaker or handset into suspended devices for restore during a2dpResume
             */
            (*sIter)->lockStreamMutex();
            if (rm->isDeviceAvailable(associatedDevices, switchDevDattr.id)) {
                // combo use-case; just remember to keep the non-a2dp devices when restoring.
                PAL_DBG(LOG_TAG, "Stream %pK is on combo device; Dont Pause/Mute", *sIter);
                (*sIter)->suspendedOutDevIds.clear();
                (*sIter)->suspendedOutDevIds.push_back(switchDevDattr.id);
                (*sIter)->suspendedOutDevIds.push_back(a2dpDattr.id);
            } else if (!((*sIter)->a2dpMuted)) {
                // only perform Mute/Pause for non combo use-case only.
                struct pal_stream_attributes sAttr;
                (*sIter)->getStreamAttributes(&sAttr);
                (*sIter)->suspendedOutDevIds.clear();
                (*sIter)->suspendedOutDevIds.push_back(a2dpDattr.id);
                if (((sAttr.type == PAL_STREAM_COMPRESSED) ||
                     (sAttr.type == PAL_STREAM_PCM_OFFLOAD))) {
                    /* First mute & then pause
                     * This is to ensure DSP has enough ramp down period in volume module.
                     * If pause is issued firstly, then there's no enough data for processing.
                     * As a result, ramp down will not happen and will only occur after resume,
                     * which is perceived as audio leakage.
                     */
                    if (!(*sIter)->mute_l(true))
                        (*sIter)->a2dpMuted = true;
                    // Pause only if the stream is not explicitly paused.
                    // In some scenarios, stream might have already paused prior to a2dpsuspend.
                    if (((*sIter)->isPaused) == false) {
                        if (!(*sIter)->pause_l())
                            (*sIter)->a2dpPaused = true;
                    }
                } else {
                    latencyMs = (*sIter)->getLatency();
                    if (maxLatencyMs < latencyMs)
                        maxLatencyMs = latencyMs;
                    // Mute
                    if (!(*sIter)->mute_l(true))
                        (*sIter)->a2dpMuted = true;
                }
            }
            (*sIter)->unlockStreamMutex();
        }
    }

    mActiveStreamMutex.unlock();

    // wait for stale pcm drained before switching to speaker
    if (maxLatencyMs > 0) {
        // multiplication factor applied to latency when calculating a safe mute delay
        // TODO: It's not needed if latency is accurate.
        const int latencyMuteFactor = 2;
        usleep(maxLatencyMs * 1000 * latencyMuteFactor);
    }

    forceDeviceSwitch(a2dpDev, &switchDevDattr, activeA2dpStreams);

    mActiveStreamMutex.lock();
    for (sIter = activeA2dpStreams.begin(); sIter != activeA2dpStreams.end(); sIter++) {
        if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
            (*sIter)->lockStreamMutex();
            struct pal_stream_attributes sAttr;
            (*sIter)->getStreamAttributes(&sAttr);
            if (((sAttr.type == PAL_STREAM_COMPRESSED) ||
                (sAttr.type == PAL_STREAM_PCM_OFFLOAD)) &&
                (!(*sIter)->isActive())) {
                /* Resume only when it was paused during a2dpSuspend.
                 * This is to avoid resuming during regular pause.
                 */
                if (((*sIter)->a2dpPaused) == true) {
                    if (!(*sIter)->resume_l())
                        (*sIter)->a2dpPaused = false;
                }
            }
            (*sIter)->unlockStreamMutex();
        }
    }
    mActiveStreamMutex.unlock();

exit:
    PAL_DBG(LOG_TAG, "exit status: %d", status);
    return status;
}

int32_t ResourceManager::a2dpResume(pal_device_id_t dev_id)
{
    int status = 0;
    std::shared_ptr<Device> activeDev = nullptr;
    struct pal_device activeDattr;
    struct pal_device a2dpDattr;
    struct pal_device_info devinfo = {};
    struct pal_volume_data *volume = NULL;
    std::vector <Stream*>::iterator sIter;
    std::vector <Stream *> activeStreams;
    std::vector <Stream *> orphanStreams;
    std::vector <Stream *> retryStreams;
    std::vector <Stream *> restoredStreams;
    std::vector <std::tuple<Stream *, uint32_t>> streamDevDisconnect;
    std::vector <std::tuple<Stream *, struct pal_device *>> streamDevConnect;

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_DBG(LOG_TAG, "enter. (device : %d)", dev_id);
#else
    PAL_DBG(LOG_TAG, "enter");
#endif

    volume = (struct pal_volume_data *)calloc(1, (sizeof(uint32_t) +
                     (sizeof(struct pal_channel_vol_kv) * (0xFFFF))));
    if (!volume) {
        status = -ENOMEM;
        goto exit;
    }

#ifdef SEC_AUDIO_CALL
    struct pal_device sco_rx_dattr;
    sco_rx_dattr.id = PAL_DEVICE_OUT_BLUETOOTH_SCO;
    activeDev = Device::getInstance(&sco_rx_dattr, rm);
    mActiveStreamMutex.lock();
    getActiveStream_l(activeStreams, activeDev);
    for (sIter = activeStreams.begin(); sIter != activeStreams.end(); sIter++) {
        if (std::find((*sIter)->suspendedOutDevIds.begin(), (*sIter)->suspendedOutDevIds.end(),
                    dev_id) != (*sIter)->suspendedOutDevIds.end()) {
            if ((*sIter)->suspendedOutDevIds.size() == 1 && (isDeviceReady(sco_rx_dattr.id))) {
                PAL_INFO(LOG_TAG, "skip a2dpResume when coming A2DP/BLE suspended as false");
                mActiveStreamMutex.unlock();
                goto exit;
            }
        }
    }
    mActiveStreamMutex.unlock();
#endif

    activeDattr.id = PAL_DEVICE_OUT_SPEAKER;
    activeDev = Device::getInstance(&activeDattr, rm);
    a2dpDattr.id = dev_id;

    status = getDeviceConfig(&a2dpDattr, NULL);
    if (status) {
        goto exit;
    }

    mActiveStreamMutex.lock();
    getActiveStream_l(activeStreams, activeDev);
    /* No-Streams active on Speaker - possibly streams are
     * associated handset device (due to voip/voice sco ended) and
     * device switch did not happen for all the streams
     */
    if (activeStreams.empty()) {
        // Hence try to check handset device as well.
        activeDattr.id = PAL_DEVICE_OUT_HANDSET;
        activeDev = Device::getInstance(&activeDattr, rm);
        getActiveStream_l(activeStreams, activeDev);
    }

    getOrphanStream_l(orphanStreams, retryStreams);
    if (activeStreams.empty() && orphanStreams.empty() && retryStreams.empty()) {
        PAL_DBG(LOG_TAG, "no active streams found");
        mActiveStreamMutex.unlock();
        goto exit;
    }

    /* Check all active streams associated with speaker or handset.
     * If actual device is a2dp only, store into stream vector for device switch.
     * If actual device is combo(a2dp + spkr/handset), restore to combo.
     * That is to connect a2dp and do not disconnect from current associated device.
     */
    for (sIter = activeStreams.begin(); sIter != activeStreams.end(); sIter++) {
        (*sIter)->lockStreamMutex();
        if (std::find((*sIter)->suspendedOutDevIds.begin(),
                (*sIter)->suspendedOutDevIds.end(), a2dpDattr.id)
                != (*sIter)->suspendedOutDevIds.end()) {
            restoredStreams.push_back((*sIter));
            if ((*sIter)->suspendedOutDevIds.size() == 1 /* none combo */) {
                streamDevDisconnect.push_back({(*sIter), activeDattr.id});
            }
            streamDevConnect.push_back({(*sIter), &a2dpDattr});
        }
        (*sIter)->unlockStreamMutex();
    }

    // retry all orphan streams which failed to restore previously.
    for (sIter = orphanStreams.begin(); sIter != orphanStreams.end(); sIter++) {
        (*sIter)->lockStreamMutex();
        if (std::find((*sIter)->suspendedOutDevIds.begin(),
                (*sIter)->suspendedOutDevIds.end(), a2dpDattr.id)
                != (*sIter)->suspendedOutDevIds.end()) {
            restoredStreams.push_back((*sIter));
            streamDevConnect.push_back({(*sIter), &a2dpDattr});
        }
        (*sIter)->unlockStreamMutex();
    }

    // retry all streams which failed to switch to desired device previously.
    for (sIter = retryStreams.begin(); sIter != retryStreams.end(); sIter++) {
        (*sIter)->lockStreamMutex();
        if (std::find((*sIter)->suspendedOutDevIds.begin(),
                (*sIter)->suspendedOutDevIds.end(), a2dpDattr.id)
                != (*sIter)->suspendedOutDevIds.end()) {
            std::vector<std::shared_ptr<Device>> devices;
            (*sIter)->getAssociatedOutDevices(devices);
            if ((devices.size() > 0) &&
                    ((*sIter)->suspendedOutDevIds.size() == 1 /* non combo */)) {
                for (auto device: devices) {
                    streamDevDisconnect.push_back({ (*sIter), device->getSndDeviceId() });
                }
            }
            restoredStreams.push_back((*sIter));
            streamDevConnect.push_back({(*sIter), &a2dpDattr});
        } else if (std::find((*sIter)->suspendedOutDevIds.begin(),
                (*sIter)->suspendedOutDevIds.end(), PAL_DEVICE_OUT_BLUETOOTH_SCO)
                != (*sIter)->suspendedOutDevIds.end()) {
            std::vector<std::shared_ptr<Device>> devices;
            (*sIter)->getAssociatedOutDevices(devices);
            if (devices.size() > 0) {
                for (auto device: devices) {
                    if (device->getSndDeviceId() == PAL_DEVICE_OUT_BLUETOOTH_SCO) {
                        streamDevDisconnect.push_back({(*sIter), PAL_DEVICE_OUT_BLUETOOTH_SCO});
                        break;
                    }
                }
            }
            restoredStreams.push_back((*sIter));
            streamDevConnect.push_back({(*sIter), &a2dpDattr});
        }
        (*sIter)->unlockStreamMutex();
    }

    if (restoredStreams.empty()) {
        PAL_DBG(LOG_TAG, "no streams to be restored");
        mActiveStreamMutex.unlock();
        goto exit;
    }
    SortAndUnique(restoredStreams);
    mActiveStreamMutex.unlock();

    /* In case of SSR down event if a2dpSuspended = false sets, since sound card state is offline
    * StreamDevSwitch() operation will be skipped. Due to this mDevices will remain as speaker
    * and when SSR is up, active streams will route to the speaker only.
    * Thus in this corner scenario, update the mDevices as BT A2DP so that when SSR is up
    * active streams will be routed to BT properly.
    */
    if (PAL_CARD_STATUS_DOWN(cardState)) {
        PAL_ERR(LOG_TAG, "Sound card offline");
        mActiveStreamMutex.lock();
        for (sIter = restoredStreams.begin(); sIter != restoredStreams.end(); sIter++) {
            if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
                (*sIter)->lockStreamMutex();
                if (std::find((*sIter)->suspendedOutDevIds.begin(),
                        (*sIter)->suspendedOutDevIds.end(), a2dpDattr.id)
                        != (*sIter)->suspendedOutDevIds.end()) {
                    if ((*sIter)->suspendedOutDevIds.size() == 1 /* non-combo */) {
                        (*sIter)->clearmDevices();
                        (*sIter)->clearOutPalDevices(*sIter);
                    }
                    (*sIter)->addmDevice(&a2dpDattr);
                    (*sIter)->addPalDevice(*sIter, &a2dpDattr);
                } else if (std::find((*sIter)->suspendedOutDevIds.begin(),
                        (*sIter)->suspendedOutDevIds.end(), PAL_DEVICE_OUT_BLUETOOTH_SCO)
                        != (*sIter)->suspendedOutDevIds.end()) {
                    (*sIter)->removemDevice(PAL_DEVICE_OUT_BLUETOOTH_SCO);
                    (*sIter)->removePalDevice(*sIter, PAL_DEVICE_OUT_BLUETOOTH_SCO);
                    (*sIter)->addmDevice(&a2dpDattr);
                    (*sIter)->addPalDevice(*sIter, &a2dpDattr);
                }
                (*sIter)->unlockStreamMutex();
            }
        }
        mActiveStreamMutex.unlock();
    } else {
        PAL_DBG(LOG_TAG, "restoring A2dp and unmuting stream");
        status = streamDevSwitch(streamDevDisconnect, streamDevConnect);
        if (status) {
            PAL_ERR(LOG_TAG, "streamDevSwitch failed %d", status);
            goto exit;
        }
    }

    mActiveStreamMutex.lock();
    for (sIter = restoredStreams.begin(); sIter != restoredStreams.end(); sIter++) {
        if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
            (*sIter)->lockStreamMutex();
            // update PAL devices for the restored streams
            if ((*sIter)->suspendedOutDevIds.size() == 1 /* non-combo */) {
                (*sIter)->clearOutPalDevices(*sIter);
            } else if (std::find((*sIter)->suspendedOutDevIds.begin(),
                    (*sIter)->suspendedOutDevIds.end(), PAL_DEVICE_OUT_BLUETOOTH_SCO)
                    != (*sIter)->suspendedOutDevIds.end()) {
                (*sIter)->removePalDevice(*sIter, PAL_DEVICE_OUT_BLUETOOTH_SCO);
            }
            (*sIter)->addPalDevice(*sIter, &a2dpDattr);

            (*sIter)->suspendedOutDevIds.clear();
            status = (*sIter)->getVolumeData(volume);
            if (status) {
                PAL_ERR(LOG_TAG, "getVolumeData failed %d", status);
                (*sIter)->unlockStreamMutex();
                continue;
            }
            /* set a2dpMuted to false so that volume can be applied
             * volume gets cached if a2dpMuted is set to true
             */
            (*sIter)->a2dpMuted = false;
            status = (*sIter)->setVolume(volume);
            if (status) {
                PAL_ERR(LOG_TAG, "setVolume failed %d", status);
                (*sIter)->a2dpMuted = true;
                (*sIter)->unlockStreamMutex();
                continue;
            }
            // set a2dpMuted to true in case unmute failed
            if ((*sIter)->mute_l(false))
                (*sIter)->a2dpMuted = true;
            (*sIter)->unlockStreamMutex();
        }
    }
    mActiveStreamMutex.unlock();

exit:
    PAL_DBG(LOG_TAG, "exit status: %d", status);
    if (volume) {
        free(volume);
    }
    return status;
}

int32_t ResourceManager::a2dpCaptureSuspend(pal_device_id_t dev_id)
{
    int status = 0;
    uint32_t prio;
    std::shared_ptr<Device> a2dpDev = nullptr;
    struct pal_device a2dpDattr = {};
    struct pal_device handsetmicDattr = {};
    std::vector <Stream*> activeA2dpStreams;
    std::vector <Stream*> activeStreams;
    std::shared_ptr<Device> handsetmicDev = nullptr;
    std::vector <Stream*>::iterator sIter;

    PAL_DBG(LOG_TAG, "enter");

    a2dpDattr.id = dev_id;
    a2dpDev = Device::getInstance(&a2dpDattr, rm);

    if (!a2dpDev) {
        PAL_ERR(LOG_TAG, "Getting a2dp/ble device instance failed");
        goto exit;
    }

    mActiveStreamMutex.lock();
    getActiveStream_l(activeA2dpStreams, a2dpDev);
    if (activeA2dpStreams.size() == 0) {
        PAL_DBG(LOG_TAG, "no active streams found");
        mActiveStreamMutex.unlock();
        goto exit;
    }

    // force switch to handset_mic
    handsetmicDattr.id = PAL_DEVICE_IN_HANDSET_MIC;
    handsetmicDev = Device::getInstance(&handsetmicDattr, rm);
    if (!handsetmicDev) {
        PAL_ERR(LOG_TAG, "Getting handset-mic device instance failed");
        mActiveStreamMutex.unlock();
        goto exit;
    }
    /* Check if any stream device attribute pair is already there for
     * the handset and use its attributes before deciding on
     * using default device info
     */
    status = handsetmicDev->getTopPriorityDeviceAttr(&handsetmicDattr, &prio);
    if (status) {
        // No active streams on handset-mic, get default dev info
        pal_device_info devInfo;
        memset(&devInfo, 0, sizeof(pal_device_info));
        status = getDeviceConfig(&handsetmicDattr, NULL);
        if (!status) {
            // get the default device info and update snd name
            getDeviceInfo(handsetmicDattr.id, (pal_stream_type_t)0,
                handsetmicDattr.custom_config.custom_key, &devInfo);
            updateSndName(handsetmicDattr.id, devInfo.sndDevName);
        }
    }

    for (sIter = activeA2dpStreams.begin(); sIter != activeA2dpStreams.end(); sIter++) {
        if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
            (*sIter)->lockStreamMutex();
            if (!((*sIter)->a2dpMuted)) {
                (*sIter)->mute_l(true);
                (*sIter)->a2dpMuted = true;
            }
            (*sIter)->unlockStreamMutex();
        }
    }
    mActiveStreamMutex.unlock();

    PAL_DBG(LOG_TAG, "selecting handset_mic and muting stream");
    forceDeviceSwitch(a2dpDev, &handsetmicDattr, activeA2dpStreams);

    mActiveStreamMutex.lock();
    for (sIter = activeA2dpStreams.begin(); sIter != activeA2dpStreams.end(); sIter++) {
        if (((*sIter) != NULL) && isStreamActive(*sIter, mActiveStreams)) {
            (*sIter)->suspendedInDevIds.clear();
            (*sIter)->suspendedInDevIds.push_back(a2dpDattr.id);
        }
    }
    mActiveStreamMutex.unlock();

exit:
    PAL_DBG(LOG_TAG, "exit status: %d", status);
    return status;
}

int32_t ResourceManager::a2dpCaptureResume(pal_device_id_t dev_id)
{
    int status = 0;
    std::shared_ptr<Device> activeDev = nullptr;
    struct pal_device activeDattr = {};
    struct pal_device a2dpDattr = {};
    struct pal_device_info devinfo = {};
    std::vector <Stream*>::iterator sIter;
    std::vector <Stream*> activeStreams;
    std::vector <Stream*> orphanStreams;
    std::vector <Stream*> retryStreams;
    std::vector <Stream*> restoredStreams;
    std::vector <std::tuple<Stream*, uint32_t>> streamDevDisconnect;
    std::vector <std::tuple<Stream*, struct pal_device*>> streamDevConnect;

    PAL_DBG(LOG_TAG, "enter");

    activeDattr.id = PAL_DEVICE_IN_HANDSET_MIC;
    activeDev = Device::getInstance(&activeDattr, rm);
    a2dpDattr.id = dev_id;

    status = getDeviceConfig(&a2dpDattr, NULL);
    if (status) {
        goto exit;
    }

    mActiveStreamMutex.lock();
    getActiveStream_l(activeStreams, activeDev);

    /* No-Streams active on Handset-mic - possibly streams are
     * associated speaker-mic device (due to camorder app UC) and
     * device switch did not happen for all the streams
     */
    if (activeStreams.empty()) {
        // Hence try to check speaker-mic device as well.
        activeDattr.id = PAL_DEVICE_IN_SPEAKER_MIC;
        activeDev = Device::getInstance(&activeDattr, rm);
        getActiveStream_l(activeStreams, activeDev);
    }

    getOrphanStream_l(orphanStreams, retryStreams);
    if (activeStreams.empty() && orphanStreams.empty() && retryStreams.empty()) {
        PAL_DBG(LOG_TAG, "no active streams found");
        mActiveStreamMutex.unlock();
        goto exit;
    }

    // check all active streams associated with handset_mic.
    // If actual device is a2dp, store into stream vector for device switch.
    for (sIter = activeStreams.begin(); sIter != activeStreams.end(); sIter++) {
        if (std::find((*sIter)->suspendedInDevIds.begin(),
                (*sIter)->suspendedInDevIds.end(), a2dpDattr.id)
                != (*sIter)->suspendedInDevIds.end()) {
            restoredStreams.push_back((*sIter));
            streamDevDisconnect.push_back({ (*sIter), activeDattr.id });
            streamDevConnect.push_back({ (*sIter), &a2dpDattr });
        }
    }

    // retry all orphan streams which failed to restore previously.
    for (sIter = orphanStreams.begin(); sIter != orphanStreams.end(); sIter++) {
        if (std::find((*sIter)->suspendedInDevIds.begin(),
                (*sIter)->suspendedInDevIds.end(), a2dpDattr.id)
                != (*sIter)->suspendedInDevIds.end()) {
            restoredStreams.push_back((*sIter));
            streamDevConnect.push_back({ (*sIter), &a2dpDattr });
        }
    }

    // retry all streams which failed to switch to desired device previously.
    for (sIter = retryStreams.begin(); sIter != retryStreams.end(); sIter++) {
        (*sIter)->lockStreamMutex();
        if (std::find((*sIter)->suspendedInDevIds.begin(),
                (*sIter)->suspendedInDevIds.end(), a2dpDattr.id)
                != (*sIter)->suspendedInDevIds.end()) {
            std::vector<std::shared_ptr<Device>> devices;
            (*sIter)->getAssociatedInDevices(devices);
            if (devices.size() > 0) {
                for (auto device : devices) {
                    streamDevDisconnect.push_back({ (*sIter), device->getSndDeviceId() });
                }
            }
            restoredStreams.push_back((*sIter));
            streamDevConnect.push_back({ (*sIter), &a2dpDattr });
        }
        (*sIter)->unlockStreamMutex();
    }

    if (restoredStreams.empty()) {
        PAL_DBG(LOG_TAG, "no streams to be restored");
        mActiveStreamMutex.unlock();
        goto exit;
    }
    mActiveStreamMutex.unlock();

    PAL_DBG(LOG_TAG, "restoring A2dp and unmuting stream");
    status = streamDevSwitch(streamDevDisconnect, streamDevConnect);
    if (status) {
        PAL_ERR(LOG_TAG, "streamDevSwitch failed %d", status);
        goto exit;
    }

    mActiveStreamMutex.lock();
    for (sIter = restoredStreams.begin(); sIter != restoredStreams.end(); sIter++) {
        if ((*sIter) && isStreamActive(*sIter, mActiveStreams)) {
            (*sIter)->lockStreamMutex();
            (*sIter)->suspendedInDevIds.clear();
            (*sIter)->mute_l(false);
            (*sIter)->a2dpMuted = false;
            (*sIter)->unlockStreamMutex();
        }
    }
    mActiveStreamMutex.unlock();

exit:
    PAL_DBG(LOG_TAG, "exit status: %d", status);
    return status;
}

int ResourceManager::getParameter(uint32_t param_id, void **param_payload,
                     size_t *payload_size, void *query __unused)
{
    int status = 0;
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_VERBOSE(LOG_TAG, "param_id=%d", param_id);
#else
    PAL_DBG(LOG_TAG, "param_id=%d", param_id);
#endif
    if (param_id == PAL_PARAM_ID_VUI_GET_META_DATA ||
        param_id == PAL_PARAM_ID_VUI_CAPTURE_META_DATA) {
        return VUIGetParameters(param_id, param_payload, payload_size);
    }

    mResourceManagerMutex.lock();
    switch (param_id) {
        case PAL_PARAM_ID_BT_A2DP_RECONFIG_SUPPORTED:
        case PAL_PARAM_ID_BT_A2DP_SUSPENDED:
        case PAL_PARAM_ID_BT_A2DP_ENCODER_LATENCY:
        case PAL_PARAM_ID_BT_A2DP_CAPTURE_SUSPENDED:
        case PAL_PARAM_ID_BT_A2DP_DECODER_LATENCY:
        {
            std::shared_ptr<Device> dev = nullptr;
            struct pal_device dattr;
            pal_param_bta2dp_t* param_bt_a2dp = nullptr;
            dattr.id = (*(pal_param_bta2dp_t**)param_payload)->dev_id;

            dev = Device::getInstance(&dattr, rm);
            if (dev) {
                status = dev->getDeviceParameter(param_id, (void**)&param_bt_a2dp);
                if (status) {
                    PAL_ERR(LOG_TAG, "get Parameter %d failed\n", param_id);
                    goto exit;
                }
                *param_payload = param_bt_a2dp;
                *payload_size = sizeof(pal_param_bta2dp_t);
            }
            break;
        }
        case PAL_PARAM_ID_GAIN_LVL_MAP:
        {
            pal_param_gain_lvl_map_t *param_gain_lvl_map =
                (pal_param_gain_lvl_map_t *)param_payload;

            param_gain_lvl_map->filled_size =
                getGainLevelMapping(param_gain_lvl_map->mapping_tbl,
                                    param_gain_lvl_map->table_size);
            *payload_size = sizeof(pal_param_gain_lvl_map_t);
            break;
        }
        case PAL_PARAM_ID_DEVICE_CAPABILITY:
        {
            pal_param_device_capability_t *param_device_capability = (pal_param_device_capability_t *)(*param_payload);
            PAL_DBG(LOG_TAG, "Device %d card = %d palid=%x",
                        param_device_capability->addr.device_num,
                        param_device_capability->addr.card_id,
                        param_device_capability->id);
            status = getDeviceDefaultCapability(*param_device_capability);
            break;
        }
        case PAL_PARAM_ID_GET_SOUND_TRIGGER_PROPERTIES:
        {
            PAL_DBG(LOG_TAG, "get sound trigge properties, status %d", status);
            struct pal_st_properties *qstp =
                (struct pal_st_properties *)calloc(1, sizeof(struct pal_st_properties));

            GetVoiceUIProperties(qstp);

            *param_payload = qstp;
            *payload_size = sizeof(pal_st_properties);
            break;
        }
        break;
        case PAL_PARAM_ID_SP_MODE:
        {
            PAL_VERBOSE(LOG_TAG, "get parameter for FTM mode");
            std::shared_ptr<Device> dev = nullptr;
            struct pal_device dattr;
            dattr.id = PAL_DEVICE_OUT_SPEAKER;
            dev = Device::getInstance(&dattr , rm);
            if (dev) {
                *payload_size = dev->getParameter(PAL_PARAM_ID_SP_MODE,
                                    param_payload);
            }
        }
        break;
        case PAL_PARAM_ID_SP_GET_CAL:
        {
            PAL_VERBOSE(LOG_TAG, "get parameter for Calibration value");
            std::shared_ptr<Device> dev = nullptr;
            struct pal_device dattr;
            dattr.id = PAL_DEVICE_OUT_SPEAKER;
            dev = Device::getInstance(&dattr , rm);
            if (dev) {
                *payload_size = dev->getParameter(PAL_PARAM_ID_SP_GET_CAL,
                                    param_payload);
            }
        }
        break;
        case PAL_PARAM_ID_SNDCARD_STATE:
        {
            PAL_VERBOSE(LOG_TAG, "get parameter for sndcard state");
            *param_payload = (uint8_t*)&rm->cardState;
            *payload_size = sizeof(rm->cardState);
            break;
        }
        case PAL_PARAM_ID_HIFI_PCM_FILTER:
        {
            PAL_VERBOSE(LOG_TAG, "get parameter for HIFI PCM Filter");

            *payload_size = sizeof(isHifiFilterEnabled);
            **(bool **)param_payload = isHifiFilterEnabled;
        }
        break;
        case PAL_PARAM_ID_LATENCY_MODE:
        {
            std::shared_ptr<Device> dev = nullptr;
            struct pal_device dattr;

            if (isDeviceAvailable((*(pal_param_latency_mode_t**)param_payload)->dev_id)) {
                dattr.id = (*(pal_param_latency_mode_t**)param_payload)->dev_id;
            } else {
                goto exit;
            }
            dev = Device::getInstance(&dattr , rm);
            if (!dev) {
                PAL_ERR(LOG_TAG, "Failed to get device instance");
                goto exit;
            }
            status = dev->getDeviceParameter(PAL_PARAM_ID_LATENCY_MODE, param_payload);
            if (status) {
                PAL_ERR(LOG_TAG, "get Parameter %d failed\n", param_id);
                goto exit;
            }
            *payload_size = sizeof(pal_param_latency_mode_t);
        }
        break;
        case PAL_PARAM_ID_PROXY_RECORD_SESSION:
        {
            PAL_VERBOSE(LOG_TAG, "get parameter for Proxy Record session");
            *payload_size = (isProxyRecordActive ? strlen("true") : strlen("false")) + 1;
            memcpy((char*)param_payload, isProxyRecordActive ? "true" : "false", *payload_size);
        }
        break;
        case PAL_PARAM_ID_ST_CAPTURE_INFO:
        {
            pal_param_st_capture_info_t *stCaptureInfo =
                                    (pal_param_st_capture_info_t *) param_payload;
            int captureHandle = stCaptureInfo->capture_handle;
            if (mStCaptureInfo.find(captureHandle) != mStCaptureInfo.end()) {
                stCaptureInfo->pal_handle = mStCaptureInfo[captureHandle];
                PAL_VERBOSE(LOG_TAG, "getParameter capture handle %d, pal handle %pK",
                                                captureHandle, stCaptureInfo->pal_handle);
            } else {
                PAL_ERR(LOG_TAG, "capture handle not found %d", captureHandle);
            }
        }
        break;
        case PAL_PARAM_ID_HAPTICS_MODE:
        {
            PAL_VERBOSE(LOG_TAG, "get parameter for FTM mode");
            std::shared_ptr<Device> dev = nullptr;
            struct pal_device dattr;
            dattr.id = PAL_DEVICE_OUT_HAPTICS_DEVICE;
            dev = Device::getInstance(&dattr , rm);
            if (dev) {
                status = dev->getParameter(PAL_PARAM_ID_HAPTICS_MODE,
                                    param_payload);
                if (status > 0) {
                    *payload_size = status;
                    status = 0;
                } else {
                    *payload_size = 0;
                }
            }
        }
        break;
#ifdef SEC_AUDIO_AMP_BIGDATA
        case PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_SAVE_RESET:
        case PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_SAVE:
        {
            std::shared_ptr<Device> dev = nullptr;
            struct pal_device dattr = {};
            pal_param_amp_bigdata_t *param_amp_bigdata = nullptr;

            dattr.id = PAL_DEVICE_OUT_SPEAKER;
            dev = Device::getInstance(&dattr , rm);
            if (dev) {
                status = dev->getParameter(param_id, (void **)&param_amp_bigdata);
                if (status) {
                    PAL_ERR(LOG_TAG, "get Parameter %d failed\n", param_id);
                    goto exit;
                }
                *param_payload = param_amp_bigdata;
                *payload_size = sizeof(pal_param_amp_bigdata_t);
            }
        }
        break;
#endif
#ifdef SEC_AUDIO_AMP_SDHMS
        case PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_RCV:
        case PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_SPK:
        {
            std::shared_ptr<Device> dev = nullptr;
            struct pal_device dattr;
            pal_param_amp_ssrm_t *param_amp_ssrm = nullptr;

            dattr.id = PAL_DEVICE_OUT_SPEAKER;
            dev = Device::getInstance(&dattr , rm);
            if (dev) {
                status = dev->getParameter(param_id, (void **)&param_amp_ssrm);
                if (status) {
                    PAL_ERR(LOG_TAG, "get Parameter %d failed\n", param_id);
                    goto exit;
                }
                *param_payload = param_amp_ssrm;
                *payload_size = sizeof(pal_param_amp_ssrm_t);
            }
        }
        break;
#endif
#ifdef ENABLE_TAS_SPK_PROT
        case PAL_TISA_PARAM_GEN_GETPARAM:
        {
            std::shared_ptr<Device> dev = nullptr;
            struct pal_device dattr;
            pal_tispk_prot_param_t *p = (pal_tispk_prot_param_t *)*param_payload;

            dattr.id = PAL_DEVICE_OUT_SPEAKER;
            dev = Device::getInstance(&dattr, rm);
            if (dev) {
                status = dev->getParameter(param_id, (void **)param_payload);
                if (status) {
                    PAL_ERR(LOG_TAG, "TI-SmartPA: RM get Parameter 0x%x failed\n", param_id);
                    goto exit;
                }
                *payload_size = p->hdr[2] * sizeof(uint32_t);
            }
        }
        break;
#endif
#ifdef SEC_AUDIO_BARGEIN_MODE
        case PAL_PARAM_ID_BARGEIN_MODE:
        {
            pal_param_bargein_mode_t *param_bargein_mode = (pal_param_bargein_mode_t *)param_payload;
            mActiveStreamMutex.lock();
            param_bargein_mode->mode = GetBargeinMode();
            mActiveStreamMutex.unlock();
            *payload_size = sizeof(pal_param_bargein_mode_t);
        }
        break;
#endif
        default:
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Unknown ParamID:%d", param_id);
            break;
    }
exit:
    mResourceManagerMutex.unlock();
    return status;
}


int ResourceManager::getParameter(uint32_t param_id, void *param_payload,
                                  size_t payload_size __unused,
                                  pal_device_id_t pal_device_id,
                                  pal_stream_type_t pal_stream_type)
{
    int status = -EINVAL;

    PAL_DBG(LOG_TAG, "param_id=%d", param_id);
    switch (param_id) {
        case PAL_PARAM_ID_UIEFFECT:
        {
#ifdef SEC_AUDIO_COMMON
            pal_param_payload *pal_param = (pal_param_payload *)param_payload;
            effect_pal_payload_t *effectPayload = (effect_pal_payload_t *)pal_param->payload;
            pal_effect_custom_payload_t *effectCustomPayload = nullptr;
            effectCustomPayload = (pal_effect_custom_payload_t *)effectPayload->payload;
            PAL_INFO(LOG_TAG, "PAL_PARAM_ID_UIEFFECT %s(0x%x)",
                                customPayloadParamName.at(effectCustomPayload->paramId).c_str(),
                                effectCustomPayload->paramId);
#endif
            bool match = false;
            std::list<Stream*>::iterator sIter;
            lockActiveStream();
            for(sIter = mActiveStreams.begin(); sIter != mActiveStreams.end(); sIter++) {
#ifdef SEC_AUDIO_COMMON
                match = (*sIter)->checkStreamEffectMatch(pal_device_id,
                                                         pal_stream_type,
                                                         effectCustomPayload->paramId);
#else
                match = (*sIter)->checkStreamMatch(pal_device_id, pal_stream_type);
#endif
                if (match) {
                    if (increaseStreamUserCounter(*sIter) < 0)
                        continue;
                    unlockActiveStream();

#ifdef SEC_AUDIO_CALL
                    if (effectCustomPayload->paramId == PARAM_ID_PP_AUDIO_ECHOREF_PARAMS ||
                        effectCustomPayload->paramId == PARAM_ID_ECHOREF_MUTE_DETECT_PARAM) {
                        status = (*sIter)->getParameters(param_id, (void **)param_payload);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG, "getParameters(paramId 0x%x) Failed",
                                            effectCustomPayload->paramId);
                        }
                    } else
#endif
                    {
                        status = (*sIter)->getEffectParameters(param_payload);
                    }

                    lockActiveStream();
                    decreaseStreamUserCounter(*sIter);
                    break;
                }
            }
            unlockActiveStream();
            break;
        }
        default:
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Unknown ParamID:%d", param_id);
            break;
    }

    return status;
}

int ResourceManager::setParameter(uint32_t param_id, void *param_payload,
                                  size_t payload_size)
{
    int status = 0;

    PAL_DBG(LOG_TAG, "Enter param id: %d", param_id);

    if (param_id == PAL_PARAM_ID_VUI_SET_META_DATA) {
        return VUISetParameters(param_id, param_payload, payload_size);
    }

    mResourceManagerMutex.lock();
    switch (param_id) {
        case PAL_PARAM_ID_UHQA_FLAG:
        {
            pal_param_uhqa_t* param_uhqa_flag = (pal_param_uhqa_t*) param_payload;
#ifdef SEC_AUDIO_SUPPORT_UHQ
            PAL_INFO(LOG_TAG, "UHQA State:%d", param_uhqa_flag->state);
#else
            PAL_INFO(LOG_TAG, "UHQA State:%d", param_uhqa_flag->uhqa_state);
#endif
            if (payload_size == sizeof(pal_param_uhqa_t)) {
#ifdef SEC_AUDIO_SUPPORT_UHQ
                stateUHQA = param_uhqa_flag->state;
#else
                if (param_uhqa_flag->uhqa_state)
                    isUHQAEnabled = true;
                else
                    isUHQAEnabled = false;
#endif
            } else {
                PAL_ERR(LOG_TAG,"Incorrect size : expected (%zu), received(%zu)",
                        sizeof(pal_param_uhqa_t), payload_size);
                status = -EINVAL;
            }
        }
        break;
        case PAL_PARAM_ID_SCREEN_STATE:
        {
            pal_param_screen_state_t* param_screen_st = (pal_param_screen_state_t*) param_payload;
            PAL_INFO(LOG_TAG, "Screen State:%d", param_screen_st->screen_state);
            if (payload_size == sizeof(pal_param_screen_state_t)) {
                status = handleScreenStatusChange(*param_screen_st);
            } else {
                PAL_ERR(LOG_TAG,"Incorrect size : expected (%zu), received(%zu)",
                        sizeof(pal_param_screen_state_t), payload_size);
                status = -EINVAL;
            }
        }
        break;
        case PAL_PARAM_ID_DEVICE_ROTATION:
        {
            pal_param_device_rotation_t* param_device_rot =
                                   (pal_param_device_rotation_t*) param_payload;

            PAL_INFO(LOG_TAG, "Device Rotation :%d", param_device_rot->rotation_type);
            if (payload_size == sizeof(pal_param_device_rotation_t)) {
                mResourceManagerMutex.unlock();
                mActiveStreamMutex.lock();
                status = handleDeviceRotationChange(*param_device_rot);
                status = SetOrientationCal(*param_device_rot);
                mActiveStreamMutex.unlock();
                mResourceManagerMutex.lock();
            } else {
                PAL_ERR(LOG_TAG, "incorrect payload size : expected (%zu), received(%zu)",
                      sizeof(pal_param_device_rotation_t), payload_size);
                status = -EINVAL;
                goto exit;
            }
        }
        break;
        case PAL_PARAM_ID_HAPTICS_MODE:
        {
            pal_haptics_payload *hapModeVal =
                (pal_haptics_payload *) param_payload;

            if (payload_size == sizeof(pal_haptics_payload)) {
                switch(hapModeVal->operationMode) {
                    case PAL_HAP_MODE_FACTORY_TEST:
                    case PAL_HAP_MODE_DYNAMIC_CAL:
                    {
                        struct pal_device dattr;
                        dattr.id = PAL_DEVICE_OUT_HAPTICS_DEVICE;
                        std::shared_ptr<Device> dev = nullptr;

                        memset (&mHapticsModeValue, 0,
                                        sizeof(pal_haptics_payload));
                        mHapticsModeValue.operationMode =
                                hapModeVal->operationMode;

                        dev = Device::getInstance(&dattr , rm);
                        if (dev) {
                            PAL_DBG(LOG_TAG, "Got Haptics Device Instance, mode:%d",
                                                          hapModeVal->operationMode);
                            dev->setParameter(hapModeVal->operationMode, nullptr);
                        }
                        else {
                            PAL_DBG(LOG_TAG, "Unable to get haptics device instance");
                        }
                    }
                    break;
                    default:
                    {
                        PAL_ERR(LOG_TAG, "unsupported hap op mode",
                                hapModeVal->operationMode);
                        status = -EINVAL;
                        goto exit;
                    }
                }
            }
        }
        break;
        case PAL_PARAM_ID_SP_MODE:
        {
            pal_spkr_prot_payload *spModeval =
                    (pal_spkr_prot_payload *) param_payload;

            if (payload_size == sizeof(pal_spkr_prot_payload)) {
                switch(spModeval->operationMode) {
                    case PAL_SP_MODE_DYNAMIC_CAL:
                    {
                        struct pal_device dattr;
                        dattr.id = PAL_DEVICE_OUT_SPEAKER;
                        std::shared_ptr<Device> dev = nullptr;

                        memset (&mSpkrProtModeValue, 0,
                                        sizeof(pal_spkr_prot_payload));
                        mSpkrProtModeValue.operationMode =
                                PAL_SP_MODE_DYNAMIC_CAL;

                        dev = Device::getInstance(&dattr , rm);
                        if (dev) {
                            PAL_DBG(LOG_TAG, "Got Speaker instance");
                            dev->setParameter(PAL_SP_MODE_DYNAMIC_CAL, nullptr);
                        }
                        else {
                            PAL_DBG(LOG_TAG, "Unable to get speaker instance");
                        }
                    }
                    break;
                    case PAL_SP_MODE_FACTORY_TEST:
                    {
                        memset (&mSpkrProtModeValue, 0,
                                        sizeof(pal_spkr_prot_payload));
                        mSpkrProtModeValue.operationMode =
                                PAL_SP_MODE_FACTORY_TEST;
                        mSpkrProtModeValue.spkrHeatupTime =
                                spModeval->spkrHeatupTime;
                        mSpkrProtModeValue.operationModeRunTime =
                                spModeval->operationModeRunTime;
                    }
                    break;
                    case PAL_SP_MODE_V_VALIDATION:
                    {
                        memset (&mSpkrProtModeValue, 0,
                                        sizeof(pal_spkr_prot_payload));
                        mSpkrProtModeValue.operationMode =
                                PAL_SP_MODE_V_VALIDATION;
                        mSpkrProtModeValue.spkrHeatupTime =
                                spModeval->spkrHeatupTime;
                        mSpkrProtModeValue.operationModeRunTime =
                                spModeval->operationModeRunTime;
                    }
                    break;
                }
            } else {
                PAL_ERR(LOG_TAG,"Incorrect size : expected (%zu), received(%zu)",
                        sizeof(pal_param_device_rotation_t), payload_size);
                status = -EINVAL;
            }
        }
        break;
        case PAL_PARAM_ID_DEVICE_CONNECTION:
        {
            pal_param_device_connection_t *device_connection =
                (pal_param_device_connection_t *)param_payload;
            std::shared_ptr<Device> dev = nullptr;
            struct pal_device dattr;
            pal_device_id_t st_device;

            PAL_INFO(LOG_TAG, "Device %d connected = %d",
                        device_connection->id,
                        device_connection->connection_state);
            if (payload_size == sizeof(pal_param_device_connection_t)) {
#ifdef SEC_AUDIO_BT_OFFLOAD
                // to check device connnected state is changed (disconnect -> connect)
                bool device_available = isDeviceAvailable(device_connection->id);
#endif
                status = handleDeviceConnectionChange(*device_connection);
                if (!status && (device_connection->id == PAL_DEVICE_OUT_BLUETOOTH_A2DP ||
                    device_connection->id == PAL_DEVICE_IN_BLUETOOTH_A2DP ||
                    device_connection->id == PAL_DEVICE_OUT_BLUETOOTH_BLE ||
                    device_connection->id == PAL_DEVICE_IN_BLUETOOTH_BLE ||
                    device_connection->id == PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST)) {
                    dattr.id = device_connection->id;
                    dev = Device::getInstance(&dattr, rm);
                    if (dev) {
                        status = dev->setDeviceParameter(param_id, param_payload);
#ifdef SEC_AUDIO_BT_OFFLOAD
                        // check a2dp restart needed for a2dp playback
                        // (TODO) Need to check for BLE
                        if ((device_connection->id == PAL_DEVICE_OUT_BLUETOOTH_A2DP)
                                && (device_connection->connection_state)
                                && !device_available) {
                            std::vector<Stream*> activestreams;
                            getActiveStream_l(activestreams, dev);
                            // if active stream exist on a2dp device,
                            // call suspend to reroute / start a2dp again
                            if (activestreams.size() != 0) {
                                PAL_INFO(LOG_TAG, "active streams found on a2dp device, set suspend t->f");
                                pal_param_bta2dp_t param_bt_a2dp;
                                param_bt_a2dp.dev_id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
                                param_bt_a2dp.a2dp_suspended = true;
                                mResourceManagerMutex.unlock();
                                dev->setDeviceParameter(PAL_PARAM_ID_BT_A2DP_SUSPENDED, &param_bt_a2dp);
                                param_bt_a2dp.a2dp_suspended = false;
                                dev->setDeviceParameter(PAL_PARAM_ID_BT_A2DP_SUSPENDED, &param_bt_a2dp);
                                mResourceManagerMutex.lock();
                            }
                        }
#endif
                    }
                } else {
                    /* Handle device switch for Sound Trigger streams */
                    if (device_connection->id == PAL_DEVICE_IN_WIRED_HEADSET) {
                        st_device = PAL_DEVICE_IN_HEADSET_VA_MIC;
                    } else {
                        PAL_INFO(LOG_TAG, "Unsupported device %d for Sound Trigger streams",
                                 device_connection->id);
                        goto exit;
                    }
                    SwitchSoundTriggerDevices(device_connection->connection_state, st_device);
                }
            } else {
                PAL_ERR(LOG_TAG,"Incorrect size : expected (%zu), received(%zu)",
                      sizeof(pal_param_device_connection_t), payload_size);
                status = -EINVAL;
            }
        }
        break;
        case PAL_PARAM_ID_CHARGING_STATE:
        {
            pal_param_charging_state *battery_charging_state =
                (pal_param_charging_state *)param_payload;
            bool action;
            if (IsTransitToNonLPIOnChargingSupported()) {
                if (payload_size == sizeof(pal_param_charging_state)) {
                    PAL_INFO(LOG_TAG, "Charging State = %d",
                              battery_charging_state->charging_state);
                    if (charging_state_ ==
                        battery_charging_state->charging_state) {
                        PAL_DBG(LOG_TAG, "Charging state unchanged, ignore");
                        break;
                    }
                    charging_state_ = battery_charging_state->charging_state;
                    mResourceManagerMutex.unlock();
                    mActiveStreamMutex.lock();
                    onChargingStateChange();
                    mActiveStreamMutex.unlock();
                    mResourceManagerMutex.lock();
                } else {
                    PAL_ERR(LOG_TAG,
                            "Incorrect size : expected (%zu), received(%zu)",
                            sizeof(pal_param_charging_state), payload_size);
                    status = -EINVAL;
                }
            } else {
                PAL_DBG(LOG_TAG,
                          "transit_to_non_lpi_on_charging set to false\n");
            }
        }
        break;
        case PAL_PARAM_ID_CHARGER_STATE:
        {
            int i, tag;
            struct pal_device dattr;
            Stream *stream = nullptr;
            std::vector<Stream*> activestreams;
            std::shared_ptr<Device> dev = nullptr;

            pal_param_charger_state *charger_state =
                (pal_param_charger_state *)param_payload;

            if (!isChargeConcurrencyEnabled) goto exit;

            if (payload_size != sizeof(pal_param_charger_state)) {
                PAL_ERR(LOG_TAG, "Incorrect size: expected (%zu), received(%zu)",
                                  sizeof(pal_param_charger_state), payload_size);
                status = -EINVAL;
                goto exit;
            }
            if (is_charger_online_ != charger_state->is_charger_online) {
                dattr.id = PAL_DEVICE_OUT_SPEAKER;
                is_charger_online_ = charger_state->is_charger_online;
                current_concurrent_state_ = charger_state->is_concurrent_boost_enable;
                for (i = 0; i < active_devices.size(); i++) {
                    int deviceId = active_devices[i].first->getSndDeviceId();
                    if (deviceId == dattr.id) {
                        dev = Device::getInstance(&dattr, rm);
                        tag = is_charger_online_ ? CHARGE_CONCURRENCY_ON_TAG
                        : CHARGE_CONCURRENCY_OFF_TAG;
                        //Setting deviceRX: Config ICL Tag in AL module.
                        status = rm->getActiveStream_l(activestreams, dev);
                        if ((0 != status) || (activestreams.size() == 0)) {
                            PAL_DBG(LOG_TAG, "no active stream available");
                            goto exit;
                        }
                        stream = static_cast<Stream*>(activestreams[0]);
                        mResourceManagerMutex.unlock();
                        /*
                         When charger is offline, reconfig ICL at normal gain first then
                         handle charger event, Otherwise for charger online case handle
                         charger event and then set config as part of device switch.
                        */
                        if (tag == CHARGE_CONCURRENCY_OFF_TAG)
                            status = setSessionParamConfig(param_id, stream, tag);
                        if (0 == status)
                            status = handleChargerEvent(stream, tag);
                        mResourceManagerMutex.lock();
                        if (0 != status)
                            PAL_ERR(LOG_TAG, "SetSession Param config failed %d", status);
                        break;
                    }
                }
                if (i == active_devices.size())
                    PAL_DBG(LOG_TAG, "Device %d is not available\n", dattr.id);
            } else {
                PAL_DBG(LOG_TAG, "Charger state unchanged, ignore");
            }
        }
        break;
        case PAL_PARAM_ID_BT_SCO:
        {
            std::shared_ptr<Device> dev = nullptr;
            struct pal_device dattr;
            pal_param_btsco_t* param_bt_sco = nullptr;
            bool isScoOn = false;

            dattr.id = PAL_DEVICE_OUT_BLUETOOTH_SCO;
            if (!isDeviceAvailable(dattr.id)) {
                dattr.id = PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
                if (!isDeviceAvailable(dattr.id)) {
                    PAL_INFO(LOG_TAG, "SCO output and input devices are all unavailable");
                }
            }

            dev = Device::getInstance(&dattr, rm);
            if (!dev) {
                PAL_ERR(LOG_TAG, "Device getInstance failed");
                status = -ENODEV;
                goto exit;
            }

            isScoOn = dev->isDeviceReady();
            param_bt_sco = (pal_param_btsco_t*)param_payload;
            if (isScoOn == param_bt_sco->bt_sco_on) {
                PAL_INFO(LOG_TAG, "SCO already in requested state, ignoring");
                goto exit;
            }

            status = dev->setDeviceParameter(param_id, param_payload);
            if (status) {
                PAL_ERR(LOG_TAG, "set Parameter %d failed\n", param_id);
                goto exit;
            }

            if (param_bt_sco->bt_sco_on) {
                mResourceManagerMutex.unlock();
                reconfigureScoStreams();
                mResourceManagerMutex.lock();
            }
        }
        break;
        case PAL_PARAM_ID_BT_SCO_WB:
        case PAL_PARAM_ID_BT_SCO_SWB:
        case PAL_PARAM_ID_BT_SCO_LC3:
        case PAL_PARAM_ID_BT_SCO_NREC:
#ifdef SEC_AUDIO_BLUETOOTH
        case PAL_PARAM_ID_BT_SCO_CODEC_TYPE:
#endif
        {
            std::shared_ptr<Device> dev = nullptr;
            std::vector<pal_device_id_t> scoDev;
            scoDev.push_back(PAL_DEVICE_OUT_BLUETOOTH_SCO);
            scoDev.push_back(PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET);
            struct pal_device dattr;
            mResourceManagerMutex.unlock();
            for (auto devId: scoDev) {
                dattr.id = devId;
                dev = Device::getInstance(&dattr, rm);
                if (dev)
                    status = dev->setDeviceParameter(param_id, param_payload);
            }
            mResourceManagerMutex.lock();
        }
        break;
        case PAL_PARAM_ID_BT_A2DP_RECONFIG:
        {
            std::shared_ptr<BtA2dp> a2dp_dev = nullptr;
            std::vector <Stream *> activeA2dpStreams;
            struct pal_device dattr;
            pal_param_bta2dp_t *current_param_bt_a2dp = nullptr;
            pal_param_bta2dp_t param_bt_a2dp;
            int retrycnt = 20;
            const int retryPeriodMs = 100;

            if (isDeviceAvailable(PAL_DEVICE_OUT_BLUETOOTH_A2DP)) {
                dattr.id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
                a2dp_dev = std::dynamic_pointer_cast<BtA2dp>
                                (BtA2dp::getInstance(&dattr, rm));
                if (!a2dp_dev) {
                    PAL_ERR(LOG_TAG, "Device getInstance failed");
                    status = -ENODEV;
                    goto exit;
                }

                if (a2dp_dev->checkDeviceStatus() == A2DP_STATE_DISCONNECTED) {
                    PAL_ERR(LOG_TAG, "failed to open A2dp source, skip a2dp reconfig.");
                    status = -ENODEV;
                    goto exit;
                }

                getActiveStream_l(activeA2dpStreams, a2dp_dev);
                if (activeA2dpStreams.size() == 0) {
                    PAL_DBG(LOG_TAG, "no active a2dp stream available, skip a2dp reconfig.");
                    status = 0;
                    goto exit;
                }

                a2dp_dev->setDeviceParameter(param_id, param_payload);
                a2dp_dev->getDeviceParameter(param_id, (void **)&current_param_bt_a2dp);
                if ((current_param_bt_a2dp->reconfig == true) &&
                    (current_param_bt_a2dp->a2dp_suspended == false)) {
                    mResourceManagerMutex.unlock();
                    status = a2dpReconfig();

                    /* During reconfig stage, if a2dp is not in a ready state streamdevswitch
                     * to BT will be failed. Reiterate the a2dpreconfig until it
                     * succeeds with sleep period of 100 msecs and retry count = 20.
                     */
                    while ((status != 0) && (retrycnt > 0)) {
                        if (isDeviceReady(PAL_DEVICE_OUT_BLUETOOTH_A2DP)) {
                            status = a2dpReconfig();
                        }
                        usleep(retryPeriodMs * 1000);
                        retrycnt--;
                    }

                    mResourceManagerMutex.lock();

                    param_bt_a2dp.reconfig = false;
                    a2dp_dev->setDeviceParameter(param_id, &param_bt_a2dp);
                }
            }
        }
        break;
        case PAL_PARAM_ID_BT_A2DP_SUSPENDED:
        {
            std::shared_ptr<Device> a2dp_dev = nullptr;
            struct pal_device a2dp_dattr;
            pal_param_bta2dp_t *current_param_bt_a2dp = nullptr;
            pal_param_bta2dp_t *param_bt_a2dp = nullptr;
            bool skip_switch = false;

            mResourceManagerMutex.unlock();
            param_bt_a2dp = (pal_param_bta2dp_t*)param_payload;
            a2dp_dattr.id = param_bt_a2dp->dev_id;

            a2dp_dev = Device::getInstance(&a2dp_dattr , rm);
            if (!a2dp_dev) {
                PAL_ERR(LOG_TAG, "Failed to get A2DP/BLE instance");
                status = -ENODEV;
                goto exit_no_unlock;
            }

            a2dp_dev->getDeviceParameter(param_id, (void **)&current_param_bt_a2dp);
            /* If device is already suspended from framework, ignore suspend/resume
             * which is sent via reconfig_cb. Honouring the param in such scenario
             * will lead to incorrect stream state.
             */
            if (current_param_bt_a2dp->a2dp_suspended &&
                current_param_bt_a2dp->is_suspend_setparam &&
                !param_bt_a2dp->is_suspend_setparam) {
                PAL_INFO(LOG_TAG, "suspend/resume from reconfig_cb ignored");
                goto exit_no_unlock;
            }

            if (current_param_bt_a2dp->a2dp_suspended ==
                    param_bt_a2dp->a2dp_suspended) {
                PAL_INFO(LOG_TAG, "A2DP/BLE already in requested state, ignoring");
                goto exit_no_unlock;
            }

            if ((!isDeviceAvailable(param_bt_a2dp->dev_id)) ||
                 (param_bt_a2dp->is_suspend_setparam && param_bt_a2dp->is_in_call)) {
                skip_switch = true;
            }
#ifdef SEC_AUDIO_BT_OFFLOAD
            else if ((a2dp_dattr.id == PAL_DEVICE_OUT_BLUETOOTH_A2DP)
                    && (!param_bt_a2dp->is_bt_offload_enabled)) {
                // if using bt hal, no need to switch
                PAL_VERBOSE(LOG_TAG, "using bt hal case, do not need to switch sco to a2dp");
                skip_switch = true;
            }
#endif
#ifdef SEC_AUDIO_BLE_OFFLOAD
            else if (a2dp_dattr.id == PAL_DEVICE_OUT_BLUETOOTH_BLE) {
                PAL_DBG(LOG_TAG, "by ble suspend false, not to switch sco to dummy device");
                skip_switch = true;
            }
#endif

            if (ResourceManager::isDummyDevEnabled) {
                if (!skip_switch && param_bt_a2dp->a2dp_suspended == false) {
                    struct pal_device sco_tx_dattr = {};
                    struct pal_device sco_rx_dattr = {};
                    std::shared_ptr<Device> sco_tx_dev = nullptr;
                    std::shared_ptr<Device> sco_rx_dev = nullptr;
                    struct pal_device in_dummy_dattr = {};
                    std::vector<Stream *> activestreams;
                    std::vector<Stream *>::iterator sIter;
                    pal_stream_type_t streamType;

                    /* Handle bt sco mic running usecase */
                    sco_tx_dattr.id = PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
                    if (isDeviceAvailable(sco_tx_dattr.id)) {
                        in_dummy_dattr.id = PAL_DEVICE_IN_DUMMY;
                        sco_tx_dev = Device::getInstance(&sco_tx_dattr, rm);
                        mActiveStreamMutex.lock();
                        getActiveStream_l(activestreams, sco_tx_dev);
                        mActiveStreamMutex.unlock();
                        if (activestreams.size() > 0) {
                            /* Mark streams over IN_SCO, so as to give them chance
                             * to resume over A2DP/BLE if a2dpCatureSuspend=false is
                             * received at later stage.
                             */
                            for (sIter = activestreams.begin();
                                     sIter != activestreams.end(); sIter++) {
                                (*sIter)->suspendedInDevIds.clear();
                                if (isDeviceAvailable(PAL_DEVICE_IN_BLUETOOTH_A2DP))
                                    (*sIter)->suspendedInDevIds.
                                                  push_back(PAL_DEVICE_IN_BLUETOOTH_A2DP);
                                else if (isDeviceAvailable(PAL_DEVICE_IN_BLUETOOTH_BLE))
                                    (*sIter)->suspendedInDevIds.
                                                  push_back(PAL_DEVICE_IN_BLUETOOTH_BLE);
                            }
                            PAL_DBG(LOG_TAG, "a2dp resumed, switch bt sco mic to in_dummy device");
                            getDeviceConfig(&in_dummy_dattr, NULL);
                            rm->forceDeviceSwitch(sco_tx_dev, &in_dummy_dattr);
                        }
                    }

                    /* Handle bt sco out running usecase */
                    mActiveStreamMutex.lock();
                    sco_rx_dattr.id = PAL_DEVICE_OUT_BLUETOOTH_SCO;
                    if (isDeviceAvailable(sco_rx_dattr.id)) {
                        sco_rx_dev = Device::getInstance(&sco_rx_dattr, rm);
                        getActiveStream_l(activestreams, sco_rx_dev);
                        for (sIter = activestreams.begin(); sIter != activestreams.end(); sIter++) {
                            status = (*sIter)->getStreamType(&streamType);
                            if (0 != status) {
                                PAL_ERR(LOG_TAG, "getStreamType failed with status = %d", status);
                                continue;
                            }
                            if ((streamType == PAL_STREAM_LOW_LATENCY) ||
                                (streamType == PAL_STREAM_ULTRA_LOW_LATENCY) ||
                                (streamType == PAL_STREAM_VOIP_RX) ||
                                (streamType == PAL_STREAM_PCM_OFFLOAD) ||
                                (streamType == PAL_STREAM_DEEP_BUFFER) ||
                                (streamType == PAL_STREAM_SPATIAL_AUDIO) ||
                                (streamType == PAL_STREAM_COMPRESSED) ||
                                (streamType == PAL_STREAM_GENERIC)) {
                                if ((*sIter)->suspendedOutDevIds.empty()) {
                                    (*sIter)->suspendedOutDevIds.
                                        push_back(PAL_DEVICE_OUT_BLUETOOTH_SCO);
                                    PAL_DBG(LOG_TAG, "a2dp resumed, \
                                        mark sco streams as to route them later");
                                }
                            }
                        }
                    }
                    mActiveStreamMutex.unlock();
                }
            } else {
                if (!skip_switch && param_bt_a2dp->a2dp_suspended == false) {
                    struct pal_device sco_tx_dattr;
                    struct pal_device sco_rx_dattr;
                    std::shared_ptr<Device> sco_tx_dev = nullptr;
                    std::shared_ptr<Device> sco_rx_dev = nullptr;
                    struct pal_device handset_tx_dattr = {};
                    struct pal_device_info devInfo = {};
                    struct pal_stream_attributes sAttr;
                    std::vector<Stream*> activestreams;
                    std::vector<Stream*>::iterator sIter;
                    Stream *stream = NULL;
                    pal_stream_type_t streamType;

                    mActiveStreamMutex.lock();
                    /* Handle bt sco mic running usecase */
                    sco_tx_dattr.id = PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
                    if (isDeviceAvailable(sco_tx_dattr.id)) {
                        handset_tx_dattr.id = PAL_DEVICE_IN_HANDSET_MIC;
                        sco_tx_dev = Device::getInstance(&sco_tx_dattr, rm);
                        getActiveStream_l(activestreams, sco_tx_dev);
                        if (activestreams.size() > 0) {
                            /* Mark streams over IN_SCO, so as to give them chance
                             * to resume over A2DP/BLE if a2dpCatureSuspend=false is
                             * received at later stage.
                             */
                            for (sIter = activestreams.begin();
                                     sIter != activestreams.end(); sIter++) {
                                (*sIter)->suspendedInDevIds.clear();
                                if (isDeviceAvailable(PAL_DEVICE_IN_BLUETOOTH_A2DP))
                                    (*sIter)->suspendedInDevIds.
                                                  push_back(PAL_DEVICE_IN_BLUETOOTH_A2DP);
                                else if (isDeviceAvailable(PAL_DEVICE_IN_BLUETOOTH_BLE))
                                    (*sIter)->suspendedInDevIds.
                                                  push_back(PAL_DEVICE_IN_BLUETOOTH_BLE);
                            }
#ifdef SEC_AUDIO_CALL
                            stream = static_cast<Stream *>(activestreams[0]);
                            stream->getStreamAttributes(&sAttr);
                            if ((sAttr.direction == PAL_AUDIO_INPUT) && (sAttr.type != PAL_STREAM_VOIP_TX)) {
                                PAL_INFO(LOG_TAG, "a2dp resumed, switch bt sco mic to handset mic");
                                getDeviceConfig(&handset_tx_dattr, &sAttr);
                                getDeviceInfo(handset_tx_dattr.id, sAttr.type,
                                    handset_tx_dattr.custom_config.custom_key, &devInfo);
                                updateSndName(handset_tx_dattr.id, devInfo.sndDevName);
                                mActiveStreamMutex.unlock();
                                rm->forceDeviceSwitch(sco_tx_dev, &handset_tx_dattr);
                                mActiveStreamMutex.lock();
                            } else {
                                PAL_INFO(LOG_TAG, "skip force routing when coming a2dpsuspended as false during call");
                            }
#else
                            PAL_DBG(LOG_TAG, "a2dp resumed, switch bt sco mic to handset mic");
                            stream = static_cast<Stream *>(activestreams[0]);
                            stream->getStreamAttributes(&sAttr);
                            getDeviceConfig(&handset_tx_dattr, &sAttr);
                            getDeviceInfo(handset_tx_dattr.id, sAttr.type,
                                    handset_tx_dattr.custom_config.custom_key, &devInfo);
                            updateSndName(handset_tx_dattr.id, devInfo.sndDevName);
                            mActiveStreamMutex.unlock();
                            rm->forceDeviceSwitch(sco_tx_dev, &handset_tx_dattr);
                            mActiveStreamMutex.lock();
#endif
                        }
                    }

                    /* Handle bt sco running usecase */
                    sco_rx_dattr.id = PAL_DEVICE_OUT_BLUETOOTH_SCO;
                    if (isDeviceAvailable(sco_rx_dattr.id)) {
                        sco_rx_dev = Device::getInstance(&sco_rx_dattr, rm);
                        getActiveStream_l(activestreams, sco_rx_dev);
                        for (sIter = activestreams.begin(); sIter != activestreams.end(); sIter++) {
                            status = (*sIter)->getStreamType(&streamType);
                            if (0 != status) {
                                PAL_ERR(LOG_TAG, "getStreamType failed with status = %d", status);
                                continue;
                            }
                            if ((streamType == PAL_STREAM_LOW_LATENCY) ||
                                (streamType == PAL_STREAM_ULTRA_LOW_LATENCY) ||
                                (streamType == PAL_STREAM_VOIP_RX) ||
                                (streamType == PAL_STREAM_PCM_OFFLOAD) ||
                                (streamType == PAL_STREAM_DEEP_BUFFER) ||
                                (streamType == PAL_STREAM_SPATIAL_AUDIO) ||
                                (streamType == PAL_STREAM_COMPRESSED) ||
#ifdef SEC_AUDIO_EARLYDROP_PATCH
                                ((a2dp_dattr.id == PAL_DEVICE_OUT_BLUETOOTH_BLE) && (streamType == PAL_STREAM_VOICE_CALL)) ||
#endif
                                (streamType == PAL_STREAM_GENERIC)) {
                                if ((*sIter)->suspendedOutDevIds.empty()) {
                                    (*sIter)->suspendedOutDevIds.
                                        push_back(PAL_DEVICE_OUT_BLUETOOTH_SCO);
                                    PAL_DBG(LOG_TAG, "a2dp resumed, \
                                        mark sco streams as to route them later");
                                }
                            }
                        }
                    }
                    mActiveStreamMutex.unlock();
                }
            }

            status = a2dp_dev->setDeviceParameter(param_id, param_payload);
            mResourceManagerMutex.lock();
            if (status) {
                PAL_ERR(LOG_TAG, "set Parameter %d failed\n", param_id);
                goto exit;
            }
        }
        break;
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
        case PAL_PARAM_ID_BT_A2DP_SBM_CONFIG:
        case PAL_PARAM_ID_BT_A2DP_DELAY_REPORT:
#endif
        case PAL_PARAM_ID_BT_A2DP_TWS_CONFIG:
        case PAL_PARAM_ID_BT_A2DP_LC3_CONFIG:
        {
            std::shared_ptr<Device> dev = nullptr;
            struct pal_device dattr;

            if (isDeviceAvailable(PAL_DEVICE_OUT_BLUETOOTH_A2DP)) {
                dattr.id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
            } else if (isDeviceAvailable(PAL_DEVICE_OUT_BLUETOOTH_BLE)) {
                dattr.id = PAL_DEVICE_OUT_BLUETOOTH_BLE;
            } else if (isDeviceAvailable(PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST)) {
                dattr.id = PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST;
            }
            dev = Device::getInstance(&dattr, rm);
            if (dev) {
                status = dev->setDeviceParameter(param_id, param_payload);
                if (status) {
                    PAL_ERR(LOG_TAG, "set Parameter %d failed\n", param_id);
                    goto exit;
                }
            }
        }
        break;
        case PAL_PARAM_ID_GAIN_LVL_CAL:
        {
            struct pal_device dattr;
            Stream *stream = NULL;
            std::vector<Stream*> activestreams;
            struct pal_stream_attributes sAttr;
            Session *session = NULL;

            pal_param_gain_lvl_cal_t *gain_lvl_cal = (pal_param_gain_lvl_cal_t *) param_payload;
            if (payload_size != sizeof(pal_param_gain_lvl_cal_t)) {
                PAL_ERR(LOG_TAG, "incorrect payload size : expected (%zu), received(%zu)",
                      sizeof(pal_param_gain_lvl_cal_t), payload_size);
                status = -EINVAL;
                goto exit;
            }

            for (int i = 0; i < active_devices.size(); i++) {
                int deviceId = active_devices[i].first->getSndDeviceId();
                status = active_devices[i].first->getDeviceAttributes(&dattr);
                if (0 != status) {
                   PAL_ERR(LOG_TAG,"getDeviceAttributes Failed");
                   goto exit;
                }
                if ((PAL_DEVICE_OUT_SPEAKER == deviceId) ||
                    (PAL_DEVICE_OUT_WIRED_HEADSET == deviceId) ||
                    (PAL_DEVICE_OUT_WIRED_HEADPHONE == deviceId)) {
                    status = getActiveStream_l(activestreams, active_devices[i].first);
                    if ((0 != status) || (activestreams.size() == 0)) {
                       PAL_ERR(LOG_TAG, "no other active streams found");
                       status = -EINVAL;
                       goto exit;
                    }

                    stream = static_cast<Stream *>(activestreams[0]);
                    stream->getStreamAttributes(&sAttr);
                    if ((sAttr.direction == PAL_AUDIO_OUTPUT) &&
                        ((sAttr.type == PAL_STREAM_LOW_LATENCY) ||
                        (sAttr.type == PAL_STREAM_DEEP_BUFFER) ||
                        (sAttr.type == PAL_STREAM_COMPRESSED) ||
#ifdef SEC_AUDIO_EARLYDROP_PATCH
                        (sAttr.type == PAL_STREAM_GENERIC) ||
#endif
                        (sAttr.type == PAL_STREAM_PCM_OFFLOAD))) {
                        stream->setGainLevel(gain_lvl_cal->level);
                        stream->getAssociatedSession(&session);
                        status = session->setConfig(stream, CALIBRATION, TAG_DEVICE_PP_MBDRC);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG, "session setConfig failed with status %d", status);
                            goto exit;
                        }
                    }
                }
            }
        }
        break;
#ifdef SEC_AUDIO_LOOPBACK_TEST
        case PAL_PARAM_ID_LOOPBACK_MODE:
        {
            pal_param_loopback_t* param_device_loopback =
                                   (pal_param_loopback_t*) param_payload;
            if (payload_size == sizeof(pal_param_loopback_t)) {
                vsidInfo.loopback_mode = param_device_loopback->loopback_mode;
            } else {
                PAL_ERR(LOG_TAG,"Incorrect size : expected (%zu), received(%zu)",
                        sizeof(pal_param_loopback_t), payload_size);
                status = -EINVAL;
                goto exit;
            }
        }
        break;
#endif
#if defined(SEC_AUDIO_DUAL_SPEAKER) || defined(SEC_AUDIO_SCREEN_MIRRORING) // { SUPPORT_VOIP_VIA_SMART_VIEW
        case PAL_PARAM_ID_SPEAKER_STATUS:
        {
            pal_param_speaker_status_t * param_speaker_status =
                                   (pal_param_speaker_status_t*) param_payload;
            if (payload_size == sizeof(pal_param_speaker_status_t)) {
                struct pal_device dattr;
                dattr.id = PAL_DEVICE_OUT_SPEAKER;
                std::shared_ptr<Device> dev = nullptr;
                dev = Device::getInstance(&dattr, rm);
                if (!dev) {
                    PAL_ERR(LOG_TAG, "Device getInstance failed");
                    status = -ENODEV;
                    goto exit;
                }
                status = dev->setParameter(param_id, param_payload);
            } else {
                PAL_ERR(LOG_TAG, "Incorrect size : expected (%zu), received(%zu)",
                        sizeof(pal_param_speaker_status_t), payload_size);
                status = -EINVAL;
                goto exit;
            }
        }
        break;
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW
#ifdef SEC_AUDIO_SPEAKER_CALIBRATION
        case PAL_PARAM_ID_SEPAKER_AMP_RUN_CAL:
        {
            pal_param_cal_trigger_t * param_cal_trigger =
                                   (pal_param_cal_trigger_t*) param_payload;
            if (payload_size == sizeof(pal_param_cal_trigger_t)) {
                struct pal_device dattr;
                dattr.id = PAL_DEVICE_OUT_SPEAKER;
                std::shared_ptr<Device> dev = nullptr;
                dev = Device::getInstance(&dattr, rm);
                if (!dev) {
                    PAL_ERR(LOG_TAG, "Device getInstance failed");
                    status = -ENODEV;
                    goto exit;
                }
                status = dev->setParameter(param_id, param_payload);
            } else {
                PAL_ERR(LOG_TAG, "Incorrect size : expected (%zu), received(%zu)",
                        sizeof(pal_param_cal_trigger_t), payload_size);
                status = -EINVAL;
                goto exit;
            }
        }
        break;
#endif
#ifdef SEC_AUDIO_FMRADIO
        case PAL_PARAM_ID_FMRADIO_USB_GAIN:
        {
            pal_param_fmradio_usb_gain_t * param_fmradio_usb_gain =
                                   (pal_param_fmradio_usb_gain_t*) param_payload;
            if (payload_size == sizeof(pal_param_fmradio_usb_gain_t)) {
                struct pal_device dattr;
                dattr.id = PAL_DEVICE_OUT_USB_HEADSET;
                std::shared_ptr<Device> dev = nullptr;
                dev = Device::getInstance(&dattr, rm);
                if (!dev) {
                    PAL_ERR(LOG_TAG, "Device getInstance failed");
                    status = -ENODEV;
                    goto exit;
                }
                status = dev->setParameter(param_id, param_payload);
            } else {
                PAL_ERR(LOG_TAG, "Incorrect size : expected (%zu), received(%zu)",
                        sizeof(pal_param_fmradio_usb_gain_t), payload_size);
                status = -EINVAL;
                goto exit;
            }
        }
        break;
#endif
#ifdef SEC_AUDIO_SUPPORT_HAPTIC_PLAYBACK
        case PAL_PARAM_ID_HAPTIC_SOURCE:
        {
            pal_param_haptic_source_t * param_haptic_source =
                                   (pal_param_haptic_source_t*) param_payload;
            if (payload_size == sizeof(pal_param_haptic_source_t)) {
                struct pal_device dattr;
                dattr.id = PAL_DEVICE_OUT_HAPTICS_DEVICE;
                std::shared_ptr<Device> dev = nullptr;
                dev = Device::getInstance(&dattr, rm);
                if (!dev) {
                    PAL_ERR(LOG_TAG, "Device getInstance failed");
                    status = -ENODEV;
                    goto exit;
                }
                status = dev->setParameter(param_id, param_payload);
            } else {
                PAL_ERR(LOG_TAG, "Incorrect size : expected (%zu), received(%zu)",
                        sizeof(pal_param_haptic_source_t), payload_size);
                status = -EINVAL;
                goto exit;
            }
        }
        break;
#endif
        case PAL_PARAM_ID_PROXY_CHANNEL_CONFIG:
        {
            pal_param_proxy_channel_config_t *param_proxy =
                (pal_param_proxy_channel_config_t *)param_payload;
            rm->num_proxy_channels = param_proxy->num_proxy_channels;
        }
        break;
        case PAL_PARAM_ID_HAPTICS_INTENSITY:
        {
            if (!ResourceManager::isHapticsthroughWSA) {
                pal_param_haptics_intensity *hInt =
                       (pal_param_haptics_intensity *)param_payload;
                PAL_DBG(LOG_TAG, "Haptics Intensity %d", hInt->intensity);
                char mixer_ctl_name[128] =  "Haptics Amplitude Step";
                struct mixer_ctl *ctl = mixer_get_ctl_by_name(audio_hw_mixer, mixer_ctl_name);
                if (!ctl) {
                    PAL_ERR(LOG_TAG, "Could not get ctl for mixer cmd - %s", mixer_ctl_name);
                    status = -EINVAL;
                    goto exit;
                }
                mixer_ctl_set_value(ctl, 0, hInt->intensity);
            }
        }
        break;
        case PAL_PARAM_ID_HAPTICS_VOLUME:
        {
            std::list<Stream*>::iterator sIter;
            pal_stream_attributes st_attr;
            for(sIter = mActiveStreams.begin(); sIter != mActiveStreams.end(); sIter++) {
                (*sIter)->getStreamAttributes(&st_attr);
                if (st_attr.type == PAL_STREAM_HAPTICS) {
                    status = (*sIter)->setVolume((struct pal_volume_data *)param_payload);
                    if (status) {
                        PAL_ERR(LOG_TAG, "Failed to set volume for haptics");
                        goto exit;
                    }
                }
            }
        }
        break;
        case PAL_PARAM_ID_BT_A2DP_CAPTURE_SUSPENDED:
        {
            std::shared_ptr<Device> a2dp_dev = nullptr;
            struct pal_device a2dp_dattr = {};
            pal_param_bta2dp_t* current_param_bt_a2dp = nullptr;
            pal_param_bta2dp_t* param_bt_a2dp = nullptr;
            bool skip_switch = false;
#ifdef SEC_AUDIO_BLE_OFFLOAD
            bool is_a2dp_rx_suspended = false;
#endif

            mResourceManagerMutex.unlock();
            param_bt_a2dp = (pal_param_bta2dp_t*)param_payload;
            a2dp_dattr.id = param_bt_a2dp->dev_id;

            a2dp_dev = Device::getInstance(&a2dp_dattr, rm);
            if (!a2dp_dev) {
                PAL_ERR(LOG_TAG, "failed to get a2dp/ble capture instance");
                status = -ENODEV;
                goto exit_no_unlock;
            }

            a2dp_dev->getDeviceParameter(param_id, (void**)&current_param_bt_a2dp);
            /* If device is already suspended from framework, ignore suspend/resume
             * which is sent via reconfig_cb. Honouring the param in such scenario
             * will lead to incorrect stream state.
             */
            if (current_param_bt_a2dp->a2dp_capture_suspended &&
                current_param_bt_a2dp->is_suspend_setparam &&
                !param_bt_a2dp->is_suspend_setparam) {
                PAL_INFO(LOG_TAG, "suspend/resume from reconfig_cb ignored");
                goto exit_no_unlock;
            }

            if (current_param_bt_a2dp->a2dp_capture_suspended ==
                    param_bt_a2dp->a2dp_capture_suspended) {
                PAL_INFO(LOG_TAG, "a2dp/ble already in requested state, ignoring");
                goto exit_no_unlock;
            }

            if((!isDeviceAvailable(param_bt_a2dp->dev_id)) ||
               (param_bt_a2dp->is_suspend_setparam && param_bt_a2dp->is_in_call))
            {
                skip_switch = true;
            }
#ifdef SEC_AUDIO_BLE_OFFLOAD
            else if (a2dp_dattr.id == PAL_DEVICE_IN_BLUETOOTH_BLE) {
                PAL_DBG(LOG_TAG, "by ble suspend false, not to switch sco to dummy device");
                skip_switch = true;
            }
#endif

            if (ResourceManager::isDummyDevEnabled) {
                if (!skip_switch && param_bt_a2dp->a2dp_capture_suspended == false) {
                    struct pal_device sco_rx_dattr = {};
                    std::shared_ptr<Device> sco_rx_dev = nullptr;
                    struct pal_device out_dummy_dattr = {};
                    std::vector<Stream*> activestreams;
                    struct pal_device sco_tx_dattr;
                    std::shared_ptr<Device> sco_tx_dev = nullptr;
                    std::vector<Stream*>::iterator sIter;
                    pal_stream_type_t streamType;

                    /* Handle bt sco out running usecase */
                    sco_rx_dattr.id = PAL_DEVICE_OUT_BLUETOOTH_SCO;
                    if (isDeviceAvailable(sco_rx_dattr.id)) {
                        out_dummy_dattr.id = PAL_DEVICE_OUT_DUMMY;
                        sco_rx_dev = Device::getInstance(&sco_rx_dattr, rm);
                        mActiveStreamMutex.lock();
                        getActiveStream_l(activestreams, sco_rx_dev);
                        mActiveStreamMutex.unlock();
                        if (activestreams.size() > 0) {
                            /* Mark streams over OUT_SCO, so as to give them chance
                             * to resume over A2DP/BLE if a2dpSuspend=false is
                             * received at later stage.
                             */
                            for (sIter = activestreams.begin();
                                     sIter != activestreams.end(); sIter++) {
                                (*sIter)->suspendedOutDevIds.clear();
                                if (isDeviceAvailable(PAL_DEVICE_OUT_BLUETOOTH_A2DP))
                                    (*sIter)->suspendedOutDevIds.
                                                  push_back(PAL_DEVICE_OUT_BLUETOOTH_A2DP);
                                else if (isDeviceAvailable(PAL_DEVICE_OUT_BLUETOOTH_BLE))
                                    (*sIter)->suspendedOutDevIds.
                                                  push_back(PAL_DEVICE_OUT_BLUETOOTH_BLE);
                            }
                            PAL_DBG(LOG_TAG, "a2dp resumed, switch bt sco out to out_dummy device");
                            getDeviceConfig(&out_dummy_dattr, NULL);
                            rm->forceDeviceSwitch(sco_rx_dev, &out_dummy_dattr);
                        }
                    }

                    mActiveStreamMutex.lock();
                    /* Handle bt sco running usecase */
                    sco_tx_dattr.id = PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
                    if (isDeviceAvailable(sco_tx_dattr.id)) {
                        sco_tx_dev = Device::getInstance(&sco_tx_dattr, rm);
                        getActiveStream_l(activestreams, sco_tx_dev);
                        for (sIter = activestreams.begin(); sIter != activestreams.end(); sIter++) {
                            status = (*sIter)->getStreamType(&streamType);
                            if (0 != status) {
                                PAL_ERR(LOG_TAG, "getStreamType failed with status = %d", status);
                                continue;
                            }
                            if ((streamType == PAL_STREAM_VOIP_TX) ||
                                (streamType == PAL_STREAM_DEEP_BUFFER)) {
                                (*sIter)->suspendedInDevIds.clear();
                                (*sIter)->suspendedInDevIds.push_back(a2dp_dattr.id);
                                PAL_DBG(LOG_TAG, "a2dp resumed, mark sco streams as to route them later");
                            }
                        }
                    }
                    mActiveStreamMutex.unlock();
                }
            } else {
                if (!skip_switch && param_bt_a2dp->a2dp_capture_suspended == false) {
                    /* Handle bt sco out running usecase */
                    struct pal_device sco_rx_dattr;
                    struct pal_stream_attributes sAttr;
                    Stream* stream = NULL;
                    std::vector<Stream*> activestreams;
                    struct pal_device sco_tx_dattr;
                    std::shared_ptr<Device> sco_tx_dev = nullptr;
                    std::vector<Stream*>::iterator sIter;
                    pal_stream_type_t streamType;

                    mActiveStreamMutex.lock();
                    sco_rx_dattr.id = PAL_DEVICE_OUT_BLUETOOTH_SCO;
                    PAL_DBG(LOG_TAG, "a2dp resumed, switch bt sco rx to speaker");
                    if (isDeviceAvailable(sco_rx_dattr.id)) {
                        struct pal_device speaker_dattr;
                        std::shared_ptr<Device> sco_rx_dev = nullptr;

                        speaker_dattr.id = PAL_DEVICE_OUT_SPEAKER;
                        sco_rx_dev = Device::getInstance(&sco_rx_dattr, rm);
                        getActiveStream_l(activestreams, sco_rx_dev);
                        if (activestreams.size() > 0) {
                            /* Mark streams over OUT_SCO, so as to give them chance
                             * to resume over A2DP/BLE if a2dpSuspend=false is
                             * received at later stage.
                             */
                            for (sIter = activestreams.begin();
                                     sIter != activestreams.end(); sIter++) {
                                (*sIter)->suspendedOutDevIds.clear();
                                if (isDeviceAvailable(PAL_DEVICE_OUT_BLUETOOTH_A2DP))
                                    (*sIter)->suspendedOutDevIds.
                                                  push_back(PAL_DEVICE_OUT_BLUETOOTH_A2DP);
                                else if (isDeviceAvailable(PAL_DEVICE_OUT_BLUETOOTH_BLE))
                                    (*sIter)->suspendedOutDevIds.
                                                  push_back(PAL_DEVICE_OUT_BLUETOOTH_BLE);
                            }
                            stream = static_cast<Stream*>(activestreams[0]);
                            stream->getStreamAttributes(&sAttr);
                            getDeviceConfig(&speaker_dattr, &sAttr);
                            mActiveStreamMutex.unlock();
                            rm->forceDeviceSwitch(sco_rx_dev, &speaker_dattr);
                            mActiveStreamMutex.lock();
                        }
                    }

                    /* Handle bt sco running usecase */
                    sco_tx_dattr.id = PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
                    if (isDeviceAvailable(sco_tx_dattr.id)) {
                        sco_tx_dev = Device::getInstance(&sco_tx_dattr, rm);
                        getActiveStream_l(activestreams, sco_tx_dev);
                        for (sIter = activestreams.begin(); sIter != activestreams.end(); sIter++) {
                            status = (*sIter)->getStreamType(&streamType);
                            if (0 != status) {
                                PAL_ERR(LOG_TAG, "getStreamType failed with status = %d", status);
                                continue;
                            }
                            if ((streamType == PAL_STREAM_VOIP_TX) ||
                                 (streamType == PAL_STREAM_DEEP_BUFFER)) {
                                (*sIter)->suspendedInDevIds.clear();
                                (*sIter)->suspendedInDevIds.push_back(a2dp_dattr.id);
                                PAL_DBG(LOG_TAG, "a2dp resumed, mark sco streams as to route them later");
                            }
                        }
                    }
                    mActiveStreamMutex.unlock();
                }
            }

            status = a2dp_dev->setDeviceParameter(param_id, param_payload);
            mResourceManagerMutex.lock();
            if (status) {
                PAL_ERR(LOG_TAG, "set Parameter %d failed\n", param_id);
                goto exit;
            }
        }
        break;
        case PAL_PARAM_ID_MSPP_LINEAR_GAIN:
        {
            struct pal_device dattr;
            Stream *stream = NULL;
            std::vector<Stream*> activestreams;
            struct pal_stream_attributes sAttr;
            Session *session = NULL;

            pal_param_mspp_linear_gain_t *linear_gain = (pal_param_mspp_linear_gain_t *) param_payload;
            if (payload_size != sizeof(pal_param_mspp_linear_gain_t)) {
                PAL_ERR(LOG_TAG, "incorrect payload size : expected (%zu), received(%zu)",
                      sizeof(pal_param_mspp_linear_gain_t), payload_size);
                status = -EINVAL;
                goto exit;
            }
            PAL_DBG(LOG_TAG, "set mspp linear gain (0x%x)", linear_gain->gain);
            rm->linear_gain.gain =  linear_gain->gain;
            for (int i = 0; i < active_devices.size(); i++) {
                int deviceId = active_devices[i].first->getSndDeviceId();
                status = active_devices[i].first->getDeviceAttributes(&dattr);
                if (0 != status) {
                   PAL_ERR(LOG_TAG,"getDeviceAttributes Failed");
                   goto exit;
                }
                if (PAL_DEVICE_OUT_SPEAKER == deviceId && !strcmp(dattr.custom_config.custom_key, "mspp")) {
                    status = getActiveStream_l(activestreams, active_devices[i].first);
                    if ((0 != status) || (activestreams.size() == 0)) {
                       PAL_INFO(LOG_TAG, "no other active streams found");
                       status = 0;
                       goto exit;
                    }

                    for (int j = 0; j < activestreams.size(); j++) {
                       stream = static_cast<Stream *>(activestreams[j]);
                       stream->getStreamAttributes(&sAttr);
                       if ((sAttr.direction == PAL_AUDIO_OUTPUT) &&
                           ((sAttr.type == PAL_STREAM_LOW_LATENCY) ||
                           (sAttr.type == PAL_STREAM_DEEP_BUFFER) ||
                           (sAttr.type == PAL_STREAM_COMPRESSED) ||
                           (sAttr.type == PAL_STREAM_PCM_OFFLOAD))) {
                           stream->getAssociatedSession(&session);
                           status = session->setParameters(stream, TAG_MODULE_MSPP,
                               param_id, param_payload);
                           if (0 != status) {
                               PAL_ERR(LOG_TAG, "session setConfig failed. stream: %d, status: %d",
                                      sAttr.type, status);
                           }
                       }
                    }
                }
            }
        }
        break;
        case PAL_PARAM_ID_SET_SOURCE_METADATA:
        {
            struct pal_device dattr;
            std::shared_ptr<Device> dev = nullptr;
            if (isDeviceAvailable(PAL_DEVICE_OUT_BLUETOOTH_BLE)) {
                dattr.id = PAL_DEVICE_OUT_BLUETOOTH_BLE;
                dev = Device::getInstance(&dattr, rm);
                if (!dev) {
                    PAL_ERR(LOG_TAG, "Device getInstance failed");
                    goto exit;
                }
                mResourceManagerMutex.unlock();
                dev->setDeviceParameter(param_id, param_payload);
                mResourceManagerMutex.lock();
            }

            if (isDeviceAvailable(PAL_DEVICE_OUT_BLUETOOTH_A2DP)) {
                dattr.id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
                dev = Device::getInstance(&dattr, rm);
                if (!dev) {
                    PAL_ERR(LOG_TAG, "Device getInstance failed");
                    goto exit;
                }
                mResourceManagerMutex.unlock();
                dev->setDeviceParameter(param_id, param_payload);
                mResourceManagerMutex.lock();
            }
        }
        break;
        case PAL_PARAM_ID_SET_SINK_METADATA:
        {
            struct pal_device dattr;
            std::shared_ptr<Device> dev = nullptr;
            if (isDeviceAvailable(PAL_DEVICE_IN_BLUETOOTH_BLE)) {
                dattr.id = PAL_DEVICE_IN_BLUETOOTH_BLE;
            } else {
                PAL_VERBOSE(LOG_TAG, "BLE device is unavailable");
                goto exit;
            }

            dev = Device::getInstance(&dattr, rm);
            if (!dev) {
                PAL_ERR(LOG_TAG, "Device getInstance failed");
                goto exit;
            }
            PAL_INFO(LOG_TAG, "PAL_PARAM_ID_SET_SINK_METADATA device setparam");
            mResourceManagerMutex.unlock();
            dev->setDeviceParameter(param_id, param_payload);
            mResourceManagerMutex.lock();
        }
        break;
        case PAL_PARAM_ID_LATENCY_MODE:
        {
            struct pal_device dattr;
            std::shared_ptr<Device> dev = nullptr;
            if (isDeviceAvailable(((pal_param_latency_mode_t*)param_payload)->dev_id)) {
                dattr.id = ((pal_param_latency_mode_t*)param_payload)->dev_id;
            } else {
                goto exit;
            }

            dev = Device::getInstance(&dattr, rm);
            if (!dev) {
                PAL_ERR(LOG_TAG, "Device getInstance failed");
                goto exit;
            }
            status = dev->setDeviceParameter(param_id, param_payload);
            if (status) {
                PAL_ERR(LOG_TAG, "set Parameter %d failed", param_id);
                goto exit;
            }
        }
        break;
        case PAL_PARAM_ID_RESOURCES_AVAILABLE:
        {
            pal_param_resources_available_t *resources_avail =
                (pal_param_resources_available_t *)param_payload;
            if (resources_avail) {
                onResourceAvailCb =
                    (SoundTriggerOnResourceAvailableCallback)resources_avail->callback;
                onResourceAvailCookie = resources_avail->cookie;
                PAL_VERBOSE(LOG_TAG, "setParameter onResourceAvailCb %pk"
                    " onResourceAvailCookie %pk", onResourceAvailCb, onResourceAvailCookie);
            } else {
                PAL_ERR(LOG_TAG, "Invalid ST resource payload");
                status = -EINVAL;
            }
        }
        break;
#ifdef SEC_AUDIO_BLE_OFFLOAD
        case PAL_PARAM_ID_BT_A2DP_SUSPENDED_FOR_BLE:
        {
            // refer PAL_PARAM_ID_BT_A2DP_SUSPENDED, need to use same mutex
            pal_param_bta2dp_t* pal_param_bta2dp = (pal_param_bta2dp_t*) param_payload;

            if (payload_size == sizeof(pal_param_bta2dp_t)) {
                struct pal_device dattr;
                std::shared_ptr<Device> dev = nullptr;
                dattr.id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
                dev = Device::getInstance(&dattr, rm);
                if (!dev) {
                    PAL_ERR(LOG_TAG, "Device getInstance failed");
                    goto exit;
                }
                PAL_DBG(LOG_TAG,"set a2dp_suspended_for_ble %d ",
                                 pal_param_bta2dp->a2dp_suspended_for_ble);

                mResourceManagerMutex.unlock();
                dev->setDeviceParameter(param_id, param_payload);
                mResourceManagerMutex.lock();
            } else {
                PAL_ERR(LOG_TAG, "Incorrect size : expected (%zu), received(%zu)",
                        sizeof(pal_param_bta2dp_t), payload_size);
                status = -EINVAL;
                goto exit;
            }
        }
        break;
#endif
#ifdef SEC_AUDIO_AMP_SDHMS
        case PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_RCV:
        case PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_SPK:
        {
            pal_param_amp_ssrm_t *param_amp_ssrm =
                                   (pal_param_amp_ssrm_t*) param_payload;
            if (payload_size == sizeof(pal_param_amp_ssrm_t)) {
                struct pal_device dattr;
                dattr.id = PAL_DEVICE_OUT_SPEAKER;
                std::shared_ptr<Device> dev = nullptr;
                dev = Device::getInstance(&dattr, rm);
                if (!dev) {
                    PAL_ERR(LOG_TAG, "Device getInstance failed");
                    status = -ENODEV;
                    goto exit;
                }
                status = dev->setParameter(param_id, param_payload);
            } else {
                PAL_ERR(LOG_TAG, "Incorrect size : expected (%zu), received(%zu)",
                        sizeof(pal_param_amp_ssrm_t), payload_size);
                status = -EINVAL;
                goto exit;
            }
        }
        break;
#endif
#ifdef ENABLE_TAS_SPK_PROT
        case PAL_TISA_PARAM_GEN_SETPARAM: {
            struct pal_device dattr;
            dattr.id = PAL_DEVICE_OUT_SPEAKER;
            std::shared_ptr<Device> dev = nullptr;

            dev = Device::getInstance(&dattr, rm);
            if (dev) {
                PAL_DBG(LOG_TAG, "TI-SmartPA: calling setparam to devie");
                dev->setParameter(PAL_TISA_PARAM_GEN_SETPARAM, param_payload);
            }
            else {
                PAL_DBG(LOG_TAG, "TI-SmartPA: Unable to get speaker instance");
            }
        }
        break;
#endif
#ifdef SEC_AUDIO_BARGEIN_MODE
        case PAL_PARAM_ID_BARGEIN_MODE:
        {
            pal_param_bargein_mode_t *param_bargein_mode =
                                   (pal_param_bargein_mode_t*) param_payload;
            if (payload_size == sizeof(pal_param_bargein_mode_t)) {
                bargein_mode = param_bargein_mode->mode;
            } else {
                PAL_ERR(LOG_TAG, "Incorrect size : expected (%zu), received(%zu)",
                        sizeof(pal_param_bargein_mode_t), payload_size);
                status = -EINVAL;
                goto exit;
            }
        }
        break;
#endif
#ifdef SEC_AUDIO_CALL_VOIP
        case PAL_PARAM_ID_VOIP_MIC_MODE:
        {
            struct pal_device dattr;
            Stream *stream = NULL;
            std::vector<Stream*> activestreams;
            struct pal_stream_attributes sAttr;
            Session *session = NULL;

            pal_param_voip_mic_mode_t *param_mid_mode =
                                   (pal_param_voip_mic_mode_t*) param_payload;

            if (payload_size == sizeof(pal_param_voip_mic_mode_t)) {
                for (int i = 0; i < active_devices.size(); i++) {
                    int deviceId = active_devices[i].first->getSndDeviceId();
                    status = active_devices[i].first->getDeviceAttributes(&dattr);
                    if (0 != status) {
                       PAL_ERR(LOG_TAG,"getDeviceAttributes Failed");
                       goto exit;
                    }
                    if ((PAL_DEVICE_OUT_HANDSET == deviceId) ||
                        (PAL_DEVICE_OUT_SPEAKER == deviceId) ||
                        (PAL_DEVICE_IN_HANDSET_MIC == deviceId) ||
                        (PAL_DEVICE_IN_SPEAKER_MIC == deviceId)) {
                        status = getActiveStream_l(activestreams, active_devices[i].first);
                        if ((0 != status) || (activestreams.size() == 0)) {
                           PAL_ERR(LOG_TAG, "no other active streams found");
                           status = -EINVAL;
                           goto exit;
                        }

                        for (int j = 0; j < activestreams.size(); j++) {
                            stream = static_cast<Stream *>(activestreams[j]);
                            stream->getStreamAttributes(&sAttr);
                            if ((sAttr.type == PAL_STREAM_VOIP_RX) ||
                                (sAttr.type == PAL_STREAM_VOIP_TX)) {
                                stream->setMicMode(param_mid_mode);
                                stream->getAssociatedSession(&session);
                                status = session->setConfig(stream, CALIBRATION, TAG_MIC_MODE_CUSTOM);
                                if (0 != status) {
                                    PAL_ERR(LOG_TAG, "session setConfig failed. stream: %d, status: %d",
                                        sAttr.type, status);
                                }
                            }
                        }
                    }
                }
            } else {
                PAL_ERR(LOG_TAG, "Incorrect size : expected (%zu), received(%zu)",
                        sizeof(pal_param_voip_mic_mode_t), payload_size);
                status = -EINVAL;
                goto exit;
            }
            PAL_DBG(LOG_TAG, "mic_mode:%d", param_mid_mode->mic_mode);
        }
        break;
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
        case PAL_PARAM_ID_INTERPRETER_MODE:
        {
            pal_param_interpreter_mode_t *param_interpreter_mode =
                                   (pal_param_interpreter_mode_t*) param_payload;
            if (payload_size == sizeof(pal_param_interpreter_mode_t)) {
                interpreter_mode = param_interpreter_mode->mode;
            } else {
                PAL_ERR(LOG_TAG, "Incorrect size : expected (%zu), received(%zu)",
                        sizeof(pal_param_interpreter_mode_t), payload_size);
                status = -EINVAL;
                goto exit;
            }
        }
        break;
#endif
#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO
        case PAL_PARAM_ID_BTMIX_RECORD:
        {
            pal_param_btmix_record_t *param_btmix_record =
                                   (pal_param_btmix_record_t*) param_payload;
            if (payload_size == sizeof(pal_param_btmix_record_t)) {
                isBtmixRecord = param_btmix_record->enable;
            } else {
                PAL_ERR(LOG_TAG, "Incorrect size : expected (%zu), received(%zu)",
                        sizeof(pal_param_btmix_record_t), payload_size);
                status = -EINVAL;
                goto exit;
            }
            PAL_DBG(LOG_TAG, "isBtmixRecord:%d", isBtmixRecord);
        }
        break;
#endif
        default:
            PAL_ERR(LOG_TAG, "Unknown ParamID:%d", param_id);
            break;
    }

exit:
    mResourceManagerMutex.unlock();
exit_no_unlock:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}


int ResourceManager::setParameter(uint32_t param_id, void *param_payload,
                                  size_t payload_size __unused,
                                  pal_device_id_t pal_device_id,
                                  pal_stream_type_t pal_stream_type)
{
    int status = -EINVAL;

    PAL_DBG(LOG_TAG, "Enter param id: %d", param_id);

    switch (param_id) {
        case PAL_PARAM_ID_UIEFFECT:
        {
#ifdef SEC_AUDIO_COMMON
            pal_param_payload *pal_param = (pal_param_payload *)param_payload;
            effect_pal_payload_t *effectPayload = (effect_pal_payload_t *)pal_param->payload;
            pal_effect_custom_payload_t *effectCustomPayload = nullptr;
            effectCustomPayload = (pal_effect_custom_payload_t *)effectPayload->payload;
            PAL_INFO(LOG_TAG, "PAL_PARAM_ID_UIEFFECT %s(0x%x)",
                                customPayloadParamName.at(effectCustomPayload->paramId).c_str(),
                                effectCustomPayload->paramId);
#endif
            bool match = false;
            lockActiveStream();
            std::list<Stream*>::iterator sIter;
            for(sIter = mActiveStreams.begin(); sIter != mActiveStreams.end();
                    sIter++) {
                if ((*sIter) != NULL) {
#ifdef SEC_AUDIO_COMMON
                    match = (*sIter)->checkStreamEffectMatch(pal_device_id,
                                                            pal_stream_type,
                                                            effectCustomPayload->paramId);
#else
                    match = (*sIter)->checkStreamMatch(pal_device_id,
                                                       pal_stream_type);
#endif
                    if (match) {
                        if (increaseStreamUserCounter(*sIter) < 0)
                            continue;
                        unlockActiveStream();
                        status = (*sIter)->setEffectParameters(param_payload);
                        lockActiveStream();
                        decreaseStreamUserCounter(*sIter);
                        if (status) {
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
                            PAL_INFO(LOG_TAG, "failed to set param for pal_device_id=%x stream_type=%x",
                                   pal_device_id, pal_stream_type);
#else
                            PAL_ERR(LOG_TAG, "failed to set param for pal_device_id=%x stream_type=%x",
                                   pal_device_id, pal_stream_type);
#endif
                        }
                    }
                } else {
                    PAL_ERR(LOG_TAG, "There is no active stream.");
                }
            }
            unlockActiveStream();
        }
        break;
        default:
            PAL_ERR(LOG_TAG, "Unknown ParamID:%d", param_id);
            break;
    }

    PAL_DBG(LOG_TAG, "Exit status: %d",status);
    return status;
}


int ResourceManager::rwParameterACDB(uint32_t paramId, void *paramPayload,
                 size_t payloadSize  __unused, pal_device_id_t palDeviceId,
                 pal_stream_type_t palStreamType, uint32_t sampleRate,
                 uint32_t instanceId, bool isParamWrite, bool isPlay)
{
    int status = -EINVAL;
    StreamACDB *s = NULL;
    struct pal_stream_attributes sattr;
    struct pal_device dattr;
    bool match = false;
    uint32_t matchCount = 0;
    std::list<Stream*>::iterator sIter;

    PAL_DBG(LOG_TAG, "Enter: device=%d type=%d rate=%d instance=%d is_param_write=%d\n",
            palDeviceId, palStreamType, sampleRate,
            instanceId, isParamWrite);

    switch (paramId) {
        case PAL_PARAM_ID_UIEFFECT:
        {
            /*
             * set default stream type (low latency) for device-only effect.
             * the instance is shared by streams.
             */
            if (palStreamType == PAL_STREAM_GENERIC) {
                palStreamType = PAL_STREAM_LOW_LATENCY;
                PAL_INFO(LOG_TAG, "change PAL stream from %d to %d for device effect",
                            PAL_STREAM_GENERIC, palStreamType);
            }

            /*
             * set default device (speaker) for stream-only effect.
             * the instance is shared by devices.
             */
            if (palDeviceId == PAL_DEVICE_NONE) {
                if (isPlay)
                    palDeviceId = PAL_DEVICE_OUT_SPEAKER;
                else
                    palDeviceId = PAL_DEVICE_IN_HANDSET_MIC;

                PAL_INFO(LOG_TAG, "change PAL device id from %d to %d for stream effect",
                            PAL_DEVICE_NONE, palDeviceId);
            }

            sattr.type = palStreamType;
            sattr.out_media_config.sample_rate = sampleRate;
            sattr.direction = PAL_AUDIO_OUTPUT;
            dattr.id = palDeviceId;
            s = new StreamACDB(&sattr, &dattr, instanceId, getInstance());
            if (!s) {
                status = -EINVAL;
                PAL_ERR(LOG_TAG, "stream creation failed status %d", status);
                goto error;
            }

            status = s->rwACDBParam(palDeviceId, palStreamType, sampleRate,
                instanceId, paramPayload, isParamWrite);

            delete s;
        }
        break;
        default:
            PAL_ERR(LOG_TAG, "Unknown ParamID:%d", paramId);
            break;
    }

error:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);

    return status;
}

int ResourceManager::handleScreenStatusChange(pal_param_screen_state_t screen_state)
{
    int status = 0;

    if (screen_state_ != screen_state.screen_state) {
        if (screen_state.screen_state == false) {
            /* have appropriate streams transition to LPI */
            PAL_VERBOSE(LOG_TAG, "Screen State printout");
        }
        else {
            /* have appropriate streams transition out of LPI */
            PAL_VERBOSE(LOG_TAG, "Screen State printout");
        }
        screen_state_ = screen_state.screen_state;
        /* update
         * for (typename std::vector<StreamSoundTrigger*>::iterator iter = active_streams_st.begin();
         *    iter != active_streams_st.end(); iter++) {
         *   status = (*iter)->handleScreenState(screen_state_);
         *  }
         */
    }
    return status;
}

int ResourceManager::handleDeviceRotationChange (pal_param_device_rotation_t
                                                         rotation_type) {
    std::vector<Stream*>::iterator sIter;
    pal_stream_type_t streamType;
    struct pal_device dattr;
    int status = 0;
    PAL_INFO(LOG_TAG, "Device Rotation Changed %d", rotation_type.rotation_type);
    rotation_type_ = rotation_type.rotation_type;

    /**Get the active device list and check if speaker is present.
     */
    for (int i = 0; i < active_devices.size(); i++) {
        int deviceId = active_devices[i].first->getSndDeviceId();
        status = active_devices[i].first->getDeviceAttributes(&dattr);
        if(0 != status) {
           PAL_ERR(LOG_TAG,"getDeviceAttributes Failed");
           goto error;
        }
        PAL_INFO(LOG_TAG, "Device Got %d with channel %d",deviceId,
                                                 dattr.config.ch_info.channels);
        if ((PAL_DEVICE_OUT_SPEAKER == deviceId) &&
            (2 == dattr.config.ch_info.channels)) {

            PAL_INFO(LOG_TAG, "Device is Stereo Speaker");
            std::vector <Stream *> activeStreams;
            getActiveStream_l(activeStreams, active_devices[i].first);
            for (sIter = activeStreams.begin(); sIter != activeStreams.end(); sIter++) {
                status = (*sIter)->getStreamType(&streamType);
                if(0 != status) {
                   PAL_ERR(LOG_TAG,"setParameters Failed");
                   goto error;
                }
                /** Check for the Streams which can require Stereo speaker functionality.
                 * Mainly these will need :
                 * 1. Deep Buffer
                 * 2. PCM offload
                 * 3. Compressed
                 */
                if ((PAL_STREAM_DEEP_BUFFER == streamType) ||
                    (PAL_STREAM_COMPRESSED == streamType) ||
                    (PAL_STREAM_PCM_OFFLOAD == streamType) ||
                    (PAL_STREAM_ULTRA_LOW_LATENCY == streamType) ||
#ifdef SEC_AUDIO_EARLYDROP_PATCH
                    (PAL_STREAM_GENERIC == streamType) ||
#endif
                    (PAL_STREAM_LOW_LATENCY == streamType)) {

                    PAL_INFO(LOG_TAG, "Rotation for stream %d", streamType);
                    // Need to set the rotation now.
                    status = (*sIter)->setParameters(PAL_PARAM_ID_DEVICE_ROTATION,
                                                     (void*)&rotation_type);
                    if (0 != status) {
                        PAL_ERR(LOG_TAG,"setParameters Failed for stream %d", streamType);
                    }
                }
            }
        }
    }
error :
    PAL_INFO(LOG_TAG, "Exiting handleDeviceRotationChange, status %d", status);
    return status;
}

// called with mActiveStreamMutex locked
void ResourceManager::onChargingStateChange()
{
    std::vector<pal_stream_type_t> st_streams;
    bool need_switch = false;
    bool use_lpi_temp = false;

    // no need to handle car mode if no Voice Stream exists
    if (active_streams_st.size() == 0)
        return;

    if (charging_state_ && use_lpi_) {
        use_lpi_temp = false;
        need_switch = true;
    } else if (!charging_state_ && !use_lpi_) {
        if (!concurrencyEnableCount) {
            use_lpi_temp = true;
            need_switch = true;
        }
    }

    if (need_switch) {
        if (active_streams_st.size())
            st_streams.push_back(PAL_STREAM_VOICE_UI);
        if (active_streams_acd.size())
            st_streams.push_back(PAL_STREAM_ACD);
        if (active_streams_asr.size())
            st_streams.push_back(PAL_STREAM_ASR);
        if (active_streams_sensor_pcm_data.size())
            st_streams.push_back(PAL_STREAM_SENSOR_PCM_DATA);

        if (!checkAndUpdateDeferSwitchState(!use_lpi_temp)) {
            use_lpi_ = use_lpi_temp;
            handleConcurrentStreamSwitch(st_streams);
        }
    }
}

// called with mActiveStreamMutex locked
void ResourceManager::onVUIStreamRegistered()
{
    std::vector<pal_stream_type_t> st_streams;

    if (!charging_state_ || !IsTransitToNonLPIOnChargingSupported())
        return;

    if (active_streams_acd.size())
        st_streams.push_back(PAL_STREAM_ACD);
    if (active_streams_sensor_pcm_data.size())
        st_streams.push_back(PAL_STREAM_SENSOR_PCM_DATA);

    if (use_lpi_) {
        use_lpi_ = false;
        handleConcurrentStreamSwitch(st_streams);
    }
}

// called with mActiveStreamMutex locked
void ResourceManager::onVUIStreamDeregistered()
{
    std::vector<pal_stream_type_t> st_streams;

    if (!charging_state_ || !IsTransitToNonLPIOnChargingSupported())
        return;

    if (active_streams_acd.size())
        st_streams.push_back(PAL_STREAM_ACD);
    if (active_streams_sensor_pcm_data.size())
        st_streams.push_back(PAL_STREAM_SENSOR_PCM_DATA);

    if (!use_lpi_ && !concurrencyEnableCount) {
        use_lpi_ = true;
        handleConcurrentStreamSwitch(st_streams);
    }
}

int ResourceManager::SetOrientationCal(pal_param_device_rotation_t
                                                         rotation_type) {
    std::vector<Stream*>::iterator sIter;
    struct pal_stream_attributes sAttr;
    Stream *stream = NULL;
    struct pal_device dattr;
    Session *session = NULL;
    std::vector<Stream*> activestreams;
    int status = 0;
    PAL_INFO(LOG_TAG, "Device Rotation Changed %d", rotation_type.rotation_type);
    rm->mOrientation = rotation_type.rotation_type == PAL_SPEAKER_ROTATION_LR ? ORIENTATION_0 : ORIENTATION_270;

    /**Get the active device list and check if speaker is present.
     */
    for (int i = 0; i < active_devices.size(); i++) {
        int deviceId = active_devices[i].first->getSndDeviceId();
        status = active_devices[i].first->getDeviceAttributes(&dattr);
        if(0 != status) {
           PAL_ERR(LOG_TAG,"getDeviceAttributes Failed");
           goto error;
        }
        if ((PAL_DEVICE_OUT_SPEAKER == deviceId || PAL_DEVICE_IN_HANDSET_MIC == deviceId)
            && !strcmp(dattr.custom_config.custom_key, "mspp")) {
            status = getActiveStream_l(activestreams, active_devices[i].first);
            if ((0 != status) || (activestreams.size() == 0)) {
               PAL_ERR(LOG_TAG, "no other active streams found");
               status = -EINVAL;
               goto error;
            }

            stream = static_cast<Stream *>(activestreams[0]);
            stream->getStreamAttributes(&sAttr);
            if ((sAttr.direction == PAL_AUDIO_OUTPUT ||
                 sAttr.direction == PAL_AUDIO_INPUT ) &&
                ((sAttr.type == PAL_STREAM_LOW_LATENCY) ||
                (sAttr.type == PAL_STREAM_DEEP_BUFFER) ||
                (sAttr.type == PAL_STREAM_COMPRESSED) ||
                (sAttr.type == PAL_STREAM_PCM_OFFLOAD))) {
                stream->setOrientation(rm->mOrientation);
                stream->getAssociatedSession(&session);
                PAL_INFO(LOG_TAG, "Apply device rotation");
                status = session->setConfig(stream, MODULE, ORIENTATION_TAG);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "session setConfig failed with status %d", status);
                    goto error;
                }
            }
        }
    }
error :
    PAL_INFO(LOG_TAG, "Exiting SetOrientationCal, status %d", status);
    return status;
}

#ifdef SEC_AUDIO_BARGEIN_MODE
int ResourceManager::GetBargeinMode() {
    std::vector<Stream*>::iterator sIter;
    struct pal_stream_attributes sAttr;
    Stream *stream = NULL;
    struct pal_device dattr;
    Session *session = NULL;
    std::vector<Stream*> activestreams;
    int status = 0;

    for (int i = 0; i < active_devices.size(); i++) {
        int deviceId = active_devices[i].first->getSndDeviceId();
        status = active_devices[i].first->getDeviceAttributes(&dattr);
        if(0 != status) {
           PAL_ERR(LOG_TAG,"getDeviceAttributes Failed");
           goto error;
        }
        if ((PAL_DEVICE_IN_SPEAKER_MIC == deviceId) && !strcmp(dattr.custom_config.custom_key, "bargein_aec")) {
            status = getActiveStream_l(activestreams, active_devices[i].first);
            if ((0 != status) || (activestreams.size() == 0)) {
               PAL_ERR(LOG_TAG, "no other active streams found");
               status = -EINVAL;
               goto error;
            }

            stream = static_cast<Stream *>(activestreams[0]);
            stream->getStreamAttributes(&sAttr);
            if ((sAttr.direction == PAL_AUDIO_INPUT) &&
                (sAttr.type == PAL_STREAM_DEEP_BUFFER)) {
                stream->getAssociatedSession(&session);
                int payloadSize = sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + sizeof(int) * 532;
                effect_pal_payload_t *effectPayload = (effect_pal_payload_t *)calloc(1, payloadSize);
                effectPayload->isTKV = PARAM_NONTKV;
                effectPayload->tag = TAG_SPEECH_UI_MODE;
                effectPayload->payloadSize = sizeof(pal_effect_custom_payload_t) + sizeof(int) * 532;

                pal_effect_custom_payload_t * customPayload = (pal_effect_custom_payload_t *)effectPayload->payload;
                customPayload->paramId = 0x1000B030 /* PARAM_ID_SPEECH_UI */;

                session->getEffectParameters(stream, effectPayload);

                PAL_INFO(LOG_TAG, "Preset BaselineVersion: (%lu)(%lu)(%lu), operating_mode: 0x%x",
                ((uint8_t *)customPayload->data)[2],
                ((uint8_t *)customPayload->data)[3],
                ((uint8_t *)customPayload->data)[4],
                ((uint32_t *)customPayload->data)[14]);

                uint32_t operating_mode = ((uint32_t *)customPayload->data)[14];
                int res = (operating_mode == 0) ? 2 : 0;

                delete effectPayload;
                return res;
            }
        }
    }

    return bargein_mode;

error :
    PAL_INFO(LOG_TAG, "Exiting GetBargeinMode, status %d", status);
    return -1;
}
#endif

bool ResourceManager::getScreenState()
{
    return screen_state_;
}

pal_speaker_rotation_type ResourceManager::getCurrentRotationType()
{
    return rotation_type_;
}

int ResourceManager::getDeviceDefaultCapability(pal_param_device_capability_t capability) {
    int status = 0;
    pal_device_id_t device_pal_id = capability.id;
    bool device_available = isDeviceAvailable(device_pal_id);

    struct pal_device conn_device;
    std::shared_ptr<Device> dev = nullptr;
    std::shared_ptr<Device> candidate_device;

    memset(&conn_device, 0, sizeof(struct pal_device));
    conn_device.id = device_pal_id;
    PAL_DBG(LOG_TAG, "device pal id=%x available=%x", device_pal_id, device_available);
    dev = Device::getInstance(&conn_device, rm);
    if (dev)
        status = dev->getDefaultConfig(capability);
    else
        PAL_ERR(LOG_TAG, "failed to get device instance.");

    return status;
}

int ResourceManager::handleDeviceConnectionChange(pal_param_device_connection_t connection_state) {
    int status = 0;
    pal_device_id_t device_id = connection_state.id;
    bool is_connected = connection_state.connection_state;
    bool device_available = isDeviceAvailable(device_id);
    struct pal_device dAttr;
    struct pal_device conn_device;
    std::shared_ptr<Device> dev = nullptr;
    struct pal_device_info devinfo = {};
    int32_t scoCount = is_connected ? 1 : -1;
    bool removeScoDevice = false;

    if (isBtScoDevice(device_id)) {
        PAL_DBG(LOG_TAG, "Enter: scoOutConnectCount=%d, scoInConnectCount=%d",
                                        scoOutConnectCount, scoInConnectCount);
        if (device_id == PAL_DEVICE_OUT_BLUETOOTH_SCO) {
            scoOutConnectCount += scoCount;
            removeScoDevice = !scoOutConnectCount;
        } else {
            scoInConnectCount += scoCount;
            removeScoDevice = !scoInConnectCount;
        }
    }

    PAL_DBG(LOG_TAG, "Enter");
    memset(&conn_device, 0, sizeof(struct pal_device));
    if (is_connected && !device_available) {
        if (isPluginDevice(device_id) || isDpDevice(device_id)) {
            conn_device.id = device_id;
            dev = Device::getInstance(&conn_device, rm);
            if (dev) {
                status = addPlugInDevice(dev, connection_state);
                if (!status) {
                    PAL_DBG(LOG_TAG, "Mark device %d as available", device_id);
                    avail_devices_.push_back(device_id);
                } else if (status == -ENOENT) {
                    status = 0; //ignore error for no-entry devices
                }
                goto exit;
            } else {
                PAL_ERR(LOG_TAG, "Device creation failed");
                throw std::runtime_error("failed to create device object");
            }
        }

        if (device_id == PAL_DEVICE_OUT_BLUETOOTH_A2DP ||
            device_id == PAL_DEVICE_IN_BLUETOOTH_A2DP ||
            device_id == PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST ||
            device_id == PAL_DEVICE_OUT_BLUETOOTH_BLE ||
            device_id == PAL_DEVICE_IN_BLUETOOTH_BLE ||
            isBtScoDevice(device_id)) {
            dAttr.id = device_id;
            /* Stream type is irrelevant here as we need device num channels
               which is independent of stype for BT devices */
            status = getDeviceConfig(&dAttr, NULL);
            if (status) {
                PAL_ERR(LOG_TAG, "Device config not overwritten %d", status);
                goto err;
            }
            dev = Device::getInstance(&dAttr, rm);
            if (!dev) {
                PAL_ERR(LOG_TAG, "Device creation failed");
                throw std::runtime_error("failed to create device object");
                status = -EIO;
                goto err;
            }
        }
        if (!dev) {
            dAttr.id = device_id;
            dev = Device::getInstance(&dAttr, rm);
            if (!dev)
                PAL_ERR(LOG_TAG, "get dev instance for %d failed", device_id);
        }
        if (dev) {
            PAL_DBG(LOG_TAG, "Mark device %d as available", device_id);
            avail_devices_.push_back(device_id);
        }
    } else if (!is_connected && device_available) {
        if (isPluginDevice(device_id) || isDpDevice(device_id)) {
            removePlugInDevice(device_id, connection_state);
        }

        if (isValidDevId(device_id)) {
            auto iter =
                std::find(avail_devices_.begin(), avail_devices_.end(),
                            device_id);

            if (iter != avail_devices_.end()) {
                PAL_INFO(LOG_TAG, "found device id 0x%x in avail_device",
                                        device_id);
                conn_device.id = device_id;
                dev = Device::getInstance(&conn_device, rm);
                if (!dev) {
                    PAL_ERR(LOG_TAG, "Device getInstance failed");
                    throw std::runtime_error("failed to get device object");
                    status = -EIO;
                    goto err;
                }

                if (isBtScoDevice(device_id) && (removeScoDevice == false))
                    goto exit;

                dev->setDeviceAttributes(conn_device);
                PAL_INFO(LOG_TAG, "device attribute cleared");
                PAL_DBG(LOG_TAG, "Mark device %d as unavailable", device_id);
            }
        }
        auto iter =
            std::find(avail_devices_.begin(), avail_devices_.end(),
                        device_id);
        if (iter != avail_devices_.end())
            avail_devices_.erase(iter);
#ifdef SEC_AUDIO_BT_MULTIPLE_PROFILE
    } else if (isBtA2dpDevice(device_id)) {
        status = 0;
        PAL_INFO(LOG_TAG, "Invalid operation, Device %d, connection state %d, device avalibilty %d",
                device_id, is_connected, device_available);
#endif
    } else if (!isBtScoDevice(device_id)) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid operation, Device %d, connection state %d, device avalibilty %d",
                device_id, is_connected, device_available);
    }

    goto exit;

err:
    if (status && isBtScoDevice(device_id)) {
        if (device_id == PAL_DEVICE_OUT_BLUETOOTH_SCO)
            scoOutConnectCount -= scoCount;
        else
            scoInConnectCount -= scoCount;
    }

exit:
    if (isBtScoDevice(device_id))
        PAL_DBG(LOG_TAG, "Exit: scoOutConnectCount=%d, scoInConnectCount=%d",
                                        scoOutConnectCount, scoInConnectCount);
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int ResourceManager::resetStreamInstanceID(Stream *s){
    return s ? resetStreamInstanceID(s, s->getInstanceId()) : -EINVAL;
}

int ResourceManager::resetStreamInstanceID(Stream *str, uint32_t sInstanceID) {
    int status = 0;
    pal_stream_attributes StrAttr;
    std::string streamSelector;

    if(sInstanceID < INSTANCE_1){
        PAL_ERR(LOG_TAG,"Invalid Stream Instance ID\n");
        return -EINVAL;
    }

    status = str->getStreamAttributes(&StrAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"getStreamAttributes Failed \n");
        return status;
    }

    mResourceManagerMutex.lock();

    switch (StrAttr.type) {
        case PAL_STREAM_VOICE_UI: {
            streamSelector = str->getStreamSelector();

            if (streamSelector.empty()) {
                PAL_DBG(LOG_TAG, "no streamSelector");
                break;
            }

            for (int x = 0; x < STInstancesLists.size(); x++) {
                if (!STInstancesLists[x].first.compare(streamSelector)) {
                    PAL_DBG(LOG_TAG,"Found matching StreamConfig:%s in STInstancesLists(%d)",
                        streamSelector.c_str(), x);

                    for (int i = 0; i < max_session_num; i++) {
                        if (STInstancesLists[x].second[i].first == sInstanceID){
                            STInstancesLists[x].second[i].second = false;
                            PAL_DBG(LOG_TAG,"ListNodeIndex(%d), InstanceIndex(%d)"
                                  "Instance(%d) to false",
                                  x,
                                  i,
                                  sInstanceID);
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
        case PAL_STREAM_NON_TUNNEL: {
            uint32_t pathIdx = getNTPathForStreamAttr(StrAttr);
            auto NTStreamInstancesMap = mNTStreamInstancesList[pathIdx];
            if (NTStreamInstancesMap->find(str->getInstanceId()) != NTStreamInstancesMap->end()) {
                NTStreamInstancesMap->at(str->getInstanceId()) = false;
            }
            str->setInstanceId(0);
            break;
        }
        case PAL_STREAM_SENSOR_PCM_DATA: {
            for (auto instance : PCMDataInstances) {
                if (sInstanceID == instance.first) {
                    PAL_DBG(LOG_TAG, "Reset Sensor PCM Data instance: %d to false", sInstanceID);
                    PCMDataInstances[instance.first] = false;
                    break;
                }
            }
            str->setInstanceId(0);
            break;
        }
        default: {
            if (StrAttr.direction == PAL_AUDIO_INPUT) {
                in_stream_instances[StrAttr.type - 1] &= ~(1 << (sInstanceID - 1));
                str->setInstanceId(0);
            } else {
                stream_instances[StrAttr.type - 1] &= ~(1 << (sInstanceID - 1));
                str->setInstanceId(0);
            }
        }
    }

    mResourceManagerMutex.unlock();
    return status;
}

int ResourceManager::getStreamInstanceID(Stream *str) {
    int i, status = 0, listNodeIndex = -1;
    pal_stream_attributes StrAttr;
    std::string streamSelector;

    status = str->getStreamAttributes(&StrAttr);

    if (status != 0) {
        PAL_ERR(LOG_TAG,"getStreamAttributes Failed \n");
        return status;
    }

    mResourceManagerMutex.lock();

    switch (StrAttr.type) {
        case PAL_STREAM_VOICE_UI: {
            PAL_DBG(LOG_TAG,"STInstancesLists.size (%zu)", STInstancesLists.size());

            streamSelector = str->getStreamSelector();

            if (streamSelector.empty()) {
                PAL_DBG(LOG_TAG, "no stream selector");
                break;
            }

            for (int x = 0; x < STInstancesLists.size(); x++) {
                if (!STInstancesLists[x].first.compare(streamSelector)) {
                    PAL_DBG(LOG_TAG,"Found list for StreamConfig(%s),index(%d)",
                        streamSelector.c_str(), x);
                    listNodeIndex = x;
                    break;
                }
            }

            if (listNodeIndex < 0) {
                InstanceListNode_t streamConfigInstanceList;
                PAL_DBG(LOG_TAG,"Create InstanceID list for streamConfig %s",
                    streamSelector.c_str());

                STInstancesLists.push_back(make_pair(
                    streamSelector,
                    streamConfigInstanceList));
                //Initialize List
                for (i = 1; i <= max_session_num; i++) {
                    STInstancesLists.back().second.push_back(std::make_pair(i, false));
                }
                listNodeIndex = STInstancesLists.size() - 1;
            }

            for (i = 0; i < max_session_num; i++) {
                if (!STInstancesLists[listNodeIndex].second[i].second) {
                    STInstancesLists[listNodeIndex].second[i].second = true;
                    status = STInstancesLists[listNodeIndex].second[i].first;
                    PAL_DBG(LOG_TAG,"ListNodeIndex(%d), InstanceIndex(%d)"
                          "Instance(%d) to true",
                          listNodeIndex,
                          i,
                          status);
                    break;
                }
            }
            break;
        }
        case PAL_STREAM_NON_TUNNEL: {
            int instanceId = str->getInstanceId();
            if (!instanceId) {
                status = instanceId = getAvailableNTStreamInstance(StrAttr);
                if (status < 0) {
                    PAL_ERR(LOG_TAG, "No available stream instance");
                    break;
                }
                str->setInstanceId(instanceId);
                PAL_DBG(LOG_TAG, "NT instance id %d", instanceId);
            }
            break;
        }
        case PAL_STREAM_SENSOR_PCM_DATA: {
            int instanceId = str->getInstanceId();
            uint32_t num_instances = PCMDataInstances.size();

            if (!instanceId) {
                for (auto instance : PCMDataInstances) {
                    if (false == instance.second) {
                        PAL_DBG(LOG_TAG,
                                "Found an available instance id: %d in PCMDataInstances",
                                instance.first);
                        instanceId = instance.first;
                        PCMDataInstances[instance.first] = true;
                        goto done;
                    }
                }
                instanceId = num_instances + 1;
                PCMDataInstances.insert(std::make_pair(instanceId, true));
done:
                str->setInstanceId(instanceId);
                PAL_DBG(LOG_TAG,
                        "Sensor PCM Data instance id: %d, number of instances: %d",
                        instanceId, PCMDataInstances.size());
            }
            status = instanceId;
            break;
        }
#ifdef SEC_AUDIO_SUPPORT_LOWLATENCY_MEDIA
        case PAL_STREAM_LOW_LATENCY: {
            if (StrAttr.direction == PAL_AUDIO_OUTPUT) {
                if (stream_instances[StrAttr.type - 1] ==  -1) {
                    PAL_ERR(LOG_TAG, "All stream instances taken");
                    status = -EINVAL;
                    break;
                }
                if (StrAttr.info.lowlatency_media.mode) {
                    status = INSTANCE_1;
                    stream_instances[StrAttr.type - 1] |= INSTANCE_1;
                } else {
                    for (i = INSTANCE_1; i < MAX_STREAM_INSTANCES; ++i)
                    if (!(stream_instances[StrAttr.type - 1] & (1 << i))) {
                        stream_instances[StrAttr.type - 1] |= (1 << i);
                        status = i + 1;
                        break;
                    }
                }
                str->setInstanceId(status);
            }
        }
        break;
#endif
        default: {
            status = str->getInstanceId();
            if (StrAttr.direction == PAL_AUDIO_INPUT && !status) {
                if (in_stream_instances[StrAttr.type - 1] ==  -1) {
                    PAL_ERR(LOG_TAG, "All stream instances taken");
                    status = -EINVAL;
                    break;
                }
                for (i = 0; i < MAX_STREAM_INSTANCES; ++i)
                    if (!(in_stream_instances[StrAttr.type - 1] & (1 << i))) {
                        in_stream_instances[StrAttr.type - 1] |= (1 << i);
                        status = i + 1;
                        break;
                    }
                str->setInstanceId(status);
            } else if (!status) {
                if (stream_instances[StrAttr.type - 1] ==  -1) {
                    PAL_ERR(LOG_TAG, "All stream instances taken");
                    status = -EINVAL;
                    break;
                }
                for (i = 0; i < MAX_STREAM_INSTANCES; ++i)
                    if (!(stream_instances[StrAttr.type - 1] & (1 << i))) {
                        stream_instances[StrAttr.type - 1] |= (1 << i);
                        status = i + 1;
                        break;
                    }
                str->setInstanceId(status);
            }
        }
    }

    mResourceManagerMutex.unlock();
    return status;
}

bool ResourceManager::isDeviceAvailable(pal_device_id_t id)
{
    bool is_available = false;
    typename std::vector<pal_device_id_t>::iterator iter =
        std::find(avail_devices_.begin(), avail_devices_.end(), id);

    if (iter != avail_devices_.end())
        is_available = true;

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    if (!is_available)
#endif
    {
        PAL_DBG(LOG_TAG, "Device %d, is_available = %d", id, is_available);
    }

    return is_available;
}

bool ResourceManager::isDeviceAvailable(
    std::vector<std::shared_ptr<Device>> devices, pal_device_id_t id)
{
    bool isAvailable = false;

    for (int i = 0; i < devices.size(); i++) {
        if (devices[i]->getSndDeviceId() == id)
            isAvailable = true;
    }

    return isAvailable;
}

bool ResourceManager::isDeviceAvailable(
    struct pal_device *devices, uint32_t devCount, pal_device_id_t id)
{
    bool isAvailable = false;

    for (int i = 0; i < devCount; i++) {
        if (devices[i].id == id)
            isAvailable = true;
    }

    return isAvailable;
}

bool ResourceManager::isDisconnectedDeviceStillActive(
    std::set<pal_device_id_t> &curPalDevices, std::set<pal_device_id_t> &activeDevices,
    pal_device_id_t id)
{
    return (!isDeviceAvailable(id)) &&
        (curPalDevices.find(id) != curPalDevices.end()) &&
        (activeDevices.find(id) != activeDevices.end());
}

bool ResourceManager::isDeviceReady(pal_device_id_t id)
{
    struct pal_device dAttr;
    std::shared_ptr<Device> dev = nullptr;
    bool is_ready = false;

    switch (id) {
        case PAL_DEVICE_OUT_BLUETOOTH_SCO:
        case PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET:
        case PAL_DEVICE_OUT_BLUETOOTH_A2DP:
        case PAL_DEVICE_IN_BLUETOOTH_A2DP:
        case PAL_DEVICE_OUT_BLUETOOTH_BLE:
        case PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST:
        case PAL_DEVICE_IN_BLUETOOTH_BLE:
        {
            if (!isDeviceAvailable(id))
                return is_ready;

            dAttr.id = id;
            dev = Device::getInstance((struct pal_device *)&dAttr , rm);
            if (!dev) {
                PAL_ERR(LOG_TAG, "Device getInstance failed");
                return false;
            }
            is_ready = dev->isDeviceReady();
            break;
        }
        default:
            is_ready = true;
            break;
    }

    return is_ready;
}

bool ResourceManager::isBtA2dpDevice(pal_device_id_t id)
{
    if (id == PAL_DEVICE_OUT_BLUETOOTH_A2DP ||
        id == PAL_DEVICE_OUT_BLUETOOTH_BLE ||
        id == PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST ||
        id == PAL_DEVICE_IN_BLUETOOTH_A2DP ||
        id == PAL_DEVICE_IN_BLUETOOTH_BLE)
        return true;
    else
        return false;
}

bool ResourceManager::isBtScoDevice(pal_device_id_t id)
{
    if (id == PAL_DEVICE_OUT_BLUETOOTH_SCO ||
        id == PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET)
        return true;
    else
        return false;
}

bool ResourceManager::isBtDevice(pal_device_id_t id)
{
    switch (id) {
        case PAL_DEVICE_OUT_BLUETOOTH_A2DP:
        case PAL_DEVICE_IN_BLUETOOTH_A2DP:
        case PAL_DEVICE_OUT_BLUETOOTH_SCO:
        case PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET:
        case PAL_DEVICE_OUT_BLUETOOTH_BLE:
        case PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST:
        case PAL_DEVICE_IN_BLUETOOTH_BLE:
            return true;
        default:
            return false;
    }
}

void ResourceManager::updateBtCodecMap(std::pair<uint32_t, std::string> key, std::string value)
{
    btCodecMap.insert(std::make_pair(key, value));
}

std::string ResourceManager::getBtCodecLib(uint32_t codecFormat, std::string codecType)
{
    std::map<std::pair<uint32_t, std::string>, std::string>::iterator iter;

    iter = btCodecMap.find(std::make_pair(codecFormat, codecType));
    if (iter != btCodecMap.end()) {
        return iter->second;
    }

    return std::string();
}

void ResourceManager::updateBtSlimClockSrcMap(uint32_t key, uint32_t value)
{
    btSlimClockSrcMap.insert(std::make_pair(key, value));
}

uint32_t ResourceManager::getBtSlimClockSrc(uint32_t codecFormat)
{
    std::map<uint32_t, std::uint32_t>::iterator iter;

    iter = btSlimClockSrcMap.find(codecFormat);
    if (iter != btSlimClockSrcMap.end())
        return iter->second;

    return CLOCK_SRC_DEFAULT;
}

void ResourceManager::processSilenceDetectionConfig(const XML_Char **attr)
{
    if (!strcmp(attr[0], "pcm")) {
        ResourceManager::isSilenceDetectionEnabledPcm = atoi(attr[1])?true:false;
    }

    if (!strcmp(attr[2], "voice")) {
        ResourceManager::isSilenceDetectionEnabledVoice = atoi(attr[3])?true:false;
    }

    if (!strcmp(attr[4], "duration")) {
        ResourceManager::silenceDetectionDuration = atoi(attr[5]);
    }

    return;
}

void ResourceManager::processPerfLockConfig(const XML_Char **attr)
{
    if (strcmp(attr[0], "library") != 0) {
        PAL_ERR(LOG_TAG,"'library' not found");
        return;
    }

    if (strcmp(attr[2], "config") != 0) {
        PAL_ERR(LOG_TAG,"'config' not found");
        return;
    }
    PerfLockConfig perfConfig;
    perfConfig.libraryName = std::string(attr[1]);

    std::string config = std::string(attr[3]);
    std::vector<int> perfLockConfigs;
    size_t pos = 0;
    while ((pos = config.find(',')) != std::string::npos) {
        std::string token = config.substr(0, pos);
        perfLockConfigs.push_back(convertCharToHex(token));
        config.erase(0, pos + 1);
    }
    perfLockConfigs.push_back(convertCharToHex(config));
    perfConfig.perfLockOpts = perfLockConfigs;
    perfConfig.usePerfLock = true;

    PerfLock::setPerfLockOpt(perfConfig);
}

void ResourceManager::processBTCodecInfo(const XML_Char **attr, const int attr_count)
{
    char *saveptr = NULL;
    char *token = NULL;
    std::vector<std::string> codec_formats, codec_types;

    if (strcmp(attr[0], "codec_format") != 0) {
        PAL_ERR(LOG_TAG,"'codec_format' not found");
        goto done;
    }

    if (strcmp(attr[2], "codec_type") != 0) {
        PAL_ERR(LOG_TAG,"'codec_type' not found");
        goto done;
    }

    if (strcmp(attr[4], "codec_library") != 0) {
        PAL_ERR(LOG_TAG,"'codec_library' not found");
        goto done;
    }

    token = strtok_r((char *)attr[1], "|", &saveptr);
    while (token != NULL) {
        if (strlen(token) != 0) {
            codec_formats.push_back(std::string(token));
        }
        token = strtok_r(NULL, "|", &saveptr);
    }

    token = strtok_r((char *)attr[3], "|", &saveptr);
    while (token != NULL) {
        if (strlen(token) != 0) {
            codec_types.push_back(std::string(token));
        }
        token = strtok_r(NULL, "|", &saveptr);
    }

    if (codec_formats.empty() || codec_types.empty()) {
        PAL_INFO(LOG_TAG,"BT codec formats or types empty!");
        goto done;
    }

    for (std::vector<std::string>::iterator iter1 = codec_formats.begin();
            iter1 != codec_formats.end(); ++iter1) {
        if (btFmtTable.find(*iter1) != btFmtTable.end()) {
            for (std::vector<std::string>::iterator iter2 = codec_types.begin();
                    iter2 != codec_types.end(); ++iter2) {
                PAL_VERBOSE(LOG_TAG, "BT Codec Info %s=%s, %s=%s, %s=%s",
                        attr[0], (*iter1).c_str(), attr[2], (*iter2).c_str(), attr[4], attr[5]);

                updateBtCodecMap(std::make_pair(btFmtTable[*iter1], *iter2),  std::string(attr[5]));
            }

            /* Clock is an optional attribute unlike codec_format & codec_type.
             * Check attr count before accessing it.
             * attr[6] & attr[7] reserved for clock source value
             */
            if (attr_count >= 8 && strcmp(attr[6], "clock") == 0) {
                PAL_VERBOSE(LOG_TAG, "BT Codec Info %s=%s, %s=%s",
                        attr[0], (*iter1).c_str(), attr[6], attr[7]);
                updateBtSlimClockSrcMap(btFmtTable[*iter1], atoi(attr[7]));
            }
        }
    }

done:
    return;
}

std::string ResourceManager::getSpkrTempCtrl(int channel)
{
    std::map<int, std::string>::iterator iter;


    iter = spkrTempCtrlsMap.find(channel);
    if (iter != spkrTempCtrlsMap.end()) {
        return iter->second;
    }

    return std::string();
}

void ResourceManager::updateSpkrTempCtrls(int key, std::string value)
{
    spkrTempCtrlsMap.insert(std::make_pair(key, value));
}

void ResourceManager::processSpkrTempCtrls(const XML_Char **attr)
{
    std::map<std::string, int>::iterator iter;

    if ((strcmp(attr[0], "spkr_posn") != 0) ||
        (strcmp(attr[2], "ctrl") != 0)) {
        PAL_ERR(LOG_TAG,"invalid attribute passed %s %s expected spkr_posn and ctrl",
                         attr[0], attr[2]);
        goto done;
    }

    iter = spkrPosTable.find(std::string(attr[1]));

    if (iter != spkrPosTable.end())
        updateSpkrTempCtrls(iter->second, std::string(attr[3]));

done:
    return;
}

bool ResourceManager::isPluginDevice(pal_device_id_t id) {
    if (id == PAL_DEVICE_OUT_USB_DEVICE ||
        id == PAL_DEVICE_OUT_USB_HEADSET ||
        id == PAL_DEVICE_IN_USB_DEVICE ||
        id == PAL_DEVICE_IN_USB_HEADSET)
        return true;
    else
        return false;
}

bool ResourceManager::isDpDevice(pal_device_id_t id) {
    if (id == PAL_DEVICE_OUT_AUX_DIGITAL || id == PAL_DEVICE_OUT_AUX_DIGITAL_1 ||
        id == PAL_DEVICE_OUT_HDMI)
        return true;
    else
        return false;
}

bool ResourceManager::isPluginPlaybackDevice(pal_device_id_t id) {
    if (id == PAL_DEVICE_OUT_USB_DEVICE ||
        id == PAL_DEVICE_OUT_USB_HEADSET ||
        id == PAL_DEVICE_OUT_WIRED_HEADSET ||
        id == PAL_DEVICE_OUT_WIRED_HEADPHONE)
        return true;
    else
        return false;
}

bool ResourceManager::isPluginCaptureDevice(pal_device_id_t id) {
    if (id == PAL_DEVICE_IN_USB_DEVICE ||
        id == PAL_DEVICE_IN_USB_HEADSET ||
        id == PAL_DEVICE_IN_WIRED_HEADSET)
        return true;
    else
        return false;
}

void ResourceManager::processConfigParams(const XML_Char **attr)
{
    if (strcmp(attr[0], "key") != 0) {
        PAL_ERR(LOG_TAG,"'key' not found");
        goto done;
    }

    if (strcmp(attr[2], "value") != 0) {
        PAL_ERR(LOG_TAG,"'value' not found");
        goto done;
    }
    PAL_VERBOSE(LOG_TAG, "String %s %s %s %s ",attr[0],attr[1],attr[2],attr[3]);
    configParamKVPairs = str_parms_create();
    if (configParamKVPairs) {
        str_parms_add_str(configParamKVPairs, (char*)attr[1], (char*)attr[3]);
        setConfigParams(configParamKVPairs);
        str_parms_destroy(configParamKVPairs);
    }
done:
    return;
}

void ResourceManager::processCardInfo(struct xml_userdata *data, const XML_Char *tag_name)
{
    if (!strcmp(tag_name, "id")) {
        snd_virt_card = atoi(data->data_buf);
        data->card_found = true;
        PAL_VERBOSE(LOG_TAG, "virtual soundcard number : %d ", snd_virt_card);
    }
}

void ResourceManager::processDeviceIdProp(struct xml_userdata *data, const XML_Char *tag_name)
{
    int device, size = -1;
    struct deviceCap dev;

    memset(&dev, 0, sizeof(struct deviceCap));
    if (!strcmp(tag_name, "pcm-device") ||
        !strcmp(tag_name, "compress-device") ||
        !strcmp(tag_name, "mixer"))
        return;

    if (!strcmp(tag_name, "id")) {
        device = atoi(data->data_buf);
        dev.deviceId = device;
        devInfo.push_back(dev);
    } else if (!strcmp(tag_name, "name")) {
        size = devInfo.size() - 1;
        strlcpy(devInfo[size].name, data->data_buf, strlen(data->data_buf)+1);
        if(strstr(data->data_buf,"PCM")) {
            devInfo[size].type = PCM;
        } else if (strstr(data->data_buf,"COMP")) {
            devInfo[size].type = COMPRESS;
        } else if (strstr(data->data_buf,"VOICEMMODE1")){
            devInfo[size].type = VOICE1;
        } else if (strstr(data->data_buf,"VOICEMMODE2")){
            devInfo[size].type = VOICE2;
        } else if (strstr(data->data_buf,"ExtEC")){
            devInfo[size].type = ExtEC;
        }
    }
}

void ResourceManager::processDeviceCapability(struct xml_userdata *data, const XML_Char *tag_name)
{
    int size = -1;
    int val = -1;
    if (!strlen(data->data_buf) || !strlen(tag_name))
        return;
    if (strcmp(tag_name, "props") == 0)
        return;
    size = devInfo.size() - 1;
    if (strcmp(tag_name, "playback") == 0) {
        val = atoi(data->data_buf);
        devInfo[size].playback = val;
    } else if (strcmp(tag_name, "capture") == 0) {
        val = atoi(data->data_buf);
        devInfo[size].record = val;
    } else if (strcmp(tag_name,"session_mode") == 0) {
        val = atoi(data->data_buf);
        devInfo[size].sess_mode = (sess_mode_t) val;
    }
}

void ResourceManager::process_gain_db_to_level_map(struct xml_userdata *data, const XML_Char **attr)
{
    struct pal_amp_db_and_gain_table tbl_entry;

    if (data->gain_lvl_parsed)
        return;

    if ((strcmp(attr[0], "db") != 0) ||
        (strcmp(attr[2], "level") != 0)) {
        PAL_ERR(LOG_TAG, "invalid attribute passed  %s %sexpected amp db level", attr[0], attr[2]);
        goto done;
    }

    tbl_entry.db = atof(attr[1]);
    tbl_entry.amp = exp(tbl_entry.db * 0.115129f);
    tbl_entry.level = atoi(attr[3]);

    // custome level should be > 0. Level 0 is fixed for default
    if (tbl_entry.level <= 0) {
        PAL_ERR(LOG_TAG, "amp [%f]  db [%f] level [%d]",
               tbl_entry.amp, tbl_entry.db, tbl_entry.level);
        goto done;
    }

    PAL_VERBOSE(LOG_TAG, "amp [%f]  db [%f] level [%d]",
           tbl_entry.amp, tbl_entry.db, tbl_entry.level);

    if (!gainLvlMap.empty() && (gainLvlMap.back().amp >= tbl_entry.amp)) {
        PAL_ERR(LOG_TAG, "value not in ascending order .. rejecting custom mapping");
        gainLvlMap.clear();
        data->gain_lvl_parsed = true;
    }

    gainLvlMap.push_back(tbl_entry);

done:
    return;
}

int ResourceManager::getGainLevelMapping(struct pal_amp_db_and_gain_table *mapTbl, int tblSize)
{
    int size = 0;

    if (gainLvlMap.empty()) {
        PAL_DBG(LOG_TAG, "empty or currupted gain_mapping_table");
        return 0;
    }

    for (; size < gainLvlMap.size() && size <= tblSize; size++) {
        mapTbl[size] = gainLvlMap.at(size);
        PAL_VERBOSE(LOG_TAG, "added amp[%f] db[%f] level[%d]",
                mapTbl[size].amp, mapTbl[size].db, mapTbl[size].level);
    }

    return size;
}

void ResourceManager::snd_reset_data_buf(struct xml_userdata *data)
{
    data->offs = 0;
    data->data_buf[data->offs] = '\0';
}

void ResourceManager::process_voicemode_info(const XML_Char **attr)
{
    std::string tagkey(attr[1]);
    std::string tagvalue(attr[3]);
    struct vsid_modepair modepair = {};

    if (strcmp(attr[0], "key") !=0) {
        PAL_ERR(LOG_TAG, "key not found");
        return;
    }
    modepair.key = convertCharToHex(tagkey);

    if (strcmp(attr[2], "value") !=0) {
        PAL_ERR(LOG_TAG, "value not found");
        return;
    }
    modepair.value = convertCharToHex(tagvalue);
    PAL_VERBOSE(LOG_TAG, "key  %x value  %x", modepair.key, modepair.value);
    vsidInfo.modepair.push_back(modepair);
}

void ResourceManager::process_config_volume(struct xml_userdata *data, const XML_Char *tag_name)
{
    if (data->offs <= 0 || data->resourcexml_parsed)
        return;

    data->data_buf[data->offs] = '\0';
    if (data->tag == TAG_CONFIG_VOLUME) {
        if (strcmp(tag_name, "use_volume_set_param") == 0) {
            volumeSetParamInfo_.isVolumeUsingSetParam = atoi(data->data_buf);
        }
    }
    if (data->tag == TAG_CONFIG_VOLUME_SET_PARAM_SUPPORTED_STREAM) {
        std::string stream_name(data->data_buf);
        PAL_DBG(LOG_TAG, "Stream name to be added : %s", stream_name.c_str());
        uint32_t st = usecaseIdLUT.at(stream_name);
        volumeSetParamInfo_.streams_.push_back(st);
        PAL_DBG(LOG_TAG, "Stream type added for volume set param : %d", st);
    }
    if (!strcmp(tag_name, "supported_stream")) {
        data->tag = TAG_CONFIG_VOLUME_SET_PARAM_SUPPORTED_STREAMS;
    } else if (!strcmp(tag_name, "supported_streams")) {
        data->tag = TAG_CONFIG_VOLUME;
    } else if (!strcmp(tag_name, "config_volume")) {
        data->tag = TAG_RESOURCE_MANAGER_INFO;
    }
}

void ResourceManager::process_config_lpm(struct xml_userdata *data, const XML_Char *tag_name)
{
    if (data->offs <= 0 || data->resourcexml_parsed)
        return;

    data->data_buf[data->offs] = '\0';
    if (data->tag == TAG_CONFIG_LPM) {
        if (strcmp(tag_name, "use_disable_lpm") == 0) {
            disableLpmInfo_.isDisableLpm = atoi(data->data_buf);
        }
    }
    if (data->tag == TAG_CONFIG_LPM_SUPPORTED_STREAM) {
        std::string stream_name(data->data_buf);
        PAL_DBG(LOG_TAG, "Stream name to be added : %s", stream_name.c_str());
        uint32_t st = usecaseIdLUT.at(stream_name);
        disableLpmInfo_.streams_.push_back(st);
        PAL_DBG(LOG_TAG, "Stream type added for disable lpm : %d", st);
    }
    if (!strcmp(tag_name, "lpm_supported_stream")) {
        data->tag = TAG_CONFIG_LPM_SUPPORTED_STREAMS;
    } else if (!strcmp(tag_name, "lpm_supported_streams")) {
        data->tag = TAG_CONFIG_LPM;
    } else if (!strcmp(tag_name, "config_lpm")) {
        data->tag = TAG_RESOURCE_MANAGER_INFO;
    }
}

void ResourceManager::process_config_voice(struct xml_userdata *data, const XML_Char *tag_name)
{
    if(data->voice_info_parsed)
        return;

    if (data->offs <= 0)
        return;
    data->data_buf[data->offs] = '\0';
    if (data->tag == TAG_CONFIG_VOICE) {
        if (strcmp(tag_name, "vsid") == 0) {
            std::string vsidvalue(data->data_buf);
            vsidInfo.vsid = convertCharToHex(vsidvalue);
        }
#ifdef SEC_AUDIO_LOOPBACK_TEST
        if (strcmp(tag_name, "loopbackDelay") == 0) {
            vsidInfo.loopback_mode_delay[PAL_LOOPBACK_PACKET_DELAY_MODE] = atoi(data->data_buf);
        }
        if (strcmp(tag_name, "loopbackNoDelay") == 0) {
            vsidInfo.loopback_mode_delay[PAL_LOOPBACK_PACKET_NO_DELAY_MODE] = atoi(data->data_buf);
        }
#else
        if (strcmp(tag_name, "loopbackDelay") == 0) {
            vsidInfo.loopback_delay = atoi(data->data_buf);
        }
#endif
        if (strcmp(tag_name, "maxVolIndex") == 0) {
            max_voice_vol = atoi(data->data_buf);
        }
    }
    if (!strcmp(tag_name, "modepair")) {
        data->tag = TAG_CONFIG_MODE_MAP;
    } else if (!strcmp(tag_name, "mode_map")) {
        data->tag = TAG_CONFIG_VOICE;
    } else if (!strcmp(tag_name, "config_voice")) {
        data->tag = TAG_RESOURCE_MANAGER_INFO;
        data->voice_info_parsed = true;
    }
}

void ResourceManager::process_usecase()
{
    struct usecase_info usecase_data = {};
    usecase_data.config = {};
    usecase_data.priority = MIN_USECASE_PRIORITY;
    usecase_data.channel = 0;
    int size = 0;

    size = deviceInfo.size() - 1;
    usecase_data.ec_enable = deviceInfo[size].ec_enable;
    deviceInfo[size].usecase.push_back(usecase_data);
}

void ResourceManager::process_custom_config(const XML_Char **attr){
    struct usecase_custom_config_info custom_config_data = {};
    int size = 0, sizeusecase = 0;

    std::string key(attr[1]);

    custom_config_data.sndDevName = "";
    custom_config_data.channel = 0;
    custom_config_data.priority = MIN_USECASE_PRIORITY;
    custom_config_data.key = "";

    if (attr[0] && !strcmp(attr[0], "key")) {
        custom_config_data.key = key;
    }

    size = deviceInfo.size() - 1;
    sizeusecase = deviceInfo[size].usecase.size() - 1;
    custom_config_data.ec_enable = deviceInfo[size].usecase[sizeusecase].ec_enable;
    deviceInfo[size].usecase[sizeusecase].config.push_back(custom_config_data);
    PAL_DBG(LOG_TAG, "custom config key is %s", custom_config_data.key.c_str());
}

void ResourceManager::process_lpi_vote_streams(struct xml_userdata *data,
                                               const XML_Char *tag_name)
{
    if (data->offs <= 0 || data->resourcexml_parsed)
        return;

    data->data_buf[data->offs] = '\0';

    if (data->tag == TAG_LPI_VOTE_STREAM) {
        std::string stream_name(data->data_buf);
        PAL_DBG(LOG_TAG, "Stream name to be added : :%s", stream_name.c_str());
        uint32_t st = usecaseIdLUT.at(stream_name);
        sleep_monitor_vote_type_[st] = LPI_VOTE;
        PAL_DBG(LOG_TAG, "Stream type added : %d", st);
    } else if (data->tag == TAG_AVOID_VOTE_STREAM) {
        std::string stream_name(data->data_buf);
        PAL_DBG(LOG_TAG, "Stream name to be added : :%s", stream_name.c_str());
        uint32_t st = usecaseIdLUT.at(stream_name);
        sleep_monitor_vote_type_[st] = AVOID_VOTE;
        PAL_DBG(LOG_TAG, "Stream type added : %d", st);
    }

    if (!strcmp(tag_name, "low_power_stream_type") ||
        !strcmp(tag_name, "avoid_vote_stream_type")) {
        data->tag = TAG_SLEEP_MONITOR_LPI_STREAM;
    } else if (!strcmp(tag_name, "sleep_monitor_vote_streams")) {
        data->tag = TAG_RESOURCE_MANAGER_INFO;
    }

}

void ResourceManager::process_snd_card_standby_support_streams(struct xml_userdata *data,
                                                                const XML_Char *tag_name)
{
    if (data->offs <= 0 || data->resourcexml_parsed)
        return;

    data->data_buf[data->offs] = '\0';
    if (data->tag == TAG_STANDBY_STREAM_TYPE) {
        std::string stream_name(data->data_buf);
        PAL_DBG(LOG_TAG, "Stream name to be added : %s", stream_name.c_str());
        uint32_t st = usecaseIdLUT.at(stream_name);
        sndCardStandbySupportedStreams_.push_back(st);
        PAL_DBG(LOG_TAG, "Stream type added : %d", st);
    }

    if (!strcmp(tag_name, "snd_card_sb_stream_type")) {
        data->tag = TAG_STANDBY_SUPPORT_STREAMS;
    }
}

bool ResourceManager::isStreamSupportedInsndCardStandy(uint32_t type)
{
    return (find(sndCardStandbySupportedStreams_.begin(),
                 sndCardStandbySupportedStreams_.end(), type) !=
                 sndCardStandbySupportedStreams_.end());
}

uint32_t ResourceManager::palFormatToBitwidthLookup(const pal_audio_fmt_t format)
{
    audio_bit_width_t bit_width_ret = AUDIO_BIT_WIDTH_DEFAULT_16;
    switch (format) {
        case PAL_AUDIO_FMT_PCM_S8:
            bit_width_ret = AUDIO_BIT_WIDTH_8;
            break;
        case PAL_AUDIO_FMT_PCM_S24_3LE:
        case PAL_AUDIO_FMT_PCM_S24_LE:
            bit_width_ret = AUDIO_BIT_WIDTH_24;
            break;
        case PAL_AUDIO_FMT_PCM_S32_LE:
            bit_width_ret = AUDIO_BIT_WIDTH_32;
            break;
        default:
            break;
    }

    return static_cast<uint32_t>(bit_width_ret);
};

void ResourceManager::process_device_info(struct xml_userdata *data, const XML_Char *tag_name)
{

    struct deviceIn dev = {
        .bitFormatSupported = PAL_AUDIO_FMT_PCM_S16_LE,
        .ec_enable = true,
    };
    int size = 0 , sizeusecase = 0, sizecustomconfig = 0;

    if (data->offs <= 0)
        return;
    data->data_buf[data->offs] = '\0';

    if (data->resourcexml_parsed)
      return;

    if ((data->tag == TAG_IN_DEVICE) || (data->tag == TAG_OUT_DEVICE)) {
        if (!strcmp(tag_name, "id")) {
            std::string deviceName(data->data_buf);
            dev.deviceId  = deviceIdLUT.at(deviceName);
            deviceInfo.push_back(dev);
        } else if (!strcmp(tag_name, "back_end_name")) {
            std::string backendname(data->data_buf);
            size = deviceInfo.size() - 1;
            updateBackEndName(deviceInfo[size].deviceId, backendname);
        } else if (!strcmp(tag_name, "max_channels")) {
            size = deviceInfo.size() - 1;
            deviceInfo[size].max_channel = atoi(data->data_buf);
        } else if (!strcmp(tag_name, "channels")) {
            size = deviceInfo.size() - 1;
            deviceInfo[size].channel = atoi(data->data_buf);
        } else if (!strcmp(tag_name, "samplerate")) {
            size = deviceInfo.size() - 1;
            deviceInfo[size].samplerate = atoi(data->data_buf);
        } else if (!strcmp(tag_name, "snd_device_name")) {
            size = deviceInfo.size() - 1;
            std::string snddevname(data->data_buf);
            deviceInfo[size].sndDevName = snddevname;
            updateSndName(deviceInfo[size].deviceId, snddevname);
        } else if (!strcmp(tag_name, "qmp_enable")) {
            if (atoi(data->data_buf))
                isQmpEnabled = true;
        } else if (!strcmp(tag_name, "speaker_protection_enabled")) {
            if (atoi(data->data_buf))
                isSpeakerProtectionEnabled = true;
        } else if (!strcmp(tag_name, "handset_protection_enabled")) {
            if (atoi(data->data_buf))
                isHandsetProtectionEnabled = true;
        } else if (!strcmp(tag_name, "haptics_protection_enabled")) {
            if (atoi(data->data_buf))
                isHapticsProtectionEnabled = true;
        } else if (!strcmp(tag_name, "ext_ec_ref_enabled")) {
            size = deviceInfo.size() - 1;
            deviceInfo[size].isExternalECRefEnabled = atoi(data->data_buf);
            if (deviceInfo[size].isExternalECRefEnabled) {
                PAL_DBG(LOG_TAG, "found ext ec ref enabled device is %d",
                    deviceInfo[size].deviceId);
            }
        } else if (!strcmp(tag_name, "usb_uuid_based_tuning")) {
            size = deviceInfo.size() - 1;
            deviceInfo[size].isUSBUUIdBasedTuningEnabled = atoi(data->data_buf);
            if (deviceInfo[size].isUSBUUIdBasedTuningEnabled) {
                PAL_DBG(LOG_TAG, "found usb_uuid_based_tuning enabled device is %d",
                    deviceInfo[size].deviceId);
            }
        } else if (!strcmp(tag_name, "Charge_concurrency_enabled")) {
            if (atoi(data->data_buf))
                isChargeConcurrencyEnabled = true;
        } else if (!strcmp(tag_name, "cps_mode")) {
            cpsMode = atoi(data->data_buf);
        } else if (!strcmp(tag_name, "supported_bit_format")) {
            size = deviceInfo.size() - 1;
            if(!strcmp(data->data_buf, "PAL_AUDIO_FMT_PCM_S24_3LE"))
               deviceInfo[size].bitFormatSupported = PAL_AUDIO_FMT_PCM_S24_3LE;
            else if(!strcmp(data->data_buf, "PAL_AUDIO_FMT_PCM_S24_LE"))
               deviceInfo[size].bitFormatSupported = PAL_AUDIO_FMT_PCM_S24_LE;
            else if(!strcmp(data->data_buf, "PAL_AUDIO_FMT_PCM_S32_LE"))
               deviceInfo[size].bitFormatSupported = PAL_AUDIO_FMT_PCM_S32_LE;
            else
               deviceInfo[size].bitFormatSupported = PAL_AUDIO_FMT_PCM_S16_LE;
        } else if (!strcmp(tag_name, "vbat_enabled")) {
            if (atoi(data->data_buf))
                isVbatEnabled = true;
        }
        else if (!strcmp(tag_name, "bit_width")) {
            size = deviceInfo.size() - 1;
            deviceInfo[size].bit_width = atoi(data->data_buf);
            if (!isBitWidthSupported(deviceInfo[size].bit_width)) {
                PAL_ERR(LOG_TAG,"Invalid bit width %d setting to default BITWIDTH_16",
                        deviceInfo[size].bit_width);
                deviceInfo[size].bit_width = BITWIDTH_16;
            }
        }
        else if (!strcmp(tag_name, "speaker_mono_right")) {
            if (atoi(data->data_buf))
                isMainSpeakerRight = true;
        } else if (!strcmp(tag_name, "quick_cal_time")) {
            spQuickCalTime = atoi(data->data_buf);
        }else if (!strcmp(tag_name, "ras_enabled")) {
            if (atoi(data->data_buf))
                isRasEnabled = true;
        } else if (!strcmp(tag_name, "fractional_sr")) {
            size = deviceInfo.size() - 1;
            deviceInfo[size].fractionalSRSupported = atoi(data->data_buf);
        } else if (!strcmp(tag_name, "ec_enable")) {
            size = deviceInfo.size() - 1;
            deviceInfo[size].ec_enable = atoi(data->data_buf);
        }
    } else if (data->tag == TAG_USECASE) {
        if (!strcmp(tag_name, "name")) {
            std::string userIdname(data->data_buf);
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            deviceInfo[size].usecase[sizeusecase].type = usecaseIdLUT.at(userIdname);
        } else if (!strcmp(tag_name, "sidetone_mode")) {
            std::string mode(data->data_buf);
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            deviceInfo[size].usecase[sizeusecase].sidetoneMode = sidetoneModetoId.at(mode);
        } else if (!strcmp(tag_name, "snd_device_name")) {
            std::string sndDev(data->data_buf);
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            deviceInfo[size].usecase[sizeusecase].sndDevName = sndDev;
        } else if (!strcmp(tag_name, "channels")) {
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            deviceInfo[size].usecase[sizeusecase].channel = atoi(data->data_buf);
        } else if (!strcmp(tag_name, "samplerate")) {
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            deviceInfo[size].usecase[sizeusecase].samplerate =  atoi(data->data_buf);
        }  else if (!strcmp(tag_name, "priority")) {
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            deviceInfo[size].usecase[sizeusecase].priority = atoi(data->data_buf);
        }  else if (!strcmp(tag_name, "bit_width")) {
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            deviceInfo[size].usecase[sizeusecase].bit_width = atoi(data->data_buf);
            if (!isBitWidthSupported(deviceInfo[size].usecase[sizeusecase].bit_width)) {
                PAL_ERR(LOG_TAG,"Invalid bit width %d setting to default BITWIDTH_16",
                        deviceInfo[size].usecase[sizeusecase].bit_width);
                deviceInfo[size].usecase[sizeusecase].bit_width = BITWIDTH_16;
            }
        }  else if (!strcmp(tag_name, "ec_enable")) {
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            deviceInfo[size].usecase[sizeusecase].ec_enable = atoi(data->data_buf);
        }  else if (!strcmp(tag_name, "backend_name")) {
            std::string backendname(data->data_buf);
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            if (deviceInfo[size].deviceId == PAL_DEVICE_OUT_HAPTICS_DEVICE) {
                if (ResourceManager::isHapticsthroughWSA) {
                    updateBackEndName(deviceInfo[size].deviceId, backendname);
                }
            }
        }
    } else if (data->tag == TAG_ECREF) {
        if (!strcmp(tag_name, "id")) {
            std::string rxDeviceName(data->data_buf);
            pal_device_id_t rxDeviceId  = deviceIdLUT.at(rxDeviceName);
            std::vector<std::pair<Stream *, int>> str_list;
            str_list.clear();
            size = deviceInfo.size() - 1;
            deviceInfo[size].rx_dev_ids.push_back(rxDeviceId);
            deviceInfo[size].ec_ref_count_map.insert({rxDeviceId, str_list});
        }
    } else if (data->tag == TAG_VI_CHMAP) {
        if (!strcmp(tag_name, "channel")) {
            spViChannelMapCfg.push_back(atoi(data->data_buf));
        }
    } else if (data->tag == TAG_CUSTOMCONFIG) {
        if (!strcmp(tag_name, "snd_device_name")) {
            std::string sndDev(data->data_buf);
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            sizecustomconfig = deviceInfo[size].usecase[sizeusecase].config.size() - 1;
            deviceInfo[size].usecase[sizeusecase].config[sizecustomconfig].sndDevName = sndDev;
        }  else if (!strcmp(tag_name, "channels")) {
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            sizecustomconfig = deviceInfo[size].usecase[sizeusecase].config.size() - 1;
            deviceInfo[size].usecase[sizeusecase].config[sizecustomconfig].channel = atoi(data->data_buf);
        }  else if (!strcmp(tag_name, "samplerate")) {
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            sizecustomconfig = deviceInfo[size].usecase[sizeusecase].config.size() - 1;
            deviceInfo[size].usecase[sizeusecase].config[sizecustomconfig].samplerate = atoi(data->data_buf);
        } else if (!strcmp(tag_name, "sidetone_mode")) {
            std::string mode(data->data_buf);
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            sizecustomconfig = deviceInfo[size].usecase[sizeusecase].config.size() - 1;
            deviceInfo[size].usecase[sizeusecase].config[sizecustomconfig].sidetoneMode = sidetoneModetoId.at(mode);
        } else if (!strcmp(tag_name, "priority")) {
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            sizecustomconfig = deviceInfo[size].usecase[sizeusecase].config.size() - 1;
             deviceInfo[size].usecase[sizeusecase].config[sizecustomconfig].priority = atoi(data->data_buf);
        } else if (!strcmp(tag_name, "bit_width")) {
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            sizecustomconfig = deviceInfo[size].usecase[sizeusecase].config.size() - 1;
            deviceInfo[size].usecase[sizeusecase].config[sizecustomconfig].bit_width = atoi(data->data_buf);
            if (!isBitWidthSupported(deviceInfo[size].usecase[sizeusecase].config[sizecustomconfig].bit_width)) {
                PAL_ERR(LOG_TAG,"Invalid bit width %d setting to default BITWIDTH_16",
                        deviceInfo[size].usecase[sizeusecase].config[sizecustomconfig].bit_width);
                deviceInfo[size].usecase[sizeusecase].config[sizecustomconfig].bit_width = BITWIDTH_16;
            }
        } else if (!strcmp(tag_name, "ec_enable")) {
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            sizecustomconfig = deviceInfo[size].usecase[sizeusecase].config.size() - 1;
            deviceInfo[size].usecase[sizeusecase].config[sizecustomconfig].ec_enable = atoi(data->data_buf);
        } else if (!strcmp(tag_name, "backend_name")) {
            std::string backendname(data->data_buf);
            size = deviceInfo.size() - 1;
            sizeusecase = deviceInfo[size].usecase.size() - 1;
            sizecustomconfig = deviceInfo[size].usecase[sizeusecase].config.size() - 1;
            if (deviceInfo[size].deviceId == PAL_DEVICE_OUT_HAPTICS_DEVICE) {
                if (ResourceManager::isHapticsthroughWSA) {
                    updateBackEndName(deviceInfo[size].deviceId, backendname);
                }
            }
        }
    }
    if (!strcmp(tag_name, "usecase")) {
        data->tag = TAG_IN_DEVICE;
    } else if (!strcmp(tag_name, "in-device") || !strcmp(tag_name, "out-device")) {
        data->tag = TAG_DEVICE_PROFILE;
    } else if (!strcmp(tag_name, "device_profile")) {
        data->tag = TAG_RESOURCE_MANAGER_INFO;
    } else if (!strcmp(tag_name, "sidetone_mode")) {
        data->tag = TAG_USECASE;
    } else if (!strcmp(tag_name, "ec_rx_device")) {
        data->tag = TAG_ECREF;
    } else if (!strcmp(tag_name, "sp_vi_ch_map")) {
        data->tag = TAG_IN_DEVICE;
    } else if (!strcmp(tag_name, "resource_manager_info")) {
        data->tag = TAG_RESOURCE_ROOT;
        data->resourcexml_parsed = true;
    } else if (!strcmp(tag_name, "custom-config")) {
        data->tag = TAG_USECASE;
        data->inCustomConfig = 0;
    }
}

void ResourceManager::process_input_streams(struct xml_userdata *data, const XML_Char *tag_name)
{
    struct tx_ecinfo txecinfo = {};
    int type = 0;
    int size = -1;

    if (data->offs <= 0)
        return;
    data->data_buf[data->offs] = '\0';

    if (data->resourcexml_parsed)
      return;

    if (data->tag == TAG_INSTREAM) {
        if (!strcmp(tag_name, "name")) {
            std::string userIdname(data->data_buf);
            txecinfo.tx_stream_type  = usecaseIdLUT.at(userIdname);
            txEcInfo.push_back(txecinfo);
            PAL_DBG(LOG_TAG, "name %d", txecinfo.tx_stream_type);
        }
    } else if (data->tag == TAG_ECREF) {
        if (!strcmp(tag_name, "disabled_stream")) {
            std::string userIdname(data->data_buf);
            type  = usecaseIdLUT.at(userIdname);
            size = txEcInfo.size() - 1;
            txEcInfo[size].disabled_rx_streams.push_back(type);
            PAL_DBG(LOG_TAG, "ecref %d", type);
        }
    }
    if (!strcmp(tag_name, "in_streams")) {
        data->tag = TAG_INSTREAMS;
    } else if (!strcmp(tag_name, "in_stream")) {
        data->tag = TAG_INSTREAM;
    } else if (!strcmp(tag_name, "policies")) {
        data->tag = TAG_POLICIES;
    } else if (!strcmp(tag_name, "ec_ref")) {
        data->tag = TAG_ECREF;
    } else if (!strcmp(tag_name, "resource_manager_info")) {
        data->tag = TAG_RESOURCE_ROOT;
        data->resourcexml_parsed = true;
    }
}

void ResourceManager::process_group_device_config(struct xml_userdata *data, const char* tag, const char** attr)
{
    std::map<group_dev_config_idx_t, std::shared_ptr<group_dev_config_t>>::iterator it;
    std::shared_ptr<group_dev_config_t> group_device_config = NULL;

    PAL_DBG(LOG_TAG, "processing tag :%s", tag);
    if (!strcmp(tag, "upd_rx")) {
        data->group_dev_idx = GRP_UPD_RX;
        auto grp_dev_cfg = std::make_shared<group_dev_config_t>();
        groupDevConfigMap.insert(std::make_pair(GRP_UPD_RX, grp_dev_cfg));
    } else if (!strcmp(tag, "handset")) {
        data->group_dev_idx = GRP_HANDSET;
        auto grp_dev_cfg = std::make_shared<group_dev_config_t>();
        groupDevConfigMap.insert(std::make_pair(GRP_HANDSET, grp_dev_cfg));
    } else if (!strcmp(tag, "speaker")) {
        data->group_dev_idx = GRP_SPEAKER;
        auto grp_dev_cfg = std::make_shared<group_dev_config_t>();
        groupDevConfigMap.insert(std::make_pair(GRP_SPEAKER, grp_dev_cfg));
    }else if (!strcmp(tag, "speaker_voice")) {
        data->group_dev_idx = GRP_SPEAKER_VOICE;
        auto grp_dev_cfg = std::make_shared<group_dev_config_t>();
        groupDevConfigMap.insert(std::make_pair(GRP_SPEAKER_VOICE, grp_dev_cfg));
    } else if (!strcmp(tag, "upd_rx_handset")) {
        data->group_dev_idx = GRP_UPD_RX_HANDSET;
        auto grp_dev_cfg = std::make_shared<group_dev_config_t>();
        groupDevConfigMap.insert(std::make_pair(GRP_UPD_RX_HANDSET, grp_dev_cfg));
    } else if (!strcmp(tag, "upd_rx_speaker")) {
        data->group_dev_idx = GRP_UPD_RX_SPEAKER;
        auto grp_dev_cfg = std::make_shared<group_dev_config_t>();
        groupDevConfigMap.insert(std::make_pair(GRP_UPD_RX_SPEAKER, grp_dev_cfg));
    }

    if (!strcmp(tag, "snd_device")) {
        it = groupDevConfigMap.find(data->group_dev_idx);
        if (it != groupDevConfigMap.end()) {
            group_device_config =  it->second;
            if (group_device_config) {
                group_device_config->snd_dev_name = attr[1];
            }
        }
    } else if (!strcmp(tag, "devicepp_mfc")) {
        it = groupDevConfigMap.find(data->group_dev_idx);
        if (it != groupDevConfigMap.end()) {
            group_device_config =  it->second;
            if (group_device_config) {
                group_device_config->devpp_mfc_cfg.sample_rate = atoi(attr[1]);
                group_device_config->devpp_mfc_cfg.channels = atoi(attr[3]);
                group_device_config->devpp_mfc_cfg.bit_width = atoi(attr[5]);
            }
        }
    } else if (!strcmp(tag, "group_dev")) {
        it = groupDevConfigMap.find(data->group_dev_idx);
        if (it != groupDevConfigMap.end()) {
            group_device_config =  it->second;
            if (group_device_config) {
                group_device_config->grp_dev_hwep_cfg.sample_rate = atoi(attr[1]);
                group_device_config->grp_dev_hwep_cfg.channels = atoi(attr[3]);
                if(!strcmp(attr[5], "PAL_AUDIO_FMT_PCM_S24_3LE"))
                   group_device_config->grp_dev_hwep_cfg.aud_fmt_id = PAL_AUDIO_FMT_PCM_S24_3LE;
                else if(!strcmp(attr[5], "PAL_AUDIO_FMT_PCM_S24_LE"))
                   group_device_config->grp_dev_hwep_cfg.aud_fmt_id = PAL_AUDIO_FMT_PCM_S24_LE;
                else if(!strcmp(attr[5], "PAL_AUDIO_FMT_PCM_S32_LE"))
                   group_device_config->grp_dev_hwep_cfg.aud_fmt_id = PAL_AUDIO_FMT_PCM_S32_LE;
                else
                   group_device_config->grp_dev_hwep_cfg.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
                group_device_config->grp_dev_hwep_cfg.slot_mask = atoi(attr[7]);
            }
        }
    }
}

void ResourceManager::snd_process_data_buf(struct xml_userdata *data, const XML_Char *tag_name)
{
    if (data->offs <= 0)
        return;

    data->data_buf[data->offs] = '\0';

    if (data->card_parsed)
        return;

    if (data->current_tag == TAG_ROOT)
        return;

    if (data->current_tag == TAG_CARD) {
        processCardInfo(data, tag_name);
    } else if (data->current_tag == TAG_PLUGIN) {
        //snd_parse_plugin_properties(data, tag_name);
    } else if (data->current_tag == TAG_DEVICE) {
        //PAL_ERR(LOG_TAG,"tag %s", (char*)tag_name);
        processDeviceIdProp(data, tag_name);
    } else if (data->current_tag == TAG_DEV_PROPS) {
        processDeviceCapability(data, tag_name);
    }
}

void ResourceManager::setGaplessMode(const XML_Char **attr)
{
    if (strcmp(attr[0], "key") != 0) {
        PAL_ERR(LOG_TAG, "key not found");
        return;
    }
    if (strcmp(attr[2], "value") != 0) {
        PAL_ERR(LOG_TAG, "value not found");
        return;
    }
    if (atoi(attr[3])) {
       isGaplessEnabled = true;
       return;
    }
}

void ResourceManager::startTag(void *userdata, const XML_Char *tag_name,
                               const XML_Char **attr)
{
    stream_supported_type type;
    struct xml_userdata *data = (struct xml_userdata *)userdata;
    static std::shared_ptr<SoundTriggerPlatformInfo> st_info = nullptr;

    if (st_info && data->is_parsing_sound_trigger) {
        st_info->HandleStartTag((const char *)tag_name, (const char **)attr);
        snd_reset_data_buf(data);
        return;
    }

    if (data->is_parsing_group_device) {
        process_group_device_config(data, (const char *)tag_name, (const char **)attr);
        snd_reset_data_buf(data);
        return;
    }

    if (!strcmp(tag_name, "sound_trigger_platform_info")) {
        data->is_parsing_sound_trigger = true;
        st_info = SoundTriggerPlatformInfo::GetInstance();
        return;
    }

    if (!strcmp(tag_name, "group_device_cfg")) {
        if (ResourceManager::isUPDVirtualPortEnabled)
            data->is_parsing_group_device = true;
        return;
    }

    if (strcmp(tag_name, "device") == 0) {
        return;
    } else if(strcmp(tag_name, "param") == 0) {
        processConfigParams(attr);
    } else if (strcmp(tag_name, "codec") == 0) {
        processBTCodecInfo(attr, XML_GetSpecifiedAttributeCount(data->parser));
        return;
    } else if (strcmp(tag_name, "config_gapless") == 0) {
        setGaplessMode(attr);
        return;
    } else if(strcmp(tag_name, "temp_ctrl") == 0) {
        processSpkrTempCtrls(attr);
        return;
    } else if (!strcmp(tag_name, "usb_vendor")) {
        if (attr[1])
            usb_vendor_uuid_list.push_back(attr[1]);
        return;
    } else if (!strcmp(tag_name, "perf_lock")) {
        processPerfLockConfig(attr);
        return;
    } else if (!strcmp(tag_name, "silence_detection_config")){
        processSilenceDetectionConfig(attr);
        return;
    }

    if (data->card_parsed)
        return;

    snd_reset_data_buf(data);

    if (!strcmp(tag_name, "resource_manager_info")) {
        data->tag = TAG_RESOURCE_MANAGER_INFO;
    } else if (!strcmp(tag_name, "config_voice")) {
        data->tag = TAG_CONFIG_VOICE;
    } else if (!strcmp(tag_name, "mode_map")) {
        data->tag = TAG_CONFIG_MODE_MAP;
    } else if (!strcmp(tag_name, "modepair")) {
        data->tag = TAG_CONFIG_MODE_PAIR;
        process_voicemode_info(attr);
    } else if (!strcmp(tag_name, "gain_db_to_level_mapping")) {
        data->tag = TAG_GAIN_LEVEL_MAP;
    } else if (!strcmp(tag_name, "gain_level_map")) {
        data->tag = TAG_GAIN_LEVEL_PAIR;
        process_gain_db_to_level_map(data, attr);
    } else if (!strcmp(tag_name, "device_profile")) {
        data->tag = TAG_DEVICE_PROFILE;
    } else if (!strcmp(tag_name, "in-device")) {
        data->tag = TAG_IN_DEVICE;
    } else if (!strcmp(tag_name, "out-device")) {
        data->tag = TAG_OUT_DEVICE;
    } else if (!strcmp(tag_name, "usecase")) {
        process_usecase();
        data->tag = TAG_USECASE;
    } else if (!strcmp(tag_name, "in_streams")) {
        data->tag = TAG_INSTREAMS;
    } else if (!strcmp(tag_name, "in_stream")) {
        data->tag = TAG_INSTREAM;
    } else if (!strcmp(tag_name, "policies")) {
        data->tag = TAG_POLICIES;
    } else if (!strcmp(tag_name, "ec_ref")) {
        data->tag = TAG_ECREF;
    } else if (!strcmp(tag_name, "ec_rx_device")) {
        data->tag = TAG_ECREF;
    } else if(!strcmp(tag_name, "sp_vi_ch_map")) {
        data->tag = TAG_VI_CHMAP;
    } else if (!strcmp(tag_name, "sidetone_mode")) {
        data->tag = TAG_USECASE;
    } else if (!strcmp(tag_name, "low_power_stream_type")) {
        data->tag = TAG_LPI_VOTE_STREAM;
    } else if (!strcmp(tag_name, "avoid_vote_stream_type")) {
        data->tag = TAG_AVOID_VOTE_STREAM;
    } else if (!strcmp(tag_name, "sleep_monitor_vote_streams")) {
         data->tag = TAG_SLEEP_MONITOR_LPI_STREAM;
    } else if (!strcmp(tag_name, "snd_card_standby_support_streams")) {
         data->tag = TAG_STANDBY_SUPPORT_STREAMS;
    } else if (!strcmp(tag_name, "snd_card_sb_stream_type")) {
         data->tag = TAG_STANDBY_STREAM_TYPE;
    } else if (!strcmp(tag_name, "custom-config")) {
        process_custom_config(attr);
        data->inCustomConfig = 1;
        data->tag = TAG_CUSTOMCONFIG;
    } else if (!strcmp(tag_name, "config_volume")) {
        data->tag = TAG_CONFIG_VOLUME;
    } else if (!strcmp(tag_name, "supported_streams")) {
        data->tag = TAG_CONFIG_VOLUME_SET_PARAM_SUPPORTED_STREAMS;
    } else if (!strcmp(tag_name, "supported_stream")) {
        data->tag = TAG_CONFIG_VOLUME_SET_PARAM_SUPPORTED_STREAM;
    } else if (!strcmp(tag_name, "config_lpm")) {
        data->tag = TAG_CONFIG_LPM;
    } else if (!strcmp(tag_name, "lpm_supported_streams")) {
        data->tag = TAG_CONFIG_LPM_SUPPORTED_STREAMS;
    } else if (!strcmp(tag_name, "lpm_supported_stream")) {
        data->tag = TAG_CONFIG_LPM_SUPPORTED_STREAM;
    }

    if (!strcmp(tag_name, "card"))
        data->current_tag = TAG_CARD;
    if (strcmp(tag_name, "pcm-device") == 0) {
        type = PCM;
        data->current_tag = TAG_DEVICE;
    } else if (strcmp(tag_name, "compress-device") == 0) {
        data->current_tag = TAG_DEVICE;
        type = COMPRESS;
    } else if (strcmp(tag_name, "mixer") == 0) {
        data->current_tag = TAG_MIXER;
    } else if (strstr(tag_name, "plugin")) {
        data->current_tag = TAG_PLUGIN;
    } else if (!strcmp(tag_name, "props")) {
        data->current_tag = TAG_DEV_PROPS;
    }
    if (data->current_tag != TAG_CARD && !data->card_found)
        return;
}

void ResourceManager::endTag(void *userdata, const XML_Char *tag_name)
{
    struct xml_userdata *data = (struct xml_userdata *)userdata;
    std::shared_ptr<SoundTriggerPlatformInfo> st_info =
        SoundTriggerPlatformInfo::GetInstance();

    if (!strcmp(tag_name, "sound_trigger_platform_info")) {
        data->is_parsing_sound_trigger = false;
        return;
    }

    if (data->is_parsing_sound_trigger) {
        st_info->HandleEndTag(data, (const char *)tag_name);
        snd_reset_data_buf(data);
        return;
    }

    if (!strcmp(tag_name, "group_device_cfg")) {
        data->is_parsing_group_device = false;
        return;
    }

    process_config_voice(data,tag_name);
    process_device_info(data,tag_name);
    process_input_streams(data,tag_name);
    process_lpi_vote_streams(data, tag_name);
    process_snd_card_standby_support_streams(data, tag_name);
    process_config_volume(data, tag_name);
    process_config_lpm(data, tag_name);

    if (data->card_parsed)
        return;
    if (data->current_tag != TAG_CARD && !data->card_found)
        return;
    snd_process_data_buf(data, tag_name);
    snd_reset_data_buf(data);
    if (!strcmp(tag_name, "mixer") || !strcmp(tag_name, "pcm-device") || !strcmp(tag_name, "compress-device"))
        data->current_tag = TAG_CARD;
    else if (strstr(tag_name, "plugin") || !strcmp(tag_name, "props"))
        data->current_tag = TAG_DEVICE;
    else if(!strcmp(tag_name, "card")) {
        data->current_tag = TAG_ROOT;
        if (data->card_found)
            data->card_parsed = true;
    }
}

void ResourceManager::snd_data_handler(void *userdata, const XML_Char *s, int len)
{
   struct xml_userdata *data = (struct xml_userdata *)userdata;

   if (len + data->offs >= sizeof(data->data_buf) ) {
       data->offs += len;
       /* string length overflow, return */
       return;
   } else {
       memcpy(data->data_buf + data->offs, s, len);
       data->offs += len;
   }
}

int ResourceManager::XmlParser(std::string xmlFile)
{
    XML_Parser parser;
    FILE *file = NULL;
    int ret = 0;
    int bytes_read;
    void *buf = NULL;
    struct xml_userdata data;
    memset(&data, 0, sizeof(data));

    PAL_INFO(LOG_TAG, "XML parsing started - file name %s", xmlFile.c_str());
    file = fopen(xmlFile.c_str(), "r");
    if(!file) {
        ret = -ENOENT;
        PAL_ERR(LOG_TAG, "Failed to open xml file name %s ret %d", xmlFile.c_str(), ret);
        goto done;
    }

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        ret = -EINVAL;
        PAL_ERR(LOG_TAG, "Failed to create XML ret %d", ret);
        goto closeFile;
    }

    data.parser = parser;
    XML_SetUserData(parser, &data);
    XML_SetElementHandler(parser, startTag, endTag);
    XML_SetCharacterDataHandler(parser, snd_data_handler);

    while (1) {
        buf = XML_GetBuffer(parser, 1024);
        if(buf == NULL) {
            ret = -EINVAL;
            PAL_ERR(LOG_TAG, "XML_Getbuffer failed ret %d", ret);
            goto freeParser;
        }

        bytes_read = fread(buf, 1, 1024, file);
        if(bytes_read < 0) {
            ret = -EINVAL;
            PAL_ERR(LOG_TAG, "fread failed ret %d", ret);
            goto freeParser;
        }

        if(XML_ParseBuffer(parser, bytes_read, bytes_read == 0) == XML_STATUS_ERROR) {
            ret = -EINVAL;
            PAL_ERR(LOG_TAG, "XML ParseBuffer failed for %s file ret %d", xmlFile.c_str(), ret);
            goto freeParser;
        }
        if (bytes_read == 0)
            break;
    }

freeParser:
    XML_ParserFree(parser);
closeFile:
    fclose(file);
done:
    return ret;
}

/* Function to get audio vendor configs path */
void ResourceManager::getVendorConfigPath (char* config_file_path, int path_size)
{
   char vendor_sku[PROPERTY_VALUE_MAX] = {'\0'};
   if (property_get("ro.boot.product.vendor.sku", vendor_sku, "") <= 0) {
#if defined(FEATURE_IPQ_OPENWRT) || defined(LINUX_ENABLED)
       /* Audio configs are stored in /etc */
       snprintf(config_file_path, path_size, "%s", "/etc");
#else
       /* Audio configs are stored in /vendor/etc */
       snprintf(config_file_path, path_size, "%s", "/vendor/etc");
#endif
    } else {
       /* Audio configs are stored in /vendor/etc/audio/sku_${vendor_sku} */
       snprintf(config_file_path, path_size,
                       "%s%s", "/vendor/etc/audio/sku_", vendor_sku);
    }
}

void ResourceManager::restoreDevice(std::shared_ptr<Device> dev)
{
    int status = 0;
    std::vector <std::tuple<Stream *, uint32_t>> sharedBEStreamDev;
    struct pal_device newDevAttr;
    struct pal_device curDevAttr;
    std::vector <std::shared_ptr<Device>> streamDevices;
    struct pal_stream_attributes sAttr;
    Stream *sharedStream = nullptr;
    std::vector <std::tuple<Stream *, uint32_t>> streamDevDisconnect;
    std::vector <std::tuple<Stream *, struct pal_device *>> streamDevConnect;
    std::vector <Stream *> streamsToSwitch;
    std::vector <Stream*>::iterator sIter;
#ifdef SEC_AUDIO_COMMON
    char preSndDevName[DEVICE_NAME_MAX_SIZE] = {0};
#endif

    PAL_DBG(LOG_TAG, "Enter");

    if (!dev) {
        PAL_ERR(LOG_TAG, "invalid dev cannot restore device");
        goto exit;
    }

#ifndef SEC_AUDIO_COMMON
    // Below code is added for QC Patch(3891320), remove it with QC case#05961502
    if (isPluginPlaybackDevice((pal_device_id_t)dev->getSndDeviceId()) &&
        (dev->getDeviceCount() != 0)) {
        PAL_ERR(LOG_TAG, "don't restore device for usb/3.5 hs playback");
        goto exit;
    }
#endif
    // if haptics device to be stopped, check and restore headset device config
    if (dev->getSndDeviceId() == PAL_DEVICE_OUT_HAPTICS_DEVICE &&
                                      !ResourceManager::isHapticsthroughWSA) {
        curDevAttr.id = PAL_DEVICE_OUT_WIRED_HEADSET;
        dev = Device::getInstance(&curDevAttr, rm);
        if (!dev) {
            PAL_ERR(LOG_TAG, "Getting headset device instance failed");
            goto exit;
        }
    }

    /*get current running device info*/
    dev->getDeviceAttributes(&curDevAttr);
    PAL_DBG(LOG_TAG,"current device: %d running at, ch %d, sr %d, bit_width %d, fmt %d, sndDev %s",
                    curDevAttr.id,
                    curDevAttr.config.ch_info.channels,
                    curDevAttr.config.sample_rate,
                    curDevAttr.config.bit_width,
                    curDevAttr.config.aud_fmt_id,
                    curDevAttr.sndDevName);
#ifdef SEC_AUDIO_COMMON
    strlcpy(preSndDevName, curDevAttr.sndDevName, DEVICE_NAME_MAX_SIZE);
#endif

    mActiveStreamMutex.lock();
    // check if need to update active group devcie config when usecase goes aways
    // if stream device is with same virtual backend, it can be handled in shared backend case
    if (dev->getDeviceCount() == 0) {
        status = checkAndUpdateGroupDevConfig(&curDevAttr, &sAttr, streamsToSwitch, &newDevAttr, false);
        if (status) {
            PAL_ERR(LOG_TAG, "no valid group device config found");
            streamsToSwitch.clear();
        }
    }

    /*
     * special case to update newDevAttr for haptics + headset concurrency
     * in case there're two or more active streams on headset and one of them goes away
     * still need to check if haptics is active and keep headset sample rate as 48K
     */
    if (dev->getSndDeviceId() == PAL_DEVICE_OUT_WIRED_HEADSET &&
                                       !ResourceManager::isHapticsthroughWSA) {
        newDevAttr.id = PAL_DEVICE_OUT_WIRED_HEADSET;
        dev = Device::getInstance(&newDevAttr, rm);
        if (!dev) {
            PAL_ERR(LOG_TAG, "Getting headset device instance failed");
            mActiveStreamMutex.unlock();
            goto exit;
        }
        dev->getDeviceAttributes(&newDevAttr);
        checkHapticsConcurrency(&newDevAttr, NULL, streamsToSwitch, NULL);
    }

    getSharedBEActiveStreamDevs(sharedBEStreamDev, dev->getSndDeviceId());
    if (sharedBEStreamDev.size() > 0) {
        /* check to see if curDevAttr changed, curDevAttr will be updated if switch is needed */
        bool switchStreams = compareSharedBEStreamDevAttr(sharedBEStreamDev, &curDevAttr, false/* disable device */);
        if (switchStreams && isDeviceReady(curDevAttr.id)) {
            /*device switch every stream to new dev attr*/
            for (const auto &elem : sharedBEStreamDev) {
                 sharedStream = std::get<0>(elem);
                 streamDevDisconnect.push_back({sharedStream,dev->getSndDeviceId()});
                 streamDevConnect.push_back({sharedStream,&curDevAttr});
            }
        }

        if (!streamDevDisconnect.empty()) {
            PAL_DBG(LOG_TAG,"Restore required");
            PAL_DBG(LOG_TAG,"switched to dev: %d, attr are, ch %d, sr %d, bit_width %d, fmt %d, sndDev %s",
                            curDevAttr.id,
                            curDevAttr.config.ch_info.channels,
                            curDevAttr.config.sample_rate,
                            curDevAttr.config.bit_width,
                            curDevAttr.config.aud_fmt_id,
                            curDevAttr.sndDevName);
        } else {
            PAL_DBG(LOG_TAG,"device switch not needed params are all the same");
        }
        if (status) {
            PAL_ERR(LOG_TAG,"device switch failed with %d", status);
        }
    } else {
        if (!streamsToSwitch.empty()) {
            for(sIter = streamsToSwitch.begin(); sIter != streamsToSwitch.end(); sIter++) {
                streamDevDisconnect.push_back({(*sIter), newDevAttr.id});
                streamDevConnect.push_back({(*sIter), &newDevAttr});
            }
        } else {
            PAL_DBG(LOG_TAG, "no active device, switch un-needed");
        }
    }

    mActiveStreamMutex.unlock();

#ifdef SEC_AUDIO_COMMON
    // Below code is added for QC Patch(3891320) at the beginning of this function
    if ((isPluginPlaybackDevice((pal_device_id_t)dev->getSndDeviceId()) ||
        isPluginCaptureDevice((pal_device_id_t)dev->getSndDeviceId())) &&
        (dev->getDeviceCount() != 0)) {
        if (!strcmp(preSndDevName, curDevAttr.sndDevName)) { // added by samsung
            PAL_ERR(LOG_TAG, "don't restore device for usb/3.5 device");
            goto exit;
        }
    }
#endif

    if (!streamDevDisconnect.empty())
        streamDevSwitch(streamDevDisconnect, streamDevConnect);
exit:
    PAL_DBG(LOG_TAG, "Exit");
    return;
}

bool ResourceManager::doDevAttrDiffer(struct pal_device *inDevAttr,
                                      struct pal_device *curDevAttr)
{
    bool ret = false;
    std::shared_ptr<Device> dev = nullptr;

    if (!inDevAttr->id || !curDevAttr->id) {
        PAL_DBG(LOG_TAG, "Invalid input or output device attribute");
        goto exit;
    }

    dev = Device::getInstance(curDevAttr, rm);
    if (!dev) {
        PAL_ERR(LOG_TAG, "No device instance found");
        goto exit;
    }

    /* if it's group device, compare group config to decide device switch */
    if (ResourceManager::activeGroupDevConfig &&
            (inDevAttr->id == PAL_DEVICE_OUT_SPEAKER ||
             inDevAttr->id == PAL_DEVICE_OUT_HANDSET)) {
        uint32_t in_sample_rate = 0;
        uint32_t in_channels  = 0;
        if (ResourceManager::activeGroupDevConfig->grp_dev_hwep_cfg.sample_rate == 0)
            in_sample_rate = inDevAttr->config.sample_rate;
        else
            in_sample_rate = ResourceManager::activeGroupDevConfig->grp_dev_hwep_cfg.sample_rate;
        if (in_sample_rate !=
            ResourceManager::currentGroupDevConfig.grp_dev_hwep_cfg.sample_rate) {
            PAL_DBG(LOG_TAG, "found diff sample rate %d, running dev has %d, device switch needed",
                    in_sample_rate,
                    ResourceManager::currentGroupDevConfig.grp_dev_hwep_cfg.sample_rate);
            ret = true;
        }
        if (ResourceManager::activeGroupDevConfig->grp_dev_hwep_cfg.channels == 0)
            in_channels = inDevAttr->config.ch_info.channels;
        else
            in_channels = ResourceManager::activeGroupDevConfig->grp_dev_hwep_cfg.channels;
        if (in_channels !=
            ResourceManager::currentGroupDevConfig.grp_dev_hwep_cfg.channels) {
            PAL_DBG(LOG_TAG, "found diff channel %d, running dev has %d, device switch needed",
                    in_channels,
                    ResourceManager::currentGroupDevConfig.grp_dev_hwep_cfg.channels);
            ret = true;
        }
        if (ResourceManager::activeGroupDevConfig->grp_dev_hwep_cfg.aud_fmt_id !=
            ResourceManager::currentGroupDevConfig.grp_dev_hwep_cfg.aud_fmt_id) {
            PAL_DBG(LOG_TAG, "found diff format %d, running dev has %d, device switch needed",
                    ResourceManager::activeGroupDevConfig->grp_dev_hwep_cfg.aud_fmt_id,
                    ResourceManager::currentGroupDevConfig.grp_dev_hwep_cfg.aud_fmt_id);
            ret = true;
        }
        if (ResourceManager::activeGroupDevConfig->grp_dev_hwep_cfg.slot_mask !=
            ResourceManager::currentGroupDevConfig.grp_dev_hwep_cfg.slot_mask) {
            PAL_DBG(LOG_TAG, "found diff slot mask %d, running dev has %d, device switch needed",
                    ResourceManager::activeGroupDevConfig->grp_dev_hwep_cfg.slot_mask,
                    ResourceManager::currentGroupDevConfig.grp_dev_hwep_cfg.slot_mask);
            ret = true;
        }
        if (strcmp(ResourceManager::activeGroupDevConfig->snd_dev_name.c_str(),
                   ResourceManager::currentGroupDevConfig.snd_dev_name.c_str())) {
            PAL_DBG(LOG_TAG, "found new snd device %s, device switch needed",
                    ResourceManager::activeGroupDevConfig->snd_dev_name.c_str());
            ret = true;
        }
        /* special case when we are switching with shared BE
         * always switch all to incoming device
         */
        if (inDevAttr->id != curDevAttr->id) {
            PAL_DBG(LOG_TAG, "found diff in device id cur dev %d incomming dev %d, device switch needed",
                    curDevAttr->id, inDevAttr->id);
            ret = true;
        }
        return ret;
    }

    if (inDevAttr->config.sample_rate != curDevAttr->config.sample_rate) {
        PAL_DBG(LOG_TAG, "found diff sample rate %d, running dev has %d, device switch needed",
                inDevAttr->config.sample_rate, curDevAttr->config.sample_rate);
        ret = true;
    }
    if (inDevAttr->config.bit_width != curDevAttr->config.bit_width) {
        PAL_DBG(LOG_TAG, "found diff bit width %d, running dev has %d, device switch needed",
                inDevAttr->config.bit_width, curDevAttr->config.bit_width);
        ret = true;
    }
    if (inDevAttr->config.ch_info.channels != curDevAttr->config.ch_info.channels) {
        PAL_DBG(LOG_TAG, "found diff channels %d, running dev has %d, device switch needed",
                inDevAttr->config.ch_info.channels, curDevAttr->config.ch_info.channels);
        ret = true;
    }
    if ((strcmp(inDevAttr->sndDevName, curDevAttr->sndDevName) != 0)) {
        PAL_DBG(LOG_TAG, "found new snd device %s, device switch needed",
                inDevAttr->sndDevName);
        ret = true;
    }
    /* special case when we are switching with shared BE
     * always switch all to incoming device
     */
    if (inDevAttr->id != curDevAttr->id) {
        PAL_DBG(LOG_TAG, "found diff in device id cur dev %d incomming dev %d, device switch needed",
                curDevAttr->id, inDevAttr->id);
        ret = true;
    }

    if (ret &&
        (inDevAttr->id == PAL_DEVICE_OUT_WIRED_HEADSET ||
         inDevAttr->id == PAL_DEVICE_OUT_WIRED_HEADPHONE) &&
        !ResourceManager::isHapticsthroughWSA) {
        // double check if the SR we are going to switch is supported by haptics.
        ret = checkDeviceSwitchForHaptics(inDevAttr, curDevAttr);
    }
    // special case for A2DP/BLE device to override device switch
    if (((inDevAttr->id == PAL_DEVICE_OUT_BLUETOOTH_A2DP) &&
        (curDevAttr->id == PAL_DEVICE_OUT_BLUETOOTH_A2DP)) ||
        ((inDevAttr->id == PAL_DEVICE_OUT_BLUETOOTH_BLE) &&
        (curDevAttr->id == PAL_DEVICE_OUT_BLUETOOTH_BLE)) ||
        ((inDevAttr->id == PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST) &&
        (curDevAttr->id == PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST)) ||
        ((inDevAttr->id == PAL_DEVICE_IN_BLUETOOTH_BLE) &&
        (curDevAttr->id == PAL_DEVICE_IN_BLUETOOTH_BLE))) {
        pal_param_bta2dp_t *param_bt_a2dp = nullptr;

        if (isDeviceAvailable(inDevAttr->id)) {
            dev = Device::getInstance(inDevAttr , rm);
            if (dev && !(dev->getDeviceParameter(PAL_PARAM_ID_BT_A2DP_FORCE_SWITCH, (void **)&param_bt_a2dp))) {
                if (param_bt_a2dp) {
                    ret = param_bt_a2dp->is_force_switch;
                    PAL_INFO(LOG_TAG, "A2DP force device switch is %d", ret);
                }
            } else {
                PAL_ERR(LOG_TAG, "get A2DP force device switch device parameter failed");
            }
        }
    }

exit:
    return ret;
}

bool ResourceManager::checkDeviceSwitchForHaptics(struct pal_device *inDevAttr,
                                                  struct pal_device *curDevAttr) {
    std::vector <Stream *> activeHapticsStreams;
    int ret = true;

    struct pal_device hapticsDattr;
    std::shared_ptr<Device> hapticsDev = nullptr;

    hapticsDattr.id = PAL_DEVICE_OUT_HAPTICS_DEVICE;
    hapticsDev = Device::getInstance(&hapticsDattr, rm);

    if (!hapticsDev) {
        PAL_ERR(LOG_TAG, "Getting Device instance failed");
        return ret;
    }
    getActiveStream_l(activeHapticsStreams, hapticsDev);
    if (activeHapticsStreams.size()) {
        hapticsDev->getDeviceAttributes(&hapticsDattr);
        if ((inDevAttr->config.sample_rate % SAMPLINGRATE_44K == 0) &&
            (curDevAttr->config.sample_rate % SAMPLINGRATE_44K != 0) &&
            (hapticsDattr.config.sample_rate % SAMPLINGRATE_44K != 0)) {
            PAL_DBG(LOG_TAG, "haptics is running, can't switch to non-supporting SR");
            ret = false;
        }
    }
    return ret;
}

int32_t  ResourceManager::getActiveVoiceCallDevices(std::vector <std::shared_ptr<Device>> &devices) {
    std::list<Stream*>::iterator it;
    pal_stream_attributes sAttr;
    int status = 0;

    for(it = mActiveStreams.begin(); it != mActiveStreams.end(); it++) {
        (*it)->getStreamAttributes(&sAttr);
        status = (*it)->getStreamAttributes(&sAttr);
        if (status != 0) {
            PAL_ERR(LOG_TAG,"stream get attributes failed");
            goto exit;
        }
        if(sAttr.type == PAL_STREAM_VOICE_CALL)
        {
            (*it)->getAssociatedDevices(devices);
            if (devices.empty()) {
                PAL_ERR(LOG_TAG, "Voice stream is not assoicated with a device");
                status = -EINVAL;
            }
            break;
        }
    }
exit:
    return status;
}

int32_t ResourceManager::reconfigureInCallMusicStream(struct sessionToPayloadParam deviceData) {
    int status = 0;
    std::list<Stream*>::iterator it;
    StreamInCall *sInCall = nullptr;
    struct pal_stream_attributes sAttr;

    for (it = mActiveStreams.begin(); it != mActiveStreams.end(); it++) {
        (*it)->getStreamAttributes(&sAttr);
        status = (*it)->getStreamAttributes(&sAttr);
        if (status != 0) {
            PAL_ERR(LOG_TAG, "stream get attributes failed");
            goto exit;
        }
        if (sAttr.type == PAL_STREAM_VOICE_CALL_MUSIC &&
        sAttr.info.incall_music_info.local_playback) {
            PAL_INFO(LOG_TAG, "found incall stream to configure");
            sInCall = dynamic_cast<StreamInCall*>(*it);
            sInCall->reconfigureModule(PER_STREAM_PER_DEVICE_MFC, "ZERO", &deviceData);
            break;
        }
    }
    if (!sInCall) {
        PAL_DBG(LOG_TAG, "No In-Call Music Stream found to configure");
    }
exit:
    return status;
}

int32_t ResourceManager::resumeInCallMusic() {
    int status = 0;
    std::list<Stream*>::iterator it;
    StreamInCall *sInCall = nullptr;
    struct pal_stream_attributes sAttr;

    for (it = mActiveStreams.begin(); it != mActiveStreams.end(); it++) {
        (*it)->getStreamAttributes(&sAttr);
        status = (*it)->getStreamAttributes(&sAttr);
        if (status != 0) {
            PAL_ERR(LOG_TAG, "stream get attributes failed");
            goto exit;
        }
        if (sAttr.type == PAL_STREAM_VOICE_CALL_MUSIC &&
        sAttr.info.incall_music_info.local_playback) {
            PAL_INFO(LOG_TAG, "found incall stream to resume");
            sInCall = dynamic_cast<StreamInCall*>(*it);
            sInCall->resume();
            break;
        }
    }
    if (!sInCall) {
        PAL_DBG(LOG_TAG, "No In-Call Music Stream found to resume");
    }
exit:
    return status;
}

int32_t ResourceManager::pauseInCallMusic() {
    int status = 0;
    std::list<Stream*>::iterator it;
    StreamInCall *sInCall = nullptr;
    struct pal_stream_attributes sAttr;

    for (it = mActiveStreams.begin(); it != mActiveStreams.end(); it++) {
        (*it)->getStreamAttributes(&sAttr);
        status = (*it)->getStreamAttributes(&sAttr);
        if (status != 0) {
            PAL_ERR(LOG_TAG, "stream get attributes failed");
            goto exit;
        }
        if (sAttr.type == PAL_STREAM_VOICE_CALL_MUSIC &&
        sAttr.info.incall_music_info.local_playback) {
            PAL_INFO(LOG_TAG, "found incall stream to pause and flush");
            sInCall = dynamic_cast<StreamInCall*>(*it);
            sInCall->pause();
            sInCall->flush();
            break;
        }
    }
    if (!sInCall) {
        PAL_DBG(LOG_TAG, "No In-Call Music Stream found to pause");
    }
exit:
    return status;
}

void ResourceManager::RegisterSTCaptureHandle(pal_param_st_capture_info_t stCaptureInfo,
                                              bool start) {
    PAL_DBG(LOG_TAG, "start %d, capture handle %d, pal handle %pK",
                          start, stCaptureInfo.capture_handle, stCaptureInfo.pal_handle);
    if (start) {
        mStCaptureInfo[stCaptureInfo.capture_handle] = stCaptureInfo.pal_handle;
    } else {
        mStCaptureInfo.erase(stCaptureInfo.capture_handle);
    }
}

void ResourceManager::setProxyRecordActive(bool isActive) {
    isProxyRecordActive = isActive;
}

void ResourceManager::reconfigureScoStreams() {
    int status = 0;
    std::shared_ptr <Device> dev = nullptr;
    std::vector <pal_device_id_t> scoDevs;
    std::vector <Stream *> activeScoStreams;
    std::vector <Stream *>::iterator sIter;
    std::vector <std::tuple<Stream*, uint32_t>> streamDevDisconnect;
    std::vector <std::tuple<Stream*, struct pal_device *>> streamDevConnect;
    std::vector <pal_device *> palDevices;
    struct pal_device *dattr = nullptr;
    std::vector <std::tuple<Stream *, uint32_t>>::iterator sIter1;

    scoDevs.push_back(PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET);
    scoDevs.push_back(PAL_DEVICE_OUT_BLUETOOTH_SCO);
    for (auto devId : scoDevs) {
        dattr = (pal_device *) malloc(sizeof(struct pal_device));
        if (!dattr) {
            PAL_ERR(LOG_TAG, "malloc failed for SCO device %d", devId);
        }
        palDevices.push_back(dattr);
        dattr->id = devId;
        status = getDeviceConfig(dattr, NULL);
        if (status) {
            PAL_ERR(LOG_TAG, "getDeviceConfig failed for SCO device %d", devId);
                return;
        }
        dev = Device::getInstance(dattr, rm);
        activeScoStreams.clear();
        mActiveStreamMutex.lock();
        getActiveStream_l(activeScoStreams, dev);

        if (activeScoStreams.size() > 0) {
            for (sIter = activeScoStreams.begin();
                sIter != activeScoStreams.end(); sIter++) {
                status = increaseStreamUserCounter(*sIter);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Error incrementing the stream counter for the stream handle: %pK", *sIter);
                    continue;
                }
                streamDevDisconnect.push_back({*sIter, devId});
                streamDevConnect.push_back({*sIter, dattr});
            }
        }
        mActiveStreamMutex.unlock();
        dattr = nullptr;
    }

    PAL_DBG(LOG_TAG, "streamDevDisconnect size=%d and streamDevConnect size=%d",
            streamDevDisconnect.size(), streamDevConnect.size());
    status = streamDevSwitch(streamDevDisconnect, streamDevConnect);
    if (status) {
        PAL_ERR(LOG_TAG, "streamDevSwitch failed %d", status);
    }
    mActiveStreamMutex.lock();
    for (sIter1 = streamDevDisconnect.begin(); sIter1 != streamDevDisconnect.end(); sIter1++) {
        if (std::get<0>(*sIter1) != NULL) {
            status = decreaseStreamUserCounter(std::get<0>(*sIter1));
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error decrementing the stream counter for the stream handle: %pK", std::get<0>(*sIter1));
                continue;
            }
        }
    }
    mActiveStreamMutex.unlock();
    for (auto dAttr : palDevices) free(dAttr);
}

#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
void ResourceManager::notifyClientSoundTriggerResourceAvailable() {
    if (onResourceAvailCb) {
        onResourceAvailCb(onResourceAvailCookie);
    }
}
#endif

#ifdef SEC_AUDIO_COMMON
void ResourceManager::dump(int fd)
{
    dprintf(fd, " \n");
    dprintf(fd, "ResourceManager : \n");
    if (ssrTimeinfo) {
        dprintf(fd, "\tssr occurred: %s", asctime(ssrTimeinfo));
    }
    sec_pal_sysfs_dump(fd);
#ifdef SEC_AUDIO_MUTE_DETECTION
    android::AudioInfo::GetInstance()->dumpSecSaveAudioLog(fd);
#endif
}
#endif

#ifdef SEC_AUDIO_MUTE_DETECTION
int ResourceManager::getKernelConfigInfo(char *mixer_str)
{
    struct mixer_ctl *ctl;
    int64_t *buf;
    unsigned int num_values;
    int status = 0;

    PAL_DBG(LOG_TAG, "mixer %pK, control %s", audio_hw_mixer, mixer_str);
    ctl = mixer_get_ctl_by_name(audio_hw_mixer, mixer_str);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s", mixer_str);
        status = -ENOENT;
        return status;
    }

    num_values = mixer_ctl_get_num_values(ctl);
    PAL_VERBOSE(LOG_TAG, "num_values: %d", num_values);
    buf = (int64_t *)calloc(sizeof(int64_t), num_values);
    if (!buf) {
        PAL_ERR(LOG_TAG, "Failed to allocate buf");
        status = -ENOMEM;
        goto exit;
    }

    status = mixer_ctl_get_array(ctl, buf, num_values);
    if (status < 0) {
        PAL_ERR(LOG_TAG, "Failed to mixer_ctl_get_array");
        goto exit;
    }

    if (!strcmp(mixer_str, TX_CONFIG_INFO) && (num_values >= 7)) {
        int clk = 0;
        int status = 0;
        int shift_clk = 1;
        int shift_status = 1;
        for (int i = 0; i < TX_CONFIG_MIC_SIZE; i++) {
            clk += (shift_clk * buf[i+1]); // 1,2,3
            status += (shift_status * buf[i+4]); // 4,5,6
            shift_clk *= TX_CONFIG_MIC_CLK_SIZE; // disable, enable
            shift_status *= TX_CONFIG_MIC_STATUS_SIZE; // off, normal, abnormal, n/a
        }
        PAL_INFO(LOG_TAG, "micbias=%d, dmic clk=%d(%d,%d,%d), dmic status=%d(%d,%d,%d)",
                buf[0], clk, buf[1], buf[2], buf[3], status, buf[4], buf[5], buf[6]);
        android::String8 codecStatus = android::String8::format("micbias=%d, dmic clk=%d(%d,%d,%d), dmic status=%d(%d,%d,%d)",
            buf[0], clk, buf[1], buf[2], buf[3], status, buf[4], buf[5], buf[6]);
        android::AudioInfo::GetInstance()->setSecSaveAudioLog(codecStatus);

    }

    if (!strcmp(mixer_str, RX_CONFIG_INFO) && (num_values >= 2)) {
        PAL_INFO(LOG_TAG, "rcv status=%d, spk status=%d", buf[0], buf[1]);
        android::String8 codecStatus = android::String8::format("rcv status=%d, spk status=%d", buf[0], buf[1]);
        android::AudioInfo::GetInstance()->setSecSaveAudioLog(codecStatus);
    }

exit:
    if (buf)
        free(buf);

    return status;
}

int ResourceManager::setKernelRegDump(char *mixer_str)
{
    struct mixer_ctl *ctl;
    int status = 0;

    PAL_DBG(LOG_TAG, "mixer %pK, control %s", audio_hw_mixer, mixer_str);
    ctl = mixer_get_ctl_by_name(audio_hw_mixer, mixer_str);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s", mixer_str);
        status = -ENOENT;
        return status;
    }

    PAL_DBG(LOG_TAG, "Setting mixer control: %s", mixer_str);
    status = mixer_ctl_set_value(ctl, 0, 1);
    if (status) {
        PAL_ERR(LOG_TAG, "mixer_ctl_set_value %s error %d", mixer_str, status);
        return status;
    }

    return status;
}
#endif
