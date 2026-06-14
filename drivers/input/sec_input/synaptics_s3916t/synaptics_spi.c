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

#define RD_CHUNK_SIZE_SPI (1024)
#define WR_CHUNK_SIZE_SPI (1024)

static unsigned char *rx_buf;
static unsigned char *tx_buf;

static unsigned int buf_size;

static struct spi_transfer *xfer;

static int synaptics_ts_spi_alloc_mem(struct spi_device *client, unsigned int size)
{
	if (!xfer) {
		xfer = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
		if (!xfer)
			return -ENOMEM;
	} else {
		memset(xfer, 0, sizeof(struct spi_transfer));
	}

	if (size > buf_size) {
		if (rx_buf) {
			kfree((void *)rx_buf);
			rx_buf = NULL;
		}
		if (tx_buf) {
			kfree((void *)tx_buf);
			tx_buf = NULL;
		}

		rx_buf = kzalloc(size, GFP_KERNEL);
		if (!rx_buf) {
			buf_size = 0;
			return -ENOMEM;
		}
		tx_buf = kzalloc(size, GFP_KERNEL);
		if (!tx_buf) {
			buf_size = 0;
			return -ENOMEM;
		}

		buf_size = size;
	} else {
		memset(rx_buf, 0, size);
		memset(tx_buf, 0, size);
	}

	return 0;
}

static int synaptics_ts_spi_write(struct synaptics_ts_data *ts, u8 *reg, int cnum, u8 *data, int len)
{
	int ret;
	struct spi_message msg;
	struct spi_device *spi = ts->client;
	int i;
	const int len_max = 0xffff;

	if (!spi) {
		input_err(true, ts->dev,
			"%s: Invalid bus io device.\n", __func__);
		return -ENXIO;
	}

	if (cnum + len >= len_max) {
		input_err(true, ts->dev,
			"%s: The spi buffer size is exceeded.\n", __func__);
		return -ENOMEM;
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

	if (atomic_read(&ts->plat_data->power_state) == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, ts->dev, "%s: POWER_STATUS : OFF\n", __func__);
		return -EIO;
	}

	if (atomic_read(&ts->plat_data->shutdown_called)) {
		input_err(true, ts->dev, "%s shutdown was called\n", __func__);
		return -EIO;
	}

	mutex_lock(&ts->transfer_mutex);

	spi_message_init(&msg);

	ret = synaptics_ts_spi_alloc_mem(spi, len + cnum);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Failed to allocate memory\n", __func__);
		goto exit;
	}

	memcpy(tx_buf, reg, cnum);
	if (len > 0) {
		if (data) {
			memcpy(tx_buf + cnum, data, len);
		} else {
			ret = -ENOMEM;
			goto exit;
		}
	}

	spi_message_init(&msg);
	xfer[0].len = len + cnum;
	xfer[0].tx_buf = tx_buf;
	//xfer[0].rx_buf = rx_buf;
	spi_message_add_tail(&xfer[0], &msg);


	if (gpio_is_valid(ts->plat_data->gpio_spi_cs)) {
		gpio_direction_output(ts->plat_data->gpio_spi_cs, 0);
		if (gpio_get_value(ts->plat_data->gpio_spi_cs) != 0)
			input_err(true, ts->dev, "%s: gpio value invalid:%d\n",
				__func__, gpio_get_value(ts->plat_data->gpio_spi_cs));
	}

	if (spi->mode != SPI_MODE_0) {
		input_err(true, ts->dev, "%s: spi mode is not 0, %d\n",
				__func__, spi->mode);
		spi->mode = SPI_MODE_0;
	}

	ret = spi_sync(spi, &msg);
	if (ret != 0) {
		input_err(true, ts->dev, "%s: Failed to complete SPI transfer, error = %d\n",
				__func__, ret);
		ret = -EIO;
		goto exit;
	}

	if (gpio_is_valid(ts->plat_data->gpio_spi_cs)) {
		gpio_direction_output(ts->plat_data->gpio_spi_cs, 1);
		if (gpio_get_value(ts->plat_data->gpio_spi_cs) != 1)
			input_err(true, ts->dev, "%s: gpio value invalid:%d\n",
				__func__, gpio_get_value(ts->plat_data->gpio_spi_cs));
	}

	ret = len + cnum;

	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_WRITE_CMD) {
		unsigned char *wdata;

		wdata = kzalloc((cnum + len) * 3 + 6, GFP_KERNEL);

		for (i = 0; i < cnum; i++) {
			unsigned char strbuff[6];

			memset(strbuff, 0x00, 6);
			snprintf(strbuff, 6, "%02X ", reg[i]);
			strlcat(wdata, strbuff, (cnum + len) * 3 + 6);
		}
			
		for (i = 0; i < len; i++) {
			unsigned char strbuff[6];

			memset(strbuff, 0x00, 6);
			snprintf(strbuff, 6, "%02X ", data[i]);
			strlcat(wdata, strbuff, (cnum + len) * 3 + 6);
		}

		input_info(true, ts->dev, "%s: sec_input:spi_cmd: write(%d): %s\n", __func__, len, wdata);
		kfree(wdata);
	}
