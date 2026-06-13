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

#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H
#include <algorithm>
#include <vector>
#include <memory>
#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#ifdef FEATURE_IPQ_OPENWRT
#include "audio_route.h"
#else
#include "audio_route/audio_route.h"
#endif
#include <tinyalsa/asoundlib.h>
#include <array>
#include <map>
#include <set>
#include <expat.h>
#include <stdio.h>
#include <queue>
#include <deque>
#include <unordered_map>
#include <vui_dmgr_audio_intf.h>
#include <audio_feature_stats_intf.h>
#include <amdb_api.h>
#include "PalCommon.h"
#include "PalDefs.h"
#include "ChargerListener.h"
#include "SndCardMonitor.h"
#include "ContextManager.h"
#include "SoundTriggerPlatformInfo.h"
#include "SignalHandler.h"
#include "MemLogBuilder.h"

#ifdef SEC_AUDIO_COMMON
#include "SecPalDefs.h"
#endif
#ifdef SEC_AUDIO_MUTE_DETECTION
#define TX_CONFIG_INFO "TX_CONFIG_INFO"
#define RX_CONFIG_INFO "RX_CONFIG_INFO"
#define TX_REG_DUMP "TX_REG_DUMP"
#define RX_REG_DUMP "RX_REG_DUMP"

#define TX_CONFIG_MIC_SIZE (3)
#define TX_CONFIG_MIC_CLK_SIZE (2)
#define TX_CONFIG_MIC_STATUS_SIZE (4)

#define EVENT_ID_SILENCE_DETECTION 0x08001AC3
#endif

typedef enum {
    RX_HOSTLESS = 1,
    TX_HOSTLESS,
} hostless_dir_t;

#define audio_mixer mixer
#define MAX_SND_CARD 10
#define DUMMY_SND_CARD MAX_SND_CARD
#define VENDOR_CONFIG_PATH_MAX_LENGTH 128
#define VOLUME_TOLERANCE 0.000001
#define AUDIO_PARAMETER_KEY_NATIVE_AUDIO "audio.nat.codec.enabled"
#define AUDIO_PARAMETER_KEY_NATIVE_AUDIO_MODE "native_audio_mode"
#define AUDIO_PARAMETER_KEY_MAX_SESSIONS "max_sessions"
#define AUDIO_PARAMETER_KEY_MAX_NT_SESSIONS "max_nonTunnel_sessions"
#define AUDIO_PARAMETER_KEY_LOG_LEVEL "logging_level"
#define AUDIO_PARAMETER_KEY_CONTEXT_MANAGER_ENABLE "context_manager_enable"
#define AUDIO_PARAMETER_KEY_HIFI_FILTER "hifi_filter"
#define AUDIO_PARAMETER_KEY_LPI_LOGGING "lpi_logging_enable"
#define AUDIO_PARAMETER_KEY_UPD_DEDICATED_BE "upd_dedicated_be"
#define AUDIO_PARAMETER_KEY_DUAL_MONO "dual_mono"
#define AUDIO_PARAMETER_KEY_SIGNAL_HANDLER "signal_handler"
#define AUDIO_PARAMETER_KEY_DEVICE_MUX "device_mux_config"
#define AUDIO_PARAMETER_KEY_UPD_DUTY_CYCLE "upd_duty_cycle_enable"
#define AUDIO_PARAMETER_KEY_UPD_VIRTUAL_PORT "upd_virtual_port"
#define AUDIO_PARAMETER_KEY_HAPTICS_PRIORITY "haptics_priority"
#define AUDIO_PARAMETER_KEY_WSA_HAPTICS "haptics_through_wsa"
#define AUDIO_PARAMETER_KEY_DUMMY_DEV_ENABLE "dummy_dev_enable"
#define MAX_PCM_NAME_SIZE 50
#define MAX_STREAM_INSTANCES (sizeof(uint64_t) << 3)
#define MIN_USECASE_PRIORITY 0xFFFFFFFF
#if LINUX_ENABLED
#define QVA_VERSION "/data/vendor/audio/adc_qva_version.txt"
#if defined(__LP64__)
#define ADM_LIBRARY_PATH "/usr/lib64/libadm.so"
#define VUI_DMGR_LIB_PATH "/usr/lib64/libvui_dmgr_client.so"
#define AFS_LIB_PATH "/usr/lib64/libaudiofeaturestats.so"
#else
#define ADM_LIBRARY_PATH "/usr/lib/libadm.so"
#define VUI_DMGR_MANAGER_LIB_PATH "/usr/lib/libvui_dmgr_client.so"
#define AFS_LIB_PATH "/usr/lib/libaudiofeaturestats.so"
#endif
#else
#define QVA_VERSION "/data/vendor/audio/adc_qva_version.txt"
#ifdef __LP64__
#define ADM_LIBRARY_PATH "/vendor/lib64/libadm.so"
#define VUI_DMGR_LIB_PATH "/vendor/lib64/libvui_dmgr_client.so"
#define AFS_LIB_PATH "/vendor/lib64/libaudiofeaturestats.so"
#else
#define ADM_LIBRARY_PATH "/vendor/lib/libadm.so"
#define VUI_DMGR_LIB_PATH "/vendor/lib/libvui_dmgr_client.so"
#define AFS_LIB_PATH "/vendor/lib/libaudiofeaturestats.so"
#endif
#endif

#ifdef SOC_PERIPHERAL_PROT
#define SOC_PERIPHERAL_LIBRARY_PATH "/vendor/lib64/libPeripheralStateUtils.so"
#endif

#ifdef SEC_AUDIO_COMMON
#define LOG_DEBUG_LEVEL_PROP    "ro.vendor.boot.debug_level"
#define LOG_DEBUG_LEVEL_LOW     "0x4f4c"
#define PAL_LOG_LEVEL_LOW       (PAL_LOG_ERR|PAL_LOG_INFO)
#define PAL_LOG_LEVEL_DEFAULT   (PAL_LOG_ERR|PAL_LOG_INFO|PAL_LOG_DBG)
#endif

using InstanceListNode_t = std::vector<std::pair<int32_t, bool>> ;
using nonTunnelInstMap_t = std::unordered_map<uint32_t, bool>;

#ifdef SOC_PERIPHERAL_PROT
extern "C" {
#include "CPeripheralAccessControl.h"
#include "peripheralStateUtils.h"
}
#endif

typedef enum {
    TAG_ROOT,
    TAG_CARD,
    TAG_DEVICE,
    TAG_PLUGIN,
    TAG_DEV_PROPS,
    TAG_NONE,
    TAG_MIXER,
} snd_card_defs_xml_tags_t;

