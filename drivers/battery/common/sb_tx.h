/*
 * sb_tx.h
 * Samsung Mobile Wireless TX Header
 *
 * Copyright (C) 2021 Samsung Electronics, Inc.
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

#ifndef __SB_TX_H
#define __SB_TX_H __FILE__

#define TX_MODULE_NAME	"sb-tx"

/* tx_event */
#define SB_TX_EVENT_TX_STATUS		0x00000001
#define SB_TX_EVENT_RX_CONNECT		0x00000002
#define SB_TX_EVENT_TX_FOD			0x00000004
#define SB_TX_EVENT_TX_HIGH_TEMP	0x00000008
#define SB_TX_EVENT_RX_UNSAFE_TEMP	0x00000010
#define SB_TX_EVENT_RX_CHG_SWITCH	0x00000020
#define SB_TX_EVENT_RX_CS100		0x00000040
#define SB_TX_EVENT_TX_OTG_ON		0x00000080
#define SB_TX_EVENT_TX_LOW_TEMP		0x00000100
#define SB_TX_EVENT_TX_SOC_DRAIN	0x00000200
#define SB_TX_EVENT_TX_CRITICAL_EOC	0x00000400
#define SB_TX_EVENT_TX_CAMERA_ON	0x00000800
#define SB_TX_EVENT_TX_OCP			0x00001000
#define SB_TX_EVENT_TX_MISALIGN		0x00002000
#define SB_TX_EVENT_TX_ETC			0x00004000
#define SB_TX_EVENT_TX_RETRY		0x00008000
#define SB_TX_EVENT_TX_5V_TA		0x00010000
#define SB_TX_EVENT_TX_AC_MISSING	0x00020000
#define SB_TX_EVENT_ALL_MASK		0x0003ffff
#define SB_TX_EVENT_TX_ERR			(SB_TX_EVENT_TX_FOD | \
	SB_TX_EVENT_TX_HIGH_TEMP | SB_TX_EVENT_RX_UNSAFE_TEMP | \
	SB_TX_EVENT_RX_CHG_SWITCH | SB_TX_EVENT_RX_CS100 | \
	SB_TX_EVENT_TX_OTG_ON | SB_TX_EVENT_TX_LOW_TEMP | \
	SB_TX_EVENT_TX_SOC_DRAIN | SB_TX_EVENT_TX_CRITICAL_EOC | \
	SB_TX_EVENT_TX_CAMERA_ON | SB_TX_EVENT_TX_OCP | \
	SB_TX_EVENT_TX_MISALIGN | SB_TX_EVENT_TX_ETC | \
	SB_TX_EVENT_TX_5V_TA | SB_TX_EVENT_TX_AC_MISSING)

#define SB_TX_RETRY_NONE		0x0000
#define SB_TX_RETRY_MISALIGN	0x0001
#define SB_TX_RETRY_CAMERA		0x0002
#define SB_TX_RETRY_CALL		0x0004
#define SB_TX_RETRY_MIX_TEMP	0x0008
#define SB_TX_RETRY_HIGH_TEMP	0x0010
#define SB_TX_RETRY_LOW_TEMP	0x0020
#define SB_TX_RETRY_OCP			0x0040

enum power_supply_property;
union power_supply_propval;

#define SB_TX_DISABLE	(-2222)
#if defined(CONFIG_WIRELESS_TX_MODE)
int sb_tx_init(struct sec_battery_info *battery, char *wrl_name);

int sb_tx_set_enable(bool tx_enable, int cable_type);
bool sb_tx_get_enable(void);

int sb_tx_set_event(int value, int mask);

/* for set/get properties - called in wireless set/get property */
int sb_tx_psy_set_property(enum power_supply_property psp, const union power_supply_propval *value);
int sb_tx_psy_get_property(enum power_supply_property psp, union power_supply_propval *value);

/* for monitor tx state - called in battery drv */
int sb_tx_monitor(int cable_type, int capacity, int lcd_state);

/* temporary function */
int sb_tx_init_aov(void);
bool sb_tx_is_aov_enabled(int cable_type);
int sb_tx_monitor_aov(int vout, bool phm);
#else
static inline int sb_tx_init(struct sec_battery_info *battery, char *wrl_name) { return SB_TX_DISABLE; }

static inline int sb_tx_set_enable(bool tx_enable, int cable_type) { return SB_TX_DISABLE; }
static inline bool sb_tx_get_enable(void) { return false; }

static inline int sb_tx_set_event(int value, int mask) { return SB_TX_DISABLE; }

/* for set/get properties - called in wireless set/get property */
static inline int sb_tx_psy_set_property(enum power_supply_property psp, const union power_supply_propval *value)
{ return SB_TX_DISABLE; }
static inline int sb_tx_psy_get_property(enum power_supply_property psp, union power_supply_propval *value)
{ return SB_TX_DISABLE; }

/* for monitor tx state - called in battery drv */
static inline int sb_tx_monitor(int cable_type, int capacity, int lcd_state) { return SB_TX_DISABLE; }


/* temporary function */
static inline int sb_tx_init_aov(void) { return SB_TX_DISABLE; }
static inline bool sb_tx_is_aov_enabled(int cable_type) { return false; }
static inline int sb_tx_monitor_aov(int vout, bool phm) { return SB_TX_DISABLE; }
#endif

#endif /* __SB_TX_H */

