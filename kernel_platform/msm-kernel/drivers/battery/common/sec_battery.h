/*
 * sec_battery.h
 * Samsung Mobile Battery Header
 *
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
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

#ifndef __SEC_BATTERY_H
#define __SEC_BATTERY_H __FILE__

#include "sec_charging_common.h"
#include <linux/of_gpio.h>
#include <linux/alarmtimer.h>
#include <linux/pm_wakeup.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>

#if IS_ENABLED(CONFIG_HALL_NOTIFIER) && !defined(CONFIG_ARCH_MEDIATEK)
#include <linux/hall/hall_ic_notifier.h>
#endif

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
#include <linux/usb/typec/common/pdic_notifier.h>
#endif

#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
#include <linux/usb/typec/manager/usb_typec_manager_notifier.h>
#else
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/common/muic.h>
#include <linux/muic/common/muic_notifier.h>
#endif
#endif
#include <linux/battery/sec_pd.h>
#include "sec_cisd.h"
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
#include "sec_direct_charger.h"
#endif
#if defined(CONFIG_WIRELESS_AUTH)
#include "sec_battery_misc.h"
#endif
#include "sec_adc.h"
#include "sb_checklist_app.h"
#include "sb_full_soc.h"

extern const char *sb_get_ct_str(int cable_type);
extern const char *sb_get_cm_str(int chg_mode);
extern const char *sb_get_bst_str(int st);
extern const char *sb_get_hl_str(int health);
extern const char *sb_get_tz_str(int tz);
extern const char *sb_charge_mode_str(int charge_mode);

#if IS_ENABLED(CONFIG_SEC_KUNIT)
#include <kunit/test.h>
#include <kunit/mock.h>
#else
#ifndef EXPORT_SYMBOL_KUNIT
#define EXPORT_SYMBOL_KUNIT(sym)    /* nothing */
#endif
#ifndef __mockable
#define __mockable                  /* nothing */
#endif
#ifndef __visible_for_testing
#define __visible_for_testing       static
#endif
#endif /* CONFIG_SEC_KUNIT */

/* current event */
#define SEC_BAT_CURRENT_EVENT_NONE					0x000000
#define SEC_BAT_CURRENT_EVENT_AFC					0x000001
#define SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE		0x000002
#define SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL	0x000004
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL1	0x000008
#define SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING	0x000020
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL2	0x000080
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL3	0x000010
#define SEC_BAT_CURRENT_EVENT_SWELLING_MODE		(SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL1 | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL2 | SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL3)
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE		(SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL1 | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL2 | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL3)
#define SEC_BAT_CURRENT_EVENT_CHG_LIMIT			0x000200
#define SEC_BAT_CURRENT_EVENT_CALL			0x000400
#define SEC_BAT_CURRENT_EVENT_SLATE			0x000800
#define SEC_BAT_CURRENT_EVENT_VBAT_OVP			0x001000
#define SEC_BAT_CURRENT_EVENT_VSYS_OVP			0x002000
#define SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK		0x004000
#define SEC_BAT_CURRENT_EVENT_AICL			0x008000
#define SEC_BAT_CURRENT_EVENT_HV_DISABLE		0x010000
#define SEC_BAT_CURRENT_EVENT_SELECT_PDO		0x020000
#define SEC_BAT_CURRENT_EVENT_FG_RESET			0x040000
#define SEC_BAT_CURRENT_EVENT_WDT_EXPIRED		0x080000
#define SEC_BAT_CURRENT_EVENT_NOPD_HV_DISABLE		0x100000
#if defined(CONFIG_ISDB_CHARGING_CONTROL)
#define SEC_BAT_CURRENT_EVENT_ISDB			0x200000
#else
#define SEC_BAT_CURRENT_EVENT_ISDB			0x000000
#endif
#define SEC_BAT_CURRENT_EVENT_DC_ERR			0x400000
#define SEC_BAT_CURRENT_EVENT_SIOP_LIMIT		0x800000
#define SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST		0x1000000
#define SEC_BAT_CURRENT_EVENT_25W_OCP			0x2000000
#define SEC_BAT_CURRENT_EVENT_AFC_DISABLE		0x4000000
#define SEC_BAT_CURRENT_EVENT_SEND_UVDM			0x8000000

#define SEC_BAT_CURRENT_EVENT_USB_SUSPENDED		0x10000000
#define SEC_BAT_CURRENT_EVENT_USB_SUPER			0x20000000
#define SEC_BAT_CURRENT_EVENT_USB_100MA			0x40000000
#define SEC_BAT_CURRENT_EVENT_USB_STATE		(SEC_BAT_CURRENT_EVENT_USB_SUSPENDED |\
						SEC_BAT_CURRENT_EVENT_USB_SUPER |\
						SEC_BAT_CURRENT_EVENT_USB_100MA)
#define SEC_BAT_CURRENT_EVENT_WPC_EN		0x80000000

/* misc_event */
#define BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE		0x00000001
#define BATT_MISC_EVENT_WIRELESS_BACKPACK_TYPE		0x00000002
#define BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE		0x00000004
#define BATT_MISC_EVENT_BATT_RESET_SOC			0x00000008
#define BATT_MISC_EVENT_WATER_HICCUP_TYPE		0x00000020
#define BATT_MISC_EVENT_WIRELESS_DET_LEVEL		0x00000040
#define BATT_MISC_EVENT_WIRELESS_FOD			0x00000100
#define BATT_MISC_EVENT_WIRELESS_AUTH_START		0x00000200
#define BATT_MISC_EVENT_WIRELESS_AUTH_RECVED		0x00000400
#define BATT_MISC_EVENT_WIRELESS_AUTH_FAIL		0x00000800
#define BATT_MISC_EVENT_WIRELESS_AUTH_PASS		0x00001000
#define BATT_MISC_EVENT_TEMP_HICCUP_TYPE		0x00002000
#define BATT_MISC_EVENT_DIRECT_POWER_MODE		0x00004000
#define BATT_MISC_EVENT_WIRELESS_OVERHEAT		0x00008000
#define BATT_MISC_EVENT_BATTERY_HEALTH			0x000F0000
#define BATT_MISC_EVENT_HEALTH_OVERHEATLIMIT		0x00100000
//#define BATT_MISC_EVENT_ABNORMAL_PAD		0x00200000
#define BATT_MISC_EVENT_WIRELESS_MISALIGN	0x00400000
#define BATT_MISC_EVENT_FULL_CAPACITY		0x01000000
#define BATT_MISC_EVENT_PASS_THROUGH		0x02000000
#define BATT_MISC_EVENT_MAIN_POWERPATH		0x04000000
#define BATT_MISC_EVENT_SUB_POWERPATH		0x08000000
#define BATT_MISC_EVENT_HV_BY_AICL			0x10000000
#define BATT_MISC_EVENT_WC_JIG_PAD			0x20000000

#define BATTERY_HEALTH_SHIFT	16
enum misc_battery_health {
	BATTERY_HEALTH_UNKNOWN = 0,
	BATTERY_HEALTH_GOOD,
	BATTERY_HEALTH_NORMAL,
	BATTERY_HEALTH_AGED,
	BATTERY_HEALTH_MAX = BATTERY_HEALTH_AGED,

	/* For event */
	BATTERY_HEALTH_BAD = 0xF,
};
#define BATT_MISC_EVENT_MUIC_ABNORMAL	(BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE |\
					BATT_MISC_EVENT_WATER_HICCUP_TYPE |\
					BATT_MISC_EVENT_TEMP_HICCUP_TYPE)

