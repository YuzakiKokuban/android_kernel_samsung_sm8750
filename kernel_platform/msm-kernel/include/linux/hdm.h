/*
 * @file hdm.h
 * @brief Header file for HDM driver
 * Copyright (c) 2019, Samsung Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __HDM_H__
#define __HDM_H__
#include <linux/types.h>


#ifndef __ASSEMBLY__

#define HDM_CMD_LEN ((size_t)8)

#define HDM_P_BITMASK       0x3FF
#define HDM_C_BITMASK       0xF0000
#define HDM_FLAG_UPDATE     0x10000

#define HDM_HYP_CALL        0x40000
#define HDM_HYP_INIT        0x50000
#define HDM_HYP_CLEAR       0x60000
#define HDM_HYP_CALLP       0x80000
#define HDM_CMD_MAX         0xFFFFF

#define HDM_GET_SUPPORTED_SUBSYSTEM 6

#define HDM_CAM     0x1
#define HDM_MMC     0x2
#define HDM_USB     0x4
#define HDM_WIFI    0x8
#define HDM_BT      0x10
#define HDM_GPS     0x20
#define HDM_NFC     0x40
#define HDM_AUD     0x80
#define HDM_CP      0x100
#define HDM_SPK     0x200
#define MAX_DEVICE_NUM  10

extern bool hdm_is_applied(uint32_t);

#endif //__ASSEMBLY__
#endif //__HDM_H__
