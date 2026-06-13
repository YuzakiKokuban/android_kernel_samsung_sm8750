/*
 * /drivers/rtc/power-on-alarm.c
 *
 *  Copyright (C) 2021 Samsung Electronics
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/time.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/alarmtimer.h>
#include <linux/ioctl.h>
#include <linux/sec_param.h>
#include <linux/rtc.h>
#include <linux/of_device.h>

#define PON_ALARM_KPARAM_MAGIC	0x41504153
#define PON_ALARM_START_POLL_TIME	(10LL * NSEC_PER_SEC) /* 10 sec */
#define PON_ALARM_BOOTING_TIME	(5 * 60)
#define PON_ALARM_POLL_TIME	(15 * 60)

#define BOOTALM_BIT_EN		0
#define BOOTALM_BIT_YEAR	1
#define BOOTALM_BIT_MONTH	5
#define BOOTALM_BIT_DAY		7
#define BOOTALM_BIT_HOUR	9
#define BOOTALM_BIT_MIN		11
#define BOOTALM_BIT_TOTAL	13

#define RTC_ALM_SEC_MASK 0x00000003
#define RTC_RESET_NUM 70
#define RTC_RESET_YEAR 50

#define ALARM_ENABLE_MULTIPLIER 1

#define PRIME_EPOCH_YEAR	1900

enum pon_alarm_state {
	PON_ALARM_DISTANT = 0,
	PON_ALARM_NEAR,
	PON_ALARM_EXPIRED,
	PON_ALARM_OVER,
	PON_ALARM_ERROR
};

struct pon_alarm_info {
	struct device *dev;
	struct rtc_device *rtcdev;
	struct rtc_wkalrm val;
	struct alarm check_poll;
	struct work_struct check_func;
	struct wakeup_source *ws;
	uint lpm_mode;
	unsigned char triggered;
};

struct alarm_timespec {
	char alarm[14];
};

#define ANDROID_ALARM_BASE_CMD(cmd)		(cmd & ~(_IOC(0, 0, 0xf0, 0)))
#define ANDROID_ALARM_SET_ALARM_BOOT	_IOW('a', 7, struct alarm_timespec)


extern void pmic_rtc_setalarm(struct rtc_wkalrm *alm);

static unsigned int __read_mostly rtcalarm;
static unsigned int __read_mostly lpcharge;
module_param(rtcalarm, uint, 0444);
module_param(lpcharge, uint, 0444);

struct pon_alarm_info pon_alarm;

static void pon_alarm_normalize(struct rtc_wkalrm *alarm)
{
	if (!alarm->enabled) {
		/* RTC reset + 50 years = 1580518864 = 0x5e34cdd0 */
		alarm->time.tm_year = RTC_RESET_NUM + RTC_RESET_YEAR;
		alarm->time.tm_mon = 1;
		alarm->time.tm_mday = 1;
		alarm->time.tm_hour = 1;
		alarm->time.tm_min = 1;
		alarm->time.tm_sec = 4;
	}
}

static void pon_alarm_set(struct rtc_wkalrm *alarm)
{
	time64_t secs_pwron;
	unsigned int rtc_alarm_params[3];
	int ret;

	memcpy(&pon_alarm.val, alarm, sizeof(struct rtc_wkalrm));
	pon_alarm_normalize(&pon_alarm.val);

	pr_info("%s: reserve pmic alarm\n", __func__);
	pmic_rtc_setalarm(&pon_alarm.val);

	secs_pwron = rtc_tm_to_time64(&pon_alarm.val.time);
	rtc_alarm_params[0] = PON_ALARM_KPARAM_MAGIC;
	rtc_alarm_params[1] = (unsigned int)pon_alarm.val.enabled;
	rtc_alarm_params[2] = (unsigned int)secs_pwron;
	ret = sec_set_param(param_index_sapa, rtc_alarm_params);
	pr_info("%s: ret=%d, enabled=%d, alarm=%u\n",
				__func__, ret, rtc_alarm_params[1], rtc_alarm_params[2]);
}

