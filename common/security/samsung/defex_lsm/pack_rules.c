// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#define __PACK_RULES20

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "include/defex_rules.h"
#include "include/defex_tree.h"

#define SAFE_STRCOPY(dst, src) do { memccpy(dst, src, 0, sizeof(dst)); dst[sizeof(dst) - 1] = 0; \
				} while (0)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif /* ARRAY_SIZE */

const struct feature_match_entry feature_match[] = {
	{"feature_safeplace_path", feature_safeplace_path},
	{"feature_ped_exception", feature_ped_exception},
	{"feature_immutable_path_open", feature_immutable_path_open},
	{"feature_immutable_path_write", feature_immutable_path_write},
	{"feature_immutable_root", feature_immutable_root},
	{"feature_immutable_src_exception", feature_immutable_src_exception},
	{"feature_immutable_dst_exception", feature_immutable_dst_exception},
	{"feature_umhbin_path", feature_umhbin_path},
	{"feature_integrity_check", feature_integrity_check},
};

struct file_list_item {
	char file_name[PATH_MAX];
#ifdef DEFEX_INTEGRITY_ENABLE
	char integrity[INTEGRITY_LENGTH * 2 + 1];
#endif /* DEFEX_INTEGRITY_ENABLE */
	unsigned int features;
	int is_recovery;
};

unsigned int packfiles_size;

struct file_list_item *file_list;
int file_list_count;

#ifndef DEFEX_DEBUG_ENABLE
int debug_ifdef_is_active;
void process_debug_ifdef(const char *src_str);
#endif

/* Show rules vars */
const char header_name[16] = { "DEFEX_RULES_FILE" };
static char work_path[512];
static unsigned int global_data_size;
struct d_tree extern_tree;

struct d_tree rules_tree;
unsigned char *rules_data;
int rules_data_size;

unsigned char *text_data;
int text_data_size;

unsigned char *integrity_data;
int integrity_data_size;

unsigned char *bin_data;
int bin_data_size;

struct d_tree_item work_item;

struct d_tree_item *d_tree_arr[65000];
unsigned int d_tree_arr_count;


void add_rule(const char *part1, const char *part2, unsigned int for_recovery,
	unsigned int feature, const char *integrity);
void addline2tree(char *src_line, unsigned int feature);
char *extract_rule_text(const char *src_line);
int store_tree(FILE *f, FILE *f_bin);

#ifdef DEFEX_INTEGRITY_ENABLE
/* Transfer string to hex */
char null_integrity[INTEGRITY_LENGTH * 2 + 1];
unsigned char byte_hex_to_bin(char input);
int string_hex_to_bin(const char *input, size_t inputLen, unsigned char *output);
#endif /* DEFEX_INTEGRITY_ENABLE */

/* Suplementary functions for reducing rules */
unsigned int str_to_feature(const char *str);
int remove_substr(char *str, const char *part);
void trim_cr_lf(char *str);
char *remove_redundant_chars(char *str);
int check_path_in_use(const char *path);
int load_file_list(const char *name);
int lookup_file_list(const char *rule, int for_recovery);

/* Suplementary functions for showing rules */
void feature_to_str(char *str, unsigned int flags);
int parse_items(struct d_tree_item *base, size_t path_length, int level);
int defex_show_structure(void *packed_rules, size_t rules_size);

/* Main processing functions */
int reduce_rules(const char *source_rules_file, const char *reduced_rules_file,
		const char *list_file);
int pack_rules(const char *source_rules_file, const char *packed_rules_file,
		const char *packed_rules_binfile);
int parse_packed_bin_file(const char *source_bin_file);

#ifndef __visible_for_testing
#define __visible_for_testing static
#endif

#include "core/defex_tree.c"

unsigned int str_to_feature(const char *str)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(feature_match); i++) {
		if (strstr(str, feature_match[i].feature_name))
			return feature_match[i].feature_num;
	}
	return 0;
}

