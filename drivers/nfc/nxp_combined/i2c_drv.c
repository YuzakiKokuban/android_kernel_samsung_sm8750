/******************************************************************************
 * Copyright (C) 2015, The Linux Foundation. All rights reserved.
 * Copyright (C) 2013-2022 NXP
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ******************************************************************************/
/*
 * Copyright (C) 2010 Trusted Logic S.A.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif
#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
#include <linux/platform_device.h>
#endif

#include "common_ese.h"
#include "p73.h"
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include "common.h"

static int nfc_reboot_cb(struct notifier_block *nb,
				unsigned long action, void *data);
#endif

#ifdef FEATURE_CORE_RESET_NTF_CHECK
struct nfc_core_reset_type {
	char name[16];
	int data_len;
	u8 err_type;
};

/* if Power Reset comes after NFC on, it's normal operation */
struct nfc_core_reset_type core_reset[] = {
	{"Unrecover",	0xA, 0x00},
	{"Power Reset",	0x9, 0x01}, /* at new version */
	{"Power Reset",	0xA, 0x01}, /* at old version */
	{"Assert",	0x6, 0xA0},
	{"Clock lost",	0x6, 0xA4}
};

static bool is_core_reset;

bool nfc_check_core_reset_type(u8 *data, int len)
{
	int arr_size = ARRAY_SIZE(core_reset);
	int i;
	bool found = false;

	for (i = 0; i < arr_size; i++) {
		if (core_reset[i].data_len == len &&
				core_reset[i].err_type == data[0]) {
			found = true;
			break;
		}
	}

	if (found) {
		char info[64] = {0x0, };
		int j, idx = 0;

		idx = snprintf(info, 64, "6000%02X", len);
		for (j = 0; j < len; j++)
			idx += snprintf(info + idx, 64 - idx, "%02X", data[j]);
		NFC_LOG_INFO("CORE_RESET: %s (%s)\n", core_reset[i].name, info);
		nfc_print_status();
	}

	return found;
}

void nfc_check_is_core_reset_ntf(u8 *data, int len)
{
	if (is_core_reset) {
		/* check payload */
		nfc_check_core_reset_type(data, len);
		is_core_reset = false;
	} else if (len == 3) { /* check header */
		/* check if it's core reset ntf */
		if (data[0] == 0x60 && data[1] == 0x00) {
			switch (data[2]) {
			case 0x06:
			case 0x0A:
				is_core_reset = true;
				break;
			default:
				is_core_reset = false;
			}
		} else {
			is_core_reset = false;
		}
	}
}
#endif

/**
 * i2c_disable_irq()
 *
 * Check if interrupt is disabled or not
 * and disable interrupt
 *
 * Return: int
 */
int i2c_disable_irq(struct nfc_dev *dev)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->i2c_dev.irq_enabled_lock, flags);
	if (dev->i2c_dev.irq_enabled) {
		disable_irq_nosync(dev->i2c_dev.client->irq);
		dev->i2c_dev.irq_enabled = false;
	}
	spin_unlock_irqrestore(&dev->i2c_dev.irq_enabled_lock, flags);

	return 0;
}

/**
 * i2c_enable_irq()
 *
 * Check if interrupt is enabled or not
 * and enable interrupt
 *
 * Return: int
 */
int i2c_enable_irq(struct nfc_dev *dev)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->i2c_dev.irq_enabled_lock, flags);
	if (!dev->i2c_dev.irq_enabled) {
		dev->i2c_dev.irq_enabled = true;
		enable_irq(dev->i2c_dev.client->irq);
	}
	spin_unlock_irqrestore(&dev->i2c_dev.irq_enabled_lock, flags);

	return 0;
}

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
void i2c_disable_clk_irq(struct nfc_dev *dev)
{
	struct platform_configs *nfc_configs = &dev->configs;
	struct platform_gpio *nfc_gpio = &dev->configs.gpio;

	if (nfc_configs->clk_req_wake || nfc_configs->clk_req_all_trigger) {
		if (nfc_gpio->clk_req_irq_enabled) {
			disable_irq_nosync(nfc_gpio->clk_req_irq);
			nfc_gpio->clk_req_irq_enabled = false;
		}
		dev->clk_req_wakelock = false;
	}
}

void i2c_enable_clk_irq(struct nfc_dev *dev)
{
	struct platform_configs *nfc_configs = &dev->configs;
	struct platform_gpio *nfc_gpio = &dev->configs.gpio;

#if IS_ENABLED(CONFIG_SEC_NFC_EINT_EXYNOS)
	if (nfc_configs->clk_req_eint_mode)
		return;
#endif

	if (nfc_configs->clk_req_wake || nfc_configs->clk_req_all_trigger) {
		if (!nfc_gpio->clk_req_irq_enabled) {
			enable_irq(nfc_gpio->clk_req_irq);
			nfc_gpio->clk_req_irq_enabled = true;
		}
	}
}
#endif

