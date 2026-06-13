/*
 *
 * Copyright (C) 2017-2020 Samsung Electronics
 *
 * Author:Wookwang Lee. <wookwang.lee@samsung.com>,
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
 * along with this program.If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __LINUX_PDIC_CORE_H__
#define __LINUX_PDIC_CORE_H__

/* PDIC Dock Observer Callback parameter */
enum {
	PDIC_DOCK_DETACHED	= 0,
	PDIC_DOCK_HMT		= 105,	/* Samsung Gear VR */
	PDIC_DOCK_ABNORMAL	= 106,
	PDIC_DOCK_MPA		= 109,	/* Samsung Multi Port Adaptor */
	PDIC_DOCK_DEX		= 110,	/* Samsung Dex */
	PDIC_DOCK_HDMI		= 111,	/* Samsung HDMI Dongle */
	PDIC_DOCK_T_VR		= 112,
	PDIC_DOCK_UVDM		= 113,
	PDIC_DOCK_DEXPAD	= 114,
	PDIC_DOCK_UNSUPPORTED_AUDIO = 115,	/* Ra/Ra TypeC Analog Earphone*/
	PDIC_DOCK_NEW		= 200,  /* For New uevent */
};

typedef enum {
	TYPE_C_DETACH = 0,
	TYPE_C_ATTACH_DFP = 1, /* Host */
	TYPE_C_ATTACH_UFP = 2, /* Device */
	TYPE_C_ATTACH_DRP = 3, /* Dual role */
	TYPE_C_ATTACH_SRC = 4, /* SRC */
	TYPE_C_ATTACH_SNK = 5, /* SNK */
	TYPE_C_RR_SWAP = 6,
	TYPE_C_DR_SWAP = 7,
} PDIC_OTP_MODE;

#if defined(CONFIG_TYPEC)
typedef enum {
	TRY_ROLE_SWAP_NONE = 0,
	TRY_ROLE_SWAP_PR = 1, /* pr_swap */
	TRY_ROLE_SWAP_DR = 2, /* dr_swap */
	TRY_ROLE_SWAP_TYPE = 3, /* type */
	TRY_ROLE_SWAP_VC = 4, /* vconn swap */
} PDIC_ROLE_SWAP_MODE;

#define TRY_ROLE_SWAP_WAIT_MS 5000
#endif
#define DUAL_ROLE_SET_MODE_WAIT_MS 2000
#define GEAR_VR_DETACH_WAIT_MS		1000
#define SAMSUNG_PRODUCT_ID		0x6860
#define SAMSUNG_PRODUCT_TYPE	0x2
/* Samsung Acc VID */
#define SAMSUNG_VENDOR_ID		0x04E8
#define SAMSUNG_MPA_VENDOR_ID		0x04B4
#define TypeC_DP_SUPPORT	(0xFF01)
/* Samsung Acc PID */
#define GEARVR_PRODUCT_ID		0xA500
#define GEARVR_PRODUCT_ID_1		0xA501
#define GEARVR_PRODUCT_ID_2		0xA502
#define GEARVR_PRODUCT_ID_3		0xA503
#define GEARVR_PRODUCT_ID_4		0xA504
#define GEARVR_PRODUCT_ID_5		0xA505
#define DEXDOCK_PRODUCT_ID		0xA020
#define HDMI_PRODUCT_ID			0xA025
#define MPA2_PRODUCT_ID			0xA027
#define UVDM_PROTOCOL_ID		0xA028
#define DEXPAD_PRODUCT_ID		0xA029
#define MPA_PRODUCT_ID			0x2122
#define FRIENDS_PRODUCT_ID		0xB002

/* Samsung UVDM structure */
#define SEC_UVDM_SHORT_DATA		0x0
#define SEC_UVDM_LONG_DATA		0x1
#define SEC_UVDM_ININIATOR		0x0
#define SEC_UVDM_RESPONDER_INIT	0x0
#define SEC_UVDM_RESPONDER_ACK	0x1
#define SEC_UVDM_RESPONDER_NAK	0x2
#define SEC_UVDM_RESPONDER_BUSY	0x3
#define SEC_UVDM_RX_HEADER_BUSY	0x2
#define SEC_UVDM_UNSTRUCTURED_VDM	0x4
#define SEC_UVDM_RX_HEADER_ACK	0x0
#define SEC_UVDM_RX_HEADER_NAK	0x1


#define SEC_UVDM_ALIGN (4)
#define SEC_UVDM_MAXDATA_FIRST (12)
#define SEC_UVDM_MAXDATA_NORMAL (16)
#define SEC_UVDM_CHECKSUM_COUNT (20)

/*For DP Pin Assignment */
#define DP_PIN_ASSIGNMENT_NODE	0x00000000
#define DP_PIN_ASSIGNMENT_A	0x00000001	/* ( 1 << 0 ) */
#define DP_PIN_ASSIGNMENT_B	0x00000002	/* ( 1 << 1 ) */
#define DP_PIN_ASSIGNMENT_C	0x00000004	/* ( 1 << 2 ) */
#define DP_PIN_ASSIGNMENT_D	0x00000008	/* ( 1 << 3 ) */
#define DP_PIN_ASSIGNMENT_E	0x00000010	/* ( 1 << 4 ) */
#define DP_PIN_ASSIGNMENT_F	0x00000020	/* ( 1 << 5 ) */

#define MAX_BUF_DATA 256

typedef union {
	u16 word;
	u8  byte[2];

	struct {
		unsigned msg_type:5;
		unsigned port_data_role:1;
		unsigned spec_revision:2;
		unsigned port_power_role:1;
		unsigned msg_id:3;
		unsigned num_data_objs:3;
		unsigned extended:1;
	};
} msg_header_type;

