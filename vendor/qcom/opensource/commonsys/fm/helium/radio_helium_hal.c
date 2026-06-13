/*
Copyright (c) 2015-2016 The Linux Foundation. All rights reserved.

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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>
#include "radio-helium-commands.h"
#include "radio-helium.h"
#include "fm_hci_api.h"
#include <dlfcn.h>
#include <errno.h>

int hci_fm_get_signal_threshold();
int hci_fm_enable_recv_req();
int hci_fm_mute_mode_req(struct hci_fm_mute_mode_req *);
static int oda_agt;
static int grp_mask;
static int rt_plus_carrier = -1;
static int ert_carrier = -1;
static unsigned char ert_buf[256];
static unsigned char ert_len;
static unsigned char c_byt_pair_index;
static char utf_8_flag;
static char rt_ert_flag;
static char formatting_dir;
static uint32_t ch_det_th_mask_flag;
static uint32_t def_data_rd_mask_flag;
static uint32_t blend_tbl_mask_flag;
static uint32_t station_param_mask_flag;
static uint32_t station_dbg_param_mask_flag;
uint64_t flag;
static int slimbus_flag = 0;
struct fm_hal_t *hal = NULL;
static pthread_mutex_t hal_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t hal_cond = PTHREAD_COND_INITIALIZER;
#undef LOG_TAG
#define LOG_TAG "radio_helium"
#define WAIT_TIMEOUT 20000 /* 20*1000us */
#define HAL_TIMEOUT  3

static void radio_hci_req_complete(char result)
{
  ALOGD("%s:enetred %s result %d", LOG_TAG, __func__, result);
}

static void radio_hci_status_complete(int result)
{
   ALOGD("%s:enetred %s result %d", LOG_TAG, __func__, result);
}

static void hci_cc_fm_enable_rsp(char *ev_rsp)
{
    struct hci_fm_conf_rsp  *rsp;

    if (ev_rsp == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG, __func__);
        return;
    }
    rsp = (struct hci_fm_conf_rsp *)ev_rsp;
    if (!slimbus_flag)
        hal->jni_cb->thread_evt_cb(0);
    radio_hci_req_complete(rsp->status);
    hal->jni_cb->enabled_cb();
    if (rsp->status == FM_HC_STATUS_SUCCESS)
        hal->radio->mode = FM_RECV;
}

static void hci_cc_conf_rsp(char *ev_rsp)
{
    struct hci_fm_conf_rsp  *rsp;

    if (ev_rsp == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG, __func__);
        return;
    }
    rsp = (struct hci_fm_conf_rsp *)ev_rsp;
    radio_hci_req_complete(rsp->status);
    if (!rsp->status) {
        hal->radio->recv_conf = rsp->recv_conf_rsp;
    }
}

static void hci_cc_fm_disable_rsp(char *ev_buff)
{
    char status;

    if (ev_buff == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG, __func__);
        return;
    }
    ALOGV("%s++", __func__);
    status = (char) *ev_buff;
    radio_hci_req_complete(status);
    if (hal->radio->mode == FM_TURNING_OFF) {
        ALOGD("%s:calling fm close\n", LOG_TAG );
        fm_hci_close(hal->private_data);
    }
}

static void hci_cc_rsp(char *ev_buff)
{
    char status;

    if (ev_buff == NULL) {
        ALOGE("%s:%s, socket buffer is null\n", LOG_TAG, __func__);
        return;
    }
    status = (char)*ev_buff;

    radio_hci_req_complete(status);
}

static void hci_cc_rds_grp_cntrs_rsp(char *ev_buff)
{
    char status;
    if (ev_buff == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG, __func__);
        return;
    }
    status = ev_buff[0];
    ALOGI("%s:%s, status =%d\n", LOG_TAG, __func__,status);
    if (status < 0) {
        ALOGE("%s:%s, read rds_grp_cntrs failed status=%d\n", LOG_TAG, __func__,status);
    }
    hal->jni_cb->rds_grp_cntrs_rsp_cb(&ev_buff[1]);
}

static void hci_cc_rds_grp_cntrs_ext_rsp(char *ev_buff)
{
    char status;
    if (ev_buff == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG, __func__);
        return;
    }
    status = ev_buff[0];
    ALOGI("%s:%s, status =%d\n", LOG_TAG, __func__,status);
    if (status < 0) {
        ALOGE("%s:%s, read rds_grp_cntrs_ext failed status=%d\n", LOG_TAG, __func__,status);
    }
    hal->jni_cb->rds_grp_cntrs_ext_rsp_cb(&ev_buff[1]);
}

static void hci_cc_riva_peek_rsp(char *ev_buff)
{
    char status;

    if (ev_buff == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG, __func__);
        return;
    }
    status = ev_buff[0];
    ALOGE("%s:%s, status =%d\n", LOG_TAG, __func__,status);
    if (status < 0) {
        ALOGE("%s:%s, peek failed=%d\n", LOG_TAG, __func__, status);
    }
    hal->jni_cb->fm_peek_rsp_cb(&ev_buff[PEEK_DATA_OFSET]);
    radio_hci_req_complete(status);
}

static void hci_cc_ssbi_peek_rsp(char *ev_buff)
{
    char status;

    if (ev_buff == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG, __func__);
        return;
    }
    status = ev_buff[0];
    ALOGE("%s:%s, status =%d\n", LOG_TAG, __func__,status);
    if (status < 0) {
        ALOGE("%s:%s,ssbi peek failed=%d\n", LOG_TAG, __func__, status);
    }
    hal->jni_cb->fm_ssbi_peek_rsp_cb(&ev_buff[PEEK_DATA_OFSET]);
    radio_hci_req_complete(status);
}

static void hci_cc_agc_rsp(char *ev_buff)
{
    char status;
    ALOGV("inside hci_cc_agc_rsp");
    if (ev_buff == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG, __func__);
        return;
    }
    status = ev_buff[0];
    ALOGV("%s:%s, status =%d\n", LOG_TAG, __func__,status);
    if (status != 0) {
        ALOGE("%s:%s,agc gain failed=%d\n", LOG_TAG, __func__, status);
    } else {
        hal->jni_cb->fm_agc_gain_rsp_cb(&ev_buff[1]);
    }
    radio_hci_req_complete(status);
}

static void hci_cc_get_ch_det_threshold_rsp(char *ev_buff)
{
    int status;
    int val = 0;
    if (ev_buff == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG, __func__);
        return;
    }
    status = ev_buff[0];
    ALOGV("%s:%s, status =%d\n", LOG_TAG, __func__,status);
    if (status != 0) {
        ALOGE("%s:%s,ssbi peek failed=%d\n", LOG_TAG, __func__, status);
    } else {
        memcpy(&hal->radio->ch_det_threshold, &ev_buff[1],
                        sizeof(struct hci_fm_ch_det_threshold));
        radio_hci_req_complete(status);

        if (test_bit(ch_det_th_mask_flag, CMD_CHDET_SINR_TH))
            val = hal->radio->ch_det_threshold.sinr;
        else if (test_bit(ch_det_th_mask_flag, CMD_CHDET_SINR_SAMPLE))
            val = hal->radio->ch_det_threshold.sinr_samples;
        else if (test_bit(ch_det_th_mask_flag, CMD_CHDET_INTF_TH_LOW))
            val = hal->radio->ch_det_threshold.low_th;
        else if (test_bit(ch_det_th_mask_flag, CMD_CHDET_INTF_TH_HIGH))
            val = hal->radio->ch_det_threshold.high_th;
    }
    clear_all_bit(ch_det_th_mask_flag);

    if (!hal->set_cmd_sent)
        hal->jni_cb->fm_get_ch_det_thr_cb(val, status);
    else {
        pthread_mutex_lock(&hal->cmd_lock);
        pthread_cond_broadcast(&hal->cmd_cond);
        pthread_mutex_unlock(&hal->cmd_lock);
    }
}

static void hci_cc_set_ch_det_threshold_rsp(char *ev_buff)
{
    int status = ev_buff[0];

    hal->jni_cb->fm_set_ch_det_thr_cb(status);
}

static void hci_cc_sig_threshold_rsp(char *ev_buff)
{
    int status, val = -1;
    ALOGD("hci_cc_sig_threshold_rsp");

    status = ev_buff[0];

    if (status != 0) {
        ALOGE("%s: status= 0x%x", __func__, status);
    } else {
        val = ev_buff[1];
    }
    hal->jni_cb->fm_get_sig_thres_cb(val, status);
}

static void hci_cc_default_data_read_rsp(uint8_t ev_buff[])
{
    int status, val= 0;
    uint8_t data_len = 0;

    if (ev_buff == NULL) {
        ALOGE("Response buffer is null");
        return;
    }
    status = ev_buff[0];
    if (status == 0) {
        data_len = ev_buff[1];
        ALOGV("hci_cc_default_data_read_rsp:data_len = %d", data_len);
        memcpy(&hal->radio->def_data, &ev_buff[1], data_len + sizeof(uint8_t));

        if (test_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_RMSSI_TH)) {
            val = hal->radio->def_data.data[AF_RMSSI_TH_OFFSET];
            clear_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_RMSSI_TH);
        } else if (test_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_RMSSI_SAMPLE)) {
            val = hal->radio->def_data.data[AF_RMSSI_SAMPLES_OFFSET];
            clear_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_RMSSI_SAMPLE);
        } else if (test_bit(def_data_rd_mask_flag, CMD_DEFRD_GD_CH_RMSSI_TH)) {
            val = hal->radio->def_data.data[GD_CH_RMSSI_TH_OFFSET];
            if (val > MAX_GD_CH_RMSSI_TH)
                val -= 256;
            clear_bit(def_data_rd_mask_flag, CMD_DEFRD_GD_CH_RMSSI_TH);
        } else if (test_bit(def_data_rd_mask_flag, CMD_DEFRD_SEARCH_ALGO)) {
            val = hal->radio->def_data.data[SRCH_ALGO_TYPE_OFFSET];
            clear_bit(def_data_rd_mask_flag, CMD_DEFRD_SEARCH_ALGO);
        } else if (test_bit(def_data_rd_mask_flag, CMD_DEFRD_SINR_FIRST_STAGE)) {
            val = hal->radio->def_data.data[SINRFIRSTSTAGE_OFFSET];
            if (val > MAX_SINR_FIRSTSTAGE)
                val -= 256;
            clear_bit(def_data_rd_mask_flag, CMD_DEFRD_SINR_FIRST_STAGE);
        } else if (test_bit(def_data_rd_mask_flag, CMD_DEFRD_RMSSI_FIRST_STAGE)) {
            val = hal->radio->def_data.data[RMSSIFIRSTSTAGE_OFFSET];
            clear_bit(def_data_rd_mask_flag, CMD_DEFRD_RMSSI_FIRST_STAGE);
        } else if (test_bit(def_data_rd_mask_flag, CMD_DEFRD_CF0TH12)) {
            val = (hal->radio->def_data.data[CF0TH12_BYTE1_OFFSET] |
                    (hal->radio->def_data.data[CF0TH12_BYTE2_OFFSET] << 8));
            clear_bit(def_data_rd_mask_flag, CMD_DEFRD_CF0TH12);
        } else if (test_bit(def_data_rd_mask_flag, CMD_DEFRD_TUNE_POWER)) {
            clear_bit(def_data_rd_mask_flag, CMD_DEFRD_TUNE_POWER);
        } else if (test_bit(def_data_rd_mask_flag, CMD_DEFRD_REPEATCOUNT)) {
            val = hal->radio->def_data.data[RX_REPEATE_BYTE_OFFSET];
            clear_bit(def_data_rd_mask_flag, CMD_DEFRD_REPEATCOUNT);
        } else if(test_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_ALGO)) {
            val = hal->radio->def_data.data[AF_ALGO_OFFSET];
            clear_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_ALGO);
        } else if (test_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_SINR_GD_CH_TH)) {
            val = hal->radio->def_data.data[AF_SINR_GD_CH_TH_OFFSET];
            clear_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_SINR_GD_CH_TH);
        } else if (test_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_SINR_TH)) {
            val = hal->radio->def_data.data[AF_SINR_TH_OFFSET];
            clear_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_SINR_TH);
        }
    } else {
        ALOGE("%s: Error: Status= 0x%x", __func__, status);
    }
    if (!hal->set_cmd_sent)
        hal->jni_cb->fm_def_data_read_cb(val, status);
    else {
        pthread_mutex_lock(&hal->cmd_lock);
        pthread_cond_broadcast(&hal->cmd_cond);
        pthread_mutex_unlock(&hal->cmd_lock);
    }


}

