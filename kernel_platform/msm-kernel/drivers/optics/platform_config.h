/*
 *
 * $Id: platform_config.h
 *
 * Copyright (C) 2019 Bk, sensortek Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 */

#ifndef __PLATFORM_CONFIG_H__
#define __PLATFORM_CONFIG_H__

#include <linux/types.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#ifdef CONFIG_IIO
	#include <linux/iio/events.h>
	#include <linux/iio/buffer.h>
	#include <linux/iio/iio.h>
	#include <linux/iio/trigger.h>
	#include <linux/iio/trigger_consumer.h>
	#include <linux/iio/triggered_buffer.h>
	#include <linux/iio/sysfs.h>

#endif

typedef enum
{
	ADDR_8BIT,
	ADDR_16BIT,
} I2C_REG_ADDR_TYPE;

typedef enum
{
	SPI_MODE0,
	SPI_MODE1,
	SPI_MODE2,
	SPI_MODE3,
} SPI_TRANSFER_MODE;

struct spi_manager
{
	struct spi_device *spi;
	struct mutex lock;
	SPI_TRANSFER_MODE trans_mode;
	void *any;
	u8 *spi_buffer;        /* SPI buffer, used for SPI transfer. */
} ;

struct i2c_manager
{
	struct i2c_client *client;
	struct mutex lock;
	I2C_REG_ADDR_TYPE addr_type;
	void *any;
} ;

#define kzalloc(size, mode) kzalloc(size, mode)
#define kfree(ptr) kfree(ptr)

extern const struct stk_bus_ops stk_spi_bops;
extern const struct stk_bus_ops stk_i2c_bops;
extern const struct stk_timer_ops stk_t_ops;
extern const struct stk_gpio_ops stk_g_ops;

#endif // __PLATFORM_CONFIG_H__