#define DEFAULT_HEALTH_CHECK_COUNT	5

#define SIOP_INPUT_LIMIT_CURRENT                1200
#define SIOP_CHARGING_LIMIT_CURRENT             1800
#define SIOP_WIRELESS_INPUT_LIMIT_CURRENT       600
#define SIOP_HV_WIRELESS_INPUT_LIMIT_CURRENT	700
#define SIOP_STORE_HV_WIRELESS_CHARGING_LIMIT_CURRENT	450
#define SIOP_HV_INPUT_LIMIT_CURRENT			700
#define SIOP_HV_CHARGING_LIMIT_CURRENT			1800
#define SIOP_HV_12V_INPUT_LIMIT_CURRENT			535
#define SIOP_HV_12V_CHARGING_LIMIT_CURRENT		1000
#define SIOP_APDO_INPUT_LIMIT_CURRENT				1000
#define SIOP_APDO_CHARGING_LIMIT_CURRENT			2000

#define WIRELESS_OTG_INPUT_CURRENT 900

enum {
	SEC_INPUT_VOLTAGE_0V = 0,
	SEC_INPUT_VOLTAGE_NONE = 1000,
	SEC_INPUT_VOLTAGE_APDO = 1234,
	SEC_INPUT_VOLTAGE_5V = 5000,
	SEC_INPUT_VOLTAGE_5_5V = 5500,
	SEC_INPUT_VOLTAGE_9V = 9000,
	SEC_INPUT_VOLTAGE_10V = 10000,
	SEC_INPUT_VOLTAGE_12V = 12000,
	SEC_INPUT_VOLTAGE_12_5V = 12500,
	SEC_INPUT_VOLTAGE_13V = 13000,
};

#define HV_CHARGER_STATUS_STANDARD1	12000 /* mW */
#define HV_CHARGER_STATUS_STANDARD2	20000 /* mW */
#define HV_CHARGER_STATUS_STANDARD3 24500 /* mW */
#define HV_CHARGER_STATUS_STANDARD4 40000 /* mW */

#define DC_CHARGER_MIN_CURRENT 1000 /* mA */
#define DC_CHARGER_3TO1_MIN_VOLTAGE 16 /* V */
#define DC_CHARGER_3TO1_SKIP_CABLE_CURRENT 5000 /* mA */
#define DC_CHARGER_3TO1_SKIP_CABLE_VOLT 9000 /* mV */

#define WFC10_WIRELESS_POWER	7500000 /* mW */
#define WFC20_WIRELESS_POWER	11000000 /* mW */
#define WFC21_WIRELESS_POWER	15000000 /* mW */

#define mW_by_mVmA(v, a)	((v) * (a) / 1000)
#define mV_by_mWmA(w, a)	((a) ? (((w) * 1000) / (a)) : (0))
#define mA_by_mWmV(w, v)	((v) ? (((w) * 1000) / (v)) : (0))

#define DEFAULT_FULL_MARGIN 50 /* mV */
#define DEFAULT_RCHG_MARGIN 70 /* mV */
#define DEFAULT_LOW_SWELLING_RCHG_MARGIN 150 /* mV */

enum battery_misc_test {
	MISC_TEST_RESET = 0,
	MISC_TEST_DISPLAY,
	MISC_TEST_EPT_UNKNOWN,
	MISC_TEST_TRACE_VTRACK,
	MISC_TEST_DC_ERR,
	MISC_TEST_MAX,
};

enum {
	NORMAL_TA,
	AFC_9V_OR_15W,
	AFC_12V_OR_20W,
	SFC_25W,
	SFC_45W,
};

struct sec_bat_pdic_list {
	unsigned int max_pd_count;
	bool now_isApdo;
	unsigned int num_fpdo;
	unsigned int num_apdo;
};

enum {
	USB_CONN_NORMAL = 0x0,
	USB_CONN_SLOPE_OVER = 0x1,
	USB_CONN_GAP_OVER1 = 0x2,
	USB_CONN_GAP_OVER2 = 0x4,
	USB_CONN_OVERHEATLIMIT = 0x8,
};

#define MAX_USB_CONN_CHECK_CNT 10

typedef struct sec_charging_current {
	unsigned int input_current_limit;
	unsigned int fast_charging_current;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	unsigned int main_limiter_current;
	unsigned int sub_limiter_current;
#endif
} sec_charging_current_t;

/**
 * struct sec_bat_adc_table_data - adc to temperature table for sec battery
 * driver
 * @adc: adc value
 * @temperature: temperature(C) * 10
 */
typedef struct sec_bat_adc_table_data {
	int adc;
	int data;
} sec_bat_adc_table_data_t;

typedef struct sec_bat_adc_region {
	int min;
	int max;
} sec_bat_adc_region_t;

/* nv, hv, fpdo, apdo, wpc, wpc_hv */
enum sec_siop_curr_type {
	SIOP_CURR_TYPE_NV = 0,
	SIOP_CURR_TYPE_HV,
	SIOP_CURR_TYPE_FPDO,
	SIOP_CURR_TYPE_APDO,
	SIOP_CURR_TYPE_WPC_NV,
	SIOP_CURR_TYPE_WPC_HV,
	SIOP_CURR_TYPE_MAX,
};

struct sec_siop_table {
	int level;
	int icl[SIOP_CURR_TYPE_MAX];
	int fcc[SIOP_CURR_TYPE_MAX];
};

#define SIOP_SCENARIO_NUM_MAX		10

struct sec_bat_thm_info {
	int source;
	sec_bat_adc_table_data_t *adc_table;
	unsigned int adc_table_size;
	int offset;
	int check_type;
	unsigned int check_count;
	int test;
	int adc;
	int channel;
	unsigned int adc_rsense;
};

enum sec_battery_thm_info {
	THM_INFO_NONE = 0,
	THM_INFO_BAT,
	THM_INFO_USB,
	THM_INFO_CHG,
	THM_INFO_WPC,
	THM_INFO_SUB_BAT,
	THM_INFO_BLK,
	THM_INFO_DCHG,
};

/* LRP structure */
#define LRP_PROPS 12
#define FOREACH_LRP_TYPE(GEN_LRP_TYPE) \
	GEN_LRP_TYPE(LRP_NORMAL) \
	GEN_LRP_TYPE(LRP_25W) \
	GEN_LRP_TYPE(LRP_45W) \
	GEN_LRP_TYPE(LRP_MAX)

#define GENERATE_LRP_ENUM(ENUM) ENUM,
#define GENERATE_LRP_STRING(STRING) #STRING,

enum LRP_TYPE_ENUM {
	FOREACH_LRP_TYPE(GENERATE_LRP_ENUM)
};

static const char * const LRP_TYPE_STRING[] = {
	FOREACH_LRP_TYPE(GENERATE_LRP_STRING)
};

enum {
	LRP_NONE = 0,
	LRP_STEP1,
	LRP_STEP2,
};

enum {
	ST1 = 0,
	ST2,
};

enum {
	LCD_ON = 0,
	LCD_OFF,
};

struct lrp_temp_t {
	int trig[2][2];
	int recov[2][2];
};

struct lrp_current_t {
	int st_icl[2];
	int st_fcc[2];
};

