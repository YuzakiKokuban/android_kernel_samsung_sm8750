/* drivers/muic/muic-core.c
 *
 * Copyright (c) 2011-2023 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/kernel.h>

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
#include <linux/sec_class.h>
#endif

#if IS_ENABLED(CONFIG_ANDROID_SWITCH) || IS_ENABLED(CONFIG_SWITCH)
#include <linux/switch.h>
#endif

#if defined(CONFIG_USB_HW_PARAM)
#include <linux/usb_notify.h>
#endif

#include <linux/muic/common/muic.h>
#include <linux/muic/common/muic_kunit.h>
#include <linux/muic/common/muic_notifier.h>

static void muic_init_switch_dev_cb(void);
static void muic_cleanup_switch_dev_cb(void);
#if IS_MODULE(CONFIG_MUIC_NOTIFIER)
static int muic_init_gpio_cb(int switch_sel);
#else
static int muic_init_gpio_cb(void);
#endif

struct muic_platform_data muic_pdata = {
	.init_switch_dev_cb	= muic_init_switch_dev_cb,
	.cleanup_switch_dev_cb	= muic_cleanup_switch_dev_cb,
	.init_gpio_cb		= muic_init_gpio_cb,
};
EXPORT_SYMBOL(muic_pdata);

#if IS_ENABLED(CONFIG_USE_SECOND_MUIC)
struct muic_platform_data muic_pdata2;
#endif

#if IS_ENABLED(CONFIG_ANDROID_SWITCH) || IS_ENABLED(CONFIG_SWITCH)
static struct switch_dev switch_dock = {
	.name = "dock",
};

struct switch_dev switch_uart3 = {
	.name = "uart3", /* sys/class/switch/uart3/state */
};
#endif

static struct notifier_block dock_notifier_block;
static struct notifier_block cable_data_notifier_block;

static int muic_device_uevent(struct muic_platform_data *muic_pdata,
	char *envp_ext[])
{
	int ret = 0;

	if (muic_pdata && muic_pdata->muic_device)
		kobject_uevent_env(&muic_pdata->muic_device->kobj,
				KOBJ_CHANGE, envp_ext);
	else {
		ret = -ENOENT;
		pr_err("%s: muic_pdata or muic_device is NULL\n", __func__);
		goto err;
	}
	pr_info("%s\n", __func__);
err:
	return ret;
}

void muic_send_lcd_on_uevent(struct muic_platform_data *muic_pdata)
{
	char *envp[2];
	char *type = {"TYPE=LCD_ON"};
	int index = 0;

	envp[index++] = type;
	envp[index++] = NULL;

	if (muic_device_uevent(muic_pdata, envp)) {
		pr_err("%s: muic_device_uevent has error\n", __func__);
		goto err;
	}
	pr_info("%s\n", __func__);
err:
	return;
}
EXPORT_SYMBOL(muic_send_lcd_on_uevent);

void muic_send_dock_intent(int type)
{
	pr_info("%s: MUIC dock type(%d)\n", __func__, type);
#if IS_ENABLED(CONFIG_ANDROID_SWITCH) || IS_ENABLED(CONFIG_SWITCH)
	switch_set_state(&switch_dock, type);
#endif
}
EXPORT_SYMBOL(muic_send_dock_intent);

static int muic_dock_attach_notify(int type, const char *name)
{
	pr_info("%s: %s\n", __func__, name);
	muic_send_dock_intent(type);

	return NOTIFY_OK;
}

static int muic_dock_detach_notify(void)
{
	pr_info("%s\n", __func__);
	muic_send_dock_intent(MUIC_DOCK_DETACHED);

	return NOTIFY_OK;
}

static muic_attached_dev_t muic_get_attached_dev(void *data)
{
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER) && IS_ENABLED(CONFIG_MUIC_SUPPORT_PDIC)
	PD_NOTI_ATTACH_TYPEDEF *pnoti = (PD_NOTI_ATTACH_TYPEDEF *)data;
	muic_attached_dev_t attached_dev = pnoti->cable_type;
#else
	muic_attached_dev_t attached_dev = *(muic_attached_dev_t *)data;
#endif

	return attached_dev;
}