typedef enum {
    TAG_RESOURCE_ROOT,
    TAG_RESOURCE_MANAGER_INFO,
    TAG_DEVICE_PROFILE,
    TAG_IN_DEVICE,
    TAG_OUT_DEVICE,
    TAG_USECASE,
    TAG_CONFIG_VOICE,
    TAG_CONFIG_MODE_MAP,
    TAG_CONFIG_MODE_PAIR,
    TAG_GAIN_LEVEL_MAP,
    TAG_GAIN_LEVEL_PAIR,
    TAG_INSTREAMS,
    TAG_INSTREAM,
    TAG_POLICIES,
    TAG_ECREF,
    TAG_VI_CHMAP,
    TAG_CUSTOMCONFIG,
    TAG_LPI_VOTE_STREAM,
    TAG_SLEEP_MONITOR_LPI_STREAM,
    TAG_CONFIG_VOLUME,
    TAG_CONFIG_VOLUME_SET_PARAM_SUPPORTED_STREAM,
    TAG_CONFIG_VOLUME_SET_PARAM_SUPPORTED_STREAMS,
    TAG_CONFIG_LPM,
    TAG_CONFIG_LPM_SUPPORTED_STREAM,
    TAG_CONFIG_LPM_SUPPORTED_STREAMS,
    TAG_STREAMS_AVOID_SLEEP_MONITOR_VOTE,
    TAG_AVOID_VOTE_STREAM,
    TAG_STANDBY_STREAM_TYPE,
    TAG_STANDBY_SUPPORT_STREAMS,
} resource_xml_tags_t;

typedef enum {
    PCM,
    COMPRESS,
    VOICE1,
    VOICE2,
    ExtEC,
} stream_supported_type;

typedef enum {
    ST_PAUSE = 1,
    ST_RESUME,
    ST_ENABLE_LPI,
    ST_HANDLE_CONCURRENT_STREAM,
    ST_HANDLE_CONNECT_DEVICE,
    ST_HANDLE_DISCONNECT_DEVICE,
    ST_HANDLE_CHARGING_STATE,
    ST_INTERNAL_PAUSE,
    ST_INTERNAL_RESUME,
} st_action;

typedef enum
{
    GRP_DEV_CONFIG_INVALID = -1,
    GRP_UPD_RX,
    GRP_HANDSET,
    GRP_SPEAKER,
    GRP_SPEAKER_VOICE,
    GRP_UPD_RX_HANDSET,
    GRP_UPD_RX_SPEAKER,
    GRP_DEV_CONFIG_IDX_MAX,
} group_dev_config_idx_t;

struct xml_userdata {
    char data_buf[1024];
    size_t offs;

    unsigned int card;
    bool card_found;
    bool card_parsed;
    bool resourcexml_parsed;
    bool voice_info_parsed;
    bool gain_lvl_parsed;
    snd_card_defs_xml_tags_t current_tag;
    bool is_parsing_sound_trigger;
    bool is_parsing_group_device;
    group_dev_config_idx_t group_dev_idx;
    resource_xml_tags_t tag;
    bool inCustomConfig;
    XML_Parser parser;
};

typedef enum {
    DEFAULT = 0,
    HOSTLESS,
    NON_TUNNEL,
    NO_CONFIG,
} sess_mode_t;

struct deviceCap {
    int deviceId;
    char name[MAX_PCM_NAME_SIZE];
    stream_supported_type type;
    int playback;
    int record;
    sess_mode_t sess_mode;
};

typedef enum {
    SIDETONE_OFF,
    SIDETONE_HW,
    SIDETONE_SW,
} sidetone_mode_t;

typedef enum {
    AUDIO_BIT_WIDTH_8 = 8,
    AUDIO_BIT_WIDTH_DEFAULT_16 = 16,
    AUDIO_BIT_WIDTH_24 = 24,
    AUDIO_BIT_WIDTH_32 = 32,
} audio_bit_width_t;

typedef enum {
    CHARGER_ON_PB_STARTS,
    PB_ON_CHARGER_INSERT,
    PB_ON_CHARGER_REMOVE,
    CONCURRENCY_PB_STOPS
} charger_boost_mode_t;

typedef enum {
    NO_DEFER,
    DEFER_LPI_NLPI_SWITCH,
    DEFER_NLPI_LPI_SWITCH,
} defer_switch_state_t;

struct usecase_custom_config_info
{
    std::string key;
    std::string sndDevName;
    int channel;
    sidetone_mode_t sidetoneMode;
    int samplerate;
    uint32_t priority;
    uint32_t bit_width;
    bool ec_enable;
};

struct usecase_info {
    int type;
    int samplerate;
    sidetone_mode_t sidetoneMode;
    std::string sndDevName;
    int channel;
    std::vector<usecase_custom_config_info> config;
    uint32_t priority;
    uint32_t bit_width;
    bool ec_enable;
};

struct pal_device_info {
     int channels;
     int max_channels;
     int samplerate;
     std::string sndDevName;
     bool isExternalECRefEnabledFlag;
     bool isUSBUUIdBasedTuningEnabledFlag;
     uint32_t priority;
     bool fractionalSRSupported;
     bool channels_overwrite;
     bool samplerate_overwrite;
     bool sndDevName_overwrite;
     bool bit_width_overwrite;
     uint32_t bit_width;
     pal_audio_fmt_t bitFormatSupported;
};

struct vsid_modepair {
    unsigned int key;
    unsigned int value;
};

struct vsid_info {
     int vsid;
     std::vector<vsid_modepair> modepair;
     int loopback_delay;
#ifdef SEC_AUDIO_LOOPBACK_TEST
     int loopback_mode;
     int loopback_mode_delay[PAL_LOOPBACK_MODE_MAX];
#endif
};

struct volume_set_param_info {
    int isVolumeUsingSetParam;
    std::vector<uint32_t> streams_;
};

struct disable_lpm_info {
    int isDisableLpm;
    std::vector<uint32_t> streams_;
};

struct tx_ecinfo {
    int tx_stream_type;
    std::vector<int> disabled_rx_streams;
};

enum {
    NATIVE_AUDIO_MODE_SRC = 1,
    NATIVE_AUDIO_MODE_TRUE_44_1,
    NATIVE_AUDIO_MODE_MULTIPLE_MIX_IN_CODEC,
    NATIVE_AUDIO_MODE_MULTIPLE_MIX_IN_DSP,
    NATIVE_AUDIO_MODE_INVALID
};

enum {
    HAPTICS_MODE_INVALID,
    HAPTICS_MODE_TOUCH,
    HAPTICS_MODE_RINGTONE,
};

struct nativeAudioProp {
   bool rm_na_prop_enabled;
   bool ui_na_prop_enabled;
   int na_mode;
};

typedef struct devpp_mfc_config
{
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t bit_width;
} devpp_mfc_config_t;

typedef struct group_dev_hwep_config_ctl
{
    uint32_t sample_rate;
    uint32_t channels;
    pal_audio_fmt_t aud_fmt_id;
    uint32_t slot_mask;
} group_dev_hwep_config_t;

typedef struct group_dev_config
{
    std::string snd_dev_name;
    devpp_mfc_config_t  devpp_mfc_cfg;
    group_dev_hwep_config_t grp_dev_hwep_cfg;
} group_dev_config_t;

/* AFS parameter data */
typedef struct afs_param_payload_h {
    char qva_version[50] = {0};
    uint32_t is_present;
    uint32_t error_code;
    uint32_t module_version_major;
    uint32_t module_version_minor;
    amdb_module_build_ts_info_t build_ts;
} __attribute__ ((packed)) afs_param_payload_t;

static const constexpr uint32_t DEFAULT_NT_SESSION_TYPE_COUNT = 2;

enum NTStreamTypes_t : uint32_t {
    NT_PATH_ENCODE = 0,
    NT_PATH_DECODE
};

typedef void (*SoundTriggerOnResourceAvailableCallback)(uint64_t cookie);