static char *pon_alarm_get_timeformat(struct rtc_time tm)
{
	static char time_str[20];

	snprintf(time_str, sizeof(time_str), "%4d-%02d-%02d %02d:%02d:%02d",
			tm.tm_year, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
	return time_str;
}

static void pon_get_alarm_time(struct rtc_wkalrm *alm, u8 *buf_ptr)
{
	/*
	 *	0|1234|56|78|90|12
	 *	1|2010|01|01|00|00
	 *	en|yyyy|mm|dd|hh|mm
	 */
	alm->time.tm_sec = 0;
	alm->time.tm_min = (buf_ptr[BOOTALM_BIT_MIN] - '0') * 10
					+ (buf_ptr[BOOTALM_BIT_MIN+1] - '0');
	alm->time.tm_hour = (buf_ptr[BOOTALM_BIT_HOUR] - '0') * 10
					+ (buf_ptr[BOOTALM_BIT_HOUR+1] - '0');
	alm->time.tm_mday = (buf_ptr[BOOTALM_BIT_DAY] - '0') * 10
					+ (buf_ptr[BOOTALM_BIT_DAY+1] - '0');
	alm->time.tm_mon  = (buf_ptr[BOOTALM_BIT_MONTH] - '0') * 10
					+ (buf_ptr[BOOTALM_BIT_MONTH+1] - '0');
	alm->time.tm_year = (buf_ptr[BOOTALM_BIT_YEAR] - '0') * 1000
					+ (buf_ptr[BOOTALM_BIT_YEAR+1] - '0') * 100
					+ (buf_ptr[BOOTALM_BIT_YEAR+2] - '0') * 10
					+ (buf_ptr[BOOTALM_BIT_YEAR+3] - '0');

	switch (*buf_ptr) {
	case '1':
		alm->enabled = 1;
		break;
	case '2':
		alm->enabled = 2;
		break;
	default:
		pr_warn("Invalid value for alarm enable flag: %c, setting to 0\n", *buf_ptr);
		alm->enabled = 0;
	}

	pr_info("%s: %s => tm(%d %s)\n", __func__, buf_ptr, alm->enabled, pon_alarm_get_timeformat(alm->time));
}

static void pon_read_kernel_time(struct timespec64 *ktm_ts, struct rtc_wkalrm *alm)
{
	struct rtc_time ktm_tm;

	/* read kernel time */
	ktime_get_real_ts64(ktm_ts);
	ktm_tm = rtc_ktime_to_tm(timespec64_to_ktime(*ktm_ts));
	pr_info("%s: <KTM > %s\n", __func__, pon_alarm_get_timeformat(ktm_tm));

	alm->time.tm_mon -= 1;
	alm->time.tm_year -= PRIME_EPOCH_YEAR;
	pr_info("%s: <ALRM> %s\n", __func__, pon_alarm_get_timeformat(alm->time));
}

static int pon_read_current_time(time64_t *rtc_sec, struct rtc_time *rtc_tm)
{
	int ret = 0;

	ret = rtc_read_time(pon_alarm.rtcdev, rtc_tm);
	if (ret) {
		pr_err("%s: rtc read failed, ret=%d\n", __func__, ret);
		return ret;
	}

	*rtc_sec = rtc_tm_to_time64(rtc_tm);
	pr_info("%s: <rtc > %s -> %lld\n", __func__, pon_alarm_get_timeformat(*rtc_tm), *rtc_sec);

	return ret;
}

static void pon_calculate_offset(struct timespec64 *delta, struct timespec64 ktm_ts, time64_t rtc_sec)
{
	set_normalized_timespec64(delta,
			(time64_t)ktm_ts.tv_sec - rtc_sec,
			(s64)ktm_ts.tv_nsec);
}

static time64_t set_rtc_alm_sec(struct timespec64 delta, struct rtc_wkalrm *alm)
{
	time64_t rtc_alm_sec = rtc_tm_to_time64(&alm->time) - delta.tv_sec;

	rtc_alm_sec = (rtc_alm_sec & ~RTC_ALM_SEC_MASK) | ((alm->enabled - 1) << (ALARM_ENABLE_MULTIPLIER));
	return rtc_alm_sec;
}

static void pon_convert_utc_to_rtc(struct timespec64 delta, struct rtc_wkalrm *alm)
{
	time64_t rtc_alm_sec = set_rtc_alm_sec(delta, alm);

	alm->enabled = 1;
	rtc_time64_to_tm(rtc_alm_sec, &alm->time);

	pr_info("%s: <alrm> %s -> %lld\n", __func__, pon_alarm_get_timeformat(alm->time), rtc_alm_sec);
}

static void pon_alarm_parse_data(char *alarm_data, struct rtc_wkalrm *alm)
{
	char buf_ptr[BOOTALM_BIT_TOTAL + 1] = {0,};
	struct rtc_time rtc_tm;
	struct timespec64 delta, ktm_ts;
	time64_t rtc_sec;
	int ret;

	alarm_data[BOOTALM_BIT_TOTAL] = '\0';
	strlcpy(buf_ptr, alarm_data, BOOTALM_BIT_TOTAL+1);

	pon_get_alarm_time(alm, buf_ptr);
	if (!alm->enabled) {
		pr_info("%s: pon alarm is not enabled!\n", __func__);
		return;
	}

	pon_read_kernel_time(&ktm_ts, alm);
	ret = pon_read_current_time(&rtc_sec, &rtc_tm);
	if (ret < 0) {
		pr_info("%s: pon read current time fail!\n", __func__);
		return;
	}

	pon_calculate_offset(&delta, ktm_ts, rtc_sec);

	pon_convert_utc_to_rtc(delta, alm);
}

static enum pon_alarm_state check_pon_alarm_state(time64_t rtc_secs, time64_t secs_pwron, time64_t *data)
{
	if (rtc_secs < secs_pwron) {
		if (secs_pwron - rtc_secs > PON_ALARM_POLL_TIME)
			return PON_ALARM_DISTANT;
		if (data)
			*data = secs_pwron - rtc_secs;
		return PON_ALARM_NEAR;
	} else if (rtc_secs <= secs_pwron + PON_ALARM_BOOTING_TIME) {
		if (data)
			*data = rtc_secs + 10;
		return PON_ALARM_EXPIRED;
	} else {
		return PON_ALARM_OVER;
	}
}

static int pon_alarm_check_state(time64_t *data)
{
	struct rtc_time tm;
	time64_t rtc_secs;
	time64_t secs_pwron;
	enum pon_alarm_state state = PON_ALARM_NEAR;
	int ret;

	ret = rtc_read_time(pon_alarm.rtcdev, &tm);
	if (ret) {
		pr_err("%s: rtc read failed.\n", __func__);
		return PON_ALARM_ERROR;
	}
	rtc_secs = rtc_tm_to_time64(&tm);
	secs_pwron = rtc_tm_to_time64(&pon_alarm.val.time);

	state = check_pon_alarm_state(rtc_secs, secs_pwron, data);

	pr_info("%s: rtc:%lld, alrm:%lld[%d]\n", __func__, rtc_secs, secs_pwron, state);
	return state;
}

static void pon_alarm_check_func(struct work_struct *work)
{
	struct pon_alarm_info *pon_info = container_of(work, struct pon_alarm_info, check_func);
	int res;
	time64_t remain;

	res = pon_alarm_check_state(&remain);
	if (res <= PON_ALARM_NEAR) {
		ktime_t ktime;

		if (res == PON_ALARM_DISTANT)
			remain = PON_ALARM_POLL_TIME;
		ktime = ns_to_ktime((u64)remain * NSEC_PER_SEC);
		alarm_start_relative(&pon_info->check_poll, ktime);
		pr_info("%s: next %lld s\n", __func__, remain);
	} else if (res == PON_ALARM_EXPIRED) {
		__pm_stay_awake(pon_info->ws);
		pon_info->triggered = 1;
	}
}

static enum alarmtimer_restart pon_alarm_check_callback(struct alarm *alarm, ktime_t now)
{
	struct pon_alarm_info *pon_info = container_of(alarm, struct pon_alarm_info, check_poll);

	schedule_work(&pon_info->check_func);
	return ALARMTIMER_NORESTART;
}


static int pon_alarm_open(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static ssize_t pon_alarm_read(struct file *file,
	char __user *buff, size_t count, loff_t *ppos)
{
	char trigger = (pon_alarm.triggered) ? '1' : '0';

	if (copy_to_user((void __user *)buff, &trigger, 1)) {
		pr_err("%s: read failed.\n", __func__);
		return -EFAULT;
	}
	pr_info("%s: trigger=%c\n", __func__, trigger);
	return 1;
}

static long pon_alarm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct alarm_timespec data;
	struct rtc_wkalrm alm;

	pr_info("%s: cmd=%08x\n", __func__, cmd);

	switch (ANDROID_ALARM_BASE_CMD(cmd)) {
	case ANDROID_ALARM_SET_ALARM_BOOT:
		if (copy_from_user(data.alarm, (void __user *)arg, 14)) {
			ret = -EFAULT;
			pr_err("%s: SET ret=%d\n", __func__, ret);
			return ret;
		}
		pon_alarm_parse_data(data.alarm, &alm);
		pon_alarm_set(&alm);
		break;
	}

	return ret;
}

static int pon_alarm_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations pon_alarm_fops = {
	.owner = THIS_MODULE,
	.open = pon_alarm_open,
	.read = pon_alarm_read,
	.unlocked_ioctl = pon_alarm_ioctl,
	.release = pon_alarm_release,
};

static struct miscdevice pon_alarm_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "power_on_alarm",
	.fops = &pon_alarm_fops,
};


