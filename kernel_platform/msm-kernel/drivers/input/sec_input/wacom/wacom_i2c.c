/*
 *  wacom.c - Wacom Digitizer Controller (I2C bus)
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "wacom_dev.h"

static u32 slavepcb;
module_param(slavepcb, uint, 0644);
MODULE_PARM_DESC(slavepcb, "slave pcb id");

int wacom_send_sel(struct wacom_data *wacom, const char *buf, int count, bool mode)
{
	struct i2c_client *client = mode ? wacom->client_boot : wacom->client;
	int retry = WACOM_I2C_RETRY;
	int ret;
	u8 *buff;
	int i;

	/* in LPM, waiting blsp block resume */
	if (!wacom->plat_data->resume_done.done) {
		__pm_wakeup_event(wacom->plat_data->sec_ws, SEC_TS_WAKE_LOCK_TIME);
		ret = wait_for_completion_interruptible_timeout(&wacom->plat_data->resume_done,
				msecs_to_jiffies(SEC_TS_WAKE_LOCK_TIME));
		if (ret <= 0) {
			input_err(true, wacom->dev,
					"%s: LPM: pm resume is not handled [timeout]\n", __func__);
			return -ENOMEM;
		} else {
			input_info(true, wacom->dev,
					"%s: run LPM interrupt handler, %d\n", __func__, jiffies_to_msecs(ret));
		}
	}

	buff = kzalloc(count, GFP_KERNEL);
	if (!buff)
		return -ENOMEM;

	mutex_lock(&wacom->i2c_mutex);

	memcpy(buff, buf, count);

	reinit_completion(&wacom->i2c_done);
	do {
		if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
			input_err(true, &client->dev, "%s: Power status off\n", __func__);
			ret = -EIO;

			goto out;
		}

		ret = i2c_master_send(client, buff, count);
		if (ret == count)
			break;

		if (retry < WACOM_I2C_RETRY) {
			input_err(true, &client->dev, "%s: I2C retry(%d) mode(%d)\n",
					__func__, WACOM_I2C_RETRY - retry, mode);
			wacom->i2c_fail_count++;
		}
	} while (--retry);

out:
	complete_all(&wacom->i2c_done);
	mutex_unlock(&wacom->i2c_mutex);

	if (wacom->debug_flag & WACOM_DEBUG_PRINT_I2C_WRITE_CMD) {
		pr_info("sec_input : i2c_cmd: W: ");
		for (i = 0; i < count; i++)
			pr_cont("%02X ", buf[i]);
		pr_cont("\n");
	}

	kfree(buff);

	return ret;
}

int wacom_recv_sel(struct wacom_data *wacom, char *buf, int count, bool mode)
{
	struct i2c_client *client = mode ? wacom->client_boot : wacom->client;
	int retry = WACOM_I2C_RETRY;
	int ret;
	u8 *buff;
	int i;

	/* in LPM, waiting blsp block resume */
	if (!wacom->plat_data->resume_done.done) {
		__pm_wakeup_event(wacom->plat_data->sec_ws, SEC_TS_WAKE_LOCK_TIME);
		ret = wait_for_completion_interruptible_timeout(&wacom->plat_data->resume_done,
				msecs_to_jiffies(SEC_TS_WAKE_LOCK_TIME));
		if (ret <= 0) {
			input_err(true, wacom->dev,
					"%s: LPM: pm resume is not handled [timeout]\n", __func__);
			return -ENOMEM;
		} else {
			input_info(true, wacom->dev,
					"%s: run LPM interrupt handler, %d\n", __func__, jiffies_to_msecs(ret));
		}
	}

	buff = kzalloc(count, GFP_KERNEL);
	if (!buff)
		return -ENOMEM;

	mutex_lock(&wacom->i2c_mutex);

	reinit_completion(&wacom->i2c_done);
	do {
		if (sec_input_cmp_ic_status(wacom->dev, CHECK_POWEROFF)) {
			input_err(true, &client->dev, "%s: Power status off\n",	__func__);
			ret = -EIO;

			goto out;
		}

		ret = i2c_master_recv(client, buff, count);
		if (ret == count)
			break;

		if (retry < WACOM_I2C_RETRY) {
			input_err(true, &client->dev, "%s: I2C retry(%d) mode(%d)\n",
					__func__, WACOM_I2C_RETRY - retry, mode);
			wacom->i2c_fail_count++;
		}
	} while (--retry);

	memcpy(buf, buff, count);

	if (wacom->debug_flag & WACOM_DEBUG_PRINT_I2C_READ_CMD) {
		pr_info("sec_input : i2c_cmd: R: ");
		for (i = 0; i < count; i++)
			pr_cont("%02X ", buf[i]);
		pr_cont("\n");
	}

out:
	complete_all(&wacom->i2c_done);
	mutex_unlock(&wacom->i2c_mutex);

	kfree(buff);

	return ret;
}

