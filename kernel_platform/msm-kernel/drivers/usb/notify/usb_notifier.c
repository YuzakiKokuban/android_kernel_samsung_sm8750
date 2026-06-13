// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019-2023 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#define pr_fmt(fmt) "usb_notifier: " fmt

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/usb_notify.h>
#ifdef CONFIG_OF
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
#include <linux/usb/typec/common/pdic_notifier.h>
#endif
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/common/muic.h>
#include <linux/muic/common/muic_notifier.h>
#endif
#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
#include <linux/vbus_notifier.h>
#endif
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
#include <linux/usb/typec/manager/usb_typec_manager_notifier.h>
#endif

#include <linux/battery/sec_battery_common.h>
#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
#include <linux/combo_redriver/ps5169.h>
#endif
#include "usb_notifier.h"


struct usb_notifier_platform_data {
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	struct	notifier_block ccic_usb_nb;
	int is_host;
#endif
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
	struct	notifier_block muic_usb_nb;
#endif
#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
	struct	notifier_block vbus_nb;
#endif
	int	gpio_redriver_en;
	int disable_control_en;
	int	unsupport_host_en;
	int	support_reverse_bypass_en;
};

#ifdef CONFIG_OF
static void of_get_usb_redriver_dt(struct device_node *np,
		struct usb_notifier_platform_data *pdata)
{
	pdata->gpio_redriver_en = of_get_named_gpio(np, "qcom,gpios_redriver_en", 0);
	if (pdata->gpio_redriver_en < 0)
		pr_info("There is not USB 3.0 redriver pin\n");

	if (!gpio_is_valid(pdata->gpio_redriver_en))
		pr_err("%s: usb30_redriver_en: Invalied gpio pins\n", __func__);

	pr_info("redriver_en : %d\n", pdata->gpio_redriver_en);
}

static void of_get_disable_control_en_dt(struct device_node *np,
		struct usb_notifier_platform_data *pdata)
{
	of_property_read_u32(np, "qcom,disable_control_en", &pdata->disable_control_en);

	pr_info("disable_control_en : %d\n", pdata->disable_control_en);
}

static void of_get_unsupport_host_dt(struct device_node *np,
		struct usb_notifier_platform_data *pdata)
{
	of_property_read_u32(np, "qcom,unsupport_host_en", &pdata->unsupport_host_en);

	pr_info("unsupport_host_en : %d\n", pdata->unsupport_host_en);
}

static void of_get_support_reverse_bypass_dt(struct device_node *np,
		struct usb_notifier_platform_data *pdata)
{
	pdata->support_reverse_bypass_en = of_property_read_bool(np, "support_reverse_bypass_en");

	pr_info("support_reverse_bypass_en : %d\n", pdata->support_reverse_bypass_en);
}

static int of_usb_notifier_dt(struct device *dev,
		struct usb_notifier_platform_data *pdata)
{
	struct device_node *np = dev->of_node;

	if (!np)
		return -EINVAL;

	of_get_usb_redriver_dt(np, pdata);
	of_get_disable_control_en_dt(np, pdata);
	of_get_unsupport_host_dt(np, pdata);
	of_get_support_reverse_bypass_dt(np, pdata);
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
static int ccic_usb_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	PD_NOTI_USB_STATUS_TYPEDEF usb_status = *(PD_NOTI_USB_STATUS_TYPEDEF *)data;
	struct otg_notify *o_notify = get_otg_notify();
	struct usb_notifier_platform_data *pdata =
		container_of(nb, struct usb_notifier_platform_data, ccic_usb_nb);

	if (usb_status.dest != PDIC_NOTIFY_DEV_USB)
		return 0;