static void hci_cc_default_data_write_rsp(char *ev_buff)
{
    int status = ev_buff[0];

    hal->jni_cb->fm_def_data_write_cb(status);
}

static void hci_cc_get_blend_tbl_rsp(char *ev_buff)
{
    int status, val = -1;

    if (ev_buff == NULL) {
        ALOGE("%s:response buffer in null", LOG_TAG);
        return;
    }

    status = ev_buff[0];
    if (status != 0) {
        ALOGE("%s: status = 0x%x", LOG_TAG, status);
    } else {
        memcpy(&hal->radio->blend_tbl, &ev_buff[1],
                sizeof(struct hci_fm_blend_table));

        ALOGD("hci_cc_get_blend_tbl_rsp: data");
        int i;
        for (i = 0; i < 8; i++)
            ALOGE("data[%d] = 0x%x", i, ev_buff[1 + i]);
        if (test_bit(blend_tbl_mask_flag, CMD_BLENDTBL_SINR_HI)) {
            val = hal->radio->blend_tbl.BlendSinrHi;
            ALOGE("%s: Sinrhi val = %d", LOG_TAG, val);
            clear_bit(blend_tbl_mask_flag, CMD_BLENDTBL_SINR_HI);
        } else if (test_bit(blend_tbl_mask_flag, CMD_BLENDTBL_RMSSI_HI)) {
            val = hal->radio->blend_tbl.BlendRmssiHi;
            ALOGE("%s: BlendRmssiHi val = %d", LOG_TAG, val);
            clear_bit(blend_tbl_mask_flag, CMD_BLENDTBL_RMSSI_HI);
        }
    }

    if (!hal->set_cmd_sent)
        hal->jni_cb->fm_get_blend_cb(val, status);
    else {
        pthread_mutex_lock(&hal->cmd_lock);
        pthread_cond_broadcast(&hal->cmd_cond);
        pthread_mutex_unlock(&hal->cmd_lock);
    }
}

static void hci_cc_set_blend_tbl_rsp(char *ev_buff)
{
    int status = ev_buff[0];

    hal->jni_cb->fm_set_blend_cb(status);
}

static void hci_cc_station_rsp(char *ev_buff)
{
    int val = -1, status = ev_buff[0];
    unsigned char *tmp = (unsigned char *)(&hal->radio->fm_st_rsp.station_rsp)
                                             + sizeof(char);

    if (status == FM_HC_STATUS_SUCCESS) {
        memcpy(tmp, &ev_buff[1],
                sizeof(struct hci_ev_tune_status) - sizeof(char));
        if (test_bit(station_param_mask_flag, CMD_STNPARAM_RSSI)) {
                val = hal->radio->fm_st_rsp.station_rsp.rssi;
        } else if (test_bit(station_param_mask_flag, CMD_STNPARAM_SINR)) {
            val = hal->radio->fm_st_rsp.station_rsp.sinr;
        }
    }
    ALOGE("hci_cc_station_rsp: val =%x, status = %x", val, status);

    clear_all_bit(station_param_mask_flag);
    hal->jni_cb->fm_get_station_param_cb(val, status);
}

static void hci_cc_dbg_param_rsp(char *ev_buff)
{
    int val = -1, status = ev_buff[0];

    if (status == FM_HC_STATUS_SUCCESS) {
        memcpy(&hal->radio->st_dbg_param, &ev_buff[1],
                sizeof(struct hci_fm_dbg_param_rsp));
        if (test_bit(station_dbg_param_mask_flag, CMD_STNDBGPARAM_INFDETOUT)) {
            val = hal->radio->st_dbg_param.in_det_out;
        } else if (test_bit(station_dbg_param_mask_flag, CMD_STNDBGPARAM_IOVERC)) {
            val = hal->radio->st_dbg_param.io_verc;
        }
    }
    ALOGE("hci_cc_dbg_param_rsp: val =%x, status = %x", val, status);
    hal->jni_cb->fm_get_station_debug_param_cb(val, status);
    clear_all_bit(station_dbg_param_mask_flag);
}

static void hci_cc_enable_slimbus_rsp(char *ev_buff)
{
    ALOGD("%s status %d", __func__, ev_buff[0]);
    slimbus_flag = 1;
    hal->jni_cb->thread_evt_cb(0);
    hal->jni_cb->enable_slimbus_cb(ev_buff[0]);
}

static void hci_cc_enable_softmute_rsp(char *ev_buff)
{
    ALOGD("%s status %d", __func__, ev_buff[0]);
    hal->jni_cb->enable_softmute_cb(ev_buff[0]);
}

static inline void hci_cmd_complete_event(uint8_t buff[])
{
    uint16_t opcode;
    char *pbuf;

    if (buff == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG, __func__);
        return;
    }
    ALOGV("%s:buff[1] = 0x%x buff[2] = 0x%x", LOG_TAG, buff[1], buff[2]);
    opcode = ((buff[2] << 8) | buff[1]);
    ALOGV("%s: Received HCI CMD COMPLETE EVENT for the opcode: 0x%x", __func__, opcode);
    pbuf = (char *)&buff[3];

    switch (opcode) {
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_ENABLE_RECV_REQ):
            ALOGE("%s: Recvd. CC event for FM_ENABLE_RECV_REQ", __func__);
            hci_cc_fm_enable_rsp(pbuf);
            break;
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_GET_RECV_CONF_REQ):
            hci_cc_conf_rsp(pbuf);
            break;
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_DISABLE_RECV_REQ):
            hci_cc_fm_disable_rsp(pbuf);
            break;
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_SET_MUTE_MODE_REQ):
            hci_cc_enable_softmute_rsp(pbuf);
            break; 
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_SET_RECV_CONF_REQ):
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_SET_STEREO_MODE_REQ):
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_SET_ANTENNA):
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_SET_SIGNAL_THRESHOLD):
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_CANCEL_SEARCH):
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_RDS_GRP):
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_RDS_GRP_PROCESS):
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_EN_WAN_AVD_CTRL):
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_EN_NOTCH_CTRL):
            hci_cc_rsp(pbuf);
            break;
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_SET_CH_DET_THRESHOLD):
            hci_cc_set_ch_det_threshold_rsp(pbuf);
            break;
    case hci_common_cmd_op_pack(HCI_OCF_FM_RESET):
    case hci_diagnostic_cmd_op_pack(HCI_OCF_FM_SSBI_POKE_REG):
    case hci_diagnostic_cmd_op_pack(HCI_OCF_FM_POKE_DATA):
    case hci_diagnostic_cmd_op_pack(HCI_FM_SET_INTERNAL_TONE_GENRATOR):
    case hci_common_cmd_op_pack(HCI_OCF_FM_SET_CALIBRATION):
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_SET_EVENT_MASK):
    case hci_common_cmd_op_pack(HCI_OCF_FM_SET_SPUR_TABLE):
            hci_cc_rsp(pbuf);
            break;
    case hci_status_param_op_pack(HCI_OCF_FM_READ_GRP_COUNTERS):
            hci_cc_rds_grp_cntrs_rsp(pbuf);
            break;
    case hci_status_param_op_pack(HCI_OCF_FM_READ_GRP_COUNTERS_EXT):
            hci_cc_rds_grp_cntrs_ext_rsp(pbuf);
            break;
    case hci_diagnostic_cmd_op_pack(HCI_OCF_FM_PEEK_DATA):
            hci_cc_riva_peek_rsp((char *)buff);
            break;
    case hci_diagnostic_cmd_op_pack(HCI_OCF_FM_SSBI_PEEK_REG):
            hci_cc_ssbi_peek_rsp((char *)buff);
            break;
    case hci_diagnostic_cmd_op_pack(HCI_FM_SET_GET_RESET_AGC):
            hci_cc_agc_rsp(pbuf);
            break;
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_GET_CH_DET_THRESHOLD):
            hci_cc_get_ch_det_threshold_rsp(pbuf);
            break;
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_GET_SIGNAL_THRESHOLD):
            hci_cc_sig_threshold_rsp(pbuf);
            break;
    case hci_common_cmd_op_pack(HCI_OCF_FM_DEFAULT_DATA_READ):
            hci_cc_default_data_read_rsp(&buff[3]);
            break;
    case hci_common_cmd_op_pack(HCI_OCF_FM_DEFAULT_DATA_WRITE):
            hci_cc_default_data_write_rsp(pbuf);
            break;
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_GET_BLND_TBL):
            hci_cc_get_blend_tbl_rsp(pbuf);
            break;
    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_SET_BLND_TBL):
            hci_cc_set_blend_tbl_rsp(pbuf);
            break;

    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_GET_STATION_PARAM_REQ):
            hci_cc_station_rsp(pbuf);
            break;

    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_LOW_PASS_FILTER_CTRL):
            ALOGI("%s: recived LPF enable event", __func__);
            hci_cc_rsp(pbuf);
            break;

    case hci_diagnostic_cmd_op_pack(HCI_OCF_FM_STATION_DBG_PARAM):
            hci_cc_dbg_param_rsp(pbuf);
            break;

    case hci_diagnostic_cmd_op_pack(HCI_OCF_FM_ENABLE_SLIMBUS):
            hci_cc_enable_slimbus_rsp(pbuf);
            break;

