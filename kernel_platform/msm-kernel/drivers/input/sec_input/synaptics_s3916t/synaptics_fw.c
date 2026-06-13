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

#include <linux/input/spu-sign-verify.h>

enum {
	TSP_BUILT_IN = 0,
	TSP_SDCARD,
	TSP_SIGNED_SDCARD,
	TSP_SPU,
	TSP_VERIFICATION,
};

#define FW_UPDATE_RETRY_COUNT 3

#define FLASH_READ_DELAY_MS (10)
#define FLASH_WRITE_DELAY_MS (20)
#define FLASH_ERASE_DELAY_MS (500)

#define BOOT_CONFIG_SIZE 8
#define BOOT_CONFIG_SLOTS 16

#define DO_NONE (0)
#define DO_UPDATE (1)

/**
 * syna_tcm_identify()
 *
 * Implement the standard command code, which is used to request
 * an IDENTIFY report packet.
 *
 * @param
 *    [ in] tcm_dev: the device handle
 *    [out] id_info: the identification info packet returned
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
int synaptics_ts_identify(struct synaptics_ts_data *ts,
		struct synaptics_ts_identification_info *id_info)
{
	int retval = 0;
	unsigned char resp_code;

	if (!ts) {
		pr_err("%s%s: Invalid ts device handle\n", SECLOG, __func__);
		return -EINVAL;
	}

	retval = ts->write_message(ts,
			SYNAPTICS_TS_CMD_IDENTIFY,
			NULL,
			0,
			&resp_code,
			0);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to send command 0x%02x\n", __func__, SYNAPTICS_TS_CMD_IDENTIFY);
		goto exit;
	}

	ts->dev_mode = ts->id_info.mode;

	if (id_info == NULL)
		goto show_info;

	/* copy identify info to caller */
	retval = synaptics_ts_pal_mem_cpy((unsigned char *)id_info,
			sizeof(struct synaptics_ts_identification_info),
			ts->resp_buf.buf,
			ts->resp_buf.buf_size,
			MIN(sizeof(*id_info), ts->resp_buf.data_length));
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to copy identify info to caller\n", __func__);
		goto exit;
	}

show_info:
	input_err(true, ts->dev, "%s: TCM Fw mode: 0x%02x, TCM ver.: %d\n", __func__,
		ts->id_info.mode, ts->id_info.version);

exit:
	return retval;
}


/**
 * syna_tcm_set_up_flash_access()
 *
 * Enter the bootloader fw if not in the mode.
 * Besides, get the necessary parameters in boot info.
 *
 * @param
 *    [ in] tcm_dev:      the device handle
 *    [out] reflash_data: data blob for reflash
 *
 * @return
 *    Result of image file comparison
 */
static int synaptics_ts_set_up_flash_access(struct synaptics_ts_data *ts,
		struct synaptics_ts_reflash_data_blob *reflash_data)
{
	int retval;
	unsigned int temp;
	struct synaptics_ts_identification_info id_info;
	struct synaptics_ts_boot_info *boot_info;
	unsigned int wr_chunk;

	if (!reflash_data) {
		input_err(true, ts->dev, "%s: Invalid reflash data blob\n", __func__);
		return -EINVAL;
	}

	input_info(true, ts->dev, "%s:Set up flash access\n", __func__);

	retval = synaptics_ts_identify(ts, &id_info);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to do identification\n", __func__);
		return retval;
	}

	/* switch to bootloader mode */
	if (IS_APP_FW_MODE(id_info.mode)) {
		input_err(true, ts->dev, "%s: Prepare to enter bootloader mode\n", __func__);

		retval = synaptics_ts_switch_fw_mode(ts, SYNAPTICS_TS_MODE_BOOTLOADER);
		if (retval < 0) {
			input_err(true, ts->dev, "%s: Fail to enter bootloader mode\n", __func__);
			return retval;
		}
	}

	if (!IS_BOOTLOADER_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Fail to enter bootloader mode (current: 0x%x)\n", __func__,
			ts->dev_mode);
		return retval;
	}

	boot_info = &ts->boot_info;

	/* get boot info to set up the flash access */
	retval = synaptics_ts_get_boot_info(ts, boot_info);
	if (retval < 0) {
		input_err(true, ts->dev, "%s:Fail to get boot info at mode 0x%x\n", __func__,
			id_info.mode);
		return retval;
	}

	wr_chunk = ts->max_wr_size;

	temp = boot_info->write_block_size_words;
	reflash_data->write_block_size = temp * 2;

	input_err(true, ts->dev, "%s:Write block size: %d (words size: %d)\n", __func__,
		reflash_data->write_block_size, temp);

	temp = synaptics_ts_pal_le2_to_uint(boot_info->erase_page_size_words);
	reflash_data->page_size = temp * 2;

	input_err(true, ts->dev, "%s:Erase page size: %d (words size: %d)\n", __func__,
		reflash_data->page_size, temp);

	temp = synaptics_ts_pal_le2_to_uint(boot_info->max_write_payload_size);
	reflash_data->max_write_payload_size = temp;

	input_err(true, ts->dev, "%s:Max write flash data size: %d\n", __func__,
		reflash_data->max_write_payload_size);

	if (reflash_data->write_block_size > (wr_chunk - 9)) {
		input_err(true, ts->dev, "%s:Write block size, %d, greater than chunk space, %d\n", __func__,
			reflash_data->write_block_size, (wr_chunk - 9));
		return -EINVAL;
	}

	if (reflash_data->write_block_size == 0) {
		input_err(true, ts->dev, "%s:Invalid write block size %d\n", __func__,
			reflash_data->write_block_size);
		return -EINVAL;
	}

	if (reflash_data->page_size == 0) {
		input_err(true, ts->dev, "%s:Invalid erase page size %d\n", __func__,
			reflash_data->page_size);
		return -EINVAL;
	}

	return 0;
}