	switch (usb_status.drp) {
	case USB_STATUS_NOTIFY_ATTACH_DFP:
		pr_info("%s: Turn On Host(DFP), max speed restrict = %d\n", __func__, usb_status.sub3);

//UFP = 0, DFP = 1
#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
		ps5169_config(USB_ONLY_MODE, 1);
#endif
		dwc3_max_speed_setting(usb_status.sub3);
		send_otg_notify(o_notify, NOTIFY_EVENT_HOST, 1);
		pdata->is_host = 1;
		break;
	case USB_STATUS_NOTIFY_ATTACH_UFP:
		pr_info("%s: Turn On Device(UFP)\n", __func__);
		dwc3_max_speed_setting(usb_status.sub3);
		send_otg_notify(o_notify, NOTIFY_EVENT_VBUS, 1);
		break;
	case USB_STATUS_NOTIFY_DETACH:
		if (pdata->is_host) {
			pr_info("%s: Turn Off Host(DFP)\n", __func__);
			send_otg_notify(o_notify, NOTIFY_EVENT_HOST, 0);
			pdata->is_host = 0;
		} else {
			pr_info("%s: Turn Off Device(UFP)\n", __func__);
			send_otg_notify(o_notify, NOTIFY_EVENT_VBUS, 0);
		}
#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
		ps5169_config(CLEAR_STATE, 0);
#endif
		break;
	default:
		pr_info("%s: unsupported DRP type : %d.\n", __func__, usb_status.drp);
		break;
	}
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
static int muic_usb_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	struct otg_notify *o_notify = get_otg_notify();
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	PD_NOTI_ATTACH_TYPEDEF *p_noti = (PD_NOTI_ATTACH_TYPEDEF *)data;
	muic_attached_dev_t attached_dev = p_noti->cable_type;

	pr_info("%s action=%lu, attached_dev=%d\n",
		__func__, action, attached_dev);

	switch (attached_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_USB_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_CDP_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_USB_CABLE, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_USB_CABLE, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	default:
		break;
	}
#else
	muic_attached_dev_t attached_dev = *(muic_attached_dev_t *)data;

	pr_info("%s action=%lu, attached_dev=%d\n",
		__func__, action, attached_dev);

	switch (attached_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_USB_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_CDP_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_USB_CABLE, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_USB_CABLE, 1);
		else
			;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
		fallthrough;
#endif
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_VBUS, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_VBUS, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_OTG_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_HOST, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_HOST, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_HMT_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_HMT, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_HMT, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			pr_info("%s - USB_HOST_TEST_DETACHED\n", __func__);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			pr_info("%s - USB_HOST_TEST_ATTACHED\n", __func__);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_SMARTDOCK_TA_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_SMARTDOCK_TA, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_SMARTDOCK_TA, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_SMARTDOCK_USB_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify
				(o_notify, NOTIFY_EVENT_SMARTDOCK_USB, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify
				(o_notify, NOTIFY_EVENT_SMARTDOCK_USB, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_AUDIODOCK_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_AUDIODOCK, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_AUDIODOCK, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_UNIVERSAL_MMDOCK_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_MMDOCK, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_MMDOCK, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_USB_LANHUB_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_LANHUB, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH) {
			send_otg_notify(o_notify, NOTIFY_EVENT_DRIVE_VBUS, 0);
			send_otg_notify(o_notify, NOTIFY_EVENT_LANHUB, 1);
		} else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	case ATTACHED_DEV_GAMEPAD_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_GAMEPAD, 0);
		else if (action == MUIC_NOTIFY_CMD_ATTACH)
			send_otg_notify(o_notify, NOTIFY_EVENT_GAMEPAD, 1);
		else
			pr_err("%s - ACTION Error!\n", __func__);
		break;
	default:
		break;
	}
#endif
	return 0;
}
#endif
#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
static int vbus_handle_notification(struct notifier_block *nb,
		unsigned long cmd, void *data)
{
	vbus_status_t vbus_type = *(vbus_status_t *)data;
	struct otg_notify *o_notify;

	o_notify = get_otg_notify();

	pr_info("%s cmd=%lu, vbus_type=%s\n",
		__func__, cmd, vbus_type == STATUS_VBUS_HIGH ? "HIGH" : "LOW");

	switch (vbus_type) {
	case STATUS_VBUS_HIGH:
		send_otg_notify(o_notify, NOTIFY_EVENT_VBUSPOWER, 1);
		break;
	case STATUS_VBUS_LOW:
		send_otg_notify(o_notify, NOTIFY_EVENT_VBUSPOWER, 0);
		break;
	default:
		break;
	}
	return 0;
}
#endif

static int otg_accessory_power(bool enable)
{
	struct power_supply *psy_otg;
	union power_supply_propval val;
	int on = !!enable;
	int ret = 0;

	pr_info("%s %d, enable=%d\n", __func__, __LINE__, enable);
	/* otg psy test */
	psy_otg = get_power_supply_by_name("otg");

	if (psy_otg) {
		val.intval = enable;
		ret = psy_otg->desc->set_property(psy_otg, POWER_SUPPLY_PROP_ONLINE, &val);
	} else {
		pr_err("%s: Fail to get psy battery\n", __func__);
		return -1;
	}
	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	} else {
		pr_info("otg accessory power = %d\n", on);
	}

	return ret;
}

