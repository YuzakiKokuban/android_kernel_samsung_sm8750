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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/mod_devicetable.h>
#include <linux/mfd/max77775_log.h>
#include <linux/mfd/max77775-private.h>
#include <linux/completion.h>
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
#include <linux/usb_notify.h>
#endif
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
#include <linux/usb/typec/common/pdic_core.h>
#include <linux/usb/typec/common/pdic_notifier.h>
#endif
#include <linux/usb/typec/maxim/max77775_kunit.h>
#include <linux/usb/typec/maxim/max77775_usbc.h>
#include <linux/usb/typec/maxim/max77775_alternate.h>

#define UVDM_DEBUG (0)
#define SEC_UVDM_ALIGN		(4)
#define MAX_DATA_FIRST_UVDMSET	12
#define MAX_DATA_NORMAL_UVDMSET	16
#define CHECKSUM_DATA_COUNT		20
#define MAX_INPUT_DATA (255)

extern struct max77775_usbc_platform_data *g_usbc_data;

const struct DP_DP_DISCOVER_IDENTITY DP_DISCOVER_IDENTITY = {
	{
		.BITS.Num_Of_VDO = 1,
		.BITS.Cmd_Type = ACK,
		.BITS.Reserved = 0
	},

	{
		.BITS.VDM_command = Discover_Identity,
		.BITS.Rsvd2_VDM_header = 0,
		.BITS.VDM_command_type = REQ,
		.BITS.Object_Position = 0,
		.BITS.Rsvd_VDM_header = 0,
		.BITS.Structured_VDM_Version = Version_1_0,
		.BITS.VDM_Type = STRUCTURED_VDM,
		.BITS.Standard_Vendor_ID = 0xFF00
	}

};
const struct DP_DP_DISCOVER_ENTER_MODE DP_DISCOVER_ENTER_MODE = {
	{
		.BITS.Num_Of_VDO = 1,
		.BITS.Cmd_Type = ACK,
		.BITS.Reserved = 0
	},
	{
		.BITS.VDM_command = Enter_Mode,
		.BITS.Rsvd2_VDM_header = 0,
		.BITS.VDM_command_type = REQ,
		.BITS.Object_Position = 1,
		.BITS.Rsvd_VDM_header = 0,
		.BITS.Structured_VDM_Version = Version_1_0,
		.BITS.VDM_Type = STRUCTURED_VDM,
		.BITS.Standard_Vendor_ID = 0xFF01
	}
};

struct DP_DP_CONFIGURE DP_CONFIGURE = {
	{
		.BITS.Num_Of_VDO = 2,
		.BITS.Cmd_Type = ACK,
		.BITS.Reserved = 0
	},
	{
		.BITS.VDM_command = 17, /* SVID Specific Command */
		.BITS.Rsvd2_VDM_header = 0,
		.BITS.VDM_command_type = REQ,
		.BITS.Object_Position = 1,
		.BITS.Rsvd_VDM_header = 0,
		.BITS.Structured_VDM_Version = Version_1_0,
		.BITS.VDM_Type = STRUCTURED_VDM,
		.BITS.Standard_Vendor_ID = 0xFF01
	},
	{
		.BITS.SEL_Configuration = num_Cfg_UFP_U_as_UFP_D,
		.BITS.Select_DP_V1p3 = 1,
		.BITS.Select_USB_Gen2 = 0,
		.BITS.Select_Reserved_1 = 0,
		.BITS.Select_Reserved_2 = 0,
		.BITS.DFP_D_PIN_Assign_A = 0,
		.BITS.DFP_D_PIN_Assign_B = 0,
		.BITS.DFP_D_PIN_Assign_C = 0,
		.BITS.DFP_D_PIN_Assign_D = 1,
		.BITS.DFP_D_PIN_Assign_E = 0,
		.BITS.DFP_D_PIN_Assign_F = 0,
		.BITS.DFP_D_PIN_Reserved = 0,
		.BITS.UFP_D_PIN_Assign_A = 0,
		.BITS.UFP_D_PIN_Assign_B = 0,
		.BITS.UFP_D_PIN_Assign_C = 0,
		.BITS.UFP_D_PIN_Assign_D = 0,
		.BITS.UFP_D_PIN_Assign_E = 0,
		.BITS.UFP_D_PIN_Assign_F = 0,
		.BITS.UFP_D_PIN_Reserved = 0,
		.BITS.DP_MODE_Reserved = 0
	}
};

struct SS_DEX_DISCOVER_MODE SS_DEX_DISCOVER_MODE_MODE = {
	{
		.BITS.Num_Of_VDO = 1,
		.BITS.Cmd_Type = ACK,
		.BITS.Reserved = 0
	},
	{
		.BITS.VDM_command = Discover_Modes,
		.BITS.Rsvd2_VDM_header = 0,
		.BITS.VDM_command_type = REQ,
		.BITS.Object_Position = 0,
		.BITS.Rsvd_VDM_header = 0,
		.BITS.Structured_VDM_Version = Version_1_0,
		.BITS.VDM_Type = STRUCTURED_VDM,
		.BITS.Standard_Vendor_ID = SAMSUNG_VENDOR_ID
	}
};

struct SS_DEX_ENTER_MODE SS_DEX_DISCOVER_ENTER_MODE = {
	{
		.BITS.Num_Of_VDO = 1,
		.BITS.Cmd_Type = ACK,
		.BITS.Reserved = 0
	},
	{
		.BITS.VDM_command = Enter_Mode,
		.BITS.Rsvd2_VDM_header = 0,
		.BITS.VDM_command_type = REQ,
		.BITS.Object_Position = 1,
		.BITS.Rsvd_VDM_header = 0,
		.BITS.Structured_VDM_Version = Version_1_0,
		.BITS.VDM_Type = STRUCTURED_VDM,
		.BITS.Standard_Vendor_ID = SAMSUNG_VENDOR_ID
	}
};

struct SS_UNSTRUCTURED_VDM_MSG SS_DEX_UNSTRUCTURED_VDM;

static char vdm_msg_irq_state_print[9][40] = {
	{"bFLAG_Vdm_Reserve_b0"},
	{"bFLAG_Vdm_Discover_ID"},
	{"bFLAG_Vdm_Discover_SVIDs"},
	{"bFLAG_Vdm_Discover_MODEs"},
	{"bFLAG_Vdm_Enter_Mode"},
	{"bFLAG_Vdm_Exit_Mode"},
	{"bFLAG_Vdm_Attention"},
	{"bFlag_Vdm_DP_Status_Update"},
	{"bFlag_Vdm_DP_Configure"},
};
static char dp_pin_assignment_print[7][40] = {
	{"DP_Pin_Assignment_None"},
	{"DP_Pin_Assignment_A"},
	{"DP_Pin_Assignment_B"},
	{"DP_Pin_Assignment_C"},
	{"DP_Pin_Assignment_D"},
	{"DP_Pin_Assignment_E"},
	{"DP_Pin_Assignment_F"},
};

static uint8_t dp_pin_assignment_data[7] = {
	DP_PIN_ASSIGNMENT_NODE,
	DP_PIN_ASSIGNMENT_A,
	DP_PIN_ASSIGNMENT_B,
	DP_PIN_ASSIGNMENT_C,
	DP_PIN_ASSIGNMENT_D,
	DP_PIN_ASSIGNMENT_E,
	DP_PIN_ASSIGNMENT_F,
};

bool max77775_check_hmd_dev(struct max77775_usbc_platform_data *usbpd_data)
{
	struct max77775_hmd_power_dev *hmd_list;
	int i;
	bool ret = false;
	uint16_t vid = usbpd_data->Vendor_ID;
	uint16_t pid = usbpd_data->Product_ID;

	if (!vid && !pid)
		return ret;
	hmd_list = usbpd_data->hmd_list;
	if (!hmd_list) {
		msg_maxim("hmd_list is null!");
		return ret;
	}
	for (i = 0; i < MAX_NUM_HMD; i++) {
		if (strlen(hmd_list[i].hmd_name) > 0)
			msg_maxim("%s,0x%04x,0x%04x",
				hmd_list[i].hmd_name,
				hmd_list[i].vid,
				hmd_list[i].pid);
	}
	for (i = 0; i < MAX_NUM_HMD; i++) {
		if (hmd_list[i].hmd_name[0]) {
			if (vid == hmd_list[i].vid && pid == hmd_list[i].pid) {
				msg_maxim("hmd found %s,0x%04x,0x%04x",
					hmd_list[i].hmd_name,
					hmd_list[i].vid,
					hmd_list[i].pid);
				ret = true;
				break;
			}
			continue;
		}
		break;
	}

	return ret;
}

