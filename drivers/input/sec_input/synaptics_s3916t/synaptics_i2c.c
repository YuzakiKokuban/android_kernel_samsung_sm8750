/* drivers/input/sec_input/synaptics/synaptics_core.c
 *
 * Copyright (C) 2020 Samsung Electronics Co., Ltd.
 *
 * Core file for Samsung TSC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "synaptics_dev.h"
#include "synaptics_reg.h"

#define RD_CHUNK_SIZE_I2C (1024)
#define WR_CHUNK_SIZE_I2C (1024)


static int synaptics_ts_i2c_write(struct synaptics_ts_data *ts, u8 *reg, int cnum, u8 *data, int len)
{
	struct i2c_client *client = ts->client;
	int ret;
	unsigned char retry;
	struct i2c_msg msg;
	int i;
	const int len_max = 0xffff;
	u8 *buf;
	u8 *buff;

	if (len + 1 > len_max) {
		input_err(true, ts->dev,
				"%s: The i2c buffer size is exceeded.\n", __func__);
		return -ENOMEM;
	}

	if (atomic_read(&ts->plat_data->shutdown_called)) {
		input_err(true, ts->dev, "%s: shutdown called\n", __func__);
		return -EIO;
	}

	if (!ts->plat_data->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->plat_data->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, ts->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

	if (sec_check_secure_trusted_mode_status(ts->plat_data))
		return -EBUSY;

#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
	buf = kzalloc(cnum, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memcpy(buf, reg, cnum);

	buff = kzalloc(len + cnum, GFP_KERNEL);
	if (!buff) {
		kfree(buf);
		return -ENOMEM;
	}

	if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
		input_err(true, ts->dev, "%s: POWER_STATUS : OFF\n", __func__);
		goto err;
	}

	memcpy(buff, buf, cnum);
	memcpy(buff + cnum, data, len);

	msg.addr = client->addr;
	msg.flags = 0 | I2C_M_DMA_SAFE;
	msg.len = len + cnum;
	msg.buf = buff;

	mutex_lock(&ts->transfer_mutex);
	for (retry = 0; retry < SYNAPTICS_TS_I2C_RETRY_CNT; retry++) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1)
			break;

		if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
			input_err(true, ts->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
			mutex_unlock(&ts->transfer_mutex);
			goto err;
		}

		sec_delay(20);
	}

	mutex_unlock(&ts->transfer_mutex);

	if (retry == SYNAPTICS_TS_I2C_RETRY_CNT) {
		char result[32];
		input_err(true, ts->dev, "%s: I2C write over retry limit retry:%d\n", __func__, retry);
		ts->plat_data->hw_param.comm_err_count++;

		snprintf(result, sizeof(result), "RESULT=I2C");
		sec_cmd_send_event_to_user(&ts->sec, NULL, result);

		ret = -EIO;
		if (ts->probe_done && !atomic_read(&ts->reset_is_on_going) && !atomic_read(&ts->plat_data->shutdown_called))
			schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
	}

	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_WRITE_CMD) {
		char *dbuff;
		char *dtbuff;
		int dbuff_len = ((cnum + len) * 3) + 6;
		int dtbuff_len = 3 * 3 + 3;

		dbuff = kzalloc(dbuff_len, GFP_KERNEL);
		if (!dbuff)
			goto out_write;

		dtbuff = kzalloc(dtbuff_len, GFP_KERNEL);
		if (!dtbuff) {
			kfree(dbuff);
			goto out_write;
		}

		for (i = 0; i < cnum; i++) {
			memset(dtbuff, 0x00, dtbuff_len);
			snprintf(dtbuff, dtbuff_len, "%02X ", reg[i]);
			strlcat(dbuff, dtbuff, dbuff_len);
		}
		strlcat(dbuff, " | ", dbuff_len);
		for (i = 0; i < len; i++) {
			memset(dtbuff, 0x00, dtbuff_len);
			snprintf(dtbuff, dtbuff_len, "%02X ", data[i]);
			strlcat(dbuff, dtbuff, dbuff_len);
		}
		input_info(true, ts->dev, "%s: debug: %s\n", __func__, dbuff);
		kfree(dbuff);
		kfree(dtbuff);
	}

out_write:
	if (ret == 1) {
		kfree(buf);
		kfree(buff);
		return 0;
	}
err:
	kfree(buf);
	kfree(buff);
	return -EIO;
}


/**
 * syna_i2c_read()
 *
 * TouchCom over I2C uses the normal I2C addressing and transaction direction
 * mechanisms to select the device and retrieve the data.
 *
 * @param
 *    [ in] hw_if:   the handle of hw interface
 *    [out] rd_data: buffer for storing data retrieved from device
 *    [ in] rd_len: number of bytes retrieved from device
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int synaptics_ts_i2c_only_read(struct synaptics_ts_data *ts, u8 *data, int len)
{
	struct i2c_client *client = ts->client;
	int ret;
	unsigned char retry;
	struct i2c_msg msg;
	int remain = len;
	int i;
	u8 *buff;

	if (atomic_read(&ts->plat_data->shutdown_called)) {
		input_err(true, ts->dev, "%s: shutdown called\n", __func__);
		return -EIO;
	}

	if (!ts->plat_data->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->plat_data->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, ts->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}
	if (sec_check_secure_trusted_mode_status(ts->plat_data))
		return -EBUSY;

#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif

	buff = kzalloc(len, GFP_KERNEL);
	if (!buff)
		return -ENOMEM;

	if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
		input_err(true, ts->dev, "%s: POWER_STATUS : OFF\n", __func__);
		goto err;
	}

	msg.addr = client->addr;
	msg.flags = I2C_M_RD | I2C_M_DMA_SAFE;
	msg.buf = buff;

	mutex_lock(&ts->transfer_mutex);
	if (len <= ts->plat_data->i2c_burstmax) {
		msg.len = len;
		for (retry = 0; retry < SYNAPTICS_TS_I2C_RETRY_CNT; retry++) {
			ret = i2c_transfer(client->adapter, &msg, 1);
			if (ret == 1)
				break;
			sec_delay(20);
			if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
				input_err(true, ts->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
				mutex_unlock(&ts->transfer_mutex);
				goto err;
			}
		}
	} else {
		/*
		 * I2C read buffer is 256 byte. do not support long buffer over than 256.
		 * So, try to seperate reading data about 256 bytes.
		 */
		for (retry = 0; retry < SYNAPTICS_TS_I2C_RETRY_CNT; retry++) {
			ret = i2c_transfer(client->adapter, &msg, 1);
			if (ret == 1)
				break;
			sec_delay(20);
			if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
				input_err(true, ts->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
				mutex_unlock(&ts->transfer_mutex);
				goto err;
			}
		}

		do {
			if (remain > ts->plat_data->i2c_burstmax)
				msg.len = ts->plat_data->i2c_burstmax;
			else
				msg.len = remain;

			remain -= ts->plat_data->i2c_burstmax;

			for (retry = 0; retry < SYNAPTICS_TS_I2C_RETRY_CNT; retry++) {
				ret = i2c_transfer(client->adapter, &msg, 1);
				if (ret == 1)
					break;
				sec_delay(20);
				if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
					input_err(true, ts->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
					mutex_unlock(&ts->transfer_mutex);
					goto err;
				}
			}
			msg.buf += msg.len;
		} while (remain > 0);
	}

	mutex_unlock(&ts->transfer_mutex);

	if (retry == SYNAPTICS_TS_I2C_RETRY_CNT) {
		char result[32];
		input_err(true, ts->dev, "%s: I2C read over retry limit retry:%d\n", __func__, retry);
		ts->plat_data->hw_param.comm_err_count++;

		snprintf(result, sizeof(result), "RESULT=I2C");
		sec_cmd_send_event_to_user(&ts->sec, NULL, result);

		ret = -EIO;
		if (ts->probe_done && !atomic_read(&ts->reset_is_on_going) && !atomic_read(&ts->plat_data->shutdown_called))
			schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
	}

	memcpy(data, buff, len);
	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_READ_CMD) {
		char *dbuff;
		char *dtbuff;
		int dbuff_len = (len * 3) + 6;
		int dtbuff_len = 3 * 3 + 3;

		dbuff = kzalloc(dbuff_len, GFP_KERNEL);
		if (!dbuff)
			goto out_read;

		dtbuff = kzalloc(dtbuff_len, GFP_KERNEL);
		if (!dtbuff) {
			kfree(dbuff);
			goto out_read;
		}

		for (i = 0; i < len; i++) {
			memset(dtbuff, 0x00, dtbuff_len);
			snprintf(dtbuff, dtbuff_len, "%02X ", data[i]);
			strlcat(dbuff, dtbuff, dbuff_len);
		}
		input_info(true, ts->dev, "%s: debug: %s\n", __func__, dbuff);
		kfree(dbuff);
		kfree(dtbuff);
	}

