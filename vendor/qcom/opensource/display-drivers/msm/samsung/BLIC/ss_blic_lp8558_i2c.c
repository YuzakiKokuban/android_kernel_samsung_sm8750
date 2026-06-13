/*
 * lp8558_hw_i2c.c - Platform data for lp8558 backlight driver
 *
 * Copyright (C) 2020 Samsung Electronics
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

#include "ss_blic_lp8558_i2c.h"

static int ss_backlight_i2c_read(struct i2c_client *client, u8 addr,  u8 *value)
{
	int retry = 3;
	u8 wr[] = {addr};

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
	int ret;

	do {
		ret = i2c_transfer(client->adapter, msg, 2);
		if (ret != 2) {
			pr_err("%s : client->addr 0x%02x read_addr 0x%02x error (ret == %d)\n", __func__, client->addr, addr, ret);
		} else
			break;
	} while (--retry);

	return ret;
}

static int ss_backlight_i2c_write(struct i2c_client *client, u8 addr,  u8 value)
{
	int retry = 3;
	u8 wr[] = {addr, value};
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.buf = wr,
			.len = 2
		}
	};
	int ret;

	do {
		ret = i2c_transfer(client->adapter, msg, 1);
		if (ret != 1) {
			pr_err("%s : addr 0x%02x value 0x%02x error (ret == %d)\n", __func__,
				 addr, value, ret);
		} else
			break;
	} while (--retry);

	return ret;
}

int ss_blic_lp8558_control(struct samsung_display_driver_data *vdd)
{
	u8 buf[2];
	//u8 value = 0x02; //VN & VP 5.5V

	if (!backlight_pinfo.client) {
		pr_err("%s : error pinfo\n", __func__);
		return 0;
	}

	if (vdd->is_factory_mode && !ss_is_ub_connected(vdd)) {
		LCD_INFO(vdd, "Do not panel power on..\n");
		return 0;
	}

	/*
	{0x98, 0x1b},
	{0x9E, 0x21},
	{0xA0, 0x65},
	{0xA1, 0x6E},
	{0xA5, 0x04},
	{0xA6, 0x80},
	*/
	ss_backlight_i2c_write(backlight_pinfo.client, 0x98, 0x80);
	ss_backlight_i2c_write(backlight_pinfo.client, 0x9E, 0x22);
	ss_backlight_i2c_write(backlight_pinfo.client, 0xA0, 0x8E);
	ss_backlight_i2c_write(backlight_pinfo.client, 0xA1, 0x6E);
	ss_backlight_i2c_write(backlight_pinfo.client, 0xA9, 0xE0);

	ss_backlight_i2c_write(backlight_pinfo.client, 0xA5, 0x04);
	ss_backlight_i2c_write(backlight_pinfo.client, 0xA6, 0x80);


	ss_backlight_i2c_read(backlight_pinfo.client, 0xA0, &buf[0]);
	ss_backlight_i2c_read(backlight_pinfo.client, 0x9E, &buf[1]);

	LCD_INFO(vdd, "read values.. A0h:0x%x, 9Eh:0x%x\n", buf[0], buf[1]);

	return 0;
}

static int ss_blic_lp8558_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	pr_info("%s \n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		pr_err("%s i2c check fail\n", __func__);
		return -EIO;
	}

	backlight_pinfo.client = client;

	i2c_set_clientdata(client, &backlight_pinfo);

	return 0;
}

static int ss_blic_lp8558_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id ss_blic_lp8558_id[] = {
	{"lp8558", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, ss_blic_lp8558_id);

static struct of_device_id ss_blic_lp8558_match_table[] = {
	{ .compatible = "lp8558,display_backlight",},
	{ }
};

struct i2c_driver ss_blic_lp8558_driver = {
	.probe = ss_blic_lp8558_probe,
	.remove = ss_blic_lp8558_remove,
	.driver = {
		.name = "lp8558",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(ss_blic_lp8558_match_table),
#endif
		   },
	.id_table = ss_blic_lp8558_id,
};

int ss_blic_lp8558_init(void)
{

	int ret = 0;

	ret = i2c_add_driver(&ss_blic_lp8558_driver);
	if (ret) {
		pr_err("%s : lp8558_init registration failed. ret= %d\n", __func__, ret);
	}

	return ret;
}

void ss_blic_lp8558_exit(void)
{
	i2c_del_driver(&ss_blic_lp8558_driver);
}


