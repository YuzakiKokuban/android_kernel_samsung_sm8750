// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/crc32.h>
#include <linux/firmware.h>
#include <media/cam_sensor.h>

#include "cam_sec_eeprom_core.h"
#include "cam_eeprom_soc.h"
#include "cam_debug_util.h"
#include "cam_common_util.h"
#include "cam_packet_util.h"
#include <linux/ctype.h>

#if defined(CONFIG_SAMSUNG_WACOM_NOTIFIER)
#include "cam_notifier.h"
#endif

#define CAM_EEPROM_DBG  1
#define MAX_READ_SIZE  0x7FFFF

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

char cam_cal_check[INDEX_MAX][SYSFS_FW_VER_SIZE] = { [0 ... INDEX_MAX - 1] = "NULL" };

#ifdef CAM_EEPROM_DBG_DUMP
int cam_sec_eeprom_dump(
	uint32_t subdev_id, uint8_t *mapdata,
	uint32_t addr, uint32_t size)
{
	int rc = 0;
	int j;

	if (mapdata == NULL) {
		CAM_ERR(CAM_EEPROM, "mapdata is NULL");
		return -1;
	}
	if (size == 0) {
		CAM_ERR(CAM_EEPROM, "size is 0");
		return -1;
	}

	CAM_INFO(CAM_EEPROM,
		"subdev_id: %d, eeprom dump addr = 0x%04X, total read size = %d",
		subdev_id, addr, size);
	for (j = 0; j < size; j++)
		CAM_INFO(CAM_EEPROM, "addr = 0x%04X, data = 0x%02X",
			addr+j, mapdata[addr+j]);

	return rc;
}
#endif

int is_valid_index(
	struct config_info_t *config_info,
	enum config_name_info_index config_index,
	uint32_t *config_addr)
{
	if (config_index >= MAX_CONFIG_INFO_IDX)
	{
		CAM_ERR(CAM_EEPROM, "invalid index: %d, max:%d",
			config_index, MAX_CONFIG_INFO_IDX);
		return 0;
	}

	if (config_info[config_index].is_set == 1)
	{
		*config_addr = config_info[config_index].value;
		CAM_DBG(CAM_EEPROM, "%s: %d, is_set: %d, addr: 0x%08X",
			config_info_strs[config_index], config_index,
			config_info[config_index].is_set,
			config_info[config_index].value);

		return 1;
	}
	else
	{
		*config_addr = 0;
		CAM_DBG(CAM_EEPROM, "%s: %d, is_set: %d",
			config_info_strs[config_index], config_index,
			config_info[config_index].is_set);

		return 0;
	}
}

void uint_to_char_array(uint32_t num, char* arr, int arr_size) {
	int i = 0;
	int shift = 0;

	for (i = 0; i < arr_size; i++) {
		shift = (arr_size - 1 - i) * 8;
		arr[i] = (num >> shift) & 0xFF;
		CAM_DBG(CAM_EEPROM, "arr[%d] %c, shift %d", i, arr[i], shift);
	}
}

int map_sensor_id_to_sysfs_index(int sensor_id)
{
	switch(sensor_id)
	{
		case SEC_WIDE_SENSOR:
			return INDEX_REAR;
		case SEC_ULTRA_WIDE_SENSOR:
			return INDEX_REAR2;
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
		case SEC_TELE_SENSOR:
			return INDEX_REAR3;
#endif
#if defined(CONFIG_SAMSUNG_REAR_QUADRA)
		case SEC_TELE2_SENSOR:
			return INDEX_REAR4;
#endif
		case SEC_FRONT_SENSOR:
			return INDEX_FRONT;
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
		case SEC_FRONT_AUX1_SENSOR:
			return INDEX_FRONT2;
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
		case SEC_FRONT_TOP_SENSOR:
			return INDEX_FRONT3;
#else
		case SEC_FRONT_TOP_SENSOR:
			return INDEX_FRONT2;
#endif
#endif
		default:
			return -EINVAL;
	}
	return -EINVAL;
}

void map_sensor_id_to_type_str(int sensor_id, char* type_str)
{
	switch(sensor_id)
	{
		case SEC_WIDE_SENSOR:
			strlcpy(type_str, "Rear", FROM_MODULE_FW_INFO_SIZE);
			break;
		case SEC_ULTRA_WIDE_SENSOR:
			strlcpy(type_str, "Rear2", FROM_MODULE_FW_INFO_SIZE);
			break;
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
		case SEC_TELE_SENSOR:
			strlcpy(type_str, "Rear3", FROM_MODULE_FW_INFO_SIZE);
			break;
#endif
#if defined(CONFIG_SAMSUNG_REAR_QUADRA)
		case SEC_TELE2_SENSOR:
			strlcpy(type_str, "Rear4", FROM_MODULE_FW_INFO_SIZE);
			break;
#endif
		case SEC_FRONT_SENSOR:
			strlcpy(type_str, "Front", FROM_MODULE_FW_INFO_SIZE);
			break;
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
		case SEC_FRONT_AUX1_SENSOR:
			strlcpy(type_str, "Front2", FROM_MODULE_FW_INFO_SIZE);
			break;
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
		case SEC_FRONT_TOP_SENSOR:
			strlcpy(type_str, "Front3", FROM_MODULE_FW_INFO_SIZE);
			break;
#else
		case SEC_FRONT_TOP_SENSOR:
			strlcpy(type_str, "Front2", FROM_MODULE_FW_INFO_SIZE);
			break;
#endif
#endif
		default:
			break;
	}
}

int cam_sec_eeprom_module_info_set_sensor_id(
	struct cam_eeprom_ctrl_t *e_ctrl,
	struct module_info_t *m_info,
	struct config_info_t *config_info,
	char *sensor_ver)
{
	uint32_t config_addr = 0;
	uint32_t config_idx = ADDR_M_SENSOR_ID;
	struct module_version_t *module_version = NULL;
	char	*sensorId = "";

	if (e_ctrl == NULL ||
		e_ctrl->cal_data.mapdata == NULL ||
		m_info == NULL ||
		m_info->module_version.sensor_id == NULL ||
		m_info->module_version.sensor2_id == NULL ||
		config_info == NULL) {
		CAM_ERR(CAM_EEPROM, "Invalid argument");
		return -EINVAL;
	}

	if (!is_valid_index(config_info, config_idx, &config_addr)) {
		CAM_ERR(CAM_EEPROM, "Invalid argument");
		return -EINVAL;
	}

	module_version = &m_info->module_version;
	sensorId = module_version->sensor_id;

	memcpy(sensorId, &e_ctrl->cal_data.mapdata[config_addr], FROM_SENSOR_ID_SIZE);
	sensorId[FROM_SENSOR_ID_SIZE] = '\0';

	if (sensor_ver != NULL)
		sensor_ver[0] = sensorId[8];

	CAM_INFO(CAM_EEPROM,
		"%s sensor_id = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
		m_info->type_str,
		sensorId[0], sensorId[1], sensorId[2], sensorId[3],
		sensorId[4], sensorId[5], sensorId[6], sensorId[7],
		sensorId[8], sensorId[9], sensorId[10], sensorId[11],
		sensorId[12], sensorId[13], sensorId[14], sensorId[15]);

	return 0;
}

int cam_sec_eeprom_module_info_set_module_id(
	struct module_info_t *m_info, uint8_t *map_data)
{
	char 	*moduleId = "";