typedef struct sec_battery_platform_data {
	/* NO NEED TO BE CHANGED */
	/* callback functions */
	void (*initial_check)(void);
	void (*monitor_additional_check)(void);
	bool (*bat_gpio_init)(void);
	bool (*fg_gpio_init)(void);
	bool (*is_lpm)(void);
	bool (*check_jig_status)(void);
	bool (*is_interrupt_cable_check_possible)(int);
	int (*check_cable_callback)(void);
	int (*get_cable_from_extended_cable_type)(int);
	bool (*cable_switch_check)(void);
	bool (*cable_switch_normal)(void);
	bool (*check_cable_result_callback)(int);
	bool (*check_battery_callback)(void);
	bool (*check_battery_result_callback)(void);
	int (*ovp_uvlo_callback)(void);
	bool (*ovp_uvlo_result_callback)(int);
	bool (*fuelalert_process)(bool);
	bool (*get_temperature_callback)(
			enum power_supply_property,
			union power_supply_propval*);

	/* ADC region by power supply type
	 * ADC region should be exclusive
	 */
	sec_bat_adc_region_t *cable_adc_value;
	/* charging current for type (0: not use) */
	sec_charging_current_t charging_current[SEC_BATTERY_CABLE_MAX];
	unsigned int *polling_time;
	char *chip_vendor;
	/* NO NEED TO BE CHANGED */
	unsigned int pre_afc_input_current;
	unsigned int pre_wc_afc_input_current;
	unsigned int select_pd_input_current;
	unsigned int store_mode_max_input_power;

	char *pmic_name;

	/* battery */
	char *vendor;
	int technology;
	void *battery_data;

	int bat_gpio_ta_nconnected;
	/* 1 : active high, 0 : active low */
	int bat_polarity_ta_nconnected;
	sec_battery_cable_check_t cable_check_type;
	sec_battery_cable_source_t cable_source_type;

	unsigned int swelling_high_rechg_voltage;
	unsigned int swelling_low_rechg_voltage;
	unsigned int swelling_low_cool3_rechg_voltage;
	bool chgen_over_swell_rechg_vol;

#if IS_ENABLED(CONFIG_STEP_CHARGING)
	/* step charging */
	unsigned int **step_chg_cond;
	unsigned int **wpc_step_chg_cond;
	unsigned int **step_chg_cond_soc;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	unsigned int **step_chg_cond_sub;
#endif
	unsigned int *step_chg_cond_curr;
	unsigned int **step_chg_curr;
	unsigned int **step_chg_vfloat;
	unsigned int *wpc_step_chg_cond_curr;
	unsigned int **wpc_step_chg_curr;
	unsigned int **wpc_step_chg_vfloat;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	unsigned int **dc_step_chg_cond_vol_sub;
	unsigned int dc_step_cond_v_margin_main;
	unsigned int dc_step_cond_v_margin_sub;
	unsigned int sc_vbat_thresh_main; /* main vbat threshold which dc to sc */
	unsigned int sc_vbat_thresh_sub; /* sub vbat threshold which dc to sc */
	unsigned int *dc_step_chg_vsublim;
#endif
	unsigned int dc_step_chg_cond_v_margin;
	unsigned int **dc_step_chg_cond_vol;
	unsigned int **dc_step_chg_cond_soc;
	unsigned int **dc_step_chg_cond_iin;
	unsigned int *dc_step_chg_vol_offset;
	int dc_step_chg_iin_check_cnt;

	unsigned int **dc_step_chg_val_iout;
	unsigned int **dc_step_chg_val_vfloat;
#endif
#endif

	/* Monitor setting */
	int polling_type;
	/* for initial check */
	unsigned int monitor_initial_count;

	/* Battery check */
	sec_battery_check_t battery_check_type;
	/* how many times do we need to check battery */
	unsigned int check_count;
	/* ADC */
	/* battery check ADC maximum value */
	unsigned int check_adc_max;
	/* battery check ADC minimum value */
	unsigned int check_adc_min;

	/* OVP/UVLO check */
	int ovp_uvlo_check_type;

	struct sec_bat_thm_info bat_thm_info;
	struct sec_bat_thm_info usb_thm_info;
	struct sec_bat_thm_info chg_thm_info;
	struct sec_bat_thm_info wpc_thm_info;
	struct sec_bat_thm_info sub_bat_thm_info;
	struct sec_bat_thm_info blk_thm_info;
	struct sec_bat_thm_info dchg_thm_info;
	bool dctp_by_cgtp;
	bool dctp_bootmode_en;
	bool lrpts_by_batts;
	int usb_temp_check_type_backup; /* sec_bat_set_temp_control_test() */
	int lrp_temp_check_type;
	unsigned int temp_check_count;

	sec_bat_adc_table_data_t *inbat_adc_table;
	unsigned int inbat_adc_table_size;
	unsigned int inbat_voltage;
	unsigned int inbat_ocv_type;

	/*
	 * limit can be ADC value or Temperature
	 * depending on temp_check_type
	 * temperature should be temp x 10 (0.1 degree)
	 */
	int wireless_cold_cool3_thresh;
	int wireless_cool3_cool2_thresh;
	int wireless_cool2_cool1_thresh;
	int wireless_cool1_normal_thresh;
	int wireless_normal_warm_thresh;
	int wireless_warm_overheat_thresh;

	int wire_cold_cool3_thresh;
	int wire_cool3_cool2_thresh;
	int wire_cool2_cool1_thresh;
	int wire_cool1_normal_thresh;
	int wire_normal_warm_thresh;
	int wire_warm_overheat_thresh;

	int wire_warm_current;
	int wire_cool1_current;
	int wire_cool2_current;
	int wire_cool3_current;
	int wireless_warm_current;
	int wireless_cool1_current;
	int wireless_cool2_current;
	int wireless_cool3_current;
	int high_temp_float;
	int low_temp_float;
	int low_temp_cool3_float;

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	unsigned int limiter_main_warm_current;
	unsigned int limiter_sub_warm_current;
	unsigned int limiter_main_wireless_warm_current;
	unsigned int limiter_sub_wireless_warm_current;
	unsigned int limiter_main_cool1_current;
	unsigned int limiter_sub_cool1_current;
	unsigned int limiter_main_cool2_current;
	unsigned int limiter_sub_cool2_current;
	unsigned int limiter_main_cool3_current;
	unsigned int limiter_sub_cool3_current;
	unsigned int limiter_aging_float_offset;

	unsigned int step_chg_vsublim;
#if IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
	bool dual_battery_individual_topoff;
	bool dual_battery_individual_fv;
	int dual_battery_balance_threshold;
#endif
#endif

	int buck_recovery_margin;

	int tx_high_threshold;
	int tx_high_recovery;
	int tx_low_threshold;
	int tx_low_recovery;
	int chg_12v_high_temp;
	int chg_high_temp;
	int chg_high_temp_recovery;
	int dchg_high_temp[4];
	int dchg_high_temp_recovery[4];
	int dchg_high_batt_temp[4];
	int dchg_high_batt_temp_recovery[4];

	struct lrp_temp_t lrp_temp[LRP_MAX];
	struct lrp_current_t lrp_curr[LRP_MAX];

	unsigned int chg_charging_limit_current;
	unsigned int chg_input_limit_current;
	unsigned int dchg_charging_limit_current;
	unsigned int dchg_input_limit_current;
	unsigned int sub_temp_control_source;
	unsigned int wpc_temp_control_source;
	unsigned int wpc_temp_lcd_on_control_source;
	int wpc_high_temp;
	int wpc_high_temp_recovery;
	int wpc_high_temp_12w;
	int wpc_high_temp_recovery_12w;
	int wpc_high_temp_15w;
	int wpc_high_temp_recovery_15w;
	unsigned int wpc_input_limit_current;
	unsigned int wpc_charging_limit_current;
	bool enable_check_wpc_temp_v2;
	int wpc_temp_v2_cond;
	int wpc_temp_v2_cond_12w;
	int wpc_temp_v2_cond_15w;
	int wpc_temp_offset_step;

	int wpc_lrp_high_temp;
	int wpc_lrp_high_temp_recovery;
	int wpc_lrp_high_temp_12w;
	int wpc_lrp_high_temp_recovery_12w;
	int wpc_lrp_high_temp_15w;
	int wpc_lrp_high_temp_recovery_15w;
	int wpc_lrp_temp_v2_cond;
	int wpc_lrp_temp_v2_cond_12w;
	int wpc_lrp_temp_v2_cond_15w;

	unsigned int wpc_step_limit_size;
	unsigned int *wpc_step_limit_temp;
	unsigned int *wpc_step_limit_fcc;
	unsigned int *wpc_step_limit_fcc_12w;
	unsigned int *wpc_step_limit_fcc_15w;

	int wpc_lcd_on_high_temp;
	int wpc_lcd_on_high_temp_rec;
	int wpc_lcd_on_high_temp_12w;
	int wpc_lcd_on_high_temp_rec_12w;
	int wpc_lcd_on_high_temp_15w;
	int wpc_lcd_on_high_temp_rec_15w;
	unsigned int wpc_lcd_on_input_limit_current;
	unsigned int wpc_flicker_wa_input_limit_current;
	unsigned int sleep_mode_limit_current;
	unsigned int wc_full_input_limit_current;
	unsigned int max_charging_current;
	unsigned int max_charging_charge_power;
	unsigned int apdo_max_volt;
	unsigned int apdo_max_volt_arr[DC_MODE_MAX];

	int mix_high_temp;
	int mix_high_chg_temp;
	int mix_high_temp_recovery;

	bool enable_mix_v2;
	int mix_v2_lrp_recov;
	int mix_v2_lrp_cond;
	int mix_v2_bat_cond;
	int mix_v2_chg_cond;
	int mix_v2_dchg_cond;

	bool wpc_high_check_using_lrp;
	bool wpc_high_check_with_nv;

	unsigned int icl_by_tx_gear; /* check limited charging current during wireless power sharing with cable charging */
	unsigned int fcc_by_tx;
	unsigned int fcc_by_tx_gear;
	unsigned int wpc_input_limit_by_tx_check; /* check limited wpc input current with tx device */
	unsigned int wpc_input_limit_current_by_tx;

	/* If these is NOT full check type or NONE full check type,
	 * it is skipped
	 */
	/* 1st full check */
	int full_check_type;
	/* 2nd full check */
	int full_check_type_2nd;
	unsigned int full_check_count;
	int chg_gpio_full_check;
	/* 1 : active high, 0 : active low */
	int chg_polarity_full_check;
	sec_battery_full_condition_t full_condition_type;
	unsigned int full_condition_soc;
	unsigned int full_condition_vcell;
	unsigned int full_condition_avgvcell;
	unsigned int full_condition_ocv;

	unsigned int recharge_check_count;
	sec_battery_recharge_condition_t recharge_condition_type;
	unsigned int recharge_condition_soc;
	unsigned int recharge_condition_vcell;

	unsigned long charging_reset_time;

	/* fuel gauge */
	char *fuelgauge_name;

	unsigned int store_mode_charging_max;
	unsigned int store_mode_charging_min;
	unsigned int store_mode_buckoff;

	/* charger */
	char *charger_name;
	char *otg_name;
	char *fgsrc_switch_name;
	bool support_fgsrc_change;
	bool dynamic_cv_factor;
	bool slowcharging_usb_bootcomplete;

	/* wireless charger */
	char *wireless_charger_name;
	int wireless_cc_cv;
	bool p2p_cv_headroom;

	/* float voltage (mV) */
	unsigned int chg_float_voltage;
	unsigned int chg_float_voltage_conv;

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	/* current limiter */
	char *dual_battery_name;
	char *main_limiter_name;
	char *sub_limiter_name;
	int main_bat_enb_gpio;
	int main_bat_enb2_gpio;
	int sub_bat_enb_gpio;

	bool step_chg_use_vnow;
	bool dc_step_chg_use_vnow;
#endif

#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
	char *dual_fuelgauge_name;
	char *main_fuelgauge_name;
	char *sub_fuelgauge_name;
	unsigned int main_design_capacity;
	unsigned int sub_design_capacity;
#endif

	int num_age_step;
	int age_step;
	sec_age_data_t* age_data;
	battery_health_condition* health_condition;

	int siop_icl;
	int siop_fcc;
	int siop_hv_icl;
	int siop_hv_icl_2nd;
	int siop_hv_fcc;
	int siop_hv_12v_icl;
	int siop_hv_12v_fcc;
	int siop_apdo_icl;
	int siop_apdo_fcc;

	int siop_wpc_icl;
	int *siop_wpc_fcc;
	int siop_hv_wpc_icl;
	int rechg_hv_wpc_icl;
	int *siop_hv_wpc_fcc;
	int wireless_otg_input_current;
	int wc_hero_stand_cc_cv;
	int wc_hero_stand_cv_current;
	int wc_hero_stand_hv_cv_current;

	int siop_scenarios_num;
	int siop_curr_type_num;
	struct sec_siop_table siop_table[SIOP_SCENARIO_NUM_MAX];

	int default_input_current;
	int default_charging_current;
	int default_usb_input_current;
	int default_usb_charging_current;
	unsigned int default_wc20_input_current;
	unsigned int default_wc20_charging_current;
	unsigned int default_mpp_input_current;
	unsigned int default_mpp_charging_current;
	int max_input_voltage;
	int max_input_current;
	int pre_afc_work_delay;
	int pre_wc_afc_work_delay;

	unsigned int rp_current_rp1;
	unsigned int rp_current_rp2;
	unsigned int rp_current_rp3;
	unsigned int rp_current_rdu_rp3;
	unsigned int rp_current_abnormal_rp3;

	bool fake_capacity;
	bool en_batt_full_status_usage;
	bool en_auto_shipmode_temp_ctrl;
	bool boosting_voltage_aicl;
	bool tx_5v_disable;
	unsigned int phm_vout_ctrl_dev;
	unsigned int power_value;

	bool bc12_ifcon_wa;

	bool mass_with_usb_thm;
	bool usb_protection;

	/* tx power sharging */
	unsigned int tx_stop_capacity;

	unsigned int battery_full_capacity;
	unsigned int cisd_cap_high_thr;
	unsigned int cisd_cap_low_thr;
	unsigned int cisd_cap_limit;
	int max_voltage_thr;
	unsigned int auth_ic_support;
	unsigned int cisd_alg_index;
	unsigned int *ignore_cisd_index;
	unsigned int *ignore_cisd_index_d;

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	/* zone 1 : 0C ~ 0.4C */
	unsigned int zone1_limiter_current;
	unsigned int main_zone1_current_rate;
	unsigned int sub_zone1_current_rate;
	/* zone 2 : 0.4C ~ 1.1C */
	unsigned int zone2_limiter_current;
	unsigned int main_zone2_current_rate;
	unsigned int sub_zone2_current_rate;
	/* zone 3 : 1.1C ~ MAX */
	unsigned int zone3_limiter_current;
	unsigned int main_zone3_current_rate;
	unsigned int sub_zone3_current_rate;

	unsigned int force_recharge_margin;
	unsigned int max_main_limiter_current;
	unsigned int min_main_limiter_current;
	unsigned int max_sub_limiter_current;
	unsigned int min_sub_limiter_current;
	/* fully turn on flag */
	bool main_fto;
	bool sub_fto;
	/* fully turn on threshold current */
	unsigned int main_fto_current_thresh;
	unsigned int sub_fto_current_thresh;
#endif

	/* ADC setting */
	unsigned int adc_check_count;

	unsigned int adc_read_type;

	unsigned int full_check_current_1st;
	unsigned int full_check_current_2nd;

	unsigned int pd_charging_charge_power;
	unsigned int nv_charge_power;

	unsigned int expired_time;
	unsigned int recharging_expired_time;
	int standard_curr;

	unsigned int tx_minduty_5V;
	unsigned int tx_minduty_default;

	unsigned int tx_ping_duty_no_ta;
	unsigned int tx_ping_duty_default;

	unsigned int tx_uno_vout;
	unsigned int tx_uno_iout;
	unsigned int tx_uno_iout_gear;
	unsigned int tx_uno_iout_aov_gear;
	unsigned int tx_buds_vout; // true wireless stereo type like buds
	unsigned int tx_gear_vout; // watch type
	unsigned int tx_ping_vout;
	unsigned int tx_mfc_iout_gear;
	unsigned int tx_mfc_iout_aov_gear;
	unsigned int tx_mfc_iout_phone;
	unsigned int tx_mfc_iout_phone_5v;
	unsigned int tx_mfc_iout_lcd_on;

	unsigned int tx_aov_start_vout;
	unsigned int tx_aov_freq_low;
	unsigned int tx_aov_freq_high;
	unsigned int tx_aov_delay;
	unsigned int tx_aov_delay_phm_escape;

	bool tx_lrp_temp_compensation;
	int tx_lrp_temp_trig;
	int tx_lrp_temp_recov;

	/* MAIN LRPST compensation */
	bool lr_enable;
	unsigned int lr_param_bat_thm;
	unsigned int lr_param_sub_bat_thm;
	unsigned int lr_delta;
	unsigned int lr_param_init_bat_thm;
	unsigned int lr_param_init_sub_bat_thm;
	unsigned int lr_round_off;

	bool wpc_vout_ctrl_lcd_on;
	bool soc_by_repcap_en;

	unsigned int d2d_check_type;
	bool support_vpdo;
	bool support_fpdo_dc;
	unsigned int fpdo_dc_charge_power;

	bool sc_LRP_25W;
	int batt_temp_adj_gap_inc;
	int change_FV_after_full;

	bool support_usb_conn_check;
	unsigned int usb_conn_slope_avg;

	bool wpc_warm_fod;
	unsigned int wpc_warm_fod_icc;

	unsigned int max_wlc_icl_15w;
	unsigned int max_wlc_icl_12w;

	bool loosened_unknown_temp;

	bool support_spsn_ctrl;

	bool pogo_chgin;
	bool hall_ic_notifier;

	int mpla_thr_cnt;
	bool support_mpla_thr_recov;
	int mpla_thr_recov_cnt;

	bool batt_id_changed;
	int current_bat_id;
	bool multi_batt_id_chg_support;
} sec_battery_platform_data_t;

