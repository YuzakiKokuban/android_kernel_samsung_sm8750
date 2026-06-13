/*
 * max77775-muic.c - MUIC driver for the Maxim 77775
 *
 *  Copyright (C) 2015 Samsung Electronics
 *  Insun Choi <insun77.choi@samsung.com>
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

 #define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/workqueue.h>
#if IS_ENABLED(CONFIG_ANDROID_SWITCH) || IS_ENABLED(CONFIG_SWITCH)
#include <linux/switch.h>
#endif
/* MUIC header file */
#include <linux/mfd/max77775_log.h>
#include <linux/mfd/max77775.h>
#include <linux/mfd/max77775-private.h>
#include <linux/muic/common/muic.h>
#include <linux/usb/typec/maxim/max77775-muic.h>
#include <linux/usb/typec/maxim/max77775.h>
#include <linux/usb/typec/maxim/max77775_usbc.h>

#if IS_ENABLED(CONFIG_MUIC_SUPPORT_PDIC)
#include <linux/usb/typec/common/pdic_notifier.h>
#endif

#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/common/muic_notifier.h>
#endif /* CONFIG_MUIC_NOTIFIER */

#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
#include <linux/vbus_notifier.h>
#endif /* CONFIG_VBUS_NOTIFIER */

#if IS_ENABLED(CONFIG_USB_EXTERNAL_NOTIFY)
#include <linux/usb_notify.h>
#endif

#include <linux/usb/typec/common/pdic_param.h>

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
#include <linux/battery/sec_battery_common.h>
#endif

struct max77775_muic_data *g_muic_data;

/* for the bringup, should be fixed */
void __init_usbc_cmd_data(usbc_cmd_data *cmd_data)
{
	pr_warn("%s is not defined!\n", __func__);
}
int __max77775_usbc_opcode_write(struct max77775_usbc_platform_data *usbc_data,
	usbc_cmd_data *write_op)
{
	pr_warn("%s is not defined!\n", __func__);
	return 0;
}
void init_usbc_cmd_data(usbc_cmd_data *cmd_data)
	__attribute__((weak, alias("__init_usbc_cmd_data")));
int max77775_usbc_opcode_write(struct max77775_usbc_platform_data *usbc_data,
	usbc_cmd_data *write_op)
	__attribute__((weak, alias("__max77775_usbc_opcode_write")));

static bool debug_en_vps;
static void max77775_muic_detect_dev(struct max77775_muic_data *muic_data, int irq);

struct max77775_muic_vps_data {
	int				adc;
	int				vbvolt;
	int				chgtyp;
	int				muic_switch;
	const char			*vps_name;
	const muic_attached_dev_t	attached_dev;
};

static int max77775_muic_read_reg
		(struct i2c_client *i2c, u8 reg, u8 *value)
{
	int ret = max77775_read_reg(i2c, reg, value);

	return ret;
}

#if 0
static int max77775_muic_write_reg
		(struct i2c_client *i2c, u8 reg, u8 value, bool debug_en)
{
	int ret = max77775_write_reg(i2c, reg, value);

	if (debug_en)
		md75_info_usb("%s Reg[0x%02x]: 0x%02x\n",
				__func__, reg, value);

	return ret;
}
#endif

#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
static void max77775_muic_handle_vbus(struct max77775_muic_data *muic_data)
{
	int vbvolt = muic_data->status3 & BC_STATUS_VBUSDET_MASK;
	vbus_status_t status = (vbvolt > 0) ?
		STATUS_VBUS_HIGH : STATUS_VBUS_LOW;

	md75_info_usb("%s <%d>\n", __func__, status);

	vbus_notifier_handle(status);
}
#endif

static const struct max77775_muic_vps_data muic_vps_table[] = {
	{
		.adc		= MAX77775_UIADC_523K,
		.vbvolt		= VB_DONTCARE,
		.chgtyp		= CHGTYP_DONTCARE,
		.muic_switch	= COM_UART,
		.vps_name	= "JIG UART OFF",
		.attached_dev	= ATTACHED_DEV_JIG_UART_OFF_MUIC,
	},
	{
		.adc		= MAX77775_UIADC_619K,
		.vbvolt		= VB_LOW,
		.chgtyp		= CHGTYP_NO_VOLTAGE,
		.muic_switch	= COM_UART,
		.vps_name	= "JIG UART ON",
		.attached_dev	= ATTACHED_DEV_JIG_UART_ON_MUIC,
	},
	{
		.adc		= MAX77775_UIADC_619K,
		.vbvolt		= VB_HIGH,
		.chgtyp		= CHGTYP_DONTCARE,
		.muic_switch	= COM_UART,
		.vps_name	= "JIG UART ON/VB",
		.attached_dev	= ATTACHED_DEV_JIG_UART_ON_VB_MUIC,
	},
	{
		.adc		= MAX77775_UIADC_255K,
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		.vbvolt		= VB_DONTCARE,
#else
		.vbvolt		= VB_HIGH,
#endif
		.chgtyp		= CHGTYP_DONTCARE,
		.muic_switch	= COM_USB,
		.vps_name	= "JIG USB OFF",
		.attached_dev	= ATTACHED_DEV_JIG_USB_OFF_MUIC,
	},
	{
		.adc		= MAX77775_UIADC_301K,
		.vbvolt		= VB_DONTCARE,
		.chgtyp		= CHGTYP_DONTCARE,
		.muic_switch	= COM_USB,
		.vps_name	= "JIG USB ON",
		.attached_dev	= ATTACHED_DEV_JIG_USB_ON_MUIC,
	},
	{
		.adc		= MAX77775_UIADC_OPEN,
		.vbvolt		= VB_HIGH,
		.chgtyp		= CHGTYP_DEDICATED_CHARGER,
		.muic_switch	= COM_OPEN,
		.vps_name	= "TA",
		.attached_dev	= ATTACHED_DEV_TA_MUIC,
	},
	{
		.adc		= MAX77775_UIADC_OPEN,
		.vbvolt		= VB_HIGH,
		.chgtyp		= CHGTYP_USB,
		.muic_switch	= COM_USB,
		.vps_name	= "USB",
		.attached_dev	= ATTACHED_DEV_USB_MUIC,
	},
	{
		.adc		= MAX77775_UIADC_OPEN,
		.vbvolt		= VB_HIGH,
		.chgtyp		= CHGTYP_TIMEOUT_OPEN,
		.muic_switch	= COM_USB,
		.vps_name	= "DCD Timeout",
		.attached_dev	= ATTACHED_DEV_TIMEOUT_OPEN_MUIC,
	},
	{
		.adc		= MAX77775_UIADC_OPEN,
		.vbvolt		= VB_HIGH,
		.chgtyp		= CHGTYP_CDP,
		.muic_switch	= COM_USB,
		.vps_name	= "CDP",
		.attached_dev	= ATTACHED_DEV_CDP_MUIC,
	},
	{
		.adc		= MAX77775_UIADC_GND,
		.vbvolt		= VB_DONTCARE,
		.chgtyp		= CHGTYP_DONTCARE,
		.muic_switch	= COM_USB,
		.vps_name	= "OTG",
		.attached_dev	= ATTACHED_DEV_OTG_MUIC,
	},
	{
		.adc		= MAX77775_UIADC_OPEN,
		.vbvolt		= VB_HIGH,
		.chgtyp		= CHGTYP_UNOFFICIAL_CHARGER,
		.muic_switch	= COM_OPEN,
		.vps_name	= "Unofficial TA",
		.attached_dev	= ATTACHED_DEV_UNOFFICIAL_TA_MUIC,
	},
#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
	{
		.adc		= MAX77775_UIADC_OPEN,
		.vbvolt		= VB_HIGH,
		.chgtyp		= CHGTYP_HICCUP_MODE,
		.muic_switch	= COM_OPEN,
		.vps_name	= "Hiccup mode",
		.attached_dev	= ATTACHED_DEV_HICCUP_MUIC,
	},
#endif
#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC)
	{
		.adc		= MAX77775_UIADC_OPEN,
		.vbvolt		= VB_HIGH,
		.chgtyp		= CHGTYP_DEDICATED_CHARGER,
		.muic_switch	= COM_OPEN,
		.vps_name	= "AFC Charger",
		.attached_dev	= ATTACHED_DEV_AFC_CHARGER_9V_MUIC,
	},
	{
		.adc		= MAX77775_UIADC_OPEN,
		.vbvolt		= VB_HIGH,
		.chgtyp		= CHGTYP_DEDICATED_CHARGER,
		.muic_switch	= COM_OPEN,
		.vps_name	= "AFC Charger",
		.attached_dev	= ATTACHED_DEV_AFC_CHARGER_5V_MUIC,
	},
	{
		.adc		= MAX77775_UIADC_OPEN,
		.vbvolt		= VB_HIGH,
		.chgtyp		= CHGTYP_DEDICATED_CHARGER,
		.muic_switch	= COM_OPEN,
		.vps_name	= "QC Charger",
		.attached_dev	= ATTACHED_DEV_QC_CHARGER_9V_MUIC,
	},
	{
		.adc		= MAX77775_UIADC_OPEN,
		.vbvolt		= VB_HIGH,
		.chgtyp		= CHGTYP_DEDICATED_CHARGER,
		.muic_switch	= COM_OPEN,
		.vps_name	= "QC Charger",
		.attached_dev	= ATTACHED_DEV_QC_CHARGER_5V_MUIC,
	},
#endif
};

static int muic_lookup_vps_table(muic_attached_dev_t new_dev,
	struct max77775_muic_data *muic_data)
{
	int i;
	struct i2c_client *i2c = muic_data->i2c;
	u8 reg_data;
	const struct max77775_muic_vps_data *tmp_vps;

	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_USBC_STATUS2, &reg_data);
	reg_data = reg_data & USBC_STATUS2_SYSMSG_MASK;
	md75_info_usb("%s Last sysmsg = 0x%02x\n", __func__, reg_data);

	for (i = 0; i < (int)ARRAY_SIZE(muic_vps_table); i++) {
		tmp_vps = &(muic_vps_table[i]);

		if (tmp_vps->attached_dev != new_dev)
			continue;

		md75_info_usb("%s (%d) vps table match found at i(%d), %s\n",
				__func__, new_dev, i,
				tmp_vps->vps_name);

		return i;
	}

	md75_info_usb("%s can't find (%d) on vps table\n",
			__func__, new_dev);

	return -ENODEV;
}

static void max77775_switch_path(struct max77775_muic_data *muic_data,
	u8 reg_val)
{
	struct max77775_usbc_platform_data *usbc_pdata = muic_data->usbc_pdata;
	usbc_cmd_data write_data;

	md75_info_usb("%s value(0x%x)\n", __func__, reg_val);

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	if (muic_data->usbc_pdata->fac_water_enable) {
		md75_info_usb("%s fac_water_enable(%d), skip\n", __func__,
			muic_data->usbc_pdata->fac_water_enable);
		return;
	}
#endif

	init_usbc_cmd_data(&write_data);
	write_data.opcode = COMMAND_CONTROL1_WRITE;
	write_data.write_length = 2;
	write_data.write_data[0] = reg_val & NOBCCOMP_USBAUTODET_MASK;
	write_data.write_data[1] = reg_val & COMSW_MASK;
	write_data.read_length = 0;

	max77775_usbc_opcode_write(usbc_pdata, &write_data);
}

static void com_to_open(struct max77775_muic_data *muic_data)
{
	u8 reg_val;

	md75_info_usb("%s\n", __func__);

	reg_val = COM_OPEN;

	/* write command - switch */
	max77775_switch_path(muic_data, reg_val);
}