	if (m_info == NULL ||
		m_info->module_version.module_id == NULL ||
		map_data == NULL)
	{
		CAM_ERR(CAM_EEPROM, "Invalid argument");
		return -EINVAL;
	}
	moduleId = m_info->module_version.module_id;

	memcpy(moduleId, map_data, FROM_MODULE_ID_SIZE);
	moduleId[FROM_MODULE_ID_SIZE] = '\0';

	CAM_DBG(CAM_EEPROM, "%s module_id = %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
		m_info->type_str,
		moduleId[0], moduleId[1], moduleId[2], moduleId[3],
		moduleId[4], moduleId[5], moduleId[6], moduleId[7],
		moduleId[8], moduleId[9]);

	return 0;
}

static int cam_sec_eeprom_module_info_set_load_version(
	struct cam_eeprom_ctrl_t *e_ctrl,
	struct module_info_t *m_info,
	struct config_info_t *config_info)
{
	int         rc                  = 0;
	int         i                   = 0;
	int         rev                 = 0;
	int         index               = -1;

	uint8_t     loadfrom            = 'N';
	uint8_t     sensor_ver[2]       = {0,};
	uint8_t     dll_ver[2]          = {0,};
	uint32_t    normal_is_supported = 0;
	uint8_t     normal_cri_rev      = 0;
	uint8_t     is_version_null     = FALSE;

	uint32_t    config_index        = 0;
	uint32_t    config_addr         = 0;
	uint8_t     criterion_rev		= 0;

	char        cal_ver[FROM_MODULE_FW_INFO_SIZE+1]  = "";
	char        ideal_ver[FROM_MODULE_FW_INFO_SIZE+1] = "";

	char        *module_fw_ver;
	char        *load_fw_ver;
	char        *phone_fw_ver;

	struct module_version_t *module_version = NULL;

	if ((e_ctrl == NULL) || (e_ctrl->cal_data.mapdata == NULL)) {
		CAM_ERR(CAM_EEPROM, "e_ctrl is NULL");
		return -EINVAL;
	}

	if ((m_info == NULL) || (config_info == NULL)) {
		CAM_ERR(CAM_EEPROM, "invalid argument");
		rc = 1;
		return rc;
	}

	index = map_sensor_id_to_sysfs_index(e_ctrl->soc_info.index);
	if (index < 0 || index >= INDEX_MAX)
		return -EINVAL;

	map_sensor_id_to_type_str(e_ctrl->soc_info.index, m_info->type_str);

	m_info->type_str[FROM_MODULE_FW_INFO_SIZE-1] = '\0';
	m_info->type 						= e_ctrl->soc_info.index;
	m_info->main_sub					   = MAIN_MODULE;

	module_version = &m_info->module_version;

	module_version->sensor_id			   = sensor_id[index];
	module_version->sensor2_id			   = sensor_id[index];
	module_version->module_id			   = module_id[index];

	module_version->module_info			   = module_info[index];

	module_version->cam_cal_ack			   = cam_cal_check[index];
	if (e_ctrl->soc_info.index == SEC_FRONT_TOP_SENSOR)
		module_version->cam_cal_ack		   = cam_cal_check[INDEX_FRONT];
	module_version->cam_fw_ver			   = fw_ver[index];
	module_version->cam_fw_full_ver		   = fw_full_ver[index];

	module_version->fw_user_ver			   = fw_user_ver[index];
	module_version->fw_factory_ver		   = fw_factory_ver[index];

	criterion_rev = module_version->phone_chk_ver_info[0];

	module_fw_ver = module_version->module_fw_ver;
	phone_fw_ver = module_version->phone_fw_ver;
	load_fw_ver = module_version->load_fw_ver;

	memset(module_fw_ver, 0x00, FROM_MODULE_FW_INFO_SIZE+1);
	memset(phone_fw_ver, 0x00, FROM_MODULE_FW_INFO_SIZE+1);
	memset(load_fw_ver, 0x00, FROM_MODULE_FW_INFO_SIZE+1);

	if (is_valid_index(config_info, ADDR_M_CALMAP_VER, &config_addr) == 1) {
		config_addr += 0x03;
		m_info->map_version = e_ctrl->cal_data.mapdata[config_addr];
	}

	if (m_info->map_version >= 0x80 || !isalnum(m_info->map_version)) {
		CAM_INFO(CAM_EEPROM, "subdev_id: %d, map version = 0x%x", m_info->type, m_info->map_version);
		m_info->map_version = '0';
	} else {
		CAM_INFO(CAM_EEPROM, "subdev_id: %d, map version = %c [0x%x]", m_info->type, m_info->map_version, m_info->map_version);
	}

	config_index = m_info->main_sub == MAIN_MODULE ? ADDR_M_FW_VER : ADDR_S_FW_VER;
	if (is_valid_index(config_info, config_index, &config_addr) == 1)
	{
		memcpy(module_fw_ver, &e_ctrl->cal_data.mapdata[config_addr], FROM_MODULE_FW_INFO_SIZE);
		module_fw_ver[FROM_MODULE_FW_INFO_SIZE] = '\0';
		CAM_DBG(CAM_EEPROM,
			"%s manufacturer info = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
			m_info->type_str,
			module_fw_ver[0], module_fw_ver[1], module_fw_ver[2], module_fw_ver[3], module_fw_ver[4],
			module_fw_ver[5], module_fw_ver[6], module_fw_ver[7], module_fw_ver[8], module_fw_ver[9],
			module_fw_ver[10]);

		/* temp phone version */
		snprintf(phone_fw_ver, FROM_MODULE_FW_INFO_SIZE+1, "%s%s%s%s",
			module_version->phone_hw_info, module_version->phone_sw_info,
			module_version->phone_vendor_info, module_version->phone_process_info);
		phone_fw_ver[FROM_MODULE_FW_INFO_SIZE] = '\0';
		CAM_DBG(CAM_EEPROM,
			"%s phone info = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
			m_info->type_str,
			phone_fw_ver[0], phone_fw_ver[1], phone_fw_ver[2], phone_fw_ver[3], phone_fw_ver[4],
			phone_fw_ver[5], phone_fw_ver[6], phone_fw_ver[7], phone_fw_ver[8], phone_fw_ver[9],
			phone_fw_ver[10]);
	}

	/* temp load version */
	if (m_info->type == SEC_WIDE_SENSOR && m_info->main_sub == MAIN_MODULE &&
		strncmp(phone_fw_ver, module_fw_ver, HW_INFO_MAX_SIZE-1) == 0 &&
		strncmp(&phone_fw_ver[HW_INFO_MAX_SIZE-1], &module_fw_ver[HW_INFO_MAX_SIZE-1], SW_INFO_MAX_SIZE-1) >= 0)
	{
		CAM_INFO(CAM_EEPROM, "Load from phone");
		strcpy(load_fw_ver, phone_fw_ver);
		loadfrom = 'P';
	}
	else
	{
		CAM_INFO(CAM_EEPROM, "Load from EEPROM");
		strcpy(load_fw_ver, module_fw_ver);
		loadfrom = 'E';
	}

