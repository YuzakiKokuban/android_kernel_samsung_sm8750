/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AHAL_AudioExtension_QTI"

#include <Utils.h>
#include <qti-audio-core/Utils.h>
#include <android-base/logging.h>
#include <dlfcn.h>
#include <extensions/AudioExtension.h>
#include <log/log.h>
#include "PalApi.h"
#ifdef SEC_AUDIO_SUPPORT_REMOTE_MIC
#include <unordered_set>
#endif
#ifdef SEC_AUDIO_COMMON
#include "SecPalDefs.h"
#endif

#define DEFAULT_OUTPUT_SAMPLING_RATE 48000
#define CODEC_BACKEND_DEFAULT_BIT_WIDTH 16
#define AFS_PARAMETER_QVA_VERSION "qva.version"

#define AUDIO_PARAMETER_KEY_CAN_OPEN_PROXY "can_open_proxy"

#define AFS_QVA_FILE_NAME "/data/vendor/audio/adc_qva_version.txt"

using ::aidl::android::media::audio::common::AudioDevice;
using ::aidl::android::media::audio::common::AudioDeviceType;
using ::aidl::android::media::audio::common::AudioDeviceDescription;

namespace qti::audio::core {

std::mutex AudioExtension::reconfig_wait_mutex_;
bool BatteryListenerExtension::isCharging;
#ifdef SEC_AUDIO_SUPPORT_USB_OFFLOAD
struct pal_usb_device_address AudioExtensionBase::mUsbAddr;
#endif

AudioExtensionBase::AudioExtensionBase(std::string library, bool enabled)
    : mLibraryName(library), mEnabled(enabled) {
    LOG(INFO) << __func__ << " opening " << mLibraryName.c_str() << " enabled " << enabled;
    if (mEnabled) {
        mHandle = dlopen(mLibraryName.c_str(), RTLD_LAZY);
        if (mHandle == nullptr) {
            const char *error = dlerror();
            LOG(INFO) << __func__ << " Failed to dlopen  " << mLibraryName.c_str() << " error  "
                      << error;
        }
    }
#ifdef SEC_AUDIO_SUPPORT_USB_OFFLOAD
    mUsbAddr.card_id = -1;
    mUsbAddr.device_num = -1;
#endif
}

AudioExtensionBase::~AudioExtensionBase() {
    cleanUp();
}

void AudioExtension::audio_extn_get_parameters(struct str_parms *query, struct str_parms *reply) {
    char *kv_pairs = NULL;
    char value[32] = {0};
    int ret, val = 0;
}
void AudioExtension::audio_extn_set_parameters(struct str_parms *params) {
    mHfpExtension->audio_extn_hfp_set_parameters(params);
    mFmExtension->audio_extn_fm_set_parameters(params);
    audio_feature_stats_set_parameters(params);
}

void AudioExtension::audio_feature_stats_set_parameters(struct str_parms *params) {
    FILE *fp;
    int status = 0;
    char value[50] = {0};

    status = str_parms_get_str(params, AFS_PARAMETER_QVA_VERSION, value, sizeof(value));
    if (status >= 0) {
        fp = fopen(AFS_QVA_FILE_NAME, "w");
        if (!fp) {
            LOG(ERROR) << __func__ << " File open failed for write";
        } else {
            char qva_version[50] = "qva_version=";
            strlcat(qva_version, value, sizeof(qva_version));
            LOG(DEBUG) << __func__ << " QVA Version : " << qva_version;
            fprintf(fp, "%s", qva_version);
            fclose(fp);
        }
    }
}

void AudioExtensionBase::cleanUp() {
    if (mHandle != nullptr) {
        dlclose(mHandle);
    }
}

void BatteryListenerExtension::setChargingMode(bool is_charging) {
    int32_t result = 0;
    pal_param_charging_state_t charge_state;

    LOG(DEBUG) << __func__ << " enter, is_charging " << is_charging;
    isCharging = is_charging;
    charge_state.charging_state = is_charging;

    result = pal_set_param(PAL_PARAM_ID_CHARGING_STATE, (void *)&charge_state,
                           sizeof(pal_param_charging_state_t));
    if (result) LOG(DEBUG) << __func__ << " error while handling charging event result " << result;

    LOG(DEBUG) << __func__ << " exit";
}

void on_battery_status_changed(bool charging) {
    LOG(DEBUG) << __func__ << " battery status changed to " << charging;
    BatteryListenerExtension::setChargingMode(charging);
}

BatteryListenerExtension::~BatteryListenerExtension() {
    battery_properties_listener_deinit();
}

void BatteryListenerExtension::battery_properties_listener_deinit() {
    if (batt_listener_deinit) batt_listener_deinit();
}

bool BatteryListenerExtension::battery_properties_is_charging() {
    return (batt_prop_is_charging) ? batt_prop_is_charging() : false;
}

void BatteryListenerExtension::battery_properties_listener_init() {
    if (batt_listener_init) batt_listener_init(on_battery_status_changed);
}

BatteryListenerExtension::BatteryListenerExtension()
    : AudioExtensionBase(kBatteryListenerLibrary, isExtensionEnabled(kBatteryListenerProperty)) {
    LOG(INFO) << __func__ << " Enter";
    if (mHandle != nullptr) {
        if (!(batt_listener_init =
                      (batt_listener_init_t)dlsym(mHandle, "battery_properties_listener_init")) ||
            !(batt_listener_deinit = (batt_listener_deinit_t)dlsym(
                      mHandle, "battery_properties_listener_deinit")) ||
            !(batt_prop_is_charging =
                      (batt_prop_is_charging_t)dlsym(mHandle, "battery_properties_is_charging"))) {
            LOG(ERROR) << __func__ << "dlsym failed";
            goto feature_disabled;
        }
        LOG(INFO) << __func__ << "----- Feature BATTERY_LISTENER is enabled ----";
        battery_properties_listener_init();
        setChargingMode(battery_properties_is_charging());
        return;
    }

feature_disabled:
    if (mHandle) {
        dlclose(mHandle);
        mHandle = NULL;
    }

    batt_listener_init = NULL;
    batt_listener_deinit = NULL;
    batt_prop_is_charging = NULL;
    LOG(INFO) << __func__ << "----- Feature BATTERY_LISTENER is disabled ----";
}

static int reconfig_cb(tSESSION_TYPE session_type, int state) {
    int ret = 0;
    pal_param_bta2dp_t param_bt_a2dp;
    LOG(DEBUG) << __func__ << " reconfig_cb enter with state "
               << reconfigStateName.at(state).c_str() << " for "
               << deviceNameLUT.at(SessionTypePalDevMap.at(session_type)).c_str();

    /* If reconfiguration is in progress state (state = 0), perform a2dp suspend.
     * If reconfiguration is in complete state (state = 1), perform a2dp resume.
     * Set LC3 channel mode as mono (state = 2).
     * Set LC3 channel mode as stereo (state = 3).
     */
    if (session_type == LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
        if ((tRECONFIG_STATE)state == SESSION_SUSPEND) {
            std::unique_lock<std::mutex> guard(AudioExtension::reconfig_wait_mutex_);
            param_bt_a2dp.a2dp_suspended = true;
            param_bt_a2dp.is_suspend_setparam = false;
            param_bt_a2dp.dev_id = PAL_DEVICE_OUT_BLUETOOTH_BLE;

            ret = pal_set_param(PAL_PARAM_ID_BT_A2DP_SUSPENDED, (void *)&param_bt_a2dp,
                                sizeof(pal_param_bta2dp_t));
        } else if ((tRECONFIG_STATE)state == SESSION_RESUME) {
            std::unique_lock<std::mutex> guard(AudioExtension::reconfig_wait_mutex_);
            param_bt_a2dp.a2dp_suspended = false;
            param_bt_a2dp.is_suspend_setparam = false;
            param_bt_a2dp.dev_id = PAL_DEVICE_OUT_BLUETOOTH_BLE;

            ret = pal_set_param(PAL_PARAM_ID_BT_A2DP_SUSPENDED, (void *)&param_bt_a2dp,
                                sizeof(pal_param_bta2dp_t));
        } else if ((tRECONFIG_STATE)state == CHANNEL_MONO) {
            param_bt_a2dp.is_lc3_mono_mode_on = true;

            ret = pal_set_param(PAL_PARAM_ID_BT_A2DP_LC3_CONFIG, (void *)&param_bt_a2dp,
                                sizeof(pal_param_bta2dp_t));
        } else if ((tRECONFIG_STATE)state == CHANNEL_STEREO) {
            param_bt_a2dp.is_lc3_mono_mode_on = false;

            ret = pal_set_param(PAL_PARAM_ID_BT_A2DP_LC3_CONFIG, (void *)&param_bt_a2dp,
                                sizeof(pal_param_bta2dp_t));
        }
    } else if (session_type == LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
        if ((tRECONFIG_STATE)state == SESSION_SUSPEND) {
            std::unique_lock<std::mutex> guard(AudioExtension::reconfig_wait_mutex_);
            param_bt_a2dp.a2dp_capture_suspended = true;
            param_bt_a2dp.is_suspend_setparam = false;
            param_bt_a2dp.dev_id = PAL_DEVICE_IN_BLUETOOTH_BLE;

            ret = pal_set_param(PAL_PARAM_ID_BT_A2DP_CAPTURE_SUSPENDED, (void *)&param_bt_a2dp,
                                sizeof(pal_param_bta2dp_t));
        } else if ((tRECONFIG_STATE)state == SESSION_RESUME) {
            std::unique_lock<std::mutex> guard(AudioExtension::reconfig_wait_mutex_);
            param_bt_a2dp.a2dp_capture_suspended = false;
            param_bt_a2dp.is_suspend_setparam = false;
            param_bt_a2dp.dev_id = PAL_DEVICE_IN_BLUETOOTH_BLE;

            ret = pal_set_param(PAL_PARAM_ID_BT_A2DP_CAPTURE_SUSPENDED, (void *)&param_bt_a2dp,
                                sizeof(pal_param_bta2dp_t));
        }
    } else if (session_type == A2DP_HARDWARE_OFFLOAD_DATAPATH) {
        if ((tRECONFIG_STATE)state == SESSION_SUSPEND) {
            std::unique_lock<std::mutex> guard(AudioExtension::reconfig_wait_mutex_);
            param_bt_a2dp.a2dp_suspended = true;
            param_bt_a2dp.is_suspend_setparam = false;
            param_bt_a2dp.dev_id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;

            ret = pal_set_param(PAL_PARAM_ID_BT_A2DP_SUSPENDED, (void *)&param_bt_a2dp,
                                sizeof(pal_param_bta2dp_t));
        } else if ((tRECONFIG_STATE)state == SESSION_RESUME) {
            std::unique_lock<std::mutex> guard(AudioExtension::reconfig_wait_mutex_);
            param_bt_a2dp.a2dp_suspended = false;
            param_bt_a2dp.is_suspend_setparam = false;
            param_bt_a2dp.dev_id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;

            ret = pal_set_param(PAL_PARAM_ID_BT_A2DP_SUSPENDED, (void *)&param_bt_a2dp,
                                sizeof(pal_param_bta2dp_t));
        }
    }
    LOG(DEBUG) << __func__ << " reconfig_cb exit with state " << reconfigStateName.at(state).c_str()
               << " for " << deviceNameLUT.at(SessionTypePalDevMap.at(session_type)).c_str();
    return ret;
}

A2dpExtension::~A2dpExtension() {}
A2dpExtension::A2dpExtension()
    : AudioExtensionBase(kBluetoothIpcLibrary, isExtensionEnabled(kBluetoothProperty)) {
    LOG(INFO) << __func__ << " Enter";
    if (mHandle != nullptr) {
        if (!(a2dp_bt_audio_pre_init =
                      (a2dp_bt_audio_pre_init_t)dlsym(mHandle, "bt_audio_pre_init"))) {
            LOG(ERROR) << __func__ << " dlsym failed";
            goto feature_disabled;
        }

        if (mHandle && a2dp_bt_audio_pre_init) {
            LOG(VERBOSE) << __func__ << " calling BT module preinit";
            // fwk related check's will be done in the BT layer
            a2dp_bt_audio_pre_init();
        }

        if (!(register_reconfig_cb =
                      (register_reconfig_cb_t)dlsym(mHandle, "register_reconfig_cb"))) {
            LOG(ERROR) << __func__ << " dlsym failed for reconfig";
            goto feature_disabled;
        }

        if (mHandle && register_reconfig_cb) {
            LOG(VERBOSE) << __func__ << " calling BT module register reconfig";
            int (*reconfig_cb_ptr)(tSESSION_TYPE, int) = &reconfig_cb;
            register_reconfig_cb(reconfig_cb_ptr);
        }

        LOG(VERBOSE) << __func__ << "---- Feature A2DP offload is Enabled ---";
        return;
    }

feature_disabled:
    if (mHandle) {
        dlclose(mHandle);
        mHandle = NULL;
    }

    a2dp_bt_audio_pre_init = nullptr;
    LOG(VERBOSE) << __func__ << "---- Feature A2DP offload is disabled ---";
}

AudioDevice HfpExtension::audio_extn_hfp_get_matching_tx_device(const AudioDevice& rxDevice) {
    if (rxDevice.type.type == AudioDeviceType::OUT_SPEAKER_EARPIECE) {
        return AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE};
    } else if (rxDevice.type.type == AudioDeviceType::OUT_SPEAKER) {
        return AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE_BACK};
    } else if (rxDevice.type.type == AudioDeviceType::OUT_HEADSET &&
               rxDevice.type.connection == AudioDeviceDescription::CONNECTION_ANALOG) {
        return AudioDevice{.type.type = AudioDeviceType::IN_HEADSET,
                           .type.connection = AudioDeviceDescription::CONNECTION_ANALOG,
                           .address = rxDevice.address};
    } else if (rxDevice.type.type == AudioDeviceType::OUT_HEADPHONE &&
               rxDevice.type.connection == AudioDeviceDescription::CONNECTION_ANALOG) {
        return AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE};
    } else if ((rxDevice.type.type == AudioDeviceType::OUT_DEVICE ||
                rxDevice.type.type == AudioDeviceType::OUT_HEADSET) &&
               rxDevice.type.connection == AudioDeviceDescription::CONNECTION_BT_SCO) {
        return AudioDevice{.type.type = AudioDeviceType::IN_HEADSET,
                           .type.connection = AudioDeviceDescription::CONNECTION_BT_SCO};
    } else if (rxDevice.type.type == AudioDeviceType::OUT_HEADSET &&
               rxDevice.type.connection == AudioDeviceDescription::CONNECTION_BT_LE) {
        return AudioDevice{.type.type = AudioDeviceType::IN_HEADSET,
                           .type.connection = AudioDeviceDescription::CONNECTION_BT_LE};
    } else if ((rxDevice.type.type == AudioDeviceType::OUT_DEVICE ||
                rxDevice.type.type == AudioDeviceType::OUT_HEADSET) &&
               rxDevice.type.connection == AudioDeviceDescription::CONNECTION_USB) {
        if (mPlatform.getUSBCapEnable()) {
            return AudioDevice{.type.type = AudioDeviceType::IN_HEADSET,
                               .type.connection = AudioDeviceDescription::CONNECTION_USB,
                               .address = rxDevice.address};
        } else {
            return AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE};
        }
    } else if (rxDevice.type.type == AudioDeviceType::OUT_HEARING_AID) {
        return AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE};
    } else {
        LOG(ERROR) << __func__ << ": unable to find matching TX device for " << rxDevice.toString();
    }
    return {};
}

