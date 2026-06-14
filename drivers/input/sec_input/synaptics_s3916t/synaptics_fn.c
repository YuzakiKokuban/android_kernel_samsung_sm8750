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

/**
 * syna_tcm_send_command()
 *
 * Helper to forward the custom commnd to the device
 *
 * @param
 *    [ in] tcm_dev:        the device handle
 *    [ in] command:        TouchComm command
 *    [ in] payload:        data payload, if any
 *    [ in] payload_length: length of data payload, if any
 *    [out] resp_code:      response code returned
 *    [out] resp:           buffer to store the response data
 *    [ in] delay_ms_resp:  delay time for response reading.
 *                          '0' is in default; and,
 *                          'FORCE_ATTN_DRIVEN' is to read resp in ISR
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
int synaptics_ts_send_command(struct synaptics_ts_data *ts,
			unsigned char command, unsigned char *payload,
			unsigned int payload_length, unsigned char *resp_code,
			struct synaptics_ts_buffer *resp, unsigned int delay_ms_resp)
{
	int retval = 0;

	if (!ts) {
		pr_err("%s%s: Invalid tcm device handle\n", SECLOG, __func__);
		return SEC_ERROR;
	}

	if (!resp_code) {
		input_err(true, ts->dev, "Invalid parameter\n");
		return SEC_ERROR;
	}

	retval = ts->write_message(ts,
			command,
			payload,
			payload_length,
			resp_code,
			delay_ms_resp);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to send command 0x%02x\n",
			command);
		goto exit;
	}

	/* response data returned */
	if ((resp != NULL) && (ts->resp_buf.data_length > 0)) {
		retval = synaptics_ts_buf_copy(resp, &ts->resp_buf);
		if (retval < 0) {
			input_err(true, ts->dev, "Fail to copy resp data\n");
			goto exit;
		}
	}

exit:
	return retval;
}

/**
 * syna_tcm_send_immediate_command()
 *
 * Helper to forward the immediate custom commnd to the device. This
 * function only support the immediate command. For having the response
 * please use the generic commands through the synaptics_ts_send_command
 * function. The firmware will not send the response for the immediate
 * commands including the error code.
 *
 * @param
 *    [ in] tcm_dev:        the device handle
 *    [ in] command:        TouchComm immediate command
 *    [ in] payload:        data payload, if any
 *    [ in] payload_length: length of data payload, if any
 *
 * @return
 *    on success for sending the command, 0 or positive value;
 *    otherwise, negative value on error.
 */
int synaptics_ts_send_immediate_command(struct synaptics_ts_data *ts,
			unsigned char command, unsigned char *payload,
			unsigned int payload_length)
{
	int retval = 0;

	if (!ts) {
		pr_err("%s%s: Invalid tcm device handle\n", SECLOG, __func__);
		return SEC_ERROR;
	}

	retval = ts->write_immediate_message(ts,
			command,
			payload,
			payload_length);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to send immediate command 0x%02x\n",
			command);
	}

	return retval;
}


/**
 * syna_tcm_rezero()
 *
 * Implement the application fw command code to force the device to rezero its
 * baseline estimate.
 *
 * @param
 *    [ in] tcm_dev: the device handle
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
int synaptics_ts_rezero(struct synaptics_ts_data *ts)
{
	int retval = 0;
	unsigned char resp_code;

	input_err(true, ts->dev, "%s:\n", __func__);

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s:Device is not in application fw mode, mode: %x\n", __func__,
			ts->dev_mode);
		return -EINVAL;
	}

	retval = ts->write_message(ts,
			SYNAPTICS_TS_CMD_REZERO,
			NULL,
			0,
			&resp_code,
			0);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to send command 0x%02x\n",
			SYNAPTICS_TS_CMD_REZERO);
		goto exit;
	}


	retval = 0;
exit:
	return retval;
}

/**
 * synaptics_ts_pal_mem_cpy()
 *
 * Ensure the safe size before copying the values of num bytes from the
 * location to the memory block pointed to by destination.
 *
 * @param
 *    [out] dest:      pointer to the destination space
 *    [ in] dest_size: size of destination array
 *    [ in] src:       pointer to the source of data to be copied
 *    [ in] src_size:  size of source array
 *    [ in] num:       number of bytes to copy
 *
 * @return
 *    0 on success; otherwise, on error.
 */
inline int synaptics_ts_memcpy(void *dest, unsigned int dest_size,
		const void *src, unsigned int src_size, unsigned int num)
{
	if (dest == NULL || src == NULL)
		return -1;

	if (num > dest_size || num > src_size)
		return -1;

	memcpy((void *)dest, (const void *)src, num);

	return 0;
}

/**
 * syna_tcm_get_boot_info()
 *
 * Implement the bootloader command code, which is used to request a
 * boot information packet.
 *
 * @param
 *    [ in] tcm_dev:   the device handle
 *    [out] boot_info: the boot info packet returned
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
int synaptics_ts_get_boot_info(struct synaptics_ts_data *ts,
		struct synaptics_ts_boot_info *boot_info)
{
	int retval = 0;
	unsigned char resp_code;
	unsigned int resp_data_len = 0;
	unsigned int copy_size;

	retval = ts->write_message(ts,
			SYNAPTICS_TS_CMD_GET_BOOT_INFO,
			NULL,
			0,
			&resp_code,
			0);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to send command 0x%02x\n", __func__,
			SYNAPTICS_TS_CMD_GET_BOOT_INFO);
		goto exit;
	}

	resp_data_len =  ts->resp_buf.data_length;
	copy_size = MIN(sizeof(struct synaptics_ts_boot_info), resp_data_len);

	/* save the boot_info */
	retval = synaptics_ts_memcpy((unsigned char *)&ts->boot_info,
			sizeof(struct synaptics_ts_boot_info),
			ts->resp_buf.buf,
			ts->resp_buf.buf_size,
			copy_size);
	if (retval < 0) {
		input_err(true, ts->dev, "%s:Fail to copy boot info\n", __func__);
		goto exit;
	}

	if (boot_info == NULL)
		goto exit;

	/* copy boot_info to caller */
	retval = synaptics_ts_memcpy((unsigned char *)boot_info,
			sizeof(struct synaptics_ts_boot_info),
			ts->resp_buf.buf,
			ts->resp_buf.buf_size,
			copy_size);
	if (retval < 0)
		input_err(true, ts->dev, "%s:Fail to copy boot info to caller\n", __func__);
exit:
	return retval;
}


/**
 * syna_tcm_get_app_info()
 *
 * Implement the application fw command code to request an application
 * info packet from device.
 *
 * @param
 *    [ in] tcm_dev:  the device handle
 *    [out] app_info: the application info packet returned
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
int synaptics_ts_get_app_info(struct synaptics_ts_data *ts,
		struct synaptics_ts_application_info *app_info)
{
	int retval = 0;
	unsigned char resp_code;
	unsigned int app_status;
	unsigned int resp_data_len = 0;
	unsigned int copy_size;
	struct synaptics_ts_application_info *info;
	int i;

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s:Device is not in application fw mode, mode: %x\n", __func__,
			ts->dev_mode);
		return -EINVAL;
	}

	retval = ts->write_message(ts,
			SYNAPTICS_TS_CMD_GET_APPLICATION_INFO,
			NULL,
			0,
			&resp_code,
			0);
	if (retval < 0) {
		input_err(true, ts->dev, "%s:Fail to send command 0x%02x\n", __func__,
			SYNAPTICS_TS_CMD_GET_APPLICATION_INFO);
		goto exit;
	}

	resp_data_len = ts->resp_buf.data_length;
	copy_size = MIN(sizeof(ts->app_info), resp_data_len);

	info = &ts->app_info;

	/* save the app_info */
	retval = synaptics_ts_memcpy((unsigned char *)info,
			sizeof(struct synaptics_ts_application_info),
			ts->resp_buf.buf,
			ts->resp_buf.buf_size,
			copy_size);
	if (retval < 0) {
		input_err(true, ts->dev, "%s:Fail to copy application info\n", __func__);
		goto exit;
	}

	if (app_info == NULL)
		goto show_info;

	/* copy app_info to caller */
	retval = synaptics_ts_memcpy((unsigned char *)app_info,
			sizeof(struct synaptics_ts_application_info),
			ts->resp_buf.buf,
			ts->resp_buf.buf_size,
			copy_size);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to copy application info to caller\n", __func__);
		goto exit;
	}

show_info:
	app_status = synaptics_ts_pal_le2_to_uint(ts->app_info.status);

	if (app_status == APP_STATUS_BAD_APP_CONFIG) {
		input_err(true, ts->dev, "%s: Bad application firmware, status: 0x%x\n", __func__, app_status);
		retval = -ENODEV;
		goto exit;
	} else if (app_status != APP_STATUS_OK) {
		input_err(true, ts->dev, "%s:Incorrect application status, 0x%x\n", __func__, app_status);
		retval = -ENODEV;
		goto exit;
	}

	ts->max_objects = synaptics_ts_pal_le2_to_uint(info->max_objects);
	ts->max_x = synaptics_ts_pal_le2_to_uint(info->max_x);
	ts->max_y = synaptics_ts_pal_le2_to_uint(info->max_y);
	ts->plat_data->max_x = ts->max_x;
	ts->plat_data->max_y = ts->max_y;

	ts->cols = synaptics_ts_pal_le2_to_uint(info->num_of_image_cols);
	ts->rows = synaptics_ts_pal_le2_to_uint(info->num_of_image_rows);

	ts->plat_data->x_node_num = ts->rows;
	ts->plat_data->y_node_num = ts->cols;

	synaptics_ts_pal_mem_cpy((unsigned char *)ts->config_id,
			MAX_SIZE_CONFIG_ID,
			info->customer_config_id,
			MAX_SIZE_CONFIG_ID,
			MAX_SIZE_CONFIG_ID);

	for (i = 0; i < 4; i++)
		ts->plat_data->img_version_of_ic[i] = ts->config_id[i];

	input_info(true, ts->dev, "%s: ic:%02x%02x%02x%02x\n", __func__,
		ts->plat_data->img_version_of_ic[0], ts->plat_data->img_version_of_ic[1],
		ts->plat_data->img_version_of_ic[2], ts->plat_data->img_version_of_ic[3]);

	input_info(true, ts->dev, "%s: App info version: %d, status: %d\n", __func__,
		synaptics_ts_pal_le2_to_uint(info->version), app_status);
	input_info(true, ts->dev, "%s: App info: max_objs: %d, max_x:%d, max_y: %d, img: %dx%d\n", __func__,
		ts->max_objects, ts->max_x, ts->max_y,
		ts->rows, ts->cols);

exit:
	return retval;
}

/**
 * syna_tcm_v1_dispatch_report()
 *
 * Handle the TouchCom report packet being received.
 *
 * If it's an identify report, parse the identification packet and signal
 * the command completion just in case.
 * Otherwise, copy the data from internal buffer.in to internal buffer.report
 *
 * @param
 *    [ in] tcm_dev: the device handle
 *
 * @return
 *    none.
 */
static void synaptics_ts_v1_dispatch_report(struct synaptics_ts_data *ts)
{
	int retval;
	struct synaptics_ts_message_data_blob *tcm_msg = NULL;
	synaptics_ts_pal_completion_t *cmd_completion = NULL;

	if (!ts) {
		pr_err("%s%s: Invalid tcm device handle\n", SECLOG, __func__);
		return;
	}

	tcm_msg = &ts->msg_data;
	cmd_completion = &tcm_msg->cmd_completion;

	tcm_msg->report_code = tcm_msg->status_report_code;

	if (tcm_msg->payload_length == 0) {
		ts->report_buf.data_length = 0;
		goto exit;
	}

	/* The identify report may be resulted from reset or fw mode switching
	 */
	if (tcm_msg->report_code == REPORT_IDENTIFY) {

		synaptics_ts_buf_lock(&tcm_msg->in);

		retval = synaptics_ts_v1_parse_idinfo(ts,
				&tcm_msg->in.buf[SYNAPTICS_TS_MESSAGE_HEADER_SIZE],
				tcm_msg->in.buf_size - SYNAPTICS_TS_MESSAGE_HEADER_SIZE,
				tcm_msg->payload_length);
		if (retval < 0) {
			input_err(true, ts->dev, "%s: Fail to identify device\n", __func__);
			synaptics_ts_buf_unlock(&tcm_msg->in);
			return;
		}

		synaptics_ts_buf_unlock(&tcm_msg->in);

		/* in case, the identify info packet is caused by the command */
		if (ATOMIC_GET(tcm_msg->command_status) == CMD_STATE_BUSY) {
			switch (tcm_msg->command) {
			case SYNAPTICS_TS_CMD_RESET:
				input_err(true, ts->dev, "%s: Reset by CMD_RESET\n", __func__);
				fallthrough;
			case SYNAPTICS_TS_CMD_REBOOT_TO_ROM_BOOTLOADER:
			case SYNAPTICS_TS_CMD_RUN_BOOTLOADER_FIRMWARE:
			case SYNAPTICS_TS_CMD_RUN_APPLICATION_FIRMWARE:
			case SYNAPTICS_TS_CMD_ENTER_PRODUCTION_TEST_MODE:
			case SYNAPTICS_TS_CMD_ROMBOOT_RUN_BOOTLOADER_FIRMWARE:
				tcm_msg->status_report_code = STATUS_OK;
				tcm_msg->response_code = STATUS_OK;
				ATOMIC_SET(tcm_msg->command_status,
					CMD_STATE_IDLE);
				synaptics_ts_pal_completion_complete(cmd_completion);
				goto exit;
			default:
				input_err(true, ts->dev, "%s: Device has been reset\n", __func__);
				ATOMIC_SET(tcm_msg->command_status,
					CMD_STATE_ERROR);
				synaptics_ts_pal_completion_complete(cmd_completion);
				goto exit;
			}
		}
	}

	/* store the received report into the internal buffer.report */
	synaptics_ts_buf_lock(&ts->report_buf);

	retval = synaptics_ts_buf_alloc(&ts->report_buf,
			tcm_msg->payload_length);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to allocate memory for internal buf.report\n", __func__);
		synaptics_ts_buf_unlock(&ts->report_buf);
		goto exit;
	}

	synaptics_ts_buf_lock(&tcm_msg->in);

	retval = synaptics_ts_pal_mem_cpy(ts->report_buf.buf,
			ts->report_buf.buf_size,
			&tcm_msg->in.buf[SYNAPTICS_TS_MESSAGE_HEADER_SIZE],
			tcm_msg->in.buf_size - SYNAPTICS_TS_MESSAGE_HEADER_SIZE,
			tcm_msg->payload_length);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to copy payload to buf_report\n", __func__);
		synaptics_ts_buf_unlock(&tcm_msg->in);
		synaptics_ts_buf_unlock(&ts->report_buf);
		goto exit;
	}

	ts->report_buf.data_length = tcm_msg->payload_length;

	synaptics_ts_buf_unlock(&tcm_msg->in);
	synaptics_ts_buf_unlock(&ts->report_buf);

