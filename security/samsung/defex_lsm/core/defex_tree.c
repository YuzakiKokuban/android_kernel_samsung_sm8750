// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifdef __KERNEL__

#include <linux/string.h>
#include <linux/types.h>
#include <linux/version.h>
#include "include/defex_internal.h"

#else

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#endif /* __KERNEL__ */

#include "../include/defex_debug.h"
#include "../include/defex_rules.h"
#include "../include/defex_tree.h"

static const char dtree_magic_v10[16] = "DEFEX_RULES_FILE";
static const char dtree_magic_v20[8] = "DEFEX2.0";

struct subpath_extract_ctx {
	const char *path;
};

__visible_for_testing void subpath_extract_init(struct subpath_extract_ctx *ctx, const char *path)
{
	if (!ctx)
		return;
	if (path) {
		while (*path == '/')
			path++;
	}
	ctx->path = path;
}

__visible_for_testing int subpath_extract_next(struct subpath_extract_ctx *ctx,
		const char **subpath_ptr)
{
	const char *path, *next_separator;
	int l = 0;

	if (!ctx || !ctx->path)
		return 0;

	path = ctx->path;
	next_separator = strchr(path, '/');
	if (!next_separator)
		l = (int)strlen(path);
	else
		l = (int)(next_separator - path);
	if (subpath_ptr)
		*subpath_ptr = path;

	path += l;
	while (*path == '/')
		path++;

	ctx->path = path;
	return l;
}

unsigned short d_tree_get_version(void *data, size_t data_size)
{
	size_t data_size_tmp;
	enum d_tree_version ver = D_TREE_VERSION_INVALID;
	struct rule_item_struct *header_ptr_v10 = (struct rule_item_struct *)data;
	static const char *ver_txt[2] = { "1.0", "2.0" };

#ifdef DEFEX_INTEGRITY_ENABLE
	const int integrity_state = 1;
#else
	const int integrity_state = 0;
#endif /* DEFEX_INTEGRITY_ENABLE */

	if (!data)
		return (unsigned short)ver;

	/* checking the version 1.0 */
	if (data_size >= D_TREE_V10_HDR_SIZE &&
			header_ptr_v10->next_level == D_TREE_V10_HDR_SIZE &&
			header_ptr_v10->size == sizeof(dtree_magic_v10)) {
		if (!memcmp(header_ptr_v10->name, dtree_magic_v10, sizeof(dtree_magic_v10))) {
			ver = D_TREE_VERSION_10;
			goto label_ver_ok;
		}

	}

	/* checking the version 2.0 */
	if (data_size >= D_TREE_V20_HDR_SIZE) {
		data_size_tmp = D_TREE_DWORD_REF(data, D_TREE_HDR_DATA_SIZE);
		if (data_size_tmp <= data_size && data_size_tmp >= D_TREE_V20_HDR_SIZE) {
			if (!memcmp(D_TREE_GET_PTR(data, D_TREE_HDR_VERSION), dtree_magic_v20,
					sizeof(dtree_magic_v20))) {
				ver = D_TREE_VERSION_20;
				goto label_ver_ok;
			}
		}
	}
	defex_log_err("Rules structure is wrong. Integrity state: %d", integrity_state);
	return (unsigned short)ver;

label_ver_ok:
	defex_log_info("Rules version:%s structure is OK. Integrity state: %d", ver_txt[ver - 1],
		integrity_state);
	return (unsigned short)ver;
}

int d_tree_get_header(void *data, size_t data_size, struct d_tree *the_tree)
{
	int ret = -1;

	if (!data || !the_tree)
		return ret;

	memset(the_tree, 0, sizeof(*the_tree));

	the_tree->version = d_tree_get_version(data, data_size);
	if (the_tree->version == D_TREE_VERSION_INVALID)
		return ret;

	the_tree->data = data;
	the_tree->text_data = data;
	the_tree->integrity_data = data;
	the_tree->bin_data = data;

	if (the_tree->version == D_TREE_VERSION_10) {
		the_tree->data_size = GET_ITEM_PTR(0, data)->data_size;
		the_tree->root_offset = 0;
		ret = 0;
	}

	if (the_tree->version == D_TREE_VERSION_20) {
		the_tree->data_size = D_TREE_DWORD_REF(data, D_TREE_HDR_DATA_SIZE);
		the_tree->root_offset = D_TREE_DWORD_REF(data, D_TREE_HDR_ROOT_OFFSET);
		the_tree->text_offset = D_TREE_DWORD_REF(data, D_TREE_HDR_TEXT_OFFSET);
		the_tree->text_count = D_TREE_WORD_REF(data, D_TREE_HDR_TEXT_COUNT);
		the_tree->integrity_offset = D_TREE_DWORD_REF(data, D_TREE_HDR_INTEGRITY_OFFSET);
		the_tree->integrity_count = D_TREE_WORD_REF(data, D_TREE_HDR_INTEGRITY_COUNT);
		the_tree->bin_offset = D_TREE_DWORD_REF(data, D_TREE_HDR_BIN_OFFSET);
		the_tree->bin_count = D_TREE_WORD_REF(data, D_TREE_HDR_BIN_COUNT);
		ret = 0;
	}
	return ret;
}