/*    case hci_common_cmd_op_pack(HCI_OCF_FM_GET_SPUR_TABLE):
            hci_cc_get_spur_tbl(buff);
            break;

    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_GET_PROGRAM_SERVICE_REQ):
            hci_cc_prg_srv_rsp(buff);
            break;

    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_GET_RADIO_TEXT_REQ):
            hci_cc_rd_txt_rsp(buff);
            break;

    case hci_recv_ctrl_cmd_op_pack(HCI_OCF_FM_GET_AF_LIST_REQ):
            hci_cc_af_list_rsp(buff);
            break;

    case hci_common_cmd_op_pack(HCI_OCF_FM_GET_FEATURE_LIST):
            hci_cc_feature_list_rsp(buff);
            break;

    case hci_status_param_op_pack(HCI_OCF_FM_READ_GRP_COUNTERS):
            hci_cc_rds_grp_cntrs_rsp(buff);
            break;
    case hci_common_cmd_op_pack(HCI_OCF_FM_DO_CALIBRATION):
            hci_cc_do_calibration_rsp(buff);
            break;

    default:
            ALOGE("opcode 0x%x", opcode);
            break; */
    }
}

static inline void hci_cmd_status_event(uint8_t st_rsp[])
{
    struct hci_ev_cmd_status *ev = (void *) st_rsp;
    uint16_t opcode;

    if (st_rsp == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG, __func__);
        return;
    }
    ALOGE("%s:st_rsp[2] = 0x%x st_rsp[3] = 0x%x", LOG_TAG, st_rsp[2], st_rsp[3]);
    opcode = ((st_rsp[3] << 8) | st_rsp[2]);
    ALOGE("%s: Received HCI CMD STATUS EVENT for opcode: 0x%x", __func__, opcode);

    radio_hci_status_complete(ev->status);
}

static inline void hci_ev_tune_status(uint8_t buff[])
{

    memcpy(&hal->radio->fm_st_rsp.station_rsp, &buff[0],
                               sizeof(struct hci_ev_tune_status));
    ALOGD("freq = %d", hal->radio->fm_st_rsp.station_rsp.station_freq);
    hal->jni_cb->tune_cb(hal->radio->fm_st_rsp.station_rsp.station_freq);

    //    if (hal->radio->fm_st_rsp.station_rsp.serv_avble)
          // todo callback for threshould

    if (hal->radio->fm_st_rsp.station_rsp.stereo_prg)
        hal->jni_cb->stereo_status_cb(true);
    else if (hal->radio->fm_st_rsp.station_rsp.stereo_prg == 0)
        hal->jni_cb->stereo_status_cb(false);

    if (hal->radio->fm_st_rsp.station_rsp.rds_sync_status)
        hal->jni_cb->rds_avail_status_cb(true);
    else
        hal->jni_cb->rds_avail_status_cb(false);
}

static inline void hci_ev_search_next()
{
    hal->jni_cb->scan_next_cb();
}

static inline void hci_ev_stereo_status(uint8_t buff[])
{
    char st_status;

    if (buff == NULL) {
        ALOGE("%s:%s, socket buffer is null\n", LOG_TAG,__func__);
        return;
    }
    st_status =  buff[0];
    if (st_status)
        hal->jni_cb->stereo_status_cb(true);
    else
        hal->jni_cb->stereo_status_cb(false);
}

static void hci_ev_rds_lock_status(uint8_t buff[])
{
    char rds_status;

    if (buff == NULL) {
        ALOGE("%s:%s, socket buffer is null\n", LOG_TAG, __func__);
        return;
    }

    rds_status = buff[0];

    if (rds_status)
        hal->jni_cb->rds_avail_status_cb(true);
    else
        hal->jni_cb->rds_avail_status_cb(false);
}

static inline void hci_ev_program_service(uint8_t buff[])
{
    int len;
    char *data;

    if (buff == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG,__func__);
        return;
    }
    if (buff[RDS_PS_LENGTH_OFFSET] < RDS_PS_NUMBER_MIN ||
        buff[RDS_PS_LENGTH_OFFSET] > RDS_PS_NUMBER_MAX) {
        ALOGE("%s:Invalid PS strings number", LOG_TAG);
        return;
    }
    len = (buff[RDS_PS_LENGTH_OFFSET] * RDS_STRING) + RDS_OFFSET;
    data = malloc(len);
    if (!data) {
        ALOGE("%s:Failed to allocate memory", LOG_TAG);
        return;
    }

    data[0] = buff[RDS_PS_LENGTH_OFFSET];
    data[1] = buff[RDS_PTYPE];
    data[2] = buff[RDS_PID_LOWER];
    data[3] = buff[RDS_PID_HIGHER];
    data[4] = 0;

    memcpy(data+RDS_OFFSET, &buff[RDS_PS_DATA_OFFSET], len-RDS_OFFSET);

    ALOGV("call ps-callback");
    hal->jni_cb->ps_update_cb(data);

    free(data);
}

static inline void hci_ev_radio_text(uint8_t buff[])
{
    int len = 0;
    char *data;

    if (buff == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG,__func__);
        return;
    }

    while ((buff[len+RDS_OFFSET] != 0x0d) && (len < MAX_RT_LENGTH))
           len++;
    if (len == 0)
        return;

    ALOGV("%s:%s: radio text length=%d\n", LOG_TAG, __func__,len);
    data = malloc(len+RDS_OFFSET+1);
    if (!data) {
        ALOGE("%s:Failed to allocate memory", LOG_TAG);
        return;
    }

    data[0] = len;
    data[1] = buff[RDS_PTYPE];
    data[2] = buff[RDS_PID_LOWER];
    data[3] = buff[RDS_PID_HIGHER];
    data[4] = buff[RT_A_B_FLAG_OFFSET];

    memcpy(data+RDS_OFFSET, &buff[RDS_OFFSET], len);

    data[len+RDS_OFFSET] = 0x00;
    hal->jni_cb->rt_update_cb(data);
    free(data);
}

static void hci_ev_af_list(uint8_t buff[])
{
    struct hci_ev_af_list ev;

    if (buff == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG, __func__);
        return;
    }
    ev.tune_freq = *((int *) &buff[0]);
    ev.pi_code = *((__le16 *) &buff[PI_CODE_OFFSET]);
    ev.af_size = buff[AF_SIZE_OFFSET];
    if (ev.af_size > AF_LIST_MAX) {
        ALOGE("%s:AF list size received more than available size", LOG_TAG);
        return;
    }
    memcpy(&ev.af_list[0], &buff[AF_LIST_OFFSET],
                                        ev.af_size * sizeof(int));
    hal->jni_cb->af_list_update_cb((uint16_t *)&ev);
}

static inline void hci_ev_search_compl(uint8_t buff[])
{
    if (buff == NULL) {
        ALOGE("%s:%s,buffer is null\n", LOG_TAG, __func__);
        return;
    }
    hal->radio->search_on = 0;
    hal->jni_cb->seek_cmpl_cb(hal->radio->fm_st_rsp.station_rsp.station_freq);
}

static inline void hci_ev_srch_st_list_compl(uint8_t buff[])
{
    struct hci_ev_srch_list_compl *ev ;
    int cnt;
    int stn_num;
    int rel_freq;
    int abs_freq;
    int len;

    if (buff == NULL) {
        ALOGE("%s:%s, buffer is null\n", LOG_TAG,__func__);
        return;
    }
    ev = malloc(sizeof(*ev));
    if (!ev) {
        ALOGE("%s:Memory allocation failed", LOG_TAG);
        return ;
    }

    ev->num_stations_found = buff[STN_NUM_OFFSET];
    len = ev->num_stations_found * PARAMS_PER_STATION + STN_FREQ_OFFSET;

    for(cnt = STN_FREQ_OFFSET, stn_num = 0;
           (cnt < len) && (stn_num < ev->num_stations_found)
                      && (stn_num < SIZE_ARRAY(ev->rel_freq));
            cnt += PARAMS_PER_STATION, stn_num++) {

        abs_freq = *((int *)&buff[cnt]);
        rel_freq = abs_freq - hal->radio->recv_conf.band_low_limit;
        rel_freq = (rel_freq * 20) / KHZ_TO_MHZ;

        ev->rel_freq[stn_num].rel_freq_lsb = GET_LSB(rel_freq);
        ev->rel_freq[stn_num].rel_freq_msb = GET_MSB(rel_freq);
    }

    len = ev->num_stations_found * 2 + sizeof(ev->num_stations_found);
    hal->jni_cb->srch_list_cb((uint16_t *)ev);
    free(ev);
}

static inline void hci_ev_rt_plus_id(uint8_t buff[])
{
    char *data = NULL;
    int len = 15;

    ALOGD("%s:%s: start", LOG_TAG, __func__);
    data = malloc(len);
    if (data != NULL) {
       data[0] = len;
       data[1] = buff[RDS_PTYPE];
       data[2] = buff[RDS_PID_LOWER];
       data[3] = buff[RDS_PID_HIGHER];
       data[4] = buff[3];

      memcpy(&data[RDS_OFFSET], &buff[4], len-RDS_OFFSET);
      ALOGD("%s:%s: RT+ ID grouptype=0x%x\n", LOG_TAG, __func__,data[4]);
      free(data);
    } else {
        ALOGE("%s:memory allocation failed\n", LOG_TAG);
    }
}

static void hci_ev_rt_plus_tag(uint8_t buff[])
{
    char *data = NULL;
    int len = 15;

    ALOGD("%s:%s: start", LOG_TAG, __func__);
    data = malloc(len);
    if (data != NULL) {
        data[0] = len;
        ALOGI("%s:%s: data length=%d\n", LOG_TAG, __func__,data[0]);
        data[1] = buff[RDS_PTYPE];
        data[2] = buff[RDS_PID_LOWER];
        data[3] = buff[RDS_PID_HIGHER];
        data[4] = buff[3];
        memcpy(&data[RDS_OFFSET], &buff[4], len-RDS_OFFSET);
        // data[len] = 0x00;
        hal->jni_cb->rt_plus_update_cb(data);
        free(data);
     } else {
        ALOGE("%s:memory allocation failed\n", LOG_TAG);
     }
}

static void  hci_ev_ext_country_code(uint8_t buff[])
{
    char *data = NULL;
    int len = ECC_EVENT_BUFSIZE;
    ALOGD("%s:%s: start", LOG_TAG, __func__);
    data = malloc(len);
    if (data != NULL) {
        data[0] = len;
        ALOGI("%s:%s: data length=%d\n", LOG_TAG, __func__,data[0]);
        data[1] = buff[RDS_PTYPE];
        data[2] = buff[RDS_PID_LOWER];
        data[3] = buff[RDS_PID_HIGHER];
        data[4] = buff[3];
        memcpy(&data[RDS_OFFSET], &buff[4], len-RDS_OFFSET);
        hal->jni_cb->ext_country_code_cb(data);
        free(data);
    } else {
        ALOGE("%s:memory allocation failed\n", LOG_TAG);
    }
}