exit:
	return;
}

/**
 * syna_tcm_v1_dispatch_response()
 *
 * Handle the response packet.
 *
 * Copy the data from internal buffer.in to internal buffer.resp,
 * and then signal the command completion.
 *
 * @param
 *    [ in] tcm_dev: the device handle
 *
 * @return
 *    none.
 */
static void synaptics_ts_v1_dispatch_response(struct synaptics_ts_data *ts)
{
	int retval;
	struct synaptics_ts_message_data_blob *tcm_msg = NULL;
	synaptics_ts_pal_completion_t *cmd_completion = NULL;

	if (!ts) {
		pr_err("%s%s: Invalid tcm device handle\n", SECLOG, __func__);
		return;
	}

	tcm_msg = &ts->msg_data;
	cmd_completion = &tcm_msg->cmd_completion;

	tcm_msg->response_code = tcm_msg->status_report_code;

	if (ATOMIC_GET(tcm_msg->command_status) != CMD_STATE_BUSY)
		return;

	if (tcm_msg->payload_length == 0) {
		ts->resp_buf.data_length = tcm_msg->payload_length;
		ATOMIC_SET(tcm_msg->command_status, CMD_STATE_IDLE);
		goto exit;
	}

	/* copy the received resp data into the internal buffer.resp */
	synaptics_ts_buf_lock(&ts->resp_buf);

	retval = synaptics_ts_buf_alloc(&ts->resp_buf,
			tcm_msg->payload_length);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to allocate memory for internal buf.resp\n", __func__);
		synaptics_ts_buf_unlock(&ts->resp_buf);
		goto exit;
	}

	synaptics_ts_buf_lock(&tcm_msg->in);

	retval = synaptics_ts_pal_mem_cpy(ts->resp_buf.buf,
			ts->resp_buf.buf_size,
			&tcm_msg->in.buf[SYNAPTICS_TS_MESSAGE_HEADER_SIZE],
			tcm_msg->in.buf_size - SYNAPTICS_TS_MESSAGE_HEADER_SIZE,
			tcm_msg->payload_length);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to copy payload to internal resp_buf\n", __func__);
		synaptics_ts_buf_unlock(&tcm_msg->in);
		synaptics_ts_buf_unlock(&ts->resp_buf);
		goto exit;
	}

	ts->resp_buf.data_length = tcm_msg->payload_length;

	synaptics_ts_buf_unlock(&tcm_msg->in);
	synaptics_ts_buf_unlock(&ts->resp_buf);

	ATOMIC_SET(tcm_msg->command_status, CMD_STATE_IDLE);

exit:
	synaptics_ts_pal_completion_complete(cmd_completion);
}

static int synaptics_ts_v1_continued_read(struct synaptics_ts_data *ts,
		unsigned int length)
{
	int retval = 0;
	unsigned char marker;
	unsigned char code;
	unsigned int idx;
	unsigned int offset;
	unsigned int chunks;
	unsigned int chunk_space;
	unsigned int xfer_length;
	unsigned int total_length;
	unsigned int remaining_length;
	struct synaptics_ts_message_data_blob *tcm_msg = NULL;

	if (!ts) {
		pr_err("%s%s: Invalid tcm device handle\n", SECLOG, __func__);
		return -EINVAL;
	}

	tcm_msg = &ts->msg_data;

	/* continued read packet contains the header, payload, and a padding */
	total_length = SYNAPTICS_TS_MESSAGE_HEADER_SIZE + length + 1;
	remaining_length = total_length - SYNAPTICS_TS_MESSAGE_HEADER_SIZE;

	if (total_length > PAGE_SIZE) {
		input_fail_hist(true, ts->dev, "%s: Invalid length %d to read\n", __func__, total_length);
		return -EINVAL;
	}

	synaptics_ts_buf_lock(&tcm_msg->in);

	/* in case the current buf.in is smaller than requested size */
	retval = synaptics_ts_buf_realloc(&tcm_msg->in,
			total_length + 1);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to allocate memory for internal buf_in\n", __func__);
		synaptics_ts_buf_unlock(&tcm_msg->in);
		return retval;
	}

	/* delay for the bus turnaround time */
	synaptics_ts_pal_sleep_us(MSG_DELAY_US_MIN, MSG_DELAY_US_MAX);

	/* available chunk space for payload =
	 *     total chunk size - (header marker byte + header status byte)
	 */
	if (ts->max_rd_size == 0)
		chunk_space = remaining_length;
	else
		chunk_space = ts->max_rd_size - 2;

	chunks = synaptics_ts_pal_ceil_div(remaining_length, chunk_space);
	chunks = chunks == 0 ? 1 : chunks;

	offset = SYNAPTICS_TS_MESSAGE_HEADER_SIZE;

	synaptics_ts_buf_lock(&tcm_msg->temp);

	for (idx = 0; idx < chunks; idx++) {
		if (remaining_length > chunk_space)
			xfer_length = chunk_space;
		else
			xfer_length = remaining_length;

		if (xfer_length == 1) {
			tcm_msg->in.buf[offset] = SYNAPTICS_TS_V1_MESSAGE_PADDING;
			offset += xfer_length;
			remaining_length -= xfer_length;
			continue;
		}

		/* allocate the internal temp buffer */
		retval = synaptics_ts_buf_alloc(&tcm_msg->temp,
				xfer_length + 2);
		if (retval < 0) {
			input_err(true, ts->dev, "%s: Fail to allocate memory for internal buf.temp\n", __func__);
			synaptics_ts_buf_unlock(&tcm_msg->temp);
			synaptics_ts_buf_unlock(&tcm_msg->in);
			return retval;
		}
		/* retrieve data from the bus
		 * data should include header marker and status code
		 */
		retval = ts->synaptics_ts_read_data_only(ts,
				tcm_msg->temp.buf,
				xfer_length + 2);
		if (retval < 0) {
			input_err(true, ts->dev, "%s: Fail to read %d bytes from device\n", __func__,
				xfer_length + 2);
			synaptics_ts_buf_unlock(&tcm_msg->temp);
			synaptics_ts_buf_unlock(&tcm_msg->in);
			return retval;
		}

		/* check the data content */
		marker = tcm_msg->temp.buf[0];
		code = tcm_msg->temp.buf[1];

		if (marker != SYNAPTICS_TS_V1_MESSAGE_MARKER) {
			input_err(true, ts->dev, "%s: Incorrect header marker, 0x%02x\n", __func__,
					marker);
			synaptics_ts_buf_unlock(&tcm_msg->temp);
			synaptics_ts_buf_unlock(&tcm_msg->in);
			return -EIO;
		}

		if (code != STATUS_CONTINUED_READ) {
			input_err(true, ts->dev, "%s: Incorrect header status code, 0x%02x\n", __func__,
					code);
			synaptics_ts_buf_unlock(&tcm_msg->temp);
			synaptics_ts_buf_unlock(&tcm_msg->in);
			return -EIO;
		}

		/* copy data from internal buffer.temp to buffer.in */
		retval = synaptics_ts_pal_mem_cpy(&tcm_msg->in.buf[offset],
				tcm_msg->in.buf_size - offset,
				&tcm_msg->temp.buf[2],
				tcm_msg->temp.buf_size - 2,
				xfer_length);
		if (retval < 0) {
			input_err(true, ts->dev, "%s: Fail to copy payload\n", __func__);
			synaptics_ts_buf_unlock(&tcm_msg->temp);
			synaptics_ts_buf_unlock(&tcm_msg->in);
			return retval;
		}

		offset += xfer_length;
		remaining_length -= xfer_length;
	}

	synaptics_ts_buf_unlock(&tcm_msg->temp);
	synaptics_ts_buf_unlock(&tcm_msg->in);

	return 0;
}


/**
 * syna_tcm_v1_read_message()
 *
 * Read in a TouchCom packet from device.
 * The packet including its payload is read in from device and stored in
 * the internal buffer.resp or buffer.report based on the code received.
 *
 * @param
 *    [ in] tcm_dev:            the device handle
 *    [out] status_report_code: status code or report code received
 *
 * @return
 *    0 or positive value on success; otherwise, on error.
 */
static int synaptics_ts_v1_read_message(struct synaptics_ts_data *ts, unsigned char *status_report_code)
{
	int retval = 0;
	bool retry;
	struct synaptics_ts_message_data_blob *tcm_msg = NULL;
	synaptics_ts_pal_mutex_t *rw_mutex = NULL;
	synaptics_ts_pal_completion_t *cmd_completion = NULL;

	if (!ts) {
		pr_err("%s%s:Invalid tcm device handle\n", SECLOG, __func__);
		return -EINVAL;
	}

	if (sec_check_secure_trusted_mode_status(ts->plat_data))
		return -EBUSY;

	if (!ts->header) {
		ts->header = devm_kzalloc(ts->dev, sizeof(struct synaptics_ts_v1_message_header), GFP_KERNEL);
		if (!ts->header)
			return -ENODEV;
	}

#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
	tcm_msg = &ts->msg_data;
	rw_mutex = &tcm_msg->rw_mutex;
	cmd_completion = &tcm_msg->cmd_completion;

	if (status_report_code)
		*status_report_code = STATUS_INVALID;

	synaptics_ts_pal_mutex_lock(rw_mutex);

	retry = true;

retry:
	synaptics_ts_buf_lock(&tcm_msg->in);

	/* in case the current in.buf is smaller than header size */
	retval = synaptics_ts_buf_realloc(&tcm_msg->in, SYNAPTICS_TS_MESSAGE_HEADER_SIZE);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to allocate memory for internal buf_in\n", __func__);
		synaptics_ts_buf_unlock(&tcm_msg->in);
		tcm_msg->status_report_code = STATUS_INVALID;
		tcm_msg->payload_length = 0;
		goto exit;
	}

	/* read in the message header from device */
	retval = ts->synaptics_ts_read_data_only(ts,
			tcm_msg->in.buf,
			SYNAPTICS_TS_MESSAGE_HEADER_SIZE);
	if (retval < 0) {
		input_err(true, ts->dev, "%s:Fail to read message header from device\n", __func__);
		synaptics_ts_buf_unlock(&tcm_msg->in);

		tcm_msg->status_report_code = STATUS_INVALID;
		tcm_msg->payload_length = 0;

		if (retry) {
			synaptics_ts_pal_sleep_us(RD_RETRY_US_MIN, RD_RETRY_US_MAX);
			retry = false;
			goto retry;
		}
		goto exit;
	}

	/* check the message header */
	memcpy(ts->header, tcm_msg->in.buf, sizeof(struct synaptics_ts_v1_message_header));

	if (ts->header->marker != SYNAPTICS_TS_V1_MESSAGE_MARKER) {
		input_err(true, ts->dev, "%s:Incorrect header marker, 0x%02x\n", __func__, ts->header->marker);
		synaptics_ts_buf_unlock(&tcm_msg->in);

		tcm_msg->status_report_code = STATUS_INVALID;
		tcm_msg->payload_length = 0;

		retval = -1;
		if (retry) {
			synaptics_ts_pal_sleep_us(RD_RETRY_US_MIN, RD_RETRY_US_MAX);
			retry = false;
			goto retry;
		}
		goto exit;
	}

	tcm_msg->status_report_code = ts->header->code;

	tcm_msg->payload_length = synaptics_ts_pal_le2_to_uint(ts->header->length);

	if (tcm_msg->status_report_code != STATUS_IDLE)
		input_dbg(false, ts->dev, "%s:Status code: 0x%02x, length: %d (%02x %02x %02x %02x)\n", __func__,
			tcm_msg->status_report_code, tcm_msg->payload_length,
			ts->header->marker, ts->header->code, ts->header->length[0],
			ts->header->length[1]);

	synaptics_ts_buf_unlock(&tcm_msg->in);

	/* retrieve the remaining data, if any */
	if (tcm_msg->payload_length > 0) {
		retval = synaptics_ts_v1_continued_read(ts,
				tcm_msg->payload_length);
		if (retval < 0) {
			input_err(true, ts->dev, "Fail to do continued read\n");
			goto exit;
		}
	}

	if ((tcm_msg->status_report_code <= STATUS_ERROR) ||
		(tcm_msg->status_report_code == STATUS_INVALID)) {
		switch (tcm_msg->status_report_code) {
		case STATUS_OK:
			break;
		case STATUS_CONTINUED_READ:
			input_err(true, ts->dev, "%s:Out-of-sync continued read\n", __func__);
			fallthrough;
		case STATUS_IDLE:
			tcm_msg->payload_length = 0;
			retval = 0;
			goto exit;
		case STATUS_BAD_COMMAND:
			input_fail_hist(true, ts->dev, "%s:Status Bad\n", __func__);
			tcm_msg->payload_length = 0;
			goto do_dispatch;
		default:
			input_fail_hist(true, ts->dev, "%s:Incorrect Status code, 0x%02x\n", __func__,
				tcm_msg->status_report_code);
			tcm_msg->payload_length = 0;
			goto do_dispatch;
		}
	}

	/* refill the header for dispatching */
	synaptics_ts_buf_lock(&tcm_msg->in);

	tcm_msg->in.buf[0] = SYNAPTICS_TS_V1_MESSAGE_MARKER;
	tcm_msg->in.buf[1] = tcm_msg->status_report_code;
	tcm_msg->in.buf[2] = (unsigned char)tcm_msg->payload_length;
	tcm_msg->in.buf[3] = (unsigned char)(tcm_msg->payload_length >> 8);

	synaptics_ts_buf_unlock(&tcm_msg->in);