/**
 * syna_ts_run_bootloader_fw()
 *
 * Requests that the bootloader firmware be run.
 * Once the bootloader firmware has finished starting, an IDENTIFY report
 * to indicate that it is in the new mode.
 *
 * @param
 *    [ in] ts_dev: the device handle
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
static int synaptics_ts_run_bootloader_fw(struct synaptics_ts_data *ts)
{
	int retval = 0;
	unsigned char resp_code;

	retval = ts->write_message(ts,
			SYNAPTICS_TS_CMD_RUN_BOOTLOADER_FIRMWARE,
			NULL,
			0,
			&resp_code,
			FW_MODE_SWITCH_DELAY_MS);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to send command 0x%02x\n", __func__,
			SYNAPTICS_TS_CMD_RUN_BOOTLOADER_FIRMWARE);
		goto exit;
	}

	if (!IS_BOOTLOADER_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Fail to enter bootloader, mode: %x\n", __func__,
			ts->dev_mode);
		retval = -ENODEV;
		goto exit;
	}

	input_err(true, ts->dev, "%s: Bootloader Firmware (mode 0x%x) activated\n", __func__,
		ts->dev_mode);

	retval = 0;
exit:
	return retval;
}

/**
 * syna_ts_run_application_fw()
 *
 * Requests that the application firmware be run.
 * Once the application firmware has finished starting, an IDENTIFY report
 * to indicate that it is in the new mode.
 *
 * @param
 *    [ in] ts_dev: the device handle
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
static int synaptics_ts_run_application_fw(struct synaptics_ts_data *ts)
{
	int retval = 0;
	unsigned char resp_code;

	retval = ts->write_message(ts,
			SYNAPTICS_TS_CMD_RUN_APPLICATION_FIRMWARE,
			NULL,
			0,
			&resp_code,
			FW_MODE_SWITCH_DELAY_MS);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to send command 0x%02x\n", __func__,
			SYNAPTICS_TS_CMD_RUN_APPLICATION_FIRMWARE);
		goto exit;
	}

	if (IS_NOT_APP_FW_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Fail to enter application fw, mode: %x\n", __func__,
			ts->dev_mode);
		retval = -ENODEV;
		goto exit;
	}

	input_err(true, ts->dev, "%s: Application Firmware (mode 0x%x) activated\n", __func__,
		ts->dev_mode);

	retval = 0;

exit:
	return retval;
}

/**
 * syna_ts_switch_fw_mode()
 *
 * Implement the command code to switch the firmware mode.
 *
 * @param
 *    [ in] ts_dev: the device handle
 *    [ in] mode:    target firmware mode
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
int synaptics_ts_switch_fw_mode(struct synaptics_ts_data *ts,
		unsigned char mode)
{
	int retval = 0;

	switch (mode) {
	case SYNAPTICS_TS_MODE_APPLICATION_FIRMWARE:
		retval = synaptics_ts_run_application_fw(ts);
		if (retval < 0) {
			input_err(true, ts->dev, "%s: Fail to switch to application mode\n", __func__);
			goto exit;
		}
		break;
	case SYNAPTICS_TS_MODE_BOOTLOADER:
	case SYNAPTICS_TS_MODE_TDDI_BOOTLOADER:
	case SYNAPTICS_TS_MODE_TDDI_HDL_BOOTLOADER:
	case SYNAPTICS_TS_MODE_MULTICHIP_TDDI_BOOTLOADER:
		retval = synaptics_ts_run_bootloader_fw(ts);
		if (retval < 0) {
			input_err(true, ts->dev, "%s: Fail to switch to bootloader mode\n", __func__);
			goto exit;
		}
		break;
	case SYNAPTICS_TS_MODE_ROMBOOTLOADER:
		/*need to make nextversion */
		input_err(true, ts->dev, "%s:ROM_BOOTLOADER\n", __func__);
		break;
	default:
		input_err(true, ts->dev, "%s: Invalid firmware mode requested\n", __func__);
		retval = -EINVAL;
		goto exit;
	}

	retval = 0;

exit:
	return retval;
}


/**
 * syna_ts_compare_image_id_info()
 *
 * Compare the ID information between device and the image file,
 * and then determine the area to be updated.
 * The function should be called after parsing the image file.
 *
 * @param
 *    [ in] tcm_dev:       the device handle
 *    [ in] reflash_data:  data blob for reflash
 *
 * @return
 *    Blocks to be updated
 */
int synaptics_ts_compare_image_id_info(struct synaptics_ts_data *ts,
		struct synaptics_ts_reflash_data_blob *reflash_data, bool external_bin)
{
	enum update_area result;
	unsigned int idx;
	unsigned int image_fw_id;
	unsigned int device_fw_id;
	unsigned char *image_config_id;
	unsigned char *device_config_id;
	struct app_config_header *header;
	const unsigned char *app_config_data;
	struct block_data *app_config;

	result = UPDATE_NONE;

	if (!ts) {
		pr_err("%s%s:Invalid ts device handle\n", SECLOG, __func__);
		return -EINVAL;
	}

	if (!reflash_data) {
		input_err(true, ts->dev, "Invalid reflash_data\n");
		return -EINVAL;
	}

	app_config = &reflash_data->image_info.data[AREA_APP_CONFIG];

	if (app_config->size < sizeof(struct app_config_header)) {
		input_err(true, ts->dev, "%s:Invalid application config in image file\n", __func__);
		return -EINVAL;
	}

	app_config_data = app_config->data;
	header = (struct app_config_header *)app_config_data;

	image_fw_id = synaptics_ts_pal_le4_to_uint(header->build_id);
	device_fw_id = ts->packrat_number;

	input_info(true, ts->dev, "%s:Device firmware ID: %d, image build id: %d\n", __func__,
		device_fw_id, image_fw_id);

	image_config_id = header->customer_config_id;
	device_config_id = ts->app_info.customer_config_id;

	for (idx = 0; idx < 4; idx++) {
		if (!external_bin)
			ts->plat_data->img_version_of_bin[idx] = image_config_id[idx];
		ts->plat_data->img_version_of_ic[idx] = device_config_id[idx];
	}

	ts->plat_data->ic_vendor_name[0] = 'S';
	ts->plat_data->ic_vendor_name[1] = 'Y';

	if (sec_input_need_fw_update(ts->plat_data))
		result = UPDATE_FIRMWARE_AND_CONFIG;
	else
		result = UPDATE_NONE;

	switch (result) {
	case UPDATE_FIRMWARE_AND_CONFIG:
		input_err(true, ts->dev, "%s:Update firmware and config\n", __func__);
		break;
	case UPDATE_CONFIG:
		input_err(true, ts->dev, "%s:Update config only\n", __func__);
		break;
	case UPDATE_NONE:
	default:
		input_err(true, ts->dev, "%s:No need to do reflash\n", __func__);
		break;
	}

	return (int)result;
}


/**
 * syna_tcm_get_flash_area_string()
 *
 * Return the string ID of target area in the flash memory
 *
 * @param
 *    [ in] area: target flash area
 *
 * @return
 *    the string ID
 */
static inline char *synaptics_ts_get_flash_area_string(enum flash_area area)
{
	if (area < AREA_MAX)
		return (char *)flash_area_str[area];
	else
		return "";
}

/**
 * syna_tcm_save_flash_block_data()
 *
 * Save the block data of flash memory to the corresponding structure.
 *
 * @param
 *    [out] image_info: image info used for storing the block data
 *    [ in] area:       target area
 *    [ in] content:    content of data
 *    [ in] flash_addr: offset of block data
 *    [ in] size:       size of block data
 *    [ in] checksum:   checksum of block data
 *
 * @return
 *     on success, return 0; otherwise, negative value on error.
 */
static int synaptics_ts_save_flash_block_data(struct synaptics_ts_data *ts, struct image_info *image_info,
		enum flash_area area, const unsigned char *content,
		unsigned int offset, unsigned int size, unsigned int checksum)
{
	if (area >= AREA_MAX) {
		input_err(true, ts->dev, "%s: Invalid flash area\n", __func__);
		return -EINVAL;
	}

	if (checksum != CRC32((const char *)content, size)) {
		input_err(true, ts->dev, "%s:%s checksum error, in image: 0x%x (0x%x)\n", __func__,
			AREA_ID_STR(area), checksum,
			CRC32((const char *)content, size));
		return -EINVAL;
	}
	image_info->data[area].size = size;
	image_info->data[area].data = content;
	image_info->data[area].flash_addr = offset;
	image_info->data[area].id = (unsigned char)area;
	image_info->data[area].available = true;

	input_err(true, ts->dev, "%s:%s area - address:0x%08x (%d), size:%d\n", __func__,
		AREA_ID_STR(area), offset, offset, size);

	return 0;
}

/**
 * syna_tcm_get_flash_area_id()
 *
 * Return the corresponding ID of flash area based on the given string
 *
 * @param
 *    [ in] str: string to look for
 *
 *
 * @return
 *    if matching, return the corresponding ID; otherwise, return AREA_MAX.
 */
static enum flash_area synaptics_ts_get_flash_area_id(char *str)
{
	int area;
	char *target;
	unsigned int len;

