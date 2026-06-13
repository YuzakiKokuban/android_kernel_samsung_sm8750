#ifndef __SEC_SYSUP_H__
#define __SEC_SYSUP_H__

#define EDTBO_FIEMAP_MAGIC		0x00007763

struct fiemap_extent_p {
	unsigned long fe_logical;	/* logical offset in bytes for the start of the extent from the beginning of the file */
	unsigned long fe_physical;	/* physical offset in bytes for the start of the extent from the beginning of the disk */
	unsigned long fe_length;	/* length in bytes for this extent */
	unsigned long fe_reserved64[2];
	unsigned int fe_flags;		/* FIEMAP_EXTENT_* flags for this extent */
	unsigned int fe_reserved[3];
};

struct fiemap_p {
	unsigned long fm_start;		/* logical offset (inclusive) at which to start mapping (in) */
	unsigned long fm_length;	/* logical length of mapping which userspace wants (in) */
	unsigned int fm_flags;		/* FIEMAP_FLAG_* flags for request (in/out) */
	unsigned int fm_mapped_extents;	/* number of extents that were mapped (out) */
	unsigned int fm_extent_count;	/* size of fm_extents array (in) */
	unsigned int fm_reserved;
	struct fiemap_extent_p fm_extents[128];	/* array of mapped extents (out) */
};

#endif /* __SEC_SYSUP_H__ */
