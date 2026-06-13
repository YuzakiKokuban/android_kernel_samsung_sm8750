/*
 * =================================================================
 *
 *	Description:  samsung display debug common file
 *	Company:  Samsung Electronics
 *
 * ================================================================
 *
 *
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017, Samsung Electronics. All rights reserved.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/clk-provider.h>
#include "ss_dsi_panel_common.h"

bool enable_pr_debug;

DEFINE_SPINLOCK(ss_rlock);

/* MIPI retry/recovery log history */
static struct ss_dbg_rlog {
	struct ss_rlog_data logs[SS_RLOG_ENTRY];
	u32 first;
	u32 last;
	u32 curr;
} ss_dbg_rlog;

/* Samsung mipi recovert (rlog) & DUMP Function */
void ss_rlog(int ndx, int type, int retry_cnt, int recovery_done)
{
	unsigned long flags;
	struct ss_rlog_data *log;

	spin_lock_irqsave(&ss_rlock, flags);
	log = &ss_dbg_rlog.logs[ss_dbg_rlog.curr];
	log->time = ktime_to_us(ktime_get());

	log->type = type;
	log->ndx = ndx;
	log->retry_cnt = retry_cnt;
	log->recovery_done = recovery_done;

	ss_dbg_rlog.curr = (ss_dbg_rlog.curr + 1) % SS_RLOG_ENTRY;
	ss_dbg_rlog.last++;

	spin_unlock_irqrestore(&ss_rlock, flags);
}

DEFINE_SPINLOCK(ss_xlock);

static struct ss_dbg_xlog {
	struct ss_tlog logs[SS_XLOG_ENTRY];
	u32 first;
	u32 last;
	u32 curr;
	struct dentry *ss_xlog;
	u32 xlog_enable;
	u32 panic_on_err;
} ss_dbg_xlog;

void ss_display_panic(struct samsung_display_driver_data *vdd, const char *panic_msg)
{
#if IS_ENABLED(CONFIG_SEC_DEBUG)
	if (sec_debug_is_enabled())
		panic(panic_msg);
#endif

	/* For some reason, vdd may be NULL. That's OK. */
	if (vdd)
		LCD_ERR(vdd, "Panic is triggered only debug level MID or HIGH\n");
	else
		pr_err("Panic is triggered only debug level MID or HIGH\n");

	return;
}

/* Samsung XLOG & DUMP Function */
void ss_xlog(const char *name, int flag, ...)
{
	unsigned long flags;
	int i, val = 0;
	va_list args;
	struct ss_tlog *log;

	spin_lock_irqsave(&ss_xlock, flags);
	log = &ss_dbg_xlog.logs[ss_dbg_xlog.curr];
	log->time = ktime_to_us(ktime_get());
	log->name = name;
	log->pid = current->pid;

	va_start(args, flag);
	for (i = 0; i < SS_XLOG_MAX_DATA; i++) {
		val = va_arg(args, int);
		if (val == DATA_LIMITER)
			break;

		log->data[i] = val;
	}
	va_end(args);
	log->data_cnt = i;
	ss_dbg_xlog.curr = (ss_dbg_xlog.curr + 1) % SS_XLOG_ENTRY;
	ss_dbg_xlog.last++;

	spin_unlock_irqrestore(&ss_xlock, flags);
}

static bool __ss_dump_xlog_calc_range(void)
{
	static u32 next;
	bool need_dump = true;
	unsigned long flags;
	struct ss_dbg_xlog *xlog = &ss_dbg_xlog;

	spin_lock_irqsave(&ss_xlock, flags);

	xlog->first = next;

	if (xlog->last == xlog->first) {
		need_dump = false;
		goto dump_exit;
	}

	if (xlog->last < xlog->first) {
		xlog->first %= SS_XLOG_ENTRY;
		if (xlog->last < xlog->first)
			xlog->last += SS_XLOG_ENTRY;
	}

	if ((xlog->last - xlog->first) > SS_XLOG_ENTRY) {
		pr_warn("xlog buffer overflow before dump: %d\n",
			xlog->last - xlog->first);
		xlog->first = xlog->last - SS_XLOG_ENTRY;
	}

	next = xlog->first + 1;

dump_exit:
	spin_unlock_irqrestore(&ss_xlock, flags);

	return need_dump;
}

static ssize_t ss_xlog_dump_entry(char *xlog_buf, ssize_t xlog_buf_size)
{
	int i;
	ssize_t off = 0;
	struct ss_tlog *log;
	unsigned long flags;

	spin_lock_irqsave(&ss_xlock, flags);

	log = &ss_dbg_xlog.logs[ss_dbg_xlog.first %
		SS_XLOG_ENTRY];

	off = scnprintf((xlog_buf + off), (xlog_buf_size - off),
		"[%5llu.%6llu]:", log->time/1000000, log->time%1000000);

	if (off < SS_XLOG_BUF_ALIGN_TIME) {
		memset((xlog_buf + off), 0x20, (SS_XLOG_BUF_ALIGN_TIME - off));
		off = SS_XLOG_BUF_ALIGN_TIME;
	}

	off += scnprintf((xlog_buf + off), (xlog_buf_size - off), "%s => ",
		log->name);

	for (i = 0; i < log->data_cnt; i++)
		off += scnprintf((xlog_buf + off), (xlog_buf_size - off),
			"%x ", log->data[i]);

	off += scnprintf((xlog_buf + off), (xlog_buf_size - off), "\n");

	spin_unlock_irqrestore(&ss_xlock, flags);

	return off;
}

void ss_dump_xlog(void)
{
	char xlog_buf[SS_XLOG_BUF_MAX] = {0,};

	pr_info("============ Start Samsung XLOG ============\n");

	while (__ss_dump_xlog_calc_range()) {
		ss_xlog_dump_entry(xlog_buf, SS_XLOG_BUF_MAX);
		pr_info("%s", xlog_buf);
	}

	pr_info("============ Finish Samsung XLOG ============\n");
}

static ssize_t ss_xlog_dump_read(struct file *file, char __user *buff,
		size_t count, loff_t *ppos)
{
	ssize_t len = 0;
	char xlog_buf[SS_XLOG_BUF_MAX];

	if (__ss_dump_xlog_calc_range()) {
		len = ss_xlog_dump_entry(xlog_buf, SS_XLOG_BUF_MAX);

		if (len < 0 || len > count) {
			pr_err("len is more than user buffer size");
			return 0;
		}

		if (copy_to_user(buff, xlog_buf, len))
			return -EFAULT;
		*ppos += len;
	}

	return len;
}

static int ss_xlog_dump_show(struct seq_file *s, void *unused)
{
	ssize_t len = 0;
	char xlog_buf[SS_XLOG_BUF_MAX];

	if (__ss_dump_xlog_calc_range()) {
		len = ss_xlog_dump_entry(xlog_buf, SS_XLOG_BUF_MAX);
		seq_printf(s, "%s", xlog_buf);
	}

	return len;
}

static int ss_xlog_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, ss_xlog_dump_show, inode->i_private);
}

void ss_store_xlog_panic_dbg(void)
{
	int last, i;
	ssize_t len = 0;
	char err_buf[SS_XLOG_PANIC_DBG_LENGTH] = {0,};
	struct ss_tlog *log;
	struct ss_dbg_xlog *xlog = &ss_dbg_xlog;
	struct samsung_display_driver_data *vdd_primary = ss_get_vdd(PRIMARY_DISPLAY_NDX);
	struct samsung_display_driver_data *vdd_secondary = ss_get_vdd(SECONDARY_DISPLAY_NDX);

	last = xlog->last;
	if (last)
		last--;

	while (last >= 0) {
		log = &ss_dbg_xlog.logs[last % SS_XLOG_ENTRY];
		len += scnprintf((err_buf + len), (sizeof(err_buf) - len),
			"[%llu.%3llu]%s=>", log->time/1000000,
			log->time%1000000, log->name);
		if (len >= SS_XLOG_PANIC_DBG_LENGTH)
			goto end;
		for (i = 0; i < log->data_cnt; i++) {
			len += scnprintf((err_buf + len),
				(sizeof(err_buf) - len), "%x ", log->data[i]);
			if (len >= SS_XLOG_PANIC_DBG_LENGTH)
				goto end;
		}
		last--;
	}
end:
	pr_info("%s:%s\n", __func__, err_buf);

	if (vdd_primary && ss_gpio_is_valid(vdd_primary->ub_con_det.gpio))
		LCD_INFO(vdd_primary, "ub con gpio for primary = %d\n", ss_gpio_get_value(vdd_primary, vdd_primary->ub_con_det.gpio));
	if (vdd_secondary && ss_gpio_is_valid(vdd_secondary->ub_con_det.gpio))
		LCD_INFO(vdd_secondary, "ub con gpio for secondary = %d\n", ss_gpio_get_value(vdd_secondary, vdd_secondary->ub_con_det.gpio));

}

static const struct file_operations xlog_dump_ops = {
	.open = ss_xlog_dump_open,
	.read = ss_xlog_dump_read,
	.release = single_release,
};

