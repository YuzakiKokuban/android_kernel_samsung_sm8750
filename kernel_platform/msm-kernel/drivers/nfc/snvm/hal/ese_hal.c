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
#include <linux/kernel.h>
#include <linux/slab.h>
#include "ese_i2c.h"
#include "ese_spi.h"
#include "ese_hal.h"

#undef ENABLE_HAL_LOG

struct ese_hal_s {
	void *client;
	int (*send)(void *ctx, unsigned char *buf, unsigned int size);
	int (*receive)(void *ctx, unsigned char *buf, unsigned int size);
};

int ese_hal_send(void *ctx, unsigned char *buf, unsigned int size)
{
	struct ese_hal_s *hal = (struct ese_hal_s *)ctx;

	if (hal == NULL || hal->client == NULL || hal->send == NULL
			|| buf == NULL || size == 0) {
		return -1;
	}
#ifdef ENABLE_HAL_LOG
	print_hex_dump(KERN_DEBUG, "[star-hal] send : ", DUMP_PREFIX_NONE, 16, 1, buf, size, 0);
#endif
	if (hal->send(hal->client, buf, size) < 0) {
		return -1;
	}

	return (int)size;
}

int ese_hal_receive(void *ctx, unsigned char *buf, unsigned int size)
{
	struct ese_hal_s *hal = (struct ese_hal_s *)ctx;

	if (hal == NULL || hal->client == NULL || hal->receive == NULL
			|| buf == NULL || size == 0) {
		return -1;
	}

	if (hal->receive(hal->client, buf, size) < 0) {
		return -1;
	}
#ifdef ENABLE_HAL_LOG
	print_hex_dump(KERN_DEBUG, "[star-hal] recv : ", DUMP_PREFIX_NONE, 16, 1, buf, size, 0);
#endif
	return (int)size;
}

void *ese_hal_init(enum hal_type_e type, void *client)
{
	struct ese_hal_s *hal = NULL;

	if (client == NULL) {
		return NULL;
	}

	hal = kzalloc(sizeof(struct ese_hal_s), GFP_KERNEL);
	if (hal == NULL) {
		return NULL;
	}

	switch(type) {
	case ESE_HAL_I2C:
		hal->client = client;
		hal->send = ese_i2c_send;
		hal->receive = ese_i2c_receive;
		break;
	case ESE_HAL_SPI:
		hal->client = client;
		hal->send = ese_spi_send;
		hal->receive = ese_spi_receive;
		break;
	default:
		kfree(hal);
		return NULL;
	}

	return hal;
}

void ese_hal_release(void *ctx)
{
	if (ctx != NULL) {
		kfree(ctx);
	}
}
