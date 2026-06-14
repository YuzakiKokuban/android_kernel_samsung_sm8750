/* SPDX-License-Identifier: GPL-2.0-only */
/* drivers/input/sec_input/sec_input.h
 *
 * Core file for Samsung input device driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _SEC_INPUT_H_
#define _SEC_INPUT_H_

#if IS_ENABLED(CONFIG_SEC_KUNIT)
#include <kunit/test.h>
#define __visible_for_testing
#if IS_ENABLED(CONFIG_UML)
#include <kunit/mock.h>
#endif
#else
#include "sec_input_kunit_dummy.h"
#endif

#include <asm/unaligned.h>
#include <linux/completion.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/pm_wakeup.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/rtc.h>
#include <linux/errno.h>
#include <linux/init.h>

#if IS_ENABLED(CONFIG_INPUT_SEC_TRUSTED_TOUCH)
#include "sec_trusted_touch.h"
#endif
#if IS_ENABLED(CONFIG_INPUT_SEC_SECURE_TOUCH)
#include "sec_secure_touch.h"
#endif

#define SECURE_TOUCH_ENABLE	1
#define SECURE_TOUCH_DISABLE	0

#if IS_ENABLED(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif
#include <linux/notifier.h>
#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
#include <linux/vbus_notifier.h>
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
#include <linux/usb/typec/manager/usb_typec_manager_notifier.h>
#endif
#endif
#if (KERNEL_VERSION(5, 14, 0) <= LINUX_VERSION_CODE)
#include <linux/devm-helpers.h>
#define SEC_INPUT_INIT_WORK(dev, work, func)	devm_work_autocancel(dev, work, func)
#else
#define SEC_INPUT_INIT_WORK(dev, work, func)	INIT_WORK(work, func)
#endif

#include "sec_cmd.h"
#include "sec_tclm_v2.h"
#include "sec_input_multi_dev.h"
#if IS_ENABLED(CONFIG_TOUCHSCREEN_DUMP_MODE)
#include "sec_tsp_dumpkey.h"
#endif

#if (KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE)
#define sec_input_proc_ops(ops_owner, ops_name, read_fn, write_fn)	\
const struct proc_ops ops_name = {					\
	.proc_read = read_fn,						\
	.proc_write = write_fn,						\
	.proc_lseek = generic_file_llseek,				\
}
#else
#define sec_input_proc_ops(ops_owner, ops_name, read_fn, write_fn)	\
const struct file_operations ops_name = {				\
	.owner = ops_owner,						\
	.read = read_fn,						\
	.write = write_fn,						\
	.llseek = generic_file_llseek,					\
}
#endif

/*
 * sys/class/sec/tsp/support_feature
 * bit value should be made a promise with InputFramework.
 */
#define INPUT_FEATURE_ENABLE_SETTINGS_AOT	(1 << 0) /* Double tap wakeup settings */
#define INPUT_FEATURE_ENABLE_PRESSURE		(1 << 1) /* homekey pressure */
//#define INPUT_FEATURE_ENABLE_SYNC_RR120		(1 << 2) /* UNUSED: sync reportrate 120hz */
#define INPUT_FEATURE_ENABLE_VRR		(1 << 3) /* variable refresh rate (support 240hz) */
#define INPUT_FEATURE_SUPPORT_WIRELESS_TX		(1 << 4) /* enable call wireless cmd during wireless power sharing */
#define INPUT_FEATURE_ENABLE_SYSINPUT_ENABLED		(1 << 5) /* resume/suspend called by system input service */
#define INPUT_FEATURE_ENABLE_PROX_LP_SCAN_ENABLED	(1 << 6) /* prox_lp_scan_mode called by system input service */

#define INPUT_FEATURE_SUPPORT_OPEN_SHORT_TEST		(1 << 8) /* open/short test support */
#define INPUT_FEATURE_SUPPORT_MIS_CALIBRATION_TEST	(1 << 9) /* mis-calibration test support */
#define INPUT_FEATURE_ENABLE_MULTI_CALIBRATION		(1 << 10) /* multi calibration support */

#define INPUT_FEATURE_SUPPORT_INPUT_MONITOR			(1 << 16) /* input monitor support */

#define INPUT_FEATURE_SUPPORT_RAWDATA_TRANSFER			(1 << 18) /* rawdata motion control */
#define INPUT_FEATURE_SUPPORT_POCKET_DETECT			(1 << 19) /* rawdata motion control: pocket */
#define INPUT_FEATURE_SUPPORT_MOTION_PALM			(1 << 20) /* rawdata motion control: palm */
#define INPUT_FEATURE_SUPPORT_MOTION_AIVF			(1 << 21) /* rawdata motion control: aivf */
#define INPUT_FEATURE_SUPPORT_MOTION_PALM_SWIPE		(1 << 22) /* rawdata motion control: palm swipe */
#define INPUT_FEATURE_SUPPORT_MOTION_AWD			(1 << 23) /* rawdata motion control: awd */

/*
 * sec Log
 */
#define SECLOG				"[sec_input]"
#define INPUT_MODULE_INFO_BUF_SIZE	64
#define INPUT_LOG_BUF_SIZE			512
#define INPUT_TCLM_LOG_BUF_SIZE		64
#define INPUT_DEBUG_INFO_SIZE		1024

#if IS_ENABLED(CONFIG_SEC_DEBUG_TSP_LOG)
#include "sec_tsp_log.h"

