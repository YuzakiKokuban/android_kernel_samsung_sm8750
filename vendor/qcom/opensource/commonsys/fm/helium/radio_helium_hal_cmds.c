/*
Copyright (c) 2015, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <utils/Log.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "radio-helium-commands.h"
#include "radio-helium.h"
#include "fm_hci_api.h"
#include <dlfcn.h>
#undef LOG_TAG
#define LOG_TAG "radio_helium"
extern struct fm_hal_t *hal;

static int send_fm_cmd_pkt(uint16_t opcode,  uint32_t len, void *param)
{
    int p_len = 3 + len;
    int ret = 0;
    ALOGV("Send_fm_cmd_pkt, opcode: %x", opcode);

    struct fm_command_header_t *hdr = (struct fm_command_header_t *) malloc(p_len);
    if (!hdr) {
        ALOGE("%s:hdr allocation failed", LOG_TAG);
        return -FM_HC_STATUS_NOMEM;
    }

    memset(hdr, 0, p_len);
    ALOGV("%s:opcode: %x", LOG_TAG, opcode);

    hdr->opcode = opcode;
    hdr->len = len;
    if (len)
        memcpy(hdr->params, (uint8_t *)param, len);
    ret = fm_hci_transmit(hal->private_data, hdr);

    ALOGV("%s:transmit done. status = %d", __func__, ret);
    return ret;
}

int hci_fm_get_signal_threshold()
{
    uint16_t opcode = 0;

    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
            HCI_OCF_FM_GET_SIGNAL_THRESHOLD);
    return send_fm_cmd_pkt(opcode, 0, NULL);
}

int hci_fm_enable_recv_req()
{
    uint16_t opcode = 0;

    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                                  HCI_OCF_FM_ENABLE_RECV_REQ);
    return send_fm_cmd_pkt(opcode, 0, NULL);
}

int hci_fm_disable_recv_req()
{
  uint16_t opcode = 0;

  opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                                 HCI_OCF_FM_DISABLE_RECV_REQ);
  return send_fm_cmd_pkt(opcode, 0, NULL);
}

int  hci_fm_mute_mode_req(struct hci_fm_mute_mode_req *mute)
{
    uint16_t opcode = 0;
    int len = 0;
    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                               HCI_OCF_FM_SET_MUTE_MODE_REQ);
    len = sizeof(struct hci_fm_mute_mode_req);
    return send_fm_cmd_pkt(opcode, len, mute);
}

int helium_search_list(struct hci_fm_search_station_list_req *s_list)
{
   uint16_t opcode = 0;

   if (s_list == NULL) {
       ALOGE("%s:%s, search list param is null\n", LOG_TAG, __func__);
       return -EINVAL;
   }
   opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                HCI_OCF_FM_SEARCH_STATIONS_LIST);
   return send_fm_cmd_pkt(opcode, sizeof((*s_list)), s_list);
}

int helium_search_rds_stations(struct hci_fm_search_rds_station_req *rds_srch)
{
   uint16_t opcode = 0;

   if (rds_srch == NULL) {
       ALOGE("%s:%s, rds stations param is null\n", LOG_TAG, __func__);
       return -EINVAL;
   }
   opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                HCI_OCF_FM_SEARCH_RDS_STATIONS);
   return send_fm_cmd_pkt(opcode, sizeof((*rds_srch)), rds_srch);
}

int helium_search_stations(struct hci_fm_search_station_req *srch)
{
   uint16_t opcode = 0;

   if (srch == NULL) {
       ALOGE("%s:%s, search station param is null\n", LOG_TAG, __func__);
       return -EINVAL;
   }
   opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                HCI_OCF_FM_SEARCH_STATIONS);
   return send_fm_cmd_pkt(opcode, sizeof((*srch)), srch);
}

int helium_cancel_search_req()
{
   uint16_t opcode = 0;

   opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                HCI_OCF_FM_CANCEL_SEARCH);
   return send_fm_cmd_pkt(opcode, 0, NULL);
}

int hci_fm_set_recv_conf_req (struct hci_fm_recv_conf_req *conf)
{
    uint16_t opcode = 0;

    if (conf == NULL) {
        ALOGE("%s:%s, recv conf is null\n", LOG_TAG, __func__);
        return -EINVAL;
    }
    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                              HCI_OCF_FM_SET_RECV_CONF_REQ);
    return send_fm_cmd_pkt(opcode, sizeof((*conf)), conf);
}

int hci_fm_get_program_service_req ()
{
    uint16_t opcode = 0;

   opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                         HCI_OCF_FM_GET_PROGRAM_SERVICE_REQ);
    return send_fm_cmd_pkt(opcode, 0, NULL);
}

int hci_fm_get_rds_grpcounters_req (int val)
{
    uint16_t opcode = 0;

   opcode = hci_opcode_pack(HCI_OGF_FM_STATUS_PARAMETERS_CMD_REQ,
                         HCI_OCF_FM_READ_GRP_COUNTERS);
    return send_fm_cmd_pkt(opcode, sizeof(val), &val);
}

int hci_fm_get_rds_grpcounters_ext_req (int val)
{
    uint16_t opcode = 0;

   opcode = hci_opcode_pack(HCI_OGF_FM_STATUS_PARAMETERS_CMD_REQ,
                         HCI_OCF_FM_READ_GRP_COUNTERS_EXT);
    return send_fm_cmd_pkt(opcode, sizeof(val), &val);
}


int hci_fm_set_notch_filter_req (int val)
{
    uint16_t opcode = 0;

   opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                     HCI_OCF_FM_EN_NOTCH_CTRL);
    return send_fm_cmd_pkt(opcode, sizeof(val), &val);
}


int helium_set_sig_threshold_req(char th)
{
    uint16_t opcode = 0;

    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                               HCI_OCF_FM_SET_SIGNAL_THRESHOLD);
    return send_fm_cmd_pkt(opcode, sizeof(th), &th);
}

int helium_rds_grp_mask_req(struct hci_fm_rds_grp_req *rds_grp_msk)
{
    uint16_t opcode = 0;

    if (rds_grp_msk == NULL) {
        ALOGE("%s:%s, grp mask param is null\n", LOG_TAG, __func__);
        return -EINVAL;
    }
    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                                    HCI_OCF_FM_RDS_GRP);
    return send_fm_cmd_pkt(opcode, sizeof(*rds_grp_msk), rds_grp_msk);
}

int helium_rds_grp_process_req(int rds_grp)
{
    uint16_t opcode = 0;

    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                              HCI_OCF_FM_RDS_GRP_PROCESS);
    return send_fm_cmd_pkt(opcode, sizeof(rds_grp), &rds_grp);
}

int helium_set_event_mask_req(char e_mask)
{
    uint16_t opcode = 0;

    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                          HCI_OCF_FM_SET_EVENT_MASK);
    return send_fm_cmd_pkt(opcode, sizeof(e_mask), &e_mask);
}

int helium_set_antenna_req(char ant)
{
    uint16_t opcode = 0;

    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                                   HCI_OCF_FM_SET_ANTENNA);
    return send_fm_cmd_pkt(opcode, sizeof(ant), &ant);
}

int helium_set_fm_mute_mode_req(struct hci_fm_mute_mode_req *mute)
{
    uint16_t opcode = 0;

    if (mute == NULL) {
        ALOGE("%s:%s, mute mode is null\n", LOG_TAG, __func__);
        return -EINVAL;
    }
    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                               HCI_OCF_FM_SET_MUTE_MODE_REQ);
    return send_fm_cmd_pkt(opcode, sizeof((*mute)), mute);
}

int hci_fm_tune_station_req(int param)
{
    uint16_t opcode = 0;
    int tune_freq = param;

    ALOGV("%s:tune_freq: %d", LOG_TAG, tune_freq);
    opcode = hci_opcode_pack(HCI_OGF_FM_COMMON_CTRL_CMD_REQ,
                                  HCI_OCF_FM_TUNE_STATION_REQ);
    return send_fm_cmd_pkt(opcode, sizeof(tune_freq), &tune_freq);
}

int hci_set_fm_stereo_mode_req(struct hci_fm_stereo_mode_req *param)
{
    uint16_t opcode = 0;
    struct hci_fm_stereo_mode_req *stereo_mode_req =
                           (struct hci_fm_stereo_mode_req *) param;

    if (stereo_mode_req == NULL) {
        ALOGE("%s:%s, stere mode req is null\n", LOG_TAG, __func__);
        return -EINVAL;
    }
    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                             HCI_OCF_FM_SET_STEREO_MODE_REQ);
    return send_fm_cmd_pkt(opcode, sizeof((*stereo_mode_req)),
                                              stereo_mode_req);
}

int hci_peek_data(struct hci_fm_riva_data *data)
{
    uint16_t opcode = 0;

    if (data == NULL) {
        ALOGE("%s:%s, peek data req is null\n", LOG_TAG, __func__);
        return -EINVAL;
    }
    opcode = hci_opcode_pack(HCI_OGF_FM_DIAGNOSTIC_CMD_REQ,
                HCI_OCF_FM_PEEK_DATA);
    return send_fm_cmd_pkt(opcode, sizeof((*data)), data);
}

int hci_poke_data(struct hci_fm_riva_poke *data)
{
    uint16_t opcode = 0;

    if (data == NULL) {
        ALOGE("%s:%s, poke data req is null\n", LOG_TAG, __func__);
        return -EINVAL;
    }
    opcode = hci_opcode_pack(HCI_OGF_FM_DIAGNOSTIC_CMD_REQ,
                HCI_OCF_FM_POKE_DATA);
    return send_fm_cmd_pkt(opcode, sizeof((*data)), data);
}

int hci_ssbi_poke_reg(struct hci_fm_ssbi_req *data)
{
    uint16_t opcode = 0;

    if (data == NULL) {
        ALOGE("%s:%s,SSBI poke data req is null\n", LOG_TAG, __func__);
        return -EINVAL;
    }
    opcode = hci_opcode_pack(HCI_OGF_FM_DIAGNOSTIC_CMD_REQ,
                HCI_OCF_FM_SSBI_POKE_REG);
    return send_fm_cmd_pkt(opcode, sizeof((*data)), data);
}

int hci_ssbi_peek_reg(struct hci_fm_ssbi_peek *data)
{
    uint16_t opcode = 0;

    if (data == NULL) {
        ALOGE("%s:%s,SSBI peek data req is null\n", LOG_TAG, __func__);
        return -EINVAL;
    }
    opcode = hci_opcode_pack(HCI_OGF_FM_DIAGNOSTIC_CMD_REQ,
                HCI_OCF_FM_SSBI_PEEK_REG);
   return send_fm_cmd_pkt(opcode, sizeof((*data)), data);
}

int hci_get_set_reset_agc_req(struct hci_fm_set_get_reset_agc *data)
{
    uint16_t opcode = 0;
    if (data == NULL) {
        ALOGE("%s:%s,AGC set get reset req is null\n", LOG_TAG, __func__);
        return -EINVAL;
    }
    opcode = hci_opcode_pack(HCI_OGF_FM_DIAGNOSTIC_CMD_REQ,
    HCI_FM_SET_GET_RESET_AGC);
    return send_fm_cmd_pkt(opcode, sizeof((*data)), data);
}

int hci_fm_get_ch_det_th()
{
    ALOGV("%s", __func__);
    uint16_t opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
            HCI_OCF_FM_GET_CH_DET_THRESHOLD);
    return send_fm_cmd_pkt(opcode, 0, NULL);
}

int set_ch_det_thresholds_req(struct hci_fm_ch_det_threshold *ch_det_th)
{
    uint16_t opcode = 0;

    if (ch_det_th == NULL) {
        ALOGE("%s,%s channel det thrshld is null\n", LOG_TAG,  __func__);
        return -EINVAL;
    }
    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                            HCI_OCF_FM_SET_CH_DET_THRESHOLD);
    return send_fm_cmd_pkt(opcode, sizeof((*ch_det_th)), ch_det_th);
}

int hci_fm_default_data_read_req(struct hci_fm_def_data_rd_req *def_data_rd)
{
    uint16_t opcode = 0;

    if (def_data_rd == NULL) {
        ALOGE("Def data read param is null");
        return -EINVAL;
    }

    opcode = hci_opcode_pack(HCI_OGF_FM_COMMON_CTRL_CMD_REQ,
            HCI_OCF_FM_DEFAULT_DATA_READ);
    return send_fm_cmd_pkt(opcode, sizeof(struct hci_fm_def_data_rd_req),
            def_data_rd);
}

int hci_fm_get_blend_req()
{
    uint16_t opcode = 0;

    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
            HCI_OCF_FM_GET_BLND_TBL);
    return send_fm_cmd_pkt(opcode, 0, NULL);
}

int hci_fm_set_blend_tbl_req(struct hci_fm_blend_table *blnd_tbl)
{
    int opcode = 0;

    if (blnd_tbl == NULL) {
        ALOGE("Req param is null");
        return -EINVAL;
    }

    opcode =  hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
            HCI_OCF_FM_SET_BLND_TBL);
    return send_fm_cmd_pkt(opcode, sizeof(struct hci_fm_blend_table),
            blnd_tbl);
}

int hci_fm_default_data_write_req(struct hci_fm_def_data_wr_req * data_wrt)
{
    int opcode = 0;

    if (data_wrt == NULL) {
        ALOGE("req param is null");
        return -EINVAL;
    }

    opcode = hci_opcode_pack(HCI_OGF_FM_COMMON_CTRL_CMD_REQ,
            HCI_OCF_FM_DEFAULT_DATA_WRITE);
    return send_fm_cmd_pkt(opcode, data_wrt->length + sizeof(char) * 2,
            data_wrt);
}

int hci_fm_get_station_cmd_param_req()
{
    int opcode = 0;

    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
            HCI_OCF_FM_GET_STATION_PARAM_REQ);
    return send_fm_cmd_pkt(opcode, 0,  NULL);
}

int hci_fm_get_station_dbg_param_req()
{
    int opcode = 0;

    opcode = hci_opcode_pack(HCI_OGF_FM_DIAGNOSTIC_CMD_REQ,
            HCI_OCF_FM_STATION_DBG_PARAM);
    return send_fm_cmd_pkt(opcode, 0, NULL);
}

int hci_fm_enable_lpf(int enable)
{
    ALOGI("%s: enable: %x", __func__, enable);

    uint16_t opcode = 0;
    int enable_lpf = enable;

    opcode = hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ,
                                  HCI_OCF_FM_LOW_PASS_FILTER_CTRL);
    return send_fm_cmd_pkt(opcode, sizeof(enable_lpf), &enable_lpf);
}
int hci_fm_enable_slimbus(uint8_t val) {
    ALOGE("%s", __func__);
    uint16_t opcode = 0;

    opcode = hci_opcode_pack(HCI_OGF_FM_DIAGNOSTIC_CMD_REQ,
                                HCI_OCF_FM_ENABLE_SLIMBUS);

    ALOGE("%s:val = %d, uint8 val = %d", __func__, val, (uint8_t)val);
    return send_fm_cmd_pkt(opcode , sizeof(val), &val);
}