int max77775_process_check_accessory(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
#if defined(CONFIG_USB_HW_PARAM) || IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
	struct otg_notify *o_notify = get_otg_notify();
#endif
	uint16_t vid = usbpd_data->Vendor_ID;
	uint16_t pid = usbpd_data->Product_ID;
	uint16_t acc_type = PDIC_DOCK_DETACHED;

	if (usbpd_data->usb_mock.check_accessory)
		return usbpd_data->usb_mock.check_accessory(data);

	/* detect Gear VR */
	if (usbpd_data->acc_type == PDIC_DOCK_DETACHED) {
		if (vid == SAMSUNG_VENDOR_ID) {
			switch (pid) {
			/* GearVR: Reserved GearVR PID+6 */
			case GEARVR_PRODUCT_ID:
			case GEARVR_PRODUCT_ID_1:
			case GEARVR_PRODUCT_ID_2:
			case GEARVR_PRODUCT_ID_3:
			case GEARVR_PRODUCT_ID_4:
			case GEARVR_PRODUCT_ID_5:
				acc_type = PDIC_DOCK_HMT;
				msg_maxim("Samsung Gear VR connected.");
#if defined(CONFIG_USB_HW_PARAM)
				if (o_notify)
					inc_hw_param(o_notify, USB_CCIC_VR_USE_COUNT);
#endif
				break;
			case DEXDOCK_PRODUCT_ID:
				acc_type = PDIC_DOCK_DEX;
				msg_maxim("Samsung DEX connected.");
#if defined(CONFIG_USB_HW_PARAM)
				if (o_notify)
					inc_hw_param(o_notify, USB_CCIC_DEX_USE_COUNT);
#endif
				break;
			case DEXPAD_PRODUCT_ID:
				acc_type = PDIC_DOCK_DEXPAD;
				msg_maxim("Samsung DEX PAD connected.");
#if defined(CONFIG_USB_HW_PARAM)
				if (o_notify)
					inc_hw_param(o_notify, USB_CCIC_DEX_USE_COUNT);
#endif
				break;
			case HDMI_PRODUCT_ID:
				acc_type = PDIC_DOCK_HDMI;
				msg_maxim("Samsung HDMI connected.");
				break;
			default:
				msg_maxim("default device connected.");
				acc_type = PDIC_DOCK_NEW;
				break;
			}
		} else if (vid == SAMSUNG_MPA_VENDOR_ID) {
			switch (pid) {
			case MPA_PRODUCT_ID:
				acc_type = PDIC_DOCK_MPA;
				msg_maxim("Samsung MPA connected.");
				break;
			default:
				msg_maxim("default device connected.");
				acc_type = PDIC_DOCK_NEW;
				break;
			}
		} else {
			msg_maxim("unknown device connected.");
			acc_type = PDIC_DOCK_NEW;
		}
		usbpd_data->acc_type = acc_type;
	} else
		acc_type = usbpd_data->acc_type;

	if (acc_type != PDIC_DOCK_NEW)
		pdic_send_dock_intent(acc_type);

	pdic_send_dock_uevent(vid, pid, acc_type);

	mutex_lock(&usbpd_data->hmd_power_lock);
	if (max77775_check_hmd_dev(usbpd_data)) {
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
		if (o_notify)
			send_otg_notify(o_notify, NOTIFY_EVENT_HMD_EXT_CURRENT, 1);
#endif
	}
	mutex_unlock(&usbpd_data->hmd_power_lock);
	return 1;
}

void max77775_vdm_process_printf(char *type, char *vdm_data, int len)
{
#ifdef DEBUG_VDM_PRINT
	int i = 0;

	for (i = 2; i < len; i++)
		msg_maxim("[%s] , %d, [0x%x]", type, i, vdm_data[i]);
#endif
}

struct vdm_info {
	void *buf;
	int len;
	int vdo0_num;
	uint8_t w_data;
};

void max77775_request_vdm(struct max77775_usbc_platform_data *usbpd_data,
										struct vdm_info *vdm, bool in_lock)
{
	/* send the opcode */
	usbc_cmd_data write_data;
	int len = vdm->len;
	int vdm_header_num = sizeof(UND_DATA_MSG_VDM_HEADER_Type);

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		return;
	}

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_SET_VDM_REQ;
	memcpy(write_data.write_data, vdm->buf, len);
	write_data.write_length = len;
	if (vdm->w_data)
		write_data.write_data[6] = vdm->w_data;
	write_data.read_length = OPCODE_SIZE + OPCODE_HEADER_SIZE
								+ vdm_header_num + (vdm->vdo0_num * 4);
	if (in_lock)
		max77775_usbc_opcode_push(usbpd_data, &write_data);
	else
		max77775_usbc_opcode_write(usbpd_data, &write_data);
}

void max77775_set_discover_identity(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	struct vdm_info vdm;

	vdm.buf = (void *)&DP_DISCOVER_IDENTITY;
	vdm.len = sizeof(DP_DISCOVER_IDENTITY);
	vdm.vdo0_num = DP_DISCOVER_IDENTITY.byte_data.BITS.Num_Of_VDO;
	vdm.w_data = 0;

	max77775_request_vdm(usbpd_data, &vdm, false);
}
EXPORT_SYMBOL_KUNIT(max77775_set_discover_identity);

void max77775_set_dp_enter_mode(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	struct vdm_info vdm;

	vdm.buf = (void *)&DP_DISCOVER_ENTER_MODE;
	vdm.len = sizeof(DP_DISCOVER_ENTER_MODE);
	vdm.vdo0_num = DP_DISCOVER_ENTER_MODE.byte_data.BITS.Num_Of_VDO;
	vdm.w_data = 0;

	max77775_request_vdm(usbpd_data, &vdm, true);
}
EXPORT_SYMBOL_KUNIT(max77775_set_dp_enter_mode);

void max77775_set_dp_configure(void *data, uint8_t w_data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	struct vdm_info vdm;

	vdm.buf = (void *)&DP_CONFIGURE;
	vdm.len = sizeof(DP_CONFIGURE);
	vdm.vdo0_num = DP_CONFIGURE.byte_data.BITS.Num_Of_VDO;
	vdm.w_data = w_data;

	max77775_request_vdm(usbpd_data, &vdm, true);
}
EXPORT_SYMBOL_KUNIT(max77775_set_dp_configure);

void max77775_set_dex_disover_mode(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	struct vdm_info vdm;

	vdm.buf = (void *)&SS_DEX_DISCOVER_MODE_MODE;
	vdm.len = sizeof(SS_DEX_DISCOVER_MODE_MODE);
	vdm.vdo0_num = SS_DEX_DISCOVER_MODE_MODE.byte_data.BITS.Num_Of_VDO;
	vdm.w_data = 0;

	max77775_request_vdm(usbpd_data, &vdm, true);
}
EXPORT_SYMBOL_KUNIT(max77775_set_dex_disover_mode);

void max77775_set_dex_enter_mode(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	struct vdm_info vdm;

	vdm.buf = (void *)&SS_DEX_DISCOVER_ENTER_MODE;
	vdm.len = sizeof(SS_DEX_DISCOVER_ENTER_MODE);
	vdm.vdo0_num = SS_DEX_DISCOVER_ENTER_MODE.byte_data.BITS.Num_Of_VDO;
	vdm.w_data = 0;

	max77775_request_vdm(usbpd_data, &vdm, true);
}
EXPORT_SYMBOL_KUNIT(max77775_set_dex_enter_mode);

__visible_for_testing void max77775_vdm_process_discover_identity(void *data, char *vdm_data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	UND_DATA_MSG_ID_HEADER_Type *DATA_MSG_ID = NULL;
	UND_PRODUCT_VDO_Type *DATA_MSG_PRODUCT = NULL;

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		return;
	}

	/* Message Type Definition */
	DATA_MSG_ID = (UND_DATA_MSG_ID_HEADER_Type *)&vdm_data[8];
	DATA_MSG_PRODUCT = (UND_PRODUCT_VDO_Type *)&vdm_data[16];
	usbpd_data->is_sent_pin_configuration = 0;
	usbpd_data->is_samsung_accessory_enter_mode = 0;
	usbpd_data->Vendor_ID = DATA_MSG_ID->BITS.USB_Vendor_ID;
	usbpd_data->Product_ID = DATA_MSG_PRODUCT->BITS.Product_ID;
	usbpd_data->Device_Version = DATA_MSG_PRODUCT->BITS.Device_Version;
	msg_maxim("Vendor_ID : 0x%X, Product_ID : 0x%X Device Version 0x%X",
		usbpd_data->Vendor_ID, usbpd_data->Product_ID, usbpd_data->Device_Version);
	max77775_ccic_event_work(usbpd_data,
			PDIC_NOTIFY_DEV_ALL, PDIC_NOTIFY_ID_DEVICE_INFO,
			usbpd_data->Vendor_ID, usbpd_data->Product_ID, usbpd_data->Device_Version);
	if (max77775_process_check_accessory(usbpd_data))
		msg_maxim("Samsung Accessory Connected.");
}
EXPORT_SYMBOL_KUNIT(max77775_vdm_process_discover_identity);

__visible_for_testing void max77775_vdm_process_discover_svids(void *data, char *vdm_data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	int i = 0;
	uint16_t svid = 0;
	DIS_MODE_DP_CAPA_Type *pDP_DIS_MODE = (DIS_MODE_DP_CAPA_Type *)&vdm_data[0];
	/* Number_of_obj has msg_header & vdm_header, each vdo has 2 svids
	 * This logic can work until Max VDOs 12 */
	int num_of_vdos = (pDP_DIS_MODE->MSG_HEADER.BITS.Number_of_obj - 2) * 2;
	UND_VDO1_Type  *DATA_MSG_VDO1 = (UND_VDO1_Type  *)&vdm_data[8];
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
	int timeleft = 0;
	struct otg_notify *o_notify = get_otg_notify();
#endif

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		return;
	}

	usbpd_data->SVID_DP = 0;
	usbpd_data->SVID_0 = DATA_MSG_VDO1->BITS.SVID_0;
	usbpd_data->SVID_1 = DATA_MSG_VDO1->BITS.SVID_1;

	for (i = 0; i < num_of_vdos; i++) {
		memcpy(&svid, &vdm_data[8 + i * 2], 2);
		if (svid == TypeC_DP_SUPPORT) {
			msg_maxim("svid_%d : 0x%X", i, svid);
			usbpd_data->SVID_DP = svid;
			break;
		}
	}

	if (usbpd_data->SVID_DP == TypeC_DP_SUPPORT) {
		usbpd_data->dp_is_connect = 1;
		/* If you want to support USB SuperSpeed when you connect
		 * Display port dongle, You should change dp_hs_connect depend
		 * on Pin assignment.If DP use 4lane(Pin Assignment C,E,A),
		 * dp_hs_connect is 1. USB can support HS.If DP use 2lane(Pin Assigment B,D,F), dp_hs_connect is 0. USB
		 * can support SS
		 */
		 usbpd_data->dp_hs_connect = 1;

		max77775_ccic_event_work(usbpd_data,
				PDIC_NOTIFY_DEV_DP, PDIC_NOTIFY_ID_DP_CONNECT,
				PDIC_NOTIFY_ATTACH, usbpd_data->Vendor_ID, usbpd_data->Product_ID);
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
		if (o_notify) {
#if defined(CONFIG_USB_HW_PARAM)
			inc_hw_param(o_notify, USB_CCIC_DP_USE_COUNT);
#endif
			msg_maxim("%s wait_event %d", __func__,
					(usbpd_data->host_turn_on_wait_time)*HZ);

			timeleft = wait_event_interruptible_timeout(usbpd_data->host_turn_on_wait_q,
					usbpd_data->host_turn_on_event && !usbpd_data->detach_done_wait
#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
					&& !usbpd_data->wait_entermode
#endif
					, (usbpd_data->host_turn_on_wait_time)*HZ);
			msg_maxim("%s host turn on wait = %d", __func__, timeleft);
		}
#endif
		max77775_ccic_event_work(usbpd_data,
			PDIC_NOTIFY_DEV_USB_DP, PDIC_NOTIFY_ID_USB_DP,
			usbpd_data->dp_is_connect /*attach*/, usbpd_data->dp_hs_connect, 0);
	}
	msg_maxim("SVID_0 : 0x%X, SVID_1 : 0x%X",
			usbpd_data->SVID_0, usbpd_data->SVID_1);
}
EXPORT_SYMBOL_KUNIT(max77775_vdm_process_discover_svids);

