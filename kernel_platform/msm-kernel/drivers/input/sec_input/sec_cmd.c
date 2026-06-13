// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sec_cmd.h"
#include "sec_input.h"
#include "sec_tsp_log.h"

struct class *tsp_sec_class;

#if IS_ENABLED(CONFIG_SEC_KUNIT)
__visible_for_testing struct sec_cmd_data *kunit_sec;
EXPORT_SYMBOL(kunit_sec);
#endif

const char *str_power_state[3] = { "OFF", "LP", "ON" };
const char *str_use_case[CHECK_ALL + 1] = {
	"NONE",		// 0
	"OFF",		// 1	CHECK_POWEROFF
	"LP",		// 2	CHECK_LPMODE
	"LP/OFF",	// 3	CHECK_POWEROFF | CHECK_LPMODE
	"ON",		// 4	CHECK_POWERON
	"ON/OFF",	// 5	CHECK_POWEROFF | CHECK_POWERON
	"ON/LP",	// 6	CHECK_ON_LP
	"ON/LP/OFF",	// 7	CHECK_ALL
};

#ifdef USE_SEC_CMD_QUEUE
static void cmd_store_function(struct sec_cmd_data *data);
void sec_cmd_execution(struct sec_cmd_data *data, bool lock)
{
	if (lock)
		mutex_lock(&data->fs_lock);

	/* check lock	*/
	mutex_lock(&data->cmd_lock);
	atomic_set(&data->cmd_is_running, 1);
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = SEC_CMD_STATUS_RUNNING;
	cmd_store_function(data);

	if (lock)
		mutex_unlock(&data->fs_lock);
}
#endif

void sec_cmd_set_cmd_exit(struct sec_cmd_data *data)
{
#ifdef USE_SEC_CMD_QUEUE
	mutex_lock(&data->fifo_lock);
	if (kfifo_len(&data->cmd_queue)) {
		input_info(true, data->fac_dev, "%s: do next cmd, left cmd[%d]\n", __func__,
			(int)(kfifo_len(&data->cmd_queue) / sizeof(struct command)));
		mutex_unlock(&data->fifo_lock);

		sec_cmd_execution(data, false);
	} else {
		mutex_unlock(&data->fifo_lock);

		mutex_lock(&data->cmd_lock);
		atomic_set(&data->cmd_is_running, 0);
		mutex_unlock(&data->cmd_lock);
	}
	if (data->wait_cmd_result_done)
		complete_all(&data->cmd_result_done);
#else
	mutex_lock(&data->cmd_lock);
	atomic_set(&data->cmd_is_running, 0);
	mutex_unlock(&data->cmd_lock);
#endif
}
EXPORT_SYMBOL(sec_cmd_set_cmd_exit);

#ifdef USE_SEC_CMD_QUEUE
static void cmd_exit_work(struct work_struct *work)
{
	struct sec_cmd_data *data = container_of(work, struct sec_cmd_data, cmd_work.work);

	sec_cmd_execution(data, true);
}
#endif

void sec_cmd_set_default_result(struct sec_cmd_data *data)
{
	char *delim = ":";

	memset(data->cmd_result, 0x00, SEC_CMD_RESULT_STR_LEN_EXPAND);
	memcpy(data->cmd_result, data->cmd, SEC_CMD_STR_LEN);
	strlcat(data->cmd_result, delim, SEC_CMD_RESULT_STR_LEN_EXPAND);
	data->cmd_result_expand = 0;
	data->cmd_result_expand_count = 0;
}
EXPORT_SYMBOL(sec_cmd_set_default_result);

void sec_cmd_set_cmd_result_all(struct sec_cmd_data *data, char *buff, int len, char *item)
{
	char *delim1 = " ";
	char *delim2 = ":";
	int cmd_result_len;

	cmd_result_len = (int)strlen(data->cmd_result_all) + len + 2 + (int)strlen(item);

	if (cmd_result_len >= SEC_CMD_RESULT_STR_LEN) {
		input_err(true, data->fac_dev, "%s: cmd length is over (%d)!!", __func__, cmd_result_len);
		return;
	}

	data->item_count++;
	strlcat(data->cmd_result_all, delim1, sizeof(data->cmd_result_all));
	strlcat(data->cmd_result_all, item, sizeof(data->cmd_result_all));
	strlcat(data->cmd_result_all, delim2, sizeof(data->cmd_result_all));
	strlcat(data->cmd_result_all, buff, sizeof(data->cmd_result_all));
}
EXPORT_SYMBOL(sec_cmd_set_cmd_result_all);

void sec_cmd_set_cmd_result(struct sec_cmd_data *data, char *buff, int len)
{
	int cmd_result_len = (int)strlen(data->cmd_result) + (int)strlen(buff);

	if (cmd_result_len >= (unsigned int)SEC_CMD_RESULT_STR_LEN_EXPAND) {
		input_err(true, data->fac_dev, "%s: cmd length is over (%d)!!", __func__, cmd_result_len);
		strlcat(data->cmd_result, "NG", SEC_CMD_RESULT_STR_LEN_EXPAND);
		return;
	}

	data->cmd_result_expand = cmd_result_len / SEC_CMD_RESULT_STR_LEN;
	data->cmd_result_expand_count = 0;

	strlcat(data->cmd_result, buff, SEC_CMD_RESULT_STR_LEN_EXPAND);
}
EXPORT_SYMBOL(sec_cmd_set_cmd_result);

void sec_cmd_check_store_condition(struct sec_cmd_data *data, struct sec_cmd *sec_cmd_ptr)
{
	struct sec_ts_plat_data *plat_data;
	int prev_result_len = 0;

	if (data->dev == NULL) {
		input_err(true, data->fac_dev, "%s: device is NULL, need plat_data\n", __func__);
		sec_cmd_set_default_result(data);
		prev_result_len = (int)strlen(data->cmd_result);
		sec_cmd_ptr->cmd_func(data);
		goto out;
	}
	plat_data = data->dev->platform_data;

	sec_cmd_set_default_result(data);
	prev_result_len = (int)strlen(data->cmd_result);

	if (sec_input_cmp_ic_status(data->dev, sec_cmd_ptr->cmd_use_cases)) {
		sec_cmd_ptr->cmd_func(data);
	} else {
		input_info(true, data->fac_dev, "current_power:%s, not %s%s\n",
			str_power_state[atomic_read(&plat_data->power_state)],
			str_use_case[sec_cmd_ptr->cmd_use_cases],
			sec_cmd_ptr->cmd_func_forced ? ", call force_func" : "");
		if (sec_cmd_ptr->cmd_func_forced)
			sec_cmd_ptr->cmd_func_forced(data);
		else
			goto CMD_NG;
	}

out:
	if (prev_result_len == (int)strlen(data->cmd_result)) {
		if ((data->cmd_state == SEC_CMD_STATUS_WAITING) || (data->cmd_state == SEC_CMD_STATUS_OK))
			sec_cmd_set_cmd_result(data, "OK", SEC_CMD_RESULT_STR_LEN_EXPAND);
		else if (data->cmd_state == SEC_CMD_STATUS_NOT_APPLICABLE)
			sec_cmd_set_cmd_result(data, "NA", SEC_CMD_RESULT_STR_LEN_EXPAND);
		else
			sec_cmd_set_cmd_result(data, "NG", SEC_CMD_RESULT_STR_LEN_EXPAND);
	}

	if (sec_cmd_ptr->wait_read_result == EXIT_RESULT) {
		data->cmd_state = SEC_CMD_STATUS_WAITING;
		sec_cmd_set_cmd_exit(data);
	}

	return;
CMD_NG:
	data->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(data, "NG", SEC_CMD_RESULT_STR_LEN_EXPAND);

	if (sec_cmd_ptr->wait_read_result == EXIT_RESULT)
		sec_cmd_set_cmd_exit(data);
}