void HfpExtension::audio_extn_hfp_set_device(const std::vector<AudioDevice>& devices,
                                              const bool updateRx) {
    AudioDevice rxDevice;
    AudioDevice txDevice;
    if (devices.size() != 1) {
        LOG(ERROR) << __func__ << " invalid size / combo devices unsupported: " << devices;
        return;
    }

    LOG(DEBUG) << __func__ << (updateRx ? " Rx " : " Tx") << " devices : " << devices;
    if (updateRx) {
        rxDevice = devices[0];
        txDevice = audio_extn_hfp_get_matching_tx_device(rxDevice);
        if (hfp_set_device) {
            auto palDevices = mPlatform.convertToPalDevices({rxDevice, txDevice});
            hfp_set_device(reinterpret_cast<pal_device*>(palDevices.data()));
        }
    }
}

void HfpExtension::audio_extn_hfp_set_parameters(struct str_parms *params) {
    if (hfp_set_parameters) hfp_set_parameters(micMute, params);
}

int HfpExtension::audio_extn_hfp_set_mic_mute(bool state) {
    if (audio_extn_hfp_is_active()) {
        micMute = state;
        return ((hfp_set_mic_mute) ? hfp_set_mic_mute(state) : -1);
    }
    return -1;
}

bool HfpExtension::audio_extn_hfp_is_active() {
    return ((hfp_is_active) ? hfp_is_active() : false);
}