static u8 max77775_vdm_choose_pin_assignment(DIS_MODE_DP_CAPA_Type *pDP_DIS_MODE)
{
	u8 pin_assignment = DP_PIN_ASSIGNMENT_NODE;
	u8 port_capability = pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.Port_Capability;
	u8 receptacle_indication = pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.Receptacle_Indication;

	/* this setting is based on vesa spec (DP_Alt_Mode_on_USB_Type-C)
	* A USB Type-C Receptacle that supports DFP_D functionality (e.g., the receptacle can behave
	* as a DisplayPort Source device or as a DFP_D on a DisplayPort Branch device) shall support
	* one or more DFP_D pin assignments. Likewise, a USB Type-C Receptacle that supports UFP_D
	* (e.g., the receptacle can behave as a DisplayPort Sink device or as the UFP_D on a DisplayPort
	* Branch device) shall support one or more UFP_D pin assignments.
	*
	* A USB Type-C Plug that is intended to plug directly into a receptacle-based DFP_D (e.g., the
	* plug can behave as a DisplayPort Sink device or as the UFP_D on a DisplayPort Branch device)
	* shall support one or more DFP_D pin assignments. Likewise, a USB Type-C Plug that is intended
	* to plug directly into a receptacle-based UFP_D (e.g., the plug can behave as a DisplayPort Source
	* device or as the DFP_D on a DisplayPort Branch device) shall support one or more UFP_D
	* pin assignments.
	*/

	if (port_capability == num_UFP_D_Capable || port_capability == num_DFP_D_and_UFP_D_Capable) {
		if (receptacle_indication == num_USB_TYPE_C_Receptacle) {
			pin_assignment = pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.UFP_D_Pin_Assignments;
			msg_maxim("1. support UFP_D 0x%08x", pin_assignment);
		} else {
			pin_assignment = pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.DFP_D_Pin_Assignments;
			msg_maxim("2. support DFP_D 0x%08x", pin_assignment);
		}
	} else {
		pin_assignment = DP_PIN_ASSIGNMENT_NODE;
		msg_maxim("do not support Port_Capability %x", port_capability);
	}

	return pin_assignment;
}

__visible_for_testing void max77775_vdm_process_discover_mode(void *data, char *vdm_data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	DIS_MODE_DP_CAPA_Type *pDP_DIS_MODE = (DIS_MODE_DP_CAPA_Type *)&vdm_data[0];
	UND_DATA_MSG_VDM_HEADER_Type *DATA_MSG_VDM = (UND_DATA_MSG_VDM_HEADER_Type *)&vdm_data[4];

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		return;
	}

	msg_maxim("vendor_id = 0x%04x , svid_1 = 0x%04x", DATA_MSG_VDM->BITS.Standard_Vendor_ID, usbpd_data->SVID_1);
	if (DATA_MSG_VDM->BITS.Standard_Vendor_ID == TypeC_DP_SUPPORT && usbpd_data->SVID_DP == TypeC_DP_SUPPORT) {
		/*  pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS. */
		msg_maxim("pDP_DIS_MODE->MSG_HEADER.DATA = 0x%08X", pDP_DIS_MODE->MSG_HEADER.DATA);
		msg_maxim("pDP_DIS_MODE->DATA_MSG_VDM_HEADER.DATA = 0x%08X", pDP_DIS_MODE->DATA_MSG_VDM_HEADER.DATA);
		msg_maxim("pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.DATA = 0x%08X", pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.DATA);

		if (pDP_DIS_MODE->MSG_HEADER.BITS.Number_of_obj > 1)
			usbpd_data->pin_assignment = max77775_vdm_choose_pin_assignment(pDP_DIS_MODE);
	}

	if (DATA_MSG_VDM->BITS.Standard_Vendor_ID == SAMSUNG_VENDOR_ID) {
		msg_maxim("dex mode discover_mode ack status!");
		/*  pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS. */
		msg_maxim("pDP_DIS_MODE->MSG_HEADER.DATA = 0x%08X", pDP_DIS_MODE->MSG_HEADER.DATA);
		msg_maxim("pDP_DIS_MODE->DATA_MSG_VDM_HEADER.DATA = 0x%08X", pDP_DIS_MODE->DATA_MSG_VDM_HEADER.DATA);
		msg_maxim("pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.DATA = 0x%08X", pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.DATA);
		/*Samsung Enter Mode */
		if (usbpd_data->send_enter_mode_req == 0) {
			msg_maxim("dex: second enter mode request");
			usbpd_data->send_enter_mode_req = 1;
			max77775_set_dex_enter_mode(usbpd_data);
		}
	} else {
		max77775_set_dp_enter_mode(usbpd_data);
	}
}
EXPORT_SYMBOL_KUNIT(max77775_vdm_process_discover_mode);

__visible_for_testing void max77775_vdm_process_enter_mode(void *data, char *vdm_data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	UND_DATA_MSG_VDM_HEADER_Type *DATA_MSG_VDM = (UND_DATA_MSG_VDM_HEADER_Type *)&vdm_data[4];

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		return;
	}

	if (DATA_MSG_VDM->BITS.VDM_command_type == 1) {
		msg_maxim("EnterMode ACK.");
		if (DATA_MSG_VDM->BITS.Standard_Vendor_ID == SAMSUNG_VENDOR_ID) {
			usbpd_data->is_samsung_accessory_enter_mode = 1;
			msg_maxim("dex mode enter_mode ack status!");
		}
	} else {
		msg_maxim("EnterMode NAK.");
	}
}
EXPORT_SYMBOL_KUNIT(max77775_vdm_process_enter_mode);

__visible_for_testing int max77775_vdm_dp_select_pin(void *data, int multi_function_preference)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	int pin_sel = 0;

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		goto err;
	}

	if (multi_function_preference) {
		if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_D)
			pin_sel = PDIC_NOTIFY_DP_PIN_D;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_B)
			pin_sel = PDIC_NOTIFY_DP_PIN_B;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_F)
			pin_sel = PDIC_NOTIFY_DP_PIN_F;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_C)
			pin_sel = PDIC_NOTIFY_DP_PIN_C;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_E)
			pin_sel = PDIC_NOTIFY_DP_PIN_E;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_A)
			pin_sel = PDIC_NOTIFY_DP_PIN_A;
		else
			msg_maxim("wrong pin assignment value");
	} else {
		if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_C)
			pin_sel = PDIC_NOTIFY_DP_PIN_C;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_E)
			pin_sel = PDIC_NOTIFY_DP_PIN_E;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_A)
			pin_sel = PDIC_NOTIFY_DP_PIN_A;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_D)
			pin_sel = PDIC_NOTIFY_DP_PIN_D;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_B)
			pin_sel = PDIC_NOTIFY_DP_PIN_B;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_F)
			pin_sel = PDIC_NOTIFY_DP_PIN_F;
		else
			msg_maxim("wrong pin assignment value");
	}
#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
	if (pin_sel == PDIC_NOTIFY_DP_PIN_C ||
			pin_sel == PDIC_NOTIFY_DP_PIN_E ||
			pin_sel == PDIC_NOTIFY_DP_PIN_A)
		usb_restart_host_mode(usbpd_data->man, 4);
	else
		usb_restart_host_mode(usbpd_data->man, 2);
#endif
err:
	return pin_sel;
}
EXPORT_SYMBOL_KUNIT(max77775_vdm_dp_select_pin);

static void max77775_vdm_prepare_dp_configure(struct max77775_usbc_platform_data *usbpd_data,
				DP_STATUS_UPDATE_Type *DP_STATUS)
{
	uint8_t W_DATA = 0;
	uint8_t multi_func = 0;
	int pin_sel = 0;

	msg_maxim("DP_STATUS_UPDATE = 0x%08X", DP_STATUS->DATA_DP_STATUS_UPDATE.DATA);

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		goto skip;
	}

	if (DP_STATUS->DATA_DP_STATUS_UPDATE.BITS.Port_Connected == 0x00) {
		msg_maxim("port disconnected!");
		goto skip;
	}

	if (usbpd_data->is_sent_pin_configuration == 0) {
		multi_func = DP_STATUS->DATA_DP_STATUS_UPDATE.BITS.Multi_Function_Preference;
		pin_sel = max77775_vdm_dp_select_pin(usbpd_data, multi_func);
		usbpd_data->dp_selected_pin = pin_sel;
		W_DATA = dp_pin_assignment_data[pin_sel];

		msg_maxim("multi_func_preference %d,  %s, W_DATA : %d",
			multi_func, dp_pin_assignment_print[pin_sel], W_DATA);

		max77775_set_dp_configure(usbpd_data, W_DATA);

		usbpd_data->is_sent_pin_configuration = 1;
	} else {
		msg_maxim("pin configuration is already sent as %s!",
			dp_pin_assignment_print[usbpd_data->dp_selected_pin]);
	}
skip:
	return;
}

__visible_for_testing void max77775_vdm_announce_hpd(struct max77775_usbc_platform_data *usbpd_data,
				DP_STATUS_UPDATE_Type *DP_STATUS)
{
	int hpd = 0;
	int hpdirq = 0;

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		return;
	}

	if (DP_STATUS->DATA_DP_STATUS_UPDATE.BITS.HPD_State == 1)
		hpd = PDIC_NOTIFY_HIGH;
	else if (DP_STATUS->DATA_DP_STATUS_UPDATE.BITS.HPD_State == 0)
		hpd = PDIC_NOTIFY_LOW;

	if (DP_STATUS->DATA_DP_STATUS_UPDATE.BITS.HPD_Interrupt == 1)
		hpdirq = PDIC_NOTIFY_IRQ;

	max77775_ccic_event_work(usbpd_data,
			PDIC_NOTIFY_DEV_DP, PDIC_NOTIFY_ID_DP_HPD,
			hpd, hpdirq, 0);
}
EXPORT_SYMBOL_KUNIT(max77775_vdm_announce_hpd);

