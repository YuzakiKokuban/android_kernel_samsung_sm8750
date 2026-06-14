#ifndef __INTERNAL__SEC_QC_SMEM_H__
#define __INTERNAL__SEC_QC_SMEM_H__

#include <linux/notifier.h>

#include <linux/samsung/builder_pattern.h>

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/debug/qcom/sec_qc_user_reset.h>

typedef struct {
	uint64_t magic;
	uint64_t version;
} sec_smem_header_t;

#include "sec_qc_smem_external.h"
#include "sec_qc_smem_id_vendor0.h"
#include "sec_qc_smem_id_vendor1.h"

typedef struct {
	uint64_t ktime;
	uint64_t qtime;
	uint64_t rate;
} apps_clk_log_t;

#define MAX_CLK_LOG_CNT		10

typedef struct {
	uint32_t max_cnt;
	uint32_t index;
	apps_clk_log_t log[MAX_CLK_LOG_CNT];
} cpuclk_log_t;

#define MAX_CLUSTER_NUM		4

struct qc_smem_drvdata {
	struct builder bd;
	size_t smem_offset_vendor0;
	size_t smem_offset_vendor1;
	sec_smem_id_vendor0_v2_t *vendor0;
	unsigned int vendor0_ver;
	union {
		void *vendor1;
		/* TODO: debugging purpose only.
		 * Do NOT access below variables in the driver code.
		 * 'vendor1' can only be accessed by 'vendor1_ops' to prevent
		 * code smells like 'Change preventers'.
		 */
		const sec_smem_id_vendor1_v5_t * const __vendor1_v5;
		const sec_smem_id_vendor1_v6_t * const __vendor1_v6;
		const sec_smem_id_vendor1_v7_t * const __vendor1_v7;
	};
	unsigned int vendor1_ver;
	const struct vendor1_operations *vendor1_ops;
	ap_health_t *ap_health;
	cpuclk_log_t cpuclk_log[MAX_CLUSTER_NUM];
	struct notifier_block nb_cpuclk_log;
	struct notifier_block nb_l3clk_log;
};

extern struct qc_smem_drvdata *qc_smem;

static __always_inline bool __qc_smem_is_probed(void)
{
	return !!qc_smem;
}

/* sec_qc_smem_logger.c */
extern int __qc_smem_clk_osm_probe(struct builder *bd);
extern int __qc_smem_register_nb_cpuclk_log(struct builder *bd);
extern void __qc_smem_unregister_nb_cpuclk_log(struct builder *bd);
extern int __qc_smem_register_nb_l3clk_log(struct builder *bd);
extern void __qc_smem_unregister_nb_l3clk_log(struct builder *bd);

#endif /* __INTERNAL__SEC_QC_SMEM_H__ */
