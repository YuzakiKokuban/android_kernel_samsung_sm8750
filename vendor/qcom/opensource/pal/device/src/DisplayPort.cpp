/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: DisplayPort"
#include "DisplayPort.h"
#include "SessionAlsaUtils.h"
#include "ResourceManager.h"
#include "PayloadBuilder.h"
#include "Device.h"
#include "kvh2xml.h"

enum {
    EXT_DISPLAY_TYPE_NONE,
    EXT_DISPLAY_TYPE_HDMI,
    EXT_DISPLAY_TYPE_DP
};

/*
 * This file will have a maximum of 38 bytes:
 *
 * 4 bytes: number of audio blocks
 * 4 bytes: total length of Short Audio Descriptor (SAD) blocks
 * Maximum 10 * 3 bytes: SAD blocks
 */
#define MAX_SAD_BLOCKS      10
#define SAD_BLOCK_SIZE      3
#define ACK_ENABLE      "Ack_Enable"
#define CONNECT         "Connect"
#define DISCONNECT      "Disconnect"

static struct extDispState {
    void *edidInfo = NULL;
    bool valid = false;
    int type = EXT_DISPLAY_TYPE_NONE;
} extDisp[MAX_CONTROLLERS][MAX_STREAMS_PER_CONTROLLER];

std::shared_ptr<Device> DisplayPort::objRx = nullptr;
std::shared_ptr<Device> DisplayPort::objRx1 = nullptr;
std::shared_ptr<Device> DisplayPort::objTx = nullptr;

std::shared_ptr<Device> DisplayPort::getInstance(struct pal_device *device,
                                             std::shared_ptr<ResourceManager> Rm)
{
    if (!device)
       return NULL;

    PAL_DBG(LOG_TAG, "Enter, device id %d", device->id);

    if ((device->id == PAL_DEVICE_OUT_AUX_DIGITAL) ||
        (device->id == PAL_DEVICE_OUT_HDMI)) {
        if (!objRx) {
            std::shared_ptr<Device> sp(new DisplayPort(device, Rm));
            objRx = sp;
        }
        return objRx;
    } else if (device->id == PAL_DEVICE_OUT_AUX_DIGITAL_1) {
        if (!objRx1) {
            std::shared_ptr<Device> sp(new DisplayPort(device, Rm));
            objRx1 = sp;
        }
        return objRx1;
    } else if (device->id == PAL_DEVICE_IN_AUX_DIGITAL) {
        if (!objTx) {
            std::shared_ptr<Device> sp(new DisplayPort(device, Rm));
            objTx = sp;
        }
        return objTx;
    }
    return NULL;
}

std::shared_ptr<Device> DisplayPort::getObject(pal_device_id_t id)
{
    if ((id == PAL_DEVICE_OUT_AUX_DIGITAL) ||
        (id == PAL_DEVICE_OUT_HDMI)) {
        if (objRx) {
            if (objRx->getSndDeviceId() == id)
                return objRx;
        }
    } else if (id == PAL_DEVICE_OUT_AUX_DIGITAL_1) {
        if (objRx1) {
            if (objRx1->getSndDeviceId() == id)
                return objRx1;
        }
    } else if (id == PAL_DEVICE_IN_AUX_DIGITAL) {
        if (objTx) {
            if (objTx->getSndDeviceId() == id)
                return objTx;
        }
    }
    return NULL;
}

DisplayPort::DisplayPort(struct pal_device *device, std::shared_ptr<ResourceManager> Rm) :
Device(device, Rm)
{

}

int DisplayPort::getDeviceChannelAllocation(int num_channels)
{
    int channel_allocation = 0;

    switch (num_channels) {
        case 2:
            channel_allocation = 0x0; break;
        case 3:
            channel_allocation = 0x02; break;
        case 4:
            channel_allocation = 0x06; break;
        case 5:
            channel_allocation = 0x0A; break;
        case 6:
            channel_allocation = 0x0B; break;
        case 7:
            channel_allocation = 0x12; break;
        case 8:
            channel_allocation = 0x13; break;
        default:
            channel_allocation = 0x0; break;
            PAL_ERR(LOG_TAG, "invalid num channels: %d\n",
                    num_channels);
            break;
    }

    PAL_DBG(LOG_TAG, "num channels: %d, ca: 0x%x", num_channels,
            channel_allocation);

    return channel_allocation;
}

int DisplayPort::getDeviceAttributes(struct pal_device *dattr, Stream* streamHandle)
{
    int status = 0;
    int channel_allocation = 0;
    (void)streamHandle;

    if (!dattr) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG,"Invalid device attributes %d", status);
        return status;
    }
    ar_mem_cpy(dattr, sizeof(struct pal_device), &deviceAttr, sizeof(struct pal_device));

    channel_allocation = getDeviceChannelAllocation(deviceAttr.config.ch_info.channels);

    retrieveChannelMapLpass(channel_allocation, &dattr->config.ch_info.ch_map[0],
            PAL_MAX_CHANNELS_SUPPORTED);

    return status;
}

int DisplayPort::start()
{
    int status = 0;

    if (customPayload)
        free(customPayload);

    customPayload = NULL;
    customPayloadSize = 0;

    status = configureDpEndpoint();
    if (status != 0) {
        PAL_ERR(LOG_TAG,"Endpoint Configuration Failed");
        return status;
    }
    status = Device::start();
    return status;

}

int DisplayPort::configureDpEndpoint()
{
    int status = 0;
    std::string backEndName;
    PayloadBuilder* builder = new PayloadBuilder();
    struct dpAudioConfig cfg;
    uint8_t* payload = NULL;
    Stream *stream = NULL;
    Session *session = NULL;
    size_t payloadSize = 0;
    std::shared_ptr<Device> dev = nullptr;
    std::vector<Stream*> activestreams;
    uint32_t miid = 0;

    rm->getBackendName(deviceAttr.id, backEndName);
    dev = Device::getInstance(&deviceAttr, rm);
    status = rm->getActiveStream_l(activestreams, dev);
    if ((0 != status) || (activestreams.size() == 0)) {
        PAL_ERR(LOG_TAG, "no active stream available");
        status = -EINVAL;
        goto exit;
    }
    stream = static_cast<Stream *>(activestreams[0]);
    stream->getAssociatedSession(&session);
    status = session->getMIID(backEndName.c_str(), DEVICE_HW_ENDPOINT_RX, &miid);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", DEVICE_HW_ENDPOINT_RX, status);
        goto exit;
    }
    cfg.channel_allocation = getDeviceChannelAllocation(deviceAttr.config.ch_info.channels);
    cfg.mst_idx = dp_stream;
    cfg.dptx_idx = dp_controller;
    builder->payloadDpAudioConfig(&payload, &payloadSize, miid, &cfg);
    if (payloadSize) {
        status = updateCustomPayload(payload, payloadSize);
        free(payload);
        if (0 != status) {
            PAL_ERR(LOG_TAG," updateCustomPayload Failed\n");
            goto exit;
        }
    }

exit:
    if(builder) {
       delete builder;
       builder = NULL;
    }
    return status;
}

int DisplayPort::updateSysfsNode(const char *path, const char *data, size_t len)
{
    int err = 0;
    FILE *fd = NULL;

    fd = fopen(path, "w");
    if (!fd) {
        PAL_ERR(LOG_TAG,"Failed to open file %s\n", path);
        return -EIO;
    }
    fwrite(data, sizeof(char), len, fd);
    fclose(fd);
    return err;
}

int DisplayPort::getExtDispSysfsNodeIndex(int ext_disp_type)
{
    int node_index = -1;
    char fbvalue[80] = {0};
    char fbpath[80] = {0};
    int i = 0;
    FILE *ext_disp_fd = NULL;

    while (1) {
        snprintf(fbpath, sizeof(fbpath),
                  "/sys/class/graphics/fb%d/msm_fb_type", i);
        ext_disp_fd = fopen(fbpath, "r");
        if (ext_disp_fd) {
            if (fread(fbvalue, sizeof(char), 80, ext_disp_fd)) {
                if(((strncmp(fbvalue, "dtv panel", strlen("dtv panel")) == 0) &&
                    (ext_disp_type == EXT_DISPLAY_TYPE_HDMI)) ||
                   ((strncmp(fbvalue, "dp panel", strlen("dp panel")) == 0) &&
                    (ext_disp_type == EXT_DISPLAY_TYPE_DP))) {
                    node_index = i;
                    PAL_DBG(LOG_TAG, "Ext Disp:%d is at fb%d", ext_disp_type, i);
                    fclose(ext_disp_fd);
                    return node_index;
                }
            }
            fclose(ext_disp_fd);
            i++;
        } else {
            PAL_ERR(LOG_TAG, "Scanned till end of fbs or Failed to open fb node %d", i);
            break;
        }
    }

    return -1;
}

