/*
 * Copyrights (C) 2016-2019 Samsung Electronics, Inc.
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

#include <linux/device.h>
#include <linux/module.h>
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
#include <linux/usb_notify.h>
#endif
#if IS_ENABLED(CONFIG_SEC_PD)
#include <linux/battery/sec_pd.h>
#elif defined(CONFIG_BATTERY_NOTIFIER)
#include <linux/battery/battery_notifier.h>
#endif
#include <linux/usb/typec/common/pdic_core.h>
#include <linux/usb/typec/common/pdic_sysfs.h>
#include <linux/usb/typec/common/pdic_notifier.h>
#define DRIVER_DESC   "PDIC Notifier driver"

#define SET_PDIC_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_PDIC_NOTIFIER_BLOCK(nb)			\
		SET_PDIC_NOTIFIER_BLOCK(nb, NULL, -1)

static struct pdic_notifier_data pdic_notifier;

static int pdic_notifier_init_done;

const char *pdic_event_src_string(pdic_notifier_device src)
{
	/* enum pdic_notifier_device */
	switch (src) {
	case PDIC_NOTIFY_DEV_INITIAL:
		return "INITIAL";
	case PDIC_NOTIFY_DEV_USB:
		return "USB";
	case PDIC_NOTIFY_DEV_BATT:
		return "BATTERY";
	case PDIC_NOTIFY_DEV_PDIC:
		return "PDIC";
	case PDIC_NOTIFY_DEV_MUIC:
		return "MUIC";
	case PDIC_NOTIFY_DEV_CCIC:
		return "CCIC";
	case PDIC_NOTIFY_DEV_MANAGER:
		return "MANAGER";
	case PDIC_NOTIFY_DEV_DP:
		return "DP";
	case PDIC_NOTIFY_DEV_USB_DP:
		return "USBDP";
	case PDIC_NOTIFY_DEV_SUB_BATTERY:
		return "BATTERY2";
	case PDIC_NOTIFY_DEV_SECOND_MUIC:
		return "MUIC2";
	case PDIC_NOTIFY_DEV_DEDICATED_MUIC:
		return "DMUIC";
	case PDIC_NOTIFY_DEV_ALL:
		return "ALL";
	default:
		return "UNDEFINED";
	}
}
EXPORT_SYMBOL(pdic_event_src_string);

const char *pdic_event_dest_string(pdic_notifier_device dest)
{
	return pdic_event_src_string(dest);
}
EXPORT_SYMBOL(pdic_event_dest_string);

const char *pdic_event_id_string(pdic_notifier_id_t id)
{
	/* enum pdic_notifier_id_t */
	switch (id) {
	case PDIC_NOTIFY_ID_INITIAL:
		return "ID_INITIAL";
	case PDIC_NOTIFY_ID_ATTACH:
		return "ID_ATTACH";
	case PDIC_NOTIFY_ID_RID:
		return "ID_RID";
	case PDIC_NOTIFY_ID_USB:
		return "ID_USB";
	case PDIC_NOTIFY_ID_POWER_STATUS:
		return "ID_POWER_STATUS";
	case PDIC_NOTIFY_ID_WATER:
		return "ID_WATER";
	case PDIC_NOTIFY_ID_VCONN:
		return "ID_VCONN";
	case PDIC_NOTIFY_ID_OTG:
		return "ID_OTG";
	case PDIC_NOTIFY_ID_TA:
		return "ID_TA";
	case PDIC_NOTIFY_ID_DP_CONNECT:
		return "ID_DP_CONNECT";
	case PDIC_NOTIFY_ID_DP_HPD:
		return "ID_DP_HPD";
	case PDIC_NOTIFY_ID_DP_LINK_CONF:
		return "ID_DP_LINK_CONF";
	case PDIC_NOTIFY_ID_USB_DP:
		return "ID_USB_DP";
	case PDIC_NOTIFY_ID_ROLE_SWAP:
		return "ID_ROLE_SWAP";
	case PDIC_NOTIFY_ID_FAC:
		return "ID_FAC";
	case PDIC_NOTIFY_ID_CC_PIN_STATUS:
		return "ID_PIN_STATUS";
	case PDIC_NOTIFY_ID_WATER_CABLE:
		return "ID_WATER_CABLE";
	case PDIC_NOTIFY_ID_POFF_WATER:
		return "ID_POFF_WATER";
	case PDIC_NOTIFY_ID_DEVICE_INFO:
		return "ID_DEVICE_INFO";
	case PDIC_NOTIFY_ID_SVID_INFO:
		return "ID_SVID_INFO";
	case PDIC_NOTIFY_ID_CLEAR_INFO:
		return "ID_CLEAR_INFO";
#if IS_ENABLED(CONFIG_MUIC_POGO)
	case PDIC_NOTIFY_ID_POGO:
		return "ID_POGO";
#endif
	default:
		return "UNDEFINED";
	}
}
EXPORT_SYMBOL(pdic_event_id_string);

