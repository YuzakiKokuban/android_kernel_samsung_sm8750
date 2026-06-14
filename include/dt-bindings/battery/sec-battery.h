
#ifndef _DT_BINDINGS_BATTERY_SEC_BATTERY_H
#define _DT_BINDINGS_BATTERY_SEC_BATTERY_H

#define SEC_BATTERY_CABLE_UNKNOWN 0
#define SEC_BATTERY_CABLE_NONE                  1
#define SEC_BATTERY_CABLE_PREPARE_TA             2
#define SEC_BATTERY_CABLE_TA                     3
#define SEC_BATTERY_CABLE_USB                    4
#define SEC_BATTERY_CABLE_USB_CDP                5
#define SEC_BATTERY_CABLE_9V_TA                  6
#define SEC_BATTERY_CABLE_9V_ERR                 7
#define SEC_BATTERY_CABLE_9V_UNKNOWN             8
#define SEC_BATTERY_CABLE_12V_TA                 9
#define SEC_BATTERY_CABLE_WIRELESS               10
#define SEC_BATTERY_CABLE_HV_WIRELESS            11
#define SEC_BATTERY_CABLE_PMA_WIRELESS           12
#define SEC_BATTERY_CABLE_WIRELESS_PACK          13
#define SEC_BATTERY_CABLE_WIRELESS_HV_PACK       14
#define SEC_BATTERY_CABLE_WIRELESS_STAND         15
#define SEC_BATTERY_CABLE_WIRELESS_HV_STAND      16
#define SEC_BATTERY_CABLE_QC20                   17
#define SEC_BATTERY_CABLE_QC30                   18
#define SEC_BATTERY_CABLE_PDIC                   19
#define SEC_BATTERY_CABLE_UARTOFF                20
#define SEC_BATTERY_CABLE_OTG                    21
#define SEC_BATTERY_CABLE_LAN_HUB                22
#define SEC_BATTERY_CABLE_POWER_SHARING          23
#define SEC_BATTERY_CABLE_HMT_CONNECTED          24
#define SEC_BATTERY_CABLE_HMT_CHARGE             25
#define SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT        26
#define SEC_BATTERY_CABLE_WIRELESS_VEHICLE       27
#define SEC_BATTERY_CABLE_WIRELESS_HV_VEHICLE    28
#define SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV    29
#define SEC_BATTERY_CABLE_TIMEOUT                30
#define SEC_BATTERY_CABLE_SMART_OTG              31
#define SEC_BATTERY_CABLE_SMART_NOTG             32
#define SEC_BATTERY_CABLE_WIRELESS_TX            33
#define SEC_BATTERY_CABLE_HV_WIRELESS_20         34
#define SEC_BATTERY_CABLE_HV_WIRELESS_20_LIMIT   35
#define SEC_BATTERY_CABLE_WIRELESS_FAKE 	 36
#define SEC_BATTERY_CABLE_PREPARE_WIRELESS_20    37
#define SEC_BATTERY_CABLE_PDIC_APDO              38
#define SEC_BATTERY_CABLE_POGO                   39
#define SEC_BATTERY_CABLE_POGO_9V		 40
#define SEC_BATTERY_CABLE_FPDO_DC		 41
#define SEC_BATTERY_CABLE_WIRELESS_EPP		 42
#define SEC_BATTERY_CABLE_LO_TA			43
#define SEC_BATTERY_CABLE_WIRELESS_EPP_NV	 44
#define SEC_BATTERY_CABLE_WIRELESS_EPP_FAKE	 45
#define SEC_BATTERY_CABLE_HV_WIRELESS_DC		46
#define SEC_BATTERY_CABLE_WIRELESS_MPP		 47
#define SEC_BATTERY_CABLE_WIRELESS_MPP_FAKE		 48
#define SEC_BATTERY_CABLE_MAX			49

/* d2d support type */
#define SB_D2D_NONE	0
#define SB_D2D_SNKONLY	1
#define SB_D2D_SRCSNK	2

/* temperature check type */
#define SEC_BATTERY_TEMP_CHECK_NONE 0        /* no temperature check */
#define SEC_BATTERY_TEMP_CHECK_ADC  1             /* by ADC value */
#define SEC_BATTERY_TEMP_CHECK_TEMP 2            /* by temperature */
#define SEC_BATTERY_TEMP_CHECK_FAKE 3            /* by a fake temperature */