int DisplayPort::updateExtDispSysfsNode(int node_value, int controller, int stream)
{
    char ext_disp_ack_path[80] = {0};
    char ext_disp_ack_value[3] = {0};
    int index, ret = -1;
    struct mixer *mixer;
    int ext_disp_type = -EINVAL;
    int status = 0;

    status = rm->getHwAudioMixer(&mixer);
    if (status) {
         PAL_ERR(LOG_TAG," mixer error");
         return status;
    }

    ext_disp_type = getExtDispType(mixer, controller, stream);
    if (ext_disp_type < 0) {
        PAL_ERR(LOG_TAG, "Unable to get the external display type, err:%d", ext_disp_type);
        return -EINVAL;
    }

    index = getExtDispSysfsNodeIndex(ext_disp_type);
    if (index >= 0) {
        snprintf(ext_disp_ack_value, sizeof(ext_disp_ack_value), "%d", node_value);
        snprintf(ext_disp_ack_path, sizeof(ext_disp_ack_path),
                  "/sys/class/graphics/fb%d/hdmi_audio_cb", index);

        ret = updateSysfsNode(ext_disp_ack_path, ext_disp_ack_value,
                sizeof(ext_disp_ack_value));

        PAL_DBG(LOG_TAG, "update hdmi_audio_cb at fb[%d] to:[%d] %s",
            index, node_value, (ret >= 0) ? "success":"fail");
    }

    return ret;
}

int DisplayPort::updateAudioAckState(int node_value, int controller, int stream)
{
    int ret = 0;
    int ctl_index = 0;
    struct mixer_ctl *ctl = NULL;
    const char *ctl_prefix = "External Display";
    const char *ctl_suffix = "Audio Ack";
    char mixer_ctl_name[MIXER_PATH_MAX_LENGTH] = {0};
    struct mixer *mixer;

    ret = rm->getHwAudioMixer(&mixer);
    if (ret) {
         PAL_ERR(LOG_TAG," mixer error");
         return ret;
    }

    ctl_index = getDisplayPortCtlIndex(controller, stream);
    if (-EINVAL == ctl_index) {
        PAL_ERR(LOG_TAG, "Unknown controller/stream %d/%d",
                controller, stream);
        return -EINVAL;
    }

    if (0 == ctl_index)
        snprintf(mixer_ctl_name, sizeof(mixer_ctl_name),
                 "%s %s", ctl_prefix, ctl_suffix);
    else
        snprintf(mixer_ctl_name, sizeof(mixer_ctl_name),
                 "%s%d %s", ctl_prefix, ctl_index, ctl_suffix);

    PAL_DBG(LOG_TAG, "mixer ctl name: %s", mixer_ctl_name);
    ctl = mixer_get_ctl_by_name(mixer, mixer_ctl_name);
    /* If no mixer command support, fall back to sysfs node approach */
    if (!ctl) {
        PAL_DBG(LOG_TAG, "could not get ctl for mixer cmd(%s), use sysfs node instead\n",
              mixer_ctl_name);
        ret = updateExtDispSysfsNode(node_value, controller, stream);
    } else {
        char *ack_str = NULL;

        if (node_value == EXT_DISPLAY_PLUG_STATUS_NOTIFY_ENABLE)
            ack_str = (char*) ACK_ENABLE;
        else if (node_value == EXT_DISPLAY_PLUG_STATUS_NOTIFY_CONNECT)
            ack_str = (char*) CONNECT;
        else if (node_value == EXT_DISPLAY_PLUG_STATUS_NOTIFY_DISCONNECT)
            ack_str = (char*) DISCONNECT;
        else {
            PAL_ERR(LOG_TAG, "Invalid input parameter - 0x%x\n", node_value);
            return -EINVAL;
        }

        ret = mixer_ctl_set_enum_by_string(ctl, ack_str);
        if (ret)
            PAL_ERR(LOG_TAG, "Could not set ctl for mixer cmd - %s ret %d\n",
                   mixer_ctl_name, ret);
    }
    return ret;
}

int DisplayPort::init(pal_param_device_connection_t device_conn)
{
    PAL_DBG(LOG_TAG," Enter");
    int status = 0;
    static bool is_hdmi_sysfs_node_init = false;
    struct mixer *mixer;
    status = rm->getHwAudioMixer(&mixer);
    if (status) {
        PAL_ERR(LOG_TAG," mixer error");
        return status;
    }
    PAL_DBG(LOG_TAG," get mixer success");
    pal_param_disp_port_config_params* dp_config = (pal_param_disp_port_config_params*) &device_conn.device_config.dp_config;
    dp_controller = dp_config->controller;
    dp_stream = dp_config->stream;
    PAL_DBG(LOG_TAG," DP contr: %d  stream: %d", dp_controller, dp_stream);
    setExtDisplayDevice(mixer, dp_config->controller, dp_config->stream);
    status = getExtDispType(mixer, dp_config->controller, dp_config->stream);
    if (status < 0) {
        PAL_ERR(LOG_TAG," Failed to query disp type, status:%d", status);
        return status;
    } else {
        cacheEdid(mixer, dp_config->controller, dp_config->stream);
    }
    if (is_hdmi_sysfs_node_init == false) {
        is_hdmi_sysfs_node_init = true;
        status = updateAudioAckState(EXT_DISPLAY_PLUG_STATUS_NOTIFY_ENABLE,
                                     dp_controller, dp_stream);
        if (status < 0) {
            PAL_ERR(LOG_TAG,"EXT_DISPLAY_PLUG_STATUS_NOTIFY_ENABLE failed");
            return status;
        }
    }
    status = updateAudioAckState(EXT_DISPLAY_PLUG_STATUS_NOTIFY_CONNECT, dp_controller, dp_stream);

    PAL_DBG(LOG_TAG," Exit with status %d", status);
    return status;
}

int DisplayPort::deinit(pal_param_device_connection_t device_conn __unused)
{
    int status = updateAudioAckState(EXT_DISPLAY_PLUG_STATUS_NOTIFY_DISCONNECT,
                                     dp_controller, dp_stream);
    //To-Do : Have to invalidate the cahed EDID
    return status;
}

bool DisplayPort::isDisplayPortEnabled () {
    //TBD: Check for the system prop here
    return true;
}

/*void DisplayPort1::resetEdidInfo()
{
    DisplayPort::resetEdidInfo();

}*/


void DisplayPort::resetEdidInfo() {
    PAL_VERBOSE(LOG_TAG," enter");

    int i = 0, j = 0;
    for (i = 0; i < MAX_CONTROLLERS; ++i) {
        for (j = 0; j < MAX_STREAMS_PER_CONTROLLER; ++j) {
            struct extDispState *state = &extDisp[i][j];
            state->type = EXT_DISPLAY_TYPE_NONE;
            if (state->edidInfo) {
                free(state->edidInfo);
                state->edidInfo = NULL;
            }
            state->valid = false;
        }
    }
}

/*
 * returns index for mixer controls
 *
 * example: max controllers = 2, max streams = 4
 * controller = 0, stream = 0 => Index 0
 * ...
 * controller = 0, stream = 3 => Index 3
 * controller = 1, stream = 0 => Index 4
 * ...
 * controller = 1, stream = 3 => Index 7
 */
int32_t DisplayPort::getDisplayPortCtlIndex(int controller, int stream)
{

    if (controller < 0 || controller >= MAX_CONTROLLERS ||
            stream < 0 || stream >= MAX_STREAMS_PER_CONTROLLER) {
        PAL_ERR(LOG_TAG,"Invalid controller/stream - %d/%d",
              controller, stream);
        return -EINVAL;
    }

    return ((controller % MAX_CONTROLLERS) * MAX_STREAMS_PER_CONTROLLER) +
            (stream % MAX_STREAMS_PER_CONTROLLER);
}

