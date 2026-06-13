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

#include <linux/kernel.h>
#include <linux/string.h>

#include "ese_log.h"
#include "ese_memory.h"
#include "ese_data.h"

static int32_t _ese_data_get_from_list(data_list_t *head, uint8_t *data, uint32_t *data_size)
{
	data_list_t *cur_node;
	uint32_t offset = 0;

	if (head == NULL || data == NULL) {
		LOG_E("invalid header and data");
		return -1;
	}

	cur_node = head->next;
	while (cur_node != NULL) {
		memcpy((data + offset), cur_node->packet.data, cur_node->packet.size);
		offset += cur_node->packet.size;
		cur_node = cur_node->next;
	}

	*data_size = offset;
	return 0;
}

static int32_t _ese_data_delete_list(data_list_t *head)
{
	data_list_t *cur, *next;

	if (head == NULL) {
		return -1;
	}

	cur = head->next;
	while (cur != NULL) {
		next = cur->next;
		ESE_FREE(cur->packet.data);
		ESE_FREE(cur);
		cur = NULL;
		cur = next;
	}

	head->next = NULL;
	head->packet.data = NULL;
	head->packet.size = 0;
	return 0;
}

ESE_STATUS ese_data_init(data_list_t *head)
{
	head->next = NULL;
	head->packet.data = NULL;
	head->packet.size = 0;
	return ESESTATUS_SUCCESS;
}

ESE_STATUS ese_data_store(data_list_t *head, uint8_t *data, uint32_t data_size, uint8_t copy)
{
	data_list_t *new_node = NULL;
	data_list_t *cur_node = NULL;

	new_node = ESE_MALLOC(sizeof(data_list_t));
	if (new_node == NULL) {
		return ESESTATUS_MEMORY_ALLOCATION_FAIL;
	}

	new_node->next = NULL;
	new_node->packet.size = data_size;
	if (copy) {
		new_node->packet.data = ESE_MALLOC(data_size);
		if (new_node->packet.data == NULL) {
			ESE_FREE(new_node);
			return ESESTATUS_MEMORY_ALLOCATION_FAIL;
		}
		memcpy(new_node->packet.data, data, data_size);
	} else {
		new_node->packet.data = data;
	}

	cur_node = head;
	while (cur_node->next != NULL) {
		cur_node = cur_node->next;
	}

	cur_node->next = new_node;
	head->packet.size += data_size;
	return ESESTATUS_SUCCESS;
}

ESE_STATUS ese_data_get(data_list_t *head, uint8_t **data, uint32_t *data_size)
{
	uint32_t total_size = 0;
	uint8_t* tmp_buf = NULL;

	if (data != NULL && data_size != NULL) {
		if (head->packet.size == 0) {
			*data = NULL;
			*data_size = 0;
			return ESESTATUS_INVALID_BUFFER;
		}

		tmp_buf = ESE_MALLOC(head->packet.size);
		if (tmp_buf == NULL) {
			LOG_E("failed to allocate memory");
			_ese_data_delete_list(head);
			return ESESTATUS_MEMORY_ALLOCATION_FAIL;
		}

		if (_ese_data_get_from_list(head, tmp_buf, &total_size) != 0) {
			LOG_E("failed to get data from list");
			ESE_FREE(tmp_buf);
			_ese_data_delete_list(head);
			return ESESTATUS_INVALID_BUFFER;
		}

		if (total_size != head->packet.size) {
			LOG_E("mismatch size [%d, %d]", total_size, head->packet.size);
			ESE_FREE(tmp_buf);
			_ese_data_delete_list(head);
			return ESESTATUS_INVALID_BUFFER;
		}
		*data = tmp_buf;
		*data_size = total_size;
	}

	_ese_data_delete_list(head);
	return ESESTATUS_SUCCESS;
}

ESE_STATUS ese_data_delete(data_list_t *head)
{
	data_list_t *cur, *next;

	if (head == NULL) {
		return -1;
	}

	cur = head->next;
	while (cur != NULL) {
		next = cur->next;
		ESE_FREE(cur->packet.data);
		ESE_FREE(cur);
		cur = NULL;
		cur = next;
	}

	head->next = NULL;
	head->packet.data = NULL;
	head->packet.size = 0;
	return 0;
}
