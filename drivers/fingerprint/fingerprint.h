/*
 *  Copyright (C) 2017, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#ifndef FINGERPRINT_H_
#define FINGERPRINT_H_

#undef DEBUG /* If you need pr_debug logs, changes this difinition */
#undef pr_fmt
#define pr_fmt(fmt) "fps_%s: " fmt, __func__

#include "fingerprint_sysfs.h"

#if defined(CONFIG_FINGERPRINT_SECURE) && !defined(CONFIG_SEC_FACTORY)
#define ENABLE_SENSORS_FPRINT_SECURE
#endif

/* For Sensor Type Check */
enum {
	SENSOR_OOO = -2,
	SENSOR_UNKNOWN,
	SENSOR_FAILED,
	SENSOR_OK,
};

#define SENSOR_STATUS_SIZE 4
static char sensor_status[SENSOR_STATUS_SIZE][10] = {"ooo", "unknown", "failed", "okay"};

/* For Finger Detect Mode */
enum {
	DETECT_NORMAL = 0,
	DETECT_ADM,			/* Always on Detect Mode */
};

#endif
