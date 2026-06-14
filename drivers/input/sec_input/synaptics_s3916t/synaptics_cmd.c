// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "synaptics_dev.h"
#include "synaptics_reg.h"

static void synaptics_ts_print_channel(struct synaptics_ts_data *ts,
		struct synaptics_ts_buffer *tdata, int *min, int *max, u8 type)
{
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };
	int i = 0, j = 0, k = 0;
	short *data_ptr = NULL;
	int *data_ptr_int = NULL;
	int lsize = PRINT_SIZE * (ts->cols + 1);

	if (!ts->cols)
		return;

	switch (type) {
	case TEST_PID18_HYBRID_ABS_RAW:  /* int */
		data_ptr_int = (int *)&tdata->buf[0];
		for (i = 0; i < ts->rows + ts->cols; i++) {
			ts->pFrame[i] = *data_ptr_int;
			data_ptr_int++;
		}
		break;
	default: /* short */
		data_ptr = (short *)&tdata->buf[0];
		for (i = 0; i < ts->rows + ts->cols; i++) {
			ts->pFrame[i] = *data_ptr;
			data_ptr++;
		}
		break;
	}

	*min = *max = ts->pFrame[0];

	pStr = vzalloc(lsize);
	if (!pStr)
		return;

	memset(pStr, 0x0, lsize);
	snprintf(pTmp, sizeof(pTmp), " RX");
	strlcat(pStr, pTmp, lsize);

	for (k = 0; k < ts->cols; k++) {
		snprintf(pTmp, sizeof(pTmp), "    %02d", k);
		strlcat(pStr, pTmp, lsize);
	}
	input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev, "%s\n", pStr);

	memset(pStr, 0x0, lsize);
	snprintf(pTmp, sizeof(pTmp), " +");
	strlcat(pStr, pTmp, lsize);

	for (k = 0; k < ts->cols; k++) {
		snprintf(pTmp, sizeof(pTmp), "------");
		strlcat(pStr, pTmp, lsize);
	}
	input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev, "%s\n", pStr);

	memset(pStr, 0x0, lsize);
	snprintf(pTmp, sizeof(pTmp), " | ");
	strlcat(pStr, pTmp, lsize);

	for (i = 0; i < ts->cols + ts->rows; i++) {
		if (i == ts->cols) {
			input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev, "%s\n", pStr);
			input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev, "\n");
			memset(pStr, 0x0, lsize);
			snprintf(pTmp, sizeof(pTmp), " TX");
			strlcat(pStr, pTmp, lsize);

			for (k = 0; k < ts->rows; k++) {
				snprintf(pTmp, sizeof(pTmp), "    %02d", k);
				strlcat(pStr, pTmp, lsize);
			}

			input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev, "%s\n", pStr);

			memset(pStr, 0x0, lsize);
			snprintf(pTmp, sizeof(pTmp), " +");
			strlcat(pStr, pTmp, lsize);

			for (k = 0; k < ts->cols; k++) {
				snprintf(pTmp, sizeof(pTmp), "------");
				strlcat(pStr, pTmp, lsize);
			}
			input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev, "%s\n", pStr);

			memset(pStr, 0x0, lsize);
			snprintf(pTmp, sizeof(pTmp), " | ");
			strlcat(pStr, pTmp, lsize);
		} else if (i && !(i % ts->cols)) {
			input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev, "%s\n", pStr);
			memset(pStr, 0x0, lsize);
			snprintf(pTmp, sizeof(pTmp), " | ");
			strlcat(pStr, pTmp, lsize);
		}

		snprintf(pTmp, sizeof(pTmp), " %5d", ts->pFrame[i]);
		strlcat(pStr, pTmp, lsize);

		*min = min(*min, ts->pFrame[i]);
		*max = max(*max, ts->pFrame[i]);

		j++;
	}
	input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev, "%s\n", pStr);
	vfree(pStr);
}

static void synaptics_ts_print_frame(struct synaptics_ts_data *ts,
		struct synaptics_ts_buffer *tdata, int *min, int *max, u8 type)
{
	int i = 0;
	int j = 0;
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };
	int lsize = PRINT_SIZE * (ts->rows + 1);
	short *data_ptr = NULL;
	int *data_ptr_int = NULL;

	input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev, "%s\n", __func__);

	switch (type) {
	case TEST_PID18_HYBRID_ABS_RAW:  /* int */
		data_ptr_int = (int *)&tdata->buf[0];
		for (i = 0; i < ts->rows; i++) {
			for (j = 0; j < ts->cols; j++) {
				ts->pFrame[i + (ts->rows * j)] = *data_ptr_int;
				data_ptr_int++;
			}
		}
		break;
	default:  /* short */
		data_ptr = (short *)&tdata->buf[0];
		for (i = 0; i < ts->rows; i++) {
			for (j = 0; j < ts->cols; j++) {
				ts->pFrame[i + (ts->rows * j)] = *data_ptr;
				data_ptr++;
			}
		}
		break;
	}

	*min = *max = ts->pFrame[0];

	pStr = kzalloc(lsize, GFP_KERNEL);
	if (pStr == NULL)
		return;

	memset(pStr, 0x0, lsize);
	snprintf(pTmp, sizeof(pTmp), "      TX");
	strlcat(pStr, pTmp, lsize);

	for (i = 0; i < ts->rows; i++) {
		snprintf(pTmp, sizeof(pTmp), " %02d ", i);
		strlcat(pStr, pTmp, lsize);
	}

	input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev, "%s\n", pStr);
	memset(pStr, 0x0, lsize);
	snprintf(pTmp, sizeof(pTmp), " +");
	strlcat(pStr, pTmp, lsize);

	for (i = 0; i < ts->rows; i++) {
		snprintf(pTmp, sizeof(pTmp), "----");
		strlcat(pStr, pTmp, lsize);
	}

	input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev, "%s\n", pStr);

	for (i = 0; i < ts->cols; i++) {
		memset(pStr, 0x0, lsize);
		snprintf(pTmp, sizeof(pTmp), "RX%02d | ", i);
		strlcat(pStr, pTmp, lsize);

		for (j = 0; j < ts->rows; j++) {
			snprintf(pTmp, sizeof(pTmp), " %5d", ts->pFrame[j + (i * ts->rows)]);

			if (ts->pFrame[j + (i * ts->rows)] < *min)
				*min = ts->pFrame[j + (i * ts->rows)];

			if (ts->pFrame[j + (i * ts->rows)] > *max)
				*max = ts->pFrame[j + (i * ts->rows)];

			strlcat(pStr, pTmp, lsize);
		}
		input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev, "%s\n", pStr);
	}
	kfree(pStr);
}

static int get_self_channel_data(void *device_data, u8 type)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[16] = { 0 };
	int ii;
	int tx_min = 0;
	int rx_min = 0;
	int tx_max = 0;
	int rx_max = 0;
	char *item_name = "NULL";
	char temp[SEC_CMD_STR_LEN] = { 0 };

	switch (type) {
	case TEST_PID05_FULL_RAW_CAP:
		item_name = "FULL_RAWCAP"; /* short * */
		break;
	case TEST_PID10_DELTA_NOISE:
		item_name = "DELTA_NOISE";	/* short * */
		break;
	case TEST_PID18_HYBRID_ABS_RAW:  /* int */
		item_name = "ABS_RAW";
		break;
	case TEST_PID22_TRANS_RAW_CAP:	/* short */
		item_name = "TRANS_RAW";
		break;
	case TEST_PID29_HYBRID_ABS_NOISE:	 /*  short */
		item_name = "ABS_NOISE";
		break;
	case TEST_PID65_MISCALDATA_NORMAL:	/* short */
		item_name = "MISCALDATA_NORMAL";
		break;
	}

	for (ii = 0; ii < ts->cols; ii++) {
		if (ii == 0)
			tx_min = tx_max = ts->pFrame[ii];

		tx_min = min(tx_min, ts->pFrame[ii]);
		tx_max = max(tx_max, ts->pFrame[ii]);
	}
	snprintf(buff, sizeof(buff), "%d,%d", tx_min, tx_max);
	snprintf(temp, sizeof(temp), "%s%s", item_name, "_RX");
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, sizeof(buff), temp);

	memset(temp, 0x00, SEC_CMD_STR_LEN);

	for (ii = ts->cols; ii < ts->cols + ts->rows; ii++) {
		if (ii == ts->cols)
			rx_min = rx_max = ts->pFrame[ii];

		rx_min = min(rx_min, ts->pFrame[ii]);
		rx_max = max(rx_max, ts->pFrame[ii]);
	}
	snprintf(buff, sizeof(buff), "%d,%d", rx_min, rx_max);
	snprintf(temp, sizeof(temp), "%s%s", item_name, "_TX");
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, sizeof(buff), temp);

	return 0;
}

/**
 * syna_tcm_run_production_test()
 *
 * Implement the appplication fw command code to request the device to run
 * the production test.
 *
 * Production tests are listed at enum test_code (PID$).
 *
 * @param
 *    [ in] tcm_dev:    the device handle
 *    [ in] test_item:  the requested testing item
 *    [out] tdata:      testing data returned
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
int synaptics_ts_run_production_test(struct synaptics_ts_data *ts,
		unsigned char test_item, struct synaptics_ts_buffer *tdata)
{
	int retval = 0;
	unsigned char resp_code;
	unsigned char test_code;

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Device is not in application fw mode, mode: %x\n", __func__,
			ts->dev_mode);
		return -EINVAL;
	}

	test_code = (unsigned char)test_item;

	retval = ts->write_message(ts,
			SYNAPTICS_TS_CMD_PRODUCTION_TEST,
			&test_code,
			1,
			&resp_code,
			FORCE_ATTN_DRIVEN);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to send command 0x%02x\n",
			SYNAPTICS_TS_CMD_PRODUCTION_TEST);
		goto exit;
	}

	if (tdata == NULL)
		goto exit;

	/* copy testing data to caller */
	retval = synaptics_ts_buf_copy(tdata, &ts->resp_buf);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to copy testing data\n");
		goto exit;
	}

exit:
	return retval;
}

static void synaptics_ts_test_rawdata_read(struct synaptics_ts_data *ts,
		struct sec_cmd_data *sec, struct synaptics_ts_test_mode *mode)
{
	struct synaptics_ts_buffer test_data;
	char temp[SEC_CMD_STR_LEN] = { 0 };
	char *buff;
	char *item_name = "NULL";
	short rawcap[2] = {SHRT_MAX, SHRT_MIN}; /* min, max */
	short rawcap_edge[2] = {SHRT_MAX, SHRT_MIN}; /* min, max */
	int ret;
	int ii, jj;

	switch (mode->type) {
	case TEST_PID05_FULL_RAW_CAP:
		item_name = "FULL_RAWCAP"; /* short * */
		break;
	case TEST_PID10_DELTA_NOISE:
		item_name = "FULL_NOISE";	/* short * */
		break;
	case TEST_PID18_HYBRID_ABS_RAW:  /* int */
		item_name = "ABS_RAW";
		break;
	case TEST_PID22_TRANS_RAW_CAP:	/* short */
		item_name = "TRANS_RAW";
		break;
	case TEST_PID29_HYBRID_ABS_NOISE:	 /*  short */
		item_name = "ABS_NOISE";
		break;
	case TEST_PID65_MISCALDATA_NORMAL:	/* short */
		item_name = "MISCALDATA_NORMAL";
		break;
	default:
		break;
	}

	input_err(true, ts->dev, "%s: [%s] test\n", __func__, item_name);

	buff = kzalloc(ts->cols * ts->rows * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto error_alloc_mem;

	if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
		input_err(true, ts->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		goto error_power_state;
	}

	synaptics_ts_buf_init(&test_data);
	memset(ts->pFrame, 0x00, ts->cols * ts->rows * sizeof(int));

	ret = synaptics_ts_run_production_test(ts, mode->type, &test_data);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: failed to test\n",
			__func__);
		goto error_test_fail;
	}

	if (mode->frame_channel)
		synaptics_ts_print_channel(ts, &test_data, &mode->min, &mode->max, mode->type);
	else
		synaptics_ts_print_frame(ts, &test_data, &mode->min, &mode->max, mode->type);

	if (mode->allnode) {
		if (mode->frame_channel) {
			for (ii = 0; ii < (ts->cols + ts->rows); ii++) {
				snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", ts->pFrame[ii]);
				strlcat(buff, temp, ts->cols * ts->rows * CMD_RESULT_WORD_LEN);

				memset(temp, 0x00, SEC_CMD_STR_LEN);
			}
		} else {
			for (ii = 0; ii < (ts->cols * ts->rows); ii++) {
				snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", ts->pFrame[ii]);
				strlcat(buff, temp, ts->cols * ts->rows * CMD_RESULT_WORD_LEN);

				memset(temp, 0x00, SEC_CMD_STR_LEN);
			}
		}
	} else {
		snprintf(buff, SEC_CMD_STR_LEN, "%d,%d", mode->min, mode->max);
	}

	if (!sec)
		goto out_rawdata;

	if (mode->type == TEST_PID22_TRANS_RAW_CAP && !mode->frame_channel) {
		char rawcap_buff[SEC_CMD_STR_LEN];

		for (ii = 0; ii < ts->cols; ii++) {
			for (jj = 0; jj < ts->rows; jj++) {
				short *rawcap_ptr;

				if (ii == 0 || ii == ts->cols - 1 || jj == 0 || jj == ts->rows - 1)
					rawcap_ptr = rawcap_edge;
				else
					rawcap_ptr = rawcap;

				if (ts->pFrame[jj + (ii * ts->rows)] < rawcap_ptr[0])
					rawcap_ptr[0] = ts->pFrame[jj + (ii * ts->rows)];
				if (ts->pFrame[jj + (ii * ts->rows)] > rawcap_ptr[1])
					rawcap_ptr[1] = ts->pFrame[jj + (ii * ts->rows)];
			}
		}

		input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev,
				"%s: rawcap:%d,%d rawcap_edge:%d,%d\n",
				__func__, rawcap[0], rawcap[1], rawcap_edge[0], rawcap_edge[1]);
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
			snprintf(rawcap_buff, SEC_CMD_STR_LEN, "%d,%d", rawcap[0], rawcap[1]);
			sec_cmd_set_cmd_result_all(sec, rawcap_buff, SEC_CMD_STR_LEN, "TSP_RAWCAP");
			snprintf(rawcap_buff, SEC_CMD_STR_LEN, "%d,%d", rawcap_edge[0], rawcap_edge[1]);
			sec_cmd_set_cmd_result_all(sec, rawcap_buff, SEC_CMD_STR_LEN, "TSP_RAWCAP_EDGE");
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->cols * ts->rows * CMD_RESULT_WORD_LEN));

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff,
				strnlen(buff, ts->cols * ts->rows * CMD_RESULT_WORD_LEN), item_name);

	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev, "%s: %d, %s\n",
			__func__, mode->type, mode->allnode ? "ALL" : "");

