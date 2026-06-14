/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2014 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __LINUX_USB_NOTIFIER_H__
#define __LINUX_USB_NOTIFIER_H__

extern int dwc_msm_vbus_event(bool enable);
extern void dwc3_max_speed_setting(int speed);
extern int dwc_msm_id_event(bool enable);
extern int gadget_speed(void);
extern int is_dwc3_msm_probe_done(void);

#endif
