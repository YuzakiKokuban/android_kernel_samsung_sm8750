/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#ifndef _CAM_SEC_EEPROM_CORE_H_
#define _CAM_SEC_EEPROM_CORE_H_

#include "cam_eeprom_dev.h"

enum fw_ver_index {
	EEPROM_FW_VER = 1,
	PHONE_FW_VER,
	LOAD_FW_VER
};

enum sysfs_index {
	INDEX_REAR,
	INDEX_REAR2,
	INDEX_REAR3,
	INDEX_REAR4,
	INDEX_FRONT,
	INDEX_FRONT2,
	INDEX_FRONT3,
	INDEX_MAX,
};

#define OK 1
#define CRASH 0

#define SYSFS_FW_VER_SIZE                       40
#define SYSFS_MODULE_INFO_SIZE                  96

#define FROM_MODULE_FW_INFO_SIZE                11
#define FROM_MTF_SIZE                           54
#define FROM_MODULE_ID_SIZE                     10

#define FROM_REAR_AF_CAL_SIZE                   10
#define FROM_SENSOR_ID_SIZE                     16

#define FROM_REAR_DUAL_CAL_SIZE                 89
#define FROM_FRONT_DUAL_CAL_SIZE                1024
#define FROM_MAX_DUAL_CAL_SIZE                  ((FROM_REAR_DUAL_CAL_SIZE > FROM_FRONT_DUAL_CAL_SIZE) ? FROM_REAR_DUAL_CAL_SIZE : FROM_FRONT_DUAL_CAL_SIZE)

#define PAF_2PD_CAL_INFO_SIZE                   4096
#define PAF_SPARSEPD_CAL_INFO_SIZE              2048
#define PAF_CAL_ERR_CHECK_OFFSET                0x14

#define CAMERA_CAL_CRC_WIDE                     0x1FFF
#define FROM_REAR_HEADER_SIZE                   0x0200

#define HW_INFO_MAX_SIZE                        (6)
#define SW_INFO_MAX_SIZE                        (5)
#define VENDOR_INFO_MAX_SIZE                    (2)
#define PROCESS_INFO_MAX_SIZE                   (2)
#define CHK_VER_INFO_MAX_SIZE                   (4)
#define PROJECT_CAL_TYPE_MAX_SIZE               (20)

#define MAKE_STRINGIZE(arg) #arg

