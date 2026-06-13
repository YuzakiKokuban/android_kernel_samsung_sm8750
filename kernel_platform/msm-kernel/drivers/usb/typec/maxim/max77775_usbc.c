/*
 * Copyrights (C) 2017 Samsung Electronics, Inc.
 * Copyrights (C) 2017 Maxim Integrated Products, Inc.
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
 */

#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
#include <linux/mfd/core.h>
#include <linux/mfd/max77775_log.h>
#include <linux/mfd/max77775.h>
#include <linux/mfd/max77775-private.h>
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
#include <linux/usb_notify.h>
#endif
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
#include <linux/usb/typec/common/pdic_core.h>
#include <linux/usb/typec/common/pdic_sysfs.h>
#include <linux/usb/typec/common/pdic_notifier.h>
#include <linux/usb/typec/common/pdic_param.h>
#endif
#include <linux/muic/common/muic.h>
#include <linux/usb/typec/maxim/max77775-muic.h>
#include <linux/usb/typec/maxim/max77775_usbc.h>
#include <linux/usb/typec/maxim/max77775_alternate.h>
#include <linux/usb/typec/manager/if_cb_manager.h>
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
#include <linux/battery/sec_battery_common.h>
#endif
#ifdef MAX77775_SYS_FW_UPDATE
#include <linux/firmware.h>
#if IS_ENABLED(CONFIG_SPU_VERIFY)
#include <linux/spu-verify.h>
#endif
#endif
#include <linux/version.h>
#if IS_ENABLED(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif

static enum pdic_sysfs_property max77775_sysfs_properties[] = {
	PDIC_SYSFS_PROP_CHIP_NAME,
	PDIC_SYSFS_PROP_CUR_VERSION,
	PDIC_SYSFS_PROP_SRC_VERSION,
	PDIC_SYSFS_PROP_LPM_MODE,
	PDIC_SYSFS_PROP_STATE,
	PDIC_SYSFS_PROP_RID,
	PDIC_SYSFS_PROP_CTRL_OPTION,
	PDIC_SYSFS_PROP_BOOTING_DRY,
	PDIC_SYSFS_PROP_FW_UPDATE,
	PDIC_SYSFS_PROP_FW_UPDATE_STATUS,
	PDIC_SYSFS_PROP_FW_WATER,
	PDIC_SYSFS_PROP_DEX_FAN_UVDM,
	PDIC_SYSFS_PROP_ACC_DEVICE_VERSION,
	PDIC_SYSFS_PROP_DEBUG_OPCODE,
	PDIC_SYSFS_PROP_CONTROL_GPIO,
	PDIC_SYSFS_PROP_USBPD_IDS,
	PDIC_SYSFS_PROP_USBPD_TYPE,
	PDIC_SYSFS_PROP_CC_PIN_STATUS,
	PDIC_SYSFS_PROP_RAM_TEST,
	PDIC_SYSFS_PROP_SBU_ADC,
	PDIC_SYSFS_PROP_CC_ADC,
	PDIC_SYSFS_PROP_VSAFE0V_STATUS,
	PDIC_SYSFS_PROP_OVP_IC_SHUTDOWN,
	PDIC_SYSFS_PROP_HMD_POWER,
	PDIC_SYSFS_PROP_CABLE_CONNECT_CTRL,
#if defined(CONFIG_SEC_FACTORY)
	PDIC_SYSFS_PROP_15MODE_WATERTEST_TYPE,
#endif
	PDIC_SYSFS_PROP_MAX_COUNT,
};
#define DRIVER_VER		"1.2VER"

#define MAX77775_IRQSRC_CHG	(1 << 0)
#define MAX77775_IRQSRC_FG      (1 << 2)
#define MAX77775_IRQSRC_MUIC	(1 << 3)

struct max77775_usbc_platform_data *g_usbc_data;

#ifdef MAX77775_SYS_FW_UPDATE
#define MAXIM_DEFAULT_FW		"secure_max77775.bin"
#define MAXIM_SPU_FW			"/pdic/pdic_fw.bin"

struct pdic_fw_update {
	char id[10];
	char path[50];
	int need_verfy;
	int enforce_do;
};
#endif

#define OPCODE_RESET_COUNT 5

static void max77775_usbc_mask_irq(struct max77775_usbc_platform_data *usbc_data);
static void max77775_usbc_umask_irq(struct max77775_usbc_platform_data *usbc_data);
static void max77775_get_version_info(struct max77775_usbc_platform_data *usbc_data);
static void max77775_reset_ic(struct max77775_usbc_platform_data *usbc_data);
#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
static void max77775_usbpd_sbu_switch_control(void *data, int on);
#endif

int max77775_current_pr_state(struct max77775_usbc_platform_data *usbc_data)
{
	int current_pr = usbc_data->cc_data->current_pr;
	return current_pr;

}

void max77775_blocking_auto_vbus_control(int enable)
{
	int current_pr = 0;

	msg_maxim("disable : %d", enable);

	if (enable) {
		current_pr = max77775_current_pr_state(g_usbc_data);
		switch (current_pr) {
		case SRC:
			/* turn off the vbus */
			max77775_vbus_turn_on_ctrl(g_usbc_data, OFF, false);
			break;
		default:
			break;
		}
		g_usbc_data->mpsm_mode = MPSM_ON;
	} else {
		current_pr = max77775_current_pr_state(g_usbc_data);
		switch (current_pr) {
		case SRC:
			max77775_vbus_turn_on_ctrl(g_usbc_data, ON, false);
			break;
		default:
			break;

		}
		g_usbc_data->mpsm_mode = MPSM_OFF;
	}
	msg_maxim("current_pr : %x disable : %x", current_pr, enable);
}
EXPORT_SYMBOL(max77775_blocking_auto_vbus_control);

static void vbus_control_hard_reset(struct work_struct *work)
{
	struct max77775_usbc_platform_data *usbpd_data = g_usbc_data;

	msg_maxim("current_pr=%d", usbpd_data->cc_data->current_pr);

	if (usbpd_data->cc_data->current_pr == SRC)
		max77775_vbus_turn_on_ctrl(usbpd_data, ON, false);
}

void max77775_usbc_enable_auto_vbus(struct max77775_usbc_platform_data *usbc_data)
{
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_SAMSUNG_FACTORY_TEST;
	write_data.write_data[0] = 0x2;
	write_data.write_length = 0x1;
	write_data.read_length = 0x1;
	max77775_usbc_opcode_write(usbc_data, &write_data);
	msg_maxim("TURN ON THE AUTO VBUS");
	usbc_data->auto_vbus_en = true;
}

void max77775_usbc_disable_auto_vbus(struct max77775_usbc_platform_data *usbc_data)
{
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_SAMSUNG_FACTORY_TEST;
	write_data.write_data[0] = 0x0;
	write_data.write_length = 0x1;
	write_data.read_length = 0x1;
	max77775_usbc_opcode_write(usbc_data, &write_data);
	msg_maxim("TURN OFF THE AUTO VBUS");
	usbc_data->auto_vbus_en = false;
}

void max77775_usbc_enable_audio(struct max77775_usbc_platform_data *usbc_data)
{
	usbc_cmd_data write_data;

	/* we need new function for BIT_CCSrcDbgEn */
	usbc_data->op_ctrl1_w |= (BIT_CCVcnEn | BIT_CCTrySnkEn | BIT_CCSrcDbgEn | BIT_CCAudEn);

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_CCCTRL1_W;
	write_data.write_data[0] = usbc_data->op_ctrl1_w;
	write_data.write_length = 0x1;
	write_data.read_length = 0x1;
	max77775_usbc_opcode_write(usbc_data, &write_data);
	msg_maxim("Enable Audio Detect(0x%02X)", usbc_data->op_ctrl1_w);
}

static void max77775_send_role_swap_message(struct max77775_usbc_platform_data *usbpd_data, u8 mode)
{
	usbc_cmd_data write_data;

	max77775_usbc_clear_queue(usbpd_data);
	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_SWAP_REQUEST;
	/* 0x1 : DR_SWAP, 0x2 : PR_SWAP, 0x4: Manual Role Swap */
	write_data.write_data[0] = mode;
	write_data.write_length = 0x1;
	write_data.read_length = 0x1;
	max77775_usbc_opcode_write(usbpd_data, &write_data);
}

void max77775_rprd_mode_change(struct max77775_usbc_platform_data *usbpd_data, u8 mode)
{
	msg_maxim("mode = 0x%x", mode);

	switch (mode) {
	case TYPE_C_ATTACH_DFP:
	case TYPE_C_ATTACH_UFP:
		max77775_send_role_swap_message(usbpd_data, MANUAL_ROLE_SWAP);
		msleep(1000);
		break;
	default:
		break;
	};
}

void max77775_power_role_change(struct max77775_usbc_platform_data *usbpd_data, int power_role)
{
	msg_maxim("power_role = 0x%x", power_role);

	switch (power_role) {
	case TYPE_C_ATTACH_SRC:
	case TYPE_C_ATTACH_SNK:
		max77775_send_role_swap_message(usbpd_data, POWER_ROLE_SWAP);
		break;
	};
}

void max77775_data_role_change(struct max77775_usbc_platform_data *usbpd_data, int data_role)
{
	msg_maxim("data_role = 0x%x", data_role);

	switch (data_role) {
	case TYPE_C_ATTACH_DFP:
	case TYPE_C_ATTACH_UFP:
		max77775_send_role_swap_message(usbpd_data, DATA_ROLE_SWAP);
		break;
	};
}

static int max77775_dr_set(struct typec_port *port, enum typec_data_role role)
{
	struct max77775_usbc_platform_data *usbpd_data = typec_get_drvdata(port);
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif /* CONFIG_USB_HW_PARAM */

	if (!usbpd_data)
		return -EINVAL;
	msg_maxim("typec_power_role=%d, typec_data_role=%d, role=%d",
		usbpd_data->typec_power_role, usbpd_data->typec_data_role, role);

	if (usbpd_data->typec_data_role != TYPEC_DEVICE
		&& usbpd_data->typec_data_role != TYPEC_HOST)
		return -EPERM;
	else if (usbpd_data->typec_data_role == role)
		return -EPERM;

	reinit_completion(&usbpd_data->typec_reverse_completion);
	if (role == TYPEC_DEVICE) {
		msg_maxim("try reversing, from DFP to UFP");
		usbpd_data->typec_try_state_change = TRY_ROLE_SWAP_DR;
		max77775_data_role_change(usbpd_data, TYPE_C_ATTACH_UFP);
	} else if (role == TYPEC_HOST) {
		msg_maxim("try reversing, from UFP to DFP");
		usbpd_data->typec_try_state_change = TRY_ROLE_SWAP_DR;
		max77775_data_role_change(usbpd_data, TYPE_C_ATTACH_DFP);
	} else {
		msg_maxim("invalid typec_role");
		return -EIO;
	}
	if (!wait_for_completion_timeout(&usbpd_data->typec_reverse_completion,
				msecs_to_jiffies(TRY_ROLE_SWAP_WAIT_MS))) {
		usbpd_data->typec_try_state_change = TRY_ROLE_SWAP_NONE;
		return -ETIMEDOUT;
	}
#if defined(CONFIG_USB_HW_PARAM)
	if (o_notify)
		inc_hw_param(o_notify, USB_CCIC_DR_SWAP_COUNT);
#endif /* CONFIG_USB_HW_PARAM */
	return 0;
}

static int max77775_pr_set(struct typec_port *port, enum typec_role role)
{
	struct max77775_usbc_platform_data *usbpd_data = typec_get_drvdata(port);
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif /* CONFIG_USB_HW_PARAM */

	if (!usbpd_data)
		return -EINVAL;

	msg_maxim("typec_power_role=%d, typec_data_role=%d, role=%d",
		usbpd_data->typec_power_role, usbpd_data->typec_data_role, role);

	if (usbpd_data->typec_power_role != TYPEC_SINK
	    && usbpd_data->typec_power_role != TYPEC_SOURCE)
		return -EPERM;
	else if (usbpd_data->typec_power_role == role)
		return -EPERM;

	reinit_completion(&usbpd_data->typec_reverse_completion);
	if (role == TYPEC_SINK) {
		msg_maxim("try reversing, from Source to Sink");
		usbpd_data->typec_try_state_change = TRY_ROLE_SWAP_PR;
		max77775_power_role_change(usbpd_data, TYPE_C_ATTACH_SNK);
	} else if (role == TYPEC_SOURCE) {
		msg_maxim("try reversing, from Sink to Source");
		usbpd_data->typec_try_state_change = TRY_ROLE_SWAP_PR;
		max77775_power_role_change(usbpd_data, TYPE_C_ATTACH_SRC);
	} else {
		msg_maxim("invalid typec_role");
		return -EIO;
	}
	if (!wait_for_completion_timeout(&usbpd_data->typec_reverse_completion,
				msecs_to_jiffies(TRY_ROLE_SWAP_WAIT_MS))) {
		usbpd_data->typec_try_state_change = TRY_ROLE_SWAP_NONE;
		if (usbpd_data->typec_power_role != role)
			return -ETIMEDOUT;
	}
#if defined(CONFIG_USB_HW_PARAM)
	if (o_notify)
		inc_hw_param(o_notify, USB_CCIC_PR_SWAP_COUNT);
#endif
	return 0;
}

static int max77775_port_type_set(struct typec_port *port, enum typec_port_type port_type)
{
	struct max77775_usbc_platform_data *usbpd_data = typec_get_drvdata(port);

	if (!usbpd_data)
		return -EINVAL;

	msg_maxim("typec_power_role=%d, typec_data_role=%d, port_type=%d",
		usbpd_data->typec_power_role, usbpd_data->typec_data_role, port_type);

	reinit_completion(&usbpd_data->typec_reverse_completion);
	if (port_type == TYPEC_PORT_DFP) {
		msg_maxim("try reversing, from UFP(Sink) to DFP(Source)");
		usbpd_data->typec_try_state_change = TRY_ROLE_SWAP_TYPE;
		max77775_rprd_mode_change(usbpd_data, TYPE_C_ATTACH_DFP);
	} else if (port_type == TYPEC_PORT_UFP) {
		msg_maxim("try reversing, from DFP(Source) to UFP(Sink)");
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
		max77775_ccic_event_work(usbpd_data,
			PDIC_NOTIFY_DEV_MUIC, PDIC_NOTIFY_ID_ATTACH,
			0/*attach*/, 0/*rprd*/, 0);
#endif
		usbpd_data->typec_try_state_change = TRY_ROLE_SWAP_TYPE;
		max77775_rprd_mode_change(usbpd_data, TYPE_C_ATTACH_UFP);
	} else {
		msg_maxim("invalid typec_role");
		return 0;
	}

	if (!wait_for_completion_timeout(&usbpd_data->typec_reverse_completion,
				msecs_to_jiffies(TRY_ROLE_SWAP_WAIT_MS))) {
		usbpd_data->typec_try_state_change = TRY_ROLE_SWAP_NONE;
		return -ETIMEDOUT;
	}
	return 0;
}

static const struct typec_operations max77775_ops = {
	.dr_set = max77775_dr_set,
	.pr_set = max77775_pr_set,
	.port_type_set = max77775_port_type_set
};

int max77775_get_pd_support(struct max77775_usbc_platform_data *usbc_data)
{
	bool support_pd_role_swap = false;
	struct device_node *np = NULL;

	np = of_find_compatible_node(NULL, NULL, "maxim,max77775_pdic");

	if (np)
		support_pd_role_swap = of_property_read_bool(np, "support_pd_role_swap");
	else
		msg_maxim("np is null");

	msg_maxim("TYPEC_CLASS: support_pd_role_swap is %d, usbc_data->pd_support : %d",
		support_pd_role_swap, usbc_data->pd_support);

	if (support_pd_role_swap && usbc_data->pd_support)
		return TYPEC_PWR_MODE_PD;

	return usbc_data->pwr_opmode;
}

#if defined(MAX77775_SYS_FW_UPDATE)
static int max77775_firmware_update_sys(struct max77775_usbc_platform_data *data, int fw_dir)
{
	struct max77775_usbc_platform_data *usbc_data = data;
	max77775_fw_header *fw_header;
	const struct firmware *fw_entry;
	int fw_size, ret = 0;
#if IS_ENABLED(CONFIG_SPU_VERIFY)
	long fw_verify_size;
#endif
	struct pdic_fw_update fwup[FWUP_CMD_MAX] = {
		{"BUILT_IN", "", 0, 1},
		{"UMS", MAXIM_DEFAULT_FW, 0, 1},
		{"SPU", MAXIM_SPU_FW, 1, 0},
		{"SPU_V", MAXIM_SPU_FW, 1, 0}
	};

	if (!usbc_data) {
		msg_maxim("usbc_data is null!!");
		return -ENODEV;
	}

	switch (fw_dir) {
#if IS_ENABLED(CONFIG_SPU_VERIFY)
	case SPU:
	case SPU_VERIFICATION:
		break;
#endif
	case BUILT_IN:
		max77775_usbc_fw_setting(usbc_data->max77775, fwup[fw_dir].enforce_do);
		return 0;
	default:
		return -EINVAL;
	}

	ret = request_firmware(&fw_entry, fwup[fw_dir].path, usbc_data->dev);
	if (ret) {
		md75_info_usb("%s: firmware is not available %d\n", __func__, ret);
		return ret;
	}

	fw_size = (int)fw_entry->size;
	fw_header = (max77775_fw_header *)fw_entry->data;
	msg_maxim("Req fw %02X.%02X, length=%lu", fw_header->major, fw_header->minor, fw_entry->size);

#if IS_ENABLED(CONFIG_SPU_VERIFY)
	if (fwup[fw_dir].need_verfy) {
		fw_size = (int)fw_entry->size - SPU_METADATA_SIZE(PDIC);
		fw_verify_size = spu_firmware_signature_verify("PDIC", fw_entry->data, fw_entry->size);
		if (fw_verify_size != fw_size) {
			md75_info_usb("%s: signature verify failed, verify_ret:%ld, ori_size:%d\n",
							__func__, fw_verify_size, fw_size);
			ret = -EPERM;
			goto out;
		}
		if (fw_dir == SPU_VERIFICATION)
			goto out;
	}
#endif

	switch (usbc_data->max77775->pmic_rev) {
	case MAX77775_PASS1:
	case MAX77775_PASS2:
	case MAX77775_PASS3:
	case MAX77775_PASS4:
		ret = max77775_usbc_fw_update(usbc_data->max77775, fw_entry->data,
					fw_size, fwup[fw_dir].enforce_do);
		break;
	default:
		msg_maxim("FAILED PMIC_REVISION isn't valid (pmic_rev : 0x%x)\n",
					usbc_data->max77775->pmic_rev);
		break;
	}
#if IS_ENABLED(CONFIG_SPU_VERIFY)
out:
#endif
	release_firmware(fw_entry);
	return ret;
}
#endif

static int max77775_firmware_update_misc(struct max77775_usbc_platform_data *data,
	void *fw_data, size_t fw_size)
{
	struct max77775_usbc_platform_data *usbc_data = data;
	max77775_fw_header *fw_header;
	int ret = 0;
	const u8 *fw_bin;
	size_t fw_bin_len;
	u8 pmic_rev = 0;/* pmic Rev */
	u8 fw_enable = 0;

	if (!usbc_data) {
		msg_maxim("usbc_data is null!!");
		ret = -ENOMEM;
		goto out;
	}

	pmic_rev = usbc_data->max77775->pmic_rev;

	if (fw_size > 0) {
		msg_maxim("start, size %lu Bytes", fw_size);

		fw_bin_len = fw_size;
		fw_bin = fw_data;
		fw_header = (max77775_fw_header *)fw_bin;
		usbc_data->FW_Revision = usbc_data->max77775->FW_Revision;
		usbc_data->FW_Minor_Revision = usbc_data->max77775->FW_Minor_Revision;
		msg_maxim("chip %02X.%02X, fw %02X.%02X",
				usbc_data->FW_Revision, usbc_data->FW_Minor_Revision,
				fw_header->major, fw_header->minor);
		switch (pmic_rev) {
		case MAX77775_PASS1:
		case MAX77775_PASS2:
		case MAX77775_PASS3:
		case MAX77775_PASS4:
			fw_enable = 1;
			break;
		default:
			msg_maxim("FAILED F/W via SYS and PMIC_REVISION isn't valid");
			break;
		};

		if (fw_enable)
			ret = max77775_usbc_fw_update(usbc_data->max77775, fw_bin, (int)fw_bin_len, 1);
		else
			msg_maxim("FAILED F/W MISMATCH pmic_rev : 0x%x, fw_header->major : 0x%x",
					pmic_rev, fw_header->major);
	}
out:
	return ret;
}

void max77775_get_jig_on(struct max77775_usbc_platform_data *usbpd_data, unsigned char *data)
{
	int jigon;

	jigon = data[1];
	msg_maxim("jigon=%s", jigon ? "High" : "Low");

	complete(&usbpd_data->ccic_sysfs_completion);
}

void max77775_set_jig_on(struct max77775_usbc_platform_data *usbpd_data, int jigon)
{
	usbc_cmd_data read_data;
	usbc_cmd_data write_data;
	u8 chgin_dtls_00;
	bool chgin_valid = false;

	msg_maxim("jigon=%s", jigon ? "High" : "Low");

	max77775_read_reg(usbpd_data->max77775->charger, MAX77775_CHG_REG_DETAILS_00, &chgin_dtls_00);
	if ((chgin_dtls_00 & 0x60) == 0x60)
		chgin_valid = true;
	md75_info_usb("%s : chgin_valid = %d (%s)\n", __func__, chgin_dtls_00 & 0x60,
			chgin_valid ? "Valid" : "Invalid");

	if (!chgin_valid) {
		md75_err_usb("%s: CHGIN is not valid, return\n", __func__);
		return;
	}

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_CONTROL_JIG_W;
	write_data.write_length = 0x1;
	write_data.read_length = 0x0;
	if (jigon)
		write_data.write_data[0] = 0x1;
	else
		write_data.write_data[0] = 0x0;
	max77775_usbc_opcode_write(usbpd_data, &write_data);

	init_usbc_cmd_data(&read_data);
	read_data.opcode = OPCODE_CONTROL_JIG_R;
	read_data.write_length = 0x0;
	read_data.read_length = 0x01;
	max77775_usbc_opcode_read(usbpd_data, &read_data);
}