static void max77775_vdm_process_dp_status_update(void *data, char *vdm_data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	VDO_MESSAGE_Type *VDO_MSG;
	DP_STATUS_UPDATE_Type *DP_STATUS;
	int i;

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		return;
	}

	if (usbpd_data->SVID_DP == TypeC_DP_SUPPORT) {
		DP_STATUS = (DP_STATUS_UPDATE_Type *)&vdm_data[0];

		max77775_vdm_prepare_dp_configure(usbpd_data, DP_STATUS);

		max77775_vdm_announce_hpd(usbpd_data, DP_STATUS);
	} else {
		/* need to check F/W code */
		VDO_MSG = (VDO_MESSAGE_Type *)&vdm_data[8];
		for (i = 0; i < 6; i++)
			msg_maxim("VDO_%d : %d", i+1, VDO_MSG->VDO[i]);
	}
}

static void max77775_vdm_process_dp_attention(void *data, char *vdm_data)
{
	max77775_vdm_process_dp_status_update(data, vdm_data);
}

__visible_for_testing void max77775_vdm_process_dp_configure(void *data, char *vdm_data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	UND_DATA_MSG_VDM_HEADER_Type *DATA_MSG_VDM = (UND_DATA_MSG_VDM_HEADER_Type *)&vdm_data[4];

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		return;
	}

	msg_maxim("vendor_id = 0x%04x , svid_1 = 0x%04x", DATA_MSG_VDM->BITS.Standard_Vendor_ID, usbpd_data->SVID_1);
	if (usbpd_data->SVID_DP == TypeC_DP_SUPPORT)
		schedule_work(&usbpd_data->dp_configure_work);
	if (DATA_MSG_VDM->BITS.Standard_Vendor_ID == TypeC_DP_SUPPORT && usbpd_data->SVID_1 == TypeC_Dex_SUPPORT) {
		/* Samsung Discover Modes packet */
		usbpd_data->send_enter_mode_req = 0;
		max77775_set_dex_disover_mode(usbpd_data);
	}
}
EXPORT_SYMBOL_KUNIT(max77775_vdm_process_dp_configure);

void max77775_vdm_message_handler(struct max77775_usbc_platform_data *usbpd_data,
	char *opcode_data, int len)
{
	unsigned char vdm_data[OPCODE_DATA_LENGTH] = {0,};
	UND_DATA_MSG_VDM_HEADER_Type vdm_header;

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		return;
	}

	memset(&vdm_header, 0, sizeof(UND_DATA_MSG_VDM_HEADER_Type));
	memcpy(vdm_data, opcode_data, len);
	memcpy(&vdm_header, &vdm_data[4], sizeof(vdm_header));
	if ((vdm_header.BITS.VDM_command_type) == SEC_UVDM_RESPONDER_NAK) {
		msg_maxim("IGNORE THE NAK RESPONSE !!![%d]", vdm_data[1]);
		return;
	}

	switch (vdm_data[1]) {
	case OPCODE_ID_VDM_DISCOVER_IDENTITY:
		max77775_vdm_process_printf("VDM_DISCOVER_IDENTITY", vdm_data, len);
		max77775_vdm_process_discover_identity(usbpd_data, vdm_data);
		break;
	case OPCODE_ID_VDM_DISCOVER_SVIDS:
		max77775_vdm_process_printf("VDM_DISCOVER_SVIDS", vdm_data, len);
		max77775_vdm_process_discover_svids(usbpd_data, vdm_data);
		break;
	case OPCODE_ID_VDM_DISCOVER_MODES:
		max77775_vdm_process_printf("VDM_DISCOVER_MODES", vdm_data, len);
		vdm_data[0] = vdm_data[2];
		vdm_data[1] = vdm_data[3];
		max77775_vdm_process_discover_mode(usbpd_data, vdm_data);
		break;
	case OPCODE_ID_VDM_ENTER_MODE:
		max77775_vdm_process_printf("VDM_ENTER_MODE", vdm_data, len);
		max77775_vdm_process_enter_mode(usbpd_data, vdm_data);
		break;
	case OPCODE_ID_VDM_SVID_DP_STATUS:
		max77775_vdm_process_printf("VDM_SVID_DP_STATUS", vdm_data, len);
		vdm_data[0] = vdm_data[2];
		vdm_data[1] = vdm_data[3];
		max77775_vdm_process_dp_status_update(usbpd_data, vdm_data);
		break;
	case OPCODE_ID_VDM_SVID_DP_CONFIGURE:
		max77775_vdm_process_printf("VDM_SVID_DP_CONFIGURE", vdm_data, len);
		max77775_vdm_process_dp_configure(usbpd_data, vdm_data);
		break;
	case OPCODE_ID_VDM_ATTENTION:
		max77775_vdm_process_printf("VDM_ATTENTION", vdm_data, len);
		vdm_data[0] = vdm_data[2];
		vdm_data[1] = vdm_data[3];
		max77775_vdm_process_dp_attention(usbpd_data, vdm_data);
		break;
	case OPCODE_ID_VDM_EXIT_MODE:
		break;
	default:
		break;
	}
}

struct vdm_response {
	u8 data;
	int write_length;
	int read_length;
};

static int max77775_get_vdm_response(struct max77775_usbc_platform_data *usbpd_data,
	struct vdm_response *vdm)
{
	/* send the opcode */
	usbc_cmd_data write_data;
	int ret = 0;

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		ret = -ENOENT;
		goto skip;
	}

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_GET_VDM_RESP;
	write_data.write_data[0] = vdm->data;
	write_data.write_length = vdm->write_length;
	write_data.read_length = vdm->read_length;
	ret = max77775_usbc_opcode_write(usbpd_data, &write_data);
	if (ret)
		msg_maxim("%s error. ret=%d", __func__, ret);
skip:
	return ret;
}

__visible_for_testing int max77775_get_discover_identity(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	struct vdm_response vdm;

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		return -ENOENT;
	}

	vdm.data = OPCODE_ID_VDM_DISCOVER_IDENTITY;
	vdm.write_length = 1;
	vdm.read_length = DISCOVER_IDENTITY_RESPONSE_SIZE;
	return max77775_get_vdm_response(usbpd_data, &vdm);
}
EXPORT_SYMBOL_KUNIT(max77775_get_discover_identity);

__visible_for_testing int max77775_get_discover_svids(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	struct vdm_response vdm;

	vdm.data = OPCODE_ID_VDM_DISCOVER_SVIDS;
	vdm.write_length = 1;
	vdm.read_length = DISCOVER_SVIDS_RESPONSE_SIZE;
	return max77775_get_vdm_response(usbpd_data, &vdm);
}
EXPORT_SYMBOL_KUNIT(max77775_get_discover_svids);

__visible_for_testing int max77775_get_discover_modes(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	struct vdm_response vdm;

	vdm.data = OPCODE_ID_VDM_DISCOVER_MODES;
	vdm.write_length = 1;
	vdm.read_length = DISCOVER_MODES_RESPONSE_SIZE;
	return max77775_get_vdm_response(usbpd_data, &vdm);
}
EXPORT_SYMBOL_KUNIT(max77775_get_discover_modes);

__visible_for_testing int max77775_get_enter_mode(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	struct vdm_response vdm;

	vdm.data = OPCODE_ID_VDM_ENTER_MODE;
	vdm.write_length = 1;
	vdm.read_length = ENTER_MODE_RESPONSE_SIZE;
	return max77775_get_vdm_response(usbpd_data, &vdm);
}
EXPORT_SYMBOL_KUNIT(max77775_get_enter_mode);

__visible_for_testing int max77775_get_attention(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	struct vdm_response vdm;

	vdm.data = OPCODE_ID_VDM_ATTENTION;
	vdm.write_length = 1;
	vdm.read_length = ATTENTION_RESPONSE_SIZE;
	return max77775_get_vdm_response(usbpd_data, &vdm);
}
EXPORT_SYMBOL_KUNIT(max77775_get_attention);

__visible_for_testing int max77775_get_dp_status_update(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	struct vdm_response vdm;

	vdm.data = OPCODE_ID_VDM_SVID_DP_STATUS;
	vdm.write_length = 1;
	vdm.read_length = DP_STATUS_RESPONSE_SIZE;
	return max77775_get_vdm_response(usbpd_data, &vdm);
}
EXPORT_SYMBOL_KUNIT(max77775_get_dp_status_update);

__visible_for_testing int max77775_get_dp_configure(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	struct vdm_response vdm;

	vdm.data = OPCODE_ID_VDM_SVID_DP_CONFIGURE;
	vdm.write_length = 1;
	vdm.read_length = DP_CONFIGURE_RESPONSE_SIZE;
	return max77775_get_vdm_response(usbpd_data, &vdm);
}
EXPORT_SYMBOL_KUNIT(max77775_get_dp_configure);

static void max77775_process_alternate_mode(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	uint32_t mode;
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
	struct otg_notify *o_notify = get_otg_notify();
#endif

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		return;
	}

	mode = usbpd_data->alternate_state;

	if (mode) {
		msg_maxim("mode : 0x%x", mode);
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
		if (is_blocked(o_notify, NOTIFY_BLOCK_TYPE_HOST)) {
			msg_maxim("host is blocked, skip all the alternate mode.");
			goto process_error;
		}

		if (usbpd_data->mdm_block) {
			msg_maxim("is blocked by mdm, skip all the alternate mode.");
			goto process_error;
		}
#endif

		if (mode & VDM_DISCOVER_ID)
			if (max77775_get_discover_identity(usbpd_data))
				goto process_error;
		if (mode & VDM_DISCOVER_SVIDS)
			if (max77775_get_discover_svids(usbpd_data))
				goto process_error;
		if (mode & VDM_DISCOVER_MODES)
			if (max77775_get_discover_modes(usbpd_data))
				goto process_error;
		if (mode & VDM_ENTER_MODE)
			if (max77775_get_enter_mode(usbpd_data))
				goto process_error;
		if (mode & VDM_DP_STATUS_UPDATE)
			if (max77775_get_dp_status_update(usbpd_data))
				goto process_error;
		if (mode & VDM_DP_CONFIGURE)
			if (max77775_get_dp_configure(usbpd_data))
				goto process_error;
		if (mode & VDM_ATTENTION)
			if (max77775_get_attention(usbpd_data))
				goto process_error;
process_error:
		usbpd_data->alternate_state = 0;
	}
}

