/*
 * muic_ccic.c
 *
 * Copyright (C) 2014 Samsung Electronics
 * Thomas Ryu <smilesr.ryu@samsung.com>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/delay.h>
#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/host_notify.h>
#endif
#include <linux/string.h>

#include <linux/mfd/max77775_log.h>

#include <linux/muic/common/muic.h>
#include <linux/usb/typec/maxim/max77775-muic.h>
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/common/muic_notifier.h>
#endif

#if IS_ENABLED(CONFIG_MUIC_SUPPORT_PDIC)
#include <linux/usb/typec/common/pdic_notifier.h>
#endif
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
#include <linux/usb/typec/manager/usb_typec_manager_notifier.h>
#endif

static void max77775_muic_init_ccic_info_data(struct max77775_muic_data *muic_data)
{
	md75_info_usb("%s\n", __func__);
	muic_data->ccic_info_data.ccic_evt_rid = RID_OPEN;
	muic_data->ccic_info_data.ccic_evt_rprd = 0;
	muic_data->ccic_info_data.ccic_evt_roleswap = 0;
	muic_data->ccic_info_data.ccic_evt_dcdcnt = 0;
	muic_data->ccic_info_data.ccic_evt_attached = MUIC_PDIC_NOTI_UNDEFINED;
}

static void max77775_muic_handle_ccic_detach(struct max77775_muic_data *muic_data)
{
	md75_info_usb("%s\n", __func__);
	muic_data->ccic_info_data.ccic_evt_rprd = 0;
	muic_data->ccic_info_data.ccic_evt_roleswap = 0;
	muic_data->ccic_info_data.ccic_evt_dcdcnt = 0;
	muic_data->ccic_info_data.ccic_evt_attached = MUIC_PDIC_NOTI_DETACH;
}

static int max77775_muic_handle_ccic_ATTACH(struct max77775_muic_data *muic_data, PD_NOTI_ATTACH_TYPEDEF *pnoti)
{
	bool need_to_run_work = false;

	md75_info_usb("%s: src:%d dest:%d id:%d attach:%d cable_type:%d rprd:%d\n", __func__,
		pnoti->src, pnoti->dest, pnoti->id, pnoti->attach, pnoti->cable_type, pnoti->rprd);

	/* Attached */
	if (pnoti->attach) {
		md75_info_usb("%s: Attach, cable type=%d\n", __func__, pnoti->cable_type);

		muic_data->ccic_info_data.ccic_evt_attached = MUIC_PDIC_NOTI_ATTACH;

		if (muic_data->ccic_info_data.ccic_evt_roleswap) {
			md75_info_usb("%s: roleswap event, attach USB\n", __func__);
			muic_data->ccic_info_data.ccic_evt_roleswap = 0;
			need_to_run_work = true;
		}

		if (pnoti->rprd) {
			md75_info_usb("%s: RPRD\n", __func__);
			muic_data->ccic_info_data.ccic_evt_rprd = 1;
			need_to_run_work = true;
		}

		/* CCIC ATTACH means NO WATER */
		if (muic_data->afc_water_disable) {
			muic_data->afc_water_disable = false;
			muic_data->ccic_evt_id = PDIC_NOTIFY_ID_WATER;
			need_to_run_work = true;
		}
	} else {
		if (pnoti->rprd) {
			/* Role swap detach: attached=0, rprd=1 */
			md75_info_usb("%s: role swap event\n", __func__);
			muic_data->ccic_info_data.ccic_evt_roleswap = 1;
		} else {
			/* Detached */
			if (muic_data->ccic_info_data.ccic_evt_rprd)
				need_to_run_work = true;
			max77775_muic_handle_ccic_detach(muic_data);
		}
	}

	/* run muic event handler */
	if (need_to_run_work) {
		md75_info_usb("%s: do workqueue\n", __func__);
		schedule_work(&(muic_data->ccic_info_data_work));
	}

	return 0;
}

