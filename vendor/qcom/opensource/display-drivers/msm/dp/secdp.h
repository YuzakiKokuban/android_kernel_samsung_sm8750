/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __SECDP_H
#define __SECDP_H

#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
#include <linux/usb/typec/manager/usb_typec_manager_notifier.h>
#endif
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
#include <linux/usb/typec/common/pdic_notifier.h>
#endif
#include <linux/secdp_logger.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <linux/sched/clock.h>

/*#define MODEM_RF_INFO*/
#if defined(MODEM_RF_INFO) && IS_ENABLED(CONFIG_DEV_RIL_BRIDGE)
#include <linux/dev_ril_bridge.h>

struct rf_information {
	u8 rat;
	u32 band;
	u32 arfcn;
} __packed;
#endif

#include "dp_hpd.h"
#include "dp_power.h"
#include "dp_panel.h"
#include "dp_catalog.h"
#include "dp_parser.h"
#include "dp_link.h"
#include "secdp_sysfs.h"

/*defined at kmodule/usb/typec/common/pdic_core.h*/
#define SAMSUNG_VENDOR_ID	0x04E8

#define DEXDOCK_PRODUCT_ID      0xA020	/* EE-MG950 DeX Station */
#define DEXPAD_PRODUCT_ID       0xA029	/* EE-M5100 DeX Pad */
#define DEXCABLE_PRODUCT_ID     0xA048	/* EE-I3100 DeX Cable */
#define HG950_PRODUCT_ID        0xA025	/* EE-HG950 HDMI Adapter */
#define MPA2_PRODUCT_ID         0xA027	/* EE-P5000 Multiport Adapter */
#define MPA3_PRODUCT_ID         0xA056	/* EE-P3200 Multiport Adapter */
#define MPA4_PRODUCT_ID         0xA066	/* EE-P5400 Multiport Adapter */

#define SECDP_ENUM_STR(x)	#x

#define SECDP_USB_CONCURRENCY
#define SECDP_USE_WAKELOCK
#define SECDP_MAX_HBR2
/*#define SECDP_AUDIO_CTS*/
/*#define SECDP_HDCP_DISABLE*/
/*#define SECDP_TEST_HDCP2P2_REAUTH*/
/*#define NOT_SUPPORT_DEX_RES_CHANGE*/
#define REMOVE_YUV420_AT_PREFER
#define SYSFS_BW_CODE

#define DPCD_IEEE_OUI			0x500
#define DPCD_DEVID_STR			0x503

#define LEN_BRANCH_REV		3
#define DPCD_BRANCH_HW_REV		0x509
#define DPCD_BRANCH_SW_REV_MAJOR	0x50A
#define DPCD_BRANCH_SW_REV_MINOR	0x50B

#define MAX_CNT_LINK_STATUS_UPDATE	4
#define MAX_CNT_HDCP_RETRY		10

/* MST: max resolution, max refresh rate, max pclk */
#define MST_MAX_COLS	3840
#define MST_MAX_ROWS	2160
#define MST_MAX_FPS	30
#define MST_MAX_PCLK	300000

#define PDIC_DP_NOTI_REG_DELAY		1000

/* displayport self test */
#if defined(CONFIG_SECDP_DBG)
#define SECDP_SELF_TEST
#endif

#ifdef SECDP_SELF_TEST
#define ST_EDID_SIZE	256
#define ST_ARG_CNT	20
#define ST_TEST_EXIT	555

enum {
	ST_CLEAR_CMD,
	ST_LANE_CNT,
	ST_LINK_RATE,
	ST_CONNECTION_TEST,
	ST_HDCP_TEST,
	ST_PREEM_TUN,
	ST_VOLTAGE_TUN,
	ST_MAX,
};

struct secdp_sef_test_item {
	char cmd_str[20];
	int arg[ST_ARG_CNT];
	int arg_cnt;
	char arg_str[100];
	bool enabled;
	void (*clear)(void);
};