exit:
	mutex_unlock(&ts->transfer_mutex);

	return ret;
}

/**
 * syna_spi_read()
 *
 * TouchCom over SPI requires the host to assert the SSB signal to address
 * the device and retrieve the data.
 *
 * @param
 *    [ in] hw_if:   the handle of hw interface
 *    [out] rd_data: buffer for storing data retrieved from device
 *    [ in] rd_len: number of bytes retrieved from device
 *
 * @return
 *    on success, 0; otherwise, negative value on error.
 */
static int synaptics_ts_spi_only_read(struct synaptics_ts_data *ts, u8 *data, int len)
{
	int ret;
	struct spi_message msg;
	struct spi_device *spi = ts->client;
	int i;
	const int len_max = 0xffff;

	if (!spi) {
		input_err(true, ts->dev,
			"%s: Invalid bus io device.\n", __func__);
		return -ENXIO;
	}

	if (len >= len_max) {
		input_err(true, ts->dev,
				"%s: The spi buffer size is exceeded.\n", __func__);
		return -ENOMEM;
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

	if (atomic_read(&ts->plat_data->power_state) == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, ts->dev, "%s: POWER_STATUS : OFF\n", __func__);
		return -EIO;
	}

	if (atomic_read(&ts->plat_data->shutdown_called)) {
		input_err(true, ts->dev, "%s shutdown was called\n", __func__);
		return -EIO;
	}

	mutex_lock(&ts->transfer_mutex);

	spi_message_init(&msg);

	ret = synaptics_ts_spi_alloc_mem(spi, len);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Failed to allocate memory\n", __func__);
		goto exit;
	}

	memset(tx_buf, 0xff, len);
	xfer[0].len = len;
	xfer[0].tx_buf = tx_buf;
	xfer[0].rx_buf = rx_buf;
	spi_message_add_tail(&xfer[0], &msg);

	if (gpio_is_valid(ts->plat_data->gpio_spi_cs)) {
		gpio_direction_output(ts->plat_data->gpio_spi_cs, 0);
		if (gpio_get_value(ts->plat_data->gpio_spi_cs) != 0)
			input_err(true, ts->dev, "%s: gpio value invalid:%d\n",
				__func__, gpio_get_value(ts->plat_data->gpio_spi_cs));
	}

	if (spi->mode != SPI_MODE_0) {
		input_err(true, ts->dev, "%s: spi mode is not 0, %d\n",
				__func__, spi->mode);
		spi->mode = SPI_MODE_0;
	}

	ret = spi_sync(spi, &msg);
	if (ret != 0) {
		input_err(true, ts->dev, "%s: Failed to complete SPI transfer, error = %d\n",
				__func__, ret);
		ret = -EIO;
		goto exit;
	}

	if (gpio_is_valid(ts->plat_data->gpio_spi_cs)) {
		gpio_direction_output(ts->plat_data->gpio_spi_cs, 1);
		if (gpio_get_value(ts->plat_data->gpio_spi_cs) != 1)
			input_err(true, ts->dev, "%s: gpio value invalid:%d\n",
				__func__, gpio_get_value(ts->plat_data->gpio_spi_cs));
	}
	if (data) {
		memcpy(data, rx_buf, len);
	} else {
		ret = -ENOMEM;
		goto exit;
	}

	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_READ_CMD) {
		unsigned char *rdata;

		rdata = kzalloc(len * 3 + 3, GFP_KERNEL);
		for (i = 0; i < len; i++) {
			unsigned char strbuff[6];

			memset(strbuff, 0x00, 6);
			snprintf(strbuff, 6, "%02X ", rx_buf[i]);
			strlcat(rdata, strbuff, len * 3 + 3);
		}
			
		input_info(true, ts->dev, "%s: sec_input:spi_cmd: only read(%d): %s\n", __func__, len, rdata);
		kfree(rdata);
	}

	ret = len;

