/*
 * s2dos07.h
 *
 * Copyright (c) 2023 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef __LINUX_MFD_S2DOS07_H
#define __LINUX_MFD_S2DOS07_H
#include <linux/platform_device.h>
#include <linux/regmap.h>

#define MFD_DEV_NAME "s2dos07"

/**
 * sec_regulator_data - regulator data
 * @id: regulator id
 * @initdata: regulator init data (contraints, supplies, ...)
 */

struct s2dos07_dev {
	struct device *dev;
	struct i2c_client *i2c;
	struct mutex i2c_lock;

	int type;
	u8 rev_num; /* pmic Rev */
	bool wakeup;
	bool vgl_bypass_n_fd;
	int dp_pmic_irq;

	struct s2dos07_platform_data *pdata;
#if IS_ENABLED(CONFIG_SEC_PM)
	struct device *sec_disp_pmic_dev;
#endif /* CONFIG_SEC_PM */

	bool scp_info_supproted;
};

struct s2dos07_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *reg_node;
};

struct s2dos07_platform_data {
	bool wakeup;
	bool vgl_bypass_n_fd;
	int num_regulators;
	int num_rdata;
	struct	s2dos07_regulator_data *regulators;
	int	device_type;
	int dp_pmic_irq;
};

struct s2dos07 {
	struct regmap *regmap;
};

/* S2DOS07 registers */
/* Slave Addr : 0xC0 */
enum S2DOS07_reg {
	S2DOS07_REG_STAT = 0x02,
	S2DOS07_REG_BUCK_VOUT = 0x03,
	S2DOS07_REG_BUCK_EN = 0x04,
	S2DOS07_REG_IRQ_MASK = 0x06,
	S2DOS07_REG_IRQ = 0x07,
	S2DOS07_REG_UVP_STATUS,
	S2DOS07_REG_OVP_STATUS,
	S2DOS07_REG_OVP_MODE,

	S2DOS07_REG_DEVICE_ID = 0x0B,

	S2DOS07_REG_0D_AUTHORITY = 0x0C,
	S2DOS07_REG_0D_CONTROL = 0x0D,

	S2DOS07_REG_SCP_STATUS = 0xE9,

	S2DOS07_REG_EN_CTRL = 0x20,
	S2DOS07_REG_SS_FD_CTRL = 0x21,
	S2DOS07_REG_VGX_EN_CTRL = 0x26,
};

/* S2DOS07 regulator ids */
enum S2DOS07_regulators {
	S2DOS07_BUCK1,

#if IS_ENABLED(CONFIG_SEC_PM)
	S2DOS07_ELVXX,
#endif /* CONFIG_SEC_PM */

	S2DOS07_REG_MAX,
};

#define S2DOS07_IRQ_SCP_MASK	(1 << 6)
#define S2DOS07_IRQ_UVP_MASK	(1 << 5)
#define S2DOS07_IRQ_OVP_MASK	(1 << 4)
#define S2DOS07_IRQ_PRETSD_MASK	(1 << 3)
#define S2DOS07_IRQ_TSD_MASK	(1 << 2)
#define S2DOS07_IRQ_SSD_MASK	(1 << 1)
#define S2DOS07_IRQ_UVLO_MASK	(1 << 0)

#define S2DOS07_BUCK_MIN1	512500
#define S2DOS07_BUCK_STEP1	12500
#define S2DOS07_BUCK_VSEL_MASK	0x7F

#define S2DOS07_ENABLE_MASK_B1	(1 << 0)

#define S2DOS07_RAMP_DELAY	4000

#define S2DOS07_ENABLE_TIME_BUCK	350

#define S2DOS07_BUCK_N_VOLTAGES (S2DOS07_BUCK_VSEL_MASK + 1)

#define S2DOS07_REGULATOR_MAX (S2DOS07_REG_MAX)

/* S2DOS07 shared i2c API function */
extern int s2dos07_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest);
extern int s2dos07_write_reg(struct i2c_client *i2c, u8 reg, u8 value);
extern int s2dos07_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask);

#if IS_ENABLED(CONFIG_SEC_KUNIT)
extern int s2m_set_voltage_time_sel(struct regulator_dev *rdev,
					unsigned int old_selector,
					unsigned int new_selector);

extern struct regulator_desc regulators[];
#endif

#endif /*  __LINUX_MFD_S2DOS07_H */
