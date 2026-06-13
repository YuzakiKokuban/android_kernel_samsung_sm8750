/*
 * sb_batt_dump.c
 * Samsung Mobile Battery Dump Driver
 *
 * Copyright (C) 2021 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/slab.h>

#include <linux/battery/sb_sysfs.h>
#include <linux/battery/sb_notify.h>

#include "sec_battery.h"
#include "sec_charging_common.h"
#include "sb_batt_dump.h"

#define bd_log(str, ...) pr_info("[BATT-DUMP]:%s: "str, __func__, ##__VA_ARGS__)

#define BD_MODULE_NAME	"batt-dump"

struct sb_bd {
	struct notifier_block nb;
};

static ssize_t show_attrs(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t store_attrs(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

#define BD_SYSFS_ATTR(_name)						\
{									\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = show_attrs,					\
	.store = store_attrs,					\
}

static struct device_attribute bd_attr[] = {
	BD_SYSFS_ATTR(battery_dump),
};

enum sb_bd_attrs {
	BATTERY_DUMP = 0,
};

static ssize_t show_attrs(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - bd_attr;
	ssize_t count = 0;
	union power_supply_propval value = {0, };

	switch (offset) {
	case BATTERY_DUMP:
	{
		char temp_buf[1024] = {0,};
		int size = 1024;
		union power_supply_propval dc_state = {0, };
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
		int ret = 0;
#endif

		dc_state.strval = "NO_CHARGING";
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
		ret = psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_CHG_STATUS, dc_state);
		if (ret < 0)
			dc_state.strval = "NO_CHARGING";
#endif

		snprintf(temp_buf + strlen(temp_buf), size,
			"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%s,%s,%s,%d,%s,%d,%d,%lu,0x%x,0x%x,0x%x,%d,%d,%d,%s,",
			battery->voltage_now, battery->current_now,
			battery->current_max, battery->charging_current,
			battery->capacity,
			battery->temperature, battery->usb_temp,
			battery->chg_temp, battery->wpc_temp,
			battery->blkt_temp, battery->lrp,
			battery->dchg_temp, battery->sub_bat_temp,
			sb_get_bst_str(battery->status),
			dc_state.strval,
			sb_get_cm_str(battery->charging_mode),
			sb_get_hl_str(battery->health),
			sb_get_ct_str(battery->cable_type),
			battery->muic_cable_type,
			sb_get_tz_str(battery->thermal_zone),
			is_slate_mode(battery),
			battery->store_mode,
			(battery->safety_timer.remaining_time / 1000),
			battery->current_event,
			battery->misc_event,
			battery->tx_event,
#if defined(CONFIG_WIRELESS_RX_PHM_CTRL)
			battery->wc_rx_pdetb_mode,
#else
			battery->wc_rx_phm_mode,
#endif

			battery->srccap_transit,
			battery->current_sys_avg,
			"v0002"
		);
		size = sizeof(temp_buf) - strlen(temp_buf);

	{
		unsigned short vid = 0, pid = 0;
		unsigned int xid = 0;

		sec_pd_get_vid_pid(&vid, &pid, &xid);
		snprintf(temp_buf+strlen(temp_buf), size,
			"%04x,%04x,%08x,", vid, pid, xid);
		size = sizeof(temp_buf) - strlen(temp_buf);
	}

		snprintf(temp_buf+strlen(temp_buf), size,
			"%d,%d,%d,%d,",
			battery->voltage_now_main, battery->voltage_now_sub,
			battery->current_now_main, battery->current_now_sub);
		size = sizeof(temp_buf) - strlen(temp_buf);

		snprintf(temp_buf+strlen(temp_buf), size, "%d,", battery->batt_cycle);
		size = sizeof(temp_buf) - strlen(temp_buf);

		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_BATT_DUMP, value);
		snprintf(temp_buf+strlen(temp_buf), size, "%s,", value.strval);
		size = sizeof(temp_buf) - strlen(temp_buf);

		/* Wireless charging related log added at the end of the string */
		value.intval = SB_WRL_NONE;
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
		if (battery->wc_tx_enable) {
			value.intval = SB_WRL_TX_MODE;
			snprintf(temp_buf+strlen(temp_buf), size, "%d,", SB_WRL_TX_MODE);
		} else if (is_wireless_all_type(battery->cable_type)) {
			value.intval = SB_WRL_RX_MODE;
			snprintf(temp_buf+strlen(temp_buf), size, "%d,", SB_WRL_RX_MODE);
		} else
			goto skip_wc;

		size = sizeof(temp_buf) - strlen(temp_buf);
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_BATT_DUMP, value);

		snprintf(temp_buf+strlen(temp_buf), size, "%s", value.strval);
		size = sizeof(temp_buf) - strlen(temp_buf);
skip_wc:
#endif
		if (value.intval == SB_WRL_NONE) {
			snprintf(temp_buf+strlen(temp_buf), size, "%d,", 0);
			size = sizeof(temp_buf) - strlen(temp_buf);
		}

		count += scnprintf(buf + count, PAGE_SIZE - count, "%s\n", temp_buf);
	}
		break;
	default:
		break;
	}

	return count;
}

static ssize_t store_attrs(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	const ptrdiff_t offset = attr - bd_attr;

	switch (offset) {
	case BATTERY_DUMP:
		break;
	default:
		break;
	}

	return count;
}

static int sb_noti_handler(struct notifier_block *nb, unsigned long action, void *data)
{
	return 0;
}

int sb_bd_init(void)
{
	struct sb_bd *bd;
	int ret = 0;

	bd = kzalloc(sizeof(struct sb_bd), GFP_KERNEL);
	if (!bd)
		return -ENOMEM;

	ret = sb_sysfs_add_attrs(BD_MODULE_NAME, bd, bd_attr, ARRAY_SIZE(bd_attr));
	bd_log("sb_sysfs_add_attrs ret = %s\n", (ret) ? "fail" : "success");

	ret = sb_notify_register(&bd->nb, sb_noti_handler, BD_MODULE_NAME, SB_DEV_MODULE);
	bd_log("sb_notify_register ret = %s\n", (ret) ? "fail" : "success");

	return ret;
}
EXPORT_SYMBOL(sb_bd_init);