	for (area = AREA_MAX - 1; area >= 0; area--) {
		target = AREA_ID_STR(area);
		len = strlen(target);

		if (strncmp(str, target, len) == 0)
			return area;
	}
	return AREA_MAX;
}


/**
 * syna_tcm_parse_fw_image()
 *
 * Parse and analyze the information of each areas from the given
 * firmware image.
 *
 * @param
 *    [ in] image:        image file given
 *    [ in] image_info:   data blob stored the parsed data from an image file
 *
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
static inline int synaptics_ts_parse_fw_image(struct synaptics_ts_data *ts, const unsigned char *image,
		struct image_info *image_info)
{
	int retval = 0;
	unsigned int idx;
	unsigned int addr;
	unsigned int offset;
	unsigned int length;
	unsigned int checksum;
	unsigned int flash_addr;
	unsigned int magic_value;
	unsigned int num_of_areas;
	struct image_header *header;
	struct area_descriptor *descriptor;
	const unsigned char *content;
	enum flash_area target_area;

	if (!image) {
		input_err(true, ts->dev,
				"%s: No image data\n", __func__);
		return -EINVAL;
	}

	if (!image_info) {
		input_err(true, ts->dev, "%s: Invalid image_info blob\n", __func__);
		return -EINVAL;
	}

	synaptics_ts_pal_mem_set(image_info, 0x00, sizeof(struct image_info));

	header = (struct image_header *)image;

	magic_value = synaptics_ts_pal_le4_to_uint(header->magic_value);
	if (magic_value != IMAGE_FILE_MAGIC_VALUE) {
		input_err(true, ts->dev, "%s: Invalid image file magic value\n", __func__);
		return -EINVAL;
	}

	offset = sizeof(struct image_header);
	num_of_areas = synaptics_ts_pal_le4_to_uint(header->num_of_areas);

	for (idx = 0; idx < num_of_areas; idx++) {
		addr = synaptics_ts_pal_le4_to_uint(image + offset);
		descriptor = (struct area_descriptor *)(image + addr);
		offset += 4;

		magic_value = synaptics_ts_pal_le4_to_uint(descriptor->magic_value);
		if (magic_value != FLASH_AREA_MAGIC_VALUE)
			continue;

		length = synaptics_ts_pal_le4_to_uint(descriptor->length);
		content = (unsigned char *)descriptor + sizeof(*descriptor);
		flash_addr = synaptics_ts_pal_le4_to_uint(descriptor->flash_addr_words);
		flash_addr = flash_addr * 2;
		checksum = synaptics_ts_pal_le4_to_uint(descriptor->checksum);

		target_area = synaptics_ts_get_flash_area_id(
			(char *)descriptor->id_string);

		retval = synaptics_ts_save_flash_block_data(ts, image_info,
				target_area,
				content,
				flash_addr,
				length,
				checksum);
		if (retval < 0)
			return -EINVAL;
	}

	return 0;
}

/**
 * syna_tcm_check_flash_boot_config()
 *
 * Check whether the same flash address of boot config in between the device
 * and the image file.
 *
 * @param
 *    [ in] boot_config:     block data of boot_config from image file
 *    [ in] boot_info:       data of boot info
 *    [ in] block_size:      max size of write block
 *
 * @return
 *    0, no need to do reflash; 1, able to do reflash; error otherwise.
 */

static int synaptics_ts_check_flash_boot_config(struct synaptics_ts_data *ts, struct block_data *boot_config,
		struct synaptics_ts_boot_info *boot_info, unsigned int block_size)
{
	unsigned int start_block;
	unsigned int image_addr;
	unsigned int device_addr;

	if (boot_config->size < BOOT_CONFIG_SIZE) {
		input_err(true, ts->dev,
				"%s:No valid BOOT_CONFIG size, %d, in image file\n", __func__, boot_config->size);
		return -EINVAL;
	}

	image_addr = boot_config->flash_addr;

	input_err(true, ts->dev, "%s:Boot Config address in image file: 0x%x\n", __func__, image_addr);

	start_block = VALUE(boot_info->boot_config_start_block);
	device_addr = start_block * block_size;

	input_err(true, ts->dev, "%s:Boot Config address in device: 0x%x\n", __func__, device_addr);

	return DO_NONE;
}

/**
 * syna_tcm_check_flash_app_config()
 *
 * Check whether the same flash address of app config in between the
 * device and the image file.
 *
 * @param
 *    [ in] app_config:      block data of app_config from image file
 *    [ in] app_info:        data of application info
 *    [ in] block_size:      max size of write block
 *
 * @return
 *    0, no need to do reflash; 1, able to do reflash; error otherwise.
 */
static int synaptics_ts_check_flash_app_config(struct synaptics_ts_data *ts, struct block_data *app_config,
		struct synaptics_ts_application_info *app_info, unsigned int block_size)
{
	unsigned int temp;
	unsigned int image_addr;
	unsigned int image_size;
	unsigned int device_addr;
	unsigned int device_size;

	if (app_config->size == 0) {
		input_err(true, ts->dev, "%s: No APP_CONFIG in image file\n", __func__);
		return DO_NONE;
	}

	image_addr = app_config->flash_addr;
	image_size = app_config->size;

	input_err(true, ts->dev, "%s: App Config address in image file: 0x%x, size: %d\n", __func__, image_addr, image_size);

	temp = VALUE(app_info->app_config_start_write_block);
	device_addr = temp * block_size;
	device_size = VALUE(app_info->app_config_size);

	input_err(true, ts->dev, "%s: App Config address in device: 0x%x, size: %d\n", __func__,
		device_addr, device_size);

	if (device_addr == 0 && device_size == 0)
		return DO_UPDATE;

	if (image_addr != device_addr)
		input_err(true, ts->dev, "%s: App Config address mismatch, image:0x%x, dev:0x%x\n", __func__,
			image_addr, device_addr);

	if (image_size != device_size)
		input_err(true, ts->dev, "%s: App Config address size mismatch, image:%d, dev:%d\n", __func__,
			image_size, device_size);

	return DO_UPDATE;
}

/**
 * syna_tcm_check_flash_disp_config()
 *
 * Check whether the same flash address of display config in between the
 * device and the image file.
 *
 * @param
 *    [ in] disp_config:     block data of disp_config from image file
 *    [ in] boot_info:       data of boot info
 *    [ in] block_size:      max size of write block
 *
 * @return
 *    0, no need to do reflash; 1, able to do reflash; error otherwise.
 */
static int synaptics_ts_check_flash_disp_config(struct synaptics_ts_data *ts, struct block_data *disp_config,
		struct synaptics_ts_boot_info *boot_info, unsigned int block_size)
{
	unsigned int temp;
	unsigned int image_addr;
	unsigned int image_size;
	unsigned int device_addr;
	unsigned int device_size;

	/* disp_config area may not be included in all product */
	if (disp_config->size == 0) {
		input_err(true, ts->dev, "%s: No DISP_CONFIG in image file\n", __func__);
		return DO_NONE;
	}

	image_addr = disp_config->flash_addr;
	image_size = disp_config->size;

	input_err(true, ts->dev, "%s: Disp Config address in image file: 0x%x, size: %d\n", __func__,
		image_addr, image_size);

	temp = VALUE(boot_info->display_config_start_block);
	device_addr = temp * block_size;

	temp = VALUE(boot_info->display_config_length_blocks);
	device_size = temp * block_size;

	input_err(true, ts->dev, "%s: Disp Config address in device: 0x%x, size: %d\n", __func__,
		device_addr, device_size);

