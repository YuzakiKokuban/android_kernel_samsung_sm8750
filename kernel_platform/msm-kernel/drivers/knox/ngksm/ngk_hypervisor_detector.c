/*
 * Copyright (c) 2020-2021 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/kthread.h>
#include <linux/ngksm.h>
#ifdef CONFIG_NGKSM_UH
#include <linux/uh.h>
#endif
#include <linux/fcntl.h>
#include <linux/errno.h>
#include "ngksm_common.h"
#include "ngksm_netlink.h"
#include <linux/unistd.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/ctype.h>

#define PAD_NOT_ENABLED           0x1000
#define PAD_NOT_SUPPORTED         0x2000
#define PAD_EXECLUDE              0x3000
#define PAD_RELEASE_EXECLUDE      0x4000

#define PAD_HYP_CMD               7
#define PAD_HYP_GET_STATUS        8

#define PAD_BIT_MASK              0xFFF
#define PAD_MODE_MASK             0xF000
#define MAX_PAD_RETURN            0x2000
#define MAX_PAD_BIT               0x3FF

#define PAD_NODE_DIR              "ker"
#define PAD_CMD                   "PAD|"
#define PAD_EXECLUDE_CMD          "EX|"
#define PAD_RELEASE_EXECLUDE_CMD  "REX|"
#define PAD_MAX_CMD               9

#ifndef CONFIG_NGKSM_UH
#define UH_APP_HDM                6

unsigned long uh_call(u64 app_id, u64 command, u64 arg0, u64 arg1, u64 arg2, u64 arg3)
{
	return 0;
}
#endif

static int hyp_detector(void *data);

static struct kobject *pad_kobj;

struct hyp_detector_thread_t {
	struct task_struct *polling_thread;
	bool is_started;
	u64 applied_subsystem;
};

static struct hyp_detector_thread_t hyp_detector_thread = {NULL, false};

static void apply_execlude_policy(const u64 policy, const u64 mode)
{
	u64 hyp_return = 0;
	u64 subsystem = policy | mode;
	u64 pad_bit = 0;
	int ret = -1;

	NGKSM_LOG_INFO("%s", __func__);

	uh_call(UH_APP_HDM, PAD_HYP_CMD, (u64)&hyp_return, (u64)subsystem, 0, 0);

	if (hyp_return > MAX_PAD_RETURN) {
		NGKSM_LOG_ERROR("Wrong return, It over the MAX_PAD_RETURN");
		return;
	}

	pad_bit = hyp_return & PAD_BIT_MASK;
	if (pad_bit > MAX_PAD_BIT) {
		NGKSM_LOG_ERROR("Wrong pad bit, It over the MAX_PAD_BIT");
		return;
	}

	if (pad_bit != 0) {
		ret = ngksm_send_message("HYP_DET", "status change detected", (int64_t)pad_bit);
		if (ret != NGKSM_SUCCESS)
			NGKSM_LOG_ERROR("send message failed");
	}
	return;

}


static void onetime_hyp_status_check(void)
{
	u64 pad_bit = 0;
	u64 status = 0;
	int ret = -1;

	NGKSM_LOG_INFO("onetime_hyp_status_check");

	status = 0;
	uh_call(UH_APP_HDM, PAD_HYP_GET_STATUS, (u64)&status, 0, 0, 0);
	if (status > MAX_PAD_RETURN) {
		NGKSM_LOG_ERROR("Wrong return, It over the MAX_PAD_RETURN");
		return;
	}

	pad_bit = status & PAD_BIT_MASK;
	if (pad_bit > MAX_PAD_BIT) {
		NGKSM_LOG_ERROR("Wrong pad bit, It over the MAX_PAD_BIT");
		return;
	}

	if (pad_bit != 0) {
		ret = ngksm_send_message("HYP_DET", "status change detected", (int64_t)pad_bit);
		if (ret != NGKSM_SUCCESS)
			NGKSM_LOG_ERROR("send message failed");
	}
	return;

}

int cmd_hyp_detector(const long subsystem)
{
	u64 hyp_return = 0;
	u64 pad_bit = 0;
	u64 pad_support_bit = 0;

	if (!ngksm_daemon_ready()) {
		NGKSM_LOG_ERROR("Native daemon is not ready\n");
		return -1;
	}

	if (subsystem < 0) {
		NGKSM_LOG_INFO("Hyp detector - wrong subsystem value\n");
		return -1;
	}


	if (hyp_detector_thread.is_started && hyp_detector_thread.applied_subsystem == subsystem) {
		NGKSM_LOG_INFO("Hyp detector already started as 0x%x\n", subsystem);
		return 0;
	}

	if (!hyp_detector_thread.is_started && subsystem == 0) {
		NGKSM_LOG_INFO("Hyp detector already stopped\n");
		return 0;
	}


	uh_call(UH_APP_HDM, PAD_HYP_CMD, (u64)&hyp_return, (u64)subsystem, 0, 0);

	NGKSM_LOG_DEBUG("Hyp detector - returned value is %d\n", hyp_return);
	pad_bit = hyp_return & PAD_BIT_MASK;
	pad_support_bit = hyp_return & PAD_MODE_MASK;

	if (pad_support_bit == PAD_NOT_SUPPORTED) {
		NGKSM_LOG_INFO("Hyp detector not supported\n");
		return 0;
	}

	if (pad_bit <= 0x3FF && pad_bit > 0) {
		if (!hyp_detector_thread.is_started) {
			hyp_detector_thread.polling_thread = kthread_run(hyp_detector, NULL, "hyp detector");
			if (IS_ERR(hyp_detector_thread.polling_thread)) {
				NGKSM_LOG_ERROR("Failed to start hyp detector thread\n");
				return -1;
			}
		}
		hyp_detector_thread.is_started = true;
		hyp_detector_thread.applied_subsystem = pad_bit;
		return 0;
	}

	if (pad_bit == 0) {
		if (!hyp_detector_thread.is_started) {
			NGKSM_LOG_INFO("Hyp detector already stopped\n");
			return 0;
		}

		NGKSM_LOG_INFO("Hyp detector stop thread\n");
		hyp_detector_thread.is_started = false;
		hyp_detector_thread.applied_subsystem = 0;
		if (IS_ERR(hyp_detector_thread.polling_thread)) {
			NGKSM_LOG_ERROR("Hyp detector polling_thread is ERROR\n");
			return -1;
		}
		kthread_stop(hyp_detector_thread.polling_thread);
		onetime_hyp_status_check();
		return 0;
	}

	NGKSM_LOG_ERROR("Hyp detector returned value is not matched\n");
	return -1;
}

static ssize_t store_pad_cmd(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t len)
{
	long num;
	char temp_buf[PAD_MAX_CMD];
	int real_len = 0;
	int i;

	if (!len || len > PAD_MAX_CMD || len == 0) {
		NGKSM_LOG_ERROR("cmd len check fail len = %zd", len);
		return -EINVAL;
	}

	for (i = 0; i < strlen(buf); i++) {
		if (isalpha(buf[i]) || isdigit(buf[i]) || buf[i] == '|')
			real_len++;
	}

	memcpy(temp_buf, buf, real_len);
	temp_buf[real_len] = '\0';

	if (!strncmp(buf, PAD_CMD, strlen(PAD_CMD))) {
		if (!kstrtol(&temp_buf[strlen(PAD_CMD)], 10, &num)) {
			NGKSM_LOG_DEBUG("pad_cmd policy = %d", num);
			if (num >= 0)
				cmd_hyp_detector(num);
			else
				NGKSM_LOG_DEBUG("pad_cmd policy = %d, wrong policy", num);
		} else
			NGKSM_LOG_INFO("pad_cmd policy validation error %s", &temp_buf[strlen(PAD_CMD)]);
	} else if (!strncmp(buf, PAD_EXECLUDE_CMD, strlen(PAD_EXECLUDE_CMD))) {
		if (!kstrtol(&temp_buf[strlen(PAD_EXECLUDE_CMD)], 10, &num)) {
			NGKSM_LOG_DEBUG("pad_cmd execlude policy = %d", num);
			if (num > 0)
				apply_execlude_policy((u64)num, PAD_EXECLUDE);
			else
				NGKSM_LOG_DEBUG("pad_cmd execlude policy = %d, wrong policy", num);
		} else
			NGKSM_LOG_INFO("pad_cmd execlude policy validation error %s",
				&temp_buf[strlen(PAD_EXECLUDE_CMD)]);
	} else if (!strncmp(buf, PAD_RELEASE_EXECLUDE_CMD, strlen(PAD_RELEASE_EXECLUDE_CMD))) {
		if (!kstrtol(&temp_buf[strlen(PAD_RELEASE_EXECLUDE_CMD)], 10, &num)) {
			NGKSM_LOG_DEBUG("pad_cmd release execlude policy = %d", num);
			if (num > 0)
				apply_execlude_policy((u64)num, PAD_RELEASE_EXECLUDE);
			else
				NGKSM_LOG_DEBUG("pad_cmd release execlude policy = %d, wrong policy", num);
		} else {
			NGKSM_LOG_INFO("pad_cmd release execlude policy validation error %s",
				&temp_buf[strlen(PAD_EXECLUDE_CMD)]);
		}
	} else
		NGKSM_LOG_INFO("CMD not matched");

	NGKSM_LOG_DEBUG("%s, buf = %s", __func__, buf);
	NGKSM_LOG_DEBUG("%s, len = %zd", __func__, len);
	NGKSM_LOG_DEBUG("%s, real_len = %d", __func__, real_len);


	return len;
}

static ssize_t show_state(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	if (hyp_detector_thread.is_started == true)
		return sprintf(buf, "Running, Subsystem = 0x%llx\n", hyp_detector_thread.applied_subsystem);
	else
		return sprintf(buf, "Stopped\n");
}


static struct kobj_attribute pad_attribute =
	__ATTR(pad_cmd, 0600, NULL, store_pad_cmd);

static struct kobj_attribute state_attribute =
	__ATTR(state, 0400, show_state, NULL);


static struct attribute *attrs[] = {
	&pad_attribute.attr,
	&state_attribute.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};



static int hyp_detector(void *data)
{
	u64 pad_bit = 0;
	u64 status = 0;
	int ret = -1;

	NGKSM_LOG_INFO("%s", __func__);

	while (!kthread_should_stop()) {
		NGKSM_LOG_DEBUG("loop start");
		status = 0;
		uh_call(UH_APP_HDM, PAD_HYP_GET_STATUS, (u64)&status, 0, 0, 0);
		if (status > MAX_PAD_RETURN) {
			NGKSM_LOG_ERROR("Wrong return, It over the MAX_PAD_RETURN");
			break;
		}

		pad_bit = status & PAD_BIT_MASK;
		if (pad_bit > MAX_PAD_BIT) {
			NGKSM_LOG_ERROR("Wrong pad bit, It over the MAX_PAD_BIT");
			break;
		}

		if (pad_bit != 0) {
			ret = ngksm_send_message("HYP_DET", "status change detected", (int64_t)pad_bit);
			if (ret != NGKSM_SUCCESS)
				NGKSM_LOG_ERROR("Send message failed");
		}
		//msleep_interruptible(30000);//1000 - 1s
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(msecs_to_jiffies(60000));
	}
	set_current_state(TASK_RUNNING);
	NGKSM_LOG_DEBUG("%s, Stop loop", __func__);
	return NGKSM_SUCCESS;
}

int __init ngk_hyp_detector_init(void)
{
	int result;

	NGKSM_LOG_INFO("%s", __func__);

	pad_kobj = kobject_create_and_add(PAD_NODE_DIR, kernel_kobj);

	if (!pad_kobj) {
		NGKSM_LOG_ERROR("Failed to create sysfs directory\n");
		return -ENOMEM;
	}

	result = sysfs_create_group(pad_kobj, &attr_group);

	if (result) {
		NGKSM_LOG_ERROR("Failed to create sysfs files\n");
		kobject_put(pad_kobj);
		return result;
	}

	return NGKSM_SUCCESS;
}

void __exit ngk_hyp_detector_exit(void)
{
	u64 result;

	NGKSM_LOG_INFO("%s", __func__);

	kobject_put(pad_kobj);

	if (!hyp_detector_thread.is_started) {
		NGKSM_LOG_DEBUG("Hyp detector already stopped\n");
		return;
	}

	uh_call(UH_APP_HDM, PAD_HYP_CMD, (u64)&result, 0, 0, 0);
	kthread_stop(hyp_detector_thread.polling_thread);
	hyp_detector_thread.is_started = false;
	hyp_detector_thread.applied_subsystem = 0;
}