do_dispatch:
	/* duplicate the data to external buffer */
	if (tcm_msg->payload_length > 0) {
		retval = synaptics_ts_buf_alloc(&ts->external_buf,
				tcm_msg->payload_length);
		if (retval < 0) {
			input_err(true, ts->dev, "%s:Fail to allocate memory, external_buf invalid\n", __func__);
		} else {
			retval = synaptics_ts_pal_mem_cpy(&ts->external_buf.buf[0],
				tcm_msg->payload_length,
				&tcm_msg->in.buf[SYNAPTICS_TS_MESSAGE_HEADER_SIZE],
				tcm_msg->in.buf_size - SYNAPTICS_TS_MESSAGE_HEADER_SIZE,
				tcm_msg->payload_length);
			if (retval < 0)
				input_err(true, ts->dev, "%s: Fail to copy data to external buffer\n", __func__);
		}
	}
	ts->external_buf.data_length = tcm_msg->payload_length;

	/* process the retrieved packet */
	if (tcm_msg->status_report_code >= REPORT_IDENTIFY)
		synaptics_ts_v1_dispatch_report(ts);
	else
		synaptics_ts_v1_dispatch_response(ts);

	/* copy the status report code to caller */
	if (status_report_code)
		*status_report_code = tcm_msg->status_report_code;

	retval = 0;

exit:
	if (retval < 0) {
		if (ATOMIC_GET(tcm_msg->command_status) == CMD_STATE_BUSY) {
			ATOMIC_SET(tcm_msg->command_status, CMD_STATE_ERROR);
			synaptics_ts_pal_completion_complete(cmd_completion);
		}
	}

	/* have a delay in case the next message comes in a very short period */
	synaptics_ts_pal_sleep_us(MSG_DELAY_US_MIN, MSG_DELAY_US_MAX);

	synaptics_ts_pal_mutex_unlock(rw_mutex);

	return retval;

}

static int synaptics_ts_v1_write_message(struct synaptics_ts_data *ts,
	unsigned char command, unsigned char *payload,
	unsigned int payload_len, unsigned char *resp_code,
	unsigned int delay_ms_resp)
{
	int retval = 0;
	unsigned int idx;
	unsigned int chunks;
	unsigned int chunk_space;
	unsigned int xfer_length;
	unsigned int remaining_length;
	int timeout = 0;
	int cmd_response_timeout = CMD_RESPONSE_TIMEOUT_MS;
	int polling_ms = 0;
	struct synaptics_ts_message_data_blob *tcm_msg = NULL;
	synaptics_ts_pal_mutex_t *cmd_mutex = NULL;
	synaptics_ts_pal_mutex_t *rw_mutex = NULL;
	synaptics_ts_pal_completion_t *cmd_completion = NULL;

	if (!ts) {
		pr_err("%s%s: Invalid tcm device handle\n", SECLOG, __func__);
		return -EINVAL;
	}

	if (sec_check_secure_trusted_mode_status(ts->plat_data))
		return -EBUSY;

#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif

	tcm_msg = &ts->msg_data;
	cmd_mutex = &tcm_msg->cmd_mutex;
	rw_mutex = &tcm_msg->rw_mutex;
	cmd_completion = &tcm_msg->cmd_completion;

	if (resp_code)
		*resp_code = STATUS_INVALID;

	synaptics_ts_pal_mutex_lock(cmd_mutex);

	if (delay_ms_resp != FORCE_ATTN_DRIVEN)
		disable_irq_nosync(ts->irq);

	synaptics_ts_pal_mutex_lock(rw_mutex);

	ATOMIC_SET(tcm_msg->command_status, CMD_STATE_BUSY);

	/* reset the command completion */
	synaptics_ts_pal_completion_reset(cmd_completion);

	tcm_msg->command = command;

	/* adding two length bytes as part of payload */
	remaining_length = payload_len + 2;

	/* available space for payload = total size - command byte */
	if (ts->max_wr_size == 0)
		chunk_space = remaining_length;
	else
		chunk_space = ts->max_wr_size - 1;

	chunks = synaptics_ts_pal_ceil_div(remaining_length, chunk_space);
	chunks = chunks == 0 ? 1 : chunks;

	input_dbg(false, ts->dev, "%s: Command: 0x%02x, payload len: %d\n", __func__, command, payload_len);

	synaptics_ts_buf_lock(&tcm_msg->out);

	for (idx = 0; idx < chunks; idx++) {
		if (remaining_length > chunk_space)
			xfer_length = chunk_space;
		else
			xfer_length = remaining_length;

		/* allocate the space storing the written data */
		retval = synaptics_ts_buf_alloc(&tcm_msg->out,
				xfer_length + 1);
		if (retval < 0) {
			input_err(true, ts->dev, "%s: Fail to allocate memory for internal buf.out\n", __func__);
			synaptics_ts_buf_unlock(&tcm_msg->out);
			synaptics_ts_pal_mutex_unlock(rw_mutex);
			goto exit;
		}

		/* construct the command packet */
		if (idx == 0) {
			tcm_msg->out.buf[0] = tcm_msg->command;
			tcm_msg->out.buf[1] = (unsigned char)payload_len;
			tcm_msg->out.buf[2] = (unsigned char)(payload_len >> 8);

			if (xfer_length > 2) {
				retval = synaptics_ts_pal_mem_cpy(&tcm_msg->out.buf[3],
						tcm_msg->out.buf_size - 3,
						payload,
						remaining_length - 2,
						xfer_length - 2);
				if (retval < 0) {
					input_err(true, ts->dev, "%s: Fail to copy payload\n", __func__);
					synaptics_ts_buf_unlock(&tcm_msg->out);
					synaptics_ts_pal_mutex_unlock(rw_mutex);
					goto exit;
				}
			}
		}
		/* construct the continued writes packet */
		else {
			tcm_msg->out.buf[0] = SYNAPTICS_TS_CMD_CONTINUE_WRITE;

			retval = synaptics_ts_pal_mem_cpy(&tcm_msg->out.buf[1],
					tcm_msg->out.buf_size - 1,
					&payload[idx * chunk_space - 2],
					remaining_length,
					xfer_length);
			if (retval < 0) {
				input_err(true, ts->dev, "%s: Fail to copy continued write\n", __func__);
				synaptics_ts_buf_unlock(&tcm_msg->out);
				synaptics_ts_pal_mutex_unlock(rw_mutex);
				goto exit;
			}
		}

		/* write command packet to the device */
		retval = ts->synaptics_ts_write_data(ts,
				tcm_msg->out.buf,
				xfer_length + 1, NULL, 0);
		if (retval < 0) {
			input_err(true, ts->dev, "%s: Fail to write %d bytes to device\n", __func__,
				xfer_length + 1);
			synaptics_ts_buf_unlock(&tcm_msg->out);
			synaptics_ts_pal_mutex_unlock(rw_mutex);
			goto exit;
		}

		remaining_length -= xfer_length;

		if (chunks > 1)
			synaptics_ts_pal_sleep_us(WR_DELAY_US_MIN, WR_DELAY_US_MAX);
	}

	synaptics_ts_buf_unlock(&tcm_msg->out);

	synaptics_ts_pal_mutex_unlock(rw_mutex);

	/* polling the command response */
	timeout = 0;

	if ((command == SYNAPTICS_TS_CMD_REZERO) || (command == CMD_READ_SEC_SPONGE_REG) ||
			(command == CMD_WRITE_SEC_SPONGE_REG) || (command == CMD_SET_GRIP))
		cmd_response_timeout = CMD_RESPONSE_SHORT_TIMEOUT_MS;

	if (delay_ms_resp == FORCE_ATTN_DRIVEN)
		polling_ms = cmd_response_timeout;
	else
		polling_ms = (delay_ms_resp > 0) ? delay_ms_resp :
				CMD_RESPONSE_POLLING_DELAY_MS;

	do {
		if (atomic_read(&ts->plat_data->power_state) == SEC_INPUT_STATE_POWER_OFF)
			goto check_response;

		/* wait for the command completion triggered by read_message */
		retval = synaptics_ts_pal_completion_wait_for(cmd_completion,
				polling_ms);
		if (retval < 0)
			ATOMIC_SET(tcm_msg->command_status, CMD_STATE_BUSY);

		if (ATOMIC_GET(tcm_msg->command_status) != CMD_STATE_IDLE)
			timeout += polling_ms + 10;
		else
			goto check_response;

		/* retrieve the message packet back */
		if (delay_ms_resp != FORCE_ATTN_DRIVEN) {
			retval = synaptics_ts_v1_read_message(ts, NULL);
			if (retval < 0)
				synaptics_ts_pal_completion_reset(cmd_completion);
		}

	} while (timeout < cmd_response_timeout);

	if ((tcm_msg->response_code != STATUS_OK) && (timeout >= cmd_response_timeout)) {
		input_fail_hist(true, ts->dev, "Timed out waiting for response of command 0x%02x (%dms)\n",
			command, timeout);
		retval = -ETIMEDOUT;
		goto exit;
	}

check_response:
	if (ATOMIC_GET(tcm_msg->command_status) != CMD_STATE_IDLE) {
		input_err(true, ts->dev, "Fail to get valid response of command 0x%02x\n",
			command);
		retval = -EIO;
		goto exit;
	}

	/* copy response code to the caller */
	if (resp_code)
		*resp_code = tcm_msg->response_code;

	if (tcm_msg->response_code != STATUS_OK) {
		input_err(true, ts->dev, "Error code 0x%02x of command 0x%02x\n",
			tcm_msg->response_code, tcm_msg->command);
		retval = -EIO;
	} else {
		retval = 0;
	}

exit:
	tcm_msg->command = SYNAPTICS_TS_CMD_NONE;

	ATOMIC_SET(tcm_msg->command_status, CMD_STATE_IDLE);

	if (delay_ms_resp != FORCE_ATTN_DRIVEN)
		sec_input_irq_enable(ts->plat_data);

	synaptics_ts_pal_mutex_unlock(cmd_mutex);

	return retval;

}