int32_t DisplayPort::setExtDisplayDevice(struct audio_mixer *mixer, int controller, int stream)
{
    struct mixer_ctl *ctl = NULL;
    int ctlIndex = 0;
    const char *ctlNamePrefix = "External Display";
    const char *ctlNameSuffix = "Audio Device";
    char mixerCtlName[MIXER_PATH_MAX_LENGTH] = {0};
    long deviceValues[2] = {-1, -1};

    ctlIndex = getDisplayPortCtlIndex(controller, stream);
    if (-EINVAL == ctlIndex) {
        PAL_ERR(LOG_TAG,"Unknown controller/stream %d/%d", controller, stream);
        return -EINVAL;
    }

    PAL_DBG(LOG_TAG," ctlIndex: %d controller: %d stream: %d", ctlIndex, controller, stream);

    if (0 == ctlIndex)
        snprintf(mixerCtlName, sizeof(mixerCtlName),
                 "%s %s", ctlNamePrefix, ctlNameSuffix);
    else
        snprintf(mixerCtlName, sizeof(mixerCtlName),
                 "%s%d %s", ctlNamePrefix, ctlIndex, ctlNameSuffix);

    deviceValues[0] = controller;
    deviceValues[1] = stream;

    PAL_DBG(LOG_TAG," mixer: %pK mixer ctl name: %s", mixer, mixerCtlName);

    ctl = mixer_get_ctl_by_name(mixer, mixerCtlName);
    if (!ctl) {
        PAL_ERR(LOG_TAG,"Could not get ctl for mixer cmd - %s", mixerCtlName);
        return -EINVAL;
    }

    PAL_DBG(LOG_TAG,"controller/stream: %ld/%ld", deviceValues[0], deviceValues[1]);

    return mixer_ctl_set_array(ctl, deviceValues, ARRAY_SIZE(deviceValues));
}

int32_t DisplayPort::getExtDispType(struct audio_mixer *mixer, int controller, int stream)
{
    int dispType = EXT_DISPLAY_TYPE_NONE;
    int ctlIndex = 0;
    struct extDispState *disp = NULL;

    ctlIndex = getDisplayPortCtlIndex(controller, stream);
    if (-EINVAL == ctlIndex) {
        PAL_ERR(LOG_TAG,"Unknown controller/stream %d/%d", controller, stream);
        return -EINVAL;
    }

    disp = &extDisp[controller][stream];
    if (disp->type != EXT_DISPLAY_TYPE_NONE) {
        PAL_DBG(LOG_TAG," Returning cached ext disp type:%s",
               (disp->type == EXT_DISPLAY_TYPE_DP) ? "DisplayPort" : "HDMI");
         return disp->type;
    }

    if (isDisplayPortEnabled()) {
        struct mixer_ctl *ctl = NULL;
        const char *ctlNamePrefix = "External Display";
        const char *ctlNameSuffix = "Type";
        char mixerCtlName[MIXER_PATH_MAX_LENGTH] = {0};

        if (0 == ctlIndex)
            snprintf(mixerCtlName, sizeof(mixerCtlName),
                     "%s %s", ctlNamePrefix, ctlNameSuffix);
        else
            snprintf(mixerCtlName, sizeof(mixerCtlName),
                     "%s%d %s", ctlNamePrefix, ctlIndex, ctlNameSuffix);

        PAL_VERBOSE(LOG_TAG,"mixer ctl name: %s", mixerCtlName);

        ctl = mixer_get_ctl_by_name(mixer, mixerCtlName);
        if (!ctl) {
            PAL_ERR(LOG_TAG,"Could not get ctl for mixer cmd - %s", mixerCtlName);
            return -EINVAL;
        }

        dispType = mixer_ctl_get_value(ctl, 0);
        if (dispType == EXT_DISPLAY_TYPE_NONE) {
            PAL_ERR(LOG_TAG,"Invalid external display type: %d", dispType);
            return -EINVAL;
        }
    } else {
        dispType = EXT_DISPLAY_TYPE_HDMI;
    }

    disp->type = dispType;

    PAL_DBG(LOG_TAG," ext disp type: %s", (dispType == EXT_DISPLAY_TYPE_DP) ? "DisplayPort" : "HDMI");

    return dispType;
}

int DisplayPort::getEdidInfo(struct audio_mixer *mixer, int controller, int stream)
{

    char block[MAX_SAD_BLOCKS * SAD_BLOCK_SIZE];
    int ret, count;
    char edidData[MAX_SAD_BLOCKS * SAD_BLOCK_SIZE + 1] = {0};
    struct extDispState *state = NULL;
    int ctlIndex = 0;
    struct mixer_ctl *ctl = NULL;
    const char *ctlNamePrefix = "Display Port";
    const char *ctlNameSuffix = "EDID";
    char mixerCtlName[MIXER_PATH_MAX_LENGTH] = {0};

    ctlIndex = getDisplayPortCtlIndex(controller, stream);
    if (-EINVAL == ctlIndex) {
        PAL_ERR(LOG_TAG," Unknown controller/stream %d/%d", controller, stream);
        return -EINVAL;
    }

    state = &extDisp[controller][stream];
    if (state->valid) {
#ifdef SEC_AUDIO_COMMON
        PAL_DBG(LOG_TAG,"do not use cached edid, try to parse edid data");
#else
        /* use cached edid */
        return 0;
#endif
    }

    switch(state->type) {
        case EXT_DISPLAY_TYPE_HDMI:
            snprintf(mixerCtlName, sizeof(mixerCtlName), "HDMI EDID");
            break;
        case EXT_DISPLAY_TYPE_DP:
            if (!isDisplayPortEnabled()) {
                PAL_ERR(LOG_TAG," display port is not supported");
                return -EINVAL;
            }

            if (0 == ctlIndex)
                snprintf(mixerCtlName, sizeof(mixerCtlName),
                         "%s %s", ctlNamePrefix, ctlNameSuffix);
            else
                snprintf(mixerCtlName, sizeof(mixerCtlName),
                         "%s%d %s", ctlNamePrefix, ctlIndex, ctlNameSuffix);
            break;
        default:
            PAL_ERR(LOG_TAG," Invalid disp_type %d", state->type);
            return -EINVAL;
    }

    if (state->edidInfo == NULL)
        state->edidInfo =
            (struct edidAudioInfo *)calloc(1, sizeof(struct edidAudioInfo));

    PAL_VERBOSE(LOG_TAG," mixer ctl name: %s", mixerCtlName);

    ctl = mixer_get_ctl_by_name(mixer, mixerCtlName);
    if (!ctl) {
        PAL_ERR(LOG_TAG," Could not get ctl for mixer cmd - %s", mixerCtlName);
        goto fail;
    }

    mixer_ctl_update(ctl);

    count = mixer_ctl_get_num_values(ctl);

    /* Read SAD blocks, clamping the maximum size for safety */
    if (count > (int)sizeof(block))
        count = (int)sizeof(block);

    ret = mixer_ctl_get_array(ctl, block, count);
    if (ret != 0) {
        PAL_ERR(LOG_TAG," mixer_ctl_get_array() failed to get EDID info");
        goto fail;
    }
    edidData[0] = count;
    memcpy(&edidData[1], block, count);

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_DBG(LOG_TAG," received edid data: count %d", edidData[0]);
#else
    PAL_VERBOSE(LOG_TAG," received edid data: count %d", edidData[0]);
#endif

    if (!getSinkCaps((struct edidAudioInfo *)state->edidInfo, edidData)) {
        PAL_ERR(LOG_TAG," Failed to get extn disp sink capabilities");
        goto fail;
    }
    state->valid = true;
    return 0;
fail:
    if (state->edidInfo) {
        free(state->edidInfo);
        state->edidInfo = NULL;
        state->valid = false;
    }
    PAL_ERR(LOG_TAG," return -EINVAL");
    return -EINVAL;
}

void DisplayPort::cacheEdid(struct audio_mixer *mixer, int controller, int stream)
{
    getEdidInfo(mixer, controller, stream);
}

int32_t DisplayPort::isSampleRateSupported(uint32_t sampleRate)
{
    int32_t rc = 0;
    PAL_ERR(LOG_TAG, "sampleRate %d", sampleRate);

    if (sampleRate % SAMPLINGRATE_44K == 0)
        return rc;

    switch (sampleRate) {
        case SAMPLINGRATE_44K:
        case SAMPLINGRATE_48K:
        case SAMPLINGRATE_96K:
        case SAMPLINGRATE_192K:
        case SAMPLINGRATE_384K:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "sample rate not supported rc %d", rc);
            break;
    }
    return rc;
}

