/*
 * cps4038_charger.h
 * Samsung CPS4038 Charger Header
 *
 * Copyright (C) 2022 Samsung Electronics, Inc.
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

#ifndef __WIRELESS_CHARGER_CPS4038_H
#define __WIRELESS_CHARGER_CPS4038_H __FILE__

#include <linux/mfd/core.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <linux/alarmtimer.h>
#include <linux/pm_wakeup.h>
#include <linux/battery/sb_wireless.h>
#include "../common/sec_charging_common.h"
#include "cps4038_fod.h"
#include "cps4038_cmfet.h"

/* REGISTER MAPS */
#define MFC_CHIP_ID_L_REG					0x00
#define MFC_CHIP_ID_H_REG					0x01
#define MFC_CHIP_REVISION_REG				0x02
#define MFC_CUSTOMER_ID_REG					0x03
#define MFC_FW_MAJOR_REV_L_REG				0x04
#define MFC_FW_MAJOR_REV_H_REG				0x05
#define MFC_FW_MINOR_REV_L_REG				0x06
#define MFC_FW_MINOR_REV_H_REG				0x07
#define MFC_PRMC_ID_L_REG					0x0A
#define MFC_PRMC_ID_H_REG					0x0B
/* RXID BIT[0:47] */
#define MFC_WPC_RXID_0_REG					0x10
#define MFC_WPC_RXID_1_REG					0x11
#define MFC_WPC_RXID_2_REG					0x12
#define MFC_WPC_RXID_3_REGs					0x13
#define MFC_WPC_RXID_4_REG					0x14
#define MFC_WPC_RXID_5_REG					0x15

#define MFC_STATUS_L_REG					0x20
#define MFC_STATUS_H_REG					0x21
#define MFC_STATUS1_L_REG					0x22
#define MFC_STATUS1_H_REG					0x23
#define MFC_INT_A_L_REG						0x24
#define MFC_INT_A_H_REG						0x25
#define MFC_INT_B_L_REG					    0x26
#define MFC_INT_B_H_REG					    0x27
#define MFC_INT_A_ENABLE_L_REG				0x28
#define MFC_INT_A_ENABLE_H_REG				0x29
#define MFC_INT_B_ENABLE_L_REG				0x2A
#define MFC_INT_B_ENABLE_H_REG				0x2B
#define MFC_INT_A_CLEAR_L_REG				0x2C
#define MFC_INT_A_CLEAR_H_REG				0x2D
#define MFC_INT_B_CLEAR_L_REG				0x2E
#define MFC_INT_B_CLEAR_H_REG				0x2F

//#define MFC_CTRL_STS_REG					0x28

#define MFC_SYS_OP_MODE_REG					0x30
#define MFC_BATTERY_CHG_STATUS_REG			0x3A
#define MFC_EPT_REG			0x3B /* EPT(End of Power Transfer) cases. PMA has only EOC case */
#define MFC_ADC_VOUT_L_REG					0x3C
#define MFC_ADC_VOUT_H_REG					0x3D

#define MFC_VOUT_SET_L_REG					0x3E
#define MFC_VOUT_SET_H_REG					0x3F
#define MFC_VRECT_ADJ_REG					0x39
#define MFC_ADC_VRECT_L_REG					0x40
#define MFC_ADC_VRECT_H_REG					0x41
#define MFC_ADC_IRECT_L_REG					0x42
#define MFC_ADC_IRECT_H_REG					0x43
#define MFC_TX_IUNO_LIMIT_L_REG				0x34
#define MFC_TX_IUNO_LIMIT_H_REG				0x35
#define MFC_ADC_IOUT_L_REG					0x44
#define MFC_ADC_IOUT_H_REG					0x45
#define MFC_ADC_DIE_TEMP_L_REG				0x46 /* 8 LSB field is used, Celsius */
#define MFC_ADC_DIE_TEMP_H_REG				0x47 /* only 4 MSB[3:0] field is used, Celsius */
#define MFC_TRX_OP_FREQ_L_REG				0x48 /* kHZ */
#define MFC_TRX_OP_FREQ_H_REG				0x49 /* kHZ */
#define MFC_RX_PING_FREQ_L_REG				0x4A /* kHZ */
#define MFC_RX_PING_FREQ_H_REG				0x4B /* kHZ */
#define MFC_ILIM_SET_REG					0x4C /* ILim =  value * 0.05(A) */
#define MFC_ILIM_ADJ_REG					0x4D /* AdjVal = ILIM_ADJ * 50 */

#define MFC_AP2MFC_CMD_L_REG				0x4E
#define MFC_AP2MFC_CMD_H_REG				0x4F

/********************************************************************************/
/* Below register are functionally depends on the operation mode(TX or RX mode) */
/* RX mode */
#define MFC_WPC_PCKT_HEADER_REG				0x50
#define MFC_WPC_RX_DATA_COM_REG				0x51 /* WPC Rx to Tx COMMAND */
#define MFC_WPC_RX_DATA_VALUE0_REG			0x52
#define MFC_WPC_RX_DATA_VALUE1_REG			0x53
#define MFC_WPC_RX_DATA_VALUE2_REG			0x54
#define MFC_WPC_RX_DATA_VALUE3_REG			0x55
#define MFC_WPC_RX_DATA_VALUE4_REG			0x56
#define MFC_WPC_RX_DATA_VALUE5_REG			0x57
#define MFC_WPC_RX_DATA_VALUE6_REG			0x58
#define MFC_WPC_RX_DATA_VALUE7_REG			0x59

/* TX mode */
#define MFC_WPC_TX_DATA_COM_REG				0x50 /* WPC Tx to Rx COMMAND */
#define MFC_WPC_TX_DATA_VALUE0_REG			0x51
#define MFC_WPC_TX_DATA_VALUE1_REG			0x52
#define MFC_WPC_TX_DATA_VALUE2_REG			0x53
#define MFC_WPC_TX_DATA_VALUE3_REG			0x54
#define MFC_WPC_TX_DATA_VALUE4_REG			0x55
#define MFC_WPC_TX_DATA_VALUE5_REG			0x56
#define MFC_WPC_TX_DATA_VALUE6_REG			0x57
#define MFC_WPC_TX_DATA_VALUE7_REG			0x58
#define MFC_WPC_TX_DATA_VALUE8_REG			0x59

/* Common */
#define MFC_WPC_TRX_DATA2_COM_REG			0x5A
#define MFC_WPC_TRX_DATA2_VALUE0_REG		0x5B
#define MFC_WPC_TRX_DATA2_VALUE1_REG		0x5C
#define MFC_WPC_TRX_DATA2_VALUE2_REG		0x5D
#define MFC_WPC_TRX_DATA2_VALUE3_REG		0x5E
#define MFC_WPC_TRX_DATA2_VALUE4_REG		0x5F
#define MFC_WPC_TRX_DATA2_VALUE5_REG		0x60
#define MFC_WPC_TRX_DATA2_VALUE6_REG		0x61
#define MFC_WPC_TRX_DATA2_VALUE7_REG		0x62
#define MFC_WPC_TRX_DATA2_VALUE8_REG		0x63
/********************************************************************************/

#define MFC_ADT_TIMEOUT_PKT_REG				0x16B
#define MFC_ADT_TIMEOUT_STR_REG				0x16C

#define MFC_TX_IUNO_HYS_REG					0x36
#define MFC_TX_IUNO_OFFSET_L_REG			0x37
#define MFC_TX_IUNO_OFFSET_H_REG			0x38

#define MFC_TX_OC_FOD1_LIMIT_L_REG			0x94
#define MFC_TX_OC_FOD1_LIMIT_H_REG			0x95
#define MFC_TX_OC_FOD2_LIMIT_L_REG			0x96
#define MFC_TX_OC_FOD2_LIMIT_H_REG			0x97

#define MFC_STARTUP_EPT_COUNTER				0x6D

#define MFC_TX_DUTY_CYCLE					0xE6

/* TX Digital Ping duty, default is 45% (0x2D=45) */
#define MFC_TX_PING_DUTY_CYCLE				0x16D /* default 0x2D */
/* TX Max Operating Frequency, default is 145kHz (0x5AA/10=145KHz) */
#define MFC_TX_MAX_OP_FREQ_L_REG			0x16E /* default 0xAA */
#define MFC_TX_MAX_OP_FREQ_H_REG			0x16F /* default 0x05 */
/* TX Min Operating Frequency, default is 111kHz (0x456/10=111KHz) */
#define MFC_TX_MIN_OP_FREQ_L_REG			0x170 /* default 0x56 */
#define MFC_TX_MIN_OP_FREQ_H_REG			0x171 /* default 0x4 */
/* TX Digital Ping Frequency, default is 145kHz (0x5AA/10=145KHz) */
#define MFC_TX_PING_FREQ_L_REG				0x64 /* default 0xAA */
#define MFC_TX_PING_FREQ_H_REG				0x65 /* default 0x05 */
/* TX Mode Minimum Duty Setting Register, Min_Duty, default is 20% (0x14=20) */
#define MFC_TX_MIN_DUTY_SETTING_REG			0x66 /* default 0x14 */
#define TX_MIN_OP_FREQ_DEFAULT				1110 /* default 111kHz */

#define MFC_INVERTER_CTRL_REG				0x67
#define MFC_CMFET_CTRL_REG					0x68

/* RX Mode Communication Modulation FET Ctrl */
#define MFC_MST_MODE_SEL_REG				0x69
#define MFC_RX_OV_CLAMP_REG					0x6A
//#define MFC_RX_COMM_MOD_AFC_FET_REG		0x37
#define MFC_RX_COMM_MOD_FET_REG				0x6B

#define MFC_RECTMODE_REG					0x6C
#define MFC_START_EPT_COUNTER_REG			0x6D
#define MFC_CTRL_MODE_REG					0x6E
#define MFC_RC_PHM_PING_PERIOD_REG			0x6F