#define X_ENUMS                \
	X(DEF_M_CORE_VER)          \
	X(DEF_M_VER_HW)            \
	X(DEF_M_VER_SW)            \
	X(DEF_M_VER_ETC)           \
	X(DEF_S_VER_HW)            \
	X(DEF_M_CHK_VER)           \
	X(SIZE_M_PAF_CAL)          \
	X(SIZE_S_PAF_CAL)          \
	X(SIZE_M_DUAL_CAL)         \
	X(SIZE_S_DUAL_CAL)         \
	X(SIZE_ONLY_M_CAL_CRC)     \
	X(ADDR_M_HEADER)           \
	X(ADDR_S_FW_VER)           \
	X(ADDR_M_FW_VER)           \
	X(ADDR_M_CALMAP_VER)       \
	X(ADDR_M_DLL_VER)          \
	X(ADDR_S_DLL_VER)          \
	X(ADDR_M_MODULE_ID)        \
	X(ADDR_M_SENSOR_ID)        \
	X(ADDR_M_SENSOR_VER)       \
	X(ADDR_S_SENSOR_ID)        \
	X(ADDR_M0_MTF)             \
	X(ADDR_M1_MTF)             \
	X(ADDR_M2_MTF)             \
	X(ADDR_S0_MTF)             \
	X(ADDR_M0_LSC)             \
	X(ADDR_M1_LSC)             \
	X(ADDR_M2_LSC)             \
	X(ADDR_M0_PAF)             \
	X(ADDR_M0_BP)              \
	X(ADDR_M0_PLC)             \
	X(ADDR_M1_PAF)             \
	X(ADDR_M1_BP)              \
	X(ADDR_M1_PLC)             \
	X(ADDR_M2_PAF)             \
	X(ADDR_M2_BP)              \
	X(ADDR_M2_PLC)             \
	X(ADDR_M_AF)               \
	X(ADDR_M0_MODULE_AWB)      \
	X(ADDR_M1_MODULE_AWB)      \
	X(ADDR_M2_MODULE_AWB)      \
	X(ADDR_M0_AE)              \
	X(ADDR_M1_AE)              \
	X(ADDR_M2_AE)              \
	X(ADDR_M_OIS)              \
	X(ADDR_M_CAL_VER_WHEN_CAL) \
	X(ADDR_M_DUAL_CAL)         \
	X(ADDR_S_DUAL_CAL)         \
	X(ADDR_M_ATC_CAL)          \
	X(ADDR_S0_LSC)             \
	X(ADDR_S0_PAF)             \
	X(ADDR_S0_BP)              \
	X(ADDR_S0_PLC)             \
	X(ADDR_S0_AF)              \
	X(ADDR_S0_MODULE_AWB)      \
	X(ADDR_S0_AE)              \
	X(ADDR_S_OIS)              \
	X(ADDR_4PDC_CAL)           \
	X(ADDR_TCLSC_CAL)          \
	X(ADDR_SPDC_CAL)           \
	X(ADDR_PDXTC_CAL)          \
	X(ADDR_M_XTALK_CAL)        \
	X(ADDR_TOFCAL_START)       \
	X(ADDR_TOFCAL_SIZE)        \
	X(ADDR_TOFCAL_UID)         \
	X(ADDR_TOFCAL_RESULT)      \
	X(ADDR_VALIDATION_500)     \
	X(ADDR_VALIDATION_300)     \
	X(ADDR_CUSTOM_FW_VER)      \
	X(ADDR_CUSTOM_SENSOR_ID)

enum config_name_info_index {
#define X(Enum)       Enum,
    X_ENUMS
#undef X
	MAX_CONFIG_INFO_IDX
};

static const char* config_info_strs[] =
{
#define X(String) MAKE_STRINGIZE(String),
    X_ENUMS
#undef X
};

#define MAX_CUSTOM_STRING_LENGTH		(256)	//	should have the same value in chivendortag.h, camxpropertydefs.h

struct config_info_t {
	uint32_t    is_set;
	uint32_t    value;
};

enum main_sub {
	MAIN_MODULE,
	SUB_MODULE,
};

struct dual_tilt_t {
	int x;
	int y;
	int z;
	int sx;
	int sy;
	int range;
	int max_err;
	int avg_err;
	int dll_ver;
	char project_cal_type[PROJECT_CAL_TYPE_MAX_SIZE];
};

struct module_version_t {
	char *sensor_id;
	char *sensor2_id;
	char *module_id;

	char phone_hw_info[HW_INFO_MAX_SIZE];
	char phone_sw_info[SW_INFO_MAX_SIZE];
	char phone_vendor_info[VENDOR_INFO_MAX_SIZE];
	char phone_process_info[PROCESS_INFO_MAX_SIZE];
	char phone_chk_ver_info[CHK_VER_INFO_MAX_SIZE];

	char module_fw_ver[FROM_MODULE_FW_INFO_SIZE+1];
	char load_fw_ver[FROM_MODULE_FW_INFO_SIZE+1];
	char phone_fw_ver[FROM_MODULE_FW_INFO_SIZE+1];

	char *module_info;
	char *cam_cal_ack;
	char *cam_fw_ver;
	char *cam_fw_full_ver;

	char *fw_factory_ver;
	char *fw_user_ver;

	uint8_t *dual_cal;
	struct dual_tilt_t *dual_tilt;
};

struct module_info_t {
	struct module_version_t module_version;
	uint32_t                type;
	uint8_t                 map_version;
	enum main_sub           main_sub;
	char                    type_str[FROM_MODULE_FW_INFO_SIZE];
};