#define SS_ONCE_LOG_BUF_MAX	(2048)
static int debug_display_read_once(struct samsung_display_driver_data *vdd,
				char __user *buff, loff_t *ppos)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct lfd_div_info div_info;
	int i, scope, ret;
	char *buf = NULL;
	ssize_t len = 0;
	u8 ecc_enable = 0, ecc_restore_cnt = 0, ecc_fail_cnt = 0;
	struct ss_rlog_data *log;
	bool log_over;
	int start_idx, last_idx;

	buf = kzalloc(SS_ONCE_LOG_BUF_MAX, GFP_KERNEL);
	if (!buf) {
		LCD_ERR(vdd, "fail to alloc for once_log_buf\n");
		return 0;
	}

	for (i = PRIMARY_DISPLAY_NDX; i < MAX_DISPLAY_NDX; i++) {
		struct samsung_display_driver_data *vddp = ss_get_vdd(i);
		if (!vddp)
			continue;

		if (!vddp->manufacture_id_dsi)
			continue;

		LCD_INFO(vddp, "[%s] LCD_ID : %X\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->manufacture_id_dsi);
		len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len, "[%s] LCD_ID : %X\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->manufacture_id_dsi);

		if (vddp->cell_id_dsi) {
			LCD_INFO(vddp, "[%s] CELL_ID : %s\n",
				i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->cell_id_dsi);
			len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len, "[%s] CELL_ID : %s\n",
				i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->cell_id_dsi);
		}

		if (vddp->octa_id_dsi) {
			LCD_INFO(vddp, "[%s] OCTA_ID : %s\n",
				i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->octa_id_dsi);
			len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len, "[%s] OCTA_ID : %s\n",
				i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->octa_id_dsi);
		}

		if (vddp->ddi_id_dsi) {
			LCD_INFO(vddp, "[%s] DDI_ID : %s\n",
				i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->ddi_id_dsi);
			len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len, "[%s] DDI_ID : %s\n",
				i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->ddi_id_dsi);
		}

		LCD_INFO(vddp, "[%s] LAST RDDPM : %X\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->last_rddpm);
		len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len, "[%s] LAST RDDPM : %X\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->last_rddpm);

		LCD_INFO(vddp, "[%s] LAST RDDSM : %X\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->last_rddsm);
		len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len, "[%s] LAST RDDSM : %X\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->last_rddsm);

		LCD_INFO(vddp, "[%s] LAST ESDERR : %X\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->last_esderr);
		len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len, "[%s] LAST ESDERR : %X\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->last_esderr);

		LCD_INFO(vddp, "[%s] LAST DSIERR : %X\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->last_dsierr);
		len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len, "[%s] LAST DSIERR : %X\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->last_dsierr);

		LCD_INFO(vddp, "[%s] LAST PROTOCOL ERR : %X\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->last_protocol_err);
		len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len, "[%s] LAST PROTOCOL ERR : %X\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->last_protocol_err);

		ret = ss_check_ecc(vdd, &ecc_enable, &ecc_restore_cnt, &ecc_fail_cnt);
		if (!ret || ret == -EIO)
			len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len, "[%s] ECC: %X %X %X\n",
				i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB",
				ecc_enable, ecc_restore_cnt, ecc_fail_cnt);

		LCD_INFO(vddp, "[%s] LAST SM_SUM_O : %X [%X]\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB",
			vddp->self_disp.debug.SM_SUM_O, vddp->self_disp.operation[FLAG_SELF_MASK].img_checksum);
		len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len, "[%s] LAST SM_SUM_O : %X [%X]\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB",
			vddp->self_disp.debug.SM_SUM_O, vddp->self_disp.operation[FLAG_SELF_MASK].img_checksum);

		LCD_INFO(vddp, "[%s] MIPI_FAIL_RECOVERY_CNT : %d / MIPI_FAIL_RECOVERY_FAIL_CNT : %d\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->mipi_fail_recovery_cnt, vddp->mipi_fail_recovery_fail_cnt);
		len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len, "[%s] MIPI_FAIL_RECOVERY_CNT : %d / MIPI_FAIL_RECOVERY_FAIL_CNT : %d\n",
			i == PRIMARY_DISPLAY_NDX ? "MAIN" : "SUB", vddp->mipi_fail_recovery_cnt, vddp->mipi_fail_recovery_fail_cnt);
	}

	len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len, "VRR: adj: %d%s\n",
		vdd->vrr.adjusted_refresh_rate,
		vdd->vrr.adjusted_sot_hs_mode ? "HS" : "NM");

	if (panel && panel->cur_mode) {
		len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len, "VRR: cur_mode:  %dx%d@%d%s\n",
			panel->cur_mode->timing.h_active,
			panel->cur_mode->timing.v_active,
			panel->cur_mode->timing.refresh_rate,
			panel->cur_mode->timing.sot_hs_mode ? "HS" : "NM");
	}

	if (vdd->vrr.lfd.support_lfd) {
		for (scope = 0; scope < LFD_SCOPE_MAX; scope++) {
			ss_get_lfd_div(vdd, scope, &div_info);
			len += scnprintf(buf + len, SS_ONCE_LOG_BUF_MAX - len,
				"LFD: scope=%s: LFD freq: %dhz ~ %dhz, div: %d ~ %d\n",
				lfd_scope_name[scope],
				DIV_ROUND_UP(vdd->vrr.lfd.base_rr, div_info.min_div),
				DIV_ROUND_UP(vdd->vrr.lfd.base_rr, div_info.max_div),
				div_info.min_div, div_info.max_div);
		}
	}

	/* show display mipi fail recovery histroy */
	if (ss_dbg_rlog.last) {
		log_over = ss_dbg_rlog.last > SS_RLOG_ENTRY ? true : false;
		if (!log_over) {
			start_idx = 0;
			last_idx = ss_dbg_rlog.last - 1;
		} else {
			start_idx = ss_dbg_rlog.curr;
			if (!start_idx)
				last_idx = SS_RLOG_ENTRY - 1;
			else
				last_idx = ss_dbg_rlog.curr - 1;
		}

		while (true) {
			if (start_idx == SS_RLOG_ENTRY)
				start_idx = 0;

			log = &ss_dbg_rlog.logs[start_idx];

			/*
			* ex1) retry [1] recovery [0]
			* - Do not recovery [0] because it worked normally after retrying once [1].
			* ex2) retry [2] recovery [1]
			* - Do recovery [1] after retrying twice [2].
			* ex2) retry [2] recovery [0]
			* - Do not recovery [0] becuase mipi fail occurred during recovery. (retry 2 is max retry count)
			*/

			len += scnprintf((buf + len), (SS_ONCE_LOG_BUF_MAX - len),
				"[%6llu.%06llu]: [%50s] [SDE_%d] retry [%d] recovery [%d]\n",
				log->time/1000000, log->time%1000000,
				ss_get_cmd_name(log->type), log->ndx, log->retry_cnt, log->recovery_done);

			LCD_INFO(vdd, "[%6llu.%06llu]: [%50s] [SDE_%d] retry [%d] recovery [%d]\n",
				log->time/1000000, log->time%1000000,
				ss_get_cmd_name(log->type), log->ndx, log->retry_cnt, log->recovery_done);

			if (start_idx == last_idx)
				break;

			start_idx++;
		}
	}

	if (copy_to_user(buff, buf, len)) {
		len = -EFAULT;
		goto end;
	}

	*ppos += len;

end:
	kfree(buf);
	return len;
}

static ssize_t debug_display_read(struct file *file, char __user *buff,
		size_t count, loff_t *ppos)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;
	ssize_t len = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (vdd->debug_data->report_once) {
		LCD_INFO(vdd, "report once\n");
		vdd->debug_data->report_once = false;
		len = debug_display_read_once(vdd, buff, ppos);
		//ss_read_ddi_debug_reg(vdd);
		return len;
	}

	len = ss_xlog_dump_read(file, buff, count, ppos);
	if (len)
		return len;

	len = ss_sde_evtlog_dump_read(file, buff, count, ppos);
	if (len)
		return len;

	LCD_INFO(vdd, "done");
	return len;
}

static ssize_t debug_display_write(struct file *file, const char __user *user_buf,
			 size_t count, loff_t *ppos)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;
	char op_buf[DEBUG_DISPLAY_CMD_SIZE];
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (unlikely(!user_buf)) {
		LCD_ERR(vdd, "invalid user buffer\n");
		return -EINVAL;
	}

	if (count > DEBUG_DISPLAY_CMD_SIZE) {
		LCD_ERR(vdd, "Invalid buf len\n");
		return -EINVAL;
	}

	LCD_INFO(vdd, "count=%ld\n", count);

	ret = copy_from_user(op_buf, user_buf, DEBUG_DISPLAY_CMD_SIZE);
	if (unlikely(ret < 0)) {
		LCD_ERR(vdd, "failed to copy_from_user\n");
		return ret;
	}

	if (op_buf[0] == '1' || op_buf[0] == 'y' || op_buf[0] == 'Y' || op_buf[0] == 'A' || op_buf[0] == 'a')
		vdd->debug_data->print_all = true;

	if (op_buf[0] == '0' || op_buf[0] == 'n' || op_buf[0] == 'N') {
		vdd->debug_data->print_all = false;
		vdd->debug_data->print_cmds = false;
		vdd->debug_data->print_vsync = false;
		vdd->debug_data->print_frame = false;
	}

	if (op_buf[0] == 'C' || op_buf[0] == 'c')
		vdd->debug_data->print_cmds = true;

	if (op_buf[0] == 'V' || op_buf[0] == 'v')
		vdd->debug_data->print_vsync = true;

	if (op_buf[0] == 'F' || op_buf[0] == 'f')
		vdd->debug_data->print_frame = true;

	return count;
}

static int debug_display_open(struct inode *inode, struct file *file)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	vdd->debug_data->report_once = true;
	LCD_INFO(vdd, "done");

	/* MDP XLOG */
	ss_sde_dbg_debugfs_open();

	return 0;
}

static int debug_display_release(struct inode *inode, struct file *file)
{
	pr_err("%s : done", __func__);

	return 0;
}

static const struct file_operations debug_display_fops = {
	.owner = THIS_MODULE,
	.open = debug_display_open,
	.read = debug_display_read,
	.write = debug_display_write,
	.release = debug_display_release,
};

