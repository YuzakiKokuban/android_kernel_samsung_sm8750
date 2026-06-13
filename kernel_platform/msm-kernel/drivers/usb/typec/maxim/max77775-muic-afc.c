/*
 * max77775-muic.c - MUIC driver for the Maxim 77775
 *
 *  Copyright (C) 2015 Samsung Electronics
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
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>

#include <linux/mfd/max77775_log.h>

/* MUIC header file */
#include <linux/muic/common/muic.h>
#include <linux/usb/typec/maxim/max77775-muic.h>
#include <linux/usb/typec/maxim/max77775_usbc.h>

#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/common/muic_notifier.h>
#endif /* CONFIG_MUIC_NOTIFIER */

#if defined(CONFIG_USB_HW_PARAM)
#include <linux/usb_notify.h>
#endif

#if IS_ENABLED(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif

#define RETRY_COUNT 3

bool max77775_muic_check_is_enable_afc(struct max77775_muic_data *muic_data, muic_attached_dev_t new_dev)
{
	struct max77775_usbc_platform_data *usbc_pdata = muic_data->usbc_pdata;
	int ret = false;

	if (new_dev == ATTACHED_DEV_TA_MUIC || new_dev == ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC ||
			muic_data->is_usb_fail) {
		if (!muic_data->is_charger_ready) {
			md75_info_usb("%s Charger is not ready(%d), skip AFC\n",
				__func__, muic_data->is_charger_ready);
		} else if (muic_data->is_charger_mode) {
			md75_info_usb("%s is_charger_mode(%d), skip AFC\n",
				__func__, muic_data->is_charger_mode);
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
		} else if (usbc_pdata->fac_water_enable) {
			md75_info_usb("%s fac_water_enable(%d), skip AFC\n", __func__,
				usbc_pdata->fac_water_enable);
#endif /* CONFIG_PDIC_NOTIFIER */
		} else if (muic_data->afc_water_disable) {
			md75_info_usb("%s water detected(%d), skip AFC\n", __func__,
				muic_data->afc_water_disable);
		} else if (usbc_pdata->pd_support) {
			md75_info_usb("%s PD TA detected(%d), skip AFC\n", __func__,
				usbc_pdata->pd_support);
		} else {
			ret = true;
		}
	}

	return ret;
}

static void max77775_muic_afc_reset(struct max77775_muic_data *muic_data)
{
	struct max77775_usbc_platform_data *usbc_pdata = muic_data->usbc_pdata;
	usbc_cmd_data write_data;

	md75_info_usb("%s:%s\n", MUIC_DEV_NAME, __func__);
	muic_data->is_afc_reset = true;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = COMMAND_BC_CTRL2_WRITE;
	write_data.write_length = 1;
	write_data.write_data[0] = 0x13; /* DPDNMan enable, DP GND, DM Open */
	write_data.read_length = 0;

	max77775_usbc_opcode_write(usbc_pdata, &write_data);
}

void max77775_muic_check_afc_disabled(struct max77775_muic_data *muic_data)
{
	struct muic_platform_data *pdata = muic_data->pdata;
	muic_attached_dev_t new_attached_dev = (muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_DISABLED_MUIC ||
						muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC ||
						muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_9V_MUIC) ?
		ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC : ATTACHED_DEV_TA_MUIC;
	md75_info_usb("%s:%s\n", MUIC_DEV_NAME, __func__);

	if ((!pdata->afc_disable && (muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_DISABLED_MUIC ||
					muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC ||
					muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_5V_MUIC)) ||
		(pdata->afc_disable && (muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_9V_MUIC ||
					muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_9V_MUIC ||
					muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC ||
					muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_5V_MUIC
					))) {

		md75_info_usb("%s:%s change charger (%d) -> (%d)\n", MUIC_DEV_NAME, __func__,
			muic_data->attached_dev, new_attached_dev);

		muic_data->attached_dev = new_attached_dev;
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
		muic_notifier_attach_attached_dev(new_attached_dev);
#endif

		cancel_delayed_work_sync(&(muic_data->afc_work));
		schedule_delayed_work(&(muic_data->afc_work), msecs_to_jiffies(500));
	}
}

