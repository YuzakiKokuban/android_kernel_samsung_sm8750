/*
 * max77775_charger.h
 * Samsung max77775 Charger Header
 *
 * Copyright (C) 2015 Samsung Electronics, Inc.
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

#ifndef __MAX77775_CHARGER_H
#define __MAX77775_CHARGER_H __FILE__

#include <linux/mfd/core.h>
#include <linux/mfd/max77775.h>
#include <linux/mfd/max77775-private.h>
#include <linux/regulator/machine.h>
#include <linux/pm_wakeup.h>
#include "../../common/sec_charging_common.h"
#include <linux/types.h>
#include <linux/battery/sb_notify.h>

enum {
	CHIP_ID = 0,
	DATA,
};

enum {
	SHIP_MODE_DISABLE = 0,
	SHIP_MODE_EN_OP,
	SHIP_MODE_EN,
};

enum {
	CHGIN_SEL = 0,
	WCIN_SEL,
	WC_CHG_ALL_ON,
	WC_CHG_ALL_OFF,
};

ssize_t max77775_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t max77775_chg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define MAX77775_CHARGER_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = max77775_chg_show_attrs,			\
	.store = max77775_chg_store_attrs,			\
}

#define MAX77775_CHG_SAFEOUT2                0x80

/* MAX77775_CHG_REG_CHG_INT */
#define MAX77775_BYP_I                  (1 << 0)
#define MAX77775_INP_LIMIT_I		(1 << 1)
#define MAX77775_BATP_I                 (1 << 2)
#define MAX77775_BAT_I                  (1 << 3)
#define MAX77775_CHG_I                  (1 << 4)
#define MAX77775_WCIN_I                 (1 << 5)
#define MAX77775_CHGIN_I                (1 << 6)
#define MAX77775_AICL_I                 (1 << 7)

/* MAX77775_CHG_REG_CHG_INT_MASK */
#define MAX77775_BYP_IM                 (1 << 0)
#define MAX77775_INP_LIMIT_IM		(1 << 1)
#define MAX77775_BATP_IM                (1 << 2)
#define MAX77775_BAT_IM                 (1 << 3)
#define MAX77775_CHG_IM                 (1 << 4)
#define MAX77775_WCIN_IM                (1 << 5)
#define MAX77775_CHGIN_IM               (1 << 6)
#define MAX77775_AICL_IM                (1 << 7)

/* MAX77775_CHG_REG_CHG_INT_OK */
#define MAX77775_BYP_OK                 0x01
#define MAX77775_BYP_OK_SHIFT           0
#define MAX77775_DISQBAT_OK		0x02
#define MAX77775_DISQBAT_OK_SHIFT	1
#define MAX77775_BATP_OK		0x04
#define MAX77775_BATP_OK_SHIFT		2
#define MAX77775_BAT_OK                 0x08
#define MAX77775_BAT_OK_SHIFT           3
#define MAX77775_CHG_OK                 0x10
#define MAX77775_CHG_OK_SHIFT           4
#define MAX77775_WCIN_OK		0x20
#define MAX77775_WCIN_OK_SHIFT		5
#define MAX77775_CHGIN_OK               0x40
#define MAX77775_CHGIN_OK_SHIFT         6
#define MAX77775_AICL_OK                0x80
#define MAX77775_AICL_OK_SHIFT          7
#define MAX77775_DETBAT                 0x04
#define MAX77775_DETBAT_SHIFT           2

/* MAX77775_CHG_REG_CHG_DTLS_00 */
#define MAX77775_SYS_REG_OK				0x01
#define MAX77775_WCIN_DTLS		0x18
#define MAX77775_WCIN_DTLS_SHIFT	3
#define MAX77775_CHGIN_DTLS             0x60
#define MAX77775_CHGIN_DTLS_SHIFT       5
#define MAX77775_SPSN_DTLS		0x06
#define MAX77775_SPSN_DTLS_SHIFT	1

/* MAX77775_CHG_REG_CHG_DTLS_01 */
#define MAX77775_CHG_DTLS               0x0F
#define MAX77775_CHG_DTLS_SHIFT         0
#define MAX77775_BAT_DTLS               0x70
#define MAX77775_BAT_DTLS_SHIFT         4

