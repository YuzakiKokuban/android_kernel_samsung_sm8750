/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung TN debugging code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/list_sort.h>

#include <../../sec_input/sec_input.h>
#include <linux/hall/sec_hall_dumpkey.h>

static DEFINE_SPINLOCK(sec_hall_dumpkey_event_lock);
static atomic_t sec_hall_dumpkey_acceptable_event[KEY_MAX] __read_mostly;

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)		(sizeof(a) / sizeof(a[0]))
#endif

#define DUMP_COUNT_FIRST 1
#define DUMP_COUNT_SECOND 2

#define KEY_STATE_DOWN 1
#define KEY_STATE_UP 0

struct hall_dump_callbacks hall_dump_callbacks;
EXPORT_SYMBOL(hall_dump_callbacks);

struct device *phall;
EXPORT_SYMBOL(phall);

static unsigned int __hall_dump_keys[] = {
	KEY_POWER,
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
	KEY_HOMEPAGE,
	KEY_HOT,
	KEY_EMERGENCY,
	KEY_BACK,
	KEY_RECENT
};

struct dump_key {
	unsigned int key_code;
	unsigned int dump_count;
};

struct dump_key hall_dump_key_combination[] = {
	{KEY_VOLUMEUP, DUMP_COUNT_FIRST},
	{KEY_VOLUMEDOWN, DUMP_COUNT_SECOND},
};

struct hall_dump_key_state {
	unsigned int key_code;
	unsigned int state;
};

struct hall_dump_key_state hall_dump_key_states[] = {
	{KEY_VOLUMEDOWN, KEY_STATE_UP},
	{KEY_VOLUMEUP, KEY_STATE_UP},
	{KEY_POWER, KEY_STATE_UP},
	{KEY_HOMEPAGE, KEY_STATE_UP},
	{KEY_HOT, KEY_STATE_UP},
	{KEY_EMERGENCY, KEY_STATE_UP},
	{KEY_BACK, KEY_STATE_UP},
	{KEY_RECENT, KEY_STATE_UP},
};

static unsigned int hold_key = KEY_POWER;
static unsigned int hold_key_hold = KEY_STATE_UP;
static unsigned int check_count;
static unsigned int check_step;

static int is_hold_key(unsigned int code)
{
	return (code == hold_key);
}

static void set_hold_key_hold(int state)
{
	hold_key_hold = state;
}

static unsigned int is_hold_key_hold(void)
{
	return hold_key_hold;
}

static unsigned int get_current_step_key_code(void)
{
	return hall_dump_key_combination[check_step].key_code;
}

static int is_key_matched_for_current_step(unsigned int code)
{
	return (code == get_current_step_key_code());
}

static int is_dump_keys(unsigned int code)
{
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(hall_dump_key_states); i++)
		if (hall_dump_key_states[i].key_code == code)
			return 1;
	return 0;
}

static int get_count_for_next_step(void)
{
	int i;
	int count = 0;

	for (i = 0; i < check_step + 1; i++)
		count += hall_dump_key_combination[i].dump_count;
	return count;
}

static int is_reaching_count_for_next_step(void)
{
	return (check_count == get_count_for_next_step());
}

static int get_count_for_dump(void)
{
	unsigned long i;
	int count = 0;

	for (i = 0; i < ARRAY_SIZE(hall_dump_key_combination); i++)
		count += hall_dump_key_combination[i].dump_count;
	return count - 1;
}

static unsigned int is_key_state_down(unsigned int code)
{
	unsigned long i;

	if (is_hold_key(code))
		return is_hold_key_hold();

	for (i = 0; i < ARRAY_SIZE(hall_dump_key_states); i++)
		if (hall_dump_key_states[i].key_code == code)
			return hall_dump_key_states[i].state == KEY_STATE_DOWN;
	return 0;
}

static void set_key_state_down(unsigned int code)
{
	unsigned long i;

	if (is_hold_key(code))
		set_hold_key_hold(KEY_STATE_DOWN);

	for (i = 0; i < ARRAY_SIZE(hall_dump_key_states); i++)
		if (hall_dump_key_states[i].key_code == code)
			hall_dump_key_states[i].state = KEY_STATE_DOWN;
}

static void set_key_state_up(unsigned int code)
{
	unsigned long i;

	if (is_hold_key(code))
		set_hold_key_hold(KEY_STATE_UP);

	for (i = 0; i < ARRAY_SIZE(hall_dump_key_states); i++)
		if (hall_dump_key_states[i].key_code == code)
			hall_dump_key_states[i].state = KEY_STATE_UP;
}