static int synaptics_ts_v1_write_immediate_message(struct synaptics_ts_data *ts,
	unsigned char command, unsigned char *payload, unsigned int payload_len)
{
	int retval = 0;
	unsigned int idx;
	unsigned int chunks;
	unsigned int chunk_space;
	unsigned int xfer_length;
	unsigned int remaining_length;
	struct synaptics_ts_message_data_blob *tcm_msg = NULL;
	synaptics_ts_pal_mutex_t *cmd_mutex = NULL;
	synaptics_ts_pal_mutex_t *rw_mutex = NULL;

	if (!ts) {
		pr_err("%s%s: Invalid tcm device handle\n", SECLOG, __func__);
		return -EINVAL;
	}

	if (!ts->support_immediate_cmd) {
		input_err(true, ts->dev, "%s: not support immediate cmd\n", __func__);
		return -EINVAL;
	}

	if (sec_check_secure_trusted_mode_status(ts->plat_data))
		return -EBUSY;

#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif

	/* add the extra condition if any other immediate command is supported. */
	if (command != CMD_SET_IMMEDIATE_DYNAMIC_CONFIG && command != SYNAPTICS_TS_CMD_ERASE_FLASH) {
		pr_err("Invalid command, 0x%02x is not a immediate command\n", command);
		return -EINVAL;
	}

	tcm_msg = &ts->msg_data;
	cmd_mutex = &tcm_msg->cmd_mutex;
	rw_mutex = &tcm_msg->rw_mutex;

	synaptics_ts_pal_mutex_lock(cmd_mutex);

	synaptics_ts_pal_mutex_lock(rw_mutex);

	ATOMIC_SET(tcm_msg->command_status, CMD_STATE_BUSY);

	tcm_msg->command = command;

	/* adding two length bytes as part of payload */
	remaining_length = payload_len + 2;

	/* available space for payload = total size - command byte */
	if (ts->max_wr_size == 0)
		chunk_space = remaining_length;
	else
		chunk_space = ts->max_wr_size - 1;

	chunks = synaptics_ts_pal_ceil_div(remaining_length, chunk_space);
	chunks = chunks == 0 ? 1 : chunks;

	input_dbg(false, ts->dev, "%s: Command: 0x%02x, payload len: %d\n", __func__, command, payload_len);

	synaptics_ts_buf_lock(&tcm_msg->out);

	for (idx = 0; idx < chunks; idx++) {
		if (remaining_length > chunk_space)
			xfer_length = chunk_space;
		else
			xfer_length = remaining_length;

		/* allocate the space storing the written data */
		retval = synaptics_ts_buf_alloc(&tcm_msg->out,
				xfer_length + 1);
		if (retval < 0) {
			input_err(true, ts->dev, "%s: Fail to allocate memory for internal buf.out\n", __func__);
			synaptics_ts_buf_unlock(&tcm_msg->out);
			synaptics_ts_pal_mutex_unlock(rw_mutex);
			goto exit;
		}

		/* construct the command packet */
		if (idx == 0) {
			tcm_msg->out.buf[0] = tcm_msg->command;
			tcm_msg->out.buf[1] = (unsigned char)payload_len;
			tcm_msg->out.buf[2] = (unsigned char)(payload_len >> 8);

			if (xfer_length > 2) {
				retval = synaptics_ts_pal_mem_cpy(&tcm_msg->out.buf[3],
						tcm_msg->out.buf_size - 3,
						payload,
						remaining_length - 2,
						xfer_length - 2);
				if (retval < 0) {
					input_err(true, ts->dev, "%s: Fail to copy payload\n", __func__);
					synaptics_ts_buf_unlock(&tcm_msg->out);
					synaptics_ts_pal_mutex_unlock(rw_mutex);
					goto exit;
				}
			}
		}
		/* construct the continued writes packet */
		else {
			tcm_msg->out.buf[0] = SYNAPTICS_TS_CMD_CONTINUE_WRITE;

			retval = synaptics_ts_pal_mem_cpy(&tcm_msg->out.buf[1],
					tcm_msg->out.buf_size - 1,
					&payload[idx * chunk_space - 2],
					remaining_length,
					xfer_length);
			if (retval < 0) {
				input_err(true, ts->dev, "%s: Fail to copy continued write\n", __func__);
				synaptics_ts_buf_unlock(&tcm_msg->out);
				synaptics_ts_pal_mutex_unlock(rw_mutex);
				goto exit;
			}
		}

		/* write command packet to the device */
		retval = ts->synaptics_ts_write_data(ts,
				tcm_msg->out.buf,
				xfer_length + 1, NULL, 0);
		if (retval < 0) {
			input_err(true, ts->dev, "%s: Fail to write %d bytes to device\n", __func__,
				xfer_length + 1);
			synaptics_ts_buf_unlock(&tcm_msg->out);
			synaptics_ts_pal_mutex_unlock(rw_mutex);
			goto exit;
		}

		remaining_length -= xfer_length;

		if (chunks > 1)
			synaptics_ts_pal_sleep_us(WR_DELAY_US_MIN, WR_DELAY_US_MAX);
	}

	if (command == CMD_SET_IMMEDIATE_DYNAMIC_CONFIG && ts->mode) {
		cancel_delayed_work_sync(&ts->work_check_mode);
		ts->mode[payload[0]].drv_setting = payload[1] | (payload[2] << 8);
		ts->mode[payload[0]].is_waiting = true;
		if (!atomic_read(&ts->plat_data->shutdown_called))
			schedule_delayed_work(&ts->work_check_mode, msecs_to_jiffies(SYNAPTICS_TS_CHECK_MODE_DELAY));
	}

	synaptics_ts_buf_unlock(&tcm_msg->out);

	synaptics_ts_pal_mutex_unlock(rw_mutex);

	retval = 0;

exit:
	tcm_msg->command = SYNAPTICS_TS_CMD_NONE;

	ATOMIC_SET(tcm_msg->command_status, CMD_STATE_IDLE);

	synaptics_ts_pal_mutex_unlock(cmd_mutex);

	return retval;

}

/**
 * syna_tcm_enable_report()
 *
 * Implement the application fw command code to enable or disable a report.
 *
 * @param
 *    [ in] tcm_dev:     the device handle
 *    [ in] report_code: the requested report code being generated
 *    [ in] en:          '1' for enabling; '0' for disabling
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
int synaptics_ts_enable_report(struct synaptics_ts_data *ts,
		unsigned char report_code, bool en)
{
	int retval = 0;
	unsigned char resp_code;
	unsigned char command;

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Device is not in application fw mode, mode: %x\n", __func__,
			ts->dev_mode);
		return -EINVAL;
	}

	command = (en) ? SYNAPTICS_TS_CMD_ENABLE_REPORT : SYNAPTICS_TS_CMD_DISABLE_REPORT;

	retval = ts->write_message(ts,
			command,
			&report_code,
			1,
			&resp_code,
			0);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to send command 0x%02x to %s 0x%02x report\n", __func__,
			command, (en)?"enable":"disable", report_code);
		goto exit;
	}

	if (resp_code != STATUS_OK) {
		input_err(true, ts->dev, "%s: Fail to %s 0x%02x report, resp_code:%x\n", __func__,
			(en) ? "enable" : "disable", report_code, resp_code);
	} else {
		input_err(true, ts->dev, "%s: Report 0x%x %s\n", __func__, report_code,
			(en) ? "enabled" : "disabled");
	}

exit:
	return retval;
}

/**
 * syna_tcm_preserve_touch_report_config()
 *
 * Retrieve and preserve the current touch report configuration.
 *
 * The retrieved configuration is stored in touch_config buffer defined
 * in structure syna_tcm_dev for later using of touch position parsing.
 *
 * The touch_config buffer will be allocated internally and its size will
 * be updated accordingly.
 *
 * @param
 *    [ in] tcm_dev: the device handle
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
int synaptics_ts_preserve_touch_report_config(struct synaptics_ts_data *ts)
{
	int retval = 0;
	unsigned char resp_code;
	unsigned int size = 0;

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Device is not in application fw mode, mode: %x\n", __func__,
			ts->dev_mode);
		return -EINVAL;
	}

	retval = ts->write_message(ts,
			SYNAPTICS_TS_CMD_GET_TOUCH_REPORT_CONFIG,
			NULL,
			0,
			&resp_code,
			0);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to write command CMD_GET_TOUCH_REPORT_CONFIG\n");
		goto exit;
	}

	synaptics_ts_buf_lock(&ts->resp_buf);

	size = ts->resp_buf.data_length;
	retval = synaptics_ts_buf_alloc(&ts->touch_config,
			size);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to allocate memory for internal touch_config\n");
		synaptics_ts_buf_unlock(&ts->resp_buf);
		goto exit;
	}

	synaptics_ts_buf_lock(&ts->touch_config);

	retval = synaptics_ts_pal_mem_cpy(ts->touch_config.buf,
			ts->touch_config.buf_size,
			ts->resp_buf.buf,
			ts->resp_buf.buf_size,
			size);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to clone touch config\n");
		synaptics_ts_buf_unlock(&ts->touch_config);
		synaptics_ts_buf_unlock(&ts->resp_buf);
		goto exit;
	}

	ts->touch_config.data_length = size;

	synaptics_ts_buf_unlock(&ts->touch_config);
	synaptics_ts_buf_unlock(&ts->resp_buf);

exit:
	return retval;
}

int synaptics_ts_set_up_rawdata_report_type(struct synaptics_ts_data *ts)
{
	int retval = 0;

	if (!ts->plat_data->support_rawdata)
		return 0;

	/* VERIFY_SEC_REPORTS code segmant is used to verify the
	 * SEC reports in the prior stage, it should be disabled
	 * at the final release and configured in default fw.
	 */
	/* preserve the format of touch report */
	retval = synaptics_ts_preserve_touch_report_config(ts);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to preserve touch report config\n");
		return -EIO;
	}

	if (ts->raw_mode)
		retval = synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_EXTENDED_TOUCH_REPORT_CONTROL, 0x01);
	else
		retval = synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_EXTENDED_TOUCH_REPORT_CONTROL, 0x00);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to set immediate dynamic config\n", __func__);
		return -EIO;
	}

	return retval;
}
/**
 * syna_dev_set_up_app_fw()
 *
 * Implement the essential steps for the initialization including the
 * preparation of app info and the configuration of touch report.
 *
 * This function should be called whenever the device initially powers
 * up, resets, or firmware update.
 *
 * @param
 *    [ in] tcm: tcm driver handle
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
int synaptics_ts_set_up_app_fw(struct synaptics_ts_data *ts)
{
	int retval = 0;

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Application firmware not running, current mode: %02x\n", __func__,
			ts->dev_mode);
		return -EINVAL;
	}

	/* collect app info containing most of sensor information */
	retval = synaptics_ts_get_app_info(ts, &ts->app_info);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to get application info\n", __func__);
		return -EIO;
	}

	/* VERIFY_SEC_REPORTS code segmant is used to verify the
	 * SEC reports in the prior stage, it should be disabled
	 * at the final release and configured in default fw.
	 */
	/* preserve the format of touch report */
	retval = synaptics_ts_preserve_touch_report_config(ts);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to preserve touch report config\n");
		return -EIO;
	}

	/* disable the generic touchcomm touch report */
	retval = synaptics_ts_enable_report(ts,
			REPORT_TOUCH, false);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to disable touch report\n", __func__);
		return -EIO;
	}
	/* enable the sec reports */
	retval = synaptics_ts_enable_report(ts,
			REPORT_SEC_SPONGE_GESTURE, true);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to enable sec gesture report\n", __func__);
		return -EIO;
	}
	retval = synaptics_ts_enable_report(ts,
			REPORT_SEC_COORDINATE_EVENT, true);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to enable sec coordinate report\n", __func__);
		return -EIO;
	}
	retval = synaptics_ts_enable_report(ts,
			REPORT_SEC_STATUS_EVENT, true);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to enable sec status report\n", __func__);
		return -EIO;
	}

	return retval;
}


/**
 * syna_tcm_v1_parse_idinfo()
 *
 * Copy the given data to the identification info structure
 * and parse the basic information, e.g. fw build id.
 *
 * @param
 *    [ in] tcm_dev:  the device handle
 *    [ in] data:     data buffer
 *    [ in] size:     size of given data buffer
 *    [ in] data_len: length of actual data
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
int synaptics_ts_v1_parse_idinfo(struct synaptics_ts_data *ts,
		unsigned char *data, unsigned int size, unsigned int data_len)
{
	int retval;
	unsigned int wr_size = 0;
	unsigned int build_id = 0;
	struct synaptics_ts_identification_info *id_info;

	if ((!data) || (data_len == 0)) {
		input_err(true, ts->dev, "%s:Invalid given data buffer\n", __func__);
		return -EINVAL;
	}

	id_info = &ts->id_info;
	retval = synaptics_ts_memcpy((unsigned char *)id_info,
			sizeof(struct synaptics_ts_identification_info),
			data,
			size,
			MIN(sizeof(*id_info), data_len));
	if (retval < 0) {
		input_info(true, ts->dev, "%s: Fail to copy identification info\n", __func__);
		return retval;
	}

	build_id = synaptics_ts_pal_le4_to_uint(id_info->build_id);

	wr_size = synaptics_ts_pal_le2_to_uint(id_info->max_write_size);
	ts->max_wr_size = MIN(wr_size, ts->max_wr_size);
	if (ts->max_wr_size == 0)
		ts->max_wr_size = wr_size;

	input_info(true, ts->dev, "%s: max_wr_size = %d\n", __func__, ts->max_wr_size);

	input_info(true, ts->dev, "%s:TCM Fw mode: 0x%02x\n", __func__, id_info->mode);

	ts->packrat_number = build_id;

	ts->dev_mode = id_info->mode;

	return 0;
}

int synaptics_ts_v1_detect(struct synaptics_ts_data *ts)
{
	int retval;
	struct synaptics_ts_v1_message_header *header;
	struct synaptics_ts_message_data_blob *tcm_msg = NULL;
	unsigned int payload_length = 0;
	unsigned char data[4] = { 0 };

	if (!ts) {
		pr_err("%s%s:Invalid tcm device handle\n", SECLOG, __func__);
		return -EINVAL;
	}

	tcm_msg = &ts->msg_data;

	synaptics_ts_pal_mutex_lock(&tcm_msg->rw_mutex);

	retval = ts->synaptics_ts_get_baredata(ts, data, SYNAPTICS_TS_MESSAGE_HEADER_SIZE);
	if (retval < 0) {
		input_fail_hist(true, ts->dev, "%s: failed to get bare data\n", __func__);
		synaptics_ts_pal_mutex_unlock(&tcm_msg->rw_mutex);
		return -EINVAL;
	}

	if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x00) {
		input_fail_hist(true, ts->dev, "%s: baredata, %02x %02x %02x %02x\n", __func__,
				data[0], data[1], data[2], data[3]);
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		/* return when empty baredata while start_device for pretest without panel */
		if (mutex_is_locked(&ts->device_mutex)) {
			synaptics_ts_pal_mutex_unlock(&tcm_msg->rw_mutex);
			return -ENODEV;
		}
