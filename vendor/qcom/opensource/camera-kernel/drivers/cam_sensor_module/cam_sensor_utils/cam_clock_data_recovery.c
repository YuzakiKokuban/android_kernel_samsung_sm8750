// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "cam_clock_data_recovery.h"

struct cam_clock_data_recovery_info cam1_cdr_info;
struct cam_clock_data_recovery_info cam2_cdr_info;

int cam_clock_data_recovery_write_register(void __iomem *csiphybase, int camera_type, int is_cphy)
{
	int i, j;
	int str_idx;
	int len = 0;
	int value_len = 0;
	int count_idx = 0;
	int cdr_data[10] = { 0, };
	int cdr_value = 0;
	int cphy_delay_addr[3] = { 0x278, 0x678, 0xa78 };
	int dphy_delay_addr[4] = { 0x000, 0x400, 0x800, 0xc00 };
	struct cam_clock_data_recovery_info* cdr_info;

	if (camera_type == 1)
		cdr_info = &cam1_cdr_info;
	else if (camera_type == 2)
		cdr_info = &cam2_cdr_info;
	else
	{
		CAM_ERR(CAM_UTIL, "[CDR_DBG] invalid camera type");
		return 0;
	}

	len = strlen(cdr_info->value);

	CAM_INFO(CAM_UTIL, "[CDR_DBG] camera_type : %d, input: %s", camera_type, cdr_info->value);
	sprintf(cdr_info->result, "%s\n", "");

	for (str_idx = 0; str_idx < len; str_idx++)
	{
		if (count_idx > 2)
		{
			CAM_ERR(CAM_UTIL, "[CDR_DBG] input value overflow");
			return 0;
		}

		if (cdr_info->value[str_idx] != ',')
		{
			if (count_idx == 0)
			{
				CAM_DBG(CAM_UTIL, "[CDR_DBG] skip id part");
			}
			else
			{
				if (cdr_info->value[str_idx] >= 'a' && cdr_info->value[str_idx] <= 'f')
				{
					cdr_data[value_len] = cdr_info->value[str_idx] - 'W';
					value_len++;
				}
				else if (cdr_info->value[str_idx] >= 'A' && cdr_info->value[str_idx] <= 'F')
				{
					cdr_data[value_len] = cdr_info->value[str_idx] - '7';
					value_len++;
				}
				else if (cdr_info->value[str_idx] >= '0' && cdr_info->value[str_idx] <= '9')
				{
					cdr_data[value_len] = cdr_info->value[str_idx] - '0';
					value_len++;
				}
				else
				{
					CAM_ERR(CAM_UTIL, "[CDR_DBG] invalid input value");
					return 0;
				}
			}
		}
		else
		{
			count_idx++;
		}
	}

	for (i = 0; i < value_len; i++)
	{
		int temp = 1;
		for (j = value_len - 1; j > i; j--)
			temp = temp * 16;
		cdr_value += temp * cdr_data[i];
	}

	if (is_cphy == 1)
	{
		for (i = 0; i < 3; i++) {
			cam_io_w_mb(cdr_value,
				csiphybase + cphy_delay_addr[i]);

			CAM_INFO(CAM_UTIL, "[CDR_DBG] CPHY Offset: 0x%x, Val: 0x%x",
				cphy_delay_addr[i],
				cam_io_r_mb(csiphybase + cphy_delay_addr[i]));
		}
	}
	else if (is_cphy == 0) {
		for (i = 0; i < 4; i++) {
			cam_io_w_mb(cdr_value,
				csiphybase + dphy_delay_addr[i]);

			CAM_INFO(CAM_UTIL, "[CDR_DBG] DPHY Offset: 0x%x, Val: 0x%x",
				dphy_delay_addr[i],
				cam_io_r_mb(csiphybase + dphy_delay_addr[i]));
		}
	}

	return 1;
}

void cam_clock_data_recovery_set_value(int camera_type, const char* buf)
{
	if (camera_type == 1)
	{
		if (!strncmp(buf, "56", 2)) {
			cam1_cdr_info.cam_type = WIDE_CAM;
		} else if (!strncmp(buf, "58", 2)) {
			cam1_cdr_info.cam_type = UW_CAM;
		} else if (!strncmp(buf, "52", 2)) {
			cam1_cdr_info.cam_type = TELE1_CAM;
		} else if (!strncmp(buf, "54", 2)) {
			cam1_cdr_info.cam_type = TELE2_CAM;
		} else if (!strncmp(buf, "1", 1)) {
			cam1_cdr_info.cam_type = FRONT_CAM;
		}

		cam1_cdr_info.is_requested = 1;
		cam1_cdr_info.is_valid = 1;
		scnprintf(cam1_cdr_info.value, sizeof(cam1_cdr_info.value), "%s", buf);
	}
	else if (camera_type == 2)
	{
		if (!strncmp(buf, "56", 2)) {
			cam2_cdr_info.cam_type = WIDE_CAM;
		} else if (!strncmp(buf, "58", 2)) {
			cam2_cdr_info.cam_type = UW_CAM;
		} else if (!strncmp(buf, "52", 2)) {
			cam2_cdr_info.cam_type = TELE1_CAM;
		} else if (!strncmp(buf, "54", 2)) {
			cam2_cdr_info.cam_type = TELE2_CAM;
		} else if (!strncmp(buf, "1", 1)) {
			cam2_cdr_info.cam_type = FRONT_CAM;
		}

		cam2_cdr_info.is_requested = 1;
		cam2_cdr_info.is_valid = 1;
		scnprintf(cam2_cdr_info.value, sizeof(cam2_cdr_info.value), "%s", buf);
	}
	else
		CAM_ERR(CAM_UTIL, "[CDR_DBG] invalid camera type : %d", camera_type);
}

