#ifndef __INTERNAL__SEC_QC_SMEM_ID_VENDOR1_TYPE_H__
#define __INTERNAL__SEC_QC_SMEM_ID_VENDOR1_TYPE_H__

/* For SMEM_ID_VENDOR1 */
#define SMEM_VEN1_MAGIC 0x314E4556314E4556

typedef struct {
	uint64_t hw_rev;
} sec_smem_id_vendor1_v1_t;

typedef struct {
	uint64_t hw_rev;
	uint64_t ap_suspended;
} sec_smem_id_vendor1_v2_t;

#define NUM_CH 4
#define NUM_CS 2
#define NUM_DQ_PCH 2

typedef struct {
	uint8_t tDQSCK[NUM_CH][NUM_CS][NUM_DQ_PCH];
} ddr_rcw_t;

typedef struct {
	uint8_t coarse_cdc[NUM_CH][NUM_CS][NUM_DQ_PCH];
	uint8_t fine_cdc[NUM_CH][NUM_CS][NUM_DQ_PCH];
} ddr_wr_dqdqs_t;

typedef struct {
	/* WR */
	uint8_t wr_pr_width[NUM_CH][NUM_CS][NUM_DQ_PCH];
	uint8_t wr_min_eye_height[NUM_CH][NUM_CS];
	uint8_t wr_best_vref[NUM_CH][NUM_CS];
	uint8_t wr_vmax_to_vmid[NUM_CH][NUM_CS][NUM_DQ_PCH];
	uint8_t wr_vmid_to_vmin[NUM_CH][NUM_CS][NUM_DQ_PCH];
	/* RD */
	uint8_t rd_pr_width[NUM_CH][NUM_CS][NUM_DQ_PCH];
	uint8_t rd_min_eye_height[NUM_CH][NUM_CS];
	uint8_t rd_best_vref[NUM_CH][NUM_CS][NUM_DQ_PCH];
	/* DCC */
	uint8_t dq_dcc_abs[NUM_CH][NUM_CS][NUM_DQ_PCH];
	uint8_t dqs_dcc_adj[NUM_CH][NUM_DQ_PCH];

	uint16_t small_eye_detected;
} ddr_dqdqs_eye_t;

typedef struct {
	uint32_t version;
	ddr_rcw_t rcw;
	ddr_wr_dqdqs_t wr_dqdqs;
	ddr_dqdqs_eye_t dqdqs_eye;
} ddr_train_t;

typedef struct {
	uint64_t num;
	void * nIndex __attribute__((aligned(8)));
	void * DDRLogs __attribute__((aligned(8)));
	void * DDR_STRUCT __attribute__((aligned(8)));
} smem_ddr_stat_t;

typedef struct {
	sec_smem_header_t header;
	sec_smem_id_vendor1_v2_t ven1_v2;
	smem_ddr_stat_t ddr_stat;
} sec_smem_id_vendor1_v3_t;

typedef struct {
	void *clk __attribute__((aligned(8)));
	void *apc_cpr __attribute__((aligned(8)));
} smem_apps_stat_t;

typedef struct {
	void *vreg __attribute__((aligned(8)));
} smem_vreg_stat_t;

typedef struct {
	sec_smem_header_t header;
	sec_smem_id_vendor1_v2_t ven1_v2;
	smem_ddr_stat_t ddr_stat;
	void * ap_health __attribute__((aligned(8)));
	smem_apps_stat_t apps_stat;
	smem_vreg_stat_t vreg_stat;
	ddr_train_t ddr_training;
} sec_smem_id_vendor1_v4_t;

typedef struct {
	sec_smem_header_t header;
	sec_smem_id_vendor1_v2_t ven1_v2;
	uint8_t cover_n;
	smem_ddr_stat_t ddr_stat;
	void * ap_health __attribute__((aligned(8)));
	smem_apps_stat_t apps_stat;
	smem_vreg_stat_t vreg_stat;
	ddr_train_t ddr_training;
} sec_smem_id_vendor1_v5_t;

typedef struct {
	uint32_t pcba_config; // id_code
} huaqin_t;

typedef union {
	huaqin_t hq;
	uint32_t reserve[8];
} odm_t;

typedef struct {
	odm_t odm;
	sec_smem_header_t header;
	sec_smem_id_vendor1_v2_t ven1_v2;
	uint8_t cover_n;
	smem_ddr_stat_t ddr_stat;
	void * ap_health __attribute__((aligned(8)));
	smem_apps_stat_t apps_stat;
	smem_vreg_stat_t vreg_stat;
	ddr_train_t ddr_training;
} sec_smem_id_vendor1_v6_t;

typedef struct {
	uint8_t hw_rev[4]; /* 0 : main, 1 : sub, 2 : slave */
	uint8_t reserved1[4];
	uint8_t ap_suspended;
	uint8_t reserved2[7];
	uint8_t cover_n;
	uint8_t reserved3[3];
} sec_smem_id_vendor1_share_t; /* sharing with other subsystem */

typedef struct {
	sec_smem_header_t header;
	sec_smem_id_vendor1_share_t share;
	smem_ddr_stat_t ddr_stat;
	void * ap_health __attribute__((aligned(8)));
	smem_apps_stat_t apps_stat;
	smem_vreg_stat_t vreg_stat;
	ddr_train_t ddr_training;
} sec_smem_id_vendor1_v7_t;

#endif /* __INTERNAL__SEC_QC_SMEM_ID_VENDOR1_TYPE_H__ */