int  secdp_self_test_status(int cmd);
void secdp_self_test_start_reconnect(struct secdp_sysfs *dp_sysfs, void (*func)(struct secdp_misc *sec));
void secdp_self_test_start_hdcp_test(struct secdp_sysfs *dp_sysfs, void (*func_on)(void),
	void (*func_off)(void));
//void secdp_self_register_clear_func(int cmd, void (*func)(void));
int *secdp_self_test_get_arg(int cmd);
#endif/*SECDP_SELF_TEST*/

/* monitor aspect ratio */
enum mon_aspect_ratio_t {
	MON_RATIO_NA = -1,
	MON_RATIO_3_2,
	MON_RATIO_4_3,
	MON_RATIO_5_3,
	MON_RATIO_5_4,
	MON_RATIO_10P5_9,
	MON_RATIO_11_10,
	MON_RATIO_16_9,
	MON_RATIO_16_10,	/* same with 8:5 */
	MON_RATIO_21_9,
	MON_RATIO_21_10,
	MON_RATIO_32_9,
	MON_RATIO_32_10,
};

static inline char *secdp_aspect_ratio_to_string(enum mon_aspect_ratio_t ratio)
{
	switch (ratio) {
	case MON_RATIO_3_2:     return DP_ENUM_STR(MON_RATIO_3_2);
	case MON_RATIO_4_3:     return DP_ENUM_STR(MON_RATIO_4_3);
	case MON_RATIO_5_3:     return DP_ENUM_STR(MON_RATIO_5_3);
	case MON_RATIO_5_4:     return DP_ENUM_STR(MON_RATIO_5_4);
	case MON_RATIO_10P5_9:  return DP_ENUM_STR(MON_RATIO_10P5_9);
	case MON_RATIO_11_10:   return DP_ENUM_STR(MON_RATIO_11_10);
	case MON_RATIO_16_9:    return DP_ENUM_STR(MON_RATIO_16_9);
	case MON_RATIO_16_10:   return DP_ENUM_STR(MON_RATIO_16_10);
	case MON_RATIO_21_9:    return DP_ENUM_STR(MON_RATIO_21_9);
	case MON_RATIO_21_10:   return DP_ENUM_STR(MON_RATIO_21_10);
	case MON_RATIO_32_9:    return DP_ENUM_STR(MON_RATIO_32_9);
	case MON_RATIO_32_10:   return DP_ENUM_STR(MON_RATIO_32_10);
	case MON_RATIO_NA:      return DP_ENUM_STR(MON_RATIO_NA);
	default:                return "unknown";
	}
}

/* adapter type : SST or MST */
enum secdp_adapter_t {
	SECDP_ADT_UNKNOWN = -1,
	SECDP_ADT_SST = 10,
	SECDP_ADT_MST = 11,
};

/* dex supported resolutions */
enum dex_support_res_t {
	DEX_RES_NOT_SUPPORT = 0,
	DEX_RES_1600X900,  /* HD+ */
	DEX_RES_1920X1080, /* FHD */
	DEX_RES_1920X1200, /* WUXGA */
	DEX_RES_2560X1080, /* UW-UXGA */
	DEX_RES_2560X1440, /* QHD */
	DEX_RES_2560X1600, /* WQXGA */
	DEX_RES_3440X1440, /* UW-QHD */
	DEX_RES_END,
};
#define DEX_RES_DFT	DEX_RES_1920X1080   /* DeX default timing */
#define DEX_DFT_COL	1920
#define DEX_DFT_ROW	1080
#define DEX_RES_MAX	DEX_RES_3440X1440   /* DeX max timing */
#define DEX_MAX_COL	3440
#define DEX_MAX_ROW	1440

#define DEX_REFRESH_MIN		50
#define DEX_REFRESH_MAX		60
#define MIRROR_REFRESH_MIN	24