HfpExtension::~HfpExtension() {}
HfpExtension::HfpExtension() : AudioExtensionBase(kHfpLibrary, isExtensionEnabled(kHfpProperty)) {
    LOG(INFO) << __func__ << " Enter";
    if (mHandle != nullptr) {
        if (!(hfp_init = (hfp_init_t)dlsym(mHandle, "hfp_init")) ||
            !(hfp_is_active = (hfp_is_active_t)dlsym(mHandle, "hfp_is_active")) ||
            !(hfp_set_mic_mute = (hfp_set_mic_mute_t)dlsym(mHandle, "hfp_set_mic_mute")) ||
            !(hfp_set_mic_mute2 = (hfp_set_mic_mute2_t)dlsym(mHandle, "hfp_set_mic_mute2")) ||
            !(hfp_set_parameters = (hfp_set_parameters_t)dlsym(mHandle, "hfp_set_parameters")) ||
            !(hfp_set_device = (hfp_set_device_t)dlsym(mHandle, "hfp_set_device"))) {
            LOG(ERROR) << __func__ << " dlsym failed";
            goto feature_disabled;
        }
        LOG(DEBUG) << __func__ << "---- Feature HFP is Enabled ----";
        return;
    }

feature_disabled:
    if (mHandle) {
        dlclose(mHandle);
        mHandle = NULL;
    }

    hfp_init = NULL;
    hfp_is_active = NULL;
    hfp_get_usecase = NULL;
    hfp_set_mic_mute = NULL;
    hfp_set_mic_mute2 = NULL;
    hfp_set_parameters = NULL;
}

#ifdef SEC_AUDIO_CALL_SATELLITE
bool ExtModemCallExtension::isValidOutDevice(pal_device_id_t id) {
    switch (id) {
        case PAL_DEVICE_OUT_HANDSET:
        case PAL_DEVICE_OUT_SPEAKER:
        case PAL_DEVICE_OUT_WIRED_HEADSET:
        case PAL_DEVICE_OUT_WIRED_HEADPHONE:
        case PAL_DEVICE_OUT_BLUETOOTH_SCO:
        case PAL_DEVICE_OUT_USB_DEVICE:
        case PAL_DEVICE_OUT_USB_HEADSET:
            return true;
        default:
            return false;
    }
}

bool ExtModemCallExtension::isValidInDevice(pal_device_id_t id) {
    switch (id) {
        case PAL_DEVICE_IN_HANDSET_MIC:
        case PAL_DEVICE_IN_SPEAKER_MIC:
        case PAL_DEVICE_IN_WIRED_HEADSET:
        case PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET:
        case PAL_DEVICE_IN_USB_DEVICE:
        case PAL_DEVICE_IN_USB_HEADSET:
            return true;
        default:
            return false;
    }
}

bool ExtModemCallExtension::isUsbDevice(pal_device_id_t id) {
    switch (id) {
        case PAL_DEVICE_OUT_USB_DEVICE:
        case PAL_DEVICE_OUT_USB_HEADSET:
        case PAL_DEVICE_IN_USB_DEVICE:
        case PAL_DEVICE_IN_USB_HEADSET:
            return true;
        default:
            return false;
    }
}

void ExtModemCallExtension::setCustomKey(pal_device& palDevice, const pal_device_id_t outDeviceId) {
    int customKeyId = CUSTOM_KEY_EXT_MODEM;
    memset(palDevice.custom_config.custom_key, 0, PAL_MAX_CUSTOM_KEY_SIZE);

    if (isValidInDevice(palDevice.id)) {
        if ((palDevice.id == PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET)
                && !mPlatform.isBtNrecOn()) {
            customKeyId = CUSTOM_KEY_BT_HEADSET_NREC;
        } else if (palDevice.id == PAL_DEVICE_IN_HANDSET_MIC) {
            if (outDeviceId == PAL_DEVICE_OUT_WIRED_HEADPHONE) {
                customKeyId = CUSTOM_KEY_HEADPHONE_MIC;
            } else if ((outDeviceId == PAL_DEVICE_OUT_USB_HEADSET)
                    && !mPlatform.getUSBCapEnable()) {
                customKeyId = CUSTOM_KEY_USB_HEADPHONE_MIC;
            }
        }
    }
    strcpy(palDevice.custom_config.custom_key, ck_table[customKeyId]);
    LOG(INFO) << __func__ << " Setting custom key for call pal_devs : "
                          << palDevice.custom_config.custom_key;
}