#define MFC_IEC_QFOD_ENABLE_REG				0x114
#define MFC_IEC_QVALUE_REG					0x115
#define MFC_IEC_FRES_L_REG					0x116
#define MFC_IEC_FRES_R_REG					0x117
#define MFC_IEC_Q_THRESH1_REG				0x118
#define MFC_IEC_Q_THRESH2_REG				0x119
#define MFC_IEC_FRES_THRESH1_REG			0x11A
#define MFC_IEC_FRES_THRESH2_REG			0x11B
#define MFC_IEC_POWER_LIMIT_THRESH_L_REG	0x11C
#define MFC_IEC_POWER_LIMIT_THRESH_H_REG	0x11D
#define MFC_IEC_PLOSS_THRESH1_L_REG			0x11E
#define MFC_IEC_PLOSS_THRESH1_H_REG			0x11F
#define MFC_IEC_PLOSS_THRESH2_L_REG			0x120
#define MFC_IEC_PLOSS_THRESH2_H_REG			0x121
#define MFC_IEC_PLOSS_FREQ_THRESH1_REG		0x122
#define MFC_IEC_PLOSS_FREQ_THRESH2_REG		0x123
#define MFC_IEC_TA_POWER_LIMIT_THRESH_L_REG	0x124
#define MFC_IEC_TA_POWER_LIMIT_THRESH_H_REG	0x125
#define MFC_IEC_TA_PLOSS_THRESH1_L_REG		0x126
#define MFC_IEC_TA_PLOSS_THRESH1_H_REG		0x127
#define MFC_IEC_TA_PLOSS_THRESH2_L_REG		0x128
#define MFC_IEC_TA_PLOSS_THRESH2_H_REG		0x129
#define MFC_IEC_TA_PLOSS_FREQ_THRESH1_REG	0x12A
#define MFC_IEC_TA_PLOSS_FREQ_THRESH2_REG	0x12B
#define MFC_IEC_PLOSS_FOD_ENABLE_REG		0x12C

#define MIN_DUTY_SETTING_20_DATA	20
#define MIN_DUTY_SETTING_30_DATA	30
#define MIN_DUTY_SETTING_50_DATA	50

#define MFC_WPC_FOD_0A_REG					0x70
#define MFC_WPC_FOD_0B_REG					0x71
#define MFC_WPC_FOD_1A_REG					0x72
#define MFC_WPC_FOD_1B_REG					0x73
#define MFC_WPC_FOD_2A_REG					0x74
#define MFC_WPC_FOD_2B_REG					0x75
#define MFC_WPC_FOD_3A_REG					0x76
#define MFC_WPC_FOD_3B_REG					0x77
#define MFC_WPC_FOD_4A_REG					0x78
#define MFC_WPC_FOD_4B_REG					0x79
#define MFC_WPC_FOD_5A_REG					0x7A
#define MFC_WPC_FOD_5B_REG					0x7B
#define MFC_WPC_FOD_6A_REG					0x7C
#define MFC_WPC_FOD_6B_REG					0x7D
#define MFC_WPC_FOD_7A_REG					0x7E
#define MFC_WPC_FOD_7B_REG					0x7F
#define MFC_WPC_FOD_8A_REG					0x80
#define MFC_WPC_FOD_8B_REG					0x81
#define MFC_WPC_FOD_9A_REG					0x82
#define MFC_WPC_FOD_9B_REG					0x83

#define MFC_WPC_PARA_MODE_REG				0x8C

#define MFC_WPC_FWC_FOD_0A_REG				0x100
#define MFC_WPC_FWC_FOD_0B_REG				0x101
#define MFC_WPC_FWC_FOD_1A_REG				0x102
#define MFC_WPC_FWC_FOD_1B_REG				0x103
#define MFC_WPC_FWC_FOD_2A_REG				0x104
#define MFC_WPC_FWC_FOD_2B_REG				0x105
#define MFC_WPC_FWC_FOD_3A_REG				0x106
#define MFC_WPC_FWC_FOD_3B_REG				0x107
#define MFC_WPC_FWC_FOD_4A_REG				0x108
#define MFC_WPC_FWC_FOD_4B_REG				0x109
#define MFC_WPC_FWC_FOD_5A_REG				0x10A
#define MFC_WPC_FWC_FOD_5B_REG				0x10B
#define MFC_WPC_FWC_FOD_6A_REG				0x10C
#define MFC_WPC_FWC_FOD_6B_REG				0x10D
#define MFC_WPC_FWC_FOD_7A_REG				0x10E
#define MFC_WPC_FWC_FOD_7B_REG				0x10F
#define MFC_WPC_FWC_FOD_8A_REG				0x100
#define MFC_WPC_FWC_FOD_8B_REG				0x101
#define MFC_WPC_FWC_FOD_9A_REG				0x102
#define MFC_WPC_FWC_FOD_9B_REG				0x103

#define MFC_WPC_EPP_FOD_0A_REG	            0x1A2
#define MFC_WPC_EPP_FOD_0B_REG	            0x1A3
#define MFC_WPC_EPP_FOD_1A_REG	            0x1A4
#define MFC_WPC_EPP_FOD_1B_REG	            0x1A5
#define MFC_WPC_EPP_FOD_2A_REG	            0x1A6
#define MFC_WPC_EPP_FOD_2B_REG	            0x1A7
#define MFC_WPC_EPP_FOD_3A_REG	            0x1A8
#define MFC_WPC_EPP_FOD_3B_REG	            0x1A9
#define MFC_WPC_EPP_FOD_4A_REG	            0x1AA
#define MFC_WPC_EPP_FOD_4B_REG	            0x1AB
#define MFC_WPC_EPP_FOD_5A_REG	            0x1AC
#define MFC_WPC_EPP_FOD_5B_REG	            0x1AD
#define MFC_WPC_EPP_FOD_6A_REG	            0x1AE
#define MFC_WPC_EPP_FOD_6B_REG	            0x1AF
#define MFC_WPC_EPP_FOD_7A_REG	            0x1B0
#define MFC_WPC_EPP_FOD_7B_REG	            0x1B1
#define MFC_WPC_EPP_FOD_8A_REG	            0x1B2
#define MFC_WPC_EPP_FOD_8B_REG	            0x1B3
#define MFC_WPC_EPP_FOD_9A_REG	            0x1B4
#define MFC_WPC_EPP_FOD_9B_REG	            0x1B5

#define MFC_PMA_FOD_0A_REG					0x84
#define MFC_PMA_FOD_0B_REG					0x85
#define MFC_PMA_FOD_1A_REG					0x86
#define MFC_PMA_FOD_1B_REG					0x87
#define MFC_PMA_FOD_2A_REG					0x88
#define MFC_PMA_FOD_2B_REG					0x89
#define MFC_PMA_FOD_3A_REG					0x8A
#define MFC_PMA_FOD_3B_REG					0x8B

#define MFC_ADT_ERROR_CODE_REG				0x8D

#define MFC_TX_FOD_GAIN_REG					0x8F
#define MFC_TX_FOD_OFFSET_L_REG				0x90
#define MFC_TX_FOD_OFFSET_H_REG				0x91
#define MFC_TX_FOD_THRESH1_L_REG			0x92
#define MFC_TX_FOD_THRESH1_H_REG			0x93
#define MFC_TX_FOD_TA_THRESH_L_REG			0x98
#define MFC_TX_FOD_TA_THRESH_H_REG			0x99

#define MFX_TX_ID_VALUE_L_REG				0x9C
#define MFX_TX_ID_VALUE_H_REG				0x9D

#define MFC_DEMOD1_REG						0x9E
#define MFC_DEMOD2_REG						0x9F

#define MFC_TX_CONFLICT_CURRENT_REG			0xA0
#define MFC_RECT_MODE_AP_CTRL				0xA2

#define MFC_FW_DATA_CODE_0					0xB0
#define MFC_FW_DATA_CODE_1					0xB1
#define MFC_FW_DATA_CODE_2					0xB2
#define MFC_FW_DATA_CODE_3					0xB3
#define MFC_FW_DATA_CODE_4					0xB4
#define MFC_FW_DATA_CODE_5					0xB5
#define MFC_FW_DATA_CODE_6					0xB6
#define MFC_FW_DATA_CODE_7					0xB7
#define MFC_FW_DATA_CODE_8					0xB8
#define MFC_FW_DATA_CODE_9					0xB9
#define MFC_FW_DATA_CODE_A					0xBA

#define MFC_RX_PWR_L_REG					0xBC
#define MFC_RX_PWR_H_REG					0xBD
/* Timer code contains ASCII value. (ex. 31 means '1', 3A means ':') */
#define MFC_FW_TIMER_CODE_0					0xC0
#define MFC_FW_TIMER_CODE_1					0xC1
#define MFC_FW_TIMER_CODE_2					0xC2
#define MFC_FW_TIMER_CODE_3					0xC3
#define MFC_FW_TIMER_CODE_4					0xC4
#define MFC_FW_TIMER_CODE_5					0xC5
#define MFC_FW_TIMER_CODE_6					0xC6
#define MFC_FW_TIMER_CODE_7					0xC7

#define MFC_PWR_HOLD_INTERVAL_REG			0xCF

//#define MFC_TX_FOD_THRESH2_REG				0xE3
//#define MFC_TX_DUTY_CYCLE_REG				0xE6

//#define MFC_TX_PWR_L_REG					0xEC
//#define MFC_TX_PWR_H_REG					0xED

