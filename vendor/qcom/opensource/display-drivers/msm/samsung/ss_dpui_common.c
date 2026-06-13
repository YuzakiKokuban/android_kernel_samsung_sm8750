/*
 * Samsung Common LCD DPUI(display use info) LOGGING Driver.
 *
 * Copyright (c) 2017 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/fb.h>
#include <linux/notifier.h>
#include <linux/export.h>
#include "ss_dpui_common.h"

/*
 * DPUI : display use info (panel common info)
 * DPCI : display controller info (ap dependent info)
 */
static BLOCKING_NOTIFIER_HEAD(dpui_notifier_list);
static BLOCKING_NOTIFIER_HEAD(dpci_notifier_list);
static DEFINE_MUTEX(dpui_lock);

/**
 * dpui_logging_notify - notify clients of fb_events
 * @val: dpui log type
 * @v : data
 *
 */
void dpui_logging_notify(unsigned long val, enum dpui_type type, void *v)
{
	if (type == DPUI_TYPE_CTRL)
		blocking_notifier_call_chain(&dpci_notifier_list, val, v);
	else
		blocking_notifier_call_chain(&dpui_notifier_list, val, v);
}
EXPORT_SYMBOL_GPL(dpui_logging_notify);

/**
 *	dpui_logging_register - register a client notifier
 *	@n: notifier block to callback on events
 */
int dpui_logging_register(struct notifier_block *n, enum dpui_type type)
{
	int ret;

	if (type <= DPUI_TYPE_NONE || type >= MAX_DPUI_TYPE) {
		pr_err("%s out of dpui_type range (%d)\n", __func__, type);
		return -EINVAL;
	}

	if (type == DPUI_TYPE_CTRL)
		ret = blocking_notifier_chain_register(&dpci_notifier_list, n);
	else
		ret = blocking_notifier_chain_register(&dpui_notifier_list, n);
	if (ret < 0) {
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
				__func__, ret);
		return ret;
	}

	pr_info("%s register type %s\n", __func__, dpui_type_name[type]);
	return 0;
}
EXPORT_SYMBOL_GPL(dpui_logging_register);

/**
 *	dpui_logging_unregister - unregister a client notifier
 *	@n: notifier block to callback on events
 */
int dpui_logging_unregister(struct notifier_block *n)
{
	return blocking_notifier_chain_unregister(&dpui_notifier_list, n);
}
EXPORT_SYMBOL_GPL(dpui_logging_unregister);

static bool is_dpui_var_u32(enum dpui_key key)
{
	return (dpui.field[key].var_type == DPUI_VAR_U32);
}

void update_dpui_log(enum dpui_log_level level, enum dpui_type type)
{
	if (level < 0 || level >= MAX_DPUI_LOG_LEVEL) {
		pr_err("%s invalid log level %d\n", __func__, level);
		return;
	}

	dpui_logging_notify(level, type, &dpui);
	pr_info("%s update dpui log(%d) done\n",
			__func__, level);
}

int clear_dpui_log(enum dpui_log_level level, enum dpui_type type)
{
	int i;
	int ret = 0;

	if (level < 0 || level >= MAX_DPUI_LOG_LEVEL) {
		pr_err("%s invalid log level %d\n", __func__, level);
		return -EINVAL;
	}

	mutex_lock(&dpui_lock);
	for (i = 0; i < ARRAY_SIZE(dpui.field); i++) {
		if (dpui.field[i].type != type)
			continue;
		if (dpui.field[i].auto_clear)
			dpui.field[i].initialized = false;
	}
	mutex_unlock(&dpui_lock);

	pr_info("%s clear dpui log(%d) done\n",
			__func__, level);

	return ret;
}

static int __get_dpui_field(enum dpui_key key, char *buf)
{
	if (!buf) {
		pr_err("%s buf is null\n", __func__);
		return 0;
	}

	if (!DPUI_VALID_KEY(key)) {
		pr_err("%s out of dpui_key range (%d)\n", __func__, key);
		return 0;
	}

	if (!dpui.field[key].initialized) {
		pr_debug("%s DPUI:%s not initialized, so use default value\n",
				__func__, dpui_key_name[key]);
		return snprintf(buf, MAX_DPUI_KEY_LEN + MAX_DPUI_VAL_LEN,
			"\"%s\":\"%s\"", dpui_key_name[key], dpui.field[key].default_value);
	}

	return snprintf(buf, MAX_DPUI_KEY_LEN + MAX_DPUI_VAL_LEN,
			"\"%s\":\"%s\"", dpui_key_name[key], dpui.field[key].buf);
}

void print_dpui_field(enum dpui_key key)
{
	char tbuf[MAX_DPUI_KEY_LEN + MAX_DPUI_VAL_LEN];

	if (!DPUI_VALID_KEY(key)) {
		pr_err("%s out of dpui_key range (%d)\n", __func__, key);
		return;
	}

	__get_dpui_field(key, tbuf);
	pr_info("DPUI: %s\n", tbuf);
}

static int __set_dpui_field(enum dpui_key key, char *buf, int size)
{
	if (!buf) {
		pr_err("%s buf is null\n", __func__);
		return -EINVAL;
	}

	if (!DPUI_VALID_KEY(key)) {
		pr_err("%s out of dpui_key range (%d)\n", __func__, key);
		return -EINVAL;
	}

	if (size > MAX_DPUI_VAL_LEN - 1) {
		pr_err("%s exceed dpui value size (%d)\n", __func__, size);
		return -EINVAL;
	}
	memcpy(dpui.field[key].buf, buf, size);
	dpui.field[key].buf[size] = '\0';
	dpui.field[key].initialized = true;

	return 0;
}

