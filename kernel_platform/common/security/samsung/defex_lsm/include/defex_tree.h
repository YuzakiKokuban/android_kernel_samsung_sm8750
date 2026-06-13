/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef __DEFEX_TREE_H
#define __DEFEX_TREE_H


#define D_TREE_GET_PTR(base, offset)	(((unsigned char *)(base)) + (offset))
#define D_TREE_BYTE_REF(base, offset)	(*(D_TREE_GET_PTR(base, offset)))
#define D_TREE_WORD_REF(base, offset)	(*(unsigned short int *)(D_TREE_GET_PTR(base, offset)))
#define D_TREE_DWORD_REF(base, offset)	(*(unsigned int *)(D_TREE_GET_PTR(base, offset)))

#define D_TREE_V10_HDR_SIZE	(sizeof(struct rule_item_struct) + sizeof(dtree_magic_v10))

enum d_tree_version {
	D_TREE_VERSION_INVALID = 0,
	D_TREE_VERSION_10 = 1,
	D_TREE_VERSION_20 = 2
};

enum d_tree_table_type {
	d_tree_string_table = 0,
	d_tree_integrity_table,
	d_tree_bin_table
};


/*
 * Defex tree header for v1.0
 * [0][2]:next_level - header size
 * [2][4]:data_size [next_file, feature_type]
 * [6][1]:string_length == 16
 * [7][32]:integrity[INTEGRITY_LENGTH]
 * [][n]:string == "DEFEX_RULES_FILE"
 */

// Defex tree header for v2.0
enum d_tree_hdr_offset {
// [0][8]:version == "DEFEX2.0"
	D_TREE_HDR_VERSION = 0,
// [8][4]:data_size
	D_TREE_HDR_DATA_SIZE = 8,
// [12][4]:root_offset
	D_TREE_HDR_ROOT_OFFSET = 12,
// [16][4]:text_offset
	D_TREE_HDR_TEXT_OFFSET = 16,
// [20][2]:text_count
	D_TREE_HDR_TEXT_COUNT = 20,
// [22][4]:integrity_offset
	D_TREE_HDR_INTEGRITY_OFFSET = 22,
// [26][2]:integrity_count
	D_TREE_HDR_INTEGRITY_COUNT = 26,
// [28][4]:bin_offset
	D_TREE_HDR_BIN_OFFSET = 28,
// [32][2]:bin_count
	D_TREE_HDR_BIN_COUNT = 32,
// total: 34 bytes
	D_TREE_V20_HDR_SIZE = 34
};

/*
 * String library
 * 1. offset and length table
 * [4]:size - total size of offsets and strings
 * [2]:length, [2]:offset from beginning of library (index1)
 * ...
 *
 * 2. Strings without trailing \0 endings
 */

#define active_feature_mask	((1 << 20) - 1)

enum feature_types_20 {
	/* version 1.0 */
	feature_is_file = (1 << 0),
	feature_for_recovery = (1 << 1),
	feature_ped_path = (1 << 2),
	feature_ped_exception = (1 << 3),
	feature_ped_status = (1 << 4),
	feature_safeplace_path = (1 << 5),
	feature_safeplace_status = (1 << 6),
	feature_immutable_path_open = (1 << 7),
	feature_immutable_path_write = (1 << 8),
	feature_immutable_src_exception = (1 << 9),
	feature_immutable_status = (1 << 10),
	feature_umhbin_path = (1 << 11),
	feature_trusted_map_status = (1 << 12),
	feature_integrity_check = (1 << 13),
	feature_immutable_dst_exception = (1 << 14),
	feature_immutable_root = (1 << 15),
	feature_immutable_root_status = (1 << 16),

	/* version 2.0 */
	d_tree_item_path = (1UL << 31),
	d_tree_item_text = (1 << 30),
	d_tree_item_bin = (1 << 29),
	d_tree_item_children = (1 << 28),
	d_tree_item_integrity = (1 << 27),
	d_tree_item_linked = (1 << 26),
	d_tree_item_byte = (1 << 25),
	d_tree_item_word = (1 << 24),
	d_tree_item_dword = (1 << 23),
};


struct d_tree {
	unsigned char *data;
	unsigned short version;
	unsigned int data_size;

	unsigned int root_offset;

	unsigned char *text_data;
	unsigned int text_offset;
	unsigned short text_count;

	unsigned char *integrity_data;
	unsigned int integrity_offset;
	unsigned short integrity_count;

	unsigned char *bin_data;
	unsigned int bin_offset;
	unsigned short bin_count;
};