#endif
	} else {
		input_info(true, ts->dev, "%s: baredata, %02x %02x %02x %02x\n", __func__,
				data[0], data[1], data[2], data[3]);
	}

	header = (struct synaptics_ts_v1_message_header *)data;

	if ((header->marker == SYNAPTICS_TS_V1_MESSAGE_MARKER) && (header->length > 0)) {
		/* retrieve the identify info packet */
		payload_length = synaptics_ts_pal_le2_to_uint(header->length);
		retval = synaptics_ts_v1_continued_read(ts,
				payload_length);
		if (retval < 0) {
			input_info(true, ts->dev, "%s:Fail to read in identify info packet\n", __func__);
			synaptics_ts_pal_mutex_unlock(&tcm_msg->rw_mutex);
			return retval;
		}
	}

	synaptics_ts_pal_mutex_unlock(&tcm_msg->rw_mutex);

	if (header->code == REPORT_IDENTIFY) {
		synaptics_ts_buf_lock(&tcm_msg->in);
		retval = synaptics_ts_v1_parse_idinfo(ts,
				&tcm_msg->in.buf[SYNAPTICS_TS_MESSAGE_HEADER_SIZE],
				tcm_msg->in.buf_size - SYNAPTICS_TS_MESSAGE_HEADER_SIZE,
				payload_length);
		if (retval < 0) {
			input_info(true, ts->dev, "%s:Fail to identify device\n", __func__);
			synaptics_ts_buf_unlock(&tcm_msg->in);
			return retval;
		}
		synaptics_ts_buf_unlock(&tcm_msg->in);
	}

	/* expose the read / write operations */
	ts->read_message = synaptics_ts_v1_read_message;
	ts->write_message = synaptics_ts_v1_write_message;
	ts->write_immediate_message = synaptics_ts_v1_write_immediate_message;

	return retval;
}

int synaptics_ts_detect_protocol(struct synaptics_ts_data *ts)
{
	return synaptics_ts_v1_detect(ts);
}

int synaptics_ts_set_custom_library(struct synaptics_ts_data *ts)
{
	int retval;
	unsigned short offset;
	unsigned char data;
	unsigned char data_orig;
	unsigned char data_dump[2];
	unsigned char force_fod_enable = 0;

	offset = SEC_TS_CMD_SPONGE_FEATURE_ENABLE_0;

	retval = ts->synaptics_ts_read_sponge(ts,
			&data, sizeof(data), offset, 1);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to read sponge reg, offset:0x%x len:%d\n",
			offset, 1);
		return retval;
	}

	data_orig = data;

#if IS_ENABLED(CONFIG_SEC_FACTORY)
		/* enable FOD when LCD on state */
	if (ts->plat_data->support_fod && atomic_read(&ts->plat_data->enabled))
		force_fod_enable = SEC_TS_MODE_SPONGE_PRESS;
#endif

	if (ts->plat_data->prox_power_off) {
		data = ts->plat_data->lowpower_mode & ~SEC_TS_MODE_SPONGE_DOUBLETAP_TO_WAKEUP;
		input_info(true, ts->dev, "%s: prox off. disable AOT\n", __func__);
	} else {
		data = ts->plat_data->lowpower_mode | force_fod_enable;
	}

	input_info(true, ts->dev, "sponge data:0x%x (original:0x%x)%s\n",
			data, data_orig,
			force_fod_enable ? ", force fod enable" : "");

	retval = ts->synaptics_ts_write_sponge(ts,
			&data, sizeof(data), offset, 1);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to write sponge reg, data:0x%x offset:0x%x\n",
			data, offset);
		return retval;
	}

	/* aod noti rect enable */
	offset = SEC_TS_CMD_SPONGE_FEATURE_ENABLE_1;
	data = ts->plat_data->sponge_mode;
#if !IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
	/* inf dump enable */
	data = ts->plat_data->sponge_mode |= SEC_TS_MODE_SPONGE_INF_DUMP;
#endif
	input_info(true, ts->dev, "%s: Sponge_01 (0x%02x)\n",
			__func__, ts->plat_data->sponge_mode);

	retval = ts->synaptics_ts_write_sponge(ts, &data, 1, offset, 1);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to write sponge reg, data:0x%x, offset:0x%x\n",
			data, offset);
		return retval;
	}

	/* read dump info */
	offset = SEC_TS_CMD_SPONGE_LP_DUMP;
	retval = ts->synaptics_ts_read_sponge(ts,
			data_dump, sizeof(data_dump), offset, sizeof(data_dump));
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Failed to read dump_data\n", __func__);
		return retval;
	}

	/* synaptics ic have problem with sponge infinite dump. it takes too long time while isr.
	 * need to be enable after fix this issue
	 * ts->sponge_inf_dump = (data_dump[0] & SEC_TS_SPONGE_DUMP_INF_MASK) >> SEC_TS_SPONGE_DUMP_INF_SHIFT;
	 */
	ts->sponge_dump_format = data_dump[0] & SEC_TS_SPONGE_DUMP_EVENT_MASK;
	ts->sponge_dump_event = data_dump[1];
	ts->sponge_dump_border = SEC_TS_CMD_SPONGE_LP_DUMP_EVENT
					+ (ts->sponge_dump_format * ts->sponge_dump_event);
	ts->sponge_dump_border_lsb = ts->sponge_dump_border & 0xFF;
	ts->sponge_dump_border_msb = (ts->sponge_dump_border & 0xFF00) >> 8;

	return 0;
}

void synaptics_ts_get_custom_library(struct synaptics_ts_data *ts)
{
	u8 data[6] = { 0 };
	int ret, i;
	unsigned short offset;

	offset = SEC_TS_CMD_SPONGE_AOD_ACTIVE_INFO;
	ret = ts->synaptics_ts_read_sponge(ts,
			data, sizeof(data), offset, sizeof(data));
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Failed to read aod active area\n", __func__);
		return;
	}

	for (i = 0; i < 3; i++)
		ts->plat_data->aod_data.active_area[i] = (data[i * 2 + 1] & 0xFF) << 8 | (data[i * 2] & 0xFF);

	input_info(true, ts->dev, "%s: aod_active_area - top:%d, edge:%d, bottom:%d\n",
			__func__, ts->plat_data->aod_data.active_area[0],
			ts->plat_data->aod_data.active_area[1], ts->plat_data->aod_data.active_area[2]);

	memset(data, 0x00, 6);

	offset = SEC_TS_CMD_SPONGE_FOD_INFO;
	ret = ts->synaptics_ts_read_sponge(ts,
			data, sizeof(data), offset, sizeof(data));
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Failed to read fod info\n", __func__);
		return;
	}

	sec_input_set_fod_info(ts->dev, data[0], data[1], data[2], data[3]);

	offset = SEC_TS_CMD_SPONGE_FEATURE_ENABLE_1;
	ret = ts->synaptics_ts_read_sponge(ts, data, 1, offset, 1);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Failed to sponge_mode\n", __func__);
		return;
	}
	ts->plat_data->sponge_mode = data[0];

	input_info(true, ts->dev, "%s: sponge_mode %x\n", __func__, ts->plat_data->sponge_mode);
}

void synaptics_ts_set_fod_finger_merge(struct synaptics_ts_data *ts)
{

}

int synaptics_ts_read_from_sponge(struct synaptics_ts_data *ts, unsigned char *rd_buf, int size_buf,
		unsigned short offset, unsigned int rd_len)
{
	int retval = 0;
	unsigned char payload[4] = {0};
	unsigned char resp_code;
	struct synaptics_ts_buffer resp;

	if (ts->dev_mode != SYNAPTICS_TS_MODE_APPLICATION_FIRMWARE) {
		input_err(true, ts->dev, "Application firmware not running, current mode: %02x\n",
			ts->dev_mode);
		return SEC_ERROR;
	}

	if ((!rd_buf) || (size_buf == 0) || (size_buf < rd_len) || (rd_len == 0)) {
		input_err(true, ts->dev, "Invalid buffer size\n");
		return SEC_ERROR;
	}

	mutex_lock(&ts->sponge_mutex);

	synaptics_ts_buf_init(&resp);

	payload[0] = (unsigned char)(offset & 0xFF);
	payload[1] = (unsigned char)((offset >> 8) & 0xFF);
	payload[2] = (unsigned char)(rd_len & 0xFF);
	payload[3] = (unsigned char)((rd_len >> 8) & 0xFF);

	retval = synaptics_ts_send_command(ts,
			CMD_READ_SEC_SPONGE_REG,
			payload,
			sizeof(payload),
			&resp_code,
			&resp,
			FORCE_ATTN_DRIVEN);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to read from sponge, status:%x (offset:%x len:%d)\n",
			resp_code, offset, rd_len);
		goto exit;
	}

	if (resp.data_length != rd_len) {
		input_err(true, ts->dev, "Invalid data length\n");
		retval = SEC_ERROR;
		goto exit;
	}

	retval = synaptics_ts_pal_mem_cpy(rd_buf,
			size_buf,
			resp.buf,
			resp.buf_size,
			rd_len);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to copy sponge data to caller\n");
		goto exit;
	}

exit:
	synaptics_ts_buf_release(&resp);
	mutex_unlock(&ts->sponge_mutex);

	return retval;

}

int synaptics_ts_write_to_sponge(struct synaptics_ts_data *ts, unsigned char *wr_buf, int size_buf,
		unsigned short offset, unsigned int wr_len)
{
	int retval = 0;
	unsigned char *payload = NULL;
	unsigned int payload_size;
	unsigned char resp_code;
	struct synaptics_ts_buffer resp;

	if (ts->dev_mode != SYNAPTICS_TS_MODE_APPLICATION_FIRMWARE) {
		input_err(true, ts->dev, "Application firmware not running, current mode: %02x\n",
			ts->dev_mode);
		return SEC_ERROR;
	}

	if ((!wr_buf) || (size_buf == 0) || (size_buf < wr_len)) {
		input_err(true, ts->dev, "Invalid buffer size\n");
		return SEC_ERROR;
	}

	if (wr_len == 0)
		return 0;

	mutex_lock(&ts->sponge_mutex);

	synaptics_ts_buf_init(&resp);

	payload_size = wr_len + 2;
	payload = synaptics_ts_pal_mem_alloc(payload_size, sizeof(unsigned char));
	if (!payload) {
		input_err(true, ts->dev, "Fail to allocate payload buffer\n");
		goto exit;
	}

	payload[0] = (unsigned char)(offset & 0xFF);
	payload[1] = (unsigned char)((offset >> 8) & 0xFF);

	retval = synaptics_ts_pal_mem_cpy(&payload[2],
			payload_size - 2,
			wr_buf,
			size_buf,
			wr_len);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to copy sponge data to wr buffer\n");
		goto exit;
	}

	retval = synaptics_ts_send_command(ts,
			CMD_WRITE_SEC_SPONGE_REG,
			payload,
			payload_size,
			&resp_code,
			&resp,
			FORCE_ATTN_DRIVEN);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to write to sponge, status:%x (offset:%x len:%d)\n",
			resp_code, offset, wr_len);
		goto exit;
	}

exit:
	if (payload)
		synaptics_ts_pal_mem_free(payload);

	synaptics_ts_buf_release(&resp);
	mutex_unlock(&ts->sponge_mutex);
	return retval;
}

int synaptics_ts_set_touch_function(struct synaptics_ts_data *ts)
{
	return 0;
}

void synaptics_ts_get_touch_function(struct work_struct *work)
{

}

