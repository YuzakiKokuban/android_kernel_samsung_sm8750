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

#ifndef _ESE_DATA_H_
#define _ESE_DATA_H_

#include "ese_error.h"

typedef struct data_packet_s {
	uint8_t *data;
	uint32_t size;
} data_packet_t;

typedef struct data_list_s {
	data_packet_t packet;
	struct data_list_s *next;
} data_list_t;

ESE_STATUS ese_data_init(data_list_t *head);
ESE_STATUS ese_data_store(data_list_t *head, uint8_t *data, uint32_t data_size, uint8_t copy);
ESE_STATUS ese_data_get(data_list_t *head, uint8_t **data, uint32_t *data_size);
ESE_STATUS ese_data_delete(data_list_t *head);

#endif