#ifndef USE_SEC_CMD_QUEUE
__visible_for_testing ssize_t cmd_store(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct sec_cmd_data *data = dev_get_drvdata(dev);
	char *cur, *start, *end;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int len, i;
	struct sec_cmd *sec_cmd_ptr = NULL;
	char delim = ',';
	bool cmd_found = false;
	int param_cnt = 0;

	if (!data) {
		input_err(true, dev, "%s: No platform data found\n", __func__);
		return -EINVAL;
	}

	if (strnlen(buf, SEC_CMD_STR_LEN) >= SEC_CMD_STR_LEN) {
		input_err(true, data->fac_dev, "%s: cmd length(strlen(buf)) is over (%d,%s)!!\n",
				__func__, (int)strlen(buf), buf);
		return -EINVAL;
	}

	if (count >= (unsigned int)SEC_CMD_STR_LEN) {
		input_err(true, data->fac_dev, "%s: cmd length(count) is over (%d,%s)!!\n",
				__func__, (unsigned int)count, buf);
		return -EINVAL;
	}

	if (atomic_read(&data->cmd_is_running)) {
		input_err(true, data->fac_dev, "%s: other cmd is running.\n", __func__);
		return -EBUSY;
	}

	/* check lock   */
	mutex_lock(&data->cmd_lock);
	atomic_set(&data->cmd_is_running, 1);
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = SEC_CMD_STATUS_RUNNING;
	for (i = 0; i < ARRAY_SIZE(data->cmd_param); i++)
		data->cmd_param[i] = 0;

	len = (int)count;
	if (*(buf + len - 1) == '\n')
		len--;

	memset(data->cmd, 0x00, ARRAY_SIZE(data->cmd));
	memcpy(data->cmd, buf, len);

	cur = strchr(buf, (int)delim);
	if (cur)
		memcpy(buff, buf, cur - buf);
	else
		memcpy(buff, buf, len);

	input_dbg(false, data->fac_dev, "%s: COMMAND = %s\n", __func__, buff);

	/* find command */
	list_for_each_entry(sec_cmd_ptr, &data->cmd_list_head, list) {
		if (!strncmp(buff, sec_cmd_ptr->cmd_name, SEC_CMD_STR_LEN)) {
			if (!sec_cmd_ptr->not_support_cmds) {
				cmd_found = true;
				break;
			}
			input_err(true, data->fac_dev, "%s: [%s] is in not_support_cmds list\n", __func__, buff);
		}
	}

check_not_support_cmd:
	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(sec_cmd_ptr, &data->cmd_list_head, list) {
			if (!strncmp("not_support_cmd", sec_cmd_ptr->cmd_name,
				SEC_CMD_STR_LEN))
				break;
		}
	}

	/* parsing parameters */
	if (cur && cmd_found) {
		cur++;
		start = cur;
		memset(buff, 0x00, ARRAY_SIZE(buff));

		do {
			if (*cur == delim || cur - buf == len) {
				end = cur;
				memcpy(buff, start, end - start);
				*(buff + strnlen(buff, ARRAY_SIZE(buff))) = '\0';
				if (kstrtoint(buff, 10, data->cmd_param + param_cnt) < 0) {
					input_err(true, data->fac_dev, "%s: error to parse parameter\n", __func__);
					cmd_found = false;
					goto check_not_support_cmd;
				}
				start = cur + 1;
				memset(buff, 0x00, ARRAY_SIZE(buff));
				param_cnt++;
			}
			cur++;
		} while ((cur - buf <= len) && (param_cnt < SEC_CMD_PARAM_NUM));
	}

	if (cmd_found) {
		char dbuff[13];
		char tdbuff[13 * SEC_CMD_PARAM_NUM] = { 0 };

		for (i = 0; i < param_cnt; i++) {
			snprintf(dbuff, sizeof(dbuff), "%d ", data->cmd_param[i]);
			strlcat(tdbuff, dbuff, sizeof(tdbuff));
		}

		if (param_cnt == 0)
			snprintf(tdbuff, sizeof(tdbuff), "none");

		input_info(true, data->fac_dev, "%s: cmd = %s param = %s%s\n", __func__, sec_cmd_ptr->cmd_name,
			tdbuff, sec_cmd_ptr->wait_read_result ? " (wait)" : "");
	} else {
		input_info(false, data->fac_dev, "%s: cmd = %s(%s)\n", __func__, buff, sec_cmd_ptr->cmd_name);
	}

	if (sec_cmd_ptr->cmd_use_cases)
		sec_cmd_check_store_condition(data, sec_cmd_ptr);
	else
		sec_cmd_ptr->cmd_func(data);

	return count;
}
#if IS_ENABLED(CONFIG_SEC_KUNIT)
EXPORT_SYMBOL_KUNIT(cmd_store);
#endif