void feature_to_str(char *str, unsigned int flags)
{
	size_t i;

	str[0] = 0;
	for (i = 0; i < ARRAY_SIZE(feature_match); i++)
		if (flags & feature_match[i].feature_num) {
			if (str[0])
				strcat(str, ", ");
			strcat(str, feature_match[i].feature_name);
		}
	if (flags & feature_for_recovery) {
		if (str[0])
			strcat(str, ", ");
		strcat(str, "feature_for_recovery");
	}
}

int remove_substr(char *str, const char *part)
{
	size_t l, part_l;
	char *ptr;

	l = strnlen(str, PATH_MAX - 1);
	ptr = strstr(str, part);
	if (ptr) {
		part_l = strnlen(part, 32) - 1;
		memmove(ptr, ptr + part_l, l - (size_t)(ptr - str) - part_l + 1);
	}
	return (ptr != NULL);
}

void trim_cr_lf(char *str)
{
	char *ptr;
	/* remove CR or LF at the end */
	ptr = strchr(str, '\r');
	if (ptr)
		*ptr = 0;
	ptr = strchr(str, '\n');
	if (ptr)
		*ptr = 0;
}

char *remove_redundant_chars(char *str)
{
	size_t l;

	/* skip hash values in the begin */
	str += 65;
	trim_cr_lf(str);
	l = strnlen(str, (size_t)PATH_MAX - 1);
	/* remove starting dot or space */
	while (l && (*str == '.' || *str == ' '))
		str++;
	return str;
}

int check_path_in_use(const char *path)
{
	size_t i;
	static const char * const path_list[] = {
		"/root/",
		"/product",
		"/recovery/",
		"/system/",
		"/tmp/",
		"/vendor/",
#if defined(DEFEX_FACTORY_ENABLE)
		"/data/",
#endif
		"/apex/",
		"/system_ext/",
		"/postinstall/"
	};

	for (i = 0; i < ARRAY_SIZE(path_list); i++) {
		if (!strncmp(path, path_list[i], strnlen(path_list[i], PATH_MAX)))
			return 1;
	}
	return 0;
}

char *extract_rule_text(const char *src_line)
{
	char *start_ptr, *end_ptr;

	start_ptr = strchr(src_line, '\"');
	if (start_ptr) {
		start_ptr++;
		end_ptr = strchr(start_ptr, '\"');
		if (end_ptr) {
			*end_ptr = 0;
			return start_ptr;
		}
	}
	return NULL;
}

#ifdef DEFEX_INTEGRITY_ENABLE
unsigned char byte_hex_to_bin(char input)
{
	if (input >= 0x30 && input <= 0x39)
		return (unsigned char)input - 0x30;
	else if (input >= 0x41 && input <= 0x46)
		return (unsigned char)input - 0x37;
	else if (input >= 0x61 && input <= 0x66)
		return (unsigned char)input - 0x57;
	return 0xFF;
}

int string_hex_to_bin(const char *input, size_t inputLen, unsigned char *output)
{
	unsigned char convert1, convert2;
	char c;
	const char *ptr;
	size_t i, retry_count = 16;

	if (!input || !output)
		return 0;

	/* Check input is a paired value. */
	if (inputLen % 2 != 0)
		return 0;

	/* divide length by 2 */
	inputLen >>= 1;
	memset(output, 0, inputLen);

	do {
		ptr = input;
		/* Convert hex code to bin. */
		for (i = 0; i < inputLen; i++) {
			c = *ptr++;
			if (!c)
				return 0;
			convert1 = byte_hex_to_bin(c);
			c = *ptr++;
			if (!c)
				return 0;
			convert2 = byte_hex_to_bin(c);
			if (convert1 == 0xFF || convert2 == 0xFF)
				return 0;
			output[i] = (unsigned char)((convert1 << 4) | convert2);
		}
		if (i == inputLen)
			return (int)inputLen;
		input++;
	} while (--retry_count);
	return 0;
}
#endif /* DEFEX_INTEGRITY_ENABLE */

