/*
 * driver/muic/muic_sysfs.c - micro USB switch device driver
 *
 * Copyright (C) 2019 Samsung Electronics
 * Sejong Park <sejong123.park@samsung.com>
 * Taejung Kim <tj.kim@samsung.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#define pr_fmt(fmt)	"[MUIC] " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/muic/common/muic_notifier.h>
#include <linux/muic/common/muic.h>
#include <linux/muic/common/muic_sysfs.h>
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
#include <linux/sec_class.h>
#endif
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
#include <linux/battery/sec_battery_common.h>
#endif
#if IS_BUILTIN(CONFIG_MUIC_NOTIFIER)
#if defined(CONFIG_ARCH_QCOM)
#include <linux/sec_param.h>
#endif
#endif
#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
#include <linux/usb/typec/manager/if_cb_manager.h>
#endif

static ssize_t muic_sysfs_uart_en_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	int ret = 0;

	if (!pdata->rustproof_on) {
		pr_info("%s UART ENABLE\n",  __func__);
		ret = sprintf(buf, "1\n");
	} else {
		pr_info("%s UART DISABLE\n",  __func__);
		ret = sprintf(buf, "0\n");
	}

	return ret;
}

static ssize_t muic_sysfs_set_uart_en(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	int ret = 0;

	if (!strncmp(buf, "1", 1)) {
		pdata->rustproof_on = false;
		MUIC_PDATA_FUNC_MULTI_PARAM
			(pdata->sysfs_cb.set_uart_en,
				pdata->drv_data, MUIC_ENABLE, &ret);
	} else if (!strncmp(buf, "0", 1)) {
		pdata->rustproof_on = true;
		MUIC_PDATA_FUNC_MULTI_PARAM
			(pdata->sysfs_cb.set_uart_en,
				pdata->drv_data, MUIC_DISABLE, &ret);
	} else
		pr_info("%s invalid value\n",  __func__);

	pr_info("%s uart_en(%d)\n",
		__func__, !pdata->rustproof_on);

	return count;
}

static ssize_t muic_sysfs_uart_sel_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	const char *mode = "UNKNOWN\n";

	switch (pdata->uart_path) {
	case MUIC_PATH_UART_AP:
		mode = "AP\n";
		break;
	case MUIC_PATH_UART_CP:
		mode = "CP\n";
		break;
	default:
		break;
	}

	pr_info("%s %s", __func__, mode);
	return snprintf(buf, strlen(mode) + 1, "%s", mode);
}

static ssize_t muic_sysfs_set_uart_sel(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);

	pr_info("%s %s\n", __func__, buf);
	if (!strncasecmp(buf, "AP", 2))
		pdata->uart_path = MUIC_PATH_UART_AP;
	else if (!strncasecmp(buf, "CP", 2))
		pdata->uart_path = MUIC_PATH_UART_CP;
	else
		pr_warn("%s invalid value\n", __func__);

	MUIC_PDATA_VOID_FUNC(pdata->sysfs_cb.set_uart_sel, pdata->drv_data);

	return count;
}

static ssize_t muic_sysfs_usb_sel_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	const char *mode = "UNKNOWN\n";

	switch (pdata->usb_path) {
	case MUIC_PATH_USB_AP:
		mode = "PDA\n";
		break;
	case MUIC_PATH_USB_CP:
		mode = "MODEM\n";
		break;
	default:
		break;
	}

	pr_info("%s %s", __func__, mode);
	return sprintf(buf, "%s", mode);
}

static ssize_t muic_sysfs_set_usb_sel(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);

	if (!strncasecmp(buf, "PDA", 3))
		pdata->usb_path = MUIC_PATH_USB_AP;
	else if (!strncasecmp(buf, "MODEM", 5))
		pdata->usb_path = MUIC_PATH_USB_CP;
	else
		pr_warn("%s invalid value\n", __func__);

	pr_info("%s usb_path(%d)\n", __func__,
			pdata->usb_path);

	return count;
}

static ssize_t muic_sysfs_usb_en_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	int ret = 0;

	pr_info("%s+\n", __func__);
	MUIC_PDATA_FUNC(pdata->sysfs_cb.get_usb_en, pdata->drv_data, &ret);

	pr_info("%s ret=%d-\n", __func__, ret);

	return sprintf(buf, "%s attached_dev = %d\n",
		__func__, ret);
}

static ssize_t muic_sysfs_set_usb_en(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	int ret = 0;

	pr_info("%s+\n", __func__);
	if (!strncasecmp(buf, "1", 1)) {
		MUIC_PDATA_FUNC_MULTI_PARAM(pdata->sysfs_cb.set_usb_en, pdata->drv_data,
			MUIC_ENABLE, &ret);
	} else if (!strncasecmp(buf, "0", 1)) {
		MUIC_PDATA_FUNC_MULTI_PARAM(pdata->sysfs_cb.set_usb_en, pdata->drv_data,
			MUIC_DISABLE, &ret);
	} else {
		pr_info("%s invalid value\n", __func__);
	}

	pr_info("%s ret=%d-\n", __func__, ret);
	return count;
}

static ssize_t muic_sysfs_adc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	int adc;

	pr_info("%s+\n", __func__);
	MUIC_PDATA_FUNC(pdata->sysfs_cb.get_adc, pdata->drv_data, &adc);

	if (adc < 0) {
		pr_err("%s err read adc reg(%d)\n",
			__func__, adc);
		return sprintf(buf, "UNKNOWN\n");
	}

	pr_info("%s adc=%x-\n", __func__, adc);
	return sprintf(buf, "%x\n", adc);
}

#if IS_ENABLED(CONFIG_MUIC_DEBUG)
static ssize_t muic_sysfs_mansw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	char mesg[256] = "";
	int ret;

	MUIC_PDATA_FUNC_MULTI_PARAM(pdata->sysfs_cb.get_mansw, pdata->drv_data, mesg, &ret);
	pr_info("%s:%s\n", __func__, mesg);
	return sprintf(buf, "%s\n", mesg);
}

static ssize_t muic_sysfs_interrupt_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	char mesg[256] = "";
	int ret;

	MUIC_PDATA_FUNC_MULTI_PARAM(pdata->sysfs_cb.get_interrupt_status, pdata->drv_data, mesg, &ret);
	pr_info("%s:%s\n", __func__, mesg);
	return sprintf(buf, "%s\n", mesg);

}

static ssize_t muic_sysfs_registers_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	char mesg[256] = "";
	int ret;

	MUIC_PDATA_FUNC_MULTI_PARAM(pdata->sysfs_cb.get_register, pdata->drv_data, mesg, &ret);
	pr_info("%s:%s\n", __func__, mesg);
	return sprintf(buf, "%s\n", mesg);
}
#endif

static ssize_t muic_sysfs_usb_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	static unsigned long switch_slot_time;
	int mdev = 0;

	MUIC_PDATA_FUNC(pdata->sysfs_cb.get_attached_dev, pdata->drv_data, &mdev);

	pr_info("%s attached_dev:%d\n", __func__, mdev);

	if (printk_timed_ratelimit(&switch_slot_time, 5000))
		pr_info("%s attached_dev(%d)\n",
			__func__, mdev);

	switch (mdev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		return sprintf(buf, "USB_STATE_CONFIGURED\n");
	default:
		break;
	}

	return sprintf(buf, "USB_STATE_NOTCONFIGURED\n");
}

#if IS_ENABLED(CONFIG_USB_HOST_NOTIFY)
static ssize_t muic_sysfs_otg_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	int ret;

	MUIC_PDATA_FUNC(pdata->sysfs_cb.get_otg_test, pdata->drv_data, &ret);

	pr_info("%s ret=%d\n", __func__, ret);
	return sprintf(buf, "%d\n", ret);
}

static ssize_t muic_sysfs_set_otg_test(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	int ret = 0;

	pr_info("%s buf:%s\n", __func__, buf);

	/*
	 *	The otg_test is set 0 durring the otg test. Not 1 !!!
	 */

	if (!strncmp(buf, "0", 1)) {
		MUIC_PDATA_FUNC_MULTI_PARAM(pdata->sysfs_cb.set_otg_test, pdata->drv_data, 1, &ret);
	} else if (!strncmp(buf, "1", 1)) {
		MUIC_PDATA_FUNC_MULTI_PARAM(pdata->sysfs_cb.set_otg_test, pdata->drv_data, 0, &ret);
	} else {
		pr_info("%s Wrong command\n", __func__);
		return count;
	}

	return count;
}
#endif

