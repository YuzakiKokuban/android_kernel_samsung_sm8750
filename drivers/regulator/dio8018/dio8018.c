// SPDX-License-Identifier: GPL-2.0
/*
 * dio8018.c - Regulator driver for the DIOO DIO8018
 *
 * Copyright (c) 2023 DIOO Microciurcuits Co., Ltd Jiangsu
 *
 */

#include <linux/device.h>
#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include <linux/of_gpio.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#if IS_ENABLED(CONFIG_REGULATOR_DEBUG_CONTROL)
#include <linux/regulator/debug-regulator.h>
#endif
#include <linux/regulator/dio8018.h>
#include <linux/regulator/of_regulator.h>

#if IS_ENABLED(CONFIG_SEC_KUNIT)
#include <kunit/test.h>
#include <kunit/mock.h>
#else
#ifndef EXPORT_SYMBOL_KUNIT
#define EXPORT_SYMBOL_KUNIT(sym)	/* nothing */
#endif
#ifndef __mockable
#define __mockable	/* nothing */
#endif
#ifndef __visible_for_testing
#define __visible_for_testing	static
#endif
#endif /* CONFIG_SEC_KUNIT */
#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)
#include <linux/regulator/pmic_class.h>
#else

//#define _SUPPORT_SYSFS_INTERFACE

#if defined(_SUPPORT_SYSFS_INTERFACE)
struct pmic_device_attribute {
	struct device_attribute dev_attr;
};

#define PMIC_ATTR(_name, _mode, _show, _store)	\
	{ .dev_attr = __ATTR(_name, _mode, _show, _store) }

static struct class *pmic_class;
static atomic_t pmic_dev;

#endif /*end of #if defined(_SUPPORT_SYSFS_INTERFACE)*/
#endif /*end of #if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)*/

struct dio8018_data {
	struct dio8018_dev *iodev;
	int num_regulators;
	struct regulator_dev *rdev[DIO8018_REGULATOR_MAX];

	struct workqueue_struct *wq;
	struct work_struct work;
	bool need_self_recovery;

#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC) || defined(_SUPPORT_SYSFS_INTERFACE)
	u8 read_addr;
	u8 read_val;
	struct device *dev;
#endif
};

#define DIO8018_LDO12_VOLTAGES 188
// 0x3E ~ 0xBB  VOUT = 0.800V + [(d − 99) x 8mV]
static const struct linear_range dio8018_ldo12_range[] = {
	REGULATOR_LINEAR_RANGE(504000, 0x3e, 0xbb, 8000),
};

#define DIO8018_LDO37_VOLTAGES 256
// 0x10 ~ 0xff   VOUT =1.500V + [(d − 16) x 8mV
static const struct linear_range dio8018_ldo37_range[] = {
	REGULATOR_LINEAR_RANGE(1500000, 0x10, 0xff, 8000),
};

#define DIO8018_LDO_CURRENT_COUNT	2
static const unsigned int dio8018_ldo_current[DIO8018_REGULATOR_MAX][DIO8018_LDO_CURRENT_COUNT] = {{ 1300000, 1800000 },
																								{ 1300000, 1800000 },
																								{ 450000, 650000 },
																								{ 450000, 650000 },
																								{ 650000, 950000 },
																								{ 450000, 650000 },
																								{ 650000, 950000 }};


int dio8018_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct dio8018_data *info = i2c_get_clientdata(i2c);
	struct dio8018_dev *dio8018 = info->iodev;
	int ret;

	mutex_lock(&dio8018->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&dio8018->i2c_lock);
	if (ret < 0) {
		pr_info("%s:%s reg(0x%02x), ret(%d)\n",
			 DIO8018_DEV_NAME, __func__, reg, ret);
		return ret;
	}

	ret &= 0xff;
	*dest = ret;
	return 0;
}
EXPORT_SYMBOL_GPL(dio8018_read_reg);