#define DEV_NAME_SIZE 24
int ss_disp_dbg_info_misc_register(void)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(0);
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	static char devname[DEV_NAME_SIZE] = {'\0', };
	struct miscdevice *dev = &vdd->debug_data->dev;
	int ret;

	dev->minor = MISC_DYNAMIC_MINOR;
	snprintf(devname, DEV_NAME_SIZE, "sec_display_debug");
	dev->name = devname;
	dev->fops = &debug_display_fops;
	dev->parent = &display->pdev->dev;
	ret = ss_wrapper_misc_register(vdd, dev);
	if (ret) {
		LCD_INFO(vdd, "failed to register driver : %d\n", ret);
		return -ENODEV;
	}

	return 0;
}

enum {
	RDDPM_POC_LOAD = 0,
	RDDPM_REAL_DISPON,
	RDDPM_DISPON,
	RDDPM_NORON,
	RDDPM_SLPOUT,
	RDDPM_PTLON,
	RDDPM_IDMON,
	RDDPM_BSTVTG,
	MAX_RDDPM_BIT
};

static char *rddpm_bit[MAX_RDDPM_BIT] = {
	"FLASH LOAD",
	"REAL DISPON",
	"DISP ON",
	"NORAL MODE",
	"SLEEP OUT",
	"PARTIAL MODE",
	"IDLE MODE",
	"BOOSTER VOLTAGE",
};

int ss_check_rddpm(struct samsung_display_driver_data *vdd, u8 *rddpm)
{
	int bit;
	int ret = 0;
	int read_len = 0;

	*rddpm = 0;

	if (SS_IS_SS_CMDS_NULL(ss_get_ss_cmds(vdd, RX_LDI_DEBUG0)))
		return -ENODEV;

	read_len = ss_send_cmd_get_rx(vdd, RX_LDI_DEBUG0, rddpm);
	if (read_len <= 0) {
		LCD_ERR(vdd, "fail to read rddpm(read_len=%d)\n", read_len);
		return -EFAULT;
	}

	vdd->last_rddpm = *rddpm;

	for (bit = RDDPM_POC_LOAD; bit < MAX_RDDPM_BIT; bit++) {
		if (bit == RDDPM_PTLON || bit == RDDPM_IDMON) /* don't care partial/idle mode */
			continue;

		if (!(*rddpm & (1 << bit))) {
			LCD_ERR(vdd, "%x : rddpm err(bit%d): %s\n", *rddpm, bit, rddpm_bit[bit]);

			/* boost voltage (ELAVDD_7P9) fault status
			 * ELAVDD_7P9 is controlled by EL_ON1, which is
			 * monotired by DISP_DET pin (esd detection pin).
			 * Check DISP_DET (EL_ON1) pin level.
			 */
			if (bit == RDDPM_BSTVTG) {
				struct esd_recovery *esd = &vdd->esd_recovery;
				int i;

				for (i = 0; i < esd->num_of_gpio; i++)
					if (!ss_gpio_get_value(vdd, esd->esd_gpio[i]))
						LCD_INFO(vdd, "err: ELON1 drop(gpio: %d)\n",
								esd->esd_gpio[i]);
			}
			ret = -EIO;
		}
	}

	return ret;
}

enum {
	ESDERR_DSI = 0,
	ESDERR_HSCLK,
	ESDERR_VLIN3,
	ESDERR_ELVDD,
	ESDERR_CHKSUM,
	ESDERR_HSYNC,
	ESDERR_VLIN1,
	ESDERR_GRAM,
	ESDERR_VGH,
	ESDERR_VGL,
	MAX_ESDERR_BIT
};

static char *esderr_bit[MAX_ESDERR_BIT] = {
	"ESDERR_DSI",
	"ESDERR_HSCLK",
	"ESDERR_VLIN3",
	"ESDERR_ELVDD",
	"ESDERR_CHKSUM",
	"ESDERR_HSYNC",
	"ESDERR_VLIN1",
	"ESDERR_GRAM",
	"ESDERR_VGH",
	"ESDERR_VGL",
};

int ss_check_esderr(struct samsung_display_driver_data *vdd, u16 *esderr)
{
	u8 recv_buf[2] = {0, };
	int bit;
	int ret = 0;
	int read_len = 0;

	if (SS_IS_SS_CMDS_NULL(ss_get_ss_cmds(vdd, RX_LDI_DEBUG2)))
		return -ENODEV;

	read_len = ss_send_cmd_get_rx(vdd, RX_LDI_DEBUG2, recv_buf);
	if (read_len <= 0) {
		LCD_ERR(vdd, "fail to read esderr(read_len=%d)\n", read_len);
		*esderr = 0;
		return -EFAULT;
	}

	*esderr = (recv_buf[0] << 8) | recv_buf[1];
	vdd->last_esderr = *esderr;

	for (bit = ESDERR_DSI; bit < MAX_ESDERR_BIT; bit++) {
		if ((*esderr & (1 << bit))) {
			LCD_INFO(vdd, "esderr err(bit%d): %s\n", bit, esderr_bit[bit]);
			ret = -EIO;

			if (bit == ESDERR_VLIN3)
				inc_dpui_u32_field(DPUI_KEY_PNVLO3E, 1);
			else if (bit == ESDERR_ELVDD)
				inc_dpui_u32_field(DPUI_KEY_PNELVDE, 1);
			else if (bit == ESDERR_VLIN1)
				inc_dpui_u32_field(DPUI_KEY_PNVLI1E, 1);
		}
	}

	return ret;
}

enum {
	RDDSM_DSI_ERR = 0,
	RDDSM_OTP_ERR = 2,
	RDDSM_LV1_ERR = 3,
	RDDSM_TEMOD = 6,
	RDDSM_TEON = 7,
	MAX_RDDSM_BIT
};

static char *rddsm_bit[MAX_RDDSM_BIT] = {
	"DSI_ERR",
	"",
	"OTP_ERR",
	"LV1_ERR",
	"",
	"",
	"TEMOD",
	"TEON",
};

int ss_check_rddsm(struct samsung_display_driver_data *vdd, u8 *rddsm)
{
	int bit;
	int ret = 0;
	int read_len = 0;

	*rddsm = 0;

	if (SS_IS_SS_CMDS_NULL(ss_get_ss_cmds(vdd, RX_LDI_DEBUG3)))
		return -ENODEV;

	read_len = ss_send_cmd_get_rx(vdd, RX_LDI_DEBUG3, rddsm);
	if (read_len <= 0) {
		LCD_ERR(vdd, "fail to read rddsm(read_len=%d)\n", read_len);
		return -EFAULT;
	}

	vdd->last_rddsm = *rddsm;

	for (bit = RDDSM_DSI_ERR; bit < MAX_RDDSM_BIT; bit++) {
		if (bit == RDDSM_DSI_ERR || bit == RDDSM_OTP_ERR || bit == RDDSM_LV1_ERR) {
			if ((*rddsm & (1 << bit))) {
				LCD_INFO(vdd, "rddsm err(bit%d): %s\n", bit, rddsm_bit[bit]);
				ret = -EIO;
			}
		} else if (bit == RDDSM_TEON) {
			if (!(*rddsm & (1 << bit))) {
				LCD_INFO(vdd, "rddsm err(bit%d): %s\n", bit, rddsm_bit[bit]);
				ret = -EIO;
			}
		}
	}

	return ret;
}

int ss_check_dsierr(struct samsung_display_driver_data *vdd, u8 *dsierr_cnt)
{
	int ret = 0;
	int read_len = 0;

	if (SS_IS_SS_CMDS_NULL(ss_get_ss_cmds(vdd, RX_LDI_DEBUG4)))
		return -ENODEV;

	read_len = ss_send_cmd_get_rx(vdd, RX_LDI_DEBUG4, dsierr_cnt);
	if (read_len <= 0) {
		*dsierr_cnt = 0;
		LCD_ERR(vdd, "fail to read dsierr_cnt(ret=%d)\n", ret);
		return -EFAULT;
	}

	vdd->last_dsierr = *dsierr_cnt;

	if (*dsierr_cnt) {
		LCD_ERR(vdd, "DSI Error Count: %d\n", *dsierr_cnt);
		ret = -EIO;
#if IS_ENABLED(CONFIG_SEC_ABC)
		if (vdd->ndx == PRIMARY_DISPLAY_NDX)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
			sec_abc_send_event("MODULE=display@INFO=act_section_dsierr0");
#else
			sec_abc_send_event("MODULE=display@WARN=act_section_dsierr0");
#endif
		else
#if IS_ENABLED(CONFIG_SEC_FACTORY)
			sec_abc_send_event("MODULE=display@INFO=act_section_dsierr1");
#else
			sec_abc_send_event("MODULE=display@WARN=act_section_dsierr1");
#endif
#endif
	}

	inc_dpui_u32_field(DPUI_KEY_PNDSIE, *dsierr_cnt);

	return ret;
}

int ss_read_self_diag(struct samsung_display_driver_data *vdd)
{
	char self_diag = 0;
	int read_len = 0;

	if (SS_IS_SS_CMDS_NULL(ss_get_ss_cmds(vdd, RX_LDI_DEBUG5)))
		return 0;

	read_len = ss_send_cmd_get_rx(vdd, RX_LDI_DEBUG5, &self_diag);
	if (read_len <= 0) {
		LCD_ERR(vdd, "fail to read rddpm(ret=%d)\n", read_len);
		return 0;
	}

	LCD_DEBUG(vdd, "========== SHOW PANEL [0Fh:SELF_DIAG] INFO ==========\n");
	LCD_DEBUG(vdd, "* Reg Value : 0x%02x, Result : %s\n",
			self_diag, (self_diag & 0x80) ? "GOOD" : "NG");
	if ((self_diag & 0x80) == 0)
		LCD_INFO(vdd, "* OTP Reg Loading Error\n");
	LCD_DEBUG(vdd, "=====================================================\n");

	inc_dpui_u32_field(DPUI_KEY_PNSDRE, (self_diag & 0x80) ? 0 : 1);

	return 0;
}