struct sec_ttf_data;

struct sec_eoc_info {
	bool eoc_check;
	unsigned int eoc_cnt;
};

struct sec_safety_timer {
	bool stop_timer; /* this flag is for pausing safety timer not init as default */
	unsigned long prev_time; /* calculation for elapsed time(sec) */
	unsigned long remaining_time; /* remaining time (msec), when this time gets 0, then safety timer is expired */
	unsigned long rescaled_remaining_time; /* rescaled remaining time(msec) */
};

struct sec_battery_info {
	struct device *dev;
	sec_battery_platform_data_t *pdata;
	struct sec_ttf_data *ttf_d;
	struct sec_eoc_info *eoc_d;

	/* power supply used in Android */
	struct power_supply *psy_bat;
	struct power_supply *psy_usb;
	struct power_supply *psy_ac;
	struct power_supply *psy_wireless;
	struct power_supply *psy_pogo;
	struct power_supply *psy_otg;

	int pd_usb_attached;
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	struct notifier_block usb_typec_nb;
#else
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
	struct notifier_block batt_nb;
#endif
#endif
	struct notifier_block sb_nb;
	struct notifier_block hallic_nb;

	bool pdic_attach;
	bool pdic_ps_rdy;
	bool init_src_cap;
	SEC_PD_SINK_STATUS sink_status;
	SEC_PD_SINK_STATUS *psink_status;
	struct sec_bat_pdic_list pd_list;