#define MFC_RPP_SCALE_COEF_REG					0xF0
//#define MFC_ACTIVE_LOAD_CONTROL_REG				0xF1
/* Parameter 1: Major and Minor Version */
#define MFC_TX_RXID1_READ_REG					0xF2
/* Parameter 2~3: Manufacturer Code */
#define MFC_TX_RXID2_READ_REG					0xF3
#define MFC_TX_RXID3_READ_REG					0xF4
/* Parameter 4~10: Basic Device Identifier */
//#define MFC_TX_RXID4_READ_REG					0xF5
//#define MFC_TX_RXID5_READ_REG					0xF6
//#define MFC_TX_RXID6_READ_REG					0xF7
//#define MFC_TX_RXID7_READ_REG					0xF8
//#define MFC_TX_RXID8_READ_REG					0xF9
//#define MFC_TX_RXID9_READ_REG					0xFA
//#define MFC_TX_RXID10_READ_REG					0xFB

/* Target Vrect is ReadOnly register, and updated by every 10ms
 * Its default value is 0x1A90(6800mV).
 * Target_Vrect (Iout,Vout) = {Vout + 0.05} + { Vrect(Iout,5V)-Vrect(1A,5V) } * 5/9
 */
#define MFC_TARGET_VRECT_L_REG				0x0164 /* default 0x90 */
#define MFC_TARGET_VRECT_H_REG				0x0165 /* default 0x1A */

#define MFC_MPP_CLOAK_REASON_REG                0x180
#define MFC_PROTOCOL_SELECTION_REG		        0x181
#define MFC_MPP_POWER_LEVEL_SETTING_REG         0x182
#define MFC_MPP_GP_STATE_REG		            0x183
#define MFC_MPP_EPP_NEGO_DONE_POWER_L_REG       0x184
#define MFC_MPP_EPP_NEGO_DONE_POWER_H_REG		0x185
#define MFC_MPP_EPP_POTENTIAL_LOAD_POWER_L_REG  0x186
#define MFC_MPP_EPP_POTENTIAL_LOAD_POWER_H_REG	0x187
#define MFC_MPP_EPP_NEGOTIABLE_LOAD_POWER_L_REG 0x188
#define MFC_MPP_EPP_NEGOTIABLE_LOAD_POWER_H_REG	0x189
#define MFC_MPP_PTX_EXTENDED_ID0_REG		    0x18A
#define MFC_MPP_PTX_EXTENDED_ID1_REG		    0x18B
#define MFC_MPP_PTX_EXTENDED_ID2_REG		    0x18C
#define MFC_MPP_DC_CURRENT_MOD_BASE_LIGHT_REG	0x18D
#define MFC_MPP_DC_CURRENT_MOD_BASE_HEAVY_REG	0x18E
#define MFC_MPP_DC_CURRENT_MOD_DEPTH_REG        0x18F
#define MFC_MPP_EXIT_CLOAK_REASON_REG           0x190
#define MFC_MPP_FOD_QF_REG		                0x191
#define MFC_MPP_FOD_RF_REG		                0x192
#define MFC_MPP_ALPHA0_RX_REG		            0x193
#define MFC_MPP_ALPHA1_RX_REG		            0x194
#define MFC_MPP_ALPHA_KTH_RX_REG		        0x195
#define MFC_MPP_RX_COIL_RAC_L_REG		        0x196
#define MFC_MPP_RX_COIL_RAC_H_REG		        0x197
#define MFC_MPP_RX_CAP_ESR_L_REG		        0x198
#define MFC_MPP_RX_CAP_ESR_H_REG		        0x199
#define MFC_MPP_M_COIL_L_REG		            0x19A
#define MFC_MPP_M_COIL_H_REG		            0x19B
#define MFC_MPP_ALPHA_FM_L_REG		            0x19C
#define MFC_MPP_ALPHA_FM_H_REG		            0x19D
#define MFC_MPP_ALPHA_FM_DC_L_REG		        0x19E
#define MFC_MPP_ALPHA_FM_DC_H_REG		        0x19F
#define MFC_MPP_G_COIL_Tx_L_REG		            0x1A0
#define MFC_MPP_G_COIL_Tx_H_REG		            0x1A1
#define MFC_MPP_THERMAL_CTRL_REG	            0x1B6
#define MFC_TX_WPC_AUTH_SUPPORT_REG	            0x1B7

#define MFC_CEP_TIME_OUT_REG					0x1B8
#define MFC_ESTIMATED_K_L_REG					0x1BA
#define MFC_ESTIMATED_K_H_REG					0x1BB
#define MFC_TARGET_ILIM_REG					0x1BC
#define MFC_RECENT_ILIM_REG					0x1BD

#define SS_ID		0x42
#define SS_CODE		0x64

/* Cloak reason Register, CLOAK_REASON (0x180) */
#define MFC_TRX_MPP_CLOAK_GENERIC			    0x0
#define MFC_TRX_MPP_CLOAK_FORCED			    0x1
#define MFC_TRX_MPP_CLOAK_THERMALLY_CONSTRAINED 0x2
#define MFC_TRX_MPP_CLOAK_INSUFFICIENT_POWER    0x3
#define MFC_TRX_MPP_CLOAK_COEX_MITIGATION	    0x4
#define MFC_TRX_MPP_CLOAK_END_OF_CHARGE			0x5
#define MFC_TRX_MPP_CLOAK_PTX_INITIATED			0x6

/* Cloak exit reason Register, EXIT_CLOAK_REASON (0x190) */
#define MFC_TRX_MPP_EXIT_CLOAK_NORMAL			    0x0
#define MFC_TRX_MPP_EXIT_CLOAK_FSK_FAIL			    0x1
#define MFC_TRX_MPP_EXIT_CLOAK_DET_PING_TIMEOUT     0x2
#define MFC_TRX_MPP_EXIT_CLOAK_PING_TIMEOUT         0x3
#define MFC_TRX_MPP_EXIT_CLOAK_PTX_ID_FAIL  	    0x4

/* full mode transfer type Register, FULL_MODE_TRANS_TYPE (0x181) */
#define MFC_RX_MPP_FULL_MODE_TRAN_NO_POWER_RESET   0x00
#define MFC_RX_MPP_FULL_MODE_TRAN_POWER_RESET      0x01

/* nego power level Register, POWER_LEVEL_SETTING (0x182) */
#define MFC_RX_MPP_NEGO_POWER_10W           10
#define MFC_RX_MPP_NEGO_POWER_15W           15

/* ADT type Register, ADT_TYPE (0x800[7:3]) */
#define MFC_ADT_FWC_EPP_END			        0x0
#define MFC_ADT_FWC_EPP_GENERAL			    0x1
#define MFC_ADT_FWC_EPP_AUTHENTICATION      0x2
#define MFC_ADT_FWC_EPP_RESET               0x12

#define MFC_ADT_MPP_RESET_ALL			    0x0
#define MFC_ADT_MPP_RESET_SINGLE			0x1
#define MFC_ADT_MPP_ABORT                   0x2
#define MFC_ADT_MPP_COMPLETE                0x3
#define MFC_ADT_MPP_OPEN                    0x4

#define MFC_RX_CEP_PACKET_COUNTER0			0x029C
#define MFC_RX_CEP_PACKET_COUNTER1			0x029D
#define MFC_RX_CEP_PACKET_COUNTER2			0x029E
#define MFC_RX_CEP_PACKET_COUNTER3			0x029F
#define MFC_RX_RPP_PACKET_COUNTER0			0x02A0
#define MFC_RX_RPP_PACKET_COUNTER1			0x02A1
#define MFC_RX_RPP_PACKET_COUNTER2			0x02A2
#define MFC_RX_RPP_PACKET_COUNTER3			0x02A3
#define MFC_RX_CSP_PACKET_COUNTER0			0x02A4
#define MFC_RX_CSP_PACKET_COUNTER1			0x02A5
#define MFC_RX_CSP_PACKET_COUNTER2			0x02A6
#define MFC_RX_CSP_PACKET_COUNTER3			0x02A7
#define MFC_RX_PPP_PACKET_COUNTER0			0x02A8
#define MFC_RX_PPP_PACKET_COUNTER1			0x02A9
#define MFC_RX_PPP_PACKET_COUNTER2			0x02AA
#define MFC_RX_PPP_PACKET_COUNTER3			0x02AB

/* ADT Buffer Registers, (0x0800 ~ 0x0FFF) */
#define MFC_ADT_BUFFER_ADT_TYPE_REG				0x0800
#define MFC_ADT_BUFFER_ADT_MSG_SIZE_REG			0x0801
#define MFC_ADT_BUFFER_ADT_PARAM_REG			0x0804
#define MFC_ADT_BUFFER_ADT_PARAM_MAX_REG		0x0FFF

/* System Operating Mode Register, Sys_Op_Mode (0x2B) */
#define PAD_MODE_MISSING			0
#define PAD_MODE_WPC_BASIC			1
#define PAD_MODE_WPC_ADV			2
#define PAD_MODE_PMA_SR1			3
#define PAD_MODE_PMA_SR1E			4
#define PAD_MODE_UNKNOWN			5

/* MFC_RX_DATA_COM_REG (0x51) : RX Data Command VALUE of 0x19 PPP Heaader */
#define	WPC_COM_CLEAR_PACKET_COUNTING		0x01
#define	WPC_COM_START_PACKET_COUNTING		0x02
#define	WPC_COM_DISABLE_PACKET_COUNTING		0x03