#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
void max77775_set_moisture_cc_open(struct max77775_usbc_platform_data *usbpd_data)
{
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_MOISTURE_CC_OPEN;
	write_data.write_length = 0x1;
	write_data.read_length = 0x0;
	write_data.write_data[0] = 0x1;
	max77775_usbc_opcode_write(usbpd_data, &write_data);
}
#endif
void max77775_control_option_command(struct max77775_usbc_platform_data *usbpd_data, int cmd)
{
	struct max77775_cc_data *cc_data = usbpd_data->cc_data;
	u8 ccstat = 0;
	usbc_cmd_data write_data;

	/* for maxim request : they want to check ccstate here */
	max77775_read_reg(usbpd_data->muic, REG_CC_STATUS1, &cc_data->cc_status1);
	ccstat =  (cc_data->cc_status1 & BIT_CCStat) >> FFS(BIT_CCStat);
	msg_maxim("usb: cmd=0x%x ccstat : %d", cmd, ccstat);

	init_usbc_cmd_data(&write_data);
	/* 1 : Vconn control option command ON */
	/* 2 : Vconn control option command OFF */
	/* 3 : Water Detect option command ON */
	/* 4 : Water Detect option command OFF */
	if (cmd == 1)
		usbpd_data->vconn_test = 0;
	else if (cmd == 2)
		usbpd_data->vconn_test = 0; /* do nothing */
	else if (cmd == 3) { /* if SBU pin is low, water interrupt is happened. */
		write_data.opcode = OPCODE_SAMSUNG_FACTORY_TEST;
		write_data.write_data[0] = 0x1;
		write_data.write_length = 0x1;
		write_data.read_length = 0x0;
		max77775_usbc_opcode_write(usbpd_data, &write_data);
	} else if (cmd == 4) {
		write_data.opcode = OPCODE_SAMSUNG_FACTORY_TEST;
		write_data.write_data[0] = 0x0;
		write_data.write_length = 0x1;
		write_data.read_length = 0x0;
		max77775_usbc_opcode_write(usbpd_data, &write_data);
	}

	if ((cmd & 0xF) == 0x3)
		usbpd_data->fac_water_enable = 1;
	else if ((cmd & 0xF) == 0x4)
		usbpd_data->fac_water_enable = 0;
}

void max77775_response_sbu_read(struct max77775_usbc_platform_data *usbpd_data, unsigned char *data)
{
	usbpd_data->sbu[0] = data[2];
	usbpd_data->sbu[1] = data[3];

	msg_maxim("opt(%d) SBU1 = 0x%x, SBU2 = 0x%x",
			  data[1], usbpd_data->sbu[0], usbpd_data->sbu[1]);
	complete(&usbpd_data->ccic_sysfs_completion);
}

void max77775_request_sbu_read(struct max77775_usbc_platform_data *usbpd_data)
{
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_READ_SBU;
	write_data.write_data[0] = 0x0;
	write_data.write_length = 0x1;
	write_data.read_length = 0x3;
	max77775_usbc_opcode_write(usbpd_data, &write_data);
}

void max77775_response_cc_read(struct max77775_usbc_platform_data *usbpd_data, unsigned char *data)
{
	u8 cc1 = 0, cc2 = 0;

	cc1 = data[1];
	cc2 = data[2];

	msg_maxim("CC1 = 0x%x, CC2 = 0x%x", cc1, cc2);

	usbpd_data->cc[0] = cc1;
	usbpd_data->cc[1] = cc2;

	complete(&usbpd_data->ccic_sysfs_completion);
}

void max77775_request_cc_read(struct max77775_usbc_platform_data *usbpd_data)
{
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_READ_CC;
	write_data.write_length = 0x0;
	write_data.read_length = 0x2;
	max77775_usbc_opcode_write(usbpd_data, &write_data);
}

void max77775_request_selftest_read(struct max77775_usbc_platform_data *usbpd_data)
{
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_READ_SBU;
	write_data.write_length = 0x1;
	write_data.write_data[0] = 0x1;
	write_data.read_length = 0x3;
	max77775_usbc_opcode_write(usbpd_data, &write_data);
}

void max77775_request_cccontrol4(struct max77775_usbc_platform_data *usbpd_data)
{
	usbc_cmd_data read_data;

	init_usbc_cmd_data(&read_data);
	read_data.opcode = OPCODE_CCCTRL4_R;
	read_data.write_length = 0x0;
	read_data.read_length = 0x1;
	max77775_usbc_opcode_read(g_usbc_data, &read_data);
}

void max77775_set_CCForceError(struct max77775_usbc_platform_data *usbpd_data)
{
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_CCCTRL3_W;
	write_data.write_data[0] = 0x84;
	write_data.write_length = 0x1;
	write_data.read_length = 0x0;
	max77775_usbc_opcode_write(usbpd_data, &write_data);
}

void max77775_set_lockerroren(struct max77775_usbc_platform_data *usbpd_data,
	unsigned char data, u8 en)
{
	usbc_cmd_data write_data;
	u8 ccctrl4_reg = data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_CCCTRL4_W;
	ccctrl4_reg &= ~(0x1 << 4);
	write_data.write_data[0] = ccctrl4_reg | ((en & 0x1) << 4);
	write_data.write_length = 0x1;
	write_data.read_length = 0x0;
	max77775_usbc_opcode_write(usbpd_data, &write_data);
}

void max77775_ccctrl4_read_complete(struct max77775_usbc_platform_data *usbpd_data,
	unsigned char *data)
{
	usbpd_data->ccctrl4_reg = data[1];
	complete(&usbpd_data->op_completion);
}

void max77775_pdic_manual_ccopen_request(int is_on)
{
	struct max77775_usbc_platform_data *usbpd_data = g_usbc_data;

	msg_maxim("is_on %d > %d", usbpd_data->cc_open_req, is_on);
	if (usbpd_data->cc_open_req != is_on) {
		usbpd_data->cc_open_req = is_on;
		schedule_work(&usbpd_data->cc_open_req_work);
	}
}
EXPORT_SYMBOL(max77775_pdic_manual_ccopen_request);

static void max77775_cc_open_work_func(
		struct work_struct *work)
{
	struct max77775_usbc_platform_data *usbc_data;
	u8 lock_err_en;
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	int event;
#endif

	usbc_data = container_of(work, struct max77775_usbc_platform_data, cc_open_req_work);
	msg_maxim("%s", usbc_data->cc_open_req ? "set":"clear");

	if (usbc_data->cc_open_req) {
		reinit_completion(&usbc_data->op_completion);
		max77775_request_cccontrol4(usbc_data);
		if (!wait_for_completion_timeout(&usbc_data->op_completion, msecs_to_jiffies(1000))) {
			msg_maxim("OPCMD COMPLETION TIMEOUT");
			return;
		}
		lock_err_en = GET_CCCTRL4_LOCK_ERROR_EN(usbc_data->ccctrl4_reg);
		msg_maxim("data: 0x%x lock_err_en=%d", usbc_data->ccctrl4_reg, lock_err_en);
		if (!lock_err_en) {
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
			event = NOTIFY_EXTRA_CCOPEN_REQ_SET;
			store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
			max77775_set_lockerroren(usbc_data, usbc_data->ccctrl4_reg, 1);
		}
		max77775_set_CCForceError(usbc_data);
	} else {
		reinit_completion(&usbc_data->op_completion);
		max77775_request_cccontrol4(usbc_data);
		if (!wait_for_completion_timeout(&usbc_data->op_completion, msecs_to_jiffies(1000)))
			msg_maxim("OPCMD COMPLETION TIMEOUT");

		lock_err_en = GET_CCCTRL4_LOCK_ERROR_EN(usbc_data->ccctrl4_reg);
		msg_maxim("data: 0x%x lock_err_en=%d", usbc_data->ccctrl4_reg, lock_err_en);
		if (lock_err_en) {
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
			event = NOTIFY_EXTRA_CCOPEN_REQ_CLEAR;
			store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
			max77775_set_lockerroren(usbc_data, usbc_data->ccctrl4_reg, 0);
		}
	}
	complete(&usbc_data->cc_open_work_completion);
}

static void max77775_dp_configure_work_func(
		struct work_struct *work)
{
	struct max77775_usbc_platform_data *usbpd_data;
#if !IS_ENABLED(CONFIG_ARCH_QCOM) || !defined(CONFIG_SEC_FACTORY)
	int timeleft = 0;
#endif

	usbpd_data = container_of(work, struct max77775_usbc_platform_data, dp_configure_work);
#if !IS_ENABLED(CONFIG_ARCH_QCOM) || !defined(CONFIG_SEC_FACTORY)
	timeleft = wait_event_interruptible_timeout(usbpd_data->device_add_wait_q,
			usbpd_data->device_add || usbpd_data->pd_data->cc_status == CC_NO_CONN, HZ/2);
	msg_maxim("%s timeleft = %d", __func__, timeleft);
#endif
	if (usbpd_data->pd_data->cc_status != CC_NO_CONN)
		max77775_ccic_event_work(usbpd_data, PDIC_NOTIFY_DEV_DP,
			PDIC_NOTIFY_ID_DP_LINK_CONF, usbpd_data->dp_selected_pin, 0, 0);
}

#if defined(MAX77775_SYS_FW_UPDATE)
static int max77775_firmware_update_sysfs(struct max77775_usbc_platform_data *usbpd_data, int fw_dir)
{
	int ret = 0;
	if (usbpd_data->fw_update != 1) {
		usbpd_data->fw_update = 1;
		max77775_usbc_mask_irq(usbpd_data);
		max77775_write_reg(usbpd_data->muic, REG_PD_INT_M, 0xFF);
		max77775_write_reg(usbpd_data->muic, REG_CC_INT_M, 0xFF);
		max77775_write_reg(usbpd_data->muic, REG_UIC_INT_M, 0xFF);
		max77775_write_reg(usbpd_data->muic, REG_VDM_INT_M, 0xFF);
		max77775_write_reg(usbpd_data->muic, REG_SPARE_INT_M, 0xFF);
		ret = max77775_firmware_update_sys(usbpd_data, fw_dir);
		max77775_write_reg(usbpd_data->muic, REG_UIC_INT_M, REG_UIC_INT_M_INIT);
		max77775_write_reg(usbpd_data->muic, REG_CC_INT_M, REG_CC_INT_M_INIT);
		max77775_write_reg(usbpd_data->muic, REG_PD_INT_M, REG_PD_INT_M_INIT);
		max77775_write_reg(usbpd_data->muic, REG_VDM_INT_M, REG_VDM_INT_M_INIT);
		max77775_write_reg(usbpd_data->muic, REG_SPARE_INT_M, REG_SPARE_INT_M_INIT);
		// max77775_usbc_enable_auto_vbus(usbpd_data);
		max77775_set_enable_alternate_mode(ALTERNATE_MODE_START);
		max77775_usbc_umask_irq(usbpd_data);
		if (ret)
			usbpd_data->fw_update = 2;
		else
			usbpd_data->fw_update = 0;
			msg_maxim("Completed to try FW Update");
	} else
		msg_maxim("Failed to try FW Update");

	return ret;
}
#endif

static int max77775_firmware_update_callback(void *data,
		void *fw_data, size_t fw_size)
{
	struct max77775_usbc_platform_data *usbpd_data
		= (struct max77775_usbc_platform_data *)data;
	int ret = 0;

	if (usbpd_data->fw_update != 1) {
		usbpd_data->fw_update = 1;
		max77775_usbc_mask_irq(usbpd_data);
		max77775_write_reg(usbpd_data->muic, REG_PD_INT_M, 0xFF);
		max77775_write_reg(usbpd_data->muic, REG_CC_INT_M, 0xFF);
		max77775_write_reg(usbpd_data->muic, REG_UIC_INT_M, 0xFF);
		max77775_write_reg(usbpd_data->muic, REG_VDM_INT_M, 0xFF);
		max77775_write_reg(usbpd_data->muic, REG_SPARE_INT_M, 0xFF);
		ret = max77775_firmware_update_misc(usbpd_data, fw_data, fw_size);
		max77775_write_reg(usbpd_data->muic, REG_UIC_INT_M, REG_UIC_INT_M_INIT);
		max77775_write_reg(usbpd_data->muic, REG_CC_INT_M, REG_CC_INT_M_INIT);
		max77775_write_reg(usbpd_data->muic, REG_PD_INT_M, REG_PD_INT_M_INIT);
		max77775_write_reg(usbpd_data->muic, REG_VDM_INT_M, REG_VDM_INT_M_INIT);
		max77775_write_reg(usbpd_data->muic, REG_SPARE_INT_M, REG_SPARE_INT_M_INIT);
		// max77775_usbc_enable_auto_vbus(usbpd_data);
		max77775_set_enable_alternate_mode(ALTERNATE_MODE_START);
		max77775_usbc_umask_irq(usbpd_data);
		if (ret)
			usbpd_data->fw_update = 2;
		else
			usbpd_data->fw_update = 0;
	} else
		msg_maxim("Failed to try FW Update");

	return ret;
}

static size_t max77775_get_firmware_size(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data
		= (struct max77775_usbc_platform_data *)data;
	unsigned long ret = 0;

	ret = usbpd_data->max77775->fw_size;

	return ret;
}

#if defined(MAX77775_SYS_FW_UPDATE)
static void max77775_firmware_update_sysfs_work(struct work_struct *work)
{
	struct max77775_usbc_platform_data *usbpd_data = container_of(work,
			struct max77775_usbc_platform_data, fw_update_work);

	max77775_firmware_update_sysfs(usbpd_data, BUILT_IN);
}
#endif

int max77775_request_vsafe0v_read(struct max77775_usbc_platform_data *usbpd_data)
{
	u8  cc_status2 = 0;
	int vsafe0v = 0;

	max77775_read_reg(usbpd_data->muic, REG_CC_STATUS2, &cc_status2);

	vsafe0v = (cc_status2 & BIT_VSAFE0V) >> FFS(BIT_VSAFE0V);
	md75_info_usb("%s: ccstatus2: 0x%x  %d\n", __func__, cc_status2, vsafe0v);
	return vsafe0v;
}

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
static int max77775_sysfs_get_local_prop(struct _pdic_data_t *ppdic_data,
					enum pdic_sysfs_property prop,
					char *buf)
{
	int retval = -ENODEV, i = 0;
	u8 cur_major = 0, cur_minor = 0, src_major = 0, src_minor = 0;
	u8 cur_pid = 0, src_pid = 0;
	struct max77775_usbc_platform_data *usbpd_data =
		(struct max77775_usbc_platform_data *)ppdic_data->drv_data;

	if (!usbpd_data) {
		msg_maxim("usbpd_data is null : request prop = %d", prop);
		return -ENODEV;
	}

	switch (prop) {
	case PDIC_SYSFS_PROP_CUR_VERSION:
		retval = max77775_read_reg(usbpd_data->muic, REG_UIC_FW_REV, &cur_major);
		if (retval < 0) {
			msg_maxim("Failed to read FW_REV");
			return retval;
		}
		retval = max77775_read_reg(usbpd_data->muic, REG_UIC_FW_REV2, &cur_minor);
		if (retval < 0) {
			msg_maxim("Failed to read FW_REV2");
			return retval;
		}
		retval = max77775_read_reg(usbpd_data->muic, REG_PRODUCT_ID, &cur_pid);
		if (retval < 0) {
			msg_maxim("Failed to read PRODUCT_ID");
			return retval;
		}
		retval = sprintf(buf, "%02X.%02X\n", cur_major, cur_minor);
		msg_maxim("usb: PDIC_SYSFS_PROP_CUR_VERSION : %02X.%02X (PID%02X)",
				cur_major, cur_minor, cur_pid);
		break;
	case PDIC_SYSFS_PROP_SRC_VERSION:
		src_major = usbpd_data->max77775->FW_Revision_bin;
		src_minor = usbpd_data->max77775->FW_Minor_Revision_bin;
		src_pid = usbpd_data->max77775->FW_Product_ID_bin;
		retval = sprintf(buf, "%02X.%02X\n", src_major, src_minor);
		msg_maxim("usb: PDIC_SYSFS_PROP_SRC_VERSION : %02X.%02X (PID%02X)",
				src_major, src_minor, src_pid);
		break;
	case PDIC_SYSFS_PROP_LPM_MODE:
		retval = sprintf(buf, "%d\n", usbpd_data->manual_lpm_mode);
		msg_maxim("usb: PDIC_SYSFS_PROP_LPM_MODE : %d",
				usbpd_data->manual_lpm_mode);
		break;
	case PDIC_SYSFS_PROP_STATE:
		retval = sprintf(buf, "%d\n", usbpd_data->pd_state);
		msg_maxim("usb: PDIC_SYSFS_PROP_STATE : %d",
				usbpd_data->pd_state);
		break;
	case PDIC_SYSFS_PROP_RID:
		retval = sprintf(buf, "%d\n", usbpd_data->cur_rid);
		msg_maxim("usb: PDIC_SYSFS_PROP_RID : %d",
				usbpd_data->cur_rid);
		break;
	case PDIC_SYSFS_PROP_BOOTING_DRY:
		usbpd_data->sbu[0] = 0;
		usbpd_data->sbu[1] = 0;
		reinit_completion(&usbpd_data->ccic_sysfs_completion);
		max77775_request_selftest_read(usbpd_data);
		i = wait_for_completion_timeout(&usbpd_data->ccic_sysfs_completion, msecs_to_jiffies(1000 * 5));
		if (i == 0)
			msg_maxim("CCIC SYSFS COMPLETION TIMEOUT");
		msg_maxim("usb: PDIC_SYSFS_PROP_BOOTING_DRY timeout : %d", i);
		if (usbpd_data->sbu[0] >= 7 && usbpd_data->sbu[1]  >= 7)
			retval = sprintf(buf, "%d\n", 1);
		else
			retval = sprintf(buf, "%d\n", 0);
		break;
	case PDIC_SYSFS_PROP_FW_UPDATE_STATUS:

		retval = sprintf(buf, "%s\n", usbpd_data->fw_update == 1 ? "UPDATE" :
						usbpd_data->fw_update == 2 ? "NG" : "OK");
		msg_maxim("usb: PDIC_SYSFS_PROP_FW_UPDATE_STATUS : %s",
						usbpd_data->fw_update == 1 ? "UPDATE" :
						usbpd_data->fw_update == 2 ? "NG" : "OK");
		break;
	case PDIC_SYSFS_PROP_FW_WATER:
		retval = sprintf(buf, "%d\n", usbpd_data->current_connstat == WATER ? 1 : 0);
		msg_maxim("usb: PDIC_SYSFS_PROP_FW_WATER : %d",
				usbpd_data->current_connstat == WATER ? 1 : 0);
		break;
	case PDIC_SYSFS_PROP_ACC_DEVICE_VERSION:
		retval = sprintf(buf, "%04x\n", usbpd_data->Device_Version);
		msg_maxim("usb: PDIC_SYSFS_PROP_ACC_DEVICE_VERSION : %d",
				usbpd_data->Device_Version);
		break;
	case PDIC_SYSFS_PROP_CONTROL_GPIO:
		usbpd_data->sbu[0] = 0;
		usbpd_data->sbu[1] = 0;
		reinit_completion(&usbpd_data->ccic_sysfs_completion);
		max77775_request_sbu_read(usbpd_data);
		i = wait_for_completion_timeout(&usbpd_data->ccic_sysfs_completion, msecs_to_jiffies(200 * 5));
		if (i == 0)
			msg_maxim("CCIC SYSFS COMPLETION TIMEOUT");
		/* compare SBU1, SBU2 values after interrupt */
		msg_maxim("usb: PDIC_SYSFS_PROP_CONTROL_GPIO SBU1 = 0x%x ,SBU2 = 0x%x timeout:%d",
		usbpd_data->sbu[0], usbpd_data->sbu[1], i);
		/* return value must be 1 or 0 */
		retval = sprintf(buf, "%d %d\n", !!usbpd_data->sbu[0], !!usbpd_data->sbu[1]);
		break;
	case PDIC_SYSFS_PROP_USBPD_IDS:
		retval = sprintf(buf, "%04x:%04x\n",
				le16_to_cpu(usbpd_data->Vendor_ID),
				le16_to_cpu(usbpd_data->Product_ID));
		msg_maxim("usb: PDIC_SYSFS_USBPD_IDS : %04x:%04x",
			le16_to_cpu(usbpd_data->Vendor_ID), le16_to_cpu(usbpd_data->Product_ID));
		break;
	case PDIC_SYSFS_PROP_USBPD_TYPE:
		retval = sprintf(buf, "%d\n", usbpd_data->acc_type);
		msg_maxim("usb: PDIC_SYSFS_USBPD_TYPE : %d",
				usbpd_data->acc_type);
		break;
	case PDIC_SYSFS_PROP_CC_PIN_STATUS:
		retval = sprintf(buf, "%d\n", usbpd_data->cc_pin_status);
		msg_maxim("usb: PDIC_SYSFS_PROP_PIN_STATUS : %d",
				  usbpd_data->cc_pin_status);
		break;
	case PDIC_SYSFS_PROP_SBU_ADC:
		usbpd_data->sbu[0] = 0;
		usbpd_data->sbu[1] = 0;
		reinit_completion(&usbpd_data->ccic_sysfs_completion);
		max77775_request_selftest_read(usbpd_data);
		i = wait_for_completion_timeout(&usbpd_data->ccic_sysfs_completion, msecs_to_jiffies(1000 * 5));
		if (i == 0)
			msg_maxim("CCIC SYSFS COMPLETION TIMEOUT");
		msg_maxim("usb: PDIC_SYSFS_PROP_SBU_ADC : %d %d timeout : %d",
				usbpd_data->sbu[0], usbpd_data->sbu[1], i);
		retval = sprintf(buf, "%d %d\n", usbpd_data->sbu[0], usbpd_data->sbu[1]);
		break;
	case PDIC_SYSFS_PROP_CC_ADC:
		usbpd_data->cc[0] = 0;usbpd_data->cc[1] = 0;
		reinit_completion(&usbpd_data->ccic_sysfs_completion);
		max77775_request_cc_read(usbpd_data);
		i = wait_for_completion_timeout(&usbpd_data->ccic_sysfs_completion, msecs_to_jiffies(1000 * 5));
		if (i == 0)
			msg_maxim("CCIC SYSFS COMPLETION TIMEOUT");
		msg_maxim("usb: PDIC_SYSFS_PROP_CC_ADC : %d %d timeout : %d",
				usbpd_data->cc[0], usbpd_data->cc[1], i);
		retval = sprintf(buf, "%d %d\n", usbpd_data->cc[0], usbpd_data->cc[1]);
		break;
	case PDIC_SYSFS_PROP_VSAFE0V_STATUS:
		usbpd_data->vsafe0v_status = max77775_request_vsafe0v_read(usbpd_data);
		retval = sprintf(buf, "%d\n", usbpd_data->vsafe0v_status);
		msg_maxim("usb: PDIC_SYSFS_PROP_VSAFE0V_STATUS : %d",
				usbpd_data->vsafe0v_status);
		break;
#if defined(CONFIG_SEC_FACTORY)
	case PDIC_SYSFS_PROP_15MODE_WATERTEST_TYPE:
#if 0 //use unsupport water pid fw feature (ex CONFIG_MAX77775_FW_PID05_SUPPORT)
		retval = sprintf(buf, "unsupport\n");
#else
		retval = sprintf(buf, "uevent\n");
#endif
		md75_info_usb("%s : PDIC_SYSFS_PROP_15MODE_WATERTEST_TYPE : uevent", __func__);
		break;
#endif
	case PDIC_SYSFS_PROP_CABLE_CONNECT_CTRL:
		/* Android auto Cable Breaker
		 *  get state value of CC
		 *
		 * 0: CC connent(DRP set)
		 * 1: CC Open
		 */
		retval = sprintf(buf, "%d\n", usbpd_data->cc_open_req);
		msg_maxim("usb: %s PDIC_SYSFS_PROP_CABLE_CONNECT_CTRL %s",
				__func__, usbpd_data->cc_open_req?"CC Open":"CC Connect");
		break;

	default:
		msg_maxim("prop read not supported prop (%d)", prop);
		retval = -ENODATA;
		break;
	}

	return retval;
}

