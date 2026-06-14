/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */

#ifndef LINUX_RANGE_SENSOR_H
#define LINUX_RANGE_SENSOR_H

#if   defined(__linux__)
#include <linux/types.h>
#else
#include <stdint.h>
#include <sys/types.h>
#endif

#define NUMBER_OF_ZONE   64
#define NUMBER_OF_TARGET  1

struct range_sensor_data_t {
   __u16 depth16[NUMBER_OF_ZONE];
   __u16 dmax[NUMBER_OF_ZONE];
   __u32 peak_signal[NUMBER_OF_ZONE];
   __u8 glass_detection_flag;
};

#endif