/* RX Data Value1 Register (Data Sending), RX_Data_VALUE1_Out (0x51) : Function and Description */
#define	WPC_COM_UNKNOWN					0x00
#define	WPC_COM_TX_ID					0x01
#define	WPC_COM_CHG_STATUS				0x05
#define	WPC_COM_AFC_SET					0x06
#define	WPC_COM_AFC_DEBOUNCE				0x07 /* Data Values [ 0~1000mV : 0x0000~0x03E8 ], 2 bytes*/
#define	WPC_COM_SID_TAG					0x08
#define	WPC_COM_SID_TOKEN				0x09
#define	WPC_COM_TX_STANDBY				0x0A
#define	WPC_COM_LED_CONTROL				0x0B /* Data Value LED Enable(0x00), LED Disable(0xFF) */
#define	WPC_COM_REQ_AFC_TX				0x0C /* Data Value (0x00) */
#define	WPC_COM_COOLING_CTRL				0x0D /* Data Value ON(0x00), OFF(0xFF) */
#define	WPC_COM_RX_ID					0x0E /* Received RX ID */
#define	WPC_COM_CHG_LEVEL				0x0F /* Battery level */
#define	WPC_COM_ENTER_PHM				0x18 /* GEAR entered PHM */
#define	WPC_COM_DISABLE_TX				0x19 /* Turn off UNO of TX, OFF(0xFF) */
#define	WPC_COM_PAD_LED					0x20 /* PAD LED */
#define	WPC_COM_REQ_PWR_BUDG			0x21
#define	WPC_COM_OP_FREQ_SET				0xD1
#define	WPC_COM_WDT_ERR					0xE7 /* Data Value WDT Error */

/* RX Data Value 2~5 Register (Data Sending), RX_Data_Value2_5_Out : Function and Description */
#define	RX_DATA_VAL2_5V					0x05
#define	RX_DATA_VAL2_10V				0x2C
#define	RX_DATA_VAL2_12V				0x4B
#define	RX_DATA_VAL2_12_5V				0x69
#define	RX_DATA_VAL2_20V				0x9B
#define	RX_DATA_VAL2_TA_CONNECT_DURING_WC		0x55
#define	RX_DATA_VAL2_MISALIGN				0xFF
#define	RX_DATA_VAL2_ENABLE				0x01

#define	RX_DATA_VAL2_RXID_ACC_BUDS			0x70
#define	RX_DATA_VAL2_RXID_ACC_BUDS_MAX		0x8F

/* MFC_TX_DATA_COM_REG (0x58) : TX Command */
#define	WPC_TX_COM_UNKNOWN		0x00
#define	WPC_TX_COM_TX_ID		0x01
#define	WPC_TX_COM_AFC_SET		0x02
#define	WPC_TX_COM_ACK			0x03
#define	WPC_TX_COM_NAK			0x04
#define WPC_TX_COM_CHG_ERR		0x05
#define WPC_TX_COM_WPS		0x07
#define WPC_TX_COM_RX_POWER		0x0A
#define WPC_TX_COM_TX_PWR_BUDG	0x0C

/* value of WPC_TX_COM_AFC_SET(0x02) */
#define TX_AFC_SET_5V			0x00
#define TX_AFC_SET_10V			0x01
#define TX_AFC_SET_12V			0x02
#define TX_AFC_SET_18V			0x03
#define TX_AFC_SET_19V			0x04
#define TX_AFC_SET_20V			0x05
#define TX_AFC_SET_24V			0x06

/* value of WPC_TX_COM_TX_ID(0x01) */
#define TX_ID_UNKNOWN				0x00
#define TX_ID_SNGL_PORT_START		0x01
#define TX_ID_VEHICLE_PAD			0x11
#define TX_ID_PG950_D_PAD			0x14
#define TX_ID_P1100_PAD			0x16
#define TX_ID_P1300_PAD			0x17
#define TX_ID_P2400_PAD_NOAUTH	0x18
#define TX_ID_SNGL_PORT_END		0x1F
#define TX_ID_MULTI_PORT_START	0x20
#define TX_ID_P4300_PAD			0x25
#define TX_ID_P5400_PAD_NOAUTH	0x27
#define TX_ID_MULTI_PORT_END		0x2F
#define TX_ID_STAND_TYPE_START	0x30
#define TX_ID_PG950_S_PAD			0x31
#define TX_ID_N3300_V_PAD			0x35
#define TX_ID_N3300_H_PAD			0xF2
#define TX_ID_STAND_TYPE_END		0x3F
#define TX_ID_BATT_PACK_TA			0x41 /* 0x40 ~ 0x41 is N/C*/
#define TX_ID_BATT_PACK_U1200		0x42
#define TX_ID_BATT_PACK_U3300		0x43
#define TX_ID_BATT_PACK_END		0x4F /* reserved 0x40 ~ 0x4F for wireless battery pack */
#define TX_ID_UNO_TX				0x72
#define TX_ID_UNO_TX_B0			0x80
#define TX_ID_UNO_TX_B1			0x81
#define TX_ID_UNO_TX_B2			0x82
#define TX_ID_UNO_TX_MAX			0x9F

#define TX_ID_AUTH_PAD				0xA0
#define TX_ID_P5200_PAD			0xA0
#define TX_ID_N5200_V_PAD			0xA1
#define TX_ID_N5200_H_PAD			0xA2
#define TX_ID_P2400_PAD			0xA3
#define TX_ID_P5400_PAD			0xA4
#define TX_ID_AUTH_PAD_ACLASS_END	0xAF
#define TX_ID_AUTH_PAD_END		0xBF /* reserved 0xA1 ~ 0xBF for auth pad */
#define TX_ID_JIG_PAD				0xED /* for factory */
#define TX_ID_FG_PAD				0xEF /* Galaxy Friends */
#define TX_ID_NON_AUTH_PAD		0xF0
#define TX_ID_NON_AUTH_PAD_END	0xFF

/* value of WPC_TX_COM_CHG_ERR(0x05) */
#define TX_CHG_ERR_OTP			0x12
#define TX_CHG_ERR_OCP			0x13
#define TX_CHG_ERR_DARKZONE		0x14
#define TX_CHG_ERR_FOD			(0x20 ... 0x27)

/* value of WPC_TX_COM_WPS 0x07) */
#define WPS_AICL_RESET		0x01

/* value of WPC_TX_COM_RX_POWER(0x0A) */
#define TX_RX_POWER_0W			0x0
#define TX_RX_POWER_3W			0x1E
#define TX_RX_POWER_5W			0x32
#define TX_RX_POWER_6W			0x3C
#define TX_RX_POWER_6_5W		0x41
#define TX_RX_POWER_7_5W		0x4B
#define TX_RX_POWER_8W			0x50
#define TX_RX_POWER_10W			0x64
#define TX_RX_POWER_11W			0x6E
#define TX_RX_POWER_12W			0x78
#define TX_RX_POWER_15W			0x96
#define TX_RX_POWER_17_5W		0xAF
#define TX_RX_POWER_20W			0xC8

#define MFC_NUM_FOD_REG					20

/* BIT DEFINE of Command Register, COM_L(0x4E) */
#define MFC_CMD_TOGGLE_PHM_SHIFT			7
#define MFC_CMD_RESERVED6_SHIFT				6
#define MFC_CMD_CLEAR_INT_SHIFT				5
#define MFC_CMD_SEND_CHG_STS_SHIFT			4
#define MFC_CMD_SEND_EOP_SHIFT				3
#define MFC_CMD_MCU_RESET_SHIFT				2
#define MFC_CMD_TOGGLE_LDO_SHIFT			1
#define MFC_CMD_SEND_TRX_DATA_SHIFT			0
#define MFC_CMD_TOGGLE_PHM_MASK		(1 << MFC_CMD_TOGGLE_PHM_SHIFT)
#define MFC_CMD_RESERVED6_MASK		(1 << MFC_CMD_RESERVED6_SHIFT)
#define MFC_CMD_CLEAR_INT_MASK		(1 << MFC_CMD_CLEAR_INT_SHIFT)
#define MFC_CMD_SEND_CHG_STS_MASK	(1 << MFC_CMD_SEND_CHG_STS_SHIFT) /* MFC MCU sends ChgStatus packet to TX */
#define MFC_CMD_SEND_EOP_MASK				(1 << MFC_CMD_SEND_EOP_SHIFT)
#define MFC_CMD_MCU_RESET_MASK				(1 << MFC_CMD_MCU_RESET_SHIFT)
#define MFC_CMD_TOGGLE_LDO_MASK				(1 << MFC_CMD_TOGGLE_LDO_SHIFT)
#define MFC_CMD_SEND_TRX_DATA_MASK			(1 << MFC_CMD_SEND_TRX_DATA_SHIFT)

/* Command Register, COM_H(0x4F) */
#define MFC_CMD2_MPP_STOP_SEND_ASK_SHIFT	5
#define MFC_CMD2_MPP_EXIT_CLOAK_SHIFT		4
#define MFC_CMD2_MPP_ENTER_CLOAK_SHIFT		3
#define MFC_CMD2_MPP_FULL_MODE_SHIFT		2
#define MFC_CMD2_ADT_SENT_SHIFT				1
#define MFC_CMD2_WP_ON_SHIFT				0
#define MFC_CMD2_MPP_STOP_SEND_ASK_MASK	(1 << MFC_CMD2_MPP_STOP_SEND_ASK_SHIFT)
#define MFC_CMD2_MPP_EXIT_CLOAK_MASK		(1 << MFC_CMD2_MPP_EXIT_CLOAK_SHIFT)
#define MFC_CMD2_MPP_ENTER_CLOAK_MASK		(1 << MFC_CMD2_MPP_ENTER_CLOAK_SHIFT)
#define MFC_CMD2_MPP_FULL_MODE_MASK			(1 << MFC_CMD2_MPP_FULL_MODE_SHIFT)
#define MFC_CMD2_ADT_SENT_MASK				(1 << MFC_CMD2_ADT_SENT_SHIFT)
#define MFC_CMD2_WP_ON_MASK					(1 << MFC_CMD2_WP_ON_SHIFT)