void add_rule(const char *part1, const char *part2, unsigned int for_recovery,
		unsigned int feature, const char *integrity)
{
	struct d_tree_item *item_part1 = NULL, *item_part2 = NULL;
	unsigned int part2_feature = 0;
#ifdef DEFEX_INTEGRITY_ENABLE
	int index;
	unsigned char tmp_integrity[INTEGRITY_LENGTH];
#endif /* DEFEX_INTEGRITY_ENABLE */

	(void)integrity;

	/* Define the feature value for second part of the rule */
	if (feature & feature_immutable_src_exception)
		part2_feature |= feature_immutable_dst_exception;
	if (feature & feature_immutable_root)
		part2_feature |= feature_immutable_root;

	if (part1) {
		item_part1 = add_tree_path(part1, for_recovery);
		if (item_part1)
			item_part1->features |= feature;
	}
	if (part2) {
		item_part2 = add_tree_path(part2, 0);
		if (item_part2)
			item_part2->features |= part2_feature;
	}
	/* add the reference from part1 to part2 */
	if (item_part1 && item_part2) {
		d_tree_add_link(item_part1, item_part2->item_index, feature);
		d_tree_add_link(item_part2, item_part1->item_index, part2_feature);
	}

#ifdef DEFEX_INTEGRITY_ENABLE
	if (integrity && item_part1) {
		if (string_hex_to_bin(integrity + 1, INTEGRITY_LENGTH * 2, tmp_integrity)
				== INTEGRITY_LENGTH) {
			index = add_table_item(d_tree_integrity_table, &rules_tree,
				tmp_integrity, INTEGRITY_LENGTH, 0);
			if (index) {
				item_part1->features |= d_tree_item_integrity;
				//item_part1->features |= feature_integrity_check;
				item_part1->integrity_index = (unsigned short)index;
			}
		}
	}
#endif /* DEFEX_INTEGRITY_ENABLE */

}

void addline2tree(char *src_line, unsigned int feature)
{
	char *part1, *part2;
	char *n_sign = NULL, *r_sign = NULL;
	char *integrity;

	(void)integrity;
	/* extract the first part */
	part1 = extract_rule_text(src_line);
	if (!part1)
		return;
	/* extract the second part after ':' symbol (immutable_dst_exception rule) */
	part2 = strchr(part1, ':');
	if (part2) {
		*part2 = 0;
		++part2;
	}

#ifdef DEFEX_INTEGRITY_ENABLE
	integrity = strchr((part2) ? part2 : part1, '\0') + 1;
	integrity = extract_rule_text(integrity);
	if (integrity) {
		n_sign = strchr(integrity, 'N');
		r_sign = strchr(integrity, 'R');
	}
#endif /* DEFEX_INTEGRITY_ENABLE */

	if (n_sign || !r_sign)
		add_rule(part1, part2, 0, feature, n_sign);

	if (r_sign)
		add_rule(part1, part2, 1, feature, r_sign);
}