int dio8018_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct dio8018_data *info = i2c_get_clientdata(i2c);
	struct dio8018_dev *dio8018 = info->iodev;
	int ret;

	mutex_lock(&dio8018->i2c_lock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&dio8018->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(dio8018_bulk_read);

int dio8018_read_word(struct i2c_client *i2c, u8 reg)
{
	struct dio8018_data *info = i2c_get_clientdata(i2c);
	struct dio8018_dev *dio8018 = info->iodev;
	int ret;

	mutex_lock(&dio8018->i2c_lock);
	ret = i2c_smbus_read_word_data(i2c, reg);
	mutex_unlock(&dio8018->i2c_lock);
	if (ret < 0)
		return ret;

	return ret;
}
EXPORT_SYMBOL_GPL(dio8018_read_word);

int dio8018_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct dio8018_data *info = i2c_get_clientdata(i2c);
	struct dio8018_dev *dio8018 = info->iodev;
	int ret;

	mutex_lock(&dio8018->i2c_lock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	mutex_unlock(&dio8018->i2c_lock);
	if (ret < 0)
		pr_info("%s:%s reg(0x%02hhx), ret(%d)\n",
				DIO8018_DEV_NAME, __func__, reg, ret);

	return ret;
}
EXPORT_SYMBOL_GPL(dio8018_write_reg);

int dio8018_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct dio8018_data *info = i2c_get_clientdata(i2c);
	struct dio8018_dev *dio8018 = info->iodev;
	int ret;

	mutex_lock(&dio8018->i2c_lock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&dio8018->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(dio8018_bulk_write);

int dio8018_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct dio8018_data *info = i2c_get_clientdata(i2c);
	struct dio8018_dev *dio8018 = info->iodev;
	int ret;
	u8 old_val, new_val;

	mutex_lock(&dio8018->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		old_val = ret & 0xff;
		new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&dio8018->i2c_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(dio8018_update_reg);

static int dio8018_reset(struct i2c_client *i2c, struct dio8018_platform_data *pdata)
{

	int ret = 0;
	u8 val = 0;

	if (!i2c||!pdata)
		return ENXIO;

	if (pdata && pdata->need_hw_reset && pdata->pmic_reset_gpio >= 0) {
		gpio_request(pdata->pmic_reset_gpio, "dio8018_reset_gpio");
		gpio_direction_output(pdata->pmic_reset_gpio, 0x1);
		mdelay(10);
		gpio_set_value(pdata->pmic_reset_gpio, 0x0);
		mdelay(100);
		gpio_set_value(pdata->pmic_reset_gpio, 0x1);
		mdelay(10);
	}


	ret = dio8018_read_reg(i2c, DIO8018_REG_PRODUCT_ID, &val);
	pr_info("%s: read PRODUCT ID %d 0x%x\n", __func__, ret, val);

	if (ret < 0) {
		pr_info("%s: read DIO8018_REG_PRODUCT_ID fail\n", __func__);
		return ret;
	}

	if (pdata->need_sw_reset) {
		ret = dio8018_read_reg(i2c, DIO8018_REG_RESET, &val);
		if (ret < 0) {
			pr_info("%s: read DIO8018_REG_RESET fail\n", __func__);
			return ret;
		}

		val |= (0xb0 | DIO8018_FLT_SD_B);
		dio8018_write_reg(i2c, DIO8018_REG_RESET, val);
		mdelay(100);
	}

	return 0;
}

static int dio8018_set_interrupt(struct i2c_client *i2c, u32 int_level_sel, u32 int_outmode_sel)
{
	int ret = 0;
	u8 val = 0;

	if (i2c) {
		ret = dio8018_read_reg(i2c, DIO8018_REG_I2C_ADDR, &val);
		if (ret < 0) {
			pr_info("%s: fail to read I2C_ADDR\n", __func__);
			return ret;
		}
		pr_info("%s: read I2C_ADDR %d 0x%x\n", __func__, ret, val);

		if (int_level_sel)
			val |= 0x80;
		else
			val &= ~0x80;

		if (int_outmode_sel)
			val |= 0x40;
		else
			val &= ~0x40;

		dio8018_write_reg(i2c, DIO8018_REG_I2C_ADDR, val);
		pr_info("%s: write I2C_ADDR %d 0x%x\n", __func__, ret, val);
	}

	return 0;
}

static int dio8018_enable(struct regulator_dev *rdev)
{
	struct dio8018_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;

	return dio8018_update_reg(i2c, rdev->desc->enable_reg,
					rdev->desc->enable_mask,
					rdev->desc->enable_mask);
}

static int dio8018_disable_regmap(struct regulator_dev *rdev)
{
	struct dio8018_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;

	return dio8018_update_reg(i2c, rdev->desc->enable_reg,
				   0, rdev->desc->enable_mask);
}

static int dio8018_is_enabled_regmap(struct regulator_dev *rdev)
{
	struct dio8018_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret;
	u8 val;

	ret = dio8018_read_reg(i2c, rdev->desc->enable_reg, &val);
	if (ret < 0)
		return ret;

	return (val & rdev->desc->enable_mask) != 0;
}

static int dio8018_get_voltage_sel_regmap(struct regulator_dev *rdev)
{
	struct dio8018_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	u8 val;

	if (dio8018_read_reg(i2c, rdev->desc->vsel_reg, &val) < 0)
		return dio8018_read_reg(i2c, rdev->desc->vsel_reg, &val);

	val &= rdev->desc->vsel_mask;

	return val;
}

static int dio8018_set_voltage_sel_regmap(struct regulator_dev *rdev, unsigned int sel)
{
	struct dio8018_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret;

	ret = dio8018_update_reg(i2c, rdev->desc->vsel_reg,
				sel, rdev->desc->vsel_mask);
	if (ret < 0)
		goto out;

	if (rdev->desc->apply_bit)
		ret = dio8018_update_reg(i2c, rdev->desc->apply_reg,
					 rdev->desc->apply_bit,
					 rdev->desc->apply_bit);
	return ret;
out:
	pr_warn("%s: failed to set voltage_sel_regmap\n", rdev->desc->name);
	return ret;
}

__visible_for_testing
int dio8018_ldo_current_check(struct regulator_dev *rdev, int min_uA, int max_uA)
{
	int sel = -1;

	if (rdev->desc->id < DIO8018_LDO1 || rdev->desc->id >= DIO8018_REGULATOR_MAX)
		return -EINVAL;

	for (int i = DIO8018_LDO_CURRENT_COUNT-1; i >= 0 ; i--) {
		if (min_uA <= dio8018_ldo_current[rdev->desc->id][i] && dio8018_ldo_current[rdev->desc->id][i] <= max_uA) {
			sel = i;
			break;
		}
	}

	if (sel < 0)
		return -EINVAL;

	sel <<= ffs(rdev->desc->enable_mask) - 1;

	return sel;
}
#if IS_ENABLED(CONFIG_SEC_KUNIT)
EXPORT_SYMBOL_KUNIT(dio8018_ldo_current_check);
#endif


static int dio8018_set_current_limit(struct regulator_dev *rdev, int min_uA, int max_uA)
{
	struct dio8018_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int sel = -1;

	sel = dio8018_ldo_current_check(rdev, min_uA, max_uA);

	if (sel == -EINVAL)
		return -EINVAL;

	return dio8018_update_reg(i2c, DIO8018_REG_IOUT, sel, rdev->desc->enable_mask);
}
static int dio8018_get_current_limit(struct regulator_dev *rdev)
{
	struct dio8018_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	u8 val = 0;
	int ret = 0;

	ret = dio8018_read_reg(i2c, DIO8018_REG_IOUT, &val);
	if (ret < 0)
		return ret;

	val &= rdev->desc->enable_mask;
	val >>= ffs(rdev->desc->enable_mask) - 1;
	if (val < DIO8018_LDO_CURRENT_COUNT && (rdev->desc->id >= DIO8018_LDO1 && rdev->desc->id < DIO8018_REGULATOR_MAX))
		return dio8018_ldo_current[rdev->desc->id][val];

	return -EINVAL;
}

static const struct regulator_ops dio8018_ldo_ops = {
	.list_voltage		= regulator_list_voltage_linear_range,
	.is_enabled			= dio8018_is_enabled_regmap,
	.enable				= dio8018_enable,
	.disable			= dio8018_disable_regmap,
	.get_voltage_sel	= dio8018_get_voltage_sel_regmap,
	.set_voltage_sel	= dio8018_set_voltage_sel_regmap,
	.set_voltage_time_sel	= regulator_set_voltage_time_sel,
	.set_current_limit		= dio8018_set_current_limit,
	.get_current_limit		= dio8018_get_current_limit,
};

#define _LDO(macro)	DIO8018_LDO##macro
#define _REG(ctrl)	DIO8018_REG##ctrl
#define _ldo_ops(num)	dio8018_ldo_ops##num


#define LDO_DESC(_name, _id, _ops, d, r, n, v, e)	{	\
	.name		= _name,				\
	.id		= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.ramp_delay = d,							\
	.linear_ranges = r,					\
	.n_linear_ranges = ARRAY_SIZE(r),		\
	.n_voltages	= n,		\
	.vsel_reg	= v,					\
	.vsel_mask	= DIO8018_LDO_VSEL_MASK,		\
	.enable_reg	= DIO8018_REG_ENABLE,			\
	.enable_mask	= 0x01<<e,		\
	.enable_time	= DIO8018_ENABLE_TIME_LDO	\
}

static struct regulator_desc regulators[DIO8018_REGULATOR_MAX] = {
	/* name, id, ops, min_uv, uV_step, vsel_reg, enable_reg */
	LDO_DESC("dio8018-ldo1", _LDO(1), &_ldo_ops(), DIO8018_RAMP_DELAY1, dio8018_ldo12_range, DIO8018_LDO12_VOLTAGES,
		_REG(_LDO1_VOUT), _LDO(1)),
	LDO_DESC("dio8018-ldo2", _LDO(2), &_ldo_ops(), DIO8018_RAMP_DELAY1, dio8018_ldo12_range, DIO8018_LDO12_VOLTAGES,
		_REG(_LDO2_VOUT), _LDO(2)),
	LDO_DESC("dio8018-ldo3", _LDO(3), &_ldo_ops(), DIO8018_RAMP_DELAY2, dio8018_ldo37_range, DIO8018_LDO37_VOLTAGES,
		_REG(_LDO3_VOUT), _LDO(3)),
	LDO_DESC("dio8018-ldo4", _LDO(4), &_ldo_ops(), DIO8018_RAMP_DELAY2, dio8018_ldo37_range, DIO8018_LDO37_VOLTAGES,
		_REG(_LDO4_VOUT), _LDO(4)),
	LDO_DESC("dio8018-ldo5", _LDO(5), &_ldo_ops(), DIO8018_RAMP_DELAY2, dio8018_ldo37_range, DIO8018_LDO37_VOLTAGES,
		_REG(_LDO5_VOUT), _LDO(5)),
	LDO_DESC("dio8018-ldo6", _LDO(6), &_ldo_ops(), DIO8018_RAMP_DELAY2, dio8018_ldo37_range, DIO8018_LDO37_VOLTAGES,
		_REG(_LDO6_VOUT), _LDO(6)),
	LDO_DESC("dio8018-ldo7", _LDO(7), &_ldo_ops(), DIO8018_RAMP_DELAY2, dio8018_ldo37_range, DIO8018_LDO37_VOLTAGES,
		_REG(_LDO7_VOUT), _LDO(7))
};

#if IS_ENABLED(CONFIG_SEC_KUNIT)
struct regulator_desc *dio8018_regulators;
EXPORT_SYMBOL_KUNIT(dio8018_regulators);
#endif

#if IS_ENABLED(CONFIG_SEC_FACTORY)
/*
 * Recovery logic for DIO8018 detach test
 */

bool regulator_should_be_enabled(struct regulator_dev *rdev)
{
	return (rdev->constraints->always_on) || (rdev->use_count > 0);
}

int dio8018_need_self_recovery(struct dio8018_data *dio8018)
{
	struct regulator_dev *rdev;
	int i, ret = 0;

	for (i = 0; i < dio8018->num_regulators; i++) {
		rdev = dio8018->rdev[i];
		if (!rdev)
			continue;

		if (!regulator_should_be_enabled(rdev))
			continue;

		if (!dio8018_is_enabled_regmap(rdev)) {
			pr_info("%s: %s\n", __func__, rdev->desc->name);
			ret = 1;
		}
	}

	return ret;
}

int dio8018_recovery_voltage(struct regulator_dev *rdev)
{
	int ret = -1;
	unsigned int vol, reg;

	// Get and calculate voltage from regulator framework
	vol = (rdev->constraints->min_uV - rdev->desc->min_uV) / rdev->desc->uV_step;
	reg = dio8018_get_voltage_sel_regmap(rdev);

	// Set proper voltage according to regulator type
	if (rdev->desc->vsel_mask == DIO8018_LDO_VSEL_MASK)
		ret = dio8018_set_voltage_sel_regmap(rdev, vol);

	return ret;
}

int dio8018_recovery(struct dio8018_data *dio8018)
{
	struct regulator_dev *rdev;
	int i, ret = 0;

	if (!dio8018) {
		pr_info("%s: There is no local rdev data\n", __func__);
		return -ENODEV;
	}

	for (i = 0; i < dio8018->num_regulators; i++) {
		rdev = dio8018->rdev[i];
		if (!rdev)
			continue;

		pr_info("%s: %s: max(%d), min(%d), always_on(%d), use_count(%d)\n",
					__func__, rdev->desc->name, rdev->constraints->max_uV, rdev->desc->min_uV,
					rdev->constraints->always_on, rdev->use_count);

		// Make sure enabled registers are cleared
		dio8018_disable_regmap(rdev);

		ret = dio8018_recovery_voltage(rdev);
		if (ret < 0)
			return ret;

		if (regulator_should_be_enabled(rdev)) {
			ret = dio8018_enable(rdev);
			if (ret < 0)
				return ret;
		}
	}

	pr_info("%s: dio8018 is successfully recovered!\n", __func__);

	return ret;
}

static int dio8018_regulator_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct dio8018_data *dio8018 = platform_get_drvdata(pdev);

	pr_info("%s: Check recovery needs\n", __func__);

	if (dio8018_need_self_recovery(dio8018))
		dio8018_recovery(dio8018);

	return 0;
}

const struct dev_pm_ops dio8018_regulator_pm = {
	.resume = dio8018_regulator_resume,
};
#endif

static void dio8018_irq_work(struct work_struct *work)
{
	struct dio8018_data *dio8018 = container_of(work, struct dio8018_data, work);
	int i = 0;
	u8 intr1 = 0, intr2 = 0, intr3 = 0;
	u8 intr12_mask[DIO8018_REGULATOR_MAX] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40};
	u8 intr3_mask[DIO8018_REGULATOR_MAX] = {0x01, 0x01, 0x02, 0x02, 0x04, 0x08, 0x10};

	// UVP interrupt
	if (dio8018_read_reg(dio8018->iodev->i2c, DIO8018_REG_INTRRUPT1, &intr1) < 0)
		goto _out;
	// OCP interrupt
	if (dio8018_read_reg(dio8018->iodev->i2c, DIO8018_REG_INTRRUPT2, &intr2) < 0)
		goto _out;
	// TSD / UVLO interrupt
	if (dio8018_read_reg(dio8018->iodev->i2c, DIO8018_REG_INTRRUPT3, &intr3) < 0)
		goto _out;

	pr_info("%s: 0x%02x 0x%02x 0x%02x\n", __func__, intr1, intr2, intr3);

	/*notify..*/
	for (i = 0; i < dio8018->num_regulators; i++) {
		struct regulator_dev *rdev = dio8018->rdev[i];

		if (intr1 & intr12_mask[rdev->desc->id] || intr3 & intr3_mask[rdev->desc->id]) {
			regulator_notifier_call_chain(rdev, REGULATOR_EVENT_UNDER_VOLTAGE, NULL);
			pr_info("%s: %s REGULATOR_EVENT_UNDER_VOLTAGE\n", __func__, rdev->desc->name);
		}

		if (intr2 & intr12_mask[rdev->desc->id]) {
			regulator_notifier_call_chain(rdev, REGULATOR_EVENT_OVER_CURRENT, NULL);
			pr_info("%s: %s REGULATOR_EVENT_OVER_CURRENT\n", __func__, rdev->desc->name);
		}

		if (intr3 & 0x80 || intr3 & 0x40) {
			regulator_notifier_call_chain(rdev, REGULATOR_EVENT_OVER_TEMP, NULL);
			pr_info("%s: %s REGULATOR_EVENT_OVER_TEMP\n", __func__, rdev->desc->name);
		}

		if (intr3 & 0x20) {
			regulator_notifier_call_chain(rdev, REGULATOR_EVENT_FAIL, NULL);
			pr_info("%s: %s REGULATOR_EVENT_FAIL\n", __func__, rdev->desc->name);
		}
	}

_out:

	enable_irq(dio8018->iodev->pmic_irq);

}

