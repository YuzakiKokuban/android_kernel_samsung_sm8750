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

#include <linux/slab.h>

#include "ese_log.h"
#include "ese_memory.h"

#ifdef CONFIG_STAR_MEMORY_LEAK
#define MAX_MEMORY_ALLOC_NUM	100

void *alloc_list[MAX_MEMORY_ALLOC_NUM];

void *ese_malloc(size_t size)
{
	int i = 0;

	for (i = 0; i < MAX_MEMORY_ALLOC_NUM; i ++) {
		if (alloc_list[i] == NULL) {
			break;
		}
	}

	if (i == MAX_MEMORY_ALLOC_NUM) {
		LOG_E("<MEMORY> exceed alloc list size");
		return NULL;
	}

	alloc_list[i] = kzalloc(size, GFP_KERNEL);
	LOG_I("<MEMORY> kzalloc addr : %p, size : %u", alloc_list[i], (unsigned int)size);
	return alloc_list[i];
}

void ese_free(void *ptr)
{
	int i = 0;

	for (i = 0; i < MAX_MEMORY_ALLOC_NUM; i ++) {
		if (alloc_list[i] == ptr) {
			break;
		}
	}

	if (i == MAX_MEMORY_ALLOC_NUM) {
		LOG_E("<MEMORY> failed to find memory in alloc list : %p", ptr);
		return;
	}

	alloc_list[i] = NULL;
	kfree((void *)ptr);
	LOG_I("<MEMORY> free addr : %p", ptr);
}

void ese_alloc_list(void)
{
	int i = 0;

	for (i = 0; i < MAX_MEMORY_ALLOC_NUM; i ++) {
		if (alloc_list[i] != NULL) {
			LOG_E("<MEMORY> non free memory : %p", alloc_list[i]);
		}
	}
}
#endif