int store_tree(FILE *f, FILE *f_bin)
{
	unsigned char *ptr = (unsigned char *)rules_data;
	static char work_str[4096];
	unsigned int i, j, offset = 0, index = 0;
	unsigned int table_size;
	struct d_tree_item *item, *root_item = d_tree_arr[0];
	unsigned short *link_array;
	unsigned int link_size = 0;

	packfiles_size = 0;
	rules_tree.root_offset = (unsigned int)d_tree_write_header(rules_data, &rules_tree);
	root_item->item_offset = 0;
	root_item->item_size = d_tree_calc_item_size(root_item);
	packfiles_size += root_item->item_size;
	d_tree_store_to_buffer(root_item);

	rules_tree.data_size = packfiles_size + rules_tree.root_offset;

	if (rules_tree.text_count) {
		rules_tree.text_offset = rules_tree.data_size;
		table_size = D_TREE_DWORD_REF(text_data, 0);
		rules_tree.data_size += table_size;
		rules_tree.text_data = rules_tree.data;
		memcpy(D_TREE_GET_PTR(rules_data, rules_tree.text_offset), text_data, table_size);
	}

	if (rules_tree.integrity_count) {
		rules_tree.integrity_offset = rules_tree.data_size;
		table_size = D_TREE_DWORD_REF(integrity_data, 0);
		rules_tree.data_size += table_size;
		rules_tree.integrity_data = rules_tree.data;
		memcpy(D_TREE_GET_PTR(rules_data, rules_tree.integrity_offset),
			integrity_data, table_size);
	}

	if (rules_tree.bin_count) {

		for (i = 1; i <= d_tree_arr_count; i++) {
			item = d_tree_arr[i];
			if (item && (item->features & d_tree_item_linked) && item->link_index) {
				link_array =
					(unsigned short *)d_tree_get_table_data(d_tree_bin_table,
					&rules_tree, item->link_index, &link_size);
				if (link_array && link_size) {
					for (j = 0; j < (link_size / 2); j++) {
						link_array[j] =
							d_tree_arr[link_array[j]]->item_offset;
					}
				}
			}
		}

		rules_tree.bin_offset = rules_tree.data_size;
		table_size = D_TREE_DWORD_REF(bin_data, 0);
		rules_tree.data_size += table_size;
		rules_tree.bin_data = rules_tree.data;
		memcpy(D_TREE_GET_PTR(rules_data, rules_tree.bin_offset), bin_data, table_size);
	}
	d_tree_write_header(rules_data, &rules_tree);

	work_str[0] = 0;
	fprintf(f, "#ifndef DEFEX_RAMDISK_ENABLE\n\n");
	fprintf(f, "const unsigned char defex_packed_rules[] = {\n");
	for (i = 0; i < rules_tree.data_size; i++) {
		if (index)
			offset += (unsigned int)snprintf(work_str + offset,
				sizeof(work_str) - offset, ", ");
		offset += (unsigned int)snprintf(work_str + offset, sizeof(work_str) - offset,
			"0x%02x", ptr[i]);
		index++;
		if (index == 16) {
			fprintf(f, "\t%s,\n", work_str);
			index = 0;
			offset = 0;
		}
	}
	if (index)
		fprintf(f, "\t%s\n", work_str);
	fprintf(f, "};\n");
	fprintf(f, "\n#endif /* DEFEX_RAMDISK_ENABLE */\n\n");
	fprintf(f, "#define DEFEX_RULES_ARRAY_SIZE\t\t%d\n", packfiles_size);
	if (f_bin)
		fwrite(rules_data, 1, rules_tree.data_size, f_bin);
	return 0;
}

int load_file_list(const char *name)
{
	int found;
	char *str;
	FILE *lst_file = NULL;
	struct file_list_item *file_list_new;
	static char work_str[PATH_MAX * 2];

	lst_file = fopen(name, "r");
	if (!lst_file)
		return -1;

	while (!feof(lst_file)) {
		if (!fgets(work_str, sizeof(work_str), lst_file))
			break;
		str = remove_redundant_chars(work_str);
		if (*str == '/' && check_path_in_use(str)) {
			remove_substr(str, "/root/");
			found = remove_substr(str, "/recovery/");
			file_list_count++;
			file_list_new = realloc(file_list,
				sizeof(struct file_list_item) * (size_t)file_list_count);
			if (!file_list_new) {
				free(file_list);
				printf("WARNING: Can not allocate the filelist item!\n");
				exit(-1);
			}
			file_list = file_list_new;
#ifdef DEFEX_INTEGRITY_ENABLE
			SAFE_STRCOPY(file_list[file_list_count - 1].integrity, work_str);
#endif /* DEFEX_INTEGRITY_ENABLE */
			SAFE_STRCOPY(file_list[file_list_count - 1].file_name, str);
			file_list[file_list_count - 1].is_recovery = found;
		}
	}
	fclose(lst_file);
	return 0;
}

