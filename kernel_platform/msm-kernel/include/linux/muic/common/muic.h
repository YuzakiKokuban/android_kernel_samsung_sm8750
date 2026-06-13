// SPDX-License-Identifier: GPL-2.0
/*
 * include/linux/muic/common/muic.h
 *
 * header file supporting MUIC common information
 *
 * Copyright (C) 2022 Samsung Electronics
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __MUIC_H__
#define __MUIC_H__

#ifdef CONFIG_IFCONN_NOTIFIER
#include <linux/ifconn/ifconn_notifier.h>
#endif
#include <linux/muic/common/muic_param.h>
#include <linux/power_supply.h>

#define MUIC_CORE "MUIC_CORE"
#define SIOP (1 << 0)
#define AFC_REQUEST_CHARGER SIOP
#define FLED (1 << 1)
#define AFC_REQUEST_MST (1 << 2)
#define AFC_REQUEST_MFC (1 << 3)
#define AFC_REQUEST_DETACH_CLEAR_BIT ((SIOP))
/* Status of IF PMIC chip (suspend and resume) */
enum {
	MUIC_SUSPEND		= 0,
	MUIC_RESUME,
};

/* MUIC Interrupt */
enum {
	MUIC_INTR_DETACH	= 0,
	MUIC_INTR_ATTACH
};

enum muic_op_mode {
	OPMODE_MUIC = 0<<0,
	OPMODE_PDIC = 1<<0,
};

/* MUIC Dock Observer Callback parameter */
enum {
	MUIC_DOCK_DETACHED	= 0,
	MUIC_DOCK_DESKDOCK	= 1,
	MUIC_DOCK_CARDOCK	= 2,
	MUIC_DOCK_AUDIODOCK	= 101,
	MUIC_DOCK_SMARTDOCK	= 102,
	MUIC_DOCK_HMT		= 105,
	MUIC_DOCK_ABNORMAL	= 106,
	MUIC_DOCK_GAMEPAD	= 107,
	MUIC_DOCK_GAMEPAD_WITH_EARJACK	= 108,
};

/* MUIC Path */
enum {
	MUIC_PATH_USB_AP	= 0,
	MUIC_PATH_USB_CP,
	MUIC_PATH_UART_AP,
	MUIC_PATH_UART_CP,
	MUIC_PATH_OPEN,
	MUIC_PATH_AUDIO,
};

/* bootparam SWITCH_SEL */
enum {
	SWITCH_SEL_USB_MASK	= 0x1,
	SWITCH_SEL_UART_MASK	= 0x2,
	SWITCH_SEL_RUSTPROOF_MASK	= 0x8,
	SWITCH_SEL_AFC_DISABLE_MASK	= 0x100,
};

/* bootparam CHARGING_MODE */
enum {
	CH_MODE_AFC_DISABLE_VAL = 0x31, /* char '1' */
};

enum driver_probe_flag {
	MUIC_PROBE_DONE = 1 << 0,
	CHARGER_PROBE_DONE = 1 << 1,
};