out_read:
	kfree(buff);
	return ret;
err:
	kfree(buff);
	return -EIO;
}

static int synaptics_ts_i2c_read(struct synaptics_ts_data *ts, u8 *reg, int cnum, u8 *data, int len)
{
	int ret;
	unsigned char retry;
	struct i2c_client *client = ts->client;
	struct i2c_msg msg[2];
	int remain = len;
	int i;
	u8 *buff;
	u8 *buf;

	if (atomic_read(&ts->plat_data->shutdown_called)) {
		input_err(true, ts->dev, "%s: shutdown called\n", __func__);
		return -EIO;
	}

	if (!ts->plat_data->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->plat_data->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, ts->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

	if (sec_check_secure_trusted_mode_status(ts->plat_data))
		return -EBUSY;
#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
	buf = kzalloc(cnum, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memcpy(buf, reg, cnum);

	buff = kzalloc(len, GFP_KERNEL);
	if (!buff) {
		kfree(buf);
		return -ENOMEM;
	}

	if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
		input_err(true, ts->dev, "%s: POWER_STATUS : OFF\n", __func__);
		goto err;
	}

	msg[0].addr = client->addr;
	msg[0].flags = 0 | I2C_M_DMA_SAFE;
	msg[0].len = cnum;
	msg[0].buf = buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD | I2C_M_DMA_SAFE;
	msg[1].buf = buff;

	mutex_lock(&ts->transfer_mutex);
	if (len <= ts->plat_data->i2c_burstmax) {
		msg[1].len = len;
		for (retry = 0; retry < SYNAPTICS_TS_I2C_RETRY_CNT; retry++) {
			ret = i2c_transfer(client->adapter, msg, 2);
			if (ret == 2)
				break;
			sec_delay(20);
			if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
				input_err(true, ts->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
				mutex_unlock(&ts->transfer_mutex);
				goto err;
			}
		}
	} else {
		/*
		 * I2C read buffer is 256 byte. do not support long buffer over than 256.
		 * So, try to seperate reading data about 256 bytes.
		 */
		for (retry = 0; retry < SYNAPTICS_TS_I2C_RETRY_CNT; retry++) {
			ret = i2c_transfer(client->adapter, msg, 1);
			if (ret == 1)
				break;
			sec_delay(20);
			if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
				input_err(true, ts->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
				mutex_unlock(&ts->transfer_mutex);
				goto err;
			}
		}

		do {
			if (remain > ts->plat_data->i2c_burstmax)
				msg[1].len = ts->plat_data->i2c_burstmax;
			else
				msg[1].len = remain;

			remain -= ts->plat_data->i2c_burstmax;

			for (retry = 0; retry < SYNAPTICS_TS_I2C_RETRY_CNT; retry++) {
				ret = i2c_transfer(client->adapter, &msg[1], 1);
				if (ret == 1)
					break;
				sec_delay(20);
				if (sec_input_cmp_ic_status(ts->dev, CHECK_POWEROFF)) {
					input_err(true, ts->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
					mutex_unlock(&ts->transfer_mutex);
					goto err;
				}
			}
			msg[1].buf += msg[1].len;
		} while (remain > 0);
	}

	mutex_unlock(&ts->transfer_mutex);

	if (retry == SYNAPTICS_TS_I2C_RETRY_CNT) {
		char result[32];
		input_err(true, ts->dev, "%s: I2C read over retry limit retry:%d\n", __func__, retry);
		ts->plat_data->hw_param.comm_err_count++;

		snprintf(result, sizeof(result), "RESULT=I2C");
		sec_cmd_send_event_to_user(&ts->sec, NULL, result);

		ret = -EIO;
		if (ts->probe_done && !atomic_read(&ts->reset_is_on_going) && !atomic_read(&ts->plat_data->shutdown_called))
			schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
	}

	memcpy(data, buff, len);
	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_READ_CMD) {
		char *dbuff;
		char *dtbuff;
		int dbuff_len = ((cnum + len) * 3) + 6;
		int dtbuff_len = 3 * 3 + 3;

		dbuff = kzalloc(dbuff_len, GFP_KERNEL);
		if (!dbuff)
			goto out_read;

		dtbuff = kzalloc(dtbuff_len, GFP_KERNEL);
		if (!dtbuff) {
			kfree(dbuff);
			goto out_read;
		}

		for (i = 0; i < cnum; i++) {
			memset(dtbuff, 0x00, dtbuff_len);
			snprintf(dtbuff, dtbuff_len, "%02X ", reg[i]);
			strlcat(dbuff, dtbuff, dbuff_len);
		}
		strlcat(dbuff, " | ", dbuff_len);
		for (i = 0; i < len; i++) {
			memset(dtbuff, 0x00, dtbuff_len);
			snprintf(dtbuff, dtbuff_len, "%02X ", data[i]);
			strlcat(dbuff, dtbuff, dbuff_len);
		}
		input_info(true, ts->dev, "%s: debug: %s\n", __func__, dbuff);
		kfree(dbuff);
		kfree(dtbuff);
	}

out_read:
	kfree(buf);
	kfree(buff);
	return ret;
err:
	kfree(buf);
	kfree(buff);
	return -EIO;
}

static int synaptics_ts_i2c_get_baredata(struct synaptics_ts_data *ts, u8 *data, int len)
{
	unsigned char address = 0x07;

	return ts->synaptics_ts_read_data(ts, &address, 1, data, len);
}

#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
extern int stui_i2c_lock(struct i2c_adapter *adap);
extern int stui_i2c_unlock(struct i2c_adapter *adap);

int synaptics_stui_tsp_enter(void)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(ptsp);
	int ret = 0;
	struct i2c_client *client;

	if (!ts)
		return -EINVAL;

	client = (struct i2c_client *)ts->client;

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	sec_input_notify(&ts->synaptics_input_nb, NOTIFIER_SECURE_TOUCH_ENABLE, NULL);
#endif

	sec_input_irq_disable(ts->plat_data);
	synaptics_ts_release_all_finger(ts);

	ret = stui_i2c_lock(client->adapter);
	if (ret) {
		pr_err("[STUI] stui_i2c_lock failed : %d\n", ret);
#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
		sec_input_notify(&ts->synaptics_input_nb, NOTIFIER_SECURE_TOUCH_DISABLE, NULL);
#endif
		sec_input_irq_enable(ts->plat_data);
		return -1;
	}

	return 0;
}

