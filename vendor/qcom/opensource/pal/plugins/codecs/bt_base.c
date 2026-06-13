/*
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 */

#define LOG_TAG "PAL: bt_base"

#include "bt_base.h"
#include <common_enc_dec_api.h>
#include <media_fmt_api.h>
#include <errno.h>
#include <stdlib.h>

int bt_base_populate_real_module_id(custom_block_t *blk, int32_t real_module_id)
{
    struct param_id_placeholder_real_module_id_t *param = NULL;

    blk->param_id = PARAM_ID_REAL_MODULE_ID;
    blk->payload_sz = sizeof(struct param_id_placeholder_real_module_id_t);
    blk->payload = calloc(1, blk->payload_sz);
    if (!blk->payload) {
        blk->payload_sz = 0;
        return -ENOMEM;
    }

    param = (struct param_id_placeholder_real_module_id_t*) blk->payload;
    param->real_module_id = real_module_id;
    return 0;
}

int bt_base_populate_enc_bitrate(custom_block_t *blk, int32_t bit_rate)
{
    struct param_id_enc_bitrate_param_t *param = NULL;

    blk->param_id = PARAM_ID_ENC_BITRATE;
    blk->payload_sz = sizeof(struct param_id_enc_bitrate_param_t);
    blk->payload = calloc(1, blk->payload_sz);
    if (!blk->payload) {
        blk->payload_sz = 0;
        return -ENOMEM;
    }

    param = (struct param_id_enc_bitrate_param_t*) blk->payload;
    param->bitrate = bit_rate;
    return 0;
}

int bt_base_populate_enc_output_cfg(custom_block_t *blk, uint32_t fmt_id,
                                    void *payload, size_t size)
{
    struct param_id_encoder_output_config_t *param = NULL;
    void *enc_payload;

    blk->param_id = PARAM_ID_ENCODER_OUTPUT_CONFIG;
    blk->payload_sz = sizeof(struct param_id_encoder_output_config_t) + size;
    blk->payload = (uint8_t *)calloc(1, blk->payload_sz);
    if (!blk->payload) {
        blk->payload_sz = 0;
        return -ENOMEM;
    }
    param = (struct param_id_encoder_output_config_t *) blk->payload;
    param->data_format  = DATA_FORMAT_FIXED_POINT;
    param->fmt_id       = fmt_id;
    param->payload_size = size;
    enc_payload = (void *)(blk->payload + sizeof(struct param_id_encoder_output_config_t));
    memcpy(enc_payload, payload, size);

    return 0;
}

int bt_base_populate_enc_cmn_param(custom_block_t *blk, uint32_t param_id,
                                 void *payload, size_t size)
{
    blk->param_id = param_id;
    blk->payload_sz = size;
    blk->payload = (uint8_t *)calloc(1, blk->payload_sz);
    if (!blk->payload) {
        blk->payload_sz = 0;
        return -ENOMEM;
    }
    memcpy(blk->payload, payload, size);

    return 0;
}
