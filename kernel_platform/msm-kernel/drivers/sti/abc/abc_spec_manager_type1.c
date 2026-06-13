/* abc_spec_manager_type1.c
 *
 * Abnormal Behavior Catcher's spec(type1) manager
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
int abc_alloc_memory_to_buffer_type1(struct spec_data_type1 *spec_type1, int size)
{
	struct abc_fault_info *temp_abc_elements;
	int buffer_max;

	buffer_max = sec_abc_get_buffer_size_from_threshold_cnt(size);

	if (spec_type1->buffer.buffer_max >= buffer_max) {
		ABC_PRINT("Doesn't need to alloc memory");
		return 0;
	}

	if (!spec_type1->buffer.abc_element) {
		ABC_PRINT("allocate new memory");
		temp_abc_elements = kcalloc(buffer_max, sizeof(spec_type1->buffer.abc_element[0]), GFP_KERNEL);
	} else {
		ABC_PRINT("reallocate memory");
		temp_abc_elements = krealloc(spec_type1->buffer.abc_element,
					sizeof(spec_type1->buffer.abc_element[0]) * buffer_max, GFP_KERNEL);
	}

	if (unlikely(ZERO_OR_NULL_PTR(temp_abc_elements))) {
		ABC_PRINT("Failed to allocate memory to buffer");
		spec_type1->buffer.abc_element = NULL;
		spec_type1->buffer.buffer_max = 0;
		spec_type1->buffer.size = 0;
		return -ENOMEM;
	}

	spec_type1->buffer.abc_element = temp_abc_elements;
	spec_type1->buffer.buffer_max = buffer_max;

	return 0;
}
EXPORT_SYMBOL_KUNIT(abc_alloc_memory_to_buffer_type1);

#if IS_ENABLED(CONFIG_OF)
int abc_parse_dt_type1(struct device *dev,
					   struct device_node *np,
					   int idx,
					   struct spec_data_type1 *spec_type1)
{
	int registered_idx;

	if (of_property_read_string_index(np, MODULE_KEY, idx, (const char **)&spec_type1->common_spec.module_name)) {
		dev_err(dev, "Failed to get module name : node not exist");
		return -EINVAL;
	}

	if (of_property_read_string_index(np, ERROR_KEY, idx, (const char **)&spec_type1->common_spec.error_name)) {
		dev_err(dev, "Failed to get error name : node not exist");
		return -EINVAL;
	}

	registered_idx = sec_abc_get_idx_of_registered_event((char *)spec_type1->common_spec.module_name,
				(char *)spec_type1->common_spec.error_name);

	if (registered_idx < 0) {
		ABC_PRINT("Unregistered Event. %s : %s", spec_type1->common_spec.module_name,
			spec_type1->common_spec.error_name);
		return -EINVAL;
	}

	spec_type1->common_spec.idx = registered_idx;

	if (of_property_read_u32_index(np, THRESHOLD_CNT_KEY, idx, &spec_type1->threshold_cnt)) {
		dev_err(dev, "Failed to get threshold count : node not exist");
		return -EINVAL;
	}

	if (of_property_read_u32_index(np, THRESHOLD_TIME_KEY, idx, &spec_type1->threshold_time)) {
		dev_err(dev, "Failed to get threshold time : node not exist");
		return -EINVAL;
	}

	if (spec_type1->threshold_time == 0)
		spec_type1->threshold_time = INT_MAX;

	if (abc_alloc_memory_to_buffer_type1(spec_type1, spec_type1->threshold_cnt + 1))
		return -ENOMEM;

	spec_type1->default_count = spec_type1->threshold_cnt;
	spec_type1->buffer.size = spec_type1->threshold_cnt + 1;
	spec_type1->default_enabled = abc_event_list[spec_type1->common_spec.idx].enabled;
	sec_abc_reset_buffer_type1(spec_type1);

	ABC_PRINT("type1-spec : module(%s) error(%s) threshold_cnt(%d) threshold_time(%d) enabled(%s)",
		  spec_type1->common_spec.module_name,
		  spec_type1->common_spec.error_name,
		  spec_type1->threshold_cnt,
		  spec_type1->threshold_time,
		  ((abc_event_list[spec_type1->common_spec.idx].enabled) ? "ON" : "OFF"));

	return 0;
}
#endif

struct abc_common_spec_data *sec_abc_get_matched_common_spec_type1(char *module_name, char *error_name)
{
	struct spec_data_type1 *spec_type1;

	list_for_each_entry(spec_type1, &abc_spec_list, node) {
		if (!strcmp(spec_type1->common_spec.module_name, module_name)
			&& !strcmp(spec_type1->common_spec.error_name, error_name))
			return &(spec_type1->common_spec);
	}
	return NULL;
}

bool sec_abc_reached_spec_type1(struct abc_common_spec_data *common_spec, unsigned int cur_time)
{
	struct spec_data_type1 *spec_type1;

	spec_type1 = container_of(common_spec, struct spec_data_type1, common_spec);

	do_div(cur_time, MSEC_PER_SEC);

	ABC_DEBUG("MODULE(%s) WARN(%s) warn_cnt(%d) time(%d)",
			  spec_type1->common_spec.module_name,
			  spec_type1->common_spec.error_name,
			  spec_type1->buffer.warn_cnt,
			  cur_time);

	if (spec_type1->buffer.warn_cnt >= spec_type1->threshold_cnt) {
		if (sec_abc_get_diff_time_type1(&spec_type1->buffer) < spec_type1->threshold_time) {
			ABC_PRINT("%s occurred. Send uevent", spec_type1->common_spec.error_name);
			return true;
		}
		sec_abc_dequeue_event_data_type1(common_spec);
	}
	return false;
}
EXPORT_SYMBOL_KUNIT(sec_abc_reached_spec_type1);

void sec_abc_reset_buffer_type1(struct spec_data_type1 *spec_type1)
{
	spec_type1->buffer.rear = 0;
	spec_type1->buffer.front = 0;
	spec_type1->buffer.warn_cnt = 0;
}
EXPORT_SYMBOL_KUNIT(sec_abc_reset_buffer_type1);

bool sec_abc_is_full_type1(struct abc_event_buffer *buffer)
{
	if ((buffer->rear + 1) % buffer->size == buffer->front)
		return true;
	else
		return false;
}
EXPORT_SYMBOL_KUNIT(sec_abc_is_full_type1);

bool sec_abc_is_empty_type1(struct abc_event_buffer *buffer)
{
	if (buffer->front == buffer->rear)
		return true;
	else
		return false;
}
EXPORT_SYMBOL_KUNIT(sec_abc_is_empty_type1);

void sec_abc_enqueue_type1(struct abc_event_buffer *buffer, struct abc_fault_info in)
{
	if (sec_abc_is_full_type1(buffer)) {
		ABC_PRINT_KUNIT("queue is full");
	} else {
		buffer->rear = (buffer->rear + 1) % buffer->size;
		buffer->abc_element[buffer->rear] = in;
	}
}
EXPORT_SYMBOL_KUNIT(sec_abc_enqueue_type1);

struct abc_fault_info sec_abc_dequeue_type1(struct abc_event_buffer *buffer)
{
	struct abc_fault_info out;

	if (sec_abc_is_empty_type1(buffer)) {
		ABC_PRINT_KUNIT("queue is empty");
		out.cur_time = out.cur_cnt = 0;
	} else {
		buffer->front = (buffer->front + 1) % buffer->size;
		out = buffer->abc_element[buffer->front];
	}
	return out;
}
EXPORT_SYMBOL_KUNIT(sec_abc_dequeue_type1);

void sec_abc_enqueue_event_data_type1(struct abc_common_spec_data *common_spec, unsigned int cur_time)
{
	struct spec_data_type1 *spec_type1 = container_of(common_spec, struct spec_data_type1, common_spec);
	struct abc_fault_info in;

	do_div(cur_time, MSEC_PER_SEC);

	in.cur_time = (int)cur_time;
	in.cur_cnt = spec_type1->buffer.warn_cnt++;

	sec_abc_enqueue_type1(&spec_type1->buffer, in);
}

void sec_abc_dequeue_event_data_type1(struct abc_common_spec_data *common_spec)
{
	struct spec_data_type1 *spec_type1 = container_of(common_spec, struct spec_data_type1, common_spec);
	struct abc_fault_info out;

	out = sec_abc_dequeue_type1(&spec_type1->buffer);
	spec_type1->buffer.warn_cnt--;
}

int sec_abc_get_diff_time_type1(struct abc_event_buffer *buffer)
{
	int front_time, rear_time;

	front_time = buffer->abc_element[(buffer->front + 1) % buffer->size].cur_time;
	rear_time = buffer->abc_element[buffer->rear].cur_time;

	ABC_DEBUG("front time : %d sec (%d) rear_time %d sec (%d) diff : %d",
		  front_time,
		  buffer->front + 1,
		  rear_time,
		  buffer->rear,
		  rear_time - front_time);

	return rear_time - front_time;
}
EXPORT_SYMBOL_KUNIT(sec_abc_get_diff_time_type1);

MODULE_DESCRIPTION("Samsung ABC Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