/*
 * assume that 1 HMD device has name(14),vid(4),pid(4) each, then
 * max 32 HMD devices(name,vid,pid) need 806 bytes including TAG, NUM, comba
 */
#define MAX_HMD_POWER_STORE_LEN	1024
enum {
	HMD_POWER_MON = 0,	/* monitor name field */
	HMD_POWER_VID,		/* vid field */
	HMD_POWER_PID,		/* pid field */
	HMD_POWER_FIELD_MAX,
};

/* convert VID/PID string to uint in hexadecimal */
static int _max77775_strtoint(char *tok, uint *result)
{
	int  ret = 0;

	if (!tok || !result) {
		msg_maxim("invalid arg!");
		ret = -EINVAL;
		goto end;
	}

	if (strlen(tok) == 5 && tok[4] == 0xa/*LF*/) {
		/* continue since it's ended with line feed */
	} else if (strlen(tok) != 4) {
		msg_maxim("%s should have 4 len, but %lu!", tok, strlen(tok));
		ret = -EINVAL;
		goto end;
	}

	ret = kstrtouint(tok, 16, result);
	if (ret) {
		msg_maxim("fail to convert %s! ret:%d", tok, ret);
		goto end;
	}
end:
	return ret;
}

int max77775_store_hmd_dev(struct max77775_usbc_platform_data *usbc_data, char *str, size_t len, int num_hmd)
{
	struct max77775_hmd_power_dev *hmd_list;
	char *tok;
	int  i, j, ret = 0, rmdr;
	uint value;

	if (num_hmd <= 0 || num_hmd > MAX_NUM_HMD) {
		msg_maxim("invalid num_hmd! %d", num_hmd);
		ret = -EINVAL;
		goto end;
	}

	hmd_list = usbc_data->hmd_list;
	if (!hmd_list) {
		msg_maxim("hmd_list is null!");
		ret = -ENOMEM;
		goto end;
	}

	msg_maxim("+++ %s, %lu, %d", str, len, num_hmd);

	/* reset */
	for (i = 0; i < MAX_NUM_HMD; i++) {
		memset(hmd_list[i].hmd_name, 0, NAME_LEN_HMD);
		hmd_list[i].vid  = 0;
		hmd_list[i].pid = 0;
	}

	tok = strsep(&str, ",");
	i = 0, j = 0;
	while (tok != NULL && *tok != 0xa/*LF*/) {
		if (i >= num_hmd * HMD_POWER_FIELD_MAX) {
			msg_maxim("num of tok cannot exceed <%dx%d>!",
				num_hmd, HMD_POWER_FIELD_MAX);
			break;
		}
		if (j >= MAX_NUM_HMD) {
			msg_maxim("num of HMD cannot exceed %d!",
				MAX_NUM_HMD);
			break;
		}

		rmdr = i % HMD_POWER_FIELD_MAX;

		switch (rmdr) {
		case HMD_POWER_MON:
			strlcpy(hmd_list[j].hmd_name, tok, NAME_LEN_HMD);
			break;

		case HMD_POWER_VID:
		case HMD_POWER_PID:
			ret = _max77775_strtoint(tok, &value);
			if (ret)
				goto end;

			if (rmdr == HMD_POWER_VID) {
				hmd_list[j].vid  = value;
			} else {
				hmd_list[j].pid = value;
				j++;	/* move next */
			}
			break;
		}

		tok = strsep(&str, ",");
		i++;
	}
	for (i = 0; i < MAX_NUM_HMD; i++) {
		if (strlen(hmd_list[i].hmd_name) > 0)
			msg_maxim("%s,0x%04x,0x%04x",
				hmd_list[i].hmd_name,
				hmd_list[i].vid,
				hmd_list[i].pid);
	}

end:
	return ret;
}

#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
static void max77775_control_gpio_for_sbu(int onoff)
{
	struct otg_notify *o_notify = get_otg_notify();
	struct usb_notifier_platform_data *pdata = get_notify_data(o_notify);

	if (o_notify && o_notify->set_ldo_onoff)
		o_notify->set_ldo_onoff(pdata, onoff);
}
#endif

static ssize_t max77775_sysfs_set_prop(struct _pdic_data_t *ppdic_data,
				    enum pdic_sysfs_property prop,
				    const char *buf, size_t size)
{
	ssize_t retval = size;
	int mode = 0;
	int ret = 0;
	struct max77775_usbc_platform_data *usbpd_data =
		(struct max77775_usbc_platform_data *)ppdic_data->drv_data;
	int rv, len;
	char str[MAX_HMD_POWER_STORE_LEN] = {0,}, *p, *tok;
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
	struct otg_notify *o_notify = get_otg_notify();
#endif

	if (!usbpd_data) {
		msg_maxim("usbpd_data is null : request prop = %d", prop);
		return -ENODEV;
	}
	switch (prop) {
	case PDIC_SYSFS_PROP_LPM_MODE:
		rv = sscanf(buf, "%d", &mode);
		msg_maxim("usb: PDIC_SYSFS_PROP_LPM_MODE mode=%d", mode);
		switch (mode) {
		case 0:
			max77775_set_jig_on(usbpd_data, 0);
			usbpd_data->manual_lpm_mode = 0;
			break;
		case 1:
		case 2:
			max77775_set_jig_on(usbpd_data, 1);
			usbpd_data->manual_lpm_mode = 1;
			break;
		default:
			max77775_set_jig_on(usbpd_data, 0);
			usbpd_data->manual_lpm_mode = 0;
			break;
			}
		break;
	case PDIC_SYSFS_PROP_CTRL_OPTION:
		rv = sscanf(buf, "%d", &mode);
		msg_maxim("usb: PDIC_SYSFS_PROP_CTRL_OPTION mode=%d", mode);
		max77775_control_option_command(usbpd_data, mode);
		break;
	case PDIC_SYSFS_PROP_FW_UPDATE:
		rv = sscanf(buf, "%d", &mode);
		msg_maxim("PDIC_SYSFS_PROP_FW_UPDATE mode=%d", mode);

#ifdef MAX77775_SYS_FW_UPDATE
		md75_info_usb("%s before : FW_REV %02X.%02X\n", __func__,
				usbpd_data->max77775->FW_Revision,
				usbpd_data->max77775->FW_Minor_Revision);

		/* Factory cmd for firmware update
		* argument represent what is source of firmware like below.
		*
		* 0 : [BUILT_IN] Getting firmware from source.
		* 1 : [UMS] Getting firmware from sd card.
		* 2 : [SPU] Getting firmware from SPU APP.
		*/
		switch (mode) {
		case BUILT_IN:
			schedule_work(&usbpd_data->fw_update_work);
			break;
		case UMS:
		case SPU:
		case SPU_VERIFICATION:
			ret = max77775_firmware_update_sysfs(usbpd_data, mode);
			break;
		default:
			ret = -EINVAL;
			msg_maxim("Not support command[%d]", mode);
			break;
		}
		if (ret < 0) {
			msg_maxim("Failed to update FW");
			return ret;
		}

		max77775_get_version_info(usbpd_data);
#else
		return -EINVAL;
#endif
	    break;
	case PDIC_SYSFS_PROP_DEX_FAN_UVDM:
		rv = sscanf(buf, "%d", &mode);
		msg_maxim("PDIC_SYSFS_PROP_DEX_FAN_UVDM mode=%d", mode);
		max77775_send_dex_fan_unstructured_vdm_message(usbpd_data, mode);
		break;
	case PDIC_SYSFS_PROP_DEBUG_OPCODE:
		rv = sscanf(buf, "%d", &mode);
		msg_maxim("PDIC_SYSFS_PROP_DEBUG_OPCODE mode=%d", mode);
		break;
	case PDIC_SYSFS_PROP_CONTROL_GPIO:
		rv = sscanf(buf, "%d", &mode);
		msg_maxim("PDIC_SYSFS_PROP_CONTROL_GPIO mode=%d. do nothing for control gpio.", mode);
		/* orignal concept : mode 0 : SBU1/SBU2 set as open-drain status
		 *                         mode 1 : SBU1/SBU2 set as default status - Pull up
		 *  But, max77775 is always open-drain status so we don't need to control it.
		 */
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
		max77775_control_gpio_for_sbu(!mode);
#endif
		break;
	case PDIC_SYSFS_PROP_OVP_IC_SHUTDOWN:
		rv = sscanf(buf, "%d", &mode);
		msg_maxim("PDIC_SYSFS_PROP_OVP_IC_SHUTDOWN mode=%d", mode);
		if (ret)
			return -ENODATA;
		break;
	case PDIC_SYSFS_PROP_HMD_POWER:
		if (size >= MAX_HMD_POWER_STORE_LEN) {
			msg_maxim("too long args! %lu", size);
			return -EOVERFLOW;
		}
		mutex_lock(&usbpd_data->hmd_power_lock);
		memcpy(str, buf, size);
		p	= str;
		tok = strsep(&p, ",");
		if (tok) {
			len = strlen(tok);
			msg_maxim("tok: %s, len: %d", tok, len);
		} else {
			len = 0;
			msg_maxim("tok: len: 0");
		}

		if (!strncmp(TAG_HMD, tok, len)) {
			/* called by HmtManager to inform list of supported HMD devices
			 *
			 * Format :
			 *	 HMD,NUM,NAME01,VID01,PID01,NAME02,VID02,PID02,...
			 *
			 *	 HMD  : tag
			 *	 NUM  : num of HMD dev ..... max 2 bytes to decimal (max 32)
			 *	 NAME : name of HMD ...... max 14 bytes, char string
			 *	 VID  : vendor	id ....... 4 bytes to hexadecimal
			 *	 PID  : product id ....... 4 bytes to hexadecimal
			 *
			 * ex) HMD,2,PicoVR,2d40,0000,Nreal light,0486,573c
			 *
			 * call hmd store function with tag(HMD),NUM removed
			 */
			int num_hmd = 0, sz = 0;

			tok = strsep(&p, ",");
			if (tok) {
				sz = strlen(tok);
				ret = kstrtouint(tok, 10, &num_hmd);
			}

			msg_maxim("HMD num: %d, sz:%d", num_hmd, sz);

			max77775_store_hmd_dev(usbpd_data, str + (len + sz + 2), size - (len + sz + 2),
				num_hmd);

			if (usbpd_data->acc_type == PDIC_DOCK_NEW && max77775_check_hmd_dev(usbpd_data)) {
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
				if (o_notify)
					send_otg_notify(o_notify, NOTIFY_EVENT_HMD_EXT_CURRENT, 1);
#endif
			}
			mutex_unlock(&usbpd_data->hmd_power_lock);
			return size;
		}
		mutex_unlock(&usbpd_data->hmd_power_lock);
		break;
	case PDIC_SYSFS_PROP_CABLE_CONNECT_CTRL:
		/* Android auto Cable Breaker
		 * CC attach/ detach control
		 *
		 * 0: CC connent(DRP set)
		 * 1: CC Open
		 */
		rv = sscanf(buf, "%d", &mode);
		msg_maxim("usb: %s PDIC_SYSFS_PROP_CABLE_CONNECT_CTRL %s",
				__func__, mode?"CC Open":"CC Connect");
		reinit_completion(&usbpd_data->cc_open_work_completion);

		switch (mode) {
		case 0:
			/* cc connect */
			max77775_pdic_manual_ccopen_request(mode);
			break;
		case 1:
			/* cc open */
			max77775_pdic_manual_ccopen_request(mode);
			ret = wait_for_completion_timeout(&usbpd_data->cc_open_work_completion,
												msecs_to_jiffies(200));
			if (!ret)
				msg_maxim("cc_open_work opcmd (cc open) timeout");

			/* send usb detach event */
			/* USB */
			max77775_ccic_event_work(usbpd_data,
				PDIC_NOTIFY_DEV_USB, PDIC_NOTIFY_ID_USB,
				0/*attach*/, USB_STATUS_NOTIFY_DETACH/*drp*/, 0);
			break;
		default:
			break;

		}
		break;
	default:
		md75_info_usb("%s prop write not supported prop (%d)\n", __func__, prop);
		retval = -ENODATA;
		return retval;
	}
	return size;
}

static int max77775_sysfs_is_writeable(struct _pdic_data_t *ppdic_data,
				    enum pdic_sysfs_property prop)
{
	switch (prop) {
	case PDIC_SYSFS_PROP_LPM_MODE:
	case PDIC_SYSFS_PROP_CTRL_OPTION:
	case PDIC_SYSFS_PROP_DEBUG_OPCODE:
	case PDIC_SYSFS_PROP_CONTROL_GPIO:
	case PDIC_SYSFS_PROP_CABLE_CONNECT_CTRL:
		return 1;
	default:
		return 0;
	}
}

static int max77775_sysfs_is_writeonly(struct _pdic_data_t *ppdic_data,
				    enum pdic_sysfs_property prop)
{
	switch (prop) {
	case PDIC_SYSFS_PROP_FW_UPDATE:
	case PDIC_SYSFS_PROP_DEX_FAN_UVDM:
	case PDIC_SYSFS_PROP_OVP_IC_SHUTDOWN:
	case PDIC_SYSFS_PROP_HMD_POWER:
		return 1;
	default:
		return 0;
	}
}
#endif
static ssize_t max77775_fw_update(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int start_fw_update = 0;
	usbc_cmd_data read_data;
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&read_data);
	init_usbc_cmd_data(&write_data);
	read_data.opcode = OPCODE_CTRL1_R;
	read_data.write_length = 0x0;
	read_data.read_length = 0x1;

	write_data.opcode = OPCODE_CTRL1_W;
	write_data.write_data[0] = 0x09;
	write_data.write_length = 0x1;
	write_data.read_length = 0x0;

	if (kstrtou32(buf, 0, &start_fw_update)) {
		dev_err(dev,
			"%s: Failed converting from str to u32.", __func__);
	}

	msg_maxim("start_fw_update %d", start_fw_update);

	max77775_usbc_opcode_rw(g_usbc_data, &read_data, &write_data);

	switch (start_fw_update) {
	case 1:
		max77775_usbc_opcode_rw(g_usbc_data, &read_data, &write_data);
		break;
	case 2:
		max77775_usbc_opcode_read(g_usbc_data, &read_data);
		break;
	case 3:
		max77775_usbc_opcode_write(g_usbc_data, &write_data);

		break;
	default:
		break;
	}
	return size;
}
static DEVICE_ATTR(fw_update, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, max77775_fw_update);

static struct attribute *max77775_attr[] = {
	&dev_attr_fw_update.attr,
	NULL,
};

static struct attribute_group max77775_attr_grp = {
	.attrs = max77775_attr,
};

static void max77775_get_version_info(struct max77775_usbc_platform_data *usbc_data)
{
	u8 hw_rev[4] = {0, };
	u8 sw_main[3] = {0, };
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	u8 sw_boot = 0;
#endif

	hw_rev[0] = usbc_data->max77775->pmic_rev;
	sw_main[0] = usbc_data->max77775->FW_Revision;
	sw_main[1] = usbc_data->max77775->FW_Minor_Revision;

	usbc_data->HW_Revision = hw_rev[0];
	usbc_data->FW_Revision = sw_main[0];
	usbc_data->FW_Minor_Revision = sw_main[1];

	/* H/W, Minor, Major, Boot */
	msg_maxim("HW rev is %02Xh, FW rev is %02X.%02X!",
			usbc_data->HW_Revision, usbc_data->FW_Revision, usbc_data->FW_Minor_Revision);
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	store_ccic_version(&hw_rev[0], &sw_main[0], &sw_boot);
#endif
}

static void max77775_init_opcode
		(struct max77775_usbc_platform_data *usbc_data, int reset)
{
	struct max77775_platform_data *pdata = usbc_data->max77775_data;

	max77775_usbc_disable_auto_vbus(usbc_data);
	if (pdata && pdata->support_audio)
		max77775_usbc_enable_audio(usbc_data);
	if (reset) {
		max77775_set_enable_alternate_mode(ALTERNATE_MODE_START);
		max77775_muic_enable_detecting_short(usbc_data->muic_data);
	}

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
	if (!is_lpcharge_pdic_param())
		max77775_set_moisture_cc_open(usbc_data);
#endif
#endif
}

static bool max77775_check_recover_opcode(u8 opcode)
{
	bool ret = false;

	switch (opcode) {
	case OPCODE_CCCTRL1_W:
	case OPCODE_SAMSUNG_FACTORY_TEST:
	case OPCODE_SET_ALTERNATEMODE:
	case OPCODE_SBU_CTRL1_W:
	case OPCODE_SAMSUNG_CCSBU_SHORT:
#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
	case OPCODE_MOISTURE_CC_OPEN:
#endif
		ret = true;
		break;
	default:
		ret = false;
		break;
	}
	return ret;
}

static void max77775_recover_opcode
		(struct max77775_usbc_platform_data *usbc_data, bool opcode_list[])
{
	int i;

	for (i = 0; i < OPCODE_NONE; i++) {
		if (opcode_list[i]) {
			msg_maxim("opcode = 0x%02x", i);
			switch (i) {
			case OPCODE_CCCTRL1_W:
				if (usbc_data->op_ctrl1_w & BIT_CCAudEn)
					max77775_usbc_enable_audio(usbc_data);
				break;
			case OPCODE_SAMSUNG_FACTORY_TEST:
				if (usbc_data->auto_vbus_en)
					max77775_usbc_enable_auto_vbus(usbc_data);
				else
					max77775_usbc_disable_auto_vbus(usbc_data);
				break;
			case OPCODE_SET_ALTERNATEMODE:
				max77775_set_enable_alternate_mode
					(usbc_data->set_altmode);
				break;
#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
			case OPCODE_SBU_CTRL1_W:
				max77775_usbpd_sbu_switch_control(usbc_data, usbc_data->sbu_switch_status);
				break;
#endif
			case OPCODE_SAMSUNG_CCSBU_SHORT:
				max77775_muic_enable_detecting_short
					(usbc_data->muic_data);
				break;
#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
			case OPCODE_MOISTURE_CC_OPEN:
				max77775_set_moisture_cc_open(usbc_data);
				break;
#endif
			default:
				break;
			}
			opcode_list[i] = false;
		}
	}
}

void init_usbc_cmd_data(usbc_cmd_data *cmd_data)
{
	cmd_data->opcode = OPCODE_NONE;
	cmd_data->prev_opcode = OPCODE_NONE;
	cmd_data->response = OPCODE_NONE;
	cmd_data->val = REG_NONE;
	cmd_data->mask = REG_NONE;
	cmd_data->reg = REG_NONE;
	cmd_data->noti_cmd = OPCODE_NOTI_NONE;
	cmd_data->write_length = 0;
	cmd_data->read_length = 0;
	cmd_data->seq = 0;
	cmd_data->is_uvdm = 0;
	memset(cmd_data->write_data, REG_NONE, OPCODE_DATA_LENGTH);
	memset(cmd_data->read_data, REG_NONE, OPCODE_DATA_LENGTH);
}

static void init_usbc_cmd_node(usbc_cmd_node *usbc_cmd_node)
{
	usbc_cmd_data *cmd_data = &(usbc_cmd_node->cmd_data);

	pr_debug("%s:%s\n", "MAX77775", __func__);

	usbc_cmd_node->next = NULL;

	init_usbc_cmd_data(cmd_data);
}

static void copy_usbc_cmd_data(usbc_cmd_data *from, usbc_cmd_data *to)
{
	to->opcode = from->opcode;
	to->response = from->response;
	memcpy(to->read_data, from->read_data, OPCODE_DATA_LENGTH);
	memcpy(to->write_data, from->write_data, OPCODE_DATA_LENGTH);
	to->reg = from->reg;
	to->mask = from->mask;
	to->val = from->val;
	to->seq = from->seq;
	to->read_length = from->read_length;
	to->write_length = from->write_length;
	to->prev_opcode = from->prev_opcode;
	to->is_uvdm = from->is_uvdm;
}

