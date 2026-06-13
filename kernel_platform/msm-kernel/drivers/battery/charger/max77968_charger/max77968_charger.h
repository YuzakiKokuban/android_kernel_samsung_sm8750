/*
 * Driver for the MAXIM MAX77968 battery charger.
 *
 * Copyright (C) 2023-2024 Analog Devices
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MAX77968_CHARGER_H_
#define _MAX77968_CHARGER_H_

/* Generate the code for the IC issue workaround */
#define ISSUE_WORKAROUND

/* Generate the code for external switch configuration 6 */
#define EXT_SW_CONFIG_6

/* Max77968 driver version */
#define MAX77968_DRV_VER		"0.3.12.3"

/* Vendor ID for ADI */
#define ADI_VENDOR_ID			0x0

struct max77968_platform_data {
	int irq_gpio;				/* GPIO pin that's connected to INT# */
	unsigned int iin_cfg;		/* Input Current Limit - uA unit */
	unsigned int ichg_cfg;		/* Charging Current - uA unit */
	unsigned int vfloat;		/* Float Voltage - uV unit */
	unsigned int iin_topoff;	/* Input Topoff current - uV unit */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	unsigned int fpdo_dc_iin_topoff;		/* FPDO DC Input Topoff current - uA unit */
	unsigned int fpdo_dc_vnow_topoff;		/* FPDO DC Vnow Topoff condition - uV unit */
	unsigned int fpdo_dc_iin_lowest_limit;	/* FPDO DC IIN lowest limit condition - uA unit */
#endif
	unsigned int fsw_cfg_3to1;		/* Switching frequency for 3:1 or 1:3 */
	unsigned int fsw_cfg_2to1;		/* Switching frequency for 2:1 or 1:2 */
	unsigned int fsw_cfg_byp;		/* Switching frequency for bypass mode */
	unsigned int fsw_cfg_fpdo;		/* Switching frequency for fixed pdo */
	unsigned int ntc_ot_th;			/* NTC over temperature alert threshold */
	unsigned int ntc_ot_en;			/* Enable or Disable NTC over temperature alert, 0 - Disable, 1 - Enable */
	unsigned int chg_mode;			/* Default direct charging mode */
	unsigned int cv_polling;		/* CV mode polling time in step1 charging - ms unit */
	unsigned int step1_vth;			/* Step1 vfloat threshold - uV unit */
	unsigned int vbatt_adc_from;	/* read vbatt from "fuel gauge" or "direct charger" */
	char	*fg_name;				/* fuelgauge power supply name */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	char *sec_dc_name;
#endif
};

#define BITS(_end, _start) ((BIT(_end) - BIT(_start)) + BIT(_end))
#define MASK2SHIFT(_mask)	__ffs(_mask)
#define MIN(a, b)	((a < b) ? (a):(b))
#define MAX(a, b)	((a > b) ? (a):(b))

/* Registers & Bit */
#define REVISION_REG                            0x0
#define REVISION_VENDOR_ID                      BITS(2, 0)
#define REVISION_OTP_ID                         BITS(4, 3)
#define REVISION_REVISION_ID                    BITS(7, 5)

#define SW_RST_REG                              0x1
#define SW_RST_SW_RST                           BITS(7, 0)

#define INT_SRC1_REG                            0x2
#define T_ALARM_INT                             BIT(0)
#define T_SHDN_INT                              BIT(1)
#define VBATT_OVP_INT                           BIT(2)
#define RVSBST_UVLO_INT                         BIT(3)
#define VOUT_UVLO_INT                           BIT(4)
#define VOUT_OVP_INT                            BIT(5)
#define VIN_UVLO_INT                            BIT(6)
#define VIN_OVP_INT                             BIT(7)

#define INT_SRC2_REG                            0x3
#define RVSBST_VIN_VALID_INT                    BIT(0)
#define RVSBST_OCP_INT                          BIT(1)
#define RVSBST_SS_FAULT_INT                     BIT(2)
#define RVSBST_SSDONE_INT                       BIT(3)
#define CHGR_RCP_INT                            BIT(4)
#define CHGR_OCP_INT                            BIT(5)
#define CHGR_SS_FAULT_INT                       BIT(6)
#define CHGR_SSDONE_INT                         BIT(7)

#define INT_SRC3_REG                            0x4
#define VIN_SHORT_INT                           BIT(0)
#define IIN_UCP_INT                             BIT(1)
#define IIN_OCP_INT                             BIT(2)
#define REG_TIMEOUT_INT                         BIT(3)
#define IIN_REG_TRACK_INT                       BIT(4)
#define TEMP_REG_INT                            BIT(5)
#define VBATT_REG_INT                           BIT(6)
#define IIN_REG_INT                             BIT(7)

#define INT_SRC4_REG                            0x5
#define EXT2_SW_OPEN_INT                        BIT(0)
#define VEXT2_UVLO_INT                          BIT(1)
#define VEXT2_OVP_INT                           BIT(2)
#define EXT1_SW_OPEN_INT                        BIT(4)
#define VEXT1_UVLO_INT                          BIT(5)
#define VEXT1_OVP_INT                           BIT(6)

#define INT_SRC5_REG                            0x6
#define NTC_OT_INT                              BIT(1)
#define WT_INT                                  BIT(2)
#define ADC_EOC_INT                             BIT(3)
#define BST_UVP_INT                             BIT(4)
#define CFLY_SHORT_INT                          BIT(5)
#define CFLY_OPEN_INT                           BIT(6)
#define PVDD_UVP_INT                            BIT(7)

#define INT_SRC1_M_REG                          0x7

#define INT_SRC2_M_REG                          0x8

#define INT_SRC3_M_REG                          0x9

#define INT_SRC4_M_REG                          0xA

#define INT_SRC5_M_REG                          0xB

#define STATUS1_REG                             0xC
#define T_ALARM_S                               BIT(0)
#define T_SHDN_S                                BIT(1)
#define RVSBST_UVLO_S                           BIT(3)
#define VOUT_UVLO_S                             BIT(4)
#define VOUT_OVP_S                              BIT(5)
#define VIN_UVLO_S                              BIT(6)
#define VIN_OVP_S                               BIT(7)

#define STATUS2_REG                             0xD
#define TEMP_REG_S                              BIT(5)
#define VBATT_REG_S                             BIT(6)
#define IIN_REG_S                               BIT(7)

#define STATUS3_REG                             0xE
#define EXT2_SW_OPEN_S                          BIT(0)
#define VEXT2_UVLO_S                            BIT(1)
#define VEXT2_OVP_S                             BIT(2)
#define EXT1_SW_OPEN_S                          BIT(4)
#define VEXT1_UVLO_S                            BIT(5)
#define VEXT1_OVP_S                             BIT(6)

#define EXT1_SW_CTRL_REG                        0x10
#define EXT1_SW_CTRL1                           BIT(0)
#define EXT1_SW_CTRL2                           BIT(1)
#define EXT1_DISCHG_CTRL1                       BIT(3)
#define EXT1_DISCHG_CTRL2                       BIT(4)
#define VEXT1_DRV_VOLT                          BIT(5)
#define EXT_SW_RVS_TURN_ON_SPEED                BITS(7, 6)