static inline char *secdp_dex_res_to_string(int res)
{
	switch (res) {
	case DEX_RES_NOT_SUPPORT:
		return DP_ENUM_STR(DEX_RES_NOT_SUPPORT);
	case DEX_RES_1600X900:
		return DP_ENUM_STR(DEX_RES_1600X900);
	case DEX_RES_1920X1080:
		return DP_ENUM_STR(DEX_RES_1920X1080);
	case DEX_RES_1920X1200:
		return DP_ENUM_STR(DEX_RES_1920X1200);
	case DEX_RES_2560X1080:
		return DP_ENUM_STR(DEX_RES_2560X1080);
	case DEX_RES_2560X1440:
		return DP_ENUM_STR(DEX_RES_2560X1440);
	case DEX_RES_2560X1600:
		return DP_ENUM_STR(DEX_RES_2560X1600);
	case DEX_RES_3440X1440:
		return DP_ENUM_STR(DEX_RES_3440X1440);
	default:
		return "unknown";
	}
}

enum DEX_STATUS {
	DEX_DISABLED = 0,
	DEX_ENABLED,
	DEX_MODE_CHANGING,
};

/** redriver devices */
enum secdp_redrv_dev {
	SECDP_REDRV_NONE = 0,
	SECDP_REDRV_PTN36502,	/* don't need AUX_SEL control */
	SECDP_REDRV_PS5169,	/* need AUX_SEL control */
};

static inline char *secdp_redrv_to_string(int res)
{
	switch (res) {
	case SECDP_REDRV_NONE:
		return DP_ENUM_STR(SECDP_REDRV_NONE);
	case SECDP_REDRV_PTN36502:
		return DP_ENUM_STR(SECDP_REDRV_PTN36502);
	case SECDP_REDRV_PS5169:
		return DP_ENUM_STR(SECDP_REDRV_PS5169);
	default:
		return "unknown";
	}
}

struct secdp_adapter {
	uint ven_id;
	uint prod_id;
	char ieee_oui[4];  /* DPCD 500h ~ 502h */
	char devid_str[7]; /* DPCD 503h ~ 508h */
	char fw_ver[10];   /* firmware ver, 0:h/w, 1:s/w major, 2:s/w minor */

	bool ss_genuine;
	bool ss_legacy;
	enum dex_support_res_t dex_type;
};

#define MON_NAME_LEN	14	/* monitor name length, max 13 chars + null */

#define MAX_NUM_HMD	32
#define DEX_TAG_HMD	"HMD"

struct secdp_sink_dev {
	uint ven_id;	/* vendor id from PDIC */
	uint prod_id;	/* product id from PDIC */
	char monitor_name[MON_NAME_LEN];	/* from EDID */
};

struct secdp_pdic_noti {
	struct delayed_work   reg_work;
	struct notifier_block nb;
	bool registered;
	bool reset;   /* true if PDIC or SSUSB get reset after DP connection */
};

struct secdp_prefer {
	enum mon_aspect_ratio_t	ratio;

	bool exist;   /* true if preferred resolution */
	int  hdisp;   /* horizontal pixel of preferred resolution */
	int  vdisp;   /* vertical pixel of preferred resolution */
	int  refresh; /* refresh rate of preferred resolution */
};

struct secdp_dex {
	struct class *sysfs_class;
	enum dex_support_res_t	res;	/* dex supported resolution */

	enum DEX_STATUS prev; /* previously known as "dex_now" */
	enum DEX_STATUS curr; /* previously known as "dex_en" */
	int  setting_ui;      /* "dex_set", true if setting has Dex mode */

	bool ignore_prefer_ratio; /* true if prefer ratio does not match to dex ratio */
	bool adapter_check_skip;

	/*
	 * 2 if resolution is changed during dex mode change.
	 * And once dex framework reads the dex_node_stauts using dex node,
	 * it's assigned to same value with curr.
	 */
	enum DEX_STATUS status; /* previously known as "dex_node_status" */

	bool reconnecting; /* true if dex is under reconnecting */
};

struct secdp_display_timing {
	u32  active_h;
	u32  active_v;
	u32  refresh_rate;
	bool interlaced;
	int  clock;    /* pixel clock, refer to "struct drm_display_mode" */
	enum dex_support_res_t dex_res;    /* dex supported resolution */
	enum mon_aspect_ratio_t mon_ratio; /* monitor aspect ratio */
	int  supported;                    /* for unit test */
	u64  total;
};