const char *pdic_rid_string(pdic_notifier_rid_t rid)
{
	switch (rid) {
	case RID_UNDEFINED:
		return "RID_UNDEFINED";
	case RID_000K:
		return "RID_000K";
	case RID_001K:
		return "RID_001K";
	case RID_255K:
		return "RID_255K";
	case RID_301K:
		return "RID_301K";
	case RID_523K:
		return "RID_523K";
	case RID_619K:
		return "RID_619K";
	case RID_OPEN:
		return "RID_OPEN";
	default:
		return "RID_UNDEFINED";
	}
}
EXPORT_SYMBOL(pdic_rid_string);

const char *pdic_usbstatus_string(USB_STATUS usbstatus)
{
	switch (usbstatus) {
	case USB_STATUS_NOTIFY_DETACH:
		return "USB_DETACH";
	case USB_STATUS_NOTIFY_ATTACH_DFP:
		return "USB_ATTACH_DFP";
	case USB_STATUS_NOTIFY_ATTACH_UFP:
		return "USB_ATTACH_UFP";
	case USB_STATUS_NOTIFY_ATTACH_DRP:
		return "USB_ATTACH_DRP";
	default:
		return "UNDEFINED";
	}
}
EXPORT_SYMBOL(pdic_usbstatus_string);

const char *pdic_ccpinstatus_string(pdic_notifier_pin_status_t ccpinstatus)
{
	switch (ccpinstatus) {
	case PDIC_NOTIFY_PIN_STATUS_NO_DETERMINATION:
		return "NO_DETERMINATION";
	case PDIC_NOTIFY_PIN_STATUS_CC1_ACTIVE:
		return "CC1_ACTIVE";
	case PDIC_NOTIFY_PIN_STATUS_CC2_ACTIVE:
		return "CC2_ACTIVE";
	case PDIC_NOTIFY_PIN_STATUS_AUDIO_ACCESSORY:
		return "AUDIO_ACCESSORY";
	case PDIC_NOTIFY_PIN_STATUS_DEBUG_ACCESSORY:
		return "DEBUG_ACCESSORY";
	case PDIC_NOTIFY_PIN_STATUS_PDIC_ERROR:
		return "CCIC_ERROR";
	case PDIC_NOTIFY_PIN_STATUS_DISABLED:
		return "DISABLED";
	case PDIC_NOTIFY_PIN_STATUS_RFU:
		return "RFU";
	case PDIC_NOTIFY_PIN_STATUS_NOCC_USB_ACTIVE:
		return "NOCC_USB_ACTIVE";
	default:
		return "NO_DETERMINATION";
	}
}

