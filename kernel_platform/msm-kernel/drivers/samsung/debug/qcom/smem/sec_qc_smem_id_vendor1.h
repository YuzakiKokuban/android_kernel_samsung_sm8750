#ifndef __INTERNAL__SEC_QC_SMEM_ID_VENDOR1_H__
#define __INTERNAL__SEC_QC_SMEM_ID_VENDOR1_H__

#include "sec_qc_smem_id_vendor1_type.h"

struct qc_smem_drvdata;

struct vendor1_operations {
	sec_smem_header_t *(*header)(void *vendor1);
	sec_smem_id_vendor1_v2_t *(*ven1_v2)(void *vendor1);
	sec_smem_id_vendor1_share_t *(*share)(void *vendor1);
	smem_ddr_stat_t *(*ddr_stat)(void *vendor1);
	void **(*ap_health)(void *vendor1);
	smem_apps_stat_t *(*apps_stat)(void *vendor1);
	smem_vreg_stat_t *(*vreg_stat)(void *vendor1);
	ddr_train_t *(*ddr_training)(void *vendor1);
};

/* Concrete creators for vendor1_ops */
extern const struct vendor1_operations *__qc_smem_vendor1_v5_ops_creator(void);
extern const struct vendor1_operations *__qc_smem_vendor1_v6_ops_creator(void);
extern const struct vendor1_operations *__qc_smem_vendor1_v7_ops_creator(void);

extern sec_smem_header_t *__qc_smem_id_vendor1_get_header(struct qc_smem_drvdata *drvdata);
extern sec_smem_id_vendor1_v2_t *__qc_smem_id_vendor1_get_ven1_v2(struct qc_smem_drvdata *drvdata);
extern sec_smem_id_vendor1_share_t *__qc_smem_id_vendor1_get_share(struct qc_smem_drvdata *drvdata);
extern smem_ddr_stat_t *__qc_smem_id_vendor1_get_ddr_stat(struct qc_smem_drvdata *drvdata);
extern void **__qc_smem_id_vendor1_get_ap_health(struct qc_smem_drvdata *drvdata);
extern smem_apps_stat_t *__qc_smem_id_vendor1_get_apps_stat(struct qc_smem_drvdata *drvdata);
extern smem_vreg_stat_t *__qc_smem_id_vendor1_get_vreg_stat(struct qc_smem_drvdata *drvdata);
extern ddr_train_t *__qc_smem_id_vendor1_get_ddr_training(struct qc_smem_drvdata *drvdata);

extern int __qc_smem_id_vendor1_init(struct builder *bd);

#endif /* __INTERNAL__SEC_QC_SMEM_ID_VENDOR1_H__ */