/* Protocol selection Register(0x181) */
#define MFC_PROTOCOL_MPP_RESTRICTED_SHIFT	2
#define MFC_PROTOCOL_MPP_FULL_SHIFT			1
#define MFC_PROTOCOL_EPP_SHIFT				0
#define MFC_PROTOCOL_MPP_RESTRICTED_MASK	(1 << MFC_PROTOCOL_MPP_RESTRICTED_SHIFT)
#define MFC_PROTOCOL_MPP_FULL_MASK			(1 << MFC_PROTOCOL_MPP_FULL_SHIFT)
#define MFC_PROTOCOL_EPP_MASK					(1 << MFC_PROTOCOL_EPP_SHIFT)
#define MFC_PROTOCOL_BPP_MASK					0

/* Chip Revision and Font Register, Chip_Rev (0x02) */
#define MFC_CHIP_REVISION_MASK				0xf0
#define MFC_CHIP_FONT_MASK					0x0f

/* BIT DEFINE of Status Registers, Status_L (0x20), Status_H (0x21), Status1_L (0x22), Status1_H (0x23) */
#define MFC_STAT_L_STAT_VOUT_SHIFT				7
#define MFC_STAT_L_STAT_VRECT_SHIFT				6
#define MFC_STAT_L_OP_MODE_SHIFT				5
#define MFC_STAT_L_OVER_VOL_SHIFT				4
#define MFC_STAT_L_OVER_CURR_SHIFT				3
#define MFC_STAT_L_OVER_TEMP_SHIFT				2
#define MFC_STAT_L_TXCONFLICT_SHIFT				1
#define MFC_STAT_L_ADT_ERROR_SHIFT				0
#define MFC_STAT_L_STAT_VOUT_MASK				(1 << MFC_STAT_L_STAT_VOUT_SHIFT)
#define MFC_STAT_L_STAT_VRECT_MASK				(1 << MFC_STAT_L_STAT_VRECT_SHIFT)
#define MFC_STAT_L_OP_MODE_MASK					(1 << MFC_STAT_L_OP_MODE_SHIFT)
#define MFC_STAT_L_OVER_VOL_MASK				(1 << MFC_STAT_L_OVER_VOL_SHIFT)
#define MFC_STAT_L_OVER_CURR_MASK				(1 << MFC_STAT_L_OVER_CURR_SHIFT)
#define MFC_STAT_L_OVER_TEMP_MASK				(1 << MFC_STAT_L_OVER_TEMP_SHIFT)
#define MFC_STAT_L_TXCONFLICT_MASK				(1 << MFC_STAT_L_TXCONFLICT_SHIFT)
#define MFC_STAT_L_ADT_ERROR_MASK				(1 << MFC_STAT_L_ADT_ERROR_SHIFT)

#define MFC_STAT_H_TRX_DATA_RECEIVED_SHIFT		7
#define MFC_STAT_H_TX_OCP_SHIFT					6
#define MFC_STAT_H_TX_MODE_RX_NOT_DET_SHIFT		5
#define MFC_STAT_H_TX_FOD_SHIFT					4
#define MFC_STAT_H_TX_CON_DISCON_SHIFT			3
#define MFC_STAT_H_AC_MISSING_DET_SHIFT			2
#define MFC_STAT_H_ADT_RECEIVED_SHIFT			1
#define MFC_STAT_H_ADT_SENT_SHIFT				0
#define MFC_STAT_H_TRX_DATA_RECEIVED_MASK		(1 << MFC_STAT_H_TRX_DATA_RECEIVED_SHIFT)
#define MFC_STAT_H_TX_OCP_MASK					(1 << MFC_STAT_H_TX_OCP_SHIFT)
#define MFC_STAT_H_TX_MODE_RX_NOT_DET_MASK		(1 << MFC_STAT_H_TX_MODE_RX_NOT_DET_SHIFT)
#define MFC_STAT_H_TX_FOD_MASK					(1 << MFC_STAT_H_TX_FOD_SHIFT)
#define MFC_STAT_H_TX_CON_DISCON_MASK			(1 << MFC_STAT_H_TX_CON_DISCON_SHIFT)
#define MFC_STAT_H_AC_MISSING_DET_MASK			(1 << MFC_STAT_H_AC_MISSING_DET_SHIFT)
#define MFC_STAT_H_ADT_RECEIVED_MASK			(1 << MFC_STAT_H_ADT_RECEIVED_SHIFT)
#define MFC_STAT_H_ADT_SENT_MASK				(1 << MFC_STAT_H_ADT_SENT_SHIFT)

#define MFC_STAT1_L_EPP_NEGO_FAIL_SHIFT			7
#define MFC_STAT1_L_EPP_NEGO_PASS_SHIFT			6
#define MFC_STAT1_L_EXIT_CLOAK_SHIFT			5
#define MFC_STAT1_L_DECREASE_POWER_SHIFT		4
#define MFC_STAT1_L_INCREASE_POWER_SHIFT		3
#define MFC_STAT1_L_360K_NEGO_PASS_SHIFT		2
#define MFC_STAT1_L_EPP_SUPPROT_SHIFT		    1
#define MFC_STAT1_L_MPP_SUPPROT_SHIFT			0
#define MFC_STAT1_L_EPP_NEGO_FAIL_MASK			(1 << MFC_STAT1_L_EPP_NEGO_FAIL_SHIFT)
#define MFC_STAT1_L_EPP_NEGO_PASS_MASK			(1 << MFC_STAT1_L_EPP_NEGO_PASS_SHIFT)
#define MFC_STAT1_L_EXIT_CLOAK_MASK				(1 << MFC_STAT1_L_EXIT_CLOAK_SHIFT)
#define MFC_STAT1_L_DECREASE_POWER_MASK			(1 << MFC_STAT1_L_DECREASE_POWER_SHIFT)
#define MFC_STAT1_L_INCREASE_POWER_MASK			(1 << MFC_STAT1_L_INCREASE_POWER_SHIFT)
#define MFC_STAT1_L_360K_NEGO_PASS_MASK			(1 << MFC_STAT1_L_360K_NEGO_PASS_SHIFT)
#define MFC_STAT1_L_EPP_SUPPROT_MASK			(1 << MFC_STAT1_L_EPP_SUPPROT_SHIFT)
#define MFC_STAT1_L_MPP_SUPPROT_MASK			(1 << MFC_STAT1_L_MPP_SUPPROT_SHIFT)

#define MFC_STAT1_H_PROTOCOL_SELECT_SHIFT		0
#define MFC_STAT1_H_PROTOCOL_SELECT_MASK		(1 << MFC_STAT1_H_PROTOCOL_SELECT_SHIFT)

/* BIT DEFINE of Interrupt_A Registers, INTA_L (0x24), INTA_H (0x25), INTB_L (0x26), INTB_H (0x27) */
#define MFC_INTA_L_STAT_VOUT_SHIFT				7
#define MFC_INTA_L_STAT_VRECT_SHIFT				6
#define MFC_INTA_L_OP_MODE_SHIFT				5
#define MFC_INTA_L_OVER_VOL_SHIFT				4
#define MFC_INTA_L_OVER_CURR_SHIFT				3
#define MFC_INTA_L_OVER_TEMP_SHIFT				2
#define MFC_INTA_L_TXCONFLICT_SHIFT				1
#define MFC_INTA_L_ADT_ERROR_SHIFT				0
#define MFC_INTA_L_STAT_VOUT_MASK				(1 << MFC_INTA_L_STAT_VOUT_SHIFT)
#define MFC_INTA_L_STAT_VRECT_MASK				(1 << MFC_INTA_L_STAT_VRECT_SHIFT)
#define MFC_INTA_L_OP_MODE_MASK					(1 << MFC_INTA_L_OP_MODE_SHIFT)
#define MFC_INTA_L_OVER_VOL_MASK				(1 << MFC_INTA_L_OVER_VOL_SHIFT)
#define MFC_INTA_L_OVER_CURR_MASK				(1 << MFC_STAT_L_OVER_CURR_SHIFT)
#define MFC_INTA_L_OVER_TEMP_MASK				(1 << MFC_STAT_L_OVER_TEMP_SHIFT)
#define MFC_INTA_L_TXCONFLICT_MASK				(1 << MFC_STAT_L_TXCONFLICT_SHIFT)
#define MFC_INTA_L_ADT_ERROR_MASK				(1 << MFC_INTA_L_ADT_ERROR_SHIFT)

#define MFC_INTA_H_TRX_DATA_RECEIVED_SHIFT		7
#define MFC_INTA_H_TX_OCP_SHIFT					6
#define MFC_INTA_H_TX_MODE_RX_NOT_DET			5
#define MFC_INTA_H_TX_FOD_SHIFT					4
#define MFC_INTA_H_TX_CON_DISCON_SHIFT			3
#define MFC_INTA_H_AC_MISSING_DET_SHIFT			2
#define MFC_INTA_H_ADT_RECEIVED_SHIFT			1
#define MFC_INTA_H_ADT_SENT_SHIFT				0
#define MFC_INTA_H_TRX_DATA_RECEIVED_MASK		(1 << MFC_INTA_H_TRX_DATA_RECEIVED_SHIFT)
#define MFC_INTA_H_TX_OCP_MASK					(1 << MFC_INTA_H_TX_OCP_SHIFT)
#define MFC_INTA_H_TX_MODE_RX_NOT_DET_MASK			(1 << MFC_INTA_H_TX_MODE_RX_NOT_DET)
#define MFC_INTA_H_TX_FOD_MASK					(1 << MFC_INTA_H_TX_FOD_SHIFT)
#define MFC_INTA_H_TX_CON_DISCON_MASK			(1 << MFC_INTA_H_TX_CON_DISCON_SHIFT)
#define MFC_INTA_H_AC_MISSING_DET_MASK			(1 << MFC_INTA_H_AC_MISSING_DET_SHIFT)
#define MFC_INTA_H_ADT_RECEIVED_MASK			(1 << MFC_INTA_H_ADT_RECEIVED_SHIFT)
#define MFC_INTA_H_ADT_SENT_MASK				(1 << MFC_INTA_H_ADT_SENT_SHIFT)