out_rawdata:
	synaptics_ts_buf_release(&test_data);
	kfree(buff);

	synaptics_ts_locked_release_all_finger(ts);

	return;
error_test_fail:
	synaptics_ts_buf_release(&test_data);
error_power_state:
	kfree(buff);
error_alloc_mem:
	if (!sec)
		return;

	snprintf(temp, SEC_CMD_STR_LEN, "NG");
	sec_cmd_set_cmd_result(sec, temp, SEC_CMD_STR_LEN);
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, temp, SEC_CMD_STR_LEN, item_name);
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	synaptics_ts_locked_release_all_finger(ts);

}

/**************************************************************************************************/
/* for bigdata */
/* read param */
static ssize_t hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[SEC_INPUT_HW_PARAM_SIZE];
	char tbuff[SEC_CMD_STR_LEN];
	char mdev[SEC_CMD_STR_LEN];

	memset(mdev, 0x00, sizeof(mdev));
	if (GET_DEV_COUNT(ts->multi_dev) == MULTI_DEV_SUB)
		snprintf(mdev, sizeof(mdev), "%s", "2");
	else
		snprintf(mdev, sizeof(mdev), "%s", "");

	memset(buff, 0x00, sizeof(buff));

	sec_input_get_common_hw_param(ts->plat_data, buff);

	/* module_id */
	memset(tbuff, 0x00, sizeof(tbuff));

	snprintf(tbuff, sizeof(tbuff), ",\"TMOD%s\":\"SY%02X%02X%02X%c%01X\"",
			mdev, ts->plat_data->img_version_of_bin[1], ts->plat_data->img_version_of_bin[2],
			ts->plat_data->img_version_of_bin[3],
#ifdef TCLM_CONCEPT
			ts->tdata->tclm_string[ts->tdata->nvdata.cal_position].s_name,
			ts->tdata->nvdata.cal_count & 0xF);
#else
			'0', 0);
#endif
	strlcat(buff, tbuff, sizeof(buff));

	/* vendor_id */
	memset(tbuff, 0x00, sizeof(tbuff));
	if (ts->plat_data->img_version_of_ic[0] == 0x31)
		snprintf(tbuff, sizeof(tbuff), ",\"TVEN%s\":\"SYNA_S3908\"", mdev);
	else if (ts->plat_data->img_version_of_ic[0] == 0x32)
		snprintf(tbuff, sizeof(tbuff), ",\"TVEN%s\":\"SYNA_S3907\"", mdev);
	else if (ts->plat_data->img_version_of_ic[0] == 0x33)
		snprintf(tbuff, sizeof(tbuff), ",\"TVEN%s\":\"SYNA_S3916\"", mdev);
	else if (ts->plat_data->img_version_of_ic[0] == 0x34)
		snprintf(tbuff, sizeof(tbuff), ",\"TVEN%s\":\"SYNA_S3908S\"", mdev);
	else
		snprintf(tbuff, sizeof(tbuff), ",\"TVEN%s\":\"SYNA\"", mdev);

	strlcat(buff, tbuff, sizeof(buff));

	input_info(true, ts->dev, "%s: %s\n", __func__, buff);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

/* clear param */
static ssize_t hw_param_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	sec_input_clear_common_hw_param(ts->plat_data);

	return count;
}

#define SENSITIVITY_POINT_CNT	9	/* ~ davinci : 5 ea => 9 ea */
static ssize_t sensitivity_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int value[SENSITIVITY_POINT_CNT] = { 0 };
	int ret, i;
	char tempv[10] = { 0 };
	char buff[SENSITIVITY_POINT_CNT * 10] = { 0 };
	struct synaptics_ts_buffer test_data;
	unsigned short *data_ptr = NULL;

	synaptics_ts_buf_init(&test_data);

	ret = synaptics_ts_run_production_test(ts,
			TEST_PID60_SENSITIVITY,
			&test_data);
	if (ret < 0) {
		input_err(true, ts->dev, "Fail to run test %d\n",
		TEST_PID54_BSC_DIFF_TEST);
		synaptics_ts_buf_release(&test_data);
		return ret;
	}

	data_ptr = (short *)&test_data.buf[0];
	for (i = 0; i < SENSITIVITY_POINT_CNT; i++) {
		value[i] = *data_ptr;
		if (i != 0)
			strlcat(buff, ",", sizeof(buff));
		snprintf(tempv, 10, "%d", value[i]);
		strlcat(buff, tempv, sizeof(buff));
		data_ptr++;
	}

	input_info(true, ts->dev, "%s: sensitivity mode : %s\n", __func__, buff);
	synaptics_ts_buf_release(&test_data);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

static ssize_t sensitivity_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;
	unsigned long value = 0;

	if (count > 2)
		return -EINVAL;

	ret = kstrtoul(buf, 10, &value);
	if (ret != 0)
		return ret;

	if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
		input_err(true, ts->dev, "%s: power off in IC\n", __func__);
		return 0;
	}

	input_err(true, ts->dev, "%s: enable:%ld\n", __func__, value);

	input_info(true, ts->dev, "%s: done\n", __func__);

	return count;
}

ssize_t get_lp_dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	u8 string_data[10] = {0, };
	u16 current_index;
	u16 dump_start, dump_end, dump_cnt;
	int i, ret, dump_area, dump_gain, block_cnt, block_size;
	unsigned char *sec_spg_dat;
	unsigned char *r_buf = NULL;
	int size[2] = { 0 };
	int read_offset[2] = { 0 };
	u8 data = 0;
	u16 string_addr;
	char dev_buff[10] = {0, };

	if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
		input_err(true, ts->dev, "%s: Touch is stopped!\n", __func__);
		if (buf)
			return snprintf(buf, SEC_CMD_BUF_SIZE, "TSP turned off");
		else
			return 0;
	}

	if (atomic_read(&ts->reset_is_on_going)) {
		input_err(true, ts->dev, "%s: Reset is ongoing!\n", __func__);
		if (buf)
			return snprintf(buf, SEC_CMD_BUF_SIZE, "Reset is ongoing");
		else
			return 0;
	}

	if (IS_FOLD_DEV(ts->multi_dev))
		snprintf(dev_buff, sizeof(dev_buff), "%4s: ", ts->multi_dev->name);
	else
		snprintf(dev_buff, sizeof(dev_buff), "");

	/* preparing dump buffer */
	sec_spg_dat = vmalloc(SEC_TS_MAX_SPONGE_DUMP_BUFFER);
	if (!sec_spg_dat) {
		if (buf)
			return snprintf(buf, SEC_CMD_BUF_SIZE, "vmalloc failed");
		else
			return 0;
	}
	memset(sec_spg_dat, 0, SEC_TS_MAX_SPONGE_DUMP_BUFFER);

	ret = ts->synaptics_ts_read_sponge(ts, string_data, 2, SEC_TS_CMD_SPONGE_LP_DUMP_CUR_IDX, 2);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Failed to read rect\n", __func__);
		if (buf)
			snprintf(buf, SEC_CMD_BUF_SIZE, "NG, Failed to read rect");
		goto out;
	}

	if (ts->sponge_inf_dump)
		dump_gain = 2;
	else
		dump_gain = 1;

	current_index = (string_data[1] & 0xFF) << 8 | (string_data[0] & 0xFF);
	dump_start = SEC_TS_CMD_SPONGE_LP_DUMP_EVENT;
	dump_end = dump_start + (ts->sponge_dump_format * ((ts->sponge_dump_event * dump_gain) - 1));

	if (current_index == 0) {
		input_info(true, ts->dev, "%s: length 0\n", __func__);
		if (buf)
			snprintf(buf, SEC_CMD_BUF_SIZE, "length 0");
		goto out;
	}

	if (current_index > dump_end || current_index < dump_start) {
		input_err(true, ts->dev,
				"Failed to Sponge LP log %d\n", current_index);
		if (buf)
			snprintf(buf, SEC_CMD_BUF_SIZE,
					"NG, Failed to Sponge LP log, current_index=%d",
					current_index);
		goto out;
	}

	/* legacy get_lp_dump */
	input_info(true, ts->dev, "%s: DEBUG format=%d, num=%d, start=%d, end=%d, current_index=%d\n",
			__func__, ts->sponge_dump_format, ts->sponge_dump_event, dump_start, dump_end, current_index);

	size[0] = dump_end - current_index;
	size[1] = (current_index + ts->sponge_dump_format) - dump_start;
	read_offset[0] = current_index + ts->sponge_dump_format;
	read_offset[1] = dump_start;
	block_size = max(size[0], size[1]) + 1;

	r_buf = kzalloc(block_size, GFP_KERNEL);
	if (!r_buf) {
		if (buf)
			snprintf(buf, SEC_CMD_BUF_SIZE, "alloc r_buf failed, block_size:%d", block_size);
		goto out;
	}

	for (block_cnt = 0; block_cnt < 2; block_cnt++) {
		input_info(true, ts->dev, "%s: block%d - size:%d, read_offset:%d\n",
				__func__, block_cnt, size[block_cnt], read_offset[block_cnt]);

		if (size[block_cnt] == 0)
			continue;

		memset(r_buf, 0x0, block_size);

		ret = ts->synaptics_ts_read_sponge(ts, r_buf, block_size, read_offset[block_cnt], size[block_cnt]);
		if (ret < 0) {
			input_err(true, ts->dev, "%s: Failed to read data for block%d, size:%d, from %d\n",
					__func__, block_cnt, size[block_cnt], read_offset[block_cnt]);
			if (buf)
				snprintf(buf, SEC_CMD_BUF_SIZE, "NG, Failed to read data for block%d, size:%d, from %d",
					block_cnt, size[block_cnt], read_offset[block_cnt]);
			goto out;
		}

		string_addr = read_offset[block_cnt];
		for (i = string_addr; i < size[block_cnt] + read_offset[block_cnt]; i += ts->sponge_dump_format) {
			char buff[40] = {0, };
			u16 data0, data1, data2, data3, data4;

			data0 = (r_buf[i + 1 - string_addr] & 0xFF) << 8 | (r_buf[i + 0 - string_addr] & 0xFF);
			data1 = (r_buf[i + 3 - string_addr] & 0xFF) << 8 | (r_buf[i + 2 - string_addr] & 0xFF);
			data2 = (r_buf[i + 5 - string_addr] & 0xFF) << 8 | (r_buf[i + 4 - string_addr] & 0xFF);
			data3 = (r_buf[i + 7 - string_addr] & 0xFF) << 8 | (r_buf[i + 6 - string_addr] & 0xFF);
			data4 = (r_buf[i + 9 - string_addr] & 0xFF) << 8 | (r_buf[i + 8 - string_addr] & 0xFF);

			if (data0 || data1 || data2 || data3 || data4) {
				if (ts->sponge_dump_format == 10) {
					snprintf(buff, sizeof(buff),
							"%s%03d: %04x%04x%04x%04x%04x\n",
							dev_buff, i, data0, data1, data2, data3, data4);
				} else {
					snprintf(buff, sizeof(buff),
							"%s%03d: %04x%04x%04x%04x\n",
							dev_buff, i, data0, data1, data2, data3);
				}
				if (buf)
					strlcat(buf, buff, PAGE_SIZE);
#if IS_ENABLED(CONFIG_SEC_DEBUG_TSP_LOG)
				if (!ts->sponge_inf_dump)
					sec_tsp_sponge_log(buff);
#endif
			}
		}
	}

	if (ts->sponge_inf_dump) {
		if (current_index >= ts->sponge_dump_border) {
			dump_cnt = ((current_index - (ts->sponge_dump_border)) / ts->sponge_dump_format) + 1;
			dump_area = 1;
			sec_spg_dat[0] = ts->sponge_dump_border_lsb;
			sec_spg_dat[1] = ts->sponge_dump_border_msb;
		} else {
			dump_cnt = ((current_index - SEC_TS_CMD_SPONGE_LP_DUMP_EVENT) / ts->sponge_dump_format) + 1;
			dump_area = 0;
			sec_spg_dat[0] = SEC_TS_CMD_SPONGE_LP_DUMP_EVENT;
			sec_spg_dat[1] = 0;
		}

		ret = ts->synaptics_ts_read_sponge(ts, sec_spg_dat, dump_cnt * ts->sponge_dump_format,
					(sec_spg_dat[1] & 0xFF) << 8 | (sec_spg_dat[0] & 0xFF), dump_cnt * ts->sponge_dump_format);
		if (ret < 0) {
			input_err(true, ts->dev, "%s: Failed to read sponge\n", __func__);
			goto out;
		}

		for (i = 0 ; i <= dump_cnt ; i++) {
			int e_offset = i * ts->sponge_dump_format;
			char ibuff[40] = {0, };
			u16 edata[5];

			edata[0] = (sec_spg_dat[1 + e_offset] & 0xFF) << 8 | (sec_spg_dat[0 + e_offset] & 0xFF);
			edata[1] = (sec_spg_dat[3 + e_offset] & 0xFF) << 8 | (sec_spg_dat[2 + e_offset] & 0xFF);
			edata[2] = (sec_spg_dat[5 + e_offset] & 0xFF) << 8 | (sec_spg_dat[4 + e_offset] & 0xFF);
			edata[3] = (sec_spg_dat[7 + e_offset] & 0xFF) << 8 | (sec_spg_dat[6 + e_offset] & 0xFF);
			edata[4] = (sec_spg_dat[9 + e_offset] & 0xFF) << 8 | (sec_spg_dat[8 + e_offset] & 0xFF);

			if (edata[0] || edata[1] || edata[2] || edata[3] || edata[4]) {
				snprintf(ibuff, sizeof(ibuff), "%s%03d: %04x%04x%04x%04x%04x\n",
						dev_buff, i + (ts->sponge_dump_event * dump_area),
						edata[0], edata[1], edata[2], edata[3], edata[4]);
#if IS_ENABLED(CONFIG_SEC_DEBUG_TSP_LOG)
				sec_tsp_sponge_log(ibuff);
#endif
			}
		}

		ts->sponge_dump_delayed_flag = false;
		data = ts->plat_data->sponge_mode |= SEC_TS_MODE_SPONGE_INF_DUMP_CLEAR;
		ret = ts->synaptics_ts_write_sponge(ts, &data, 1, 0x01, 1);
		if (ret < 0)
			input_err(true, ts->dev, "%s: Failed to clear sponge dump\n", __func__);
		ts->plat_data->sponge_mode &= ~SEC_TS_MODE_SPONGE_INF_DUMP_CLEAR;
	}
out:
	vfree(sec_spg_dat);
	if (r_buf)
		kfree(r_buf);
	if (buf)
		return strlen(buf);
	else
		return 0;
}

