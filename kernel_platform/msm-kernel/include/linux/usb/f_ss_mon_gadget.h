/*
 * Gadget Function Driver for Android USB accessories
 *
 * Copyright (C) 2021 samsung.com.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _F_SS_MON_GADGHET_H_
#define _F_SS_MON_GADGHET_H_

#include <linux/usb/gadget.h>

extern void usb_reset_notify(struct usb_gadget *gadget);
extern void vbus_session_notify(struct usb_gadget *gadget, int is_active, int ret);
extern void make_suspend_current_event(void);
#endif
