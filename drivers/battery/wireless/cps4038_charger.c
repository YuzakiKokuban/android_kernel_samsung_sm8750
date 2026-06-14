/*
 *  cps4038_charger.c
 *  Samsung CPS4038 IC Charger Driver
 *
 *  Copyright (C) 2022 Samsung Electronics
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "cps4038_charger.h"
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/ctype.h>
#include <linux/firmware.h>
#if IS_ENABLED(CONFIG_SPU_VERIFY)
#include <linux/spu-verify.h>
#endif

#include "cps4038_firmware.h"

#define ENABLE 1
#define DISABLE 0
#define CMD_CNT 3
#define ISR_CNT 10
#define MAX_I2C_ERROR_COUNT		30
#define MAX_GPIO_IRQ_MISSING_COUNT	20
#define MAX_MTP_PGM_CNT 3

#define MAX_BUF 4095
#define SENDSZ 16

#define VALID_VRECT_LEVEL	2700

#if defined(CONFIG_MST_PCR)
static bool use_pcr_fix_mode;
#endif

static char * __read_mostly carrierid;
module_param(carrierid, charp, 0444);

#if defined(CONFIG_WIRELESS_IC_PARAM)
static unsigned int __read_mostly wireless_ic;
module_param(wireless_ic, uint, 0444);
#endif

static u8 ADT_buffer_rdata[MAX_BUF] = {0, };
static int adt_readSize;
static bool is_shutdn;

typedef struct _sgf_data {
	unsigned int	size;
	unsigned int	type;
	char			*data;
} sgf_data;

static const u16 mfc_cps_vout_val16[] = {
	0x006F, /* MFC_VOUT_4_5V */
	0x0079, /* MFC_VOUT_4_7V */
	0x007E, /* MFC_VOUT_4_8V */
	0x0083, /* MFC_VOUT_4_9V */
	0x0088, /* MFC_VOUT_5V */
	0x00A1, /* MFC_VOUT_5_5V */
	0x00BA, /* MFC_VOUT_6V */
	0x00EC, /* MFC_VOUT_7V */
	0x011E, /* MFC_VOUT_8V */
	0x0150, /* MFC_VOUT_9V */
	0x0182, /* MFC_VOUT_10V */
	0x01B4, /* MFC_VOUT_11V */
	0x01E6, /* MFC_VOUT_12V */
	0x01ff, /* MFC_VOUT_12_5V */
	0x0079, /* MFC_VOUT_OTG, 4.7V */
	0x0218, /* MFC_VOUT_13V */
};

static struct device_attribute mfc_attrs[] = {
	CPS4038_ATTR(mfc_addr),
	CPS4038_ATTR(mfc_size),
	CPS4038_ATTR(mfc_data),
	CPS4038_ATTR(mfc_packet),
	CPS4038_ATTR(mfc_epp_t),
};

static enum power_supply_property mfc_charger_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static irqreturn_t mfc_wpc_det_irq_thread(int irq, void *irq_data);
static irqreturn_t mfc_wpc_irq_thread(int irq, void *irq_data);
static int mfc_reg_multi_write_verify(struct i2c_client *client, u16 reg, const u8 *val, int size);
static void mfc_set_tx_fod_thresh1(struct i2c_client *client, u32 fod_thresh1);
static void mfc_set_tx_fod_ta_thresh(struct i2c_client *client, u32 fod_thresh);
static int mfc_cps_wls_write_word(struct i2c_client *client, u32 reg, u32 val);
static int mfc_cps_wls_read_word(struct i2c_client *client, u32 reg, u8 *val);
static void mfc_deactivate_work_content(struct mfc_charger_data *charger);
static void mfc_mpp_epp_nego_done(struct mfc_charger_data *charger);

#if defined(CONFIG_WIRELESS_IC_PARAM)
static unsigned int mfc_get_wrlic(void) { return wireless_ic; }
#endif

static bool carrierid_is(char *str)
{
	if (carrierid == NULL)
		return false;

	pr_info("%s: %s\n", __func__, carrierid);

	return !strncmp(carrierid, str, 3);
}

static int mfc_get_volt(u32 vout)
{
	switch(vout) {
	case WIRELESS_VOUT_5V:
		return 5;
	case WIRELESS_VOUT_9V:
		return 9;
	case WIRELESS_VOUT_10V:
		return 10;
	case WIRELESS_VOUT_11V:
		return 11;
	case WIRELESS_VOUT_12V:
		return 12;
	case WIRELESS_VOUT_13V:
		return 13;
	default:
		return 1;
	}
}

static int cps4038_get_op_mode(void *pdata)
{
	struct mfc_charger_data *charger = pdata;
	int wc_opmode = WPC_OP_MODE_NONE;

	if (!charger->det_state)
		return wc_opmode;

	switch (charger->rx_op_mode) {
	case MFC_RX_MODE_WPC_BPP:
		wc_opmode = is_ppde_wireless_type(charger->pdata->cable_type) ? WPC_OP_MODE_PPDE : WPC_OP_MODE_BPP;
		break;
	case MFC_RX_MODE_WPC_EPP:
	case MFC_RX_MODE_WPC_EPP_NEGO:
		wc_opmode = WPC_OP_MODE_EPP;
		break;
	case MFC_RX_MODE_WPC_MPP_RESTRICT:
	case MFC_RX_MODE_WPC_MPP_FULL:
	case MFC_RX_MODE_WPC_MPP_CLOAK:
	case MFC_RX_MODE_WPC_MPP_NEGO:
		wc_opmode = charger->pdata->mpp_disable ? WPC_OP_MODE_EPP : WPC_OP_MODE_MPP;
		break;
	}
	return wc_opmode;
}

static bool cps4038_is_mpp_op_mode(struct mfc_charger_data *charger)
{
	bool ret = false;

	switch (charger->rx_op_mode) {
	case MFC_RX_MODE_WPC_MPP_RESTRICT:
	case MFC_RX_MODE_WPC_MPP_FULL:
	case MFC_RX_MODE_WPC_MPP_CLOAK:
	case MFC_RX_MODE_WPC_MPP_NEGO:
		ret = true;
		// Fall through
		break;
	default:
		ret = false;
	}
	return ret;
}

static int cps4038_get_qi_ver(void *pdata)
{
	struct mfc_charger_data *charger = pdata;

	if (!charger->det_state)
		return 0;

	return (charger->mpp_epp_tx_id >> 16);
}

static int cps4038_get_auth_mode(void *pdata)
{
	struct mfc_charger_data *charger = pdata;

	pr_info("%s: det_state(%d) rx_op_mode(%d)\n",
				__func__, charger->det_state, charger->rx_op_mode);

	if (!charger->det_state)
		return WPC_AUTH_MODE_NONE;

	if (mpp_mode(charger->rx_op_mode))
		return WPC_AUTH_MODE_MPP;

	if (is_ppde_wireless_type(charger->pdata->cable_type))
		return WPC_AUTH_MODE_PPDE;

	if (epp_mode(charger->rx_op_mode))
		return WPC_AUTH_MODE_EPP;

	return WPC_AUTH_MODE_BPP;
}

static const struct sb_wireless_op cps4038_sbw_op = {
	.get_op_mode = cps4038_get_op_mode,
	.get_qi_ver = cps4038_get_qi_ver,
	.get_auth_mode = cps4038_get_auth_mode,
};

static void mfc_check_i2c_error(struct mfc_charger_data *charger, bool is_error)
{
	u8 wpc_det = 0;
	u8 wpc_pdrc = 0;
	u8 wpc_pdet_b = 0;

	if (charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP_FAKE)
		return;

	if (!is_error) {
		charger->i2c_error_count = 0;
		charger->gpio_irq_missing_wa_cnt = 0;
		return;
	}

	wpc_det = gpio_get_value(charger->pdata->wpc_det);
	wpc_pdrc = gpio_get_value(charger->pdata->wpc_pdrc);
	wpc_pdet_b = gpio_get_value(charger->pdata->wpc_pdet_b);

	charger->i2c_error_count =
		(charger->det_state && wpc_det) ?
		(charger->i2c_error_count + 1) : 0;

	if (charger->i2c_error_count > MAX_I2C_ERROR_COUNT) {
		charger->i2c_error_count = 0;
		queue_delayed_work(charger->wqueue, &charger->wpc_i2c_error_work, 0);
	}

	/* gpio irq missing W/A */
	if (wpc_det || !wpc_pdrc || charger->pdata->cable_type == SEC_BATTERY_CABLE_NONE ||
			(charger->rx_phm_status && !wpc_pdet_b)) {
		charger->gpio_irq_missing_wa_cnt = 0;
	} else {
		charger->gpio_irq_missing_wa_cnt++;
		pr_info("%s: gpio irq missing W/A(%d), det(%d), pdrc(%d), ct(%d), phm(%d), pdet_b(%d)\n",
				__func__, charger->gpio_irq_missing_wa_cnt, wpc_det, wpc_pdrc,
				charger->pdata->cable_type, charger->rx_phm_status, wpc_pdet_b);

		if (charger->gpio_irq_missing_wa_cnt > MAX_GPIO_IRQ_MISSING_COUNT) {
			charger->gpio_irq_missing_wa_cnt = 0;
			__pm_stay_awake(charger->wpc_det_ws);
			queue_delayed_work(charger->wqueue,
				&charger->wpc_deactivate_work, msecs_to_jiffies(0));
		}
	}
}

static bool is_no_hv(struct mfc_charger_data *charger)
{
	return (charger->pdata->no_hv == 1);
}

static int mfc_reg_read(struct i2c_client *client, u16 reg, u8 *val)
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);
	int ret;
	struct i2c_msg msg[2];
	u8 wbuf[2];
	u8 rbuf[2];

	if (charger->reg_access_lock) {
		pr_err("%s: can not access to reg during fw update\n", __func__);
		return -1;
	}

	msg[0].addr = client->addr;
	msg[0].flags = client->flags & I2C_M_TEN;
	msg[0].len = 2;
	msg[0].buf = wbuf;

	wbuf[0] = (reg & 0xFF00) >> 8;
	wbuf[1] = (reg & 0xFF);

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = rbuf;

	mutex_lock(&charger->io_lock);
	ret = i2c_transfer(client->adapter, msg, 2);
	mfc_check_i2c_error(charger, (ret < 0));
	mutex_unlock(&charger->io_lock);
	if (ret < 0) {
		pr_err("%s: i2c read error, reg: 0x%x, ret: %d (called by %ps)\n",
			__func__, reg, ret, __builtin_return_address(0));
		return -1;
	}

	*val = rbuf[0];

	return ret;
}

static int mfc_reg_multi_read(struct i2c_client *client, u16 reg, u8 *val, int size)
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);
	int ret;
	struct i2c_msg msg[2];
	u8 wbuf[2];

	if (charger->reg_access_lock) {
		pr_err("%s: can not access to reg during fw update\n", __func__);
		return -1;
	}

	msg[0].addr = client->addr;
	msg[0].flags = client->flags & I2C_M_TEN;
	msg[0].len = 2;
	msg[0].buf = wbuf;

	wbuf[0] = (reg & 0xFF00) >> 8;
	wbuf[1] = (reg & 0xFF);

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = size;
	msg[1].buf = val;

	mutex_lock(&charger->io_lock);
	ret = i2c_transfer(client->adapter, msg, 2);
	mfc_check_i2c_error(charger, (ret < 0));
	mutex_unlock(&charger->io_lock);
	if (ret < 0) {
		pr_err("%s: i2c transfer fail", __func__);
		return -1;
	}

	return ret;
}

static int mfc_reg_write(struct i2c_client *client, u16 reg, u8 val)
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);
	int ret;
	unsigned char data[3] = { reg >> 8, reg & 0xff, val };

	if (charger->reg_access_lock) {
		pr_err("%s: can not access to reg during fw update\n", __func__);
		return -1;
	}

	mutex_lock(&charger->io_lock);
	ret = i2c_master_send(client, data, 3);
	mfc_check_i2c_error(charger, (ret < 3));
	mutex_unlock(&charger->io_lock);
	if (ret < 3) {
		pr_err("%s: i2c write error, reg: 0x%x, ret: %d (called by %ps)\n",
				__func__, reg, ret, __builtin_return_address(0));
		return ret < 0 ? ret : -EIO;
	}

	return 0;
}

static int mfc_reg_update(struct i2c_client *client, u16 reg, u8 val, u8 mask)
{
	//val = 0b 0000 0001
	//ms = 0b 1111 1110
	struct mfc_charger_data *charger = i2c_get_clientdata(client);
	unsigned char data[3] = {reg >> 8, reg & 0xff, val};
	u8 data2;
	int ret;

	if (charger->reg_access_lock) {
		pr_err("%s: can not access to reg during fw update\n", __func__);
		return -1;
	}

	ret = mfc_reg_read(client, reg, &data2);
	if (ret >= 0) {
		u8 old_val = data2 & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));

		data[2] = new_val;
		mutex_lock(&charger->io_lock);
		ret = i2c_master_send(client, data, 3);
		mfc_check_i2c_error(charger, (ret < 3));
		mutex_unlock(&charger->io_lock);
		if (ret < 3) {
			pr_err("%s: i2c write error, reg: 0x%x, ret: %d\n",
				__func__, reg, ret);
			return ret < 0 ? ret : -EIO;
		}
	}
	mfc_reg_read(client, reg, &data2);

	return ret;
}

static void cps4038_set_target_ilim(struct mfc_charger_data *charger, int ilim)
{
	u8 data = ilim/25;

	if (charger->sleep_mode && charger->target_current <= ilim && charger->target_current) {
		pr_info("@MPP %s: do not ctrl because sleep_mode(%d, %d)\n",
			__func__, charger->target_current, ilim);
		return;
	}

	charger->target_current = ilim;

	pr_info("%s: ilim(%d) reg(0x%x)\n", __func__, charger->target_current, data);
	mfc_reg_write(charger->client, MFC_TARGET_ILIM_REG, data);
}

static void cps4038_set_recent_ilim(struct mfc_charger_data *charger, int ilim)
{
	u8 data = ilim/25;

	pr_info("%s: ilim(%d) reg(0x%x)\n", __func__, ilim, data);
	mfc_reg_write(charger->client, MFC_RECENT_ILIM_REG, data);
}

static unsigned int mfc_get_firmware_version(struct mfc_charger_data *charger, int firm_mode)
{
	unsigned int ver_major = 0, ver_minor = 0;
	int ret;
	u8 fw_major[2] = {0, };
	u8 fw_minor[2] = {0, };

	pr_info("%s: called by (%ps)\n", __func__, __builtin_return_address(0));
	switch (firm_mode) {
	case MFC_RX_FIRMWARE:
		ret = mfc_reg_read(charger->client, MFC_FW_MAJOR_REV_L_REG, &fw_major[0]);
		if (ret < 0)
			break;

		ret = mfc_reg_read(charger->client, MFC_FW_MAJOR_REV_H_REG, &fw_major[1]);
		if (ret < 0)
			break;

		ver_major = fw_major[0] | (fw_major[1] << 8);
		pr_info("%s: rx major firmware version 0x%x\n", __func__, ver_major);

		ret = mfc_reg_read(charger->client, MFC_FW_MINOR_REV_L_REG, &fw_minor[0]);
		if (ret < 0)
			break;

		ret = mfc_reg_read(charger->client, MFC_FW_MINOR_REV_H_REG, &fw_minor[1]);
		if (ret < 0)
			break;

		ver_minor = fw_minor[0] | (fw_minor[1] << 8);
		pr_info("%s: rx minor firmware version 0x%x\n", __func__, ver_minor);
		break;
	default:
		pr_err("%s: Wrong firmware mode\n", __func__);
		break;
	}

#if defined(CONFIG_WIRELESS_IC_PARAM)
	if (ver_minor > 0 && charger->wireless_fw_ver_param != ver_minor) {
		pr_info("%s: fw_ver (param:0x%04X, version:0x%04X)\n",
			__func__, charger->wireless_fw_ver_param, ver_minor);
		charger->wireless_fw_ver_param = ver_minor;
		charger->wireless_param_info &= 0xFF0000FF;
		charger->wireless_param_info |= (charger->wireless_fw_ver_param & 0xFFFF) << 8;
		pr_info("%s: wireless_param_info (0x%08X)\n", __func__, charger->wireless_param_info);
	}
#endif

	return ver_minor;
}

static unsigned int mfc_get_bin_fw_version(struct device *dev)
{
	const char *fw_path = MFC_FLASH_FW_HEX_CPS_PATH;
	int ret = 0;
	static unsigned int bin_fw_ver;
	const struct firmware *firm_data_bin;

	if (bin_fw_ver != 0)
		return bin_fw_ver;

	ret = request_firmware(&firm_data_bin, fw_path, dev);
	if (ret < 0) {
		dev_err(dev, "%s: failed to request firmware %s (%d)\n",
			__func__, fw_path, ret);
		return bin_fw_ver;
	}

	/* get minor firmware version from firmware file */
	bin_fw_ver = firm_data_bin->data[MFC_FW_VER_BIN_ADDRESS+2] | (firm_data_bin->data[MFC_FW_VER_BIN_ADDRESS+3] << 8);
	release_firmware(firm_data_bin);
	pr_info("%s: bin rx minor firmware version 0x%04X\n", __func__, bin_fw_ver);

	return bin_fw_ver;
}

static bool mfc_check_chip_id(struct mfc_charger_data *charger)
{
	u8 data[4] = {0, };

	charger->reg_access_lock = true;

	mfc_cps_wls_write_word(charger->client, 0xFFFFFF00, 0x0000000E); /*enable 32bit i2c*/
	mfc_cps_wls_write_word(charger->client, 0x4000E75C, 0x00001250); /*write password*/
	mfc_cps_wls_read_word(charger->client, 0x60D4, data);
	mfc_cps_wls_write_word(charger->client, 0xFFFFFF00, 0x00000000); /* i2c 32bit mode disable */

	charger->reg_access_lock = false;

	pr_info("%s: CHIP_L(0x%x), CHIP_H(0x%x)\n", __func__, data[2], data[3]);

	if ((data[2] == 0x38) && (data[3] == 0x40))
		return true;

	return false;
}

static int mfc_get_chip_id(struct mfc_charger_data *charger)
{
	u8 chip_id = 0, chip_id_h = 0;
	int ret = 0;

	ret = mfc_reg_read(charger->client, MFC_CHIP_ID_L_REG, &chip_id);
	ret = mfc_reg_read(charger->client, MFC_CHIP_ID_H_REG, &chip_id_h);
	if (ret >= 0) {
		pr_info("%s: CHIP_L(0x%x), CHIP_H(0x%x)\n", __func__, chip_id, chip_id_h);
		charger->chip_id = MFC_CHIP_CPS;
	} else
		return -1;

#if defined(CONFIG_WIRELESS_IC_PARAM)
	if (chip_id > 0 && charger->wireless_chip_id_param != chip_id) {
		pr_info("%s: chip_id (param:0x%02X, chip_id:0x%02X)\n",
			__func__, charger->wireless_chip_id_param, chip_id);
		charger->wireless_chip_id_param = chip_id;
		charger->wireless_param_info &= 0x00FFFFFF;
		charger->wireless_param_info |= (charger->wireless_chip_id_param & 0xFF) << 24;
		pr_info("%s: wireless_param_info (0x%08X)\n", __func__, charger->wireless_param_info);
	}
#endif

	return charger->chip_id;
}

static int mfc_get_ic_revision(struct mfc_charger_data *charger, int read_mode)
{
	u8 temp;
	int ret = -1;

	pr_info("%s: called by (%ps)\n", __func__, __builtin_return_address(0));

	switch (read_mode) {
	case MFC_IC_REVISION:
		ret = mfc_reg_read(charger->client, MFC_CHIP_REVISION_REG, &temp);
		if (ret >= 0) {
			temp &= MFC_CHIP_REVISION_MASK;
			pr_info("%s: ic revision = %d\n", __func__, temp);
			ret = temp;
		}
		break;
	case MFC_IC_FONT:
		ret = mfc_reg_read(charger->client, MFC_CHIP_REVISION_REG, &temp);
		if (ret >= 0) {
			temp &= MFC_CHIP_FONT_MASK;
			pr_info("%s: ic font = %d\n", __func__, temp);
			ret = temp;
		}
		break;
	default:
		break;
	}
	return ret;
}

static int mfc_get_adc(struct mfc_charger_data *charger, int adc_type)
{
	int ret = 0;
	u8 data[2] = {0,};

	switch (adc_type) {
	case MFC_ADC_VOUT:
		ret = mfc_reg_read(charger->client, MFC_ADC_VOUT_L_REG, &data[0]);
		if (ret < 0)
			break;
		ret = mfc_reg_read(charger->client, MFC_ADC_VOUT_H_REG, &data[1]);
		if (ret < 0)
			break;

		ret = (data[0] | (data[1] << 8));
		charger->mfc_adc_vout = ret;
		break;
	case MFC_ADC_VRECT:
		ret = mfc_reg_read(charger->client, MFC_ADC_VRECT_L_REG, &data[0]);
		if (ret < 0)
			break;
		ret = mfc_reg_read(charger->client, MFC_ADC_VRECT_H_REG, &data[1]);
		if (ret < 0)
			break;

		ret = (data[0] | (data[1] << 8));
		charger->mfc_adc_vrect = ret;
		break;
	case MFC_ADC_RX_IOUT:
		ret = mfc_reg_read(charger->client, MFC_ADC_IOUT_L_REG, &data[0]);
		if (ret < 0)
			break;
		ret = mfc_reg_read(charger->client, MFC_ADC_IOUT_H_REG, &data[1]);
		if (ret < 0)
			break;

		ret = (data[0] | (data[1] << 8));
		charger->mfc_adc_rx_iout = ret;
		break;
	case MFC_ADC_DIE_TEMP:
		ret = mfc_reg_read(charger->client, MFC_ADC_DIE_TEMP_L_REG, &data[0]);
		if (ret < 0)
			break;
		ret = mfc_reg_read(charger->client, MFC_ADC_DIE_TEMP_H_REG, &data[1]);
		if (ret < 0)
			break;

		/* only 4 MSB[3:0] field is used, Celsius */
		data[1] &= 0x0f;
		ret = (data[0] | (data[1] << 8));
		break;
	case MFC_ADC_OP_FRQ: /* kHz */
		ret = mfc_reg_read(charger->client, MFC_TRX_OP_FREQ_L_REG, &data[0]);
		if (ret < 0)
			break;
		ret = mfc_reg_read(charger->client, MFC_TRX_OP_FREQ_H_REG, &data[1]);
		if (ret < 0)
			break;

		ret = (data[0] | (data[1] << 8)) / 10;
		charger->mfc_adc_op_frq = ret;
		break;
	case MFC_ADC_TX_MAX_OP_FRQ:
		ret = mfc_reg_read(charger->client, MFC_TX_MAX_OP_FREQ_L_REG, &data[0]);
		if (ret < 0)
			break;
		ret = mfc_reg_read(charger->client, MFC_TX_MAX_OP_FREQ_H_REG, &data[1]);
		if (ret < 0)
			break;

		ret = (data[0] | (data[1] << 8)) / 10;
		charger->mfc_adc_tx_max_op_frq = ret;
		break;
	case MFC_ADC_TX_MIN_OP_FRQ:
		ret = mfc_reg_read(charger->client, MFC_TX_MIN_OP_FREQ_L_REG, &data[0]);
		if (ret < 0)
			break;
		ret = mfc_reg_read(charger->client, MFC_TX_MIN_OP_FREQ_H_REG, &data[1]);
		if (ret < 0)
			break;

		ret = (data[0] | (data[1] << 8)) / 10;
		charger->mfc_adc_tx_min_op_frq = ret;
		break;
	case MFC_ADC_PING_FRQ:
		ret = mfc_reg_read(charger->client, MFC_TX_PING_FREQ_L_REG, &data[0]);
		if (ret < 0)
			break;
		ret = mfc_reg_read(charger->client, MFC_TX_PING_FREQ_H_REG, &data[1]);
		if (ret < 0)
			break;

		ret = (data[0] | (data[1] << 8)) / 10;
		charger->mfc_adc_ping_frq = ret;
		break;
	case MFC_ADC_TX_VOUT:
		ret = mfc_reg_read(charger->client, MFC_ADC_VOUT_L_REG, &data[0]);
		if (ret < 0)
			break;
		ret = mfc_reg_read(charger->client, MFC_ADC_VOUT_H_REG, &data[1]);
		if (ret < 0)
			break;

		ret = (data[0] | (data[1] << 8));
		charger->mfc_adc_tx_vout = ret;
		break;
	case MFC_ADC_TX_IOUT:
		if (charger->wc_tx_enable) {
			ret = mfc_reg_read(charger->client, MFC_ADC_IRECT_L_REG, &data[0]);
			if (ret < 0)
				break;
			ret = mfc_reg_read(charger->client, MFC_ADC_IRECT_H_REG, &data[1]);
			if (ret < 0)
				break;
		} else {
			ret = mfc_reg_read(charger->client, MFC_ADC_IOUT_L_REG, &data[0]);
			if (ret < 0)
				break;
			ret = mfc_reg_read(charger->client, MFC_ADC_IOUT_H_REG, &data[1]);
			if (ret < 0)
				break;
		}

		ret = (data[0] | (data[1] << 8));
		charger->mfc_adc_tx_iout = ret;
		break;
	default:
		break;
	}

	return ret;
}

static void mfc_set_wpc_en(struct mfc_charger_data *charger, char flag, char on)
{
	union power_supply_propval value = {0, };
	int enable = 0, temp = charger->wpc_en_flag;
	int old_val = 0, new_val = 0;

	psy_do_property("battery", get,
		POWER_SUPPLY_PROP_STATUS, value);

	mutex_lock(&charger->wpc_en_lock);

	if (on) {
		if (value.intval == POWER_SUPPLY_STATUS_DISCHARGING)
			charger->wpc_en_flag |= WPC_EN_CHARGING;
		charger->wpc_en_flag |= flag;
	} else {
		charger->wpc_en_flag &= ~flag;
	}

	if (charger->wpc_en_flag & WPC_EN_FW)
		enable = 1;
	else if (!(charger->wpc_en_flag & WPC_EN_SYSFS) || !(charger->wpc_en_flag & WPC_EN_CCIC))
		enable = 0;
	else if (!(charger->wpc_en_flag & (WPC_EN_SLATE | WPC_EN_MST)))
		enable = 0;
	else if (!(charger->wpc_en_flag & (WPC_EN_CHARGING | WPC_EN_MST | WPC_EN_TX)))
		enable = 0;
	else
		enable = 1;

	if (charger->pdata->wpc_en >= 0) {
		old_val = gpio_get_value(charger->pdata->wpc_en);
		if (enable)
			gpio_direction_output(charger->pdata->wpc_en, 0);
		else
			gpio_direction_output(charger->pdata->wpc_en, 1);
		new_val = gpio_get_value(charger->pdata->wpc_en);

		pr_info("@DIS_MFC %s: before(0x%x), after(0x%x), en(%d), val(%d->%d)\n",
			__func__, temp, charger->wpc_en_flag, enable, old_val, new_val);

		mutex_unlock(&charger->wpc_en_lock);
		if (old_val != new_val) {
			value.intval = enable;
			psy_do_property("battery", set,
				POWER_SUPPLY_EXT_PROP_WPC_EN, value);
		}
	} else {
		pr_info("@DIS_MFC %s: there`s no wpc_en\n", __func__);
		mutex_unlock(&charger->wpc_en_lock);
	}
}

static void mfc_set_coil_sw_en(struct mfc_charger_data *charger, int enable)
{
#if defined(CONFIG_SEC_FACTORY)
	pr_info("@Tx_Mode %s: do not set with factory bin\n", __func__);
	return;
#endif

	if (charger->pdata->coil_sw_en < 0) {
		pr_info("@Tx_Mode %s: no coil_sw_en\n", __func__);
		return;
	}

	if (gpio_get_value(charger->pdata->coil_sw_en) == enable) {
		pr_info("@Tx_Mode %s: Skip to set same config, val(%d)\n",
			__func__, gpio_get_value(charger->pdata->coil_sw_en));
		return;
	}

	gpio_direction_output(charger->pdata->coil_sw_en, enable);
	pr_info("@Tx_Mode %s: en(%d), val(%d)\n", __func__,
		enable, gpio_get_value(charger->pdata->coil_sw_en));
}

static void mfc_set_force_vout(struct mfc_charger_data *charger, int vout)
{
	u8 data[2] = {0,};

	if ((charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_EPP_FAKE) &&
		(charger->rx_op_mode == MFC_RX_MODE_WPC_EPP_NEGO)) {
		pr_info("%s: Escape MPP and EPP Calibration step\n", __func__);
		return;
	}

	if (charger->wc_ldo_status == MFC_LDO_OFF) {
		pr_info("%s: vout updated to 5V becauseof wc ldo\n", __func__);
		vout = MFC_VOUT_5V;
	}

	data[0] = mfc_cps_vout_val16[vout] & 0xff;
	data[1] = (mfc_cps_vout_val16[vout] & 0xff00) >> 8;
	mfc_reg_write(charger->client, MFC_VOUT_SET_H_REG, data[1]);
	mfc_reg_write(charger->client, MFC_VOUT_SET_L_REG, data[0]);

	mfc_fod_set_vout(charger->fod, (2280 + (mfc_cps_vout_val16[vout] * 20)));
	mfc_cmfet_set_vout(charger->cmfet, (2280 + (mfc_cps_vout_val16[vout] * 20)));
	msleep(100);

	pr_info("%s: set force vout(%s, 0x%04X) read = %d mV\n", __func__,
		sb_rx_vout_str(vout), (data[0] | (data[1] << 8)), mfc_get_adc(charger, MFC_ADC_VOUT));
	mfc_get_adc(charger, MFC_ADC_VRECT);
	charger->pdata->vout_status = vout;
}

static void mfc_set_vout(struct mfc_charger_data *charger, int vout)
{
	u8 data[2] = {0,};

	if ((charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP) ||
		((charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_EPP_FAKE) &&
		(charger->rx_op_mode == MFC_RX_MODE_WPC_EPP_NEGO))) {
		pr_info("%s: Escape MPP and EPP Calibration step\n", __func__);
		return;
	}

	if (charger->wc_ldo_status == MFC_LDO_OFF) {
		pr_info("%s: vout updated to 5V becauseof wc ldo\n", __func__);
		vout = MFC_VOUT_5V;
	}

	if (charger->is_otg_on && (vout > MFC_VOUT_5V)) {
		pr_info("%s: skip set vout in otg case\n", __func__);
		return;
	}

	data[0] = mfc_cps_vout_val16[vout] & 0xff;
	data[1] = (mfc_cps_vout_val16[vout] & 0xff00) >> 8;
	mfc_reg_write(charger->client, MFC_VOUT_SET_H_REG, data[1]);
	mfc_reg_write(charger->client, MFC_VOUT_SET_L_REG, data[0]);

	mfc_fod_set_vout(charger->fod, (2280 + (mfc_cps_vout_val16[vout] * 20)));
	mfc_cmfet_set_vout(charger->cmfet, (2280 + (mfc_cps_vout_val16[vout] * 20)));
	msleep(100);

	pr_info("%s: set vout(%s, 0x%04X) read = %d mV\n", __func__,
		sb_rx_vout_str(vout), (data[0] | (data[1] << 8)), mfc_get_adc(charger, MFC_ADC_VOUT));
	mfc_get_adc(charger, MFC_ADC_VRECT);
	charger->pdata->vout_status = vout;
}

static int mfc_get_vout(struct mfc_charger_data *charger)
{
	u8 data[2] = {0,};
	int ret;

	ret = mfc_reg_read(charger->client, MFC_VOUT_SET_L_REG, &data[0]);
	ret = mfc_reg_read(charger->client, MFC_VOUT_SET_H_REG, &data[1]);

	if (ret < 0) {
		pr_err("%s: fail to read vout. (%d)\n", __func__, ret);
	} else {
		ret = (data[0] | (data[1] << 8));
		pr_info("%s: vout(0x%04x)\n", __func__, ret);
	}

	return ret;
}

static void mfc_uno_on(struct mfc_charger_data *charger, bool on)
{
	union power_supply_propval value = {0, };

	if (charger->wc_tx_enable && on) { /* UNO ON */
		value.intval = SEC_BAT_CHG_MODE_UNO_ONLY;
		psy_do_property("otg", set,
			POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL, value);
		pr_info("%s: UNO ON\n", __func__);
	} else if (on) { /* UNO ON */
		value.intval = 1;
		psy_do_property("otg", set,
			POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL, value);
		pr_info("%s: UNO ON\n", __func__);
	} else { /* UNO OFF */
		value.intval = 0;
		psy_do_property("otg", set,
			POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL, value);

		if (delayed_work_pending(&charger->wpc_tx_duty_min_work)) {
			__pm_relax(charger->wpc_tx_duty_min_ws);
			cancel_delayed_work(&charger->wpc_tx_duty_min_work);
		}
		pr_info("%s: UNO OFF\n", __func__);
	}
}

static void mfc_rpp_set(struct mfc_charger_data *charger, u8 val)
{
	u8 data;
	int ret;

	pr_info("%s: Scale Factor %d\n", __func__, val);
	mfc_reg_write(charger->client, MFC_RPP_SCALE_COEF_REG, val);
	usleep_range(5000, 6000);
	ret = mfc_reg_read(charger->client, MFC_RPP_SCALE_COEF_REG, &data);
	if (ret < 0)
		pr_err("%s: fail to read RPP scaling coefficient. (%d)\n", __func__, ret);
	else
		pr_info("%s: RPP scaling coefficient(0x%x)\n", __func__, data);
}

static void mfc_set_tx_conflict_current(struct mfc_charger_data *charger, int tx_cf_current)
{
	u8 data = (u8)(tx_cf_current / 100);

	mfc_reg_write(charger->client, MFC_TX_CONFLICT_CURRENT_REG, data);
	pr_info("%s: current = %d, data = 0x%x\n", __func__, tx_cf_current, data);
}

static void mfc_set_tx_digital_ping_freq(struct mfc_charger_data *charger, unsigned int op_freq)
{
	u8 data[2] = {0,};

	pr_info("%s: tx_digital_ping_freq = %d KHz\n", __func__, op_freq / 10);

	data[0] = op_freq & 0xFF;
	data[1] = (op_freq & 0xFF00) >> 8;
	mfc_reg_write(charger->client, MFC_TX_PING_FREQ_L_REG, data[0]);
	mfc_reg_write(charger->client, MFC_TX_PING_FREQ_H_REG, data[1]);
}

static void mfc_set_tx_min_op_freq(struct mfc_charger_data *charger, unsigned int op_freq)
{
	u8 data[2] = {0,};

	pr_info("%s: tx_min_op_freq = %d KHz\n", __func__, op_freq / 10);

	data[0] = op_freq & 0xFF;
	data[1] = (op_freq & 0xFF00) >> 8;
	mfc_reg_write(charger->client, MFC_TX_MIN_OP_FREQ_L_REG, data[0]);
	mfc_reg_write(charger->client, MFC_TX_MIN_OP_FREQ_H_REG, data[1]);
}

static void mfc_set_tx_max_op_freq(struct mfc_charger_data *charger, unsigned int op_freq)
{
	u8 data[2] = {0,};

	pr_info("%s: tx_max_op_freq = %d KHz\n", __func__, op_freq / 10);

	data[0] = op_freq & 0xFF;
	data[1] = (op_freq & 0xFF00) >> 8;
	mfc_reg_write(charger->client, MFC_TX_MAX_OP_FREQ_L_REG, data[0]);
	mfc_reg_write(charger->client, MFC_TX_MAX_OP_FREQ_H_REG, data[1]);
}

static void mfc_set_min_duty(struct mfc_charger_data *charger, unsigned int duty)
{
	pr_info("%s: min duty = %d\n", __func__, duty);
	mfc_reg_write(charger->client, MFC_TX_MIN_DUTY_SETTING_REG, duty);
}

static void mfc_set_cmd_l_reg(struct mfc_charger_data *charger, u8 val, u8 mask)
{
	u8 temp = 0;
	int ret = 0, i = 0;

	do {
		pr_info("%s\n", __func__);
		ret = mfc_reg_update(charger->client, MFC_AP2MFC_CMD_L_REG, val, mask); // command
		if (ret >= 0) {
			usleep_range(10000, 11000);
			ret = mfc_reg_read(charger->client, MFC_AP2MFC_CMD_L_REG, &temp); // check out set bit exists
			if (temp != 0)
				pr_info("%s: CMD is not clear yet, cnt = %d\n", __func__, i);
			if (ret < 0 || i > 3)
				break;
		}
		i++;
	} while ((temp != 0) && (i < 3));
}

static void mfc_set_cmd_h_reg(struct mfc_charger_data *charger, u8 val, u8 mask)
{
	u8 temp = 0;
	int ret = 0, i = 0;

	do {
		pr_info("%s\n", __func__);
		ret = mfc_reg_update(charger->client, MFC_AP2MFC_CMD_H_REG, val, mask); // command
		if (ret >= 0) {
			usleep_range(10000, 11000);
			ret = mfc_reg_read(charger->client, MFC_AP2MFC_CMD_H_REG, &temp); // check out set bit exists
			if (temp != 0)
				pr_info("%s: CMD is not clear yet, cnt = %d\n", __func__, i);
			if (ret < 0 || i > 3)
				break;
		}
		i++;
	} while ((temp != 0) && (i < 3));
}

static void mfc_soft_reset_ask_stop(struct mfc_charger_data *charger)
{
	pr_info("%s\n", __func__);

	mfc_set_cmd_h_reg(charger,
		MFC_CMD2_MPP_STOP_SEND_ASK_MASK, MFC_CMD2_MPP_STOP_SEND_ASK_MASK);
}

//need power on wireless IC
static void mfc_mpp_exit_cloak_cmd(struct mfc_charger_data *charger)
{
	mfc_set_cmd_h_reg(charger, MFC_CMD2_MPP_EXIT_CLOAK_MASK, MFC_CMD2_MPP_EXIT_CLOAK_MASK);
	pr_info("@MPP %s\n", __func__);

	if (delayed_work_pending(&charger->set_mpp_cloak_work)) {
		__pm_relax(charger->set_mpp_cloak_ws);
		cancel_delayed_work(&charger->set_mpp_cloak_work);
	}
	__pm_stay_awake(charger->set_mpp_cloak_ws);
	charger->mpp_cloak_status = false;
	queue_delayed_work(charger->wqueue,
		&charger->set_mpp_cloak_work, msecs_to_jiffies(2000));
}

static void mfc_mpp_exit_cloak_turn_off_power(struct mfc_charger_data *charger)
{
	charger->mpp_cloak = 0;
	if (charger->pdata->mpp_sw > 0) {
		if (gpio_get_value(charger->pdata->mpp_sw))
			gpio_direction_output(charger->pdata->mpp_sw, 0);
	}
	pr_info("@MPP %s: coil sw (%d)\n", __func__,
		(charger->pdata->mpp_sw < 0 ? charger->pdata->mpp_sw :
									gpio_get_value(charger->pdata->mpp_sw)));
}

//need load step down and power on wireless IC
static void mfc_mpp_enter_cloak(struct mfc_charger_data *charger, u8 cloak_reason)
{
	charger->mpp_cloak = cloak_reason;

	if (charger->pdata->mpp_sw > 0) {
		pr_info("@MPP %s: coil sw (%d)\n", __func__, gpio_get_value(charger->pdata->mpp_sw));

		if (!gpio_get_value(charger->pdata->mpp_sw))
			gpio_direction_output(charger->pdata->mpp_sw, 1);
	}
	if (mfc_reg_write(charger->client, MFC_MPP_CLOAK_REASON_REG, cloak_reason) >= 0) {
		mfc_set_cmd_h_reg(charger, MFC_CMD2_MPP_ENTER_CLOAK_MASK, MFC_CMD2_MPP_ENTER_CLOAK_MASK);
		pr_info("@MPP %s: enter cloak and reason is %d coil sw (%d)\n", __func__,
			charger->mpp_cloak, (charger->pdata->mpp_sw < 0 ? charger->pdata->mpp_sw :
							gpio_get_value(charger->pdata->mpp_sw)));
	}
	if (delayed_work_pending(&charger->set_mpp_cloak_work)) {
		__pm_relax(charger->set_mpp_cloak_ws);
		cancel_delayed_work(&charger->set_mpp_cloak_work);
	}
	__pm_stay_awake(charger->set_mpp_cloak_ws);
	charger->mpp_cloak_status = false;
	queue_delayed_work(charger->wqueue,
		&charger->set_mpp_cloak_work, msecs_to_jiffies(2000));
}

#if 0
static void mfc_mpp_full_mode_enable(struct mfc_charger_data *charger)
{
	mfc_set_cmd_h_reg(charger, MFC_CMD2_MPP_FULL_MODE_SHIFT, MFC_CMD2_MPP_FULL_MODE_SHIFT);
	pr_info("%s: restrict mode to full mode\n", __func__);
}

//if trans_type = MFC_RX_MPP_FULL_MODE_TRAN_POWER_RESET, need disable restrict mode and enable full mode after wireless chip restart
static void mfc_mpp_full_mode_trans(struct mfc_charger_data *charger, u8 trans_type)
{
	int ret = 0;

	ret = mfc_reg_write(charger->client, MFC_MPP_FULL_MODE_TRANS_TYPE_REG, trans_type);
	if (ret < 0)
		return;

	pr_info("%s: mpp full mode transfer %d\n", __func__, trans_type);
}
#endif

static void mfc_protocol_selection(struct mfc_charger_data *charger, u8 protocol)
{
	int ret = 0;

	ret = mfc_reg_write(charger->client, MFC_PROTOCOL_SELECTION_REG, protocol);
	if (ret < 0)
		return;
	pr_info("%s: set protocol to %d\n", __func__, protocol);
}

//set negotiate power with Tx, set before negotiation phase.
static void mfc_mpp_epp_nego_power_set(struct mfc_charger_data *charger, u8 nego_load_power)
{
	int ret = 0;
#if 0
	u8 reg_data = 0;

	ret = mfc_reg_read(charger->client, MFC_INT_B_ENABLE_L_REG, &reg_data);
	if ((ret >= 0) && !(reg_data & MFC_INTB_L_INCREASE_POWER_MASK)) {
		pr_info("@MPP @EPP %s: Skip set negotiate power by high temp\n", __func__);
		return;
	}
#endif
	ret = mfc_reg_write(charger->client, MFC_MPP_POWER_LEVEL_SETTING_REG, (nego_load_power << 1));
	if (ret < 0)
		return;

	pr_info("@MPP @EPP %s: MPP or EPP set negotiate power %dW\n", __func__, nego_load_power);
}

static void mfc_mpp_inc_int_ctrl(struct mfc_charger_data *charger, u32 enable)
{
	int ret = 0;

	ret = mfc_reg_update(charger->client, MFC_INT_B_ENABLE_L_REG,
		(enable << MFC_INTB_L_INCREASE_POWER_SHIFT), MFC_INTB_L_INCREASE_POWER_MASK);
	if (ret < 0)
		pr_info("@MPP %s: MFC_INT_B_ENABLE_L_REG write error(%d)\n",
			__func__, ret);

	pr_info("@MPP %s: enable(%d)\n", __func__, enable);
}

static void mfc_mpp_thermal_ctrl(struct mfc_charger_data *charger, u32 enable)
{
	int ret = 0;
	u8 reg_data = 0;

	if (charger->sleep_mode && !enable) {
		pr_info("@MPP %s: do not ctrl because sleep_mode(%d)\n",
			__func__, charger->sleep_mode);
		return;
	}

	if (enable)
		reg_data = ((charger->target_current / 25) << 1 | enable);

	ret = mfc_reg_write(charger->client, MFC_MPP_THERMAL_CTRL_REG, reg_data);
	if (ret < 0)
		pr_info("@MPP %s: MFC_MPP_THERMAL_CTRL_REG write error(%d)\n",
			__func__, ret);

	pr_info("@MPP %s: enable(%d) current(%d)\n", __func__, enable, charger->target_current);
}

static void mfc_epp_enable(struct mfc_charger_data *charger, u32 enable)
{
	if (charger->high_swell && enable) {
		pr_info("@MPP %s: do not enable - high_swell(%d)\n",
			__func__, charger->high_swell);
		enable = 0;
	}

	if (charger->pdata->mag_det >= 0)
		gpio_direction_output(charger->pdata->mag_det, enable);

	if (charger->pdata->wpc_mode >= 0)
		gpio_direction_output(charger->pdata->wpc_mode, enable);

	pr_info("@MPP %s: enable(%d), wpc_mode = %d\n",
		__func__, enable,
		(charger->pdata->wpc_mode >= 0 ? gpio_get_value(charger->pdata->wpc_mode) : -1));
}

#if 0
static u32 mfc_mpp_epp_get_nego_done_power(struct mfc_charger_data *charger)
{
	u32 data = 0;
	u8 temp = 0;

	mfc_reg_read(charger->client, MFC_MPP_EPP_NEGO_DONE_POWER_L_REG, &temp);
	data = temp;
	mfc_reg_read(charger->client, MFC_MPP_EPP_NEGO_DONE_POWER_H_REG, &temp);
	data |= (temp << 8);
	data = data * 100;
	pr_info("@MPP @EPP %s: mpp or epp negotiated power %dmW\n", __func__, data);
	return data;
}

static u8 mfc_mpp_get_gp_state(struct mfc_charger_data *charger)
{
	u8 data = 0;

	mfc_reg_read(charger->client, MFC_MPP_GP_STATE_REG, &data);
	pr_info("%s: mpp gp state %02X\n", __func__, data);
	return data;
}
#endif

static void mfc_send_eop(struct mfc_charger_data *charger, int health_mode)
{
	int i = 0;
	int ret = 0;

	pr_info("%s: health_mode(0x%x), cable_type(%d)\n",
		__func__, health_mode, charger->pdata->cable_type);

	switch (health_mode) {
	case POWER_SUPPLY_HEALTH_OVERHEAT:
	case POWER_SUPPLY_EXT_HEALTH_OVERHEATLIMIT:
	case POWER_SUPPLY_HEALTH_COLD:
		pr_info("%s: ept-ot\n", __func__);
		if (charger->pdata->cable_type == SEC_BATTERY_CABLE_PMA_WIRELESS) {
			for (i = 0; i < CMD_CNT; i++) {
				ret = mfc_reg_write(charger->client, MFC_EPT_REG, MFC_WPC_EPT_END_OF_CHG);
				if (ret < 0)
					break;

				mfc_set_cmd_l_reg(charger, MFC_CMD_SEND_EOP_MASK, MFC_CMD_SEND_EOP_MASK);
				msleep(250);
			}
		} else {
			for (i = 0; i < CMD_CNT; i++) {
				ret = mfc_reg_write(charger->client, MFC_EPT_REG, MFC_WPC_EPT_OVER_TEMP);
				if (ret < 0)
					break;

				mfc_set_cmd_l_reg(charger, MFC_CMD_SEND_EOP_MASK, MFC_CMD_SEND_EOP_MASK);
				msleep(250);
			}
		}
		break;
	default:
		break;
	}
}

static void mfc_packet_assist(struct mfc_charger_data *charger)
{
	u8 dummy;

	mfc_reg_read(charger->client, MFC_WPC_RX_DATA_COM_REG, &dummy);
	mfc_reg_read(charger->client, MFC_WPC_RX_DATA_VALUE0_REG, &dummy);
	mfc_reg_read(charger->client, MFC_AP2MFC_CMD_L_REG, &dummy);
}

static void mfc_wpc_check_fod_count(struct mfc_charger_data *charger, int fod_state)
{
	if (charger->pdata->ping_nen < 0)
		return;

	pr_info("%s: state(%d) cnt(%d) cnt_thresh(%d) phm(%d)!\n",
		__func__, fod_state, charger->fod_cnt, charger->pdata->fod_cnt_thresh, charger->rx_phm_state);

	if (fod_state == FOD_CNT_CLEAR) {
		if (charger->fod_cnt == 0)
			return;
		if (charger->fod_cnt > charger->pdata->fod_cnt_thresh && charger->rx_phm_state != ENTER_PHM) {
			pr_info("%s: set wpc_mode high\n", __func__);
			mfc_epp_enable(charger, 1);
			mfc_soft_reset_ask_stop(charger);
		}
		charger->fod_cnt = 0;
		return;
	}
	if (fod_state == FOD_CNT_ADD) {
		if (++charger->fod_cnt > charger->pdata->fod_cnt_thresh) {
			pr_info("%s: set wpc_mode low\n", __func__);
			mfc_epp_enable(charger, 0);
		}
	}
}

static void mfc_send_packet(struct mfc_charger_data *charger, u8 header, u8 rx_data_com, u8 *data_val, int data_size)
{
	int i;

	mfc_reg_write(charger->client, MFC_WPC_PCKT_HEADER_REG, header);
	mfc_reg_write(charger->client, MFC_WPC_RX_DATA_COM_REG, rx_data_com);

	for (i = 0; i < data_size; i++)
		mfc_reg_write(charger->client, MFC_WPC_RX_DATA_VALUE0_REG + i, data_val[i]);
	mfc_set_cmd_l_reg(charger, MFC_CMD_SEND_TRX_DATA_MASK, MFC_CMD_SEND_TRX_DATA_MASK);
}

static int mfc_send_cs100(struct mfc_charger_data *charger)
{
	int i = 0;
	int ret = 0;

	for (i = 0; i < CMD_CNT; i++) {
		ret = mfc_reg_write(charger->client, MFC_BATTERY_CHG_STATUS_REG, 100);
		if (ret >= 0) {
			mfc_set_cmd_l_reg(charger, MFC_CMD_SEND_CHG_STS_MASK, MFC_CMD_SEND_CHG_STS_MASK);
			msleep(250);
			ret = 1;
		} else {
			ret = -1;
			break;
		}
	}
	return ret;
}

static void mfc_send_ept_packet(struct mfc_charger_data *charger, uint8_t ept_value)
{
	pr_info("%s: send ept value = 0x%x\n", __func__, ept_value);
	mfc_reg_write(charger->client, MFC_EPT_REG, ept_value);
	mfc_set_cmd_l_reg(charger, MFC_CMD_SEND_EOP_MASK, MFC_CMD_SEND_EOP_MASK);
}

static void mfc_send_command(struct mfc_charger_data *charger, int cmd_mode)
{
	u8 data_val[4];
	u8 cmd = 0;
	u8 i;

	switch (cmd_mode) {
	case MFC_AFC_CONF_5V:
		pr_info("%s: WPC/PMA set 5V\n", __func__);
		cmd = WPC_COM_AFC_SET;
		data_val[0] = 0x05; /* Value for WPC AFC_SET 5V */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);  // 0x28 0x06 0x05
		msleep(120);

		charger->vout_mode = WIRELESS_VOUT_5V;
		cancel_delayed_work(&charger->wpc_vout_mode_work);
		__pm_stay_awake(charger->wpc_vout_mode_ws);
		queue_delayed_work(charger->wqueue, &charger->wpc_vout_mode_work, 0);

		pr_info("%s: vout read = %d\n", __func__, mfc_get_adc(charger, MFC_ADC_VOUT));
		mfc_packet_assist(charger);
		break;
	case MFC_AFC_CONF_10V:
		pr_info("%s: WPC set 10V\n", __func__);
		/* trigger 10V vout work after 8sec */
		__pm_stay_awake(charger->wpc_afc_vout_ws);
#if defined(CONFIG_SEC_FACTORY)
		queue_delayed_work(charger->wqueue, &charger->wpc_afc_vout_work, msecs_to_jiffies(3000));
#else
		queue_delayed_work(charger->wqueue, &charger->wpc_afc_vout_work, msecs_to_jiffies(8000));
#endif
		break;
	case MFC_AFC_CONF_5V_TX:
		for (i = 0; i < CMD_CNT; i++) {
			cmd = WPC_COM_AFC_SET;
			data_val[0] = RX_DATA_VAL2_5V; /* Value for WPC AFC_SET 5V */
			pr_info("%s: set 5V to TX, cnt = %d\n", __func__, i);
			mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);  // 28 06 05
			mfc_packet_assist(charger);
			msleep(100);
		}
		mfc_rpp_set(charger, 128);
		break;
	case MFC_AFC_CONF_10V_TX:
		for (i = 0; i < CMD_CNT; i++) {
			cmd = WPC_COM_AFC_SET;
			data_val[0] = RX_DATA_VAL2_10V; /* Value for WPC AFC_SET 10V */
			pr_info("%s: set 10V to TX, cnt = %d\n", __func__, i);
			mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);  // 28 06 2c
			mfc_packet_assist(charger);
			msleep(100);
		}
		mfc_rpp_set(charger, 64);
		break;
	case MFC_AFC_CONF_12V_TX:
		for (i = 0; i < CMD_CNT; i++) {
			cmd = WPC_COM_AFC_SET;
			data_val[0] = RX_DATA_VAL2_12V; /* Value for WPC AFC_SET 12V */
			pr_info("%s: set 12V to TX, cnt = %d\n", __func__, i);
			mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
			mfc_packet_assist(charger);
			msleep(100);
		}
		mfc_rpp_set(charger, 64);
		break;
	case MFC_AFC_CONF_12_5V_TX:
		disable_irq(charger->pdata->irq_wpc_int); /* disable int in order not to have rx power int before this work done */
		for (i = 0; i < CMD_CNT; i++) {
			cmd = WPC_COM_AFC_SET;
			data_val[0] = RX_DATA_VAL2_12_5V; /* Value for WPC AFC_SET 12V */
			pr_info("%s: set 12.5V to TX, cnt = %d\n", __func__, i);
			mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
			mfc_packet_assist(charger);
			msleep(100);
		}
		mfc_rpp_set(charger, 64);
		enable_irq(charger->pdata->irq_wpc_int);

		if (charger->pdata->cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20) {
			if (delayed_work_pending(&charger->wpc_rx_power_trans_fail_work)) {
				__pm_relax(charger->wpc_rx_power_trans_fail_ws);
				cancel_delayed_work(&charger->wpc_rx_power_trans_fail_work);
			}
			charger->check_rx_power = true;
			__pm_stay_awake(charger->wpc_rx_power_trans_fail_ws);
			queue_delayed_work(charger->wqueue,
				&charger->wpc_rx_power_trans_fail_work, msecs_to_jiffies(3000)); /* this work is for in case of missing rx power intterupt */
		}
		break;
	case MFC_AFC_CONF_20V_TX:
		for (i = 0; i < CMD_CNT; i++) {
			cmd = WPC_COM_AFC_SET;
			data_val[0] = RX_DATA_VAL2_20V; /* Value for WPC AFC_SET 20V */
			pr_info("%s: set 20V to TX, cnt = %d\n", __func__, i);
			mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
			mfc_packet_assist(charger);
			msleep(100);
		}
		mfc_rpp_set(charger, 32);
		break;
	case MFC_LED_CONTROL_ON:
		pr_info("%s: led on\n", __func__);
		cmd = WPC_COM_LED_CONTROL;
		data_val[0] = 0x00; /* Value for WPC LED ON */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		msleep(100);
		break;
	case MFC_LED_CONTROL_OFF:
		pr_info("%s: led off\n", __func__);
		cmd = WPC_COM_LED_CONTROL;
		data_val[0] = 0xff; /* Value for WPC LED OFF */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		msleep(100);
		break;
	case MFC_LED_CONTROL_DIMMING:
		pr_info("%s: led dimming\n", __func__);
		cmd = WPC_COM_LED_CONTROL;
		data_val[0] = 0x55; /* Value for WPC LED DIMMING */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		msleep(100);
		break;
	case MFC_FAN_CONTROL_ON:
		pr_info("%s: fan on\n", __func__);
		cmd = WPC_COM_COOLING_CTRL;
		data_val[0] = 0x00; /* Value for WPC FAN ON */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		msleep(100);
		break;
	case MFC_FAN_CONTROL_OFF:
		pr_info("%s: fan off\n", __func__);
		cmd = WPC_COM_COOLING_CTRL;
		data_val[0] = 0xff; /* Value for WPC FAN OFF */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		msleep(100);
		break;
	case MFC_REQUEST_AFC_TX:
		pr_info("%s: request afc tx, cable(0x%x)\n", __func__, charger->pdata->cable_type);
		cmd = WPC_COM_REQ_AFC_TX;
		data_val[0] = 0x00; /* Value for WPC Request AFC_TX */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	case MFC_REQUEST_TX_ID:
		pr_info("%s: request TX ID\n", __func__);
		cmd = WPC_COM_TX_ID;
		data_val[0] = 0x00; /* Value for WPC TX ID */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	case MFC_DISABLE_TX:
		pr_info("%s: Disable TX\n", __func__);
		cmd = WPC_COM_DISABLE_TX;
		data_val[0] = 0x55; /* Value for Disable TX */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	case MFC_PHM_ON:
		pr_info("%s: Enter PHM\n", __func__);
		cmd = WPC_COM_ENTER_PHM;
		data_val[0] = 0x01; /* Enter PHM */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	case MFC_SET_OP_FREQ:
		pr_info("%s: set tx op freq\n", __func__);
		cmd = WPC_COM_OP_FREQ_SET;
		data_val[0] = 0x69; /* Value for OP FREQ 120.5kHz */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	case MFC_TX_UNO_OFF:
		pr_info("%s: UNO off TX\n", __func__);
		cmd = WPC_COM_DISABLE_TX;
		data_val[0] = 0xFF; /* Value for Uno off */
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	case MFC_REQ_TX_PWR_BUDG:
		pr_info("%s: request PWR BUDG\n", __func__);
		cmd = WPC_COM_REQ_PWR_BUDG;
		data_val[0] = 0x00;
		mfc_send_packet(charger, MFC_HEADER_AFC_CONF, cmd, data_val, 1);
		break;
	default:
		break;
	}
}

static void mfc_send_fsk(struct mfc_charger_data *charger, u8 tx_data_com, u8 data_val)
{
	int i;

	for (i = 0; i < 3; i++) {
		mfc_reg_write(charger->client, MFC_WPC_TX_DATA_COM_REG, tx_data_com);
		mfc_reg_write(charger->client, MFC_WPC_TX_DATA_VALUE0_REG, data_val);
		mfc_set_cmd_l_reg(charger, MFC_CMD_SEND_TRX_DATA_MASK, MFC_CMD_SEND_TRX_DATA_MASK);
		msleep(250);
	}
}

static void mfc_led_control(struct mfc_charger_data *charger, int state)
{
	int i = 0;

	for (i = 0; i < CMD_CNT; i++)
		mfc_send_command(charger, state);
}

static void mfc_fan_control(struct mfc_charger_data *charger, bool on)
{
	int i = 0;

	if (on) {
		for (i = 0; i < CMD_CNT - 1; i++)
			mfc_send_command(charger, MFC_FAN_CONTROL_ON);
	} else {
		for (i = 0; i < CMD_CNT - 1; i++)
			mfc_send_command(charger, MFC_FAN_CONTROL_OFF);
	}
}

static void mfc_set_vrect_adjust(struct mfc_charger_data *charger, int set)
{
	if (charger->vout_mode == charger->pdata->wpc_vout_ctrl_full)
		return;

	switch (set) {
	case MFC_HEADROOM_0:
			mfc_reg_write(charger->client, MFC_VRECT_ADJ_REG, 0x0);
		break;
	case MFC_HEADROOM_1:
			mfc_reg_write(charger->client, MFC_VRECT_ADJ_REG, 0x1c); //LSB 10mV
		break;
	case MFC_HEADROOM_2:
			mfc_reg_write(charger->client, MFC_VRECT_ADJ_REG, 0x32);
		break;
	case MFC_HEADROOM_3:
			mfc_reg_write(charger->client, MFC_VRECT_ADJ_REG, 0x41);
		break;
	case MFC_HEADROOM_4:
			mfc_reg_write(charger->client, MFC_VRECT_ADJ_REG, 0x03);
		break;
	case MFC_HEADROOM_5:
			mfc_reg_write(charger->client, MFC_VRECT_ADJ_REG, 0x08);
		break;
	case MFC_HEADROOM_6:
			mfc_reg_write(charger->client, MFC_VRECT_ADJ_REG, 0x0a);
		break;
	case MFC_HEADROOM_7:
			mfc_reg_write(charger->client, MFC_VRECT_ADJ_REG, 0xc4);
		break;
	default:
		pr_info("%s no headroom mode\n", __func__);
		break;
	}
}

static void mfc_set_vout_ctrl_1st_full(struct mfc_charger_data *charger)
{
	if (!volt_ctrl_pad(charger->tx_id) ||
		(charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP))
		return;

	charger->vout_mode = WIRELESS_VOUT_CC_CV_VOUT;
	cancel_delayed_work(&charger->wpc_vout_mode_work);
	__pm_stay_awake(charger->wpc_vout_mode_ws);
	queue_delayed_work(charger->wqueue,
		&charger->wpc_vout_mode_work, msecs_to_jiffies(250));
}

static void mfc_set_vout_ctrl_2nd_full(struct mfc_charger_data *charger)
{
	if (!charger->pdata->wpc_vout_ctrl_full
		|| (charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP)
		|| !volt_ctrl_pad(charger->tx_id))
		return;

	if (charger->pdata->wpc_headroom_ctrl_full)
		mfc_set_vrect_adjust(charger, MFC_HEADROOM_7);
	charger->vout_mode = charger->pdata->wpc_vout_ctrl_full;
	cancel_delayed_work(&charger->wpc_vout_mode_work);
	__pm_stay_awake(charger->wpc_vout_mode_ws);
	queue_delayed_work(charger->wqueue,
		&charger->wpc_vout_mode_work, msecs_to_jiffies(250));
	pr_info("%s: 2nd wireless charging done! vout set %s & headroom offset %dmV!\n",
		__func__, sb_vout_ctr_mode_str(charger->vout_mode),
		charger->pdata->wpc_headroom_ctrl_full ? -600 : 0);
}

static void mfc_set_cep_timeout(struct mfc_charger_data *charger, u32 timeout)
{
	if (timeout) {
		pr_info("%s: timeout = %dmsec\n", __func__, timeout);
		timeout = timeout/100;
		mfc_reg_write(charger->client, MFC_CEP_TIME_OUT_REG, timeout);
	}
}

/* these pads can control fans for sleep/auto mode */
static bool is_sleep_mode_active(int pad_id)
{
	/* standard fw, hv pad, no multiport pad, non hv pad and PG950 pad */
	if (fan_ctrl_pad(pad_id)) {
		pr_info("%s: this pad can control fan for auto mode\n", __func__);
		return 1;
	}

	pr_info("%s: this pad cannot control fan\n", __func__);
	return 0;
}

static void mfc_mis_align(struct mfc_charger_data *charger)
{
	if (charger->wc_checking_align || (charger->rx_op_mode == MFC_RX_MODE_WPC_MPP_CLOAK)) {
		pr_info("%s: skip Reset M0\n", __func__);
		return;
	}

	pr_info("%s: Reset M0\n", __func__);
	/* reset MCU of MFC IC */
	mfc_set_cmd_l_reg(charger, MFC_CMD_MCU_RESET_MASK, MFC_CMD_MCU_RESET_MASK);
}

static bool mfc_tx_function_check(struct mfc_charger_data *charger)
{
	u8 reg_f2, reg_f4;

	mfc_reg_read(charger->client, MFC_TX_RXID1_READ_REG, &reg_f2);
	mfc_reg_read(charger->client, MFC_TX_RXID3_READ_REG, &reg_f4);

	pr_info("@Tx_Mode %s: 0x%x 0x%x\n", __func__, reg_f2, reg_f4);

	if ((reg_f2 == SS_ID) && (reg_f4 == SS_CODE))
		return true;
	else
		return false;
}

static void mfc_set_tx_ioffset(struct mfc_charger_data *charger, int ioffset)
{
	u8 ioffset_data_l = 0, ioffset_data_h = 0;

	ioffset_data_l = 0xFF & ioffset;
	ioffset_data_h = 0xFF & (ioffset >> 8);

	mfc_reg_write(charger->client, MFC_TX_IUNO_OFFSET_L_REG, ioffset_data_l);
	mfc_reg_write(charger->client, MFC_TX_IUNO_OFFSET_H_REG, ioffset_data_h);

	pr_info("@Tx_Mode %s: Tx Iout set %d(0x%x 0x%x)\n", __func__, ioffset, ioffset_data_h, ioffset_data_l);
}

static void mfc_set_tx_iout(struct mfc_charger_data *charger, unsigned int iout)
{
	u8 iout_data_l = 0, iout_data_h = 0;

	iout_data_l = 0xFF & iout;
	iout_data_h = 0xFF & (iout >> 8);

	mfc_reg_write(charger->client, MFC_TX_IUNO_LIMIT_L_REG, iout_data_l);
	mfc_reg_write(charger->client, MFC_TX_IUNO_LIMIT_H_REG, iout_data_h);

	pr_info("@Tx_Mode %s: Tx Iout set %d(0x%x 0x%x)\n", __func__, iout, iout_data_h, iout_data_l);

	if (iout == 1100)
		mfc_set_tx_ioffset(charger, 100);
	else
		mfc_set_tx_ioffset(charger, 150);
}

static void mfc_test_read(struct mfc_charger_data *charger)
{
	u8 reg_data;

	if (!charger->wc_tx_enable)
		return;

	if (mfc_reg_read(charger->client, MFC_STARTUP_EPT_COUNTER, &reg_data) <= 0)
		return;

	pr_info("%s: [AOV] ept-counter = %d\n", __func__, reg_data);
}

static void mfc_set_tx_vout(struct mfc_charger_data *charger, unsigned int vout)
{
	unsigned int vout_val;
	u8 vout_data_l = 0, vout_data_h = 0;

	if (vout < WC_TX_VOUT_MIN) {
		pr_err("%s: vout(%d) is lower than min\n", __func__, vout);
		vout = WC_TX_VOUT_MIN;
	} else if (vout > WC_TX_VOUT_MAX) {
		pr_err("%s: vout(%d) is higher than max\n", __func__, vout);
		vout = WC_TX_VOUT_MAX;
	}

	mfc_test_read(charger);

	if (vout <= 2280)
		vout_val = 0;
	else
		vout_val = (vout - 2280) / 20;
	vout_data_l = 0xFF & vout_val;
	vout_data_h = 0xFF & (vout_val >> 8);

	mfc_reg_write(charger->client, MFC_VOUT_SET_L_REG, vout_data_l);
	mfc_reg_write(charger->client, MFC_VOUT_SET_H_REG, vout_data_h);

	pr_info("@Tx_Mode %s: Tx Vout set %d(0x%x 0x%x)\n", __func__, vout, vout_data_h, vout_data_l);
}

static void mfc_print_buffer(struct mfc_charger_data *charger, u8 *buffer, int size)
{
	int start_idx = 0;

	do {
		char temp_buf[1024] = {0, };
		int size_temp = 0, str_len = 1024;
		int old_idx = start_idx;

		size_temp = ((start_idx + 0x7F) > size) ? size : (start_idx + 0x7F);
		for (; start_idx < size_temp; start_idx++) {
			snprintf(temp_buf + strlen(temp_buf), str_len, "0x%02x ", buffer[start_idx]);
			str_len = 1024 - strlen(temp_buf);
		}
		pr_info("%s: (%04d ~ %04d) %s\n", __func__, old_idx, start_idx - 1, temp_buf);
	} while (start_idx < size);
}

static int mfc_set_psy_wrl(struct mfc_charger_data *charger, int prop, int data)
{
	union power_supply_propval value = { data, };

	return psy_do_property("wireless", set, prop, value);
}

static void mfc_auth_send_adt_status(struct mfc_charger_data *charger, u8 adt_status)
{
	charger->adt_transfer_status = adt_status;
	mfc_set_psy_wrl(charger, POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, adt_status);
}

static void mfc_set_online(struct mfc_charger_data *charger, int type)
{
	charger->pdata->cable_type = type;
	mfc_set_psy_wrl(charger, POWER_SUPPLY_PROP_ONLINE, type);
}

static void mfc_set_tx_phm(struct mfc_charger_data *charger, bool enable)
{
	charger->tx_device_phm = enable;
	mfc_set_psy_wrl(charger, POWER_SUPPLY_EXT_PROP_GEAR_PHM_EVENT, enable);
}

static void mfc_set_rx_power(struct mfc_charger_data *charger, unsigned int rx_power)
{
	if (rx_power > TX_RX_POWER_20W * 100000)
		rx_power = TX_RX_POWER_12W * 100000;

	mfc_set_psy_wrl(charger, POWER_SUPPLY_EXT_PROP_WIRELESS_RX_POWER, rx_power);
}

static void mfc_set_mpp_default_current(struct mfc_charger_data *charger, int input_current)
{
	mfc_set_psy_wrl(charger, POWER_SUPPLY_EXT_PROP_WIRELESS_MPP_PWR, input_current);
}

static int mfc_auth_adt_read(struct mfc_charger_data *charger, u8 *readData)
{
	int buffAddr = MFC_ADT_BUFFER_ADT_PARAM_REG;
	int i = 0, size = 0;
	int ret = 0;
	u8 size_temp[2] = {0, 0};

	ret = mfc_reg_multi_read(charger->client, MFC_ADT_BUFFER_ADT_TYPE_REG, size_temp, 2);
	if (ret < 0) {
		pr_err("%s:  failed to read adt type (ret = %d)\n", __func__, ret);
		return ret;
	}

	adt_readSize = ((size_temp[0] & 0x07) << 8) | (size_temp[1]);
	pr_info("%s %s: (size_temp = 0x%x, readSize = %d bytes)\n",
		WC_AUTH_MSG, __func__, (size_temp[0] | size_temp[1]), adt_readSize);
	if (adt_readSize <= 0) {
		pr_err("%s %s: failed to read adt data size\n", WC_AUTH_MSG, __func__);
		return -EINVAL;
	}
	size = adt_readSize;
	if ((buffAddr + adt_readSize) - 1 > MFC_ADT_BUFFER_ADT_PARAM_MAX_REG) {
		pr_err("%s %s: failed to read adt stream, too large data\n", WC_AUTH_MSG, __func__);
		return -EINVAL;
	}

	if (size <= 2) {
		pr_err("%s %s: data from mcu has invalid size\n", WC_AUTH_MSG, __func__);
		/* notify auth service to send TX PAD a request key */
		mfc_set_psy_wrl(charger,
			POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, WIRELESS_AUTH_FAIL);
		return -EINVAL;
	}

	while (size > SENDSZ) {
		ret = mfc_reg_multi_read(charger->client, buffAddr, readData + i, SENDSZ);
		if (ret < 0) {
			pr_err("%s %s: failed to read adt stream (%d, buff %d)\n",
				WC_AUTH_MSG, __func__, ret, buffAddr);
			return ret;
		}
		i += SENDSZ;
		buffAddr += SENDSZ;
		size = size - SENDSZ;
		pr_info("%s %s: 0x%x %d %d\n", WC_AUTH_MSG, __func__, i, buffAddr, size);
	}

	if (size > 0) {
		ret = mfc_reg_multi_read(charger->client, buffAddr, readData + i, size);
		if (ret < 0) {
			pr_err("%s %s: failed to read adt stream (%d, buff %d)\n",
				WC_AUTH_MSG, __func__, ret, buffAddr);
			return ret;
		}
	}
	mfc_print_buffer(charger, readData, adt_readSize);
	mfc_auth_send_adt_status(charger, WIRELESS_AUTH_RECEIVED);

	pr_info("%s %s: succeeded to read ADT\n", WC_AUTH_MSG, __func__);

	return 0;
}

static int mfc_auth_adt_write(struct mfc_charger_data *charger, u8 *srcData, int srcSize)
{
	int buffAddr = MFC_ADT_BUFFER_ADT_PARAM_REG;
	u8 wdata = 0;
	u8 sBuf[144] = {0,};
	int ret;
	u8 i;

	pr_info("%s %s: start to write ADT\n", WC_AUTH_MSG, __func__);

	if ((buffAddr + srcSize) - 1 > MFC_ADT_BUFFER_ADT_PARAM_MAX_REG) {
		pr_err("%s %s: failed to write adt stream, too large data.\n", WC_AUTH_MSG, __func__);
		return -EINVAL;
	}

	if (charger->rx_op_mode == MFC_RX_MODE_WPC_MPP_FULL) {
		wdata = (MFC_ADT_MPP_OPEN << 3); /*MPP Authentication purpose */
	} else if (charger->rx_op_mode == MFC_RX_MODE_WPC_EPP) {
		if (is_samsung_pad((charger->mpp_epp_tx_id & 0xFF))) {
			wdata = (MFC_ADT_FWC_EPP_GENERAL << 3);/*FWC Authentication purpose */
		} else {
			wdata = (MFC_ADT_FWC_EPP_AUTHENTICATION << 3); /* EPP Authentication purpose */
		}
	} else if (charger->rx_op_mode == MFC_RX_MODE_WPC_BPP) {
		wdata = (MFC_ADT_FWC_EPP_GENERAL << 3);/*FWC Authentication purpose */
	} else {
		pr_info("%s %s: error Rx mode%d\n", WC_AUTH_MSG, __func__, charger->rx_op_mode);
		return -EINVAL;
	}

	pr_info("%s %s: wdata(0x%x)\n", WC_AUTH_MSG, __func__, wdata);
	wdata = wdata | ((srcSize >> 8) & 0x07);
	mfc_reg_write(charger->client, MFC_ADT_BUFFER_ADT_TYPE_REG, wdata);

	wdata = srcSize; /* need to check */
	mfc_reg_write(charger->client, MFC_ADT_BUFFER_ADT_MSG_SIZE_REG, wdata);

	buffAddr = MFC_ADT_BUFFER_ADT_PARAM_REG;
	for (i = 0; i < srcSize; i += 128) { //write each 128byte
		int restSize = srcSize - i;

		if (restSize < 128) {
			memcpy(sBuf, srcData + i, restSize);
			ret = mfc_reg_multi_write_verify(charger->client, buffAddr, sBuf, restSize);
			if (ret < 0) {
				pr_err("%s %s: failed to write adt stream (%d, buff %d)\n",
					WC_AUTH_MSG, __func__, ret, buffAddr);
				return ret;
			}
			break;
		}

		memcpy(sBuf, srcData + i, 128);
		ret = mfc_reg_multi_write_verify(charger->client, buffAddr, sBuf, 128);
		if (ret < 0) {
			pr_err("%s %s: failed to write adt stream (%d, buff %d)\n",
				WC_AUTH_MSG, __func__, ret, buffAddr);
			return ret;
		}
		buffAddr += 128;

		if (buffAddr > MFC_ADT_BUFFER_ADT_PARAM_MAX_REG - 128)
			break;
	}
	pr_info("%s %s: succeeded to write ADT\n", WC_AUTH_MSG, __func__);
	return 0;
}

static void mfc_auth_adt_send(struct mfc_charger_data *charger, u8 *srcData, int srcSize)
{
	u8 temp;
	int ret = 0;

	charger->adt_transfer_status = WIRELESS_AUTH_SENT;
	ret = mfc_auth_adt_write(charger, srcData, srcSize); /* write buff fw datas to send fw datas to tx */

	mfc_reg_read(charger->client, MFC_AP2MFC_CMD_H_REG, &temp); // check out set bit exists
	pr_info("%s %s: MFC_CMD_H_REG(0x%x)\n", WC_AUTH_MSG, __func__, temp);
	temp |= MFC_CMD2_ADT_SENT_MASK;
	pr_info("%s %s: MFC_CMD_H_REG(0x%x)\n", WC_AUTH_MSG, __func__, temp);
	mfc_reg_write(charger->client, MFC_AP2MFC_CMD_H_REG, temp);

	mfc_reg_read(charger->client, MFC_AP2MFC_CMD_H_REG, &temp); // check out set bit exists
	pr_info("%s %s: MFC_CMD_H_REG(0x%x)\n", WC_AUTH_MSG, __func__, temp);

	mfc_reg_update(charger->client, MFC_INT_A_ENABLE_H_REG,
					MFC_STAT_H_ADT_SENT_MASK, MFC_STAT_H_ADT_SENT_MASK);
}

#define AUTH_READY 0
#define AUTH_COMPLETE 1
#define AUTH_OPFREQ 140
static void mfc_auth_set_configs(struct mfc_charger_data *charger, int opt)
{
	u8 data = 0;

	if (opt == AUTH_READY) {
		int op_freq = mfc_get_adc(charger, MFC_ADC_OP_FRQ);

		pr_info("%s: op_freq(%d)\n", __func__, op_freq);
		mfc_cmfet_set_auth(charger->cmfet, true);
		if ((charger->tx_id == TX_ID_P5200_PAD) && (op_freq >= AUTH_OPFREQ)) {
			mfc_reg_write(charger->client, MFC_RECT_MODE_AP_CTRL, 0x01);
			mfc_reg_update(charger->client, MFC_RECTMODE_REG, 0x40, 0xC0);
			mfc_reg_read(charger->client, MFC_RECTMODE_REG, &data);
		}
	} else if (opt == AUTH_COMPLETE) {
		if (charger->tx_id == TX_ID_P5200_PAD) {
			mfc_reg_write(charger->client, MFC_RECT_MODE_AP_CTRL, 0x00);
			mfc_reg_read(charger->client, MFC_RECTMODE_REG, &data);
		}
		mfc_cmfet_set_auth(charger->cmfet, false);
	} else {
		pr_info("%s: undefined cmd(%d)\n", __func__, opt);
	}
}

/* uno on/off control function */
static void mfc_set_tx_power(struct mfc_charger_data *charger, bool on)
{
	union power_supply_propval value = {0, };

	charger->wc_tx_enable = on;

	if (on) {
		u32 tx_max_op_freq = charger->pdata->tx_max_op_freq,
			tx_min_op_freq = charger->pdata->tx_min_op_freq;

		pr_info("@Tx_Mode %s: Turn On TX Power\n", __func__);
		mfc_set_coil_sw_en(charger, 1);
		mfc_uno_on(charger, true);
		msleep(200);

		charger->otp_firmware_ver = mfc_get_firmware_version(charger, MFC_RX_FIRMWARE);
		charger->wc_rx_fod = false;
		if (delayed_work_pending(&charger->wpc_tx_duty_min_work)) {
			__pm_relax(charger->wpc_tx_duty_min_ws);
			cancel_delayed_work(&charger->wpc_tx_duty_min_work);
		}

		/* Set TX OP Freq (MAX/MIN) */
		if (tx_max_op_freq > 0)
			mfc_set_tx_max_op_freq(charger, tx_max_op_freq);
		if (tx_min_op_freq > 0)
			mfc_set_tx_min_op_freq(charger, tx_min_op_freq);
	} else {
		pr_info("@Tx_Mode %s: Turn Off TX Power, and reset UNO config\n", __func__);

		mfc_uno_on(charger, false);
		mfc_set_coil_sw_en(charger, 0);

		value.intval = WC_TX_VOUT_5000MV;
		psy_do_property("otg", set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);

		charger->wc_rx_connected = false;
		charger->wc_rx_type = NO_DEV;
		charger->duty_min = MIN_DUTY_SETTING_20_DATA;
		charger->tx_device_phm = 0;

		alarm_cancel(&charger->phm_alarm);
		if (delayed_work_pending(&charger->wpc_rx_type_det_work)) {
			cancel_delayed_work(&charger->wpc_rx_type_det_work);
			__pm_relax(charger->wpc_rx_det_ws);
		}
		if (delayed_work_pending(&charger->wpc_rx_connection_work))
			cancel_delayed_work(&charger->wpc_rx_connection_work);
		if (delayed_work_pending(&charger->wpc_tx_isr_work)) {
			cancel_delayed_work(&charger->wpc_tx_isr_work);
			__pm_relax(charger->wpc_tx_ws);
		}
		if (delayed_work_pending(&charger->wpc_tx_phm_work)) {
			cancel_delayed_work(&charger->wpc_tx_phm_work);
			__pm_relax(charger->wpc_tx_phm_ws);
		}
		if (delayed_work_pending(&charger->mode_change_work)) {
			cancel_delayed_work(&charger->mode_change_work);
			__pm_relax(charger->mode_change_ws);
		}

		charger->tx_status = SEC_TX_OFF;
	}
}

/* determine rx connection status with tx sharing mode */
static void mfc_wpc_rx_connection_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_rx_connection_work.work);

	if (!charger->wc_tx_enable) {
		pr_info("@Tx_Mode %s : wc_tx_enable(%d)\n", __func__, charger->wc_tx_enable);
		charger->wc_rx_connected = false;
		return;
	}

	if (charger->wc_rx_connected) {
		pr_info("@Tx_Mode %s: Rx(%s) connected\n",
			__func__, mfc_tx_function_check(charger) ? "Samsung" : "Other");

		cancel_delayed_work(&charger->wpc_rx_type_det_work);
		__pm_stay_awake(charger->wpc_rx_det_ws);
		queue_delayed_work(charger->wqueue, &charger->wpc_rx_type_det_work, msecs_to_jiffies(1000));

		__pm_stay_awake(charger->wpc_tx_duty_min_ws);
		queue_delayed_work(charger->wqueue, &charger->wpc_tx_duty_min_work, msecs_to_jiffies(5000));

		pr_info("%s: tx op freq = %dKhz\n", __func__, mfc_get_adc(charger, MFC_ADC_TX_MAX_OP_FRQ));
		charger->duty_min = MIN_DUTY_SETTING_30_DATA;
		mfc_set_min_duty(charger, MIN_DUTY_SETTING_30_DATA);
	} else {
		pr_info("@Tx_Mode %s: Rx disconnected\n", __func__);

		mfc_set_coil_sw_en(charger, 1);
		charger->wc_rx_fod = false;
		charger->wc_rx_type = NO_DEV;
		charger->tx_device_phm = 0;

		if (delayed_work_pending(&charger->wpc_rx_type_det_work)) {
			__pm_relax(charger->wpc_rx_det_ws);
			cancel_delayed_work(&charger->wpc_rx_type_det_work);
		}

		if (delayed_work_pending(&charger->wpc_tx_duty_min_work)) {
			__pm_relax(charger->wpc_tx_duty_min_ws);
			cancel_delayed_work(&charger->wpc_tx_duty_min_work);
		}
	}

	/* set rx connection condition */
	mfc_set_psy_wrl(charger, POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONNECTED, charger->wc_rx_connected);
}

/*
 * determine rx connection status with tx sharing mode,
 * this TX device has MFC_INTA_H_TRX_DATA_RECEIVED_MASK irq and RX device is connected HV wired cable
 */
static void mfc_tx_handle_rx_packet(struct mfc_charger_data *charger)
{
	u8 cmd_data = 0, val_data = 0, val2_data = 0;
	union power_supply_propval value = {0, };

	mfc_reg_read(charger->client, MFC_WPC_TRX_DATA2_COM_REG, &cmd_data);
	mfc_reg_read(charger->client, MFC_WPC_TRX_DATA2_VALUE0_REG, &val_data);
	mfc_reg_read(charger->client, MFC_WPC_TRX_DATA2_VALUE1_REG, &val2_data);
	charger->pdata->trx_data_cmd = cmd_data;
	charger->pdata->trx_data_val = val_data;

	pr_info("@Tx_Mode %s: CMD : 0x%x, DATA : 0x%x, DATA2 : 0x%x\n",
			__func__, cmd_data, val_data, val2_data);

	/* When RX device has got a AFC TA, this TX device should turn off TX power sharing(uno) */
	if (cmd_data == MFC_HEADER_AFC_CONF) { /* 0x28 */
		switch (val_data) {
		case WPC_COM_DISABLE_TX: /* 0x19 Turn off UNO of TX */
			if (val2_data == RX_DATA_VAL2_MISALIGN) {
				psy_do_property("wireless", get, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR, value);
				if (value.intval & BATT_TX_EVENT_WIRELESS_TX_RETRY) {
					pr_info("@Tx_Mode %s: ignore misalign packet during TX retry\n", __func__);
					return;
				}
				pr_info("@Tx_Mode %s: RX send TX off packet(Misalign or darkzone)\n", __func__);
				value.intval = BATT_TX_EVENT_WIRELESS_TX_MISALIGN;
				psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR, value);
			} else if (val2_data == RX_DATA_VAL2_TA_CONNECT_DURING_WC) {
				pr_info("@Tx_Mode %s: TX dev should turn off TX(uno), RX dev has AFC TA\n", __func__);
				value.intval = BATT_TX_EVENT_WIRELESS_RX_CHG_SWITCH;
				psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR, value);
			}
			break;
		case WPC_COM_ENTER_PHM: /* 0x18 Received PHM, GEAR sent PHM packet to TX */
			if (val2_data == RX_DATA_VAL2_ENABLE)
				pr_info("@Tx_Mode %s: Received PHM\n", __func__);
			break;
		case WPC_COM_RX_ID: /* 0x0E Received RX ID */
			if (val2_data >= RX_DATA_VAL2_RXID_ACC_BUDS && val2_data <= RX_DATA_VAL2_RXID_ACC_BUDS_MAX) {
				charger->wc_rx_type = SS_BUDS;
				pr_info("@Tx_Mode %s: Buds connected\n", __func__);
				mfc_set_tx_fod_thresh1(charger->client, charger->pdata->buds_fod_thresh1);
				mfc_set_tx_fod_ta_thresh(charger->client, charger->pdata->buds_fod_ta_thresh);
				value.intval = charger->wc_rx_type;
				psy_do_property("wireless", set,
						POWER_SUPPLY_EXT_PROP_WIRELESS_RX_TYPE, value);
			}
			break;
		default:
			pr_info("@Tx_Mode %s: unsupport : 0x%x", __func__, val_data);
			break;
		}
	} else if (cmd_data == MFC_HEADER_END_CHARGE_STATUS) {
		if (val_data == MFC_ECS_CS100) {	/* CS 100 */
			pr_info("@Tx_Mode %s: CS100 Received, TX power off\n", __func__);
			value.intval = BATT_TX_EVENT_WIRELESS_RX_CS100;
			psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR, value);
		}
	} else if (cmd_data == MFC_HEADER_END_POWER_TRANSFER) {
		if (val_data == MFC_EPT_CODE_OVER_TEMPERATURE) {
			pr_info("@Tx_Mode %s: EPT-OT Received, TX power off\n", __func__);
			value.intval = BATT_TX_EVENT_WIRELESS_RX_UNSAFE_TEMP;
			psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR, value);
		} else if (val_data == MFC_EPT_CODE_BATTERY_FAILURE) {
			if (charger->wc_rx_type != SS_GEAR) {
				pr_info("@Tx_Mode %s: EPT06 Received, TX power off and retry\n", __func__);
				value.intval = BATT_TX_EVENT_WIRELESS_TX_RETRY;
				psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR, value);
			}
		}
	} else if (cmd_data == MFC_HEADER_PROPRIETARY_1_BYTE) {
		if (val_data == WPC_COM_WDT_ERR) {
			pr_info("@Tx_Mode %s: WDT Error Received, TX power off\n", __func__);
			value.intval = BATT_TX_EVENT_WIRELESS_TX_ETC;
			psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR, value);
		}
	}
}

static int datacmp(const char *cs, const char *ct, int count)
{
	unsigned char c1, c2;

	while (count) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2) {
			pr_err("%s: cnt %d\n", __func__, count);
			return c1 < c2 ? -1 : 1;
		}
		count--;
	}
	return 0;
}

static int mfc_reg_multi_write_verify(struct i2c_client *client, u16 reg, const u8 *val, int size)
{
	int ret = 0;
	int cnt = 0;
	int retry_cnt = 0;
	unsigned char data[SENDSZ + 2];
	u8 rdata[SENDSZ + 2];
	struct mfc_charger_data *charger = i2c_get_clientdata(client);

	if (charger->reg_access_lock) {
		pr_err("%s: can not access to reg during fw update\n", __func__);
		return -1;
	}

	while (size > SENDSZ) {
		data[0] = (reg + cnt) >> 8;
		data[1] = (reg + cnt) & 0xFF;
		memcpy(data + 2, val + cnt, SENDSZ);
//		dev_dbg(&client->dev, "%s: addr: 0x%x, cnt: 0x%x\n", __func__, reg + cnt, cnt);
		ret = i2c_master_send(client, data, SENDSZ + 2);
		if (ret < SENDSZ + 2) {
			pr_err("%s: i2c write error, reg: 0x%x\n", __func__, reg);
			return ret < 0 ? ret : -EIO;
		}
		if (mfc_reg_multi_read(client, reg + cnt, rdata, SENDSZ) < 0) {
			pr_err("%s: read failed(%d)\n", __func__, reg + cnt);
			return -1;
		}
		if (datacmp(val + cnt, rdata, SENDSZ)) {
			pr_err("%s: data is not matched. retry(%d)\n", __func__, retry_cnt);
			retry_cnt++;
			if (retry_cnt > 4) {
				pr_err("%s: data is not matched. write failed\n", __func__);
				retry_cnt = 0;
				return -1;
			}
			continue;
		}
//		pr_debug("%s: data is matched!\n", __func__);
		cnt += SENDSZ;
		size -= SENDSZ;
		retry_cnt = 0;
	}
	while (size > 0) {
		data[0] = (reg + cnt) >> 8;
		data[1] = (reg + cnt) & 0xFF;
		memcpy(data + 2, val + cnt, size);
//		dev_dbg(&client->dev, "%s: addr: 0x%x, cnt: 0x%x, size: 0x%x\n", __func__, reg + cnt, cnt, size);
		ret = i2c_master_send(client, data, size + 2);
		if (ret < size + 2) {
			pr_err("%s: i2c write error, reg: 0x%x\n", __func__, reg);
			return ret < 0 ? ret : -EIO;
		}
		if (mfc_reg_multi_read(client, reg + cnt, rdata, size) < 0) {
			pr_err("%s: read failed(%d)\n", __func__, reg + cnt);
			return -1;
		}
		if (datacmp(val + cnt, rdata, size)) {
			pr_err("%s: data is not matched. retry(%d)\n", __func__, retry_cnt);
			retry_cnt++;
			if (retry_cnt > 4) {
				pr_err("%s: data is not matched. write failed\n", __func__);
				retry_cnt = 0;
				return -1;
			}
			continue;
		}
		size = 0;
		pr_err("%s: data is matched!\n", __func__);
	}
	return ret;
}

#if 0
static int mfc_reg_multi_write(struct i2c_client *client, u16 reg, const u8 *val, int size)
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);
	int ret = 0;
	unsigned char data[SENDSZ + 2];
	int cnt = 0;

	pr_err("%s: size: 0x%x\n", __func__, size);
	while (size > SENDSZ) {
		data[0] = (reg + cnt) >> 8;
		data[1] = (reg + cnt) & 0xff;
		memcpy(data + 2, val + cnt, SENDSZ);
		mutex_lock(&charger->io_lock);
		ret = i2c_master_send(client, data, SENDSZ + 2);
		mutex_unlock(&charger->io_lock);
		if (ret < SENDSZ + 2) {
			pr_err("%s: i2c write error, reg: 0x%x\n", __func__, reg);
			return ret < 0 ? ret : -EIO;
		}
		cnt = cnt + SENDSZ;
		size = size - SENDSZ;
	}
	if (size > 0) {
		data[0] = (reg + cnt) >> 8;
		data[1] = (reg + cnt) & 0xff;
		memcpy(data + 2, val + cnt, size);
		mutex_lock(&charger->io_lock);
		ret = i2c_master_send(client, data, size + 2);
		mutex_unlock(&charger->io_lock);
		if (ret < size + 2) {
			dev_err(&client->dev, "%s: i2c write error, reg: 0x%x\n", __func__, reg);
			return ret < 0 ? ret : -EIO;
		}
	}

	return ret;
}
#endif

static int mfc_cps_wls_write_word(struct i2c_client *client, u32 reg, u32 val)
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);
	int ret = 0;
	unsigned char data[8];

	data[0] = reg >> 24;
	data[1] = (reg >> 16) & 0xff;
	data[2] = (reg >> 8) & 0xff;
	data[3] = reg & 0xff;
	data[4] = val & 0xff;
	data[5] = (val >> 8) & 0xff;
	data[6] = (val >> 16) & 0xff;
	data[7] = (val >> 24) & 0xff;
	mutex_lock(&charger->io_lock);
	ret = i2c_master_send(client, data, 8);
	mutex_unlock(&charger->io_lock);
	if (ret < 8) {
		dev_err(&client->dev, "%s: i2c write error, reg: 0x%x\n", __func__, reg);
		return ret < 0 ? ret : -EIO;
	}

	return ret;
}

static int mfc_cps_wls_read_word(struct i2c_client *client, u32 reg, u8 *val)
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);
	int ret;
	struct i2c_msg msg[2];
	u8 wbuf[4];

	msg[0].addr = client->addr;
	msg[0].flags = client->flags & I2C_M_TEN;
	msg[0].len = 4;
	msg[0].buf = wbuf;

	wbuf[0] = reg >> 24;
	wbuf[1] = (reg >> 16) & 0xff;
	wbuf[2] = (reg >> 8) & 0xff;
	wbuf[3] = reg & 0xff;
	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 4;
	msg[1].buf = val;

	mutex_lock(&charger->io_lock);
	ret = i2c_transfer(client->adapter, msg, 2);
	mfc_check_i2c_error(charger, (ret < 0));
	mutex_unlock(&charger->io_lock);
	if (ret < 0) {
		pr_err("%s: i2c transfer fail", __func__);
		return -1;
	}

	return ret;
}
static void mfc_cps_cmd_send(struct i2c_client *client, u32 cmd)
{
	//struct mfc_charger_data *charger = i2c_get_clientdata(client);
	//u8 data[4];
	//u32 res;

	pr_info("%s: ---> send_cmd %02x\n", __func__, cmd);
	mfc_cps_wls_write_word(client, ADDR_CMD, cmd);
}

static int mfc_cps_wls_cmd_wait(struct i2c_client *client)
{
	int res;
	//struct mfc_charger_data *charger = i2c_get_clientdata(client);
	int cnt = 0;
	u8 data[4];

	while (1) {
		usleep_range(1000, 1100);
		if (mfc_cps_wls_read_word(client, ADDR_FLAG, data) < 0) {
			pr_err("%s: read failed(%d)\n", __func__, ADDR_FLAG);
			return -1;
		}
		res = data[0];
		switch (res) {
		case RUNNING:
		case PASS:
			break;
		case FAIL:
			pr_info("%s: ---> FAIL : 0x%x\n", __func__, res);
			return res;
		case ILLEGAL:
			pr_info("%s: ---> ILLEGAL : 0x%x\n", __func__, res);
			return res;
		default:
			pr_info("%s: ---> ERROR-CODE : 0x%x\n", __func__, res);
			return res;
		}
		if (res == PASS)
			break;
	    /*3s over time*/
		if ((cnt++) == 3000) {
			pr_info("%s: ---> CMD-OVERTIME\n", __func__);
			break;
		}
	}
	return res;
}

static int mfc_cps_wls_program_sram(struct i2c_client *client, int reg, const u8 *wdate, int size)
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);
	int ret = 0;
	u8 data[SENDSZ + 4];
	int cnt = 0;

	pr_info("%s: size: 0x%x\n", __func__, size);
	while (size > SENDSZ) {
		data[0] = (reg+cnt) >> 24;
		data[1] = ((reg+cnt) >> 16) & 0xff;
		data[2] = ((reg+cnt) >> 8) & 0xff;
		data[3] = (reg+cnt) & 0xff;
		memcpy(data + 4, wdate + cnt, SENDSZ);
		mutex_lock(&charger->io_lock);
		ret = i2c_master_send(client, data, SENDSZ + 4);
		mutex_unlock(&charger->io_lock);
		if (ret < SENDSZ + 4) {
			pr_err("%s: i2c write error, reg: 0x%x\n", __func__, (reg + cnt));
			return ret < 0 ? ret : -EIO;
		}
		cnt = cnt + SENDSZ;
		size = size - SENDSZ;
	}
	if (size > 0) {
		data[0] = (reg+cnt) >> 24;
		data[1] = ((reg+cnt) >> 16) & 0xff;
		data[2] = ((reg+cnt) >> 8) & 0xff;
		data[3] = (reg+cnt) & 0xff;
		memcpy(data + 4, wdate + cnt, size);
		mutex_lock(&charger->io_lock);
		ret = i2c_master_send(client, data, size + 4);
		mutex_unlock(&charger->io_lock);
		if (ret < size + 4) {
			pr_err("%s: i2c write error, reg: 0x%x\n", __func__, (reg + cnt));
			return ret < 0 ? ret : -EIO;
		}
	}

	return ret;
}

static int PgmOTPwRAM_CPS(struct mfc_charger_data *charger, unsigned short OtpAddr,
					  const u8 *srcData, int srcOffs, int fw_size)
{
	int addr;
	int i, j;
	int ret, retry = 3;
	u8 fw_ver[4] = {0, };
	u8 fw_ver_bin[4] = {0, };
    //int flag = 0;
	int cfg_buf_size;
	int buf0_flag;
	int buf1_flag;
	int result;
	int def_fw_size = 24 * 1024; // 24KB
	//int write_count = 0;

	mfc_reg_read(charger->client, MFC_FW_MAJOR_REV_L_REG, &fw_ver[0]);
	mfc_reg_read(charger->client, MFC_FW_MAJOR_REV_H_REG, &fw_ver[1]);
	pr_info("%s BEFORE rx major firmware version 0x%x\n",
		__func__, fw_ver[0] | (fw_ver[1] << 8));

	mfc_reg_read(charger->client, MFC_FW_MINOR_REV_L_REG, &fw_ver[2]);
	mfc_reg_read(charger->client, MFC_FW_MINOR_REV_H_REG, &fw_ver[3]);
	pr_info("%s BEFORE rx minor firmware version 0x%x\n",
		__func__, fw_ver[2] | (fw_ver[3] << 8));

	/* get major and minor firmware version from firmware file */
	memcpy(fw_ver_bin, &srcData[MFC_FW_VER_BIN_ADDRESS], 4);

	pr_info("%s NEW rx major firmware version 0x%x\n",
		__func__, fw_ver_bin[0] | (fw_ver_bin[1] << 8));
	pr_info("%s NEW rx minor firmware version 0x%x\n",
		__func__, fw_ver_bin[2] | (fw_ver_bin[3] << 8));

	/*
	 * block to access regisgter map during fw update
	 * register-map is not available since SRAM is usued as buffer or bootloader
	 */
	charger->reg_access_lock = true;

	/***************************************************************************************
     *                                  Step1, load to sram                                *
     ***************************************************************************************/
	mfc_cps_wls_write_word(charger->client, 0xFFFFFF00, 0x0000000E); /*enable 32bit i2c*/
	mfc_cps_wls_write_word(charger->client, 0x4000E75C, 0x00001250); /*write password*/
	mfc_cps_wls_write_word(charger->client, 0x40040010, 0x00000006); /*reset and halt mcu*/
	mfc_cps_wls_write_word(charger->client, 0x4000E01C, 0x000003E8); /*config SDA timeout to 1s*/
	pr_info("%s: START LOAD SRAM HEX\n", __func__);
	mfc_cps_wls_program_sram(charger->client, 0x20000000, CPS4038_BL, 0x0800);

	mfc_cps_wls_write_word(charger->client, 0x400400A0, 0x000000FF); /*enable remap function*/
	mfc_cps_wls_write_word(charger->client, 0x40040010, 0x00008003); /*triming load function is disabled and run mcu*/

	usleep_range(10000, 11000);

	mfc_cps_wls_write_word(charger->client, 0xFFFFFF00, 0x0000000E); /*enable 32bit i2c*/
	mfc_cps_wls_write_word(charger->client, 0x4000E01C, 0x000003E8); /*config SDA timeout to 1s*/

	usleep_range(10000, 11000);

	/***************************************************************************************
     *                          Step2, bootloader crc check                                *
	***************************************************************************************/
	mfc_cps_cmd_send(charger->client, CACL_CRC_TEST);
	result = mfc_cps_wls_cmd_wait(charger->client);

	if (result != PASS) {
		pr_err("%s: ---> BOOTLOADER CRC FAIL\n", __func__);
		goto fw_update_fail;
	}

	pr_info("%s: ---> LOAD BOOTLOADER SUCCESSFUL\n", __func__);

	/***************************************************************************************
     *                          Step3, load firmware to MTP                                *
	***************************************************************************************/
	pr_info("%s: START LOAD APP HEX\n", __func__);
	buf0_flag = 0;
	buf1_flag = 0;
	cfg_buf_size = 512;
	addr = 0;
	mfc_cps_wls_write_word(charger->client, ADDR_BUF_SIZE, cfg_buf_size);

	if (fw_size < def_fw_size) {
		pr_err("%s: ---> FW Size(%d) TOO SMALL, Expected(%d)\n", __func__, fw_size, def_fw_size);
		goto fw_update_fail;
	}

	result = mfc_cps_wls_cmd_wait(charger->client);
	if (result != PASS) {
		pr_err("%s: ---> ERASE FAIL\n", __func__);
		goto fw_update_fail;
	}

	for (i = 0; i < (def_fw_size)/4/cfg_buf_size; i++) {
		if (buf0_flag == 0) {
			mfc_cps_wls_program_sram(charger->client, ADDR_BUFFER0, srcData+addr, cfg_buf_size*4);
			addr  = addr + cfg_buf_size*4;

			if (buf1_flag == 1) {
				result = mfc_cps_wls_cmd_wait(charger->client);
				if (result != PASS) {
					pr_err("%s: ---> WRITE BUFFER1 DATA TO MTP FAIL\n", __func__);
					goto fw_update_fail;
				}
				buf1_flag = 0;
			}
			mfc_cps_cmd_send(charger->client, PGM_BUFFER0);
			buf0_flag = 1;
			continue;
		}

		if (buf1_flag == 0) {
			mfc_cps_wls_program_sram(charger->client, ADDR_BUFFER1, srcData+addr, cfg_buf_size*4);
			addr  = addr + cfg_buf_size*4;

			if (buf0_flag == 1) {
				result = mfc_cps_wls_cmd_wait(charger->client);
				if (result != PASS) {
					pr_err("%s: ---> WRITE BUFFER0 DATA TO MTP FAIL\n", __func__);
					goto fw_update_fail;
				}
				buf0_flag = 0;
			}
			mfc_cps_cmd_send(charger->client, PGM_BUFFER1);
			buf1_flag = 1;
			continue;
		}
	}

	if (buf0_flag == 1) {
		result = mfc_cps_wls_cmd_wait(charger->client);
		if (result != PASS) {
			pr_err("%s: ---> WRITE BUFFER0 DATA TO MTP FAIL\n", __func__);
			goto fw_update_fail;
		}
		buf0_flag = 0;
	}

	if (buf1_flag == 1) {
		result = mfc_cps_wls_cmd_wait(charger->client);
		if (result != PASS) {
			pr_err("%s: ---> WRITE BUFFER1 DATA TO MTP FAIL\n", __func__);
			return MFC_FWUP_ERR_FAIL;
		}
		buf1_flag = 0;
	}
	pr_info("%s: ---> LOAD APP HEX SUCCESSFUL\n", __func__);
	/***************************************************************************************
     *                          Step4, check app CRC                                       *
	***************************************************************************************/
	mfc_cps_cmd_send(charger->client, CACL_CRC_APP);
	result = mfc_cps_wls_cmd_wait(charger->client);

	if (result != PASS) {
		pr_err("%s: ---> APP CRC FAIL\n", __func__);
		goto fw_update_fail;
	}
	pr_info("%s: ---> CHERK APP CRC SUCCESSFUL\n", __func__);
	/***************************************************************************************
     *                          Step5, write mcu start flag                                *
	***************************************************************************************/
	mfc_cps_cmd_send(charger->client, PGM_WR_FLAG);
	result = mfc_cps_wls_cmd_wait(charger->client);
	if (result != PASS) {
		pr_err("%s:---> WRITE MCU START FLAG FAIL\n", __func__);
		goto fw_update_fail;
	}

	pr_info("%s: ---> WRITE MCU START FLAG SUCCESSFUL\n", __func__);
	mfc_cps_wls_write_word(charger->client, 0x40040010, 0x00000008); /* reset all system */

	msleep(100);

	mfc_cps_wls_write_word(charger->client, 0xFFFFFF00, 0x00000000); /* i2c 32bit mode disable */

	charger->reg_access_lock = false;

	for (i = 0; i < retry; i++) {
		usleep_range(10000, 11000);
		ret = MFC_FWUP_ERR_SUCCEEDED;

		mfc_reg_read(charger->client, MFC_FW_MAJOR_REV_L_REG, &fw_ver[0]);
		mfc_reg_read(charger->client, MFC_FW_MAJOR_REV_H_REG, &fw_ver[1]);
		pr_info("%s NOW rx major firmware version 0x%x\n",
			__func__, fw_ver[0] | (fw_ver[1] << 8));

		mfc_reg_read(charger->client, MFC_FW_MINOR_REV_L_REG, &fw_ver[2]);
		mfc_reg_read(charger->client, MFC_FW_MINOR_REV_H_REG, &fw_ver[3]);
		pr_info("%s NOW rx minor firmware version 0x%x\n",
			__func__, fw_ver[2] | (fw_ver[3] << 8));

		for (j = 0; j < 4; j++)
			if (fw_ver[j] != fw_ver_bin[j])
				ret = MFC_FWUP_ERR_COMMON_FAIL;

		if (ret != MFC_FWUP_ERR_COMMON_FAIL)
			break;
		else {
			mfc_uno_on(charger, false);
			msleep(200);
			mfc_uno_on(charger, true);
			msleep(200);
		}
	}

	return ret;

fw_update_fail:
	charger->reg_access_lock = false;
	return MFC_FWUP_ERR_FAIL;
}

static void mfc_reset_rx_power(struct mfc_charger_data *charger, u8 rx_power)
{
#if defined(CONFIG_SEC_FACTORY)
	if (delayed_work_pending(&charger->wpc_rx_power_work))
		cancel_delayed_work(&charger->wpc_rx_power_work);
#endif

	if (charger->adt_transfer_status == WIRELESS_AUTH_PASS)
		mfc_set_rx_power(charger, rx_power * 100000);
	else
		pr_info("%s %s: undefine rx power scenario, It is auth failed case how dose it get rx power?\n",
			WC_AUTH_MSG, __func__);
}

static void mfc_wpc_rx_power_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_rx_power_work.work);

	pr_info("%s: rx power = %d, This W/A is only for Factory\n", __func__, charger->max_power_by_txid);
	mfc_set_rx_power(charger, charger->max_power_by_txid);
}

static void mfc_set_pad_hv(struct mfc_charger_data *charger, bool set_hv)
{
	if (!is_hv_wireless_type(charger->pdata->cable_type) ||
		(charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP) ||
		(charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_EPP))
		return;

	if (set_hv && charger->is_full_status)
		return;

	if (set_hv) {
		if (charger->pdata->cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20)
			mfc_send_command(charger, charger->vrect_by_txid);
		else
			mfc_send_command(charger, MFC_AFC_CONF_10V_TX);

	} else {
		mfc_send_command(charger, MFC_AFC_CONF_5V_TX);
	}
	charger->is_afc_tx = set_hv;
}

static void mfc_recover_vout_by_pad(struct mfc_charger_data *charger)
{
	if (!is_hv_wireless_type(charger->pdata->cable_type) ||
		(charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP))
		return;

	if (charger->is_full_status)
		return;

	if (charger->vout_mode != WIRELESS_VOUT_5V &&
		charger->vout_mode != WIRELESS_VOUT_5V_STEP &&
		charger->vout_mode != WIRELESS_VOUT_5_5V_STEP &&
		charger->vout_mode != WIRELESS_VOUT_OTG) {
		// need to fix here
		if ((charger->pdata->cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20) ||
			(charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_EPP))
			charger->vout_mode = charger->vout_by_txid;
		else
			charger->vout_mode = WIRELESS_VOUT_10V;

		if (!charger->is_otg_on) {
			cancel_delayed_work(&charger->wpc_vout_mode_work);
			__pm_stay_awake(charger->wpc_vout_mode_ws);
			queue_delayed_work(charger->wqueue, &charger->wpc_vout_mode_work, 0);

			if ((charger->pdata->cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20) &&
				(charger->tx_id == TX_ID_FG_PAD)) {
				pr_info("%s: set power = %d\n", __func__, charger->current_rx_power);
				mfc_reset_rx_power(charger, charger->current_rx_power);
			}
		}
	} else if (charger->sleep_mode && (charger->mpp_epp_nego_done_power >= TX_RX_POWER_8W)) {
		cancel_delayed_work(&charger->wpc_vout_mode_work);
		__pm_stay_awake(charger->wpc_vout_mode_ws);
		queue_delayed_work(charger->wqueue, &charger->wpc_vout_mode_work, 0);
	}
}

static void mfc_wpc_afc_vout_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_afc_vout_work.work);

	pr_info("%s: start, current cable(%d)\n", __func__, charger->pdata->cable_type);

	/* change cable type */
	if (charger->pdata->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV)
		charger->pdata->cable_type = SEC_BATTERY_CABLE_HV_WIRELESS;

	mfc_set_online(charger, charger->pdata->cable_type);

	pr_info("%s: check OTG(%d), full(%d) tx_id(0x%x)\n",
		__func__, charger->is_otg_on, charger->is_full_status, charger->tx_id);

	if (charger->is_full_status && volt_ctrl_pad(charger->tx_id)) {
		pr_info("%s: skip voltgate set to pad, full status with PG950 pad\n", __func__);
		goto skip_set_afc_vout;
	}
	// need to fix here to get vout setting
	mfc_set_pad_hv(charger, true);
	mfc_recover_vout_by_pad(charger);
	pr_info("%s: is_afc_tx = %d vout read = %d\n", __func__,
			charger->is_afc_tx, mfc_get_adc(charger, MFC_ADC_VOUT));
skip_set_afc_vout:
	__pm_relax(charger->wpc_afc_vout_ws);
}

static void mfc_wpc_fw_update_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_fw_update_work.work);
	union power_supply_propval value = {0, };
	int ret = 0;
	int i = 0;
	bool is_changed = false;
	char fwtime[8] = {32, 32, 32, 32, 32, 32, 32, 32};
	char fwdate[8] = {32, 32, 32, 32, 32, 32, 32, 32};
	u8 data = 32; /* ascii space */
	const char *fw_path = MFC_FLASH_FW_HEX_CPS_PATH;
	size_t fw_size;
	const struct firmware *firm_data_bin;
	unsigned int bin_fw_ver;
	u8 pgmCnt = 0;
	bool repairEn = false;

	pr_info("%s: firmware update mode is = %d\n", __func__, charger->fw_cmd);

	if (gpio_get_value(charger->pdata->wpc_en)) {
		pr_info("%s: wpc_en disabled\n", __func__);
		goto end_of_fw_work;
	}
	mfc_set_wpc_en(charger, WPC_EN_FW, true); /* keep the wpc_en low during fw update */

	switch (charger->fw_cmd) {
	case SEC_WIRELESS_FW_UPDATE_SPU_MODE:
	case SEC_WIRELESS_FW_UPDATE_SPU_VERIFY_MODE:
	case SEC_WIRELESS_FW_UPDATE_SDCARD_MODE:
	case SEC_WIRELESS_FW_UPDATE_AUTO_MODE:
	case SEC_WIRELESS_FW_UPDATE_BUILTIN_MODE:
		mfc_uno_on(charger, true);
		msleep(200);
		if (!mfc_check_chip_id(charger)) {
			pr_info("%s: current IC's chip_id is not matching to driver's chip_id(0x%x)\n",
				__func__, MFC_CHIP_CPS);
			break;
		}
		bin_fw_ver = mfc_get_bin_fw_version(&charger->client->dev);
		if (charger->fw_cmd == SEC_WIRELESS_FW_UPDATE_AUTO_MODE) {
			charger->otp_firmware_ver = mfc_get_firmware_version(charger, MFC_RX_FIRMWARE);
			pr_info("%s: current version (0x%04X) ------ BIN_VERSION (0x%04X)\n",
					__func__, charger->otp_firmware_ver, bin_fw_ver);
#if defined(CONFIG_WIRELESS_IC_PARAM)
			if (charger->wireless_fw_mode_param == SEC_WIRELESS_FW_UPDATE_SPU_MODE &&
				charger->otp_firmware_ver > bin_fw_ver) {
				pr_info("%s: current version (0x%04X) is higher than BIN_VERSION by SPU(0x%04X)\n",
					__func__, charger->otp_firmware_ver, bin_fw_ver);
				break;
			}
#endif
			if (charger->otp_firmware_ver == bin_fw_ver) {
				pr_info("%s: current version (0x%04X) is same to BIN_VERSION (0x%04X)\n",
					__func__, charger->otp_firmware_ver, bin_fw_ver);
				break;
			}
		}
		charger->pdata->otp_firmware_result = MFC_FWUP_ERR_RUNNING;
		is_changed = true;

		dev_err(&charger->client->dev, "%s, request_firmware\n", __func__);

		if (charger->fw_cmd == SEC_WIRELESS_FW_UPDATE_SPU_MODE ||
			charger->fw_cmd == SEC_WIRELESS_FW_UPDATE_SPU_VERIFY_MODE)
			fw_path = MFC_FW_SPU_BIN_PATH;
		else if (charger->fw_cmd == SEC_WIRELESS_FW_UPDATE_SDCARD_MODE)
			fw_path = MFC_FW_SDCARD_BIN_PATH;
		else
			fw_path = MFC_FLASH_FW_HEX_CPS_PATH;

		ret = request_firmware(&firm_data_bin, fw_path, &charger->client->dev);
		if (ret < 0) {
			dev_err(&charger->client->dev, "%s: failed to request firmware %s (%d)\n",
				__func__, fw_path, ret);
			charger->pdata->otp_firmware_result = MFC_FWUP_ERR_REQUEST_FW_BIN;
			goto fw_err;
		}
		fw_size = firm_data_bin->size;

#if IS_ENABLED(CONFIG_SPU_VERIFY)
		if (charger->fw_cmd == SEC_WIRELESS_FW_UPDATE_SPU_MODE ||
			charger->fw_cmd == SEC_WIRELESS_FW_UPDATE_SPU_VERIFY_MODE) {
			if (spu_firmware_signature_verify("MFC", firm_data_bin->data, fw_size) ==
				((int)fw_size - SPU_METADATA_SIZE(MFC))) {
				pr_err("%s: spu_firmware_signature_verify success\n", __func__);
				(int)fw_size -= SPU_METADATA_SIZE(MFC);
				if (charger->fw_cmd == SEC_WIRELESS_FW_UPDATE_SPU_VERIFY_MODE) {
					charger->pdata->otp_firmware_result = MFC_FW_RESULT_PASS;
					goto fw_err;
				}
			} else {
				pr_err("%s: spu_firmware_signature_verify failed\n", __func__);
				goto fw_err;
			}
		}
#endif

		disable_irq(charger->pdata->irq_wpc_int);
		disable_irq(charger->pdata->irq_wpc_det);
		if (charger->pdata->irq_wpc_pdrc)
			disable_irq(charger->pdata->irq_wpc_pdrc);
		if (charger->pdata->irq_wpc_pdet_b)
			disable_irq(charger->pdata->irq_wpc_pdet_b);
		pr_info("%s data size = %ld\n", __func__, (long)fw_size);

		do {
			if (repairEn == true) {
				if (charger->pdata->wpc_en >= 0)
					gpio_direction_output(charger->pdata->wpc_en, 1);
				mfc_uno_on(charger, false);
				msleep(1000);
				mfc_uno_on(charger, true);
				if (charger->pdata->wpc_en >= 0)
					gpio_direction_output(charger->pdata->wpc_en, 0);
				msleep(300);
			}
			ret = PgmOTPwRAM_CPS(charger, 0, firm_data_bin->data, 0, (int)fw_size);
			if (ret != MFC_FWUP_ERR_SUCCEEDED)
				repairEn = true;
			else
				repairEn = false;

			pgmCnt++;

			pr_info("%s %s: repairEn(%d), pgmCnt(%d), ret(%d)\n",
				MFC_FW_MSG, __func__, repairEn, pgmCnt, ret);
		} while ((ret != MFC_FWUP_ERR_SUCCEEDED) && (pgmCnt < MAX_MTP_PGM_CNT));

		release_firmware(firm_data_bin);

		for (i = 0; i < 8; i++) {
			if (mfc_reg_read(charger->client, MFC_FW_DATA_CODE_0+i, &data) > 0)
				fwdate[i] = (char)data;
		}
		for (i = 0; i < 8; i++) {
			if (mfc_reg_read(charger->client, MFC_FW_TIMER_CODE_0+i, &data) > 0)
				fwtime[i] = (char)data;
		}
		pr_info("%s: %d%d%d%d%d%d%d%d, %d%d%d%d%d%d%d%d\n", __func__,
			fwdate[0], fwdate[1], fwdate[2], fwdate[3], fwdate[4], fwdate[5], fwdate[6], fwdate[7],
			fwtime[0], fwtime[1], fwtime[2], fwtime[3], fwtime[4], fwtime[5], fwtime[6], fwtime[7]);

		charger->otp_firmware_ver = mfc_get_firmware_version(charger, MFC_RX_FIRMWARE);
		charger->pdata->wc_ic_rev = mfc_get_ic_revision(charger, MFC_IC_REVISION);

		for (i = 0; i < 8; i++) {
			if (mfc_reg_read(charger->client, MFC_FW_DATA_CODE_0+i, &data) > 0)
				fwdate[i] = (char)data;
		}
		for (i = 0; i < 8; i++) {
			if (mfc_reg_read(charger->client, MFC_FW_TIMER_CODE_0+i, &data) > 0)
				fwtime[i] = (char)data;
		}
		pr_info("%s: %d%d%d%d%d%d%d%d, %d%d%d%d%d%d%d%d\n", __func__,
			fwdate[0], fwdate[1], fwdate[2], fwdate[3], fwdate[4], fwdate[5], fwdate[6], fwdate[7],
			fwtime[0], fwtime[1], fwtime[2], fwtime[3], fwtime[4], fwtime[5], fwtime[6], fwtime[7]);

		enable_irq(charger->pdata->irq_wpc_int);
		enable_irq(charger->pdata->irq_wpc_det);
		if (charger->pdata->irq_wpc_pdrc)
			enable_irq(charger->pdata->irq_wpc_pdrc);
		if (charger->pdata->irq_wpc_pdet_b)
			enable_irq(charger->pdata->irq_wpc_pdet_b);
		break;
	default:
		break;
	}

	msleep(200);
	mfc_uno_on(charger, false);
	pr_info("%s---------------------------------------------------------------\n", __func__);

	if (is_changed) {
		if (ret == MFC_FWUP_ERR_SUCCEEDED) {
			charger->pdata->otp_firmware_result = MFC_FW_RESULT_PASS;
#if defined(CONFIG_WIRELESS_IC_PARAM)
			charger->wireless_fw_mode_param = charger->fw_cmd & 0xF;
			pr_info("%s: succeed. fw_mode(0x%01X)\n",
				__func__, charger->wireless_fw_mode_param);
			charger->wireless_param_info &= 0xFFFFFF0F;
			charger->wireless_param_info |= (charger->wireless_fw_mode_param & 0xF) << 4;
			pr_info("%s: wireless_param_info (0x%08X)\n", __func__, charger->wireless_param_info);
#endif
		} else {
			charger->pdata->otp_firmware_result = ret;
		}
	}
	value.intval = false;
	psy_do_property("battery", set, POWER_SUPPLY_EXT_PROP_MFC_FW_UPDATE, value);

	mfc_set_wpc_en(charger, WPC_EN_FW, false);
	__pm_relax(charger->wpc_update_ws);
	return;
fw_err:
	mfc_uno_on(charger, false);
	mfc_set_wpc_en(charger, WPC_EN_FW, false);
end_of_fw_work:
	value.intval = false;
	psy_do_property("battery", set, POWER_SUPPLY_EXT_PROP_MFC_FW_UPDATE, value);
	__pm_relax(charger->wpc_update_ws);
}

static void mfc_set_tx_data(struct mfc_charger_data *charger, int idx)
{
	if (idx < 0)
		idx = 0;
	else if (idx >= charger->pdata->len_wc20_list)
		idx = charger->pdata->len_wc20_list - 1;

	charger->vout_by_txid = charger->pdata->wireless20_vout_list[idx];
	charger->vrect_by_txid = charger->pdata->wireless20_vrect_list[idx];
	charger->max_power_by_txid = charger->pdata->wireless20_max_power_list[idx];
}

static int mfc_set_sgf_data(struct mfc_charger_data *charger, sgf_data *pdata)
{
	pr_info("%s: size = %d, type = %d\n", __func__, pdata->size, pdata->type);

	switch (pdata->type) {
	case WPC_TX_COM_RX_POWER:
	{
		union power_supply_propval value = {0, };
		int tx_power = *(int *)pdata->data;

		switch (tx_power) {
		case TX_RX_POWER_7_5W:
			pr_info("%s : TX Power is 7.5W\n", __func__);
			charger->current_rx_power = TX_RX_POWER_7_5W;
			mfc_set_tx_data(charger, 0);
			break;
		case TX_RX_POWER_12W:
			pr_info("%s : TX Power is 12W\n", __func__);
			charger->current_rx_power = TX_RX_POWER_12W;
			mfc_set_tx_data(charger, 1);
			break;
		case TX_RX_POWER_15W:
			pr_info("%s : TX Power is 15W\n", __func__);
			charger->current_rx_power = TX_RX_POWER_15W;
			mfc_set_tx_data(charger, 2);
			break;
		case TX_RX_POWER_17_5W:
			pr_info("%s : TX Power is 17.5W\n", __func__);
			charger->current_rx_power = TX_RX_POWER_17_5W;
			mfc_set_tx_data(charger, 3);
			break;
		case TX_RX_POWER_20W:
			pr_info("%s : TX Power is 20W\n", __func__);
			charger->current_rx_power = TX_RX_POWER_20W;
			mfc_set_tx_data(charger, 4);
			break;
		default:
			pr_info("%s : Undefined TX Power(%d)\n", __func__, tx_power);
			return -EINVAL;
		}
		charger->adt_transfer_status = WIRELESS_AUTH_PASS;
		charger->pdata->cable_type = value.intval = SEC_BATTERY_CABLE_HV_WIRELESS_20;
		pr_info("%s: change cable type to WPC HV 2.0\n", __func__);

		__pm_stay_awake(charger->wpc_afc_vout_ws);
		queue_delayed_work(charger->wqueue, &charger->wpc_afc_vout_work, msecs_to_jiffies(0));
	}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

#if defined(CONFIG_MST_V2)
static void mfc_send_mst_cmd(int cmd, struct mfc_charger_data *charger, u8 irq_src_l, u8 irq_src_h)
{
	switch (cmd) {
	case MST_MODE_ON:
		/* clear interrupt */
		mfc_reg_write(charger->client, MFC_INT_A_CLEAR_L_REG, irq_src_l);	// clear int
		mfc_reg_write(charger->client, MFC_INT_A_CLEAR_H_REG, irq_src_h);	// clear int
		mfc_set_cmd_l_reg(charger, 0x20, MFC_CMD_CLEAR_INT_MASK);	// command

#if defined(CONFIG_MST_PCR)
		pr_info("%s : MST ISET_PCR : %d\n", __func__, charger->pdata->mst_iset_pcr);
		mfc_reg_write(charger->client, MFC_ISET_PCR, charger->pdata->mst_iset_pcr);
		if (use_pcr_fix_mode) {
			mfc_reg_write(charger->client, PCR_FIX_MODE, 0x01);
		}
		/* set PCR mode2 */
		mfc_reg_write(charger->client, MFC_MST_MODE_SEL_REG, MFC_TX_MODE_MST_PCR_MODE2);
#else
		mfc_reg_write(charger->client, MFC_MST_MODE_SEL_REG, MFC_TX_MODE_MST_MODE2);	/* set MST mode2 */
#endif
		pr_info("%s 2AC Missing ! : MST on REV : %d\n", __func__, charger->pdata->wc_ic_rev);

		/* clear interrupt */
		mfc_reg_write(charger->client, MFC_INT_A_CLEAR_L_REG, irq_src_l);	// clear int
		mfc_reg_write(charger->client, MFC_INT_A_CLEAR_H_REG, irq_src_h);	// clear int
		mfc_set_cmd_l_reg(charger, 0x20, MFC_CMD_CLEAR_INT_MASK);	// command

		usleep_range(10000, 11000);
		break;
	case MST_MODE_OFF:
		pr_info("%s: set MST mode off\n", __func__);
		break;
	default:
		break;
	}
}

static int mfc_get_mst_mode(struct mfc_charger_data *charger)
{
	u8 mst_mode, reg_data;
	int ret;

	ret = mfc_reg_read(charger->client, MFC_MST_MODE_SEL_REG, &mst_mode);
	if (ret < 0) {
		pr_info("%s mst mode(0x2) i2c write failed, ret = %d\n",
			__func__, ret);
		return ret;
	}

	ret = mfc_reg_read(charger->client, MFC_SYS_OP_MODE_REG, &reg_data);
	if (ret < 0) {
		pr_info("%s mst mode change irq(0x4) read failed, ret = %d\n",
			__func__, ret);
		return ret;
	}

	pr_info("%s mst mode check: mst_mode = %d, reg_data = %d\n",
		__func__, mst_mode, reg_data);

	reg_data &= 0x0C;	/* use only [3:2]bit of sys_op_mode register for MST */

	ret = 0;
	if (reg_data == 0x4)
		ret = mst_mode;

	return ret;
}
#endif

#define ALIGN_WORK_CHK_CNT	5
#define ALIGN_WORK_DELAY	500
#define ALIGN_CHK_PERIOD	1000
#define ALIGN_WORK_CHK_PERIOD	100
#define MISALIGN_TX_OFF_TIME	10

static int mfc_get_target_vout(struct mfc_charger_data *charger)
{
	if (charger->vout_strength == 0)
		return (charger->pdata->mis_align_target_vout + charger->pdata->mis_align_offset);
	else
		return charger->pdata->mis_align_target_vout; // falling uvlo
}

static int mfc_unsafe_vout_check(struct mfc_charger_data *charger)
{
	int vout;
	int target_vout;

	if (charger->pdata->wpc_vout_ctrl_full && charger->is_full_status)
		return 0;

	vout = mfc_get_adc(charger, MFC_ADC_VOUT);
	target_vout = mfc_get_target_vout(charger);

	pr_info("%s: vout(%d) target_vout(%d)\n", __func__, vout, target_vout);
	if (vout < target_vout)
		return 1;
	return 0;
}

static bool mfc_check_wire_status(void)
{
	union power_supply_propval value = {0, };
	int wire_type = SEC_BATTERY_CABLE_NONE;

	psy_do_property("battery", get, POWER_SUPPLY_EXT_PROP_CHARGE_COUNTER_SHADOW, value);
		wire_type = value.intval;

	if (is_wired_type(wire_type) || (wire_type == SEC_BATTERY_CABLE_OTG)) {
		pr_info("%s: return misalign check, cable_type(%d)\n",
				__func__, wire_type);
		return true;
	}

	return false;
}

static void mfc_wpc_align_check_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, align_check_work.work);
	struct timespec64 current_ts = {0, };
	union power_supply_propval value = {0, };
	long checking_time = 0;
	int vout = 0, vout_avr = 0, i = 0;
	static int vout_sum, align_work_cnt;

	if (!charger->det_state)
		goto end_align_work;

	if (charger->pdata->wpc_vout_ctrl_full && charger->is_full_status)
		goto end_align_work;

	if (charger->wc_align_check_start.tv_sec == 0) {
		charger->wc_align_check_start = ktime_to_timespec64(ktime_get_boottime());
		align_work_cnt = 0;
		vout_sum = 0;
	}
	current_ts = ktime_to_timespec64(ktime_get_boottime());
	checking_time = current_ts.tv_sec - charger->wc_align_check_start.tv_sec;

	vout = mfc_get_adc(charger, MFC_ADC_VOUT);
	vout_sum += vout;
	align_work_cnt++;
	vout_avr = vout_sum / align_work_cnt;

	pr_info("%s: vout(%d), vout_avr(%d), work_cnt(%d), checking_time(%ld)\n",
		__func__, vout, vout_avr, align_work_cnt, checking_time);

	if (align_work_cnt < ALIGN_WORK_CHK_CNT) {
		queue_delayed_work(charger->wqueue,
			&charger->align_check_work, msecs_to_jiffies(ALIGN_WORK_CHK_PERIOD));

		return;
	}

	if (vout_avr >= mfc_get_target_vout(charger)) {
		value.intval = charger->vout_strength = 100;
		psy_do_property("battery",
			set, POWER_SUPPLY_EXT_PROP_WPC_FREQ_STRENGTH, value);
		psy_do_property("wireless", set, POWER_SUPPLY_PROP_CURRENT_MAX, value);
		pr_info("%s: Finish to check (Align)\n", __func__);
		goto end_align_work;
	} else if (checking_time >= MISALIGN_TX_OFF_TIME) {
		pr_info("%s: %s to check (Timer cnt :%d)\n",
			__func__, charger->mis_align_tx_try_cnt == MISALIGN_TX_TRY_CNT ? "Finish" : "Retry",
			charger->mis_align_tx_try_cnt);

		for (i = 0; i < 30; i++) {
			pr_info("%s: Send a packet to TX device to stop power sharing\n",
				__func__);
			mfc_send_command(charger, MFC_TX_UNO_OFF);
			if (mfc_get_adc(charger, MFC_ADC_VRECT) <= 0)
				break;
		}
		charger->mis_align_tx_try_cnt++;
		mfc_set_wpc_en(charger, WPC_EN_CHARGING, false);
		goto end_algin_work_by_retry;
	} else if (checking_time >= MISALIGN_TX_OFF_TIME * MISALIGN_TX_TRY_CNT) {
		pr_info("%s: Finish to check (Timer expired %d secs)\n",
			__func__, MISALIGN_TX_OFF_TIME * MISALIGN_TX_TRY_CNT);
		goto end_align_work;
	} else {
		if (mfc_check_wire_status())
			goto end_align_work;

		pr_info("%s: Continue to check until %d secs (Misalign)\n",
			__func__, MISALIGN_TX_OFF_TIME * MISALIGN_TX_TRY_CNT);
		value.intval = charger->vout_strength = 0;
		psy_do_property("battery",
			set, POWER_SUPPLY_EXT_PROP_WPC_FREQ_STRENGTH, value);

		align_work_cnt = 0;
		vout_sum = 0;
		queue_delayed_work(charger->wqueue,
			&charger->align_check_work, msecs_to_jiffies(ALIGN_CHK_PERIOD));
	}

	return;

end_align_work:
	mfc_set_wpc_en(charger, WPC_EN_CHARGING, true);
	charger->mis_align_tx_try_cnt = 1;
	charger->wc_checking_align = false;
	charger->wc_align_check_start.tv_sec = 0;
end_algin_work_by_retry:
	__pm_relax(charger->align_check_ws);
}

static void mfc_wpc_align_check(struct mfc_charger_data *charger, unsigned int work_delay)
{
	if (!charger->pdata->mis_align_guide)
		return;

	if (mfc_check_wire_status())
		return;

	if (charger->wc_checking_align) {
		pr_info("%s: return, wc_checking_align(%d)\n", __func__, charger->wc_checking_align);
		return;
	}

	if (!charger->pdata->is_charging) {
		pr_info("%s: return, is_charging(%d)\n",
			__func__, charger->pdata->is_charging);
		return;
	}

	if (charger->vout_strength >= 100) {
		if (!mfc_unsafe_vout_check(charger)) {
			pr_info("%s: return, safe vout\n", __func__);
			return;
		}
	}

	pr_info("%s: start\n", __func__);
	__pm_stay_awake(charger->align_check_ws);
	charger->wc_checking_align = true;
	queue_delayed_work(charger->wqueue, &charger->align_check_work, msecs_to_jiffies(work_delay));
}

static void mfc_start_wpc_tx_id_work(struct mfc_charger_data *charger, unsigned int delay)
{
	__pm_stay_awake(charger->wpc_tx_id_ws);
	queue_delayed_work(charger->wqueue, &charger->wpc_tx_id_work, msecs_to_jiffies(delay));
}

static bool mfc_check_to_start_afc_tx(struct mfc_charger_data *charger)
{
	int vrect_level, vout_level;

	vrect_level = mfc_get_adc(charger, MFC_ADC_VRECT);
	vout_level = mfc_get_adc(charger, MFC_ADC_VOUT);
	pr_info("%s: read vrect(%dmV), vout(%dmV)\n", __func__, vrect_level, vout_level);

	return (vrect_level < 8500 || vout_level < 8500);
}

static void mfc_set_mpp_sleep_mode(struct mfc_charger_data *charger, int enable)
{
	int is_thm_limited = charger->thermal_ctrl;

	pr_info("@MPP %s enable(%d) thm_ctrl(%d)\n", __func__, enable, is_thm_limited);

	if (enable) {
		cps4038_set_target_ilim(charger, 400);
		mfc_mpp_thermal_ctrl(charger, enable);
	} else {
		if (is_thm_limited)
			return;
		mfc_mpp_thermal_ctrl(charger, enable);
		cps4038_set_target_ilim(charger,
			charger->mpp_epp_nego_done_power * 100 / mfc_get_volt(charger->pdata->mpp_vout));
	}
}

static void mfc_start_bpp_mode(struct mfc_charger_data *charger)
{
	if (!charger->is_full_status) {
		/* send request afc_tx , request afc is mandatory */
		msleep(charger->req_afc_delay);
		mfc_send_command(charger, MFC_REQUEST_AFC_TX);
		__pm_stay_awake(charger->wpc_tx_pwr_budg_ws);
		queue_delayed_work(charger->wqueue,
			&charger->wpc_tx_pwr_budg_work, msecs_to_jiffies(1200));
	}
}

static void mfc_set_epp_mode(struct mfc_charger_data *charger, int nego_power)
{
	mfc_set_psy_wrl(charger,
		POWER_SUPPLY_EXT_PROP_WIRELESS_MAX_VOUT,
		charger->vout_by_txid);

	/* Update max power */
	charger->max_power_by_txid = nego_power * 100000;
	mfc_set_rx_power(charger, charger->max_power_by_txid);
	charger->current_rx_power = nego_power;
}

static void mfc_set_epp_nv_mode(struct mfc_charger_data *charger)
{
	mfc_set_psy_wrl(charger,
		POWER_SUPPLY_EXT_PROP_WIRELESS_MAX_VOUT,
		WIRELESS_VOUT_5_5V);

	mfc_set_rx_power(charger, charger->max_power_by_txid);
}

static bool is_wpc_auth_support(struct mfc_charger_data *charger)
{
	u8 reg_data = 0;

	if (mfc_reg_read(charger->client, MFC_TX_WPC_AUTH_SUPPORT_REG, &reg_data) >= 0) {
		if (reg_data == 0x00) {
			pr_info("@EPP %s: wpc auth not support (0x%x)\n", __func__, reg_data);
			return false;
		} else {
			pr_info("@EPP %s: wpc auth support (0x%x)\n", __func__, reg_data);
		}
	} else {
		return false;
	}

	return true;
}

static void mfc_set_epp_count(struct mfc_charger_data *charger, unsigned int count)
{
	if (delayed_work_pending(&charger->epp_count_work)) {
		__pm_relax(charger->epp_count_ws);
		cancel_delayed_work(&charger->epp_count_work);
	}

	charger->epp_count = count;
	pr_info("%s: %d\n", __func__, count);
	if (count <= 0)
		return;

	__pm_stay_awake(charger->epp_count_ws);
	queue_delayed_work(charger->wqueue, &charger->epp_count_work, msecs_to_jiffies(4000));
}

static void mfc_epp_count_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, epp_count_work.work);

	pr_info("%s: clear epp count from %d to 0\n", __func__, charger->epp_count);
	charger->epp_count = 0;
	__pm_relax(charger->epp_count_ws);
}

static void mfc_epp_mode_change_check_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, epp_mode_change_check_work.work);
	u8 op_mode = 0;

	if (charger->adt_transfer_status == WIRELESS_AUTH_WAIT && charger->rx_op_mode == MFC_RX_MODE_WPC_EPP_NEGO) {
		mfc_reg_read(charger->client, MFC_SYS_OP_MODE_REG, &op_mode);
		op_mode = op_mode >> 5;
		pr_info("%s: rx op_mode register = 0x%x, there was no mode change int, retry mode_change_work\n", __func__, op_mode);
		__pm_stay_awake(charger->mode_change_ws);
		queue_delayed_work(charger->wqueue, &charger->mode_change_work, 0);
	}

	__pm_relax(charger->epp_mode_change_check_ws);
}

static void mfc_mpp_fake_ui_clear_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, mpp_fake_ui_clear_work.work);

	pr_info("%s: clear mpp fake ui\n", __func__);

	//should clear MPP fake ui
	charger->mpp_fake_ui_flag = false;
	__pm_relax(charger->mpp_fake_ui_clear_ws);
}

static void mfc_wpc_mode_change_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, mode_change_work.work);
	u8 op_mode = 0;

	pr_info("%s: start\n", __func__);
	if (mfc_reg_read(charger->client, MFC_SYS_OP_MODE_REG, &op_mode) <= 0)
		goto end_work;

	charger->rx_op_mode = op_mode >> 5;
	pr_info("%s: rx op_mode register = 0x%x\n", __func__, charger->rx_op_mode);

	//Enable authentication, but please note that re-nego or cloak will enter MPP or EPP again
	switch (charger->rx_op_mode) {
	case MFC_RX_MODE_AC_MISSING:
		//pr_info("%s: MFC_RX_MODE_AC_MISSING\n", __func__);
		break;
	case MFC_RX_MODE_WPC_BPP:
		//change to BPP ui
		charger->mpp_fake_ui_flag = false;
		pr_info("%s: MFC_RX_MODE_WPC_BPP\n", __func__);
		if (charger->fod_cnt > charger->pdata->fod_cnt_thresh) {
			pr_info("%s: clear fod cnt w/a\n", __func__);
			mfc_wpc_check_fod_count(charger, FOD_CNT_CLEAR);
		}
		/*TODO: enable FWC AUTH*/
		break;
	case MFC_RX_MODE_WPC_EPP:
		pr_info("@EPP %s: MFC_RX_MODE_WPC_EPP\n", __func__);
		if (delayed_work_pending(&charger->epp_mode_change_check_work)) {
			__pm_relax(charger->epp_mode_change_check_ws);
			cancel_delayed_work(&charger->epp_mode_change_check_work);
		}

		if (!is_3rd_pad((charger->mpp_epp_tx_id & 0xFFFF)))
			mfc_epp_enable(charger, 1);
		mfc_set_epp_count(charger, 0);

		if (is_samsung_pad((charger->mpp_epp_tx_id & 0xFF))) {
			if (charger->is_full_status || charger->sleep_mode) {
				cancel_delayed_work(&charger->wpc_vout_mode_work);
				__pm_stay_awake(charger->wpc_vout_mode_ws);
				queue_delayed_work(charger->wqueue, &charger->wpc_vout_mode_work, 0);
			}
			mfc_start_wpc_tx_id_work(charger, 1000);
		} else {
			cancel_delayed_work(&charger->wpc_vout_mode_work);
			__pm_stay_awake(charger->wpc_vout_mode_ws);
			queue_delayed_work(charger->wqueue, &charger->wpc_vout_mode_work, 0);
			if (charger->mpp_epp_nego_done_power < TX_RX_POWER_8W) {
				mfc_set_online(charger, SEC_BATTERY_CABLE_WIRELESS_EPP_NV);
				mfc_set_epp_nv_mode(charger);
				break;
			}
			mfc_set_online(charger, SEC_BATTERY_CABLE_WIRELESS_EPP);

#if defined(CONFIG_SEC_FACTORY)
			charger->adt_transfer_status = WIRELESS_AUTH_PASS;
#endif
			if (charger->adt_transfer_status == WIRELESS_AUTH_PASS) {
				mfc_set_epp_mode(charger, charger->mpp_epp_nego_done_power);
				break;
			}

			if (charger->adt_transfer_status != WIRELESS_AUTH_WAIT)
				break;

			if (!is_wpc_auth_support(charger)) {
				mfc_set_epp_mode(charger, TX_RX_POWER_8W);
				break;
			}

			mfc_auth_set_configs(charger, AUTH_READY);
			/* notify WPC auth service to send TX PAD a request key */
			mfc_auth_send_adt_status(charger, WIRELESS_AUTH_START);
		}
		break;
	case MFC_RX_MODE_WPC_MPP_RESTRICT:
		pr_info("@MPP %s: MFC_RX_MODE_WPC_MPP_RESTRICT\n", __func__);
		//Rx report restricted mode during 360K BPP, so need restart and enter BPP mode.
		mfc_send_ept_packet(charger, MFC_WPC_EPT_RESTART);
		break;
	case MFC_RX_MODE_WPC_MPP_FULL:
		pr_info("@MPP %s: MFC_RX_MODE_WPC_MPP_FULL\n", __func__);
#if defined(CONFIG_SEC_FACTORY)
		charger->adt_transfer_status = WIRELESS_AUTH_PASS;
#endif
		if (charger->adt_transfer_status == WIRELESS_AUTH_PASS) {
			mfc_set_epp_mode(charger, charger->mpp_epp_nego_done_power);
			break;
		}

		if (charger->adt_transfer_status != WIRELESS_AUTH_WAIT)
			break;

		if (!is_wpc_auth_support(charger)) {
			mfc_set_epp_mode(charger, TX_RX_POWER_8W);
			break;
		}

		mfc_auth_set_configs(charger, AUTH_READY);
		/* notify WPC auth service to send TX PAD a request key */
		msleep(1800);
		mfc_auth_send_adt_status(charger, WIRELESS_AUTH_START);
		break;
	case MFC_RX_MODE_WPC_MPP_CLOAK:
		pr_info("@MPP %s: MFC_RX_MODE_WPC_MPP_CLOAK\n", __func__);
		charger->mpp_cloak_status = true;
		break;
	case MFC_RX_MODE_WPC_MPP_NEGO:
		pr_info("@MPP %s: MFC_RX_MODE_WPC_MPP_NEGO\n", __func__);
		//mfc_mpp_epp_nego_power_set(charger, MFC_RX_MPP_NEGO_POWER_15W); // need to add this in dtsi
		if (charger->sleep_mode)
			mfc_set_mpp_sleep_mode(charger, charger->sleep_mode);
		break;
	case MFC_RX_MODE_WPC_EPP_NEGO:
		pr_info("@EPP %s: MFC_RX_MODE_WPC_EPP_NEGO\n", __func__);
		//mfc_mpp_epp_nego_power_set(charger, MFC_RX_MPP_NEGO_POWER_15W); // need to add this in dtsi
		break;
	}

	if (!charger->wc_tx_enable)
		goto end_work;

	op_mode = op_mode & 0xF;
	pr_info("%s: tx op_mode = 0x%x\n", __func__, op_mode);

	if (op_mode == MFC_TX_MODE_TX_PWR_HOLD) {
		if (charger->wc_rx_type == SS_GEAR) {
			/* start 3min alarm timer */
			pr_info("@Tx_Mode %s: Received PHM and start PHM disable alarm by 3min\n", __func__);
			alarm_start(&charger->phm_alarm,
					ktime_add(ktime_get_boottime(), ktime_set(180, 0)));
		} else {
			pr_info("%s: TX entered PHM but no PHM disable 3min timer\n", __func__);
		}

		mfc_set_tx_phm(charger, true);
	} else {
		mfc_test_read(charger);

		if (charger->tx_device_phm) {
			pr_info("@Tx_Mode %s: Ended PHM\n", __func__);
			mfc_set_tx_phm(charger, false);
		}
		if (charger->phm_alarm.state & ALARMTIMER_STATE_ENQUEUED) {
			pr_info("@Tx_Mode %s: escape PHM mode, cancel PHM alarm\n", __func__);
			cancel_delayed_work(&charger->wpc_tx_phm_work);
			__pm_relax(charger->wpc_tx_phm_ws);
			alarm_cancel(&charger->phm_alarm);
		}
	}

end_work:
	__pm_relax(charger->mode_change_ws);
}

static void cps4038_adt_transfer_result(struct mfc_charger_data *charger, int adt_state)
{
#if !defined(CONFIG_SEC_FACTORY)
	if ((charger->pdata->cable_type == SEC_BATTERY_CABLE_NONE) ||
		(charger->adt_transfer_status == WIRELESS_AUTH_WAIT)) {
		pr_info("%s %s: auth service sent wrong cmd(%d)\n", WC_AUTH_MSG, __func__, adt_state);
		return;
	} else if (charger->adt_transfer_status == adt_state) {
		pr_info("%s %s: skip a same PASS/FAIL result\n", WC_AUTH_MSG, __func__);
		return;
	} else if ((adt_state != WIRELESS_AUTH_PASS) && (adt_state != WIRELESS_AUTH_FAIL)) {
		pr_info("%s %s: undefined PASS/FAIL result(%d)\n", WC_AUTH_MSG, __func__, adt_state);
		charger->adt_transfer_status = adt_state;
		goto end_adt;
	}

	charger->adt_transfer_status = adt_state;
	switch (cps4038_get_auth_mode(charger)) {
	case WPC_AUTH_MODE_EPP:
		mfc_set_epp_mode(charger,
			(adt_state != WIRELESS_AUTH_PASS) ? TX_RX_POWER_8W : charger->mpp_epp_nego_done_power);
		break;
	case WPC_AUTH_MODE_MPP:
		pr_info("@MPP @EPP %s: Tx nego done power: %d\n",
			__func__, charger->mpp_epp_nego_done_power);
		mfc_set_epp_mode(charger,
			(adt_state != WIRELESS_AUTH_PASS) ? TX_RX_POWER_8W : charger->mpp_epp_nego_done_power);

		break;
	case WPC_AUTH_MODE_PPDE:
		if (adt_state == WIRELESS_AUTH_PASS) {
			mfc_fod_set_op_mode(charger->fod, WPC_OP_MODE_PPDE);
			charger->pdata->cable_type = SEC_BATTERY_CABLE_HV_WIRELESS_20;
			__pm_stay_awake(charger->wpc_afc_vout_ws);
			queue_delayed_work(charger->wqueue,
				&charger->wpc_afc_vout_work, msecs_to_jiffies(0));
			pr_info("%s %s: PASS! type = %d\n", WC_AUTH_MSG,
					__func__, charger->pdata->cable_type);
		} else {
			if (epp_mode(charger->rx_op_mode) ||
				charger->afc_tx_done) {
				charger->pdata->cable_type = SEC_BATTERY_CABLE_HV_WIRELESS;
				__pm_stay_awake(charger->wpc_afc_vout_ws);
				queue_delayed_work(charger->wqueue,
					&charger->wpc_afc_vout_work, msecs_to_jiffies(0));
			} else {
				mfc_set_online(charger, SEC_BATTERY_CABLE_WIRELESS);
			}
		}
		break;
	case WPC_AUTH_MODE_BPP:
	default:
		break;
	}

end_adt:
	mfc_auth_set_configs(charger, AUTH_COMPLETE);
#endif
}

static const char *mfc_bd_log(struct mfc_charger_data *charger, int wrl_mode)
{
	memset(charger->d_buf, 0, MFC_BAT_DUMP_SIZE);

	if (wrl_mode == SB_WRL_TX_MODE) {
		snprintf(charger->d_buf, MFC_BAT_DUMP_SIZE, "%d,%d,%d,%d,%d,%d,%s,%x,0x%x,",
			charger->mfc_adc_tx_vout,
			charger->mfc_adc_tx_iout,
			charger->mfc_adc_ping_frq,
			charger->mfc_adc_tx_min_op_frq,
			charger->mfc_adc_tx_max_op_frq,
			charger->tx_device_phm,
			sb_rx_type_str(charger->wc_rx_type),
			charger->otp_firmware_ver,
			charger->pdata->wc_ic_rev);
	} else if (wrl_mode == SB_WRL_RX_MODE) {
		snprintf(charger->d_buf, MFC_BAT_DUMP_SIZE, "%d,%d,%d,%d,0x%x,%d,%x,0x%x,0x%x,%016llX,%016llX,",
			charger->mfc_adc_vout,
			charger->mfc_adc_vrect,
			charger->mfc_adc_rx_iout,
			charger->mfc_adc_op_frq,
			charger->tx_id,
			charger->die_temp,
			charger->otp_firmware_ver,
			charger->pdata->wc_ic_rev,
			charger->rx_op_mode,
			charger->now_cmfet_state.value,
			charger->now_fod_state.value);
	}

	return charger->d_buf;
}

static void mfc_check_phm_status(struct mfc_charger_data *charger)
{
	union power_supply_propval value = { 0, };
	int is_wireless = false;

	psy_do_property("wireless", get, POWER_SUPPLY_PROP_ONLINE, value);
	is_wireless = value.intval;

	if (!is_wireless) {
		charger->rx_phm_check_cnt = 0;
		return;
	}
	pr_info("%s: is_wireless=%d cnt=%d\n", __func__, is_wireless, charger->rx_phm_check_cnt);

	if (charger->rx_phm_check_cnt++ > 5) {
		pr_info("%s wc deactivated during phm:rx_phm_status(%d)\n",
			__func__, charger->rx_phm_status);

		if (charger->rx_phm_status) {
			charger->rx_phm_state = END_PHM;
			__pm_stay_awake(charger->wpc_rx_phm_ws);
			queue_delayed_work(charger->wqueue, &charger->wpc_rx_phm_work, 0);
		} else
			mfc_deactivate_work_content(charger);
	}
}

static void cps4038_monitor_work(struct mfc_charger_data *charger)
{
	union power_supply_propval value = { 0, };
	struct sec_vote *chgen_vote = NULL;
	int pdet_b = 1, wpc_det = 0;
	int thermal_zone = BAT_THERMAL_NORMAL, capacity = 50, chgen = SEC_BAT_CHG_MODE_CHARGING;
	int ret = 0, temperature = 0;

	if (gpio_get_value(charger->pdata->wpc_en))
		pr_info("@DIS_MFC %s: charger->wpc_en_flag(0x%x)\n", __func__, charger->wpc_en_flag);

	if (charger->pdata->wpc_pdet_b >= 0)
		pdet_b = gpio_get_value(charger->pdata->wpc_pdet_b);

	if (charger->pdata->wpc_det >= 0)
		wpc_det = gpio_get_value(charger->pdata->wpc_det);

	if (!wpc_det) {
		if (!pdet_b) {
			pr_info("%s: now phm!\n", __func__);
			charger->rx_phm_check_cnt = 0;
		} else
			mfc_check_phm_status(charger);
		return;
	}
	charger->rx_phm_check_cnt = 0;

	ret = psy_do_property("battery", get,
		POWER_SUPPLY_EXT_PROP_THERMAL_ZONE, value);
	if (!ret)
		thermal_zone = value.intval;

	ret = psy_do_property("battery", get,
		POWER_SUPPLY_PROP_CAPACITY, value);
	if (!ret)
		capacity = value.intval;

	ret = psy_do_property("battery", get,
		POWER_SUPPLY_PROP_TEMP, value);
	if (!ret)
		temperature = value.intval;

	chgen_vote = find_vote("CHGEN");
	if (chgen_vote) {
		ret = get_sec_voter_status(chgen_vote, VOTER_SWELLING, &chgen);
		if (ret < 0)
			chgen = SEC_BAT_CHG_MODE_CHARGING;
	}

	switch (thermal_zone) {
	case BAT_THERMAL_OVERHEATLIMIT:
	case BAT_THERMAL_OVERHEAT:
	case BAT_THERMAL_WARM:
		if (cps4038_is_mpp_op_mode(charger)) {
			if (charger->rx_op_mode == MFC_RX_MODE_WPC_MPP_CLOAK)
				mfc_mpp_exit_cloak_cmd(charger);
			mfc_epp_enable(charger, 0);
			if (delayed_work_pending(&charger->epp_clear_timer_work)) {
				__pm_relax(charger->epp_clear_ws);
				cancel_delayed_work(&charger->epp_clear_timer_work);
			}
			charger->high_swell = true;
			mfc_send_ept_packet(charger, MFC_WPC_EPT_RESTART);
		}
		mfc_cmfet_set_high_swell(charger->cmfet, true);
		mfc_fod_set_high_swell(charger->fod, true);
		if ((chgen == SEC_BAT_CHG_MODE_CHARGING_OFF) ||
			(chgen == SEC_BAT_CHG_MODE_BUCK_OFF)) {
			mfc_cmfet_set_chg_done(charger->cmfet, true);
			mfc_fod_set_bat_state(charger->fod, MFC_FOD_BAT_STATE_FULL);
		} else if (chgen == SEC_BAT_CHG_MODE_CHARGING) {
			mfc_cmfet_set_chg_done(charger->cmfet, false);
		}
		break;
	default:
		if (charger->high_swell && charger->pdata->high_swell_recov >= temperature) {
			mfc_soft_reset_ask_stop(charger);
			charger->high_swell = false;
			mfc_epp_enable(charger, 1);
		}
		mfc_cmfet_set_high_swell(charger->cmfet, false);
		mfc_fod_set_high_swell(charger->fod, false);
		if (chgen == SEC_BAT_CHG_MODE_CHARGING)
			mfc_cmfet_set_chg_done(charger->cmfet, false);
		break;
	}
	mfc_cmfet_set_bat_cap(charger->cmfet, capacity);
	mfc_fod_set_bat_cap(charger->fod, capacity);

	pr_info("%s: check thermal_zone = %d, capacity = %d, chgen = %d temp=%d swell(%d)\n",
		__func__, thermal_zone, capacity, chgen, temperature, charger->high_swell);
}

static int is_param_bin_fw_ver_equal(unsigned int param_fw_ver, unsigned int bin_fw_ver)
{
#if defined(CONFIG_WIRELESS_IC_PARAM)
	pr_info("%s: param_fw_ver(0x%04X), bin_fw_ver(0x%04X)\n",
		__func__, param_fw_ver, bin_fw_ver);

	return (param_fw_ver == bin_fw_ver) ? 1 : 0;
#else
	return 0;
#endif
}

static int cps4038_chg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct mfc_charger_data *charger = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;
//	union power_supply_propval value;

	switch ((int)psp) {
	case POWER_SUPPLY_PROP_STATUS:
		pr_info("%s: charger->pdata->cs100_status %d\n", __func__, charger->pdata->cs100_status);
		val->intval = charger->pdata->cs100_status;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	case POWER_SUPPLY_PROP_HEALTH:
		return -ENODATA;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		if (charger->pdata->is_charging)
			val->intval = mfc_get_vout(charger);
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		return -ENODATA;
	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("%s: cable_type =%d\n", __func__, charger->pdata->cable_type);
		val->intval = charger->pdata->cable_type;
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		pr_info("%s: POWER_SUPPLY_PROP_MANUFACTURER, intval(0x%x), called by(%ps)\n",
				__func__, val->intval, __builtin_return_address(0));
		if (val->intval == SEC_WIRELESS_OTP_FIRM_RESULT) {
			pr_info("%s: otp firmware result = %d\n", __func__, charger->pdata->otp_firmware_result);
			val->intval = charger->pdata->otp_firmware_result;
		} else if (val->intval == SEC_WIRELESS_IC_REVISION) {
			pr_info("%s: check ic revision\n", __func__);
			val->intval = mfc_get_ic_revision(charger, MFC_IC_REVISION);
		} else if (val->intval == SEC_WIRELESS_IC_CHIP_ID) {
			pr_info("%s: check ic chip_id(0x%02X)\n", __func__, charger->chip_id);
			val->intval = charger->chip_id;
		} else if (val->intval == SEC_WIRELESS_OTP_FIRM_VER_BIN) {
			/* update latest kernl f/w version */
			val->intval = mfc_get_bin_fw_version(&charger->client->dev);
		} else if (val->intval == SEC_WIRELESS_OTP_FIRM_VER) {
			val->intval = mfc_get_firmware_version(charger, MFC_RX_FIRMWARE);
			pr_info("%s: check f/w revision (0x%x)\n", __func__, val->intval);
			if (val->intval < 0 && charger->otp_firmware_ver > 0)
				val->intval = charger->otp_firmware_ver;
		} else if (val->intval == SEC_WIRELESS_OTP_FIRM_VERIFY) {
			pr_info("%s: CPS FIRM_VERIFY is not implemented\n", __func__);
			val->intval = 1;
		} else {
			val->intval = -ENODATA;
			pr_err("%s: wrong mode\n", __func__);
		}
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW: /* vout */
		if (charger->pdata->is_charging) {
			val->intval = mfc_get_adc(charger, MFC_ADC_VOUT);
			pr_info("%s: wc vout (%d)\n", __func__, val->intval);
		} else {
			val->intval = 0;
		}
		break;
	case POWER_SUPPLY_PROP_ENERGY_AVG: /* vrect */
		if (charger->pdata->is_charging)
			val->intval = mfc_get_adc(charger, MFC_ADC_VRECT);
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		val->intval = charger->vrect_by_txid;
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		val->intval = mfc_get_adc(charger, val->intval);
		break;
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		break;
	case POWER_SUPPLY_PROP_CHARGE_EMPTY:
		val->intval = charger->wc_ldo_status;
		break;
	case POWER_SUPPLY_EXT_PROP_MIN ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_WIRELESS_OP_FREQ:
			val->intval = mfc_get_adc(charger, MFC_ADC_OP_FRQ);
			pr_info("%s: Operating FQ %dkHz\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_OP_FREQ_STRENGTH:
			val->intval = charger->vout_strength;
			pr_info("%s: vout strength = (%d)\n",
				__func__, charger->vout_strength);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TRX_CMD:
			val->intval = charger->pdata->trx_data_cmd;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TRX_VAL:
			val->intval = charger->pdata->trx_data_val;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID:
			val->intval = charger->tx_id;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID_CNT:
			val->intval = charger->tx_id_cnt;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONNECTED:
			val->intval = charger->wc_rx_connected;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_TYPE:
			val->intval = charger->wc_rx_type;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_VIN:
			val->intval = mfc_get_adc(charger, MFC_ADC_TX_VOUT);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_IIN:
			val->intval = mfc_get_adc(charger, MFC_ADC_TX_IOUT);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_POWER:
			val->intval = charger->current_rx_power;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS:
			val->intval = charger->adt_transfer_status;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_DATA:
			{
				//int i = 0;
				//u8 *p_data;

				if (charger->adt_transfer_status == WIRELESS_AUTH_RECEIVED) {
					pr_info("%s %s: MFC_ADT_RECEIVED (%d)\n",
							WC_AUTH_MSG, __func__, charger->adt_transfer_status);
					val->strval = (u8 *)ADT_buffer_rdata;
					//p_data = ADT_buffer_rdata;
					//for (i = 0; i < adt_readSize; i++)
					//	pr_info("%s: auth read data = %x", __func__, p_data[i]);
					//pr_info("\n", __func__);
				} else {
					pr_info("%s: data hasn't been received yet\n", __func__);
					return -ENODATA;
				}
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_SIZE:
			val->intval = adt_readSize;
			pr_info("%s %s: MFC_ADT_RECEIVED (%d), DATA SIZE(%d)\n",
					WC_AUTH_MSG, __func__, charger->adt_transfer_status, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_VOUT:
			val->intval = charger->vout_mode;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_INITIAL_WC_CHECK:
			val->intval = charger->initial_wc_check;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_CHECK_FW_VER:
			val->intval = is_param_bin_fw_ver_equal(
				charger->wireless_fw_ver_param, mfc_get_bin_fw_version(&charger->client->dev));
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_MST_PWR_EN:
			if (gpio_is_valid(charger->pdata->mst_pwr_en)) {
				val->intval = gpio_get_value(charger->pdata->mst_pwr_en);
			} else {
				pr_info("%s: invalid gpio(mst_pwr_en)\n", __func__);
				val->intval = 0;
			}
			break;
		case POWER_SUPPLY_EXT_PROP_PAD_VOLT_CTRL:
			val->intval = charger->is_afc_tx;
			break;
		case POWER_SUPPLY_EXT_PROP_WPC_EN:
			val->intval = gpio_get_value(charger->pdata->wpc_en);
			break;
#if defined(CONFIG_MST_V2)
		case POWER_SUPPLY_EXT_PROP_MST_MODE:
			val->intval = mfc_get_mst_mode(charger);
			break;
		case POWER_SUPPLY_EXT_PROP_MST_DELAY:
			val->intval = DELAY_FOR_MST;
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_MONITOR_WORK:
			cps4038_monitor_work(charger);
			break;
		case POWER_SUPPLY_EXT_PROP_GEAR_PHM_EVENT:
			val->intval = charger->tx_device_phm;
			break;
		case POWER_SUPPLY_EXT_PROP_RX_PHM:
			val->intval = charger->rx_phm_status;
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL:
		case POWER_SUPPLY_EXT_PROP_CHARGE_POWERED_OTG_CONTROL:
			return -ENODATA;
		case POWER_SUPPLY_EXT_PROP_INPUT_VOLTAGE_REGULATION:
			val->intval = charger->pdata->vout_status;
			break;
#if defined(CONFIG_WIRELESS_IC_PARAM)
		case POWER_SUPPLY_EXT_PROP_WIRELESS_PARAM_INFO:
			val->intval = charger->wireless_param_info;
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_WIRELESS_SGF:
		{
			sgf_data data;

			data.size = *(int *)val->strval;
			data.type = *(int *)(val->strval + 4);
			data.data = (char *)(val->strval + 8);
			mfc_set_sgf_data(charger, &data);
		}
			break;
		case POWER_SUPPLY_EXT_PROP_WPC_FREQ_STRENGTH:
			pr_info("%s: vout_strength = %d\n",
				__func__, charger->vout_strength);
			val->intval = charger->vout_strength;
			break;
		case POWER_SUPPLY_EXT_PROP_BATT_DUMP:
			val->strval = mfc_bd_log(charger, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_TX_PWR_BUDG:
			switch (charger->tx_pwr_budg) {
			case MFC_TX_PWR_BUDG_2W:
			case MFC_TX_PWR_BUDG_5W:
				val->intval = RX_POWER_5W;
				break;
			case MFC_TX_PWR_BUDG_7_5W:
				val->intval = RX_POWER_7_5W;
				break;
			case MFC_TX_PWR_BUDG_12W:
				val->intval = RX_POWER_12W;
				break;
			case MFC_TX_PWR_BUDG_15W:
				val->intval = RX_POWER_15W;
				break;
			default:
				val->intval = RX_POWER_NONE;
			}
			break;
		case POWER_SUPPLY_EXT_PROP_MPP_CLOAK:
			val->intval = charger->rx_op_mode == MFC_RX_MODE_WPC_MPP_CLOAK ? 1 : 0;
			pr_info("@MPP %s: MPP_CLOAK(%d)\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_MPP_VALUE:
		{
			u8 reg_data;

			if (!mpp_mode(charger->rx_op_mode)) {
				val->intval = -1;
				break;
			}
			mfc_reg_read(charger->client, MFC_ESTIMATED_K_L_REG, &reg_data);    //just MPP use estimated K
			val->intval = reg_data;
			mfc_reg_read(charger->client, MFC_ESTIMATED_K_H_REG, &reg_data);
			val->intval |= (reg_data << 8);
			val->intval = val->intval * 1000 / 4095;
			//actual k = mpp_estimated_k/4095
			pr_info("@MPP %s: MPP_VALUE estimated K: %d\n", __func__, val->intval);
		}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_OP_MODE:
			val->intval = charger->rx_op_mode;
			break;
		case POWER_SUPPLY_EXT_PROP_NEGO_DONE_PWR:
			val->intval = charger->mpp_epp_nego_done_power * 100000;
			break;
		default:
			return -ENODATA;
		}
		break;
	default:
		return -ENODATA;
	}
	return 0;
}

static void mfc_wpc_vout_mode_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_vout_mode_work.work);
	int vout_step = charger->pdata->vout_status;
	int vout = MFC_VOUT_10V;
	int wpc_vout_ctrl_lcd_on = 0;
	union power_supply_propval value = {0, };

	if (is_shutdn) {
		pr_err("%s: Escape by shtudown\n", __func__);
		return;
	}

	if (charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP &&
		charger->vout_mode != WIRELESS_VOUT_13V) {
		pr_info("@mpp %s: skip - vout_mode(%s), vout_status(%s)\n", __func__,
			sb_vout_ctr_mode_str(charger->vout_mode),
			sb_rx_vout_str(charger->pdata->vout_status));
		__pm_relax(charger->wpc_vout_mode_ws);
	}
	pr_info("%s: start - vout_mode(%s), vout_status(%s)\n", __func__,
		sb_vout_ctr_mode_str(charger->vout_mode),
		sb_rx_vout_str(charger->pdata->vout_status));
	switch (charger->vout_mode) {
	case WIRELESS_VOUT_4_5V:
		mfc_set_vout(charger, MFC_VOUT_4_5V);
		break;
	case WIRELESS_VOUT_5V:
		mfc_set_vout(charger, MFC_VOUT_5V);
		break;
	case WIRELESS_VOUT_5_5V:
		mfc_set_vout(charger, MFC_VOUT_5_5V);
		break;
	case WIRELESS_VOUT_9V:
		mfc_set_vout(charger, MFC_VOUT_9V);
		break;
	case WIRELESS_VOUT_10V:
		mfc_set_vout(charger, MFC_VOUT_10V);
		/* reset AICL */
		psy_do_property("wireless", set, POWER_SUPPLY_PROP_CURRENT_MAX, value);
		break;
	case WIRELESS_VOUT_11V:
		mfc_set_vout(charger, MFC_VOUT_11V);
		/* reset AICL */
		psy_do_property("wireless", set, POWER_SUPPLY_PROP_CURRENT_MAX, value);
		break;
	case WIRELESS_VOUT_12V:
		mfc_set_vout(charger, MFC_VOUT_12V);
		/* reset AICL */
		psy_do_property("wireless", set, POWER_SUPPLY_PROP_CURRENT_MAX, value);
		break;
	case WIRELESS_VOUT_12_5V:
		mfc_set_vout(charger, MFC_VOUT_12_5V);
		/* reset AICL */
		psy_do_property("wireless", set, POWER_SUPPLY_PROP_CURRENT_MAX, value);
		break;
	case WIRELESS_VOUT_13V:
		mfc_set_vout(charger, MFC_VOUT_13V);
		/* reset AICL */
		psy_do_property("wireless", set, POWER_SUPPLY_PROP_CURRENT_MAX, value);
		break;
	case WIRELESS_VOUT_5V_STEP:
		vout_step--;
		if (vout_step >= MFC_VOUT_5V) {
			mfc_set_vout(charger, vout_step);
			cancel_delayed_work(&charger->wpc_vout_mode_work);
			queue_delayed_work(charger->wqueue, &charger->wpc_vout_mode_work, msecs_to_jiffies(250));
			return;
		}
		break;
	case WIRELESS_VOUT_5_5V_STEP:
		psy_do_property("battery", get,	POWER_SUPPLY_EXT_PROP_LCD_FLICKER, value);
		wpc_vout_ctrl_lcd_on = value.intval;
		vout_step--;
		if (vout_step < MFC_VOUT_5_5V) {
			if (wpc_vout_ctrl_lcd_on && opfreq_ctrl_pad(charger->tx_id)) {
				pr_info("%s: tx id = 0x%x , set op freq\n", __func__, charger->tx_id);
				mfc_send_command(charger, MFC_SET_OP_FREQ);
				msleep(500);
			}
			break;
		}

		if (wpc_vout_ctrl_lcd_on) {
			psy_do_property("battery", get,	POWER_SUPPLY_EXT_PROP_PAD_VOLT_CTRL, value);
			if (value.intval && charger->is_afc_tx) {
				if (vout_step == charger->flicker_vout_threshold) {
					mfc_set_vout(charger, vout_step);
					cancel_delayed_work(&charger->wpc_vout_mode_work);
					queue_delayed_work(charger->wqueue,
						&charger->wpc_vout_mode_work,
						msecs_to_jiffies(charger->flicker_delay));
					return;
				} else if (vout_step < charger->flicker_vout_threshold) {
					pr_info("%s: set TX 5V because LCD ON\n", __func__);
					mfc_set_pad_hv(charger, false);
					charger->pad_ctrl_by_lcd = true;
				}
			}
		}
		mfc_set_vout(charger, vout_step);
		cancel_delayed_work(&charger->wpc_vout_mode_work);
		queue_delayed_work(charger->wqueue,
			&charger->wpc_vout_mode_work, msecs_to_jiffies(250));
		return;
	case WIRELESS_VOUT_4_5V_STEP:
		vout_step--;
		if (vout_step == MFC_VOUT_4_9V)
			vout_step = MFC_VOUT_4_5V;
		if (vout_step >= MFC_VOUT_4_5V) {
			mfc_set_vout(charger, vout_step);
			cancel_delayed_work(&charger->wpc_vout_mode_work);
			queue_delayed_work(charger->wqueue,
				&charger->wpc_vout_mode_work, msecs_to_jiffies(250));
			return;
		}
		break;
	case WIRELESS_VOUT_9V_STEP:
		vout = MFC_VOUT_9V;
		fallthrough;
	case WIRELESS_VOUT_10V_STEP:
		vout_step++;
		if (vout_step <= vout) {
			mfc_set_vout(charger, vout_step);
			cancel_delayed_work(&charger->wpc_vout_mode_work);
			queue_delayed_work(charger->wqueue,
				&charger->wpc_vout_mode_work, msecs_to_jiffies(250));
			return;
		}
		break;
	case WIRELESS_VOUT_CC_CV_VOUT:
		mfc_set_vout(charger, MFC_VOUT_5_5V);
		break;
	case WIRELESS_VOUT_OTG:
		mfc_set_vout(charger, MFC_VOUT_OTG);
		break;
	default:
		break;
	}
#if !defined(CONFIG_SEC_FACTORY)
	if (charger->pdata->vout_status <= MFC_VOUT_5_5V &&
		(charger->is_full_status || charger->sleep_mode || (charger->tx_id == TX_ID_BATT_PACK_U1200)))
		mfc_set_pad_hv(charger, false);
#endif
	pr_info("%s: finish - vout_mode(%s), vout_status(%s)\n",
		__func__, sb_vout_ctr_mode_str(charger->vout_mode), sb_rx_vout_str(charger->pdata->vout_status));
	__pm_relax(charger->wpc_vout_mode_ws);
}

static void mfc_wpc_i2c_error_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_i2c_error_work.work);

	if (charger->det_state &&
		gpio_get_value(charger->pdata->wpc_det)) {
		union power_supply_propval value;

		psy_do_property("battery", set,
			POWER_SUPPLY_EXT_PROP_WC_CONTROL, value);
	}
}

static void mfc_set_tx_fod_common(struct mfc_charger_data *charger)
{
	u8 data[2] = {0,};

	data[0] = charger->pdata->tx_fod_offset & 0xff;
	data[1] = (charger->pdata->tx_fod_offset & 0xff00) >> 8;
	pr_info("%s: tx_fod_gain(0x%x), tx_fod_offset(0x%x, 0x%x)\n",
		__func__, charger->pdata->tx_fod_gain, data[1], data[0]);

	/* Gain */
	mfc_reg_write(charger->client, MFC_TX_FOD_GAIN_REG, charger->pdata->tx_fod_gain);
	/* Offset */
	mfc_reg_write(charger->client, MFC_TX_FOD_OFFSET_L_REG, data[0]);
	mfc_reg_write(charger->client, MFC_TX_FOD_OFFSET_H_REG, data[1]);
}

static void mfc_set_tx_fod_thresh1(struct i2c_client *client, u32 fod_thresh1)
{
	u8 data[2] = {0,};

	/* Thresh1 */
	data[0] = fod_thresh1 & 0xff;
	data[1] = (fod_thresh1 & 0xff00) >> 8;

	pr_info("%s: fod_thresh1(0x%x, 0x%x)\n", __func__, data[1], data[0]);
	mfc_reg_write(client, MFC_TX_FOD_THRESH1_L_REG, data[0]);
	mfc_reg_write(client, MFC_TX_FOD_THRESH1_H_REG, data[1]);
}

static void mfc_set_tx_fod_ta_thresh(struct i2c_client *client, u32 fod_thresh)
{
	u8 data[2] = {0,};

	/* TA Thresh */
	data[0] = fod_thresh & 0xff;
	data[1] = (fod_thresh & 0xff00) >> 8;
	pr_info("%s: fod_thresh1(0x%x, 0x%x)\n", __func__, data[1], data[0]);
	mfc_reg_write(client, MFC_TX_FOD_TA_THRESH_L_REG, data[0]);
	mfc_reg_write(client, MFC_TX_FOD_TA_THRESH_H_REG, data[1]);
}

static void mfc_wpc_rx_type_det_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_rx_type_det_work.work);
	u8 reg_data, prmc_id;
	union power_supply_propval value;

	if (!charger->wc_tx_enable) {
		__pm_relax(charger->wpc_rx_det_ws);
		return;
	}

	mfc_reg_read(charger->client, MFC_STARTUP_EPT_COUNTER, &reg_data);
	mfc_reg_read(charger->client, MFC_TX_RXID1_READ_REG, &prmc_id);

	pr_info("@Tx_Mode %s: prmc_id 0x%x\n", __func__, prmc_id);

	mfc_set_tx_fod_common(charger);
	if (prmc_id == 0x42 && reg_data >= 1) {
		pr_info("@Tx_Mode %s: Samsung Gear Connected\n", __func__);
		charger->wc_rx_type = SS_GEAR;
		mfc_set_tx_digital_ping_freq(charger, charger->pdata->gear_op_freq);
		if (charger->pdata->gear_min_op_freq_delay > 0) {
			mfc_set_tx_min_op_freq(charger, charger->pdata->gear_min_op_freq);
			cancel_delayed_work(&charger->wpc_tx_min_op_freq_work);
			__pm_stay_awake(charger->wpc_tx_min_opfq_ws);
			queue_delayed_work(charger->wqueue, &charger->wpc_tx_min_op_freq_work,
				msecs_to_jiffies(charger->pdata->gear_min_op_freq_delay));
		}
	} else if (prmc_id == 0x42) {
		pr_info("@Tx_Mode %s: Samsung Phone Connected\n", __func__);
		charger->wc_rx_type = SS_PHONE;
		mfc_set_coil_sw_en(charger, 0);
		mfc_set_tx_fod_thresh1(charger->client, charger->pdata->phone_fod_thresh1);
		mfc_set_tx_fod_ta_thresh(charger->client, charger->pdata->phone_fod_ta_thresh);
	} else {
		pr_info("@Tx_Mode %s: Unknown device connected\n", __func__);
		charger->wc_rx_type = OTHER_DEV;
	}
	value.intval = charger->wc_rx_type;
	psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_RX_TYPE, value);

	__pm_relax(charger->wpc_rx_det_ws);
}

static void mfc_tx_min_op_freq_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_tx_min_op_freq_work.work);

	mfc_set_tx_min_op_freq(charger, TX_MIN_OP_FREQ_DEFAULT);

	__pm_relax(charger->wpc_tx_min_opfq_ws);
}

static void mfc_tx_duty_min_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_tx_duty_min_work.work);

	charger->duty_min = MIN_DUTY_SETTING_20_DATA;
	/* recover min duty */
	mfc_set_min_duty(charger, MIN_DUTY_SETTING_20_DATA);
	pr_info("%s: tx op freq = %dKhz\n", __func__, mfc_get_adc(charger, MFC_ADC_TX_MAX_OP_FRQ));

	__pm_relax(charger->wpc_tx_duty_min_ws);
}

static void mfc_cs100_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_cs100_work.work);

	charger->pdata->cs100_status = mfc_send_cs100(charger);
	__pm_relax(charger->wpc_cs100_ws);
}

static void mfc_rx_power_trans_fail_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_rx_power_trans_fail_work.work);

	pr_info("%s %d\n", __func__, charger->check_rx_power);
	if (charger->check_rx_power) {
		pr_info("%s: set 7.5W\n", __func__);
		mfc_reset_rx_power(charger, TX_RX_POWER_7_5W);
		charger->current_rx_power = TX_RX_POWER_7_5W;
	}
	__pm_relax(charger->wpc_rx_power_trans_fail_ws);
}

static void mfc_wpc_deactivate_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_deactivate_work.work);

	pr_info("%s\n", __func__);
	mfc_deactivate_work_content(charger);
	__pm_relax(charger->wpc_det_ws);
}

static void mfc_tx_phm_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_tx_phm_work.work);

	pr_info("@Tx_Mode %s\n", __func__);
	mfc_set_cmd_l_reg(charger, MFC_CMD_TOGGLE_PHM_MASK, MFC_CMD_TOGGLE_PHM_MASK);

	if (charger->tx_device_phm)
		mfc_set_tx_phm(charger, false);

	charger->skip_phm_work_in_sleep = false;
	__pm_relax(charger->wpc_tx_phm_ws);
}

static void mfc_clear_mpla_thr_recov_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, mpla_thr_recov_work.work);

	pr_info("@MPP %s: Clear MPLA throtting recovery\n", __func__);

	charger->is_mpla_thr_recov = false;
	__pm_relax(charger->mpla_thr_recov_ws);
}

static void mfc_set_mpp_cover_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, set_mpp_cover_work.work);

	charger->magnet_case_det = charger->mpp_case;
	mfc_fod_set_magnet_case_det(charger->fod, charger->magnet_case_det);
	pr_info("@MPP %s: MPP_COVER(%d)\n", __func__, charger->magnet_case_det);

	__pm_relax(charger->set_mpp_cover_ws);
}

static void mfc_set_mpp_cloak_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, set_mpp_cloak_work.work);
	union power_supply_propval value;

	pr_info("@MPP %s(%d)\n", __func__, charger->mpp_cloak_status);
	if (charger->mpp_cloak_status) {
		__pm_relax(charger->set_mpp_cloak_ws);
		return;
	}

	value.intval = CLOAK_TURN_OFF_PWR;
	psy_do_property("wireless", set,
		POWER_SUPPLY_EXT_PROP_MPP_CLOAK, value);
	mfc_send_ept_packet(charger, MFC_WPC_EPT_RESTART);
	__pm_relax(charger->set_mpp_cloak_ws);
}

static void mfc_wpc_check_mpp_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_check_mpp_work.work);
	u8 wpc_det = 0;
	u8 wpc_pdrc = 0;

	wpc_det = gpio_get_value(charger->pdata->wpc_det);
	wpc_pdrc = gpio_get_value(charger->pdata->wpc_pdrc);

	pr_info("%s cable_type(%d) rx_op_mode(%d) wpc_det(%d) pdrc(%d)\n",
		__func__, charger->pdata->cable_type, charger->rx_op_mode, wpc_det, wpc_pdrc);

	if (charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP_FAKE) {
		if ((!wpc_det && wpc_pdrc) || (!cps4038_is_mpp_op_mode(charger))) {
			mfc_set_online(charger, SEC_BATTERY_CABLE_NONE);
			mfc_epp_enable(charger, 1);
			__pm_relax(charger->check_mpp_ws);
		} else {
			cancel_delayed_work(&charger->wpc_check_mpp_work);
			queue_delayed_work(charger->wqueue,
					&charger->wpc_check_mpp_work, msecs_to_jiffies(2000));
		}
	} else
		__pm_relax(charger->check_mpp_ws);
}

static void mfc_wpc_init_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_init_work.work);
	union power_supply_propval value = {0, };

	pr_info("%s\n", __func__);
	if (charger->pdata->cable_type != SEC_BATTERY_CABLE_NONE) {
		mfc_set_online(charger, charger->pdata->cable_type);
		if (!charger->pdata->hall_ic_notifier) {
			pr_info("%s: Reset M0\n", __func__);
			/* reset MCU of MFC IC */
			mfc_set_cmd_l_reg(charger, MFC_CMD_MCU_RESET_MASK, MFC_CMD_MCU_RESET_MASK);
		}
	}
	if (charger->is_otg_on) {
		psy_do_property("wireless", set,
			POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL, value);
	}
}

static bool mfc_wpc_check_phm_exit_state(struct mfc_charger_data *charger)
{
	int det_state, vrect, i, ret;

	for (i = 0; i < 8; i++) {
		u8 status_l = 0;

		msleep(250);

		det_state = gpio_get_value(charger->pdata->wpc_det);
		vrect = mfc_get_adc(charger, MFC_ADC_VRECT);
		ret = mfc_reg_read(charger->client, MFC_STATUS_L_REG, &status_l);
		pr_info("%s: i(%d), det(%d), vrect(%d), status(%d, 0x%x)\n",
			__func__, i, det_state, vrect, ret, status_l);

		if (det_state)
			return true;

		if ((status_l & MFC_INTA_L_STAT_VRECT_MASK) &&
			(vrect > VALID_VRECT_LEVEL))
			return true;
	}

	return false;
}

static void mfc_wpc_phm_exit_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_phm_exit_work.work);
	int i, det_state, vrect;

	det_state = gpio_get_value(charger->pdata->wpc_det);
	vrect = mfc_get_adc(charger, MFC_ADC_VRECT);
	pr_info("%s: phm(%d), det(%d), vrect(%d)\n",
		__func__, charger->rx_phm_status, det_state, vrect);
	if (det_state)
		goto clear_phm;

	for (i = 0; i < 2; i++) {
		mfc_set_wpc_en(charger, WPC_EN_CHARGING, false);
		msleep(510);
		mfc_set_wpc_en(charger, WPC_EN_CHARGING, true);

		if (mfc_wpc_check_phm_exit_state(charger))
			goto clear_phm;
	}

	if (!mfc_wpc_check_phm_exit_state(charger)) {
		/* reset rx ic and tx pad for phm exit */
		mfc_set_wpc_en(charger, WPC_EN_CHARGING, false);
		msleep(750);
		mfc_deactivate_work_content(charger);
		msleep(750);
		mfc_set_wpc_en(charger, WPC_EN_CHARGING, true);
	}

clear_phm:
	charger->rx_phm_status = false;
	gpio_direction_output(charger->pdata->ping_nen, 1);
	mfc_set_psy_wrl(charger, POWER_SUPPLY_EXT_PROP_RX_PHM, false);
	charger->tx_id = TX_ID_UNKNOWN;
	charger->tx_id_done = false;
	charger->req_tx_id = false;
	charger->tx_id_cnt = 0;
	charger->pdata->cable_type = SEC_BATTERY_CABLE_NONE;
	if ((charger->adt_transfer_status != WIRELESS_AUTH_PASS) &&
		(charger->adt_transfer_status != WIRELESS_AUTH_FAIL))
		charger->adt_transfer_status = WIRELESS_AUTH_WAIT;

	__pm_relax(charger->wpc_phm_exit_ws);
}

static void mfc_epp_clear_timer_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, epp_clear_timer_work.work);

	pr_info("%s : bpp -> epp gpio\n", __func__);
	mfc_epp_enable(charger, 1);

	__pm_relax(charger->epp_clear_ws);
}

static void mfc_set_afc_vout_control(struct mfc_charger_data *charger, int vout_mode)
{
	if (is_shutdn || charger->pad_ctrl_by_lcd) {
		pr_info("%s: block to set high vout level(vs=%s) because shutdn(%d)\n",
			__func__, sb_rx_vout_str(charger->pdata->vout_status), is_shutdn);
		return;
	}

	if (charger->is_full_status) {
		pr_info("%s: block to set high vout level(vs=%s) because full status(%d)\n",
			__func__, sb_rx_vout_str(charger->pdata->vout_status),
			charger->is_full_status);
		return;
	}

	if (!charger->is_afc_tx) {
		pr_info("%s: need to set afc tx before vout control\n", __func__);
		mfc_set_pad_hv(charger, true);
		pr_info("%s: is_afc_tx = %d vout read = %d\n", __func__,
			charger->is_afc_tx, mfc_get_adc(charger, MFC_ADC_VOUT));
	}
	charger->vout_mode = vout_mode;
	cancel_delayed_work(&charger->wpc_vout_mode_work);
	__pm_stay_awake(charger->wpc_vout_mode_ws);
	queue_delayed_work(charger->wqueue,
		&charger->wpc_vout_mode_work, msecs_to_jiffies(250));
}

static void mfc_recover_vout(struct mfc_charger_data *charger)
{
	int ct = charger->pdata->cable_type;

	pr_info("%s: cable_type =%d\n", __func__, ct);

	if (is_hv_wireless_type(ct) && !is_pwr_nego_wireless_type(ct))
		mfc_set_vout(charger, MFC_VOUT_10V);

	if (ct == SEC_BATTERY_CABLE_WIRELESS_MPP)
		mfc_mpp_inc_int_ctrl(charger, 1);
}

#define RX_PHM_CMD_CNT 5
static void mfc_rx_phm_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_rx_phm_work.work);

	union power_supply_propval value = {0, };
	u8 pdet_b = 0, wpc_det = 0;
	u8 count = RX_PHM_CMD_CNT;

	if (charger->pdata->ping_nen < 0 || charger->pdata->wpc_pdet_b < 0 || charger->magnet_case_det) {
		pr_info("%s: magnet_case_det(%d)\n", __func__, charger->magnet_case_det);
		charger->rx_phm_state = NONE_PHM;
		__pm_relax(charger->wpc_rx_phm_ws);
		return;
	}

	pr_info("%s: tx_id_cnt(%d) tx_id(0x%x)\n", __func__, charger->tx_id_cnt, charger->tx_id);
	if (charger->rx_phm_state == ENTER_PHM && !is_phm_supported_pad(charger->tx_id)) {
		pr_info("%s: rx_phm unsupported\n", __func__);
		charger->rx_phm_state = NONE_PHM;
		__pm_relax(charger->wpc_rx_phm_ws);
		return;
	}

	switch (charger->rx_phm_state) {
	case ENTER_PHM:
		pr_info("%s: set ping_nen low, enter phm\n", __func__);
		charger->rx_phm_status = true;
		gpio_direction_output(charger->pdata->ping_nen, 0);
		value.intval = charger->rx_phm_status;
		psy_do_property("wireless", set,
			POWER_SUPPLY_EXT_PROP_RX_PHM, value);

		while (count-- > 0) {
			mfc_send_command(charger, MFC_PHM_ON);
			msleep(300);
		}
		pdet_b = gpio_get_value(charger->pdata->wpc_pdet_b);
		wpc_det = gpio_get_value(charger->pdata->wpc_det);
		pr_info("%s: check pdet_b = %d, wpc_det = %d for fail case\n", __func__, pdet_b, wpc_det);
		if (pdet_b || wpc_det) {
			pr_info("%s: set ping_nen high, phm fail case\n", __func__);
			charger->rx_phm_status = false;
			gpio_direction_output(charger->pdata->ping_nen, 1);
			value.intval = charger->rx_phm_status;
			psy_do_property("wireless", set,
				POWER_SUPPLY_EXT_PROP_RX_PHM, value);
			msleep(50);
		} else {
			value.intval = 1;
			psy_do_property("battery", set,
				POWER_SUPPLY_EXT_PROP_RX_PHM, value);
		}
		break;
	case EXIT_PHM:
		if (charger->rx_phm_status) {
			pr_info("%s: set ping_nen high, exit phm\n", __func__);
			__pm_stay_awake(charger->wpc_phm_exit_ws);
			queue_delayed_work(charger->wqueue, &charger->wpc_phm_exit_work, 0);
		} else
			pr_info("%s: skip exit phm\n", __func__);
		break;
	case END_PHM:
		if (charger->rx_phm_status) {
			pr_info("%s: set ping_nen high, end phm with detach\n", __func__);
			charger->rx_phm_status = false;
			gpio_direction_output(charger->pdata->ping_nen, 1);
			value.intval = charger->rx_phm_status;
			psy_do_property("wireless", set,
				POWER_SUPPLY_EXT_PROP_RX_PHM, value);
			mfc_deactivate_work_content(charger);
		} else
			pr_info("%s: skip end phm\n", __func__);
		break;
	case FAILED_PHM:
		break;
	default:
		break;
	}
	__pm_relax(charger->wpc_rx_phm_ws);
}

static void print_fod_log(struct mfc_charger_data *charger, union mfc_fod_state *state)
{
	u8 fod_data[MFC_NUM_FOD_REG];
	u16 fod_reg;
	char str[512] = { 0, };
	int i, ret = 0, str_size = 512;

	switch (state->fake_op_mode) {
	case WPC_OP_MODE_PPDE:
		fod_reg = MFC_WPC_FWC_FOD_0A_REG;
		break;
	case WPC_OP_MODE_EPP:
		fod_reg = MFC_WPC_EPP_FOD_0A_REG;
		break;
	case WPC_OP_MODE_MPP:
	case WPC_OP_MODE_BPP:
	default:
		fod_reg = MFC_WPC_FOD_0A_REG;
		break;
	}

	snprintf(str, str_size, "[0x%llX][%s]", state->value, sb_wrl_op_mode_str(state->fake_op_mode));
	str_size = sizeof(str) - strlen(str);

	for (i = 0; i < MFC_NUM_FOD_REG; i++) {
		ret = mfc_reg_read(charger->client, fod_reg + i, &fod_data[i]);
		if (ret < 0) {
			pr_info("%s: %s failed to read fod reg ret(%d)\n", __func__, str, ret);
			return;
		}

		snprintf(str + strlen(str), str_size, " 0x%02X:0x%02X", fod_reg + i, fod_data[i]);
		str_size = sizeof(str) - strlen(str);
	}

	pr_info("%s: %s\n", __func__, str);
}

static int cps4038_set_fod(struct device *dev, union mfc_fod_state *state, fod_data_t *data)
{
	struct mfc_charger_data *charger = dev_get_drvdata(dev);
	int i, ret = 0;

	if (charger->pdata->cable_type == SEC_BATTERY_CABLE_NONE)
		return 0;
	if (data == NULL)
		return 0;

	for (i = 0; i < MFC_NUM_FOD_REG; i++) {
		ret = mfc_reg_write(charger->client, MFC_WPC_FWC_FOD_0A_REG + i, data[i]);
		if (ret < 0)
			goto err_write_reg;

		ret = mfc_reg_write(charger->client, MFC_WPC_EPP_FOD_0A_REG + i, data[i]);
		if (ret < 0)
			goto err_write_reg;

		ret = mfc_reg_write(charger->client, MFC_WPC_FOD_0A_REG + i, data[i]);
		if (ret < 0)
			goto err_write_reg;
	}

	print_fod_log(charger, state);
	return 0;

err_write_reg:
	pr_err("%s: failed to write fod reg(ret = %d)\n", __func__, ret);
	return ret;
}

static void cps4038_print_fod(struct mfc_charger_data *charger)
{
	union mfc_fod_state now_state = { 0, };

	mfc_fod_get_state(charger->fod, &now_state);
	charger->now_fod_state.value = now_state.value;

	print_fod_log(charger, &now_state);
}

static int cps4038_set_cmfet(struct device *dev, union mfc_cmfet_state *state, bool cma, bool cmb)
{
	struct mfc_charger_data *charger = dev_get_drvdata(dev);
	int ret = 0;
	u8 data;

	if (!charger->det_state) {
		pr_info("%s: wireless is disconnected, state(%lld)\n", __func__, state->value);
		return 0;
	}

	if (cps4038_get_op_mode(charger) == WPC_OP_MODE_MPP ||
		charger->rx_op_mode == MFC_RX_MODE_WPC_EPP_NEGO) {
		pr_info("%s: do not ctrl cma/cmb when mpp. rx_op_mode(%d)\n", __func__, charger->rx_op_mode);
		return 0;
	}

	data = ((cma) ? 0xC0 : 0x00) | ((cmb) ? 0x30 : 0x00);
	ret = mfc_reg_write(charger->client, MFC_CMFET_CTRL_REG, data);
	mfc_reg_read(charger->client, MFC_CMFET_CTRL_REG, &data);
	pr_info("%s: state(0x%llX), ret(%d), data(0x%X)\n", __func__, state->value, ret, data);

	return ret;
}

static void cps4038_print_cmfet(struct mfc_charger_data *charger)
{
	union mfc_cmfet_state state = { 0, };
	int ret = 0;
	u8 data = 0;

	mfc_cmfet_get_state(charger->cmfet, &state);
	charger->now_cmfet_state.value = state.value;

	ret = mfc_reg_read(charger->client, MFC_CMFET_CTRL_REG, &data);
	pr_info("%s: [0x%llX] ret = %d, data = 0x%x\n", __func__, state.value, ret, data);
}

static bool cps4038_set_force_vout(struct mfc_charger_data *charger, int vout)
{
	bool ret = true;

	switch (vout) {
	case WIRELESS_VOUT_FORCE_13V:
		mfc_set_force_vout(charger, MFC_VOUT_13V);
		break;
	case WIRELESS_VOUT_FORCE_10V:
		mfc_set_force_vout(charger, MFC_VOUT_10V);
		break;
	case WIRELESS_VOUT_FORCE_9V:
		mfc_set_force_vout(charger, MFC_VOUT_9V);
		break;
	case WIRELESS_VOUT_FORCE_5V:
		mfc_set_force_vout(charger, MFC_VOUT_5V);
		break;
	case WIRELESS_VOUT_FORCE_4_7V:
		mfc_set_force_vout(charger, MFC_VOUT_4_7V);
		break;
	case WIRELESS_VOUT_FORCE_4_8V:
		mfc_set_force_vout(charger, MFC_VOUT_4_8V);
		break;
	case WIRELESS_VOUT_FORCE_4_9V:
		mfc_set_force_vout(charger, MFC_VOUT_4_9V);
		break;
	default:
		ret = false;
		break;
	}

	return ret;
}

#if defined(CONFIG_UPDATE_BATTERY_DATA)
static int mfc_chg_parse_dt(struct device *dev, mfc_charger_platform_data_t *pdata);
#endif
static int cps4038_chg_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct mfc_charger_data *charger = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;
	int i = 0;
	u8 tmp = 0;

	switch ((int)psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (val->intval == POWER_SUPPLY_STATUS_FULL) {
			pr_info("%s: full status\n", __func__);
			charger->is_full_status = 1;
			if (!is_wireless_fake_type(charger->pdata->cable_type)) {
				mfc_fod_set_bat_state(charger->fod, MFC_FOD_BAT_STATE_FULL);
				if (charger->tx_id == WC_PAD_P2400_P9500 || charger->tx_id == WC_PAD_P2400)
					mfc_fod_refresh(charger->fod);
				__pm_stay_awake(charger->wpc_cs100_ws);
				queue_delayed_work(charger->wqueue, &charger->wpc_cs100_work, msecs_to_jiffies(0));
			}
		} else if (val->intval == POWER_SUPPLY_STATUS_NOT_CHARGING) {
			mfc_mis_align(charger);
		} else if (val->intval == POWER_SUPPLY_STATUS_CHARGING) {
			charger->is_full_status = 0;
			mfc_set_pad_hv(charger, true);
			mfc_recover_vout_by_pad(charger);

			pr_info("%s: CC status. tx_id(0x%x)\n", __func__, charger->tx_id);
		} else if (val->intval == POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE) {
			pr_info("%s: CV status. tx_id(0x%x)\n", __func__, charger->tx_id);
		}
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		queue_delayed_work(charger->wqueue, &charger->wpc_init_work, 0);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		if (val->intval == POWER_SUPPLY_HEALTH_OVERHEAT ||
			val->intval == POWER_SUPPLY_EXT_HEALTH_OVERHEATLIMIT ||
			val->intval == POWER_SUPPLY_HEALTH_COLD)
			mfc_send_eop(charger, val->intval);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("%s: ept-internal fault\n", __func__);
		mfc_send_ept_packet(charger, MFC_WPC_EPT_INT_FAULT);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		if (val->intval) {
			charger->is_mst_on = MST_MODE_2;
			pr_info("%s: set MST mode 2\n", __func__);
		} else {
#if defined(CONFIG_MST_V2)
			// it will send MST driver a message.
			mfc_send_mst_cmd(MST_MODE_OFF, charger, 0, 0);
#endif
			pr_info("%s: set MST mode off\n", __func__);
			charger->is_mst_on = MST_MODE_0;
		}
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		charger->pdata->otp_firmware_result = val->intval;
		pr_info("%s: otp_firmware result initialize (%d)\n", __func__,
			charger->pdata->otp_firmware_result);
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		if (!charger->rx_phm_status) {
			if (charger->wc_tx_enable) {
				pr_info("@Tx_Mode %s: FW Ver(0x%x) TX_VOUT(%dmV) TX_IOUT(%dmA), PHM(%d), %s connected\n", __func__,
					charger->otp_firmware_ver,
					mfc_get_adc(charger, MFC_ADC_TX_VOUT),
					mfc_get_adc(charger, MFC_ADC_TX_IOUT),
					charger->tx_device_phm,
					sb_rx_type_str(charger->wc_rx_type));
				pr_info("@Tx_Mode %s: PING_FRQ(%dKHz) OP_FRQ(%dKHz) TX_MIN_FRQ(%dKHz) TX_MAX_FRQ(%dKHz)\n",
					__func__,
					mfc_get_adc(charger, MFC_ADC_PING_FRQ),
					mfc_get_adc(charger, MFC_ADC_OP_FRQ),
					mfc_get_adc(charger, MFC_ADC_TX_MIN_OP_FRQ),
					mfc_get_adc(charger, MFC_ADC_TX_MAX_OP_FRQ));
			} else if (charger->pdata->cable_type != SEC_BATTERY_CABLE_NONE) {
				charger->die_temp = mfc_get_adc(charger, MFC_ADC_DIE_TEMP);
				pr_info("%s: FW Ver(%x) RX_VOUT(%dmV) RX_VRECT(%dmV) RX_IOUT(%dmA)\n", __func__,
					charger->otp_firmware_ver,
					mfc_get_adc(charger, MFC_ADC_VOUT),
					mfc_get_adc(charger, MFC_ADC_VRECT),
					mfc_get_adc(charger, MFC_ADC_RX_IOUT));
				pr_info("%s: OP_FRQ(%dKHz) TX ID(0x%x) IC Rev(0x%x) Die temp(%d'C)\n", __func__,
					mfc_get_adc(charger, MFC_ADC_OP_FRQ),
					charger->tx_id,
					charger->pdata->wc_ic_rev,
					charger->die_temp);

				cps4038_print_fod(charger);
				cps4038_print_cmfet(charger);
			}
		}
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		break;
	case POWER_SUPPLY_PROP_CHARGE_EMPTY:
	{
		int vout = 0, vrect = 0;
		u8 is_vout_on = 0;
		bool error = false;

		if (mfc_reg_read(charger->client, MFC_STATUS_L_REG, &is_vout_on) < 0)
			error = true;
		is_vout_on = is_vout_on >> 7;
		vout = mfc_get_adc(charger, MFC_ADC_VOUT);
		vrect = mfc_get_adc(charger, MFC_ADC_VRECT);
		pr_info("%s: SET MFC LDO (%s), Current VOUT STAT (%d), RX_VOUT = %dmV, RX_VRECT = %dmV, error(%d)\n",
				__func__, (val->intval == MFC_LDO_ON ? "ON" : "OFF"), is_vout_on, vout, vrect, error);
		if ((val->intval == MFC_LDO_ON) && (!is_vout_on || error)) { /* LDO ON */
			pr_info("%s: MFC LDO ON toggle ------------ cable_work\n", __func__);
			for (i = 0; i < 2; i++) {
				mfc_reg_read(charger->client, MFC_STATUS_L_REG, &is_vout_on);
				is_vout_on = is_vout_on >> 7;
				if (!is_vout_on)
					mfc_set_cmd_l_reg(charger, MFC_CMD_TOGGLE_LDO_MASK, MFC_CMD_TOGGLE_LDO_MASK);
				else
					break;
				msleep(500);
				mfc_reg_read(charger->client, MFC_STATUS_L_REG, &is_vout_on);
				is_vout_on = is_vout_on >> 7;
				if (is_vout_on) {
					pr_info("%s: cnt = %d, LDO is ON -> MFC LDO STAT(%d)\n",
						__func__, i, is_vout_on);
					break;
				}
				msleep(1000);
				vout = mfc_get_adc(charger, MFC_ADC_VOUT);
				vrect = mfc_get_adc(charger, MFC_ADC_VRECT);
				pr_info("%s: cnt = %d, LDO Should ON -> MFC LDO STAT(%d), RX_VOUT = %dmV, RX_VRECT = %dmV\n",
						__func__, i, is_vout_on, vout, vrect);
			}
			charger->wc_ldo_status = MFC_LDO_ON;
			mfc_recover_vout(charger);
		} else if ((val->intval == MFC_LDO_OFF) && (is_vout_on || error)) { /* LDO OFF */
			mfc_set_vout(charger, MFC_VOUT_5V);
			pr_info("%s: MFC LDO OFF toggle ------------ cable_work\n", __func__);
			mfc_set_cmd_l_reg(charger, MFC_CMD_TOGGLE_LDO_MASK, MFC_CMD_TOGGLE_LDO_MASK);
			msleep(100);
			mfc_reg_read(charger->client, MFC_STATUS_L_REG, &is_vout_on);
			is_vout_on = is_vout_on >> 7;
			vout = mfc_get_adc(charger, MFC_ADC_VOUT);
			vrect = mfc_get_adc(charger, MFC_ADC_VRECT);
			pr_info("%s: LDO Should OFF -> MFC LDO STAT(%d), RX_VOUT = %dmV, RX_VRECT = %dmV\n",
					__func__, is_vout_on, vout, vrect);
			charger->wc_ldo_status = MFC_LDO_OFF;
		}
	}
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		charger->input_current = val->intval;
		pr_info("%s: input_current: %d\n", __func__, charger->input_current);
		cps4038_set_recent_ilim(charger, charger->input_current);
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		return -ENODATA;
	case POWER_SUPPLY_EXT_PROP_MIN ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_WC_CONTROL:
			if (val->intval == 0) {
				tmp = 0x01;
				mfc_send_packet(charger, MFC_HEADER_AFC_CONF,
					0x20, &tmp, 1);
				pr_info("%s: send command after wc control\n", __func__);
				msleep(150);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WC_EPT_UNKNOWN:
			if (val->intval == EPT_UNKNOWN)
				mfc_send_ept_packet(charger, MFC_WPC_EPT_UNKNOWN);
			else if (val->intval == EPT_RESTART)
				mfc_send_ept_packet(charger, MFC_WPC_EPT_RESTART);
			else if (val->intval == CEP_TIMEOUT)
				mfc_soft_reset_ask_stop(charger);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_SWITCH:
			/*
			 * It is a RX device , send a packet to TX device to stop power sharing.
			 * TX device will have MFC_INTA_H_TRX_DATA_RECEIVED_MASK irq
			 */
			if (charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_TX) {
				if (val->intval) {
					pr_info("%s: It is a RX device , send a packet to TX device to stop power sharing\n",
							__func__);
					mfc_send_command(charger, MFC_DISABLE_TX);
				}
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_SEND_FSK:
			/* send fsk packet for rx device aicl reset */
			if (val->intval && (charger->wc_rx_type != SS_GEAR)) {
				pr_info("@Tx_mode %s: Send FSK packet for Rx device aicl reset\n", __func__);
				mfc_send_fsk(charger, WPC_TX_COM_WPS, WPS_AICL_RESET);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ENABLE:
			/* on/off tx power */
			mfc_set_tx_power(charger, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONNECTED:
			charger->wc_rx_connected = val->intval;
			queue_delayed_work(charger->wqueue, &charger->wpc_rx_connection_work, 0);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS: /* it has only PASS and FAIL */
			cps4038_adt_transfer_result(charger, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_DATA: /* data from auth service will be sent */
			if (charger->pdata->cable_type != SEC_BATTERY_CABLE_NONE) {
				u8 *p_data;

				p_data = (u8 *)val->strval;

				for (i = 0; i < adt_readSize; i++)
					pr_info("%s %s: p_data[%d] = %x\n", WC_AUTH_MSG, __func__, i, p_data[i]);
				mfc_auth_adt_send(charger, p_data, adt_readSize);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_SIZE:
			if (charger->pdata->cable_type != SEC_BATTERY_CABLE_NONE)
				adt_readSize = val->intval;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_TYPE:
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT:
			mfc_set_tx_vout(charger, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_IOUT:
			mfc_set_tx_iout(charger, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TIMER_ON:
			pr_info("%s %s: TX receiver detecting timer enable(%d)\n", WC_AUTH_MSG, __func__, val->intval);
			if (charger->wc_tx_enable) {
				if (val->intval) {
					pr_info("%s %s: enable TX OFF timer (90sec)", WC_AUTH_MSG, __func__);
					mfc_reg_update(charger->client, MFC_INT_A_ENABLE_H_REG, (0x1 << 5), (0x1 << 5));
				} else {
					pr_info("%s %s: disable TX OFF timer (90sec)", WC_AUTH_MSG, __func__);
					mfc_reg_update(charger->client, MFC_INT_A_ENABLE_H_REG, 0x0, (0x1 << 5));
				}
			} else {
				pr_info("%s %s: Don't need to set TX 90sec timer, on TX OFF state\n",
					WC_AUTH_MSG, __func__);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_MIN_DUTY:
			if (delayed_work_pending(&charger->wpc_tx_duty_min_work)) {
				__pm_relax(charger->wpc_tx_duty_min_ws);
				cancel_delayed_work(&charger->wpc_tx_duty_min_work);
			}
			charger->duty_min = val->intval;
			mfc_set_min_duty(charger, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_CALL_EVENT:
			if (val->intval & BATT_EXT_EVENT_CALL) {
				charger->device_event |= BATT_EXT_EVENT_CALL;

#if defined(CONFIG_WIRELESS_RX_PHM_CTRL)
				charger->rx_phm_state = ENTER_PHM;
				__pm_stay_awake(charger->wpc_rx_phm_ws);
				queue_delayed_work(charger->wqueue, &charger->wpc_rx_phm_work, 0);
#else
				/* call in is after wireless connection */
				if (charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_PACK ||
					charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_HV_PACK ||
					charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_TX) {
					union power_supply_propval value2;

					pr_info("%s %s: enter PHM\n", WC_TX_MSG, __func__);
					/* notify "wireless" PHM status */
					value2.intval = 1;
					psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_CALL_EVENT, value2);
					mfc_send_command(charger, MFC_PHM_ON);
					msleep(250);
					mfc_send_command(charger, MFC_PHM_ON);
				}
#endif
			} else if (val->intval == BATT_EXT_EVENT_NONE) {
				if (charger->device_event & BATT_EXT_EVENT_CALL) {
					charger->device_event &= ~BATT_EXT_EVENT_CALL;
#if defined(CONFIG_WIRELESS_RX_PHM_CTRL)
					charger->rx_phm_state = EXIT_PHM;
					__pm_stay_awake(charger->wpc_rx_phm_ws);
					queue_delayed_work(charger->wqueue, &charger->wpc_rx_phm_work, 0);
#endif
				}
			}
			break;
		case POWER_SUPPLY_EXT_PROP_RX_PHM:
			charger->rx_phm_state = val->intval;
			__pm_stay_awake(charger->wpc_rx_phm_ws);
			if (charger->tx_id_done)
				queue_delayed_work(charger->wqueue, &charger->wpc_rx_phm_work, 0);
			else
				queue_delayed_work(charger->wqueue, &charger->wpc_rx_phm_work, msecs_to_jiffies(20000));
			break;
		case POWER_SUPPLY_EXT_PROP_SLEEP_MODE:
			charger->sleep_mode = val->intval;
			if (cps4038_get_op_mode(charger) == WPC_OP_MODE_MPP) {
				pr_info("@MPP %s: sleep_mode(%d)\n", __func__, charger->sleep_mode);
				mfc_set_mpp_sleep_mode(charger, charger->sleep_mode);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_PAD_VOLT_CTRL:
			if (delayed_work_pending(&charger->wpc_vout_mode_work)) {
				pr_info("%s: Already vout change. skip pad control\n", __func__);
				return 0;
			}

			if (!volt_ctrl_pad(charger->tx_id))
				break;

			if (val->intval && charger->is_afc_tx) {
				pr_info("%s: set TX 5V because LCD ON\n", __func__);
				mfc_set_pad_hv(charger, false);
				charger->pad_ctrl_by_lcd = true;
			} else if (!val->intval && !charger->is_afc_tx && charger->pad_ctrl_by_lcd) {
				mfc_set_pad_hv(charger, true);
				charger->pad_ctrl_by_lcd = false;
				pr_info("%s: need to set afc tx because LCD OFF\n", __func__);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_VOUT:
			for (i = 0; i < charger->pdata->len_wc20_list; i++)
				charger->pdata->wireless20_vout_list[i] = val->intval;

			pr_info("%s: vout(%d) len(%d)\n", __func__, val->intval, i - 1);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_1ST_DONE:
			/* 1st chg done ~ 2nd chg done : CMA+CMB (samsung pad only) */
			mfc_cmfet_set_full(charger->cmfet, true);
			pr_info("%s: 1st wireless charging done! CMA CMB\n", __func__);
			mfc_set_vout_ctrl_1st_full(charger);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_2ND_DONE:
			mfc_cmfet_set_chg_done(charger->cmfet, true);
			pr_info("%s: 2nd wireless charging done! CMA only\n", __func__);
			mfc_set_vout_ctrl_2nd_full(charger);
			break;
		case POWER_SUPPLY_EXT_PROP_WPC_EN:
			mfc_set_wpc_en(charger, val->strval[0], val->strval[1]);
			break;
		case POWER_SUPPLY_EXT_PROP_WPC_EN_MST:
			if (val->intval)
				mfc_set_wpc_en(charger, WPC_EN_MST, true);
			else
				mfc_set_wpc_en(charger, WPC_EN_MST, false);
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL:
			if (val->intval) {
				charger->is_otg_on = true;
				pr_info("%s: mfc otg on, rx_op_mode is %d\n", __func__, charger->rx_op_mode);
				if (mpp_mode(charger->rx_op_mode)) {
					pr_info("%s: mfc otg on, MPP mode change to BPP\n", __func__);
					mfc_send_ept_packet(charger, MFC_WPC_EPT_RESTART);
					//enable fake UI
				}
			} else {
				pr_info("%s: mfc otg off, rx_op_mode is %d before OTG on\n", __func__,
					charger->rx_op_mode);
				charger->is_otg_on = false;
				if (mpp_mode(charger->rx_op_mode) || bpp_mode(charger->rx_op_mode)) {
					mfc_soft_reset_ask_stop(charger);
					//enable fake UI
				}
			}
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGE_POWERED_OTG_CONTROL:
			{
				unsigned int work_state;

				mutex_lock(&charger->fw_lock);
				/* check delayed work state */
				work_state = work_busy(&charger->wpc_fw_update_work.work);
				pr_info("%s: check fw_work state(0x%x)\n", __func__, work_state);
				if (work_state & (WORK_BUSY_PENDING | WORK_BUSY_RUNNING)) {
					pr_info("%s: skip update_fw!!\n", __func__);
				} else {
					charger->fw_cmd = val->intval;
					__pm_stay_awake(charger->wpc_update_ws);
					queue_delayed_work(charger->wqueue, &charger->wpc_fw_update_work, 0);
					pr_info("%s: rx result = %d, tx result = %d\n", __func__,
						charger->pdata->otp_firmware_result,
						charger->pdata->tx_firmware_result);
				}
				mutex_unlock(&charger->fw_lock);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_INPUT_VOLTAGE_REGULATION:
			if (val->intval == WIRELESS_VOUT_NORMAL_VOLTAGE) {
				pr_info("%s: Wireless Vout forced set to 5V. + PAD CMD\n", __func__);
				for (i = 0; i < CMD_CNT; i++) {
					mfc_send_command(charger, MFC_AFC_CONF_5V);
					msleep(250);
				}
			} else if (val->intval == WIRELESS_VOUT_HIGH_VOLTAGE) {
				pr_info("%s: Wireless Vout forced set to 10V. + PAD CMD\n", __func__);
				for (i = 0; i < CMD_CNT; i++) {
					mfc_send_command(charger, MFC_AFC_CONF_10V);
					msleep(250);
				}
			} else if (val->intval == WIRELESS_VOUT_CC_CV_VOUT) {
				charger->vout_mode = val->intval;
				cancel_delayed_work(&charger->wpc_vout_mode_work);
				__pm_stay_awake(charger->wpc_vout_mode_ws);
				queue_delayed_work(charger->wqueue, &charger->wpc_vout_mode_work, 0);
			} else if (val->intval == WIRELESS_VOUT_5V ||
					val->intval == WIRELESS_VOUT_5V_STEP ||
					val->intval == WIRELESS_VOUT_5_5V_STEP ||
					val->intval == WIRELESS_VOUT_OTG) {
				int def_delay = 0;

				charger->vout_mode = val->intval;
				if (is_hv_wireless_type(charger->pdata->cable_type) &&
					val->intval != WIRELESS_VOUT_OTG)
					def_delay = 250;
				if (def_delay && val->intval == WIRELESS_VOUT_5_5V_STEP)
					def_delay = 2000;
				cancel_delayed_work(&charger->wpc_vout_mode_work);
				__pm_stay_awake(charger->wpc_vout_mode_ws);
				queue_delayed_work(charger->wqueue,
					&charger->wpc_vout_mode_work, msecs_to_jiffies(def_delay));
			} else if (val->intval == WIRELESS_VOUT_9V ||
				val->intval == WIRELESS_VOUT_10V ||
				val->intval == WIRELESS_VOUT_11V ||
				val->intval == WIRELESS_VOUT_12V ||
				val->intval == WIRELESS_VOUT_12_5V ||
				val->intval == WIRELESS_VOUT_9V_STEP ||
				val->intval == WIRELESS_VOUT_10V_STEP) {
				pr_info("%s: vout (%d)\n", __func__, val->intval);
				mfc_set_afc_vout_control(charger, val->intval);
			} else {
				if (!cps4038_set_force_vout(charger, val->intval))
					pr_info("%s: Unknown Command(%d)\n", __func__, val->intval);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONTROL:
			if (val->intval == WIRELESS_PAD_FAN_OFF) {
				pr_info("%s: fan off\n", __func__);
				mfc_fan_control(charger, 0);
			} else if (val->intval == WIRELESS_PAD_FAN_ON) {
				pr_info("%s: fan on\n", __func__);
				mfc_fan_control(charger, 1);
			} else if (val->intval == WIRELESS_PAD_LED_OFF) {
				pr_info("%s: led off\n", __func__);
				mfc_led_control(charger, MFC_LED_CONTROL_OFF);
			} else if (val->intval == WIRELESS_PAD_LED_ON) {
				pr_info("%s: led on\n", __func__);
				mfc_led_control(charger, MFC_LED_CONTROL_ON);
			} else if (val->intval == WIRELESS_PAD_LED_DIMMING) {
				pr_info("%s: led dimming\n", __func__);
				mfc_led_control(charger, MFC_LED_CONTROL_DIMMING);
			} else if (val->intval == WIRELESS_VRECT_ADJ_ON) {
				pr_info("%s: vrect adjust to have big headroom(default value)\n", __func__);
				mfc_set_vrect_adjust(charger, MFC_HEADROOM_1);
			} else if (val->intval == WIRELESS_VRECT_ADJ_OFF) {
				pr_info("%s: vrect adjust to have small headroom\n", __func__);
				mfc_set_vrect_adjust(charger, MFC_HEADROOM_0);
			} else if (val->intval == WIRELESS_VRECT_ADJ_ROOM_0) {
				pr_info("%s: vrect adjust to have headroom 0(0mV)\n", __func__);
				mfc_set_vrect_adjust(charger, MFC_HEADROOM_0);
			} else if (val->intval == WIRELESS_VRECT_ADJ_ROOM_1) {
				pr_info("%s: vrect adjust to have headroom 1(277mV)\n", __func__);
				mfc_set_vrect_adjust(charger, MFC_HEADROOM_1);
			} else if (val->intval == WIRELESS_VRECT_ADJ_ROOM_2) {
				pr_info("%s: vrect adjust to have headroom 2(497mV)\n", __func__);
				mfc_set_vrect_adjust(charger, MFC_HEADROOM_2);
			} else if (val->intval == WIRELESS_VRECT_ADJ_ROOM_3) {
				pr_info("%s: vrect adjust to have headroom 3(650mV)\n", __func__);
				mfc_set_vrect_adjust(charger, MFC_HEADROOM_3);
			} else if (val->intval == WIRELESS_VRECT_ADJ_ROOM_4) {
				pr_info("%s: vrect adjust to have headroom 4(30mV)\n", __func__);
				mfc_set_vrect_adjust(charger, MFC_HEADROOM_4);
			} else if (val->intval == WIRELESS_VRECT_ADJ_ROOM_5) {
				pr_info("%s: vrect adjust to have headroom 5(82mV)\n", __func__);
				mfc_set_vrect_adjust(charger, MFC_HEADROOM_5);
			} else if (val->intval == WIRELESS_CLAMP_ENABLE) {
				pr_info("%s: enable clamp1, clamp2 for WPC modulation\n", __func__);
			} else if (val->intval == WIRELESS_SLEEP_MODE_ENABLE) {
				if (is_sleep_mode_active(charger->tx_id)) {
					pr_info("%s: sleep_mode enable\n", __func__);
					msleep(500);
					pr_info("%s: led dimming\n", __func__);
					mfc_led_control(charger, MFC_LED_CONTROL_DIMMING);
					msleep(500);
					pr_info("%s: fan off\n", __func__);
					mfc_fan_control(charger, 0);
				} else {
					pr_info("%s: sleep_mode inactive\n", __func__);
				}
			} else if (val->intval == WIRELESS_SLEEP_MODE_DISABLE) {
				if (is_sleep_mode_active(charger->tx_id)) {
					pr_info("%s: sleep_mode disable\n", __func__);
					msleep(500);
					pr_info("%s: led on\n", __func__);
					mfc_led_control(charger, MFC_LED_CONTROL_ON);
					msleep(500);
					pr_info("%s: fan on\n", __func__);
					mfc_fan_control(charger, 1);
				} else {
					pr_info("%s: sleep_mode inactive\n", __func__);
				}
			} else {
				pr_info("%s: Unknown Command(%d)\n", __func__, val->intval);
			}
			break;
#if defined(CONFIG_UPDATE_BATTERY_DATA)
		case POWER_SUPPLY_EXT_PROP_POWER_DESIGN:
			mfc_chg_parse_dt(charger->dev, charger->pdata);
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_FILTER_CFG:
			charger->led_cover = val->intval;
			pr_info("%s: LED_COVER(%d)\n", __func__, charger->led_cover);
			break;
		case POWER_SUPPLY_EXT_PROP_TX_PWR_BUDG:
			break;
		case POWER_SUPPLY_EXT_PROP_MPP_COVER:
		{
			int delay = 5000;
#if defined(CONFIG_SEC_FACTORY)
			delay = 0;
#endif
			if (charger->init_cover) {
				delay = 0;
				pr_info("@MPP %s: update delay(%d) as init(%d)\n",
					__func__, delay, charger->init_cover);
				charger->init_cover = false;
				if (is_wireless_all_type(charger->pdata->cable_type))
					mfc_soft_reset_ask_stop(charger);
			}
			if (delayed_work_pending(&charger->set_mpp_cover_work)) {
				__pm_relax(charger->set_mpp_cover_ws);
				cancel_delayed_work(&charger->set_mpp_cover_work);
			}
			pr_info("@MPP %s: MPP_COVER(%d->%d) delay(%d)\n",
				__func__, charger->mpp_case, val->intval, delay);
			charger->mpp_case = val->intval;
			__pm_stay_awake(charger->set_mpp_cover_ws);
			queue_delayed_work(charger->wqueue,
				&charger->set_mpp_cover_work, msecs_to_jiffies(delay));
			//mfc_mpp_enable(charger, val->intval);
		}
			break;
		case POWER_SUPPLY_EXT_PROP_MPP_CLOAK:
			pr_info("@MPP %s: MPP_CLOAK(%d)\n", __func__, val->intval);
			if (val->intval == CLOAK_PTX_INITIATED)
				mfc_mpp_enter_cloak(charger, MFC_TRX_MPP_CLOAK_PTX_INITIATED);
			else if (val->intval == CLOAK_END_OF_CHARGE)
				mfc_mpp_enter_cloak(charger, MFC_TRX_MPP_CLOAK_END_OF_CHARGE);
			else if (val->intval == CLOAK_GENERIC)
				mfc_mpp_enter_cloak(charger, MFC_TRX_MPP_CLOAK_GENERIC);
			else if (val->intval == CLOAK_EXIT_CMD)
				mfc_mpp_exit_cloak_cmd(charger);
			else
				mfc_mpp_exit_cloak_turn_off_power(charger);
			break;
		case POWER_SUPPLY_EXT_PROP_MPP_ICL_CTRL:
			charger->thermal_ctrl = val->intval;
			pr_info("@MPP %s: MPP_THERMAL_CTRL(%d)\n", __func__, charger->thermal_ctrl);
			mfc_mpp_thermal_ctrl(charger, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_MPP_INC_INT_CTRL:
			pr_info("@MPP %s: %s INCREASE INT\n", __func__, val->intval ? "ENABLE" : "DISABLE");
#if 0
			if (val->intval == 1)
				mfc_mpp_epp_nego_power_set(charger, MFC_RX_MPP_NEGO_POWER_15W); // need delay?
#endif
			mfc_mpp_inc_int_ctrl(charger, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_MPP_PWR:
			pr_info("%s:MPP_PWR input_current: %d\n", __func__, val->intval);
			cps4038_set_target_ilim(charger, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_MPLA_THR_RECOV:
			charger->is_mpla_thr_recov = true;
			__pm_stay_awake(charger->mpla_thr_recov_ws);
			queue_delayed_work(charger->wqueue,
				&charger->mpla_thr_recov_work, msecs_to_jiffies(2000));
			break;
		default:
			return -ENODATA;
		}
		break;
	default:
		return -ENODATA;
	}

	return 0;
}

static u32 mfc_get_wireless20_vout_by_txid(struct mfc_charger_data *charger, u8 txid)
{
	u32 vout = 0, i = 0;

	if (txid < TX_ID_AUTH_PAD || txid > TX_ID_AUTH_PAD_END) {
		pr_info("%s: wrong tx id(0x%x) for wireless 2.0\n", __func__, txid);
		i = 0; /* defalut value is WIRELESS_VOUT_10V */
	} else if (txid >= TX_ID_AUTH_PAD + charger->pdata->len_wc20_list) {
		pr_info("%s: undefined tx id(0x%x) of the device\n", __func__, txid);
		i = charger->pdata->len_wc20_list - 1;
	} else
		i = txid - TX_ID_AUTH_PAD;

	vout = charger->pdata->wireless20_vout_list[i];
	pr_info("%s: vout = %d\n", __func__, vout);
	return vout;
}

static u32 mfc_get_wireless20_vrect_by_txid(struct mfc_charger_data *charger, u8 txid)
{
	u32 vrect = 0, i = 0;

	if (txid < TX_ID_AUTH_PAD || txid > TX_ID_AUTH_PAD_END) {
		pr_info("%s: wrong tx id(0x%x) for wireless 2.0\n", __func__, txid);
		i = 0; /* defalut value is MFC_AFC_CONF_10V_TX */
	} else if (txid >= TX_ID_AUTH_PAD + charger->pdata->len_wc20_list) {
		pr_info("%s: undefined tx id(0x%x) of the device\n", __func__, txid);
		i = charger->pdata->len_wc20_list - 1;
	} else {
		i = txid - TX_ID_AUTH_PAD;
	}

	vrect = charger->pdata->wireless20_vrect_list[i];
	pr_info("%s: vrect = %d\n", __func__, vrect);
	return vrect;
}

static u32 mfc_get_wireless20_max_power_by_txid(struct mfc_charger_data *charger, u8 txid)
{
	u32 max_power = 0, i = 0;

	if (txid < TX_ID_AUTH_PAD || txid > TX_ID_AUTH_PAD_END) {
		pr_info("%s: wrong tx id(0x%x) for wireless 2.0\n", __func__, txid);
		i = 0; /* defalut value is SEC_WIRELESS_RX_POWER_12W */
	} else if (txid >= TX_ID_AUTH_PAD + charger->pdata->len_wc20_list) {
		pr_info("%s: undefined tx id(0x%x) of the device\n", __func__, txid);
		i = charger->pdata->len_wc20_list - 1;
	} else {
		i = txid - TX_ID_AUTH_PAD;
	}

	max_power = charger->pdata->wireless20_max_power_list[i];
	pr_info("%s: max rx power = %d\n", __func__, max_power);
	return max_power;
}

static void mfc_recv_tx_pwr_budg(struct mfc_charger_data *charger, u8 data)
{
	switch (data) {
	case MFC_TX_PWR_BUDG_2W:
		pr_info("%s: TX POWER BUDGET 2W\n", __func__);
		charger->tx_pwr_budg = MFC_TX_PWR_BUDG_2W;
		break;
	case MFC_TX_PWR_BUDG_5W:
		pr_info("%s: TX POWER BUDGET 5W\n", __func__);
		charger->tx_pwr_budg = MFC_TX_PWR_BUDG_5W;
		break;
	case MFC_TX_PWR_BUDG_7_5W:
		pr_info("%s: TX POWER BUDGET 7.5W\n", __func__);
		charger->tx_pwr_budg = MFC_TX_PWR_BUDG_7_5W;
		break;
	case MFC_TX_PWR_BUDG_12W:
		pr_info("%s: TX POWER BUDGET 12W\n", __func__);
		charger->tx_pwr_budg = MFC_TX_PWR_BUDG_12W;
		break;
	case MFC_TX_PWR_BUDG_15W:
		pr_info("%s: TX POWER BUDGET 15W\n", __func__);
		charger->tx_pwr_budg = MFC_TX_PWR_BUDG_15W;
		break;
	default:
		pr_info("%s: NOT DEFINED TX POWER BUDGET(%d)\n", __func__, data);
		charger->tx_pwr_budg = MFC_TX_PWR_BUDG_NONE;
		break;
	}
}

static void mfc_activate_work_content(struct mfc_charger_data *charger)
{
	int cable_type;
	u8 pad_mode;
	u8 vrect;

	charger->initial_wc_check = true;
	charger->otp_firmware_ver = mfc_get_firmware_version(charger, MFC_RX_FIRMWARE);
	charger->pdata->wc_ic_rev = mfc_get_ic_revision(charger, MFC_IC_REVISION);

	charger->wc_tx_enable = false;

	/* enable Mode Change INT */
	mfc_reg_update(charger->client, MFC_INT_A_ENABLE_L_REG,
			MFC_STAT_L_OP_MODE_MASK, MFC_STAT_L_OP_MODE_MASK);
	mfc_reg_update(charger->client, MFC_INT_A_ENABLE_L_REG,
			MFC_STAT_L_OVER_TEMP_MASK, MFC_STAT_L_OVER_TEMP_MASK);

	if (!charger->pdata->default_clamp_volt) {
		/* SET OV 20V */
		mfc_reg_write(charger->client, MFC_RX_OV_CLAMP_REG, 0x4);
	}
	/* SET ILIM 1.6A */
	mfc_reg_write(charger->client, MFC_ILIM_SET_REG, 0x20);

	/* read vrect adjust */
	mfc_reg_read(charger->client, MFC_VRECT_ADJ_REG, &vrect);

	pr_info("%s: wireless charger activated, set V_INT as PN\n", __func__);

	/* CMA/CMB refresh */
	mfc_cmfet_refresh(charger->cmfet);

	/* read pad mode */
	mfc_reg_read(charger->client, MFC_SYS_OP_MODE_REG, &pad_mode);
	charger->rx_op_mode = pad_mode >> 5;
	pr_info("%s: Pad type (0x%x)\n", __func__, charger->rx_op_mode);

	cable_type = charger->pdata->cable_type;
	charger->pdata->is_charging = 1;
	if ((cable_type == SEC_BATTERY_CABLE_NONE) ||
		cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP_FAKE ||
		is_wireless_fake_type(cable_type)) {
		if (bpp_mode(charger->rx_op_mode)) {
			cable_type = SEC_BATTERY_CABLE_WIRELESS;

			if (!charger->is_otg_on) {
				charger->pdata->vout_status = MFC_VOUT_5_5V;
				mfc_set_vout(charger, charger->pdata->vout_status);
			}

			if (!is_no_hv(charger)) {
				if (mfc_check_to_start_afc_tx(charger)) {
					mfc_start_bpp_mode(charger);
				} else {
					/* reboot status with previous hv voltage setting */
					/* re-set vout level */
					charger->pad_vout = PAD_VOUT_10V;
					mfc_set_vout(charger, MFC_VOUT_10V);

					/* change cable type */
					cable_type = SEC_BATTERY_CABLE_HV_WIRELESS;
				}
			}
			mfc_start_wpc_tx_id_work(charger, 2500);
			mfc_set_online(charger, cable_type);
		} else if (mpp_mode(charger->rx_op_mode)) {
			if (cable_type != SEC_BATTERY_CABLE_WIRELESS_MPP) {
				mfc_mpp_epp_nego_done(charger);
			}
		} else if (charger->rx_op_mode == MFC_RX_MODE_WPC_EPP) {
			if ((cable_type != SEC_BATTERY_CABLE_WIRELESS_EPP) &&
				(cable_type != SEC_BATTERY_CABLE_WIRELESS_EPP_NV)) {
				mfc_mpp_epp_nego_done(charger);
				cancel_delayed_work(&charger->mode_change_work);
				__pm_stay_awake(charger->mode_change_ws);
				queue_delayed_work(charger->wqueue, &charger->mode_change_work, 0);
			}

			if (is_samsung_pad((charger->mpp_epp_tx_id & 0xFF)) ||
				is_3rd_pad((charger->mpp_epp_tx_id & 0xFFFF)))
				charger->epp_time = 6000;
		} else if (charger->rx_op_mode == MFC_RX_MODE_WPC_EPP_NEGO) {
			if (cable_type != SEC_BATTERY_CABLE_WIRELESS_EPP_FAKE)
				mfc_mpp_epp_nego_done(charger);

			if (is_samsung_pad((charger->mpp_epp_tx_id & 0xFF)) ||
				is_3rd_pad((charger->mpp_epp_tx_id & 0xFFFF)))
				charger->epp_time = 6000;
		}
		mfc_fod_set_op_mode(charger->fod, cps4038_get_op_mode(charger));
	}
	mfc_fod_set_magnet_case_det(charger->fod, charger->magnet_case_det);
	mfc_fod_refresh(charger->fod);
}

static void mfc_deactivate_work_content(struct mfc_charger_data *charger)
{
	union power_supply_propval value;

	/* Send last tx_id to battery to count tx_id */
	value.intval = charger->tx_id;
	psy_do_property("wireless", set, POWER_SUPPLY_PROP_AUTHENTIC, value);
	charger->pdata->cable_type = SEC_BATTERY_CABLE_NONE;
	charger->pdata->is_charging = 0;
	charger->pdata->vout_status = MFC_VOUT_5V;
	charger->pad_vout = PAD_VOUT_5V;
	charger->pdata->opfq_cnt = 0;
	charger->pdata->trx_data_cmd = 0;
	charger->pdata->trx_data_val = 0;
	charger->vout_mode = 0;
	charger->is_full_status = 0;
	charger->is_afc_tx = false;
	charger->pad_ctrl_by_lcd = false;
	charger->tx_id = TX_ID_UNKNOWN;
	charger->i2c_error_count = 0;
	charger->gpio_irq_missing_wa_cnt = 0;
	charger->adt_transfer_status = WIRELESS_AUTH_WAIT;
	charger->current_rx_power = TX_RX_POWER_0W;
	charger->tx_id_cnt = 0;
	charger->wc_ldo_status = MFC_LDO_ON;
	charger->tx_id_done = false;
	charger->req_tx_id = false;
	charger->req_afc_delay = REQ_AFC_DLY;
	charger->afc_tx_done = false;
	charger->wc_align_check_start.tv_sec = 0;
	charger->vout_strength = 100;
	charger->check_rx_power = false;
	charger->tx_pwr_budg = MFC_TX_PWR_BUDG_NONE;
	charger->mpp_epp_tx_id = 0;
	charger->mpp_epp_nego_done_power = 0;
	charger->mpp_epp_tx_potential_load_power = 0;
	charger->mpp_epp_tx_negotiable_load_power = 0;
	charger->mpp_cloak = 0;
	charger->rx_op_mode = 0;

	charger->vout_by_txid = 0;
	charger->vrect_by_txid = 0;
	charger->max_power_by_txid = 0;
	charger->thermal_ctrl = 0;
	//mfc_mpp_exit_cloak_turn_off_power(charger);

	mfc_set_online(charger, SEC_BATTERY_CABLE_NONE);
	mfc_fod_init_state(charger->fod);
	mfc_cmfet_init_state(charger->cmfet);
	value.intval = 0;
	psy_do_property("battery", set,
		POWER_SUPPLY_EXT_PROP_RX_PHM, value);

	/* tx retry case for mis-align */
	if (charger->wc_checking_align) {
		/* reset try cnt in real detach status */
		if (!gpio_get_value(charger->pdata->wpc_en))
			charger->mis_align_tx_try_cnt = 1;
		else {
			if (charger->mis_align_tx_try_cnt > MISALIGN_TX_TRY_CNT)
				charger->mis_align_tx_try_cnt = 1;
			msleep(1000);
			mfc_set_wpc_en(charger, WPC_EN_CHARGING, true);
		}
	} else
		charger->mis_align_tx_try_cnt = 1;

	charger->wc_checking_align = false;

	pr_info("%s: wpc deactivated, set V_INT as PD\n", __func__);

	if (delayed_work_pending(&charger->wpc_afc_vout_work)) {
		__pm_relax(charger->wpc_afc_vout_ws);
		cancel_delayed_work(&charger->wpc_afc_vout_work);
	}
	if (delayed_work_pending(&charger->wpc_vout_mode_work)) {
		__pm_relax(charger->wpc_vout_mode_ws);
		cancel_delayed_work(&charger->wpc_vout_mode_work);
	}
	if (delayed_work_pending(&charger->wpc_cs100_work)) {
		__pm_relax(charger->wpc_cs100_ws);
		cancel_delayed_work(&charger->wpc_cs100_work);
	}
	if (delayed_work_pending(&charger->wpc_rx_power_trans_fail_work)) {
		__pm_relax(charger->wpc_rx_power_trans_fail_ws);
		cancel_delayed_work(&charger->wpc_rx_power_trans_fail_work);
	}
	if (delayed_work_pending(&charger->wpc_rx_phm_work)) {
		__pm_relax(charger->wpc_rx_phm_ws);
		cancel_delayed_work(&charger->wpc_rx_phm_work);
	}
	if (delayed_work_pending(&charger->epp_mode_change_check_work)) {
		__pm_relax(charger->epp_mode_change_check_ws);
		cancel_delayed_work(&charger->epp_mode_change_check_work);
	}

	cancel_delayed_work(&charger->wpc_isr_work);
	cancel_delayed_work(&charger->wpc_tx_isr_work);
	cancel_delayed_work(&charger->wpc_tx_id_work);
	cancel_delayed_work(&charger->wpc_tx_pwr_budg_work);
	cancel_delayed_work(&charger->wpc_i2c_error_work);
	cancel_delayed_work(&charger->align_check_work);
	__pm_relax(charger->wpc_rx_ws);
	__pm_relax(charger->wpc_tx_ws);
	__pm_relax(charger->wpc_tx_id_ws);
	__pm_relax(charger->wpc_tx_pwr_budg_ws);
	__pm_relax(charger->align_check_ws);
}

static void mfc_wpc_det_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_det_work.work);

	u8 pdet_b = 0, p_nen = 0;

	pr_info("%s : start\n", __func__);

	/*
	 * We don't have to handle the wpc detect handling,
	 * when it's the MST mode.
	 */
	if (charger->is_mst_on == MST_MODE_2) {
		pr_info("%s: check det_state(%d - %d)\n", __func__,
			charger->det_state, gpio_get_value(charger->pdata->wpc_det));

		if (charger->det_state == 0) {
			pr_info("%s: skip wpc_det_work for MST operation\n", __func__);
			__pm_relax(charger->wpc_det_ws);
			return;
		}
	}

	if (charger->pdata->wpc_pdet_b >= 0)
		pdet_b = gpio_get_value(charger->pdata->wpc_pdet_b);
	if (charger->pdata->ping_nen >= 0)
		p_nen = gpio_get_value(charger->pdata->ping_nen);
	pr_info("%s: det = %d, pdet_b = %d, ping_nen = %d\n",
		__func__, gpio_get_value(charger->pdata->wpc_det), pdet_b, p_nen);

	if (charger->det_state) {
		int i = 0;

		for (i = 0; i < 3; i++) {
			if (mfc_get_chip_id(charger) >= 0)
				break;
		}
		if (i == 3) {
			__pm_relax(charger->wpc_det_ws);
			return;
		}

		if (charger->rx_op_mode == 0 && charger->magnet_case_det) {
			u8 op_mode = 0;

			if (delayed_work_pending(&charger->mode_change_work)) {
				mfc_reg_read(charger->client, MFC_SYS_OP_MODE_REG, &op_mode);
				pr_info("%s: rx_op_mode %d 0x%x\n", __func__, charger->rx_op_mode, op_mode);
				if ((op_mode >> 5) == 0) {
					mfc_soft_reset_ask_stop(charger);
					__pm_relax(charger->wpc_det_ws);
					return;
				}
			} else {
				mfc_soft_reset_ask_stop(charger);
				__pm_relax(charger->wpc_det_ws);
				return;
			}
		}
		mfc_wpc_check_fod_count(charger, FOD_CNT_CLEAR);
		mfc_get_adc(charger, MFC_ADC_VRECT);
		mfc_get_adc(charger, MFC_ADC_VOUT);
		mfc_activate_work_content(charger);
	} else {
		if (charger->rx_phm_status && !pdet_b) {
			pr_info("%s: phm status, skip deactivated\n", __func__);
			charger->tx_id_cnt = 0;
			__pm_relax(charger->wpc_det_ws);
			return;
		}
		mfc_deactivate_work_content(charger);
	}
	__pm_relax(charger->wpc_det_ws);
}

static void mfc_wpc_pdrc_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_pdrc_work.work);
	u8 data[2] = {0,};

	if (charger->is_mst_on == MST_MODE_2 || charger->wc_tx_enable) {
		pr_info("%s: Noise made false Vrect IRQ !\n", __func__);
		if (charger->wc_tx_enable) {
			union power_supply_propval value;

			value.intval = BATT_TX_EVENT_WIRELESS_TX_ETC;
			psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR, value);
		}
		__pm_relax(charger->wpc_pdrc_ws);
		return;
	}

	pr_info("%s : cable_type: %d, status: %d\n", __func__,
		charger->pdata->cable_type, charger->pdrc_state);

	if (charger->pdata->cable_type == SEC_BATTERY_CABLE_NONE && (charger->pdrc_state == 0)) {
		int i = 0;

		for (i = 0; i < 3; i++) {
			if (mfc_get_chip_id(charger) >= 0)
				break;
		}
		if (i == 3) {
			__pm_relax(charger->wpc_pdrc_ws);
			return;
		}
		mfc_wpc_check_fod_count(charger, FOD_CNT_CLEAR);
		mfc_set_online(charger, SEC_BATTERY_CABLE_WIRELESS_FAKE);
		mfc_reg_read(charger->client, MFC_RX_PING_FREQ_L_REG, &data[0]);
		mfc_reg_read(charger->client, MFC_RX_PING_FREQ_H_REG, &data[1]);
		charger->ping_freq = (data[0] | (data[1] << 8));

		//continue MPP fake ui
		if (delayed_work_pending(&charger->mpp_fake_ui_clear_work)) {
			__pm_relax(charger->mpp_fake_ui_clear_ws);
			cancel_delayed_work(&charger->mpp_fake_ui_clear_work);
		}
	} else if ((is_wireless_fake_type(charger->pdata->cable_type)) && (charger->pdrc_state == 1)) {
		union power_supply_propval value = {0, };

		charger->is_full_status = 0;
		mfc_set_online(charger, SEC_BATTERY_CABLE_NONE);
		mfc_fod_init_state(charger->fod);
		mfc_cmfet_init_state(charger->cmfet);
		value.intval = 0;
		psy_do_property("battery", set,
			POWER_SUPPLY_EXT_PROP_RX_PHM, value);
	}

	__pm_relax(charger->wpc_pdrc_ws);
}

/* INT_A */
static void mfc_wpc_tx_isr_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_tx_isr_work.work);

	u8 status_l = 0, status_h = 0;
	int ret = 0;

	pr_info("@Tx_Mode %s\n", __func__);

	if (!charger->wc_tx_enable) {
		__pm_relax(charger->wpc_tx_ws);
		return;
	}

	ret = mfc_reg_read(charger->client, MFC_STATUS_L_REG, &status_l);
	ret = mfc_reg_read(charger->client, MFC_STATUS_H_REG, &status_h);

	pr_info("@Tx_Mode %s: DATA RECEIVED! status(0x%x)\n", __func__, status_h << 8 | status_l);

	mfc_tx_handle_rx_packet(charger);

	__pm_relax(charger->wpc_tx_ws);
}

/* INT_A */
static void mfc_wpc_isr_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_isr_work.work);
	u8 cmd_data, val_data;
	int wpc_vout_ctrl_lcd_on = 0;
	union power_supply_propval value = {0, };

	if (!charger->det_state) {
		pr_info("%s: charger->det_state is 0. exit wpc_isr_work.\n", __func__);
		__pm_relax(charger->wpc_rx_ws);
		return;
	}

	pr_info("%s: cable_type (%d)\n", __func__, charger->pdata->cable_type);

	mfc_reg_read(charger->client, MFC_WPC_TRX_DATA2_COM_REG, &cmd_data);
	mfc_reg_read(charger->client, MFC_WPC_TRX_DATA2_VALUE0_REG, &val_data);
	charger->pdata->trx_data_cmd = cmd_data;
	charger->pdata->trx_data_val = val_data;

	pr_info("%s: WPC Interrupt Occurred, CMD : 0x%x, DATA : 0x%x\n", __func__, cmd_data, val_data);

	if (mpp_mode(charger->rx_op_mode)) {
		if ((cmd_data == MFC_HEADER_CLOAK) && ((val_data & 0xF0) == 0)) {
			pr_info("@MPP %s: MFC_HEADER_CLOAK - set vout 10v\n", __func__);
			mfc_set_vout(charger, MFC_VOUT_10V);
			value.intval = CLOAK_PTX_INITIATED;
			psy_do_property("wireless", set,
				POWER_SUPPLY_EXT_PROP_MPP_CLOAK, value);
		}
		pr_info("@MPP %s: op_mode is MPP(%d), exit sfc\n", __func__, charger->rx_op_mode);
		return;
	} else if (epp_mode(charger->rx_op_mode) && !is_samsung_pad((charger->mpp_epp_tx_id & 0xFF))) {
		pr_info("@EPP %s: op_mode is EPP(%d), exit sfc\n", __func__, charger->rx_op_mode);
		return;
	}

	/* None or RX mode */
	if (cmd_data == WPC_TX_COM_AFC_SET) {
		switch (val_data) {
		case TX_AFC_SET_5V:
			pr_info("%s: data = 0x%x, might be 5V irq\n", __func__, val_data);
			charger->pad_vout = PAD_VOUT_5V;
			break;
		case TX_AFC_SET_10V:
			pr_info("%s: data = 0x%x, might be 10V irq\n", __func__, val_data);
			charger->afc_tx_done = true;
			if (!gpio_get_value(charger->pdata->wpc_det)) {
				pr_err("%s: Wireless charging is paused during set high voltage.\n", __func__);
				__pm_relax(charger->wpc_rx_ws);
				return;
			}
			if (!charger->pdata->no_hv) {
				if (is_hv_wireless_type(charger->pdata->cable_type) ||
					(charger->pdata->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV)) {
					pr_err("%s: Is is already HV wireless cable. No need to set again\n", __func__);
					__pm_relax(charger->wpc_rx_ws);
					return;
				}

				/* send AFC_SET */
				mfc_send_command(charger, MFC_AFC_CONF_10V);
				msleep(500);

				/* change cable type */
				mfc_set_online(charger, SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV);

				charger->pad_vout = PAD_VOUT_10V;
				mfc_fod_set_op_mode(charger->fod, WPC_OP_MODE_PPDE);
			}
			break;
		case TX_AFC_SET_12V:
			break;
		case TX_AFC_SET_18V:
		case TX_AFC_SET_19V:
		case TX_AFC_SET_20V:
		case TX_AFC_SET_24V:
			break;
		default:
			pr_info("%s: unsupport : 0x%x", __func__, val_data);
			break;
		}
	} else if (cmd_data == WPC_TX_COM_TX_ID) {
		if (charger->req_tx_id)
			charger->req_tx_id = false;

		if (!charger->tx_id_done) {
			int capacity = 0;
			bool skip_send_online = false;

			charger->tx_id = val_data;
			mfc_fod_set_tx_id(charger->fod, val_data);

			psy_do_property("battery", get, POWER_SUPPLY_PROP_CAPACITY, value);
			capacity = value.intval;

			psy_do_property("battery", get, POWER_SUPPLY_EXT_PROP_LCD_FLICKER, value);
			wpc_vout_ctrl_lcd_on = value.intval;

			switch (val_data) {
			case TX_ID_UNKNOWN:
				value.intval = charger->pdata->cable_type;
				break;
			case TX_ID_VEHICLE_PAD:
				if (charger->pad_vout == PAD_VOUT_10V) {
					if (charger->pdata->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV) {
						charger->pdata->cable_type = SEC_BATTERY_CABLE_WIRELESS_HV_VEHICLE;
						skip_send_online = true;
					} else {
						charger->pdata->cable_type = value.intval =
							SEC_BATTERY_CABLE_WIRELESS_HV_VEHICLE;
					}
				} else {
					charger->pdata->cable_type = value.intval = SEC_BATTERY_CABLE_WIRELESS_VEHICLE;
				}
				pr_info("%s: VEHICLE Wireless Charge PAD %s\n", __func__,
					charger->pad_vout == PAD_VOUT_10V ? "HV" : "");

				break;
			case TX_ID_STAND_TYPE_START:
				if (charger->pad_vout == PAD_VOUT_10V) {
					if (charger->pdata->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV) {
						charger->pdata->cable_type = SEC_BATTERY_CABLE_WIRELESS_HV_STAND;
						skip_send_online = true;
					} else {
						charger->pdata->cable_type = value.intval = SEC_BATTERY_CABLE_WIRELESS_HV_STAND;
					}
				} else {
					charger->pdata->cable_type = value.intval =
						SEC_BATTERY_CABLE_WIRELESS_STAND;
				}
				pr_info("%s: Cable(%d), STAND Wireless Charge PAD %s\n", __func__,
					charger->pdata->cable_type, charger->pad_vout == PAD_VOUT_10V ? "HV" : "");
				break;
			case TX_ID_MULTI_PORT_START ... TX_ID_MULTI_PORT_END:
				value.intval = charger->pdata->cable_type;
				pr_info("%s: MULTI PORT PAD : 0x%x\n", __func__, val_data);
				break;
			case TX_ID_BATT_PACK_U1200 ... TX_ID_BATT_PACK_END:
				if (charger->pad_vout == PAD_VOUT_10V) {
					if (charger->pdata->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV) {
						charger->pdata->cable_type = SEC_BATTERY_CABLE_WIRELESS_HV_PACK;
						skip_send_online = true;
						pr_info("%s: WIRELESS HV BATTERY PACK (PREP)\n", __func__);
					} else {
						charger->pdata->cable_type = value.intval =
							SEC_BATTERY_CABLE_WIRELESS_HV_PACK;
						pr_info("%s: WIRELESS HV BATTERY PACK\n", __func__);
					}
				} else {
					charger->pdata->cable_type = value.intval = SEC_BATTERY_CABLE_WIRELESS_PACK;
					pr_info("%s: WIRELESS BATTERY PACK\n", __func__);
				}
#if !defined(CONFIG_WIRELESS_RX_PHM_CTRL)
				if (charger->device_event & BATT_EXT_EVENT_CALL) {
					union power_supply_propval value2;

					pr_info("%s: enter PHM\n", __func__);
					/* notify "wireless" PHM status */
					value2.intval = 1;
					psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_CALL_EVENT, value2);
					mfc_send_command(charger, MFC_PHM_ON);
					msleep(250);
					mfc_send_command(charger, MFC_PHM_ON);
				}
#endif
				break;
			case TX_ID_UNO_TX:
			case TX_ID_UNO_TX_B0 ... TX_ID_UNO_TX_MAX:
				charger->pdata->cable_type = value.intval = SEC_BATTERY_CABLE_WIRELESS_TX;
				pr_info("@Tx_Mode %s: TX by UNO\n", __func__);
				if (capacity < 100)
					mfc_wpc_align_check(charger, ALIGN_WORK_DELAY);
#if !defined(CONFIG_WIRELESS_RX_PHM_CTRL)
				if (charger->device_event & BATT_EXT_EVENT_CALL) {
					pr_info("%s: enter PHM\n", __func__);
					mfc_send_command(charger, MFC_PHM_ON);
					msleep(250);
					mfc_send_command(charger, MFC_PHM_ON);
				}
#endif
				break;
			case TX_ID_NON_AUTH_PAD ... TX_ID_NON_AUTH_PAD_END:
				value.intval = charger->pdata->cable_type;
				break;
			case TX_ID_AUTH_PAD ... TX_ID_AUTH_PAD_END:
				if (charger->pdata->no_hv) {
					pr_info("%s: WIRELESS HV is disabled\n", __func__);
					break;
				}
				if (epp_mode(charger->rx_op_mode) && (charger->tx_id == TX_ID_P2400_PAD))
					mfc_send_command(charger, MFC_AFC_CONF_10V_TX);

				charger->vout_by_txid = mfc_get_wireless20_vout_by_txid(charger, val_data);
				charger->vrect_by_txid = mfc_get_wireless20_vrect_by_txid(charger, val_data);
				charger->max_power_by_txid = mfc_get_wireless20_max_power_by_txid(charger, val_data);
#if !defined(CONFIG_SEC_FACTORY)
				/* power on charging */
				if (charger->adt_transfer_status == WIRELESS_AUTH_WAIT) {
					/* change first wireless type to prepare wc2.0 for auth mode */
					charger->pdata->cable_type = value.intval = SEC_BATTERY_CABLE_PREPARE_WIRELESS_20;
					pr_info("%s %s: AUTH PAD for WIRELESS 2.0 : 0x%x\n", WC_AUTH_MSG,
							__func__, val_data);

					mfc_auth_set_configs(charger, AUTH_READY);
					if (delayed_work_pending(&charger->wpc_afc_vout_work)) {
						__pm_relax(charger->wpc_afc_vout_ws);
						cancel_delayed_work(&charger->wpc_afc_vout_work);
					}
					/* notify PPDE auth service to send TX PAD a request key */
					mfc_auth_send_adt_status(charger, WIRELESS_AUTH_START);
				} else if (charger->adt_transfer_status == WIRELESS_AUTH_PASS) {
					if (delayed_work_pending(&charger->wpc_afc_vout_work)) {
						__pm_relax(charger->wpc_afc_vout_ws);
						cancel_delayed_work(&charger->wpc_afc_vout_work);
					}
					charger->pdata->cable_type = value.intval = SEC_BATTERY_CABLE_HV_WIRELESS_20;
					skip_send_online = true;
					__pm_stay_awake(charger->wpc_afc_vout_ws);
					queue_delayed_work(charger->wqueue,
						&charger->wpc_afc_vout_work, msecs_to_jiffies(0));
				} else if (charger->adt_transfer_status == WIRELESS_AUTH_FAIL) {
					charger->pdata->cable_type = SEC_BATTERY_CABLE_HV_WIRELESS;
					value.intval = charger->pdata->cable_type;
					skip_send_online = true;
					if (charger->afc_tx_done) {
						charger->pdata->cable_type = SEC_BATTERY_CABLE_HV_WIRELESS;
						__pm_stay_awake(charger->wpc_afc_vout_ws);
						queue_delayed_work(charger->wqueue,
							&charger->wpc_afc_vout_work, msecs_to_jiffies(0));
					} else {
						mfc_set_online(charger, SEC_BATTERY_CABLE_WIRELESS);
					}
				} else {
					value.intval = charger->pdata->cable_type;
					pr_info("%s %s: undefined auth TX-ID scenario, auth service works strange...\n",
							WC_AUTH_MSG, __func__);
				}
#else /* FACTORY MODE dose not process auth, set 12W right after */
				if (charger->adt_transfer_status == WIRELESS_AUTH_WAIT) {
					if (delayed_work_pending(&charger->wpc_afc_vout_work)) {
						__pm_relax(charger->wpc_afc_vout_ws);
						cancel_delayed_work(&charger->wpc_afc_vout_work);
					}
					charger->adt_transfer_status = WIRELESS_AUTH_PASS;
					charger->pdata->cable_type = value.intval = SEC_BATTERY_CABLE_HV_WIRELESS_20;
					skip_send_online = true;
					__pm_stay_awake(charger->wpc_afc_vout_ws);
					queue_delayed_work(charger->wqueue,
						&charger->wpc_afc_vout_work, msecs_to_jiffies(0));
				}
				pr_info("%s %s: AUTH PAD for WIRELESS 2.0 but FACTORY MODE\n", WC_AUTH_MSG, __func__);
#endif
				break;
			case TX_ID_PG950_S_PAD:
				value.intval = charger->pdata->cable_type;
				pr_info("%s: PG950 PAD STAND : 0x%x\n", __func__, val_data);
				break;
			case TX_ID_PG950_D_PAD:
				value.intval = charger->pdata->cable_type;
				pr_info("%s: PG950 PAD DOWN : 0x%x\n", __func__, val_data);
				break;
			case TX_ID_P2400_PAD_NOAUTH:
				value.intval = charger->pdata->cable_type;
				if (epp_mode(charger->rx_op_mode))
					mfc_send_command(charger, MFC_AFC_CONF_10V_TX);
				break;
			default:
				value.intval = charger->pdata->cable_type;
				pr_info("%s: UNDEFINED PAD : 0x%x\n", __func__, val_data);
				break;
			}

			if (wpc_vout_ctrl_lcd_on && opfreq_ctrl_pad(charger->tx_id)) {
				pr_info("%s: tx id = 0x%x , set op freq\n", __func__, val_data);
				mfc_send_command(charger, MFC_SET_OP_FREQ);
				msleep(500);
			}

			if (epp_mode(charger->rx_op_mode)) {
				if (charger->adt_transfer_status == WIRELESS_AUTH_WAIT) {
					mfc_set_online(charger, SEC_BATTERY_CABLE_WIRELESS_EPP);
					charger->max_power_by_txid = TX_RX_POWER_8W * 100000;
					charger->current_rx_power = TX_RX_POWER_8W;
					skip_send_online = true;
				}
			}

			if (!skip_send_online) {
				pr_info("%s: change cable type\n", __func__);
				mfc_set_online(charger, value.intval);
			}

			/* send max vout informaion of this pad such as WIRELESS_VOUT_10V, WIRELESS_VOUT_12_5V */
			mfc_set_psy_wrl(charger, POWER_SUPPLY_EXT_PROP_WIRELESS_MAX_VOUT, charger->vout_by_txid);

			/*
			 * send rx power informaion of this pad such as
			 * SEC_WIRELESS_RX_POWER_12W, SEC_WIRELESS_RX_POWER_17_5W
			 */
			mfc_set_rx_power(charger, charger->max_power_by_txid);

#if defined(CONFIG_SEC_FACTORY)
			queue_delayed_work(charger->wqueue, &charger->wpc_rx_power_work, msecs_to_jiffies(3000));
#endif
			charger->tx_id_done = true;
			pr_info("%s: TX_ID : 0x%x\n", __func__, val_data);
		} else {
			pr_err("%s: TX ID isr is already done\n", __func__);
		}
	} else if (cmd_data == WPC_TX_COM_CHG_ERR) {
		switch (val_data) {
		case TX_CHG_ERR_OTP:
		case TX_CHG_ERR_OCP:
		case TX_CHG_ERR_DARKZONE:
			pr_info("%s: Received CHG error from the TX(0x%x)\n", __func__, val_data);
			break;
		/*case TX_CHG_ERR_FOD:*/
		case 0x20 ... 0x27:
			pr_info("%s: Received FOD state from the TX(0x%x)\n", __func__, val_data);
			value.intval = val_data;
			psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_ERR, value);
			break;
		default:
			pr_info("%s: Undefined Type(0x%x)\n", __func__, val_data);
			break;
		}
	} else if (cmd_data == WPC_TX_COM_RX_POWER) {
		charger->check_rx_power = false;
		if (delayed_work_pending(&charger->wpc_rx_power_trans_fail_work)) {
			__pm_relax(charger->wpc_rx_power_trans_fail_ws);
			cancel_delayed_work(&charger->wpc_rx_power_trans_fail_work);
		}
		if (charger->max_power_by_txid / 100000 < val_data) {
			pr_info("%s %s: RX Power(%d) is bigger than max power(%d)\n",
				WC_AUTH_MSG, __func__, val_data, charger->max_power_by_txid / 100000);
			val_data = charger->max_power_by_txid / 100000;
		}
		if (charger->current_rx_power != val_data) {
			switch (val_data) {
			case TX_RX_POWER_3W:
				pr_info("%s %s: RX Power is 3W\n", WC_AUTH_MSG, __func__);
				charger->current_rx_power = TX_RX_POWER_3W;
				break;
			case TX_RX_POWER_5W:
				pr_info("%s %s: RX Power is 5W\n", WC_AUTH_MSG, __func__);
				charger->current_rx_power = TX_RX_POWER_5W;
				break;
			case TX_RX_POWER_6_5W:
				pr_info("%s %s: RX Power is 6.5W\n", WC_AUTH_MSG, __func__);
				charger->current_rx_power = TX_RX_POWER_6_5W;
				break;
			case TX_RX_POWER_7_5W:
				pr_info("%s %s: RX Power is 7.5W\n", WC_AUTH_MSG, __func__);
				mfc_reset_rx_power(charger, TX_RX_POWER_7_5W);
				charger->current_rx_power = TX_RX_POWER_7_5W;
				break;
			case TX_RX_POWER_10W:
				pr_info("%s %s: RX Power is 10W\n", WC_AUTH_MSG, __func__);
				charger->current_rx_power = TX_RX_POWER_10W;
				break;
			case TX_RX_POWER_11W:
				pr_info("%s %s: RX Power is 11W\n", WC_AUTH_MSG, __func__);
				mfc_reset_rx_power(charger, TX_RX_POWER_11W);
				charger->current_rx_power = TX_RX_POWER_11W;
				break;
			case TX_RX_POWER_12W:
				pr_info("%s %s: RX Power is 12W\n", WC_AUTH_MSG, __func__);
				mfc_reset_rx_power(charger, TX_RX_POWER_12W);
				charger->current_rx_power = TX_RX_POWER_12W;
				break;
			case TX_RX_POWER_15W:
				pr_info("%s %s: RX Power is 15W\n", WC_AUTH_MSG, __func__);
				mfc_reset_rx_power(charger, TX_RX_POWER_15W);
				charger->current_rx_power = TX_RX_POWER_15W;
				break;
			case TX_RX_POWER_17_5W:
				pr_info("%s %s: RX Power is 17.5W\n", WC_AUTH_MSG, __func__);
				mfc_reset_rx_power(charger, TX_RX_POWER_17_5W);
				charger->current_rx_power = TX_RX_POWER_17_5W;
				break;
			case TX_RX_POWER_20W:
				pr_info("%s %s: RX Power is 20W\n", WC_AUTH_MSG, __func__);
				mfc_reset_rx_power(charger, TX_RX_POWER_20W);
				charger->current_rx_power = TX_RX_POWER_20W;
				break;
			default:
				pr_info("%s %s: Undefined RX Power(0x%x)\n", WC_AUTH_MSG, __func__, val_data);
				break;
			}
		} else {
			pr_info("%s: skip same RX Power\n", __func__);
		}
	} else if (cmd_data == WPC_TX_COM_WPS) {
		switch (val_data) {
		case WPS_AICL_RESET:
			value.intval = 1;
			pr_info("@Tx_mode %s: Rx devic AICL Reset\n", __func__);
			psy_do_property("wireless", set, POWER_SUPPLY_PROP_CURRENT_MAX, value);
			break;
		default:
			pr_info("%s %s: Undefined RX Power(0x%x)\n", WC_AUTH_MSG, __func__, val_data);
			break;
		}
	} else if (cmd_data == WPC_TX_COM_TX_PWR_BUDG) {
		pr_info("%s: WPC_TX_COM_TX_PWR_BUDG (0x%x)\n", __func__, val_data);
		mfc_recv_tx_pwr_budg(charger, val_data);
	}
	__pm_relax(charger->wpc_rx_ws);
}

static void mfc_set_unknown_pad_config(struct mfc_charger_data *charger)
{
	pr_info("%s: TX ID not Received, cable_type(%d)\n",
			__func__, charger->pdata->cable_type);

	if (epp_mode(charger->rx_op_mode)) {
		cancel_delayed_work(&charger->wpc_vout_mode_work);
		__pm_stay_awake(charger->wpc_vout_mode_ws);
		queue_delayed_work(charger->wqueue, &charger->wpc_vout_mode_work, 0);
		if (charger->mpp_epp_nego_done_power < TX_RX_POWER_8W) {
			mfc_set_online(charger, SEC_BATTERY_CABLE_WIRELESS_EPP_NV);
			mfc_set_epp_nv_mode(charger);
		} else {
			mfc_set_online(charger, SEC_BATTERY_CABLE_WIRELESS_EPP);
			mfc_set_epp_mode(charger, TX_RX_POWER_8W);
		}
		return;
	}

	if (!charger->pdata->default_clamp_volt) {
		if (is_hv_wireless_type(charger->pdata->cable_type)) {
			/* SET OV 13V */
			mfc_reg_write(charger->client, MFC_RX_OV_CLAMP_REG, 0x1);
		} else {
			/* SET OV 11V */
			mfc_reg_write(charger->client, MFC_RX_OV_CLAMP_REG, 0x0);
		}
	}
}

static void mfc_wpc_tx_pwr_budg_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_tx_pwr_budg_work.work);

	mfc_send_command(charger, MFC_REQ_TX_PWR_BUDG);
	__pm_relax(charger->wpc_tx_pwr_budg_ws);
}

static void mfc_wpc_tx_id_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_tx_id_work.work);
	union power_supply_propval value;

	pr_info("%s\n", __func__);

	if (!charger->tx_id) {
		if (charger->tx_id_cnt < 10) {
			mfc_send_command(charger, MFC_REQUEST_TX_ID);
			charger->req_tx_id = true;
			charger->tx_id_cnt++;

			pr_info("%s: request TX ID cnt (%d)\n", __func__, charger->tx_id_cnt);
			queue_delayed_work(charger->wqueue, &charger->wpc_tx_id_work, msecs_to_jiffies(1500));
			return;
		}

		mfc_set_unknown_pad_config(charger);
		charger->req_tx_id = false;
	} else {
		pr_info("%s: TX ID (0x%x)\n", __func__, charger->tx_id);
		if (charger->tx_id == TX_ID_JIG_PAD) {
			value.intval = 1;
			pr_info("%s: it is jig pad\n", __func__);
			psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_JIG_PAD, value);
		} else {
			mfc_cmfet_set_tx_id(charger->cmfet, charger->tx_id);
		}
	}

	__pm_relax(charger->wpc_tx_id_ws);
}

static irqreturn_t mfc_wpc_det_irq_thread(int irq, void *irq_data)
{
	struct mfc_charger_data *charger = irq_data;
	u8 det_state;

	pr_info("%s(%d)\n", __func__, irq);

	det_state = gpio_get_value(charger->pdata->wpc_det);

	if (charger->det_state != det_state) {
		union power_supply_propval value = {0, };

		value.intval = 0;
		psy_do_property("battery", set,
			POWER_SUPPLY_EXT_PROP_RX_PHM, value);

		pr_info("%s: det(%d to %d)\n", __func__, charger->det_state, det_state);
		charger->det_state = det_state;
		queue_delayed_work(charger->wqueue, &charger->wpc_det_work, 0);
	} else
		__pm_relax(charger->wpc_det_ws);

	return IRQ_HANDLED;
}

static bool mfc_check_skip_epp_timer(struct mfc_charger_data *charger)
{
	bool ret = false;

	if (charger->rx_op_mode == MFC_RX_MODE_WPC_MPP_FULL ||
		charger->rx_op_mode == MFC_RX_MODE_WPC_BPP)
		ret = true;

	return ret;
}

static void mfc_wpc_mpp_enable_ctrl(struct mfc_charger_data *charger, bool pdrc_state)
{
	int epp_clear_delay = 0;

	if (!pdrc_state) {
		/* disable epp */
		mfc_epp_enable(charger, 0);
		if (delayed_work_pending(&charger->epp_clear_timer_work)) {
			__pm_relax(charger->epp_clear_ws);
			cancel_delayed_work(&charger->epp_clear_timer_work);
		}
		return;
	}

	if (charger->rx_op_mode == MFC_RX_MODE_WPC_EPP)
		if (!is_3rd_pad((charger->mpp_epp_tx_id & 0xFFFF)))
			return;

	if (mfc_check_skip_epp_timer(charger)) {
		mfc_epp_enable(charger, 1);
		mfc_set_epp_count(charger, 0);
	} else {
		if (charger->epp_time == 6000) {
			epp_clear_delay = charger->epp_time;
			charger->epp_time = charger->pdata->epp_time_final;
		} else if (charger->epp_count == 0) {
			epp_clear_delay = charger->pdata->epp_time_init;
		} else if (charger->epp_count < 2) {
			epp_clear_delay = charger->pdata->epp_time_mid_1;
		} else if (charger->epp_count < charger->pdata->mpp_epp_max_count) {
			epp_clear_delay = charger->pdata->epp_time_mid_2;
		} else {
			epp_clear_delay = charger->pdata->epp_time_final;
		}

		if ((charger->mpp_epp_tx_id & 0xFFFF) == 0x0028) {
			pr_info("%s update delay %d -> 4000\n", __func__, epp_clear_delay);
			epp_clear_delay = 4000;
		}

		pr_info("%s: set delay(%d), count(%d, %d)\n",
			__func__, epp_clear_delay, charger->epp_count, charger->pdata->mpp_epp_max_count);
		__pm_stay_awake(charger->epp_clear_ws);
		queue_delayed_work(charger->wqueue, &charger->epp_clear_timer_work,
			msecs_to_jiffies(epp_clear_delay));

		mfc_set_epp_count(charger, charger->epp_count + 1);
	}
}

static irqreturn_t mfc_wpc_pdrc_irq_thread(int irq, void *irq_data)
{
	struct mfc_charger_data *charger = irq_data;
	u8 pdrc_state;

	pr_info("%s(%d)\n", __func__, irq);

	pdrc_state = gpio_get_value(charger->pdata->wpc_pdrc);

	if (charger->pdrc_state != pdrc_state) {
		pr_info("%s: cable type %d, pdrc(%d to %d)\n", __func__,
			charger->pdata->cable_type, charger->pdrc_state, pdrc_state);
		charger->pdrc_state = pdrc_state;
		if (charger->pdata->cable_type == SEC_BATTERY_CABLE_NONE)
			queue_delayed_work(charger->wqueue, &charger->wpc_pdrc_work, msecs_to_jiffies(0));
		else if (is_wireless_fake_type(charger->pdata->cable_type))
			queue_delayed_work(charger->wqueue, &charger->wpc_pdrc_work, msecs_to_jiffies(900));
		else
			__pm_relax(charger->wpc_pdrc_ws);

		mfc_wpc_mpp_enable_ctrl(charger, pdrc_state);

		if (pdrc_state) {
			if (delayed_work_pending(&charger->mpp_fake_ui_clear_work)) {
				__pm_relax(charger->mpp_fake_ui_clear_ws);
				cancel_delayed_work(&charger->mpp_fake_ui_clear_work);
			}

			if (charger->ping_freq < 3000 && charger->mpp_fake_ui_flag == true) {
				__pm_stay_awake(charger->mpp_fake_ui_clear_ws);
				queue_delayed_work(charger->wqueue, &charger->mpp_fake_ui_clear_work,
							msecs_to_jiffies(6000)); //reping time + 4s
			} else {
				//clear MPP fake ui
				charger->mpp_fake_ui_flag = false;
			}
		}
	}  else {
		if (!delayed_work_pending(&charger->wpc_pdrc_work))
			__pm_relax(charger->wpc_pdrc_ws);
	}

	return IRQ_HANDLED;
}

static irqreturn_t mfc_wpc_pdet_b_irq_thread(int irq, void *irq_data)
{
	struct mfc_charger_data *charger = irq_data;
	u8 pdetb_state = 0, det_state = 0;

	pdetb_state = gpio_get_value(charger->pdata->wpc_pdet_b);
	det_state = gpio_get_value(charger->pdata->wpc_det);

	pr_info("%s : pdet_b = %d, wpc_det = %d\n",
		__func__, pdetb_state, det_state);

	if (charger->rx_phm_status && !det_state && pdetb_state) {
		pr_info("%s wireless charger deactivated during phm\n", __func__);
		charger->rx_phm_state = END_PHM;
		__pm_stay_awake(charger->wpc_rx_phm_ws);
		queue_delayed_work(charger->wqueue, &charger->wpc_rx_phm_work, 0);
	}
	__pm_relax(charger->wpc_pdet_b_ws);

	return IRQ_HANDLED;
}

static void mfc_set_iec_params(struct mfc_charger_data *charger, mfc_charger_platform_data_t *pdata)
{
	pr_info("%s: iec_q_thresh_1 = %d\n", __func__, pdata->iec_q_thresh_1);
	mfc_reg_write(charger->client, MFC_IEC_Q_THRESH1_REG, pdata->iec_q_thresh_1);

	pr_info("%s: iec_q_thresh_2 = %d\n", __func__, pdata->iec_q_thresh_2);
	mfc_reg_write(charger->client, MFC_IEC_Q_THRESH2_REG, pdata->iec_q_thresh_2);

	pr_info("%s: iec_fres_thresh_1 = %d\n", __func__, pdata->iec_fres_thresh_1);
	mfc_reg_write(charger->client, MFC_IEC_FRES_THRESH1_REG, pdata->iec_fres_thresh_1);

	pr_info("%s: iec_fres_thresh_2 = %d\n", __func__, pdata->iec_fres_thresh_2);
	mfc_reg_write(charger->client, MFC_IEC_FRES_THRESH2_REG, pdata->iec_fres_thresh_2);

	pr_info("%s: iec_power_limit_thresh = %d\n", __func__, pdata->iec_power_limit_thresh);
	mfc_reg_write(charger->client, MFC_IEC_POWER_LIMIT_THRESH_L_REG, pdata->iec_power_limit_thresh);
	mfc_reg_write(charger->client, MFC_IEC_POWER_LIMIT_THRESH_H_REG, (pdata->iec_power_limit_thresh >> 8));

	pr_info("%s: iec_ploss_thresh_1 = %d\n", __func__, pdata->iec_ploss_thresh_1);
	mfc_reg_write(charger->client, MFC_IEC_PLOSS_THRESH1_L_REG, pdata->iec_ploss_thresh_1);
	mfc_reg_write(charger->client, MFC_IEC_PLOSS_THRESH1_H_REG, (pdata->iec_ploss_thresh_1 >> 8));

	pr_info("%s: iec_ploss_thresh_2 = %d\n", __func__, pdata->iec_ploss_thresh_2);
	mfc_reg_write(charger->client, MFC_IEC_PLOSS_THRESH2_L_REG, pdata->iec_ploss_thresh_2);
	mfc_reg_write(charger->client, MFC_IEC_PLOSS_THRESH2_H_REG, (pdata->iec_ploss_thresh_2 >> 8));

	pr_info("%s: iec_ploss_freq_thresh_1 = %d\n", __func__, pdata->iec_ploss_freq_thresh_1);
	mfc_reg_write(charger->client, MFC_IEC_PLOSS_FREQ_THRESH1_REG, pdata->iec_ploss_freq_thresh_1);

	pr_info("%s: iec_ploss_freq_thresh_2 = %d\n", __func__, pdata->iec_ploss_freq_thresh_2);
	mfc_reg_write(charger->client, MFC_IEC_PLOSS_FREQ_THRESH2_REG, pdata->iec_ploss_freq_thresh_2);

	pr_info("%s: iec_ta_power_limit_thresh = %d\n", __func__, pdata->iec_ta_power_limit_thresh);
	mfc_reg_write(charger->client, MFC_IEC_TA_POWER_LIMIT_THRESH_L_REG, pdata->iec_ta_power_limit_thresh);
	mfc_reg_write(charger->client, MFC_IEC_TA_POWER_LIMIT_THRESH_H_REG, (pdata->iec_ta_power_limit_thresh >> 8));

	pr_info("%s: iec_ta_ploss_thresh_1 = %d\n", __func__, pdata->iec_ta_ploss_thresh_1);
	mfc_reg_write(charger->client, MFC_IEC_TA_PLOSS_THRESH1_L_REG, pdata->iec_ta_ploss_thresh_1);
	mfc_reg_write(charger->client, MFC_IEC_TA_PLOSS_THRESH1_H_REG, (pdata->iec_ta_ploss_thresh_1 >> 8));

	pr_info("%s: iec_ta_ploss_thresh_2 = %d\n", __func__, pdata->iec_ta_ploss_thresh_2);
	mfc_reg_write(charger->client, MFC_IEC_TA_PLOSS_THRESH2_L_REG, pdata->iec_ta_ploss_thresh_2);
	mfc_reg_write(charger->client, MFC_IEC_TA_PLOSS_THRESH2_H_REG, (pdata->iec_ta_ploss_thresh_2 >> 8));

	pr_info("%s: iec_ta_ploss_freq_thresh_1 = %d\n", __func__, pdata->iec_ta_ploss_freq_thresh_1);
	mfc_reg_write(charger->client, MFC_IEC_TA_PLOSS_FREQ_THRESH1_REG, pdata->iec_ta_ploss_freq_thresh_1);

	pr_info("%s: iec_ta_ploss_freq_thresh_2 = %d\n", __func__, pdata->iec_ta_ploss_freq_thresh_2);
	mfc_reg_write(charger->client, MFC_IEC_TA_PLOSS_FREQ_THRESH2_REG, pdata->iec_ta_ploss_freq_thresh_2);

	pr_info("%s: iec_ploss_fod_enable = %d\n", __func__, pdata->iec_ploss_fod_enable);
	mfc_reg_write(charger->client, MFC_IEC_PLOSS_FOD_ENABLE_REG, pdata->iec_ploss_fod_enable);

	pr_info("%s: iec_qfod_enable = %d\n", __func__, pdata->iec_qfod_enable);
	mfc_reg_write(charger->client, MFC_IEC_QFOD_ENABLE_REG, pdata->iec_qfod_enable);
}

/* mfc_mst_routine : MST dedicated codes */
static void mfc_mst_routine(struct mfc_charger_data *charger, u8 irq_src_l, u8 irq_src_h)
{
	u8 data = 0;

	pr_info("%s\n", __func__);

	if (charger->is_mst_on == MST_MODE_2) {
		charger->wc_tx_enable = false;
#if defined(CONFIG_MST_V2)
		// it will send MST driver a message.
		mfc_send_mst_cmd(MST_MODE_ON, charger, irq_src_l, irq_src_h);
#endif
	} else if (charger->wc_tx_enable) {
		mfc_reg_read(charger->client, MFC_STATUS_H_REG, &data);
		data &= 0x4; /* AC MISSING DETECT */
		msleep(100);
		pr_info("@Tx_Mode %s: 0x21 Register AC Missing(%d)\n", __func__, data);
		if (data) {
			/* initialize control reg */
			mfc_set_tx_conflict_current(charger, charger->pdata->tx_conflict_curr);
			mfc_set_iec_params(charger, charger->pdata);
			mfc_reg_write(charger->client, MFC_TX_IUNO_HYS_REG, 0x50);
			mfc_reg_write(charger->client, MFC_DEMOD1_REG, 0x00);
			mfc_reg_write(charger->client, MFC_DEMOD2_REG, 0x00);
			mfc_reg_write(charger->client, MFC_MST_MODE_SEL_REG, 0x03); /* set TX-ON mode */
			mfc_set_cep_timeout(charger, charger->pdata->cep_timeout); // this is only for CAN
			pr_info("@Tx_Mode %s: TX-ON Mode : %d\n", __func__, charger->pdata->wc_ic_rev);
		} // ac missing is 0, ie, TX detected
	}
}

static irqreturn_t mfc_wpc_irq_handler(int irq, void *irq_data)
{
	struct mfc_charger_data *charger = irq_data;

	if (irq == charger->pdata->irq_wpc_int)
		__pm_stay_awake(charger->wpc_ws);
	else if (irq == charger->pdata->irq_wpc_det)
		__pm_stay_awake(charger->wpc_det_ws);
	else if (irq == charger->pdata->irq_wpc_pdrc)
		__pm_stay_awake(charger->wpc_pdrc_ws);
	else if (irq == charger->pdata->irq_wpc_pdet_b)
		__pm_stay_awake(charger->wpc_pdet_b_ws);

#if IS_ENABLED(CONFIG_ENABLE_WIRELESS_IRQ_IN_SLEEP)
	return charger->is_suspend ? IRQ_HANDLED : IRQ_WAKE_THREAD;
#else
	return IRQ_WAKE_THREAD;
#endif
}

static void mfc_mpp_epp_nego_done(struct mfc_charger_data *charger)
{
	u8 reg_data = 0;

	mfc_reg_read(charger->client, MFC_SYS_OP_MODE_REG, &reg_data);
	charger->rx_op_mode = reg_data >> 5;
	pr_info("@MPP @EPP %s:Rx op_mode %d\n", __func__, charger->rx_op_mode);

	mfc_reg_read(charger->client, MFC_MPP_EPP_NEGO_DONE_POWER_L_REG, &reg_data);
	charger->mpp_epp_nego_done_power = reg_data;
	mfc_reg_read(charger->client, MFC_MPP_EPP_NEGO_DONE_POWER_H_REG, &reg_data);
	charger->mpp_epp_nego_done_power |= (reg_data << 8);

	//Vrect is 12V and load current<595mA. modulation base: 70mA
	//mfc_reg_write(charger->client, MFC_MPP_DC_CURRENT_MOD_BASE_LIGHT_REG, 70);
	//DC current modulation depth setting value, 50mA
	//mfc_reg_write(charger->client, MFC_MPP_DC_CURRENT_MOD_DEPTH_REG, 50);

	mfc_reg_read(charger->client, MFC_MPP_PTX_EXTENDED_ID0_REG, &reg_data);
	charger->mpp_epp_tx_id = (reg_data << 16);
	mfc_reg_read(charger->client, MFC_MPP_PTX_EXTENDED_ID1_REG, &reg_data);
	charger->mpp_epp_tx_id |= (reg_data << 8);
	mfc_reg_read(charger->client, MFC_MPP_PTX_EXTENDED_ID2_REG, &reg_data);
	charger->mpp_epp_tx_id |= reg_data;

	mfc_reg_read(charger->client, MFC_MPP_EPP_POTENTIAL_LOAD_POWER_L_REG, &reg_data);
	charger->mpp_epp_tx_potential_load_power = reg_data;
	mfc_reg_read(charger->client, MFC_MPP_EPP_POTENTIAL_LOAD_POWER_H_REG, &reg_data);
	charger->mpp_epp_tx_potential_load_power |= (reg_data << 8);

	mfc_reg_read(charger->client, MFC_MPP_EPP_NEGOTIABLE_LOAD_POWER_L_REG, &reg_data);
	charger->mpp_epp_tx_negotiable_load_power = reg_data;
	mfc_reg_read(charger->client, MFC_MPP_EPP_NEGOTIABLE_LOAD_POWER_H_REG, &reg_data);
	charger->mpp_epp_tx_negotiable_load_power |= (reg_data << 8);

	pr_info("@MPP @EPP %s: Tx id: 0x%06X, Tx nego done power: %dmW, Tx potential power: %dmW, Tx negotiable power: %dmW\n",
		__func__, charger->mpp_epp_tx_id, charger->mpp_epp_nego_done_power * 100,
		charger->mpp_epp_tx_potential_load_power * 100, charger->mpp_epp_tx_negotiable_load_power * 100);

	if (epp_mode(charger->rx_op_mode)) {
		charger->max_power_by_txid = charger->pdata->mpp_epp_def_power;
		charger->current_rx_power = charger->max_power_by_txid / 100000;

		/* check epp nego power */
		if ((charger->mpp_epp_nego_done_power < TX_RX_POWER_8W) || charger->is_full_status) {
			charger->vrect_by_txid = MFC_AFC_CONF_5V;
			charger->vout_by_txid = WIRELESS_VOUT_5_5V;
			charger->vout_mode = WIRELESS_VOUT_5_5V;
			charger->pdata->vout_status = MFC_VOUT_5_5V;
		} else {
			charger->vrect_by_txid = MFC_AFC_CONF_12_5V_TX;
			charger->vout_by_txid = charger->pdata->epp_vout;
			charger->vout_mode = charger->pdata->epp_vout;
			charger->pdata->vout_status = MFC_VOUT_12V;
			if (charger->sleep_mode)
				charger->vout_mode = WIRELESS_VOUT_5_5V_STEP;
		}

		mfc_set_epp_mode(charger, charger->mpp_epp_nego_done_power > TX_RX_POWER_8W ?
				TX_RX_POWER_8W : charger->mpp_epp_nego_done_power);
		if ((charger->vout_mode == WIRELESS_VOUT_5_5V_STEP) ||
			(charger->vout_mode == WIRELESS_VOUT_5_5V))
			mfc_fod_set_vout(charger->fod, 5500);
		else
			mfc_fod_set_vout(charger->fod, 12000);
		mfc_fod_set_op_mode(charger->fod, WPC_OP_MODE_EPP);
		mfc_set_online(charger, SEC_BATTERY_CABLE_WIRELESS_EPP_FAKE);

		/* check op mode after 30secs in order to rerun mode_change_work when if mode change INT does not work */
		__pm_stay_awake(charger->epp_mode_change_check_ws);
		queue_delayed_work(charger->wqueue, &charger->epp_mode_change_check_work, msecs_to_jiffies(45000));
		charger->mpp_fake_ui_flag = false;  //change to EPP fake ui
	} else if (mpp_mode(charger->rx_op_mode)) {
		if (charger->mpp_epp_nego_done_power < TX_RX_POWER_5W)
			mfc_set_mpp_default_current(charger,
					(charger->mpp_epp_nego_done_power * 100 / mfc_get_volt(charger->pdata->mpp_vout)));

		mfc_reg_read(charger->client, MFC_ESTIMATED_K_L_REG, &reg_data);    //just MPP use estimated K
		charger->mpp_estimated_k = reg_data;
		mfc_reg_read(charger->client, MFC_ESTIMATED_K_H_REG, &reg_data);
		charger->mpp_estimated_k |= (reg_data << 8);

		//actual k = mpp_estimated_k/4095
		pr_info("@MPP %s: estimated K: %d\n", __func__, charger->mpp_estimated_k * 1000 / 4095);

		/* check mpp nego power */
		charger->vrect_by_txid = charger->pdata->mpp_vout;
		charger->vout_by_txid = charger->pdata->mpp_vout;
		charger->vout_mode = charger->pdata->mpp_vout;
		charger->pdata->vout_status = MFC_VOUT_12V;

		if (charger->adt_transfer_status >= WIRELESS_AUTH_FAIL) { // Fail or Pass
			mfc_set_epp_mode(charger, charger->mpp_epp_nego_done_power > TX_RX_POWER_8W ?
				TX_RX_POWER_8W : charger->mpp_epp_nego_done_power);
		}
		//should change to SEC_BATTERY_CABLE_WIRELESS_MPP, and change to MPP ui
		mfc_set_online(charger, SEC_BATTERY_CABLE_WIRELESS_MPP);
		charger->mpp_fake_ui_flag = false;
	}
	mfc_fod_set_vendor_id(charger->fod, (charger->mpp_epp_tx_id & 0xFF));
}

static irqreturn_t mfc_wpc_irq_thread(int irq, void *irq_data)
{
	struct mfc_charger_data *charger = irq_data;
	u8 int_state;
	int ret = 0;
	u8 irq_src_l = 0, irq_src_h = 0, irq_src1_l = 0, irq_src1_h = 0;
	u8 status_l = 0, status_h = 0, status1_l = 0, status1_h = 0;
	bool end_irq = false;
	union power_supply_propval value;

	pr_info("%s: start(%d)!\n", __func__, irq);

	int_state = gpio_get_value(charger->pdata->wpc_int);
	pr_info("%s: int_state = %d\n", __func__, int_state);

	if (int_state == 1) {
		pr_info("%s: Falling edge, End up ISR\n", __func__);
		mfc_wpc_check_fod_count(charger, FOD_CNT_ADD);
		__pm_relax(charger->wpc_ws);
		return IRQ_HANDLED;
	}

	ret = mfc_reg_read(charger->client, MFC_INT_A_L_REG, &irq_src_l);
	if (ret < 0) {
		pr_err("%s: Failed to read INT_A_L_REG: %d\n", __func__, ret);
		__pm_relax(charger->wpc_ws);
		return IRQ_HANDLED;
	}
	ret = mfc_reg_read(charger->client, MFC_INT_A_H_REG, &irq_src_h);
	if (ret < 0) {
		pr_err("%s: Failed to read INT_A_H_REG: %d\n", __func__, ret);
		__pm_relax(charger->wpc_ws);
		return IRQ_HANDLED;
	}

	ret = mfc_reg_read(charger->client, MFC_INT_B_L_REG, &irq_src1_l);
	if (ret < 0) {
		pr_err("%s: Failed to read INT_B_L_REG: %d\n", __func__, ret);
		__pm_relax(charger->wpc_ws);
		return IRQ_HANDLED;
	}
	ret = mfc_reg_read(charger->client, MFC_INT_B_H_REG, &irq_src1_h);
	if (ret < 0) {
		pr_err("%s: Failed to read INT_B_H_REG: %d\n", __func__, ret);
		__pm_relax(charger->wpc_ws);
		return IRQ_HANDLED;
	}

	ret = mfc_reg_read(charger->client, MFC_STATUS_L_REG, &status_l);
	if (ret < 0) {
		pr_err("%s: Failed to read STATUS_L_REG: %d\n", __func__, ret);
		__pm_relax(charger->wpc_ws);
		return IRQ_HANDLED;
	}
	ret = mfc_reg_read(charger->client, MFC_STATUS_H_REG, &status_h);
	if (ret < 0) {
		pr_err("%s: Failed to read STATUS_H_REG: %d\n", __func__, ret);
		__pm_relax(charger->wpc_ws);
		return IRQ_HANDLED;
	}

	ret = mfc_reg_read(charger->client, MFC_STATUS1_L_REG, &status1_l);
	if (ret < 0) {
		pr_err("%s: Failed to read STATUS1_L_REG: %d\n", __func__, ret);
		__pm_relax(charger->wpc_ws);
		return IRQ_HANDLED;
	}
	ret = mfc_reg_read(charger->client, MFC_STATUS1_H_REG, &status1_h);
	if (ret < 0) {
		pr_err("%s: Failed to read STATUS1_H_REG: %d\n", __func__, ret);
		__pm_relax(charger->wpc_ws);
		return IRQ_HANDLED;
	}
	pr_info("%s: interrupt source(0x%x), status(0x%x)\n", __func__,
		((irq_src1_h << 24) | (irq_src1_l << 16) | (irq_src_h << 8) | irq_src_l),
		((status1_h << 24) | (status1_l << 16) | (status_h << 8) | status_l));

	if (irq_src1_h & MFC_INTB_H_PROTOCOL_SELECT_MASK) {
		int capacity = 0, chg_type = 0;

		ret = psy_do_property("battery", get, POWER_SUPPLY_PROP_CAPACITY, value);
		if (!ret)
			capacity = value.intval;
		ret = psy_do_property("battery", get, POWER_SUPPLY_PROP_ONLINE, value);
		if (!ret) {
			pr_info("%s: online=%d\n", __func__, value.intval);
			chg_type = is_all_hv_wire_type(value.intval);
		}
		pr_info("%s: EPP or MPP %d times, otg status: %d swell(%d) capacity(%d) chg_type(%d)\n",
			__func__, (charger->epp_count + 1), charger->is_otg_on,
			charger->high_swell, capacity, chg_type);
		if (charger->is_otg_on || charger->high_swell || capacity >= 100 || chg_type ||
			charger->is_mpla_thr_recov)
			mfc_protocol_selection(charger, MFC_PROTOCOL_BPP_MASK);
		else if (charger->epp_count < charger->pdata->mpp_epp_max_count) {
			if (charger->magnet_case_det && !charger->pdata->mpp_disable)
				mfc_protocol_selection(charger, MFC_PROTOCOL_MPP_FULL_MASK);
			else
				mfc_protocol_selection(charger, MFC_PROTOCOL_BPP_MASK);
		}
		mfc_wpc_check_fod_count(charger, FOD_CNT_ADD);
		if (charger->magnet_case_det && !charger->pdata->mpp_disable) {
			mfc_reg_write(charger->client, MFC_MPP_ALPHA0_RX_REG, charger->pdata->alpha0_rx);
			mfc_reg_write(charger->client, MFC_MPP_ALPHA1_RX_REG, charger->pdata->alpha1_rx);
			mfc_reg_write(charger->client, MFC_MPP_ALPHA_KTH_RX_REG, charger->pdata->alpha_kth_rx);
		}
	}

	if (irq_src1_l & MFC_INTB_L_MPP_SUPPROT_MASK) {
		if (status1_l & MFC_STAT1_L_MPP_SUPPROT_MASK) {
			//Should MPP FAKE Charing UI
			charger->mpp_fake_ui_flag = true;
			pr_info("@MPP %s:Tx pad support MPP IRQ\n", __func__);
			mfc_epp_enable(charger, 1);
			if (delayed_work_pending(&charger->mpp_fake_ui_clear_work)) {
				__pm_relax(charger->mpp_fake_ui_clear_ws);
				cancel_delayed_work(&charger->mpp_fake_ui_clear_work);
			}

			mfc_set_online(charger, SEC_BATTERY_CABLE_WIRELESS_MPP_FAKE);
			cancel_delayed_work(&charger->wpc_check_mpp_work);
			__pm_stay_awake(charger->check_mpp_ws);
			queue_delayed_work(charger->wqueue, &charger->wpc_check_mpp_work, msecs_to_jiffies(3000));
		} else
			pr_info("@MPP %s:Tx pad not support MPP IRQ, Rx will enter EPP mode\n", __func__);
	}

	if (irq_src1_l & MFC_INTB_L_EPP_SUPPROT_MASK) {
		if (status1_l & MFC_STAT1_L_EPP_SUPPROT_MASK) {
			//mfc_mpp_full_mode_trans(charger, 0x1); // EPP
			pr_info("@EPP %s:Tx pad support EPP IRQ\n", __func__);
		} else
			pr_info("@EPP %s:Tx pad not support EPP IRQ, Rx will enter BPP mode\n", __func__);
	}

	if (irq_src1_l & MFC_INTB_L_360K_NEGO_PASS_MASK) {
		pr_info("@MPP %s: Rx MPP nego pass IRQ\n", __func__);
		mfc_mpp_epp_nego_done(charger);

		if (charger->magnet_case_det) {
			mfc_reg_write(charger->client, MFC_MPP_M_COIL_L_REG, (charger->pdata->m_coil & 0xFF));
			mfc_reg_write(charger->client, MFC_MPP_M_COIL_H_REG, ((charger->pdata->m_coil >> 8) & 0xFF));
			mfc_reg_write(charger->client, MFC_MPP_RX_COIL_RAC_L_REG, (charger->pdata->rx_coil_rac & 0xFF));
			mfc_reg_write(charger->client, MFC_MPP_RX_COIL_RAC_H_REG, ((charger->pdata->rx_coil_rac >> 8) & 0xFF));
			mfc_reg_write(charger->client, MFC_MPP_RX_CAP_ESR_L_REG, (charger->pdata->rx_cap_esr & 0xFF));
			mfc_reg_write(charger->client, MFC_MPP_RX_CAP_ESR_H_REG, ((charger->pdata->rx_cap_esr >> 8) & 0xFF));
		}
	}

	if (irq_src1_l & MFC_INTB_L_EPP_NEGO_PASS_MASK) {
		pr_info("@EPP %s:Rx EPP nego pass IRQ\n", __func__);
		mfc_mpp_epp_nego_done(charger);
	}

	if (irq_src1_l & MFC_INTB_L_EPP_NEGO_FAIL_MASK) {
		pr_info("@EPP %s:Rx EPP nego fail IRQ\n", __func__);
	}

	if (irq_src1_l & MFC_INTB_L_INCREASE_POWER_MASK) {
		pr_info("@MPP %s:Rx MPP increase power IRQ\n", __func__);
		value.intval = 1;
		psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_MPP_ICL_CTRL, value);
	}

	if (irq_src1_l & MFC_INTB_L_DECREASE_POWER_MASK) {
		pr_info("@MPP %s:Rx MPP decrease power IRQ\n", __func__);
		value.intval = 0;
		psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_MPP_ICL_CTRL, value);
	}

	if (irq_src1_l & MFC_INTB_L_EXIT_CLOAK_MASK) {
		int capacity = 0, chg_type = 0;
		u8 cloak_exit_reason;

		ret = psy_do_property("battery", get, POWER_SUPPLY_PROP_CAPACITY, value);
		if (!ret)
			capacity = value.intval;
		ret = psy_do_property("battery", get, POWER_SUPPLY_PROP_ONLINE, value);
		if (!ret) {
			pr_info("%s: online=%d\n", __func__, value.intval);
			chg_type = is_all_hv_wire_type(value.intval);
		}

		mfc_epp_enable(charger, !(chg_type || charger->is_otg_on || capacity >= 100));
		mfc_reg_read(charger->client, MFC_MPP_EXIT_CLOAK_REASON_REG, &cloak_exit_reason);
		pr_info("@MPP %s: Exit Cloak IRQ - reason : 0x%x mpp_cloak(%d) otg status: %d capacity(%d) ct(%d)\n",
			__func__, cloak_exit_reason, charger->mpp_cloak, charger->is_otg_on, capacity, chg_type);
		charger->mpp_cloak_status = true;
		value.intval = CLOAK_TURN_OFF_PWR;
		psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_MPP_CLOAK, value);
		cps4038_set_force_vout(charger, WIRELESS_VOUT_FORCE_13V);
	}

	if (irq_src_h & MFC_INTA_H_AC_MISSING_DET_MASK) {
		pr_info("%s: 1AC Missing ! : MFC is on\n", __func__);
		mfc_mst_routine(charger, irq_src_l, irq_src_h);
	}

	if (irq_src_l & MFC_INTA_L_OP_MODE_MASK) {
		pr_info("%s: MODE CHANGE IRQ !\n", __func__);
		__pm_stay_awake(charger->mode_change_ws);
		queue_delayed_work(charger->wqueue, &charger->mode_change_work, 0);
	}

	if ((irq_src_l & MFC_INTA_L_OVER_VOL_MASK) || (irq_src_l & MFC_INTA_L_OVER_CURR_MASK))
		pr_info("%s: ABNORMAL STAT IRQ !\n", __func__);

	if (irq_src_l & MFC_INTA_L_OVER_TEMP_MASK) {
		pr_info("%s: OVER TEMP IRQ !\n", __func__);
		if (charger->wc_tx_enable) {
			value.intval = BATT_TX_EVENT_WIRELESS_RX_UNSAFE_TEMP;
			end_irq = true;
			goto INT_END;
		}
	}

	if (irq_src_l & MFC_INTA_L_STAT_VRECT_MASK) {
		int epp_ref_qf = 0, epp_ref_rf = 0;

		mfc_get_adc(charger, MFC_ADC_VRECT);
		mfc_fod_get_ext(charger->fod, MFC_FOD_EXT_EPP_REF_QF, &epp_ref_qf);
		mfc_fod_get_ext(charger->fod, MFC_FOD_EXT_EPP_REF_RF, &epp_ref_rf);
		pr_info("%s: Vrect IRQ (0x%x, 0x%x)!\n", __func__, epp_ref_qf, epp_ref_rf);

		if (epp_ref_qf)
			mfc_reg_write(charger->client, MFC_MPP_FOD_QF_REG, epp_ref_qf);
		if (epp_ref_rf)
			mfc_reg_write(charger->client, MFC_MPP_FOD_RF_REG, epp_ref_rf);
		mfc_mpp_epp_nego_power_set(charger, charger->pdata->mpp_epp_power_level);

		if (charger->magnet_case_det) {
			mfc_reg_write(charger->client, MFC_MPP_ALPHA_FM_L_REG, (charger->pdata->alpha_fm & 0xFF));
			mfc_reg_write(charger->client, MFC_MPP_ALPHA_FM_H_REG, ((charger->pdata->alpha_fm >> 8) & 0xFF));
			mfc_reg_write(charger->client, MFC_MPP_ALPHA_FM_DC_L_REG, (charger->pdata->alpha_fm_dc & 0xFF));
			mfc_reg_write(charger->client, MFC_MPP_ALPHA_FM_DC_H_REG, ((charger->pdata->alpha_fm_dc >> 8) & 0xFF));
			mfc_reg_write(charger->client, MFC_MPP_G_COIL_Tx_L_REG, (charger->pdata->g_coil_tx & 0xFF));
			mfc_reg_write(charger->client, MFC_MPP_G_COIL_Tx_H_REG, ((charger->pdata->g_coil_tx >> 8) & 0xFF));
		}

		if (charger->is_mst_on == MST_MODE_2 || charger->wc_tx_enable) {
			pr_info("%s: Noise made false Vrect IRQ!\n", __func__);
			if (charger->wc_tx_enable) {
				value.intval = BATT_TX_EVENT_WIRELESS_TX_ETC;
				end_irq = true;
				goto INT_END;
			}
		}
	}

	if (irq_src_l & MFC_INTA_L_TXCONFLICT_MASK) {
		pr_info("@Tx_Mode %s: TXCONFLICT IRQ !\n", __func__);
		if (charger->wc_tx_enable && (status_l & MFC_STAT_L_TXCONFLICT_MASK)) {
			value.intval = BATT_TX_EVENT_WIRELESS_TX_ETC;
			end_irq = true;
			goto INT_END;
		}
	}

	if (irq_src_h & MFC_INTA_H_TRX_DATA_RECEIVED_MASK) {
		pr_info("%s: TX RECEIVED IRQ !\n", __func__);
		if (charger->wc_tx_enable && !delayed_work_pending(&charger->wpc_tx_isr_work)) {
			__pm_stay_awake(charger->wpc_tx_ws);
			queue_delayed_work(charger->wqueue, &charger->wpc_tx_isr_work, 0);
		} else if (charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_STAND ||
			charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_HV_STAND) {
			pr_info("%s: Don't run ISR_WORK for NO ACK !\n", __func__);
		} else if (!delayed_work_pending(&charger->wpc_isr_work)) {
			__pm_stay_awake(charger->wpc_rx_ws);
			queue_delayed_work(charger->wqueue, &charger->wpc_isr_work, 0);
		}
	}

	if (irq_src_h & MFC_INTA_H_TX_CON_DISCON_MASK) {
		if (charger->wc_tx_enable) {
			pr_info("@Tx_Mode %s: TX CONNECT IRQ !\n", __func__);
			charger->tx_status = SEC_TX_POWER_TRANSFER;
			if (status_h & MFC_STAT_H_TX_CON_DISCON_MASK) {
				/* determine rx connection status with tx sharing mode */
				if (!charger->wc_rx_connected) {
					charger->wc_rx_connected = true;
					queue_delayed_work(charger->wqueue, &charger->wpc_rx_connection_work, 0);
				}
			} else {
				/* determine rx connection status with tx sharing mode */
				if (!charger->wc_rx_connected) {
					pr_info("@Tx_Mode %s: Ignore IRQ!! already Rx disconnected!\n", __func__);
				} else {
					charger->wc_rx_connected = false;
					queue_delayed_work(charger->wqueue, &charger->wpc_rx_connection_work, 0);
				}
			}
		} else {
			if (status_l & MFC_INTA_L_STAT_VRECT_MASK)
				pr_info("%s: VRECT SIGNAL !\n", __func__);
		}
	}

	/* when rx is not detacted in 3 mins */
	if (irq_src_h & MFC_INTA_H_TX_MODE_RX_NOT_DET_MASK) {
		if (!(irq_src_h & MFC_INTA_H_TX_CON_DISCON_MASK)) {
			pr_info("@Tx_Mode %s: Receiver NOT DETECTED IRQ !\n", __func__);
			if (charger->wc_tx_enable) {
				value.intval = BATT_TX_EVENT_WIRELESS_TX_ETC;
				end_irq = true;
				goto INT_END;
			}
		}
	}

	if (irq_src_h & MFC_STAT_H_ADT_RECEIVED_MASK) {
		pr_info("%s %s: ADT Received IRQ !\n", WC_AUTH_MSG, __func__);
		mfc_auth_adt_read(charger, ADT_buffer_rdata);
	}

	if (irq_src_h & MFC_STAT_H_ADT_SENT_MASK) {
		pr_info("%s %s: ADT Sent successful IRQ !\n", WC_AUTH_MSG, __func__);
		mfc_reg_update(charger->client, MFC_INT_A_ENABLE_H_REG, 0, MFC_STAT_H_ADT_SENT_MASK);
	}

	if (irq_src_h & MFC_INTA_H_TX_FOD_MASK) {
		pr_info("%s: TX FOD IRQ !\n", __func__);
		// this code will move to wpc_tx_isr_work
		if (charger->wc_tx_enable) {
			if (status_h & MFC_STAT_H_TX_FOD_MASK) {
				charger->wc_rx_fod = true;
				value.intval = BATT_TX_EVENT_WIRELESS_TX_FOD;
				end_irq = true;
				goto INT_END;
			}
		}
	}

	if (irq_src_h & MFC_INTA_H_TX_OCP_MASK)
		pr_info("%s: TX Over Current IRQ !\n", __func__);

INT_END:
	ret = mfc_reg_write(charger->client, MFC_INT_A_CLEAR_L_REG, irq_src_l); // clear int
	ret = mfc_reg_write(charger->client, MFC_INT_A_CLEAR_H_REG, irq_src_h); // clear int
	ret = mfc_reg_write(charger->client, MFC_INT_B_CLEAR_L_REG, irq_src1_l); // clear int
	ret = mfc_reg_write(charger->client, MFC_INT_B_CLEAR_H_REG, irq_src1_h); // clear int
	mfc_set_cmd_l_reg(charger, MFC_CMD_CLEAR_INT_MASK, MFC_CMD_CLEAR_INT_MASK); // command
	ret = mfc_reg_read(charger->client, MFC_STATUS_L_REG, &status_l);
	ret = mfc_reg_read(charger->client, MFC_STATUS_H_REG, &status_h);
	ret = mfc_reg_read(charger->client, MFC_STATUS1_L_REG, &status1_l);
	ret = mfc_reg_read(charger->client, MFC_STATUS1_H_REG, &status1_h);
	pr_info("%s: status after clear irq, status(0x%x)\n", __func__,
		((status1_h << 24) | (status1_l << 16) | (status_h << 8) | status_l));

	/* tx off should work having done i2c */
	if (end_irq)
		psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR, value);

	pr_info("%s: end!\n", __func__);

	__pm_relax(charger->wpc_ws);

	return IRQ_HANDLED;
}

static void mfc_chg_parse_mpp_data(struct device_node *np, mfc_charger_platform_data_t *pdata)
{
	pr_info("@mpp: %s\n", __func__);

	if (!of_property_read_bool(np, "battery,support_mpp_data"))
		return;

	sb_of_parse_u32_dt(np, "battery,alpha0_rx", pdata, alpha0_rx, 0);
	sb_of_parse_u32_dt(np, "battery,alpha1_rx", pdata, alpha1_rx, 0);
	sb_of_parse_u32_dt(np, "battery,alpha_kth_rx", pdata, alpha_kth_rx, 0);
	sb_of_parse_u32_dt(np, "battery,rx_coil_rac", pdata, rx_coil_rac, 0);
	sb_of_parse_u32_dt(np, "battery,rx_cap_esr", pdata, rx_cap_esr, 0);
	sb_of_parse_u32_dt(np, "battery,m_coil", pdata, m_coil, 0);
	sb_of_parse_u32_dt(np, "battery,alpha_fm", pdata, alpha_fm, 0);
	sb_of_parse_u32_dt(np, "battery,alpha_fm_dc", pdata, alpha_fm_dc, 0);
	sb_of_parse_u32_dt(np, "battery,g_coil_tx", pdata, g_coil_tx, 0);
}

static void mfc_chg_parse_iec_data(struct device_node *np, mfc_charger_platform_data_t *pdata)
{
	int ret = 0;

	ret = of_property_read_u32(np, "battery,wireless20_iec_qfod_enable", &pdata->iec_qfod_enable);
	if (ret < 0) {
		pr_info("%s: fail to read iec_qfod_enable\n", __func__);
		pdata->iec_qfod_enable = 0x00;
	} else {
		pr_info("%s: wireless20_iec_qfod_enable = %d\n", __func__, pdata->iec_qfod_enable);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_q_thresh_1", &pdata->iec_q_thresh_1);
	if (ret < 0) {
		pr_info("%s: fail to read iec_q_thresh_1\n", __func__);
		pdata->iec_q_thresh_1 = 0x46;
	} else {
		pr_info("%s: wireless20_iec_q_thresh_1 = %d\n", __func__, pdata->iec_q_thresh_1);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_q_thresh_2", &pdata->iec_q_thresh_2);
	if (ret < 0) {
		pr_info("%s: fail to read iec_q_thresh_2\n", __func__);
		pdata->iec_q_thresh_2 = 0x3c;
	} else {
		pr_info("%s: wireless20_iec_q_thresh_2 = %d\n", __func__, pdata->iec_q_thresh_2);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_fres_thresh_1", &pdata->iec_fres_thresh_1);
	if (ret < 0) {
		pr_info("%s: fail to read iec_fres_thresh_1\n", __func__);
		pdata->iec_fres_thresh_1 = 0x47;
	} else {
		pr_info("%s: wireless20_iec_fres_thresh_1 = %d\n", __func__, pdata->iec_fres_thresh_1);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_fres_thresh_2", &pdata->iec_fres_thresh_2);
	if (ret < 0) {
		pr_info("%s: fail to read iec_fres_thresh_2\n", __func__);
		pdata->iec_fres_thresh_2 = 0x55;
	} else {
		pr_info("%s: wireless20_iec_fres_thresh_2 = %d\n", __func__, pdata->iec_fres_thresh_2);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_power_limit_thresh", &pdata->iec_power_limit_thresh);
	if (ret < 0) {
		pr_info("%s: fail to read iec_power_limit_thresh\n", __func__);
		pdata->iec_power_limit_thresh = 0x190;
	} else {
		pr_info("%s: wireless20_iec_power_limit_thresh = %d\n", __func__, pdata->iec_power_limit_thresh);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_ploss_thresh_1", &pdata->iec_ploss_thresh_1);
	if (ret < 0) {
		pr_info("%s: fail to read iec_ploss_thresh_1\n", __func__);
		pdata->iec_ploss_thresh_1 = 0x384;
	} else {
		pr_info("%s: wireless20_iec_ploss_thresh_1 = %d\n", __func__, pdata->iec_ploss_thresh_1);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_ploss_thresh_2", &pdata->iec_ploss_thresh_2);
	if (ret < 0) {
		pr_info("%s: fail to read iec_ploss_thresh_2\n", __func__);
		pdata->iec_ploss_thresh_2 = 0x384;
	} else {
		pr_info("%s: wireless20_iec_ploss_thresh_2 = %d\n", __func__, pdata->iec_ploss_thresh_2);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_ploss_freq_thresh_1", &pdata->iec_ploss_freq_thresh_1);
	if (ret < 0) {
		pr_info("%s: fail to read iec_ploss_freq_thresh_1\n", __func__);
		pdata->iec_ploss_freq_thresh_1 = 0xff;
	} else {
		pr_info("%s: wireless20_iec_ploss_freq_thresh_1 = %d\n", __func__, pdata->iec_ploss_freq_thresh_1);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_ploss_freq_thresh_2", &pdata->iec_ploss_freq_thresh_2);
	if (ret < 0) {
		pr_info("%s: fail to read iec_ploss_freq_thresh_2\n", __func__);
		pdata->iec_ploss_freq_thresh_2 = 0xff;
	} else {
		pr_info("%s: wireless20_iec_ploss_freq_thresh_2 = %d\n", __func__, pdata->iec_ploss_freq_thresh_2);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_ta_power_limit_thresh", &pdata->iec_ta_power_limit_thresh);
	if (ret < 0) {
		pr_info("%s: fail to read iec_ta_power_limit_thresh\n", __func__);
		pdata->iec_ta_power_limit_thresh = 0x258;
	} else {
		pr_info("%s: wireless20_iec_ta_power_limit_thresh = %d\n", __func__, pdata->iec_ta_power_limit_thresh);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_ta_ploss_thresh_1", &pdata->iec_ta_ploss_thresh_1);
	if (ret < 0) {
		pr_info("%s: fail to read iec_ta_ploss_thresh_1\n", __func__);
		pdata->iec_ta_ploss_thresh_1 = 0x4b0;
	} else {
		pr_info("%s: wireless20_iec_ta_ploss_thresh_1 = %d\n", __func__, pdata->iec_ta_ploss_thresh_1);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_ta_ploss_thresh_2", &pdata->iec_ta_ploss_thresh_2);
	if (ret < 0) {
		pr_info("%s: fail to read iec_ta_ploss_thresh_2\n", __func__);
		pdata->iec_ta_ploss_thresh_2 = 0x4b0;
	} else {
		pr_info("%s: wireless20_iec_ta_ploss_thresh_2 = %d\n", __func__, pdata->iec_ta_ploss_thresh_2);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_ta_ploss_freq_thresh_1", &pdata->iec_ta_ploss_freq_thresh_1);
	if (ret < 0) {
		pr_info("%s: fail to read iec_ta_ploss_freq_thresh_1\n", __func__);
		pdata->iec_ta_ploss_freq_thresh_1 = 0xff;
	} else {
		pr_info("%s: wireless20_iec_ta_ploss_freq_thresh_1 = %d\n", __func__, pdata->iec_ta_ploss_freq_thresh_1);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_ta_ploss_freq_thresh_2", &pdata->iec_ta_ploss_freq_thresh_2);
	if (ret < 0) {
		pr_info("%s: fail to read iec_ta_ploss_freq_thresh_2\n", __func__);
		pdata->iec_ta_ploss_freq_thresh_2 = 0xff;
	} else {
		pr_info("%s: wireless20_iec_ta_ploss_freq_thresh_2 = %d\n", __func__, pdata->iec_ta_ploss_freq_thresh_2);
	}

	ret = of_property_read_u32(np, "battery,wireless20_iec_ploss_fod_enable", &pdata->iec_ploss_fod_enable);
	if (ret < 0) {
		pr_info("%s: fail to read iec_ploss_fod_enable\n", __func__);
		pdata->iec_ploss_fod_enable = 0x0;
	} else {
		pr_info("%s: wireless20_iec_ploss_fod_enable = %d\n", __func__, pdata->iec_ploss_fod_enable);
	}
}

#if defined(CONFIG_WIRELESS_IC_PARAM)
static void mfc_parse_param_value(struct mfc_charger_data *charger)
{
	unsigned int param_value = mfc_get_wrlic();

	charger->wireless_param_info = param_value;
	charger->wireless_chip_id_param = (param_value & 0xFF000000) >> 24;
	charger->wireless_fw_ver_param = (param_value & 0x00FFFF00) >> 8;
	charger->wireless_fw_mode_param = (param_value & 0x000000F0) >> 4;

	pr_info("%s: wireless_ic(0x%08X), chip_id(0x%02X), fw_ver(0x%04X), fw_mode(0x%01X)\n",
		__func__, param_value, charger->wireless_chip_id_param,
		charger->wireless_fw_ver_param, charger->wireless_fw_mode_param);
}
#endif

static int mfc_chg_parse_dt(struct device *dev, mfc_charger_platform_data_t *pdata)
{
	int ret = 0;
	struct device_node *np = dev->of_node;
	struct device_node *np_battery;
	int len, i;
	const u32 *p;

	if (!np) {
		pr_err("%s: np NULL\n", __func__);
		return 1;
	}
	ret = of_property_read_string(np,
		"battery,wireless_charger_name", (char const **)&pdata->wireless_charger_name);
	if (ret < 0)
		pr_info("%s: Wireless Charger name is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,charger_name", (char const **)&pdata->wired_charger_name);
	if (ret < 0)
		pr_info("%s: Charger name is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,fuelgauge_name", (char const **)&pdata->fuelgauge_name);
	if (ret < 0)
		pr_info("%s: Fuelgauge name is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,phone_fod_thresh1",
					&pdata->phone_fod_thresh1);
	if (ret < 0) {
		pr_info("%s: fail to read phone_fod_thresh1\n", __func__);
		pdata->phone_fod_thresh1 = 0x07D0; /* default 2000mW */
	}

	ret = of_property_read_u32(np, "battery,phone_fod_ta_thresh",
					&pdata->phone_fod_ta_thresh);
	if (ret < 0) {
		pr_info("%s: fail to read phone_fod_ta_thresh\n", __func__);
		pdata->phone_fod_ta_thresh = 0x0320; /* default 800mW */
	}

	ret = of_property_read_u32(np, "battery,tx_fod_gain",
					&pdata->tx_fod_gain);
	if (ret < 0) {
		pr_info("%s: fail to read tx_fod_gain\n", __func__);
		pdata->tx_fod_gain = 0x72;
	}

	ret = of_property_read_u32(np, "battery,tx_fod_offset",
					&pdata->tx_fod_offset);
	if (ret < 0) {
		pr_info("%s: fail to read tx_fod_offset\n", __func__);
		pdata->tx_fod_offset = 0xFF9C;
	}

	ret = of_property_read_u32(np, "battery,buds_fod_thresh1",
					&pdata->buds_fod_thresh1);
	if (ret < 0) {
		pr_info("%s: fail to read buds_fod_thresh1\n", __func__);
		pdata->buds_fod_thresh1 = 0x07D0; /* default 2000mW */
	}

	ret = of_property_read_u32(np, "battery,buds_fod_ta_thresh",
					&pdata->buds_fod_ta_thresh);
	if (ret < 0) {
		pr_info("%s: fail to read buds_fod_ta_thresh\n", __func__);
		pdata->buds_fod_ta_thresh = 0x0320;  /* default 800mW */
	}

	ret = of_property_read_u32(np, "battery,tx_max_op_freq",
					&pdata->tx_max_op_freq);
	if (ret < 0)
		pr_info("%s: fail to read tx_max_op_freq\n", __func__);

	ret = of_property_read_u32(np, "battery,tx_min_op_freq",
					&pdata->tx_min_op_freq);
	if (ret < 0)
		pr_info("%s: fail to read tx_min_op_freq\n", __func__);

	if (carrierid_is("XAC")) {
		pr_info("%s: use cep_timeout_xac\n", __func__);
		ret = of_property_read_u32(np, "battery,cep_timeout_xac",
						&pdata->cep_timeout);
	} else {
		ret = of_property_read_u32(np, "battery,cep_timeout",
							&pdata->cep_timeout);
	}

	if (ret < 0) {
		pr_info("%s: fail to read cep_timeout\n", __func__);
		pdata->cep_timeout = 0;  /* default 0x11 1.7sec */
	}

	ret = of_property_read_u32(np, "battery,tx_conflict_curr",
					&pdata->tx_conflict_curr);
	if (ret < 0) {
		pr_info("%s: fail to read tx_conflict_curr\n", __func__);
		pdata->tx_conflict_curr = 1200;
	}

	ret = of_property_read_u32(np, "battery,gear_op_freq",
					&pdata->gear_op_freq);
	if (ret < 0) {
		pr_info("%s: fail to read gear_op_freq\n", __func__);
		pdata->gear_op_freq = 1450; // 145kHZ
	}

	ret = of_property_read_u32(np, "battery,gear_min_op_freq",
					&pdata->gear_min_op_freq);
	if (ret < 0) {
		pr_info("%s: fail to read gear_min_op_freq\n", __func__);
		pdata->gear_min_op_freq = 1250; // 125kHZ
	}

	ret = of_property_read_u32(np, "battery,tx_gear_min_op_freq_delay",
					&pdata->gear_min_op_freq_delay);
	if (ret < 0) {
		pr_info("%s: fail to read gear_min_op_freq_delay\n", __func__);
		pdata->gear_min_op_freq_delay = 0;
	}

	/* wpc_det */
	ret = pdata->wpc_det = sb_of_get_named_gpio_flags(np, "battery,wpc_det");
	if (ret < 0) {
		dev_err(dev, "%s: can't get wpc_det\r\n", __func__);
	} else {
		pdata->irq_wpc_det = gpio_to_irq(pdata->wpc_det);
		pr_info("%s: wpc_det = 0x%x, irq_wpc_det = 0x%x\n",
			__func__, pdata->wpc_det, pdata->irq_wpc_det);
	}
	/* wpc_int (This GPIO means MFC_AP_INT) */
	ret = pdata->wpc_int = sb_of_get_named_gpio_flags(np, "battery,wpc_int");
	if (ret < 0) {
		dev_err(dev, "%s: can't wpc_int\r\n", __func__);
	} else {
		pdata->irq_wpc_int = gpio_to_irq(pdata->wpc_int);
		pr_info("%s: wpc_int = 0x%x, irq_wpc_int = 0x%x\n",
			__func__, pdata->wpc_int, pdata->irq_wpc_int);
	}

	/* mst_pwr_en (MST PWR EN) */
	ret = pdata->mst_pwr_en = sb_of_get_named_gpio_flags(np, "battery,mst_pwr_en");
	if (ret < 0)
		dev_err(dev, "%s: can't mst_pwr_en\r\n", __func__);

	/* wpc_pdrc (This GPIO means VRECT_INT) */
	ret = pdata->wpc_pdrc = sb_of_get_named_gpio_flags(np, "battery,wpc_pdrc");
	if (ret < 0) {
		dev_err(dev, "%s : can't wpc_pdrc\r\n", __func__);
	} else {
		pdata->irq_wpc_pdrc = gpio_to_irq(pdata->wpc_pdrc);
		pr_info("%s wpc_pdrc = 0x%x, irq_wpc_pdrc = 0x%x\n",
			__func__, pdata->wpc_pdrc, pdata->irq_wpc_pdrc);
	}

	/* wpc_pdet_b (PDET_B) */
	ret = pdata->wpc_pdet_b = sb_of_get_named_gpio_flags(np, "battery,wpc_pdet_b");
	if (ret < 0) {
		dev_err(dev, "%s : can't wpc_pdet_b\r\n", __func__);
	} else {
		pdata->irq_wpc_pdet_b = gpio_to_irq(pdata->wpc_pdet_b);
		pr_info("%s wpc_pdet_b = 0x%x, irq_wpc_pdet_b = 0x%x\n",
			__func__, pdata->wpc_pdet_b, pdata->irq_wpc_pdet_b);
	}

	/* wpc_en (MFC EN) */
	ret = pdata->wpc_en = sb_of_get_named_gpio_flags(np, "battery,wpc_en");
	if (ret < 0)
		dev_err(dev, "%s: can't wpc_en\r\n", __func__);

	/* coil_sw_en (COIL SW EN N) */
	ret = pdata->coil_sw_en = sb_of_get_named_gpio_flags(np, "battery,coil_sw_en");
	if (ret < 0)
		dev_err(dev, "%s: can't coil_sw_en\r\n", __func__);

	/* ping_nen (PING nEN) */
	ret = pdata->ping_nen = sb_of_get_named_gpio_flags(np, "battery,wpc_ping_nen");
	if (ret < 0)
		dev_err(dev, "%s : can't wpc_ping_nen\r\n", __func__);

	ret = pdata->mag_det = sb_of_get_named_gpio_flags(np, "battery,wpc_mag_det");
	if (ret < 0)
		dev_err(dev, "%s: can't wpc_mag_det\r\n", __func__);

	ret = pdata->wpc_mode = sb_of_get_named_gpio_flags(np, "battery,wpc_mode");
	if (ret < 0)
		dev_err(dev, "%s: can't wpc_mode\r\n", __func__);

	ret = pdata->mpp_sw = sb_of_get_named_gpio_flags(np, "battery,wpc_mpp_sw");
	if (ret < 0)
		dev_err(dev, "%s: can't wpc_mpp_sw\r\n", __func__);

	p = of_get_property(np, "battery,wireless20_vout_list", &len);
	if (p) {
		len = len / sizeof(u32);
		pdata->len_wc20_list = len;
		pdata->wireless20_vout_list = kcalloc(len, sizeof(*pdata->wireless20_vout_list), GFP_KERNEL);
		ret = of_property_read_u32_array(np, "battery,wireless20_vout_list",
			pdata->wireless20_vout_list, len);

		for (i = 0; i < len; i++)
			pr_info("%s: wireless20_vout_list = %d ", __func__, pdata->wireless20_vout_list[i]);
		pr_info("%s: len_wc20_list = %d ", __func__, pdata->len_wc20_list);
	} else {
		pr_err("%s: there is no wireless20_vout_list\n", __func__);
	}

	p = of_get_property(np, "battery,wireless20_vrect_list", &len);
	if (p) {
		len = len / sizeof(u32);
		pdata->wireless20_vrect_list =
			kcalloc(len, sizeof(*pdata->wireless20_vrect_list), GFP_KERNEL);
		ret = of_property_read_u32_array(np, "battery,wireless20_vrect_list",
			pdata->wireless20_vrect_list, len);

		for (i = 0; i < len; i++)
			pr_info("%s: wireless20_vrect_list = %d ", __func__, pdata->wireless20_vrect_list[i]);
	} else {
		pr_err("%s: there is no wireless20_vrect_list\n", __func__);
	}

	p = of_get_property(np, "battery,wireless20_max_power_list", &len);
	if (p) {
		len = len / sizeof(u32);
		pdata->wireless20_max_power_list =
			kcalloc(len, sizeof(*pdata->wireless20_max_power_list), GFP_KERNEL);
		ret = of_property_read_u32_array(np, "battery,wireless20_max_power_list",
				pdata->wireless20_max_power_list, len);

		for (i = 0; i < len; i++)
			pr_info("%s: wireless20_max_power_list = %d\n",
				__func__, pdata->wireless20_max_power_list[i]);
	} else {
		pr_err("%s: there is no wireless20_max_power_list\n", __func__);
	}
	pdata->no_hv =
	    of_property_read_bool(np, "battery,wireless_no_hv");

	ret = of_property_read_u32(np, "battery,wpc_vout_ctrl_full",
			&pdata->wpc_vout_ctrl_full);
	if (ret)
		pr_err("%s: wpc_vout_ctrl_full is Empty\n", __func__);

	if (pdata->wpc_vout_ctrl_full)
		pdata->wpc_headroom_ctrl_full = of_property_read_bool(np, "battery,wpc_headroom_ctrl_full");

	pdata->mis_align_guide = of_property_read_bool(np, "battery,mis_align_guide");
	if (pdata->mis_align_guide) {
		ret = of_property_read_u32(np, "battery,mis_align_target_vout",
						&pdata->mis_align_target_vout);
		if (ret)
			pdata->mis_align_target_vout = 5000;

		ret = of_property_read_u32(np, "battery,mis_align_offset",
					&pdata->mis_align_offset);
		if (ret)
			pdata->mis_align_offset = 200;
		pr_info("%s: mis_align_guide, vout:%d, offset:%d\n", __func__,
			pdata->mis_align_target_vout, pdata->mis_align_offset);
	}
	pdata->unknown_cmb_ctrl = of_property_read_bool(np, "battery,unknown_cmb_ctrl");
	pdata->default_clamp_volt = of_property_read_bool(np, "battery,default_clamp_volt");
	if (pdata->default_clamp_volt)
		pr_info("%s: default_clamp_volt(%d)\n", __func__, pdata->default_clamp_volt);

#if defined(CONFIG_MST_PCR)
	ret = of_property_read_u32(np, "battery,mst_iset_pcr",
					&pdata->mst_iset_pcr);
	if (ret < 0) {
		pr_info("%s: fail to read mst_iset_pcr. default set 2.8A(0x06)\n", __func__);
		pdata->mst_iset_pcr = 0x06;  /* 2.8A */
	}
	use_pcr_fix_mode = of_property_read_bool(np, "battery,pcr_fix_mode");
	if (use_pcr_fix_mode) {
		pr_info("%s: Using PCR_FIX_MODE for MST\n", __func__);
	}
#endif

	ret = of_property_read_u32(np, "battery,epp_vout",
				&pdata->epp_vout);
	if (ret) {
		pr_err("%s: epp_vout is Empty\n", __func__);
		pdata->epp_vout = WIRELESS_VOUT_12V;
	}

	ret = of_property_read_u32(np, "battery,mpp_vout",
				&pdata->mpp_vout);
	if (ret) {
		pr_err("%s: mpp_vout is Empty\n", __func__);
		pdata->mpp_vout = WIRELESS_VOUT_13V;
	}

	ret = of_property_read_u32(np, "battery,mpp_epp_def_power",
				&pdata->mpp_epp_def_power);
	if (ret) {
		pr_err("%s: mpp_epp_def_power is Empty\n", __func__);
		pdata->mpp_epp_def_power = TX_RX_POWER_5W * 100000;
	}

	ret = of_property_read_u32(np, "battery,mpp_epp_max_count",
				&pdata->mpp_epp_max_count);
	if (ret) {
		pr_err("%s: mpp_epp_max_count is Empty\n", __func__);
		pdata->mpp_epp_max_count = 3;
	}

	ret = of_property_read_u32(np, "battery,epp_time_init",
				&pdata->epp_time_init);
	if (ret) {
		pr_err("%s: epp_time_init is Empty\n", __func__);
		pdata->epp_time_init = 1000;
	}

	ret = of_property_read_u32(np, "battery,epp_time_mid_1",
				&pdata->epp_time_mid_1);
	if (ret) {
		pr_err("%s: epp_time_mid_1 is Empty\n", __func__);
		pdata->epp_time_mid_1 = 1000;
	}

	ret = of_property_read_u32(np, "battery,epp_time_mid_2",
				&pdata->epp_time_mid_2);
	if (ret) {
		pr_err("%s: epp_time_mid_2 is Empty\n", __func__);
		pdata->epp_time_mid_2 = pdata->epp_time_mid_1;
	}

	ret = of_property_read_u32(np, "battery,epp_time_final",
				&pdata->epp_time_final);
	if (ret) {
		pr_err("%s: epp_time_final is Empty\n", __func__);
		pdata->epp_time_final = 4000;
	}

	ret = of_property_read_u32(np, "battery,fod_cnt_thresh", &pdata->fod_cnt_thresh);
	if (ret) {
		pr_err("%s: fod_cnt_thresh is Empty\n", __func__);
		pdata->fod_cnt_thresh = 5;
	}

	ret = of_property_read_u32(np, "battery,mpp_epp_power_level",
				&pdata->mpp_epp_power_level);
	if (ret) {
		pr_err("%s: mpp_epp_power_level is Empty\n", __func__);
		pdata->mpp_epp_power_level = MFC_RX_MPP_NEGO_POWER_15W;
	}

#if defined(CONFIG_SEC_FACTORY)
	pdata->mpp_disable = false;
#else
	pdata->mpp_disable = of_property_read_bool(np, "battery,mpp_disable");
#endif

	mfc_chg_parse_iec_data(np, pdata);
	mfc_chg_parse_mpp_data(np, pdata);
	np = dev->of_node;

	np_battery = of_find_node_by_name(NULL, "battery");
	if (!np_battery) {
		dev_err(dev, "%s: empty of_node for battery\n", __func__);
		return -EINVAL;
	}
	ret = of_property_read_u32(np_battery, "battery,wpc_temp_v2_cond",
			&pdata->high_swell_recov);
	if (ret) {
		dev_info(dev, "%s: battery,wpc_temp_v2_cond is Empty\n", __func__);
		pdata->high_swell_recov = 390;
	}
	pr_info("%s: high_swell_recov(%d)\n", __func__, pdata->high_swell_recov);

	pdata->hall_ic_notifier = of_property_read_bool(np_battery,
						     "battery,hall_ic_notifier");
	pr_info("%s: support hall_ic_notifier(%d)\n", __func__, pdata->hall_ic_notifier);
	return 0;
}

static int mfc_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < (int)ARRAY_SIZE(mfc_attrs); i++) {
		rc = device_create_file(dev, &mfc_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	return rc;

create_attrs_failed:
	dev_err(dev, "%s: failed (%d)\n", __func__, rc);
	while (i--)
		device_remove_file(dev, &mfc_attrs[i]);
	return rc;
}

ssize_t cps4038_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mfc_charger_data *charger = power_supply_get_drvdata(psy);

	const ptrdiff_t offset = attr - mfc_attrs;
	int i = 0;

	dev_info(charger->dev, "%s\n", __func__);

	switch (offset) {
	case MFC_ADDR:
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%x\n", charger->addr);
		break;
	case MFC_SIZE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%x\n", charger->size);
		break;
	case MFC_DATA:
		if (charger->size == 0) {
			charger->size = 1;
		} else if (charger->size + charger->addr <= 0xFFFF) {
			u8 data;
			int j;

			for (j = 0; j < charger->size; j++) {
				if (mfc_reg_read(charger->client, charger->addr + j, &data) < 0) {
					dev_info(charger->dev, "%s: read fail\n", __func__);
					i += scnprintf(buf + i, PAGE_SIZE - i, "addr: 0x%x read fail\n",
						charger->addr + j);
					continue;
				}
				i += scnprintf(buf + i, PAGE_SIZE - i, "addr: 0x%x, data: 0x%x\n",
					charger->addr + j, data);
			}
		}
		break;
	case MFC_PACKET:
		break;
	case MFC_EPP_T:
		i += scnprintf(buf + i, PAGE_SIZE - i, "epp_time: i:%d, m1:%d, m2:%d, f:%d\n",
			charger->pdata->epp_time_init, charger->pdata->epp_time_mid_1,
			charger->pdata->epp_time_mid_2, charger->pdata->epp_time_final);
		dev_info(charger->dev, "%s epp_time: i:%d, m1:%d, m2:%d, f:%d\n", __func__,
				charger->pdata->epp_time_init, charger->pdata->epp_time_mid_1,
				charger->pdata->epp_time_mid_2, charger->pdata->epp_time_final);
		break;
	default:
		return -EINVAL;
	}
	return i;
}

ssize_t cps4038_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mfc_charger_data *charger = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - mfc_attrs;
	int x, ret;
	u8 header, data_com, data_val;

	dev_info(charger->dev, "%s\n", __func__);

	switch (offset) {
	case MFC_ADDR:
		if (sscanf(buf, "0x%4x\n", &x) == 1)
			charger->addr = x;
		ret = count;
		break;
	case MFC_SIZE:
		if (sscanf(buf, "%5d\n", &x) == 1)
			charger->size = x;
		ret = count;
		break;
	case MFC_DATA:
		if (sscanf(buf, "0x%10x", &x) == 1) {
			u8 data = x;

			if (mfc_reg_write(charger->client, charger->addr, data) < 0)
				dev_info(charger->dev, "%s: addr: 0x%x write fail\n", __func__, charger->addr);
		}
		ret = count;
		break;
	case MFC_PACKET:
	{
		u32 header_temp, data_com_temp, data_val_temp;

		if (sscanf(buf, "0x%4x 0x%4x 0x%4x\n", &header_temp, &data_com_temp, &data_val_temp) == 3) {
			header = (u8)header_temp;
			data_com = (u8)data_com_temp;
			data_val = (u8)data_val_temp;
			dev_info(charger->dev, "%s 0x%x, 0x%x, 0x%x\n", __func__, header, data_com, data_val);
			mfc_send_packet(charger, header, data_com, &data_val, 1);
		}
		ret = count;
	}
		break;
	case MFC_EPP_T:
	{
		int init_epp_time, mid_epp_time_1, mid_epp_time_2, final_epp_time;

		if (sscanf(buf,
			"%10d %10d %10d %10d\n",
			&init_epp_time, &mid_epp_time_1, &mid_epp_time_2, &final_epp_time) > 0) {
			charger->pdata->epp_time_init = init_epp_time;
			charger->pdata->epp_time_mid_1 = mid_epp_time_1;
			charger->pdata->epp_time_mid_2 = mid_epp_time_2;
			charger->pdata->epp_time_final = final_epp_time;
			dev_info(charger->dev, "%s epp_time: i:%d, m1:%d, m2:%d, f:%d\n", __func__,
				charger->pdata->epp_time_init, charger->pdata->epp_time_mid_1,
				charger->pdata->epp_time_mid_2, charger->pdata->epp_time_final);
		}
		ret = count;
	}
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static const struct power_supply_desc mfc_charger_power_supply_desc = {
	.name = "mfc-charger",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.properties = mfc_charger_props,
	.num_properties = ARRAY_SIZE(mfc_charger_props),
	.get_property = cps4038_chg_get_property,
	.set_property = cps4038_chg_set_property,
};

static void mfc_wpc_int_req_work(struct work_struct *work)
{
	struct mfc_charger_data *charger =
		container_of(work, struct mfc_charger_data, wpc_int_req_work.work);

	int ret = 0;

	pr_info("%s\n", __func__);
	/* wpc_irq */
	if (charger->pdata->irq_wpc_int) {
		msleep(100);
		ret = request_threaded_irq(charger->pdata->irq_wpc_int,
				mfc_wpc_irq_handler, mfc_wpc_irq_thread,
				IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				"wpc-irq", charger);
		if (ret)
			pr_err("%s: Failed to Request IRQ\n", __func__);
	}
	if (ret < 0)
		free_irq(charger->pdata->irq_wpc_det, NULL);
}

static enum alarmtimer_restart mfc_phm_alarm(
	struct alarm *alarm, ktime_t now)
{
	struct mfc_charger_data *charger = container_of(alarm,
				struct mfc_charger_data, phm_alarm);

	pr_info("%s: forced escape to PHM\n", __func__);
	__pm_stay_awake(charger->wpc_tx_phm_ws);
	if (charger->is_suspend)
		charger->skip_phm_work_in_sleep = true;
	else
		queue_delayed_work(charger->wqueue, &charger->wpc_tx_phm_work, 0);

	return ALARMTIMER_NORESTART;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0))
static int cps4038_charger_probe(struct i2c_client *client)
#else
static int cps4038_charger_probe(struct i2c_client *client, const struct i2c_device_id *id)
#endif
{
	struct device_node *of_node = client->dev.of_node;
	struct mfc_charger_data *charger;
	mfc_charger_platform_data_t *pdata = client->dev.platform_data;
	struct power_supply_config mfc_cfg = {};
	int ret = 0;
	u8 int_state;

	dev_info(&client->dev,
		"%s: MFC cps4038 Charger Driver Loading\n", __func__);

	if (of_node) {
		pdata = devm_kzalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata)
			return -ENOMEM;

		ret = mfc_chg_parse_dt(&client->dev, pdata);
		if (ret < 0)
			goto err_parse_dt;
	} else {
		pdata = client->dev.platform_data;
		return -ENOMEM;
	}

	charger = kcalloc(1, sizeof(*charger), GFP_KERNEL);
	if (charger == NULL) {
		ret = -ENOMEM;
		goto err_wpc_nomem;
	}
	charger->dev = &client->dev;

	ret = i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA |
		I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_I2C_BLOCK);
	if (!ret) {
		ret = i2c_get_functionality(client->adapter);
		dev_err(charger->dev, "I2C functionality is not supported.\n");
		ret = -ENODEV;
		goto err_i2cfunc_not_support;
	}

	is_shutdn = false;
	charger->client = client;
	charger->pdata = pdata;

	pr_info("%s: %s\n", __func__, charger->pdata->wireless_charger_name);

	i2c_set_clientdata(client, charger);
#if defined(CONFIG_WIRELESS_IC_PARAM)
	mfc_parse_param_value(charger);
#endif

	charger->fod = mfc_fod_init(charger->dev, MFC_NUM_FOD_REG, cps4038_set_fod);
	if (IS_ERR(charger->fod))
		pr_err("%s: failed to init fod (ret = %ld)\n", __func__, PTR_ERR(charger->fod));
	charger->cmfet = mfc_cmfet_init(charger->dev, cps4038_set_cmfet);
	if (IS_ERR(charger->cmfet))
		pr_err("%s: failed to init cmfet (ret = %ld)\n", __func__, PTR_ERR(charger->cmfet));

	charger->pdata->cable_type = SEC_BATTERY_CABLE_NONE;
	charger->pdata->is_charging = 0;
	charger->tx_status = 0;
	charger->pdata->cs100_status = 0;
	charger->pdata->vout_status = MFC_VOUT_5V;
	charger->pdata->opfq_cnt = 0;
	charger->flicker_delay = 1000;
	charger->flicker_vout_threshold = MFC_VOUT_8V;
	charger->is_mst_on = MST_MODE_0;
	charger->chip_id = MFC_CHIP_CPS;
	charger->is_otg_on = false;
	charger->led_cover = 0;
	charger->vout_mode = WIRELESS_VOUT_OFF;
	charger->is_full_status = 0;
	charger->is_afc_tx = false;
	charger->pad_ctrl_by_lcd = false;
	charger->wc_tx_enable = false;
	charger->initial_wc_check = false;
	charger->wc_rx_connected = false;
	charger->wc_rx_fod = false;
	charger->adt_transfer_status = WIRELESS_AUTH_WAIT;
	charger->current_rx_power = TX_RX_POWER_0W;
	charger->tx_id = TX_ID_UNKNOWN;
	charger->tx_id_cnt = 0;
	charger->wc_ldo_status = MFC_LDO_ON;
	charger->tx_id_done = false;
	charger->wc_rx_type = NO_DEV;
	charger->is_suspend = false;
	charger->device_event = 0;
	charger->duty_min = MIN_DUTY_SETTING_20_DATA;
	charger->wpc_en_flag = (WPC_EN_SYSFS | WPC_EN_CHARGING | WPC_EN_CCIC | WPC_EN_SLATE);
	charger->req_tx_id = false;
	charger->afc_tx_done = false;
	charger->sleep_mode = false;
	charger->req_afc_delay = REQ_AFC_DLY;
	charger->mis_align_tx_try_cnt = 1;
	charger->wc_checking_align = false;
	charger->wc_align_check_start.tv_sec = 0;
	charger->vout_strength = 100;
	charger->reg_access_lock = false;
	charger->det_state = 0;
	charger->pdrc_state = 1;
	charger->fw_cmd = SEC_WIRELESS_FW_UPDATE_AUTO_MODE;
	charger->mpp_epp_tx_id = 0;
	charger->mpp_epp_nego_done_power = 0;
	charger->mpp_epp_tx_potential_load_power = 0;
	charger->mpp_epp_tx_negotiable_load_power = 0;
	charger->mpp_cloak = 0;
	charger->magnet_case_det = 0;
	charger->mpp_case = 0;
	charger->target_current = 0;
	charger->thermal_ctrl = 0;
	charger->high_swell = false;
	charger->init_cover = true;
	charger->fod_cnt = 0;
	charger->rx_phm_check_cnt = 0;

	mutex_init(&charger->io_lock);
	mutex_init(&charger->wpc_en_lock);
	mutex_init(&charger->fw_lock);

	charger->wqueue = create_singlethread_workqueue("mfc_workqueue");
	if (!charger->wqueue) {
		pr_err("%s: Fail to Create Workqueue\n", __func__);
		goto err_pdata_free;
	}

	charger->wpc_ws = wakeup_source_register(&client->dev, "wpc_ws");
	charger->wpc_det_ws = wakeup_source_register(&client->dev, "wpc_det_ws");
	charger->wpc_rx_ws = wakeup_source_register(&client->dev, "wpc_rx_ws");
	charger->wpc_tx_ws = wakeup_source_register(&client->dev, "wpc_tx_ws");
	charger->wpc_update_ws = wakeup_source_register(&client->dev, "wpc_update_ws");
	charger->wpc_tx_min_opfq_ws = wakeup_source_register(&client->dev, "wpc_tx_min_opfq_ws");
	charger->wpc_tx_duty_min_ws = wakeup_source_register(&client->dev, "wpc_tx_duty_min_ws");
	charger->wpc_afc_vout_ws = wakeup_source_register(&client->dev, "wpc_afc_vout_ws");
	charger->wpc_vout_mode_ws = wakeup_source_register(&client->dev, "wpc_vout_mode_ws");
	charger->wpc_rx_det_ws = wakeup_source_register(&client->dev, "wpc_rx_det_ws");
	charger->wpc_tx_phm_ws = wakeup_source_register(&client->dev, "wpc_tx_phm_ws");
	charger->wpc_tx_id_ws = wakeup_source_register(&client->dev, "wpc_tx_id_ws");
	charger->wpc_tx_pwr_budg_ws = wakeup_source_register(&client->dev, "wpc_tx_pwr_budg_ws");
	charger->wpc_pdrc_ws = wakeup_source_register(&client->dev, "wpc_pdrc_ws");
	charger->align_check_ws = wakeup_source_register(&client->dev, "align_check_ws");
	charger->mode_change_ws = wakeup_source_register(&client->dev, "mode_change_ws");
	charger->wpc_cs100_ws = wakeup_source_register(&client->dev, "wpc_cs100_ws");
	charger->wpc_rx_power_trans_fail_ws = wakeup_source_register(&client->dev, "wpc_rx_power_trans_fail_ws");
	charger->wpc_pdet_b_ws = wakeup_source_register(&client->dev, "wpc_pdet_b_ws");
	charger->wpc_rx_phm_ws = wakeup_source_register(&client->dev, "wpc_rx_phm_ws");
	charger->wpc_phm_exit_ws = wakeup_source_register(&client->dev, "wpc_phm_exit_ws");
	charger->epp_clear_ws = wakeup_source_register(&client->dev, "epp_clear_ws");
	charger->epp_count_ws = wakeup_source_register(&client->dev, "epp_count_ws");
	charger->check_mpp_ws = wakeup_source_register(&client->dev, "check_mpp_ws");
	charger->epp_mode_change_check_ws = wakeup_source_register(&client->dev, "epp_mode_change_check_ws");
	charger->mpp_fake_ui_clear_ws = wakeup_source_register(&client->dev, "mpp_fake_ui_cleark_ws");
	charger->set_mpp_cover_ws = wakeup_source_register(&client->dev, "set_mpp_cover_ws");
	charger->set_mpp_cloak_ws = wakeup_source_register(&client->dev, "set_mpp_cloak_ws");
	charger->mpla_thr_recov_ws = wakeup_source_register(&client->dev, "mpla_thr_recov_ws");

	/* wpc_det */
	if (charger->pdata->irq_wpc_det) {
		INIT_DELAYED_WORK(&charger->wpc_det_work, mfc_wpc_det_work);
	}

	/* wpc_irq (INT_A) */
	if (charger->pdata->irq_wpc_int) {
		INIT_DELAYED_WORK(&charger->wpc_isr_work, mfc_wpc_isr_work);
		INIT_DELAYED_WORK(&charger->wpc_tx_isr_work, mfc_wpc_tx_isr_work);
		INIT_DELAYED_WORK(&charger->wpc_tx_id_work, mfc_wpc_tx_id_work);
		INIT_DELAYED_WORK(&charger->wpc_tx_pwr_budg_work, mfc_wpc_tx_pwr_budg_work);
		INIT_DELAYED_WORK(&charger->wpc_int_req_work, mfc_wpc_int_req_work);
	}
	INIT_DELAYED_WORK(&charger->wpc_vout_mode_work, mfc_wpc_vout_mode_work);
	INIT_DELAYED_WORK(&charger->wpc_afc_vout_work, mfc_wpc_afc_vout_work);
	INIT_DELAYED_WORK(&charger->wpc_fw_update_work, mfc_wpc_fw_update_work);
	INIT_DELAYED_WORK(&charger->wpc_i2c_error_work, mfc_wpc_i2c_error_work);
	INIT_DELAYED_WORK(&charger->wpc_rx_type_det_work, mfc_wpc_rx_type_det_work);
	INIT_DELAYED_WORK(&charger->wpc_rx_connection_work, mfc_wpc_rx_connection_work);
	INIT_DELAYED_WORK(&charger->wpc_tx_min_op_freq_work, mfc_tx_min_op_freq_work);
	INIT_DELAYED_WORK(&charger->wpc_tx_duty_min_work, mfc_tx_duty_min_work);
	INIT_DELAYED_WORK(&charger->wpc_tx_phm_work, mfc_tx_phm_work);
	INIT_DELAYED_WORK(&charger->wpc_rx_power_work, mfc_wpc_rx_power_work);
	INIT_DELAYED_WORK(&charger->wpc_init_work, mfc_wpc_init_work);
	INIT_DELAYED_WORK(&charger->align_check_work, mfc_wpc_align_check_work);
	INIT_DELAYED_WORK(&charger->mode_change_work, mfc_wpc_mode_change_work);
	INIT_DELAYED_WORK(&charger->wpc_cs100_work, mfc_cs100_work);
	INIT_DELAYED_WORK(&charger->wpc_rx_power_trans_fail_work, mfc_rx_power_trans_fail_work);
	INIT_DELAYED_WORK(&charger->wpc_rx_phm_work, mfc_rx_phm_work);
	INIT_DELAYED_WORK(&charger->wpc_deactivate_work, mfc_wpc_deactivate_work);
	INIT_DELAYED_WORK(&charger->wpc_phm_exit_work, mfc_wpc_phm_exit_work);
	INIT_DELAYED_WORK(&charger->epp_clear_timer_work, mfc_epp_clear_timer_work);
	INIT_DELAYED_WORK(&charger->epp_count_work, mfc_epp_count_work);
	INIT_DELAYED_WORK(&charger->wpc_check_mpp_work, mfc_wpc_check_mpp_work);
	INIT_DELAYED_WORK(&charger->epp_mode_change_check_work, mfc_epp_mode_change_check_work);
	INIT_DELAYED_WORK(&charger->mpp_fake_ui_clear_work, mfc_mpp_fake_ui_clear_work);
	INIT_DELAYED_WORK(&charger->set_mpp_cover_work, mfc_set_mpp_cover_work);
	INIT_DELAYED_WORK(&charger->set_mpp_cloak_work, mfc_set_mpp_cloak_work);
	INIT_DELAYED_WORK(&charger->mpla_thr_recov_work, mfc_clear_mpla_thr_recov_work);

	/*
	 * Default Idle voltage of the INT_A is LOW.
	 * Prevent the un-wanted INT_A Falling handling.
	 * This is a work-around, and will be fixed by the revision.
	 */
	//INIT_DELAYED_WORK(&charger->mst_off_work, mfc_mst_off_work);

	alarm_init(&charger->phm_alarm, ALARM_BOOTTIME, mfc_phm_alarm);

	mfc_cfg.drv_data = charger;
	charger->psy_chg = power_supply_register(charger->dev, &mfc_charger_power_supply_desc, &mfc_cfg);
	if (IS_ERR(charger->psy_chg)) {
		ret = PTR_ERR(charger->psy_chg);
		pr_err("%s: Failed to Register psy_chg(%d)\n", __func__, ret);
		goto err_supply_unreg;
	}

	ret = mfc_create_attrs(&charger->psy_chg->dev);
	if (ret) {
		dev_err(charger->dev,
			"%s : Failed to create_attrs\n", __func__);
	}

	/* Enable interrupts after battery driver load */
	/* wpc_det */
	if (charger->pdata->irq_wpc_det) {
		ret = request_threaded_irq(charger->pdata->irq_wpc_det,
				mfc_wpc_irq_handler, mfc_wpc_det_irq_thread,
				IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
				IRQF_ONESHOT,
				"wpc-det-irq", charger);
		if (ret) {
			pr_err("%s: Failed to Request IRQ\n", __func__);
			goto err_irq_wpc_det;
		}
	}

	/* wpc_pdrc */
	if (charger->pdata->irq_wpc_pdrc) {
		INIT_DELAYED_WORK(&charger->wpc_pdrc_work, mfc_wpc_pdrc_work);
		ret = request_threaded_irq(charger->pdata->irq_wpc_pdrc,
				mfc_wpc_irq_handler, mfc_wpc_pdrc_irq_thread,
				IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
				IRQF_ONESHOT,
				"wpc-pdrc-irq", charger);
		if (ret) {
			pr_err("%s: Failed to Request pdrc IRQ\n", __func__);
			goto err_irq_wpc_det;
		}
	}

	/* wpc_pdet_b */
	if (charger->pdata->irq_wpc_pdet_b) {
		ret = request_threaded_irq(charger->pdata->irq_wpc_pdet_b,
				mfc_wpc_irq_handler, mfc_wpc_pdet_b_irq_thread,
				IRQF_TRIGGER_RISING |
				IRQF_ONESHOT,
				"wpc-pdet-b-irq", charger);
		if (ret) {
			pr_err("%s: Failed to Request pdet_b IRQ\n", __func__);
			goto err_irq_wpc_det;
		}
	}

	/* wpc_irq */
	queue_delayed_work(charger->wqueue, &charger->wpc_int_req_work, msecs_to_jiffies(100));

	int_state = gpio_get_value(charger->pdata->wpc_int);
	pr_info("%s: int_state = %d\n", __func__, int_state);
	if (gpio_get_value(charger->pdata->wpc_det)) {
		u8 irq_src[2];

		pr_info("%s: Charger interrupt occurred during lpm\n", __func__);
		charger->det_state = gpio_get_value(charger->pdata->wpc_det);

		mfc_reg_read(charger->client, MFC_INT_A_L_REG, &irq_src[0]);
		mfc_reg_read(charger->client, MFC_INT_A_H_REG, &irq_src[1]);
		/* clear interrupt */
		pr_info("%s: interrupt source(0x%x)\n", __func__, irq_src[1] << 8 | irq_src[0]);
		mfc_reg_write(charger->client, MFC_INT_A_CLEAR_L_REG, irq_src[0]); // clear int
		mfc_reg_write(charger->client, MFC_INT_A_CLEAR_H_REG, irq_src[1]); // clear int
		mfc_set_cmd_l_reg(charger, MFC_CMD_CLEAR_INT_MASK, MFC_CMD_CLEAR_INT_MASK); // command
		if (charger->pdata->wired_charger_name) {
			union power_supply_propval value;

			ret = psy_do_property(charger->pdata->wired_charger_name, get,
				POWER_SUPPLY_EXT_PROP_INPUT_CURRENT_LIMIT_WRL, value);
			charger->input_current = (ret) ? 500 : value.intval;
			pr_info("%s: updated input current (%d)\n",
				__func__, charger->input_current);
		}
		charger->req_afc_delay = 0;
		__pm_stay_awake(charger->wpc_det_ws);
		queue_delayed_work(charger->wqueue, &charger->wpc_det_work, 0);
		if (!int_state && !delayed_work_pending(&charger->wpc_isr_work)) {
			__pm_stay_awake(charger->wpc_rx_ws);
			queue_delayed_work(charger->wqueue, &charger->wpc_isr_work, msecs_to_jiffies(2000));
		}
	}

	sec_chg_set_dev_init(SC_DEV_WRL_CHG);
	ret = sb_wireless_set_op(charger, &cps4038_sbw_op);
	dev_info(&client->dev, "%s: MFC cps4038 Charger Driver Loaded(op = %d)\n", __func__, ret);
	device_init_wakeup(charger->dev, 1);
	return 0;

err_irq_wpc_det:
	power_supply_unregister(charger->psy_chg);
err_supply_unreg:
	wakeup_source_unregister(charger->wpc_ws);
	wakeup_source_unregister(charger->wpc_det_ws);
	wakeup_source_unregister(charger->wpc_rx_ws);
	wakeup_source_unregister(charger->wpc_tx_ws);
	wakeup_source_unregister(charger->wpc_update_ws);
	wakeup_source_unregister(charger->wpc_tx_min_opfq_ws);
	wakeup_source_unregister(charger->wpc_tx_duty_min_ws);
	wakeup_source_unregister(charger->wpc_afc_vout_ws);
	wakeup_source_unregister(charger->wpc_vout_mode_ws);
	wakeup_source_unregister(charger->wpc_rx_det_ws);
	wakeup_source_unregister(charger->wpc_tx_phm_ws);
	wakeup_source_unregister(charger->wpc_tx_id_ws);
	wakeup_source_unregister(charger->wpc_pdrc_ws);
	wakeup_source_unregister(charger->wpc_cs100_ws);
	wakeup_source_unregister(charger->wpc_pdet_b_ws);
	wakeup_source_unregister(charger->wpc_rx_phm_ws);
	wakeup_source_unregister(charger->wpc_phm_exit_ws);
	wakeup_source_unregister(charger->epp_clear_ws);
	wakeup_source_unregister(charger->check_mpp_ws);
	wakeup_source_unregister(charger->set_mpp_cover_ws);
	wakeup_source_unregister(charger->set_mpp_cloak_ws);
	wakeup_source_unregister(charger->mpla_thr_recov_ws);
err_pdata_free:
	mutex_destroy(&charger->io_lock);
	mutex_destroy(&charger->wpc_en_lock);
	mutex_destroy(&charger->fw_lock);
err_i2cfunc_not_support:
	kfree(charger);
err_wpc_nomem:
err_parse_dt:
	devm_kfree(&client->dev, pdata);

	return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int cps4038_charger_remove(struct i2c_client *client)
#else
static void cps4038_charger_remove(struct i2c_client *client)
#endif
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);

	alarm_cancel(&charger->phm_alarm);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	return 0;
#endif
}

#if defined(CONFIG_PM)
static int mfc_charger_suspend(struct device *dev)
{
	struct mfc_charger_data *charger = dev_get_drvdata(dev);

	pr_info("%s: det(%d) int(%d)\n", __func__,
			gpio_get_value(charger->pdata->wpc_det),
			gpio_get_value(charger->pdata->wpc_int));

	charger->is_suspend = true;

	if (device_may_wakeup(charger->dev)) {
		enable_irq_wake(charger->pdata->irq_wpc_int);
		enable_irq_wake(charger->pdata->irq_wpc_det);
		if (charger->pdata->irq_wpc_pdrc)
			enable_irq_wake(charger->pdata->irq_wpc_pdrc);
		if (charger->pdata->irq_wpc_pdet_b)
			enable_irq_wake(charger->pdata->irq_wpc_pdet_b);
	}
#if !IS_ENABLED(CONFIG_ENABLE_WIRELESS_IRQ_IN_SLEEP)
	disable_irq(charger->pdata->irq_wpc_int);
	disable_irq(charger->pdata->irq_wpc_det);
	if (charger->pdata->irq_wpc_pdrc)
		disable_irq(charger->pdata->irq_wpc_pdrc);
	if (charger->pdata->irq_wpc_pdet_b)
		disable_irq(charger->pdata->irq_wpc_pdet_b);
#endif

	return 0;
}

static int mfc_charger_resume(struct device *dev)
{
	struct mfc_charger_data *charger = dev_get_drvdata(dev);

	pr_info("%s: det(%d) int(%d)\n", __func__,
			gpio_get_value(charger->pdata->wpc_det),
			gpio_get_value(charger->pdata->wpc_int));

	charger->is_suspend = false;

	if (device_may_wakeup(charger->dev)) {
		disable_irq_wake(charger->pdata->irq_wpc_int);
		disable_irq_wake(charger->pdata->irq_wpc_det);
		if (charger->pdata->irq_wpc_pdrc)
			disable_irq_wake(charger->pdata->irq_wpc_pdrc);
		if (charger->pdata->irq_wpc_pdet_b)
			disable_irq_wake(charger->pdata->irq_wpc_pdet_b);
	}
#if !IS_ENABLED(CONFIG_ENABLE_WIRELESS_IRQ_IN_SLEEP)
	enable_irq(charger->pdata->irq_wpc_int);
	enable_irq(charger->pdata->irq_wpc_det);
	if (charger->pdata->irq_wpc_pdrc)
		enable_irq(charger->pdata->irq_wpc_pdrc);
	if (charger->pdata->irq_wpc_pdet_b)
		enable_irq(charger->pdata->irq_wpc_pdet_b);
#else
	/* Level triggering makes infinite IRQ, Edge triggering is required */
	__pm_stay_awake(charger->wpc_ws);
	__pm_stay_awake(charger->wpc_det_ws);
	mfc_wpc_irq_thread(0, charger);
	if (charger->pdata->irq_wpc_pdrc) {
		__pm_stay_awake(charger->wpc_pdrc_ws);
		mfc_wpc_pdrc_irq_thread(0, charger);
	}
	if (charger->pdata->irq_wpc_pdet_b) {
		__pm_stay_awake(charger->wpc_pdet_b_ws);
		mfc_wpc_pdet_b_irq_thread(0, charger);
	}
	mfc_wpc_det_irq_thread(0, charger);
#endif

	if (charger->skip_phm_work_in_sleep)
		queue_delayed_work(charger->wqueue, &charger->wpc_tx_phm_work, 0);

	return 0;
}
#else
#define mfc_charger_suspend NULL
#define mfc_charger_resume NULL
#endif

#if defined(CONFIG_WIRELESS_RX_PHM_CTRL)
static void mfc_disable_irq_nosync(int irq)
{
	struct irq_desc *desc;

	if (irq <= 0)
		return;

	desc = irq_to_desc(irq);
	if (desc->depth == 0)
		disable_irq_nosync(irq);
}
#endif

static void cps4038_charger_shutdown(struct i2c_client *client)
{
	struct mfc_charger_data *charger = i2c_get_clientdata(client);

	is_shutdn = true;
	pr_info("%s\n", __func__);

	cancel_delayed_work(&charger->wpc_vout_mode_work);
	alarm_cancel(&charger->phm_alarm);

	if (gpio_get_value(charger->pdata->wpc_det)) {
		pr_info("%s: forced 5V Vout\n", __func__);
		/* Prevent for unexpected FOD when reboot on morphie pad */
		mfc_set_vrect_adjust(charger, MFC_HEADROOM_6);
		mfc_set_vout(charger, MFC_VOUT_5V);
		mfc_set_pad_hv(charger, false);
	}
	cancel_delayed_work(&charger->wpc_tx_min_op_freq_work);
	cancel_delayed_work(&charger->wpc_tx_duty_min_work);
	cancel_delayed_work(&charger->wpc_rx_phm_work);

#if defined(CONFIG_WIRELESS_RX_PHM_CTRL)
	if (charger->pdata->cable_type != SEC_BATTERY_CABLE_NONE ||
		charger->rx_phm_status) {
		mfc_disable_irq_nosync(charger->pdata->irq_wpc_pdrc);
		mfc_disable_irq_nosync(charger->pdata->irq_wpc_det);
		mfc_disable_irq_nosync(charger->pdata->irq_wpc_pdet_b);

		if (charger->pdata->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP)
			mfc_send_ept_packet(charger, MFC_WPC_EPT_RESTART);

		if (charger->pdata->ping_nen >= 0) {
			if (!gpio_get_value(charger->pdata->ping_nen)) {
				/* reset rx ic and tx pad for phm exit */
				mfc_set_wpc_en(charger, WPC_EN_CHARGING, false);
				msleep(1500);
			}
		}
	}
#endif
}

static const struct i2c_device_id cps4038_charger_id_table[] = {
	{ "cps4038-charger", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, cps4038_charger_id_table);

#ifdef CONFIG_OF
static const struct of_device_id cps4038_charger_match_table[] = {
	{ .compatible = "cps,cps4038-charger",},
	{},
};
MODULE_DEVICE_TABLE(of, cps4038_charger_match_table);
#else
#define cps4038_charger_match_table NULL
#endif

const struct dev_pm_ops cps4038_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(mfc_charger_suspend, mfc_charger_resume)
};

static struct i2c_driver cps4038_charger_driver = {
	.driver = {
		.name	= "cps4038-charger",
		.owner	= THIS_MODULE,
#if defined(CONFIG_PM)
		.pm = &cps4038_pm,
#endif /* CONFIG_PM */
		.of_match_table = cps4038_charger_match_table,
	},
	.shutdown	= cps4038_charger_shutdown,
	.probe	= cps4038_charger_probe,
	.remove	= cps4038_charger_remove,
	.id_table	= cps4038_charger_id_table,
};

static int __init cps4038_charger_init(void)
{
	pr_info("%s\n", __func__);
	return i2c_add_driver(&cps4038_charger_driver);
}

static void __exit cps4038_charger_exit(void)
{
	pr_info("%s\n", __func__);
	i2c_del_driver(&cps4038_charger_driver);
}

module_init(cps4038_charger_init);
module_exit(cps4038_charger_exit);

MODULE_DESCRIPTION("Samsung CPS4038 Charger Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