static int com_to_usb_ap(struct max77775_muic_data *muic_data)
{
	u8 reg_val;
	int ret = 0;

	md75_info_usb("%s\n", __func__);
#if IS_ENABLED(CONFIG_MUIC_POGO)
	if (muic_data->pogo_adc ==ADC_HMT) {
		pr_info("%s: pogo adc is 49.9K just return\n", __func__);
		return ret;
	}
#endif

	reg_val = COM_USB;

	/* write command - switch */
	max77775_switch_path(muic_data, reg_val);

	return ret;
}

#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
static void max77775_muic_hiccup_clear(struct max77775_muic_data *muic_data)
{
	struct max77775_usbc_platform_data *usbc_pdata = muic_data->usbc_pdata;
	usbc_cmd_data clear_data;

	clear_data.opcode = OPCODE_HICCUP_ENABLE;
	clear_data.write_length = 1;
	clear_data.write_data[0] = 0x0; /* Clear SW_CTRL1(open/open) and FC_CTRL */
	clear_data.read_length = 4;

	md75_info_usb("%s\n", __func__);

	max77775_usbc_opcode_write(usbc_pdata, &clear_data);
}

static void max77775_muic_hiccup_on(struct max77775_muic_data *muic_data)
{
	struct max77775_usbc_platform_data *usbc_pdata = muic_data->usbc_pdata;
	usbc_cmd_data write_data;

	write_data.opcode = OPCODE_HICCUP_ENABLE;
	write_data.write_length = 1;
	write_data.write_data[0] = 0x1; /* Turn On SW_CTRL1 and FC_CTRL for hiccup */
	write_data.read_length = 4;

	md75_info_usb("%s: set DP/DN to GND\n", __func__);
	max77775_usbc_opcode_write(usbc_pdata, &write_data);
}
#endif

static void com_to_uart_ap(struct max77775_muic_data *muic_data)
{
	u8 reg_val;
	if ((muic_data->pdata->opmode == OPMODE_MUIC) && muic_data->pdata->rustproof_on)
		reg_val = COM_OPEN;
	else
		reg_val = COM_UART;

	md75_info_usb("%s(%d)\n", __func__, reg_val);

	/* write command - switch */
	max77775_switch_path(muic_data, reg_val);
}

static void com_to_uart_cp(struct max77775_muic_data *muic_data)
{
	u8 reg_val;

	if ((muic_data->pdata->opmode == OPMODE_MUIC) && muic_data->pdata->rustproof_on)
		reg_val = COM_OPEN;
	else
		reg_val = COM_UART_CP;

	md75_info_usb("%s(%d)\n", __func__, reg_val);

	/* write command - switch */
	max77775_switch_path(muic_data, reg_val);
}

static int write_vps_regs(struct max77775_muic_data *muic_data,
			muic_attached_dev_t new_dev)
{
	const struct max77775_muic_vps_data *tmp_vps;
	int vps_index;
	u8 prev_switch;

	vps_index = muic_lookup_vps_table(muic_data->attached_dev, muic_data);
	if (vps_index < 0) {
		md75_info_usb("%s: prev cable is none.\n", __func__);
		prev_switch = COM_OPEN;
	} else {
		/* Prev cable information. */
		tmp_vps = &(muic_vps_table[vps_index]);
		prev_switch = tmp_vps->muic_switch;
	}

	if (prev_switch == muic_data->switch_val)
		md75_info_usb("%s Duplicated(0x%02x), just ignore\n",
			__func__, muic_data->switch_val);
#if 0
	else {
		/* write command - switch */
		max77775_switch_path(muic_data, muic_data->switch_val);
	}
#endif

	return 0;
}

/* muic uart path control function */
static int switch_to_ap_uart(struct max77775_muic_data *muic_data,
						muic_attached_dev_t new_dev)
{
	int ret = 0;

	switch (new_dev) {
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:
		com_to_uart_ap(muic_data);
		break;
	default:
		pr_warn("%s current attached is (%d) not Jig UART Off\n",
			__func__, muic_data->attached_dev);
		break;
	}

	return ret;
}

static int switch_to_cp_uart(struct max77775_muic_data *muic_data,
						muic_attached_dev_t new_dev)
{
	int ret = 0;

	switch (new_dev) {
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:
		com_to_uart_cp(muic_data);
		break;
	default:
		pr_warn("%s current attached is (%d) not Jig UART Off\n",
			__func__, muic_data->attached_dev);
		break;
	}

	return ret;
}

void max77775_muic_enable_detecting_short(struct max77775_muic_data *muic_data)
{

	struct max77775_usbc_platform_data *usbc_pdata = muic_data->usbc_pdata;
	usbc_cmd_data write_data;

	md75_info_usb("%s\n", __func__);

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_SAMSUNG_CCSBU_SHORT;
	write_data.write_length = 1;
	/*
	 * bit 0: Enable detecting vbus-cc short
	 * bit 1: Enable detecting sbu-gnd short
	 * bit 2: Enable detecting vbus-sbu short
	 */
#if !defined(CONFIG_SEC_FACTORY)
	write_data.write_data[0] = 0x7;
#else
	/* W/A, in factory mode, sbu-gnd short disable */
	write_data.write_data[0] = 0x5;
#endif
	write_data.read_length = 1;

	max77775_usbc_opcode_write(usbc_pdata, &write_data);

}

static void max77775_muic_dp_reset(struct max77775_muic_data *muic_data)
{
	struct max77775_usbc_platform_data *usbc_pdata = muic_data->usbc_pdata;
	usbc_cmd_data update_data;

	md75_info_usb("%s\n", __func__);

	init_usbc_cmd_data(&update_data);
	update_data.opcode = COMMAND_BC_CTRL2_READ;
	update_data.mask = BC_CTRL2_DPDNMan_MASK | BC_CTRL2_DPDrv_MASK;
	update_data.val = 0x10;

	max77775_usbc_opcode_update(usbc_pdata, &update_data);
}

static void max77775_muic_enable_chgdet(struct max77775_muic_data *muic_data)
{
	struct max77775_usbc_platform_data *usbc_pdata = muic_data->usbc_pdata;
	usbc_cmd_data update_data;

	md75_info_usb("%s\n", __func__);

	init_usbc_cmd_data(&update_data);
	update_data.opcode = COMMAND_BC_CTRL1_READ;
	update_data.mask = BC_CTRL1_CHGDetEn_MASK | BC_CTRL1_CHGDetMan_MASK;
	update_data.val = 0xff;

	max77775_usbc_opcode_update(usbc_pdata, &update_data);
}

#if defined(CONFIG_MUIC_POGO_DISABLE_TYPEC)
static void max77775_muic_disable_chgdet(struct max77775_muic_data *muic_data)
{
	struct max77775_usbc_platform_data *usbc_pdata = muic_data->usbc_pdata;
	usbc_cmd_data update_data;

	pr_info("%s\n", __func__);

	init_usbc_cmd_data(&update_data);
	update_data.opcode = COMMAND_BC_CTRL1_READ;
	update_data.mask = BC_CTRL1_CHGDetEn_MASK;
	update_data.val = 0x0;

	max77775_usbc_opcode_update(usbc_pdata, &update_data);
}
#endif

static u8 max77775_muic_get_adc_value(struct max77775_muic_data *muic_data)
{
	u8 status;
	u8 adc = MAX77775_UIADC_ERROR;
	int ret;

	ret = max77775_muic_read_reg(muic_data->i2c,
			MAX77775_USBC_REG_USBC_STATUS1, &status);
	if (ret)
		md75_err_usb("%s fail to read muic reg(%d)\n",
				__func__, ret);
	else
		adc = status & USBC_STATUS1_UIADC_MASK;

	return adc;
}

static u8 max77775_muic_get_vbadc_value(struct max77775_muic_data *muic_data)
{
	u8 status;
	u8 vbadc = 0;
	int ret;

	ret = max77775_muic_read_reg(muic_data->i2c,
			MAX77775_USBC_REG_USBC_STATUS1, &status);
	if (ret)
		md75_err_usb("%s fail to read muic reg(%d)\n",
				__func__, ret);
	else
		vbadc = (status & USBC_STATUS1_VBADC_MASK) >> USBC_STATUS1_VBADC_SHIFT;

	return vbadc;
}

static ssize_t max77775_muic_show_uart_sel(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;
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

	md75_info_usb("%s %s", __func__, mode);
	return sprintf(buf, "%s", mode);
}

static ssize_t max77775_muic_set_uart_sel(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;

	mutex_lock(&muic_data->muic_mutex);

	if (!strncasecmp(buf, "AP", 2)) {
		pdata->uart_path = MUIC_PATH_UART_AP;
		switch_to_ap_uart(muic_data, muic_data->attached_dev);
	} else if (!strncasecmp(buf, "CP", 2)) {
		pdata->uart_path = MUIC_PATH_UART_CP;
		switch_to_cp_uart(muic_data, muic_data->attached_dev);
	} else {
		pr_warn("%s invalid value\n", __func__);
	}

	md75_info_usb("%s uart_path(%d)\n", __func__,
			pdata->uart_path);

	mutex_unlock(&muic_data->muic_mutex);

	return count;
}

static ssize_t max77775_muic_show_usb_sel(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;
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

	pr_debug("%s %s", __func__, mode);
	return sprintf(buf, "%s", mode);
}

static ssize_t max77775_muic_set_usb_sel(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;

	if (!strncasecmp(buf, "PDA", 3))
		pdata->usb_path = MUIC_PATH_USB_AP;
	else if (!strncasecmp(buf, "MODEM", 5))
		pdata->usb_path = MUIC_PATH_USB_CP;
	else
		pr_warn("%s invalid value\n", __func__);

	md75_info_usb("%s usb_path(%d)\n", __func__,
			pdata->usb_path);

	return count;
}

static ssize_t max77775_muic_show_uart_en(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;

	if (!pdata->rustproof_on) {
		md75_info_usb("%s UART ENABLE\n", __func__);
		return sprintf(buf, "1\n");
	}

	md75_info_usb("%s UART DISABLE", __func__);
	return sprintf(buf, "0\n");
}

static ssize_t max77775_muic_set_uart_en(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;

	if (!strncasecmp(buf, "1", 1))
		pdata->rustproof_on = false;
	else if (!strncasecmp(buf, "0", 1))
		pdata->rustproof_on = true;
	else
		pr_warn("%s invalid value\n", __func__);

	md75_info_usb("%s uart_en(%d)\n", __func__,
			!pdata->rustproof_on);

	return count;
}

static ssize_t max77775_muic_show_adc(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
	u8 adc;

	adc = max77775_muic_get_adc_value(muic_data);
	md75_info_usb("%s adc(0x%02x)\n", __func__, adc);

	if (adc == MAX77775_UIADC_ERROR) {
		md75_err_usb("%s fail to read adc value\n",
				__func__);
		return sprintf(buf, "UNKNOWN\n");
	}

	switch (adc) {
	case MAX77775_UIADC_GND:
		adc = 0;
		break;
	case MAX77775_UIADC_255K:
		adc = 0x18;
		break;
	case MAX77775_UIADC_301K:
		adc = 0x19;
		break;
	case MAX77775_UIADC_523K:
		adc = 0x1c;
		break;
	case MAX77775_UIADC_619K:
		adc = 0x1d;
		break;
	case MAX77775_UIADC_OPEN:
		adc = 0x1f;
		break;
	default:
		adc = 0xff;
	}

	return sprintf(buf, "adc: 0x%x\n", adc);
}

