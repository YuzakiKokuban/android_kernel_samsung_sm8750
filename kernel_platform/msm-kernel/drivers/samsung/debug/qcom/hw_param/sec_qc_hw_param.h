#ifndef __INTERNAL__SEC_QC_HW_PARAM_H__
#define __INTERNAL__SEC_QC_HW_PARAM_H__

#include <linux/mutex.h>
#include <linux/proc_fs.h>

#include <linux/samsung/builder_pattern.h>

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/debug/qcom/sec_qc_user_reset.h>

#include "sec_qc_hw_param_mock.h"

#define PARAM0_IVALID		1
#define PARAM0_LESS_THAN_0	2

#define DEFAULT_LEN_STR         1023
#define SPECIAL_LEN_STR         2047
#define EXTRA_LEN_STR           ((SPECIAL_LEN_STR) - (31 + 5))

#define __qc_hw_param_scnprintf(buf, size, offset, fmt, ...) \
do { \
	if ((size) <= (offset)) \
		break; \
	offset += scnprintf(&(buf)[offset], size - (size_t)offset, fmt, \
			##__VA_ARGS__); \
} while (0)

struct qc_ap_info {
	struct device_node *np_hw_param;
	unsigned int system_rev;
};

struct qc_hw_param_drvdata {
	struct builder bd;
	ap_health_t *ap_health;
	struct device *sec_hw_param_dev;
	struct qc_ap_info ap_info;
	struct proc_dir_entry *errp_extra_proc;
	bool use_ddr_eye_info;
};

extern struct qc_hw_param_drvdata *qc_hw_param;

static __always_inline bool __qc_hw_param_is_probed(void)
{
	return !!qc_hw_param;
}

/* TODO: internal use only */
extern bool __qc_hw_param_is_valid_reset_reason(unsigned int reset_reason);
extern void __qc_hw_param_clean_format(char *buf, ssize_t *size, int max_len_str);
extern int __qc_hw_param_read_param0(const char *name);

/* sec_qc_ap_info.c */
extern struct device_attribute dev_attr_ap_info;
extern int __qc_ap_info_init(struct builder *bd);

/* sec_qc_ap_health.c */
extern struct device_attribute dev_attr_ap_health;

/* sec_qc_last_dcvs.c */
extern struct device_attribute dev_attr_last_dcvs;
extern int __qc_last_dcvs_init(struct builder *bd);

/* sec_qc_extra_info.c */
extern struct device_attribute dev_attr_extra_info;

/* sec_qc_extrb_info.c */
extern struct device_attribute dev_attr_extrb_info;

/* sec_qc_extrc_info.c */
extern struct device_attribute dev_attr_extrc_info;

/* sec_qc_extrt_info.c */
extern struct device_attribute dev_attr_extrt_info;

/* sec_qc_extrm_info.c */
extern struct device_attribute dev_attr_extrm_info;

/* sec_qc_errp_extra.c */
extern int __qc_errp_extra_init(struct builder *bd);
extern void __qc_errp_extra_exit(struct builder *bd);

/* sec_qc_ddr_info.c */
extern struct device_attribute dev_attr_ddr_info;
extern struct device_attribute dev_attr_eye_rd_info;
extern struct device_attribute dev_attr_eye_dcc_info;
extern struct device_attribute dev_attr_eye_wr1_info;
extern struct device_attribute dev_attr_eye_wr2_info;

#endif /* __INTERNAL__SEC_QC_HW_PARAM_H__ */
