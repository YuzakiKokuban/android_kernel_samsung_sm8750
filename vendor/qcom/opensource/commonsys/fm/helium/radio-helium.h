/*
Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.

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

#ifndef __RADIO_HELIUM_H__
#define __RADIO_HELIUM_H__

#include <stdbool.h>

#define HELIUM_CMD_TIME_OUT (5)
#define MIN_TX_TONE_VAL  0x00
#define MAX_TX_TONE_VAL  0x07
#define MIN_HARD_MUTE_VAL  0x00
#define MAX_HARD_MUTE_VAL  0x03
#define MIN_SRCH_MODE  0x00
#define MAX_SRCH_MODE  0x09
#define MIN_SCAN_DWELL  0x00
#define MAX_SCAN_DWELL  0x0F
#define MIN_SIG_TH  0x00
#define MAX_SIG_TH  0x03
#define MIN_PTY  0X00
#define MAX_PTY  0x1F
#define MIN_PI  0x0000
#define MAX_PI  0xFFFF
#define MIN_SRCH_STATIONS_CNT  0x00
#define MAX_SRCH_STATIONS_CNT  0x14
#define MIN_CHAN_SPACING  0x00
#define MAX_CHAN_SPACING  0x02
#define MIN_EMPHASIS  0x00
#define MAX_EMPHASIS  0x01
#define MIN_RDS_STD  0x00
#define MAX_RDS_STD  0x02
#define MIN_ANTENNA_VAL  0x00
#define MAX_ANTENNA_VAL  0x01
#define MIN_TX_PS_REPEAT_CNT  0x01
#define MAX_TX_PS_REPEAT_CNT  0x0F
#define MIN_SOFT_MUTE  0x00
#define MAX_SOFT_MUTE  0x01
#define MIN_PEEK_ACCESS_LEN  0x01
#define MAX_PEEK_ACCESS_LEN  0xF9
#define MIN_RESET_CNTR  0x00
#define MAX_RESET_CNTR  0x01
#define MIN_HLSI  0x00
#define MAX_HLSI  0x02
#define MIN_NOTCH_FILTER  0x00
#define MAX_NOTCH_FILTER  0x02
#define MIN_INTF_DET_OUT_LW_TH  0x00
#define MAX_INTF_DET_OUT_LW_TH  0xFF
#define MIN_INTF_DET_OUT_HG_TH  0x00
#define MAX_INTF_DET_OUT_HG_TH  0xFF
#define MIN_SINR_TH  -128
#define MAX_SINR_TH  127
#define MIN_SINR_SAMPLES  0x01
#define MAX_SINR_SAMPLES  0xFF
#define MIN_BLEND_HI  -128
#define MAX_BLEND_HI  127

/* HCI data types */
#define RADIO_HCI_COMMAND_PKT   0x11
#define RADIO_HCI_EVENT_PKT     0x14
/*HCI reponce packets*/
#define MAX_RIVA_PEEK_RSP_SIZE   251
/* default data access */
#define DEFAULT_DATA_OFFSET 2
#define DEFAULT_DATA_SIZE 249
/* Power levels are 0-7, but SOC will expect values from 0-255
 * So the each level step size will be 255/7 = 36 */
#define FM_TX_PWR_LVL_STEP_SIZE 36
#define FM_TX_PWR_LVL_0         0 /* Lowest power lvl that can be set for Tx */
#define FM_TX_PWR_LVL_MAX       7 /* Max power lvl for Tx */
#define FM_TX_PHY_CFG_MODE   0x3c
#define FM_TX_PHY_CFG_LEN    0x10
#define FM_TX_PWR_GAIN_OFFSET 14
/**RDS CONFIG MODE**/
#define FM_RDS_CNFG_MODE    0x0f
#define FM_RDS_CNFG_LEN     0x10

/**AF JUMP CONFIG MODE**/
#define AF_ALGO_OFFSET      0
#define AF_RMSSI_TH_OFFSET  1
#define AF_RMSSI_SAMPLES_OFFSET 2
#define AF_SINR_GD_CH_TH_OFFSET 4
#define AF_SINR_TH_OFFSET   5
/**RX CONFIG MODE**/
#define FM_RX_CONFG_MODE    0x15
#define FM_RX_CNFG_LEN      0x15
#define GD_CH_RMSSI_TH_OFFSET   0x03
#define MAX_GD_CH_RMSSI_TH  0x7F
#define SRCH_ALGO_TYPE_OFFSET  0x02
#define SINRFIRSTSTAGE_OFFSET  0x03
#define RMSSIFIRSTSTAGE_OFFSET 0x04
#define CF0TH12_BYTE1_OFFSET   0x00
#define CF0TH12_BYTE2_OFFSET   0x01
#define MAX_SINR_FIRSTSTAGE 0x7F
#define MAX_RMSSI_FIRSTSTAGE    0x7F
#define RDS_PS0_XFR_MODE 0x01
#define RDS_PS0_LEN 0x06
#define RX_REPEATE_BYTE_OFFSET 0x05
#define FM_SPUR_TBL_SIZE 0xF0
#define SPUR_DATA_LEN 0x10
#define ENTRIES_EACH_CMD 0x0F
#define SPUR_DATA_INDEX 0x02
#define FM_AF_LIST_MAX_SIZE   0xC8
#define AF_LIST_MAX     (FM_AF_LIST_MAX_SIZE / 4) /* Each AF frequency consist
                            of sizeof(int) bytes */
#define MAX_BLEND_INDEX 0x31

#define FM_SRCH_CONFG_MODE  0x41
#define FM_AFJUMP_CONFG_MODE 0x42
#define FM_SRCH_CNFG_LEN    0x08
#define FM_AFJUMP_CNFG_LEN  0x06
#define STD_BUF_SIZE  256

/* HCI timeouts */
#define RADIO_HCI_TIMEOUT   (10000) /* 10 seconds */

#define ECC_EVENT_BUFSIZE   12
typedef enum {
    ASSOCIATE_JVM,
    DISASSOCIATE_JVM
} bt_cb_thread_evt;

