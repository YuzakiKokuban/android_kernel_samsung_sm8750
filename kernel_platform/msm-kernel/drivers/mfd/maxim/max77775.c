/*
 * max77775.c - mfd core driver for the Maxim 77775
 *
 * Copyright (C) 2016 Samsung Electronics
 * Insun Choi <insun77.choi@samsung.com>
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
 *
 * This driver is based on max8997.c
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/mfd/core.h>
#include <linux/mfd/max77775_log.h>
#include <linux/mfd/max77775.h>
#include <linux/mfd/max77775-private.h>
#include <linux/regulator/machine.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <linux/firmware.h>

#include <linux/usb/typec/common/pdic_param.h>
#if defined(CONFIG_HV_MUIC_MAX77775_AFC)
#include <linux/muic/common/muic.h>
#endif

#define FW_BIN_NAME		"max77775-fw.bin"
#define EXTRA_FW_BIN_NAME	"max77775-extra-fw.bin"
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
#include <linux/usb_notify.h>
#endif
#if defined(CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif /* CONFIG_OF */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
#include <linux/battery/sec_battery_common.h>
#endif
#if IS_ENABLED(CONFIG_SEC_ABC) && IS_ENABLED(CONFIG_MAX77775_ABC_IFPMIC_EVENT)
#include <linux/sti/abc_common.h>
#endif

#if IS_ENABLED(CONFIG_SEC_MPARAM) || (IS_MODULE(CONFIG_SEC_PARAM) && defined(CONFIG_ARCH_EXYNOS))
extern int factory_mode;
#else
static int __read_mostly factory_mode;
module_param(factory_mode, int, 0444);
#endif

#define I2C_ADDR_PMIC		(0xCC >> 1)
#define I2C_ADDR_MUIC		(0x4A >> 1)
#define I2C_ADDR_CHG		(0xD2 >> 1)
#define I2C_ADDR_FG     	(0x6C >> 1)
#define I2C_ADDR_TESTSID	(0xC4 >> 1)

#define I2C_RETRY_CNT	3

#define MD75_FIRMWARE_TIMEOUT_SEC	5
#define MD75_FIRMWARE_TIMEOUT_START	1
#define MD75_FIRMWARE_TIMEOUT_PASS	2
#define MD75_FIRMWARE_TIMEOUT_FAIL	3
#define MD75_FIRMWARE_TIMEOUT_COMPLETE	4

/*
 * pmic revision information
 */
struct max77775_revision_struct {
	u8 id;
	u8 rev;
	u8 logical_id;
};

static struct max77775_revision_struct max77775_revision[] = {
	{ 0x75, 0x01, MAX77775_PASS1},	/* MD75 PASS1 */
	{ 0x75, 0x02, MAX77775_PASS2},	/* MD75 PASS2 */
	{ 0x75, 0x03, MAX77775_PASS3},	/* MD75 PASS3 */
	{ 0x75, 0x04, MAX77775_PASS4},	/* MD75 PASS4 */
};

static struct mfd_cell max77775_devs[] = {
#if IS_ENABLED(CONFIG_CCIC_MAX77775)
	{ .name = "max77775-usbc", },
#endif
#if IS_ENABLED(CONFIG_FUELGAUGE_MAX77775)
	{ .name = "max77775-fuelgauge", },
#endif
#if IS_ENABLED(CONFIG_CHARGER_MAX77775)
	{ .name = "max77775-charger", },
#endif
};

static int max77775_firmware_timeout_state;
static int firmware_timeout_count;
static struct platform_device *pdev;

static int max77775_get_facmode(void) { return factory_mode; }

int max77775_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct max77775_dev *max77775 = i2c_get_clientdata(i2c);
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif
	int ret, i;

	if (max77775->shutdown) {
		md75_err_usb("%s:%s shutdown. i2c command is skiped\n",
			MFD_DEV_NAME, __func__);
		return 0;
	}

	mutex_lock(&max77775->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; ++i) {
		ret = i2c_smbus_read_byte_data(i2c, reg);
		if (ret >= 0)
			break;
		md75_info_usb("%s:%s reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
			MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
	}
	mutex_unlock(&max77775->i2c_lock);
	if (ret < 0) {
		md75_info_usb("%s:%s reg(0x%x), ret(%d)\n", MFD_DEV_NAME, __func__, reg, ret);
#if defined(CONFIG_USB_HW_PARAM)
		if (o_notify)
			inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif

#if IS_ENABLED(CONFIG_SEC_ABC) && IS_ENABLED(CONFIG_MAX77775_ABC_IFPMIC_EVENT)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		sec_abc_send_event("MODULE=pdic@INFO=i2c_fail");
#else
		sec_abc_send_event("MODULE=pdic@WARN=i2c_fail");
#endif
#endif

		return ret;
	}

	ret &= 0xff;
	*dest = ret;
	return 0;
}
EXPORT_SYMBOL_GPL(max77775_read_reg);