bool is_empty_usbc_cmd_queue(usbc_cmd_queue_t *usbc_cmd_queue)
{
	bool ret = false;

	if (usbc_cmd_queue->front == NULL)
		ret = true;

	if (ret)
		msg_maxim("usbc_cmd_queue Empty(%c)", ret ? 'T' : 'F');

	return ret;
}

void enqueue_usbc_cmd(usbc_cmd_queue_t *usbc_cmd_queue, usbc_cmd_data *cmd_data)
{
	usbc_cmd_node	*temp_node = kzalloc(sizeof(usbc_cmd_node), GFP_KERNEL);

	if (!temp_node) {
		msg_maxim("failed to allocate usbc command queue");
		return;
	}

	init_usbc_cmd_node(temp_node);

	copy_usbc_cmd_data(cmd_data, &(temp_node->cmd_data));

	if (is_empty_usbc_cmd_queue(usbc_cmd_queue)) {
		usbc_cmd_queue->front = temp_node;
		usbc_cmd_queue->rear = temp_node;
	} else {
		usbc_cmd_queue->rear->next = temp_node;
		usbc_cmd_queue->rear = temp_node;
	}
}

static void dequeue_usbc_cmd
	(usbc_cmd_queue_t *usbc_cmd_queue, usbc_cmd_data *cmd_data)
{
	usbc_cmd_node *temp_node;

	if (is_empty_usbc_cmd_queue(usbc_cmd_queue)) {
		msg_maxim("Queue, Empty!");
		return;
	}

	temp_node = usbc_cmd_queue->front;
	copy_usbc_cmd_data(&(temp_node->cmd_data), cmd_data);

	msg_maxim("Opcode(0x%02x) Response(0x%02x)", cmd_data->opcode, cmd_data->response);

	if (usbc_cmd_queue->front->next == NULL) {
		msg_maxim("front->next = NULL");
		usbc_cmd_queue->front = NULL;
	} else
		usbc_cmd_queue->front = usbc_cmd_queue->front->next;

	if (is_empty_usbc_cmd_queue(usbc_cmd_queue))
		usbc_cmd_queue->rear = NULL;

	kfree(temp_node);
}

static bool front_usbc_cmd
	(usbc_cmd_queue_t *cmd_queue, usbc_cmd_data *cmd_data)
{
	if (is_empty_usbc_cmd_queue(cmd_queue)) {
		msg_maxim("Queue, Empty!");
		return false;
	}

	copy_usbc_cmd_data(&(cmd_queue->front->cmd_data), cmd_data);
	msg_maxim("Opcode(0x%02x)", cmd_data->opcode);
	return true;
}

static bool is_usbc_notifier_opcode(u8 opcode)
{
	bool noti = false;

	return noti;
}

/*
 * max77775_i2c_opcode_write - SMBus "opcode write" protocol
 * @chip: max77775 platform data
 * @command: OPcode
 * @values: Byte array into which data will be read; big enough to hold
 *	the data returned by the slave.
 *
 * This executes the SMBus "opcode read" protocol, returning negative errno
 * else the number of data bytes in the slave's response.
 */
int max77775_i2c_opcode_write(struct max77775_usbc_platform_data *usbc_data,
		u8 opcode, u8 length, u8 *values)
{
	u8 write_values[OPCODE_MAX_LENGTH] = { 0, };
	int ret = 0;

	if (length > OPCODE_DATA_LENGTH)
		return -EMSGSIZE;

	write_values[0] = opcode;
	if (length)
		memcpy(&write_values[1], values, length);

#if 0
	int i = 0; // To use this, move int i to the top to avoid build error
	for (i = 0; i < length + OPCODE_SIZE; i++)
		msg_maxim("[%d], 0x[%x]", i, write_values[i]);
#else
	msg_maxim("opcode 0x%x, write_length %d",
			opcode, length + OPCODE_SIZE);
	print_hex_dump(KERN_INFO, "max77775: opcode_write: ",
			DUMP_PREFIX_OFFSET, 16, 1, write_values,
			length + OPCODE_SIZE, false);
#endif

	/* Write opcode and data */
	ret = max77775_bulk_write(usbc_data->muic, OPCODE_WRITE,
			length + OPCODE_SIZE, write_values);
	/* Write end of data by 0x00 */
	if (length < OPCODE_DATA_LENGTH)
		max77775_write_reg(usbc_data->muic, OPCODE_WRITE_END, 0x00);

	if (opcode == OPCODE_SET_ALTERNATEMODE)
		usbc_data->set_altmode_error = ret;

	if (ret == 0)
		usbc_data->opcode_stamp = jiffies;

	return ret;
}

/**
 * max77775_i2c_opcode_read - SMBus "opcode read" protocol
 * @chip: max77775 platform data
 * @command: OPcode
 * @values: Byte array into which data will be read; big enough to hold
 *	the data returned by the slave.
 *
 * This executes the SMBus "opcode read" protocol, returning negative errno
 * else the number of data bytes in the slave's response.
 */
int max77775_i2c_opcode_read(struct max77775_usbc_platform_data *usbc_data,
		u8 opcode, u8 length, u8 *values)
{
	int size = 0;

	if (length > OPCODE_DATA_LENGTH)
		return -EMSGSIZE;

	/*
	 * We don't need to use opcode to get any feedback
	 */

	/* Read opcode data */
    max77775_bulk_read(usbc_data->muic, OPCODE_READ, OPCODE_SIZE, values);
    if (length > 0)
		size = max77775_bulk_read(usbc_data->muic, OPCODE_READ + OPCODE_SIZE,
								  length, values + OPCODE_SIZE);

#if 0
	int i = 0; // To use this, move int i to the top to avoid build error
	for (i = 0; i < length + OPCODE_SIZE; i++)
		msg_maxim("[%d], 0x[%x]", i, values[i]);
#else
	msg_maxim("opcode 0x%x, read_length %d, ret_error %d",
			opcode, length + OPCODE_SIZE, size);
	print_hex_dump(KERN_INFO, "max77775: opcode_read: ",
			DUMP_PREFIX_OFFSET, 16, 1, values,
			length + OPCODE_SIZE, false);
#endif
	return size;
}

static void max77775_notify_execute(struct max77775_usbc_platform_data *usbc_data,
		const usbc_cmd_data *cmd_data)
{
		/* to do  */
}

static void max77775_handle_update_opcode(struct max77775_usbc_platform_data *usbc_data,
		const usbc_cmd_data *cmd_data, unsigned char *data)
{
	usbc_cmd_data write_data;
	u8 read_value = data[1];
	u8 write_value = (read_value & (~cmd_data->mask)) | (cmd_data->val & cmd_data->mask);
	u8 opcode = cmd_data->response + 1; /* write opcode = read opocde + 1 */

	md75_info_usb("%s: value update [0x%x]->[0x%x] at OPCODE(0x%x)\n", __func__,
			read_value, write_value, opcode);

	init_usbc_cmd_data(&write_data);
	write_data.opcode = opcode;
	write_data.write_length = 1;
	write_data.write_data[0] = write_value;
	write_data.read_length = 0;

	max77775_usbc_opcode_push(usbc_data, &write_data);
}

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
void max77775_request_response(struct max77775_usbc_platform_data *usbc_data)
{
	usbc_cmd_data value;

	init_usbc_cmd_data(&value);
	value.opcode = OPCODE_READ_RESPONSE_FOR_GET_REQUEST;
	value.read_length = 28;
	max77775_usbc_opcode_write(usbc_data, &value);

	md75_info_usb("%s : OPCODE(0x%02x) W_LENGTH(%d) R_LENGTH(%d)\n",
		__func__, value.opcode, value.write_length, value.read_length);
}
#endif

#define UNKNOWN_VID 0xFFFF
void max77775_send_get_request(struct max77775_usbc_platform_data *usbc_data, unsigned char *data)
{
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	enum {
		SENT_REQ_MSG = 0,
		ERR_SNK_RDY = 5,
		ERR_PD20,
		ERR_SNKTXNG,
	};
	SEC_PD_SINK_STATUS *snk_sts = &usbc_data->pd_data->pd_noti.sink_status;

	if (data[1] == SENT_REQ_MSG) {
	} else { /* ERROR case */
		/* Mark Error in xid */
		if (data[1] == ERR_SNK_RDY) {
			usbc_data->pd_data->sent_chg_info = false;
		}
		snk_sts->xid = (UNKNOWN_VID << 16) | (data[1] << 8);
		msg_maxim("%s, Err : %d", __func__, data[1]);
	}
#endif
}

void max77775_extend_msg_process(struct max77775_usbc_platform_data *usbc_data, unsigned char *data,
		unsigned char len)
{
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	SEC_PD_SINK_STATUS *snk_sts = &usbc_data->pd_data->pd_noti.sink_status;

	snk_sts->vid = *(unsigned short *)(data + 2);
	snk_sts->pid = *(unsigned short *)(data + 4);
	snk_sts->xid = *(unsigned int *)(data + 6);
	msg_maxim("%s, %04x, %04x, %08x",
		__func__, snk_sts->vid, snk_sts->pid, snk_sts->xid);
	if (snk_sts->fp_sec_pd_ext_cb)
		snk_sts->fp_sec_pd_ext_cb(snk_sts->vid, snk_sts->pid);
#endif
}

void max77775_read_response(struct max77775_usbc_platform_data *usbc_data, unsigned char *data)
{
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	SEC_PD_SINK_STATUS *snk_sts = &usbc_data->pd_data->pd_noti.sink_status;

	switch (data[1] >> 5) {
	case OPCODE_GET_SRC_CAP_EXT:
		max77775_extend_msg_process(usbc_data, data+2, data[1] & 0x1F);
		break;
	default:
		snk_sts->xid = (UNKNOWN_VID << 16) | data[1];
		msg_maxim("%s, Err : %d", __func__, data[1]);
		break;
	}
#endif
}

static void max77775_irq_execute(struct max77775_usbc_platform_data *usbc_data,
		const usbc_cmd_data *cmd_data)
{
	int len = cmd_data->read_length;
	unsigned char data[OPCODE_DATA_LENGTH + OPCODE_SIZE] = {0,};
	u8 response = 0xff;
	u8 vdm_opcode_header = 0x0;
	UND_DATA_MSG_VDM_HEADER_Type vdm_header;
	u8 vdm_command = 0x0;
	u8 vdm_type = 0x0;
	u8 vdm_response = 0x0;
	u8 reqd_vdm_command = 0;
	uint8_t W_DATA = 0x0;

	memset(&vdm_header, 0, sizeof(UND_DATA_MSG_VDM_HEADER_Type));
	max77775_i2c_opcode_read(usbc_data, cmd_data->response,
			len, data);

	/* opcode identifying the messsage type. (0x51)*/
	response = data[0];

	if (response != cmd_data->response) {
		msg_maxim("Response [0x%02x] != [0x%02x]",
			response, cmd_data->response);
		if (cmd_data->response == OPCODE_FW_OPCODE_CLEAR) {
			msg_maxim("Response after FW opcode cleared, just return");
			return;
		}
	}

	/* to do(read switch case) */
	switch (response) {
	case OPCODE_BCCTRL1_R:
	case OPCODE_BCCTRL2_R:
	case OPCODE_CTRL1_R:
	case OPCODE_CCCTRL1_R:
	case OPCODE_CCCTRL2_R:
	case OPCODE_CCCTRL3_R:
	case OPCODE_VCONN_ILIM_R:
		if (cmd_data->seq == OPCODE_UPDATE_SEQ)
			max77775_handle_update_opcode(usbc_data, cmd_data, data);
		break;
#if IS_ENABLED(CONFIG_HV_MUIC_MAX77775_AFC)
	case COMMAND_AFC_RESULT_READ:
	case COMMAND_QC_2_0_SET:
		max77775_muic_handle_detect_dev_hv(usbc_data->muic_data, data);
		break;
#endif
	case OPCODE_CURRENT_SRCCAP:
		max77775_current_pdo(usbc_data, data);
		break;
	case OPCODE_GET_SRCCAP:
		max77775_pdo_list(usbc_data, data);
		break;
	case OPCODE_SEND_GET_REQUEST:
		max77775_send_get_request(usbc_data, data);
		break;
	case OPCODE_READ_RESPONSE_FOR_GET_REQUEST:
		max77775_read_response(usbc_data, data);
		break;
	case OPCODE_SRCCAP_REQUEST:
		/*
		 * If response of Source_Capablities message is SinkTxNg(0xFE) or Not in Ready State(0xFF)
		 * It means that the message can not be sent to Port Partner.
		 * After Attaching Rp 3.0A, send again the message.
		 */
		if (data[1] == 0xfe || data[1] == 0xff) {
			usbc_data->srcccap_request_retry = true;
			md75_info_usb("%s : srcccap_request_retry is set\n", __func__);
		}
		break;
	case OPCODE_APDO_SRCCAP_REQUEST:
		max77775_response_apdo_request(usbc_data, data);
		break;
	case OPCODE_SET_PPS:
		max77775_response_set_pps(usbc_data, data);
		break;
	case OPCODE_SAMSUNG_READ_MESSAGE:
		md75_info_usb("@TA_ALERT: %s : OPCODE[%x] Data[1] = 0x%x Data[7] = 0x%x Data[9] = 0x%x\n",
			__func__, OPCODE_SAMSUNG_READ_MESSAGE, data[1], data[7], data[9]);
#if defined(CONFIG_DIRECT_CHARGING)
		if ((data[0] == 0x5D) &&
			/* OCP would be set to Alert or Status message */
			((data[1] == 0x01 && data[7] == 0x04) || (data[1] == 0x02 && (data[9] & 0x02)))) {
			union power_supply_propval value = {0,};
			value.intval = true;
			psy_do_property("battery", set,
				POWER_SUPPLY_EXT_PROP_DIRECT_TA_ALERT, value);
		}
#endif
		break;
	case OPCODE_VDM_DISCOVER_GET_VDM_RESP:
		max77775_vdm_message_handler(usbc_data, data, len + OPCODE_SIZE);
		break;
	case OPCODE_READ_SBU:
		max77775_response_sbu_read(usbc_data, data);
		break;
	case OPCODE_READ_CC:
		max77775_response_cc_read(usbc_data, data);
		break;
	case OPCODE_VDM_DISCOVER_SET_VDM_REQ:
		vdm_opcode_header = data[1];
		switch (vdm_opcode_header) {
		case 0xFF:
			msg_maxim("This isn't invalid response(OPCODE : 0x48, HEADER : 0xFF)");
			break;
		default:
			memcpy(&vdm_header, &data[2], sizeof(vdm_header));
			vdm_type = vdm_header.BITS.VDM_Type;
			vdm_command = vdm_header.BITS.VDM_command;
			vdm_response = vdm_header.BITS.VDM_command_type;
			msg_maxim("vdm_type[%x], vdm_command[%x], vdm_response[%x]",
				vdm_type, vdm_command, vdm_response);
			switch (vdm_type) {
			case STRUCTURED_VDM:
				if (vdm_response == SEC_UVDM_RESPONDER_ACK) {
					switch (vdm_command) {
					case Discover_Identity:
						msg_maxim("ignore Discover_Identity");
						break;
					case Discover_SVIDs:
						msg_maxim("ignore Discover_SVIDs");
						break;
					case Discover_Modes:
						/* work around. The discover mode irq is not happened */
						if (vdm_header.BITS.Standard_Vendor_ID
										== SAMSUNG_VENDOR_ID) {
							if (usbc_data->send_enter_mode_req == 0) {
								/*Samsung Enter Mode */
								msg_maxim("dex: second enter mode request");
								usbc_data->send_enter_mode_req = 1;
								max77775_set_dex_enter_mode(usbc_data);
							}
						} else
							msg_maxim("ignore Discover_Modes");
						break;
					case Enter_Mode:
						/* work around. The enter mode irq is not happened */
						if (vdm_header.BITS.Standard_Vendor_ID
										== SAMSUNG_VENDOR_ID) {
							usbc_data->is_samsung_accessory_enter_mode = 1;
							msg_maxim("dex mode enter_mode ack status!");
						} else
							msg_maxim("ignore Enter_Mode");
						break;
					case Exit_Mode:
						msg_maxim("ignore Exit_Mode");
						break;
					case Attention:
						msg_maxim("ignore Attention");
						break;
					case Configure:
						break;
					default:
						msg_maxim("vdm_command isn't valid[%x]", vdm_command);
						break;
					};
				} else if (vdm_response == SEC_UVDM_ININIATOR) {
					switch (vdm_command) {
					case Attention:
						/* Attention message is not able to be received via 0x48 OPCode */
						/* Check requested vdm command and responded vdm command */
						{
							/* Read requested vdm command */
							max77775_read_reg(usbc_data->muic, 0x23, &reqd_vdm_command);
							reqd_vdm_command &= 0x1F; /* Command bit, b4...0 */

							if (reqd_vdm_command == Configure) {
								W_DATA = 1 << (usbc_data->dp_selected_pin - 1);
								/* Retry Configure message */
								msg_maxim("Retry Configure message, W_DATA = %x, dp_selected_pin = %d",
										W_DATA, usbc_data->dp_selected_pin);
								max77775_set_dp_configure(usbc_data, W_DATA);
							}
						}
						break;
					case Discover_Identity:
					case Discover_SVIDs:
					case Discover_Modes:
					case Enter_Mode:
					case Configure:
					default:
						/* Nothing */
						break;
					};
				} else
					msg_maxim("vdm_response is error value[%x]", vdm_response);
				break;
			case UNSTRUCTURED_VDM:
				max77775_uvdm_opcode_response_handler(usbc_data, data, len + OPCODE_SIZE);
				break;
			default:
				msg_maxim("vdm_type isn't valid error");
				break;
			};
			break;
		};
		break;
	case OPCODE_SET_ALTERNATEMODE:
		usbc_data->max77775->set_altmode_en = 1;
		break;
	case OPCODE_FW_OPCODE_CLEAR:
		msg_maxim("Cleared FW OPCODE");
		break;
	case OPCODE_CCCTRL4_R:
		max77775_ccctrl4_read_complete(usbc_data, data);
		break;
	case OPCODE_CONTROL_JIG_R:
		max77775_get_jig_on(usbc_data, data);
		break;
	default:
		break;
	}
}

void max77775_usbc_dequeue_queue(struct max77775_usbc_platform_data *usbc_data)
{
	usbc_cmd_data cmd_data;
	usbc_cmd_queue_t *cmd_queue = NULL;

	cmd_queue = &(usbc_data->usbc_cmd_queue);

	init_usbc_cmd_data(&cmd_data);

	if (is_empty_usbc_cmd_queue(cmd_queue)) {
		msg_maxim("Queue, Empty");
		return;
	}

	dequeue_usbc_cmd(cmd_queue, &cmd_data);
	msg_maxim("!! Dequeue queue : opcode : %x, 1st data : %x. 2st data : %x",
		cmd_data.write_data[0],
		cmd_data.read_data[0],
		cmd_data.val);
}

static void max77775_usbc_clear_fw_queue(struct max77775_usbc_platform_data *usbc_data)
{
	usbc_cmd_data write_data;

	msg_maxim("called");

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_FW_OPCODE_CLEAR;
	max77775_usbc_opcode_write(usbc_data, &write_data);
}

void max77775_usbc_clear_queue(struct max77775_usbc_platform_data *usbc_data)
{
	usbc_cmd_data cmd_data;
	usbc_cmd_queue_t *cmd_queue = NULL;

	mutex_lock(&usbc_data->op_lock);
	msg_maxim("IN");
	cmd_queue = &(usbc_data->usbc_cmd_queue);

	while (!is_empty_usbc_cmd_queue(cmd_queue)) {
		init_usbc_cmd_data(&cmd_data);
		dequeue_usbc_cmd(cmd_queue, &cmd_data);
		if (max77775_check_recover_opcode(cmd_data.opcode)) {
			usbc_data->need_recover = true;
			usbc_data->recover_opcode_list[cmd_data.opcode] = usbc_data->need_recover;
		}
	}
	usbc_data->opcode_stamp = 0;
	msg_maxim("OUT");
	mutex_unlock(&usbc_data->op_lock);
	/* also clear fw opcode queue to sync with driver */
	max77775_usbc_clear_fw_queue(usbc_data);
}

bool max77775_is_noirq_opcode(uint8_t opcode)
{
	bool ret = false;

	switch (opcode) {
	case OPCODE_ICURR_AUTOIBUS_ON:
		ret = true;
		break;
	default:
		break;
	}

	return ret;
}

bool max77775_is_stuck_suppose_opcode(uint8_t opcode)
{
	bool ret = false;

	switch (opcode) {
	case OPCODE_APDO_SRCCAP_REQUEST:
	case OPCODE_SET_PPS:
	case OPCODE_BCCTRL1_R:
		ret = true;
		break;
	default:
		break;
	}

	return ret;
}

static int max77775_usbc_increase_opcode_fail(struct max77775_usbc_platform_data *usbc_data,
		bool enforce)
{
	const int reset_count = OPCODE_RESET_COUNT;

	usbc_data->opcode_fail_count++;
	md75_info_usb("%s: opcode_fail_count = %d enforce = %d\n", __func__,
		usbc_data->opcode_fail_count, enforce);

	if (usbc_data->opcode_fail_count == reset_count || enforce) {
#if IS_ENABLED(CONFIG_USB_HW_PARAM)
		struct otg_notify *o_notify = get_otg_notify();

		if (o_notify)
			inc_hw_param(o_notify, USB_CCIC_STUCK_COUNT);
#endif
#if IS_ENABLED(CONFIG_SEC_ABC)
		sec_abc_send_event("MODULE=pdic@INFO=ccic_stuck");
#endif
	}

	if (usbc_data->opcode_fail_count >= reset_count || enforce) {
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
		union power_supply_propval value = {0, };
#endif

		md75_info_usb("%s: STUCK. we will call uic reset.\n", __func__);
#if IS_ENABLED(CONFIG_USB_USING_ADVANCED_USBLOG)
		printk_usb(NOTIFY_PRINTK_USB_SNAPSHOT, "ic stuck.\n");
#endif

		max77775_reset_ic(usbc_data);
		usbc_data->stuck_suppose = 1;

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
		value.intval = 1;
		psy_do_property("sec-direct-charger", set,
			POWER_SUPPLY_EXT_PROP_DIRECT_CLEAR_ERR, value);
#endif
		return 1;
	}
	return 0;
}