std::unique_ptr<pal_stream_attributes> ExtModemCallExtension::getExtModemCallAttributes(
        pal_stream_loopback_type_t type) {
    auto attributes = std::make_unique<pal_stream_attributes>();
    struct pal_channel_info channelInfo;
    channelInfo.channels = 1;
    channelInfo.ch_map[0] = PAL_CHMAP_CHANNEL_FL;

    attributes->type = PAL_STREAM_LOOPBACK;
    attributes->info.opt_stream_info.loopback_type = type;
    attributes->direction = PAL_AUDIO_INPUT_OUTPUT;
    attributes->in_media_config.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
    attributes->in_media_config.ch_info = channelInfo;
    attributes->in_media_config.bit_width = CODEC_BACKEND_DEFAULT_BIT_WIDTH;
    attributes->in_media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;

    attributes->out_media_config.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
    attributes->out_media_config.bit_width = CODEC_BACKEND_DEFAULT_BIT_WIDTH;
    attributes->out_media_config.ch_info = channelInfo;
    attributes->out_media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
    return std::move(attributes);
}

void ExtModemCallExtension::configurePalDevices(struct pal_device *palDevices, const pal_device_id_t callRxDeviceId) {
    const int num_pal_devs = 2;
    struct pal_channel_info channelInfo;
    channelInfo.channels = 1;
    channelInfo.ch_map[0] = PAL_CHMAP_CHANNEL_FL;

    for (int i = 0; i < num_pal_devs; ++i) {
        if ((palDevices[i].id == PAL_DEVICE_OUT_EXT_MODEM ||
                palDevices[i].id == PAL_DEVICE_IN_EXT_MODEM_MIC)) {
            palDevices[i].config.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
            palDevices[i].config.bit_width = CODEC_BACKEND_DEFAULT_BIT_WIDTH;
            palDevices[i].config.ch_info = channelInfo;
            palDevices[i].config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
        } else {
            // call devices
            setCustomKey(palDevices[i], callRxDeviceId);
#ifdef SEC_AUDIO_SUPPORT_USB_OFFLOAD
            if (isUsbDevice(palDevices[i].id)) {
                // Configure USB Digital Headset parameters
                pal_param_device_capability_t *device_cap_query =
                        (pal_param_device_capability_t *)malloc(sizeof(pal_param_device_capability_t));
                if (!device_cap_query) {
                    LOG(ERROR) << __func__ << " Failed to allocate mem for device_cap_query";
                    return;
                }
                dynamic_media_config_t dynamic_media_config;
                size_t payload_size = 0;
                if (isValidOutDevice(palDevices[i].id) && isUsbDevice(palDevices[i].id)) {
                    device_cap_query->id = PAL_DEVICE_OUT_USB_DEVICE;
                    device_cap_query->is_playback = true;
                } else {
                    device_cap_query->id = PAL_DEVICE_IN_USB_DEVICE;
                    device_cap_query->is_playback = false;
                }
                // get usb details
                device_cap_query->addr.card_id = AudioExtensionBase::mUsbAddr.card_id;
                device_cap_query->addr.device_num = AudioExtensionBase::mUsbAddr.device_num;
                device_cap_query->config = &dynamic_media_config;
                pal_get_param(PAL_PARAM_ID_DEVICE_CAPABILITY, (void **)&device_cap_query, &payload_size,
                                nullptr);
                palDevices[i].address.card_id = AudioExtensionBase::mUsbAddr.card_id;
                palDevices[i].address.device_num = AudioExtensionBase::mUsbAddr.device_num;
                palDevices[i].config.sample_rate = dynamic_media_config.sample_rate[0];
                palDevices[i].config.ch_info = channelInfo;
                palDevices[i].config.aud_fmt_id = (pal_audio_fmt_t)dynamic_media_config.format[0];
                free(device_cap_query);
            }
#endif
        }
        
    }
}

int32_t ExtModemCallExtension::startCall(struct pal_device *callDevices) {
    int32_t ret = 0;
    uint32_t no_of_devices = 2;
    struct pal_device devices[2] = {};

    LOG(DEBUG) << __func__ << ": Enter";
    if (rxStreamHandle || txStreamHandle) {
        // external modem call already running;
        LOG(DEBUG) << __func__ << " ext modem call already running";
        return 0;
    }

    if (!isValidOutDevice(callDevices[0].id)) {
        LOG(ERROR) << __func__ << " invalid call device id: " << callDevices[0].id;
        return -EINVAL;
    }

    /* ExtModemCall Tx -> Call Rx */
    auto stream_attr = getExtModemCallAttributes(PAL_STREAM_LOOPBACK_EXT_MODEM_RX);
    devices[0].id = PAL_DEVICE_IN_EXT_MODEM_MIC;
    devices[1].id = callDevices[0].id;
    configurePalDevices(devices, callDevices[0].id);

    if (ret = ::pal_stream_open(stream_attr.get(), no_of_devices, devices, 0, NULL, NULL, 0,
                          &rxStreamHandle);
        ret) {
        LOG(ERROR) << __func__ << " rx stream (ExtModemCall Tx->Call Rx) open failed, rc " << ret;
        rxStreamHandle = nullptr;
        return ret;
    }

    if (ret = ::pal_stream_start(rxStreamHandle); ret) {
        LOG(ERROR) << __func__ << " rx stream (ExtModemCall Tx->Call Rx) start failed, rc " << ret;
        pal_stream_close(rxStreamHandle);
        rxStreamHandle = nullptr;
        return ret;
    }

    /* Call Tx -> ExtModemCall Rx */
    auto stream_tx_attr = getExtModemCallAttributes(PAL_STREAM_LOOPBACK_EXT_MODEM_TX);
    devices[0].id = PAL_DEVICE_OUT_EXT_MODEM;
    devices[1].id = callDevices[1].id;
    configurePalDevices(devices, callDevices[0].id);

    if (ret = ::pal_stream_open(stream_tx_attr.get(), no_of_devices, devices, 0, NULL, NULL, 0,
                          &txStreamHandle);
        ret) {
        LOG(ERROR) << __func__ << " tx stream (Call Tx->ExtModemCall Rx) open failed, rc " << ret;
        pal_stream_stop(rxStreamHandle);
        pal_stream_close(rxStreamHandle);
        rxStreamHandle = nullptr;
        return ret;
    }

    if (ret = ::pal_stream_start(txStreamHandle); ret) {
        LOG(ERROR) << __func__ << " tx stream (Call Tx->ExtModemCall Rx) start failed, rc " << ret;
        pal_stream_close(txStreamHandle);
        pal_stream_stop(rxStreamHandle);
        pal_stream_close(rxStreamHandle);
        rxStreamHandle = nullptr;
        txStreamHandle = nullptr;
        return ret;
    }

    isExtModemCallRunning = true;

    LOG(DEBUG) << __func__ << ": Exit";
    return ret;
}