#else	/* defined USE_SEC_CMD_QUEUE */
static void cmd_store_function(struct sec_cmd_data *data)
{
	char *cur, *start, *end;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int len, i;
	struct sec_cmd *sec_cmd_ptr = NULL;
	char delim = ',';
	bool cmd_found = false;
	int param_cnt = 0;
	int ret;
	const char *buf;
	size_t count;
	struct command cmd = {{0}};

	if (!data) {
		input_err(true, NULL, "%s: No platform data found\n", __func__);
		return;
	}

	mutex_lock(&data->fifo_lock);
	if (kfifo_len(&data->cmd_queue)) {
		ret = kfifo_out(&data->cmd_queue, &cmd, sizeof(struct command));
		if (!ret) {
			input_err(true, data->fac_dev, "%s: kfifo_out failed, it seems empty, ret=%d\n", __func__, ret);
			mutex_unlock(&data->fifo_lock);
			return;
		}
	} else {
		input_err(true, data->fac_dev, "%s: left cmd is nothing\n", __func__);
		mutex_unlock(&data->fifo_lock);
		mutex_lock(&data->cmd_lock);
		atomic_set(&data->cmd_is_running, 0);
		mutex_unlock(&data->cmd_lock);
		return;
	}
	mutex_unlock(&data->fifo_lock);

	buf = cmd.cmd;
	count = strlen(buf);

	for (i = 0; i < (int)ARRAY_SIZE(data->cmd_param); i++)
		data->cmd_param[i] = 0;

	len = (int)count;
	if (*(buf + len - 1) == '\n')
		len--;

	memset(data->cmd, 0x00, ARRAY_SIZE(data->cmd));
	memcpy(data->cmd, buf, len);

	cur = strchr(buf, (int)delim);
	if (cur)
		memcpy(buff, buf, cur - buf);
	else
		memcpy(buff, buf, len);

	input_dbg(false, data->fac_dev, "%s: COMMAND : %s\n", __func__, buff);

	/* find command */
	list_for_each_entry(sec_cmd_ptr, &data->cmd_list_head, list) {
		if (!strncmp(buff, sec_cmd_ptr->cmd_name, SEC_CMD_STR_LEN)) {
			if (!sec_cmd_ptr->not_support_cmds) {
				cmd_found = true;
				break;
			}
			input_err(true, data->fac_dev, "%s: [%s] is in not_support_cmds list\n", __func__, buff);
		}
	}

check_not_support_cmd:
	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(sec_cmd_ptr, &data->cmd_list_head, list) {
			if (!strncmp("not_support_cmd", sec_cmd_ptr->cmd_name,
				SEC_CMD_STR_LEN))
				break;
		}
	}

	/* parsing parameters */
	if (cur && cmd_found) {
		cur++;
		start = cur;
		memset(buff, 0x00, ARRAY_SIZE(buff));

		do {
			if (*cur == delim || cur - buf == len) {
				end = cur;
				memcpy(buff, start, end - start);
				*(buff + strnlen(buff, ARRAY_SIZE(buff))) = '\0';
				if (kstrtoint(buff, 10, data->cmd_param + param_cnt) < 0) {
					input_err(true, data->fac_dev, "%s: error to parse parameter\n", __func__);
					cmd_found = false;
					goto check_not_support_cmd;
				}
				start = cur + 1;
				memset(buff, 0x00, ARRAY_SIZE(buff));
				param_cnt++;
			}
			cur++;
		} while ((cur - buf <= len) && (param_cnt < SEC_CMD_PARAM_NUM));
	}

	if (cmd_found) {
		char dbuff[13];
		char tdbuff[13 * SEC_CMD_PARAM_NUM] = { 0 };

		for (i = 0; i < param_cnt; i++) {
			snprintf(dbuff, sizeof(dbuff), "%d ", data->cmd_param[i]);
			strlcat(tdbuff, dbuff, sizeof(tdbuff));
		}

		if (param_cnt == 0)
			snprintf(tdbuff, sizeof(tdbuff), "none");

		input_info(true, data->fac_dev, "%s: cmd = %s param = %s%s\n", __func__, sec_cmd_ptr->cmd_name,
			tdbuff, sec_cmd_ptr->wait_read_result ? " (wait)" : "");
	} else {
		input_info(false, data->fac_dev, "%s: cmd = %s(%s)\n", __func__, buff, sec_cmd_ptr->cmd_name);
	}

	if (sec_cmd_ptr->cmd_use_cases)
		sec_cmd_check_store_condition(data, sec_cmd_ptr);
	else
		sec_cmd_ptr->cmd_func(data);

	if (cmd_found && sec_cmd_ptr->cmd_log) {
		char tbuf[32];
		unsigned long long t;
		unsigned long nanosec_rem;

		memset(tbuf, 0x00, sizeof(tbuf));
		t = local_clock();
		nanosec_rem = do_div(t, 1000000000);
		snprintf(tbuf, sizeof(tbuf), "[r:%lu.%06lu]",
				(unsigned long)t,
				nanosec_rem / 1000);
#if IS_ENABLED(CONFIG_SEC_DEBUG_TSP_LOG)
		sec_debug_tsp_command_history(tbuf);
#endif
	}
}

__visible_for_testing ssize_t cmd_store(struct device *dev, struct device_attribute *devattr,
			   const char *buf, size_t count)
{
	struct sec_cmd_data *data = dev_get_drvdata(dev);
	struct command cmd = {{0}};
	struct sec_cmd *sec_cmd_ptr = NULL;
	int queue_size;

	if (!data) {
		input_err(true, dev, "%s: No platform data found\n", __func__);
		return -EINVAL;
	}

	if (strnlen(buf, SEC_CMD_STR_LEN) >= SEC_CMD_STR_LEN) {
		input_err(true, data->fac_dev, "%s: cmd length(strlen(buf)) is over (%d,%s)!!\n",
				__func__, (int)strlen(buf), buf);
		return -EINVAL;
	}

	if (count >= (unsigned int)SEC_CMD_STR_LEN) {
		input_err(true, data->fac_dev, "%s: cmd length(count) is over (%d,%s)!!\n",
				__func__, (unsigned int)count, buf);
		return -EINVAL;
	}

	if (strnlen(buf, SEC_CMD_STR_LEN) == 0) {
		input_err(true, data->fac_dev, "%s: cmd length is zero (%d,%s) count(%ld)!!\n",
				__func__, (int)strlen(buf), buf, count);
		return -EINVAL;
	}

	strncpy(cmd.cmd, buf, count);

	if (data->wait_cmd_result_done) {
		int ret;

		mutex_lock(&data->wait_lock);
		if (!data->cmd_result_done.done)
			input_info(true, data->fac_dev, "%s: %s - waiting prev cmd...\n", __func__, cmd.cmd);
		ret = wait_for_completion_interruptible_timeout(&data->cmd_result_done, msecs_to_jiffies(2000));
		if (ret <= 0)
			input_err(true, data->fac_dev, "%s: completion %d\n", __func__, ret);

		reinit_completion(&data->cmd_result_done);
		mutex_unlock(&data->wait_lock);
	}

	list_for_each_entry(sec_cmd_ptr, &data->cmd_list_head, list) {
		if (!strncmp(cmd.cmd, sec_cmd_ptr->cmd_name, strlen(sec_cmd_ptr->cmd_name))) {
			if (sec_cmd_ptr->cmd_log) {
				char task_info[40];
				char tbuf[32];
				unsigned long long t;
				unsigned long nanosec_rem;

				memset(tbuf, 0x00, sizeof(tbuf));
				t = local_clock();
				nanosec_rem = do_div(t, 1000000000);
				snprintf(tbuf, sizeof(tbuf), "[q:%lu.%06lu]",
						(unsigned long)t,
						nanosec_rem / 1000);

				snprintf(task_info, 40, "\n[%d:%s:%s]", current->pid, current->comm, dev_name(data->fac_dev));
#if IS_ENABLED(CONFIG_SEC_DEBUG_TSP_LOG)
				sec_debug_tsp_command_history(task_info);
				sec_debug_tsp_command_history(cmd.cmd);
				sec_debug_tsp_command_history(tbuf);
#endif
			}
			break;
		}
	}

	mutex_lock(&data->fifo_lock);
	queue_size = (kfifo_len(&data->cmd_queue) / sizeof(struct command));

	if (kfifo_avail(&data->cmd_queue) && (queue_size < SEC_CMD_MAX_QUEUE)) {
		kfifo_in(&data->cmd_queue, &cmd, sizeof(struct command));
		input_info(false, data->fac_dev, "%s: push cmd: %s\n", __func__, cmd.cmd);
	} else {
		input_err(true, data->fac_dev, "%s: cmd_queue is full!!\n", __func__);

		kfifo_reset(&data->cmd_queue);
		input_err(true, data->fac_dev, "%s: cmd_queue is reset!!\n", __func__);
		mutex_unlock(&data->fifo_lock);

		mutex_lock(&data->cmd_lock);
		atomic_set(&data->cmd_is_running, 0);
		mutex_unlock(&data->cmd_lock);

		if (data->wait_cmd_result_done)
			complete_all(&data->cmd_result_done);

		return -ENOSPC;
	}

	if (atomic_read(&data->cmd_is_running)) {
		input_err(true, data->fac_dev, "%s: other cmd is running. Wait until previous cmd is done[%d]\n",
			__func__, (int)(kfifo_len(&data->cmd_queue) / sizeof(struct command)));
		mutex_unlock(&data->fifo_lock);
		return count;
	}
	mutex_unlock(&data->fifo_lock);

	sec_cmd_execution(data, true);
	return count;
}
#if IS_ENABLED(CONFIG_SEC_KUNIT)
EXPORT_SYMBOL_KUNIT(cmd_store);
#endif
#endif

