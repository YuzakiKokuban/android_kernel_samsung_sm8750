/*
 * sm5443_charger.h - SM5443 Charger device driver for SAMSUNG platform
 *
 * Copyright (C) 2023 SiliconMitus Co.Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sm5443_direct_charger.h"

#ifndef __SM5443_CHARGER_H__
#define __SM5443_CHARGER_H__

#define SM5443_I2C_HID_ADDR     0x1C

#define SM5443_TA_MIN_CURRENT   1000
#define SM5443_WPC_MIN_CURRENT  400
#define SM5443_CV_OFFSET        90
#define SM5443_CI_OFFSET        100
#define SM5443_VBATREG_OFFS     40
#define SM5443_PRESET_VBATREG   4440
#define SM5443_CV_VBATREG       4450
#define SM5443_FG_VNOW_MAX      4460

/* TA Type*/
enum {
	SM5443_TA_UNKNOWN,
	SM5443_TA_USBPD,
	SM5443_TA_WIRELESS,
	SM5443_TA_USBPD_2P0,
};

enum SM5443_flag1_desc {
	SM5443_FLAG1_PMID2VOUT_OVP  = 1 << 7,
	SM5443_FLAG1_PMID2VOUT_UVP  = 1 << 6,
	SM5443_FLAG1_VINPD          = 1 << 5,
	SM5443_FLAG1_VDSQREGP       = 1 << 4,
	SM5443_FLAG1_VINOVP         = 1 << 3,
	SM5443_FLAG1_IBUSOCP        = 1 << 2,
	SM5443_FLAG1_CHGON          = 1 << 1,
	SM5443_FLAG1_IBUSUCP        = 1 << 0,
};

enum SM5443_flag2_desc {
	SM5443_FLAG2_VBATOVP        = 1 << 7,
	SM5443_FLAG2_VBATREG        = 1 << 5,
	SM5443_FLAG2_TSD            = 1 << 3,
	SM5443_FLAG2_VIN2OUTUVP     = 1 << 2,
	SM5443_FLAG2_VIN2OUTOVP     = 1 << 1,
	SM5443_FLAG2_CNSHTP         = 1 << 0,
};

enum SM5443_flag3_desc {
	SM5443_FLAG3_VINPOK         = 1 << 7,
	SM5443_FLAG3_VOUTPOK        = 1 << 6,
	SM5443_FLAG3_WTDTMR         = 1 << 5,
	SM5443_FLAG3_VBATALM        = 1 << 4,
	SM5443_FLAG3_VINUVLO        = 1 << 3,
	SM5443_FLAG3_CHGONTMR       = 1 << 1,
	SM5443_FLAG3_ADCDONE        = 1 << 0,
};

enum SM5443_flag4_desc {
	SM5443_FLAG4_VOUTOVP        = 1 << 6,
	SM5443_FLAG4_IBUSOCP_RVS    = 1 << 5,
	SM5443_FLAG4_RVSRDY         = 1 << 4,
	SM5443_FLAG4_THEMP          = 1 << 3,
	SM5443_FLAG4_IBUSREG        = 1 << 2,
	SM5443_FLAG4_VOUTUVLO       = 1 << 1,
};

enum SM5443_flag5_desc {
	SM5443_FLAG5_VEXT1OVP       = 1 << 7,
	SM5443_FLAG5_VEXT1UVLO      = 1 << 6,
	SM5443_FLAG5_VEXT1OPEN      = 1 << 5,
	SM5443_FLAG5_VEXT1SHTP      = 1 << 4,
	SM5443_FLAG5_VEXT2OVP       = 1 << 3,
	SM5443_FLAG5_VEXT2UVLO      = 1 << 2,
	SM5443_FLAG5_VEXT2OPEN      = 1 << 1,
	SM5443_FLAG5_VEXT2SHTP      = 1 << 0,
};

enum SM5443_flag6_desc {
	SM5443_FLAG6_IBUSREG_TRACK  = 1 << 7,
	SM5443_FLAG6_VEXT1PD        = 1 << 6,
	SM5443_FLAG6_VEXT2PD        = 1 << 5,
	SM5443_FLAG6_BSTUVLOAP      = 1 << 4,
	SM5443_FLAG6_BSTUVLOBP      = 1 << 3,
};

