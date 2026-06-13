#ifndef __LEGACY__SEC_DEBUG_H__
#define __LEGACY__SEC_DEBUG_H__

#include <linux/samsung/debug/sec_debug_region.h>
#include <linux/samsung/debug/sec_debug.h>
#include <linux/samsung/debug/sec_reboot_cmd.h>
#include <linux/samsung/debug/sec_crashkey_long.h>
#include <linux/samsung/debug/sec_force_err.h>
#include <linux/samsung/debug/sec_crashkey.h>
#include <linux/samsung/debug/sec_arm64_ap_context.h>
#include <linux/samsung/debug/sec_log_buf.h>
#include <linux/samsung/debug/sec_upload_cause.h>

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>

static __always_inline bool read_debug_partition(size_t index, void *value)
{
	return sec_qc_dbg_part_read(index, value);
}

static __always_inline bool write_debug_partition(size_t index, void *value)
{
	return sec_qc_dbg_part_write(index, value);
}

#include <linux/samsung/debug/qcom/sec_qc_hw_param.h>

#endif /* __LEGACY__SEC_DEBUG_H__ */