__visible_for_testing ssize_t cmd_status_show(struct device *dev,
				 struct device_attribute *devattr, char *buf)
{
	struct sec_cmd_data *data = dev_get_drvdata(dev);
	char buff[16] = { 0 };

	if (!data) {
		input_err(true, dev, "%s: No platform data found\n", __func__);
		return -EINVAL;
	}

	if (data->cmd_state == SEC_CMD_STATUS_WAITING)
		snprintf(buff, sizeof(buff), "WAITING");

	else if (data->cmd_state == SEC_CMD_STATUS_RUNNING)
		snprintf(buff, sizeof(buff), "RUNNING");

	else if (data->cmd_state == SEC_CMD_STATUS_OK)
		snprintf(buff, sizeof(buff), "OK");

	else if (data->cmd_state == SEC_CMD_STATUS_FAIL)
		snprintf(buff, sizeof(buff), "FAIL");

	else if (data->cmd_state == SEC_CMD_STATUS_EXPAND)
		snprintf(buff, sizeof(buff), "EXPAND");

	else if (data->cmd_state == SEC_CMD_STATUS_NOT_APPLICABLE)
		snprintf(buff, sizeof(buff), "NOT_APPLICABLE");

	input_dbg(false, data->fac_dev, "%s: %d, %s\n", __func__, data->cmd_state, buff);

	return snprintf(buf, sizeof(buff), "%s\n", buff);
}
#if IS_ENABLED(CONFIG_SEC_KUNIT)
EXPORT_SYMBOL_KUNIT(cmd_status_show);
#endif

__visible_for_testing ssize_t cmd_status_all_show(struct device *dev,
				 struct device_attribute *devattr, char *buf)
{
	struct sec_cmd_data *data = dev_get_drvdata(dev);
	char buff[16] = { 0 };

	if (!data) {
		input_err(true, dev, "%s: No platform data found\n", __func__);
		return -EINVAL;
	}

	if (data->cmd_all_factory_state == SEC_CMD_STATUS_WAITING)
		snprintf(buff, sizeof(buff), "WAITING");

	else if (data->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		snprintf(buff, sizeof(buff), "RUNNING");

	else if (data->cmd_all_factory_state == SEC_CMD_STATUS_OK)
		snprintf(buff, sizeof(buff), "OK");

	else if (data->cmd_all_factory_state == SEC_CMD_STATUS_FAIL)
		snprintf(buff, sizeof(buff), "FAIL");

	else if (data->cmd_state == SEC_CMD_STATUS_EXPAND)
		snprintf(buff, sizeof(buff), "EXPAND");

	else if (data->cmd_all_factory_state == SEC_CMD_STATUS_NOT_APPLICABLE)
		snprintf(buff, sizeof(buff), "NOT_APPLICABLE");

	input_dbg(false, data->fac_dev, "%s: %d, %s\n", __func__, data->cmd_all_factory_state, buff);

	return snprintf(buf, sizeof(buff), "%s\n", buff);
}
#if IS_ENABLED(CONFIG_SEC_KUNIT)
EXPORT_SYMBOL_KUNIT(cmd_status_all_show);
#endif

__visible_for_testing ssize_t cmd_result_show(struct device *dev,
				 struct device_attribute *devattr, char *buf)
{
	struct sec_cmd_data *data = dev_get_drvdata(dev);
	int size;

	if (!data) {
		input_err(true, dev, "%s: No platform data found\n", __func__);
		return -EINVAL;
	}

	size = snprintf(buf, SEC_CMD_RESULT_STR_LEN, "%s\n",
		data->cmd_result + (SEC_CMD_RESULT_STR_LEN - 1) * data->cmd_result_expand_count);

	if (data->cmd_result_expand_count != data->cmd_result_expand) {
		data->cmd_state = SEC_CMD_STATUS_EXPAND;
		data->cmd_result_expand_count++;
	} else {
		data->cmd_state = SEC_CMD_STATUS_WAITING;
	}

	input_info(false, data->fac_dev, "%s: %s", __func__, buf);

	sec_cmd_set_cmd_exit(data);

	return size;
}
#if IS_ENABLED(CONFIG_SEC_KUNIT)
EXPORT_SYMBOL_KUNIT(cmd_result_show);
#endif

__visible_for_testing ssize_t cmd_result_all_show(struct device *dev,
				 struct device_attribute *devattr, char *buf)
{
	struct sec_cmd_data *data = dev_get_drvdata(dev);
	int size;

	if (!data) {
		input_err(true, dev, "%s: No platform data found\n", __func__);
		return -EINVAL;
	}