struct secdp_hmd {
	struct secdp_sink_dev list[MAX_NUM_HMD]; /* supported HMD dev list */
	struct mutex lock;
	bool exist;	/* true if connected sink is known HMD device */
};

struct secdp_hdcp {
	struct delayed_work start_work;
	int retry;	/* count if dp link is unstable during hdcp */
	u32 fail_cnt;
};

struct secdp_hpd {
	struct delayed_work noti_work;
	bool noti_deferred;
	atomic_t val;	/* 1 if hpd high, 0 if hpd low" */
	bool prev_evt;
};

struct secdp_debug {
	bool prefer_check_skip;
};

struct secdp_misc {
	struct device *dev;
	struct delayed_work link_status_work;
	struct delayed_work link_backoff_work;
	bool backoff_start;
	struct delayed_work poor_discon_work;

	struct device *uevent_dev;
#ifdef MODEM_RF_INFO
	struct rf_information rf_info;
	struct notifier_block modem_rfinfo_nb;
#endif

	bool extdisp_off;
	bool cable_connected; /* previously known as "cable_connected_phy" */
	bool link_conf;       /* previously known as "sec_link_conf" */
	struct secdp_hpd hpd;
	int mode_cnt;

	struct secdp_adapter adapter;
	struct secdp_pdic_noti pdic_noti;

	struct secdp_display_timing prf_timing; /* preferred timing */
	struct secdp_display_timing mrr_timing; /* max "mirror" timing */
	struct secdp_display_timing dex_timing; /* max "dex" timing */

	struct secdp_prefer prefer;
	struct secdp_hdcp hdcp;
	struct secdp_debug debug;
	struct secdp_sysfs *sysfs;
	struct secdp_dex dex;
	struct secdp_hmd hmd;

	struct completion dp_off_comp;
	struct completion dp_discon_comp;
	bool dp_disconnecting;
	bool lpm_booting;

	struct mutex notify_lock;
	struct mutex attention_lock;
	struct mutex notifier_lock;
	atomic_t noti_status;

	struct notifier_block reboot_nb;
	bool reboot; /* true if rebooted or shutdown */

#ifdef SECDP_USE_WAKELOCK
	struct wakeup_source *ws;
#endif
#ifdef SECDP_SELF_TEST
	struct delayed_work self_test_reconnect_work;
	struct delayed_work self_test_hdcp_test_work;

	void (*self_test_reconnect_cb)(struct secdp_misc *sec);
	void (*self_test_hdcp_on_cb)(void);
	void (*self_test_hdcp_off_cb)(void);
#endif
};

bool secdp_adapter_check_parade(struct secdp_misc *sec);
bool secdp_adapter_check_ps176(struct secdp_misc *sec);
bool secdp_adapter_check_ps176_legacy(struct secdp_misc *sec);
bool secdp_adapter_check_realtek(struct secdp_misc *sec);

bool secdp_get_lpm_mode(struct secdp_misc *sec);
int  secdp_send_deferred_hpd_noti(struct secdp_misc *sec);

int  secdp_pdic_noti_register_ex(struct secdp_misc *sec, bool retry);
bool secdp_phy_reset_check(struct device *dev);
bool secdp_get_power_status(struct device *dev);
bool secdp_get_cable_status(struct device *dev);
int  secdp_get_hpd_status(struct device *dev);
struct drm_connector *secdp_get_connector(struct secdp_misc *sec);

bool secdp_check_hmd_dev(struct secdp_misc *sec, const char *name_to_search);
int  secdp_store_hmd_dev(struct secdp_misc *sec, char *buf, size_t len, int num);

void secdp_timing_init(struct secdp_misc *sec);
void secdp_extdisp_on(struct secdp_misc *sec);
void secdp_extdisp_off(struct secdp_misc *sec);
void secdp_reconnect(struct secdp_misc *sec);
bool secdp_check_reconnect(struct secdp_misc *sec);

void secdp_link_backoff_start(void);
void secdp_link_backoff_stop(void);
bool secdp_adapter_is_legacy(struct platform_device *pdev);

bool secdp_panel_hdr_supported(void);

#endif/*__SECDP_H*/