static int max77775_muic_handle_ccic_RID(struct max77775_muic_data *muic_data, PD_NOTI_RID_TYPEDEF *pnoti)
{
	int prev_rid = muic_data->ccic_info_data.ccic_evt_rid;
	int rid = pnoti->rid;

	md75_info_usb("%s: src:%d dest:%d id:%d rid:%d sub2:%d sub3:%d\n", __func__,
		pnoti->src, pnoti->dest, pnoti->id, pnoti->rid, pnoti->sub2, pnoti->sub3);

	if (rid > RID_OPEN || rid <= RID_UNDEFINED) {
		md75_info_usb("%s: Out of range of RID(%d)\n", __func__, rid);
		return 0;
	}

	muic_data->ccic_info_data.ccic_evt_rid = rid;

	switch (rid) {
	case RID_000K:
	case RID_056K:
	case RID_255K:
	case RID_301K:
	case RID_523K:
	case RID_619K:
	case RID_OPEN:
		if (prev_rid != rid) {
			md75_info_usb("%s: do workqueue\n", __func__);
			schedule_work(&(muic_data->ccic_info_data_work));
		}
		break;
	default:
		md75_err_usb("%s:Not determined now\n", __func__);
		break;
	}

	return 0;
}

static int max77775_muic_handle_ccic_WATER(struct max77775_muic_data *muic_data, PD_NOTI_ATTACH_TYPEDEF *pnoti)
{
	md75_info_usb("%s: src:%d dest:%d id:%d attach:%d cable_type:%d rprd:%d\n", __func__,
		pnoti->src, pnoti->dest, pnoti->id, pnoti->attach, pnoti->cable_type, pnoti->rprd);

	if (pnoti->attach == PDIC_NOTIFY_ATTACH) {
		muic_data->afc_water_disable = true;
		md75_info_usb("%s: Water detect, do workqueue\n", __func__);
		schedule_work(&(muic_data->ccic_info_data_work));
	} else if (pnoti->attach == PDIC_NOTIFY_DETACH) {
		muic_data->afc_water_disable = false;
		muic_data->ccic_evt_id = PDIC_NOTIFY_ID_WATER;
		schedule_work(&(muic_data->ccic_info_data_work));
		md75_info_usb("%s: Dry detect, do workqueue\n", __func__);
	}

	return 0;
}

static void max77775_muic_handle_ccic_usb(struct max77775_muic_data *muic_data,
		PD_NOTI_USB_STATUS_TYPEDEF *pnoti)
{
	if (pnoti->attach == PDIC_NOTIFY_ATTACH) {
		md75_info_usb("%s attach, return\n", __func__);
		return;
	}

	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_TIMEOUT_OPEN_MUIC:
		muic_data->is_usb_fail = true;
#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC) && IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
		if (max77775_muic_check_is_enable_afc(muic_data, muic_data->attached_dev)) {
			md75_info_usb("%s afc work 1200ms\n", __func__);

			__pm_wakeup_event(muic_data->afc_retry_ws, 1500);
			cancel_delayed_work_sync(&(muic_data->afc_work));
			schedule_delayed_work(&(muic_data->afc_work),
					msecs_to_jiffies(1200));
		}
#endif
		break;
	default:
		break;
	}
}

#if defined(CONFIG_HICCUP_CHARGER)
static int max77775_muic_handle_ccic_hiccup(struct max77775_muic_data *muic_data, PD_NOTI_ATTACH_TYPEDEF *pnoti)
{
	md75_info_usb("%s: src:%d dest:%d id:%d attach:%d cable_type:%d rprd:%d\n", __func__,
		pnoti->src, pnoti->dest, pnoti->id, pnoti->attach, pnoti->cable_type, pnoti->rprd);

	if (muic_data->pdata->muic_set_hiccup_mode_cb) {
		if (pnoti->attach == PDIC_NOTIFY_ATTACH)
			muic_data->pdata->muic_set_hiccup_mode_cb(MUIC_HICCUP_MODE_ON);
		else
			muic_data->pdata->muic_set_hiccup_mode_cb(MUIC_HICCUP_MODE_OFF);
	}

	return 0;
}
#endif
static int max77775_muic_handle_ccic_notification(struct notifier_block *nb,
				unsigned long action, void *data)
{
	PD_NOTI_TYPEDEF *pnoti = (PD_NOTI_TYPEDEF *)data;
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	struct max77775_muic_data *muic_data = container_of(nb, struct max77775_muic_data, manager_nb);
#else
	struct max77775_muic_data *muic_data = container_of(nb, struct max77775_muic_data, ccic_nb);
#endif

	md75_info_usb("%s: Rcvd Noti=> action: %d src:%d dest:%d id:%d sub[%d %d %d]\n", __func__,
		(int)action, pnoti->src, pnoti->dest, pnoti->id, pnoti->sub1, pnoti->sub2, pnoti->sub3);

#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	if (pnoti->dest != PDIC_NOTIFY_DEV_MUIC) {
		md75_info_usb("%s destination id is invalid\n", __func__);
		return 0;
	}