//TBD why do these channels have to be supported, DisplayPorts support only 1/2?
int32_t DisplayPort::isChannelSupported(uint32_t numChannels)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "numChannels %u", numChannels);
    switch (numChannels) {
        case CHANNELS_1:
        case CHANNELS_2:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "channels not supported rc %d", rc);
            break;
    }
    return rc;
}

int32_t DisplayPort::isBitWidthSupported(uint32_t bitWidth)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "bitWidth %u", bitWidth);
    switch (bitWidth) {
        case BITWIDTH_16:
        case BITWIDTH_24:
        case BITWIDTH_32:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "bit width not supported rc %d", rc);
            break;
    }
    return rc;
}

int32_t DisplayPort::checkAndUpdateBitWidth(uint32_t *bitWidth)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "bitWidth %u", *bitWidth);
    switch (*bitWidth) {
        case BITWIDTH_16:
        case BITWIDTH_24:
        case BITWIDTH_32:
            break;
        default:
            *bitWidth = BITWIDTH_16;
            PAL_DBG(LOG_TAG, "bit width not supported, setting to default 16 bit");
            break;
    }
    return rc;
}

int32_t DisplayPort::checkAndUpdateSampleRate(uint32_t *sampleRate)
{
    int32_t rc = 0;

    if (*sampleRate <= SAMPLINGRATE_48K)
        *sampleRate = SAMPLINGRATE_48K;
    else if (*sampleRate > SAMPLINGRATE_48K && *sampleRate <= SAMPLINGRATE_96K)
        *sampleRate = SAMPLINGRATE_96K;
    else if (*sampleRate > SAMPLINGRATE_96K && *sampleRate <= SAMPLINGRATE_192K)
        *sampleRate = SAMPLINGRATE_192K;
    else if (*sampleRate > SAMPLINGRATE_192K && *sampleRate <= SAMPLINGRATE_384K)
        *sampleRate = SAMPLINGRATE_384K;

    PAL_DBG(LOG_TAG, "sampleRate %d", *sampleRate);

    return rc;
}



/* ----------------------------------------------------------------------------------
   ------------------------         Edid                          -------------------
   ----------------------------------------------------------------------------------*/
const char * DisplayPort::edidFormatToStr(unsigned char format)
{
    static std::string formatStr = "??";

    switch (format) {
    case LPCM:
        formatStr = "Format:LPCM";
        break;
    case AC3:
        formatStr = "Format:AC-3";
        break;
    case MPEG1:
        formatStr = "Format:MPEG1 (Layers 1 & 2)";
        break;
    case MP3:
        formatStr =  "Format:MP3 (MPEG1 Layer 3)";
        break;
    case MPEG2_MULTI_CHANNEL:
        formatStr = "Format:MPEG2 (multichannel)";
        break;
    case AAC:
        formatStr =  "Format:AAC";
        break;
    case DTS:
        formatStr =  "Format:DTS";
        break;
    case ATRAC:
        formatStr =  "Format:ATRAC";
        break;
    case SACD:
        formatStr =  "Format:One-bit audio aka SACD";
        break;
    case DOLBY_DIGITAL_PLUS:
        formatStr =  "Format:Dolby Digital +";
        break;
    case DTS_HD:
        formatStr =  "Format:DTS-HD";
        break;
    case MAT:
        formatStr =  "Format:MAT (MLP)";
        break;
    case DST:
        formatStr =  "Format:DST";
        break;
    case WMA_PRO:
        formatStr =  "Format:WMA Pro";
        break;
    default:
        break;
    }
    return formatStr.c_str();
}

bool DisplayPort::isSampleRateSupported(unsigned char srByte, int samplingRate)
{
    int result = 0;
    // Codec Supports Sample rate in range of 48K-192K
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_INFO(LOG_TAG," srByte: %d, samplingRate: %d", srByte, samplingRate);
#else
    PAL_VERBOSE(LOG_TAG," srByte: %d, samplingRate: %d", srByte, samplingRate);
#endif
    switch (samplingRate) {
    case 192000:
        result = (srByte & BIT(6));
        break;
    case 176400:
        result = (srByte & BIT(5));
        break;
    case 96000:
        result = (srByte & BIT(4));
        break;
    case 88200:
        result = (srByte & BIT(3));
        break;
    case 48000:
        result = (srByte & BIT(2));
        break;
    case 44100:
        result = (srByte & BIT(1));
        break;
    case 32000:
        result = (srByte & BIT(0));
        break;
     default:
        break;
    }

    if (result)
        return true;

    return false;
}

unsigned char DisplayPort::getEdidBpsByte(unsigned char byte,
                        unsigned char format)
{
    if (format == 0) {
        PAL_VERBOSE(LOG_TAG," not lpcm format, return 0");
        return 0;
    }
    return byte;
}

bool DisplayPort::isSupportedBps(unsigned char bpsByte, int bps)
{
    int result = 0;

    switch (bps) {
    case 24:
        PAL_VERBOSE(LOG_TAG,"24bit");
        result = (bpsByte & BIT(2));
        break;
    case 16:
        PAL_VERBOSE(LOG_TAG,"16bit");
        result = (bpsByte & BIT(0));
        break;
     default:
        break;
    }

    if (result)
        return true;

    return false;
}

int DisplayPort::getHighestEdidSF(unsigned char byte)
{
    int nfreq = 0;

    if (byte & BIT(6)) {
        PAL_VERBOSE(LOG_TAG,"Highest: 192kHz");
        nfreq = 192000;
    } else if (byte & BIT(5)) {
        PAL_VERBOSE(LOG_TAG,"Highest: 176kHz");
        nfreq = 176000;
    } else if (byte & BIT(4)) {
        PAL_VERBOSE(LOG_TAG,"Highest: 96kHz");
        nfreq = 96000;
    } else if (byte & BIT(3)) {
        PAL_VERBOSE(LOG_TAG,"Highest: 88.2kHz");
        nfreq = 88200;
    } else if (byte & BIT(2)) {
        PAL_VERBOSE(LOG_TAG,"Highest: 48kHz");
        nfreq = 48000;
    } else if (byte & BIT(1)) {
        PAL_VERBOSE(LOG_TAG,"Highest: 44.1kHz");
        nfreq = 44100;
    } else if (byte & BIT(0)) {
        PAL_VERBOSE(LOG_TAG,"Highest: 32kHz");
        nfreq = 32000;
    }
    return nfreq;
}