static void hci_ev_driver_rds_event(uint8_t buff[])
{
    uint8_t rds_type;
    char *rds_data = NULL;
    rds_type = buff[0];

    ALOGD("%s:%s:rds type = 0x%x", LOG_TAG, __func__, rds_type);
    rds_data = malloc(STD_BUF_SIZE);
    if (rds_data == NULL) {
        ALOGE("%s:memory allocation failed\n", LOG_TAG);
        return;
    } else {
        memcpy(rds_data, &buff[1],STD_BUF_SIZE);
    }

    switch (rds_type) {
        case HCI_EV_RADIO_TEXT:
            hal->jni_cb->rt_update_cb(rds_data);
            break;

        case HCI_EV_PROGRAM_SERVICE:
            hal->jni_cb->ps_update_cb(rds_data);
            break;

        case HCI_EV_FM_AF_LIST:
            hal->jni_cb->af_list_update_cb((uint16_t *)&rds_data);
            break;

        case HCI_EV_RADIO_TEXT_PLUS_TAG:
            hal->jni_cb->rt_plus_update_cb(rds_data);
            break;

        case HCI_EV_E_RADIO_TEXT:
            hal->jni_cb->ert_update_cb(rds_data);
            break;

        default:
            ALOGD("%s: Unknown RDS event", __func__);
            break;
        }

    free(rds_data);
}

static void hci_ev_ert()
{
    char *data = NULL;

    if (ert_len <= 0)
        return;
    data = malloc(ert_len + 3);
    if (data != NULL) {
        data[0] = ert_len;
        data[1] = utf_8_flag;
        data[2] = formatting_dir;
        memcpy((data + 3), ert_buf, ert_len);
        hal->jni_cb->ert_update_cb(data);
        free(data);
    }
}

static void hci_ev_hw_error()
{
   ALOGE("%s:%s: start", LOG_TAG, __func__);
   fm_hci_close(hal->private_data);
}

static void hci_buff_ert(struct rds_grp_data *rds_buf)
{
    int i;
    unsigned short int info_byte = 0;
    unsigned short int byte_pair_index;

    if (rds_buf == NULL) {
        ALOGE("%s:%s, rds buffer is null\n", LOG_TAG, __func__);
        return;
    }
    byte_pair_index = AGT(rds_buf->rdsBlk[1].rdsLsb);
    if (byte_pair_index == 0) {
        c_byt_pair_index = 0;
        ert_len = 0;
    }
    if (c_byt_pair_index == byte_pair_index) {
        c_byt_pair_index++;
        for (i = 2; i <= 3; i++) {
             info_byte = rds_buf->rdsBlk[i].rdsLsb;
             info_byte |= (rds_buf->rdsBlk[i].rdsMsb << 8);
             ert_buf[ert_len++] = rds_buf->rdsBlk[i].rdsMsb;
             ert_buf[ert_len++] = rds_buf->rdsBlk[i].rdsLsb;
             if ((utf_8_flag == 0) && (info_byte == CARRIAGE_RETURN)) {
                 ert_len -= 2;
                 break;
             } else if ((utf_8_flag == 1) &&
                        (rds_buf->rdsBlk[i].rdsMsb == CARRIAGE_RETURN)) {
                 info_byte = CARRIAGE_RETURN;
                 ert_len -= 2;
                 break;
             } else if ((utf_8_flag == 1) &&
                        (rds_buf->rdsBlk[i].rdsLsb == CARRIAGE_RETURN)) {
                 info_byte = CARRIAGE_RETURN;
                 ert_len--;
                 break;
             }
        }
        if ((byte_pair_index == MAX_ERT_SEGMENT) ||
            (info_byte == CARRIAGE_RETURN)) {
            hci_ev_ert();
            c_byt_pair_index = 0;
            ert_len = 0;
        }
    } else {
        ert_len = 0;
        c_byt_pair_index = 0;
    }
}

static void hci_ev_raw_rds_group_data(uint8_t buff[])
{
    unsigned char blocknum, index;
    struct rds_grp_data temp;
    unsigned int mask_bit;
    unsigned short int aid, agt, gtc;
    unsigned short int carrier;

    index = RDSGRP_DATA_OFFSET;

    if (buff == NULL) {
        ALOGE("%s:%s, socket buffer is null\n", LOG_TAG, __func__);
        return;
    }

    for (blocknum = 0; blocknum < RDS_BLOCKS_NUM; blocknum++) {
         temp.rdsBlk[blocknum].rdsLsb = buff[index];
         temp.rdsBlk[blocknum].rdsMsb = buff[index+1];
         index = index + 2;
    }

    aid = AID(temp.rdsBlk[3].rdsLsb, temp.rdsBlk[3].rdsMsb);
    gtc = GTC(temp.rdsBlk[1].rdsMsb);
    agt = AGT(temp.rdsBlk[1].rdsLsb);

    if (gtc == GRP_3A) {
        switch (aid) {
        case ERT_AID:
            /* calculate the grp mask for RDS grp
             * which will contain actual eRT text
             *
             * Bit Pos  0  1  2  3  4   5  6   7
             * Grp Type 0A 0B 1A 1B 2A  2B 3A  3B
             *
             * similary for rest grps
             */
             mask_bit = (((agt >> 1) << 1) + (agt & 1));
             oda_agt = (1 << mask_bit);
             utf_8_flag = (temp.rdsBlk[2].rdsLsb & 1);
             formatting_dir = EXTRACT_BIT(temp.rdsBlk[2].rdsLsb,
                                               ERT_FORMAT_DIR_BIT);
             if (ert_carrier != agt)
                 hal->jni_cb->oda_update_cb();
             ert_carrier = agt;
             break;
        case RT_PLUS_AID:
            /* calculate the grp mask for RDS grp
             * which will contain actual eRT text
             *
             * Bit Pos  0  1  2  3  4   5  6   7
             * Grp Type 0A 0B 1A 1B 2A  2B 3A  3B
             *
             * similary for rest grps
             */
             mask_bit = (((agt >> 1) << 1) + (agt & 1));
             oda_agt =  (1 << mask_bit);
             /*Extract 5th bit of MSB (b7b6b5b4b3b2b1b0)*/
             rt_ert_flag = EXTRACT_BIT(temp.rdsBlk[2].rdsMsb,
                                              RT_ERT_FLAG_BIT);
             if (rt_plus_carrier != agt)
                 hal->jni_cb->oda_update_cb();
             rt_plus_carrier = agt;
             break;
        default:
             oda_agt = 0;
             break;
        }
    } else {
        carrier = gtc;
        if (carrier == rt_plus_carrier) {
         //    hci_ev_rt_plus(temp);
        }
        else if (carrier == ert_carrier) {
             ALOGI("%s:: calling event ert", __func__);
             hci_buff_ert(&temp);
       }
    }
}

static void radio_hci_event_packet(char *evt_buf)
{
    char evt;

    ALOGE("%s:%s: Received %d bytes of HCI EVENT PKT from Controller", LOG_TAG,
                                      __func__, ((struct fm_event_header_t *)evt_buf)->evt_len);
    evt = ((struct fm_event_header_t *)evt_buf)->evt_code;
    ALOGE("%s:evt: %d", LOG_TAG, evt);

    switch(evt) {
    case HCI_EV_TUNE_STATUS:
        hci_ev_tune_status(((struct fm_event_header_t *)evt_buf)->params);
        break;
    case HCI_EV_SEARCH_PROGRESS:
    case HCI_EV_SEARCH_RDS_PROGRESS:
    case HCI_EV_SEARCH_LIST_PROGRESS:
        hci_ev_search_next();
        break;
    case HCI_EV_STEREO_STATUS:
        hci_ev_stereo_status(((struct fm_event_header_t *)evt_buf)->params);
        break;
    case HCI_EV_RDS_LOCK_STATUS:
        hci_ev_rds_lock_status(((struct fm_event_header_t *)evt_buf)->params);
        break;
/*    case HCI_EV_SERVICE_AVAILABLE:
        hci_ev_service_available(hdev, skb);
        break; */
    case HCI_EV_RDS_RX_DATA:
        hci_ev_raw_rds_group_data(((struct fm_event_header_t *)evt_buf)->params);
        break;
    case HCI_EV_PROGRAM_SERVICE:
        hci_ev_program_service(((struct fm_event_header_t *)evt_buf)->params);
        break;
    case HCI_EV_RADIO_TEXT:
        hci_ev_radio_text(((struct fm_event_header_t *)evt_buf)->params);
        break;
    case HCI_EV_FM_AF_LIST:
        hci_ev_af_list(((struct fm_event_header_t *)evt_buf)->params);
        break;
    case HCI_EV_CMD_COMPLETE:
        ALOGE("%s:%s: Received HCI_EV_CMD_COMPLETE", LOG_TAG, __func__);
        hci_cmd_complete_event(((struct fm_event_header_t *)evt_buf)->params);
        break;
    case HCI_EV_CMD_STATUS:
        hci_cmd_status_event(((struct fm_event_header_t *)evt_buf)->params);
        break;
    case HCI_EV_SEARCH_COMPLETE:
    case HCI_EV_SEARCH_RDS_COMPLETE:
        hci_ev_search_compl(((struct fm_event_header_t *)evt_buf)->params);
        break;
    case HCI_EV_SEARCH_LIST_COMPLETE:
        hci_ev_srch_st_list_compl(((struct fm_event_header_t *)evt_buf)->params);
        break;
    case HCI_EV_RADIO_TEXT_PLUS_ID:
        hci_ev_rt_plus_id(((struct fm_event_header_t *)evt_buf)->params);
        break;
    case HCI_EV_RADIO_TEXT_PLUS_TAG:
        hci_ev_rt_plus_tag(((struct fm_event_header_t *)evt_buf)->params);
        break;
    case HCI_EV_EXT_COUNTRY_CODE:
        hci_ev_ext_country_code(((struct fm_event_header_t *)evt_buf)->params);
        break;
    case HCI_EV_DRIVER_RDS_EVENT:
        hci_ev_driver_rds_event(((struct fm_event_header_t *)evt_buf)->params);
        break;
    case HCI_EV_HW_ERR_EVENT:
        hci_ev_hw_error();
        break;
    default:
        break;
    }
}

/* 'evt_buf' contains the event received from Controller */
int process_event(unsigned char *evt_buf)
{
    ALOGI("%s: %s: Received event notification from FM-HCI thread. EVT CODE: %d ",
                            LOG_TAG,  __func__, ((struct fm_event_header_t *)evt_buf)->evt_code);
    radio_hci_event_packet((char*)evt_buf);
    return 0;
}

int fm_hci_close_done(void)
{
    ALOGI("fm_hci_close_done");
    const fm_hal_callbacks_t *ptr = NULL;

    pthread_mutex_lock(&hal_lock);
    if(hal != NULL){
        ptr = hal->jni_cb;
        ALOGI("clearing hal ");
        free(hal->radio);
        hal->radio = NULL;
        hal->jni_cb = NULL;
        pthread_cond_destroy(&hal->cmd_cond);
        pthread_mutex_destroy(&hal->cmd_lock);
        free(hal);
        hal = NULL;

        ALOGI("Notifying FM OFF to JNI");
        ptr->disabled_cb();
        ptr->thread_evt_cb(1);
    }
    pthread_cond_broadcast(&hal_cond);
    pthread_mutex_unlock(&hal_lock);
    return 0;
}

