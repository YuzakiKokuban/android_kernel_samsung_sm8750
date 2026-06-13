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
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#include "Device.h"
#include <tinyalsa/asoundlib.h>
#include <bt_intf.h>
#include <bt_ble.h>
#include <vector>
#include <mutex>
#include <system/audio.h>
#include "Session.h"

#include "SecProductFeature_BLUETOOTH.h"

#define DISALLOW_COPY_AND_ASSIGN(name) \
    name(const name &); \
    name &operator=(const name &)

enum A2DP_STATE {
    A2DP_STATE_CONNECTED,
    A2DP_STATE_STARTED,
    A2DP_STATE_STOPPED,
    A2DP_STATE_DISCONNECTED,
};

#define SPEECH_MODE_INVALID 0xFFFF

#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
#define BINAURAL_RECORDING_LATENCY 185
#define BT_MIX_LATENCY 185
#endif

enum A2DP_ROLE {
    SOURCE = 0,
    SINK,
};

#define TO_AIR		0
#define FROM_AIR	1

enum streamMapDir {
    STREAM_MAP_IN = 1 << 0,
    STREAM_MAP_OUT = 1 << 1,
};

typedef enum {
    SESSION_UNKNOWN,
    /** A2DP legacy that AVDTP media is encoded by Bluetooth Stack */
    A2DP_SOFTWARE_ENCODING_DATAPATH,
    /** The encoding of AVDTP media is done by HW and there is control only */
    A2DP_HARDWARE_OFFLOAD_DATAPATH,
    /** Used when encoded by Bluetooth Stack and streaming to Hearing Aid */
    HEARING_AID_SOFTWARE_ENCODING_DATAPATH,
    /** Used when encoded by Bluetooth Stack and streaming to LE Audio device */
    LE_AUDIO_SOFTWARE_ENCODING_DATAPATH,
    /** Used when decoded by Bluetooth Stack and streaming to audio framework */
    LE_AUDIO_SOFTWARE_DECODED_DATAPATH,
    /** Encoding is done by HW an there is control only */
    LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
    /** Decoding is done by HW an there is control only */
    LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH,
    /** SW Encoding for LE Audio Broadcast */
    LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH,
    /** HW Encoding for LE Audio Broadcast */
    LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
}tSESSION_TYPE;

typedef enum {
    CTRL_ACK_SUCCESS,
    CTRL_ACK_UNSUPPORTED,
    CTRL_ACK_FAILURE,
    CTRL_ACK_PENDING,
    CTRL_ACK_INCALL_FAILURE,
    CTRL_ACK_DISCONNECT_IN_PROGRESS,
    CTRL_SKT_DISCONNECTED,
    CTRL_ACK_UNKNOWN,
    CTRL_ACK_RECONFIGURATION,
};

typedef void (*bt_audio_pre_init_t)(void);
typedef int (*audio_source_open_api_t)(tSESSION_TYPE session_type);
typedef int (*audio_source_close_api_t)(tSESSION_TYPE session_type);
typedef int (*audio_source_start_api_t)(tSESSION_TYPE session_type);
typedef int (*audio_source_stop_api_t)(tSESSION_TYPE session_type);
typedef int (*audio_source_suspend_api_t)(tSESSION_TYPE session_type);
typedef void (*audio_source_handoff_triggered_t)(void);
typedef void (*clear_source_a2dpsuspend_flag_t)(void);
typedef void * (*audio_get_enc_config_api_t)(tSESSION_TYPE session_type, uint8_t *multicast_status,
                                        uint8_t *num_dev, audio_format_t *codec_format);
typedef int (*audio_source_check_a2dp_ready_api_t)(tSESSION_TYPE session_type);
typedef bool (*audio_is_tws_mono_mode_enable_t)(void);
typedef int (*audio_sink_start_api_t)(tSESSION_TYPE session_type);
typedef int (*audio_sink_stop_api_t)(tSESSION_TYPE session_type);
typedef void * (*audio_get_dec_config_t)(audio_format_t *codec_format);
typedef void * (*audio_sink_session_setup_complete_t)(uint64_t system_latency);
typedef int (*audio_sink_check_a2dp_ready_t)(void);
typedef uint16_t (*audio_sink_get_a2dp_latency_api_t)(tSESSION_TYPE session_type);
typedef bool (*audio_is_scrambling_enabled_t)(void);
typedef int (*audio_sink_suspend_api_t)(tSESSION_TYPE session_type);
typedef void (*btoffload_update_metadata_api_t)(tSESSION_TYPE session_type, void* metadata);
typedef int (*audio_sink_open_api_t)(tSESSION_TYPE session_type);
typedef int (*audio_sink_close_api_t)(tSESSION_TYPE session_type);
typedef int (*audio_source_get_supported_latency_modes_api_t)(tSESSION_TYPE session_type, size_t *num_modes, size_t max_latency_modes, uint32_t *modes);
typedef int (*audio_source_set_latency_mode_api_t)(tSESSION_TYPE session_type, uint32_t mode);