enum af_offset_index {
	AF_CAL_NEAR_IDX = 0,
	AF_CAL_FAR_IDX,
	AF_CAL_M1_IDX,
	AF_CAL_M2_IDX,
	AF_CAL_M3_IDX,
	AF_CAL_M4_IDX,
	AF_CAL_M5_IDX,
	AF_CAL_M6_IDX,
	AF_CAL_M7_IDX,
	AF_CAL_M8_IDX,
	AF_CAL_IDX_MAX
};

struct af_index_t {
	enum af_offset_index idx;
	uint32_t     offset;
};

#define AF_CAL_NEAR_OFFSET_FROM_AF                  0x0010
#define AF_CAL_FAR_OFFSET_FROM_AF                   0x0004
#define AF_CAL_M1_OFFSET_FROM_AF                    0x0008
#define AF_CAL_M2_OFFSET_FROM_AF                    0x000C
#define AF_CAL_M0_OFFSET_FROM_AF                    0x0000

#define PAF_OFFSET_CAL_ERR_CHECK                    (0x0014)
#define PAF_MID_SIZE                                936
#define PAF_MID_OFFSET                              (0x0730)

#define PAF_FAR_SIZE                                234
#define PAF_FAR_OFFSET                              (0x0CD0)

#define MAX_AF_CAL_STR_SIZE                         256

//extern int rear_af_cal[FROM_REAR_AF_CAL_SIZE + 1];
extern char af_cal_str[INDEX_MAX][MAX_AF_CAL_STR_SIZE];
extern char sensor_id[INDEX_MAX][FROM_SENSOR_ID_SIZE + 1];
extern uint8_t module_id[INDEX_MAX][FROM_MODULE_ID_SIZE + 1];
extern char module_info[INDEX_MAX][SYSFS_MODULE_INFO_SIZE];
extern char mtf_exif[INDEX_MAX][FROM_MTF_SIZE + 1];
extern char fw_ver[INDEX_MAX][SYSFS_FW_VER_SIZE];
extern char fw_full_ver[INDEX_MAX][SYSFS_FW_VER_SIZE];
extern char fw_factory_ver[INDEX_MAX][SYSFS_FW_VER_SIZE];
extern char fw_user_ver[INDEX_MAX][SYSFS_FW_VER_SIZE];
extern uint32_t paf_err_data_result[INDEX_MAX];
#if defined(CONFIG_SAMSUNG_REAR_DUAL)
extern uint8_t dual_cal[INDEX_MAX][FROM_MAX_DUAL_CAL_SIZE + 1];
extern struct dual_tilt_t dual_tilt[INDEX_MAX];
#endif
#if defined(CONFIG_CAMERA_HW_ERROR_DETECT)
extern char retry_cnt[INDEX_MAX][5];
#endif

extern char cal_crc[SYSFS_FW_VER_SIZE];
extern char rear_mtf2_exif[FROM_MTF_SIZE + 1];
extern char rear_paf_cal_data_far[PAF_2PD_CAL_INFO_SIZE];
extern char rear_paf_cal_data_mid[PAF_2PD_CAL_INFO_SIZE];
extern char rear_f2_paf_cal_data_far[PAF_2PD_CAL_INFO_SIZE];
extern char rear_f2_paf_cal_data_mid[PAF_2PD_CAL_INFO_SIZE];
extern uint32_t f2_paf_err_data_result;

/* phone fw info */
extern uint32_t CAMERA_NORMAL_CAL_CRC;

#if !defined(CONFIG_SAMSUNG_FRONT_TOP_EEPROM)
extern uint32_t front_af_cal_pan;
extern uint32_t front_af_cal_macro;
#endif

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || \
	defined(CONFIG_SAMSUNG_LPAI_OIS)
#define OIS_XYGG_SIZE                               8
#define OIS_XYSR_SIZE                               4
#define OIS_XYGG_START_OFFSET                       0x10
#define OIS_CAL_MARK_START_OFFSET                   0x30
#define OIS_XYSR_START_OFFSET                       0x38
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE) || \
	defined(CONFIG_SAMSUNG_REAR_QUADRA)
#define OIS_CROSSTALK_SIZE                          4
#define OIS_CROSSTALK_START_OFFSET                  0x1C
#endif