int helium_search_req(int on, int direct)
{
    int retval = 0;
    enum search_t srch;
    int saved_val;
    int dir;

    srch = hal->radio->g_search_mode & SRCH_MODE;
    saved_val = hal->radio->search_on;
    hal->radio->search_on = on;
    if (direct)
        dir = SRCH_DIR_UP;
    else
        dir = SRCH_DIR_DOWN;

    if (on) {
        switch (srch) {
        case SCAN_FOR_STRONG:
        case SCAN_FOR_WEAK:
            hal->radio->srch_st_list.srch_list_dir = dir;
            hal->radio->srch_st_list.srch_list_mode = srch;
            retval = helium_search_list(&hal->radio->srch_st_list);
            break;
        case RDS_SEEK_PTY:
        case RDS_SCAN_PTY:
        case RDS_SEEK_PI:
            srch = srch - SEARCH_RDS_STNS_MODE_OFFSET;
            hal->radio->srch_rds.srch_station.srch_mode = srch;
            hal->radio->srch_rds.srch_station.srch_dir = dir;
            hal->radio->srch_rds.srch_station.scan_time = hal->radio->g_scan_time;
            retval = helium_search_rds_stations(&hal->radio->srch_rds);
            break;
        default:
            hal->radio->srch_st.srch_mode = srch;
            hal->radio->srch_st.scan_time = hal->radio->g_scan_time;
            hal->radio->srch_st.srch_dir = dir;
            retval = helium_search_stations(&hal->radio->srch_st);
            break;
        }
    } else {
        retval = helium_cancel_search_req();
    }

    if (retval < 0)
        hal->radio->search_on = saved_val;
    return retval;
}

int helium_recv_set_region(int req_region)
{
    int retval;
    int saved_val;

    saved_val = hal->radio->region;
    hal->radio->region = req_region;

    retval = hci_fm_set_recv_conf_req(&hal->radio->recv_conf);
    if (retval < 0)
        hal->radio->region = saved_val;
    return retval;
}

int set_low_power_mode(int lp_mode)
{
    int rds_grps_proc = 0x00;
    int retval = 0;

    if (hal->radio->power_mode != lp_mode) {
       if (lp_mode) {
           hal->radio->event_mask = 0x00;
           if (hal->radio->af_jump_bit)
               rds_grps_proc = 0x00 | AF_JUMP_ENABLE;
           else
               rds_grps_proc = 0x00;
           retval = helium_rds_grp_process_req(rds_grps_proc);
           if (retval < 0) {
               ALOGE("%s:Disable RDS failed", LOG_TAG);
               return retval;
           }
           retval = helium_set_event_mask_req(hal->radio->event_mask);
       } else {
           hal->radio->event_mask = SIG_LEVEL_INTR | RDS_SYNC_INTR | AUDIO_CTRL_INTR;
           retval = helium_set_event_mask_req(hal->radio->event_mask);
           if (retval < 0) {
               ALOGE("%s:Enable Async events failed", LOG_TAG);
               return retval;
           }
           hal->radio->g_rds_grp_proc_ps = 0x000000FF;
           retval = helium_rds_grp_process_req(hal->radio->g_rds_grp_proc_ps);
       }
       hal->radio->power_mode = lp_mode;
    }
    return retval;
}

static int helium_send_hci_cmd(int cmd, void *cmd_param)
{
    int ret = FM_HC_STATUS_FAIL;
    struct timespec ts;
    struct hci_fm_def_data_rd_req *rd;

    pthread_mutex_lock(&(hal->cmd_lock));
    hal->set_cmd_sent = true;
    switch (cmd) {
        case HCI_FM_HELIUM_SINR_SAMPLES:
        case HCI_FM_HELIUM_SINR_THRESHOLD:
        case HCI_FM_HELIUM_INTF_LOW_THRESHOLD:
        case HCI_FM_HELIUM_INTF_HIGH_THRESHOLD:
            ret = hci_fm_get_ch_det_th();
            break;
        case HCI_FM_HELIUM_SINRFIRSTSTAGE:
        case HCI_FM_HELIUM_RMSSIFIRSTSTAGE:
        case HCI_FM_HELIUM_CF0TH12:
        case HCI_FM_HELIUM_SRCHALGOTYPE:
        case HCI_FM_HELIUM_AF_RMSSI_TH:
        case HCI_FM_HELIUM_GOOD_CH_RMSSI_TH:
        case HCI_FM_HELIUM_AF_RMSSI_SAMPLES:
        case HCI_FM_HELIUM_RXREPEATCOUNT:
        case HCI_FM_HELIUM_AF_ALGO:
        case HCI_FM_HELIUM_AF_SINR_GD_CH_TH:
        case HCI_FM_HELIUM_AF_SINR_TH:
            rd = (struct hci_fm_def_data_rd_req *) cmd_param;
            ret = hci_fm_default_data_read_req(rd);
            break;
        case HCI_FM_HELIUM_BLEND_SINRHI:
            ret = hci_fm_get_blend_req();
            break;
        case HCI_FM_HELIUM_BLEND_RMSSIHI:
            ret = hci_fm_get_blend_req();
            break;
    }
    if (ret == FM_HC_STATUS_SUCCESS) {
        if ((ret = clock_gettime(CLOCK_REALTIME, &ts)) == 0) {
            ts.tv_sec += HELIUM_CMD_TIME_OUT;
            ALOGD("%s:waiting for cmd %d response ", LOG_TAG, cmd);
            ret = pthread_cond_timedwait(&hal->cmd_cond, &hal->cmd_lock,
                    &ts);
            ALOGD("%s: received %d cmd response.", LOG_TAG, cmd);
        } else {
            ALOGE("%s: clock gettime failed. err = %d(%s)", LOG_TAG,
                    errno, strerror(errno));
        }
    }
    hal->set_cmd_sent = false;
    pthread_mutex_unlock(&hal->cmd_lock);

    return ret;
}

/* Callback function to be registered with FM-HCI for event notification */
static struct fm_hci_callbacks_t hal_cb = {
    process_event,
    fm_hci_close_done
};

int hal_init(const fm_hal_callbacks_t *cb)
{
    int ret = -FM_HC_STATUS_FAIL;
    fm_hci_hal_t hci_hal;
    struct timespec ts;

    ALOGD("++%s", __func__);

    memset(&hci_hal, 0, sizeof(fm_hci_hal_t));

    pthread_mutex_lock(&(hal_lock));
    while (hal) {
        ALOGE("%s:HAL is still available wait for last hal session to close", __func__);
        if ((ret = clock_gettime(CLOCK_REALTIME, &ts)) == 0) {
            ts.tv_sec += HAL_TIMEOUT;
            ret = pthread_cond_timedwait(&hal_cond, &hal_lock,
                    &ts);
            if(ret == ETIMEDOUT) {
                ALOGE("%s:FM Hci close is stuck kiiling the fm process", __func__);
                kill(getpid(), SIGKILL);
            } else {
                ALOGD("%s:last HAL session is closed ", LOG_TAG);
            }
        } else {
            ALOGE("%s: clock gettime failed. err = %d(%s)", LOG_TAG,
                    errno, strerror(errno));
        }
    }
    pthread_mutex_unlock(&hal_lock);

    hal = malloc(sizeof(struct fm_hal_t));
    if (!hal) {
        ALOGE("%s:Failed to allocate memory", __func__);
        ret = -FM_HC_STATUS_NOMEM;
        goto out;
    }
    memset(hal, 0, sizeof(struct fm_hal_t));
    hal->jni_cb = cb;

    pthread_mutex_init(&hal->cmd_lock, NULL);
    pthread_cond_init(&hal->cmd_cond, NULL);
    hal->set_cmd_sent = false;

    hal->radio = malloc(sizeof(struct radio_helium_device));
    if (!hal->radio) {
        ALOGE("%s:Failed to allocate memory for device", __func__);
        goto out;
    }

    memset(hal->radio, 0,  sizeof(struct radio_helium_device));

    hci_hal.hal = hal;
    hci_hal.cb = &hal_cb;

    /* Initialize the FM-HCI */
    ret = fm_hci_init(&hci_hal);
    if (ret != FM_HC_STATUS_SUCCESS) {
        ALOGE("%s:fm_hci_init failed", __func__);
        goto out;
    }
    hal->private_data = hci_hal.hci;

    return FM_HC_STATUS_SUCCESS;

out:
    ALOGV("--%s", __func__);
    if (hal) {
        if (hal->radio) {
            free(hal->radio);
            hal->radio = NULL;
        }
        hal->jni_cb = NULL;
        pthread_cond_destroy(&hal->cmd_cond);
        pthread_mutex_destroy(&hal->cmd_lock);
        free(hal);
        hal = NULL;
    }
    return ret;
}