static void max77775_muic_afc_hv_tx_byte_set(struct max77775_muic_data *muic_data, u8 tx_byte)
{
	struct max77775_usbc_platform_data *usbc_pdata = muic_data->usbc_pdata;
	usbc_cmd_data write_data;

	md75_info_usb("%s:%s tx_byte(0x%02x)\n", MUIC_DEV_NAME, __func__, tx_byte);

	init_usbc_cmd_data(&write_data);
	write_data.opcode = COMMAND_AFC_RESULT_READ;
	write_data.write_length = 1;
	write_data.write_data[0] = tx_byte;
	write_data.read_length = 10;

	max77775_usbc_opcode_write(usbc_pdata, &write_data);
}

void max77775_muic_clear_hv_control(struct max77775_muic_data *muic_data)
{
	struct max77775_usbc_platform_data *usbc_pdata = muic_data->usbc_pdata;
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = COMMAND_HV_CONTROL_WRITE;
	write_data.write_length = 1;
	write_data.write_data[0] = 0;

	max77775_usbc_opcode_write(usbc_pdata, &write_data);
}

int max77775_muic_afc_hv_set(struct max77775_muic_data *muic_data, int voltage)
{
	struct max77775_usbc_platform_data *usbc_pdata = muic_data->usbc_pdata;
	usbc_cmd_data write_data;
	u8 tx_byte;

	switch (voltage) {
	case 5:
		tx_byte = 0x08;
		break;
	case 9:
		tx_byte = 0x46;
		break;
	default:
		md75_info_usb("%s:%s invalid value(%d), return\n", MUIC_DEV_NAME,
				__func__, voltage);
		return -EINVAL;
	}

	md75_info_usb("%s:%s voltage(%d)\n", MUIC_DEV_NAME, __func__, voltage);

	init_usbc_cmd_data(&write_data);
	write_data.opcode = COMMAND_AFC_RESULT_READ;
	write_data.write_length = 1;
	write_data.write_data[0] = tx_byte;
	write_data.read_length = 10;

	return max77775_usbc_opcode_write(usbc_pdata, &write_data);
}

int max77775_muic_qc_hv_set(struct max77775_muic_data *muic_data, int voltage)
{
	struct max77775_usbc_platform_data *usbc_pdata = muic_data->usbc_pdata;
	usbc_cmd_data write_data;
	u8 dpdndrv;

	switch (voltage) {
	case 5:
		dpdndrv = 0x04;
		break;
	case 9:
		dpdndrv = 0x09;
		break;
	default:
		md75_info_usb("%s:%s invalid value(%d), return\n", MUIC_DEV_NAME,
				__func__, voltage);
		return -EINVAL;
	}

	md75_info_usb("%s:%s voltage(%d)\n", MUIC_DEV_NAME, __func__, voltage);

	init_usbc_cmd_data(&write_data);
	write_data.opcode = COMMAND_QC_2_0_SET;
	write_data.write_length = 1;
	write_data.write_data[0] = dpdndrv;
	write_data.read_length = 2;

	return max77775_usbc_opcode_write(usbc_pdata, &write_data);
}