enum SM5443_flag7_desc {
	SM5443_FLAG7_CFLY1A_SHTP    = 1 << 7,
	SM5443_FLAG7_CFLY2A_SHTP    = 1 << 6,
	SM5443_FLAG7_CFLY1B_SHTP    = 1 << 5,
	SM5443_FLAG7_CFLY2B_SHTP    = 1 << 4,
	SM5443_FLAG7_CFLY1A_OPEN    = 1 << 3,
	SM5443_FLAG7_CFLY2A_OPEN    = 1 << 2,
	SM5443_FLAG7_CFLY1B_OPEN    = 1 << 1,
	SM5443_FLAG7_CFLY2B_OPEN    = 1 << 0,
};

enum SM5443_reg_addr {
	SM5443_REG_DEVICE_ID        = 0x00,
	SM5443_REG_CNTL1            = 0x01,
	SM5443_REG_CNTL2            = 0x02,
	SM5443_REG_CNTL3            = 0x03,
	SM5443_REG_INOUT_PROT1      = 0x04,
	SM5443_REG_CNTL4            = 0x05,
	SM5443_REG_INOUT_PROT2      = 0x06,
	SM5443_REG_IBUS_OCP         = 0x07,
	SM5443_REG_VBAT_OVP         = 0x08,
	SM5443_REG_REG1             = 0x0A,
	SM5443_REG_FLAG1            = 0x0B,
	SM5443_REG_FLAGMSK1         = 0x0C,
	SM5443_REG_FLAG2            = 0x0D,
	SM5443_REG_FLAGMSK2         = 0x0E,
	SM5443_REG_FLAG3            = 0x0F,
	SM5443_REG_FLAGMSK3         = 0x10,
	SM5443_REG_ADC_CNTL1        = 0x11,
	SM5443_REG_VIN_ADC_H        = 0x12,
	SM5443_REG_VIN_ADC_L        = 0x13,
	SM5443_REG_IBUS_ADC_H       = 0x14,
	SM5443_REG_IBUS_ADC_L       = 0x15,
	SM5443_REG_VBAT_ADC_H       = 0x16,
	SM5443_REG_VBAT_ADC_L       = 0x17,
	SM5443_REG_TDIE_ADC         = 0x1A,
	SM5443_REG_ADC_CNTL2        = 0x1B,
	SM5443_REG_PMID_ADC_H       = 0x1C,
	SM5443_REG_PMID_ADC_L       = 0x1D,
	SM5443_REG_VEXT1_ADC_H      = 0x1E,
	SM5443_REG_VEXT1_ADC_L      = 0x1F,
	SM5443_REG_VEXT2_ADC_H      = 0x20,
	SM5443_REG_VEXT2_ADC_L      = 0x21,
	SM5443_REG_VOUT_ADC_H       = 0x22,
	SM5443_REG_VOUT_ADC_L       = 0x23,
	SM5443_REG_EXT1_CNTL        = 0x24,
	SM5443_REG_EXT2_CNTL        = 0x25,
	SM5443_REG_EXTFET_CNTL      = 0x26,
	SM5443_REG_IBUSREG_TRACK    = 0x27,
	SM5443_REG_DG_TIMER_1       = 0x28,
	SM5443_REG_DG_TIMER_2       = 0x29,
	SM5443_REG_DG_TIMER_3       = 0x2A,
	SM5443_REG_MSKDEG_OP        = 0x51,
	SM5443_REG_PH2_DT_QH2       = 0x54,
	SM5443_REG_PH1_DT_QH1       = 0x56,
	SM5443_REG_THEM             = 0x60,
	SM5443_REG_CNTL5            = 0x61,
	SM5443_REG_FLAG4            = 0x63,
	SM5443_REG_FLAGMSK4         = 0x64,
	SM5443_REG_NCT_ADC_H        = 0x65,
	SM5443_REG_NCT_ADC_L        = 0x66,
	SM5443_REG_VBATALM          = 0x67,
	SM5443_REG_FLAG5            = 0x68,
	SM5443_REG_FLAGMSK5         = 0x69,
	SM5443_REG_FLAG6            = 0x6A,
	SM5443_REG_FLAGMSK6         = 0x6B,
	SM5443_REG_FLAG7            = 0x6C,
	SM5443_REG_FLAGMSK7         = 0x6D,
	SM5443_REG_HID_MODE         = 0xF4,
};

