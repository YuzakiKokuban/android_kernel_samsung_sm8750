/* SPDX-License-Identifier: GPL-2.0-only */
/* drivers/input/sec_input/sec_input_multi_dev.h
 *
 * Core file for Samsung input device driver for multi device
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SEC_INPUT_MULTI_DEV_H_
#define _SEC_INPUT_MULTI_DEV_H_

#include "sec_input.h"

#define MULTI_DEV_DEBUG_INFO_SIZE	256

#define MULTI_DEV_NONE			0
#define MULTI_DEV_MAIN			1
#define MULTI_DEV_SUB			2
#define FOLD_STATUS_FOLDING		1
#define FOLD_STATUS_UNFOLDING		0
#define GET_DEV_COUNT(mdev)		((mdev == NULL) ? MULTI_DEV_NONE : mdev->device_count)
#define IS_FOLD(x)			(x == MULTI_DEV_MAIN || x == MULTI_DEV_SUB)
#define IS_FOLD_DEV(mdev)		IS_FOLD(GET_DEV_COUNT(mdev))
#define IS_NOT_FOLD(x)			!IS_FOLD(x)
#define IS_NOT_FOLD_DEV(mdev)		IS_NOT_FOLD(GET_DEV_COUNT(mdev))
#define GET_FOLD_STR(x)			(x == MULTI_DEV_MAIN ? "MAIN" : x == MULTI_DEV_SUB ? "SUB" : "NONE")
#define GET_SEC_CLASS_DEVT_TSP(mdev)	(GET_DEV_COUNT(mdev) == MULTI_DEV_MAIN ? SEC_CLASS_DEVT_TSP1 \
						: GET_DEV_COUNT(mdev) == MULTI_DEV_SUB ? SEC_CLASS_DEVT_TSP2 \
						: SEC_CLASS_DEVT_TSP)
#if IS_ENABLED(CONFIG_SEC_ABC)
#define GET_INT_ABC_TYPE(mdev)		(GET_DEV_COUNT(mdev) == MULTI_DEV_SUB ? SEC_ABC_SEND_EVENT_TYPE_SUB \
						: SEC_ABC_SEND_EVENT_TYPE)
#endif

struct sec_input_multi_device {
	struct device *dev;

	int device_count;
	const char *name;

	bool device_ready;

	int flip_status;
	int flip_status_current;
	int change_flip_status;

	unsigned int flip_mismatch_count;

	struct delayed_work switching_work;
};

#if IS_ENABLED(CONFIG_SEC_INPUT_MULTI_DEVICE)
bool sec_input_need_fold_off(struct sec_input_multi_device *mdev);
void sec_input_set_fold_state(struct sec_input_multi_device *mdev, int state);
void sec_input_multi_device_ready(struct sec_input_multi_device *mdev);

void sec_input_multi_device_create(struct device *dev);
void sec_input_multi_device_remove(struct sec_input_multi_device *mdev);
void sec_input_get_multi_device_debug_info(struct sec_input_multi_device *mdev,
		char *buf, ssize_t size);
#else
static inline bool sec_input_need_fold_off(struct sec_input_multi_device *mdev) { return false; }
static inline void sec_input_set_fold_state(struct sec_input_multi_device *mdev, int state) {}
static inline void sec_input_multi_device_ready(struct sec_input_multi_device *mdev) {}

static inline void sec_input_multi_device_create(struct device *dev) {}
static inline void sec_input_multi_device_remove(struct sec_input_multi_device *mdev) {}
static inline void sec_input_get_multi_device_debug_info(struct sec_input_multi_device *mdev,
		char *buf, ssize_t size) { return; }
#endif
#endif
