// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * Sensitive Data Protection
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <ddar/cache_cleanup.h>

#define DDAR_CACHE_CLEANUP_DEBUG   0

#if DDAR_CACHE_CLEANUP_DEBUG
static void ddar_page_dump(unsigned char *buf, int len, const char *str)
{
	unsigned int i;
	char s[512];

	s[0] = 0;
	for (i = 0; i < len && i < 32; ++i) {
		char tmp[8];

		sprintf(tmp, " %02x", buf[i]);
		strcat(s, tmp);
	}

	if (len > 32) {
		char tmp[8];

		sprintf(tmp, " ...");
		strcat(s, tmp);
	}

	pr_debug("%s [%s len=%d]\n", s, str, len);
}
#else
static void ddar_page_dump(unsigned char *buf, int len, const char *str)
{
}
#endif

void ddar_page_cleanup(struct page *page)
{
	if (page && page->mapping) {
		if (mapping_sensitive(page->mapping)) {
			void *d;

#if DDAR_CACHE_CLEANUP_DEBUG
			pr_debug("%s : deleting [%s] sensitive page.\n",
					__func__, page->mapping->host->i_sb->s_type->name);
			//dump_stack();
#endif
			d = kmap_local_page(page);
			if (d) {
				ddar_page_dump((unsigned char *)d, PAGE_SIZE, "freeing");
				clear_page(d);
				ddar_page_dump((unsigned char *)d, PAGE_SIZE, "freed");
				kunmap_local(d);
			}
		}
	}
}