__visible_for_testing int check_tree_bounds(const struct d_tree *__tree,
		unsigned int offset, unsigned int size)
{
	size_t max_offset = (size_t)__tree->root_offset + offset + size;
	return (max_offset <= __tree->data_size);
}

__visible_for_testing int check_tab_bounds(const struct d_tree *__tree,
		unsigned int tab_offset, unsigned int offset, unsigned int size)
{
#ifdef __KERNEL__
	size_t max_offset = (size_t)tab_offset + offset + size;
	return (max_offset <= __tree->data_size);
#else
	(void)__tree;
	(void)tab_offset;
	(void)offset;
	(void)size;
	return 1;
#endif
}

int d_tree_get_item_header(struct d_tree *the_tree, unsigned int offset, struct d_tree_item *item)
{
	unsigned int features, field_offset = 0, next_offset;
	struct rule_item_struct *item_v10;
	void *item_ptr;

	if (!the_tree || !item)
		return 0;
	item_ptr = D_TREE_GET_PTR(the_tree->data, the_tree->root_offset + offset);
	memset(item, 0, sizeof(*item));
	item->tree = the_tree;
	item->item_offset = (unsigned short)offset;
	if (the_tree->version == D_TREE_VERSION_10) {
		item_v10 = GET_ITEM_PTR(0, item_ptr);
		item->item_v10 = item_v10;
		item->features = item_v10->feature_type;
		return sizeof(struct rule_item_struct) + item_v10->size;
	}

	if (the_tree->version != D_TREE_VERSION_20)
		return (int)field_offset;

	next_offset = field_offset + 4;
	if (!check_tree_bounds(the_tree, offset, next_offset))
		return 0;
	features = D_TREE_DWORD_REF(item_ptr, field_offset);
	item->features = features;
	field_offset = next_offset;
	if (features & d_tree_item_path) {
		next_offset = field_offset + 2;
		if (!check_tree_bounds(the_tree, offset, next_offset))
			return 0;
		item->path_index = D_TREE_WORD_REF(item_ptr, field_offset);
		field_offset = next_offset;
	}
	if (features & d_tree_item_text) {
		next_offset = field_offset + 2;
		if (!check_tree_bounds(the_tree, offset, next_offset))
			return 0;
		item->text_index = D_TREE_WORD_REF(item_ptr, field_offset);
		field_offset = next_offset;
	}
	if (features & d_tree_item_integrity) {
		next_offset = field_offset + 2;
		if (!check_tree_bounds(the_tree, offset, next_offset))
			return 0;
		item->integrity_index = D_TREE_WORD_REF(item_ptr, field_offset);
		field_offset = next_offset;
	}
	if (features & d_tree_item_bin) {
		next_offset = field_offset + 2;
		if (!check_tree_bounds(the_tree, offset, next_offset))
			return 0;
		item->bin_index = D_TREE_WORD_REF(item_ptr, field_offset);
		field_offset = next_offset;
	}
	if (features & d_tree_item_children) {
		next_offset = field_offset + 6;
		if (!check_tree_bounds(the_tree, offset, next_offset))
			return 0;
		item->child_offset = D_TREE_WORD_REF(item_ptr, field_offset);
		field_offset += 2;
		item->child_count = D_TREE_WORD_REF(item_ptr, field_offset);
		field_offset += 2;
		item->child_size = D_TREE_WORD_REF(item_ptr, field_offset);
		field_offset += 2;
		if (!item->child_count || !item->child_size
			|| !check_tree_bounds(the_tree, item->child_offset, 4 +
				(item->child_count - 1) * (unsigned int)item->child_size))
			return 0;
	}
	if (features & d_tree_item_linked) {
		next_offset = field_offset + 6;
		if (!check_tree_bounds(the_tree, offset, next_offset))
			return 0;
		item->linked_features = D_TREE_DWORD_REF(item_ptr, field_offset);
		field_offset += 4;
		item->link_index = D_TREE_WORD_REF(item_ptr, field_offset);
		field_offset += 2;
	}
	if (features & d_tree_item_dword) {
		next_offset = field_offset + 4;
		if (!check_tree_bounds(the_tree, offset, next_offset))
			return 0;
		item->data_dword = D_TREE_DWORD_REF(item_ptr, field_offset);
		field_offset = next_offset;
	}
	if (features & d_tree_item_word) {
		next_offset = field_offset + 2;
		if (!check_tree_bounds(the_tree, offset, next_offset))
			return 0;
		item->data_word = D_TREE_WORD_REF(item_ptr, field_offset);
		field_offset = next_offset;
	}
	if (features & d_tree_item_byte) {
		next_offset = field_offset + 1;
		if (!check_tree_bounds(the_tree, offset, next_offset))
			return 0;
		item->data_byte = D_TREE_BYTE_REF(item_ptr, field_offset);
		field_offset = next_offset;
	}
	item->item_size = (unsigned short)field_offset;
	return (int)field_offset;
}