void DisplayPort::updateChannelMap(edidAudioInfo* info)
{
    /* HDMI Cable follows CEA standard so SAD is received in CEA
     * Input source file channel map is fed to ASM in WAV standard(audio.h)
     * so upto 7.1 SAD bits are:
     * in CEA convention: RLC/RRC,FLC/FRC,RC,RL/RR,FC,LFE,FL/FR
     * in WAV convention: BL/BR,FLC/FRC,BC,SL/SR,FC,LFE,FL/FR
     * Corresponding ADSP IDs (apr-audio_v2.h):
     * PCM_CHANNEL_FL/PCM_CHANNEL_FR,
     * PCM_CHANNEL_LFE,
     * PCM_CHANNEL_FC,
     * PCM_CHANNEL_LS/PCM_CHANNEL_RS,
     * PCM_CHANNEL_CS,
     * PCM_CHANNEL_FLC/PCM_CHANNEL_FRC
     * PCM_CHANNEL_LB/PCM_CHANNEL_RB
     */
    if (!info)
        return;
    memset(info->channelMap, 0, MAX_CHANNELS_SUPPORTED);
    if(info->speakerAllocation[0] & BIT(0)) {
        info->channelMap[0] = PCM_CHANNEL_L;
        info->channelMap[1] = PCM_CHANNEL_R;
    }
    if(info->speakerAllocation[0] & BIT(1)) {
        info->channelMap[2] = PCM_CHANNEL_LFE;
    }
    if(info->speakerAllocation[0] & BIT(2)) {
        info->channelMap[3] = PCM_CHANNEL_C;
    }
    if(info->speakerAllocation[0] & BIT(3)) {
    /*
     * As per CEA(HDMI Cable) standard Bit 3 is equivalent
     * to SideLeft/SideRight of WAV standard
     */
        info->channelMap[4] = PCM_CHANNEL_LS;
        info->channelMap[5] = PCM_CHANNEL_RS;
    }
    if(info->speakerAllocation[0] & BIT(4)) {
        if(info->speakerAllocation[0] & BIT(3)) {
            info->channelMap[6] = PCM_CHANNEL_CS;
            info->channelMap[7] = 0;
        } else if (info->speakerAllocation[1] & BIT(1)) {
            info->channelMap[6] = PCM_CHANNEL_CS;
            info->channelMap[7] = PCM_CHANNEL_TS;
        } else if (info->speakerAllocation[1] & BIT(2)) {
            info->channelMap[6] = PCM_CHANNEL_CS;
            info->channelMap[7] = PCM_CHANNEL_CVH;
        } else {
            info->channelMap[4] = PCM_CHANNEL_CS;
            info->channelMap[5] = 0;
        }
    }
    if(info->speakerAllocation[0] & BIT(5)) {
        info->channelMap[6] = PCM_CHANNEL_FLC;
        info->channelMap[7] = PCM_CHANNEL_FRC;
    }
    if(info->speakerAllocation[0] & BIT(6)) {
        // If RLC/RRC is present, RC is invalid as per specification
        info->speakerAllocation[0] &= 0xef;
        /*
         * As per CEA(HDMI Cable) standard Bit 6 is equivalent
         * to BackLeft/BackRight of WAV standard
         */
        info->channelMap[6] = PCM_CHANNEL_LB;
        info->channelMap[7] = PCM_CHANNEL_RB;
    }
    // higher channel are not defined by LPASS
    //info->nSpeakerAllocation[0] &= 0x3f;
    if(info->speakerAllocation[0] & BIT(7)) {
        info->channelMap[6] = 0; // PCM_CHANNEL_FLW; but not defined by LPASS
        info->channelMap[7] = 0; // PCM_CHANNEL_FRW; but not defined by LPASS
    }
    if(info->speakerAllocation[1] & BIT(0)) {
        info->channelMap[6] = 0; // PCM_CHANNEL_FLH; but not defined by LPASS
        info->channelMap[7] = 0; // PCM_CHANNEL_FRH; but not defined by LPASS
    }

    PAL_VERBOSE(LOG_TAG," channel map updated to [%d %d %d %d %d %d %d %d ]  [%x %x %x]"
        , info->channelMap[0], info->channelMap[1], info->channelMap[2]
        , info->channelMap[3], info->channelMap[4], info->channelMap[5]
        , info->channelMap[6], info->channelMap[7]
        , info->speakerAllocation[0], info->speakerAllocation[1]
        , info->speakerAllocation[2]);
}

void DisplayPort::dumpSpeakerAllocation(edidAudioInfo* info)
{
    if (!info)
        return;

    if (info->speakerAllocation[0] & BIT(7))
        PAL_VERBOSE(LOG_TAG,"FLW/FRW");
    if (info->speakerAllocation[0] & BIT(6))
        PAL_VERBOSE(LOG_TAG,"RLC/RRC");
    if (info->speakerAllocation[0] & BIT(5))
        PAL_VERBOSE(LOG_TAG,"FLC/FRC");
    if (info->speakerAllocation[0] & BIT(4))
        PAL_VERBOSE(LOG_TAG,"RC");
    if (info->speakerAllocation[0] & BIT(3))
        PAL_VERBOSE(LOG_TAG,"RL/RR");
    if (info->speakerAllocation[0] & BIT(2))
        PAL_VERBOSE(LOG_TAG,"FC");
    if (info->speakerAllocation[0] & BIT(1))
        PAL_VERBOSE(LOG_TAG,"LFE");
    if (info->speakerAllocation[0] & BIT(0))
        PAL_VERBOSE(LOG_TAG,"FL/FR");
    if (info->speakerAllocation[1] & BIT(2))
        PAL_VERBOSE(LOG_TAG,"FCH");
    if (info->speakerAllocation[1] & BIT(1))
        PAL_VERBOSE(LOG_TAG,"TC");
    if (info->speakerAllocation[1] & BIT(0))
        PAL_VERBOSE(LOG_TAG,"FLH/FRH");
}

void DisplayPort::updateChannelAllocation(edidAudioInfo* info)
{
    int16_t ca;
    int16_t spkrAlloc;

    if (!info)
        return;

    /* Most common 5.1 SAD is 0xF, ca 0x0b
     * and 7.1 SAD is 0x4F, ca 0x13 */
    spkrAlloc = ((info->speakerAllocation[1]) << 8) |
               (info->speakerAllocation[0]);
    PAL_VERBOSE(LOG_TAG,"info->nSpeakerAllocation %x %x\n", info->speakerAllocation[0],
                                              info->speakerAllocation[1]);
    PAL_VERBOSE(LOG_TAG,"spkrAlloc: %x", spkrAlloc);

    /* The below switch case calculates channel allocation values
       as defined in CEA-861 section 6.6.2 */
    switch (spkrAlloc) {
    case BIT(0):                                           ca = 0x00; break;
    case BIT(0)|BIT(1):                                    ca = 0x01; break;
    case BIT(0)|BIT(2):                                    ca = 0x02; break;
    case BIT(0)|BIT(1)|BIT(2):                             ca = 0x03; break;
    case BIT(0)|BIT(4):                                    ca = 0x04; break;
    case BIT(0)|BIT(1)|BIT(4):                             ca = 0x05; break;
    case BIT(0)|BIT(2)|BIT(4):                             ca = 0x06; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(4):                      ca = 0x07; break;
    case BIT(0)|BIT(3):                                    ca = 0x08; break;
    case BIT(0)|BIT(1)|BIT(3):                             ca = 0x09; break;
    case BIT(0)|BIT(2)|BIT(3):                             ca = 0x0A; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3):                      ca = 0x0B; break;
    case BIT(0)|BIT(3)|BIT(4):                             ca = 0x0C; break;
    case BIT(0)|BIT(1)|BIT(3)|BIT(4):                      ca = 0x0D; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(4):                      ca = 0x0E; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4):               ca = 0x0F; break;
    case BIT(0)|BIT(3)|BIT(6):                             ca = 0x10; break;
    case BIT(0)|BIT(1)|BIT(3)|BIT(6):                      ca = 0x11; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(6):                      ca = 0x12; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(6):               ca = 0x13; break;
    case BIT(0)|BIT(5):                                    ca = 0x14; break;
    case BIT(0)|BIT(1)|BIT(5):                             ca = 0x15; break;
    case BIT(0)|BIT(2)|BIT(5):                             ca = 0x16; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(5):                      ca = 0x17; break;
    case BIT(0)|BIT(4)|BIT(5):                             ca = 0x18; break;
    case BIT(0)|BIT(1)|BIT(4)|BIT(5):                      ca = 0x19; break;
    case BIT(0)|BIT(2)|BIT(4)|BIT(5):                      ca = 0x1A; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(4)|BIT(5):               ca = 0x1B; break;
    case BIT(0)|BIT(3)|BIT(5):                             ca = 0x1C; break;
    case BIT(0)|BIT(1)|BIT(3)|BIT(5):                      ca = 0x1D; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(5):                      ca = 0x1E; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(5):               ca = 0x1F; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(10):                     ca = 0x20; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(10):              ca = 0x21; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(9):                      ca = 0x22; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(9):               ca = 0x23; break;
    case BIT(0)|BIT(3)|BIT(8):                             ca = 0x24; break;
    case BIT(0)|BIT(1)|BIT(3)|BIT(8):                      ca = 0x25; break;
    case BIT(0)|BIT(3)|BIT(7):                             ca = 0x26; break;
    case BIT(0)|BIT(1)|BIT(3)|BIT(7):                      ca = 0x27; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(4)|BIT(9):               ca = 0x28; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(9):        ca = 0x29; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(4)|BIT(10):              ca = 0x2A; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(10):       ca = 0x2B; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(9)|BIT(10):              ca = 0x2C; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(9)|BIT(10):       ca = 0x2D; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(8):                      ca = 0x2E; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(8):               ca = 0x2F; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(7):                      ca = 0x30; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(7):               ca = 0x31; break;
    default:                                               ca = 0x0;  break;
    }
    PAL_DBG(LOG_TAG," channel allocation: %x", ca);
    info->channelAllocation = ca;
}

