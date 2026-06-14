/*
 * Copyright (C) 2020 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __ESE_HAL__H
#define __ESE_HAL__H

enum hal_type_e {
	ESE_HAL_I2C = 0,
	ESE_HAL_SPI,
};

void *ese_hal_init(enum hal_type_e type, void *client);
void ese_hal_release(void *ctx);
int ese_hal_send(void *ctx, unsigned char *buf, unsigned int size);
int ese_hal_receive(void *ctx, unsigned char *buf, unsigned int size);

#endif