#define EXT2_SW_CTRL_REG                        0x11
#define EXT2_SW_CTRL1                           BIT(0)
#define EXT2_SW_CTRL2                           BIT(1)
#define EXT2_GATE_CTRL                          BIT(2)
#define EXT2_DISCHG_CTRL1                       BIT(3)
#define EXT2_DISCHG_CTRL2                       BIT(4)
#define VEXT2_DRV_VOLT                          BIT(5)
#define EXT_DRV_LPM_EN                          BIT(7)

#define VEXT1_OVP_CFG_REG                       0x12
#define VEXT_OVP_EN                             BIT(0)
#define VEXT_OVP_DEG                            BITS(2, 1)
#define VEXT_OVP_TH                             BITS(5, 3)

#define VEXT2_OVP_CFG_REG                       0x13

#define EXT1_SW_OPEN_DET_CFG_REG                0x14
#define EXT_SW_OPEN_DET_EN                      BIT(0)
#define EXT_SW_OPEN_DET_DEB                     BIT(1)
#define EXT_SW_OPEN_DET_TH                      BITS(3, 2)
#define EXT_SW_OPEN_DET_OFF                     BIT(4)

#define EXT2_SW_OPEN_DET_CFG_REG                0x15

#define SCC_EN_REG                              0x16
#define SCC_OPERATION_MODE                      BITS(2, 0)
#define SCC_STANDBY_MODE_SET                    BIT(4)
#define SCC_VIN_DISCHG_EN                       BIT(5)
#define SCC_RVSBST_VIN_VALID_EN                 BIT(6)
#define SCC_SCC_PH_SEL                          BIT(7)

#define FSW_CFG_REG                             0x17
#define FSW_FREQ                                BITS(3, 0)
#define FSW_SYNC_EN                             BIT(4)
#define FSW_SYNC_ROLE                           BIT(5)
#define FSW_DTHR                                BIT(6)
#define FSW_DTHR_DEPTH                          BIT(7)

#define SKIP_CFG_REG                            0x18
#define SKIP_CFG_AUDIO                          BIT(0)
#define SKIP_CFG_SKIP                           BIT(1)
#define SKIP_CFG_VSKIP                          BITS(4, 2)
#define SKIP_CFG_ISKIP                          BITS(7, 5)

#define SS_CFG_REG                              0x19
#define SS_CFG_SS_T                             BITS(5, 3)
#define SS_CFG_SS_I                             BITS(7, 6)

#define IIN_REGULATION_REG                      0x1A
#define IIN_REG_EN                              BIT(0)
#define IIN_REG_TH                              BITS(7, 1)

#define IIN_REG_TRACK_REG                       0x1B
#define TRACK_IIN_REG_TRACK_EN                  BIT(0)
#define TRACK_IIN_REG_TRACK_STEP                BIT(1)
#define TRACK_IIN_TRACK_CNT_NUM                 BITS(5, 2)
#define TRACK_VBATT_REG_EN                      BIT(7)

#define VBATT_REGULATION_REG                    0x1C


#define TEMP_REG_REG                            0x1D
#define TEMP_REG_TEMP_REG                       BITS(2, 0)

#define RT_CFG_REG                              0x1E
#define RT_CFG_REG_TIMER                        BITS(2, 0)
#define RT_CFG_IIN_DELAY_TIMER                  BITS(5, 3)

#define VIN_OVP_CFG_REG                         0x1F
#define VIN_OVP_EN                              BIT(0)
#define VIN_OVP_DEG                             BIT(1)
#define VIN_OVP_TH                              BITS(3, 2)

#define VOUT_OVP_CFG_REG                        0x20
#define VOUT_OVP_EN                             BIT(0)
#define VOUT_OVP_DEG                            BIT(1)
#define VOUT_OVP_TH                             BIT(2)
#define VBATT_OVP_DEG                           BIT(4)

#define VBATT_OVP_CFG_REG                       0x21
#define VBATT_OVP_EN                            BIT(0)
#define VBATT_OVP_TH                            BITS(7, 1)

#define CHGR_OCP_CFG_REG                        0x22
#define CHGR_OCP_EN                             BIT(0)
#define CHGR_OCP_DEG                            BITS(3, 1)
#define CHGR_OCP                                BITS(7, 4)

#define RVSBST_OCP_CFG_REG                      0x23
#define RVSBST_OCP_EN                           BIT(0)
#define RVSBST_OCP_DEG                          BITS(3, 1)
#define RVSBST_OCP                              BITS(7, 4)

#define CHGR_RCP_CFG_REG                        0x24
#define CHGR_RCP_EN                             BIT(0)
#define CHGR_RCP_DEG                            BITS(3, 1)
#define CHGR_RCP                                BITS(6, 4)

#define IIN_OCP_CFG_REG                         0x25
#define IIN_OCP_EN                              BIT(0)
#define IIN_OCP                                 BITS(6, 1)

#define IIN_OCP_DEG_CFG_REG                     0x26
#define IIN_OCP_DEG                             BITS(1, 0)

#define IIN_UCP_CFG_REG                         0x27
#define IIN_UCP_EN                              BIT(0)
#define IIN_UCP_DEG                             BITS(3, 1)
#define IIN_UCP                                 BITS(6, 4)

#define VIN_SHORT_CFG_REG                       0x28
#define VIN_SHORT_EN                            BIT(0)
#define VIN_SHORT_DEG                           BITS(3, 1)
#define VIN_SHORT_TH                            BITS(5, 4)

#define WT_CFG_REG                              0x29
#define WD_TIMER_EN                             BIT(0)
#define WD_TIMER                                BITS(2, 1)
#define WDTCLR                                  BITS(7, 6)

#define ADC_EN_REG                              0x30
#define ADC_EN                                  BIT(0)
#define ADC_EN_BATTONLY                         BIT(1)

#define ADC_CFG1_REG                            0x31
#define VIN_READ_EN                             BIT(0)
#define PMID_READ_EN                            BIT(1)
#define VEXT1_READ_EN                           BIT(2)
#define VEXT2_READ_EN                           BIT(3)
#define VOUT_READ_EN                            BIT(4)
#define VBATT_READ_EN                           BIT(5)
#define NTC_READ_EN                             BIT(6)
#define TDIE_READ_EN                            BIT(7)

#define ADC_CFG2_REG                            0x32
#define CONV_NUM                                BITS(1, 0)
#define RSVD                                    BITS(3, 2)
#define INTERVAL                                BITS(5, 4)
#define NTC_OT_EN                               BIT(6)
#define IIN_READ_EN                             BIT(7)

#define NTC_OT_TH1_REG                          0x33
#define NTC_OT_TH_HIGH_BIT                      BITS(7, 0)

#define NTC_OT_TH2_REG							0x34
#define NTC_OT_TH_LOW_BIT                       BITS(7, 4)

#define ADC_VIN_READ_REG                       0x35
#define ADC_PMID_READ_REG                      0x37
#define ADC_VEXT1_READ_REG                     0x39
#define ADC_VEXT2_READ_REG                     0x3B
#define ADC_VOUT_READ_REG                      0x3D
#define ADC_VBATT_READ_REG                     0x3F
#define ADC_NTC_READ_REG                       0x41
#define ADC_TDIE_READ_REG                      0x43
#define ADC_IIN_READ_REG                       0x45