int pdic_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
			pdic_notifier_device listener)
{
	int ret = 0;

	pr_info("%s: listener=%d register\n", __func__, listener);

#if IS_BUILTIN(CONFIG_PDIC_NOTIFIER)
	/* Check if PDIC Notifier is ready. */
	if (!pdic_notifier_init_done)
		pdic_notifier_init();
#endif

	SET_PDIC_NOTIFIER_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register(&(pdic_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
				__func__, ret);

	/* current pdic's attached_device status notify */
	mutex_lock(&pdic_notifier.notify_mutex);
	nb->notifier_call(nb, 0,
			&(pdic_notifier.pdic_template));
	mutex_unlock(&pdic_notifier.notify_mutex);

	return ret;
}
EXPORT_SYMBOL(pdic_notifier_register);

int pdic_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s: listener=%d unregister\n", __func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&(pdic_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_unregister error(%d)\n",
				__func__, ret);
	DESTROY_PDIC_NOTIFIER_BLOCK(nb);

	return ret;
}
EXPORT_SYMBOL(pdic_notifier_unregister);

void pdic_uevent_work(int id, int state)
{
	char *water[2] = { "CCIC=WATER", NULL };
	char *dry[2] = { "CCIC=DRY", NULL };
	char *vconn[2] = { "CCIC=VCONN", NULL };
#if defined(CONFIG_SEC_FACTORY)
	char pdicrid[15] = {0,};
	char *rid[2] = {pdicrid, NULL};
	char pdicFacErr[20] = {0,};
	char *facErr[2] = {pdicFacErr, NULL};
	char pdicPinStat[20] = {0,};
	char *pinStat[2] = {pdicPinStat, NULL};
#endif
	struct device *pdic_device = get_pdic_device();

	if (!pdic_device) {
		pr_info("pdic_dev is null\n");
		return;
	}

	pr_info("usb: %s: id=%s state=%d\n", __func__, pdic_event_id_string(id), state);

	switch (id) {
	case PDIC_NOTIFY_ID_WATER:
		if (state)
			kobject_uevent_env(&pdic_device->kobj, KOBJ_CHANGE, water);
		else
			kobject_uevent_env(&pdic_device->kobj, KOBJ_CHANGE, dry);
		break;
	case PDIC_NOTIFY_ID_VCONN:
		kobject_uevent_env(&pdic_device->kobj, KOBJ_CHANGE, vconn);
		break;
#if defined(CONFIG_SEC_FACTORY)
	case PDIC_NOTIFY_ID_RID:
		snprintf(pdicrid, sizeof(pdicrid), "%s", pdic_rid_string(state));
		kobject_uevent_env(&pdic_device->kobj, KOBJ_CHANGE, rid);
		break;
	case PDIC_NOTIFY_ID_FAC:
		snprintf(pdicFacErr, sizeof(pdicFacErr), "%s:%d", "ERR_STATE", state);
		kobject_uevent_env(&pdic_device->kobj, KOBJ_CHANGE, facErr);
		break;
	case PDIC_NOTIFY_ID_CC_PIN_STATUS:
		snprintf(pdicPinStat, sizeof(pdicPinStat), "%s", pdic_ccpinstatus_string(state));
		kobject_uevent_env(&pdic_device->kobj, KOBJ_CHANGE, pinStat);
		break;
#endif
	default:
		break;
	}
}

