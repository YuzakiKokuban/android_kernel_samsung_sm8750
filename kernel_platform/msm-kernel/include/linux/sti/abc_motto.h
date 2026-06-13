/* abc_motto.h
 *
 * Abnormal Behavior Catcher MOTTO Support
 *
 * Copyright (C) 2020 Samsung Electronics
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

enum motto_event_module {
	MOTTO_MODULE_NONE = -1,
	MOTTO_MODULE_GPU = 0,
	MOTTO_MODULE_DECON,
	MOTTO_MODULE_CAMERA,
	MOTTO_MODULE_NPU,
	MOTTO_MODULE_MAX,
};

#define MOTTO_MODULE_NAME_LEN	32
#define MOTTO_BUFFER_MAX		256
struct motto_event_type {
	enum motto_event_module module;
	char motto_event_module_name[MOTTO_MODULE_NAME_LEN];
	char motto_event_type_str[MOTTO_BUFFER_MAX];
};

struct abc_motto_data {
	u8 boot_time;
	u8 dev_err_count[MOTTO_MODULE_MAX];
};

extern void motto_send_bootcheck_info(int boot_time);

void motto_send_device_info(char *module_str, char *event_type);
void motto_init(struct platform_device *);