typedef void (*session_callback)(uint64_t hdl, uint32_t event_id, void *event_data,
                uint32_t event_size);
bool isPalPCMFormat(uint32_t fmt_id);

typedef void* (*adm_init_t)();
typedef void (*adm_deinit_t)(void *);
typedef void (*adm_register_output_stream_t)(void *, void*);
typedef void (*adm_register_input_stream_t)(void *, void*);
typedef void (*adm_deregister_stream_t)(void *, void*);
typedef void (*adm_request_focus_t)(void *, void*);
typedef void (*adm_abandon_focus_t)(void *, void*);
typedef void (*adm_set_config_t)(void *, void*,
        struct pcm *, struct pcm_config *);
typedef void (*adm_request_focus_v2_t)(void *, void*, long);
typedef void (*adm_on_routing_change_t)(void *, void*);
typedef int (*adm_request_focus_v2_1_t)(void *, void*, long);


#ifdef SOC_PERIPHERAL_PROT
typedef int32_t (*getPeripheralStatusFnPtr)(void *context);
typedef void* (*registerPeripheralCBFnPtr)(uint32_t peripheral, PeripheralStateCB NotifyEvent);
typedef int32_t (*deregisterPeripheralCBFnPtr)(void *context);
#endif

class Device;
class Stream;
class StreamPCM;
class StreamCompress;
class StreamSoundTrigger;
class StreamACD;
class StreamASR;
class StreamInCall;
class StreamNonTunnel;
class SoundTriggerEngine;
class SndCardMonitor;
class StreamUltraSound;
class ContextManager;
class StreamSensorPCMData;
class StreamContextProxy;
class StreamCommonProxy;
class StreamHaptics;
class StreamSensorRenderer;

struct deviceIn {
    int deviceId;
    int max_channel;
    int channel;
    int samplerate;
    std::vector<usecase_info> usecase;
    // dev ids supporting ec ref
    std::vector<pal_device_id_t> rx_dev_ids;
    /*
     * map dynamically maintain ec ref count, key for this map
     * is rx device id, which is present in rx_dev_ids, and value
     * for this map is a vector of all active tx streams using
     * this rx device as ec ref. For each Tx stream, we have a
     * EC ref count, indicating number of Rx streams which uses
     * this rx device as output device and also not disabled stream
     * type to the Tx stream. E.g., for SVA and Recording stream,
     * LL playback with speaker may only count for Recording stream
     * when ll barge-in is not enabled.
     */
    std::map<int, std::vector<std::pair<Stream *, int>>> ec_ref_count_map;
    std::string sndDevName;
    bool isExternalECRefEnabled;
    bool isUSBUUIdBasedTuningEnabled;
    bool fractionalSRSupported;
    uint32_t bit_width;
    pal_audio_fmt_t bitFormatSupported;
    bool ec_enable;
};

class ResourceManager
{
private:
    //both of the below are update on register and deregister stream
    int mPriorityHighestPriorityActiveStream; //priority of the highest priority active stream
    Stream* mHighestPriorityActiveStream; //pointer to the highest priority active stream
    int getNumFEs(const pal_stream_type_t sType) const;
    bool ifVoiceorVoipCall (pal_stream_type_t streamType) const;
    int getCallPriority(bool ifVoiceCall) const;
    int getStreamAttrPriority (const pal_stream_attributes* sAttr) const;
    template <class T>

    void getHigherPriorityActiveStreams(const int inComingStreamPriority,
                                        std::vector<Stream*> &activestreams,
                                        std::vector<T> sourcestreams);
    const std::vector<int> allocateVoiceFrontEndIds(std::vector<int> listAllPcmVoiceFrontEnds,
                                  const int howMany);
    int getDeviceDefaultCapability(pal_param_device_capability_t capability);

    int handleScreenStatusChange(pal_param_screen_state_t screen_state);
    int handleDeviceRotationChange(pal_param_device_rotation_t rotation_type);
    int handleDeviceConnectionChange(pal_param_device_connection_t connection_state);
    int SetOrientationCal(pal_param_device_rotation_t rotation_type);
    int32_t streamDevDisconnect(std::vector <std::tuple<Stream *, uint32_t>> streamDevDisconnectList);
    int32_t streamDevConnect(std::vector <std::tuple<Stream *, struct pal_device *>> streamDevConnectList);
    int32_t streamDevDisconnect_l(std::vector <std::tuple<Stream *, uint32_t>> streamDevDisconnectList);
    int32_t streamDevConnect_l(std::vector <std::tuple<Stream *, struct pal_device *>> streamDevConnectList);
    void ssrHandlingLoop(std::shared_ptr<ResourceManager> rm);
    int updateECDeviceMap(std::shared_ptr<Device> rx_dev,
                        std::shared_ptr<Device> tx_dev,
                        Stream *tx_str, int count, bool is_txstop);
    std::shared_ptr<Device> clearInternalECRefCounts(Stream *tx_str,
                        std::shared_ptr<Device> tx_dev);
    static bool isBitWidthSupported(uint32_t bitWidth);
    uint32_t getNTPathForStreamAttr(const pal_stream_attributes &attr);
    ssize_t getAvailableNTStreamInstance(const pal_stream_attributes &attr);
    int getECEnableSetting(std::shared_ptr<Device> tx_dev, Stream * streamHandle, bool *ec_enable);
    int checkandEnableECForTXStream_l(std::shared_ptr<Device> tx_dev, Stream *tx_stream, bool ec_enable);
    int checkandEnableECForRXStream_l(std::shared_ptr<Device> rx_dev, Stream *rx_stream, bool ec_enable);
    int checkandEnableEC_l(std::shared_ptr<Device> d, Stream *s, bool enable);
    void onChargingStateChange();
    void onVUIStreamRegistered();
    void onVUIStreamDeregistered();
    bool checkDeviceSwitchForHaptics(struct pal_device *inDevAttr, struct pal_device *curDevAttr);
    SoundTriggerOnResourceAvailableCallback onResourceAvailCb = NULL;
    uint64_t onResourceAvailCookie;
protected:
    std::list <Stream*> mActiveStreams;
    std::list <StreamPCM*> active_streams_ll;
    std::list <StreamPCM*> active_streams_ulla;
    std::list <StreamPCM*> active_streams_ull;
    std::list <StreamPCM*> active_streams_db;
    std::list <StreamPCM*> active_streams_sa;
    std::list <StreamPCM*> active_streams_po;
    std::list <StreamPCM*> active_streams_proxy;
    std::list <StreamPCM*> active_streams_haptics;
    std::list <StreamPCM*> active_streams_raw;
    std::list <StreamPCM*> active_streams_voice_rec;
    std::list <StreamInCall*> active_streams_incall_record;
    std::list <StreamNonTunnel*> active_streams_non_tunnel;
    std::list <StreamInCall*> active_streams_incall_music;
    std::list <StreamCompress*> active_streams_comp;
    std::list <StreamSoundTrigger*> active_streams_st;
    std::list <StreamACD*> active_streams_acd;
    std::list <StreamASR*> active_streams_asr;
    std::list <StreamUltraSound*> active_streams_ultrasound;
    std::list <StreamSensorPCMData*> active_streams_sensor_pcm_data;
    std::list <StreamContextProxy*> active_streams_context_proxy;
    std::list <StreamCommonProxy*> active_streams_afs;
    std::list <StreamSensorRenderer*> active_streams_sensor_renderer;
    std::vector <std::pair<std::shared_ptr<Device>, Stream*>> active_devices;
    std::vector <std::shared_ptr<Device>> plugin_devices_;
    std::vector <pal_device_id_t> avail_devices_;
    std::map<Stream*, std::pair<uint32_t, bool>> mActiveStreamUserCounter;
    bool bOverwriteFlag;
    bool screen_state_ = true;
    bool charging_state_;
    bool is_charger_online_;
    bool is_concurrent_boost_state_;
    bool use_lpi_;
    bool current_concurrent_state_;
    bool is_ICL_config_;
    bool force_nlpi_ = false;
    pal_speaker_rotation_type rotation_type_;
    bool isDeviceSwitch = false;
    static std::mutex mResourceManagerMutex;
    static std::mutex mGraphMutex;
    static std::mutex mActiveStreamMutex;
    static std::mutex mSleepMonitorMutex;
    static std::mutex mListFrontEndsMutex;
    static int snd_virt_card;
    static int snd_hw_card;