#endif
	muic_data->ccic_evt_id = pnoti->id;

	switch (pnoti->id) {
	case PDIC_NOTIFY_ID_ATTACH:
		md75_info_usb("%s: PDIC_NOTIFY_ID_ATTACH: %s\n", __func__,
				pnoti->sub1 ? "Attached" : "Detached");
		max77775_muic_handle_ccic_ATTACH(muic_data, (PD_NOTI_ATTACH_TYPEDEF *)pnoti);
		break;
	case PDIC_NOTIFY_ID_RID:
		md75_info_usb("%s: PDIC_NOTIFY_ID_RID\n", __func__);
		max77775_muic_handle_ccic_RID(muic_data, (PD_NOTI_RID_TYPEDEF *)pnoti);
		break;
	case PDIC_NOTIFY_ID_WATER:
		md75_info_usb("%s: PDIC_NOTIFY_ID_WATER\n", __func__);
		max77775_muic_handle_ccic_WATER(muic_data, (PD_NOTI_ATTACH_TYPEDEF *)pnoti);
		break;
	case PDIC_NOTIFY_ID_WATER_CABLE:
		md75_info_usb("%s: PDIC_NOTIFY_ID_WATER_CABLE\n", __func__);
#if defined(CONFIG_HICCUP_CHARGER)
		max77775_muic_handle_ccic_hiccup(muic_data, (PD_NOTI_ATTACH_TYPEDEF *)pnoti);
#endif
		break;
	case PDIC_NOTIFY_ID_USB:
		md75_info_usb("%s: PDIC_NOTIFY_ID_USB\n", __func__);
		max77775_muic_handle_ccic_usb(muic_data, (PD_NOTI_USB_STATUS_TYPEDEF *)pnoti);
		break;
	default:
		md75_info_usb("%s: Undefined Noti. ID\n", __func__);
		return NOTIFY_DONE;
	}

	return NOTIFY_DONE;
}

void max77775_muic_register_ccic_notifier(struct max77775_muic_data *muic_data)
{
	int ret = 0;

	md75_info_usb("%s: Registering PDIC_NOTIFY_DEV_MUIC.\n", __func__);

	max77775_muic_init_ccic_info_data(muic_data);
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	ret = manager_notifier_register(&muic_data->manager_nb,
		max77775_muic_handle_ccic_notification, MANAGER_NOTIFY_PDIC_MUIC);
#else
	ret = ccic_notifier_register(&muic_data->ccic_nb,
		max77775_muic_handle_ccic_notification, PDIC_NOTIFY_DEV_MUIC);
#endif
	if (ret < 0) {
		md75_info_usb("%s: PDIC Noti. is not ready\n", __func__);
		return;
	}

	md75_info_usb("%s: done.\n", __func__);
}

void max77775_muic_unregister_ccic_notifier(struct max77775_muic_data *muic_data)
{
	int ret = 0;

#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	ret = manager_notifier_unregister(&muic_data->manager_nb);
#else
	ret = ccic_notifier_unregister(&muic_data->ccic_nb);
#endif
	if (ret < 0) {
		md75_info_usb("%s: PDIC Noti. is not ready\n", __func__);
		return;
	}

	md75_info_usb("%s: done.\n", __func__);
}
