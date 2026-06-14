/*
 * max77775_fuelgauge.h
 * Samsung max77775 Fuel Gauge Header
 *
 * Copyright (C) 2015 Samsung Electronics, Inc.
 *
 * This software is 77854 under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MAX77775_FUELGAUGE_H
#define __MAX77775_FUELGAUGE_H __FILE__

#include <linux/mfd/core.h>
#include <linux/mfd/max77775.h>
#include <linux/mfd/max77775-private.h>
#include <linux/regulator/machine.h>
#include <linux/pm_wakeup.h>
#include "../../common/sec_charging_common.h"
#include <linux/types.h>

/* Client address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */

#define PRINT_COUNT	10

#define ALERT_EN 0x04
#define CAPACITY_SCALE_DEFAULT_CURRENT 1000
#define CAPACITY_SCALE_HV_CURRENT 600

#define FG_BATT_DUMP_SIZE 128

enum max77775_vempty_mode {
	VEMPTY_MODE_HW = 0,
	VEMPTY_MODE_SW,
	VEMPTY_MODE_SW_VALERT,
	VEMPTY_MODE_SW_RECOVERY,
};

enum {
	FG_DATA,
};

ssize_t max77775_fg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t max77775_fg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define MAX77775_FG_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0660},	\
	.show = max77775_fg_show_attrs,			\
	.store = max77775_fg_store_attrs,			\
}

struct sec_fg_info {
	/* test print count */
	int pr_cnt;
	/* full charge comp */
	struct delayed_work	full_comp_work;

	/* battery info */
	u32 soc;

	/* miscellaneous */
	unsigned long fullcap_check_interval;
	int full_check_flag;
	bool is_first_check;
};

enum {
	FG_LEVEL = 0,
	FG_TEMPERATURE,
	FG_VOLTAGE,
	FG_CURRENT,
	FG_CURRENT_AVG,
	FG_CHECK_STATUS,
	FG_RAW_SOC,
	FG_VF_SOC,
	FG_AV_SOC,
	FG_FULLCAP,
	FG_FULLCAPNOM,
	FG_FULLCAPREP,
	FG_MIXCAP,
	FG_AVCAP,
	FG_REPCAP,
	FG_CYCLE,
	FG_QH,
	FG_QH_VF_SOC,
	FG_ISYS,
	FG_ISYS_AVG,
	FG_VSYS,
	FG_IIN,
	FG_VBYP,
};

enum {
	POSITIVE = 0,
	NEGATIVE,
};

enum {
	RANGE = 0,
	SLOPE,
	OFFSET,
	TABLE_MAX
};

#define CURRENT_RANGE_MAX_NUM	5

struct battery_data_t {
	u8 battery_id;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
#if defined(CONFIG_ID_USING_BAT_SUBBAT)
	u8 main_battery_id;
#endif
	u8 sub_battery_id;
#endif
	u32 V_empty;
	u32 V_empty_origin;
	u32 sw_v_empty_vol;
	u32 sw_v_empty_vol_cisd;
	u32 sw_v_empty_recover_vol;
	u32 Capacity;
	u8  *type_str;
	u32 ichgterm;
	u32 misccfg;
	u32 fullsocthr;
	u32 ichgterm_2nd;
	u32 misccfg_2nd;
	u32 fullsocthr_2nd;
	u32 coff_origin;
	u32 coff_charging;
	u32 cgain_origin;
	u32 cgain_charging;
	u32 low_temp_filtercfg;
	u32 normal_temp_filtercfg;
	u32 trigger_low_temp_wa;
	u32 recv_low_temp_wa;
};

/* FullCap learning setting */
#define VFFULLCAP_CHECK_INTERVAL	300 /* sec */
/* soc should be 0.1% unit */
#define VFSOC_FOR_FULLCAP_LEARNING	950
#define LOW_CURRENT_FOR_FULLCAP_LEARNING	20
#define HIGH_CURRENT_FOR_FULLCAP_LEARNING	120
#define LOW_AVGCURRENT_FOR_FULLCAP_LEARNING	20
#define HIGH_AVGCURRENT_FOR_FULLCAP_LEARNING	100