	//	basically, cal_ver is the version when the module is calibrated.
	//	It can be different in case that the module_fw_ver is updated by FW on F-ROM for testing.
	//	otherwise, module_fw_ver and cal_ver should be the same.
	config_index = m_info->main_sub == MAIN_MODULE ? ADDR_M_FW_VER : ADDR_S_FW_VER;
	if (is_valid_index(config_info, config_index, &config_addr) == 1)
	{
		is_version_null = FALSE;
		for(i = 0; i < FROM_MODULE_FW_INFO_SIZE; i ++) {
			if (e_ctrl->cal_data.mapdata[config_addr + i] >= 0x80 || !isalnum(e_ctrl->cal_data.mapdata[config_addr + i])) {
				cal_ver[i] = ' ';
				is_version_null = TRUE;
			} else {
				cal_ver[i] = e_ctrl->cal_data.mapdata[config_addr + i];
			}

			if (phone_fw_ver[i] >= 0x80 || !isalnum(phone_fw_ver[i]))
			{
				phone_fw_ver[i] = ' ';
			}
		}
	}

	if (is_valid_index(config_info, ADDR_M_MODULE_ID, &config_addr) == 1)
	{
		config_addr += 0x06;
		cam_sec_eeprom_module_info_set_module_id(m_info, &e_ctrl->cal_data.mapdata[config_addr]);
	}

	sensor_ver[0] = 0;
	sensor_ver[1] = 0;
	dll_ver[0] = 0;
	dll_ver[1] = 0;

	cam_sec_eeprom_module_info_set_sensor_id(
		e_ctrl, m_info, config_info, sensor_ver);

	if (is_valid_index(config_info, ADDR_M_DLL_VER, &config_addr) == 1)
	{
		config_addr += 0x03;
		dll_ver[0] = e_ctrl->cal_data.mapdata[config_addr] - '0';
	}

	normal_is_supported = e_ctrl->camera_normal_cal_crc;

	if (is_valid_index(config_info, DEF_M_CHK_VER, &config_addr) == 1)
	{
		normal_cri_rev = criterion_rev;
	}

	strcpy(ideal_ver, phone_fw_ver);
	if (module_fw_ver[9] < 0x80 && isalnum(module_fw_ver[9])) {
		ideal_ver[9] = module_fw_ver[9];
	}
	if (module_fw_ver[10] < 0x80 && isalnum(module_fw_ver[10])) {
		ideal_ver[10] = module_fw_ver[10];
	}

	if (rev < normal_cri_rev && is_version_null == TRUE)
	{
		strcpy(cal_ver, ideal_ver);
		loadfrom = 'P';
		CAM_ERR(CAM_EEPROM, "set tmp ver: %s", cal_ver);
	}

	snprintf(module_version->module_info, SYSFS_MODULE_INFO_SIZE, "SSCAL %c%s%04X%04XR%02dM%cD%02XD%02XS%02XS%02X/%s%04X%04XR%02d",
		loadfrom, cal_ver, (e_ctrl->is_supported >> 16) & 0xFFFF, e_ctrl->is_supported & 0xFFFF,
		rev & 0xFF, m_info->map_version, dll_ver[0] & 0xFF, dll_ver[1] & 0xFF, sensor_ver[0] & 0xFF, sensor_ver[1] & 0xFF,
		ideal_ver, (normal_is_supported >> 16) & 0xFFFF, normal_is_supported & 0xFFFF, normal_cri_rev);
#ifdef CAM_EEPROM_DBG
	CAM_DBG(CAM_EEPROM, "%s info = %s", m_info->type_str, module_version->module_info);
#endif

	/* update EEPROM fw version on sysfs */
	// if (m_info->type != SEC_WIDE_SENSOR)
	{
		strncpy(load_fw_ver, module_fw_ver, FROM_MODULE_FW_INFO_SIZE);
		load_fw_ver[FROM_MODULE_FW_INFO_SIZE] = '\0';
		sprintf(phone_fw_ver, "N");
	}

	//	tele module
	if (m_info->type == SEC_WIDE_SENSOR && m_info->main_sub != MAIN_MODULE)
	{
		sprintf(phone_fw_ver, "N");
	}

	sprintf(module_version->cam_fw_ver, "%s %s\n", module_fw_ver, load_fw_ver);
	sprintf(module_version->cam_fw_full_ver, "%s %s %s\n", module_fw_ver, phone_fw_ver, load_fw_ver);

#ifdef CAM_EEPROM_DBG
	CAM_DBG(CAM_EEPROM, "%s manufacturer info = %c %c %c %c %c %c %c %c %c %c %c",
		m_info->type_str,
		module_fw_ver[0], module_fw_ver[1], module_fw_ver[2], module_fw_ver[3], module_fw_ver[4],
		module_fw_ver[5], module_fw_ver[6], module_fw_ver[7], module_fw_ver[8], module_fw_ver[9],
		module_fw_ver[10]);

	CAM_DBG(CAM_EEPROM, "%s phone_fw_ver = %c %c %c %c %c %c %c %c %c %c %c",
		m_info->type_str,
		phone_fw_ver[0], phone_fw_ver[1], phone_fw_ver[2], phone_fw_ver[3], phone_fw_ver[4],
		phone_fw_ver[5], phone_fw_ver[6], phone_fw_ver[7], phone_fw_ver[8], phone_fw_ver[9],
		phone_fw_ver[10]);

	CAM_DBG(CAM_EEPROM, "%s load_fw_ver = %c %c %c %c %c %c %c %c %c %c %c",
		m_info->type_str,
		load_fw_ver[0], load_fw_ver[1], load_fw_ver[2], load_fw_ver[3], load_fw_ver[4],
		load_fw_ver[5], load_fw_ver[6], load_fw_ver[7], load_fw_ver[8], load_fw_ver[9],
		load_fw_ver[10]);
#endif

	return rc;
}

int cam_sec_eeprom_update_def_info(
	struct module_info_t* m_info,
	struct config_info_t *config_info)
{
	uint32_t value;
	struct module_version_t *module_version = NULL;

	if (m_info == NULL || config_info == NULL)
	{
		CAM_ERR(CAM_EEPROM, "Invalid argument");
		return -EINVAL;
	}

	module_version = &m_info->module_version;
	if (is_valid_index(config_info, DEF_M_CORE_VER, &value)) {
		module_version->phone_hw_info[0] = (value) & 0xFF;
	}

	if (is_valid_index(config_info, DEF_M_VER_HW, &value)) {
		uint_to_char_array(value, &module_version->phone_hw_info[1], 4);
		CAM_DBG(CAM_EEPROM, "M_HW_INFO: %c %c%c %c %c",
			module_version->phone_hw_info[0], module_version->phone_hw_info[1],
			module_version->phone_hw_info[2], module_version->phone_hw_info[3],
			module_version->phone_hw_info[4]);
	}

	if (is_valid_index(config_info, DEF_M_VER_SW, &value)) {
		uint_to_char_array(value, module_version->phone_sw_info, 4);
		CAM_DBG(CAM_EEPROM, "M_SW_INFO: %c %c %c%c",
			module_version->phone_sw_info[0], module_version->phone_sw_info[1],
			module_version->phone_sw_info[2], module_version->phone_sw_info[3]);
	}

	if (is_valid_index(config_info, DEF_M_VER_ETC, &value)) {
		module_version->phone_vendor_info[0] = (value >> 24) & 0xFF;
		module_version->phone_process_info[0] = (value >> 16) & 0xFF;
		CAM_DBG(CAM_EEPROM, "M_ETC_VER: %c %c",
			module_version->phone_vendor_info[0], module_version->phone_process_info[0]);
	}

	if (is_valid_index(config_info, DEF_M_CHK_VER, &value)) {
		uint_to_char_array(value, module_version->phone_chk_ver_info, 4);
		module_version->phone_chk_ver_info[3] += '0'; // min_cal_map_ver
		CAM_DBG(CAM_EEPROM,
			"value: 0x%08X, criterion_rev: %d, module_ver_on_pvr: %c, module_ver_on_sra: %c, min_cal_map_ver: %d",
			value, module_version->phone_chk_ver_info[0],
			module_version->phone_chk_ver_info[1],
			module_version->phone_chk_ver_info[2],
			module_version->phone_chk_ver_info[3]);
	}

