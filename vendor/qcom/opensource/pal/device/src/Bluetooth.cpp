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

#define LOG_TAG "PAL: Bluetooth"
#include "Bluetooth.h"
#include "ResourceManager.h"
#include "PayloadBuilder.h"
#include "Stream.h"
#include "Session.h"
#include "SessionAlsaUtils.h"
#include "Device.h"
#include "kvh2xml.h"
#include <dlfcn.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <sstream>
#include <string>
#include <regex>
#include <hardware/audio.h> // SS_A2DP_FEATURE
#ifdef SEC_AUDIO_MUTE_DETECTION
#include <audioInfo.h>
#endif

#define PARAM_ID_RESET_PLACEHOLDER_MODULE 0x08001173
#define BT_IPC_SOURCE_LIB                 "btaudio_offload_if.so"
#define BT_IPC_SINK_LIB                   "libbthost_if_sink.so"
#define MIXER_SET_FEEDBACK_CHANNEL        "BT set feedback channel"
#define MIXER_SET_CODEC_TYPE              "BT codec type"
#define BT_SLIMBUS_CLK_STR                "BT SLIMBUS CLK SRC"

#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
#define ENC_NONE                                     0
#define ENC_SBC                                      1
#define ENC_APTX                                     2
#define ENC_SSC                                      3
#define ENC_SS_SBC                                   4

#ifdef QCA_OFFLOAD
#define SSC_BITRATE_GAME_MODE_ON 4
#define SSC_BITRATE_GAME_MODE_OFF 5
#endif

#define MAX_OUT_LATENCY 2000 //ms
#define ADJUST_LATENCY 190 //The larger the ADJUST_LATENCY(ms), the faster the sound

int peerMtu = 0;

static void peerMtu_cb(uint32_t mtu)
{
    PAL_INFO(LOG_TAG, "peerMtu = %d", mtu);
    peerMtu = mtu;
}

static uint32_t calculate_latency(uint32_t latency, uint32_t delay_report)
{
    if (delay_report < MAX_OUT_LATENCY) {
        if (delay_report > latency) {
            latency = delay_report;
        }
    } else if (latency < MAX_OUT_LATENCY) {
        latency = MAX_OUT_LATENCY;
    }

    PAL_INFO(LOG_TAG, "latency = %d", latency);
    return latency;
}

void bitrate_cb(uint32_t bitrate) {
    int status = 0;
    std::string backEndName;
    std::shared_ptr<Device> dev = nullptr;
    struct pal_device curDevAttr;
    Stream *stream = NULL;
    std::vector<Stream*> activestreams;
    std::shared_ptr<ResourceManager> rmLocal = ResourceManager::getInstance();
    Session *session = NULL;
    std::shared_ptr<BtA2dp> a2dpDev = nullptr;
    enum A2DP_STATE a2dpState;
    codec_format_t currCodecFormat;
    pal_param_bta2dp_dynbitrate_t dynbitrate;

    PAL_INFO(LOG_TAG, "Requested bitrate = %d", bitrate);

    memset(&curDevAttr, 0, sizeof(curDevAttr));
    curDevAttr.id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
    dev = Device::getInstance(&curDevAttr, rmLocal);
    status = rmLocal->getActiveStream_l(activestreams, dev);
    if ((0 != status) || (activestreams.size() == 0)) {
        PAL_ERR(LOG_TAG, "no active stream available");
        return;
    }
    stream = static_cast<Stream *>(activestreams[0]);
    stream->getAssociatedSession(&session);

    a2dpDev = std::dynamic_pointer_cast<BtA2dp>(dev);
    a2dpState = a2dpDev->getA2dpState();
    currCodecFormat = a2dpDev->getCodecFormat();
    PAL_INFO(LOG_TAG, "a2dpState = %d, currCodecFormat = %d", a2dpState, currCodecFormat);

#ifdef QCA_OFFLOAD
    if (bitrate == SSC_BITRATE_GAME_MODE_ON) a2dpDev->setGameMode(true);
    else if (bitrate == SSC_BITRATE_GAME_MODE_OFF) a2dpDev->setGameMode(false);
#endif

    if ((a2dpState == A2DP_STATE_STARTED) && (CODEC_TYPE_SBC == currCodecFormat || CODEC_TYPE_SSC == currCodecFormat))
    {
        dynbitrate.bitrate = bitrate;
        session->setParameters(stream, BT_PLACEHOLDER_ENCODER, PAL_PARAM_ID_BT_A2DP_DYN_BITRATE, &dynbitrate);
    }
}

enum A2DP_STATE BtA2dp::getA2dpState() {
    return a2dpState;
}

codec_format_t BtA2dp::getCodecFormat() {
    return codecFormat;
}

bool BtA2dp::is_a2dp_offload()
{
    if (get_session_type() == A2DP_HARDWARE_OFFLOAD_DATAPATH) {
        return true;
    }
    return false;
}

#ifdef QCA_OFFLOAD
void BtA2dp::setGameMode(bool gameMode) {
    game_mode = gameMode;
}
#else
void Bluetooth::set_a2dp_suspend(int a2dp_suspend)
{
    int status = 0;
    Stream *stream = NULL;
    Session *session = NULL;
    std::vector<Stream*> activestreams;
    std::string backEndName;
    std::shared_ptr<Device> dev = nullptr;
    pal_param_ss_pack_a2dp_suspend_t param_a2dp_suspend;

    rm->getBackendName(deviceAttr.id, backEndName);
    dev = Device::getInstance(&deviceAttr, rm);
    status = rm->getActiveStream_l(activestreams, dev);
    if ((0 != status) || (activestreams.size() == 0)) {
        PAL_ERR(LOG_TAG, "no active stream available");
        return;
    }
    stream = static_cast<Stream *>(activestreams[0]);
    stream->getAssociatedSession(&session);

    PAL_INFO(LOG_TAG, "set_a2dp_suspend %d", a2dp_suspend);

    param_a2dp_suspend.a2dp_suspend = a2dp_suspend;
    session->setParameters(stream, SS_COP_PACKETIZER, PAL_PARAM_ID_BT_A2DP_SUSPENDED, &param_a2dp_suspend);
}
#endif
#endif

Bluetooth::Bluetooth(struct pal_device *device, std::shared_ptr<ResourceManager> Rm)
    : Device(device, Rm),
      codecFormat(CODEC_TYPE_INVALID),
      codecInfo(NULL),
      isAbrEnabled(false),
      isConfigured(false),
      isLC3MonoModeOn(false),
      isTwsMonoModeOn(false),
      isScramblingEnabled(false),
      isDummySink(false),
      abrRefCnt(0),
      totalActiveSessionRequests(0)
{
}

Bluetooth::~Bluetooth()
{
}

int Bluetooth::updateDeviceMetadata()
{
    int ret = 0;
    std::string backEndName;
    std::vector <std::pair<int, int>> keyVector;
    struct mixer_ctl *ctrl = NULL;

    if (ResourceManager::isCPEnabled) {
        ctrl = mixer_get_ctl_by_name(hwMixerHandle,
                                     MIXER_SET_CODEC_TYPE);
        if (!ctrl) {
            PAL_ERR(LOG_TAG, "ERROR %s mixer control not identified",
                    MIXER_SET_CODEC_TYPE);
            return ret;
        }

        ret = mixer_ctl_set_enum_by_string(ctrl, btCodecFormatLUT.at(codecFormat).c_str());
        if (ret) {
            PAL_ERR(LOG_TAG, "Mixer control %s set with %s failed: %d",
                    MIXER_SET_CODEC_TYPE, btCodecFormatLUT.at(codecFormat).c_str(), ret);
            return ret;
        }
    }

    ret = PayloadBuilder::getBtDeviceKV(deviceAttr.id, keyVector, codecFormat,
        isAbrEnabled, false);
    if (ret)
        PAL_ERR(LOG_TAG, "No KVs found for device id %d codec format:0x%x",
            deviceAttr.id, codecFormat);

    rm->getBackendName(deviceAttr.id, backEndName);
    ret = SessionAlsaUtils::setDeviceMetadata(rm, backEndName, keyVector);
    return ret;
}

void Bluetooth::updateDeviceAttributes()
{
    deviceAttr.config.sample_rate = codecConfig.sample_rate;

    /* Sample rate calculation is done by kernel proxy driver in
     * case of XPAN. Send Encoder sample rate itself as part of
     * device attributes.
     *
     * For SCO devices, update proper sample rate. If there is
     * incoming stream over SCO, it will fetch proper device
     * attributes due to call to updateSampleRate. This will
     * cause unnecessary device switch if current device attributes
     * are not updated properly. Also device sample rate for Voice
     * usecase with APTX_AD_SPEECH and LC3_VOICE is hardcoded, so
     * it won't cause any issues.
     */
    if (ResourceManager::isCPEnabled && !rm->isBtScoDevice(deviceAttr.id))
        return;

    switch (codecFormat) {
    case CODEC_TYPE_AAC:
    case CODEC_TYPE_SBC:
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    case CODEC_TYPE_SSC:
#endif
        if (codecType == DEC &&
            (codecConfig.sample_rate == 44100 ||
             codecConfig.sample_rate == 48000))
            deviceAttr.config.sample_rate = codecConfig.sample_rate * 2;
        break;
    case CODEC_TYPE_LDAC:
    case CODEC_TYPE_APTX_AD:
        if (codecType == ENC &&
            (codecConfig.sample_rate == 44100 ||
             codecConfig.sample_rate == 48000))
        deviceAttr.config.sample_rate = codecConfig.sample_rate * 2;
        break;
    case CODEC_TYPE_APTX_AD_SPEECH:
    case CODEC_TYPE_LC3:
        deviceAttr.config.sample_rate = SAMPLINGRATE_96K;
        deviceAttr.config.aud_fmt_id = PAL_AUDIO_FMT_DEFAULT_COMPRESSED;
        break;
// SS_BT_HFP - H_127 : RVP
#ifdef SEC_AUDIO_BLUETOOTH
    case CODEC_TYPE_RVP:
        deviceAttr.config.sample_rate = SAMPLINGRATE_96K;
        deviceAttr.config.aud_fmt_id = PAL_AUDIO_FMT_DEFAULT_COMPRESSED;
        break;
#endif
// SS_BT_HFP - H_127 end
    case CODEC_TYPE_APTX_AD_QLEA:
        if (codecVersion == V1)
            deviceAttr.config.sample_rate = SAMPLINGRATE_96K;
        else
            deviceAttr.config.sample_rate = SAMPLINGRATE_192K;
        deviceAttr.config.aud_fmt_id = PAL_AUDIO_FMT_DEFAULT_COMPRESSED;
        break;
    default:
        break;
    }
}

bool Bluetooth::isPlaceholderEncoder()
{
    switch (codecFormat) {
        case CODEC_TYPE_LDAC:
        case CODEC_TYPE_APTX_AD:
        case CODEC_TYPE_APTX_AD_SPEECH:
// SS_BT_HFP - H_127 : RVP
#ifdef SEC_AUDIO_BLUETOOTH
        case CODEC_TYPE_RVP:
#endif
// SS_BT_HFP - H_127 end
        case CODEC_TYPE_LC3:
        case CODEC_TYPE_APTX_AD_QLEA:
        case CODEC_TYPE_APTX_AD_R4:
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
        case CODEC_TYPE_SBC:
        case CODEC_TYPE_SSC:
#endif
            return false;
        case CODEC_TYPE_AAC:
            return isAbrEnabled ? false : true;
        default:
            return true;
    }
}

int Bluetooth::getPluginPayload(void **libHandle, bt_codec_t **btCodec,
              bt_enc_payload_t **out_buf, codec_type codecType)
{
    std::string lib_path;
    open_fn_t plugin_open_fn = NULL;
    int status = 0;
    bt_codec_t *codec = NULL;
    void *handle = NULL;

    lib_path = rm->getBtCodecLib(codecFormat, (codecType == ENC ? "enc" : "dec"));
    if (lib_path.empty()) {
        PAL_ERR(LOG_TAG, "fail to get BT codec library");
        return -ENOSYS;
    }

    handle = dlopen(lib_path.c_str(), RTLD_NOW);
    if (handle == NULL) {
        PAL_ERR(LOG_TAG, "failed to dlopen lib %s", lib_path.c_str());
        return -EINVAL;
    }

    dlerror();
    plugin_open_fn = (open_fn_t)dlsym(handle, "plugin_open");
    if (!plugin_open_fn) {
        PAL_ERR(LOG_TAG, "dlsym to open fn failed, err = '%s'", dlerror());
        status = -EINVAL;
        goto error;
    }

    status = plugin_open_fn(&codec, codecFormat, codecType);
    if (status) {
        PAL_ERR(LOG_TAG, "failed to open plugin %d", status);
        goto error;
    }

    status = codec->plugin_populate_payload(codec, codecInfo, (void **)out_buf);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "fail to pack the encoder config %d", status);
        goto error;
    }
    *btCodec = codec;
    *libHandle = handle;
    goto done;

error:
    if (codec)
        codec->close_plugin(codec);

    if (handle)
        dlclose(handle);
done:
    return status;
}

int Bluetooth::checkAndUpdateCustomPayload(uint8_t **paramData, size_t *paramSize)
{
    int ret = -EINVAL;

    if (paramSize == 0)
        return ret;

    ret = updateCustomPayload(*paramData, *paramSize);
    free(*paramData);
    *paramData = NULL;
    *paramSize = 0;
    return 0;
}

int Bluetooth::configureCOPModule(int32_t pcmId, const char *backendName, uint32_t tagId, uint32_t streamMapDir, bool isFbPayload)
{
    PayloadBuilder* builder = new PayloadBuilder();
    uint8_t* paramData = NULL;
    size_t paramSize = 0;
    uint32_t miid = 0;
    int status = 0;

    if ((tagId == COP_PACKETIZER_V0) && ResourceManager::isCPEnabled)
        return status;

    status = SessionAlsaUtils::getModuleInstanceId(virtualMixerHandle,
                     pcmId, backendName, tagId, &miid);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
        goto done;
    }

    switch(tagId) {
    case COP_DEPACKETIZER_V2:
    case COP_PACKETIZER_V2:
        if (streamMapDir & STREAM_MAP_IN) {
            builder->payloadCopV2StreamInfo(&paramData, &paramSize,
                    miid, codecInfo, true /* StreamMapIn */);
            if (isFbPayload)
                status = fbDev->checkAndUpdateCustomPayload(&paramData, &paramSize);
            else
                status = this->checkAndUpdateCustomPayload(&paramData, &paramSize);
            if (status) {
                PAL_ERR(LOG_TAG, "Invalid COPv2 module param size");
                goto done;
            }
        }
        if (streamMapDir & STREAM_MAP_OUT) {
            builder->payloadCopV2StreamInfo(&paramData, &paramSize,
                    miid, codecInfo, false /* StreamMapOut */);
            if (isFbPayload)
                status = fbDev->checkAndUpdateCustomPayload(&paramData, &paramSize);
            else
                status = this->checkAndUpdateCustomPayload(&paramData, &paramSize);
            status = checkAndUpdateCustomPayload(&paramData, &paramSize);
            if (status) {
                PAL_ERR(LOG_TAG, "Invalid COPv2 module param size");
                goto done;
            }
        }
        if (tagId == COP_DEPACKETIZER_V2)
            break;
        [[fallthrough]];
    case COP_PACKETIZER_V0:
        if (ResourceManager::isCPEnabled)
            break;

        // PARAM_ID_COP_PACKETIZER_OUTPUT_MEDIA_FORMAT
        if (isFbPayload) {
            builder->payloadCopPackConfig(&paramData, &paramSize, miid, &fbDev->deviceAttr.config);
            status = fbDev->checkAndUpdateCustomPayload(&paramData, &paramSize);
        } else {
            builder->payloadCopPackConfig(&paramData, &paramSize, miid, &deviceAttr.config);
            status = this->checkAndUpdateCustomPayload(&paramData, &paramSize);
        }
        if (status) {
            PAL_ERR(LOG_TAG, "Invalid COP module param size");
            goto done;
        }
        if (isScramblingEnabled) {
            builder->payloadScramblingConfig(&paramData, &paramSize, miid, isScramblingEnabled);
            if (isFbPayload)
                status = fbDev->checkAndUpdateCustomPayload(&paramData, &paramSize);
            else
                status = this->checkAndUpdateCustomPayload(&paramData, &paramSize);
            if (status) {
                PAL_ERR(LOG_TAG, "Invalid COP module param size");
                goto done;
            }
        }
        break;
    }
done:
    if (builder) {
       delete builder;
       builder = NULL;
    }
    return status;
}