#define MAX77968_MAX_REG                       0x4F

#define ADC_VAL_HIGH_BIT                       BITS(7, 0)
#define ADC_VAL_HIGH_MASK                      BITS(11, 4)
#define ADC_VAL_LOW_BIT                        BITS(7, 4)
#define ADC_VAL_LOW_MASK                       BITS(3, 0)

/* IC Revision ID */
enum {
	MAX77968_PASS1 = 0x0,
	MAX77968_PASS2 = 0x0,
	MAX77968_PASS3 = 0x2,
	MAX77968_PASS4 = 0x3,
};

/* For debuggin the cause of the error */
#define ERR_NODE_NONE	                       0x000000
#define ERR_NODE_VIN_UVLO                      0x400001
#define ERR_NODE_VIN_OVP			           0x400002
#define ERR_NODE_VOUT_OVP		               0x400003
#define ERR_NODE_VOUT_UVLO                     0x400004
#define ERR_NODE_VBAT_OVP                      0x400005
#define ERR_NODE_IBUS_OCP                      0x400006
#define ERR_NODE_IBUS_UCP                      0x400007
#define ERR_NODE_CFLY_SHORT                    0x400009
#define ERR_NODE_VIN_SHORT                     0x40001A
#define ERR_NODE_THM_SHUTDOWN                  0x40001C
#define ERR_NODE_WTD_TIMER                     0x40001D
#define ERR_NODE_NTC_PROT                      0x40001F
#define ERR_NODE_DIE_TEMP_WARN                 0x400027
#define ERR_NODE_SOFT_START_TIMEOUT            0x400028
#define ERR_NODE_VEXT1_OVLO                    0x40002B
#define ERR_NODE_VEXT2_OVLO                    0x40002C
#define ERR_NODE_VEXT1_UVLO                    0x40002D
#define ERR_NODE_VEXT2_UVLO                    0x40002E
#define ERR_NODE_CHGR_OCP	                   0x40002F
#define ERR_NODE_CFLY_OPEN_DET                 0x400030
#define ERR_NODE_BST_UVP                       0x400031
#define ERR_NODE_PVDD_UVP                      0x400032
#define ERR_NODE_REGULATION_TIMEOUT            0x400033
#define ERR_NODE_RVSBST_OCP                    0x400034
#define ERR_NODE_CHGR_RCP                      0x400035
#define ERR_NODE_NEW_FEATURE1                  0x400036
#define ERR_NODE_NEW_FEATURE2                  0x400037
#define ERR_NODE_NEW_FEATURE3                  0x400038
#define ERR_NODE_NEW_FEATURE4                  0x400039
#define ERR_NODE_NEW_FEATURE5                  0x40003A
#define ERR_NODE_NEW_FEATURE6                  0x40003B
#define ERR_NODE_NEW_FEATURE7                  0x40003C
#define ERR_NODE_NEW_FEATURE8                  0x40003D
#define ERR_NODE_NEW_FEATURE9                  0x40003E
#define ERR_NODE_NEW_FEATURE10                 0x40003F

/* Interrupt trigger flags */
enum {
	INT_NOT_TRIGGERED,
	INT_TRIGGERED,
};

/* Interrupt EXT switch turn-on speed control for RVS mode */
enum {
	TURN_ON_SPEED_VERY_FAST = 0,
	TURN_ON_SPEED_FAST,
	TURN_ON_SPEED_MEDIUM,
	TURN_ON_SPEED_SLOW,
};

/* VEXT gate drive voltage selection */
enum {
	LOW_DRV_VOLTAGE,
	HIGH_DRV_VOLTAGE,
};

/* EXT2_DRV Gate Select */
enum {
	EXT2_GATE_EXT_SW,
	EXT2_GATE_VB_FET,
};

/* VEXT1/VEXT2 OVP threshold */
enum {
	EXT_OVP_THRES_6P5V = 0,
	EXT_OVP_THRES_13P0V,
	EXT_OVP_THRES_13P5V,
	EXT_OVP_THRES_17P0V,
	EXT_OVP_THRES_17P5V,
	EXT_OVP_THRES_18P0V,
	EXT_OVP_THRES_18P5V,
	EXT_OVP_THRES_19P0V,
};

/* VEXT1/VEXT2 deglich time */
enum {
	EXT_OVP_DEG_TIME_NO = 0,
	EXT_OVP_DEG_TIME_100us,
	EXT_OVP_DEG_TIME_20ms,
	EXT_OVP_DEG_TIME_100ms,
};

/* VEXT1/VEXT2 open detection threshold */
enum {
	OPEN_THRES_0P5V = 0,
	OPEN_THRES_1P0V,
	OPEN_THRES_2P0V,
	OPEN_THRES_3P0V,
};

/* VEXT1/VEXT2 open detection debounce time */
enum {
	EXT_OPEN_DEB_TIME_20ms,
	EXT_OPEN_DEB_TIME_100ms,
};

/* SCC SYNC clock phase shift */
enum {
	PHASE_90,
	PHASE_180,
};

/* SCC standby state control */
enum {
	SHUTDOWN_STATE_SET,
	STANDBY_STATE_SET,
};

/* SCC Operation mode */
enum {
	STANDBY_STATE = 0,
	FWD_3TO1,
	FWD_2TO1,
	FWD_1TO1,
	RVS_1TO1,
	RVS_1TO2,
	RVS_1TO3,
	OPERATION_MODE_MAX,
};

/* SCC frequency dithering depth */
enum {
	DTHR_DEPTH_3,
	DTHR_DEPTH_6,
};

/* SCC SYNC role configuration */
enum {
	SCC_SLAVE,
	SCC_MASTER,
};

/* SCC switching frequency */
enum {
	FSW_200kHz = 0,
	FSW_300kHz,
	FSW_400kHz,
	FSW_500kHz,
	FSW_600kHz,
	FSW_666kHz,
	FSW_750kHz,
	FSW_857kHz,
	FSW_1000kHz,
	FSW_1200kHz,
	FSW_1500kHz,
};

/* Local die temperature regulation threshold */
enum {
	TEMP_REG_OFF = 0,
	TEMP_REG_100degC,
	TEMP_REG_105degC,
	TEMP_REG_110degC,
	TEMP_REG_115degC,
	TEMP_REG_120degC,
	TEMP_REG_125degC,
	TEMP_REG_130degC,
};

/* Invalid input current detection */
enum {
	IIN_DELAY_TIMER_OFF = 0,
	IIN_DELAY_TIMER_200ms,
	IIN_DELAY_TIMER_400ms,
	IIN_DELAY_TIMER_800ms,
	IIN_DELAY_TIMER_1600ms,
	IIN_DELAY_TIMER_3200ms,
	IIN_DELAY_TIMER_6400ms,
	IIN_DELAY_TIMER_12800ms,
};

/* Regulation timer */
enum {
	REG_TIMER_OFF = 0,
	REG_TIMER_200ms,
	REG_TIMER_400ms,
	REG_TIMER_800ms,
	REG_TIMER_1600ms,
	REG_TIMER_3200ms,
	REG_TIMER_6400ms,
	REG_TIMER_12800ms,
};