    static std::shared_ptr<ResourceManager> rm;
    static struct audio_route* audio_route;
    static struct audio_mixer* audio_virt_mixer;
    static struct audio_mixer* audio_hw_mixer;
    static std::vector <int> streamTag;
    static std::vector <int> streamPpTag;
    static std::vector <int> mixerTag;
    static std::vector <int> devicePpTag;
    static std::vector <int> deviceTag;
    static std::vector<std::pair<int32_t, int32_t>> devicePcmId;
    static std::vector<std::pair<int32_t, std::string>> deviceLinkName;
    static std::vector<int> listAllFrontEndIds;
    static std::vector<int> listAllPcmPlaybackFrontEnds;
    static std::vector<int> listAllPcmRecordFrontEnds;
    static std::vector<int> listAllPcmHostlessRxFrontEnds;
    static std::vector<int> listAllNonTunnelSessionIds;
    static std::vector<int> listAllPcmHostlessTxFrontEnds;
    static std::vector<int> listAllCompressPlaybackFrontEnds;
    static std::vector<int> listAllCompressRecordFrontEnds;
    static std::vector<int> listFreeFrontEndIds;
    static std::vector<int> listAllPcmVoice1RxFrontEnds;
    static std::vector<int> listAllPcmVoice1TxFrontEnds;
    static std::vector<int> listAllPcmVoice2RxFrontEnds;
    static std::vector<int> listAllPcmVoice2TxFrontEnds;
    static std::vector<int> listAllPcmExtEcTxFrontEnds;
    static std::vector<int> listAllPcmInCallRecordFrontEnds;
    static std::vector<int> listAllPcmInCallMusicFrontEnds;
    static std::vector<int> listAllPcmContextProxyFrontEnds;
    static std::vector<std::pair<int32_t, std::string>> listAllBackEndIds;
    static std::vector<std::pair<int32_t, std::string>> sndDeviceNameLUT;
    static std::vector<deviceCap> devInfo;
    static std::map<std::pair<uint32_t, std::string>, std::string> btCodecMap;
    static std::map<std::string, uint32_t> btFmtTable;
    static std::map<std::string, int> spkrPosTable;
    static std::map<int, std::string> spkrTempCtrlsMap;
    static std::map<uint32_t, uint32_t> btSlimClockSrcMap;
    static std::vector<deviceIn> deviceInfo;
    static std::vector<tx_ecinfo> txEcInfo;
    static struct vsid_info vsidInfo;
    static struct volume_set_param_info volumeSetParamInfo_;
    static struct disable_lpm_info disableLpmInfo_;
    static std::vector<struct pal_amp_db_and_gain_table> gainLvlMap;
    static SndCardMonitor *sndmon;
    static std::vector <vote_type_t> sleep_monitor_vote_type_;
    /* condition variable for which ssrHandlerLoop will wait */
    static std::condition_variable cv;
    static std::mutex cvMutex;
    static std::queue<card_status_t> msgQ;
    static std::thread workerThread;
    std::vector<std::pair<std::string, InstanceListNode_t>> STInstancesLists;
    uint64_t stream_instances[PAL_STREAM_MAX];
    uint64_t in_stream_instances[PAL_STREAM_MAX];
    static int mixerEventRegisterCount;
    static int TxconcurrencyEnableCount;
    static int concurrencyEnableCount;
    static int concurrencyDisableCount;
    static int ACDConcurrencyEnableCount;
    static int ACDConcurrencyDisableCount;
    static int ASRConcurrencyEnableCount;
    static int ASRConcurrencyDisableCount;
    static int SNSPCMDataConcurrencyEnableCount;
    static int SNSPCMDataConcurrencyDisableCount;
    static defer_switch_state_t deferredSwitchState;
    static int wake_lock_fd;
    static int wake_unlock_fd;
    static uint32_t wake_lock_cnt;
    static bool lpi_logging_;
    std::map<int, std::pair<session_callback, uint64_t>> mixerEventCallbackMap;
    static std::thread mixerEventTread;
    /*
     * Thread to handle deferred switch, only applicable
     * when low latency bargein is enabled.
     */
    static std::thread vui_deferred_switch_thread_;
    static std::condition_variable vui_switch_cv_;
    static std::mutex vui_switch_mutex_;
    static bool vui_switch_thread_exit_;
    static int deferred_switch_cnt_;
    static void voiceUIDeferredSwitchLoop(std::shared_ptr<ResourceManager> rm);
    std::shared_ptr<CaptureProfile> SoundTriggerCaptureProfile;
    std::shared_ptr<CaptureProfile> TXMacroCaptureProfile;
    ResourceManager();
    ContextManager *ctxMgr;
#ifdef ADSP_SLEEP_MONITOR
    int32_t lpi_counter_;
    int32_t nlpi_counter_;
    int sleepmon_fd_;
#endif
    static std::map<group_dev_config_idx_t, std::shared_ptr<group_dev_config_t>> groupDevConfigMap;
    std::array<std::shared_ptr<nonTunnelInstMap_t>, DEFAULT_NT_SESSION_TYPE_COUNT> mNTStreamInstancesList;
    int32_t scoOutConnectCount = 0;
    int32_t scoInConnectCount = 0;
    std::shared_ptr<SignalHandler> mSigHandler;
    static std::vector<int> spViChannelMapCfg;
    std::map<int, bool> PCMDataInstances;
    std::unordered_map<int, pal_stream_handle_t *> mStCaptureInfo;

public:
    ~ResourceManager();
    static bool mixerClosed;
    enum card_status_t cardState;
    bool ssrStarted = false;
    static bool isQmpEnabled;
    /* Variable to store whether Speaker protection is enabled or not */
    static bool isSpeakerProtectionEnabled;
    static bool isHandsetProtectionEnabled;
    static bool isHapticsProtectionEnabled;
    static bool isChargeConcurrencyEnabled;
    static int cpsMode;
    static bool isVbatEnabled;
    static bool isRasEnabled;
    static bool isGaplessEnabled;
    static bool isContextManagerEnabled;
    static bool isDualMonoEnabled;
    static bool isDeviceMuxConfigEnabled;
#ifdef SEC_AUDIO_SUPPORT_UHQ
    static pal_uhqa_state stateUHQA;
#else
    static bool isUHQAEnabled;
#endif
    static bool isSignalHandlerEnabled;
    static bool isCPEnabled;
    static bool isCRSCallEnabled;
    static bool isDummyDevEnabled;
    static bool isProxyRecordActive;
    static std::mutex mChargerBoostMutex;
    /* Variable to store which speaker side is being used for call audio.
     * Valid for Stereo case only
     */
    static pal_audio_event_callback callback_event;
    static bool isMainSpeakerRight;
    /* Variable to store Quick calibration time for Speaker protection */
    static int spQuickCalTime;
    /* Variable to store the mode request for Speaker protection */
    pal_spkr_prot_payload mSpkrProtModeValue;