const char *d_tree_get_table_data(enum d_tree_table_type t_type, struct d_tree *the_tree,
		unsigned int index, unsigned int *data_size)
{
	unsigned int tab_offset = 0, offset, size = 0, max_index;
	const char *data_ptr = NULL;
	void *tab_ptr = NULL;

	if (data_size)
		*data_size = 0;

	if (!the_tree)
		return NULL;

	switch (t_type) {
	case d_tree_string_table:
		tab_offset = the_tree->text_offset;
		if (the_tree->text_data)
			tab_ptr = D_TREE_GET_PTR(the_tree->text_data, tab_offset);
		max_index = the_tree->text_count;
		break;
	case d_tree_integrity_table:
		tab_offset = the_tree->integrity_offset;
		if (the_tree->integrity_data)
			tab_ptr = D_TREE_GET_PTR(the_tree->integrity_data, tab_offset);
		max_index = the_tree->integrity_count;
		break;
	case d_tree_bin_table:
		tab_offset = the_tree->bin_offset;
		if (the_tree->bin_data)
			tab_ptr = D_TREE_GET_PTR(the_tree->bin_data, tab_offset);
		max_index = the_tree->bin_count;
		break;
	default:
		return NULL;
	}
	if (((uintptr_t)tab_ptr) > 0xffff && index && index <= max_index
			&& check_tab_bounds(the_tree, tab_offset, 0, 0)) {
		index *= 4;
		if (check_tab_bounds(the_tree, tab_offset, index, 4)) {
			size = D_TREE_WORD_REF(tab_ptr, index);
			offset = D_TREE_WORD_REF(tab_ptr, index + 2);
			if (check_tab_bounds(the_tree, tab_offset, offset, size))
				data_ptr = (const void *)D_TREE_GET_PTR(tab_ptr, offset);
			else
				size = 0;
		}
	}
	if (data_size && data_ptr)
		*data_size = size;
	return data_ptr;
}

const char *d_tree_get_subpath(struct d_tree_item *item, unsigned int *subpath_size)
{
	const char *text_ptr = NULL;

	if (subpath_size)
		*subpath_size = 0;

	if (!item || !item->tree)
		return NULL;

	if (item->tree->version == D_TREE_VERSION_10) {
		text_ptr = (const char *)item->item_v10->name;
		if (subpath_size)
			*subpath_size = item->item_v10->size;
	}
	if (item->tree->version == D_TREE_VERSION_20)
		text_ptr = (const char *)d_tree_get_table_data(d_tree_string_table, item->tree,
			item->path_index, subpath_size);
	return text_ptr;
}

const char *d_tree_get_text(struct d_tree_item *item, unsigned int *text_size)
{
	const char *text_ptr = NULL;

	if (text_size)
		*text_size = 0;

	if (!item || !item->tree)
		return NULL;

	if (item->tree->version == D_TREE_VERSION_10) {
		text_ptr = (const char *)item->item_v10->name;
		if (text_size)
			*text_size = item->item_v10->size;
	}
	if (item->tree->version == D_TREE_VERSION_20)
		text_ptr = (const char *)d_tree_get_table_data(d_tree_string_table, item->tree,
			item->text_index, text_size);
	return text_ptr;
}

const unsigned char *d_tree_get_integrity(struct d_tree_item *item, unsigned int *data_size)
{
	const unsigned char *data_ptr = NULL;

	if (data_size)
		*data_size = 0;

	if (!item || !item->tree)
		return NULL;

#ifdef DEFEX_INTEGRITY_ENABLE
	if (item->tree->version == D_TREE_VERSION_10) {
		data_ptr = (const unsigned char *)item->item_v10->integrity;
		if (data_size)
			*data_size = INTEGRITY_LENGTH;
	}
#endif /* DEFEX_INTEGRITY_ENABLE */
	if (item->tree->version == D_TREE_VERSION_20)
		data_ptr = (const unsigned char *)d_tree_get_table_data(d_tree_integrity_table,
			item->tree, item->integrity_index, data_size);
	return data_ptr;
}

const unsigned char *d_tree_get_bin(struct d_tree_item *item, unsigned int index,
		unsigned int *data_size)
{
	const unsigned char *data_ptr = NULL;

	if (data_size)
		*data_size = 0;

	if (!item || !item->tree)
		return NULL;

	if (item->tree->version == D_TREE_VERSION_20)
		data_ptr = (const unsigned char *)d_tree_get_table_data(d_tree_bin_table,
			item->tree, index, data_size);
	return data_ptr;
}

struct d_tree_item *d_tree_next_child(struct d_tree_item *parent_item,
		struct d_tree_item *child_item)
{
	size_t offset = 0;

	if (parent_item->tree->version == D_TREE_VERSION_10)
		offset = child_item->item_v10->next_file;

	if (parent_item->tree->version == D_TREE_VERSION_20
			&& ((parent_item->child_index + 1) < parent_item->child_count)) {
		++parent_item->child_index;
		offset = (size_t)parent_item->child_offset
			+ (parent_item->child_index * (size_t)parent_item->child_size);
	}

	if (offset && d_tree_get_item_header(parent_item->tree, (unsigned int)offset, child_item))
		return child_item;

	return NULL;
}

