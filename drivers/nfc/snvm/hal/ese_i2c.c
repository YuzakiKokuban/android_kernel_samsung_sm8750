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
#include <linux/i2c.h>

#include "ese_i2c.h"

#define ERR(msg...)		pr_err("[star-i2c] : " msg)
#define INFO(msg...)	pr_info("[star-i2c] : " msg)

int ese_i2c_send(void *ctx, unsigned char *buf, unsigned int size)
{
	struct i2c_client *client = ctx;
	int ret = 0;

	ret = i2c_master_send(client, buf, size);
	if (ret < 0) {
		ERR("failed to send data %d", ret);
		return ret;
	}

	return (int)size;
}

int ese_i2c_receive(void *ctx, unsigned char *buf, unsigned int size)
{
	struct i2c_client *client = ctx;
	int ret = 0;

	ret = i2c_master_recv(client, (void *)buf, size);
	if (ret < 0) {
		ERR("failed to recv data %d", ret);
		return ret;
	}

	return (int)size;
}