/* MAX77775_CHG_REG_CHG_DTLS_02 */
#define MAX77775_BYP_DTLS               0x0F
#define MAX77775_BYP_DTLS_SHIFT         0
#define MAX77775_BYP_DTLS0      0x1
#define MAX77775_BYP_DTLS1      0x2
#define MAX77775_BYP_DTLS2      0x4
#define MAX77775_BYP_DTLS3      0x8

#if 1
/* MAX77775_CHG_REG_CHG_CNFG_00 */
#define CHG_CNFG_00_MODE_SHIFT		        0
#define CHG_CNFG_00_CHG_SHIFT		        0
#define CHG_CNFG_00_WDTEN_SHIFT		        4
#define CHG_CNFG_00_MODE_MASK		        (0x0F << CHG_CNFG_00_MODE_SHIFT)
#define CHG_CNFG_00_CHG_MASK		        (1 << CHG_CNFG_00_CHG_SHIFT)
#define CHG_CNFG_00_WDTEN_MASK		        (1 << CHG_CNFG_00_WDTEN_SHIFT)
#define MAX77775_MODE_BOOST			0x08
#endif
#define CHG_CNFG_00_SPSN_DET_EN_SHIFT		7
#define CHG_CNFG_00_SPSN_DET_EN_MASK		(1 << CHG_CNFG_00_SPSN_DET_EN_SHIFT)
#define MAX77775_SPSN_DET_ENABLE		0x01
#define MAX77775_SPSN_DET_DISABLE		0x00

/* MAX77775_CHG_REG_CHG_CNFG_00 MODE[3:0] */
#define MAX77775_MODE_0_ALL_OFF						0x0
#define MAX77775_MODE_1_BUCK_OFF_LINEAR_CHG_ON		0x1
#define MAX77775_MODE_2_BUCK_LINEAR_CHG_ON_BOOST_UNO_ON	0x2
#define MAX77775_MODE_3_BUCK_LINEAR_CHG_ON_BOOST_OTG_ON 0x3
#define MAX77775_MODE_4_BUCK_ON						0x4
#define MAX77775_MODE_5_BUCK_CHG_ON					0x5
#define MAX77775_MODE_7_BUCK_LINEAR_CHG_ON			0x7
#define MAX77775_MODE_8_BOOST_UNO_ON				0x8
#define MAX77775_MODE_9_BOOST_ON					0x9
#define MAX77775_MODE_A_BOOST_OTG_ON				0xA
#define MAX77775_MODE_B_BOOST_OTG_UNO_ON				0xB
#define MAX77775_MODE_C_BUCK_BOOST_UNO_ON				0xC
#define MAX77775_MODE_D_BUCK_CHG_BOOST_UNO_ON			0xD
#define MAX77775_MODE_E_BUCK_BOOST_OTG_ON				0xE
#define MAX77775_MODE_F_BUCK_CHG_BOOST_OTG_ON			0xF

/* MAX77775_CHG_REG_CHG_CNFG_01 */
#define CHG_CNFG_01_FCHGTIME_SHIFT			0
#define CHG_CNFG_01_FCHGTIME_MASK			(0x7 << CHG_CNFG_01_FCHGTIME_SHIFT)
#define MAX77775_FCHGTIME_DISABLE			0x0

#define CHG_CNFG_01_RECYCLE_EN_SHIFT	3
#define CHG_CNFG_01_RECYCLE_EN_MASK	(0x1 << CHG_CNFG_01_RECYCLE_EN_SHIFT)
#define MAX77775_RECYCLE_EN_ENABLE	0x1

#define CHG_CNFG_01_CHG_RSTRT_SHIFT	4
#define CHG_CNFG_01_CHG_RSTRT_MASK	(0x3 << CHG_CNFG_01_CHG_RSTRT_SHIFT)
#define MAX77775_CHG_RSTRT_DISABLE	0x3

#define CHG_CNFG_01_VTRICKLE_EN_SHIFT		7
#define CHG_CNFG_01_VTRICKLE_EN_MASK		(0x1 << CHG_CNFG_01_VTRICKLE_EN_SHIFT)
#define MAX77775_CHG_VTRICKLE_EN_DISABLE	0x0
#define MAX77775_CHG_VTRICKLE_EN_ENABLE		0x1