static ssize_t fod_pos_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[3] = { 0 };
	int i, ret;

	if (!ts->plat_data->support_fod) {
		input_err(true, ts->dev, "%s: fod is not supported\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "NA");
	}

	if (!ts->plat_data->fod_data.vi_size) {
		input_err(true, ts->dev, "%s: not read fod_info yet\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "NG");
	}

	if (ts->plat_data->fod_data.vi_event == CMD_SYSFS) {
		ret = ts->synaptics_ts_read_sponge(ts, ts->plat_data->fod_data.vi_data, ts->plat_data->fod_data.vi_size,
				SEC_TS_CMD_SPONGE_FOD_POSITION, ts->plat_data->fod_data.vi_size);
		if (ret < 0) {
			input_err(true, ts->dev, "%s: Failed to read\n", __func__);
			return snprintf(buf, SEC_CMD_BUF_SIZE, "NG");
		}
	}

	for (i = 0; i < ts->plat_data->fod_data.vi_size; i++) {
		snprintf(buff, 3, "%02X", ts->plat_data->fod_data.vi_data[i]);
		strlcat(buf, buff, SEC_CMD_BUF_SIZE);
	}

	return strlen(buf);
}

static ssize_t fod_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	return sec_input_get_fod_info(ts->dev, buf);
}

static ssize_t aod_active_area_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	input_info(true, ts->dev, "%s: top:%d, edge:%d, bottom:%d\n",
		 __func__, ts->plat_data->aod_data.active_area[0],
		 ts->plat_data->aod_data.active_area[1], ts->plat_data->aod_data.active_area[2]);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d,%d,%d",
		 ts->plat_data->aod_data.active_area[0], ts->plat_data->aod_data.active_area[1],
		 ts->plat_data->aod_data.active_area[2]);
}

static ssize_t virtual_prox_show(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int retval;

	retval = (ts->hover_event != 3) ? 0 : 3;

	input_info(true, ts->dev, "%s: hover_event: %d, retval: %d\n", __func__, ts->hover_event, retval);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", retval);
}

static ssize_t virtual_prox_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;
	u8 data;

	if (!ts->plat_data->support_ear_detect) {
		input_err(true, ts->dev, "%s: ear detection is not supported\n", __func__);
		return -ENODEV;
	}

	ret = kstrtou8(buf, 8, &data);
	if (ret < 0)
		return ret;

	if (data != 0 && data != 1 && data != 3) {
		input_err(true, ts->dev, "%s: %d is wrong param\n", __func__, data);
		return -EINVAL;
	}

	ts->plat_data->ed_enable = data;
	synaptics_ts_ear_detect_enable(ts, ts->plat_data->ed_enable);
	input_info(true, ts->dev, "%s: %d\n", __func__, data);

	return count;
}

static ssize_t debug_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[INPUT_DEBUG_INFO_SIZE];
	char tbuff[128];

	if (sizeof(tbuff) > INPUT_DEBUG_INFO_SIZE)
		input_info(true, ts->dev, "%s: buff_size[%d], tbuff_size[%ld]\n",
				__func__, INPUT_DEBUG_INFO_SIZE, sizeof(tbuff));

	memset(buff, 0x00, sizeof(buff));
	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "FW: (I)%02X%02X%02X%02X / (B)%02X%02X%02X%02X\n",
			ts->plat_data->img_version_of_ic[0], ts->plat_data->img_version_of_ic[1],
			ts->plat_data->img_version_of_ic[2], ts->plat_data->img_version_of_ic[3],
			ts->plat_data->img_version_of_bin[0], ts->plat_data->img_version_of_bin[1],
			ts->plat_data->img_version_of_bin[2], ts->plat_data->img_version_of_bin[3]);
	strlcat(buff, tbuff, sizeof(buff));

#ifdef TCLM_CONCEPT
	if (ts->tdata->tclm_level != TCLM_LEVEL_NOT_SUPPORT) {
		memset(tbuff, 0x00, sizeof(tbuff));
		snprintf(tbuff, sizeof(tbuff), "C%02XT%04X.%4s%s Cal_flag:%d fail_cnt:%d\n",
			ts->tdata->nvdata.cal_count, ts->tdata->nvdata.tune_fix_ver,
			ts->tdata->tclm_string[ts->tdata->nvdata.cal_position].f_name,
			(ts->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L" : " ",
			ts->tdata->nvdata.cal_fail_flag, ts->tdata->nvdata.cal_fail_cnt);
		strlcat(buff, tbuff, sizeof(buff));
	}
#endif

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "ic_reset_count %d\nchecksum bit %d\n",
			ts->plat_data->hw_param.ic_reset_count,
			ts->plat_data->hw_param.checksum_result);
	strlcat(buff, tbuff, sizeof(buff));

	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "EVENT_STATUS: WET:%d GLOVE:%d GAME:%d NOISE:%d COVER:%d NOTE:%d TA:%d WIRELESS:%d SIP:%d LS:%d PK:%d CALL:%d\n",
				ts->plat_data->ic_mode_status.wet, ts->plat_data->ic_mode_status.glove,
				ts->plat_data->ic_mode_status.game, ts->plat_data->ic_mode_status.noise,
				ts->plat_data->ic_mode_status.cover, ts->plat_data->ic_mode_status.note,
				ts->plat_data->ic_mode_status.ta, ts->plat_data->ic_mode_status.wireless,
				ts->plat_data->ic_mode_status.sip, ts->plat_data->ic_mode_status.lowsen,
				ts->plat_data->ic_mode_status .pocket, ts->plat_data->ic_mode_status.call);
	strlcat(buff, tbuff, sizeof(buff));

	if (ts->mode) {
		int count = 0, i = 0;

		memset(tbuff, 0x00, sizeof(tbuff));
		snprintf(tbuff, sizeof(tbuff), "- dynamic config list\n");
		strlcat(buff, tbuff, sizeof(buff));
		snprintf(tbuff, sizeof(tbuff), "  mode\t| drv\t/ ic\t| wait\t| mismatch\n");
		strlcat(buff, tbuff, sizeof(buff));
		for (i = 0; i < DC_MAX; i++) {
			if (ts->mode[i].drv_setting != 0 || ts->mode[i].ic_state != 0 ||
					ts->mode[i].drv_setting != ts->mode[i].ic_state) {
				snprintf(tbuff, sizeof(tbuff), "  0x%02x\t| %d\t/ %d\t| %d\t| %d\n",
						i, ts->mode[i].drv_setting, ts->mode[i].ic_state,
						ts->mode[i].is_waiting,
						(ts->mode[i].drv_setting != ts->mode[i].ic_state) ? 1 : 0);
				strlcat(buff, tbuff, sizeof(buff));
				count++;
			}
		}
		if (count == 0) {
			snprintf(tbuff, sizeof(tbuff), "  none\n");
			strlcat(buff, tbuff, sizeof(buff));
		}
	}

	sec_input_get_multi_device_debug_info(ts->multi_dev, buff, sizeof(buff));

	input_info(true, ts->dev, "%s: size: %ld\n", __func__, strlen(buff));

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

static DEVICE_ATTR_RW(hw_param);
static DEVICE_ATTR_RO(get_lp_dump);
static DEVICE_ATTR_RW(sensitivity_mode);
static DEVICE_ATTR_RO(fod_pos);
static DEVICE_ATTR_RO(fod_info);
static DEVICE_ATTR_RO(aod_active_area);
static DEVICE_ATTR_RW(virtual_prox);
static DEVICE_ATTR_RO(debug_info);

static struct attribute *cmd_attributes[] = {
	&dev_attr_hw_param.attr,
	&dev_attr_get_lp_dump.attr,
	&dev_attr_sensitivity_mode.attr,
	&dev_attr_fod_pos.attr,
	&dev_attr_fod_info.attr,
	&dev_attr_aod_active_area.attr,
	&dev_attr_virtual_prox.attr,
	&dev_attr_debug_info.attr,
	NULL,
};

static struct attribute_group cmd_attr_group = {
	.attrs = cmd_attributes,
};

static void fw_update(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int retval = 0;

	mutex_lock(&ts->plat_data->enable_mutex);
	retval = synaptics_ts_fw_update_on_hidden_menu(ts, sec->cmd_param[0]);
	if (retval < 0) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_err(true, ts->dev, "%s: failed [%d]\n", __func__, retval);
	} else {
		sec->cmd_state = SEC_CMD_STATUS_OK;
		input_info(true, ts->dev, "%s: success [%d]\n", __func__, retval);
	}
	mutex_unlock(&ts->plat_data->enable_mutex);
}

static void get_fw_ver_bin(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[16] = { 0 };

	snprintf(buff, sizeof(buff), "SY%02X%02X%02X%02X",
			ts->plat_data->img_version_of_bin[0], ts->plat_data->img_version_of_bin[1],
			ts->plat_data->img_version_of_bin[2], ts->plat_data->img_version_of_bin[3]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_BIN");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s: %s\n", __func__, buff);
}

static void get_fw_ver_ic(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[16] = { 0 };
	char model[16] = { 0 };
	int ret;
	int idx;

	ret = synaptics_ts_get_app_info(ts, &ts->app_info);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Fail to get application info\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	for (idx = 0; idx < 4; idx++)
		ts->plat_data->img_version_of_ic[idx] = ts->config_id[idx];

	snprintf(buff, sizeof(buff), "SY%02X%02X%02X%02X",
			ts->plat_data->img_version_of_ic[0], ts->plat_data->img_version_of_ic[1],
			ts->plat_data->img_version_of_ic[2], ts->plat_data->img_version_of_ic[3]);
	snprintf(model, sizeof(model), "SY%02X%02X",
			ts->plat_data->img_version_of_ic[0], ts->plat_data->img_version_of_ic[1]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_IC");
		sec_cmd_set_cmd_result_all(sec, model, strnlen(model, sizeof(model)), "FW_MODEL");
	}
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_vendor(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[16] = { 0 };

	strncpy(buff, "SYNAPTICS", sizeof(buff));
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_VENDOR");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_name(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[16] = { 0 };

	if (ts->plat_data->img_version_of_ic[0] == 0x31)
		strncpy(buff, "S3908", sizeof(buff));
	else if (ts->plat_data->img_version_of_ic[0] == 0x32)
		strncpy(buff, "S3907", sizeof(buff));
	else if (ts->plat_data->img_version_of_ic[0] == 0x33)
		strncpy(buff, "S3916A", sizeof(buff));
	else if (ts->plat_data->img_version_of_ic[0] == 0x34)
		strncpy(buff, "S3908S", sizeof(buff));
	else if (ts->plat_data->img_version_of_ic[0] == 0x35)
		strncpy(buff, "S3916T", sizeof(buff));
	else if (ts->plat_data->img_version_of_ic[0] == 0x36)
		strncpy(buff, "S3908T", sizeof(buff));
	else
		strncpy(buff, "N/A", sizeof(buff));

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_NAME");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s: %s\n", __func__, buff);
}

static void get_x_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[16] = { 0 };

	snprintf(buff, sizeof(buff), "%d", ts->rows);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s: %s\n", __func__, buff);
}

static void get_y_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[16] = { 0 };

	snprintf(buff, sizeof(buff), "%d", ts->cols);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s: %s\n", __func__, buff);
}

static void module_off_master(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret = 0;

	ret = ts->plat_data->stop_device(ts);
	if (ret == 0)
		sec->cmd_state = SEC_CMD_STATUS_OK;
	else
		sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, ts->dev, "%s: %d\n", __func__, ret);
}

static void module_on_master(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret = 0;

	ret = ts->plat_data->start_device(ts);
	if (ret == 0)
		sec->cmd_state = SEC_CMD_STATUS_OK;
	else
		sec->cmd_state = SEC_CMD_STATUS_FAIL;

	ts->plat_data->init(ts);

	input_info(true, ts->dev, "%s: %d\n", __func__, ret);
}

static void get_crc_check(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	struct synaptics_ts_message_data_blob *tcm_msg = NULL;
	unsigned char resp_code;
	int ret;

	ret = ts->write_message(ts,
			SYNAPTICS_TS_CMD_IDENTIFY,
			NULL,
			0,
			&resp_code,
			0);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: failed to write identify cmd\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	tcm_msg = &ts->msg_data;

	synaptics_ts_buf_lock(&tcm_msg->in);

	ret = synaptics_ts_v1_parse_idinfo(ts,
			&tcm_msg->in.buf[SYNAPTICS_TS_MESSAGE_HEADER_SIZE],
			tcm_msg->in.buf_size - SYNAPTICS_TS_MESSAGE_HEADER_SIZE,
			tcm_msg->payload_length);
	if (ret < 0) {
		input_info(true, ts->dev, "%s:Fail to identify device\n", __func__);
		synaptics_ts_buf_unlock(&tcm_msg->in);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	synaptics_ts_buf_unlock(&tcm_msg->in);

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_info(true, ts->dev, "%s: in 0x%02X mode\n", __func__, ts->dev_mode);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		input_info(true, ts->dev, "%s: in application mode\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
}

static void run_full_rawcap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	struct synaptics_ts_test_mode mode;

	memset(&mode, 0x00, sizeof(struct synaptics_ts_test_mode));
	mode.type = TEST_PID05_FULL_RAW_CAP;

	synaptics_ts_test_rawdata_read(ts, sec, &mode);
}

static void run_full_rawcap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	struct synaptics_ts_test_mode mode;

	memset(&mode, 0x00, sizeof(struct synaptics_ts_test_mode));
	mode.type = TEST_PID05_FULL_RAW_CAP;
	mode.allnode = TEST_MODE_ALL_NODE;

	synaptics_ts_test_rawdata_read(ts, sec, &mode);
}

static void run_trans_rawcap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	struct synaptics_ts_test_mode mode;

	memset(&mode, 0x00, sizeof(struct synaptics_ts_test_mode));
	mode.type = TEST_PID22_TRANS_RAW_CAP;

	synaptics_ts_test_rawdata_read(ts, sec, &mode);
}

static void run_trans_rawcap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	struct synaptics_ts_test_mode mode;

	memset(&mode, 0x00, sizeof(struct synaptics_ts_test_mode));
	mode.type = TEST_PID22_TRANS_RAW_CAP;
	mode.allnode = TEST_MODE_ALL_NODE;

	synaptics_ts_test_rawdata_read(ts, sec, &mode);
}

static void run_delta_noise_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	struct synaptics_ts_test_mode mode;

	memset(&mode, 0x00, sizeof(struct synaptics_ts_test_mode));
	mode.type = TEST_PID10_DELTA_NOISE;

	synaptics_ts_test_rawdata_read(ts, sec, &mode);
}

static void run_delta_noise_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	struct synaptics_ts_test_mode mode;

	memset(&mode, 0x00, sizeof(struct synaptics_ts_test_mode));
	mode.type = TEST_PID10_DELTA_NOISE;
	mode.allnode = TEST_MODE_ALL_NODE;

	synaptics_ts_test_rawdata_read(ts, sec, &mode);
}

static void run_abs_rawcap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	struct synaptics_ts_test_mode mode;

	memset(&mode, 0x00, sizeof(struct synaptics_ts_test_mode));
	mode.type = TEST_PID18_HYBRID_ABS_RAW;
	mode.frame_channel = TEST_MODE_READ_CHANNEL;

	synaptics_ts_test_rawdata_read(ts, sec, &mode);
}