#define input_dbg(mode, dev, fmt, ...)						\
({										\
	dev_dbg(dev, SECLOG " " fmt, ## __VA_ARGS__);				\
	sec_input_log(mode, dev, fmt, ## __VA_ARGS__);				\
})
#define input_info(mode, dev, fmt, ...)						\
({										\
	dev_info(dev, SECLOG " " fmt, ## __VA_ARGS__);				\
	sec_input_log(mode, dev, fmt, ## __VA_ARGS__);				\
})
#define input_err(mode, dev, fmt, ...)						\
({										\
	dev_err(dev, SECLOG " " fmt, ## __VA_ARGS__);				\
	sec_input_log(mode, dev, fmt, ## __VA_ARGS__);				\
})
#define input_fail_hist(mode, dev, fmt, ...)					\
({										\
	dev_err(dev, SECLOG " " fmt, ## __VA_ARGS__);				\
	sec_input_fail_hist_log(mode, dev, fmt, ## __VA_ARGS__);		\
})
#define input_raw_info_d(dev_count, dev, fmt, ...)				\
({										\
	dev_info(dev, SECLOG " " fmt, ## __VA_ARGS__);				\
	sec_input_raw_info_log(dev_count, dev, fmt, ## __VA_ARGS__);	\
})
#define input_raw_info(mode, dev, fmt, ...)					\
({										\
	if (mode) {								\
		input_raw_info_d(0, dev, fmt, ## __VA_ARGS__);			\
	} else {									\
		dev_info(dev, SECLOG " " fmt, ## __VA_ARGS__);			\
	}									\
})
#define input_raw_data_clear_by_device(mode) sec_tsp_raw_data_clear(mode)
#define input_raw_data_clear() sec_tsp_raw_data_clear(0)
#define input_log_fix()	sec_tsp_log_fix()
#else
#define input_dbg(mode, dev, fmt, ...)						\
({										\
	dev_dbg(dev, SECLOG " " fmt, ## __VA_ARGS__);				\
})
#define input_info(mode, dev, fmt, ...)						\
({										\
	dev_info(dev, SECLOG " " fmt, ## __VA_ARGS__);				\
})
#define input_err(mode, dev, fmt, ...)						\
({										\
	dev_err(dev, SECLOG " " fmt, ## __VA_ARGS__);				\
})
#define input_fail_hist(mode, dev, fmt, ...) input_err(mode, dev, fmt, ## __VA_ARGS__)
#define input_raw_info_d(dev_count, dev, fmt, ...) input_info(true, dev, fmt, ## __VA_ARGS__)
#define input_raw_info(mode, dev, fmt, ...) input_info(mode, dev, fmt, ## __VA_ARGS__)
#define input_log_fix()	{}
#define input_raw_data_clear_by_device(mode) {}
#define input_raw_data_clear() {}
#endif

/*
 * for input_event_codes.h
 */
#define KEY_HOT			252
#define KEY_WAKEUP_UNLOCK	253	/* Wake-up to recent view, ex: AOP */
#define KEY_RECENT		254

#define KEY_WATCH		550	/* Premium watch: 2finger double tap */

#define BTN_PALM		0x118	/* palm flag */
#define BTN_LARGE_PALM		0x119	/* large palm flag */

#define KEY_APPSELECT		0x244	/* AL Select Task/Application */
#define KEY_EMERGENCY		0x2a0
#define KEY_INT_CANCEL		0x2be	/* for touch event skip */
#define KEY_DEX_ON		0x2bd
#define KEY_WINK		0x2bf	/* Intelligence Key */
#define KEY_APPS		0x2c1	/* APPS key */
#define KEY_SIP			0x2c2	/* Sip key */
#define KEY_SETTING		0x2c5	/* Setting */
#define KEY_SFINDER		0x2c6	/* Sfinder key */
#define KEY_SHARE		0x2c9	/* keyboard share */
#define KEY_FN_LOCK		0x2ca	/* fn_lock key */
#define KEY_FN_UNLOCK		0x2cb	/* fn_unlock key */

#define KEY_AI_HOT		0x2F8	/* ai hot key */

#define ABS_MT_CUSTOM		0x3e	/* custom event */

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#define SW_PEN_INSERT		0x0f  /* set = pen insert, remove */
#else
#define SW_PEN_INSERT		0x13  /* set = pen insert, remove */
#endif
#define SW_PEN_REVERSE_INSERT	0x0d  /* set = pen reverse insert, remove */

#define EXYNOS_DISPLAY_INPUT_NOTIFIER ((IS_ENABLED(CONFIG_EXYNOS_DPU30) || IS_ENABLED(CONFIG_DRM_SAMSUNG_DPU)) && IS_ENABLED(CONFIG_PANEL_NOTIFY))

enum grip_write_mode {
	G_NONE				= 0,
	G_SET_EDGE_HANDLER		= 1,
	G_SET_EDGE_ZONE			= 2,
	G_SET_NORMAL_MODE		= 4,
	G_SET_LANDSCAPE_MODE	= 8,
	G_CLR_LANDSCAPE_MODE	= 16,
};
enum grip_set_data {
	ONLY_EDGE_HANDLER		= 0,
	GRIP_ALL_DATA			= 1,
};

enum external_noise_mode {
	EXT_NOISE_MODE_NONE		= 0,
	EXT_NOISE_MODE_MONITOR		= 1,	/* for dex mode */
	EXT_NOISE_MODE_MAX,			/* add new mode above this line */
};

enum wireless_charger_param {
	TYPE_WIRELESS_CHARGER_NONE	= 0,
	TYPE_WIRELESS_CHARGER		= 1,
	TYPE_WIRELESS_BATTERY_PACK	= 3,
};

enum charger_param {
	TYPE_WIRE_CHARGER_NONE	= 0,
	TYPE_WIRE_CHARGER		= 1,
};

enum set_temperature_state {
	SEC_INPUT_SET_TEMPERATURE_NORMAL = 0,
	SEC_INPUT_SET_TEMPERATURE_IN_IRQ,
	SEC_INPUT_SET_TEMPERATURE_FORCE,
};

enum input_device_type {
	INPUT_DEVICE_TYPE_TOUCH = 0,
	INPUT_DEVICE_TYPE_TOUCHPAD,
	INPUT_DEVICE_TYPE_PROXIMITY,
	INPUT_DEVICE_TYPE_MAX,
};

/* FACTORY TEST RESULT SAVING FUNCTION
 * bit 3 ~ 0 : OCTA Assy
 * bit 7 ~ 4 : OCTA module
 * param[0] : OCTA modue(1) / OCTA Assy(2)
 * param[1] : TEST NONE(0) / TEST FAIL(1) / TEST PASS(2) : 2 bit
 */
#define TEST_OCTA_MODULE 1
#define TEST_OCTA_ASSAY  2
#define TEST_OCTA_NONE  0
#define TEST_OCTA_FAIL  1
#define TEST_OCTA_PASS  2

/*
 * write 0xE4 [ 11 | 10 | 01 | 00 ]
 * MSB <-------------------> LSB
 * read 0xE4
 * mapping sequnce : LSB -> MSB
 * struct sec_ts_test_result {
 * * assy : front + OCTA assay
 * * module : only OCTA
 *  union {
 *   struct {
 *    u8 assy_count:2;  -> 00
 *    u8 assy_result:2;  -> 01
 *    u8 module_count:2; -> 10
 *    u8 module_result:2; -> 11
 *   } __attribute__ ((packed));
 *   unsigned char data[1];
 *  };
 *};
 */

struct sec_ts_test_result {
	union {
		struct {
			u8 assy_count:2;
			u8 assy_result:2;
			u8 module_count:2;
			u8 module_result:2;
		} __attribute__ ((packed));
			unsigned char data[1];
	};
};

enum sponge_event_type {
	SPONGE_EVENT_TYPE_SPAY			= 0x04,
	SPONGE_EVENT_TYPE_SINGLE_TAP		= 0x08,
	SPONGE_EVENT_TYPE_AOD_PRESS		= 0x09,
	SPONGE_EVENT_TYPE_AOD_LONGPRESS		= 0x0A,
	SPONGE_EVENT_TYPE_AOD_DOUBLETAB		= 0x0B,
	SPONGE_EVENT_TYPE_AOD_HOMEKEY_PRESS	= 0x0C,
	SPONGE_EVENT_TYPE_AOD_HOMEKEY_RELEASE	= 0x0D,
	SPONGE_EVENT_TYPE_AOD_HOMEKEY_RELEASE_NO_HAPTIC	= 0x0E,
	SPONGE_EVENT_TYPE_FOD_PRESS		= 0x0F,
	SPONGE_EVENT_TYPE_FOD_RELEASE		= 0x10,
	SPONGE_EVENT_TYPE_FOD_OUT		= 0x11,
	SPONGE_EVENT_TYPE_LONG_PRESS		= 0x12,
	SPONGE_EVENT_TYPE_TSP_SCAN_UNBLOCK	= 0xE1,
	SPONGE_EVENT_TYPE_TSP_SCAN_BLOCK	= 0xE2,
};

enum proximity_event_type {
	PROX_EVENT_TYPE_NP_FAR			= 0,
	PROX_EVENT_TYPE_NP_SENSITIVE_CLOSE	= 1,
	PROX_EVENT_TYPE_NP_ENOUGH_CLOSE		= 2,
	PROX_EVENT_TYPE_NP_STRONG_CLOSE		= 3,
	PROX_EVENT_TYPE_LP_CLOSE		= 4,
	PROX_EVENT_TYPE_LP_FAR			= 5,

	PROX_EVENT_TYPE_POCKET_RELEASE		= 10,
	PROX_EVENT_TYPE_POCKET_PRESS		= 11,

	PROX_EVENT_TYPE_LIGHTSENSOR_RELEASE	= 20,
	PROX_EVENT_TYPE_LIGHTSENSOR_PRESS	= 21,
};

/* SEC_TS_DEBUG : Print event contents */
#define SEC_TS_DEBUG_PRINT_ALLEVENT  0x01
#define SEC_TS_DEBUG_PRINT_ONEEVENT  0x02
#define SEC_TS_DEBUG_PRINT_READ_CMD  0x04
#define SEC_TS_DEBUG_PRINT_WRITE_CMD 0x08

#define CMD_RESULT_WORD_LEN		10

#define SEC_TS_I2C_RETRY_CNT		3
#define SEC_TS_WAIT_RETRY_CNT		100

#define SEC_TS_LOCATION_DETECT_SIZE		6
#define SEC_TS_SUPPORT_TOUCH_COUNT		10
#define SEC_TS_GESTURE_REPORT_BUFF_SIZE		20

/* SPONGE MODE 0x00 */
#define SEC_TS_MODE_SPONGE_SWIPE		(1 << 1)
#define SEC_TS_MODE_SPONGE_AOD			(1 << 2)
#define SEC_TS_MODE_SPONGE_SINGLE_TAP		(1 << 3)
#define SEC_TS_MODE_SPONGE_PRESS		(1 << 4)
#define SEC_TS_MODE_SPONGE_DOUBLETAP_TO_WAKEUP	(1 << 5)
#define SEC_TS_MODE_SPONGE_TWO_FINGER_DOUBLETAP	(1 << 7)
#define SEC_TS_MODE_SPONGE_VVC			(1 << 8)

/* SPONGE MODE 0x01 */
#define SEC_TS_MODE_SPONGE_INF_DUMP_CLEAR	(1 << 0)
#define SEC_TS_MODE_SPONGE_INF_DUMP		(1 << 1)
#define SEC_TS_MODE_SPONGE_AOD_NOTI_RECT_AOT	(1 << 2)

/*SPONGE library parameters*/
#define SEC_TS_MAX_SPONGE_DUMP_BUFFER	512
#define SEC_TS_SPONGE_DUMP_EVENT_MASK	0x7F
#define SEC_TS_SPONGE_DUMP_INF_MASK	0x80
#define SEC_TS_SPONGE_DUMP_INF_SHIFT	7

/* SEC_TS SPONGE OPCODE COMMAND */
#define SEC_TS_CMD_SPONGE_FEATURE_ENABLE_0			0x00
#define SEC_TS_CMD_SPONGE_FEATURE_ENABLE_1			0x01
#define SEC_TS_CMD_SPONGE_DUMP_FLUSH			SEC_TS_CMD_SPONGE_FEATURE_ENABLE_1 //will be removed.
#define SEC_TS_CMD_SPONGE_AOD_RECT			0x02
#define SEC_TS_CMD_SPONGE_AOD_ACTIVE_INFO		0x0A
#define SEC_TS_CMD_SPONGE_OFFSET_UTC			0x10
#define SEC_TS_CMD_SPONGE_PRESS_PROPERTY		0x14
#define SEC_TS_CMD_SPONGE_FOD_INFO			0x15
#define SEC_TS_CMD_SPONGE_FOD_POSITION			0x19
#define SEC_TS_CMD_SPONGE_AOD_NOTI_RECT			0x53
#define SEC_TS_CMD_SPONGE_GET_INFO			0x90
#define SEC_TS_CMD_SPONGE_WRITE_PARAM			0x91
#define SEC_TS_CMD_SPONGE_READ_PARAM			0x92
#define SEC_TS_CMD_SPONGE_NOTIFY_PACKET			0x93
#define SEC_TS_CMD_SPONGE_LP_DUMP			0xF0
#define SEC_TS_CMD_SPONGE_LP_DUMP_CUR_IDX		0xF2
#define SEC_TS_CMD_SPONGE_LP_DUMP_EVENT			0xF4

#define SEC_INPUT_DEFAULT_AVDD_NAME	"tsp_avdd_ldo"
#define SEC_INPUT_DEFAULT_DVDD_NAME	"tsp_io_ldo"

#define SEC_TS_MAX_FW_PATH		64
#define TSP_EXTERNAL_FW			"tsp.bin"
#define TSP_EXTERNAL_FW_SIGNED		"tsp_signed.bin"
#define TSP_SPU_FW_SIGNED		"/TSP/ffu_tsp.bin"

enum fw_version_index {
	SEC_INPUT_FW_IC_VER = 0,
	SEC_INPUT_FW_VER_PROJECT_ID,
	SEC_INPUT_FW_MODULE_VER,
	SEC_INPUT_FW_VER,
	SEC_INPUT_FW_INDEX_MAX,
};

enum bringup_fw_update_condition {
	BRINGUP_NONE = 0,
	BRINGUP_SKIP_FW_UPDATE = 1,
	BRINGUP_SKIP_FW_UPDATE_WITH_REQUEST_FW = 2,
	BRINGUP_FW_UPDATE_WHEN_VERSION_MISMATCH = 3,
	BRINGUP_FW_UPDATE_ALWAYS = 5,
};

#if IS_ENABLED(CONFIG_SEC_ABC)
#define SEC_ABC_SEND_EVENT_TYPE "MODULE=tsp@WARN=tsp_int_fault"
#define SEC_ABC_SEND_EVENT_TYPE_SUB "MODULE=tsp_sub@WARN=tsp_int_fault"
#define SEC_ABC_SEND_EVENT_TYPE_WACOM_DIGITIZER_NOT_CONNECTED "MODULE=wacom@WARN=digitizer_not_connected"
#endif

enum display_state {
	DISPLAY_STATE_SERVICE_SHUTDOWN = -1,
	DISPLAY_STATE_NONE = 0,
	DISPLAY_STATE_OFF,
	DISPLAY_STATE_ON,
	DISPLAY_STATE_DOZE,
	DISPLAY_STATE_DOZE_SUSPEND,
	DISPLAY_STATE_LPM_OFF = 20,
	DISPLAY_STATE_FORCE_OFF,
	DISPLAY_STATE_FORCE_ON,
};

enum display_event {
	DISPLAY_EVENT_EARLY = 0,
	DISPLAY_EVENT_LATE,
};
/*If you want to add mode, please check "MODE_TO_CHECK_BIT" as well.*/
enum power_mode {
	SEC_INPUT_STATE_POWER_OFF = 0,
	SEC_INPUT_STATE_LPM,
	SEC_INPUT_STATE_POWER_ON
};

#define CHECK_POWEROFF		(1 << SEC_INPUT_STATE_POWER_OFF)
#define CHECK_LPMODE		(1 << SEC_INPUT_STATE_LPM)
#define CHECK_POWERON		(1 << SEC_INPUT_STATE_POWER_ON)
#define CHECK_ON_LP		(CHECK_POWERON | CHECK_LPMODE)
#define CHECK_ALL		(CHECK_POWERON | CHECK_LPMODE | CHECK_POWEROFF)
#define MODE_TO_CHECK_BIT(x)	(1 << x)

struct sec_input_ic_mode_status {
	union {
		struct {
			u8 call:1;
			u8 pocket:1;
			u8 lowsen:1;
			u8 sip:1;
			u8 wireless:1;
			u8 ta:1;
			u8 note:1;
			u8 cover:1;
			u8 noise:1;
			u8 game:1;
			u8 glove:1;
			u8 wet:1;
			u8 reserved:4;
		} __packed;
		unsigned char data[2];
	};
};

enum switch_system_mode {
	TO_TOUCH_MODE			= 0,
	TO_LOWPOWER_MODE		= 1,
};

/*
 * sec_cover_state need to match with CoverState class of frameworks
 */
enum sec_cover_type {
	SEC_COVER_TYPE_FLIP_COVER		= 0,
	SEC_COVER_TYPE_SVIEW_COVER		= 1,
	SEC_COVER_TYPE_NONE			= 2,
	SEC_COVER_TYPE_SVIEW_CHARGER_COVER	= 3,
	SEC_COVER_TYPE_HEALTH_COVER		= 4,
	SEC_COVER_TYPE_S_CHARGER_COVER		= 5,
	SEC_COVER_TYPE_S_VIEW_WALLET_COVER	= 6,
	SEC_COVER_TYPE_LED_COVER		= 7,
	SEC_COVER_TYPE_CLEAR_COVER		= 8,
	SEC_COVER_TYPE_KEYBOARD_KOR_COVER	= 9,

	SEC_COVER_TYPE_KEYBOARD_US_COVER	= 10,
	SEC_COVER_TYPE_NEON_COVER		= 11,
	SEC_COVER_TYPE_ALCANTARA_COVER		= 12,
	SEC_COVER_TYPE_GAMEPACK_COVER		= 13,
	SEC_COVER_TYPE_LED_BACK_COVER		= 14,
	SEC_COVER_TYPE_CLEAR_SIDE_VIEW_COVER	= 15,
	SEC_COVER_TYPE_MINI_SVIEW_WALLET_COVER	= 16,
	SEC_COVER_TYPE_CLEAR_CAMERA_VIEW_COVER	= 17,

	SEC_COVER_TYPE_MONTBLANC_COVER		= 100,

	SEC_COVER_TYPE_NFC_SMART_COVER		= 255,
};

#define TEST_MODE_MIN_MAX		false
#define TEST_MODE_ALL_NODE		true
#define TEST_MODE_READ_FRAME		false
#define TEST_MODE_READ_CHANNEL		true

enum offset_fac_position {
	OFFSET_FAC_NOSAVE		= 0,	// FW index 0
	OFFSET_FAC_SUB			= 1,	// FW Index 2
	OFFSET_FAC_MAIN			= 2,	// FW Index 3
	OFFSET_FAC_SVC			= 3,	// FW Index 4
};

enum offset_fw_position {
	OFFSET_FW_NOSAVE		= 0,
	OFFSET_FW_SDC			= 1,
	OFFSET_FW_SUB			= 2,
	OFFSET_FW_MAIN			= 3,
	OFFSET_FW_SVC			= 4,
};

enum offset_fac_data_type {
	OFFSET_FAC_DATA_NO			= 0,
	OFFSET_FAC_DATA_CM			= 1,
	OFFSET_FAC_DATA_CM2			= 2,
	OFFSET_FAC_DATA_CM3			= 3,
	OFFSET_FAC_DATA_MISCAL			= 5,
	OFFSET_FAC_DATA_SELF_FAIL	= 7,
};

enum sec_input_notify_t {
	NOTIFIER_NOTHING = 0,
	NOTIFIER_MAIN_TOUCH_ON,
	NOTIFIER_MAIN_TOUCH_OFF,
	NOTIFIER_SUB_TOUCH_ON,
	NOTIFIER_SUB_TOUCH_OFF,
	NOTIFIER_SECURE_TOUCH_ENABLE,		/* secure touch is enabled */
	NOTIFIER_SECURE_TOUCH_DISABLE,		/* secure touch is disabled */
	NOTIFIER_TSP_BLOCKING_REQUEST,		/* wacom called tsp block enable */
	NOTIFIER_TSP_BLOCKING_RELEASE,		/* wacom called tsp block disable */
	NOTIFIER_WACOM_PEN_CHARGING_FINISHED,	/* to tsp: pen charging finished */
	NOTIFIER_WACOM_PEN_CHARGING_STARTED,	/* to tsp: pen charging started */
	NOTIFIER_WACOM_PEN_INSERT,		/* to tsp: pen is inserted */
	NOTIFIER_WACOM_PEN_REMOVE,		/* to tsp: pen is removed */
	NOTIFIER_WACOM_PEN_HOVER_IN,		/* to tsp: pen hover is in */
	NOTIFIER_WACOM_PEN_HOVER_OUT,		/* to tsp: pen hover is out */
	NOTIFIER_LCD_VRR_LFD_LOCK_REQUEST,	/* to LCD: set LFD min lock */
	NOTIFIER_LCD_VRR_LFD_LOCK_RELEASE,	/* to LCD: unset LFD min lock */
	NOTIFIER_LCD_VRR_LFD_OFF_REQUEST,	/* to LCD: set LFD OFF */
	NOTIFIER_LCD_VRR_LFD_OFF_RELEASE,	/* to LCD: unset LFD OFF */
	NOTIFIER_TSP_ESD_INTERRUPT,
	NOTIFIER_WACOM_SAVING_MODE_ON,		/* to tsp: multi spen enable cmd on */
	NOTIFIER_WACOM_SAVING_MODE_OFF,		/* to tsp: multi spen enable cmd off */
	NOTIFIER_WACOM_KEYBOARDCOVER_FLIP_OPEN,
	NOTIFIER_WACOM_KEYBOARDCOVER_FLIP_CLOSE,
	NOTIFIER_VALUE_MAX,
};

enum notify_set_scan_mode {
	DISABLE_TSP_SCAN_BLOCK		= 0,
	ENABLE_TSP_SCAN_BLOCK		= 1,
	ENABLE_SPEN_CHARGING_MODE		= 2,
	DISABLE_SPEN_CHARGING_MODE		= 3,
	ENABLE_SPEN_IN		= 4,
	ENABLE_SPEN_OUT		= 5,
};

enum coord_action {
	SEC_TS_COORDINATE_ACTION_NONE = 0,
	SEC_TS_COORDINATE_ACTION_PRESS = 1,
	SEC_TS_COORDINATE_ACTION_MOVE = 2,
	SEC_TS_COORDINATE_ACTION_RELEASE = 3,
	SEC_TS_COORDINATE_ACTION_FORCE_RELEASE = 4,
};

#define SEC_TS_LFD_CTRL_LOCK_TIME	500	/* msec */
#define SEC_TS_WAKE_LOCK_TIME		500	/* msec */

enum lfd_lock_ctrl {
	SEC_TS_LFD_CTRL_LOCK = 0,
	SEC_TS_LFD_CTRL_UNLOCK,
};

enum notify_tsp_type {
	MAIN_TOUCHSCREEN = 0,
	SUB_TOUCHSCREEN,
};

enum sec_ts_error {
	SEC_ERROR = -1,
	SEC_SUCCESS = 0,
	SEC_SKIP = 1,
	SEC_FORCE = 2,
};

struct sec_input_grip_data {
	u8 edgehandler_direction;
	int edgehandler_start_y;
	int edgehandler_end_y;
	u16 edge_range;
	u8 deadzone_up_x;
	u8 deadzone_dn_x;
	int deadzone_y;
	u8 deadzone_dn2_x;
	int deadzone_dn_y;
	u8 landscape_mode;
	int landscape_edge;
	u16 landscape_deadzone;
	u16 landscape_top_deadzone;
	u16 landscape_bottom_deadzone;
	u16 landscape_top_gripzone;
	u16 landscape_bottom_gripzone;
};

struct sec_input_notify_data {
	u8 dual_policy;
};

struct sec_ts_coordinate {
	u8 id;
	u8 ttype;
	u8 action;
	u16 x;
	u16 y;
	u16 p_x;
	u16 p_y;
	u8 z;
	u8 hover_flag;
	u8 glove_flag;
	u8 touch_height;
	u16 mcount;
	u8 major;
	u8 minor;
	bool palm;
	int palm_count;
	u8 left_event;
	u8 noise_level;
	u8 max_strength;
	u8 hover_id_num;
	u8 noise_status;
	u8 freq_id;
	u8 fod_debug;
};

struct sec_ts_aod_data {
	u16 rect_data[4];
	u16 active_area[3];
};

struct sec_ts_fod_data {
	bool set_val;
	u16 rect_data[4];

	u8 vi_x;
	u8 vi_y;
	u8 vi_size;
	u8 vi_data[SEC_CMD_STR_LEN];
	u8 vi_event;
	u8 press_prop;
};

enum sec_ts_fod_vi_method {
	CMD_SYSFS = 0,
	CMD_IRQ = 1,
};

#define SEC_INPUT_HW_PARAM_SIZE		512

struct sec_ts_hw_param_data {
	unsigned char ito_test[4];
	bool check_multi;
	unsigned int multi_count;
	unsigned int wet_count;
	unsigned int noise_count;
	unsigned int comm_err_count;
	unsigned int checksum_result;
	unsigned int all_finger_count;
	unsigned int all_aod_tap_count;
	unsigned int all_spay_count;
	int mode_change_failed_count;
	int ic_reset_count;
};

struct sec_ts_plat_data {
	struct device *dev;

	struct input_dev *input_dev;
	struct input_dev *input_dev_pad;
	struct input_dev *input_dev_proximity;

	struct sec_input_multi_device *multi_dev;

	int max_x;
	int max_y;
	int x_node_num;
	int y_node_num;
	int custom_rawdata_size;

	unsigned int irq_gpio;
	int gpio_spi_cs;
	int i2c_burstmax;
	int bringup;
	u32 area_indicator;
	u32 area_navigation;
	u32 area_edge;
	char location[SEC_TS_LOCATION_DETECT_SIZE];

	u8 prox_power_off;
	bool power_enabled;

	struct sec_ts_coordinate coord[SEC_TS_SUPPORT_TOUCH_COUNT];
	struct sec_ts_coordinate prev_coord[SEC_TS_SUPPORT_TOUCH_COUNT];
	bool fill_slot;

	int touch_count;
	unsigned int palm_flag;
	bool blocking_palm;
	atomic_t touch_noise_status;
	atomic_t touch_pre_noise_status;

	struct sec_ts_fod_data fod_data;
	struct sec_ts_aod_data aod_data;
	struct sec_ts_aod_data aod_noti_data;

	const char *firmware_name;
	struct regulator *dvdd;
	struct regulator *avdd;

	char ic_vendor_name[3];
	u8 img_version_of_ic[SEC_INPUT_FW_INDEX_MAX];
	u8 img_version_of_bin[SEC_INPUT_FW_INDEX_MAX];

	struct pinctrl *pinctrl;

	int (*pinctrl_configure)(struct device *dev, bool on);
	int (*power)(struct device *dev, bool on);
	int (*start_device)(void *data);
	int (*stop_device)(void *data);
	int (*lpmode)(void *data, u8 mode);
	void (*init)(void *data);
	int (*stui_tsp_enter)(void);
	int (*stui_tsp_exit)(void);
	int (*stui_tsp_type)(void);
	int (*probe)(struct device *dev);

	int (*enable)(struct device *dev);
	int (*disable)(struct device *dev);
	struct mutex enable_mutex;

	u8 *external_firmware_data;
	int external_firmware_size;

	union power_supply_propval psy_value;
	struct power_supply *psy;
	u8 tsp_temperature_data;
	bool tsp_temperature_data_skip;
	int (*set_temperature)(struct device *dev, u8 temperature_data);

	struct sec_input_grip_data grip_data;
	void (*set_grip_data)(struct device *dev, u8 flag);

	atomic_t enabled;
	atomic_t power_state;
	atomic_t shutdown_called;

	u16 touch_functions;
	u16 ic_status;
	u8 lowpower_mode;
	u8 sponge_mode;
	u8 external_noise_mode;
	u8 touchable_area;
	u8 ed_enable;
	u8 pocket_mode;
	u8 lightsensor_detect;
	u8 fod_lp_mode;
	int cover_type;
	u8 wirelesscharger_mode;
	bool force_wirelesscharger_mode;
	int wet_mode;
	int noise_mode; /* for debug app */
	int freq_id; /* for debug app */
	int low_sensitivity_mode;
	struct sec_input_ic_mode_status ic_mode_status;

	bool regulator_boot_on;
	bool support_dex;
	bool support_ear_detect;

	atomic_t secure_enabled;
	atomic_t secure_pending_irqs;
	struct completion secure_powerdown;
	struct completion secure_interrupt;
	int ss_touch_num;
#if IS_ENABLED(CONFIG_INPUT_SEC_TRUSTED_TOUCH)
	struct sec_trusted_touch *pvm;
#endif
#define SEC_INPUT_IRQ_ENABLE			0
#define SEC_INPUT_IRQ_DISABLE			1
#define SEC_INPUT_IRQ_DISABLE_NOSYNC		2
	int irq;
	atomic_t irq_enabled;
	struct mutex irq_lock;

	struct device *bus_master;

	bool support_fod;
	bool support_fod_lp_mode;
	bool support_fod_control;
	bool enable_settings_aot;
	bool support_refresh_rate_mode;
	bool support_vrr;
	bool support_open_short_test;
	bool support_mis_calibration_test;
	bool support_wireless_tx;
	bool support_lightsensor_detect;
	bool support_input_monitor;
	int support_sensor_hall;
	bool chip_on_board;
	bool enable_sysinput_enabled;
	bool support_rawdata;
	bool support_rawdata_motion_aivf;
	bool support_rawdata_motion_palm;
	bool support_rawdata_pocket_detect;
	bool support_rawdata_awd;
	bool not_support_io_ldo;
	bool not_support_vdd;
	bool not_support_temp_noti;
	bool support_vbus_notifier;
	bool support_always_on;
	bool prox_lp_scan_enabled;
	bool support_self_rawdata;

	int always_lpm;

	bool work_queue_probe_enabled;
	bool first_booting_disabled;
	int work_queue_probe_delay;
	struct work_struct probe_work;
	struct workqueue_struct *probe_workqueue;

	struct work_struct irq_work;
	struct workqueue_struct *irq_workqueue;
	struct completion resume_done;
	struct wakeup_source *sec_ws;

	struct sec_ts_hw_param_data hw_param;

	struct delayed_work interrupt_notify_work;

	int (*set_charger_mode)(struct device *dev, bool on);
	bool charger_flag;
	struct work_struct vbus_notifier_work;
	struct workqueue_struct *vbus_notifier_workqueue;
	struct notifier_block vbus_nb;
	struct notifier_block ccic_nb;
	bool otg_flag;
	bool vbus_notifier_init_flag;

	u32 print_info_cnt_release;
	u32 print_info_cnt_open;
};

struct sec_ts_external_api_func {
	struct power_supply *(*power_supply_get_by_name)(const char *name);
	int (*power_supply_get_property)(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val);

	struct input_dev *(*devm_input_allocate_device)(struct device *dev);
	int (*input_register_device)(struct input_dev *dev);
	void (*input_sync)(struct input_dev *dev);
};

struct sec_ts_secure_data {
	int (*stui_tsp_enter)(void);
	int (*stui_tsp_exit)(void);
	int (*stui_tsp_type)(void);
};

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
extern int get_lcd_attached(char *mode);
#endif

#if IS_ENABLED(CONFIG_EXYNOS_DPU30) || IS_ENABLED(CONFIG_MCD_PANEL) || IS_ENABLED(CONFIG_USDM_PANEL)
extern int get_lcd_info(char *arg);
#endif

#if IS_ENABLED(CONFIG_SMCDSD_PANEL)
extern unsigned int lcdtype;
#endif

extern struct sec_ts_external_api_func efunc;

/* input.c */
bool sec_input_cmp_ic_status(struct device *dev, int check_bit);
ssize_t sec_input_get_common_hw_param(struct sec_ts_plat_data *pdata, char *buf);
void sec_input_clear_common_hw_param(struct sec_ts_plat_data *pdata);
int sec_input_power(struct device *dev, bool on);
void sec_input_print_info(struct device *dev, struct sec_tclm_data *tdata);
int sec_input_pinctrl_configure(struct device *dev, bool on);
void sec_input_unregister_vbus_notifier(struct device *dev);
void sec_input_register_vbus_notifier(struct device *dev);
#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
int sec_input_ccic_notification(struct notifier_block *nb, unsigned long action, void *data);
#endif
int sec_input_vbus_notification(struct notifier_block *nb, unsigned long cmd, void *data);
void sec_input_vbus_notification_work(struct work_struct *work);
#endif
void sec_input_probe_work_remove(struct sec_ts_plat_data *pdata);
void sec_input_probe_work(struct work_struct *work);
int sec_input_enable_device(struct device *dev);
int sec_input_disable_device(struct device *dev);
void sec_input_utc_marker(struct device *dev, const char *annotation);
bool sec_input_need_ic_off(struct sec_ts_plat_data *pdata);
void sec_delay(unsigned int ms);
bool sec_check_secure_trusted_mode_status(struct sec_ts_plat_data *pdata);
bool sec_input_need_fw_update(struct sec_ts_plat_data *pdata);
#if IS_ENABLED(CONFIG_SEC_DEBUG_TSP_LOG)
void sec_input_log(bool mode, struct device *dev, char *fmt, ...);
void sec_input_fail_hist_log(bool mode, struct device *dev, char *fmt, ...);
void sec_input_raw_info_log(int dev_count, struct device *dev, char *fmt, ...);
#endif

/* input_parsedt.c */
int sec_input_parse_dt(struct device *dev);
void sec_tclm_parse_dt(struct device *dev, struct sec_tclm_data *tdata);
int sec_input_get_lcd_id(struct device *dev);
int sec_input_multi_device_parse_dt(struct device *dev);
int sec_input_lcd_parse_panel_id(char *panel_id);
int sec_input_check_fw_name_incell(struct device *dev, const char **firmware_name, const char **firmware_name_2nd);
void sec_input_support_feature_parse_dt(struct device *dev);

/* input_irq.c */
enum sec_ts_error sec_input_handler_start(struct device *dev);
void sec_input_handler_wait_resume_work(struct work_struct *work);
void sec_input_irq_enable(struct sec_ts_plat_data *pdata);
void sec_input_irq_disable(struct sec_ts_plat_data *pdata);
void sec_input_irq_disable_nosync(struct sec_ts_plat_data *pdata);
/* input_report */
int sec_input_device_register(struct device *dev, void *data);
void sec_input_coord_event_sync_slot(struct device *dev);
void sec_input_proximity_report(struct device *dev, int data);
void sec_input_release_all_finger(struct device *dev);
void sec_input_coord_event_fill_slot(struct device *dev, int t_id);
void sec_input_coord_report(struct device *dev, u8 t_id);
void sec_input_coord_log(struct device *dev, u8 t_id, int action);
void location_detect(struct sec_ts_plat_data *pdata, int t_id);

/* input_stuid.c */
void stui_tsp_init(int (*stui_tsp_enter)(void), int (*stui_tsp_exit)(void), int (*stui_tsp_type)(void));
int stui_tsp_enter(void);
int stui_tsp_exit(void);
int stui_tsp_type(void);

/* input_ic_settings.c */
enum sec_ts_error sec_input_set_temperature(struct device *dev, int state);
void sec_input_set_grip_type(struct device *dev, u8 set_type);
int sec_input_store_grip_data(struct device *dev, int *cmd_param);
void sec_input_set_fod_info(struct device *dev, int vi_x, int vi_y, int vi_size, int vi_event);
ssize_t sec_input_get_fod_info(struct device *dev, char *buf);
bool sec_input_set_fod_rect(struct device *dev, int *rect_data);
enum sec_ts_error sec_input_check_wirelesscharger_mode(struct device *dev, enum wireless_charger_param mode, int force);

/* input_notifier */
void sec_input_register_notify(struct notifier_block *nb, notifier_fn_t notifier_call, int priority);
void sec_input_unregister_notify(struct notifier_block *nb);
int sec_input_notify(struct notifier_block *nb, unsigned long noti, void *v);
int sec_input_self_request_notify(struct notifier_block *nb);

#if IS_ENABLED(CONFIG_UML)
/*ic_setting*/
enum sec_ts_error sec_input_set_temperature_data(struct device *dev, int force);
DECLARE_REDIRECT_MOCKABLE(sec_input_temperature_state_check,
			RETURNS(enum sec_ts_error),
			PARAMS(struct device *, enum set_temperature_state));
DECLARE_REDIRECT_MOCKABLE(sec_input_get_power_supply, RETURNS(enum sec_ts_error), PARAMS(struct device *));
int sec_input_store_grip_data_edge(struct sec_ts_plat_data *pdata, int *cmd_param);
int sec_input_store_grip_data_portrait(struct sec_ts_plat_data *pdata, int *cmd_param);
int sec_input_store_grip_data_landscape(struct sec_ts_plat_data *pdata, int *cmd_param);

/*report.c*/
int sec_input_device_init(struct device *dev, enum input_device_type type, void *data);
void sec_input_set_prop(struct device *dev, struct input_dev *input_dev, enum input_device_type type, void *data);
void sec_input_set_prop_touch(struct device *dev, struct input_dev *input_dev, u8 propbit, void *data);
void sec_input_set_prop_proximity(struct device *dev, struct input_dev *input_dev, void *data);


#endif
#endif