void ExtModemCallExtension::stopCall() {
    LOG(DEBUG) << __func__ << ": Enter";
    isExtModemCallRunning = false;
    if (rxStreamHandle) {
        pal_stream_stop(rxStreamHandle);
        pal_stream_close(rxStreamHandle);
        rxStreamHandle = nullptr;
    }
    if (txStreamHandle) {
        pal_stream_stop(txStreamHandle);
        pal_stream_close(txStreamHandle);
        txStreamHandle = nullptr;
    }

    LOG(DEBUG) << __func__ << ": Exit";
}

void ExtModemCallExtension::setDevice(struct pal_device *devices) {
    int32_t rc = 0;

    if (isExtModemCallRunning && hasValidStreamHandle() &&
            isValidOutDevice(devices[0].id) && isValidInDevice(devices[1].id)) {
        setCustomKey(devices[0], devices[0].id);
        rc = pal_stream_set_device(rxStreamHandle, 1, &devices[0]);
        if (!rc) {
            setCustomKey(devices[1], devices[0].id);
            rc = pal_stream_set_device(txStreamHandle, 1, &devices[1]);
        }
    }

    if (rc) {
        LOG(ERROR) << __func__ << ": failed to set devices";
    }
    return;
}

ExtModemCallExtension::~ExtModemCallExtension() {}
ExtModemCallExtension::ExtModemCallExtension() : AudioExtensionBase(kDummyLibrary) {
    isExtModemCallRunning = false;
    rxStreamHandle = nullptr;
    txStreamHandle = nullptr;
}
#endif

FmExtension::~FmExtension() {}

bool FmExtension::audio_extn_fm_get_status() {
    if (fm_running_status) return fm_running_status;

    return false;
}

void FmExtension::audio_extn_fm_set_parameters(struct str_parms *params) {
#ifdef SEC_AUDIO_FMRADIO
    char value[32] = {0};
    int ret = str_parms_get_str(params, "handle_fm", value, sizeof(value));
    if (ret >= 0) {
        str_parms_add_int(params, "usb_card_id", AudioExtensionBase::mUsbAddr.card_id);
        str_parms_add_int(params, "usb_device_num", AudioExtensionBase::mUsbAddr.device_num);
    }
#endif
    if (fm_set_params) fm_set_params(params);
}
FmExtension::FmExtension() : AudioExtensionBase(kFmLibrary) {
    LOG(INFO) << __func__ << " Enter";
    if (mHandle != nullptr) {
        fm_set_params = (set_parameters_t)dlsym(mHandle, "fm_set_parameters");
        fm_running_status = (fm_running_status_t)dlsym(mHandle, "fm_get_running_status");
        if (!fm_set_params || !fm_running_status) {
            LOG(ERROR) << "error " << dlerror();
            dlclose(mHandle);
            fm_set_params = NULL;
            fm_running_status = NULL;
        }
    } else {
        fm_set_params = NULL;
        fm_running_status = NULL;
    }
}

int KarokeExtension::karaoke_open(pal_device_id_t device_out, pal_stream_callback pal_callback,
pal_channel_info ch_info) {
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
    if (karaoke_stream_handle != NULL) {
        if (int ret = karaoke_stop(); ret) {
            LOG(ERROR) << __func__ <<" Failed to stop karaoke stream, ret = " << ret;
            return 0;
        } else {
            if (int ret = karaoke_close(); ret) {
                LOG(ERROR) << __func__ << " Failed to close karaoke stream, ret = " << ret;
                return 0;
            } else {
                LOG(DEBUG) << __func__ << " Karaoke stream close success";
            }
        }
    }
#endif
    const int num_pal_devs = 2;
    struct pal_device pal_devs[num_pal_devs];
    karaoke_stream_handle = NULL;
    pal_device_id_t device_in;
    dynamic_media_config_t dynamic_media_config;
    size_t payload_size = 0;

    // Configuring Hostless Loopback
    if (device_out == PAL_DEVICE_OUT_WIRED_HEADSET)
        device_in = PAL_DEVICE_IN_WIRED_HEADSET;
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
    else if (device_out == PAL_DEVICE_OUT_WIRED_HEADPHONE) {
        device_in = PAL_DEVICE_IN_HANDSET_MIC;
    }
#endif
    else if (device_out == PAL_DEVICE_OUT_USB_HEADSET) {
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
        if (!mPlatform.getUSBCapEnable()) {
            device_in = PAL_DEVICE_IN_HANDSET_MIC;
        } else
#endif
        device_in = PAL_DEVICE_IN_USB_HEADSET;
        // get capability from device of USB
    } else
        return 0;

    sattr.type = PAL_STREAM_LOOPBACK;
    sattr.info.opt_stream_info.loopback_type = PAL_STREAM_LOOPBACK_KARAOKE;
    sattr.direction = PAL_AUDIO_INPUT_OUTPUT;
    sattr.in_media_config.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
    sattr.in_media_config.bit_width = CODEC_BACKEND_DEFAULT_BIT_WIDTH;
    sattr.in_media_config.ch_info = ch_info;
    sattr.in_media_config.aud_fmt_id = PAL_AUDIO_FMT_DEFAULT_PCM;
    sattr.out_media_config.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
    sattr.out_media_config.bit_width = CODEC_BACKEND_DEFAULT_BIT_WIDTH;
    sattr.out_media_config.ch_info = ch_info;
    sattr.out_media_config.aud_fmt_id = PAL_AUDIO_FMT_DEFAULT_PCM;
    for (int i = 0; i < num_pal_devs; ++i) {
        pal_devs[i].id = i ? device_in : device_out;
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
        memset(pal_devs[i].custom_config.custom_key, 0, PAL_MAX_CUSTOM_KEY_SIZE);
#endif
        if (device_out == PAL_DEVICE_OUT_USB_HEADSET || device_in == PAL_DEVICE_IN_USB_HEADSET) {
            // Configure USB Digital Headset parameters
            pal_param_device_capability_t *device_cap_query =
                    (pal_param_device_capability_t *)malloc(sizeof(pal_param_device_capability_t));
            if (!device_cap_query) {
                LOG(ERROR) << __func__ << " Failed to allocate mem for device_cap_query";
                return 0;
            }

            if (pal_devs[i].id == PAL_DEVICE_OUT_USB_HEADSET) {
                device_cap_query->id = PAL_DEVICE_OUT_USB_DEVICE;
                device_cap_query->is_playback = true;
            } else {
                device_cap_query->id = PAL_DEVICE_IN_USB_DEVICE;
                device_cap_query->is_playback = false;
            }
            // TODO: //get usb details
#ifdef SEC_AUDIO_SUPPORT_USB_OFFLOAD
            device_cap_query->addr.card_id = AudioExtensionBase::mUsbAddr.card_id;       // adevice->usb_card_id_;
            device_cap_query->addr.device_num = AudioExtensionBase::mUsbAddr.device_num; // adevice->usb_dev_num_;
#else
            device_cap_query->addr.card_id = 0;    // adevice->usb_card_id_;
            device_cap_query->addr.device_num = 0; // adevice->usb_dev_num_;
#endif
            device_cap_query->config = &dynamic_media_config;
            pal_get_param(PAL_PARAM_ID_DEVICE_CAPABILITY, (void **)&device_cap_query, &payload_size,
                          nullptr);
#ifdef SEC_AUDIO_SUPPORT_USB_OFFLOAD
            pal_devs[i].address.card_id = AudioExtensionBase::mUsbAddr.card_id;       // adevice->usb_card_id_;
            pal_devs[i].address.device_num = AudioExtensionBase::mUsbAddr.device_num; // adevice->usb_dev_num_;
#else
            pal_devs[i].address.card_id = 0;    // adevice->usb_card_id_;
            pal_devs[i].address.device_num = 0; // adevice->usb_dev_num_;
#endif
            pal_devs[i].config.sample_rate = dynamic_media_config.sample_rate[0];
            pal_devs[i].config.ch_info = ch_info;
            pal_devs[i].config.aud_fmt_id = (pal_audio_fmt_t)dynamic_media_config.format[0];
            free(device_cap_query);
        } else {
            pal_devs[i].config.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
            pal_devs[i].config.bit_width = CODEC_BACKEND_DEFAULT_BIT_WIDTH;
            pal_devs[i].config.ch_info = ch_info;
            pal_devs[i].config.aud_fmt_id = PAL_AUDIO_FMT_DEFAULT_PCM;
        }
    }
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
    //Configure custom config key for karaoke listenback
    strcpy(pal_devs[0].custom_config.custom_key, ck_table[CUSTOM_KEY_LISTENBACK]);
    LOG(INFO) << __func__ << " Setting custom key for rx pal_devs : " << pal_devs[0].custom_config.custom_key;

    if (isVoiceRecognitionStreamCreated()) {
        strcpy(pal_devs[1].custom_config.custom_key, ck_table[CUSTOM_KEY_VR]);
        LOG(INFO) << __func__ << " Setting custom key for tx pal_devs : " << pal_devs[1].custom_config.custom_key;
    }
#endif
    return pal_stream_open(&sattr, num_pal_devs, pal_devs, 0, NULL, pal_callback, (uint64_t)this,
                           &karaoke_stream_handle);
}