static irqreturn_t dio8018_irq_thread(int irq, void *irq_data)
{
	struct dio8018_data *dio8018 = (struct dio8018_data *)irq_data;

	pr_info("%s: interrupt occurred(%d)\n", __func__, irq);

	disable_irq_nosync(irq);

	queue_work(dio8018->wq, &dio8018->work);

	return IRQ_HANDLED;
}


#if IS_ENABLED(CONFIG_OF)
static int dio8018_pmic_dt_parse_pdata(struct device *dev,
					struct dio8018_platform_data *pdata)
{
	struct device_node *pmic_np, *regulators_np, *reg_np;
	struct dio8018_regulator_data *rdata;
	size_t i;

	pmic_np = dev->of_node;
	if (!pmic_np) {
		dev_err(dev, "could not find pmic sub-node\n");
		return -ENODEV;
	}

	pdata->pmic_irq_gpio = of_get_named_gpio(pmic_np, "dio8018,dio8018_int", 0);
	if (!gpio_is_valid(pdata->pmic_irq_gpio)) {
		pr_err("%s error reading dio8018_irq = %d\n", __func__, pdata->pmic_irq_gpio);
		pdata->pmic_irq_gpio = -ENXIO;
	}

	if (of_property_read_u32(pmic_np, "dio8018,dio8018_int_level", &pdata->pmic_irq_level_sel) < 0)
		pdata->pmic_irq_level_sel = 1; // set as register default value

