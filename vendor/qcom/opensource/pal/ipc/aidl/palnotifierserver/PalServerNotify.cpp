/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PalServerNotify"
#include "PalServerNotify.h"
#include <aidl/vendor/qti/hardware/pal/PalMessageQueueFlagBits.h>
#include <cutils/list.h>
#include <cutils/android_filesystem_config.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <pal/BinderStatus.h>
#include <pal/SharedMemoryWrapper.h>
#include <pal/Utils.h>

using ndk::ScopedAStatus;

namespace aidl::vendor::qti::hardware::paleventnotifier {

static list_declare(client_list);
static pthread_mutex_t client_list_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    struct listnode list;
    uint32_t pid;
    std::shared_ptr<IPALEventNotifierCallback> pal_clbk;
    bool isnotified;
} clientInfo;

int handle_pal_cb(pal_callback_config_t *config, uint32_t event, bool isregister) {

    ALOGD("handle_pal_cb event is %d", event);
    clientInfo *client_handle = NULL;
    struct listnode* node = NULL;
    uint16_t inChannels = 0;
    uint16_t outChannels = 0;
    inChannels = config->streamAttributes.in_media_config.ch_info.channels;
    outChannels = config->streamAttributes.out_media_config.ch_info.channels;
    PalCallbackConfig palCallbackConfig;
    if(config) {
        palCallbackConfig.currentDevices.resize(config->noOfCurrentDevices);
        for(int i = 0; i < config->noOfCurrentDevices; i++){
            palCallbackConfig.currentDevices[i] = (PalDeviceId)config->currentDevices[i];
        }
        if(event == PAL_NOTIFY_DEVICESWITCH) {
            palCallbackConfig.prevDevices.resize(config->noOfPrevDevices);
            for(int i = 0; i < config->noOfPrevDevices; i++){
                palCallbackConfig.prevDevices[i] = (PalDeviceId)config->prevDevices[i];
            }
        }
        palCallbackConfig.noOfCurrentDevices = config->noOfCurrentDevices;
        palCallbackConfig.noOfPrevDevices = config->noOfPrevDevices;
        palCallbackConfig.streamAttributes.type = (PalStreamType)config->streamAttributes.type;
        palCallbackConfig.streamAttributes.info.version = config->streamAttributes.info.opt_stream_info.version;
        palCallbackConfig.streamAttributes.info.size = config->streamAttributes.info.opt_stream_info.size;
        palCallbackConfig.streamAttributes.info.durationUs = config->streamAttributes.info.opt_stream_info.duration_us;
        palCallbackConfig.streamAttributes.info.rxProxyType = config->streamAttributes.info.opt_stream_info.rx_proxy_type;
        palCallbackConfig.streamAttributes.info.txProxyType = config->streamAttributes.info.opt_stream_info.tx_proxy_type;
        palCallbackConfig.streamAttributes.info.hasVideo = config->streamAttributes.info.opt_stream_info.has_video;
        palCallbackConfig.streamAttributes.info.isStreaming = config->streamAttributes.info.opt_stream_info.is_streaming;
        palCallbackConfig.streamAttributes.info.loopbackType = config->streamAttributes.info.opt_stream_info.loopback_type;
        palCallbackConfig.streamAttributes.info.hapticsType = config->streamAttributes.info.opt_stream_info.haptics_type;
        palCallbackConfig.streamAttributes.flags = (PalStreamFlag)config->streamAttributes.flags;
        palCallbackConfig.streamAttributes.direction = (PalStreamDirection)config->streamAttributes.direction;
        palCallbackConfig.streamAttributes.inMediaConfig.sampleRate = config->streamAttributes.in_media_config.sample_rate;
        palCallbackConfig.streamAttributes.inMediaConfig.bitwidth = config->streamAttributes.in_media_config.bit_width;
        palCallbackConfig.streamAttributes.outMediaConfig.sampleRate = config->streamAttributes.out_media_config.sample_rate;
        palCallbackConfig.streamAttributes.outMediaConfig.bitwidth = config->streamAttributes.out_media_config.bit_width;
        if(inChannels) {
            palCallbackConfig.streamAttributes.inMediaConfig.chInfo.channels = config->streamAttributes.in_media_config.ch_info.channels;
            memset(&palCallbackConfig.streamAttributes.inMediaConfig.chInfo.chMap, 0, sizeof(uint8_t[PAL_MAX_CHANNELS_SUPPORTED]));
            memcpy(&palCallbackConfig.streamAttributes.inMediaConfig.chInfo.chMap, &config->streamAttributes.in_media_config.ch_info.ch_map, inChannels);
        }
        palCallbackConfig.streamAttributes.inMediaConfig.audioFormatId = (PalAudioFmt)config->streamAttributes.in_media_config.aud_fmt_id;
        if(outChannels) {
            palCallbackConfig.streamAttributes.outMediaConfig.chInfo.channels = config->streamAttributes.out_media_config.ch_info.channels;
            memset(&palCallbackConfig.streamAttributes.outMediaConfig.chInfo.chMap, 0, sizeof(uint8_t[PAL_MAX_CHANNELS_SUPPORTED]));
            memcpy(&palCallbackConfig.streamAttributes.outMediaConfig.chInfo.chMap, &config->streamAttributes.out_media_config.ch_info.ch_map, outChannels);
        }
        palCallbackConfig.streamAttributes.outMediaConfig.audioFormatId = (PalAudioFmt)config->streamAttributes.out_media_config.aud_fmt_id;
    }

    list_for_each(node, &client_list) {
        client_handle = node_to_item(node, clientInfo, list);
        if(isregister) {
            if(client_handle->isnotified == false)
                client_handle->pal_clbk->onStart(palCallbackConfig);
        }
        else {
            if(event == PAL_NOTIFY_START) {
                ALOGD("PAL_NOTIFY_START notification for client %d", client_handle->pid);
                client_handle->pal_clbk->onStart(palCallbackConfig);
            }
            else if (event == PAL_NOTIFY_STOP) {
                ALOGD("PAL_NOTIFY_STOP notification for client %d", client_handle->pid);
                client_handle->pal_clbk->onStop(palCallbackConfig);
            }
            else if (event == PAL_NOTIFY_DEVICESWITCH) {
                ALOGD("PAL_NOTIFY_DEVICESWITCH notification for client %d", client_handle->pid);
                client_handle->pal_clbk->onDeviceSwitch(palCallbackConfig);
            }
        }
    }
    return 0;
}