void synaptics_ts_external_func_work(struct work_struct *work)
{
	struct synaptics_ts_data *ts = container_of(work, struct synaptics_ts_data,
		set_temperature_work.work);

	cancel_delayed_work(&ts->set_temperature_work);
	synaptics_ts_external_func(ts);
}

void  synaptics_ts_set_utc_sponge(struct synaptics_ts_data *ts)
{
	int ret;
	u8 data[4] = {0};
	struct timespec64 current_time;

	ktime_get_real_ts64(&current_time);
	data[0] = (0xFF & (u8)((current_time.tv_sec) >> 0));
	data[1] = (0xFF & (u8)((current_time.tv_sec) >> 8));
	data[2] = (0xFF & (u8)((current_time.tv_sec) >> 16));
	data[3] = (0xFF & (u8)((current_time.tv_sec) >> 24));
	input_info(true, ts->dev, "Write UTC to Sponge = %X\n", (int)(current_time.tv_sec));

	ret = ts->synaptics_ts_write_sponge(ts, data, sizeof(data), SEC_TS_CMD_SPONGE_OFFSET_UTC, sizeof(data));
	if (ret < 0)
		input_err(true, ts->dev, "%s: Failed to write sponge\n", __func__);

}

/**
 * syna_tcm_get_dynamic_config()
 *
 * Implement the application fw command code to get the value from the a single
 * field of the dynamic configuration.
 *
 * @param
 *    [ in] tcm_dev:  the device handle
 *    [ in] id:       target field id
 *    [out] value:    the value returned
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
int synaptics_ts_get_dynamic_config(struct synaptics_ts_data *ts,
		unsigned char id, unsigned short *value)
{
	int retval = 0;
	unsigned char out;
	unsigned char resp_code;

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Device is not in application fw mode, mode: %x\n", __func__,
			ts->dev_mode);
		return -EINVAL;
	}

	out = (unsigned char)id;

	retval = ts->write_message(ts,
			SYNAPTICS_TS_CMD_GET_DYNAMIC_CONFIG,
			&out,
			sizeof(out),
			&resp_code,
			FORCE_ATTN_DRIVEN);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to send command 0x%02x to get dynamic field 0x%x\n", __func__,
			SYNAPTICS_TS_CMD_GET_DYNAMIC_CONFIG, (unsigned char)id);
		goto exit;
	}

	/* return dynamic config data */
	if (ts->resp_buf.data_length < 2) {
		input_err(true, ts->dev, "%s: Invalid resp data size, %d\n", __func__,
			ts->resp_buf.data_length);
		goto exit;
	}

	*value = (unsigned short)synaptics_ts_pal_le2_to_uint(ts->resp_buf.buf);

	input_err(true, ts->dev, "%s: Get %d from dynamic field 0x%x\n", __func__, *value, id);

	retval = 0;

exit:
	return retval;
}

/**
 * syna_tcm_set_dynamic_config()
 *
 * Implement the application fw command code to set the specified value to
 * the selected field of the dynamic configuration.
 *
 * @param
 *    [ in] tcm_dev:  the device handle
 *    [ in] id:       target field id
 *    [ in] value:    the value to the selected field
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
int synaptics_ts_set_dynamic_config(struct synaptics_ts_data *ts,
		unsigned char id, unsigned short value)
{
	int retval = 0;
	unsigned char out[3];
	unsigned char resp_code;

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Device is not in application fw mode, mode: %x\n", __func__,
			ts->dev_mode);
		return -EINVAL;
	}

	input_err(true, ts->dev, "%s: Set %d to dynamic field 0x%x\n", __func__, value, id);

	out[0] = (unsigned char)id;
	out[1] = (unsigned char)value;
	out[2] = (unsigned char)(value >> 8);

	retval = ts->write_message(ts,
			SYNAPTICS_TS_CMD_SET_DYNAMIC_CONFIG,
			out,
			sizeof(out),
			&resp_code,
			FORCE_ATTN_DRIVEN);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to send command 0x%02x to set %d to field 0x%x\n", __func__,
			SYNAPTICS_TS_CMD_SET_DYNAMIC_CONFIG, value, (unsigned char)id);
		goto exit;
	}

	retval = 0;

exit:
	return retval;
}

/**
 * syna_tcm_set_immediate_dynamic_config()
 *
 * Implement the application firmware command code to set the specified
 * value to the selected field of the immediate dynamic configuration.
 * The firmware will not send the response for immediate commands
 * including error response for setting the config. The firmware will
 * ignore the operation of dynamic config setting when failed to set the
 * config.
 *
 * @param
 *    [ in] tcm_dev:  the device handle
 *    [ in] id:       target field id
 *    [ in] value:    the value to the selected field
 *
 * @return
 *    on success for sending the config, 0 or positive value; otherwise,
 *    negative value on error.
 */
int synaptics_ts_set_immediate_dynamic_config(struct synaptics_ts_data *ts,
		unsigned char id, unsigned short value)
{
	int retval = 0;
	unsigned char out[3];

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Device is not in application fw mode, mode: %x\n", __func__,
			ts->dev_mode);
		return -EINVAL;
	}

	input_err(true, ts->dev, "%s: Set %d to dynamic field 0x%x\n", __func__, value, id);

	out[0] = (unsigned char)id;
	out[1] = (unsigned char)value;
	out[2] = (unsigned char)(value >> 8);

	retval = ts->write_immediate_message(ts,
			CMD_SET_IMMEDIATE_DYNAMIC_CONFIG,
			out,
			sizeof(out));
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to send immediate command 0x%02x to set %d to field 0x%x\n", __func__,
			CMD_SET_IMMEDIATE_DYNAMIC_CONFIG, value, (unsigned char)id);
		goto exit;
	}

exit:
	return retval;
}

int synaptics_ts_clear_buffer(struct synaptics_ts_data *ts)
{
	int retval = 0;
	unsigned char resp_code;
	struct synaptics_ts_buffer resp;

	input_err(true, ts->dev, "%s\n", __func__);

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Device is not in application fw mode, mode: %x\n", __func__,
			ts->dev_mode);
		return -EINVAL;
	}

	synaptics_ts_buf_init(&resp);

	retval = synaptics_ts_send_command(ts,
			CMD_EVENT_BUFFER_CLEAR,
			NULL,
			0,
			&resp_code,
			&resp,
			FORCE_ATTN_DRIVEN);
	if (retval < 0)
		input_err(true, ts->dev, "%s: write clear event failed\n", __func__);

	synaptics_ts_buf_release(&resp);

	return retval;
}

int synaptics_ts_set_lowpowermode(void *data, u8 mode)
{
	struct synaptics_ts_data *ts = (struct synaptics_ts_data *)data;
	struct irq_desc *desc = irq_to_desc(ts->irq);
	int ret = 0;
	int retrycnt = 0;
	unsigned short para;

	input_err(true, ts->dev, "%s: %s(%X)\n", __func__,
			mode == TO_LOWPOWER_MODE ? "ENTER" : "EXIT", ts->plat_data->lowpower_mode);

	if (mode) {
		synaptics_ts_set_utc_sponge(ts);

#if IS_ENABLED(CONFIG_TOUCHSCREEN_DUMP_MODE)
		if (ts->sponge_inf_dump) {
			if (ts->sponge_dump_delayed_flag) {
				synaptics_ts_sponge_dump_flush(ts, ts->sponge_dump_delayed_area);
				ts->sponge_dump_delayed_flag = false;
				input_info(true, ts->dev, "%s : Sponge dump flush delayed work have proceed\n", __func__);
			}
		}
#endif
		ret = synaptics_ts_set_custom_library(ts);
		if (ret < 0)
			goto error;
	} else {
		if (!atomic_read(&ts->plat_data->shutdown_called))
			schedule_work(&ts->work_read_functions.work);
	}

retry_pmode:
	if (mode)
		synaptics_ts_clear_buffer(ts);

	synaptics_ts_set_dynamic_config(ts, DC_ENABLE_WAKEUP_GESTURE_MODE, mode);
	synaptics_ts_get_dynamic_config(ts, DC_ENABLE_WAKEUP_GESTURE_MODE, &para);

	if (mode != (u8)para) {
		retrycnt++;
		ts->plat_data->hw_param.mode_change_failed_count++;
		if (retrycnt < 5)
			goto retry_pmode;
	}

	synaptics_ts_locked_release_all_finger(ts);

	if (mode) {
		while (desc->wake_depth < 1)
			enable_irq_wake(ts->irq);
	} else {
		while (desc->wake_depth > 0)
			disable_irq_wake(ts->irq);
	}

	if (mode == TO_LOWPOWER_MODE)
		atomic_set(&ts->plat_data->power_state, SEC_INPUT_STATE_LPM);
	else
		atomic_set(&ts->plat_data->power_state, SEC_INPUT_STATE_POWER_ON);

error:
	input_info(true, ts->dev, "%s: end %d\n", __func__, ret);

	return ret;
}

void synaptics_ts_release_all_finger(struct synaptics_ts_data *ts)
{
	sec_input_release_all_finger(ts->dev);

}

void synaptics_ts_locked_release_all_finger(struct synaptics_ts_data *ts)
{
	mutex_lock(&ts->eventlock);

	synaptics_ts_release_all_finger(ts);

	mutex_unlock(&ts->eventlock);
}

int synaptics_ts_setup(struct synaptics_ts_data *ts)
{
	int ret = 0;

	input_info(true, ts->dev, "%s\n", __func__);

	ret = synaptics_ts_detect_protocol(ts);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: fail to detect protocol\n", __func__);
		goto out;
	}

	/* check the running mode */
	switch (ts->dev_mode) {
	case SYNAPTICS_TS_MODE_APPLICATION_FIRMWARE:
		input_info(true, ts->dev, "%s: Device in Application FW, build id: %d, %s\n", __func__,
			ts->packrat_number,
			ts->id_info.part_number);
		break;
	case SYNAPTICS_TS_MODE_BOOTLOADER:
	case SYNAPTICS_TS_MODE_TDDI_BOOTLOADER:
		input_info(true, ts->dev, "%s: Device in Bootloader\n", __func__);
		goto out;
	case SYNAPTICS_TS_MODE_ROMBOOTLOADER:
		input_info(true, ts->dev, "%s: Device in ROMBoot uBL\n", __func__);
		goto out;
	case SYNAPTICS_TS_MODE_MULTICHIP_TDDI_BOOTLOADER:
		input_info(true, ts->dev, "%s: Device in multi-chip TDDI Bootloader\n", __func__);
		goto out;
	default:
		input_err(true, ts->dev, "%s: Found TouchCom device, but unsupported mode: 0x%02x\n", __func__,
			ts->dev_mode);
		goto out;
	}

	ret = synaptics_ts_set_up_app_fw(ts);
	if (ret < 0)
		input_err(true, ts->dev, "%s: Fail to set up application firmware\n", __func__);
out:
	return ret;
}

void synaptics_ts_hw_reset(struct synaptics_ts_data *ts)
{
	input_info(true, ts->dev, "%s\n", __func__);

	atomic_set(&ts->plat_data->power_state, SEC_INPUT_STATE_POWER_OFF);

	if (ts->plat_data->power)
		ts->plat_data->power(ts->dev, false);

	ts->plat_data->pinctrl_configure(ts->dev, false);

	if (gpio_is_valid(ts->plat_data->gpio_spi_cs))
		gpio_direction_output(ts->plat_data->gpio_spi_cs, 0);

	sec_delay(10);

	ts->plat_data->pinctrl_configure(ts->dev, true);

	if (ts->plat_data->power)
		ts->plat_data->power(ts->dev, true);

	if (gpio_is_valid(ts->plat_data->gpio_spi_cs))
		gpio_direction_output(ts->plat_data->gpio_spi_cs, 1);

	sec_delay(ts->power_on_delay);

	atomic_set(&ts->plat_data->power_state, SEC_INPUT_STATE_POWER_ON);
}


int synaptics_ts_soft_reset(struct synaptics_ts_data *ts)
{
	int retval = 0;
	unsigned char resp_code;
	unsigned int delay;

	delay = RESET_DELAY_MS;

	input_info(true, ts->dev, "%s: Recover IC, discharge time:%d\n", __func__, delay);

	retval = ts->write_message(ts,
			SYNAPTICS_TS_CMD_RESET,
			NULL,
			0,
			&resp_code,
			delay);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to send command 0x%02x\n", __func__, SYNAPTICS_TS_CMD_RESET);
		goto exit;
	}

	/* current device mode is expected to be updated
	 * because identification report will be received after reset
	 */
	ts->dev_mode = ts->id_info.mode;
	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Device mode 0x%02X running after reset\n", __func__,
			ts->dev_mode);
	}

	retval = 0;
exit:
	return retval;

}