/* ADC type */
	/* NOT using this ADC channel */
#define SEC_BATTERY_ADC_TYPE_NONE	0
	/* ADC in AP */
#define SEC_BATTERY_ADC_TYPE_AP		1
	 /* ADC by additional IC */
#define SEC_BATTERY_ADC_TYPE_IC		2
#define SEC_BATTERY_ADC_TYPE_NUM	3

/* ADC read type */
#define SEC_BATTERY_ADC_PROCESSED 0
#define SEC_BATTERY_ADC_RAW 1

/* thermal source */
/* none */
#define SEC_BATTERY_THERMAL_SOURCE_NONE		0
/* by external source */
#define SEC_BATTERY_THERMAL_SOURCE_CALLBACK	1
/* by ADC */
#define SEC_BATTERY_THERMAL_SOURCE_ADC		2
/* by charger */
#define SEC_BATTERY_THERMAL_SOURCE_CHG_ADC	3
/* by fuel gauge */
#define SEC_BATTERY_THERMAL_SOURCE_FG		4
/* by fuel gauge adc */
#define SEC_BATTERY_THERMAL_SOURCE_FG_ADC		5

#define SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE             1
/* SEC_BATTERY_CABLE_CHECK_NOINCOMPATIBLECHARGE
 * for incompatible charger
 * (Not compliant to USB specification,
 *  cable type is SEC_BATTERY_CABLE_UNKNOWN),
 * do NOT charge and show message to user
 * (only for VZW)
 */
#define SEC_BATTERY_CABLE_CHECK_NOINCOMPATIBLECHARGE    2
/* SEC_BATTERY_CABLE_CHECK_PSY
 * check cable by power supply set_property
 */
#define SEC_BATTERY_CABLE_CHECK_PSY                     4
/* SEC_BATTERY_CABLE_CHECK_INT
 * check cable by interrupt
 */
#define SEC_BATTERY_CABLE_CHECK_INT                     8
/* SEC_BATTERY_CABLE_CHECK_CHGINT
 * check cable by charger interrupt
 */
#define SEC_BATTERY_CABLE_CHECK_CHGINT                  16
/* SEC_BATTERY_CABLE_CHECK_POLLING
 * check cable by GPIO polling
 */
#define SEC_BATTERY_CABLE_CHECK_POLLING                 32


/* SEC_BATTERY_CABLE_SOURCE_EXTERNAL
 * already given by external argument
 */
#define SEC_BATTERY_CABLE_SOURCE_EXTERNAL       1
/* SEC_BATTERY_CABLE_SOURCE_CALLBACK
 * by callback (MUIC, USB switch)
 */
#define SEC_BATTERY_CABLE_SOURCE_CALLBACK       2
/* SEC_BATTERY_CABLE_SOURCE_ADC
 * by ADC
 */
#define SEC_BATTERY_CABLE_SOURCE_ADC            4


	/* polling work queue */
#define SEC_BATTERY_MONITOR_WORKQUEUE	0
	/* alarm polling */
#define SEC_BATTERY_MONITOR_ALARM	1
	/* timer polling (NOT USE) */
#define SEC_BATTERY_MONITOR_TIMER	2

/* OVP, UVLO check : POWER_SUPPLY_PROP_HEALTH */

	/* by callback function */
#define SEC_BATTERY_OVP_UVLO_CALLBACK		0
	/* by PMIC polling */
#define SEC_BATTERY_OVP_UVLO_PMICPOLLING	1
	/* by PMIC interrupt */
#define SEC_BATTERY_OVP_UVLO_PMICINT		2
	/* by charger polling */
#define SEC_BATTERY_OVP_UVLO_CHGPOLLING		3
	/* by charger interrupt */
#define SEC_BATTERY_OVP_UVLO_CHGINT		4


/* full charged check : POWER_SUPPLY_PROP_STATUS */

#define SEC_BATTERY_FULLCHARGED_NONE		0
	/* current check by ADC */
#define SEC_BATTERY_FULLCHARGED_ADC		1
	/* fuel gauge current check */
