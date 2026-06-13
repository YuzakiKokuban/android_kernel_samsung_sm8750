// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "cam_sec_eeprom_core_test.h"
#include <linux/random.h>
#include <linux/crc32.h>

static void set_config(
	struct config_info_t *config_info,
	enum config_name_info_index config_index,
	uint32_t value)
{
	config_info[config_index].is_set = 1;
	config_info[config_index].value = value;
}

static void is_valid_index_test_success(struct kunit *test)
{
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	uint32_t config_addr = 0, dummy_addr = 1129;
	int rc = 0;

	memset(config_info, 0, sizeof(config_info));

	set_config(config_info, DEF_M_VER_HW, dummy_addr);

	rc = is_valid_index(config_info, DEF_M_VER_HW, &config_addr);

	KUNIT_EXPECT_EQ(test, 1, rc);
	KUNIT_EXPECT_EQ(test, dummy_addr, config_addr);
}

static void is_valid_index_test_failure(struct kunit *test)
{
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	uint32_t config_addr = 0;
	int rc = 0;

	memset(config_info, 0, sizeof(config_info));

	// config_index >= MAX_CONFIG_INFO_IDX
	rc = is_valid_index(config_info, MAX_CONFIG_INFO_IDX + 1, &config_addr);
	KUNIT_EXPECT_EQ(test, 0, rc);

	// config_info[config_index].is_set == 0
	rc = is_valid_index(config_info, DEF_M_VER_HW, &config_addr);
	KUNIT_EXPECT_EQ(test, 0, rc);
	KUNIT_EXPECT_EQ(test, 0, config_addr);
}

static void map_sensor_id_to_sysfs_index_test_success(struct kunit *test)
{
	int index = -1;
	index = map_sensor_id_to_sysfs_index(SEC_WIDE_SENSOR);
	KUNIT_EXPECT_EQ(test, INDEX_REAR, index);

	index = map_sensor_id_to_sysfs_index(SEC_ULTRA_WIDE_SENSOR);
	KUNIT_EXPECT_EQ(test, INDEX_REAR2, index);

	index = map_sensor_id_to_sysfs_index(SEC_FRONT_SENSOR);
	KUNIT_EXPECT_EQ(test, INDEX_FRONT, index);

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	index = map_sensor_id_to_sysfs_index(SEC_TELE_SENSOR);
	KUNIT_EXPECT_EQ(test, INDEX_REAR3, index);
#endif

#if defined(CONFIG_SAMSUNG_REAR_QUADRA)
	 index = map_sensor_id_to_sysfs_index(SEC_TELE2_SENSOR);
	 KUNIT_EXPECT_EQ(test, INDEX_REAR4, index);
#endif

#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
	index = map_sensor_id_to_sysfs_index(SEC_FRONT_AUX1_SENSOR);
	KUNIT_EXPECT_EQ(test, INDEX_FRONT2, index);
#endif

#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
	index = map_sensor_id_to_sysfs_index(SEC_FRONT_TOP_SENSOR)
	KUNIT_EXPECT_EQ(test, INDEX_FRONT3, index);
#else
	index = map_sensor_id_to_sysfs_index(SEC_FRONT_TOP_SENSOR);
	KUNIT_EXPECT_EQ(test, INDEX_FRONT2, index);
#endif
#endif

	index = map_sensor_id_to_sysfs_index(SEC_SENSOR_ID_MAX);
	KUNIT_EXPECT_EQ(test, -EINVAL, index);
}

static void map_sensor_id_to_type_str_test_success(struct kunit *test)
{
	char type_str[FROM_MODULE_FW_INFO_SIZE] = "";

	strlcpy(type_str, "", FROM_MODULE_FW_INFO_SIZE);
	map_sensor_id_to_type_str(SEC_WIDE_SENSOR, type_str);
	KUNIT_EXPECT_STREQ(test, "Rear", type_str);

	strlcpy(type_str, "", FROM_MODULE_FW_INFO_SIZE);
	map_sensor_id_to_type_str(SEC_ULTRA_WIDE_SENSOR, type_str);
	KUNIT_EXPECT_STREQ(test, "Rear2", type_str);

	strlcpy(type_str, "", FROM_MODULE_FW_INFO_SIZE);
	map_sensor_id_to_type_str(SEC_FRONT_SENSOR, type_str);
	KUNIT_EXPECT_STREQ(test, "Front", type_str);


#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	strlcpy(type_str, "", FROM_MODULE_FW_INFO_SIZE);
	map_sensor_id_to_type_str(SEC_TELE_SENSOR, type_str);
	KUNIT_EXPECT_STREQ(test, "Rear3", type_str);
#endif

#if defined(CONFIG_SAMSUNG_REAR_QUADRA)
	strlcpy(type_str, "", FROM_MODULE_FW_INFO_SIZE);
	map_sensor_id_to_type_str(SEC_TELE2_SENSOR, type_str);
	KUNIT_EXPECT_STREQ(test, "Rear4", type_str);
#endif

#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
	strlcpy(type_str, "", FROM_MODULE_FW_INFO_SIZE);
	map_sensor_id_to_type_str(SEC_FRONT_AUX1_SENSOR, type_str);
	KUNIT_EXPECT_STREQ(test, "Front2", type_str);
#endif

#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
	strlcpy(type_str, "", FROM_MODULE_FW_INFO_SIZE);
	map_sensor_id_to_type_str(SEC_FRONT_TOP_SENSOR, type_str);
	KUNIT_EXPECT_STREQ(test, "Front3", type_str);
#else
	strlcpy(type_str, "", FROM_MODULE_FW_INFO_SIZE);
	map_sensor_id_to_type_str(SEC_FRONT_TOP_SENSOR, type_str);
	KUNIT_EXPECT_STREQ(test, "Front2", type_str);
#endif
#endif

	strlcpy(type_str, "", FROM_MODULE_FW_INFO_SIZE);
	map_sensor_id_to_type_str(SEC_SENSOR_ID_MAX, type_str);
	KUNIT_EXPECT_STREQ(test, "", type_str);
}

