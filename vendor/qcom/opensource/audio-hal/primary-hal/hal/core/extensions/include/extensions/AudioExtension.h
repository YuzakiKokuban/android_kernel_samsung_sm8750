/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <PalApi.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>
#include <memory>
#include <mutex>
#include <string>
#include <aidl/android/media/audio/common/AudioDevice.h>
#include <qti-audio-core/Platform.h>
#include "extensions/battery_listener.h"

#ifdef SEC_AUDIO_DSM_AMP
#ifdef SEC_AUDIO_VI_FEEDBACK
#define CODEC_BACKEND_FEEDBACK_BIT_WIDTH 24
#else
#define CODEC_BACKEND_FEEDBACK_BIT_WIDTH 16
#endif
#endif

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
    MAX,
} tSESSION_TYPE;

namespace qti::audio::core {
// RAII based classes to dlopen/dysym on init and dlclose on dest.

#ifdef __LP64__
static std::string kBluetoothIpcLibrary = "/vendor/lib64/btaudio_offload_if.so";
#else
static std::string kBluetoothIpcLibrary = "/vendor/lib/btaudio_offload_if.so";
#endif
static std::string kBatteryListenerLibrary = std::string("libbatterylistener.so");
static std::string kHfpLibrary = "libhfp_pal.so";
static std::string kFmLibrary = "libfmpal.so";
static std::string kKarokeLibrary = "dummy.so"; // TODO
static std::string kGefLibrary = "libqtigefar.so";
#ifdef SEC_AUDIO_COMMON
static std::string kDummyLibrary = "dummy.so";
#endif

static std::string kBatteryListenerProperty = "vendor.audio.feature.battery_listener.enable";
static std::string kHfpProperty = "vendor.audio.feature.hfp.enable";
static std::string kBluetoothProperty = "vendor.audio.feature.a2dp_offload.enable";

const std::map<tSESSION_TYPE, pal_device_id_t> SessionTypePalDevMap{
        {A2DP_HARDWARE_OFFLOAD_DATAPATH, PAL_DEVICE_OUT_BLUETOOTH_A2DP},
        {LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH, PAL_DEVICE_OUT_BLUETOOTH_BLE},
        {LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH, PAL_DEVICE_IN_BLUETOOTH_BLE},
        {LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
         PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST},
};

typedef enum {
    /**If reconfiguration is in progress state */
    SESSION_SUSPEND,
    /**If reconfiguration is in complete state */
    SESSION_RESUME,
    /**To set Lc3 channel mode as Mono */
    CHANNEL_MONO,
    /**To set LC3 channel mode as Stereo */
    CHANNEL_STEREO,
} tRECONFIG_STATE;

const std::map<int32_t, std::string> reconfigStateName{
        {SESSION_SUSPEND, std::string{"SESSION_SUSPEND"}},
        {SESSION_RESUME, std::string{"SESSION_RESUME"}},
        {CHANNEL_MONO, std::string{"CHANNEL_MONO"}},
        {CHANNEL_STEREO, std::string{"CHANNEL_STEREO"}},
};

typedef void (*batt_listener_init_t)(battery_status_change_fn_t);
typedef void (*batt_listener_deinit_t)();
typedef bool (*batt_prop_is_charging_t)();

typedef void (*set_parameters_t)(struct str_parms*);
typedef void (*hfp_set_parameters_t)(bool val, struct str_parms*);
typedef void (*get_parameters_t)(struct str_parms*, struct str_parms*);
typedef bool (*fm_running_status_t)();

typedef void (*hfp_init_t)();
typedef bool (*hfp_is_active_t)();
typedef int (*hfp_get_usecase_t)();
typedef int (*hfp_set_mic_mute_t)(bool state);
typedef int (*hfp_set_mic_mute2_t)(bool state);
typedef void (*hfp_set_device_t)(struct pal_device *devices);

typedef void (*a2dp_bt_audio_pre_init_t)(void);
typedef void (*register_reconfig_cb_t)(int (*reconfig_cb)(tSESSION_TYPE, int));

typedef void (*gef_init_t)(void);
typedef void (*gef_deinit_t)(void);

static bool isExtensionEnabled(std::string property) {
    return property_get_bool(property.c_str(), false);
}
class AudioExtensionBase {
  public:
    AudioExtensionBase(std::string library, bool enabled = true);
    ~AudioExtensionBase();
#ifdef SEC_AUDIO_SUPPORT_USB_OFFLOAD
    static void setUSBCardConfig(pal_usb_device_address addr) { mUsbAddr = addr; }
#endif
  protected:
    void* mHandle = nullptr;
    bool mEnabled;
#ifdef SEC_AUDIO_SUPPORT_USB_OFFLOAD
    static struct pal_usb_device_address mUsbAddr;
#endif
    std::string mLibraryName;
    Platform& mPlatform{Platform::getInstance()};

