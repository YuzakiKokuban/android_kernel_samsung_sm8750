/* SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2022 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SEC_CMD_H_
#define _SEC_CMD_H_
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <linux/input.h>
#include <linux/sched/clock.h>
#include <linux/uaccess.h>
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
#include <linux/sec_class.h>
#endif

#if !IS_ENABLED(CONFIG_SEC_FACTORY)
#define USE_SEC_CMD_QUEUE
#include <linux/kfifo.h>
#endif

#define SEC_CLASS_DEVT_TSP		10
#define SEC_CLASS_DEVT_TKEY		11
#define SEC_CLASS_DEVT_WACOM		12
#define SEC_CLASS_DEVT_SIDEKEY		13
#define SEC_CLASS_DEV_NAME_TSP		"tsp"
#define SEC_CLASS_DEV_NAME_TKEY		"sec_touchkey"
#define SEC_CLASS_DEV_NAME_WACOM	"sec_epen"
#define SEC_CLASS_DEV_NAME_SIDEKEY	"sec_sidekey"

#define SEC_CLASS_DEVT_TSP1		15
#define SEC_CLASS_DEVT_TSP2		16
#define SEC_CLASS_DEV_NAME_TSP1		"tsp1"
#define SEC_CLASS_DEV_NAME_TSP2		"tsp2"

#define SEC_CMD(name, func)		.cmd_name = name, .cmd_func = func
#define SEC_CMD_H(name, func)		.cmd_name = name, .cmd_func = func, .cmd_log = 1
#define SEC_CMD_V2(name, func, force_func, use_cases, wait_result) \
					.cmd_name = name, .cmd_func = func, .cmd_func_forced = force_func, \
					.cmd_use_cases = use_cases, .wait_read_result = wait_result
#define SEC_CMD_V2_H(name, func, force_func, use_cases, wait_result) \
					.cmd_name = name, .cmd_func = func, .cmd_func_forced = force_func, \
					.cmd_use_cases = use_cases, .wait_read_result = wait_result, .cmd_log = 1

#define SEC_CMD_BUF_SIZE		(4096 - 1)
#define SEC_CMD_STR_LEN			256
#define SEC_CMD_RESULT_STR_LEN		(4096 - 1)
#define SEC_CMD_RESULT_STR_LEN_EXPAND	(SEC_CMD_RESULT_STR_LEN * 6)
#define SEC_CMD_PARAM_NUM		8

#define WAIT_RESULT true
#define EXIT_RESULT false

struct sec_cmd {
	struct list_head	list;
	const char		*cmd_name;
	void			(*cmd_func)(void *device_data);
	int			(*cmd_func_forced)(void *device_data);
/*
 * cmd_use_cases using as below
 * 1 : call function, 0 : no call
 * first bit(1<<0) CHECK_POWEROFF
 * second bit(1<<1) CHECK_LPMODE
 * third bit(1<<2) CHECK_POWERON
 */
	int			cmd_use_cases;
	bool			wait_read_result;
	int			cmd_log;
	bool			not_support_cmds;
};

enum sec_cmd_status_uevent_type {
	STATUS_TYPE_WET = 0,
	STATUS_TYPE_NOISE,
	STATUS_TYPE_FREQ,
};

enum SEC_CMD_STATUS {
	SEC_CMD_STATUS_WAITING = 0,
	SEC_CMD_STATUS_RUNNING,		// = 1
	SEC_CMD_STATUS_OK,		// = 2
	SEC_CMD_STATUS_FAIL,		// = 3
	SEC_CMD_STATUS_EXPAND,		// = 4
	SEC_CMD_STATUS_NOT_APPLICABLE,	// = 5
};

#define INPUT_CMD_RESULT_NOT_EXIT	0
#define INPUT_CMD_RESULT_NEED_EXIT	1

#define input_cmd_result(cmd_state_parm, need_exit)				\
({										\
	if (need_exit == INPUT_CMD_RESULT_NEED_EXIT) {				\
		if (cmd_state_parm == SEC_CMD_STATUS_OK)			\
			snprintf(buff, sizeof(buff), "OK");			\
		else if (cmd_state_parm == SEC_CMD_STATUS_FAIL)			\
			snprintf(buff, sizeof(buff), "NG");			\
		else if (cmd_state_parm == SEC_CMD_STATUS_NOT_APPLICABLE)	\
			snprintf(buff, sizeof(buff), "NA");			\
	}									\
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));		\
	sec->cmd_state = cmd_state_parm;					\
	if (need_exit == INPUT_CMD_RESULT_NEED_EXIT)				\
		sec_cmd_set_cmd_exit(sec);					\
	input_info(true, ptsp, "%s: %s\n", __func__, buff);			\
})

#ifdef USE_SEC_CMD_QUEUE
#define SEC_CMD_MAX_QUEUE	10

struct command {
	char	cmd[SEC_CMD_STR_LEN];
};
#endif

struct sec_cmd_data {
	struct device		*fac_dev;
	struct device		*dev;
	struct list_head	cmd_list_head;
	u8			cmd_state;
	char			cmd[SEC_CMD_STR_LEN];
	int			cmd_param[SEC_CMD_PARAM_NUM];
	char			*cmd_result;
	int			cmd_result_expand;
	int			cmd_result_expand_count;
	int			cmd_buffer_size;
	atomic_t			cmd_is_running;
	struct mutex		cmd_lock;
	struct mutex		fs_lock;
	struct mutex		io_lock;
#ifdef USE_SEC_CMD_QUEUE
	struct kfifo		cmd_queue;
	struct mutex		fifo_lock;
	struct delayed_work	cmd_work;
	struct mutex		wait_lock;
	struct completion	cmd_result_done;
#endif
	bool			wait_cmd_result_done;
	int item_count;
	char cmd_result_all[SEC_CMD_RESULT_STR_LEN];
	u8 cmd_all_factory_state;
	struct attribute_group *vendor_attr_group;
};

void sec_cmd_set_cmd_exit(struct sec_cmd_data *data);
void sec_cmd_set_default_result(struct sec_cmd_data *data);
void sec_cmd_set_cmd_result(struct sec_cmd_data *data, char *buff, int len);
void sec_cmd_set_cmd_result_all(struct sec_cmd_data *data, char *buff, int len, char *item);
int sec_cmd_init(struct sec_cmd_data *data, struct device *dev, struct sec_cmd *cmds,
			int len, int devt, struct attribute_group *vendor_attr_group);
int sec_cmd_init_without_platdata(struct sec_cmd_data *data, struct sec_cmd *cmds,
			int len, int devt, struct attribute_group *vendor_attr_group);
void sec_cmd_exit(struct sec_cmd_data *data, int devt);
void sec_cmd_send_event_to_user(struct sec_cmd_data *data, char *test, char *result);
void sec_cmd_send_status_uevent(struct sec_cmd_data *data, enum sec_cmd_status_uevent_type type, int value);
void sec_cmd_send_gesture_uevent(struct sec_cmd_data *data, int type, int x, int y);
void sec_cmd_io_clear_firmware_data(struct sec_cmd_data *data);
int sec_cmd_io_init(struct sec_cmd_data *data, int devt);

#endif /* _SEC_CMD_H_ */