#if !defined(CONFIG_MAX77775_MUIC_QC_DISABLE)
static void max77775_muic_handle_detect_dev_mpnack(struct max77775_muic_data *muic_data)
{
	struct max77775_usbc_platform_data *usbc_pdata = muic_data->usbc_pdata;
	usbc_cmd_data write_data;
	u8 dpdndrv = 0x09;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = COMMAND_QC_2_0_SET;
	write_data.write_length = 1;
	write_data.write_data[0] = dpdndrv;
	write_data.read_length = 2;

	max77775_usbc_opcode_write(usbc_pdata, &write_data);
}
#endif
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
static void max77775_muic_handle_afc_retry(struct max77775_muic_data *muic_data,
			muic_attached_dev_t current_attached_dev)
{
	md75_info_usb("%s:%s current_attached_dev: %d\n", MUIC_DEV_NAME, __func__, current_attached_dev);

	muic_data->attached_dev = current_attached_dev;
	if (muic_data->attached_dev != ATTACHED_DEV_TIMEOUT_OPEN_MUIC) {
		muic_data->attached_dev = ATTACHED_DEV_RETRY_TIMEOUT_OPEN_MUIC;
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
		muic_notifier_detach_attached_dev(current_attached_dev);
		muic_notifier_attach_attached_dev(muic_data->attached_dev);
#endif
	}
}
#endif
void max77775_muic_handle_detect_dev_afc(struct max77775_muic_data *muic_data, unsigned char *data)
{
	int result = data[1];
	int vbadc = data[2];
	int vbadc2 = (muic_data->status1 & USBC_STATUS1_VBADC_MASK) >> USBC_STATUS1_VBADC_SHIFT;
	muic_attached_dev_t new_afc_dev = muic_data->attached_dev;
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
	bool noti = true;
#endif
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
	bool afc_err = false;
#endif
	bool afc_nack = false;
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
	bool afc_retry_fail = false;
	muic_attached_dev_t current_attached_dev = muic_data->attached_dev;
#endif
	int i = 0;
	int ret = 0;

	/* W/A: vbadc of opcode result is 0, but vbadc register value is not 0 */
	if (vbadc == 0 && vbadc2 > 0)
		vbadc = data[2] = vbadc2;

	md75_info_usb("%s:%s result:0x%x vbadc:0x%x rxbyte:0x%x %x %x %x %x %x %x %x\n", MUIC_DEV_NAME,
			__func__, data[1], data[2], data[3], data[4], data[5],
			data[6], data[7], data[8], data[9], data[10]);

	switch (result) {
	case 0:
		md75_info_usb("%s:%s AFC Success, vbadc(%d)\n", MUIC_DEV_NAME, __func__, vbadc);
		muic_data->afc_retry = 0;

		if (vbadc >= MAX77775_VBADC_4_5V_TO_5_5V &&
				vbadc <= MAX77775_VBADC_6_5V_TO_7_5V) {
			if (muic_data->pdata->afc_disable) {
				md75_info_usb("%s:%s AFC disabled, set cable type to AFC_CHARGER_DISABLED\n", MUIC_DEV_NAME, __func__);
				new_afc_dev = ATTACHED_DEV_AFC_CHARGER_DISABLED_MUIC;
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
				if (muic_data->is_usb_fail)
					new_afc_dev = ATTACHED_DEV_RETRY_AFC_CHARGER_5V_MUIC;
#endif
			} else {
				new_afc_dev = ATTACHED_DEV_AFC_CHARGER_5V_MUIC;
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
				if (muic_data->is_usb_fail)
					new_afc_dev = ATTACHED_DEV_RETRY_AFC_CHARGER_5V_MUIC;
#endif
			}
		} else if (vbadc >= MAX77775_VBADC_7_5V_TO_8_5V &&
				vbadc <= MAX77775_VBADC_9_5V_TO_10_5V) {
			new_afc_dev = ATTACHED_DEV_AFC_CHARGER_9V_MUIC;
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
			if (muic_data->is_usb_fail)
				new_afc_dev = ATTACHED_DEV_RETRY_AFC_CHARGER_9V_MUIC;
#endif
		}
#if defined(CONFIG_USB_HW_PARAM)
		else
			afc_err = true;
#endif

		if (new_afc_dev != muic_data->attached_dev) {
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
			if (muic_data->is_usb_fail)
				muic_notifier_detach_attached_dev(muic_data->attached_dev);
#endif
			muic_notifier_attach_attached_dev(new_afc_dev);
#endif /* CONFIG_MUIC_NOTIFIER */
			muic_data->attached_dev = new_afc_dev;
		}
		break;
	case 1:
		md75_info_usb("%s:%s No CHGIN\n", MUIC_DEV_NAME, __func__);
		if (muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_9V_MUIC ||
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
				muic_data->attached_dev == ATTACHED_DEV_RETRY_AFC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_RETRY_AFC_CHARGER_9V_MUIC ||
#endif
				muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_9V_MUIC)
			afc_nack = true;
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
		else if (muic_data->is_usb_fail)
			afc_retry_fail = true;
#endif
		break;
	case 2:
		md75_info_usb("%s:%s Not High Voltage DCP\n", MUIC_DEV_NAME, __func__);
		if (muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_9V_MUIC ||
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
				muic_data->attached_dev == ATTACHED_DEV_RETRY_AFC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_RETRY_AFC_CHARGER_9V_MUIC ||
#endif
				muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_9V_MUIC)
			afc_nack = true;
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
		else if (muic_data->is_usb_fail)
			afc_retry_fail = true;
#endif
		break;
	case 3:
		md75_info_usb("%s:%s Not DCP\n", MUIC_DEV_NAME, __func__);
		if (muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_9V_MUIC ||
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
				muic_data->attached_dev == ATTACHED_DEV_RETRY_AFC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_RETRY_AFC_CHARGER_9V_MUIC ||
#endif
				muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_9V_MUIC)
			afc_nack = true;
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
		else if (muic_data->is_usb_fail)
			afc_retry_fail = true;
#endif
		break;
	case 4:
		md75_info_usb("%s:%s MPing NACK\n", MUIC_DEV_NAME, __func__);
		if (muic_data->pdata->afc_disable) {
			md75_info_usb("%s:%s skip checking QC TA by afc disable, just return!\n", MUIC_DEV_NAME, __func__);
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
			if (muic_data->is_usb_fail)
				afc_retry_fail = true;
#endif
		}
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
		else if (muic_data->is_usb_fail) {
			afc_retry_fail = true;
			md75_info_usb("%s:%s skip checking QC TA by usb fail, just return!\n", MUIC_DEV_NAME, __func__);
		}
#endif
#if !defined(CONFIG_MAX77775_MUIC_QC_DISABLE)
		else {
			md75_info_usb("%s:%s checking QC TA!\n", MUIC_DEV_NAME, __func__);
			max77775_muic_handle_detect_dev_mpnack(muic_data);
		}
#endif
		break;
	case 5:
		md75_info_usb("%s:%s Unsupported TX data\n", MUIC_DEV_NAME, __func__);
		if (muic_data->afc_retry++ < RETRY_COUNT) {
			md75_info_usb("%s:%s Retry(%d)\n", MUIC_DEV_NAME, __func__, muic_data->afc_retry);
			for (i = 3; (i <= 10) && (data[i] != 0); i++) {
				if ((muic_data->hv_voltage == 9) && ((data[i] & 0xF0) == 0x40)) {
					/* 9V case */
					md75_info_usb("%s:%s seleted tx byte = 0x%02x", MUIC_DEV_NAME,
							__func__, data[i]);
					max77775_muic_afc_hv_tx_byte_set(muic_data, data[i]);
					break;
				} else if ((muic_data->hv_voltage == 5) && ((data[i] & 0xF0) == 0x0)) {
					/* 5V case */
					md75_info_usb("%s:%s seleted tx byte = 0x%02x", MUIC_DEV_NAME,
							__func__, data[i]);
					max77775_muic_afc_hv_tx_byte_set(muic_data, data[i]);
					break;
				}
			}
		}
		break;
	case 6:
		md75_info_usb("%s:%s Vbus is not changed with 3 continuous ping\n",
				MUIC_DEV_NAME, __func__);
		afc_nack = true;
		break;
	case 7:
		md75_info_usb("%s:%s Vbus is not changed in 1sec\n",
				MUIC_DEV_NAME, __func__);
		afc_nack = true;
		break;
	case 8:
		md75_info_usb("%s:%s CC-Vbus Short case\n", MUIC_DEV_NAME, __func__);

		muic_data->attached_dev = ATTACHED_DEV_TA_MUIC;
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
		if (muic_data->is_usb_fail) {
			afc_retry_fail = true;
			noti = false;
		}
#endif
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
		if (noti)
			muic_notifier_attach_attached_dev(muic_data->attached_dev);
#endif /* CONFIG_MUIC_NOTIFIER */
#if IS_ENABLED(CONFIG_SEC_ABC)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		sec_abc_send_event("MODULE=muic@INFO=cable_short");
#else
		sec_abc_send_event("MODULE=muic@WARN=cable_short");
#endif
#endif
		break;
	case 9:
		md75_info_usb("%s:%s SBU-Gnd Short case\n", MUIC_DEV_NAME, __func__);

		muic_data->attached_dev = ATTACHED_DEV_TA_MUIC;
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
		if (muic_data->is_usb_fail) {
			afc_retry_fail = true;
			noti = false;
		}
#endif
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
		if (noti)
			muic_notifier_attach_attached_dev(muic_data->attached_dev);
#endif /* CONFIG_MUIC_NOTIFIER */
#if IS_ENABLED(CONFIG_SEC_ABC)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		sec_abc_send_event("MODULE=muic@INFO=cable_short");
#else
		sec_abc_send_event("MODULE=muic@WARN=cable_short");
#endif
#endif
		break;
	case 10:
		md75_info_usb("%s:%s SBU-Vbus Short case\n", MUIC_DEV_NAME, __func__);

		muic_data->attached_dev = ATTACHED_DEV_TA_MUIC;
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
		if (muic_data->is_usb_fail) {
			afc_retry_fail = true;
			noti = false;
		}
#endif
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
		if (noti)
			muic_notifier_attach_attached_dev(muic_data->attached_dev);
#endif /* CONFIG_MUIC_NOTIFIER */
#if IS_ENABLED(CONFIG_SEC_ABC)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		sec_abc_send_event("MODULE=muic@INFO=cable_short");
#else
		sec_abc_send_event("MODULE=muic@WARN=cable_short");
#endif
#endif
		break;
	case 11:
		md75_info_usb("%s:%s Not Rp 56K\n", MUIC_DEV_NAME, __func__);
		if (muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_9V_MUIC ||
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
				muic_data->attached_dev == ATTACHED_DEV_RETRY_AFC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_RETRY_AFC_CHARGER_9V_MUIC ||
#endif
				muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_9V_MUIC)
			afc_nack = true;
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
		else if (muic_data->is_usb_fail)
			afc_retry_fail = true;
#endif
		break;
	case 16:
		md75_info_usb("%s:%s A parity check failed during resceiving data\n",
				MUIC_DEV_NAME, __func__);
		afc_nack = true;
		break;
	case 17:
		md75_info_usb("%s:%s The slave does not respond to the master ping\n",
				MUIC_DEV_NAME, __func__);
		afc_nack = true;
		break;
	case 18:
		md75_info_usb("%s:%s RX buffer overflow is detected\n",
				MUIC_DEV_NAME, __func__);
		afc_nack = true;
		break;
	default:
		md75_info_usb("%s:%s AFC error(%d)\n", MUIC_DEV_NAME, __func__, result);
		afc_nack = true;
		break;
	}

	if (afc_nack) {
		if (muic_data->afc_retry++ < RETRY_COUNT) {
			md75_info_usb("%s:%s Retry(%d)\n", MUIC_DEV_NAME, __func__, muic_data->afc_retry);
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
			/* Charging TG's request, send PREPARE noti */
			if (!muic_data->is_usb_fail)
				muic_notifier_attach_attached_dev(ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC);
#endif /* CONFIG_MUIC_NOTIFIER */
			max77775_muic_afc_hv_set(muic_data, muic_data->hv_voltage);
		} else {
			md75_info_usb("%s:%s Retry Done, do not retry\n", MUIC_DEV_NAME, __func__);
			if (vbadc >= MAX77775_VBADC_7_5V_TO_8_5V) {
				max77775_muic_afc_reset(muic_data);
				muic_data->afc_retry = 0;
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
				if (muic_data->is_usb_fail)
					afc_retry_fail = true;
#endif
			} else {
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
				/* Send attached device noti to clear prepare noti */
				if (muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC ||
					muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_9V_MUIC)
					muic_notifier_attach_attached_dev(muic_data->attached_dev);
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
				else if (muic_data->is_usb_fail)
					afc_retry_fail = true;
#endif
				else
					muic_notifier_attach_attached_dev(ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC);
#endif /* CONFIG_MUIC_NOTIFIER */
			}
			mutex_lock(&muic_data->afc_lock);
			muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_DISABLE_CHANGE_DURING_WORK_END;
			muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_SET_VOLTAGE_CHANGE_DURING_WORK_END;
			muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_WORK_PROCESS_END;
			mutex_unlock(&muic_data->afc_lock);
#if IS_ENABLED(CONFIG_SEC_ABC)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
			sec_abc_send_event("MODULE=muic@INFO=afc_hv_fail");
#else
			sec_abc_send_event("MODULE=muic@WARN=afc_hv_fail");
#endif
#endif
		}
	} else {
		mutex_lock(&muic_data->afc_lock);
		if (muic_data->pdata->afc_disabled_updated & MAX77775_MUIC_AFC_DISABLE_CHANGE_DURING_WORK) {
			max77775_muic_check_afc_disabled(muic_data);
			muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_DISABLE_CHANGE_DURING_WORK_END;
		}

		muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_WORK_PROCESS_END;

		if (muic_data->pdata->afc_disabled_updated & MAX77775_MUIC_AFC_SET_VOLTAGE_CHANGE_DURING_WORK) {
			muic_data->pdata->afc_disabled_updated |= MAX77775_MUIC_AFC_WORK_PROCESS;

			ret = __max77775_muic_afc_set_voltage(muic_data, muic_data->reserve_hv_voltage);
			if (ret < 0)
				muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_WORK_PROCESS_END;

			muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_SET_VOLTAGE_CHANGE_DURING_WORK_END;
		}
		mutex_unlock(&muic_data->afc_lock);
	}

#if defined(CONFIG_USB_HW_PARAM)
	if (o_notify) {
		if (muic_data->is_skip_bigdata)
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
			goto afc_out;
#else
			return;
#endif
		if (afc_err && !afc_nack)
			inc_hw_param(o_notify, USB_MUIC_AFC_ERROR_COUNT);
		if (afc_nack) {
			inc_hw_param(o_notify, USB_MUIC_AFC_ERROR_COUNT);
			inc_hw_param(o_notify, USB_MUIC_AFC_NACK_COUNT);
			muic_data->is_skip_bigdata = true;
		}
	}
#endif
#if IS_ENABLED(CONFIG_MUIC_AFC_RETRY)
#if defined(CONFIG_USB_HW_PARAM)
afc_out:
#endif
	if (afc_retry_fail)
		max77775_muic_handle_afc_retry(muic_data, current_attached_dev);
#endif /* CONFIG_MUIC_AFC_RETRY */
}