#define MFC_INTB_L_EPP_NEGO_FAIL_SHIFT		    7
#define MFC_INTB_L_EPP_NEGO_PASS_SHIFT		    6
#define MFC_INTB_L_EXIT_CLOAK_SHIFT			    5
#define MFC_INTB_L_DECREASE_POWER_SHIFT		    4
#define MFC_INTB_L_INCREASE_POWER_SHIFT		    3
#define MFC_INTB_L_360K_NEGO_PASS_SHIFT		    2
#define MFC_INTB_L_EPP_SUPPROT_SHIFT		    1
#define MFC_INTB_L_MPP_SUPPROT_SHIFT		    0
#define MFC_INTB_L_EPP_NEGO_FAIL_MASK			(1 << MFC_INTB_L_EPP_NEGO_FAIL_SHIFT)
#define MFC_INTB_L_EPP_NEGO_PASS_MASK			(1 << MFC_INTB_L_EPP_NEGO_PASS_SHIFT)
#define MFC_INTB_L_EXIT_CLOAK_MASK			    (1 << MFC_INTB_L_EXIT_CLOAK_SHIFT)
#define MFC_INTB_L_DECREASE_POWER_MASK		    (1 << MFC_INTB_L_DECREASE_POWER_SHIFT)
#define MFC_INTB_L_INCREASE_POWER_MASK		    (1 << MFC_INTB_L_INCREASE_POWER_SHIFT)
#define MFC_INTB_L_360K_NEGO_PASS_MASK		    (1 << MFC_INTB_L_360K_NEGO_PASS_SHIFT)
#define MFC_INTB_L_EPP_SUPPROT_MASK			    (1 << MFC_INTB_L_EPP_SUPPROT_SHIFT)
#define MFC_INTB_L_MPP_SUPPROT_MASK			    (1 << MFC_INTB_L_MPP_SUPPROT_SHIFT)

#define MFC_INTB_H_PROTOCOL_SELECT_SHIFT			0
#define MFC_INTB_H_PROTOCOL_SELECT_MASK			(1 << MFC_INTB_H_PROTOCOL_SELECT_SHIFT)

/* System Operating Mode Register, Sys_op_mode(0x2B) */
/* RX MODE[7:5] */
#define MFC_RX_MODE_AC_MISSING					0x0
#define MFC_RX_MODE_WPC_BASIC					0x1
#define MFC_RX_MODE_WPC_ADV						0x2
#define MFC_RX_MODE_PMA_SR1						0x3
#define MFC_RX_MODE_PMA_SR1E					0x4
#define MFC_RX_MODE_RESERVED1					0x5
#define MFC_RX_MODE_RESERVED2					0x6
#define MFC_RX_MODE_UNKNOWN						0x7

//#if defined(CONFIG_WIRELESS_CHARGER_cps4038)
/* TX MODE[3:0] */
#define MFC_TX_MODE_RX_MODE				0x0
#define MFC_TX_MODE_MST_MODE1			0x1
#define MFC_TX_MODE_MST_MODE2			0x2
#define MFC_TX_MODE_TX_MODE				0x3
#define MFC_TX_MODE_MST_PCR_MODE1		0x7
#define MFC_TX_MODE_MST_PCR_MODE2		0xF
//#endif
/* TX MODE[3:0] */
#define MFC_TX_MODE_BACK_PWR_MISSING			0x0
#define MFC_TX_MODE_MST_ON						0x4
#define MFC_TX_MODE_TX_MODE_ON					0x8
#define MFC_TX_MODE_TX_ERROR					0x9 /* TX FOD, TX conflict */
#define MFC_TX_MODE_TX_PWR_HOLD					0xA

/* End of Power Transfer Register, EPT (0x3B) (RX only) */
#define MFC_WPC_EPT_UNKNOWN					0x0
#define MFC_WPC_EPT_END_OF_CHG				0x1
#define MFC_WPC_EPT_INT_FAULT					0x2
#define MFC_WPC_EPT_OVER_TEMP				0x3
#define MFC_WPC_EPT_OVER_VOL					0x4
#define MFC_WPC_EPT_OVER_CURR				0x5
#define MFC_WPC_EPT_BATT_FAIL					0x6
#define MFC_WPC_EPT_RECONFIG					0x7
#define MFC_WPC_EPT_NO_RESPONSE				0x8
#define MFC_WPC_EPT_ABORTED_NEGO			0xA
#define MFC_WPC_EPT_RESTART					0xB

/* Proprietary Packet Header Register, PPP_Header VALUE(0x50) */
#define MFC_HEADER_END_SIG_STRENGTH			0x01 /* Message Size 1 */
#define MFC_HEADER_END_POWER_TRANSFER		0x02 /* Message Size 1 */
#define MFC_HEADER_END_CTR_ERROR			0x03 /* Message Size 1 */
#define MFC_HEADER_END_RECEIVED_POWER		0x04 /* Message Size 1 */
#define MFC_HEADER_END_CHARGE_STATUS		0x05 /* Message Size 1 */
#define MFC_HEADER_POWER_CTR_HOLD_OFF		0x06 /* Message Size 1 */
#define MFC_HEADER_PROPRIETARY_1_BYTE		0x18 /* Message Size 1 */
#define MFC_HEADER_PACKET_COUNTING			0x19 /* Message Size 1 */
#define MFC_HEADER_CLOAK						0x1E
#define MFC_HEADER_AFC_CONF					0x28 /* Message Size 2 */
#define MFC_HEADER_CONFIGURATION			0x51 /* Message Size 5 */
#define MFC_HEADER_IDENTIFICATION			0x71 /* Message Size 7 */
#define MFC_HEADER_EXTENDED_IDENT			0x81 /* Message Size 8 */

/* END CHARGE STATUS CODES IN WPC */
#define	MFC_ECS_CS100					0x64 /* CS 100 */

/* TX Data Command Register, TX Data_COM VALUE(0x50) */
#define MFC_TX_DATA_COM_TX_ID				0x01

/* END POWER TRANSFER CODES IN WPC */
#define MFC_EPT_CODE_UNKNOWN				0x00
#define MFC_EPT_CODE_CHARGE_COMPLETE		0x01
#define MFC_EPT_CODE_INTERNAL_FAULT		0x02
#define MFC_EPT_CODE_OVER_TEMPERATURE		0x03
#define MFC_EPT_CODE_OVER_VOLTAGE			0x04
#define MFC_EPT_CODE_OVER_CURRENT			0x05
#define MFC_EPT_CODE_BATTERY_FAILURE		0x06
#define MFC_EPT_CODE_RECONFIGURE			0x07
#define MFC_EPT_CODE_NO_RESPONSE			0x08

#define MFC_POWER_MODE_MASK				(0x1 << 0)
#define MFC_SEND_USER_PKT_DONE_MASK		(0x1 << 7)
#define MFC_SEND_USER_PKT_ERR_MASK		(0x3 << 5)
#define MFC_SEND_ALIGN_MASK				(0x1 << 3)
#define MFC_SEND_EPT_CC_MASK			(0x1 << 0)
#define MFC_SEND_EOC_MASK				(0x1 << 0)

#define MFC_PTK_ERR_NO_ERR				0x00
#define MFC_PTK_ERR_ERR					0x01
#define MFC_PTK_ERR_ILLEGAL_HD			0x02
#define MFC_PTK_ERR_NO_DEF				0x03

#define MFC_FW_RESULT_DOWNLOADING		2
#define MFC_FW_RESULT_PASS				1
#define MFC_FW_RESULT_FAIL				0

#define REQ_AFC_DLY	300

#define MFC_FW_MSG		"@MFC_FW "

/* value of TX POWER BUDGET */
#define MFC_TX_PWR_BUDG_NONE	0x00
#define MFC_TX_PWR_BUDG_2W		0x14
#define MFC_TX_PWR_BUDG_5W		0x32
#define MFC_TX_PWR_BUDG_7_5W	0x4B
#define MFC_TX_PWR_BUDG_10W		0x64
#define MFC_TX_PWR_BUDG_12W		0x78
#define MFC_TX_PWR_BUDG_15W		0x96

/* CPS F/W Update & Verification Define */
#define ADDR_BUFFER0        0x20000800
#define ADDR_BUFFER1        0x20001000
#define ADDR_CMD            0x20001800
#define ADDR_FLAG           0x20001804
#define ADDR_BUF_SIZE       0x20001808
#define ADDR_FW_VER         0x2000180C

#define PGM_BUFFER0         0x10
#define PGM_BUFFER1         0x20
#define PGM_BUFFER2         0x30
#define PGM_BUFFER3         0x40
#define PGM_BUFFER0_1       0x50
#define PGM_ERASER_0        0x60
#define PGM_ERASER_1        0x70
#define PGM_WR_FLAG         0x80

#define CACL_CRC_APP        0x90
#define CACL_CRC_TEST       0xB0

#define PGM_ADDR_SET        0xC0

#define RUNNING             0x66
#define PASS                0x55
#define FAIL                0xAA
#define ILLEGAL             0x40

#if defined(CONFIG_MST_V2)
#define MST_MODE_ON				1		// ON Message to MFC ic
#define MST_MODE_OFF			0		// OFF Message to MFC ic
#define DELAY_FOR_MST			100		// S.LSI : 100 ms
#define MFC_MST_LDO_CONFIG_1	0x7400
#define MFC_MST_LDO_CONFIG_2	0x7409
#define MFC_MST_LDO_CONFIG_3	0x7418
#define MFC_MST_LDO_CONFIG_4	0x3014
#define MFC_MST_LDO_CONFIG_5	0x3405
#define MFC_MST_LDO_CONFIG_6	0x3010
#define MFC_MST_LDO_TURN_ON		0x301c
#define MFC_MST_LDO_CONFIG_8	0x343c
#define MFC_MST_OVER_TEMP_INT	0x0024
#define MFC_ISET_PCR			0x0169
#define MFC_RES_PCR				0x016A
#define PCR_FIX_MODE			0x0168
#endif