static int max77775_usbc_handle_noirq_opcode(struct max77775_usbc_platform_data *usbc_data, usbc_cmd_data cmd_data)
{
	int ret;
	usbc_cmd_queue_t *cmd_queue = NULL;
	usbc_cmd_data cmd_data_dummy;
	unsigned char rbuf[OPCODE_DATA_LENGTH + OPCODE_SIZE] = {0,};
	int retry = 10;

	ret = max77775_i2c_opcode_write(usbc_data, cmd_data.opcode,
			cmd_data.write_length, cmd_data.write_data);
	if (ret < 0) {
		msg_maxim("i2c write fail. dequeue opcode");
		max77775_usbc_dequeue_queue(usbc_data);
		return 0;
	}

	cmd_queue = &(usbc_data->usbc_cmd_queue);
	if (is_empty_usbc_cmd_queue(cmd_queue)) {
		msg_maxim("cmd_queue is empty");
		return 0;
	}

	dequeue_usbc_cmd(cmd_queue, &cmd_data_dummy);
	if (cmd_data_dummy.response != cmd_data.opcode) {
		msg_maxim("response unmatch (response %x, opcode %x",
			cmd_data_dummy.response, cmd_data.opcode);
		if (cmd_data_dummy.opcode != OPCODE_NONE) {
			msg_maxim("re-enqueue opcode %x", cmd_data_dummy.opcode);
			enqueue_usbc_cmd(cmd_queue, &cmd_data_dummy);
			return 0;
		}
	}

	do {
		ret = max77775_i2c_opcode_read(usbc_data, cmd_data_dummy.response,
				cmd_data_dummy.read_length, rbuf);
		if (retry < 8)
			msleep(100);
	} while ((--retry > 0) && (rbuf[0] != cmd_data_dummy.response));

	if (rbuf[0] != cmd_data_dummy.response) {
		md75_err_usb("%s: NO RESPONSE for OPCODE (response==0x%02X)\n",
				__func__, rbuf[0]);
		return max77775_usbc_increase_opcode_fail(usbc_data, 0);
	} else {
		usbc_data->opcode_fail_count = 0;
		usbc_data->stuck_suppose = 0;
	}

	return 0;
}

static void max77775_usbc_cmd_run(struct max77775_usbc_platform_data *usbc_data)
{
	usbc_cmd_queue_t *cmd_queue = NULL;
	usbc_cmd_data cmd_data;
	int ret = 0;

	cmd_queue = &(usbc_data->usbc_cmd_queue);

	init_usbc_cmd_data(&cmd_data);

	if (is_empty_usbc_cmd_queue(cmd_queue)) {
		msg_maxim("Queue, Empty");
		return;
	}

	dequeue_usbc_cmd(cmd_queue, &cmd_data);

	if (is_usbc_notifier_opcode(cmd_data.opcode)) {
		max77775_notify_execute(usbc_data, &cmd_data);
		max77775_usbc_cmd_run(usbc_data);
	} else if (cmd_data.opcode == OPCODE_NONE) {/* Apcmdres isr */
		msg_maxim("Apcmdres ISR !!!");
		max77775_irq_execute(usbc_data, &cmd_data);
		usbc_data->opcode_stamp = 0;
		max77775_usbc_cmd_run(usbc_data);
	} else if (max77775_is_noirq_opcode(cmd_data.opcode)) {
		if (!max77775_usbc_handle_noirq_opcode(usbc_data, cmd_data))
			max77775_usbc_cmd_run(usbc_data);
	} else { /* No ISR */
		msg_maxim("No ISR");
		copy_usbc_cmd_data(&cmd_data, &(usbc_data->last_opcode));
		ret = max77775_i2c_opcode_write(usbc_data, cmd_data.opcode,
				cmd_data.write_length, cmd_data.write_data);
		if (ret < 0) {
			msg_maxim("i2c write fail. dequeue opcode");
			max77775_usbc_dequeue_queue(usbc_data);
		}
	}
}

int max77775_usbc_opcode_write(struct max77775_usbc_platform_data *usbc_data,
	usbc_cmd_data *write_op)
{
	struct max77775_dev *max77775 = usbc_data->max77775;
	usbc_cmd_queue_t *cmd_queue = &(usbc_data->usbc_cmd_queue);
	usbc_cmd_data execute_cmd_data;
	usbc_cmd_data current_cmd;
	bool enforce = false;

	if (usbc_data->usb_mock.opcode_write)
		return usbc_data->usb_mock.opcode_write((void *)usbc_data, write_op);

	if (max77775->suspended == true) {
		msg_maxim("max77775 in suspend state, just return");
		return -EACCES;
	}

	mutex_lock(&usbc_data->op_lock);
	init_usbc_cmd_data(&current_cmd);

	/* the messages sent to USBC. */
	init_usbc_cmd_data(&execute_cmd_data);
	execute_cmd_data.opcode = write_op->opcode;
	execute_cmd_data.write_length = write_op->write_length;
	execute_cmd_data.is_uvdm = write_op->is_uvdm;
	memcpy(execute_cmd_data.write_data, write_op->write_data, OPCODE_DATA_LENGTH);
	execute_cmd_data.seq = OPCODE_WRITE_SEQ;
	enqueue_usbc_cmd(cmd_queue, &execute_cmd_data);

	/* the messages recevied From USBC. */
	init_usbc_cmd_data(&execute_cmd_data);
	execute_cmd_data.response = write_op->opcode;
	execute_cmd_data.read_length = write_op->read_length;
	execute_cmd_data.is_uvdm = write_op->is_uvdm;
	execute_cmd_data.seq = OPCODE_WRITE_SEQ;
	enqueue_usbc_cmd(cmd_queue, &execute_cmd_data);

	msg_maxim("W->W opcode[0x%02x] write_length[%d] read_length[%d]",
		write_op->opcode, write_op->write_length, write_op->read_length);

	front_usbc_cmd(cmd_queue, &current_cmd);
	if (current_cmd.opcode == write_op->opcode)
		max77775_usbc_cmd_run(usbc_data);
	else {
		msg_maxim("!!!current_cmd.opcode [0x%02x][0x%02x], read_op->opcode[0x%02x]",
			current_cmd.opcode, current_cmd.response, write_op->opcode);
		if (usbc_data->opcode_stamp != 0 && current_cmd.opcode == OPCODE_NONE) {
			if (time_after(jiffies,
					usbc_data->opcode_stamp + MAX77775_MAX_APDCMD_TIME)) {
				usbc_data->opcode_stamp = 0;
				msg_maxim("error. we will dequeue response data");
				max77775_usbc_dequeue_queue(usbc_data);
				if (max77775_is_stuck_suppose_opcode(current_cmd.response))
					enforce = true;
				if (max77775_usbc_increase_opcode_fail(usbc_data, enforce))
					goto out;
				max77775_usbc_cmd_run(usbc_data);
			}
		}
	}
out:
	mutex_unlock(&usbc_data->op_lock);

	return 0;
}

int max77775_usbc_opcode_read(struct max77775_usbc_platform_data *usbc_data,
	usbc_cmd_data *read_op)
{
	struct max77775_dev *max77775 = usbc_data->max77775;
	usbc_cmd_queue_t *cmd_queue = &(usbc_data->usbc_cmd_queue);
	usbc_cmd_data execute_cmd_data;
	usbc_cmd_data current_cmd;
	bool enforce = false;

	if (usbc_data->usb_mock.opcode_read)
		return usbc_data->usb_mock.opcode_read((void *)usbc_data, read_op);

	if (max77775->suspended == true) {
		msg_maxim("max77775 in suspend state, just return");
		return -EACCES;
	}

	mutex_lock(&usbc_data->op_lock);
	init_usbc_cmd_data(&current_cmd);

	/* the messages sent to USBC. */
	init_usbc_cmd_data(&execute_cmd_data);
	execute_cmd_data.opcode = read_op->opcode;
	execute_cmd_data.write_length = read_op->write_length;
	execute_cmd_data.is_uvdm = read_op->is_uvdm;
	memcpy(execute_cmd_data.write_data, read_op->write_data, read_op->write_length);
	execute_cmd_data.seq = OPCODE_READ_SEQ;
	enqueue_usbc_cmd(cmd_queue, &execute_cmd_data);

	/* the messages recevied From USBC. */
	init_usbc_cmd_data(&execute_cmd_data);
	execute_cmd_data.response = read_op->opcode;
	execute_cmd_data.read_length = read_op->read_length;
	execute_cmd_data.is_uvdm = read_op->is_uvdm;
	execute_cmd_data.seq = OPCODE_READ_SEQ;
	enqueue_usbc_cmd(cmd_queue, &execute_cmd_data);

	msg_maxim("R->R opcode[0x%02x] write_length[%d] read_length[%d]",
		read_op->opcode, read_op->write_length, read_op->read_length);

	front_usbc_cmd(cmd_queue, &current_cmd);
	if (current_cmd.opcode == read_op->opcode)
		max77775_usbc_cmd_run(usbc_data);
	else {
		msg_maxim("!!!current_cmd.opcode [0x%02x][0x%02x], read_op->opcode[0x%02x]",
			current_cmd.opcode, current_cmd.response, read_op->opcode);
		if (usbc_data->opcode_stamp != 0 && current_cmd.opcode == OPCODE_NONE) {
			if (time_after(jiffies,
					usbc_data->opcode_stamp + MAX77775_MAX_APDCMD_TIME)) {
				usbc_data->opcode_stamp = 0;
				msg_maxim("error. we will dequeue response data");
				max77775_usbc_dequeue_queue(usbc_data);
				if (max77775_is_stuck_suppose_opcode(current_cmd.response))
					enforce = true;
				if (max77775_usbc_increase_opcode_fail(usbc_data, enforce))
					goto out;
				max77775_usbc_cmd_run(usbc_data);
			}
		}
	}
out:
	mutex_unlock(&usbc_data->op_lock);

	return 0;
}

int max77775_usbc_opcode_update(struct max77775_usbc_platform_data *usbc_data,
	usbc_cmd_data *update_op)
{
	struct max77775_dev *max77775 = usbc_data->max77775;
	usbc_cmd_queue_t *cmd_queue = &(usbc_data->usbc_cmd_queue);
	usbc_cmd_data execute_cmd_data;
	usbc_cmd_data current_cmd;
	bool enforce = false;

	if (usbc_data->usb_mock.opcode_update)
		return usbc_data->usb_mock.opcode_update((void *)usbc_data, update_op);

	if (max77775->suspended == true) {
		msg_maxim("max77775 in suspend state, just return");
		return -EACCES;
	}

	switch (update_op->opcode) {
	case OPCODE_BCCTRL1_R:
	case OPCODE_BCCTRL2_R:
	case OPCODE_CTRL1_R:
	case OPCODE_CCCTRL1_R:
	case OPCODE_CCCTRL2_R:
	case OPCODE_CCCTRL3_R:
	case OPCODE_CCCTRL4_R:
	case OPCODE_VCONN_ILIM_R:
		break;
	default:
		md75_err_usb("%s: invalid usage(0x%x), return\n", __func__, update_op->opcode);
		return -EINVAL;
	}

	mutex_lock(&usbc_data->op_lock);
	init_usbc_cmd_data(&current_cmd);

	/* the messages sent to USBC. */
	init_usbc_cmd_data(&execute_cmd_data);
	execute_cmd_data.opcode = update_op->opcode;
	execute_cmd_data.write_length = 0;
	execute_cmd_data.is_uvdm = update_op->is_uvdm;
	memcpy(execute_cmd_data.write_data, update_op->write_data, update_op->write_length);
	execute_cmd_data.seq = OPCODE_UPDATE_SEQ;
	enqueue_usbc_cmd(cmd_queue, &execute_cmd_data);

	/* the messages recevied From USBC. */
	init_usbc_cmd_data(&execute_cmd_data);
	execute_cmd_data.response = update_op->opcode;
	execute_cmd_data.read_length = 1;
	execute_cmd_data.seq = OPCODE_UPDATE_SEQ;
	execute_cmd_data.val = update_op->val;
	execute_cmd_data.mask = update_op->mask;
	execute_cmd_data.is_uvdm = update_op->is_uvdm;
	enqueue_usbc_cmd(cmd_queue, &execute_cmd_data);

	msg_maxim("U->U opcode[0x%02x] write_length[%d] read_length[%d]",
		update_op->opcode, update_op->write_length, update_op->read_length);

	front_usbc_cmd(cmd_queue, &current_cmd);
	if (current_cmd.opcode == update_op->opcode)
		max77775_usbc_cmd_run(usbc_data);
	else {
		msg_maxim("!!! current_cmd.opcode [0x%02x], update_op->opcode[0x%02x]",
			current_cmd.opcode, update_op->opcode);
		if (usbc_data->opcode_stamp != 0 && current_cmd.opcode == OPCODE_NONE) {
			if (time_after(jiffies,
					usbc_data->opcode_stamp + MAX77775_MAX_APDCMD_TIME)) {
				usbc_data->opcode_stamp = 0;
				msg_maxim("error. we will dequeue response data");
				max77775_usbc_dequeue_queue(usbc_data);
				if (max77775_is_stuck_suppose_opcode(current_cmd.response))
					enforce = true;
				if (max77775_usbc_increase_opcode_fail(usbc_data, enforce))
					goto out;
				max77775_usbc_cmd_run(usbc_data);
			}
		}
	}
out:
	mutex_unlock(&usbc_data->op_lock);

	return 0;
}

int max77775_usbc_opcode_push(struct max77775_usbc_platform_data *usbc_data,
	usbc_cmd_data *read_op)
{
	struct max77775_dev *max77775 = usbc_data->max77775;
	usbc_cmd_queue_t *cmd_queue = &(usbc_data->usbc_cmd_queue);
	usbc_cmd_data execute_cmd_data;
	usbc_cmd_data current_cmd;

	if (usbc_data->usb_mock.opcode_push)
		return usbc_data->usb_mock.opcode_push((void *)usbc_data, read_op);

	if (max77775->suspended == true) {
		msg_maxim("max77775 in suspend state, just return");
		return -EACCES;
	}

	init_usbc_cmd_data(&current_cmd);

	/* the messages sent to USBC. */
	init_usbc_cmd_data(&execute_cmd_data);
	execute_cmd_data.opcode = read_op->opcode;
	execute_cmd_data.write_length = read_op->write_length;
	execute_cmd_data.is_uvdm = read_op->is_uvdm;
	memcpy(execute_cmd_data.write_data, read_op->write_data, read_op->write_length);
	execute_cmd_data.seq = OPCODE_PUSH_SEQ;
	enqueue_usbc_cmd(cmd_queue, &execute_cmd_data);

	/* the messages recevied From USBC. */
	init_usbc_cmd_data(&execute_cmd_data);
	execute_cmd_data.response = read_op->opcode;
	execute_cmd_data.read_length = read_op->read_length;
	execute_cmd_data.is_uvdm = read_op->is_uvdm;
	execute_cmd_data.seq = OPCODE_PUSH_SEQ;
	enqueue_usbc_cmd(cmd_queue, &execute_cmd_data);

	msg_maxim("P->P opcode[0x%02x] write_length[%d] read_length[%d]",
		read_op->opcode, read_op->write_length, read_op->read_length);

	return 0;
}

int max77775_usbc_opcode_rw(struct max77775_usbc_platform_data *usbc_data,
	usbc_cmd_data *read_op, usbc_cmd_data *write_op)
{
	struct max77775_dev *max77775 = usbc_data->max77775;
	usbc_cmd_queue_t *cmd_queue = &(usbc_data->usbc_cmd_queue);
	usbc_cmd_data execute_cmd_data;
	usbc_cmd_data current_cmd;
	bool enforce = false;

	if (usbc_data->usb_mock.opcode_rw)
		return usbc_data->usb_mock.opcode_rw((void *)usbc_data, read_op, write_op);

	if (max77775->suspended == true) {
		msg_maxim("max77775 in suspend state, just return");
		return -EACCES;
	}

	mutex_lock(&usbc_data->op_lock);
	init_usbc_cmd_data(&current_cmd);

	/* the messages sent to USBC. */
	init_usbc_cmd_data(&execute_cmd_data);
	execute_cmd_data.opcode = read_op->opcode;
	execute_cmd_data.write_length = read_op->write_length;
	execute_cmd_data.is_uvdm = read_op->is_uvdm;
	memcpy(execute_cmd_data.write_data, read_op->write_data, read_op->write_length);
	execute_cmd_data.seq = OPCODE_RW_SEQ;
	enqueue_usbc_cmd(cmd_queue, &execute_cmd_data);

	/* the messages recevied From USBC. */
	init_usbc_cmd_data(&execute_cmd_data);
	execute_cmd_data.response = read_op->opcode;
	execute_cmd_data.read_length = read_op->read_length;
	execute_cmd_data.is_uvdm = read_op->is_uvdm;
	execute_cmd_data.seq = OPCODE_RW_SEQ;
	enqueue_usbc_cmd(cmd_queue, &execute_cmd_data);

	/* the messages sent to USBC. */
	init_usbc_cmd_data(&execute_cmd_data);
	execute_cmd_data.opcode = write_op->opcode;
	execute_cmd_data.write_length = write_op->write_length;
	execute_cmd_data.is_uvdm = write_op->is_uvdm;
	memcpy(execute_cmd_data.write_data, write_op->write_data, OPCODE_DATA_LENGTH);
	execute_cmd_data.seq = OPCODE_RW_SEQ;
	enqueue_usbc_cmd(cmd_queue, &execute_cmd_data);

	/* the messages recevied From USBC. */
	init_usbc_cmd_data(&execute_cmd_data);
	execute_cmd_data.response = write_op->opcode;
	execute_cmd_data.read_length = write_op->read_length;
	execute_cmd_data.is_uvdm = write_op->is_uvdm;
	execute_cmd_data.seq = OPCODE_RW_SEQ;
	enqueue_usbc_cmd(cmd_queue, &execute_cmd_data);

	msg_maxim("RW->R opcode[0x%02x] write_length[%d] read_length[%d]",
		read_op->opcode, read_op->write_length, read_op->read_length);
	msg_maxim("RW->W opcode[0x%02x] write_length[%d] read_length[%d]",
		write_op->opcode, write_op->write_length, write_op->read_length);

	front_usbc_cmd(cmd_queue, &current_cmd);
	if (current_cmd.opcode == read_op->opcode)
		max77775_usbc_cmd_run(usbc_data);
	else {
		msg_maxim("!!! current_cmd.opcode [0x%02x], read_op->opcode[0x%02x]",
			current_cmd.opcode, read_op->opcode);
		if (usbc_data->opcode_stamp != 0 && current_cmd.opcode == OPCODE_NONE) {
			if (time_after(jiffies,
					usbc_data->opcode_stamp + MAX77775_MAX_APDCMD_TIME)) {
				usbc_data->opcode_stamp = 0;
				msg_maxim("error. we will dequeue response data");
				max77775_usbc_dequeue_queue(usbc_data);
				if (max77775_is_stuck_suppose_opcode(current_cmd.response))
					enforce = true;
				if (max77775_usbc_increase_opcode_fail(usbc_data, enforce))
					goto out;
				max77775_usbc_cmd_run(usbc_data);
			}
		}
	}
out:
	mutex_unlock(&usbc_data->op_lock);

	return 0;
}

/*
 * max77775_uic_op_send_work_func func - Send OPCode
 * @work: work_struct of max77775_i2c
 *
 * Wait for OPCode response
 */
static void max77775_uic_op_send_work_func(
		struct work_struct *work)
{
	struct max77775_usbc_platform_data *usbc_data;
	struct max77775_opcode opcodes[] = {
		{
			.opcode = OPCODE_BCCTRL1_R,
			.data = { 0, },
			.write_length = 0,
			.read_length = 1,
		},
		{
			.opcode = OPCODE_BCCTRL1_W,
			.data = { 0x20, },
			.write_length = 1,
			.read_length = 0,
		},
		{
			.opcode = OPCODE_BCCTRL2_R,
			.data = { 0, },
			.write_length = 0,
			.read_length = 1,
		},
		{
			.opcode = OPCODE_BCCTRL2_W,
			.data = { 0x10, },
			.write_length = 1,
			.read_length = 0,
		},
		{
			.opcode = OPCODE_CURRENT_SRCCAP,
			.data = { 0, },
			.write_length = 1,
			.read_length = 4,
		},
		{
			.opcode = OPCODE_AFC_HV_W,
			.data = { 0x46, },
			.write_length = 1,
			.read_length = 2,
		},
		{
			.opcode = OPCODE_SRCCAP_REQUEST,
			.data = { 0x00, },
			.write_length = 1,
			.read_length = 32,
		},
	};
	int op_loop;

	usbc_data = container_of(work, struct max77775_usbc_platform_data, op_send_work);

	msg_maxim("IN");
	for (op_loop = 0; op_loop < ARRAY_SIZE(opcodes); op_loop++) {
		if (usbc_data->op_code == opcodes[op_loop].opcode) {
			max77775_i2c_opcode_write(usbc_data, opcodes[op_loop].opcode,
				opcodes[op_loop].write_length, opcodes[op_loop].data);
			break;
		}
	}
	msg_maxim("OUT");
}

static void max77775_reset_ic(struct max77775_usbc_platform_data *usbc_data)
{
	struct max77775_dev *max77775 = usbc_data->max77775;

	//gurantee to block i2c trasaction during ccic reset
	msg_maxim("IN");
	mutex_lock(&max77775->i2c_lock);
	max77775_write_reg_nolock(usbc_data->muic, 0x80, 0x0F);
	msleep(150); /* need 150ms delay */
	mutex_unlock(&max77775->i2c_lock);
	msg_maxim("OUT");
}