int wacom_send_boot(struct wacom_data *wacom, const char *buf, int count)
{
	return wacom_send_sel(wacom, buf, count, WACOM_I2C_MODE_BOOT);
}

int wacom_send(struct wacom_data *wacom, const char *buf, int count)
{
	return wacom_send_sel(wacom, buf, count, WACOM_I2C_MODE_NORMAL);
}

int wacom_recv_boot(struct wacom_data *wacom, char *buf, int count)
{
	return wacom_recv_sel(wacom, buf, count, WACOM_I2C_MODE_BOOT);
}

int wacom_recv(struct wacom_data *wacom, char *buf, int count)
{
	return wacom_recv_sel(wacom, buf, count, WACOM_I2C_MODE_NORMAL);
}

static int wacom_i2c_init(struct i2c_client *client)
{
	struct wacom_data *wacom;
	struct sec_ts_plat_data *pdata;
	int ret = 0;

	wacom = devm_kzalloc(&client->dev, sizeof(struct wacom_data), GFP_KERNEL);
	if (!wacom) {
		ret = -ENOMEM;
		goto error_allocate_pdata;
	}

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev, sizeof(struct sec_ts_plat_data), GFP_KERNEL);
		if (!pdata) {
			ret = -ENOMEM;
			goto error_allocate_pdata;
		}
		client->dev.platform_data = pdata;
		ret = sec_input_parse_dt(&client->dev);
		if (ret) {
			input_err(true, &client->dev, "failed to sec_input_parse_dt\n");
			goto error_allocate_pdata;
		}
	} else {
		pdata = client->dev.platform_data;
		if (!pdata) {
			ret = -ENOMEM;
			input_err(true, &client->dev, "%s: No platform data found\n", __func__);
			goto error_allocate_pdata;
		}
	}

	client->irq = gpio_to_irq(pdata->irq_gpio);
	wacom->client = client;
	wacom->irq = client->irq;
	wacom->plat_data = pdata;
	wacom->dev = &client->dev;

	ret = wacom_init(wacom);
	if (ret < 0) {
		input_err(true, &client->dev, "%s: fail to do ts init\n", __func__);
		goto error_init;
	}

	/* using 2 slave address. one is normal mode, another is boot mode for
	 * fw update.
	 */
#if (KERNEL_VERSION(5, 5, 0) <= LINUX_VERSION_CODE)
	wacom->client_boot = i2c_new_dummy_device(client->adapter, wacom->boot_addr);
#else
	wacom->client_boot = i2c_new_dummy(client->adapter, wacom->boot_addr);
#endif
	if (!wacom->client_boot) {
		ret = -ENOMEM;
		input_err(true, &client->dev, "failed to register sub client[0x%x]\n", wacom->boot_addr);
		goto error_allocate_pdata;
	}
		/*Set client data */
	i2c_set_clientdata(client, wacom);
	i2c_set_clientdata(wacom->client_boot, wacom);

	return 0;

