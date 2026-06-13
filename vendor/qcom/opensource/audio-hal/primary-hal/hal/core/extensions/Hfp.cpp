/*
 * Copyright (c) 2012-2020, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#define LOG_TAG "AHAL_HFP_QTI"
#define LOG_NDDEBUG 0

#include <android-base/logging.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "PalApi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_PARAMETER_HFP_ENABLE "hfp_enable"
#define AUDIO_PARAMETER_HFP_SET_SAMPLING_RATE "hfp_set_sampling_rate"
#define AUDIO_PARAMETER_KEY_HFP_VOLUME "hfp_volume"
#define AUDIO_PARAMETER_HFP_PCM_DEV_ID "hfp_pcm_dev_id"

#define AUDIO_PARAMETER_KEY_HFP_MIC_VOLUME "hfp_mic_volume"

struct hfp_module {
    bool is_hfp_running;
    float hfp_volume;
    float mic_volume;
    bool mic_mute;
    uint32_t sample_rate;
    pal_stream_handle_t *rx_stream_handle;
    pal_stream_handle_t *tx_stream_handle;
};

#define PLAYBACK_VOLUME_MAX 0x2000
#define CAPTURE_VOLUME_DEFAULT (15.0)
static struct hfp_module hfpmod = {
        .is_hfp_running = 0,
        .hfp_volume = 0,
        .mic_volume = CAPTURE_VOLUME_DEFAULT,
        .mic_mute = 0,
        .sample_rate = 16000,
};

static int32_t hfp_set_volume(float value) {
    int32_t vol, ret = 0;
    struct pal_volume_data *pal_volume = NULL;

    LOG(VERBOSE) << __func__ << " entry";

    hfpmod.hfp_volume = value;

    if (!hfpmod.is_hfp_running) {
        LOG(VERBOSE) << __func__ << " HFP not active, ignoring set_hfp_volume call";
        return -EIO;
    }

    LOG(DEBUG) << __func__ << " Setting HFP volume to  " << value;

    pal_volume = (struct pal_volume_data *)malloc(sizeof(struct pal_volume_data) +
                                                  sizeof(struct pal_channel_vol_kv));

    if (!pal_volume) return -ENOMEM;

    pal_volume->no_of_volpair = 1;
    pal_volume->volume_pair[0].channel_mask = 0x03;
    pal_volume->volume_pair[0].vol = value;
    ret = pal_stream_set_volume(hfpmod.rx_stream_handle, pal_volume);
    if (ret) LOG(ERROR) << __func__ << " set volume failed:  " << ret;

    free(pal_volume);
    LOG(VERBOSE) << __func__ << " exit";
    return ret;
}

/*Set mic volume to value.
 *
 * This interface is used for mic volume control, set mic volume as value(range 0 ~ 15).
 *
*/
static int hfp_set_mic_volume(float value) {
    int volume, ret = 0;
    struct pal_volume_data *pal_volume = NULL;

    LOG(DEBUG) << __func__ << " enter value= " << value;

    if (!hfpmod.is_hfp_running) {
        LOG(ERROR) << __func__ << " HFP not active, ignoring set_hfp_mic_volume call";
        return -EIO;
    }

    if (value < 0.0) {
        LOG(DEBUG) << __func__ << " " << value << " Under 0.0, assuming 0.0";
        value = 0.0;
    } else if (value > CAPTURE_VOLUME_DEFAULT) {
        value = CAPTURE_VOLUME_DEFAULT;
        LOG(DEBUG) << __func__ << " Volume brought within range " << value;
    }

    value = value / CAPTURE_VOLUME_DEFAULT;

    volume = (int)(value * PLAYBACK_VOLUME_MAX);

    pal_volume = (struct pal_volume_data *)malloc(sizeof(struct pal_volume_data) +
                                                  sizeof(struct pal_channel_vol_kv));
    if (!pal_volume) {
        LOG(ERROR) << __func__ << " Failed to allocate memory for pal_volume";
        return -ENOMEM;
    }
    pal_volume->no_of_volpair = 1;
    pal_volume->volume_pair[0].channel_mask = 0x03;
    pal_volume->volume_pair[0].vol = value;
    if (pal_stream_set_volume(hfpmod.tx_stream_handle, pal_volume) < 0) {
        LOG(ERROR) << __func__ << " Couldn't set HFP Volume " << volume;
        free(pal_volume);
        pal_volume = NULL;
        return -EINVAL;
    }

    free(pal_volume);
    pal_volume = NULL;

    return ret;
}

static float hfp_get_mic_volume(void) {
    return hfpmod.mic_volume;
}