	if (of_property_read_u32(pmic_np, "dio8018,dio8018_int_outmode", &pdata->pmic_irq_outmode_sel) < 0)
		pdata->pmic_irq_outmode_sel = 0; // set as register default value

	pdata->wakeup = of_property_read_bool(pmic_np, "dio8018,wakeup");

	pdata->need_hw_reset = of_property_read_bool(pmic_np, "dio8018,need_hw_reset");

	pdata->pmic_reset_gpio = of_get_named_gpio(pmic_np, "dio8018,dio8018_reset", 0);
	if (!gpio_is_valid(pdata->pmic_reset_gpio)) {
		dev_err(dev, "hw reset gpio is not ready\n");
		return -EPROBE_DEFER;
	}

	pdata->need_sw_reset = of_property_read_bool(pmic_np, "dio8018,need_sw_reset");

	regulators_np = of_find_node_by_name(pmic_np, "regulators");
	if (!regulators_np) {
		dev_err(dev, "could not find regulators sub-node\n");
		return -EINVAL;
	}

	pdata->need_self_recovery = of_property_read_bool(pmic_np, "dio8018,need_self_recovery");

	/* count the number of regulators to be supported in pmic */
	pdata->num_regulators = 0;
	for_each_child_of_node(regulators_np, reg_np) {
		pdata->num_regulators++;
	}