error_init:
error_allocate_pdata:
	input_err(true, &client->dev, "%s: failed(%d)\n", __func__, ret);
	input_log_fix();
	return ret;
}
#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
static int wacom_i2c_probe(struct i2c_client *client)
#else
static int wacom_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
#endif
{
	struct wacom_data *wacom;
	int ret = 0;

	input_info(true, &client->dev, "%s\n", __func__);

	ret = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);
	if (!ret) {
		input_err(true, &client->dev, "I2C functionality not supported\n");
		return -EIO;
	}

	ret = wacom_i2c_init(client);
	if (ret < 0) {
		input_err(true, &client->dev, "%s: fail to init resource\n", __func__);
		return ret;
	}

	wacom = i2c_get_clientdata(client);

	wacom->slavepcb = slavepcb;
	input_info(true, &client->dev, "%s : slavepcb is: %d\n",	__func__, wacom->slavepcb);

	return wacom_probe(wacom);
}

#ifdef CONFIG_PM
static int wacom_i2c_suspend(struct device *dev)
{
	struct wacom_data *wacom = dev_get_drvdata(dev);
	int ret;

	if (wacom->i2c_done.done == 0) {
		/* completion.done == 0 :: initialized
		 * completion.done > 0 :: completeted
		 */
		ret = wait_for_completion_interruptible_timeout(&wacom->i2c_done,
				msecs_to_jiffies(SEC_TS_WAKE_LOCK_TIME));
		if (ret <= 0)
			input_err(true, wacom->dev, "%s: completion expired, %d\n", __func__, ret);
	}

	wacom->pm_suspend = true;
	reinit_completion(&wacom->plat_data->resume_done);

#ifndef USE_OPEN_CLOSE
	if (wacom->input_dev->users)
		wacom_sleep_sequence(wacom);
#endif

	return 0;
}

static int wacom_i2c_resume(struct device *dev)
{
	struct wacom_data *wacom = dev_get_drvdata(dev);

	wacom->pm_suspend = false;
	complete_all(&wacom->plat_data->resume_done);
#ifndef USE_OPEN_CLOSE
	if (wacom->input_dev->users)
		wacom_wakeup_sequence(wacom);
#endif

	return 0;
}

static SIMPLE_DEV_PM_OPS(wacom_pm_ops, wacom_i2c_suspend, wacom_i2c_resume);
#endif

static void wacom_i2c_shutdown(struct i2c_client *client)
{
	struct wacom_data *wacom = i2c_get_clientdata(client);

	if (!wacom)
		return;

	g_wacom = NULL;
	wacom->probe_done = false;
	wacom->plat_data->enable = NULL;
	wacom->plat_data->disable = NULL;

	input_info(true, wacom->dev, "%s called!\n", __func__);

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	sec_input_unregister_notify(&wacom->nb);
#endif

	if (gpio_is_valid(wacom->esd_detect_gpio))
		cancel_delayed_work_sync(&wacom->esd_reset_work);
	cancel_delayed_work_sync(&wacom->open_test_dwork);
	cancel_delayed_work_sync(&wacom->work_print_info);

	wacom_enable_irq(wacom, false);

	atomic_set(&wacom->plat_data->power_state, SEC_INPUT_STATE_POWER_OFF);
	wacom_power(wacom, false);

	input_info(true, wacom->dev, "%s Done\n", __func__);
}

#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
static void wacom_i2c_remove(struct i2c_client *client)
#else
static int wacom_i2c_remove(struct i2c_client *client)
#endif
{
	struct wacom_data *wacom = i2c_get_clientdata(client);

	wacom_dev_remove(wacom);

#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	return;
#else
	return 0;
#endif
}

static const struct i2c_device_id wacom_id[] = {
	{"wacom,wez02", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, wacom_id);

#ifdef CONFIG_OF
static const struct of_device_id wacom_dt_ids[] = {
	{.compatible = "wacom,wez02"},
	{}
};
#endif

static struct i2c_driver wacom_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "wacom,wez02",
#ifdef CONFIG_PM
		.pm = &wacom_pm_ops,
#endif
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(wacom_dt_ids),
#endif
	},
	.probe = wacom_i2c_probe,
	.remove = wacom_i2c_remove,
	.shutdown = wacom_i2c_shutdown,
	.id_table = wacom_id,
};

module_i2c_driver(wacom_driver);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("Driver for Wacom Digitizer Controller");
MODULE_LICENSE("GPL");
