/*
 * s2mpb02_regulator.c - Regulator driver for the Samsung s2mpb02
 *
 * Copyright (C) 2014 Samsung Electronics
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/module.h>
#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#if IS_ENABLED(CONFIG_REGULATOR_DEBUG_CONTROL)
#include <linux/regulator/debug-regulator.h>
#endif
#include <linux/mfd/samsung/s2mpb02.h>
#include <linux/mfd/samsung/s2mpb02-regulator.h>
#include <linux/regulator/of_regulator.h>
#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)
#include <linux/regulator/pmic_class.h>
#endif

struct s2mpb02_data {
	struct s2mpb02_dev *iodev;
	int num_regulators;
	struct regulator_dev *rdev[S2MPB02_REGULATOR_MAX];
	bool need_recovery;
#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)
	u8 read_addr;
	u8 read_val;
	struct device *dev;
#endif
};

static int s2m_enable(struct regulator_dev *rdev)
{
	struct s2mpb02_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;

	return s2mpb02_update_reg(i2c, rdev->desc->enable_reg,
					rdev->desc->enable_mask,
					rdev->desc->enable_mask);
}

static int s2m_disable_regmap(struct regulator_dev *rdev)
{
	struct s2mpb02_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	u8 val;

	if (rdev->desc->enable_is_inverted)
		val = rdev->desc->enable_mask;
	else
		val = 0;

	return s2mpb02_update_reg(i2c, rdev->desc->enable_reg,
				   val, rdev->desc->enable_mask);
}

static int s2m_is_enabled_regmap(struct regulator_dev *rdev)
{
	struct s2mpb02_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret;
	u8 val;

	ret = s2mpb02_read_reg(i2c, rdev->desc->enable_reg, &val);
	if (ret < 0)
		return ret;

	val &= rdev->desc->enable_mask;

	return (val == rdev->desc->enable_mask);
}

static int s2m_get_voltage_sel_regmap(struct regulator_dev *rdev)
{
	struct s2mpb02_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret;
	u8 val;

	ret = s2mpb02_read_reg(i2c, rdev->desc->vsel_reg, &val);
	if (ret < 0)
		return ret;

	val &= rdev->desc->vsel_mask;

	return val;
}

static int s2m_set_voltage_sel_regmap(struct regulator_dev *rdev, unsigned sel)
{
	struct s2mpb02_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret;

	ret = s2mpb02_update_reg(i2c, rdev->desc->vsel_reg,
				sel, rdev->desc->vsel_mask);
	if (ret < 0)
		goto out;

	if (rdev->desc->apply_bit)
		ret = s2mpb02_update_reg(i2c, rdev->desc->apply_reg,
					 rdev->desc->apply_bit,
					 rdev->desc->apply_bit);
	return ret;
out:
	pr_warn("%s: failed to set voltage_sel_regmap\n", rdev->desc->name);
	return ret;
}

static int s2m_set_voltage_sel_regmap_buck(struct regulator_dev *rdev,
					unsigned sel)
{
	struct s2mpb02_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret;

	ret = s2mpb02_write_reg(i2c, rdev->desc->vsel_reg, sel);
	if (ret < 0)
		goto out;

	if (rdev->desc->apply_bit)
		ret = s2mpb02_update_reg(i2c, rdev->desc->apply_reg,
					 rdev->desc->apply_bit,
					 rdev->desc->apply_bit);
	return ret;
out:
	pr_warn("%s: failed to set voltage_sel_regmap\n", rdev->desc->name);
	return ret;
}

static int s2m_set_voltage_time_sel(struct regulator_dev *rdev,
				   unsigned int old_selector,
				   unsigned int new_selector)
{
	int old_volt, new_volt;

	/* sanity check */
	if (!rdev->desc->ops->list_voltage)
		return -EINVAL;

	old_volt = rdev->desc->ops->list_voltage(rdev, old_selector);
	new_volt = rdev->desc->ops->list_voltage(rdev, new_selector);

	if (old_selector < new_selector)
		return DIV_ROUND_UP(new_volt - old_volt, S2MPB02_RAMP_DELAY);

	return 0;
}