static void run_abs_rawcap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	struct synaptics_ts_test_mode mode;

	memset(&mode, 0x00, sizeof(struct synaptics_ts_test_mode));
	mode.type = TEST_PID18_HYBRID_ABS_RAW;
	mode.allnode = TEST_MODE_ALL_NODE;
	mode.frame_channel = TEST_MODE_READ_CHANNEL;

	synaptics_ts_test_rawdata_read(ts, sec, &mode);
}

static void run_abs_noise_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	struct synaptics_ts_test_mode mode;

	memset(&mode, 0x00, sizeof(struct synaptics_ts_test_mode));
	mode.type = TEST_PID29_HYBRID_ABS_NOISE;
	mode.frame_channel = TEST_MODE_READ_CHANNEL;

	synaptics_ts_test_rawdata_read(ts, sec, &mode);
}

static void run_abs_noise_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	struct synaptics_ts_test_mode mode;

	memset(&mode, 0x00, sizeof(struct synaptics_ts_test_mode));
	mode.type = TEST_PID29_HYBRID_ABS_NOISE;
	mode.frame_channel = TEST_MODE_READ_CHANNEL;
	mode.allnode = TEST_MODE_ALL_NODE;

	synaptics_ts_test_rawdata_read(ts, sec, &mode);
}

static int get_gap_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[16] = { 0 };
	int ii;
	int node_gap_tx = 0;
	int node_gap_rx = 0;
	int tx_max = 0;
	int rx_max = 0;

	for (ii = 0; ii < (ts->rows * ts->cols); ii++) {
		if ((ii + 1) % (ts->rows) != 0) {
			node_gap_tx = (abs(ts->pFrame[ii + 1] - ts->pFrame[ii])) * 100 / max(ts->pFrame[ii], ts->pFrame[ii + 1]);
			tx_max = max(tx_max, node_gap_tx);
		}

		if (ii < (ts->cols - 1) * ts->rows) {
			node_gap_rx = (abs(ts->pFrame[ii + ts->rows] - ts->pFrame[ii])) * 100 / max(ts->pFrame[ii], ts->pFrame[ii + ts->rows]);
			rx_max = max(rx_max, node_gap_rx);
		}
	}

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		snprintf(buff, sizeof(buff), "%d,%d", 0, tx_max);
		sec_cmd_set_cmd_result_all(sec, buff, SEC_CMD_STR_LEN, "GAP_DATA_TX");
		snprintf(buff, sizeof(buff), "%d,%d", 0, rx_max);
		sec_cmd_set_cmd_result_all(sec, buff, SEC_CMD_STR_LEN, "GAP_DATA_RX");
		snprintf(buff, sizeof(buff), "%d,%d", 0, max(tx_max, rx_max));
		sec_cmd_set_cmd_result_all(sec, buff, SEC_CMD_STR_LEN, "GAP_DATA");
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;
	return 0;
}

static void get_gap_data_x_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char *buff = NULL;
	int ii;
	int node_gap = 0;
	char temp[SEC_CMD_STR_LEN] = { 0 };

	buff = kzalloc(ts->cols * ts->rows * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	for (ii = 0; ii < (ts->rows * ts->cols); ii++) {
		if ((ii + 1) % (ts->rows) != 0) {
			node_gap = (abs(ts->pFrame[ii + 1] - ts->pFrame[ii])) * 100 / max(ts->pFrame[ii], ts->pFrame[ii + 1]);
			snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", node_gap);
			strlcat(buff, temp, ts->cols * ts->rows * CMD_RESULT_WORD_LEN);
			memset(temp, 0x00, SEC_CMD_STR_LEN);
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->cols * ts->rows * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);
}

static void get_gap_data_y_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char *buff = NULL;
	int ii;
	int node_gap = 0;
	char temp[SEC_CMD_STR_LEN] = { 0 };

	buff = kzalloc(ts->cols * ts->rows * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	for (ii = 0; ii < (ts->rows * ts->cols); ii++) {
		if (ii < (ts->cols - 1) * ts->rows) {
			node_gap = (abs(ts->pFrame[ii + ts->rows] - ts->pFrame[ii])) * 100 / max(ts->pFrame[ii], ts->pFrame[ii + ts->rows]);

			snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", node_gap);
			strlcat(buff, temp, ts->cols * ts->rows * CMD_RESULT_WORD_LEN);
			memset(temp, 0x00, SEC_CMD_STR_LEN);
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->cols * ts->rows * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);
}

#define GAP_MULTIPLE_VAL	100
static void get_gap_data_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char *buff = NULL;
	int ii;
	char temp[SEC_CMD_STR_LEN] = { 0 };
	int node_gap_tx = 0;
	int node_gap_rx = 0;
	int node_gap_max = 0;

	buff = kzalloc(ts->cols * ts->rows * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	for (ii = 0; ii < (ts->rows * ts->cols); ii++) {
		node_gap_tx = node_gap_rx = node_gap_max = 0;

		/* rx(x) gap max */
		if ((ii + 1) % (ts->rows) != 0)
			node_gap_tx = 1 * GAP_MULTIPLE_VAL - (min(ts->pFrame[ii], ts->pFrame[ii + 1]) * GAP_MULTIPLE_VAL) / (max(ts->pFrame[ii], ts->pFrame[ii + 1]));

		/* tx(y) gap max */
		if (ii < (ts->cols - 1) * ts->rows)
			node_gap_rx = 1 * GAP_MULTIPLE_VAL - (min(ts->pFrame[ii], ts->pFrame[ii + ts->rows]) * GAP_MULTIPLE_VAL) / (max(ts->pFrame[ii], ts->pFrame[ii + ts->rows]));

		node_gap_max = max(node_gap_tx, node_gap_rx);

		snprintf(temp, sizeof(temp), "%d,", node_gap_max);
		strlcat(buff, temp, ts->cols * ts->rows * 6);
		memset(temp, 0x00, CMD_RESULT_WORD_LEN);
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->cols * ts->rows * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);
}

static void run_interrupt_gpio_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	unsigned char *buf = NULL;
	int ret, status1, status2;
	unsigned char command[3] = {0x00, 0x00, 0x00};

	synaptics_ts_enable_report(ts, REPORT_SEC_COORDINATE_EVENT, false);
	synaptics_ts_enable_report(ts, REPORT_SEC_STATUS_EVENT, false);

	buf = synaptics_ts_pal_mem_alloc(2, sizeof(struct synaptics_ts_identification_info));
	if (unlikely(buf == NULL)) {
		input_err(true, ts->dev, "%s: Fail to create a buffer for test\n", __func__);
		goto err_power_state;
	}

	sec_input_irq_disable(ts->plat_data);

	command[0] = CMD_DBG_ATTN;
	ret = ts->synaptics_ts_write_data(ts, command, sizeof(command), NULL, 0);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Fail to write cmd\n", __func__);
		goto err;
	}

	msleep(20);
	status1 = gpio_get_value(ts->plat_data->irq_gpio);
	input_info(true, ts->dev, "%s: gpio value %d (should be 0)\n", __func__, status1);

	msleep(20);

	ret = ts->synaptics_ts_read_data_only(ts, buf, SYNAPTICS_TS_MESSAGE_HEADER_SIZE);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Fail to read cmd\n", __func__);
		goto err;
	}

	msleep(10);

	status2 = gpio_get_value(ts->plat_data->irq_gpio);
	input_info(true, ts->dev, "%s: gpio value %d (should be 1)\n", __func__, status2);

	if ((status1 == 0) && (status2 == 1)) {
		snprintf(buff, sizeof(buff), "0");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	} else {
		if (status1 != 0)
			snprintf(buff, sizeof(buff), "1:HIGH");
		else if (status2 != 1)
			snprintf(buff, sizeof(buff), "1:LOW");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}

	sec_input_irq_enable(ts->plat_data);
	synaptics_ts_pal_mem_free(buf);

	synaptics_ts_enable_report(ts, REPORT_SEC_COORDINATE_EVENT, true);
	synaptics_ts_enable_report(ts, REPORT_SEC_STATUS_EVENT, true);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "INT_GPIO");
	return;

err:
	sec_input_irq_enable(ts->plat_data);
	synaptics_ts_pal_mem_free(buf);
err_power_state:

	synaptics_ts_enable_report(ts, REPORT_SEC_COORDINATE_EVENT, true);
	synaptics_ts_enable_report(ts, REPORT_SEC_STATUS_EVENT, true);

	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "INT_GPIO");
}

static void run_sram_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	int ret;
	u8 result = 0;
	unsigned char command[4] = {SYNAPTICS_TS_CMD_PRODUCTION_TEST,
		0x01, 0x00, TEST_PID53_SRAM_TEST};
	unsigned char id;
	int retry = 0, retry_limit = 5;

	synaptics_ts_enable_report(ts, REPORT_SEC_COORDINATE_EVENT, false);
	synaptics_ts_enable_report(ts, REPORT_SEC_STATUS_EVENT, false);

	sec_input_irq_disable(ts->plat_data);

	ret = ts->synaptics_ts_write_data(ts, command, sizeof(command), NULL, 0);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Fail to write cmd\n", __func__);
		sec_input_irq_enable(ts->plat_data);
		synaptics_ts_enable_report(ts, REPORT_SEC_COORDINATE_EVENT, true);
		synaptics_ts_enable_report(ts, REPORT_SEC_STATUS_EVENT, true);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	sec_delay(500);

	do {
		ret = ts->read_message(ts, &id);
		if (ret < 0) {
			retry++;
			sec_delay(10);
			continue;
		}

		if (id == REPORT_IDENTIFY)
			break;

		retry++;
		sec_delay(10);

	} while (retry < retry_limit);

	if (id == REPORT_IDENTIFY) {
		result = 0;		/* PASS */
		sec->cmd_state = SEC_CMD_STATUS_OK;
	} else {
		result = 1;		/* FAIL */
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}

	sec_input_irq_enable(ts->plat_data);
	sec_delay(200);

	synaptics_ts_enable_report(ts, REPORT_SEC_COORDINATE_EVENT, true);
	synaptics_ts_enable_report(ts, REPORT_SEC_STATUS_EVENT, true);

	snprintf(buff, sizeof(buff), "%d", result);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SRAM");

}

/* Automatic Test Equipment for the checking of VDDHSO */
/* Return - 1 : ATE screened, 0 : NOT screen */
static void run_ate_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	int ret = 0;
	u8 result = 0;
	unsigned char resp_code;
	unsigned char test_code = TEST_PID82_CUSTOMER_DATA_CHECK;

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Device is not in application fw mode, mode: %x\n", __func__,
			ts->dev_mode);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	synaptics_ts_enable_report(ts, REPORT_SEC_COORDINATE_EVENT, false);

	ret = ts->write_message(ts,
			SYNAPTICS_TS_CMD_PRODUCTION_TEST,
			&test_code,
			1,
			&resp_code,
			CMD_RESPONSE_POLLING_DELAY_MS);
	if (ret < 0) {
		input_err(true, ts->dev, "Fail to send command 0x%02x\n",
			SYNAPTICS_TS_CMD_PRODUCTION_TEST);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		synaptics_ts_enable_report(ts, REPORT_SEC_COORDINATE_EVENT, true);
		return;
	}

	if (resp_code != STATUS_OK) {
		input_err(true, ts->dev, "Invalid status of PID82\n");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		synaptics_ts_enable_report(ts, REPORT_SEC_COORDINATE_EVENT, true);
		return;
	}

	synaptics_ts_buf_lock(&ts->resp_buf);

	if (ts->resp_buf.data_length > 0) {
		input_info(true, ts->dev, "%s: resp_buf.buf[0] = %x\n", __func__, ts->resp_buf.buf[0]);
		if ((ts->resp_buf.buf[0] & 0x01) == 0x01)
			result = true;	 /* Pass */
	}

	synaptics_ts_buf_unlock(&ts->resp_buf);

	snprintf(buff, sizeof(buff), "%d", result);
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "ATE");
	synaptics_ts_enable_report(ts, REPORT_SEC_COORDINATE_EVENT, true);
}

static int check_support_calibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (ts->tdata->tclm_level == TCLM_LEVEL_NOT_SUPPORT) {
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		input_info(true, ts->dev, "%s: tclm not supported\n", __func__);
		return SEC_ERROR;
	}

	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	return SEC_SUCCESS;
}

static void run_hsync_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	int ret = 0;
	unsigned char resp_code;
	unsigned char test_code = TEST_PID88_HSO_CHECK;
	int min, max;

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Device is not in application fw mode, mode: %x\n", __func__,
			ts->dev_mode);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	ret = ts->write_message(ts,
			SYNAPTICS_TS_CMD_PRODUCTION_TEST,
			&test_code,
			1,
			&resp_code,
			CMD_RESPONSE_POLLING_DELAY_MS);
	if (ret < 0) {
		input_err(true, ts->dev, "Fail to send command 0x%02x\n",
			SYNAPTICS_TS_CMD_PRODUCTION_TEST);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	if (resp_code != STATUS_OK) {
		input_err(true, ts->dev, "Invalid status of PID82\n");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	synaptics_ts_buf_lock(&ts->resp_buf);

	min = (ts->resp_buf.buf[5] & 0xFF) << 8 | (ts->resp_buf.buf[4] & 0xFF);
	max = (ts->resp_buf.buf[7] & 0xFF) << 8 | (ts->resp_buf.buf[6] & 0xFF);

	input_info(true, ts->dev, "%s: min:%d, max:%d, resp_buf : 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
			__func__, min, max, ts->resp_buf.buf[0], ts->resp_buf.buf[1],
			ts->resp_buf.buf[2], ts->resp_buf.buf[3], ts->resp_buf.buf[4],
			ts->resp_buf.buf[5], ts->resp_buf.buf[6], ts->resp_buf.buf[7]);

	synaptics_ts_buf_unlock(&ts->resp_buf);

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "HSYNC");
}

static void run_extended_trx_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	struct synaptics_ts_buffer test_data;
	short *data_ptr = NULL;
	int ret;

	sec_cmd_set_default_result(sec);

	if (ts->plat_data->firmware_name) {
		if (strncmp("tsp_synaptics/s3916t_ps", ts->plat_data->firmware_name, 23) != 0) {
			snprintf(buff, sizeof(buff), "NA");
			sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
			sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
			return;
		}
	}

	synaptics_ts_buf_init(&test_data);

	ret = synaptics_ts_run_production_test(ts,
			TEST_PID48_EXTENDED_TRX_TEST,
			&test_data);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Fail to run test %d\n",
				__func__, TEST_PID48_EXTENDED_TRX_TEST);
		synaptics_ts_buf_release(&test_data);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		return;
	}

	data_ptr = (short *)&test_data.buf[0];
	input_info(true, ts->dev, "%s: %d, %d, %d\n", __func__, data_ptr[0], data_ptr[1], data_ptr[2]);

	snprintf(buff, sizeof(buff), "%d", data_ptr[0]);
	synaptics_ts_buf_release(&test_data);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, ts->dev, "%s: %s\n", __func__, buff);
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "EXT_TRX");
}