enum SM5443_EXT_CNTL {
	SM5443_EXTMODE_WIRED_CHG,
	SM5443_EXTMODE_WIRELESS_CHG,
	SM5443_EXTMODE_WIRED_SHR,
	SM5443_EXTMODE_WIRELESS_SHR,
	SM5443_EXTMODE_UNKNOWN,
};

enum SM5443_vbatovp_offset {
	SM5443_VBATOVP_50   = 0x0,
	SM5443_VBATOVP_100  = 0x1,
	SM5443_VBATOVP_150  = 0x2,
	SM5443_VBATOVP_200  = 0x3,
};

enum SM5443_ibusocp_offset {
	SM5443_IBUSOCP_100  = 0x0,
	SM5443_IBUSOCP_200  = 0x1,
	SM5443_IBUSOCP_300  = 0x2,
	SM5443_IBUSOCP_400  = 0x3,
	SM5443_IBUSOCP_500  = 0x4,
	SM5443_IBUSOCP_600  = 0x5,
	SM5443_IBUSOCP_700  = 0x6,
	SM5443_IBUSOCP_800  = 0x7,
};

enum SM5443_adc_channel {
	SM5443_ADC_THEM     = 0x0,
	SM5443_ADC_TDIE,
	SM5443_ADC_VBUS,
	SM5443_ADC_IBUS,
	SM5443_ADC_VBAT,
	SM5443_ADC_VOUT,
	SM5443_ADC_VEXT1,
	SM5443_ADC_VEXT2,
	SM5443_ADC_PMID,
};

enum SM5443_wdt_tmr {
	WDT_TIMER_S_4       = 0x0,
	WDT_TIMER_S_8       = 0x1,
	WDT_TIMER_S_16      = 0x2,
	WDT_TIMER_S_32      = 0x3,
	WDT_TIMER_S_40      = 0x4,
	WDT_TIMER_S_80      = 0x5,
};

enum sm5443_chip_id {
	SM5443_ALONE = 0x0,
	SM5443_MAIN = 0x1,
	SM5443_SUB = 0x2,
};

struct sm5443_platform_data {
	u8 rev_id;
	int irq_gpio;
	u32 r_ttl;
	u32 freq[3];
	u32 freq_fpdo;
	u32 freq_siop[2];
	u32 topoff;
	u32 wc_topoff;
	u32 en_vbatreg;
	u32 fpdo_topoff;
	u32 fpdo_mainvbat_reg;
	u32 fpdo_subvbat_reg;
	u32 fpdo_vnow_reg;
	u32 init_pps_c_rate;

	struct {
		u32 chg_float_voltage;
		u32 fpdo_chg_curr;
		char *sec_dc_name;
		char *fuelgauge_name;
	} battery;
};

struct sm5443_charger {
	struct device *dev;
	struct i2c_client *i2c;
	struct i2c_client *i2c_hid;
	struct sm5443_platform_data *pdata;
	struct power_supply	*psy_chg;
	struct sm_dc_info *pps_dc;
	struct sm_dc_info *wpc_dc;
	atomic_t shutdown_cnt;
	int ps_type;
	int chip_id;
	u8 call_state;

	struct mutex i2c_lock;
	struct mutex pd_lock;
	struct wakeup_source *chg_ws;

	int irq;
	int cable_online;
	bool vbus_in;
	bool rev_boost;
	bool ibusocp;
	u32 max_vbat;
	u32 target_vbat;
	u32 target_ibus;
	u32 target_ibat;
	u32 ta_type;
	u8 op_mode_ratio;

	bool wdt_disable;

	/* debug */
	struct dentry *debug_root;
	u32 debug_address;
	int addr;
	int size;
	bool valid_ic;
};

#endif  /* __SM5443_CHARGER_H__ */