void DisplayPort::retrieveChannelMapLpass(int ca, uint8_t *ch_map, int ch_map_size)
{
    if (!ch_map)
        return;

    if (((ca < 0) || (ca > 0x1f)) &&
         (ca != 0x2f)) {
        PAL_ERR(LOG_TAG,"Channel allocation out of supported range");
        return;
    }
    PAL_VERBOSE(LOG_TAG,"channelAllocation 0x%x", ca);

    if (ch_map_size < MAX_CHANNELS_SUPPORTED)
        return;

    memset(ch_map, 0, MAX_CHANNELS_SUPPORTED);

    switch(ca) {
    case 0x0:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        break;
    case 0x1:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        break;
    case 0x2:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_C;
        break;
    case 0x3:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_C;
        break;
    case 0x4:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_CS;
        break;
    case 0x5:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_CS;
        break;
    case 0x6:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_C;
        ch_map[3] = PCM_CHANNEL_CS;
        break;
    case 0x7:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_C;
        ch_map[4] = PCM_CHANNEL_CS;
        break;
    case 0x8:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LS;
        ch_map[3] = PCM_CHANNEL_RS;
        break;
    case 0x9:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_LS;
        ch_map[4] = PCM_CHANNEL_RS;
        break;
    case 0xa:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_C;
        ch_map[3] = PCM_CHANNEL_LS;
        ch_map[4] = PCM_CHANNEL_RS;
        break;
    case 0xb:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_C;
        ch_map[4] = PCM_CHANNEL_LS;
        ch_map[5] = PCM_CHANNEL_RS;
        break;
    case 0xc:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LS;
        ch_map[3] = PCM_CHANNEL_RS;
        ch_map[4] = PCM_CHANNEL_CS;
        break;
    case 0xd:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_LS;
        ch_map[4] = PCM_CHANNEL_RS;
        ch_map[5] = PCM_CHANNEL_CS;
        break;
    case 0xe:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_C;
        ch_map[3] = PCM_CHANNEL_LS;
        ch_map[4] = PCM_CHANNEL_RS;
        ch_map[5] = PCM_CHANNEL_CS;
        break;
    case 0xf:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_C;
        ch_map[4] = PCM_CHANNEL_LS;
        ch_map[5] = PCM_CHANNEL_RS;
        ch_map[6] = PCM_CHANNEL_CS;
        break;
    case 0x10:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LS;
        ch_map[3] = PCM_CHANNEL_RS;
        ch_map[4] = PCM_CHANNEL_LB;
        ch_map[5] = PCM_CHANNEL_RB;
        break;
    case 0x11:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_LS;
        ch_map[4] = PCM_CHANNEL_RS;
        ch_map[5] = PCM_CHANNEL_LB;
        ch_map[6] = PCM_CHANNEL_RB;
        break;
    case 0x12:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_C;
        ch_map[3] = PCM_CHANNEL_LS;
        ch_map[4] = PCM_CHANNEL_RS;
        ch_map[5] = PCM_CHANNEL_LB;
        ch_map[6] = PCM_CHANNEL_RB;
        break;
    case 0x13:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_C;
        ch_map[4] = PCM_CHANNEL_LS;
        ch_map[5] = PCM_CHANNEL_RS;
        ch_map[6] = PCM_CHANNEL_LB;
        ch_map[7] = PCM_CHANNEL_RB;
        break;
    case 0x14:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_FLC;
        ch_map[3] = PCM_CHANNEL_FRC;
        break;
    case 0x15:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_FLC;
        ch_map[4] = PCM_CHANNEL_FRC;
        break;
    case 0x16:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_C;
        ch_map[3] = PCM_CHANNEL_FLC;
        ch_map[4] = PCM_CHANNEL_FRC;
        break;
    case 0x17:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_C;
        ch_map[4] = PCM_CHANNEL_FLC;
        ch_map[5] = PCM_CHANNEL_FRC;
        break;
    case 0x18:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_CS;
        ch_map[3] = PCM_CHANNEL_FLC;
        ch_map[4] = PCM_CHANNEL_FRC;
        break;
    case 0x19:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_CS;
        ch_map[4] = PCM_CHANNEL_FLC;
        ch_map[5] = PCM_CHANNEL_FRC;
        break;
    case 0x1a:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_C;
        ch_map[3] = PCM_CHANNEL_CS;
        ch_map[4] = PCM_CHANNEL_FLC;
        ch_map[5] = PCM_CHANNEL_FRC;
        break;
    case 0x1b:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_C;
        ch_map[4] = PCM_CHANNEL_CS;
        ch_map[5] = PCM_CHANNEL_FLC;
        ch_map[6] = PCM_CHANNEL_FRC;
        break;
    case 0x1c:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LS;
        ch_map[3] = PCM_CHANNEL_RS;
        ch_map[4] = PCM_CHANNEL_FLC;
        ch_map[5] = PCM_CHANNEL_FRC;
        break;
    case 0x1d:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_LS;
        ch_map[4] = PCM_CHANNEL_RS;
        ch_map[5] = PCM_CHANNEL_FLC;
        ch_map[6] = PCM_CHANNEL_FRC;
        break;
    case 0x1e:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_C;
        ch_map[3] = PCM_CHANNEL_LS;
        ch_map[4] = PCM_CHANNEL_RS;
        ch_map[5] = PCM_CHANNEL_FLC;
        ch_map[6] = PCM_CHANNEL_FRC;
        break;
    case 0x1f:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_C;
        ch_map[4] = PCM_CHANNEL_LS;
        ch_map[5] = PCM_CHANNEL_RS;
        ch_map[6] = PCM_CHANNEL_FLC;
        ch_map[7] = PCM_CHANNEL_FRC;
        break;
    case 0x2f:
        ch_map[0] = PCM_CHANNEL_L;
        ch_map[1] = PCM_CHANNEL_R;
        ch_map[2] = PCM_CHANNEL_LFE;
        ch_map[3] = PCM_CHANNEL_C;
        ch_map[4] = PCM_CHANNEL_LS;
        ch_map[5] = PCM_CHANNEL_RS;
        ch_map[6] = 0; // PCM_CHANNEL_TFL; but not defined by LPASS
        ch_map[7] = 0; // PCM_CHANNEL_TFR; but not defined by LPASS
        break;
    default:
        break;
    }
    PAL_DBG(LOG_TAG," channel map updated to [%d %d %d %d %d %d %d %d ]",
          ch_map[0], ch_map[1], ch_map[2],
          ch_map[3], ch_map[4], ch_map[5],
          ch_map[6], ch_map[7]);
}

void DisplayPort::updateChannelMapLpass(edidAudioInfo* info)
{
    if (!info)
        return;

    retrieveChannelMapLpass(info->channelAllocation, (uint8_t *)&info->channelMap[0],
            MAX_CHANNELS_SUPPORTED);
}