/* Called by the JNI for performing the FM operations */
static int set_fm_ctrl(int cmd, int val)
{
    int ret = 0;
    int saved_val;
    char temp_val = 0;
    unsigned int rds_grps_proc = 0;
    struct hci_fm_def_data_wr_req def_data_wrt;
    struct hci_fm_def_data_rd_req def_data_rd;

    pthread_mutex_lock(&hal_lock);
    if (!hal) {
        ALOGE("%s:ALERT: command sent before hal init", __func__);
        ret = -FM_HC_STATUS_FAIL;
        goto end;
    }
    ALOGD("%s:cmd: %x, val: %d",LOG_TAG, cmd, val);

    switch (cmd) {
    case HCI_FM_HELIUM_AUDIO_MUTE:
        saved_val = hal->radio->mute_mode.hard_mute;
        hal->radio->mute_mode.hard_mute = val;
        ret = hci_fm_mute_mode_req(&hal->radio->mute_mode);
        if (ret < 0) {
            ALOGE("%s:Error while set FM hard mute :%d", LOG_TAG, ret);
            hal->radio->mute_mode.hard_mute = saved_val;
        }
        break;
    case HCI_FM_HELIUM_SRCHMODE:
        if (is_valid_srch_mode(val))
            hal->radio->g_search_mode = val;
        else
            ret = -EINVAL;
        break;
    case HCI_FM_HELIUM_SCANDWELL:
        if (is_valid_scan_dwell_prd(val))
            hal->radio->g_scan_time = val;
        else
            ret = -EINVAL;
        break;
    case HCI_FM_HELIUM_SRCHON:
        helium_search_req(val, SRCH_DIR_UP);
        break;
    case HCI_FM_HELIUM_STATE:
        switch (val) {
        case FM_RECV:
            ret = hci_fm_enable_recv_req();
            break;
        case FM_OFF:
            hal->radio->mode = FM_TURNING_OFF;
            hci_fm_disable_recv_req();
            break;
        default:
            break;
        }
        break;
    case HCI_FM_HELIUM_REGION:
        ret = helium_recv_set_region(val);
        break;
    case HCI_FM_HELIUM_SIGNAL_TH:
        temp_val = val;
        ret = helium_set_sig_threshold_req(temp_val);
        if (ret < 0) {
            ALOGE("%s:Error while setting signal threshold\n", LOG_TAG);
            goto end;
        }
        break;
    case HCI_FM_HELIUM_SRCH_PTY:
        if (is_valid_pty(val)) {
            hal->radio->srch_rds.srch_pty = val;
            hal->radio->srch_st_list.srch_pty = val;
        } else {
            ret = -EINVAL;
        }
         break;
    case HCI_FM_HELIUM_SRCH_PI:
         if (is_valid_pi(val))
             hal->radio->srch_rds.srch_pi = val;
         else
             ret = -EINVAL;
         break;
    case HCI_FM_HELIUM_SRCH_CNT:
         if (is_valid_srch_station_cnt(val))
             hal->radio->srch_st_list.srch_list_max = val;
         else
             ret = -EINVAL;
         break;
    case HCI_FM_HELIUM_SPACING:
         saved_val = hal->radio->recv_conf.ch_spacing;
         hal->radio->recv_conf.ch_spacing = val;
         ret = hci_fm_set_recv_conf_req(&hal->radio->recv_conf);
         if (ret < 0) {
             ALOGE("%s:Error in setting channel spacing", LOG_TAG);
             hal->radio->recv_conf.ch_spacing = saved_val;
             goto end;
        }
        break;
    case HCI_FM_HELIUM_EMPHASIS:
         saved_val = hal->radio->recv_conf.emphasis;
         hal->radio->recv_conf.emphasis = val;
         ret = hci_fm_set_recv_conf_req(&hal->radio->recv_conf);
         if (ret < 0) {
             ALOGE("%s:Error in setting emphasis", LOG_TAG);
             hal->radio->recv_conf.emphasis = saved_val;
             goto end;
         }
         break;
    case HCI_FM_HELIUM_RDS_STD:
         saved_val = hal->radio->recv_conf.rds_std;
         hal->radio->recv_conf.rds_std = val;
         ret = hci_fm_set_recv_conf_req(&hal->radio->recv_conf);
         if (ret < 0) {
             ALOGE("%s:Error in rds_std", LOG_TAG);
             hal->radio->recv_conf.rds_std = saved_val;
             goto end;
         }
         break;
    case HCI_FM_HELIUM_RDSON:
         saved_val = hal->radio->recv_conf.rds_std;
         hal->radio->recv_conf.rds_std = val;
         ret = hci_fm_set_recv_conf_req(&hal->radio->recv_conf);
         if (ret < 0) {
             ALOGE("%s:Error in rds_std", LOG_TAG);
             hal->radio->recv_conf.rds_std = saved_val;
             goto end;
         }
         break;
    case HCI_FM_HELIUM_RDSGROUP_MASK:
         saved_val = hal->radio->rds_grp.rds_grp_enable_mask;
         grp_mask = (grp_mask | oda_agt | val);
         hal->radio->rds_grp.rds_grp_enable_mask = grp_mask;
         hal->radio->rds_grp.rds_buf_size = 1;
         hal->radio->rds_grp.en_rds_change_filter = 0;
         ret = helium_rds_grp_mask_req(&hal->radio->rds_grp);
         if (ret < 0) {
             ALOGE("%s:error in setting group mask\n", LOG_TAG);
             hal->radio->rds_grp.rds_grp_enable_mask = saved_val;
             goto end;
        }
        break;
    case HCI_FM_HELIUM_RDSGROUP_PROC:
         saved_val = hal->radio->g_rds_grp_proc_ps;
         rds_grps_proc = hal->radio->g_rds_grp_proc_ps | (val & 0xFF);
         hal->radio->g_rds_grp_proc_ps = rds_grps_proc;
         ret = helium_rds_grp_process_req(hal->radio->g_rds_grp_proc_ps);
         if (ret < 0) {
             hal->radio->g_rds_grp_proc_ps = saved_val;
             goto end;
         }
         break;

    case HCI_FM_HELIUM_RDS_GRP_COUNTERS:
         ALOGD("%s: rds_grp counter read  value=%d ", LOG_TAG,val);
         saved_val = hal->radio->g_rds_grp_proc_ps;
         ret = hci_fm_get_rds_grpcounters_req(val);
         if (ret < 0) {
             hal->radio->g_rds_grp_proc_ps = saved_val;
             goto end;
         }
         break;

    case HCI_FM_HELIUM_RDS_GRP_COUNTERS_EXT:
         ALOGD("%s: rds_grp counter read  value=%d ", LOG_TAG,val);
         saved_val = hal->radio->g_rds_grp_proc_ps;
         ret = hci_fm_get_rds_grpcounters_ext_req(val);
         if (ret < 0) {
            hal->radio->g_rds_grp_proc_ps = saved_val;
            goto end ;
         }
         break;

    case HCI_FM_HELIUM_SET_NOTCH_FILTER:
         ALOGD("%s: set notch filter  notch=%d ", LOG_TAG,val);
         ret = hci_fm_set_notch_filter_req(val);
         if (ret < 0) {
            goto end;
         }
         break;

    case HCI_FM_HELIUM_RDSD_BUF:
         hal->radio->rds_grp.rds_buf_size = val;
         break;
    case HCI_FM_HELIUM_PSALL:
         saved_val = hal->radio->g_rds_grp_proc_ps;
         rds_grps_proc = (val << RDS_CONFIG_OFFSET);
         hal->radio->g_rds_grp_proc_ps |= rds_grps_proc;
         ret = helium_rds_grp_process_req(hal->radio->g_rds_grp_proc_ps);
         if (ret < 0) {
             hal->radio->g_rds_grp_proc_ps = saved_val;
             goto end;
        }
        break;
    case HCI_FM_HELIUM_AF_JUMP:
        saved_val = hal->radio->g_rds_grp_proc_ps;
        hal->radio->g_rds_grp_proc_ps &= ~(1 << RDS_AF_JUMP_OFFSET);
        hal->radio->af_jump_bit = val;
        rds_grps_proc = 0x00;
        rds_grps_proc = (val << RDS_AF_JUMP_OFFSET);
        hal->radio->g_rds_grp_proc_ps |= rds_grps_proc;
        ret = helium_rds_grp_process_req(hal->radio->g_rds_grp_proc_ps);
        if (ret < 0) {
            hal->radio->g_rds_grp_proc_ps = saved_val;
            goto end;
        }
        break;
    case HCI_FM_HELIUM_LP_MODE:
         set_low_power_mode(val);
         break;
    case HCI_FM_HELIUM_ANTENNA:
        temp_val = val;
        ret = helium_set_antenna_req(temp_val);
        if (ret < 0) {
            ALOGE("%s:Set Antenna failed retval = %x", LOG_TAG, ret);
            goto end;
        }
        hal->radio->g_antenna =  val;
        break;
    case HCI_FM_HELIUM_SOFT_MUTE:
         saved_val = hal->radio->mute_mode.soft_mute;
         hal->radio->mute_mode.soft_mute = val;
         ret = helium_set_fm_mute_mode_req(&hal->radio->mute_mode);
         if (ret < 0) {
             ALOGE("%s:Error while setting FM soft mute %d", LOG_TAG, ret);
             hal->radio->mute_mode.soft_mute = saved_val;
             goto end;
         }
         break;
    case HCI_FM_HELIUM_FREQ:
        hci_fm_tune_station_req(val);
        break;
    case HCI_FM_HELIUM_SEEK:
        helium_search_req(1, val);
        break;
    case HCI_FM_HELIUM_UPPER_BAND:
        hal->radio->recv_conf.band_high_limit = val;
        break;
    case HCI_FM_HELIUM_LOWER_BAND:
        hal->radio->recv_conf.band_low_limit = val;
        break;
    case HCI_FM_HELIUM_AUDIO_MODE:
        hal->radio->stereo_mode.stereo_mode = (char)val ? 0:1;
        hal->radio->stereo_mode.sig_blend  = 1;
        hal->radio->stereo_mode.intf_blend = 0;
        hal->radio->stereo_mode.most_switch =0;
        hci_set_fm_stereo_mode_req(&hal->radio->stereo_mode);
        break;
    case HCI_FM_HELIUM_RIVA_ACCS_ADDR:
        hal->radio->riva_data_req.cmd_params.start_addr = val;
        break;
    case HCI_FM_HELIUM_RIVA_ACCS_LEN:
        if (is_valid_peek_len(val)) {
            hal->radio->riva_data_req.cmd_params.length = val;
        } else {
            ret = -1;
            ALOGE("%s: riva access len is not valid\n", LOG_TAG);
            goto end;
        }
        break;
    case HCI_FM_HELIUM_RIVA_PEEK:
        hal->radio->riva_data_req.cmd_params.subopcode = RIVA_PEEK_OPCODE;
        val = hci_peek_data(&hal->radio->riva_data_req.cmd_params);
        break;
    case HCI_FM_HELIUM_RIVA_POKE:
         if (hal->radio->riva_data_req.cmd_params.length <=
                    MAX_RIVA_PEEK_RSP_SIZE) {
             hal->radio->riva_data_req.cmd_params.subopcode =
                                                RIVA_POKE_OPCODE;
             ret = hci_poke_data(&hal->radio->riva_data_req);
         } else {
             ALOGE("%s: riva access len is not valid for poke\n", LOG_TAG);
             ret = -1;
             goto end;
         }
         break;
    case HCI_FM_HELIUM_SSBI_ACCS_ADDR:
        hal->radio->ssbi_data_accs.start_addr = val;
        break;
    case HCI_FM_HELIUM_SSBI_POKE:
        hal->radio->ssbi_data_accs.data = val;
        ret = hci_ssbi_poke_reg(&hal->radio->ssbi_data_accs);
        break;
    case HCI_FM_HELIUM_SSBI_PEEK:
        hal->radio->ssbi_peek_reg.start_address = val;
        hci_ssbi_peek_reg(&hal->radio->ssbi_peek_reg);
        break;
    case HCI_FM_HELIUM_AGC_UCCTRL:
        hal->radio->set_get_reset_agc.ucctrl = val;
        break;
    case HCI_FM_HELIUM_AGC_GAIN_STATE:
        hal->radio->set_get_reset_agc.ucgainstate = val;
        hci_get_set_reset_agc_req(&hal->radio->set_get_reset_agc);
        break;
    case HCI_FM_HELIUM_SINR_SAMPLES:
         if (!is_valid_sinr_samples(val)) {
             ALOGE("%s: sinr samples count is not valid\n", __func__);
             ret = -1;
             goto end;
         }
         ret = helium_send_hci_cmd(cmd, NULL);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         hal->radio->ch_det_threshold.sinr_samples = (char)val;
         ret = set_ch_det_thresholds_req(&hal->radio->ch_det_threshold);
         if (ret < 0) {
             ALOGE("Failed to set SINR samples  %d", ret);
             goto end;
         }
         break;
    case HCI_FM_HELIUM_SINR_THRESHOLD:
         if (!is_valid_sinr_th(val)) {
             ALOGE("%s: sinr threshold is not valid\n", __func__);
             ret = -1;
             goto end;
         }
         ret = helium_send_hci_cmd(cmd, NULL);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         hal->radio->ch_det_threshold.sinr = (char)val;
         ret = set_ch_det_thresholds_req(&hal->radio->ch_det_threshold);
         break;
    case HCI_FM_HELIUM_INTF_LOW_THRESHOLD:
         if (!is_valid_intf_det_low_th(val)) {
             ALOGE("%s: intf det low threshold is not valid\n", __func__);
             ret = -1;
             goto end;
         }
         ret = helium_send_hci_cmd(cmd, NULL);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         hal->radio->ch_det_threshold.low_th = (char)val;
         ret = set_ch_det_thresholds_req(&hal->radio->ch_det_threshold);
         break;
    case HCI_FM_HELIUM_INTF_HIGH_THRESHOLD:
         if (!is_valid_intf_det_hgh_th(val)) {
             ALOGE("%s: intf high threshold is not valid\n", __func__);
             ret = -1;
             goto end;
         }
         ret = helium_send_hci_cmd(cmd, NULL);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         hal->radio->ch_det_threshold.high_th = (char)val;
         ret = set_ch_det_thresholds_req(&hal->radio->ch_det_threshold);
         break;
    case HCI_FM_HELIUM_SINRFIRSTSTAGE:
         def_data_rd.mode = FM_SRCH_CONFG_MODE;
         def_data_rd.length = FM_SRCH_CNFG_LEN;
         def_data_rd.param_len = 0;
         def_data_rd.param = 0;

         ret = helium_send_hci_cmd(cmd, (void *)&def_data_rd);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         def_data_wrt.mode = FM_SRCH_CONFG_MODE;
         def_data_wrt.length = FM_SRCH_CNFG_LEN;
         memcpy(&def_data_wrt.data, &hal->radio->def_data.data,
                 hal->radio->def_data.data_len);
         def_data_wrt.data[SINRFIRSTSTAGE_OFFSET] = val;
         ret = hci_fm_default_data_write_req(&def_data_wrt);
         break;
    case HCI_FM_HELIUM_RMSSIFIRSTSTAGE:
         def_data_rd.mode = FM_SRCH_CONFG_MODE;
         def_data_rd.length = FM_SRCH_CNFG_LEN;
         def_data_rd.param_len = 0;
         def_data_rd.param = 0;

         ret = helium_send_hci_cmd(cmd, (void *)&def_data_rd);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         def_data_wrt.mode = FM_SRCH_CONFG_MODE;
         def_data_wrt.length = FM_SRCH_CNFG_LEN;
         memcpy(&def_data_wrt.data, &hal->radio->def_data.data,
                 hal->radio->def_data.data_len);
         def_data_wrt.data[RMSSIFIRSTSTAGE_OFFSET] = val;
         ret = hci_fm_default_data_write_req(&def_data_wrt);
         break;
    case HCI_FM_HELIUM_CF0TH12:
         def_data_rd.mode = FM_SRCH_CONFG_MODE;
         def_data_rd.length = FM_SRCH_CNFG_LEN;
         def_data_rd.param_len = 0;
         def_data_rd.param = 0;

         ret = helium_send_hci_cmd(cmd, (void *)&def_data_rd);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         def_data_wrt.mode = FM_SRCH_CONFG_MODE;
         def_data_wrt.length = FM_SRCH_CNFG_LEN;
         memcpy(&def_data_wrt.data, &hal->radio->def_data.data,
                 hal->radio->def_data.data_len);
         def_data_wrt.data[CF0TH12_BYTE1_OFFSET] = (val & 0xFF);
         def_data_wrt.data[CF0TH12_BYTE2_OFFSET] = ((val >> 8) & 0xFF);
         ret = hci_fm_default_data_write_req(&def_data_wrt);
         break;
    case HCI_FM_HELIUM_SRCHALGOTYPE:
         def_data_rd.mode = FM_SRCH_CONFG_MODE;
         def_data_rd.length = FM_SRCH_CNFG_LEN;
         def_data_rd.param_len = 0;
         def_data_rd.param = 0;

         ret = helium_send_hci_cmd(cmd, (void *)&def_data_rd);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         def_data_wrt.mode = FM_SRCH_CONFG_MODE;
         def_data_wrt.length = FM_SRCH_CNFG_LEN;
         memcpy(&def_data_wrt.data, &hal->radio->def_data.data,
                 hal->radio->def_data.data_len);
         def_data_wrt.data[SRCH_ALGO_TYPE_OFFSET] = val;
         ret = hci_fm_default_data_write_req(&def_data_wrt);
         break;
    case HCI_FM_HELIUM_AF_RMSSI_TH:
         def_data_rd.mode = FM_AFJUMP_CONFG_MODE;
         def_data_rd.length = FM_AFJUMP_CNFG_LEN;
         def_data_rd.param_len = 0;
         def_data_rd.param = 0;

         ret = helium_send_hci_cmd(cmd, (void *)&def_data_rd);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         def_data_wrt.mode = FM_AFJUMP_CONFG_MODE;
         def_data_wrt.length = FM_AFJUMP_CNFG_LEN;
         memcpy(&def_data_wrt.data, &hal->radio->def_data.data,
                 hal->radio->def_data.data_len);
         def_data_wrt.data[AF_RMSSI_TH_OFFSET] = (val & 0xFF);
         ret = hci_fm_default_data_write_req(&def_data_wrt);
         break;
    case HCI_FM_HELIUM_GOOD_CH_RMSSI_TH:
         def_data_rd.mode = FM_AFJUMP_CONFG_MODE;
         def_data_rd.length = FM_AFJUMP_CNFG_LEN;
         def_data_rd.param_len = 0;
         def_data_rd.param = 0;

         ret = helium_send_hci_cmd(cmd, (void *)&def_data_rd);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         def_data_wrt.mode = FM_AFJUMP_CONFG_MODE;
         def_data_wrt.length = FM_AFJUMP_CNFG_LEN;
         memcpy(&def_data_wrt.data, &hal->radio->def_data.data,
                 hal->radio->def_data.data_len);
         def_data_wrt.data[GD_CH_RMSSI_TH_OFFSET] = val;
         ret = hci_fm_default_data_write_req(&def_data_wrt);
         break;
    case HCI_FM_HELIUM_AF_RMSSI_SAMPLES:
         def_data_rd.mode = FM_AFJUMP_CONFG_MODE;
         def_data_rd.length = FM_AFJUMP_CNFG_LEN;
         def_data_rd.param_len = 0;
         def_data_rd.param = 0;

         ret = helium_send_hci_cmd(cmd, (void *)&def_data_rd);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         def_data_wrt.mode = FM_AFJUMP_CONFG_MODE;
         def_data_wrt.length = FM_AFJUMP_CNFG_LEN;
         memcpy(&def_data_wrt.data, &hal->radio->def_data.data,
                 hal->radio->def_data.data_len);
         def_data_wrt.data[AF_RMSSI_SAMPLES_OFFSET] = val;
         ret = hci_fm_default_data_write_req(&def_data_wrt);
         break;
    case HCI_FM_HELIUM_RXREPEATCOUNT:
         def_data_rd.mode = RDS_PS0_XFR_MODE;
         def_data_rd.length = RDS_PS0_LEN;
         def_data_rd.param_len = 0;
         def_data_rd.param = 0;

         ret = helium_send_hci_cmd(cmd, (void *)&def_data_rd);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         def_data_wrt.mode = RDS_PS0_XFR_MODE;
         def_data_wrt.length = RDS_PS0_LEN;
         memcpy(&def_data_wrt.data, &hal->radio->def_data.data,
                 hal->radio->def_data.data_len);
         def_data_wrt.data[RX_REPEATE_BYTE_OFFSET] = val;
         ret = hci_fm_default_data_write_req(&def_data_wrt);
         break;
    case HCI_FM_HELIUM_AF_ALGO:
         def_data_rd.mode = FM_AFJUMP_CONFG_MODE;
         def_data_rd.length = FM_AFJUMP_CNFG_LEN;
         def_data_rd.param_len = 0;
         def_data_rd.param = 0;

         ret = helium_send_hci_cmd(cmd, (void *)&def_data_rd);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         def_data_wrt.mode = FM_AFJUMP_CONFG_MODE;
         def_data_wrt.length = FM_AFJUMP_CNFG_LEN;
         memcpy(&def_data_wrt.data, &hal->radio->def_data.data,
                 hal->radio->def_data.data_len);
         def_data_wrt.data[AF_ALGO_OFFSET] = (char)val;
         ret = hci_fm_default_data_write_req(&def_data_wrt);
         break;
    case HCI_FM_HELIUM_AF_SINR_GD_CH_TH:
         def_data_rd.mode = FM_AFJUMP_CONFG_MODE;
         def_data_rd.length = FM_AFJUMP_CNFG_LEN;
         def_data_rd.param_len = 0;
         def_data_rd.param = 0;

         ret = helium_send_hci_cmd(cmd, (void *)&def_data_rd);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         def_data_wrt.mode = FM_AFJUMP_CONFG_MODE;
         def_data_wrt.length = FM_AFJUMP_CNFG_LEN;
         memcpy(&def_data_wrt.data, &hal->radio->def_data.data,
                 hal->radio->def_data.data_len);
         def_data_wrt.data[AF_SINR_GD_CH_TH_OFFSET] = (char)val;
         ret = hci_fm_default_data_write_req(&def_data_wrt);
         break;
    case HCI_FM_HELIUM_AF_SINR_TH:
         def_data_rd.mode = FM_AFJUMP_CONFG_MODE;
         def_data_rd.length = FM_AFJUMP_CNFG_LEN;
         def_data_rd.param_len = 0;
         def_data_rd.param = 0;

         ret = helium_send_hci_cmd(cmd, (void *)&def_data_rd);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }
         def_data_wrt.mode = FM_AFJUMP_CONFG_MODE;
         def_data_wrt.length = FM_AFJUMP_CNFG_LEN;
         memcpy(&def_data_wrt.data, &hal->radio->def_data.data,
                 hal->radio->def_data.data_len);
         def_data_wrt.data[AF_SINR_TH_OFFSET] = (char)val;
         ret = hci_fm_default_data_write_req(&def_data_wrt);
         break;
    case HCI_FM_HELIUM_BLEND_SINRHI:
         if (!is_valid_blend_value(val)) {
             ALOGE("%s: sinr samples count is not valid\n", __func__);
             ret = -1;
             goto end;
         }
         ret = helium_send_hci_cmd(cmd, NULL);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }

         hal->radio->blend_tbl.BlendSinrHi = (char)val;
         ret = hci_fm_set_blend_tbl_req(&hal->radio->blend_tbl);
         break;
    case HCI_FM_HELIUM_BLEND_RMSSIHI:
         if (!is_valid_blend_value(val)) {
             ALOGE("%s: sinr samples count is not valid\n", __func__);
             ret = -1;
             goto end;
         }
         ret = helium_send_hci_cmd(cmd, NULL);
         if (ret != FM_HC_STATUS_SUCCESS) {
             ALOGE("%s: 0x%x cmd failed", LOG_TAG, cmd);
             goto end;
         }

         hal->radio->blend_tbl.BlendRmssiHi = (char)val;
         ret = hci_fm_set_blend_tbl_req(&hal->radio->blend_tbl);
         break;
    case HCI_FM_HELIUM_ENABLE_LPF:
         ALOGI("%s: val: %x", __func__, val);
         if (!(ret = hci_fm_enable_lpf(val))) {
             ALOGI("%s: command sent sucessfully", __func__);
         }
         break;
    case HCI_FM_HELIUM_AUDIO:
         ALOGE("%s slimbus port", val ? "enable" : "disable");
         ret = hci_fm_enable_slimbus(val);
         break;
    default:
        ALOGE("%s:%s: Not a valid FM CMD!!", LOG_TAG, __func__);
        ret = 0;
        break;
    }