	data->cmd_state = SEC_CMD_STATUS_WAITING;
	input_info(false, data->fac_dev, "%s: %d, %s\n", __func__, data->item_count, data->cmd_result_all);
	size = snprintf(buf, SEC_CMD_RESULT_STR_LEN, "%d%s\n", data->item_count, data->cmd_result_all);

	sec_cmd_set_cmd_exit(data);

	data->item_count = 0;
	memset(data->cmd_result_all, 0x00, SEC_CMD_RESULT_STR_LEN);

	return size;
}
#if IS_ENABLED(CONFIG_SEC_KUNIT)
EXPORT_SYMBOL_KUNIT(cmd_result_all_show);
#endif

__visible_for_testing ssize_t cmd_list_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *data = dev_get_drvdata(dev);
	struct sec_cmd *sec_cmd_ptr = NULL;
	char *buffer;
	char buffer_name[SEC_CMD_STR_LEN];
	int ret = 0;

	buffer = kzalloc(data->cmd_buffer_size + 30, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	snprintf(buffer, 30, "++factory command list++\n");

	list_for_each_entry(sec_cmd_ptr, &data->cmd_list_head, list) {
		if (sec_cmd_ptr->not_support_cmds)
			continue;
		if (strncmp(sec_cmd_ptr->cmd_name, "not_support_cmd", 15)) {
			snprintf(buffer_name, SEC_CMD_STR_LEN, "%s\n", sec_cmd_ptr->cmd_name);
			strlcat(buffer, buffer_name, data->cmd_buffer_size + 30);
		}
	}

	ret = snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buffer);
	kfree(buffer);

	return ret;
}
#if IS_ENABLED(CONFIG_SEC_KUNIT)
EXPORT_SYMBOL_KUNIT(cmd_list_show);
#endif

static DEVICE_ATTR(cmd, 0220, NULL, cmd_store);
static DEVICE_ATTR_RO(cmd_status);
static DEVICE_ATTR_RO(cmd_status_all);
static DEVICE_ATTR_RO(cmd_result);
static DEVICE_ATTR_RO(cmd_result_all);
static DEVICE_ATTR_RO(cmd_list);

static struct attribute *sec_fac_attrs[] = {
	&dev_attr_cmd.attr,
	&dev_attr_cmd_status.attr,
	&dev_attr_cmd_status_all.attr,
	&dev_attr_cmd_result.attr,
	&dev_attr_cmd_result_all.attr,
	&dev_attr_cmd_list.attr,
	NULL,
};

static struct attribute_group sec_fac_attr_group = {
	.attrs = sec_fac_attrs,
};

static ssize_t prox_power_off_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_plat_data *plat_data = sec->dev->platform_data;

	input_info(true, sec->dev, "%s: %d\n", __func__, plat_data->prox_power_off);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", plat_data->prox_power_off);
}

static ssize_t prox_power_off_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_plat_data *plat_data = sec->dev->platform_data;
	long data;
	int ret;

	ret = kstrtol(buf, 10, &data);
	if (ret < 0)
		return ret;

	input_info(true, sec->dev, "%s: %ld\n", __func__, data);

	plat_data->prox_power_off = data;

	return count;
}

static ssize_t support_feature_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_plat_data *plat_data = sec->dev->platform_data;
	u32 feature = 0;

	if (plat_data->enable_settings_aot)
		feature |= INPUT_FEATURE_ENABLE_SETTINGS_AOT;

	if (plat_data->support_vrr)
		feature |= INPUT_FEATURE_ENABLE_VRR;

	if (plat_data->support_open_short_test)
		feature |= INPUT_FEATURE_SUPPORT_OPEN_SHORT_TEST;

	if (plat_data->support_mis_calibration_test)
		feature |= INPUT_FEATURE_SUPPORT_MIS_CALIBRATION_TEST;

	if (plat_data->support_wireless_tx)
		feature |= INPUT_FEATURE_SUPPORT_WIRELESS_TX;

	if (plat_data->enable_sysinput_enabled)
		feature |= INPUT_FEATURE_ENABLE_SYSINPUT_ENABLED;

	if (plat_data->prox_lp_scan_enabled)
		feature |= INPUT_FEATURE_ENABLE_PROX_LP_SCAN_ENABLED;

	if (plat_data->support_input_monitor)
		feature |= INPUT_FEATURE_SUPPORT_INPUT_MONITOR;

	if (plat_data->support_rawdata)
		feature |= INPUT_FEATURE_SUPPORT_RAWDATA_TRANSFER;

	if (plat_data->support_rawdata_motion_aivf)
		feature |= INPUT_FEATURE_SUPPORT_MOTION_AIVF;

	if (plat_data->support_rawdata_motion_palm)
		feature |= INPUT_FEATURE_SUPPORT_MOTION_PALM;

	if (plat_data->support_rawdata_pocket_detect)
		feature |= INPUT_FEATURE_SUPPORT_POCKET_DETECT;

	if (plat_data->support_rawdata_awd)
		feature |= INPUT_FEATURE_SUPPORT_MOTION_AWD;

	input_info(true, sec->dev, "%s: %d%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
			__func__, feature,
			feature & INPUT_FEATURE_ENABLE_SETTINGS_AOT ? " aot" : "",
			feature & INPUT_FEATURE_ENABLE_PRESSURE ? " pressure" : "",
			feature & INPUT_FEATURE_ENABLE_VRR ? " vrr" : "",
			feature & INPUT_FEATURE_SUPPORT_OPEN_SHORT_TEST ? " openshort" : "",
			feature & INPUT_FEATURE_SUPPORT_MIS_CALIBRATION_TEST ? " miscal" : "",
			feature & INPUT_FEATURE_SUPPORT_WIRELESS_TX ? " wirelesstx" : "",
			feature & INPUT_FEATURE_SUPPORT_INPUT_MONITOR ? " inputmonitor" : "",
			feature & INPUT_FEATURE_ENABLE_SYSINPUT_ENABLED ? " SE" : "",
			feature & INPUT_FEATURE_ENABLE_PROX_LP_SCAN_ENABLED ? " LPSCAN" : "",
			feature & INPUT_FEATURE_SUPPORT_RAWDATA_TRANSFER ? " RAW" : "",
			feature & INPUT_FEATURE_SUPPORT_MOTION_AIVF ? " AIVF" : "",
			feature & INPUT_FEATURE_SUPPORT_MOTION_PALM ? " PALM" : "",
			feature & INPUT_FEATURE_SUPPORT_POCKET_DETECT ? " POCKET" : "",
			feature & INPUT_FEATURE_SUPPORT_MOTION_AWD ? " AWD" : "");

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", feature);
}

