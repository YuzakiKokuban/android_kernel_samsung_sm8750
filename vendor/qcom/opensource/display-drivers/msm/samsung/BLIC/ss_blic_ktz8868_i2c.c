/*
 * ktz8868_hw_i2c.c - Platform data for ktz8868 backlight driver
 *
 * Copyright (C) 2024 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <linux/of_gpio.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/device.h>

#include "ss_blic_ktz8868_i2c.h"
__visible_for_testing struct ss_blic_ktz8868_info blic_pinfo;

static int ss_backlight_i2c_read(struct i2c_client *client, u8 addr,  u8 *value)
{
	int retry = 3;
	u8 wr[] = {addr};
	int ret = -1;

	/* Check if BLIC is probed or not, to prevent KP */
	if (!client) {
		pr_err("[SDE] %s: No blic, not probed.\n", __func__);
	} else {
		struct i2c_msg msg[] = {
			{
				.addr = client->addr,
				.flags = 0,
				.buf = wr,
				.len = 1
			}, {
				.addr = client->addr,
				.flags = I2C_M_RD,
				.buf = value,
				.len = 1
			},
		};

		do {
			ret = i2c_transfer(client->adapter, msg, 2);
			if (ret != 2)
				pr_err("[SDE] %s: client->addr 0x%02x read_addr 0x%02x error (ret == %d)\n",
						__func__, client->addr, addr, ret);
			else
				break;
		} while (--retry);
	}
	return ret;
}

static int ss_backlight_i2c_write(struct i2c_client *client, u8 addr,  u8 value)
{
	int retry = 3;
	u8 wr[] = {addr, value};
	int ret = -1;

	/* Check if BLIC is probed or not, to prevent KP */
	if (!client) {
		pr_err("[SDE] %s: No blic, not probed.\n", __func__);
	} else {
		struct i2c_msg msg[] = {
			{
				.addr = client->addr,
				.flags = 0,
				.buf = wr,
				.len = 2
			}
		};

		do {
			ret = i2c_transfer(client->adapter, msg, 1);
			if (ret != 1) {
				pr_err("[SDE] %s: addr 0x%02x value 0x%02x error (ret == %d)\n", __func__,
					 addr, value, ret);
			} else
				break;
		} while (--retry);
	}
	return ret;
}

int ss_blic_ktz8868_control(bool enable)
{
	u8 check;
	u8 (*data)[BLIC_MAX];
	int size;
	int ret = 0;
	int i;

	if (enable) {
		size = ARRAY_SIZE(ktz8868_en);
		data = ktz8868_en;
	} else {
		size = ARRAY_SIZE(ktz8868_dis);
		data = ktz8868_dis;
	}

	if (!blic_pinfo.client) {
		pr_err("[SDE] %s: i2c is not prepared\n", __func__);
		return -ENODEV;
	}

	for (i = 0; i < size; i++) {
		ss_backlight_i2c_write(blic_pinfo.client, data[i][BLIC_ADDR], data[i][BLIC_VAL]);
		ss_backlight_i2c_read(blic_pinfo.client, data[i][BLIC_ADDR], &check);

		if (check != data[i][BLIC_VAL]) {
			pr_err("[SDE] %s: Config failed: add: 0x%02x, write=0x%02x, read=0x%02x\n",
					__func__, data[i][BLIC_ADDR], data[i][BLIC_VAL], check);
			ret = -EINVAL;
		}
	}

	pr_err("[SDE] %s: Config done\n", __func__);

	return ret;
}

static int ss_blic_ktz8868_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	pr_info("[SDE] %s++\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		pr_err("[SDE] %s: i2c check fail\n", __func__);
		return -EIO;
	}

	blic_pinfo.client = client;
	i2c_set_clientdata(client, &blic_pinfo);

	pr_info("[SDE] %s--\n", __func__);

	return 0;
}

static void ss_blic_ktz8868_remove(struct i2c_client *client)
{
	return;
}

static const struct i2c_device_id ss_blic_ktz8868_id[] = {
	{"ktz8868", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, ss_blic_ktz8868_id);

static struct of_device_id ss_blic_ktz8868_match_table[] = {
	{ .compatible = "ktz8868,display_backlight",},
	{ }
};

struct i2c_driver ss_blic_ktz8868_driver = {
	.probe = ss_blic_ktz8868_probe,
	.remove = ss_blic_ktz8868_remove,
	.driver = {
		.name = "ktz8868",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(ss_blic_ktz8868_match_table),
#endif
		   },
	.id_table = ss_blic_ktz8868_id,
};

int ss_blic_ktz8868_init(void)
{

	int ret = 0;

	ret = i2c_add_driver(&ss_blic_ktz8868_driver);
	if (ret)
		pr_err("[SDE] %s: blic registration failed, ret = %d\n", __func__, ret);
	else
		pr_info("[SDE] %s blic registered\n", __func__);

	return ret;
}

void ss_blic_ktz8868_exit(void)
{
	i2c_del_driver(&ss_blic_ktz8868_driver);
}