static ssize_t max77775_muic_show_usb_state(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);

	pr_debug("%s attached_dev(%d)\n", __func__,
			muic_data->attached_dev);

	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_USB_MUIC:
		return sprintf(buf, "USB_STATE_CONFIGURED\n");
	default:
		break;
	}

	return sprintf(buf, "USB_STATE_NOTCONFIGURED\n");
}

static ssize_t max77775_muic_show_attached_dev(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
	const struct max77775_muic_vps_data *tmp_vps;
	int vps_index;

	vps_index = muic_lookup_vps_table(muic_data->attached_dev, muic_data);
	if (vps_index < 0) {
#if IS_ENABLED(CONFIG_MUIC_POGO)
		if (muic_data->pogo_adc == ADC_HMT)
			return sprintf(buf, "POGO Dock 49.9K\n");
		else if (muic_data->pogo_adc == ADC_INCOMPATIBLE_VZW)
			return sprintf(buf, "POGO Dock 34K\n");
#endif /* CONFIG_MUIC_POGO */
		return sprintf(buf, "No VPS\n");
	}

	tmp_vps = &(muic_vps_table[vps_index]);

#if IS_ENABLED(CONFIG_MUIC_POGO)
	if (muic_data->pogo_adc == ADC_HMT)
		return sprintf(buf, "POGO Dock 49.9K+%s\n", tmp_vps->vps_name);
	else if (muic_data->pogo_adc == ADC_INCOMPATIBLE_VZW)
		return sprintf(buf, "POGO Dock 34K+%s\n", tmp_vps->vps_name);
#endif /* CONFIG_MUIC_POGO */

	return sprintf(buf, "%s\n", tmp_vps->vps_name);
}

static ssize_t max77775_muic_show_otg_test(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
	int ret = -ENODEV;

	if (muic_data->is_otg_test == true)
		ret = 0;
	else
		ret = 1;
	md75_info_usb("%s ret:%d\n", __func__, ret);

	return sprintf(buf, "%d\n", ret);
}

static ssize_t max77775_muic_set_otg_test(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
	int ret = -ENODEV;

	mutex_lock(&muic_data->muic_mutex);

	md75_info_usb("%s buf:%s\n", __func__, buf);
	if (!strncmp(buf, "0", 1)) {
		muic_data->is_otg_test = true;
		ret = 0;
	} else if (!strncmp(buf, "1", 1)) {
		muic_data->is_otg_test = false;
		ret = 1;
	}

	md75_info_usb("%s ret: %d\n", __func__, ret);

	mutex_unlock(&muic_data->muic_mutex);
	return count;
}

static ssize_t max77775_muic_show_apo_factory(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
	const char *mode;

	/* true: Factory mode, false: not Factory mode */
	if (muic_data->is_factory_start)
		mode = "FACTORY_MODE";
	else
		mode = "NOT_FACTORY_MODE";

	md75_info_usb("%s apo factory=%s\n", __func__, mode);

	return sprintf(buf, "%s\n", mode);
}

static ssize_t max77775_muic_set_apo_factory(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
#if IS_ENABLED(CONFIG_SEC_FACTORY)
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
#endif /* CONFIG_SEC_FACTORY */
	const char *mode;

	md75_info_usb("%s buf:%s\n", __func__, buf);

	/* "FACTORY_START": factory mode */
	if (!strncmp(buf, "FACTORY_START", 13)) {
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		muic_data->is_factory_start = true;
#endif /* CONFIG_SEC_FACTORY */
		mode = "FACTORY_MODE";
	} else {
		pr_warn("%s Wrong command\n", __func__);
		return count;
	}

	md75_info_usb("%s apo factory=%s\n", __func__, mode);

	return count;
}

#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC) || defined(CONFIG_SUPPORT_QC30)
static ssize_t max77775_muic_show_afc_disable(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;

	if (pdata->afc_disable) {
		md75_info_usb("%s AFC DISABLE\n", __func__);
		return sprintf(buf, "1\n");
	}

	md75_info_usb("%s AFC ENABLE", __func__);
	return sprintf(buf, "0\n");
}

static ssize_t max77775_muic_set_afc_disable(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;
	int param_val, ret = 0;
	bool curr_val = pdata->afc_disable;
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	union power_supply_propval psy_val;
#endif

	if (!strncasecmp(buf, "1", 1)) {
		/* Disable AFC */
		pdata->afc_disable = true;
		muic_afc_request_cause_clear();
	} else if (!strncasecmp(buf, "0", 1)) {
		/* Enable AFC */
		pdata->afc_disable = false;
	} else {
		pr_warn("%s invalid value\n", __func__);
	}

	param_val = pdata->afc_disable ? '1' : '0';
	md75_info_usb("%s: param_val:%d\n", __func__, param_val);

	if (ret < 0) {
		md75_info_usb("%s:set_param failed - %02x:%02x(%d)\n", __func__,
			param_val, curr_val, ret);

		pdata->afc_disable = curr_val;

		return -EIO;
	} else {
		mutex_lock(&muic_data->afc_lock);
		md75_info_usb("%s: afc_disable:%d (AFC %s)\n", __func__,
			pdata->afc_disable, pdata->afc_disable ? "Disabled" : "Enabled");

		if (pdata->afc_disabled_updated & MAX77775_MUIC_AFC_WORK_PROCESS)
			pdata->afc_disabled_updated |= MAX77775_MUIC_AFC_DISABLE_CHANGE_DURING_WORK;
		else
			max77775_muic_check_afc_disabled(muic_data);
		mutex_unlock(&muic_data->afc_lock);
	}

	md75_info_usb("%s afc_disable(%d)\n", __func__, pdata->afc_disable);

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	psy_val.intval = param_val;
	psy_do_property("battery", set, POWER_SUPPLY_EXT_PROP_HV_DISABLE, psy_val);
#endif

	return count;
}
#endif /* CONFIG_HV_MUIC_MAX77775_AFC */

static ssize_t max77775_muic_show_vbus_value(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);
	u8 vbadc;

	vbadc = max77775_muic_get_vbadc_value(muic_data);
	md75_info_usb("%s vbadc(0x%02x)\n", __func__, vbadc);

	switch (vbadc) {
	case MAX77775_VBADC_3_8V_UNDER:
		vbadc = 0;
		break;
	case MAX77775_VBADC_3_8V_TO_4_5V ... MAX77775_VBADC_6_5V_TO_7_5V:
		vbadc = 5;
		break;
	case MAX77775_VBADC_7_5V_TO_8_5V ... MAX77775_VBADC_8_5V_TO_9_5V:
		vbadc = 9;
		break;
	default:
		vbadc += 3;
	}

	return sprintf(buf, "%d\n", vbadc);
}

#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
static void max77775_muic_reset_hiccup_mode_for_watercable(struct max77775_muic_data *muic_data)
{
	/*Source Connection Status of Moisture Case, 0x0: unplug TA, 0x1:plug TA*/
	if (muic_data->usbc_pdata->ta_conn_status == 0x1)
		max77775_ccic_event_work(muic_data->usbc_pdata,
			PDIC_NOTIFY_DEV_MUIC, PDIC_NOTIFY_ID_WATER_CABLE, 1, 0, 0);
}
#endif
static ssize_t hiccup_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "ENABLE\n");
}

static ssize_t hiccup_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct max77775_muic_data *muic_data = dev_get_drvdata(dev);

	if (!strncasecmp(buf, "DISABLE", 7)) {
		md75_info_usb("%s\n", __func__);
		max77775_pdic_manual_ccopen_request(0);
		max77775_muic_hiccup_clear(muic_data);
		muic_data->is_hiccup_mode = MUIC_HICCUP_MODE_OFF;
#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
		max77775_muic_reset_hiccup_mode_for_watercable(muic_data);
#endif
	} else
		pr_warn("%s invalid com : %s\n", __func__, buf);

	return count;
}
#endif /* CONFIG_HICCUP_CHARGER */

static DEVICE_ATTR(uart_sel, 0664, max77775_muic_show_uart_sel,
		max77775_muic_set_uart_sel);
static DEVICE_ATTR(usb_sel, 0664, max77775_muic_show_usb_sel,
		max77775_muic_set_usb_sel);
static DEVICE_ATTR(uart_en, 0660, max77775_muic_show_uart_en,
		max77775_muic_set_uart_en);
static DEVICE_ATTR(adc, 0444, max77775_muic_show_adc, NULL);
static DEVICE_ATTR(usb_state, 0444, max77775_muic_show_usb_state, NULL);
static DEVICE_ATTR(attached_dev, 0444, max77775_muic_show_attached_dev, NULL);
static DEVICE_ATTR(otg_test, 0664,
		max77775_muic_show_otg_test, max77775_muic_set_otg_test);
static DEVICE_ATTR(apo_factory, 0664,
		max77775_muic_show_apo_factory, max77775_muic_set_apo_factory);
#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC)
static DEVICE_ATTR(afc_disable, 0664,
		max77775_muic_show_afc_disable, max77775_muic_set_afc_disable);
#endif /* CONFIG_HV_MUIC_MAX77775_AFC */
static DEVICE_ATTR(vbus_value, 0444, max77775_muic_show_vbus_value, NULL);
static DEVICE_ATTR(vbus_value_pd, 0444, max77775_muic_show_vbus_value, NULL);
#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
static DEVICE_ATTR_RW(hiccup);
#endif /* CONFIG_HICCUP_CHARGER */

static struct attribute *max77775_muic_attributes[] = {
	&dev_attr_uart_sel.attr,
	&dev_attr_usb_sel.attr,
	&dev_attr_uart_en.attr,
	&dev_attr_adc.attr,
	&dev_attr_usb_state.attr,
	&dev_attr_attached_dev.attr,
	&dev_attr_otg_test.attr,
	&dev_attr_apo_factory.attr,
#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC)
	&dev_attr_afc_disable.attr,
#endif /* CONFIG_HV_MUIC_MAX77775_AFC */
	&dev_attr_vbus_value.attr,
	&dev_attr_vbus_value_pd.attr,
#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
	&dev_attr_hiccup.attr,
#endif /* CONFIG_HICCUP_CHARGER */
	NULL
};

static const struct attribute_group max77775_muic_group = {
	.attrs = max77775_muic_attributes,
};

void max77775_muic_read_register(struct i2c_client *i2c)
{
	const enum max77775_usbc_reg regfile[] = {
		MAX77775_USBC_REG_PRODUCT_ID,
		MAX77775_USBC_REG_USBC_STATUS1,
		MAX77775_USBC_REG_USBC_STATUS2,
		MAX77775_USBC_REG_BC_STATUS,
		MAX77775_USBC_REG_UIC_INT_M,
	};
	u8 val;
	int i, ret;

	md75_info_usb("%s read register--------------\n", __func__);
	for (i = 0; i < (int)ARRAY_SIZE(regfile); i++) {
		ret = max77775_muic_read_reg(i2c, regfile[i], &val);
		if (ret) {
			md75_err_usb("%s fail to read muic reg(0x%02x), ret=%d\n",
					__func__, regfile[i], ret);
			continue;
		}

		md75_info_usb("%s reg(0x%02x)=[0x%02x]\n",
			__func__, regfile[i], val);
	}
	md75_info_usb("%s end register---------------\n", __func__);
}

static int max77775_muic_attach_uart_path(struct max77775_muic_data *muic_data,
					muic_attached_dev_t new_dev)
{
	struct muic_platform_data *pdata = muic_data->pdata;
	int ret = 0;

	md75_info_usb("%s\n", __func__);

	if (pdata->uart_path == MUIC_PATH_UART_AP)
		ret = switch_to_ap_uart(muic_data, new_dev);
	else if (pdata->uart_path == MUIC_PATH_UART_CP)
		ret = switch_to_cp_uart(muic_data, new_dev);
	else
		pr_warn("%s invalid uart_path\n", __func__);

	return ret;
}