/* VIN OVP threshold */
/* N is the conversion ratio */
enum {
	VIN_OVP_TH_N_X_5P2V = 0,
	VIN_OVP_TH_N_X_5P3V,
	VIN_OVP_TH_N_X_5P4V,
	VIN_OVP_TH_N_X_5P5V,
};

/* VIN OVP deglich time */
enum {
	VIN_OVP_DEG_TIME_NO,
	VIN_OVP_DEG_TIME_80us,
};

/* VBATT OVP deglich time */
enum {
	VBATT_OVP_DEG_TIME_NO,
	VBATT_OVP_DEG_TIME_100us,
};

/* VOUT OVP threshold */
enum {
	VOUT_OVP_THRES_4P9V,
	VOUT_OVP_THRES_5P1V,
};

/* VOUT OVP deglich time */
enum {
	VOUT_OVP_DEG_TIME_NO,
	VOUT_OVP_DEG_TIME_100us,
};

/* Charging mode OCP deglitch timing */
enum {
	CHGR_OCP_DEG_NO = 0,
	CHGR_OCP_DEG_10us,
	CHGR_OCP_DEG_50us,
	CHGR_OCP_DEG_4ms,
	CHGR_OCP_DEG_10ms,
	CHGR_OCP_DEG_20ms,
	CHGR_OCP_DEG_50ms,
};


/* Reverse boost mode OCP deglich time */
enum {
	RVSBST_OCP_DEG_NO = 0,
	RVSBST_OCP_DEG_10us,
	RVSBST_OCP_DEG_50us,
	RVSBST_OCP_DEG_4ms,
	RVSBST_OCP_DEG_10ms,
	RVSBST_OCP_DEG_20ms,
	RVSBST_OCP_DEG_50ms,
};


/* Charging mode RCP deglitch timing */
enum {
	CHGR_RCP_DEG_NO = 0,
	CHGR_RCP_DEG_10us,
	CHGR_RCP_DEG_50us,
	CHGR_RCP_DEG_4ms,
	CHGR_RCP_DEG_10ms,
	CHGR_RCP_DEG_20ms,
	CHGR_RCP_DEG_50ms,
};


/* Input current OCP deglich time */
enum {
	IIN_OCP_DEG_NO = 0,
	IIN_OCP_DEG_10us,
	IIN_OCP_DEG_100us,
	IIN_OCP_DEG_1ms,
};

/* Input current UCP deglitch timing */
enum {
	IIN_UCP_DEG_NO = 0,
	IIN_UCP_DEG_10us,
	IIN_UCP_DEG_100us,
	IIN_UCP_DEG_4ms,
	IIN_UCP_DEG_5ms,
	IIN_UCP_DEG_20ms,
	IIN_UCP_DEG_50ms,
};

/* VIN short protection threshold */
enum {
	VIN_SHORT_TH_200mV = 0,
	VIN_SHORT_TH_300mV,
	VIN_SHORT_TH_500mV,
	VIN_SHORT_TH_100mV,
};

/* VIN short protection deglitch timing */
enum {
	VIN_SHORT_DEG_NO = 0,
	VIN_SHORT_DEG_10us,
	VIN_SHORT_DEG_50us,
	VIN_SHORT_DEG_4ms,
	VIN_SHORT_DEG_10ms,
	VIN_SHORT_DEG_20ms,
	VIN_SHORT_DEG_50ms,
};

/* ADC Channel */
enum {
	ADC_CH_VIN,		// 0 - ADC_CH_VIN
	ADC_CH_PMID,	// 1 - ADC_CH_PMID
	ADC_CH_VEXT1,	// 2 - ADC_CH_VEXT1
	ADC_CH_VEXT2,	// 3 - ADC_CH_VEXT2
	ADC_CH_VOUT,	// 4 - ADC_CH_VOUT
	ADC_CH_VBATT,	// 5 - ADC_CH_VBATT
	ADC_CH_NTC,		// 6 - ADC_CH_NTC
	ADC_CH_TDIE,	// 7 - ADC_CH_TDIE
	ADC_CH_IIN,		// 8 - ADC_CH_IIN
	ADC_READ_MAX,
};

/* Interrupt and Mask Register Buffer */
enum {
	REG_INT1,
	REG_INT2,
	REG_INT3,
	REG_INT4,
	REG_INT5,
	REG_INT_MAX
};

/* Interrupt and Mask Register Buffer */
enum {
	REG_STATUS1,
	REG_STATUS2,
	REG_STATUS3,
	REG_STATUS_MAX
};

/* Watchdog Timer */
enum {
	WDT_4SEC,
	WDT_8SEC,
	WDT_16SEC,
	WDT_32SEC,
};

/* Skip Mode Current Threshold */
enum {
	ISKIP_100mA = 0,
	ISKIP_200mA,
	ISKIP_300mA,
	ISKIP_400mA,
	ISKIP_500mA,
	ISKIP_600mA,
	ISKIP_700mA,
	ISKIP_800mA,
};

/* Skip Mode Voltage Threshold */
enum {
	VSKIP_10mV = 0,
	VSKIP_20mV,
	VSKIP_40mV,
	VSKIP_60mV,
	VSKIP_80mV,
	VSKIP_100mV,
	VSKIP_120mV,
	VSKIP_140mV,
};

/* ADC step and maximum value */
#define VIN_STEP		4395		// 4mV(4000uV) LSB, Range(0V ~ 15.36V)
#define VIN_MAX			15360000	// 15.36V(15360mV, 15360000uV)
#define PMID_STEP		4395		// 5.25mV(5250uV) LSB, Range(0V ~ 20V)
#define PMID_MAX		20000000	// 20.0V(20000mV, 20000000uV)
#define VEXT1_STEP		4395		// 4mV(4000uV) LSB, Range(0V ~ 15.36V)
#define VEXT1_MAX		15360000	// 15.36V(15360mV, 15360000uV)
#define VEXT2_STEP		4395		// 2mV(2000uV) LSB, Range(0V ~ 5V)
#define VEXT2_MAX		5000000		// 5V(5000mV, 5000000uV)
#define VOUT_STEP		1221		// 2mV(2000uV) LSB, Range(0V ~ 5V)
#define VOUT_MAX		5000000		// 5V(5000mV, 5000000uV)
#define VBATT_STEP		1221		// 2mV(2000uV) LSB, Range(0V ~ 5V)
#define VBATT_MAX		5000000		// 5V(5000mV, 5000000uV)
#define NTC_STEP		464			// 1mV(1000uV) LSB, Range(0V ~ 3.3V)
#define NTC_MAX			1500000		// 1.5V(1500mV, 1500000uV)
#define TDIE_STEP		81			// 0.5C LSB, Range(0 ~ 150C)
#define TDIE_DENOM		1000		// 1000, denominator
#define TDIE_MAX		150			// 150C
#define IIN_STEP		1563		// 2mA(2000uA) LSB, Range(0A ~ 6.5A)
#define IIN_MAX			6500000		// 6.5A(6500mA, 6500000uA)

/* Device standby mode */
#define MAX77968_SHUTDOWN_MODE  0x00    // 00b: in shutdown mode
#define MAX77968_STANDBY_MODE   BIT(4)  // 01b: in standby mode