	rdata = devm_kzalloc(dev, sizeof(*rdata) *
				pdata->num_regulators, GFP_KERNEL);
	if (!rdata) {
		dev_err(dev, "could not allocate memory for regulator data\n");
		return -ENOMEM;
	}

	pdata->regulators = rdata;
	pdata->num_rdata = 0;
	for_each_child_of_node(regulators_np, reg_np) {
		for (i = 0; i < ARRAY_SIZE(regulators); i++)
			if (!of_node_cmp(reg_np->name,
					regulators[i].name))
				break;

		if (i == ARRAY_SIZE(regulators)) {
			dev_warn(dev,
			"don't know how to configure regulator %s\n",
			reg_np->name);
			continue;
		}

		rdata->id = i;
		rdata->initdata = of_get_regulator_init_data(
						dev, reg_np,
						&regulators[i]);
		rdata->reg_node = reg_np;
		rdata++;
		pdata->num_rdata++;
	}
	of_node_put(regulators_np);

	return 0;
}
#else
static int dio8018_pmic_dt_parse_pdata(struct dio8018_dev *iodev,
					struct dio8018_platform_data *pdata)
{
	pr_info("%s:%s !IS_ENABLED(CONFIG_OF)\n", DIO8018_DEV_NAME, __func__);
	return 0;
}
#endif /* CONFIG_OF */