	if (image_addr != device_addr)
		input_err(true, ts->dev, "%s: Disp Config address mismatch, image:0x%x, dev:0x%x\n", __func__,
			image_addr, device_addr);

	if (image_size != device_size)
		input_err(true, ts->dev, "%s: Disp Config address size mismatch, image:%d, dev:%d\n", __func__,
			image_size, device_size);

	return DO_UPDATE;
}

/**
 * syna_tcm_check_flash_app_code()
 *
 * Check whether the valid size of app firmware in the image file
 *
 * @param
 *    [ in] app_code:      block data of app_code from image file
 *
 * @return
 *    0, no need to do reflash; 1, able to do reflash; error otherwise.
 */
static int synaptics_ts_check_flash_app_code(struct synaptics_ts_data *ts, struct block_data *app_code)
{

	if (app_code->size == 0) {
		input_err(true, ts->dev, "%s: No %s in image file\n", __func__, AREA_ID_STR(app_code->id));
		return -EINVAL;
	}

	return DO_UPDATE;
}

/**
 * syna_tcm_check_flash_openshort()
 *
 * Check whether the valid size of openshort area in the image file
 *
 * @param
 *    [ in] open_short:      block data of open_short from image file
 *
 * @return
 *    0, no need to do reflash; 1, able to do reflash; error otherwise.
 */
static int synaptics_ts_check_flash_openshort(struct synaptics_ts_data *ts, struct block_data *open_short)
{
	/* open_short area may not be included in all product */
	if (open_short->size == 0) {
		input_err(true, ts->dev, "%s: No %s in image file\n", __func__, AREA_ID_STR(open_short->id));
		return DO_NONE;
	}

	return DO_UPDATE;
}

/**
 * syna_tcm_check_flash_app_prod_test()
 *
 * Check whether the valid size of app prod_test area in the image file
 *
 * @param
 *    [ in] prod_test:  block data of app_prod_test from image file
 *
 * @return
 *    0, no need to do reflash; 1, able to do reflash; error otherwise.
 */
static int synaptics_ts_check_flash_app_prod_test(struct synaptics_ts_data *ts, struct block_data *prod_test)
{
	/* app_prod_test area may not be included in all product */
	if (prod_test->size == 0) {
		input_err(true, ts->dev, "%s: No %s in image file\n", __func__, AREA_ID_STR(prod_test->id));
		return DO_NONE;
	}

	return DO_UPDATE;
}

/**
 * syna_tcm_check_flash_ppdt()
 *
 * Check whether the valid size of ppdt area in the image file
 *
 * @param
 *    [ in] ppdt:      block data of PPDT from image file
 *
 * @return
 *    0, no need to do reflash; 1, able to do reflash; error otherwise.
 */
static int synaptics_ts_check_flash_ppdt(struct synaptics_ts_data *ts, struct block_data *ppdt)
{
	/* open_short area may not be included in all product */
	if (ppdt->size == 0) {
		input_err(true, ts->dev, "%s: No %s in image file\n", __func__, AREA_ID_STR(ppdt->id));
		return DO_NONE;
	}

	return DO_UPDATE;
}

/**
 * syna_tcm_reflash_send_command()
 *
 * Helper to wrap up the write_message() function.
 *
 * @param
 *    [ in] tcm_dev:        the device handle
 *    [ in] command:        given command code
 *    [ in] payload:        payload data, if any
 *    [ in] payload_len:    length of payload data
 *    [ in] delay_ms_resp:  delay time to get the response of command
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
static int synaptics_ts_reflash_send_command(struct synaptics_ts_data *ts,
		unsigned char command, unsigned char *payload,
		unsigned int payload_len, unsigned int delay_ms_resp)
{
	int retval = 0;
	unsigned char resp_code;

	if (!ts) {
		pr_err("%s%s: Invalid ts device handle\n", SECLOG, __func__);
		return -EINVAL;
	}

	if (!IS_BOOTLOADER_MODE(ts->dev_mode)) {
		input_err(true, ts->dev, "%s: Device is not in BL mode, 0x%x\n", __func__, ts->dev_mode);
		retval = -EINVAL;
	}

	retval = ts->write_message(ts,
			command,
			payload,
			payload_len,
			&resp_code,
			delay_ms_resp);
	if (retval < 0)
		input_err(true, ts->dev, "%s: Fail to send command 0x%02x\n", __func__, command);

	return retval;
}

/**
 * syna_tcm_write_flash()
 *
 * Implement the bootloader command to write specified data to flash memory.
 *
 * If the length of the data to write is not an integer multiple of words,
 * the trailing byte will be discarded.  If the length of the data to write
 * is not an integer number of write blocks, it will be zero-padded to the
 * next write block.
 *
 * @param
 *    [ in] tcm_dev:      the device handle
 *    [ in] reflash_data: data blob for reflash
 *    [ in] address:      the address in flash memory to write
 *    [ in] wr_data:      data to write
 *    [ in] wr_len:       length of data to write
 *    [ in] wr_delay_ms:  a short delay after the command executed
 *                        set '0' to use default time, which is 20 ms;
 *                        set 'FORCE_ATTN_DRIVEN' to adopt ATTN-driven.
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
static int synaptics_ts_write_flash(struct synaptics_ts_data *ts,
		struct synaptics_ts_reflash_data_blob *reflash_data,
		unsigned int address, const unsigned char *wr_data,
		unsigned int wr_len, unsigned int wr_delay_ms)
{
	int retval;
	unsigned int offset;
	unsigned int w_length;
	unsigned int xfer_length;
	unsigned int remaining_length;
	unsigned int flash_address;
	unsigned int block_address;
	unsigned int num_blocks;
	unsigned int resp_delay;

	if (!ts) {
		pr_err("%s%s: Invalid ts device handle\n", SECLOG, __func__);
		return -EINVAL;
	}

	/* ensure that the length to write is the multiple of max_write_payload_size */
	w_length = reflash_data->max_write_payload_size;

	w_length = w_length - (w_length % reflash_data->write_block_size);

	offset = 0;

	remaining_length = wr_len;

	synaptics_ts_buf_lock(&reflash_data->out);

	while (remaining_length) {
		if (remaining_length > w_length)
			xfer_length = w_length;
		else
			xfer_length = remaining_length;

		retval = synaptics_ts_buf_alloc(&reflash_data->out,
				xfer_length + 2);
		if (retval < 0) {
			input_err(true, ts->dev, "%s: Fail to allocate memory for buf.out\n", __func__);
			synaptics_ts_buf_unlock(&reflash_data->out);
			return retval;
		}

		flash_address = address + offset;
		block_address = flash_address / reflash_data->write_block_size;
		reflash_data->out.buf[0] = (unsigned char)block_address;
		reflash_data->out.buf[1] = (unsigned char)(block_address >> 8);

		num_blocks = synaptics_ts_pal_ceil_div(xfer_length, reflash_data->write_block_size);

		resp_delay = synaptics_ts_fw_resp_delay_ms(ts, xfer_length, wr_delay_ms, num_blocks);

		retval = synaptics_ts_memcpy(&reflash_data->out.buf[2],
				reflash_data->out.buf_size - 2,
				&wr_data[offset],
				wr_len - offset,
				xfer_length);
		if (retval < 0) {
			input_err(true, ts->dev, "%s: Fail to copy write data ,size: %d\n", __func__,
				xfer_length);
			synaptics_ts_buf_unlock(&reflash_data->out);
			return retval;
		}

		retval = synaptics_ts_reflash_send_command(ts,
				SYNAPTICS_TS_CMD_WRITE_FLASH,
				reflash_data->out.buf,
				xfer_length + 2,
				resp_delay);
		if (retval < 0) {
			input_err(true, ts->dev, "%s: Fail to write data to flash addr 0x%x, size %d\n", __func__,
				flash_address, xfer_length + 2);
			synaptics_ts_buf_unlock(&reflash_data->out);
			return retval;
		}

		offset += xfer_length;
		remaining_length -= xfer_length;
	}

	synaptics_ts_buf_unlock(&reflash_data->out);
	return 0;
}