int d_tree_compare_item_name(struct d_tree_item *item, const char *name, size_t l)
{
	const char *subdir_ptr;
	unsigned int subdir_size;
	size_t part_size, name_offset = 0, wildcard_offset = 0;

	subdir_ptr = d_tree_get_subpath(item, &subdir_size);
	if (subdir_ptr && name && l) {
		if (!(item->features & d_tree_item_wildcard)) {
			if ((size_t)subdir_size == l && !memcmp(name, subdir_ptr, l))
				return 1;
			return 0;
		}
		while (wildcard_offset < subdir_size) {
			part_size = ((const uint8_t *)subdir_ptr)[wildcard_offset++];
			if (part_size & 0x80) {
				part_size &= 0x7F;
				name_offset += (part_size) ? part_size : l;
			} else {
				if (name_offset > l)
					return 0;
				if ((l - name_offset) < part_size
				   || (subdir_size - wildcard_offset) < part_size)
					return 0;
				if (memcmp(name + name_offset,
					   subdir_ptr + wildcard_offset, part_size))
					return 0;
				name_offset += part_size;
				wildcard_offset += part_size;
			}
			if (name_offset >= l && wildcard_offset >= subdir_size)
				return 1;
		}
	}
	return 0;
}

size_t d_tree_unpack_wildcard(const char *packed_str, size_t packed_size, char *unpacked_str,
		size_t unpacked_size)
{
	size_t part_size, offset = 0;

	while (packed_size) {
		--packed_size;
		part_size = (uint8_t)*packed_str++;
		if (part_size & 0x80) {
			part_size &= 0x7F;
			if (offset >= (unpacked_size - 4))
				break;
			offset += (size_t)sprintf(unpacked_str + offset, "[*%d]", (int)part_size);
		} else {
			if ((offset + part_size) >= unpacked_size)
				break;
			memcpy(unpacked_str + offset, packed_str, part_size);
			offset += part_size;
			packed_str += part_size;
			packed_size -= part_size;
		}
	}
	unpacked_str[offset] = 0;
	return offset;
}

struct d_tree_item *d_tree_lookup_dir_init(struct d_tree_item *parent_item,
		struct d_tree_item *child_item)
{
	unsigned int offset = 0;

	parent_item->child_index = 0;
	if (parent_item->tree->version == D_TREE_VERSION_10)
		offset = parent_item->item_v10->next_level;

	if (parent_item->tree->version == D_TREE_VERSION_20 && parent_item->child_count)
		offset = parent_item->child_offset;

	if (offset && d_tree_get_item_header(parent_item->tree, offset, child_item))
		return child_item;

	return NULL;
}

unsigned int d_tree_lookup_dir(struct d_tree_ctx *ctx, struct d_tree_item *parent_item,
		const char *name, size_t l)
{
	struct d_tree_item tmp_item, *child_item;
	unsigned int offset = 0;
	unsigned int feature_flags, tmp_item_attr = 0, tmp_item_rec = 0;
	unsigned int feature_mask = ctx->feature_flags & (~feature_for_recovery);
	unsigned int recovery_mask = ctx->feature_flags & feature_for_recovery;

	child_item = d_tree_lookup_dir_init(parent_item, &tmp_item);

	while (child_item) {
		if (d_tree_compare_item_name(child_item, name, l)) {
			feature_flags = child_item->features;
			if (!(feature_flags & feature_is_file)) {
				offset = child_item->item_offset;
				break;
			}
			if ((feature_flags & feature_mask) == feature_mask)
				tmp_item_attr = child_item->item_offset;
			if ((feature_flags & feature_for_recovery) == recovery_mask)
				tmp_item_rec = child_item->item_offset;
			if (tmp_item_attr && tmp_item_attr == tmp_item_rec)
				break;
		}
		child_item = d_tree_next_child(parent_item, child_item);
	}
	if (!offset)
		offset = (tmp_item_attr) ? tmp_item_attr : tmp_item_rec;
	if (offset)
		ctx->item_offset = offset;
	return offset;
}

void d_tree_lookup_init(struct d_tree *the_tree,
	struct d_tree_ctx *ctx,
	unsigned int starting_item_offset,
	unsigned int feature_flags,
	unsigned int lookup_flags)
{
	memset(ctx, 0, sizeof(struct d_tree_ctx));
	ctx->tree = the_tree;
	ctx->feature_flags = feature_flags;
	ctx->lookup_flags = lookup_flags;
	ctx->item_offset = starting_item_offset;
}

int d_tree_lookup_path(struct d_tree_ctx *ctx,
			const char *file_path,
			struct d_tree_item *found_item)
{
	struct subpath_extract_ctx subpath_ctx;
	const char *subpath;
	size_t l;
	unsigned int cur_features, offset;
	struct d_tree_item local_item;

	if (!found_item)
		found_item = &local_item;

	if (!ctx)
		return -1;

	offset = ctx->item_offset;

	if (!d_tree_get_item_header(ctx->tree, offset, found_item))
		return -1;