int lookup_file_list(const char *rule, int for_recovery)
{
	int i;
	size_t l;

	l = strnlen(rule, PATH_MAX);
	if (l < 2)
		return -1;
	for (i = 0; i < file_list_count; i++) {
		if (file_list[i].is_recovery == for_recovery
			&& !strncmp(file_list[i].file_name, rule, l)
			&& !strncmp(file_list[i].file_name, rule, strnlen(file_list[i].file_name,
				PATH_MAX)))
			return i;
	}
	return -1;
}

#ifndef DEFEX_DEBUG_ENABLE
void process_debug_ifdef(const char *src_str)
{
	char *ptr;

	ptr = strstr(src_str, "#ifdef DEFEX_DEBUG_ENABLE");
	if (ptr) {
		while (ptr > src_str) {
			ptr--;
			if (*ptr != ' ' && *ptr != '\t')
				return;
		}
		debug_ifdef_is_active = 1;
		return;
	}
	ptr = strstr(src_str, "#endif");
	if (ptr && debug_ifdef_is_active) {
		while (ptr > src_str) {
			ptr--;
			if (*ptr != ' ' && *ptr != '\t')
				return;
		}
		debug_ifdef_is_active = 0;
		return;
	}
}
#endif

int reduce_rules(const char *source_rules_file, const char *reduced_rules_file,
		const char *list_file)
{
	int ret_val = -1;
	int found_normal = -1, found_recovery = -1;
	char *rule, *colon_ptr;
	static char work_str[PATH_MAX * 2], tmp_str[PATH_MAX * 2], rule_part1[PATH_MAX * 2];
	FILE *src_file = NULL, *dst_file = NULL;
#ifdef DEFEX_INTEGRITY_ENABLE
	char *line_end, *integrity_normal;
	char *integrity_recovery;
#endif /* DEFEX_INTEGRITY_ENABLE */

	src_file = fopen(source_rules_file, "r");
	if (!src_file)
		return -1;
	dst_file = fopen(reduced_rules_file, "wt");
	if (!dst_file)
		goto do_close2;

	if (load_file_list(list_file) != 0)
		goto do_close1;

#ifdef DEFEX_INTEGRITY_ENABLE
	memset(null_integrity, '0', sizeof(null_integrity) - 1);
	null_integrity[sizeof(null_integrity) - 1] = 0;
#endif /* DEFEX_INTEGRITY_ENABLE */

	while (!feof(src_file)) {
		if (!fgets(work_str, sizeof(work_str), src_file))
			break;

		if (str_to_feature(work_str)) {
			trim_cr_lf(work_str);
			SAFE_STRCOPY(tmp_str, work_str);
			rule = extract_rule_text(tmp_str);
			SAFE_STRCOPY(rule_part1, rule);
			colon_ptr = strchr(rule_part1, ':');
			if (colon_ptr)
				*colon_ptr = 0;
			found_normal = lookup_file_list(rule_part1, 0);
			found_recovery = lookup_file_list(rule_part1, 1);
			if (rule && !found_normal && !found_recovery && !strstr(work_str,
					"/* DEFAULT */")) {
				printf("removed rule: %s\n", rule);
				continue;
			}
#ifdef DEFEX_INTEGRITY_ENABLE
			integrity_normal = null_integrity;
			integrity_recovery = null_integrity;
			if (found_normal >= 0)
				integrity_normal = file_list[found_normal].integrity;
			if (found_recovery >= 0)
				integrity_recovery = file_list[found_recovery].integrity;

			line_end = strstr(work_str, "},");
			if (line_end) {
				*line_end = 0;
				line_end += 2;
			}

			/* Add hash value after each file path */
			if (found_normal >= 0 || (found_normal < 0 && found_recovery < 0))
				printf("remained rule: %s, %s %s\n", rule,
					integrity_normal, (line_end != NULL) ? line_end : "");
			if (found_recovery >= 0)
				printf("remained rule: %s, %s %s (R)\n",
					rule, integrity_recovery,
					(line_end != NULL) ? line_end : "");

			fprintf(dst_file, "%s,\"", work_str);
			if (found_normal >= 0)
				fprintf(dst_file, "N%s", integrity_normal);
			if (found_recovery >= 0)
				fprintf(dst_file, "R%s", integrity_recovery);

			fprintf(dst_file, "\"}, %s\n", (line_end != NULL) ? line_end : "");

#else
			printf("remained rule: %s\n", work_str);
			fputs(work_str, dst_file);
			fputs("\n", dst_file);
#endif /* DEFEX_INTEGRITY_ENABLE */
		} else
			fputs(work_str, dst_file);
	}
	ret_val = 0;
do_close1:
	fclose(dst_file);
do_close2:
	fclose(src_file);
	return ret_val;
}