    /* Variable to store the mode request for Haptics Device */
    pal_haptics_payload mHapticsModeValue;
    /* Variable to store the device orientation for Speaker*/
    int mOrientation = 0;
    pal_global_callback globalCb = NULL;
    uint32_t num_proxy_channels = 0;
    /* Flag to store the state of VI record */
    static bool isVIRecordStarted;
    /* Flag to indicate if shared backend is enabled for UPD */
    static bool isUpdDedicatedBeEnabled;
    /* Flag to indicate if shared backend is enabled for UPD */
    static bool isUpdDutyCycleEnabled;
    /* Flag to indicate if virtual port is enabled for UPD */
    static bool isUPDVirtualPortEnabled;
    /* Flag to indicate if Haptics isdriven thorugh WSA */
    static bool isHapticsthroughWSA;
    /* Variable to store max volume index for voice call */
    static int max_voice_vol;
    /*Silence Detection Enable flag for PCM session*/
    static bool isSilenceDetectionEnabledPcm;
    /*Silence Detection Enable flag for Voice session*/
    static bool isSilenceDetectionEnabledVoice;
    /*Silence Detection Duration Configuration*/
    static uint32_t silenceDetectionDuration;
    /*variable to store MSPP linear gain*/
    pal_param_mspp_linear_gain_t linear_gain;
#ifdef SOC_PERIPHERAL_PROT
    static std::thread socPerithread;
    static bool isTZSecureZone;
    static void *tz_handle;
    static int deregPeripheralCb(void *cntxt);
    static int registertoPeripheral(uint32_t pUID);
    static int32_t secureZoneEventCb(const uint32_t peripheral,
                                           const uint8_t secureState);
    static void loadSocPeripheralLib();
    static void *socPeripheralLibHdl;
    static getPeripheralStatusFnPtr mGetPeripheralState;
    static registerPeripheralCBFnPtr mRegisterPeripheralCb;
    static deregisterPeripheralCBFnPtr mDeregisterPeripheralCb;
#endif
    uint64_t cookie;
    int initSndMonitor();
    int initContextManager();
    void deInitContextManager();
    adm_init_t admInitFn = NULL;
    adm_deinit_t admDeInitFn = NULL;
    adm_register_output_stream_t admRegisterOutputStreamFn = NULL;
    adm_register_input_stream_t admRegisterInputStreamFn = NULL;
    adm_deregister_stream_t admDeregisterStreamFn = NULL;
    adm_request_focus_t admRequestFocusFn = NULL;
    adm_abandon_focus_t admAbandonFocusFn = NULL;
    adm_set_config_t admSetConfigFn = NULL;
    adm_request_focus_v2_t admRequestFocusV2Fn = NULL;
    adm_on_routing_change_t admOnRoutingChangeFn = NULL;
    adm_request_focus_v2_1_t  admRequestFocus_v2_1Fn = NULL;
    void *admData = NULL;
    void *admLibHdl = NULL;
    static void *cl_lib_handle;
    static cl_init_t cl_init;
    static cl_deinit_t cl_deinit;
    static cl_set_boost_state_t cl_set_boost_state;
    static std::shared_ptr<group_dev_config_t> activeGroupDevConfig;
    static group_dev_config_t currentGroupDevConfig;

    static void *vui_dmgr_lib_handle;
    static vui_dmgr_init_t vui_dmgr_init;
    static vui_dmgr_deinit_t vui_dmgr_deinit;
    static void voiceuiDmgrManagerInit();
    static void voiceuiDmgrManagerDeInit();
    static int32_t voiceuiDmgrPalCallback(int32_t param_id, void *payload, size_t payload_size);
    int32_t voiceuiDmgrRestartUseCases(vui_dmgr_param_restart_usecases_t *uc_info);

    pal_stream_handle_t *afs_stream_handle = NULL;
    static void *feature_stats_handle;
    static afs_init_t feature_stats_init;
    static afs_deinit_t feature_stats_deinit;
    static void AudioFeatureStatsInit();
    static void AudioFeatureStatsDeInit();
    static int AudioFeatureStatsGetInfo(void **afs_payload, size_t *afs_payload_size);
    void checkQVAAppPresence(afs_param_payload_t *payload);
    pal_param_payload *AFSWakeUpAlgoDetection();

    /* checks config for both stream and device */
    bool isStreamSupported(struct pal_stream_attributes *attributes,
                           struct pal_device *devices, int no_of_devices);
    int32_t getDeviceConfig(struct pal_device *deviceattr,
                            struct pal_stream_attributes *attributes);
    /*getDeviceInfo - updates channels, fluence info of the device*/
    void getDeviceInfo(pal_device_id_t deviceId, pal_stream_type_t type,
                       std::string key, struct pal_device_info *devinfo);
    bool getEcRefStatus(pal_stream_type_t tx_streamtype,pal_stream_type_t rx_streamtype);
    int32_t getVsidInfo(struct vsid_info  *info);
    int32_t getVolumeSetParamInfo(struct volume_set_param_info *volinfo);
    int32_t getDisableLpmInfo(struct disable_lpm_info *lpminfo);
    int getMaxVoiceVol();
    void getChannelMap(uint8_t *channel_map, int channels);
    pal_audio_fmt_t getAudioFmt(uint32_t bitWidth);
    int registerStream(Stream *s);
    int deregisterStream(Stream *s);
    int isActiveStream(pal_stream_handle_t *handle);
    int initStreamUserCounter(Stream *s);
    int deactivateStreamUserCounter(Stream *s);
    int eraseStreamUserCounter(Stream *s);
    int increaseStreamUserCounter(Stream* s);
    int decreaseStreamUserCounter(Stream* s);
    int getStreamUserCounter(Stream *s);
    int printStreamUserCounter(Stream *s);
    int registerDevice(std::shared_ptr<Device> d, Stream *s);
    int deregisterDevice(std::shared_ptr<Device> d, Stream *s);
    int registerDevice_l(std::shared_ptr<Device> d, Stream *s);
    int deregisterDevice_l(std::shared_ptr<Device> d, Stream *s);
    int registerMixerEventCallback(const std::vector<int> &DevIds,
                                   session_callback callback,
                                   uint64_t cookie, bool is_register);
    int updateECDeviceMap_l(std::shared_ptr<Device> rx_dev,
                            std::shared_ptr<Device> tx_dev,
                            Stream *tx_str, int count, bool is_txstop);
    bool isDeviceActive(pal_device_id_t deviceId);
    bool isDeviceActive(std::shared_ptr<Device> d, Stream *s);
    bool isDeviceActive_l(std::shared_ptr<Device> d, Stream *s);
    int addPlugInDevice(std::shared_ptr<Device> d,
                        pal_param_device_connection_t connection_state);
    int removePlugInDevice(pal_device_id_t device_id,
                           pal_param_device_connection_t connection_state);
    /* bIsUpdated - to specify if the config is updated by rm */
    int checkAndGetDeviceConfig(struct pal_device *device ,bool* bIsUpdated);
    static void getFileNameExtn(const char* in_snd_card_name, char* file_name_extn,
                                char* file_name_extn_wo_variant);
    int init_audio();
    void loadAdmLib();
    static int init();
    static void deinit();
    static std::shared_ptr<ResourceManager> getInstance();
    static int XmlParser(std::string xmlFile);
    static void updatePcmId(int32_t deviceId, int32_t pcmId);
    static void updateLinkName(int32_t deviceId, std::string linkName);
    static void updateSndName(int32_t deviceId, std::string sndName);
    static void updateBackEndName(int32_t deviceId, std::string backEndName);
    static void updateBtCodecMap(std::pair<uint32_t, std::string> key, std::string value);
    static std::string getBtCodecLib(uint32_t codecFormat, std::string codecType);
    static void updateSpkrTempCtrls(int key, std::string value);
    static std::string getSpkrTempCtrl(int channel);
    static void updateBtSlimClockSrcMap(uint32_t key, uint32_t value);
    static uint32_t getBtSlimClockSrc(uint32_t codecFormat);
    int getGainLevelMapping(struct pal_amp_db_and_gain_table *mapTbl, int tblSize);