#define SEC_BATTERY_FULLCHARGED_FG_CURRENT	2
	/* time check */
#define SEC_BATTERY_FULLCHARGED_TIME		3
	/* SOC check */
#define SEC_BATTERY_FULLCHARGED_SOC		4
	/* charger GPIO, NO additional full condition */
#define SEC_BATTERY_FULLCHARGED_CHGGPIO		5
	/* charger interrupt, NO additional full condition */
#define SEC_BATTERY_FULLCHARGED_CHGINT		6
	/* charger power supply property, NO additional full condition */
#define SEC_BATTERY_FULLCHARGED_CHGPSY		7
	/* Limiter power supply property, NO additional full condition */
#define SEC_BATTERY_FULLCHARGED_LIMITER		8

#define	TEMP_CONTROL_SOURCE_NONE	0
#define	TEMP_CONTROL_SOURCE_BAT_THM	1
#define	TEMP_CONTROL_SOURCE_CHG_THM	2
#define	TEMP_CONTROL_SOURCE_WPC_THM	3
#define	TEMP_CONTROL_SOURCE_USB_THM	4

/* SEC_BATTERY_FULL_CONDITION_NOTIMEFULL
 * full-charged by absolute-timer only in high voltage
 */
#define SEC_BATTERY_FULL_CONDITION_NOTIMEFULL   1
/* SEC_BATTERY_FULL_CONDITION_NOSLEEPINFULL
 * do not set polling time as sleep polling time in full-charged
 */
#define SEC_BATTERY_FULL_CONDITION_NOSLEEPINFULL        2
/* SEC_BATTERY_FULL_CONDITION_SOC
 * use capacity for full-charged check
 */
#define SEC_BATTERY_FULL_CONDITION_SOC          4
/* SEC_BATTERY_FULL_CONDITION_VCELL
 * use VCELL for full-charged check
 */
#define SEC_BATTERY_FULL_CONDITION_VCELL        8
/* SEC_BATTERY_FULL_CONDITION_AVGVCELL
 * use average VCELL for full-charged check
 */
#define SEC_BATTERY_FULL_CONDITION_AVGVCELL     16
/* SEC_BATTERY_FULL_CONDITION_OCV
 * use OCV for full-charged check
 */
#define SEC_BATTERY_FULL_CONDITION_OCV          32

/* recharge check condition type (can be used overlapped) */
#define sec_battery_recharge_condition_t unsigned int
/* SEC_BATTERY_RECHARGE_CONDITION_SOC
 * use capacity for recharging check
 */
#define SEC_BATTERY_RECHARGE_CONDITION_SOC              1
/* SEC_BATTERY_RECHARGE_CONDITION_AVGVCELL
 * use average VCELL for recharging check
 */
#define SEC_BATTERY_RECHARGE_CONDITION_AVGVCELL         2
/* SEC_BATTERY_RECHARGE_CONDITION_VCELL
 * use VCELL for recharging check
 */
#define SEC_BATTERY_RECHARGE_CONDITION_VCELL            4

/* SEC_BATTERY_RECHARGE_CONDITION_LIMITER
 * use VCELL of LIMITER for recharging check
 */
#define SEC_BATTERY_RECHARGE_CONDITION_LIMITER          8

/* SEC_BATTERY_RECHARGE_CONDITION_DUAL_BATTERY_INDIVIDUAL_FV
 * use VCELL of LIMITER for dual battery individual fv recharging check
 */
#define SEC_BATTERY_RECHARGE_CONDITION_DUAL_BATTERY_INDIVIDUAL_FV	16

#define SIOP_DEFAULT	0xFFFF
#define SIOP_SKIP		0xFFFE

/* inbat ocv type */
#define SEC_BATTERY_OCV_NONE			0
#define SEC_BATTERY_OCV_FG_SRC_CHANGE	1
#define SEC_BATTERY_OCV_FG_NOSRC_CHANGE	2
#define SEC_BATTERY_OCV_ADC				3
#define SEC_BATTERY_OCV_VOLT_FROM_PMIC	4