static void increase_step(void)
{
	if (check_step < ARRAY_SIZE(hall_dump_key_combination))
		check_step++;
	else if (hall_dump_callbacks.inform_dump)
		hall_dump_callbacks.inform_dump(phall);
}

static void reset_step(void)
{
	check_step = 0;
}

static void increase_count(void)
{
	if (check_count < get_count_for_dump())
		check_count++;
	else if (hall_dump_callbacks.inform_dump)
		hall_dump_callbacks.inform_dump(phall);
}

static void reset_count(void)
{
	check_count = 0;
}

static void check_hall_dump_keys(struct sec_hall_dumpkey_param *param)
{
	unsigned int code = param->keycode;
	int state = param->down;

	if (!is_dump_keys(code))
		return;

	if (state == KEY_STATE_DOWN) {
		/* Duplicated input */
		if (is_key_state_down(code))
			return;
		set_key_state_down(code);

		if (is_hold_key(code)) {
			set_hold_key_hold(KEY_STATE_DOWN);
			return;
		}
		if (is_hold_key_hold()) {
			if (is_key_matched_for_current_step(code)) {
				increase_count();
			} else {
				reset_count();
				reset_step();
			}
			if (is_reaching_count_for_next_step())
				increase_step();
		}

	} else {
		set_key_state_up(code);
		if (is_hold_key(code)) {
			set_hold_key_hold(KEY_STATE_UP);
			reset_step();
			reset_count();
		}
	}

}

static inline void update_acceptable_event(unsigned int event_code, bool is_add)
{
	if (is_add)
		atomic_inc(&(sec_hall_dumpkey_acceptable_event[event_code]));
	else
		atomic_dec(&(sec_hall_dumpkey_acceptable_event[event_code]));
}

static inline bool is_event_supported(unsigned int event_type,
		unsigned int event_code)
{
	bool ret;

	if (event_type != EV_KEY || event_code >= KEY_MAX)
		return false;

	ret = !!atomic_read(&(sec_hall_dumpkey_acceptable_event[event_code]));

	return ret;
}

static void sec_hall_dumpkey_event(struct input_handle *handle, unsigned int event_type,
		unsigned int event_code, int value)
{
	struct sec_hall_dumpkey_param param = {
		.keycode = event_code,
		.down = value,
	};

	if (!is_event_supported(event_type, event_code))
		return;

	//pr_info("%s key_event: %u, %d\n", SECLOG, event_code, value);

	spin_lock(&sec_hall_dumpkey_event_lock);

	check_hall_dump_keys(&param);

	spin_unlock(&sec_hall_dumpkey_event_lock);
}

static int sec_hall_dumpkey_connect(struct input_handler *handler, struct input_dev *dev,
		const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "sec_hall_dumpkey";

	error = input_register_handle(handle);
	if (error)
		goto err_free_handle;

	error = input_open_device(handle);
	if (error)
		goto err_unregister_handle;

	return 0;

err_unregister_handle:
	input_unregister_handle(handle);
err_free_handle:
	kfree(handle);
	return error;
}

static void sec_hall_dumpkey_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id sec_hall_dumpkey_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
	},
	{},
};

static struct input_handler sec_hall_dumpkey_handler = {
	.event		= sec_hall_dumpkey_event,
	.connect	= sec_hall_dumpkey_connect,
	.disconnect	= sec_hall_dumpkey_disconnect,
	.name		= "sec_hall_dumpkey",
	.id_table	= sec_hall_dumpkey_ids,
};

static int __init sec_hall_dumpkey_init(void)
{
	int err;
	size_t i;

	pr_info("%s %s\n", SECLOG, __func__);

	for (i = 0; i < KEY_MAX; i++)
		atomic_set(&(sec_hall_dumpkey_acceptable_event[i]), 0);

	for (i = 0; i < ARRAY_SIZE(__hall_dump_keys); i++)
		update_acceptable_event(__hall_dump_keys[i], true);

	spin_lock_init(&sec_hall_dumpkey_event_lock);

	err = input_register_handler(&sec_hall_dumpkey_handler);

	return err;

}

static void __exit sec_hall_dumpkey_exit(void)
{
	input_unregister_handler(&sec_hall_dumpkey_handler);
}

module_init(sec_hall_dumpkey_init);
module_exit(sec_hall_dumpkey_exit);

MODULE_DESCRIPTION("Samsung hall dumpkey driver");
MODULE_LICENSE("GPL");