void DisplayPort::updateChannelMask(edidAudioInfo* info)
{
    if (!info)
        return;
    if (((info->channelAllocation < 0) ||
         (info->channelAllocation > 0x1f)) &&
         (info->channelAllocation != 0x2f)) {
        PAL_ERR(LOG_TAG,"Channel allocation out of supported range");
        return;
    }
    PAL_VERBOSE(LOG_TAG,"channelAllocation 0x%x", info->channelAllocation);
    // Don't distinguish channel mask below?
    // AUDIO_CHANNEL_OUT_5POINT1 and AUDIO_CHANNEL_OUT_5POINT1_SIDE
    // AUDIO_CHANNEL_OUT_QUAD and AUDIO_CHANNEL_OUT_QUAD_SIDE
    switch(info->channelAllocation) {
    case 0x0:
        info->channelMask = AUDIO_CHANNEL_OUT_STEREO;
        break;
    case 0x1:
        info->channelMask = AUDIO_CHANNEL_OUT_2POINT1;
        break;
    case 0x2:
        info->channelMask = AUDIO_CHANNEL_OUT_STEREO;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_CENTER;
        break;
    case 0x3:
        info->channelMask = AUDIO_CHANNEL_OUT_2POINT1;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_CENTER;
        break;
    case 0x4:
        info->channelMask = AUDIO_CHANNEL_OUT_STEREO;
        info->channelMask |= AUDIO_CHANNEL_OUT_BACK_CENTER;
        break;
    case 0x5:
        info->channelMask = AUDIO_CHANNEL_OUT_2POINT1;
        info->channelMask |= AUDIO_CHANNEL_OUT_LOW_FREQUENCY;
        info->channelMask |= AUDIO_CHANNEL_OUT_BACK_CENTER;
        break;
    case 0x6:
        info->channelMask = AUDIO_CHANNEL_OUT_SURROUND;
        break;
    case 0x7:
        info->channelMask = AUDIO_CHANNEL_OUT_SURROUND;
        info->channelMask |= AUDIO_CHANNEL_OUT_LOW_FREQUENCY;
        break;
    case 0x8:
        info->channelMask = AUDIO_CHANNEL_OUT_QUAD;
        break;
    case 0x9:
        info->channelMask = AUDIO_CHANNEL_OUT_QUAD;
        info->channelMask |= AUDIO_CHANNEL_OUT_LOW_FREQUENCY;
        break;
    case 0xa:
        info->channelMask = AUDIO_CHANNEL_OUT_PENTA;
        break;
    case 0xb:
        info->channelMask = AUDIO_CHANNEL_OUT_5POINT1;
        break;
    case 0xc:
        info->channelMask = AUDIO_CHANNEL_OUT_QUAD;
        info->channelMask |= AUDIO_CHANNEL_OUT_BACK_CENTER;
        break;
    case 0xd:
        info->channelMask = AUDIO_CHANNEL_OUT_QUAD;
        info->channelMask |= AUDIO_CHANNEL_OUT_LOW_FREQUENCY;
        info->channelMask |= AUDIO_CHANNEL_OUT_BACK_CENTER;
        break;
    case 0xe:
        info->channelMask = AUDIO_CHANNEL_OUT_PENTA;
        info->channelMask |= AUDIO_CHANNEL_OUT_BACK_CENTER;
        break;
    case 0xf:
        info->channelMask = AUDIO_CHANNEL_OUT_5POINT1;
        info->channelMask |= AUDIO_CHANNEL_OUT_BACK_CENTER;
        break;
    case 0x10:
        info->channelMask = AUDIO_CHANNEL_OUT_QUAD;
        info->channelMask |= AUDIO_CHANNEL_OUT_SIDE_LEFT;
        info->channelMask |= AUDIO_CHANNEL_OUT_SIDE_RIGHT;
        break;
    case 0x11:
        info->channelMask = AUDIO_CHANNEL_OUT_QUAD;
        info->channelMask |= AUDIO_CHANNEL_OUT_LOW_FREQUENCY;
        info->channelMask |= AUDIO_CHANNEL_OUT_SIDE_LEFT;
        info->channelMask |= AUDIO_CHANNEL_OUT_SIDE_RIGHT;
        break;
    case 0x12:
        info->channelMask = AUDIO_CHANNEL_OUT_QUAD;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_SIDE_LEFT;
        info->channelMask |= AUDIO_CHANNEL_OUT_SIDE_RIGHT;
        break;
    case 0x13:
        info->channelMask = AUDIO_CHANNEL_OUT_7POINT1;
        break;
    case 0x14:
        info->channelMask = AUDIO_CHANNEL_OUT_FRONT_LEFT;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
        break;
    case 0x15:
        info->channelMask = AUDIO_CHANNEL_OUT_2POINT1;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
        break;
    case 0x16:
        info->channelMask = AUDIO_CHANNEL_OUT_FRONT_LEFT;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
        break;
    case 0x17:
        info->channelMask = AUDIO_CHANNEL_OUT_2POINT1;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
        break;
    case 0x18:
        info->channelMask = AUDIO_CHANNEL_OUT_FRONT_LEFT;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT;
        info->channelMask |= AUDIO_CHANNEL_OUT_BACK_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
        break;
    case 0x19:
        info->channelMask = AUDIO_CHANNEL_OUT_2POINT1;
        info->channelMask |= AUDIO_CHANNEL_OUT_BACK_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
        break;
    case 0x1a:
        info->channelMask = AUDIO_CHANNEL_OUT_SURROUND;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
        break;
    case 0x1b:
        info->channelMask = AUDIO_CHANNEL_OUT_SURROUND;
        info->channelMask |= AUDIO_CHANNEL_OUT_LOW_FREQUENCY;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
        break;
    case 0x1c:
        info->channelMask = AUDIO_CHANNEL_OUT_QUAD;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
        break;
    case 0x1d:
        info->channelMask = AUDIO_CHANNEL_OUT_QUAD;
        info->channelMask |= AUDIO_CHANNEL_OUT_LOW_FREQUENCY;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
        break;
    case 0x1e:
        info->channelMask = AUDIO_CHANNEL_OUT_PENTA;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
        break;
    case 0x1f:
        info->channelMask = AUDIO_CHANNEL_OUT_5POINT1;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
        info->channelMask |= AUDIO_CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
        break;
    case 0x2f:
        info->channelMask = AUDIO_CHANNEL_OUT_5POINT1POINT2;
        break;
    default:
        break;
    }
    PAL_DBG(LOG_TAG," channel mask updated to %d", info->channelMask);
}

void DisplayPort::dumpEdidData(edidAudioInfo *info)
{

    int i;
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    for (i = 0; i < info->audioBlocks && i < MAX_EDID_BLOCKS; i++) {
        PAL_DBG(LOG_TAG,"FormatId:%d rate:%d bps:%d channels:%d",
              info->audioBlocksArray[i].formatId,
              info->audioBlocksArray[i].samplingFreqBitmask,
              info->audioBlocksArray[i].bitsPerSampleBitmask,
              info->audioBlocksArray[i].channels);
    }
    PAL_DBG(LOG_TAG,"no of audio blocks:%d", info->audioBlocks);
    PAL_DBG(LOG_TAG,"speaker allocation:[%x %x %x]",
           info->speakerAllocation[0], info->speakerAllocation[1],
           info->speakerAllocation[2]);
    PAL_DBG(LOG_TAG,"channel map:[%x %x %x %x %x %x %x %x]",
           info->channelMap[0], info->channelMap[1],
           info->channelMap[2], info->channelMap[3],
           info->channelMap[4], info->channelMap[5],
           info->channelMap[6], info->channelMap[7]);
    PAL_DBG(LOG_TAG,"channel allocation:%d", info->channelAllocation);
    PAL_DBG(LOG_TAG,"[%d %d %d %d %d %d %d %d ]",
           info->channelMap[0], info->channelMap[1],
           info->channelMap[2], info->channelMap[3],
           info->channelMap[4], info->channelMap[5],
           info->channelMap[6], info->channelMap[7]);
#else
    for (i = 0; i < info->audioBlocks && i < MAX_EDID_BLOCKS; i++) {
        PAL_VERBOSE(LOG_TAG,"FormatId:%d rate:%d bps:%d channels:%d",
              info->audioBlocksArray[i].formatId,
              info->audioBlocksArray[i].samplingFreqBitmask,
              info->audioBlocksArray[i].bitsPerSampleBitmask,
              info->audioBlocksArray[i].channels);
    }
    PAL_VERBOSE(LOG_TAG,"no of audio blocks:%d", info->audioBlocks);
    PAL_VERBOSE(LOG_TAG,"speaker allocation:[%x %x %x]",
           info->speakerAllocation[0], info->speakerAllocation[1],
           info->speakerAllocation[2]);
    PAL_VERBOSE(LOG_TAG,"channel map:[%x %x %x %x %x %x %x %x]",
           info->channelMap[0], info->channelMap[1],
           info->channelMap[2], info->channelMap[3],
           info->channelMap[4], info->channelMap[5],
           info->channelMap[6], info->channelMap[7]);
    PAL_VERBOSE(LOG_TAG,"channel allocation:%d", info->channelAllocation);
    PAL_VERBOSE(LOG_TAG,"[%d %d %d %d %d %d %d %d ]",
           info->channelMap[0], info->channelMap[1],
           info->channelMap[2], info->channelMap[3],
           info->channelMap[4], info->channelMap[5],
           info->channelMap[6], info->channelMap[7]);
#endif
}