void max77775_muic_handle_detect_dev_qc(struct max77775_muic_data *muic_data, unsigned char *data)
{
	int result = data[1];
	int vbadc = data[2];
	int vbadc2 = (muic_data->status1 & USBC_STATUS1_VBADC_MASK) >> USBC_STATUS1_VBADC_SHIFT;
	muic_attached_dev_t new_afc_dev = muic_data->attached_dev;
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
	bool afc_err = false;
#endif
	bool afc_nack = false;
	int ret = 0;

	/* W/A: vbadc of opcode result is 0, but vbadc register value is not 0 */
	if (vbadc == 0 && vbadc2 > 0)
		vbadc = data[2] = vbadc2;

	md75_info_usb("%s:%s result:0x%x vbadc:0x%x\n", MUIC_DEV_NAME,
			__func__, data[1], data[2]);

	switch (result) {
	case 0:
		md75_info_usb("%s:%s QC2.0 Success, vbadc(%d)\n", MUIC_DEV_NAME, __func__, vbadc);
		muic_data->afc_retry = 0;

		if (vbadc >= MAX77775_VBADC_4_5V_TO_5_5V &&
				vbadc <= MAX77775_VBADC_6_5V_TO_7_5V)
			new_afc_dev = ATTACHED_DEV_QC_CHARGER_5V_MUIC;
		else if (vbadc >= MAX77775_VBADC_7_5V_TO_8_5V &&
				vbadc <= MAX77775_VBADC_9_5V_TO_10_5V)
			new_afc_dev = ATTACHED_DEV_QC_CHARGER_9V_MUIC;
#if defined(CONFIG_USB_HW_PARAM)
		else
			afc_err = true;
#endif

		if (new_afc_dev != muic_data->attached_dev) {
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
			muic_notifier_attach_attached_dev(new_afc_dev);
#endif /* CONFIG_MUIC_NOTIFIER */
			muic_data->attached_dev = new_afc_dev;
		}
		break;
	case 1:
		md75_info_usb("%s:%s No CHGIN\n", MUIC_DEV_NAME, __func__);
		if (muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_9V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_9V_MUIC)
			afc_nack = true;
		break;
	case 2:
		md75_info_usb("%s:%s Not High Voltage DCP\n",
				MUIC_DEV_NAME, __func__);
		if (muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_9V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_9V_MUIC)
			afc_nack = true;
		break;
	case 3:
		md75_info_usb("%s:%s Not DCP\n", MUIC_DEV_NAME, __func__);
		if (muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_AFC_CHARGER_9V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_5V_MUIC ||
				muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_9V_MUIC)
			afc_nack = true;
		break;
	case 6:
		md75_info_usb("%s:%s Vbus is not changed with 3 continuous ping\n",
				MUIC_DEV_NAME, __func__);
		afc_nack = true;
		break;
	case 7:
		md75_info_usb("%s:%s Vbus is not changed in 1 sec\n",
				MUIC_DEV_NAME, __func__);
		afc_nack = true;
		break;
	default:
		md75_info_usb("%s:%s QC2.0 error(%d)\n", MUIC_DEV_NAME, __func__, result);
		afc_nack = true;
		break;
	}

	if (afc_nack) {
		if (muic_data->afc_retry++ < RETRY_COUNT) {
			md75_info_usb("%s:%s Retry(%d)\n", MUIC_DEV_NAME, __func__, muic_data->afc_retry);
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
			/* Charging TG's request, send PREPARE noti */
			muic_notifier_attach_attached_dev(ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC);
#endif /* CONFIG_MUIC_NOTIFIER */
			max77775_muic_qc_hv_set(muic_data, muic_data->hv_voltage);
		} else {
			md75_info_usb("%s:%s Retry Done, do not retry\n", MUIC_DEV_NAME, __func__);
			if (vbadc >= MAX77775_VBADC_7_5V_TO_8_5V) {
				max77775_muic_afc_reset(muic_data);
				muic_data->afc_retry = 0;
			} else {
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
				/* Send attached device noti to clear prepare noti */
				if (muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_5V_MUIC ||
					muic_data->attached_dev == ATTACHED_DEV_QC_CHARGER_9V_MUIC)
					muic_notifier_attach_attached_dev(muic_data->attached_dev);
				else
					muic_notifier_attach_attached_dev(ATTACHED_DEV_QC_CHARGER_ERR_V_MUIC);
#endif /* CONFIG_MUIC_NOTIFIER */
			}
			mutex_lock(&muic_data->afc_lock);
			muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_DISABLE_CHANGE_DURING_WORK_END;
			muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_SET_VOLTAGE_CHANGE_DURING_WORK_END;
			muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_WORK_PROCESS_END;
			mutex_unlock(&muic_data->afc_lock);
#if IS_ENABLED(CONFIG_SEC_ABC)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
			sec_abc_send_event("MODULE=muic@INFO=qc_hv_fail");
#else
			sec_abc_send_event("MODULE=muic@WARN=qc_hv_fail");
#endif
#endif
		}
	} else {
		mutex_lock(&muic_data->afc_lock);
		if (muic_data->pdata->afc_disabled_updated & MAX77775_MUIC_AFC_DISABLE_CHANGE_DURING_WORK) {
			max77775_muic_check_afc_disabled(muic_data);
			muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_DISABLE_CHANGE_DURING_WORK_END;
		}

		muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_WORK_PROCESS_END;

		if (muic_data->pdata->afc_disabled_updated & MAX77775_MUIC_AFC_SET_VOLTAGE_CHANGE_DURING_WORK) {
			muic_data->pdata->afc_disabled_updated |= MAX77775_MUIC_AFC_WORK_PROCESS;

			ret = __max77775_muic_afc_set_voltage(muic_data, muic_data->reserve_hv_voltage);
			if (ret < 0)
				muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_WORK_PROCESS_END;

			muic_data->pdata->afc_disabled_updated &= MAX77775_MUIC_AFC_SET_VOLTAGE_CHANGE_DURING_WORK_END;
		}
		mutex_unlock(&muic_data->afc_lock);
	}

#if defined(CONFIG_USB_HW_PARAM)
	if (o_notify) {
		if (muic_data->is_skip_bigdata)
			goto qc_out;

		if (afc_err && !afc_nack)
			inc_hw_param(o_notify, USB_MUIC_AFC_ERROR_COUNT);
		if (afc_nack) {
			inc_hw_param(o_notify, USB_MUIC_AFC_ERROR_COUNT);
			inc_hw_param(o_notify, USB_MUIC_AFC_NACK_COUNT);
			muic_data->is_skip_bigdata = true;
		}
	}
qc_out:
	return;
#endif
}