int pack_rules(const char *source_rules_file, const char *packed_rules_file,
		const char *packed_rules_binfile)
{
	int ret_val = -1;
	unsigned int feature;
	FILE *src_file = NULL, *dst_file = NULL, *dst_binfile = NULL;
	static char work_str[PATH_MAX*2];

	src_file = fopen(source_rules_file, "r");
	if (!src_file) {
		printf("Failed to open %s, %s\n", source_rules_file, strerror(errno));
		return -1;
	}
	dst_file = fopen(packed_rules_file, "wt");
	if (!dst_file) {
		printf("Failed to open %s, %s\n", packed_rules_file, strerror(errno));
		goto do_close2;
	}
	if (packed_rules_binfile) {
		dst_binfile = fopen(packed_rules_binfile, "wb");
		if (!dst_binfile)
			printf("Failed to open %s, %s - Ignore\n", packed_rules_binfile,
				strerror(errno));
	}

	while (!feof(src_file)) {
		if (!fgets(work_str, sizeof(work_str), src_file))
			break;
#ifndef DEFEX_DEBUG_ENABLE
		process_debug_ifdef(work_str);
		if (!debug_ifdef_is_active) {
#endif
			feature = str_to_feature(work_str);
			if (feature) {
				addline2tree(work_str, feature);
				continue;
		}
#ifndef DEFEX_DEBUG_ENABLE
		}
#endif
	}
	store_tree(dst_file, dst_binfile);
	if (d_tree_arr_count < 2)
		printf("WARNING: Defex packed rules tree is empty!\n");
	ret_val = 0;
	if (dst_binfile)
		fclose(dst_binfile);
	fclose(dst_file);
do_close2:
	fclose(src_file);
	return ret_val;
}

int parse_items(struct d_tree_item *base, size_t path_length, int level)
{
	struct d_tree_item tmp_item, *child_item;
	const char *subdir_ptr;
	unsigned int subdir_size;
	static char feature_list[128];
	int err, ret = 0;

	if (level > 8) {
		printf("Level is too deep\n");
		return -1;
	}
	if (path_length > (sizeof(work_path) - 128)) {
		printf("Work path is too long\n");
		return -1;
	}

	child_item = d_tree_lookup_dir_init(base, &tmp_item);

	while (child_item) {
		subdir_size = 0;
		subdir_ptr = d_tree_get_subpath(child_item, &subdir_size);
		if (subdir_ptr)
			memcpy(work_path + path_length, subdir_ptr, (size_t)subdir_size);
		subdir_size += (unsigned int)path_length;
		work_path[subdir_size] = 0;

		if ((child_item->features & d_tree_item_children) && child_item->child_count) {
			if (child_item->features & active_feature_mask) {
				feature_to_str(feature_list, child_item->features);
				printf("%s%c - %s\n", work_path,
					((child_item->features & feature_is_file)?' ':'/'),
					feature_list);
			}
			work_path[subdir_size++] = '/';
			work_path[subdir_size] = 0;
			err = parse_items(child_item, (size_t)subdir_size, level + 1);
			if (err != 0)
				return err;

		} else {
			feature_to_str(feature_list, child_item->features);
			printf("%s%c%s - %s\n", work_path,
				((child_item->features & feature_is_file) ? ' ' : '/'), "",
				feature_list);
		}
		work_path[path_length] = 0;
		child_item = d_tree_next_child(base, child_item);
	}
	return ret;
}