extern uint8_t ois_cal_mark[INDEX_MAX];
extern int ois_gain_result[INDEX_MAX];
extern uint8_t ois_xygg[INDEX_MAX][OIS_XYGG_SIZE];
extern int ois_sr_result[INDEX_MAX];
extern uint8_t ois_xysr[INDEX_MAX][OIS_XYSR_SIZE];
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE) || \
	defined(CONFIG_SAMSUNG_REAR_QUADRA)
extern int ois_cross_talk_result[INDEX_MAX];
extern uint8_t ois_cross_talk[INDEX_MAX][OIS_CROSSTALK_SIZE];
#endif
#endif

int is_valid_index(
	struct config_info_t *config_info,
	enum config_name_info_index config_index,
	uint32_t *config_addr);

void uint_to_char_array(
	uint32_t num, char* arr, int arr_size);

int map_sensor_id_to_sysfs_index(
	int sensor_id);

void map_sensor_id_to_type_str(
	int sensor_id, char* type_str);

int cam_sec_eeprom_module_info_set_sensor_id(
	struct cam_eeprom_ctrl_t *e_ctrl,
	struct module_info_t *m_info,
	struct config_info_t *config_info,
	char *sensor_ver);

int cam_sec_eeprom_module_info_set_module_id(
	struct module_info_t *m_info,
	uint8_t *map_data);

int cam_sec_eeprom_update_def_info(
	struct module_info_t* m_info,
	struct config_info_t *config_info);

int cam_sec_eeprom_module_info_set_paf(
	struct config_info_t *config_info, uint32_t dual_addr_idx,
	uint32_t st_offset, uint32_t mid_far_size, uint8_t *map_data, char *log_str,
	char *paf_cal, uint32_t paf_cal_size);

int cam_sec_eeprom_update_afcal(
	struct cam_eeprom_ctrl_t *e_ctrl,
	struct config_info_t *config_info);

int cam_sec_eeprom_update_mtf_exif(
	struct cam_eeprom_ctrl_t *e_ctrl,
	struct config_info_t *config_info);

int cam_sec_eeprom_update_mtf2_exif(
	struct cam_eeprom_ctrl_t *e_ctrl,
	struct config_info_t *config_info);

int cam_sec_eeprom_update_paf(
	struct cam_eeprom_ctrl_t *e_ctrl,
	struct config_info_t *config_info);

int cam_sec_eeprom_update_paf_err(
	struct cam_eeprom_ctrl_t *e_ctrl,
	struct config_info_t *config_info);

int cam_sec_eeprom_reset_module_info(
	struct cam_eeprom_ctrl_t *e_ctrl);

int cam_sec_eeprom_update_module_info(
	struct cam_eeprom_ctrl_t *e_ctrl,
	struct module_info_t *m_info,
	struct config_info_t *config_info);

void cam_sec_eeprom_update_sysfs_fw_version(
	const char *update_fw_ver,
	enum fw_ver_index update_fw_index,
	struct module_info_t *m_info);

int32_t cam_sec_eeprom_check_firmware_cal(
	struct cam_eeprom_ctrl_t *e_ctrl,
	struct module_info_t *m_info);

int cam_sec_eeprom_verify_sum(
	const char *mem, uint32_t size,
	uint32_t sum, uint32_t rev_endian);

uint32_t cam_sec_eeprom_match_crc(
	struct cam_eeprom_memory_block_t *data,
	uint32_t subdev_id,
	struct config_info_t *config_info);

int32_t cam_sec_eeprom_calc_calmap_size(
	struct cam_eeprom_ctrl_t *e_ctrl);

int32_t cam_sec_eeprom_fill_config_info(
	struct config_info_t *config_info,
	char *configString,
	uint32_t value);

int32_t cam_sec_eeprom_get_custom_info(
	struct cam_eeprom_ctrl_t *e_ctrl,
	struct cam_packet *csl_packet,
	struct config_info_t *config_info);
#endif
/* _CAM_SEC_EEPROM_CORE_H_ */