static int max77775_muic_handle_detach(struct max77775_muic_data *muic_data, int irq)
{
	int ret = 0;
	muic_attached_dev_t attached_dev = muic_data->attached_dev;

#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC)
	/* Do workaround if Vbusdet goes to low status */
	if (muic_data->is_check_hv && irq == muic_data->irq_vbusdet &&
			(muic_data->status3 & BC_STATUS_VBUSDET_MASK) == 0)
		muic_data->is_check_hv = false;
	muic_data->hv_voltage = 0;
	muic_data->afc_retry = 0;
	muic_data->is_afc_reset = false;
	muic_data->is_skip_bigdata = false;
	muic_data->is_usb_fail = false;
	cancel_delayed_work_sync(&(muic_data->afc_work));
	muic_data->pdata->afc_disabled_updated = MAX77775_MUIC_AFC_STATUS_CLEAR;
	muic_afc_request_cause_clear();
#endif

	if (attached_dev == ATTACHED_DEV_NONE_MUIC) {
		md75_info_usb("%s Duplicated(%d), just ignore\n",
				__func__, attached_dev);
		goto out_without_noti;
	}

	muic_data->dcdtmo_retry = 0;
#if 0
	/* Enable Charger Detection */
	max77775_muic_enable_chgdet(muic_data);
#endif

	muic_lookup_vps_table(attached_dev, muic_data);

	switch (attached_dev) {
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_TA_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
		if ((muic_data->status3 & BC_STATUS_VBUSDET_MASK) > 0) {
			/* W/A for chgtype 0 irq when CC pin is only detached */
			md75_info_usb("%s Vbus is high, keep the current state(%d)\n",
					__func__, attached_dev);
			return 0;
		}
		break;
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_NONE_MUIC:
		com_to_open(muic_data);
		break;
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_OTG_MUIC:
	case ATTACHED_DEV_TIMEOUT_OPEN_MUIC:
		if (muic_data->ccic_info_data.ccic_evt_attached == MUIC_PDIC_NOTI_DETACH)
			com_to_open(muic_data);
		break;
	case ATTACHED_DEV_UNOFFICIAL_ID_MUIC:
		goto out_without_noti;
	default:
		break;
	}

#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
	muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;
	muic_notifier_detach_attached_dev(attached_dev);
#endif /* CONFIG_MUIC_NOTIFIER */

out_without_noti:
	muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;
	return ret;
}

static int max77775_muic_logically_detach(struct max77775_muic_data *muic_data,
						muic_attached_dev_t new_dev)
{
	muic_attached_dev_t attached_dev = muic_data->attached_dev;
	bool force_path_open = true;
	int ret = 0;

	switch (attached_dev) {
	case ATTACHED_DEV_OTG_MUIC:
		break;
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_TIMEOUT_OPEN_MUIC:
		if (new_dev == ATTACHED_DEV_OTG_MUIC) {
			md75_info_usb("%s: data role changed, not detach\n", __func__);
			force_path_open = false;
			goto out;
		}
		break;
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		break;
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:
	case ATTACHED_DEV_UNKNOWN_MUIC:
		if (new_dev == ATTACHED_DEV_JIG_UART_OFF_MUIC ||
				new_dev == ATTACHED_DEV_JIG_UART_OFF_VB_MUIC ||
				new_dev == ATTACHED_DEV_JIG_UART_ON_MUIC ||
				new_dev == ATTACHED_DEV_JIG_UART_ON_VB_MUIC)
			force_path_open = false;
		break;
	case ATTACHED_DEV_TA_MUIC:
#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC)
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_5V_MUIC:
#endif
#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
		if (new_dev == ATTACHED_DEV_HICCUP_MUIC) {
			md75_info_usb("%s hiccup charger, do not logically detach\n", __func__);
			force_path_open = false;
			goto out;
		}
#endif
		break;
	case ATTACHED_DEV_UNOFFICIAL_TA_MUIC:
		break;
#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
	case ATTACHED_DEV_HICCUP_MUIC:
		break;
#endif /* CONFIG_HICCUP_CHARGER */
	case ATTACHED_DEV_NONE_MUIC:
		force_path_open = false;
		goto out;
	default:
		pr_warn("%s try to attach without logically detach\n",
				__func__);
		goto out;
	}

	md75_info_usb("%s attached(%d)!=new(%d), assume detach\n",
			__func__, attached_dev, new_dev);

	muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
	muic_notifier_detach_attached_dev(attached_dev);
#endif /* CONFIG_MUIC_NOTIFIER */

out:
	if (force_path_open)
		com_to_open(muic_data);

	return ret;
}

static int max77775_muic_handle_attach(struct max77775_muic_data *muic_data,
		muic_attached_dev_t new_dev, int irq)
{
	bool notify_skip = false;
#if IS_ENABLED(CONFIG_CCIC_MAX77775)
	int fw_update_state = muic_data->usbc_pdata->max77775->fw_update_state;
#endif /* CONFIG_CCIC_MAX77775 */
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif /* CONFIG_USB_HW_PARAM */
	int ret = 0;

	md75_info_usb("%s\n", __func__);

	if (new_dev == muic_data->attached_dev) {
		switch (new_dev) {
		case ATTACHED_DEV_OTG_MUIC:
		case ATTACHED_DEV_JIG_USB_ON_MUIC:
			/* W/A for setting usb path */
			md75_info_usb("%s:%s Duplicated(%d), Not ignore\n",
				MUIC_DEV_NAME, __func__, muic_data->attached_dev);
			notify_skip = true;
			goto handle_attach;
		default:
			break;
		}

		if (new_dev == ATTACHED_DEV_HICCUP_MUIC)
			goto handle_attach;

		md75_info_usb("%s Duplicated(%d), just ignore\n",
				__func__, muic_data->attached_dev);
		return ret;
	}

	ret = max77775_muic_logically_detach(muic_data, new_dev);
	if (ret)
		pr_warn("%s fail to logically detach(%d)\n",
				__func__, ret);

handle_attach:
	switch (new_dev) {
	case ATTACHED_DEV_OTG_MUIC:
		ret = com_to_usb_ap(muic_data);
		break;
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
		ret = max77775_muic_attach_uart_path(muic_data, new_dev);
		break;
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:
		ret = max77775_muic_attach_uart_path(muic_data, new_dev);
		break;
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_TA_MUIC:
		ret = write_vps_regs(muic_data, new_dev);
		break;
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
		ret = com_to_usb_ap(muic_data);
		break;
	case ATTACHED_DEV_TIMEOUT_OPEN_MUIC:
		md75_info_usb("%s DCD_TIMEOUT system_state = 0x%x\n", __func__, system_state);
#if IS_ENABLED(CONFIG_CCIC_MAX77775)
		if (fw_update_state == FW_UPDATE_END && system_state < SYSTEM_RUNNING) {
			/* TA Reset, D+ gnd */
			max77775_muic_dp_reset(muic_data);

			max77775_muic_enable_chgdet(muic_data);
			goto out;
		}
#endif /* CONFIG_CCIC_MAX77775 */
		ret = com_to_usb_ap(muic_data);
		break;
#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
	case ATTACHED_DEV_HICCUP_MUIC:
		max77775_muic_hiccup_on(muic_data);
		if (!(muic_data->status3 & BC_STATUS_VBUSDET_MASK))
			notify_skip = true;
		break;
#endif /* CONFIG_HICCUP_CHARGER */
	default:
		pr_warn("%s unsupported dev(%d)\n", __func__,
				new_dev);
		ret = -ENODEV;
		goto out;
	}

	muic_data->attached_dev = new_dev;
	if (notify_skip) {
		md75_info_usb("%s: noti\n", __func__);
	} else {
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
		muic_notifier_attach_attached_dev(new_dev);
#endif /* CONFIG_MUIC_NOTIFIER */
	}

#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC)
	if (max77775_muic_check_is_enable_afc(muic_data, new_dev)) {
		/* Maxim's request, wait 1200ms for checking HVDCP */
		md75_info_usb("%s afc work after 1200ms\n", __func__);
		cancel_delayed_work_sync(&(muic_data->afc_work));
		schedule_delayed_work(&(muic_data->afc_work),
				msecs_to_jiffies(1200));
	}
#endif /* CONFIG_HV_MUIC_MAX77775_AFC */

#if defined(CONFIG_USB_HW_PARAM)
	if (o_notify &&
			muic_data->bc1p2_retry_count >= 2 &&
			muic_data->dcdtmo_retry > 0 &&
			muic_data->dcdtmo_retry < muic_data->bc1p2_retry_count &&
			new_dev != ATTACHED_DEV_TIMEOUT_OPEN_MUIC)
		inc_hw_param(o_notify, USB_MUIC_BC12_RETRY_SUCCESS_COUNT);
#endif /* CONFIG_USB_HW_PARAM */

out:
	return ret;
}

static bool muic_check_vps_adc
			(const struct max77775_muic_vps_data *tmp_vps, u8 adc)
{
	bool ret = false;

	if (tmp_vps->adc == adc) {
		ret = true;
		goto out;
	}

	if (tmp_vps->adc == MAX77775_UIADC_DONTCARE)
		ret = true;

out:
	if (debug_en_vps) {
		md75_info_usb("%s vps(%s) adc(0x%02x) ret(%c)\n",
				__func__, tmp_vps->vps_name,
				adc, ret ? 'T' : 'F');
	}

	return ret;
}

static bool muic_check_vps_vbvolt(const struct max77775_muic_vps_data *tmp_vps,
	u8 vbvolt)
{
	bool ret = false;

	if (tmp_vps->vbvolt == vbvolt) {
		ret = true;
		goto out;
	}

	if (tmp_vps->vbvolt == VB_DONTCARE)
		ret = true;

out:
	if (debug_en_vps) {
		pr_debug("%s vps(%s) vbvolt(0x%02x) ret(%c)\n",
				__func__, tmp_vps->vps_name,
				vbvolt, ret ? 'T' : 'F');
	}

	return ret;
}

static bool muic_check_vps_chgtyp(const struct max77775_muic_vps_data *tmp_vps,
	u8 chgtyp)
{
	bool ret = false;

	if (tmp_vps->chgtyp == chgtyp) {
		ret = true;
		goto out;
	}

	if (tmp_vps->chgtyp == CHGTYP_ANY) {
		if (chgtyp > CHGTYP_NO_VOLTAGE) {
			ret = true;
			goto out;
		}
	}

	if (tmp_vps->chgtyp == CHGTYP_DONTCARE)
		ret = true;

out:
	if (debug_en_vps) {
		md75_info_usb("%s vps(%s) chgtyp(0x%02x) ret(%c)\n",
				__func__, tmp_vps->vps_name,
				chgtyp, ret ? 'T' : 'F');
	}

	return ret;
}