enum {
	PROTERR_SOT = 0,
	PROTERR_SOT_SYNC,
	PROTERR_EOT_SYNC,
	PROTERR_ESCAPE_ENTRY,
	PROTERR_LPTX_SYNC,
	PROTERR_HSRX_TIMEOUT,
	PROTERR_FALSE_CTRL,
	PROTERR_DLANE_CONTENTION,
	PROTERR_ECC_SINGLEBIT,
	PROTERR_ECC_MULTIBIT,
	PROTERR_CHECKSUM,
	PROTERR_INVALID_DATATYPE,
	PROTERR_INVALID_VCID,
	PROTERR_INVALID_TXLEN,
	PROTERR_DATA_PLANE_CONTENTION,
	PROTERR_PROTOCOL_VIOLATION,
	MAX_PROTERR_BIT
};

static char *protocol_err_bit[MAX_PROTERR_BIT] = {
	"PROTERR_SOT",
	"PROTERR_SOT_SYNC",
	"PROTERR_EOT_SYNC",
	"PROTERR_ESCAPE_ENTRY",
	"PROTERR_LPTX_SYNC",
	"PROTERR_HSRX_TIMEOUT",
	"PROTERR_FALSE_CTRL",
	"PROTERR_DLANE_CONTENTION",
	"PROTERR_ECC_SINGLEBIT",
	"PROTERR_ECC_MULTIBIT",
	"PROTERR_CHECKSUM",
	"PROTERR_INVALID_DATATYPE",
	"PROTERR_INVALID_VCID",
	"PROTERR_INVALID_TXLEN",
	"PROTERR_DATA_PLANE_CONTENTION",
	"PROTERR_PROTOCOL_VIOLATION",
};

int ss_check_mipi_protocol_err(struct samsung_display_driver_data *vdd, u16 *protocol_err)
{
	u8 rbuf[2] = {0, };
	int bit;
	int ret = 0;
	int read_len = 0;

	if (SS_IS_SS_CMDS_NULL(ss_get_ss_cmds(vdd, RX_LDI_DEBUG6)))
		return -ENODEV;

	read_len = ss_send_cmd_get_rx(vdd, RX_LDI_DEBUG6, rbuf);
	if (read_len <= 0) {
		*protocol_err = 0;
		LCD_ERR(vdd, "fail to read protocol_err(ret=%d)\n", read_len);
		return -EFAULT;
	}

	*protocol_err = (rbuf[0] << 8) | rbuf[1];
	vdd->last_protocol_err = *protocol_err;

	for (bit = PROTERR_SOT ; bit < MAX_PROTERR_BIT; bit++) {
		if ((*protocol_err & (1 << bit))) {
			LCD_INFO(vdd, "protocol_err err(bit%d): %s\n", bit, protocol_err_bit[bit]);
			ret = -EIO;
		}
	}

	return ret;
}

int ss_check_flash_done(struct samsung_display_driver_data *vdd, u8 *buf)
{
	int ret = 0;

	if (vdd->panel_func.check_flash_done) {
		vdd->panel_func.check_flash_done(vdd);
		ret = vdd->need_panel_recovery;
	} else {
		LCD_INFO(vdd, "check_flash function is Null\n");
	}

	return ret;
}

int ss_check_ecc(struct samsung_display_driver_data *vdd,
		u8 *enable, u8 *cnt_restore, u8 *cnt_fail)
{
	u8 buf[3] = {0, };
	int cnt_total;
	int ret = 0;

	/* temporally return it */
	/* P240911-03425, P240910-01213, P240909-03617*/
	return 0;
	if (SS_IS_SS_CMDS_NULL(ss_get_ss_cmds(vdd, RX_GCT_ECC)))
		return -ENODEV;

	ret = ss_send_cmd_get_rx(vdd, RX_GCT_ECC, buf);
	if (ret <= 0) {
		LCD_ERR(vdd, "fail to read gct ecc (%d)\n", ret);
		return ret;
	}

	*enable = buf[0];
	cnt_total = buf[1];
	*cnt_fail = buf[2];
	*cnt_restore = cnt_total - *cnt_fail;

	LCD_INFO(vdd, "ECC = [ENABLE] : 0x%02X, [RESTORE] : 0x%02X, [FAIL TO RESTORE] : 0x%02X\n",
			*enable, *cnt_restore, *cnt_fail);

	if (*cnt_fail > 0) {
		LCD_ERR(vdd, "fail to restore error bits of gram (cnt: %d)\n", *cnt_fail);
		return -EIO;
	}

	return 0;
}

int ss_read_ddi_debug_reg(struct samsung_display_driver_data *vdd)
{
	u8 rddpm = 0, rddsm = 0, dsierr_cnt = 0, flash_done = 0;
	u16 esderr = 0, protocol_err = 0;
	u8 ecc_enable = 0, ecc_restore_cnt = 0, ecc_fail_cnt = 0;
	int ret = 0;

	ret |= ss_check_rddpm(vdd, &rddpm);
	ret |= ss_check_rddsm(vdd, &rddsm);
	ret |= ss_check_esderr(vdd, &esderr);
	ret |= ss_check_dsierr(vdd, &dsierr_cnt);
	ret |= ss_check_mipi_protocol_err(vdd, &protocol_err);
	ret |= ss_check_ecc(vdd, &ecc_enable, &ecc_restore_cnt, &ecc_fail_cnt);
	ret = ret & -EIO;

	/* ss_check_flash_done return value is vdd->flash_done_fail */
	ret |= ss_check_flash_done(vdd, &flash_done);

	ss_read_pps_data(vdd);

	if (ret) {
		LCD_ERR(vdd, "fail: panel dbg: %x %x %x %x %x %x %x\n",
				rddpm, rddsm, esderr, dsierr_cnt, protocol_err, flash_done, ecc_fail_cnt);
		SS_XLOG(vdd->ndx, rddpm, rddsm, esderr, dsierr_cnt, protocol_err, flash_done, ecc_fail_cnt);
	} else {
		LCD_INFO(vdd, "pass: panel dbg: %x %x %x %x %x %x %x\n",
				rddpm, rddsm, esderr, dsierr_cnt, protocol_err, flash_done, ecc_fail_cnt);
	}

	return ret;
}

/* DDI CMD log buffer max size is 512 bytes.
 * But, qct display driver limit max read size to 255 bytes (0xFF)
 * due to panel dtsi. panel dtsi determines length with one byte.
 * samsung,ldi_debug_logbuf_rx_cmds_revA   = [06 01 00 00 00 00 01 9C FF 00];
 */

#define DDI_CMD_LOGBUF_SIZE	15

int ss_read_ddi_cmd_log(struct samsung_display_driver_data *vdd, char *read_buf)
{
	int ret;
	int i;

	ret = ss_send_cmd_get_rx(vdd, RX_LDI_DEBUG_LOGBUF, read_buf);
	if (ret <= 0) {
		*read_buf = 0;
		LCD_INFO(vdd, "fail to read ddi cmd log buffer(ret=%d)\n", ret);
		return ret;
	}

	LCD_INFO(vdd, "DDI command log:");
	for (i = 0; i < DDI_CMD_LOGBUF_SIZE; i++)
		pr_cont(" %02x", read_buf[i]);
	pr_cont("\n");

	return 0;
}

#if IS_ENABLED(CONFIG_SEC_FACTORY)
char bootloader_pps1_data[SZ_64]; /* 0xA2 : PPS data (0x00 ~ 0x2C) */
char bootloader_pps2_data[SZ_64]; /* 0xA2 : PPS data (0x2d ~ 0x58)*/
int ss_read_pps_data(struct samsung_display_driver_data *vdd)
{
	int ret;

	ret = ss_send_cmd_get_rx(vdd, RX_LDI_DEBUG_PPS1, bootloader_pps1_data);
	if (ret <= 0) {
		LCD_INFO(vdd, "fail to read pps_data(ret=%d)\n", ret);
		return ret;
	}

	ret = ss_send_cmd_get_rx(vdd, RX_LDI_DEBUG_PPS2, bootloader_pps2_data);
	if (ret <= 0) {
		LCD_INFO(vdd, "fail to read pps_data(ret=%d)\n", ret);
		return ret;
	}

	return 0;
}
#else
int ss_read_pps_data(struct samsung_display_driver_data *vdd)
{
	LCD_DEBUG(vdd, "nothing to do\n");
	return 0;
}
#endif

struct clock_index_table {
	int rat;
	int band;
	int link_MHz;
	int index;
};

