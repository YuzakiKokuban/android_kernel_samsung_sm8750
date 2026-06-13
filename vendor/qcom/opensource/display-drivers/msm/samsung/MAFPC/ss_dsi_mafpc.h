/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * DDI ABC operation
 * Author: Samsung Display Driver Team <cj1225.jang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SS_DSI_MAFPC_H__
#define __SS_DSI_MAFPC_H__

#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/file.h>

 /* Align & Paylod Size for Embedded mode Transfer */
#define MAFPC_CMD_ALIGN 16
#define MAFPC_MAX_PAYLOAD_SIZE 200

 /* Align & Paylod Size for Non-Embedded mode(Mass) Command Transfer */
#define MAFPC_MASS_CMD_ALIGN 256
#define MAFPC_MAX_PAYLOAD_SIZE_MASS 0xFFFF0 /* 983,024 */

/*
 * ioctl
 */
#define MAFPC_IOCTL_MAGIC	'M'
#define MAFPC_IOCTL_ON		_IOW(MAFPC_IOCTL_MAGIC, 1, int)
#define MAFPC_IOCTL_ON_INSTANT	_IOW(MAFPC_IOCTL_MAGIC, 2, int)
#define MAFPC_IOCTL_OFF		_IOW(MAFPC_IOCTL_MAGIC, 3, int)
#define MAFPC_IOCTL_OFF_INSTANT	_IOW(MAFPC_IOCTL_MAGIC, 4, int)
#define MAFPC_IOCTL_CLEAR_IMAGE_BUFFER  _IOW(MAFPC_IOCTL_MAGIC, 5, int)
#define MAFPC_IOCTL_GET_VER        _IOR(MAFPC_IOCTL_MAGIC, 6, __u32)
#define MAFPC_IOCTL_GET_CRC        _IOR(MAFPC_IOCTL_MAGIC, 7, __u32)
#define IOCTL_MAFPC_MAX		10

struct ss_cmd_desc;

struct MAFPC {
	struct miscdevice dev;

	int is_support;
	int factory_support;
	int file_open;
	int need_to_write;
	int is_br_table_updated;
	int force_delay;

	int en;
	u32 version;
	

	char *enable_cmd_buf;
	u32 enable_cmd_size;
	u32 header_cmd_size;
	char *img_buf;
	u32 img_size;
	u32 img_crc;
	char *scale_table_buf;
	u32 scale_table_size;

	int scale_idx;

	u32 wpos;
	u64 wsize;

	int img_checksum;
	u32 img_checksum_cal;

	struct mutex vdd_mafpc_lock;
	struct mutex vdd_mafpc_crc_check_lock;

	char *crc_img_buf;
	u32 crc_img_size;

	u8 crc_pass_data[2];	/*implemented in dtsi */
	u8 crc_read_data[2];
	int crc_size;

	/* mafpc Function */
	int (*init)(struct samsung_display_driver_data *vdd);
	int (*data_init)(struct samsung_display_driver_data *vdd);
	int (*make_img_cmds)(struct samsung_display_driver_data *vdd, char *data, u32 data_size, int cmd_type);
	int (*make_img_mass_cmds)(struct samsung_display_driver_data *vdd, char *data, u32 data_size, struct ss_cmd_desc *ss_cmd);
	int (*img_write)(struct samsung_display_driver_data *vdd, bool is_instant);
	int (*enable)(struct samsung_display_driver_data *vdd, int enable);
	int (*crc_check)(struct samsung_display_driver_data *vdd);
	int (*debug)(struct samsung_display_driver_data *vdd);
};

int ss_mafpc_init(struct samsung_display_driver_data *vdd);

/* 2551 (Normal: 0~2550) + 1 (HBM) */
#define MAX_MAFPC_BL_SCALE	2552

static int brightness_scale_idx[MAX_MAFPC_BL_SCALE] = {
	[0 ... 9] = 0,
	[10 ... 19] = 1,
	[20 ... 30] = 2,
	[31 ... 40] = 3,
	[41 ... 50] = 4,
	[51 ... 59] = 5,
	[60 ... 68] = 6,
	[69 ... 79] = 7,
	[80 ... 90] = 8,
	[91 ... 100] = 9,
	[101 ... 120] = 10,
	[121 ... 129] = 11,
	[130 ... 139] = 12,
	[140 ... 158] = 13,
	[159 ... 178] = 14,
	[179 ... 188] = 15,
	[189 ... 200] = 16,
	[201 ... 210] = 17,
	[211 ... 229] = 18,
	[230 ... 249] = 19,
	[250 ... 258] = 20,
	[259 ... 269] = 21,
	[270 ... 299] = 22,
	[300 ... 319] = 23,
	[320 ... 339] = 24,
	[340 ... 359] = 25,
	[360 ... 379] = 26,
	[380 ... 409] = 27,
	[410 ... 439] = 28,
	[440 ... 459] = 29,
	[460 ... 499] = 30,
	[500 ... 530] = 31,
	[531 ... 570] = 32,
	[571 ... 599] = 33,
	[600 ... 639] = 34,
	[640 ... 678] = 35,
	[679 ... 729] = 36,
	[730 ... 769] = 37,
	[770 ... 829] = 38,
	[830 ... 880] = 39,
	[881 ... 930] = 40,
	[931 ... 999] = 41,
	[1000 ... 1059] = 42,
	[1060 ... 1129] = 43,
	[1130 ... 1199] = 44,
	[1200 ... 1280] = 45,
	[1281 ... 1339] = 46,
	[1340 ... 1409] = 47,
	[1410 ... 1479] = 48,
	[1480 ... 1559] = 49,
	[1560 ... 1599] = 50,
	[1600 ... 1640] = 51,
	[1641 ... 1679] = 52,
	[1680 ... 1729] = 53,
	[1730 ... 1769] = 54,
	[1770 ... 1819] = 55,
	[1820 ... 1919] = 56,
	[1920 ... 2000] = 57,
	[2001 ... 2099] = 58,
	[2100 ... 2179] = 59,
	[2180 ... 2189] = 60,
	[2190 ... 2209] = 61,
	[2210 ... 2229] = 62,
	[2230 ... 2249] = 63,
	[2250 ... 2269] = 64,
	[2270 ... 2289] = 65,
	[2290 ... 2309] = 66,
	[2310 ... 2329] = 67,
	[2330 ... 2350] = 68,
	[2351 ... 2400] = 69,
	[2401 ... 2449] = 70,
	[2450 ... 2499] = 71,
	[2500 ... 2549] = 72,
	[2550] = 73,
	[2551] = 74, /* for HBM */
};

#endif /* __SELF_DISPLAY_H__ */