static irqreturn_t i2c_irq_handler(int irq, void *dev_id)
{
	struct nfc_dev *nfc_dev = dev_id;

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	wake_lock_timeout(&nfc_dev->nfc_wake_lock, 2*HZ);
#else
	struct i2c_dev *i2c_dev = &nfc_dev->i2c_dev;

	if (device_may_wakeup(&i2c_dev->client->dev))
		pm_wakeup_event(&i2c_dev->client->dev, WAKEUP_SRC_TIMEOUT);
#endif

	i2c_disable_irq(nfc_dev);
	wake_up(&nfc_dev->read_wq);

	NFC_LOG_REC("irq\n");

	return IRQ_HANDLED;
}

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
static irqreturn_t nfc_clk_req_irq_handler(int irq, void *dev_id)
{
	struct nfc_dev *nfc_dev = dev_id;
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;
	struct platform_configs *nfc_configs = &nfc_dev->configs;
	static bool nfc_clk_status;

#if IS_ENABLED(CONFIG_SEC_NFC_EINT_EXYNOS)
	if (nfc_configs->clk_req_eint_mode)
		return IRQ_HANDLED;
#endif

	if (nfc_configs->change_clkreq_for_acpm)
		return IRQ_HANDLED;

	if (nfc_configs->clk_req_all_trigger) {
		if (gpio_get_value(nfc_gpio->clk_req)) {
			NFC_LOG_REC("clk_req 1\n");
			if (!wake_lock_active(&nfc_dev->nfc_clk_wake_lock))
				wake_lock(&nfc_dev->nfc_clk_wake_lock);

			if (nfc_configs->nfc_clock && !nfc_clk_status) {
				if (clk_prepare_enable(nfc_configs->nfc_clock)) {
					NFC_LOG_REC("clock enable failed\n");
					return IRQ_HANDLED;
				}
				nfc_clk_status = true;
			}
		} else {
			NFC_LOG_REC("clk_req 0\n");
			if (wake_lock_active(&nfc_dev->nfc_clk_wake_lock))
				wake_unlock(&nfc_dev->nfc_clk_wake_lock);

			if (nfc_configs->nfc_clock && nfc_clk_status) {
				clk_disable_unprepare(nfc_configs->nfc_clock);
				nfc_clk_status = false;
			}
		}
	} else {
		NFC_LOG_REC("clk_req w:%d\n", nfc_dev->clk_req_wakelock);
		if (nfc_dev->clk_req_wakelock) {
			nfc_dev->clk_req_wakelock = false;
			wake_lock_timeout(&nfc_dev->nfc_clk_wake_lock, 2*HZ);
		}
	}
	return IRQ_HANDLED;
}

#define PRINT_NFC_BUF 0
#if PRINT_NFC_BUF
static void print_hex_buf(const char *tag, const char *data, int len)
{
	pr_info("%s len: %d\n", tag, len);
	print_hex_dump(KERN_DEBUG, tag, DUMP_PREFIX_OFFSET, 16, 8,
		data, len, false);
}
#else
static void print_hex_buf(const char *tag, const char *data, int len)
{
	do {} while (0);
}
#endif/*PRINT_NFC_BUF*/

static void secnfc_check_screen_on_rsp(struct nfc_dev *nfc_dev,
		const char *buf, size_t count)
{
	if (nfc_dev->screen_on_cmd && nfc_dev->screen_cfg) {
		nfc_dev->screen_on_cmd = false;
		nfc_dev->screen_cfg = false;
		NFC_LOG_INFO("scrn_on\n");
	}
}

static void secnfc_check_screen_off_rsp(struct nfc_dev *nfc_dev,
		const char *buf, size_t count)
{
	if (!nfc_dev->screen_off_cmd || !nfc_dev->screen_cfg) {
		nfc_dev->screen_off_rsp_count = 0;
		return;
	}

	if (!nfc_dev->screen_off_rsp_count && count == 3/*header*/) {
		nfc_dev->screen_off_rsp_count++;
		return;
	}

	if (nfc_dev->screen_off_rsp_count == 1/*payload*/) {
		if (!buf[0]) {//00 or 0000
			if (wake_lock_active(&nfc_dev->nfc_wake_lock))
				wake_unlock(&nfc_dev->nfc_wake_lock);
			NFC_LOG_INFO("scrn_off\n");

			nfc_dev->screen_cfg = false;
			nfc_dev->screen_off_cmd = false;
		}
		nfc_dev->screen_off_rsp_count = 0;
		return;
	}

	nfc_dev->screen_off_rsp_count = 0;
}
#endif