void max77775_receive_alternate_message(struct max77775_usbc_platform_data *data, MAX77775_VDM_MSG_IRQ_STATUS_Type *VDM_MSG_IRQ_State)
{
	struct max77775_usbc_platform_data *usbpd_data = data;

	if (!usbpd_data) {
		msg_maxim("%s usbpd_data is null.", __func__);
		return;
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_ID) {
		msg_maxim(": %s", &vdm_msg_irq_state_print[1][0]);
		usbpd_data->alternate_state |= VDM_DISCOVER_ID;
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_SVIDs) {
		msg_maxim(": %s", &vdm_msg_irq_state_print[2][0]);
		usbpd_data->alternate_state |= VDM_DISCOVER_SVIDS;
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_MODEs) {
		msg_maxim(": %s", &vdm_msg_irq_state_print[3][0]);
		usbpd_data->alternate_state |= VDM_DISCOVER_MODES;
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Enter_Mode) {
		msg_maxim(": %s", &vdm_msg_irq_state_print[4][0]);
		usbpd_data->alternate_state |= VDM_ENTER_MODE;
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Exit_Mode) {
		msg_maxim(": %s", &vdm_msg_irq_state_print[5][0]);
		usbpd_data->alternate_state |= VDM_EXIT_MODE;
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Attention) {
		msg_maxim(": %s", &vdm_msg_irq_state_print[6][0]);
		usbpd_data->alternate_state |= VDM_ATTENTION;
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_DP_Status_Update) {
		msg_maxim(": %s", &vdm_msg_irq_state_print[7][0]);
		usbpd_data->alternate_state |= VDM_DP_STATUS_UPDATE;
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_DP_Configure) {
		msg_maxim(": %s", &vdm_msg_irq_state_print[8][0]);
		usbpd_data->alternate_state |= VDM_DP_CONFIGURE;
	}

	max77775_process_alternate_mode(usbpd_data);
}

static void set_sec_uvdm_txdataheader(void *data, int first_set, int cur_uvdmset,
		int total_data_size, int remained_data_size)
{
	U_SEC_TX_DATA_HEADER *SEC_TX_DATA_HEADER;
	uint8_t *SendMSG = (uint8_t *)data;

	if (first_set)
		SEC_TX_DATA_HEADER = (U_SEC_TX_DATA_HEADER *)&SendMSG[12];
	else
		SEC_TX_DATA_HEADER = (U_SEC_TX_DATA_HEADER *)&SendMSG[8];

	SEC_TX_DATA_HEADER->BITS.data_size_of_current_set =
		get_data_size(first_set, remained_data_size);
	SEC_TX_DATA_HEADER->BITS.total_data_size = total_data_size;
	SEC_TX_DATA_HEADER->BITS.order_of_current_uvdm_set = cur_uvdmset;
	SEC_TX_DATA_HEADER->BITS.reserved = 0;
}

static void set_msgheader(void *data, int msg_type, int obj_num)
{
	/* Common : Fill the VDM OpCode MSGHeader */
	SEND_VDM_BYTE_DATA *MSG_HDR;
	uint8_t *SendMSG = (uint8_t *)data;

	MSG_HDR = (SEND_VDM_BYTE_DATA *)&SendMSG[0];
	MSG_HDR->BITS.Num_Of_VDO = obj_num;
	if (msg_type == NAK)
		MSG_HDR->BITS.Reserved = 7;
	MSG_HDR->BITS.Cmd_Type = msg_type;
}

static void set_uvdmheader(void *data, int vendor_id, int vdm_type)
{
	/* Common : Fill the UVDMHeader */
	UND_UNSTRUCTURED_VDM_HEADER_Type *UVDM_HEADER;
	U_DATA_MSG_VDM_HEADER_Type *VDM_HEADER;
	uint8_t *SendMSG = (uint8_t *)data;

	UVDM_HEADER = (UND_UNSTRUCTURED_VDM_HEADER_Type *)&SendMSG[4];
	UVDM_HEADER->BITS.USB_Vendor_ID = vendor_id;
	UVDM_HEADER->BITS.VDM_TYPE = vdm_type;
	VDM_HEADER = (U_DATA_MSG_VDM_HEADER_Type *)&SendMSG[4];
	VDM_HEADER->BITS.VDM_command = 4; //from s2mm005 concept
}

static void set_sec_uvdmheader(void *data, int pid, bool data_type, int cmd_type,
		bool dir, int total_uvdmset_num, uint8_t received_data)
{
	U_SEC_UVDM_HEADER *SEC_VDM_HEADER;
	uint8_t *SendMSG = (uint8_t *)data;

	SEC_VDM_HEADER = (U_SEC_UVDM_HEADER *)&SendMSG[8];
	SEC_VDM_HEADER->BITS.pid = pid;
	SEC_VDM_HEADER->BITS.data_type = data_type;
	SEC_VDM_HEADER->BITS.command_type = cmd_type;
	SEC_VDM_HEADER->BITS.direction = dir;
	if (dir == DIR_OUT)
		SEC_VDM_HEADER->BITS.total_number_of_uvdm_set = total_uvdmset_num;
	if (data_type == TYPE_SHORT)
		SEC_VDM_HEADER->BITS.data = received_data;
	else
		SEC_VDM_HEADER->BITS.data = 0;
#if (UVDM_DEBUG)
	msg_maxim("pid = 0x%x  data_type=%d ,cmd_type =%d,direction= %d, total_num_of_uvdm_set = %d",
		SEC_VDM_HEADER->BITS.pid,
		SEC_VDM_HEADER->BITS.data_type,
		SEC_VDM_HEADER->BITS.command_type,
		SEC_VDM_HEADER->BITS.direction,
		SEC_VDM_HEADER->BITS.total_number_of_uvdm_set);
#endif
}

static void set_sec_uvdm_txdata_tailer(void *data)
{
	U_SEC_TX_DATA_TAILER *SEC_TX_DATA_TAILER;
	uint8_t *SendMSG = (uint8_t *)data;

	SEC_TX_DATA_TAILER = (U_SEC_TX_DATA_TAILER *)&SendMSG[28];
	SEC_TX_DATA_TAILER->BITS.checksum = get_checksum(SendMSG, 8, SAMSUNGUVDM_CHECKSUM_DATA_COUNT);
	SEC_TX_DATA_TAILER->BITS.reserved = 0;
}

static void set_sec_uvdm_rxdata_header(void *data, int cur_uvdmset_num, int cur_uvdmset_data, int ack)
{
	U_SEC_RX_DATA_HEADER *SEC_UVDM_RX_HEADER;
	uint8_t *SendMSG = (uint8_t *)data;

	SEC_UVDM_RX_HEADER = (U_SEC_RX_DATA_HEADER *)&SendMSG[8];
	SEC_UVDM_RX_HEADER->BITS.order_of_current_uvdm_set = cur_uvdmset_num;
	SEC_UVDM_RX_HEADER->BITS.received_data_size_of_current_set = cur_uvdmset_data;
	SEC_UVDM_RX_HEADER->BITS.result_value = ack;
	SEC_UVDM_RX_HEADER->BITS.reserved = 0;
}

static int max77775_request_uvdm(struct max77775_usbc_platform_data *usbpd_data,
	void *data, int specific_read_len)
{
	struct SS_UNSTRUCTURED_VDM_MSG vdm_opcode_msg;
	uint8_t *SendMSG = (uint8_t *)data;
	usbc_cmd_data write_data;
	int len = sizeof(struct SS_UNSTRUCTURED_VDM_MSG);

	if (!usbpd_data)
		return -ENOENT;

	memset(&vdm_opcode_msg, 0, len);
	/* Common : MSGHeader */
	memcpy(&vdm_opcode_msg.byte_data, &SendMSG[0], sizeof(SEND_VDM_BYTE_DATA));
	/* Copy the data from SendMSG buffer */
	memcpy(&vdm_opcode_msg.VDO_MSG.VDO[0], &SendMSG[SAMSUNGUVDM_UVDM_HEADER_OFFSET],
		sizeof(VDO_MESSAGE_Type));
	/* Write SendMSG buffer via I2C */
	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_SET_VDM_REQ;
	write_data.is_uvdm = 1;
	memcpy(write_data.write_data, &vdm_opcode_msg, sizeof(vdm_opcode_msg));
	write_data.write_length = len;
	if (specific_read_len)
		write_data.read_length = specific_read_len;
	else
		write_data.read_length = len;
	max77775_usbc_opcode_write(usbpd_data, &write_data);
	msg_maxim("opcode is sent");
	return 0;
}

void max77775_send_dex_fan_unstructured_vdm_message(void *data, int cmd)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	uint8_t SendMSG[32] = {0,};
	uint32_t message = (uint32_t)cmd;
	VDO_MESSAGE_Type *VDO_MSG = (VDO_MESSAGE_Type *)&SendMSG[4];

	if (!usbpd_data)
		return;

	set_msgheader(SendMSG, ACK, 2);

	VDO_MSG->VDO[0] = 0x04E80000;
	VDO_MSG->VDO[1] = message;

	md75_info_usb("%s: cmd : 0x%x\n", __func__, cmd);

	max77775_request_uvdm(usbpd_data, SendMSG, SAMSUNGUVDM_DEXFAN_RESPONSE_SIZE);
}
EXPORT_SYMBOL_KUNIT(max77775_send_dex_fan_unstructured_vdm_message);

void max77775_set_alternate_mode_opcode(void *data, int mode)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	usbc_cmd_data write_data;

	if (!usbpd_data)
		return;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_SET_ALTERNATEMODE;
	write_data.write_data[0] = mode;
	write_data.write_length = 0x1;
	write_data.read_length = 0x1;
	max77775_usbc_opcode_write(usbpd_data, &write_data);
}