typedef int (*audio_source_open_t)(void);
typedef int (*audio_source_close_t)(void);
typedef int (*audio_source_start_t)(void);
typedef int (*audio_source_stop_t)(void);
typedef int (*audio_source_suspend_t)(void);
typedef void* (*audio_get_enc_config_t)(uint8_t* multicast_status,
    uint8_t* num_dev, audio_format_t* codec_format);
typedef int (*audio_source_check_a2dp_ready_t)(void);
typedef int (*audio_sink_start_t)(void);
typedef int (*audio_sink_stop_t)(void);
typedef uint16_t(*audio_sink_get_a2dp_latency_t)(void);
typedef int (*audio_sink_suspend_t)(void);
typedef int (*audio_sink_open_t)(void);
typedef int (*audio_sink_close_t)(void);
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
//typedef void (tbit_rate_cback)(uint32_t bitrate);
typedef void (*audio_get_dynamic_bitrate_t)(void (*bitrate_cback)(uint32_t bitrate));
//typedef void (tpeer_mtu_cback)(int mtu);
typedef void (*audio_get_peer_mtu_t)(void (*peer_mtu_cback)(uint32_t mtu));
#endif

// Abstract base class
class Bluetooth : public Device
{
protected:
    Bluetooth(struct pal_device *device, std::shared_ptr<ResourceManager> Rm);

    codec_type                 codecType;
    struct pal_media_config    codecConfig;
    codec_format_t             codecFormat;
    void                       *codecInfo;
    void                       *pluginHandler;
    bt_codec_t                 *pluginCodec;
    bool                       isAbrEnabled;
    bool                       isConfigured;
    bool                       isLC3MonoModeOn;
    bool                       isTwsMonoModeOn;
    bool                       isScramblingEnabled;
    bool                       isDummySink;
    struct pcm                 *fbPcm;
    std::vector<int>           fbpcmDevIds;
    std::shared_ptr<Bluetooth> fbDev;
    int                        abrRefCnt;
    std::mutex                 mAbrMutex;
    int                        totalActiveSessionRequests;
    codec_version_t            codecVersion;

#if defined(SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD) && !defined(QCA_OFFLOAD)
    void set_a2dp_suspend(int a2dp_suspend);
#endif

    int32_t getPCMId();
    int checkAndUpdateCustomPayload(uint8_t **paramData, size_t *paramSize);
    int getPluginPayload(void **handle, bt_codec_t **btCodec,
                         bt_enc_payload_t **out_buf,
                         codec_type codecType);
    int configureCOPModule(int32_t pcmId, const char *backendName, uint32_t tagId, uint32_t streamMapDir, bool isFbpayload);
    int configureRATModule(int32_t pcmId, const char *backendName, uint32_t tagId, bool isFbpayload);
    int configurePCMConverterModule(int32_t pcmId, const char *backendName, uint32_t tagId, bool isFbpayload);
    int configureGraphModules();
    int configureNrecParameters(bool isNrecEnabled);
    int updateDeviceMetadata();
    void updateDeviceAttributes();
    bool isPlaceholderEncoder();
    void startAbr();
    void stopAbr();
    int32_t configureSlimbusClockSrc(void);
public:
    int getCodecConfig(struct pal_media_config *config) override;
    virtual ~Bluetooth();
};

class BtA2dp : public Bluetooth
{
protected:
    static std::shared_ptr<Device> objRx;
    static std::shared_ptr<Device> objTx;
    static std::shared_ptr<Device> objBleRx;
    static std::shared_ptr<Device> objBleTx;
    static std::shared_ptr<Device> objBleBroadcastRx;
    BtA2dp(struct pal_device *device, std::shared_ptr<ResourceManager> Rm);
    pal_param_bta2dp_t param_bt_a2dp;

private:
    /* BT IPC related members */
    static void                                 *bt_lib_source_handle;
    static bt_audio_pre_init_t                  bt_audio_pre_init;
    static audio_source_open_api_t              audio_source_open_api;
    static audio_source_close_api_t             audio_source_close_api;
    static audio_source_start_api_t             audio_source_start_api;
    static audio_source_stop_api_t              audio_source_stop_api;
    static audio_source_suspend_api_t           audio_source_suspend_api;
    static audio_source_handoff_triggered_t     audio_source_handoff_triggered;
    static clear_source_a2dpsuspend_flag_t      clear_source_a2dpsuspend_flag;
    static audio_get_enc_config_api_t           audio_get_enc_config_api;
    static audio_source_check_a2dp_ready_api_t  audio_source_check_a2dp_ready_api;
    static audio_is_tws_mono_mode_enable_t      audio_is_tws_mono_mode_enable;
    static audio_sink_get_a2dp_latency_api_t    audio_sink_get_a2dp_latency_api;