#if IS_ENABLED(CONFIG_SEC_PM)
#define S2MPB02_BUCK_MODE_MASK	(3 << 2)
#define S2MPB02_BUCK_MODE_FPWM	(3 << 2)
#define S2MPB02_BUCK_MODE_AUTO	(2 << 2)

/* BUCKs & BB support [Auto/Force PWM] mode */
static int s2m_set_buck_mode(struct regulator_dev *rdev, unsigned int mode)
{
	struct s2mpb02_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	u8 val;

	dev_info(info->iodev->dev, "%s: mode: %u\n", __func__, mode);

	if (mode == REGULATOR_MODE_FAST)
		val = S2MPB02_BUCK_MODE_FPWM;
	else if (mode == REGULATOR_MODE_NORMAL)
		val = S2MPB02_BUCK_MODE_AUTO;
	else
		return -EINVAL;

	return s2mpb02_update_reg(i2c, rdev->desc->enable_reg, val,
			S2MPB02_BUCK_MODE_MASK);
}

static unsigned int s2m_get_buck_mode(struct regulator_dev *rdev)
{
	struct s2mpb02_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret;
	u8 val;

	ret = s2mpb02_read_reg(i2c, rdev->desc->enable_reg, &val);
	if (ret < 0)
		return ret;

	val = val & S2MPB02_BUCK_MODE_MASK;
	dev_info(info->iodev->dev, "%s: val: %u\n", __func__, val >> 2);

	if (val == S2MPB02_BUCK_MODE_FPWM)
		ret = REGULATOR_MODE_FAST;
	else if (val == S2MPB02_BUCK_MODE_AUTO)
		ret = REGULATOR_MODE_NORMAL;
	else
		ret = -EINVAL;

	return ret;
}
#endif /* CONFIG_SEC_PM */

static struct regulator_ops s2mpb02_ldo_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= s2m_is_enabled_regmap,
	.enable			= s2m_enable,
	.disable		= s2m_disable_regmap,
	.get_voltage_sel	= s2m_get_voltage_sel_regmap,
	.set_voltage_sel	= s2m_set_voltage_sel_regmap,
	.set_voltage_time_sel	= s2m_set_voltage_time_sel,
};

static struct regulator_ops s2mpb02_buck_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= s2m_is_enabled_regmap,
	.enable			= s2m_enable,
	.disable		= s2m_disable_regmap,
	.get_voltage_sel	= s2m_get_voltage_sel_regmap,
	.set_voltage_sel	= s2m_set_voltage_sel_regmap_buck,
	.set_voltage_time_sel	= s2m_set_voltage_time_sel,
#if IS_ENABLED(CONFIG_SEC_PM)
	.set_mode		= s2m_set_buck_mode,
	.get_mode		= s2m_get_buck_mode,
#endif /* CONFIG_SEC_PM */
};

#if IS_ENABLED(CONFIG_SEC_PM)
static unsigned int s2mpb02_of_map_mode(unsigned int mode)
{
	switch (mode) {
	case 1 ... 2:		/* Forced PWM & Auto */
		return mode;
	default:
		return REGULATOR_MODE_INVALID;
	}
}
#else
static unsigned int s2mpb02_of_map_mode(unsigned int mode)
{
	return REGULATOR_MODE_INVALID;
}
#endif /* CONFIG_SEC_PM */

#define _BUCK(macro)	S2MPB02_BUCK##macro
#define _buck_ops(num)	s2mpb02_buck_ops##num

#define _LDO(macro)	S2MPB02_LDO##macro
#define _REG(ctrl)	S2MPB02_REG##ctrl
#define _ldo_ops(num)	s2mpb02_ldo_ops##num
#define _TIME(macro)	S2MPB02_ENABLE_TIME##macro

#define BUCK_DESC(_name, _id, _ops, m, s, v, e, t)	{	\
	.name		= _name,				\
	.id		= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= m,					\
	.uV_step	= s,					\
	.n_voltages	= S2MPB02_BUCK_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2MPB02_BUCK_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= S2MPB02_BUCK_ENABLE_MASK,		\
	.enable_time	= t,					\
	.of_map_mode	= s2mpb02_of_map_mode,			\
}

#define LDO_DESC(_name, _id, _ops, m, s, v, e, t)	{	\
	.name		= _name,				\
	.id		= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= m,					\
	.uV_step	= s,					\
	.n_voltages	= S2MPB02_LDO_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2MPB02_LDO_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= S2MPB02_LDO_ENABLE_MASK,		\
	.enable_time	= t					\
}