/* MUIC ADC table */
typedef enum {
	ADC_GND			= 0x00,
	ADC_SEND_END		= 0x01, /* 0x00001 2K ohm */
	ADC_REMOTE_S11		= 0x0c, /* 0x01100 20.5K ohm */
	ADC_REMOTE_S12		= 0x0d, /* 0x01101 24.07K ohm */
	ADC_RESERVED_VZW	= 0x0e, /* 0x01110 28.7K ohm */
	ADC_INCOMPATIBLE_VZW	= 0x0f, /* 0x01111 34K ohm */
	ADC_SMARTDOCK		= 0x10, /* 0x10000 40.2K ohm */
	ADC_RDU_TA		= 0x10, /* 0x10000 40.2K ohm */
	ADC_HMT			= 0x11, /* 0x10001 49.9K ohm */
	ADC_AUDIODOCK		= 0x12, /* 0x10010 64.9K ohm */
	ADC_USB_LANHUB		= 0x13, /* 0x10011 80.07K ohm */
	ADC_CHARGING_CABLE	= 0x14,	/* 0x10100 102K ohm */
	ADC_UNIVERSAL_MMDOCK	= 0x15, /* 0x10101 121K ohm */
	ADC_GAMEPAD		= 0x15, /* 0x10101 121K ohm */
	ADC_UART_CABLE		= 0x16, /* 0x10110 150K ohm */
	ADC_CEA936ATYPE1_CHG	= 0x17,	/* 0x10111 200K ohm */
	ADC_JIG_USB_OFF		= 0x18, /* 0x11000 255K ohm */
	ADC_JIG_USB_ON		= 0x19, /* 0x11001 301K ohm */
	ADC_DESKDOCK		= 0x1a, /* 0x11010 365K ohm */
	ADC_CEA936ATYPE2_CHG	= 0x1b, /* 0x11011 442K ohm */
	ADC_JIG_UART_OFF	= 0x1c, /* 0x11100 523K ohm */
	ADC_JIG_UART_ON		= 0x1d, /* 0x11101 619K ohm */
	ADC_AUDIOMODE_W_REMOTE	= 0x1e, /* 0x11110 1000K ohm */
	ADC_OPEN		= 0x1f,
	ADC_OPEN_219		= 0xfb, /* ADC open or 219.3K ohm */
	ADC_219			= 0xfc, /* ADC open or 219.3K ohm */

	ADC_UNDEFINED		= 0xfd, /* Undefied range */
	ADC_DONTCARE		= 0xfe, /* ADC don't care for MHL */
	ADC_ERROR		= 0xff, /* ADC value read error */
} muic_adc_t;

#define IS_JIG_ADC(adc) \
	(((adc == ADC_JIG_USB_OFF) \
	|| (adc == ADC_JIG_USB_ON) \
	|| (adc == ADC_JIG_UART_OFF) \
	|| (adc == ADC_JIG_UART_ON)) ? 1 : 0)

#define ADC_WATER_THRESHOLD	ADC_OPEN

