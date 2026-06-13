/* abc_common.h
 *
 * Abnormal Behavior Catcher Common Driver
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * Hyeokseon Yu <hyeokseon.yu@samsung.com>
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

#ifndef SEC_ABC_H
#define SEC_ABC_H
#include <linux/kconfig.h>
#include <linux/kernel.h>
#include <linux/module.h>
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
#include <linux/sec_class.h>
#else
extern struct class *sec_class;
#endif
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kfifo.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/workqueue.h>
#include <linux/rtc.h>
#include <linux/version.h>
#if IS_ENABLED(CONFIG_SEC_KUNIT)
#include <kunit/test.h>
#include <kunit/mock.h>
#else
#define __visible_for_testing static
#endif
#if (KERNEL_VERSION(4, 11, 0) <= LINUX_VERSION_CODE)
#include <linux/sched/clock.h>
#else
#include <linux/sched.h>
#endif
#if IS_ENABLED(CONFIG_SEC_ABC_MOTTO)
#include <linux/sti/abc_motto.h>
#endif
#ifndef EXPORT_SYMBOL_KUNIT
#define EXPORT_SYMBOL_KUNIT(sym)	/* nothing */
#endif
#define ABC_CMD_MAX					8
#define ABC_UEVENT_MAX				10
#define ABC_BUFFER_MAX				128
#define ABC_CMD_STR_MAX				16
#define ABC_TYPE_STR_MAX			8
#define ABC_EVENT_STR_MAX			60
#define ABC_SPEC_CMD_STR_MAX		3
#define ABC_WORK_MAX				5
#define ABC_WAIT_ENABLE_TIMEOUT		10000
#define ERROR_REPORT_MODE_BIT		(1<<0)
#define ALL_REPORT_MODE_BIT			(1<<1)
#define PRE_EVENT_ENABLE_BIT		(1<<7)
#define ABC_DISABLED				0
#define ABC_PREOCCURRED_EVENT_MAX	30
#define TIME_STAMP_STR_MAX			25
#define ABC_CLEAR_EVENT_TIMEOUT		300000
#define ABC_EVENT_BUFFER_MAX		30
#define ABC_DEFAULT_COUNT			0
#define ABC_TEST_STR_MAX		128
#define ABC_SKIP_EVENT_COUNT_THRESHOLD		100

enum abc_enable_cmd {
	ERROR_REPORT_MODE_ENABLE = 0,
	ERROR_REPORT_MODE_DISABLE,
	ALL_REPORT_MODE_ENABLE,
	ALL_REPORT_MODE_DISABLE,
	PRE_EVENT_ENABLE,
	PRE_EVENT_DISABLE,
	ABC_ENABLE_CMD_MAX,
};

enum abc_event_group {
	ABC_GROUP_NONE = -1,
	ABC_GROUP_CAMERA_MIPI_ERROR_ALL = 0,
	ABC_GROUP_MAX,
};

struct registered_abc_event_struct {
	char module_name[ABC_EVENT_STR_MAX];
	char error_name[ABC_EVENT_STR_MAX];
	char host[ABC_EVENT_STR_MAX];
	bool enabled;
	bool singular_spec;
	int error_count;
	enum abc_event_group group;
};

struct abc_event_group_struct {
	enum abc_event_group group;
	char module[ABC_EVENT_STR_MAX];
	char name[ABC_EVENT_STR_MAX];
};

struct abc_spec_cmd {
	char module[ABC_EVENT_STR_MAX];
	char name[ABC_EVENT_STR_MAX];
	char spec[ABC_EVENT_STR_MAX];
};

struct abc_enable_cmd_struct {
	enum abc_enable_cmd cmd;
	int enable_value;
	char abc_cmd_str[ABC_CMD_STR_MAX];
};

enum DATA_TYPE {
	TYPE_STRING,
	TYPE_INT,
};

struct abc_key_data {
	char event_type[ABC_TYPE_STR_MAX];
	char event_module[ABC_EVENT_STR_MAX];
	char event_name[ABC_EVENT_STR_MAX];
	char ext_log[ABC_EVENT_STR_MAX];
	unsigned int cur_time;
	int idx;
};

struct abc_pre_event {
	struct list_head node;
	int error_cnt;
	int all_cnt;
	int idx;
	struct abc_key_data key_data;
};

struct abc_fault_info {
	unsigned int cur_time;
	int cur_cnt;
};

struct abc_event_buffer {
	int size;
	int rear;
	int front;
	int warn_cnt;
	int buffer_max;
	struct abc_fault_info *abc_element;
};

struct abc_common_spec_data {
	char *module_name;
	char *error_name;
	int idx;
	//int spec_type;		In case a new spec type is added
};

struct spec_data_type1 {
	int threshold_cnt;
	int threshold_time;
	int default_count;
	bool default_enabled;
	struct list_head node;
	struct abc_common_spec_data common_spec;
	struct abc_event_buffer buffer;
};

struct abc_platform_data {
	unsigned int nItem;
#if IS_ENABLED(CONFIG_SEC_ABC_MOTTO)
	struct abc_motto_data *motto_data;
#endif
	struct list_head abc_spec_list;
};

struct abc_event_work {
	char abc_str[ABC_BUFFER_MAX];
	struct work_struct work;
};

struct abc_info {
	struct device *dev;
	struct workqueue_struct *workqueue;
	struct abc_event_work event_work_data[ABC_WORK_MAX];
	struct completion enable_done;
	struct delayed_work clear_pre_events;
#if IS_ENABLED(CONFIG_SEC_KUNIT)
	struct completion test_work_done;
	struct completion test_uevent_done;
#endif
	struct abc_platform_data *pdata;
	struct mutex work_mutex;
	struct mutex spec_mutex;
	struct mutex pre_event_mutex;
	struct mutex enable_mutex;
};
void abc_common_test_get_log_str(char *log_str);
int sec_abc_get_idx_of_registered_event(char *module_name, char *error_name);
extern void sec_abc_change_spec(const char *str);
extern int sec_abc_read_spec(char *str);
extern void sec_abc_send_event(char *str);
extern int sec_abc_get_enabled(void);
extern int sec_abc_wait_enabled(void);
int sec_abc_save_pre_events(struct abc_key_data *key_data, char *uevent_type);
extern struct registered_abc_event_struct abc_event_list[];
extern int REGISTERED_ABC_EVENT_TOTAL;

#define ABC_PRINT(format, ...) pr_info("[sec_abc] %s : " format, __func__, ##__VA_ARGS__)
#define ABC_DEBUG(format, ...) pr_debug("[sec_abc] %s : " format, __func__, ##__VA_ARGS__)

#ifdef CONFIG_SEC_KUNIT
#define ABC_PRINT_KUNIT(format, ...) do { \
	char temp[ABC_TEST_STR_MAX]; \
	ABC_PRINT(format, ##__VA_ARGS__); \
	snprintf(temp, ABC_TEST_STR_MAX, format, ##__VA_ARGS__); \
	abc_common_test_get_log_str(temp); \
} while (0)
#define ABC_DEBUG_KUNIT(format, ...) do { \
	char temp[ABC_TEST_STR_MAX]; \
	ABC_PRINT(format, ##__VA_ARGS__); \
	snprintf(temp, ABC_TEST_STR_MAX, format, ##__VA_ARGS__); \
	abc_common_test_get_log_str(temp); \
} while (0)
#else
#define ABC_PRINT_KUNIT(format, ...) ABC_PRINT(format, ##__VA_ARGS__)
#define ABC_DEBUG_KUNIT(format, ...) ABC_DEBUG(format, ##__VA_ARGS__)
#endif

#endif
