/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
 *
 *	Author: jb09.kim
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2012, Samsung Electronics. All rights reserved.

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

#ifndef SS_DSI_PANEL_COMMON_H
#define SS_DSI_PANEL_COMMON_H

#if IS_ENABLED(CONFIG_SEC_KUNIT)
#if IS_ENABLED(CONFIG_UML)
#include "kunit_test/ss_kunit_test_garbage_macro.h"
#endif
#include <kunit/mock.h>
#endif	/* #ifdef CONFIG_SEC_KUNIT */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/leds.h>
#include <linux/err.h>
#include <linux/lcd.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <asm/div64.h>
#include <linux/interrupt.h>
#include <linux/firmware.h>
#include <linux/namei.h>

#include <linux/sched.h>
#include <linux/dma-buf.h>
#include <linux/debugfs.h>
#include <linux/miscdevice.h>
#include <linux/reboot.h>
#include <video/mipi_display.h>
#if IS_ENABLED(CONFIG_DEV_RIL_BRIDGE)
#include <linux/dev_ril_bridge.h>
#if IS_ENABLED(CONFIG_SDP)
#include <linux/sdp/adaptive_mipi_v2.h>
#include <linux/sdp/adaptive_mipi_v2_cp_info.h>
#endif
#endif
#include <linux/regulator/consumer.h>
#if IS_ENABLED(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif

#include "dsi_display.h"
#include "dsi_panel.h"
#include "dsi_drm.h"
#include "sde_kms.h"
#include "sde_connector.h"
#include "sde_encoder.h"
#include "sde_encoder_phys.h"
#include "sde_trace.h"

#if IS_ENABLED(CONFIG_DRM_SDE_VM)
#include "sde_vm.h"
#endif

#include "ss_dpui_common.h"

#include "ss_dsi_panel_sysfs.h"
#include "ss_dsi_panel_debug.h"

#include "ss_copr_common.h"

#include "./SELF_DISPLAY/self_display.h"
#include "./MAFPC/ss_dsi_mafpc.h"

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
#include <linux/sec_panel_notifier_v2.h>
#endif

#if IS_ENABLED(CONFIG_SEC_DEBUG)
#include <linux/sec_debug.h>
#endif

#if IS_ENABLED(CONFIG_SEC_PARAM)
#include <linux/samsung/bsp/sec_param.h>
#endif

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
#include <linux/input/sec_input.h>
#endif

#include "ss_wrapper_common.h"
#include "ss_panel_parse.h"

#include <linux/hashtable.h>

extern bool enable_pr_debug;