/* Device current status */
#define MAX77968_STANDBY_STATE	0x00	// 00b: in standby state
#define MAX77968_F31_MODE	    0x01	// 01b: Forward 3:1 Mode
#define MAX77968_F21_MODE	    0x02	// 01b: Forward 2:1 Mode
#define MAX77968_F11_MODE	    0x03	// 01b: Forward 1:1 Mode
#define MAX77968_R11_MODE	    0x04	// 01b: Reverse 1:1 Mode
#define MAX77968_R12_MODE	    0x05	// 01b: Reverse 1:2 Mode
#define MAX77968_R13_MODE	    0x06	// 01b: Reverse 1:3 Mode

#define MAX77968_21SW_F11_MODE	0x80	// 10b: 2:1 switching or forward 1:1 mode
#define MAX77968_12SW_R11_MODE	0xC0	// 11b: 1:2 switching or reverse 1:1 mode

/* Battery overvoltage protection configuration */
#define VBATT_OVP_TH_MIN					4000000
#define VBATT_OVP_TH_MAX					4600000
#define VBATT_OVP_TH_STEP					5000	// Unit: 5mV
#define VBATT_OVP_TH_CFG(_vbatt_ovp_th)		((_vbatt_ovp_th - VBATT_OVP_TH_MIN)/VBATT_OVP_TH_STEP)

/* Charging mode OCP threshold */
#define CHGR_OCP_MIN						110000
#define CHGR_OCP_MAX						335000
#define CHGR_OCP_STEP						15000   // Unit: 15mV
#define CHGR_OCP_CFG(_chgr_ocp)				((_chgr_ocp - CHGR_OCP_MIN)/CHGR_OCP_STEP)

/* Reverse boost mode OCP threshold */
#define RVSBST_OCP_MIN						70000
#define RVSBST_OCP_MAX						295000
#define RVSBST_OCP_STEP						15000   // Unit: 15 mV
#define RVSBST_OCP_CFG(_rvsbst_ocp)			((_rvsbst_ocp - RVSBST_OCP_MIN)/RVSBST_OCP_STEP)

/* Charging mode RCP threshold */
#define CHGR_RCP_MIN						0
#define CHGR_RCP_MAX						70000
#define CHGR_RCP_STEP						10000  // Unit: 10 mV
#define CHGR_RCP_CFG(_chgr_rcp)				((_chgr_rcp - CHGR_RCP_MIN)/CHGR_RCP_STEP)

/* Input current overcurrent protection configuration */
#define IIN_OCP_MIN							500000
#define IIN_OCP_MAX							6000000
#define IIN_OCP_STEP						100000  // Unit: 100 mA
#define IIN_OCP_CFG(_iin_ocp)				((_iin_ocp - IIN_OCP_MIN)/IIN_OCP_STEP)

/* Input current undercurrent protection configuration */
#define IIN_UCP_MIN							100000
#define IIN_UCP_MAX							800000
#define IIN_UCP_STEP						100000  // Unit: 100mA
#define IIN_UCP_CFG(_iin_ucp)	            ((_iin_ucp - IIN_UCP_MIN)/IIN_UCP_STEP)

/* NTC over temperature alert configuration */
#define NTC_OT_MIN							0
#define NTC_OT_MAX							1500000
#define NTC_OT_STEP							464   // Unit: 0.464mV
#define NTC_OT_CFG(_ntc_ot_th)				((_ntc_ot_th - NTC_OT_MIN)/NTC_OT_STEP)

/* Regulation voltage and current */
#define IIN_REG_STEP						50000    // 50mA - VIN regulations current step
#define IIN_REG_MIN							500000   // 500mA
#define IIN_REG_MAX							5500000  // 6000mA

#define VBAT_REG_STEP						5000     // 5mV - VBAT regulations voltage step
#define VBAT_REG_MIN						3800000  // 3800m
#define VBAT_REG_MAX						4550000  // 4550mV

// input current, unit - uA
#define IIN_REG_CFG(_input_current)			((_input_current - IIN_REG_MIN)/IIN_REG_STEP)
// battery voltage, unit - uV
#define VBAT_REG_CFG(_bat_voltage)			((_bat_voltage - VBAT_REG_MIN)/VBAT_REG_STEP)

/* Switching Frequency default value */
#define FSW_CFG_DFT					FSW_857kHz
/* Switching Frequency default value for bypass */
#define FSW_CFG_BYP_DFT				FSW_857kHz
/* Switching Frequency value for 2:1 mode */
#define FSW_CFG_2TO1_DFT			FSW_500kHz
/* Switching Frequency value for 3:1 mode */
#define FSW_CFG_3TO1_DFT			FSW_857kHz
/* Battery minimum voltage Threshold for direct charging */
#define DC_VBAT_MIN                 3100000	// 3100000uV
/* Battery minimum voltage threshold for direct charging error */
#define DC_VBAT_MIN_ERR             3100000	// 3100000uV
/* Charging Float Voltage default value - Battery Voltage Regulation */
#define VBAT_REG_DFT				4450000	// 4450000uV
/* Input Current Limit default value - Input Current Regulation */
#define IIN_REG_DFT					3000000	// 3000000uA
/* NTC over temperature alert threshold voltage default value */
#define NTC_OT_TH_DFT				1110000	// 1.11V(1110000uV)

/* Input Current Limit offset value - Input Current Regulation */
#define IIN_REG_OFFSET1				300000	// 300mA
#define IIN_REG_OFFSET2				350000	// 350mA
#define IIN_REG_OFFSET3				400000	// 400mA
#define IIN_REG_OFFSET4				450000	// 450mA
#define IIN_REG_OFFSET5				500000	// 500mA
#define IIN_REG_OFFSET1_TH	        2000000	// 2000mA
#define IIN_REG_OFFSET2_TH	        3000000	// 3000mA
#define IIN_REG_OFFSET3_TH	        4000000	// 4000mA
#define IIN_REG_OFFSET4_TH	        4500000	// 4500mA
#define IIN_REG_OFFSET_FPDO	        200000	// 200mA - for FPDO

#define IIN_REG_RX_OFFSET1          300000	// 300mA
#define IIN_REG_RX_OFFSET2          350000	// 350mA
#define IIN_REG_RX_OFFSET3          400000	// 400mA
#define IIN_REG_RX_OFFSET4			450000	// 450mA

#define IIN_REG_RX_OFFSET1_TH       1000000	// 1000mA
#define IIN_REG_RX_OFFSET2_TH       2000000	// 2000mA
#define IIN_REG_RX_OFFSET3_TH       3000000	// 3000mA
#define IIN_REG_RX_OFFSET4_TH       4000000	// 4000mA

// Additional Error Code
#define ERROR_DCRCP					99
#define ERROR_WT_EXPIRED			100

// Protection Threshold Settings
#define TEMP_REG_TH_SET				(TEMP_REG_130degC)
#define VIN_OVP_TH_SET			    (VIN_OVP_TH_N_X_5P5V)
#define VOUT_OVP_TH_SET			    (VOUT_OVP_THRES_5P1V)
#define VBATT_OVP_TH_SET		    (4600000)
#define CHGR_OCP_TH_SET			    (335000)

