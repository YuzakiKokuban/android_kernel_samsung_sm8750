/*
 * 
 *	
 *
 * Copyright (C) 2021 Samsung Electronics
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#ifndef __PDIC_SPEC_DEF_H__
#define __PDIC_SPEC_DEF_H__

#define DP_SVID			(0xFF01)
#define SAMSUNG_SVID	(0x04E8)
#define PD_SID			(0xFF00)

typedef enum {
	S_DISCOVER_IDENTIFY	= 0x1,
	S_DISCOVER_SVIDS,
	S_DISCOVER_MODES,
	S_ENTER_MODE,
	S_EXIT_MODE,
	S_ATTENTION,
	S_DISPLAYPORT_STATUS_UPDATE	= 0x10,
	S_DISPLAYPORT_CONFIGURE	= 0x11,
} S_VDM_HEADER_COMMAND;

typedef enum {
	S_TYPE_REQ = 0,
	S_TYPE_ACK,
	S_TYPE_NAK,
	S_TYPE_BUSY,
} S_VDM_HEADER_COMMAND_TYPE;

typedef union {
	uint32_t        DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t	Number_of_obj:3,
				MSG_ID:3,
				Port_Power_Role:1,
				Specification_Rev:2,
				Port_Data_Role:1,
				Reserved:1,
				MSG_Type:4;
	} BITS;
} S_MSG_HEADER;

typedef union {
	uint32_t        DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t	VDM_command:5,
				Rsvd2_VDM_header:1,
				VDM_command_type:2,
				Object_Position:3,
				Rsvd_VDM_header:2,
				Structured_VDM_Version:2,
				VDM_Type:1,
				Standard_Vendor_ID:16;
	} BITS;
} S_VDM_HEADER;

typedef union {
	uint32_t        DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t	USB_Vendor_ID:16,
				Rsvd_ID_header:10,
				Modal_Operation_Supported:1,
				Product_Type:3,
				Data_Capable_USB_Device:1,
				Data_Capable_USB_Host:1;
	} BITS;
} S_ID_HEADER_VDO;

typedef union {
	uint32_t	DATA;
	struct {
		uint8_t		BDATA[4];
	} BYTES;
	struct {
		uint32_t	Cert_TID:20,
				Rsvd_cert_VDOer:12;
	} BITS;
} S_CERT_STAT_VDO;

typedef union {
	uint32_t        DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t    Device_Version:16,
			    Product_ID:16;
	} BITS;
} S_PRODUCT_VDO;

typedef union {
	uint32_t        DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t	USB_Superspeed_Signaling_Support:3,
				SOP_contoller_present:1,
				Vbus_through_cable:1,
				Vbus_Current_Handling_Capability:2,
				SSRX2_Directionality_Support:1,
				SSRX1_Directionality_Support:1,
				SSTX2_Directionality_Support:1,
				SSTX1_Directionality_Support:1,
				Cable_Termination_Type:2,
				Cable_Latency:4,
				TypeC_to_Plug_Receptacle:1,
				TypeC_to_ABC:2,
				Rsvd_CABLE_VDO:4,
				Cable_Firmware_Version:4,
				Cable_HW_Version:4;
	} BITS;
} S_CABLE_VDO;

typedef union {
	uint32_t        DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t    SVID_1:16,
			    SVID_0:16;
	} BITS;
} S_SVID_VDO;

typedef struct {
	S_MSG_HEADER		msg_header;
	S_VDM_HEADER		vdm_header;
	S_ID_HEADER_VDO		id_header;
	S_CERT_STAT_VDO		cert_stat;
	S_PRODUCT_VDO		product;
} DISCOVER_IDENTITY_RESPONSE;

typedef struct {
	S_MSG_HEADER		msg_header;
	S_VDM_HEADER		vdm_header;
	S_SVID_VDO			svid_vdo[6];
} DISCOVER_SVID_RESPONSE;

typedef struct {
	S_MSG_HEADER		msg_header;
	S_VDM_HEADER		vdm_header;
} ENTER_MODE_RESPONSE;

/*
 *
 * Display port defines
 *
 */

/* For DP VDM Modes VDO Port_Capability */
typedef enum
{
    S_RESERVED					= 0,
    S_UFP_D_CAPABLE				= 1,
    S_DFP_D_CAPABLE				= 2,
    S_DFP_D_AND_UFP_D_CAPABLE	= 3
} S_DP_CAPABILITIES_VDO_PORT_CAPABILITY;

/* For DP VDM Modes VDO Receptacle_Indication */
typedef enum
{
    S_USB_TYPE_C_PLUG			= 0,
    S_USB_TYPE_C_RECEPTACLE		= 1
} S_DP_CAPABILITIES_VDO_RECEPTACLE_INDICATION;

/* For DP_Status_Update Port_Connected */
typedef enum
{
    S_ADAPTOR_DISABLE         = 0,
    S_CONNECT_DFP_D           = 1,
    S_CONNECT_UFP_D           = 2,
    S_CONNECT_DFP_D_and_UFP_D = 3
} S_DP_STATUS_UPDATE_VDO_CONNECTED;

/* For DP_Configure Select_Configuration */
typedef enum
{
    S_CFG_FOR_USB             = 0,
    S_CFG_UFP_U_AS_DFP_D      = 1,
    S_CFG_UFP_U_AS_UFP_D      = 2,
    S_CFG_RESERVED            = 3
} S_DP_STATUS_UPDATE_VDO_SELECT_CONFIGUTATION;

typedef union {
	uint32_t	DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t	Port_Capability:2,
				Signalling_DP:4,
				Receptacle_Indication:1,
				USB_2p0_Not_Used:1,
				DFP_D_Pin_Assignments:8,
				UFP_D_Pin_Assignments:8,
				DP_MODE_VDO_Reserved:8;
	} BITS;
} S_DP_CAPABILITIES_VDO;

typedef union {
	uint32_t    DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t    Port_Connected:2,
			    Power_Low:1,
			    Enabled:1,
			    Multi_Function_Preference:1,
			    USB_Configuration_Req:1,
			    Exit_DP_Mode_Req:1,
			    HPD_State:1,
			    HPD_Interrupt:1,
			    Reserved:23;
	} BITS;
} S_DP_STATUS_UPDATE_VDO;

typedef struct {
	S_MSG_HEADER			msg_header;
	S_VDM_HEADER			vdm_header;
	S_DP_CAPABILITIES_VDO	dp_mode_vdo;
} DP_DISCOVER_MODE_RESPONSE;

typedef struct {
	S_MSG_HEADER			msg_header;
	S_VDM_HEADER			vdm_header;
	S_DP_STATUS_UPDATE_VDO	dp_status_update_vdo;
} DP_STATUS_RESPONSE;

typedef struct {
	S_MSG_HEADER			msg_header;
	S_VDM_HEADER			vdm_header;
} DP_CONFIGURATION_RESPONSE;

typedef struct {
	S_MSG_HEADER			msg_header;
	S_VDM_HEADER			vdm_header;
	S_DP_STATUS_UPDATE_VDO	dp_status_update_vdo;
} DP_ATTENTION_REQUEST;
#endif /* __PDIC_SPEC_DEF_H__ */
