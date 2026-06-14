// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Specific feature
 *
 * Copyright (C) 2023 Samsung Electronics Co., Ltd.
 *
 * Authors:
 *	Storage Driver <storage.sec@samsung.com>
 */

#ifndef __UFS_SEC_FEATURE_H__
#define __UFS_SEC_FEATURE_H__

#include "../core/ufshcd-priv.h"
#include <ufs/ufshci.h>
#include <linux/sched/clock.h>
#include <linux/notifier.h>

/* unique number */
#define UFS_UN_20_DIGITS 20
#define UFS_UN_MAX_DIGITS (UFS_UN_20_DIGITS + 1)
#define SERIAL_NUM_SIZE 7

#define SCSI_UFS_TIMEOUT (10 * HZ)

#define UFS_S_INFO_SIZE 512
#define UFS_SHI_SIZE 256

#define HEALTH_DESC_PARAM_SEC_FLT 0x22
#define HEALTH_DESC_PARAM_KIC_FLT 0x11
#define HEALTH_DESC_PARAM_MIC_FLT 0x5
#define HEALTH_DESC_PARAM_SKH_FLT 0x5

struct ufs_vendor_dev_info {
	struct ufs_hba *hba;
	char unique_number[UFS_UN_MAX_DIGITS];
	u8 lt;
	u16 flt;
	u8 eli;
	unsigned int ic;
	char s_info[UFS_S_INFO_SIZE];
	char shi[UFS_SHI_SIZE];
	bool device_stuck;
};

struct ufs_sec_cmd_info {
	u8 opcode;
	u32 lba;
	int transfer_len;
	u8 lun;
};

enum ufs_sec_wb_state {
	WB_OFF = 0,
	WB_ON
};

struct ufs_sec_wb_info {
	bool support;
	u64 state_ts;
	u64 enable_ms;
	u64 disable_ms;
	u64 amount_kb;
	u64 enable_cnt;
	u64 disable_cnt;
	u64 err_cnt;
};

enum ufs_sec_hcgc_op {
	HCGC_OP_nop = 0,
	HCGC_OP_stop,
	HCGC_OP_analyze,
	HCGC_OP_execute,
	HCGC_OP_max,
};

/* HCGC : vendor specific flag_idn */
enum {
	QUERY_FLAG_IDN_SEC_HCGC_ANALYSYS = 0x13,
	QUERY_FLAG_IDN_SEC_HCGC_EXECUTE = 0x14,
};

/* HCGC : vendor specific attr_idn */
enum {
	QUERY_ATTR_IDN_SEC_HCGC_STATE = 0xF0,	// bHCGCState, bHCGCProgressStatus
	QUERY_ATTR_IDN_SEC_HCGC_SIZE = 0xFA,  // wHCGCSize
	QUERY_ATTR_IDN_SEC_HCGC_AVAIL_SIZE = 0xFC,  // wHCGCAvailSize, bHCGCFreeBlockMaxSize
	QUERY_ATTR_IDN_SEC_HCGC_RATIO = 0xFE,  // bHCGCRatio, bHCGCFreeBlockLevel
	QUERY_ATTR_IDN_SEC_HCGC_OPERATION = 0xFF, // wHCGCOperation
};

/* HCGC : vendor specific desc_idn */
enum {
	QUERY_DESC_IDN_VENDOR_DEVICE = 0xF0
};

/* HCGC : vendor specific device_desc_param */
enum {
	DEVICE_DESC_PARAM_VENDOR_FEA_SUP = 0xFB
};

/* HCGC : Possible values for dExtendedUFSFeaturesSupport */
enum {
	UFS_SEC_EXT_HCGC_SUPPORT = BIT(10),
};

/* HCGC : Possible values for dVendorSpecificFeaturesSupport */
enum {
	UFS_VENDOR_DEV_HCGC = BIT(0),
	UFS_VENDOR_DEV_STREAMID = BIT(4),
	UFS_VENDOR_DEV_PSA = BIT(5),
};

enum ufs_sec_hcgc_status {
	HCGC_STATE_need_to_analyze = 0,
	HCGC_STATE_analyzing,
	HCGC_STATE_need_to_execute,
	HCGC_STATE_executing,
	HCGC_STATE_done,
	HCGC_STATE_max,
};

struct ufs_sec_hcgc_info {
	bool support;		/* UFS : feature support */
	bool allow;		/* Host : feature allow */
	int disable_threshold_lt;	/* LT threshold that HCGC is not allowed */

