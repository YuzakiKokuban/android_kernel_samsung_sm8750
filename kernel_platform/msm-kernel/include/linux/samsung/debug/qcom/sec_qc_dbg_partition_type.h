#ifndef __SEC_QC_DBG_PARTITION_TYPE_H__
#define __SEC_QC_DBG_PARTITION_TYPE_H__

#include "sec_qc_user_reset_type.h"

enum debug_partition_index {
	debug_index_reset_summary_info = 0,
	debug_index_reset_header = 0,
	debug_index_reset_ex_info,
	debug_index_ap_health,
	debug_index_lcd_debug_info,
	debug_index_reset_history,
	debug_index_reserve_0,
	debug_index_reserve_1,
	debug_index_onoff_history,
	debug_index_reset_tzlog,
	debug_index_reset_extrc_info,
	debug_index_auto_comment,
	debug_index_reset_rkplog,
	debug_index_modem_info,
	debug_index_reset_klog,
	debug_index_reset_lpm_klog,
	debug_index_reset_summary,
	debug_index_reset_last_kmsg,
	debug_index_max,
};

#define DEBUG_PART_MAX_TABLE (debug_index_max)

#define AP_HEALTH_MAGIC		0x48544C4145485041
#define AP_HEALTH_VER		2
#define MAX_PCIE_NUM		3

typedef struct {
	uint64_t magic;
	uint32_t size;
	uint16_t version;
	uint16_t need_write;
} ap_health_header_t;

struct edac_cnt {
	uint32_t ue_cnt;
	uint32_t ce_cnt;
};

typedef struct {
	struct edac_cnt edac[CONFIG_SEC_QC_NR_CPUS][2]; // L1, L2
	struct edac_cnt edac_l3; // L3
	uint32_t edac_bus_cnt;
	struct edac_cnt edac_llcc_data_ram; // LLCC Data RAM
	struct edac_cnt edac_llcc_tag_ram; // LLCC Tag RAM
} cache_health_t;

typedef struct {
	uint32_t phy_init_fail_cnt;
	uint32_t link_down_cnt;
	uint32_t link_up_fail_cnt;
	uint32_t link_up_fail_ltssm;
} pcie_health_t;

typedef struct {
	uint32_t np;
	uint32_t rp;
	uint32_t mp;
	uint32_t kp;
	uint32_t dp;
	uint32_t wp;
	uint32_t tp;
	uint32_t sp;
	uint32_t pp;
	uint32_t cp;
} reset_reason_t;

enum {
	L3,
	PWR_CLUSTER,
	PERF_CLUSTER,
	PRIME_CLUSTER,
};

#define MAX_CLUSTER_NUM 4
#define CPU_NUM_PER_CLUSTER 4
#define MAX_VREG_CNT 3
#define MAX_BATT_DCVS 10

typedef struct {
	uint32_t cpu_KHz;
	uint32_t reserved;
} apps_dcvs_t;

typedef struct {
	uint32_t ddr_KHz;
	uint16_t mV[MAX_VREG_CNT];
} rpm_dcvs_t;

typedef struct {
	uint64_t ktime;
	int32_t cap;
	int32_t volt;
	int32_t temp;
	int32_t curr;
} batt_dcvs_t;

typedef struct {
	uint32_t tail;
	batt_dcvs_t batt[MAX_BATT_DCVS];
} battery_health_t;

typedef struct {
	uint64_t pon_reason;
	uint64_t fault_reason;
} pon_dcvs_t;

typedef struct {
	apps_dcvs_t apps[MAX_CLUSTER_NUM];
	rpm_dcvs_t rpm;
	batt_dcvs_t batt;
	pon_dcvs_t pon;
} dcvs_info_t;

typedef struct {
	ap_health_header_t header;
	uint32_t last_rst_reason;
	dcvs_info_t last_dcvs;
	uint64_t spare_magic1;
	reset_reason_t rr;
	cache_health_t cache;
	pcie_health_t pcie[MAX_PCIE_NUM];
	battery_health_t battery;
	uint64_t spare_magic2;
	reset_reason_t daily_rr;
	cache_health_t daily_cache;
	pcie_health_t daily_pcie[MAX_PCIE_NUM];
	uint64_t spare_magic3;
} ap_health_t;

/* Synchronize with Fence Name Length */
#define MAX_FTOUT_NAME 128

struct lcd_debug_ftout {
	uint32_t count;
	char name[MAX_FTOUT_NAME];
};

#define FW_UP_MAX_RETRY 20
struct lcd_debug_fw_up {
	uint32_t try_count;
	uint32_t pass_count;
	uint32_t fail_line_count;
	uint32_t fail_line[FW_UP_MAX_RETRY];
	uint32_t fail_count;
	uint32_t fail_address[FW_UP_MAX_RETRY];
};

struct lcd_debug_t {
	struct lcd_debug_ftout ftout;
	struct lcd_debug_fw_up fw_up;
};

#define SEC_DEBUG_ONOFF_HISTORY_MAX_CNT		20
#define SEC_DEBUG_ONOFF_REASON_STR_SIZE		128

typedef struct debug_onoff_reason {
	int64_t rtc_offset;
	int64_t local_time;
	uint32_t boot_cnt;
	char reason[SEC_DEBUG_ONOFF_REASON_STR_SIZE];
} onoff_reason_t;

typedef struct debug_onoff_history {
	uint32_t magic;
	uint32_t size;
	uint32_t index;
	onoff_reason_t history[SEC_DEBUG_ONOFF_HISTORY_MAX_CNT];
} onoff_history_t;

#define DEBUG_PARTITION_MAGIC	0x41114729
#define SECTOR_UNIT_SIZE			(4096) /* UFS */
#define SEC_DEBUG_PARTITION_SIZE                (0xA00000)              /* 10MB */
#define SEC_DEBUG_RESET_EXTRC_SIZE		(2 * 1024)		/* 2KB */
#define SEC_DEBUG_AUTO_COMMENT_SIZE		(0x01000)		/* 4KB */
#define SEC_DEBUG_RESET_HISTORY_MAX_CNT		(10)
#define SEC_DEBUG_RESET_HISTORY_SIZE            (SEC_DEBUG_AUTO_COMMENT_SIZE*SEC_DEBUG_RESET_HISTORY_MAX_CNT)
#define SEC_DEBUG_RESET_ETRM_SIZE		(0x3c0)
#define SEC_DEBUG_RESET_KLOG_SIZE		(0x200000)		/* 2MB */
#define SEC_DEBUG_RESET_LPM_KLOG_SIZE		(0x200000)		/* 2MB */
#define SEC_DEBUG_RESET_LAST_KMSG_SIZE		(0x200000)		/* 2MB */

#endif /* __SEC_QC_DBG_PARTITION_TYPE_H__ */