#define TUNE_PARAM 16
#define SIZE_ARRAY(x)  (sizeof(x) / sizeof((x)[0]))
typedef void (*enb_result_cb)(void);
typedef void (*tune_rsp_cb)(int Freq);
typedef void (*seek_rsp_cb)(int Freq);
typedef void (*scan_rsp_cb)(void);
typedef void (*srch_list_rsp_cb)(uint16_t *scan_tbl);
typedef void (*stereo_mode_cb)(bool status);
typedef void (*rds_avl_sts_cb)(bool status);
typedef void (*af_list_cb)(uint16_t *af_list);
typedef void (*rt_cb)(char *rt);
typedef void (*ps_cb)(char *ps);
typedef void (*oda_cb)(void);
typedef void (*rt_plus_cb)(char *rt_plus);
typedef void (*ert_cb)(char *ert);
typedef void (*disable_cb)(void);
typedef void (*callback_thread_event)(unsigned int evt);
typedef void (*rds_grp_cntrs_cb)(char *rds_params);
typedef void (*rds_grp_cntrs_ext_cb)(char *rds_params);
typedef void (*fm_peek_cb)(char *peek_rsp);
typedef void (*fm_ssbi_peek_cb)(char *ssbi_peek_rsp);
typedef void (*fm_agc_gain_cb)(char *agc_gain_rsp);
typedef void (*fm_ch_det_th_cb)(char *ch_det_rsp);
typedef void (*fm_ecc_evt_cb)(char *ecc_rsp);
typedef void (*fm_sig_thr_cb) (int val, int status);
typedef void (*fm_get_ch_det_thrs_cb) (int val, int status);
typedef void (*fm_def_data_rd_cb) (int val, int status);
typedef void (*fm_get_blnd_cb) (int val, int status);
typedef void (*fm_set_ch_det_thrs_cb) (int status);
typedef void (*fm_def_data_wrt_cb) (int status);
typedef void (*fm_set_blnd_cb) (int status);
typedef void (*fm_get_stn_prm_cb) (int val, int status);
typedef void (*fm_get_stn_dbg_prm_cb) (int val, int status);
typedef void (*fm_enable_slimbus_cb) (int status);
typedef void (*fm_enable_softmute_cb) (int status);

typedef struct {
    size_t  size;

    enb_result_cb  enabled_cb;
    tune_rsp_cb tune_cb;
    seek_rsp_cb  seek_cmpl_cb;
    scan_rsp_cb  scan_next_cb;
    srch_list_rsp_cb  srch_list_cb;
    stereo_mode_cb  stereo_status_cb;
    rds_avl_sts_cb  rds_avail_status_cb;
    af_list_cb  af_list_update_cb;
    rt_cb  rt_update_cb;
    ps_cb  ps_update_cb;
    oda_cb  oda_update_cb;
    rt_plus_cb  rt_plus_update_cb;
    ert_cb  ert_update_cb;
    disable_cb  disabled_cb;
    rds_grp_cntrs_cb rds_grp_cntrs_rsp_cb;
    rds_grp_cntrs_ext_cb rds_grp_cntrs_ext_rsp_cb;
    fm_peek_cb fm_peek_rsp_cb;
    fm_ssbi_peek_cb fm_ssbi_peek_rsp_cb;
    fm_agc_gain_cb fm_agc_gain_rsp_cb;
    fm_ch_det_th_cb fm_ch_det_th_rsp_cb;
    fm_ecc_evt_cb  ext_country_code_cb;
    callback_thread_event thread_evt_cb;
    fm_sig_thr_cb fm_get_sig_thres_cb;
    fm_get_ch_det_thrs_cb fm_get_ch_det_thr_cb;
    fm_def_data_rd_cb fm_def_data_read_cb;
    fm_get_blnd_cb fm_get_blend_cb;
    fm_set_ch_det_thrs_cb fm_set_ch_det_thr_cb;
    fm_def_data_wrt_cb fm_def_data_write_cb;
    fm_set_blnd_cb fm_set_blend_cb;
    fm_get_stn_prm_cb fm_get_station_param_cb;
    fm_get_stn_dbg_prm_cb fm_get_station_debug_param_cb;
    fm_enable_slimbus_cb enable_slimbus_cb;
    fm_enable_softmute_cb enable_softmute_cb;
} fm_hal_callbacks_t;

/* Opcode OCF */
/* HCI recv control commands opcode */
#define HCI_OCF_FM_ENABLE_RECV_REQ          0x0001
#define HCI_OCF_FM_DISABLE_RECV_REQ         0x0002
#define HCI_OCF_FM_GET_RECV_CONF_REQ        0x0003
#define HCI_OCF_FM_SET_RECV_CONF_REQ        0x0004
#define HCI_OCF_FM_SET_MUTE_MODE_REQ        0x0005
#define HCI_OCF_FM_SET_STEREO_MODE_REQ      0x0006
#define HCI_OCF_FM_SET_ANTENNA              0x0007
#define HCI_OCF_FM_SET_SIGNAL_THRESHOLD     0x0008
#define HCI_OCF_FM_GET_SIGNAL_THRESHOLD     0x0009
#define HCI_OCF_FM_GET_STATION_PARAM_REQ    0x000A
#define HCI_OCF_FM_GET_PROGRAM_SERVICE_REQ  0x000B
#define HCI_OCF_FM_GET_RADIO_TEXT_REQ       0x000C
#define HCI_OCF_FM_GET_AF_LIST_REQ          0x000D
#define HCI_OCF_FM_SEARCH_STATIONS          0x000E
#define HCI_OCF_FM_SEARCH_RDS_STATIONS      0x000F
#define HCI_OCF_FM_SEARCH_STATIONS_LIST     0x0010
#define HCI_OCF_FM_CANCEL_SEARCH            0x0011
#define HCI_OCF_FM_RDS_GRP                  0x0012
#define HCI_OCF_FM_RDS_GRP_PROCESS          0x0013
#define HCI_OCF_FM_EN_WAN_AVD_CTRL          0x0014
#define HCI_OCF_FM_EN_NOTCH_CTRL            0x0015
#define HCI_OCF_FM_SET_EVENT_MASK           0x0016
#define HCI_OCF_FM_SET_CH_DET_THRESHOLD     0x0017
#define HCI_OCF_FM_GET_CH_DET_THRESHOLD     0x0018
#define HCI_OCF_FM_SET_BLND_TBL             0x001B
#define HCI_OCF_FM_GET_BLND_TBL             0x001C
#define HCI_OCF_FM_LOW_PASS_FILTER_CTRL     0x001F
/* HCI trans control commans opcode*/
#define HCI_OCF_FM_ENABLE_TRANS_REQ         0x0001
#define HCI_OCF_FM_DISABLE_TRANS_REQ        0x0002
#define HCI_OCF_FM_GET_TRANS_CONF_REQ       0x0003
#define HCI_OCF_FM_SET_TRANS_CONF_REQ       0x0004
#define HCI_OCF_FM_RDS_RT_REQ               0x0008
#define HCI_OCF_FM_RDS_PS_REQ               0x0009