static void max77775_print_usbc_status_register
	(struct max77775_usbc_platform_data *usbpd_data)
{
	u8 status[7] = {0,};

	if (!usbpd_data)
		return;

	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_USBC_STATUS1, &status[0]);
	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_USBC_STATUS2, &status[1]);
	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_BC_STATUS, &status[2]);
	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_CC_STATUS1, &status[3]);
	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_CC_STATUS2, &status[4]);
	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_PD_STATUS1, &status[5]);
	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_PD_STATUS2, &status[6]);

	msg_maxim("USBC1:0x%02x, USBC2:0x%02x, BC:0x%02x",
		status[0], status[1], status[2]);
	msg_maxim("CC_STATUS0:0x%x, CC_STATUS1:0x%x, PD_STATUS0:0x%x, PD_STATUS1:0x%x",
		status[3], status[4], status[5], status[6]);
}

static void max77775_print_usbc_int_mask_register
	(struct max77775_usbc_platform_data *usbpd_data)
{
	u8 status[4] = {0,};

	if (!usbpd_data)
		return;

	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_UIC_INT_M, &status[0]);
	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_CC_INT_M, &status[1]);
	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_PD_INT_M, &status[2]);
	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_VDM_INT_M, &status[3]);

	msg_maxim("UIC_INT_M:0x%x, CC_INT_M:0x%x, PD_INT_M:0x%x, VDM_INT_M:0x%x",
		status[0], status[1], status[2], status[3]);
}

static void max77775_set_usbc_int_mask_register
	(struct max77775_usbc_platform_data *usbpd_data)
{
	u8 status[2] = {0,};

	if (!usbpd_data)
		return;

	max77775_write_reg(usbpd_data->muic, MAX77775_USBC_REG_VDM_INT_M, 0xF0);
	max77775_write_reg(usbpd_data->muic, MAX77775_USBC_REG_PD_INT_M, 0x0);
	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_PD_INT_M, &status[0]);
	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_VDM_INT_M, &status[1]);

	msg_maxim("PD_INT_M:0x%x, VDM_INT_M:0x%x", status[0], status[1]);
}

#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
static void max77775_print_usbc_spare_register
	(struct max77775_usbc_platform_data *usbpd_data)
{
	u8 status[2] = {0,};

	if (!usbpd_data)
		return;

	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_SPARE_STATUS1, &status[0]);
	max77775_read_reg(usbpd_data->muic, MAX77775_USBC_REG_SPARE_INT_M, &status[1]);

	msg_maxim("TA_STATUS:0x%x, SPARE_INT_M:0x%x",
		status[0], status[1]);
}
#endif

void max77775_set_enable_alternate_mode(int mode)
{
	struct max77775_usbc_platform_data *usbpd_data = NULL;
	static int check_is_driver_loaded;
	int is_first_booting = 0;
	struct max77775_pd_data *pd_data = NULL;

	usbpd_data = g_usbc_data;

	if (!usbpd_data)
		return;

	is_first_booting = usbpd_data->is_first_booting;
	pd_data = usbpd_data->pd_data;

	msg_maxim("is_first_booting  : %x mode %x",
			usbpd_data->is_first_booting, mode);
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	store_usblog_notify(NOTIFY_ALTERNATEMODE, (void *)&mode, NULL);
#endif
	usbpd_data->set_altmode = mode;

	if ((mode & ALTERNATE_MODE_NOT_READY) &&
	    (mode & ALTERNATE_MODE_READY)) {
		msg_maxim("mode is invalid!");
		return;
	}
	if ((mode & ALTERNATE_MODE_START) && (mode & ALTERNATE_MODE_STOP)) {
		msg_maxim("mode is invalid!");
		return;
	}

	if ((mode & ALTERNATE_MODE_STOP)) {
		max77775_set_alternate_mode_opcode(usbpd_data,
			MAXIM_ENABLE_ALTERNATE_SRCCAP);
		msg_maxim("alternate mode is stopped! enable srccap ");
	} else {
		if (mode & ALTERNATE_MODE_NOT_READY) {
			check_is_driver_loaded = 0;
			msg_maxim("alternate mode is not ready!");
		} else if (mode & ALTERNATE_MODE_READY) {
			check_is_driver_loaded = 1;
			msg_maxim("alternate mode is ready!");
		} else {
			;
		}

		if (check_is_driver_loaded) {
			switch (is_first_booting) {
			case 0: /*this routine is calling after complete a booting.*/
				if (mode & ALTERNATE_MODE_START) {
					max77775_set_alternate_mode_opcode(usbpd_data,
						MAXIM_ENABLE_ALTERNATE_SRC_VDM);
					msg_maxim("[NO BOOTING TIME] !!!alternate mode is started!");
				} else if (mode & ALTERNATE_MODE_STOP) {
					max77775_set_alternate_mode_opcode(usbpd_data,
						MAXIM_ENABLE_ALTERNATE_SRCCAP);
					msg_maxim("[NO BOOTING TIME] alternate mode is stopped!");
				}
				break;
			case 1:
				if (mode & ALTERNATE_MODE_START) {
					msg_maxim("[ON BOOTING TIME] !!!alternate mode is started!");
					max77775_set_alternate_mode_opcode(usbpd_data,
						MAXIM_ENABLE_ALTERNATE_SRC_VDM);
					msg_maxim("!![ON BOOTING TIME] SEND THE PACKET REGARDING IN CASE OF VR/DP ");
					/* FOR THE DEX FUNCTION. */
					max77775_print_usbc_status_register(usbpd_data);

					max77775_print_usbc_int_mask_register(usbpd_data);

					max77775_set_usbc_int_mask_register(usbpd_data);
#if defined(CONFIG_MAX77775_CCOPEN_AFTER_WATERCABLE)
					max77775_print_usbc_spare_register(usbpd_data);
#endif
					usbpd_data->is_first_booting = 0;
				} else if (mode & ALTERNATE_MODE_STOP) {
					max77775_set_alternate_mode_opcode(usbpd_data,
						MAXIM_ENABLE_ALTERNATE_SRCCAP);
					msg_maxim("[ON BOOTING TIME] alternate mode is stopped!");
				}
				break;
			default:
				msg_maxim("[Never calling] is_first_booting [ %d]", is_first_booting);
				break;
			}
		}
	}
}

static int check_is_wait_ack_accessroy(int vid, int pid, int svid)
{
	int should_wait = true;

	if (vid == SAMSUNG_VENDOR_ID && pid == DEXDOCK_PRODUCT_ID && svid == TypeC_DP_SUPPORT) {
		msg_maxim("no need to wait ack response!");
		should_wait = false;
	}
	return should_wait;
}

static int max77775_send_sec_unstructured_short_vdm_message(void *data, void *buf, size_t size)
{
	struct max77775_usbc_platform_data *usbpd_data = data;
	uint8_t SendMSG[32] = {0,};
	/* Message Type Definition */
	uint8_t received_data = 0;
	int time_left;
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	int event;
#endif

	if ((buf == NULL) || size <= 0) {
		msg_maxim("given data is not valid !");
		return -EINVAL;
	}

	if (!usbpd_data->is_samsung_accessory_enter_mode) {
		msg_maxim("samsung_accessory mode is not ready!");
		return -ENXIO;
	}
	/* 1. Calc the receivced data size and determin the uvdm set count and last data of uvdm set. */
	received_data = *(char *)buf;
	/* 2. Common : Fill the MSGHeader */
	set_msgheader(SendMSG, ACK, 2);
	/* 3. Common : Fill the UVDMHeader*/
	set_uvdmheader(SendMSG, SAMSUNG_VENDOR_ID, 0);
	/* 4. Common : Fill the First SEC_VDMHeader*/
	set_sec_uvdmheader(SendMSG, usbpd_data->Product_ID, TYPE_SHORT,	SEC_UVDM_ININIATOR, DIR_OUT, 1, received_data);
	usbpd_data->is_in_first_sec_uvdm_req = true;

	usbpd_data->uvdm_error = 0;
	max77775_request_uvdm(usbpd_data, SendMSG, 0);

	if (check_is_wait_ack_accessroy(usbpd_data->Vendor_ID, usbpd_data->Product_ID, usbpd_data->SVID_DP)) {
		reinit_completion(&usbpd_data->uvdm_longpacket_out_wait);
		/* Wait Response*/
		time_left =
			wait_for_completion_interruptible_timeout(&usbpd_data->uvdm_longpacket_out_wait,
							  msecs_to_jiffies(SAMSUNGUVDM_WAIT_MS));
		if (time_left <= 0) {
			usbpd_data->is_in_first_sec_uvdm_req = false;
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
			event = NOTIFY_EXTRA_UVDM_TIMEOUT;
			store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
			return -ETIME;
		}
		if (usbpd_data->uvdm_error) {
			usbpd_data->is_in_first_sec_uvdm_req = false;
			return usbpd_data->uvdm_error;
		}
	}
	msg_maxim("exit : short data transfer complete!");
	usbpd_data->is_in_first_sec_uvdm_req = false;
	return size;
}