static u8 max77775_muic_update_adc_with_rid(struct max77775_muic_data *muic_data,
	u8 adc)
{
	u8 new_adc = adc;

	if (muic_data->pdata->opmode & OPMODE_PDIC) {
		switch (muic_data->ccic_info_data.ccic_evt_rid) {
		case RID_000K:
			new_adc = MAX77775_UIADC_GND;
			break;
		case RID_255K:
			new_adc = MAX77775_UIADC_255K;
			break;
		case RID_301K:
			new_adc = MAX77775_UIADC_301K;
			break;
		case RID_523K:
			new_adc = MAX77775_UIADC_523K;
			break;
		case RID_619K:
			new_adc = MAX77775_UIADC_619K;
			break;
		default:
			new_adc = MAX77775_UIADC_OPEN;
			break;
		}

		if (muic_data->ccic_info_data.ccic_evt_rprd)
			new_adc = MAX77775_UIADC_GND;

		md75_info_usb("%s: adc(0x%x->0x%x) rid(%d) rprd(%d)\n",
			__func__, adc, new_adc,
			muic_data->ccic_info_data.ccic_evt_rid,
			muic_data->ccic_info_data.ccic_evt_rprd);
	}

	return new_adc;
}

static u8 max77775_resolve_chgtyp(struct max77775_muic_data *muic_data, u8 chgtyp,
		u8 spchgtyp, u8 dcdtmo, int irq)
{
	u8 ret = chgtyp;
	u8 ccistat = 0;

	max77775_read_reg(muic_data->i2c, REG_CC_STATUS1, &ccistat);
	ccistat = (ccistat & BIT_CCIStat) >> FFS(BIT_CCIStat);

#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
	/* Check hiccup mode */
	if (muic_data->is_hiccup_mode > MUIC_HICCUP_MODE_OFF) {
		md75_info_usb("%s is_hiccup_mode(%d)\n", __func__, muic_data->is_hiccup_mode);
		return CHGTYP_HICCUP_MODE;
	}
#endif /* CONFIG_HICCUP_CHARGER */

	/* Check DCD timeout */
	if (dcdtmo && chgtyp == CHGTYP_USB &&
			(irq == muic_data->irq_chgtyp || irq == MUIC_IRQ_INIT_DETECT)) {
		if (irq == MUIC_IRQ_INIT_DETECT) {
			ret = CHGTYP_TIMEOUT_OPEN;

			if (ccistat == CCI_500mA) {
				ret = CHGTYP_NO_VOLTAGE;
				muic_data->dcdtmo_retry = muic_data->bc1p2_retry_count;
				md75_info_usb("%s: DCD_TIMEOUT retry in init\n", __func__);
				max77775_muic_enable_chgdet(muic_data);
			}
		} else {
			ret = (muic_data->dcdtmo_retry >= muic_data->bc1p2_retry_count) ? CHGTYP_TIMEOUT_OPEN : CHGTYP_NO_VOLTAGE;
		}
		goto out;
	}

	/* Check Special chgtyp */
	switch (spchgtyp) {
	case PRCHGTYP_SAMSUNG_2A:
	case PRCHGTYP_APPLE_500MA:
	case PRCHGTYP_APPLE_1A:
	case PRCHGTYP_APPLE_2A:
	case PRCHGTYP_APPLE_12W:
		if (chgtyp == CHGTYP_USB || chgtyp == CHGTYP_CDP) {
			ret = CHGTYP_UNOFFICIAL_CHARGER;
			goto out;
		}
		break;
	default:
		break;
	}

out:
	if (ret != chgtyp)
		md75_info_usb("%s chgtyp(0x%x) spchgtyp(0x%x) dcdtmo(0x%x) -> chgtyp(0x%x)",
				__func__, chgtyp, spchgtyp, dcdtmo, ret);

	return ret;
}

muic_attached_dev_t max77775_muic_check_new_dev(struct max77775_muic_data *muic_data,
	int *intr, int irq)
{
	const struct max77775_muic_vps_data *tmp_vps;
	muic_attached_dev_t new_dev = ATTACHED_DEV_NONE_MUIC;
	u8 adc = muic_data->status1 & USBC_STATUS1_UIADC_MASK;
	u8 vbvolt = muic_data->status3 & BC_STATUS_VBUSDET_MASK;
	u8 chgtyp = muic_data->status3 & BC_STATUS_CHGTYP_MASK;
	u8 spchgtyp = (muic_data->status3 & BC_STATUS_PRCHGTYP_MASK) >> BC_STATUS_PRCHGTYP_SHIFT;
	u8 dcdtmo = (muic_data->status3 & BC_STATUS_DCDTMO_MASK) >> BC_STATUS_DCDTMO_SHIFT;
	unsigned long i;

	chgtyp = max77775_resolve_chgtyp(muic_data, chgtyp, spchgtyp, dcdtmo, irq);

	adc = max77775_muic_update_adc_with_rid(muic_data, adc);

	for (i = 0; i < (int)ARRAY_SIZE(muic_vps_table); i++) {
		tmp_vps = &(muic_vps_table[i]);

		if (!(muic_check_vps_adc(tmp_vps, adc)))
			continue;

		if (!(muic_check_vps_vbvolt(tmp_vps, vbvolt)))
			continue;

		if (!(muic_check_vps_chgtyp(tmp_vps, chgtyp)))
			continue;

		md75_info_usb("%s vps table match found at i(%lu), %s\n",
			__func__, i, tmp_vps->vps_name);

		new_dev = tmp_vps->attached_dev;
		muic_data->switch_val = tmp_vps->muic_switch;

		*intr = MUIC_INTR_ATTACH;
		break;
	}

	md75_info_usb("%s %d->%d switch_val[0x%02x]\n", __func__,
		muic_data->attached_dev, new_dev, muic_data->switch_val);

	return new_dev;
}

static void max77775_muic_detect_dev(struct max77775_muic_data *muic_data,
	int irq)
{
	struct i2c_client *i2c = muic_data->i2c;
	muic_attached_dev_t new_dev = ATTACHED_DEV_NONE_MUIC;
	int intr = MUIC_INTR_DETACH;
	u8 status[5];
	u8 adc, vbvolt, chgtyp, spchgtyp, sysmsg, vbadc, dcdtmo, ccstat;
	static unsigned long killer_stamp;
	int ret;
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	int event;
#endif
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif

	ret = max77775_bulk_read(i2c,
		MAX77775_USBC_REG_USBC_STATUS1, 5, status);
	if (ret) {
		md75_err_usb("%s fail to read muic reg(%d)\n",
				__func__, ret);
		return;
	}

	md75_info_usb("%s USBC1:0x%02x, USBC2:0x%02x, BC:0x%02x\n",
			__func__, status[0], status[1], status[2]);

	/* attached status */
	muic_data->status1 = status[0];
	muic_data->status2 = status[1];
	muic_data->status3 = status[2];

	adc = status[0] & USBC_STATUS1_UIADC_MASK;
	sysmsg = status[1] & USBC_STATUS2_SYSMSG_MASK;
	vbvolt = (status[2] & BC_STATUS_VBUSDET_MASK) >> BC_STATUS_VBUSDET_SHIFT;
	chgtyp = status[2] & BC_STATUS_CHGTYP_MASK;
	spchgtyp = (status[2] & BC_STATUS_PRCHGTYP_MASK) >> BC_STATUS_PRCHGTYP_SHIFT;
	vbadc = (status[0] & USBC_STATUS1_VBADC_MASK) >> USBC_STATUS1_VBADC_SHIFT;
	dcdtmo = (status[2] & BC_STATUS_DCDTMO_MASK) >> BC_STATUS_DCDTMO_SHIFT;
	ccstat = (status[4] & BIT_CCStat) >> FFS(BIT_CCStat);

	md75_info_usb("%s adc:0x%x vbvolt:0x%x chgtyp:0x%x spchgtyp:0x%x sysmsg:0x%x vbadc:0x%x dcdtmo:0x%x\n",
		__func__, adc, vbvolt, chgtyp, spchgtyp, sysmsg, vbadc, dcdtmo);

	/* Set the fake_vbus charger type */
	muic_data->fake_chgtyp = chgtyp;

	if (irq == muic_data->irq_vbadc) {
		if (vbadc == MAX77775_VBADC_3_8V_TO_4_5V &&
				ccstat == cc_No_Connection) {
			/* W/A of CC is detached but Vbus is valid(3.8~4.5V) */
			vbvolt = 0;
			muic_data->status3 = muic_data->status3 & ~(BC_STATUS_VBUSDET_MASK);
			md75_info_usb("%s vbadc(0x%x), ccstat(0x%x), set vbvolt to 0 => BC(0x%x)\n",
					__func__, vbadc, ccstat, muic_data->status3);
#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC)
		} else if (vbadc > MAX77775_VBADC_3_8V_TO_4_5V &&
				vbadc <= MAX77775_VBADC_6_5V_TO_7_5V &&
				muic_data->is_afc_reset) {
			muic_data->is_afc_reset = false;
			md75_info_usb("%s afc reset is done\n", __func__);

			switch (muic_data->attached_dev) {
			case ATTACHED_DEV_AFC_CHARGER_5V_MUIC:
			case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
				muic_data->attached_dev = ATTACHED_DEV_AFC_CHARGER_5V_MUIC;
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
				muic_notifier_attach_attached_dev(muic_data->attached_dev);
#endif /* CONFIG_MUIC_NOTIFIER */
				break;
			case ATTACHED_DEV_QC_CHARGER_5V_MUIC:
			case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
				muic_data->attached_dev = ATTACHED_DEV_QC_CHARGER_5V_MUIC;
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
				muic_notifier_attach_attached_dev(muic_data->attached_dev);
#endif /* CONFIG_MUIC_NOTIFIER */
				break;
			default:
				break;
			}
			return;
#endif /* CONFIG_HV_MUIC_MAX77775_AFC */
		} else {
			md75_info_usb("%s vbadc irq(%d), return\n",
					__func__, muic_data->irq_vbadc);
			return;
		}
	}

	if (irq == muic_data->irq_dcdtmo && dcdtmo) {
		muic_data->dcdtmo_retry++;
		md75_info_usb("%s:%s DCD_TIMEOUT retry count: %d\n", MUIC_DEV_NAME, __func__, muic_data->dcdtmo_retry);
	}

	if (!is_lpcharge_pdic_param() && !muic_data->is_factory_start) {
#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
		if ((irq == MUIC_IRQ_PDIC_HANDLER) &&
				(muic_data->ccic_evt_id == PDIC_NOTIFY_ID_WATER)) {
			/* Force hiccup clear once at water state */
			if (muic_data->afc_water_disable)
				max77775_muic_hiccup_clear(muic_data);
		}

		if (muic_data->afc_water_disable && !muic_data->is_hiccup_mode) {
			if (vbvolt > 0) {
				max77775_muic_hiccup_on(muic_data);
			} else {
				/* Clear muic deive type and hiccup at water state (booting with water) */
				if (muic_data->attached_dev != ATTACHED_DEV_NONE_MUIC) {
					md75_info_usb("%s initialize hiccup state and device type(%d) at hiccup booting\n",
							__func__, muic_data->attached_dev);
					muic_notifier_detach_attached_dev(muic_data->attached_dev);
					muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;
					max77775_muic_hiccup_clear(muic_data);
				}
			}
#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
			max77775_muic_handle_vbus(muic_data);
#endif
			return;
		}
#endif /* CONFIG_HICCUP_CHARGER */
	}

	if (irq == muic_data->irq_chgtyp && vbvolt == 0 && chgtyp > 0) {
		killer_stamp = jiffies;
	} else if (irq == muic_data->irq_vbusdet && chgtyp > 0 && vbvolt > 0 &&
			time_before(jiffies, killer_stamp + msecs_to_jiffies(500))) {
		md75_info_usb("%s: this is checking killer, retry bc12\n", __func__);
		max77775_muic_enable_chgdet(muic_data);
#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
		max77775_muic_handle_vbus(muic_data);
#endif
		return;
	} else if (irq == muic_data->irq_vbusdet && vbvolt == 0) {
		killer_stamp = 0;
	}

	new_dev = max77775_muic_check_new_dev(muic_data, &intr, irq);

	if (intr == MUIC_INTR_ATTACH) {
		md75_info_usb("%s ATTACHED\n", __func__);

		ret = max77775_muic_handle_attach(muic_data, new_dev, irq);
		if (ret)
			md75_err_usb("%s cannot handle attach(%d)\n", __func__, ret);
	} else {
		md75_info_usb("%s DETACHED\n", __func__);

		if (irq == muic_data->irq_chgtyp && vbvolt == 0 && chgtyp == CHGTYP_DEDICATED_CHARGER) {
			md75_info_usb("[MUIC] %s USB Killer Detected!!!\n", __func__);
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
			event = NOTIFY_EXTRA_USBKILLER;
			store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
#if defined(CONFIG_USB_HW_PARAM)
			if (o_notify)
				inc_hw_param(o_notify, USB_CCIC_USB_KILLER_COUNT);
#endif
		}

		ret = max77775_muic_handle_detach(muic_data, irq);
		if (ret)
			md75_err_usb("%s cannot handle detach(%d)\n", __func__, ret);
	}
#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
	max77775_muic_handle_vbus(muic_data);
#endif
}

