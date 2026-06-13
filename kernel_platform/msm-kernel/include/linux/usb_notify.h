/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  usb notify header
 *
 * Copyright (C) 2011-2023 Samsung, Inc.
 * Author: Dongrak Shin <dongrak.shin@samsung.com>
 *
 */

 /* usb notify layer v4.0 */

#ifndef __LINUX_USB_NOTIFY_H__
#define __LINUX_USB_NOTIFY_H__

#include <linux/notifier.h>
#include <linux/host_notify.h>
#include <linux/external_notify.h>
#include <linux/usblog_proc_notify.h>
#if defined(CONFIG_USB_HW_PARAM)
#include <linux/usb_hw_param.h>
#endif
#include <linux/usb.h>

enum otg_notify_events {
	NOTIFY_EVENT_NONE,
	NOTIFY_EVENT_VBUS,
	NOTIFY_EVENT_HOST,
	NOTIFY_EVENT_CHARGER,
	NOTIFY_EVENT_SMARTDOCK_TA,
	NOTIFY_EVENT_SMARTDOCK_USB,
	NOTIFY_EVENT_AUDIODOCK,
	NOTIFY_EVENT_LANHUB,
	NOTIFY_EVENT_LANHUB_TA,
	NOTIFY_EVENT_MMDOCK,
	NOTIFY_EVENT_HMT,
	NOTIFY_EVENT_GAMEPAD,
	NOTIFY_EVENT_POGO,
	NOTIFY_EVENT_HOST_RELOAD,
	NOTIFY_EVENT_DRIVE_VBUS,
	NOTIFY_EVENT_ALL_DISABLE,
	NOTIFY_EVENT_HOST_DISABLE,
	NOTIFY_EVENT_CLIENT_DISABLE,
	NOTIFY_EVENT_MDM_ON_OFF,
	NOTIFY_EVENT_MDM_ON_OFF_FOR_ID,
	NOTIFY_EVENT_MDM_ON_OFF_FOR_SERIAL,
	NOTIFY_EVENT_OVERCURRENT,
	NOTIFY_EVENT_SMSC_OVC,
	NOTIFY_EVENT_SMTD_EXT_CURRENT,
	NOTIFY_EVENT_MMD_EXT_CURRENT,
	NOTIFY_EVENT_HMD_EXT_CURRENT,
	NOTIFY_EVENT_DEVICE_CONNECT,
	NOTIFY_EVENT_GAMEPAD_CONNECT,
	NOTIFY_EVENT_LANHUB_CONNECT,
	NOTIFY_EVENT_POWER_SOURCE,
	NOTIFY_EVENT_PD_CONTRACT,
	NOTIFY_EVENT_PD_USB_COMM_CAPABLE,
	NOTIFY_EVENT_VBUS_RESET,
	NOTIFY_EVENT_RESERVE_BOOSTER,
	NOTIFY_EVENT_USB_CABLE,
	NOTIFY_EVENT_USBD_SUSPENDED,
	NOTIFY_EVENT_USBD_UNCONFIGURED,
	NOTIFY_EVENT_USBD_CONFIGURED,
	NOTIFY_EVENT_VBUSPOWER,
	NOTIFY_EVENT_DR_SWAP,
	NOTIFY_EVENT_REVERSE_BYPASS_DEVICE_CONNECT,
	NOTIFY_EVENT_REVERSE_BYPASS_DEVICE_ATTACH,
	NOTIFY_EVENT_VIRTUAL,
};

#define VIRT_EVENT(a) (a+NOTIFY_EVENT_VIRTUAL)
#define PHY_EVENT(a) (a%NOTIFY_EVENT_VIRTUAL)
#define IS_VIRTUAL(a) (a >= NOTIFY_EVENT_VIRTUAL ? 1 : 0)

enum otg_notify_event_status {
	NOTIFY_EVENT_DISABLED,
	NOTIFY_EVENT_DISABLING,
	NOTIFY_EVENT_ENABLED,
	NOTIFY_EVENT_ENABLING,
	NOTIFY_EVENT_BLOCKED,
	NOTIFY_EVENT_BLOCKING,
};

enum otg_notify_evt_type {
	NOTIFY_EVENT_EXTRA = (1 << 0),
	NOTIFY_EVENT_STATE = (1 << 1),
	NOTIFY_EVENT_DELAY = (1 << 2),
	NOTIFY_EVENT_NEED_VBUSDRIVE = (1 << 3),
	NOTIFY_EVENT_NOBLOCKING = (1 << 4),
	NOTIFY_EVENT_NOSAVE = (1 << 5),
	NOTIFY_EVENT_NEED_HOST = (1 << 6),
	NOTIFY_EVENT_NEED_CLIENT = (1 << 7),
};