end:
    if (ret < 0)
        ALOGE("%s:%s: 0x%x cmd failed", LOG_TAG, __func__, cmd);
    pthread_mutex_unlock(&hal_lock);
    return ret;
}

static int get_fm_ctrl(int cmd, int *val)
{
    int ret = 0;
    struct hci_fm_def_data_rd_req def_data_rd;

    pthread_mutex_lock(&hal_lock);
    if (!hal) {
        ALOGE("%s:ALERT: command sent before hal_init", __func__);
        ret = -FM_HC_STATUS_FAIL;
        goto end;
    }

    ALOGE("%s: cmd = 0x%x", __func__, cmd);
    switch(cmd) {
    case HCI_FM_HELIUM_FREQ:
        if (!val) {
            ret = -FM_HC_STATUS_NULL_POINTER;
            goto end;
        }
        *val = hal->radio->fm_st_rsp.station_rsp.station_freq;
        break;
    case HCI_FM_HELIUM_UPPER_BAND:
        if (!val) {
            ret = -FM_HC_STATUS_NULL_POINTER;
            goto end;
        }
        *val = hal->radio->recv_conf.band_high_limit;
        break;
    case HCI_FM_HELIUM_LOWER_BAND:
        if (!val) {
            ret = -FM_HC_STATUS_NULL_POINTER;
            goto end;
        }
        *val = hal->radio->recv_conf.band_low_limit;
        break;
    case HCI_FM_HELIUM_AUDIO_MUTE:
        if (!val) {
            ret = -FM_HC_STATUS_NULL_POINTER;
            goto end;
        }
        *val = hal->radio->mute_mode.hard_mute;
        break;
    case HCI_FM_HELIUM_SINR_SAMPLES:
        set_bit(ch_det_th_mask_flag, CMD_CHDET_SINR_SAMPLE);
        ret = hci_fm_get_ch_det_th();
        if (ret != FM_HC_STATUS_SUCCESS)
            clear_bit(ch_det_th_mask_flag, CMD_CHDET_SINR_SAMPLE);
        break;
    case HCI_FM_HELIUM_SINR_THRESHOLD:
        set_bit(ch_det_th_mask_flag, CMD_CHDET_SINR_TH);
        ret = hci_fm_get_ch_det_th();
        if (ret != FM_HC_STATUS_SUCCESS)
            clear_bit(ch_det_th_mask_flag, CMD_CHDET_SINR_TH);
        break;
    case HCI_FM_HELIUM_INTF_LOW_THRESHOLD:
        set_bit(ch_det_th_mask_flag, CMD_CHDET_INTF_TH_LOW);
        ret = hci_fm_get_ch_det_th();
        if (ret != FM_HC_STATUS_SUCCESS)
            clear_bit(ch_det_th_mask_flag, CMD_CHDET_INTF_TH_LOW);
        break;
    case HCI_FM_HELIUM_INTF_HIGH_THRESHOLD:
        set_bit(ch_det_th_mask_flag, CMD_CHDET_INTF_TH_HIGH);
        ret = hci_fm_get_ch_det_th();
        if (ret != FM_HC_STATUS_SUCCESS)
            clear_bit(ch_det_th_mask_flag, CMD_CHDET_INTF_TH_HIGH);
        break;
    case HCI_FM_HELIUM_SINRFIRSTSTAGE:
        set_bit(def_data_rd_mask_flag, CMD_DEFRD_SINR_FIRST_STAGE);
        def_data_rd.mode = FM_SRCH_CONFG_MODE;
        def_data_rd.length = FM_SRCH_CNFG_LEN;
        goto cmd;
    case HCI_FM_HELIUM_RMSSIFIRSTSTAGE:
        set_bit(def_data_rd_mask_flag, CMD_DEFRD_RMSSI_FIRST_STAGE);
        def_data_rd.mode = FM_SRCH_CONFG_MODE;
        def_data_rd.length = FM_SRCH_CNFG_LEN;
        goto cmd;
    case HCI_FM_HELIUM_CF0TH12:
        set_bit(def_data_rd_mask_flag, CMD_DEFRD_CF0TH12);
        def_data_rd.mode = FM_SRCH_CONFG_MODE;
        def_data_rd.length = FM_SRCH_CNFG_LEN;
        goto cmd;
    case HCI_FM_HELIUM_SRCHALGOTYPE:
        def_data_rd.mode = FM_SRCH_CONFG_MODE;
        def_data_rd.length = FM_SRCH_CNFG_LEN;
        set_bit(def_data_rd_mask_flag, CMD_DEFRD_SEARCH_ALGO);
        goto cmd;
    case HCI_FM_HELIUM_AF_RMSSI_TH:
        set_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_RMSSI_TH);
        def_data_rd.mode = FM_AFJUMP_CONFG_MODE;
        def_data_rd.length = FM_AFJUMP_CNFG_LEN;
        goto cmd;
    case HCI_FM_HELIUM_GOOD_CH_RMSSI_TH:
        set_bit(def_data_rd_mask_flag, CMD_DEFRD_GD_CH_RMSSI_TH);
        def_data_rd.mode = FM_AFJUMP_CONFG_MODE;
        def_data_rd.length = FM_AFJUMP_CNFG_LEN;
        goto cmd;
    case HCI_FM_HELIUM_AF_RMSSI_SAMPLES:
        set_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_RMSSI_SAMPLE);
        def_data_rd.mode = FM_AFJUMP_CONFG_MODE;
        def_data_rd.length = FM_AFJUMP_CNFG_LEN;
        goto cmd;
    case HCI_FM_HELIUM_AF_ALGO:
        set_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_ALGO);
        def_data_rd.mode = FM_AFJUMP_CONFG_MODE;
        def_data_rd.length = FM_AFJUMP_CNFG_LEN;
        goto cmd;
    case HCI_FM_HELIUM_AF_SINR_GD_CH_TH:
        set_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_SINR_GD_CH_TH);
        def_data_rd.mode = FM_AFJUMP_CONFG_MODE;
        def_data_rd.length = FM_AFJUMP_CNFG_LEN;
        goto cmd;
    case HCI_FM_HELIUM_AF_SINR_TH:
        set_bit(def_data_rd_mask_flag, CMD_DEFRD_AF_SINR_TH);
        def_data_rd.mode = FM_AFJUMP_CONFG_MODE;
        def_data_rd.length = FM_AFJUMP_CNFG_LEN;