/**
 * syna_tcm_write_flash_block()
 *
 * Write data to the target block data area in the flash memory.
 *
 * @param
 *    [ in] tcm_dev:       the device handle
 *    [ in] reflash_data:  data blob for reflash
 *    [ in] area:          target block area to write
 *    [ in] resp_reading:  method to read in the response
 *                          a positive value presents the us time delay for the processing
 *                          of each BLOCKs in the flash to write;
 *                          or, set '0' or 'RESP_IN_ATTN' for ATTN driven
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
static int synaptics_ts_write_flash_block(struct synaptics_ts_data *ts,
		struct synaptics_ts_reflash_data_blob *reflash_data,
		struct block_data *block, unsigned int resp_reading)
{
	int retval;
	unsigned int size;
	unsigned int flash_addr;
	const unsigned char *data;

	if (!ts) {
		pr_err("%s%s: Invalid ts device handle\n", SECLOG, __func__);
		return -EINVAL;
	}

	if (!reflash_data) {
		input_err(true, ts->dev, "%s: Invalid reflash data blob\n", __func__);
		return -EINVAL;
	}

	if (!block) {
		input_err(true, ts->dev, "%s: Invalid block data\n", __func__);
		return -EINVAL;
	}

	data = block->data;
	size = block->size;
	flash_addr = block->flash_addr;

	input_err(true, ts->dev, "%s: Write data to %s - address: 0x%x, size: %d\n", __func__,
		AREA_ID_STR(block->id), flash_addr, size);

	if (size == 0) {
		input_err(true, ts->dev, "%s: No need to update, size = %d\n", __func__, size);
		goto exit;
	}

	retval = synaptics_ts_write_flash(ts, reflash_data,
			flash_addr, data, size, resp_reading);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to write %s to flash (addr: 0x%x, size: %d)\n", __func__,
			AREA_ID_STR(block->id), flash_addr, size);
		return retval;
	}

exit:
	input_err(true, ts->dev, "%s: %s area written\n", __func__, AREA_ID_STR(block->id));

	return 0;
}

/**
 * syna_tcm_erase_flash()
 *
 * Implement the bootloader command, which is used to erase the specified
 * blocks of flash memory.
 *
 * Until this command completes, the device may be unresponsive.
 * Therefore, this helper is implemented as a blocked function, and the delay
 * time is set to DEFAULT_FLASH_ERASE_DELAY_MS in default.
 *
 * @param
 *    [ in] tcm_dev:        the device handle
 *    [ in] reflash_data:   data blob for reflash
 *    [ in] address:        the address in flash memory to read
 *    [ in] size:           size of data to write
 *    [ in] erase_delay_ms: a short delay after the command executed
 *                          set a positive value or 'DEFAULT_FLASH_ERASE_DELAY' to use default;
 *                          set '0' or 'RESP_IN_ATTN' to select ATTN-driven.
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
static int synaptics_ts_erase_flash(struct synaptics_ts_data *ts,
		struct synaptics_ts_reflash_data_blob *reflash_data,
		unsigned int address, unsigned int size,
		unsigned int erase_delay_ms)
{
	int retval;
	unsigned int page_start = 0;
	unsigned int page_count = 0;
	unsigned char out_buf[4] = {0};
	int size_erase_cmd;
	unsigned int resp_delay;

	page_start = address / reflash_data->page_size;

	page_count = synaptics_ts_fw_set_page_count(ts, reflash_data, size);

	resp_delay = synaptics_ts_fw_set_erase_delay(ts, erase_delay_ms, page_count);

	if ((page_start > 0xff) || (page_count > 0xff)) {
		size_erase_cmd = 4;

		out_buf[0] = (unsigned char)(page_start & 0xff);
		out_buf[1] = (unsigned char)((page_start >> 8) & 0xff);
		out_buf[2] = (unsigned char)(page_count & 0xff);
		out_buf[3] = (unsigned char)((page_count >> 8) & 0xff);
	} else {
		size_erase_cmd = 2;

		out_buf[0] = (unsigned char)(page_start & 0xff);
		out_buf[1] = (unsigned char)(page_count & 0xff);
	}

	retval = synaptics_ts_reflash_send_command(ts,
			SYNAPTICS_TS_CMD_ERASE_FLASH,
			out_buf,
			size_erase_cmd,
			resp_delay);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to erase data at flash page 0x%x, count %d\n", __func__,
			page_start, page_count);
		return retval;
	}

	return 0;
}

/**
 * syna_tcm_erase_flash_block()
 *
 * Mass erase the target block data area in the flash memory.
 *
 * @param
 *    [ in] tcm_dev:      the device handle
 *    [ in] reflash_data: data blob for reflash
 *    [ in] block:        target block area to erase
 *    [ in] resp_reading:  method to read in the response
 *                          a positive value presents the us time delay for the processing
 *                          of each PAGEs in the flash to erase;
 *                          or, set '0' or 'RESP_IN_ATTN' for ATTN driven
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
static int synaptics_ts_erase_flash_block(struct synaptics_ts_data *ts,
		struct synaptics_ts_reflash_data_blob *reflash_data,
		struct block_data *block, unsigned int resp_reading)
{
	int retval;
	unsigned int size;
	unsigned int flash_addr;

	if (!ts) {
		pr_err("%s%s: Invalid ts device handle\n", SECLOG, __func__);
		return -EINVAL;
	}

	if (!reflash_data) {
		input_err(true, ts->dev, "%s: Invalid reflash data blob\n", __func__);
		return -EINVAL;
	}

	if (!block) {
		input_err(true, ts->dev, "%s: Invalid block data\n", __func__);
		return -EINVAL;
	}

	flash_addr = block->flash_addr;

	size = block->size;

	input_info(true, ts->dev, "%s: Erase %s block - address: 0x%x, size: %d\n", __func__,
		AREA_ID_STR(block->id), flash_addr, size);

	if (size == 0) {
		input_info(true, ts->dev, "%s: No need to erase, size = %d\n", __func__, size);
		goto exit;
	}

	retval = synaptics_ts_erase_flash(ts, reflash_data,
			flash_addr, size, resp_reading);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to erase %s data (addr: 0x%x, size: %d)\n", __func__,
			AREA_ID_STR(block->id), flash_addr, size);
		return retval;
	}

exit:
	input_info(true, ts->dev, "%s: %s area erased\n", __func__, AREA_ID_STR(block->id));

	return 0;
}

/**
 * syna_tcm_check_flash_block()
 *
 * Dispatch to the proper helper to ensure the data of associated block area
 * is correct in between the device and the image file.
 *
 * @param
 *    [ in] tcm_dev:      the device handle
 *    [ in] reflash_data: data blob for reflash
 *    [ in] block:        target block area to check
 *
 * @return
 *    0, no need to do reflash; 1, able to do reflash; error otherwise.
 */