bool DisplayPort::getSinkCaps(edidAudioInfo* info, char *edidData)
{
    unsigned char channels[MAX_EDID_BLOCKS];
    unsigned char formats[MAX_EDID_BLOCKS];
    unsigned char frequency[MAX_EDID_BLOCKS];
    unsigned char bitrate[MAX_EDID_BLOCKS];
    int i = 0;
    int length, countDesc;

    if (!info || !edidData) {
        PAL_ERR(LOG_TAG,"No valid EDID");
        return false;
    }

    length = (int) *edidData++;
    PAL_VERBOSE(LOG_TAG,"Total length is %d",length);

    countDesc = length/MIN_AUDIO_DESC_LENGTH;

    if (!countDesc) {
        PAL_ERR(LOG_TAG,"insufficient descriptors");
        return false;
    }

    memset(info, 0, sizeof(edidAudioInfo));

    info->audioBlocks = countDesc-1;
    if (info->audioBlocks > MAX_EDID_BLOCKS) {
        info->audioBlocks = MAX_EDID_BLOCKS;
    }

    PAL_VERBOSE(LOG_TAG,"Total # of audio descriptors %d",countDesc);

    for (i=0; i<info->audioBlocks; i++) {
        // last block for speaker allocation;
        channels [i]   = (*edidData & 0x7) + 1;
        formats  [i]   = (*edidData++) >> 3;
        frequency[i]   = *edidData++;
        bitrate  [i]   = *edidData++;
    }
    info->speakerAllocation[0] = *edidData++;
    info->speakerAllocation[1] = *edidData++;
    info->speakerAllocation[2] = *edidData++;

    updateChannelMap(info);
    updateChannelAllocation(info);
    updateChannelMapLpass(info);
    updateChannelMask(info);

    for (i=0; i<info->audioBlocks; i++) {
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
        PAL_DBG(LOG_TAG,"AUDIO DESC BLOCK # %d\n",i);

        info->audioBlocksArray[i].channels = channels[i];
        PAL_DBG(LOG_TAG,"info->audioBlocksArray[i].channels %d\n",
              info->audioBlocksArray[i].channels);

        PAL_DBG(LOG_TAG,"Format Byte %d\n", formats[i]);
        info->audioBlocksArray[i].formatId = (edidAudioFormatId)formats[i];
        PAL_DBG(LOG_TAG,"info->audioBlocksArray[i].formatId %s",
             edidFormatToStr(formats[i]));

        PAL_DBG(LOG_TAG,"Frequency Bitmask %d\n", frequency[i]);
        info->audioBlocksArray[i].samplingFreqBitmask = frequency[i];
        PAL_DBG(LOG_TAG,"info->audioBlocksArray[i].samplingFreqBitmask %d",
              info->audioBlocksArray[i].samplingFreqBitmask);

        PAL_DBG(LOG_TAG,"BitsPerSample Bitmask %d\n", bitrate[i]);
        info->audioBlocksArray[i].bitsPerSampleBitmask =
                   getEdidBpsByte(bitrate[i],formats[i]);
        PAL_DBG(LOG_TAG,"info->audioBlocksArray[i].bitsPerSampleBitmask %d",
              info->audioBlocksArray[i].bitsPerSampleBitmask);
#else
        PAL_VERBOSE(LOG_TAG,"AUDIO DESC BLOCK # %d\n",i);

        info->audioBlocksArray[i].channels = channels[i];
        PAL_DBG(LOG_TAG,"info->audioBlocksArray[i].channels %d\n",
              info->audioBlocksArray[i].channels);

        PAL_VERBOSE(LOG_TAG,"Format Byte %d\n", formats[i]);
        info->audioBlocksArray[i].formatId = (edidAudioFormatId)formats[i];
        PAL_DBG(LOG_TAG,"info->audioBlocksArray[i].formatId %s",
             edidFormatToStr(formats[i]));

        PAL_VERBOSE(LOG_TAG,"Frequency Bitmask %d\n", frequency[i]);
        info->audioBlocksArray[i].samplingFreqBitmask = frequency[i];
        PAL_VERBOSE(LOG_TAG,"info->audioBlocksArray[i].samplingFreqBitmask %d",
              info->audioBlocksArray[i].samplingFreqBitmask);

        PAL_VERBOSE(LOG_TAG,"BitsPerSample Bitmask %d\n", bitrate[i]);
        info->audioBlocksArray[i].bitsPerSampleBitmask =
                   getEdidBpsByte(bitrate[i],formats[i]);
        PAL_VERBOSE(LOG_TAG,"info->audioBlocksArray[i].bitsPerSampleBitmask %d",
              info->audioBlocksArray[i].bitsPerSampleBitmask);
#endif
    }
    dumpSpeakerAllocation(info);
    dumpEdidData(info);
    return true;
}

bool DisplayPort::isSupportedSR(edidAudioInfo* info, int sr)
{
    int i = 0;
    struct extDispState *state = NULL;

    state = &extDisp[dp_controller][dp_stream];
    if (state && state->edidInfo)
    {
        info = (edidAudioInfo*) state->edidInfo;
    }
    if (info != NULL && sr != 0) {
        for (i = 0; i < info->audioBlocks && i < MAX_EDID_BLOCKS; i++) {
        if (isSampleRateSupported(info->audioBlocksArray[i].samplingFreqBitmask,
                    sr)) {
                PAL_DBG(LOG_TAG," Returns true for sample rate [%d]", sr);
                return true;
            }
        }
    }
    PAL_ERR(LOG_TAG," Returns false for sample rate [%d]", sr);
    return false;
}

int DisplayPort::getMaxChannel()
{
    int i = 0;
    struct extDispState *state = NULL;
    int max_channel = 2;
    edidAudioInfo *info = NULL;

    state = &extDisp[dp_controller][dp_stream];
    if (state && state->edidInfo)
    {
        info = (edidAudioInfo*) state->edidInfo;
    }

    if (info != NULL) {
        for (i = 0; i < info->audioBlocks && i < MAX_EDID_BLOCKS; i++) {
            if (info->audioBlocksArray[i].formatId == LPCM) {
                if (max_channel < info->audioBlocksArray[i].channels) {
                    max_channel = info->audioBlocksArray[i].channels;
                    PAL_DBG(LOG_TAG," Max channels updated to [%d]", max_channel);
                }
            }
        }
    }
    return max_channel;
}

bool DisplayPort::isSupportedBps(edidAudioInfo* info, int bps)
{
    int i = 0;

    if (bps == 16) {
        //16 bit bps is always supported
        //some oem may not update 16bit support in their edid info
        return true;
    }

    if (info != NULL && bps != 0) {
        for (i = 0; i < info->audioBlocks && i < MAX_EDID_BLOCKS; i++) {
            if (isSupportedBps(info->audioBlocksArray[i].bitsPerSampleBitmask, bps)) {
                PAL_VERBOSE(LOG_TAG," returns true for bit width [%d]", bps);
                return true;
            }
        }
    }
    PAL_VERBOSE(LOG_TAG," returns false for bit width [%d]", bps);
    return false;
}

int DisplayPort::getHighestSupportedSR()
{
    int sr = 0;
    int highestSR = 0;
    int i;
    struct extDispState *state = NULL;
    edidAudioInfo *info = NULL;

    state = &extDisp[dp_controller][dp_stream];
    if (state && state->edidInfo)
    {
        info = (edidAudioInfo*) state->edidInfo;
    }

    if (info != NULL) {
        for (i = 0; i < info->audioBlocks && i < MAX_EDID_BLOCKS; i++) {
          sr = getHighestEdidSF(info->audioBlocksArray[i].samplingFreqBitmask);
          if (sr > highestSR)
            highestSR = sr;
        }
    }
    else {
        PAL_ERR(LOG_TAG," info is NULL");
        highestSR = SAMPLINGRATE_48K;
    }

    if (highestSR == 0) {
        PAL_ERR(LOG_TAG,"Unable to get Highest SR. Setting default SR");
        highestSR = SAMPLINGRATE_48K;
    }

    PAL_VERBOSE(LOG_TAG," returns [%d] for highest supported sr", highestSR);
    return highestSR;
}

int DisplayPort::getHighestSupportedBps()
{
    int bpsMask = 0;
    int highestBps = 0;
    int i;
    struct extDispState *state = NULL;
    edidAudioInfo *info = NULL;

    state = &extDisp[dp_controller][dp_stream];
    if (state && state->edidInfo)
    {
        info = (edidAudioInfo*) state->edidInfo;
    }

    if (info != NULL) {
        for (i = 0; i < info->audioBlocks && i < MAX_EDID_BLOCKS; i++) {
            bpsMask = info->audioBlocksArray[i].bitsPerSampleBitmask;
            if (isSupportedBps(bpsMask, 24)) {
                highestBps = 24;
                break;
            }
            else if (isSupportedBps(bpsMask, BITWIDTH_16))
                if (highestBps < BITWIDTH_16)
                    highestBps = BITWIDTH_16;
        }
    }

    if (highestBps == 0) {
        PAL_ERR(LOG_TAG, "None of the supported BPS is highest");
        highestBps = 16;
    }
    return highestBps;
}