static int __get_dpui_u32_field(enum dpui_key key, u32 *value)
{
	int rc, cur_val;

	if (value == NULL) {
		pr_err("%s invalid value pointer\n", __func__);
		return -EINVAL;
	}

	if (!DPUI_VALID_KEY(key)) {
		pr_err("%s out of dpui_key range (%d)\n", __func__, key);
		return -EINVAL;
	}

	rc = kstrtouint(dpui.field[key].buf, (unsigned int)0, &cur_val);
	if (rc < 0) {
		pr_err("%s failed to get value\n", __func__);
		return rc;
	}

	*value = cur_val;

	return 0;
}

static int __set_dpui_u32_field(enum dpui_key key, u32 value)
{
	char tbuf[MAX_DPUI_VAL_LEN];
	int size;

	if (!DPUI_VALID_KEY(key)) {
		pr_err("%s out of dpui_key range (%d)\n", __func__, key);
		return -EINVAL;
	}

	if (!is_dpui_var_u32(key)) {
		pr_err("%s invalid type %d\n", __func__, dpui.field[key].var_type);
		return -EINVAL;
	}

	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%u", value);
	if (size > MAX_DPUI_VAL_LEN) {
		pr_err("%s exceed dpui value size (%d)\n", __func__, size);
		return -EINVAL;
	}
	__set_dpui_field(key, tbuf, size);

	return 0;
}

static int __inc_dpui_u32_field(enum dpui_key key, u32 value)
{
	int ret;
	u32 cur_val = 0;

	if (!DPUI_VALID_KEY(key)) {
		pr_err("%s out of dpui_key range (%d)\n", __func__, key);
		return -EINVAL;
	}

	if (!is_dpui_var_u32(key)) {
		pr_err("%s invalid type %d\n", __func__, dpui.field[key].var_type);
		return -EINVAL;
	}

	if (dpui.field[key].initialized) {
		ret = __get_dpui_u32_field(key, &cur_val);
		if (ret < 0) {
			pr_err("%s failed to get u32 field (%d)\n", __func__, ret);
			return -EINVAL;
		}
	}

	__set_dpui_u32_field(key, cur_val + value);

	return 0;
}

int get_dpui_field(enum dpui_key key, char *buf)
{
	int ret;

	mutex_lock(&dpui_lock);
	ret = __get_dpui_field(key, buf);
	mutex_unlock(&dpui_lock);

	return ret;
}

int set_dpui_field(enum dpui_key key, char *buf, int size)
{
	int ret;

	mutex_lock(&dpui_lock);
	ret = __set_dpui_field(key, buf, size);
	mutex_unlock(&dpui_lock);

	return ret;
}

int get_dpui_u32_field(enum dpui_key key, u32 *value)
{
	int ret;

	mutex_lock(&dpui_lock);
	ret = __get_dpui_u32_field(key, value);
	mutex_unlock(&dpui_lock);

	return ret;
}

int set_dpui_u32_field(enum dpui_key key, u32 value)
{
	int ret;

	mutex_lock(&dpui_lock);
	ret = __set_dpui_u32_field(key, value);
	mutex_unlock(&dpui_lock);

	return ret;
}

int inc_dpui_u32_field(enum dpui_key key, u32 value)
{
	int ret;

	mutex_lock(&dpui_lock);
	ret = __inc_dpui_u32_field(key, value);
	mutex_unlock(&dpui_lock);

	return ret;
}
EXPORT_SYMBOL(inc_dpui_u32_field);

int inc_dpui_u32_field_nolock(enum dpui_key key, u32 value)
{
	return __inc_dpui_u32_field(key, value);
}

int __get_dpui_log(char *buf, enum dpui_log_level level, enum dpui_type type)
{
	int i, ret, len = 0;
	char tbuf[MAX_DPUI_KEY_LEN + MAX_DPUI_VAL_LEN];

	if (!buf) {
		pr_err("%s buf is null\n", __func__);
		return -EINVAL;
	}

	if (level < 0 || level >= MAX_DPUI_LOG_LEVEL) {
		pr_err("%s invalid log level %d\n", __func__, level);
		return -EINVAL;
	}

	mutex_lock(&dpui_lock);
	for (i = DPUI_KEY_NONE + 1; i < MAX_DPUI_KEY; i++) {
		if (level != DPUI_LOG_LEVEL_ALL && dpui.field[i].level != level) {
			pr_warn("%s DPUI:%s different log level %d %d\n",
					__func__, dpui_key_name[dpui.field[i].key],
					dpui.field[i].level, level);
			continue;
		}

		if (type != dpui.field[i].type)
			continue;

		ret = __get_dpui_field(i, tbuf);
		if (ret == 0)
			continue;

		if (len)
			len += snprintf(buf + len, 3, ",");
		len += snprintf(buf + len, MAX_DPUI_KEY_LEN + MAX_DPUI_VAL_LEN,
				"%s", tbuf);
	}
	mutex_unlock(&dpui_lock);

	return len;
}

int get_dpui_log(char *buf, enum dpui_log_level level, enum dpui_type type)
{
	return __get_dpui_log(buf, level, type);
}
EXPORT_SYMBOL_GPL(get_dpui_log);