static void run_factory_miscalibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	struct synaptics_ts_buffer test_data;
	unsigned short *data_ptr = NULL;
	int ret, i;
	short result[3] = {0};

	if (check_support_calibration(device_data) < 0)
		return;

	synaptics_ts_buf_init(&test_data);

	ret = synaptics_ts_run_production_test(ts,
			TEST_PID54_BSC_DIFF_TEST,
			&test_data);
	if (ret < 0) {
		input_err(true, ts->dev, "Fail to run test %d\n",
		TEST_PID54_BSC_DIFF_TEST);
		synaptics_ts_buf_release(&test_data);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data_ptr = (unsigned short *)&test_data.buf[0];
	for (i = 0; i < test_data.data_length / 2; i++) {
		result[i] = *data_ptr;
		data_ptr++;
	}

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		snprintf(buff, sizeof(buff), "%d,%d", result[1], result[2]);
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MIS_CAL");
	} else {
		if (result[0] == 1)
			snprintf(buff, sizeof(buff), "OK,%d,%d", result[1], result[2]);
		else
			snprintf(buff, sizeof(buff), "NG,%d,%d", result[1], result[2]);
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	synaptics_ts_buf_release(&test_data);
}

static void run_miscalibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	struct synaptics_ts_buffer test_data;
	unsigned short *data_ptr = NULL;
	int ret, i;
	short result[3] = {0};

	if (check_support_calibration(device_data) < 0)
		return;

	synaptics_ts_buf_init(&test_data);

	ret = synaptics_ts_run_production_test(ts,
			TEST_PID61_MISCAL,
			&test_data);
	if (ret < 0) {
		input_err(true, ts->dev, "Fail to run test %d\n",
		TEST_PID61_MISCAL);
		synaptics_ts_buf_release(&test_data);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data_ptr = (unsigned short *)&test_data.buf[0];
	for (i = 0; i < test_data.data_length / 2; i++) {
		result[i] = *data_ptr;
		data_ptr++;
	}

	if (result[0] == 1)
		snprintf(buff, sizeof(buff), "OK,%d,%d", result[1], result[2]);
	else
		snprintf(buff, sizeof(buff), "NG,%d,%d", result[1], result[2]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	synaptics_ts_buf_release(&test_data);
}

static void run_miscal_data_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	struct synaptics_ts_test_mode mode;

	if (check_support_calibration(device_data) < 0)
		return;

	memset(&mode, 0x00, sizeof(struct synaptics_ts_test_mode));
	mode.type = TEST_PID65_MISCALDATA_NORMAL;
	mode.allnode = TEST_MODE_ALL_NODE;

	synaptics_ts_test_rawdata_read(ts, sec, &mode);
}

void synaptics_ts_rawdata_read_all(struct synaptics_ts_data *ts)
{
	struct synaptics_ts_buffer test_data;
	int min, max;
	int i, test_num;
	int ret;
	u8 test_type[7] = {TEST_PID10_DELTA_NOISE,
			TEST_PID18_HYBRID_ABS_RAW, TEST_PID22_TRANS_RAW_CAP, TEST_PID29_HYBRID_ABS_NOISE,
			TEST_PID65_MISCALDATA_NORMAL, TEST_PID66_MISCALDATA_NOISE, TEST_PID67_MISCALDATA_WET};

	if (ts->tdata->tclm_level == TCLM_LEVEL_NOT_SUPPORT)
		test_num = 4;
	else if (ts->support_miscal_wet)
		test_num = 7;
	else
		test_num = 6;

	ts->tsp_dump_lock = 1;
	input_raw_data_clear_by_device(GET_DEV_COUNT(ts->multi_dev));

	for (i = 0; i < test_num; i++) {
		input_raw_info_d(GET_DEV_COUNT(ts->multi_dev), ts->dev, "%s: %d\n", __func__, test_type[i]);
		synaptics_ts_buf_init(&test_data);
		ret = synaptics_ts_run_production_test(ts, test_type[i], &test_data);
		if (ret < 0) {
			input_err(true, ts->dev, "Fail to run test %d\n", test_type[i]);
			synaptics_ts_buf_release(&test_data);
			goto out;
		}
		if (test_type[i] == TEST_PID18_HYBRID_ABS_RAW || test_type[i] == TEST_PID29_HYBRID_ABS_NOISE)
			synaptics_ts_print_channel(ts, &test_data, &min, &max, test_type[i]);
		else
			synaptics_ts_print_frame(ts, &test_data, &min, &max, test_type[i]);
		synaptics_ts_buf_release(&test_data);
	}

	synaptics_ts_clear_buffer(ts);
	synaptics_ts_locked_release_all_finger(ts);

out:
	ts->tsp_dump_lock = 0;
}

static void factory_cmd_result_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	sec->item_count = 0;
	memset(sec->cmd_result_all, 0x00, SEC_CMD_RESULT_STR_LEN);

	if (ts->tsp_dump_lock == 1) {
		input_err(true, ts->dev, "%s: already checking now\n", __func__);
		sec->cmd_all_factory_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}
	if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
		input_err(true, ts->dev, "%s: IC is power off\n", __func__);
		sec->cmd_all_factory_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	sec->cmd_all_factory_state = SEC_CMD_STATUS_RUNNING;

	get_chip_vendor(sec);
	get_chip_name(sec);
	get_fw_ver_bin(sec);
	get_fw_ver_ic(sec);

	run_trans_rawcap_read(sec);
	get_gap_data(sec);

	run_full_rawcap_read(sec);
	run_delta_noise_read(sec);
	run_abs_rawcap_read(sec);
	get_self_channel_data(sec, TEST_PID18_HYBRID_ABS_RAW);
	run_abs_noise_read(sec);

	if (ts->tdata->tclm_level != TCLM_LEVEL_NOT_SUPPORT)
		run_factory_miscalibration(sec);

	run_sram_test(sec);
	run_interrupt_gpio_test(sec);
	run_ate_test(sec);
	run_hsync_test(sec);
	run_extended_trx_test(sec);

	sec->cmd_all_factory_state = SEC_CMD_STATUS_OK;

out:
	input_info(true, ts->dev, "%s: %d%s\n", __func__, sec->item_count, sec->cmd_result_all);
}

static void run_force_calibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int rc;

	if (check_support_calibration(device_data) < 0)
		goto out_force_cal;

	if (ts->plat_data->touch_count > 0) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out_force_cal;
	}

	rc = synaptics_ts_calibration(ts);
	if (rc < 0) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out_force_cal;
	}

#ifdef TCLM_CONCEPT
	/* devide tclm case */
	sec_tclm_case(ts->tdata, sec->cmd_param[0]);

	input_info(true, ts->dev, "%s: param, %d, %c, %d\n", __func__,
		sec->cmd_param[0], sec->cmd_param[0], ts->tdata->root_of_calibration);

	rc = sec_execute_tclm_package(ts->tdata, 1);
	if (rc < 0) {
		input_err(true, ts->dev,
					"%s: sec_execute_tclm_package\n", __func__);
	}
	sec_tclm_root_of_cal(ts->tdata, CALPOSITION_NONE);
#endif

	sec->cmd_state = SEC_CMD_STATUS_OK;

out_force_cal:
#ifdef TCLM_CONCEPT
	/* not to enter external factory mode without setting everytime */
	ts->tdata->external_factory = false;
#endif
	input_info(true, ts->dev, "%s: %s\n", __func__, sec->cmd_state == SEC_CMD_STATUS_OK ? "OK" : "NG");
}

static int run_force_calibration_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

#ifdef TCLM_CONCEPT
	ts->tdata->external_factory = false;
#endif

	return check_support_calibration(device_data);
}