static ssize_t enabled_show(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_plat_data *plat_data = sec->dev->platform_data;

	if (!plat_data->enable_sysinput_enabled)
		return -EINVAL;

	input_info(true, sec->dev, "%s: enabled %d\n", __func__, atomic_read(&plat_data->enabled));

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", atomic_read(&plat_data->enabled));
}

ssize_t sec_cmd_enabled_show(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	return enabled_show(dev, attr, buf);
}
#if IS_ENABLED(CONFIG_SEC_KUNIT)
EXPORT_SYMBOL_KUNIT(sec_cmd_enabled_show);
#endif

static ssize_t enabled_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_plat_data *plat_data = sec->dev->platform_data;
	int buff[2];
	int ret;

	if (!plat_data->enable_sysinput_enabled)
		return -EINVAL;

	ret = sscanf(buf, "%d,%d", &buff[0], &buff[1]);
	if (ret != 2) {
		input_err(true, sec->dev,
				"%s: failed read params [%d]\n", __func__, ret);
		return -EINVAL;
	}

	if (buff[0] == DISPLAY_STATE_ON && buff[1] == DISPLAY_EVENT_LATE) {
		input_info(true, sec->dev, "%s: DISPLAY_STATE_ON\n", __func__);
		if (atomic_read(&plat_data->enabled)) {
			input_err(true, sec->dev, "%s: device already enabled\n", __func__);
			goto out;
		}
		ret = sec_input_enable_device(plat_data->dev);
	} else if (buff[0] == DISPLAY_STATE_OFF && buff[1] == DISPLAY_EVENT_EARLY) {
		input_info(true, sec->dev, "%s: DISPLAY_STATE_OFF\n", __func__);
		if (!atomic_read(&plat_data->enabled)) {
			input_err(true, sec->dev, "%s: device already disabled\n", __func__);
			goto out;
		}
		ret = sec_input_disable_device(plat_data->dev);
	} else if (buff[0] == DISPLAY_STATE_FORCE_ON) {
		input_info(true, sec->dev, "%s: DISPLAY_STATE_FORCE_ON\n", __func__);
		if (atomic_read(&plat_data->enabled)) {
			input_err(true, sec->dev, "%s: device already enabled\n", __func__);
			goto out;
		}
		ret = sec_input_enable_device(plat_data->dev);
	} else if (buff[0] == DISPLAY_STATE_FORCE_OFF || buff[0] == DISPLAY_STATE_LPM_OFF) {
		input_info(true, sec->dev, "%s: %s\n", __func__, buff[0] == DISPLAY_STATE_FORCE_OFF ?
				"DISPLAY_STATE_FORCE_OFF" : "DISPLAY_STATE_LPM_OFF");
		if (!atomic_read(&plat_data->enabled)) {
			input_err(true, sec->dev, "%s: device already disabled\n", __func__);
			goto out;
		}
		ret = sec_input_disable_device(plat_data->dev);
	}

	if (ret < 0)
		return ret;

out:
	return count;
}

ssize_t sec_cmd_enabled_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	return enabled_store(dev, attr, buf, count);
}
#if IS_ENABLED(CONFIG_SEC_KUNIT)
EXPORT_SYMBOL_KUNIT(sec_cmd_enabled_store);
#endif

static DEVICE_ATTR_RW(prox_power_off);
static DEVICE_ATTR_RO(support_feature);
static DEVICE_ATTR_RW(enabled);

static struct attribute *sec_fac_common_attrs[] = {
	&dev_attr_prox_power_off.attr,
	&dev_attr_support_feature.attr,
	NULL,
};

static struct attribute_group sec_fac_common_attr_group = {
	.attrs = sec_fac_common_attrs,
};

static struct attribute *sec_fac_enabled_attrs[] = {
	&dev_attr_enabled.attr,
	NULL
};

static const struct attribute_group sec_fac_enabled_attr_group = {
	.attrs = sec_fac_enabled_attrs,
};

static int sec_cmd_enabled_sysfs_create(struct device *dev)
{
	struct kernfs_node *enabled_sd = NULL;
	int retval = 0;

	enabled_sd = sysfs_get_dirent(dev->kobj.sd, "enabled");
	if (IS_ERR_OR_NULL(enabled_sd)) {
		input_info(true, dev, "%s: make 'enabled' node\n", __func__);
		retval = sysfs_create_group(&dev->kobj, &sec_fac_enabled_attr_group);
		if (retval < 0) {
			input_err(true, dev, "%s: Failed to create enabled sysfs: %d\n", __func__, retval);
		}
	} else {
		input_dbg(true, dev, "%s: 'enabled' is already exist\n", __func__);
	}

	return retval;
}

static void sec_cmd_parse_dt_not_support_cmds(struct sec_cmd_data *data)
{
	struct device *dev;
	struct device_node *np;
	struct sec_cmd *sec_cmd_ptr = NULL;
	int count = 0;
	int ii = 0;
	int ret = 0;

	if (data->dev)
		dev = data->dev;
	else
		return;

	if (dev->of_node)
		np = dev->of_node;
	else
		return;

	count = of_property_count_strings(np, "sec_cmd,not_support_cmds");
	for (ii = 0; ii < count; ii++) {
		const char *buff;

		ret = of_property_read_string_index(np, "sec_cmd,not_support_cmds", ii, &buff);
		if (ret < 0 || strlen(buff) <= 0) {
			input_err(true, dev, "%s: failed get sec_cmd,not_support_cmds: %d, %lu\n", __func__, ret, strlen(buff));
			return;
		}
		list_for_each_entry(sec_cmd_ptr, &data->cmd_list_head, list) {
			if (!strncmp(buff, sec_cmd_ptr->cmd_name, SEC_CMD_STR_LEN)) {
				sec_cmd_ptr->not_support_cmds = true;
			}
		}

		input_info(true, dev, "%s: sec_cmd,not_support_cmds(%d): %s\n", __func__, ii, buff);
	}
}