static int synaptics_ts_check_flash_block(struct synaptics_ts_data *ts,
		struct synaptics_ts_reflash_data_blob *reflash_data,
		struct block_data *block)
{
	int retval = 0;
	struct synaptics_ts_application_info *app_info;
	struct synaptics_ts_boot_info *boot_info;

	if (!ts) {
		pr_err("%s%s: Invalid ts device handle\n", SECLOG, __func__);
		return DO_NONE;
	}

	if (!reflash_data) {
		input_err(true, ts->dev, "%s: Invalid reflash data blob\n", __func__);
		return DO_NONE;
	}

	if (!block) {
		input_err(true, ts->dev, "%s: Invalid block data\n", __func__);
		return DO_NONE;
	}

	app_info = &ts->app_info;
	boot_info = &ts->boot_info;

	switch (block->id) {
	case AREA_APP_CODE:
		retval = synaptics_ts_check_flash_app_code(ts, block);
		break;
	case AREA_APP_CONFIG:
		retval = synaptics_ts_check_flash_app_config(ts, block, app_info,
				reflash_data->write_block_size);
		break;
	case AREA_BOOT_CONFIG:
		retval = synaptics_ts_check_flash_boot_config(ts, block, boot_info,
				reflash_data->write_block_size);
		break;
	case AREA_DISP_CONFIG:
		retval = synaptics_ts_check_flash_disp_config(ts, block, boot_info,
				reflash_data->write_block_size);
		break;
	case AREA_OPEN_SHORT_TUNING:
		retval = synaptics_ts_check_flash_openshort(ts, block);
		break;
	case AREA_PROD_TEST:
		retval = synaptics_ts_check_flash_app_prod_test(ts, block);
		break;
	case AREA_PPDT:
		retval = synaptics_ts_check_flash_ppdt(ts, block);
		break;
	default:
		retval = DO_NONE;
		break;
	}

	return retval;
}

/**
 * syna_tcm_update_flash_block()
 *
 * Perform the reflash sequence to the target area
 *
 * @param
 *    [ in] tcm_dev:       the device handle
 *    [ in] reflash_data:  data blob for reflash
 *    [ in] block:         target block area to update
 *    [ in] delay_setting: set up the us delay time to wait for the completion of flash access
 *                            for polling,     set a value formatted with [erase us | write us];
 *                            for ATTN-driven, use '0' or 'RESP_IN_ATTN'
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
static int synaptics_ts_update_flash_block(struct synaptics_ts_data *ts,
		struct synaptics_ts_reflash_data_blob *reflash_data,
		struct block_data *block, unsigned int delay_setting)
{
	int retval;
	unsigned int erase_delay_ms = (delay_setting >> 16) & 0xFFFF;
	unsigned int wr_blk_delay_ms = delay_setting & 0xFFFF;

	if (!ts) {
		pr_err("%s%s: Invalid ts device handle\n", SECLOG, __func__);
		return -EINVAL;
	}

	if (!reflash_data) {
		input_err(true, ts->dev, "%s: Invalid reflash data blob\n", __func__);
		return -EINVAL;
	}

	if (!block) {
		input_err(true, ts->dev, "%s: Invalid block data\n", __func__);
		return -EINVAL;
	}

	/* reflash is not needed for the partition */
	retval = synaptics_ts_check_flash_block(ts,
			reflash_data,
			block);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Invalid %s area\n", __func__, AREA_ID_STR(block->id));
		return retval;
	}

	if (retval == DO_NONE)
		return 0;

	input_err(true, ts->dev, "%s: Prepare to erase %s area\n", __func__, AREA_ID_STR(block->id));

	retval = synaptics_ts_erase_flash_block(ts,
			reflash_data,
			block,
			erase_delay_ms);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to erase %s area\n", __func__, AREA_ID_STR(block->id));
		return retval;
	}

	input_err(true, ts->dev, "%s: Prepare to update %s area\n", __func__, AREA_ID_STR(block->id));

	retval = synaptics_ts_write_flash_block(ts,
			reflash_data,
			block,
			wr_blk_delay_ms);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to write %s area\n", __func__, AREA_ID_STR(block->id));
		return retval;
	}

	return 0;
}

/**
 * syna_tcm_do_reflash_generic()
 *
 * Implement the generic sequence of fw update in MODE_BOOTLOADER.
 *
 * Typically, it is applied on most of discrete touch controllers
 *
 * @param
 *    [ in] tcm_dev:         the device handle
 *    [ in] reflash_data:    misc. data used for fw update
 *    [ in] type:            the area to update
 *    [ in] delay_setting:   set up the us delay time to wait for the completion of flash access
 *                               for polling,     set a value formatted with [erase ms | write us];
 *                               for ATTN-driven, use '0' or 'RESP_IN_ATTN'
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
static int synaptics_ts_do_reflash_generic(struct synaptics_ts_data *ts,
		struct synaptics_ts_reflash_data_blob *reflash_data,
		enum update_area type, unsigned int delay_setting)
{
	int retval = 0;
	struct block_data *block;

	if (!ts) {
		pr_err("%s%s: Invalid ts device handle\n", SECLOG, __func__);
		return -EINVAL;
	}

	if (!reflash_data) {
		input_err(true, ts->dev, "%s: Invalid reflash_data blob\n", __func__);
		return -EINVAL;
	}

	if (ts->dev_mode != SYNAPTICS_TS_MODE_BOOTLOADER) {
		input_err(true, ts->dev, "%s: Incorrect bootloader mode, 0x%02x, expected: 0x%02x\n", __func__,
			ts->dev_mode, SYNAPTICS_TS_MODE_BOOTLOADER);
		return -EINVAL;
	}

	if (type != UPDATE_FIRMWARE_AND_CONFIG) {
		input_err(true, ts->dev, "%s: type is not UPDATE_FIRMWARE_AND_CONFIG\n", __func__);
		return -EINVAL;
	}

	block = &reflash_data->image_info.data[AREA_APP_CODE];

	retval = synaptics_ts_update_flash_block(ts,
			reflash_data,
			block,
			delay_setting);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to update application firmware\n", __func__);
		goto exit;
	}
	block = &reflash_data->image_info.data[AREA_APP_CONFIG];
	retval = synaptics_ts_update_flash_block(ts,
			reflash_data,
			block,
			delay_setting);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to update application config\n", __func__);
		goto exit;
	}

exit:
	return retval;
}

/**
 * syna_tcm_do_fw_update()
 *
 * The entry function to perform fw update upon TouchBoot.
 *
 * @param
 *    [ in] tcm_dev:         the device handle
 *    [ in] image:           binary data to write
 *    [ in] image_size:      size of data array
 *    [ in] delay_setting: set up the us delay time to wait for the completion of flash access
 *                            for polling,     set a value formatted with [erase ms | write us];
 *                            for ATTN-driven, use '0' or 'RESP_IN_ATTN'
 *    [ in] force_reflash:   '1' to do reflash anyway
 *                           '0' to compare ID info before doing reflash.
 *
 * @return
 *    on success, 0 or positive value; otherwise, negative value on error.
 */
