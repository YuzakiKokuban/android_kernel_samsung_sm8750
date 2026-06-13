/*
 * Copyrights (C) 2019 Samsung Electronics, Inc.
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
 */

#ifndef __PDIC_NOTIFIER_H__
#define __PDIC_NOTIFIER_H__

/* PDIC notifier call sequence,
 * largest priority number device will be called first.
 * refer Team Docs > ETC > 1. USB Type C >
 * code managing > pdic_notifier, pdic_notifier
 */
typedef enum {
	PDIC_NOTIFY_DEV_INITIAL		= 0,
	PDIC_NOTIFY_DEV_USB		= 1,
	PDIC_NOTIFY_DEV_BATT		= 2,
	PDIC_NOTIFY_DEV_PDIC		= 3,
	PDIC_NOTIFY_DEV_MUIC		= 4,
	PDIC_NOTIFY_DEV_CCIC		= 5,
	PDIC_NOTIFY_DEV_MANAGER		= 6,
	PDIC_NOTIFY_DEV_DP		= 7,
	PDIC_NOTIFY_DEV_USB_DP		= 8,
	PDIC_NOTIFY_DEV_SUB_BATTERY	= 9,
	PDIC_NOTIFY_DEV_SECOND_MUIC	= 10,
	PDIC_NOTIFY_DEV_DEDICATED_MUIC	= 11,
	PDIC_NOTIFY_DEV_ALL 		= 12,
} pdic_notifier_device;

typedef enum {
	PDIC_NOTIFY_ID_INITIAL			= 0,
	PDIC_NOTIFY_ID_ATTACH			= 1,
	PDIC_NOTIFY_ID_RID				= 2,
	PDIC_NOTIFY_ID_USB				= 3,
	PDIC_NOTIFY_ID_POWER_STATUS		= 4,
	PDIC_NOTIFY_ID_WATER			= 5,
	PDIC_NOTIFY_ID_VCONN			= 6,
	PDIC_NOTIFY_ID_OTG				= 7,
	PDIC_NOTIFY_ID_TA				= 8,
	PDIC_NOTIFY_ID_DP_CONNECT		= 9,
	PDIC_NOTIFY_ID_DP_HPD			= 10,
	PDIC_NOTIFY_ID_DP_LINK_CONF		= 11,
	PDIC_NOTIFY_ID_USB_DP			= 12,
	PDIC_NOTIFY_ID_ROLE_SWAP		= 13,
	PDIC_NOTIFY_ID_FAC				= 14,
	PDIC_NOTIFY_ID_CC_PIN_STATUS	= 15,
	PDIC_NOTIFY_ID_WATER_CABLE		= 16,
	PDIC_NOTIFY_ID_POFF_WATER		= 17,
	PDIC_NOTIFY_ID_DEVICE_INFO		= 18,
	PDIC_NOTIFY_ID_SVID_INFO		= 19,
	PDIC_NOTIFY_ID_CLEAR_INFO		= 20,
#if IS_ENABLED(CONFIG_MUIC_POGO)
	PDIC_NOTIFY_ID_POGO				= 21,
#endif
	PDIC_NOTIFY_ID_RPLEVEL		= 22,
} pdic_notifier_id_t;

typedef enum {
	RID_UNDEFINED	= 0,
	RID_000K		= 1,
	RID_001K		= 2,
	RID_255K		= 3,
	RID_301K		= 4,
	RID_523K		= 5,
	RID_619K		= 6,
	RID_OPEN		= 7,
	RID_056K		= 8,
} pdic_notifier_rid_t;

typedef enum {
	USB_STATUS_NOTIFY_DETACH	= 0,
	USB_STATUS_NOTIFY_ATTACH_DFP	= 1,
	USB_STATUS_NOTIFY_ATTACH_UFP	= 2,
	USB_STATUS_NOTIFY_ATTACH_DRP	= 3,
} USB_STATUS;

typedef enum {
	PDIC_NOTIFY_PIN_STATUS_NO_DETERMINATION = 0,
	PDIC_NOTIFY_PIN_STATUS_CC1_ACTIVE		= 1,
	PDIC_NOTIFY_PIN_STATUS_CC2_ACTIVE		= 2,
	PDIC_NOTIFY_PIN_STATUS_AUDIO_ACCESSORY	= 3,
	PDIC_NOTIFY_PIN_STATUS_DEBUG_ACCESSORY	= 4,
	PDIC_NOTIFY_PIN_STATUS_PDIC_ERROR		= 5,
	PDIC_NOTIFY_PIN_STATUS_DISABLED			= 6,
	PDIC_NOTIFY_PIN_STATUS_RFU				= 7,
	PDIC_NOTIFY_PIN_STATUS_NOCC_USB_ACTIVE	= 8,
} pdic_notifier_pin_status_t;