#define HCI_OCF_FM_ENABLE_SLIMBUS           (0x000E)

/* HCI common control commands opcode */
#define HCI_OCF_FM_TUNE_STATION_REQ         0x0001
#define HCI_OCF_FM_DEFAULT_DATA_READ        0x0002
#define HCI_OCF_FM_DEFAULT_DATA_WRITE       0x0003
#define HCI_OCF_FM_RESET                    0x0004
#define HCI_OCF_FM_GET_FEATURE_LIST         0x0005
#define HCI_OCF_FM_DO_CALIBRATION           0x0006
#define HCI_OCF_FM_SET_CALIBRATION          0x0007
#define HCI_OCF_FM_SET_SPUR_TABLE           0x0008
#define HCI_OCF_FM_GET_SPUR_TABLE           0x0009

/*HCI Status parameters commands*/
#define HCI_OCF_FM_READ_GRP_COUNTERS        0x0001

#define HCI_OCF_FM_READ_GRP_COUNTERS_EXT    0x0002


/*HCI Diagnostic commands*/
#define HCI_OCF_FM_PEEK_DATA                0x0002
#define HCI_OCF_FM_POKE_DATA                0x0003
#define HCI_OCF_FM_SSBI_PEEK_REG            0x0004
#define HCI_OCF_FM_SSBI_POKE_REG            0x0005
#define HCI_OCF_FM_STATION_DBG_PARAM        0x0007
#define HCI_FM_SET_INTERNAL_TONE_GENRATOR   0x0008
#define HCI_FM_SET_GET_RESET_AGC            0x000D

/* Opcode OGF */
#define HCI_OGF_FM_RECV_CTRL_CMD_REQ            0x0013
#define HCI_OGF_FM_TRANS_CTRL_CMD_REQ           0x0014
#define HCI_OGF_FM_COMMON_CTRL_CMD_REQ          0x0015
#define HCI_OGF_FM_STATUS_PARAMETERS_CMD_REQ    0x0016
#define HCI_OGF_FM_TEST_CMD_REQ                 0x0017
#define HCI_OGF_FM_DIAGNOSTIC_CMD_REQ           0x003F

/* Command opcode pack/unpack */
#define hci_opcode_pack(ogf, ocf)    (uint16_t) (((ocf) & 0x03ff)|((ogf) << 10))
#define hci_opcode_ogf(op)    (op >> 10)
#define hci_opcode_ocf(op)    (op & 0x03ff)
#define hci_recv_ctrl_cmd_op_pack(ocf) \
     (uint16_t) hci_opcode_pack(HCI_OGF_FM_RECV_CTRL_CMD_REQ, ocf)
#define hci_trans_ctrl_cmd_op_pack(ocf) \
     (uint16_t) hci_opcode_pack(HCI_OGF_FM_TRANS_CTRL_CMD_REQ, ocf)
#define hci_common_cmd_op_pack(ocf) \
     (uint16_t) hci_opcode_pack(HCI_OGF_FM_COMMON_CTRL_CMD_REQ, ocf)
#define hci_status_param_op_pack(ocf)   \
     (uint16_t) hci_opcode_pack(HCI_OGF_FM_STATUS_PARAMETERS_CMD_REQ, ocf)
#define hci_diagnostic_cmd_op_pack(ocf) \
     (uint16_t) hci_opcode_pack(HCI_OGF_FM_DIAGNOSTIC_CMD_REQ, ocf)

/* HCI commands with no arguments*/
#define HCI_FM_ENABLE_RECV_CMD 1
#define HCI_FM_DISABLE_RECV_CMD 2
#define HCI_FM_GET_RECV_CONF_CMD 3
#define HCI_FM_GET_STATION_PARAM_CMD 4
#define HCI_FM_GET_SIGNAL_TH_CMD 5
#define HCI_FM_GET_PROGRAM_SERVICE_CMD 6
#define HCI_FM_GET_RADIO_TEXT_CMD 7
#define HCI_FM_GET_AF_LIST_CMD 8
#define HCI_FM_CANCEL_SEARCH_CMD 9
#define HCI_FM_RESET_CMD 10
#define HCI_FM_GET_FEATURES_CMD 11
#define HCI_FM_STATION_DBG_PARAM_CMD 12
#define HCI_FM_ENABLE_TRANS_CMD 13
#define HCI_FM_DISABLE_TRANS_CMD 14
#define HCI_FM_GET_TX_CONFIG 15
#define HCI_FM_GET_DET_CH_TH_CMD 16
#define HCI_FM_GET_BLND_TBL_CMD 17

/* Defines for FM TX*/
#define TX_PS_DATA_LENGTH 108
#define TX_RT_DATA_LENGTH 64
#define PS_STRING_LEN     9

/* ----- HCI Command request ----- */
struct hci_fm_recv_conf_req {
    char  emphasis;
    char  ch_spacing;
    char  rds_std;
    char  hlsi;
    int   band_low_limit;
    int   band_high_limit;
} ;

/* ----- HCI Command request ----- */
struct hci_fm_trans_conf_req_struct {
    char  emphasis;
    char  rds_std;
    int   band_low_limit;
    int   band_high_limit;
} ;