#define RVSBST_OCP_TH_SET		    (190000)
#define CHG_RCP_TH_SET              (CHGR_RCP_MAX)
#define IIN_OCP_TH_SET			    (IIN_OCP_MAX)
#define IIN_UCP_TH_SET			    (IIN_UCP_MIN)
#define VIN_SHORT_TH_SET            (VIN_SHORT_TH_100mV)

// Software Reset Op code
#define SW_RST_OPCODE				0xA5

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
#define MAX77968_SEC_DENOM_U_M			1000    // 1000, denominator
#define MAX77968_SEC_FPDO_DC_IV			9	    // 9V

#if defined(ISSUE_WORKAROUND)
	#define MAX77968_ADC_WR_T				25
	#define MAX77968_BATT_WDT_CONTROL_T		500
#else
	#define MAX77968_BATT_WDT_CONTROL_T		500
#endif

#endif

/* IBAT sense location */
enum {
	IBAT_SENSE_R_BOTTOM_SIDE,
	IBAT_SENSE_R_TOP_SIDE,
};

/* IBAT sense resistor */
enum {
	IBAT_SENSE_R_1mOhm,
	IBAT_SENSE_R_2mOhm,
	IBAT_SENSE_R_5mOhm,
};

/* VIN OV TRACK DELTA */
enum {
	OV_TRACK_DELTA_200mV,
	OV_TRACK_DELTA_400mV,
	OV_TRACK_DELTA_600mV,
	OV_TRACK_DELTA_800mV,
};

/* VIN UV TRACK DELTA */
enum {
	UV_TRACK_DELTA_0mV,
	UV_TRACK_DELTA_200mV,
	UV_TRACK_DELTA_400mV,
	UV_TRACK_DELTA_600mV,
};

/* ADC_AVERAGE_TIMES  */
enum {
	ADC_AVG_2sample = 0,
	ADC_AVG_4sample,
	ADC_AVG_8sample,
	ADC_AVG_16sample,
};

/* Timer definition */
#define VBATMIN_CHECK_T	1000	// Vbat min check timer - 1000ms
#define CCMODE_CHECK_T	2000	// CC mode polling timer - 2000ms
#define CVMODE_CHECK_T	2000	// CV mode polling timer - 2000ms
#define CVMODE_CHECK2_T	1000	// CV mode polling timer2 - 1000ms
#define CVMODE_CHECK3_T	5000	// CV mode polling timer3 for fixed PDO - 5000ms
#define BYPMODE_CHECK_T	10000	// Bypass mode polling timer - 10000ms
#define PDMSG_WAIT_T	200		// PD message waiting time - 200ms
#define ENABLE_DELAY_T	200		// DC Enable waiting time - 200ms
#define PPS_PERIODIC_T	10000	// PPS periodic request message timer - 10000ms
#define UVLO_CHECK_T	1000	// UVLO check timer - 1000ms
#define BYPASS_WAIT_T	200		// Bypass mode waiting time - 200ms
#define INIT_WAKEUP_T	10000	// Initial wakeup timeout - 10000ms
#define DISABLE_DELAY_T	100		// DC Disable waiting time for sw_freq change - 100ms
#define REVERSE_WAIT_T	200		// Reverse mode waiting time - 200ms
#define REVERSE_CHECK_T	5000	// Reverse mode polling timer - 5000ms
#define IIN_CFG_WAIT_T	150		// Input regulation settle time for soft start - 150ms
#define DC_MODE_CHANGES_WAIT_T	100	// Peset waiting time when wireless charging - 100ms
#define RXMSG_WAIT_T	1000	// Rx message waiting time - 1000ms

/* Adaptive temp control */
#define IIN_TEMP_CONTROL_DFT	400000	// 400mA

/* Battery minimum voltage Threshold for direct charging */
#define DC_VBAT_MIN			3100000	// 3100000uV
/* Battery minimum voltage threshold for direct charging error */
#define DC_VBAT_MIN_ERR		3100000	// 3100000uV

/* Charging Done Condition */
#define ICHG_DONE_DFT	1000000	// 1000mA
#define IIN_DONE_DFT	500000	// 500mA

/* Maximum TA voltage threshold */
#define TA_MAX_VOL		5100000 // 5100000uV - 10.2V
/* Minimum TA voltage threshold */
#define TA_MIN_VOL		3500000	// 3500000uV - 3.5V
/* Maximum TA current threshold */
#define TA_MAX_CUR		4500000	// 4500000uA - 4.5V
/* Minimum TA current threshold */
#define TA_MIN_CUR		1000000	// 1000000uA - PPS minimum current - 1A

/* Minimum TA voltage threshold in Preset mode */
#define TA_MIN_VOL_PRESET	3500000	// 3500000uV - 3.5V

/* TA voltage offset for the initial TA voltage */
#define TA_VOL_PRE_OFFSET			300000	// 300000uV - 300mV
/* TA voltage offset for the initial TA voltage in retry */
#define TA_VOL_PRE_OFFSET_RETRY_INC	40000	// 40000uV - 40mV
/* Maximum VIN voltage preset offset before enabling SCC */
#define VIN_VOL_PRE_OFFSET			(TA_VOL_PRE_OFFSET + 1200000)

/* Adjust CC mode TA voltage step */
#define TA_VOL_STEP_ADJ_CC		20000	// 20000uV - 20mV
/* Pre CC mode TA voltage step */
#define TA_VOL_STEP_PRE_CC		100000	// 100000uV - 100mV
/* Pre CV mode TA voltage step */
#define TA_VOL_STEP_PRE_CV		40000	// 40000uV - 40mV
/* IIN_CC adc offset for accuracy */
#define IIN_ADC_OFFSET			20000	// 20000uA - 20mA
/* IIN_CC compensation offset */
#define IIN_CC_COMP_OFFSET		50000	// 50000uA
/* IIN_CC decrease step */
#define IIN_CC_DEC_STEP			50000   // 50000uA - 50mA
/* IIN_CC compensation offset */
#define IIN_CC_COMP_DOWN_OFFSET	100000	// 100000uA
#define IIN_CC_COMP_UP_OFFSET	60000	// 50000uA
/* Battery Threshold to set TA_VOL_STEP to 20mV in ADJUST_CC */
#define BAT_VOL_TH_TO_ADJ_CC_MIN_STEP	4420000	// 4420000uV - 4.42V

#if IS_ENABLED(CONFIG_SEC_FACTORY)
#define FACTORY_TA_VOL_STEP_ADJ_CC_MAX			(400000)
#define FACTORY_ADJ_CC_PDMSG_WAIT_T				(200)
#endif
/* IIN_CC compensation offset in Power Limit Mode(Constant Power) TA */
#define IIN_CC_COMP_OFFSET_CP	70000	// 20000uA - 70mA

/* TA maximum voltage that can support constant current in Constant Power Mode */
#define TA_MAX_VOL_CP		10000000	// 9760000uV --> 9800000uV --> 10000000uV - 10V
/* maximum retry counter for restarting charging */
#define MAX_RETRY_CNT		3		// 3times
/* TA IIN tolerance */
#define TA_IIN_OFFSET		100000	// 100mA