void synaptics_ts_reset_work(struct work_struct *work)
{
	struct synaptics_ts_data *ts = container_of(work, struct synaptics_ts_data,
			reset_work.work);
	int ret;
	char result[32];
	char test[32];

	if (sec_check_secure_trusted_mode_status(ts->plat_data))
		return;

	if (atomic_read(&ts->reset_is_on_going)) {
		input_err(true, ts->dev, "%s: reset is ongoing\n", __func__);
		return;
	}

	mutex_lock(&ts->plat_data->enable_mutex);
	__pm_stay_awake(ts->plat_data->sec_ws);

	atomic_set(&ts->reset_is_on_going, 1);
	ts->plat_data->hw_param.ic_reset_count++;
	input_info(true, ts->dev, "%s\n", __func__);

	ts->plat_data->stop_device(ts);

	sec_delay(30);

	ret = ts->plat_data->start_device(ts);
	if (ret < 0) {
		/* for ACT i2c recovery fail test */
		snprintf(test, sizeof(test), "TEST=RECOVERY");
		snprintf(result, sizeof(result), "RESULT=FAIL");
		sec_cmd_send_event_to_user(&ts->sec, test, result);

		input_err(true, ts->dev, "%s: failed to reset, ret:%d\n", __func__, ret);
		atomic_set(&ts->reset_is_on_going, 0);
		cancel_delayed_work(&ts->reset_work);
		if (!atomic_read(&ts->plat_data->shutdown_called))
			schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
		mutex_unlock(&ts->plat_data->enable_mutex);

		snprintf(result, sizeof(result), "RESULT=RESET");
		sec_cmd_send_event_to_user(&ts->sec, NULL, result);

		__pm_relax(ts->plat_data->sec_ws);

		return;
	}

	if (!atomic_read(&ts->plat_data->enabled)) {
		input_err(true, ts->dev, "%s: call input_close\n", __func__);

		if (sec_input_need_ic_off(ts->plat_data)) {
			ts->plat_data->stop_device(ts);
		} else {
			ret = ts->plat_data->lpmode(ts, TO_LOWPOWER_MODE);
			if (ret < 0) {
				input_err(true, ts->dev, "%s: failed to reset, ret:%d\n", __func__, ret);
				atomic_set(&ts->reset_is_on_going, 0);
				cancel_delayed_work(&ts->reset_work);
				if (!atomic_read(&ts->plat_data->shutdown_called))
					schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
				mutex_unlock(&ts->plat_data->enable_mutex);
				__pm_relax(ts->plat_data->sec_ws);
				return;
			}
			if (ts->plat_data->lowpower_mode & SEC_TS_MODE_SPONGE_AOD)
				synaptics_ts_set_aod_rect(ts);
		}
	}

	atomic_set(&ts->reset_is_on_going, 0);
	mutex_unlock(&ts->plat_data->enable_mutex);

	snprintf(result, sizeof(result), "RESULT=RESET");
	sec_cmd_send_event_to_user(&ts->sec, NULL, result);

	__pm_relax(ts->plat_data->sec_ws);
}

void synatpics_ts_check_mode_work(struct work_struct *work)
{
	struct synaptics_ts_data *ts = container_of(work, struct synaptics_ts_data,
			work_check_mode.work);
	int i = 0;

	if (!ts->mode) {
		input_err(true, ts->dev, "%s: mode is not allocated\n", __func__);
		return;
	}

	if (atomic_read(&ts->plat_data->shutdown_called))
		return;

	for (i = 0; i < DC_MAX; i++)
		if (ts->mode[i].drv_setting != ts->mode[i].ic_state)
			input_fail_hist(true, ts->dev,
					"%s: mode 0x%02x%s : ic state %d is not matched with drv setting %d\n",
					__func__, i, ts->mode[i].is_waiting ? "(waiting)" : "",
					ts->mode[i].ic_state, ts->mode[i].drv_setting);
}

void synaptics_ts_print_info_work(struct work_struct *work)
{
	struct synaptics_ts_data *ts = container_of(work, struct synaptics_ts_data,
			work_print_info.work);

	sec_input_print_info(ts->dev, ts->tdata);

	if (atomic_read(&ts->sec.cmd_is_running))
		input_err(true, ts->dev, "%s: skip set temperature, cmd running\n", __func__);
	else
		sec_input_set_temperature(ts->dev, SEC_INPUT_SET_TEMPERATURE_NORMAL);

	if (!atomic_read(&ts->plat_data->shutdown_called))
		schedule_delayed_work(&ts->work_print_info, msecs_to_jiffies(TOUCH_PRINT_INFO_DWORK_TIME));
}

void synaptics_ts_read_info_work(struct work_struct *work)
{
	struct synaptics_ts_data *ts = container_of(work, struct synaptics_ts_data,
			work_read_info.work);
#ifdef TCLM_CONCEPT
	int ret;

	ret = sec_tclm_check_cal_case(ts->tdata);
	input_info(true, ts->dev, "%s: sec_tclm_check_cal_case ret: %d\n", __func__, ret);
#endif

	synaptics_ts_rawdata_read_all(ts);

	if (atomic_read(&ts->plat_data->shutdown_called)) {
		input_err(true, ts->dev, "%s done, do not run work\n", __func__);
		return;
	}
	ts->info_work_done = true;
	schedule_work(&ts->work_print_info.work);

	sec_input_multi_device_ready(ts->multi_dev);
}

void synaptics_ts_notify_pen_hover_work(struct work_struct *work)
{
	struct synaptics_ts_data *ts = container_of(work, struct synaptics_ts_data,
			work_notify_pen_hover.work);
	int ret;
	bool enable;

	if (atomic_read(&ts->plat_data->shutdown_called)) {
		input_err(true, ts->dev, "%s done, do not run work\n", __func__);
		return;
	}

	enable = !!(ts->pen_hover_in);

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_PEN_MODE, enable);

	input_info(true, ts->dev, "%s: pen mode %sable, ret=%d\n",
			__func__, enable ? "en" : "dis", ret);
}

void synaptics_ts_set_cover_type(struct synaptics_ts_data *ts, bool enable)
{
	int ret;
	u8 cover_cmd;

	input_info(true, ts->dev, "%s: %s, type:%d\n",
			__func__, enable ? "close" : "open", ts->plat_data->cover_type);

	cover_cmd = ts->plat_data->cover_type & 0xFF;
	ts->cover_closed = enable;

	if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
		input_err(true, ts->dev, "%s: pwr off, close:%d, touch_fn:%x\n", __func__,
				enable, ts->plat_data->touch_functions);
		return;
	}

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_COVER, enable);
	if (ret < 0) {
		input_err(true, ts->dev,
				"%s: Failed to send cover enable command: %d, ret:%d\n",
				__func__, enable, ret);
		return;
	}

	if (enable) {
		ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_COVERTYPE, cover_cmd);
		if (ret < 0) {
			input_err(true, ts->dev,
					"%s: Failed to send covertype command: %d, ret:%d\n",
					__func__, cover_cmd, ret);
			return;
		}
	}

	return;

}

int synaptics_ts_set_temperature(struct device *dev, u8 temperature_data)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);
	int ret;

	if (ts->plat_data->not_support_temp_noti) {
		input_info(true, ts->dev, "%s: SKIP! temp:%d\n",
					__func__, temperature_data);
		return SEC_ERROR;
	}

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_TSP_SET_TEMP, temperature_data);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: failed to set temperature_data(%d)\n",
					__func__, temperature_data);
	}

	return SEC_SUCCESS;
}

int synaptics_ts_set_aod_rect(struct synaptics_ts_data *ts)
{
	unsigned char data[8] = {0};
	int ret, i;
	unsigned short offset;

	for (i = 0; i < 4; i++) {
		data[i * 2] = ts->plat_data->aod_data.rect_data[i] & 0xFF;
		data[i * 2 + 1] = (ts->plat_data->aod_data.rect_data[i] >> 8) & 0xFF;
	}

	offset = SEC_TS_CMD_SPONGE_AOD_RECT;

	ret = ts->synaptics_ts_write_sponge(ts,
			data, sizeof(data), offset, sizeof(data));
	if (ret < 0)
		input_err(true, ts->dev, "%s: Failed to write sponge\n", __func__);

	return ret;
}

int synaptics_ts_set_aod_noti_rect(struct synaptics_ts_data *ts)
{
	unsigned char data[8] = {0};
	int ret, i;
	unsigned short offset;

	for (i = 0; i < 4; i++) {
		data[i * 2] = ts->plat_data->aod_noti_data.rect_data[i] & 0xFF;
		data[i * 2 + 1] = (ts->plat_data->aod_noti_data.rect_data[i] >> 8) & 0xFF;
	}

	offset = SEC_TS_CMD_SPONGE_AOD_NOTI_RECT;

	ret = ts->synaptics_ts_write_sponge(ts,
			data, sizeof(data), offset, sizeof(data));
	if (ret < 0)
		input_err(true, ts->dev, "%s: Failed to write sponge\n", __func__);

	return ret;
}

int synaptics_ts_set_press_property(struct synaptics_ts_data *ts)
{
	u8 data[3] = {0};
	unsigned short offset;
	int ret;

	if (!ts->plat_data->support_fod)
		return 0;

	offset = SEC_TS_CMD_SPONGE_PRESS_PROPERTY;
	data[0] = ts->plat_data->fod_data.press_prop;

	ret = ts->synaptics_ts_write_sponge(ts, data, sizeof(data), offset, sizeof(data));
	ret = 0;
	if (ret < 0)
		input_err(true, ts->dev, "%s: Failed to write sponge\n", __func__);

	input_info(true, ts->dev, "%s: %d\n", __func__, ts->plat_data->fod_data.press_prop);

	return ret;
}

int synaptics_ts_set_fod_rect(struct synaptics_ts_data *ts)
{
	u8 data[8] = {0};
	int ret, i;
	unsigned short offset;

	input_info(true, ts->dev, "%s: l:%d, t:%d, r:%d, b:%d\n",
		__func__, ts->plat_data->fod_data.rect_data[0], ts->plat_data->fod_data.rect_data[1],
		ts->plat_data->fod_data.rect_data[2], ts->plat_data->fod_data.rect_data[3]);

	offset = 0x4b;

	for (i = 0; i < 4; i++) {
		data[i * 2] = ts->plat_data->fod_data.rect_data[i] & 0xFF;
		data[i * 2 + 1] = (ts->plat_data->fod_data.rect_data[i] >> 8) & 0xFF;
	}

	ret = ts->synaptics_ts_write_sponge(ts, data, sizeof(data), offset, sizeof(data));
	if (ret < 0)
		input_err(true, ts->dev, "%s: Failed to write sponge\n", __func__);

	return ret;

}

int synaptics_ts_write_grip_data(struct synaptics_ts_data *ts, unsigned char *payload, int payload_size)
{
	int ret;
	unsigned char resp_code;
	struct synaptics_ts_buffer resp;

	input_dbg(true, ts->dev, "%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X %02X,%02X,%02X,%02X,%02X,%02X,%02X\n",
			payload[0], payload[1], payload[2], payload[3],
			payload[4], payload[5], payload[6], payload[7],
			payload[8], payload[9], payload[10], payload[11],
			payload[12], payload[13], payload[14], payload[15]);

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Device is not in application fw mode, mode: %x\n", __func__,
			ts->dev_mode);
		return -EINVAL;
	}

	synaptics_ts_buf_init(&resp);

	ret = synaptics_ts_send_command(ts,
		CMD_SET_GRIP,
		payload,
		payload_size,
		&resp_code,
		&resp,
		FORCE_ATTN_DRIVEN);
	if (ret < 0)
		input_err(true, ts->dev, "%s: Fail to write grip data, %d\n", __func__, ret);

	synaptics_ts_buf_release(&resp);

	return ret;
}

/*
 *	flag     1  :  set edge handler
 *		2  :  set (portrait, normal) edge zone data
 *		4  :  set (portrait, normal) dead zone data
 *		8  :  set landscape mode data
 *		16 :  mode clear
 *	data
 *		0xAA, FFF (y start), FFF (y end),  FF(direction)
 *		0xAB, FFFF (edge zone)
 *		0xAC, FF (up x), FF (down x), FFFF (up y), FF (bottom x), FFFF (down y)
 *		0xAD, FF (mode), FFF (edge), FFF (dead zone x), FF (dead zone top y), FF (dead zone bottom y)
 *	case
 *		edge handler set :  0xAA....
 *		booting time :  0xAA...  + 0xAB...
 *		normal mode : 0xAC...  (+0xAB...)
 *		landscape mode : 0xAD...
 *		landscape -> normal (if same with old data) : 0xAD, 0
 *		landscape -> normal (etc) : 0xAC....  + 0xAD, 0
 */