    int setParameter(uint32_t param_id, void *param_payload,
                     size_t payload_size);
    int setParameter(uint32_t param_id, void *param_payload,
                     size_t payload_size, pal_device_id_t pal_device_id,
                     pal_stream_type_t pal_stream_type);
    int setSessionParamConfig(uint32_t param_id, Stream *stream, int tag);
    int handleChargerEvent(Stream *stream, int tag);
    int rwParameterACDB(uint32_t param_id, void *param_payload,
                     size_t payload_size, pal_device_id_t pal_device_id,
                     pal_stream_type_t pal_stream_type, uint32_t sample_rate,
                     uint32_t instance_id, bool is_param_write, bool is_play);
    int getParameter(uint32_t param_id, void **param_payload,
                     size_t *payload_size, void *query = nullptr);
    int getParameter(uint32_t param_id, void *param_payload,
                     size_t payload_size, pal_device_id_t pal_device_id,
                     pal_stream_type_t pal_stream_type);
    int getVirtualSndCard();
    int getHwSndCard();
    int getPcmDeviceId(int deviceId);
    int getAudioRoute(struct audio_route** ar);
    int getVirtualAudioMixer(struct audio_mixer **am);
    int getHwAudioMixer(struct audio_mixer **am);
    int getActiveStream(std::vector<Stream*> &activestreams, std::shared_ptr<Device> d = nullptr);
    int getActiveStream_l(std::vector<Stream*> &activestreams,std::shared_ptr<Device> d = nullptr);
    int getOrphanStream(std::vector<Stream*> &orphanstreams, std::vector<Stream*> &retrystreams);
    int getOrphanStream_l(std::vector<Stream*> &orphanstreams, std::vector<Stream*> &retrystreams);
    int getActiveDevices(std::vector<std::shared_ptr<Device>> &deviceList);
    int getSndDeviceName(int deviceId, char *device_name);
    int getDeviceEpName(int deviceId, std::string &epName);
    int getBackendName(int deviceId, std::string &backendName);
    void updateVirtualBackendName();
    void updateVirtualBESndName();
    int getStreamTag(std::vector <int> &tag);
    int getDeviceTag(std::vector <int> &tag);
    int getMixerTag(std::vector <int> &tag);
    int getStreamPpTag(std::vector <int> &tag);
    int getDevicePpTag(std::vector <int> &tag);
    int getDeviceDirection(uint32_t beDevId);
    void getSpViChannelMapCfg(int32_t *channelMap, uint32_t numOfChannels);
    const std::vector<int> allocateFrontEndIds (const struct pal_stream_attributes &,
                                                int lDirection);
    const std::vector<int> allocateFrontEndExtEcIds ();
    void freeFrontEndEcTxIds (const std::vector<int> f);
    void freeFrontEndIds (const std::vector<int> f,
                          const struct pal_stream_attributes &,
                          int lDirection);
    const std::vector<std::string> getBackEndNames(const std::vector<std::shared_ptr<Device>> &deviceList) const;
    void getSharedBEDevices(std::vector<std::shared_ptr<Device>> &deviceList, std::shared_ptr<Device> inDevice) const;
    static std::vector <std::string> usb_vendor_uuid_list;
    void getBackEndNames( const std::vector<std::shared_ptr<Device>> &deviceList,
                          std::vector<std::pair<int32_t, std::string>> &rxBackEndNames,
                          std::vector<std::pair<int32_t, std::string>> &txBackEndNames) const;
    void handleA2dpBleConcurrency(std::shared_ptr<Device> *inDev,
            struct pal_device *inDevAttr, struct pal_device &dummyDevAttr,
            std::vector <std::tuple<Stream *, uint32_t>> &streamDevDisconnect,
            std::vector <std::tuple<Stream *, struct pal_device *>> &streamDevConnect);
    bool updateDeviceConfig(std::shared_ptr<Device> *inDev,
             struct pal_device *inDevAttr, const pal_stream_attributes* inStrAttr);
    int findActiveStreamsNotInDisconnectList(
            std::vector <std::tuple<Stream *, uint32_t>> &streamDevDisconnectList,
            std::vector <std::tuple<Stream *, uint32_t>> &streamsSkippingSwitch);
    int restoreDeviceConfigForUPD(
            std::vector <std::tuple<Stream *, uint32_t>> &streamDevDisconnect,
            std::vector <std::tuple<Stream *, struct pal_device *>> &StreamDevConnect,
            std::vector <std::tuple<Stream *, uint32_t>> &streamsSkippingSwitch);
    int32_t forceDeviceSwitch(std::shared_ptr<Device> inDev, struct pal_device *newDevAttr);
    int32_t forceDeviceSwitch(std::shared_ptr<Device> inDev, struct pal_device *newDevAttr,
                              std::vector <Stream *> prevActiveStreams);
    const std::string getPALDeviceName(const pal_device_id_t id) const;
    bool isNonALSACodec(const struct pal_device *device) const;
    bool isNLPISwitchSupported();
    bool IsLPISupported();
    bool IsLowLatencyBargeinSupported();
    bool IsAudioCaptureConcurrencySupported();
    bool IsVoiceCallConcurrencySupported();
    bool IsVoipConcurrencySupported();
    bool IsTransitToNonLPIOnChargingSupported();
    bool IsDedicatedBEForUPDEnabled();
    bool IsDutyCycleForUPDEnabled();
    bool IsVirtualPortForUPDEnabled();
    uint32_t getHapticsPriority();
    bool IsHapticsThroughWSA();
    void GetSoundTriggerConcurrencyCount(pal_stream_type_t type, int32_t *enable_count, int32_t *disable_count);
    void GetSoundTriggerConcurrencyCount_l(pal_stream_type_t type, int32_t *enable_count, int32_t *disable_count);
    bool GetChargingState() const { return charging_state_; }
    bool getChargerOnlineState(void) const { return is_charger_online_; }
    bool getConcurrentBoostState(void) const { return is_concurrent_boost_state_; }
    bool getLPIUsage() const { return use_lpi_ && !force_nlpi_; }
    bool getInputCurrentLimitorConfigStatus(void) const { return is_ICL_config_; }
    bool CheckForForcedTransitToNonLPI();
    void setForceNLPI(bool enable) { force_nlpi_ = enable; }
    void GetVoiceUIProperties(struct pal_st_properties *qstp);
    int HandleDetectionStreamAction(pal_stream_type_t type, int32_t action, void *data);
    void HandleStreamPauseResume(pal_stream_type_t st_type, bool active);
    std::shared_ptr<CaptureProfile> GetASRCaptureProfileByPriority(
        StreamASR *s, std::shared_ptr<CaptureProfile> cap_prof_priority, std::string backend);
    std::shared_ptr<CaptureProfile> GetACDCaptureProfileByPriority(
        StreamACD *s, std::shared_ptr<CaptureProfile> cap_prof_priority, std::string backend);
    std::shared_ptr<CaptureProfile> GetSVACaptureProfileByPriority(
        StreamSoundTrigger *s, std::shared_ptr<CaptureProfile> cap_prof_priority, std::string backend);
    std::shared_ptr<CaptureProfile> GetSPDCaptureProfileByPriority(
        StreamSensorPCMData *s, std::shared_ptr<CaptureProfile> cap_prof_priority, std::string backend);
    std::shared_ptr<CaptureProfile> GetCaptureProfileByPriority(Stream *s, std::string backend);
    bool UpdateSoundTriggerCaptureProfile(Stream *s, bool is_active);
    std::shared_ptr<CaptureProfile> GetSoundTriggerCaptureProfile() const {
        std::lock_guard<std::mutex> lck(mResourceManagerMutex);
        return SoundTriggerCaptureProfile;
    }
    std::shared_ptr<CaptureProfile> GetTXMacroCaptureProfile() const {
        std::lock_guard<std::mutex> lck(mResourceManagerMutex);
        return TXMacroCaptureProfile;
    }
    void SwitchSoundTriggerDevices(bool connect_state, pal_device_id_t st_device);
    static void mixerEventWaitThreadLoop(std::shared_ptr<ResourceManager> rm);
    bool isCallbackRegistered() { return (mixerEventRegisterCount > 0); }
    int handleMixerEvent(struct mixer *mixer, char *mixer_str);
    int StopOtherDetectionStreams(void *st);
    int StartOtherDetectionStreams(void *st);
    void GetConcurrencyInfo(pal_stream_type_t st_type,
                         pal_stream_type_t in_type, pal_stream_direction_t dir,
                         bool *rx_conc, bool *tx_conc, bool *conc_en);
    void ConcurrentStreamStatus(pal_stream_type_t type,
                                pal_stream_direction_t dir,
                                bool active);
    void HandleConcurrencyForSoundTriggerStreams(pal_stream_type_t type,
                                pal_stream_direction_t dir,
                                bool active);
    bool isAnyVUIStreamBuffering();
    bool isTxConcurrencyActive() { return (TxconcurrencyEnableCount > 0); }
    void handleDeferredSwitch();
    void handleConcurrentStreamSwitch(std::vector<pal_stream_type_t>& st_streams);
    void forceSwitchSoundTriggerStreams(bool active);
    std::shared_ptr<Device> getActiveEchoReferenceRxDevices(Stream *tx_str);
    std::shared_ptr<Device> getActiveEchoReferenceRxDevices_l(Stream *tx_str);
    std::vector<Stream*> getConcurrentTxStream(
        Stream *rx_str, std::shared_ptr<Device> rx_device);
    std::vector<Stream*> getConcurrentTxStream_l(
        Stream *rx_str, std::shared_ptr<Device> rx_device);
    bool checkECRef(std::shared_ptr<Device> rx_dev,
                    std::shared_ptr<Device> tx_dev);
    bool isExternalECSupported(std::shared_ptr<Device> tx_dev);
    bool isExternalECRefEnabled(int rx_dev_id);
    void disableInternalECRefs(Stream *s);
    void restoreInternalECRefs();
    bool checkStreamMatch(Stream *target, Stream *ref);