int synaptics_stui_tsp_exit(void)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(ptsp);
	int ret = 0;
	struct i2c_client *client;

	if (!ts)
		return -EINVAL;

	client = (struct i2c_client *)ts->client;

	ret = stui_i2c_unlock(client->adapter);
	if (ret)
		pr_err("[STUI] stui_i2c_unlock failed : %d\n", ret);

	sec_input_irq_enable(ts->plat_data);

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	sec_input_notify(&ts->synaptics_input_nb, NOTIFIER_SECURE_TOUCH_DISABLE, NULL);
#endif

	return ret;
}

int synaptics_stui_tsp_type(void)
{
	return STUI_TSP_TYPE_SYNAPTICS;
}
#endif

static int synaptics_ts_i2c_init(struct i2c_client *client)
{
	struct synaptics_ts_data *ts;
	struct sec_ts_plat_data *pdata;
	struct sec_tclm_data *tdata = NULL;
	int ret = 0;

	ts = devm_kzalloc(&client->dev, sizeof(struct synaptics_ts_data), GFP_KERNEL);
	if (!ts) {
		ret = -ENOMEM;
		goto error_allocate_pdata;
	}

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
				sizeof(struct sec_ts_plat_data), GFP_KERNEL);

		if (!pdata) {
			ret = -ENOMEM;
			goto error_allocate_pdata;
		}

		client->dev.platform_data = pdata;

		ret = sec_input_parse_dt(&client->dev);
		if (ret) {
			input_err(true, &client->dev, "%s: Failed to parse dt\n", __func__);
			goto error_allocate_pdata;
		}
		tdata = devm_kzalloc(&client->dev,
				sizeof(struct sec_tclm_data), GFP_KERNEL);
		if (!tdata) {
			ret = -ENOMEM;
			goto error_allocate_mem;
		}