int KarokeExtension::karaoke_start() {
    return pal_stream_start(karaoke_stream_handle);
}

int KarokeExtension::karaoke_stop() {
    return pal_stream_stop(karaoke_stream_handle);
}

int KarokeExtension::karaoke_close() {
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
    int ret = pal_stream_close(karaoke_stream_handle);
    karaoke_stream_handle = NULL;
    return ret;
#else
    return pal_stream_close(karaoke_stream_handle);
#endif
}

#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
bool KarokeExtension::isKaraokeActive() {
    if (karaoke_stream_handle != NULL)
        return true;

    return false;
}

void KarokeExtension::init() {
    karaoke_stream_handle = NULL;
}
#endif

KarokeExtension::~KarokeExtension() {}
KarokeExtension::KarokeExtension() : AudioExtensionBase(kKarokeLibrary) {
    LOG(INFO) << __func__ << " Enter";
    if (mHandle != nullptr) {
    }
}

#ifdef SEC_AUDIO_SUPPORT_REMOTE_MIC
const std::unordered_set<pal_device_id_t> validAasDevices = {
    PAL_DEVICE_OUT_WIRED_HEADSET,
    PAL_DEVICE_OUT_WIRED_HEADPHONE,
    PAL_DEVICE_OUT_BLUETOOTH_SCO,
    PAL_DEVICE_OUT_USB_HEADSET
};

bool AasExtension::isAasDeviceAvailable(const pal_device_id_t deviceId) {
    return (validAasDevices.find(deviceId) != validAasDevices.end());
}

bool AasExtension::isValidStatusForAas(const pal_device_id_t outDeviceId) {
    if (mPlatform.getCallMode() == AUDIO_MODE_IN_CALL) {
        /* on - support only Normal Mode.  off - working all audio mode */
        LOG(ERROR) << __func__ << ": Not support AAS during call mode";
        return false;
    }
    if (!isAasDeviceAvailable(outDeviceId)
            || (!mPlatform.isBtScoOn() && (outDeviceId == PAL_DEVICE_OUT_BLUETOOTH_SCO))) {
        LOG(ERROR) << __func__ << ": Invalid device state " << outDeviceId;
        return false;
    }
    return true;
}

void AasExtension::setAasCustomKey(pal_device& palDevice, const pal_device_id_t outDeviceId) {
    int customKeyId = CUSTOM_KEY_INVALID;
    if (palDevice.id == outDeviceId) {
        customKeyId = CUSTOM_KEY_AAS;
    } else {
        // In device refers to out device ID for custom key.
        switch (outDeviceId) {
            case PAL_DEVICE_OUT_WIRED_HEADSET:
            case PAL_DEVICE_OUT_WIRED_HEADPHONE:
                customKeyId = CUSTOM_KEY_AAS_HEADSET;
                break;
            case PAL_DEVICE_OUT_BLUETOOTH_SCO:
                customKeyId = CUSTOM_KEY_AAS_BT_HEADSET;
                break;
            case PAL_DEVICE_OUT_USB_HEADSET:
                customKeyId = CUSTOM_KEY_AAS_USB_HEADSET;
                break;
            default:
                break;
        }
    }
    if (customKeyId != CUSTOM_KEY_INVALID) {
        strcpy(palDevice.custom_config.custom_key, ck_table[customKeyId]);
        LOG(INFO) << __func__ << " Setting custom key for "
                    << ((palDevice.id == outDeviceId) ? "out" : "in")
                    << " pal_devs : " << palDevice.custom_config.custom_key;
    }
}

int AasExtension::updateAasStream(const bool enable, const pal_device_id_t outDeviceId) {
    int ret = 0;
    LOG(DEBUG) << __func__ << " Enter : enable " << enable;

    if (enable) {
        if (!isValidStatusForAas(outDeviceId)) {
            return stopAasStream();
        }
        ret = startAasStream(outDeviceId);
        if (ret) {
            LOG(ERROR) << __func__ << ": Failed to start AAS stream ret = " << ret;
            stopAasStream();
        } else {
            LOG(DEBUG) << __func__ << ": Start AAS stream success";
        }
    }  else {
        // Disable
        ret = stopAasStream();
    }
    LOG(DEBUG) << __func__ << " Exit: ret " << ret;
    return ret;
}