static int32_t start_hfp(struct str_parms *parms __unused) {
    int32_t ret = 0;
    uint32_t no_of_devices = 2;
    struct pal_stream_attributes stream_attr = {};
    struct pal_stream_attributes stream_tx_attr = {};
    struct pal_device devices[2] = {};
    struct pal_channel_info ch_info;

    LOG(DEBUG) << __func__ << " HFP start enter";
    if (hfpmod.rx_stream_handle || hfpmod.tx_stream_handle) return 0; // hfp already running;

    pal_param_device_connection_t param_device_connection;

    param_device_connection.id = PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
    param_device_connection.connection_state = true;
    ret = pal_set_param(PAL_PARAM_ID_DEVICE_CONNECTION, (void *)&param_device_connection,
                        sizeof(pal_param_device_connection_t));
    if (ret != 0) {
        LOG(ERROR) << __func__ << " Set PAL_PARAM_ID_DEVICE_CONNECTION for  "
                   << param_device_connection.id << " failed";
        return ret;
    }

    param_device_connection.id = PAL_DEVICE_OUT_BLUETOOTH_SCO;
    param_device_connection.connection_state = true;
    ret = pal_set_param(PAL_PARAM_ID_DEVICE_CONNECTION, (void *)&param_device_connection,
                        sizeof(pal_param_device_connection_t));
    if (ret != 0) {
        LOG(ERROR) << __func__ << " Set PAL_PARAM_ID_DEVICE_CONNECTION for  "
                   << param_device_connection.id << " failed";
        return ret;
    }

    pal_param_btsco_t param_btsco;

    param_btsco.is_bt_hfp = true;
    param_btsco.bt_sco_on = true;
    ret = pal_set_param(PAL_PARAM_ID_BT_SCO, (void *)&param_btsco, sizeof(pal_param_btsco_t));
    if (ret != 0) {
        LOG(ERROR) << __func__ << " Set PAL_PARAM_ID_BT_SCO failed";
        return ret;
    }

    if (hfpmod.sample_rate == 16000) {
        param_btsco.bt_wb_speech_enabled = true;
    } else {
        param_btsco.bt_wb_speech_enabled = false;
    }

    ret = pal_set_param(PAL_PARAM_ID_BT_SCO_WB, (void *)&param_btsco, sizeof(pal_param_btsco_t));
    if (ret != 0) {
        LOG(ERROR) << __func__ << " Set PAL_PARAM_ID_BT_SCO_WB failed";
        return ret;
    }

    ch_info.channels = 1;
    ch_info.ch_map[0] = PAL_CHMAP_CHANNEL_FL;

    /* BT SCO -> Spkr */
    stream_attr.type = PAL_STREAM_LOOPBACK;
    stream_attr.info.opt_stream_info.loopback_type = PAL_STREAM_LOOPBACK_HFP_RX;
    stream_attr.direction = PAL_AUDIO_INPUT_OUTPUT;
    stream_attr.in_media_config.sample_rate = hfpmod.sample_rate;
    stream_attr.in_media_config.bit_width = 16;
    stream_attr.in_media_config.ch_info = ch_info;
    stream_attr.in_media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;

    stream_attr.out_media_config.sample_rate = 48000;
    stream_attr.out_media_config.bit_width = 16;
    stream_attr.out_media_config.ch_info = ch_info;
    stream_attr.out_media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;

    devices[0].id = PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
    devices[0].config.sample_rate = hfpmod.sample_rate;
    devices[0].config.bit_width = 16;
    devices[0].config.ch_info = ch_info;
    devices[0].config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;

    devices[1].id = PAL_DEVICE_OUT_SPEAKER;

    ret = pal_stream_open(&stream_attr, no_of_devices, devices, 0, NULL, NULL, 0,
                          &hfpmod.rx_stream_handle);
    if (ret != 0) {
        LOG(ERROR) << __func__ << " HFP rx stream (BT SCO->Spkr) open failed, rc " << ret;
        return ret;
    }
    ret = pal_stream_start(hfpmod.rx_stream_handle);
    if (ret != 0) {
        LOG(ERROR) << __func__ << " HFP rx stream (BT SCO->Spkr) open failed, rc " << ret;
        pal_stream_close(hfpmod.rx_stream_handle);
        return ret;
    }

    /* Mic -> BT SCO */
    stream_tx_attr.type = PAL_STREAM_LOOPBACK;
    stream_tx_attr.info.opt_stream_info.loopback_type = PAL_STREAM_LOOPBACK_HFP_TX;
    stream_tx_attr.direction = PAL_AUDIO_INPUT_OUTPUT;
    stream_tx_attr.in_media_config.sample_rate = hfpmod.sample_rate;
    stream_tx_attr.in_media_config.bit_width = 16;
    stream_tx_attr.in_media_config.ch_info = ch_info;
    stream_tx_attr.in_media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;

    stream_tx_attr.out_media_config.sample_rate = 48000;
    stream_tx_attr.out_media_config.bit_width = 16;
    stream_tx_attr.out_media_config.ch_info = ch_info;
    stream_tx_attr.out_media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;

    devices[0].id = PAL_DEVICE_OUT_BLUETOOTH_SCO;
    devices[0].config.sample_rate = hfpmod.sample_rate;
    devices[0].config.bit_width = 16;
    devices[0].config.ch_info = ch_info;
    devices[0].config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;

    devices[1].id = PAL_DEVICE_IN_SPEAKER_MIC;

    ret = pal_stream_open(&stream_tx_attr, no_of_devices, devices, 0, NULL, NULL, 0,
                          &hfpmod.tx_stream_handle);
    if (ret != 0) {
        LOG(ERROR) << __func__ << " HFP tx stream (Mic->BT SCO) open failed, rc " << ret;
        pal_stream_stop(hfpmod.rx_stream_handle);
        pal_stream_close(hfpmod.rx_stream_handle);
        hfpmod.rx_stream_handle = NULL;
        return ret;
    }
    ret = pal_stream_start(hfpmod.tx_stream_handle);
    if (ret != 0) {
        LOG(ERROR) << __func__ << " HFP tx stream (Mic->BT SCO) open failed, rc " << ret;
        pal_stream_close(hfpmod.tx_stream_handle);
        pal_stream_stop(hfpmod.rx_stream_handle);
        pal_stream_close(hfpmod.rx_stream_handle);
        hfpmod.rx_stream_handle = NULL;
        hfpmod.tx_stream_handle = NULL;
        return ret;
    }
    hfpmod.mic_mute = false;
    hfpmod.is_hfp_running = true;
    hfp_set_volume(hfpmod.hfp_volume);

    LOG(DEBUG) << __func__ << " HFP start end";
    return ret;
}

