/* abc_spec_manager.c
 *
 * Abnormal Behavior Catcher's spec manager.
 *
 * Copyright 2021 Samsung Electronics
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
#include <linux/sti/abc_spec_manager.h>
#if IS_ENABLED(CONFIG_SEC_KUNIT)
#include <linux/sti/abc_kunit.h>
#endif
struct list_head abc_spec_list;
EXPORT_SYMBOL_KUNIT(abc_spec_list);

struct abc_event_group_struct abc_event_group_list[] = {
	{ABC_GROUP_CAMERA_MIPI_ERROR_ALL, "camera", "mipi_error_all"},
};

#if IS_ENABLED(CONFIG_OF)
int abc_parse_dt(struct device *dev)
{
	struct abc_platform_data *pdata = dev->platform_data;
	struct spec_data_type1 *spec_type1;
	struct device_node *np;
	struct device_node *type1_np;
	int idx, rc;

	np = dev->of_node;
	pdata->nItem = of_get_child_count(np);
	if (!pdata->nItem) {
		dev_err(dev, "There are no items");
		return -ENODEV;
	}

	/* spec_type_1 */
	type1_np = of_find_node_by_name(np, "abc_spec_type1");
	rc = of_property_count_strings(type1_np, ERROR_KEY);
	INIT_LIST_HEAD(&abc_spec_list);

	for (idx = 0; idx < rc; idx++) {
		spec_type1 = devm_kzalloc(dev, sizeof(struct spec_data_type1), GFP_KERNEL);

		if (!spec_type1)
			return -ENOMEM;

		if (abc_parse_dt_type1(dev, type1_np, idx, spec_type1)) {
			ABC_PRINT("failed parse dt spec_type1 idx : %d", idx);
			continue;
		}
		list_add_tail(&spec_type1->node, &abc_spec_list);
	}
	return 0;
}
#endif

int sec_abc_get_normal_token_value(char *dst, char *src, char *token)
{
	int token_len = strlen(token);

	if (strncmp(src, token, token_len) || !*(src + token_len)) {
		ABC_DEBUG("Invalid input : src-%s, token-%s", src, token);
		return -EINVAL;
	}

	strlcpy(dst, src + token_len, ABC_EVENT_STR_MAX);
	return 0;
}
EXPORT_SYMBOL_KUNIT(sec_abc_get_normal_token_value);

int sec_abc_get_event_module(char *dst, char *src)
{
	return sec_abc_get_normal_token_value(dst, src, "MODULE=");
}
EXPORT_SYMBOL_KUNIT(sec_abc_get_event_module);

int sec_abc_get_ext_log(char *dst, char *src)
{
	return sec_abc_get_normal_token_value(dst, src, "EXT_LOG=");
}
EXPORT_SYMBOL_KUNIT(sec_abc_get_ext_log);

int sec_abc_get_event_name(char *dst, char *src)
{
	int ret_info = 0, ret_warn = 0;

	ret_info = sec_abc_get_normal_token_value(dst, src, "INFO=");
	ret_warn = sec_abc_get_normal_token_value(dst, src, "WARN=");

	return (ret_info & ret_warn) ? -EINVAL : 0;
}
EXPORT_SYMBOL_KUNIT(sec_abc_get_event_name);

int sec_abc_get_event_type(char *dst, char *src)
{
	if (strncmp(src, "WARN", 4) && strncmp(src, "INFO", 4)) {
		ABC_PRINT("Invalid input : %s", src);
		return -EINVAL;
	}

	strlcpy(dst, src, 5);
	return 0;
}
EXPORT_SYMBOL_KUNIT(sec_abc_get_event_type);