/* pdic's attached_device attach broadcast */
int pdic_notifier_notify(PD_NOTI_TYPEDEF *p_noti, void *pd, int pdic_attach)
{
	int ret = 0;

#if IS_BUILTIN(CONFIG_PDIC_NOTIFIER)
	/* Check if PDIC Notifier is ready. */
	if (!pdic_notifier_init_done)
		pdic_notifier_init();
#endif

	mutex_lock(&pdic_notifier.notify_mutex);
	pdic_notifier.pdic_template = *p_noti;

	switch (p_noti->id) {
#if IS_ENABLED(CONFIG_SEC_PD) || defined(CONFIG_BATTERY_NOTIFIER)
	case PDIC_NOTIFY_ID_POWER_STATUS:		/* PDIC_NOTIFY_EVENT_PD_SINK */
		pr_info("%s: src:%01x dest:%01x id:%02x "
			"attach:%02x cable_type:%02x rprd:%01x\n", __func__,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->src,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->dest,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->id,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->attach,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->cable_type,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->rprd);

		if (pd != NULL) {
			pdic_notifier.pdic_template.pd = pd;
			pr_info("%s: PD event:%d, num:%d, sel:%d \n", __func__,
				((struct pdic_notifier_struct *)pd)->event,
				((struct pdic_notifier_struct *)pd)->sink_status.available_pdo_num,
				((struct pdic_notifier_struct *)pd)->sink_status.selected_pdo_num);
		}
		break;
#endif
	case PDIC_NOTIFY_ID_ATTACH:
		pr_info("%s: src:%01x dest:%01x id:%02x "
			"attach:%02x cable_type:%02x rprd:%01x\n", __func__,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->src,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->dest,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->id,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->attach,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->cable_type,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->rprd);
		break;
	case PDIC_NOTIFY_ID_RID:
		pr_info("%s: src:%01x dest:%01x id:%02x rid:%02x\n", __func__,
			((PD_NOTI_RID_TYPEDEF *)p_noti)->src,
			((PD_NOTI_RID_TYPEDEF *)p_noti)->dest,
			((PD_NOTI_RID_TYPEDEF *)p_noti)->id,
			((PD_NOTI_RID_TYPEDEF *)p_noti)->rid);
#if defined(CONFIG_SEC_FACTORY)
			pdic_uevent_work(PDIC_NOTIFY_ID_RID, ((PD_NOTI_RID_TYPEDEF *)p_noti)->rid);
#endif
		break;
#ifdef CONFIG_SEC_FACTORY
	case PDIC_NOTIFY_ID_FAC:
		pr_info("%s: src:%01x dest:%01x id:%02x ErrState:%02x\n", __func__,
			p_noti->src, p_noti->dest, p_noti->id, p_noti->sub1);
			pdic_uevent_work(PDIC_NOTIFY_ID_FAC, p_noti->sub1);
			mutex_unlock(&pdic_notifier.notify_mutex);
			return 0;
#endif
	case PDIC_NOTIFY_ID_WATER:
		pr_info("%s: src:%01x dest:%01x id:%02x attach:%02x\n", __func__,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->src,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->dest,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->id,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->attach);
			pdic_uevent_work(PDIC_NOTIFY_ID_WATER, ((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->attach);
#ifdef CONFIG_SEC_FACTORY
			pr_info("%s: Do not notifier, just return\n", __func__);
			mutex_unlock(&pdic_notifier.notify_mutex);
			return 0;
#endif
		break;
	case PDIC_NOTIFY_ID_VCONN:
		pdic_uevent_work(PDIC_NOTIFY_ID_VCONN, 0);
		break;
	case PDIC_NOTIFY_ID_ROLE_SWAP:
		pr_info("%s: src:%01x dest:%01x id:%02x sub1:%02x\n", __func__,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->src,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->dest,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->id,
			((PD_NOTI_ATTACH_TYPEDEF *)p_noti)->attach);
		break;
#ifdef CONFIG_SEC_FACTORY
	case PDIC_NOTIFY_ID_CC_PIN_STATUS:
		pr_info("%s: src:%01x dest:%01x id:%02x pinStatus:%02x\n", __func__,
			p_noti->src, p_noti->dest, p_noti->id, p_noti->sub1);
			pdic_uevent_work(PDIC_NOTIFY_ID_CC_PIN_STATUS, p_noti->sub1);
			mutex_unlock(&pdic_notifier.notify_mutex);
			return 0;
#endif
	case PDIC_NOTIFY_ID_DEVICE_INFO:
		pr_info("%s: src:%01x dest:%01x id:%02x vendor_id:%04x product_id:%04x ifpmic_index:%02x version:%02x\n",
			__func__,
			((PD_NOTI_DEVICE_INFO_TYPEDEF *)p_noti)->src,
			((PD_NOTI_DEVICE_INFO_TYPEDEF *)p_noti)->dest,
			((PD_NOTI_DEVICE_INFO_TYPEDEF *)p_noti)->id,
			((PD_NOTI_DEVICE_INFO_TYPEDEF *)p_noti)->vendor_id,
			((PD_NOTI_DEVICE_INFO_TYPEDEF *)p_noti)->product_id,
			((PD_NOTI_DEVICE_INFO_TYPEDEF *)p_noti)->ifpmic_index,
			((PD_NOTI_DEVICE_INFO_TYPEDEF *)p_noti)->version);
		break;
	case PDIC_NOTIFY_ID_SVID_INFO:
		pr_info("%s: src:%01x dest:%01x id:%02x standard_vendor_id:%04x\n",
			__func__,
			((PD_NOTI_SVID_INFO_TYPEDEF *)p_noti)->src,
			((PD_NOTI_SVID_INFO_TYPEDEF *)p_noti)->dest,
			((PD_NOTI_SVID_INFO_TYPEDEF *)p_noti)->id,
			((PD_NOTI_SVID_INFO_TYPEDEF *)p_noti)->standard_vendor_id);
		break;
	case PDIC_NOTIFY_ID_CLEAR_INFO:
		pr_info("%s: src:%01x dest:%01x id:%02x clear_id:%04x\n",
			__func__,
			((PD_NOTI_CLEAR_INFO_TYPEDEF *)p_noti)->src,
			((PD_NOTI_CLEAR_INFO_TYPEDEF *)p_noti)->dest,
			((PD_NOTI_CLEAR_INFO_TYPEDEF *)p_noti)->id,
			((PD_NOTI_CLEAR_INFO_TYPEDEF *)p_noti)->clear_id);
		break;
	default:
		pr_info("%s: src:%01x dest:%01x id:%02x "
			"sub1:%d sub2:%02x sub3:%02x\n", __func__,
			((PD_NOTI_TYPEDEF *)p_noti)->src,
			((PD_NOTI_TYPEDEF *)p_noti)->dest,
			((PD_NOTI_TYPEDEF *)p_noti)->id,
			((PD_NOTI_TYPEDEF *)p_noti)->sub1,
			((PD_NOTI_TYPEDEF *)p_noti)->sub2,
			((PD_NOTI_TYPEDEF *)p_noti)->sub3);
		break;
	}
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	if (p_noti->id != PDIC_NOTIFY_ID_POWER_STATUS)
		store_usblog_notify(NOTIFY_CCIC_EVENT, (void *)p_noti, NULL);
#endif
	ret = blocking_notifier_call_chain(&(pdic_notifier.notifier_call_chain),
			p_noti->id, &(pdic_notifier.pdic_template));

	switch (ret) {
	case NOTIFY_STOP_MASK:
	case NOTIFY_BAD:
		pr_err("%s: notify error occur(0x%x)\n", __func__, ret);
		break;
	case NOTIFY_DONE:
	case NOTIFY_OK:
		pr_info("%s: notify done(0x%x)\n", __func__, ret);
		break;
	default:
		pr_info("%s: notify status unknown(0x%x)\n", __func__, ret);
		break;
	}

	mutex_unlock(&pdic_notifier.notify_mutex);
	return ret;
}
EXPORT_SYMBOL(pdic_notifier_notify);

int pdic_notifier_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);
	if (pdic_notifier_init_done) {
		pr_err("%s already registered\n", __func__);
		goto out;
	}
	pdic_notifier_init_done = 1;
	pdic_core_init();
	BLOCKING_INIT_NOTIFIER_HEAD(&(pdic_notifier.notifier_call_chain));
	mutex_init(&pdic_notifier.notify_mutex);

out:
	return ret;
}

static void __exit pdic_notifier_exit(void)
{
	mutex_destroy(&pdic_notifier.notify_mutex);
	pr_info("%s: exit\n", __func__);
}

device_initcall(pdic_notifier_init);
module_exit(pdic_notifier_exit);

MODULE_AUTHOR("Samsung USB Team");
MODULE_DESCRIPTION("Pdic Notifier");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_DESC);