static int max77775_send_sec_unstructured_long_vdm_message(void *data, void *buf, size_t size)
{
	struct max77775_usbc_platform_data *usbpd_data;
	uint8_t SendMSG[32] = {0,};
	uint8_t *SEC_DATA;
	int need_uvdmset_count = 0;
	int cur_uvdmset_data = 0;
	int cur_uvdmset_num = 0;
	int accumulated_data_size = 0;
	int remained_data_size = 0;
	uint8_t received_data[MAX_INPUT_DATA] = {0,};
	int time_left;
	int i;
	int received_data_index;
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	int event;
#endif

	usbpd_data = data;
	if (!usbpd_data)
		return -ENXIO;

	if (!buf) {
		msg_maxim("given data is not valid !");
		return -EINVAL;
	}

	/* 1. Calc the receivced data size and determin the uvdm set count and last data of uvdm set. */
	set_endian(buf, received_data, size);
	need_uvdmset_count = set_uvdmset_count(size);
	msg_maxim("need_uvdmset_count = %d", need_uvdmset_count);
	usbpd_data->is_in_first_sec_uvdm_req = true;
	usbpd_data->is_in_sec_uvdm_out = DIR_OUT;
	cur_uvdmset_num = 1;
	accumulated_data_size = 0;
	remained_data_size = size;
	received_data_index = 0;
	/* 2. Common : Fill the MSGHeader */
	set_msgheader(SendMSG, ACK, 7);
	/* 3. Common : Fill the UVDMHeader*/
	set_uvdmheader(SendMSG, SAMSUNG_VENDOR_ID, 0);
	/* 4. Common : Fill the First SEC_VDMHeader*/
	if (usbpd_data->is_in_first_sec_uvdm_req)
		set_sec_uvdmheader(SendMSG, usbpd_data->Product_ID,
						TYPE_LONG, SEC_UVDM_ININIATOR, DIR_OUT, need_uvdmset_count, 0);

	while (cur_uvdmset_num <= need_uvdmset_count) {
		cur_uvdmset_data = 0;
		time_left = 0;

		set_sec_uvdm_txdataheader(SendMSG, usbpd_data->is_in_first_sec_uvdm_req,
				cur_uvdmset_num, size, remained_data_size);

		cur_uvdmset_data = get_data_size(usbpd_data->is_in_first_sec_uvdm_req, remained_data_size);
		msg_maxim("data_size_of_current_set = %d ,total_data_size = %ld, order_of_current_uvdm_set = %d",
			cur_uvdmset_data, size, cur_uvdmset_num);
		/* 6. Common : Fill the DATA */
		if (usbpd_data->is_in_first_sec_uvdm_req) {
			SEC_DATA = (uint8_t *)&SendMSG[8+8];
			for (i = 0; i < SAMSUNGUVDM_MAXDATA_FIRST_UVDMSET; i++)
				SEC_DATA[i] = received_data[received_data_index++];
		} else {
			SEC_DATA = (uint8_t *)&SendMSG[8+4];
			for (i = 0; i < SAMSUNGUVDM_MAXDATA_NORMAL_UVDMSET; i++)
				SEC_DATA[i] = received_data[received_data_index++];
		}
		/* 7. Common : Fill the TX_DATA_Tailer */
		set_sec_uvdm_txdata_tailer(SendMSG);
		/* 8. Send data to PDIC */
		usbpd_data->uvdm_error = 0;
		max77775_request_uvdm(usbpd_data, SendMSG, 0);
		/* 9. Wait Response*/
		reinit_completion(&usbpd_data->uvdm_longpacket_out_wait);
		time_left =
			wait_for_completion_interruptible_timeout(&usbpd_data->uvdm_longpacket_out_wait,
							  msecs_to_jiffies(SAMSUNGUVDM_WAIT_MS));
		if (time_left <= 0) {
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
			event = NOTIFY_EXTRA_UVDM_TIMEOUT;
			store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
			return -ETIME;
		}

		if (usbpd_data->uvdm_error)
			return usbpd_data->uvdm_error;

		accumulated_data_size += cur_uvdmset_data;
		remained_data_size -= cur_uvdmset_data;
		if (usbpd_data->is_in_first_sec_uvdm_req)
			usbpd_data->is_in_first_sec_uvdm_req = false;
		cur_uvdmset_num++;
	}
	return size;
}

__visible_for_testing int max77775_check_sec_uvdm_responder(struct max77775_usbc_platform_data *usbpd_data,
	int command_type, char *prefix)
{
	bool ret = false;

	if (!usbpd_data)
		return ret;

	if (command_type == SEC_UVDM_RESPONDER_ACK)
		return true;
	else if (command_type == SEC_UVDM_RESPONDER_NAK) {
		msg_maxim("%s SEC_UVDM_RESPONDER_NAK received", prefix);
		usbpd_data->uvdm_error = -ENODATA;
	} else if (command_type == SEC_UVDM_RESPONDER_BUSY) {
		msg_maxim("%s SEC_UVDM_RESPONDER_BUSY received", prefix);
		usbpd_data->uvdm_error = -EBUSY;
	} else {
		msg_maxim("%s Undefined RESPONDER value", prefix);
		usbpd_data->uvdm_error = -EPROTO;
	}
	return ret;
}
EXPORT_SYMBOL_KUNIT(max77775_check_sec_uvdm_responder);

__visible_for_testing int max77775_check_sec_uvdm_rx_header(struct max77775_usbc_platform_data *usbpd_data,
	int result_value)
{
	bool ret = false;

	if (!usbpd_data)
		return ret;

	if (result_value == RX_ACK) {
		return true;
	} else if (result_value == RX_NAK) {
		msg_maxim("DIR_OUT RX_NAK received");
		usbpd_data->uvdm_error = -ENODATA;
	} else if (result_value == RX_BUSY) {
		msg_maxim("DIR_OUT RX_BUSY received");
		usbpd_data->uvdm_error = -EBUSY;
	} else {
		msg_maxim("DIR_OUT Undefined RX value");
		usbpd_data->uvdm_error = -EPROTO;
	}
	return ret;
}
EXPORT_SYMBOL_KUNIT(max77775_check_sec_uvdm_rx_header);

static void print_sec_uvdm_response_header_data(uint32_t data)
{
#if (UVDM_DEBUG)
	msg_maxim("DIR_OUT SEC_UVDM_RESPONSE_HEADER : 0x%x", data);
#endif
}

static void print_sec_uvdm_response_header(U_SEC_UVDM_RESPONSE_HEADER *header)
{
	if (!header)
		return;

#if (UVDM_DEBUG)
	msg_maxim("DIR_IN data_type = %d , command_type = %d, direction=%d, total_number_of_uvdm_set=%d",
		header->BITS.data_type,
		header->BITS.command_type,
		header->BITS.direction,
		header->BITS.total_number_of_uvdm_set);
#endif
}

static void print_sec_uvdm_rx_header_data(uint32_t data)
{
#if (UVDM_DEBUG)
	msg_maxim("DIR_OUT SEC_UVDM_RX_HEADER : 0x%x", data);
#endif
}

static void print_sec_uvdm_tx_header(U_SEC_TX_DATA_HEADER *header)
{
	if (!header)
		return;

#if (UVDM_DEBUG)
	msg_maxim("DIR_IN order_of_current_uvdm_set = %d , total_data_size = %d, data_size_of_current_set=%d",
		header->BITS.order_of_current_uvdm_set,
		header->BITS.total_data_size,
		header->BITS.data_size_of_current_set);
#endif
}

static void max77775_process_out_uvdm_response
				(struct max77775_usbc_platform_data *usbpd_data, uint8_t read_msg[])
{
	U_SEC_UVDM_RESPONSE_HEADER *SEC_UVDM_RESPONSE_HEADER;
	U_SEC_RX_DATA_HEADER *SEC_UVDM_RX_HEADER;

	if (!usbpd_data)
		return;

	if (usbpd_data->is_in_first_sec_uvdm_req) {
		SEC_UVDM_RESPONSE_HEADER = (U_SEC_UVDM_RESPONSE_HEADER *)&read_msg[VDO1_OFFSET];

		print_sec_uvdm_response_header_data(SEC_UVDM_RESPONSE_HEADER->data);

		if (SEC_UVDM_RESPONSE_HEADER->BITS.data_type == TYPE_LONG) {
			if (max77775_check_sec_uvdm_responder(usbpd_data,
					(int)SEC_UVDM_RESPONSE_HEADER->BITS.command_type, "DIR_OUT")) {
				SEC_UVDM_RX_HEADER = (U_SEC_RX_DATA_HEADER *)&read_msg[VDO2_OFFSET];

				print_sec_uvdm_rx_header_data(SEC_UVDM_RX_HEADER->data);

				max77775_check_sec_uvdm_rx_header
						(usbpd_data, SEC_UVDM_RX_HEADER->BITS.result_value);
			}
		} else { /* TYPE_SHORT */
			max77775_check_sec_uvdm_responder(usbpd_data,
				(int)SEC_UVDM_RESPONSE_HEADER->BITS.command_type, "DIR_OUT SHORT");
		}
	} else { /* after 2nd packet for TYPE_LONG */
		SEC_UVDM_RX_HEADER = (U_SEC_RX_DATA_HEADER *)&read_msg[6];
		max77775_check_sec_uvdm_rx_header
				(usbpd_data, SEC_UVDM_RX_HEADER->BITS.result_value);
	}
	complete(&usbpd_data->uvdm_longpacket_out_wait);
	msg_maxim("DIR_OUT complete!");
}

static void max77775_process_in_uvdm_response
				(struct max77775_usbc_platform_data *usbpd_data, uint8_t read_msg[])
{
	U_SEC_UVDM_RESPONSE_HEADER *SEC_UVDM_RESPONSE_HEADER;
	U_SEC_TX_DATA_HEADER *SEC_UVDM_TX_HEADER;

	if (!usbpd_data)
		return;

	if (usbpd_data->is_in_first_sec_uvdm_req) { /* LONG and SHORT response is same */
		SEC_UVDM_RESPONSE_HEADER = (U_SEC_UVDM_RESPONSE_HEADER *)&read_msg[VDO1_OFFSET];
		SEC_UVDM_TX_HEADER = (U_SEC_TX_DATA_HEADER *)&read_msg[VDO2_OFFSET];

		print_sec_uvdm_response_header(SEC_UVDM_RESPONSE_HEADER);

		if (max77775_check_sec_uvdm_responder(usbpd_data,
				(int)SEC_UVDM_RESPONSE_HEADER->BITS.command_type, "DIR_IN")) {

			memcpy(usbpd_data->ReadMSG, read_msg, OPCODE_DATA_LENGTH);

			print_sec_uvdm_tx_header(SEC_UVDM_TX_HEADER);

			msg_maxim("DIR_IN 1st Response");
		}
	} else {
		/* don't have ack packet after 2nd sec_tx_data_header */
		memcpy(usbpd_data->ReadMSG, read_msg, OPCODE_DATA_LENGTH);
		SEC_UVDM_TX_HEADER = (U_SEC_TX_DATA_HEADER *)&usbpd_data->ReadMSG[VDO1_OFFSET];

		print_sec_uvdm_tx_header(SEC_UVDM_TX_HEADER);

		if (SEC_UVDM_TX_HEADER->data != 0)
			msg_maxim("DIR_IN %dth Response", SEC_UVDM_TX_HEADER->BITS.order_of_current_uvdm_set);
		else
			msg_maxim("DIR_IN Last Response. It's zero");
	}
	complete(&usbpd_data->uvdm_longpacket_in_wait);
	msg_maxim("DIR_IN complete!");
}