	subpath_extract_init(&subpath_ctx, file_path);
	do {
		l = (size_t)subpath_extract_next(&subpath_ctx, &subpath);
		if (!l)
			break;

		offset = d_tree_lookup_dir(ctx, found_item, subpath, l);
		if (!offset)
			break;

		if (!d_tree_get_item_header(ctx->tree, offset, found_item))
			return -1;

		cur_features = found_item->features;

		if (cur_features & ctx->feature_flags) {
			if (ctx->feature_flags & (feature_immutable_path_open
					| feature_immutable_path_write) &&
				!(cur_features & feature_is_file)) {
				/* Allow open the folder by default */
				if (!*subpath_ctx.path)
					return 0;
			}
			if ((cur_features & feature_is_file) || !*subpath_ctx.path) {
				if (d_tree_check_linked_rules(found_item, ctx->linked_offset,
						ctx->feature_flags) != 0)
					return (int)offset;
				return 0;
			}
			if (d_tree_check_linked_rules(found_item, ctx->linked_offset,
					ctx->feature_flags) > 0)
				return (int)offset;
		}
	} while (*subpath_ctx.path);
	return 0;
}

int d_tree_check_linked_rules(struct d_tree_item *part1_item, unsigned int part2_offset,
		unsigned int feature)
{
	const unsigned short *link_array;
	unsigned int i, link_size = 0;

	if (!part2_offset) {
		if (!(part1_item->features & d_tree_item_linked)
				|| !part1_item->link_index)
			return 1;
		return -1;
	}

	if (!(part1_item->linked_features & feature))
		return 0;
	/* Search on the list of linked items */
	link_array = (const unsigned short *)d_tree_get_bin(part1_item, part1_item->link_index,
		&link_size);
	if (link_array && link_size) {
		link_size >>= 1;
		for (i = 0; i < link_size; i++) {
			if (link_array[i] == part2_offset)
				return 1;
		}
	}
	return 0;
}


#ifndef __KERNEL__

__visible_for_testing void move_data_block(void *src_ptr, size_t size, int offset);

/*
 * Move the data block left or right related to current place
 * positive offset value moves it left (higher address),
 * negative offset value moves it right (lower address).
 */
__visible_for_testing void move_data_block(void *src_ptr, size_t size, int offset)
{
	unsigned char *dst_ptr = D_TREE_GET_PTR(src_ptr, offset);

	if (size != 0 && offset != 0)
		memmove(dst_ptr, src_ptr, size);
}

int d_tree_get_wildcard_offset(const char *src_ptr, size_t l)
{
	char c;
	const char *start_ptr;
	int digits, wildcard_size;
	size_t offset = 0;

	if (!l)
		l = strnlen(src_ptr, PATH_MAX);
	while ((start_ptr = memchr(src_ptr + offset, '[', l - offset)) != NULL) {
		offset = (size_t)(start_ptr - src_ptr);
		++start_ptr;
		if (*start_ptr != '*') {
			++offset;
			continue;
		}
		++start_ptr;
		digits = 0;
		wildcard_size = 0;
		while ((c = *start_ptr) != 0) {
			if (c == ']')
				if (digits && wildcard_size < 128)
					return (int)offset;
			if (isdigit(c)) {
				digits++;
				wildcard_size = wildcard_size * 10 + (c - '0');
			} else
				break;
			++start_ptr;
		}
		++offset;
	}
	return -1;

}

char *d_tree_pack_wildcard(const char *src_ptr, size_t l)
{
	char c;
	int  offset, wildcard_size;
	size_t part_size, block_size, out_offset = 0;
	static char work_str[PATH_MAX];

	while (l) {
		offset = d_tree_get_wildcard_offset(src_ptr, l);
		if (!offset) {
			src_ptr += 2;
			l -= 2;
			wildcard_size = 0;
			while (isdigit(c = *src_ptr)) {
				src_ptr++;
				l--;
				wildcard_size = wildcard_size * 10 + (c - '0');
			}
			src_ptr++;
			l--;
			if (out_offset >= (PATH_MAX - 2) || wildcard_size > 127)
				goto do_exit;
			work_str[out_offset++] = (char)((uint8_t)wildcard_size | 0x80);
		} else {
			part_size = (offset < 0) ? l : (size_t)offset;
			while (part_size) {
				block_size = (part_size > 127) ? 127 : part_size;
				if ((out_offset + block_size) >= (PATH_MAX - 2))
					goto do_exit;
				work_str[out_offset++] = (char)block_size;
				memcpy(work_str + out_offset, src_ptr, block_size);
				src_ptr += block_size;
				out_offset += block_size;
				l -= block_size;
				part_size -= block_size;
			}
		}
	}
do_exit:
	work_str[out_offset] = 0;
	return work_str;
}


void init_tree_data(enum d_tree_version version)
{
	memset(&rules_tree, 0, sizeof(rules_tree));
	rules_data = calloc(1024, 1024);
	rules_data_size = 1024 * 1024;

	text_data = calloc(1024, 1024);
	if (!text_data)
		return;
	text_data_size = 1024 * 1024;
	D_TREE_DWORD_REF(text_data, 0) = 4;

	integrity_data = calloc(1024, 1024);
	if (!integrity_data)
		return;
	integrity_data_size = 1024 * 1024;
	D_TREE_DWORD_REF(integrity_data, 0) = 4;

	bin_data = calloc(1024, 1024);
	if (!bin_data)
		return;
	bin_data_size = 1024 * 1024;
	D_TREE_DWORD_REF(bin_data, 0) = 4;

	rules_tree.version = version;
	rules_tree.data_size = D_TREE_V20_HDR_SIZE;
	rules_tree.data = rules_data;
	rules_tree.text_data = text_data;
	rules_tree.integrity_data = integrity_data;
	rules_tree.bin_data = bin_data;

	// create the root dir
	create_tree_item(NULL, 0);
}