enum otg_notify_block_type {
	NOTIFY_BLOCK_TYPE_NONE = 0,
	NOTIFY_BLOCK_TYPE_HOST = (1 << 0),
	NOTIFY_BLOCK_TYPE_CLIENT = (1 << 1),
	NOTIFY_BLOCK_TYPE_ALL = (1 << 0 | 1 << 1),
};

enum otg_notify_mdm_type {
	NOTIFY_MDM_TYPE_OFF,
	NOTIFY_MDM_TYPE_ON,
};

enum otg_notify_gpio {
	NOTIFY_VBUS,
	NOTIFY_REDRIVER,
};

enum otg_op_pos {
	NOTIFY_OP_OFF,
	NOTIFY_OP_POST,
	NOTIFY_OP_PRE,
};

enum ovc_check_value {
	HNOTIFY_LOW,
	HNOTIFY_HIGH,
	HNOTIFY_INITIAL,
};

enum otg_notify_power_role {
	HNOTIFY_SINK,
	HNOTIFY_SOURCE,
};

enum otg_notify_data_role {
	HNOTIFY_UFP,
	HNOTIFY_DFP,
};

enum usb_restrict_type {
	USB_SECURE_RESTRICTED,
	USB_TIME_SECURE_RESTRICTED,
	USB_SECURE_RELEASE,
};

enum usb_restrict_group {
	USB_GROUP_AUDIO,
	USB_GROUP_OTEHR,
	USB_GROUP_MAX,
};

enum usb_certi_type {
	USB_CERTI_UNSUPPORT_ACCESSORY,
	USB_CERTI_NO_RESPONSE,
	USB_CERTI_HUB_DEPTH_EXCEED,
	USB_CERTI_HUB_POWER_EXCEED,
	USB_CERTI_HOST_RESOURCE_EXCEED,
	USB_CERTI_WARM_RESET,
};

enum usb_err_type {
	USB_ERR_ABNORMAL_RESET,
};

enum usb_itracker_type {
	NOTIFY_USB_CC_REPEAT,
};

enum usb_current_state {
	NOTIFY_USB_UNCONFIGURED,
	NOTIFY_USB_SUSPENDED,
	NOTIFY_USB_CONFIGURED,
};

enum usb_allowlist_state {
	NOTIFY_MDM_NONE = 0,
	NOTIFY_MDM_SERIAL,
	NOTIFY_MDM_ID,
	NOTIFY_MDM_ID_AND_SERIAL,
};

enum usb_request_action_type {
	USB_REQUEST_NOTHING,
	USB_REQUEST_DUMPSTATE,
};

enum otg_notify_reverse_bypass_status {
	NOTIFY_EVENT_REVERSE_BYPASS_OFF,
	NOTIFY_EVENT_REVERSE_BYPASS_PREPARE,
	NOTIFY_EVENT_REVERSE_BYPASS_ON,
};

enum otg_notify_illegal_type {
	NOTIFY_EVENT_AUDIO_DESCRIPTOR,
	NOTIFY_EVENT_SECURE_DISCONNECTION,
};

enum usb_lock_state {
	USB_NOTIFY_UNLOCK = 0,
	USB_NOTIFY_LOCK_USB_WORK,
	USB_NOTIFY_LOCK_USB_RESTRICT,
	USB_NOTIFY_INIT_STATE = 3,
};

enum usb_check_allowlist_result {
	USB_NOTIFY_NOLIST = 0,
	USB_NOTIFY_ALLOWLOST,
	USB_NOTIFY_NORESTRICT,
};

enum usb_comm_capable {
	USB_NOTIFY_NO_COMM_CAPABLE = 0,
	USB_NOTIFY_COMM_CAPABLE = 1,
	USB_NOTIFY_INIT_COM_CAPABLE = 2,
};

struct otg_notify {
	int vbus_detect_gpio;
	int redriver_en_gpio;
	int is_wakelock;
	int is_host_wakelock; /*unused field*/
	int unsupport_host;
	int smsc_ovc_poll_sec;
	int auto_drive_vbus;
	int booting_delay_sec;
	int disable_control;
	int device_check_sec;
	int pre_peri_delay_us;
	int booting_delay_sync_usb;
	int (*pre_gpio)(int gpio, int use);
	int (*post_gpio)(int gpio, int use);
	int (*vbus_drive)(bool enable);
	int (*reverse_bypass_drive)(int mode);
	int (*get_support_reverse_bypass_en)(void *data);
	int (*set_host)(bool enable);
	int (*set_peripheral)(bool enable);
	int (*set_charger)(bool enable);
	int (*post_vbus_detect)(bool on);
	int (*set_lanhubta)(int enable);
	int (*set_battcall)(int event, int enable);
	int (*set_chg_current)(int state);
	void (*set_ldo_onoff)(void *data, unsigned int onoff);
	int (*get_gadget_speed)(void);
	int (*is_skip_list)(int index);
	int (*usb_maximum_speed)(int speed);
	void *o_data;
	void *u_notify;
};