/* ----- HCI Command request ----- */
struct hci_fm_tx_ps {
    char   ps_control;
    short  pi;
    char   pty;
    char   ps_repeatcount;
    char   ps_num;
    char   ps_data[TX_PS_DATA_LENGTH];
} ;

struct hci_fm_tx_rt {
    char    rt_control;
    short   pi;
    char    pty;
    char    rt_len;
    char    rt_data[TX_RT_DATA_LENGTH];
} ;

struct hci_fm_mute_mode_req {
    char  hard_mute;
    char  soft_mute;
} ;

struct hci_fm_stereo_mode_req {
    char    stereo_mode;
    char    sig_blend;
    char    intf_blend;
    char    most_switch;
} ;

struct hci_fm_search_station_req {
    char    srch_mode;
    char    scan_time;
    char    srch_dir;
} ;

struct hci_fm_search_rds_station_req {
    struct hci_fm_search_station_req srch_station;
    char    srch_pty;
    short   srch_pi;
} ;

struct hci_fm_rds_grp_cntrs_params {
  int totalRdsSBlockErrors;
  int totalRdsGroups;
  int totalRdsGroup0;
  int totalRdsGroup2;
  int totalRdsBlockB;
  int totalRdsProcessedGroup0;
  int totalRdsProcessedGroup2;
  int totalRdsGroupFiltered;
  int totalRdsChangeFiltered;
} ;

struct hci_fm_search_station_list_req {
    char    srch_list_mode;
    char    srch_list_dir;
    int   srch_list_max;
    char    srch_pty;
} ;

struct hci_fm_rds_grp_req {
    int   rds_grp_enable_mask;
    int   rds_buf_size;
    char    en_rds_change_filter;
} ;

struct hci_fm_en_avd_ctrl_req {
    char    no_freqs;
    char    freq_index;
    char    lo_shft;
    short   freq_min;
    short   freq_max;
} ;

struct hci_fm_def_data_rd_req {
    char    mode;
    char    length;
    char    param_len;
    char    param;
} ;

struct hci_fm_def_data_wr_req {
    char    mode;
    char    length;
    char   data[DEFAULT_DATA_SIZE];
} ;

struct hci_fm_riva_data {
    char subopcode;
    int   start_addr;
    char    length;
} ;

struct hci_fm_riva_poke {
    struct hci_fm_riva_data cmd_params;
    char    data[MAX_RIVA_PEEK_RSP_SIZE];
} ;

struct hci_fm_ssbi_req {
    short   start_addr;
    char    data;
} ;
struct hci_fm_ssbi_peek {
    short start_address;
} ;

struct hci_fm_set_get_reset_agc {
    char ucctrl;
    char ucgainstate;
} ;

struct hci_fm_ch_det_threshold {
    char sinr;
    char sinr_samples;
    char low_th;
    char high_th;
} ;

struct hci_fm_blend_table {
    char BlendType;
    char BlendRampRateUp;
    char BlendDebounceNumSampleUp;
    char BlendDebounceIdxUp;
    char BlendSinrIdxSkipStep;
    char BlendSinrHi;
    char BlendRmssiHi;
    char BlendIndexHi;
    char BlendIndex[MAX_BLEND_INDEX];
} ;

struct hci_fm_def_data_rd {
    char mode;
    char length;
    char param_len;
    char param;
};

/*HCI events*/
#define HCI_EV_TUNE_STATUS              0x01
#define HCI_EV_RDS_LOCK_STATUS          0x02
#define HCI_EV_STEREO_STATUS            0x03
#define HCI_EV_SERVICE_AVAILABLE        0x04
#define HCI_EV_SEARCH_PROGRESS          0x05
#define HCI_EV_SEARCH_RDS_PROGRESS      0x06
#define HCI_EV_SEARCH_LIST_PROGRESS     0x07
#define HCI_EV_RDS_RX_DATA              0x08
#define HCI_EV_PROGRAM_SERVICE          0x09
#define HCI_EV_RADIO_TEXT               0x0A
#define HCI_EV_FM_AF_LIST               0x0B
#define HCI_EV_TX_RDS_GRP_AVBLE         0x0C
#define HCI_EV_TX_RDS_GRP_COMPL         0x0D
#define HCI_EV_TX_RDS_CONT_GRP_COMPL    0x0E
#define HCI_EV_CMD_COMPLETE             0x0F
#define HCI_EV_CMD_STATUS               0x10
#define HCI_EV_TUNE_COMPLETE            0x11
#define HCI_EV_SEARCH_COMPLETE          0x12
#define HCI_EV_SEARCH_RDS_COMPLETE      0x13
#define HCI_EV_SEARCH_LIST_COMPLETE     0x14

#define HCI_EV_EXT_COUNTRY_CODE         0x17
#define HCI_EV_RADIO_TEXT_PLUS_ID       0x18
#define HCI_EV_RADIO_TEXT_PLUS_TAG      0x19
#define HCI_EV_HW_ERR_EVENT             0x1A

/*HCI event opcode for fm driver RDS support*/
#define HCI_EV_DRIVER_RDS_EVENT         0x1B
#define HCI_EV_E_RADIO_TEXT             0x1C

#define HCI_REQ_DONE      0
#define HCI_REQ_PEND      1
#define HCI_REQ_CANCELED  2
#define HCI_REQ_STATUS    3

#define MAX_RAW_RDS_GRPS     21

#define RDSGRP_DATA_OFFSET   0x1

/*RT PLUS*/
#define DUMMY_CLASS             0
#define RT_PLUS_LEN_1_TAG       3
#define RT_ERT_FLAG_BIT         5

/*TAG1*/
#define TAG1_MSB_OFFSET         3
#define TAG1_MSB_MASK           7
#define TAG1_LSB_OFFSET         5
#define TAG1_POS_MSB_MASK       31
#define TAG1_POS_MSB_OFFSET     1
#define TAG1_POS_LSB_OFFSET     7
#define TAG1_LEN_OFFSET         1
#define TAG1_LEN_MASK           63