    static void                                 *bt_lib_sink_handle;
    static audio_sink_start_api_t               audio_sink_start_api;
    static audio_sink_stop_api_t                audio_sink_stop_api;
    static audio_get_dec_config_t               audio_get_dec_config;
    static audio_sink_session_setup_complete_t  audio_sink_session_setup_complete;
    static audio_sink_check_a2dp_ready_t        audio_sink_check_a2dp_ready;
    static audio_is_scrambling_enabled_t        audio_is_scrambling_enabled;
    static audio_sink_suspend_api_t             audio_sink_suspend_api;
    static btoffload_update_metadata_api_t      btoffload_update_metadata_api;
    static audio_sink_open_api_t                audio_sink_open_api;
    static audio_sink_close_api_t               audio_sink_close_api;
    static audio_source_get_supported_latency_modes_api_t audio_source_get_supported_latency_modes_api;
    static audio_source_set_latency_mode_api_t audio_source_set_latency_mode_api;

    static audio_source_open_t                  audio_source_open;
    static audio_source_close_t                 audio_source_close;
    static audio_source_start_t                 audio_source_start;
    static audio_source_stop_t                  audio_source_stop;
    static audio_source_suspend_t               audio_source_suspend;
    static audio_get_enc_config_t               audio_get_enc_config;
    static audio_source_check_a2dp_ready_t      audio_source_check_a2dp_ready;
    static audio_sink_get_a2dp_latency_t        audio_sink_get_a2dp_latency;

    static audio_sink_start_t                   audio_sink_start;
    static audio_sink_stop_t                    audio_sink_stop;
    static audio_sink_suspend_t                 audio_sink_suspend;
    static audio_sink_open_t                    audio_sink_open;
    static audio_sink_close_t                   audio_sink_close;
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    static audio_get_dynamic_bitrate_t          audio_get_dynamic_bitrate;
    static audio_get_peer_mtu_t                 audio_get_peer_mtu;
#endif
    /* member variables */
    uint8_t         a2dpRole;  // source or sink
    enum A2DP_STATE a2dpState;
    bool            isA2dpOffloadSupported;
    uint32_t        a2dpLatencyMode;
    uint32_t        codecLatency;

#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    uint32_t delay_report;
    bool use_offload_hal;
    audio_source_t audio_source;
#ifdef QCA_OFFLOAD
    bool game_mode;
#endif
#endif

    uint32_t getLatency(uint32_t slatency);
    int startPlayback();
    int stopPlayback();
    int startCapture();
    int stopCapture();

    /* common member funtions */
    void init_a2dp_source();
    void open_a2dp_source();
    int close_audio_source();
    tSESSION_TYPE get_session_type();

    void init_a2dp_sink();
    void open_a2dp_sink();
    int close_audio_sink();
    bool a2dp_send_sink_setup_complete(void);
    using Bluetooth::init;
    void init();

public:
    int start();
    int stop();
    bool isDeviceReady() override;
    int32_t setDeviceParameter(uint32_t param_id, void *param) override;
    int32_t getDeviceParameter(uint32_t param_id, void **param) override;

    static std::shared_ptr<Device> getObject(pal_device_id_t id);
    static std::shared_ptr<Device> getInstance(struct pal_device *device,
                                               std::shared_ptr<ResourceManager> Rm);

#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    bool is_a2dp_offload();
    enum A2DP_STATE getA2dpState();
    codec_format_t getCodecFormat();
#ifdef QCA_OFFLOAD
    void setGameMode(bool gameMode);
#endif
#endif

    virtual ~BtA2dp();
    DISALLOW_COPY_AND_ASSIGN(BtA2dp);
    int32_t checkDeviceStatus();
};

class BtSco : public Bluetooth
{
protected:
    static std::shared_ptr<Device> objRx;
    static std::shared_ptr<Device> objTx;
    BtSco(struct pal_device *device, std::shared_ptr<ResourceManager> Rm);
    static bool isScoOn;
    static bool isWbSpeechEnabled;
    static int  swbSpeechMode;
    static bool isSwbLc3Enabled;
    static audio_lc3_codec_cfg_t lc3CodecInfo;
    static bool isNrecEnabled;
    int startSwb();
// SS_BT_HFP - H_127 : RVP
#ifdef SEC_AUDIO_BLUETOOTH
    static int rvpMode;
    int startRvp();
#endif
// SS_BT_HFP - H_127 end

public:
    int start();
    int stop();
    bool isDeviceReady() override;
    int32_t setDeviceParameter(uint32_t param_id, void *param) override;
    void convertCodecInfo(audio_lc3_codec_cfg_t &lc3CodecInfo, btsco_lc3_cfg_t &lc3Cfg);
    void updateSampleRate(uint32_t *sampleRate);
    bool isScoNbWbActive() override;
    static std::shared_ptr<Device> getObject(pal_device_id_t id);
    static std::shared_ptr<Device> getInstance(struct pal_device *device,
                                               std::shared_ptr<ResourceManager> Rm);
    virtual ~BtSco();
    DISALLOW_COPY_AND_ASSIGN(BtSco);
};

#endif /* _BLUETOOTH_H_ */