#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC) || defined(_SUPPORT_SYSFS_INTERFACE)
static ssize_t dio8018_read_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct dio8018_data *dio8018 = dev_get_drvdata(dev);
	int ret;
	u8 val, reg_addr;

	if (buf == NULL) {
		pr_info("%s: empty buffer\n", __func__);
		return -1;
	}

	ret = kstrtou8(buf, 0, &reg_addr);
	if (ret < 0)
		pr_info("%s: fail to transform i2c address\n", __func__);

	ret = dio8018_read_reg(dio8018->iodev->i2c, reg_addr, &val);
	if (ret < 0)
		pr_info("%s: fail to read i2c address\n", __func__);

	pr_info("%s: reg(0x%02x) data(0x%02x)\n", __func__, reg_addr, val);
	dio8018->read_addr = reg_addr;
	dio8018->read_val = val;

	return size;
}

static ssize_t dio8018_read_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct dio8018_data *dio8018 = dev_get_drvdata(dev);

	return sprintf(buf, "0x%02hhx: 0x%02hhx\n", dio8018->read_addr,
		       dio8018->read_val);
}

static ssize_t dio8018_write_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t size)
{
	struct dio8018_data *dio8018 = dev_get_drvdata(dev);
	int ret;
	u8 reg = 0, data = 0;

	if (buf == NULL) {
		pr_info("%s: empty buffer\n", __func__);
		return size;
	}

	ret = sscanf(buf, "0x%02hhx 0x%02hhx", &reg, &data);
	if (ret != 2) {
		pr_info("%s: input error\n", __func__);
		return size;
	}

	pr_info("%s: reg(0x%02hhx) data(0x%02hhx)\n", __func__, reg, data);

	ret = dio8018_write_reg(dio8018->iodev->i2c, reg, data);
	if (ret < 0)
		pr_info("%s: fail to write i2c addr/data\n", __func__);

	return size;
}

static ssize_t dio8018_write_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	return sprintf(buf, "echo (register addr.) (data) > dio8018_write\n");
}

#define ATTR_REGULATOR	(2)
static struct pmic_device_attribute regulator_attr[] = {
	PMIC_ATTR(dio8018_write, 0644, dio8018_write_show, dio8018_write_store),
	PMIC_ATTR(dio8018_read, 0644, dio8018_read_show, dio8018_read_store),
};

static int dio8018_create_sysfs(struct dio8018_data *dio8018)
{
	struct device *dio8018_pmic = dio8018->dev;
	struct device *dev = dio8018->iodev->dev;
	char device_name[32] = {0,};
	int err = -ENODEV, i = 0;

	dio8018->read_addr = 0;
	dio8018->read_val = 0;

	/* Dynamic allocation for device name */
	snprintf(device_name, sizeof(device_name) - 1, "%s@%s",
		 dev_driver_string(dev), dev_name(dev));

#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)
	dio8018_pmic = pmic_device_create(dio8018, device_name);
#elif defined(_SUPPORT_SYSFS_INTERFACE)
	pmic_class = class_create(THIS_MODULE, "pmic");
	if (IS_ERR(pmic_class)) {
		pr_err("Failed to create class(pmic) %ld\n", PTR_ERR(pmic_class));
		return -1;
	}

	dio8018_pmic = device_create(pmic_class, NULL, atomic_inc_return(&pmic_dev),
			dio8018, "%s", device_name);
	if (IS_ERR(dio8018_pmic))
		pr_err("Failed to create device %s %ld\n", device_name, PTR_ERR(dio8018_pmic));
	else
		pr_debug("%s : %s : %d\n", __func__, device_name, dio8018_pmic->devt);
#endif

	dio8018->dev = dio8018_pmic;

	/* Create sysfs entries */
	for (i = 0; i < ATTR_REGULATOR; i++) {
		err = device_create_file(dio8018_pmic, &regulator_attr[i].dev_attr);
		if (err)
			goto remove_pmic_device;
	}

	return 0;

remove_pmic_device:
	for (i--; i >= 0; i--)
		device_remove_file(dio8018_pmic, &regulator_attr[i].dev_attr);
#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)
	pmic_device_destroy(dio8018_pmic->devt);
#else
	device_destroy(pmic_class, dio8018_pmic->devt);
#endif
	return -1;
}
#endif