#ifdef TCLM_CONCEPT
static void set_tsp_test_result(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	struct sec_ts_test_result *result;

	synaptics_ts_tclm_read(ts->dev, SEC_TCLM_NVM_ALL_DATA);
	if (ts->fac_nv == 0xFF)
		ts->fac_nv = 0;

	result = (struct sec_ts_test_result *)&ts->fac_nv;

	if (sec->cmd_param[0] == TEST_OCTA_ASSAY) {
		result->assy_result = sec->cmd_param[1];
		if (result->assy_count < 3)
			result->assy_count++;
	}

	if (sec->cmd_param[0] == TEST_OCTA_MODULE) {
		result->module_result = sec->cmd_param[1];
		if (result->module_count < 3)
			result->module_count++;
	}

	input_info(true, ts->dev, "%s: %d, %d, %d, %d, 0x%X\n", __func__,
			result->module_result, result->module_count,
			result->assy_result, result->assy_count, result->data[0]);

	ts->fac_nv = *result->data;

	synaptics_ts_tclm_write(ts->dev, SEC_TCLM_NVM_ALL_DATA);
	synaptics_ts_tclm_read(ts->dev, SEC_TCLM_NVM_ALL_DATA);

	input_info(true, ts->dev, "%s: command (1)%X, (2)%X: %X\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1], ts->fac_nv);

	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void get_tsp_test_result(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	struct sec_ts_test_result *result;

	synaptics_ts_tclm_read(ts->dev, SEC_TCLM_NVM_ALL_DATA);
	if (ts->fac_nv == 0xFF) {
		ts->fac_nv = 0;
		synaptics_ts_tclm_write(ts->dev, SEC_TCLM_NVM_ALL_DATA);
	}

	result = (struct sec_ts_test_result *)&ts->fac_nv;

	input_info(true, ts->dev, "%s: [0x%X][0x%X] M:%d, M:%d, A:%d, A:%d\n",
			__func__, *result->data, ts->fac_nv,
			result->module_result, result->module_count,
			result->assy_result, result->assy_count);

	snprintf(buff, sizeof(buff), "M:%s, M:%d, A:%s, A:%d",
			result->module_result == 0 ? "NONE" :
			result->module_result == 1 ? "FAIL" :
			result->module_result == 2 ? "PASS" : "A",
			result->module_count,
			result->assy_result == 0 ? "NONE" :
			result->assy_result == 1 ? "FAIL" :
			result->assy_result == 2 ? "PASS" : "A",
			result->assy_count);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void clear_tsp_test_result(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	ts->fac_nv = 0;
	synaptics_ts_tclm_write(ts->dev, SEC_TCLM_NVM_ALL_DATA);
	synaptics_ts_tclm_read(ts->dev, SEC_TCLM_NVM_ALL_DATA);

	input_info(true, ts->dev, "%s: fac_nv:0x%02X\n", __func__, ts->fac_nv);

	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void increase_disassemble_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	synaptics_ts_tclm_read(ts->dev, SEC_TCLM_NVM_ALL_DATA);

	input_info(true, ts->dev, "%s: disassemble count is #1 %d\n", __func__, ts->disassemble_count);

	if (ts->disassemble_count == 0xFF)
		ts->disassemble_count = 0;

	if (ts->disassemble_count < 0xFE)
		ts->disassemble_count++;

	/* Use TSP NV area : in this model, use only one byte
	 * buff[0] : offset from user NVM storage
	 * buff[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
	 * buff[2] : write data
	 */
	synaptics_ts_tclm_write(ts->dev, SEC_TCLM_NVM_ALL_DATA);
	synaptics_ts_tclm_read(ts->dev, SEC_TCLM_NVM_ALL_DATA);

	input_info(true, ts->dev, "%s: check disassemble count: %d\n", __func__, ts->disassemble_count);

	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void get_disassemble_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	synaptics_ts_tclm_read(ts->dev, SEC_TCLM_NVM_ALL_DATA);

	input_info(true, ts->dev, "%s: read disassemble count: %d\n", __func__, ts->disassemble_count);

	snprintf(buff, sizeof(buff), "%d", ts->disassemble_count);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void get_pat_information(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[50] = { 0 };

	if (check_support_calibration(device_data) < 0)
		return;

#ifdef CONFIG_SEC_FACTORY
	if (ts->factory_position == 1) {
		sec_tclm_initialize(ts->tdata);
		if (sec_input_cmp_ic_status(ts->dev, CHECK_ON_LP))
			ts->tdata->tclm_read(ts->dev, SEC_TCLM_NVM_ALL_DATA);
	}
#endif
	/* fixed tune version will be saved at excute autotune */
	snprintf(buff, sizeof(buff), "C%02XT%04X.%4s%s%c%d%c%d%c%d",
		ts->tdata->nvdata.cal_count, ts->tdata->nvdata.tune_fix_ver,
		ts->tdata->tclm_string[ts->tdata->nvdata.cal_position].f_name,
		(ts->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L " : " ",
		ts->tdata->cal_pos_hist_last3[0], ts->tdata->cal_pos_hist_last3[1],
		ts->tdata->cal_pos_hist_last3[2], ts->tdata->cal_pos_hist_last3[3],
		ts->tdata->cal_pos_hist_last3[4], ts->tdata->cal_pos_hist_last3[5]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s: %s\n", __func__, buff);
}

static void set_external_factory(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	ts->tdata->external_factory = true;

	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s\n", __func__);
}
#endif

static void set_factory_level(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < OFFSET_FAC_SUB || sec->cmd_param[0] > OFFSET_FAC_MAIN) {
		input_err(true, ts->dev,
				"%s: cmd data is abnormal, %d\n", __func__, sec->cmd_param[0]);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	ts->factory_position = sec->cmd_param[0];

	input_info(true, ts->dev, "%s: %d\n", __func__, ts->factory_position);
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void run_trx_short_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	struct synaptics_ts_buffer test_data;
	char buff[SEC_CMD_STR_LEN];
	u8 type = 0;
	int ret;
	char result_uevent[32];
	char test[32];
	char tempn[40] = {0};
	int i, j;
	unsigned char result_data = 0;
	u8 *test_result_buff;
	unsigned char tmp = 0;
	unsigned char p = 0;

	if (sec->cmd_param[0] == 1 && sec->cmd_param[1] == 0) {
		input_err(true, ts->dev,
				"%s: %s: seperate cm1 test open / short test result\n", __func__, buff);

		snprintf(buff, sizeof(buff), "%s", "CONT");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_OK;
		return;
	}

	if (sec->cmd_param[0] == 1 && sec->cmd_param[1] == 1) {
		type = TEST_PID58_TRX_OPEN;
	} else if (sec->cmd_param[0] == 1 && sec->cmd_param[1] == 2) {
		type = TEST_PID01_TRX_SHORTS;
	} else {
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		input_info(true, ts->dev, "%s : not supported test case\n", __func__);
		return;
	}

	test_result_buff = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!test_result_buff) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	if (sec->cmd_param[1])
		snprintf(test, sizeof(test), "TEST=%d,%d", sec->cmd_param[0], sec->cmd_param[1]);
	else
		snprintf(test, sizeof(test), "TEST=%d", sec->cmd_param[0]);

	synaptics_ts_buf_init(&test_data);

	ret = synaptics_ts_run_production_test(ts, type, &test_data);
	if (ret < 0) {
		input_err(true, ts->dev, "Fail to run test %d\n", type);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;

		snprintf(result_uevent, sizeof(result_uevent), "RESULT=FAIL");
		sec_cmd_send_event_to_user(&ts->sec, test, result_uevent);

		synaptics_ts_buf_release(&test_data);
		kfree(test_result_buff);
		return;
	}

	memset(tempn, 0x00, 40);
	/* print log */
	if (type == TEST_PID58_TRX_OPEN)
		snprintf(tempn, 40, " TX/RX_OPEN:");
	else if (type == TEST_PID01_TRX_SHORTS)
		snprintf(tempn, 40, " TX/RX_SHORT:");

	strlcat(test_result_buff, tempn, PAGE_SIZE);

	if (type == TEST_PID01_TRX_SHORTS) {
		for (i = 0; i < test_data.data_length; i++) {
			memset(tempn, 0x00, 40);
			tmp = test_data.buf[i];
			result_data += tmp;
			for (j = 0; j < 8; j++) {
				p = GET_BIT_LSB(tmp, j);
				if (p == 0x1) {
					snprintf(tempn, 20, "TRX%d,", i * 8 + j);
					strlcat(test_result_buff, tempn, PAGE_SIZE);
				}
			}
		}
	} else {
		for (i = 0; i < test_data.data_length; i++) {
			memset(tempn, 0x00, 40);
			tmp = test_data.buf[i];
			result_data += tmp;
			for (j = 0; j < 8; j++) {
				p = GET_BIT_MSB(tmp, j);
				if (p == 0x1) {
					if (i * 8 + j < ts->cols)
						snprintf(tempn, 20, "RX%d,", i * 8 + j);
					else
						snprintf(tempn, 20, "TX%d,", (i * 8 + j) - ts->cols);
					strlcat(test_result_buff, tempn, PAGE_SIZE);
				}
			}
		}
	}
	synaptics_ts_buf_release(&test_data);

	if (result_data == 0)
		ret = SEC_SUCCESS;
	else
		ret = SEC_ERROR;

	if (ret < 0) {
		sec_cmd_set_cmd_result(sec, test_result_buff, strnlen(buff, PAGE_SIZE));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_info(true, ts->dev, "%s: %s\n", __func__, test_result_buff);
		snprintf(result_uevent, sizeof(result_uevent), "RESULT=FAIL");
	} else {
		sec->cmd_state = SEC_CMD_STATUS_OK;
		input_info(true, ts->dev, "%s: OK\n", __func__);
		snprintf(result_uevent, sizeof(result_uevent), "RESULT=PASS");
	}
	sec_cmd_send_event_to_user(&ts->sec, test, result_uevent);

	kfree(test_result_buff);
}

static void run_jitter_delta_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	int result = -1;
	const int IDX_MIN = 1;
	const int IDX_MAX = 0;
	short min[2] = { 0 };
	short max[2] = { 0 };
	short avg[2] = { 0 };
	unsigned char buf[32] = { 0 };
	unsigned int time = 0;
	unsigned int timeout = 30;
	unsigned char command[4] = {SYNAPTICS_TS_CMD_PRODUCTION_TEST,
		0x01, 0x00, TEST_PID52_JITTER_DELTA_TEST};

	synaptics_ts_release_all_finger(ts);

	sec_input_irq_disable(ts->plat_data);

	ret = ts->synaptics_ts_write_data(ts, command, sizeof(command), NULL, 0);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: failed to set active mode\n", __func__);
		goto OUT_JITTER_DELTA;
	}

	/* jitter delta + 1000 frame*/
	for (time = 0; time < timeout; time++) {
		sec_delay(500);

		ret = ts->synaptics_ts_read_data_only(ts,
			buf,
			sizeof(buf));
		if (ret < 0) {
			input_info(true, ts->dev, "Fail to read the test result\n");
			goto OUT_JITTER_DELTA;
		}

		if (buf[1] == STATUS_OK) {
			result = 0;
			break;
		}
	}

	if (time >= timeout) {
		input_info(true, ts->dev, "Timed out waiting for result of PT34\n");
		goto OUT_JITTER_DELTA;
	}

	min[IDX_MIN] = (short)synaptics_ts_pal_le2_to_uint(&buf[SYNAPTICS_TS_MESSAGE_HEADER_SIZE + 0]);
	min[IDX_MAX] = (short)synaptics_ts_pal_le2_to_uint(&buf[SYNAPTICS_TS_MESSAGE_HEADER_SIZE + 2]);

	max[IDX_MIN] = (short)synaptics_ts_pal_le2_to_uint(&buf[SYNAPTICS_TS_MESSAGE_HEADER_SIZE + 4]);
	max[IDX_MAX] = (short)synaptics_ts_pal_le2_to_uint(&buf[SYNAPTICS_TS_MESSAGE_HEADER_SIZE + 6]);

	avg[IDX_MIN] = (short)synaptics_ts_pal_le2_to_uint(&buf[SYNAPTICS_TS_MESSAGE_HEADER_SIZE + 8]);
	avg[IDX_MAX] = (short)synaptics_ts_pal_le2_to_uint(&buf[SYNAPTICS_TS_MESSAGE_HEADER_SIZE + 10]);

OUT_JITTER_DELTA:

	sec_input_irq_enable(ts->plat_data);

	if (result < 0)
		snprintf(buff, sizeof(buff), "NG");
	else
		snprintf(buff, sizeof(buff), "%d,%d,%d,%d,%d,%d", min[IDX_MIN], min[IDX_MAX],
						max[IDX_MIN], max[IDX_MAX], avg[IDX_MIN], avg[IDX_MAX]);

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		char buffer[SEC_CMD_STR_LEN] = { 0 };

		snprintf(buffer, sizeof(buffer), "%d,%d", min[IDX_MIN], min[IDX_MAX]);
		sec_cmd_set_cmd_result_all(sec, buffer, strnlen(buffer, sizeof(buffer)), "JITTER_DELTA_MIN");

		memset(buffer, 0x00, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%d,%d", max[IDX_MIN], max[IDX_MAX]);
		sec_cmd_set_cmd_result_all(sec, buffer, strnlen(buffer, sizeof(buffer)), "JITTER_DELTA_MAX");

		memset(buffer, 0x00, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%d,%d", avg[IDX_MIN], avg[IDX_MAX]);
		sec_cmd_set_cmd_result_all(sec, buffer, strnlen(buffer, sizeof(buffer)), "JITTER_DELTA_AVG");
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, ts->dev, "%s: %s\n", __func__, buff);
}

static void run_snr_non_touched(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	unsigned char buf[2 * 7 * (3 * 3) + 4] = { 0 };
	int ret = 0;
	int wait_time = 0;
	unsigned int time = 0;
	unsigned int timeout = 30;
	unsigned int input = 0;
	unsigned char command[4] = {SYNAPTICS_TS_CMD_PRODUCTION_TEST,
			0x01, 0x00, TEST_PID56_TSP_SNR_NONTOUCH};

	input_info(true, ts->dev, "%s\n", __func__);

	if (sec->cmd_param[0] < 1 || sec->cmd_param[0] > 1000) {
		input_err(true, ts->dev, "%s: strange value frame:%d\n",
				__func__, sec->cmd_param[0]);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input = sec->cmd_param[0];

	/* set up snr frame */
	ret = synaptics_ts_set_immediate_dynamic_config(ts,
			DC_TSP_SNR_TEST_FRAMES,
			(unsigned short)input);
	if (ret < 0) {
		input_err(true, ts->dev, "Fail to set %d with dynamic command 0x%x\n",
			input, DC_TSP_SNR_TEST_FRAMES);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	sec_input_irq_disable(ts->plat_data);
	/* enter SNR mode */
	ret = ts->synaptics_ts_write_data(ts, command, sizeof(command), NULL, 0);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: failed to set active mode\n", __func__);
		goto out;
	}

	wait_time = (sec->cmd_param[0] * 1000) / 120 + (sec->cmd_param[0] * 1000) % 120;

	sec_delay(wait_time);

	for (time = 0; time < timeout; time++) {
		sec_delay(20);

		ret = ts->synaptics_ts_read_data_only(ts,
			buf,
			sizeof(buf));
		if (ret < 0) {
			input_info(true, ts->dev, "Fail to read the test result\n");
			goto out;
		}

		if (buf[1] == STATUS_OK)
			break;
	}

	if (buf[1] == 0) {
		input_err(true, ts->dev, "%s: failed non-touched status:%d\n", __func__, buf[1]);
		goto out;
	}

	sec_input_irq_enable(ts->plat_data);
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s: OK\n", __func__);
	return;
out:
	sec_input_irq_enable(ts->plat_data);
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, ts->dev, "%s: NG\n", __func__);
}

static void run_snr_touched(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char tbuff[SEC_CMD_STR_LEN] = { 0 };
	unsigned char buf[2 * 7 * (3 * 3) + 4] = { 0 };
	int ret = 0;
	int wait_time = 0;
	int i = 0;
	unsigned int time = 0;
	unsigned int timeout = 30;
	unsigned int input = 0;
	unsigned char command[4] = {SYNAPTICS_TS_CMD_PRODUCTION_TEST,
			0x01, 0x00, TEST_PID57_TSP_SNR_TOUCH};
	struct tsp_snr_result_of_point result[9];

	memset(result, 0x00, sizeof(struct tsp_snr_result_of_point) * 9);

	input_info(true, ts->dev, "%s\n", __func__);

	if (sec->cmd_param[0] < 1 || sec->cmd_param[0] > 1000) {
		input_err(true, ts->dev, "%s: strange value frame:%d\n",
				__func__, sec->cmd_param[0]);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input = sec->cmd_param[0];

	/* set up snr frame */
	ret = synaptics_ts_set_immediate_dynamic_config(ts,
			DC_TSP_SNR_TEST_FRAMES,
			(unsigned short)input);
	if (ret < 0) {
		input_err(true, ts->dev, "Fail to set %d with dynamic command 0x%x\n",
			input, DC_TSP_SNR_TEST_FRAMES);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	sec_input_irq_disable(ts->plat_data);
	/* enter SNR mode */
	ret = ts->synaptics_ts_write_data(ts, command, sizeof(command), NULL, 0);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: failed to set active mode\n", __func__);
		goto out;
	}

	wait_time = (sec->cmd_param[0] * 1000) / 120 + (sec->cmd_param[0] * 1000) % 120;

	sec_delay(wait_time);

	for (time = 0; time < timeout; time++) {
		sec_delay(20);

		ret = ts->synaptics_ts_read_data_only(ts,
			buf,
			sizeof(buf));
		if (ret < 0) {
			input_info(true, ts->dev, "Fail to read the test result\n");
			goto out;
		}

		if (buf[1] == STATUS_OK)
			break;
	}

	if (buf[1] == 0) {
		input_err(true, ts->dev, "%s: failed non-touched status:%d\n", __func__, buf[1]);
		goto out;
	}

	sec_input_irq_enable(ts->plat_data);

	//memcpy(result, &buf[SYNAPTICS_TS_MESSAGE_HEADER_SIZE], sizeof(struct tsp_snr_result_of_point) * 9);

	for (i = 0; i < 9; i++) {
		result[i].max = (short)synaptics_ts_pal_le2_to_uint(&buf[i * 14 + SYNAPTICS_TS_MESSAGE_HEADER_SIZE]);
		result[i].min = (short)synaptics_ts_pal_le2_to_uint(&buf[i * 14 + SYNAPTICS_TS_MESSAGE_HEADER_SIZE + 2]);
		result[i].average = (short)synaptics_ts_pal_le2_to_uint(&buf[i * 14 + SYNAPTICS_TS_MESSAGE_HEADER_SIZE + 4]);
		result[i].nontouch_peak_noise = (short)synaptics_ts_pal_le2_to_uint(&buf[i * 14 + SYNAPTICS_TS_MESSAGE_HEADER_SIZE + 6]);
		result[i].touch_peak_noise = (short)synaptics_ts_pal_le2_to_uint(&buf[i * 14 + SYNAPTICS_TS_MESSAGE_HEADER_SIZE + 8]);
		result[i].snr1 = (short)synaptics_ts_pal_le2_to_uint(&buf[i * 14 + SYNAPTICS_TS_MESSAGE_HEADER_SIZE + 10]);
		result[i].snr2 = (short)synaptics_ts_pal_le2_to_uint(&buf[i * 14 + SYNAPTICS_TS_MESSAGE_HEADER_SIZE + 12]);

		input_info(true, ts->dev, "%s: average:%d, snr1:%d, snr2:%d\n", __func__,
			result[i].average, result[i].snr1, result[i].snr2);
		snprintf(tbuff, sizeof(tbuff), "%d,%d,%d,", result[i].average, result[i].snr1, result[i].snr2);
		strlcat(buff, tbuff, sizeof(buff));
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, ts->dev, "%s: %s\n", __func__, buff);

	return;

out:
	sec_input_irq_enable(ts->plat_data);
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, ts->dev, "%s: NG\n", __func__);
}

static void factory_cmd_result_all_imagetest(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	sec->item_count = 0;
	memset(sec->cmd_result_all, 0x00, SEC_CMD_RESULT_STR_LEN);

	if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
		input_err(true, ts->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		sec->cmd_all_factory_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	sec->cmd_all_factory_state = SEC_CMD_STATUS_RUNNING;

	run_jitter_delta_test(sec);

	sec->cmd_all_factory_state = SEC_CMD_STATUS_OK;

out:
	input_info(true, ts->dev, "%s: %d%s\n", __func__, sec->item_count, sec->cmd_result_all);
}

static int glove_mode_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	ts->glove_mode = sec->cmd_param[0];
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s: %d\n", __func__, sec->cmd_param[0]);

	return SEC_SUCCESS;
}

static void glove_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = glove_mode_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_HIGHSENSMODE, ts->glove_mode);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: failed, retval = %d\n", __func__, ret);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}

	input_info(true, ts->dev, "%s: %s\n", __func__, sec->cmd_state == SEC_CMD_STATUS_OK ? "OK" : "NG");
}

static int dead_zone_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	ts->dead_zone = sec->cmd_param[0];
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s: %d\n", __func__, sec->cmd_param[0]);

	return SEC_SUCCESS;
}

static void dead_zone_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = dead_zone_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_DEADZONE, ts->dead_zone);
	if (ret < 0) {
		input_err(true, ts->dev,
				"%s: failed to set deadzone\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}

	input_info(true, ts->dev, "%s: %s\n", __func__, sec->cmd_state == SEC_CMD_STATUS_OK ? "OK" : "NG");
};

static int clear_cover_mode_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 3) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	if (sec->cmd_param[0] > 1) {
		ts->plat_data->cover_type = sec->cmd_param[1];
		ts->cover_closed = true;
	} else {
		ts->cover_closed = false;
	}
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s: OK %d,%d\n", __func__, sec->cmd_param[0], sec->cmd_param[1]);

	return SEC_SUCCESS;
}