	bool hv_pdo;
	bool update_pd_list;
	int d2d_auth;
	bool vpdo_src_boost;
	bool vpdo_ocp;
	int vpdo_auth_stat;
	int hp_d2d;

	bool is_sysovlo;
	bool is_vbatovlo;
	bool is_abnormal_temp;

	bool safety_timer_set;
	bool lcd_status;
	bool skip_swelling;
	bool wc_auth_retried;

	int status;
	int health;
	bool present;
	unsigned int charger_mode;

	int voltage_now;		/* cell voltage (mV) */
	int voltage_avg;		/* average voltage (mV) */
	int voltage_ocv;		/* open circuit voltage (mV) */
	int current_now;		/* current (mA) */
	int inbat_adc;			/* inbat adc */
	int current_avg;		/* average current (mA) */
	int current_max;		/* input current limit (mA) */
	int current_sys;		/* system current (mA) */
	int current_sys_avg;		/* average system current (mA) */
	int charge_counter;		/* remaining capacity (uAh) */
	int current_adc;

	int voltage_now_main;		/* pack voltage main battery (mV) */
	int voltage_now_sub;		/* pack voltage sub battery (mV) */
	int voltage_avg_main;		/* pack voltage main battery (mV) */
	int voltage_avg_sub;		/* pack voltage sub battery (mV) */
	int current_now_main;		/* current from main battery (mA) */
	int current_now_sub;		/* current from sub battery (mA) */
	int current_avg_main;		/* current from main battery (mA) */
	int current_avg_sub;		/* current from sub battery (mA) */
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	unsigned int limiter_check;
	bool set_lower_curr;
#if IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
	unsigned int recharging_voltage_main;
	unsigned int recharging_voltage_sub;
#endif
#endif

	unsigned int capacity;			/* SOC (%) */
#if (IS_ENABLED(CONFIG_DUAL_FUELGAUGE) || IS_ENABLED(CONFIG_DUAL_SBP))
	int chgin_sub;					/* input voltage of sub battery (mV) */
	unsigned int main_capacity;		/* MAIN SOC (%) */
	unsigned int sub_capacity;		/* SUB SOC (%) */
#endif

	unsigned int input_voltage;		/* CHGIN/WCIN input voltage (V) */
	unsigned int charge_power;		/* charge power (mW) */
	unsigned int max_charge_power;		/* max charge power (mW) */
	unsigned int pd_max_charge_power;		/* max charge power for pd (mW) */
	unsigned int pd_rated_power;		/* rated power for pd (W) */