    static void endTag(void *userdata __unused, const XML_Char *tag_name);
    static void snd_reset_data_buf(struct xml_userdata *data);
    static void snd_process_data_buf(struct xml_userdata *data, const XML_Char *tag_name);
    static void process_device_info(struct xml_userdata *data, const XML_Char *tag_name);
    static void process_input_streams(struct xml_userdata *data, const XML_Char *tag_name);
    static void process_config_voice(struct xml_userdata *data, const XML_Char *tag_name);
    static void process_config_volume(struct xml_userdata *data, const XML_Char *tag_name);
    static void process_config_lpm(struct xml_userdata *data, const XML_Char *tag_name);
    static void process_lpi_vote_streams(struct xml_userdata *data, const XML_Char *tag_name);
    static void process_snd_card_standby_support_streams(struct xml_userdata *data,
                                                        const XML_Char *tag_name);
    static void process_kvinfo(const XML_Char **attr, bool overwrite);
    static void process_voicemode_info(const XML_Char **attr);
    static void process_gain_db_to_level_map(struct xml_userdata *data, const XML_Char **attr);
    static void processCardInfo(struct xml_userdata *data, const XML_Char *tag_name);
    static void processSpkrTempCtrls(const XML_Char **attr);
    static void processBTCodecInfo(const XML_Char **attr, const int attr_count);
    static void processSilenceDetectionConfig(const XML_Char **attr);
    static void processPerfLockConfig(const XML_Char **attr);
    static void startTag(void *userdata __unused, const XML_Char *tag_name, const XML_Char **attr);
    static void snd_data_handler(void *userdata, const XML_Char *s, int len);
    static void processDeviceIdProp(struct xml_userdata *data, const XML_Char *tag_name);
    static void processDeviceCapability(struct xml_userdata *data, const XML_Char *tag_name);
    static void process_group_device_config(struct xml_userdata *data, const char* tag, const char** attr);
    static int getNativeAudioSupport();
    static int setNativeAudioSupport(int na_mode);
    static void getNativeAudioParams(struct str_parms *query,struct str_parms *reply,char *value, int len);
    static int setConfigParams(struct str_parms *parms);
    static int setNativeAudioParams(struct str_parms *parms,char *value, int len);
    static int setLoggingLevelParams(struct str_parms *parms,char *value, int len);
    static int setContextManagerEnableParam(struct str_parms *parms,char *value, int len);
    static int setLpiLoggingParams(struct str_parms *parms, char *value, int len);
    static int setUpdDedicatedBeEnableParam(struct str_parms *parms,char *value, int len);
    static int setUpdDutyCycleEnableParam(struct str_parms *parms,char *value, int len);
    static int setUpdVirtualPortParam(struct str_parms *parms, char *value, int len);
    static int setDualMonoEnableParam(struct str_parms *parms,char *value, int len);
    static int setSignalHandlerEnableParam(struct str_parms *parms,char *value, int len);
    static int setMuxconfigEnableParam(struct str_parms *parms,char *value, int len);
    static int setHapticsPriorityParam(struct str_parms *parms,char *value, int len);
    static int setHapticsDrivenParam(struct str_parms *parms,char *value, int len);
    static void setConnectivityProxyEnableParam(struct str_parms *parms,char *value, int len);
    static void setDummyDevEnableParam(struct str_parms *parms,char *value, int len);
    static bool isLpiLoggingEnabled();
    static void processConfigParams(const XML_Char **attr);
    static bool isValidDevId(int deviceId);
    static bool isValidStreamId(int streamId);
    static bool isOutputDevId(int deviceId);
    static bool isInputDevId(int deviceId);
    static bool matchDevDir(int devId1, int devId2);
    static int convertCharToHex(std::string num);
    static pal_stream_type_t getStreamType(std::string stream_name);
    static pal_device_id_t getDeviceId(std::string device_name);
    bool getScreenState();
    bool isDeviceAvailable(pal_device_id_t id);
    bool isDeviceAvailable(std::vector<std::shared_ptr<Device>> devices, pal_device_id_t id);
    bool isDeviceAvailable(struct pal_device *devices, uint32_t devCount, pal_device_id_t id);
    bool isDisconnectedDeviceStillActive(std::set<pal_device_id_t> &curPalDevices, std::set<pal_device_id_t> &activeDevices, pal_device_id_t id);
    bool isDeviceReady(pal_device_id_t id);
    static bool isBtScoDevice(pal_device_id_t id);
    static bool isBtDevice(pal_device_id_t id);
    static bool isBtA2dpDevice(pal_device_id_t id);
    int32_t a2dpSuspend(pal_device_id_t dev_id);
    int32_t a2dpSuspendToDummy(pal_device_id_t dev_id);
    int32_t a2dpResume(pal_device_id_t dev_id);
    int32_t a2dpResumeFromDummy(pal_device_id_t dev_id);
    int32_t a2dpCaptureSuspend(pal_device_id_t dev_id);
    int32_t a2dpCaptureSuspendToDummy(pal_device_id_t dev_id);
    int32_t a2dpCaptureResume(pal_device_id_t dev_id);
    int32_t a2dpCaptureResumeFromDummy(pal_device_id_t dev_id);
    int32_t a2dpReconfig();
    bool isPluginDevice(pal_device_id_t id);
    bool isDpDevice(pal_device_id_t id);
    bool isPluginPlaybackDevice(pal_device_id_t id);
    bool isPluginCaptureDevice(pal_device_id_t id);