void synaptics_set_grip_data_to_ic(struct device *dev, u8 flag)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);
	unsigned char payload[16] = {0};
	int payload_size = 0;

	input_info(true, ts->dev, "%s: flag: 0x%02X (clr,lan,nor,edg,han)\n", __func__, flag);

	if (flag & G_SET_EDGE_HANDLER) {
		memset(payload, 0x00, 16);
		payload[0] = 0x0;
		payload[1] = 0x0;
		payload[2] = ts->plat_data->grip_data.edgehandler_direction  & 0xFF;
		payload[3] = (ts->plat_data->grip_data.edgehandler_direction >> 8) & 0xFF;
		payload[4] = ts->plat_data->grip_data.edgehandler_start_y  & 0xFF;
		payload[5] = (ts->plat_data->grip_data.edgehandler_start_y >> 8) & 0xFF;
		payload[6] = ts->plat_data->grip_data.edgehandler_end_y  & 0xFF;
		payload[7] = (ts->plat_data->grip_data.edgehandler_end_y >> 8) & 0xFF;
		payload_size = 8;
		if (synaptics_ts_write_grip_data(ts, payload, payload_size) < 0)
			return;
	}

	if (flag & G_SET_NORMAL_MODE) {
		memset(payload, 0x00, 16);
		payload[0] = 0x01;
		payload[1] = 0x0;
		payload[2] = ts->plat_data->grip_data.edge_range & 0xFF;
		payload[3] = (ts->plat_data->grip_data.edge_range >> 8) & 0xFF;
		payload[4] = ts->plat_data->grip_data.deadzone_up_x & 0xFF;
		payload[5] = (ts->plat_data->grip_data.deadzone_up_x >> 8) & 0xFF;
		payload[6] = ts->plat_data->grip_data.deadzone_dn_x & 0xFF;
		payload[7] = (ts->plat_data->grip_data.deadzone_dn_x >> 8) & 0xFF;
		payload[8] = ts->plat_data->grip_data.deadzone_y & 0xFF;
		payload[9] = (ts->plat_data->grip_data.deadzone_y >> 8) & 0xFF;
		if (GET_DEV_COUNT(ts->multi_dev) == MULTI_DEV_SUB) {
			/* temporary */
			payload_size = 10;
		} else {
			payload[10] = ts->plat_data->grip_data.deadzone_dn2_x & 0xFF;
			payload[11] = (ts->plat_data->grip_data.deadzone_dn2_x >> 8) & 0xFF;
			payload[12] = ts->plat_data->grip_data.deadzone_dn_y & 0xFF;
			payload[13] = (ts->plat_data->grip_data.deadzone_dn_y >> 8) & 0xFF;
			payload_size = 14;
		}
		if (synaptics_ts_write_grip_data(ts, payload, payload_size) < 0)
			return;
	}

	if (flag & G_SET_LANDSCAPE_MODE) {
		memset(payload, 0x00, 16);
		payload[0] = 0x02;
		payload[1] = 0x00;
		payload[2] = ts->plat_data->grip_data.landscape_mode & 0xFF;
		payload[3] = (ts->plat_data->grip_data.landscape_mode >> 8) & 0xFF;
		payload[4] = ts->plat_data->grip_data.landscape_edge & 0xFF;
		payload[5] = (ts->plat_data->grip_data.landscape_edge >> 8) & 0xFF;
		payload[6] = ts->plat_data->grip_data.landscape_deadzone & 0xFF;
		payload[7] = (ts->plat_data->grip_data.landscape_deadzone >> 8) & 0xFF;
		payload[8] = ts->plat_data->grip_data.landscape_top_deadzone & 0xFF;
		payload[9] = (ts->plat_data->grip_data.landscape_top_deadzone >> 8) & 0xFF;
		payload[10] = ts->plat_data->grip_data.landscape_bottom_deadzone & 0xFF;
		payload[11] = (ts->plat_data->grip_data.landscape_bottom_deadzone >> 8) & 0xFF;
		payload[12] = ts->plat_data->grip_data.landscape_top_gripzone & 0xFF;
		payload[13] = (ts->plat_data->grip_data.landscape_top_gripzone >> 8) & 0xFF;
		payload[14] = ts->plat_data->grip_data.landscape_bottom_gripzone & 0xFF;
		payload[15] = (ts->plat_data->grip_data.landscape_bottom_gripzone >> 8) & 0xFF;
		payload_size = 16;
		if (synaptics_ts_write_grip_data(ts, payload, payload_size) < 0)
			return;
	}

	if (flag & G_CLR_LANDSCAPE_MODE) {
		memset(payload, 0x00, 16);
		payload[0] = 0x02;
		payload[1] = 0x00;
		payload[2] = ts->plat_data->grip_data.landscape_mode & 0xFF;
		payload[3] = (ts->plat_data->grip_data.landscape_mode >> 8) & 0xFF;
		payload_size = 4;
		if (synaptics_ts_write_grip_data(ts, payload, payload_size) < 0)
			return;
	}
}

int synaptics_ts_ear_detect_enable(struct synaptics_ts_data *ts, u8 enable)
{
	int retval = 0;

	if (ts->dev_mode != SYNAPTICS_TS_MODE_APPLICATION_FIRMWARE) {
		input_err(true, ts->dev, "Application firmware not running, current mode: %02x\n",
			ts->dev_mode);
		return SEC_ERROR;
	}

	/* set up ear detection mode */
	retval = synaptics_ts_set_immediate_dynamic_config(ts,
			DC_DISABLE_PROXIMITY,
			enable);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to set %d with dynamic command 0x%x\n",
			DC_DISABLE_PROXIMITY, enable);
		return retval;
	}

	input_info(true, ts->dev, "%s: %s\n",
			__func__, ts->plat_data->ed_enable ? "on" : "off");

	return 0;
}

int synaptics_ts_pocket_mode_enable(struct synaptics_ts_data *ts, u8 enable)
{
	int ret;

	if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
		input_err(true, ts->dev, "%s: ic power is off\n", __func__);
		return 0;
	}

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_TSP_ENABLE_POCKET_MODE, enable);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Failed to send %d, ret:%d\n",
				__func__, enable, ret);
		return ret;
	}

	input_info(true, ts->dev, "%s: %s\n", __func__, enable ? "on" : "off");

	return 0;
}

int synaptics_ts_low_sensitivity_mode_enable(struct synaptics_ts_data *ts, u16 enable)
{
	int ret;

	if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
		input_err(true, ts->dev, "%s: ic power is off\n", __func__);
		return 0;
	}

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_TSP_ENABLE_LOW_SENSITIVITY_MODE, enable);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Failed to send %d, ret:%d\n",
				__func__, enable, ret);
		return ret;
	}

	input_info(true, ts->dev, "%s: 0x%02X\n", __func__, enable);

	return 0;
}

int synaptics_ts_set_charger_mode(struct device *dev, bool on)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);
	int ret;

	if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
		input_err(true, dev, "%s: ic power is off\n", __func__);
		return 0;
	}

	ret = synaptics_ts_set_immediate_dynamic_config(ts, DC_ENABLE_CHARGER_CONNECTED, on);
	if (ret < 0) {
		input_err(true, dev, "%s: Failed to send charger cmd: %d, ret:%d\n",
				__func__, on, ret);
		return ret;
	}

	input_info(true, dev, "%s: %s\n", __func__, on ? "on" : "off");

	return 0;
}

/**
 * syna_testing_pt1e()
 *
 * Sample code to perform BSC Calib (PT1E) testing
 *
 * @param
 *    [ in] tcm: the driver handle
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
int synaptics_ts_calibration(struct synaptics_ts_data *ts)
{
	int retval;
	bool result = false;
	struct synaptics_ts_buffer test_data;
	unsigned short *data_ptr = NULL;
	unsigned char parameter;
	unsigned char resp_code;
	unsigned char test_code;
	int debug_flag;

	synaptics_ts_buf_init(&test_data);

	input_info(true, ts->dev, "%s: Start testing\n", __func__);

	debug_flag = ts->debug_flag;
	ts->debug_flag |= (SEC_TS_DEBUG_PRINT_WRITE_CMD | SEC_TS_DEBUG_PRINT_READ_CMD);

	parameter = 3;
	retval = synaptics_ts_send_command(ts,
			CMD_BSC_DO_CALIBRATION,
			&parameter,
			1,
			&resp_code,
			NULL,
			FORCE_ATTN_DRIVEN);
	if ((retval < 0) || resp_code != STATUS_OK) {
		input_err(true, ts->dev, "%s: Fail to erase bsc data\n", __func__);
		result = false;
		goto exit;
	}

	parameter = 2;
	retval = synaptics_ts_send_command(ts,
			CMD_BSC_DO_CALIBRATION,
			&parameter,
			1,
			&resp_code,
			NULL,
			FORCE_ATTN_DRIVEN);
	if ((retval < 0) || resp_code != STATUS_OK) {
		input_err(true, ts->dev, "%s: Fail to calibrate bsc\n", __func__);
		result = false;
		goto exit;
	}

	test_code = (unsigned char)TEST_PID30_BSC_CALIB_TEST;

	retval = ts->write_message(ts,
			SYNAPTICS_TS_CMD_PRODUCTION_TEST,
			&test_code,
			1,
			&resp_code,
			FORCE_ATTN_DRIVEN);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to send command 0x%02x\n",
			SYNAPTICS_TS_CMD_PRODUCTION_TEST);
		result = false;
		goto exit;
	}

	retval = synaptics_ts_buf_copy(&test_data, &ts->resp_buf);
	if (retval < 0) {
		input_err(true, ts->dev, "Fail to copy testing data\n");
		result = false;
		goto exit;
	}

	data_ptr = (unsigned short *)&test_data.buf[0];
	if (*data_ptr == 1)
		result = true;

	ts->debug_flag = debug_flag;
exit:
	synaptics_ts_soft_reset(ts);

	input_err(true, ts->dev, "%s: Result = %s\n",  __func__, (result) ? "pass" : "fail");

	synaptics_ts_buf_release(&test_data);

	return ((result) ? 0 : -1);
}

#ifdef TCLM_CONCEPT
int synaptics_ts_tclm_execute_force_calibration(struct device *dev, int cal_mode)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);

	return synaptics_ts_calibration(ts);
}
#endif

int synaptics_ts_tclm_read(struct device *dev, int address)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);
	int ret = 0;
	u16 ic_version;
	unsigned char payload[2];
	unsigned char resp_code;
	struct synaptics_ts_buffer resp;

	synaptics_ts_buf_init(&resp);

	payload[0] = 0x00;
	payload[1] = SYNAPTICS_TS_NVM_CALIB_DATA_SIZE;
	ret = synaptics_ts_send_command(ts,
			CMD_ACCESS_CALIB_DATA_FROM_NVM,
			payload,
			sizeof(payload),
			&resp_code,
			&resp,
			0);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Fail to read calib data, status:%x\n", __func__,
			resp_code);
		return ret;
	}

	if (address == SEC_TCLM_NVM_OFFSET_IC_FIRMWARE_VER) {
		ret = synaptics_ts_get_app_info(ts, &ts->app_info);
		if (ret < 0)
			input_err(true, ts->dev, "%s: Fail to get application info\n", __func__);

		ic_version = (ts->plat_data->img_version_of_ic[2] << 8) | (ts->plat_data->img_version_of_ic[3] & 0xFF);
		input_info(true, ts->dev, "%s: version %04X\n", __func__, ic_version);
		synaptics_ts_buf_release(&resp);
		return ic_version;
	}
	memset(&ts->tdata->nvdata, 0x00, sizeof(struct sec_tclm_nvdata));
	if (resp.buf != NULL) {
		ts->fac_nv = resp.buf[0];
		ts->disassemble_count = resp.buf[1];
		memcpy(&ts->tdata->nvdata, &resp.buf[2], sizeof(struct sec_tclm_nvdata));
	} else {
		input_fail_hist(true, ts->dev, "%s: resp.buf is NULL\n", __func__);
	}

	synaptics_ts_buf_release(&resp);
	return ret;
}

int synaptics_ts_tclm_write(struct device *dev, int address)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(dev);
	int ret = 1;
	unsigned char payload_erase;
	unsigned char resp_code;
	unsigned char payload_write[SYNAPTICS_TS_NVM_CALIB_DATA_SIZE + 2];
	struct synaptics_ts_buffer resp;

	input_err(true, ts->dev, "%s: start\n", __func__);

	synaptics_ts_buf_init(&resp);
	payload_erase = 0x02;

	ret = synaptics_ts_send_command(ts,
			CMD_ACCESS_CALIB_DATA_FROM_NVM,
			&payload_erase,
			sizeof(payload_erase),
			&resp_code,
			&resp,
			100);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Fail to erase calib data, status:%x\n", __func__,
			resp_code);
	}

	memset(payload_write, 0x00, SYNAPTICS_TS_NVM_CALIB_DATA_SIZE + 2);

	payload_write[0] = 0x01;
	payload_write[1] = SYNAPTICS_TS_NVM_CALIB_DATA_SIZE;
	payload_write[2] = ts->fac_nv;
	payload_write[3] = ts->disassemble_count;

	memcpy(&payload_write[4], &ts->tdata->nvdata, sizeof(struct sec_tclm_nvdata));

	ret = synaptics_ts_send_command(ts,
			CMD_ACCESS_CALIB_DATA_FROM_NVM,
			payload_write,
			SEC_NVM_CALIB_DATA_SIZE + 2,
			&resp_code,
			&resp,
			0);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Fail to write calib data, status:%x\n", __func__,
			resp_code);
		synaptics_ts_buf_release(&resp);
		return ret;
	}

	synaptics_ts_buf_release(&resp);
	return ret;
}

MODULE_LICENSE("GPL");