#define LCD_DEBUG(V, X, ...)	\
		do {	\
			if (enable_pr_debug)	\
				pr_info("[SDE_%d] %s : "X, V ? V->ndx : 0, __func__, ## __VA_ARGS__);\
			else	\
				pr_debug("[SDE_%d] %s : "X, V ? V->ndx : 0, __func__, ## __VA_ARGS__);\
		} while (0)

#if IS_ENABLED(CONFIG_SEC_KUNIT)
/* Too much log causes kunit test app crash */
#define LCD_INFO_IF(V, X, ...) \
	do { \
		if (V->debug_data && V->debug_data->print_cmds) \
			pr_debug("[SDE_%d] %s : "X, V ? V->ndx : 0, __func__, ## __VA_ARGS__); \
	} while (0)

#define LCD_INFO(V, X, ...) pr_info("[%llu.%llu][SDE_%d] %s : "X, ktime_to_ms(ktime_get())/1000, ktime_to_ms(ktime_get())%1000, V ? V->ndx : 0, __func__, ## __VA_ARGS__)
#define LCD_INFO_ONCE(V, X, ...) pr_info_once("[%llu.%llu][SDE_%d] %s : "X, ktime_to_ms(ktime_get())/1000, ktime_to_ms(ktime_get())%1000, V ? V->ndx : 0, __func__, ## __VA_ARGS__)
#define LCD_INFO_NOVDD(X, ...) pr_info("[SDE] %s : "X, __func__, ## __VA_ARGS__)
#define LCD_ERR(V, X, ...) pr_err("[%lld.%lld][SDE_%d] %s : error: "X, ktime_to_ms(ktime_get())/1000, ktime_to_ms(ktime_get())%1000, V ? V->ndx : 0, __func__, ## __VA_ARGS__)
#define LCD_ERR_NOVDD(X, ...) pr_err("[SDE] %s : error: "X, __func__, ## __VA_ARGS__)
#define LCD_INFO_CRITICAL(ndx, X, ...) pr_err("[SDE_%d] %s : "X, ndx, __func__, ## __VA_ARGS__)
#define LCD_WARN(V, X, ...) pr_warn("[SDE_%d] %s : "X, V ? V->ndx : 0, __func__, ## __VA_ARGS__)
#define LCD_WARN_NOVDD(X, ...) pr_warn("[SDE] %s : "X, __func__, ## __VA_ARGS__)
#else /* #if IS_ENABLED(CONFIG_SEC_KUNIT) */
#define LCD_INFO_IF(V, X, ...) \
	do { \
		if (V->debug_data && (V->debug_data->print_all || V->debug_data->print_cmds)) \
			pr_info("[SDE_%d] %s : "X, V ? V->ndx : 0, __func__, ## __VA_ARGS__); \
	} while (0)

#define LCD_INFO(V, X, ...) pr_info("[SDE_%d] %s : "X, V ? V->ndx : 0, __func__, ## __VA_ARGS__)
#define LCD_INFO_NOVDD(X, ...) pr_info("[SDE] %s : "X, __func__, ## __VA_ARGS__)
#define LCD_INFO_ONCE(V, X, ...) pr_info_once("[SDE_%d] %s : "X, V ? V->ndx : 0, __func__, ## __VA_ARGS__)
#define LCD_ERR(V, X, ...) pr_err("[SDE_%d] %s : error: "X, V ? V->ndx : 0, __func__, ## __VA_ARGS__)
#define LCD_ERR_NOVDD(X, ...) pr_err("[SDE] %s : error: "X, __func__, ## __VA_ARGS__)
#define LCD_INFO_CRITICAL(ndx, X, ...) pr_err("[SDE_%d] %s : "X, ndx, __func__, ## __VA_ARGS__)
#define LCD_WARN(V, X, ...) pr_warn("[SDE_%d] %s : "X, V ? V->ndx : 0, __func__, ## __VA_ARGS__)
#define LCD_WARN_NOVDD(X, ...) pr_warn("[SDE] %s : "X, __func__, ## __VA_ARGS__)
#endif /* #ifdef CONFIG_SEC_KUNIT */

#define MAX_PANEL_NAME_SIZE 100

#define PARSE_STRING 64
#define MAX_BACKLIGHT_TFT_GPIO 4
#define	CHECK_VSYNC_COUNT 3
#define	CHECK_EARLY_TE_COUNT 3

/* Register dump info */
#define MAX_INTF_NUM 2

/* PBA booting ID */
#define PBA_ID 0xFFFFFF

/* Default elvss_interpolation_lowest_temperature */
#define ELVSS_INTERPOLATION_TEMPERATURE -20

/* Default lux value for entering mdnie HBM */
#define MDNIE_HBM_CE_LUX 40000

/* MAX ESD Recovery gpio */
#define MAX_ESD_GPIO 2

#define MDNIE_TUNE_MAX_SIZE 6

#define USE_CURRENT_BL_LEVEL 0xFFFFFF

#define RX_SIZE_LIMIT	10

#define SDE_CLK_WORK_DELAY	200

#define PANEL_DATA_NAME_MAIN "panel_data_main"
#define PANEL_DATA_NAME_SUB "panel_data_sub"

/* Some project, like sm7225, doesn't support kunit test .
 * To avoid build error, define __mockable that is defined in kunit/mock.h.
 */
#ifndef __mockable
#define __mockable
#endif

#ifndef __visible_for_testing
#define __visible_for_testing static
#endif

struct vrr_info;
struct lfd_base_str;
enum LFD_SCOPE_ID;

enum SS_CMD_CTRL {
	CMD_CTRL_WRITE = 0,
	CMD_CTRL_READ,
	CMD_CTRL_WRITE_TYPE,
	CMD_CTRL_READ_TYPE,
};

enum SS_CMD_CTRL_STR {
	CMD_STR_NUL = 0x0,	/* '\0' */
	CMD_STR_TAB = 0x9,	/* '\t' */
	CMD_STR_SPACE = 0x20,	/* ' ' */
	CMD_STR_QUOT = 0x22,	/* '"' */
	CMD_STR_CRLF = 0xA,	/* '\n' */
	CMD_STR_CR = 0xD,	/* '\r' */
	CMD_STR_EQUAL = 0x3D,	/* '=' */
};

/* operator */
#define CMDSTR_WRITE	"W"
#define CMDSTR_READ	"R"
#define CMDSTR_DELAY	"Delay"

#define CMDSTR_WRITE_TYPE	"WT"
#define CMDSTR_READ_TYPE	"RT"
#define CMDSTR_IF	"IF"
#define CMDSTR_ELSE	"ELSE"
#define CMDSTR_UPDATE	"UPDATE"

#define CMDSTR_END	"END"
#define CMDSTR_APPLY	"APPLY"

#define CMDSTR_AND	"AND"
#define CMDSTR_OR	"OR"
#define CMDSTR_THEN	"THEN"

#define CMDSTR_COMMENT "//"
#define CMDSTR_COMMENT2 "/*"
#define CMDSTR_COMMENT3 "*/"

#define IS_CMD_DELIMITER(c)	((c) == CMD_STR_SPACE || (c) == CMD_STR_CRLF || (c) == CMD_STR_TAB || (c) == CMD_STR_CR)
#define IS_CMD_NEWLINE(c)	((c) == CMD_STR_CRLF || (c) == CMD_STR_CR)

#define GET_PRIM_MS(c) ((c) / MSEC_PER_SEC)
#define GET_REST_MS(c) ((c) % MSEC_PER_SEC)

#define GET_PRIM_US(c) ((c) / USEC_PER_SEC)
#define GET_REST_US(c) ((c) % USEC_PER_SEC)

#define GET_PRIM_NS(c) ((c) / NSEC_PER_SEC)
#define GET_REST_NS(c) ((c) % NSEC_PER_SEC)

enum PANEL_LEVEL_KEY {
	LEVEL_KEY_NONE = 0,
	LEVEL0_KEY = BIT(0),
	LEVEL1_KEY = BIT(1),
	LEVEL2_KEY = BIT(2),
};

enum backlight_origin {
	BACKLIGHT_NORMAL,
	BACKLIGHT_FINGERMASK_ON,
	BACKLIGHT_FINGERMASK_ON_SUSTAIN,
	BACKLIGHT_FINGERMASK_OFF,
};

enum br_mode {
	FLASH_GAMMA = 0, /* read gamma related data from flash memory */
	TABLE_GAMMA,	 /* use gamma related data from fixed values */
	GAMMA_MODE2,	 /* use gamma mode2 */
	MAX_BR_MODE,
};

enum BR_TYPE {
	BR_TYPE_NORMAL = 0,
	BR_TYPE_HBM,
	BR_TYPE_HMT,
	BR_TYPE_AOD,
	BR_TYPE_MAX,
};

#define SAMSUNG_DISPLAY_PINCTRL0_STATE_DEFAULT "samsung_display_gpio_control0_default"
#define SAMSUNG_DISPLAY_PINCTRL0_STATE_SLEEP  "samsung_display_gpio_control0_sleep"
#define SAMSUNG_DISPLAY_PINCTRL1_STATE_DEFAULT "samsung_display_gpio_control0_default"
#define SAMSUNG_DISPLAY_PINCTRL1_STATE_SLEEP  "samsung_display_gpio_control0_sleep"

#define SAMSUNG_DISPLAY_PANEL_NAME "samsung_display_panel"

enum {
	SAMSUNG_GPIO_CONTROL0,
	SAMSUNG_GPIO_CONTROL1,
};

enum IRC_MODE {
	IRC_MODERATO_MODE = 0,
	IRC_FLAT_GAMMA_MODE,
	IRC_FLAT_Z_GAMMA_MODE,
	IRC_MAX_MODE,
};

enum {
	VDDM_ORG = 0,
	VDDM_LV,
	VDDM_HV,
	MAX_VDDM,
};

enum {
	LPM_MODE_OFF = 0,			/* Panel LPM Mode OFF */
	ALPM_MODE_ON,			/* ALPM Mode On */
	HLPM_MODE_ON,			/* HLPM Mode On */
	MAX_LPM_MODE,			/* Panel LPM Mode MAX */
};

struct lpm_info {
	bool is_support;
	int hlpm_mode;
	u8 origin_mode;
	u8 mode;
	u8 hz;
	bool esd_recovery;
	int need_self_grid;
	bool during_ctrl; /* For lego, regards as LPM mode if during_ctrl is set */
	int entry_frame;
	int entry_delay;
	int exit_frame;
	int exit_delay;
	bool need_br_update;
	struct mutex lpm_lock;
};

struct clk_timing_table {
	int tab_size;
	int *clk_rate;
};

/* TBR: make new structure including rat/band/from/end/clk/osc
 * and allocate memory for the structure.
 */
struct clk_sel_table {
	int col;
	int tab_size;
	int *rat;
	int *band;
	int *from;
	int *end;
	int *target_clk_idx;
	int *target_osc_idx;
};

struct rf_info {
	u8 rat;
	u32 band;
	u32 arfcn;
} __packed;

struct dyn_mipi_clk {
	/* dynamic mipi v1 */
	struct notifier_block notifier;
	struct mutex dyn_mipi_lock; /* protect rf_info's rat, band, and arfcn */
	struct clk_sel_table clk_sel_table;
	struct rf_info rf_info;

#if IS_ENABLED(CONFIG_SDP)
	/* adaptive mipi v2 */
	bool is_adaptive_mipi_v2;
	struct adaptive_mipi_v2_info adaptive_mipi_v2_info;
#endif

	/* common */
	struct clk_timing_table clk_timing_table;
	int is_support;
	int osc_support;
	int force_idx;  /* force to set clk idx for test purpose */

	/*
		requested_clk_rate & requested_clk_idx
		should be updated at same time for concurrency.
		clk_rate is changed, updated clk_idx should be used.
	*/
	int requested_clk_rate;
	int requested_clk_idx;
	int requested_osc_idx;
};

struct cmd_legoop_map {
	/* TODO: change variable name: it was originated for u8 mipi commands,
	 * but now, used for integer value, also..
	 */
	int **cmds;
	int col_size;
	int row_size;
};

enum CD_MAP_TABLE_LIST {
	NORMAL = 0,
	HBM,
	AOD,
	HMT,
	CD_MAP_TABLE_MAX,
};

struct candela_map_data {
	int bl_idx;
	int bl_level;
	int wrdisbv;	/* Gamma mode2 WRDISBV Write Display Brightness */
	int cd;	/* This is cacultated brightness to standardize brightness formula */
	int abc_scale_idx; /* ABC scale idx */
};

struct candela_map_table {
	struct candela_map_data *cd_map;
	int tab_size;
	int min_lv;
	int max_lv;
};

enum ss_dsi_cmd_set_type {
	SS_DSI_CMD_SET_START = DSI_CMD_SET_MAX + 1,

	/* TX */
	TX_CMD_START,

	/* Level key comamnds are referred while parsing other commands.
	 * Put level key first.
	 */
	TX_LEVEL_KEY_START,
	TX_LEVEL0_KEY_ENABLE,
	TX_LEVEL0_KEY_DISABLE,
	TX_LEVEL1_KEY_ENABLE,
	TX_LEVEL1_KEY_DISABLE,
	TX_LEVEL2_KEY_ENABLE,
	TX_LEVEL2_KEY_DISABLE,
	TX_LEVEL_KEY_END,

	TX_DSI_CMD_SET_ON,
	TX_DSI_CMD_SET_ON_ESYNC_POST, /* after ESYNC On */
	TX_DSI_CMD_SET_ON_POST,  /* after frame update */
	TX_DSI_CMD_SET_OFF,
	TX_TIMING_SWITCH,
	TX_MTP_WRITE_SYSFS,
	TX_TEMP_DSC,
	TX_DISPLAY_ON,
	TX_DISPLAY_ON_POST,
	TX_FIRST_DISPLAY_ON,
	TX_DISPLAY_OFF,
	TX_SS_BRIGHT_CTRL,
	TX_SS_SMOOTH_BR_DONE,
	TX_SS_LPM_BRIGHT_CTRL,
	TX_SS_HMT_BRIGHT_CTRL,
	TX_SW_RESET,
	TX_MDNIE_ADB_TEST,
	TX_SELF_GRID_ON,
	TX_SELF_GRID_OFF,
	TX_MIPI_LANE_SET,

	TX_LPM_ON,
	TX_LPM_OFF,
	TX_LPM_ON_AOR,
	TX_LPM_OFF_AOR,

	TX_PACKET_SIZE,
	TX_MDNIE_TUNE,
	TX_HMT_ENABLE,
	TX_HMT_DISABLE,
	TX_FFC,
	TX_OSC,
	TX_TFT_PWM,
	TX_COPR_ENABLE,
	TX_COPR_DISABLE,

	TX_DYNAMIC_HLPM_ENABLE,
	TX_DYNAMIC_HLPM_DISABLE,

	TX_MCD_ON,
	TX_MCD_OFF,
	TX_XTALK_ON,
	TX_XTALK_OFF,
	TX_IRC,
	TX_IRC_OFF,
	TX_SMOOTH_DIMMING_ON,
	TX_SMOOTH_DIMMING_OFF,

	TX_SS_DIA,
	TX_SELF_IDLE_AOD_ENTER,
	TX_SELF_IDLE_AOD_EXIT,
	TX_SELF_IDLE_TIMER_ON,
	TX_SELF_IDLE_TIMER_OFF,
	TX_SELF_IDLE_MOVE_ON_PATTERN1,
	TX_SELF_IDLE_MOVE_ON_PATTERN2,
	TX_SELF_IDLE_MOVE_ON_PATTERN3,
	TX_SELF_IDLE_MOVE_ON_PATTERN4,
	TX_SELF_IDLE_MOVE_OFF,
	TX_SPSRAM_DATA_READ,

	/* SELF DISPLAY */
	TX_SELF_DISP_CMD_START,
	TX_SELF_DISP_ON,
	TX_SELF_DISP_OFF,
	TX_SELF_TIME_SET,
	TX_SELF_MOVE_ON,
	TX_SELF_MOVE_ON_100,
	TX_SELF_MOVE_ON_200,
	TX_SELF_MOVE_ON_500,
	TX_SELF_MOVE_ON_1000,
	TX_SELF_MOVE_ON_DEBUG,
	TX_SELF_MOVE_RESET,
	TX_SELF_MOVE_OFF,
	TX_SELF_MOVE_2C_SYNC_OFF,
	TX_SELF_MASK_SETTING,
	TX_SELF_MASK_SIDE_MEM_SET,
	TX_SELF_MASK_ON,//TBR
	TX_SELF_MASK_SET_PRE,
	TX_SELF_MASK_SET_POST,
	TX_SELF_MASK_ON_FACTORY,
	TX_SELF_MASK_OFF,
	TX_SELF_MASK_UDC_ON,
	TX_SELF_MASK_UDC_OFF,
	TX_SELF_MASK_GREEN_CIRCLE_ON,		/* Finger Print Green Circle */
	TX_SELF_MASK_GREEN_CIRCLE_OFF,
	TX_SELF_MASK_GREEN_CIRCLE_ON_FACTORY,
	TX_SELF_MASK_GREEN_CIRCLE_OFF_FACTORY,
	TX_SELF_MASK_IMAGE,
	TX_SELF_MASK_IMAGE_CRC,
	TX_SELF_ICON_SET_PRE,
	TX_SELF_ICON_SET_POST,
	TX_SELF_ICON_SIDE_MEM_SET,
	TX_SELF_ICON_GRID,
	TX_SELF_ICON_ON,
	TX_SELF_ICON_ON_GRID_ON,
	TX_SELF_ICON_ON_GRID_OFF,
	TX_SELF_ICON_OFF_GRID_ON,
	TX_SELF_ICON_OFF_GRID_OFF,
	TX_SELF_ICON_GRID_2C_SYNC_OFF,
	TX_SELF_ICON_OFF,
	TX_SELF_ICON_IMAGE,
	TX_SELF_BRIGHTNESS_ICON_ON,
	TX_SELF_BRIGHTNESS_ICON_OFF,
	TX_SELF_ACLOCK_SET_PRE,
	TX_SELF_ACLOCK_SET_POST,
	TX_SELF_ACLOCK_SIDE_MEM_SET,
	TX_SELF_ACLOCK_ON,
	TX_SELF_ACLOCK_TIME_UPDATE,
	TX_SELF_ACLOCK_ROTATION,
	TX_SELF_ACLOCK_OFF,
	TX_SELF_ACLOCK_HIDE,
	TX_SELF_ACLOCK_IMAGE,
	TX_SELF_DCLOCK_SET_PRE,
	TX_SELF_DCLOCK_SET_POST,
	TX_SELF_DCLOCK_SIDE_MEM_SET,
	TX_SELF_DCLOCK_ON,
	TX_SELF_DCLOCK_BLINKING_ON,
	TX_SELF_DCLOCK_BLINKING_OFF,
	TX_SELF_DCLOCK_TIME_UPDATE,
	TX_SELF_DCLOCK_OFF,
	TX_SELF_DCLOCK_HIDE,
	TX_SELF_DCLOCK_IMAGE,
	TX_SELF_CLOCK_2C_SYNC_OFF,
	TX_SELF_VIDEO_IMAGE,
	TX_SELF_VIDEO_SIDE_MEM_SET,
	TX_SELF_VIDEO_ON,
	TX_SELF_VIDEO_OFF,
	TX_SELF_PARTIAL_HLPM_SCAN_SET,
	RX_SELF_DISP_DEBUG,
	TX_SELF_MASK_CHECK,//TBR
	TX_SELF_MASK_CHECK_PRE1,
	TX_SELF_MASK_CHECK_PRE2,
	TX_SELF_MASK_CHECK_POST,
	RX_SELF_MASK_CHECK,
	TX_SELF_DISP_CMD_END,

	/* MAFPC (ABC) */
	TX_MAFPC_CMD_START,
	TX_MAFPC_FLASH_SEL,
	TX_MAFPC_BRIGHTNESS_SCALE,
	RX_MAFPC_READ_1,
	RX_MAFPC_READ_2,
	RX_MAFPC_READ_3,
	TX_MAFPC_SET_PRE_FOR_INSTANT,
	TX_MAFPC_SET_PRE,
	TX_MAFPC_SET_PRE2,
	TX_MAFPC_SET_POST,
	TX_MAFPC_SET_POST_FOR_INSTANT,
	TX_MAFPC_SETTING,
	TX_MAFPC_SETTING_INSTANCE,
	TX_MAFPC_CTRL_DATA,
	TX_MAFPC_ON,
	TX_MAFPC_ON_FACTORY,
	TX_MAFPC_OFF,
	TX_MAFPC_TE_ON,
	TX_MAFPC_TE_OFF,
	TX_MAFPC_IMAGE,
	TX_MAFPC_CRC_CHECK_IMAGE,
	TX_MAFPC_CRC_CHECK_PRE1,
	TX_MAFPC_CRC_CHECK_PRE2,
	TX_MAFPC_CRC_CHECK_POST,
	TX_MAFPC_CRC_CHECK, //TBR
	RX_MAFPC_CRC_CHECK,
	TX_MAFPC_CMD_END,

	/* TEST MODE */
	TX_TEST_MODE_CMD_START,

	RX_GCT_ECC,	/* Error Correction Code */
 	RX_SSR,		/* Self Source Repair */
 	RX_SSR_ON,
	RX_SSR_CHECK,

	TX_GCT_ENTER,
	TX_GCT_LV,
	TX_GCT_HV,

	TX_GRAY_SPOT_TEST_ON,
	TX_GRAY_SPOT_TEST_OFF,
	TX_VGLHIGHDOT_TEST_ON,
	TX_VGLHIGHDOT_TEST_2, /* for 1hz VGL -6V AT CMD */
	TX_VGLHIGHDOT_TEST_OFF,
	TX_MICRO_SHORT_TEST_ON,
	TX_MICRO_SHORT_TEST_OFF,
	TX_CCD_ON,
	TX_CCD_OFF,
	TX_BRIGHTDOT_ON,
	TX_BRIGHTDOT_OFF,
	TX_BRIGHTDOT_LF_ON,
	TX_BRIGHTDOT_LF_OFF,
	TX_DSC_CRC,
	TX_PANEL_AGING_ON,
	TX_PANEL_AGING_OFF,
	TX_TEST_MODE_CMD_END,

	/* FLASH GAMMA */
	TX_FLASH_GAMMA_PRE1,
	TX_FLASH_GAMMA_PRE2,
	TX_FLASH_GAMMA,
	TX_FLASH_GAMMA_POST,

	TX_FD_ON,
	TX_FD_OFF,

	TX_VRR_GM2_GAMMA_COMP,
	TX_VRR_GM2_GAMMA_COMP2,
	TX_VRR_GM2_GAMMA_COMP3,

	TX_DFPS,

	TX_ADJUST_TE,

	TX_FG_ERR,

	TX_EARLY_TE,

	TX_VLIN1_TEST_ENTER,
	TX_VLIN1_TEST_EXIT,

	TX_LOCAL_HBM_ENTER,
	TX_LOCAL_HBM_ON_CIRCLE_OFF,
	TX_LOCAL_HBM_ON_CIRCLE_ON,
	TX_LOCAL_HBM_COMPENSATION,
	TX_LOCAL_HBM_EXIT,
	TX_CMD_END,

	/* RX */
	RX_CMD_START,
	RX_MANUFACTURE_ID,
	RX_MANUFACTURE_ID0,
	RX_MANUFACTURE_ID1,
	RX_MANUFACTURE_ID2,
	RX_MODULE_INFO,
	RX_MANUFACTURE_DATE,
	RX_DDI_ID,
	RX_CELL_ID,
	RX_OCTA_ID,
	RX_RDDPM,
	RX_MTP_READ_SYSFS,
	RX_MDNIE,
	RX_LDI_DEBUG0, /* 0x0A : RDDPM */
	RX_LDI_DEBUG1,
	RX_LDI_DEBUG2, /* 0xEE : ERR_FG */
	RX_LDI_DEBUG3, /* 0x0E : RDDSM */
	RX_LDI_DEBUG4, /* 0x05 : DSI_ERR */
	RX_LDI_DEBUG5, /* 0x0F : OTP loading error count */
	RX_LDI_DEBUG6, /* 0xE9 : MIPI protocol error  */
	RX_LDI_DEBUG7, /* 0x03 : DSC enable/disable  */
	RX_LDI_DEBUG8, /* 0xA2 : PPS cmd  */
	RX_LDI_DEBUG_LOGBUF, /* 0x9C : command log buffer */
	RX_LDI_DEBUG_PPS1, /* 0xA2 : PPS data (0x00 ~ 0x2C) */
	RX_LDI_DEBUG_PPS2, /* 0xA2 : PPS data (0x2d ~ 0x58)*/
	RX_LDI_LOADING_DET,
	RX_FLASH_GAMMA,
	RX_FLASH_LOADING_CHECK,
	RX_UDC_DATA,
	RX_LOCAL_HBM_COMPENSATION,
	RX_CMD_END,

	SS_DSI_CMD_SET_MAX,
};

enum ss_cmd_set_state {
	SS_CMD_SET_STATE_LP = 0,
	SS_CMD_SET_STATE_HS,
	SS_CMD_SET_STATE_MAX
};

/* updatable status
 * SS_CMD_NO_UPDATABLE: normal commands, do not allow to update the value
 * SS_CMD_UPDATABLE: 0xXX in panel dtsi, default value is 0, and update in driver
 *                   used by "gray spot off" and "each mode"
 */
enum ss_cmd_update {
	SS_CMD_NO_UPDATABLE = 0,
	SS_CMD_UPDATABLE,
};

enum ss_cmd_op {
	SS_CMD_OP_NONE = 0,
	SS_CMD_OP_IF,
	SS_CMD_OP_ELSE,
	SS_CMD_OP_IF_BLOCK,
	SS_CMD_OP_UPDATE,
};

enum ss_cmd_op_cond  {
	SS_CMD_OP_COND_AND = 0,
	SS_CMD_OP_COND_OR,
};

/* IF BLOCK
 * op = SS_CMD_OP_IF_BLOCK
 * op_symbol = "VRR"
 * op_val = "60HS"
 * op = SS_CMD_OP_IF_BLOCK
 * op_symbol = "MODE"
 * op_val = "NORMAL"
 * candidate_buf = NULL
 * candidate_len = 0
 * ...
 *
 * IF
 * op = SS_CMD_OP_IF
 * op_symbol = "VRR"
 * op_val = "60HS"
 * candidate_buf = "0x03 0x0A"
 * candidate_len = 2
 * op_symbol = "VRR"
 * op_val = "48"
 * candidate_buf = "0x01 0x25"
 * candidate_len = 2
 * ...
 *
 * UPDATE
 * op = SS_CMD_OP_UPDATE
 * op_symbol = "BRIGHTNESS"
 * op_val = NULL
 * candidate_buf = NULL
 * candidate_len = 0
 *
 */
struct ss_cmd_op_str {
	struct list_head list;
	struct list_head sibling;

	enum ss_cmd_op op;
	char *symbol;
	char *val;

	enum ss_cmd_op_cond cond;
	u8 *candidate_buf; /* include whole cmd, not only 0xXX */
	size_t candidate_len;
};

struct ss_cmd_desc {
	struct dsi_cmd_desc *qc_cmd;

	bool last_command;
	u32  post_wait_ms;
	u32  post_wait_frame;

	u8 type; /* ex: 29h, 39h, 05h, or 06h */
	size_t tx_len;
	u8 *txbuf;
	size_t rx_len;
	u8 *rxbuf;
	int rx_offset; /* read_startoffset */

	enum ss_cmd_update updatable; /* if true, allow to change txbuf's value */

	struct list_head op_list;
	bool skip_by_cond; /* skip both command transmission and post_wait_ms delay */

	/* deprecated */
	bool skip_lvl_key; /* skip only command transmission, but delay post_wait_ms */

	u8 *pos_0xXX; /* used for only 'UPDATE' op */
};

struct ss_cmd_set {
	struct dsi_panel_cmd_set base;

	/* Even samsung,allow_level_key_once property is set,
	 * some commands, like GCT command, should send level key
	 * in the middle of commands.
	 * Declare like samsung,gct_lv_tx_cmds_revA_skip_tot_lvl in panel dtsi.
	 */
	bool is_skip_tot_lvl;

	u32 count;
	struct ss_cmd_desc *cmds;

	enum ss_dsi_cmd_set_type ss_type;
	enum ss_cmd_set_state state;

	char *name;

	struct ss_cmd_set *cmd_set_rev[SUPPORT_PANEL_REVISION];

	void *self_disp_cmd_set_rev;
};

#define SS_CMD_PROP_SIZE ((SS_DSI_CMD_SET_MAX) - (SS_DSI_CMD_SET_START) + 1)

extern char ss_cmd_set_prop_map[SS_CMD_PROP_SIZE][SS_CMD_PROP_STR_LEN];

struct samsung_display_dtsi_data {
	bool samsung_lp11_init;
	bool samsung_tcon_clk_on_support;
	bool samsung_esc_clk_128M;
	bool samsung_support_factory_panel_swap;
	u32  samsung_power_on_reset_delay;
	u32 lpm_swire_delay;
	u32 samsung_lpm_init_delay;	/* 2C/3C memory access -> wait for samsung_lpm_init_delay ms -> HLPM on seq. */
	u8	samsung_delayed_display_on;

	/* If display->ctrl_count is 2, it broadcasts.
	 * To prevent to send mipi cmd thru mipi dsi1, set samsung_cmds_unicast flag.
	 */
	bool samsung_cmds_unicast;

	/* To reduce DISPLAY ON time */
	u32 samsung_reduce_display_on_time;
	u32 samsung_dsi_force_clock_lane_hs;
	s64 after_reset_delay;
	s64 sleep_out_to_on_delay;
	u32 samsung_finger_print_irq_num;
	u32 samsung_home_key_irq_num;

	int backlight_tft_gpio[MAX_BACKLIGHT_TFT_GPIO];

	/* PANEL TX / RX CMDS LIST */
	struct ss_cmd_set ss_cmd_sets[SS_CMD_PROP_SIZE];

	/* TFT LCD Features*/
	int tft_common_support;
	int backlight_gpio_config;
	int pwm_ap_support;
	const char *tft_module_name;
	const char *panel_vendor;
	const char *disp_model;

	/* MDINE HBM_CE_TEXT_MDNIE mode used */
	int hbm_ce_text_mode_support;

	/* Backlight IC discharge delay */
	int blic_discharging_delay_tft;
};

struct display_status {
	bool wait_disp_on;
	int wait_actual_disp_on;
	int disp_on_pre; /* set to 1 at first ss_panel_on_pre(). it  means that kernel initialized display */
	bool first_commit_disp_on;
};

struct hmt_info {
	bool is_support;		/* support hmt ? */
	bool hmt_on;	/* current hmt on/off state */

	int bl_level;
	int cmd_idx;
};

#define ESD_WORK_DELAY		1000 /* Every ESD delayed work should use same delay */

#define ESD_MASK_DEFAULT	0x00
#define ESD_MASK_PANEL_ON_OFF	BIT(0)
#define ESD_MASK_PANEL_LPM	BIT(1)
#define ESD_MASK_WORK		BIT(2)
#define ESD_MASK_GCT_TEST	BIT(3)
#define ESD_MASK_MAFPC_CRC	BIT(4)
#define ESD_MASK_REBOOT_CB	BIT(5)

struct esd_recovery {
	struct mutex esd_lock;
	bool esd_recovery_init;
	bool is_enabled_esd_recovery;
	bool is_wakeup_source;
	int esd_gpio[MAX_ESD_GPIO];
	u8 num_of_gpio;
	unsigned long irqflags[MAX_ESD_GPIO];
	u32 esd_mask;
	int (*esd_irq_enable)(bool enable, bool nosync, void *data, u32 esd_mask);
};

struct samsung_register_info {
	size_t virtual_addr;
};

struct samsung_register_dump_info {
	/* DSI PLL */
	struct samsung_register_info dsi_pll;

	/* DSI CTRL */
	struct samsung_register_info dsi_ctrl;

	/* DSI PHY */
	struct samsung_register_info dsi_phy;
};

/* Display Clock Tracer */
#define DCT_HASH_BITS	(8)
#define MAX_CALLFUNC_NUM	(16)
#define CALLFUNC_NUM_DISP	(6)

enum DCT_TAG {
	DCT_TAG_LINK_CLK = 0,
	DCT_TAG_MDP_CORE_CLK,
	DCT_TAG_RSCC_VSYNC_CLK,
	MAX_DCT_TAG,
};

enum DCT_ENABLE {
	DCT_DISABLE = 0,
	DCT_ENABLE = 1,
	DCT_EARLY_GATE = 2,
	MAX_DCT_ENABLE,
};

struct dct_info {
	struct hlist_node node;
	size_t *call_funcs;
	int count;
	u32 hash;
	atomic_t refcount;
	u64 time;
};

struct samsung_display_debug_data {
	struct dentry *root;
	struct dentry *dump;
	struct dentry *hw_info;
	struct dentry *display_status;
	struct dentry *display_ltp;

	bool print_all;
	bool print_cmds;
	bool print_vsync;
	bool print_frame;
	u32 frame_cnt;
	bool *is_factory_mode;
	bool panic_on_pptimeout;

	/* misc */
	struct miscdevice dev;
	bool report_once;

	bool need_recovery;

	/* Display Clock Tracer */
	DECLARE_HASHTABLE(dct_hashtable[MAX_DCT_TAG], DCT_HASH_BITS);
	struct clk *dct_clk[MAX_DCT_TAG];
	u32 init_enable_count[MAX_DCT_TAG];
	bool is_dcp_enabled;
	bool support_dct;
};

struct self_display {
	struct miscdevice dev;

	int is_support;
	int factory_support;
	int on;
	int file_open;
	int time_set;
	bool is_initialized;
	bool udc_mask_enable;

	struct self_time_info st_info;
	struct self_icon_info si_info;
	struct self_grid_info sg_info;
	struct self_analog_clk_info sa_info;
	struct self_digital_clk_info sd_info;
	struct self_partial_hlpm_scan sphs_info;

	struct mutex vdd_self_display_lock;
	struct mutex vdd_self_display_ioctl_lock;
	struct self_display_op operation[FLAG_SELF_DISP_MAX];

	struct ss_cmd_desc self_mask_cmd;
	struct ss_cmd_desc self_mask_crc_cmd;

	struct self_display_debug debug;

	bool mask_crc_force_pass;
	u8 mask_crc_pass_data[2];
	u8 mask_crc_read_data[2];
	int mask_crc_size;

	/* Self display Function */
	int (*init)(struct samsung_display_driver_data *vdd);
	int (*data_init)(struct samsung_display_driver_data *vdd);
	void (*reset_status)(struct samsung_display_driver_data *vdd);
	int (*aod_enter)(struct samsung_display_driver_data *vdd);
	int (*aod_exit)(struct samsung_display_driver_data *vdd);
	void (*self_mask_img_write)(struct samsung_display_driver_data *vdd);
	int (*self_mask_on)(struct samsung_display_driver_data *vdd, int enable);
	int (*self_mask_udc_on)(struct samsung_display_driver_data *vdd, int enable);
	int (*self_mask_check)(struct samsung_display_driver_data *vdd);
	void (*self_blinking_on)(struct samsung_display_driver_data *vdd, int enable);
	int (*self_display_debug)(struct samsung_display_driver_data *vdd);
	void (*self_move_set)(struct samsung_display_driver_data *vdd, int ctrl);
	int (*self_icon_set)(struct samsung_display_driver_data *vdd);
	int (*self_grid_set)(struct samsung_display_driver_data *vdd);
	int (*self_aclock_set)(struct samsung_display_driver_data *vdd);
	int (*self_dclock_set)(struct samsung_display_driver_data *vdd);
	int (*self_time_set)(struct samsung_display_driver_data *vdd, int from_self_move);
	int (*self_partial_hlpm_scan_set)(struct samsung_display_driver_data *vdd);
};

enum mdss_cpufreq_cluster {
	CPUFREQ_CLUSTER_BIG,
	CPUFREQ_CLUSTER_LITTLE,
	CPUFREQ_CLUSTER_ALL,
};

enum ss_panel_pwr_state {
	PANEL_PWR_OFF,
	PANEL_PWR_ON_READY,
	PANEL_PWR_ON,
	PANEL_PWR_LPM,
	PANEL_PWR_GCT,
	MAX_PANEL_PWR,
};

enum ss_test_mode_state {
	PANEL_TEST_NONE,
	PANEL_TEST_GCT,
	MAX_PANEL_TEST_MODE,
};

/*  COMMON_DISPLAY_NDX
 *   - ss_display ndx for common data such as debugging info
 *    which do not need to be handled separately by the panels
 *  PRIMARY_DISPLAY_NDX
 *   - ss_display ndx for data only for primary display
 *  SECONDARY_DISPLAY_NDX
 *   - ss_display ndx for data only for secondary display
 */
enum ss_display_ndx {
	COMMON_DISPLAY_NDX = 0,
	PRIMARY_DISPLAY_NDX = 0,
	SECONDARY_DISPLAY_NDX,
	MAX_DISPLAY_NDX,
};

#define GCT_RES_CHECKSUM_PASS	(1)
#define GCT_RES_CHECKSUM_NG	(0)
#define GCT_RES_CHECKSUM_OFF	(-2)
#define GCT_RES_CHECKSUM_NOT_SUPPORT	(-3)

struct gram_checksum_test {
	bool is_support;
	bool is_running;
	int on;
	u8 checksum[4];
	u8 valid_checksum[4];
};

struct temperature_table {
	int size;
	int *val;
};

struct enter_hbm_ce_table {
	int *idx;
	int *lux;
	int size;
};

struct mdnie_info {
	int support_mdnie;
	int support_trans_dimming;
	int disable_trans_dimming;

	int enter_hbm_ce_lux;	// to apply HBM CE mode in mDNIe
	struct enter_hbm_ce_table hbm_ce_table;

	bool tuning_enable_tft;

	int lcd_on_notifiy;

	int mdnie_x;
	int mdnie_y;

	int mdnie_tune_size[MDNIE_TUNE_MAX_SIZE];

	bool aolce_on;

	struct mdnie_lite_tun_type *mdnie_tune_state_dsi;
	struct mdnie_lite_tune_data *mdnie_data;
};

struct otp_info {
	u8 addr;
	u8 val;
	u16 offset;
};

struct brightness_info {
	int elvss_interpolation_temperature;

	int bl_idx;
	int prev_bl_level;
	int bl_level;		// brightness level via backlight dev
	int max_bl_level;
	int cd_level;
	int gm2_wrdisbv;	/* Gamma mode2 WRDISBV Write Display Brightness */
	int gamma_mode2_support;

	/* Smooth Dimming */
	bool ap_driven_smooth_dim;
	int smooth_dim_ap_frame_cnt; /* AP driven smooth dimming */
	int smooth_dim_ddi_frame_cnt; /* DDI driven smooth dimming */
	bool smooth_dim_off;
	bool smooth_br_done_work_in_progress;

	/* SAMSUNG_FINGERPRINT */
	int finger_mask_bl_level;
	int finger_mask_hbm_on;

	u8 elvss_value[2];	// elvss otp value, deprecated...  use otp
	u8 *irc_otp;		// irc otp value, deprecated...   use otp

	int aor_data;

	/* To enhance color accuracy, change IRC mode for each screen mode.
	 * - Adaptive display mode: Moderato mode
	 * - Other screen modes: Flat Gamma mode
	 */
	enum IRC_MODE irc_mode;
	int support_irc;

	struct workqueue_struct *br_wq;
	struct work_struct br_work;
};

struct ub_con_detect {
	spinlock_t irq_lock;
	int gpio;
	unsigned long irqflag;
	bool enabled;
	int ub_con_cnt;
	bool ub_con_ignore_user;
	int (*ub_con_det_irq_enable)(struct samsung_display_driver_data *vdd, bool enable);
	/* TBR: why it should check current_wakeup_context_gpio_status ...??? */
	//int current_wakeup_context_gpio_status;
};

struct motto_data {
	bool init_backup;
	u32 motto_swing;
	u32 vreg_ctrl_0;
	u32 hstx_init;
	u32 motto_emphasis;
	u32 cal_sel_init;
	u32 cmn_ctrl2_init;
	u32 cal_sel_curr; /* current value */
	u32 cmn_ctrl2_curr;
};

enum CHECK_SUPPORT_MODE {
	CHECK_SUPPORT_MCD,
	CHECK_SUPPORT_GRAYSPOT,
	CHECK_SUPPORT_MST,
	CHECK_SUPPORT_XTALK,
	CHECK_SUPPORT_HMD,
	CHECK_SUPPORT_GCT,
	CHECK_SUPPORT_BRIGHTDOT,
};

enum ddi_test_mode_list {
	DDI_TEST_DSC_CRC,
	DDI_TEST_ECC,
	DDI_TEST_SSR,
	DDI_TEST_FLASH_TEST,
	DDI_TEST_MAX,
};

struct ddi_test_mode {
	u8 *pass_val;
	int pass_val_size;
};

int samsung_panel_initialize(char *boot_str, unsigned int display_type);
void PBA_BOOTING_FHD_init(struct samsung_display_driver_data *vdd);
void PBA_BOOTING_FHD_DSI1_init(struct samsung_display_driver_data *vdd);
void PA1_S6E3FAE_AMB616FL03_FHD_init(struct samsung_display_driver_data *vdd);
void PA1_S6E3FAE_AMB616FL03_VHM_FHD_init(struct samsung_display_driver_data *vdd);
void PA2_S6E3HAF_AMB666FM03_WQHD_init(struct samsung_display_driver_data *vdd);
void PA2_S6E3HAF_AMB666FM01_WQHD_init(struct samsung_display_driver_data *vdd);
void PA2_S6E3HAF_AMB666FM03_VHM_WQHD_init(struct samsung_display_driver_data *vdd);
void PA3_S6E3HAF_AMB679FN01_WQHD_init(struct samsung_display_driver_data *vdd);
void PA3_S6E3HAF_AMB686HX01_WQHD_init(struct samsung_display_driver_data *vdd);
void PA3_S6E3HAF_AMB686HX01_VHM_WQHD_init(struct samsung_display_driver_data *vdd);
void A36_ANA6710_AMS663FS02_FHD_init(struct samsung_display_driver_data *vdd);
void A36_S6E3FC5_AMS663FS01_FHD_init(struct samsung_display_driver_data *vdd);
void GTA5P_HX83123A_TL101VVMS01_WUXGA_init(struct samsung_display_driver_data *vdd);
void GTA5P_NT31536E_TL101VVMS01_WUXGA_init(struct samsung_display_driver_data *vdd);
void Q7M_ANA38407_AMSA10FA01_WQXGA_init(struct samsung_display_driver_data *vdd);
void Q7M_S6E3FAE_AMB649GY01_HD_init(struct samsung_display_driver_data *vdd);
void XC7P_NT36672C_TL066FVMC21_FHD_init(struct samsung_display_driver_data *vdd);
void Q7_S6E3XA5_AMF800GX01_QXGA_init(struct samsung_display_driver_data *vdd);
void Q7_S6E3FAE_AMB649GY01_HD_init(struct samsung_display_driver_data *vdd);
void PS_S6E3HAF_AMB666LH01_WQHD_init(struct samsung_display_driver_data *vdd);
void PS_S6E3HAF_AMB666LH01_VHM_WQHD_init(struct samsung_display_driver_data *vdd);
void XC7P_NT36672C_TL066FVMC2101_FHD_init(struct samsung_display_driver_data *vdd);

struct panel_func {
	/* ON/OFF */
	int (*samsung_panel_on_pre)(struct samsung_display_driver_data *vdd);
	int (*samsung_panel_on_post)(struct samsung_display_driver_data *vdd);
	int (*samsung_display_on_post)(struct samsung_display_driver_data *vdd);
	int (*samsung_panel_off_pre)(struct samsung_display_driver_data *vdd);
	int (*samsung_panel_off_post)(struct samsung_display_driver_data *vdd);
	void (*samsung_panel_init)(struct samsung_display_driver_data *vdd);
	void (*mipi_lane_setting)(struct samsung_display_driver_data *vdd);

	/* DDI RX */
	char (*samsung_panel_revision)(struct samsung_display_driver_data *vdd);
	int (*samsung_module_info_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_manufacture_date_read)(struct samsung_display_driver_data *vdd);
	bool (*samsung_ddi_id_read)(struct samsung_display_driver_data *vdd);

	int (*samsung_cell_id_read)(struct samsung_display_driver_data *vdd);
	bool (*samsung_octa_id_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_hbm_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_elvss_read)(struct samsung_display_driver_data *vdd); // deprecated, use samsung_otp_read
	int (*samsung_irc_read)(struct samsung_display_driver_data *vdd); // deprecated, use samsung_otp_read
	int (*samsung_otp_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_mdnie_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_cmd_log_read)(struct samsung_display_driver_data *vdd);

	int (*get_hbm_candela_value)(int level);

	int (*pre_brightness)(struct samsung_display_driver_data *vdd);
	int (*pre_hmt_brightness)(struct samsung_display_driver_data *vdd);
	int (*pre_lpm_brightness)(struct samsung_display_driver_data *vdd);

	/* TFT */
	int (*samsung_buck_control)(struct samsung_display_driver_data *vdd);
	int (*samsung_blic_control)(struct samsung_display_driver_data *vdd);
	void (*samsung_tft_blic_init)(struct samsung_display_driver_data *vdd);
	void (*samsung_brightness_tft_pwm)(struct samsung_display_driver_data *vdd, int level);
	struct dsi_panel_cmd_set * (*samsung_brightness_tft_pwm_ldi)(struct samsung_display_driver_data *vdd, int *level_key);

	void (*samsung_bl_ic_pwm_en)(int enable);
	void (*samsung_bl_ic_i2c_ctrl)(int scaled_level);
	void (*samsung_bl_ic_outdoor)(int enable);

	/*LVDS*/
	void (*samsung_ql_lvds_register_set)(struct samsung_display_driver_data *vdd);
	int (*samsung_lvds_write_reg)(u16 addr, u32 data);

	/* A3 line panel data parsing fn */
	int (*parsing_otherline_pdata)(struct file *f, struct samsung_display_driver_data *vdd,
		char *src, int len);
	void (*set_panel_fab_type)(int type);
	int (*get_panel_fab_type)(void);

	/* color weakness */
	void (*color_weakness_ccb_on_off)(struct samsung_display_driver_data *vdd, int mode);

	/* DDI H/W Cursor */
	int (*ddi_hw_cursor)(struct samsung_display_driver_data *vdd, int *input);

	/* ECC read */
	int (*ecc_read)(struct samsung_display_driver_data *vdd);

	/* SSR read */
	int (*ssr_read)(struct samsung_display_driver_data *vdd);

	/* Gram Checksum Special Test */
	int (*samsung_gct_read)(struct samsung_display_driver_data *vdd);
	int (*samsung_gct_write)(struct samsung_display_driver_data *vdd);

	/* CCD */
	int (*samsung_ccd_read)(struct samsung_display_driver_data *vdd, char *buf);

	/* DSC_CRC */
	int (*samsung_dsc_crc_read)(struct samsung_display_driver_data *vdd, char *buf);

	/* GraySpot Test */
	void (*samsung_gray_spot)(struct samsung_display_driver_data *vdd, int enable);

	/* MCD Test */
	void (*samsung_mcd_pre)(struct samsung_display_driver_data *vdd, int enable);
	void (*samsung_mcd_post)(struct samsung_display_driver_data *vdd, int enable);

	/* DDI Flash Test */
	int (*ddi_flash_test)(struct samsung_display_driver_data *vdd, char* buf);
	int (*check_flash_done)(struct samsung_display_driver_data *vdd);

	/* print result of gamma comp */
	void (*samsung_print_gamma_comp)(struct samsung_display_driver_data *vdd);

	/* Gamma mode2 gamma compensation (for 48/96hz VRR mode) */
	int (*samsung_gm2_gamma_comp_init)(struct samsung_display_driver_data *vdd);

	/* Read UDC datga */
	int (*read_udc_data)(struct samsung_display_driver_data *vdd);

	/* This has been applied temporarily for sensors and will be removed later.*/
	void (*get_aor_data)(struct samsung_display_driver_data *vdd);

	/* gamma_flash : interpolation function */
	/* Below formula could be changed for each panel */
	int (*get_gamma_V_size)(void);
	void (*convert_GAMMA_to_V)(unsigned char *src, unsigned int *dst);
	void (*convert_V_to_GAMMA)(unsigned int *src, unsigned char *dst);

	int (*samsung_lfd_get_base_val)(struct vrr_info *vrr,
			enum LFD_SCOPE_ID scope, struct lfd_base_str *lfd_base);
	int (*post_vrr)(struct samsung_display_driver_data *vdd,
			int old_rr, bool old_hs, bool old_phs,
			int new_rr, bool new_hs, bool new_phs);

	/* check current mode (VRR, DMS, or etc..) to support tests (MCD, GCT, or etc..) */
	bool (*samsung_check_support_mode)(struct samsung_display_driver_data *vdd,
			enum CHECK_SUPPORT_MODE mode);

	/*
		For video panel :
		dfps operation related panel update func.
	*/
	void (*samsung_dfps_panel_update)(struct samsung_display_driver_data *vdd, int fps);

	/* Dynamic MIPI Clock */
	int (*samsung_dyn_mipi_pre)(struct samsung_display_driver_data *vdd);
	int (*samsung_dyn_mipi_post)(struct samsung_display_driver_data *vdd);

	int (*samsung_timing_switch_pre)(struct samsung_display_driver_data *vdd);
	int (*samsung_timing_switch_post)(struct samsung_display_driver_data *vdd);

	void (*read_flash)(struct samsung_display_driver_data *vdd, u32 addr, u32 size, u8 *buf);

	bool (*analog_offset_on)(struct samsung_display_driver_data *vdd);

	void (*pre_mdnie)(struct samsung_display_driver_data *vdd, struct dsi_cmd_desc *tune_data);
	void (*set_vividness)(struct samsung_display_driver_data *vdd, struct mdnie_lite_tun_type *tune, struct dsi_cmd_desc *tune_data);
};

enum SS_VBIAS_MODE {
	VBIAS_MODE_NS_WARM = 0,
	VBIAS_MODE_NS_COOL,
	VBIAS_MODE_NS_COLD,
	VBIAS_MODE_HS_WARM,
	VBIAS_MODE_HS_COOL,
	VBIAS_MODE_HS_COLD,
	VBIAS_MODE_MAX
};

enum PHY_STRENGTH_SETTING {
	SS_PHY_CMN_VREG_CTRL_0 = 0,
	SS_PHY_CMN_CTRL_2,
	SS_PHY_CMN_GLBL_RESCODE_OFFSET_TOP_CTRL,
	SS_PHY_CMN_GLBL_RESCODE_OFFSET_BOT_CTRL,
	SS_PHY_CMN_GLBL_RESCODE_OFFSET_MID_CTRL,
	SS_PHY_CMN_GLBL_STR_SWI_CAL_SEL_CTRL,
	SS_PHY_CMN_MAX,
};

/* GM2 DDI FLASH DATA TABLE */
struct gm2_flash_table {
	int idx;
	bool init_done;

	u8 *raw_buf;
	int buf_len;

	int start_addr;
	int end_addr;
};

/* DDI flash buffer for HOP display */
struct flash_gm2 {
	bool flash_gm2_init_done;

	u8 *ddi_flash_raw_buf;
	int ddi_flash_raw_len;
	int ddi_flash_start_addr;

	int off_ns_temp_warm;	/* NS mode, T > 0 */
	int off_ns_temp_cool;	/* NS mode, -15 < T <= 0 */
	int off_ns_temp_cold;	/* NS mode, T <= -15 */
	int off_hs_temp_warm;	/* HS mode, T > 0 */
	int off_hs_temp_cool;	/* HS mode, -15 < T <= 0 */
	int off_hs_temp_cold;	/* HS mode, T <= -15 */

	int off_gm2_flash_checksum;

	u16 checksum_tot_flash;
	u16 checksum_tot_cal;
	u16 checksum_one_mode_flash;
	u16 checksum_one_mode_mtp;
	int is_flash_checksum_ok;
};

struct mtp_gm2_info  {
	bool mtp_gm2_init_done;
	u8 *gamma_org; /* original MTP gamma value */
	u8 *gamma_comp; /* compensated gamma value (for 48/96hz mode) */
};

#define MAX_NIGHT_DIM_IDX 3
struct ss_brightness_info {
	bool allow_br_update;
	int temperature;
	int lux;			// current lux via sysfs
	int acl_status;
	bool is_hbm;
	bool last_br_is_hbm;
	ktime_t last_tx_time;

	/* TODO: rm below flags.. instead, use the other exist values...
	 * after move init seq. (read ddi id, hbm, etc..) to probe timing,
	 * like gamma flash.
	 */
	int hbm_loaded_dsi;
	int elvss_loaded_dsi;
	int irc_loaded_dsi;

	/* graudal acl: config_adaptiveControlValues in config_display.xml
	 * has step values, like 0, 1, 2, 3, 4, and 5.
	 * In case of gallery app, PMS sets <5 ~ 0> via adaptive_control sysfs.
	 * gradual_acl_val has the value, 0 ~ 5, means acl percentage step,
	 * then ss_acl_on() will set appropriate acl cmd value.
	 */
	int gradual_acl_val;

	/* TFT BL DCS Scaled level*/
	int scaled_level;

	/* TFT LCD Features*/
	int (*backlight_tft_config)(struct samsung_display_driver_data *vdd, int enable);
	void (*backlight_tft_pwm_control)(struct samsung_display_driver_data *vdd, int bl_lvl);

	struct candela_map_table candela_map_table[CD_MAP_TABLE_MAX][SUPPORT_PANEL_REVISION];

	int hbm_plus_level;

	struct cmd_legoop_map glut_offset_48hs;
	struct cmd_legoop_map glut_offset_60hs;
	struct cmd_legoop_map glut_offset_96hs;
	struct cmd_legoop_map glut_offset_120hs;
	struct cmd_legoop_map glut_offset_night_dim[MAX_NIGHT_DIM_IDX];
	struct cmd_legoop_map glut_offset_night_dim_96hs;
	struct cmd_legoop_map glut_offset_night_dim_120hs;
	int glut_offset_size;
	bool glut_00_val;
	bool glut_skip;

	struct cmd_legoop_map analog_offset_48hs[SUPPORT_PANEL_REVISION];
	struct cmd_legoop_map analog_offset_60hs[SUPPORT_PANEL_REVISION];
	struct cmd_legoop_map analog_offset_96hs[SUPPORT_PANEL_REVISION];
	struct cmd_legoop_map analog_offset_120hs[SUPPORT_PANEL_REVISION];
	int analog_offset_size;

	struct cmd_legoop_map manual_aor_120hs[SUPPORT_PANEL_REVISION];
	struct cmd_legoop_map manual_aor_96hs[SUPPORT_PANEL_REVISION];
	struct cmd_legoop_map manual_aor_60hs[SUPPORT_PANEL_REVISION];
	struct cmd_legoop_map manual_aor_48hs[SUPPORT_PANEL_REVISION];
	struct cmd_legoop_map acl_offset_map_table[SUPPORT_PANEL_REVISION];
	struct cmd_legoop_map irc_offset_map_table[SUPPORT_PANEL_REVISION];

	/*
	 *  AOR & IRC Interpolation feature
	 */

	int normal_brightness_step;
	int hmd_brightness_step;

	/*
	 * Common brightness table
	 */
	struct brightness_info common_br;

	/*
	 * Flash gamma feature
	 */

	int gamma_size;
	int aor_size;
	int vint_size;
	int elvss_size;
	int irc_size;
	int dbv_size;

	/* GM2 flash */
	int gm2_flash_tbl_cnt;
	int gm2_flash_checksum_cal;
	int gm2_flash_checksum_raw;
	int gm2_flash_write_check;
	struct gm2_flash_table *gm2_flash_tbl;

	/* DDI flash buffer for HOP display */
	struct flash_gm2 gm2_table;
	/* used for gamma compensation for 48/96hz VRR mode in gamma mode2 */
	struct mtp_gm2_info gm2_mtp;

	int force_use_table_for_hmd_done;

	bool no_brightness;
};

struct vrr_bridge_rr_tbl {
	int tot_steps;
	int *fps;

	int fps_base;
	int fps_start; /* used for gamma/aor interpolation during bridge RR */
	int fps_end; /* used for gamma/aor interpolation during bridge RR */
	int vbp_base;
	int vactive_base;
	int vfp_base;
	bool sot_hs_base;

	/* allowed brightness [candela]
	 * ex) HAB:
	 * 48NM/96NM BRR: 11~420nit
	 * 96HS/120HS BRR: 11~420nit
	 * 60HS/120HS BRR: 98~420nit
	 */
	int min_cd;
	int max_cd;

	/* In every BRR steps, it stays for delay_frame_cnt * TE_period
	 * ex) In HAB DDI,
	 * 48/60, 96/120, 60->120: 4 frames
	 * 120->60: 8 frames
	 */
	int delay_frame_cnt;
};

#define GET_LFD_INT_PART(rr, div) (rr * 10 / (div ? div : 1) / 10) /* Get Integer Part of LFD hz */
#define GET_LFD_DEC_PART(rr, div) (rr * 10 / (div ? div : 1) % 10) /* Get Decimal Part of LFD hz */

enum VRR_LFD_FAC_MODE {
	VRR_LFD_FAC_LFDON = 0,	/* dynamic 120hz <-> 10hz in HS mode, or 60hz <-> 30hz in NS mode */
	VRR_LFD_FAC_HIGH = 1,	/* always 120hz in HS mode, or 60hz in NS mode */
	VRR_LFD_FAC_LOW = 2,	/* set LFD min and max freq. to lowest value (HS: 10hz, NS: 30hz).
				 * In image update case, freq. will reach to 120hz, but it will
				 * ignore bridge step to enter LFD mode (ignore frame insertion setting)
				 */
	VRR_LFD_FAC_MAX,
};

enum VRR_LFD_AOD_MODE {
	VRR_LFD_AOD_LFDON = 0,	/* image update case: 30hz, LFD case: 1hz */
	VRR_LFD_AOD_LFDOFF = 1,	/* image update case: 30hz, LFD case: 30hz */
	VRR_LFD_AOD_MAX,
};

enum VRR_LFD_TSPLPM_MODE {
	VRR_LFD_TSPLPM_LFDON = 0,
	VRR_LFD_TSPLPM_LFDOFF = 1,
	VRR_LFD_TSPLPM_MAX,
};

enum VRR_LFD_INPUT_MODE {
	VRR_LFD_INPUT_LFDON = 0,
	VRR_LFD_INPUT_LFDOFF = 1,
	VRR_LFD_INPUT_MAX,
};

enum VRR_LFD_DISP_MODE {
	VRR_LFD_DISP_LFDON = 0,
	VRR_LFD_DISP_LFDOFF = 1, /* low brightness case */
	VRR_LFD_DISP_HBM_1HZ = 2,
	VRR_LFD_DISP_MAX,
};

enum LFD_CLIENT_ID {
	LFD_CLIENT_FAC = 0,
	LFD_CLIENT_DISP,
	LFD_CLIENT_INPUT,
	LFD_CLIENT_AOD,
	LFD_CLIENT_VID,
	LFD_CLIENT_HMD,
	LFD_CLIENT_MAX
};

enum LFD_SCOPE_ID {
	LFD_SCOPE_NORMAL = 0,
	LFD_SCOPE_LPM,
	LFD_SCOPE_HMD,
	LFD_SCOPE_MAX,
};

enum LFD_FUNC_FIX {
	LFD_FUNC_FIX_OFF = 0,
	LFD_FUNC_FIX_HIGH = 1,
	LFD_FUNC_FIX_LOW = 2,
	LFD_FUNC_FIX_LFD = 3,
	LFD_FUNC_FIX_HIGHDOT = 4,
	LFD_FUNC_FIX_MAX
};

enum LFD_FUNC_SCALABILITY {
	LFD_FUNC_SCALABILITY0 = 0,	/* 40lux ~ 7400lux, defealt, div=11 */
	LFD_FUNC_SCALABILITY1 = 1,	/* under 40lux, LFD off, div=1 */
	LFD_FUNC_SCALABILITY2 = 2,	/* touch press/release, div=2 */
	LFD_FUNC_SCALABILITY3 = 3,	/* TBD, div=4 */
	LFD_FUNC_SCALABILITY4 = 4,	/* TBD, div=5 */
	LFD_FUNC_SCALABILITY5 = 5,	/* 40lux ~ 7400lux, same set with LFD_FUNC_SCALABILITY0, div=11 */
	LFD_FUNC_SCALABILITY6 = 6,	/* over 7400lux (HBM), div=120 */
	LFD_FUNC_SCALABILITY_MAX
};

#define LFD_FUNC_MIN_CLEAR	0
#define LFD_FUNC_MAX_CLEAR	0

struct lfd_base_str {
	u32 max_div_def;
	u32 min_div_def;
	u32 min_div_lowest;
	u32 fix_div_def;
	u32 highdot_div_def;	/* For LFD_FUNC_FIX_HIGHDOT */
	u32 fix_low_div_def;	/* For LFD_FUNC_FIX_LOW */
	int base_rr;
};

struct lfd_mngr {
	enum LFD_FUNC_FIX fix[LFD_SCOPE_MAX];
	enum LFD_FUNC_SCALABILITY scalability[LFD_SCOPE_MAX];
	u32 min[LFD_SCOPE_MAX];
	u32 max[LFD_SCOPE_MAX];
};

struct lfd_info {
	struct lfd_mngr lfd_mngr[LFD_CLIENT_MAX];

	struct notifier_block nb_lfd_touch;
	struct workqueue_struct *lfd_touch_wq;
	struct work_struct lfd_touch_work;

	bool support_lfd; /* Low Frequency Driving mode for HOP display */
	bool running_lfd;

	u32 min_div; /* max_div_def ~ min_div_def, or min_div_lowest */
	u32 max_div; /* max_div_def ~ min_div_def */
	u32 base_rr;
};

struct lfd_div_info {
	u32 min_div;
	u32 max_div;
	u32 fix;
};

struct vrr_info {
	/* LFD information and mangager */
	struct lfd_info lfd;

	/* ss_panel_vrr_switch() is main VRR function.
	 * It can be called in multiple threads, display thread
	 * and kworker thread.
	 * To guarantee exclusive VRR processing, use vrr_lock.
	 */
	struct mutex vrr_lock;

	/* - samsung,support_vrr_based_bl : Samsung concept.
	 *   Change VRR mode in brightness update due to brithness compensation for VRR change.
	 * - No samsung,support_vrr_based_bl : Qcomm original concept.
	 *   Change VRR mode in dsi_panel_switch() by sending DSI_CMD_SET_TIMING_SWITCH command.
	 */
	bool support_vrr_based_bl;

	/* MDP set SDE_RSC_CMD_STATE for inter-frame power collapse, which refer to TE.
	 *
	 * In case of SS VRR change (120HS -> 60HS), it changes VRR mode
	 * in other work thread, not it commit thread.
	 * In result, MDP works at 60hz, while panel is working at 120hz yet, for a while.
	 * It causes higher TE period than MDP expected, so it lost wr_ptr irq,
	 * and causes delayed pp_done irq and frame drop.
	 *
	 * To prevent this
	 * During VRR change, set RSC fps to maximum 120hz, to wake up RSC most frequently,
	 * not to lost wr_ptr irq.
	 * After finish VRR change, recover original RSC fps, and rsc timeout value.
	 */
	bool keep_max_rsc_fps;

	bool keep_max_clk;

	bool running_vrr_mdp; /* MDP get VRR request ~ VRR work start */
	bool running_vrr; /* VRR work start ~ end */

	int check_vsync;

	/* QCT use DMS instead of VRR feature for command mdoe panel.
	 * is_vrr_changing flag shows it is VRR chaging mode while DMS flag is set,
	 * and be used for samsung display driver.
	 */
	bool is_vrr_changing;
	bool is_multi_resolution_changing;

	u32 cur_h_active;
	u32 cur_v_active;
	/*
	 * - cur_refresh_rate should be set right before brightness setting (ss_brightness_dcs)
	 * - cur_refresh_rate could not be changed during brightness setting (ss_brightness_dcs)
	 *   Best way is that guarantees to keep cur_refresh_rate during brightness setting,
	 *   cur_refresh_rate is changed in ss_panel_vrr_switch(), and it sets brightness setting
	 *   with final target refresh rate, so there is no problem.
	 */
	int cur_refresh_rate;
	bool cur_sot_hs_mode;	/* Scan of Time HS mode for VRR */
	bool cur_phs_mode;		/* pHS mode for seamless */

	int prev_refresh_rate;
	bool prev_sot_hs_mode;
	bool prev_phs_mode;

	u32 adjusted_h_active;
	u32 adjusted_v_active;
	int adjusted_refresh_rate;
	bool adjusted_sot_hs_mode;	/* Scan of Time HS mode for VRR */
	bool adjusted_phs_mode;		/* pHS mode for seamless */

	struct workqueue_struct *vrr_workqueue;
	struct work_struct vrr_work;

	bool param_update;

	bool need_vrr_update;
};

struct dvrr_info {

	/*
	 * Dynamic(Discrete) VRR : FPS Change wihtout mode Switching
	 */
	bool support_dvrr;

	struct mutex dvrr_lock;

	u32 cur_frame_interval;
	u32 prev_frame_interval;

	bool dvrr_update;
};

enum PANEL_POWERS {
	PANEL_POWERS_ON_PRE_LP11 = 0,
	PANEL_POWERS_ON_POST_LP11,
	PANEL_POWERS_OFF_PRE_LP11,
	PANEL_POWERS_OFF_POST_LP11,
	PANEL_POWERS_LPM_ON,
	PANEL_POWERS_LPM_OFF,
	PANEL_POWERS_ON_MIDDLE_OF_LP_HS_CLK,
	PANEL_POWERS_OFF_MIDDLE_OF_LP_HS_CLK,
	PANEL_POWERS_AOLCE_ON,
	PANEL_POWERS_AOLCE_OFF,
	PANEL_POWERS_EVLDD_ELVSS,
	MAX_PANEL_POWERS,
};

enum panel_pwr_type {
	PANEL_PWR_PMIC = 0,
	PANEL_PWR_PMIC_SPECIFIC, /* has different seq than usual (ex. enable -> voltage set) */
	PANEL_PWR_GPIO,
	PANEL_PWR_GPIO_SPECIFIC,
	PANEL_PWR_PANEL_SPECIFIC,
	PANEL_PWR_MAX,
};

struct panel_pwr_seq {
	int onoff;
	int post_delay;
};

struct pwr_node {
	enum panel_pwr_type type;
	char *name;

	struct panel_pwr_seq *pwr_seq;
	int pwr_seq_count;

	/* for pmic type */
	struct regulator *reg;
	int voltage; // max_voltage
	int load;
	bool use_shared;

	/* for ssd type */
	int current_uA;

	/* for gpio type */
	int gpio;
};

struct panel_power {
	struct pwr_node *pwrs;
	int pwr_count;

	int (*parse_cb)(struct samsung_display_driver_data *vdd, struct pwr_node *pwr,
			struct device_node *np);
	int (*ctrl_cb)(struct samsung_display_driver_data *vdd, struct pwr_node *pwr, bool enable);
};

/* Under Display Camera */
struct UDC {
	u32 start_addr;
	int size;
	u8 *data;
	bool read_done;
};

#define MAX_DELAY_NUM	(8)

struct seq_delay {
	int delay[MAX_DELAY_NUM];
	int update_count;
	bool update; /* if true, update delay */
};

struct resolution_info {
	char name[16];
	int width;
	int height;
};

enum tear_partial_bottom_state {
	TEAR_PARTIAL_BOTTOM_REL = 0,
	TEAR_PARTIAL_BOTTOM_SET,
	TEAR_PARTIAL_BOTTOM_READY,
};

#define MAX_TS_BUF_CNT	(60)
#define MAX_CMD_EVTLOG_NUM	(1000)
#define MAX_CMD_EVTLOG_CMDS_NUM	(30)

struct cmd_evtlog {
	ktime_t time;
	u8 cmds[MAX_CMD_EVTLOG_CMDS_NUM];
	size_t cmd_len;
};

struct dbg_tear_info {
	struct workqueue_struct *dbg_tear_wq;
	struct delayed_work dbg_tear_work;
	bool is_no_te;
	bool force_panic_no_te;
	bool dump_msg_log;

	ktime_t ts_frame_start[MAX_TS_BUF_CNT];
	ktime_t ts_frame_end[MAX_TS_BUF_CNT];
	ktime_t ts_frame_te[MAX_TS_BUF_CNT];
	ktime_t ts_frame_commit[MAX_TS_BUF_CNT];
	int pos_frame_start;
	int pos_frame_end;
	int pos_frame_te;
	int pos_frame_commit;

	int early_te_delay_us;
	enum tear_partial_bottom_state tear_partial_bottom;

	struct cmd_evtlog evtlog[MAX_CMD_EVTLOG_NUM];
	int pos_evtlog;
};

#define MAX_DISABLE_VRR_MODES_COUNT	(20)
struct disable_vrr_modes_str {
	int fps;
	bool hs;
	bool phs;
};

struct cmd_ref_state {
	int cur_refresh_rate;
	bool sot_hs;
	bool sot_phs;
	int prev_refresh_rate;
	bool prev_sot_hs;
	bool prev_sot_phs;
	bool running_vrr;
	bool running_lfd;
	bool need_vrr_update;
	bool lpm_ongoing;
	bool hmt_on;
	bool is_hbm;
	bool is_hbm_plus;
	int lfd_max_fps; /* It is expressed by x10 to support decimal points */
	int lfd_min_fps; /* It is expressed by x10 to support decimal points */
	int lfd_fix;
	u64 clk_rate_MHz;
	int clk_idx;
	int osc_idx;
	int h_active;
	int v_active;
	enum IRC_MODE irc_mode;
	int gradual_acl_val;
	bool gradual_acl_changed;
	bool acl_on;
	bool smooth_dim_off;
	bool display_on;
	bool first_high_level;
	bool dia_off;
	bool night_dim;
	bool early_te;
	int bl_idx;
	int bl_level;
	int cd_level;
	int temperature;
	bool finger_mask_updated;
	int panel_revision;
	bool is_factory_mode;
	bool mafpc_en;
	bool local_hbm_using;

	/* DVRR Related */
	u32 cur_frame_interval;
	u32 prev_frame_interval;
	bool dvrr_update;
};

struct op_sym_cb_tbl {
	char *symbol;
	int (*cb)(struct samsung_display_driver_data *vdd,
			char *val, struct ss_cmd_desc *cmd);
	bool op_update; /* IF/ELSE/IFBLOCK: false, UPDATE: true */
};

/*
 * Manage vdd per dsi_panel, respectivley, like below table.
 * Each dsi_display has one dsi_panel and one vdd, respectively.
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * case		physical conneciton	panel dtsi	dsi_dsiplay	dsi_panel	vdd	dsi_ctrl	lcd sysfs	bl sysfs
 * ------------------------------------------------------------------------------------------------------------------------------------------
 * single dsi	1 panel, 1 dsi port	1		1		1		1	1		1		1
 * split mode	1 panel, 2 dsi port	1		1		1		1	2		1		1
 * dual panel	2 panel, 2 dsi port	2		2		2		2	2		2		1
 * hall ic	2 panel, 1 dsi port	2		2		2		2	1		1		1
 * ------------------------------------------------------------------------------------------------------------------------------------------
 */
#define MAX_OTP_LEN	(256)
#define MAX_OP_SYM_CB_SIZE	(256)

/* TBR: lots of status flags.. integrate flags: panel_dead, is_factory_mode, and etc.. */
struct samsung_display_driver_data {

	void *msm_private;

	char panel_name[MAX_CMDLINE_PARAM_LEN];

	/* mipi cmd type */
	int cmd_type;

	/* For Video Hybrid Mode(VHM) */
	bool esync_support;

	bool panel_dead;
	int read_panel_status_from_lk;
	bool is_factory_mode;
	bool is_factory_pretest; // to change PD(pretest)/NP(not pretest) config of UB_CON gpio
	bool panel_attach_status; // 1: attached, 0: detached
	int panel_revision;
	char *panel_vendor;

	/* SAMSUNG_FINGERPRINT */
	bool support_optical_fingerprint;
	bool finger_mask_updated;
	int finger_mask;
	int local_hbm_val;

	int panel_hbm_entry_delay; /* HBM entry delay after cmd tx, Unit = vsync */
	int panel_hbm_entry_after_vsync_pre; /* Delay after last VSYNC before cmd Tx, Unit = us */
	int panel_hbm_entry_after_vsync; /* Delay after last VSYNC after cmd Tx, Unit = us */
	int panel_hbm_exit_delay; /* HBM exit delay after cmd tx, Unit = vsync */
	int panel_hbm_exit_after_vsync_pre; /* Delay after last VSYNC before cmd Tx, Unit = us */
	int panel_hbm_exit_after_vsync; /* Delay after last VSYNC after cmd Tx, Unit = us */
	int panel_hbm_exit_frame_wait; /* Wait until last Fingermask Frame kickoff started */

	struct lcd_device *lcd_dev;

	struct display_status display_status_dsi;

	struct mutex vdd_lock;
	struct mutex cmd_lock;
	struct mutex bl_lock;
	struct mutex display_on_lock;

	struct samsung_display_debug_data *debug_data;
	struct list_head vdd_list;

	bool need_panel_recovery;
	bool during_panel_recovery;
	int mipi_fail_recovery_cnt;
	int mipi_fail_recovery_fail_cnt;

	atomic_t block_commit_cnt;
	struct wait_queue_head block_commit_wq;

	int siop_status;

	struct panel_func panel_func;

	bool ss_cmd_dsc_long_write; /* 39h instead of 29h */
	bool ss_cmd_dsc_short_write_param;  /* 15h instead of 05h */
	bool ss_cmd_always_last_command_set;

	// parsed data from dtsi
	struct samsung_display_dtsi_data dtsi_data;

	/*
	 *  Panel read data
	 */
	int manufacture_id_dsi;
	int module_info_loaded_dsi;
	int manufacture_date_loaded_dsi;

	int manufacture_date_dsi;
	int manufacture_time_dsi;

	int ddi_id_loaded_dsi;
	u8 *ddi_id_dsi;
	int ddi_id_len;

	int cell_id_loaded_dsi;	/* Panel Unique Cell ID */
	u8 *cell_id_dsi;		/* white coordinate + manufacture date */
	int cell_id_len;
	u8 *w_coordinate; /* white coordinate, panel unique mtp value */
	int w_coordinate_len;

	u8 *local_hbm_compensation_value;
	int local_hbm_compensation_len;

	int octa_id_loaded_dsi;		/* Panel Unique OCTA ID */
	u8 *octa_id_dsi;
	int octa_id_len;

	u8 last_rddpm;			/* indicate rddpm value before last display off */
	u8 last_rddsm;			/* indicate rddsm value before last display off */
	u16 last_esderr;			/* indicate esderr value before last display off */
	u8 last_dsierr;			/* indicate dsierr value before last display off */
	u16 last_protocol_err;	/* indicate protocol_err value before last display off */

	int select_panel_gpio;
	bool select_panel_use_expander_gpio;

	/* X-Talk */
	int xtalk_mode;
	bool xtalk_mode_on; /* used to skip brightness update */

	/* Reset Time check */
	ktime_t reset_time_64;

	/* Sleep_out cmd time check */
	ktime_t sleep_out_time;
	ktime_t tx_set_on_time;

	/* Some panel read operation should be called after on-command. */
	bool skip_read_on_pre;

	/* Support Global Para */
	bool gpara;
	bool pointing_gpara;
	/* if samsung,two_byte_gpara is true,
	 * that means DDI uses two_byte_gpara
	 * 06 01 00 00 00 00 01 DA 01 00 : use 1byte gpara, length is 10
	 * 06 01 00 00 00 00 01 DA 01 00 00 : use 2byte gpara, length is 11
	*/
	bool two_byte_gpara;

	/* Num_of interface get from priv_info->topology.num_intf */
	int num_of_intf;

	/* set_elvss */
	int set_factory_elvss;

	/*
	 *  panel pwr state
	 */
	enum ss_panel_pwr_state panel_state;

	/*
	 * ddi test_mode
	 */
	enum ss_test_mode_state test_mode_state;

	/* ndx means display ID
	 * ndx = 0: primary display
	 * ndx = 1: secondary display
	 */
	enum ss_display_ndx ndx;

	/*
	 *  register dump info
	 */
	struct samsung_register_dump_info dump_info[MAX_INTF_NUM];

	/*
	 *  FTF
	 */
	/* CABC feature */
	int support_cabc;

	/* LP RX timeout recovery */
	bool support_lp_rx_err_recovery;

	/*
	 *  ESD
	 */
	struct esd_recovery esd_recovery;

	/* FG_ERR */
	int fg_err_gpio;

	/* Panel ESD Recovery Count */
	int panel_recovery_cnt;

	/* lp_rx timeout Count */
	int lp_rx_fail_cnt;

	/*
	 *  Image dump
	 */
	struct workqueue_struct *image_dump_workqueue;
	struct work_struct image_dump_work;

	/*
	 *  Other line panel support
	 */
	struct workqueue_struct *other_line_panel_support_workq;
	struct delayed_work other_line_panel_support_work;
	int other_line_panel_work_cnt;

	/*
	 *  LPM
	 */
	struct lpm_info panel_lpm;

	/*
	 *	HMT
	 */
	struct hmt_info hmt;

	/*
	 *  Big Data
	 */
	struct notifier_block dpui_notif;
	struct notifier_block dpci_notif;
	u64 dsi_errors; /* report dpci bigdata */

	/*
	 *  COPR
	 */
	struct COPR copr;

	/*
	 *  Dynamic MIPI Clock
	 */
	struct dyn_mipi_clk dyn_mipi_clk;

	/*
	 *  GCT
	 */
	struct gram_checksum_test gct;

	/*
	 *  smmu debug(sde & rotator)
	 */
	struct ss_smmu_debug ss_debug_smmu[SMMU_MAX_DEBUG];
	struct kmem_cache *ss_debug_smmu_cache;

	/*
	 *  SELF DISPLAY
	 */
	struct self_display self_disp;

	/* MAFPC */
	struct MAFPC mafpc;
	struct ss_cmd_desc mafpc_crc_img_cmd;
	struct ss_cmd_desc mafpc_img_cmd;
	struct cmd_legoop_map mafpc_scale_table;

	/*
	 * Samsung brightness information for smart dimming
	 */
	struct ss_brightness_info br_info;

	/*
	 * mDNIe
	 */
	struct mdnie_info mdnie;
	int mdnie_loaded_dsi;

	/*
	 * grayspot test
	 */
	int grayspot;

	/*
	 * CCD fail value
	 */
	int ccd_pass_val;
	int ccd_fail_val;

	/* Pass val (test_mode) */
	struct ddi_test_mode ddi_test[DDI_TEST_MAX];
#if 1
	int *dsc_crc_pass_val;
	int dsc_crc_pass_val_size;
	int *ecc_pass_val;
	int ecc_pass_val_size;
	int *ssr_pass_val;
	int ssr_pass_val_size;
	int *flash_test_pass_val;
	int flash_test_pass_val_size;
#endif
	int samsung_splash_enabled;
	int cmd_set_on_splash_enabled;

	/* UB CON DETECT */
	struct ub_con_detect ub_con_det;

	/* VRR (Variable Refresh Rate) */
	struct vrr_info vrr;

	/* DVRR (Dynamic(Discrete) Variable Refresh Rate) */
	struct dvrr_info dvrr;

	/* Motto phy tune */
	struct device *motto_device;
	struct motto_data motto_info;

	/* wakeup source */
	struct wakeup_source *panel_wakeup_source;

	/* panel notifier work */
	struct workqueue_struct *notify_wq;
	struct work_struct bl_event_work;
	struct work_struct vrr_event_work;
	struct work_struct lfd_event_work;
	struct work_struct panel_state_event_work;
	struct work_struct test_mode_event_work;
	struct work_struct screen_mode_event_work;
	struct work_struct esd_event_work;

	/* ESD delayed work */
	struct delayed_work esd_enable_event_work;
	struct delayed_work esd_disable_event_work;

	/* SDE Clock work */
	struct delayed_work sde_max_clk_work;
	struct delayed_work sde_normal_clk_work;

	/* smooth_br work */
	struct delayed_work smooth_br_done_work;

	struct mutex notify_lock;
	bool is_autorefresh_fail;

	/* window color to make different br table */
	char window_color[2];
	bool support_window_color;

	struct panel_power panel_powers[MAX_PANEL_POWERS];

	/* need additional mipi support */
	bool mipi_header_modi;

	/* need to send brighgntess cmd blocking frame update */
	bool need_brightness_lock;
	bool block_frame_oneshot;
	bool no_qcom_pps;
	u8 qc_pps_cmd[89];

	/* Single TX  */
	bool not_support_single_tx;

	/* Some TDDI (Himax) panel can get esd noti from touch driver */
	bool esd_touch_notify;
	struct notifier_block nb_esd_touch;

	/* Reboot Callback (For recovery mode) */
	struct notifier_block nb_reboot;

	int check_fw_id; 	/* save ddi_fw id (revision)*/
	bool is_recovery_mode;

	/* phy strength setting bit field */
	DECLARE_BITMAP(ss_phy_ctrl_bit, SS_PHY_CMN_MAX);
	uint32_t ss_phy_ctrl_data[SS_PHY_CMN_MAX];

	/* Bloom5G need old style aor dimming : using both A-dimming & S-dimming */
	bool old_aor_dimming;

	/* BIT0: brightdot test, BIT1: brightdot test in LFD 0.5hz */
	u32 brightdot_state;

	/* Some panel has unstable TE period, and causes wr_ptr timeout panic
	 * in inter-frame RSC idle policy.
	 * W/A: select four frame RSC idle policy.
	 */
	bool rsc_4_frame_idle;

	/* Video mode vblank_irq time for fp hbm sync */
	ktime_t vblank_irq_time;
	struct wait_queue_head ss_vsync_wq;
	atomic_t ss_vsync_cnt;

	/* flag to support reading module id at probe timing */
	bool support_early_id_read;

	/* UDC data */
	struct UDC udc;

	/* flag that display_on (29h) is on/off */
	bool display_on;

	/* Delay ms afer display_on (29h) */
	int display_on_post_delay;

	/* mdp clock underflow */
	int cnt_mdp_clk_underflow;

	struct seq_delay on_delay;
	struct seq_delay off_delay;

	bool use_flash_done_recovery;

	bool otp_init_done;
	struct otp_info otp[MAX_OTP_LEN]; /* elvss, irc, or etc */
	int otp_len;

	struct resolution_info *res;
	int res_count;

	bool dia_off; /* default: dia_off = false == dia on */

	/* Add total level key feature.
	 * Skip level key commands in the middle of other commands to reduce
	 * duplicated commands transmission.
	 * Send level key only once at first.
	 */
	bool allow_level_key_once;

	/* For file test */
	char *f_buf;
	size_t f_size;
	char *h_buf;
	size_t h_size;
	bool file_loading;

	struct dbg_tear_info dbg_tear_info;

	struct temperature_table temp_table;

	int night_dim;
	int night_dim_max_lv;
	bool early_te;
	int check_early_te;

	int disable_vrr_modes_count;
	struct disable_vrr_modes_str disable_vrr_modes[MAX_DISABLE_VRR_MODES_COUNT];

	struct cmd_ref_state cmd_ref_state;

	struct op_sym_cb_tbl sym_cb_tbl[MAX_OP_SYM_CB_SIZE];
	int sym_cb_tbl_size;
	/* check for display on time */
	s64 display_on_time;

	bool panel_againg_status;
};

extern struct list_head vdds_list;

extern char *lfd_client_name[LFD_CLIENT_MAX];
extern char *lfd_scope_name[LFD_SCOPE_MAX];

#if IS_ENABLED(CONFIG_UML)
struct display_uml_bridge_msg {
	unsigned int dev_id;
	unsigned int data_len;
	struct rf_info *data;
};
#define DISPLAY_TEST_CP_CHANNEL_INFO 0x01
#undef NOTIFY_DONE
#undef NOTIFY_OK
#undef NOTIFY_STOP_MASK
#undef NOTIFY_BAD
#define NOTIFY_DONE		0x0000		/* Don't care */
#define NOTIFY_OK		0x0001		/* Suits me */
#define NOTIFY_STOP_MASK	0x8000		/* Don't call further */
#define NOTIFY_BAD		(NOTIFY_STOP_MASK|0x0002)
#endif

/* COMMON FUNCTION */
void ss_panel_init(struct dsi_panel *panel);
int parse_dt_data(struct samsung_display_driver_data *vdd, struct device_node *np,
		void *data, size_t size, char *cmd_string, char panel_rev,
		int (*fnc)(struct samsung_display_driver_data *, struct device_node *, void *, char *));
int ss_parse_panel_legoop_table(struct samsung_display_driver_data *vdd,
		struct device_node *np,	void *tbl, char *keystring);

/* VRR */
int ss_panel_dms_switch(struct samsung_display_driver_data *vdd);
void ss_panel_vrr_switch_work(struct work_struct *work);
int ss_get_lfd_div(struct samsung_display_driver_data *vdd, enum LFD_SCOPE_ID scope,
			struct lfd_div_info *div_info);
int ss_send_cmd(struct samsung_display_driver_data *vdd,
		int cmd);
int ss_write_ddi_ram(struct samsung_display_driver_data *vdd,
				int target, u8 *buffer, int len);
int ss_panel_on(struct samsung_display_driver_data *vdd);
int ss_panel_off_pre(struct samsung_display_driver_data *vdd);
int ss_panel_off_post(struct samsung_display_driver_data *vdd);

int ss_esync_on_post(struct samsung_display_driver_data *vdd);
int ss_esync_off_pre(struct samsung_display_driver_data *vdd);

int ss_backlight_tft_gpio_config(struct samsung_display_driver_data *vdd, int enable);
int ss_backlight_tft_request_gpios(struct samsung_display_driver_data *vdd);
void ss_panel_low_power_config(struct samsung_display_driver_data *vdd, int enable);

/*
 * Check lcd attached status for DISPLAY_1 or DISPLAY_2
 * if the lcd was not attached, the function will return 0
 */
int ss_panel_attached(int ndx);
int get_lcd_attached(char *mode);
int get_lcd_attached_secondary(char *mode);
int ss_panel_attached(int ndx);

struct samsung_display_driver_data *check_valid_ctrl(struct dsi_panel *panel);

char ss_panel_id0_get(struct samsung_display_driver_data *vdd);
char ss_panel_id1_get(struct samsung_display_driver_data *vdd);
char ss_panel_id2_get(struct samsung_display_driver_data *vdd);
char ss_panel_rev_get(struct samsung_display_driver_data *vdd);

int ss_panel_attach_get(struct samsung_display_driver_data *vdd);
int ss_panel_attach_set(struct samsung_display_driver_data *vdd, bool attach);

int ss_read_loading_detection(void);

int ss_convert_img_to_mass_cmds(struct samsung_display_driver_data *vdd,
		char *data, u32 data_size, struct ss_cmd_desc *ss_cmd);

/* EXTERN VARIABLE */
extern struct dsi_status_data *pstatus_data;

/* HMT FUNCTION */
int hmt_enable(struct samsung_display_driver_data *vdd);
int hmt_reverse_update(struct samsung_display_driver_data *vdd, bool enable);

 /* CORP CALC */
void ss_copr_calc_work(struct work_struct *work);
void ss_copr_calc_delayed_work(struct delayed_work *work);

void ss_panel_lpm_ctrl(struct samsung_display_driver_data *vdd, int enable);

/* Dynamic MIPI Clock FUNCTION */
int ss_change_dyn_mipi_clk_timing(struct samsung_display_driver_data *vdd);
int ss_dyn_mipi_clk_tx_ffc(struct samsung_display_driver_data *vdd);

void ss_read_mtp(struct samsung_display_driver_data *vdd, int addr, int len, int pos, u8 *buf);
int ss_read_mtp_sysfs(struct samsung_display_driver_data *vdd, int addr, int len, int pos, u8 *buf);
void ss_write_mtp(struct samsung_display_driver_data *vdd, int len, u8 *buf);

/* Other line panel support */
#define MAX_READ_LINE_SIZE 256
int read_line(char *src, char *buf, int *pos, int len);

/* UB_CON_DET */
enum {
	UB_CONNECTED = 1,
	UB_DISCONNECTED = 0,
	UB_UNKNOWN_CONNECTION = -1,
};

int ss_is_ub_connected(struct samsung_display_driver_data *vdd);
void ss_ub_con_notify(struct samsung_display_driver_data *vdd, bool connected);
void ss_send_ub_uevent(struct samsung_display_driver_data *vdd);

void ss_set_panel_state(struct samsung_display_driver_data *vdd, enum ss_panel_pwr_state panel_state);
void ss_set_test_mode_state(struct samsung_display_driver_data *vdd, enum ss_test_mode_state state);

/* Do dsi related works before first kickoff (dsi init in kernel side) */
int ss_early_display_init(struct samsung_display_driver_data *vdd);

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
int ss_notify_queue_work(struct samsung_display_driver_data *vdd,
	enum panel_notifier_event_t event);
#endif

void spsram_read_bytes(struct samsung_display_driver_data *vdd, int addr, int rsize, u8 *buf);

/***************************************************************************************************
*		BRIGHTNESS RELATED.
****************************************************************************************************/

#define DEFAULT_BRIGHTNESS 255

#define HBM_MODE 6

/* BRIGHTNESS RELATED FUNCTION */
int ss_brightness_dcs(struct samsung_display_driver_data *vdd, int level, int backlight_origin);
void ss_brightness_tft_pwm(struct samsung_display_driver_data *vdd, int level);
int set_br_values(struct samsung_display_driver_data *vdd, struct candela_map_table *table, int target_level);

bool is_hbm_level(struct samsung_display_driver_data *vdd);

/* SAMSUNG_FINGERPRINT */
void ss_send_hbm_fingermask_image_tx(struct samsung_display_driver_data *vdd, bool on);
void ss_wait_for_vsync(struct samsung_display_driver_data *vdd,
		int num_of_vsnc, int delay_after_vsync);

/* HMT BRIGHTNESS */
int ss_brightness_dcs_hmt(struct samsung_display_driver_data *vdd, int level);
int hmt_bright_update(struct samsung_display_driver_data *vdd);

/* TFT BL DCS RELATED FUNCTION */
int get_scaled_level(struct samsung_display_driver_data *vdd, int ndx);

void ss_set_vrr_switch(struct samsung_display_driver_data *vdd, bool param_update);

bool ss_is_sot_hs_from_drm_mode(const struct drm_display_mode *drm_mode);
bool ss_is_phs_from_drm_mode(const struct drm_display_mode *drm_mode);
int ss_vrr_apply_dsi_bridge_mode_fixup(struct dsi_display *display,
				struct drm_display_mode *cur_mode,
				struct dsi_display_mode cur_dsi_mode,
				struct dsi_display_mode *adj_mode);
int ss_vrr_save_dsi_bridge_mode_fixup(struct dsi_display *display,
				struct drm_display_mode *cur_mode,
				struct dsi_display_mode cur_dsi_mode,
				struct dsi_display_mode *adj_mode,
				struct drm_crtc_state *crtc_state);

int ss_panel_dvrr_switch(struct samsung_display_driver_data *vdd, u32 frame_interval);

void ss_event_frame_update_pre(struct samsung_display_driver_data *vdd);
void ss_event_frame_update_post(struct samsung_display_driver_data *vdd);
void ss_event_rd_ptr_irq(struct samsung_display_driver_data *vdd);

void ss_delay(s64 delay, ktime_t from);

void ss_wait_for_te_gpio(struct samsung_display_driver_data *vdd, int num_of_te, int delay_after_te, bool preemption);
void ss_panel_recovery(struct samsung_display_driver_data *vdd);

int ss_rf_info_notify_callback(struct notifier_block *nb,
				unsigned long size, void *data);

struct dsi_panel_cmd_set *ss_get_cmds(struct samsung_display_driver_data *vdd, int type);

int ss_frame_delay(int fps, int frame);
int ss_frame_to_ms(struct samsung_display_driver_data *vdd, int frame);

bool ss_is_tear_case(struct samsung_display_driver_data *vdd);
bool ss_is_no_te_case(struct samsung_display_driver_data *vdd);
//void ss_print_ss_cmd_set(struct samsung_display_driver_data *vdd, struct ss_cmd_set *set);

int ss_get_pending_kickoff_cnt(struct samsung_display_driver_data *vdd);
void ss_wait_for_kickoff_done(struct samsung_display_driver_data *vdd);

irqreturn_t ss_ub_con_det_handler(int irq, void *handle);
int ss_ub_con_det_irq_enable(struct samsung_display_driver_data *vdd, bool enable);

bool ss_ddi_id_read(struct samsung_display_driver_data *vdd);
bool ss_octa_id_read(struct samsung_display_driver_data *vdd);
void ss_block_commit(struct samsung_display_driver_data *vdd);
void ss_release_commit(struct samsung_display_driver_data *vdd);
int ss_check_panel_connection(struct samsung_display_driver_data *vdd);
struct samsung_display_driver_data *ss_get_vdd_from_panel_name(const char *panel_name);

/***************************************************************************************************
*		BRIGHTNESS RELATED END.
****************************************************************************************************/

/* MDNIE_LITE_COMMON_HEADER */
#include "ss_dsi_mdnie_lite_common.h"
/* SAMSUNG MODEL HEADER */


/* Interface betwenn samsung and msm display driver
 */
extern struct dsi_display_boot_param boot_displays[MAX_DSI_ACTIVE_DISPLAY];
extern char dsi_display_primary[MAX_CMDLINE_PARAM_LEN];
extern char dsi_display_secondary[MAX_CMDLINE_PARAM_LEN];


static inline void ss_get_primary_panel_name_cmdline(char *panel_name)
{
	char *pos = NULL;
	size_t len;

	pos = strnstr(dsi_display_primary, ":", sizeof(dsi_display_primary));
	if (!pos)
		return;

	len = (size_t) (pos - dsi_display_primary) + 1;

	strlcpy(panel_name, dsi_display_primary, len);
}

static inline void ss_get_secondary_panel_name_cmdline(char *panel_name)
{
	char *pos = NULL;
	size_t len;

	pos = strnstr(dsi_display_secondary, ":", sizeof(dsi_display_secondary));
	if (!pos)
		return;

	len = (size_t) (pos - dsi_display_secondary) + 1;

	strlcpy(panel_name, dsi_display_secondary, len);
}

struct samsung_display_driver_data *ss_get_vdd(enum ss_display_ndx ndx);

static inline struct samsung_display_driver_data *ss_get_vdd_from_display(struct dsi_display *display)
{
	return display->panel->panel_private;
}

#define GET_DSI_PANEL(vdd)	((struct dsi_panel *) (vdd)->msm_private)
static inline struct dsi_display *GET_DSI_DISPLAY(
		struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct dsi_display *display = dev_get_drvdata(panel->parent);
	return display;
}

static inline struct drm_device *GET_DRM_DEV(
		struct samsung_display_driver_data *vdd)
{
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	struct drm_device *ddev = display->drm_dev;

	return ddev;
}

static inline struct msm_kms *GET_MSM_KMS(
		struct samsung_display_driver_data *vdd)
{
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	struct drm_device *ddev = display->drm_dev;
	struct msm_drm_private *priv = ddev->dev_private;

	return priv->kms;
}
#define GET_SDE_KMS(vdd)	to_sde_kms(GET_MSM_KMS(vdd))

static inline struct drm_crtc *GET_DRM_CRTC(
		struct samsung_display_driver_data *vdd)
{
#if 1
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	struct drm_device *ddev = display->drm_dev;
	struct msm_drm_private *priv = ddev->dev_private;
	int id;

	// TODO: get correct crtc_id ...
	id = 0; // crtc id: 0 = primary display, 1 = wb...

	return priv->crtcs[id];
#else
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	struct drm_device *ddev = display->drm_dev;
	struct list_head *crtc_list;
	struct drm_crtc *crtc = NULL, *crtc_iter;
	struct sde_crtc *sde_crtc;

	// refer to _sde_kms_setup_displays()
	crtc_list = &ddev->mode_config.crtc_list;
	list_for_each_entry(crtc_iter, crtc_list, head) {
		sde_crtc = to_sde_crtc(crtc_iter);
		if (sde_crtc->display == display) {
			crtc = crtc_iter;
			break;
		}
	}

	if (!crtc)
		LCD_ERR(vdd, "failed to find drm_connector\n");

	return crtc;

#endif

}

static inline struct drm_encoder *GET_DRM_ENCODER(
		struct samsung_display_driver_data *vdd)
{
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);

	return display->bridge->base.encoder;
}

/* TODO: move whole bridge funcitons to ss_wrapper_common.c file */
static inline struct drm_connector *GET_DRM_CONNECTORS(
		struct samsung_display_driver_data *vdd)
{
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	struct drm_device *ddev = display->drm_dev;
	struct list_head *connector_list;
	struct drm_connector *conn = NULL, *conn_iter;
	struct sde_connector *c_conn;

	if (IS_ERR_OR_NULL(ddev)) {
		LCD_ERR(vdd, "ddev is NULL\n");
		return NULL;
	}

	/* refer to _sde_kms_setup_displays() */
	connector_list = &ddev->mode_config.connector_list;
	list_for_each_entry(conn_iter, connector_list, head) {
		c_conn = to_sde_connector(conn_iter);
		if (c_conn->display == display) {
			conn = conn_iter;
			break;
		}
	}

	if (!conn)
		LCD_ERR(vdd, "failed to find drm_connector\n");

	return conn;
}

#define GET_SDE_CONNECTOR(vdd)	to_sde_connector(GET_DRM_CONNECTORS(vdd))

static inline struct backlight_device *GET_SDE_BACKLIGHT_DEVICE(
		struct samsung_display_driver_data *vdd)
{
	struct backlight_device *bd = NULL;
	struct sde_connector *conn = NULL;

	if (IS_ERR_OR_NULL(vdd))
		goto end;

	conn = GET_SDE_CONNECTOR(vdd);
	if (IS_ERR_OR_NULL(conn))
		goto end;

	bd = conn->bl_device;
	if (IS_ERR_OR_NULL(bd))
		goto end;

end:
	return bd;
}

/* In dual panel, it has two panel, and
 * ndx = 0: primary panel, ndx = 1: secondary panel
 * In one panel with dual dsi port, like split mode,
 * it has only one panel, and ndx = 0.
 * In one panel with single dsi port,
 * it has only one panel, and ndx = 0.
 */
static inline enum ss_display_ndx ss_get_display_ndx(
		struct samsung_display_driver_data *vdd)
{
	return vdd->ndx;
}

extern struct samsung_display_driver_data vdd_data[MAX_DISPLAY_NDX];

static inline const char *ss_get_panel_name(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	return panel->name;
}

/* return panel x resolution */
static inline u32 ss_get_xres(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	/* FC2 change: set panle mode in SurfaceFlinger initialization, instead of kenrel booting... */
	if (!panel->cur_mode) {
		LCD_ERR(vdd, "err: no panel mode yet...\n");
		return 0;
	}

	return panel->cur_mode->timing.h_active;
}

/* return panel y resolution */
static inline u32 ss_get_yres(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	/* FC2 change: set panle mode in SurfaceFlinger initialization, instead of kenrel booting... */
	if (!panel->cur_mode) {
		LCD_ERR(vdd, "err: no panel mode yet...\n");
		return 0;
	}
	return panel->cur_mode->timing.v_active;
}

/* check if it is dual dsi port */
static inline bool ss_is_dual_dsi(struct samsung_display_driver_data *vdd)
{
	if (vdd->num_of_intf == 2)
		return true;
	else
		return false;
}

/* check if it is dual dsi port */
static inline bool ss_is_single_dsi(struct samsung_display_driver_data *vdd)
{
	if (vdd->num_of_intf == 1)
		return true;
	else
		return false;
}

/* check if it is command mode panel */
static inline bool ss_is_cmd_mode(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	if (panel->panel_mode == DSI_OP_CMD_MODE)
		return true;
	else
		return false;
}

/* check if it is video mode panel */
static inline bool ss_is_video_mode(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	if (panel->panel_mode == DSI_OP_VIDEO_MODE)
		return true;
	else
		return false;
}
/* check if it controls backlight via MIPI DCS */
static inline bool ss_is_bl_dcs(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	if (panel->bl_config.type == DSI_BACKLIGHT_DCS)
		return true;
	else
		return false;
}

/* check if panel is on */
static inline bool ss_is_panel_on(struct samsung_display_driver_data *vdd)
{
	return (vdd->panel_state == PANEL_PWR_ON);
}

static inline bool ss_is_panel_on_ready(struct samsung_display_driver_data *vdd)
{
	return (vdd->panel_state == PANEL_PWR_ON_READY);
}

static inline bool ss_is_ready_to_send_cmd(struct samsung_display_driver_data *vdd)
{
	return ((vdd->panel_state == PANEL_PWR_ON) ||
			(vdd->panel_state == PANEL_PWR_LPM) ||
			(vdd->panel_state == PANEL_PWR_ON_READY));
}

static inline bool ss_is_panel_off(struct samsung_display_driver_data *vdd)
{
	return (vdd->panel_state == PANEL_PWR_OFF);
}

/* check if panel is low power mode */
static inline bool ss_is_panel_lpm(
		struct samsung_display_driver_data *vdd)
{
	return (vdd->panel_state == PANEL_PWR_LPM);
}

static inline bool ss_is_panel_lpm_ongoing(
		struct samsung_display_driver_data *vdd)
{
	/* For lego, regards as LPM mode for normal and lpm transition phase
	 * Discussed with dlab.
	 */
	return (vdd->panel_state == PANEL_PWR_LPM || vdd->panel_lpm.during_ctrl);
}

static inline int ss_is_read_cmd(struct dsi_panel_cmd_set *set)
{
	switch(set->cmds->msg.type) {
		case MIPI_DSI_DCS_READ:
		case MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM:
		case MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM:
		case MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM:
			if (set->count == 1)
				return 1;
			else {
				pr_err("[SDE] set->count[%d] is not 1.. ", set->count);
				return 0;
			}
		default:
			pr_debug("[SDE] type[%02x] is not read type.. ", set->cmds->msg.type);
			return 0;
	}
}

static inline int ss_is_read_ss_cmd(struct ss_cmd_desc *cmd)
{
	switch (cmd->type) {
	case MIPI_DSI_DCS_READ:
	case MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM:
		return 1;
	default:
		return 0;
	}
}

/* check if it is seamless mode (continuous splash mode) */
static inline bool ss_is_seamless_mode(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	/* FC2 change: set panle mode in SurfaceFlinger initialization, instead of kenrel booting... */
	if (!panel->cur_mode) {
		LCD_ERR(vdd,  "err: no panel mode yet...\n");
		// setting ture prevents tx dcs cmd...
		return false;
	}

	if (panel->cur_mode->dsi_mode_flags & DSI_MODE_FLAG_SEAMLESS)
		return true;
	else
		return false;
}

static inline void ss_set_seamless_mode(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	/* FC2 change: set panle mode in SurfaceFlinger initialization, instead of kenrel booting... */
	if (!panel->cur_mode) {
		LCD_ERR(vdd,  "err: no panel mode yet...\n");
		return;
	}

	panel->cur_mode->dsi_mode_flags |= DSI_MODE_FLAG_SEAMLESS;
}

static inline unsigned int ss_get_te_gpio(struct samsung_display_driver_data *vdd)
{
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	return display->disp_te_gpio;
}

static inline bool is_ss_cmd(int type)
{
	if (type > SS_DSI_CMD_SET_START && type < SS_DSI_CMD_SET_MAX)
		return true;
	return false;
}

extern const char *cmd_set_prop_map[DSI_CMD_SET_MAX];
static inline char *ss_get_cmd_name(int type)
{
	static char str_buffer[SS_CMD_PROP_STR_LEN];

	if (type > SS_DSI_CMD_SET_START && type < SS_DSI_CMD_SET_MAX)
		strcpy(str_buffer, ss_cmd_set_prop_map[type - SS_DSI_CMD_SET_START]);
	else
		strcpy(str_buffer, cmd_set_prop_map[type]);

	return str_buffer;
}

/* deprecated: TODO: remove this */
static inline void ss_alloc_ss_txbuf(struct dsi_cmd_desc *cmds, u8 *ss_txbuf)
{
	/*
	 * Allocate new txbuf for samsung display (ss_txbuf).
	 *
	 * Due to GKI, We do not add any s/w code in generaic kernel code.
	 * Because tx_buf in struct mipi_dsi_msg is const type,
	 * it is NOT allowed to modify during running time.
	 * To modify tx buf during running time, samsung allocate new tx buf (ss_tx_buf).
	 */

	cmds->msg.tx_buf = ss_txbuf;

	return;
}

static inline bool __must_check SS_IS_CMDS_NULL(struct dsi_panel_cmd_set *set)
{
	return unlikely(!set) || unlikely(!set->cmds);
}

static inline bool __must_check SS_IS_SS_CMDS_NULL(struct ss_cmd_set *set)
{
	return unlikely(!set) || unlikely(!set->cmds);
}

static inline struct device_node *ss_get_panel_of(
		struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	return panel->panel_of_node;
}

static inline struct device_node *ss_get_mafpc_of(
		struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	return panel->mafpc_of_node;
}

static inline struct device_node *ss_get_self_disp_of(
		struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	return panel->self_display_of_node;
}

/* TBR: deprecated code. */

/*
 * Find cmd idx in cmd_set.
 * cmd : cmd register to find.
 * offset : global offset value.
 *        : 0x00 - has 0x00 offset gpara or No gpara.
 */
static inline int ss_get_cmd_idx(struct dsi_panel_cmd_set *set, int offset, u8 cmd)
{
	int i;
	int idx = -1;
	int para = 0;

	for (i = 0; i < set->count; i++) {
		if (set->cmds[i].ss_txbuf[0] == cmd) {
			/* check proper offset */
			if (i == 0) {
				if (offset == 0x00)
					idx = i;
			} else {
				if (set->cmds[i - 1].ss_txbuf[0] == 0xb0) {
					/* 1byte offset */
					if (set->cmds[i - 1].msg.tx_len == 3) {
						if ((set->cmds[i - 1].ss_txbuf[1] == offset)
							&& (set->cmds[i - 1].ss_txbuf[2] == cmd))
							idx = i;
					}
					/* 2byte offset */
					else if (set->cmds[i - 1].msg.tx_len == 4) {
						para = (set->cmds[i - 1].ss_txbuf[1] & 0xFF) << 8;
						para |= set->cmds[i - 1].ss_txbuf[2] & 0xFF;
						if ((para == offset) && (set->cmds[i - 1].ss_txbuf[3] == cmd))
							idx = i;
					}
				} else {
					if (offset == 0x00)
						idx = i;
				}
			}

			if (idx != -1) {
				pr_debug("find cmd[0x%x] offset[0x%x] idx (%d)\n", cmd, offset, idx);
				break;
			}
		}
	}

	if (idx == -1)
		pr_err("[SDE] Can not find cmd[0x%x] offset[0x%x]\n", cmd, offset);

	return idx;
}

static inline uint GET_BITS(uint data, int from, int to) {
	int i, j;
	uint ret = 0;

	for (i = from, j = 0; i <= to; i++, j++)
		ret |= (data & BIT(i)) ? (1 << j) : 0;

	return ret;
}

static inline void ss_print_cmd_desc(struct dsi_cmd_desc *cmd_desc, struct samsung_display_driver_data *vdd)
{
	struct mipi_dsi_msg *msg = &cmd_desc->msg;
	char buf[1024];
	int len = 0;
	size_t i;
	u8 *tx_buf = (u8 *)msg->tx_buf;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->debug_data))
		return;

	/* Packet Info */
	len += snprintf(buf, sizeof(buf) - len,  "%02X ", msg->type);
	len += snprintf(buf + len, sizeof(buf) - len, "%02X ",
			!!(cmd_desc->ctrl_flags & DSI_CTRL_CMD_LAST_COMMAND));
	len += snprintf(buf + len, sizeof(buf) - len, "%02X ", msg->channel);
	len += snprintf(buf + len, sizeof(buf) - len, "%02X ",
						(unsigned int)msg->flags);
	len += snprintf(buf + len, sizeof(buf) - len, "%02X ", cmd_desc->post_wait_ms); /* Delay */
	len += snprintf(buf + len, sizeof(buf) - len, "%02X ",
						(unsigned int)msg->tx_len);

	/* Packet Payload */
	for (i = 0 ; i < msg->tx_len; i++) {
		len += snprintf(buf + len, sizeof(buf) - len, "%02X ", tx_buf[i]);
		/* Break to prevent show too long command */
		if (i > 250)
			break;
	}

	LCD_INFO_IF(vdd, "(%02d) %s\n", (unsigned int)msg->tx_len, buf);
}

static inline void ss_print_cmd_desc_evtlog(struct dsi_cmd_desc *cmd_desc, struct samsung_display_driver_data *vdd)
{
	struct mipi_dsi_msg *msg = &cmd_desc->msg;
	size_t i;
	u8 *tx_buf = (u8 *)msg->tx_buf;

	struct dbg_tear_info *dbg = &vdd->dbg_tear_info;
	int pos = (dbg->pos_evtlog + 1) % MAX_CMD_EVTLOG_NUM;
	struct cmd_evtlog *evtlog = &dbg->evtlog[pos];

	dbg->pos_evtlog = pos;

	evtlog->time = ktime_get();
	evtlog->cmd_len = msg->tx_len;
	for (i = 0 ; i < msg->tx_len && i < MAX_CMD_EVTLOG_CMDS_NUM; i++)
		evtlog->cmds[i] = tx_buf[i];
}

static inline void ss_print_cmd_set(struct dsi_panel_cmd_set *cmd_set, struct samsung_display_driver_data *vdd)
{
	int cmd_idx = 0;
	char buf[1024];
	int len = 0;
	size_t i;
	u8 *tx_buf = NULL;
	struct mipi_dsi_msg *msg = NULL;
	struct dsi_cmd_desc *cmd_desc = NULL;

	if (IS_ERR_OR_NULL(vdd))
		return;

	LCD_INFO_IF(vdd, "Print [%s] cmd\n", cmd_set->name);

	for (cmd_idx = 0 ; cmd_idx < cmd_set->count ; cmd_idx++) {
		cmd_desc = &cmd_set->cmds[cmd_idx];
		msg = &cmd_desc->msg;
		tx_buf = (u8 *)msg->tx_buf;

		/* Packet Info */
		len += snprintf(buf, sizeof(buf) - len,  "%02X ", msg->type);
		len += snprintf(buf + len, sizeof(buf) - len, "%02X ",
				!!(cmd_desc->ctrl_flags & DSI_CTRL_CMD_LAST_COMMAND));
		len += snprintf(buf + len, sizeof(buf) - len, "%02X ", msg->channel);
		len += snprintf(buf + len, sizeof(buf) - len, "%02X ",
							(unsigned int)msg->flags);
		len += snprintf(buf + len, sizeof(buf) - len, "%02X ", cmd_desc->post_wait_ms); /* Delay */
		len += snprintf(buf + len, sizeof(buf) - len, "%02X ",
							(unsigned int)msg->tx_len);

		/* Packet Payload */
		for (i = 0 ; i < msg->tx_len; i++) {
			len += snprintf(buf + len, sizeof(buf) - len, "%02X ", tx_buf[i]);
			/* Break to prevent show too long command */
			if (i > 250)
				break;
		}

		LCD_INFO_IF(vdd, "(%02d) %s\n", (unsigned int)msg->tx_len, buf);
	}
}

static inline void ss_print_vsync(struct samsung_display_driver_data *vdd)
{
	static ktime_t last_t[MAX_DISPLAY_NDX];
	static ktime_t delta_t[MAX_DISPLAY_NDX];
	int idx = vdd->ndx;
	int cal_fps;
	int cur_fps = vdd->vrr.cur_refresh_rate;

	delta_t[idx] = ktime_get() - last_t[idx];
	cal_fps = 1000000 / ktime_to_us(delta_t[idx]);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->debug_data))
		return;

	if ((vdd->debug_data->print_all || vdd->debug_data->print_vsync) ||
			(vdd->vrr.check_vsync > 0 && vdd->vrr.check_vsync != CHECK_VSYNC_COUNT)) {
		LCD_INFO(vdd, "Vsync (%llu.%llu ms delta, %d/%d)\n",
			GET_PRIM_US(delta_t[idx]), GET_REST_US(delta_t[idx]), cal_fps, cur_fps);
		vdd->vrr.check_vsync--;
	} else if (vdd->vrr.check_vsync == CHECK_VSYNC_COUNT)
		vdd->vrr.check_vsync--; /* Skip First Vsync to reduce log & ignore inaccurate one */
	else
		vdd->vrr.check_vsync = 0;

	if (vdd->vrr.running_vrr)
		SS_XLOG(cal_fps, cur_fps);


	last_t[idx] = ktime_get();
}

