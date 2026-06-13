// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2021-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/rtc.h>

#include <linux/samsung/bsp/sec_param.h>
#include <linux/samsung/debug/sec_log_buf.h>
#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>

#include "sec_qc_debug.h"

static inline void ____qc_reboot_recovery(const char *cmd)
{
	char recovery_cause[256] = { '\0', };
	bool valid;

	if (strcmp(cmd, "recovery"))
		return;

	valid = sec_param_get(param_index_reboot_recovery_cause,
			recovery_cause);
	if (!valid)
		pr_warn("failed to get param (idx:%u)\n",
				param_index_reboot_recovery_cause);

	/* if already occupied, then skip */
	if (recovery_cause[0] || strlen(recovery_cause))
		return;

	snprintf(recovery_cause, sizeof(recovery_cause),
			"%s:%d ", current->comm, task_pid_nr(current));

	valid = sec_param_set(param_index_reboot_recovery_cause,
				      recovery_cause);
	if (!valid)
		pr_warn("failed to set param (idx:%u)\n",
				param_index_reboot_recovery_cause);
}

static inline void __qc_reboot_recovery(struct qc_debug_drvdata *drvdata,
		unsigned long action, void *data)
{
	if (!data || (action != SYS_RESTART))
		return;

	____qc_reboot_recovery((const char *)data);
}

static inline void ____qc_reboot_param(const char *cmd)
{
	char __param_cmd[256];
	char *param_cmd = __param_cmd;
	const char *param_str, *index_str, *data_str;
	const char *delim = "_";
	unsigned int index;
	bool valid = false;

	if (strncmp(cmd, "param", strlen("param")))
		return;

	strlcpy(__param_cmd, cmd, sizeof(__param_cmd));

	param_str = strsep(&param_cmd, delim);
	if (!param_str) {
		pr_warn("can't extract a token for 'param'!\n");
		return;
	}

	index_str = strsep(&param_cmd, delim);
	if (!index_str) {
		pr_warn("can't extract a token for 'index'!\n");
		return;
	}

	if (kstrtouint(index_str, 0, &index)) {
		pr_warn("malfomred 'param' cmd (cmd:%s)\n", cmd);
		return;
	}

	data_str = strsep(&param_cmd, delim);
	if (!data_str) {
		pr_warn("malfomred 'param' cmd (cmd:%s)\n", cmd);
		valid = false;
	} else if (data_str[0] == '0') {
		unsigned long value;

		if (!kstrtoul(&data_str[1], 0, &value))
			valid = sec_param_set(index, &value);
	} else if (data_str[0] == '1') {
		valid = sec_param_set(index, &data_str[1]);
	}

	if (!valid)
		pr_warn("failed to set param (idx:%u, cmd:%s)\n",
				index, cmd);
}

static inline void __qc_reboot_param(struct qc_debug_drvdata *drvdata,
		unsigned long action, void *data)
{

	if (!data || (action != SYS_RESTART))
		return;

	____qc_reboot_param((const char *)data);
}

static inline void ____qc_reboot_store_last_kmsg(
		struct qc_debug_drvdata *drvdata,
		unsigned long action, const char *cmd)
{
	struct device *dev = drvdata->bd.dev;
	const struct sec_log_buf_head *s_log_buf;
	bool valid;

	s_log_buf = sec_log_buf_get_header();
	if (IS_ERR(s_log_buf)) {
		dev_info(dev, "sec_log_buf is not initialized.\n");
		return;
	}

	dev_info(dev, "%s, %s\n",
			action == SYS_RESTART ? "reboot" : "power off",
			cmd);

	valid = sec_qc_dbg_part_write(debug_index_reset_last_kmsg, s_log_buf);
	if (!valid)
		dev_warn(dev, "failed to write sec log buf to dbg partition\n");
}

static inline void __qc_reboot_store_last_kmsg(
		struct qc_debug_drvdata *drvdata,
		unsigned long action, void *data)
{
	if (action != SYS_RESTART && action != SYS_POWER_OFF)
		return;

	____qc_reboot_store_last_kmsg(drvdata, action, data);
}

static inline time64_t __qc_reboot_get_rtc_offset(void)
{
	struct rtc_time tm;
	struct rtc_device *rtc;

	rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);
	if (!rtc)
		return -ENODEV;

	if (rtc_read_time(rtc, &tm)) {
		dev_warn(rtc->dev.parent,
				"hctosys: unable to read the hardware clock\n");
		return -ENXIO;
	}

	return rtc_tm_to_time64(&tm);
}

static void __qc_reboot_update_onoff_history_on_lpm_mode(
		struct rtc_time *local_tm,
		onoff_reason_t *curr, onoff_reason_t *prev)
{
	if (curr->rtc_offset < 0)
		goto err_rtc_offset;

	if (!prev)
		goto err_without_prev;

	if (curr->rtc_offset < prev->rtc_offset || !prev->local_time)
		goto err_invalid_prev;

	curr->local_time = prev->local_time +
			(curr->rtc_offset - prev->rtc_offset);
	rtc_time64_to_tm(curr->local_time, local_tm);

err_invalid_prev:
err_without_prev:
err_rtc_offset:
	curr->local_time = 0;
}