typedef union {
	u32 object;
	u16 word[2];
	u8  byte[4];
	struct {
		unsigned vendor_defined:15;
		unsigned vdm_type:1;
		unsigned vendor_id:16;
	};
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
} uvdm_header;

typedef union {
	u32 object;
	u16 word[2];
	u8  byte[4];

	struct{
		unsigned data:8;
		unsigned total_set_num:4;
		unsigned direction:1;
		unsigned cmd_type:2;
		unsigned data_type:1;
		unsigned pid:16;
	};
} s_uvdm_header;

typedef union {
	u32 object;
	u16 word[2];
	u8  byte[4];

	struct{
		unsigned cur_size:8;
		unsigned total_size:8;
		unsigned reserved:12;
		unsigned order_cur_set:4;
	};
} s_tx_header;

typedef union {
	u32 object;
	u16 word[2];
	u8  byte[4];

	struct{
		unsigned checksum:16;
		unsigned reserved:16;
	};
} s_tx_tailer;

typedef union {
	u32 object;
	u16 word[2];
	u8  byte[4];

	struct{
		unsigned reserved:18;
		unsigned result_value:2;
		unsigned rcv_data_size:8;
		unsigned order_cur_set:4;
	};
} s_rx_header;

enum usbpd_port_data_role {
	USBPD_UFP,
	USBPD_DFP,
};

enum usbpd_port_power_role {
	USBPD_SINK,
	USBPD_SOURCE,
	USBPD_DRP,
};

enum usbpd_port_vconn_role {
	USBPD_VCONN_OFF,
	USBPD_VCONN_ON,
};

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
struct pdic_state_work {
	struct work_struct pdic_work;
	int dest;
	int id;
	int attach;
	int event;
	int sub;
};
typedef enum {
	CLIENT_OFF = 0,
	CLIENT_ON = 1,
} PDIC_DEVICE_REASON;

typedef enum {
	HOST_OFF = 0,
	HOST_ON = 1,
} PDIC_HOST_REASON;
#endif

enum uvdm_data_type {
	TYPE_SHORT = 0,
	TYPE_LONG,
};

enum uvdm_direction_type {
	DIR_OUT = 0,
	DIR_IN,
};

struct uvdm_data {
	unsigned short pid; /* Product ID */
	char type; /* uvdm_data_type */
	char dir; /* uvdm_direction_type */
	unsigned int size; /* data size */
	void __user *pData; /* data pointer */
};
#ifdef CONFIG_COMPAT
struct uvdm_data_32 {
	unsigned short pid; /* Product ID */
	char type; /* uvdm_data_type */
	char dir; /* uvdm_direction_type */
	unsigned int size; /* data size */
	compat_uptr_t pData; /* data pointer */
};
#endif

struct pdic_misc_dev {
	struct uvdm_data u_data;
#ifdef CONFIG_COMPAT
	struct uvdm_data_32 u_data_32;
#endif
	atomic_t open_excl;
	atomic_t ioctl_excl;
	int (*uvdm_write)(void *data, int size);
	int (*uvdm_read)(void *data);
	int (*uvdm_ready)(void);
	void (*uvdm_close)(void);
	bool (*pps_control)(int en);
};

struct pdic_misc_data {
	void *fw_buf;
	size_t offset;
	size_t fw_buf_size;
	int is_error;
};

struct pdic_data {
	int (*firmware_update)(void *data,
		void *fw_bin, size_t fw_size);
	size_t (*get_prev_fw_size)(void *data);
	void *data;
};

struct pdic_fwupdate_data {
	struct pdic_misc_data *misc_data;
	struct pdic_data *ic_data;
	atomic_t opened;
};

struct pdic_misc_core {
	struct pdic_misc_dev c_dev;
	struct pdic_fwupdate_data fw_data;
};

typedef struct _pdic_data_t {
	const char *name;
	void *pdic_sysfs_prop;
	void *drv_data;
	void (*set_enable_alternate_mode)(int);
	struct pdic_misc_dev *misc_dev;
	struct pdic_data fw_data;
} pdic_data_t, *ppdic_data_t;

/* ----------------------------------
 *          pdic_core.c functions
 *-----------------------------------
 */
int pdic_core_init(void);
int pdic_core_register_chip(ppdic_data_t ppdic_data);
void pdic_core_unregister_chip(void);
int pdic_register_switch_device(int mode);
void pdic_send_dock_intent(int type);
void pdic_send_dock_uevent(u32 vid, u32 pid, int state);
void *pdic_core_get_drvdata(void);
/* ----------------------------------
 *          pdic_misc.c functions
 *-----------------------------------
 */
int pdic_misc_init(ppdic_data_t ppdic_data);
void pdic_misc_exit(void);
/* SEC UVDM Utility function */
int get_checksum(const char *data, int start_addr, int size);
int get_data_size(bool is_first_data, int data_size);
int set_endian(const char *src, char *dest, int size);
int set_uvdmset_count(int size);
void set_msg_header(void *data, int msg_type, int obj_num);
void set_uvdm_header(void *data, int vid, int vdm_type);
void set_sec_uvdm_header(void *data, int pid, bool data_type, int cmd_type,
		bool dir, int total_set_num, uint8_t received_data);
void set_sec_uvdm_tx_header(void *data, int first_set, int cur_set, int total_size,
		int remained_size);
void set_sec_uvdm_tx_tailer(void *data);
void set_sec_uvdm_rx_header(void *data, int cur_num, int cur_set, int ack);
struct device *get_pdic_device(void);
#endif