void free_tree_data(void)
{
	unsigned int i;

	if (rules_data)
		free(rules_data);
	if (text_data)
		free(text_data);
	if (integrity_data)
		free(integrity_data);
	if (bin_data)
		free(bin_data);
	for (i = 0; i < d_tree_arr_count; i++)
		if (d_tree_arr[i])
			free(d_tree_arr[i]);
}

#ifdef __NEVER_DEFINED__
__visible_for_testing int data_realloc(unsigned char **data_ptr, int *data_size, int required_size)
{
	unsigned char *new_data;

	required_size = ((required_size + 1023) >> 10) << 10;
	if (*data_size < required_size) {
		required_size += (64 * 1024);
		new_data = realloc(*data_ptr, required_size);
		if (!new_data) {
			free_tree_data();
			printf("WARNING: Can not allocate the data buffer!\n");
			return -1;
		}
		*data_ptr = new_data;
		*data_size = required_size;
	}
	rules_tree.data = rules_data;
	rules_tree.text_data = text_data;
	rules_tree.integrity_data = integrity_data;
	rules_tree.bin_data = bin_data;
	return 0;
}
#endif

size_t d_tree_write_header(void *data, struct d_tree *the_tree)
{
	size_t offset = 0;

	if (the_tree->version == D_TREE_VERSION_20) {
		memcpy(D_TREE_GET_PTR(data, offset), dtree_magic_v20, sizeof(dtree_magic_v20));
		offset += sizeof(dtree_magic_v20);
		D_TREE_DWORD_REF(data, offset) = the_tree->data_size;
		offset += 4;
		D_TREE_DWORD_REF(data, offset) = the_tree->root_offset;
		offset += 4;
		D_TREE_DWORD_REF(data, offset) = the_tree->text_offset;
		offset += 4;
		D_TREE_WORD_REF(data, offset) = the_tree->text_count;
		offset += 2;
		D_TREE_DWORD_REF(data, offset) = the_tree->integrity_offset;
		offset += 4;
		D_TREE_WORD_REF(data, offset) = the_tree->integrity_count;
		offset += 2;
		D_TREE_DWORD_REF(data, offset) = the_tree->bin_offset;
		offset += 4;
		D_TREE_WORD_REF(data, offset) = the_tree->bin_count;
		offset += 2;
		if (offset != D_TREE_V20_HDR_SIZE)
			return 0;
	}
	return offset;
}

size_t d_tree_write_item_header(struct d_tree *the_tree, unsigned int offset,
		struct d_tree_item *item)
{
	unsigned int features;
	size_t field_offset = 0;
	void *item_ptr;

	if (!the_tree || !item)
		return field_offset;
	item_ptr = D_TREE_GET_PTR(the_tree->data, the_tree->root_offset + offset);

	if (the_tree->version != D_TREE_VERSION_20)
		return field_offset;

	D_TREE_DWORD_REF(item_ptr, field_offset) = item->features;
	features = item->features;
	field_offset += 4;
	if (features & d_tree_item_path) {
		D_TREE_WORD_REF(item_ptr, field_offset) = item->path_index;
		field_offset += 2;
	}
	if (features & d_tree_item_text) {
		D_TREE_WORD_REF(item_ptr, field_offset) = item->text_index;
		field_offset += 2;
	}
	if (features & d_tree_item_integrity) {
		D_TREE_WORD_REF(item_ptr, field_offset) = item->integrity_index;
		field_offset += 2;
	}
	if (features & d_tree_item_bin) {
		D_TREE_WORD_REF(item_ptr, field_offset) = item->bin_index;
		field_offset += 2;
	}
	if (features & d_tree_item_children) {
		D_TREE_WORD_REF(item_ptr, field_offset) = item->child_offset;
		field_offset += 2;
		D_TREE_WORD_REF(item_ptr, field_offset) = item->child_count;
		field_offset += 2;
		D_TREE_WORD_REF(item_ptr, field_offset) = item->child_size;
		field_offset += 2;
	}
	if (features & d_tree_item_linked) {
		D_TREE_DWORD_REF(item_ptr, field_offset) = item->linked_features;
		field_offset += 4;
		D_TREE_WORD_REF(item_ptr, field_offset) = item->link_index;
		field_offset += 2;
	}
	if (features & d_tree_item_dword) {
		D_TREE_DWORD_REF(item_ptr, field_offset) = item->data_dword;
		field_offset += 4;
	}
	if (features & d_tree_item_word) {
		D_TREE_WORD_REF(item_ptr, field_offset) = item->data_word;
		field_offset += 2;
	}
	if (features & d_tree_item_byte) {
		D_TREE_BYTE_REF(item_ptr, field_offset) = item->data_byte;
		field_offset += 1;
	}
	return field_offset;
}

