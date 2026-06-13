/*
 *
 * $Id: stk6d2x_sec.h
 *
 * Copyright (C) 2012~2018 Bk, sensortek Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 */

#ifndef __STK6D2X_SEC_H__
#define __STK6D2X_SEC_H__

#include <stk6d2x.h>
#include <common_define.h>
#include <linux/version.h>

// #define SUPPORT_SENSOR_CLASS

typedef struct stk6d2x_wrapper
{
	struct i2c_manager      i2c_mgr;
	stk6d2x_data            alps_data;
	struct device           *dev;
	struct device           *sensor_dev;
#ifdef SUPPORT_SENSOR_CLASS
	struct sensors_classdev als_cdev;
#endif
	struct input_dev        *als_input_dev;
	atomic_t                recv_reg;
} stk6d2x_wrapper;

#endif // __STK6D2X_SEC_H__