int i2c_read(struct nfc_dev *nfc_dev, char *buf, size_t count, int timeout)
{
	int ret;
	struct i2c_dev *i2c_dev = &nfc_dev->i2c_dev;
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;

	NFC_LOG_REC("rd: %zu\n", count);

	if (timeout > NCI_CMD_RSP_TIMEOUT_MS)
		timeout = NCI_CMD_RSP_TIMEOUT_MS;

	if (count > MAX_NCI_BUFFER_SIZE)
		count = MAX_NCI_BUFFER_SIZE;

	if (!gpio_get_value(nfc_gpio->irq)) {
		while (1) {
			ret = 0;
			i2c_enable_irq(nfc_dev);
			if (!gpio_get_value(nfc_gpio->irq)) {
				if (timeout) {
					ret = wait_event_interruptible_timeout(
						nfc_dev->read_wq,
						!i2c_dev->irq_enabled,
						msecs_to_jiffies(timeout));
					if (ret <= 0) {
						NFC_LOG_ERR("%s: timeout error\n",
							__func__);
						goto err;
					}
				} else {
					ret = wait_event_interruptible(
						nfc_dev->read_wq,
						!i2c_dev->irq_enabled);
					if (ret) {
						NFC_LOG_ERR("%s: err wakeup of wq\n",
							__func__);
						goto err;
					}
				}
			}
			i2c_disable_irq(nfc_dev);

			if (gpio_get_value(nfc_gpio->irq))
				break;
			if (!gpio_get_value(nfc_gpio->ven)) {
				NFC_LOG_ERR("%s: releasing read\n", __func__);
				ret = -EIO;
				goto err;
			}
			/*
			 * NFC service wanted to close the driver so,
			 * release the calling reader thread asap.
			 *
			 * This can happen in case of nfc node close call from
			 * eSE HAL in that case the NFC HAL reader thread
			 * will again call read system call
			 */
			if (nfc_dev->release_read) {
				NFC_LOG_REC("%s: releasing read\n", __func__);
				return 0;
			}
			NFC_LOG_ERR("%s: spurious interrupt detected\n", __func__);
		}
	}

	memset(buf, 0x00, count);
	/* Read data */
	ret = i2c_master_recv(nfc_dev->i2c_dev.client, buf, count);
	if (ret <= 0) {
		NFC_LOG_ERR("%s: returned %d\n", __func__, ret);
		goto err;
	}
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	print_hex_buf("nfc_rd: ", buf, count);
	secnfc_check_screen_on_rsp(nfc_dev, buf, ret);
	secnfc_check_screen_off_rsp(nfc_dev, buf, ret);
#ifdef FEATURE_CORE_RESET_NTF_CHECK
	nfc_check_is_core_reset_ntf(buf, ret);
#endif
#endif
	/* check if it's response of cold reset command
	 * NFC HAL process shouldn't receive this data as
	 * command was sent by driver
	 */
	if (nfc_dev->cold_reset.rsp_pending) {
		if (IS_PROP_CMD_RSP(buf)) {
			/* Read data */
			ret = i2c_master_recv(nfc_dev->i2c_dev.client,
					&buf[NCI_PAYLOAD_IDX],
					buf[NCI_PAYLOAD_LEN_IDX]);
			if (ret <= 0) {
				NFC_LOG_ERR("%s: error reading cold rst/prot rsp\n",
					__func__);
				goto err;
			}
			wakeup_on_prop_rsp(nfc_dev, buf);
			/*
			 * NFC process doesn't know about cold reset command
			 * being sent as it was initiated by eSE process
			 * we shouldn't return any data to NFC process
			 */
			return 0;
		}
	}
err:
	return ret;
}

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
#define SCREEN_ONOFF_CMD_SZ	4
#define SCREEN_SET_CFG_SZ	7

static bool secnfc_check_screen_on_cmd(struct nfc_dev *nfc_dev,
		const char *buf, size_t count)
{
	if (count != SCREEN_ONOFF_CMD_SZ)
		return false;

	if (buf[0] == 0x20 && buf[1] == 0x09 && buf[2] == 0x01 && buf[3] == 0x2)
		return true;

	return false;
}

static bool secnfc_check_screen_off_cmd(struct nfc_dev *nfc_dev,
		const char *buf, size_t count)
{
	if (count != SCREEN_ONOFF_CMD_SZ)
		return false;

	if (buf[0] == 0x20 && buf[1] == 0x09 && buf[2] == 0x01 &&
			(buf[3] == 0x1 || buf[3] == 0x3))
		return true;

	return false;
}

static bool secnfc_check_screen_cfg(struct nfc_dev *nfc_dev,
		const char *buf, size_t count)
{
	if (count != SCREEN_SET_CFG_SZ)
		return false;

	if (buf[0] == 0x20 && buf[1] == 0x02 && buf[2] == 0x04 &&
			buf[3] == 0x01 && buf[4] == 0x02 &&
			buf[5] == 0x01 && buf[6] == 0x00)
		return true;

	return false;
}

static void secnfc_configure_clk_irq_based_on_screen_onoff(struct nfc_dev *nfc_dev, bool is_screen_on)
{
	struct platform_configs *nfc_configs = &nfc_dev->configs;

	if (!nfc_configs->disable_clk_irq_during_wakeup)
		return;

	if (is_screen_on)
		i2c_disable_clk_irq(nfc_dev);
	else
		i2c_enable_clk_irq(nfc_dev);
}

static void secnfc_check_screen_onoff(struct nfc_dev *nfc_dev,
		const char *buf, size_t count)
{
	if (secnfc_check_screen_cfg(nfc_dev, buf, count)) {
		nfc_dev->screen_cfg = true;
	} else if (secnfc_check_screen_on_cmd(nfc_dev, buf, count)) {
		nfc_dev->screen_on_cmd = true;
		secnfc_configure_clk_irq_based_on_screen_onoff(nfc_dev, true);
	} else if (secnfc_check_screen_off_cmd(nfc_dev, buf, count)) {
		nfc_dev->screen_off_cmd = true;
		secnfc_configure_clk_irq_based_on_screen_onoff(nfc_dev, false);
	} else {
		nfc_dev->screen_on_cmd = false;
		nfc_dev->screen_off_cmd = false;
		nfc_dev->screen_cfg = false;
	}
}
#endif

