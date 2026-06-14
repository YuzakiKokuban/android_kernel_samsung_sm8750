/*
 *
 * $Id: common_define.h
 *
 * Copyright (C) 2019 Bk, sensortek Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 */

#ifndef __DEFINE_COMMON_H__
#define __DEFINE_COMMON_H__

#include "platform_config.h"

typedef struct stk_timer_info stk_timer_info;
typedef struct stk_gpio_info stk_gpio_info;

struct stk_bus_ops
{
	int bustype;
	int (*init)(void *);
	int (*read)(int, unsigned int, unsigned char *);
	int (*read_block)(int, unsigned int, int, void *);
	int (*write)(int, unsigned int, unsigned char);
	int (*write_block)(int, unsigned int, void *, int);
	int (*read_modify_write)(int, unsigned int, unsigned char, unsigned char);
	int (*remove)(void *);
};

typedef enum
{
	SECOND,
	M_SECOND,
	U_SECOND,
	N_SECOND,
} TIMER_UNIT;

typedef enum
{
	US_RANGE_DELAY,
	MS_DELAY,
} BUSY_WAIT_TYPE;

struct stk_timer_info
{
	char            wq_name[4096];
	uint32_t        interval_time;
	TIMER_UNIT      timer_unit;
	void            (*timer_cb)(stk_timer_info *t_info);
	bool            is_active;
	bool            is_exist;
	bool            is_periodic;
	bool            change_interval_time;
	void            *any;
} ;

struct stk_timer_ops
{
	int (*register_timer)(stk_timer_info *);
	int (*start_timer)(stk_timer_info *);
	int (*stop_timer)(stk_timer_info *);
	int (*remove)(stk_timer_info *);
	void (*busy_wait)(unsigned long, unsigned long, BUSY_WAIT_TYPE);
};

typedef enum
{
	TRIGGER_RISING,
	TRIGGER_FALLING,
	TRIGGER_HIGH,
	TRIGGER_LOW,
} GPIO_TRIGGER_TYPE;

struct stk_gpio_info
{
	char                wq_name[4096];
	char                device_name[4096];
	void                (*gpio_cb)(stk_gpio_info *gpio_info);
	GPIO_TRIGGER_TYPE   trig_type;
	int                 int_pin;
	int32_t             irq;
	bool                is_active;
	bool                is_exist;
	void                *any;
} ;

struct stk_gpio_ops
{
	int (*register_gpio_irq)(stk_gpio_info *);
	int (*start_gpio_irq)(stk_gpio_info *);
	int (*stop_gpio_irq)(stk_gpio_info *);
	int (*remove)(stk_gpio_info *);
};

struct stk_storage_ops
{
	int (*init_storage)(void);
	int (*write_to_storage)(char *, uint8_t *, int);
	int (*read_from_storage)(char *, uint8_t *, int);
	int (*remove)(void);
};

struct common_function
{
	const struct stk_bus_ops *bops;
	const struct stk_timer_ops *tops;
	const struct stk_gpio_ops *gops;
};

typedef struct stk_register_table
{
	uint8_t address;
	uint8_t value;
	uint8_t mask_bit;
} stk_register_table;

#endif // __DEFINE_COMMON_H__