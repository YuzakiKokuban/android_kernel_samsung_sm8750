// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com/
 */

#include <linux/fat_common.h>
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <asm/unaligned.h>
#include "bs.h"

#ifdef CONFIG_PROC_STLOG
#include <linux/fslog.h>
#define ST_LOG(fmt, ...) fslog_stlog(fmt, ##__VA_ARGS__)
#else
#define ST_LOG(fmt, ...)
#endif

#ifdef CONFIG_FS_COMMON_STLOG

struct fs_common_fsinfo {
	u8 type[OEM_NAME_LEN];
	u32 vol_id;
	u32 sect_per_clus;
	u64 data_start_sect;
	u64 total_sectors;
	u64 root_start_sect;
	u32 root_sectors;
};

static void __get_fsi_exfat(struct super_block *sb, struct pbr_t *pbr,
			    struct fs_common_fsinfo *fsi)
{
	struct pbr64_t *bs = (struct pbr64_t *)pbr;

	strscpy(fsi->type, "exfat", OEM_NAME_LEN);
	fsi->sect_per_clus = 1 << bs->bsx.sect_per_clus_bits;
	fsi->vol_id = le32_to_cpu(bs->bsx.vol_serial);
	fsi->root_start_sect = 0;
	fsi->data_start_sect = le32_to_cpu(bs->bsx.clu_offset);
	fsi->total_sectors = le64_to_cpu(bs->bsx.vol_length);
}

static void __get_fsi_fat32(struct super_block *sb, struct pbr_t *pbr,
			    struct fs_common_fsinfo *fsi)
{
	struct pbr32_t *bs = (struct pbr32_t *)pbr;
	u64 data_start_sect;
	u32 fat_sectors;

	strscpy(fsi->type, "fat32", OEM_NAME_LEN);
	fsi->sect_per_clus = bs->bpb.sect_per_clus;

	fat_sectors = le32_to_cpu(bs->bpb.f32.num_fat32_sectors);
	if (!fat_sectors)
		fat_sectors = le16_to_cpu(bs->bpb.num_fat_sectors);

	data_start_sect = le16_to_cpu(bs->bpb.num_reserved);
	data_start_sect += fat_sectors;
	if (bs->bpb.num_fats > 1)
		data_start_sect += fat_sectors;

	fsi->root_start_sect = 0;
	fsi->data_start_sect = data_start_sect;
	fsi->vol_id = get_unaligned_le32(bs->bsx.vol_serial);
	fsi->total_sectors = get_unaligned_le16(bs->bpb.num_sectors);
	if (!fsi->total_sectors)
		fsi->total_sectors = le32_to_cpu(bs->bpb.num_huge_sectors);
}

static void __get_fsi_fat(struct super_block *sb, struct pbr_t *pbr,
			  struct fs_common_fsinfo *fsi)
{
	struct pbr32_t *bs = (struct pbr32_t *)pbr;
	u64 root_start_sect;
	u32 fat_sectors;
	u32 root_sectors;
	u32 num_clusters;
	u32 sect_per_clus_bits;

	strscpy(fsi->type, "fat16", OEM_NAME_LEN);
	fsi->sect_per_clus = bs->bpb.sect_per_clus;
	sect_per_clus_bits = ilog2(bs->bpb.sect_per_clus);
	root_sectors = get_unaligned_le16(bs->bpb.num_root_entries);
	root_sectors <<= DENTRY_SIZE_BITS;
	root_sectors = DIV_ROUND_UP(root_sectors, sb->s_blocksize);

	fat_sectors = le16_to_cpu(bs->bpb.num_fat_sectors);

	root_start_sect = le16_to_cpu(bs->bpb.num_reserved);
	root_start_sect += fat_sectors;
	if (bs->bpb.num_fats > 1)
		root_start_sect += fat_sectors;

	fsi->root_sectors = root_sectors;
	fsi->root_start_sect = root_start_sect;
	fsi->data_start_sect = root_start_sect + root_sectors;
	fsi->vol_id = get_unaligned_le32(bs->bpb.f16.vol_serial);
	fsi->total_sectors = get_unaligned_le16(bs->bpb.num_sectors);
	if (!fsi->total_sectors)
		fsi->total_sectors = le32_to_cpu(bs->bpb.num_huge_sectors);

	/* bogus value */
	if (fsi->total_sectors <= fsi->data_start_sect)
		return;

	/* check fat12 */
	num_clusters = (u32)((fsi->total_sectors - fsi->data_start_sect) >>
			    sect_per_clus_bits) + CLUS_BASE;
	if (num_clusters < FAT12_THRESHOLD)
		strscpy(fsi->type, "fat12", OEM_NAME_LEN);
}

static inline bool is_ntfs(struct pbr_t *pbr)
{
	if (!memcmp(pbr->bpb.fat.oem_name, OEM_NAME_NTFS, OEM_NAME_LEN))
		return true;
	return false;
}

static inline bool is_exfat(struct pbr_t *pbr)
{
	if (!memcmp(pbr->bpb.f64.oem_name, OEM_NAME_EXFAT, OEM_NAME_LEN))
		return true;
	return false;
}

static inline bool is_fat32(struct pbr_t *pbr)
{
	if (!le16_to_cpu(pbr->bpb.fat.num_fat_sectors))
		return true;
	return false;
}

void fs_common_stlog_bs(struct super_block *sb,	const char *prefix, void *bs)
{
	struct pbr_t *pbr = (struct pbr_t *)bs;
	struct fs_common_fsinfo	fsi = {0, };
	struct gendisk *disk = sb->s_bdev->bd_disk;
	u64 fs_kb;
	u64 bdev_kb;
	u32 root_kb;

	/* not supported yet */
	if (is_ntfs(pbr))
		return;

	if (is_exfat(pbr))
		__get_fsi_exfat(sb, pbr, &fsi);
	else if (is_fat32(pbr))
		__get_fsi_fat32(sb, pbr, &fsi);
	else
		__get_fsi_fat(sb, pbr, &fsi);

	fs_kb = (fsi.total_sectors * (sb->s_blocksize >> SECT_SIZE_BITS)) >> 1;
	bdev_kb = (u64)bdev_nr_sectors(sb->s_bdev) >> 1;

	fs_common_stlog(sb, prefix,
			"volume info  : %s (%04hX-%04hX, bps : %lu, spc : %u, data start : %llu, %s)",
			fsi.type,
			(fsi.vol_id >> 16) & 0xffff, fsi.vol_id & 0xffff,
			sb->s_blocksize, fsi.sect_per_clus,
			fsi.data_start_sect,
			(fsi.data_start_sect & (fsi.sect_per_clus - 1)) ?
			"misaligned" : "aligned");
	fs_common_stlog(sb, prefix,
			"volume size  : %llu KB (disk : %llu KB, part : %llu KB)",
			fs_kb, disk ? (u64)(get_capacity(disk) >> 1) : 0,
			bdev_kb);

	if (fsi.root_start_sect) {
		root_kb = (fsi.root_sectors *
			  (sb->s_blocksize >> SECT_SIZE_BITS)) >> 1;
		fs_common_stlog(sb, prefix,
				"rootdir info : start: %llu, size: %u KB",
				fsi.root_start_sect, root_kb);
	}

	if (fs_kb > bdev_kb) {
		fs_common_stlog(sb, prefix,
				"WRONG volume : beyond end of device");
	}
}
#endif