/* MUIC attached device type */
typedef enum {
	ATTACHED_DEV_NONE_MUIC = 0,

	ATTACHED_DEV_USB_MUIC = 1,
	ATTACHED_DEV_CDP_MUIC,
	ATTACHED_DEV_OTG_MUIC,
	ATTACHED_DEV_TA_MUIC,
	ATTACHED_DEV_UNOFFICIAL_MUIC,
	ATTACHED_DEV_UNOFFICIAL_TA_MUIC,
	ATTACHED_DEV_UNOFFICIAL_ID_MUIC,
	ATTACHED_DEV_UNOFFICIAL_ID_TA_MUIC,
	ATTACHED_DEV_UNOFFICIAL_ID_ANY_MUIC,
	ATTACHED_DEV_UNOFFICIAL_ID_USB_MUIC,

	ATTACHED_DEV_UNOFFICIAL_ID_CDP_MUIC = 11,
	ATTACHED_DEV_UNDEFINED_CHARGING_MUIC,
	ATTACHED_DEV_DESKDOCK_MUIC,
	ATTACHED_DEV_UNKNOWN_VB_MUIC,
	ATTACHED_DEV_DESKDOCK_VB_MUIC,
	ATTACHED_DEV_CARDOCK_MUIC,
	ATTACHED_DEV_JIG_UART_OFF_MUIC,
	ATTACHED_DEV_JIG_UART_OFF_VB_MUIC,	/* VBUS enabled */
	ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC,	/* for otg test */
	ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC,	/* for fuelgauge test */

	ATTACHED_DEV_JIG_UART_ON_MUIC = 21,
	ATTACHED_DEV_JIG_UART_ON_VB_MUIC,	/* VBUS enabled */
	ATTACHED_DEV_JIG_USB_OFF_MUIC,
	ATTACHED_DEV_JIG_USB_ON_MUIC,
	ATTACHED_DEV_SMARTDOCK_MUIC,
	ATTACHED_DEV_SMARTDOCK_VB_MUIC,
	ATTACHED_DEV_SMARTDOCK_TA_MUIC,
	ATTACHED_DEV_SMARTDOCK_USB_MUIC,
	ATTACHED_DEV_UNIVERSAL_MMDOCK_MUIC,
	ATTACHED_DEV_AUDIODOCK_MUIC,

	ATTACHED_DEV_MHL_MUIC = 31,
	ATTACHED_DEV_CHARGING_CABLE_MUIC,
	ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC,
	ATTACHED_DEV_AFC_CHARGER_PREPARE_DUPLI_MUIC,
	ATTACHED_DEV_AFC_CHARGER_5V_MUIC,
	ATTACHED_DEV_AFC_CHARGER_5V_DUPLI_MUIC,
	ATTACHED_DEV_AFC_CHARGER_9V_MUIC,
	ATTACHED_DEV_AFC_CHARGER_9V_DUPLI_MUIC,
	ATTACHED_DEV_AFC_CHARGER_12V_MUIC,
	ATTACHED_DEV_AFC_CHARGER_12V_DUPLI_MUIC,

	ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC = 41,
	ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC,
	ATTACHED_DEV_AFC_CHARGER_DISABLED_MUIC,
	ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC,
	ATTACHED_DEV_QC_CHARGER_5V_MUIC,
	ATTACHED_DEV_QC_CHARGER_ERR_V_MUIC,
	ATTACHED_DEV_QC_CHARGER_9V_MUIC,
	ATTACHED_DEV_HV_ID_ERR_UNDEFINED_MUIC,
	ATTACHED_DEV_HV_ID_ERR_UNSUPPORTED_MUIC,
	ATTACHED_DEV_HV_ID_ERR_SUPPORTED_MUIC,

	ATTACHED_DEV_HMT_MUIC = 51,
	ATTACHED_DEV_VZW_ACC_MUIC,
	ATTACHED_DEV_VZW_INCOMPATIBLE_MUIC,
	ATTACHED_DEV_USB_LANHUB_MUIC,
	ATTACHED_DEV_TYPE1_CHG_MUIC,
	ATTACHED_DEV_TYPE2_CHG_MUIC,
	ATTACHED_DEV_TYPE3_MUIC,
	ATTACHED_DEV_TYPE3_MUIC_TA,
	ATTACHED_DEV_TYPE3_ADAPTER_MUIC,
	ATTACHED_DEV_TYPE3_CHARGER_MUIC,

	ATTACHED_DEV_NONE_TYPE3_MUIC = 61,
	ATTACHED_DEV_UNSUPPORTED_ID_MUIC,
	ATTACHED_DEV_UNSUPPORTED_ID_VB_MUIC,
	ATTACHED_DEV_TIMEOUT_OPEN_MUIC,
	ATTACHED_DEV_WIRELESS_PAD_MUIC,
	ATTACHED_DEV_CARKIT_MUIC,
	ATTACHED_DEV_POWERPACK_MUIC,
	ATTACHED_DEV_UNDEFINED_RANGE_MUIC,
	ATTACHED_DEV_HICCUP_MUIC,
	ATTACHED_DEV_CHK_WATER_REQ,

	ATTACHED_DEV_CHK_WATER_DRY_REQ = 71,
	ATTACHED_DEV_GAMEPAD_MUIC,
	ATTACHED_DEV_CHECK_OCP,
	ATTACHED_DEV_RDU_TA_MUIC,
	ATTACHED_DEV_FACTORY_UART_MUIC,
	ATTACHED_DEV_PE_CHARGER_PREPARE_MUIC,
	ATTACHED_DEV_PE_CHARGER_9V_MUIC,
	ATTACHED_DEV_TURBO_CHARGER,
	ATTACHED_DEV_SPECOUT_CHARGER_MUIC,
	ATTACHED_DEV_UNKNOWN_MUIC,

	ATTACHED_DEV_POGO_DOCK_MUIC = 81,
	ATTACHED_DEV_POGO_DOCK_5V_MUIC,
	ATTACHED_DEV_POGO_DOCK_9V_MUIC,
	ATTACHED_DEV_POGO_DOCK_34K_MUIC,
	ATTACHED_DEV_POGO_DOCK_49_9K_MUIC,
	ATTACHED_DEV_ABNORMAL_OTG_MUIC,
	ATTACHED_DEV_RETRY_TIMEOUT_OPEN_MUIC,
	ATTACHED_DEV_RETRY_AFC_CHARGER_5V_MUIC,
	ATTACHED_DEV_RETRY_AFC_CHARGER_9V_MUIC,
	ATTACHED_DEV_WIRELESS_TA_MUIC,

	ATTACHED_DEV_LO_TA_MUIC = 91,
	ATTACHED_DEV_NUM,
} muic_attached_dev_t;