int i2c_write(struct nfc_dev *nfc_dev, const char *buf, size_t count,
		int max_retry_cnt)
{
	int ret = -EINVAL;
	int retry_cnt;
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;

	if (count > MAX_DL_BUFFER_SIZE)
		count = MAX_DL_BUFFER_SIZE;

	NFC_LOG_REC("wr: %zu\n", count);
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	print_hex_buf("nfc_wr: ", buf, count);
	secnfc_check_screen_onoff(nfc_dev, buf, count);
#endif
	/*
	 * Wait for any pending read for max 15ms before write
	 * This is to avoid any packet corruption during read, when
	 * the host cmds resets NFCC during any parallel read operation
	 */
	for (retry_cnt = 1; retry_cnt <= MAX_WRITE_IRQ_COUNT; retry_cnt++) {
		if (gpio_get_value(nfc_gpio->irq)) {
			NFC_LOG_ERR("%s: irq high during write, wait\n", __func__);
			usleep_range(WRITE_RETRY_WAIT_TIME_US,
				     WRITE_RETRY_WAIT_TIME_US + 100);
		} else {
			break;
		}
		if (retry_cnt == MAX_WRITE_IRQ_COUNT &&
			gpio_get_value(nfc_gpio->irq)) {
			NFC_LOG_ERR("%s: allow after maximum wait\n", __func__);
		}
	}

	for (retry_cnt = 1; retry_cnt <= max_retry_cnt; retry_cnt++) {
		ret = i2c_master_send(nfc_dev->i2c_dev.client, buf, count);
		if (ret <= 0) {
			NFC_LOG_ERR("%s: write failed ret(%d), maybe in standby\n",
				__func__, ret);
			usleep_range(WRITE_RETRY_WAIT_TIME_US,
				WRITE_RETRY_WAIT_TIME_US + 100);
		} else if (ret != count) {
			NFC_LOG_ERR("%s: failed to write %d\n", __func__, ret);
			ret = -EIO;
		} else if (ret == count)
			break;
	}
	return ret;
}

ssize_t nfc_i2c_dev_read(struct file *filp, char __user *buf, size_t count,
			loff_t *offset)
{
	int ret;
	struct nfc_dev *nfc_dev = (struct nfc_dev *)filp->private_data;

	if (!nfc_dev) {
		NFC_LOG_ERR("%s: device doesn't exist anymore\n", __func__);
		return -ENODEV;
	}
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	if (count > MAX_NCI_BUFFER_SIZE) {
		//NFC_LOG_ERR("%s: too big count %u\n", __func__, count);
		return -EINVAL;
	}
#endif
	mutex_lock(&nfc_dev->read_mutex);
	if (filp->f_flags & O_NONBLOCK) {
		ret = i2c_master_recv(nfc_dev->i2c_dev.client, nfc_dev->read_kbuf, count);
		NFC_LOG_ERR("%s: NONBLOCK read ret = %d\n", __func__, ret);
	} else {
		ret = i2c_read(nfc_dev, nfc_dev->read_kbuf, count, 0);
	}
	if (ret > 0) {
		if (copy_to_user(buf, nfc_dev->read_kbuf, ret)) {
			NFC_LOG_ERR("%s: failed to copy to user space\n", __func__);
			ret = -EFAULT;
		}
	}
	mutex_unlock(&nfc_dev->read_mutex);
	return ret;
}