	return 0;
}

int cam_sec_eeprom_module_info_set_paf(
	struct config_info_t *config_info, uint32_t dual_addr_idx,
	uint32_t st_offset, uint32_t mid_far_size, uint8_t *map_data, char *log_str,
	char *paf_cal, uint32_t paf_cal_size)
{
	int i, step, offset = 0, cnt = 0;
	uint32_t size;
	uint32_t config_addr = 0;
	char suffix = ',';

	if (!is_valid_index(config_info, dual_addr_idx, &config_addr)) {
		CAM_ERR(CAM_EEPROM, "Invalid argument");
		return -EINVAL;
	}

	step = 2;
	if (mid_far_size == PAF_MID_SIZE)
		step = 8;

	size = mid_far_size/step;
	if (size <= 1)
	{
		CAM_ERR(CAM_EEPROM, "mid_far_size was wrong mid_far_size = %d", mid_far_size);
		return 0;
	}

	CAM_DBG(CAM_EEPROM, "paf_cal: %p, paf_cal_size: %d", paf_cal, paf_cal_size);
	config_addr += st_offset;

	memset(paf_cal, 0, paf_cal_size);

	for (i = 0; i < size; i++) {
		suffix = (i == (size - 1)) ? '\n' : ',';
		cnt = scnprintf(&paf_cal[offset], paf_cal_size - offset,
			"%d%c", *((s16 *)&map_data[config_addr + step*i]), suffix);
		offset += cnt;
	}

	paf_cal[offset] = '\0';

	CAM_DBG(CAM_EEPROM, "%s = %s", log_str, paf_cal);

	return 0;
}

int cam_sec_eeprom_update_afcal(
	struct cam_eeprom_ctrl_t *e_ctrl, struct config_info_t *config_info)
{
	int i, offset = 0, cnt = 0, len = 0;
	uint32_t tempval;
	uint32_t config_addr = 0;
	struct af_index_t af_idx[] = {
		{AF_CAL_NEAR_IDX, AF_CAL_NEAR_OFFSET_FROM_AF},
		{AF_CAL_FAR_IDX, AF_CAL_FAR_OFFSET_FROM_AF},
		{AF_CAL_M1_IDX, AF_CAL_M1_OFFSET_FROM_AF},
		{AF_CAL_M2_IDX, AF_CAL_M2_OFFSET_FROM_AF},
	};
#if defined(CONFIG_SAMSUNG_AFCAL_EXT)
	struct af_index_t af_ext_idx[] = {
		{AF_CAL_NEAR_IDX, AF_CAL_NEAR_OFFSET_FROM_AF},
		{AF_CAL_FAR_IDX, AF_CAL_FAR_OFFSET_FROM_AF},
		{AF_CAL_M3_IDX, AF_CAL_M0_OFFSET_FROM_AF},
		{AF_CAL_M1_IDX, AF_CAL_M1_OFFSET_FROM_AF},
		{AF_CAL_M2_IDX, AF_CAL_M2_OFFSET_FROM_AF},
	};
#endif
	uint32_t config_idx = ADDR_M_AF;
	uint32_t num_idx = ARRAY_SIZE(af_idx);
#if defined(CONFIG_SAMSUNG_AFCAL_EXT)
	uint32_t num_ext_idx = ARRAY_SIZE(af_ext_idx);
#endif
	int index = map_sensor_id_to_sysfs_index(e_ctrl->soc_info.index);

	if (index < 0 || index >= INDEX_MAX) {
		CAM_ERR(CAM_EEPROM, "Invalid Camera ID!");
		return -EINVAL;
	}

	if (e_ctrl->soc_info.index == SEC_TELE_SENSOR ||
		e_ctrl->soc_info.index == SEC_TELE2_SENSOR)
		config_idx = ADDR_S0_AF;

	if (!is_valid_index(config_info, config_idx, &config_addr)) {
		CAM_ERR(CAM_EEPROM, "Invalid argument");
		return -EINVAL;
	}

	if (e_ctrl->soc_info.index == SEC_FRONT_SENSOR ||
		e_ctrl->soc_info.index == SEC_FRONT_TOP_SENSOR)
		num_idx = 2;

	CAM_INFO(CAM_EEPROM, "config_idx: 0x%04X, af_cal_str = %s",
		config_idx, af_cal_str[index]);

	memset(af_cal_str[index], 0, MAX_AF_CAL_STR_SIZE);

#if defined(CONFIG_SAMSUNG_AFCAL_EXT)
	if (e_ctrl->soc_info.index == SEC_TELE2_SENSOR)
	{
		for(i = 0; i < num_ext_idx; i ++)
		{
			memcpy(&tempval, &e_ctrl->cal_data.mapdata[config_addr + af_ext_idx[i].offset], 4);

			cnt = scnprintf(&af_cal_str[index][offset],
				MAX_AF_CAL_STR_SIZE - offset, "%d ", tempval);
			offset += cnt;
		}
	} else
#endif
	{
		for(i = 0; i < num_idx; i ++)
		{
			memcpy(&tempval, &e_ctrl->cal_data.mapdata[config_addr + af_idx[i].offset], 4);

			cnt = scnprintf(&af_cal_str[index][offset],
				MAX_AF_CAL_STR_SIZE - offset, "%d ", tempval);
			offset += cnt;
		}
	}

	af_cal_str[index][offset] = '\0';

	len = strlen(af_cal_str[index]);
	if (af_cal_str[index][len-1] == ' ')
		af_cal_str[index][len-1] = '\0';

	CAM_INFO(CAM_EEPROM, "af_cal_str = %s", af_cal_str[index]);

	return 0;
}

int cam_sec_eeprom_update_mtf_exif(
	struct cam_eeprom_ctrl_t *e_ctrl, struct config_info_t *config_info)
{
	uint32_t config_addr = 0;
	uint32_t config_idx = ADDR_M0_MTF;
	int index = map_sensor_id_to_sysfs_index(e_ctrl->soc_info.index);

	if (index < 0 || index >= INDEX_MAX)
		return -EINVAL;

	if (e_ctrl->soc_info.index == SEC_TELE_SENSOR ||
		e_ctrl->soc_info.index == SEC_TELE2_SENSOR)
		config_idx = ADDR_S0_MTF;

	if (!is_valid_index(config_info, config_idx, &config_addr)) {
		CAM_ERR(CAM_EEPROM, "Invalid argument");
		return -EINVAL;
	}

	memcpy(mtf_exif[index], &e_ctrl->cal_data.mapdata[config_addr], FROM_MTF_SIZE);
	mtf_exif[index][FROM_MTF_SIZE] = '\0';
	CAM_DBG(CAM_EEPROM, "index %d mtf exif = %s", index, mtf_exif[index]);

	return 0;
}

int cam_sec_eeprom_update_mtf2_exif(
	struct cam_eeprom_ctrl_t *e_ctrl, struct config_info_t *config_info)
{
	uint32_t config_addr	= 0;
	uint32_t config_idx = ADDR_M1_MTF;
	int index = map_sensor_id_to_sysfs_index(e_ctrl->soc_info.index);

	if (index < 0 || index >= INDEX_MAX)
		return -EINVAL;