#ifdef CONFIG_MUIC_HV_FORCE_LIMIT
/* MUIC attached device type */
typedef enum {
	SILENT_CHG_DONE = 0,
	SILENT_CHG_CHANGING = 1,

	SILENT_CHG_NUM,
} muic_silent_change_state_t;
#endif

/* MUIC HV State type */
typedef enum {
	HV_STATE_INVALID = -1,
	HV_STATE_IDLE = 0,
	HV_STATE_DCP_CHARGER = 1,
	HV_STATE_FAST_CHARGE_ADAPTOR = 2,
	HV_STATE_FAST_CHARGE_COMMUNICATION = 3,
	HV_STATE_AFC_5V_CHARGER = 4,
	HV_STATE_AFC_9V_CHARGER = 5,
	HV_STATE_QC_CHARGER = 6,
	HV_STATE_QC_5V_CHARGER = 7,
	HV_STATE_QC_9V_CHARGER = 8,
	HV_STATE_QC_FAILED,
	HV_STATE_MAX_NUM,
} muic_hv_state_t;

typedef enum {
	HV_TRANS_INVALID = -1,
	HV_TRANS_MUIC_DETACH = 0,
	HV_TRANS_DCP_DETECTED,
	HV_TRANS_NO_RESPONSE,
	HV_TRANS_VDNMON_LOW,
	HV_TRANS_FAST_CHARGE_PING_RESPONSE,
	HV_TRANS_AFC_TA_DETECTED,
	HV_TRANS_QC_TA_DETECTED,
	HV_TRANS_VBUS_5V_BOOST,
	HV_TRANS_VBUS_BOOST,
	HV_TRANS_VBUS_REDUCE,
	HV_TRANS_VBUS_UPDATE,
	HV_TRANS_FAST_CHARGE_REOPEN,
	HV_TRANS_MAX_NUM,
} muic_hv_transaction_t;

typedef enum {
	HV_9V = 0,
	HV_5V,
} muic_hv_voltage_t;

#ifdef CONFIG_MUIC_COMMON_SYSFS
struct muic_sysfs_cb {
	int (*set_uart_en)(void *data, int en);
	void (*set_uart_sel)(void *data);
	int (*get_usb_en)(void *data);
	int (*set_usb_en)(void *data, int en);
	int (*get_adc)(void *data);
	int (*get_mansw)(void *data, char *mesg);
	int (*get_interrupt_status)(void *data, char *mesg);
	int (*get_register)(void *data, char *mesg);
	int (*get_attached_dev)(void *data);
	int (*get_otg_test)(void *data);
	int (*set_otg_test)(void *data, int en);
	void (*set_audio_path)(void *data);
	void (*set_apo_factory)(void *data);
	int (*get_vbus_value)(void *data);
	void (*set_afc_disable)(void *data);
	int (*afc_set_voltage)(void *data, int vol);
	int (*get_hiccup)(void *data);
	int (*set_hiccup)(void *data, int en);
	int (*set_overheat_hiccup)(void *data, int en);
};
#endif
/* muic common callback driver internal data structure
 * that setted at muic-core.c file
 */
struct muic_platform_data {
#ifdef CONFIG_MUIC_COMMON_SYSFS
	struct device *switch_device;
	struct mutex sysfs_mutex;
	struct muic_sysfs_cb sysfs_cb;
#endif
	struct device *muic_device;

#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
	struct muic_dev	*muic_d;
	struct if_cb_manager	*man;
#endif

	int switch_sel;

	/* muic current USB/UART path */
	int usb_path;
	int uart_path;

	bool rustproof_on;
	bool afc_disable;

	int afc_disabled_updated;

	enum muic_op_mode opmode;

	int vbvolt;
	int adc;

	bool is_factory_start;

	unsigned long driver_probe_flag;

	/* muic switch dev register function for DockObserver */
	void (*init_switch_dev_cb) (void);
	void (*cleanup_switch_dev_cb) (void);

	void (*jig_uart_cb)(int jig_state);

	/* muic GPIO control function */
#if IS_MODULE(CONFIG_MUIC_NOTIFIER)
	int (*init_gpio_cb)(int switch_sel);
#else
	int (*init_gpio_cb)(void);
#endif
	int (*set_gpio_usb_sel)(void *data, int usb_path);
	int (*set_gpio_uart_sel)(void *data, int uart_path);
	int (*set_safeout) (int safeout_path);