/* TODO: print info. for both of primary and secondary vdd */
static ssize_t ss_read_dyn_mipi_clk_index_table(struct file *file, char __user *buff,
		size_t count, loff_t *ppos)
{
	static char clk_table_buff[1024];
	static int num_of_initialized;
	int buff_offset = 0;
	int buff_size = 1024;
	struct samsung_display_driver_data *vdd;
	struct clk_timing_table *clk_timing_table = NULL;
	struct clk_sel_table *clk_sel_table = NULL;
	struct clock_index_table *clk_index_table = NULL;
	int i = 0;
	int index_table_size = 0;
	int clk_timing_table_size = 0;

	vdd = (struct samsung_display_driver_data *)file->private_data;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "Invalid vdd\n");
		return -EINVAL;
	}

	/* Initialize index table size and clock index table */
	clk_timing_table = &vdd->dyn_mipi_clk.clk_timing_table;
	if (IS_ERR_OR_NULL(clk_timing_table)) {
		LCD_INFO(vdd, "Invalid timing table\n");
		return -EINVAL;
	}

	index_table_size = clk_timing_table->tab_size;
	clk_index_table = kzalloc(sizeof(struct clock_index_table)
				* index_table_size, GFP_KERNEL);

	if (IS_ERR_OR_NULL(clk_index_table)) {
		LCD_INFO(vdd, "Fail to allocate clk_index_table\n");
		return -ENOMEM;
	}

	if (num_of_initialized >= index_table_size)
		goto end;

	for (i = 0; i < index_table_size; i++) {
		/* Initialize the index in  increasing order */
		clk_index_table[i].index = i;
	}

	/*
	 * Initialize index 0 to abnormal value
	 * because the index 0 is used when if abnormal values
	 * are written into "/sys/class/lcd/panel/rf_info"
	 */
	clk_index_table[0].rat = -1;
	clk_index_table[0].band = -1;
	clk_index_table[0].link_MHz = -1;
	clk_index_table[0].index = 0;
	num_of_initialized++;

	/* Initialize clock select table size */
	clk_sel_table = &vdd->dyn_mipi_clk.clk_sel_table;
	if (IS_ERR_OR_NULL(clk_timing_table)) {
		LCD_INFO(vdd, "Invalid timing table\n");
		return -EINVAL;
	}

	clk_timing_table_size = clk_sel_table->tab_size;

	/* Check clcok select table and initialize index table */
	for (i = 0; i < clk_timing_table_size; i++) {
		int index = 0;

		if (num_of_initialized >= index_table_size)
			break;

		if (unlikely(IS_ERR_OR_NULL(clk_sel_table->rat) &&
				IS_ERR_OR_NULL(clk_sel_table->band) &&
				IS_ERR_OR_NULL(clk_sel_table->from) &&
				IS_ERR_OR_NULL(clk_sel_table->end) &&
				IS_ERR_OR_NULL(clk_sel_table->target_clk_idx))
		   )
			break;

		index = clk_sel_table->target_clk_idx[i];
		if (unlikely(index > clk_timing_table_size)) {
			LCD_INFO(vdd, "Invalid index: %d\n", index);
			continue;
		}

		/* */
		if (!clk_index_table[index].rat &&
				(clk_index_table[index].index == index)) {
			clk_index_table[index].rat = clk_sel_table->rat[i];
			clk_index_table[index].band = clk_sel_table->band[i];
			clk_index_table[index].link_MHz =
				(clk_sel_table->from[i] + clk_sel_table->end[i]) / 2;
			clk_index_table[index].index = clk_sel_table->target_clk_idx[i];
			num_of_initialized++;
		}
	}

end:
	/* Add index table Length */
	if (!strnlen(clk_table_buff, buff_size)) {
		snprintf(clk_table_buff + buff_offset, buff_size - buff_offset,
				"Index Table Length: %d\n", index_table_size);
		buff_offset = strnlen(clk_table_buff, buff_size);

		for (i = 0; i < index_table_size; i++) {
			if (clk_index_table[i].rat) {
				snprintf(clk_table_buff + buff_offset, buff_size - buff_offset,
						"RAT: %d, BAND: %d, link_MHz: %d, INDEX: %d\n",
						clk_index_table[i].rat, clk_index_table[i].band,
						clk_index_table[i].link_MHz, clk_index_table[i].index
						);
				buff_offset = strnlen(clk_table_buff, buff_size);
			}
		}
	}

	kfree(clk_index_table);

	return simple_read_from_buffer(buff, count, ppos, clk_table_buff, strlen(clk_table_buff));
}

static void ss_dbg_no_te(struct samsung_display_driver_data *vdd)
{
	struct dbg_tear_info *dbg = &vdd->dbg_tear_info;
	int i, j, pos;

	if (!vdd) {
		pr_err("SDE err: vdd null\n");
		return;
	}

	LCD_ERR(vdd, "no TE\n");

	/* TE on/off in RDDSM */
	LCD_INFO(vdd, "dump ddi dbg registers\n");
	ss_read_ddi_debug_reg(vdd);

	/* TE gpio*/
	LCD_INFO(vdd, "check TE gpio\n");
	ss_check_te(vdd);

	/* mipi command event log */
	if (dbg->dump_msg_log) {
		LCD_INFO(vdd, "dump command event log(pos: %d)\n", dbg->pos_evtlog);
		for (i = 0; i < MAX_CMD_EVTLOG_NUM; i++) {
			pos = (dbg->pos_evtlog - i + MAX_CMD_EVTLOG_NUM) % MAX_CMD_EVTLOG_NUM;

			pr_info("t: %lld, ", dbg->evtlog[pos].time);
			if (dbg->evtlog[pos].cmd_len > MAX_CMD_EVTLOG_CMDS_NUM)
				pr_cont("len: %zd, ", dbg->evtlog[pos].cmd_len);
			for (j = 0; j < dbg->evtlog[pos].cmd_len && j < MAX_CMD_EVTLOG_CMDS_NUM	; j++)
				pr_cont("%02X ", dbg->evtlog[pos].cmds[j]);
			pr_cont("\n");
		}
	}

	/* panic */
	if (dbg->force_panic_no_te)
		ss_display_panic(vdd, "No TE");
}

#define MAX_PRINT_BUF (512)
#define MAX_PRINT_TS_CNT (10)
static void ss_dbg_tear_work(struct work_struct *work)
{
	struct delayed_work * dwork = to_delayed_work(work);
	struct dbg_tear_info *dbg =
		container_of(dwork, struct dbg_tear_info, dbg_tear_work);
	struct samsung_display_driver_data *vdd = dbg ?
		container_of(dbg, struct samsung_display_driver_data, dbg_tear_info) : NULL;
	struct drm_device *ddev = NULL;
	char buf[MAX_PRINT_BUF];
	int pos;
	int i;

	if (!vdd) {
		LCD_ERR(vdd, "no dbg/vdd\n");
		return;
	}

	ddev = GET_DRM_DEV(vdd);

	if (pm_runtime_status_suspended(ddev->dev)) {
		LCD_INFO(vdd, "pm_runtime_suspend.. just return!\n");
		return;
	}

	/* TE timestamp */
	pos = 0;
	memset(buf, 0, MAX_PRINT_BUF);

	pos += scnprintf(buf + pos, MAX_PRINT_BUF - pos, " TE: ");
	for (i = 0; i < MAX_PRINT_TS_CNT; i++)
		pos += scnprintf(buf + pos, MAX_PRINT_BUF - pos, "%lld ",
			(dbg->ts_frame_te[(dbg->pos_frame_te - i + MAX_TS_BUF_CNT) % MAX_TS_BUF_CNT] / 1000000));

	/* WR_PTR timestamp */
	pos += scnprintf(buf + pos, MAX_PRINT_BUF - pos, " WR: ");
	for (i = 0; i < MAX_PRINT_TS_CNT; i++)
		pos += scnprintf(buf + pos, MAX_PRINT_BUF - pos, "%lld ",
			dbg->ts_frame_start[(dbg->pos_frame_start - i + MAX_TS_BUF_CNT) % MAX_TS_BUF_CNT] / 1000000);

	/* CTL_DONE timestamp */
	pos += scnprintf(buf + pos, MAX_PRINT_BUF - pos, " DONE: ");
	for (i = 0; i < MAX_PRINT_TS_CNT; i++)
		pos += scnprintf(buf + pos, MAX_PRINT_BUF - pos, "%lld ",
			dbg->ts_frame_end[(dbg->pos_frame_end - i + MAX_TS_BUF_CNT) % MAX_TS_BUF_CNT] / 1000000);
	LCD_INFO(vdd, "dump TS: %s\n", buf);

	/* No TE case */
	if (dbg->is_no_te) {
		dbg->is_no_te = false;

		if (!vdd->gct.is_running)
			ss_dbg_no_te(vdd);
	}
}

void ss_check_te(struct samsung_display_driver_data *vdd)
{
	unsigned int disp_te_gpio;
	int rc, te_count = 0;
	int te_max = 20000; /*sampling 200ms */

	/* TE is generated after on_seq (PANEL_PWR_ON or PANEL_PWR_LPM) */
	/* Do not check the te signal when panel is off or on_ready (before on_seq) */
	if (ss_is_panel_on_ready(vdd) || ss_is_panel_off(vdd))
		return;

	disp_te_gpio = ss_get_te_gpio(vdd);

	LCD_INFO(vdd, "============ start waiting for TE CTRL%d ============\n", vdd->ndx);
	if (ss_gpio_is_valid(disp_te_gpio)) {
		for (te_count = 0 ; te_count < te_max ; te_count++) {
			rc = ss_gpio_get_value(vdd, disp_te_gpio);
			if (rc == 1) {
				LCD_INFO(vdd, "LDI generate TE within = %d.%dms\n", te_count/100, te_count%100);
				break;
			}
			/* usleep suspends the calling thread whereas udelay is a
			 * busy wait. Here the value of te_gpio is checked in a loop of
			 * max count = 250. If this loop has to iterate multiple
			 * times before the te_gpio is 1, the calling thread will end
			 * up in suspend/wakeup sequence multiple times if usleep is
			 * used, which is an overhead. So use udelay instead of usleep.
			 */
			udelay(10);
		}
		if (te_count == te_max) {
			LCD_ERR(vdd, "LDI doesn't generate TE");
			SS_XLOG(0xbad);
			inc_dpui_u32_field(DPUI_KEY_QCT_NO_TE, 1);


#if IS_ENABLED(CONFIG_SEC_ABC) && !IS_ENABLED(CONFIG_SEC_FACTORY)
			if (vdd->ndx == PRIMARY_DISPLAY_NDX) {
				LCD_INFO(vdd, "send abc event..\n");
				sec_abc_send_event("MODULE=display@WARN=panel_main_no_te");
			} else {
				LCD_INFO(vdd, "send abc event..\n");
				sec_abc_send_event("MODULE=display@WARN=panel_sub_no_te");
			}
#endif
		}
	} else
		LCD_ERR(vdd, "disp_te_gpio is not valid\n");
	LCD_INFO(vdd, "============ end waiting for TE CTRL%d ============\n", vdd->ndx);
}