static int __init pon_alarm_init(void)
{
	struct rtc_device *rtcdev;
	int ret;

	rtcdev = alarmtimer_get_rtcdev();
	if (!rtcdev) {
		pr_err("%s: no rtcdev (defer)\n", __func__);
		return -ENODEV;
	}

	ret = misc_register(&pon_alarm_device);
	if (ret) {
		pr_err("%s: ret=%d, fail to register misc device\n", __func__, ret);
		return ret;
	}

	pr_info("%s: rtcalarm=%u, lpcharge=%u\n", __func__, rtcalarm, lpcharge);
	pon_alarm.rtcdev = rtcdev;
	pon_alarm.lpm_mode = lpcharge;
	pon_alarm.triggered = 0;
	pon_alarm.val.enabled = (rtcalarm) ? 1 : 0;
	rtc_time64_to_tm((time64_t)rtcalarm, &pon_alarm.val.time);

	if (pon_alarm.lpm_mode && pon_alarm.val.enabled) {
		pr_info("%s: reserve pmic alarm\n", __func__);
		pmic_rtc_setalarm(&pon_alarm.val);
		pon_alarm.ws = wakeup_source_register(pon_alarm.dev, "PON_ALARM");

		alarm_init(&pon_alarm.check_poll, ALARM_REALTIME, pon_alarm_check_callback);
		INIT_WORK(&pon_alarm.check_func, pon_alarm_check_func);
		alarm_start_relative(&pon_alarm.check_poll, ns_to_ktime(PON_ALARM_START_POLL_TIME));
	}

	pr_info("%s: done\n", __func__);
	return 0;
}

static void  __exit pon_alarm_exit(void)
{
	misc_deregister(&pon_alarm_device);
	if (pon_alarm.lpm_mode && pon_alarm.val.enabled) {
		pr_info("%s: clear for lpm_mode\n", __func__);
		cancel_work_sync(&pon_alarm.check_func);
		alarm_cancel(&pon_alarm.check_poll);
		wakeup_source_unregister(pon_alarm.ws);
	}
}

module_init(pon_alarm_init);
module_exit(pon_alarm_exit);

MODULE_ALIAS("platform:sec_pon_alarm");
MODULE_DESCRIPTION("SEC_PON_ALARM driver");
MODULE_LICENSE("GPL v2");