  private:
    void cleanUp();
};

class BatteryListenerExtension : public AudioExtensionBase {
  public:
    BatteryListenerExtension();
    ~BatteryListenerExtension();
    void battery_properties_listener_init();
    void battery_properties_listener_deinit();
    bool battery_properties_is_charging();
    static void setChargingMode(bool is_charging);
    static bool isCharging;
    // void on_battery_status_changed(bool charging);
  private:
    batt_listener_init_t batt_listener_init;
    batt_listener_deinit_t batt_listener_deinit;
    batt_prop_is_charging_t batt_prop_is_charging;
};

class A2dpExtension : public AudioExtensionBase {
  public:
    A2dpExtension();
    ~A2dpExtension();

    a2dp_bt_audio_pre_init_t a2dp_bt_audio_pre_init = nullptr;
    register_reconfig_cb_t register_reconfig_cb = nullptr;
};

class HfpExtension : public AudioExtensionBase {
  public:
    HfpExtension();
    ~HfpExtension();
    bool audio_extn_hfp_is_active();
    int audio_extn_hfp_set_mic_mute(bool state);
    int audio_extn_hfp_set_mic_mute2(bool state);
    void audio_extn_hfp_set_parameters(struct str_parms* params);
    void audio_extn_hfp_set_device(const std::vector<::aidl::android::media::audio::common::AudioDevice>&
            devices, const bool updateRx);
    ::aidl::android::media::audio::common::AudioDevice audio_extn_hfp_get_matching_tx_device(
            const ::aidl::android::media::audio::common::AudioDevice& rxDevice);

  private:
    hfp_init_t hfp_init;
    hfp_is_active_t hfp_is_active;
    hfp_get_usecase_t hfp_get_usecase;
    hfp_set_mic_mute_t hfp_set_mic_mute;
    hfp_set_parameters_t hfp_set_parameters;
    hfp_set_mic_mute2_t hfp_set_mic_mute2;
    hfp_set_device_t hfp_set_device;
    bool micMute;
};

#ifdef SEC_AUDIO_CALL_SATELLITE
class ExtModemCallExtension : public AudioExtensionBase {
  public:
    ExtModemCallExtension();
    ~ExtModemCallExtension();
    int32_t startCall(struct pal_device *callDevices);
    void stopCall();
    void setDevice(struct pal_device *devices);
    bool isCallActive() { return isExtModemCallRunning; }
    pal_stream_handle_t* getRxStreamHandle() { return rxStreamHandle; };
    pal_stream_handle_t* getTxStreamHandle() { return txStreamHandle; };

