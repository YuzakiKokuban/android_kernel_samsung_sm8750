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

#ifndef __ESE_MEMORY__H
#define __ESE_MEMORY__H

#ifdef CONFIG_STAR_MEMORY_LEAK
void *ese_malloc(size_t size);
void ese_free(void *ptr);
void ese_print_list(void);

#define ESE_MALLOC		ese_malloc
#define ESE_FREE		ese_free
#define ESE_ALLOC_LIST() ese_alloc_list()
#else
#include <linux/slab.h>

#define ESE_MALLOC(x)	kzalloc(x, GFP_KERNEL)
#define ESE_FREE(x)		kfree(x)
#define ESE_ALLOC_LIST()
#endif

#endif