/* MAX77775_CHG_REG_CHG_CNFG_02 */
#define CHG_CNFG_02_OTG_ILIM_SHIFT		6
#define CHG_CNFG_02_OTG_ILIM_MASK		(0x3 << CHG_CNFG_02_OTG_ILIM_SHIFT)
#define MAX77775_OTG_ILIM_500		0x0
#define MAX77775_OTG_ILIM_900		0x1
#define MAX77775_OTG_ILIM_1200		0x2
#define MAX77775_OTG_ILIM_1500		0x3
#define MAX77775_CHG_CC                         0x3F

/* MAX77775_CHG_REG_CHG_CNFG_03 */
#define CHG_CNFG_03_TO_ITH_SHIFT	0
#define CHG_CNFG_03_TO_ITH_MASK		(0xF << CHG_CNFG_03_TO_ITH_SHIFT)
#define MAX77775_TO_ITH_150MA		0x0

#define CHG_CNFG_03_TO_TIME_SHIFT	4
#define CHG_CNFG_03_TO_TIME_MASK	(0x3 << CHG_CNFG_03_TO_TIME_SHIFT)
#define MAX77775_TO_TIME_30M		0x0
#define MAX77775_TO_TIME_40M		0x1
#define MAX77775_TO_TIME_50M		0x2
#define MAX77775_TO_TIME_70M		0x3

#define CHG_CNFG_03_SYS_TRACK_DIS_SHIFT		7
#define CHG_CNFG_03_SYS_TRACK_DIS_MASK		(0x1 << CHG_CNFG_03_SYS_TRACK_DIS_SHIFT)
#define MAX77775_SYS_TRACK_ENABLE	        0x0
#define MAX77775_SYS_TRACK_DISABLE	        0x1

/* MAX77775_CHG_REG_CHG_CNFG_04 */
#define MAX77775_CHG_MINVSYS_MASK               0xC0
#define MAX77775_CHG_MINVSYS_SHIFT		6
#define MAX77775_CHG_PRM_MASK                   0x1F
#define MAX77775_CHG_PRM_SHIFT                  0

#define CHG_CNFG_04_CHG_CV_PRM_SHIFT            0
#define CHG_CNFG_04_CHG_CV_PRM_MASK             (0x3F << CHG_CNFG_04_CHG_CV_PRM_SHIFT)

/* MAX77775_CHG_REG_CHG_CNFG_05 */
#define CHG_CNFG_05_REG_B2SOVRC_SHIFT	0
#define CHG_CNFG_05_REG_B2SOVRC_MASK	(0xF << CHG_CNFG_05_REG_B2SOVRC_SHIFT)
#define MAX77775_B2SOVRC_DISABLE	0x0
#define CHG_CNFG_05_REG_B2SOVRC_RST	0x01

#define CHG_CNFG_05_REG_UNOILIM_SHIFT	4
#define CHG_CNFG_05_REG_UNOILIM_MASK	(0x7 << CHG_CNFG_05_REG_UNOILIM_SHIFT)
#define MAX77775_UNOILIM_600			0x1
#define MAX77775_UNOILIM_800			0x2
#define MAX77775_UNOILIM_1000			0x3
#define MAX77775_UNOILIM_1200			0x4
#define MAX77775_UNOILIM_1400			0x5
#define MAX77775_UNOILIM_1600			0x6
#define MAX77775_UNOILIM_2000			0x7


/* MAX77775_CHG_CNFG_06 */
#define CHG_CNFG_06_WDTCLR_SHIFT		0
#define CHG_CNFG_06_WDTCLR_MASK			(0x3 << CHG_CNFG_06_WDTCLR_SHIFT)
#define MAX77775_WDTCLR					0x01
#define CHG_CNFG_06_DIS_AICL_SHIFT		4
#define CHG_CNFG_06_DIS_AICL_MASK		(0x1 << CHG_CNFG_06_DIS_AICL_SHIFT)
#define MAX77775_DIS_AICL				0x0
#define CHG_CNFG_06_B2SOVRC_DTC_SHIFT	7
#define CHG_CNFG_06_B2SOVRC_DTC_MASK	(0x1 << CHG_CNFG_06_B2SOVRC_DTC_SHIFT)
#define MAX77775_B2SOVRC_DTC_100MS		0x1