static ssize_t muic_sysfs_attached_dev_show(struct device *dev,
	 struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	int mdev = 0;

	MUIC_PDATA_FUNC(pdata->sysfs_cb.get_attached_dev, pdata->drv_data, &mdev);

	pr_info("%s attached_dev:%d\n", __func__, mdev);

	switch (mdev) {
	case ATTACHED_DEV_NONE_MUIC:
		return sprintf(buf, "No VPS\n");
	case ATTACHED_DEV_USB_MUIC:
		return sprintf(buf, "USB\n");
	case ATTACHED_DEV_CDP_MUIC:
		return sprintf(buf, "CDP\n");
	case ATTACHED_DEV_OTG_MUIC:
		return sprintf(buf, "OTG\n");
	case ATTACHED_DEV_TA_MUIC:
		return sprintf(buf, "TA\n");
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		return sprintf(buf, "JIG UART OFF\n");
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
		return sprintf(buf, "JIG UART OFF/VB\n");
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
		return sprintf(buf, "JIG UART ON\n");
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:
		return sprintf(buf, "JIG UART ON/VB\n");
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
		return sprintf(buf, "JIG USB OFF\n");
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		return sprintf(buf, "JIG USB ON\n");
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
		return sprintf(buf, "DESKDOCK\n");
	case ATTACHED_DEV_AUDIODOCK_MUIC:
		return sprintf(buf, "AUDIODOCK\n");
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
		return sprintf(buf, "PS CABLE\n");
	case ATTACHED_DEV_AFC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_DISABLED_MUIC:
	case ATTACHED_DEV_QC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
		return sprintf(buf, "AFC Charger\n");
	case ATTACHED_DEV_FACTORY_UART_MUIC:
		return sprintf(buf, "FACTORY UART\n");
	case ATTACHED_DEV_POGO_DOCK_MUIC:
		return sprintf(buf, "POGO Dock\n");
	case ATTACHED_DEV_POGO_DOCK_5V_MUIC:
		return sprintf(buf, "POGO Dock/5V\n");
	case ATTACHED_DEV_POGO_DOCK_9V_MUIC:
		return sprintf(buf, "POGO Dock/9V\n");
	default:
		break;
	}

	return sprintf(buf, "UNKNOWN\n");
}