#ifdef TCLM_CONCEPT
		sec_tclm_parse_dt(&client->dev, tdata);
#endif
	} else {
		pdata = client->dev.platform_data;
		if (!pdata) {
			ret = -ENOMEM;
			input_err(true, &client->dev, "%s: No platform data found\n", __func__);
			goto error_allocate_pdata;
		}
	}

	pdata->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(pdata->pinctrl))
		input_err(true, &client->dev, "%s: could not get pinctrl\n", __func__);

#if !IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
	ts->vendor_data = devm_kzalloc(&client->dev, sizeof(struct synaptics_ts_sysfs), GFP_KERNEL);
	if (!ts->vendor_data) {
		ret = -ENOMEM;
		goto error_allocate_mem;
	}
#endif

	client->irq = gpio_to_irq(pdata->irq_gpio);

	ts->client = client;
	ts->dev = &client->dev;
	ts->plat_data = pdata;
	ts->plat_data->bus_master = &client->adapter->dev;
	ts->irq = client->irq;

#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
	ts->plat_data->stui_tsp_enter = synaptics_stui_tsp_enter;
	ts->plat_data->stui_tsp_exit = synaptics_stui_tsp_exit;
	ts->plat_data->stui_tsp_type = synaptics_stui_tsp_type;
#endif

	mutex_init(&ts->transfer_mutex);

	ts->synaptics_ts_write_data = synaptics_ts_i2c_write;
	ts->synaptics_ts_read_data = synaptics_ts_i2c_read;
	ts->synaptics_ts_read_data_only = synaptics_ts_i2c_only_read;
	ts->synaptics_ts_get_baredata = synaptics_ts_i2c_get_baredata;

	if (!ts->synaptics_ts_write_data || !ts->synaptics_ts_read_data) {
		input_err(true, &client->dev, "%s: not valid r/w operations\n", __func__);
		ret = -ENOMEM;
		goto error_allocate_mem;
	}

	ts->tdata = tdata;
	if (!ts->tdata) {
		ret = -ENOMEM;
		goto error_allocate_mem;
	}

	ts->max_rd_size = RD_CHUNK_SIZE_I2C;
	ts->max_wr_size = WR_CHUNK_SIZE_I2C;

	ret = synaptics_ts_init(ts);
	if (ret < 0) {
		input_err(true, &client->dev, "%s: fail to do ts init\n", __func__);
		goto error_init;
	}

	i2c_set_clientdata(client, ts);

	return 0;