/* enum sec_wireless_rx_power_list */
#define SEC_WIRELESS_RX_POWER_3W		3000000
#define SEC_WIRELESS_RX_POWER_5W		5000000
#define SEC_WIRELESS_RX_POWER_6_5W		6500000
#define SEC_WIRELESS_RX_POWER_7_5W		7500000
#define SEC_WIRELESS_RX_POWER_10W		10000000
#define SEC_WIRELESS_RX_POWER_11W		11000000
#define SEC_WIRELESS_RX_POWER_12W		12000000
#define SEC_WIRELESS_RX_POWER_15W		15000000
#define SEC_WIRELESS_RX_POWER_17_5W		17500000
#define SEC_WIRELESS_RX_POWER_20W		20000000
#define SEC_WIRELESS_RX_POWER_MAX		20000000

/* enum sec_wireless_rx_power_class_list */
#define SEC_WIRELESS_RX_POWER_CLASS_1	1	/* 4.5W ~ 7.5W */
#define SEC_WIRELESS_RX_POWER_CLASS_2	2	/* 7.6W ~ 12W */
#define SEC_WIRELESS_RX_POWER_CLASS_3	3	/* 12.1W ~ 20W */
#define SEC_WIRELESS_RX_POWER_CLASS_4	4	/* reserved */
#define SEC_WIRELESS_RX_POWER_CLASS_5	5	/* reserved */

#define SEC_WIRELESS_PHM_VOUT_CTRL_NO_DEV		0
#define SEC_WIRELESS_PHM_VOUT_CTRL_OTHER_DEV	1
#define SEC_WIRELESS_PHM_VOUT_CTRL_GEAR			2
#define SEC_WIRELESS_PHM_VOUT_CTRL_PHONE		4
#define SEC_WIRELESS_PHM_VOUT_CTRL_BUDS			8

/* enum sec_wireless_rx_control_mode */
#define WIRELESS_PAD_FAN_OFF		0
#define WIRELESS_PAD_FAN_ON		1
#define WIRELESS_PAD_LED_OFF		2
#define WIRELESS_PAD_LED_ON		3
#define WIRELESS_PAD_LED_DIMMING	4
#define WIRELESS_VRECT_ADJ_ON		5
#define WIRELESS_VRECT_ADJ_OFF		6
#define WIRELESS_VRECT_ADJ_ROOM_0	7
#define WIRELESS_VRECT_ADJ_ROOM_1	8
#define WIRELESS_VRECT_ADJ_ROOM_2	9
#define WIRELESS_VRECT_ADJ_ROOM_3	10
#define WIRELESS_VRECT_ADJ_ROOM_4	11
#define WIRELESS_VRECT_ADJ_ROOM_5	12
#define WIRELESS_CLAMP_ENABLE		13
#define WIRELESS_SLEEP_MODE_ENABLE	14
#define WIRELESS_SLEEP_MODE_DISABLE	15

/* enum sec_wireless_tx_vout */
#define WC_TX_VOUT_OFF		0
#define WC_TX_VOUT_5000MV	5000
#define WC_TX_VOUT_5500MV	5500
#define WC_TX_VOUT_6000MV	6000
#define WC_TX_VOUT_6500MV	6500
#define WC_TX_VOUT_7000MV	7000
#define WC_TX_VOUT_7500MV	7500
#define WC_TX_VOUT_8000MV	8000
#define WC_TX_VOUT_8500MV	8500
#define WC_TX_VOUT_9000MV	9000
#define WC_TX_VOUT_MIN		WC_TX_VOUT_5000MV
#define WC_TX_VOUT_MAX		WC_TX_VOUT_9000MV
#define WC_TX_VOUT_STEP_AOV	500