static ssize_t muic_sysfs_audio_path_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("%s\n", __func__);
	return 0;
}

static ssize_t muic_sysfs_set_audio_path(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);

	pr_info("%s\n", __func__);
	MUIC_PDATA_VOID_FUNC(pdata->sysfs_cb.set_audio_path, pdata->drv_data);

	return count;
}

static ssize_t muic_sysfs_apo_factory_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	const char *mode;

	/* true: Factory mode, false: not Factory mode */
	if (pdata->is_factory_start)
		mode = "FACTORY_MODE";
	else
		mode = "NOT_FACTORY_MODE";

	pr_info("%s : %s\n",
		__func__, mode);

	return sprintf(buf, "%s\n", mode);
}

static ssize_t muic_sysfs_set_apo_factory(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);

	pr_info("%s\n", __func__);
	/* "FACTORY_START": factory mode */
	if (!strncmp(buf, "FACTORY_START", 13)) {
		pdata->is_factory_start = true;
		MUIC_PDATA_VOID_FUNC(pdata->sysfs_cb.set_apo_factory,
				pdata->drv_data);
	} else
		pr_info("%s Wrong command\n",  __func__);

	return count;
}

static ssize_t muic_sysfs_vbus_value_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	int val = 0;

	pr_info("%s\n", __func__);
	MUIC_PDATA_FUNC(pdata->sysfs_cb.get_vbus_value, pdata->drv_data, &val);

	switch (val) {
	case 0 ... 3:
		val = 0;
		break;
	case 4 ... 6:
		val = 5;
		break;
	case 7 ... 9:
		val = 9;
		break;
	}
	pr_info("%s val=%d-\n", __func__, val);
	return sprintf(buf, "%d\n", val);
}

#if IS_ENABLED(CONFIG_MUIC_HV)
static ssize_t muic_sysfs_afc_disable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);

	pr_info("%s\n", __func__);
	if (pdata->afc_disable) {
		pr_info("%s AFC DISABLE\n", __func__);
		return sprintf(buf, "1\n");
	}

	pr_info("%s AFC ENABLE", __func__);
	return sprintf(buf, "0\n");
}