int max77775_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77775_dev *max77775 = i2c_get_clientdata(i2c);
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif
	int ret, i;

	if (max77775->shutdown) {
		md75_err_usb("%s:%s shutdown. i2c command is skiped\n",
			MFD_DEV_NAME, __func__);
		return 0;
	}

	mutex_lock(&max77775->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; ++i) {
		ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
		if (ret >= 0)
			break;
		md75_info_usb("%s:%s reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
			MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
	}
	mutex_unlock(&max77775->i2c_lock);
	if (ret < 0) {
#if defined(CONFIG_USB_HW_PARAM)
		if (o_notify)
			inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif

#if IS_ENABLED(CONFIG_SEC_ABC) && IS_ENABLED(CONFIG_MAX77775_ABC_IFPMIC_EVENT)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		sec_abc_send_event("MODULE=pdic@INFO=i2c_fail");
#else
		sec_abc_send_event("MODULE=pdic@WARN=i2c_fail");
#endif
#endif

		return ret;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(max77775_bulk_read);

int max77775_read_word(struct i2c_client *i2c, u8 reg)
{
	struct max77775_dev *max77775 = i2c_get_clientdata(i2c);
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif
	int ret, i;

	if (max77775->shutdown) {
		md75_err_usb("%s:%s shutdown. i2c command is skiped\n",
			MFD_DEV_NAME, __func__);
		return 0;
	}

	mutex_lock(&max77775->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; ++i) {
		ret = i2c_smbus_read_word_data(i2c, reg);
		if (ret >= 0)
			break;
		md75_info_usb("%s:%s reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
			MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
	}
	mutex_unlock(&max77775->i2c_lock);
#if defined(CONFIG_USB_HW_PARAM)
	if (ret < 0) {
		if (o_notify)
			inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
	}
#endif

#if IS_ENABLED(CONFIG_SEC_ABC) && IS_ENABLED(CONFIG_MAX77775_ABC_IFPMIC_EVENT)
	if (ret < 0)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		sec_abc_send_event("MODULE=pdic@INFO=i2c_fail");
#else
		sec_abc_send_event("MODULE=pdic@WARN=i2c_fail");
#endif
#endif

	return ret;
}
EXPORT_SYMBOL_GPL(max77775_read_word);

int max77775_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct max77775_dev *max77775 = i2c_get_clientdata(i2c);
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif
	int ret = -EIO, i;
	int timeout = 2000; /* 2sec */
	int interval = 100;

	if (max77775->shutdown) {
		md75_err_usb("%s:%s shutdown. i2c command is skiped\n",
			MFD_DEV_NAME, __func__);
		return 0;
	}

	while (ret == -EIO) {
		mutex_lock(&max77775->i2c_lock);
		for (i = 0; i < I2C_RETRY_CNT; ++i) {
			ret = i2c_smbus_write_byte_data(i2c, reg, value);
			if ((ret >= 0) || (ret == -EIO))
				break;
			md75_info_usb("%s:%s reg(0x%02x), ret(%d), i2c_retry_cnt(%d/%d)\n",
				MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
		}
		mutex_unlock(&max77775->i2c_lock);

		if (ret < 0) {
			md75_info_usb("%s:%s reg(0x%x), ret(%d), timeout %d\n",
					MFD_DEV_NAME, __func__, reg, ret, timeout);

			if (timeout < 0)
				break;

			msleep(interval);
			timeout -= interval;
		}
	}
#if defined(CONFIG_USB_HW_PARAM)
	if (o_notify && ret < 0)
		inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif

#if IS_ENABLED(CONFIG_SEC_ABC) && IS_ENABLED(CONFIG_MAX77775_ABC_IFPMIC_EVENT)
	if (ret < 0)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		sec_abc_send_event("MODULE=pdic@INFO=i2c_fail");
#else
		sec_abc_send_event("MODULE=pdic@WARN=i2c_fail");
#endif
#endif

	return ret;
}
EXPORT_SYMBOL_GPL(max77775_write_reg);

int max77775_write_reg_nolock(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct max77775_dev *max77775 = i2c_get_clientdata(i2c);
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif
	int ret = -EIO;
	int timeout = 2000; /* 2sec */
	int interval = 100;

	if (max77775->shutdown) {
		md75_err_usb("%s:%s shutdown. i2c command is skiped\n",
			MFD_DEV_NAME, __func__);
		return 0;
	}

	while (ret == -EIO) {
		ret = i2c_smbus_write_byte_data(i2c, reg, value);

		if (ret < 0) {
			md75_info_usb("%s:%s reg(0x%x), ret(%d), timeout %d\n",
					MFD_DEV_NAME, __func__, reg, ret, timeout);

			if (timeout < 0)
				break;

			msleep(interval);
			timeout -= interval;
		}
	}
#if defined(CONFIG_USB_HW_PARAM)
	if (o_notify && ret < 0)
		inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif

#if IS_ENABLED(CONFIG_SEC_ABC) && IS_ENABLED(CONFIG_MAX77775_ABC_IFPMIC_EVENT)
	if (ret < 0)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		sec_abc_send_event("MODULE=pdic@INFO=i2c_fail");
#else
		sec_abc_send_event("MODULE=pdic@WARN=i2c_fail");
#endif
#endif

	return ret;
}
EXPORT_SYMBOL_GPL(max77775_write_reg_nolock);

int max77775_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77775_dev *max77775 = i2c_get_clientdata(i2c);
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif
	int ret = -EIO, i;
	int timeout = 2000; /* 2sec */
	int interval = 100;

	if (max77775->shutdown) {
		md75_err_usb("%s:%s shutdown. i2c command is skiped\n",
			MFD_DEV_NAME, __func__);
		return 0;
	}

	while (ret == -EIO) {
		mutex_lock(&max77775->i2c_lock);
		for (i = 0; i < I2C_RETRY_CNT; ++i) {
			ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
			if ((ret >= 0) || (ret == -EIO))
				break;
			md75_info_usb("%s:%s reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
				MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
		}
		mutex_unlock(&max77775->i2c_lock);

		if (ret < 0) {
			md75_info_usb("%s:%s reg(0x%x), ret(%d), timeout %d\n",
					MFD_DEV_NAME, __func__, reg, ret, timeout);

			if (timeout < 0)
				break;

			msleep(interval);
			timeout -= interval;
		}
	}
#if defined(CONFIG_USB_HW_PARAM)
	if (o_notify && ret < 0)
		inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif

#if IS_ENABLED(CONFIG_SEC_ABC) && IS_ENABLED(CONFIG_MAX77775_ABC_IFPMIC_EVENT)
	if (ret < 0)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		sec_abc_send_event("MODULE=pdic@INFO=i2c_fail");
#else
		sec_abc_send_event("MODULE=pdic@WARN=i2c_fail");
#endif
#endif

	return ret;
}
EXPORT_SYMBOL_GPL(max77775_bulk_write);

int max77775_write_word(struct i2c_client *i2c, u8 reg, u16 value)
{
	struct max77775_dev *max77775 = i2c_get_clientdata(i2c);
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif
	int ret, i;

	if (max77775->shutdown) {
		md75_err_usb("%s:%s shutdown. i2c command is skiped\n",
			MFD_DEV_NAME, __func__);
		return 0;
	}

	mutex_lock(&max77775->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; ++i) {
		ret = i2c_smbus_write_word_data(i2c, reg, value);
		if (ret >= 0)
			break;
		md75_info_usb("%s:%s reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
			MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
	}
	mutex_unlock(&max77775->i2c_lock);
	if (ret < 0) {
#if defined(CONFIG_USB_HW_PARAM)
		if (o_notify)
			inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif

#if IS_ENABLED(CONFIG_SEC_ABC) && IS_ENABLED(CONFIG_MAX77775_ABC_IFPMIC_EVENT)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		sec_abc_send_event("MODULE=pdic@INFO=i2c_fail");
#else
		sec_abc_send_event("MODULE=pdic@WARN=i2c_fail");
#endif
#endif

		return ret;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(max77775_write_word);

int max77775_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct max77775_dev *max77775 = i2c_get_clientdata(i2c);
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif
	int ret, i;
	u8 old_val, new_val;

	if (max77775->shutdown) {
		md75_err_usb("%s:%s shutdown. i2c command is skiped\n",
			MFD_DEV_NAME, __func__);
		return 0;
	}

	mutex_lock(&max77775->i2c_lock);
	for (i = 0; i < I2C_RETRY_CNT; ++i) {
		ret = i2c_smbus_read_byte_data(i2c, reg);
		if (ret >= 0)
			break;
		md75_info_usb("%s:%s read reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
			MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
	}
	if (ret < 0) {
#if defined(CONFIG_USB_HW_PARAM)
		if (o_notify)
			inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif
		goto err;
	}
	if (ret >= 0) {
		old_val = ret & 0xff;
		new_val = (val & mask) | (old_val & (~mask));
		for (i = 0; i < I2C_RETRY_CNT; ++i) {
			ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
			if (ret >= 0)
				break;
			md75_info_usb("%s:%s write reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
				MFD_DEV_NAME, __func__, reg, ret, i + 1, I2C_RETRY_CNT);
		}
		if (ret < 0) {
#if defined(CONFIG_USB_HW_PARAM)
			if (o_notify)
				inc_hw_param(o_notify, USB_CCIC_I2C_ERROR_COUNT);
#endif
			goto err;
		}
	}
err:
	mutex_unlock(&max77775->i2c_lock);
#if IS_ENABLED(CONFIG_SEC_ABC) && IS_ENABLED(CONFIG_MAX77775_ABC_IFPMIC_EVENT)
	if (ret < 0)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		sec_abc_send_event("MODULE=pdic@INFO=i2c_fail");
#else
		sec_abc_send_event("MODULE=pdic@WARN=i2c_fail");
#endif
#endif
	return ret;
}
EXPORT_SYMBOL_GPL(max77775_update_reg);

#if defined(CONFIG_OF)
static int of_max77775_dt(struct device *dev, struct max77775_platform_data *pdata)
{
	struct device_node *np_max77775 = dev->of_node;
	struct device_node *np_battery;
#if defined(CONFIG_SEC_FACTORY)	
	struct device_node *np_fuelgauge;
#endif
	int ret, val;

	if (!np_max77775)
		return -EINVAL;

	pdata->irq_gpio = of_get_named_gpio(np_max77775, "max77775,irq-gpio", 0);

	if (of_property_read_u32(np_max77775, "max77775,rev", &pdata->rev))
		pdata->rev = 0;

	if (of_property_read_u32(np_max77775, "max77775,fw_product_id", &pdata->fw_product_id))
		pdata->fw_product_id = 0;

#if defined(CONFIG_SEC_FACTORY)
	pdata->blocking_waterevent = 0;
#else
	pdata->blocking_waterevent = of_property_read_bool(np_max77775, "max77775,blocking_waterevent");
#endif
	ret = of_property_read_u32(np_max77775, "max77775,extra_fw_enable", &val);
	if (ret)
		pdata->extra_fw_enable = 0;
	else
		pdata->extra_fw_enable = val;

	np_battery = of_find_node_by_name(NULL, "mfc-charger");
	if (!np_battery) {
		md75_info_usb("%s: np_battery NULL\n", __func__);
	} else {
		pdata->wpc_en = of_get_named_gpio(np_battery, "battery,wpc_en", 0);
		if (pdata->wpc_en < 0) {
			md75_info_usb("%s: can't get wpc_en (%d)\n", __func__, pdata->wpc_en);
			pdata->wpc_en = 0;
		}

		ret = of_property_read_string(np_battery,
				"battery,wireless_charger_name", (char const **)&pdata->wireless_charger_name);
		if (ret)
			md75_info_usb("%s: Wireless charger name is Empty\n", __func__);
	}
	pdata->support_audio = of_property_read_bool(np_max77775, "max77775,support-audio");

#if defined(CONFIG_SEC_FACTORY)
	np_fuelgauge = of_find_node_by_name(NULL, "max77775-fuelgauge");
	if (!np_fuelgauge) {
		md75_info_usb("%s: np_fuelgauge NULL\n", __func__);
		pdata->fg_resistor = 1;
	} else {
		ret = of_property_read_u32(np_fuelgauge, "fuelgauge,fg_resistor", &pdata->fg_resistor);

		if (ret) {
			pr_err("%s: error reading fg_resistor %d\n", __func__, ret);
			pdata->fg_resistor = 1;
		}
	}
#endif

	return 0;
}
#endif /* CONFIG_OF */

static void max77775_print_pdata_property(struct max77775_platform_data *pdata)
{
	md75_info_usb("%s: irq-gpio: %u\n", __func__, pdata->irq_gpio);
	md75_info_usb("%s: extra_fw_enable: %d\n", __func__,
			pdata->extra_fw_enable);
	md75_info_usb("%s: support_audio %d\n", __func__, pdata->support_audio);
}

/* samsung */
#if IS_ENABLED(CONFIG_CCIC_MAX77775)
static void max77775_reset_ic(struct max77775_dev *max77775)
{
	md75_info_usb("%s: Reset!!\n", __func__);
	max77775_write_reg(max77775->muic, MAX77775_USBC_REG_UIC_SWRST, 0x0F);
	msleep(150);
}

static void max77775_usbc_wait_response_q(struct work_struct *work)
{
	struct max77775_dev *max77775;
	u8 read_value = 0x00;
	u8 dummy[2] = { 0, };

	max77775 = container_of(work, struct max77775_dev, fw_work);

	while (max77775->fw_update_state == FW_UPDATE_WAIT_RESP_START) {
		max77775_bulk_read(max77775->muic, REG_UIC_INT, 1, dummy);
		read_value = dummy[0];
		if ((read_value & BIT_APCmdResI) == BIT_APCmdResI)
			break;
	}

	complete_all(&max77775->fw_completion);
}

static int max77775_usbc_wait_response(struct max77775_dev *max77775)
{
	unsigned long time_remaining = 0;

	max77775->fw_update_state = FW_UPDATE_WAIT_RESP_START;

	init_completion(&max77775->fw_completion);
	queue_work(max77775->fw_workqueue, &max77775->fw_work);

	time_remaining = wait_for_completion_timeout(
			&max77775->fw_completion,
			msecs_to_jiffies(FW_WAIT_TIMEOUT));

	max77775->fw_update_state = FW_UPDATE_WAIT_RESP_STOP;

	if (!time_remaining) {
		md75_info_usb("%s: Failed to update due to timeout\n", __func__);
		cancel_work_sync(&max77775->fw_work);
		return FW_UPDATE_TIMEOUT_FAIL;
	}

	return 0;
}

static int __max77775_usbc_fw_update(
		struct max77775_dev *max77775, const u8 *fw_bin)
{
	u8 fw_cmd = FW_CMD_END;
	u8 fw_len = 0;
	u8 fw_opcode = 0;
	u8 fw_data_len = 0;
	u8 fw_data[FW_CMD_WRITE_SIZE] = { 0, };
	u8 verify_data[FW_VERIFY_DATA_SIZE] = { 0, };
	int ret = -FW_UPDATE_CMD_FAIL;

	/*
	 * fw_bin[0] = Write Command (0x01)
	 * or
	 * fw_bin[0] = Read Command (0x03)
	 * or
	 * fw_bin[0] = End Command (0x00)
	 */
	fw_cmd = fw_bin[0];

	/*
	 * Check FW Command
	 */
	if (fw_cmd == FW_CMD_END) {
		max77775_reset_ic(max77775);
		max77775->fw_update_state = FW_UPDATE_END;
		return FW_UPDATE_END;
	}

	/*
	 * fw_bin[1] = Length ( OPCode + Data )
	 */
	fw_len = fw_bin[1];

	/*
	 * Check fw data length
	 * We support 0x22 or 0x04 only
	 */
	if (fw_len != 0x22 && fw_len != 0x04)
		return FW_UPDATE_MAX_LENGTH_FAIL;

	/*
	 * fw_bin[2] = OPCode
	 */
	fw_opcode = fw_bin[2];

	/*
	 * In case write command,
	 * fw_bin[35:3] = Data
	 *
	 * In case read command,
	 * fw_bin[5:3]  = Data
	 */
	fw_data_len = fw_len - 1; /* exclude opcode */
	memcpy(fw_data, &fw_bin[3], fw_data_len);

	switch (fw_cmd) {
	case FW_CMD_WRITE:
		if (fw_data_len > I2C_SMBUS_BLOCK_MAX) {
			/* write the half data */
			max77775_bulk_write(max77775->muic,
					fw_opcode,
					I2C_SMBUS_BLOCK_HALF,
					fw_data);
			max77775_bulk_write(max77775->muic,
					fw_opcode + I2C_SMBUS_BLOCK_HALF,
					fw_data_len - I2C_SMBUS_BLOCK_HALF,
					&fw_data[I2C_SMBUS_BLOCK_HALF]);
		} else
			max77775_bulk_write(max77775->muic,
					fw_opcode,
					fw_data_len,
					fw_data);

		ret = max77775_usbc_wait_response(max77775);
		if (ret)
			return ret;

		/*
		 * Why do we need 1ms sleep in case MQ81?
		 */
		/* msleep(1); */

		return FW_CMD_WRITE_SIZE;


	case FW_CMD_READ:
		max77775_bulk_read(max77775->muic,
				fw_opcode,
				fw_data_len,
				verify_data);
		/*
		 * Check fw data sequence number
		 * It should be increased from 1 step by step.
		 */
		if (memcmp(verify_data, &fw_data[1], 2)) {
			md75_info_usb("%s: [0x%02x 0x%02x], [0x%02x, 0x%02x], [0x%02x, 0x%02x]\n",
					__func__,
					verify_data[0], fw_data[0],
					verify_data[1], fw_data[1],
					verify_data[2], fw_data[2]);
			if (verify_data[0] == 0xF5 && verify_data[1] == 0x10)
				return FW_UPDATE_ERR_CODE_FAIL;
			else
				return FW_UPDATE_VERIFY_FAIL;
		}

		return FW_CMD_READ_SIZE;
	}

	md75_info_usb("%s: Command error\n", __func__);

	return ret;
}

int max77775_write_fw_noautoibus(struct max77775_dev *max77775)
{
	u8 write_values[OPCODE_MAX_LENGTH] = { 0, };
	int ret = 0;
	int length = 0x1;
	int i = 0;

	write_values[0] = OPCODE_SAMSUNG_FW_AUTOIBUS;
	write_values[1] = 0x3; /* usbc fw off & auto off(manual on) */

	for (i = 0; i < length + OPCODE_SIZE; i++)
		md75_info_usb("%s: [%d], 0x[%x]\n", __func__, i, write_values[i]);

	/* Write opcode and data */
	ret = max77775_bulk_write(max77775->muic, OPCODE_WRITE,
			length + OPCODE_SIZE, write_values);
	/* Write end of data by 0x00 */
	if (length < OPCODE_DATA_LENGTH)
		max77775_write_reg(max77775->muic, OPCODE_WRITE_END, 0x00);
	return 0;
}

static int max77775_fuelgauge_read_vcell(struct max77775_dev *max77775)
{
	u8 data[2];
	u32 vcell;
	u16 w_data;
	u32 temp;
	u32 temp2;

	if (max77775_bulk_read(max77775->fuelgauge, MAX77775_FG_REG_VCELL, 2, data) < 0) {
		md75_err_usb("%s: Failed to read VCELL\n", __func__);
		return -1;
	}

	w_data = (data[1] << 8) | data[0];

	temp = (w_data & 0xFFF) * 78125;
	vcell = temp / 1000000;

	temp = ((w_data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	vcell += (temp2 << 4);

	return vcell;
}

static int max77775_fuelgauge_read_vbyp(struct max77775_dev *max77775)
{
	u8 data[2];
	u32 vbyp, temp;
	u16 w_data;

	if (max77775_bulk_read(max77775->fuelgauge, MAX77775_FG_REG_VBYP, 2, data) < 0) {
		pr_err("%s: Failed to read MAX77775_FG_REG_VBYP\n", __func__);
		return -1;
	}

	w_data = (data[1] << 8) | data[0];

	temp = (w_data & 0xFFF) * 427246;
	vbyp = temp / 1000000;

	temp = ((w_data & 0xF000) >> 4) * 427246;
	temp /= 1000000;
	vbyp += (temp << 4);

	return vbyp;
}

#if defined(CONFIG_SEC_FACTORY)
static int max77775_fuelgauge_read_current_ma(struct max77775_dev *max77775)
{
	u8 data1[2];
	u32 temp, sign;
	s32 i_current;

	if (max77775_bulk_read(max77775->fuelgauge, MAX77775_FG_REG_CURRENT, 2, data1) < 0) {
		pr_err("%s: Failed to read MAX77775_FG_REG_CURRENT\n", __func__);
		return -1;
	}

	temp = ((data1[1] << 8) | data1[0]) & 0xFFFF;
	/* Debug log for abnormal current case */
	if (temp & (0x1 << 15)) {
		sign = 1;
		temp = (~temp & 0xFFFF) + 1;
	} else {
		sign = 0;
	}

	i_current = temp * 15625 * (max77775->pdata->fg_resistor) / 100000;

	if (sign)
		i_current *= -1;

	return i_current;
}
#endif

static void max77775_wc_control(struct max77775_dev *max77775, bool enable)
{
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	union power_supply_propval value = {0, };
	char wpc_en_status[2];
	int ret = 0;

	wpc_en_status[0] = WPC_EN_CCIC;
	wpc_en_status[1] = enable ? true : false;
	value.strval = wpc_en_status;
	ret = psy_do_property(max77775->pdata->wireless_charger_name, set, POWER_SUPPLY_EXT_PROP_WPC_EN, value);

	if (ret < 0) {
		if (max77775->pdata->wpc_en) {
			if (enable) {
				gpio_direction_output(max77775->pdata->wpc_en, 0);
				md75_info_usb("%s: WC CONTROL: ENABLE\n", __func__);
			} else {
				gpio_direction_output(max77775->pdata->wpc_en, 1);
				md75_info_usb("%s: WC CONTROL: DISABLE\n", __func__);
			}
		} else {
			md75_info_usb("%s : no wpc_en\n", __func__);
		}
	} else {
		md75_info_usb("%s: WC CONTROL: %s\n", __func__, wpc_en_status[1] ? "Enable" : "Disable");
	}

	md75_info_usb("%s: wpc_en(%d)\n", __func__, gpio_get_value(max77775->pdata->wpc_en));
#endif
}

#if defined(CONFIG_SEC_FACTORY)
bool max77775_is_factory(struct max77775_dev *max77775)
{
	bool is_factory = false;
	u8 chgin_dtls;
	u8 fctid = 7;
	u8 uidadc = 7;

	max77775_read_reg(max77775->charger, MAX77775_CHG_REG_DETAILS_00, &chgin_dtls);
	chgin_dtls = ((chgin_dtls & 0x60) >> 5);

	if (max77775->FW_Revision == 0xFF) {
		is_factory = (chgin_dtls == 3) ? true : false;
		md75_info_usb("%s: FW_Revision=0x%02X chgin_dtls=0x%02X is_factory=%d(forced)\n", __func__, max77775->FW_Revision, chgin_dtls, is_factory);
		return is_factory;
	}

	max77775_read_reg(max77775->muic, MAX77775_USBC_REG_USBC_STATUS1, &uidadc);
	uidadc = uidadc & 0x07;
	switch (uidadc) {
	case 3:	/* 255K */
	case 4:	/* 301K */
	case 5:	/* 523K */
	case 6:	/* 619K */
		if (chgin_dtls == 3)
			is_factory = true;
		break;
	default:
		break;
	}

	max77775_read_reg(max77775->muic, MAX77775_USBC_REG_PD_STATUS2, &fctid);
	fctid = fctid & 0x0F;
	switch (fctid) {
	case 3:	/* 255K */
	case 4:	/* 301K */
	case 5:	/* 523K */
	case 6:	/* 619K */
		if (chgin_dtls == 3)
			is_factory = true;
		break;
	default:
		break;
	}

	md75_info_usb("%s: FW_Revision=0x%02X, chgin_dtls=%x, uidadc=%x, fctid=%x, is_factory=%d\n",
			__func__, max77775->FW_Revision, chgin_dtls, uidadc, fctid, (int)is_factory);
	return is_factory;
}

bool max77775_is_uid(struct max77775_dev *max77775)
{
	bool is_uid = false;
	u8 uidadc = 7;

	max77775_read_reg(max77775->muic, MAX77775_USBC_REG_USBC_STATUS1, &uidadc);
	uidadc = uidadc & 0x07;
	switch (uidadc) {
	case 3:	/* 255K */
	case 4:	/* 301K */
	case 5:	/* 523K */
	case 6:	/* 619K */
			is_uid = true;
		break;
	default:
		break;
	}
	md75_info_usb("%s: uidadc=%x, is_uid=%d\n",
			__func__, uidadc, (int)is_uid);
	return is_uid;
}
#endif /* CONFIG_SEC_FACTORY */

void max77775_set_dpdm_gnd(struct max77775_dev *max77775)
{
	md75_info_usb("%s : Set DPDN GND\n", __func__);
	max77775_write_reg(max77775->muic, OPCODE_WRITE, 0x04);
	max77775_write_reg(max77775->muic, OPCODE_DATAOUT1, 0x10);
	max77775_write_reg(max77775->muic, OPCODE_WRITE_END, 0x00);
	msleep(150);
}

int max77775_usbc_fw_update(struct max77775_dev *max77775,
		const u8 *fw_bin, int fw_bin_len, int enforce_do)
{
	max77775_fw_header *fw_header;
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif
	int offset = 0;
	unsigned long duration = 0;
	int size = 0;
	int try_count = 0;
	int ret = 0;
	u8 pmicrev = 0x00;
	u8 usbc_status1 = 0x0;
	u8 pd_status2 = 0x0;
	static u8 fct_id; /* FCT cable */
	u8 uidadc; /* FCT cable */
	u8 try_command = 0;
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	u8 sw_boot = 0;
#endif
	u8 chg_cnfg_00 = 0;
	u8 chg_cnfg_11 = 0;
	u8 chg_cnfg_12 = 0;
	bool recovery_needed = false;
	bool wpc_en_changed = 0;
	int vcell = 0;
#if defined(CONFIG_SEC_FACTORY)	
	s32 i_current = 0;
#endif
	int vbyp = 0;
	u8 recv_opcode = 0;
	u8 noAutoIbus = 0;
	u8 chgin_lim = 0;
	u8 chgin_dtls = 0;
	u8 wcin_dtls = 0;
	u8 vbadc = 0;
	bool is_factory = false;
	bool is_skip = false;
	int error = 0;
	bool is_testsid = false;
#if defined(CONFIG_SEC_FACTORY)
	int current_fct_id = 0;
#endif

	u8 bc_status = 0;
	u8 chg_type = 0;
	u8 set_dpdn_gnd = 0;
	bool is_err_code = false;
	int fwupdate_retry_cnt = FW_VERIFY_TRY_COUNT;
	int opcode_retry_cnt = 0;
	int opcode2_retry_cnt = 0;

	max77775->fw_size = fw_bin_len;
	fw_header = (max77775_fw_header *)fw_bin;
	md75_info_usb("%s: FW_CHK: magic/%x/ major/%x/ minor/%x/ id/%x/ rev/%x/\n",
			__func__, fw_header->magic, fw_header->major,
			fw_header->minor, fw_header->id, fw_header->rev);

	max77775_read_reg(max77775->i2c, MAX77775_PMIC_REG_PMICREV, &pmicrev);
	if (max77775->required_hw_rev != (pmicrev & 0x7)) {
		md75_info_usb("%s: FW_SKIP: hw_rev mismatch. required_hw_rev=%x:pmicrev=%x\n",
			__func__, max77775->required_hw_rev, pmicrev);
		return 0;
	}

	if (max77775->required_fw_pid != fw_header->id) {
		md75_info_usb("%s: FW_SKIP: product id mismatch. required_fw_pid=%x:fw_header:%x\n",
			__func__, max77775->required_fw_pid, fw_header->id);
		return 0;
	}

	if (fw_header->magic == MAX77775_SIGN)
		md75_info_usb("%s: FW_MAGIC: matched\n", __func__);

	max77775_read_reg(max77775->charger, MAX77775_CHG_REG_CNFG_00, &chg_cnfg_00);
	max77775_read_reg(max77775->charger, MAX77775_CHG_REG_CNFG_11, &chg_cnfg_11);
	max77775_read_reg(max77775->charger, MAX77775_CHG_REG_CNFG_12, &chg_cnfg_12);
	md75_info_usb("%s: FW_INFO: chg_cnfg_00=0x%02X | chg_cnfg_11=0x%02X | chg_cnfg_12=0x%02X\n",
			__func__, chg_cnfg_00, chg_cnfg_11, chg_cnfg_12);

#if defined(CONFIG_SEC_FACTORY)
	max77775_read_reg(max77775->muic, REG_UIC_FW_REV, &max77775->FW_Revision);
	is_factory = max77775_is_factory(max77775);
	if (is_factory) {
		if (max77775->FW_Revision != 0xFF) {
			max77775_read_reg(max77775->muic, REG_USBC_STATUS1, &usbc_status1);
			vbadc = (usbc_status1 & BIT_VBADC) >> FFS(BIT_VBADC);
		} else
			md75_info_usb("%s: vbadc check was skipped\n", __func__);
	}
	is_skip = max77775_is_uid(max77775);
#endif /* CONFIG_SEC_FACTORY */

	max77775_read_reg(max77775->muic, REG_BC_STATUS, &bc_status);
	chg_type = (bc_status & BIT_ChgTyp) >> FFS(BIT_ChgTyp);

	disable_irq(max77775->irq);
retry:
	md75_info_usb("%s: FW_TRY: try_count=%d, try_command=%d\n",
			__func__, try_count, try_command);
	max77775_write_reg(max77775->muic, REG_PD_INT_M, 0xFF);
	max77775_write_reg(max77775->muic, REG_CC_INT_M, 0xFF);
	max77775_write_reg(max77775->muic, REG_UIC_INT_M, 0xFF);
	max77775_write_reg(max77775->muic, REG_VDM_INT_M, 0xFF);
#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
	max77775_write_reg(max77775->muic, REG_SPARE_INT_M, 0xFF);
#endif

	offset = 0;
	duration = 0;
	size = 0;
	ret = 0;
	opcode_retry_cnt = 0;
	opcode2_retry_cnt = 0;

	ret = max77775_read_reg(max77775->muic, REG_PRODUCT_ID, &max77775->FW_Product_ID);
	ret = max77775_read_reg(max77775->muic, REG_UIC_FW_REV, &max77775->FW_Revision);
	ret = max77775_read_reg(max77775->muic, REG_UIC_FW_REV2, &max77775->FW_Minor_Revision);

	max77775->FW_Product_ID_bin = fw_header->id;
	max77775->FW_Revision_bin = fw_header->major;
	max77775->FW_Minor_Revision_bin = fw_header->minor;

	if (ret < 0 && (try_count == 0 && try_command == 0)) {
		md75_info_usb("%s: FW_READFAILED: Failed to read FW_REV\n", __func__);
		error = -EIO;
		goto out;
	}

	duration = jiffies;

	md75_info_usb("%s: FW_INFO: chip : %02X.%02X(PID%02X), bin : %02X.%02X(PID%02X)\n",
			__func__, max77775->FW_Revision,
			max77775->FW_Minor_Revision, max77775->FW_Product_ID,
			fw_header->major, fw_header->minor, fw_header->id);
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	store_ccic_bin_version(&fw_header->major, &sw_boot);
#endif

	if ((max77775->FW_Revision == 0xff) ||
		enforce_do ||
		(!is_skip &&
		((max77775->FW_Revision != fw_header->major) ||
		(max77775->FW_Minor_Revision != fw_header->minor) ||
		(max77775->FW_Product_ID != fw_header->id)))) {

		if (IS_ENABLED(CONFIG_SEC_FACTORY_INTERPOSER) && !enforce_do) {
			md75_err_usb("%s: Skip fw update on secondary factory binary\n", __func__);
			error = -EINVAL;
			goto out;
		}

#if defined(CONFIG_SEC_FACTORY)
		max77775_read_reg(max77775->muic, MAX77775_USBC_REG_USBC_STATUS1, &usbc_status1);
		uidadc = (usbc_status1 & BIT_UIDADC) >> FFS(BIT_UIDADC);
		max77775_read_reg(max77775->muic, REG_PD_STATUS2, &pd_status2);
		fct_id = (pd_status2 & BIT_FCT_ID) >> FFS(BIT_FCT_ID);
		if ((is_testsid == false) && ((uidadc == 6) || (uidadc == 5) || (uidadc == 4) || (fct_id == 4) || (fct_id == 5))) {
			ret = max77775_write_reg(max77775->i2c, 0xFE, 0xC5);
			ret = max77775_write_reg(max77775->testsid, 0x9B, 0x20);
			ret = max77775_write_reg(max77775->i2c, 0xFE, 0x00);
			is_testsid = true;
			current_fct_id = fct_id;
			md75_info_usb("%s: testsid(%d)\n", __func__, (int)is_testsid);
	} else if (is_factory && (max77775->FW_Revision == 0xFF)) {
		/* Check VCell */
		vcell = max77775_fuelgauge_read_vcell(max77775);
		pr_info("%s: vcell(%dmv)\n", __func__, vcell);

		/* Check Current */
		i_current = max77775_fuelgauge_read_current_ma(max77775);
		pr_info("%s: current(%dmA)\n", __func__, i_current);

		/* Check the No Battery Status */
		if (vcell < 2000 && (i_current < 20 && i_current > -20)) {
			max77775_update_reg(max77775->charger, MAX77775_CHG_REG_CNFG_09, 0x7F, 0x7F);

			ret = max77775_write_reg(max77775->i2c, 0xFE, 0xC5);
			ret = max77775_write_reg(max77775->testsid, 0x9B, 0x20);
			ret = max77775_write_reg(max77775->i2c, 0xFE, 0x00);
			is_testsid = true;
			pr_info("%s : testsid(%d)\n", __func__, (int)is_testsid);
		}
	}
#endif /* CONFIG_SEC_FACTORY */

		if (!enforce_do) {	/* on Booting time */
			max77775_read_reg(max77775->muic, REG_PD_STATUS2, &pd_status2);
			fct_id = (pd_status2 & BIT_FCT_ID) >> FFS(BIT_FCT_ID);

			max77775_read_reg(max77775->muic, REG_USBC_STATUS1, &usbc_status1);
			uidadc = (usbc_status1 & BIT_UIDADC) >> FFS(BIT_UIDADC);
			md75_info_usb("%s: FCT_ID: 0x%x UIDADC: 0x%x\n", __func__,
					fct_id, uidadc);
		}

		max77775_read_reg(max77775->charger, MAX77775_CHG_REG_DETAILS_00, &wcin_dtls);
		wcin_dtls = (wcin_dtls & 0x18) >> 3;

		wpc_en_changed = true;
		max77775_wc_control(max77775, false);

		max77775_read_reg(max77775->charger, MAX77775_CHG_REG_DETAILS_00, &chgin_dtls);
		chgin_dtls = ((chgin_dtls & 0x60) >> 5);

		vbyp = max77775_fuelgauge_read_vbyp(max77775);

		md75_info_usb("%s: FW_INFO: chgin_dtls:0x%x, wcin_dtls:0x%x, vbadc=%x, is_factory=%d, vbyp=%d\n",
				__func__, chgin_dtls, wcin_dtls, vbadc, (int)is_factory, vbyp);

		if (try_count == 0 && try_command == 0) {

			if (is_factory) {
				if ((chgin_dtls == 3) && (vbyp > 4500 && vbyp < 5800)) {
					/* In case of : TA only connected, REV is OK or 0xFF
					 *				the vbus would be 5V by ic-reset if REV was 0xFF in pre boot-up
					 *	vbadc limit was changed to include 3 by SS request
					 */

					if (max77775->FW_Revision == 0xFF) {
					/* Set ChgMode to 0x04 */
					max77775_update_reg(max77775->charger,
								MAX77775_CHG_REG_CNFG_00, 0x04, 0x0F);
					md75_info_usb("%s: FW_INFO: +change chg_mode(4), vbadc(%d), vbyp(%d)\n",
							__func__, vbadc, vbyp);
					recovery_needed = true;
				} else {
						max77775_write_reg(max77775->muic, OPCODE_WRITE, OPCODE_CHG_CONTROL1_R);
						max77775_write_reg(max77775->muic, OPCODE_WRITE_END, 0x00);
wait_opcode1:
						msleep(5);
						max77775_read_reg(max77775->muic, OPCODE_READ, &recv_opcode);

						if (recv_opcode == OPCODE_CHG_CONTROL1_R) {
							max77775_read_reg(max77775->muic, MAX77775_USBC_REG_AP_DATAIN1, &noAutoIbus);
							noAutoIbus = ((noAutoIbus & 0x10) >> 4);

							if (noAutoIbus) {
								pr_info("%s: Read ILIM is controlled by CHG\n", __func__);
								max77775_read_reg(max77775->charger, MAX77775_CHG_REG_CNFG_09, &chgin_lim);
							} else {
								recv_opcode = 0;

								pr_info("%s: Read ILIM is controlled by USB\n", __func__);
								max77775_write_reg(max77775->muic, OPCODE_WRITE, OPCODE_CHGIN_ILIM_R);
								max77775_write_reg(max77775->muic, OPCODE_WRITE_END, 0x00);
wait_opcode2:
								msleep(5);
								max77775_read_reg(max77775->muic, OPCODE_READ, &recv_opcode);

								if (recv_opcode == OPCODE_CHGIN_ILIM_R)
									max77775_read_reg(max77775->muic, MAX77775_USBC_REG_AP_DATAIN1, &chgin_lim);
								else {
									if (opcode2_retry_cnt < 5) {
										opcode2_retry_cnt++;
										pr_info("%s: Failed to receive OPCODE_CHGIN_ILIM_R : opcode2_retry_cnt(%d)\n", __func__, opcode2_retry_cnt);
										goto wait_opcode2;
									} else {
										if (++try_count < fwupdate_retry_cnt) {
											pr_info("%s: Failed to receive OPCODE_CHGIN_ILIM_R : try_count(%d)\n", __func__, try_count);
											goto retry;
										} else {
											pr_info("%s: Failed to receive OPCODE_CHGIN_ILIM_R\n", __func__);
											error = -EAGAIN;
											goto out;
										}
									}
								}
							}

							chgin_lim = (chgin_lim & 0x7F);

							/* Chgin_lim > 1500mA */
							if (chgin_lim > 0x3B)  {
								/* Set ChgMode to 0x04 */
								max77775_update_reg(max77775->charger, MAX77775_CHG_REG_CNFG_00, 0x04, 0x0F);
								pr_info("%s: FW_INFO: +change chg_mode(4), vbadc(%d), vbyp(%d)\n", __func__, vbadc, vbyp);
								recovery_needed = true;
							} else {
								pr_info("%s: chgin_lim is not valid(0x%02X)\n", __func__, chgin_lim);
								goto out;
							}
						} else {
							if (opcode_retry_cnt < 5) {
								opcode_retry_cnt++;
								pr_info("%s: Failed to receive OPCODE_CHG_CONTROL1_R : opcode_retry_cnt(%d)\n", __func__, opcode_retry_cnt);
								goto wait_opcode1;
							} else {
								if (++try_count < fwupdate_retry_cnt) {
									pr_info("%s: Failed to receive OPCODE_CHG_CONTROL1_R : try_count(%d)\n", __func__, try_count);
									goto retry;
								} else {
									pr_info("%s: Failed to receive OPCODE_CHG_CONTROL1_R : go out\n", __func__);
									error = -EAGAIN;
									goto out;
								}
							}
						}
					}
				} else {
					pr_info("%s: chgin is not valid(%d) or out of vbyp(%d)\n", __func__, chgin_dtls, vbyp);
					goto out;
				}
			} else {
				if (max77775->FW_Revision == 0xFF) {
					if (chgin_dtls == 3) {
						/* In case of : TA connected, REV is 0xFF */

						md75_info_usb("%s: FW_INFO: Valid CHGIN_DTLS\n", __func__);

						/* Set the CHG_ILIM 500mA */
						max77775_update_reg(max77775->charger, MAX77775_CHG_REG_CNFG_09, 0x13, 0x7F);
						md75_info_usb("%s: FW_INFO: CHG_ILIM 500mA\n", __func__);

						max77775_write_reg(max77775->i2c, 0xFE, 0xC5);
						max77775_write_reg(max77775->testsid, 0x9B, 0x20);
						max77775_write_reg(max77775->i2c, 0xFE, 0x00);
						is_testsid = true;
						md75_info_usb("%s : testsid(%d)\n", __func__, (int)is_testsid);

						/* Set ChgMode to 0x00 */
						max77775_update_reg(max77775->charger, MAX77775_CHG_REG_CNFG_00, 0x00, 0x0F);
						md75_info_usb("%s: FW_INFO: +change chg_mode(0)\n", __func__);

						usleep_range(100, 200);

						/* Set ChgMode to 0x05 */
						max77775_update_reg(max77775->charger, MAX77775_CHG_REG_CNFG_00, 0x05, 0x0F);
						md75_info_usb("%s: FW_INFO: +change chg_mode(5)\n", __func__);

						recovery_needed = true;

						if (vbyp > 6000) {
							pr_info("%s: FW_SKIP: byp(%dmv) > 6000mv\n", __func__, vbyp);
							error = -EAGAIN;
							goto out;
						}
						md75_info_usb("%s: FW_INFO: byp(%dmv) < 6000mv\n", __func__, vbyp);
					} else {
						/* In case of : battery connected, REV is 0xFF */

						md75_info_usb("%s: FW_INFO: Invalid CHGIN_DTLS\n", __func__);

						/* Set Chginsel and Wcinsel to 0 */
						max77775_update_reg(max77775->charger, MAX77775_CHG_REG_CNFG_12, 0x00, 0x60);
						md75_info_usb("%s: FW_INFO: +disable CHGINSEL(0) / WCINSEL(0)\n", __func__);

						usleep_range(100, 200);

						/* Set ChgMode to 0x00 */
						max77775_update_reg(max77775->charger, MAX77775_CHG_REG_CNFG_00, 0x00, 0x0F);
						md75_info_usb("%s: FW_INFO: +change chg_mode(0)\n", __func__);

						/* Set VBypSet to 0x00 */
						max77775_write_reg(max77775->charger, MAX77775_CHG_REG_CNFG_11, 0x00);
						md75_info_usb("%s: FW_INFO: +clear VBYPSET(0x00)\n", __func__);

						usleep_range(100, 200);

						/* Set ChgMode to 0x09 */
						max77775_update_reg(max77775->charger, MAX77775_CHG_REG_CNFG_00, 0x09, 0x0F);
						md75_info_usb("%s: FW_INFO: +change chg_mode(9)\n", __func__);

						recovery_needed = true;
					}
				} else {
					/* In case of : battery connected, REV is OK */

					pr_info("%s: FW_INFO: REV is OK\n", __func__);

					/* Check VCell */
					vcell = max77775_fuelgauge_read_vcell(max77775);
					if (vcell < 3600) {
						pr_info("%s: FW_SKIP: keep chg_mode(0x%x), vcell(%dmv) < 3600mv\n", __func__, chg_cnfg_00 & 0x0F, vcell);
						error = -EAGAIN;
						goto out;
					}
					pr_info("%s: FW_INFO: vcell(%dmv) > 3600mv\n", __func__, vcell);

					/* Set Chginsel and Wcinsel to 0 */
					max77775_update_reg(max77775->charger, MAX77775_CHG_REG_CNFG_12, 0x00, 0x60);
					pr_info("%s: FW_INFO: +disable CHGINSEL(0) / WCINSEL(0)\n", __func__);

					usleep_range(100, 200);

					/* Set ChgMode to 0x00 */
					max77775_update_reg(max77775->charger, MAX77775_CHG_REG_CNFG_00, 0x00, 0x0F);
					pr_info("%s: FW_INFO: +change chg_mode(0)\n", __func__);

					/* Set VBypSet to 0x00 */
					max77775_write_reg(max77775->charger, MAX77775_CHG_REG_CNFG_11, 0x00);
					pr_info("%s: FW_INFO: +clear VBYPSET(0x00)\n", __func__);

					usleep_range(100, 200);

					/* Set ChgMode to 0x09 */
					max77775_update_reg(max77775->charger, MAX77775_CHG_REG_CNFG_00, 0x09, 0x0F);
					pr_info("%s: FW_INFO: +change chg_mode(9)\n", __func__);

					recovery_needed = true;
				}
			}
		}

		if (chg_type == 0x03 /*CHGTYP_DEDICATED_CHARGER*/ && set_dpdn_gnd == 0x0 && max77775->FW_Revision != 0xFF) {
			pr_info("%s : Set DPDN GND\n", __func__);
			set_dpdn_gnd = 1;
			max77775_write_reg(max77775->muic, OPCODE_WRITE, 0x04);
			max77775_write_reg(max77775->muic, OPCODE_DATAOUT1, 0x10);
			max77775_write_reg(max77775->muic, OPCODE_WRITE_END, 0x00);
			msleep(150);
		}

		if (is_err_code == false) {
			msleep(150);

			max77775_write_reg(max77775->muic, OPCODE_WRITE, 0xD0);
			max77775_write_reg(max77775->muic, OPCODE_WRITE_END, 0x00);
			msleep(300);
		}

		max77775_read_reg(max77775->muic, REG_UIC_FW_REV, &max77775->FW_Revision);
		max77775_read_reg(max77775->muic, REG_UIC_FW_REV2, &max77775->FW_Minor_Revision);
		md75_info_usb("%s: FW_START: (%02X.%02X)\n", __func__,
				max77775->FW_Revision,
				max77775->FW_Minor_Revision);

		if (max77775->FW_Revision != 0xFF) {
			if (++try_command < FW_SECURE_MODE_TRY_COUNT) {
				md75_info_usb("%s: FW_FAILED: the Fail to enter secure mode %d\n",
						__func__, try_command);
				max77775_reset_ic(max77775);
				if (is_err_code == true) {
					msleep(150);

					max77775_write_reg(max77775->muic, OPCODE_WRITE, 0xD0);
					max77775_write_reg(max77775->muic, OPCODE_WRITE_END, 0x00);
					msleep(300);
				}
				goto retry;
			} else {
				md75_info_usb("%s: FW_FAILED: the Secure Update Fail!!\n",
						__func__);
#if defined(CONFIG_USB_HW_PARAM)
				if (o_notify)
					inc_hw_param(o_notify, USB_CCIC_FWUP_ERROR_COUNT);
#endif
				error = -EIO;
				goto out;
			}
		}

		try_command = 0;

		for (offset = FW_HEADER_SIZE;
				offset < fw_bin_len && size != FW_UPDATE_END;) {

			size = __max77775_usbc_fw_update(max77775, &fw_bin[offset]);

			switch (size) {
			case FW_UPDATE_ERR_CODE_FAIL:
				pr_err("%s: FW_ERR_CODE_FAIL\n", __func__);
				if (is_err_code == false) {
					is_err_code = true;
					fwupdate_retry_cnt += FW_ERR_CODE_TRY_COUNT;
					pr_info("%s: FW_ERR_CODE_FAIL : Increased retry count (%d)\n", __func__, fwupdate_retry_cnt);
				}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
				fallthrough;
#endif
			case FW_UPDATE_VERIFY_FAIL:
				md75_err_usb("%s: FW_VERIFY_FAIL\n", __func__);
				offset -= FW_CMD_WRITE_SIZE;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
				fallthrough;
#endif
			case FW_UPDATE_TIMEOUT_FAIL:
				/*
				 * Retry FW updating
				 */
				if (++try_count < fwupdate_retry_cnt) {
					md75_info_usb("%s: FW_TIMEOUT: Retry fw write. ret %d, count %d, offset %d\n",
							__func__, size,
							try_count, offset);
					max77775_reset_ic(max77775);
					goto retry;
				} else {
					md75_info_usb("%s: FW_TIMEOUT: Failed to update FW. ret %d, offset %d\n",
							__func__, size,
							(offset + size));
#if defined(CONFIG_USB_HW_PARAM)
					if (o_notify)
						inc_hw_param(o_notify, USB_CCIC_FWUP_ERROR_COUNT);
#endif
					error = -EIO;
					goto out;
				}
				break;
			case FW_UPDATE_CMD_FAIL:
			case FW_UPDATE_MAX_LENGTH_FAIL:
				md75_info_usb("%s: FW_LENGTH_FAIL: Failed to update FW. ret %d, offset %d\n",
						__func__, size, (offset + size));
#if defined(CONFIG_USB_HW_PARAM)
				if (o_notify)
					inc_hw_param(o_notify, USB_CCIC_FWUP_ERROR_COUNT);
#endif
				error = -EIO;
				goto out;
			case FW_UPDATE_END: /* 0x00 */
				max77775_read_reg(max77775->muic,
						REG_UIC_FW_REV, &max77775->FW_Revision);
				max77775_read_reg(max77775->muic,
						REG_UIC_FW_REV2, &max77775->FW_Minor_Revision);
				max77775_read_reg(max77775->muic,
						REG_PRODUCT_ID, &max77775->FW_Product_ID);
				md75_info_usb("%s: chip : %02X.%02X(PID%02X), bin : %02X.%02X(PID%02X)\n",
						__func__, max77775->FW_Revision,
						max77775->FW_Minor_Revision,
						max77775->FW_Product_ID,
						fw_header->major,
						fw_header->minor,
						fw_header->id);
				md75_info_usb("%s: FW_COMPLETED\n", __func__);

				if (max77775_get_facmode())
					max77775_write_fw_noautoibus(max77775);

				break;
			default:
				offset += size;
				break;
			}
			if (offset == fw_bin_len) {
				max77775_reset_ic(max77775);
				max77775_read_reg(max77775->muic,
						REG_UIC_FW_REV, &max77775->FW_Revision);
				max77775_read_reg(max77775->muic,
						REG_UIC_FW_REV2, &max77775->FW_Minor_Revision);
				max77775_read_reg(max77775->muic,
						REG_PRODUCT_ID, &max77775->FW_Product_ID);
				md75_info_usb("%s: FW_INFO: chip : %02X.%02X(PID%02X), bin : %02X.%02X(PID%02X)\n",
						__func__, max77775->FW_Revision,
						max77775->FW_Minor_Revision,
						max77775->FW_Product_ID,
						fw_header->major,
						fw_header->minor,
						fw_header->id);

				md75_info_usb("%s: FW COMPLETED via SYS path\n",
						__func__);
			}
		}
	} else {
		md75_info_usb("%s: FW_SKIP: Don't need to update!\n", __func__);
		goto out;
	}

	duration = jiffies - duration;
	md75_info_usb("%s: FW_OK Duration : %dms\n", __func__,
			jiffies_to_msecs(duration));

out:
	if (recovery_needed) {
		if (is_factory) {
			max77775_update_reg(max77775->charger,
					MAX77775_CHG_REG_CNFG_00, chg_cnfg_00, 0x0F);
			md75_info_usb("%s: FW_INFO: -recover ChgMode(%d) -> chg_cnfg_00=0x%02X, vbadc(%d)\n",
					__func__, chg_cnfg_00 & 0x0F, chg_cnfg_00, vbadc);

		} else {
			/* Set ChgMode to 0x00 */
			max77775_update_reg(max77775->charger, MAX77775_CHG_REG_CNFG_00, 0x00, 0x0F);
			md75_info_usb("%s: FW_INFO: -recover chg_mode(0)\n", __func__);

			/* Recover VBypSet */
			max77775_write_reg(max77775->charger,
						MAX77775_CHG_REG_CNFG_11, chg_cnfg_11);
			md75_info_usb("%s: FW_INFO: -recover VBYPSET(0x%02X)\n",
					__func__, chg_cnfg_11);

			usleep_range(100, 200);

			/* Recover ChgMode */
			max77775_update_reg(max77775->charger,
					MAX77775_CHG_REG_CNFG_00, chg_cnfg_00, 0x0F);
			md75_info_usb("%s: FW_INFO: -recover ChgMode(%d) -> chg_cnfg_00=0x%02X\n",
					__func__, chg_cnfg_00 & 0x0F,
					chg_cnfg_00);

			usleep_range(100, 200);

			/* Recover Chginsel and Wcinsel */
			max77775_update_reg(max77775->charger, MAX77775_CHG_REG_CNFG_12,
					chg_cnfg_12, 0x60);
			pr_info("%s: FW_INFO: -recover CHGINSEL(%d) / WCINSEL(%d) -> chg_cnfg_12=0x%02X\n",
					__func__, chg_cnfg_12 & 0x20 ? 1 : 0,
					chg_cnfg_12 & 0x40 ? 1 : 0, chg_cnfg_12);

			usleep_range(100, 200);
		}
	}

	if (wpc_en_changed) {
		max77775_wc_control(max77775, true);
	}
	enable_irq(max77775->irq);

	if (is_testsid == true) {
#if defined(CONFIG_SEC_FACTORY)
		if (current_fct_id == 4 || current_fct_id == 5) {
			pr_info("%s : Apply 2sec, fct_id (%d)\n", __func__, fct_id);
			msleep(2000);
		}
#endif /* CONFIG_SEC_FATORY */
		max77775_write_reg(max77775->i2c, 0xFE, 0xC5);
		max77775_write_reg(max77775->testsid, 0x9B, 0x00);
		max77775_write_reg(max77775->i2c, 0xFE, 0x00);
		is_testsid = false;
		md75_info_usb("%s: testsid(%d)\n", __func__, (int)is_testsid);
	}

	return error;
}
EXPORT_SYMBOL_GPL(max77775_usbc_fw_update);

static void __max77775_usbc_fw_setting(struct max77775_dev *max77775,
		const struct firmware *fw, char *fw_bin_name, int enforce_do)
{
	md75_info_usb("%s: fw update (name=%s size=%lu enforce_do=%d)\n",
			__func__, fw_bin_name, fw->size, enforce_do);

	max77775_usbc_fw_update(max77775, fw->data, fw->size, enforce_do);
}

int max77775_usbc_fw_setting(struct max77775_dev *max77775, int enforce_do)
{
	int err = 0;
	const struct firmware *fw;
	char *fw_bin_name = FW_BIN_NAME;

	if (max77775->pdata->extra_fw_enable)
		fw_bin_name = EXTRA_FW_BIN_NAME;

	err = request_firmware(&fw, fw_bin_name, max77775->dev);
	if (!err) {
		__max77775_usbc_fw_setting(max77775, fw, fw_bin_name, enforce_do);
		release_firmware(fw);
	}
	return err;
}
EXPORT_SYMBOL_GPL(max77775_usbc_fw_setting);
#endif /* CONFIG_CCIC_MAX77775 */

static u8 max77775_revision_check(u8 pmic_id, u8 pmic_rev)
{
	int i, logical_id = 0;
	int pmic_arrary = ARRAY_SIZE(max77775_revision);

	md75_info_usb("%s: pmic_id(0x%02X) pmic_rev(0x%02X)\n",
			__func__, pmic_id, pmic_rev);
	for (i = 0; i < pmic_arrary; i++) {
		md75_info_usb("%s: max77775_revision[%d].id(0x%02X) max77775_revision[%d].rev(0x%02X)\n",
			   __func__,
			   i, max77775_revision[i].id,
			   i, max77775_revision[i].rev);
		if (max77775_revision[i].id == pmic_id &&
		    max77775_revision[i].rev  == pmic_rev)
			logical_id = max77775_revision[i].logical_id;
	}
	md75_info_usb("%s: logical_id(0x%02X)\n", __func__, logical_id);
	return logical_id;
}

void max77775_register_pdmsg_func(struct max77775_dev *max77775,
	void (*check_pdmsg)(void *data, u8 pdmsg), void *data)
{
	if (!max77775) {
		md75_err_usb("%s max77775 is null\n", __func__);
		return;
	}

	max77775->check_pdmsg = check_pdmsg;
	max77775->usbc_data = data;
}
EXPORT_SYMBOL_GPL(max77775_register_pdmsg_func);

static int max77775_check_deferred_probe(struct max77775_dev *max77775, int ret)
{
	static int fw_setting_try_count;

	if (ret && max77775_firmware_timeout_state == 0
			&& !is_recovery_mode_pdic_param()) {
		fw_setting_try_count++;
		md75_info_usb("%s: ret = (%d), return -EPROBE_DEFER(%d)\n",
				__func__, ret, fw_setting_try_count);
		return -EPROBE_DEFER;
	}

	max77775_firmware_timeout_state = MD75_FIRMWARE_TIMEOUT_COMPLETE;

	return 0;
}

static int max77775_firmware_timeout_probe(struct platform_device *pdev)
{
	md75_info_usb("%s firmware_timeout_count=%d\n",
		__func__, firmware_timeout_count);
	return 0;
}

static int max77775_firmware_timeout_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver max77775_firmware_timeout_driver = {
	.driver = {
		.name	= "max77775-firmware-timeout",
	},
	.probe		= max77775_firmware_timeout_probe,
	.remove		= max77775_firmware_timeout_remove,
};

static void firmware_load_timeout_del(void)
{
	platform_device_unregister(pdev);
	platform_driver_unregister(&max77775_firmware_timeout_driver);
}

static void max77775_firmware_load_timeout(struct work_struct *unused)
{
	int error = 0;

	md75_info_usb("%s enter\n", __func__);

	if (max77775_firmware_timeout_state == MD75_FIRMWARE_TIMEOUT_COMPLETE) {
		md75_info_usb("%s already complete\n", __func__);
		goto done;
	}

	max77775_firmware_timeout_state = MD75_FIRMWARE_TIMEOUT_START;

	while (1) {
		firmware_timeout_count++;
		error = platform_driver_register(&max77775_firmware_timeout_driver);
		if (error) {
			md75_err_usb("%s platform_driver_register error %d\n", __func__, error);
			goto err1;
		}

		pdev = platform_device_alloc("max77775-firmware-timeout", PLATFORM_DEVID_AUTO);
		if (!pdev) {
			md75_err_usb("%s pdev error\n", __func__);
			goto err2;
		}

		error = platform_device_add(pdev);
		if (error) {
			md75_err_usb("%s platform_device_add error %d\n", __func__, error);
			goto err3;
		}

		max77775_firmware_timeout_state = MD75_FIRMWARE_TIMEOUT_PASS;

		msleep(1000);

		firmware_load_timeout_del();

		/* 5 times retry. it will call deferred probe */
		if (firmware_timeout_count >= 5 ||
			max77775_firmware_timeout_state == MD75_FIRMWARE_TIMEOUT_COMPLETE)
			break;
	}
done:
	return;
err3:
	platform_device_put(pdev);
err2:
	platform_driver_unregister(&max77775_firmware_timeout_driver);
err1:
	max77775_firmware_timeout_state = MD75_FIRMWARE_TIMEOUT_FAIL;
	md75_info_usb("%s fail\n", __func__);
	return;
}

static DECLARE_DELAYED_WORK(firmware_load_work, max77775_firmware_load_timeout);
#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
static int max77775_i2c_probe(struct i2c_client *i2c)
#else
static int max77775_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *dev_id)
#endif
{
	struct max77775_dev *max77775;
	struct max77775_platform_data *pdata = i2c->dev.platform_data;
	int ret = 0;
	u8 pmic_id, pmic_rev = 0;
#if IS_ENABLED(CONFIG_CCIC_MAX77775)
	const struct firmware *fw = NULL;
	char *fw_bin_name = FW_BIN_NAME;
#endif

	md75_info_usb("%s:%s\n", MFD_DEV_NAME, __func__);

#ifdef CONFIG_USB_USING_ADVANCED_USBLOG
	store_tcpc_name(MFD_DEV_NAME);
#endif

	max77775 = devm_kzalloc(&i2c->dev, sizeof(struct max77775_dev), GFP_KERNEL);
	if (!max77775) {
		ret = -ENOMEM;
		goto err;
	}

	if (i2c->dev.of_node) {
		pdata = devm_kzalloc(&i2c->dev, sizeof(struct max77775_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			ret = -ENOMEM;
			goto err;
		}

#if defined(CONFIG_OF)
		ret = of_max77775_dt(&i2c->dev, pdata);
		if (ret < 0) {
			dev_err(&i2c->dev, "Failed to get device of_node\n");
			goto err;
		}
#endif

		i2c->dev.platform_data = pdata;
	} else
		pdata = i2c->dev.platform_data;

	max77775->dev = &i2c->dev;
	max77775->i2c = i2c;
	max77775->irq = i2c->irq;
	if (pdata) {
		max77775->pdata = pdata;

		pdata->irq_base = devm_irq_alloc_descs(&i2c->dev, -1, 0, MAX77775_IRQ_NR, -1);
		if (pdata->irq_base < 0) {
			md75_err_usb("%s:%s irq_alloc_descs Fail! ret(%d)\n",
					MFD_DEV_NAME, __func__, pdata->irq_base);
			ret = -EINVAL;
			goto err;
		} else
			max77775->irq_base = pdata->irq_base;

		max77775->irq_gpio = pdata->irq_gpio;
		max77775->blocking_waterevent = pdata->blocking_waterevent;
		max77775->required_hw_rev = pdata->rev;
		max77775->required_fw_pid = pdata->fw_product_id;
	} else {
		ret = -EINVAL;
		goto err;
	}

#if IS_ENABLED(CONFIG_CCIC_MAX77775)
	if (max77775->pdata->extra_fw_enable)
		fw_bin_name = EXTRA_FW_BIN_NAME;

	ret = request_firmware(&fw, fw_bin_name, max77775->dev);
	ret = max77775_check_deferred_probe(max77775, ret);
	if (ret)
		goto err;
#endif

	max77775_print_pdata_property(pdata);

	max77775->ws.name = MFD_DEV_NAME;
	wakeup_source_add(&max77775->ws);

	mutex_init(&max77775->i2c_lock);
	init_waitqueue_head(&max77775->suspend_wait);

	i2c_set_clientdata(i2c, max77775);

	if (max77775_read_reg(i2c, MAX77775_PMIC_REG_PMICID, &pmic_id) < 0) {
		dev_err(max77775->dev, "device not found on this channel (this is not an error)\n");
		ret = -ENODEV;
		goto err_w_lock;
	}
	if (max77775_read_reg(i2c, MAX77775_PMIC_REG_PMICREV, &pmic_rev) < 0) {
		dev_err(max77775->dev, "device not found on this channel (this is not an error)\n");
		ret = -ENODEV;
		goto err_w_lock;
	}

	md75_info_usb("%s:%s pmic_id:%x, pmic_rev:%x\n",
		MFD_DEV_NAME, __func__, pmic_id, pmic_rev);

	max77775->pmic_id = pmic_id;
	max77775->pmic_rev = max77775_revision_check(pmic_id, pmic_rev & 0x7);
	if (max77775->pmic_rev == 0) {
		dev_err(max77775->dev, "Can not find matched revision\n");
		ret = -ENODEV;
		goto err_w_lock;
	}

	/* print rev */
	md75_info_usb("%s:%s device found: id:%x rev:%x\n",
		MFD_DEV_NAME, __func__, max77775->pmic_id, max77775->pmic_rev);

	/* No active discharge on safeout ldo 1,2 */
	/* max77775_update_reg(i2c, MAX77775_PMIC_REG_SAFEOUT_CTRL, 0x00, 0x30); */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	max77775->muic = i2c_new_dummy(i2c->adapter, I2C_ADDR_MUIC);
#else
	max77775->muic = i2c_new_dummy_device(i2c->adapter, I2C_ADDR_MUIC);
#endif
	i2c_set_clientdata(max77775->muic, max77775);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	max77775->charger = i2c_new_dummy(i2c->adapter, I2C_ADDR_CHG);
#else
	max77775->charger = i2c_new_dummy_device(i2c->adapter, I2C_ADDR_CHG);
#endif
	i2c_set_clientdata(max77775->charger, max77775);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	max77775->fuelgauge = i2c_new_dummy(i2c->adapter, I2C_ADDR_FG);
#else
	max77775->fuelgauge = i2c_new_dummy_device(i2c->adapter, I2C_ADDR_FG);
#endif
	i2c_set_clientdata(max77775->fuelgauge, max77775);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	max77775->testsid = i2c_new_dummy(i2c->adapter, I2C_ADDR_TESTSID);
#else
	max77775->testsid = i2c_new_dummy_device(i2c->adapter, I2C_ADDR_TESTSID);
#endif
	i2c_set_clientdata(max77775->testsid, max77775);

	/* read PRODUCT_ID, FW_REV, FW_REV2 */
	ret = max77775_read_reg(max77775->muic, REG_PRODUCT_ID, &max77775->FW_Product_ID);
	ret += max77775_read_reg(max77775->muic, REG_UIC_FW_REV, &max77775->FW_Revision);
	ret += max77775_read_reg(max77775->muic, REG_UIC_FW_REV2, &max77775->FW_Minor_Revision);
	if (ret) {
		md75_err_usb("%s: Failed to PRODUCT_ID, UIC_FW_REV and UIC_FW_REV2\n", __func__);
		goto err_i2c;
	}

#if IS_ENABLED(CONFIG_CCIC_MAX77775)
	init_completion(&max77775->fw_completion);
	max77775->fw_workqueue = create_singlethread_workqueue("fw_update");
	if (max77775->fw_workqueue == NULL) {
		ret = -ENOMEM;
		goto err_i2c;
	}
	INIT_WORK(&max77775->fw_work, max77775_usbc_wait_response_q);

	if (fw) {
		__max77775_usbc_fw_setting(max77775, fw, fw_bin_name, 0);
		release_firmware(fw);
		fw = NULL;
	}
#endif

	disable_irq(max77775->irq);
	ret = max77775_irq_init(max77775);
	if (ret < 0)
		goto err_i2c;

	ret = mfd_add_devices(max77775->dev, -1, max77775_devs,
			ARRAY_SIZE(max77775_devs), NULL, 0, NULL);
	if (ret < 0)
		goto err_irq_init;

	ret = device_init_wakeup(max77775->dev, true);
	if (ret < 0)
		goto err_mfd;

	return ret;

err_mfd:
	mfd_remove_devices(max77775->dev);
err_irq_init:
	max77775_irq_exit(max77775);
err_i2c:
	i2c_unregister_device(max77775->muic);
	i2c_unregister_device(max77775->charger);
	i2c_unregister_device(max77775->fuelgauge);
err_w_lock:
	mutex_destroy(&max77775->i2c_lock);
	wakeup_source_remove(&max77775->ws);
#if IS_ENABLED(CONFIG_CCIC_MAX77775)
	if (fw)
		release_firmware(fw);
#endif
err:
	return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int max77775_i2c_remove(struct i2c_client *i2c)
#else
static void max77775_i2c_remove(struct i2c_client *i2c)
#endif
{
	struct max77775_dev *max77775 = i2c_get_clientdata(i2c);

	device_init_wakeup(max77775->dev, false);
	max77775_irq_exit(max77775);
	mfd_remove_devices(max77775->dev);
	i2c_unregister_device(max77775->muic);
	i2c_unregister_device(max77775->charger);
	i2c_unregister_device(max77775->fuelgauge);
	mutex_destroy(&max77775->i2c_lock);
	wakeup_source_remove(&max77775->ws);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	return 0;
#else
	return;
#endif
}

static void max77775_i2c_shutdown(struct i2c_client *i2c)
{
	struct max77775_dev *max77775 = i2c_get_clientdata(i2c);

	max77775_irq_exit(max77775);
	max77775->shutdown = 1;
}

static const struct i2c_device_id max77775_i2c_id[] = {
	{ MFD_DEV_NAME, TYPE_MAX77775 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max77775_i2c_id);

#if defined(CONFIG_OF)
static const struct of_device_id max77775_i2c_dt_ids[] = {
	{ .compatible = "maxim,max77775" },
	{ },
};
MODULE_DEVICE_TABLE(of, max77775_i2c_dt_ids);
#endif /* CONFIG_OF */

#if defined(CONFIG_PM)
static int max77775_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77775_dev *max77775 = i2c_get_clientdata(i2c);

	md75_info_usb("%s:%s\n", MFD_DEV_NAME, __func__);

	synchronize_irq(max77775->irq);

	max77775->suspended = true;

	return 0;
}

static int max77775_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77775_dev *max77775 = i2c_get_clientdata(i2c);

	md75_info_usb("%s:%s\n", MFD_DEV_NAME, __func__);

	max77775->suspended = false;
	wake_up_interruptible(&max77775->suspend_wait);

	return 0;
}
#else
#define max77775_suspend	NULL
#define max77775_resume		NULL
#endif /* CONFIG_PM */

const struct dev_pm_ops max77775_pm = {
	.suspend = max77775_suspend,
	.resume = max77775_resume,
};

static struct i2c_driver max77775_i2c_driver = {
	.driver		= {
		.name	= MFD_DEV_NAME,
		.owner	= THIS_MODULE,
#if defined(CONFIG_PM)
		.pm	= &max77775_pm,
#endif /* CONFIG_PM */
#if defined(CONFIG_OF)
		.of_match_table	= max77775_i2c_dt_ids,
#endif /* CONFIG_OF */
	},
	.probe		= max77775_i2c_probe,
	.remove		= max77775_i2c_remove,
	.shutdown 	= max77775_i2c_shutdown,
	.id_table	= max77775_i2c_id,
};

static int __init max77775_i2c_init(void)
{
	md75_info_usb("%s:%s\n", MFD_DEV_NAME, __func__);
	schedule_delayed_work(&firmware_load_work, MD75_FIRMWARE_TIMEOUT_SEC * HZ);
	return i2c_add_driver(&max77775_i2c_driver);
}
/* init early so consumer devices can complete system boot */
subsys_initcall(max77775_i2c_init);

static void __exit max77775_i2c_exit(void)
{
	cancel_delayed_work_sync(&firmware_load_work);
	i2c_del_driver(&max77775_i2c_driver);
}
module_exit(max77775_i2c_exit);

MODULE_AUTHOR("Samsung USB Team");
MODULE_DESCRIPTION("Max77775 MFD driver");
MODULE_LICENSE("GPL");