/* enum sec_wireless_vout_control_mode */
#define WIRELESS_VOUT_OFF		0
#define WIRELESS_VOUT_NORMAL_VOLTAGE	1 /* 5V , reserved by factory */
#define WIRELESS_VOUT_RESERVED		2 /* 6V */
#define WIRELESS_VOUT_HIGH_VOLTAGE	3 /* 9V , reserved by factory */
#define WIRELESS_VOUT_CC_CV_VOUT	4
#define WIRELESS_VOUT_CALL		5
#define WIRELESS_VOUT_5V		6
#define WIRELESS_VOUT_9V		7
#define WIRELESS_VOUT_10V		8
#define WIRELESS_VOUT_11V		9
#define WIRELESS_VOUT_12V		10
#define WIRELESS_VOUT_12_5V		11
#define WIRELESS_VOUT_4_5V_STEP		12
#define WIRELESS_VOUT_5V_STEP		13
#define WIRELESS_VOUT_5_5V_STEP		14
#define WIRELESS_VOUT_9V_STEP		15
#define WIRELESS_VOUT_10V_STEP		16
#define WIRELESS_VOUT_OTG			17
#define WIRELESS_VOUT_FORCE_9V	18
#define WIRELESS_VOUT_5_5V		19
#define WIRELESS_VOUT_4_5V		20
#define WIRELESS_VOUT_FORCE_4_7V		21
#define WIRELESS_VOUT_FORCE_4_8V		22
#define WIRELESS_VOUT_FORCE_4_9V		23
#define WIRELESS_VOUT_FORCE_5V	24
#define WIRELESS_VOUT_13V		25
#define WIRELESS_VOUT_FORCE_10V	26
#define WIRELESS_VOUT_FORCE_13V	27

/* enum mfc_send_command */
#define MFC_END_SIG_STRENGTH		0
#define MFC_END_POWER_TRANSFER		1
#define MFC_END_CTR_ERROR		2
#define MFC_END_RECEIVED_POWER		3
#define MFC_END_CHARGE_STATUS		4
#define MFC_POWER_CTR_HOLD_OFF		5
#define MFC_AFC_CONF_5V			6
#define MFC_AFC_CONF_10V		7
#define MFC_AFC_CONF_5V_TX		8
#define MFC_AFC_CONF_10V_TX		9
#define MFC_AFC_CONF_12V_TX		10
#define MFC_AFC_CONF_12_5V_TX		11
#define MFC_AFC_CONF_20V_TX		12
#define MFC_CONFIGURATION		13
#define MFC_IDENTIFICATION		14
#define MFC_EXTENDED_IDENT		15
#define MFC_LED_CONTROL_ON		16
#define MFC_LED_CONTROL_OFF		17
#define MFC_FAN_CONTROL_ON		18
#define MFC_FAN_CONTROL_OFF		19
#define MFC_REQUEST_AFC_TX		20
#define MFC_REQUEST_TX_ID		21
#define MFC_DISABLE_TX			22
#define MFC_PHM_ON			23
#define MFC_LED_CONTROL_DIMMING		24
#define MFC_SET_OP_FREQ			25
#define MFC_TX_UNO_OFF			26
#define MFC_REQ_TX_PWR_BUDG	27

#define MFC_VOUT_4_5V	0
#define MFC_VOUT_4_7V	1
#define MFC_VOUT_4_8V	2
#define MFC_VOUT_4_9V	3
#define MFC_VOUT_5V	4
#define MFC_VOUT_5_5V	5
#define MFC_VOUT_6V	6
#define MFC_VOUT_7V	7
#define MFC_VOUT_8V	8
#define MFC_VOUT_9V	9
#define MFC_VOUT_10V	10
#define MFC_VOUT_11V	11
#define MFC_VOUT_12V	12
#define MFC_VOUT_12_5V	13
#define MFC_VOUT_OTG	14
#define MFC_VOUT_13V	15

/* fod macro */
#define FOD_FLAG_NONE			0
#define FOD_FLAG_ADD			1
#define FOD_FLAG_USE_CC			2
#define FOD_FLAG_USE_CV			3
#define FOD_FLAG_USE_FULL		4
#define FOD_FLAG_USE_DEF_PAD	5
#define FOD_FLAG_USE_DEF_OP		6
#define SET_FOD_CC(_flag)		(FOD_FLAG_ ##_flag)
#define SET_FOD_CV(_flag)		(FOD_FLAG_ ##_flag << 4)
#define SET_FOD_FULL(_flag)		(FOD_FLAG_ ##_flag << 8)

#define	DC_MODE_NONE	0
#define	DC_MODE_1TO1	1 /* Unused */
#define	DC_MODE_2TO1	2
#define	DC_MODE_3TO1	3
#define	DC_MODE_4TO1	4
#define	DC_MODE_MAX	10

#endif /* _DT_BINDINGS_BATTERY_SEC_BATTERY_H */