void max77775_usbc_check_sysmsg(struct max77775_usbc_platform_data *usbc_data, u8 sysmsg)
{
	usbc_cmd_queue_t *cmd_queue = &(usbc_data->usbc_cmd_queue);
	bool is_empty_queue = is_empty_usbc_cmd_queue(cmd_queue);
	usbc_cmd_data cmd_data;
	usbc_cmd_data next_cmd_data;
	u8 next_opcode = 0xFF;
	u8 interrupt = 0;
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	int event;
#endif
	int ret = 0;

	if (usbc_data->shut_down) {
		msg_maxim("IGNORE SYSTEM_MSG IN SHUTDOWN MODE!!");
		return;
	}

	if (usbc_data->stuck_suppose &&
		!(sysmsg == SYSERROR_BOOT_WDT || sysmsg == SYSMSG_BOOT_POR)) {
		msg_maxim("Do usbc Reset operation, opcode_fail_count: %d", usbc_data->opcode_fail_count);
		max77775_usbc_mask_irq(usbc_data);
		max77775_reset_ic(usbc_data);
		max77775_write_reg(usbc_data->muic, REG_UIC_INT_M, REG_UIC_INT_M_INIT);
		max77775_write_reg(usbc_data->muic, REG_CC_INT_M, REG_CC_INT_M_INIT);
		max77775_write_reg(usbc_data->muic, REG_PD_INT_M, REG_PD_INT_M_INIT);
		max77775_write_reg(usbc_data->muic, REG_VDM_INT_M, REG_VDM_INT_M_INIT);
		max77775_write_reg(usbc_data->muic, REG_SPARE_INT_M, REG_SPARE_INT_M_INIT);
		/* clear UIC_INT to prevent infinite sysmsg irq*/
		g_usbc_data->max77775->enable_nested_irq = 1;
		max77775_read_reg(usbc_data->muic, MAX77775_USBC_REG_UIC_INT, &interrupt);
		g_usbc_data->max77775->usbc_irq = interrupt & 0xBF; //clear the USBC SYSTEM IRQ
		max77775_usbc_clear_queue(usbc_data);
		usbc_data->opcode_fail_count = 0;
		usbc_data->stuck_suppose = 0;
		usbc_data->is_first_booting = 1;
		usbc_data->pd_data->bPPS_on = false;
		max77775_init_opcode(usbc_data, 1);
		max77775_usbc_umask_irq(usbc_data);
	}

	switch (sysmsg) {
	case SYSERROR_NONE:
		break;
	case SYSERROR_BOOT_WDT:
		usbc_data->watchdog_count++;
		msg_maxim("SYSERROR_BOOT_WDT: %d", usbc_data->watchdog_count);
		max77775_usbc_mask_irq(usbc_data);
		max77775_write_reg(usbc_data->muic, REG_UIC_INT_M, REG_UIC_INT_M_INIT);
		max77775_write_reg(usbc_data->muic, REG_CC_INT_M, REG_CC_INT_M_INIT);
		max77775_write_reg(usbc_data->muic, REG_PD_INT_M, REG_PD_INT_M_INIT);
		max77775_write_reg(usbc_data->muic, REG_VDM_INT_M, REG_VDM_INT_M_INIT);
		max77775_write_reg(usbc_data->muic, REG_SPARE_INT_M, REG_SPARE_INT_M_INIT);
		/* clear UIC_INT to prevent infinite sysmsg irq*/
		g_usbc_data->max77775->enable_nested_irq = 1;
		max77775_read_reg(usbc_data->muic, MAX77775_USBC_REG_UIC_INT, &interrupt);
		g_usbc_data->max77775->usbc_irq = interrupt & 0xBF; //clear the USBC SYSTEM IRQ
		max77775_usbc_clear_queue(usbc_data);
		usbc_data->opcode_fail_count = 0;
		usbc_data->stuck_suppose = 0;
		usbc_data->is_first_booting = 1;
		usbc_data->pd_data->bPPS_on = false;
		max77775_init_opcode(usbc_data, 1);
		max77775_usbc_umask_irq(usbc_data);
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
		event = NOTIFY_EXTRA_SYSERROR_BOOT_WDT;
		store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
		break;
	case SYSERROR_BOOT_SWRSTREQ:
		break;
	case SYSMSG_BOOT_POR:
		usbc_data->por_count++;
		max77775_usbc_mask_irq(usbc_data);
		max77775_reset_ic(usbc_data);
		max77775_write_reg(usbc_data->muic, REG_UIC_INT_M, REG_UIC_INT_M_INIT);
		max77775_write_reg(usbc_data->muic, REG_CC_INT_M, REG_CC_INT_M_INIT);
		max77775_write_reg(usbc_data->muic, REG_PD_INT_M, REG_PD_INT_M_INIT);
		max77775_write_reg(usbc_data->muic, REG_VDM_INT_M, REG_VDM_INT_M_INIT);
		max77775_write_reg(usbc_data->muic, REG_SPARE_INT_M, REG_SPARE_INT_M_INIT);
		/* clear UIC_INT to prevent infinite sysmsg irq*/
		g_usbc_data->max77775->enable_nested_irq = 1;
		max77775_read_reg(usbc_data->muic, MAX77775_USBC_REG_UIC_INT, &interrupt);
		g_usbc_data->max77775->usbc_irq = interrupt & 0xBF; //clear the USBC SYSTEM IRQ
		msg_maxim("SYSERROR_BOOT_POR: %d, UIC_INT:0x%02x", usbc_data->por_count, interrupt);
		max77775_usbc_clear_queue(usbc_data);
		usbc_data->opcode_fail_count = 0;
		usbc_data->stuck_suppose = 0;
		usbc_data->is_first_booting = 1;
		usbc_data->pd_data->bPPS_on = false;
		max77775_init_opcode(usbc_data, 1);
		max77775_usbc_umask_irq(usbc_data);
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
		event = NOTIFY_EXTRA_SYSMSG_BOOT_POR;
		store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
		break;
	case SYSERROR_HV_NOVBUS:
		break;
	case SYSERROR_HV_FMETHOD_RXPERR:
		break;
	case SYSERROR_HV_FMETHOD_RXBUFOW:
		break;
	case SYSERROR_HV_FMETHOD_RXTFR:
		break;
	case SYSERROR_HV_FMETHOD_MPNACK:
		break;
	case SYSERROR_HV_FMETHOD_RESET_FAIL:
		break;
	case SYSMsg_AFC_Done:
		break;
	case SYSERROR_SYSPOS:
		break;
	case SYSERROR_APCMD_UNKNOWN:
		break;
	case SYSERROR_APCMD_INPROGRESS:
		break;
	case SYSERROR_APCMD_FAIL:

		init_usbc_cmd_data(&cmd_data);
		init_usbc_cmd_data(&next_cmd_data);

		if (front_usbc_cmd(cmd_queue, &next_cmd_data))
			next_opcode = next_cmd_data.response;

		if (!is_empty_queue) {
			copy_usbc_cmd_data(&(usbc_data->last_opcode), &cmd_data);

			if (next_opcode == OPCODE_VDM_DISCOVER_SET_VDM_REQ) {
				usbc_data->opcode_stamp = 0;
				max77775_usbc_dequeue_queue(usbc_data);
				cmd_data.opcode = OPCODE_NONE;
			}

			if ((cmd_data.opcode != OPCODE_NONE) && (cmd_data.opcode == next_opcode)) {
				if (next_opcode != OPCODE_VDM_DISCOVER_SET_VDM_REQ) {
					ret = max77775_i2c_opcode_write(usbc_data,
						cmd_data.opcode,
						cmd_data.write_length,
						cmd_data.write_data);
					if (ret) {
						msg_maxim("i2c write fail. dequeue opcode");
						max77775_usbc_dequeue_queue(usbc_data);
					} else
						msg_maxim("RETRY SUCCESS : %x, %x", cmd_data.opcode, next_opcode);
				} else
					msg_maxim("IGNORE COMMAND : %x, %x", cmd_data.opcode, next_opcode);
			} else {
				msg_maxim("RETRY FAILED : %x, %x", cmd_data.opcode, next_opcode);
			}

		}

		/* TO DO DEQEUE MSG. */
		break;
	case SYSMSG_CCx_5V_SHORT:
	case SYSMSG_CCx_5V_AFC_SHORT:
		msg_maxim("CC-VBUS SHORT");
		usbc_data->pd_data->cc_sbu_short = true;
#if defined(CONFIG_USB_HW_PARAM)
		if (o_notify)
			inc_hw_param(o_notify, USB_CCIC_VBUS_CC_SHORT_COUNT);
#endif
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
		event = NOTIFY_EXTRA_SYSMSG_CC_SHORT;
		store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
		usbc_data->cc_data->ccistat = CCI_SHORT;
		max77775_notify_rp_current_level(usbc_data);
		break;
	case SYSMSG_SBUx_GND_SHORT:
		msg_maxim("SBU-GND SHORT");
#if defined(CONFIG_USB_HW_PARAM)
		if (o_notify)
			inc_hw_param(o_notify, USB_CCIC_GND_SBU_SHORT_COUNT);
#endif
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
		event = NOTIFY_EXTRA_SYSMSG_SBU_GND_SHORT;
		store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
		break;
	case SYSMSG_SBUx_5V_SHORT:
		msg_maxim("SBU-VBUS SHORT");
		usbc_data->pd_data->cc_sbu_short = true;
#if defined(CONFIG_USB_HW_PARAM)
		if (o_notify)
			inc_hw_param(o_notify, USB_CCIC_VBUS_SBU_SHORT_COUNT);
#endif
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
		event = NOTIFY_EXTRA_SYSMSG_SBU_VBUS_SHORT;
		store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
		break;
	case SYSERROR_POWER_NEGO:
		if (!usbc_data->last_opcode.is_uvdm) { /* structured vdm */
			if (usbc_data->last_opcode.opcode == OPCODE_VDM_DISCOVER_SET_VDM_REQ) {
				max77775_usbc_opcode_write(usbc_data, &usbc_data->last_opcode);
				msg_maxim("SYSMSG PWR NEGO ERR : VDM request retry");
			}
		} else { /* unstructured vdm */
			usbc_data->uvdm_error = -EACCES;
			msg_maxim("SYSMSG PWR NEGO ERR : UVDM request error - dir : %d",
				usbc_data->is_in_sec_uvdm_out);
			if (usbc_data->is_in_sec_uvdm_out == DIR_OUT)
				complete(&usbc_data->uvdm_longpacket_out_wait);
			else if (usbc_data->is_in_sec_uvdm_out == DIR_IN)
				complete(&usbc_data->uvdm_longpacket_in_wait);
			else
				;
		}
		break;
#if defined(CONFIG_SEC_FACTORY)
	case SYSERROR_FACTORY_RID0:
		factory_execute_monitor(FAC_ABNORMAL_REPEAT_RID0);
		break;
#endif
	case SYSERROR_CCRP_HIGH:
		msg_maxim("CCRP HIGH");
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
		if (usbc_data->ccrp_state != 1) {
			usbc_data->ccrp_state = 1;
			max77775_ccic_event_work(usbc_data,
				PDIC_NOTIFY_DEV_BATT, PDIC_NOTIFY_ID_WATER_CABLE,
				PDIC_NOTIFY_ATTACH, 0/*rprd*/, 0);
		}
#endif
		break;
	case SYSERROR_CCRP_LOW:
		msg_maxim("CCRP LOW");
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
		if (usbc_data->ccrp_state != 0) {
			usbc_data->ccrp_state = 0;
			max77775_ccic_event_work(usbc_data,
				PDIC_NOTIFY_DEV_BATT, PDIC_NOTIFY_ID_WATER_CABLE,
				PDIC_NOTIFY_DETACH, 0/*rprd*/, 0);
		}
#endif
		break;
	case SYSMSG_ABNORMAL_TA:
		{
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			union power_supply_propval value = {0,};

			value.intval = true;
			psy_do_property("battery", set,
				POWER_SUPPLY_EXT_PROP_ABNORMAL_TA, value);
#endif
			msg_maxim("ABNORMAL TA detected");
		}
		break;
	case SYSMSG_RELEASE_CC1_SHORT:
	case SYSMSG_RELEASE_CC2_SHORT:
		msg_maxim("CCX_RELEASE_SHORT");
		usbc_data->pd_data->cc_sbu_short = false;
		usbc_data->cc_data->ccistat = NOT_IN_UFP_MODE;
		break;
	default:
		break;
	}
}

bool max77775_need_check_stuck(struct max77775_usbc_platform_data *usbc_data)
{
	usbc_cmd_queue_t *cmd_queue = &(usbc_data->usbc_cmd_queue);
	usbc_cmd_data current_cmd;
	bool ret = false;

	mutex_lock(&usbc_data->op_lock);

	if (front_usbc_cmd(cmd_queue, &current_cmd)) {
		if (usbc_data->opcode_stamp != 0 && current_cmd.opcode == OPCODE_NONE) {
			if (time_after(jiffies,
					usbc_data->opcode_stamp + MAX77775_MAX_APDCMD_TIME))
				ret = true;
		}
	}

	mutex_unlock(&usbc_data->op_lock);

	return ret;
}

void max77775_send_check_stuck_opcode(struct max77775_usbc_platform_data *usbpd_data)
{
	usbc_cmd_data read_data;

	msg_maxim("%s", __func__);

	init_usbc_cmd_data(&read_data);
	read_data.opcode = OPCODE_BCCTRL1_R;
	max77775_usbc_opcode_read(usbpd_data, &read_data);
}

void max77775_chg_check_stuck(int chgin_dtls)
{
	u8 vbdet;

	struct max77775_usbc_platform_data *usbc_data = g_usbc_data;

	max77775_read_reg(usbc_data->muic, MAX77775_USBC_REG_BC_STATUS, &vbdet);
	vbdet = ((vbdet & 0x80) >> 7);
	msg_maxim("vbdet %d, chgin_dtls %d", vbdet, chgin_dtls);

	if ((vbdet == 0x00 && chgin_dtls == 0x03)
		|| (vbdet == 0x01 && (chgin_dtls == 0x00 || chgin_dtls == 0x01 || chgin_dtls == 0x02))) {
		msg_maxim("%s stuck suppose.", __func__);
		max77775_send_check_stuck_opcode(usbc_data);
	}
}
EXPORT_SYMBOL(max77775_chg_check_stuck);

static irqreturn_t max77775_apcmd_irq(int irq, void *data)
{
	struct max77775_usbc_platform_data *usbc_data = data;
	u8 sysmsg = 0;

	usbc_data->opcode_fail_count = 0;
	usbc_data->stuck_suppose = 0;
	msg_maxim("IRQ(%d)_IN", irq);
	max77775_read_reg(usbc_data->muic, REG_USBC_STATUS2, &usbc_data->usbc_status2);
	sysmsg = usbc_data->usbc_status2;
	msg_maxim(" [IN] sysmsg : %d", sysmsg);

	mutex_lock(&usbc_data->op_lock);
	max77775_usbc_cmd_run(usbc_data);
	mutex_unlock(&usbc_data->op_lock);

	if (usbc_data->need_recover) {
		max77775_recover_opcode(usbc_data,
			usbc_data->recover_opcode_list);
		usbc_data->need_recover = false;
	}

	msg_maxim("IRQ(%d)_OUT", irq);

	return IRQ_HANDLED;
}

static irqreturn_t max77775_sysmsg_irq(int irq, void *data)
{
	struct max77775_usbc_platform_data *usbc_data = data;
	u8 sysmsg = 0;
	u8 i = 0;
	u8 raw_data[3] = {0, };
	u8 usbc_status2 = 0;
	u8 dump_reg[10] = {0, };

	for (i = 0; i < 3; i++) {
		usbc_status2 = 0;
		max77775_read_reg(usbc_data->muic, REG_USBC_STATUS2, &usbc_status2);
		raw_data[i] = usbc_status2;
	}
	if ((raw_data[0] == raw_data[1]) && (raw_data[0] == raw_data[2])) {
		sysmsg = raw_data[0];
	} else {
		max77775_bulk_read(usbc_data->muic, REG_USBC_STATUS1, 8, dump_reg);
		msg_maxim("[ERROR ]sys_reg, %x, %x, %x", raw_data[0], raw_data[1], raw_data[2]);
		msg_maxim("[ERROR ]dump_reg, %x, %x, %x, %x, %x, %x, %x, %x\n", dump_reg[0], dump_reg[1],
				  dump_reg[2], dump_reg[3], dump_reg[4], dump_reg[5], dump_reg[6], dump_reg[7]);
		sysmsg = 0x6D;
	}
	msg_maxim("IRQ(%d)_IN sysmsg: %x", irq, sysmsg);
	max77775_usbc_check_sysmsg(usbc_data, sysmsg);
	usbc_data->sysmsg = sysmsg;
	msg_maxim("IRQ(%d)_OUT sysmsg: %x", irq, sysmsg);

	return IRQ_HANDLED;
}


static irqreturn_t max77775_vdm_identity_irq(int irq, void *data)
{
	struct max77775_usbc_platform_data *usbc_data = data;
	MAX77775_VDM_MSG_IRQ_STATUS_Type VDM_MSG_IRQ_State;

	memset(&VDM_MSG_IRQ_State, 0, sizeof(VDM_MSG_IRQ_State));
	msg_maxim("IRQ(%d)_IN", irq);
	VDM_MSG_IRQ_State.BITS.Vdm_Flag_Discover_ID = 1;
	max77775_receive_alternate_message(usbc_data, &VDM_MSG_IRQ_State);
	msg_maxim("IRQ(%d)_OUT", irq);

	return IRQ_HANDLED;
}

static irqreturn_t max77775_vdm_svids_irq(int irq, void *data)
{
	struct max77775_usbc_platform_data *usbc_data = data;
	MAX77775_VDM_MSG_IRQ_STATUS_Type VDM_MSG_IRQ_State;

	memset(&VDM_MSG_IRQ_State, 0, sizeof(VDM_MSG_IRQ_State));
	msg_maxim("IRQ(%d)_IN", irq);
	VDM_MSG_IRQ_State.BITS.Vdm_Flag_Discover_SVIDs = 1;
	max77775_receive_alternate_message(usbc_data, &VDM_MSG_IRQ_State);
	msg_maxim("IRQ(%d)_OUT", irq);
	return IRQ_HANDLED;
}

static irqreturn_t max77775_vdm_discover_mode_irq(int irq, void *data)
{
	struct max77775_usbc_platform_data *usbc_data = data;
	MAX77775_VDM_MSG_IRQ_STATUS_Type VDM_MSG_IRQ_State;

	memset(&VDM_MSG_IRQ_State, 0, sizeof(VDM_MSG_IRQ_State));
	msg_maxim("IRQ(%d)_IN", irq);
	VDM_MSG_IRQ_State.BITS.Vdm_Flag_Discover_MODEs = 1;
	max77775_receive_alternate_message(usbc_data, &VDM_MSG_IRQ_State);
	msg_maxim("IRQ(%d)_OUT", irq);
	return IRQ_HANDLED;
}

static irqreturn_t max77775_vdm_enter_mode_irq(int irq, void *data)
{
	struct max77775_usbc_platform_data *usbc_data = data;
	MAX77775_VDM_MSG_IRQ_STATUS_Type VDM_MSG_IRQ_State;

	memset(&VDM_MSG_IRQ_State, 0, sizeof(VDM_MSG_IRQ_State));
	msg_maxim("IRQ(%d)_IN", irq);
	VDM_MSG_IRQ_State.BITS.Vdm_Flag_Enter_Mode = 1;
	max77775_receive_alternate_message(usbc_data, &VDM_MSG_IRQ_State);
	msg_maxim("IRQ(%d)_OUT", irq);
	return IRQ_HANDLED;
}

static irqreturn_t max77775_vdm_dp_status_irq(int irq, void *data)
{
	struct max77775_usbc_platform_data *usbc_data = data;
	MAX77775_VDM_MSG_IRQ_STATUS_Type VDM_MSG_IRQ_State;

	memset(&VDM_MSG_IRQ_State, 0, sizeof(VDM_MSG_IRQ_State));
	msg_maxim("IRQ(%d)_IN", irq);
	VDM_MSG_IRQ_State.BITS.Vdm_Flag_DP_Status_Update = 1;
	max77775_receive_alternate_message(usbc_data, &VDM_MSG_IRQ_State);
	msg_maxim("IRQ(%d)_OUT", irq);
	return IRQ_HANDLED;
}

static irqreturn_t max77775_vdm_dp_configure_irq(int irq, void *data)
{
	struct max77775_usbc_platform_data *usbc_data = data;
	MAX77775_VDM_MSG_IRQ_STATUS_Type VDM_MSG_IRQ_State;

	memset(&VDM_MSG_IRQ_State, 0, sizeof(VDM_MSG_IRQ_State));
	msg_maxim("IRQ(%d)_IN", irq);
	VDM_MSG_IRQ_State.BITS.Vdm_Flag_DP_Configure = 1;
	max77775_receive_alternate_message(usbc_data, &VDM_MSG_IRQ_State);
	msg_maxim("IRQ(%d)_OUT", irq);
	return IRQ_HANDLED;
}

static irqreturn_t max77775_vdm_attention_irq(int irq, void *data)
{
	struct max77775_usbc_platform_data *usbc_data = data;
	MAX77775_VDM_MSG_IRQ_STATUS_Type VDM_MSG_IRQ_State;

	memset(&VDM_MSG_IRQ_State, 0, sizeof(VDM_MSG_IRQ_State));
	msg_maxim("IRQ(%d)_IN", irq);
	VDM_MSG_IRQ_State.BITS.Vdm_Flag_Attention = 1;
	max77775_receive_alternate_message(usbc_data, &VDM_MSG_IRQ_State);
	msg_maxim("IRQ(%d)_OUT", irq);
	return IRQ_HANDLED;
}