static const struct file_operations dyn_mipi_clk_ops = {
	.open = simple_open,
	.read = ss_read_dyn_mipi_clk_index_table,
};

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int ss_panel_debugfs_init(struct samsung_display_driver_data *vdd)
{
	struct samsung_display_debug_data *debug_data;
	struct samsung_display_driver_data *vdd_secondary =
					ss_get_vdd(SECONDARY_DISPLAY_NDX);
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "vdd NULL error\n");
		return -ENODEV;
	}

	LCD_INFO(vdd, "init display debugfs\n");

	debug_data = vdd->debug_data;

	/* Root directory for display driver */
	debug_data->root = debugfs_create_dir("display_driver", NULL);
	if (IS_ERR_OR_NULL(debug_data->root)) {
		LCD_INFO(vdd, "debugfs_create_dir failed, error %ld(line:%d)\n",
		       PTR_ERR(debug_data->root), __LINE__);
		ret = -ENODEV;
		goto fail_alloc;
	}

	/* Directory for dump */
	debug_data->dump = debugfs_create_dir("dump", debug_data->root);
	if (IS_ERR_OR_NULL(debug_data->dump)) {
		LCD_INFO(vdd, "debugfs_create_dir failed, error %ld(line:%d)\n",
		       PTR_ERR(debug_data->dump), __LINE__);
		ret = -ENODEV;
		goto fail;
	}

	/* Directory for hw_info */
	debug_data->hw_info = debugfs_create_dir("hw_info", debug_data->root);
	if (IS_ERR_OR_NULL(debug_data->root)) {
		LCD_INFO(vdd, "debugfs_create_dir failed, error %ld(line:%d)\n",
		       PTR_ERR(debug_data->root), __LINE__);
		ret = -ENODEV;
		goto fail;
	}

	/* Directory for display_status */
	debug_data->display_status = debugfs_create_dir("display_status",
					debug_data->root);
	if (IS_ERR_OR_NULL(debug_data->display_status)) {
		LCD_INFO(vdd, "debugfs_create_dir failed, error %ld(line:%d)\n",
		       PTR_ERR(debug_data->root), __LINE__);
		ret = -ENODEV;
		goto fail;
	}

	/* Directory for display LTP */
	debug_data->display_ltp = debugfs_create_dir("display_ltp",
					debug_data->root);
	if (IS_ERR_OR_NULL(debug_data->display_status)) {
		LCD_INFO(vdd, "debugfs_create_dir failed, error %ld(line:%d)\n",
		       PTR_ERR(debug_data->root), __LINE__);
		ret = -ENODEV;
		goto fail;
	}

	/*
	 * Create file on debugfs of display_driver
	 */

	/* Create file on debugfs on dump */
	debugfs_create_file("xlog_dump", 0644, debug_data->dump,
		vdd, &xlog_dump_ops);
	debugfs_create_bool("print_all", 0600, debug_data->dump,
		&debug_data->print_all);
	debugfs_create_bool("print_cmds", 0600, debug_data->dump,
		&debug_data->print_cmds);
	debugfs_create_bool("print_vsync", 0600, debug_data->dump,
		&debug_data->print_vsync);
	debugfs_create_bool("print_frame", 0600, debug_data->dump,
		&debug_data->print_frame);
	debugfs_create_bool("panic_on_pptimeout", 0600, debug_data->dump,
		&debug_data->panic_on_pptimeout);

	/* Create file on debugfs on display_status */
	debugfs_create_u32("panel_attach_status", 0600,
		debug_data->display_status, (u32 *)&vdd->panel_attach_status);

	debugfs_create_u32("panel1_attach_status", 0600,
		debug_data->display_status, (u32 *)&vdd_secondary->panel_attach_status);

	if (!IS_ERR_OR_NULL(debug_data->is_factory_mode))
		debugfs_create_bool("is_factory_mode", 0600, debug_data->root,
		debug_data->is_factory_mode);

	debugfs_create_bool("enable_pr_debug", 0600, debug_data->root,
			&enable_pr_debug);

	/* Create debugfs file on display LTP */
	debugfs_create_file("dyn_mipi_clk_index_table", 0644, debug_data->display_ltp,
		vdd, &dyn_mipi_clk_ops);

	/* Create file on debugfs on hw_info */
	/* TBD */

	return 0;

fail:
	debugfs_remove_recursive(vdd->debug_data->root);

fail_alloc:
	kfree(vdd->debug_data);
	LCD_INFO(vdd, "Fail to create files for debugfs(ret=%d)\n", ret);

	return ret;
}
#endif

#if IS_ENABLED(CONFIG_SEC_DEBUG)
static bool ss_read_debug_partition(struct lcd_debug_t *value)
{
	return read_debug_partition(debug_index_lcd_debug_info, (void *)value);
}

static bool ss_write_debug_partition(struct lcd_debug_t *value)
{
	return write_debug_partition(debug_index_lcd_debug_info, (void *)value);
}

void ss_inc_ftout_debug(const char *name)
{
	struct lcd_debug_t lcd_debug;

	memset(&lcd_debug, 0, sizeof(struct lcd_debug_t));
	ss_read_debug_partition(&lcd_debug);
	lcd_debug.ftout.count += 1;
	strlcpy(lcd_debug.ftout.name, name, MAX_FTOUT_NAME);
	ss_write_debug_partition(&lcd_debug);
}

static int dpci_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct samsung_display_driver_data *vdd = container_of(self,
			struct samsung_display_driver_data, dpci_notif);
	ssize_t len = 0;
	char tbuf[SS_XLOG_DPCI_LENGTH] = {0,};
	struct lcd_debug_t lcd_debug;

	/* 1. Read */
	ss_read_debug_partition(&lcd_debug);
	lcd_debug.ftout.name[sizeof(lcd_debug.ftout.name) - 1] = '\0';
	LCD_INFO(vdd, "Read Result FTOUT_CNT=%d, FTOUT_NAME=%s\n", lcd_debug.ftout.count, lcd_debug.ftout.name);

	/* 2. Make String */
	if (lcd_debug.ftout.count) {
		len += scnprintf((tbuf + len), (SS_XLOG_DPCI_LENGTH - len),
			"FTOUT CNT=%d ", lcd_debug.ftout.count);
		len += scnprintf((tbuf + len), (SS_XLOG_DPCI_LENGTH - len),
			"NAME=%s ", lcd_debug.ftout.name);
	}

	/* 3. Info Clear */
	memset((void *)&lcd_debug, 0, sizeof(struct lcd_debug_t));
	ss_write_debug_partition(&lcd_debug);
	set_dpui_field(DPUI_KEY_QCT_SSLOG, tbuf, len);

	return 0;
}

static int ss_register_dpci(struct samsung_display_driver_data *vdd)
{
	int ret;
	memset(&vdd->dpci_notif, 0,
			sizeof(vdd->dpci_notif));
	vdd->dpci_notif.notifier_call = dpci_notifier_callback;

	ret = dpui_logging_register(&vdd->dpci_notif, DPUI_TYPE_CTRL);
	return ret;
}
#endif

/* TODO: do not call clk->core->enable_count directly, then remove below struct clk.
 * Clock core driver doesn't open API to get enable_count, for now..
 */
struct clk_core {
	const char              *name;
	const struct clk_ops    *ops;
	struct clk_hw           *hw;
	struct module           *owner;
	struct device           *dev;
	struct device_node      *of_node;
	struct clk_core         *parent;
	struct clk_parent_map   *parents;
	u8                      num_parents;
	u8                      new_parent_index;
	unsigned long           rate;
	unsigned long           req_rate;
	unsigned long           new_rate;
	struct clk_core         *new_parent;
	struct clk_core         *new_child;
	unsigned long           flags;
	bool                    orphan;
	bool                    rpm_enabled;
	bool                    need_sync;
	bool                    boot_enabled;
	unsigned int            enable_count;
	unsigned int            prepare_count;
	unsigned int            protect_count;
};

struct clk {
	struct clk_core *core;
};

static char *dct_clk_name[MAX_DISPLAY_NDX][MAX_DCT_TAG] = {
	{
		[DCT_TAG_LINK_CLK] =  "disp_cc_mdss_byte0_clk",
		[DCT_TAG_MDP_CORE_CLK] =  "disp_cc_mdss_mdp_clk",
		[DCT_TAG_RSCC_VSYNC_CLK] =  "disp_cc_mdss_rscc_vsync_clk",
	},
	{
		[DCT_TAG_LINK_CLK] =  "disp_cc_mdss_byte1_clk",
		[DCT_TAG_MDP_CORE_CLK] =  "disp_cc_mdss_mdp1_clk",
		[DCT_TAG_RSCC_VSYNC_CLK] =  "none",
	},
};

static struct dct_info *ss_dct_get_info(struct samsung_display_driver_data *vdd,
			enum DCT_TAG tag, size_t *funcs, int count, u32 hash)
{
	struct dct_info *info;
	int i;

	if (!vdd || !vdd->debug_data) {
		LCD_ERR_NOVDD("invalid %s\n", !vdd ? "vdd" : "debug_data");
		return NULL;
	}

	hash_for_each_possible(vdd->debug_data->dct_hashtable[tag], info, node, hash) {
		if (info->hash != hash || info->count != count)
			continue;

		for (i = 0; i < count; i++)
			if (info->call_funcs[i] != funcs[i])
				break;

		if (i == count)
			return info;
	}

	return NULL;
}

static struct dct_info *ss_dct_add_info(struct samsung_display_driver_data *vdd,
			enum DCT_TAG tag, size_t *funcs, int count, u32 hash)
{
	struct dct_info *info;
	int i;