static struct regulator_desc regulators[S2MPB02_REGULATOR_MAX] = {
	/* name, id, ops, min_uv, uV_step, vsel_reg, enable_reg */
	LDO_DESC("s2mpb02-ldo1", _LDO(1), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP1), _REG(_L1CTRL), _REG(_L1CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo2", _LDO(2), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP1), _REG(_L2CTRL), _REG(_L2CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo3", _LDO(3), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP1), _REG(_L3CTRL), _REG(_L3CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo4", _LDO(4), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP1), _REG(_L4CTRL), _REG(_L4CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo5", _LDO(5), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP1), _REG(_L5CTRL), _REG(_L5CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo6", _LDO(6), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP2), _REG(_L6CTRL), _REG(_L6CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo7", _LDO(7), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP2), _REG(_L7CTRL), _REG(_L7CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo8", _LDO(8), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP2), _REG(_L8CTRL), _REG(_L8CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo9", _LDO(9), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP2), _REG(_L9CTRL), _REG(_L9CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo10", _LDO(10), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP2), _REG(_L10CTRL), _REG(_L10CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo11", _LDO(11), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP2), _REG(_L11CTRL), _REG(_L11CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo12", _LDO(12), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP2), _REG(_L12CTRL), _REG(_L12CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo13", _LDO(13), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP2), _REG(_L13CTRL), _REG(_L13CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo14", _LDO(14), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP2), _REG(_L14CTRL), _REG(_L14CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo15", _LDO(15), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP2), _REG(_L15CTRL), _REG(_L15CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo16", _LDO(16), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP2), _REG(_L16CTRL), _REG(_L16CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo17", _LDO(17), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP2), _REG(_L17CTRL), _REG(_L17CTRL), _TIME(_LDO)),
	LDO_DESC("s2mpb02-ldo18", _LDO(18), &_ldo_ops(), _LDO(_MIN1),
		 _LDO(_STEP2), _REG(_L18CTRL), _REG(_L18CTRL), _TIME(_LDO)),

	BUCK_DESC("s2mpb02-buck1", _BUCK(1), &_buck_ops(), _BUCK(_MIN1),
		  _BUCK(_STEP1), _REG(_B1CTRL2), _REG(_B1CTRL1), _TIME(_BUCK)),
	BUCK_DESC("s2mpb02-buck2", _BUCK(2), &_buck_ops(), _BUCK(_MIN1),
		  _BUCK(_STEP1), _REG(_B2CTRL2), _REG(_B2CTRL1), _TIME(_BUCK)),
	BUCK_DESC("s2mpb02-bb", S2MPB02_BB1, &_buck_ops(), _BUCK(_MIN2),
		  _BUCK(_STEP2), _REG(_BB1CTRL2), _REG(_BB1CTRL1), _TIME(_BB)),
};

#if IS_ENABLED(CONFIG_SEC_FACTORY)
/*
 * Recovery logic for S2MPB02 detach test
 */
 
bool regulator_should_be_enabled(struct regulator_dev *rdev)
{
	return (rdev->constraints->always_on) || (rdev->use_count > 0);
}
 
int s2mpb02_need_recovery(struct s2mpb02_data *s2mpb02)
{
	struct regulator_dev *rdev;
	int i, ret = 0;

	// Check whether S2MPB02 Recovery is needed
	for (i = 0; i < s2mpb02->num_regulators; i++) {
		rdev = s2mpb02->rdev[i];
		if (!rdev)
			continue;

		if (!regulator_should_be_enabled(rdev))
			continue;

		if (!s2m_is_enabled_regmap(rdev)) {
			pr_info("%s: %s\n", __func__, rdev->desc->name);
			ret = 1;
		}
	}

	return ret;
}

int s2mpb02_recovery_voltage(struct regulator_dev *rdev)
{
	int ret = -1;
	unsigned int vol, reg;

	// Get and calculate voltage from regulator framework
	vol = (rdev->constraints->min_uV - rdev->desc->min_uV) / rdev->desc->uV_step;
	reg = s2m_get_voltage_sel_regmap(rdev);

	// Set proper voltage according to regulator type
	if (rdev->desc->vsel_mask == S2MPB02_BUCK_VSEL_MASK)
		ret = s2m_set_voltage_sel_regmap_buck(rdev, vol);
	else if (S2MPB02_LDO_VSEL_MASK)
		ret = s2m_set_voltage_sel_regmap(rdev, vol);

	return ret;
}

