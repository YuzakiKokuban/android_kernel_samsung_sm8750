/*
 * isl98608_hw_i2c.c - Platform data for isl98608 buck driver
 *
 * Copyright (C) 2011 Samsung Electronics
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

#include "ss_buck_isl98609_i2c.h"

static int ss_buck_i2c_read(struct i2c_client *client, u8 addr,  u8 *value)
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
		if (ret != 2)
			pr_err("%s : client->addr 0x%02x read_addr 0x%02x error (ret == %d)\n", __func__, client->addr, addr, ret);
		else
			break;
	} while (--retry);

	return ret;
}

static int ss_buck_i2c_write(struct i2c_client *client, u8 addr,  u8 value)
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

int ss_buck_i2c_read_ex(u8 addr,  u8 *value)
{
	int retry = 3;
	u8 wr[] = {addr};
	int ret = -1;

	/* Check if it probed or not, to prevent KP */
	if (!buck_pinfo.client) {
		pr_err("%s : No isl98609 ic, not probed.\n", __func__);
	} else {
		struct i2c_msg msg[] = {
			{
				.addr = buck_pinfo.client->addr,
				.flags = 0,
				.buf = wr,
				.len = 1
			}, {
				.addr = buck_pinfo.client->addr,
				.flags = I2C_M_RD,
				.buf = value,
				.len = 1
			},
		};

		do {
			ret = i2c_transfer(buck_pinfo.client->adapter, msg, 2);
			if (ret != 2)
				pr_err("%s : client->addr 0x%02x read_addr 0x%02x error (ret == %d)\n", __func__,
					buck_pinfo.client->addr, addr, ret);
			else
				break;
		} while (--retry);
	}

	return ret;
}


/* return -1 if no isl98609 ic, not probed */
int ss_buck_i2c_write_ex(u8 addr,  u8 value)
{
	int retry = 3;
	u8 wr[] = {addr, value};
	int ret = -1;

	/* Check if it probed or not, to prevent KP */
	if (!buck_pinfo.client) {
		pr_err("%s : No isl98609 ic, not probed.\n", __func__);
	} else {
		struct i2c_msg msg[] = {
			{
				.addr = buck_pinfo.client->addr,
				.flags = 0,
				.buf = wr,
				.len = 2
			}
		};

		do {
			ret = i2c_transfer(buck_pinfo.client->adapter, msg, 1);
			if (ret != 1) {
				pr_err("%s : addr 0x%02x value 0x%02x error (ret == %d)\n", __func__,
					 addr, value, ret);
			} else
				break;
		} while (--retry);
	}
	return ret;
}

int ss_buck_isl98608_control(struct samsung_display_driver_data *vdd)
{
	u8 buf[2];
	u8 value = 0x02; //VN & VP 5.5V

	if (!buck_pinfo.client) {
		LCD_ERR(vdd, "error pinfo\n");
		return 0;
	}

	if (vdd->is_factory_mode && !ss_is_ub_connected(vdd)) {
		LCD_ERR(vdd, "Do not panel power on..\n");
		return 0;
	}

	ss_buck_i2c_write(buck_pinfo.client, 0x06, 0x0D);
	ss_buck_i2c_write(buck_pinfo.client, 0x08, value);
	ss_buck_i2c_write(buck_pinfo.client, 0x09, value);

	ss_buck_i2c_read(buck_pinfo.client, 0x08, &buf[0]);
	ss_buck_i2c_read(buck_pinfo.client, 0x09, &buf[1]);

	if ((value != buf[0]) || (value != buf[1]))
		LCD_INFO(vdd, "0x%x 0x%x fail\n", buf[0], buf[1]);
	else
		LCD_INFO(vdd, "0x%x 0x%x success\n", buf[0], buf[1]);

	return 0;
}

static int ss_buck_isl98608_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	pr_info("%s \n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		pr_err("%s i2c check fail\n", __func__);
		return -EIO;
	}

	buck_pinfo.client = client;

	i2c_set_clientdata(client, &buck_pinfo);

	return 0;
}

static void ss_buck_isl98608_remove(struct i2c_client *client)
{
	return;
}

static const struct i2c_device_id ss_buck_isl98608_id[] = {
	{"isl98608", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, ss_buck_isl98608_id);

static struct of_device_id ss_buck_isl98608_match_table[] = {
	{ .compatible = "isl98608,display_buck",},
	{ }
};

struct i2c_driver ss_buck_isl98608_driver = {
	.probe = ss_buck_isl98608_probe,
	.remove = ss_buck_isl98608_remove,
	.driver = {
		.name = "isl98608",
		.owner = THIS_MODULE,
#if IS_ENABLED(CONFIG_OF)
		.of_match_table = of_match_ptr(ss_buck_isl98608_match_table),
#endif
		   },
	.id_table = ss_buck_isl98608_id,
};

int ss_buck_isl98608_init(void)
{

	int ret = 0;

	ret = i2c_add_driver(&ss_buck_isl98608_driver);
	if (ret)
		pr_err("%s : ss_buck_isl98608_init registration failed. ret= %d\n", __func__, ret);

	return ret;
}

void ss_buck_isl98608_exit(void)
{
	i2c_del_driver(&ss_buck_isl98608_driver);
}

//module_init(ss_buck_isl98608_init);
//module_exit(ss_buck_isl98608_exit);

//MODULE_DESCRIPTION("ss_buck_isl98608_i2c driver");
//MODULE_LICENSE("GPL");