static inline void ss_print_rw_low(struct samsung_display_driver_data *vdd)
{
	static ktime_t last_t[MAX_DISPLAY_NDX];
	static ktime_t delta_t[MAX_DISPLAY_NDX];
	int idx = vdd->ndx;
	int cal_fps;
	int cur_fps = vdd->vrr.cur_refresh_rate;

	delta_t[idx] = ktime_get() - last_t[idx];
	cal_fps = 1000000 / ktime_to_us(delta_t[idx]);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->debug_data))
		return;

	if ((vdd->debug_data->print_all || vdd->debug_data->print_vsync))
		LCD_INFO(vdd, "RW Low (%llu.%llu ms delta, %d/%d)\n",
		GET_PRIM_US(delta_t[idx]), GET_REST_US(delta_t[idx]), cal_fps, cur_fps);

	last_t[idx] = ktime_get();
}

static inline void ss_print_vsync_control(bool enable)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(PRIMARY_DISPLAY_NDX);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->debug_data))
		return;

	if (vdd->debug_data->print_all || vdd->debug_data->print_vsync)
		pr_info("[SDE] Vsync %s\n", enable ? "Enable" : "Disable");
}

/* For Command panel only */
static inline void ss_print_frame(struct samsung_display_driver_data *vdd, bool frame_start)
{
	static ktime_t last_t[MAX_DISPLAY_NDX];
	static ktime_t delta_t[MAX_DISPLAY_NDX];
	int idx = vdd->ndx;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->debug_data))
		return;

	if (vdd->debug_data->print_all || vdd->debug_data->print_frame) {
		if (frame_start) {
			/*
			 * Incase of Video Mode(Incl. VHM), there is no way to recognize frame done timing
			 * Only Frame Start can be recognized by vblank interrupt
			 */
			if (ss_is_video_mode(vdd)) {
				delta_t[idx] = ktime_get() - last_t[idx];
				LCD_INFO(vdd, "Frame[%d] Start(%lld.%lldms Delta) / %ld)\n",
					vdd->debug_data->frame_cnt++,
					GET_PRIM_US(delta_t[idx]), GET_REST_US(delta_t[idx]),
					vdd->dvrr.support_dvrr ? GET_PRIM_MS(vdd->dvrr.cur_frame_interval) : vdd->vrr.cur_refresh_rate);
			} else
				LCD_INFO(vdd, "Frame[%d] Start\n", vdd->debug_data->frame_cnt);

			last_t[idx] = ktime_get();
		} else {
			delta_t[idx] = ktime_get() - last_t[idx];
			LCD_INFO(vdd, "Frame[%d] Finish (%lld.%lldms elapsed / %ld_FPS)\n", vdd->debug_data->frame_cnt++,
							GET_PRIM_US(delta_t[idx]), GET_REST_US(delta_t[idx]),
							vdd->dvrr.support_dvrr ? GET_PRIM_MS(vdd->dvrr.cur_frame_interval) : vdd->vrr.cur_refresh_rate);
		}
	}
}