static int muic_handle_dock_notification(struct notifier_block *nb,
			unsigned long action, void *data)
{
	muic_attached_dev_t attached_dev = ATTACHED_DEV_NONE_MUIC;
	int type = MUIC_DOCK_DETACHED;
	const char *name;

	if (!data) {
		pr_err("%s. data is null\n", __func__);
		goto ignore;
	}

	attached_dev = muic_get_attached_dev(data);

	switch (attached_dev) {
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH) {
			type = MUIC_DOCK_DESKDOCK;
			name = "Desk Dock Attach";
			return muic_dock_attach_notify(type, name);
		}
		else if (action == MUIC_NOTIFY_CMD_DETACH)
			return muic_dock_detach_notify();
		break;
	case ATTACHED_DEV_CARDOCK_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH) {
			type = MUIC_DOCK_CARDOCK;
			name = "Car Dock Attach";
			return muic_dock_attach_notify(type, name);
		}
		else if (action == MUIC_NOTIFY_CMD_DETACH)
			return muic_dock_detach_notify();
		break;
	case ATTACHED_DEV_SMARTDOCK_MUIC:
	case ATTACHED_DEV_SMARTDOCK_VB_MUIC:
	case ATTACHED_DEV_SMARTDOCK_TA_MUIC:
	case ATTACHED_DEV_SMARTDOCK_USB_MUIC:
		if (action == MUIC_NOTIFY_CMD_LOGICALLY_ATTACH) {
			type = MUIC_DOCK_SMARTDOCK;
			name = "Smart Dock Attach";
			return muic_dock_attach_notify(type, name);
		}
		else if (action == MUIC_NOTIFY_CMD_LOGICALLY_DETACH)
			return muic_dock_detach_notify();
		break;
	case ATTACHED_DEV_UNIVERSAL_MMDOCK_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH) {
			type = MUIC_DOCK_SMARTDOCK;
			name = "Universal MMDock Attach";
			return muic_dock_attach_notify(type, name);
		}
		else if (action == MUIC_NOTIFY_CMD_DETACH)
			return muic_dock_detach_notify();
		break;
	case ATTACHED_DEV_AUDIODOCK_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH) {
			type = MUIC_DOCK_AUDIODOCK;
			name = "Audio Dock Attach";
			return muic_dock_attach_notify(type, name);
		}
		else if (action == MUIC_NOTIFY_CMD_DETACH)
			return muic_dock_detach_notify();
		break;
	case ATTACHED_DEV_HMT_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH) {
			type = MUIC_DOCK_HMT;
			name = "HMT Attach";
			return muic_dock_attach_notify(type, name);
		}
		else if (action == MUIC_NOTIFY_CMD_DETACH)
			return muic_dock_detach_notify();
		break;
	case ATTACHED_DEV_GAMEPAD_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH) {
			type = MUIC_DOCK_GAMEPAD;
			name = "Gamepad Attach";
			return muic_dock_attach_notify(type, name);
		} else if (action == MUIC_NOTIFY_CMD_DETACH)
			return muic_dock_detach_notify();
		break;
	default:
		break;
	}
ignore:
	pr_info("%s: ignore(%d)\n", __func__, attached_dev);
	return NOTIFY_DONE;
}

static int muic_handle_cable_data_notification(struct notifier_block *nb,
			unsigned long action, void *data)
{
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif
	muic_attached_dev_t attached_dev;
	int jig_state = 0;

	if (!data) {
		pr_err("%s. data is null\n", __func__);
		goto ignore;
	}

	attached_dev = muic_get_attached_dev(data);

	switch (attached_dev) {
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:		/* VBUS enabled */
	case ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC:	/* for otg test */
	case ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC:	/* for fg test */
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:		/* VBUS enabled */
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH)
			jig_state = 1;
		break;
	case ATTACHED_DEV_TIMEOUT_OPEN_MUIC:
#if defined(CONFIG_USB_HW_PARAM)
		if (action == MUIC_NOTIFY_CMD_ATTACH && o_notify)
			inc_hw_param(o_notify, USB_MUIC_DCD_TIMEOUT_COUNT);
#endif
		break;
	default:
		jig_state = 0;
		break;
	}

	pr_info("%s: MUIC uart type(%d)\n", __func__, jig_state);
#if IS_ENABLED(CONFIG_ANDROID_SWITCH) || IS_ENABLED(CONFIG_SWITCH)
	switch_set_state(&switch_uart3, jig_state);
#endif
	return NOTIFY_OK;
ignore:
	return NOTIFY_DONE;
}

