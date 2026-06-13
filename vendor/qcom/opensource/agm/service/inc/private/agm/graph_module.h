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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 */
#ifndef GPH_MODULE_H
#define GPH_MODULE_H

#include <agm/agm_list.h>
#include <agm/device.h>

/*Platfrom Key Value file, defines tag keys and their values*/
#include "kvh2xml.h"

/*Module specific header files used.*/
#include "apm_api.h"
#include "module_cmn_api.h"
#include "wr_sh_mem_ep_api.h"
#include "rd_sh_mem_ep_api.h"
#include "common_enc_dec_api.h"
#include "hw_intf_cmn_api.h"
#include "codec_dma_api.h"
#include "i2s_api.h"
#include "pcm_tdm_api.h"
#include "slimbus_api.h"
#include "spr_api.h"
#include "gapless_api.h"
#include "pcm_encoder_api.h"
#include "aac_encoder_api.h"

/*
 *Internal enum to identify different modules
 *Graph object only should have basic modules which
 *would be implicitly configured by it.
 *For all other modules the client is supposed to form the
 *payload and pass it as an blob, which is then sent to
 *SPF via gsl.
 *NOTE:Ensure that the hw_ep_info.intf in device object
 *also uses the same enum values to define a hw
 *interface.
*/

#define TAG_STREAM_SPR       0xC0000013
#define ALIGN_PAYLOAD(x,a)   x = (((x) + (size_t)(a-1)) & ~(size_t)(a-1))
#define PARAM_VAL_NATIVE     -1

typedef enum module
{
    MODULE_STREAM_START = 0,
    MODULE_PCM_ENCODER = MODULE_STREAM_START,
    MODULE_PCM_DECODER,
    MODULE_PCM_CONVERTER,
    MODULE_WR_SHARED_MEM,
    MODULE_PLACEHOLDER_ENCODER,
    MODULE_PLACEHOLDER_DECODER,
    MODULE_STREAM_PAUSE,
    MODULE_STREAM_SPR,
    MODULE_STREAM_GAPLESS,
    MODULE_RD_SHARED_MEM,
    /*
     *Ensure that whenever a new stream module is added it
     *is added in the end of stream module list and the end
     *is updated with the same entry.
     */
    MODULE_STREAM_END = MODULE_RD_SHARED_MEM,
    MODULE_DEVICE_START = 0,
    MODULE_HW_EP_RX = MODULE_DEVICE_START,
    MODULE_HW_EP_TX,
    /*
     *Ensure that whenever a new device module is added it
     *is added in the end of device module list and the end
     *is updated with the same entry.
     */
    MODULE_DEVICE_END = MODULE_HW_EP_RX,
}module_t;


enum  channel_num{
    CHANNEL_1 = 1,
    CHANNEL_2,
    CHANNEL_3,
    CHANNEL_4,
    CHANNEL_5,
    CHANNEL_6,
    CHANNEL_7,
    CHANNEL_8,
};

struct module_info {
    struct listnode list;
    /*local enum based module identification*/
    module_t module;
    /*module tag defined in the platform header file exported to ACDB*/
    uint32_t tag;
    /*module id querried from ACDB*/
    uint32_t mid;
    /*module instance id querried from ACDB*/
    uint32_t miid;
    /*indicates if this module instance was configured*/
    bool is_configured;
    /*Device object if this module is associated with an hardware end point*/
    struct device_obj *dev_obj;
    /*GKV which contains/describes this module*/
    struct agm_key_vector_gsl *gkv;
    /*
     *Every module defines its configuration api, which in turn
     *then is used by graph object to configure the module using
     *gsl_set_config/gsl_set_custom_config api's.
     */
    int (*configure)(struct module_info *mod, struct graph_obj *gph_obj);
};

typedef struct module_info module_info_t;

struct graph_buf_info {
    /* timestamp updated in struct gsl_buff on gsl_read */
    uint64_t timestamp;
};

struct graph_obj {
    pthread_mutex_t lock;
    pthread_mutex_t gph_open_thread_lock;
    pthread_t gph_open_thread;
    pthread_cond_t gph_opened;
    bool gph_open_thread_created;
    graph_state_t state;
    gsl_handle_t graph_handle;
    struct listnode tagged_mod_list;
    struct gsl_cmd_configure_read_write_params buf_config;
    event_cb cb;
    void *client_data;
    struct session_obj *sess_obj;
    uint32_t spr_miid;
    struct graph_buf_info buf_info;
    bool is_config_buf_params_done;
};

void get_stream_module_list_array(module_info_t **info, size_t *size);
void get_hw_ep_module_list_array(module_info_t **info, size_t *size);

#endif /*GPH_MODULE_H*/