    /* Separate device reference counts are maintained in PAL device and GSL device SGs.
     * lock graph is to sychronize these reference counts during device and session operations
     */
    void lockGraph() { mGraphMutex.lock(); };
    void unlockGraph() { mGraphMutex.unlock(); };
    void lockActiveStream() { mActiveStreamMutex.lock(); };
    void unlockActiveStream() { mActiveStreamMutex.unlock(); };
    void lockResourceManagerMutex() {mResourceManagerMutex.lock();};
    void unlockResourceManagerMutex() {mResourceManagerMutex.unlock();};
    void getSharedBEActiveStreamDevs(std::vector <std::tuple<Stream *, uint32_t>> &activeStreamDevs,
                                     int dev_id);
    bool compareSharedBEStreamDevAttr(std::vector <std::tuple<Stream *, uint32_t>> &sharedBEStreamDev,
                                     pal_device *newDevAttr, bool enable);
    int32_t streamDevSwitch(std::vector <std::tuple<Stream *, uint32_t>> streamDevDisconnectList,
                            std::vector <std::tuple<Stream *, struct pal_device *>> streamDevConnectList);
    char* getDeviceNameFromID(uint32_t id);
    int getPalValueFromGKV(pal_key_vector_t *gkv, int key);
    pal_speaker_rotation_type getCurrentRotationType();
    void ssrHandler(card_status_t state);
    int32_t getSidetoneMode(pal_device_id_t deviceId, pal_stream_type_t type,
                            sidetone_mode_t *mode);
    int getStreamInstanceID(Stream *str);
    int resetStreamInstanceID(Stream *str);
    int resetStreamInstanceID(Stream *str, uint32_t sInstanceID);
    static void setGaplessMode(const XML_Char **attr);
    static int initWakeLocks(void);
    static void deInitWakeLocks(void);
    void acquireWakeLock();
    void releaseWakeLock();
    static void process_custom_config(const XML_Char **attr);
    static void process_usecase();
    void getVendorConfigPath(char* config_file_path, int path_size);
    void restoreDevice(std::shared_ptr<Device> dev);
    bool doDevAttrDiffer(struct pal_device *inDevAttr,
                         struct pal_device *curDevAttr);
    int32_t voteSleepMonitor(Stream *str, bool vote, bool force_nlpi_vote = false);
    bool checkAndUpdateDeferSwitchState(bool stream_active);
    static uint32_t palFormatToBitwidthLookup(const pal_audio_fmt_t format);
    void chargerListenerFeatureInit();
    static void chargerListenerInit(charger_status_change_fn_t);
    static void chargerListenerDeinit();
    static void onChargerListenerStatusChanged(int event_type, int status,
                                                 bool concurrent_state);
    int chargerListenerSetBoostState(bool state, charger_boost_mode_t mode);
    int handlePBChargerInsertion(Stream *stream);
    int handlePBChargerRemoval(Stream *stream);
    static bool isGroupConfigAvailable(group_dev_config_idx_t idx);
    int checkAndUpdateGroupDevConfig(struct pal_device *deviceattr,
                                 const struct pal_stream_attributes *sAttr,
                                 std::vector<Stream*> &streamsToSwitch,
                                 struct pal_device *streamDevAttr,
                                 bool streamEnable);
    void checkHapticsConcurrency(struct pal_device *deviceattr,
                             const struct pal_stream_attributes *sAttr,
                             std::vector<Stream*> &streamsToSwitch,
                             struct pal_device *streamDevAttr);
    static void sendCrashSignal(int signal, pid_t pid, uid_t uid);
    static bool isSsrDownFeasible(std::shared_ptr<ResourceManager> rm, int type);
    bool isStreamSupportedInsndCardStandy(uint32_t type);
    void checkAndSetDutyCycleParam();
    int32_t getActiveVoiceCallDevices(std::vector <std::shared_ptr<Device>> &devices);
    bool isValidDeviceSwitchForStream(Stream *s, pal_device_id_t newDeviceId);
    int32_t reconfigureInCallMusicStream(struct sessionToPayloadParam deviceData);
    int32_t resumeInCallMusic();
    int32_t pauseInCallMusic();
    void RegisterSTCaptureHandle(pal_param_st_capture_info_t stCaptureInfo, bool start);
    static void setProxyRecordActive(bool isActive);
    void reconfigureScoStreams();

#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
    void notifyClientSoundTriggerResourceAvailable();
#endif
#ifdef SEC_AUDIO_COMMON
    struct tm * ssrTimeinfo = NULL;
    void dump(int fd);
#endif
#ifdef SEC_AUDIO_BARGEIN_MODE
    static int bargein_mode;
    int GetBargeinMode();
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
    static int interpreter_mode;
#endif
#ifdef SEC_AUDIO_MUTE_DETECTION
    int getKernelConfigInfo(char *mixer_str);
    int setKernelRegDump(char *mixer_str);
#endif
#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO
    static bool isBtmixRecord;
#endif
};

#endif