int AasExtension::startAasStream(const pal_device_id_t outDeviceId)
{
    const int num_pal_devs = 2;
    struct pal_device pal_devs[num_pal_devs];
    pal_device_id_t inDeviceId;
    struct pal_stream_attributes aasStreamAttributes;
    pal_channel_info ch_info = {0, {0}};
    int ret = 0;

    if (isAasActive()) {
        if (outDeviceId != mAasOutDeviceId) {
            LOG(DEBUG) << __func__
                        << ": Re-open AAS stream due to device change from "
                        << mAasOutDeviceId << " to " << outDeviceId;
            stopAasStream(); // close the existing stream
        } else {
            LOG(DEBUG) << __func__
                        << ": skip to setAASMode enable for same device id " << outDeviceId;
            return ret;
        }
    }
    // Configuring Hostless Loopback
    if (isAasDeviceAvailable(outDeviceId)) {
        inDeviceId = PAL_DEVICE_IN_SPEAKER_MIC;
    } else {
        LOG(ERROR) << __func__ << ": Invalid device " << outDeviceId;
        return -EINVAL;
    }

    ch_info.channels = 2;
    ch_info.ch_map[0] = PAL_CHMAP_CHANNEL_FL;
    ch_info.ch_map[1] = PAL_CHMAP_CHANNEL_FR;

    memset(&aasStreamAttributes, 0, sizeof(aasStreamAttributes));
    aasStreamAttributes.type = PAL_STREAM_LOOPBACK;
    aasStreamAttributes.info.opt_stream_info.loopback_type = PAL_STREAM_LOOPBACK_KARAOKE;
    aasStreamAttributes.direction = PAL_AUDIO_INPUT_OUTPUT;
    aasStreamAttributes.in_media_config.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
    aasStreamAttributes.in_media_config.bit_width = CODEC_BACKEND_DEFAULT_BIT_WIDTH;
    aasStreamAttributes.in_media_config.ch_info = ch_info;
    aasStreamAttributes.in_media_config.aud_fmt_id = PAL_AUDIO_FMT_DEFAULT_PCM;
    aasStreamAttributes.out_media_config.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
    aasStreamAttributes.out_media_config.bit_width = CODEC_BACKEND_DEFAULT_BIT_WIDTH;
    aasStreamAttributes.out_media_config.ch_info = ch_info;
    aasStreamAttributes.out_media_config.aud_fmt_id = PAL_AUDIO_FMT_DEFAULT_PCM;

    for (int i = 0; i < num_pal_devs; ++i) {
        pal_devs[i].id = i ? inDeviceId : outDeviceId;
        memset(pal_devs[i].custom_config.custom_key, 0, PAL_MAX_CUSTOM_KEY_SIZE);
        if (outDeviceId == PAL_DEVICE_OUT_USB_HEADSET) {
            //Configure USB Digital Headset parameters
#ifdef SEC_AUDIO_SUPPORT_USB_OFFLOAD
            pal_param_device_capability_t *device_cap_query = (pal_param_device_capability_t *)
                                                       malloc(sizeof(pal_param_device_capability_t));
            if (!device_cap_query) {
                LOG(ERROR) << __func__ << ": Failed to allocate mem for device_cap_query";
                return -ENOMEM;
            }
            dynamic_media_config_t dynamic_media_config;
            size_t payload_size = 0;
            if (pal_devs[i].id == PAL_DEVICE_OUT_USB_HEADSET) {
                device_cap_query->id = PAL_DEVICE_OUT_USB_DEVICE;
                device_cap_query->is_playback = true;
            }
            device_cap_query->addr.card_id = AudioExtensionBase::mUsbAddr.card_id;
            device_cap_query->addr.device_num = AudioExtensionBase::mUsbAddr.device_num;
            device_cap_query->config = &dynamic_media_config;
            pal_get_param(PAL_PARAM_ID_DEVICE_CAPABILITY,
                                 (void **)&device_cap_query,
                                 &payload_size, nullptr);
            pal_devs[i].address.card_id = AudioExtensionBase::mUsbAddr.card_id;
            pal_devs[i].address.device_num = AudioExtensionBase::mUsbAddr.device_num;
            pal_devs[i].config.sample_rate = dynamic_media_config.sample_rate[0];
            pal_devs[i].config.ch_info = ch_info;
            pal_devs[i].config.aud_fmt_id = (pal_audio_fmt_t)dynamic_media_config.format[0];
            free(device_cap_query);
#endif
        } else {
            pal_devs[i].config.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
            pal_devs[i].config.bit_width = CODEC_BACKEND_DEFAULT_BIT_WIDTH;
            pal_devs[i].config.ch_info = ch_info;
            pal_devs[i].config.aud_fmt_id = PAL_AUDIO_FMT_DEFAULT_PCM;
        }

        setAasCustomKey(pal_devs[i], pal_devs[0].id);  // Set AAS custom key for the device
    }

    mAasOutDeviceId = outDeviceId;

    ret = pal_stream_open(&aasStreamAttributes,
            num_pal_devs, pal_devs,
            0,
            NULL,
            NULL, //callback
            (uint64_t) this,
            &(mAasStreamHandle));

    if (ret) {
        LOG(ERROR) << __func__ << ": Failed to open AAS stream ret = " << ret;
        mAasStreamHandle = nullptr;
        return ret;
    }

    LOG(DEBUG) << __func__ << ": AAS stream open success";

    return pal_stream_start(mAasStreamHandle);
}

int AasExtension::stopAasStream()
{
    if (!isAasActive()) {
        LOG(ERROR) << __func__ << ": AAS is not active state";
        return -EINVAL;
    }

    int ret = pal_stream_stop(mAasStreamHandle);
    if (ret)
        LOG(ERROR) << __func__ << ": Failed to stop AAS Stream ret = " << ret;

    ret = pal_stream_close(mAasStreamHandle);
    if (ret) {
        LOG(ERROR) << __func__ << ": Failed to close AAS stream ret = " << ret;
    } else {
        LOG(DEBUG) << __func__ << ": AAS stream close success";
    }
    mAasStreamHandle = nullptr;
    return ret;
}

AasExtension::~AasExtension() {}
AasExtension::AasExtension() : AudioExtensionBase(kDummyLibrary) {
    LOG(INFO) << __func__ << " Enter";
    mAasOutDeviceId = PAL_DEVICE_NONE;
    mAasStreamHandle = nullptr;
}
#endif

#ifdef SEC_AUDIO_DSM_AMP
// START: FEEDBACK =======================================================
void SpeakerFeedbackExtension::init()
{
#ifdef SEC_AUDIO_DUMP
    char property_value[PROPERTY_VALUE_MAX] = {0};
#endif
#ifdef SEC_AUDIO_VI_FEEDBACK
    // use vi feedback stream for vi sensing
    mSupportViFeedback = true;
#else
    mSupportViFeedback = false;
#ifdef SEC_AUDIO_DUMP
    if (property_get("vendor.audio.vifeedback.dump", property_value, NULL) > 0) {
        mSupportViFeedback = atoi(property_value);
    }
#endif // SEC_AUDIO_DUMP
#endif // SEC_AUDIO_VI_FEEDBACK
    mFeedbackStreamHandle = NULL;
}