int sec_cmd_init(struct sec_cmd_data *data, struct device *dev, struct sec_cmd *cmds,
			int len, int devt, struct attribute_group *vendor_attr_group)
{
	const char *device_name;
	int ret, i;

	switch (devt) {
	case SEC_CLASS_DEVT_TSP:
		device_name = SEC_CLASS_DEV_NAME_TSP;
		break;
	case SEC_CLASS_DEVT_TSP1:
		device_name = SEC_CLASS_DEV_NAME_TSP1;
		break;
	case SEC_CLASS_DEVT_TSP2:
		device_name = SEC_CLASS_DEV_NAME_TSP2;
		break;
	case SEC_CLASS_DEVT_TKEY:
		device_name = SEC_CLASS_DEV_NAME_TKEY;
		break;
	case SEC_CLASS_DEVT_WACOM:
		device_name = SEC_CLASS_DEV_NAME_WACOM;
		break;
	case SEC_CLASS_DEVT_SIDEKEY:
		device_name = SEC_CLASS_DEV_NAME_SIDEKEY;
		break;
	default:
		input_err(true, NULL, "%s: not defined devt=%d\n", __func__, devt);
		return -ENODEV;
	}

	INIT_LIST_HEAD(&data->cmd_list_head);

	data->cmd_buffer_size = 0;
	for (i = 0; i < len; i++) {
		list_add_tail(&cmds[i].list, &data->cmd_list_head);
		if (cmds[i].cmd_name)
			data->cmd_buffer_size += strlen(cmds[i].cmd_name) + 1;
	}

	mutex_init(&data->cmd_lock);
	mutex_init(&data->fs_lock);

	mutex_lock(&data->cmd_lock);
	atomic_set(&data->cmd_is_running, 0);
	mutex_unlock(&data->cmd_lock);

	data->cmd_result = kzalloc(SEC_CMD_RESULT_STR_LEN_EXPAND, GFP_KERNEL);
	if (!data->cmd_result)
		goto err_alloc_cmd_result;

	if (!IS_ERR_OR_NULL(dev))
		data->dev = dev;

#ifdef USE_SEC_CMD_QUEUE
	if (kfifo_alloc(&data->cmd_queue,
		SEC_CMD_MAX_QUEUE * sizeof(struct command), GFP_KERNEL)) {
		input_err(true, NULL, "%s: %s: failed to alloc queue for cmd\n", device_name, __func__);
		goto err_alloc_queue;
	}
	mutex_init(&data->fifo_lock);
	mutex_init(&data->wait_lock);
	init_completion(&data->cmd_result_done);
	complete_all(&data->cmd_result_done);

	INIT_DELAYED_WORK(&data->cmd_work, cmd_exit_work);
#endif

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	data->fac_dev = sec_device_create(data, device_name);
#else
#if (KERNEL_VERSION(6, 4, 0) <= LINUX_VERSION_CODE)
	tsp_sec_class = class_create(device_name);
#else
	tsp_sec_class = class_create(THIS_MODULE, device_name);
#endif
	if (IS_ERR(tsp_sec_class)) {
		input_err(true, NULL, "%s: %s: Failed to create class(sec) %ld\n",
			device_name, __func__, PTR_ERR(tsp_sec_class));
		return PTR_ERR(tsp_sec_class);
	}
	data->fac_dev = device_create(tsp_sec_class, NULL, devt, data, "%s", device_name);
#endif

	if (IS_ERR(data->fac_dev)) {
		input_err(true, NULL, "%s: %s: failed to create device for the sysfs\n", device_name, __func__);
		goto err_sysfs_device;
	}

	dev_set_drvdata(data->fac_dev, data);

	sec_cmd_parse_dt_not_support_cmds(data);

	ret = sysfs_create_group(&data->fac_dev->kobj, &sec_fac_attr_group);
	if (ret < 0) {
		input_err(true, data->fac_dev, "%s: failed to create sysfs group\n", __func__);
		goto err_sysfs_group;
	}
	input_info(true, data->fac_dev, "%s create sec_fac_attr_group: done\n", __func__);

	if (!IS_ERR_OR_NULL(vendor_attr_group)) {
		ret = sysfs_create_group(&data->fac_dev->kobj, vendor_attr_group);
		if (ret < 0) {
			input_err(true, data->fac_dev, "%s: failed to create sysfs group\n", __func__);
			goto err_vendor_sysfs_group;
		}
		data->vendor_attr_group = vendor_attr_group;
		input_info(true, data->fac_dev, "%s: create vendor_attr_group: done\n", __func__);
	}

	if (!IS_ERR_OR_NULL(dev)) {
		/* if you do not use sec_ts_plat_data, should invoke sec_cmd_init_without_platdata */
		ret = sysfs_create_group(&data->fac_dev->kobj, &sec_fac_common_attr_group);
		if (ret < 0) {
			input_err(true, data->fac_dev, "%s: failed to create sec_fac_common_attr_group\n", __func__);
			goto err_common_sysfs_group;
		}
		input_info(true, data->fac_dev, "%s: create sec_fac_common_attr_group: done\n", __func__);

		ret = sec_cmd_enabled_sysfs_create(data->fac_dev);
		if (ret < 0) {
			input_err(true, data->fac_dev, "%s: failed to sec_cmd_enabled_sysfs_create\n", __func__);
			sysfs_remove_group(&data->fac_dev->kobj, &sec_fac_common_attr_group);
			goto err_common_sysfs_group;
		}
	}

	sec_cmd_io_init(data, devt);

	input_info(true, data->fac_dev, "%s: done\n", __func__);

	sec_cmd_send_event_to_user(data, NULL, "RESULT=PROBE_DONE");

	return 0;

err_common_sysfs_group:
	if (!IS_ERR_OR_NULL(data->vendor_attr_group))
		sysfs_remove_group(&data->fac_dev->kobj, data->vendor_attr_group);
err_vendor_sysfs_group:
	sysfs_remove_group(&data->fac_dev->kobj, &sec_fac_attr_group);
err_sysfs_group:
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	sec_device_destroy(data->fac_dev->devt);
#else
	device_destroy(tsp_sec_class, devt);
#endif
err_sysfs_device:
#ifdef USE_SEC_CMD_QUEUE
	mutex_destroy(&data->fifo_lock);
	kfifo_free(&data->cmd_queue);
	mutex_destroy(&data->wait_lock);
err_alloc_queue:
#endif
	kfree(data->cmd_result);
err_alloc_cmd_result:
	mutex_destroy(&data->cmd_lock);
	list_del(&data->cmd_list_head);
	input_fail_hist(true, data->fac_dev, "%s: failed\n", __func__);

	return -ENODEV;
}
EXPORT_SYMBOL(sec_cmd_init);

/*
 * sec_cmd_init_without_platdata
 *
 * If device driver doesn't use sec_ts_plat_data as platform_data, you should init cmds with this function.
 * and should make sysfs like sec_fac_common_attrs on device driver side
 */
int sec_cmd_init_without_platdata(struct sec_cmd_data *data, struct sec_cmd *cmds,
			int len, int devt, struct attribute_group *vendor_attr_group)
{
	return sec_cmd_init(data, NULL, cmds, len, devt, vendor_attr_group);
}
EXPORT_SYMBOL(sec_cmd_init_without_platdata);