	if (e_ctrl->soc_info.index != SEC_WIDE_SENSOR)
		return 0;

	if (!is_valid_index(config_info, config_idx, &config_addr)) {
		CAM_ERR(CAM_EEPROM, "Invalid argument");
		return -EINVAL;
	}

	/* rear mtf2 exif */
	memcpy(rear_mtf2_exif, &e_ctrl->cal_data.mapdata[config_addr], FROM_MTF_SIZE);
	rear_mtf2_exif[FROM_MTF_SIZE] = '\0';
	CAM_DBG(CAM_EEPROM, "rear mtf2 exif = %s", rear_mtf2_exif);

	return 0;
}

int cam_sec_eeprom_update_paf(
	struct cam_eeprom_ctrl_t *e_ctrl, struct config_info_t *config_info)
{
	uint32_t config_addr	= 0;

	if (e_ctrl->soc_info.index != SEC_WIDE_SENSOR)
		return 0;

	cam_sec_eeprom_module_info_set_paf(config_info, ADDR_M0_PAF,
		PAF_MID_OFFSET, PAF_MID_SIZE,
		e_ctrl->cal_data.mapdata, "rear_paf_cal_data_mid",
		rear_paf_cal_data_mid, (uint32_t) sizeof(rear_paf_cal_data_mid));

	cam_sec_eeprom_module_info_set_paf(config_info, ADDR_M0_PAF,
		PAF_FAR_OFFSET, PAF_FAR_SIZE,
		e_ctrl->cal_data.mapdata, "rear_paf_cal_data_far",
		rear_paf_cal_data_far, (uint32_t) sizeof(rear_paf_cal_data_far));

	cam_sec_eeprom_module_info_set_paf(config_info, ADDR_M1_PAF,
		PAF_MID_OFFSET, PAF_MID_SIZE,
		e_ctrl->cal_data.mapdata, "rear_f2_paf_cal_data_mid",
		rear_f2_paf_cal_data_mid, (uint32_t) sizeof(rear_f2_paf_cal_data_mid));

	cam_sec_eeprom_module_info_set_paf(config_info, ADDR_M1_PAF,
		PAF_FAR_OFFSET, PAF_FAR_SIZE,
		e_ctrl->cal_data.mapdata, "rear_f2_paf_cal_data_far",
		rear_f2_paf_cal_data_far, (uint32_t) sizeof(rear_f2_paf_cal_data_far));

	if (is_valid_index(config_info, ADDR_M1_PAF, &config_addr) == 1)
	{
		config_addr += PAF_CAL_ERR_CHECK_OFFSET;
		memcpy(&f2_paf_err_data_result, &e_ctrl->cal_data.mapdata[config_addr], 4);
	}

	return 0;
}

int cam_sec_eeprom_update_paf_err(
	struct cam_eeprom_ctrl_t *e_ctrl, struct config_info_t *config_info)
{
	uint32_t config_addr = 0;
	uint32_t config_idx = ADDR_M0_PAF;
	int index = map_sensor_id_to_sysfs_index(e_ctrl->soc_info.index);

	if (index < 0 || index >= INDEX_MAX)
		return -EINVAL;

	if (e_ctrl->soc_info.index == SEC_TELE_SENSOR ||
		e_ctrl->soc_info.index == SEC_TELE2_SENSOR)
		config_idx = ADDR_S0_PAF;

	if (!is_valid_index(config_info, config_idx, &config_addr)) {
		CAM_ERR(CAM_EEPROM, "Invalid argument");
		return -EINVAL;
	}

	config_addr += PAF_CAL_ERR_CHECK_OFFSET;
	memcpy(&paf_err_data_result[index],
		&e_ctrl->cal_data.mapdata[config_addr], 4);

	return 0;
}


#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || \
	defined(CONFIG_SAMSUNG_LPAI_OIS)
int cam_sec_eeprom_update_ois_info(
	struct cam_eeprom_ctrl_t *e_ctrl, struct config_info_t *config_info)
{
	uint32_t config_addr = 0;

	int index = map_sensor_id_to_sysfs_index(e_ctrl->soc_info.index);

	if (index < 0 || index >= INDEX_MAX)
		return -EINVAL;

	config_addr = ADDR_M_OIS;
	if (e_ctrl->soc_info.index == SEC_TELE_SENSOR ||
		e_ctrl->soc_info.index == SEC_TELE2_SENSOR)
		config_addr = ADDR_S_OIS;

	if (is_valid_index(config_info, config_addr, &config_addr) == 1) {
		uint8_t* cal_mark = &ois_cal_mark[index];
		int* gain_result = &ois_gain_result[index];
		int* sr_result = &ois_sr_result[index];
		uint8_t *xygg = ois_xygg[index];
		uint8_t *xysr = ois_xysr[index];

		config_addr += OIS_CAL_MARK_START_OFFSET;
		memcpy(cal_mark, &e_ctrl->cal_data.mapdata[config_addr], 1);
		config_addr -= OIS_CAL_MARK_START_OFFSET;
		*gain_result = ((*cal_mark) == 0xBB) ? 0 : 1;
		*sr_result = ((*cal_mark) == 0xBB) ? 0 : 1;

		config_addr += OIS_XYGG_START_OFFSET;
		memcpy(xygg, &e_ctrl->cal_data.mapdata[config_addr], OIS_XYGG_SIZE);
		config_addr -= OIS_XYGG_START_OFFSET;

		config_addr += OIS_XYSR_START_OFFSET;
		memcpy(xysr, &e_ctrl->cal_data.mapdata[config_addr], OIS_XYSR_SIZE);
		config_addr -= OIS_XYSR_START_OFFSET;
	}

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE) || \
	defined(CONFIG_SAMSUNG_REAR_QUADRA)
	if (is_valid_index(config_info, config_addr, &config_addr) == 1) {
		int isCal = 0, j = 0;
		uint8_t *cross_talk = ois_cross_talk[index];
		int* cross_talk_result = &ois_cross_talk_result[index];

		config_addr += OIS_CROSSTALK_START_OFFSET;
		memcpy(cross_talk, &e_ctrl->cal_data.mapdata[config_addr], OIS_CROSSTALK_SIZE);
		config_addr -= OIS_CROSSTALK_START_OFFSET;
		*cross_talk_result = 0;
		for (j = 0; j < OIS_CROSSTALK_SIZE; j++) {
			if (cross_talk[j] != 0xFF) {
				isCal = 1;
				break;
			}
		}
		*cross_talk_result = (isCal == 0) ? 1 : 0;
	}
#endif

	return 0;
}
#endif

int cam_sec_eeprom_reset_module_info(struct cam_eeprom_ctrl_t *e_ctrl)
{
	int index = map_sensor_id_to_sysfs_index(e_ctrl->soc_info.index);
	if (index < 0 || index >= INDEX_MAX) {
		CAM_ERR(CAM_EEPROM, "Invalid Camera ID!");
		return -EINVAL;
	}

	strlcpy(module_id[index], "NULL", FROM_MODULE_ID_SIZE);

	return 0;
}

int cam_sec_eeprom_update_module_info(
	struct cam_eeprom_ctrl_t *e_ctrl,
	struct module_info_t *m_info,
	struct config_info_t *config_info)
{
	int rc = 0;

	CAM_INFO(CAM_EEPROM, "E");

	if (!e_ctrl) {
		CAM_ERR(CAM_EEPROM, "e_ctrl is NULL");
		return -EINVAL;
	}