ssize_t nfc_i2c_dev_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *offset)
{
	int ret;
	struct nfc_dev *nfc_dev = (struct nfc_dev *)filp->private_data;

	if (count > MAX_DL_BUFFER_SIZE)
		count = MAX_DL_BUFFER_SIZE;

	if (!nfc_dev) {
		NFC_LOG_ERR("%s: device doesn't exist anymore\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&nfc_dev->write_mutex);
	if (copy_from_user(nfc_dev->write_kbuf, buf, count)) {
		NFC_LOG_ERR("%s: failed to copy from user space\n", __func__);
		mutex_unlock(&nfc_dev->write_mutex);
		return -EFAULT;
	}
	ret = i2c_write(nfc_dev, nfc_dev->write_kbuf, count, NO_RETRY);
	mutex_unlock(&nfc_dev->write_mutex);
	return ret;
}

static const struct file_operations nfc_i2c_dev_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = nfc_i2c_dev_read,
	.write = nfc_i2c_dev_write,
	.open = nfc_dev_open,
	.flush = nfc_dev_flush,
	.release = nfc_dev_close,
	.unlocked_ioctl = nfc_dev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = nfc_dev_compat_ioctl,
#endif
};

#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
int nfc_i2c_dev_probe(struct i2c_client *client)
#else
int nfc_i2c_dev_probe(struct i2c_client *client,const struct i2c_device_id *id)
#endif
{
	int ret = 0;
	struct nfc_dev *nfc_dev = NULL;
	struct i2c_dev *i2c_dev = NULL;
	struct platform_configs *nfc_configs = NULL;
	struct platform_gpio *nfc_gpio = NULL;

#ifdef CONFIG_SEC_NFC_LOGGER
#ifdef CONFIG_SEC_NFC_LOGGER_ADD_ACPM_LOG
	nfc_logger_acpm_log_init(0x0);
#endif
	nfc_logger_register_nfc_stauts_func(nfc_print_status);
#endif

	NFC_LOG_INFO("%s: enter\n", __func__);
	nfc_dev = kzalloc(sizeof(struct nfc_dev), GFP_KERNEL);
	if (nfc_dev == NULL) {
		ret = -ENOMEM;
		goto err;
	}
	nfc_configs = &nfc_dev->configs;
	nfc_gpio = &nfc_configs->gpio;
	/* retrieve details of gpios from dt */
	ret = nfc_parse_dt(&client->dev, nfc_configs, PLATFORM_IF_I2C);
	if (ret) {
		NFC_LOG_ERR("%s: failed to parse dt\n", __func__);
		goto err_free_nfc_dev;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		NFC_LOG_ERR("%s: need I2C_FUNC_I2C\n", __func__);
		ret = -ENODEV;
		goto err_free_nfc_dev;
	}
	nfc_dev->read_kbuf = kzalloc(MAX_NCI_BUFFER_SIZE, GFP_DMA | GFP_KERNEL);
	if (!nfc_dev->read_kbuf) {
		ret = -ENOMEM;
		goto err_free_nfc_dev;
	}
	nfc_dev->write_kbuf = kzalloc(MAX_DL_BUFFER_SIZE, GFP_DMA | GFP_KERNEL);
	if (!nfc_dev->write_kbuf) {
		ret = -ENOMEM;
		goto err_free_read_kbuf;
	}
	nfc_dev->interface = PLATFORM_IF_I2C;
	nfc_dev->nfc_state = NFC_STATE_NCI;
	nfc_dev->i2c_dev.client = client;
	i2c_dev = &nfc_dev->i2c_dev;
	nfc_dev->nfc_read = i2c_read;
	nfc_dev->nfc_write = i2c_write;
	nfc_dev->nfc_enable_intr = i2c_enable_irq;
	nfc_dev->nfc_disable_intr = i2c_disable_irq;
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	nfc_dev->nfc_enable_clk_intr = i2c_enable_clk_irq;
	nfc_dev->nfc_disable_clk_intr = i2c_disable_clk_irq;

	/* reboot notifier callback */
	nfc_dev->reboot_nb.notifier_call = nfc_reboot_cb;
	register_reboot_notifier(&nfc_dev->reboot_nb);
#endif
#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	if (!nfc_configs->late_pvdd_en) {
		ret = configure_gpio(nfc_gpio->ven, GPIO_OUTPUT);
		if (ret) {
			NFC_LOG_ERR("%s: unable to request nfc reset gpio [%d]\n", __func__,
					nfc_gpio->ven);
			goto err_free_write_kbuf;
		}
	}
#else
	ret = configure_gpio(nfc_gpio->ven, GPIO_OUTPUT);
	if (ret) {
		NFC_LOG_ERR("%s: unable to request nfc reset gpio [%d]\n", __func__,
			nfc_gpio->ven);
		goto err_free_write_kbuf;
	}
#endif
	ret = configure_gpio(nfc_gpio->irq, GPIO_IRQ);
	if (ret <= 0) {
		NFC_LOG_ERR("%s: unable to request nfc irq gpio [%d]\n", __func__,
			nfc_gpio->irq);
		goto err_free_gpio;
	}
	client->irq = ret;
	if (gpio_is_valid(nfc_gpio->dwl_req)) {
		ret = configure_gpio(nfc_gpio->dwl_req, GPIO_OUTPUT);
		if (ret) {
			NFC_LOG_ERR("%s: unable to request nfc firm downl gpio [%d]\n",
				__func__, nfc_gpio->dwl_req);
		}
	}
	/* init mutex and queues */
	init_waitqueue_head(&nfc_dev->read_wq);
	mutex_init(&nfc_dev->read_mutex);
	mutex_init(&nfc_dev->write_mutex);
	mutex_init(&nfc_dev->dev_ref_mutex);
	spin_lock_init(&i2c_dev->irq_enabled_lock);
	common_ese_init(nfc_dev);

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	wake_lock_init(&nfc_dev->nfc_wake_lock, WAKE_LOCK_SUSPEND, "nfc_wake_lock");
#endif

#ifndef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	ret = nfc_misc_register(nfc_dev, &nfc_i2c_dev_fops, DEV_COUNT,
				NFC_CHAR_DEV_NAME, CLASS_NAME);
	if (ret) {
		NFC_LOG_ERR("%s: nfc_misc_register failed\n", __func__);
		goto err_mutex_destroy;
	}
#endif
	/* interrupt initializations */
	NFC_LOG_INFO("%s: requesting IRQ %d\n", __func__, client->irq);
	i2c_dev->irq_enabled = true;
	ret = request_irq(client->irq, i2c_irq_handler, IRQF_TRIGGER_HIGH,
				client->name, nfc_dev);
	if (ret) {
		NFC_LOG_ERR("%s: request_irq failed\n", __func__);
		goto err_nfc_misc_unregister;
	}
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	if (nfc_configs->clk_req_wake || nfc_configs->clk_req_all_trigger) {
		unsigned long irq_flag = IRQF_TRIGGER_RISING | IRQF_ONESHOT;

		wake_lock_init(&nfc_dev->nfc_clk_wake_lock, WAKE_LOCK_SUSPEND, "nfc_clk_wake_lock");

		if (nfc_configs->clk_req_all_trigger)
			irq_flag |= IRQF_TRIGGER_FALLING;

		ret = configure_gpio(nfc_gpio->clk_req, GPIO_IRQ);
		if (ret <= 0) {
			NFC_LOG_ERR("%s: unable to request nfc clk req irq gpio [%d]\n", __func__, nfc_gpio->irq);
		} else {
			nfc_gpio->clk_req_irq = ret;

			NFC_LOG_INFO("clk_req IRQ num %d\n", nfc_gpio->clk_req_irq);

			ret = request_threaded_irq(nfc_gpio->clk_req_irq, NULL, nfc_clk_req_irq_handler,
					irq_flag, "pn547_clk_req", nfc_dev);
			if (ret)
				NFC_LOG_ERR("clk_req_irq failed\n");
			else {
				nfc_gpio->clk_req_irq_enabled = true;
				i2c_disable_clk_irq(nfc_dev);
			}
		}
	}

	if (!nfc_configs->late_pvdd_en)
		nfc_power_control(nfc_dev);