	if (!vdd || !vdd->debug_data) {
		LCD_ERR_NOVDD("invalid %s\n", !vdd ? "vdd" : "debug_data");
		return NULL;
	}

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		LCD_ERR_NOVDD("fail to allocate info\n");
		return NULL;
	}

	info->call_funcs = kzalloc(count * sizeof(size_t), GFP_KERNEL);
	if (!info->call_funcs) {
		LCD_ERR_NOVDD("fail to allocate call_funcs\n");
		kfree(info);
		return NULL;
	}

	for (i = 0; i < count; i++)
		info->call_funcs[i] = funcs[i];

	info->hash = hash;
	info->count = count;
	INIT_HLIST_NODE(&info->node);

	hash_add(vdd->debug_data->dct_hashtable[tag], &info->node, info->hash);

	atomic_set(&info->refcount, 0);

	return info;
}

#define DCT_SEND_CMD_FUNC_SIZE	(0x400)

#define DCT_FUNC_END_CONDITION(addr, count, max, start, end) \
	(!(addr) || (count) > (max) || ((addr) > (start) && (addr) < (end)))

static inline int ss_dct_fill_funcs(size_t *funcs, int max_count)
{
	size_t send_cmd_start = (size_t)ss_send_cmd;
	size_t send_cmd_end = send_cmd_start + DCT_SEND_CMD_FUNC_SIZE;
	int count = -1;

	funcs[++count] = (size_t)__builtin_return_address(0);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(1);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(2);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(3);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(4);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(5);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(6);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(7);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(8);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(9);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(10);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(11);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(12);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(13);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(14);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	funcs[++count] = (size_t)__builtin_return_address(15);
	if (DCT_FUNC_END_CONDITION(funcs[count], count, max_count, send_cmd_start, send_cmd_end))
		return count;

	return count + 1;
}

int ss_dct_update_ref(u32 ndx, u32 tag, u32 enable)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(ndx);
	struct dct_info *info;
	size_t funcs[MAX_CALLFUNC_NUM];
	int count;
	u32 hash = 5381;
	int i;

	if (!vdd) {
		LCD_ERR_NOVDD("invalid vdd, ndx: %d\n", ndx);
		return -ENODEV;
	}

	if (!vdd->debug_data) {
		LCD_WARN(vdd, "vdd debug is not initialized\n");
		return -EINVAL;
	}

	if (enable >= MAX_DCT_ENABLE || tag >= MAX_DCT_TAG) {
		LCD_ERR_NOVDD("invalid input: enable: %d, tag: %d\n", enable, tag);
		return -EINVAL;
	}

	if (!vdd->debug_data->is_dcp_enabled) {
		LCD_DEBUG(vdd, "dcp is disabled\n");
		return -EPERM;
	}

	count = ss_dct_fill_funcs(funcs, tag == DCT_TAG_LINK_CLK ? CALLFUNC_NUM_DISP : MAX_CALLFUNC_NUM);

	for (i = 0; i < count; i++)
		hash += (u32)funcs[i];

	info = ss_dct_get_info(vdd, tag, funcs, count, hash);
	if (!info) {
		info = ss_dct_add_info(vdd, tag, funcs, count, hash);
		if (!info) {
			LCD_ERR(vdd, "fail to add new info\n");
			return -ENOMEM;
		}
	}

	info->time = ktime_get();

	if (enable == DCT_ENABLE)
		atomic_inc(&info->refcount);
	else
		atomic_dec(&info->refcount);

	return 0;
}

static struct clk *dct_clk[MAX_DISPLAY_NDX][MAX_DCT_TAG];

void ss_dct_update_ref_for_dss(struct clk *clk, u32 enable)
{
	u32 ndx;

	if (!clk)
		return;

	for (ndx = 0; ndx < MAX_DISPLAY_NDX; ndx++) {
		if (clk == dct_clk[ndx][DCT_TAG_MDP_CORE_CLK])
			ss_dct_update_ref(ndx, DCT_TAG_MDP_CORE_CLK, enable);
		else if (clk == dct_clk[ndx][DCT_TAG_RSCC_VSYNC_CLK])
			ss_dct_update_ref(ndx, DCT_TAG_RSCC_VSYNC_CLK, enable);
	}
}

int ss_dct_update_clk(u32 ndx, u32 tag, struct clk *clk)
{
	if (ndx >= MAX_DISPLAY_NDX || tag >= MAX_DCT_TAG || !clk) {
		LCD_ERR_NOVDD("ndx: %d, tag: %d, clk: %d\n", ndx, tag, !!clk);
		return -EINVAL;
	}

	dct_clk[ndx][tag] = clk;

	return 0;
}

void ss_dct_update_clk_dss(struct clk *clk)
{
	const char *name = __clk_get_name(clk);
	int ndx, tag;

	if (!name)
		return;

	for (ndx = 0; ndx < MAX_DISPLAY_NDX; ndx++) {
		for (tag = 0; tag < MAX_DCT_TAG; tag++) {
			if (!strcmp(name, dct_clk_name[ndx][tag])) {
				ss_dct_update_clk(ndx, tag, clk);
				break;
			}
		}
	}
}

bool ss_dct_is_clk_on(struct samsung_display_driver_data *vdd)
{
	int tag;

	if (!vdd || !vdd->debug_data) {
		LCD_ERR(vdd, "%s\n", !vdd ? "null vdd" : "null debug_data");
		return false;
	}

	if (!vdd->debug_data->is_dcp_enabled) {
		LCD_DEBUG(vdd, "dcp disabled\n");
		return false;
	}

	for (tag = 0; tag < MAX_DCT_TAG; tag++)
		if (__clk_is_enabled(vdd->debug_data->dct_clk[tag]))
			return true;

	return false;
}

int ss_dct_dump_all_info(struct samsung_display_driver_data *vdd)
{
	struct dct_info *info;
	struct clk *clk;
	int i, j, tag;
	int num;
	int ref;
	int sum;

	if (!vdd || !vdd->debug_data) {
		LCD_ERR(vdd, "%s\n", !vdd ? "null vdd" : "null debug_data");
		return -ENODEV;
	}

	if (!vdd->debug_data->is_dcp_enabled) {
		LCD_DEBUG(vdd, "dcp disabled\n");
		return -EPERM;
	}

	for (tag = 0; tag < MAX_DCT_TAG; tag++) {
		clk = vdd->debug_data->dct_clk[tag];
		LCD_INFO(vdd, "clk[%d] name: [%s], enable_count: %d, init_enable_count: %d\n",
				tag, dct_clk_name[vdd->ndx][tag],
				(clk && clk->core) ? clk->core->enable_count : -1,
				vdd->debug_data->init_enable_count[tag]);

		sum = vdd->debug_data->init_enable_count[tag];
		num = 0;
		hash_for_each(vdd->debug_data->dct_hashtable[tag], i, info, node) {
			ref = atomic_read(&info->refcount);
			sum += ref;
			LCD_INFO(vdd, "[%3d] refcount: %3d, refsum: %3d, time: %llu, by [ ",
					num++, ref, sum, info->time);

			for (j = 0; j < info->count; j++)
				pr_cont("(%d)%pS, ", j, (void *)info->call_funcs[j]);
			pr_cont("]\n");
		}
	}

	LCD_INFO(vdd, "Dump Display Clock Tracer info ---\n");

	return 0;
}

static int ss_dct_init(struct samsung_display_driver_data *vdd)
{
	struct device_node *np;
	int tag;

	if (!vdd || !vdd->debug_data) {
		LCD_ERR_NOVDD("invalid vdd/debug_data\n");
		return -EINVAL;
	}

	np = ss_get_panel_of(vdd);
	vdd->debug_data->support_dct = of_property_read_bool(np, "samsung,support_dct");
	LCD_INFO(vdd, "dct %s\n", vdd->debug_data->support_dct ? "enabled" : "disabled");

	if (!vdd->debug_data->support_dct)
		return 0;

	for (tag = 0; tag < MAX_DCT_TAG; tag++) {
		struct clk *clk = dct_clk[vdd->ndx][tag];

		vdd->debug_data->dct_clk[tag] = clk;
		vdd->debug_data->init_enable_count[tag] = (clk && clk->core) ? clk->core->enable_count - 1 : 0;

		hash_init(vdd->debug_data->dct_hashtable[tag]);

		LCD_INFO(vdd, "[%d] clk name: [%s], init_enable_count: %d\n",
				tag, dct_clk_name[vdd->ndx][tag],
				vdd->debug_data->init_enable_count[tag]);
	}

	vdd->debug_data->is_dcp_enabled = true;

	LCD_INFO(vdd, "dcp init done\n");

	return 0;
}

int ss_panel_debug_init(struct samsung_display_driver_data *vdd)
{

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "vdd NULL error\n");
		return -ENODEV;
	}

	if (vdd->ndx != COMMON_DISPLAY_NDX) {
		struct samsung_display_driver_data *vdd_common = ss_get_vdd(COMMON_DISPLAY_NDX);
		vdd->debug_data = vdd_common->debug_data;
		LCD_INFO(vdd, "vdd->ndx = %d Skip.. creat debugfs for only primary vdd & copy those from common\n",
			vdd->ndx);
		return 0;
	}

	/*
	 * The debugfs must be init one time
	 * in case of dual dsi, this function will be called twice
	 */
	if (vdd->debug_data) {
		LCD_INFO(vdd, "try to initialize debug_data again...\n");
		return 0;
	}

	vdd->debug_data = kzalloc(sizeof(struct samsung_display_debug_data),
		GFP_KERNEL);
	if (IS_ERR_OR_NULL(vdd->debug_data)) {
		LCD_INFO(vdd, "no memory to create display debug data\n");
		return -ENOMEM;
	}

	/* INIT debug data */
	vdd->debug_data->is_factory_mode = &vdd->is_factory_mode;

	/*
	 * panic_on_pptimeout default value is false
	 * if you want to enable panic for specific project
	 * please change the value on your panel file.
	 * if you want to enable panic for all project
	 * please change the value here.
	 */
	vdd->debug_data->panic_on_pptimeout = false;