	if (e_ctrl->soc_info.index >= SEC_SENSOR_ID_MAX) {
		CAM_ERR(CAM_EEPROM, "subdev_id: %d is not supported",
			e_ctrl->soc_info.index);
		return 0;
	}

	// AF
	cam_sec_eeprom_update_afcal(e_ctrl, config_info);

	// MTF
	cam_sec_eeprom_update_mtf_exif(e_ctrl, config_info);
	cam_sec_eeprom_update_mtf2_exif(e_ctrl, config_info);

	// PAF
	cam_sec_eeprom_update_paf(e_ctrl, config_info);
	cam_sec_eeprom_update_paf_err(e_ctrl, config_info);

	// OIS
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || \
	defined(CONFIG_SAMSUNG_LPAI_OIS)
	cam_sec_eeprom_update_ois_info(e_ctrl, config_info);
#endif

	cam_sec_eeprom_update_def_info(m_info, config_info);
	cam_sec_eeprom_module_info_set_load_version(
		e_ctrl, m_info, config_info);
	rc = cam_sec_eeprom_check_firmware_cal(e_ctrl, m_info);

#if defined(CONFIG_SAMSUNG_WACOM_NOTIFIER)
	// Update for each module
	if (1 == (e_ctrl->is_supported & 0x1))
	{
		is_eeprom_info_update(e_ctrl->soc_info.index,
			m_info->module_version.module_fw_ver);
	}

	// Probe Timing different for each model
#if defined(CONFIG_SAMSUNG_REAR_QUADRA)
	if (SEC_TELE2_SENSOR == e_ctrl->soc_info.index)
	{
		is_eeprom_wacom_update_notifier();
	}
#endif
#endif	/* CONFIG_SAMSUNG_WACOM_NOTIFIER */

	return rc;
}

void cam_sec_eeprom_update_sysfs_fw_version(
	const char *update_fw_ver,
	enum fw_ver_index update_fw_index,
	struct module_info_t *m_info)
{
	struct module_version_t *module_version =
		&m_info->module_version;
	char *fw_ver = NULL;

	if (update_fw_index == EEPROM_FW_VER)
		fw_ver = module_version->module_fw_ver;
	else if (update_fw_index == PHONE_FW_VER)
		fw_ver = module_version->phone_fw_ver;
	else
		fw_ver = module_version->load_fw_ver;
	strlcpy(fw_ver, update_fw_ver, FROM_MODULE_FW_INFO_SIZE + 1);

	sprintf(module_version->cam_fw_ver, "%s %s\n",
		module_version->module_fw_ver,
		module_version->load_fw_ver);
	sprintf(module_version->cam_fw_full_ver, "%s %s %s\n",
		module_version->module_fw_ver,
		module_version->phone_fw_ver,
		module_version->load_fw_ver);

	CAM_INFO(CAM_EEPROM, "camera_idx: %d, cam_fw_full_ver: %s",
		m_info->type, module_version->cam_fw_full_ver);
}

int32_t cam_sec_eeprom_check_firmware_cal(
	struct cam_eeprom_ctrl_t *e_ctrl, struct module_info_t *m_info)
{
	int rc = 0, offset = 0, cnt = 0;
	char final_cmd_ack[SYSFS_FW_VER_SIZE] = "NG_";
	char cam_cal_ack[SYSFS_FW_VER_SIZE] = "NULL";

	uint8_t need_update = TRUE;
	uint8_t version_isp = 0, version_module_maker_ver = 0;
	uint8_t valid_eeprom_data = TRUE;
	uint8_t is_QC_module = TRUE;
	uint8_t camera_cal_ack = OK;
	uint8_t camera_fw_ack = OK;
	uint8_t module_ver_on_pvr = 0;
	uint8_t module_ver_on_sra = 0;
	uint8_t min_cal_map_ver = 0;
	uint32_t camera_cal_crc = e_ctrl->is_supported;
	uint32_t camera_normal_cal_crc = e_ctrl->camera_normal_cal_crc;
	struct module_version_t *module_version = NULL;

	if ((m_info == NULL) ||
		(m_info->module_version.cam_fw_ver == NULL))
	{
		CAM_ERR(CAM_EEPROM, "invalid argument");
		rc = 0;
		return rc;
	}

	module_version = &m_info->module_version;
	module_ver_on_pvr = module_version->phone_chk_ver_info[1];
	module_ver_on_sra = module_version->phone_chk_ver_info[2];
	min_cal_map_ver = module_version->phone_chk_ver_info[3];

	version_isp = module_version->cam_fw_ver[3];
	version_module_maker_ver = module_version->cam_fw_ver[10];

	if (version_isp == 0xff || version_module_maker_ver == 0xff) {
		CAM_ERR(CAM_EEPROM, "invalid eeprom data");
		valid_eeprom_data = FALSE;
		cam_sec_eeprom_update_sysfs_fw_version("NULL", EEPROM_FW_VER, m_info);
	}

	/* 1. check camera firmware and cal data */
	CAM_INFO(CAM_EEPROM, "camera_cal_crc: 0x%x", camera_cal_crc);

	if (camera_cal_crc == camera_normal_cal_crc) {
		camera_cal_ack = OK;
		strncpy(cam_cal_ack, "Normal", SYSFS_FW_VER_SIZE);
	} else {
		camera_cal_ack = CRASH;
		strncpy(cam_cal_ack, "Abnormal", SYSFS_FW_VER_SIZE);

		offset = strlen(final_cmd_ack);
		if (m_info->type == SEC_WIDE_SENSOR) {
			camera_cal_ack = CRASH;
			strncpy(cam_cal_ack, "Abnormal", SYSFS_FW_VER_SIZE);
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
			if ((camera_cal_crc & CAMERA_CAL_CRC_WIDE) != CAMERA_CAL_CRC_WIDE) {
				cnt = scnprintf(&final_cmd_ack[offset], SYSFS_FW_VER_SIZE-offset, "%s", "CD");
				offset += cnt;
			} else {
				cnt = scnprintf(&final_cmd_ack[offset], SYSFS_FW_VER_SIZE-offset, "%s", "CD4");
				offset += cnt;
			}
#else
			cnt = scnprintf(&final_cmd_ack[offset], SYSFS_FW_VER_SIZE-offset, "%s", "CD");
			offset += cnt;
#endif
		} else {
			camera_cal_ack = CRASH;
			strncpy(cam_cal_ack, "Abnormal", SYSFS_FW_VER_SIZE);
			cnt = scnprintf(&final_cmd_ack[offset], SYSFS_FW_VER_SIZE-offset, "%s", "CD3");
			offset += cnt;
		}
		final_cmd_ack[offset] = '\0';

		switch(m_info->type)
		{
			case SEC_FRONT_SENSOR:
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
			case SEC_FRONT_TOP_SENSOR:
#endif
#if defined(UNUSE_FRONT_EEPROM)
				strncpy(final_cmd_ack, "NG_", 3);
				strncpy(cam_cal_ack, "NULL", SYSFS_FW_VER_SIZE);
				camera_cal_ack = OK;
#endif
				break;

			default:
				break;
		}
	}

	/* 3-1. all success case: display LOAD FW */
	if (camera_fw_ack && camera_cal_ack)
		need_update = FALSE;

	/* 3-2. fail case: update CMD_ACK on sysfs (load fw) */
	// If not QC module, return NG.
	if (version_isp >= 0x80 || !isalnum(version_isp))
		CAM_INFO(CAM_EEPROM, "ISP Ver : 0x%x", version_isp);
	else
		CAM_INFO(CAM_EEPROM, "ISP Ver : %c", version_isp);