#else
	gpio_set_ven(nfc_dev, 1);
	gpio_set_ven(nfc_dev, 0);
	gpio_set_ven(nfc_dev, 1);
#endif
	i2c_disable_irq(nfc_dev);

	i2c_set_clientdata(client, nfc_dev);
	i2c_dev->irq_wake_up = false;

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
#ifdef CONFIG_CLK_ACPM_INIT
	if (nfc_configs->change_clkreq_for_acpm) {
		NFC_LOG_INFO("acpm_init_nfc_clk_req clk_req : %d\n", nfc_gpio->clk_req);
		acpm_init_eint_nfc_clk_req(nfc_gpio->clk_req);
	}
#endif
#if IS_ENABLED(CONFIG_NFC_SN2XX_ESE_SUPPORT)
	store_nfc_i2c_device(&client->dev);
#endif
	nfc_probe_done(nfc_dev);
#endif

	NFC_LOG_INFO("%s: probing nfc i2c successfully\n", __func__);
	return 0;
err_nfc_misc_unregister:
#ifndef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	nfc_misc_unregister(nfc_dev, DEV_COUNT);
err_mutex_destroy:
#endif
	mutex_destroy(&nfc_dev->dev_ref_mutex);
	mutex_destroy(&nfc_dev->read_mutex);
	mutex_destroy(&nfc_dev->write_mutex);
err_free_gpio:
	gpio_free_all(nfc_dev);
err_free_write_kbuf:
	kfree(nfc_dev->write_kbuf);
err_free_read_kbuf:
	kfree(nfc_dev->read_kbuf);
err_free_nfc_dev:
	kfree(nfc_dev);
err:
	NFC_LOG_ERR("%s: probing not successful, check hardware\n", __func__);
	return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
int nfc_i2c_dev_remove(struct i2c_client *client)
#else
void nfc_i2c_dev_remove(struct i2c_client *client)
#endif
{
	int ret = 0;
	struct nfc_dev *nfc_dev = NULL;

	NFC_LOG_INFO("%s: remove device\n", __func__);
	nfc_dev = i2c_get_clientdata(client);
	if (!nfc_dev) {
		NFC_LOG_ERR("%s: device doesn't exist anymore\n", __func__);
		ret = -ENODEV;
		goto end;
	}
	if (nfc_dev->dev_ref_count > 0) {
		NFC_LOG_ERR("%s: device already in use\n", __func__);
		ret = -EBUSY;
		goto end;
	}

	free_irq(client->irq, nfc_dev);
#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	nfc_misc_unregister(NULL, DEV_COUNT);
#else
	nfc_misc_unregister(nfc_dev, DEV_COUNT);
#endif
#ifdef CONFIG_SEC_NFC_LOGGER
	nfc_logger_deinit();
#endif
	wake_lock_destroy(&nfc_dev->nfc_wake_lock);

	mutex_destroy(&nfc_dev->read_mutex);
	mutex_destroy(&nfc_dev->write_mutex);
	gpio_free_all(nfc_dev);
	kfree(nfc_dev->read_kbuf);
	kfree(nfc_dev->write_kbuf);
	kfree(nfc_dev);
end:
	NFC_LOG_INFO("%s: ret :%d\n", __func__, ret);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	return ret;
#endif
}

int nfc_i2c_dev_suspend(struct device *device)
{
	struct i2c_client *client = to_i2c_client(device);
	struct nfc_dev *nfc_dev = i2c_get_clientdata(client);
	struct i2c_dev *i2c_dev = NULL;
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	struct platform_configs *nfc_configs = &nfc_dev->configs;
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;
#endif
	NFC_LOG_INFO_WITH_DATE("suspend\n");
	if (!nfc_dev) {
		NFC_LOG_ERR("%s: device doesn't exist anymore\n", __func__);
		return -ENODEV;
	}
	i2c_dev = &nfc_dev->i2c_dev;


#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	if (nfc_dev->dev_ref_count == 0)
		return 0;

	if (i2c_dev->irq_enabled && !i2c_dev->irq_wake_up) {
		if (!enable_irq_wake(client->irq))
			i2c_dev->irq_wake_up = true;
	}
	if (nfc_configs->clk_req_wake || nfc_configs->clk_req_all_trigger) {
		enable_irq_wake(nfc_gpio->clk_req_irq);
		nfc_dev->clk_req_wakelock = true;
	}
#else
	if (device_may_wakeup(&client->dev) && i2c_dev->irq_enabled) {
		if (!enable_irq_wake(client->irq))
			i2c_dev->irq_wake_up = true;
	}
#endif
	NFC_LOG_DBG("%s: irq_wake_up = %d\n", __func__, i2c_dev->irq_wake_up);
	return 0;
}