static void muic_init_switch_dev_cb(void)
{
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	struct muic_platform_data *pdata = &muic_pdata;
#endif
#if IS_ENABLED(CONFIG_ANDROID_SWITCH) || IS_ENABLED(CONFIG_SWITCH)
	int ret;

	/* for DockObserver */
	ret = switch_dev_register(&switch_dock);
	if (ret < 0) {
		pr_err("%s: Failed to register dock switch(%d)\n",
				__func__, ret);
		return;
	}

	/* for UART event */
	ret = switch_dev_register(&switch_uart3);
	if (ret < 0) {
		pr_err("%s: Failed to register uart3 switch(%d)\n",
				__func__, ret);
		return;
	}
#endif /* CONFIG_ANDROID_SWITCH || CONFIG_SWITCH */

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	pdata->muic_device = sec_device_create(NULL, "muic");
	if (IS_ERR(pdata->muic_device))
		pr_err("%s: Failed to create device(muic)!\n", __func__);
#endif

	muic_notifier_register(&dock_notifier_block,
			muic_handle_dock_notification, MUIC_NOTIFY_DEV_DOCK);
	muic_notifier_register(&cable_data_notifier_block,
			muic_handle_cable_data_notification, MUIC_NOTIFY_DEV_CABLE_DATA);

	pr_info("%s: done\n", __func__);
}

static void muic_cleanup_switch_dev_cb(void)
{
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	struct muic_platform_data *pdata = &muic_pdata;
#endif

	muic_notifier_unregister(&cable_data_notifier_block);
	muic_notifier_unregister(&dock_notifier_block);

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	if (pdata->muic_device)
		sec_device_destroy(pdata->muic_device->devt);
#endif

#if IS_ENABLED(CONFIG_ANDROID_SWITCH) || IS_ENABLED(CONFIG_SWITCH)
	/* for UART event */
	if (switch_uart3.dev)
		switch_dev_unregister(&switch_uart3);
	/* for DockObserver */
	if (switch_dock.dev)
		switch_dev_unregister(&switch_dock);
#endif /* CONFIG_ANDROID_SWITCH || CONFIG_SWITCH */

	pr_info("%s: done\n", __func__);
}

bool is_muic_usb_path_ap_usb(void)
{
	if (MUIC_PATH_USB_AP == muic_pdata.usb_path) {
		pr_info("%s: [%d]\n", __func__, muic_pdata.usb_path);
		return true;
	}

	return false;
}

bool is_muic_usb_path_cp_usb(void)
{
	if (MUIC_PATH_USB_CP == muic_pdata.usb_path) {
		pr_info("%s: [%d]\n", __func__, muic_pdata.usb_path);
		return true;
	}

	return false;
}

__visible_for_testing void muic_set_afc_disable(struct muic_platform_data *pdata)
{
#if !IS_ENABLED(CONFIG_HV_MUIC_AFC_DISABLE_ENFORCE)
	if (get_afc_mode() == CH_MODE_AFC_DISABLE_VAL) {
		pr_info("AFC mode disabled\n");
		pdata->afc_disable = true;
	} else {
		pr_info("AFC mode enabled\n");
		pdata->afc_disable = false;
	}
#else
	pr_info("AFC mode disable enforce\n");
	pdata->afc_disable = true;
#endif /* !CONFIG_HV_MUIC_AFC_DISABLE_ENFORCE */
}
EXPORT_SYMBOL_KUNIT(muic_set_afc_disable);

static const char *muic_path_name(int path)
{
	switch (path) {
	case MUIC_PATH_USB_AP:
		return "USB_AP";
	case MUIC_PATH_USB_CP:
		return "USB_CP";
	case MUIC_PATH_UART_AP:
		return "UART_AP";
	case MUIC_PATH_UART_CP:
		return "UART_CP";
	case MUIC_PATH_OPEN:
		return "OPEN";
	case MUIC_PATH_AUDIO:
		return "AUDIO";
	default:
		return "UNDEFINED";
	}
}

__visible_for_testing int muic_set_path(struct muic_platform_data *pdata)
{
	int ret = 0;

	if (pdata->set_gpio_usb_sel) {
		ret = pdata->set_gpio_usb_sel(pdata->drv_data, pdata->usb_path);
		if (ret) {
			pr_err("%s set_gpio_usb_sel error\n", __func__);
			goto err;
		}
	}

	if (pdata->set_gpio_uart_sel) {
		ret = pdata->set_gpio_uart_sel(pdata->drv_data, pdata->uart_path);
		if (ret) {
			pr_err("%s set_gpio_uart_sel error\n", __func__);
			goto err;
		}
	}

	pr_info("%s: usb_path(%s), uart_path(%s)\n", __func__,
			muic_path_name(pdata->usb_path), muic_path_name(pdata->uart_path));

err:
	return ret;
}
EXPORT_SYMBOL_KUNIT(muic_set_path);