/* TA current low offset for reducing input current */
#define TA_CUR_LOW_OFFSET			200000	// 200mA
/* TA voltage offset for 1:1 bypass mode */
#define TA_VOL_OFFSET_1TO1_BYPASS	100000	// 100mV
/* TA voltalge offset for 2:1 bypass mode */
#define TA_VOL_OFFSET_2TO1_BYPASS	200000	// 200mV
/* TA voltalge offset for 3:1 bypass mode */
#define TA_VOL_OFFSET_3TO1_BYPASS	300000	// 300mV
/* Input low current threshold to change switching frequency */
#define IIN_LOW_TH_SW_FREQ			1100000	// 1100000uA - 1.1A
/* TA voltage offset value for frequency change */
#define TA_VOL_OFFSET_SW_FREQ		600000	// 600mV

/* PD Message Voltage and Current Step */
#define PD_MSG_TA_VOL_STEP			20000	// 20mV
#define PD_MSG_TA_CUR_STEP			50000	// 50mA

/* denominator for change between micro and mili unit */
#define DENOM_U_M		1000

/* Maximum WCRX voltage threshold */
#define WCRX_MAX_VOL	5300000     // 5300000uV

/* WCRX voltage Step */
#define WCRX_VOL_STEP				20000	// 20.0mV
#define WCRX_VOL_DOWN_STEP			40000	// 40.0mV
#define WCRX_ADJUST_CC_RX_VOL_STEP	80000	// 80.0mV
#define WCRX_ADJUST_RX_VOL_STEP		40000	// 40.0mV

/* Switching charger minimum current */
#define SWCHG_ICL_MIN				100000	// 100mA
#define SWCHG_ICL_NORMAL			3000000 // 3000mA

/* Step1 vfloat threshold */
#define STEP1_VFLOAT_THRESHOLD		4200000	// 4200000uV - 4.2V

/* Top-off minimum threshold */
#define TOP_OFF_MINIMUM_THRESHOLD	4400000	// 4400000uV - 4.4V

/*CHGR_RCP Error */
#define ERROR_DCRCP		99	/* RCP Error - 99 */
/* IIN_UCP Error */
#define ERROR_DCUCP		100	/* UCP Error - 100 */

/* FPDO Charging Done counter */
#define FPDO_DONE_CNT	3

/* VBATT regulation enable/disable threshold */
#define VBATT_REG_ENA_TH	4460000	// 4460000uV - 4.46V

/* ISKIP threshold for 1:3/1:2/1:1 */
#define ISKIP_1TO3_TH		ISKIP_800mA
#define ISKIP_1TO2_TH		ISKIP_600mA
#define ISKIP_1TO1_TH		ISKIP_600mA

/* The TA voltage decreasing step in preset section */
#define TA_PRESET_DEC_STEP		60000	// 60mV

/* The PMID volt high threshold before enabling SCC */
#define PMID_VOL_HIGH_TH		5000000	// 5V

enum {
	VBATT_FROM_FG,
	VBATT_FROM_DC,
};

enum {
	WDT_DISABLE,
	WDT_ENABLE,
};

/* ADC operation mode */
enum {
	AUTO_MODE = 0,
	FORCE_SHUTDOWN_MODE,
	FORCE_HIBERNATE_MODE,
	FORCE_NORMAL_MODE,
};

/* Interrupt and Status Register Buffer */
enum {
	REG_DEVICE_0,
	REG_DEVICE_1,
	REG_DEVICE_2,
	REG_DEVICE_3,
	REG_CHARGING,
	REG_SC_0,
	REG_SC_1,
	REG_BUFFER_MAX
};

/* Direct Charging State */
enum {
	DC_STATE_NO_CHARGING,	/* No charging */
	DC_STATE_CHECK_VBAT,	/* Check min battery level */
	DC_STATE_PRESET_DC,		/* Preset TA voltage/current for the direct charging */
	DC_STATE_CHECK_ACTIVE,	/* Check active status before entering Adjust CC mode */
	DC_STATE_ADJUST_CC,		/* Adjust CC mode */
	DC_STATE_START_CC,		/* Start CC mode */
	DC_STATE_CC_MODE,		/* Check CC mode status */
	DC_STATE_START_CV,		/* Start CV mode */
	DC_STATE_CV_MODE,		/* Check CV mode status */
	DC_STATE_CHARGING_DONE,	/* Charging Done */
	DC_STATE_ADJUST_TAVOL,	/* Adjust TA voltage to set new TA current under 1000mA input */
	DC_STATE_ADJUST_TACUR,	/* Adjust TA current to set new TA current over 1000mA input */
	DC_STATE_BYPASS_MODE,	/* Check Bypass mode status */
	DC_STATE_DCMODE_CHANGE,	/* DC mode change from Normal to 1:1 or 2:1 bypass */
	DC_STATE_REVERSE_MODE,	/* Reverse 1:2 switching or reverse 1:1 bypass */
	DC_STATE_FPDO_CV_MODE,	/* Check FPDO CV mode status */
	DC_STATE_MAX,
};

/* DC Mode Status */
enum {
	DCMODE_VFLT_LOOP,
	DCMODE_IIN_LOOP,
	DCMODE_LOOP_INACTIVE,
	DCMODE_CHG_DONE,
};

/* Timer ID */
enum {
	TIMER_ID_NONE,
	TIMER_VBATMIN_CHECK,
	TIMER_PRESET_DC,
	TIMER_PRESET_CONFIG,
	TIMER_CHECK_ACTIVE,
	TIMER_ADJUST_CCMODE,
	TIMER_ENTER_CCMODE,
	TIMER_CHECK_CCMODE,
	TIMER_ENTER_CVMODE,
	TIMER_CHECK_CVMODE,
	TIMER_PDMSG_SEND,
	TIMER_ADJUST_TAVOL,
	TIMER_ADJUST_TACUR,
	TIMER_CHECK_BYPASSMODE,
	TIMER_DCMODE_CHANGE,
	TIMER_START_REVERSE,
	TIMER_CHECK_REVERSE_ACTIVE,
	TIMER_CHECK_REVERSE_MODE,
	TIMER_CHECK_FPDOCVMODE,
};

/* PD Message Type */
enum {
	PD_MSG_REQUEST_APDO,
	PD_MSG_REQUEST_FIXED_PDO,
	WCRX_REQUEST_VOLTAGE,
};

/* TA increment Type */
enum {
	INC_NONE,	/* No increment */
	INC_TA_VOL, /* TA voltage increment */
	INC_TA_CUR, /* TA current increment */
};

/* TA Type for the direct charging */
enum {
	TA_TYPE_UNKNOWN,
	TA_TYPE_USBPD,
	TA_TYPE_WIRELESS,
	TA_TYPE_USBPD_20,	/* USBPD 2.0 - fixed PDO */
};

/* TA Control method for the direct charging */
enum {
	TA_CTRL_CL_MODE,
	TA_CTRL_CV_MODE,
};

/* Direct Charging Mode for the direct charging */
enum {
	CHG_NO_DC_MODE = 0,
	CHG_2TO1_DC_MODE = 2,
	CHG_3TO1_DC_MODE = 3,
};

#if defined(ISSUE_WORKAROUND)
#define ADC_BUF_SIZE	7

