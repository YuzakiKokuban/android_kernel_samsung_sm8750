// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
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

#include "sec_input.h"
#include "sec_tsp_dumpkey.h"

static DEFINE_SPINLOCK(sec_tsp_dumpkey_event_lock);
static atomic_t sec_tsp_dumpkey_acceptable_event[KEY_MAX] __read_mostly;

/* Input sequence 9530 */
#define DUMP_COUNT_FIRST 3
#define DUMP_COUNT_SECOND 3
#define DUMP_COUNT_THIRD 3

#define KEY_STATE_DOWN 1
#define KEY_STATE_UP 0

struct tsp_dump_callbacks *dump_callbacks;

static unsigned int __tsp_dump_keys[] = {
	KEY_POWER,
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
	KEY_HOMEPAGE,
	KEY_HOT,
	KEY_EMERGENCY,
	KEY_BACK,
	KEY_RECENT,
	KEY_APPSELECT
};

struct dump_key {
	unsigned int key_code;
	unsigned int dump_count;
};

struct dump_key tsp_dump_key_combination[] = {
	{KEY_VOLUMEDOWN, DUMP_COUNT_FIRST},
	{KEY_POWER, DUMP_COUNT_SECOND},
	{KEY_VOLUMEDOWN, DUMP_COUNT_THIRD},
};

struct tsp_dump_key_state {
	unsigned int key_code;
	unsigned int state;
};

struct tsp_dump_key_state tsp_dump_key_states[] = {
	{KEY_VOLUMEDOWN, KEY_STATE_UP},
	{KEY_VOLUMEUP, KEY_STATE_UP},
	{KEY_POWER, KEY_STATE_UP},
	{KEY_HOMEPAGE, KEY_STATE_UP},
	{KEY_HOT, KEY_STATE_UP},
	{KEY_EMERGENCY, KEY_STATE_UP},
	{KEY_BACK, KEY_STATE_UP},
	{KEY_RECENT, KEY_STATE_UP},
	{KEY_APPSELECT, KEY_STATE_UP},
};

static unsigned int hold_key = KEY_VOLUMEUP;
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
	return tsp_dump_key_combination[check_step].key_code;
}

static int is_key_matched_for_current_step(unsigned int code)
{
	return (code == get_current_step_key_code());
}

static int is_dump_keys(unsigned int code)
{
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(tsp_dump_key_states); i++)
		if (tsp_dump_key_states[i].key_code == code)
			return 1;
	return 0;
}

static int get_count_for_next_step(void)
{
	int i;
	int count = 0;

	for (i = 0; i < check_step + 1; i++)
		count += tsp_dump_key_combination[i].dump_count;
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

	for (i = 0; i < ARRAY_SIZE(tsp_dump_key_combination); i++)
		count += tsp_dump_key_combination[i].dump_count;
	return count - 1;
}

static unsigned int is_key_state_down(unsigned int code)
{
	unsigned long i;

	if (is_hold_key(code))
		return is_hold_key_hold();

	for (i = 0; i < ARRAY_SIZE(tsp_dump_key_states); i++)
		if (tsp_dump_key_states[i].key_code == code)
			return tsp_dump_key_states[i].state == KEY_STATE_DOWN;
	return 0;
}

static void set_key_state_down(unsigned int code)
{
	unsigned long i;

	if (is_hold_key(code))
		set_hold_key_hold(KEY_STATE_DOWN);

	for (i = 0; i < ARRAY_SIZE(tsp_dump_key_states); i++)
		if (tsp_dump_key_states[i].key_code == code)
			tsp_dump_key_states[i].state = KEY_STATE_DOWN;
}

static void set_key_state_up(unsigned int code)
{
	unsigned long i;

	if (is_hold_key(code))
		set_hold_key_hold(KEY_STATE_UP);

	for (i = 0; i < ARRAY_SIZE(tsp_dump_key_states); i++)
		if (tsp_dump_key_states[i].key_code == code)
			tsp_dump_key_states[i].state = KEY_STATE_UP;
}

static void increase_step(void)
{
	int i;
	if (check_step < ARRAY_SIZE(tsp_dump_key_combination))
		check_step++;
	else {
		for (i = 0; i < NUM_DEVICES; i++) {
			if (dump_callbacks[i].inform_dump)
				dump_callbacks[i].inform_dump(dump_callbacks[i].ptsp);
		}
	}
}