int synaptics_ts_do_fw_update(struct synaptics_ts_data *ts,
		const unsigned char *image, unsigned int image_size,
		unsigned int delay_setting, bool force_reflash, bool external_bin)
{
	int retval = 0, retry = FW_UPDATE_RETRY_COUNT;
	unsigned int app_status;
	enum update_area type = UPDATE_NONE;
	struct synaptics_ts_reflash_data_blob reflash_data;

	if (!ts) {
		pr_err("%s%s: Invalid ts device handle\n", SECLOG, __func__);
		return -EINVAL;
	}

	if ((!image) || (image_size == 0)) {
		if (!image)
			input_err(true, ts->dev, "%s: image null\n", __func__);
		if (image_size == 0)
			input_err(true, ts->dev, "%s: image_size 0\n", __func__);
		input_err(true, ts->dev, "%s: Invalid image data\n", __func__);
		return -EINVAL;
	}

	input_err(true, ts->dev, "%s: Prepare to do reflash\n", __func__);

	synaptics_ts_buf_init(&reflash_data.out);

	reflash_data.image = image;
	reflash_data.image_size = image_size;
	synaptics_ts_pal_mem_set(&reflash_data.image_info, 0x00,
		sizeof(struct image_info));

	retval = synaptics_ts_parse_fw_image(ts, image, &reflash_data.image_info);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to parse firmware image\n", __func__);
		return retval;
	}


	input_err(true, ts->dev, "%s: Start of reflash\n", __func__);

	ATOMIC_SET(ts->firmware_flashing, 1);

	app_status = synaptics_ts_pal_le2_to_uint(ts->app_info.status);

	type = (enum update_area)synaptics_ts_compare_image_id_info(ts, &reflash_data, external_bin);

	/* to forcedly update the firmware and config
	 *   - flag of 'force_reflash' has been set
	 *   - device stays in bootloader
	 *   - app firmware doesn't run properly
	 */
	if (IS_BOOTLOADER_MODE(ts->dev_mode))
		force_reflash = true;
	if (IS_APP_FW_MODE(ts->dev_mode) && (app_status != APP_STATUS_OK)) {
		input_err(true, ts->dev, "%s: Bad application firmware, dev_mode: 0x%x, status: 0x%x\n",
		__func__, ts->dev_mode, app_status);
		force_reflash = true;
	}

	if (force_reflash) {
		type = UPDATE_FIRMWARE_AND_CONFIG;
		goto reflash;
	}

	ts->firmware_update_done = type;

	if (type == UPDATE_NONE)
		goto exit;

reflash:
	do {
		if (retval < 0) {
			ts->plat_data->hw_param.checksum_result |= SYNAPTICS_TS_CHK_UPDATE_FAIL;
			input_fail_hist(true, ts->dev, "%s: fw update is failed. retry:%d\n",
					__func__, FW_UPDATE_RETRY_COUNT - retry);
			synaptics_ts_hw_reset(ts);
			retval = synaptics_ts_setup(ts);
			if (retval < 0) {
				input_err(true, ts->dev, "%s: fail to do setup, retry=%d\n",
						__func__, FW_UPDATE_RETRY_COUNT - retry);
				continue;
			}
		}
		synaptics_ts_buf_init(&reflash_data.out);

	/* set up flash access, and enter the bootloader mode */
		retval = synaptics_ts_set_up_flash_access(ts, &reflash_data);
		if (retval < 0) {
			input_err(true, ts->dev,
					"%s: fail to set up flash access, %d\n", __func__, retval);
		} else {
			/* perform the fw update */
			if (ts->dev_mode == SYNAPTICS_TS_MODE_BOOTLOADER) {
				retval = synaptics_ts_do_reflash_generic(ts,
					&reflash_data,
					type,
					delay_setting);
				if (retval < 0) {
					input_err(true, ts->dev, "%s: fail to do firmware update, retry=%d\n",
							__func__, FW_UPDATE_RETRY_COUNT - retry);
				} else {
					input_info(true, ts->dev, "%s: succeed to do firmware update\n", __func__);
					if (synaptics_ts_soft_reset(ts) < 0)
						input_err(true, ts->dev, "Fail to do reset\n");
				}
			} else {
				retval = -EINVAL;
				input_err(true, ts->dev, "%s: Incorrect bootloader mode, 0x%02x\n", __func__,
					ts->dev_mode);
			}
		}
	} while ((retval < 0) && (--retry > 0));
exit:
	ATOMIC_SET(ts->firmware_flashing, 0);

	synaptics_ts_buf_release(&reflash_data.out);

	return retval;
}

int synaptics_ts_fw_update_on_probe(struct synaptics_ts_data *ts)
{
	int retval = 0;
	const struct firmware *fw_entry = NULL;
	char fw_path[SYNAPTICS_TS_MAX_FW_PATH];
#ifdef TCLM_CONCEPT
	int restore_cal = 0;
#endif
	unsigned int delay_setting = 0;
	int retry = 3;

	input_info(true, ts->dev, "%s:\n", __func__);

	synaptics_ts_fw_delay_setting(ts);

	if (ts->plat_data->bringup == BRINGUP_SKIP_FW_UPDATE) {
		input_info(true, ts->dev, "%s: bringup 1\n", __func__);
		goto exit_fwload;
	}

	if (!ts->plat_data->firmware_name) {
		input_err(true, ts->dev, "%s: firmware name does not declair in dts\n", __func__);
		retval = -ENOENT;
		goto exit_fwload;
	}

	snprintf(fw_path, SYNAPTICS_TS_MAX_FW_PATH, "%s", ts->plat_data->firmware_name);
	input_info(true, ts->dev, "%s: Load firmware : %s\n", __func__, fw_path);

	delay_setting = (ts->fw_erase_delay << 16) | ts->fw_write_block_delay;

	if (delay_setting == 0)
		delay_setting = FORCE_ATTN_DRIVEN;

	if (delay_setting != FORCE_ATTN_DRIVEN)
		sec_input_irq_disable(ts->plat_data);

	while (retry--) {
		retval = request_firmware(&fw_entry, fw_path, ts->dev);
		if (retval)
			input_err(true, ts->dev,
					"%s: Firmware image %s not available retry %d\n", __func__,
					fw_path, retry);
		else
			break;
		sec_delay(1000);
	}

	if (retval) {
		retval = 0;
		sec_input_irq_enable(ts->plat_data);
		goto exit_fwload;
	}

	retval = synaptics_ts_do_fw_update(ts, fw_entry->data, fw_entry->size,
			delay_setting, false, false);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: failed fw update, ret=%d\n", __func__, retval);
		sec_input_irq_enable(ts->plat_data);
		goto done;
	}

	retval = synaptics_ts_set_up_app_fw(ts);
	if (retval < 0) {
		input_err(true, ts->dev, "%s: Fail to set up application firmware\n", __func__);
		sec_input_irq_enable(ts->plat_data);
		goto done;
	}

	if (delay_setting != FORCE_ATTN_DRIVEN)
		sec_input_irq_enable(ts->plat_data);

#ifdef TCLM_CONCEPT
	if (ts->firmware_update_done) {
		retval = synaptics_ts_tclm_read(ts->dev, SEC_TCLM_NVM_ALL_DATA);
		if (retval < 0) {
			input_info(true, ts->dev, "%s: SEC_TCLM_NVM_ALL_DATA read fail", __func__);
			goto done;
		}

		input_info(true, ts->dev, "%s: tune_fix_ver [%04X] afe_base [%04X]\n",
			__func__, ts->tdata->nvdata.tune_fix_ver, ts->tdata->afe_base);

		if ((ts->tdata->tclm_level > TCLM_LEVEL_CLEAR_NV) &&
			((ts->tdata->nvdata.tune_fix_ver == 0xffff)
			|| (ts->tdata->afe_base > ts->tdata->nvdata.tune_fix_ver))) {
			/* tune version up case */
			sec_tclm_root_of_cal(ts->tdata, CALPOSITION_TUNEUP);
			restore_cal = true;
		} else if (ts->tdata->tclm_level == TCLM_LEVEL_CLEAR_NV) {
			/* firmup case */
			sec_tclm_root_of_cal(ts->tdata, CALPOSITION_FIRMUP);
			restore_cal = true;
		} else if ((ts->tdata->nvdata.tune_fix_ver >> 8) == 0x31) {
			sec_tclm_root_of_cal(ts->tdata, CALPOSITION_TUNEUP);
			restore_cal = true; /* temp */
		}

		if (restore_cal) {
			input_info(true, ts->dev, "%s: RUN OFFSET CALIBRATION\n", __func__);
			if (sec_execute_tclm_package(ts->tdata, 0) < 0)
				input_err(true, ts->dev, "%s: sec_execute_tclm_package fail\n", __func__);
		}
	}