int s2mpb02_recovery(struct s2mpb02_data *s2mpb02)
{
	struct regulator_dev *rdev;
	int i, ret = 0;

	if (!s2mpb02) {
		pr_info("%s: There is no local rdev data\n", __func__);
		return -ENODEV;
	}

	pr_info("%s: Start recovery\n", __func__);

	// S2MPB02 Recovery
	for (i = 0; i < s2mpb02->num_regulators; i++) {
		rdev = s2mpb02->rdev[i];
		if (!rdev)
			continue;

		pr_info("%s: %s: max(%d), min(%d), aon(%d), use_count(%d)\n",
					__func__, rdev->desc->name, rdev->constraints->max_uV, rdev->desc->min_uV,
					rdev->constraints->always_on, rdev->use_count);

		// Make sure enabled registers are cleared
		s2m_disable_regmap(rdev);

		ret = s2mpb02_recovery_voltage(rdev);
		if (ret < 0)
			return ret;

		if (regulator_should_be_enabled(rdev)) {
			ret = s2m_enable(rdev);
			if (ret < 0)
				return ret;
		}
	}

	pr_info("%s: S2MPB02 is successfully recovered!\n", __func__);

	return ret;
}

static int s2mpb02_regulator_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct s2mpb02_data *s2mpb02 = platform_get_drvdata(pdev);
	int ret;

	if (s2mpb02->need_recovery) {
		pr_info("%s: Check recovery needs\n", __func__);
		ret = s2mpb02_need_recovery(s2mpb02);

		if (ret)
			s2mpb02_recovery(s2mpb02);
	}

	return 0;
}

const struct dev_pm_ops s2mpb02_regulator_pm = {
	.resume = s2mpb02_regulator_resume,
};
#endif

#if IS_ENABLED(CONFIG_OF)
static int s2mpb02_pmic_dt_parse_pdata(struct s2mpb02_dev *iodev,
					struct s2mpb02_platform_data *pdata)
{
	struct device_node *pmic_np, *regulators_np, *reg_np;
	struct s2mpb02_regulator_data *rdata;
	size_t i;

	pmic_np = iodev->dev->of_node;
	if (!pmic_np) {
		dev_err(iodev->dev, "could not find pmic sub-node\n");
		return -ENODEV;
	}

	regulators_np = of_find_node_by_name(pmic_np, "regulators");
	if (!regulators_np) {
		dev_err(iodev->dev, "could not find regulators sub-node\n");
		return -EINVAL;
	}

    pdata->need_recovery = of_property_read_bool(pmic_np, "s2mpb02,need_recovery");

	/* count the number of regulators to be supported in pmic */
	pdata->num_regulators = 0;
	for_each_child_of_node(regulators_np, reg_np) {
		pdata->num_regulators++;
	}

	rdata = devm_kzalloc(iodev->dev, sizeof(*rdata) *
				pdata->num_regulators, GFP_KERNEL);
	if (!rdata) {
		dev_err(iodev->dev,
			"could not allocate memory for regulator data\n");
		return -ENOMEM;
	}

	pdata->regulators = rdata;
	pdata->num_rdata = 0;
	for_each_child_of_node(regulators_np, reg_np) {
		for (i = 0; i < ARRAY_SIZE(regulators); i++)
			if (!of_node_cmp(reg_np->name, regulators[i].name))
				break;

		if (i == ARRAY_SIZE(regulators)) {
			dev_warn(iodev->dev,
			"don't know how to configure regulator %s\n",
			reg_np->name);
			continue;
		}

		rdata->id = i;
		rdata->initdata = of_get_regulator_init_data(iodev->dev, reg_np,
							     &regulators[i]);
		rdata->reg_node = reg_np;
		rdata++;
		pdata->num_rdata++;
	}
	of_node_put(regulators_np);