/* F/W Update & Verification ERROR CODES */
enum {
	MFC_FWUP_ERR_COMMON_FAIL = 0,
	MFC_FWUP_ERR_SUCCEEDED,
	MFC_FWUP_ERR_RUNNING,

	MFC_FWUP_ERR_REQUEST_FW_BIN,

	/* F/W update error */
	MFC_FWUP_ERR_WRITE_KEY_ERR,
	MFC_FWUP_ERR_CLK_TIMING_ERR1,  /* 5 */
	MFC_FWUP_ERR_CLK_TIMING_ERR2,
	MFC_FWUP_ERR_CLK_TIMING_ERR3,
	MFC_FWUP_ERR_CLK_TIMING_ERR4,
	MFC_FWUP_ERR_INFO_PAGE_EMPTY,
	MFC_FWUP_ERR_HALT_M0_ERR, /* 10 */
	MFC_FWUP_ERR_FAIL,
	MFC_FWUP_ERR_ADDR_READ_FAIL,
	MFC_FWUP_ERR_DATA_NOT_MATCH,
	MFC_FWUP_ERR_OTP_LOADER_IN_RAM_ERR,
	MFC_FWUP_ERR_CLR_MTP_STATUS_BYTE, /* 15 */
	MFC_FWUP_ERR_MAP_RAM_TO_OTP_ERR,
	MFC_FWUP_ERR_WRITING_TO_OTP_BUFFER,
	MFC_FWUP_ERR_OTF_BUFFER_VALIDATION,
	MFC_FWUP_ERR_READING_OTP_BUFFER_STATUS,
	MFC_FWUP_ERR_TIMEOUT_ON_BUFFER_TO_OTP, /* 20 */
	MFC_FWUP_ERR_MTP_WRITE_ERR,
	MFC_FWUP_ERR_PKT_CHECKSUM_ERR,
	MFC_FWUP_ERR_UNKNOWN_ERR,
	MFC_FWUP_ERR_BUFFER_WRITE_IN_SECTOR,
	MFC_FWUP_ERR_WRITING_FW_VERION, /* 25 */

	/* F/W verification error */
	MFC_VERIFY_ERR_WRITE_KEY_ERR,
	MFC_VERIFY_ERR_HALT_M0_ERR,
	MFC_VERIFY_ERR_KZALLOC_ERR,
	MFC_VERIFY_ERR_FAIL,
	MFC_VERIFY_ERR_ADDR_READ_FAIL, /* 30 */
	MFC_VERIFY_ERR_DATA_NOT_MATCH,
	MFC_VERIFY_ERR_MTP_VERIFIER_IN_RAM_ERR,
	MFC_VERIFY_ERR_CLR_MTP_STATUS_BYTE,
	MFC_VERIFY_ERR_MAP_RAM_TO_OTP_ERR,
	MFC_VERIFY_ERR_UNLOCK_SYS_REG_ERR, /* 35 */
	MFC_VERIFY_ERR_LDO_CLK_2MHZ_ERR,
	MFC_VERIFY_ERR_LDO_OUTPUT_5_5V_ERR,
	MFC_VERIFY_ERR_ENABLE_LDO_ERR,
	MFC_VERIFY_ERR_WRITING_TO_MTP_VERIFY_BUFFER,
	MFC_VERIFY_ERR_START_MTP_VERIFY_ERR, /* 40 */
	MFC_VERIFY_ERR_READING_MTP_VERIFY_STATUS,
	MFC_VERIFY_ERR_CRC_BUSY,
	MFC_VERIFY_ERR_READING_MTP_VERIFY_PASS_FAIL,
	MFC_VERIFY_ERR_CRC_ERROR,
	MFC_VERIFY_ERR_UNKNOWN_ERR, /* 45 */
	MFC_VERIFY_ERR_BUFFER_WRITE_IN_SECTOR,

	MFC_REPAIR_ERR_HALT_M0_ERR,
	MFC_REPAIR_ERR_MTP_REPAIR_IN_RAM,
	MFC_REPAIR_ERR_CLR_MTP_STATUS_BYTE,
	MFC_REPAIR_ERR_START_MTP_REPAIR_ERR, /* 50 */
	MFC_REPAIR_ERR_READING_MTP_REPAIR_STATUS,
	MFC_REPAIR_ERR_READING_MTP_REPAIR_PASS_FAIL,
	MFC_REPAIR_ERR_BUFFER_WRITE_IN_SECTOR,
};

/* PAD Vout */
enum {
	PAD_VOUT_5V = 0,
	PAD_VOUT_9V,
	PAD_VOUT_10V,
	PAD_VOUT_12V,
	PAD_VOUT_18V,
	PAD_VOUT_19V,
	PAD_VOUT_20V,
	PAD_VOUT_24V,
};

enum {
	MFC_ADC_VOUT = 0,
	MFC_ADC_VRECT,
	MFC_ADC_RX_IOUT,
	MFC_ADC_DIE_TEMP,
	MFC_ADC_OP_FRQ,
	MFC_ADC_TX_MAX_OP_FRQ,
	MFC_ADC_TX_MIN_OP_FRQ,
	MFC_ADC_PING_FRQ,
	MFC_ADC_TX_IOUT,
	MFC_ADC_TX_VOUT,
};

enum {
	MFC_ADDR = 0,
	MFC_SIZE,
	MFC_DATA,
	MFC_PACKET,
	MFC_EPP_T,
};

enum {
	FOD_CNT_CLEAR = 0,
	FOD_CNT_ADD,
};

ssize_t cps4038_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t cps4038_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define CPS4038_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0660},	\
	.show = cps4038_show_attrs,			\
	.store = cps4038_store_attrs,			\
}

enum mfc_irq {
	MFC_IRQ_STAT_VOUT = 0,
	MFC_IRQ_STAT_VRECT,
	MFC_IRQ_MODE_CHANGE,
	MFC_IRQ_TX_DATA_RECEIVED,
	MFC_IRQ_OVER_VOLT,
	MFC_IRQ_OVER_CURR,
	MFC_IRQ_OVER_TEMP,
	MFC_IRQ_TX_OVER_CURR,
	MFC_IRQ_TX_OVER_TEMP,
	MFC_IRQ_TX_FOD,
	MFC_IRQ_TX_CONNECT,
	MFC_IRQ_NR,
};

enum mfc_firmware_mode {
	MFC_RX_FIRMWARE = 0,
	MFC_TX_FIRMWARE,
};

enum mfc_ic_revision {
	MFC_IC_REVISION = 0,
	MFC_IC_FONT,
};

enum mfc_chip_id {
	MFC_CHIP_IDT = 1,
	MFC_CHIP_LSI,
	MFC_CHIP_CPS,
};

enum mfc_headroom {
	MFC_HEADROOM_0 = 0,
	MFC_HEADROOM_1, /* 0.277V */
	MFC_HEADROOM_2, /* 0.497V */
	MFC_HEADROOM_3, /* 0.650V */
	MFC_HEADROOM_4, /* 0.030V */
	MFC_HEADROOM_5, /* 0.082V */
	MFC_HEADROOM_6, /* 0.097V */
	MFC_HEADROOM_7, /* -0.600V */
};

#if defined(CONFIG_WIRELESS_IC_PARAM)
extern unsigned int wireless_fw_ver_param;
extern unsigned int wireless_chip_id_param;
extern unsigned int wireless_fw_mode_param;
#endif

struct mfc_charger_platform_data {
	int pad_mode;
	int wpc_det;
	int irq_wpc_det;
	int wpc_int;
	int mst_pwr_en;
	int wpc_en;
	int mag_det;
	int wpc_mode;
	int mpp_sw;
	int coil_sw_en;
	int wpc_pdrc;
	int irq_wpc_pdrc;
	int ping_nen;
	int irq_wpc_int;
	int wpc_pdet_b;
	int irq_wpc_pdet_b;
	int cs100_status;
	int vout_status;
	int siop_level;
	int cable_type;
	bool default_voreg;
	int is_charging;
	u32 *wireless20_vout_list;
	u32 *wireless20_vrect_list;
	u32 *wireless20_max_power_list;
	u8 len_wc20_list;
	bool ic_on_mode;
	int hw_rev_changed; /* this is only for noble/zero2 */
	int otp_firmware_result;
	int tx_firmware_result;
	int wc_ic_grade;
	int wc_ic_rev;
	int vout;
	int vrect;
	u8 trx_data_cmd;
	u8 trx_data_val;
	char *wireless_charger_name;
	char *wired_charger_name;
	char *fuelgauge_name;
	int opfq_cnt;
	int mst_switch_delay;
	int wc_cover_rpp;
	int wc_hv_rpp;
	u32 tx_fod_gain;
	u32 tx_fod_offset;
	u32 phone_fod_thresh1;
	u32 phone_fod_ta_thresh;
	u32 buds_fod_thresh1;
	u32 buds_fod_ta_thresh;
	u32 tx_max_op_freq;
	u32 tx_min_op_freq;
	u32 gear_op_freq;
	u32 gear_min_op_freq;
	u32 gear_min_op_freq_delay;
	u32 cep_timeout;
	int no_hv;
	bool keep_tx_vout;
	u32 wpc_vout_ctrl_full;
	bool wpc_headroom_ctrl_full;
	bool mis_align_guide;
	bool unknown_cmb_ctrl;
	bool default_clamp_volt;
	u32 mis_align_target_vout;
	u32 mis_align_offset;
	int tx_conflict_curr;
	u32 iec_qfod_enable;
	u32 iec_q_thresh_1;
	u32 iec_q_thresh_2;
	u32 iec_fres_thresh_1;
	u32 iec_fres_thresh_2;
	u32 iec_power_limit_thresh;
	u32 iec_ploss_thresh_1;
	u32 iec_ploss_thresh_2;
	u32 iec_ploss_freq_thresh_1;
	u32 iec_ploss_freq_thresh_2;
	u32 iec_ta_power_limit_thresh;
	u32 iec_ta_ploss_thresh_1;
	u32 iec_ta_ploss_thresh_2;
	u32 iec_ta_ploss_freq_thresh_1;
	u32 iec_ta_ploss_freq_thresh_2;
	u32 iec_ploss_fod_enable;

#if defined(CONFIG_MST_PCR)
	u32 mst_iset_pcr;
#endif
	u32 epp_vout;
	u32 mpp_vout;
	u32 mpp_epp_def_power;
	u32 mpp_epp_max_count;
	u32 mpp_epp_power_level;