unsigned short d_tree_calc_item_size(struct d_tree_item *item)
{
	unsigned int features;
	unsigned short field_offset = 0;

	if (!item)
		return field_offset;

	if (item->tree->version != D_TREE_VERSION_20)
		return field_offset;

	features = item->features;
	field_offset += 4;
	if (features & d_tree_item_path)
		field_offset += 2;
	if (features & d_tree_item_text)
		field_offset += 2;
	if (features & d_tree_item_integrity)
		field_offset += 2;
	if (features & d_tree_item_bin)
		field_offset += 2;
	if (features & d_tree_item_children)
		field_offset += 6;
	if (features & d_tree_item_linked)
		field_offset += 6;
	if (features & d_tree_item_dword)
		field_offset += 4;
	if (features & d_tree_item_word)
		field_offset += 2;
	if (features & d_tree_item_byte)
		field_offset += 1;
	item->item_size = field_offset;
	return field_offset;
}

unsigned short d_tree_calc_max_child_size(struct d_tree_item *base)
{
	unsigned short offset, size, max_size = 0;
	struct d_tree_item *item;

	if (base && base->child_count && base->child_offset) {
		item = d_tree_arr[base->child_offset];
		do {
			size = d_tree_calc_item_size(item);
			if (size > max_size)
				max_size = size;
			offset = item->next_item;
			if (offset)
				item = d_tree_arr[offset];
		} while (offset);
	}
	return max_size;
}

void d_tree_store_to_buffer(struct d_tree_item *base)
{
	struct d_tree_item *item;
	unsigned short offset, child_offset = 0;

	if (!base)
		return;
	if (base->child_count) {
		child_offset = base->child_offset;
		base->child_size = (unsigned short)d_tree_calc_max_child_size(base);
		base->child_offset = (unsigned short)packfiles_size;
	}
	d_tree_write_item_header(&rules_tree, base->item_offset, base);
	if (base->child_count) {
		item = d_tree_arr[child_offset];
		do {
			item->item_offset = (unsigned short)packfiles_size;
			packfiles_size += base->child_size;
			offset = item->next_item;
			if (offset)
				item = d_tree_arr[offset];
		} while (offset);
		item = d_tree_arr[child_offset];
		do {
			d_tree_store_to_buffer(item);
			offset = item->next_item;
			if (offset)
				item = d_tree_arr[offset];
		} while (offset);
	}
}

unsigned short add_table_item(enum d_tree_table_type t_type, struct d_tree *the_tree,
		const void *data, size_t data_size, unsigned short overwrite_index)
{
	unsigned char *tab_ptr = NULL;
	unsigned int i, index = 1, *table_size;
	unsigned int add_size = (unsigned short)data_size + 4;
	unsigned short item_size, item_offset, old_offset, *table_count = NULL, *table_header;
	unsigned char *new_item_hdr, *new_item_data;

	if (!data || !data_size)
		return 0;

	switch (t_type) {
	case d_tree_string_table:
		tab_ptr = D_TREE_GET_PTR(the_tree->text_data, the_tree->text_offset);
		table_count = &the_tree->text_count;
		break;
	case d_tree_integrity_table:
		tab_ptr = D_TREE_GET_PTR(the_tree->integrity_data, the_tree->integrity_offset);
		table_count = &the_tree->integrity_count;
		break;
	case d_tree_bin_table:
		tab_ptr = D_TREE_GET_PTR(the_tree->bin_data, the_tree->bin_offset);
		table_count = &the_tree->bin_count;
		break;
	default:
		return 0;
	}
	table_size = (unsigned int *)tab_ptr;
	table_header = (unsigned short *)tab_ptr;

	if (t_type == d_tree_string_table && *table_count) {
		for (index = 1; index <= *table_count; index++) {
			item_size = table_header[index * 2];
			item_offset = table_header[index * 2 + 1];
			if (data_size == item_size && !memcmp(tab_ptr + item_offset, data,
					data_size))
				return (unsigned short)index;
		}
	}

	if (overwrite_index && overwrite_index <= *table_count) {
		index = overwrite_index;
		item_size = table_header[index * 2];
		item_offset = table_header[index * 2 + 1];
		add_size = (unsigned int)data_size - item_size;
		new_item_data = (tab_ptr + item_offset);
		move_data_block(new_item_data + item_size, *table_size - (item_offset + item_size),
			(int)add_size);
		memcpy(new_item_data, data, data_size);
		table_header[index * 2] = (unsigned short)data_size;
		*table_size += add_size;
		for (i = 1; i <= *table_count; i++) {
			old_offset = table_header[i * 2 + 1];
			if (old_offset > item_offset)
				table_header[i * 2 + 1] = (unsigned short)(old_offset + add_size);
		}
		return (unsigned short)index;
	}

	*table_count += 1;
	index = *table_count;
	new_item_hdr = (tab_ptr + index * 4);
	new_item_data = (tab_ptr + *table_size + 4);
	move_data_block(new_item_hdr, *table_size - index * 4, 4);
	memcpy(new_item_data, data, data_size);
	table_header[index * 2] = (unsigned short)data_size;
	table_header[index * 2 + 1] = (unsigned short)(*table_size + 4);
	for (i = 1; i < index; i++)
		table_header[i * 2 + 1] += 4;
	*table_size += add_size;
	return (unsigned short)index;
}