/* power off margin */
/* soc should be 0.1% unit */
#define POWER_OFF_SOC_HIGH_MARGIN	20
#define POWER_OFF_VOLTAGE_HIGH_MARGIN	3500
#define POWER_OFF_VOLTAGE_LOW_MARGIN	3400

#define LEARNING_QRTABLE 0x0001

/* Need to be increased if there are more than 2 BAT ID GPIOs */
#define BAT_GPIO_NO	2

typedef struct max77775_fuelgauge_platform_data {
	int jig_irq;
	int jig_gpio;
	int jig_low_active;

	int bat_id_gpio[BAT_GPIO_NO];
	int bat_gpio_cnt;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	int sub_bat_id_gpio[BAT_GPIO_NO];
	int sub_bat_gpio_cnt;
#endif
	int thermal_source;

	/* fuel alert SOC (-1: not use) */
	int fuel_alert_soc;
	int fuel_alert_vol;
	/* fuel alert can be repeated */
	bool repeated_fuelalert;
	int capacity_calculation_type;
	/* soc should be soc x 10 (0.1% degree)
	 * only for scaling
	 */
	int capacity_max;
	int capacity_max_margin;
	int capacity_min;
	unsigned int full_condition_soc;
} max77775_fuelgauge_platform_data_t;

#define FG_RESET_DATA_COUNT		5

struct verify_reg {
	u16 addr;
	u32 data;
};

struct fg_reset_wa {
	u32 fullcapnom;
	u32 dPacc;
	u32 dQacc;
	u32 rcomp0;
	u32 tempco;
};

struct lost_soc_data {
	/* dt data */
	int trig_soc; /* default 10% */
	int trig_d_soc; /* delta soc, default 2% */
	int trig_scale; /* default 2x */
	int guarantee_soc; /* default 2% */
	int min_vol; /* default 3200mV */
	int min_weight; /* default 2.0 */

	/* data */
	bool ing;
	int prev_raw_soc;
	int prev_remcap;
	int prev_qh;
	int lost_cap;
	int weight;
};

struct max77775_fuelgauge_data {
	struct device           *dev;
	struct i2c_client       *i2c;
	struct i2c_client       *pmic;
	struct mutex            fuelgauge_mutex;
	struct max77775_platform_data *max77775_pdata;
	max77775_fuelgauge_platform_data_t *pdata;
	struct power_supply	      *psy_fg;
	struct delayed_work isr_work;

	atomic_t	shutdown_cnt;

	int cable_type;
	bool is_charging;

	/* HW-dedicated fuel gauge info structure
	 * used in individual fuel gauge file only
	 * (ex. dummy_fuelgauge.c)
	 */
	struct sec_fg_info	info;
	struct battery_data_t        *battery_data;

	bool is_fuel_alerted;
	struct wakeup_source *fuel_alert_ws;

	unsigned int capacity_old;	/* only for atomic calculation */
	unsigned int capacity_max;	/* only for dynamic calculation */
	unsigned int g_capacity_max;	/* only for dynamic calculation */
	unsigned int standard_capacity;

	bool capacity_max_conv;
	bool initial_update_of_soc;
	bool initial_update_of_alert;
	bool sleep_initial_update_of_soc;
	struct mutex fg_lock;

	/* register programming */
	int reg_addr;
	u8 reg_data[2];

	int fg_irq;

	int raw_capacity;
	int current_now;
	int current_avg;

#if defined(CONFIG_UI_SOC_PROLONGING)
	int prev_raw_soc;
#endif

	bool using_temp_compensation;
	bool using_hw_vempty;
	unsigned int vempty_mode;
	int temperature;
	bool vempty_init_flag;

	int low_temp_limit;

	int vempty_recover_time;
	unsigned long vempty_time;

	u32 fg_resistor;

	struct fg_reset_wa *fg_reset_data;
	struct verify_reg *verify_selected_reg;
	unsigned int verify_selected_reg_length;
	u32 data_ver;
	bool skip_fg_verify;
	u32 err_cnt;
	u32 q_res_table[4]; /* QResidual Table */

	bool valert_count_flag;
	struct lost_soc_data lost_soc;
	char d_buf[128];
	int bd_vfocv;
	int bd_raw_soc;

	int multiplier; /* Multiply capacity values for PASS4 pmic rev */
	int pmic_ver;
};

#endif /* __MAX77775_FUELGAUGE_H */