void max77775_uvdm_opcode_response_handler(struct max77775_usbc_platform_data *usbpd_data,
		char *opcode_data, int len)
{
	int unstructured_len = sizeof(struct SS_UNSTRUCTURED_VDM_MSG);
	uint8_t read_msg[32] = {0,};

	if (!usbpd_data)
		return;

	if (len != unstructured_len + OPCODE_SIZE) {
		msg_maxim("This isn't UVDM message!");
		return;
	}

	memcpy(read_msg, opcode_data, OPCODE_DATA_LENGTH);
	usbpd_data->uvdm_error = 0;

	switch (usbpd_data->is_in_sec_uvdm_out) {
	case DIR_OUT:
		max77775_process_out_uvdm_response(usbpd_data, read_msg);
		break;
	case DIR_IN:
		max77775_process_in_uvdm_response(usbpd_data, read_msg);
		break;
	default:
		msg_maxim("Never Call!!!");
		break;
	}
}

int max77775_sec_uvdm_out_request_message(void *data, int size)
{
	struct max77775_usbc_platform_data *usbpd_data;
	struct i2c_client *i2c;
	int ret;

	usbpd_data = g_usbc_data;
	if (!usbpd_data)
		return -ENXIO;

	i2c = usbpd_data->muic;
	if (i2c == NULL) {
		msg_maxim("usbpd_data->i2c is not valid!");
		return -EINVAL;
	}

	if (data == NULL) {
		msg_maxim("given data is not valid !");
		return -EINVAL;
	}

	if (size >= SAMSUNGUVDM_MAX_LONGPACKET_SIZE) {
		msg_maxim("size %d is too big to send data", size);
		ret = -EFBIG;
	} else if (size <= SAMSUNGUVDM_MAX_SHORTPACKET_SIZE)
		ret = max77775_send_sec_unstructured_short_vdm_message(usbpd_data, data, size);
	else
		ret = max77775_send_sec_unstructured_long_vdm_message(usbpd_data, data, size);

	return ret;
}

int max77775_sec_uvdm_in_request_message(void *data)
{
	struct max77775_usbc_platform_data *usbpd_data;
	uint8_t SendMSG[32] = {0,};
	uint8_t ReadMSG[32] = {0,};
	uint8_t IN_DATA[MAX_INPUT_DATA] = {0,};

	/* Send Request */
	/* 1  Message Type Definition */
	U_SEC_UVDM_RESPONSE_HEADER	*SEC_RES_HEADER;
	U_SEC_TX_DATA_HEADER		*SEC_UVDM_TX_HEADER;
	U_SEC_TX_DATA_TAILER		*SEC_UVDM_TX_TAILER;

	int cur_uvdmset_data = 0;
	int cur_uvdmset_num = 0;
	int total_uvdmset_num = 0;
	int received_data_size = 0;
	int total_received_data_size = 0;
	int ack = 0;
	int size = 0;
	int time_left = 0;
	int cal_checksum = 0;
	int i = 0;
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	int event;
#endif

	usbpd_data = g_usbc_data;
	if (!usbpd_data)
		return -ENXIO;

	if (data == NULL) {
		msg_maxim("%s given data is not valid !", __func__);
		return -EINVAL;
	}

	usbpd_data->is_in_sec_uvdm_out = DIR_IN;
	usbpd_data->is_in_first_sec_uvdm_req = true;

	/* 1. Common : Fill the MSGHeader */
	set_msgheader(SendMSG, ACK, 2);
	/* 2. Common : Fill the UVDMHeader*/
	set_uvdmheader(SendMSG, SAMSUNG_VENDOR_ID, 0);
	/* 3. Common : Fill the First SEC_VDMHeader*/
	if (usbpd_data->is_in_first_sec_uvdm_req)
		set_sec_uvdmheader(SendMSG, usbpd_data->Product_ID, TYPE_LONG, SEC_UVDM_ININIATOR, DIR_IN, 0, 0);
	/* 8. Send data to PDIC */
	usbpd_data->uvdm_error = 0;
	max77775_request_uvdm(usbpd_data, SendMSG, 0);

	cur_uvdmset_num = 0;
	total_uvdmset_num = 1;

	do {
		reinit_completion(&usbpd_data->uvdm_longpacket_in_wait);
		time_left =
			wait_for_completion_interruptible_timeout(&usbpd_data->uvdm_longpacket_in_wait,
					msecs_to_jiffies(SAMSUNGUVDM_WAIT_MS));

		if (time_left <= 0) {
			msg_maxim("timeout");
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
			event = NOTIFY_EXTRA_UVDM_TIMEOUT;
			store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
			return -ETIME;
		}
		if (usbpd_data->uvdm_error)
			return usbpd_data->uvdm_error;
		/* read data */
		memset(ReadMSG, 0, 32);
		memcpy(ReadMSG, usbpd_data->ReadMSG, OPCODE_DATA_LENGTH);

		if (usbpd_data->is_in_first_sec_uvdm_req) {
			SEC_RES_HEADER = (U_SEC_UVDM_RESPONSE_HEADER *)&ReadMSG[6];
			SEC_UVDM_TX_HEADER = (U_SEC_TX_DATA_HEADER *)&ReadMSG[10];
#if (UVDM_DEBUG)
			msg_maxim("SEC_RES_HEADER : 0x%x, SEC_UVDM_TX_HEADER : 0x%x", SEC_RES_HEADER->data, SEC_UVDM_TX_HEADER->data);
#endif
			if (SEC_RES_HEADER->BITS.data_type == TYPE_LONG) {
				/* 1. check the data size received */
				size = SEC_UVDM_TX_HEADER->BITS.total_data_size;
				cur_uvdmset_data = SEC_UVDM_TX_HEADER->BITS.data_size_of_current_set;
				cur_uvdmset_num = SEC_UVDM_TX_HEADER->BITS.order_of_current_uvdm_set;
				total_uvdmset_num =
					SEC_RES_HEADER->BITS.total_number_of_uvdm_set;

				usbpd_data->is_in_first_sec_uvdm_req = false;
				/* 2. copy data to buffer */
				for (i = 0; i < SAMSUNGUVDM_MAXDATA_FIRST_UVDMSET; i++)
					IN_DATA[received_data_size++] = ReadMSG[14 + i];

				total_received_data_size += cur_uvdmset_data;
				usbpd_data->is_in_first_sec_uvdm_req = false;
			} else {
				IN_DATA[received_data_size++] = SEC_RES_HEADER->BITS.data;
				return received_data_size;
			}
		} else {
			SEC_UVDM_TX_HEADER = (U_SEC_TX_DATA_HEADER *)&ReadMSG[6];
			cur_uvdmset_data = SEC_UVDM_TX_HEADER->BITS.data_size_of_current_set;
			cur_uvdmset_num = SEC_UVDM_TX_HEADER->BITS.order_of_current_uvdm_set;
			/* 2. copy data to buffer */
			for (i = 0 ; i < SAMSUNGUVDM_MAXDATA_NORMAL_UVDMSET ; i++)
				IN_DATA[received_data_size++] = ReadMSG[10 + i];
			total_received_data_size += cur_uvdmset_data;
		}
		/* 3. Check Checksum */
		SEC_UVDM_TX_TAILER = (U_SEC_TX_DATA_TAILER *)&ReadMSG[26];
		cal_checksum = get_checksum(ReadMSG, 6, SAMSUNGUVDM_CHECKSUM_DATA_COUNT);
		ack = (cal_checksum == SEC_UVDM_TX_TAILER->BITS.checksum) ?
			SEC_UVDM_RX_HEADER_ACK : SEC_UVDM_RX_HEADER_NAK;
#if (UVDM_DEBUG)
		msg_maxim("SEC_UVDM_TX_TAILER : 0x%x, cal_checksum : 0x%x, size : %d", SEC_UVDM_TX_TAILER->data, cal_checksum, size);
#endif
		/* 1. Common : Fill the MSGHeader */
		if (cur_uvdmset_num == total_uvdmset_num)
			set_msgheader(SendMSG, NAK, 2);
		else
			set_msgheader(SendMSG, ACK, 2);
		/* 2. Common : Fill the UVDMHeader*/
		set_uvdmheader(SendMSG, SAMSUNG_VENDOR_ID, 0);
		/* 5-3. Common : Fill the SEC RXHeader */
		set_sec_uvdm_rxdata_header(SendMSG, cur_uvdmset_num, cur_uvdmset_data, ack);

		/* 8. Send data to PDIC */
		usbpd_data->uvdm_error = 0;
		max77775_request_uvdm(usbpd_data, SendMSG, 0);
	} while (cur_uvdmset_num < total_uvdmset_num);
	set_endian(IN_DATA, data, size);

	reinit_completion(&usbpd_data->uvdm_longpacket_in_wait);
	time_left =
		wait_for_completion_interruptible_timeout(&usbpd_data->uvdm_longpacket_in_wait,
				msecs_to_jiffies(SAMSUNGUVDM_WAIT_MS));
	if (time_left <= 0) {
		msg_maxim("last in request timeout");
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
		event = NOTIFY_EXTRA_UVDM_TIMEOUT;
		store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
		return -ETIME;
	}
	if (usbpd_data->uvdm_error)
		return usbpd_data->uvdm_error;

	return size;
}

int max77775_sec_uvdm_ready(void)
{
	int uvdm_ready = false;
	struct max77775_usbc_platform_data *usbpd_data;

	usbpd_data = g_usbc_data;

	msg_maxim("power_nego is%s done, accessory_enter_mode is%s done",
			usbpd_data->pn_flag ? "" : " not",
			usbpd_data->is_samsung_accessory_enter_mode ? "" : " not");

	if (usbpd_data->pn_flag &&
			usbpd_data->is_samsung_accessory_enter_mode)
		uvdm_ready = true;

	msg_maxim("uvdm ready = %d", uvdm_ready);
	return uvdm_ready;
}

void max77775_sec_uvdm_close(void)
{
	struct max77775_usbc_platform_data *usbpd_data;

	usbpd_data = g_usbc_data;
	complete(&usbpd_data->uvdm_longpacket_out_wait);
	complete(&usbpd_data->uvdm_longpacket_in_wait);
	msg_maxim("success");
}