enum {
//	ADC_BUF_VIN = 0,
//	ADC_BUF_PMID,
//	ADC_BUF_VOUT,
//	ADC_BUF_TDIE,
	ADC_BUF_IIN = 0,
	NUM_ADC_BUF,
};

struct circular_buf {
	unsigned int buf[ADC_BUF_SIZE];
	int head;
	int count;
	struct mutex lock;
};

#endif

/**
 * struct max77968_charger - max77968_charger charger instance
 * @monitor_wake_lock: lock to enter the suspend mode
 * @lock: protects concurrent access to online variables
 * @i2c_lock: protects concurrent access to i2c bus
 * @dev: pointer to device
 * @regmap: pointer to driver regmap
 * @mains: power_supply instance for AC/DC power
 * @dc_wq: work queue for the algorithm and monitor timer
 * @timer_work: timer work for charging
 * @timer_id: timer id for timer_work
 * @timer_period: timer period for timer_work
 * @pps_work: pps work for PPS periodic timer work
 * @mains_online: is AC/DC input connected
 * @charging_state: direct charging state
 * @ret_state: return direct charging state after DC_STATE_ADJUST_TAVOL is done
 * @iin_cc: input current for the direct charging in cc mode, uA
 * @iin_cfg: input current limit, uA
 * @vfloat: floating voltage, uV
 * @max_vfloat: maximum float voltage, uV
 * @ichg_cfg: charging current limit, uA
 * @iin_topoff: input topoff current, uA
 * @fpdo_dc_iin_topoff: input topoff current for FPDO, uA
 * @fpdo_dc_vnow_topoff: topoff battery voltage for FPDO, uV
 * @byp_mode: bypass mode, none, 1:1, 2:1 or 3:1 pass through mode
 * @ta_cur: AC/DC(TA) current, uA
 * @ta_vol: AC/DC(TA) voltage, uV
 * @ta_objpos: AC/DC(TA) PDO object position
 * @ta_target_vol: TA target voltage before any compensation
 * @ta_max_cur: TA maximum current of APDO, uA
 * @ta_max_vol: TA maximum voltage for the direct charging, uV
 * @ta_max_pwr: TA maximum power, uW
 * @prev_iin: Previous IIN ADC, uA
 * @prev_inc: Previous TA voltage or current increment factor
 * @req_new_iin: Request for new input current limit, true or false
 * @req_new_vfloat: Request for new vfloat, true or false
 * @req_new_byp_mode: Request for new bypass mode, true or false
 * @req_new_chg_mode: Request for new charging mode, true or false
 * @new_iin: New request input current limit, uA
 * @new_vfloat: New request vfloat, uV
 * @new_byp_mode: New request bypass mode, none, 1:1, 2:1 or 3:1 pass through mode
 * @new_chg_mode: New request charging mode, 1:1, 2:1, 3:1 pps charging mode
 * @new_fsw_cfg: The new switching frequency setting for new charging mode
 * @retry_cnt: retry counter for re-starting charging if charging stop happens
 * @retry_ta_vol: new TA voltage for retry
 * @ta_type: TA type for the direct charging, USBPD TA or Wireless Charger.
 * @ta_ctrl: TA control method for the direct charging, Current Limit mode or Constant Voltage mode.
 * @chg_mode: charging mode, 1:1, 2:1, 3:1 pps charging mode
 * @fsw_cfg: Switching frequency setting
 * @rev_mode: reverse mode, reverse stop, 1:1, 2:1 or 3:1 mode
 * @iin_rev: vin_ocp current for reverse mode
 * @prev_vbat: Previous VBAT_ADC in start cv and cv state, uV
 * @done_cnt: Charging done counter.
 * @pdata: pointer to platform data
 * @debug_root: debug entry
 * @debug_address: debug register address
 */
struct max77968_charger {
	struct wakeup_source	*monitor_wake_lock;
	struct mutex			lock;
	struct mutex			i2c_lock;
	struct device			*dev;
	struct regmap			*regmap;
	struct power_supply		*mains;
	struct workqueue_struct *dc_wq;
	struct delayed_work		timer_work;
	unsigned int			timer_id;
	unsigned long			timer_period;

	struct delayed_work	pps_work;

	bool				mains_online;
	unsigned int		charging_state;
	unsigned int		ret_state;

	unsigned int		iin_cc;
	unsigned int		iin_cfg;
	unsigned int		vfloat;
	unsigned int		max_vfloat;
	unsigned int		ichg_cfg;
	unsigned int		iin_topoff;
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	unsigned int		fpdo_dc_iin_topoff;
	unsigned int		fpdo_dc_vnow_topoff;
#endif
	unsigned int		byp_mode;

	unsigned int		ta_cur;
	unsigned int		ta_vol;
	unsigned int		ta_objpos;

	unsigned int		ta_target_vol;

	unsigned int		ta_max_cur;
	unsigned int		ta_max_vol;
	unsigned int		ta_max_pwr;

	unsigned int		prev_iin;
	unsigned int		prev_inc;

	bool				req_new_iin;
	bool				new_iin_buf_has_data;
	bool				req_new_vfloat;
	bool				req_new_byp_mode;
	bool				req_new_chg_mode;
	bool				new_chg_mode_buf_has_data;
	unsigned int		new_iin;
	unsigned int		new_iin_busy_buf;
	unsigned int		new_vfloat;
	unsigned int		new_byp_mode;
	unsigned int		new_chg_mode;
	unsigned int		new_chg_mode_busy_buf;

	int					ss_fault_retry_cnt;
	bool				ss_fault_inc_ta_volt;

	int					preset_ta_fault_retry_cnt;
	bool				preset_ta_fault_inc_ta_volt;
	bool				preset_ta_vol_dec_once;

	int					ta_type;
	int					ta_ctrl;
	int					chg_mode;
	unsigned int		fsw_cfg;

	bool				dec_vfloat;
	bool				req_enable;
	bool				enable;

	int					rev_mode;
	int					iin_rev;

	int					prev_vbat;
	int					done_cnt;

	bool				valid_ic;
	bool				force_vbat_reg_off;
	atomic_t			shutdown;
	atomic_t			suspend;

	struct max77968_platform_data *pdata;

	/* debug */
	struct dentry		*debug_root;
	u32					debug_address;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	int input_current;
	int float_voltage;
	int chg_status;
	int health_status;
	bool wdt_kick_disable;

	int adc_val[ADC_READ_MAX];

	unsigned int pdo_index;
	unsigned int pdo_max_voltage;
	unsigned int pdo_max_current;

	struct delayed_work wdt_control_work;
#endif

#if defined(ISSUE_WORKAROUND)
	struct i2c_client	*otpid;
	struct regmap		*otpregmap;
	struct i2c_client	*tsid;
	struct regmap		*tregmap;
	struct mutex		tregmap_lock;
	bool				pass3_wr_en;
	bool				adc_wr_en;
	struct circular_buf	circ_buf[NUM_ADC_BUF];
#endif

	u32 error_cause;
};

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
extern int sec_pd_select_pps(int num, int ppsVol, int ppsCur);
extern int sec_pd_get_apdo_max_current(unsigned int *pdo_pos, unsigned int taMaxVol, unsigned int *taMaxCur);
#endif

#endif
