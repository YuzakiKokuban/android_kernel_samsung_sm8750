#ifndef __UAPI_CAM_SEC_OIS_H__
#define __UAPI_CAM_SEC_OIS_H__

#include <linux/types.h>
#include <linux/ioctl.h>

#define MAX_OIS_CUSTOM_PAYLOAD_SIZE (32)
#define MAX_OIS_CUSTOM_CALDATA_PAYLOAD_SIZE (64)

enum ois_payload_type {
	// Custom Data Set Payload
	OIS_CMD,
	OIS_STATUS_MODE,
	OIS_DEBUG_MODE,
	OIS_CALDATA,
	OIS_FACTORY_CMD,
	// Get Status Payload
	OIS_STATIC_INFO,
	OIS_DYNAMIC_INFO,
	OIS_REGISTER_INFO,
	OIS_FACTORY_RESULT,
	OIS_PAYLOAD_MAX,
};

enum ois_factory_cmd_type {
	OIS_FCT_OIS_AUTOTEST,
	OIS_FCT_GYRO_NOISE_STDEV,
	OIS_FCT_GYRO_SELFTEST,
	OIS_FCT_GYRO_CALIBRATION,
	OIS_FCT_OIS_VALID_CHECK,
	OIS_FCT_OIS_CENTER_SHIFT,
	OIS_FCT_OIS_HALL_POSITION,
	OIS_FCT_OIS_HALL_CAL_CHK,
	OIS_FCT_OIS_CROSS_TALK,
	OIS_FCT_OIS_GET_MGLESS,
	OIS_FCT_OIS_RESET_CHECK,
	OIS_FCT_OIS_GET_FW,
	OIS_FCT_OIS_GET_EXIF, // Don't need
	OIS_FCT_OIS_EXT_CLK, // Don't need
	OIS_FCT_OIS_SET_MODE,
	OIS_FCT_RUMBA_FW_UPDATE,
	OIS_FCT_MAX,

	OIS_FCT_FORCE_KP,
	OIS_FCT_OIS_ERROR,
};

struct ois_custom_cmd_t {
	__u8 payload_type;
	__u8 ois_mode;
	__u8 ois_ctrl;
	__u8 ois_sel;
	__u8 ggfade;
	__u8 reserved[27];
} __attribute__((packed));

struct ois_custom_status_t {
	__u8 payload_type;
	__u8 status_mode;
	__u8 access_mode;
	__u8 data_type;
	__u8 total_size;
	__u16 reg_addr;
	__u8 reserved[25];
} __attribute__((packed));

struct ois_custom_register_info_t {
	__u8 payload_type;
	__u8 access_mode;
	__u8 data_type;
	__u8 total_size;
	__u16 reg_addr;
	__u8 reg_data[26];
} __attribute__((packed));

struct ois_custom_debug_mode_t {
	__u8 payload_type;
	__u8 data[31];
} __attribute__((packed));

struct ois_custom_static_info_t {
	__u8 payload_type;
	__u8 algo_version[8];
	__u8 act_pid_rev[3];
	__u32 act_prod_id[3];
	__u8 act_core_info[3];
	__u16 rumba_fw_ver;
	__u8 reserved[3];
} __attribute__((packed));

struct ois_custom_dynamic_info_t {
	__u8 payload_type;
	__u8 ois_status;
	__u16 ois_error;
	__u8 ois_mode;
	__u8 ois_outsel;
	__u8 ggfade_gain;
	__u8 ois_ctrl;
	__u8 tripod_mode;
	__u8 test_id;
	__u16 hall_xy[6];
	__u16 ext_info[5];
} __attribute__((packed));

struct ois_custom_caldata_t {
	__u8 payload_type;
	__u8 hwid;
	__u8 hwid_sub[3];
	__u32 xgg_m1;
	__u32 ygg_m1;
	__u32 xgg_m2;
	__u32 ygg_m2;
	__u32 xgg_m3;
	__u32 ygg_m3;
	__s16 xg_zero;
	__s16 yg_zero;
	__s16 zg_zero;
	__s16 xcoffset_m3;
	__s16 ycoffset_m3;
	__u8 reserved[25];
} __attribute__((packed));

struct ois_fct_general_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 reserved[30];
} __attribute__((packed));

struct ois_fct_autotest_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 threshold;
	__u8 frequency;
	__u8 amplitude;
	__u8 moduleMask;
	__u8 reserved[26];
} __attribute__((packed));

struct ois_fct_selftest_t {
	__u8 payload_type;
	__u8 test_id;
	__s16 gyro_zero_offset_x;
	__s16 gyro_zero_offset_y;
	__s16 gyro_zero_offset_z;
	__u8 need_cal_flag;
	__u8 reserved[23];
} __attribute__((packed));

struct ois_fct_center_shift_t {
	__u8 payload_type;
	__u8 test_id;
	__s16 xcoffset_m1;
	__s16 ycoffset_m1;
	__s16 xcoffset_m2;
	__s16 ycoffset_m2;
	__s16 xcoffset_m3;
	__s16 ycoffset_m3;
	__u8 reserved[18];
} __attribute__((packed));