static irqreturn_t max77775_muic_irq(int irq, void *data)
{
	struct max77775_muic_data *muic_data = data;
	struct irq_desc *desc = irq_to_desc(irq);

	if (!desc) {
		md75_info_usb("%s desc is null\n", __func__);
		goto out;
	}

	if (!muic_data) {
		md75_err_usb("%s muic_data is null(%s)\n", __func__, desc->action->name);
		goto out;
	}

	md75_info_usb("%s irq:%d (%s)\n", __func__, irq, desc->action->name);

	mutex_lock(&muic_data->muic_mutex);
	if (muic_data->is_muic_ready == true)
		max77775_muic_detect_dev(muic_data, irq);
	else
		md75_info_usb("%s MUIC is not ready, just return\n", __func__);
	mutex_unlock(&muic_data->muic_mutex);

out:
	return IRQ_HANDLED;
}

#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC)
static void max77775_muic_afc_work(struct work_struct *work)
{
	struct max77775_muic_data *muic_data =
		container_of(work, struct max77775_muic_data, afc_work.work);

	md75_info_usb("%s\n", __func__);

	if (max77775_muic_check_is_enable_afc(muic_data, muic_data->attached_dev)) {
		mutex_lock(&muic_data->afc_lock);
		muic_data->pdata->afc_disabled_updated |= MAX77775_MUIC_AFC_WORK_PROCESS;

		if (!muic_data->pdata->afc_disable) {
			muic_data->is_check_hv = true;
			muic_data->hv_voltage = 9;
			max77775_muic_afc_hv_set(muic_data, 9);
		} else {
			muic_data->is_check_hv = true;
			muic_data->hv_voltage = 5;
			max77775_muic_afc_hv_set(muic_data, 5);
		}
		mutex_unlock(&muic_data->afc_lock);
	}
}

static int max77775_muic_hv_charger_disable(bool en)
{
	struct max77775_muic_data *muic_data = g_muic_data;

	muic_data->is_charger_mode = en;

	mutex_lock(&muic_data->afc_lock);
	if (muic_data->pdata->afc_disabled_updated & MAX77775_MUIC_AFC_WORK_PROCESS)
		muic_data->pdata->afc_disabled_updated |= MAX77775_MUIC_AFC_DISABLE_CHANGE_DURING_WORK;
	else
		schedule_delayed_work(&(muic_data->afc_work), msecs_to_jiffies(0));
	mutex_unlock(&muic_data->afc_lock);

	return 0;
}

int __max77775_muic_afc_set_voltage(struct max77775_muic_data *muic_data, int voltage)
{
	int now_voltage = 0, ret = 0;

	switch (voltage) {
	case 5:
	case 9:
		break;
	default:
		md75_err_usb("%s: invalid value %d, return\n", __func__, voltage);
		ret = -EINVAL;
		goto err;
	}

	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_AFC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_5V_MUIC:
		now_voltage = 5;
		break;
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
		now_voltage = 9;
		break;
	default:
		break;
	}

	if (voltage == now_voltage) {
		md75_err_usb("%s: same with current voltage, return\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	muic_data->hv_voltage = voltage;

	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_AFC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
		ret = max77775_muic_afc_hv_set(muic_data, voltage);
		break;
	case ATTACHED_DEV_QC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
		ret = max77775_muic_qc_hv_set(muic_data, voltage);
		break;
	default:
		md75_err_usb("%s: not a HV Charger %d, return\n", __func__, muic_data->attached_dev);
		ret = -EINVAL;
		goto err;
	}
err:
	return ret;
}

static int max77775_muic_afc_set_voltage(int voltage)
{
	struct max77775_muic_data *muic_data = g_muic_data;
	int ret = 0;

	mutex_lock(&muic_data->afc_lock);

	if (muic_data->pdata->afc_disabled_updated & MAX77775_MUIC_AFC_WORK_PROCESS) {
		muic_data->pdata->afc_disabled_updated |= MAX77775_MUIC_AFC_SET_VOLTAGE_CHANGE_DURING_WORK;
		muic_data->reserve_hv_voltage = voltage;
		goto skip;
	}

	muic_data->pdata->afc_disabled_updated |= MAX77775_MUIC_AFC_WORK_PROCESS;
	muic_data->reserve_hv_voltage = 0;

	ret = __max77775_muic_afc_set_voltage(muic_data, voltage);
	if (ret < 0)
		muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_WORK_PROCESS_END;
skip:
	mutex_unlock(&muic_data->afc_lock);
	return ret;
}

static int max77775_muic_hv_charger_init(void)
{
	struct max77775_muic_data *muic_data = g_muic_data;

	if (!muic_data || !muic_data->pdata ||
		!test_bit(MUIC_PROBE_DONE, &muic_data->pdata->driver_probe_flag)) {
		md75_info_usb("[%s:%s] skip\n", MUIC_DEV_NAME, __func__);
		return 0;
	}

	if (muic_data->is_charger_ready) {
		md75_info_usb("%s: charger is already ready(%d), return\n",
				__func__, muic_data->is_charger_ready);
		return -EINVAL;
	}

	muic_data->is_charger_ready = true;

	if (max77775_muic_check_is_enable_afc(muic_data, muic_data->attached_dev)) {
		md75_info_usb("%s afc work start\n", __func__);
		cancel_delayed_work_sync(&(muic_data->afc_work));
		schedule_delayed_work(&(muic_data->afc_work), msecs_to_jiffies(0));
	}

	return 0;
}

static void max77775_muic_detect_dev_hv_work(struct work_struct *work)
{
	struct max77775_muic_data *muic_data = container_of(work,
		struct max77775_muic_data, afc_handle_work);
	unsigned char opcode = muic_data->afc_op_dataout[0];

	mutex_lock(&muic_data->muic_mutex);
	if (!max77775_muic_check_is_enable_afc(muic_data, muic_data->attached_dev)) {
		switch (muic_data->attached_dev) {
		case ATTACHED_DEV_AFC_CHARGER_5V_MUIC:
		case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
		case ATTACHED_DEV_QC_CHARGER_5V_MUIC:
		case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
			md75_info_usb("%s high voltage value is changed\n", __func__);
			break;
		default:
			md75_info_usb("%s status is changed, return\n", __func__);
			goto out;
		}
	}

	if (opcode == COMMAND_AFC_RESULT_READ)
		max77775_muic_handle_detect_dev_afc(muic_data, muic_data->afc_op_dataout);
	else if (opcode == COMMAND_QC_2_0_SET)
		max77775_muic_handle_detect_dev_qc(muic_data, muic_data->afc_op_dataout);
	else
		md75_info_usb("%s undefined opcode(%d)\n", __func__, opcode);

out:
	mutex_unlock(&muic_data->muic_mutex);
}

void max77775_muic_handle_detect_dev_hv(struct max77775_muic_data *muic_data, unsigned char *data)
{
	int i;

	for (i = 0; i < AFC_OP_OUT_LEN; i++)
		muic_data->afc_op_dataout[i] = data[i];

	schedule_work(&(muic_data->afc_handle_work));
}
#endif /* CONFIG_HV_MUIC_MAX77775_AFC */

#if IS_ENABLED(CONFIG_MUIC_POGO)
static int max77775_muic_set_pogo_adc(int adc)
{
	struct max77775_muic_data *muic_data = g_muic_data;

	pr_info("%s adc(0x%x)\n", __func__, adc);
	muic_data->pogo_adc = adc;

#if defined(CONFIG_MUIC_POGO_DISABLE_TYPEC)
	if (adc == ADC_HMT) {
		pr_info("%s adc is POGO pogo keyboard, path open, bc12 off\n", __func__);
		com_to_open(muic_data);

		max77775_muic_disable_chgdet(muic_data);
	}

	if (adc == ADC_OPEN) {
		pr_info("%s adc is open, bc12 on\n", __func__);
		max77775_muic_enable_chgdet(muic_data);
	}
#endif

	return 0;
}
#endif /* CONFIG_MUIC_POGO */

#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
static int max77775_muic_set_hiccup_mode(int on_off)
{
	struct max77775_muic_data *muic_data = g_muic_data;

	md75_info_usb("%s (%d)\n", __func__, on_off);

	switch (on_off) {
	case MUIC_HICCUP_MODE_OFF:
	case MUIC_HICCUP_MODE_ON:
		if (muic_data->is_hiccup_mode != on_off) {
			muic_data->is_hiccup_mode = on_off;
#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC)
			if (muic_data->is_check_hv)
				max77775_muic_clear_hv_control(muic_data);
#endif /* CONFIG_HV_MUIC_MAX77775_AFC */
			schedule_work(&(muic_data->ccic_info_data_work));
		}
		break;
	default:
		md75_err_usb("%s undefined value(%d), return\n", __func__, on_off);
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_HICCUP_CHARGER */

static void max77775_muic_stuck_check(struct work_struct *work)
{
	struct max77775_muic_data *muic_data =
		container_of(work, struct max77775_muic_data, stuck_check_work.work);
	struct max77775_usbc_platform_data *usbc_data = muic_data->usbc_pdata;
	struct i2c_client *i2c = muic_data->i2c;
	usbc_cmd_queue_t *cmd_queue = NULL;
	u8 cc_stat;
	u8 stop_mode;

	max77775_muic_read_reg(i2c, REG_CC_STATUS1, &cc_stat);
	cc_stat = (cc_stat & BIT_CCStat) >> FFS(BIT_CCStat);
	md75_info_usb("%s: cc_stat: 0x%x (0:No_connection/1:Sink/2:Source)\n", __func__, cc_stat);

	max77775_muic_read_reg(i2c, REG_PD_STATUS2, &stop_mode);
	stop_mode = (stop_mode & BIT_PD_STOP_MODE) >> FFS(BIT_PD_STOP_MODE);
	md75_info_usb("%s: stopmode: 0x%x\n", __func__, stop_mode);

	cmd_queue = &(usbc_data->usbc_cmd_queue);

	if ((cc_stat == cc_No_Connection || cc_stat == cc_SOURCE)
		&& (stop_mode == 1)
		&& is_empty_usbc_cmd_queue(cmd_queue)){
		max77775_send_check_stuck_opcode(usbc_data);
		md75_info_usb("%s: max77775_send_check_stuck_opcode\n", __func__);
	}

	schedule_delayed_work(&(muic_data->stuck_check_work), msecs_to_jiffies(30000));
}

static void max77775_muic_print_reg_log(struct work_struct *work)
{
	struct max77775_muic_data *muic_data =
		container_of(work, struct max77775_muic_data, debug_work.work);
	struct max77775_usbc_platform_data *usbc_data = muic_data->usbc_pdata;
	struct i2c_client *i2c = muic_data->i2c;
	struct i2c_client *pmic_i2c = muic_data->usbc_pdata->i2c;
	u8 status[12] = {0, };
#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
	u8 spare_status[2] = {0, };
#endif
	u8 fw_rev = 0, fw_rev2 = 0;
	int delay_time = 60000;
	static int prev_opcode_fail_count;

	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_USBC_STATUS1, &status[0]);
	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_USBC_STATUS2, &status[1]);
	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_BC_STATUS, &status[2]);
	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_CC_STATUS1, &status[3]);
	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_CC_STATUS2, &status[4]);
	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_PD_STATUS1, &status[5]);
	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_PD_STATUS2, &status[6]);
	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_UIC_INT_M, &status[7]);
	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_CC_INT_M, &status[8]);
	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_PD_INT_M, &status[9]);
	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_VDM_INT_M, &status[10]);
	max77775_muic_read_reg(pmic_i2c, MAX77775_PMIC_REG_INTSRC_MASK, &status[11]);
	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_UIC_FW_REV, &fw_rev);
	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_UIC_FW_REV2, &fw_rev2);

	md75_info_usb("%s USBC1:0x%02x, USBC2:0x%02x, BC:0x%02x, CC0:0x%x, CC1:0x%x, PD0:0x%x, PD1:0x%x FW:%02X.%02X attached_dev:%d\n",
		__func__, status[0], status[1], status[2], status[3], status[4], status[5], status[6],
		fw_rev, fw_rev2, muic_data->attached_dev);
	md75_info_usb("%s UIC_INT_M:0x%x, CC_INT_M:0x%x, PD_INT_M:0x%x, VDM_INT_M:0x%x, PMIC_MASK:0x%x, WDT:%d, POR:%d\n",
		__func__, status[7], status[8], status[9], status[10], status[11],
		muic_data->usbc_pdata->watchdog_count, muic_data->usbc_pdata->por_count);