/*TAG2*/
#define TAG2_MSB_OFFSET         5
#define TAG2_MSB_MASK           1
#define TAG2_LSB_OFFSET         3
#define TAG2_POS_MSB_MASK       7
#define TAG2_POS_MSB_OFFSET     3
#define TAG2_POS_LSB_OFFSET     5
#define TAG2_LEN_MASK           31

#define AGT_MASK                31
/*Extract 5 left most bits of lsb of 2nd block*/
#define AGT(x)               (x & AGT_MASK)
/*16 bits of 4th block*/
#define AID(lsb, msb)        ((msb << 8) | (lsb))
/*Extract 5 right most bits of msb of 2nd block*/
#define GTC(blk2msb)         (blk2msb >> 3)

#define GRP_3A               0x6
#define RT_PLUS_AID          0x4bd7

/*ERT*/
#define ERT_AID              0x6552
#define CARRIAGE_RETURN      0x000D
#define MAX_ERT_SEGMENT      31
#define ERT_FORMAT_DIR_BIT   1

#define EXTRACT_BIT(data, bit_pos) ((data & (1 << bit_pos)) >> bit_pos)

struct hci_ev_tune_status {
    char    sub_event;
    int     station_freq;
    char    serv_avble;
    char    rssi;
    char    stereo_prg;
    char    rds_sync_status;
    char    mute_mode;
    char    sinr;
    char    intf_det_th;
}__attribute__((packed)) ;

struct rds_blk_data {
    char  rdsMsb;
    char  rdsLsb;
    char  blockStatus;
} ;

struct rds_grp_data {
    struct rds_blk_data rdsBlk[4];
} ;

struct hci_ev_rds_rx_data {
    char    num_rds_grps;
    struct  rds_grp_data rds_grp_data[MAX_RAW_RDS_GRPS];
} ;

struct hci_ev_prg_service {
    short   pi_prg_id;
    char    pty_prg_type;
    char    ta_prg_code_type;
    char    ta_ann_code_flag;
    char    ms_switch_code_flag;
    char    dec_id_ctrl_code_flag;
    char    ps_num;
    char    prg_service_name[119];
} ;

struct hci_ev_radio_text {
    short   pi_prg_id;
    char    pty_prg_type;
    char    ta_prg_code_type;
    char    txt_ab_flag;
    char    radio_txt[64];
} ;

struct hci_ev_af_list {
    int   tune_freq;
    short   pi_code;
    uint8_t af_size;
    char    af_list[FM_AF_LIST_MAX_SIZE];
} __attribute__((packed)) ;

struct hci_ev_cmd_complete {
    char    num_hci_cmd_pkts;
    short   cmd_opcode;
} __attribute((packed));

struct hci_ev_cmd_status {
    char    status;
    char    num_hci_cmd_pkts;
    short   status_opcode;
} __attribute__((packed));

struct hci_ev_srch_st {
    int    station_freq;
    char    rds_cap;
    char   pty;
    short   status_opcode;
} __attribute__((packed));

struct hci_ev_rel_freq {
    char  rel_freq_msb;
    char  rel_freq_lsb;

} ;
struct hci_ev_srch_list_compl {
    char    num_stations_found;
    struct hci_ev_rel_freq  rel_freq[20];
} ;

/* ----- HCI Event Response ----- */
struct hci_fm_conf_rsp {
    char    status;
    struct hci_fm_recv_conf_req recv_conf_rsp;
} __attribute__((packed));

struct hci_fm_rds_grp_cntrs_rsp {
    char    status;
    struct hci_fm_rds_grp_cntrs_params recv_rds_grp_cntrs_rsp;
} __attribute__((packed));


struct hci_fm_get_trans_conf_rsp {
    char    status;
    struct hci_fm_trans_conf_req_struct trans_conf_rsp;
} __attribute__((packed));

struct hci_fm_sig_threshold_rsp {
    char    status;
    char    sig_threshold;
} ;

struct hci_fm_station_rsp {
    struct hci_ev_tune_status station_rsp;
} ;

struct hci_fm_prgm_srv_rsp {
    char    status;
    struct hci_ev_prg_service prg_srv;
} __attribute__((packed));

struct hci_fm_radio_txt_rsp {
    char    status;
    struct hci_ev_radio_text rd_txt;
} __attribute__((packed));

struct hci_fm_af_list_rsp {
    char    status;
    struct hci_ev_af_list rd_txt;
} __attribute__((packed));

struct hci_fm_data_rd_rsp {
    uint8_t data_len;
    char    data[DEFAULT_DATA_SIZE];
} ;

struct hci_fm_feature_list_rsp {
    char    status;
    char    feature_mask;
} ;

struct hci_fm_dbg_param_rsp {
    char    blend;
    char    soft_mute;
    char    inf_blend;
    char    inf_soft_mute;
    char    pilot_pil;
    char    io_verc;
    char    in_det_out;
} ;

#define CLKSPURID_INDEX0    0
#define CLKSPURID_INDEX1    5
#define CLKSPURID_INDEX2    10
#define CLKSPURID_INDEX3    15
#define CLKSPURID_INDEX4    20
#define CLKSPURID_INDEX5    25

#define MAX_SPUR_FREQ_LIMIT 30
#define CKK_SPUR        0x3B
#define SPUR_DATA_SIZE      0x4
#define SPUR_ENTRIES_PER_ID 0x5

#define COMPUTE_SPUR(val)         ((((val) - (76000)) / (50)))
#define GET_FREQ(val, bit)        ((bit == 1) ? ((val) >> 8) : ((val) & 0xFF))
#define GET_SPUR_ENTRY_LEVEL(val) ((val) / (5))

struct hci_fm_spur_data {
    int   freq[MAX_SPUR_FREQ_LIMIT];
    char  rmssi[MAX_SPUR_FREQ_LIMIT];
    char  enable[MAX_SPUR_FREQ_LIMIT];
} __attribute__((packed));

/* HCI dev events */
#define RADIO_HCI_DEV_REG           1
#define RADIO_HCI_DEV_WRITE         2

#define hci_req_lock(d)     mutex_lock(&d->req_lock)
#define hci_req_unlock(d)   mutex_unlock(&d->req_lock)