static void __qc_reboot_update_onoff_history_on_other_mode(
		struct rtc_time *local_tm,
		onoff_reason_t *curr)
{
	struct timespec64 now;
	time64_t local_time;

	ktime_get_real_ts64(&now);

	local_time = now.tv_sec;
	local_time -= (time64_t)sys_tz.tz_minuteswest * 60;	/* adjust time zone */
	curr->local_time = local_time;

	rtc_time64_to_tm(local_time, local_tm);
}

static void __qc_reboot_update_onoff_history(struct qc_debug_drvdata *drvdata,
		unsigned long action, const char *cmd,
		onoff_history_t *onoff_history)
{
	struct device *dev = drvdata->bd.dev;
	struct rtc_time local_tm;
	uint32_t curr_idx = onoff_history->index
			% SEC_DEBUG_ONOFF_HISTORY_MAX_CNT;
	onoff_reason_t *curr = &onoff_history->history[curr_idx];
	const struct sec_log_buf_head *s_log_buf;

	curr->rtc_offset = __qc_reboot_get_rtc_offset();

	if (drvdata->is_lpm_mode) {
		uint32_t prev_idx = (onoff_history->index - 1)
				% SEC_DEBUG_ONOFF_HISTORY_MAX_CNT;
		onoff_reason_t *prev = curr_idx ?
				&onoff_history->history[prev_idx] : NULL;

		__qc_reboot_update_onoff_history_on_lpm_mode(&local_tm, curr, prev);
	} else
		__qc_reboot_update_onoff_history_on_other_mode(&local_tm, curr);

	s_log_buf = sec_log_buf_get_header();
	curr->boot_cnt = IS_ERR(s_log_buf) ?  0 : s_log_buf->boot_cnt;

	snprintf(curr->reason, SEC_DEBUG_ONOFF_REASON_STR_SIZE,
			"%s at Kernel%s%s",
			action == SYS_RESTART ? "Reboot" : "Power Off",
			drvdata->is_lpm_mode ? "(in LPM) " : " ", cmd);

	onoff_history->index++;

	dev_info(dev, "%ptR(TZ:%02d), %s, %s\n",
				&local_tm, -sys_tz.tz_minuteswest / 60,
				action == SYS_RESTART ? "reboot" : "power off",
				cmd);

	dev_info(dev, "BOOT_CNT(%u) RTC(%lld)\n",
			curr->boot_cnt, curr->rtc_offset);
}

static inline void ____qc_reboot_store_onoff_history(
		struct qc_debug_drvdata *drvdata,
		unsigned long action, void *data)
{
	struct device *dev = drvdata->bd.dev;
	onoff_history_t *onoff_history;
	bool valid;

	onoff_history = kvmalloc(sizeof(onoff_history_t), GFP_KERNEL);
	if (!onoff_history)
		return;

	valid = sec_qc_dbg_part_read(debug_index_onoff_history, onoff_history);
	if (!valid) {
		dev_warn(dev, "fail to read onoff_history data\n");
		goto err_read_dbg_partition;
	}

	if (onoff_history->magic != SEC_LOG_MAGIC ||
	    onoff_history->size != sizeof(onoff_history_t)) {
		dev_warn(dev, "invalid magic & size (0x%08x, %u, %zu)\n",
				onoff_history->magic,
				onoff_history->size,
				sizeof(onoff_history_t));
		goto err_invalid_onoff_history;
	}

	__qc_reboot_update_onoff_history(drvdata, action, data,
			onoff_history);

	valid = sec_qc_dbg_part_write(debug_index_onoff_history, onoff_history);
	if (!valid)
		dev_warn(dev, "fail to wrtie onoff_history data\n");

err_invalid_onoff_history:
err_read_dbg_partition:
	kvfree(onoff_history);
}

static inline void __qc_reboot_store_onoff_history(
		struct qc_debug_drvdata *drvdata,
		unsigned long action, void *data)
{
	if (action != SYS_RESTART && action != SYS_POWER_OFF)
		return;

	____qc_reboot_store_onoff_history(drvdata, action, data);
}

static int sec_qc_debug_reboot_handle(struct notifier_block *nb,
		unsigned long action, void *data)
{
	struct qc_debug_drvdata *drvdata = container_of(nb,
			struct qc_debug_drvdata, reboot_nb);

	__qc_reboot_recovery(drvdata, action, data);
	__qc_reboot_param(drvdata, action, data);

	if (drvdata->use_store_onoff_history)
		__qc_reboot_store_onoff_history(drvdata, action, data);

	if (drvdata->use_store_last_kmsg)
		__qc_reboot_store_last_kmsg(drvdata, action, data);

	return NOTIFY_OK;
}

int sec_qc_debug_reboot_init(struct builder *bd)
{
	struct qc_debug_drvdata *drvdata =
			container_of(bd, struct qc_debug_drvdata, bd);
	struct notifier_block *nb = &drvdata->reboot_nb;

	nb->notifier_call = sec_qc_debug_reboot_handle;

	return register_reboot_notifier(nb);
}

void sec_qc_debug_reboot_exit(struct builder *bd)
{
	struct qc_debug_drvdata *drvdata =
			container_of(bd, struct qc_debug_drvdata, bd);

	unregister_reboot_notifier(&drvdata->reboot_nb);
}
