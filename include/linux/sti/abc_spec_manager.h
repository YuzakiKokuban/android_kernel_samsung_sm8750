/* abc_spec_manager.h
 *
 * Abnormal Behavior Catcher's spec manager module.
 *
 * Copyright 2021 Samsung Electronics
 *
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
 */

#ifndef SEC_ABC_SPEC_MANAGER_H
#define SEC_ABC_SPEC_MANAGER_H
#include <linux/sti/abc_common.h>
#define ERROR_KEY					"name_list"
#define MODULE_KEY					"module_list"
#define THRESHOLD_CNT_KEY			"threshold_cnt"
#define THRESHOLD_TIME_KEY			"threshold_time"

extern struct list_head abc_spec_list;

int abc_parse_dt(struct device *dev);
void sec_abc_reset_event_buffer(struct abc_key_data *key_data);
int sec_abc_make_key_data(struct abc_key_data *key_data, char *str);
bool sec_abc_reached_spec(struct abc_key_data *key_data);
void sec_abc_enqueue_event_data(struct abc_key_data *key_data);
struct abc_common_spec_data *sec_abc_get_matched_common_spec(char *module_name, char *error_name);
int sec_abc_get_buffer_size_from_threshold_cnt(int th_max);
int sec_abc_parse_spec_cmd(char *str, struct abc_spec_cmd *abc_spec);
int sec_abc_apply_changed_spec(char *module_name, char *error_name, char *spec);
enum abc_event_group sec_abc_get_group(char *module, char *name);
int sec_abc_apply_changed_group_spec(enum abc_event_group group, char *spec);
int sec_abc_get_event_module(char *dst, char *src);
int sec_abc_get_event_name(char *dst, char *src);
int sec_abc_get_event_type(char *dst, char *src);
int sec_abc_get_ext_log(char *dst, char *src);
int sec_abc_get_count(int *dst, char *src);
void sec_abc_reset_all_spec(void);
void sec_abc_free_spec_buffer(void);
void sec_abc_reset_all_buffer(void);
/* spec_type1 */
int abc_parse_dt_type1(struct device *dev,
			  struct device_node *np, int idx,
			  struct spec_data_type1 *spec_type1);
bool sec_abc_is_full_type1(struct abc_event_buffer *buffer);
bool sec_abc_is_empty_type1(struct abc_event_buffer *buffer);
int sec_abc_get_diff_time_type1(struct abc_event_buffer *buffer);
void sec_abc_reset_buffer_type1(struct spec_data_type1 *common_spec);
struct abc_fault_info sec_abc_dequeue_type1(struct abc_event_buffer *buffer);
void sec_abc_dequeue_event_data_type1(struct abc_common_spec_data *common_spec);
void sec_abc_enqueue_type1(struct abc_event_buffer *buffer, struct abc_fault_info in);
bool sec_abc_reached_spec_type1(struct abc_common_spec_data *common_spec, unsigned int cur_time);
struct abc_common_spec_data *sec_abc_get_matched_common_spec_type1(char *module_name, char *error_name);
void sec_abc_enqueue_event_data_type1(struct abc_common_spec_data *common_spec, unsigned int cur_time);
int abc_alloc_memory_to_buffer_type1(struct spec_data_type1 *spec_type1, int size);
#endif