struct d_tree_item {
	struct d_tree *tree;
	struct rule_item_struct *item_v10;
// ---- fields stored in the binary format
	unsigned int features;
// ---- depends on d_tree_item_path
	unsigned short path_index;
// ---- depends on d_tree_item_text
	unsigned short text_index;
// ---- depends on d_tree_item_integrity
	unsigned short integrity_index;
// ---- depends on d_tree_item_bin
	unsigned short bin_index;
// ---- depends on d_tree_item_children
	unsigned short child_offset;
	unsigned short child_count;
	unsigned short child_size;
// ---- depends on d_tree_item_linked
	unsigned int linked_features;
	unsigned short link_index;
// ---- depends on d_tree_item_dword
	unsigned int data_dword;
// ---- depends on d_tree_item_word
	unsigned short data_word;
// ---- depends on d_tree_item_byte
	unsigned char data_byte;

// ---- search helper fields
	union {
		unsigned short child_index;
		unsigned short next_item;
	};
	unsigned short item_offset;
	unsigned short item_index;
	unsigned short item_size;
};


struct d_tree_ctx {
	unsigned int feature_flags;
	unsigned int lookup_flags;
	struct d_tree *tree;
	unsigned int item_offset;
	unsigned int linked_offset;
};

unsigned short d_tree_get_version(void *data, size_t data_size);
int d_tree_get_header(void *data, size_t data_size, struct d_tree *the_tree);
int d_tree_get_item_header(struct d_tree *the_tree, unsigned int offset,
		struct d_tree_item *item);
const char *d_tree_get_table_data(enum d_tree_table_type t_type, struct d_tree *the_tree,
		unsigned int index, unsigned int *data_size);
const char *d_tree_get_subpath(struct d_tree_item *item, unsigned int *subpath_size);
const char *d_tree_get_text(struct d_tree_item *item, unsigned int *text_size);
const unsigned char *d_tree_get_integrity(struct d_tree_item *item, unsigned int *data_size);
const unsigned char *d_tree_get_bin(struct d_tree_item *item, unsigned int index,
		unsigned int *data_size);
struct d_tree_item *d_tree_lookup_dir_init(struct d_tree_item *parent_item,
	struct d_tree_item *child_item);
struct d_tree_item *d_tree_next_child(struct d_tree_item *parent_item,
	struct d_tree_item *child_item);
unsigned int d_tree_lookup_dir(struct d_tree_ctx *ctx, struct d_tree_item *parent_item,
		const char *name, size_t l);
void d_tree_lookup_init(struct d_tree *the_tree, struct d_tree_ctx *ctx,
		unsigned int starting_offset, unsigned int feature_flags,
			unsigned int lookup_flags);
int d_tree_lookup_path(struct d_tree_ctx *ctx, const char *file_path,
		struct d_tree_item *found_item);
int d_tree_check_linked_rules(struct d_tree_item *part1_item, int part2_offset,
		unsigned int feature);


#if !defined(__KERNEL__) && defined(__PACK_RULES20)

extern struct d_tree rules_tree;
extern unsigned char *rules_data;
extern int rules_data_size;
extern unsigned char *text_data;
extern int text_data_size;
extern unsigned char *integrity_data;
int integrity_data_size;
extern unsigned char *bin_data;
extern int bin_data_size;
extern unsigned char *work_bin_arr[65000];
extern unsigned short work_bin_sizes[65000];
extern unsigned int work_bin_arr_count;
extern struct d_tree_item work_item;
extern struct d_tree_item *d_tree_arr[65000];
extern unsigned int d_tree_arr_count;
extern unsigned int packfiles_size;


void init_tree_data(enum d_tree_version version);
void free_tree_data(void);
size_t d_tree_write_header(void *data, struct d_tree *the_tree);
unsigned short d_tree_calc_item_size(struct d_tree_item *item);
unsigned short d_tree_calc_max_child_size(struct d_tree_item *base);
size_t d_tree_write_item_header(struct d_tree *the_tree, unsigned int offset,
		struct d_tree_item *item);
void d_tree_store_to_buffer(struct d_tree_item *base);
unsigned short add_table_item(enum d_tree_table_type t_type, struct d_tree *the_tree,
		const void *data, size_t data_size, unsigned short overwrite_index);
struct d_tree_item *create_tree_item(const char *name, size_t l);
struct d_tree_item *add_tree_item(struct d_tree_item *base, const char *name, size_t l);
struct d_tree_item *lookup_dir(struct d_tree_item *base, const char *name, size_t l,
		unsigned int for_recovery);
int lookup_tree(const char *file_path, unsigned int attribute, unsigned int for_recovery);
struct d_tree_item *add_tree_path(const char *file_path, unsigned int for_recovery);
void d_tree_add_link(struct d_tree_item *item, unsigned short offset, unsigned int feature);
void add_rule(const char *part1, const char *part2, unsigned int for_recovery,
		unsigned int feature, const char *integrity);


#endif /* __KERNEL__ */

#endif /* __DEFEX_TREE_H */