static void clear_cover_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = clear_cover_mode_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	synaptics_ts_set_cover_type(ts, ts->cover_closed);
	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static void run_rawdata_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (ts->tsp_dump_lock == 1) {
		input_err(true, ts->dev, "%s: already checking now\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}
	if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
		input_err(true, ts->dev, "%s: IC is power off\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	synaptics_ts_rawdata_read_all(ts);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static int spay_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	if (sec->cmd_param[0])
		ts->plat_data->lowpower_mode |= SEC_TS_MODE_SPONGE_SWIPE;
	else
		ts->plat_data->lowpower_mode &= ~SEC_TS_MODE_SPONGE_SWIPE;
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, ts->dev, "%s: %s, %02X\n",
			__func__, sec->cmd_param[0] ? "on" : "off", ts->plat_data->lowpower_mode);
	return SEC_SUCCESS;
}

static void spay_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = spay_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	synaptics_ts_set_custom_library(ts);
	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static int aod_rect_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int i;

	input_info(true, ts->dev, "%s: w:%d, h:%d, x:%d, y:%d, lowpower_mode:0x%02X\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1],
			sec->cmd_param[2], sec->cmd_param[3], ts->plat_data->lowpower_mode);

	for (i = 0; i < 4; i++)
		ts->plat_data->aod_data.rect_data[i] = sec->cmd_param[i];

	sec->cmd_state = SEC_CMD_STATUS_OK;
	return SEC_SUCCESS;
}

static void set_aod_rect(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = aod_rect_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	ret = synaptics_ts_set_aod_rect(ts);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static int aod_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	if (sec->cmd_param[0]) {
		ts->plat_data->lowpower_mode |= SEC_TS_MODE_SPONGE_AOD;
		ts->plat_data->sponge_mode |= SEC_TS_MODE_SPONGE_AOD_NOTI_RECT_AOT;
	} else {
		ts->plat_data->lowpower_mode &= ~SEC_TS_MODE_SPONGE_AOD;
		ts->plat_data->sponge_mode &= ~SEC_TS_MODE_SPONGE_AOD_NOTI_RECT_AOT;
	}

	input_info(true, ts->dev, "%s: %s, %02X\n",
			__func__, sec->cmd_param[0] ? "on" : "off", ts->plat_data->lowpower_mode);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return SEC_SUCCESS;
}

static void aod_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = aod_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	synaptics_ts_set_custom_library(ts);
	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static int aod_noti_rect_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int i;

	input_info(true, ts->dev, "%s: w:%d, h:%d, x:%d, y:%d, lowpower_mode:0x%02X\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1],
			sec->cmd_param[2], sec->cmd_param[3], ts->plat_data->lowpower_mode);

	for (i = 0; i < 4; i++)
		ts->plat_data->aod_noti_data.rect_data[i] = sec->cmd_param[i];

	sec->cmd_state = SEC_CMD_STATUS_OK;
	return SEC_SUCCESS;
}

static void set_aod_noti_rect(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = aod_noti_rect_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	ret = synaptics_ts_set_aod_noti_rect(ts);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static int aot_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	if (sec->cmd_param[0])
		ts->plat_data->lowpower_mode |= SEC_TS_MODE_SPONGE_DOUBLETAP_TO_WAKEUP;
	else
		ts->plat_data->lowpower_mode &= ~SEC_TS_MODE_SPONGE_DOUBLETAP_TO_WAKEUP;

	input_info(true, ts->dev, "%s: %s, %02X\n",
			__func__, sec->cmd_param[0] ? "on" : "off", ts->plat_data->lowpower_mode);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return SEC_SUCCESS;
}

static void aot_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = aot_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	synaptics_ts_set_custom_library(ts);
	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static int singletap_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	if (sec->cmd_param[0])
		ts->plat_data->lowpower_mode |= SEC_TS_MODE_SPONGE_SINGLE_TAP;
	else
		ts->plat_data->lowpower_mode &= ~SEC_TS_MODE_SPONGE_SINGLE_TAP;

	input_info(true, ts->dev, "%s: %s, %02X\n",
			__func__, sec->cmd_param[0] ? "on" : "off", ts->plat_data->lowpower_mode);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return SEC_SUCCESS;
}

static void singletap_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = singletap_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	synaptics_ts_set_custom_library(ts);
	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static int ear_detect_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (!ts->plat_data->support_ear_detect) {
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return SEC_ERROR;
	}

	if (sec->cmd_param[0] != 0 && sec->cmd_param[0] != 1 && sec->cmd_param[0] != 3) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	ts->plat_data->ed_enable = sec->cmd_param[0];
	input_info(true, ts->dev, "%s: %s\n",
			__func__, sec->cmd_param[0] ? "on" : "off");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return SEC_SUCCESS;
}

static void ear_detect_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = ear_detect_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	synaptics_ts_ear_detect_enable(ts, ts->plat_data->ed_enable);
	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static int pocket_mode_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (!ts->plat_data->support_ear_detect) {
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return SEC_ERROR;
	}

	if (sec->cmd_param[0] != 0 && sec->cmd_param[0] != 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	ts->plat_data->pocket_mode = sec->cmd_param[0];
	input_info(true, ts->dev, "%s: %s\n",
			__func__, ts->plat_data->pocket_mode ? "on" : "off");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return SEC_SUCCESS;
}

static void pocket_mode_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = pocket_mode_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	synaptics_ts_pocket_mode_enable(ts, ts->plat_data->pocket_mode);
	input_info(true, ts->dev, "%s: OK\n", __func__);
}

/*
 * low_sensitivity_mode_store
 * - input param can be 3
 * param 0 : enable / disable
 *		0 - disable
 *		1 - enable
 * param 1 : sensitivity level
 *		0 - case 1
 *		1 - case 2
 * param 2 : debug
 *		0 - don't care
 *		1 - start
 *		2 - end
 */
static int low_sensitivity_mode_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	if (sec->cmd_param[1] < 0 || sec->cmd_param[1] > 7) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	if (sec->cmd_param[2] < 0 || sec->cmd_param[2] > 2) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	ts->plat_data->low_sensitivity_mode = sec->cmd_param[0] & 0x01;
	ts->plat_data->low_sensitivity_mode |= ((sec->cmd_param[1] & 0x07) << 1);
	ts->plat_data->low_sensitivity_mode |= ((sec->cmd_param[2] & 0x03) << 6);
	input_info(true, ts->dev, "%s: 0x%02X, enable:%d, level:%d, debug:%d\n",
			__func__, ts->plat_data->low_sensitivity_mode,
			sec->cmd_param[0], sec->cmd_param[1],
			sec->cmd_param[2]);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return SEC_SUCCESS;
}

static void low_sensitivity_mode_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = low_sensitivity_mode_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	mutex_lock(&ts->plat_data->enable_mutex);
	synaptics_ts_low_sensitivity_mode_enable(ts, ts->plat_data->low_sensitivity_mode);
	mutex_unlock(&ts->plat_data->enable_mutex);

	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static int fod_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (!ts->plat_data->support_fod) {
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return SEC_ERROR;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	if (sec->cmd_param[0])
		ts->plat_data->lowpower_mode |= SEC_TS_MODE_SPONGE_PRESS;
	else
		ts->plat_data->lowpower_mode &= ~SEC_TS_MODE_SPONGE_PRESS;

	ts->plat_data->fod_data.press_prop = (sec->cmd_param[1] & 0x01) | ((sec->cmd_param[2] & 0x01) << 1);
	if (ts->plat_data->support_fod_control)
		ts->plat_data->fod_data.press_prop |= ((sec->cmd_param[3] & 0x3F) << 2);

	input_info(true, ts->dev, "%s: %s, fast:%s, strict:%s, control:0x%02X, lp:0x%02X\n",
			__func__, sec->cmd_param[0] ? "on" : "off",
			ts->plat_data->fod_data.press_prop & 1 ? "on" : "off",
			ts->plat_data->fod_data.press_prop & 2 ? "on" : "off",
			(ts->plat_data->fod_data.press_prop >> 2),
			ts->plat_data->lowpower_mode);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return SEC_SUCCESS;
}

static void fod_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = fod_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	mutex_lock(&ts->plat_data->enable_mutex);
	if (!atomic_read(&ts->plat_data->enabled) && sec_input_need_ic_off(ts->plat_data)) {
		if (sec_input_cmp_ic_status(ts->dev, CHECK_LPMODE))
			disable_irq_wake(ts->irq);
		ts->plat_data->stop_device(ts);
	} else {
		synaptics_ts_set_custom_library(ts);
		synaptics_ts_set_press_property(ts);
	}
	mutex_unlock(&ts->plat_data->enable_mutex);

	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static void fod_lp_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (!!sec->cmd_param[0])
		ts->plat_data->fod_lp_mode = 1;
	else
		ts->plat_data->fod_lp_mode = 0;

	input_info(true, ts->dev, "%s: %s\n",
				__func__, sec->cmd_param[0] ? "on" : "off");

	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static int fod_rect_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (!ts->plat_data->support_fod) {
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return SEC_ERROR;
	}

	input_info(true, ts->dev, "%s: l:%d, t:%d, r:%d, b:%d\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1],
			sec->cmd_param[2], sec->cmd_param[3]);

	if (!sec_input_set_fod_rect(ts->dev, sec->cmd_param)) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;
	return SEC_SUCCESS;
}

static void set_fod_rect(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret = 0;

	ret = fod_rect_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	ret = synaptics_ts_set_fod_rect(ts);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static void fp_int_control(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret = 0;
	unsigned int input;

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	} else if (!ts->plat_data->support_fod) {
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if (sec->cmd_param[0])
		input = 0x01;
	else
		input = 0x03;

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_FOD_INT, (unsigned short)input);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: failed to set (%d)\n", __func__, sec->cmd_param[0]);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_info(true, ts->dev, "%s: fod int %d\n", __func__, sec->cmd_param[0]);

	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static int game_mode_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	ts->game_mode = sec->cmd_param[0];
	input_info(true, ts->dev, "%s: %d\n", __func__, sec->cmd_param[0]);
	sec->cmd_state = SEC_CMD_STATUS_OK;
	return SEC_SUCCESS;
}

static void set_game_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = game_mode_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_GAMEMODE, ts->game_mode);
	if (ret < 0) {
		input_err(true, ts->dev,
				"%s: failed to set game_mode\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	if (ts->game_mode == 0) {
		if (ts->raw_mode && ts->plat_data->support_rawdata)
			synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_EXTENDED_TOUCH_REPORT_CONTROL, 0x01);
	}

	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static int fast_response_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	ts->fast_response = sec->cmd_param[0];
	input_info(true, ts->dev, "%s: %d\n", __func__, sec->cmd_param[0]);
	sec->cmd_state = SEC_CMD_STATUS_OK;
	return SEC_SUCCESS;
}

static void set_fast_response(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = fast_response_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_FAST_RESPONSE, ts->fast_response);
	if (ret < 0) {
		input_err(true, ts->dev,
				"%s: failed to set fast_response\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_info(true, ts->dev, "%s: OK\n", __func__);
}

/*
 *	index  0 :  set edge handler
 *		1 :  portrait (normal) mode
 *		2 :  landscape mode
 *
 *	data
 *		0, X (direction), X (y start), X (y end)
 *		direction : 0 (off), 1 (left), 2 (right)
 *			ex) echo set_grip_data,0,2,600,900 > cmd
 *
 *		1, X (edge zone), X (dead zone up x), X (dead zone down x), X (dead zone up y), X (dead zone bottom x), X (dead zone down y)
 *			ex) echo set_grip_data,1,60,10,32,926,32,3088 > cmd
 *
 *		2, 1 (landscape mode), X (edge zone), X (dead zone x), X (dead zone top y), X (dead zone bottom y), X (edge zone top y), X (edge zone bottom y)
 *			ex) echo set_grip_data,2,1,200,100,120,0 > cmd
 *
 *		2, 0 (portrait mode)
 *			ex) echo set_grip_data,2,0  > cmd
 */

static int grip_data_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int mode;

	mode = sec_input_store_grip_data(ts->dev, sec->cmd_param);
	if (mode < 0) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;
	return mode;
}

static void set_grip_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int mode;

	mode = grip_data_store(device_data);
	if (mode < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	mutex_lock(&ts->plat_data->enable_mutex);
	synaptics_set_grip_data_to_ic(ts->dev, mode);
	mutex_unlock(&ts->plat_data->enable_mutex);
}

static int fix_active_mode_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	ts->fix_active_mode = sec->cmd_param[0];
	input_info(true, ts->dev, "%s: %d\n", __func__, sec->cmd_param[0]);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return SEC_SUCCESS;
}

static void fix_active_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = fix_active_mode_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_DISABLE_DOZE, ts->fix_active_mode);
	if (ret < 0) {
		input_err(true, ts->dev,
				"%s: failed to set fix active mode\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_info(true, ts->dev, "%s: OK\n", __func__);
};

enum prox_intensity_test_item {
	PROX_INTENSITY_TEST_THD_X = 0,
	PROX_INTENSITY_TEST_THD_Y,
	PROX_INTENSITY_TEST_SUM_X,
	PROX_INTENSITY_TEST_SUM_Y,
	PROX_INTENSITY_TEST_MAX,
};
static void run_prox_intensity_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	struct synaptics_ts_buffer test_data;
	short *data_ptr = NULL;
	short value[PROX_INTENSITY_TEST_MAX] = { 0 };
	int ret, i;

	sec_cmd_set_default_result(sec);

	if (!ts->plat_data->support_ear_detect) {
		input_info(true, ts->dev, "%s: ear detection is not supported\n", __func__);
		snprintf(buff, sizeof(buff), "NA");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		return;
	}

	synaptics_ts_buf_init(&test_data);

	ret = synaptics_ts_run_production_test(ts,
			TEST_PID198_PROX_INTENSITY,
			&test_data);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Fail to run test %d\n",
				__func__, TEST_PID198_PROX_INTENSITY);
		synaptics_ts_buf_release(&test_data);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		return;
	}

	data_ptr = (short *)&test_data.buf[0];
	for (i = 0; i < PROX_INTENSITY_TEST_MAX; i++) {
		value[i] = *data_ptr;
		data_ptr++;
	}

	synaptics_ts_buf_release(&test_data);

	snprintf(buff, sizeof(buff), "SUM_X:%d SUM_Y:%d THD_X:%d THD_Y:%d",
			value[PROX_INTENSITY_TEST_SUM_X], value[PROX_INTENSITY_TEST_SUM_Y],
			value[PROX_INTENSITY_TEST_THD_X], value[PROX_INTENSITY_TEST_THD_Y]);
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, ts->dev, "%s: %s\n", __func__, buff);
}

