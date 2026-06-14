/*
 * Copyright (C) 2020 Samsung Electronics. All rights reserved.
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
 * along with this program;
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/ioctl.h>
#include <linux/gpio.h>
#include <linux/version.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#if defined(CONFIG_SEC_SNVM_PLATFORM_DRV)
#include <linux/platform_device.h>
#endif

#include "sec_star.h"

#undef USE_INTERNAL_PULLUP //if use external pull-up, not necessary

#define ERR(msg...)		pr_err("[star-k250a] : " msg)
#define INFO(msg...)	pr_info("[star-k250a] : " msg)

struct k250a_dev {
	struct i2c_client *client;
	struct regulator *vdd;
	unsigned int reset_gpio;
	struct pinctrl *pinctrl;
	struct pinctrl_state *nvm_on_pin;
	struct pinctrl_state *nvm_off_pin;
#if defined(USE_INTERNAL_PULLUP)
#define SCL_GPIO_NUM	335
#define SDA_GPIO_NUM	320
	struct pinctrl *pinctrl;
	struct pinctrl_state *pin_i2c;
	struct pinctrl_state *pin_gpio;
#endif
	sec_star_t *star;
#ifdef CONFIG_SEC_SNVM_I2C_CLOCK_CONTROL
	struct clk *i2c_main_clk;
	struct clk *i2c_dma_clk;
	bool i2c_main_clk_enabled;
	bool i2c_dma_clk_enabled;
#endif
};

static struct k250a_dev g_k250a;

#ifdef CONFIG_SEC_SNVM_I2C_CLOCK_CONTROL
static void k250a_parse_i2c_clock(struct k250a_dev *k250a, struct device_node *np)
{
	struct device_node *i2c_np;

	i2c_np = of_parse_phandle(np, "i2c_node", 0);
	if (!i2c_np) {
		INFO("i2c_node not found\n");
		return;
	}

	INFO("i2c_node found\n");
	k250a->i2c_main_clk = of_clk_get_by_name(i2c_np, "main");

	if (IS_ERR(k250a->i2c_main_clk))
		INFO("failed to get i2c main clock\n");
	else
		INFO("i2c main clock found");

	k250a->i2c_dma_clk = of_clk_get_by_name(i2c_np, "dma");
	if (IS_ERR(k250a->i2c_dma_clk))
		INFO("failed to get i2c dma clock\n");
	else
		INFO("i2c dma clock found");
}

static void k250a_i2c_clock_enable(void)
{
	int ret = 0;

	if (!g_k250a.i2c_main_clk_enabled && !IS_ERR_OR_NULL(g_k250a.i2c_main_clk)) {
		ret = clk_prepare_enable(g_k250a.i2c_main_clk);
		if (ret)
			ERR("failed to enable i2c main clock\n");
		else
			g_k250a.i2c_main_clk_enabled = true;
	}

	if (!g_k250a.i2c_dma_clk_enabled && !IS_ERR_OR_NULL(g_k250a.i2c_dma_clk)) {
		ret = clk_prepare_enable(g_k250a.i2c_dma_clk);
		if (ret)
			ERR("failed to enable i2c dma clock\n");
		else
			g_k250a.i2c_dma_clk_enabled = true;
	}
}

static void k250a_i2c_clock_disable(void)
{
	if (g_k250a.i2c_main_clk_enabled && !IS_ERR_OR_NULL(g_k250a.i2c_main_clk)) {
		clk_disable_unprepare(g_k250a.i2c_main_clk);
		g_k250a.i2c_main_clk_enabled = false;
	}

	if (g_k250a.i2c_dma_clk_enabled && !IS_ERR_OR_NULL(g_k250a.i2c_dma_clk)) {
		clk_disable_unprepare(g_k250a.i2c_dma_clk);
		g_k250a.i2c_dma_clk_enabled = false;
	}
}
#endif

static int k250a_poweron(void)
{
	int ret = 0;
#ifdef CONFIG_SEC_SNVM_I2C_CLOCK_CONTROL
	bool i2c_main_clk = !IS_ERR_OR_NULL(g_k250a.i2c_main_clk);
	bool i2c_dma_clk = !IS_ERR_OR_NULL(g_k250a.i2c_dma_clk);

	INFO("k250a_poweron (i2c clk:%s%s)\n",
		i2c_main_clk ? " main" : "", i2c_dma_clk ? " dma" : "");
#else
	INFO("k250a_poweron\n");
#endif
	if (g_k250a.vdd == NULL) {
		if (g_k250a.reset_gpio == 0) {
			return 0;
		}

		INFO("rest pin control instead of vdd\n");

		gpio_set_value(g_k250a.reset_gpio, 0);
		usleep_range(1000, 2000);
		gpio_set_value(g_k250a.reset_gpio, 1);

		msleep(20);
		return 0;
	}

	ret = regulator_enable(g_k250a.vdd);
	if (ret) {
		ERR("%s - enable vdd failed, ret=%d\n", __func__, ret);
		return ret;
	}

	usleep_range(1000, 2000);
	if (g_k250a.nvm_on_pin) {
		if (pinctrl_select_state(g_k250a.pinctrl, g_k250a.nvm_on_pin))
			ERR("nvm on pinctrl set error\n");
		else
			INFO("nvm on pinctrl set\n");
	}

#if defined(USE_INTERNAL_PULLUP)
	if (pinctrl_select_state(g_k250a.pinctrl, g_k250a.pin_i2c) < 0) {
		ERR("failed to pinctrl_select_state for gpio");
	}
#endif

	msleep(20);

	return 0;
}

static int k250a_poweroff(void)
{
	int ret = 0;

	INFO("k250a_poweroff\n");

	if (g_k250a.vdd == NULL) {
		return 0;
	}
	if (g_k250a.nvm_off_pin) {
		if (pinctrl_select_state(g_k250a.pinctrl, g_k250a.nvm_off_pin))
			ERR("nvm off pinctrl set error\n");
		else
			INFO("nvm off pinctrl set\n");
	}

#if defined(USE_INTERNAL_PULLUP)
	if (pinctrl_select_state(g_k250a.pinctrl, g_k250a.pin_gpio) < 0) {
		ERR("failed to pinctrl_select_state for gpio");
	}
#endif

	ret = regulator_disable(g_k250a.vdd);
	if (ret) {
		ERR("%s - disable vdd failed, ret=%d\n", __func__, ret);
		return ret;
	}

	usleep_range(15000, 20000);
	return 0;
}

static int k250a_reset(void)
{
	if (g_k250a.reset_gpio == 0) {
		return -ENODEV;
	}

	gpio_set_value(g_k250a.reset_gpio, 0);
	usleep_range(1000, 2000);
	gpio_set_value(g_k250a.reset_gpio, 1);
	usleep_range(15000, 20000);
	return 0;
}

static star_dev_t star_dev = {
	.name = "k250a",
	.hal_type = SEC_HAL_I2C,
	.client = NULL,
	.power_on = k250a_poweron,
	.power_off = k250a_poweroff,
	.reset = k250a_reset
};

#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
static int k250a_probe(struct i2c_client *client)
#else
static int k250a_probe(struct i2c_client *client, const struct i2c_device_id *id)
#endif
{
	struct device_node *np = client->dev.of_node;

	INFO("Entry : %s\n", __func__);

	if (np) {
		g_k250a.vdd = devm_regulator_get_optional(&(client->dev), "1p8_pvdd");
		if (IS_ERR(g_k250a.vdd)) {
			ERR("%s - 1p8_pvdd can not be used\n", __func__);
			g_k250a.vdd = NULL;
		}

		if (of_property_read_u32(np, "reset_gpio", &(g_k250a.reset_gpio)) < 0) {
			ERR("%s - Reset Control can not be used\n", __func__);
			g_k250a.reset_gpio = 0;
		} else {
			if (gpio_request(g_k250a.reset_gpio, "sec-reset") < 0) {
				ERR("%s - Failed to request sec-reset gpio\n", __func__);
				g_k250a.reset_gpio = 0;
			} else {
				INFO("%s - Reset GPIO Num : %u\n", __func__, g_k250a.reset_gpio);
				if (gpio_direction_output(g_k250a.reset_gpio, 1) < 0) {
					ERR("Failed to set reset gpio");
				}
			}
		}
	} else {
		return -ENODEV;
	}

	g_k250a.pinctrl = devm_pinctrl_get(client->adapter->dev.parent);
	if (IS_ERR(g_k250a.pinctrl)) {
		ERR("devm_pinctrl_get failed\n");
		return -1;
	}
	g_k250a.nvm_on_pin = pinctrl_lookup_state(g_k250a.pinctrl, "nvm_on");
	if (IS_ERR(g_k250a.nvm_on_pin)) {
		ERR("pinctrl_lookup_state failed-default\n");
		g_k250a.nvm_on_pin = NULL;
	}
	g_k250a.nvm_off_pin = pinctrl_lookup_state(g_k250a.pinctrl, "nvm_off");
	if (IS_ERR(g_k250a.nvm_off_pin)) {
		ERR("pinctrl_lookup_state failed-nvm_off\n");
		g_k250a.nvm_off_pin = NULL;
	} else if (pinctrl_select_state(g_k250a.pinctrl, g_k250a.nvm_off_pin)) {
		ERR("nvm off pinctrl set error\n");
	} else {
		INFO("nvm off pinctrl set\n");
	}

#if defined(USE_INTERNAL_PULLUP)
	g_k250a.pinctrl = devm_pinctrl_get(client->adapter->dev.parent);
	if (IS_ERR(g_k250a.pinctrl)) {
		ERR("failed to devm_pinctrl_get");
		return -1;
	}

	g_k250a.pin_i2c = pinctrl_lookup_state(g_k250a.pinctrl, "default");
	if (IS_ERR(g_k250a.pin_i2c)) {
		ERR("failed to pinctrl_lookup_state for i2c");
		devm_pinctrl_put(g_k250a.pinctrl);
		return -1;
	}

	g_k250a.pin_gpio = pinctrl_lookup_state(g_k250a.pinctrl, "gpio");
	if (IS_ERR(g_k250a.pin_gpio)) {
		ERR("failed to pinctrl_lookup_state for gpio");
		devm_pinctrl_put(g_k250a.pinctrl);
		return -1;
	}

	if (pinctrl_select_state(g_k250a.pinctrl, g_k250a.pin_gpio) < 0) {
		ERR("failed to pinctrl_select_state for gpio");
		devm_pinctrl_put(g_k250a.pinctrl);
		return -1;
	}

	if (gpio_request(SCL_GPIO_NUM, "sec-scl") < 0) {
		ERR("failed to get scl gpio");
		devm_pinctrl_put(g_k250a.pinctrl);
		return -1;
	}

	if (gpio_request(SDA_GPIO_NUM, "sec-sda") < 0) {
		ERR("failed to get sda gpio");
		devm_pinctrl_put(g_k250a.pinctrl);
		return -1;
	}

	if (gpio_direction_output(SCL_GPIO_NUM, 0) < 0) {
		ERR("failed to set scl gpio");
		devm_pinctrl_put(g_k250a.pinctrl);
		return -1;
	}

	if (gpio_direction_output(SDA_GPIO_NUM, 0) < 0) {
		ERR("failed to set sda gpio");
		devm_pinctrl_put(g_k250a.pinctrl);
		return -1;
	}
#endif

	g_k250a.client = client;
	star_dev.client = client;
	g_k250a.star = star_open(&star_dev);
	if (g_k250a.star == NULL) {
		return -ENODEV;
	}

	INFO("Exit : %s\n", __func__);
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int k250a_remove(struct i2c_client *client)
#else
static void k250a_remove(struct i2c_client *client)
#endif
{
	INFO("Entry : %s\n", __func__);
#if defined(USE_INTERNAL_PULLUP)
	devm_pinctrl_put(g_k250a.pinctrl);
	gpio_free(SCL_GPIO_NUM);
	gpio_free(SDA_GPIO_NUM);
#endif
	if (g_k250a.reset_gpio != 0) {
		gpio_free(g_k250a.reset_gpio);
	}
	star_close(g_k250a.star);
	INFO("Exit : %s\n", __func__);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	return 0;
#endif
}

#if defined(CONFIG_SEC_SNVM_PLATFORM_DRV)
static int k250a_dev_open(struct inode *inode, struct file *filp)
{
	k250a_poweron();
#ifdef CONFIG_SEC_SNVM_I2C_CLOCK_CONTROL
	k250a_i2c_clock_enable();
#endif

	return 0;
}

static int ese_dev_release(struct inode *inode, struct file *filp)
{
#ifdef CONFIG_SEC_SNVM_I2C_CLOCK_CONTROL
	k250a_i2c_clock_disable();
#endif
	k250a_poweroff();

	return 0;
}

static const struct file_operations k250a_dev_fops = {
	.owner = THIS_MODULE,
	.open = k250a_dev_open,
	.release = ese_dev_release,
};

static struct miscdevice k250a_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "k250a",
	.fops = &k250a_dev_fops,
};

void k250a_parse_dt_for_platform_device(struct device *dev)
{
	g_k250a.vdd = devm_regulator_get_optional(dev, "1p8_pvdd");
	if (IS_ERR(g_k250a.vdd)) {
		ERR("%s - 1p8_pvdd can not be used\n", __func__);
		g_k250a.vdd = NULL;
	} else {
		INFO("%s: regulator_get success\n", __func__);
	}

#ifdef CONFIG_SEC_SNVM_I2C_CLOCK_CONTROL
	k250a_parse_i2c_clock(&g_k250a, dev->of_node);
#endif
}

static int k250a_platform_probe(struct platform_device *pdev)
{
	int ret = -1;

	k250a_parse_dt_for_platform_device(&pdev->dev);
	ret = misc_register(&k250a_misc_device);
	if (ret < 0)
		ERR("misc_register failed! %d\n", ret);

	INFO("%s: finished...\n", __func__);
	return 0;
}

static int k250a_platform_remove(struct platform_device *pdev)
{
	INFO("Entry : %s\n", __func__);
	return 0;
}

static const struct of_device_id k250a_secure_match_table[] = {
	{ .compatible = "sec_k250a_platform",},
	{},
};

static struct platform_driver k250a_platform_driver = {
	.driver = {
		.name = "k250a_platform",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = k250a_secure_match_table,
#endif
	},
	.probe =  k250a_platform_probe,
	.remove = k250a_platform_remove,
};
#endif

static const struct i2c_device_id k250a_id[] = {
	{"k250a", 0},
	{}
};

static const struct of_device_id k250a_match_table[] = {
	{ .compatible = "sec_k250a",},
	{},
};

static struct i2c_driver k250a_driver = {
	.id_table = k250a_id,
	.probe = k250a_probe,
	.remove = k250a_remove,
	.driver = {
		.name = "k250a",
		.owner = THIS_MODULE,
		.of_match_table = k250a_match_table,
	},
};

static int __init k250a_init(void)
{
#if defined(CONFIG_SEC_SNVM_PLATFORM_DRV)
	int ret;

	ret = platform_driver_register(&k250a_platform_driver);
	if (!ret) {
		INFO("platform_driver_register success : %s\n", __func__);
		return ret;
	} else {
		ERR("platform_driver_register fail : %s\n", __func__);
		return ret;
	}
#endif
	INFO("Entry : %s\n", __func__);

	return i2c_add_driver(&k250a_driver);
}
module_init(k250a_init);

static void __exit k250a_exit(void)
{
	INFO("Entry : %s\n", __func__);
#if defined(CONFIG_SEC_SNVM_PLATFORM_DRV)
	platform_driver_unregister(&k250a_platform_driver);
	return;
#endif
	i2c_del_driver(&k250a_driver);
}

module_exit(k250a_exit);

MODULE_AUTHOR("Sec");
MODULE_DESCRIPTION("K250A driver");
MODULE_LICENSE("GPL");