	return 0;
}
#else
static int s2mpb02_pmic_dt_parse_pdata(struct s2mpb02_dev *iodev,
					struct s2mpb02_platform_data *pdata)
{
	return 0;
}
#endif /* CONFIG_OF */
#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)
static ssize_t s2mpb02_read_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct s2mpb02_data *s2mpb02 = dev_get_drvdata(dev);
	int ret;
	u8 val, reg_addr;

	if (buf == NULL) {
		pr_info("%s: empty buffer\n", __func__);
		return -1;
	}

	ret = kstrtou8(buf, 0, &reg_addr);
	if (ret < 0)
		pr_info("%s: fail to transform i2c address\n", __func__);

	ret = s2mpb02_read_reg(s2mpb02->iodev->i2c, reg_addr, &val);
	if (ret < 0)
		pr_info("%s: fail to read i2c address\n", __func__);

	pr_info("%s: reg(0x%02hhx) data(0x%02hhx)\n", __func__, reg_addr, val);
	s2mpb02->read_addr = reg_addr;
	s2mpb02->read_val = val;

	return size;
}

static ssize_t s2mpb02_read_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct s2mpb02_data *s2mpb02 = dev_get_drvdata(dev);
	return sprintf(buf, "0x%02hhx: 0x%02hhx\n", s2mpb02->read_addr,
		       s2mpb02->read_val);
}

static ssize_t s2mpb02_write_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t size)
{
	struct s2mpb02_data *s2mpb02 = dev_get_drvdata(dev);
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

	ret = s2mpb02_write_reg(s2mpb02->iodev->i2c, reg, data);
	if (ret < 0)
		pr_info("%s: fail to write i2c addr/data\n", __func__);

	return size;
}

static ssize_t s2mpb02_write_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	return sprintf(buf, "echo (register addr.) (data) > s2mpb02_write\n");
}

#define ATTR_REGULATOR	(2)
static struct pmic_device_attribute regulator_attr[] = {
	PMIC_ATTR(s2mpb02_write, S_IRUGO | S_IWUSR, s2mpb02_write_show, s2mpb02_write_store),
	PMIC_ATTR(s2mpb02_read, S_IRUGO | S_IWUSR, s2mpb02_read_show, s2mpb02_read_store),
};

static int s2mpb02_create_sysfs(struct s2mpb02_data *s2mpb02)
{
	struct device *s2mpb02_pmic = s2mpb02->dev;
	struct device *dev = s2mpb02->iodev->dev;
	char device_name[32] = {0, };
	int err = -ENODEV, i = 0;

	pr_info("%s()\n", __func__);
	s2mpb02->read_addr = 0;
	s2mpb02->read_val = 0;

	/* Dynamic allocation for device name */
	snprintf(device_name, sizeof(device_name) - 1, "%s@%s",
		 dev_driver_string(dev), dev_name(dev));

	s2mpb02_pmic = pmic_device_create(s2mpb02, device_name);
	s2mpb02->dev = s2mpb02_pmic;

	/* Create sysfs entries */
	for (i = 0; i < ATTR_REGULATOR; i++) {
		err = device_create_file(s2mpb02_pmic, &regulator_attr[i].dev_attr);
		if (err)
			goto remove_pmic_device;
	}

	return 0;

remove_pmic_device:
	for (i--; i >= 0; i--)
		device_remove_file(s2mpb02_pmic, &regulator_attr[i].dev_attr);
	pmic_device_destroy(s2mpb02_pmic->devt);

	return -1;
}
#endif

#if IS_ENABLED(CONFIG_SEC_FACTORY)
/* 0x32:LDO_DSCH3, Bit[7~2]:unused */
#define VALID_REG	S2MPB02_REG_LDO_DSCH3	/* register address for validation */
#define VALID_MASK	0xFC		/* NA(unused) bit */

