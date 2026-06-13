#ifndef __SEC_QC_SMEM_LPDDR_INDIRECT
#warning "samsung/debug/qcom/sec_qc_smem_lpddr.h is included directly."
#error "please include sec_qc_smem.h instead of this file"
#endif

#ifndef __INDIRECT__SEC_QC_SMEM_LPDDR_H__
#define __INDIRECT__SEC_QC_SMEM_LPDDR_H__

#include <linux/err.h>
#include <linux/errno.h>

#define NUM_CH		4
#define NUM_CS		2
#define NUM_DQ_PCH	2

#if IS_ENABLED(CONFIG_SEC_QC_SMEM)
extern uint8_t sec_qc_smem_lpddr_get_revision_id1(void);
extern uint8_t sec_qc_smem_lpddr_get_revision_id2(void);
extern uint8_t sec_qc_smem_lpddr_get_total_density(void);
extern const char *sec_qc_smem_lpddr_get_vendor_name(void);
extern uint32_t sec_qc_smem_lpddr_get_DSF_version(void);
extern uint8_t sec_qc_smem_lpddr_get_rcw_tDQSCK(size_t ch, size_t cs, size_t dq);
extern uint8_t sec_qc_smem_lpddr_get_wr_coarseCDC(size_t ch, size_t cs, size_t dq);
extern uint8_t sec_qc_smem_lpddr_get_wr_fineCDC(size_t ch, size_t cs, size_t dq);
extern uint8_t sec_qc_smem_lpddr_get_wr_pr_width(size_t ch, size_t cs, size_t dq);
extern uint8_t sec_qc_smem_lpddr_get_wr_min_eye_height(size_t ch, size_t cs);
extern uint8_t sec_qc_smem_lpddr_get_wr_best_vref(size_t ch, size_t cs);
extern uint8_t sec_qc_smem_lpddr_get_wr_vmax_to_vmid(size_t ch, size_t cs, size_t dq);
extern uint8_t sec_qc_smem_lpddr_get_wr_vmid_to_vmin(size_t ch, size_t cs, size_t dq);
extern uint8_t sec_qc_smem_lpddr_get_dqs_dcc_adj(size_t ch, size_t dq);
extern uint8_t sec_qc_smem_lpddr_get_rd_pr_width(size_t ch, size_t cs, size_t dq);
extern uint8_t sec_qc_smem_lpddr_get_rd_min_eye_height(size_t ch, size_t cs);
extern uint8_t sec_qc_smem_lpddr_get_rd_best_vref(size_t ch, size_t cs, size_t dq);
extern uint8_t sec_qc_smem_lpddr_get_dq_dcc_abs(size_t ch, size_t cs, size_t dq);
extern uint8_t sec_qc_smem_lpddr_get_small_eye_detected(void);
#else
static inline uint8_t sec_qc_smem_lpddr_get_revision_id1(void) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_revision_id2(void) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_total_density(void) { return -ENODEV; }
static inline const char *sec_qc_smem_lpddr_get_vendor_name(void) { return ERR_PTR(-ENODEV); }
static inline uint32_t sec_qc_smem_lpddr_get_DSF_version(void) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_rcw_tDQSCK(size_t ch, size_t cs, size_t dq) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_wr_coarseCDC(size_t ch, size_t cs, size_t dq) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_wr_fineCDC(size_t ch, size_t cs, size_t dq) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_wr_pr_width(size_t ch, size_t cs, size_t dq) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_wr_min_eye_height(size_t ch, size_t cs) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_wr_best_vref(size_t ch, size_t cs) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_wr_vmax_to_vmid(size_t ch, size_t cs, size_t dq) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_wr_vmid_to_vmin(size_t ch, size_t cs, size_t dq) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_dqs_dcc_adj(size_t ch, size_t dq) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_rd_pr_width(size_t ch, size_t cs, size_t dq) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_rd_min_eye_height(size_t ch, size_t cs) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_rd_best_vref(size_t ch, size_t cs, size_t dq) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_dq_dcc_abs(size_t ch, size_t cs, size_t dq) { return -ENODEV; }
static inline uint8_t sec_qc_smem_lpddr_get_small_eye_detected(void) { return -ENODEV; }
#endif

#endif /* __INDIRECT__SEC_QC_SMEM_LPDDR_H__ */