static irqreturn_t max77775_vir_altmode_irq(int irq, void *data)
{
	struct max77775_usbc_platform_data *usbc_data = data;

	msg_maxim("max77775_vir_altmode_irq");

	if (usbc_data->shut_down) {
		msg_maxim("%s doing shutdown. skip set alternate mode", __func__);
		goto skip;
	}

	max77775_set_enable_alternate_mode
		(usbc_data->set_altmode);

skip:
	return IRQ_HANDLED;
}

static irqreturn_t max77775_usbid_irq(int irq, void *data)
{
#if 0 /* TODO */
	struct max77775_usbc_platform_data *usbc_data = data;
#endif

	msg_maxim("IRQ(%d)_IN", irq);
	/* TODO */
	msg_maxim("IRQ(%d)_OUT", irq);
	return IRQ_HANDLED;
}

#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
static irqreturn_t max77775_taconnect_irq(int irq, void *data)
{
	struct max77775_usbc_platform_data *usbc_data = data;
	u8 spare_status1 = 0;
	u8 cc_open_status = 0;

	msg_maxim("%s: IRQ(%d)_IN", __func__, irq);
	if (usbc_data->current_connstat == WATER) {
		max77775_read_reg(usbc_data->muic, MAX77775_USBC_REG_SPARE_STATUS1, &spare_status1);
		usbc_data->ta_conn_status = (spare_status1 & BIT_SPARE_TA_CONNECT) >> FFS(BIT_SPARE_TA_CONNECT);
		cc_open_status = (spare_status1 & BIT_SPARE_CC_OPEN) >> FFS(BIT_SPARE_CC_OPEN);
		/*Source Connection Status of Moisture Case, 0x0: unplug TA, 0x1:plug TA*/
		if (usbc_data->ta_conn_status == 0x1) {
			msg_maxim("Water Cable Attached (CC OPEN Status : %s)", cc_open_status ? "Enabled" : "Disabled");
			cancel_delayed_work(&usbc_data->set_ccopen_for_watercable_work);
			schedule_delayed_work(&usbc_data->set_ccopen_for_watercable_work, msecs_to_jiffies(100));
		} else {
			msg_maxim("Water Cable Detached (CC OPEN Status : %s)", cc_open_status ? "Enabled" : "Disabled");
			cancel_delayed_work(&usbc_data->set_ccopen_for_watercable_work);
		}
	}
	msg_maxim("%s: IRQ(%d)_OUT", __func__, irq);
	return IRQ_HANDLED;
}
#endif

