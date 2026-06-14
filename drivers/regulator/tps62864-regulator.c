// SPDX-License-Identifier: GPL-2.0-only
// Copyright Axis Communications AB

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/driver.h>

#include <dt-bindings/regulator/ti,tps62864.h>
#include <linux/of_gpio.h>
#include <linux/version.h>
#include <linux/delay.h>

#if IS_ENABLED(CONFIG_REGULATOR_DEBUG_CONTROL)
#include <linux/regulator/debug-regulator.h>
#endif

#define TPS62864_VOUT1		0x01
#define TPS62864_VOUT1_VO1_SET	GENMASK(7, 0)

#define TPS62864_CONTROL	0x03
#define TPS62864_CONTROL_FPWM	BIT(4)
#define TPS62864_CONTROL_SWEN	BIT(5)

#define TPS62864_MIN_MV		400
#define TPS62864_MAX_MV		1675
#define TPS62864_STEP_MV	5

static const struct regmap_config tps62864_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int tps62864_set_mode(struct regulator_dev *rdev, unsigned int mode)
{
	unsigned int val;

	switch (mode) {
	case REGULATOR_MODE_NORMAL:
		val = 0;
		break;
	case REGULATOR_MODE_FAST:
		val = TPS62864_CONTROL_FPWM;
		break;
	default:
		return -EINVAL;
	}

	return regmap_update_bits(rdev->regmap, TPS62864_CONTROL,
				  TPS62864_CONTROL_FPWM, val);
}

static unsigned int tps62864_get_mode(struct regulator_dev *rdev)
{
	unsigned int val;
	int ret;

	ret = regmap_read(rdev->regmap, TPS62864_CONTROL, &val);
	if (ret < 0)
		return 0;

	return (val & TPS62864_CONTROL_FPWM) ? REGULATOR_MODE_FAST : REGULATOR_MODE_NORMAL;
}

static const struct regulator_ops tps62864_regulator_ops = {
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.set_mode = tps62864_set_mode,
	.get_mode = tps62864_get_mode,
	.is_enabled = regulator_is_enabled_regmap,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.list_voltage = regulator_list_voltage_linear,
};

static unsigned int tps62864_of_map_mode(unsigned int mode)
{
	switch (mode) {
	case TPS62864_MODE_NORMAL:
		return REGULATOR_MODE_NORMAL;
	case TPS62864_MODE_FPWM:
		return REGULATOR_MODE_FAST;
	default:
		return REGULATOR_MODE_INVALID;
	}
}

static const struct regulator_desc tps62864_reg = {
	.name = "tps62864",
	.of_match = "SW",
	.owner = THIS_MODULE,
	.ops = &tps62864_regulator_ops,
	.of_map_mode = tps62864_of_map_mode,
	.regulators_node = "regulators",
	.type = REGULATOR_VOLTAGE,
	.n_voltages = ((TPS62864_MAX_MV - TPS62864_MIN_MV) / TPS62864_STEP_MV) + 1,
	.min_uV = TPS62864_MIN_MV * 1000,
	.uV_step = TPS62864_STEP_MV * 1000,
	.vsel_reg = TPS62864_VOUT1,
	.vsel_mask = TPS62864_VOUT1_VO1_SET,
	.enable_reg = TPS62864_CONTROL,
	.enable_mask = TPS62864_CONTROL_SWEN,
	.ramp_delay = 1000,
	/* tDelay + tRamp, rounded up */
	.enable_time = 3000,
};

static const struct of_device_id tps62864_dt_ids[] = {
	{ .compatible = "ti,tps62864", },
	{ }
};
MODULE_DEVICE_TABLE(of, tps62864_dt_ids);

static int tps62864_en_gpio(struct device *dev)
{
	int16_t gpio_array_size = 0;
	int32_t i = 0;
	uint16_t gpio_pin = 0;
	int ret = 0;

#if KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE
	gpio_array_size = of_count_phandle_with_args(
		dev->of_node, "gpios", "#gpio-cells");
#else
	gpio_array_size = of_gpio_count(dev->of_node);
#endif

	if (gpio_array_size > 0) {
		for (i = 0; i < gpio_array_size; i++) {
#if KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE
			gpio_pin = of_get_named_gpio(dev->of_node, "gpios", i);
#else
			gpio_pin = of_get_gpio(dev->of_node, i);
#endif
			ret = gpio_request(gpio_pin, NULL);
			if(ret) {
				pr_err("Failed to request gpio pin %u\n",gpio_pin);
			}
			gpio_direction_output(gpio_pin, 1);
			pr_info("request and direction gpio %u\n", gpio_pin);

			gpio_set_value_cansleep(gpio_pin, 1);
			pr_info("enable gpio %u\n", gpio_pin);
			usleep_range(3000, 3100);
		}
	}

	return 0;

}

static int tps62864_i2c_probe(struct i2c_client *i2c)
{
	struct device *dev = &i2c->dev;
	struct regulator_config config = {};
	struct regulator_dev *rdev;
	struct regmap *regmap;
#if IS_ENABLED(CONFIG_REGULATOR_DEBUG_CONTROL)
	int ret = 0;
#endif

	pr_info("tps62864_i2c_probe E\n");

	regmap = devm_regmap_init_i2c(i2c, &tps62864_regmap_config);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	config.dev = &i2c->dev;
	config.of_node = dev->of_node;
	config.regmap = regmap;

	tps62864_en_gpio(dev);

	rdev = devm_regulator_register(&i2c->dev, &tps62864_reg, &config);
	if (IS_ERR(rdev)) {
		dev_err(&i2c->dev, "Failed to register tps62864 regulator\n");
		return PTR_ERR(rdev);
	}

	// default off
	regulator_disable_regmap(rdev);

#if IS_ENABLED(CONFIG_REGULATOR_DEBUG_CONTROL)
	ret = devm_regulator_debug_register(dev, rdev);
	if (ret)
		dev_err(&i2c->dev, "Failed to register debug regulator\n");
#endif

	pr_info("tps62864_i2c_probe X\n");
	return 0;
}

static const struct i2c_device_id tps62864_i2c_id[] = {
	{ "tps62864", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, tps62864_i2c_id);

static struct i2c_driver tps62864_regulator_driver = {
	.driver = {
		.name = "tps62864",
		.of_match_table = tps62864_dt_ids,
	},
	.probe = tps62864_i2c_probe,
	.id_table = tps62864_i2c_id,
};

module_i2c_driver(tps62864_regulator_driver);

MODULE_LICENSE("GPL v2");
