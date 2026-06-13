// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015-2017, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/regmap.h>
#include <linux/module.h>
#include <linux/init.h>
#include <soc/soundwire.h>

#define ADDR_BYTES                (2)
#define ADDR_BYTES_4              (4)
#define VAL_BYTES                 (1)
#define PAD_BYTES                 (0)
#define SCP1_ADDRESS_VAL_MASK     (0x7f800000)
#define SCP2_ADDRESS_VAL_MASK     (0x007f8000)
#define BIT_WIDTH_CHECK_MASK      (0xffff0000)
#define SCP1_ADDRESS_VAL_SHIFT    (23)
#define SCP2_ADDRESS_VAL_SHIFT    (15)
#define SCP1_ADDRESS              (0X48)
#define SCP2_ADDRESS              (0X49)
#define SDCA_READ_WRITE_BIT       (0x8000)
static DEFINE_MUTEX(swr_rw_lock);

static int regmap_swr_reg_address_get(struct swr_device *swr,
			u16 *reg_addr, const void *reg, size_t reg_size)
{
	u8 scp1_val = 0, scp2_val = 0;
	u32 temp = 0;
	int ret = 0;

	if (reg_size == ADDR_BYTES_4) {
		temp = (*(u32 *)reg) & SCP1_ADDRESS_VAL_MASK;
		scp1_val = temp >> SCP1_ADDRESS_VAL_SHIFT;

		temp = (*(u32 *)reg) & SCP2_ADDRESS_VAL_MASK;
		scp2_val = temp >> SCP2_ADDRESS_VAL_SHIFT;

		if (scp1_val || scp2_val) {
			if (scp1_val != swr->g_scp1_val) {
				ret = swr_write(swr, swr->dev_num, SCP1_ADDRESS, &scp1_val);
				if (ret < 0) {
					dev_err(&swr->dev, "%s: write reg scp1_address failed, err %d\n",
						__func__, ret);
					return ret;
				}
				swr->g_scp1_val = scp1_val;
			}

			if (scp2_val != swr->g_scp2_val) {
				ret = swr_write(swr, swr->dev_num, SCP2_ADDRESS, &scp2_val);
				if (ret < 0) {
					dev_err(&swr->dev, "%s: write reg scp2_address failed, err %d\n",
					__func__, ret);
					return ret;
				}
				swr->g_scp2_val = scp2_val;
			}
			*reg_addr = (*(u16 *)reg | SDCA_READ_WRITE_BIT);
			dev_dbg(&swr->dev, "%s: reg: 0x%x, scp1_val: 0x%x, scp2_val: 0x%x, reg_addr: 0x%x\n",
				__func__, *(u32 *)reg, scp1_val, scp2_val, *reg_addr);
		} else {
			*reg_addr = *(u16 *)reg;
		}
	} else {
		*reg_addr = *(u16 *)reg;
	}

	return ret;
}

static int regmap_swr_gather_write(void *context,
				const void *reg, size_t reg_size,
				const void *val, size_t val_len)
{
	struct device *dev = context;
	struct swr_device *swr = to_swr_device(dev);
	struct regmap *map = dev_get_regmap(dev, NULL);
	int i, ret = 0;
	u16 reg_addr = 0;
	u8 *value;

	if (map == NULL) {
		dev_err_ratelimited(dev, "%s: regmap is NULL\n", __func__);
		return -EINVAL;
	}

	if (swr == NULL) {
		dev_err_ratelimited(dev, "%s: swr device is NULL\n", __func__);
		return -EINVAL;
	}

	if ((reg_size != ADDR_BYTES) && (reg_size != ADDR_BYTES_4)) {
		dev_err_ratelimited(dev, "%s: reg size %zd bytes not supported\n",
			__func__, reg_size);
		return -EINVAL;
	}

	mutex_lock(&swr_rw_lock);
	ret = regmap_swr_reg_address_get(swr, &reg_addr, reg, reg_size);
	if (ret < 0) {
		mutex_unlock(&swr_rw_lock);
		return ret;
	}

	/* val_len = VAL_BYTES * val_count */
	for (i = 0; i < (val_len / VAL_BYTES); i++) {
		value = (u8 *)val + (VAL_BYTES * i);
		ret = swr_write(swr, swr->dev_num, (reg_addr + i), value);
		if (ret < 0) {
			dev_err_ratelimited(dev, "%s: write reg 0x%x failed, err %d\n",
				__func__, (reg_addr + i), ret);
			break;
		}
		dev_dbg(dev, "%s: dev_num: 0x%x, gather write reg: 0x%x, value: 0x%x\n",
				__func__, swr->dev_num, (reg_addr + i), *value);
	}
	mutex_unlock(&swr_rw_lock);
	return ret;
}