	u32 bHCGCState;
	u32 wHCGCAvailSize;
	u32 wHCGCSize;
	u32 bHCGCRatio;
	u32 bHCGCOperation;

	atomic_t hcgc_op_cnt[HCGC_OP_max];	/* HCGC op count */
	atomic_t hcgc_op_err_cnt[HCGC_OP_max];	/* HCGC op error count */
};

enum ufs_sec_log_str_t {
	UFS_SEC_CMD_SEND,
	UFS_SEC_CMD_COMP,
	UFS_SEC_QUERY_SEND,
	UFS_SEC_QUERY_COMP,
	UFS_SEC_NOP_SEND,
	UFS_SEC_NOP_COMP,
	UFS_SEC_TM_SEND,
	UFS_SEC_TM_COMP,
	UFS_SEC_TM_ERR,
	UFS_SEC_UIC_SEND,
	UFS_SEC_UIC_COMP,
};

static const char * const ufs_sec_log_str[] = {
	[UFS_SEC_CMD_SEND] = "scsi_send",
	[UFS_SEC_CMD_COMP] = "scsi_cmpl",
	[UFS_SEC_QUERY_SEND] = "query_send",
	[UFS_SEC_QUERY_COMP] = "query_cmpl",
	[UFS_SEC_NOP_SEND] = "nop_send",
	[UFS_SEC_NOP_COMP] = "nop_cmpl",
	[UFS_SEC_TM_SEND] = "tm_send",
	[UFS_SEC_TM_COMP] = "tm_cmpl",
	[UFS_SEC_TM_ERR] = "tm_err",
	[UFS_SEC_UIC_SEND] = "uic_send",
	[UFS_SEC_UIC_COMP] = "uic_cmpl",
};

struct ufs_sec_cmd_log_entry {
	const char *str;	/* ufs_sec_log_str */
	u8 lun;
	u8 cmd_id;
	u32 lba;
	int transfer_len;
	u8 idn;		/* used only for query idn */
	unsigned long outstanding_reqs;
	unsigned int tag;
	u64 tstamp;
};

#define UFS_SEC_CMD_LOGGING_MAX 200
#define UFS_SEC_CMD_LOGNODE_MAX 64
struct ufs_sec_cmd_log_info {
	struct ufs_sec_cmd_log_entry *entries;
	int pos;
};

struct ufs_sec_feature_info {
	struct ufs_vendor_dev_info *vdi;

	struct ufs_sec_wb_info *ufs_wb;
	struct ufs_sec_wb_info *ufs_wb_backup;

	struct ufs_sec_hcgc_info *ufs_hcgc;

	struct ufs_sec_err_info *ufs_err;
	struct ufs_sec_err_info *ufs_err_backup;
	struct ufs_sec_err_info *ufs_err_hist;

	struct ufs_sec_cmd_log_info *ufs_cmd_log;

	struct notifier_block reboot_notify;
	struct delayed_work noti_work;

	u32 ext_ufs_feature_sup;
	u32 vendor_spec_feature_sup;

	u32 last_ucmd;
	bool ucmd_complete;

	enum query_opcode last_qcmd;
	enum dev_cmd_type qcmd_type;
	bool qcmd_complete;
};

extern struct device *sec_ufs_node_dev;

void ufs_sec_get_health_desc(struct ufs_hba *hba);

inline bool ufs_sec_is_err_cnt_allowed(void);

/* call by vendor module */
void ufs_sec_config_features(struct ufs_hba *hba);
void ufs_sec_adjust_caps_quirks(struct ufs_hba *hba);
void ufs_sec_init_logging(struct device *dev);
void ufs_sec_set_features(struct ufs_hba *hba);
void ufs_sec_remove_features(struct ufs_hba *hba);
void ufs_sec_register_vendor_hooks(void);

void ufs_sec_check_device_stuck(void);

void ufs_sec_inc_hwrst_cnt(void);
void ufs_sec_inc_op_err(struct ufs_hba *hba, enum ufs_event_type evt, void *data);
void ufs_sec_print_err(void);

/* SEC next WB */
bool ufs_sec_is_wb_supported(void);
int ufs_sec_wb_ctrl(bool enable);
void ufs_sec_wb_register_reset_notify(void *func);

/* SEC HCGC */
bool ufs_sec_is_hcgc_allowed(void);
int ufs_sec_hcgc_query_attr(struct ufs_hba *hba,
		enum query_opcode opcode, enum attr_idn idn, u32 *val);
#endif