struct otg_booster {
	char *name;
	int (*booster)(bool enable);
};

#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
extern const char *event_string(enum otg_notify_events event);
extern const char *status_string(enum otg_notify_event_status status);
extern void send_usb_mdm_uevent(void);
extern void send_usb_certi_uevent(int usb_certi);
extern void send_usb_err_uevent(int usb_certi, int mode);
extern void send_usb_itracker_uevent(int err_type);
extern void send_usb_restrict_uevent(int usb_restrict);
extern int usb_check_whitelist_for_id(struct usb_device *dev);
extern int usb_check_whitelist_for_serial(struct usb_device *dev);
extern int usb_check_whitelist_for_mdm(struct usb_device *dev);
extern int usb_check_whitelist_enable_state(void);
#ifndef CONFIG_DISABLE_LOCKSCREEN_USB_RESTRICTION
extern int usb_check_allowlist_for_lockscreen_enabled_id(struct usb_device *dev);
extern bool check_usb_restrict_lock_state(struct otg_notify *n);
#endif
extern int usb_otg_restart_accessory(struct usb_device *dev);
extern void send_otg_notify(struct otg_notify *n,
					unsigned long event, int enable);
extern int get_typec_status(struct otg_notify *n, int event);
extern struct otg_booster *find_get_booster(struct otg_notify *n);
extern int register_booster(struct otg_notify *n, struct otg_booster *b);
extern int register_ovc_func(struct otg_notify *n,
				int (*check_state)(void *), void *data);
extern int get_booster(struct otg_notify *n);
extern int get_usb_mode(struct otg_notify *n);
extern unsigned long get_cable_type(struct otg_notify *n);
extern int is_usb_host(struct otg_notify *n);
extern bool is_blocked(struct otg_notify *n, int type);
extern bool is_snkdfp_usb_device_connected(struct otg_notify *n);
extern int get_con_dev_max_speed(struct otg_notify *n);
extern void set_con_dev_max_speed
		(struct otg_notify *n, int speed);
extern void set_con_dev_hub(struct otg_notify *n, int speed, int conn);
extern void set_request_action(struct otg_notify *n, unsigned int request_action);
extern int is_known_usbaudio(struct usb_device *dev);
extern int check_usbaudio(struct usb_device *dev);
extern int check_usbgroup(struct usb_device *dev);
extern int is_usbhub(struct usb_device *dev);
extern void set_usb_audio_cardnum(int card_num, int bundle, int attach);
extern void send_usb_audio_uevent(struct usb_device *dev,
		int cardnum, int attach);
extern int send_usb_notify_uevent
		(struct otg_notify *n, char *envp_ext[]);
extern int check_new_device_added(struct usb_device *udev);
extern int set_lpm_charging_type_done(struct otg_notify *n,
		unsigned int state);
extern int detect_illegal_condition(int type);
extern int disconnect_unauthorized_device(struct usb_device *dev);
#if defined(CONFIG_USB_HW_PARAM)
extern unsigned long long *get_hw_param(struct otg_notify *n,
					enum usb_hw_param index);
extern int inc_hw_param(struct otg_notify *n,
					enum usb_hw_param index);
extern int inc_hw_param_host(struct host_notify_dev *dev,
					enum usb_hw_param index);
extern int register_hw_param_manager(struct otg_notify *n,
					unsigned long (*fptr)(int));
#endif
extern void *get_notify_data(struct otg_notify *n);
extern void set_notify_data(struct otg_notify *n, void *data);
extern struct otg_notify *get_otg_notify(void);
extern void enable_usb_notify(void);
extern int set_otg_notify(struct otg_notify *n);
extern void put_otg_notify(struct otg_notify *n);
#else
static inline const char *event_string(enum otg_notify_events event)
			{return NULL; }
static inline const char *status_string(enum otg_notify_event_status status)
			{return NULL; }
static inline void send_usb_mdm_uevent(void) {}
static inline void send_usb_certi_uevent(int usb_certi) {}
static inline void send_usb_err_uevent(int usb_certi, int mode) {}
static inline void send_usb_itracker_uevent(int err_type) {}
static inline void send_usb_restrict_uevent(int usb_restrict) {}
static inline int usb_check_whitelist_for_mdm(struct usb_device *dev)
			{return 0; }