	/* keep awake until monitor is done */
	struct wakeup_source *monitor_ws;
	struct workqueue_struct *monitor_wqueue;
	struct delayed_work monitor_work;
	unsigned int polling_count;
	unsigned int polling_time;
	bool polling_in_sleep;
	bool polling_short;

	struct delayed_work polling_work;
	struct alarm polling_alarm;
	ktime_t last_poll_time;

	struct cisd cisd;
	bool skip_cisd;
	bool usb_overheat_check;
	bool otg_check;
	bool d2d_check;
	int prev_volt;
	int prev_temp;
	int prev_jig_on;
	int enable_update_data;
	int prev_chg_on;

#if defined(CONFIG_WIRELESS_AUTH)
	sec_bat_misc_dev_t *misc_dev;
#endif

	/* battery check */
	unsigned int check_count;
	/* ADC check */
	unsigned int check_adc_count;
	unsigned int check_adc_value;

	/* health change check*/
	bool health_change;
	/* ovp-uvlo health check */
	int health_check_count;

	/* time check */
	unsigned long charging_retention_time; /* retention time of charger connection */
	unsigned long charging_start_time;
	unsigned long charging_passed_time;
	unsigned long charging_next_time;
	unsigned long charging_fullcharged_time;

	unsigned long wc_heating_start_time;
	unsigned long wc_heating_passed_time;

	/* chg temperature check */
	unsigned int chg_limit;
	unsigned int chg_limit_recovery_cable;
	unsigned int mix_limit;

	/* lrp temperature check */
	unsigned int lrp_limit;
	unsigned int lrp_step;

	/* temperature check */
	int temperature;	/* battery temperature */
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	bool test_max_current;
	bool test_charge_current;
#if IS_ENABLED(CONFIG_STEP_CHARGING)
	int test_step_condition;
#endif
#endif
	int temper_amb;		/* target temperature */
	int usb_temp;
	int chg_temp;		/* charger temperature */
	int wpc_temp;
	int sub_bat_temp;
	int usb_conn_status;
	int usb_protection_temp;
	int temp_gap_bat_usb;
	int dchg_temp;
	int blkt_temp;		/* blanket temperature(instead of batt temp in mix_temp func for tablet model) */

	int lrp; /* Linear Regression for Predicting Surface Temperature, this value is from SSRM */
	int lrp_test;
	unsigned int lrp_chg_src;

	int overheatlimit_threshold_backup; /* sec_bat_set_temp_control_test() */
	int overheatlimit_recovery_backup; /* sec_bat_set_temp_control_test() */
	int overheatlimit_threshold;
	int overheatlimit_recovery;
	int cold_cool3_thresh;
	int cool3_cool2_thresh;
	int cool2_cool1_thresh;
	int cool1_normal_thresh;
	int normal_warm_thresh;
	int warm_overheat_thresh;
	int thermal_zone;
	int bat_thm_count;
	int adc_init_count;

	/* charging */
	unsigned int charging_mode;
	bool is_recharging;
	int wdt_kick_disable;

	bool is_jig_on;
	int cable_type;
	int muic_cable_type;

	bool auto_mode;

	struct wakeup_source *cable_ws;
	struct delayed_work cable_work;
	struct wakeup_source *vbus_ws;
	struct wakeup_source *input_ws;
	struct delayed_work input_check_work;
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	struct delayed_work fw_init_work;
#endif
	struct delayed_work siop_level_work;
	struct wakeup_source *siop_level_ws;
	struct wakeup_source *wpc_tx_ws;
	struct delayed_work wpc_tx_work;
	struct wakeup_source *wpc_tx_en_ws;
	struct delayed_work wpc_tx_en_work;
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	struct delayed_work batt_data_work;
	struct wakeup_source *batt_data_ws;
	char *data_path;
#endif
#ifdef CONFIG_OF
	struct delayed_work parse_mode_dt_work;
	struct wakeup_source *parse_mode_dt_ws;
#endif
	struct delayed_work dev_init_work;
	struct wakeup_source *dev_init_ws;
	struct delayed_work afc_init_work;
	struct delayed_work usb_conn_check_work;
	struct wakeup_source *usb_conn_check_ws;
	struct delayed_work transit_clear_work;
	struct wakeup_source *transit_clear_ws;
	struct delayed_work mpla_thr_cnt_check_work;
	struct wakeup_source *mpla_thr_cnt_check_ws;

	char batt_type[48];
	unsigned int full_check_cnt;
	unsigned int recharge_check_cnt;

	unsigned int input_check_cnt;
	unsigned int usb_conn_check_cnt;
	bool run_usb_conn_check;

	struct mutex iolock;
	int input_current;
	int charging_current;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	unsigned int main_current;
	unsigned int sub_current;
#endif
	int topoff_condition;
	int wpc_vout_level;
	int wpc_max_vout_level;
	unsigned int current_event;

#if IS_ENABLED(CONFIG_BATTERY_AUTH_SLE956681)
	int vk_key_status;
#endif
	/* wireless charging enable */
	struct mutex wclock;
	bool wc_enable;
	int wc_enable_cnt;
	int wc_enable_cnt_value;
	int led_cover;
	int mag_cover;
	int wc_status;
	bool wc_cv_mode;
#if defined(CONFIG_WIRELESS_RX_PHM_CTRL)
	bool wc_rx_pdetb_mode; // phm state in case of rx mode with models which support pdet_b gpio
#else
	bool wc_rx_phm_mode; // phm state in case of rx mode with models which non-support pdet_b gpio
#endif
	bool wc_tx_phm_mode; // phm state in case of tx mode
	bool prev_tx_phm_mode; // prev phm state in case of tx mode
	bool wc_tx_adaptive_vout;
	bool wc_need_ldo_on;

	int wire_status;
#if IS_ENABLED(CONFIG_MTK_CHARGER) && !IS_ENABLED(CONFIG_VIRTUAL_MUIC)
	int bc12_cable;
#endif

	/* wireless tx */
	bool wc_tx_enable;
	bool wc_rx_connected;
	bool afc_disable;
	bool pd_disable;
	bool buck_cntl_by_tx;
	bool tx_switch_mode_change;
	int wc_tx_vout;
	bool uno_en;
	unsigned int wc_rx_type;
	unsigned int tx_minduty;
	unsigned int tx_ping_duty;
	unsigned int tx_switch_mode;
	unsigned int tx_switch_start_soc;

	unsigned int tx_mfc_iout;
	unsigned int tx_uno_iout;

	int pogo_status;
	bool pogo_9v;

	/* test mode */
	int test_mode;
	bool factory_mode;
	bool factory_mode_boot_on;
	bool display_test;
	bool store_mode;
#if defined(CONFIG_BC12_DEVICE) && defined(CONFIG_SEC_FACTORY)
	bool vbat_adc_open;
#endif

	/* usb suspend */
	int prev_usb_conf;

	/* MTBF test for CMCC */
	bool is_hc_usb;

	int siop_level;
	int stability_test;
	int eng_not_full_status;

	int wpc_temp_v2_offset;
	bool wpc_vout_ctrl_mode;
	char *hv_chg_name;
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	bool disable_mfc;
	bool nv_wc_temp_ctrl_skip;
	int tx_avg_curr;
	int tx_time_cnt;
	int tx_total_power;
	struct delayed_work wpc_txpower_calc_work;