static void uint_to_char_array_test_success(struct kunit *test)
{
	int i = 0, j = 0;
	uint32_t dummy_num = 0;
	char dummy_arr[4] = { 0, };

	for (i = 1; i < ARRAY_SIZE(dummy_arr); i++) {
		get_random_bytes((void *)&dummy_num, i);
		uint_to_char_array(dummy_num, dummy_arr, i);

		for (j = 0; j < i; j++)
			KUNIT_EXPECT_EQ(test, dummy_arr[j], (dummy_num >> (i - 1 - j) * 8) & 0xFF);
	}
}

static void sensor_id_test_success(struct kunit *test)
{
	struct cam_eeprom_ctrl_t e_ctrl;
	struct module_info_t m_info;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	int rc = 0;
	uint32_t config_idx = ADDR_M_SENSOR_ID;
	uint32_t config_addr = 0x20;
	uint8_t map_data[1024] = { 0, };
	char sensor_ver[2] = { 0, };
	char out_sensor_id[FROM_SENSOR_ID_SIZE + 1] = { 0, };
	uint8_t dummy_sensor_id[FROM_SENSOR_ID_SIZE + 1] = { 0, };

	memset(&e_ctrl, 0, sizeof(e_ctrl));
	memset(&m_info, 0, sizeof(m_info));
	memset(config_info, 0, sizeof(config_info));

	get_random_bytes(dummy_sensor_id, FROM_SENSOR_ID_SIZE);
	dummy_sensor_id[FROM_SENSOR_ID_SIZE] = '\0';

	e_ctrl.cal_data.mapdata = map_data;

	m_info.module_version.sensor_id = out_sensor_id;
	m_info.module_version.sensor2_id = out_sensor_id;

	set_config(config_info, config_idx, config_addr);
	memcpy(&map_data[config_addr], dummy_sensor_id, FROM_SENSOR_ID_SIZE + 1);

	rc = cam_sec_eeprom_module_info_set_sensor_id(
		&e_ctrl, &m_info, config_info, sensor_ver);

	KUNIT_EXPECT_EQ(test, 0, rc);
	KUNIT_EXPECT_STREQ(test, dummy_sensor_id, out_sensor_id);
	KUNIT_EXPECT_EQ(test, sensor_ver[0], dummy_sensor_id[8]);
}

static void sensor_id_test_failure(struct kunit *test)
{
	struct cam_eeprom_ctrl_t e_ctrl;
	struct module_info_t m_info;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	int rc = 0;
	uint32_t config_idx = ADDR_M_SENSOR_ID;
	uint8_t map_data[1024] = { 0, };
	char sensor_ver[2] = { 0, };
	char out_sensor_id[FROM_SENSOR_ID_SIZE + 1] = { 0, };

	memset(&e_ctrl, 0, sizeof(e_ctrl));
	memset(&m_info, 0, sizeof(m_info));
	memset(config_info, 0, sizeof(config_info));

	// e_ctrl == NULL
	rc = cam_sec_eeprom_module_info_set_sensor_id(
		NULL, &m_info, config_info, sensor_ver);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	// e_ctrl.cal_data.mapdata == NULL
	rc = cam_sec_eeprom_module_info_set_sensor_id(
		&e_ctrl, &m_info, config_info, sensor_ver);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	e_ctrl.cal_data.mapdata = map_data;

	// m_info == NULL
	rc = cam_sec_eeprom_module_info_set_sensor_id(
		&e_ctrl, NULL, config_info, sensor_ver);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	m_info.module_version.sensor2_id = out_sensor_id;

	// m_info.module_version.sensor_id == NULL
	rc = cam_sec_eeprom_module_info_set_sensor_id(
		&e_ctrl, &m_info, config_info, sensor_ver);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	m_info.module_version.sensor_id = out_sensor_id;
	m_info.module_version.sensor2_id = NULL;

	// m_info.module_version.sensor2_id == NULL
	rc = cam_sec_eeprom_module_info_set_sensor_id(
		&e_ctrl, &m_info, config_info, sensor_ver);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	m_info.module_version.sensor2_id = out_sensor_id;

	// config_info == NULL
	rc = cam_sec_eeprom_module_info_set_sensor_id(
		&e_ctrl, &m_info, NULL, sensor_ver);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	config_info[config_idx].is_set = 0;

	// is_valid_index failure
	rc = cam_sec_eeprom_module_info_set_sensor_id(
		&e_ctrl, &m_info, config_info, sensor_ver);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);
}

static void module_id_test_success(struct kunit *test)
{
	struct module_info_t m_info;
	int rc = 0;
	uint32_t config_addr = 0x20;
	uint8_t map_data[1024] = { 0, };
	uint8_t dummy_module_id[FROM_MODULE_ID_SIZE + 1] = { 0, };
	char out_module_id[FROM_MODULE_ID_SIZE + 1] = { 0, };

	memset(&m_info, 0, sizeof(m_info));

	get_random_bytes(dummy_module_id, FROM_MODULE_ID_SIZE);
	dummy_module_id[FROM_MODULE_ID_SIZE] = '\0';

	m_info.module_version.module_id = out_module_id;

	memcpy(&map_data[config_addr], dummy_module_id, FROM_MODULE_ID_SIZE + 1);

	rc = cam_sec_eeprom_module_info_set_module_id(&m_info,
		&map_data[config_addr]);

	KUNIT_EXPECT_EQ(test, 0, rc);
	KUNIT_EXPECT_STREQ(test, dummy_module_id, out_module_id);
}

static void module_id_test_failure(struct kunit *test)
{
	struct module_info_t m_info;
	int rc = 0;
	uint32_t config_addr = 0x20;
	uint8_t map_data[1024] = { 0, };
	char out_module_id[FROM_MODULE_ID_SIZE + 1] = { 0, };

	memset(&m_info, 0, sizeof(m_info));

	// m_info == NULL
	rc = cam_sec_eeprom_module_info_set_module_id(NULL,
		&map_data[config_addr]);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	// m_info->module_version.module_id == NULL
	rc = cam_sec_eeprom_module_info_set_module_id(&m_info,
		&map_data[config_addr]);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	m_info.module_version.module_id = out_module_id;

	// map_data == NULL
	rc = cam_sec_eeprom_module_info_set_module_id(&m_info,
		NULL);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);
}