#if IS_MODULE(CONFIG_MUIC_NOTIFIER)
static int muic_init_gpio_cb(int switch_sel)
{
	struct muic_platform_data *pdata = &muic_pdata;
	int ret = 0;

	pr_info("%s (%d)\n", __func__, switch_sel);

	if (switch_sel & SWITCH_SEL_USB_MASK)
		pdata->usb_path = MUIC_PATH_USB_AP;
	else
		pdata->usb_path = MUIC_PATH_USB_CP;

	if (switch_sel & SWITCH_SEL_UART_MASK)
		pdata->uart_path = MUIC_PATH_UART_AP;
	else
		pdata->uart_path = MUIC_PATH_UART_CP;

	/* These flags MUST be updated again from probe function */
	pdata->rustproof_on = false;

#if !defined(CONFIG_SEC_FACTORY) && defined(CONFIG_MUIC_SUPPORT_TYPEB)
	if (!(switch_sel & SWITCH_SEL_RUSTPROOF_MASK))
		pdata->rustproof_on = true;
#endif

	muic_set_afc_disable(pdata);

	muic_set_path(pdata);

	return ret;
}
#else
static int muic_init_gpio_cb(void)
{
	struct muic_platform_data *pdata = &muic_pdata;
	int switch_sel = get_switch_sel();
	int uart_sel = get_uart_sel();
	int ret = 0;

	pr_info("%s: switch_sel(%d) uart_sel(%d)\n", __func__, switch_sel, uart_sel);

	pdata->usb_path = MUIC_PATH_USB_AP;

	pdata->uart_path = MUIC_PATH_UART_AP;

	if (switch_sel != -1) {
		if (switch_sel & SWITCH_SEL_USB_MASK)
			pdata->usb_path = MUIC_PATH_USB_AP;
		else
			pdata->usb_path = MUIC_PATH_USB_CP;

		if (switch_sel & SWITCH_SEL_UART_MASK)
			pdata->uart_path = MUIC_PATH_UART_AP;
		else
			pdata->uart_path = MUIC_PATH_UART_CP;

#if IS_ENABLED(CONFIG_MUIC_UART_SWITCH)
		if (switch_sel & SWITCH_SEL_UART_MASK2)
			pdata->uart_path = MUIC_PATH_UART_CP2;
#endif
	}

#if defined(CONFIG_MUIC_SUPPORT_UART_SEL)
	if (uart_sel == MUIC_PATH_UART_CP)
		pdata->uart_path = MUIC_PATH_UART_CP;
#endif

	/* These flags MUST be updated again from probe function */
	pdata->rustproof_on = false;

	muic_set_afc_disable(pdata);

	muic_set_path(pdata);

	return ret;
}
#endif

int muic_afc_get_voltage(void)
{
	struct muic_platform_data *pdata = &muic_pdata;
	int vol = -ENODEV;

	pr_info("%s : %dV\n", __func__, vol);

	if (pdata && pdata->muic_afc_get_voltage_cb)
		vol = pdata->muic_afc_get_voltage_cb();

	return vol;
}
EXPORT_SYMBOL(muic_afc_get_voltage);

int muic_afc_request_cause_clear(void)
{
	struct muic_platform_data *pdata = &muic_pdata;

	if (pdata == NULL)
		return -ENOENT;
	pdata->afc_request_cause &= ~(AFC_REQUEST_DETACH_CLEAR_BIT);
	return 0;
}
EXPORT_SYMBOL_GPL(muic_afc_request_cause_clear);

int muic_afc_request_cause_clear_bit(int cause)
{
	struct muic_platform_data *pdata = &muic_pdata;

	if (pdata == NULL)
		return -ENOENT;

	pdata->afc_request_cause &= ~(cause);
	pr_info("%s: clear cause=%d, afc_request_cause = %d\n"
				, __func__, cause, pdata->afc_request_cause);

	return 0;
}
EXPORT_SYMBOL_GPL(muic_afc_request_cause_clear_bit);

int muic_afc_get_request_cause(void)
{
	struct muic_platform_data *pdata = &muic_pdata;
	int ret = 0;

	if (pdata == NULL) {
		return ret;
	}

	ret = pdata->afc_request_cause;

	return ret;
}
EXPORT_SYMBOL_GPL(muic_afc_get_request_cause);

bool muic_is_enable_afc_request(void)
{
	struct muic_platform_data *pdata = &muic_pdata;
	bool ret = false;

	if (pdata == NULL) {
		return ret;
	}

	ret = pdata->afc_request_cause == 0;

	return ret;
}
EXPORT_SYMBOL_GPL(muic_is_enable_afc_request);

static int muic_afc_request_voltage_check(int cause, int vol)
{
	int ret = 0;

	if (vol == 9 && cause == 0)
		ret = 9;
	else
		ret = 5;
	pr_info("%s: cause=%x %dv->%dv\n", __func__, cause, vol, ret);
	return ret;
}