/* MAX77775_CHG_REG_CHG_CNFG_07 */
#define MAX77775_CHG_FMBST				0x04
#define CHG_CNFG_07_REG_FMBST_SHIFT		2
#define CHG_CNFG_07_REG_FMBST_MASK		(0x1 << CHG_CNFG_07_REG_FMBST_SHIFT)
#define CHG_CNFG_07_REG_FGSRC_SHIFT		1
#define CHG_CNFG_07_REG_FGSRC_MASK		(0x1 << CHG_CNFG_07_REG_FGSRC_SHIFT)
#define CHG_CNFG_07_REG_SHIPMODE_SHIFT		0
#define CHG_CNFG_07_REG_SHIPMODE_MASK		(0x1 << CHG_CNFG_07_REG_SHIPMODE_SHIFT)

/* MAX77775_CHG_REG_CHG_CNFG_08 */
#define CHG_CNFG_08_REG_FSW_SHIFT	0
#define CHG_CNFG_08_REG_FSW_MASK	(0x3 << CHG_CNFG_08_REG_FSW_SHIFT)
#define MAX77775_CHG_FSW_3MHz		0x00
#define MAX77775_CHG_FSW_2MHz		0x01
#define MAX77775_CHG_FSW_1_5MHz		0x02
#define CHG_CNFG_08_REG_VOTG_SHIFT	4
#define CHG_CNFG_08_REG_VOTG_MASK	(0x3 << CHG_CNFG_08_REG_VOTG_SHIFT)
#define CHG_CNFG_08_REG_SPSN_DET_SHIFT	2
#define CHG_CNFG_08_REG_SPSN_DET_MASK	(0x3 << CHG_CNFG_08_REG_SPSN_DET_SHIFT)
#define MAX77775_CHG_SPSN_1K		0x00
#define MAX77775_CHG_SPSN_10K		0x01
#define MAX77775_CHG_SPSN_100K		0x02
#define MAX77775_CHG_SPSN_OPEN		0x03
#define CHG_CNFG_08_BYPASS_MODE_SHIFT	7
#define CHG_CNFG_08_BYPASS_MODE_MASK	(0x1 << CHG_CNFG_08_BYPASS_MODE_SHIFT)

/* MAX77775_CHG_REG_CHG_CNFG_09 */
#define MAX77775_CHG_CHGIN_LIM                  0x7F
#define MAX77775_CHG_EN                         0x80

/* MAX77775_CHG_REG_CHG_CNFG_10 */
#define MAX77775_CHG_WCIN_LIM                   0x3F

/* MAX77775_CHG_REG_CHG_CNFG_11 */
#define CHG_CNFG_11_VBYPSET_SHIFT		0
#define CHG_CNFG_11_VBYPSET_MASK		(0xFF << CHG_CNFG_11_VBYPSET_SHIFT)

/* MAX77775_CHG_REG_CHG_CNFG_12 */
#define MAX77775_CHG_WCINSEL			0x40
#define CHG_CNFG_12_CHGINSEL_SHIFT		5
#define CHG_CNFG_12_CHGINSEL_MASK		(0x1 << CHG_CNFG_12_CHGINSEL_SHIFT)
#define CHG_CNFG_12_WCINSEL_SHIFT		6
#define CHG_CNFG_12_WCINSEL_MASK		(0x1 << CHG_CNFG_12_WCINSEL_SHIFT)
#define CHG_CNFG_12_VCHGIN_REG_MASK		(0x3 << 3)
#define CHG_CNFG_12_WCIN_REG_MASK		(0x3 << 1)
#define CHG_CNFG_12_REG_DISKIP_SHIFT	0
#define CHG_CNFG_12_REG_DISKIP_MASK		(0x1 << CHG_CNFG_12_REG_DISKIP_SHIFT)
#define MAX77775_DISABLE_SKIP			0x1
#define MAX77775_AUTO_SKIP				0x0

/* MAX77775_CHG_REG_CHG_CNFG_13 */
#define CHG_CNFG_13_AUTOSHIP_TH_SHIFT			6
#define CHG_CNFG_13_AUTOSHIP_TH_MASK			(0x3 << CHG_CNFG_13_AUTOSHIP_TH_SHIFT)
#define CHG_CNFG_13_SHIP_EXT_DB_SHIFT			4
#define CHG_CNFG_13_SHIP_EXT_DB_MASK			(0x3 << CHG_CNFG_13_SHIP_EXT_DB_SHIFT)
#define CHG_CNFG_13_REG_AUTO_SHIPMODE_SHIFT		3
#define CHG_CNFG_13_REG_AUTO_SHIPMODE_MASK		(0x1 << CHG_CNFG_13_REG_AUTO_SHIPMODE_SHIFT)
#define CHG_CNFG_13_BYPASS_AUTOSHIP_EN_SHIFT		2
#define CHG_CNFG_13_BYPASS_AUTOSHIP_EN_MASK		(0x1 << CHG_CNFG_13_BYPASS_AUTOSHIP_EN_SHIFT)
#define CHG_CNFG_13_SHIP_ENTRY_DB_MASK			0x3