char* cam_clock_data_recovery_get_value(int camera_type)
{
	if (camera_type == 1)
		return cam1_cdr_info.value;
	else if (camera_type == 2)
		return cam2_cdr_info.value;
	else {
		CAM_ERR(CAM_UTIL, "[CDR_DBG] invalid camera type : %d", camera_type);
		return 0;
	}
}

void cam_clock_data_recovery_set_result(int camera_type, enum cam_clock_data_recovery_error error_type)
{
	if (camera_type == 1)
	{
		cam_clock_data_recovery_get_timestamp(camera_type, CDR_END_TS);
		sprintf(cam1_cdr_info.result, "%d,%lld\n", error_type, cam1_cdr_info.timestamp[CDR_END_TS]-cam1_cdr_info.timestamp[CDR_START_TS]);
		CAM_INFO(CAM_UTIL, "[CDR_DBG] camera_type : %d, %s, time(ms): %llu",
			camera_type, ((error_type == 0) ? "mipi_overflow" : "i2c_fail"), cam1_cdr_info.timestamp[CDR_END_TS]-cam1_cdr_info.timestamp[CDR_START_TS]);
	}
	else if (camera_type == 2)
	{
		cam_clock_data_recovery_get_timestamp(camera_type, CDR_END_TS);
		sprintf(cam2_cdr_info.result, "%d,%lld\n", error_type, cam2_cdr_info.timestamp[CDR_END_TS]-cam2_cdr_info.timestamp[CDR_START_TS]);
		CAM_INFO(CAM_UTIL, "[CDR_DBG] camera_type : %d, %s, time(ms): %llu",
			camera_type, ((error_type == 0) ? "mipi_overflow" : "i2c_fail"), cam2_cdr_info.timestamp[CDR_END_TS]-cam2_cdr_info.timestamp[CDR_START_TS]);
	}
}

char* cam_clock_data_recovery_get_result(int camera_type)
{
	if (camera_type == 1)
		return cam1_cdr_info.result;
	else if (camera_type == 2)
		return cam2_cdr_info.result;
	else {
		CAM_ERR(CAM_UTIL, "[CDR_DBG] invalid camera type : %d", camera_type);
		return 0;
	}
}

void cam_clock_data_recovery_reset_result(int camera_type, const char* buf)
{
	if (camera_type == 1)
		scnprintf(cam1_cdr_info.result, sizeof(cam1_cdr_info.result), "%s", buf);
	else if (camera_type == 2)
		scnprintf(cam2_cdr_info.result, sizeof(cam2_cdr_info.result), "%s", buf);
	else {
		CAM_ERR(CAM_UTIL, "[CDR_DBG] invalid camera type : %d", camera_type);
	}
}

int cam_clock_data_get_camera_type(uint8_t csiphy_idx)
{
	if (cam1_cdr_info.cam_type == csiphy_idx && cam1_cdr_info.is_valid == 1)
		return 1;
	else if (cam2_cdr_info.cam_type == csiphy_idx && cam2_cdr_info.is_valid == 1)
		return 2;
	else {
		return 0;
	}
}

int cam_clock_data_recovery_is_requested(int camera_type)
{
	if (camera_type == 1)
		return cam1_cdr_info.is_requested;
	else if (camera_type == 2)
		return cam2_cdr_info.is_requested;
	else {
		CAM_ERR(CAM_UTIL, "[CDR_DBG] invalid camera type : %d", camera_type);
		return 0;
	}
}

void cam_clock_data_recovery_reset_request(int camera_type)
{
	if (camera_type == 1)
		cam1_cdr_info.is_requested = 0;
	else if (camera_type == 2)
		cam2_cdr_info.is_requested = 0;
}

void cam_clock_data_recovery_get_timestamp(int camera_type, enum cam_clock_data_recovery_timestamp type)
{
	if (camera_type == 1)
	{
	 	cam1_cdr_info.timestamp[type] = ktime_get();
	 	cam1_cdr_info.timestamp[type] = cam1_cdr_info.timestamp[type] / 1000 / 1000;
	}
	else if (camera_type == 2)
	{
	 	cam2_cdr_info.timestamp[type] = ktime_get();
	 	cam2_cdr_info.timestamp[type] = cam2_cdr_info.timestamp[type] / 1000 / 1000;
	}
}