int defex_show_structure(void *packed_rules, size_t rules_size)
{
	struct d_tree_item base;
	int res;

	work_path[0] = '/';
	work_path[1] = 0;

	if (d_tree_get_header(packed_rules, rules_size, &extern_tree)) {
		printf("ERROR: Unknown file version!\n");
		return -1;
	}

	packfiles_size = (unsigned int)rules_size;
	global_data_size = extern_tree.data_size;

	printf("Rules binary size: %d\n", packfiles_size);
	printf("Rules internal data size: %d\n", global_data_size);

	if (global_data_size > packfiles_size)
		printf("WARNING: Internal size is bigger than binary size, possible"
			" structure error!\n");

	printf("File List:\n");
	if (!d_tree_get_item_header(&extern_tree, 0, &base))
		return -1;

	res = parse_items(&base, 1, 1);
	printf("== End of File List ==\n");
	return res;
}

int parse_packed_bin_file(const char *source_bin_file)
{
	struct stat sb;
	FILE *policy_file = NULL;
	size_t policy_size, readed;
	unsigned char *policy_data = NULL;

	if (stat(source_bin_file, &sb) == -1) {
		perror("Error");
		return -1;
	}

	policy_size = (size_t)sb.st_size;

	printf("Try to parse file: %s\n", source_bin_file);

	policy_file = fopen(source_bin_file, "rb");
	if (policy_file == NULL) {
		perror("Error");
		return -1;
	}

	policy_data = malloc(policy_size);

	if (policy_data == NULL) {
		perror("Error");
		goto exit;
	}

	readed = fread(policy_data, 1, policy_size, policy_file);
	if (readed != policy_size) {
		printf("Read Error: readed %d bytes", (int)readed);
		goto exit;
	}

	defex_show_structure((void *)policy_data, policy_size);

exit:
	free(policy_data);
	fclose(policy_file);
	return 0;
}

int main(int argc, char **argv)
{
	static char param[4][PATH_MAX];
	char *src_file = NULL, *packed_file = NULL, *packed_bin_file = NULL;
	char *reduced_file = NULL, *list_file = NULL;
	int i;

	init_tree_data(D_TREE_VERSION_20);
	if (argc == 3) {
		if (!strncmp(argv[1], "-s", 2)) {
			SAFE_STRCOPY(param[0], argv[2]);
			src_file = param[0];
			parse_packed_bin_file(src_file);
			return 0;
		}
	}

	if (argc < 4 || argc > 5) {
		printf("Invalid number of arguments\n");
		goto show_help;
	}

	for (i = 0; i < (argc - 2); i++) {
		SAFE_STRCOPY(param[i], argv[i + 2]);
		switch (i) {
		case 0:
			src_file = param[i];
			break;
		case 1:
			packed_file = reduced_file = param[i];
			break;
		case 2:
			packed_bin_file = list_file = param[i];
			break;
		}
	}

	if (!strncmp(argv[1], "-p", 2)) {
		if (pack_rules(src_file, packed_file, packed_bin_file) != 0)
			goto show_help;
		return 0;

	} else if (!strncmp(argv[1], "-r", 2) && list_file) {
		if (reduce_rules(src_file, reduced_file, list_file) != 0)
			goto show_help;
		return 0;
	}
	printf("Invalid command\n");

show_help:
	printf("Defex rules processing utility.\nUSAGE:\n%s <CMD> <PARAMS>\n"
		"Commands:\n"
		"  -p - Pack rules file to the tree. Params: <SOURCE_FILE> <PACKED_FILE>"
		" [PACKED_BIN_FILE]\n"
		"  -r - Reduce rules file (remove unexistent files). Params: <SOURCE_FILE>"
		" <REDUCED_FILE> <FILE_LIST>\n"
		"  -s - Show rules binary file content. Params: <PACKED_BIN_FILE>\n",
		argv[0]);
	return -1;
}