	/* muic cable data collecting function */
	void (*init_cable_data_collect_cb)(void);

	/* muic AFC voltage switching function */
	int (*muic_afc_set_voltage_cb)(int voltage);

	/* muic AFC get voltage function */
	int (*muic_afc_get_voltage_cb)(void);

	/* muic hv charger disable function */
	int (*muic_hv_charger_disable_cb)(bool en);

	/* muic check charger init function */
	int (*muic_hv_charger_init_cb)(void);

	/* muic set hiccup mode function */
	int (*muic_set_hiccup_mode_cb)(int on_off);

	/* muic set pogo adc function */
	int (*muic_set_pogo_adc_cb)(int adc);

	/* muic request afc cause */
	int afc_request_cause;

	void *drv_data;
};

#define MUIC_PDATA_VOID_FUNC(func, param) \
{\
	if (func)	\
		func(param);	\
	else	\
		pr_err("[muic_core] func not defined %s\n", __func__);	\
}

#define MUIC_PDATA_VOID_FUNC_MULTI_PARAM(func, param1, param2) \
{\
	if (func)	\
		func(param1, param2);	\
	else	\
		pr_err("[muic_core] func not defined %s\n", __func__);	\
}

#define MUIC_PDATA_FUNC(func, param, ret) \
{\
	*ret = 0;	\
	if (func)	\
		*ret = func(param);	\
	else	\
		pr_err("[muic_core] func not defined %s\n", __func__);	\
}

#define MUIC_PDATA_FUNC_MULTI_PARAM(func, param1, param2, ret) \
{					\
	*ret = 0;	\
	if (func)	\
		*ret = func(param1, param2);	\
	else	\
		pr_err("[muic_core] func not defined %s\n", __func__);	\
}

#define MUIC_IS_ATTACHED(dev) \
	(((dev != ATTACHED_DEV_UNKNOWN_MUIC) && (dev != ATTACHED_DEV_NONE_MUIC)) ? (1) : (0))

enum muic_param_en {
	MUIC_DISABLE = 0,
	MUIC_ENABLE
};

/* Integration */
#define ENUM_STR(x, r) { case x: r = #x; break; }

#define MASK_1b (1)
#define MASK_2b (0x3)
#define MASK_3b (0x7)
#define MASK_4b (0xf)
#define MASK_5b (0x1f)
#define MASK_6b (0x3f)
#define MASK_7b (0x7f)
#define MASK_8b (0xff)

#define IS_VCHGIN_9V(x) ((8000 <= x) && (x <= 10300))
#define IS_VCHGIN_5V(x) ((4000 <= x) && (x <= 6000))

#define AFC_MRXRDY_CNT_LIMIT (3)
#define AFC_MPING_RETRY_CNT_LIMIT (10)
#define AFC_QC_RETRY_CNT_LIMIT (3)
#define VCHGIN_CHECK_CNT_LIMIT (3)
#define AFC_QC_RETRY_WAIT_CNT_LIMIT (3)

typedef enum {
	AFC_IRQ_VDNMON = 1,
	AFC_IRQ_DNRES,
	AFC_IRQ_MPNACK,
	AFC_IRQ_MRXBUFOW,
	AFC_IRQ_MRXTRF,
	AFC_IRQ_MRXPERR,
	AFC_IRQ_MRXRDY = 7,
} afc_int_t;

typedef enum {
	AFC_NOT_MASK = 0,
	AFC_MASK = 1,
} int_mask_t;

typedef enum {
	QC_PROTOCOL,
	AFC_PROTOCOL,
} protocol_sw_t;

typedef enum {
	QC_UNKHOWN,
	QC_5V,
	QC_9V,
	QC_12V,
} qc_2p0_type_t;

typedef enum {
	VDNMON_LOW		= 0x00,
	VDNMON_HIGH		= (0x1 << 1),

	VDNMON_DONTCARE		= 0xff,
} vdnmon_t;

/* MUIC afc irq type */
typedef enum {
	MUIC_AFC_IRQ_VDNMON = 0,
	MUIC_AFC_IRQ_MRXRDY,
	MUIC_AFC_IRQ_VBADC,
	MUIC_AFC_IRQ_MPNACK,
	MUIC_AFC_IRQ_DONTCARE = 0xff,
} muic_afc_irq_t;