static inline bool is_acl_on(struct samsung_display_driver_data *vdd)
{
	if (vdd->br_info.acl_status || vdd->siop_status)
		return true;
	else
		return false;
}

static inline char *ss_get_str_property(const char *buf, const char *name, int *lenp)
{
	const char *cur = buf;
	const char *backup = NULL;
	const char *data_start = NULL;
	char *data = NULL;

	if (!buf || !name)
		return NULL;

	*lenp = 0;

	do {
		cur = strstr(cur, name);
		if (cur) {
			/*
			 * Case 1) TARGET_SYMBOL= ""
			 * Case 2) TARGET_SYMBOL = ""
			 * Case 3) TARGET_SYMBOL_OTHER = ""
			 *
			 * In case of 1), cur + strlen(name) is '=' (CMD_STR_EQUAL)
			 * In case of 2), cur + strlen(name) is ' ' (DELIMITER)
			 * In case of 3), cur + strlen(name) is '_'
			 *
			 * We should skip only case 3)
			 */
			if ((!IS_CMD_DELIMITER(*(cur + strlen(name))) && *(cur + strlen(name)) != CMD_STR_EQUAL)
				|| !IS_CMD_DELIMITER(*(cur-1))) {
				cur++;
				continue;
			}

			backup = cur;
			cur = strnchr(cur, strlen(name) + 5, CMD_STR_EQUAL);
			if (cur)
				break;
			cur = backup + 1;
		}

	} while (cur);

	if (!cur)
		return NULL;

	cur = strnchr(cur, 5, CMD_STR_QUOT); /* Find '"' */
	if (!cur) {
		pr_err("[SDE] Can't find matched data with [%s]\n", name);
		return NULL;
	}

	cur++;
	data_start = cur;

	while (*cur != CMD_STR_QUOT) {
		cur++;
		*lenp = *lenp + 1;

		if (*cur == CMD_STR_NUL || *cur == CMD_STR_EQUAL) {
			pr_err("[SDE] [%s] parsing fail\n", name);
			*lenp = 0;
			return NULL;
		}
	}
	*lenp = *lenp + 1; /* lenp include last NULL char('\0') to match with of_get_property() API */

	/* *lenp > 1 means there is even one data (XXXX = "1") */
	if (*lenp > 1) {
		data = kvzalloc(*lenp, GFP_KERNEL);
		if (!data) {
			pr_err("[SDE] [%s] data alloc fail (out of memory)\n", name);
			return NULL;
		}
	} else {
		pr_debug("[SDE] [%s] is empty\n", name);
		*lenp = 0;
		return NULL;
	}

	strncpy(data, data_start, *lenp - 1);

	pr_debug("[SDE] %s : name = [%s], len=[%d], payload=[%s]\n", __func__, name, *lenp, data);

	return (u8 *)data;
}

static inline bool is_running_vrr(struct samsung_display_driver_data *vdd)
{
	if (vdd->vrr.running_vrr_mdp || vdd->vrr.running_vrr)
		return true;
	else
		return false;
}
#endif /* SAMSUNG_DSI_PANEL_COMMON_H */