	unsigned int wc20_vout;
	unsigned int wc20_power_class;
	unsigned int wc20_rx_power;
	struct delayed_work wc20_current_work;
	struct wakeup_source *wc20_current_ws;
#endif
	struct delayed_work slowcharging_work;
	int batt_cycle;
	int batt_asoc;
	int batt_full_status_usage;
#if IS_ENABLED(CONFIG_STEP_CHARGING)
	bool step_charging_skip_lcd_on;
	bool step_chg_en_in_factory;
	unsigned int step_chg_type;
	unsigned int step_chg_charge_power;
	int step_chg_status;
	int step_chg_step;
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	unsigned int wpc_step_chg_type;
	unsigned int wpc_step_chg_charge_power;
	int wpc_step_chg_step;
	int wpc_step_chg_status;
#endif
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	int dc_step_chg_step;
	unsigned int *dc_step_chg_type;
	unsigned int dc_step_chg_charge_power;

	bool dc_float_voltage_set;
	unsigned int dc_step_chg_iin_cnt;
#endif
#endif
	bool dchg_dc_in_swelling;
	struct mutex misclock;
	struct mutex txeventlock;
	unsigned int misc_event;
	unsigned int tx_event;
	unsigned int ext_event;
	unsigned int prev_misc_event;
	unsigned int tx_retry_case;
	unsigned int tx_misalign_cnt;
	unsigned int tx_ocp_cnt;
	struct delayed_work ext_event_work;
	struct delayed_work misc_event_work;
	struct wakeup_source *ext_event_ws;
	struct wakeup_source *misc_event_ws;
	struct wakeup_source *tx_event_ws;
	struct mutex batt_handlelock;
	struct mutex current_eventlock;
	struct mutex typec_notylock;
#if IS_ENABLED(CONFIG_MTK_CHARGER)  && !IS_ENABLED(CONFIG_VIRTUAL_MUIC)
	struct mutex bc12_notylock;
#endif
	struct mutex voutlock;
	unsigned long tx_misalign_start_time;
	unsigned long tx_misalign_passed_time;
	unsigned long tx_ocp_start_time;
	unsigned long tx_ocp_passed_time;

	unsigned int hiccup_status;
	bool hiccup_clear;

	struct sec_safety_timer safety_timer;
	int fg_reset;

	struct sec_vote *fcc_vote;
	struct sec_vote *input_vote;
	struct sec_vote *fv_vote;
	struct sec_vote *dc_fv_vote;
	struct sec_vote *chgen_vote;
	struct sec_vote *topoff_vote;
	struct sec_vote *iv_vote;
	struct sec_vote *dc_op_mode_vote;
	struct sec_vote *apdo_max_volt_vote;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	struct sec_vote *vlim_vote;
#endif

	struct sb_full_soc *fs;

	/* 25w ta alert */
	bool ta_alert_wa;
	int ta_alert_mode;

	bool sleep_mode;
	bool mfc_fw_update;

	int charging_night_mode;

	/* Linear Regression for Predicting Battery Temperature */
	unsigned long lr_start_time;
	unsigned long lr_time_span;
	int lr_bat_temp;
	int lr_bat_t_1;

	bool is_fpdo_dc;

#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
	bool usb_factory_init;
	int usb_factory_mode;
#if defined(CONFIG_SEC_FACTORY)
	bool usb_factory_slate_mode;
#endif
	unsigned int batt_f_mode;
#endif
	bool abnormal_ta;
	int srccap_transit_cnt;
	bool srccap_transit;
	int dc_check_cnt;
	bool usb_slow_chg;
	bool usb_bootcomplete;
	unsigned int flash_state;
	unsigned int mst_en;
#if IS_ENABLED(CONFIG_MTK_CHARGER)
	unsigned int mtk_fg_init;
#endif
#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
	struct notifier_block vbus_nb;
#endif
	bool is_otg_on;
	bool smart_sw_src;
	int dc_op_mode;
	int dc_ta_op_max_mode;
	int dc_ta_forced_op_max_mode;
};

enum {
	EXT_DEV_NONE = 0,
	EXT_DEV_GAMEPAD_CHG,
	EXT_DEV_GAMEPAD_OTG,
};

#if IS_ENABLED(CONFIG_MTK_CHARGER) && IS_ENABLED(CONFIG_AFC_CHARGER)
extern int afc_set_voltage(int vol);
#endif
extern unsigned int sec_bat_get_lpmode(void);
extern void sec_bat_set_lpmode(unsigned int value);
extern int sec_bat_get_fgreset(void);
extern int sec_bat_get_facmode(void);
extern unsigned int sec_bat_get_chgmode(void);
extern void sec_bat_set_chgmode(unsigned int value);
extern unsigned int sec_bat_get_dispd(void);
extern void sec_bat_set_dispd(unsigned int value);

extern int adc_read(struct sec_battery_info *battery, int channel);
extern void adc_init(struct platform_device *pdev, struct sec_battery_info *battery);
extern void adc_exit(struct sec_battery_info *battery);
extern void sec_cable_init(struct platform_device *pdev, struct sec_battery_info *battery);
extern int sec_bat_get_adc_data(struct device *dev, int adc_ch, int count, int batt_adc_type);
extern int sec_bat_get_charger_type_adc(struct sec_battery_info *battery);
extern bool sec_bat_convert_adc_to_val(int adc, int offset,
		sec_bat_adc_table_data_t *adc_table, int size, int *value);
extern int sec_bat_get_adc_value(struct sec_battery_info *battery, int channel);
extern int sec_bat_get_inbat_vol_by_adc(struct sec_battery_info *battery);
extern bool sec_bat_check_vf_adc(struct sec_battery_info *battery);
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
extern int sec_bat_get_direct_chg_temp_adc(struct sec_battery_info *battery, int adc_data, int count, int check_type);
#endif
extern void sec_bat_set_misc_event(struct sec_battery_info *battery, unsigned int misc_event_val, unsigned int misc_event_mask);
extern void sec_bat_set_tx_event(struct sec_battery_info *battery,
	unsigned int tx_event_val, unsigned int tx_event_mask);