struct d_tree_item *create_tree_item(const char *name, size_t l)
{
	struct d_tree_item *item;

	if (!name)
		l = 0;

	item = malloc(sizeof(struct d_tree_item));
	if (!item)
		return item;
	memset(item, 0, sizeof(struct d_tree_item));
	item->item_index = (unsigned short)d_tree_arr_count;
	item->tree = &rules_tree;
	item->path_index = add_table_item(d_tree_string_table, &rules_tree, name, l, 0);
	if (item->path_index)
		item->features |= d_tree_item_path;
	d_tree_arr[d_tree_arr_count] = item;
	d_tree_arr_count++;
	return item;
}

struct d_tree_item *add_tree_item(struct d_tree_item *base, const char *name, size_t l)
{
	struct d_tree_item *item, *new_item = NULL;
	enum feature_types_20 add_features = 0;

	if (!base)
		return new_item;

	if (d_tree_get_wildcard_offset(name, l) >= 0) {
		name = d_tree_pack_wildcard(name, l);
		l = strnlen(name, PATH_MAX);
		add_features = d_tree_item_wildcard;
	}
	new_item = create_tree_item(name, l);
	if (!new_item)
		return new_item;
	new_item->features |= add_features;
	if (!base->child_offset) {
		base->child_offset = new_item->item_index;
		base->features |= d_tree_item_children;
	} else {
		item = d_tree_arr[base->child_offset];
		while (item->next_item)
			item = d_tree_arr[item->next_item];
		item->next_item = new_item->item_index;
	}
	base->child_count++;
	return new_item;
}

struct d_tree_item *lookup_dir(struct d_tree_item *base, const char *name, size_t l,
		unsigned int for_recovery)
{
	struct d_tree_item *item = NULL;
	const char *item_text;
	unsigned int offset, item_text_size = 0;

	if (!base || !base->child_offset)
		return item;
	item = d_tree_arr[base->child_offset];
	if (d_tree_get_wildcard_offset(name, l) >= 0) {
		name = d_tree_pack_wildcard(name, l);
		l = strnlen(name, PATH_MAX);
	}
	do {
		item_text = d_tree_get_subpath(item, &item_text_size);
		if (!item_text)
			return NULL;
		if ((!(item->features & feature_is_file)
				|| (!!(item->features & feature_for_recovery)) == for_recovery)
				&& (size_t)item_text_size == l
				&& !memcmp(name, item_text, l))
			return item;
		offset = item->next_item;
		item = d_tree_arr[offset];
	} while (offset);
	return NULL;
}

struct d_tree_item *add_tree_path(const char *file_path, unsigned int for_recovery)
{
	const char *ptr, *next_separator;
	struct d_tree_item *base, *cur_item = NULL;
	size_t l;

	if (!file_path || *file_path != '/')
		return NULL;
	base = d_tree_arr[0];
	ptr = file_path + 1;
	do {
		next_separator = strchr(ptr, '/');
		if (!next_separator)
			l = strlen(ptr);
		else
			l = (size_t)(next_separator - ptr);
		if (!l)
			return NULL; /* two slashes in sequence */
		cur_item = lookup_dir(base, ptr, l, for_recovery);
		if (!cur_item) {
			cur_item = add_tree_item(base, ptr, l);
			if (!cur_item)
				return cur_item;
			/* slash wasn't found, it's a file */
			if (!next_separator) {
				cur_item->features |= feature_is_file;
				if (for_recovery)
					cur_item->features |= feature_for_recovery;
			}
		}
		base = cur_item;
		ptr += l;
		if (next_separator)
			ptr++;
	} while (*ptr);
	return cur_item;
}

void d_tree_add_link(struct d_tree_item *item, unsigned short offset, unsigned int feature)
{
	const unsigned short *old_link_array = NULL;
	unsigned short *new_link_array;
	unsigned int i, link_size = 0, is_exist = 0;

	feature &= (feature_immutable_src_exception | feature_immutable_dst_exception
		| feature_immutable_root | feature_immutable_root_v2);
	old_link_array = (const unsigned short *)d_tree_get_table_data(d_tree_bin_table, &rules_tree,
		item->link_index, &link_size);
	new_link_array = malloc(link_size + 16);
	if (!new_link_array)
		return;
	if (old_link_array) {
		for (i = 0; i < (link_size >> 1); i++)
			if (old_link_array[i] == offset) {
				is_exist = 1;
				free(new_link_array);
				break;
			}
		if (is_exist == 1) {
			item->linked_features |= feature;
			return;
		}
		memcpy(new_link_array, old_link_array, link_size);
	}
	new_link_array[link_size >> 1] = offset;
	link_size += 2;
	item->link_index = add_table_item(d_tree_bin_table, &rules_tree, new_link_array,
		link_size, item->link_index);
	item->features |= d_tree_item_linked;
	item->linked_features |= feature;
	free(new_link_array);
}

#endif /* __KERNEL__ */