error_init:
error_allocate_mem:
error_allocate_pdata:
	input_err(true, &client->dev, "%s: failed(%d)\n", __func__, ret);
	input_log_fix();
	return ret;
}

static int synaptics_ts_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct synaptics_ts_data *ts;
	int ret = 0;

	input_info(true, &client->dev, "%s\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev, "%s: EIO err!\n", __func__);
		return -EIO;
	}

	ret = synaptics_ts_i2c_init(client);
	if (ret < 0) {
		input_err(true, &client->dev, "%s: fail to init resource\n", __func__);
		return ret;
	}

	ts = i2c_get_clientdata(client);
	if (!ts->plat_data->work_queue_probe_enabled)
		return synaptics_ts_probe(ts->dev);

	queue_work(ts->plat_data->probe_workqueue, &ts->plat_data->probe_work);
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static void synaptics_ts_i2c_remove(struct i2c_client *client)
{
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);

	synaptics_ts_remove(ts);
}
#else
static int synaptics_ts_i2c_remove(struct i2c_client *client)
{
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);

	synaptics_ts_remove(ts);

	return 0;
}
#endif

static void synaptics_ts_i2c_shutdown(struct i2c_client *client)
{
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);

	synaptics_ts_shutdown(ts);
}

static const struct i2c_device_id synaptics_ts_id[] = {
	{ SYNAPTICS_TS_I2C_NAME, 0 },
	{ },
};

#if IS_ENABLED(CONFIG_PM)
static const struct dev_pm_ops synaptics_ts_dev_pm_ops = {
	.suspend = synaptics_ts_pm_suspend,
	.resume = synaptics_ts_pm_resume,
};
#endif

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id synaptics_ts_match_table[] = {
	{ .compatible = "synaptics,s3916t",},
	{ },
};
#else
#define synaptics_ts_match_table NULL
#endif

static struct i2c_driver synaptics_ts_driver = {
	.probe		= synaptics_ts_i2c_probe,
	.remove		= synaptics_ts_i2c_remove,
	.shutdown	= synaptics_ts_i2c_shutdown,
	.id_table	= synaptics_ts_id,
	.driver = {
		.owner	= THIS_MODULE,
		.name	= SYNAPTICS_TS_I2C_NAME,
#if IS_ENABLED(CONFIG_OF)
		.of_match_table = synaptics_ts_match_table,
#endif
#if IS_ENABLED(CONFIG_PM)
		.pm = &synaptics_ts_dev_pm_ops,
#endif
	},
};

module_i2c_driver(synaptics_ts_driver);

MODULE_SOFTDEP("pre: acpm-mfd-bus");
MODULE_DESCRIPTION("synaptics TouchScreen driver");
MODULE_LICENSE("GPL");