static int s2mpb02_verify_i2c_bus(struct s2mpb02_data* s2mpb02)
{
	u8 old_val, val;
	bool result = false;
	int ret;

	/* Checking S2MPB02 validation */
	/* 1. Write 1 to 0x32's bit[7~2] */
	ret = s2mpb02_read_reg(s2mpb02->iodev->i2c, VALID_REG, &val);
	if (ret < 0) {
		pr_info("%s: fail to read i2c address\n", __func__);
		return -ENXIO;
	}
	pr_info("%s: %s: init state: reg(0x%02x) data(0x%02x)\n",
				MFD_DEV_NAME, __func__, VALID_REG, val);
	old_val = val;

	ret = s2mpb02_write_reg(s2mpb02->iodev->i2c, VALID_REG, (old_val | VALID_MASK));
	if (ret < 0) {
		pr_info("%s: fail to write i2c address\n", __func__);
		return -ENXIO;
	}

	ret = s2mpb02_read_reg(s2mpb02->iodev->i2c, VALID_REG, &val);
	if (ret < 0) {
		pr_info("%s: fail to read i2c address\n", __func__);
		return -ENXIO;
	}
	pr_info("%s: %s: updated state: reg(0x%02x) data(0x%02x)\n",
				MFD_DEV_NAME, __func__, VALID_REG, val);

	if ((val & VALID_MASK) == VALID_MASK)
		result = true;

	if (!result) {
		pr_info("%s: %s: ERROR: state is not updated!: reg(0x%02x) data(0x%02x)\n",
			MFD_DEV_NAME, __func__, VALID_REG, val);
		return -ENODEV;
	}

	/* 2. Write 0 to 0x32's bit[7~2] */
	ret = s2mpb02_read_reg(s2mpb02->iodev->i2c, VALID_REG, &val);
	if (ret < 0) {
		pr_info("%s: fail to read i2c address\n", __func__);
		return -ENXIO;
	}
	pr_info("%s: %s: init state: reg(0x%02x) data(0x%02x)\n",
				MFD_DEV_NAME, __func__, VALID_REG, val);

	ret = s2mpb02_write_reg(s2mpb02->iodev->i2c, VALID_REG, (val & 0x03));
	if (ret < 0) {
		pr_info("%s: fail to write i2c address\n", __func__);
		return -ENXIO;
	}

	ret = s2mpb02_read_reg(s2mpb02->iodev->i2c, VALID_REG, &val);
	if (ret < 0) {
		pr_info("%s: fail to read i2c address\n", __func__);
		return -ENXIO;
	}
	pr_info("%s: %s: updated state: reg(0x%02x) data(0x%02x)\n",
				MFD_DEV_NAME, __func__, VALID_REG, val);

	if ((val & VALID_MASK) == 0x0)
		result = true;

	if (!result) {
		pr_info("%s: %s: ERROR: state is not updated!: reg(0x%02x) data(0x%02x)\n",
			MFD_DEV_NAME, __func__, VALID_REG, val);
		return -ENODEV;
	}

	/* 3. Restore old_val to 0x32's bit[7~2] */
	ret = s2mpb02_write_reg(s2mpb02->iodev->i2c, VALID_REG, old_val);
	if (ret < 0) {
		pr_info("%s: fail to write i2c address\n", __func__);
		return -ENXIO;
	}

	ret = s2mpb02_read_reg(s2mpb02->iodev->i2c, VALID_REG, &val);
	if (ret < 0) {
		pr_info("%s: fail to read i2c address\n", __func__);
		return -ENXIO;
	}

	if (old_val != val) {
		pr_info("%s: ERROR: old_val(0x%02x), val(0x%02x) are different!\n", __func__, old_val, val);
		return -EIO;
	}

	pr_info("%s: %s: restored value: reg(0x%02x) data(0x%02x)\n",
				MFD_DEV_NAME, __func__, VALID_REG, val);

	pr_err("%s: %s: i2c operation is %s\n", MFD_DEV_NAME, __func__,
						result ? "ok" : "not ok");

	return 0;
}
#endif