/* MAX77775_CHG_REG_CNFG_15 */
#define CHG_CNFG_15_SCP_DIS_SHIFT			5
#define CHG_CNFG_15_SCP_DIS_MASK			(0x1 << CHG_CNFG_15_SCP_DIS_SHIFT)

/* MAX77775_CHG_REG_CHG_CNFG_14 */
#define CHG_CNFG_14_OCPWARN_POL_SHIFT		5
#define CHG_CNFG_14_OCPWARN_POL_MASK		(0x1 << CHG_CNFG_14_OCPWARN_POL_SHIFT)
#define CHG_CNFG_14_OCPWARN_EN_SHIFT		4
#define CHG_CNFG_14_OCPWARN_EN_MASK			(0x1 << CHG_CNFG_14_OCPWARN_EN_SHIFT)
#define CHG_CNFG_14_OCPWARN_TH_MASK			0xF
#define OCPWARN_TH_2_8A						0x1
#define OCPWARN_TH_3_6A						0x2
#define OCPWARN_TH_4_4A						0x3
#define OCPWARN_TH_5_2A						0x4
#define OCPWARN_TH_6_0A						0x5
#define OCPWARN_TH_6_8A						0x6
#define OCPWARN_TH_7_6A						0x7
#define OCPWARN_TH_8_4A						0x8
#define OCPWARN_TH_9_2A						0x9
#define OCPWARN_TH_10_0A					0xA
#define OCPWARN_TH_10_8A					0xB
#define OCPWARN_TH_11_6A					0xC
#define OCPWARN_TH_12_4A					0xD
#define OCPWARN_TH_12_8A					0xE
#define OCPWARN_TH_13_0A					0xF

/* MAX77775_CHG_REG_CHG_CNFG_16 */
#define CHG_CNFG_16_REBALANCE_DIS_SHIFT		7
#define CHG_CNFG_16_REBALANCE_DIS_MASK		(0x1 << CHG_CNFG_16_REBALANCE_DIS_SHIFT)
#define CHG_CNFG_16_BATTPROT_EN_SHIFT		6
#define CHG_CNFG_16_BATTPROT_EN_MASK		(0x1 << CHG_CNFG_16_BATTPROT_EN_SHIFT)
#define CHG_CNFG_16_VTRACK_SHIFT			1
#define CHG_CNFG_16_VTRACK_MASK				(0x1F << CHG_CNFG_16_VTRACK_SHIFT)
#define CHG_CNFG_16_EXT_SYS_SHIFT			0
#define CHG_CNFG_16_EXT_SYS_MASK			(0x1 << CHG_CNFG_16_EXT_SYS_SHIFT)

/* MAX77775_CHG_REG_CHG_CNFG_17 */
#define CHG_CNFG_17_OCPWARN_CC_SHIFT			7
#define CHG_CNFG_17_OCPWARN_CC_MASK				(0x1 << CHG_CNFG_17_OCPWARN_CC_SHIFT)
#define CHG_CNFG_17_EN_CTP_BAT_PROT_SHIFT		4
#define CHG_CNFG_17_WCIN_OVLO_DIS_SHIFT			2
#define CHG_CNFG_17_WCIN_OVLO_DIS_MASK			(0x1 << CHG_CNFG_17_WCIN_OVLO_DIS_SHIFT)
#define CHG_CNFG_17_EN_CTP_BAT_PROT_MASK		(0x1 << CHG_CNFG_17_EN_CTP_BAT_PROT_SHIFT)
#define CHG_CNFG_17_BATTPROT_WR_EN_SHIFT		1
#define CHG_CNFG_17_BATTPROT_WR_EN_MASK		(0x1 << CHG_CNFG_17_BATTPROT_WR_EN_SHIFT)
#define CHG_CNFG_17_BYPASS_MODE_WR_EN_SHIFT		0
#define CHG_CNFG_17_BYPASS_MODE_WR_EN_MASK		(0x1 << CHG_CNFG_17_BYPASS_MODE_WR_EN_SHIFT)

