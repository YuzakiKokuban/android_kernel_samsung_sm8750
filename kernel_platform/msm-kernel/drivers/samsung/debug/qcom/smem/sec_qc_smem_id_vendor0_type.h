#ifndef __INTERNAL__SEC_QC_SMEM_ID_VENDOR0_TYPE_H__
#define __INTERNAL__SEC_QC_SMEM_ID_VENDOR0_TYPE_H__

/* For SMEM_ID_VENDOR0 */
#define SMEM_VEN0_MAGIC		0x304E4556304E4556

#define DDR_IFNO_REVISION_ID1		8
#define DDR_IFNO_REVISION_ID2		16
#define DDR_IFNO_TOTAL_DENSITY		24

typedef struct{
	uint64_t afe_rx_good;
	uint64_t afe_rx_mute;
	uint64_t afe_tx_good;
	uint64_t afe_tx_mute;
} smem_afe_log_t;

typedef struct{
	uint64_t reserved[5];
} smem_afe_ext_t;

typedef struct {
	uint32_t ddr_vendor;
	uint32_t reserved;
	smem_afe_log_t afe_log;
} sec_smem_id_vendor0_v1_t;

typedef struct {
	sec_smem_header_t header;
	uint32_t ddr_vendor;
	uint32_t reserved;
	smem_afe_log_t afe_log;
	smem_afe_ext_t afe_ext;
} sec_smem_id_vendor0_v2_t;

#endif /* __INTERNAL__SEC_QC_SMEM_ID_VENDOR0_TYPE_H__ */