  private:
    bool hasValidStreamHandle() { return (rxStreamHandle && txStreamHandle); }
    bool isValidOutDevice(pal_device_id_t id);
    bool isValidInDevice(pal_device_id_t id);
    bool isUsbDevice(pal_device_id_t id);
    void setCustomKey(pal_device& palDevice, const pal_device_id_t outDeviceId);
    void configurePalDevices(struct pal_device *palDevices, const pal_device_id_t callRxDeviceId);
    std::unique_ptr<pal_stream_attributes> getExtModemCallAttributes(
                                          pal_stream_loopback_type_t type);
    bool isExtModemCallRunning;
    pal_stream_handle_t *rxStreamHandle;
    pal_stream_handle_t *txStreamHandle;
};
#endif

class FmExtension : public AudioExtensionBase {
  public:
    FmExtension();
    ~FmExtension();
    set_parameters_t fm_set_params;
    fm_running_status_t fm_running_status;
    void audio_extn_fm_set_parameters(struct str_parms* params);
    bool audio_extn_fm_get_status();
};

class KarokeExtension : public AudioExtensionBase {
  public:
    KarokeExtension();
    ~KarokeExtension();
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
    void init();
    bool isKaraokeActive();
    bool isVoiceRecognitionStreamCreated() { return mIsVoiceRecognitionStreamCreated; }
    void setVoiceRecognitionStreamCreated(bool on) { mIsVoiceRecognitionStreamCreated = on; }
#endif
    int karaoke_open(pal_device_id_t device_out, pal_stream_callback pal_callback,
                     pal_channel_info ch_info);
    int karaoke_start();
    int karaoke_stop();
    int karaoke_close();
  protected:
    pal_stream_handle_t* karaoke_stream_handle;
    struct pal_stream_attributes sattr;
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
    bool mIsVoiceRecognitionStreamCreated = false;
#endif
};

#ifdef SEC_AUDIO_SUPPORT_REMOTE_MIC
class AasExtension : public AudioExtensionBase {
  public:
    AasExtension();
    ~AasExtension();
    int startAasStream(const pal_device_id_t outDeviceId);
    int stopAasStream();
    int updateAasStream(const bool enable, const pal_device_id_t outDeviceId);
  protected:
    bool isAasActive() { return (mAasStreamHandle != nullptr) ? true : false; }
    bool isAasDeviceAvailable(const pal_device_id_t deviceId);
    bool isValidStatusForAas(const pal_device_id_t outDeviceId);
    void setAasCustomKey(pal_device& palDevice, const pal_device_id_t outDeviceId);
    pal_device_id_t mAasOutDeviceId;
    pal_stream_handle_t *mAasStreamHandle;
};
#endif

#ifdef SEC_AUDIO_DSM_AMP
class SpeakerFeedbackExtension : public AudioExtensionBase {
public:
    SpeakerFeedbackExtension();
    ~SpeakerFeedbackExtension();
    void init();
    void start();
    int setDevice(pal_stream_handle_t *stream_handle,
                    uint32_t no_of_devices, struct pal_device *devices);
    void stop();
    void close();
protected:
    pal_stream_handle_t *mFeedbackStreamHandle;
    bool mSupportViFeedback;
    std::mutex mFeedbackMutex;
};
#endif

class GefExtension : public AudioExtensionBase {
  public:
    GefExtension();
    ~GefExtension();
    void gef_interface_init();
    void gef_interface_deinit();

  private:
    gef_init_t gef_init;
    gef_deinit_t gef_deinit;
};

class AudioExtension {
  public:
    static AudioExtension& getInstance() {
        static const auto kAudioExtension = []() {
            std::unique_ptr<AudioExtension> audioExt{new AudioExtension()};
            return std::move(audioExt);
        }();
        return *(kAudioExtension.get());
    }
    void audio_extn_set_parameters(struct str_parms* params);
    void audio_extn_get_parameters(struct str_parms* params, struct str_parms* reply);
    void audio_feature_stats_set_parameters(struct str_parms* params);
    explicit AudioExtension() = default;
    AudioExtension(const AudioExtension&) = delete;
    AudioExtension& operator=(const AudioExtension& x) = delete;

    AudioExtension(AudioExtension&& other) = delete;
    AudioExtension& operator=(AudioExtension&& other) = delete;
    std::unique_ptr<BatteryListenerExtension> mBatteryListenerExtension =
            std::make_unique<BatteryListenerExtension>();
    std::unique_ptr<A2dpExtension> mA2dpExtension = std::make_unique<A2dpExtension>();
    std::unique_ptr<HfpExtension> mHfpExtension = std::make_unique<HfpExtension>();
#ifdef SEC_AUDIO_CALL_SATELLITE
    std::unique_ptr<ExtModemCallExtension> mExtModemCallExtension = std::make_unique<ExtModemCallExtension>();
#endif
    std::unique_ptr<FmExtension> mFmExtension = std::make_unique<FmExtension>();
    std::unique_ptr<KarokeExtension> mKarokeExtension = std::make_unique<KarokeExtension>();
#ifdef SEC_AUDIO_SUPPORT_REMOTE_MIC
    std::unique_ptr<AasExtension> mAasExtension = std::make_unique<AasExtension>();
#endif
#ifdef SEC_AUDIO_DSM_AMP
    std::unique_ptr<SpeakerFeedbackExtension> mSpeakerFeedbackExtension
                                    = std::make_unique<SpeakerFeedbackExtension>();
#endif
    std::unique_ptr<GefExtension> mGefExtension = std::make_unique<GefExtension>();
    static std::mutex reconfig_wait_mutex_;
};
} // namespace qti::audio::core