static void touch_aging_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	unsigned char resp_code;
	unsigned char command;
	struct synaptics_ts_buffer resp;
	int ret = 0;

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Device is not in application fw mode, mode: %x\n", __func__,
			ts->dev_mode);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	if (sec->cmd_param[0])
		command = CMD_SET_AGING_MODE_ON;
	else
		command = CMD_SET_AGING_MODE_OFF;

	synaptics_ts_buf_init(&resp);

	ret = synaptics_ts_send_command(ts,
			command,
			NULL,
			0,
			&resp_code,
			&resp,
			FORCE_ATTN_DRIVEN);
	if (ret < 0)
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	else
		sec->cmd_state = SEC_CMD_STATUS_OK;

	synaptics_ts_buf_release(&resp);

	input_info(true, ts->dev, "%s: %d %s\n", __func__,
			sec->cmd_param[0], sec->cmd_state == SEC_CMD_STATUS_OK ? "OK" : "NG");
}

/*	for game mode
 *	byte[0]: Setting for the Game Mode with 240Hz scan rate
 *		- 0: Disable
 *		- 1: Enable
 *
 *	byte[1]: Vsycn mode
 *		- 0: Normal 60
 *		- 1: HS60
 *		- 2: HS120
 *		- 3: VSYNC 48
 *		- 4: VSYNC 96
 */

static int scan_rate_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1 || sec->cmd_param[1] < 0 || sec->cmd_param[1] > 4) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	ts->scan_rate = sec->cmd_param[0];
	input_info(true, ts->dev, "%s: %d\n", __func__, sec->cmd_param[0]);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return SEC_SUCCESS;
}

static void set_scan_rate(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = scan_rate_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_SET_SCANRATE, ts->scan_rate);
	if (ret < 0) {
		input_err(true, ts->dev,
				"%s: failed to set fix_active_mode\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_info(true, ts->dev, "%s: OK\n", __func__);
}

/*
 * cmd_param[0] means,
 * 0 : normal (60hz)
 * 1 : adaptive
 * 2 : always (120hz)
 */

static int refresh_rate_mode_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (!ts->plat_data->support_refresh_rate_mode) {
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return SEC_ERROR;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 2) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	if (sec->cmd_param[0] > 0) {
		/* 120hz */
		ts->refresh_rate = 1 | (2 << 8);
	} else {
		/* 60hz */
		ts->refresh_rate = 0 | (1 << 8);
	}

	input_info(true, ts->dev, "%s: %d, refresh_rate:0x%02X\n",
			__func__, sec->cmd_param[0], ts->refresh_rate);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return SEC_SUCCESS;
}
static void refresh_rate_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = refresh_rate_mode_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_SET_SCANRATE, ts->refresh_rate);
	if (ret < 0) {
		input_err(true, ts->dev,
				"%s: failed to set refresh rate mode\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static int sip_mode_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	ts->sip_mode = sec->cmd_param[0];
	input_info(true, ts->dev, "%s: %d\n", __func__, sec->cmd_param[0]);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return SEC_SUCCESS;
}

static void set_sip_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = sip_mode_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_SIPMODE, ts->sip_mode);
	if (ret < 0) {
		input_err(true, ts->dev,
				"%s: failed to set sip mode\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static int wirelesscharger_mode_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	input_info(true, ts->dev, "%s: %d,%d\n", __func__, sec->cmd_param[0], sec->cmd_param[1]);

	ret = sec_input_check_wirelesscharger_mode(ts->dev, sec->cmd_param[0], sec->cmd_param[1]);
	if (ret == SEC_ERROR)
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	else
		sec->cmd_state = SEC_CMD_STATUS_OK;

	return ret;
}

static void set_wirelesscharger_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = wirelesscharger_mode_store(device_data);
	if (ret != SEC_SUCCESS)
		return;

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_WIRELESS_CHARGER,
						ts->plat_data->wirelesscharger_mode);
	if (ret < 0) {
		input_err(true, ts->dev,
				"%s: failed to set wireless charge mode\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static int note_mode_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return SEC_ERROR;
	}

	ts->note_mode = sec->cmd_param[0];
	input_info(true, ts->dev, "%s: %d\n", __func__, sec->cmd_param[0]);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return SEC_SUCCESS;
}

static void set_note_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	int ret;

	ret = note_mode_store(device_data);
	if (ret < 0) {
		input_info(true, ts->dev, "%s: NG\n", __func__);
		return;
	}

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_NOTEMODE, ts->note_mode);
	if (ret < 0) {
		input_err(true, ts->dev,
				"%s: failed to set note mode\n", __func__);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static void debug(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	ts->debug_flag = sec->cmd_param[0];

	input_info(true, ts->dev, "%s: %d\n", __func__, ts->debug_flag);
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void set_fold_state(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (IS_NOT_FOLD_DEV(ts->multi_dev)) {
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	sec_input_set_fold_state(ts->multi_dev, sec->cmd_param[0]);
	ts->irq_empty_count = 0;

	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static int rawdata_store(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (!ts->plat_data->support_rawdata) {
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return SEC_ERROR;
	}

	ts->raw_mode = sec->cmd_param[0];
	input_info(true, ts->dev, "%s: %d\n", __func__, sec->cmd_param[0]);
	sec->cmd_state = SEC_CMD_STATUS_OK;
#if IS_ENABLED(CONFIG_SEC_INPUT_RAWDATA)
	sec_input_rawdata_buffer_alloc();
#endif
	return SEC_SUCCESS;
}

static void rawdata_init(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (rawdata_store(device_data) < 0)
		return;

	if (ts->raw_mode) {
		if (ts->game_mode)
			input_info(true, ts->dev, "%s: game mode is ongoing\n", __func__);
		else
			synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_EXTENDED_TOUCH_REPORT_CONTROL, 0x01);
	} else {
		synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_EXTENDED_TOUCH_REPORT_CONTROL, 0x00);
	}

	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static void blocking_palm(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	if (!ts->plat_data->support_rawdata) {
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ts->plat_data->blocking_palm = sec->cmd_param[0];
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s: OK\n", __func__);
}

static void get_status(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	snprintf(buff, sizeof(buff), "WET:%d,NOISE:%d,FREQ:%d",
			ts->plat_data->wet_mode, ts->plat_data->noise_mode, ts->plat_data->freq_id);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, ts->dev, "%s: %s\n", __func__, buff);
}

static void not_support_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct synaptics_ts_data *ts = container_of(sec, struct synaptics_ts_data, sec);

	input_info(true, ts->dev, "%s\n", __func__);
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
}

static struct sec_cmd sec_cmds[] = {
	{SEC_CMD_V2("fw_update", fw_update, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("get_fw_ver_bin", get_fw_ver_bin, NULL, CHECK_ALL, WAIT_RESULT),},
	{SEC_CMD_V2("get_fw_ver_ic", get_fw_ver_ic, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("module_off_master", module_off_master, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("module_on_master", module_on_master, NULL, CHECK_POWEROFF, WAIT_RESULT),},
	{SEC_CMD_V2("get_chip_vendor", get_chip_vendor, NULL, CHECK_ALL, WAIT_RESULT),},
	{SEC_CMD_V2("get_chip_name", get_chip_name, NULL, CHECK_ALL, WAIT_RESULT),},
	{SEC_CMD_V2("get_x_num", get_x_num, NULL, CHECK_ALL, WAIT_RESULT),},
	{SEC_CMD_V2("get_y_num", get_y_num, NULL, CHECK_ALL, WAIT_RESULT),},
	{SEC_CMD_V2("run_full_rawcap_read", run_full_rawcap_read, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_full_rawcap_read_all", run_full_rawcap_read_all, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_trans_rawcap_read", run_trans_rawcap_read, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_trans_rawcap_read_all", run_trans_rawcap_read_all, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_delta_noise_read", run_delta_noise_read, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_delta_noise_read_all", run_delta_noise_read_all, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_abs_rawcap_read", run_abs_rawcap_read, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_abs_rawcap_read_all", run_abs_rawcap_read_all, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_abs_noise_read", run_abs_noise_read, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_abs_noise_read_all", run_abs_noise_read_all, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("get_gap_data_x_all", get_gap_data_x_all, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("get_gap_data_y_all", get_gap_data_y_all, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("get_gap_data_all", get_gap_data_all, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_sram_test", run_sram_test, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_ate_test", run_ate_test, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_hsync_test", run_hsync_test, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_extended_trx_test", run_extended_trx_test, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_interrupt_gpio_test", run_interrupt_gpio_test, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_factory_miscalibration", run_factory_miscalibration, check_support_calibration,
			CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_miscalibration", run_miscalibration, check_support_calibration, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_force_calibration", run_force_calibration, run_force_calibration_store,
			CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_miscal_data_read_all", run_miscal_data_read_all, check_support_calibration,
			CHECK_ON_LP, WAIT_RESULT),},
	// {SEC_CMD_V2("get_wet_mode", get_wet_mode, NULL, CHECK_ALL, WAIT_RESULT),},
	{SEC_CMD_V2("run_cs_raw_read_all", run_trans_rawcap_read_all, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_cs_delta_read_all", run_delta_noise_read_all, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_rawdata_read_all_for_ghost", run_rawdata_read_all, NULL, CHECK_ALL, WAIT_RESULT),},
	{SEC_CMD_V2("run_trx_short_test", run_trx_short_test, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_jitter_delta_test", run_jitter_delta_test, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("get_crc_check", get_crc_check, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("factory_cmd_result_all", factory_cmd_result_all, NULL, CHECK_ALL, WAIT_RESULT),},
	{SEC_CMD_V2("factory_cmd_result_all_imagetest", factory_cmd_result_all_imagetest, NULL,
			CHECK_ALL, WAIT_RESULT),},
	{SEC_CMD_V2("run_snr_non_touched", run_snr_non_touched, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("run_snr_touched", run_snr_touched, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("set_factory_level", set_factory_level, NULL, CHECK_ALL, WAIT_RESULT),},
	{SEC_CMD_V2_H("fix_active_mode", fix_active_mode, fix_active_mode_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2_H("touch_aging_mode", touch_aging_mode, NULL, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2("run_prox_intensity_read_all", run_prox_intensity_read_all, NULL, CHECK_ON_LP, WAIT_RESULT),},
#ifdef TCLM_CONCEPT
	{SEC_CMD_V2("set_tsp_test_result", set_tsp_test_result, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("get_tsp_test_result", get_tsp_test_result, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("clear_tsp_test_result", clear_tsp_test_result, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("increase_disassemble_count", increase_disassemble_count, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("get_disassemble_count", get_disassemble_count, NULL, CHECK_ON_LP, WAIT_RESULT),},
	{SEC_CMD_V2("get_pat_information", get_pat_information, NULL, CHECK_ALL, WAIT_RESULT),},
	{SEC_CMD_V2("set_external_factory", set_external_factory, NULL, CHECK_ALL, WAIT_RESULT),},
#endif
	{SEC_CMD_V2_H("glove_mode", glove_mode, glove_mode_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2("dead_zone_enable", dead_zone_enable, dead_zone_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2_H("clear_cover_mode", clear_cover_mode, clear_cover_mode_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2_H("set_wirelesscharger_mode", set_wirelesscharger_mode, wirelesscharger_mode_store,
			CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2_H("spay_enable", spay_enable, spay_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2("set_aod_rect", set_aod_rect, aod_rect_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2_H("aod_enable", aod_enable, aod_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2("set_aod_noti_rect", set_aod_noti_rect, aod_noti_rect_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2_H("aot_enable", aot_enable, aot_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2("fod_enable", fod_enable, fod_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2_H("fod_lp_mode", fod_lp_mode, NULL, CHECK_ALL, EXIT_RESULT),},
	{SEC_CMD_V2("set_fod_rect", set_fod_rect, fod_rect_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2("fp_int_control", fp_int_control, NULL, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2_H("singletap_enable", singletap_enable, singletap_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2_H("ear_detect_enable", ear_detect_enable, ear_detect_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2_H("pocket_mode_enable", pocket_mode_enable, pocket_mode_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2("low_sensitivity_mode_enable", low_sensitivity_mode_enable, low_sensitivity_mode_store,
			CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2("set_grip_data", set_grip_data, grip_data_store, CHECK_ON_LP, EXIT_RESULT),},
	// {SEC_CMD_V2_H("external_noise_mode", external_noise_mode, external_noise_mode_store,
	//		CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2_H("set_scan_rate", set_scan_rate, scan_rate_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2_H("refresh_rate_mode", refresh_rate_mode, refresh_rate_mode_store, CHECK_ON_LP, EXIT_RESULT),},
	// {SEC_CMD_V2_H("set_touchable_area", set_touchable_area, touchable_area_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2("set_sip_mode", set_sip_mode, sip_mode_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2("set_note_mode", set_note_mode, note_mode_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2_H("set_game_mode", set_game_mode, game_mode_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2_H("set_fast_response", set_fast_response, fast_response_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2("debug", debug, NULL, CHECK_ALL, EXIT_RESULT),},
	{SEC_CMD_V2("rawdata_init", rawdata_init, rawdata_store, CHECK_ON_LP, EXIT_RESULT),},
	{SEC_CMD_V2("blocking_palm", blocking_palm, NULL, CHECK_ALL, EXIT_RESULT),},
	{SEC_CMD_V2("set_fold_state", set_fold_state, NULL, CHECK_ALL, EXIT_RESULT),},
	{SEC_CMD_V2("get_status", get_status, NULL, CHECK_ALL, EXIT_RESULT),},
	{SEC_CMD_V2("not_support_cmd", not_support_cmd, NULL, CHECK_ALL, EXIT_RESULT),},
};

int synaptics_ts_fn_init(struct synaptics_ts_data *ts)
{
	int retval = 0;

	retval = sec_cmd_init(&ts->sec, ts->dev, sec_cmds, ARRAY_SIZE(sec_cmds),
			GET_SEC_CLASS_DEVT_TSP(ts->multi_dev), &cmd_attr_group);
	if (retval < 0) {
		input_err(true, ts->dev,
				"%s: Failed to sec_cmd_init\n", __func__);
		return retval;
	}

	retval = sysfs_create_link(&ts->sec.fac_dev->kobj,
			&ts->plat_data->input_dev->dev.kobj, "input");
	if (retval < 0) {
		input_err(true, ts->dev,
				"%s: Failed to create input symbolic link\n",
				__func__);
		goto err_create_link;
	}

	return 0;

err_create_link:
	sec_cmd_exit(&ts->sec, GET_SEC_CLASS_DEVT_TSP(ts->multi_dev));
	return retval;

}

void synaptics_ts_fn_remove(struct synaptics_ts_data *ts)
{
	input_err(true, ts->dev, "%s\n", __func__);

	sysfs_remove_link(&ts->sec.fac_dev->kobj, "input");

	sec_cmd_exit(&ts->sec, GET_SEC_CLASS_DEVT_TSP(ts->multi_dev));
}

MODULE_LICENSE("GPL");
