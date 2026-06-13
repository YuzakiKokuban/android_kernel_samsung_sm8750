/*
 * max77816_i2c.c - Platform data for max77816 buck booster hw i2c driver
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

#include "ss_boost_max77816_i2c.h"
__visible_for_testing struct ss_boost_max77816_info boost_pinfo;

static int ss_boost_i2c_read(struct i2c_client *client, u8 addr,  u8 *value)
{
	int retry = 3;
	u8 wr[] = {addr};
	int ret = -1;

	/* Check if BOOST IC probed or not, to prevent KP */
	if (!client) {
		pr_err("[SDE] %s: No boost ic, not probed.\n", __func__);
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

static int ss_boost_i2c_write(struct i2c_client *client, u8 addr,  u8 value)
{
	int retry = 3;
	u8 wr[] = {addr, value};
	int ret = -1;

	/* Check if BOOST IC probed or not, to prevent KP */
	if (!client) {
		pr_err("[SDE] %s : No boost ic, not probed.\n", __func__);
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

int ss_boost_max77816_control(bool enable)
{
	u8 check;
	u8 (*data)[BOOSTIC_MAX];
	int size;
	int ret = 0;
	int i;

	if (enable) {
		size = ARRAY_SIZE(max77816_en);
		data = max77816_en;
	} else {
		size = ARRAY_SIZE(max77816_dis);
		data = max77816_dis;
	}

	if (!boost_pinfo.client) {
		pr_err("[SDE] %s: i2c is not prepared\n", __func__);
		return -ENODEV;
	}

	for (i = 0; i < size; i++) {
		ss_boost_i2c_write(boost_pinfo.client, data[i][BOOSTIC_ADDR], data[i][BOOSTIC_VAL]);
		ss_boost_i2c_read(boost_pinfo.client, data[i][BOOSTIC_ADDR], &check);

		if (check != data[i][BOOSTIC_VAL]) {
			pr_err("[SDE] %s: Config failed: add: 0x%02x, write=0x%02x, read=0x%02x\n",
					__func__, data[i][BOOSTIC_ADDR], data[i][BOOSTIC_VAL], check);
			ret = -EINVAL;
		}
	}

	pr_err("[SDE] %s: Config done\n", __func__);

	return ret;
}

#if KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE
static int ss_boost_max77816_probe(struct i2c_client *client)
#else
static int ss_boost_max77816_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
#endif
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	pr_info("[SDE] %s++\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		pr_err("[SDE] %s: i2c check fail\n", __func__);
		return -EIO;
	}

	boost_pinfo.client = client;
	i2c_set_clientdata(client, &boost_pinfo);

	pr_info("[SDE] %s--\n", __func__);

	return 0;
}

static void ss_boost_max77816_remove(struct i2c_client *client)
{
	return;
}

static const struct i2c_device_id ss_boost_max77816_id[] = {
	{"max77816", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, ss_boost_max77816_id);

static const struct of_device_id ss_boost_max77816_match_table[] = {
	{ .compatible = "max77816,display_boost",},
	{ }
};

struct i2c_driver ss_boost_max77816_driver = {
	.probe = ss_boost_max77816_probe,
	.remove = ss_boost_max77816_remove,
	.driver = {
		.name = "max77816",
		.owner = THIS_MODULE,
#if IS_ENABLED(CONFIG_OF)
		.of_match_table = of_match_ptr(ss_boost_max77816_match_table),
#endif
		   },
	.id_table = ss_boost_max77816_id,
};

int ss_boost_max77816_init(void)
{

	int ret = 0;

	ret = i2c_add_driver(&ss_boost_max77816_driver);
	if (ret)
		pr_err("[SDE] %s: boost ic registration failed, ret = %d\n", __func__, ret);
	else
		pr_info("[SDE] %s boost ic registered\n", __func__);

	return ret;
}

void ss_boost_max77816_exit(void)
{
	i2c_del_driver(&ss_boost_max77816_driver);
}