typedef enum {
	PDIC_NOTIFY_DP_PIN_UNKNOWN	= 0,
	PDIC_NOTIFY_DP_PIN_A		= 1,
	PDIC_NOTIFY_DP_PIN_B		= 2,
	PDIC_NOTIFY_DP_PIN_C		= 3,
	PDIC_NOTIFY_DP_PIN_D		= 4,
	PDIC_NOTIFY_DP_PIN_E		= 5,
	PDIC_NOTIFY_DP_PIN_F		= 6,
} pdic_notifier_dp_pinconf_t;

typedef enum {
	PDIC_NOTIFY_DETACH = 0,
	PDIC_NOTIFY_ATTACH = 1,
} pdic_notifier_attach_t;

typedef enum {
	PDIC_NOTIFY_DEVICE	= 0,
	PDIC_NOTIFY_HOST	= 1,
} pdic_notifier_attach_rprd_t;

typedef enum {
	PDIC_NOTIFY_LOW		= 0,
	PDIC_NOTIFY_HIGH	= 1,
	PDIC_NOTIFY_IRQ		= 2,
} pdic_notifier_dp_hpd_t;

typedef enum {
	PDIC_NOTIFY_PD_EVENT_DETACH = 0,
	PDIC_NOTIFY_PD_EVENT_CCIC_ATTACH,
	PDIC_NOTIFY_PD_EVENT_SINK,
	PDIC_NOTIFY_PD_EVENT_SOURCE,
	PDIC_NOTIFY_PD_EVENT_SINK_CAP,
	PDIC_NOTIFY_PD_EVENT_PRSWAP_SNKTOSRC,
} pdic_notifier_pd_event_t;

typedef struct {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t sub1:16;
	uint64_t sub2:16;
	uint64_t sub3:16;
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	void *pd;
#endif
} PD_NOTI_TYPEDEF;

/* ID = 1 : Attach */
typedef struct {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t attach:16;
	uint64_t rprd:16;
	uint64_t cable_type:16;
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	void *pd;
#endif
} PD_NOTI_ATTACH_TYPEDEF;

/* ID = 2 : RID */
typedef struct {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t rid:16;
	uint64_t sub2:16;
	uint64_t sub3:16;
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	void *pd;
#endif
} PD_NOTI_RID_TYPEDEF;

/* ID = 3 : USB status */
typedef struct {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t attach:16;
	uint64_t drp:16;
	uint64_t sub3:16;
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	void *pd;
#endif
} PD_NOTI_USB_STATUS_TYPEDEF;

typedef struct {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t is_connect:16;
	uint64_t hs_connect:16;
	uint64_t reserved:16;
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	void *pd;
#endif
} USB_DP_NOTI_TYPEDEF;

/* ID = 4 : POWER STATUS */
typedef struct {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t attach:16;
	uint64_t event:16;
	uint64_t data:16;
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	void *pd;
#endif
} PD_NOTI_POWER_STATUS_TYPEDEF;

/* ID = 18 : Device Info */
typedef struct {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t vendor_id:16;
	uint64_t product_id:16;
	uint64_t version:8;
	uint64_t ifpmic_index:8;
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	void *pd;
#endif
} PD_NOTI_DEVICE_INFO_TYPEDEF;

/* ID = 19 : Standard Vendor ID Info */
typedef struct {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t standard_vendor_id:16;
	uint64_t reserved:32;
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	void *pd;
#endif
} PD_NOTI_SVID_INFO_TYPEDEF;

/* ID = 20 : Clear ID Info */
typedef struct {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t clear_id:16;
	uint64_t reserved:32;
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	void *pd;
#endif
} PD_NOTI_CLEAR_INFO_TYPEDEF;

struct pdic_notifier_data {
	PD_NOTI_TYPEDEF pdic_template;
	struct blocking_notifier_head notifier_call_chain;

	struct mutex notify_mutex;
};

#define PDIC_NOTIFIER_BLOCK(name)	\
	struct notifier_block (name)

#define SET_PDIC_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_PDIC_NOTIFIER_BLOCK(nb)			\
		SET_PDIC_NOTIFIER_BLOCK(nb, NULL, -1)

extern int pdic_notifier_notify(PD_NOTI_TYPEDEF *noti, void *pd,
		int pdic_attach);
extern int pdic_notifier_register(struct notifier_block *nb,
		notifier_fn_t notifier, pdic_notifier_device listener);
extern int pdic_notifier_unregister(struct notifier_block *nb);
extern void pdic_uevent_work(int id, int state);

const char *pdic_event_src_string(pdic_notifier_device src);
const char *pdic_event_dest_string(pdic_notifier_device dest);
const char *pdic_event_id_string(pdic_notifier_id_t id);
const char *pdic_rid_string(pdic_notifier_rid_t rid);
const char *pdic_usbstatus_string(USB_STATUS usbstatus);

#ifndef MODULE
extern int pdic_notifier_init(void);
#endif
#endif /* __PDIC_NOTIFIER_H__ */