static int regmap_swr_raw_multi_reg_write(void *context, const void *data,
					  size_t count)
{
	struct device *dev = context;
	struct swr_device *swr = to_swr_device(dev);
	struct regmap *map = dev_get_regmap(dev, NULL);
	size_t num_regs;
	int i = 0;
	int ret = 0;
	u16 *reg;
	u8 *val;
	u8 *buf;

	if (swr == NULL) {
		dev_err_ratelimited(dev, "%s: swr device is NULL\n", __func__);
		return -EINVAL;
	}

	if (map == NULL) {
		dev_err_ratelimited(dev, "%s: regmap is NULL\n", __func__);
		return -EINVAL;
	}

	if (ADDR_BYTES + VAL_BYTES + PAD_BYTES == 0) {
		dev_err_ratelimited(dev, "%s: sum of addr, value and pad is 0\n", __func__);
		return -EINVAL;
	}
	num_regs = count / (ADDR_BYTES + VAL_BYTES + PAD_BYTES);

	reg = kcalloc(num_regs, sizeof(u16), GFP_KERNEL);
	if (!reg)
		return -ENOMEM;

	val = kcalloc(num_regs, sizeof(u8), GFP_KERNEL);
	if (!val) {
		ret = -ENOMEM;
		goto mem_fail;
	}

	buf = (u8 *)data;
	for (i = 0; i < num_regs; i++) {
		reg[i] = *(u16 *)buf;
		buf += (ADDR_BYTES + PAD_BYTES);
		val[i] = *buf;
		buf += VAL_BYTES;
	}
	ret = swr_bulk_write(swr, swr->dev_num, reg, val, num_regs);
	if (ret)
		dev_err_ratelimited(dev, "%s: multi reg write failed\n", __func__);

	kfree(val);
mem_fail:
	kfree(reg);
	return ret;
}

static int regmap_swr_write(void *context, const void *data, size_t count)
{
	struct device *dev = context;
	struct swr_device *swr = to_swr_device(dev);
	struct regmap *map = dev_get_regmap(dev, NULL);
	int addr_bytes = 0;

	if (map == NULL) {
		dev_err_ratelimited(dev, "%s: regmap is NULL\n", __func__);
		return -EINVAL;
	}

	if (swr == NULL) {
		dev_err_ratelimited(dev, "%s: swr is NULL\n", __func__);
		return -EINVAL;
	}

	addr_bytes = (swr->paging_support ? ADDR_BYTES_4 : ADDR_BYTES);

	WARN_ON(count < addr_bytes);

	if (count > (addr_bytes + VAL_BYTES + PAD_BYTES))
		return regmap_swr_raw_multi_reg_write(context, data, count);
	else
		return regmap_swr_gather_write(context, data, addr_bytes,
					       (data + addr_bytes),
					       (count - addr_bytes));
}

static int regmap_swr_read(void *context,
			const void *reg, size_t reg_size,
			void *val, size_t val_size)
{
	struct device *dev = context;
	struct swr_device *swr = to_swr_device(dev);
	struct regmap *map = dev_get_regmap(dev, NULL);
	int ret = 0;
	u16 reg_addr = 0;

	if (map == NULL) {
		dev_err_ratelimited(dev, "%s: regmap is NULL\n", __func__);
		return -EINVAL;
	}
	if (swr == NULL) {
		dev_err_ratelimited(dev, "%s: swr is NULL\n", __func__);
		return -EINVAL;
	}

	if ((reg_size != ADDR_BYTES) && (reg_size != ADDR_BYTES_4)) {
		dev_err_ratelimited(dev, "%s: reg size %zd bytes not supported\n",
			__func__, reg_size);
		return -EINVAL;
	}

	mutex_lock(&swr_rw_lock);
	ret = regmap_swr_reg_address_get(swr, &reg_addr, reg, reg_size);
	if (ret < 0) {
		dev_err_ratelimited(dev,
			"%s: regmap_swr_reg_address_get failed, reg: 0x%x\n",
					__func__, *(u32 *)reg);
		mutex_unlock(&swr_rw_lock);
		return ret;
	}

	ret = swr_read(swr, swr->dev_num, reg_addr, val, val_size);
	if (ret < 0)
		dev_err_ratelimited(dev, "%s: codec reg 0x%x read failed %d\n",
			__func__, reg_addr, ret);

	mutex_unlock(&swr_rw_lock);
	return ret;
}

static struct regmap_bus regmap_swr = {
	.write = regmap_swr_write,
	.gather_write = regmap_swr_gather_write,
	.read = regmap_swr_read,
	.reg_format_endian_default = REGMAP_ENDIAN_NATIVE,
	.val_format_endian_default = REGMAP_ENDIAN_NATIVE,
};

struct regmap *__regmap_init_swr(struct swr_device *swr,
				 const struct regmap_config *config,
				 struct lock_class_key *lock_key,
				 const char *lock_name)
{
	return __regmap_init(&swr->dev, &regmap_swr, &swr->dev, config,
			   lock_key, lock_name);
}
EXPORT_SYMBOL(__regmap_init_swr);

struct regmap *__devm_regmap_init_swr(struct swr_device *swr,
				      const struct regmap_config *config,
				      struct lock_class_key *lock_key,
				      const char *lock_name)
{
	return __devm_regmap_init(&swr->dev, &regmap_swr, &swr->dev, config,
				lock_key, lock_name);
}
EXPORT_SYMBOL(__devm_regmap_init_swr);

MODULE_LICENSE("GPL v2");