#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_SPARE_STATUS1, &spare_status[0]);
	max77775_muic_read_reg(i2c, MAX77775_USBC_REG_SPARE_INT_M, &spare_status[1]);

	md75_info_usb("%s TA:0x%x, SPARE_INT_M:0x%x\n",
		__func__, spare_status[0], spare_status[1]);
#endif

	if (fw_rev == 0xFF && usbc_data->fw_update != 1) {
		usbc_data->fw_update = 1;
		md75_info_usb("%s: FW : %02X.%02X >> Start FW Update\n", __func__, fw_rev, fw_rev2);

		if (max77775_usbc_fw_setting(usbc_data->max77775, 0) < 0) {
			usbc_data->fw_update = 2;
			md75_info_usb("%s: Failed to try FW Update\n", __func__);
		} else {
			usbc_data->fw_update = 0;
			md75_info_usb("%s: Completed to try FW Update\n", __func__);
		}
	}

	if (max77775_need_check_stuck(usbc_data)) {
		msg_maxim("%s stuck suppose.", __func__);
		max77775_send_check_stuck_opcode(usbc_data);
	}

	if (usbc_data->opcode_fail_count) {
		delay_time = 11000;
		if (prev_opcode_fail_count != usbc_data->opcode_fail_count)
			__pm_wakeup_event(muic_data->muic_ws, 11000 + 1000);
	}

	prev_opcode_fail_count = usbc_data->opcode_fail_count;

	msg_maxim("%s usbc_data->opcode_fail_count(%d), delay_time(%d)",
		__func__, usbc_data->opcode_fail_count, delay_time);
	schedule_delayed_work(&(muic_data->debug_work),
		msecs_to_jiffies(delay_time));
}

static void max77775_muic_handle_ccic_event(struct work_struct *work)
{
	struct max77775_muic_data *muic_data = container_of(work,
		struct max77775_muic_data, ccic_info_data_work);

	md75_info_usb("%s\n", __func__);

	mutex_lock(&muic_data->muic_mutex);
	if (muic_data->is_muic_ready == true)
		max77775_muic_detect_dev(muic_data, MUIC_IRQ_PDIC_HANDLER);
	else
		md75_info_usb("%s MUIC is not ready, just return\n", __func__);
	mutex_unlock(&muic_data->muic_mutex);
}

#define REQUEST_IRQ(_irq, _dev_id, _name)				\
do {									\
	ret = request_threaded_irq(_irq, NULL, max77775_muic_irq,	\
				IRQF_NO_SUSPEND, _name, _dev_id);	\
	if (ret < 0) {							\
		md75_err_usb("%s Failed to request IRQ #%d: %d\n",		\
			__func__, _irq, ret);	\
		_irq = 0;						\
	}								\
} while (0)

static int max77775_muic_irq_init(struct max77775_muic_data *muic_data)
{
	int ret = 0;

	md75_info_usb("%s\n", __func__);

	if (muic_data->mfd_pdata && (muic_data->mfd_pdata->irq_base > 0)) {
		int irq_base = muic_data->mfd_pdata->irq_base;

		/* request MUIC IRQ */
		muic_data->irq_uiadc = irq_base + MAX77775_USBC_IRQ_UIDADC_INT;
		REQUEST_IRQ(muic_data->irq_uiadc, muic_data, "muic-uiadc");

		muic_data->irq_chgtyp = irq_base + MAX77775_USBC_IRQ_CHGT_INT;
		REQUEST_IRQ(muic_data->irq_chgtyp, muic_data, "muic-chgtyp");

		muic_data->irq_dcdtmo = irq_base + MAX77775_USBC_IRQ_DCD_INT;
		REQUEST_IRQ(muic_data->irq_dcdtmo, muic_data, "muic-dcdtmo");

		muic_data->irq_vbadc = irq_base + MAX77775_USBC_IRQ_VBADC_INT;
		REQUEST_IRQ(muic_data->irq_vbadc, muic_data, "muic-vbadc");

		muic_data->irq_vbusdet = irq_base + MAX77775_USBC_IRQ_VBUS_INT;
		REQUEST_IRQ(muic_data->irq_vbusdet, muic_data, "muic-vbusdet");
	}

	md75_info_usb("%s uiadc(%d), chgtyp(%d), dcdtmo(%d), vbadc(%d), vbusdet(%d)\n",
			__func__, muic_data->irq_uiadc,
			muic_data->irq_chgtyp, muic_data->irq_dcdtmo,
			muic_data->irq_vbadc, muic_data->irq_vbusdet);
	return ret;
}

#define FREE_IRQ(_irq, _dev_id, _name)					\
do {									\
	if (_irq) {							\
		free_irq(_irq, _dev_id);				\
		md75_info_usb("%s IRQ(%d):%s free done\n",	\
				__func__, _irq, _name);			\
	}								\
} while (0)

static void max77775_muic_free_irqs(struct max77775_muic_data *muic_data)
{
	md75_info_usb("%s\n", __func__);

	disable_irq(muic_data->irq_uiadc);
	disable_irq(muic_data->irq_chgtyp);
	disable_irq(muic_data->irq_dcdtmo);
	disable_irq(muic_data->irq_vbadc);
	disable_irq(muic_data->irq_vbusdet);

	/* free MUIC IRQ */
	FREE_IRQ(muic_data->irq_uiadc, muic_data, "muic-uiadc");
	FREE_IRQ(muic_data->irq_chgtyp, muic_data, "muic-chgtyp");
	FREE_IRQ(muic_data->irq_dcdtmo, muic_data, "muic-dcdtmo");
	FREE_IRQ(muic_data->irq_vbadc, muic_data, "muic-vbadc");
	FREE_IRQ(muic_data->irq_vbusdet, muic_data, "muic-vbusdet");
}

static void max77775_muic_init_detect(struct max77775_muic_data *muic_data)
{
	md75_info_usb("%s\n", __func__);

	mutex_lock(&muic_data->muic_mutex);
	muic_data->is_muic_ready = true;

	max77775_muic_detect_dev(muic_data, MUIC_IRQ_INIT_DETECT);
	max77775_muic_enable_detecting_short(muic_data);

	mutex_unlock(&muic_data->muic_mutex);
}

static void max77775_set_bc1p2_retry_count(struct max77775_muic_data *muic_data)
{
	struct device_node *np = NULL;
	int count;

	np = of_find_compatible_node(NULL, NULL, "maxim,max77775");

	if (np && !of_property_read_u32(np, "max77775,bc1p2_retry_count", &count))
		muic_data->bc1p2_retry_count = count;
	else
		muic_data->bc1p2_retry_count = 1; /* default retry count */

	md75_info_usb("%s:%s BC1p2 Retry count: %d\n", MUIC_DEV_NAME,
				__func__, muic_data->bc1p2_retry_count);
}

static void max77775_muic_clear_interrupt(struct max77775_muic_data *muic_data)
{
	struct i2c_client *i2c = muic_data->i2c;
	u8 interrupt;
	int ret;

	md75_info_usb("%s\n", __func__);

	ret = max77775_muic_read_reg(i2c,
			MAX77775_USBC_REG_UIC_INT, &interrupt);
	if (ret)
		md75_err_usb("%s fail to read muic INT1 reg(%d)\n",
			__func__, ret);

	md75_info_usb("%s CLEAR!! UIC_INT:0x%02x\n",
		__func__, interrupt);
}

static int max77775_muic_init_regs(struct max77775_muic_data *muic_data)
{
	int ret;

	md75_info_usb("%s\n", __func__);

	max77775_muic_clear_interrupt(muic_data);

	ret = max77775_muic_irq_init(muic_data);
	if (ret < 0) {
		md75_err_usb("%s Failed to initialize MUIC irq:%d\n",
				__func__, ret);
		max77775_muic_free_irqs(muic_data);
	}

	return ret;
}

#if IS_ENABLED(CONFIG_USB_EXTERNAL_NOTIFY)
static int muic_handle_usb_notification(struct notifier_block *nb,
				unsigned long action, void *data)
{
	struct max77775_muic_data *pmuic =
		container_of(nb, struct max77775_muic_data, usb_nb);

	switch (action) {
	/* Abnormal device */
	case EXTERNAL_NOTIFY_3S_NODEVICE:
		md75_info_usb("%s: 3S_NODEVICE(USB HOST Connection timeout)\n",
				__func__);
		if (pmuic->attached_dev == ATTACHED_DEV_HMT_MUIC)
			muic_send_dock_intent(MUIC_DOCK_ABNORMAL);
		break;

	/* Gamepad device connected */
	case EXTERNAL_NOTIFY_DEVICE_CONNECT:
		md75_info_usb("%s: DEVICE_CONNECT(Gamepad)\n", __func__);
		break;

	default:
		break;
	}

	return NOTIFY_DONE;
}


static void muic_register_usb_notifier(struct max77775_muic_data *pmuic)
{
	int ret = 0;

	md75_info_usb("%s\n", __func__);


	ret = usb_external_notify_register(&pmuic->usb_nb,
		muic_handle_usb_notification, EXTERNAL_NOTIFY_DEV_MUIC);
	if (ret < 0) {
		md75_info_usb("%s: USB Noti. is not ready.\n", __func__);
		return;
	}

	md75_info_usb("%s: done.\n", __func__);
}