int nfc_i2c_dev_resume(struct device *device)
{
	struct i2c_client *client = to_i2c_client(device);
	struct nfc_dev *nfc_dev = i2c_get_clientdata(client);
	struct i2c_dev *i2c_dev = NULL;
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	struct platform_configs *nfc_configs = &nfc_dev->configs;
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;
#endif
	NFC_LOG_INFO_WITH_DATE("resume\n");
	if (!nfc_dev) {
		NFC_LOG_ERR("%s: device doesn't exist anymore\n", __func__);
		return -ENODEV;
	}
	i2c_dev = &nfc_dev->i2c_dev;

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	if (nfc_dev->dev_ref_count == 0)
		return 0;

	if (i2c_dev->irq_wake_up) {
		if (!disable_irq_wake(client->irq))
			i2c_dev->irq_wake_up = false;
	}
	if (nfc_configs->clk_req_wake || nfc_configs->clk_req_all_trigger)
		disable_irq_wake(nfc_gpio->clk_req_irq);
#else
	if (device_may_wakeup(&client->dev) && i2c_dev->irq_wake_up) {
		if (!disable_irq_wake(client->irq))
			i2c_dev->irq_wake_up = false;
	}
#endif
	NFC_LOG_DBG("%s: irq_wake_up = %d\n", __func__, i2c_dev->irq_wake_up);
	return 0;
}

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
#define SN300_PVDD_MIN	1100000
#define SN300_PVDD_MAX	1300000

static int nfc_pvdd_off(struct nfc_dev *nfc_dev)
{
	struct platform_configs *nfc_configs;
	int ret = 0, pvdd_val;

//	NFC_LOG_INFO("nfc_pvdd_off ++\n");

	nfc_configs = &nfc_dev->configs;

	pvdd_val = regulator_get_voltage(nfc_configs->nfc_pvdd);
	NFC_LOG_INFO("pvdd_val %d\n", pvdd_val);
	
	if (SN300_PVDD_MIN < pvdd_val && pvdd_val < SN300_PVDD_MAX) {
//		NFC_LOG_INFO("nfc IO force off\n");
		ret = nfc_regulator_onoff(nfc_dev, 0);
		if (ret < 0)
			NFC_LOG_ERR("%s pn547 regulator_on FAIL %d\n", __func__, ret);
	}

//	NFC_LOG_INFO("nfc_pvdd_off --\n");
//exit:
	return ret;
}

static int nfc_reboot_cb(struct notifier_block *nb,
				unsigned long action, void *data)
{
	struct nfc_dev *nfc_dev = container_of(nb, struct nfc_dev, reboot_nb);
	struct platform_gpio *nfc_gpio;
	struct platform_configs *nfc_configs;
	int ret;

	if (!nfc_dev) {
		NFC_LOG_INFO("no nfc dev\n");
		goto exit;
	}

	NFC_LOG_INFO("nfc_reboot_cb ++\n");

	nfc_gpio = &nfc_dev->configs.gpio;
	nfc_configs = &nfc_dev->configs;

	if (nfc_configs->ap_vendor == AP_VENDOR_QCT) {
		ret = gpio_direction_output(nfc_gpio->ven, 0);
		if (ret)
			NFC_LOG_ERR("VEN control fail %d\n", ret);

		usleep_range(300, 301);
		nfc_pvdd_off(nfc_dev);
	}

	NFC_LOG_INFO("nfc_reboot_cb --\n");
exit:
	return NOTIFY_OK;
}

static void nfc_shutdown(struct i2c_client *client)
{
	struct nfc_dev *nfc_dev = i2c_get_clientdata(client);
	int ret;

	NFC_LOG_INFO("nfc_shutdown ++\n");

	if (nfc_dev) {
		struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;
		struct platform_configs *nfc_configs = &nfc_dev->configs;

		/*
		 * In case of S.LSI, NFC card mode doesn't work in LPM.
		 * so, it's skipped on S.LSI models using sn110. it need to be checked
		 */
		if (nfc_configs->ap_vendor != AP_VENDOR_SLSI) {
			if (nfc_configs->ap_vendor == AP_VENDOR_MTK) {
				/*use internal pull-up case*/
				struct pinctrl *pinctrl = NULL;

				pinctrl = devm_pinctrl_get_select(&client->dev, "i2c_off");
				if (IS_ERR_OR_NULL(pinctrl))
					NFC_LOG_ERR("Failed to pinctrl i2c_off\n");
				else
					devm_pinctrl_put(pinctrl);
				NFC_LOG_ERR("i2c off pinctrl called\n");
			};

			ret = gpio_direction_output(nfc_gpio->ven, 0);
			if (ret)
				NFC_LOG_ERR("VEN control fail %d\n", ret);

			if (nfc_configs->ap_vendor == AP_VENDOR_QCT) {
				usleep_range(300, 301);
				nfc_pvdd_off(nfc_dev);
			}
		}
	}

	NFC_LOG_INFO("nfc_shutdown --\n");
}
#endif