exit:
	mutex_unlock(&ts->transfer_mutex);

	return ret;
}

static int synaptics_ts_spi_read(struct synaptics_ts_data *ts, u8 *reg, int cnum, u8 *data, int len)
{
	int ret;

	ret = synaptics_ts_spi_write(ts, reg, cnum, NULL, 0);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Failed to complete SPI transfer, error = %d\n",
				__func__, ret);
		return ret;
	}

	ret = synaptics_ts_spi_only_read(ts, data, len);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Failed to complete SPI transfer, error = %d\n",
				__func__, ret);
		return ret;
	}

	return len;
}

static int synaptics_ts_spi_get_baredata(struct synaptics_ts_data *ts, u8 *data, int len)
{
	return ts->synaptics_ts_read_data_only(ts, data, len);
}


#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
extern int stui_spi_lock(struct spi_master *spi);
extern int stui_spi_unlock(struct spi_master *spi);

int synaptics_stui_tsp_enter(void)
{
	struct synaptics_ts_data *ts = dev_get_drvdata(ptsp);
	struct spi_device *client;
	int ret = 0;

	if (!ts)
		return -EINVAL;

	client = (struct spi_device *)ts->client;

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	sec_input_notify(&ts->synaptics_input_nb, NOTIFIER_SECURE_TOUCH_ENABLE, NULL);
#endif

	sec_input_irq_disable(ts->plat_data);
	synaptics_ts_release_all_finger(ts);

	ret = stui_spi_lock(client->controller);
	if (ret < 0) {
		pr_err("[STUI] stui_spi_lock failed : %d\n", ret);
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
	struct spi_device *client;
	int ret = 0;

	if (!ts)
		return -EINVAL;

	client = (struct spi_device *)ts->client;

	ret = stui_spi_unlock(client->controller);
	if (ret < 0)
		pr_err("[STUI] stui_spi_unlock failed : %d\n", ret);

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


static int synaptics_ts_spi_init(struct spi_device *client)
{
	struct synaptics_ts_data *ts;
	struct sec_ts_plat_data *pdata;
	struct sec_tclm_data *tdata = NULL;
	int ret = 0;

	input_info(true, &client->dev, "%s\n", __func__);

	rx_buf = NULL;
	tx_buf = NULL;
	xfer = NULL;
	buf_size = 0;;

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
	input_err(true, &client->dev, "%s: pdata->irq_gpio: %d, client->irq: %d\n", __func__, pdata->irq_gpio, client->irq);

	ts->client = client;
	ts->dev = &client->dev;
	ts->plat_data = pdata;
	ts->plat_data->bus_master = &client->controller->dev;
	ts->irq = client->irq;

#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
	ts->plat_data->stui_tsp_enter = synaptics_stui_tsp_enter;
	ts->plat_data->stui_tsp_exit = synaptics_stui_tsp_exit;
	ts->plat_data->stui_tsp_type = synaptics_stui_tsp_type;
#endif

	mutex_init(&ts->transfer_mutex);

	ts->synaptics_ts_write_data = synaptics_ts_spi_write;
	ts->synaptics_ts_read_data = synaptics_ts_spi_read;
	ts->synaptics_ts_read_data_only = synaptics_ts_spi_only_read;
	ts->synaptics_ts_get_baredata = synaptics_ts_spi_get_baredata;

	if (!ts->synaptics_ts_write_data || !ts->synaptics_ts_read_data) {
		input_err(true, &client->dev, "%s: not valid r/w operations\n", __func__);
		ret = -ENOMEM;
		goto error_allocate_mem;
	}

	input_err(true, &client->dev, "%s: cs gpio: %d, value: %d\n", __func__, ts->plat_data->gpio_spi_cs, gpio_get_value(ts->plat_data->gpio_spi_cs));

	ts->tdata = tdata;
	if (!ts->tdata) {
		ret = -ENOMEM;
		goto error_allocate_mem;
	}

	ts->max_rd_size = RD_CHUNK_SIZE_SPI;
	ts->max_wr_size = WR_CHUNK_SIZE_SPI;

	ret = synaptics_ts_init(ts);
	if (ret < 0) {
		input_err(true, &client->dev, "%s: fail to do ts init\n", __func__);
		goto err_init;
	}

	spi_set_drvdata(client, ts);

	ts->spi_mode = 0;

	/* set up spi driver */
	switch (ts->spi_mode) {
	case 0:
		client->mode = SPI_MODE_0;
		break;
	case 1:
		client->mode = SPI_MODE_1;
		break;
	case 2:
		client->mode = SPI_MODE_2;
		break;
	case 3:
		client->mode = SPI_MODE_3;
		break;
	}

	client->bits_per_word = 8;
	ret = spi_setup(client);
	if (ret < 0) {
		input_err(true, &client->dev, "%s: fail to set up SPI protocol driver\n", __func__);
		goto err_init;
	}

	return 0;

err_init:
error_allocate_mem:
error_allocate_pdata:
	input_err(true, &client->dev, "%s: failed(%d)\n", __func__, ret);
	input_log_fix();
	return ret;
}

static int synaptics_ts_spi_probe(struct spi_device *client)
{
	struct synaptics_ts_data *ts;
	int ret = 0;

	input_info(true, &client->dev, "%s\n", __func__);

	if (client->master->flags & SPI_MASTER_HALF_DUPLEX) {
		input_err(true, &client->dev, "%s: Full duplex not supported by host\n", __func__);
		return -EIO;
	}

	ret = synaptics_ts_spi_init(client);
	if (ret < 0) {
		input_err(true, &client->dev, "%s: fail to init resource\n", __func__);
		return ret;
	}

	ts = spi_get_drvdata(client);
	if (!ts->plat_data->work_queue_probe_enabled)
		return synaptics_ts_probe(ts->dev);

	queue_work(ts->plat_data->probe_workqueue, &ts->plat_data->probe_work);

	return 0;
}

static int synaptics_ts_dev_remove(struct spi_device *client)
{
	struct synaptics_ts_data *ts = spi_get_drvdata(client);

	synaptics_ts_remove(ts);

	if (rx_buf) {
		kfree((void *)rx_buf);
		rx_buf = NULL;
	}

	if (tx_buf) {
		kfree((void *)tx_buf);
		tx_buf = NULL;
	}

	if (xfer) {
		kfree((void *)xfer);
		xfer = NULL;
	}

	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
static void synaptics_ts_spi_remove(struct spi_device *client)
{
	synaptics_ts_dev_remove(client);
}
#else
static int synaptics_ts_spi_remove(struct spi_device *client)
{
	synaptics_ts_dev_remove(client);

	return 0;
}
#endif

static void synaptics_ts_spi_shutdown(struct spi_device *client)
{
	struct synaptics_ts_data *ts = spi_get_drvdata(client);

	synaptics_ts_shutdown(ts);
}

static const struct spi_device_id synaptics_ts_id[] = {
	{ SYNAPTICS_TS_NAME, 0 },
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

static struct spi_driver synaptics_ts_driver = {
	.probe		= synaptics_ts_spi_probe,
	.remove		= synaptics_ts_spi_remove,
	.shutdown	= synaptics_ts_spi_shutdown,
	.id_table	= synaptics_ts_id,
	.driver = {
		.owner	= THIS_MODULE,
		.name	= SYNAPTICS_TS_NAME,
#if IS_ENABLED(CONFIG_OF)
		.of_match_table = synaptics_ts_match_table,
#endif
#if IS_ENABLED(CONFIG_PM)
		.pm = &synaptics_ts_dev_pm_ops,
#endif
	},
};

module_spi_driver(synaptics_ts_driver);

MODULE_SOFTDEP("pre: acpm-mfd-bus");
MODULE_DESCRIPTION("synaptics TouchScreen driver");
MODULE_LICENSE("GPL");
