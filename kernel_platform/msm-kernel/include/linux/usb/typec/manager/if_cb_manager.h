/*
 * Copyright (C) 2018-2019 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *
 */

#ifndef __IF_CB_MANAGER_H__
#define __IF_CB_MANAGER_H__

enum sbu_switch_status {
	UNDEFINED = -1,
	OPEN_SBU = 0,
	CLOSE_SBU_CC1_ACTIVE = 1,
	CLOSE_SBU_CC2_ACTIVE = 2,
};

struct usb_ops {
	void (*usb_set_vbus_current)(void *data, int state);
	int (*usb_restart_host_mode)(void *data, int lanes);
};

struct muic_ops {
	int (*muic_check_usb_killer)(void *data);
	void (*muic_set_bypass)(void *data, int enable);
	void (*muic_set_bc12)(void *data, int enable);
};

struct usbpd_ops {
	int (*usbpd_sbu_test_read)(void *data);
	void (*usbpd_set_host_on)(void *data, int mode);
	void (*usbpd_cc_control_command)(void *data, int is_off);
	void (*usbpd_wait_entermode)(void *data, int on);
	void (*usbpd_water_pd_ta_notify)(void *data);
	void (*usbpd_sbu_switch_control)(void *data, int on);
};

struct lvs_ops {
	int (*lvs_cc_attach)(void *data, int on);
};

struct usb_dev {
	const struct usb_ops *ops;
	void *data;
};

struct muic_dev {
	const struct muic_ops *ops;
	void *data;
};

struct usbpd_dev {
	const struct usbpd_ops *ops;
	void *data;
};

struct lvs_dev {
	const struct lvs_ops *ops;
	void *data;
};

struct if_cb_manager {
	struct usb_dev *usb_d;
	struct muic_dev *muic_d;
	struct usbpd_dev *usbpd_d;
	struct lvs_dev *lvs_d;
};

extern struct if_cb_manager *register_usb(struct usb_dev *usb);
extern struct if_cb_manager *register_muic(struct muic_dev *muic);
extern struct if_cb_manager *register_usbpd(struct usbpd_dev *usbpd);
extern struct if_cb_manager *register_lvs(struct lvs_dev *lvs);
extern void usb_set_vbus_current(struct if_cb_manager *man_core, int state);
extern int usb_restart_host_mode(struct if_cb_manager *man_core, int lanes);
extern int muic_check_usb_killer(struct if_cb_manager *man_core);
extern void muic_set_bypass(struct if_cb_manager *man_core, int enable);
extern void muic_set_bc12(struct if_cb_manager *man_core, int enable);
extern int usbpd_sbu_test_read(struct if_cb_manager *man_core);
extern void usbpd_set_host_on(struct if_cb_manager *man_core, int mode);
extern void usbpd_cc_control_command(struct if_cb_manager *man_core, int is_off);
extern void usbpd_wait_entermode(struct if_cb_manager *man_core, int on);
extern void usbpd_water_pd_ta_notify(struct if_cb_manager *man_core);
extern void usbpd_sbu_switch_control(int on);

#endif /* __IF_CB_MANAGER_H__ */