static void muic_unregister_usb_notifier(struct max77775_muic_data *pmuic)
{
	int ret = 0;

	md75_info_usb("%s\n", __func__);

	ret = usb_external_notify_unregister(&pmuic->usb_nb);
	if (ret < 0) {
		md75_info_usb("%s: USB Noti. unregister error.\n", __func__);
		return;
	}

	md75_info_usb("%s: done.\n", __func__);
}
#else
static void muic_register_usb_notifier(struct max77775_muic_data *pmuic) {}
static void muic_unregister_usb_notifier(struct max77775_muic_data *pmuic) {}
#endif

int max77775_muic_probe(struct max77775_usbc_platform_data *usbc_data)
{
	struct max77775_platform_data *mfd_pdata = usbc_data->max77775_data;
	struct max77775_muic_data *muic_data;
	int ret = 0;
#if IS_ENABLED(CONFIG_ARCH_QCOM) && !defined(CONFIG_USB_ARCH_EXYNOS)
	u8 pogo_adc = 0;
#endif
	md75_info_usb("%s\n", __func__);

	muic_data = devm_kzalloc(usbc_data->dev, sizeof(struct max77775_muic_data), GFP_KERNEL);
	if (!muic_data) {
		ret = -ENOMEM;
		goto err_return;
	}

	if (!mfd_pdata) {
		md75_err_usb("%s: failed to get mfd platform data\n", __func__);
		ret = -ENOMEM;
		goto err_return;
	}

	mutex_init(&muic_data->muic_mutex);
	mutex_init(&muic_data->afc_lock);
	muic_data->muic_ws = wakeup_source_register(usbc_data->dev, "muic-irq");
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
	muic_data->afc_retry_ws = wakeup_source_register(usbc_data->dev, "muic-afc-retry");
#endif
	muic_data->i2c = usbc_data->muic;
	muic_data->mfd_pdata = mfd_pdata;
	muic_data->usbc_pdata = usbc_data;
	muic_data->pdata = &muic_pdata;
	muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;
	muic_data->is_muic_ready = false;
	muic_data->is_otg_test = false;
	muic_data->is_factory_start = false;
	muic_data->switch_val = COM_OPEN;
	muic_data->is_charger_mode = false;
	muic_data->dcdtmo_retry = 0;

	usbc_data->muic_data = muic_data;
	g_muic_data = muic_data;

#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
	muic_data->muic_d.ops = NULL;
	muic_data->muic_d.data = (void *)muic_data;
	muic_data->man = register_muic(&(muic_data->muic_d));
#endif
	if (muic_data->pdata->init_gpio_cb) {
#if IS_MODULE(CONFIG_MUIC_NOTIFIER)
		ret = muic_data->pdata->init_gpio_cb(get_switch_sel());
#else
		ret = muic_data->pdata->init_gpio_cb();
#endif
		if (ret) {
			md75_err_usb("%s: failed to init gpio(%d)\n", __func__, ret);
			goto fail;
		}
	}

	mutex_lock(&muic_data->muic_mutex);

	/* create sysfs group */
	if (switch_device) {
		ret = sysfs_create_group(&switch_device->kobj, &max77775_muic_group);
		if (ret) {
			md75_err_usb("%s: failed to create attribute group. error: %d\n",
					__func__, ret);
			goto fail_sysfs_create;
		}
		kobject_uevent(&switch_device->kobj, KOBJ_CHANGE);
		dev_set_drvdata(switch_device, muic_data);
	}

	if (muic_data->pdata->init_switch_dev_cb)
		muic_data->pdata->init_switch_dev_cb();

	ret = max77775_muic_init_regs(muic_data);
	if (ret < 0) {
		md75_err_usb("%s Failed to initialize MUIC irq:%d\n",
				__func__, ret);
		goto fail_init_irq;
	}

	mutex_unlock(&muic_data->muic_mutex);

#if IS_ENABLED(CONFIG_USB_ARCH_EXYNOS)
	muic_data->pdata->opmode = get_pdic_info() & 0x0f;
#else
	ret = max77775_read_reg(muic_data->i2c, MAX77775_USBC_REG_USBC_STATUS1, &pogo_adc);
	pogo_adc &= USBC_STATUS1_UIADC_MASK;
	md75_info_usb("%s:%s POGO_ADC : 0x%02x\n", MUIC_DEV_NAME, __func__, pogo_adc);

	if (pogo_adc == 0x07)
		muic_data->pdata->opmode = OPMODE_PDIC;
	else
		muic_data->pdata->opmode = OPMODE_MUIC;
#endif
	if (muic_data->pdata->opmode & OPMODE_PDIC) {
		max77775_muic_register_ccic_notifier(muic_data);
		INIT_WORK(&(muic_data->ccic_info_data_work),
			max77775_muic_handle_ccic_event);
	}

	muic_data->afc_water_disable = false;

#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC)
	if (get_afc_mode() == CH_MODE_AFC_DISABLE_VAL) {
		md75_info_usb("%s: afc_disable: AFC disabled\n", __func__);
		muic_data->pdata->afc_disable = true;
	} else {
		md75_info_usb("%s: afc_disable: AFC enabled\n", __func__);
		muic_data->pdata->afc_disable = false;
	}
	muic_data->pdata->afc_disabled_updated = 0;

	INIT_DELAYED_WORK(&(muic_data->afc_work),
		max77775_muic_afc_work);
	INIT_WORK(&(muic_data->afc_handle_work),
		max77775_muic_detect_dev_hv_work);

	/* set MUIC afc voltage switching function */
	muic_data->pdata->muic_afc_set_voltage_cb = max77775_muic_afc_set_voltage;
	muic_data->pdata->muic_hv_charger_disable_cb = max77775_muic_hv_charger_disable;

#if IS_ENABLED(CONFIG_MUIC_POGO)
	muic_data->pdata->muic_set_pogo_adc_cb = max77775_muic_set_pogo_adc;
	muic_data->pogo_adc = ADC_OPEN;
#endif /* CONFIG_MUIC_POGO */

	/* set MUIC check charger init function */
	muic_data->pdata->muic_hv_charger_init_cb = max77775_muic_hv_charger_init;
	muic_data->is_charger_ready = false;
	muic_data->is_check_hv = false;
	muic_data->hv_voltage = 0;
	muic_data->afc_retry = 0;
	muic_data->is_afc_reset = false;
	muic_data->is_skip_bigdata = false;
	muic_data->is_usb_fail = false;
#endif /* CONFIG_HV_MUIC_MAX77775_AFC */

#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
	muic_data->is_hiccup_mode = 0;
	muic_data->pdata->muic_set_hiccup_mode_cb = max77775_muic_set_hiccup_mode;
#endif /* CONFIG_HICCUP_CHARGER */

	/* set bc1p2 retry count */
	max77775_set_bc1p2_retry_count(muic_data);

	/* initial cable detection */
	max77775_muic_init_detect(muic_data);

	/* register usb external notifier */
	muic_register_usb_notifier(muic_data);

	INIT_DELAYED_WORK(&(muic_data->debug_work),
		max77775_muic_print_reg_log);
	schedule_delayed_work(&(muic_data->debug_work),
		msecs_to_jiffies(10000));

	INIT_DELAYED_WORK(&(muic_data->stuck_check_work),
		max77775_muic_stuck_check);
	schedule_delayed_work(&(muic_data->stuck_check_work),
		msecs_to_jiffies(30000));

	/* hv charger init */
	set_bit(MUIC_PROBE_DONE, &muic_data->pdata->driver_probe_flag);
	if (test_bit(CHARGER_PROBE_DONE, &muic_data->pdata->driver_probe_flag))
		max77775_muic_hv_charger_init();

	return 0;

fail_init_irq:
	if (muic_data->pdata->cleanup_switch_dev_cb)
		muic_data->pdata->cleanup_switch_dev_cb();
	if (switch_device)
		sysfs_remove_group(&switch_device->kobj, &max77775_muic_group);
fail_sysfs_create:
	mutex_unlock(&muic_data->muic_mutex);
fail:
	mutex_destroy(&muic_data->muic_mutex);
err_return:
	return ret;
}

int max77775_muic_remove(struct max77775_usbc_platform_data *usbc_data)
{
	struct max77775_muic_data *muic_data = usbc_data->muic_data;

	if (switch_device)
		sysfs_remove_group(&switch_device->kobj, &max77775_muic_group);

	if (muic_data) {
		md75_info_usb("%s\n", __func__);

		max77775_muic_free_irqs(muic_data);

		if (muic_data->pdata->opmode & OPMODE_PDIC) {
			max77775_muic_unregister_ccic_notifier(muic_data);
			cancel_work_sync(&(muic_data->ccic_info_data_work));
		}
#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC)
		cancel_delayed_work_sync(&(muic_data->afc_work));
		cancel_work_sync(&(muic_data->afc_handle_work));
#endif
		muic_unregister_usb_notifier(muic_data);
		cancel_delayed_work_sync(&(muic_data->debug_work));
		cancel_delayed_work_sync(&(muic_data->stuck_check_work));

		if (muic_data->pdata->cleanup_switch_dev_cb)
			muic_data->pdata->cleanup_switch_dev_cb();

		mutex_destroy(&muic_data->muic_mutex);
		wakeup_source_unregister(muic_data->muic_ws);
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
		wakeup_source_unregister(muic_data->afc_retry_ws);
#endif
	}

	return 0;
}

void max77775_muic_shutdown(struct max77775_usbc_platform_data *usbc_data)
{
	struct max77775_muic_data *muic_data = usbc_data->muic_data;

	md75_info_usb("%s +\n", __func__);

	if (switch_device)
		sysfs_remove_group(&switch_device->kobj, &max77775_muic_group);

	if (!muic_data) {
		md75_err_usb("%s no drvdata\n", __func__);
		goto out;
	}

	com_to_open(muic_data);
#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
	max77775_muic_hiccup_clear(muic_data);
#endif

	max77775_muic_free_irqs(muic_data);

	if ((muic_data->pdata) && (muic_data->pdata->opmode & OPMODE_PDIC)) {
		max77775_muic_unregister_ccic_notifier(muic_data);
		cancel_work_sync(&(muic_data->ccic_info_data_work));
	}
#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC)
	if (muic_data->pdata) {
		/* clear MUIC afc voltage switching function */
		muic_data->pdata->muic_afc_set_voltage_cb = NULL;
		/* clear MUIC check charger init function */
		muic_data->pdata->muic_hv_charger_init_cb = NULL;
	}
	cancel_delayed_work_sync(&(muic_data->afc_work));
	cancel_work_sync(&(muic_data->afc_handle_work));
#endif
#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
	if (muic_data->pdata)
		muic_data->pdata->muic_set_hiccup_mode_cb = NULL;
#endif /* CONFIG_HICCUP_CHARGER */
	muic_unregister_usb_notifier(muic_data);
	cancel_delayed_work_sync(&(muic_data->debug_work));
	cancel_delayed_work_sync(&(muic_data->stuck_check_work));

out:
	md75_info_usb("%s -\n", __func__);
}

int max77775_muic_suspend(struct max77775_usbc_platform_data *usbc_data)
{
	struct max77775_muic_data *muic_data = usbc_data->muic_data;

	md75_info_usb("%s\n", __func__);
	cancel_delayed_work_sync(&(muic_data->debug_work));
	cancel_delayed_work_sync(&(muic_data->stuck_check_work));

	return 0;
}

int max77775_muic_resume(struct max77775_usbc_platform_data *usbc_data)
{
	struct max77775_muic_data *muic_data = usbc_data->muic_data;

	md75_info_usb("%s\n", __func__);
	schedule_delayed_work(&(muic_data->debug_work), msecs_to_jiffies(1000));
	schedule_delayed_work(&(muic_data->stuck_check_work), msecs_to_jiffies(30000));

	return 0;
}