	if (version_isp != 'Q' && version_isp != 'U' &&
		version_isp != 'A' && version_isp != 'X' && version_isp != 'E') {
		CAM_ERR(CAM_EEPROM, "This is not Qualcomm module!");

		if (m_info->type == SEC_WIDE_SENSOR) {
			strncpy(final_cmd_ack, "NG_FWCD", SYSFS_FW_VER_SIZE);
			strncpy(cam_cal_ack, "Abnormal", SYSFS_FW_VER_SIZE);
		} else {
			strncpy(final_cmd_ack, "NG_CD3_L", SYSFS_FW_VER_SIZE);
			strncpy(cam_cal_ack, "Abnormal", SYSFS_FW_VER_SIZE);
		}

		need_update = TRUE;
		is_QC_module = FALSE;
		camera_cal_ack = CRASH;
	}

	if (need_update) {
		CAM_ERR(CAM_EEPROM, "final_cmd_ack : %s", final_cmd_ack);
		cam_sec_eeprom_update_sysfs_fw_version(
			final_cmd_ack, LOAD_FW_VER, m_info);
	} else {
		// just display success fw version log
		CAM_INFO(CAM_EEPROM, "final_cmd_ack : %s", final_cmd_ack);
		memset(final_cmd_ack, 0, sizeof(final_cmd_ack));
		strncpy(final_cmd_ack, module_version->cam_fw_full_ver, SYSFS_FW_VER_SIZE);
		final_cmd_ack[SYSFS_FW_VER_SIZE-1] = '\0';

		CAM_INFO(CAM_EEPROM, "final_cmd_ack : %s", final_cmd_ack);
	}

	/* 4. update CAL check ack on sysfs rear_calcheck */
	strlcpy(module_version->cam_cal_ack, cam_cal_ack, SYSFS_FW_VER_SIZE);
	snprintf(cal_crc, SYSFS_FW_VER_SIZE, "%s %s\n",
		cam_cal_check[INDEX_REAR], cam_cal_check[INDEX_FRONT]);

	CAM_INFO(CAM_EEPROM,
		"version_module_maker: 0x%x, MODULE_VER_ON_PVR: 0x%x, MODULE_VER_ON_SRA: 0x%x",
		version_module_maker_ver, module_ver_on_pvr, module_ver_on_sra);
	CAM_INFO(CAM_EEPROM,
		"cal_map_version: 0x%x vs FROM_CAL_MAP_VERSION: 0x%x",
		m_info->map_version, min_cal_map_ver);

	if ((is_QC_module == TRUE) &&
		((valid_eeprom_data == FALSE) ||
		(m_info->map_version < min_cal_map_ver) ||
		(version_module_maker_ver < module_ver_on_pvr))) {
		strncpy(module_version->fw_user_ver, "NG", SYSFS_FW_VER_SIZE);
	} else {
		if (camera_cal_ack == CRASH)
			strncpy(module_version->fw_user_ver, "NG", SYSFS_FW_VER_SIZE);
		else
			strncpy(module_version->fw_user_ver, "OK", SYSFS_FW_VER_SIZE);
	}

	if ((is_QC_module == TRUE) &&
		((valid_eeprom_data == FALSE) ||
		(m_info->map_version < min_cal_map_ver)
		|| (version_module_maker_ver < module_ver_on_sra))) {
		strncpy(module_version->fw_factory_ver, "NG_VER", SYSFS_FW_VER_SIZE);
	} else {
		if (camera_cal_ack == CRASH) {
			if (m_info->type == SEC_WIDE_SENSOR) {
				strncpy(module_version->fw_factory_ver, "NG_VER", SYSFS_FW_VER_SIZE);
			} else {
				strncpy(module_version->fw_factory_ver, "NG_CRC", SYSFS_FW_VER_SIZE);

				if (m_info->type == SEC_FRONT_SENSOR ||
					m_info->type == SEC_ULTRA_WIDE_SENSOR) // TEMP_8550
					strncpy(module_version->fw_factory_ver, "OK", SYSFS_FW_VER_SIZE);
			}
		}
		else {
			strncpy(module_version->fw_factory_ver, "OK", SYSFS_FW_VER_SIZE);
		}
	}

	return rc;
}

/**
 * cam_sec_eeprom_verify_sum - verify crc32 checksum
 * @mem:			data buffer
 * @size:			size of data buffer
 * @sum:			expected checksum
 * @rev_endian:	compare reversed endian (0:little, 1:big)
 *
 * Returns 0 if checksum match, -EINVAL otherwise.
 */
int cam_sec_eeprom_verify_sum(
	const char *mem, uint32_t size,
	uint32_t sum, uint32_t rev_endian)
{
	uint32_t crc = ~0;
	uint32_t cmp_crc = 0;

	/* check overflow */
	if (size > crc - sizeof(uint32_t))
		return -EINVAL;

	crc = crc32_le(crc, mem, size);

	crc = ~crc;
	cmp_crc = crc;
	if (rev_endian == 1) {
		cmp_crc = (((crc) & 0xFF) << 24)
				| (((crc) & 0xFF00) << 8)
				| (((crc) >> 8) & 0xFF00)
				| ((crc) >> 24);
	}
	CAM_DBG(CAM_EEPROM, "endian %d, expect 0x%x, result 0x%x",
		rev_endian, sum, cmp_crc);

	if (cmp_crc != sum) {
		CAM_ERR(CAM_EEPROM, "endian %d, expect 0x%x, result 0x%x",
			rev_endian, sum, cmp_crc);
		return -EINVAL;
	}

	CAM_DBG(CAM_EEPROM, "checksum pass 0x%x", sum);
	return 0;
}

/**
 * cam_sec_eeprom_match_crc - verify multiple regions using crc
 * @data:	data block to be verified
 *
 * Iterates through all regions stored in @data.  Regions with odd index
 * are treated as data, and its next region is treated as checksum.  Thus
 * regions of even index must have valid_size of 4 or 0 (skip verification).
 * Returns a bitmask of verified regions, starting from LSB.  1 indicates
 * a checksum match, while 0 indicates checksum mismatch or not verified.
 */
uint32_t cam_sec_eeprom_match_crc(
	struct cam_eeprom_memory_block_t *data,
	uint32_t subdev_id,
	struct config_info_t *config_info)
{
	int j, rc;
	uint32_t *sum;
	uint32_t ret = 0;
	uint8_t *memptr, *memptr_crc;
	struct cam_eeprom_memory_map_t *map;
	uint8_t map_ver = 0;
	uint32_t config_addr = 0;

	if (!data) {
		CAM_ERR(CAM_EEPROM, "data is NULL");
		return 0;
	}

	if (subdev_id >= SEC_SENSOR_ID_MAX) {
		CAM_INFO(CAM_EEPROM, "subdev_id: %d is not supported", subdev_id);
		return 0;
	}

	map = data->map;
	if (is_valid_index(config_info, ADDR_M_CALMAP_VER, &config_addr) == 1) {
		config_addr += 0x03;
		map_ver = data->mapdata[config_addr];
	} else {
		CAM_INFO(CAM_EEPROM, "ADDR_M_CALMAP_VER is not set: %d", subdev_id);
	}

	if (map_ver >= 0x80 || !isalnum(map_ver))
		CAM_INFO(CAM_EEPROM, "map subdev_id = %d, version = 0x%x",
			subdev_id, map_ver);
	else
		CAM_INFO(CAM_EEPROM, "map subdev_id = %d, version = %c [0x%x]",
			subdev_id, map_ver, map_ver);