typedef enum tx_data{
    MUIC_HV_5V = 0,
    MUIC_HV_9V,
} muic_afc_txdata_t;

enum power_supply_lsi_property {
#if !defined(CONFIG_BATTERY_SAMSUNG) || \
	IS_ENABLED(CONFIG_MFD_S2MU106) || IS_ENABLED(CONFIG_MFD_S2MF301) || defined(CONFIG_BATTERY_GKI)
	POWER_SUPPLY_LSI_PROP_MIN = 10000,
#else
	POWER_SUPPLY_LSI_PROP_MIN = POWER_SUPPLY_EXT_PROP_MAX + 1,
#endif
	POWER_SUPPLY_LSI_PROP_POWER_ROLE,
	POWER_SUPPLY_LSI_PROP_WATER_CHECK,
	POWER_SUPPLY_LSI_PROP_DRY_CHECK,
	POWER_SUPPLY_LSI_PROP_WATER_CHECKDONE,
	POWER_SUPPLY_LSI_PROP_PM_IRQ_TIME,
	POWER_SUPPLY_LSI_PROP_USBPD_OPMODE,
	POWER_SUPPLY_LSI_PROP_USBPD_RPCUR,
	POWER_SUPPLY_LSI_PROP_USBPD_ATTACHED,
	POWER_SUPPLY_LSI_PROP_USBPD_SOURCE_ATTACH,
	POWER_SUPPLY_LSI_PROP_WATER_GET_POWER_ROLE,
	POWER_SUPPLY_LSI_PROP_GET_CC_STATE,
	POWER_SUPPLY_LSI_PROP_WATER_STATUS,
	POWER_SUPPLY_LSI_PROP_PD_PSY,
	POWER_SUPPLY_LSI_PROP_HICCUP_MODE,
	POWER_SUPPLY_LSI_PROP_FAC_WATER_CHECK,
	POWER_SUPPLY_LSI_PROP_SET_TH,
	POWER_SUPPLY_LSI_PROP_PM_VCHGIN,
	POWER_SUPPLY_LSI_PROP_2LV_3LV_CHG_MODE,
	POWER_SUPPLY_LSI_PROP_USBPD_RESET,
	POWER_SUPPLY_LSI_PROP_PD_SUPPORT,
	POWER_SUPPLY_LSI_PROP_VCHGIN,
	POWER_SUPPLY_LSI_PROP_VWCIN,
	POWER_SUPPLY_LSI_PROP_VBYP,
	POWER_SUPPLY_LSI_PROP_VSYS,
	POWER_SUPPLY_LSI_PROP_VBAT,
	POWER_SUPPLY_LSI_PROP_VGPADC,
	POWER_SUPPLY_LSI_PROP_VGPADC1,
	POWER_SUPPLY_LSI_PROP_VGPADC2,
	POWER_SUPPLY_LSI_PROP_ENABLE_WATER,
	POWER_SUPPLY_LSI_PROP_VCC1,
	POWER_SUPPLY_LSI_PROP_VCC2,
	POWER_SUPPLY_LSI_PROP_SBU_OVP_STATE,
	POWER_SUPPLY_LSI_PROP_ICHGIN,
	POWER_SUPPLY_LSI_PROP_IWCIN,
	POWER_SUPPLY_LSI_PROP_IOTG,
	POWER_SUPPLY_LSI_PROP_ITX,
	POWER_SUPPLY_LSI_PROP_CO_ENABLE,
	POWER_SUPPLY_LSI_PROP_RR_ENABLE,
	POWER_SUPPLY_LSI_PROP_PM_FACTORY,
	POWER_SUPPLY_LSI_PROP_PCP_CLK,
	POWER_SUPPLY_LSI_PROP_RID_OPS,
	POWER_SUPPLY_LSI_PROP_RID_DISABLE,
	POWER_SUPPLY_LSI_PROP_GET_REV,
#if IS_ENABLED(CONFIG_MFD_S2MU106) || IS_ENABLED(CONFIG_MFD_S2MF301) || defined(CONFIG_BATTERY_GKI)
	POWER_SUPPLY_LSI_PROP_MAX,
#endif
};