static int __dio8018_pmic_probe(struct i2c_client *i2c)
{
	struct dio8018_dev *iodev;
	struct dio8018_platform_data *pdata = i2c->dev.platform_data;
	struct regulator_config config = { };
	struct dio8018_data *dio8018;
	size_t i;
	int ret = 0;

	pr_info("%s:%s\n", DIO8018_DEV_NAME, __func__);

	iodev = devm_kzalloc(&i2c->dev, sizeof(struct dio8018_dev), GFP_KERNEL);
	if (!iodev) {
		dev_err(&i2c->dev, "%s: Failed to alloc mem for dio8018\n", __func__);
		return -ENOMEM;
	}

#if IS_ENABLED(CONFIG_SEC_KUNIT)
	dio8018_regulators = devm_kzalloc(&i2c->dev, sizeof(struct regulator_desc *), GFP_KERNEL);
	if (!dio8018_regulators) {
		dev_err(&i2c->dev, "%s: Failed to alloc mem for dio8018_regulators\n", __func__);
		return -ENOMEM;
	}
#endif

	if (i2c->dev.of_node) {
		pdata = devm_kzalloc(&i2c->dev,
			sizeof(struct dio8018_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&i2c->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_pdata;
		}
		ret = dio8018_pmic_dt_parse_pdata(&i2c->dev, pdata);
		if (ret < 0) {
			dev_err(&i2c->dev, "Failed to get device of_node\n");
			goto err_pdata;
		}

		i2c->dev.platform_data = pdata;
	} else
		pdata = i2c->dev.platform_data;

	iodev->dev = &i2c->dev;
	iodev->i2c = i2c;

	if (pdata) {
		iodev->pdata = pdata;
		iodev->wakeup = pdata->wakeup;
	} else {
		ret = -EINVAL;
		goto err_pdata;
	}
	mutex_init(&iodev->i2c_lock);

	dio8018 = devm_kzalloc(&i2c->dev, sizeof(struct dio8018_data),
				GFP_KERNEL);
	if (!dio8018) {
		pr_info("[%s:%d] if (!dio8018)\n", __FILE__, __LINE__);
		ret = -ENOMEM;
		goto err_dio8018_data;
	}

	i2c_set_clientdata(i2c, dio8018);
	dio8018->iodev = iodev;
	dio8018->num_regulators = pdata->num_rdata;
	dio8018->need_self_recovery = pdata->need_self_recovery;

	if (pdata->need_hw_reset)
		dio8018_reset(i2c, pdata);

	for (i = 0; i < pdata->num_rdata; i++) {
		int id = pdata->regulators[i].id;

		config.dev = &i2c->dev;
		config.init_data = pdata->regulators[i].initdata;
		config.driver_data = dio8018;
		config.of_node = pdata->regulators[i].reg_node;
		dio8018->rdev[i] = devm_regulator_register(&i2c->dev,
							   &regulators[id], &config);
		if (IS_ERR(dio8018->rdev[i])) {
			ret = PTR_ERR(dio8018->rdev[i]);
			dev_err(&i2c->dev, "regulator init failed for %d\n",
				id);
			dio8018->rdev[i] = NULL;
			goto err_dio8018_data;
		}
#if IS_ENABLED(CONFIG_REGULATOR_DEBUG_CONTROL)
		ret = devm_regulator_debug_register(&i2c->dev, dio8018->rdev[i]);
		if (ret)
			dev_err(&i2c->dev, "failed to register debug regulator for %lu, rc=%d\n",
					i, ret);
#endif
	}

	dio8018_set_interrupt(i2c, pdata->pmic_irq_level_sel, pdata->pmic_irq_outmode_sel);

	if (pdata->pmic_irq_gpio >= 0) {
		gpio_request(pdata->pmic_irq_gpio, "dio8018_irq_gpio");
		gpio_direction_input(pdata->pmic_irq_gpio);

		iodev->pmic_irq = gpio_to_irq(pdata->pmic_irq_gpio);

		if (iodev->pmic_irq > 0) {

			dio8018->wq = create_singlethread_workqueue("dio8018-wq");
			if (!dio8018->wq) {
				dev_err(&i2c->dev, "%s: Failed to Request IRQ\n", __func__);
				goto err_dio8018_data;
			}

			INIT_WORK(&dio8018->work, dio8018_irq_work);

			ret = request_threaded_irq(iodev->pmic_irq,
					NULL, dio8018_irq_thread,
					IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
					"dio8018-pmic-irq", dio8018);
			if (ret) {
				dev_err(&i2c->dev, "%s: Failed to Request IRQ\n", __func__);
				goto err_dio8018_data;
			}

			if (pdata->wakeup) {
				enable_irq_wake(iodev->pmic_irq);
				ret = device_init_wakeup(iodev->dev, pdata->wakeup);
				if (ret < 0) {
					pr_err("%s: Fail to device init wakeup fail(%d)\n", __func__, ret);
					goto err_dio8018_data;
				}
			}
		} else {
			dev_err(&i2c->dev, "%s: Failed gpio_to_irq(%d)\n", __func__, iodev->pmic_irq);
			goto err_dio8018_data;
		}
	} else {
		pr_info("%s: Interrupt pin was not used.\n", __func__);
	}

#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC) || defined(_SUPPORT_SYSFS_INTERFACE)
	ret = dio8018_create_sysfs(dio8018);
	if (ret < 0) {
		pr_err("%s: dio8018_create_sysfs fail\n", __func__);
		goto err_dio8018_data;
	}
#endif

	/* Disable UVP or OCP event */
	ret = dio8018_update_reg(i2c, DIO8018_REG_RESET, DIO8018_FLT_SD_B, DIO8018_FLT_SD_B);

	pr_info("%s: complete.\n", __func__);
	return ret;

err_dio8018_data:
	mutex_destroy(&iodev->i2c_lock);
	if (dio8018 && dio8018->wq)
		destroy_workqueue(dio8018->wq);
err_pdata:
	pr_info("[%s:%d] err\n", __func__, __LINE__);
	return ret;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id dio8018_i2c_dt_ids[] = {
	{ .compatible = "dioo,dio8018pmic" },
	{ },
};
#endif /* CONFIG_OF */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int dio8018_pmic_probe(struct i2c_client *i2c)
{
	return __dio8018_pmic_probe(i2c);
}
#else
static int dio8018_pmic_probe(struct i2c_client *i2c,
				const struct i2c_device_id *dev_id)
{
	return __dio8018_pmic_probe(i2c);
}
#endif

static void dio8018_pmic_remove(struct i2c_client *i2c)
{
	struct dio8018_data *info = i2c_get_clientdata(i2c);
	struct dio8018_dev *dio8018 = info->iodev;

#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC) || defined(_SUPPORT_SYSFS_INTERFACE)
	struct device *dio8018_pmic = info->dev;
	int i = 0;

	/* Remove sysfs entries */
	for (i = 0; i < ATTR_REGULATOR; i++)
		device_remove_file(dio8018_pmic, &regulator_attr[i].dev_attr);

#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)
	pmic_device_destroy(dio8018_pmic->devt);
#else
	device_destroy(pmic_class, dio8018_pmic->devt);
#endif

#endif

	if (dio8018->pdata && dio8018->pdata->pmic_irq_gpio)
		gpio_free(dio8018->pdata->pmic_irq_gpio);

	if (dio8018->pdata && dio8018->pdata->need_hw_reset && dio8018->pdata->pmic_reset_gpio >= 0)
		gpio_free(dio8018->pdata->pmic_reset_gpio);

	if (dio8018->pmic_irq > 0)
		free_irq(dio8018->pmic_irq, NULL);

	if (info->wq)
		destroy_workqueue(info->wq);

}

#if IS_ENABLED(CONFIG_OF)
static const struct i2c_device_id dio8018_pmic_id[] = {
	{"dio8018-regulator", 0},
	{},
};
#endif

static struct i2c_driver dio8018_i2c_driver = {
	.driver = {
		.name = "dio8018-regulator",
		.owner = THIS_MODULE,
#if IS_ENABLED(CONFIG_OF)
		.of_match_table	= dio8018_i2c_dt_ids,
#endif /* CONFIG_OF */
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		.pm = &dio8018_regulator_pm,
#endif
		.suppress_bind_attrs = true,
	},
	.probe = dio8018_pmic_probe,
	.remove = dio8018_pmic_remove,
	.id_table = dio8018_pmic_id,
};

static int __init dio8018_i2c_init(void)
{
	pr_info("%s:%s\n", DIO8018_DEV_NAME, __func__);
	return i2c_add_driver(&dio8018_i2c_driver);
}
subsys_initcall(dio8018_i2c_init);

static void __exit dio8018_i2c_exit(void)
{
	i2c_del_driver(&dio8018_i2c_driver);
}
module_exit(dio8018_i2c_exit);

MODULE_DESCRIPTION("DIOO DIO8018 Regulator Driver");
MODULE_LICENSE("GPL");