void sec_cmd_exit(struct sec_cmd_data *data, int devt)
{
#ifdef USE_SEC_CMD_QUEUE
	struct command cmd = {{0}};
	int ret;
#endif

	if (data->fac_dev == NULL) {
		input_info(true, NULL, "%s: fac_dev is already NULL\n", __func__);
		return;
	}

	input_info(true, data->fac_dev, "%s\n", __func__);

	if (!IS_ERR_OR_NULL(data->fac_dev))
		sysfs_remove_group(&data->fac_dev->kobj, &sec_fac_common_attr_group);
	if (!IS_ERR_OR_NULL(data->vendor_attr_group))
		sysfs_remove_group(&data->fac_dev->kobj, data->vendor_attr_group);
	sysfs_remove_group(&data->fac_dev->kobj, &sec_fac_attr_group);
	dev_set_drvdata(data->fac_dev, NULL);
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	sec_device_destroy(data->fac_dev->devt);
#else
	device_destroy(tsp_sec_class, devt);
#endif
#ifdef USE_SEC_CMD_QUEUE
	mutex_lock(&data->fifo_lock);
	while (kfifo_len(&data->cmd_queue)) {
		ret = kfifo_out(&data->cmd_queue, &cmd, sizeof(struct command));
		if (!ret)
			input_err(true, data->fac_dev, "%s: kfifo_out failed, it seems empty, ret=%d\n", __func__, ret);
		input_info(true, data->fac_dev, "%s: remove pending commands: %s", __func__, cmd.cmd);
	}
	mutex_unlock(&data->fifo_lock);
	mutex_destroy(&data->fifo_lock);
	kfifo_free(&data->cmd_queue);
	mutex_destroy(&data->wait_lock);

	cancel_delayed_work_sync(&data->cmd_work);
	flush_delayed_work(&data->cmd_work);
#endif
	data->fac_dev = NULL;
	kfree(data->cmd_result);
	mutex_destroy(&data->cmd_lock);
	list_del(&data->cmd_list_head);

}
EXPORT_SYMBOL(sec_cmd_exit);

void sec_cmd_send_event_to_user(struct sec_cmd_data *data, char *test, char *result)
{
	char *event[5];
	char timestamp[32];
	char feature[32];
	char stest[32];
	char sresult[64];
	ktime_t calltime;
	u64 realtime;
	int curr_time;
	char *eol = "\0";

	if (!data || !data->fac_dev)
		return;

	calltime = ktime_get();
	realtime = ktime_to_ns(calltime);
	do_div(realtime, NSEC_PER_USEC);
	curr_time = realtime / USEC_PER_MSEC;

	snprintf(timestamp, 32, "TIMESTAMP=%d", curr_time);
	strncat(timestamp, eol, 1);
	snprintf(feature, 32, "FEATURE=TSP");
	strncat(feature, eol, 1);
	if (!test)
		snprintf(stest, 32, "TEST=NULL");
	else
		snprintf(stest, 32, "%s", test);

	strncat(stest, eol, 1);

	if (!result)
		snprintf(sresult, 64, "RESULT=NULL");
	else
		snprintf(sresult, 64, "%s", result);

	strncat(sresult, eol, 1);

	input_info(true, data->fac_dev, "%s: time:%s, feature:%s, test:%s, result:%s\n",
			__func__, timestamp, feature, stest, sresult);

	event[0] = timestamp;
	event[1] = feature;
	event[2] = stest;
	event[3] = sresult;
	event[4] = NULL;

	kobject_uevent_env(&data->fac_dev->kobj, KOBJ_CHANGE, event);
}
EXPORT_SYMBOL(sec_cmd_send_event_to_user);

void sec_cmd_send_status_uevent(struct sec_cmd_data *data, enum sec_cmd_status_uevent_type type, int value)
{
	char test[32] = { 0 };
	char result[32] = { 0 };

	switch (type) {
	case STATUS_TYPE_WET:
		snprintf(test, sizeof(test), "STATUS=WET");
		break;
	case STATUS_TYPE_NOISE:
		snprintf(test, sizeof(test), "STATUS=NOISE");
		break;
	case STATUS_TYPE_FREQ:
		snprintf(test, sizeof(test), "STATUS=FREQ");
		break;
	default:
		input_info(true, data->fac_dev, "%s: undefined type %d\n", __func__, type);
		return;
	}

	snprintf(result, sizeof(result), "VALUE=%d", value);
	sec_cmd_send_event_to_user(data, test, result);
}
EXPORT_SYMBOL(sec_cmd_send_status_uevent);

void sec_cmd_send_gesture_uevent(struct sec_cmd_data *data, int type, int x, int y)
{
	struct sec_ts_plat_data *plat_data;
	char buff[SEC_TS_GESTURE_REPORT_BUFF_SIZE] = { 0 };
	char test[32] = { 0 };
	char result[32] = { 0 };

	if (!data || !data->dev || !data->fac_dev) {
		pr_info("%s %s: data or data->dev or data->facdev is null.\n", SECLOG, __func__);
		return;
	}

	plat_data = data->dev->platform_data;

	snprintf(test, sizeof(test), "GESTURE=%d", type);
	snprintf(result, sizeof(result), "POS=%d,%d", x, y);

	sec_cmd_send_event_to_user(data, test, result);

	if (type == SPONGE_EVENT_TYPE_SPAY) {
		snprintf(buff, sizeof(buff), "SPAY");
		plat_data->hw_param.all_spay_count++;
	} else if (type == SPONGE_EVENT_TYPE_SINGLE_TAP) {
		snprintf(buff, sizeof(buff), "SINGLE TAP");
	} else if (type == SPONGE_EVENT_TYPE_AOD_DOUBLETAB) {
		snprintf(buff, sizeof(buff), "AOD");
		plat_data->hw_param.all_aod_tap_count++;
	} else if (type == SPONGE_EVENT_TYPE_FOD_PRESS) {
		snprintf(buff, sizeof(buff), "FOD PRESS");
	} else if (type == SPONGE_EVENT_TYPE_FOD_RELEASE) {
		snprintf(buff, sizeof(buff), "FOD RELEASE");
	} else if (type == SPONGE_EVENT_TYPE_FOD_OUT) {
		snprintf(buff, sizeof(buff), "FOD OUT");
	} else if (type == SPONGE_EVENT_TYPE_TSP_SCAN_UNBLOCK) {
		snprintf(buff, sizeof(buff), "SCAN UNBLOCK");
	} else if (type == SPONGE_EVENT_TYPE_TSP_SCAN_BLOCK) {
		snprintf(buff, sizeof(buff), "SCAN BLOCK");
	} else if (type == SPONGE_EVENT_TYPE_LONG_PRESS) {
		snprintf(buff, sizeof(buff), "LONG PRESS");
	} else {
		return;
	}

	input_info(true, data->fac_dev, "%s: %s\n", __func__, buff);
}
EXPORT_SYMBOL(sec_cmd_send_gesture_uevent);

MODULE_DESCRIPTION("Samsung input command");
MODULE_LICENSE("GPL");