::ndk::ScopedAStatus PalServerNotify::ipc_pal_notify_register_callback(const std::shared_ptr<IPALEventNotifierCallback>& cb, int* ret) {
    int pid = AIBinder_getCallingPid();
    int status = 0;
    clientInfo *client_handle = NULL;
    struct listnode* node = NULL;
    int clientUnregistered = -EINVAL;

    if (cb == NULL) {
        pthread_mutex_lock(&client_list_lock);
        list_for_each(node, &client_list) {
            client_handle = node_to_item(node, clientInfo, list);
            if (client_handle->pid == pid) {
                list_remove(node);
                free(client_handle);
                clientUnregistered = 0;
                break;
            }
        }
        pthread_mutex_unlock(&client_list_lock);
        return status_tToBinderResult(clientUnregistered);
    }
    pthread_mutex_lock(&client_list_lock);
    list_for_each(node, &client_list) {
        client_handle = node_to_item(node, clientInfo, list);
        if (client_handle->pid == pid)
            goto registered_cb;
    }
    client_handle = (clientInfo *)calloc(1, sizeof(clientInfo));
    if (client_handle == NULL) {
        ALOGE("%s: Cannot allocate memory for client handle\n", __func__);
        pthread_mutex_unlock(&client_list_lock);
        return status_tToBinderResult(-ENOMEM);
    }
    client_handle->pid = pid;
    list_add_tail(&client_list, &client_handle->list);
registered_cb:
    client_handle->pal_clbk = cb;
    pthread_mutex_unlock(&client_list_lock);
    status = pal_register_for_events(handle_pal_cb);
    client_handle->isnotified = true;
    return status_tToBinderResult(status);
}

}