static int set_online(int event, int state)
{
	union power_supply_propval value;
	struct power_supply *psy, *psy_otg;

	pr_info("%s: %d, %d\n", __func__, event, state);

	psy = power_supply_get_by_name("battery");
	if (!psy) {
		pr_err("%s: fail to get battery power_supply\n", __func__);
		return -1;
	}
	psy_otg = get_power_supply_by_name("otg");
	if (!psy_otg) {
		pr_err("%s: fail to get battery power_supply_otg\n", __func__);
		return -1;
	}

	if (event == NOTIFY_EVENT_HMD_EXT_CURRENT) {
		value.intval = state;
		psy_otg->desc->set_property(psy_otg, POWER_SUPPLY_PROP_VOLTAGE_MAX, &value);
	} else {
		if (state)
			value.intval = SEC_BATTERY_CABLE_SMART_OTG;
		else
			value.intval = SEC_BATTERY_CABLE_SMART_NOTG;

		psy->desc->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	}

	return 0;
}

static int qcom_set_host(bool enable)
{
	dwc_msm_id_event(enable);
	return 0;
}

static int qcom_set_peripheral(bool enable)
{
#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
	if (enable)
		ps5169_config(USB_ONLY_MODE, 0);
#endif
	dwc_msm_vbus_event(enable);
	return 0;
}

static int qcom_get_gadget_speed(void)
{
	return gadget_speed();
}

static int usb_set_chg_current(int state)
{
	union power_supply_propval val;
	struct device_node *np_charger = NULL;
	char *charger_name;

	np_charger = of_find_node_by_name(NULL, "battery");
	if (!np_charger) {
		pr_err("%s: failed to get the battery device node\n", __func__);
		return 0;
	} else {
		if (!of_property_read_string(np_charger, "battery,charger_name",
					(char const **)&charger_name)) {
			pr_info("%s: charger_name = %s\n", __func__,
					charger_name);
		} else {
			pr_err("%s: failed to get the charger name\n",  __func__);
			return 0;
		}
	}
	/* current setting */
	pr_info("usb : charing current set = %d\n", state);

	switch (state) {
	case NOTIFY_USB_SUSPENDED:
		val.intval = USB_CURRENT_SUSPENDED;
		break;
	case NOTIFY_USB_UNCONFIGURED:
		val.intval = USB_CURRENT_UNCONFIGURED;
		break;
	case NOTIFY_USB_CONFIGURED:
		val.intval = USB_CURRENT_HIGH_SPEED;
		break;
	default:
		val.intval = USB_CURRENT_HIGH_SPEED;
		break;
	}

	psy_do_property("battery", set,
		POWER_SUPPLY_EXT_PROP_USB_CONFIGURE, val);

	return 0;
}

#if IS_ENABLED(CONFIG_USB_HW_PARAM)
static int is_skip_list(int index)
{
	int ret = 0;

	switch (index) {
	case USB_CCIC_VR_USE_COUNT:
	case USB_HOST_CLASS_COMM_COUNT:
	case USB_HOST_CLASS_PHYSICAL_COUNT:
	case USB_HOST_CLASS_IMAGE_COUNT:
	case USB_HOST_CLASS_PRINTER_COUNT:
	case USB_HOST_CLASS_CDC_COUNT:
	case USB_HOST_CLASS_CSCID_COUNT:
	case USB_HOST_CLASS_CONTENT_COUNT:
	case USB_HOST_CLASS_VIDEO_COUNT:
	case USB_HOST_CLASS_WIRELESS_COUNT:
	case USB_HOST_CLASS_MISC_COUNT:
	case USB_HOST_CLASS_APP_COUNT:
	case USB_HALL_FOLDING_COUNT:
	case USB_HOST_CLASS_VENDOR_COUNT:
	case USB_CCIC_FWUP_ERROR_COUNT:
	case USB_CCIC_VERSION:
		ret = 1;
		break;
	default:
		break;
	}

	return ret;
}
#endif

static int reverse_bypass_power(int mode)
{
	union power_supply_propval val;
	int ret = 0;

	pr_info("%s %d, mode=%d\n", __func__, __LINE__, mode);

	if (mode)
		val.intval = TURN_OTG_OFF_RB_ON;
	else
		val.intval = TURN_RB_OFF;
	ret = psy_do_property("otg", set, POWER_SUPPLY_EXT_PROP_OTG_VBUS_CTRL, val);
	if (ret < 0) {
		pr_err("%s: Fail to control reverse bypass\n", __func__);
		return -1;
	}

	return ret;
}