static ssize_t muic_sysfs_set_afc_disable(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	bool curr_val = pdata->afc_disable;
	int ret = 0;
	int param_val = 0;
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	union power_supply_propval psy_val;
#endif

	pr_info("%s+\n", __func__);
	if (!strncasecmp(buf, "1", 1)) {
		pdata->afc_disable = true;
		muic_afc_request_cause_clear();
	} else if (!strncasecmp(buf, "0", 1)) {
		pdata->afc_disable = false;
	} else {
		pr_warn("%s invalid value\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	param_val = pdata->afc_disable ? '1' : '0';

#if IS_BUILTIN(CONFIG_MUIC_NOTIFIER)
#if defined(CONFIG_USB_DWC3_MSM)
	ret = sec_set_param(param_index_afc_disable, &param_val);
#else
	ret = sec_set_param(CM_OFFSET + 1, (char)param_val);
#endif
	if ((!IS_ENABLED(CONFIG_USB_DWC3_MSM) && ret < 0) ||
			(IS_ENABLED(CONFIG_USB_DWC3_MSM) && ret == false)) {
		pr_err("%s:set_param failed - %02x:%02x(%d)\n", __func__,
			param_val, curr_val, ret);

		pdata->afc_disable = curr_val;
		ret = -EIO;
		goto err;
	}
#endif

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	psy_val.intval = param_val;
	psy_do_property("battery", set, POWER_SUPPLY_EXT_PROP_HV_DISABLE, psy_val);
#endif

	pr_info("%s afc_disable(%d)\n", __func__, pdata->afc_disable);

	if (curr_val != pdata->afc_disable)
		MUIC_PDATA_VOID_FUNC(pdata->sysfs_cb.set_afc_disable, pdata->drv_data);

	pr_info("%s ret=%d-\n", __func__, ret);
	return count;
err:
	pr_info("%s ret=%d-\n", __func__, ret);
	return ret;
}

static ssize_t muic_sysfs_set_afc_set_voltage(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	int ret = 0;

	pr_info("%s+\n", __func__);
	if (!strncasecmp(buf, "5V", 2)) {
		MUIC_PDATA_FUNC_MULTI_PARAM(pdata->sysfs_cb.afc_set_voltage,
			pdata->drv_data, 5, &ret);
	} else if (!strncasecmp(buf, "9V", 2)) {
		MUIC_PDATA_FUNC_MULTI_PARAM(pdata->sysfs_cb.afc_set_voltage,
			pdata->drv_data, 9, &ret);
	} else {
		pr_warn("%s invalid value : %s\n", __func__, buf);
	}

	pr_info("%s ret=%d-\n", __func__, ret);
	return count;
}
#endif /* CONFIG_MUIC_HV */

#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
static ssize_t hiccup_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	int ret;

	pr_info("%s+\n", __func__);
	MUIC_PDATA_FUNC(pdata->sysfs_cb.get_hiccup,
					pdata->drv_data, &ret);

	pr_info("%s ret=%d-\n", __func__, ret);
	if (ret)
		return sprintf(buf, "ENABLE\n");
	else
		return sprintf(buf, "DISABLE\n");
}

static ssize_t hiccup_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct muic_platform_data *pdata = dev_get_drvdata(dev);
	int ret = 0;

	pr_info("%s+\n", __func__);
	if (!strncasecmp(buf, "DISABLE", 7)) {
#if IS_ENABLED(CONFIG_IF_CB_MANAGER) && IS_ENABLED(CONFIG_HICCUP_CC_DISABLE)
		usbpd_cc_control_command(pdata->man, 0);
#endif
		MUIC_PDATA_FUNC_MULTI_PARAM(pdata->sysfs_cb.set_hiccup,
					pdata->drv_data, MUIC_DISABLE, &ret);
	} else {
		pr_warn("%s invalid com : %s\n", __func__, buf);
	}

	pr_info("%s ret=%d-\n", __func__, ret);
	return count;
}
#endif /* CONFIG_HICCUP_CHARGER */

static DEVICE_ATTR(uart_en, 0664, muic_sysfs_uart_en_show,
					muic_sysfs_set_uart_en);
static DEVICE_ATTR(uart_sel, 0664, muic_sysfs_uart_sel_show,
					muic_sysfs_set_uart_sel);