#if IS_ENABLED(CONFIG_DEBUG_FS)
	ss_panel_debugfs_init(vdd);
#endif

#if IS_ENABLED(CONFIG_SEC_DEBUG)
	ss_register_dpci(vdd);
#endif

	ss_disp_dbg_info_misc_register();

	/* work thread: dump timestamps to debug tearing */
	vdd->dbg_tear_info.dbg_tear_wq = create_singlethread_workqueue("debug_tear_wq");
	if (!vdd->dbg_tear_info.dbg_tear_wq)
		LCD_ERR(vdd, "failed to create read dbg_tear workqueue\n");
	else
		INIT_DELAYED_WORK(&vdd->dbg_tear_info.dbg_tear_work, ss_dbg_tear_work);


	ss_dct_init(vdd);

	return 0;
}


/**
 *	sde & sde-rotator smmu debug.
 *	Because of preemption & performance issue,
 *	sde & sde-rotator use unique spin_lock.
 */
int ss_smmu_debug_init(struct samsung_display_driver_data *vdd)
{
	int cnt;
	int ret = 0;

	/* This debug is available by sde_debug enabled condition */
#if IS_ENABLED(CONFIG_SEC_DEBUG)
	if (!sec_debug_is_enabled()) {
		LCD_INFO(vdd, "sec_debug_is_enabled : %d\n", sec_debug_is_enabled());
		goto init_fail;
	}
#else
	goto init_fail;
#endif

	/* Create KMEM_CACHE slab */
	if (IS_ERR_OR_NULL(vdd->ss_debug_smmu_cache)) {
		vdd->ss_debug_smmu_cache = KMEM_CACHE(ss_smmu_logging, 0);

		if (IS_ERR_OR_NULL(vdd->ss_debug_smmu_cache)) {
			LCD_INFO(vdd, "ss_debug_smmu_cache is not created\n");
			goto init_fail;
		}
	}

	for (cnt = 0; cnt < SMMU_MAX_DEBUG; cnt++) {
		if (!vdd->ss_debug_smmu[cnt].init_done) {
			spin_lock_init(&vdd->ss_debug_smmu[cnt].lock);
			INIT_LIST_HEAD(&vdd->ss_debug_smmu[cnt].list);
			vdd->ss_debug_smmu[cnt].init_done = true;
		}
	}

	return ret;

init_fail:
	for (cnt = 0; cnt < SMMU_MAX_DEBUG; cnt++)
		vdd->ss_debug_smmu[cnt].init_done = false;

	return -EPERM;
}

void ss_smmu_debug_map(enum ss_smmu_type type, struct sg_table *table)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(COMMON_DISPLAY_NDX);

	spinlock_t *smmu_lock = NULL;
	struct list_head *smmu_list = NULL;
	struct ss_smmu_logging *smmu_debug = NULL;

	if (IS_ERR_OR_NULL(vdd))
		return;

	if (type >= SMMU_MAX_DEBUG || !vdd->ss_debug_smmu[type].init_done) {
		LCD_INFO(vdd, "type : %d init_done : %d\n", type,
				(type < SMMU_MAX_DEBUG) ?
				vdd->ss_debug_smmu[type].init_done : -1);
		return;
	}

	if (IS_ERR_OR_NULL(vdd->ss_debug_smmu_cache)) {
		LCD_INFO(vdd, "ss_debug_smmu_cache is not created\n");
		return;
	}

	smmu_lock = &vdd->ss_debug_smmu[type].lock;
	smmu_list = &vdd->ss_debug_smmu[type].list;

	smmu_debug = kmem_cache_alloc(vdd->ss_debug_smmu_cache, GFP_KERNEL | __GFP_ZERO);

	spin_lock(smmu_lock);

	if (!IS_ERR_OR_NULL(smmu_debug)) {
		smmu_debug->time = ktime_get();
		smmu_debug->table = table;
		INIT_LIST_HEAD(&smmu_debug->list);
		list_add(&smmu_debug->list, smmu_list);

		LCD_DEBUG(vdd, "addr : 0x%llx size : 0x%x \n", table->sgl->dma_address, table->sgl->dma_length);
	}

	spin_unlock(smmu_lock);
}

void ss_smmu_debug_unmap(enum ss_smmu_type type, struct sg_table *table)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(COMMON_DISPLAY_NDX);

	spinlock_t *smmu_lock = NULL;
	struct list_head *smmu_list = NULL;
	struct ss_smmu_logging *smmu_debug = NULL;

	if (IS_ERR_OR_NULL(vdd))
		return;

	if (type >= SMMU_MAX_DEBUG || !vdd->ss_debug_smmu[type].init_done) {
		LCD_INFO(vdd, "type : %d init_done : %d\n", type,
				(type < SMMU_MAX_DEBUG) ?
				vdd->ss_debug_smmu[type].init_done : -1);

		return;
	}

	smmu_lock = &vdd->ss_debug_smmu[type].lock;
	smmu_list = &vdd->ss_debug_smmu[type].list;

	spin_lock(smmu_lock);

	list_for_each_entry(smmu_debug, smmu_list, list) {
		if (smmu_debug->table == table) {
			LCD_DEBUG(vdd, "addr : 0x%llx size : 0x%x \n", table->sgl->dma_address, table->sgl->dma_length);
			list_del(&smmu_debug->list);
			kmem_cache_free(vdd->ss_debug_smmu_cache, smmu_debug);
			break;
		}
	}

	spin_unlock(smmu_lock);
}

#if IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
void ss_smmu_debug_log(void)
{
	pr_debug("%s : nothing to do\n", __func__);
}
#else
void ss_smmu_debug_log(void)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(COMMON_DISPLAY_NDX);

	enum ss_smmu_type type = SMMU_MAX_DEBUG;
	spinlock_t *smmu_lock = NULL;
	struct list_head *smmu_list = NULL;
	struct ss_smmu_logging *smmu_debug = NULL;

	if (IS_ERR_OR_NULL(vdd))
		return;

	for (type = SMMU_RT_DISPLAY_DEBUG; type < SMMU_MAX_DEBUG; type++) {

		if (vdd->ss_debug_smmu[type].init_done != true)
			continue;

		smmu_lock = &vdd->ss_debug_smmu[type].lock;
		smmu_list = &vdd->ss_debug_smmu[type].list;

		spin_lock(smmu_lock);

		list_for_each_entry(smmu_debug, smmu_list, list) {
#if IS_ENABLED(CONFIG_NEED_SG_DMA_LENGTH)
			LCD_INFO(vdd, "type : %s time : %lld.%6lld dma_address : 0x%llx dma_length : %d\n",
				type == SMMU_RT_DISPLAY_DEBUG ? "SMMU_RT_DISPLAY_DEBUG" : "SMMU_NRT_ROTATOR_DEBUG",
				smmu_debug->time / NSEC_PER_SEC, smmu_debug->time - ((smmu_debug->time / NSEC_PER_SEC) * NSEC_PER_SEC),
				smmu_debug->table->sgl->dma_address,
				smmu_debug->table->sgl->dma_length);
#else
			LCD_INFO(vdd, "type : %s time : %lld.%6lld dma_address : 0x%llx\n",
				type == SMMU_RT_DISPLAY_DEBUG ? "SMMU_RT_DISPLAY_DEBUG" : "SMMU_NRT_ROTATOR_DEBUG",
				smmu_debug->time / NSEC_PER_SEC, smmu_debug->time - ((smmu_debug->time / NSEC_PER_SEC) * NSEC_PER_SEC),
				smmu_debug->table->sgl->dma_address);
#endif
		}

		spin_unlock(smmu_lock);
	}
}
#endif

static DEFINE_SPINLOCK(image_logging_lock);
#define MAX_IMAGE_LOGGING 256
struct ss_image_logging image_logging[MAX_IMAGE_LOGGING];
static int image_logging_index = 0;

void ss_image_logging_update(uint32_t plane_addr, int width, int height, int src_format)
{
#if IS_ENABLED(CONFIG_SEC_DEBUG)
	if (!sec_debug_is_enabled()) {
		return;
	}
#else
	return;
#endif

	spin_lock(&image_logging_lock);

	image_logging[image_logging_index].dma_address = plane_addr;
	image_logging[image_logging_index].src_width = width;
	image_logging[image_logging_index].src_height = height;
	image_logging[image_logging_index].src_format = src_format;

	image_logging_index++;
	image_logging_index %= MAX_IMAGE_LOGGING;

	spin_unlock(&image_logging_lock);
}

void ss_xlog_vrr_change_in_drm_ioctl(int vrefresh, int sot_hs_mode, int phs_mode)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(PRIMARY_DISPLAY_NDX);

	if (vdd->vrr.adjusted_refresh_rate != 0 &&
			(vdd->vrr.adjusted_refresh_rate != vrefresh ||
			 vdd->vrr.adjusted_sot_hs_mode != sot_hs_mode ||
			 vdd->vrr.adjusted_phs_mode != phs_mode)) {
		LCD_INFO(vdd, "switch mode: drm_ioctl: %d%s -> %d%s\n",
				vdd->vrr.adjusted_refresh_rate,
				vdd->vrr.adjusted_sot_hs_mode ?
				(vdd->vrr.adjusted_phs_mode ? "PHS" : "HS") : "NM",
				vrefresh,
				sot_hs_mode ? "HS" : "NM");
		SS_XLOG(vdd->vrr.adjusted_refresh_rate,
				vdd->vrr.adjusted_sot_hs_mode,
				vdd->vrr.adjusted_phs_mode,
				vrefresh, sot_hs_mode, phs_mode);
	}
}

bool ss_is_panel_dead(int ndx)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(ndx);
	return vdd ? vdd->panel_dead : false;
}
