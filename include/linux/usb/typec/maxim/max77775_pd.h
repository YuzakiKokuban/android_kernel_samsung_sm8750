/*
 * Copyrights (C) 2017 Samsung Electronics, Inc.
 * Copyrights (C) 2017 Maxim Integrated Products, Inc.
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
 */

#ifndef __LINUX_MFD_MAX77775_PD_H
#define __LINUX_MFD_MAX77775_PD_H
#include "max77775.h"
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
#include <linux/battery/sec_pd.h>
#endif

#define MAX77775_PD_NAME	"MAX77775_PD"

enum {
	CC_SNK = 0,
	CC_SRC,
	CC_NO_CONN,
};

enum {
	D2D_NONE	= 0,
	D2D_SNKONLY,
	D2D_SRCSNK,
};

typedef enum {
	PDO_TYPE_FIXED = 0,
	PDO_TYPE_BATTERY,
	PDO_TYPE_VARIABLE,
	PDO_TYPE_APDO
} pdo_supply_type_t;

typedef union sec_pdo_object {
	uint32_t		data;
	struct {
		uint8_t		bdata[4];
	} BYTES;
	struct {
		uint32_t	reserved:30,
					type:2;
	} BITS_supply;
	struct {
		uint32_t	max_current:10,	/* 10mA units */
					voltage:10,		/* 50mV units */
					peak_current:2,
					reserved:2,
					unchuncked_extended_messages_supported:1,
					data_role_data:1,
					usb_communications_capable:1,
					unconstrained_power:1,
					usb_suspend_supported:1,
					dual_role_power:1,
					supply:2;		/* Fixed supply : 00b */
	} BITS_pdo_fixed;
	struct {
		uint32_t	max_current:10,		/* 10mA units */
				min_voltage:10,		/* 50mV units */
				max_voltage:10,		/* 50mV units */
				supply:2;		/* Variable Supply (non-Battery) : 10b */
	} BITS_pdo_variable;
	struct {
		uint32_t	max_allowable_power:10,		/* 250mW units */
				min_voltage:10,		/* 50mV units  */
				max_voltage:10,		/* 50mV units  */
				supply:2;		/* Battery : 01b */
	} BITS_pdo_battery;
	struct {
		uint32_t	max_current:7, 	/* 50mA units */
				reserved1:1,
				min_voltage:8, 	/* 100mV units	*/
				reserved2:1,
				max_voltage:8, 	/* 100mV units	*/
				reserved3:2,
				pps_power_limited:1,
				pps_supply:2,
				supply:2;		/* APDO : 11b */
	} BITS_pdo_programmable;
} U_SEC_PDO_OBJECT;

struct max77775_pd_data {
	/* interrupt pin */
	int irq_pdmsg;
	int irq_psrdy;
	int irq_datarole;
	int irq_ssacc;
	int irq_fct_id;

	u8 usbc_status1;
	u8 usbc_status2;
	u8 bc_status;
	u8 cc_status1;
	u8 cc_status2;
	u8 pd_status1;
	u8 pd_status2;

	u8 opcode_res;

	/* PD Message */
	u8 pdsmg;

	/* Data Role */
	enum max77775_data_role current_dr;
	enum max77775_data_role previous_dr;
	/* SSacc */
	u8 ssacc;
	/* FCT cable */
	u8 fct_id;
	enum max77775_ccpd_device device;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	struct pdic_notifier_struct pd_noti;
#endif
	bool pdo_list;
	bool psrdy_received;
	bool cc_sbu_short;
	bool bPPS_on;
	bool sent_chg_info;

	struct workqueue_struct *wqueue;
	struct delayed_work retry_work;
	struct delayed_work d2d_work;
	struct delayed_work abnormal_pdo_work;

	int cc_status;

	int src_cap_done;
	int auth_type;
	int d2d_type;
	int req_pdo_type;
	bool psrdy_sent;
};

#endif