/* FM RDS */
#define RDS_PTYPE 2
#define RDS_PID_LOWER 1
#define RDS_PID_HIGHER 0
#define RDS_OFFSET 5
#define RDS_PS_LENGTH_OFFSET 7
#define RDS_STRING 8
#define RDS_PS_DATA_OFFSET 8
#define RDS_CONFIG_OFFSET  3
#define RDS_AF_JUMP_OFFSET 4
#define PI_CODE_OFFSET 4
#define AF_SIZE_OFFSET 6
#define AF_LIST_OFFSET 7
#define RT_A_B_FLAG_OFFSET 4
#define RDS_PS_NUMBER_MIN 1
#define RDS_PS_NUMBER_MAX 12
/*FM states*/

enum radio_state_t {
    FM_OFF,
    FM_RECV,
    FM_TRANS,
    FM_RESET,
    FM_CALIB,
    FM_TURNING_OFF,
    FM_RECV_TURNING_ON,
    FM_TRANS_TURNING_ON,
    FM_MAX_NO_STATES,
};

enum emphasis_type {
    FM_RX_EMP75 = 0x0,
    FM_RX_EMP50 = 0x1
};

enum channel_space_type {
    FM_RX_SPACE_200KHZ = 0x0,
    FM_RX_SPACE_100KHZ = 0x1,
    FM_RX_SPACE_50KHZ = 0x2
};

enum high_low_injection {
    AUTO_HI_LO_INJECTION = 0x0,
    LOW_SIDE_INJECTION = 0x1,
    HIGH_SIDE_INJECTION = 0x2
};

enum fm_rds_type {
    FM_RX_RDBS_SYSTEM = 0x0,
    FM_RX_RDS_SYSTEM = 0x1
};

enum hlm_region_t {
    HELIUM_REGION_US,
    HELIUM_REGION_EU,
    HELIUM_REGION_JAPAN,
    HELIUM_REGION_JAPAN_WIDE,
    HELIUM_REGION_OTHER
};

/* Search options */
enum search_t {
    SEEK,
    SCAN,
    SCAN_FOR_STRONG,
    SCAN_FOR_WEAK,
    RDS_SEEK_PTY,
    RDS_SCAN_PTY,
    RDS_SEEK_PI,
    RDS_AF_JUMP,
};

enum spur_entry_levels {
    ENTRY_0,
    ENTRY_1,
    ENTRY_2,
    ENTRY_3,
    ENTRY_4,
    ENTRY_5,
};

/* Band limits */
#define REGION_US_EU_BAND_LOW              87500
#define REGION_US_EU_BAND_HIGH             108000
#define REGION_JAPAN_STANDARD_BAND_LOW     76000
#define REGION_JAPAN_STANDARD_BAND_HIGH    90000
#define REGION_JAPAN_WIDE_BAND_LOW         90000
#define REGION_JAPAN_WIDE_BAND_HIGH        108000

#define SRCH_MODE       0x07
#define SRCH_DIR        0x08 /* 0-up 1-down */
#define SCAN_DWELL      0x70
#define SRCH_ON         0x80

/* I/O Control */
#define IOC_HRD_MUTE    0x03
#define IOC_SFT_MUTE    0x01
#define IOC_MON_STR     0x01
#define IOC_SIG_BLND    0x01
#define IOC_INTF_BLND   0x01
#define IOC_ANTENNA     0x01

/* RDS Control */
#define RDS_ON      0x01
#define RDS_BUF_SZ  100

/* constants */
#define  RDS_BLOCKS_NUM (4)
#define BYTES_PER_BLOCK (3)
#define MAX_PS_LENGTH   (108)
#define MAX_RT_LENGTH   (64)
#define RDS_GRP_CNTR_LEN (36)
#define RX_RT_DATA_LENGTH (63)
/* Search direction */
#define SRCH_DIR_UP      (0)
#define SRCH_DIR_DOWN    (1)

/*Search RDS stations*/
#define SEARCH_RDS_STNS_MODE_OFFSET 4

/*Search Station list */
#define PARAMS_PER_STATION 0x08
#define STN_NUM_OFFSET     0x01
#define STN_FREQ_OFFSET    0x02
#define KHZ_TO_MHZ         1000
#define GET_MSB(x)((x >> 8) & 0xFF)
#define GET_LSB(x)((x) & 0xFF)

/* control options */
#define CTRL_ON     (1)
#define CTRL_OFF    (0)

/*Diagnostic commands*/

#define RIVA_PEEK_OPCODE 0x0D
#define RIVA_POKE_OPCODE 0x0C

#define PEEK_DATA_OFSET 0x1
#define RIVA_PEEK_PARAM     0x6
#define RIVA_PEEK_LEN_OFSET  0x6
#define SSBI_PEEK_LEN    0x01
/*Calibration data*/
#define PROCS_CALIB_MODE  1
#define PROCS_CALIB_SIZE  23
#define DC_CALIB_MODE     2
#define DC_CALIB_SIZE     48
#define RSB_CALIB_MODE    3
#define RSB_CALIB_SIZE    4
#define CALIB_DATA_OFSET  2
#define CALIB_MODE_OFSET  1
#define MAX_CALIB_SIZE 75

/* Channel validity */
#define INVALID_CHANNEL     (0)
#define VALID_CHANNEL       (1)

struct hci_fm_set_cal_req_proc {
    char    mode;
    /*Max process calibration data size*/
    char    data[PROCS_CALIB_SIZE];
} ;

struct hci_fm_set_cal_req_dc {
    char    mode;
    /*Max DC calibration data size*/
    char    data[DC_CALIB_SIZE];
} ;

struct hci_cc_do_calibration_rsp {
    char status;
    char mode;
    char data[MAX_CALIB_SIZE];
} ;

struct hci_fm_set_spur_table_req {
    char mode;
    char no_of_freqs_entries;
    char spur_data[FM_SPUR_TBL_SIZE];
} ;
/* Low Power mode*/
#define SIG_LEVEL_INTR  (1 << 0)
#define RDS_SYNC_INTR   (1 << 1)
#define AUDIO_CTRL_INTR (1 << 2)
#define AF_JUMP_ENABLE  (1 << 4)