static int32_t stop_hfp() {
    int32_t ret = 0;

    LOG(DEBUG) << __func__ << " HFP stop enter";
    hfpmod.is_hfp_running = false;
    if (hfpmod.rx_stream_handle) {
        pal_stream_stop(hfpmod.rx_stream_handle);
        pal_stream_close(hfpmod.rx_stream_handle);
        hfpmod.rx_stream_handle = NULL;
    }
    if (hfpmod.tx_stream_handle) {
        pal_stream_stop(hfpmod.tx_stream_handle);
        pal_stream_close(hfpmod.tx_stream_handle);
        hfpmod.tx_stream_handle = NULL;
    }

    pal_param_btsco_t param_btsco;

    param_btsco.is_bt_hfp = true;
    param_btsco.bt_sco_on = true;
    ret = pal_set_param(PAL_PARAM_ID_BT_SCO, (void *)&param_btsco, sizeof(pal_param_btsco_t));
    if (ret != 0) {
        LOG(DEBUG) << __func__ << " Set PAL_PARAM_ID_BT_SCO failed";
    }

    pal_param_device_connection_t param_device_connection;

    param_device_connection.id = PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
    param_device_connection.connection_state = false;
    ret = pal_set_param(PAL_PARAM_ID_DEVICE_CONNECTION, (void *)&param_device_connection,
                        sizeof(pal_param_device_connection_t));
    if (ret != 0) {
        LOG(ERROR) << __func__ << " Set PAL_PARAM_ID_DEVICE_DISCONNECTION for  "
                   << param_device_connection.id << " failed";
    }

    param_device_connection.id = PAL_DEVICE_OUT_BLUETOOTH_SCO;
    param_device_connection.connection_state = false;
    ret = pal_set_param(PAL_PARAM_ID_DEVICE_CONNECTION, (void *)&param_device_connection,
                        sizeof(pal_param_device_connection_t));
    if (ret != 0) {
        LOG(ERROR) << __func__ << " Set PAL_PARAM_ID_DEVICE_DISCONNECTION for  "
                   << param_device_connection.id << " failed";
    }

    LOG(DEBUG) << __func__ << "HFP stop end";
    return ret;
}

void hfp_init() {
    return;
}

bool hfp_is_active() {
    return hfpmod.is_hfp_running;
}

bool is_valid_out_device(pal_device_id_t id) {
    switch (id) {
        case PAL_DEVICE_OUT_HANDSET:
        case PAL_DEVICE_OUT_SPEAKER:
        case PAL_DEVICE_OUT_WIRED_HEADSET:
        case PAL_DEVICE_OUT_WIRED_HEADPHONE:
        case PAL_DEVICE_OUT_USB_DEVICE:
        case PAL_DEVICE_OUT_USB_HEADSET:
            return true;
        default:
            return false;
    }
}