static int get_support_reverse_bypass_en(void *data)
{
	struct usb_notifier_platform_data *pdata =
		(struct usb_notifier_platform_data *)(data);
	
	return pdata->support_reverse_bypass_en;
}

static struct otg_notify sec_otg_notify = {
	.vbus_drive	= otg_accessory_power,
	.reverse_bypass_drive = reverse_bypass_power,
	.get_support_reverse_bypass_en = get_support_reverse_bypass_en,
	.set_host = qcom_set_host,
	.set_peripheral	= qcom_set_peripheral,
	.get_gadget_speed = qcom_get_gadget_speed,

	.vbus_detect_gpio = -1,
	.is_host_wakelock = 0,
	.is_wakelock = 1,
	.unsupport_host = 0,
#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
	.booting_delay_sec = 16,
#else
	.booting_delay_sec = 10,
#endif
#if !IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	.auto_drive_vbus = NOTIFY_OP_PRE,
#endif
	.disable_control = 1,
	.device_check_sec = 3,
	.set_battcall = set_online,
	.set_chg_current = usb_set_chg_current,
#if IS_ENABLED(CONFIG_USB_HW_PARAM)
	.is_skip_list = is_skip_list,
#endif
	.pre_peri_delay_us = 6,
};

static int usb_notifier_probe(struct platform_device *pdev)
{
	struct usb_notifier_platform_data *pdata = NULL;
	int ret = 0;

	pr_info("notifier_probe\n");

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
			sizeof(struct usb_notifier_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		ret = of_usb_notifier_dt(&pdev->dev, pdata);
		if (ret < 0) {
			dev_err(&pdev->dev, "Failed to get device of_node\n");
			return ret;
		}

		pdev->dev.platform_data = pdata;
	} else
		pdata = pdev->dev.platform_data;

	sec_otg_notify.redriver_en_gpio = pdata->gpio_redriver_en;
	if (pdata->disable_control_en == 1)
		sec_otg_notify.disable_control = 1;
	if (pdata->unsupport_host_en == 1)
		sec_otg_notify.unsupport_host = 1;
	set_otg_notify(&sec_otg_notify);
	set_notify_data(&sec_otg_notify, pdata);
	sec_otg_notify.booting_delay_sync_usb = is_dwc3_msm_probe_done() ? 0 : 1;
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	pdata->is_host = 0;
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	manager_notifier_register(&pdata->ccic_usb_nb, ccic_usb_handle_notification,
					MANAGER_NOTIFY_PDIC_USB);
#else
	ccic_notifier_register(&pdata->ccic_usb_nb, ccic_usb_handle_notification,
				   PDIC_NOTIFY_DEV_USB);
#endif
#endif
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
	muic_notifier_register(&pdata->muic_usb_nb, muic_usb_handle_notification,
			       MUIC_NOTIFY_DEV_USB);
#endif
#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
	vbus_notifier_register(&pdata->vbus_nb, vbus_handle_notification,
			       VBUS_NOTIFY_DEV_USB);
#endif
	dev_info(&pdev->dev, "usb notifier probe\n");
	return 0;
}

static int usb_notifier_remove(struct platform_device *pdev)
{
	struct usb_notifier_platform_data *pdata = dev_get_platdata(&pdev->dev);
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	manager_notifier_unregister(&pdata->ccic_usb_nb);
#else
	ccic_notifier_unregister(&pdata->ccic_usb_nb);
#endif
#elif IS_ENABLED(CONFIG_MUIC_NOTIFIER)
	muic_notifier_unregister(&pdata->muic_usb_nb);
#endif
#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
	vbus_notifier_unregister(&pdata->vbus_nb);
#endif
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id usb_notifier_dt_ids[] = {
	{ .compatible = "samsung,usb-notifier",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, usb_notifier_dt_ids);
#endif

static struct platform_driver usb_notifier_driver = {
	.probe		= usb_notifier_probe,
	.remove		= usb_notifier_remove,
	.driver		= {
		.name	= "usb_notifier",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table	= of_match_ptr(usb_notifier_dt_ids),
#endif
	},
};

static int __init usb_notifier_init(void)
{
	return platform_driver_register(&usb_notifier_driver);
}

static void __exit usb_notifier_exit(void)
{
	platform_driver_unregister(&usb_notifier_driver);
}

late_initcall(usb_notifier_init);
module_exit(usb_notifier_exit);

MODULE_AUTHOR("Samsung USB Team");
MODULE_DESCRIPTION("USB Notifier");
MODULE_LICENSE("GPL");