#define REDUCE_CURRENT_STEP						100
#define MINIMUM_INPUT_CURRENT					300

#define WC_CURRENT_STEP		100
#define WC_CURRENT_START	480

#define WC_DEFAULT_CURRENT 0x10

#define DPM_MISC 0x4000	/* BATT_MISC_EVENT_DIRECT_POWER_MODE = 0x00004000 */

typedef struct max77775_charger_platform_data {
	/* wirelss charger */
	char *wireless_charger_name;
	int wireless_cc_cv;
	int wc_current_step;
	unsigned int nv_wc_headroom;

	/* float voltage (mV) */
	int chg_float_voltage;
	int chg_irq;
	unsigned int chg_ocp_current;
	unsigned int chg_ocp_dtc;
	unsigned int topoff_time;
	int fac_vsys;
	bool enable_noise_wa;
	bool factory_wcin_irq;
	bool user_wcin_irq;
	bool enable_sysovlo_irq;
	bool boosting_voltage_aicl;
	int fsw;
	bool enable_dpm;
	int disqbat;
	int dpm_icl;
	bool icl_tolerance_wa;
#ifdef CONFIG_IFPMIC_LIMITER
	int default_vtrack_voltage;
	int vtrack_rs;
#endif

	/* OVP/UVLO check */
	int ovp_uvlo_check_type;
	/* 1st full check */
	int full_check_type;
	/* 2nd full check */
	int full_check_type_2nd;

	bool bar_type;
	bool disable_wcin_ovlo;
	bool ocpwarn;
	int ship_entry_db_time;
} max77775_charger_platform_data_t;

struct max77775_charger_data {
	struct device           *dev;
	struct i2c_client       *i2c;
	struct i2c_client       *pmic_i2c;

	struct max77775_platform_data *max77775_pdata;

	struct power_supply	*psy_chg;
	struct power_supply	*psy_otg;

	atomic_t	shutdown_cnt;

	struct workqueue_struct *wqueue;
	struct delayed_work	aicl_work;
	struct delayed_work	isr_work;
	struct delayed_work wc_current_work;
	struct delayed_work wc_chg_current_work;
	struct delayed_work chgin_work;
#if defined(CONFIG_USE_POGO)
	struct delayed_work	wcin_det_work;
#endif
	struct delayed_work set_icl_wcin_otg_work;
	struct delayed_work ocp_warn_work;

	/* mutex */
	struct mutex            charger_mutex;
	struct mutex            mode_mutex;
	struct mutex            icl_mutex;
	struct mutex            irq_aicl_mutex;
	struct mutex            otg_mutex;

	/* wakelock */
	struct wakeup_source *wc_current_ws;
	struct wakeup_source *aicl_ws;
	struct wakeup_source *otg_ws;
	struct wakeup_source *sysovlo_ws;
	struct wakeup_source *wc_chg_current_ws;
#if defined(CONFIG_USE_POGO)
	struct wakeup_source *wcin_det_ws;
#endif
	struct wakeup_source *set_icl_wcin_otg_ws;
	struct wakeup_source *ocp_warn_ws;

	unsigned int	is_charging;
	unsigned int	cable_type;
	unsigned int	input_current;
	unsigned int	charging_current;
	unsigned int	vbus_state;
	unsigned int	chgin_dtls;
	int		aicl_curr;
	bool	slow_charging;
	int		status;
	int		charge_mode;
	u8		cnfg00_mode;
	int		fsw_now;

	bool		bat_det;
	int		irq_bypass;
	int		irq_batp;
#if defined(CONFIG_MAX77775_CHECK_B2SOVRC)
	int		irq_bat;
#endif
	int		irq_chgin;
	int		irq_aicl;
	int irq_sysovlo;
#if defined(CONFIG_USE_POGO)
	int irq_wcin;
#endif
	int		wc_current;
	int		wc_pre_current;

	bool	jig_low_active;
	int		jig_gpio;

	bool otg_on;
	bool uno_on;

	int pmic_ver;
	int float_voltage;

	int misalign_cnt;
	bool hp_otg;
	bool bypass_mode;

	int dpm_last_icl;

	bool cv_mode_check;
	int ari_cnt;
	bool spcom;
	bool wdt_test;
	bool pr_swap;

	max77775_charger_platform_data_t *pdata;
};

#endif /* __MAX77775_CHARGER_H */