static void reset_step(void)
{
	check_step = 0;
}

static void increase_count(void)
{
	int i;
	if (check_count < get_count_for_dump())
		check_count++;
	else {
		for (i = 0; i < NUM_DEVICES; i++) {
			if (dump_callbacks[i].inform_dump)
				dump_callbacks[i].inform_dump(dump_callbacks[i].ptsp);
		}
	}
}

static void reset_count(void)
{
	check_count = 0;
}

static void check_tsp_dump_keys(struct sec_tsp_dumpkey_param *param)
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
		atomic_inc(&(sec_tsp_dumpkey_acceptable_event[event_code]));
	else
		atomic_dec(&(sec_tsp_dumpkey_acceptable_event[event_code]));
}

static inline bool is_event_supported(unsigned int event_type,
		unsigned int event_code)
{
	bool ret;

	if (event_type != EV_KEY || event_code >= KEY_MAX)
		return false;

	ret = !!atomic_read(&(sec_tsp_dumpkey_acceptable_event[event_code]));

	return ret;
}

static void sec_tsp_dumpkey_event(struct input_handle *handle, unsigned int event_type,
		unsigned int event_code, int value)
{
	struct sec_tsp_dumpkey_param param = {
		.keycode = event_code,
		.down = value,
	};

	if (!is_event_supported(event_type, event_code))
		return;

	pr_info("%s key_event: %u, %d\n", SECLOG, event_code, value);

	spin_lock(&sec_tsp_dumpkey_event_lock);

	check_tsp_dump_keys(&param);

	spin_unlock(&sec_tsp_dumpkey_event_lock);
}

static int sec_tsp_dumpkey_connect(struct input_handler *handler, struct input_dev *dev,
		const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "sec_tsp_dumpkey";

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

static void sec_tsp_dumpkey_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

void sec_input_dumpkey_register(int dev_id, void (*callback)(struct device *dev), struct device *dev)
{
	if (dev_id < NUM_DEVICES && dev_id >= MULTI_DEV_NONE) {
		dump_callbacks[dev_id].inform_dump = callback;
		dump_callbacks[dev_id].ptsp = dev;
	}
}
EXPORT_SYMBOL(sec_input_dumpkey_register);

void sec_input_dumpkey_unregister(int dev_id)
{
	if (dev_id < NUM_DEVICES && dev_id >= MULTI_DEV_NONE) {
		dump_callbacks[dev_id].inform_dump = NULL;
		dump_callbacks[dev_id].ptsp = NULL;
	}
}
EXPORT_SYMBOL(sec_input_dumpkey_unregister);

static const struct input_device_id sec_tsp_dumpkey_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
	},
	{},
};

static struct input_handler sec_tsp_dumpkey_handler = {
	.event		= sec_tsp_dumpkey_event,
	.connect	= sec_tsp_dumpkey_connect,
	.disconnect	= sec_tsp_dumpkey_disconnect,
	.name		= "sec_tsp_dumpkey",
	.id_table	= sec_tsp_dumpkey_ids,
};

int sec_tsp_dumpkey_init(void)
{
	int err;
	size_t i;

	pr_info("%s %s\n", SECLOG, __func__);

	for (i = 0; i < KEY_MAX; i++)
		atomic_set(&(sec_tsp_dumpkey_acceptable_event[i]), 0);

	for (i = 0; i < ARRAY_SIZE(__tsp_dump_keys); i++)
		update_acceptable_event(__tsp_dump_keys[i], true);

	spin_lock_init(&sec_tsp_dumpkey_event_lock);

	dump_callbacks = kzalloc(sizeof(struct tsp_dump_callbacks) * NUM_DEVICES, GFP_KERNEL);
	if (!dump_callbacks)
		return -ENOMEM;

	err = input_register_handler(&sec_tsp_dumpkey_handler);

	return err;

}
EXPORT_SYMBOL(sec_tsp_dumpkey_init);

void sec_tsp_dumpkey_exit(void)
{
	input_unregister_handler(&sec_tsp_dumpkey_handler);
	kfree(dump_callbacks);
}
EXPORT_SYMBOL(sec_tsp_dumpkey_exit);

MODULE_DESCRIPTION("Samsung tsp dumpkey driver");
MODULE_LICENSE("GPL");