static int s2mpb02_pmic_probe(struct platform_device *pdev)
{
	struct s2mpb02_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct s2mpb02_platform_data *pdata = iodev->pdata;
	struct regulator_config config = { };
	struct s2mpb02_data *s2mpb02;
	struct i2c_client *i2c;
	u32 i;
	int ret;

	dev_info(&pdev->dev, "%s start\n", __func__);
	if (!pdata) {
		pr_info("[%s:%d] !pdata\n", __FILE__, __LINE__);
		dev_err(pdev->dev.parent, "No platform init data supplied.\n");
		return -ENODEV;
	}

	if (iodev->dev->of_node) {
		ret = s2mpb02_pmic_dt_parse_pdata(iodev, pdata);
		if (ret)
			goto err_pdata;
	}

	s2mpb02 = devm_kzalloc(&pdev->dev, sizeof(struct s2mpb02_data),
				GFP_KERNEL);
	if (!s2mpb02) {
		pr_info("[%s:%d] if (!s2mpb02)\n", __FILE__, __LINE__);
		return -ENOMEM;
	}

	s2mpb02->iodev = iodev;
	s2mpb02->num_regulators = pdata->num_rdata;
	s2mpb02->need_recovery = pdata->need_recovery;
	platform_set_drvdata(pdev, s2mpb02);
	i2c = s2mpb02->iodev->i2c;

#if IS_ENABLED(CONFIG_SEC_FACTORY)
	ret = s2mpb02_verify_i2c_bus(s2mpb02);
	if (ret < 0) {
		pr_err("%s: check_s2mpb02_validation fail\n", __func__);
		return -ENODEV;
	}
#endif

	for (i = 0; i < pdata->num_rdata; i++) {
		int id = pdata->regulators[i].id;
		config.dev = &pdev->dev;
		config.init_data = pdata->regulators[i].initdata;
		config.driver_data = s2mpb02;
		config.of_node = pdata->regulators[i].reg_node;
		s2mpb02->rdev[i] = devm_regulator_register(&pdev->dev,
							   &regulators[id], &config);
		if (IS_ERR(s2mpb02->rdev[i])) {
			ret = PTR_ERR(s2mpb02->rdev[i]);
			dev_err(&pdev->dev, "regulator init failed for %d\n", i);
			s2mpb02->rdev[i] = NULL;
			goto err_s2mpb02_data;
		}
#if IS_ENABLED(CONFIG_REGULATOR_DEBUG_CONTROL)
		ret = devm_regulator_debug_register(&pdev->dev, s2mpb02->rdev[i]);
		if (ret)
			dev_err(&pdev->dev, "failed to register debug regulator for %d, rc=%d\n",
					i, ret);
#endif
	}
#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)
	ret = s2mpb02_create_sysfs(s2mpb02);
	if (ret < 0) {
		pr_err("%s: s2mpb02_create_sysfs fail\n", __func__);
		return -ENODEV;
	}
#endif
	dev_info(&pdev->dev, "%s end\n", __func__);

	return 0;

err_s2mpb02_data:
err_pdata:
	return ret;
}

static int s2mpb02_pmic_remove(struct platform_device *pdev)
{
#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)
	struct s2mpb02_data *s2mpb02 = platform_get_drvdata(pdev);
	struct device *s2mpb02_pmic = s2mpb02->dev;
	int i = 0;

	dev_info(&pdev->dev, "%s\n", __func__);

	/* Remove sysfs entries */
	for (i = 0; i < ATTR_REGULATOR; i++)
		device_remove_file(s2mpb02_pmic, &regulator_attr[i].dev_attr);
	pmic_device_destroy(s2mpb02_pmic->devt);
#else
	dev_info(&pdev->dev, "%s\n", __func__);
#endif
	return 0;
}

static const struct platform_device_id s2mpb02_pmic_id[] = {
	{"s2mpb02-regulator", TYPE_S2MPB02_REG_MAIN},
	{"s2mpb02-sub-reg", TYPE_S2MPB02_REG_SUB},
	{},
};
MODULE_DEVICE_TABLE(platform, s2mpb02_pmic_id);

static struct platform_driver s2mpb02_pmic_driver = {
	.driver = {
		   .name = "s2mpb02-regulator",
		   .owner = THIS_MODULE,
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		   .pm = &s2mpb02_regulator_pm,
#endif
		   .suppress_bind_attrs = true,
		   },
	.probe = s2mpb02_pmic_probe,
	.remove = s2mpb02_pmic_remove,
	.id_table = s2mpb02_pmic_id,
};

static int __init s2mpb02_pmic_init(void)
{
	return platform_driver_register(&s2mpb02_pmic_driver);
}
subsys_initcall(s2mpb02_pmic_init);

static void __exit s2mpb02_pmic_cleanup(void)
{
	platform_driver_unregister(&s2mpb02_pmic_driver);
}
module_exit(s2mpb02_pmic_cleanup);

MODULE_DESCRIPTION("SAMSUNG s2mpb02 Regulator Driver");
MODULE_LICENSE("GPL");
