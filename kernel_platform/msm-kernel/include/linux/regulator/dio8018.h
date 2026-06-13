/* SPDX-License-Identifier: GPL-2.0 */
/*
 * dio8018.h - Regulator driver for the DIOO DIO8018
 *
 * Copyright (c) 2023 DIOO Microciurcuits Co., Ltd Jiangsu
 *
 */

#ifndef __LINUX_REGULATOR_DIO8018_H
#define __LINUX_REGULATOR_DIO8018_H
#include <linux/platform_device.h>
#include <linux/regmap.h>

#define DIO8018_DEV_NAME "dio8018"

/**
 * sec_regulator_data - regulator data
 * @id: regulator id
 * @initdata: regulator init data (contraints, supplies, ...)
 */

struct dio8018_dev {
	struct device *dev;
	struct i2c_client *i2c;
	struct mutex i2c_lock;

	int type;
	u8 rev_num; /* pmic Rev */
	bool wakeup;

	int	pmic_irq;

	struct dio8018_platform_data *pdata;
};

struct dio8018_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *reg_node;
};

struct dio8018_platform_data {
	bool wakeup;
	bool need_self_recovery;
	int num_regulators;
	int num_rdata;
	struct	dio8018_regulator_data *regulators;
	int	device_type;
	int pmic_irq_gpio;
	u32 pmic_irq_level_sel;
	u32 pmic_irq_outmode_sel;
	int pmic_reset_gpio;
	bool need_hw_reset;
	bool need_sw_reset;
};


/* dio8018 registers */
/* Slave Addr : 0xAC */
enum DIO8018_reg {
	DIO8018_REG_PRODUCT_ID,
	DIO8018_REG_REV_ID,
	DIO8018_REG_IOUT,
	DIO8018_REG_ENABLE,
	DIO8018_REG_LDO1_VOUT,
	DIO8018_REG_LDO2_VOUT,
	DIO8018_REG_LDO3_VOUT,
	DIO8018_REG_LDO4_VOUT,
	DIO8018_REG_LDO5_VOUT,
	DIO8018_REG_LDO6_VOUT,
	DIO8018_REG_LDO7_VOUT,
	DIO8018_REG_LDO12_SEQ,
	DIO8018_REG_LDO34_SEQ,
	DIO8018_REG_LDO56_SEQ,
	DIO8018_REG_LDO7_SEQ,
	DIO8018_REG_SEQUENCEING,
	DIO8018_REG_DISCHARGE,
	DIO8018_REG_RESET,
	DIO8018_REG_I2C_ADDR,
	DIO8018_REG_REV1,
	DIO8018_REG_REV2,
	DIO8018_REG_INTRRUPT1,
	DIO8018_REG_INTRRUPT2,
	DIO8018_REG_INTRRUPT3,
	DIO8018_REG_STATUS1,
	DIO8018_REG_STATUS2,
	DIO8018_REG_STATUS3,
	DIO8018_REG_STATUS4,
	DIO8018_REG_MINT1,
	DIO8018_REG_MINT2,
	DIO8018_REG_MINT3,
};

/* S2MPB03 regulator ids */
enum DIO8018_regulators {
	DIO8018_LDO1,
	DIO8018_LDO2,
	DIO8018_LDO3,
	DIO8018_LDO4,
	DIO8018_LDO5,
	DIO8018_LDO6,
	DIO8018_LDO7,
	DIO8018_LDO_MAX,
};

#define DIO8018_FLT_SD_B		1 /* 1: Prevents shutdown when a fault occurs */

#define DIO8018_LDO_VSEL_MASK	0xFF
/* Ramp delay in uV/us */
// 1.2v * 0.95 / 240 us
#define DIO8018_RAMP_DELAY1		4750
// 2.8v * 0.95 / 240 us
#define DIO8018_RAMP_DELAY2		11083

#define DIO8018_ENABLE_TIME_LDO		25

#define DIO8018_REGULATOR_MAX (DIO8018_LDO_MAX)

#if IS_ENABLED(CONFIG_SEC_KUNIT)
extern int dio8018_ldo_current_check(struct regulator_dev *rdev, int min_uA, int max_uA);
extern struct regulator_desc *dio8018_regulators;
#endif

#endif	/*  __LINUX_MFD_DIO8018_H */