static void set_paf_test_success(struct kunit *test)
{
	int rc = 0;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	uint32_t config_idx = ADDR_M0_PAF;
	uint32_t config_addr = 0x20;
	uint8_t map_data[2048] = { 0, };
	uint32_t mid_far_size = 0;
	uint32_t st_offset = 0x10;
	char paf_cal_data[PAF_2PD_CAL_INFO_SIZE] = {0,};
	uint8_t dummy_paf_cal_data[32] = {
		0x5D, 0xB3, 0x40, 0x7B, 0xCA, 0x99, 0x87, 0x26,
		0x2A, 0x75, 0xFC, 0xEA, 0x6A, 0xB1, 0x31, 0x99,
		0x06, 0x00, 0x2A, 0xAD, 0x8F, 0x06, 0x0A, 0xCD,
		0x1B, 0x61, 0xB7, 0x59, 0x2B, 0xB7, 0x1F, 0xB7,
	};
	const char *mid_str =
		"-19619,29994,6,24859,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
		"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
		"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
		"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n";

	const char *far_str =
		"-19619,31552,-26166,9863,29994,-5380,-20118,-26319,6,-21206,1679,-13046,"
		"24859,22967,-18645,-18657,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
		"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
		"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n";

	memset(config_info, 0, sizeof(config_info));

	set_config(config_info, config_idx, config_addr);

	memcpy(&map_data[config_addr + st_offset],
		dummy_paf_cal_data, sizeof(dummy_paf_cal_data));

	mid_far_size = PAF_MID_SIZE;
	rc = cam_sec_eeprom_module_info_set_paf(
		config_info, config_idx, st_offset,
		mid_far_size, map_data, "paf_test",
		paf_cal_data, sizeof(paf_cal_data));
	KUNIT_EXPECT_STREQ(test, mid_str, paf_cal_data);

	mid_far_size = PAF_FAR_SIZE;
	rc = cam_sec_eeprom_module_info_set_paf(
		config_info, config_idx, st_offset,
		mid_far_size, map_data, "paf_test",
		paf_cal_data, sizeof(paf_cal_data));
	KUNIT_EXPECT_STREQ(test, far_str, paf_cal_data);
}

static void set_paf_test_failure(struct kunit *test)
{
	int rc = 0;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	uint32_t config_idx = ADDR_M0_PAF;
	uint32_t config_addr = 0x20;
	uint8_t map_data[1024] = { 0, };
	uint32_t mid_far_size = PAF_MID_SIZE;
	uint32_t st_offset = 0x10;
	char paf_cal_data[PAF_2PD_CAL_INFO_SIZE] = {0,};

	memset(config_info, 0, sizeof(config_info));

	// is_valid_index false
	rc = cam_sec_eeprom_module_info_set_paf(
		config_info, config_idx, st_offset,
		mid_far_size, map_data, "paf_test",
		paf_cal_data, sizeof(paf_cal_data));
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	// size <= 1
	set_config(config_info, config_idx, config_addr);
	mid_far_size = 1;
	rc = cam_sec_eeprom_module_info_set_paf(
		config_info, config_idx, st_offset,
		mid_far_size, map_data, "paf_test",
		paf_cal_data, sizeof(paf_cal_data));
	KUNIT_EXPECT_EQ(test, 0, rc);
}

static void afcal_test_success(struct kunit *test)
{
	struct cam_eeprom_ctrl_t e_ctrl;
	struct module_info_t m_info;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	int rc = 0;
	uint32_t config_idx = ADDR_M_AF;
	uint32_t config_addr = 0x20;
	uint8_t map_data[1024] = { 0, };
	uint8_t dummy_afcal[MAX_AF_CAL_STR_SIZE] = { 0, };
	char dummy_afcal_str[MAX_AF_CAL_STR_SIZE] = "";
	int index = 0, i = 0, offset = 0, cnt = 0, len = 0;
	struct af_index_t af_idx[] = {
		{AF_CAL_NEAR_IDX, AF_CAL_NEAR_OFFSET_FROM_AF},
		{AF_CAL_FAR_IDX, AF_CAL_FAR_OFFSET_FROM_AF},
		{AF_CAL_M1_IDX, AF_CAL_M1_OFFSET_FROM_AF},
		{AF_CAL_M2_IDX, AF_CAL_M2_OFFSET_FROM_AF},
	};
	uint32_t num_idx = ARRAY_SIZE(af_idx);
	uint32_t tempval;

	memset(&e_ctrl, 0, sizeof(e_ctrl));
	memset(&m_info, 0, sizeof(m_info));
	memset(config_info, 0, sizeof(config_info));

	for (i = 0; i < SEC_SENSOR_ID_MAX; i++)
	{
		e_ctrl.soc_info.index = i;
		index = map_sensor_id_to_sysfs_index(e_ctrl.soc_info.index);
		if (index == -EINVAL)
			continue;

		config_idx = ADDR_M_AF;
		if (e_ctrl.soc_info.index == SEC_TELE_SENSOR ||
			e_ctrl.soc_info.index == SEC_TELE2_SENSOR)
			config_idx = ADDR_S0_AF;

		if (e_ctrl.soc_info.index == SEC_FRONT_SENSOR ||
			e_ctrl.soc_info.index == SEC_FRONT_TOP_SENSOR)
			num_idx = 2;

		memset(config_info, 0, sizeof(config_info));
		set_config(config_info, config_idx, config_addr);

		get_random_bytes(dummy_afcal, MAX_AF_CAL_STR_SIZE);

		memset(map_data, 0, sizeof(map_data));
		memcpy(&map_data[config_addr], dummy_afcal, MAX_AF_CAL_STR_SIZE);
		e_ctrl.cal_data.mapdata = map_data;

		for(i = 0; i < num_idx; i ++)
		{
			memcpy(&tempval, &map_data[config_addr + af_idx[i].offset], 4);

			cnt = scnprintf(&dummy_afcal_str[offset], MAX_AF_CAL_STR_SIZE - offset, "%d ", tempval);
			offset += cnt;
		}
		dummy_afcal_str[offset] = '\0';

		len = strlen(dummy_afcal_str);
		if (dummy_afcal_str[len-1] == ' ')
			dummy_afcal_str[len-1] = '\0';

		rc = cam_sec_eeprom_update_afcal(&e_ctrl, config_info);
		KUNIT_EXPECT_EQ(test, 0, rc);
		KUNIT_EXPECT_STREQ(test, af_cal_str[index], dummy_afcal_str);
	}
}