#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
static int nfc_platform_probe(struct platform_device *pdev)
{
	int ret = -1;

	nfc_parse_dt_for_platform_device(&pdev->dev);

	ret = nfc_misc_register(NULL, &nfc_i2c_dev_fops, DEV_COUNT,
				NFC_CHAR_DEV_NAME, CLASS_NAME);
	if (ret)
		NFC_LOG_ERR("%s: nfc_misc_register failed\n", __func__);

	NFC_LOG_INFO("%s: finished...\n", __func__);
	return 0;
}

static int nfc_platform_remove(struct platform_device *pdev)
{
	NFC_LOG_INFO("Entry : %s\n", __func__);
	return 0;
}

static const struct of_device_id nfc_platform_match_table[] = {
	{ .compatible = "nfc_platform",},
	{},
};

static struct platform_driver nfc_platform_driver = {
	.driver = {
		.name = "nfc_platform",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = nfc_platform_match_table,
#endif
	},
	.probe =  nfc_platform_probe,
	.remove = nfc_platform_remove,
};
#endif /* CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE */

static const struct i2c_device_id nfc_i2c_dev_id[] = {{NFC_I2C_DEV_ID, 0},
							{}};

static const struct of_device_id nfc_i2c_dev_match_table[] = {
	{
		.compatible = NFC_I2C_DRV_STR,
	},
	{}
};

static const struct dev_pm_ops nfc_i2c_dev_pm_ops = { SET_SYSTEM_SLEEP_PM_OPS(
	nfc_i2c_dev_suspend, nfc_i2c_dev_resume) };

static struct i2c_driver nfc_i2c_dev_driver = {
	.id_table = nfc_i2c_dev_id,
	.probe = nfc_i2c_dev_probe,
	.remove = nfc_i2c_dev_remove,
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	.shutdown = nfc_shutdown,
#endif
	.driver = {
		.name = NFC_I2C_DRV_STR,
		.pm = &nfc_i2c_dev_pm_ops,
		.of_match_table = nfc_i2c_dev_match_table,
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
	},
};

MODULE_DEVICE_TABLE(of, nfc_i2c_dev_match_table);

#if IS_MODULE(CONFIG_SAMSUNG_NFC)
extern int p61_dev_init(void);
extern void p61_dev_exit(void);
static int __init nfc_i2c_dev_init(void)
{
	int ret = 0;

#ifdef CONFIG_SEC_NFC_LOGGER
	nfc_logger_init();
	nfc_logger_set_max_count(-1);
#endif
	NFC_LOG_INFO("%s: Loading NXP NFC I2C driver\n", __func__);
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	if (nfc_get_lpcharge() == LPM_TRUE)
		return ret;
#endif
#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	ret = platform_driver_register(&nfc_platform_driver);
	NFC_LOG_INFO("%s: platform_driver_register, ret %d\n", __func__, ret);
#endif
	ret = i2c_add_driver(&nfc_i2c_dev_driver);
	if (ret != 0)
		NFC_LOG_ERR("%s: NFC I2C add driver error ret %d\n", __func__, ret);
#if IS_ENABLED(CONFIG_NFC_SN2XX_ESE_SUPPORT)
	ret = p61_dev_init();
#endif
	return ret;
}

module_init(nfc_i2c_dev_init);

static void __exit nfc_i2c_dev_exit(void)
{
	NFC_LOG_INFO("%s: Unloading NXP NFC I2C driver\n", __func__);
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	if (nfc_get_lpcharge() == LPM_TRUE)
		return;
#endif
#if IS_ENABLED(CONFIG_NFC_SN2XX_ESE_SUPPORT)
	p61_dev_exit();
#endif
	i2c_del_driver(&nfc_i2c_dev_driver);
#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	platform_driver_unregister(&nfc_platform_driver);
#endif
}

module_exit(nfc_i2c_dev_exit);
#else /* not IS_MODULE(CONFIG_SAMSUNG_NFC) */
static int __init nfc_i2c_dev_init(void)
{
	int ret = 0;

#ifdef CONFIG_SEC_NFC_LOGGER
	nfc_logger_init();
	nfc_logger_set_max_count(-1);
#endif
	NFC_LOG_INFO("%s: Loading NXP NFC I2C driver\n", __func__);
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	if (nfc_get_lpcharge() == LPM_TRUE)
		return ret;
#endif
#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	ret = platform_driver_register(&nfc_platform_driver);
	NFC_LOG_INFO("%s: platform_driver_register, ret %d\n", __func__, ret);
#endif
	ret = i2c_add_driver(&nfc_i2c_dev_driver);
	if (ret != 0)
		NFC_LOG_ERR("%s: NFC I2C add driver error ret %d\n", __func__, ret);

	return ret;
}

module_init(nfc_i2c_dev_init);

static void __exit nfc_i2c_dev_exit(void)
{
	NFC_LOG_INFO("%s: Unloading NXP NFC I2C driver\n", __func__);

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	if (nfc_get_lpcharge() == LPM_TRUE)
		return;
#endif

	i2c_del_driver(&nfc_i2c_dev_driver);
#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	platform_driver_unregister(&nfc_platform_driver);
#endif
}

module_exit(nfc_i2c_dev_exit);
#endif /* IS_MODULE(CONFIG_SAMSUNG_NFC) */

MODULE_DESCRIPTION("NXP NFC I2C driver");
MODULE_LICENSE("GPL");