int sec_abc_get_count(int *dst, char *src)
{
	if (strncmp(src, "COUNT=", 6) || !*(src + 6)) {
		ABC_PRINT("Invalid input : %s", src);
		return -EINVAL;
	}

	if (!strncmp(src + 6, "DEFAULT", 7)) {
		*dst = ABC_DEFAULT_COUNT;
		return 0;
	}

	if (kstrtoint(src + 6, 0, dst) || *dst <= 0 || *dst > ABC_EVENT_BUFFER_MAX) {
		ABC_PRINT("Invalid input : %s", src);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL_KUNIT(sec_abc_get_count);

unsigned int sec_abc_get_ktime_ms(void)
{
	u64 ktime;

	/* Calculate current kernel time */
	ktime = local_clock();
	do_div(ktime, NSEC_PER_MSEC);

	return (unsigned int)ktime;
}
EXPORT_SYMBOL_KUNIT(sec_abc_get_ktime_ms);

int sec_abc_make_key_data(struct abc_key_data *key_data, char *str)
{
	char *event_strings[ABC_UEVENT_MAX] = {0,};
	char temp[ABC_BUFFER_MAX];
	char *c, *p;
	int idx = 0, i;

	ABC_DEBUG("start : %s", str);

	strlcpy(temp, str, ABC_BUFFER_MAX);
	p = temp;

	while ((c = strsep(&p, "@")) != NULL && idx < ABC_UEVENT_MAX) {
		event_strings[idx] = c;
		idx++;
	}

	if (idx >= ABC_UEVENT_MAX)
		return -EINVAL;

	if (sec_abc_get_event_module(key_data->event_module, event_strings[0]))
		return -EINVAL;

	if (sec_abc_get_event_name(key_data->event_name, event_strings[1]))
		return -EINVAL;

	if (sec_abc_get_event_type(key_data->event_type, event_strings[1]))
		return -EINVAL;

	for (i = 2; i < idx; i++) {
		if (!strncmp(event_strings[i], "EXT_LOG=", 8)) {
			if (sec_abc_get_ext_log(key_data->ext_log, event_strings[i]))
				return -EINVAL;
		} else
			return -EINVAL;
	}

	key_data->cur_time = sec_abc_get_ktime_ms();

	ABC_DEBUG("Module(%s) Level(%s) Event(%s) EXT_LOG(%s) cur_time(%d)",
			  key_data->event_module,
			  key_data->event_type,
			  key_data->event_name,
			  key_data->ext_log,
			  key_data->cur_time);
	return 0;
}
EXPORT_SYMBOL_KUNIT(sec_abc_make_key_data);

struct abc_common_spec_data *sec_abc_get_matched_common_spec(char *module_name, char *error_name)
{
	return sec_abc_get_matched_common_spec_type1(module_name, error_name);
}
EXPORT_SYMBOL_KUNIT(sec_abc_get_matched_common_spec);

int sec_abc_get_buffer_size_from_threshold_cnt(int threshold_cnt)
{
	int i, min_buffer_size = 16;

	for (i = 1; i < threshold_cnt; i *= 2)
		;

	return min_buffer_size > i ? min_buffer_size : i;
}
EXPORT_SYMBOL_KUNIT(sec_abc_get_buffer_size_from_threshold_cnt);

void sec_abc_enqueue_event_data(struct abc_key_data *key_data)
{
	struct abc_common_spec_data *common_spec = NULL;

	common_spec = sec_abc_get_matched_common_spec(key_data->event_module, key_data->event_name);

	if (!common_spec || !strcmp(key_data->event_type, "INFO")) {
		ABC_PRINT_KUNIT("There is no matched buffer");
	} else {
		ABC_DEBUG_KUNIT("There is a matched buffer. Enqueue data");
		sec_abc_enqueue_event_data_type1(common_spec, key_data->cur_time);
	}

}
EXPORT_SYMBOL_KUNIT(sec_abc_enqueue_event_data);

void sec_abc_reset_event_buffer(struct abc_key_data *key_data)
{
	struct abc_common_spec_data *common_spec;
	struct spec_data_type1 *spec_type1;

	common_spec = sec_abc_get_matched_common_spec(key_data->event_module, key_data->event_name);

	if (!common_spec || !strcmp(key_data->event_type, "INFO")) {
		ABC_PRINT_KUNIT("There is no matched buffer");
	} else {
		ABC_PRINT_KUNIT("There is a matched buffer. Reset buffer");
		spec_type1 = container_of(common_spec, struct spec_data_type1, common_spec);
		sec_abc_reset_buffer_type1(spec_type1);
	}
}
EXPORT_SYMBOL_KUNIT(sec_abc_reset_event_buffer);

bool sec_abc_reached_spec(struct abc_key_data *key_data)
{
	struct abc_common_spec_data *common_spec;

	if (!strcmp(key_data->event_type, "INFO")) {
		ABC_PRINT("INFO doesn't have spec");
		return false;
	}

	common_spec = sec_abc_get_matched_common_spec(key_data->event_module, key_data->event_name);
	if (!common_spec)
		return true;

	return sec_abc_reached_spec_type1(common_spec, key_data->cur_time);
}
EXPORT_SYMBOL_KUNIT(sec_abc_reached_spec);

int sec_abc_parse_spec_cmd(char *str, struct abc_spec_cmd *abc_spec)
{
	int idx = 0;
	char temp[ABC_BUFFER_MAX];
	char *c, *p;
	char *sys_input_strings[3] = { 0, };

	strlcpy(temp, str, ABC_BUFFER_MAX);
	p = temp;

	while ((c = strsep(&p, "@")) != NULL && idx < ABC_SPEC_CMD_STR_MAX) {
		sys_input_strings[idx] = c;
		idx++;
	}

	if (idx != ABC_SPEC_CMD_STR_MAX)
		return -EINVAL;

	if (sec_abc_get_event_module(abc_spec->module, sys_input_strings[0]))
		return -EINVAL;

	if (sec_abc_get_event_name(abc_spec->name, sys_input_strings[1]))
		return -EINVAL;

	strlcpy(abc_spec->spec, sys_input_strings[2], ABC_EVENT_STR_MAX);

	return 0;
}
EXPORT_SYMBOL_KUNIT(sec_abc_parse_spec_cmd);

int sec_abc_apply_changed_spec(char *module_name, char *error_name, char *spec)
{
	struct abc_common_spec_data *common_spec;
	struct spec_data_type1 *spec_type1;
	int idx = 0, count = 0;

	ABC_PRINT("start : %s %s %s", module_name, error_name, spec);
	idx = sec_abc_get_idx_of_registered_event(module_name, error_name);

	if (idx < 0)
		return -EINVAL;

	if (!strcmp(spec, "OFF")) {
		abc_event_list[idx].enabled = false;
		return 0;
	}

	if (sec_abc_get_count(&count, spec))
		return -EINVAL;

	if (abc_event_list[idx].singular_spec) {
		if (count == ABC_DEFAULT_COUNT) {
			abc_event_list[idx].enabled = true;
			return 0;
		} else
			return -EINVAL;
	}

	common_spec = sec_abc_get_matched_common_spec(module_name, error_name);

	if (!common_spec)
		return -EINVAL;

	abc_event_list[idx].enabled = false;
	spec_type1 = container_of(common_spec, struct spec_data_type1, common_spec);

	if (count == ABC_DEFAULT_COUNT)
		count = spec_type1->default_count;

	spec_type1->threshold_cnt = count;
	spec_type1->buffer.size = count + 1;

	if (abc_alloc_memory_to_buffer_type1(spec_type1, spec_type1->buffer.size))
		return -ENOMEM;

	sec_abc_reset_buffer_type1(spec_type1);
	abc_event_list[idx].enabled = true;

	ABC_PRINT("MODULE(%s) ERROR(%s) COUNT(%d) TIME(%d) Enabled(%d)",
			  common_spec->module_name,
			  common_spec->error_name,
			  spec_type1->threshold_cnt,
			  spec_type1->threshold_time,
			  abc_event_list[idx].enabled);
	return 0;
}
EXPORT_SYMBOL_KUNIT(sec_abc_apply_changed_spec);

enum abc_event_group sec_abc_get_group(char *module, char *name)
{
	int idx = 0;

	for (idx = 0; idx < ARRAY_SIZE(abc_event_group_list); idx++) {
		if (strcmp(module, abc_event_group_list[idx].module) == 0 &&
			strcmp(name, abc_event_group_list[idx].name) == 0) {
			return  abc_event_group_list[idx].group;
		}
	}

	return ABC_GROUP_NONE;
}
EXPORT_SYMBOL_KUNIT(sec_abc_get_group);

int sec_abc_apply_changed_group_spec(enum abc_event_group group, char *spec)
{
	int idx = 0;

	ABC_PRINT("start : %d %s", group, spec);

	for (idx = 0; idx < REGISTERED_ABC_EVENT_TOTAL; idx++) {
		if (group == abc_event_list[idx].group) {
			if (sec_abc_apply_changed_spec(abc_event_list[idx].module_name,
				abc_event_list[idx].error_name, spec)) {
				return -EINVAL;
			}
		}
	}

	return 0;
}
EXPORT_SYMBOL_KUNIT(sec_abc_apply_changed_group_spec);

void sec_abc_change_spec(const char *str)
{
	char *cmd_string, *p;
	char temp[ABC_BUFFER_MAX * 5];
	int cnt = 0;
	enum abc_event_group group;
	struct abc_spec_cmd abc_spec;

	ABC_PRINT("start : %s", str);

	if (!strncmp(str, "reset", 5)) {
		sec_abc_reset_all_spec();
		ABC_PRINT("end : %s", str);
		return;
	}

	strlcpy(temp, str, ABC_BUFFER_MAX * 5);
	p = temp;

	while ((cmd_string = strsep(&p, ",\n")) != NULL && cnt < REGISTERED_ABC_EVENT_TOTAL) {

		cnt++;
		if (!cmd_string[0])
			continue;
		if (sec_abc_parse_spec_cmd(cmd_string, &abc_spec)) {
			ABC_PRINT_KUNIT("Invalid change cmd. Check the Input");
			break;
		}
		group = sec_abc_get_group(abc_spec.module, abc_spec.name);
		if (group == ABC_GROUP_NONE) {
			if (sec_abc_apply_changed_spec(abc_spec.module, abc_spec.name, abc_spec.spec)) {
				ABC_PRINT_KUNIT("Invalid change cmd. Check the Input");
				break;
			}
		} else {
			if (sec_abc_apply_changed_group_spec(group, abc_spec.spec)) {
				ABC_PRINT_KUNIT("Invalid change cmd. Check the Input");
				break;
			}
		}
	}
}

void sec_abc_reset_all_buffer(void)
{
	struct spec_data_type1 *spec_type1;

	list_for_each_entry(spec_type1, &abc_spec_list, node) {
		sec_abc_reset_buffer_type1(spec_type1);
	}
}
EXPORT_SYMBOL_KUNIT(sec_abc_reset_all_buffer);

void sec_abc_reset_all_spec(void)
{
	struct spec_data_type1 *spec_type1;

	list_for_each_entry(spec_type1, &abc_spec_list, node) {
		spec_type1->threshold_cnt = spec_type1->default_count;
		spec_type1->buffer.size = spec_type1->threshold_cnt + 1;
		abc_event_list[spec_type1->common_spec.idx].enabled = spec_type1->default_enabled;
		sec_abc_reset_buffer_type1(spec_type1);
	}
}
EXPORT_SYMBOL_KUNIT(sec_abc_reset_all_spec);

int sec_abc_read_spec(char *buf)
{
	struct spec_data_type1 *spec_type1;
	int len = 0, idx;

	len += scnprintf(buf + len, PAGE_SIZE - len, "spec type1\n");

	list_for_each_entry(spec_type1, &abc_spec_list, node) {

		len += scnprintf(buf + len, PAGE_SIZE - len, "MODULE=%s ",
						 spec_type1->common_spec.module_name);
		len += scnprintf(buf + len, PAGE_SIZE - len, "WARN=%s ",
						 spec_type1->common_spec.error_name);
		len += scnprintf(buf + len, PAGE_SIZE - len, "THRESHOLD_CNT=%d ",
						 spec_type1->threshold_cnt);
		len += scnprintf(buf + len, PAGE_SIZE - len, "THRESHOLD_TIME=%d ",
						 spec_type1->threshold_time);
		idx = spec_type1->common_spec.idx;
		len += scnprintf(buf + len, PAGE_SIZE - len, "ENABLE=%s",
						 ((abc_event_list[idx].enabled) ? "ON" : "OFF"));
		len += scnprintf(buf + len, PAGE_SIZE - len, "\n");
	}

	for (idx = 0; idx < ARRAY_SIZE(abc_event_group_list); idx++) {
		len += scnprintf(buf + len, PAGE_SIZE - len, "MODULE=%s ",
						 abc_event_group_list[idx].module);
		len += scnprintf(buf + len, PAGE_SIZE - len, "WARN=%s ",
						 abc_event_group_list[idx].name);
		len += scnprintf(buf + len, PAGE_SIZE - len, "THRESHOLD_CNT=group ");
		len += scnprintf(buf + len, PAGE_SIZE - len, "THRESHOLD_TIME=group ");
		len += scnprintf(buf + len, PAGE_SIZE - len, "ENABLE=group");
		len += scnprintf(buf + len, PAGE_SIZE - len, "\n");
	}

	ABC_PRINT("%d", len);
	return len;
}

void sec_abc_free_spec_buffer(void)
{
	struct spec_data_type1 *spec_buf;

	list_for_each_entry(spec_buf, &abc_spec_list, node) {
		kfree(spec_buf->buffer.abc_element);
		spec_buf->buffer.abc_element = NULL;
		spec_buf->buffer.buffer_max = 0;
	}
}
EXPORT_SYMBOL_KUNIT(sec_abc_free_spec_buffer);

MODULE_DESCRIPTION("Samsung ABC Driver's spec manager");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