#ifdef CONFIG_IFCONN_NOTIFIER
#define MUIC_SEND_NOTI_ATTACH(dev) \
{	\
	int ret;	\
	struct ifconn_notifier_template template;	\
	template.cable_type = dev;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_MUIC,	\
					IFCONN_NOTIFY_MANAGER,	\
					IFCONN_NOTIFY_ID_ATTACH,	\
					IFCONN_NOTIFY_EVENT_ATTACH,	\
					&template);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

#define MUIC_SEND_NOTI_ATTACH_ALL(dev) \
{	\
	int ret;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_MUIC,	\
					IFCONN_NOTIFY_ALL,	\
					IFCONN_NOTIFY_ID_ATTACH,	\
					dev,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					NULL);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

#define MUIC_SEND_NOTI_DETACH_ALL(dev) \
{	\
	int ret;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_MUIC,	\
					IFCONN_NOTIFY_ALL,	\
					IFCONN_NOTIFY_ID_DETACH,	\
					dev,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					NULL);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

#define MUIC_SEND_NOTI_TO_PDIC_ATTACH(dev) \
{	\
	int ret;	\
	struct ifconn_notifier_template template;	\
	template.cable_type = dev;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_MUIC,	\
					IFCONN_NOTIFY_PDIC,	\
					IFCONN_NOTIFY_ID_ATTACH,	\
					IFCONN_NOTIFY_EVENT_ATTACH,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					&template);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

#define MUIC_SEND_NOTI_TO_PDIC_DETACH(dev) \
{	\
	int ret;	\
	struct ifconn_notifier_template template;	\
	template.cable_type = dev;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_MUIC,	\
					IFCONN_NOTIFY_PDIC,	\
					IFCONN_NOTIFY_ID_DETACH,	\
					IFCONN_NOTIFY_EVENT_DETACH,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					&template);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

#define MUIC_SEND_NOTI_DETACH(dev) \
{	\
	int ret;	\
	struct ifconn_notifier_template template;	\
	template.cable_type = dev;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_MUIC,	\
					IFCONN_NOTIFY_MANAGER,	\
					IFCONN_NOTIFY_ID_DETACH,	\
					IFCONN_NOTIFY_EVENT_DETACH,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					&template);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}
#else
#define MUIC_SEND_NOTI_ATTACH(dev)	\
		muic_notifier_attach_attached_dev(dev)
#define MUIC_SEND_NOTI_DETACH(dev) \
		muic_notifier_detach_attached_dev(dev)
#define MUIC_SEND_NOTI_TO_PDIC_ATTACH(dev) \
		muic_pdic_notifier_attach_attached_dev(dev)
#define MUIC_SEND_NOTI_TO_PDIC_DETACH(dev) \
		muic_pdic_notifier_detach_attached_dev(dev)
#endif

#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
extern void muic_send_lcd_on_uevent(struct muic_platform_data *muic_pdata);
extern int muic_set_hiccup_mode(int on_off);
extern int muic_hv_charger_init(void);
#if IS_ENABLED(CONFIG_MUIC_POGO)
extern int muic_set_pogo_adc(int adc);
#endif
extern int muic_afc_get_voltage(void);
extern int muic_afc_set_voltage(int voltage);
extern int muic_afc_request_voltage(int cause, int voltage);
extern int muic_afc_request_cause_clear(void);
extern int muic_afc_request_cause_clear_bit(int cause);
extern int muic_afc_get_request_cause(void);
extern bool muic_is_enable_afc_request(void);
extern int muic_hv_charger_disable(bool en);

#else
static inline void muic_send_lcd_on_uevent(struct muic_platform_data *muic_pdata)
	{return; }
static inline int muic_set_hiccup_mode(int on_off) {return 0; }
static inline int muic_hv_charger_init(void) {return 0; }
static inline int muic_afc_get_voltage(void) {return 0; }
#if IS_ENABLED(CONFIG_MUIC_POGO)
static inline int muic_set_pogo_adc(int adc) {return 0};
#endif
static inline int muic_afc_set_voltage(int voltage) {return 0; }
static inline int muic_afc_request_voltage(int cause, int voltage);
static inline int muic_afc_request_cause_clear(void);
static inline int muic_afc_request_cause_clear_bit(int cause);
static inline int muic_afc_get_request_cause(void) {return 0;}
static inline bool muic_is_enable_afc_request(void) {return false;}
static inline int muic_hv_charger_disable(bool en) {return 0; }
#endif

#endif /* __MUIC_H__ */