extern void sec_bat_set_current_event(struct sec_battery_info *battery, unsigned int current_event_val, unsigned int current_event_mask);
extern void sec_bat_set_temp_control_test(struct sec_battery_info *battery, bool temp_enable);
extern void sec_bat_get_battery_info(struct sec_battery_info *battery);
extern int sec_bat_set_charging_current(struct sec_battery_info *battery);
extern void sec_bat_aging_check(struct sec_battery_info *battery);
#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
extern void sec_bat_aging_check_dual_battery_individual_fv(struct sec_battery_info *battery);
#endif
extern void sec_bat_reset_safety_timer(struct sec_battery_info *battery);
extern void sec_bat_set_threshold(struct sec_battery_info *battery, int cable_type);
extern void sec_bat_thermal_check(struct sec_battery_info *battery);
extern void sec_bat_set_charging_status(struct sec_battery_info *battery, int status);
extern int sec_bat_get_batt_capacity(struct sec_battery_info *battery, int old_capacity);
void sec_bat_set_health(struct sec_battery_info *battery, int status);
extern bool sec_bat_check_full(struct sec_battery_info *battery, int full_check_type);
extern bool sec_bat_check_fullcharged(struct sec_battery_info *battery);
extern void sec_bat_check_wpc_temp(struct sec_battery_info *battery, int ct, int siop_level);
extern void sec_bat_check_wpc_temp_v2(struct sec_battery_info *battery);
extern void sec_bat_check_mix_temp(struct sec_battery_info *battery, int ct, int siop_level, bool is_apdo);
extern void sec_bat_check_mix_temp_v2(struct sec_battery_info *battery);
extern void sec_bat_check_afc_temp(struct sec_battery_info *battery, int siop_level);
extern void sec_bat_check_pdic_temp(struct sec_battery_info *battery, int siop_level);
extern void sec_bat_check_direct_chg_temp(struct sec_battery_info *battery, int siop_level);
extern int sec_bat_check_power_type(int max_chg_pwr, int pd_max_chg_pwr, int ct, int ws, int is_apdo);
extern void sec_bat_check_lrp_temp(struct sec_battery_info *battery, int ct, int ws, int siop_level, bool lcd_sts);
extern void sec_bat_check_tx_temperature(struct sec_battery_info *battery);
extern void sec_bat_change_default_current(struct sec_battery_info *battery, int cable_type, int input, int output);
extern int sec_bat_set_charge(void *data, int chg_mode);
extern int lr_predict_bat_temp(struct sec_battery_info *battery, int batt_temp, int sub_bat_temp);
extern int get_chg_power_type(int ct, int ws, int pd_max_pw, int max_pw);
extern int sec_usb_conn_check(struct sec_battery_info *battery);
#if !defined(CONFIG_SEC_FACTORY)
extern void sec_bat_check_temp_ctrl_by_cable(struct sec_battery_info *battery);
extern bool sec_bat_is_unknown_wpc_temp(int wpc_temp, int usb_temp, bool loosened_unknown_temp);
extern void sec_bat_calc_unknown_wpc_temp(struct sec_battery_info *battery, int *batt_temp, int wpc_temp, int usb_temp);
#endif

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
extern void sec_bat_get_wireless_current(struct sec_battery_info *battery);
extern void sec_bat_mfc_work(struct work_struct *work);
extern int sec_bat_check_wc_available(struct sec_battery_info *battery);
extern bool sec_bat_hv_wc_normal_mode_check(struct sec_battery_info *battery);
extern void sec_bat_ext_event_work_content(struct sec_battery_info *battery);
extern void sec_bat_wpc_tx_work_content(struct sec_battery_info *battery);
extern void sec_bat_wpc_tx_en_work_content(struct sec_battery_info *battery);
extern void sec_bat_set_wc20_current(struct sec_battery_info *battery);
extern void sec_wireless_otg_vout_control(struct sec_battery_info *battery, int enable);
extern void sec_wireless_otg_icl_control(struct sec_battery_info *battery);
extern void sec_bat_set_mfc_off(struct sec_battery_info *battery, char flag, bool need_ept);
extern void sec_bat_set_mfc_on(struct sec_battery_info *battery, char flag);
extern int sec_bat_choose_cable_type(struct sec_battery_info *battery);
extern void sec_bat_handle_tx_misalign(struct sec_battery_info *battery, bool trigger_misalign);
extern void sec_bat_handle_tx_ocp(struct sec_battery_info *battery, bool trigger_ocp);
extern void sec_bat_wireless_minduty_cntl(struct sec_battery_info *battery, unsigned int duty_val);
extern void sec_bat_wireless_uno_cntl(struct sec_battery_info *battery, bool en);
extern void sec_bat_wireless_iout_cntl(struct sec_battery_info *battery, int uno_iout, int mfc_iout);
extern void sec_bat_wireless_vout_cntl(struct sec_battery_info *battery, int vout_now);
extern void sec_bat_check_tx_mode(struct sec_battery_info *battery);
extern void sec_bat_wc_cv_mode_check(struct sec_battery_info *battery);
extern void sec_bat_run_wpc_tx_work(struct sec_battery_info *battery, int work_delay);
extern void sec_bat_txpower_calc(struct sec_battery_info *battery);
extern void sec_wireless_set_tx_enable(struct sec_battery_info *battery, bool wc_tx_enable);
extern void sec_bat_check_wc_re_auth(struct sec_battery_info *battery);
extern unsigned int get_wc_vout_mW(unsigned int vout);
extern void sec_bat_mfc_ldo_cntl(struct sec_battery_info *battery, bool en);
extern int sec_bat_check_wpc_vout(struct sec_battery_info *battery, int ct, unsigned int chg_limit,
		int pre_vout, unsigned int evt);
#else
static inline void sec_bat_set_mfc_off(struct sec_battery_info *battery, char flag, bool need_ept) {}
static inline void sec_bat_set_mfc_on(struct sec_battery_info *battery, char flag) {}
#endif

#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
extern void sec_bat_fw_update(struct sec_battery_info *battery, int mode);
extern bool sec_bat_check_boost_mfc_condition(struct sec_battery_info *battery, int mode);
#endif

#if IS_ENABLED(CONFIG_STEP_CHARGING)
extern void sec_bat_reset_step_charging(struct sec_battery_info *battery);
extern void sec_step_charging_init(struct sec_battery_info *battery, struct device *dev);
extern bool sec_bat_check_step_charging(struct sec_battery_info *battery);
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
extern bool sec_bat_check_wpc_step_charging(struct sec_battery_info *battery);
#endif
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
extern bool sec_bat_check_dc_step_charging(struct sec_battery_info *battery);
extern void sec_update_dc_step_chg_dt(struct sec_battery_info *battery, int battery_id, int type);
extern int is_dc_higher_ratio_support(void);
#endif
void sec_bat_set_aging_info_step_charging(struct sec_battery_info *battery);
#endif

#if !defined(CONFIG_SEC_FACTORY)
bool sales_code_is(char *str);
#endif

#if defined(CONFIG_UPDATE_BATTERY_DATA)
extern int sec_battery_update_data(const char* file_path);
#endif
extern bool sec_bat_cisd_check(struct sec_battery_info *battery);
extern void sec_battery_cisd_init(struct sec_battery_info *battery);
extern void set_cisd_pad_data(struct sec_battery_info *battery, const char* buf);
extern void set_cisd_power_data(struct sec_battery_info *battery, const char* buf);
extern void set_cisd_pd_data(struct sec_battery_info *battery, const char *buf);
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
extern void set_cisd_dcerr_data(struct sec_battery_info *battery, const char* buf);
#endif

#if defined(CONFIG_WIRELESS_AUTH)
extern int sec_bat_misc_init(struct sec_battery_info *battery);
#endif

int sec_bat_parse_dt(struct device *dev, struct sec_battery_info *battery);
void sec_bat_parse_mode_dt(struct sec_battery_info *battery);
void sec_bat_parse_mode_dt_work(struct work_struct *work);
void sec_bat_check_battery_health(struct sec_battery_info *battery);
bool sec_bat_hv_wc_normal_mode_check(struct sec_battery_info *battery);
int sec_bat_get_temperature(struct device *dev, struct sec_bat_thm_info *info, int old_val,
		char *chg_name, char *fg_name, int batt_adc_type);
int sec_bat_get_inbat_vol_ocv(struct sec_battery_info *battery);
void sec_bat_smart_sw_src(struct sec_battery_info *battery, bool enable, int curr);
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
int sec_bat_dual_battery_vbat(struct sec_battery_info *battery, int battery_type);
#endif

#endif /* __SEC_BATTERY_H */