struct ois_fct_hall_cal_chk_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 module_id;
	__u8 reserved[29];
} __attribute__((packed));

struct ois_fct_autotest_result_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 test_results[3]; // BIT: MGLESS,Y2,Y1,X2,X1
	__u8 error_counts[9]; // X,Y1,Y2
	__u16 max_diffs[9]; // X,Y1,Y2
} __attribute__((packed));

struct ois_fct_noise_stdev_result_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 test_result;
	__s16 gyro_noise_stdev_x;
	__s16 gyro_noise_stdev_y;
	__u8 reserved[25];
} __attribute__((packed));

struct ois_fct_selftest_result_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 total_result;
	__u8 offset_test_result;
	__u8 offset_test_oiserr;
	__u8 selftest_result;
	__u8 selftest_oiserr;
	__s16 gyro_zero_offset_x;
	__s16 gyro_zero_offset_y;
	__s16 gyro_zero_offset_z;
	__u8 reserved[19];
} __attribute__((packed));

struct ois_fct_calibration_result_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 test_result;
	__u8 offset_test_oiserr;
	__s16 gyro_zero_offset_x;
	__s16 gyro_zero_offset_y;
	__s16 gyro_zero_offset_z;
	__u8 reserved[22];
} __attribute__((packed));

struct ois_fct_valid_check_result_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 test_result_m1;
	__u8 test_result_m2;
	__u8 test_result_m3;
	__u8 comm_check_oiserr;
	__u8 reserved[26];
} __attribute__((packed));

struct ois_fct_center_shift_result_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 test_result;
	__s16 xcoffset_m1;
	__s16 ycoffset_m1;
	__s16 xcoffset_m2;
	__s16 ycoffset_m2;
	__s16 xcoffset_m3;
	__s16 ycoffset_m3;
	__u8 reserved[17];
} __attribute__((packed));

struct ois_fct_hall_position_result_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 test_result;
	__u8 hall_pos_oiserr;
	__u16 target_pos_x_m1;
	__u16 target_pos_y_m1;
	__u16 target_pos_x_m2;
	__u16 target_pos_y_m2;
	__u16 target_pos_x_m3;
	__u16 target_pos_y_m3;
	__u16 hall_pos_x_m1;
	__u16 hall_pos_y_m1;
	__u16 hall_pos_x_m2;
	__u16 hall_pos_y_m2;
	__u16 hall_pos_x_m3;
	__u16 hall_pos_y_m3;
	__u8 reserved[4];
} __attribute__((packed));

struct ois_fct_hall_cal_chk_result_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 test_result;
	__u8 act_slave_addr_x;
	__u8 act_slave_addr_y;
	__u8 stored_cal_mask;
	__u8 stored_af_best_pos;
	__u16 ideal_pcal_x;
	__u16 ideal_ncal_x;
	__u16 ideal_pcal_y;
	__u16 ideal_ncal_y;
	__u16 curr_pcal_x;
	__u16 curr_ncal_x;
	__u16 curr_pcal_y;
	__u16 curr_ncal_y;
	__u8 reserved[9];
} __attribute__((packed));

struct ois_fct_ois_set_mode_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 ois_mode;
	__u8 reserved[29];
} __attribute__((packed));

struct ois_fct_cross_talk_result_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 test_result;
	__u8 rumba_slave_addr;
	__u8 step_count_x;
	__u16 step_value_x;
	__u16 init_target_x;
	__u16 init_target_y;
	__u16 read_hall_step[10];
	__u8 reserved[1];
} __attribute__((packed));

struct ois_fct_mgless_result_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 test_result;
	__u8 mgless_flag;
	__u8 reserved[28];
} __attribute__((packed));

struct ois_fct_reset_check_result_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 test_result;
	__u16 ois_err;
	__u8 ois_status;
	__u8 ois_ctrl;
	__u8 ois_mode;
	__u8 ois_sel;
	__u8 reserved[23];
} __attribute__((packed));

struct ois_fct_fw_ver_result_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 version[8];
	__u8 reserved[22];
} __attribute__((packed));

struct ois_fct_exif_result_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 version[8];
	__u16 ois_err;
	__u8 ois_status;
	__u8 ois_ctrl;
	__u8 ois_mode;
	__u8 ois_sel;
	__u8 reserved[16];
} __attribute__((packed));

struct ois_fct_rumba_fw_update_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 force_update;
	__u8 reserved[29];
} __attribute__((packed));

struct ois_fct_rumba_fw_update_result_t {
	__u8 payload_type;
	__u8 test_id;
	__u8 test_result;
	__u8 vendor_code;
	__u32 phone_bin_ver;
	__u32 module_bin_ver;
	__u8 reserved[20];
} __attribute__((packed));

struct ois_fct_hwbigdata_t {
	__u8 payload_type;
	__u8 test_id;
	__u16 ois_error;
	__u8 reserved[28];
} __attribute__((packed));
#endif