void SpeakerFeedbackExtension::start()
{
    int ret = 0;

    if (!mSupportViFeedback) {
        LOG(DEBUG) << __func__ << " : SpeakerFeedbackExtension is not supported";
        return;
    }

    std::unique_lock<std::mutex> guard(mFeedbackMutex);

    if (mFeedbackStreamHandle) {
        LOG(DEBUG) << __func__ << " : SpeakerFeedbackExtension is already opened";
        return;
    }

    struct pal_stream_attributes feedbackStreamAttr;
    struct pal_device device;
    device.id = PAL_DEVICE_IN_VI_FEEDBACK;

    struct pal_channel_info out_ch_info = {2, {PAL_CHMAP_CHANNEL_FL, PAL_CHMAP_CHANNEL_FR}};

    feedbackStreamAttr.type = PAL_STREAM_LOOPBACK;
    feedbackStreamAttr.flags = (pal_stream_flags_t)0;
    feedbackStreamAttr.direction = PAL_AUDIO_INPUT_OUTPUT;
    feedbackStreamAttr.out_media_config.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
    feedbackStreamAttr.out_media_config.bit_width = CODEC_BACKEND_FEEDBACK_BIT_WIDTH;
    feedbackStreamAttr.out_media_config.aud_fmt_id = PAL_AUDIO_FMT_DEFAULT_PCM;
    feedbackStreamAttr.out_media_config.ch_info = out_ch_info;
    feedbackStreamAttr.info.opt_stream_info.loopback_type = PAL_STREAM_LOOPBACK_CAPTURE_ONLY;

    ret = pal_stream_open(&feedbackStreamAttr,
                            1/* Single device */,
                            &device,
                            0,
                            NULL,
                            nullptr,
                            (uint64_t) this,
                            &mFeedbackStreamHandle);
    if (ret) {
        LOG(ERROR) << __func__ << " : Feedback Stream Open Error(" << ret << ")";
        // Not fatal so ignoring the error.
        return;
    } else {
        LOG(DEBUG) << __func__ << " : SpeakerFeedbackExtension is opened";
    }

    ret = pal_stream_start(mFeedbackStreamHandle);
    if (ret) {
        LOG(ERROR) << __func__ << " : failed to start feedback stream(" << ret << ")";
        pal_stream_close(mFeedbackStreamHandle);
        mFeedbackStreamHandle = NULL;

    } else {
        LOG(DEBUG) << __func__ << " : SpeakerFeedbackExtension is started";
    }

    return;
}

int SpeakerFeedbackExtension::setDevice(pal_stream_handle_t *stream_handle,
                           uint32_t no_of_devices, struct pal_device *devices) {
    int ret = 0;
    bool hasSpeakerPALDevice = false;

    if (!mSupportViFeedback) {
        LOG(DEBUG) << __func__ << " : SpeakerFeedbackExtension is not supported";
        return ret;
    }

    if (devices[0].id == PAL_DEVICE_OUT_SPEAKER) {
        hasSpeakerPALDevice = true;
    }

    if (!mFeedbackStreamHandle &&
        hasSpeakerPALDevice &&
        (pal_active_device_count(PAL_DEVICE_OUT_SPEAKER) >= 0)) {
        // start feedback when routing spk -> other device
        // in case of playback && call
        // new device is speaker and current device count is 0 -> 1
        LOG(DEBUG) << __func__  << ": Open and start feedback stream for speaker";
        start();
    }

    if (int ret = ::pal_stream_set_device(stream_handle, no_of_devices, devices);
        ret) {
        LOG(ERROR) << __func__ << ": failed to set devices";
        return ret;
    }

    if(mFeedbackStreamHandle &&
        pal_active_device_count(PAL_DEVICE_OUT_SPEAKER) == 0) {
        // stop feedback when routing spk -> other device
        // in case of playback && call
        // and current spk count is 1 -> 0
        LOG(DEBUG) << __func__ << ": stop and close feedback stream for speaker";
        stop();
        close();
    }

    return ret;
}

void SpeakerFeedbackExtension::stop()
{
    int ret = 0;

    if (!mSupportViFeedback) {
        LOG(DEBUG) << __func__ << " : SpeakerFeedbackExtension is not supported";
        return;
    }

    std::unique_lock<std::mutex> guard(mFeedbackMutex);
    if (mFeedbackStreamHandle) {
        ret = pal_stream_stop(mFeedbackStreamHandle);
        if (ret) {
            LOG(ERROR) << __func__ << " : failed to stop feedback path";
        } else {
            LOG(DEBUG) << __func__ << " : SpeakerFeedbackExtension is stopped";
        }
    }
    return;
}

void SpeakerFeedbackExtension::close()
{
    int ret = 0;

    if (!mSupportViFeedback) {
        LOG(DEBUG) << __func__ << " : SpeakerFeedbackExtension is not supported";
        return;
    }

    std::unique_lock<std::mutex> guard(mFeedbackMutex);
    if (mFeedbackStreamHandle) {
        ret = pal_stream_close(mFeedbackStreamHandle);
        if (ret) {
            LOG(ERROR) << __func__ << " : failed to stop feedback path";
        } else {
            LOG(DEBUG) << __func__ << " : SpeakerFeedbackExtension is closed";
        }
        mFeedbackStreamHandle = NULL;
    }
    return;
}

SpeakerFeedbackExtension::~SpeakerFeedbackExtension() {}
SpeakerFeedbackExtension::SpeakerFeedbackExtension() : AudioExtensionBase(kDummyLibrary) {
    LOG(INFO) << __func__ << " Enter";
    mFeedbackStreamHandle = nullptr;
    mSupportViFeedback = false;
}
// END: FEEDBACK =======================================================
#endif

void GefExtension::gef_interface_init() {
    if (gef_init) gef_init();
}

void GefExtension::gef_interface_deinit() {
    if (gef_deinit) gef_deinit();
}

GefExtension::~GefExtension() {
    gef_interface_deinit();
}

GefExtension::GefExtension() : AudioExtensionBase(kGefLibrary, true) {
    LOG(INFO) << __func__ << " Enter";
    if (mHandle != nullptr) {
        if (!(gef_init = (gef_init_t)dlsym(mHandle, "gef_interface_init")) ||
            !(gef_deinit = (gef_deinit_t)dlsym(mHandle, "gef_interface_deinit"))) {
            LOG(ERROR) << __func__ << "dlsym failed";
            goto feature_disabled;
        }
        LOG(INFO) << __func__ << "----- GEF interface is initialized ----";
        gef_interface_init();
        return;
    }

feature_disabled:
    if (mHandle) {
        dlclose(mHandle);
        mHandle = NULL;
    }

    gef_init = NULL;
    gef_deinit = NULL;
}
} // namespace qti::audio::core