static void afcal_test_failure(struct kunit *test)
{
	struct cam_eeprom_ctrl_t e_ctrl;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	int rc = 0;
	uint32_t config_idx = ADDR_M_AF;

	memset(&e_ctrl, 0, sizeof(e_ctrl));
	memset(config_info, 0, sizeof(config_info));

	// index < 0
	e_ctrl.soc_info.index = -1;
	rc = cam_sec_eeprom_update_afcal(&e_ctrl, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	// index >= INDEX_MAX
	e_ctrl.soc_info.index = SEC_SENSOR_ID_MAX + 1;
	rc = cam_sec_eeprom_update_afcal(&e_ctrl, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	// is_valid_index false
	config_info[config_idx].is_set = 0;

	// is_valid_index failure
	rc = cam_sec_eeprom_update_afcal(&e_ctrl, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);
}


static void mtf_exif_test_success(struct kunit *test)
{
	struct cam_eeprom_ctrl_t e_ctrl;
	struct module_info_t m_info;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	int rc = 0;
	uint32_t config_idx = ADDR_M0_MTF;
	uint32_t config_addr = 0x20;
	uint8_t map_data[1024] = { 0, };
	uint8_t dummy_mtf_exif[FROM_MTF_SIZE + 1] = { 0, };
	int index = 0, i = 0;

	memset(&e_ctrl, 0, sizeof(e_ctrl));
	memset(&m_info, 0, sizeof(m_info));
	memset(config_info, 0, sizeof(config_info));

	for (i = 0; i < SEC_SENSOR_ID_MAX; i++)
	{
		e_ctrl.soc_info.index = i;
		index = map_sensor_id_to_sysfs_index(e_ctrl.soc_info.index);
		if (index == -EINVAL)
			continue;

		config_idx = ADDR_M0_MTF;
		if (e_ctrl.soc_info.index == SEC_TELE_SENSOR ||
			e_ctrl.soc_info.index == SEC_TELE2_SENSOR)
			config_idx = ADDR_S0_MTF;

		memset(config_info, 0, sizeof(config_info));
		set_config(config_info, config_idx, config_addr);

		get_random_bytes(dummy_mtf_exif, FROM_MTF_SIZE);
		dummy_mtf_exif[FROM_MTF_SIZE] = '\0';

		memset(map_data, 0, sizeof(map_data));
		memcpy(&map_data[config_addr], dummy_mtf_exif, FROM_MTF_SIZE + 1);
		e_ctrl.cal_data.mapdata = map_data;

		rc = cam_sec_eeprom_update_mtf_exif(&e_ctrl, config_info);
		KUNIT_EXPECT_EQ(test, 0, rc);
		KUNIT_EXPECT_STREQ(test, mtf_exif[index], dummy_mtf_exif);
	}
}

static void mtf_exif_test_failure(struct kunit *test)
{
	struct cam_eeprom_ctrl_t e_ctrl;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	int rc = 0;
	uint32_t config_idx = ADDR_M0_MTF;

	memset(&e_ctrl, 0, sizeof(e_ctrl));
	memset(config_info, 0, sizeof(config_info));

	// index < 0
	e_ctrl.soc_info.index = -1;
	rc = cam_sec_eeprom_update_mtf_exif(&e_ctrl, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	// index >= INDEX_MAX
	e_ctrl.soc_info.index = SEC_SENSOR_ID_MAX + 1;
	rc = cam_sec_eeprom_update_mtf_exif(&e_ctrl, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	config_info[config_idx].is_set = 0;

	// is_valid_index false
	rc = cam_sec_eeprom_update_mtf_exif(&e_ctrl, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);
}

static void mtf2_exif_test_success(struct kunit *test)
{
	struct cam_eeprom_ctrl_t e_ctrl;
	struct module_info_t m_info;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	int rc = 0;
	uint32_t config_idx = ADDR_M1_MTF;
	uint32_t config_addr = 0x20;
	uint8_t map_data[1024] = { 0, };
	uint8_t dummy2_mtf_exif[FROM_MTF_SIZE + 1] = { 0, };
	int index = 0, i = 0;

	memset(&e_ctrl, 0, sizeof(e_ctrl));
	memset(&m_info, 0, sizeof(m_info));
	memset(config_info, 0, sizeof(config_info));

	for (i = 0; i < SEC_SENSOR_ID_MAX; i++)
	{
		e_ctrl.soc_info.index = i;
		index = map_sensor_id_to_sysfs_index(e_ctrl.soc_info.index);
		if (index == -EINVAL)
			continue;

		config_idx = ADDR_M1_MTF;

		memset(config_info, 0, sizeof(config_info));
		set_config(config_info, config_idx, config_addr);

		get_random_bytes(dummy2_mtf_exif, FROM_MTF_SIZE);
		dummy2_mtf_exif[FROM_MTF_SIZE] = '\0';

		memset(map_data, 0, sizeof(map_data));
		memcpy(&map_data[config_addr], dummy2_mtf_exif, FROM_MTF_SIZE + 1);
		e_ctrl.cal_data.mapdata = map_data;

		rc = cam_sec_eeprom_update_mtf2_exif(&e_ctrl, config_info);
		KUNIT_EXPECT_EQ(test, 0, rc);

		if (e_ctrl.soc_info.index == SEC_WIDE_SENSOR)
			KUNIT_EXPECT_STREQ(test, rear_mtf2_exif, dummy2_mtf_exif);
	}
}

static void mtf2_exif_test_failure(struct kunit *test)
{
	struct cam_eeprom_ctrl_t e_ctrl;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	int rc = 0;
	uint32_t config_idx = ADDR_M0_MTF;

	memset(&e_ctrl, 0, sizeof(e_ctrl));
	memset(config_info, 0, sizeof(config_info));

	// index < 0
	e_ctrl.soc_info.index = -1;
	rc = cam_sec_eeprom_update_mtf2_exif(&e_ctrl, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	// index >= INDEX_MAX
	e_ctrl.soc_info.index = SEC_SENSOR_ID_MAX + 1;
	rc = cam_sec_eeprom_update_mtf2_exif(&e_ctrl, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	config_info[config_idx].is_set = 0;

	// is_valid_index false
	rc = cam_sec_eeprom_update_mtf2_exif(&e_ctrl, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);
}

static void def_info_test_success(struct kunit *test)
{
	int rc = 0;
	struct module_info_t m_info;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	struct module_version_t *module_version = NULL;
	uint32_t core_ver = 0x48; // H
	uint32_t ver_hw = 0x3530584C; // 50XL
	uint32_t ver_sw = 0x50453030; // PE00
	uint32_t ver_etc = 0x53410000; // SA
	uint32_t chk_ver = 0x07424D1F; // 7BM 79

	memset(&m_info, 0, sizeof(m_info));
	memset(config_info, 0, sizeof(config_info));
	module_version = &m_info.module_version;

	set_config(config_info, DEF_M_CORE_VER, core_ver);
	set_config(config_info, DEF_M_VER_HW, ver_hw);
	set_config(config_info, DEF_M_VER_SW, ver_sw);
	set_config(config_info, DEF_M_VER_ETC, ver_etc);
	set_config(config_info, DEF_M_CHK_VER, chk_ver);

	rc = cam_sec_eeprom_update_def_info(&m_info, config_info);

	KUNIT_EXPECT_EQ(test, 0, rc);

	KUNIT_EXPECT_EQ(test, 'H', module_version->phone_hw_info[0]);
	KUNIT_EXPECT_EQ(test, '5', module_version->phone_hw_info[1]);
	KUNIT_EXPECT_EQ(test, '0', module_version->phone_hw_info[2]);
	KUNIT_EXPECT_EQ(test, 'X', module_version->phone_hw_info[3]);
	KUNIT_EXPECT_EQ(test, 'L', module_version->phone_hw_info[4]);

	KUNIT_EXPECT_EQ(test, 'P', module_version->phone_sw_info[0]);
	KUNIT_EXPECT_EQ(test, 'E', module_version->phone_sw_info[1]);
	KUNIT_EXPECT_EQ(test, '0', module_version->phone_sw_info[2]);
	KUNIT_EXPECT_EQ(test, '0', module_version->phone_sw_info[3]);

	KUNIT_EXPECT_EQ(test, 'S', module_version->phone_vendor_info[0]);
	KUNIT_EXPECT_EQ(test, 'A', module_version->phone_process_info[0]);

	KUNIT_EXPECT_EQ(test, 7, module_version->phone_chk_ver_info[0]);
	KUNIT_EXPECT_EQ(test, 'B', module_version->phone_chk_ver_info[1]);
	KUNIT_EXPECT_EQ(test, 'M', module_version->phone_chk_ver_info[2]);
	KUNIT_EXPECT_EQ(test, 79, module_version->phone_chk_ver_info[3]);
}

static void def_info_test_failure(struct kunit *test)
{
	int rc = 0;
	struct module_info_t m_info;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];

	memset(&m_info, 0, sizeof(m_info));
	memset(config_info, 0, sizeof(config_info));

	rc = cam_sec_eeprom_update_def_info(NULL, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	rc = cam_sec_eeprom_update_def_info(&m_info, NULL);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);
}

static void paf_test_success(struct kunit *test)
{
	int rc = 0;
	struct cam_eeprom_ctrl_t e_ctrl;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	uint32_t config_idx = ADDR_M1_PAF;
	uint32_t config_addr = 0x20;
	uint8_t map_data[1024] = { 0, };
	uint32_t dummy_paf_err = 0;

	memset(&e_ctrl, 0, sizeof(e_ctrl));
	memset(config_info, 0, sizeof(config_info));

	config_idx = ADDR_M1_PAF;

	memset(config_info, 0, sizeof(config_info));
	set_config(config_info, config_idx, config_addr);

	get_random_bytes((void *)&dummy_paf_err, 4);

	memset(map_data, 0, sizeof(map_data));
	memcpy(&map_data[config_addr + PAF_CAL_ERR_CHECK_OFFSET],
		&dummy_paf_err, 4);
	e_ctrl.cal_data.mapdata = map_data;

	rc = cam_sec_eeprom_update_paf(&e_ctrl, config_info);
	KUNIT_EXPECT_EQ(test, 0, rc);
	KUNIT_EXPECT_EQ(test, f2_paf_err_data_result, dummy_paf_err);

	e_ctrl.soc_info.index = SEC_TELE_SENSOR;
	rc = cam_sec_eeprom_update_paf(&e_ctrl, config_info);
	KUNIT_EXPECT_EQ(test, 0, rc);
}

static void paf_err_test_success(struct kunit *test)
{
	struct cam_eeprom_ctrl_t e_ctrl;
	struct module_info_t m_info;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	int rc = 0;
	uint32_t config_idx = ADDR_M0_PAF;
	uint32_t config_addr = 0x20;
	uint8_t map_data[1024] = { 0, };
	uint32_t dummy_paf_err = 0xFFFFFFFF;
	int index = 0, i = 0;

	memset(&e_ctrl, 0, sizeof(e_ctrl));
	memset(&m_info, 0, sizeof(m_info));
	memset(config_info, 0, sizeof(config_info));

	for (i = 0; i < SEC_SENSOR_ID_MAX; i++)
	{
		e_ctrl.soc_info.index = i;
		index = map_sensor_id_to_sysfs_index(e_ctrl.soc_info.index);
		if (index == -EINVAL)
			continue;

		config_idx = ADDR_M0_PAF;
		if (e_ctrl.soc_info.index == SEC_TELE_SENSOR ||
			e_ctrl.soc_info.index == SEC_TELE2_SENSOR)
			config_idx = ADDR_S0_PAF;

		memset(config_info, 0, sizeof(config_info));
		set_config(config_info, config_idx, config_addr);

		get_random_bytes((void *)&dummy_paf_err, 4);

		memset(map_data, 0, sizeof(map_data));
		memcpy(&map_data[config_addr + PAF_CAL_ERR_CHECK_OFFSET],
			&dummy_paf_err, 4);
		e_ctrl.cal_data.mapdata = map_data;

		rc = cam_sec_eeprom_update_paf_err(&e_ctrl, config_info);
		KUNIT_EXPECT_EQ(test, 0, rc);
		KUNIT_EXPECT_EQ(test, paf_err_data_result[index], dummy_paf_err);
	}
}

static void paf_err_test_failure(struct kunit *test)
{
	struct cam_eeprom_ctrl_t e_ctrl;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	int rc = 0;
	uint32_t config_idx = ADDR_M0_MTF;

	memset(&e_ctrl, 0, sizeof(e_ctrl));
	memset(config_info, 0, sizeof(config_info));

	// index < 0
	e_ctrl.soc_info.index = -1;
	rc = cam_sec_eeprom_update_paf_err(&e_ctrl, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	// index >= INDEX_MAX
	e_ctrl.soc_info.index = SEC_SENSOR_ID_MAX + 1;
	rc = cam_sec_eeprom_update_paf_err(&e_ctrl, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	config_info[config_idx].is_set = 0;

	// is_valid_index false
	rc = cam_sec_eeprom_update_paf_err(&e_ctrl, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);
}

static void reset_module_info_test_success(struct kunit *test)
{
	struct cam_eeprom_ctrl_t e_ctrl;
	int rc = 0, index = 0, i = 0;

	memset(&e_ctrl, 0, sizeof(e_ctrl));

	for (i = 0; i < SEC_SENSOR_ID_MAX; i++)
	{
		e_ctrl.soc_info.index = i;
		index = map_sensor_id_to_sysfs_index(e_ctrl.soc_info.index);
		if (index == -EINVAL)
			continue;

		strlcpy(module_id[index], "", FROM_MODULE_ID_SIZE);
		rc = cam_sec_eeprom_reset_module_info(&e_ctrl);
		KUNIT_EXPECT_EQ(test, 0, rc);
		KUNIT_EXPECT_STREQ(test, "NULL", module_id[index]);
	}
}

static void reset_module_info_test_failure(struct kunit *test)
{
	struct cam_eeprom_ctrl_t e_ctrl;
	int rc = 0;

	memset(&e_ctrl, 0, sizeof(e_ctrl));

	// index < 0
	e_ctrl.soc_info.index = -1;
	rc = cam_sec_eeprom_reset_module_info(&e_ctrl);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	// index >= INDEX_MAX
	e_ctrl.soc_info.index = SEC_SENSOR_ID_MAX + 1;
	rc = cam_sec_eeprom_reset_module_info(&e_ctrl);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);
}

static void update_sysfs_fw_version_test_success(struct kunit *test)
{
	struct module_info_t m_info;
	const char *dummy_module_fw_ver = "MODULE_VER";
	const char *dummy_phone_fw_ver = "PHONE_VER";
	const char *dummy_load_fw_ver = "LOAD_VER";
	char out_fw_ver[SYSFS_FW_VER_SIZE] = "NULL NULL\n";
	char out_fw_full_ver[SYSFS_FW_VER_SIZE] = "NULL NULL NULL\n";
	const char *dummy_fw_ver = "MODULE_VER LOAD_VER\n";
	const char *dummy_fw_full_ver = "MODULE_VER PHONE_VER LOAD_VER\n";

	memset(&m_info, 0, sizeof(m_info));

	m_info.type = SEC_WIDE_SENSOR;
	m_info.main_sub = MAIN_MODULE;
	m_info.module_version.cam_fw_ver = out_fw_ver;
	m_info.module_version.cam_fw_full_ver = out_fw_full_ver;

	cam_sec_eeprom_update_sysfs_fw_version(
		dummy_module_fw_ver, EEPROM_FW_VER, &m_info);
	KUNIT_EXPECT_STREQ(test, dummy_module_fw_ver,
		m_info.module_version.module_fw_ver);

	cam_sec_eeprom_update_sysfs_fw_version(
		dummy_phone_fw_ver, PHONE_FW_VER, &m_info);
	KUNIT_EXPECT_STREQ(test, dummy_phone_fw_ver,
		m_info.module_version.phone_fw_ver);

	cam_sec_eeprom_update_sysfs_fw_version(
		dummy_load_fw_ver, LOAD_FW_VER, &m_info);
	KUNIT_EXPECT_STREQ(test, dummy_load_fw_ver,
		m_info.module_version.load_fw_ver);

	KUNIT_EXPECT_STREQ(test, dummy_fw_ver,
		m_info.module_version.cam_fw_ver);
	KUNIT_EXPECT_STREQ(test, dummy_fw_full_ver,
		m_info.module_version.cam_fw_full_ver);
}

static void eeprom_verify_sum_test_success(struct kunit *test)
{
	int rc = 0;
	struct cam_eeprom_ctrl_t e_ctrl;
	struct cam_eeprom_memory_map_t map[1];
	uint32_t addr = 0x100;
	uint32_t valid_size = 32;
	uint32_t little_crc = 0xB52C225, big_crc = 0x25c2520b;
	char* memptr = NULL;
	uint32_t rev_endian = 0;
	uint8_t map_data[1024] = { 0, };
	uint8_t dummy_map_data[32] = {
		0xB9, 0x95, 0x43, 0xEF, 0xC1, 0x8A, 0xD7, 0xF9,
		0xD5, 0xFC, 0x2B, 0xAF, 0xAE, 0x55, 0x0C, 0x2E,
		0x64, 0x70, 0x51, 0x48, 0x6D, 0xCB, 0x5A, 0x62,
		0x15, 0xC3, 0xB1, 0x31, 0x7D, 0xB9, 0xF3, 0x24,
	};

	memset(&e_ctrl, 0, sizeof(e_ctrl));
	memset(map, 0, sizeof(map));

	e_ctrl.cal_data.num_map = 1;
	e_ctrl.cal_data.map = map;
	e_ctrl.cal_data.mapdata = map_data;

	memptr = map_data + addr;
	memcpy(memptr, dummy_map_data, sizeof(dummy_map_data));

	rev_endian = 0;
	rc = cam_sec_eeprom_verify_sum(
		memptr, valid_size, little_crc, rev_endian);
	KUNIT_EXPECT_EQ(test, 0, rc);

	rev_endian = 1;
	rc = cam_sec_eeprom_verify_sum(
		memptr, valid_size, big_crc, rev_endian);
	KUNIT_EXPECT_EQ(test, 0, rc);
}

static void eeprom_verify_sum_test_failure(struct kunit *test)
{
	int rc = 0;
	struct cam_eeprom_ctrl_t e_ctrl;
	struct cam_eeprom_memory_map_t map[1];
	uint32_t addr = 0x100;
	uint32_t valid_size = 32;
	uint32_t sum = 0, rev_endian = 0;
	uint8_t map_data[1024] = { 0, };
	char* memptr = NULL;

	memset(&e_ctrl, 0, sizeof(e_ctrl));
	memset(map, 0, sizeof(map));

	e_ctrl.cal_data.num_map = 1;
	e_ctrl.cal_data.map = map;
	e_ctrl.cal_data.mapdata = map_data;
	get_random_bytes((void *)map_data, 1024);

	map[0].mem.addr = addr;
	map[0].mem.valid_size = valid_size;

	memptr = map_data + addr;

	valid_size = ~0;
	rc = cam_sec_eeprom_verify_sum(memptr, valid_size,
		sum, rev_endian);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	sum = 1234;
	rc = cam_sec_eeprom_verify_sum(memptr, valid_size,
		sum, rev_endian);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);
}

#define CRC_TEST_MAP_SIZE (5)
static void match_crc_test_success(struct kunit *test)
{
	int rc = 0, i = 0;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	struct cam_eeprom_memory_block_t cal_data;
	struct cam_eeprom_memory_map_t map[CRC_TEST_MAP_SIZE];
	uint32_t addrs[CRC_TEST_MAP_SIZE] = { 0x00, 0x00, 0x1C,  0x20, 0x3C };
	uint32_t valid_sizes[CRC_TEST_MAP_SIZE] = { 0x40, 0x1C, 0x04, 0x1C, 0x04};
	int normal_crc = 0;
	uint8_t map_data[0x40] = {
		0x8A, 0xA7, 0x2E, 0x87, 0x34, 0x42, 0x1C, 0x59,
		0x52, 0x6E, 0x6B, 0x5E, 0x49, 0x8E, 0xB4, 0xE7,
		0x21, 0xC8, 0x20, 0x7C, 0x03, 0xAB, 0xEE, 0x31,
		0x27, 0xE5, 0xC5, 0xF0, 0x77, 0x0C, 0x1F, 0x0B,
		0xFD, 0x53, 0x59, 0x94, 0x2B, 0xBF, 0xE3, 0xB3,
		0x32, 0x63, 0xCA, 0x05, 0xA2, 0xC3, 0xEF, 0x83,
		0xD0, 0x14, 0xE3, 0xE1, 0xA0, 0xD8, 0x64, 0x55,
		0x40, 0xA4, 0x80, 0x13, 0x74, 0x05, 0x1E, 0x29
	};
	memset(&cal_data, 0, sizeof(cal_data));
	memset(map, 0, sizeof(map));

	cal_data.num_map = CRC_TEST_MAP_SIZE;
	cal_data.map = map;
	cal_data.mapdata = map_data;

	for (i = 1; i < cal_data.num_map; i++) {
		map[i].mem.addr = addrs[i];
		map[i].mem.valid_size = valid_sizes[i];
	}

	for (i = 0; i < (CRC_TEST_MAP_SIZE >> 1); i++)
		normal_crc |= (1 << i);

	rc = cam_sec_eeprom_match_crc(&cal_data, 0, config_info);
	KUNIT_EXPECT_EQ(test, normal_crc, rc);

	set_config(config_info, ADDR_M_CALMAP_VER, 0x00);
	rc = cam_sec_eeprom_match_crc(&cal_data, 0, config_info);
	KUNIT_EXPECT_EQ(test, normal_crc, rc);

	set_config(config_info, ADDR_M_CALMAP_VER, 0x01);
	rc = cam_sec_eeprom_match_crc(&cal_data, 0, config_info);
	KUNIT_EXPECT_EQ(test, normal_crc, rc);
}

static void match_crc_test_failure(struct kunit *test)
{
	int rc = 0, i = 0;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	struct cam_eeprom_memory_block_t cal_data;
	struct cam_eeprom_memory_map_t map[CRC_TEST_MAP_SIZE];
	uint32_t addrs[CRC_TEST_MAP_SIZE] = { 0x00, 0x00, 0x1C,  0x20, 0x3C };
	uint32_t valid_sizes[CRC_TEST_MAP_SIZE] = { 0x40, 0x1C, 0x04, 0x1C, 0x01};
	uint8_t map_data[0x40] = {
		0x8A, 0xA7, 0x2E, 0x87, 0x34, 0x42, 0x1C, 0x59,
		0x52, 0x6E, 0x6B, 0x5E, 0x49, 0x8E, 0xB4, 0xE7,
		0x21, 0xC8, 0x20, 0x7C, 0x03, 0xAB, 0xEE, 0x31,
		0x27, 0xE5, 0xC5, 0xF0, 0x77, 0x0C, 0x1F, 0x0B,
		0xFD, 0x53, 0x59, 0x94, 0x2B, 0xBF, 0xE3, 0xB3,
		0x32, 0x63, 0xCA, 0x05, 0xA2, 0xC3, 0xEF, 0x83,
		0xD0, 0x14, 0xE3, 0xE1, 0xA0, 0xD8, 0x64, 0x55,
		0x40, 0xA4, 0x80, 0x13, 0x74, 0x05, 0x1E, 0x29
	};
	memset(&cal_data, 0, sizeof(cal_data));
	memset(map, 0, sizeof(map));

	cal_data.num_map = CRC_TEST_MAP_SIZE;
	cal_data.map = map;
	cal_data.mapdata = map_data;

	for (i = 1; i < cal_data.num_map; i++) {
		map[i].mem.addr = addrs[i];
		map[i].mem.valid_size = valid_sizes[i];
	}

	// data == NULL
	rc = cam_sec_eeprom_match_crc(NULL, 0, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);

	// subdev_id < 0 || subdev_id >= SEC_SENSOR_ID_MAX
	rc = cam_sec_eeprom_match_crc(&cal_data, SEC_SENSOR_ID_MAX, config_info);
	KUNIT_EXPECT_EQ(test, 0, rc);

	// mem.valid_size < sizeof(uint32_t)
	rc = cam_sec_eeprom_match_crc(&cal_data, 0, config_info);
	KUNIT_EXPECT_EQ(test, -EINVAL, rc);
}

static void calc_calmap_size_test_success(struct kunit *test)
{
	struct cam_eeprom_ctrl_t e_ctrl;
	int32_t calmap_size = 0, i = 0;
	struct cam_eeprom_memory_map_t map[3];
	uint32_t addrs[3] = { 0, 100, 256 };
	uint32_t valid_sizes[3] = { 80, 20, 100 };

	memset(&e_ctrl, 0, sizeof(e_ctrl));
	memset(map, 0, sizeof(map));

	e_ctrl.cal_data.num_map = 3;
	e_ctrl.cal_data.map = map;

	for (i = 0; i < 3; i++) {
		map[i].mem.addr = addrs[i];
		map[i].mem.valid_size = valid_sizes[i];
	}
	calmap_size = cam_sec_eeprom_calc_calmap_size(&e_ctrl);
	KUNIT_EXPECT_EQ(test, 356, calmap_size);
}

static void calc_calmap_size_test_failure(struct kunit *test)
{
	struct cam_eeprom_ctrl_t e_ctrl;
	int32_t calmap_size = 0;

	memset(&e_ctrl, 0, sizeof(e_ctrl));

	// e_ctrl == NULL
	calmap_size = cam_sec_eeprom_calc_calmap_size(NULL);
	KUNIT_EXPECT_EQ(test, 0, calmap_size);

	// cal_data.num_map == 0
	calmap_size = cam_sec_eeprom_calc_calmap_size(&e_ctrl);
	KUNIT_EXPECT_EQ(test, 0, calmap_size);

	// cal_data.map == NULL
	e_ctrl.cal_data.num_map = 3;
	calmap_size = cam_sec_eeprom_calc_calmap_size(&e_ctrl);
	KUNIT_EXPECT_EQ(test, 0, calmap_size);
}

static void fill_config_info_test_success(struct kunit *test)
{
	struct module_info_t m_info;
	struct config_info_t config_info[MAX_CONFIG_INFO_IDX];
	int rc = 0, i = 0, valid_num = 0;
	struct config_info_input_t {
		char config_string[64];
		uint32_t value;
	};
	struct config_info_input_t dummy_config_info[] = {
		{ "DEF_M_VER_HW", 0x20 },
		{ "DEF_M_VER_SW", 0x11 },
		{ "ADDR_M_HEADER", 0x0c },
		{ "ADDR_M_CALMAP_VER", 0x700 },
		{ "ADDR_M0_MTF", 0x100 },

		// Invalid config
		{ "DEF_M_VER_SW", 0x30 },
		{ "DUMMY_CONFIG", 0x11 },
	};

	memset(&m_info, 0, sizeof(m_info));
	memset(config_info, 0, sizeof(config_info));

	for (i = 0; i < ARRAY_SIZE(dummy_config_info); i++)
	{
		rc = cam_sec_eeprom_fill_config_info(config_info,
			dummy_config_info[i].config_string,
			dummy_config_info[i].value);
		if (rc == 0)
			valid_num++;
	}

	KUNIT_EXPECT_EQ(test, 5, valid_num);
}

static struct kunit_case cam_sec_eeprom_test_cases[] = {
	KUNIT_CASE(is_valid_index_test_success),
	KUNIT_CASE(is_valid_index_test_failure),
	KUNIT_CASE(uint_to_char_array_test_success),
	KUNIT_CASE(map_sensor_id_to_sysfs_index_test_success),
	KUNIT_CASE(map_sensor_id_to_type_str_test_success),
	KUNIT_CASE(sensor_id_test_success),
	KUNIT_CASE(sensor_id_test_failure),
	KUNIT_CASE(module_id_test_success),
	KUNIT_CASE(module_id_test_failure),
	KUNIT_CASE(set_paf_test_success),
	KUNIT_CASE(set_paf_test_failure),
	KUNIT_CASE(afcal_test_success),
	KUNIT_CASE(afcal_test_failure),
	KUNIT_CASE(mtf_exif_test_success),
	KUNIT_CASE(mtf_exif_test_failure),
	KUNIT_CASE(mtf2_exif_test_success),
	KUNIT_CASE(mtf2_exif_test_failure),
	KUNIT_CASE(def_info_test_success),
	KUNIT_CASE(def_info_test_failure),
	KUNIT_CASE(paf_test_success),
	KUNIT_CASE(paf_err_test_success),
	KUNIT_CASE(paf_err_test_failure),
	KUNIT_CASE(reset_module_info_test_success),
	KUNIT_CASE(reset_module_info_test_failure),
	KUNIT_CASE(update_sysfs_fw_version_test_success),
	KUNIT_CASE(eeprom_verify_sum_test_success),
	KUNIT_CASE(eeprom_verify_sum_test_failure),
	KUNIT_CASE(match_crc_test_success),
	KUNIT_CASE(match_crc_test_failure),
	KUNIT_CASE(calc_calmap_size_test_success),
	KUNIT_CASE(calc_calmap_size_test_failure),
	KUNIT_CASE(fill_config_info_test_success),
	{}
};

static struct kunit_suite cam_sec_eeprom_test_suite = {
	.name = "cam_sec_eeprom_test",
	.test_cases = cam_sec_eeprom_test_cases,
};
kunit_test_suite(cam_sec_eeprom_test_suite);

int cam_kunit_sec_eeprom_test(void)
{
	kunit_run_tests(&cam_sec_eeprom_test_suite);
	return 0;
}

MODULE_LICENSE("GPL");