#endif

done:
#ifdef TCLM_CONCEPT
	sec_tclm_root_of_cal(ts->tdata, CALPOSITION_NONE);
#endif
	release_firmware(fw_entry);
exit_fwload:
	return retval;
}

static int synaptics_ts_load_fw(struct synaptics_ts_data *ts, int update_type)
{
	int error = 0;
	struct app_config_header *header;
	struct synaptics_ts_reflash_data_blob reflash_data;
	const struct firmware *fw_entry;
	char fw_path[SEC_TS_MAX_FW_PATH];
#if IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
	int is_fw_signed = 1;
#else
	int is_fw_signed = 0;
#endif
#ifdef TCLM_CONCEPT
	int restore_cal = 0;
#endif
	unsigned int delay_setting = 0;

	if ((update_type != TSP_SDCARD) && (update_type != TSP_BUILT_IN)) {
		input_err(true, ts->dev, "%s: not support update type %d\n", __func__, update_type);
		return -EINVAL;
	}

	delay_setting = (ts->fw_erase_delay << 16) | ts->fw_write_block_delay;

	if (delay_setting == 0)
		delay_setting = FORCE_ATTN_DRIVEN;

	if (delay_setting != FORCE_ATTN_DRIVEN)
		sec_input_irq_disable(ts->plat_data);

	synaptics_ts_buf_init(&reflash_data.out);
	if (update_type == TSP_SDCARD) {
		if (is_fw_signed) {
			long spu_ret = 0;
			long ori_size = 0;

			if (ts->plat_data->external_firmware_size <= 0 || !ts->plat_data->external_firmware_data) {
				input_err(true, ts->dev, "%s: data is invalid\n", __func__);
				error = -ENODEV;
				goto err_request_fw;
			}

			ori_size = ts->plat_data->external_firmware_size - SPU_METADATA_SIZE(TSP);
			spu_ret = spu_firmware_signature_verify("TSP", ts->plat_data->external_firmware_data, ts->plat_data->external_firmware_size);
			if (spu_ret != ori_size) {
				input_err(true, ts->dev, "%s: signature verify failed, spu_ret:%ld, ori_size:%ld\n",
					__func__, spu_ret, ori_size);
				error = -EPERM;
				goto err_request_fw;
			}
			reflash_data.image = ts->plat_data->external_firmware_data;
			reflash_data.image_size = ts->plat_data->external_firmware_size - SPU_METADATA_SIZE(TSP);
		} else {
			reflash_data.image = ts->plat_data->external_firmware_data;
			reflash_data.image_size = ts->plat_data->external_firmware_size;
		}
	} else if (update_type == TSP_BUILT_IN) {

		if (ts->plat_data->bringup == BRINGUP_SKIP_FW_UPDATE) {
			error = -1;
			input_err(true, ts->dev, "%s: can't update for bringup:%d\n",
					__func__, ts->plat_data->bringup);
			goto err_request_fw;
		}

		if (ts->plat_data->firmware_name)
			snprintf(fw_path, SEC_TS_MAX_FW_PATH, "%s", ts->plat_data->firmware_name);
		else
			goto err_request_fw;

		error = request_firmware(&fw_entry, fw_path, ts->dev);
		if (error) {
			input_err(true, ts->dev, "%s: firmware is not available %d\n", __func__, error);
			goto err_request_fw;
		}

		reflash_data.image = fw_entry->data;
		reflash_data.image_size = fw_entry->size;
	} else {
		input_err(true, ts->dev, "%s: not support update type %d\n", __func__, update_type);
		goto err_request_fw;
	}

	synaptics_ts_pal_mem_set(&reflash_data.image_info, 0x00,
		sizeof(struct image_info));

	error = synaptics_ts_parse_fw_image(ts, reflash_data.image, &reflash_data.image_info);
	if (error < 0) {
		input_err(true, ts->dev, "%s: Fail to parse firmware image\n", __func__);
		synaptics_ts_buf_release(&reflash_data.out);
		if (update_type == TSP_BUILT_IN)
			release_firmware(fw_entry);
		goto err_request_fw;
	}

	header = (struct app_config_header *)reflash_data.image_info.data[AREA_APP_CONFIG].data;

#ifdef TCLM_CONCEPT
	sec_tclm_root_of_cal(ts->tdata, CALPOSITION_TESTMODE);
	restore_cal = 1;
#endif
	error = synaptics_ts_do_fw_update(ts,
			reflash_data.image,
			reflash_data.image_size,
			delay_setting,
			true, true);
	if (error < 0) {
		input_err(true, ts->dev, "Fail to do reflash\n");
		goto done;
	}

	/* re-initialize the app fw */
	error = synaptics_ts_set_up_app_fw(ts);
	if (error < 0) {
		input_err(true, ts->dev, "Fail to set up app fw after fw update\n");
		goto done;
	}

	if (delay_setting != FORCE_ATTN_DRIVEN)
		sec_input_irq_enable(ts->plat_data);

#ifdef TCLM_CONCEPT
	sec_execute_tclm_package(ts->tdata, 0);
#endif

	synaptics_ts_buf_release(&reflash_data.out);
#ifdef TCLM_CONCEPT
	sec_tclm_root_of_cal(ts->tdata, CALPOSITION_NONE);
#endif
	if (update_type == TSP_BUILT_IN)
		release_firmware(fw_entry);;
	return error;
done:
	synaptics_ts_buf_release(&reflash_data.out);
#ifdef TCLM_CONCEPT
	sec_tclm_root_of_cal(ts->tdata, CALPOSITION_NONE);
#endif

	if (update_type == TSP_BUILT_IN)
		release_firmware(fw_entry);

err_request_fw:
	if (delay_setting != FORCE_ATTN_DRIVEN)
		sec_input_irq_enable(ts->plat_data);
	return error;

}

int synaptics_ts_fw_update_on_hidden_menu(struct synaptics_ts_data *ts, int update_type)
{
	int retval = SEC_ERROR;

	/* Factory cmd for firmware update
	 * argument represent what is source of firmware like below.
	 *
	 * 0 : [BUILT_IN] Getting firmware which is for user.
	 * 1 : [UMS] Getting firmware from sd card.
	 * 2 : none
	 * 3 : [FFU] Getting firmware from apk.
	 */
	if ((update_type < TSP_BUILT_IN) || (update_type > TSP_VERIFICATION)) {
		input_err(true, ts->dev, "%s: Not support command[%d]\n",
				__func__, update_type);
		return retval;
	}
	retval = synaptics_ts_load_fw(ts, update_type);

	synaptics_ts_get_custom_library(ts);
	ts->plat_data->init(ts);

	return retval;
}

MODULE_LICENSE("GPL");
