/*
 * Copyright (C) 2018-2020 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/time.h>
#include <linux/usb/ch9.h>
#include <linux/usb_notify.h>
#include <linux/usb/typec/manager/usb_typec_manager_hwparam.h>
#include <linux/usb/typec/common/pdic_param.h>

struct typec_manager_hwparam {
	struct delayed_work rtctime_update_work;
	int water_detection;
	int water_det_count;
	int water_dry_count;
	int water_Chg_count;
	unsigned long water_det_duration;
	unsigned long water_detected_time;
	unsigned long wVbus_detected;
	unsigned long wVbus_duration;
	unsigned long wVbusHigh_time;

	int usb_highspeed_count;
	int usb_superspeed_count;
};
static struct typec_manager_hwparam manager_hwparam;

static void calc_duration_time(unsigned long sTime, unsigned long eTime, unsigned long *dTime)
{
	unsigned long calcDtime;

	calcDtime = eTime - sTime;

	/* check for exception case. */
	if (calcDtime > 86400)
		calcDtime = 0;

	*dTime += calcDtime;
}

void water_dry_time_update(int mode)
{
	struct timespec64 time;
	static int time_update_check = 1;

	manager_hwparam.water_detection = mode;
	if (mode)
		manager_hwparam.water_det_count++;
	else
		manager_hwparam.water_dry_count++;

	ktime_get_real_ts64(&time);

	if (time_update_check) {
		time_update_check = 0;
		if (time.tv_sec < 31536000) { // 31536000s = 1 year
			schedule_delayed_work(&manager_hwparam.rtctime_update_work, msecs_to_jiffies(5000));
		}
	}

	if (mode) {
		/* WATER */
		manager_hwparam.water_detected_time = time.tv_sec;
	} else {
		/* DRY */
		calc_duration_time(manager_hwparam.water_detected_time,
			time.tv_sec, &manager_hwparam.water_det_duration);
	}
}

static void water_det_rtc_time_update(struct work_struct *work)
{
	struct timespec64 time;
	static int max_retry = 1;

	ktime_get_real_ts64(&time);
	if ((time.tv_sec < 31536000) && (max_retry < 5)) {
		if (manager_hwparam.wVbus_detected) {
			calc_duration_time(manager_hwparam.wVbusHigh_time,
				time.tv_sec, &manager_hwparam.wVbus_duration);
			manager_hwparam.wVbusHigh_time = time.tv_sec;
		}
		max_retry++;
		schedule_delayed_work(&manager_hwparam.rtctime_update_work, msecs_to_jiffies(5000));
	} else {
		if (manager_hwparam.water_detection) {
			manager_hwparam.water_detected_time = time.tv_sec;
			manager_hwparam.water_det_duration += max_retry*5;
			if (manager_hwparam.wVbus_detected) {
				manager_hwparam.wVbusHigh_time = time.tv_sec;
				manager_hwparam.wVbus_duration += 5;
			}
		}
	}
}

void wVbus_time_update(int status)
{
	struct timespec64 time;

	manager_hwparam.wVbus_detected = status;
	ktime_get_real_ts64(&time);

	if (status) {
		/* WVBUS HIGH */
		manager_hwparam.water_Chg_count++;
		manager_hwparam.wVbusHigh_time = time.tv_sec;
	} else {
		/* WVBUS LOW */
		calc_duration_time(manager_hwparam.wVbusHigh_time,
			time.tv_sec, &manager_hwparam.wVbus_duration);
	}
}

static unsigned long get_waterdet_duration(void)
{
	unsigned long ret = 0;
	struct timespec64 time;

	if (manager_hwparam.water_detection) {
		ktime_get_real_ts64(&time);
		calc_duration_time(manager_hwparam.water_detected_time,
			time.tv_sec, &manager_hwparam.water_det_duration);
		manager_hwparam.water_detected_time = time.tv_sec;
	}

	ret = manager_hwparam.water_det_duration/60;  /* min */
	manager_hwparam.water_det_duration -= ret*60;

	return ret;
}

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
static unsigned long get_wvbus_duration(void)
{
	unsigned long ret = 0;
	struct timespec64 time;

	if (manager_hwparam.wVbus_detected) {
		ktime_get_real_ts64(&time);	/* time.tv_sec */
		calc_duration_time(manager_hwparam.wVbusHigh_time,
			time.tv_sec, &manager_hwparam.wVbus_duration);
		manager_hwparam.wVbusHigh_time = time.tv_sec;
	}

	ret = manager_hwparam.wVbus_duration;  /* sec */
	manager_hwparam.wVbus_duration = 0;

	return ret;
}
#endif

void usb_enum_hw_param_data_update(int speed)
{
		if (speed >= USB_SPEED_SUPER)
			manager_hwparam.usb_superspeed_count++;
		else if (speed >= USB_SPEED_HIGH)
			manager_hwparam.usb_highspeed_count++;
}

unsigned long manager_hw_param_update(int param)
{
	unsigned long ret = 0;

	switch (param) {
	case USB_CCIC_WATER_INT_COUNT:
		ret = manager_hwparam.water_det_count;
		manager_hwparam.water_det_count = 0;
		break;
	case USB_CCIC_DRY_INT_COUNT:
		ret = manager_hwparam.water_dry_count;
		manager_hwparam.water_dry_count = 0;
		break;
	case USB_CLIENT_SUPER_SPEED_COUNT:
		ret = manager_hwparam.usb_superspeed_count;
		manager_hwparam.usb_superspeed_count = 0;
		break;
	case USB_CLIENT_HIGH_SPEED_COUNT:
		ret = manager_hwparam.usb_highspeed_count;
		manager_hwparam.usb_highspeed_count = 0;
		break;
	case USB_CCIC_WATER_TIME_DURATION:
		ret = get_waterdet_duration();
		break;
	case USB_CCIC_WATER_VBUS_COUNT:
		if (!is_lpcharge_pdic_param()) {
			ret = manager_hwparam.water_Chg_count;
			manager_hwparam.water_Chg_count = 0;
		}
		break;
	case USB_CCIC_WATER_LPM_VBUS_COUNT:
		if (is_lpcharge_pdic_param()) {
			ret = manager_hwparam.water_Chg_count;
			manager_hwparam.water_Chg_count = 0;
		}
		break;
	case USB_CCIC_WATER_VBUS_TIME_DURATION:
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		if (!is_lpcharge_pdic_param())
			ret = get_wvbus_duration();
#endif
		break;
	case USB_CCIC_WATER_LPM_VBUS_TIME_DURATION:
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		if (is_lpcharge_pdic_param())
			ret = get_wvbus_duration();
#endif
		break;
	default:
		break;
	}

	return ret;
}

void manager_hw_param_init(void)
{
	manager_hwparam.water_det_count = 0;
	manager_hwparam.water_dry_count = 0;
	manager_hwparam.water_det_duration = 0;
	manager_hwparam.water_Chg_count = 0;
	manager_hwparam.wVbus_duration = 0;
	manager_hwparam.usb_highspeed_count = 0;
	manager_hwparam.usb_superspeed_count = 0;

	INIT_DELAYED_WORK(&manager_hwparam.rtctime_update_work,
		water_det_rtc_time_update);
}

MODULE_AUTHOR("Samsung USB Team");
MODULE_DESCRIPTION("USB Typec Manager HWPARAM");
MODULE_LICENSE("GPL");
