#ifndef __SEC_QC_USER_RESET_H__
#define __SEC_QC_USER_RESET_H__

#include "sec_qc_user_reset_type.h"
#include "sec_qc_dbg_partition_type.h"

#if IS_ENABLED(CONFIG_SEC_QC_USER_RESET)
extern struct debug_reset_header *sec_qc_user_reset_get_reset_header(void);
extern int sec_qc_get_reset_write_cnt(void);
extern ap_health_t *sec_qc_ap_health_data_read(void);
extern int sec_qc_ap_health_data_write(ap_health_t *data);
extern int sec_qc_ap_health_data_write_delayed(ap_health_t *data);
extern unsigned int sec_qc_reset_reason_get(void);
extern const char *sec_qc_reset_reason_to_str(unsigned int reason);
#else
static inline struct debug_reset_header *sec_qc_user_reset_get_reset_header(void) { return NULL; }
static inline int sec_qc_get_reset_write_cnt(void) { return 0; }
static inline ap_health_t *sec_qc_ap_health_data_read(void) { return NULL; }
static inline int sec_qc_ap_health_data_write(ap_health_t *data) { return 0; }
static inline int sec_qc_ap_health_data_write_delayed(ap_health_t *data) { return 0; }
static inline unsigned int sec_qc_reset_reason_get(void) { return 0xFFEEFFEE; }
static inline const char *sec_qc_reset_reason_to_str(unsigned int reason) { return "NP"; }
#endif

/* TODO: data type which is not shared with BL */
struct kryo_arm64_edac_err_ctx {
	int cpu;
	int block;
	int etype;
};

enum {
	ID_L1_CACHE = 0,
	ID_L2_CACHE,
	ID_L3_CACHE,
	ID_BUS_ERR,
};

/* FIXME: KRYO_XX_XX are arleady defined in 'kryo_arm64_edac.c' */
#define __KRYO_L1_CE	0
#define __KRYO_L1_UE	1
#define __KRYO_L2_CE	2
#define __KRYO_L2_UE	3
#define __KRYO_L3_CE	4
#define __KRYO_L3_UE	5

static __always_inline int __qc_kryo_arm64_edac_update(ap_health_t *health,
		int cpu, int block, int etype)
{
	if (cpu < 0 || cpu >= num_present_cpus()) {
		pr_warn("not a valid cpu = %d\n", cpu);
		return -EINVAL;
	}

	switch (block) {
	case ID_L1_CACHE:
		if (etype == __KRYO_L1_CE) {
			health->cache.edac[cpu][block].ce_cnt++;
			health->daily_cache.edac[cpu][block].ce_cnt++;
		} else if (etype == __KRYO_L1_UE) {
			health->cache.edac[cpu][block].ue_cnt++;
			health->daily_cache.edac[cpu][block].ue_cnt++;
		}
		break;
	case ID_L2_CACHE:
		if (etype == __KRYO_L2_CE) {
			health->cache.edac[cpu][block].ce_cnt++;
			health->daily_cache.edac[cpu][block].ce_cnt++;
		} else if (etype == __KRYO_L2_UE) {
			health->cache.edac[cpu][block].ue_cnt++;
			health->daily_cache.edac[cpu][block].ue_cnt++;
		}
		break;
	case ID_L3_CACHE:
		if (etype == __KRYO_L3_CE) {
			health->cache.edac_l3.ce_cnt++;
			health->daily_cache.edac_l3.ce_cnt++;
		} else if (etype == __KRYO_L3_UE) {
			health->cache.edac_l3.ue_cnt++;
			health->daily_cache.edac_l3.ue_cnt++;
		}
		break;
	case ID_BUS_ERR:
		health->cache.edac_bus_cnt++;
		health->daily_cache.edac_bus_cnt++;
		break;
	}

	return 0;
}

static inline int sec_qc_kryo_arm64_edac_update(ap_health_t *health,
		const struct kryo_arm64_edac_err_ctx *ctx)
{
	if (!IS_ENABLED(CONFIG_SEC_QC_USER_RESET))
		return 0;

	return __qc_kryo_arm64_edac_update(health,
			ctx->cpu, ctx->block, ctx->etype);
}

#endif // __SEC_QC_USER_RESET_H__