int Bluetooth::configureRATModule(int32_t pcmId, const char *backendName, uint32_t tagId, bool isFbPayload)
{
    uint32_t miid = 0;
    PayloadBuilder* builder = new PayloadBuilder();
    uint8_t* paramData = NULL;
    size_t paramSize = 0;
    int status = 0;

    status = SessionAlsaUtils::getModuleInstanceId(virtualMixerHandle,
                     pcmId, backendName, tagId, &miid);
    if (status) {
        PAL_INFO(LOG_TAG, "Failed to get tag info %x, status = %d", RAT_RENDER, status);
        status = 0;
        goto done;
    } else {
        if (isFbPayload) {
            builder->payloadRATConfig(&paramData, &paramSize, miid, &fbDev->codecConfig);
            status = fbDev->checkAndUpdateCustomPayload(&paramData, &paramSize);
        } else {
            builder->payloadRATConfig(&paramData, &paramSize, miid, &codecConfig);
            status = this->checkAndUpdateCustomPayload(&paramData, &paramSize);
        }
        if (status) {
            PAL_ERR(LOG_TAG, "Invalid RAT module param size");
            goto done;
        }
    }
done:
    if (builder) {
       delete builder;
       builder = NULL;
    }
    return status;
}

int Bluetooth::configurePCMConverterModule(int32_t pcmId, const char *backendName, uint32_t tagId, bool isFbPayload)
{
    uint32_t miid = 0;
    PayloadBuilder* builder = new PayloadBuilder();
    bool isRx;
    uint8_t* paramData = NULL;
    size_t paramSize = 0;
    int status = 0;

    if (isFbPayload) { /* For Feedback path, isRx will be true if normal path is DECODER */
        isRx = (codecType == DEC) ? true : false;
    } else {
        isRx = (codecType == ENC) ? true : false;
    }
    status = SessionAlsaUtils::getModuleInstanceId(virtualMixerHandle,
                     pcmId, backendName, tagId, &miid);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d",
                BT_PCM_CONVERTER, status);
        goto done;
    }

    if (isFbPayload) {
        builder->payloadPcmCnvConfig(&paramData, &paramSize, miid, &fbDev->codecConfig, isRx);
        status = fbDev->checkAndUpdateCustomPayload(&paramData, &paramSize);
    } else {
        builder->payloadPcmCnvConfig(&paramData, &paramSize, miid, &codecConfig, isRx);
        status = this->checkAndUpdateCustomPayload(&paramData, &paramSize);
    }
    if (status) {
        PAL_ERR(LOG_TAG, "Invalid PCM CNV module param size");
        goto done;
    }
done:
    if (builder) {
       delete builder;
       builder = NULL;
    }
    return status;
}

int32_t Bluetooth::getPCMId()
{
    Stream *stream = NULL;
    Session *session = NULL;
    std::shared_ptr<Device> dev = nullptr;
    std::vector<Stream*> activestreams;
    int32_t pcmId = -EINVAL;
    int status = 0;

    dev = Device::getInstance(&deviceAttr, rm);
    if (dev == nullptr) {
        PAL_ERR(LOG_TAG, "device_id[%d] Instance query failed", deviceAttr.id );
        goto done;
    }
    status = rm->getActiveStream_l(activestreams, dev);
    if ((0 != status) || (activestreams.size() == 0)) {
        PAL_ERR(LOG_TAG, "no active stream available");
        goto done;
    }
    stream = static_cast<Stream *>(activestreams[0]);
    stream->getAssociatedSession(&session);
    pcmId = session->getFrontEndId((codecType == ENC) ? RX_HOSTLESS : TX_HOSTLESS);
done:
    return pcmId;
}

int Bluetooth::configureGraphModules()
{
    int status = 0, i;
    int32_t pcmId;
    bt_enc_payload_t *out_buf = NULL;
    PayloadBuilder* builder = new PayloadBuilder();
    std::string backEndName;
    uint8_t* paramData = NULL;
    size_t paramSize = 0;
    uint32_t tagId = 0, streamMapDir = 0;
    uint32_t miid = 0;
    uint32_t num_payloads = 0;
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    uint16_t enc_fmt;
#endif

    PAL_DBG(LOG_TAG, "Enter");
    PAL_INFO(LOG_TAG, "choose BT codec format %x", codecFormat);
    isConfigured = false;
    rm->getBackendName(deviceAttr.id, backEndName);
    pcmId = getPCMId();
    if (pcmId < 0) {
        PAL_ERR(LOG_TAG, "unable to get frontend ID");
        status = -EINVAL;
        goto error;
    }

    /* Retrieve plugin library from resource manager.
     * Map to interested symbols.
     */
    status = getPluginPayload(&pluginHandler, &pluginCodec, &out_buf, codecType);
    if (status) {
        PAL_ERR(LOG_TAG, "failed to payload from plugin");
        goto error;
    }

    codecConfig.sample_rate = out_buf->sample_rate;
    codecConfig.bit_width = out_buf->bit_format;
    codecConfig.ch_info.channels = out_buf->channel_count;

    isAbrEnabled = out_buf->is_abr_enabled;
    codecVersion = out_buf->codec_version;

    /* Reset device GKV for AAC ABR */
    if ((codecFormat == CODEC_TYPE_AAC) && isAbrEnabled)
        updateDeviceMetadata();

    /* Update Device sampleRate based on encoder config */
    updateDeviceAttributes();

    tagId = (codecType == ENC ? BT_PLACEHOLDER_ENCODER : BT_PLACEHOLDER_DECODER);
    status = SessionAlsaUtils::getModuleInstanceId(virtualMixerHandle,
                     pcmId, backEndName.c_str(), tagId, &miid);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
        goto error;
    }

    if (isPlaceholderEncoder()) {
        PAL_INFO(LOG_TAG, "Resetting placeholder module"); // SS_A2DP_FEATURE
        builder->payloadCustomParam(&paramData, &paramSize, NULL, 0,
                                    miid, PARAM_ID_RESET_PLACEHOLDER_MODULE);
        status = checkAndUpdateCustomPayload(&paramData, &paramSize);
        if (status) {
            PAL_ERR(LOG_TAG, "Invalid reset placeholder param size");
            goto error;
        }
    }

    /* BT Encoder & Decoder Module Configuration */
    num_payloads = out_buf->num_blks;
    for (i = 0; i < num_payloads; i++) {
        custom_block_t *blk = out_buf->blocks[i];
        builder->payloadCustomParam(&paramData, &paramSize,
                  (uint32_t *)blk->payload, blk->payload_sz, miid, blk->param_id);
        status = checkAndUpdateCustomPayload(&paramData, &paramSize);
        if (status) {
            PAL_ERR(LOG_TAG, "Failed to populateAPMHeader");
            goto error;
        }
    }

    /* ---------------------------------------------------------------------------
     *       |        Encoder       | PSPD MFC/RAT/PCM CNV | COP Packetizer/HW EP
     * ---------------------------------------------------------------------------
     * SBC   | E_SR = SR of encoder | Same as encoder      | SR:E_SR BW:16 CH:1
     * ------|                      |----------------------|----------------------
     * AAC   | E_CH = CH of encoder | Same as encoder      | SR:E_SR BW:16 CH:1
     * ------|                      |----------------------|----------------------
     * LDAC  | E_BW = BW of encoder | Same as encoder      | if E_SR = 44.1/48KHz
     *       |                      |                      |   SR:E_SR*2 BW:16 CH:1
     *       |                      |                      | else
     *       |                      |                      |   SR:E_SR BW:16 CH:1
     * ------|                      |----------------------|----------------------
     * APTX  |                      | Same as encoder      | SR:E_SR BW:16 CH:1
     * ------|                      |----------------------|----------------------
     * APTX  |                      | Same as encoder      | SR:E_SR BW:16 CH:1
     * HD    |                      |                      |
     * ------|                      |----------------------|----------------------
     * APTX  |                      | Same as encoder      | if E_SR = 44.1/48KHz
     * AD    |                      |                      |   SR:E_SR*2 BW:16 CH:1
     *       |                      |                      | else
     *       |                      |                      |   SR:E_SR BW:16 CH:1
     * ------|----------------------|----------------------|----------------------
     * LC3   | E_SR = SR of encoder | Same as encoder      | SR:96KHz BW:16 CH:1
     *       | E_CH = CH of encoder |                      |
     *       | E_BW = 24            |                      |
     * ---------------------------------------------------------------------------
     * APTX      | E_SR = 32KHz     | Same as encoder      | SR:96KHz BW:16 CH:1
     * AD Speech | E_CH = 1         |                      |
     *           | E_BW = 16        |                      |
     * ---------------------------------------------------------------------------
     * LC3       | E_SR = SR of encoder | Same as encoder  | SR:96KHz BW:16 CH:1
     * Voice     | E_CH = CH of encoder |                  |
     *           | E_BW = 24            |                  |
     * --------------------------------------------------------------------------- */
    switch (codecFormat) {
    case CODEC_TYPE_APTX_AD_SPEECH:
        PAL_DBG(LOG_TAG, "Skip the rest, static configurations coming from ACDB");
        break;
// SS_BT_HFP - H_127 : RVP
#ifdef SEC_AUDIO_BLUETOOTH
    case CODEC_TYPE_RVP:
        PAL_DBG(LOG_TAG, "RVP, Skip the rest of static configurations coming from ACDB");
        break;
#endif
// SS_BT_HFP - H_127 end
    case CODEC_TYPE_LC3:
    case CODEC_TYPE_APTX_AD_QLEA:
    case CODEC_TYPE_APTX_AD_R4: {
        builder->payloadLC3Config(&paramData, &paramSize, miid,
                                  isLC3MonoModeOn);
        status = checkAndUpdateCustomPayload(&paramData, &paramSize);
        if (status) {
            PAL_ERR(LOG_TAG, "Invalid LC3 param size");
            goto error;
        }
        status = configureRATModule(pcmId, backEndName.c_str(), RAT_RENDER, false);
        if (status) {
            PAL_ERR(LOG_TAG, "Failed to configure RAT module");
            goto error;
        }
        status = configurePCMConverterModule(pcmId, backEndName.c_str(), BT_PCM_CONVERTER, false);
        if (status) {
            PAL_ERR(LOG_TAG, "Failed to configure PCM Converter");
            goto error;
        }
        tagId = (codecType == DEC) ? COP_DEPACKETIZER_V2 : COP_PACKETIZER_V2;
        streamMapDir = (codecType == DEC) ? STREAM_MAP_IN | STREAM_MAP_OUT : STREAM_MAP_OUT;
        status = configureCOPModule(pcmId, backEndName.c_str(), tagId, streamMapDir, false);
        if (status) {
            PAL_ERR(LOG_TAG, "Failed to configure COP module 0x%x", tagId);
            goto error;
        }
        break;
    }
    case CODEC_TYPE_APTX_DUAL_MONO:
    case CODEC_TYPE_APTX_AD:
        builder->payloadTWSConfig(&paramData, &paramSize, miid,
                                  isTwsMonoModeOn, codecFormat);
        status = checkAndUpdateCustomPayload(&paramData, &paramSize);
        if (status) {
            PAL_ERR(LOG_TAG, "Invalid TWS param size");
            goto error;
        }
        [[fallthrough]];
    default:
        switch(codecType) {
        case ENC:
#if defined(SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD) && !defined(QCA_OFFLOAD) // SS_A2DP_FEATURE TODO CHECK
        status = session->getMIID(backEndName.c_str(), SS_COP_PACKETIZER, &copMiid);
        if (status) {
            PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d\n",
                    SS_COP_PACKETIZER, status);
            goto error;
        }

        /* SS packetizer needs to be configured same as Device attributes */
        builder->payloadSsPackConfig(&paramData, &paramSize, copMiid, &deviceAttr.config);
        if (paramSize) {
            dev->updateCustomPayload(paramData, paramSize);
            delete [] paramData;
            paramData = NULL;
            paramSize = 0;
        } else {
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Invalid COP module param size");
            goto error;
        }

        if (codecFormat == CODEC_TYPE_SBC) enc_fmt = ENC_SBC;
        else if (codecFormat == CODEC_TYPE_APTX)  enc_fmt = ENC_APTX;
        else enc_fmt = ENC_SSC;

        builder->payloadSsPackEncConfig(&paramData, &paramSize, copMiid, enc_fmt);
        if (paramSize) {
            dev->updateCustomPayload(paramData, paramSize);
            delete [] paramData;
            paramData = NULL;
            paramSize = 0;
        } else {
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Invalid COP module param size");
            goto error;
        }

        if (codecFormat == CODEC_TYPE_SBC) {
            builder->payloadSsPackMtuConfig(&paramData, &paramSize, copMiid, peerMtu);
            if (paramSize) {
                dev->updateCustomPayload(paramData, paramSize);
                delete [] paramData;
                paramData = NULL;
                paramSize = 0;
            } else {
                status = -EINVAL;
                PAL_ERR(LOG_TAG, "Invalid COP module param size");
                goto error;
            }
        }

        builder->payloadSsPackSuspendConfig(&paramData, &paramSize, copMiid, 0);
        if (paramSize) {
            dev->updateCustomPayload(paramData, paramSize);
            delete [] paramData;
            paramData = NULL;
            paramSize = 0;
        } else {
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Invalid COP module param size");
            goto error;
        }

        if (isScramblingEnabled) {
            builder->payloadScramblingConfig(&paramData, &paramSize, copMiid, isScramblingEnabled);
            if (paramSize) {
                dev->updateCustomPayload(paramData, paramSize);
                delete [] paramData;
                paramData = NULL;
                paramSize = 0;
            } else {
                status = -EINVAL;
                PAL_ERR(LOG_TAG, "Invalid COP module param size");
                goto error;
            }
        }
#else
            status = configureCOPModule(pcmId, backEndName.c_str(), COP_PACKETIZER_V0, 0/*don't care */, false);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to configure COP Packetizer");
                goto error;
            }
            status = configureRATModule(pcmId, backEndName.c_str(), RAT_RENDER, false);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to configure RAT module");
                goto error;
            }
            status = configurePCMConverterModule(pcmId, backEndName.c_str(), BT_PCM_CONVERTER, false);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to configure pcm Converter");
                goto error;
            }
#endif
            break;
        case DEC:
        default:
            break;
        }
    }
done:
    isConfigured = true;

error:
    if (builder) {
       delete builder;
       builder = NULL;
    }
    PAL_DBG(LOG_TAG, "Exit");
    return status;
}


int Bluetooth::configureNrecParameters(bool isNrecEnabled)
{
    int status = 0, i;
    int32_t pcmId;
    PayloadBuilder* builder = new PayloadBuilder();
    std::string backEndName;
    uint8_t* paramData = NULL;
    size_t paramSize = 0;
    uint32_t miid = 0;
    uint32_t num_payloads = 0;

    PAL_DBG(LOG_TAG, "Enter");
    if (!builder) {
        PAL_ERR(LOG_TAG, "Failed to new PayloadBuilder()");
        status = -ENOMEM;
        goto exit;
    }
    rm->getBackendName(deviceAttr.id, backEndName);
    pcmId = getPCMId();
    if (pcmId < 0) {
        PAL_ERR(LOG_TAG, "unable to get frontend ID");
        status = -EINVAL;
        goto exit;
    }

    status = SessionAlsaUtils::getModuleInstanceId(virtualMixerHandle,
                     pcmId, backEndName.c_str(), TAG_ECNS, &miid);
    if (!status) {
        PAL_DBG(LOG_TAG, "Setting NREC Configuration");
        builder->payloadNRECConfig(&paramData, &paramSize,
            miid, isNrecEnabled);
        status = checkAndUpdateCustomPayload(&paramData, &paramSize);
        if (status) {
            PAL_ERR(LOG_TAG, "Failed to payloadNRECConfig");
            goto exit;
        }
    } else {
        PAL_ERR(LOG_TAG, "Failed to find ECNS module info %x, status = %d"
            "cannot set NREC parameters",
            TAG_ECNS, status);
    }
exit:
    if (builder) {
       delete builder;
       builder = NULL;
    }
    PAL_DBG(LOG_TAG, "Exit");
    return status;
}

int Bluetooth::getCodecConfig(struct pal_media_config *config)
{
    if (!config) {
        PAL_ERR(LOG_TAG, "Invalid codec config");
        return -EINVAL;
    }

    if (isConfigured) {
        ar_mem_cpy(config, sizeof(struct pal_media_config),
                &codecConfig, sizeof(struct pal_media_config));
    }

    return 0;
}

