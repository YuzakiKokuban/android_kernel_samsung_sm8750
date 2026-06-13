/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * NFC logger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
#include <linux/sched/clock.h>
#else
#include <linux/sched.h>
#endif

#ifdef CONFIG_SEC_NFC_LOGGER_ADD_ACPM_LOG
#include <linux/io.h>
#include <soc/samsung/acpm_ipc_ctrl.h>
#include <soc/samsung/debug-snapshot.h>
#endif

#include "nfc_logger.h"

#ifdef CONFIG_NFC_LOGGER_LOWMEM
#define BUF_SIZE	SZ_64K
#else
#define BUF_SIZE	SZ_128K
#endif
#define BOOT_LOG_SIZE	2400
#define MAX_STR_LEN	160
#define PROC_FILE_NAME	"nfclog"
#define LOG_PREFIX	"sec-nfc"
#define PRINT_DATE_FREQ	20

static char nfc_log_buf[BUF_SIZE];
static unsigned int g_curpos;
static int is_nfc_logger_init;
static int is_buf_full;
static int g_log_max_count = -1;
static void (*print_nfc_status)(void);
struct proc_dir_entry *g_entry;

/* set max log count, if count is -1, no limit */
void nfc_logger_set_max_count(int count)
{
	g_log_max_count = count;
}

void nfc_logger_get_date_time(char *date_time, int size)
{
	struct timespec64 ts;
	struct tm tm;
	unsigned long sec;
	int len = 0;

	ktime_get_real_ts64(&ts);
	sec = ts.tv_sec - (sys_tz.tz_minuteswest * 60);
	time64_to_tm(sec, 0, &tm);
	len = snprintf(date_time, size, "%02d-%02d %02d:%02d:%02d.%03lu", tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);

#ifdef CONFIG_SEC_NFC_LOGGER_ADD_ACPM_LOG
	snprintf(date_time + len, size - len, ", rtc: %u", date_time,
			nfc_logger_acpm_get_rtc_time());
#endif
}

void nfc_logger_print(char *fmt, ...)
{
	int len;
	va_list args;
	char buf[MAX_STR_LEN] = {0, };
	u64 time;
	u32 nsec;
	unsigned int curpos;
	static unsigned int log_count = PRINT_DATE_FREQ;
	char date_time[64] = {0, };
	bool date_time_print = false;

	if (!is_nfc_logger_init)
		return;

	if (g_log_max_count == 0)
		return;
	else if (g_log_max_count > 0)
		g_log_max_count--;

	if (--log_count == 0) {
		nfc_logger_get_date_time(date_time, sizeof(date_time));
		log_count = PRINT_DATE_FREQ;
		date_time_print = true;
	}

	time = local_clock();
	nsec = do_div(time, 1000000000);
	if (g_curpos < BOOT_LOG_SIZE)
		len = snprintf(buf, sizeof(buf), "[B%4llu.%06u] ", time, nsec / 1000);
	else
		len = snprintf(buf, sizeof(buf), "[%5llu.%06u] ", time, nsec / 1000);

	if (date_time_print)
		len += snprintf(buf + len, sizeof(buf) - len, "[%s] ", date_time);

	va_start(args, fmt);
	len += vsnprintf(buf + len, MAX_STR_LEN - len, fmt, args);
	va_end(args);

	if (len > MAX_STR_LEN)
		len = MAX_STR_LEN;

	curpos = g_curpos;
	if (curpos + len >= BUF_SIZE) {
		g_curpos = curpos = BOOT_LOG_SIZE;
		is_buf_full = 1;
	}
	memcpy(nfc_log_buf + curpos, buf, len);
	g_curpos += len;
}

void nfc_logger_register_nfc_stauts_func(void (*print_status_callback)(void))
{
	print_nfc_status = print_status_callback;
}

void nfc_print_hex_dump(void *buf, void *pref, size_t size)
{
	uint8_t *ptr = buf;
	size_t i;
	char tmp[128] = {0x0, };
	char *ptmp = tmp;
	int len;

	if (!is_nfc_logger_init)
		return;

	if (g_log_max_count == 0)
		return;
	else if (g_log_max_count > 0)
		g_log_max_count--;

	for (i = 0; i < size; i++) {
		len = snprintf(ptmp, 4, "%02x ", *ptr++);
		ptmp = ptmp + len;
		if (((i+1)%16) == 0) {
			nfc_logger_print("%s%s\n", pref, tmp);
			ptmp = tmp;
		}
	}

	if (i % 16) {
		len = ptmp - tmp;
		tmp[len] = 0x0;
		nfc_logger_print("%s%s\n", pref, tmp);
	}
}

static ssize_t nfc_logger_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;
	size_t size;
	unsigned int curpos = g_curpos;

	if (is_buf_full || BUF_SIZE <= curpos)
		size = BUF_SIZE;
	else
		size = (size_t)curpos;

	if (pos >= size)
		return 0;

	if (print_nfc_status && !pos)
		print_nfc_status();

	count = min(len, size);

	if ((pos + count) > size)
		count = size - pos;

	if (copy_to_user(buf, nfc_log_buf + pos, count))
		return -EFAULT;

	*offset += count;

	return count;
}

#if KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE
static const struct proc_ops nfc_logger_ops = {
	.proc_read = nfc_logger_read,
	.proc_lseek = default_llseek,
};
#else
static const struct file_operations nfc_logger_ops = {
	.owner = THIS_MODULE,
	.read = nfc_logger_read,
	.llseek = default_llseek,
};
#endif

int nfc_logger_init(void)
{
	struct proc_dir_entry *entry;

	if (is_nfc_logger_init)
		return 0;

	entry = proc_create(PROC_FILE_NAME, 0444, NULL, &nfc_logger_ops);
	if (!entry) {
		pr_err("%s: failed to create proc entry\n", __func__);
		return 0;
	}

	proc_set_size(entry, BUF_SIZE);
	is_nfc_logger_init = 1;
	nfc_logger_print("nfc logger init ok\n");

	g_entry = entry;

	return 0;
}

void nfc_logger_deinit(void)
{
	if (!g_entry)
		return;

	proc_remove(g_entry);
	g_entry = NULL;
}

#ifdef CONFIG_SEC_NFC_LOGGER_ADD_ACPM_LOG
static __iomem *g_rtc_reg;

u32 nfc_logger_acpm_get_rtc_time(void)
{
	u32 rtc = 0;

	if (g_rtc_reg)
		rtc = readl(g_rtc_reg);

	return rtc;
}

void nfc_logger_acpm_log_print(void)
{
	struct nfc_clk_req_log *acpm_log;
	int last_ptr, len, i;
	u32 rtc;

	if (!acpm_get_nfc_log_buf(&acpm_log, &last_ptr, &len)) {
		for (i = 0; i < len; i++) {
			rtc = nfc_logger_acpm_get_rtc_time();
			NFC_LOG_INFO("rtc[%u] - acpm[%2d][%d] %d\n",
				rtc, i, acpm_log[i].timestamp, acpm_log[i].is_on);
		}
	}
}

void nfc_logger_acpm_log_init(u32 rtc_addr)
{
	u32 rtc_reg_addr = CONFIG_SEC_NFC_LOGGER_RTC_REG_ADDR;

	if (rtc_addr)
		rtc_reg_addr = rtc_addr;

	if (rtc_reg_addr) {
		NFC_LOG_INFO("rtc: 0x%X\n", rtc_reg_addr);
		g_rtc_reg = ioremap(rtc_reg_addr, 0x4);
	}
}
#endif

