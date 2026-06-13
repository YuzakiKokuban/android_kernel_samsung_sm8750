/*
 * Copyright (C) 2020 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 */

#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/spi/spi.h>

#include "ese_spi.h"

#define ERR(msg...)		pr_err("[star-spi] : " msg)
#define INFO(msg...)	pr_info("[star-spi] : " msg)

int ese_spi_send(void *ctx, unsigned char *buf, unsigned int size)
{
	struct spi_device *spidev = ctx;
	int ret = 0;

	ret = spi_write(spidev, buf, size);
	if (ret < 0) {
		ERR("failed to write data %d", ret);
		return ret;
	}

	return (int)size;
}

int ese_spi_receive(void *ctx, unsigned char *buf, unsigned int size)
{
	struct spi_device *spidev = ctx;
	int ret = 0;

	ret = spi_read(spidev, (void *)buf, size);
	if (ret < 0) {
		ERR("failed to read data %d", ret);
		return ret;
	}

	return (int)size;
}