void Bluetooth::startAbr()
{
    int ret = 0, dir;
    struct pal_device fbDevice;
    struct pal_channel_info ch_info;
    struct pal_stream_attributes sAttr;
    std::string backEndName;
    std::vector <std::pair<int, int>> keyVector;
    struct pcm_config config;
    struct mixer_ctl *connectCtrl = NULL;
    struct mixer_ctl *disconnectCtrl = NULL;
    struct mixer_ctl *btSetFeedbackChannelCtrl = NULL;
    std::ostringstream connectCtrlName;
    std::ostringstream disconnectCtrlName;
    unsigned int flags;
    uint32_t tagId = 0, miid = 0, streamMapDir = 0;
    void *pluginLibHandle = NULL;
    bt_codec_t *codec = NULL;
    bt_enc_payload_t *out_buf = NULL;
    custom_block_t *blk = NULL;
    uint8_t* paramData = NULL;
    size_t paramSize = 0;
    PayloadBuilder* builder = NULL;
    bool isDeviceLocked = false;
    audio_lc3_codec_cfg_t* bt_ble_codec = NULL;

    memset(&fbDevice, 0, sizeof(fbDevice));
    memset(&sAttr, 0, sizeof(sAttr));
    memset(&config, 0, sizeof(config));

    mAbrMutex.lock();
    if (abrRefCnt > 0) {
        abrRefCnt++;
        mAbrMutex.unlock();
        return;
    }
    /* Configure device attributes */
    ch_info.channels = CHANNELS_1;
    ch_info.ch_map[0] = PAL_CHMAP_CHANNEL_FL;
    fbDevice.config.ch_info = ch_info;
    fbDevice.config.bit_width = BITWIDTH_16;
    fbDevice.config.aud_fmt_id = PAL_AUDIO_FMT_DEFAULT_COMPRESSED;

    if ((codecFormat == CODEC_TYPE_APTX_AD_SPEECH) ||
            (codecFormat == CODEC_TYPE_LC3) ||
            (codecFormat == CODEC_TYPE_APTX_AD_QLEA) ||
            (codecFormat == CODEC_TYPE_RVP) || // SS_BT_HFP - H_127 : RVP
            (codecFormat == CODEC_TYPE_APTX_AD_R4)) {
        fbDevice.config.sample_rate = deviceAttr.config.sample_rate;
    } else {
        fbDevice.config.sample_rate = SAMPLINGRATE_8K;
    }

    if (codecType == DEC) { /* Usecase is TX, feedback device will be RX */
        if (deviceAttr.id == PAL_DEVICE_IN_BLUETOOTH_A2DP) {
            fbDevice.id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
        } else if (deviceAttr.id == PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET) {
            fbDevice.id = PAL_DEVICE_OUT_BLUETOOTH_SCO;
        } else if (deviceAttr.id == PAL_DEVICE_IN_BLUETOOTH_BLE) {
            fbDevice.id = PAL_DEVICE_OUT_BLUETOOTH_BLE;
        }
        dir = RX_HOSTLESS;
        flags = PCM_OUT;
    } else {
        if (deviceAttr.id == PAL_DEVICE_OUT_BLUETOOTH_A2DP) {
            fbDevice.id = PAL_DEVICE_IN_BLUETOOTH_A2DP;
        } else if (deviceAttr.id == PAL_DEVICE_OUT_BLUETOOTH_SCO) {
            fbDevice.id = PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
        } else if (deviceAttr.id == PAL_DEVICE_OUT_BLUETOOTH_BLE ||
                   deviceAttr.id == PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST) {
            fbDevice.id = PAL_DEVICE_IN_BLUETOOTH_BLE;
        }
        dir = TX_HOSTLESS;
        flags = PCM_IN;
    }

    if ((fbDevice.id == PAL_DEVICE_OUT_BLUETOOTH_SCO) ||
        (fbDevice.id == PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET))
        fbDev = std::dynamic_pointer_cast<BtSco>(BtSco::getInstance(&fbDevice, rm));
    else
        fbDev = std::dynamic_pointer_cast<BtA2dp>(BtA2dp::getInstance(&fbDevice, rm));

    if (!fbDev) {
        PAL_ERR(LOG_TAG, "failed to get Bt device object for %d", fbDevice.id);
        goto done;
    }
    builder = new PayloadBuilder();

    ret = PayloadBuilder::getBtDeviceKV(fbDevice.id, keyVector, codecFormat,
        true, true);
    if (ret)
        PAL_ERR(LOG_TAG, "No KVs found for device id %d codec format:0x%x",
            fbDevice.id, codecFormat);

    /* Configure Device Metadata */
    rm->getBackendName(fbDevice.id, backEndName);
    ret = SessionAlsaUtils::setDeviceMetadata(rm, backEndName, keyVector);
    if (ret) {
        PAL_ERR(LOG_TAG, "setDeviceMetadata for feedback device failed");
        goto done;
    }
    ret = SessionAlsaUtils::setDeviceMediaConfig(rm, backEndName, &fbDevice);
    if (ret) {
        PAL_ERR(LOG_TAG, "setDeviceMediaConfig for feedback device failed");
        goto done;
    }

    /* Retrieve Hostless PCM device id */
    sAttr.type = PAL_STREAM_LOW_LATENCY;
    sAttr.direction = PAL_AUDIO_INPUT_OUTPUT;
    fbpcmDevIds = rm->allocateFrontEndIds(sAttr, dir);
    if (fbpcmDevIds.size() == 0) {
        PAL_ERR(LOG_TAG, "allocateFrontEndIds failed");
        ret = -ENOSYS;
        goto done;
    }

    connectCtrlName << "PCM" << fbpcmDevIds.at(0) << " connect";
    connectCtrl = mixer_get_ctl_by_name(virtualMixerHandle, connectCtrlName.str().data());
    if (!connectCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s", connectCtrlName.str().data());
        goto free_fe;
    }

    ret = mixer_ctl_set_enum_by_string(connectCtrl, backEndName.c_str());
    if (ret) {
        PAL_ERR(LOG_TAG, "Mixer control %s set with %s failed: %d",
                connectCtrlName.str().data(), backEndName.c_str(), ret);
        goto free_fe;
    }

    // Notify ABR usecase information to BT driver to distinguish
    // between SCO and feedback usecase
    btSetFeedbackChannelCtrl = mixer_get_ctl_by_name(hwMixerHandle,
                                        MIXER_SET_FEEDBACK_CHANNEL);
    if (!btSetFeedbackChannelCtrl) {
        PAL_ERR(LOG_TAG, "ERROR %s mixer control not identified",
                MIXER_SET_FEEDBACK_CHANNEL);
        goto disconnect_fe;
    }

    if (mixer_ctl_set_value(btSetFeedbackChannelCtrl, 0, 1) != 0) {
        PAL_ERR(LOG_TAG, "Failed to set BT usecase");
        goto disconnect_fe;
    }

    fbDev->lockDeviceMutex();
    isDeviceLocked = true;

    if (fbDev->isConfigured == true) {
        PAL_INFO(LOG_TAG, "feedback path is already configured");
        goto start_pcm;
    }

    /* update device attributes to reflect proper device configuration */
    fbDev->deviceAttr.config = fbDevice.config;

    switch (fbDevice.id) {
    case PAL_DEVICE_OUT_BLUETOOTH_SCO:
    case PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET:
    case PAL_DEVICE_OUT_BLUETOOTH_BLE:
        tagId = (codecType == DEC ? BT_PLACEHOLDER_ENCODER : BT_PLACEHOLDER_DECODER);
        ret = SessionAlsaUtils::getModuleInstanceId(virtualMixerHandle,
                     fbpcmDevIds.at(0), backEndName.c_str(), tagId, &miid);
        if (ret) {
            PAL_ERR(LOG_TAG, "getMiid for feedback device failed");
            goto disconnect_fe;
        }

        ret = getPluginPayload(&pluginLibHandle, &codec, &out_buf, (codecType == DEC ? ENC : DEC));
        if (ret) {
            PAL_ERR(LOG_TAG, "getPluginPayload failed");
            goto disconnect_fe;
        }

        /* SWB Encoder/Decoder has only 1 param, read block 0 */
        if (out_buf->num_blks != 1) {
            PAL_ERR(LOG_TAG, "incorrect block size %d", out_buf->num_blks);
            goto disconnect_fe;
        }
        fbDev->codecConfig.sample_rate = out_buf->sample_rate;
        fbDev->codecConfig.bit_width = out_buf->bit_format;
        fbDev->codecConfig.ch_info.channels = out_buf->channel_count;
        fbDev->isAbrEnabled = out_buf->is_abr_enabled;

        blk = out_buf->blocks[0];
        builder->payloadCustomParam(&paramData, &paramSize,
                  (uint32_t *)blk->payload, blk->payload_sz, miid, blk->param_id);

        codec->close_plugin(codec);
        dlclose(pluginLibHandle);

        if (!paramData) {
            PAL_ERR(LOG_TAG, "Failed to populateAPMHeader");
            ret = -ENOMEM;
            goto disconnect_fe;
        }
        ret = fbDev->checkAndUpdateCustomPayload(&paramData, &paramSize);
        if (ret) {
            PAL_ERR(LOG_TAG, "Invalid COPv2 module param size");
            goto done;
        }
        switch (codecFormat) {
        case CODEC_TYPE_LC3:
        case CODEC_TYPE_APTX_AD_QLEA:
        case CODEC_TYPE_APTX_AD_R4:
            tagId = (flags == PCM_IN) ? COP_DEPACKETIZER_V2 : COP_PACKETIZER_V2;
            streamMapDir = (flags == PCM_IN) ? STREAM_MAP_IN | STREAM_MAP_OUT : STREAM_MAP_OUT;
            ret = configureCOPModule(fbpcmDevIds.at(0), backEndName.c_str(), tagId, streamMapDir, true);
            if (ret) {
                PAL_ERR(LOG_TAG, "Failed to configure COP module");
                goto disconnect_fe;
            }
            ret = configureRATModule(fbpcmDevIds.at(0), backEndName.c_str(), RAT_RENDER, true);
            if (ret) {
                PAL_ERR(LOG_TAG, "Failed to configure RAT module");
                goto disconnect_fe;
            }
            ret = configurePCMConverterModule(fbpcmDevIds.at(0), backEndName.c_str(), BT_PCM_CONVERTER, true);
            if (ret) {
                PAL_ERR(LOG_TAG, "Failed to configure PCM Converter");
                goto disconnect_fe;
            }
            break;
        default:
            break;
        }
        break;
    case PAL_DEVICE_IN_BLUETOOTH_A2DP:
    case PAL_DEVICE_IN_BLUETOOTH_BLE:
        switch (codecFormat) {
        case CODEC_TYPE_LC3:
        case CODEC_TYPE_APTX_AD_QLEA:
        case CODEC_TYPE_APTX_AD_R4:
            ret = configureCOPModule(fbpcmDevIds.at(0), backEndName.c_str(), COP_DEPACKETIZER_V2, STREAM_MAP_OUT, true);
            if (ret) {
                PAL_ERR(LOG_TAG, "Failed to configure 0x%x", COP_DEPACKETIZER_V2);
                goto disconnect_fe;
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    if (fbDev->customPayloadSize) {
        ret = SessionAlsaUtils::setDeviceCustomPayload(rm, backEndName,
                                    fbDev->customPayload, fbDev->customPayloadSize);
        if (ret) {
            PAL_ERR(LOG_TAG, "Error: Dev setParam failed for %d", fbDevice.id);
            goto disconnect_fe;
        }
        free(fbDev->customPayload);
        fbDev->customPayload = NULL;
        fbDev->customPayloadSize = 0;
    }
start_pcm:
    config.rate = SAMPLINGRATE_8K;
    config.format = PCM_FORMAT_S16_LE;
    config.channels = CHANNELS_1;
    config.period_size = 240;
    config.period_count = 2;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;
    fbPcm = pcm_open(rm->getVirtualSndCard(), fbpcmDevIds.at(0), flags, &config);
    if (!fbPcm) {
        PAL_ERR(LOG_TAG, "pcm open failed");
        goto disconnect_fe;
    }

    if (!pcm_is_ready(fbPcm)) {
        PAL_ERR(LOG_TAG, "pcm open not ready");
        goto err_pcm_open;
    }

    ret = pcm_start(fbPcm);
    if (ret) {
        PAL_ERR(LOG_TAG, "pcm_start rx failed %d", ret);
        goto err_pcm_open;
    }

    if ((codecFormat == CODEC_TYPE_APTX_AD_SPEECH) ||
        ((codecFormat == CODEC_TYPE_LC3) &&
         (fbDevice.id == PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET ||
          fbDevice.id == PAL_DEVICE_OUT_BLUETOOTH_SCO ||
          fbDevice.id == PAL_DEVICE_OUT_BLUETOOTH_BLE)) ||
        ((codecFormat == CODEC_TYPE_APTX_AD_R4) &&
         (fbDevice.id == PAL_DEVICE_OUT_BLUETOOTH_BLE))) {
        fbDev->isConfigured = true;
        fbDev->deviceStartStopCount++;
        fbDev->deviceCount++;
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
        std::string palDeviceName = rm->getPALDeviceName(fbDev->deviceAttr.id);
        PAL_INFO(LOG_TAG, " deviceCount %d deviceStartStopCount %d for device id %d (%s)",  // SS_A2DP_FEATURE
                fbDev->deviceCount, fbDev->deviceStartStopCount, fbDev->deviceAttr.id, palDeviceName.c_str());
        if (fbDev->deviceCount == 1) {
            android::String8 deviceLog = android::String8::format(
                "startAbr: deviceCount %d for device id %d (%s)",
                fbDev->deviceCount, fbDev->deviceAttr.id, palDeviceName.c_str());
            android::AudioInfo::GetInstance()->setSecSaveAudioLog(deviceLog);
        }
#else
        PAL_INFO(LOG_TAG, " deviceCount %d deviceStartStopCount %d for device id %d",  // SS_A2DP_FEATURE
                fbDev->deviceCount, fbDev->deviceStartStopCount, fbDev->deviceAttr.id);
#endif
    }

// SS_BT_HFP - H_127 : RVP
    if (codecFormat == CODEC_TYPE_RVP) {
        fbDev->isConfigured = true;
        fbDev->deviceStartStopCount++;
        fbDev->deviceCount++;
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
        std::string palDeviceName = rm->getPALDeviceName(fbDev->deviceAttr.id);
        PAL_DBG(LOG_TAG, " codec RVP deviceCount %d deviceStartStopCount %d for device id %d (%s)",
                fbDev->deviceCount, fbDev->deviceStartStopCount, fbDev->deviceAttr.id, palDeviceName.c_str());
        if (fbDev->deviceCount == 1) {
            android::String8 deviceLog = android::String8::format(
                "startAbr: codec RVP deviceCount %d for device id %d (%s)",
                fbDev->deviceCount, fbDev->deviceAttr.id, palDeviceName.c_str());
            android::AudioInfo::GetInstance()->setSecSaveAudioLog(deviceLog);
        }
#else
        PAL_DBG(LOG_TAG, " codec RVP deviceCount %d deviceStartStopCount %d for device id %d",
                fbDev->deviceCount, fbDev->deviceStartStopCount, fbDev->deviceAttr.id);
#endif
    }
// SS_BT_HFP - H_127 end

    abrRefCnt++;
    PAL_INFO(LOG_TAG, "Feedback Device started successfully");
    goto done;
err_pcm_open:
    pcm_close(fbPcm);
    fbPcm = NULL;
disconnect_fe:
    disconnectCtrlName << "PCM" << fbpcmDevIds.at(0) << " disconnect";
    disconnectCtrl = mixer_get_ctl_by_name(virtualMixerHandle, disconnectCtrlName.str().data());
    if(disconnectCtrl != NULL){
       mixer_ctl_set_enum_by_string(disconnectCtrl, backEndName.c_str());
    }
free_fe:
    rm->freeFrontEndIds(fbpcmDevIds, sAttr, dir);
    fbpcmDevIds.clear();
done:
    if (isDeviceLocked) {
        isDeviceLocked = false;
        if (fbDev != NULL) { // SS_A2DP_FEATURE
            fbDev->unlockDeviceMutex();
        }
    }
    mAbrMutex.unlock();
    if (builder) {
       delete builder;
       builder = NULL;
    }
    return;
}

void Bluetooth::stopAbr()
{
    struct pal_stream_attributes sAttr;
    struct mixer_ctl *btSetFeedbackChannelCtrl = NULL;
    int dir, ret = 0;
    bool isfbDeviceLocked = false;

    mAbrMutex.lock();
    if (!fbPcm) {
        PAL_ERR(LOG_TAG, "fbPcm is null");
        mAbrMutex.unlock();
        return;
    }

    if (abrRefCnt == 0) {
        PAL_INFO(LOG_TAG, "skip as abrRefCnt is zero"); // SS_A2DP_FEATURE
        mAbrMutex.unlock();
        return;
    }

    if (--abrRefCnt > 0) {
        PAL_INFO(LOG_TAG, "abrRefCnt is %d", abrRefCnt); // SS_A2DP_FEATURE
        mAbrMutex.unlock();
        return;
    }

    memset(&sAttr, 0, sizeof(sAttr));
    sAttr.type = PAL_STREAM_LOW_LATENCY;
    sAttr.direction = PAL_AUDIO_INPUT_OUTPUT;

    if (fbDev) {
        fbDev->lockDeviceMutex();
        isfbDeviceLocked = true;
    }
    pcm_stop(fbPcm);
    pcm_close(fbPcm);
    // SS_A2DP_FEATURE fbPcm = NULL;

    // Reset BT driver mixer control for ABR usecase
    btSetFeedbackChannelCtrl = mixer_get_ctl_by_name(hwMixerHandle,
                                        MIXER_SET_FEEDBACK_CHANNEL);
    if (!btSetFeedbackChannelCtrl) {
        PAL_ERR(LOG_TAG, "%s mixer control not identified",
                MIXER_SET_FEEDBACK_CHANNEL);
    } else if (mixer_ctl_set_value(btSetFeedbackChannelCtrl, 0, 0) != 0) {
        PAL_ERR(LOG_TAG, "Failed to reset BT usecase");
    }

    if ((codecFormat == CODEC_TYPE_APTX_AD_SPEECH) && fbDev) {
        if ((fbDev->deviceStartStopCount > 0) &&
            (--fbDev->deviceStartStopCount == 0)) {
            fbDev->isConfigured = false;
            fbDev->isAbrEnabled = false;
        }
        if (fbDev->deviceCount > 0)
            fbDev->deviceCount--;
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
        std::string palDeviceName = rm->getPALDeviceName(fbDev->deviceAttr.id);
        PAL_INFO(LOG_TAG, " deviceCount %d deviceStartStopCount %d for device id %d (%s)", // SS_A2DP_FEATURE
                fbDev->deviceCount, fbDev->deviceStartStopCount, fbDev->deviceAttr.id, palDeviceName.c_str());
        if (fbDev->deviceCount == 0) {
            android::String8 deviceLog = android::String8::format(
                "stopAbr: deviceCount %d for device id %d (%s)",
                fbDev->deviceCount, fbDev->deviceAttr.id, palDeviceName.c_str());
            android::AudioInfo::GetInstance()->setSecSaveAudioLog(deviceLog);
        }
#else
        PAL_INFO(LOG_TAG, " deviceCount %d deviceStartStopCount %d for device id %d", // SS_A2DP_FEATURE
                fbDev->deviceCount, fbDev->deviceStartStopCount, fbDev->deviceAttr.id);
#endif
    }

// SS_BT_HFP - H_127 : RVP
    if ((codecFormat == CODEC_TYPE_RVP) && fbDev) {
        if ((fbDev->deviceStartStopCount > 0) &&
                (--fbDev->deviceStartStopCount == 0)) {
            fbDev->isConfigured = false;
            fbDev->isAbrEnabled = false;
        }
        if (fbDev->deviceCount > 0) {
            fbDev->deviceCount--;
        }
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
        std::string palDeviceName = rm->getPALDeviceName(fbDev->deviceAttr.id);
        PAL_DBG(LOG_TAG, " codec RVP deviceCount %d deviceStartStopCount %d for device id %d (%s)",
                fbDev->deviceCount, fbDev->deviceStartStopCount, fbDev->deviceAttr.id, palDeviceName.c_str());
        if (fbDev->deviceCount == 0) {
            android::String8 deviceLog = android::String8::format(
                "stopAbr: codec RVP deviceCount %d for device id %d (%s)",
                fbDev->deviceCount, fbDev->deviceAttr.id, palDeviceName.c_str());
            android::AudioInfo::GetInstance()->setSecSaveAudioLog(deviceLog);
        }
#else
        PAL_DBG(LOG_TAG, " codec RVP deviceCount %d deviceStartStopCount %d for device id %d",
                fbDev->deviceCount, fbDev->deviceStartStopCount, fbDev->deviceAttr.id);
#endif
    }
// SS_BT_HFP - H_127 end

    if ((((codecFormat == CODEC_TYPE_LC3) &&
        (deviceAttr.id == PAL_DEVICE_OUT_BLUETOOTH_SCO ||
         deviceAttr.id == PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET ||
         deviceAttr.id == PAL_DEVICE_IN_BLUETOOTH_BLE)) ||
        ((codecFormat == CODEC_TYPE_APTX_AD_R4) &&
         (deviceAttr.id == PAL_DEVICE_IN_BLUETOOTH_BLE))) &&
         fbDev) {
        if ((fbDev->deviceStartStopCount > 0) &&
            (--fbDev->deviceStartStopCount == 0)) {
            fbDev->isConfigured = false;
            fbDev->isAbrEnabled = false;
        }
        if (fbDev->deviceCount > 0)
            fbDev->deviceCount--;
        PAL_INFO(LOG_TAG, " deviceCount %d deviceStartStopCount %d for device id %d", // SS_A2DP_FEATURE
                fbDev->deviceCount, fbDev->deviceStartStopCount, fbDev->deviceAttr.id);
    }

free_fe:
    dir = ((codecType == DEC) ? RX_HOSTLESS : TX_HOSTLESS);
    if (fbpcmDevIds.size()) {
        rm->freeFrontEndIds(fbpcmDevIds, sAttr, dir);
        fbpcmDevIds.clear();
    }

    /* Check for deviceStartStopCount, to avoid false reset of isAbrEnabled flag in
     * case of BLE playback path stops during ongoing capture session
     */
    if (deviceStartStopCount == 1) {
        isAbrEnabled = false;
    }

    if (isfbDeviceLocked) {
        isfbDeviceLocked = false;
        fbDev->unlockDeviceMutex();
    }
    mAbrMutex.unlock();
}

int32_t Bluetooth::configureSlimbusClockSrc(void)
{
    return configureDeviceClockSrc(BT_SLIMBUS_CLK_STR,
                rm->getBtSlimClockSrc(codecFormat));
}


/* Scope of BtA2dp class */
// definition of static BtA2dp member variables
std::shared_ptr<Device> BtA2dp::objRx = nullptr;
std::shared_ptr<Device> BtA2dp::objTx = nullptr;
std::shared_ptr<Device> BtA2dp::objBleRx = nullptr;
std::shared_ptr<Device> BtA2dp::objBleTx = nullptr;
std::shared_ptr<Device> BtA2dp::objBleBroadcastRx = nullptr;
void *BtA2dp::bt_lib_source_handle = nullptr;
void *BtA2dp::bt_lib_sink_handle = nullptr;
bt_audio_pre_init_t BtA2dp::bt_audio_pre_init = nullptr;
audio_source_open_t BtA2dp::audio_source_open = nullptr;
audio_source_close_t BtA2dp::audio_source_close = nullptr;
audio_source_start_t BtA2dp::audio_source_start = nullptr;
audio_source_stop_t BtA2dp::audio_source_stop = nullptr;
audio_source_suspend_t BtA2dp::audio_source_suspend = nullptr;
audio_source_handoff_triggered_t BtA2dp::audio_source_handoff_triggered = nullptr;
clear_source_a2dpsuspend_flag_t BtA2dp::clear_source_a2dpsuspend_flag = nullptr;
audio_get_enc_config_t BtA2dp::audio_get_enc_config = nullptr;
audio_source_check_a2dp_ready_t BtA2dp::audio_source_check_a2dp_ready = nullptr;
audio_is_tws_mono_mode_enable_t BtA2dp::audio_is_tws_mono_mode_enable = nullptr;
audio_sink_get_a2dp_latency_t BtA2dp::audio_sink_get_a2dp_latency = nullptr;
audio_sink_start_t BtA2dp::audio_sink_start = nullptr;
audio_sink_stop_t BtA2dp::audio_sink_stop = nullptr;
audio_get_dec_config_t BtA2dp::audio_get_dec_config = nullptr;
audio_sink_session_setup_complete_t BtA2dp::audio_sink_session_setup_complete = nullptr;
audio_sink_check_a2dp_ready_t BtA2dp::audio_sink_check_a2dp_ready = nullptr;
audio_is_scrambling_enabled_t BtA2dp::audio_is_scrambling_enabled = nullptr;
audio_sink_suspend_t BtA2dp::audio_sink_suspend = nullptr;
audio_sink_open_t BtA2dp::audio_sink_open = nullptr;
audio_sink_close_t BtA2dp::audio_sink_close = nullptr;
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
audio_get_dynamic_bitrate_t BtA2dp::audio_get_dynamic_bitrate = nullptr;
audio_get_peer_mtu_t BtA2dp::audio_get_peer_mtu = nullptr;
#endif

btoffload_update_metadata_api_t BtA2dp::btoffload_update_metadata_api = nullptr;
audio_source_open_api_t BtA2dp::audio_source_open_api = nullptr;
audio_source_close_api_t BtA2dp::audio_source_close_api = nullptr;
audio_source_start_api_t BtA2dp::audio_source_start_api = nullptr;
audio_source_stop_api_t BtA2dp::audio_source_stop_api = nullptr;
audio_source_suspend_api_t BtA2dp::audio_source_suspend_api = nullptr;
audio_get_enc_config_api_t BtA2dp::audio_get_enc_config_api = nullptr;
audio_source_check_a2dp_ready_api_t BtA2dp::audio_source_check_a2dp_ready_api = nullptr;
audio_sink_get_a2dp_latency_api_t BtA2dp::audio_sink_get_a2dp_latency_api = nullptr;
audio_sink_start_api_t BtA2dp::audio_sink_start_api = nullptr;
audio_sink_stop_api_t BtA2dp::audio_sink_stop_api = nullptr;
audio_sink_suspend_api_t BtA2dp::audio_sink_suspend_api = nullptr;
audio_sink_open_api_t BtA2dp::audio_sink_open_api = nullptr;
audio_sink_close_api_t BtA2dp::audio_sink_close_api = nullptr;
audio_source_get_supported_latency_modes_api_t BtA2dp::audio_source_get_supported_latency_modes_api = nullptr;
audio_source_set_latency_mode_api_t BtA2dp::audio_source_set_latency_mode_api = nullptr;

BtA2dp::BtA2dp(struct pal_device *device, std::shared_ptr<ResourceManager> Rm)
      : Bluetooth(device, Rm),
        a2dpState(A2DP_STATE_DISCONNECTED)
{
    a2dpRole = ((device->id == PAL_DEVICE_IN_BLUETOOTH_A2DP) || (device->id == PAL_DEVICE_IN_BLUETOOTH_BLE)) ? SINK : SOURCE;
    codecType = ((device->id == PAL_DEVICE_IN_BLUETOOTH_A2DP) || (device->id == PAL_DEVICE_IN_BLUETOOTH_BLE)) ? DEC : ENC;
    pluginHandler = NULL;
    pluginCodec = NULL;

    param_bt_a2dp.reconfig = false;
    param_bt_a2dp.a2dp_suspended = false;
    param_bt_a2dp.a2dp_capture_suspended = false;
    param_bt_a2dp.is_force_switch = false;
    param_bt_a2dp.is_suspend_setparam = false;
#if defined(SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD) && defined(SEC_AUDIO_BLE_OFFLOAD)
    param_bt_a2dp.a2dp_suspended_for_ble = false;
    param_bt_a2dp.bt_mix_latency = 0;
#endif
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    isA2dpOffloadSupported = true;
#else
    isA2dpOffloadSupported =
            property_get_bool("ro.bluetooth.a2dp_offload.supported", false) &&
            !property_get_bool("persist.bluetooth.a2dp_offload.disabled", false);
#endif
    PAL_DBG(LOG_TAG, "A2DP offload supported = %d",
            isA2dpOffloadSupported);
    param_bt_a2dp.reconfig_supported = isA2dpOffloadSupported;
    param_bt_a2dp.latency = 0;
    codecLatency = 0;
    a2dpLatencyMode = AUDIO_LATENCY_MODE_FREE;

    if (isA2dpOffloadSupported) {
        init();
    }
}

BtA2dp::~BtA2dp()
{
}

tSESSION_TYPE BtA2dp::get_session_type()
{
    tSESSION_TYPE session_type = A2DP_HARDWARE_OFFLOAD_DATAPATH;
    if (a2dpRole == SOURCE) {
        if (deviceAttr.id == PAL_DEVICE_OUT_BLUETOOTH_BLE) {
            session_type = LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
        } else if (deviceAttr.id == PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST) {
            session_type = LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
        } else {
            session_type = A2DP_HARDWARE_OFFLOAD_DATAPATH;
        }
    } else {
        session_type = LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH;
    }

    return session_type;
}

void BtA2dp::open_a2dp_source()
{
    int ret = 0;

    PAL_DBG(LOG_TAG, "Open A2DP source start");
    if (bt_lib_source_handle && (audio_source_open_api ||
        audio_source_open)) {
        if (a2dpState == A2DP_STATE_DISCONNECTED) {
            PAL_DBG(LOG_TAG, "calling BT stream open");
            /*To support backward compatibility check for BT IPC API's
             * with session_type or w/o session_type*/
            if (audio_source_open_api) {
                ret = audio_source_open_api(get_session_type());
                if (ret != 0) {
                    PAL_ERR(LOG_TAG, "Failed to open source stream for a2dp: status %d", ret);
                    // SS_A2DP_FEATURE
                    //return;
                }
            } else {
                ret = audio_source_open();
                if (ret != 0) {
                    PAL_ERR(LOG_TAG, "Failed to open source stream for a2dp: status %d", ret);
                    // SS_A2DP_FEATURE
                    //return;
                }
            }
            a2dpState = A2DP_STATE_CONNECTED;
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
            audio_get_dynamic_bitrate(&bitrate_cb);
            audio_get_peer_mtu(&peerMtu_cb);
#endif
        } else {
            PAL_DBG(LOG_TAG, "Called a2dp open with improper state %d", a2dpState);
        }
    }
}

int BtA2dp::close_audio_source()
{
    PAL_VERBOSE(LOG_TAG, "Enter");

    if (!(bt_lib_source_handle && (audio_source_close_api ||
        audio_source_close))) {
        PAL_ERR(LOG_TAG, "a2dp source handle is not identified, Ignoring close request");
        return -ENOSYS;
    }

    if (a2dpState != A2DP_STATE_DISCONNECTED) {
        PAL_DBG(LOG_TAG, "calling BT source stream close");
        mDeviceMutex.lock();
        if (audio_source_close_api) {
            if (audio_source_close_api(get_session_type()) != 0) // SS_BT_LEA : QC Bug fix, bool to int
                PAL_ERR(LOG_TAG, "failed close a2dp source control path from BT library");
        } else {
            if (audio_source_close() != 0) // SS_BT_LEA : QC Bug fix, bool to int
                PAL_ERR(LOG_TAG, "failed close a2dp source control path from BT library");
        }
        mDeviceMutex.unlock();
    }
    totalActiveSessionRequests = 0;
    param_bt_a2dp.latency = 0;
#if defined(SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD) && defined(SEC_AUDIO_BLE_OFFLOAD)
    param_bt_a2dp.a2dp_suspended_for_ble = false;
#endif
    a2dpState = A2DP_STATE_DISCONNECTED;
    isConfigured = false;
///* SS_A2DP_FEATURE
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    isLC3MonoModeOn = false; // SS_BT_LEA - MB_003 : Reset mono mode on close source instead of stop playback
    if (is_a2dp_offload()) {
        audio_get_dynamic_bitrate(NULL);
        audio_get_peer_mtu(NULL);
    }
#endif
//*/
    return 0;
}

void BtA2dp::init_a2dp_source()
{
    PAL_DBG(LOG_TAG, "init_a2dp_source START");
    if (bt_lib_source_handle == nullptr) {
        PAL_DBG(LOG_TAG, "Requesting for BT lib handle");
        bt_lib_source_handle = dlopen(BT_IPC_SOURCE_LIB, RTLD_NOW);
        if (bt_lib_source_handle == nullptr) {
            PAL_ERR(LOG_TAG, "dlopen failed for %s", BT_IPC_SOURCE_LIB);
            return;
        }
    }
    bt_audio_pre_init = (bt_audio_pre_init_t)
                  dlsym(bt_lib_source_handle, "bt_audio_pre_init");
    audio_source_open_api = (audio_source_open_api_t)
                  dlsym(bt_lib_source_handle, "audio_stream_open_api");
    audio_source_start_api = (audio_source_start_api_t)
                  dlsym(bt_lib_source_handle, "audio_start_stream_api");
    audio_get_enc_config_api = (audio_get_enc_config_api_t)
                  dlsym(bt_lib_source_handle, "audio_get_codec_config_api");
    audio_source_suspend_api = (audio_source_suspend_api_t)
                  dlsym(bt_lib_source_handle, "audio_suspend_stream_api");
    audio_source_handoff_triggered = (audio_source_handoff_triggered_t)
                  dlsym(bt_lib_source_handle, "audio_handoff_triggered");
    clear_source_a2dpsuspend_flag = (clear_source_a2dpsuspend_flag_t)
                  dlsym(bt_lib_source_handle, "clear_a2dpsuspend_flag");
    audio_source_stop_api = (audio_source_stop_api_t)
                  dlsym(bt_lib_source_handle, "audio_stop_stream_api");
    audio_source_close_api = (audio_source_close_api_t)
                  dlsym(bt_lib_source_handle, "audio_stream_close_api");
    audio_source_check_a2dp_ready_api = (audio_source_check_a2dp_ready_api_t)
                  dlsym(bt_lib_source_handle, "audio_check_a2dp_ready_api");
    audio_sink_get_a2dp_latency_api = (audio_sink_get_a2dp_latency_api_t)
                  dlsym(bt_lib_source_handle, "audio_sink_get_a2dp_latency_api");
    audio_is_tws_mono_mode_enable = (audio_is_tws_mono_mode_enable_t)
                  dlsym(bt_lib_source_handle, "isTwsMonomodeEnable");
    audio_is_scrambling_enabled = (audio_is_scrambling_enabled_t)
                  dlsym(bt_lib_source_handle, "audio_is_scrambling_enabled");
    btoffload_update_metadata_api = (btoffload_update_metadata_api_t)
                  dlsym(bt_lib_source_handle, "update_metadata");
    audio_source_get_supported_latency_modes_api = (audio_source_get_supported_latency_modes_api_t)
                  dlsym(bt_lib_source_handle, "audio_stream_get_supported_latency_modes_api");
    audio_source_set_latency_mode_api = (audio_source_set_latency_mode_api_t)
                  dlsym(bt_lib_source_handle, "audio_stream_set_latency_mode_api");

    audio_source_open = (audio_source_open_t)
        dlsym(bt_lib_source_handle, "audio_stream_open");
    audio_source_start = (audio_source_start_t)
        dlsym(bt_lib_source_handle, "audio_start_stream");
    audio_get_enc_config = (audio_get_enc_config_t)
        dlsym(bt_lib_source_handle, "audio_get_codec_config");
    audio_source_suspend = (audio_source_suspend_t)
        dlsym(bt_lib_source_handle, "audio_suspend_stream");

    audio_source_stop = (audio_source_stop_t)
        dlsym(bt_lib_source_handle, "audio_stop_stream");
    audio_source_close = (audio_source_close_t)
        dlsym(bt_lib_source_handle, "audio_stream_close");
    audio_source_check_a2dp_ready = (audio_source_check_a2dp_ready_t)
        dlsym(bt_lib_source_handle, "audio_check_a2dp_ready");
    audio_sink_get_a2dp_latency = (audio_sink_get_a2dp_latency_t)
        dlsym(bt_lib_source_handle, "audio_sink_get_a2dp_latency");
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    audio_get_dynamic_bitrate = (audio_get_dynamic_bitrate_t)
                    dlsym(bt_lib_source_handle, "register_bitrate_changed_cb");
    audio_get_peer_mtu = (audio_get_peer_mtu_t)
                    dlsym(bt_lib_source_handle, "register_mtu_changed_cb");
    if (audio_get_dynamic_bitrate == nullptr) {
        PAL_DBG(LOG_TAG, "audio_get_dynamic_bitrate null");
    }
    if (audio_get_peer_mtu == nullptr) {
        PAL_DBG(LOG_TAG, "audio_get_peer_mtu null");
    }

    delay_report = 0;
    use_offload_hal = false;
    peerMtu = 0;
#endif
    if (bt_lib_source_handle && bt_audio_pre_init) {
        PAL_DBG(LOG_TAG, "calling BT module preinit");
        bt_audio_pre_init();
    }
}

void BtA2dp::init_a2dp_sink()
{
    PAL_DBG(LOG_TAG, "Open A2DP input start");
    if (bt_lib_sink_handle == nullptr) {
        PAL_DBG(LOG_TAG, "Requesting for BT lib handle");
        bt_lib_sink_handle = dlopen(BT_IPC_SINK_LIB, RTLD_NOW);

        if (bt_lib_sink_handle == nullptr) {
#ifndef LINUX_ENABLED
            // On Mobile LE VoiceBackChannel implemented as A2DPSink Profile.
            // However - All the BT-Host IPC calls are exposed via Source LIB itself.
            PAL_DBG(LOG_TAG, "Requesting for BT lib source handle");
            bt_lib_sink_handle = dlopen(BT_IPC_SOURCE_LIB, RTLD_NOW);
            if (bt_lib_sink_handle == nullptr) {
                PAL_ERR(LOG_TAG, "DLOPEN failed");
                return;
            }
            audio_get_enc_config_api = (audio_get_enc_config_api_t)
                  dlsym(bt_lib_sink_handle, "audio_get_codec_config_api");
            audio_sink_get_a2dp_latency_api = (audio_sink_get_a2dp_latency_api_t)
                dlsym(bt_lib_sink_handle, "audio_sink_get_a2dp_latency_api");
            audio_sink_start_api = (audio_sink_start_api_t)
                  dlsym(bt_lib_sink_handle, "audio_start_stream_api");
            audio_sink_stop_api = (audio_sink_stop_api_t)
                  dlsym(bt_lib_sink_handle, "audio_stop_stream_api");
            audio_source_check_a2dp_ready_api = (audio_source_check_a2dp_ready_api_t)
                  dlsym(bt_lib_sink_handle, "audio_check_a2dp_ready_api");
            audio_sink_suspend_api = (audio_sink_suspend_api_t)
                dlsym(bt_lib_sink_handle, "audio_suspend_stream_api");
            audio_sink_open_api = (audio_sink_open_api_t)
                dlsym(bt_lib_sink_handle, "audio_stream_open_api");
            audio_sink_close_api = (audio_sink_close_api_t)
                dlsym(bt_lib_sink_handle, "audio_stream_close_api");
            btoffload_update_metadata_api = (btoffload_update_metadata_api_t)
                  dlsym(bt_lib_sink_handle, "update_metadata");

            audio_get_enc_config = (audio_get_enc_config_t)
                dlsym(bt_lib_sink_handle, "audio_get_codec_config");
            audio_sink_get_a2dp_latency = (audio_sink_get_a2dp_latency_t)
                dlsym(bt_lib_sink_handle, "audio_sink_get_a2dp_latency");
            audio_sink_start = (audio_sink_start_t)
                dlsym(bt_lib_sink_handle, "audio_start_stream");
            audio_sink_stop = (audio_sink_stop_t)
                dlsym(bt_lib_sink_handle, "audio_stop_stream");
            audio_source_check_a2dp_ready = (audio_source_check_a2dp_ready_t)
                dlsym(bt_lib_sink_handle, "audio_check_a2dp_ready");
            audio_sink_suspend = (audio_sink_suspend_t)
                dlsym(bt_lib_sink_handle, "audio_suspend_stream");
            audio_sink_open = (audio_sink_open_t)
                dlsym(bt_lib_sink_handle, "audio_stream_open");
            audio_sink_close = (audio_sink_close_t)
                dlsym(bt_lib_sink_handle, "audio_stream_close");
#else
            // On Linux Builds - A2DP Sink Profile is supported via different lib
            PAL_ERR(LOG_TAG, "DLOPEN failed for %s", BT_IPC_SINK_LIB);
#endif
        } else {
            audio_sink_start = (audio_sink_start_t)
                          dlsym(bt_lib_sink_handle, "audio_sink_start_capture");
            audio_get_dec_config = (audio_get_dec_config_t)
                          dlsym(bt_lib_sink_handle, "audio_get_decoder_config");
            audio_sink_stop = (audio_sink_stop_t)
                          dlsym(bt_lib_sink_handle, "audio_sink_stop_capture");
            audio_sink_check_a2dp_ready = (audio_sink_check_a2dp_ready_t)
                          dlsym(bt_lib_sink_handle, "audio_sink_check_a2dp_ready");
            audio_sink_session_setup_complete = (audio_sink_session_setup_complete_t)
                          dlsym(bt_lib_sink_handle, "audio_sink_session_setup_complete");
        }
    }

#ifndef LINUX_ENABLED
    isDummySink = true;
#endif

}

void BtA2dp::open_a2dp_sink()
{
    int ret = 0;

    PAL_DBG(LOG_TAG, "Open A2DP sink start");
    if (bt_lib_sink_handle && (audio_sink_open_api ||
        audio_sink_open)) {
        if (a2dpState == A2DP_STATE_DISCONNECTED) {
            PAL_DBG(LOG_TAG, "calling BT stream open");
            if (audio_sink_open_api) {
                ret = audio_sink_open_api(get_session_type());
                if (ret != 0) {
                    PAL_ERR(LOG_TAG, "Failed to open sink stream for a2dp: status %d", ret);
                }
            } else {
                ret = audio_sink_open();
                if (ret != 0) {
                    PAL_ERR(LOG_TAG, "Failed to open sink stream for a2dp: status %d", ret);
                }
            }
            a2dpState = A2DP_STATE_CONNECTED;
        }
        else {
            PAL_DBG(LOG_TAG, "Called a2dp open with improper state %d", a2dpState);
        }
    }
}

int BtA2dp::close_audio_sink()
{
    PAL_VERBOSE(LOG_TAG, "Enter");

    if (!(bt_lib_sink_handle && (audio_sink_close_api ||
        audio_sink_close))) {
        PAL_ERR(LOG_TAG, "a2dp sink handle is not identified, Ignoring close request");
        return -ENOSYS;
    }

    if (a2dpState != A2DP_STATE_DISCONNECTED) {
        PAL_DBG(LOG_TAG, "calling BT sink stream close");
        mDeviceMutex.lock();
        if (audio_sink_close_api) {
            if (audio_sink_close_api(get_session_type()) == false)
                PAL_ERR(LOG_TAG, "failed close a2dp sink control path from BT library");
        } else {
            if (audio_sink_close() == false)
                PAL_ERR(LOG_TAG, "failed close a2dp sink control path from BT library");
        }
        mDeviceMutex.unlock();
    }
    totalActiveSessionRequests = 0;
    param_bt_a2dp.latency = 0;
#if defined(SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD) && defined(SEC_AUDIO_BLE_OFFLOAD)
    param_bt_a2dp.a2dp_suspended_for_ble = false;
#endif
    a2dpState = A2DP_STATE_DISCONNECTED;
    isConfigured = false;

    return 0;
}

bool BtA2dp::a2dp_send_sink_setup_complete()
{
    uint64_t system_latency = 0;
    bool is_complete = false;

    /* TODO : Replace this with call to plugin */
    system_latency = 200;

    if (audio_sink_session_setup_complete(system_latency) == 0) {
        is_complete = true;
    }
    return is_complete;
}

void BtA2dp::init()
{
    (a2dpRole == SOURCE) ? init_a2dp_source() : init_a2dp_sink();
}

int BtA2dp::start()
{
    int status = 0;
    mDeviceMutex.lock();

    status = (a2dpRole == SOURCE) ? startPlayback() : startCapture();
    if (status) {
        goto exit;
    }

    if (totalActiveSessionRequests == 1) {
        status = configureSlimbusClockSrc();
        if (status) {
            goto exit;
        }
    }

    status = Device::start_l();

    if (customPayload) {
        free(customPayload);
        customPayload = NULL;
        customPayloadSize = 0;
    }

    PAL_INFO(LOG_TAG, "isAbrEnabled is %d", isAbrEnabled); // SS_A2DP_FEATURE
    if (!status && isAbrEnabled)
        startAbr();
exit:
    mDeviceMutex.unlock();
    return status;
}

int BtA2dp::stop()
{
    int status = 0;

    mDeviceMutex.lock();
    PAL_INFO(LOG_TAG, "isAbrEnabled is %d", isAbrEnabled); // SS_A2DP_FEATURE
    if (isAbrEnabled)
        stopAbr();

    Device::stop_l();

    status = (a2dpRole == SOURCE) ? stopPlayback() : stopCapture();
    mDeviceMutex.unlock();

    return status;
}

int BtA2dp::startPlayback()
{
    int ret = 0;
    uint8_t multi_cast = 0, num_dev = 1;

    PAL_INFO(LOG_TAG, "a2dp_start_playback start"); // SS_A2DP_FEATURE

    if (!(bt_lib_source_handle && (audio_source_start_api ||
         audio_source_start) && (audio_get_enc_config_api ||
         audio_get_enc_config))) {
        PAL_ERR(LOG_TAG, "a2dp handle is not identified, Ignoring start playback request");
        return -ENOSYS;
    }

    if (param_bt_a2dp.a2dp_suspended) {
        // session will be restarted after suspend completion
        PAL_ERR(LOG_TAG, "a2dp start requested during suspend state");
        return -ENOSYS;
    } else if (a2dpState == A2DP_STATE_DISCONNECTED) {
        // update device status, if still disconnected, return error.
        if (!(rm->isDeviceAvailable(deviceAttr.id) &&
              checkDeviceStatus() != A2DP_STATE_DISCONNECTED)) {
            PAL_ERR(LOG_TAG, "a2dp start requested when a2dp source stream is failed to open");
            return -ENOSYS;
        }
    }

#if defined(SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD) && defined(SEC_AUDIO_BLE_OFFLOAD)
    if (is_a2dp_offload() && param_bt_a2dp.a2dp_suspended_for_ble) {
        // session will be restarted after suspend completion
        PAL_ERR(LOG_TAG, "a2dp start requested during ble force reconfig %d %d",
            is_a2dp_offload(), param_bt_a2dp.a2dp_suspended_for_ble);
        return -ENOSYS;
    }
#endif

#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    if (is_a2dp_offload() && use_offload_hal == false) {
        PAL_DBG(LOG_TAG, "a2dp start requested while legacy hal is using %d %d",
            is_a2dp_offload(), use_offload_hal);
        return -ENOSYS;
    }
#endif

    if (a2dpState != A2DP_STATE_STARTED && !totalActiveSessionRequests) {
        codecFormat = CODEC_TYPE_INVALID;

        if (!isConfigured)
            isAbrEnabled = false;

        PAL_INFO(LOG_TAG, "calling BT module stream start"); // SS_A2DP_FEATURE
        /* This call indicates BT IPC lib to start playback */
        if (audio_source_start_api) {
            ret = audio_source_start_api(get_session_type());
        } else {
            ret = audio_source_start();
        }
        if (ret != 0) {
            // TODO: CTRL_ACK_RECONFIGURATION needs retry design
            if (ret <= CTRL_SKT_DISCONNECTED && ret >= CTRL_ACK_FAILURE)
                ret = -ENODEV;
            PAL_ERR(LOG_TAG, "BT controller start failed");
            return ret;
#if defined(SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD) && defined(SEC_AUDIO_BLE_OFFLOAD)
        } else if (is_a2dp_offload() && param_bt_a2dp.a2dp_suspended_for_ble) {
            PAL_ERR(LOG_TAG, "BT controller start return = %d, but ble force reconfig", ret);
            return -ENOSYS;
#endif
        }
        PAL_INFO(LOG_TAG, "BT controller start return = %d", ret);

        if (audio_source_set_latency_mode_api) {
            ret = audio_source_set_latency_mode_api(get_session_type(), a2dpLatencyMode);
            if (ret) {
                PAL_DBG(LOG_TAG, "Warning: Set latency mode failed for value %d with exit status %d", a2dpLatencyMode, ret);
                ret = 0;
            }
        }

        PAL_INFO(LOG_TAG, "configure_a2dp_encoder_format start"); // SS_A2DP_FEATURE
        if (audio_get_enc_config_api) {
            codecInfo = audio_get_enc_config_api(get_session_type(), &multi_cast, &num_dev, (audio_format_t*)&codecFormat);
        }
        else {
            codecInfo = audio_get_enc_config(&multi_cast, &num_dev, (audio_format_t*)&codecFormat);
        }

        if (codecInfo == NULL || codecFormat == CODEC_TYPE_INVALID) {
            PAL_ERR(LOG_TAG, "invalid encoder config");
            if (audio_source_stop_api) {
                audio_source_stop_api(get_session_type());
            } else {
                audio_source_stop();
            }
            return -EINVAL;
        }

        if (codecFormat == CODEC_TYPE_APTX_DUAL_MONO && audio_is_tws_mono_mode_enable)
            isTwsMonoModeOn = audio_is_tws_mono_mode_enable();

        if (audio_is_scrambling_enabled)
            isScramblingEnabled = audio_is_scrambling_enabled();
        PAL_INFO(LOG_TAG, "isScramblingEnabled = %d", isScramblingEnabled);

        /* Update Device GKV based on Encoder type */
        updateDeviceMetadata();
        if (!isConfigured) {
            ret = configureGraphModules();
            if (ret) {
                PAL_ERR(LOG_TAG, "unable to configure DSP encoder");
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
                /* WA for avoiding Buds disconnection after call termination */
                if (is_a2dp_offload()) {
                    PAL_ERR(LOG_TAG, "do sleep!");
                    usleep(300000);
                }
#endif
                if (audio_source_stop_api) {
                    audio_source_stop_api(get_session_type());
                } else {
                    audio_source_stop();
                }
                return ret;
            }
        }

#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
        if (is_a2dp_offload()) {
            if (pluginCodec) {
                codecLatency = pluginCodec->plugin_get_codec_latency(pluginCodec);
                param_bt_a2dp.latency = getLatency(delay_report);
                if (delay_report > 0) {
                    param_bt_a2dp.latency = calculate_latency(param_bt_a2dp.latency, delay_report);
                }
            } else {
                param_bt_a2dp.latency = 0;
            }
        }
#endif
        /* SS_A2DP_FEATURE
        if (pluginCodec) {
            codecLatency = pluginCodec->plugin_get_codec_latency(pluginCodec);
        }*/

        a2dpState = A2DP_STATE_STARTED;
    } else {
        /* Update Device GKV based on Already received encoder. */
        /* This is required for getting tagged module info in session class. */
        updateDeviceMetadata();
    }

    totalActiveSessionRequests++;
    PAL_INFO(LOG_TAG, "start A2DP playback total active sessions :%d", // SS_A2DP_FEATURE
            totalActiveSessionRequests);
#if defined(SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD) && defined(QCA_OFFLOAD)
    if (is_a2dp_offload() && game_mode == true) {
        PAL_INFO(LOG_TAG, "Set game mode");
        bitrate_cb(SSC_BITRATE_GAME_MODE_ON);
    }
#endif
    return ret;
}

int BtA2dp::stopPlayback()
{
    int ret =0;

    PAL_VERBOSE(LOG_TAG, "a2dp_stop_playback start");
    if (!(bt_lib_source_handle && (audio_source_stop_api ||
        audio_source_stop))) {
        PAL_ERR(LOG_TAG, "a2dp handle is not identified, Ignoring stop request");
        return -ENOSYS;
    }

    if (totalActiveSessionRequests > 0)
        totalActiveSessionRequests--;
    else
        PAL_ERR(LOG_TAG, "No active playback session requests on A2DP");

    if (a2dpState == A2DP_STATE_STARTED && !totalActiveSessionRequests) {
        PAL_VERBOSE(LOG_TAG, "calling BT module stream stop");
        if (audio_source_stop_api) {
            ret = audio_source_stop_api(get_session_type());
        } else {
            ret = audio_source_stop();
        }

        if (ret < 0) {
            PAL_ERR(LOG_TAG, "stop stream to BT IPC lib failed");
        } else {
            PAL_VERBOSE(LOG_TAG, "stop steam to BT IPC lib successful");
        }

        if (deviceStartStopCount == 0) {
            isConfigured = false;
        }
        a2dpState = A2DP_STATE_STOPPED;
        a2dpLatencyMode = AUDIO_LATENCY_MODE_FREE;
        codecInfo = NULL;
        param_bt_a2dp.latency = 0;
        codecLatency = 0;

        /* Reset isTwsMonoModeOn and isLC3MonoModeOn during stop */
        if (!param_bt_a2dp.a2dp_suspended) {
            isTwsMonoModeOn = false;
            isLC3MonoModeOn = false;
            isScramblingEnabled = false;
        }

        if (pluginCodec) {
            pluginCodec->close_plugin(pluginCodec);
            pluginCodec = NULL;
        }
        if (pluginHandler) {
            dlclose(pluginHandler);
            pluginHandler = NULL;
        }
    }

    PAL_INFO(LOG_TAG, "Stop A2DP playback, total active sessions :%d", // SS_A2DP_FEATURE
            totalActiveSessionRequests);
    return 0;
}

bool BtA2dp::isDeviceReady()
{
    bool ret = false;

    if (a2dpRole == SOURCE) {
        if (param_bt_a2dp.a2dp_suspended)
            return ret;
    } else if (a2dpRole == SINK) {
        if (param_bt_a2dp.a2dp_capture_suspended)
            return ret;
    }

    if ((a2dpState != A2DP_STATE_DISCONNECTED) &&
        (isA2dpOffloadSupported)) {
        if ((a2dpRole == SOURCE) || isDummySink) {
            if (audio_source_check_a2dp_ready_api) {
                ret = audio_source_check_a2dp_ready_api(get_session_type());
            } else {
                ret = audio_source_check_a2dp_ready();
            }
        } else {
            if (audio_sink_check_a2dp_ready)
                ret = audio_sink_check_a2dp_ready();
        }
    }
    return ret;
}

int BtA2dp::startCapture()
{
    int ret = 0;

    PAL_DBG(LOG_TAG, "a2dp_start_capture start");

    if (!isDummySink) {
        if (!(bt_lib_sink_handle && (audio_sink_start_api ||
            audio_sink_start) && audio_get_dec_config)) {
            PAL_ERR(LOG_TAG, "a2dp handle is not identified, Ignoring start capture request");
            return -ENOSYS;
        }

        if (a2dpState != A2DP_STATE_STARTED  && !totalActiveSessionRequests) {
            codecFormat = CODEC_TYPE_INVALID;
            PAL_DBG(LOG_TAG, "calling BT module stream start");
            /* This call indicates BT IPC lib to start capture */
            if (audio_sink_start_api) {
                ret = audio_sink_start_api(get_session_type());
            } else {
                ret = audio_sink_start();
            }

            PAL_INFO(LOG_TAG, "BT controller start capture return = %d",ret);
            if (ret != 0 ) {
                // TODO: CTRL_ACK_RECONFIGURATION needs retry design
                if (ret <= CTRL_SKT_DISCONNECTED && ret >= CTRL_ACK_FAILURE)
                    ret = -ENODEV;
                PAL_ERR(LOG_TAG, "BT controller start capture failed");
                return ret;
            }

            codecInfo = audio_get_dec_config((audio_format_t *)&codecFormat);
            if (codecInfo == NULL || codecFormat == CODEC_TYPE_INVALID) {
                PAL_ERR(LOG_TAG, "invalid codec config");
                if (audio_sink_stop_api) {
                    audio_sink_stop_api(get_session_type());
                } else {
                    audio_sink_stop();
                }
                return -EINVAL;
            }

            /* Update Device GKV based on Decoder type */
            updateDeviceMetadata();

            ret = configureGraphModules();
            if (ret) {
                PAL_ERR(LOG_TAG, "unable to configure DSP decoder");
                if (audio_sink_stop_api) {
                    audio_sink_stop_api(get_session_type());
                } else {
                    audio_sink_stop();
                }
                return ret;
            }
        }
    } else {
        uint8_t multi_cast = 0, num_dev = 1;

        if (!(bt_lib_sink_handle && (audio_sink_start_api ||
            audio_sink_start) && (audio_get_enc_config_api ||
            audio_get_enc_config))) {
            PAL_ERR(LOG_TAG, "a2dp handle is not identified, Ignoring start capture request");
            return -ENOSYS;
        }

        if (param_bt_a2dp.a2dp_capture_suspended) {
            // session will be restarted after suspend completion
            PAL_INFO(LOG_TAG, "a2dp start capture requested during suspend state");
            return -EINVAL;
        }

        if (a2dpState != A2DP_STATE_STARTED  && !totalActiveSessionRequests) {
            codecFormat = CODEC_TYPE_INVALID;
            PAL_DBG(LOG_TAG, "calling BT module stream start");
            /* This call indicates BT IPC lib to start */
            if (audio_sink_start_api) {
                ret = audio_sink_start_api(get_session_type());
            }
            else {
                ret = audio_sink_start();
            }

            PAL_INFO(LOG_TAG, "BT controller start return = %d",ret);
            if (ret != 0 ) {
                // TODO: CTRL_ACK_RECONFIGURATION needs retry design
                if (ret <= CTRL_SKT_DISCONNECTED && ret >= CTRL_ACK_FAILURE)
                    ret = -ENODEV;
                PAL_ERR(LOG_TAG, "BT controller start failed");
                return ret;
            }

            if (audio_get_enc_config_api) {
                codecInfo = audio_get_enc_config_api(get_session_type(), &multi_cast, &num_dev, (audio_format_t*)&codecFormat);
            } else {
                codecInfo = audio_get_enc_config(&multi_cast, &num_dev, (audio_format_t*)&codecFormat);
            }

            if (codecInfo == NULL || codecFormat == CODEC_TYPE_INVALID) {
                PAL_ERR(LOG_TAG, "invalid codec config");
                if (audio_sink_stop_api) {
                    audio_sink_stop_api(get_session_type());
                } else {
                    audio_sink_stop();
                }
                return -EINVAL;
            }

            /* Update Device GKV based on Decoder type */
            updateDeviceMetadata();

            ret = configureGraphModules();
            if (ret) {
                PAL_ERR(LOG_TAG, "unable to configure DSP decoder");
                if (audio_sink_stop_api) {
                    audio_sink_stop_api(get_session_type());
                } else {
                    audio_sink_stop();
                }
                return ret;
            }
        }
    }

    if (!isDummySink) {
        if (!a2dp_send_sink_setup_complete()) {
            PAL_ERR(LOG_TAG, "sink_setup_complete not successful");
            if (audio_sink_stop_api) {
                audio_sink_stop_api(get_session_type());
            } else {
                audio_sink_stop();
            }
            ret = -ETIMEDOUT;
        }
    }

    a2dpState = A2DP_STATE_STARTED;
    totalActiveSessionRequests++;

    PAL_DBG(LOG_TAG, "start A2DP sink total active sessions :%d",
                      totalActiveSessionRequests);
    return ret;
}

int BtA2dp::stopCapture()
{
    int ret =0;

    PAL_DBG(LOG_TAG, "a2dp_stop_capture start"); // SS_A2DP_FEATURE
    if (!(bt_lib_sink_handle && (audio_sink_stop_api ||
         audio_sink_stop))) {
        PAL_ERR(LOG_TAG, "a2dp handle is not identified, Ignoring stop request");
        return -ENOSYS;
    }

    if (totalActiveSessionRequests > 0)
        totalActiveSessionRequests--;

    if (!totalActiveSessionRequests) {
        PAL_VERBOSE(LOG_TAG, "calling BT module stream stop");
        isConfigured = false;
        if (audio_sink_stop_api) {
            ret = audio_sink_stop_api(get_session_type());
        } else {
            ret = audio_sink_stop();
        }

        if (ret < 0) {
            PAL_ERR(LOG_TAG, "stop stream to BT IPC lib failed");
        } else {
            PAL_VERBOSE(LOG_TAG, "stop steam to BT IPC lib successful");
        }

        // It can be in A2DP_STATE_DISCONNECTED, if device disconnect happens prior to Stop.
        if (a2dpState == A2DP_STATE_STARTED)
            a2dpState = A2DP_STATE_STOPPED;

        param_bt_a2dp.latency = 0;

        if (pluginCodec) {
            pluginCodec->close_plugin(pluginCodec);
            pluginCodec = NULL;
        }
        if (pluginHandler) {
            dlclose(pluginHandler);
            pluginHandler = NULL;
        }
    }
    PAL_DBG(LOG_TAG, "Stop A2DP capture, total active sessions :%d",
            totalActiveSessionRequests);
    return 0;
}

int32_t BtA2dp::setDeviceParameter(uint32_t param_id, void *param)
{
    int32_t status = 0;
    pal_param_bta2dp_t* param_a2dp = (pal_param_bta2dp_t *)param;
    bool skip_switch = false;

    if (isA2dpOffloadSupported == false) {
       PAL_VERBOSE(LOG_TAG, "no supported encoders identified,ignoring a2dp setparam");
       status = -EINVAL;
       goto exit;
    }

    switch(param_id) {
    case PAL_PARAM_ID_DEVICE_CONNECTION:
    {
        pal_param_device_connection_t *device_connection =
            (pal_param_device_connection_t *)param;
        if (device_connection->connection_state == true) {
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
            if (is_a2dp_offload() && !device_connection->is_bt_offload_enabled) {
                PAL_DBG(LOG_TAG, "DEVICE_CONNECTION %d %d",
                    is_a2dp_offload(), device_connection->is_bt_offload_enabled);
                goto exit;
            }
#endif
            if (a2dpRole == SOURCE)
                open_a2dp_source();

            else {
#ifdef A2DP_SINK_SUPPORTED

                open_a2dp_sink();
#else
                a2dpState = A2DP_STATE_CONNECTED;
#endif
            }
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
            if (is_a2dp_offload()) {
                use_offload_hal = true;
            }
#endif
        } else {
            if (a2dpRole == SOURCE) {
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
                if (is_a2dp_offload()) {
                    if (!use_offload_hal) {
                        // SS skip to call close_audio_source(), so need to reset suspend flag before break
                        // suspend t -> disconnect(a2dp hal) -> suspend f -> connect(primary hal) case
                        param_bt_a2dp.a2dp_suspended = false;
                        break;
                    }
                    use_offload_hal = false;
#ifdef QCA_OFFLOAD
                    game_mode = false;
#endif
                }
#endif
                status = close_audio_source();
            } else {
#ifdef A2DP_SINK_SUPPORTED
                status = close_audio_sink();
#else
                totalActiveSessionRequests = 0;
                param_bt_a2dp.latency = 0;
                a2dpState = A2DP_STATE_DISCONNECTED;
#endif
            }
        }
        break;
    }
    case PAL_PARAM_ID_BT_A2DP_RECONFIG:
    {
        if (a2dpState != A2DP_STATE_DISCONNECTED) {
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
            if (is_a2dp_offload() && !(isA2dpOffloadSupported && use_offload_hal)) {
                PAL_DBG(LOG_TAG, "A2DP_RECONFIG %d %d %d", is_a2dp_offload(), isA2dpOffloadSupported, use_offload_hal);
                goto exit;
            }
#endif
            param_bt_a2dp.reconfig = param_a2dp->reconfig;
        }
        break;
    }
    case PAL_PARAM_ID_BT_A2DP_SUSPENDED:
    {
        if (bt_lib_source_handle == nullptr)
            goto exit;

        param_bt_a2dp.is_suspend_setparam = param_a2dp->is_suspend_setparam;

        if (param_bt_a2dp.a2dp_suspended == param_a2dp->a2dp_suspended)
            goto exit;

        if (param_a2dp->a2dp_suspended == true) {
            param_bt_a2dp.a2dp_suspended = true;
            if (a2dpState == A2DP_STATE_DISCONNECTED)
                goto exit;
            if (ResourceManager::isDummyDevEnabled) {
                status = rm->a2dpSuspendToDummy(param_a2dp->dev_id);
            } else {
                status = rm->a2dpSuspend(param_a2dp->dev_id);
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
                if (is_a2dp_offload()) {
#ifndef QCA_OFFLOAD
                    set_a2dp_suspend(1);
#endif
                }
#endif
            }
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
            if (a2dpState == A2DP_STATE_DISCONNECTED) {
                PAL_INFO(LOG_TAG, "setDeviceParameter: a2dp is already disconnected");
                goto exit;
            }
#endif            
            if (audio_source_suspend_api)
                audio_source_suspend_api(get_session_type());
            else
                audio_source_suspend();
        } else {
            param_bt_a2dp.a2dp_suspended = false;
            if (a2dpState == A2DP_STATE_DISCONNECTED)
                goto exit;
            if (clear_source_a2dpsuspend_flag)
                clear_source_a2dpsuspend_flag();

            if (totalActiveSessionRequests > 0) {
                if (audio_source_start_api) {
                    status = audio_source_start_api(get_session_type());
                } else {
                    status = audio_source_start();
                }
                if (status) {
                    PAL_ERR(LOG_TAG, "BT controller start failed");
                    goto exit;
                }
#if defined(SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD) && !defined(QCA_OFFLOAD)
                else if (is_a2dp_offload()) {
                    set_a2dp_suspend(0);
                }
#endif
            }

            if (param_a2dp->is_suspend_setparam && param_a2dp->is_in_call)
                skip_switch = true;

            if (!skip_switch) {
                if (ResourceManager::isDummyDevEnabled) {
                    status = rm->a2dpResumeFromDummy(param_a2dp->dev_id);
                } else {
                    status = rm->a2dpResume(param_a2dp->dev_id);
                }
            }
        }
        break;
    }
#if defined(SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD) && defined(SEC_AUDIO_BLE_OFFLOAD)
    case PAL_PARAM_ID_BT_A2DP_SUSPENDED_FOR_BLE:
    {
        PAL_DBG(LOG_TAG, "setDeviceParameter: a2dp_suspended_for_ble %d", param_a2dp->a2dp_suspended_for_ble);

        if (param_bt_a2dp.a2dp_suspended_for_ble == param_a2dp->a2dp_suspended_for_ble)
            goto exit;

        int status = 0;

        if (param_a2dp->a2dp_suspended_for_ble == true) {
            param_bt_a2dp.a2dp_suspended_for_ble = true;

            if (a2dpState == A2DP_STATE_DISCONNECTED)
                goto exit;
            status = rm->a2dpSuspend(PAL_DEVICE_OUT_BLUETOOTH_A2DP);
#ifndef QCA_OFFLOAD
            set_a2dp_suspend(1);
#endif
            if (audio_source_suspend_api) {
                audio_source_suspend_api(A2DP_HARDWARE_OFFLOAD_DATAPATH);
            } else {
                 audio_source_suspend();
            }
        } else {
            if (clear_source_a2dpsuspend_flag) {
                clear_source_a2dpsuspend_flag();
            }
            param_bt_a2dp.a2dp_suspended_for_ble = false;

            if (totalActiveSessionRequests > 0) {
                if (audio_source_start_api) {
                    status = audio_source_start_api(A2DP_HARDWARE_OFFLOAD_DATAPATH);
                } else {
                    status = audio_source_start();
                }
                if (status) {
                    PAL_ERR(LOG_TAG, "BT controller start failed");
                    goto exit;
                }
#ifndef QCA_OFFLOAD
                else {
                    set_a2dp_suspend(0);
                }
#endif
            }
            status = rm->a2dpResume(PAL_DEVICE_OUT_BLUETOOTH_A2DP);
        }
    }
#endif
    case PAL_PARAM_ID_BT_A2DP_TWS_CONFIG:
    {
        isTwsMonoModeOn = param_a2dp->is_tws_mono_mode_on;
        if (a2dpState == A2DP_STATE_STARTED) {
            std::shared_ptr<Device> dev = nullptr;
            Stream *stream = NULL;
            Session *session = NULL;
            std::vector<Stream*> activestreams;
            pal_bt_tws_payload param_tws;

            dev = Device::getInstance(&deviceAttr, rm);
            status = rm->getActiveStream_l(activestreams, dev);
            if ((0 != status) || (activestreams.size() == 0)) {
                PAL_ERR(LOG_TAG, "no active stream available");
                return -EINVAL;
            }
            stream = static_cast<Stream *>(activestreams[0]);
            stream->getAssociatedSession(&session);
            param_tws.isTwsMonoModeOn = isTwsMonoModeOn;
            param_tws.codecFormat = (uint32_t)codecFormat;
            session->setParameters(stream, BT_PLACEHOLDER_ENCODER, param_id, &param_tws);
        }
        break;
    }
    case PAL_PARAM_ID_BT_A2DP_LC3_CONFIG:
    {
        isLC3MonoModeOn = param_a2dp->is_lc3_mono_mode_on;
        if (a2dpState == A2DP_STATE_STARTED) {
            std::shared_ptr<Device> dev = nullptr;
            Stream *stream = NULL;
            Session *session = NULL;
            std::vector<Stream*> activestreams;
            pal_bt_lc3_payload param_lc3;

            dev = Device::getInstance(&deviceAttr, rm);
            status = rm->getActiveStream_l(activestreams, dev);
            if ((0 != status) || (activestreams.size() == 0)) {
                PAL_ERR(LOG_TAG, "no active stream available");
                return -EINVAL;
            }
            stream = static_cast<Stream *>(activestreams[0]);
            stream->getAssociatedSession(&session);
            param_lc3.isLC3MonoModeOn = isLC3MonoModeOn;
            session->setParameters(stream, BT_PLACEHOLDER_ENCODER, param_id, &param_lc3);
        }
        break;
    }
    case PAL_PARAM_ID_BT_A2DP_CAPTURE_SUSPENDED:
    {
        if (bt_lib_sink_handle == nullptr)
            goto exit;

        param_bt_a2dp.is_suspend_setparam = param_a2dp->is_suspend_setparam;

        if (param_bt_a2dp.a2dp_capture_suspended == param_a2dp->a2dp_capture_suspended)
            goto exit;

        if (param_a2dp->a2dp_capture_suspended == true) {
            param_bt_a2dp.a2dp_capture_suspended = true;
            if (a2dpState == A2DP_STATE_DISCONNECTED)
                goto exit;

            if (ResourceManager::isDummyDevEnabled) {
                rm->a2dpCaptureSuspendToDummy(param_a2dp->dev_id);
            } else {
                rm->a2dpCaptureSuspend(param_a2dp->dev_id);
            }
            if (audio_sink_suspend_api)
                audio_sink_suspend_api(get_session_type());
            else
                audio_sink_suspend();
        } else {
            param_bt_a2dp.a2dp_capture_suspended = false;
            if (a2dpState == A2DP_STATE_DISCONNECTED)
                goto exit;

            if (clear_source_a2dpsuspend_flag)
                clear_source_a2dpsuspend_flag();

            if (totalActiveSessionRequests > 0) {
                if (audio_sink_start_api) {
                    status = audio_sink_start_api(get_session_type());
                } else {
                    status = audio_sink_start();
                }

                if (status) {
                    PAL_ERR(LOG_TAG, "BT controller start failed");
                    goto exit;
                }
            }

            if (param_a2dp->is_suspend_setparam && param_a2dp->is_in_call)
                skip_switch = true;

            if (!skip_switch) {
                if (ResourceManager::isDummyDevEnabled) {
                    rm->a2dpCaptureResumeFromDummy(param_a2dp->dev_id);
                } else {
                    rm->a2dpCaptureResume(param_a2dp->dev_id);
                }
            }
        }
        break;
    }
    case PAL_PARAM_ID_SET_SINK_METADATA:
    {
        if (btoffload_update_metadata_api) {
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
            sink_metadata *metadata = (sink_metadata *) param;
            if (metadata != NULL && metadata->tracks != NULL) {
                PAL_INFO(LOG_TAG, "audio_source = %d", metadata->tracks->source);
                audio_source = metadata->tracks->source;
            }
#endif
            PAL_INFO(LOG_TAG, "sending sink metadata to BT API");
            btoffload_update_metadata_api(get_session_type(), param);
        }
        break;
    }
    case PAL_PARAM_ID_SET_SOURCE_METADATA:
    {
        if (btoffload_update_metadata_api) {
            PAL_INFO(LOG_TAG, "sending source metadata to BT API");
            btoffload_update_metadata_api(get_session_type(), param);
        }
        break;
    }
    case PAL_PARAM_ID_LATENCY_MODE:
    {
        if (audio_source_set_latency_mode_api) {
            a2dpLatencyMode = ((pal_param_latency_mode_t *)param)->modes[0];
            status = audio_source_set_latency_mode_api(get_session_type(), a2dpLatencyMode);
            if (status) {
                PAL_ERR(LOG_TAG, "Set Parameter %d failed for value %d with exit status %d", param_id, a2dpLatencyMode, status);
                goto exit;
            }
        } else {
            status = -ENOSYS;
        }
        break;
    }
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    case PAL_PARAM_ID_BT_A2DP_DELAY_REPORT:
    {
        if (is_a2dp_offload()) {
            pal_param_bta2dp_delay_report_t *param_bt_a2dp_delay_report =
                (pal_param_bta2dp_delay_report_t *)param;

            if (param_bt_a2dp_delay_report->delay_report > ADJUST_LATENCY) {
                delay_report = param_bt_a2dp_delay_report->delay_report - ADJUST_LATENCY;
            } else {
                delay_report = 0;
            }
            if (pluginCodec) {
                codecLatency = pluginCodec->plugin_get_codec_latency(pluginCodec);
                param_bt_a2dp.latency = getLatency(delay_report);
                if (delay_report > 0) {
                    param_bt_a2dp.latency = calculate_latency(param_bt_a2dp.latency, delay_report);
                }

            }
            PAL_INFO(LOG_TAG, "delay_report = %d, latency = %d", delay_report, param_bt_a2dp.latency);
        }
        break;
    }
#endif
    default:
        return -EINVAL;
    }

exit:
    return status;
}

uint32_t BtA2dp::getLatency(uint32_t slatency)
{
    uint32_t latency = codecLatency;

    switch (codecType) {
    case ENC:
        switch (codecFormat) {
        case CODEC_TYPE_SBC:
            latency += (slatency == 0) ? DEFAULT_SINK_LATENCY_SBC : slatency;
            break;
        case CODEC_TYPE_AAC:
            latency += (slatency == 0) ? DEFAULT_SINK_LATENCY_AAC : slatency;
            break;
        case CODEC_TYPE_LDAC:
            latency += (slatency == 0) ? DEFAULT_SINK_LATENCY_LDAC : slatency;
            break;
        case CODEC_TYPE_APTX:
            latency += (slatency == 0) ? DEFAULT_SINK_LATENCY_APTX : slatency;
            break;
        case CODEC_TYPE_APTX_HD:
            latency += (slatency == 0) ? DEFAULT_SINK_LATENCY_APTX_HD : slatency;
            break;
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
        case CODEC_TYPE_SSC:
            latency += (slatency <= 0) ? DEFAULT_SINK_LATENCY_SSC : slatency;
            break;
#endif
        case CODEC_TYPE_APTX_AD:
        case CODEC_TYPE_LC3:
        case CODEC_TYPE_APTX_AD_QLEA:
        case CODEC_TYPE_APTX_AD_R4:
            latency += slatency;
            break;
        default:
            latency = DEFAULT_SINK_LATENCY;
            break;
        }
        break;
    case DEC:
        switch (codecFormat) {
        case CODEC_TYPE_SBC:
            latency = DEFAULT_SINK_LATENCY_SBC;
            break;
        case CODEC_TYPE_AAC:
            latency = DEFAULT_SINK_LATENCY_AAC;
            break;
        case CODEC_TYPE_LC3:
            latency = slatency;
            break;
        default:
            latency = DEFAULT_SINK_LATENCY;
            break;
        }
        break;
    default:
        break;
    }
    return latency;
}

int32_t BtA2dp::getDeviceParameter(uint32_t param_id, void **param)
{
    switch (param_id) {
    case PAL_PARAM_ID_BT_A2DP_RECONFIG:
    case PAL_PARAM_ID_BT_A2DP_RECONFIG_SUPPORTED:
    case PAL_PARAM_ID_BT_A2DP_SUSPENDED:
    case PAL_PARAM_ID_BT_A2DP_CAPTURE_SUSPENDED:
    case PAL_PARAM_ID_BT_A2DP_DECODER_LATENCY:
    case PAL_PARAM_ID_BT_A2DP_ENCODER_LATENCY:
#if defined(SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD) && defined(SEC_AUDIO_BLE_OFFLOAD)
    case PAL_PARAM_ID_BT_A2DP_SUSPENDED_FOR_BLE:
#endif
    {
        /* 23948329 SS_A2DP_FEATURE
        uint32_t slatency = 0;
        if (a2dpState == A2DP_STATE_STARTED && totalActiveSessionRequests &&
            ((param_bt_a2dp.latency == 0) || (codecFormat == CODEC_TYPE_APTX_AD))) {
            if (audio_sink_get_a2dp_latency_api) {
                slatency = audio_sink_get_a2dp_latency_api(get_session_type());
            } else if (audio_sink_get_a2dp_latency) {
                slatency = audio_sink_get_a2dp_latency();
            }
            param_bt_a2dp.latency = getLatency(slatency);
        }*/

#if defined(SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD) && defined(SEC_AUDIO_BLE_OFFLOAD)
        if (param_id == PAL_PARAM_ID_BT_A2DP_DECODER_LATENCY) {
            if (audio_source == AUDIO_SOURCE_CAMCORDER) {
                param_bt_a2dp.latency = BINAURAL_RECORDING_LATENCY;
                param_bt_a2dp.bt_mix_latency = BT_MIX_LATENCY;
            } else {
                param_bt_a2dp.latency = 0;
                param_bt_a2dp.bt_mix_latency = 0;
            }
        }
#endif
        *param = &param_bt_a2dp;
        break;
    }
    case PAL_PARAM_ID_BT_A2DP_FORCE_SWITCH:
    {
        if (totalActiveSessionRequests == 0 && deviceStartStopCount) {
            param_bt_a2dp.is_force_switch = true;
            PAL_DBG(LOG_TAG, "Force BT device switch for no total active BT sessions");
        } else {
            param_bt_a2dp.is_force_switch = false;
        }

        *param = &param_bt_a2dp;
        break;
    }
    case PAL_PARAM_ID_LATENCY_MODE:
    {
        int32_t status = 0;
        pal_param_latency_mode_t* param_latency_mode_ptr = (pal_param_latency_mode_t *)*param;
        if (audio_source_get_supported_latency_modes_api) {
            status = audio_source_get_supported_latency_modes_api(get_session_type(),
                        &(param_latency_mode_ptr->num_modes), param_latency_mode_ptr->num_modes,
                        param_latency_mode_ptr->modes);
            if (status) {
                PAL_ERR(LOG_TAG, "get Parameter param id %d failed", param_id);
                return status;
            }
        } else {
            param_latency_mode_ptr->num_modes = 0;
            status = -ENOSYS;
            return status;
        }
        break;
    }
    default:
        return -EINVAL;
    }
    return 0;
}

std::shared_ptr<Device> BtA2dp::getObject(pal_device_id_t id)
{
    if (id == PAL_DEVICE_OUT_BLUETOOTH_A2DP) {
        return objRx;
    }
    else if (id == PAL_DEVICE_IN_BLUETOOTH_A2DP) {
        return objTx;
    }
    else if (id == PAL_DEVICE_OUT_BLUETOOTH_BLE) {
        return objBleRx;
    }
    else if (id == PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST) {
        return objBleBroadcastRx;
    } else {
        return objBleTx;
    }
}

std::shared_ptr<Device>
BtA2dp::getInstance(struct pal_device *device, std::shared_ptr<ResourceManager> Rm)
{
    if (device->id == PAL_DEVICE_OUT_BLUETOOTH_A2DP) {
        if (!objRx) {
            PAL_INFO(LOG_TAG, "creating instance for  %d", device->id);
            std::shared_ptr<Device> sp(new BtA2dp(device, Rm));
            objRx = sp;
        }
        return objRx;
    } else if (device->id == PAL_DEVICE_IN_BLUETOOTH_A2DP) {
        if (!objTx) {
            PAL_INFO(LOG_TAG, "creating instance for  %d", device->id);
            std::shared_ptr<Device> sp(new BtA2dp(device, Rm));
            objTx = sp;
        }
        return objTx;
    } else if (device->id == PAL_DEVICE_OUT_BLUETOOTH_BLE) {
        if (!objBleRx) {
            PAL_INFO(LOG_TAG, "creating instance for  %d", device->id);
            std::shared_ptr<Device> sp(new BtA2dp(device, Rm));
            objBleRx = sp;
        }
        return objBleRx;
    } else if (device->id == PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST) {
        if (!objBleBroadcastRx) {
            PAL_INFO(LOG_TAG, "creating instance for  %d", device->id);
            std::shared_ptr<Device> sp(new BtA2dp(device, Rm));
            objBleBroadcastRx = sp;
        }
        return objBleBroadcastRx;
    } else {
        if (!objBleTx) {
            PAL_INFO(LOG_TAG, "creating instance for  %d", device->id);
            std::shared_ptr<Device> sp(new BtA2dp(device, Rm));
            objBleTx = sp;
        }
        return objBleTx;
    }
}
int32_t BtA2dp::checkDeviceStatus() {
    if (a2dpState == A2DP_STATE_DISCONNECTED) {
        PAL_INFO(LOG_TAG, "retry to open a2dp source");
        if (a2dpRole == SOURCE)
            open_a2dp_source();
        else {
#ifdef A2DP_SINK_SUPPORTED
            open_a2dp_sink();
#else
            a2dpState = A2DP_STATE_CONNECTED;
#endif
        }
    }
    PAL_DBG(LOG_TAG, "a2dpState: %d", a2dpState);
    return a2dpState;
}

/* Scope of BtScoRX/Tx class */
// definition of static BtSco member variables
std::shared_ptr<Device> BtSco::objRx = nullptr;
std::shared_ptr<Device> BtSco::objTx = nullptr;
bool BtSco::isScoOn = false;
bool BtSco::isWbSpeechEnabled = false;
int  BtSco::swbSpeechMode = SPEECH_MODE_INVALID;
bool BtSco::isSwbLc3Enabled = false;
// SS_BT_HFP - H_127 : RVP
#ifdef SEC_AUDIO_BLUETOOTH
int BtSco::rvpMode = PAL_BT_SCO_CODEC_TYPE_NONE;
#endif
// SS_BT_HFP - H_127 end
audio_lc3_codec_cfg_t BtSco::lc3CodecInfo = {};
bool BtSco::isNrecEnabled = false;

BtSco::BtSco(struct pal_device *device, std::shared_ptr<ResourceManager> Rm)
    : Bluetooth(device, Rm)
{
    codecType = (device->id == PAL_DEVICE_OUT_BLUETOOTH_SCO) ? ENC : DEC;
    pluginHandler = NULL;
    pluginCodec = NULL;
}

BtSco::~BtSco()
{
    if (lc3CodecInfo.enc_cfg.streamMapOut != NULL)
        delete [] lc3CodecInfo.enc_cfg.streamMapOut;
    if (lc3CodecInfo.dec_cfg.streamMapIn != NULL)
        delete [] lc3CodecInfo.dec_cfg.streamMapIn;
}

bool BtSco::isDeviceReady()
{
    return isScoOn;
}

void BtSco::updateSampleRate(uint32_t *sampleRate)
{
// SS_BT_HFP - H_127 : RVP
#ifdef SEC_AUDIO_BLUETOOTH
    if (rvpMode == PAL_BT_SCO_CODEC_TYPE_RVP) {
        *sampleRate = SAMPLINGRATE_96K;
    } else
#endif
// SS_BT_HFP - H_127 end
    if (isWbSpeechEnabled)
        *sampleRate = SAMPLINGRATE_16K;
    else if (swbSpeechMode != SPEECH_MODE_INVALID)
        *sampleRate = SAMPLINGRATE_96K;
    else if (isSwbLc3Enabled)
        *sampleRate = SAMPLINGRATE_96K;
    else
        *sampleRate = SAMPLINGRATE_8K;
}

int32_t BtSco::setDeviceParameter(uint32_t param_id, void *param)
{
    pal_param_btsco_t* param_bt_sco = (pal_param_btsco_t *)param;

    switch (param_id) {
// SS_BT_HFP - H_127 : RVP
#ifdef SEC_AUDIO_BLUETOOTH
    case PAL_PARAM_ID_BT_SCO_CODEC_TYPE:
        rvpMode = param_bt_sco->bt_sco_codec_type;
        PAL_DBG(LOG_TAG, "rvpMode = %d", rvpMode);
        break;
#endif
// SS_BT_HFP - H_127 end
    case PAL_PARAM_ID_BT_SCO:
        isScoOn = param_bt_sco->bt_sco_on;
        break;
    case PAL_PARAM_ID_BT_SCO_WB:
        isWbSpeechEnabled = param_bt_sco->bt_wb_speech_enabled;
        PAL_DBG(LOG_TAG, "isWbSpeechEnabled = %d", isWbSpeechEnabled);
        break;
    case PAL_PARAM_ID_BT_SCO_SWB:
        swbSpeechMode = param_bt_sco->bt_swb_speech_mode;
        PAL_DBG(LOG_TAG, "swbSpeechMode = %d", swbSpeechMode);
        break;
    case PAL_PARAM_ID_BT_SCO_LC3:
        isSwbLc3Enabled = param_bt_sco->bt_lc3_speech_enabled;
        if (isSwbLc3Enabled) {
            // parse sco lc3 parameters and pack into codec info
            convertCodecInfo(lc3CodecInfo, param_bt_sco->lc3_cfg);
        }
        PAL_DBG(LOG_TAG, "isSwbLc3Enabled = %d", isSwbLc3Enabled);
        break;
    case PAL_PARAM_ID_BT_SCO_NREC:
        isNrecEnabled = param_bt_sco->bt_sco_nrec;
        PAL_DBG(LOG_TAG, "isNrecEnabled = %d", isNrecEnabled);
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

bool BtSco::isScoNbWbActive()
{
    return codecFormat == CODEC_TYPE_INVALID;
}

void BtSco::convertCodecInfo(audio_lc3_codec_cfg_t &lc3CodecInfo,
                             btsco_lc3_cfg_t &lc3Cfg)
{
    std::vector<lc3_stream_map_t> steamMapIn;
    std::vector<lc3_stream_map_t> steamMapOut;
    uint32_t audio_location = 0;
    uint8_t stream_id = 0;
    uint8_t direction = 0;
    uint8_t value = 0;
    int idx = 0;
    std::string vendorStr(lc3Cfg.vendor);
    std::string streamMapStr(lc3Cfg.streamMap);
    std::regex vendorPattern("([0-9a-fA-F]{2})[,[:s:]]?");
    std::regex streamMapPattern("([0-9])[,[:s:]]+([0-9])[,[:s:]]+([MLR])");
    std::smatch match;

    // convert and fill in encoder cfg
    lc3CodecInfo.enc_cfg.toAirConfig.sampling_freq        = LC3_CSC[lc3Cfg.txconfig_index].sampling_freq;
    lc3CodecInfo.enc_cfg.toAirConfig.max_octets_per_frame = LC3_CSC[lc3Cfg.txconfig_index].max_octets_per_frame;
    lc3CodecInfo.enc_cfg.toAirConfig.frame_duration       = LC3_CSC[lc3Cfg.txconfig_index].frame_duration;
    lc3CodecInfo.enc_cfg.toAirConfig.bit_depth            = LC3_CSC[lc3Cfg.txconfig_index].bit_depth;
    if (lc3Cfg.fields_map & LC3_FRAME_DURATION_BIT)
        lc3CodecInfo.enc_cfg.toAirConfig.frame_duration   = lc3Cfg.frame_duration;
    lc3CodecInfo.enc_cfg.toAirConfig.api_version          = lc3Cfg.api_version;
    lc3CodecInfo.enc_cfg.toAirConfig.num_blocks           = lc3Cfg.num_blocks;
    lc3CodecInfo.enc_cfg.toAirConfig.default_q_level      = 0;
    lc3CodecInfo.enc_cfg.toAirConfig.mode                 = lc3Cfg.mode;
    lc3CodecInfo.is_enc_config_set                        = true;

    // convert and fill in decoder cfg
    lc3CodecInfo.dec_cfg.fromAirConfig.sampling_freq        = LC3_CSC[lc3Cfg.rxconfig_index].sampling_freq;
    lc3CodecInfo.dec_cfg.fromAirConfig.max_octets_per_frame = LC3_CSC[lc3Cfg.rxconfig_index].max_octets_per_frame;
    lc3CodecInfo.dec_cfg.fromAirConfig.frame_duration       = LC3_CSC[lc3Cfg.rxconfig_index].frame_duration;
    lc3CodecInfo.dec_cfg.fromAirConfig.bit_depth            = LC3_CSC[lc3Cfg.rxconfig_index].bit_depth;
    if (lc3Cfg.fields_map & LC3_FRAME_DURATION_BIT)
        lc3CodecInfo.dec_cfg.fromAirConfig.frame_duration   = lc3Cfg.frame_duration;
    lc3CodecInfo.dec_cfg.fromAirConfig.api_version          = lc3Cfg.api_version;
    lc3CodecInfo.dec_cfg.fromAirConfig.num_blocks           = lc3Cfg.num_blocks;
    lc3CodecInfo.dec_cfg.fromAirConfig.default_q_level      = 0;
    lc3CodecInfo.dec_cfg.fromAirConfig.mode                 = lc3Cfg.mode;
    lc3CodecInfo.is_dec_config_set                          = true;

    // parse vendor specific string
    idx = 15;
    while (std::regex_search(vendorStr, match, vendorPattern)) {
        if (idx < 0) {
            PAL_ERR(LOG_TAG, "wrong vendor info length, string %s", lc3Cfg.vendor);
            break;
        }
        value = (uint8_t)strtol(match[1].str().c_str(), NULL, 16);
        lc3CodecInfo.enc_cfg.toAirConfig.vendor_specific[idx] = value;
        lc3CodecInfo.dec_cfg.fromAirConfig.vendor_specific[idx--] = value;
        vendorStr = match.suffix().str();
    }
    if (idx != -1)
        PAL_ERR(LOG_TAG, "wrong vendor info length, string %s", lc3Cfg.vendor);

    // parse stream map string and append stream map structures
    while (std::regex_search(streamMapStr, match, streamMapPattern)) {
        stream_id = atoi(match[1].str().c_str());
        direction = atoi(match[2].str().c_str());
        if (!strcmp(match[3].str().c_str(), "M")) {
            audio_location = 0;
        } else if (!strcmp(match[3].str().c_str(), "L")) {
            audio_location = 1;
        } else if (!strcmp(match[3].str().c_str(), "R")) {
            audio_location = 2;
        }

        if ((stream_id > 1) || (direction > 1) || (audio_location > 2)) {
            PAL_ERR(LOG_TAG, "invalid stream info (%d, %d, %d)", stream_id, direction, audio_location);
            continue;
        }
        if (direction == TO_AIR)
            steamMapOut.push_back({audio_location, stream_id, direction});
        else
            steamMapIn.push_back({audio_location, stream_id, direction});

        streamMapStr = match.suffix().str();
    }

    PAL_DBG(LOG_TAG, "stream map out size: %d, stream map in size: %d", steamMapOut.size(), steamMapIn.size());
    if ((steamMapOut.size() == 0) || (steamMapIn.size() == 0)) {
        PAL_ERR(LOG_TAG, "invalid size steamMapOut.size %d, steamMapIn.size %d",
                steamMapOut.size(), steamMapIn.size());
        return;
    }

    idx = 0;
    lc3CodecInfo.enc_cfg.stream_map_size = steamMapOut.size();
    if (lc3CodecInfo.enc_cfg.streamMapOut != NULL)
        delete [] lc3CodecInfo.enc_cfg.streamMapOut;
    lc3CodecInfo.enc_cfg.streamMapOut = new lc3_stream_map_t[steamMapOut.size()];
    for (auto &it : steamMapOut) {
        lc3CodecInfo.enc_cfg.streamMapOut[idx].audio_location = it.audio_location;
        lc3CodecInfo.enc_cfg.streamMapOut[idx].stream_id = it.stream_id;
        lc3CodecInfo.enc_cfg.streamMapOut[idx++].direction = it.direction;
        PAL_DBG(LOG_TAG, "streamMapOut: audio_location %d, stream_id %d, direction %d",
                it.audio_location, it.stream_id, it.direction);
    }

    idx = 0;
    lc3CodecInfo.dec_cfg.stream_map_size = steamMapIn.size();
    if (lc3CodecInfo.dec_cfg.streamMapIn != NULL)
        delete [] lc3CodecInfo.dec_cfg.streamMapIn;
    lc3CodecInfo.dec_cfg.streamMapIn = new lc3_stream_map_t[steamMapIn.size()];
    for (auto &it : steamMapIn) {
        lc3CodecInfo.dec_cfg.streamMapIn[idx].audio_location = it.audio_location;
        lc3CodecInfo.dec_cfg.streamMapIn[idx].stream_id = it.stream_id;
        lc3CodecInfo.dec_cfg.streamMapIn[idx++].direction = it.direction;
        PAL_DBG(LOG_TAG, "steamMapIn: audio_location %d, stream_id %d, direction %d",
                it.audio_location, it.stream_id, it.direction);
    }

    if (lc3CodecInfo.dec_cfg.streamMapIn[0].audio_location == 0)
        lc3CodecInfo.dec_cfg.decoder_output_channel = CH_MONO;
    else
        lc3CodecInfo.dec_cfg.decoder_output_channel = CH_STEREO;
}

int BtSco::startSwb()
{
    int ret = 0;

    if (!isConfigured) {
        ret = configureGraphModules();
    } else {
        /* isAbrEnabled flag assignment will be skipped if
         * path is already configured.
         * Force isAbrEnabled flag for SWB use case. Ideally,
         * this flag should be populated from plugin.
         */
        if (codecFormat == CODEC_TYPE_APTX_AD_SPEECH)
            isAbrEnabled = true;
    }

    return ret;
}

// SS_BT_HFP - H_127 : RVP
#ifdef SEC_AUDIO_BLUETOOTH
int BtSco::startRvp()
{
    int ret = 0;

    if (!isConfigured) {
        ret = configureGraphModules();
    } else {
        if (codecFormat == CODEC_TYPE_RVP) {
            isAbrEnabled = true;
        }
    }

    return ret;
}
#endif
// SS_BT_HFP - H_127 end

int BtSco::start()
{
    int status = 0;
    mDeviceMutex.lock();

    if (customPayload)
        free(customPayload);

    customPayload = NULL;
    customPayloadSize = 0;

    if (swbSpeechMode != SPEECH_MODE_INVALID) {
        codecFormat = CODEC_TYPE_APTX_AD_SPEECH;
        codecInfo = (void *)&swbSpeechMode;
    } else if (isSwbLc3Enabled) {
        codecFormat = CODEC_TYPE_LC3;
        codecInfo = (void *)&lc3CodecInfo;
    }
// SS_BT_HFP - H_127 : RVP
#ifdef SEC_AUDIO_BLUETOOTH
    else if (rvpMode != PAL_BT_SCO_CODEC_TYPE_NONE) {
        codecFormat = CODEC_TYPE_RVP;
        codecInfo = (void *)&rvpMode;
    }
#endif
// SS_BT_HFP - H_127 end
    else {
        codecFormat = CODEC_TYPE_INVALID;
        isAbrEnabled = false;
    }

    updateDeviceMetadata();
    if ((codecFormat == CODEC_TYPE_APTX_AD_SPEECH) ||
        (codecFormat == CODEC_TYPE_LC3)) {
        status = startSwb();
        if (status)
            goto exit;
    }
// SS_BT_HFP - H_127 : RVP
#ifdef SEC_AUDIO_BLUETOOTH
    else if (codecFormat == CODEC_TYPE_RVP) {
        status = startRvp();
        if (status) {
            goto exit;
        }
    }
#endif
// SS_BT_HFP - H_127 end
    else {
        // For SCO NB and WB that don't have encoder and decoder in place,
        // just override codec configurations with device attributions.
        codecConfig.bit_width = deviceAttr.config.bit_width;
        codecConfig.sample_rate = deviceAttr.config.sample_rate;
        codecConfig.aud_fmt_id = PAL_AUDIO_FMT_DEFAULT_PCM;
        codecConfig.ch_info.channels = deviceAttr.config.ch_info.channels;
        isConfigured = true;
        PAL_DBG(LOG_TAG, "SCO WB/NB codecConfig is same as deviceAttr bw = %d,sr = %d,ch = %d",
            codecConfig.bit_width, codecConfig.sample_rate, codecConfig.ch_info.channels);
    }

    // Configure NREC only on Tx path & First session request only.
    if ((isConfigured == true) &&
        (deviceAttr.id == PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET)) {
        if (deviceStartStopCount == 0) {
            configureNrecParameters(isNrecEnabled);
        }
    }

    if (deviceStartStopCount == 0) {
        status = configureSlimbusClockSrc();
        if (status)
            goto exit;
    }

    status = Device::start_l();
    if (customPayload) {
        free(customPayload);
        customPayload = NULL;
        customPayloadSize = 0;
    }
    if (!status && isAbrEnabled)
// SS_BT_HFP - H_337 : abnormal isAbrEnabled value
    {
        PAL_INFO(LOG_TAG, "SCO isAbrEnabled %d, Type %d, Format 0x%X", isAbrEnabled, codecType, codecFormat);
// SS_BT_HFP - H_337 end
        startAbr();
    } // SS_BT_HFP - H_337 : abnormal isAbrEnabled value

exit:
    mDeviceMutex.unlock();
    return status;
}

int BtSco::stop()
{
    int status = 0;
    mDeviceMutex.lock();

    if (isAbrEnabled)
        stopAbr();

    if (pluginCodec) {
        pluginCodec->close_plugin(pluginCodec);
        pluginCodec = NULL;
    }
    if (pluginHandler) {
        dlclose(pluginHandler);
        pluginHandler = NULL;
    }

    Device::stop_l();
    if (isAbrEnabled == false)
        codecFormat = CODEC_TYPE_INVALID;
    if (deviceStartStopCount == 0)
        isConfigured = false;

    mDeviceMutex.unlock();
    return status;
}

std::shared_ptr<Device> BtSco::getObject(pal_device_id_t id)
{
    if (id == PAL_DEVICE_OUT_BLUETOOTH_SCO)
        return objRx;
    else
        return objTx;
}

std::shared_ptr<Device> BtSco::getInstance(struct pal_device *device,
                                           std::shared_ptr<ResourceManager> Rm)
{
    if (device->id == PAL_DEVICE_OUT_BLUETOOTH_SCO) {
        if (!objRx) {
            std::shared_ptr<Device> sp(new BtSco(device, Rm));
            objRx = sp;
        }
        return objRx;
    } else {
        if (!objTx) {
            PAL_ERR(LOG_TAG,  "creating instance for  %d", device->id);
            std::shared_ptr<Device> sp(new BtSco(device, Rm));
            objTx = sp;
        }
        return objTx;
    }
}
/* BtSco class end */
