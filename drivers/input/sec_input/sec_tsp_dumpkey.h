/* SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2022 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef	__SEC_TSP_DUMPKEY_H__
#define __SEC_TSP_DUMPKEY_H__

#define NUM_DEVICES 3

struct sec_tsp_dumpkey_param {
	unsigned int keycode;
	int down;
};

struct tsp_dump_callbacks {
	void (*inform_dump)(struct device *dev);
	struct device *ptsp;
};

void sec_input_dumpkey_register(int dev_id, void (*callback)(struct device *dev), struct device *dev);
void sec_input_dumpkey_unregister(int dev_id);

int sec_tsp_dumpkey_init(void);
void sec_tsp_dumpkey_exit(void);
#endif /* __SEC_TSP_DUMPKEY_H__ */