int max77775_init_irq_handler(struct max77775_usbc_platform_data *usbc_data)
{
	int ret = 0;

	usbc_data->irq_apcmd = usbc_data->irq_base + MAX77775_USBC_IRQ_APC_INT;
	if (usbc_data->irq_apcmd) {
		ret = request_threaded_irq(usbc_data->irq_apcmd,
			   NULL, max77775_apcmd_irq,
			   0,
			   "usbc-apcmd-irq", usbc_data);
		if (ret) {
			md75_err_usb("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			return ret;
		}
	}

	usbc_data->irq_sysmsg = usbc_data->irq_base + MAX77775_USBC_IRQ_SYSM_INT;
	if (usbc_data->irq_sysmsg) {
		ret = request_threaded_irq(usbc_data->irq_sysmsg,
			   NULL, max77775_sysmsg_irq,
			   0,
			   "usbc-sysmsg-irq", usbc_data);
		if (ret) {
			md75_err_usb("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			return ret;
		}
	}

	usbc_data->irq_vdm0 = usbc_data->irq_base + MAX77775_IRQ_VDM_DISCOVER_ID_INT;
	if (usbc_data->irq_vdm0) {
		ret = request_threaded_irq(usbc_data->irq_vdm0,
			   NULL, max77775_vdm_identity_irq,
			   0,
			   "usbc-vdm0-irq", usbc_data);
		if (ret) {
			md75_err_usb("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			return ret;
		}
	}

	usbc_data->irq_vdm1 = usbc_data->irq_base + MAX77775_IRQ_VDM_DISCOVER_SVIDS_INT;
	if (usbc_data->irq_vdm1) {
		ret = request_threaded_irq(usbc_data->irq_vdm1,
			   NULL, max77775_vdm_svids_irq,
			   0,
			   "usbc-vdm1-irq", usbc_data);
		if (ret) {
			md75_err_usb("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			return ret;
		}
	}

	usbc_data->irq_vdm2 = usbc_data->irq_base + MAX77775_IRQ_VDM_DISCOVER_MODES_INT;
	if (usbc_data->irq_vdm2) {
		ret = request_threaded_irq(usbc_data->irq_vdm2,
			   NULL, max77775_vdm_discover_mode_irq,
			   0,
			   "usbc-vdm2-irq", usbc_data);
		if (ret) {
			md75_err_usb("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			return ret;
		}
	}

	usbc_data->irq_vdm3 = usbc_data->irq_base + MAX77775_IRQ_VDM_ENTER_MODE_INT;
	if (usbc_data->irq_vdm3) {
		ret = request_threaded_irq(usbc_data->irq_vdm3,
			   NULL, max77775_vdm_enter_mode_irq,
			   0,
			   "usbc-vdm3-irq", usbc_data);
		if (ret) {
			md75_err_usb("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			return ret;
		}
	}

	usbc_data->irq_vdm4 = usbc_data->irq_base + MAX77775_IRQ_VDM_DP_STATUS_UPDATE_INT;
	if (usbc_data->irq_vdm4) {
		ret = request_threaded_irq(usbc_data->irq_vdm4,
			NULL, max77775_vdm_dp_status_irq,
			0,
			"usbc-vdm4-irq", usbc_data);
		if (ret) {
			md75_err_usb("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			return ret;
		}
	}

	usbc_data->irq_vdm5 = usbc_data->irq_base + MAX77775_IRQ_VDM_DP_CONFIGURE_INT;
	if (usbc_data->irq_vdm5) {
		ret = request_threaded_irq(usbc_data->irq_vdm5,
			   NULL, max77775_vdm_dp_configure_irq,
			   0,
			   "usbc-vdm5-irq", usbc_data);
		if (ret) {
			md75_err_usb("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			return ret;
		}
	}

	usbc_data->irq_vdm6 = usbc_data->irq_base + MAX77775_IRQ_VDM_ATTENTION_INT;
	if (usbc_data->irq_vdm6) {
		ret = request_threaded_irq(usbc_data->irq_vdm6,
			   NULL, max77775_vdm_attention_irq,
			   0,
			   "usbc-vdm6-irq", usbc_data);
		if (ret) {
			md75_err_usb("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			return ret;
		}
	}

	usbc_data->irq_vir0 = usbc_data->irq_base + MAX77775_VIR_IRQ_ALTERROR_INT;
	if (usbc_data->irq_vir0) {
		ret = request_threaded_irq(usbc_data->irq_vir0,
			   NULL, max77775_vir_altmode_irq,
			   0,
			   "usbc-vir0-irq", usbc_data);
		if (ret) {
			md75_err_usb("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			return ret;
		}
	}

	usbc_data->irq_usbid = usbc_data->irq_base + MAX77775_IRQ_USBID_INT;
	if (usbc_data->irq_usbid) {
		ret = request_threaded_irq(usbc_data->irq_usbid,
			   NULL, max77775_usbid_irq,
			   0,
			   "usbc-usbid-irq", usbc_data);
		if (ret) {
			md75_err_usb("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			return ret;
		}
	}

#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
	usbc_data->irq_taconn = usbc_data->irq_base + MAX77775_IRQ_TACONN_INT;
	if (usbc_data->irq_taconn) {
		ret = request_threaded_irq(usbc_data->irq_taconn,
			   NULL, max77775_taconnect_irq,
			   0,
			   "usbc-taconnect-irq", usbc_data);
		if (ret) {
			md75_err_usb("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			return ret;
		}
	}
#endif

	return ret;
}

void max77775_usbc_free_irq(struct max77775_usbc_platform_data *usbc_data)
{
	free_irq(usbc_data->irq_usbid, usbc_data);
#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
	free_irq(usbc_data->irq_taconn, usbc_data);
#endif
	free_irq(usbc_data->irq_apcmd, usbc_data);
	free_irq(usbc_data->irq_sysmsg, usbc_data);
	free_irq(usbc_data->irq_vdm0, usbc_data);
	free_irq(usbc_data->irq_vdm1, usbc_data);
	free_irq(usbc_data->irq_vdm2, usbc_data);
	free_irq(usbc_data->irq_vdm3, usbc_data);
	free_irq(usbc_data->irq_vdm4, usbc_data);
	free_irq(usbc_data->irq_vdm5, usbc_data);
	free_irq(usbc_data->irq_vdm6, usbc_data);
	free_irq(usbc_data->irq_vdm7, usbc_data);
	free_irq(usbc_data->pd_data->irq_pdmsg, usbc_data);
	free_irq(usbc_data->pd_data->irq_datarole, usbc_data);
	free_irq(usbc_data->pd_data->irq_ssacc, usbc_data);
	free_irq(usbc_data->pd_data->irq_fct_id, usbc_data);
	free_irq(usbc_data->cc_data->irq_vconncop, usbc_data);
	free_irq(usbc_data->cc_data->irq_vsafe0v, usbc_data);
	free_irq(usbc_data->cc_data->irq_detabrt, usbc_data);
	free_irq(usbc_data->cc_data->irq_vconnsc, usbc_data);
	free_irq(usbc_data->cc_data->irq_ccpinstat, usbc_data);
	free_irq(usbc_data->cc_data->irq_ccistat, usbc_data);
	free_irq(usbc_data->cc_data->irq_ccvcnstat, usbc_data);
	free_irq(usbc_data->cc_data->irq_ccstat, usbc_data);
}

static void max77775_usbc_umask_irq(struct max77775_usbc_platform_data *usbc_data)
{
	int ret = 0;
	u8 i2c_data = 0;
	/* Unmask max77775 interrupt */
	ret = max77775_read_reg(usbc_data->i2c, MAX77775_PMIC_REG_INTSRC_MASK,
			  &i2c_data);
	if (ret) {
		md75_err_usb("%s fail to read muic reg\n", __func__);
		return;
	}

	i2c_data &= ~(MAX77775_IRQSRC_USBC);	/* Unmask muic interrupt */
	max77775_write_reg(usbc_data->i2c, MAX77775_PMIC_REG_INTSRC_MASK,
			   i2c_data);
}
static void max77775_usbc_mask_irq(struct max77775_usbc_platform_data *usbc_data)
{
	int ret = 0;
	u8 i2c_data = 0;
	/* Unmask max77775 interrupt */
	ret = max77775_read_reg(usbc_data->i2c, MAX77775_PMIC_REG_INTSRC_MASK,
			  &i2c_data);
	if (ret) {
		md75_err_usb("%s fail to read muic reg\n", __func__);
		return;
	}

	i2c_data |= MAX77775_IRQSRC_USBC;	/* Mask muic interrupt */
	max77775_write_reg(usbc_data->i2c, MAX77775_PMIC_REG_INTSRC_MASK,
			   i2c_data);
}

#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
static int pdic_handle_usb_external_notifier_notification(struct notifier_block *nb,
				unsigned long action, void *data)
{
	struct max77775_usbc_platform_data *usbpd_data = g_usbc_data;
	int ret = 0;
	int enable = *(int *)data;

	md75_info_usb("%s : action=%lu , enable=%d\n", __func__, action, enable);
	switch (action) {
	case EXTERNAL_NOTIFY_HOSTBLOCK_PRE:
		if (enable) {
			max77775_set_enable_alternate_mode(ALTERNATE_MODE_STOP);
			if (usbpd_data->dp_is_connect)
				max77775_dp_detach(usbpd_data);
		} else {
			if (usbpd_data->dp_is_connect)
				max77775_dp_detach(usbpd_data);
		}
		break;
	case EXTERNAL_NOTIFY_HOSTBLOCK_POST:
		if (enable) {
		} else {
			max77775_set_enable_alternate_mode(ALTERNATE_MODE_START);
		}
		break;
	case EXTERNAL_NOTIFY_DEVICEADD:
		if (enable) {
			usbpd_data->device_add = 1;
			wake_up_interruptible(&usbpd_data->device_add_wait_q);
		}
		break;
	case EXTERNAL_NOTIFY_MDMBLOCK_PRE:
		if (enable && usbpd_data->dp_is_connect) {
			usbpd_data->mdm_block = 1;
			max77775_dp_detach(usbpd_data);
		}
		break;
	default:
		break;
	}

	return ret;
}

static void delayed_external_notifier_init(struct work_struct *work)
{
	int ret = 0;
	static int retry_count = 1;
	int max_retry_count = 5;
	struct max77775_usbc_platform_data *usbpd_data = g_usbc_data;

	md75_info_usb("%s : %d = times!\n", __func__, retry_count);

	/* Register ccic handler to ccic notifier block list */
	ret = usb_external_notify_register(&usbpd_data->usb_external_notifier_nb,
		pdic_handle_usb_external_notifier_notification, EXTERNAL_NOTIFY_DEV_PDIC);
	if (ret < 0) {
		md75_err_usb("Manager notifier init time is %d.\n", retry_count);
		if (retry_count++ != max_retry_count)
			schedule_delayed_work(&usbpd_data->usb_external_notifier_register_work, msecs_to_jiffies(2000));
		else
			md75_err_usb("fail to init external notifier\n");
	} else
		md75_info_usb("%s : external notifier register done!\n", __func__);
}
#endif

#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
static void max77775_set_ccopen_for_watercable(struct work_struct *work)
{
	struct max77775_usbc_platform_data *usbc_data = g_usbc_data;

	md75_info_usb("%s\n", __func__);
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	max77775_ccic_event_work(usbc_data,
		PDIC_NOTIFY_DEV_MUIC, PDIC_NOTIFY_ID_WATER_CABLE, 1, 0, 0);
#endif
}
#endif

#if defined(CONFIG_SEC_FACTORY)
static void factory_check_abnormal_state(struct work_struct *work)
{
	struct max77775_usbc_platform_data *usbpd_data = g_usbc_data;
	int state_cnt = usbpd_data->factory_mode.FAC_Abnormal_Repeat_State;

	msg_maxim("IN ");

	if (state_cnt >= FAC_ABNORMAL_REPEAT_STATE) {
		msg_maxim("Notify the abnormal state [STATE] [ %d]", state_cnt);
		max77775_ccic_event_work(usbpd_data,
			PDIC_NOTIFY_DEV_CCIC, PDIC_NOTIFY_ID_FAC, 1, 0, 0);
	} else
		msg_maxim("[STATE] cnt :  [%d]", state_cnt);
	usbpd_data->factory_mode.FAC_Abnormal_Repeat_State = 0;
	msg_maxim("OUT ");
}

static void factory_check_normal_rid(struct work_struct *work)
{
	struct max77775_usbc_platform_data *usbpd_data = g_usbc_data;
	int rid_cnt = usbpd_data->factory_mode.FAC_Abnormal_Repeat_RID;

	msg_maxim("IN ");

	if (rid_cnt >= FAC_ABNORMAL_REPEAT_RID) {
		msg_maxim("Notify the abnormal state [RID] [ %d]", rid_cnt);
		max77775_ccic_event_work(usbpd_data,
			PDIC_NOTIFY_DEV_CCIC, PDIC_NOTIFY_ID_FAC, 1 << 1, 0, 0);
	} else
		msg_maxim("[RID] cnt :  [%d]", rid_cnt);

	usbpd_data->factory_mode.FAC_Abnormal_Repeat_RID = 0;
	msg_maxim("OUT ");
}

void factory_execute_monitor(int type)
{
	struct max77775_usbc_platform_data *usbpd_data = g_usbc_data;
	uint32_t state_cnt = usbpd_data->factory_mode.FAC_Abnormal_Repeat_State;
	uint32_t rid_cnt = usbpd_data->factory_mode.FAC_Abnormal_Repeat_RID;
	uint32_t rid0_cnt = usbpd_data->factory_mode.FAC_Abnormal_RID0;

	switch (type) {
	case FAC_ABNORMAL_REPEAT_RID0:
		if (!rid0_cnt) {
			msg_maxim("Notify the abnormal state [RID0] [%d]!!", rid0_cnt);
			usbpd_data->factory_mode.FAC_Abnormal_RID0++;
			max77775_ccic_event_work(usbpd_data,
				PDIC_NOTIFY_DEV_CCIC, PDIC_NOTIFY_ID_FAC, 1 << 2, 0, 0);
		} else {
			usbpd_data->factory_mode.FAC_Abnormal_RID0 = 0;
		}
	break;
	case FAC_ABNORMAL_REPEAT_RID:
		if (!rid_cnt) {
			schedule_delayed_work(&usbpd_data->factory_rid_work, msecs_to_jiffies(1000));
			msg_maxim("start the factory_rid_work");
		}
		usbpd_data->factory_mode.FAC_Abnormal_Repeat_RID++;
	break;
	case FAC_ABNORMAL_REPEAT_STATE:
		if (!state_cnt) {
			schedule_delayed_work(&usbpd_data->factory_state_work, msecs_to_jiffies(1000));
			msg_maxim("start the factory_state_work");
		}
		usbpd_data->factory_mode.FAC_Abnormal_Repeat_State++;
	break;
	default:
		msg_maxim("Never Calling [%d]", type);
	break;
	}
}
#endif

#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
static void max77775_usbpd_set_host_on(void *data, int mode)
{
	struct max77775_usbc_platform_data *usbpd_data = data;

	if (!usbpd_data)
		return;

	md75_info_usb("%s : current_set is %d!\n", __func__, mode);
	if (mode) {
		usbpd_data->device_add = 0;
		usbpd_data->detach_done_wait = 0;
		usbpd_data->host_turn_on_event = 1;
		wake_up_interruptible(&usbpd_data->host_turn_on_wait_q);
	} else {
		usbpd_data->device_add = 0;
		usbpd_data->detach_done_wait = 0;
		usbpd_data->host_turn_on_event = 0;
	}
}

static void max77775_usbpd_wait_entermode(void *data, int on)
{
	struct max77775_usbc_platform_data *usbpd_data = data;

	if (!usbpd_data)
		return;

	md75_info_usb("%s : %d!\n", __func__, on);
	if (on) {
		usbpd_data->wait_entermode = 1;
	} else {
		usbpd_data->wait_entermode = 0;
		wake_up_interruptible(&usbpd_data->host_turn_on_wait_q);
	}
}

static void max77775_usbpd_sbu_switch_control(void *data, int on)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	usbc_cmd_data write_data;
	int mode = UNDEFINED;

	md75_info_usb("%s : on: %d\n", __func__, on);

	if (!usbpd_data)
		return;

	if (usbpd_data->sbu_switch_status == on) {
		md75_info_usb("%s : requested value is already set\n", __func__);
		return;
	}

	usbpd_data->sbu_switch_status = on;

	switch (on) {
	case OPEN_SBU:
		mode = 0x00;
		break;
	case CLOSE_SBU_CC1_ACTIVE:
		mode = 0x09;
		break;
	case CLOSE_SBU_CC2_ACTIVE:
		mode = 0x12;
		break;
	default:
		md75_info_usb("%s : unproper value\n", __func__);
		return;
	}

	md75_info_usb("%s : sbu_status: %d mode; 0x%x\n", __func__, usbpd_data->sbu_switch_status, mode);

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_SBU_CTRL1_W;
	write_data.write_data[0] = mode;
	write_data.write_length = 0x1;
	write_data.read_length = 0x0;

	max77775_usbc_opcode_write(usbpd_data, &write_data);
}

void max77775_acc_detach_check(struct work_struct *wk)
{
	struct delayed_work *delay_work =
		container_of(wk, struct delayed_work, work);
	struct max77775_usbc_platform_data *usbpd_data =
		container_of(delay_work, struct max77775_usbc_platform_data, acc_detach_work);
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
	struct otg_notify *o_notify = get_otg_notify();
#endif

	md75_info_usb("%s : pd_state : %d, acc_type : %d\n", __func__,
		usbpd_data->pd_state, usbpd_data->acc_type);
	if (usbpd_data->pd_state == max77775_State_PE_Initial_detach) {
		if (usbpd_data->acc_type != PDIC_DOCK_DETACHED) {
			if (usbpd_data->acc_type != PDIC_DOCK_NEW)
				pdic_send_dock_intent(PDIC_DOCK_DETACHED);
			pdic_send_dock_uevent(usbpd_data->Vendor_ID,
					usbpd_data->Product_ID,
					PDIC_DOCK_DETACHED);
			usbpd_data->acc_type = PDIC_DOCK_DETACHED;
			usbpd_data->Vendor_ID = 0;
			usbpd_data->Product_ID = 0;
			usbpd_data->send_enter_mode_req = 0;
			usbpd_data->Device_Version = 0;
			max77775_ccic_event_work(usbpd_data,
				PDIC_NOTIFY_DEV_ALL, PDIC_NOTIFY_ID_CLEAR_INFO,
				PDIC_NOTIFY_ID_DEVICE_INFO, 0, 0);
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
			if (o_notify)
				send_otg_notify(o_notify, NOTIFY_EVENT_HMD_EXT_CURRENT, 0);
#endif
		}
	}
}

struct usbpd_ops ops_usbpd = {
	.usbpd_set_host_on = max77775_usbpd_set_host_on,
	.usbpd_wait_entermode = max77775_usbpd_wait_entermode,
	.usbpd_sbu_switch_control = max77775_usbpd_sbu_switch_control,
};
#endif

static int max77775_usbc_probe(struct platform_device *pdev)
{
	struct max77775_dev *max77775 = dev_get_drvdata(pdev->dev.parent);
	struct max77775_platform_data *pdata = dev_get_platdata(max77775->dev);
	struct max77775_usbc_platform_data *usbc_data = NULL;
	int ret = 0, i = 0;
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	ppdic_data_t ppdic_data;
	ppdic_sysfs_property_t ppdic_sysfs_prop;
#endif
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
	struct otg_notify *o_notify = get_otg_notify();
#endif
#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
	struct usbpd_dev *usbpd_d;
#endif
	struct max77775_hmd_power_dev *hmd_list;

	msg_maxim("Probing : %d", max77775->irq);
	usbc_data =  devm_kzalloc(&pdev->dev,
		sizeof(struct max77775_usbc_platform_data), GFP_KERNEL);
	if (!usbc_data) {
		err_maxim("usbc_data is null");
		ret = -ENOMEM;
		goto err_usbc_data;
	}

	g_usbc_data = usbc_data;
	usbc_data->dev = &pdev->dev;
	usbc_data->max77775 = max77775;
	usbc_data->muic = max77775->muic;
	usbc_data->charger = max77775->charger;
	usbc_data->i2c = max77775->i2c;
	usbc_data->max77775_data = pdata;
	usbc_data->irq_base = pdata->irq_base;

	usbc_data->pd_data = devm_kzalloc(&pdev->dev,
		sizeof(struct max77775_pd_data), GFP_KERNEL);
	if (!usbc_data->pd_data) {
		err_maxim("pd_data is null");
		ret = -ENOMEM;
		goto err_cc_pd_data;
	}

	usbc_data->cc_data = devm_kzalloc(&pdev->dev,
		sizeof(struct max77775_cc_data), GFP_KERNEL);
	if (!usbc_data->cc_data) {
		err_maxim("cc_data is null");
		ret = -ENOMEM;
		goto err_cc_pd_data;
	}


	platform_set_drvdata(pdev, usbc_data);

	ret = sysfs_create_group(&max77775->dev->kobj, &max77775_attr_grp);
	msg_maxim("created sysfs. ret=%d", ret);

	usbc_data->HW_Revision = 0x0;
	usbc_data->FW_Revision = 0x0;
	usbc_data->plug_attach_done = 0x0;
	usbc_data->cc_data->current_pr = 0xFF;
	usbc_data->pd_data->current_dr = 0xFF;
	usbc_data->cc_data->current_vcon = 0xFF;
	usbc_data->op_code_done = 0x0;
	usbc_data->current_connstat = 0xFF;
	usbc_data->prev_connstat = 0xFF;
	usbc_data->usbc_cmd_queue.front = NULL;
	usbc_data->usbc_cmd_queue.rear = NULL;
	usbc_data->opcode_stamp = 0;
	mutex_init(&usbc_data->op_lock);
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	ppdic_data = devm_kzalloc(&pdev->dev, sizeof(pdic_data_t), GFP_KERNEL);
	if (!ppdic_data) {
		err_maxim("ppdic_data is null");
		ret = -ENOMEM;
		goto err_ppdic;
	}

	ppdic_sysfs_prop = devm_kzalloc(&pdev->dev, sizeof(pdic_sysfs_property_t), GFP_KERNEL);
	if (!ppdic_sysfs_prop) {
		err_maxim("ppdic_sysfs_prop is null");
		ret = -ENOMEM;
		goto err_ppdic;
	}

	ppdic_sysfs_prop->get_property = max77775_sysfs_get_local_prop;
	ppdic_sysfs_prop->set_property = max77775_sysfs_set_prop;
	ppdic_sysfs_prop->property_is_writeable = max77775_sysfs_is_writeable;
	ppdic_sysfs_prop->property_is_writeonly = max77775_sysfs_is_writeonly;
	ppdic_sysfs_prop->properties = max77775_sysfs_properties;
	ppdic_sysfs_prop->num_properties = ARRAY_SIZE(max77775_sysfs_properties);
	ppdic_data->pdic_sysfs_prop = ppdic_sysfs_prop;
	ppdic_data->drv_data = usbc_data;
	ppdic_data->name = "max77775";
	ppdic_data->set_enable_alternate_mode = max77775_set_enable_alternate_mode;
	pdic_core_register_chip(ppdic_data);
	usbc_data->vconn_en = 1;
	usbc_data->cur_rid = RID_OPEN;
	usbc_data->cc_pin_status = NO_DETERMINATION;
#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
	usbpd_d = devm_kzalloc(&pdev->dev,
		sizeof(struct usbpd_dev), GFP_KERNEL);
	if (!usbpd_d) {
		err_maxim("failed to allocate usbpd data");
		ret = -ENOMEM;
		goto err_usbpd_d;
	}

	usbpd_d->ops = &ops_usbpd;
	usbpd_d->data = (void *)usbc_data;
	usbc_data->man = register_usbpd(usbpd_d);
#endif
	usbc_data->typec_cap.revision = USB_TYPEC_REV_1_2;
	usbc_data->typec_cap.pd_revision = 0x300;
	usbc_data->typec_cap.prefer_role = TYPEC_NO_PREFERRED_ROLE;

	usbc_data->typec_cap.driver_data = usbc_data;
	usbc_data->typec_cap.ops = &max77775_ops;

	usbc_data->typec_cap.type = TYPEC_PORT_DRP;
	usbc_data->typec_cap.data = TYPEC_PORT_DRD;

	usbc_data->typec_power_role = TYPEC_SINK;
	usbc_data->typec_try_state_change = TRY_ROLE_SWAP_NONE;

	usbc_data->port = typec_register_port(usbc_data->dev, &usbc_data->typec_cap);
	if (IS_ERR(usbc_data->port))
		md75_err_usb("unable to register typec_register_port\n");
	else
		msg_maxim("success typec_register_port port=%pK", usbc_data->port);
	init_completion(&usbc_data->typec_reverse_completion);

	usbc_data->auto_vbus_en = false;
	usbc_data->is_first_booting = 1;
	usbc_data->pd_support = false;
	usbc_data->ccrp_state = 0;
	usbc_data->set_altmode = 0;
	usbc_data->set_altmode_error = 0;
	usbc_data->need_recover = false;
	usbc_data->op_ctrl1_w = (BIT_CCSrcSnk | BIT_CCSnkSrc);
	usbc_data->srcccap_request_retry = false;
#if defined(CONFIG_SUPPORT_SHIP_MODE)
	usbc_data->ship_mode_en = 0;
	usbc_data->ship_mode_data = 0x00;
#endif
	usbc_data->rid_check = false;
	usbc_data->lapse_idx = 0;
	for (i = 0; i < MAX_NVCN_CNT; i++)
		usbc_data->time_lapse[i] = 0;

#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
	send_otg_notify(o_notify, NOTIFY_EVENT_POWER_SOURCE, 0);
#endif
#endif
	init_completion(&usbc_data->op_completion);
	init_completion(&usbc_data->ccic_sysfs_completion);
	init_completion(&usbc_data->psrdy_wait);
	usbc_data->op_wait_queue = create_singlethread_workqueue("op_wait");
	if (usbc_data->op_wait_queue == NULL)
		goto err_op_wait_queue;
	usbc_data->op_send_queue = create_singlethread_workqueue("op_send");
	if (usbc_data->op_send_queue == NULL)
		goto err_op_send_queue;

	INIT_WORK(&usbc_data->op_send_work, max77775_uic_op_send_work_func);
	INIT_WORK(&usbc_data->cc_open_req_work, max77775_cc_open_work_func);
	INIT_WORK(&usbc_data->dp_configure_work, max77775_dp_configure_work_func);
#if defined(MAX77775_SYS_FW_UPDATE)
	INIT_WORK(&usbc_data->fw_update_work,
			max77775_firmware_update_sysfs_work);
#endif

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	/* Create a work queue for the ccic irq thread */
	usbc_data->ccic_wq
		= create_singlethread_workqueue("ccic_irq_event");
	if (!usbc_data->ccic_wq) {
		md75_err_usb("%s failed to create work queue\n", __func__);
		goto err_ccic_wq;
	}
#endif
#if defined(CONFIG_SEC_FACTORY)
	INIT_DELAYED_WORK(&usbc_data->factory_state_work,
				factory_check_abnormal_state);
	INIT_DELAYED_WORK(&usbc_data->factory_rid_work,
				factory_check_normal_rid);
#endif
	INIT_DELAYED_WORK(&usbc_data->vbus_hard_reset_work,
				vbus_control_hard_reset);
	/* turn on the VBUS automatically. */
	// max77775_usbc_enable_auto_vbus(usbc_data);
	INIT_DELAYED_WORK(&usbc_data->acc_detach_work, max77775_acc_detach_check);

	pdic_register_switch_device(1);
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
	INIT_DELAYED_WORK(&usbc_data->usb_external_notifier_register_work,
				  delayed_external_notifier_init);
#endif
#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
	INIT_DELAYED_WORK(&usbc_data->set_ccopen_for_watercable_work,
				  max77775_set_ccopen_for_watercable);
#endif
	init_completion(&usbc_data->uvdm_longpacket_out_wait);
	init_completion(&usbc_data->uvdm_longpacket_in_wait);
	init_completion(&usbc_data->cc_open_work_completion);
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	ppdic_data->fw_data.data = (void *)usbc_data;
	ppdic_data->fw_data.firmware_update = max77775_firmware_update_callback;
	ppdic_data->fw_data.get_prev_fw_size = max77775_get_firmware_size;
	ret = pdic_misc_init(ppdic_data);
	if (ret) {
		md75_err_usb("pdic_misc_init is failed, error %d\n", ret);
	} else {
		ppdic_data->misc_dev->uvdm_read = max77775_sec_uvdm_in_request_message;
		ppdic_data->misc_dev->uvdm_write = max77775_sec_uvdm_out_request_message;
		ppdic_data->misc_dev->uvdm_ready = max77775_sec_uvdm_ready;
		ppdic_data->misc_dev->uvdm_close = max77775_sec_uvdm_close;
		ppdic_data->misc_dev->pps_control = max77775_sec_pps_control;
		usbc_data->ppdic_data = ppdic_data;
	}
#endif

	hmd_list = devm_kzalloc(&pdev->dev, MAX_NUM_HMD * sizeof(*hmd_list),
				GFP_KERNEL);
	if (ZERO_OR_NULL_PTR(hmd_list)) {
		usbc_data->hmd_list = NULL;
		goto err_hmd_list;
	}

	/* add default AR/VR here */
	snprintf(hmd_list[0].hmd_name, NAME_LEN_HMD, "PicoVR");
	hmd_list[0].vid  = 0x2d40;
	hmd_list[0].pid = 0x0000;

	usbc_data->hmd_list = hmd_list;

	mutex_init(&usbc_data->hmd_power_lock);

#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
	/* Register pdic handler to pdic notifier block list */
	ret = usb_external_notify_register(&usbc_data->usb_external_notifier_nb,
		pdic_handle_usb_external_notifier_notification, EXTERNAL_NOTIFY_DEV_PDIC);
	if (ret < 0)
		schedule_delayed_work(&usbc_data->usb_external_notifier_register_work, msecs_to_jiffies(2000));
	else
		md75_info_usb("%s : external notifier register done!\n", __func__);
#endif

	init_waitqueue_head(&usbc_data->host_turn_on_wait_q);
	init_waitqueue_head(&usbc_data->device_add_wait_q);
#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
	max77775_usbpd_set_host_on(usbc_data, 0);
#endif
#if IS_ENABLED(CONFIG_USB_DWC3_MSM)
	usbc_data->host_turn_on_wait_time = 15;
#else
	usbc_data->host_turn_on_wait_time = 3;
#endif

	max77775_get_version_info(usbc_data);
	max77775_init_irq_handler(usbc_data);
#ifdef CONFIG_MUIC_MAX77775
	ret = max77775_muic_probe(usbc_data);
	if (ret) {
		msg_maxim("max77775_muic_probe error.");
		goto err_muic_probe;
	}
#endif
	max77775_cc_init(usbc_data);
	max77775_pd_init(usbc_data);
	max77775_write_reg(usbc_data->muic, REG_PD_INT_M, 0x1C);
	max77775_write_reg(usbc_data->muic, REG_VDM_INT_M, 0xFF);
	max77775_init_opcode(usbc_data, 0);
	max77775->cc_booting_complete = 1;
	max77775_usbc_umask_irq(usbc_data);

	usbc_data->cc_open_req = 1;
	max77775_pdic_manual_ccopen_request(0);

	msg_maxim("probing Complete..");
	return 0;

err_muic_probe:
	max77775_usbc_free_irq(usbc_data);
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
	if (delayed_work_pending(&usbc_data->usb_external_notifier_register_work))
		cancel_delayed_work_sync(&usbc_data->usb_external_notifier_register_work);
	usb_external_notify_unregister(&usbc_data->usb_external_notifier_nb);
#endif
	mutex_destroy(&usbc_data->hmd_power_lock);
	usbc_data->hmd_list = NULL;
err_hmd_list:
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	if (usbc_data->ppdic_data && usbc_data->ppdic_data->misc_dev)
		pdic_misc_exit();
#endif
	pdic_register_switch_device(0);
err_ccic_wq:
err_op_send_queue:
err_op_wait_queue:
	typec_unregister_port(usbc_data->port);
#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
	register_usbpd(NULL);
#endif
err_usbpd_d:
	pdic_core_unregister_chip();
err_ppdic:
	mutex_destroy(&usbc_data->op_lock);
	sysfs_remove_group(&max77775->dev->kobj, &max77775_attr_grp);
	platform_set_drvdata(pdev, NULL);
err_cc_pd_data:
	g_usbc_data = NULL;
err_usbc_data:

	return ret;
}

static int max77775_usbc_remove(struct platform_device *pdev)
{
	struct max77775_usbc_platform_data *usbc_data =
		platform_get_drvdata(pdev);
	struct max77775_dev *max77775 = usbc_data->max77775;

	max77775_usbc_free_irq(usbc_data);
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
	if (delayed_work_pending(&usbc_data->usb_external_notifier_register_work))
		cancel_delayed_work_sync(&usbc_data->usb_external_notifier_register_work);
	usb_external_notify_unregister(&usbc_data->usb_external_notifier_nb);
#endif
	mutex_destroy(&usbc_data->hmd_power_lock);
	usbc_data->hmd_list = NULL;
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	if (usbc_data->ppdic_data && usbc_data->ppdic_data->misc_dev)
		pdic_misc_exit();
#endif
	pdic_register_switch_device(0);

	typec_unregister_port(usbc_data->port);
#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
	register_usbpd(NULL);
#endif

	pdic_core_unregister_chip();
	mutex_destroy(&usbc_data->op_lock);
	sysfs_remove_group(&max77775->dev->kobj, &max77775_attr_grp);

	mutex_destroy(&usbc_data->hmd_power_lock);
	mutex_destroy(&usbc_data->op_lock);
	pdic_core_unregister_chip();

	typec_unregister_port(usbc_data->port);

	pdic_register_switch_device(0);
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	if (usbc_data->ppdic_data && usbc_data->ppdic_data->misc_dev)
		pdic_misc_exit();
#endif
#ifdef CONFIG_MUIC_MAX77775
	max77775_muic_remove(usbc_data);
#endif
	platform_set_drvdata(pdev, NULL);
	g_usbc_data = NULL;

	return 0;
}

#if defined CONFIG_PM
static int max77775_usbc_suspend(struct device *dev)
{
	struct max77775_usbc_platform_data *usbc_data =
		dev_get_drvdata(dev);

	max77775_muic_suspend(usbc_data);

	return 0;
}

static int max77775_usbc_resume(struct device *dev)
{
	struct max77775_usbc_platform_data *usbc_data =
		dev_get_drvdata(dev);

	max77775_muic_resume(usbc_data);
	if (usbc_data->set_altmode_error) {
		msg_maxim("set alternate mode");
		max77775_set_enable_alternate_mode
			(usbc_data->set_altmode);
	}

	return 0;
}
#else
#define max77775_usbc_suspend NULL
#define max77775_usbc_resume NULL
#endif

static void max77775_usbc_disable_irq(struct max77775_usbc_platform_data *usbc_data)
{
	disable_irq(usbc_data->irq_usbid);
#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
	disable_irq(usbc_data->irq_taconn);
#endif
	disable_irq(usbc_data->irq_apcmd);
	disable_irq(usbc_data->irq_sysmsg);
	disable_irq(usbc_data->irq_vdm0);
	disable_irq(usbc_data->irq_vdm1);
	disable_irq(usbc_data->irq_vdm2);
	disable_irq(usbc_data->irq_vdm3);
	disable_irq(usbc_data->irq_vdm4);
	disable_irq(usbc_data->irq_vdm5);
	disable_irq(usbc_data->irq_vdm6);
	disable_irq(usbc_data->irq_vir0);
	disable_irq(usbc_data->pd_data->irq_pdmsg);
	disable_irq(usbc_data->pd_data->irq_psrdy);
	disable_irq(usbc_data->pd_data->irq_datarole);
	disable_irq(usbc_data->pd_data->irq_ssacc);
	disable_irq(usbc_data->pd_data->irq_fct_id);
	disable_irq(usbc_data->cc_data->irq_vconncop);
	disable_irq(usbc_data->cc_data->irq_vsafe0v);
	disable_irq(usbc_data->cc_data->irq_vconnsc);
	disable_irq(usbc_data->cc_data->irq_ccpinstat);
	disable_irq(usbc_data->cc_data->irq_ccistat);
	disable_irq(usbc_data->cc_data->irq_ccvcnstat);
	disable_irq(usbc_data->cc_data->irq_ccstat);
}

static void max77775_usbc_shutdown(struct platform_device *pdev)
{
	struct max77775_usbc_platform_data *usbc_data =
		platform_get_drvdata(pdev);
	u8 status = 0, uic_int = 0, vbus = 0;
	u8 uid = 0;
#if defined(CONFIG_SUPPORT_SHIP_MODE)
	u8 op_data = 0;
#endif

	msg_maxim("max77775 usbc driver shutdown++++");
	if (!usbc_data->muic) {
		msg_maxim("no max77775 i2c client");
		return;
	}
#ifdef CONFIG_MUIC_MAX77775
	max77775_muic_shutdown(usbc_data);
#endif
	max77775_usbc_mask_irq(usbc_data);
	/* unmask */
	max77775_write_reg(usbc_data->muic, REG_PD_INT_M, 0xFF);
	max77775_write_reg(usbc_data->muic, REG_CC_INT_M, 0xFF);
	max77775_write_reg(usbc_data->muic, REG_UIC_INT_M, 0xFF);
	max77775_write_reg(usbc_data->muic, REG_VDM_INT_M, 0xFF);
	max77775_write_reg(usbc_data->muic, REG_SPARE_INT_M, 0xFF);

	max77775_usbc_disable_irq(usbc_data);
	max77775_usbc_free_irq(usbc_data);

	usbc_data->shut_down = 1;

	max77775_read_reg(usbc_data->muic, REG_USBC_STATUS1, &status);
	uid = (status & BIT_UIDADC) >> FFS(BIT_UIDADC);
	vbus = (status & BIT_VBADC) >> FFS(BIT_VBADC);

	/* send the reset command */
	if (usbc_data->current_connstat == WATER) {
#if defined(CONFIG_MAX77775_UPGRADED_WATER_DETECT)
		msg_maxim("WATER STATE, %s, %s", vbus ? "VBUS" : "NO VBUS",
					usbc_data->muic_data->is_hiccup_mode ? "HICCUP MODE" : "NOT HICCUP MODE");
		if (!vbus && !usbc_data->muic_data->is_hiccup_mode) {
			msg_maxim("call reset function");
			max77775_reset_ic(usbc_data);
		} else
			msg_maxim("don't call the reset function");
#else
		msg_maxim("no call the reset function(WATER STATE)");
#endif
	} else if (usbc_data->cur_rid != RID_OPEN && usbc_data->cur_rid != RID_UNDEFINED)
		msg_maxim("no call the reset function(RID)");
	else if (uid != UID_Open)
		msg_maxim("no call the reset function(UID)");
	else {
		max77775_reset_ic(usbc_data);
#if defined(CONFIG_SUPPORT_SHIP_MODE)
		if (usbc_data->ship_mode_en) {
			md75_info_usb("%s: reset_ic before ship_mode_en(%d, 0x%x)\n",
				__func__, usbc_data->ship_mode_en, usbc_data->ship_mode_data);
			op_data = 0x08;
			op_data |= (usbc_data->ship_mode_data << 6);
			msleep(50);
			max77775_write_reg(usbc_data->muic, OPCODE_WRITE, 0x97); //auto ship mode
			max77775_write_reg(usbc_data->muic, OPCODE_DATAOUT1, op_data);
			max77775_write_reg(usbc_data->muic, OPCODE_WRITE_END, 0x00);
			msleep(50);
			max77775_read_reg(usbc_data->muic,
				MAX77775_USBC_REG_UIC_INT, &uic_int);
		}
#endif
		max77775_write_reg(usbc_data->muic, REG_PD_INT_M, 0xFF);
		max77775_write_reg(usbc_data->muic, REG_CC_INT_M, 0xFF);
		max77775_write_reg(usbc_data->muic, REG_UIC_INT_M, 0xFF);
		max77775_write_reg(usbc_data->muic, REG_VDM_INT_M, 0xFF);
		max77775_write_reg(usbc_data->muic, REG_SPARE_INT_M, 0xFF);
		max77775_read_reg(usbc_data->muic,
				MAX77775_USBC_REG_UIC_INT, &uic_int);
	}
	msg_maxim("max77775 usbc driver shutdown----");
}

static SIMPLE_DEV_PM_OPS(max77775_usbc_pm_ops, max77775_usbc_suspend,
			 max77775_usbc_resume);

static struct platform_driver max77775_usbc_driver = {
	.driver = {
		.name = "max77775-usbc",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &max77775_usbc_pm_ops,
#endif
	},
	.shutdown = max77775_usbc_shutdown,
	.probe = max77775_usbc_probe,
	.remove = max77775_usbc_remove,
};

static int __init max77775_usbc_init(void)
{
	msg_maxim("init");
	return platform_driver_register(&max77775_usbc_driver);
}
device_initcall(max77775_usbc_init);

static void __exit max77775_usbc_exit(void)
{
	platform_driver_unregister(&max77775_usbc_driver);
}
module_exit(max77775_usbc_exit);

MODULE_DESCRIPTION("max77775 USBPD driver");
MODULE_LICENSE("GPL");