	//  idx 0 is the actual reading section (whole data)
	//  from idx 1, start to compare CRC checksum
	//  (1: CRC area for header, 2: CRC value)
	for (j = 1; j + 1 < data->num_map; j += 2) {
		memptr = data->mapdata + map[j].mem.addr;
		memptr_crc = data->mapdata + map[j+1].mem.addr;

		/* empty table or no checksum */
		if (!map[j].mem.valid_size || !map[j+1].mem.valid_size) {
			CAM_ERR(CAM_EEPROM, "continue");
			continue;
		}

		if (map[j+1].mem.valid_size < sizeof(uint32_t)) {
			CAM_ERR(CAM_EEPROM, "[%d : size 0x%X] malformatted data mapping",
				j+1, map[j+1].mem.valid_size);
			return 0;
		}
		CAM_DBG(CAM_EEPROM, "[%d] memptr 0x%x, memptr_crc 0x%x",
			j, map[j].mem.addr, map[j + 1].mem.addr);
		sum = (uint32_t *) (memptr_crc + map[j+1].mem.valid_size - sizeof(uint32_t));
		rc = cam_sec_eeprom_verify_sum(memptr, map[j].mem.valid_size, *sum, 0);

		if (!rc)
			ret |= 1 << (j/2);
	}

	CAM_INFO(CAM_EEPROM, "CRC result = 0x%08X", ret);

	return ret;
}

/**
 * cam_sec_eeprom_calc_calmap_size - Calculate cal array size based on the cal map
 * @e_ctrl:       ctrl structure
 *
 * Returns size of cal array
 */
int32_t cam_sec_eeprom_calc_calmap_size(struct cam_eeprom_ctrl_t *e_ctrl)
{
	struct cam_eeprom_memory_map_t *map = NULL;
	uint32_t min_map, max_map, min_local, max_local;
	int32_t i;
	int32_t calmap_size = 0;

	if (e_ctrl == NULL ||
		(e_ctrl->cal_data.num_map == 0) ||
		(e_ctrl->cal_data.map == NULL)) {
		CAM_INFO(CAM_EEPROM, "Invalid argument");
		return calmap_size;
	}

	map = e_ctrl->cal_data.map;
	min_map = min_local = 0xFFFFFFFF;
	max_map = max_local = 0x00;

	for (i = 0; i < e_ctrl->cal_data.num_map; i++) {
		min_local = map[i].mem.addr;
		max_local = min_local + map[i].mem.valid_size;

		min_map = min_map > min_local ? min_local : min_map;
		max_map = max_map < max_local ? max_local : max_map;

		CAM_DBG(CAM_EEPROM, "[%d / %d] min_local = 0x%X, min_map = 0x%X, max_local = 0x%X, max_map = 0x%X",
			i+1, e_ctrl->cal_data.num_map, min_local, min_map, max_local, max_map);
	}
	calmap_size = max_map - min_map;

	CAM_INFO(CAM_EEPROM, "calmap_size = 0x%X, min_map = 0x%X, max_map = 0x%X",
		calmap_size, min_map, max_map);

	return calmap_size;
}

int32_t cam_sec_eeprom_fill_config_info(
	struct config_info_t *config_info,
	char *config_string, uint32_t value)
{
	int32_t i, ret = 1;

	for(i = 0; i < MAX_CONFIG_INFO_IDX; i ++)
	{
		if (config_info[i].is_set == 1 ||
			strcmp(config_string, config_info_strs[i]) != 0)
			continue;

		config_info[i].is_set = 1;
		config_info[i].value = value;
		ret = 0;
	}

	return ret;
}

/**
 * cam_sec_eeprom_get_customInfo - parse the userspace IO config and
 *                            read phone version at eebindriver.bin
 * @e_ctrl:     ctrl structure
 * @csl_packet: csl packet received
 *
 * Returns success or failure
 */
int32_t cam_sec_eeprom_get_custom_info(
	struct cam_eeprom_ctrl_t *e_ctrl,
	struct cam_packet *csl_packet,
	struct config_info_t* config_info)
{
	struct cam_buf_io_cfg *io_cfg;
	uint32_t              i = 0;
	int                   rc = 0;
	uintptr_t             buf_addr;
	size_t                buf_size = 0;
	uint8_t               *read_buffer;

	uint8_t               *pBuf = NULL;
	uint32_t              nConfig = 0;
	char                  *config_name = "CustomInfo";

	char                  config_string[MAX_CUSTOM_STRING_LENGTH] = "";
	uint32_t              config_value = 0;

	io_cfg = (struct cam_buf_io_cfg *) ((uint8_t *)
		&csl_packet->payload +
		csl_packet->io_configs_offset);

	CAM_DBG(CAM_EEPROM, "number of IO configs: %d:",
		csl_packet->num_io_configs);

	for (i = 0; i < csl_packet->num_io_configs; i++) {
		CAM_DBG(CAM_EEPROM, "Direction: %d:", io_cfg->direction);
		if (io_cfg->direction != CAM_BUF_OUTPUT) {
			CAM_ERR(CAM_EEPROM, "Invalid direction");
			rc = -EINVAL;
			continue;
		}

		rc = cam_mem_get_cpu_buf(io_cfg->mem_handle[0],
			&buf_addr, &buf_size);
		CAM_DBG(CAM_EEPROM, "buf_addr : %pK, buf_size : %zu",
			(void *)buf_addr, buf_size);

		read_buffer = (uint8_t *)buf_addr;
		if (!read_buffer) {
			CAM_ERR(CAM_EEPROM,
				"invalid buffer to copy data");
			return -EINVAL;
		}
		read_buffer += io_cfg->offsets[0];

		if (buf_size < e_ctrl->cal_data.num_data) {
			CAM_ERR(CAM_EEPROM,
				"failed to copy, Invalid size");
			return -EINVAL;
		}

		CAM_DBG(CAM_EEPROM,
			"copy the data, len:%d, read_buffer[0] = %d, read_buffer[4] = %d",
			e_ctrl->cal_data.num_data, read_buffer[0], read_buffer[4]);

		pBuf = read_buffer;
		if (strcmp(pBuf, config_name) == 0) {
			pBuf += strlen(config_name)+1+sizeof(uint32_t);

			memcpy(&nConfig, pBuf, sizeof(uint32_t));
			pBuf += sizeof(uint32_t);

			CAM_INFO(CAM_EEPROM, "nConfig: %d", nConfig);
			for(i = 0; i < nConfig; i ++) {
				memcpy(config_string, pBuf, MAX_CUSTOM_STRING_LENGTH);
				pBuf += MAX_CUSTOM_STRING_LENGTH;

				memcpy(&config_value, pBuf, sizeof(uint32_t));
				pBuf += sizeof(uint32_t);
				CAM_DBG(CAM_EEPROM, "config_info[%d] = %s	 0x%04X",
					i, config_string, config_value);
				cam_sec_eeprom_fill_config_info(
					config_info, config_string, config_value);
			}
		}

		for(i = 0; i < MAX_CONFIG_INFO_IDX; i ++)
		{
			if (config_info[i].is_set == 1)
			{
				CAM_DBG(CAM_EEPROM, "config_info[%d] (%d) = %s	  0x%04X",
					i, config_info[i].is_set, config_info_strs[i],
					config_info[i].value);
			}
		}

		memset(read_buffer, 0x00, e_ctrl->cal_data.num_data);
	}

	return rc;
}