static inline int is_valid_tone(int tone)
{
    if ((tone >= MIN_TX_TONE_VAL) &&
        (tone <= MAX_TX_TONE_VAL))
        return 1;
    else
        return 0;
}

static inline int is_valid_hard_mute(int hard_mute)
{
    if ((hard_mute >= MIN_HARD_MUTE_VAL) &&
        (hard_mute <= MAX_HARD_MUTE_VAL))
        return 1;
    else
        return 0;
}

static inline int is_valid_srch_mode(int srch_mode)
{
    if ((srch_mode >= MIN_SRCH_MODE) &&
        (srch_mode <= MAX_SRCH_MODE))
        return 1;
    else
        return 0;
}

static inline int is_valid_scan_dwell_prd(int scan_dwell_prd)
{
    if ((scan_dwell_prd >= MIN_SCAN_DWELL) &&
        (scan_dwell_prd <= MAX_SCAN_DWELL))
        return 1;
    else
        return 0;
}

static inline int is_valid_sig_th(int sig_th)
{
    if ((sig_th >= MIN_SIG_TH) &&
        (sig_th <= MAX_SIG_TH))
        return 1;
    else
        return 0;
}

static inline int is_valid_pty(int pty)
{
    if ((pty >= MIN_PTY) &&
        (pty <= MAX_PTY))
        return 1;
    else
        return 0;
}

static inline int is_valid_pi(int pi)
{
    if ((pi >= MIN_PI) &&
        (pi <= MAX_PI))
        return 1;
    else
        return 0;
}

static inline int is_valid_srch_station_cnt(int cnt)
{
    if ((cnt >= MIN_SRCH_STATIONS_CNT) &&
        (cnt <= MAX_SRCH_STATIONS_CNT))
        return 1;
    else
        return 0;
}

static inline int is_valid_chan_spacing(int spacing)
{
    if ((spacing >= MIN_CHAN_SPACING) &&
        (spacing <= MAX_CHAN_SPACING))
        return 1;
    else
        return 0;
}

static inline int is_valid_emphasis(int emphasis)
{
    if ((emphasis >= MIN_EMPHASIS) &&
        (emphasis <= MAX_EMPHASIS))
        return 1;
    else
        return 0;
}

static inline int is_valid_rds_std(int rds_std)
{
    if ((rds_std >= MIN_RDS_STD) &&
        (rds_std <= MAX_RDS_STD))
        return 1;
    else
        return 0;
}

static inline int is_valid_antenna(int antenna_type)
{
    if ((antenna_type >= MIN_ANTENNA_VAL) &&
        (antenna_type <= MAX_ANTENNA_VAL))
        return 1;
    else
        return 0;
}

static inline int is_valid_ps_repeat_cnt(int cnt)
{
    if ((cnt >= MIN_TX_PS_REPEAT_CNT) &&
        (cnt <= MAX_TX_PS_REPEAT_CNT))
        return 1;
    else
        return 0;
}

static inline int is_valid_soft_mute(int soft_mute)
{
    if ((soft_mute >= MIN_SOFT_MUTE) &&
        (soft_mute <= MAX_SOFT_MUTE))
        return 1;
    else
        return 0;
}

static inline int is_valid_peek_len(int len)
{
    if ((len >= MIN_PEEK_ACCESS_LEN) &&
        (len <= MAX_PEEK_ACCESS_LEN))
        return 1;
    else
        return 0;
}

static inline int is_valid_reset_cntr(int cntr)
{
    if ((cntr >= MIN_RESET_CNTR) &&
        (cntr <= MAX_RESET_CNTR))
        return 1;
    else
        return 0;
}

static inline int is_valid_hlsi(int hlsi)
{
    if ((hlsi >= MIN_HLSI) &&
        (hlsi <= MAX_HLSI))
        return 1;
    else
        return 0;
}

static inline int is_valid_notch_filter(int filter)
{
    if ((filter >= MIN_NOTCH_FILTER) &&
        (filter <= MAX_NOTCH_FILTER))
        return 1;
    else
        return 0;
}

static inline int is_valid_intf_det_low_th(int th)
{
    if ((th >= MIN_INTF_DET_OUT_LW_TH) &&
        (th <= MAX_INTF_DET_OUT_LW_TH))
        return 1;
    else
        return 0;
}

static inline int is_valid_intf_det_hgh_th(int th)
{
    if ((th >= MIN_INTF_DET_OUT_HG_TH) &&
        (th <= MAX_INTF_DET_OUT_HG_TH))
        return 1;
    else
        return 0;
}

static inline int is_valid_sinr_th(int th)
{
    if ((th >= MIN_SINR_TH) &&
        (th <= MAX_SINR_TH))
        return 1;
    else
        return 0;
}

static inline int is_valid_sinr_samples(int samples_cnt)
{
    if ((samples_cnt >= MIN_SINR_SAMPLES) &&
        (samples_cnt <= MAX_SINR_SAMPLES))
        return 1;
    else
        return 0;
}

static inline int is_valid_fm_state(int state)
{
    if ((state >= 0) && (state < FM_MAX_NO_STATES))
        return 1;
    else
        return 0;
}

static inline int is_valid_blend_value(int val)
{
    if ((val >= MIN_BLEND_HI) && (val <= MAX_BLEND_HI))
        return 1;
    else
        return 0;
}

