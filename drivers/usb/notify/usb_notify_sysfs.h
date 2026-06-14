/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2015-2023 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

  /* usb notify layer v4.0 */

#ifndef __LINUX_USB_NOTIFY_SYSFS_H__
#define __LINUX_USB_NOTIFY_SYSFS_H__

#define MAX_DISABLE_STR_LEN 32
#define MAX_WHITELIST_STR_LEN 256
#define MAX_USB_AUDIO_CARDS 15
/* one card needs 9 byte ex) <card11> */
#define MAX_CARD_STR_LEN (MAX_USB_AUDIO_CARDS * 9)
#define MAX_CLASS_TYPE_NUM	USB_CLASS_VENDOR_SPEC
#define MAX_USB_SPEED_STR_LEN 15

#define ALLOWLIST_PREFIX_SIZE 5
#define MAX_VID_PID_STRING 10
#define MAX_ALLOWLIST_DEVICE_COUNT 100
#define MAX_ALLOWLIST_DEVICE_BUFFER_INDEX (MAX_ALLOWLIST_DEVICE_COUNT*2)
#define MAX_ALLOWLIST_BUFFER (MAX_VID_PID_STRING * MAX_ALLOWLIST_DEVICE_COUNT + ALLOWLIST_PREFIX_SIZE)

enum u_interface_class_type {
	U_CLASS_PER_INTERFACE = 1,
	U_CLASS_AUDIO,
	U_CLASS_COMM,
	U_CLASS_HID,
	U_CLASS_PHYSICAL,
	U_CLASS_STILL_IMAGE,
	U_CLASS_PRINTER,
	U_CLASS_MASS_STORAGE,
	U_CLASS_HUB,
	U_CLASS_CDC_DATA,
	U_CLASS_CSCID,
	U_CLASS_CONTENT_SEC,
	U_CLASS_VIDEO,
	U_CLASS_WIRELESS_CONTROLLER,
	U_CLASS_MISC,
	U_CLASS_APP_SPEC,
	U_CLASS_VENDOR_SPEC,
};

struct usb_audio_info {
	int cards;
	int bundle;
};

struct usb_notify_dev {
	const char *name;
	struct device *dev;
	struct otg_notify *o_notify;
	int index;
	unsigned int request_action;
	unsigned int lpm_charging_type_done;
	unsigned long usb_data_enabled;
	unsigned long disable_state;
	unsigned long secure_lock;
	bool first_restrict;
	int (*set_disable)(struct usb_notify_dev *udev, int param);
	void (*set_mdm)(struct usb_notify_dev *udev, int mdm_disable);
	void (*set_mdm_for_id)(struct usb_notify_dev *udev, int mdm_disable);
	void (*set_mdm_for_serial)(struct usb_notify_dev *udev, int mdm_disable);
	int (*control_usb_max_speed)(struct usb_notify_dev *udev, int speed);
	unsigned long (*fp_hw_param_manager)(int param);
	int (*set_lock_state)(struct usb_notify_dev *udev);
	char disable_state_cmd[MAX_DISABLE_STR_LEN];
	char whitelist_str[MAX_WHITELIST_STR_LEN];
	int whitelist_array_for_mdm[MAX_CLASS_TYPE_NUM+1];
	int whitelist_array_for_mdm_for_id[MAX_WHITELIST_STR_LEN];
	char whitelist_str_for_id[MAX_WHITELIST_STR_LEN];
	char whitelist_array_for_mdm_for_serial[MAX_WHITELIST_STR_LEN];
	struct usb_audio_info usb_audio_cards[MAX_USB_AUDIO_CARDS];
	int allowlist_array_lockscreen_enabled_id[MAX_ALLOWLIST_DEVICE_BUFFER_INDEX];
	char allowlist_str_lockscreen_enabled_id[MAX_ALLOWLIST_BUFFER];
	struct mutex lockscreen_enabled_lock;
};

extern int usb_notify_dev_uevent(struct usb_notify_dev *udev,
							char *envp_ext[]);
extern int usb_notify_dev_register(struct usb_notify_dev *ndev);
extern void usb_notify_dev_unregister(struct usb_notify_dev *ndev);
extern int usb_notify_class_init(void);
extern void usb_notify_class_exit(void);
#endif

