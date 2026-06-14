/*
 * max77775.h - Driver for the Maxim 77775
 *
 *  Copyright (C) 2016 Samsung Electrnoics
 *  Insun Choi <insun77.choi@samsung.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * This driver is based on max8997.h
 *
 * MAX77775 has Flash LED, SVC LED, Haptic, MUIC devices.
 * The devices share the same I2C bus and included in
 * this mfd driver.
 */

#ifndef __MAX77775_H__
#define __MAX77775_H__
#include <linux/platform_device.h>
#include <linux/regmap.h>

#define MFD_DEV_NAME "max77775"
#define M2SH(m) ((m) & 0x0F ? ((m) & 0x03 ? ((m) & 0x01 ? 0 : 1) : ((m) & 0x04 ? 2 : 3)) : \
		((m) & 0x30 ? ((m) & 0x10 ? 4 : 5) : ((m) & 0x40 ? 6 : 7)))

struct max77775_vibrator_pdata {
	int gpio;
	char *regulator_name;
	struct pwm_device *pwm;
	const char *motor_type;

	int freq;
	/* for multi-frequency */
	int freq_nums;
	u32 *freq_array;
	u32 *ratio_array; /* not used now */
	int normal_ratio;
	int overdrive_ratio;
	int high_temp_ratio;
	int high_temp_ref;
	int fold_open_ratio;
	int fold_close_ratio;
#if defined(CONFIG_SEC_VIBRATOR)
	bool calibration;
	int steps;
	int *intensities;
	int *haptic_intensities;
#endif
};

struct max77775_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *reg_node;
};

struct max77775_platform_data {
	/* IRQ */
	int irq_base;
	int irq_gpio;
	bool wakeup;
	bool blocking_waterevent;
	bool extra_fw_enable;
	int wpc_en;
	u32 rev;
	u32 fw_product_id;
	u32 fg_resistor;
	struct muic_platform_data *muic_pdata;

	int num_regulators;
	struct max77775_regulator_data *regulators;
	struct max77775_vibrator_pdata *vibrator_data;
	struct mfd_cell *sub_devices;
	int num_subdevs;
	bool support_audio;
	char *wireless_charger_name;
};

struct max77775 {
	struct regmap *regmap;
};

typedef struct {
	u32 magic;     /* magic number */
	u8 major;         /* major version */
	u8 minor;       /* minor version */
	u8 id;            /* id */
	u8 rev;           /* rev */
} max77775_fw_header;
#define MAX77775_SIGN 0xCEF166C1

enum {
	FW_UPDATE_START = 0x00,
	FW_UPDATE_WAIT_RESP_START,
	FW_UPDATE_WAIT_RESP_STOP,
	FW_UPDATE_DOING,
	FW_UPDATE_END,
};

enum {
	FW_UPDATE_FAIL = 0xF0,
	FW_UPDATE_I2C_FAIL,
	FW_UPDATE_TIMEOUT_FAIL,
	FW_UPDATE_VERIFY_FAIL,
	FW_UPDATE_CMD_FAIL,
	FW_UPDATE_MAX_LENGTH_FAIL,
	FW_UPDATE_ERR_CODE_FAIL,
};

#endif /* __MAX77775_H__ */