	u32 alpha0_rx;
	u32 alpha1_rx;
	u32 alpha_kth_rx;
	u32 rx_coil_rac;
	u32 rx_cap_esr;
	u32 m_coil;
	u32 alpha_fm;
	u32 alpha_fm_dc;
	u32 g_coil_tx;
	int high_swell_recov;
	int hall_ic_notifier;
	int epp_time_init;
	int epp_time_mid_1;
	int epp_time_mid_2;
	int epp_time_final;

	int fod_cnt_thresh;
	bool mpp_disable;
};

#define mfc_charger_platform_data_t \
	struct mfc_charger_platform_data

#define MST_MODE_0			0
#define MST_MODE_2			1

#define MFC_BAT_DUMP_SIZE	256

struct mfc_charger_data {
	struct i2c_client				*client;
	struct device					*dev;
	mfc_charger_platform_data_t		*pdata;
	struct mutex io_lock;
	struct mutex wpc_en_lock;
	struct mutex fw_lock;

	u8 det_state; /* ACTIVE HIGH */
	u8 pdrc_state; /* ACTIVE LOW */

	struct power_supply *psy_chg;
	struct wakeup_source *wpc_ws;
	struct wakeup_source *wpc_det_ws;
	struct wakeup_source *wpc_tx_ws;
	struct wakeup_source *wpc_rx_ws;
	struct wakeup_source *wpc_update_ws;
	struct wakeup_source *wpc_tx_duty_min_ws;
	struct wakeup_source *wpc_tx_min_opfq_ws;
	struct wakeup_source *wpc_afc_vout_ws;
	struct wakeup_source *wpc_vout_mode_ws;
	struct wakeup_source *wpc_rx_det_ws;
	struct wakeup_source *wpc_tx_phm_ws;
	struct wakeup_source *wpc_tx_id_ws;
	struct wakeup_source *wpc_tx_pwr_budg_ws;
	struct wakeup_source *wpc_pdrc_ws;
	struct wakeup_source *align_check_ws;
	struct wakeup_source *mode_change_ws;
	struct wakeup_source *wpc_cs100_ws;
	struct wakeup_source *wpc_rx_power_trans_fail_ws;
	struct wakeup_source *wpc_pdet_b_ws;
	struct wakeup_source *wpc_rx_phm_ws;
	struct wakeup_source *wpc_phm_exit_ws;
	struct wakeup_source *epp_clear_ws;
	struct wakeup_source *epp_count_ws;
	struct wakeup_source *check_mpp_ws;
	struct wakeup_source *epp_mode_change_check_ws;
	struct wakeup_source *mpp_fake_ui_clear_ws;
	struct wakeup_source *set_mpp_cover_ws;
	struct wakeup_source *set_mpp_cloak_ws;
	struct wakeup_source *mpla_thr_recov_ws;
	struct workqueue_struct *wqueue;
	struct work_struct wcin_work;
	struct delayed_work wpc_det_work;
	struct delayed_work wpc_pdrc_work;
	struct delayed_work wpc_isr_work;
	struct delayed_work wpc_tx_isr_work;
	struct delayed_work wpc_tx_id_work;
	struct delayed_work wpc_tx_pwr_budg_work;
	struct delayed_work mst_off_work;
	struct delayed_work wpc_int_req_work;
	struct delayed_work wpc_fw_update_work;
	struct delayed_work wpc_afc_vout_work;
	struct delayed_work wpc_fw_booting_work;
	struct delayed_work wpc_vout_mode_work;
	struct delayed_work wpc_i2c_error_work;
	struct delayed_work wpc_rx_type_det_work;
	struct delayed_work wpc_rx_connection_work;
	struct delayed_work wpc_tx_op_freq_work;
	struct delayed_work wpc_tx_duty_min_work;
	struct delayed_work wpc_tx_min_op_freq_work;
	struct delayed_work wpc_tx_phm_work;
	struct delayed_work wpc_vrect_check_work;
	struct delayed_work wpc_rx_power_work;
	struct delayed_work wpc_cs100_work;
	struct delayed_work wpc_init_work;
	struct delayed_work align_check_work;
	struct delayed_work mode_change_work;
	struct delayed_work wpc_rx_power_trans_fail_work;
	struct delayed_work wpc_rx_phm_work;
	struct delayed_work wpc_deactivate_work;
	struct delayed_work wpc_phm_exit_work;
	struct delayed_work epp_clear_timer_work;
	struct delayed_work epp_count_work;
	struct delayed_work wpc_check_mpp_work;
	struct delayed_work epp_mode_change_check_work;
	struct delayed_work mpp_fake_ui_clear_work;
	struct delayed_work set_mpp_cover_work;
	struct delayed_work set_mpp_cloak_work;
	struct delayed_work mpla_thr_recov_work;

	struct alarm phm_alarm;

	struct mfc_fod *fod;
	struct mfc_cmfet *cmfet;

	u16 addr;
	int size;
	int is_afc;
	int pad_vout;
	int is_mst_on; /* mst */
	int chip_id;
	u8 rx_op_mode;
	u8 rx_op_mode_before_otg;
	int fw_cmd;
	int vout_mode;
	u32 vout_by_txid;
	u32 vrect_by_txid;
	u32 max_power_by_txid;
	int is_full_status;
	int mst_off_lock;
	bool is_otg_on;
	int led_cover;
	bool is_probed;
	bool is_afc_tx;
	bool pad_ctrl_by_lcd;
	bool tx_id_done;
	bool is_suspend;
	int tx_id;
	int tx_id_cnt;
	bool rx_phm_status;
	int rx_phm_state;

	int flicker_delay;
	int flicker_vout_threshold;

	/* wireless tx */
	int tx_status;
	bool initial_wc_check;
	bool wc_tx_enable;
	int wc_rx_type;
	bool wc_rx_connected;
	bool wc_rx_fod;
	bool wc_ldo_status;
	int non_sleep_mode_cnt;
	u8 adt_transfer_status;
	unsigned int current_rx_power;
	u8 tx_pwr_budg;
	u8 device_event;
	int i2c_error_count;
	int gpio_irq_missing_wa_cnt;
	int input_current;
	int duty_min;
	int wpc_en_flag;
	bool tx_device_phm;
	int die_temp;

	bool req_tx_id;
	bool afc_tx_done;
	int req_afc_delay;

	bool sleep_mode;
	bool wc_checking_align;
	struct timespec64 wc_align_check_start;
	int vout_strength;
	u32 mis_align_tx_try_cnt;
	bool skip_phm_work_in_sleep;
	bool reg_access_lock;
	bool check_rx_power;

	int mfc_adc_tx_vout;
	int mfc_adc_tx_iout;
	int mfc_adc_ping_frq;
	int mfc_adc_tx_min_op_frq;
	int mfc_adc_tx_max_op_frq;
	int mfc_adc_vout;
	int mfc_adc_vrect;
	int mfc_adc_rx_iout;
	int mfc_adc_op_frq;
	union mfc_fod_state now_fod_state;
	union mfc_cmfet_state now_cmfet_state;

	int mpp_epp_tx_id;
	int mpp_epp_nego_done_power;
	int mpp_epp_tx_potential_load_power;
	int mpp_epp_tx_negotiable_load_power;
	int mpp_cloak;
	int mpp_estimated_k;
	int magnet_case_det;
	int mpp_case;
	int ping_freq;
	bool mpp_fake_ui_flag;
	int high_swell;
	bool mpp_cloak_status;
	bool init_cover;
	int fod_cnt;

	unsigned int otp_firmware_ver; /* fw version of rx ic */
//#if defined(CONFIG_WIRELESS_IC_PARAM)
	unsigned int wireless_param_info;
	unsigned int wireless_fw_ver_param;
	unsigned int wireless_chip_id_param;
	unsigned int wireless_fw_mode_param;
//#endif
	int epp_time;
	int epp_count;
	int target_current;
	int thermal_ctrl;

	int rx_phm_check_cnt;
	bool is_mpla_thr_recov;

	char d_buf[MFC_BAT_DUMP_SIZE];
};

#define bpp_mode(op_mode) (\
	(op_mode == MFC_RX_MODE_WPC_BPP))

#define mpp_mode(op_mode) (\
	(op_mode == MFC_RX_MODE_WPC_MPP_RESTRICT) || \
	(op_mode == MFC_RX_MODE_WPC_MPP_FULL) || \
	(op_mode == MFC_RX_MODE_WPC_MPP_CLOAK) || \
	(op_mode == MFC_RX_MODE_WPC_MPP_NEGO))

#define epp_mode(op_mode) (\
	(op_mode == MFC_RX_MODE_WPC_EPP) || \
	(op_mode == MFC_RX_MODE_WPC_EPP_NEGO))
#endif /* __WIRELESS_CHARGER_CPS4038_H */