cmd:
        def_data_rd.param_len = 0;
        def_data_rd.param = 0;

        ret = hci_fm_default_data_read_req(&def_data_rd);
        if (ret != FM_HC_STATUS_SUCCESS)
            clear_all_bit(def_data_rd_mask_flag);
        break;
    case HCI_FM_HELIUM_RXREPEATCOUNT:
        def_data_rd.mode = RDS_PS0_XFR_MODE;
        def_data_rd.length = RDS_PS0_LEN;
        def_data_rd.param_len = 0;
        def_data_rd.param = 0;
        set_bit(def_data_rd_mask_flag, CMD_DEFRD_REPEATCOUNT);

        ret = hci_fm_default_data_read_req(&def_data_rd);
        if (ret != FM_HC_STATUS_SUCCESS)
            clear_bit(def_data_rd_mask_flag, CMD_DEFRD_REPEATCOUNT);
        break;
    case HCI_FM_HELIUM_BLEND_SINRHI:
        set_bit(blend_tbl_mask_flag, CMD_BLENDTBL_SINR_HI);
        ret = hci_fm_get_blend_req();
        if (ret != FM_HC_STATUS_SUCCESS)
            clear_bit(blend_tbl_mask_flag, CMD_BLENDTBL_SINR_HI);
        break;
    case HCI_FM_HELIUM_BLEND_RMSSIHI:
        set_bit(blend_tbl_mask_flag, CMD_BLENDTBL_RMSSI_HI);
        ret = hci_fm_get_blend_req();
        if (ret != FM_HC_STATUS_SUCCESS)
            clear_bit(blend_tbl_mask_flag, CMD_BLENDTBL_RMSSI_HI);
        break;
    case HCI_FM_HELIUM_IOVERC:
        set_bit(station_dbg_param_mask_flag, CMD_STNDBGPARAM_IOVERC);
        ret = hci_fm_get_station_dbg_param_req();
        if (ret != FM_HC_STATUS_SUCCESS)
            clear_bit(station_dbg_param_mask_flag, CMD_STNDBGPARAM_IOVERC);
        break;
    case HCI_FM_HELIUM_INTDET:
        set_bit(station_dbg_param_mask_flag, CMD_STNDBGPARAM_INFDETOUT);
        ret = hci_fm_get_station_dbg_param_req();
        if (ret != FM_HC_STATUS_SUCCESS)
            clear_bit(station_dbg_param_mask_flag, CMD_STNDBGPARAM_INFDETOUT);
        break;
    case HCI_FM_HELIUM_GET_SINR:
        if (hal->radio->mode == FM_RECV) {
            set_bit(station_param_mask_flag, CMD_STNPARAM_SINR);
            ret = hci_fm_get_station_cmd_param_req();
            if (ret != FM_HC_STATUS_SUCCESS)
                clear_bit(station_param_mask_flag, CMD_STNPARAM_SINR);
        } else {
            ALOGE("HCI_FM_HELIUM_GET_SINR: radio is not in recv mode");
            ret = -EINVAL;
        }
        break;
    case HCI_FM_HELIUM_RMSSI:
        if (hal->radio->mode == FM_RECV) {
            set_bit(station_param_mask_flag, CMD_STNPARAM_RSSI);
            ret = hci_fm_get_station_cmd_param_req();
            if (ret != FM_HC_STATUS_SUCCESS)
                clear_bit(station_param_mask_flag, CMD_STNPARAM_RSSI);
        } else if (hal->radio->mode == FM_TRANS) {
            ALOGE("HCI_FM_HELIUM_RMSSI: radio is not in recv mode");
            ret = -EINVAL;
        }
        break;
    default:
        break;
    }

end:
    if (ret < 0)
        ALOGE("%s:%s: %d cmd failed", LOG_TAG, __func__, cmd);
    pthread_mutex_unlock(&hal_lock);
    return ret;
}

const struct fm_interface_t FM_HELIUM_LIB_INTERFACE = {
    hal_init,
    set_fm_ctrl,
    get_fm_ctrl
};