static DEVICE_ATTR(usb_en, 0664,
		muic_sysfs_usb_en_show,
		muic_sysfs_set_usb_en);
static DEVICE_ATTR(usb_sel, 0664, muic_sysfs_usb_sel_show,
					muic_sysfs_set_usb_sel);
static DEVICE_ATTR(adc, 0444, muic_sysfs_adc_show, NULL);

#if IS_ENABLED(DEBUG_MUIC)
static DEVICE_ATTR(mansw, 0444, muic_sysfs_mansw_show, NULL);
static DEVICE_ATTR(dump_registers, 0444, muic_sysfs_registers_show, NULL);
static DEVICE_ATTR(int_status, 0444, muic_sysfs_interrupt_status_show, NULL);
#endif
static DEVICE_ATTR(usb_state, 0444, muic_sysfs_usb_state_show, NULL);
#if IS_ENABLED(CONFIG_USB_HOST_NOTIFY)
static DEVICE_ATTR(otg_test, 0664,
		muic_sysfs_otg_test_show, muic_sysfs_set_otg_test);
#endif
static DEVICE_ATTR(attached_dev, 0444, muic_sysfs_attached_dev_show, NULL);
static DEVICE_ATTR(audio_path, 0664,
		muic_sysfs_audio_path_show, muic_sysfs_set_audio_path);
static DEVICE_ATTR(apo_factory, 0664,
		muic_sysfs_apo_factory_show,
		muic_sysfs_set_apo_factory);
static DEVICE_ATTR(vbus_value, 0444, muic_sysfs_vbus_value_show, NULL);
#if IS_ENABLED(CONFIG_MUIC_HV)
static DEVICE_ATTR(afc_disable, 0664,
		muic_sysfs_afc_disable_show, muic_sysfs_set_afc_disable);
static DEVICE_ATTR(afc_set_voltage, 0220,
		NULL, muic_sysfs_set_afc_set_voltage);
#endif /* CONFIG_MUIC_HV */
#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
static DEVICE_ATTR_RW(hiccup);
#endif /* CONFIG_HICCUP_CHARGER */

static struct attribute *muic_sysfs_attributes[] = {
	&dev_attr_uart_en.attr,
	&dev_attr_uart_sel.attr,
	&dev_attr_usb_en.attr,
	&dev_attr_usb_sel.attr,
	&dev_attr_adc.attr,
#if IS_ENABLED(DEBUG_MUIC)
	&dev_attr_mansw.attr,
	&dev_attr_dump_registers.attr,
	&dev_attr_int_status.attr,
#endif
	&dev_attr_usb_state.attr,
#if IS_ENABLED(CONFIG_USB_HOST_NOTIFY)
	&dev_attr_otg_test.attr,
#endif
	&dev_attr_attached_dev.attr,
	&dev_attr_audio_path.attr,
	&dev_attr_apo_factory.attr,
	&dev_attr_vbus_value.attr,
#if IS_ENABLED(CONFIG_MUIC_HV)
	&dev_attr_afc_disable.attr,
	&dev_attr_afc_set_voltage.attr,
#endif /* CONFIG_MUIC_HV */
#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
	&dev_attr_hiccup.attr,
#endif /* CONFIG_HICCUP_CHARGER */
	NULL
};

static const struct attribute_group muic_sysfs_group = {
	.attrs = muic_sysfs_attributes,
};

int muic_sysfs_init(struct muic_platform_data *pdata)
{
	int ret = 0;

	mutex_init(&pdata->sysfs_mutex);

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	if (pdata->switch_device == NULL)
		pdata->switch_device = switch_device;

	ret = sysfs_create_group(&pdata->switch_device->kobj, &muic_sysfs_group);
	if (ret) {
		pr_err("failed to create sysfs\n");
		return ret;
	} else
		kobject_uevent(&pdata->switch_device->kobj, KOBJ_CHANGE);

	dev_set_drvdata(pdata->switch_device, pdata);
#endif

	return ret;
}
EXPORT_SYMBOL_GPL(muic_sysfs_init);

void muic_sysfs_deinit(struct muic_platform_data *pdata)
{
	if (pdata->switch_device)
		sysfs_remove_group(&pdata->switch_device->kobj, &muic_sysfs_group);
}
EXPORT_SYMBOL_GPL(muic_sysfs_deinit);

