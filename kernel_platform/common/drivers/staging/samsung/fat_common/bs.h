/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com/
 */

#ifndef _FAT_COMMON_BS_H
#define _FAT_COMMON_BS_H

#include <linux/types.h>
#include <linux/magic.h>
#include <asm/byteorder.h>

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/
/* PBR entries */
#define PBR_SIGNATURE	0xAA55
#define EXT_SIGNATURE	0xAA550000
#define OEM_NAME_EXFAT	"EXFAT   "	/* size should be 8 */
#define OEM_NAME_NTFS	"NTFS    "	/* size should be 8 */
#define OEM_NAME_LEN	(8)

/* Major values */
#define DENTRY_SIZE_BITS	(5)
#define SECT_SIZE_BITS		(9)
#define CLUS_BASE		(2)

/* max number of clusters */
#define FAT12_THRESHOLD         4087        // 2^12 - 1 + 2 (clu 0 & 1)
#define FAT16_THRESHOLD         65527       // 2^16 - 1 + 2
#define FAT32_THRESHOLD         268435457   // 2^28 - 1 + 2
#define EXFAT_THRESHOLD         268435457   // 2^28 - 1 + 2

/*----------------------------------------------------------------------*/
/*  On-Disk Type Definitions                                            */
/*----------------------------------------------------------------------*/

/* FAT12/16/32 BIOS parameter block (64 bytes) */
struct bpb_t {
	__u8	jmp_boot[3];
	__u8	oem_name[8];

	__u8	sect_size[2];		/* unaligned */
	__u8	sect_per_clus;
	__le16	num_reserved;		/* . */
	__u8	num_fats;
	__u8	num_root_entries[2];	/* unaligned */
	__u8	num_sectors[2];		/* unaligned */
	__u8	media_type;
	__le16  num_fat_sectors;
	__le16  sectors_in_track;
	__le16  num_heads;
	__le32	num_hid_sectors;	/* . */
	__le32	num_huge_sectors;

	union {
		struct {
			__u8	phy_drv_no;
			__u8	state;	/* used by WinNT for mount state */
			__u8	ext_signature;
			__u8	vol_serial[4];
			__u8	vol_label[11];
			__u8	vol_type[8];
			__le16  nouse;
		} f16;

		struct {
			__le32	num_fat32_sectors;
			__le16	ext_flags;
			__u8	fs_version[2];
			__le32	root_cluster;		/* . */
			__le16	fsinfo_sector;
			__le16	backup_sector;
			__le16	reserved[6];		/* . */
		} f32;
	};
};

/* FAT32 EXTEND BIOS parameter block (32 bytes) */
struct bsx32_t {
	__u8	phy_drv_no;
	__u8	state;			/* used by WindowsNT for mount state */
	__u8	ext_signature;
	__u8	vol_serial[4];
	__u8	vol_label[11];
	__u8	vol_type[8];
	__le16  dummy[3];
};

/* EXFAT BIOS parameter block (64 bytes) */
struct bpb64_t {
	__u8	jmp_boot[3];
	__u8	oem_name[8];
	__u8	res_zero[53];
};

/* EXFAT EXTEND BIOS parameter block (56 bytes) */
struct bsx64_t {
	__le64	vol_offset;
	__le64	vol_length;
	__le32	fat_offset;
	__le32	fat_length;
	__le32	clu_offset;
	__le32	clu_count;
	__le32	root_cluster;
	__le32	vol_serial;
	__u8	fs_version[2];
	__le16	vol_flags;
	__u8	sect_size_bits;
	__u8	sect_per_clus_bits;
	__u8	num_fats;
	__u8	phy_drv_no;
	__u8	perc_in_use;
	__u8	reserved2[7];
};

/* FAT32 PBR (64 bytes) */
struct pbr16_t {
	struct bpb_t bpb;
};

/* FAT32 PBR[BPB+BSX] (96 bytes) */
struct pbr32_t {
	struct bpb_t bpb;
	struct bsx32_t bsx;
};

/* EXFAT PBR[BPB+BSX] (120 bytes) */
struct pbr64_t {
	struct bpb64_t bpb;
	struct bsx64_t bsx;
};

/* Common PBR[Partition Boot Record] (512 bytes) */
struct pbr_t {
	union {
		__u8	raw[64];
		struct bpb_t	fat;
		struct bpb64_t f64;
	} bpb;
	union {
		__u8	raw[56];
		struct bsx32_t f32;
		struct bsx64_t f64;
	} bsx;
	__u8	boot_code[390];
	__le16	signature;
};

/* FAT32 filesystem information sector (512 bytes) */
struct fat32_fsi_t {
	__le32	signature1;              // aligned
	__u8	reserved1[480];
	__le32	signature2;              // aligned
	__le32	free_cluster;            // aligned
	__le32	next_cluster;            // aligned
	__u8    reserved2[14];
	__le16	signature3[2];
};

#endif /* _FAT_COMMON_BS_H */