static inline int usb_check_whitelist_for_id(struct usb_device *dev)
			{return 0; }
static inline int usb_check_whitelist_for_serial(struct usb_device *dev)
			{return 0; }
extern inline int usb_check_whitelist_enable_state(void)
			{return 0; }
#ifndef CONFIG_DISABLE_LOCKSCREEN_USB_RESTRICTION
static inline int usb_check_allowlist_for_lockscreen_enabled_id(struct usb_device *dev)
			{return 0; }
static inline bool check_usb_restrict_lock_state(struct otg_notify *n) {return false; }
#endif
static inline int usb_otg_restart_accessory(struct usb_device *dev)
			{return 0; }
static inline void send_otg_notify(struct otg_notify *n,
					unsigned long event, int enable) { }
static inline int get_typec_status(struct otg_notify *n, int event) {return 0; }
static inline struct otg_booster *find_get_booster(struct otg_notify *n)
			{return NULL; }
static inline int register_booster(struct otg_notify *n,
					struct otg_booster *b) {return 0; }
static inline int register_ovc_func(struct otg_notify *n,
			int (*check_state)(void *), void *data) {return 0; }
static inline int get_booster(struct otg_notify *n) {return 0; }
static inline int get_usb_mode(struct otg_notify *n) {return 0; }
static inline unsigned long get_cable_type(struct otg_notify *n) {return 0; }
static inline int is_usb_host(struct otg_notify *n) {return 0; }
static inline bool is_blocked(struct otg_notify *n, int type) {return false; }
static inline bool is_snkdfp_usb_device_connected(struct otg_notify *n)
			{return false; }
static inline int get_con_dev_max_speed(struct otg_notify *n)
			{return 0; }
static inline void set_con_dev_max_speed
		(struct otg_notify *n, int speed) {}
static inline void set_con_dev_hub(struct otg_notify *n, int speed, int conn) {}
static inline  void set_request_action
		(struct otg_notify *n, unsigned int request_action) {}
static inline int is_known_usbaudio(struct usb_device *dev) {return 0; }
static inline int check_usbaudio(struct usb_device *dev) {return 0; }
static inline int check_usbgroup(struct usb_device *dev) {return 0; }
static inline int is_usbhub(struct usb_device *dev) {return 0; }
static inline void set_usb_audio_cardnum(int card_num,
		int bundle, int attach) {}
static inline void send_usb_audio_uevent(struct usb_device *dev,
		int cardnum, int attach) {}
static inline int send_usb_notify_uevent
			(struct otg_notify *n, char *envp_ext[]) {return 0; }
static inline int check_new_device_added(struct usb_device *udev) {return 0; }
static inline int set_lpm_charging_type_done(struct otg_notify *n,
		unsigned int state) {return 0; }
static inline int detect_illegal_condition(int type) {return 0; }
static inline int disconnect_unauthorized_device(struct usb_device *dev) {return 0; }
#if defined(CONFIG_USB_HW_PARAM)
static inline unsigned long long *get_hw_param(struct otg_notify *n,
			enum usb_hw_param index) {return NULL; }
static inline int inc_hw_param(struct otg_notify *n,
			enum usb_hw_param index) {return 0; }
static inline int inc_hw_param_host(struct host_notify_dev *dev,
			enum usb_hw_param index) {return 0; }
static inline int register_hw_param_manager(struct otg_notify *n,
			unsigned long (*fptr)(int)) {return 0; }
#endif
static inline void *get_notify_data(struct otg_notify *n) {return NULL; }
static inline void set_notify_data(struct otg_notify *n, void *data) {}
static inline struct otg_notify *get_otg_notify(void) {return NULL; }
static inline void enable_usb_notify(void) {}
static inline int set_otg_notify(struct otg_notify *n) {return 0; }
static inline void put_otg_notify(struct otg_notify *n) {}
#endif

#define unl_info(fmt, ...)						\
	({								\
		pr_info(fmt, ##__VA_ARGS__);			\
		printk_usb(NOTIFY_PRINTK_USB_NORMAL, fmt, ##__VA_ARGS__);			\
	})
#define unl_err(fmt, ...)						\
	({								\
		pr_err(fmt, ##__VA_ARGS__);			\
		printk_usb(NOTIFY_PRINTK_USB_NORMAL, fmt, ##__VA_ARGS__);			\
	})

#endif /* __LINUX_USB_NOTIFY_H__ */