struct radio_helium_device {
    int tune_req;
    unsigned int mode;
    short pi;
    char pty;
    char ps_repeatcount;
    char prev_trans_rds;
    char af_jump_bit;
    struct hci_fm_mute_mode_req mute_mode;
    struct hci_fm_stereo_mode_req stereo_mode;
    struct hci_fm_station_rsp fm_st_rsp;
    struct hci_fm_search_station_req srch_st;
    struct hci_fm_search_rds_station_req srch_rds;
    struct hci_fm_search_station_list_req srch_st_list;
    struct hci_fm_recv_conf_req recv_conf;
    struct hci_fm_trans_conf_req_struct trans_conf;
    struct hci_fm_rds_grp_req rds_grp;
    unsigned char g_search_mode;
    unsigned char power_mode;
    int search_on;
    unsigned char spur_table_size;
    unsigned char g_scan_time;
    unsigned int g_antenna;
    unsigned int g_rds_grp_proc_ps;
    unsigned char event_mask;
    enum hlm_region_t region;
    struct hci_fm_dbg_param_rsp st_dbg_param;
    struct hci_ev_srch_list_compl srch_st_result;
    struct hci_fm_riva_poke   riva_data_req;
    struct hci_fm_ssbi_req    ssbi_data_accs;
    struct hci_fm_ssbi_peek   ssbi_peek_reg;
    struct hci_fm_set_get_reset_agc set_get_reset_agc;
    struct hci_fm_ch_det_threshold ch_det_threshold;
    struct hci_fm_data_rd_rsp def_data;
    struct hci_fm_blend_table blend_tbl;
} __attribute__((packed));

#define set_bit(flag, bit_pos)      ((flag) |= (1 << (bit_pos)))
#define clear_bit(flag, bit_pos)    ((flag) &= (~(1 << (bit_pos))))
#define test_bit(flag, bit_pos)     ((flag) & (1 << (bit_pos)))
#define clear_all_bit(flag)         ((flag) &= (~0xFFFFFFFF))
#define CMD_CHDET_SINR_TH           (1)
#define CMD_CHDET_SINR_SAMPLE       (2)
#define CMD_CHDET_INTF_TH_LOW       (3)
#define CMD_CHDET_INTF_TH_HIGH      (4)

#define CMD_DEFRD_AF_RMSSI_TH       (1)
#define CMD_DEFRD_AF_RMSSI_SAMPLE   (2)
#define CMD_DEFRD_GD_CH_RMSSI_TH    (3)
#define CMD_DEFRD_SEARCH_ALGO       (4)
#define CMD_DEFRD_SINR_FIRST_STAGE  (5)
#define CMD_DEFRD_RMSSI_FIRST_STAGE (6)
#define CMD_DEFRD_CF0TH12           (7)
#define CMD_DEFRD_TUNE_POWER        (8)
#define CMD_DEFRD_REPEATCOUNT       (9)
#define CMD_DEFRD_AF_ALGO           (10)
#define CMD_DEFRD_AF_SINR_GD_CH_TH  (11)
#define CMD_DEFRD_AF_SINR_TH        (12)

#define CMD_STNPARAM_RSSI           (1)
#define CMD_STNPARAM_SINR           (2)
#define CMD_STNPARAM_INTF_DET_TH    (3)

#define CMD_STNDBGPARAM_BLEND       (1)
#define CMD_STNDBGPARAM_SOFTMUTE    (2)
#define CMD_STNDBGPARAM_INFBLEND    (3)
#define CMD_STNDBGPARAM_INFSOFTMUTE (4)
#define CMD_STNDBGPARAM_PILOTPLL    (5)
#define CMD_STNDBGPARAM_IOVERC      (6)
#define CMD_STNDBGPARAM_INFDETOUT   (7)

#define CMD_BLENDTBL_SINR_HI        (1)
#define CMD_BLENDTBL_RMSSI_HI       (2)

int hci_fm_disable_recv_req();
int helium_search_list(struct hci_fm_search_station_list_req *s_list);
int helium_search_rds_stations(struct hci_fm_search_rds_station_req *rds_srch);
int helium_search_stations(struct hci_fm_search_station_req *srch);
int helium_cancel_search_req();
int hci_fm_set_recv_conf_req (struct hci_fm_recv_conf_req *conf);
int hci_fm_get_program_service_req ();
int hci_fm_get_rds_grpcounters_req (int val);
int hci_fm_get_rds_grpcounters_ext_req (int val);
int hci_fm_set_notch_filter_req (int val);
int helium_set_sig_threshold_req(char th);
int helium_rds_grp_mask_req(struct hci_fm_rds_grp_req *rds_grp_msk);
int helium_rds_grp_process_req(int rds_grp);
int helium_set_event_mask_req(char e_mask);
int helium_set_antenna_req(char ant);
int helium_set_fm_mute_mode_req(struct hci_fm_mute_mode_req *mute);
int hci_fm_tune_station_req(int param);
int hci_set_fm_stereo_mode_req(struct hci_fm_stereo_mode_req *param);
int hci_peek_data(struct hci_fm_riva_data *data);
int hci_poke_data(struct hci_fm_riva_poke *data);
int hci_ssbi_poke_reg(struct hci_fm_ssbi_req *data);
int hci_ssbi_peek_reg(struct hci_fm_ssbi_peek *data);
int hci_get_set_reset_agc_req(struct hci_fm_set_get_reset_agc *data);
int hci_fm_get_ch_det_th();
int set_ch_det_thresholds_req(struct hci_fm_ch_det_threshold *ch_det_th);
int hci_fm_default_data_read_req(struct hci_fm_def_data_rd_req *def_data_rd);
int hci_fm_get_blend_req();
int hci_fm_set_blend_tbl_req(struct hci_fm_blend_table *blnd_tbl);
int hci_fm_enable_lpf(int enable);
int hci_fm_default_data_write_req(struct hci_fm_def_data_wr_req * data_wrt);
int hci_fm_get_station_dbg_param_req();
int hci_fm_get_station_cmd_param_req();
int hci_fm_enable_slimbus(uint8_t enable);
int hci_fm_enable_softmute(uint8_t enable);

struct fm_hal_t {
    struct radio_helium_device *radio;
    const fm_hal_callbacks_t *jni_cb;
    void *private_data;
    pthread_mutex_t cmd_lock;
    pthread_cond_t cmd_cond;
    bool set_cmd_sent;
};

struct fm_interface_t {
    int (*hal_init)(const fm_hal_callbacks_t *p_cb);
    int (*set_fm_ctrl)(int opcode, int val);
    int (*get_fm_ctrl) (int opcode, int *val);
};

#endif /* __UAPI_RADIO_HCI_CORE_H */

