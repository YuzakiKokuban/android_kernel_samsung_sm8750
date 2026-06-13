/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _CAM_SENSOR_IO_H_
#define _CAM_SENSOR_IO_H_

#include <media/cam_sensor.h>
#include "cam_sensor_cmn_header.h"

/* Master Types */
#define CCI_MASTER           1
#define I2C_MASTER           2
#define SPI_MASTER           3
#define I3C_MASTER           4

/**
 * @i2c_client: I2C client information structure
 * @i3c_client: I3C client information structure
 * @pm_enable: Power Management Enable flag
 * @i3c_wait_for_hotjoin: Flag to notify I3C master about HOT_JOIN wait
 */
struct cam_sensor_qup_client {
	struct i2c_client *i2c_client;
	struct i3c_device *i3c_client;
	bool   pm_ctrl_client_enable;
	bool   i3c_wait_for_hotjoin;
};

/**
 * @master_type: CCI master type
 * @qup_client: QUP client information structure
 * @cci_client: CCI client information structure
 * @spi_client: SPI client information structure
 * @pm_enable: Power Management Enable flag
 */
struct camera_io_master {
	int master_type;
	struct cam_sensor_qup_client *qup_client;
	struct cam_sensor_cci_client *cci_client;
	struct cam_sensor_spi_client *spi_client;
	char   sensor_name[CAM_SENSOR_NAME_MAX_SIZE];
};

/**
 * @io_master_info: I2C/SPI master information
 * @addr: I2C address
 * @data: I2C data
 * @addr_type: I2C addr_type
 * @data_type: I2C data type
 * @is_probing: Is probing a sensor
 *
 * This API abstracts read functionality based on master type
 */
int32_t camera_io_dev_read(struct camera_io_master *io_master_info,
	uint32_t addr, uint32_t *data,
	enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type,
	bool is_probing);

/**
 * @io_master_info: I2C/SPI master information
 * @addr: I2C address
 * @data: I2C data
 * @addr_type: I2C addr type
 * @data_type: I2C data type
 * @num_bytes: number of bytes
 *
 * This API abstracts read functionality based on master type
 */
int32_t camera_io_dev_read_seq(struct camera_io_master *io_master_info,
	uint32_t addr, uint8_t *data,
	enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type,
	int32_t num_bytes);

/**
 * @io_master_info: I2C/SPI master information
 *
 * This API initializes the I2C/SPI master based on master type
 */
int32_t camera_io_init(struct camera_io_master *io_master_info);

/**
 * @io_master_info: I2C/SPI master information
 *
 * This API releases the I2C/SPI master based on master type
 */
int32_t camera_io_release(struct camera_io_master *io_master_info);

/**
 * @io_master_info: I2C/SPI master information
 * @write_setting: write settings information
 *
 * This API abstracts write functionality based on master type
 */
int32_t camera_io_dev_write(struct camera_io_master *io_master_info,
	struct cam_sensor_i2c_reg_setting *write_setting);

/**
 * @io_master_info: I2C/SPI master information
 * @write_setting: write settings information
 * @cam_sensor_i2c_write_flag: differentiate between burst & seq
 *
 * This API abstracts write functionality based on master type and
 * write flag for continuous write
 */
int32_t camera_io_dev_write_continuous(struct camera_io_master *io_master_info,
	struct cam_sensor_i2c_reg_setting *write_setting,
	uint8_t cam_sensor_i2c_write_flag);

int32_t camera_io_dev_erase(struct camera_io_master *io_master_info,
	uint32_t addr, uint32_t size);
/**
 * @io_master_info: I2C/SPI master information
 * @addr: I2C address
 * @data: I2C data
 * @data_mask: I2C data mask
 * @data_type: I2C data type
 * @addr_type: I2C address type
 * @delay_ms: delay in milli seconds
 *
 * This API abstracts poll functionality based on master type
 */
int32_t camera_io_dev_poll(struct camera_io_master *io_master_info,
	uint32_t addr, uint16_t data, uint32_t data_mask,
	enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type,
	uint32_t delay_ms);

#include "cam_sensor_i2c.h"
#include "cam_sensor_spi.h"
#include "cam_sensor_i3c.h"

#if defined(CONFIG_SAMSUNG_CAMERA)
#define CHUNK_SIZE	200
int camera_io_dev_write_continuous_split(struct i2c_settings_list *i2c_list,
	struct camera_io_master *io_master_info);
#endif
#endif /* _CAM_SENSOR_IO_H_ */