bool is_valid_in_device(pal_device_id_t id) {
    switch (id) {
        case PAL_DEVICE_IN_HANDSET_MIC:
        case PAL_DEVICE_IN_SPEAKER_MIC:
        case PAL_DEVICE_IN_WIRED_HEADSET:
        case PAL_DEVICE_IN_USB_DEVICE:
        case PAL_DEVICE_IN_USB_HEADSET:
            return true;
        default:
            return false;
    }
}
  
bool has_valid_stream_handle() {
    return (hfpmod.rx_stream_handle && hfpmod.tx_stream_handle);
}

void hfp_set_device(struct pal_device *devices) {
    int rc = 0;

    if (hfpmod.is_hfp_running && has_valid_stream_handle() &&
        is_valid_out_device(devices[0].id) && is_valid_in_device(devices[1].id)) {
        rc = pal_stream_set_device(hfpmod.rx_stream_handle, 1, &devices[0]);
        if (!rc) {
            rc = pal_stream_set_device(hfpmod.tx_stream_handle, 1, &devices[1]);
        }
    }

    if (rc) {
        LOG(ERROR) << __func__ << ": failed to set devices for hfp";
    }
    return;
}

/*Set mic mute state.
 * *
 * * This interface is used for mic mute state control
 * */
int hfp_set_mic_mute(bool state) {
    int rc = 0;

    if (state == hfpmod.mic_mute) {
        LOG(DEBUG) << __func__ << " mic mute already " << state;
        return rc;
    }
    rc = hfp_set_mic_volume((state == true) ? 0.0 : hfpmod.mic_volume);
    if (rc == 0) hfpmod.mic_mute = state;
    LOG(DEBUG) << __func__ << " Setting mute state  " << state << " rc " << rc;
    return rc;
}

int hfp_set_mic_mute2(bool state __unused) {
    LOG(DEBUG) << __func__ << " Unsupported";
    return 0;
}

void hfp_set_parameters(bool adev_mute, struct str_parms *parms) {
    int status = 0;
    char value[32] = {0};
    float vol;
    int val;
    int rate;

    LOG(DEBUG) << __func__ << " enter";

    status = str_parms_get_str(parms, AUDIO_PARAMETER_HFP_ENABLE, value, sizeof(value));
    if (status >= 0) {
        if (!strncmp(value, "true", sizeof(value)) && !hfpmod.is_hfp_running) {
            status = start_hfp(parms);
            /*
             * Sync to adev mic mute state if hfpmod.mic_mute state is lost due
             * to HFP session tear down during device switch on companion device.
             */
            if (hfpmod.mic_mute != adev_mute) {
                LOG(DEBUG) << __func__ << " update mic mute with latest mute state " << adev_mute;
                hfp_set_mic_mute(adev_mute);
            }
        } else if (!strncmp(value, "false", sizeof(value)) && hfpmod.is_hfp_running) {
            stop_hfp();
        } else {
            LOG(ERROR) << __func__ << " hfp_enable " << value << " is unsupported";
        }
    }

    memset(value, 0, sizeof(value));
    status = str_parms_get_str(parms, AUDIO_PARAMETER_HFP_SET_SAMPLING_RATE, value, sizeof(value));
    if (status >= 0) {
        rate = atoi(value);
        if (rate == 8000) {
            hfpmod.sample_rate = (uint32_t)rate;
        } else if (rate == 16000) {
            hfpmod.sample_rate = (uint32_t)rate;
        } else
            LOG(ERROR) << __func__ << " Unsupported rate.. " << rate;
    }

    memset(value, 0, sizeof(value));
    status = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_HFP_VOLUME, value, sizeof(value));
    if (status >= 0) {
        if (sscanf(value, "%f", &vol) != 1) {
            LOG(ERROR) << __func__ << " error in retrieving hfp volume";
            status = -EIO;
            goto exit;
        }
        LOG(DEBUG) << __func__ << " set_hfp_volume usecase, Vol: " << vol;
        hfp_set_volume(vol);
    }

    memset(value, 0, sizeof(value));
    status = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_HFP_MIC_VOLUME, value, sizeof(value));
    if (status >= 0) {
        if (sscanf(value, "%f", &vol) != 1) {
            LOG(ERROR) << __func__ << " error in retrieving hfp mic volume";
            status = -EIO;
            goto exit;
        }
        LOG(DEBUG) << __func__ << " set_hfp_mic_volume usecase, Vol: " << vol;
        if (hfp_set_mic_volume(vol) == 0) hfpmod.mic_volume = vol;
    }

exit:
    LOG(DEBUG) << __func__ << " Exit";
}

#ifdef __cplusplus
}
#endif