int muic_afc_request_voltage(int cause, int voltage)
{
	struct muic_platform_data *pdata = &muic_pdata;
	int set_vol = 0, ret = 0;

	if (pdata == NULL) {
		ret = -ENOENT;
		goto out;
	}

	if (voltage == 9) {
		pr_info("%s: afc request clear, cause(%d), voltage(%d)\n", __func__, cause, voltage);
		pdata->afc_request_cause &= ~(cause);
	} else if (voltage == 5) {
		pr_info("%s: afc request set, cause(%d), voltage(%d)\n", __func__, cause, voltage);
		pdata->afc_request_cause |= (cause);
	} else {
		pr_err("%s: not support. cause(%d), voltage(%d)\n", __func__, cause, voltage);
		ret = -EINVAL;
		goto out;
	}

	set_vol = muic_afc_request_voltage_check(pdata->afc_request_cause, voltage);
	ret = muic_afc_set_voltage(set_vol);
out:
	return ret;
}
EXPORT_SYMBOL_GPL(muic_afc_request_voltage);

int muic_afc_set_voltage(int voltage)
{
	struct muic_platform_data *pdata = &muic_pdata;
#if defined(CONFIG_USE_SECOND_MUIC)
	struct muic_platform_data *pdata2 = &muic_pdata2;
#endif
	int ret = -ENODEV;

	pr_info("%s : %dV\n", __func__, voltage);

#if defined(CONFIG_USE_SECOND_MUIC)
	if (pdata2->muic_afc_set_voltage_cb) {
		ret = pdata2->muic_afc_set_voltage_cb(voltage);
		if (ret)
			pr_err("%s second muic_afc_set_voltage_cb ret=%d\n", __func__, ret);
	}
#endif

	if (pdata && pdata->muic_afc_set_voltage_cb) {
		ret = pdata->muic_afc_set_voltage_cb(voltage);
		if (ret)
			pr_err("%s muic_afc_set_voltage_cb ret=%d\n", __func__, ret);
		return ret;
	}

	pr_err("%s: cannot supported\n", __func__);
	return ret;
}
EXPORT_SYMBOL(muic_afc_set_voltage);

int muic_hv_charger_disable(bool en)
{
	struct muic_platform_data *pdata = &muic_pdata;
#if defined(CONFIG_USE_SECOND_MUIC)
	struct muic_platform_data *pdata2 = &muic_pdata2;
#endif
	int ret = -ENODEV;

	pr_info("%s %sable\n", __func__, en ? "en" : "dis");

#if defined(CONFIG_USE_SECOND_MUIC)
	if (pdata2->muic_hv_charger_disable_cb) {
		ret = pdata2->muic_hv_charger_disable_cb(en);
		if (ret)
			pr_err("%s second muic_hv_charger_disable_cb ret=%d\n", __func__, ret);
	}
#endif

	if (pdata && pdata->muic_hv_charger_disable_cb) {
		ret = pdata->muic_hv_charger_disable_cb(en);
		if (ret)
			pr_err("%s muic_hv_charger_disable_cb ret=%d\n", __func__, ret);
	}

	return ret;
}
EXPORT_SYMBOL(muic_hv_charger_disable);

int muic_hv_charger_init(void)
{
	struct muic_platform_data *pdata = &muic_pdata;

	if (pdata)
		set_bit(CHARGER_PROBE_DONE, &pdata->driver_probe_flag);

	if (pdata && pdata->muic_hv_charger_init_cb)
		return pdata->muic_hv_charger_init_cb();

	pr_err("%s: cannot supported\n", __func__);
	return -ENODEV;
}
EXPORT_SYMBOL(muic_hv_charger_init);

int muic_set_hiccup_mode(int on_off)
{
	struct muic_platform_data *pdata = &muic_pdata;

	pr_info("%s %d\n", __func__, on_off);

	if (pdata && pdata->muic_set_hiccup_mode_cb)
		return pdata->muic_set_hiccup_mode_cb(on_off);

	pr_err("%s: cannot supported\n", __func__);
	return -ENODEV;
}
EXPORT_SYMBOL_GPL(muic_set_hiccup_mode);

#if IS_ENABLED(CONFIG_MUIC_POGO)
int muic_set_pogo_adc(int adc)
{
	struct muic_platform_data *pdata = &muic_pdata;

	if (pdata && pdata->muic_set_pogo_adc_cb)
		return pdata->muic_set_pogo_adc_cb(adc);

	pr_err("%s: cannot supported\n", __func__);
	return -ENODEV;
}
EXPORT_SYMBOL_GPL(muic_set_pogo_adc);
#endif
