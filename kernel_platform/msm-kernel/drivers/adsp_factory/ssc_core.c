/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/adsp/ssc_ssr_reason.h>
#include <linux/string.h>
#ifdef CONFIG_VBUS_NOTIFIER
#include <linux/vbus_notifier.h> 
#endif
#include <linux/samsung/bsp/sec_cmdline.h>

#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE) || IS_ENABLED(CONFIG_HALL_NOTIFIER)
#include <linux/hall/hall_ic_notifier.h>
#endif
#if IS_ENABLED(CONFIG_SUPPORT_SSC_SPU)
#include <linux/adsp/ssc_spu.h>
#endif
#include "adsp.h"

#ifdef CONFIG_SUPPORT_SSC_MODE
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#endif

#include <linux/time64.h>

#include <linux/sensor/sensors_core.h>
#include <linux/notifier.h>

#include <linux/remoteproc.h>
#include <linux/remoteproc/qcom_rproc.h>

#define HISTORY_CNT 5
#define NO_SSR 0xFF
#define SSR_REASON_LEN	256
#define TIME_LEN	24
#ifdef CONFIG_SEC_FACTORY
#define SLPI_STUCK "SLPI_STUCK"
#define SLPI_PASS "SLPI_PASS"
#define PROBE_FAIL "PROBE_FAIL"
#endif

#if IS_ENABLED(CONFIG_SUPPORT_DUAL_6AXIS)
#define SUPPORT_DUAL_SENSOR "DUAL_GYRO"
#define SENSOR_DUMP_CNT 5
#define SENSOR_RDUMP_CNT 6
#else
#define SUPPORT_DUAL_SENSOR "SINGLE_GYRO"
#define SENSOR_DUMP_CNT 4
#define SENSOR_RDUMP_CNT 5
#endif

#define SENSOR_DUMP_DONE "SENSOR_DUMP_DONE"

#define SENSOR_HW_REVISION_MAX 31

#ifdef CONFIG_SUPPORT_SSC_MODE
#ifdef CONFIG_SUPPORT_SSC_MODE_FOR_MAG
#define ANT_NFC_MST  0
#define ANT_NFC_ONLY 1
#define ANT_DUMMY    2
#endif
#endif

static struct delayed_work ssr_dump_work;
static int pid;
static char panic_msg[SSR_REASON_LEN];
static char ssr_history[HISTORY_CNT][TIME_LEN + SSR_REASON_LEN];
static unsigned char ssr_idx = NO_SSR;
#if IS_ENABLED(CONFIG_SLPI_LOADING_FAILURE_DEBUG)
static unsigned int removed_sensors;
#endif

#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE) || IS_ENABLED(CONFIG_SUPPORT_VIRTUAL_OPTIC) || IS_ENABLED(CONFIG_HALL_NOTIFIER)
#define FLIP_HALL_NAME "flip"
#define MPP_BACKCOVER_HALL_NAME "back_cover"
static int32_t curr_mpp_cover_state;
//static struct hall_notifier_context *hall_notifier;
#endif
#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE) || IS_ENABLED(CONFIG_SUPPORT_VIRTUAL_OPTIC)
static int32_t curr_fstate;
#endif

enum ic_num {
	MAIN_GRIP = 0,
	SUB_GRIP,
	SUB2_GRIP,
	WIFI_GRIP,
	GRIP_MAX_CNT
};

enum sns_crash_mode {
	SC_MODE_PANIC = 1,
	SC_MODE_SSR_DUMP_ON = 2,
	SC_MODE_SSR_DUMP_OFF = 3,
	SC_MODE_SSR_DUMP_COMPLETE = 4,
	SC_MODE_FSSR_IGNORE_TRUE = 5,
	SC_MODE_FSSR_IGNORE_FALSE = 6,
	SC_MODE_RECOVERY_DISABLE_TRUE = 8,
	SC_MODE_RECOVERY_DISABLE_FALSE = 9,
	SC_MODE_MAX
};

struct sdump_data {
	struct workqueue_struct *sdump_wq;
	struct work_struct work_sdump;
	struct adsp_data *dev_data;

	u32 grip_error[GRIP_MAX_CNT];
};
struct sdump_data *pdata_sdump;

//#if defined(CONFIG_SEC_FACTORY) && IS_ENABLED(CONFIG_SUPPORT_DUAL_6AXIS)
//extern bool is_pretest(void);
//#endif
#if IS_ENABLED(CONFIG_SUPPORT_SSC_SPU)
static int fw_idx = SSC_ORI;
void ssc_set_fw_idx(int src)
{
	fw_idx = src;
}
EXPORT_SYMBOL(ssc_set_fw_idx);
#endif

int get_ssc_recovery_command(int type)
{
	struct adsp_data *data = adsp_get_struct_data();
	int ret = -1;

	if (data == NULL) {
		pr_err("[FACTORY] %s: data NULL\n", __func__);
		return -1;
	}

	switch (type) {
	case SNS_RECOVERY_SSR:
		ret = OPTION_TYPE_SSC_RECOVERY;
		break;
	case SNS_RECOVERY_SSR_DUMP_START:
		if (data->fssr_dump)
			ret = OPTION_TYPE_SSC_SSR_DUMP;
		break;
	case SNS_RECOVERY_SSR_DUMP_ON:
	case SNS_RECOVERY_SSR_DUMP_OFF:
	case SNS_RECOVERY_SSR_DUMP_COMPLETE:
		ret = OPTION_TYPE_SSC_SSR_DUMP;
		break;
	default:
		ret = OPTION_TYPE_SSC_RECOVERY;
		break;
	}
	return ret;
}

void send_ssc_recovery_command(int type)
{
	int32_t msg_buf[2] = {0, };
	int32_t msg = get_ssc_recovery_command(type);

	if (msg < 0) {
		pr_err("[FACTORY] %s: skip, type:%d\n", __func__, type);
		return;
	}
	msg_buf[0] = msg;
	msg_buf[1] = type;

	pr_info("[FACTORY] %s: msg_buf = %d,%d\n",
		__func__, msg_buf[0], msg_buf[1]);
	adsp_unicast(msg_buf, sizeof(msg_buf),
			MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);
}
EXPORT_SYMBOL(send_ssc_recovery_command);

static unsigned int sec_hw_rev(void);

#if IS_ENABLED(CONFIG_SUPPORT_AK09973)
void dhall_cal_work_func(struct work_struct *work)
{
	int buf[58] = { 1, };

	adsp_unicast(buf, sizeof(buf),
		MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_SET_REGISTER);
}

void digital_hall_factory_auto_cal_init_work(struct adsp_data *data)
{
	schedule_delayed_work(&data->dhall_cal_work, msecs_to_jiffies(8000));
}
#endif

#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE)
struct ssc_flip_data {
	struct workqueue_struct *ssc_flip_wq;
	struct work_struct work_ssc_flip;
	bool is_opend;
	bool only_update;
};
struct ssc_flip_data *pdata_ssc_flip;
#endif

#ifdef CONFIG_VBUS_NOTIFIER
#define CHARGE_START	true
#define CHARGE_END	false

struct ssc_charge_data {
	struct workqueue_struct *ssc_charge_wq;
	struct work_struct work_ssc_charge;
	bool is_charging;
};
struct ssc_charge_data *pdata_ssc_charge;
#endif

#ifdef CONFIG_VBUS_NOTIFIER
void ssc_charge_work_func(struct work_struct *work)
{
	int32_t msg_buf[2];
	msg_buf[0] = OPTION_TYPE_SSC_CHARGING_STATE;
	msg_buf[1] = (int32_t)pdata_ssc_charge->is_charging;
	pr_info("[FACTORY] %s: msg_buf = %d\n", __func__, msg_buf[1]);
	adsp_unicast(msg_buf, sizeof(msg_buf),
			MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);
}

static int ssc_core_vbus_notifier(struct notifier_block *nb,
				unsigned long action, void *data)
{
	vbus_status_t vbus_type = *(vbus_status_t *) data;
	static int vbus_pre_attach;

	if (vbus_pre_attach == vbus_type)
		return 0;

	switch (vbus_type) {
	case STATUS_VBUS_HIGH:
	case STATUS_VBUS_LOW:
		pdata_ssc_charge->is_charging = 
			(vbus_type == STATUS_VBUS_HIGH) ? true : false;
		pr_info("[FACTORY] vbus high:%d \n", (int)pdata_ssc_charge->is_charging);
		queue_work(pdata_ssc_charge->ssc_charge_wq,
			&pdata_ssc_charge->work_ssc_charge);                       
		break;
	default:
		pr_info("[FACTORY] vbus skip attach = %d\n", vbus_type);
		break;
	}

	vbus_pre_attach = vbus_type;

	return 0;
}

void sns_vbus_init_work(void)
{
	pr_info("[FACTORY] sns_vbus_init_work:%d \n", (int)pdata_ssc_charge->is_charging);
	queue_work(pdata_ssc_charge->ssc_charge_wq,
		&pdata_ssc_charge->work_ssc_charge);
}
#endif

/*************************************************************************/
/* factory Sysfs							 */
/*************************************************************************/

static char operation_mode_flag[11] = "normal";

static ssize_t dumpstate_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t type[1];

	if (pid != 0) {
		pr_info("[FACTORY] to take the logs\n");
		type[0] = 0;
	} else {
		type[0] = 2;
		pr_info("[FACTORY] logging service was stopped %d\n", pid);
	}

	pr_info("[FACTORY] %s support_algo = %u\n", __func__, data->support_algo);

	return snprintf(buf, PAGE_SIZE, "SSC_CORE\n");
}

static ssize_t operation_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s", operation_mode_flag);
}

static ssize_t operation_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < 10 && buf[i] != '\0'; i++)
		operation_mode_flag[i] = buf[i];
	operation_mode_flag[i] = '\0';

	if (!strncmp(operation_mode_flag, "restrict", 8)) {
		pr_info("[FACTORY] %s: set restrict_mode\n", __func__);
		data->restrict_mode = true;
	} else {
		pr_info("[FACTORY] %s: set normal_mode\n", __func__);
		data->restrict_mode = false;
	}
	pr_info("[FACTORY] %s: operation_mode_flag = %s\n", __func__,
		operation_mode_flag);
	return size;
}

static ssize_t mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	unsigned long timeout;
	unsigned long timeout_2;
	int32_t type[1];

	if (pid != 0) {
		timeout = jiffies + (2 * HZ);
		timeout_2 = jiffies + (10 * HZ);
		type[0] = 1;
		pr_info("[FACTORY] To stop logging %d\n", pid);
		adsp_unicast(type, sizeof(type), MSG_SSC_CORE, 0, MSG_TYPE_DUMPSTATE);

		while (pid != 0) {
			msleep(25);
			if (time_after(jiffies, timeout))
				pr_info("[FACTORY] %s: Timeout!!!\n", __func__);
			if (time_after(jiffies, timeout_2)) {
//				panic("force crash : ssc core\n");
				pr_info("[FACTORY] pid %d\n", pid);
				return snprintf(buf, PAGE_SIZE, "1\n");
			}
		}
	}

	return snprintf(buf, PAGE_SIZE, "0\n");
}

static ssize_t mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int data = 0;
	int32_t type[1];
	if (kstrtoint(buf, 10, &data)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return -EINVAL;
	}

	if (data != 1) {
		pr_err("[FACTORY] %s: data was wrong\n", __func__);
		return -EINVAL;
	}

	if (pid != 0) {
		type[0] = 1;
		adsp_unicast(type, sizeof(type), MSG_SSC_CORE, 0, MSG_TYPE_DUMPSTATE);
	}

	return size;
}

static ssize_t pid_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", pid);
}

static ssize_t pid_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int data = 0;

	if (kstrtoint(buf, 10, &data)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return -EINVAL;
	}

	pid = data;
	pr_info("[FACTORY] %s: pid %d\n", __func__, pid);

	return size;
}

static ssize_t remove_sensor_sysfs_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int type = MSG_SENSOR_MAX;
	struct adsp_data *data = dev_get_drvdata(dev);

	if (kstrtouint(buf, 10, &type)) {
		pr_err("[FACTORY] %s: kstrtouint fail\n", __func__);
		return -EINVAL;
	}

	if (type > PHYSICAL_SENSOR_SYSFS) {
		pr_err("[FACTORY] %s: Invalid type %u\n", __func__, type);
		return size;
	}
	pr_info("[FACTORY] %s: type = %u\n", __func__, type);
#if IS_ENABLED(CONFIG_SLPI_LOADING_FAILURE_DEBUG)
	removed_sensors |= (0x1 << type);
	if ((removed_sensors & 0x3f) == 0x3f)
#if 0	
		panic("slpi is not loaded, force panic\n");
#else
		pr_info("[FACTORY] %s: slpi is not loaded\n", __func__);
#endif
#endif
	mutex_lock(&data->remove_sysfs_mutex);
	adsp_factory_unregister(type);
	mutex_unlock(&data->remove_sysfs_mutex);
	return size;
}

static unsigned int system_rev __read_mostly;
static void sec_hw_rev_setup(void)
{
  const char *revision_str;
  int err;

  revision_str = sec_cmdline_get_val("androidboot.revision");
  if (!revision_str) {
    pr_err("[FACTORY] androidboot.revision is missing.\n");
    system_rev = 31;
    return;
  }

  err = kstrtouint(revision_str, 0, &system_rev);
  if (err < 0) {
    pr_err("[FACTORY] androidboot.revision is malformed (%s)\n",
      revision_str);
    system_rev = 31;
    return;
  }
}

static unsigned int sec_hw_rev(void)
{
	return system_rev;
}

int ssc_system_rev_test = -1;
static ssize_t ssc_hw_rev_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t hw_rev = sec_hw_rev();

	if(hw_rev > SENSOR_HW_REVISION_MAX)
		hw_rev = SENSOR_HW_REVISION_MAX;

	pr_info("[FACTORY] %s: ssc_rev:%d, ssc_system_rev_test %d\n", __func__, hw_rev, ssc_system_rev_test);

	if (ssc_system_rev_test < 0)
		return snprintf(buf, PAGE_SIZE, "%d\n", hw_rev);
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", ssc_system_rev_test);
}

static ssize_t ssc_hw_rev_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (kstrtoint(buf, 10, &ssc_system_rev_test)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return -EINVAL;
	}

	pr_info("[FACTORY] %s: system_rev_ssc %d\n", __func__, ssc_system_rev_test);

	return size;
}

void ssr_reason_call_back(char reason[], int len)
{
	struct timespec64 ts;
	struct tm tm;

	if (len <= 0) {
		pr_info("[FACTORY] ssr %d\n", len);
		return;
	}
	memset(panic_msg, 0, SSR_REASON_LEN);
	strlcpy(panic_msg, reason, min(len, (int)(SSR_REASON_LEN - 1)));

	// Please do not modify "[FACTORY] ssr" string.
	// It is refered from BL to log in history_auto_xxxxxx.lst file.
	pr_info("[FACTORY] ssr %s\n", panic_msg);

	if (ssr_idx == HISTORY_CNT || ssr_idx == NO_SSR)
		ssr_idx = 0;

	memset(ssr_history[ssr_idx], 0, TIME_LEN + SSR_REASON_LEN);

	ktime_get_real_ts64(&ts);
	time64_to_tm(ts.tv_sec, 0, &tm);

	sprintf(ssr_history[ssr_idx], "[%ld%02d%02d %02d:%02d:%02d] ",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	strlcat(ssr_history[ssr_idx++], panic_msg, TIME_LEN + SSR_REASON_LEN);
}
EXPORT_SYMBOL_GPL(ssr_reason_call_back);

bool ssc_get_fssr_ignore(void)
{
	struct adsp_data *data = adsp_get_struct_data();

	if (data == NULL) {
		pr_err("[FACTORY] %s: data NULL\n", __func__);
		return false;
	}
	return data->fssr_ignore;
}
EXPORT_SYMBOL_GPL(ssc_get_fssr_ignore);

static ssize_t ssr_msg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
#ifdef CONFIG_SEC_FACTORY
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t raw_data_acc[3] = {0, };
	int32_t raw_data_mag[3] = {0, };
	int ret_acc = 0;
	int ret_mag = 0;
//#if IS_ENABLED(CONFIG_SUPPORT_DUAL_6AXIS)
//	if (is_pretest())
//		return snprintf(buf, PAGE_SIZE, "%s\n", panic_msg);
//#endif
	if (!data->sysfs_created[MSG_ACCEL] && !data->sysfs_created[MSG_MAG]
		&& !data->sysfs_created[MSG_PRESSURE]) {
		pr_err("[FACTORY] %s: sensor probe fail\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%s\n", PROBE_FAIL);
	}

	mutex_lock(&data->accel_factory_mutex);
	ret_acc = get_accel_raw_data(raw_data_acc);
	mutex_unlock(&data->accel_factory_mutex);

	ret_mag = get_mag_raw_data(raw_data_mag);

	pr_info("[FACTORY] %s: accel(%d, %d, %d), mag(%d, %d, %d)\n", __func__,
		raw_data_acc[0], raw_data_acc[1], raw_data_acc[2],
		raw_data_mag[0], raw_data_mag[1], raw_data_mag[2]);

	if (ret_acc == -1 && ret_mag == -1) {
		pr_err("[FACTORY] %s: SLPI stuck, check hal log\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%s\n", SLPI_STUCK);
	} else {
		return snprintf(buf, PAGE_SIZE, "%s\n", SLPI_PASS);
	}
#else
	return snprintf(buf, PAGE_SIZE, "%s\n", panic_msg);
#endif
}

static struct rproc* get_adsp_remoteproc(void)
{
	struct device_node *np;
	phandle rproc_phandle;
	struct rproc *adsp;
	struct property *prop;
	int size;

	np = of_find_node_by_name(NULL, "ssc_adsp_rproc_phandle");
	if (np == NULL) {
		pr_err("Didn't find node for ssc_adsp_rproc_phandle\n");
		return NULL;
	}
	prop = of_find_property(np, "qcom,rproc-handle", &size);
	if (!prop) {
		pr_err("Missing remotproc handle\n");
		return NULL;
	}
	rproc_phandle = be32_to_cpup(prop->value);
	adsp = rproc_get_by_phandle(rproc_phandle);
	if (!adsp) {
		pr_err("fail to get rproc\n");
		return NULL;
	}

	return adsp;
}

static ssize_t ssr_reset_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct rproc *adsp = get_adsp_remoteproc();
	int32_t type = 0xff;

	if (adsp == NULL) {
		pr_info("[FACTORY] %s, adsp remoteproc null\n", __func__);
		goto reset_return;
	}

	adsp_unicast(&type, sizeof(int32_t), MSG_SSC_CORE, 0, MSG_TYPE_DUMPSTATE);

reset_return:
	return snprintf(buf, PAGE_SIZE, "%s\n", "Success");
}

static ssize_t support_algo_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int support_algo = 0;
	struct adsp_data *data = dev_get_drvdata(dev);

	if (kstrtouint(buf, 10, &support_algo)) {
		pr_err("[FACTORY] %s: kstrtouint fail\n", __func__);
		return -EINVAL;
	}

        data->support_algo = (uint32_t)support_algo;
	pr_info("[FACTORY] %s support_algo = %u\n", __func__, support_algo);

	return size;
}

#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
int sns_hall_notify(struct notifier_block *nb,
	unsigned long state, void *v)
{
	struct hall_notifier_context *hall_notifier = v;
	int32_t msg_buf[3]; /* [0]: OPTION_TYPE, [1]: DEVICE_MODE_TYPE, [2]: OPEN/CLOSE */

	if (strncmp(hall_notifier->name, FLIP_HALL_NAME, 4) && strncmp(hall_notifier->name, MPP_BACKCOVER_HALL_NAME, 9))
		return 0;

	pr_info("[FACTORY] %s - name = %s, state = %s\n", __func__, hall_notifier->name, state ? "close" : "open");
	msg_buf[0] = OPTION_TYPE_SSC_DEVICE_MODE;

#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE)
	if (!strncmp(hall_notifier->name, FLIP_HALL_NAME, 4))
	{
		struct adsp_data *data = container_of(nb, struct adsp_data, adsp_nb);
		msg_buf[1] = DEVICE_MODE_FLIP_OPEN;
		pr_info("[FACTORY] %s - before device mode curr:%s, fstate:%s\n",
			__func__, curr_fstate ? "close" : "open", data->fac_fstate ? "close" : "open");

		curr_fstate = (int32_t)state;
#if !defined(CONFIG_SEC_FACTORY) || !defined(CONFIG_SUPPORT_SENSOR_FLIP_MODEL)
		data->fac_fstate = curr_fstate;
#endif
		pr_info("[FACTORY] %s - after device mode curr:%s, fstate:%s\n",
			__func__, curr_fstate ? "close" : "open", data->fac_fstate ? "close" : "open");

		msg_buf[2] = curr_fstate;
		adsp_unicast(msg_buf, 0, MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);

	        // send the flip state by qmi.
		queue_work(pdata_ssc_flip->ssc_flip_wq, &pdata_ssc_flip->work_ssc_flip);
	}
#endif

	if (!strncmp(hall_notifier->name, MPP_BACKCOVER_HALL_NAME, 9))
	{
		msg_buf[1] = DEVICE_MODE_RESERVED_FOR_OEM_1;
		msg_buf[2] = state;
		curr_mpp_cover_state = (int32_t)state;
		adsp_unicast(msg_buf, sizeof(msg_buf), MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);
		pr_info("[FACTORY] %s - send mpp cover state %s\n", __func__, state ? "close" : "open");
	}
	return 0;
}

void sns_mpp_cover_init_work(void)
{
	int32_t msg_buf[3]; /* [0]: OPTION_TYPE, [1]: DEVICE_MODE_TYPE, [2]: OPEN/CLOSE */

	msg_buf[0] = OPTION_TYPE_SSC_DEVICE_MODE;
	msg_buf[1] = DEVICE_MODE_RESERVED_FOR_OEM_1;
	msg_buf[2] = curr_mpp_cover_state;
	if (curr_mpp_cover_state) {
		adsp_unicast(msg_buf, sizeof(msg_buf), MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);
		pr_info("[FACTORY] %s - send mpp cover state %s\n", __func__, curr_mpp_cover_state ? "close" : "open");
	}
}
#endif

#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE)
void ssc_flip_work_func(struct work_struct *work)
{
	int32_t msg_buf[4];
#if IS_ENABLED(CONFIG_SUPPORT_DUAL_OPTIC)
	if (pdata_ssc_flip->only_update) {
		msg_buf[0] = VOPTIC_OP_CMD_SSC_FLIP_UPDATE;
		pdata_ssc_flip->only_update = false;
	} else {
		msg_buf[0] = VOPTIC_OP_CMD_SSC_FLIP;
	}
	msg_buf[1] = (int32_t)curr_fstate;
	pr_info("[FACTORY] %s: msg_buf = %d\n", __func__, msg_buf[1]);
#if !defined(CONFIG_SEC_FACTORY) || !defined(CONFIG_SUPPORT_SENSOR_FLIP_MODEL)
	adsp_unicast(msg_buf, sizeof(msg_buf),
			MSG_VIR_OPTIC, 0, MSG_TYPE_OPTION_DEFINE);
#endif
#else
	msg_buf[0] = 11;
	msg_buf[1] = (int32_t)curr_fstate;
	pr_info("[FACTORY] %s: msg_buf = %d\n", __func__, msg_buf[1]);
	adsp_unicast(msg_buf, sizeof(msg_buf),
			MSG_LIGHT, 0, MSG_TYPE_OPTION_DEFINE);
#endif
}

void sns_flip_init_work(void)
{
	pr_info("[FACTORY] sns_flip_init_work:%d \n", (int)curr_fstate);
	queue_work(pdata_ssc_flip->ssc_flip_wq,
		&pdata_ssc_flip->work_ssc_flip);
}

void sns_device_mode_init_work(void)
{
	if(curr_fstate == 0)
		adsp_unicast(NULL, 0, MSG_SSC_CORE, 0, MSG_TYPE_FACTORY_ENABLE);
	else
		adsp_unicast(NULL, 0, MSG_SSC_CORE, 0, MSG_TYPE_FACTORY_DISABLE);
}
#endif

#if IS_ENABLED(CONFIG_SUPPORT_VIRTUAL_OPTIC)
static ssize_t fac_fstate_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int32_t fstate[2] = {VOPTIC_OP_CMD_FAC_FLIP, 0};

	mutex_lock(&data->vir_optic_factory_mutex);

	if (sysfs_streq(buf, "0"))
		data->fac_fstate = fstate[1] = curr_fstate;
	else if (sysfs_streq(buf, "1"))
		data->fac_fstate = fstate[1] = FSTATE_FAC_INACTIVE;
	else if (sysfs_streq(buf, "2"))
		data->fac_fstate = fstate[1] = FSTATE_FAC_ACTIVE;
	else if (sysfs_streq(buf, "3"))
		data->fac_fstate = fstate[1] = FSTATE_FAC_INACTIVE_2;
#if IS_ENABLED(CONFIG_TABLET_MODEL_CONCEPT)
	else if (sysfs_streq(buf, "13"))
		data->fac_fstate = fstate[1] = LIGHT_DUAL_CHECK_MODE;
#endif
	else
		data->fac_fstate = fstate[1] = curr_fstate;

	adsp_unicast(fstate, sizeof(fstate),
		MSG_VIR_OPTIC, 0, MSG_TYPE_OPTION_DEFINE);
	while (!(data->ready_flag[MSG_TYPE_OPTION_DEFINE] & 1 << MSG_VIR_OPTIC) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_OPTION_DEFINE] &= ~(1 << MSG_VIR_OPTIC);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);

	mutex_unlock(&data->vir_optic_factory_mutex);

	pr_info("[SSC_FAC] %s - Factory flip state:%d(%d)",
		__func__, fstate[1], data->msg_buf[MSG_VIR_OPTIC][0]);

	return size;
}
#endif

static ssize_t wakeup_reason_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int32_t msg_buf[2] = {OPTION_TYPE_SSC_WAKEUP_REASON, 0};

	adsp_unicast(msg_buf, sizeof(msg_buf), MSG_SSC_CORE, 0, MSG_TYPE_GET_CAL_DATA);
	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_SSC_CORE) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_SSC_CORE);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "0\n");
	}

	return snprintf(buf, PAGE_SIZE, "\
\"SW_SMD\":\"%d\",\"SW_TILT\":\"%d\",\"SW_PICKUP\":\"%d\",\"SW_SBM\":\"%d\",\"SW_WU_MOTION\":\"%d\",\
\"SW_PROX\":\"%d\",\"SW_CALLGESTURE\":\"%d\",\"SW_POCKET\":\"%d\",\"SW_LEDCOVER\":\"%d\",\"SW_FCD\":\"%d\",\
\"SW_DROPCLASSIFIER\":\"%d\",\"SW_POCKET_POSE\":\"%d\",\"SW_STEPCOUNT_ALERT\":\"%d\",\"SW_FOLDING_LPM\":\"%d\",\"SW_HINGE_ANGLE\":\"%d\",\
\"SW_LID_ANGLE\":\"%d\",\"SW_ANGLE_STATUS\":\"%d\",\"SW_SMARTALERT\":\"%d\",\"SW_SEM_ROTATE\":\"%d\",\"SW_WCD\":\"%d\",\
\"SW_PUTDOWN\":\"%d\",\"SW_SLOCATION\":\"%d\",\"SW_AMD\":\"%d\",\"SW_AOD\":\"%d\",\"SW_FLATMOTION\":\"%d\",\
\"SW_SENSORCHECK\":\"%d\",\"SW_D_POSITION\":\"%d\",\"SW_LTG\":\"%d\",\"SW_FREEFALL\":\"%d\",\"SW_PEDOMETER\":\"%d\",\
\"SW_SLM\":\"%d\",\"SW_AT\":\"%d\"\n",
			data->msg_buf[MSG_SSC_CORE][0], data->msg_buf[MSG_SSC_CORE][1],
			data->msg_buf[MSG_SSC_CORE][2], data->msg_buf[MSG_SSC_CORE][3],
			data->msg_buf[MSG_SSC_CORE][4], data->msg_buf[MSG_SSC_CORE][5],
			data->msg_buf[MSG_SSC_CORE][6], data->msg_buf[MSG_SSC_CORE][7],
			data->msg_buf[MSG_SSC_CORE][8], data->msg_buf[MSG_SSC_CORE][9],
			data->msg_buf[MSG_SSC_CORE][10], data->msg_buf[MSG_SSC_CORE][11],
			data->msg_buf[MSG_SSC_CORE][12], data->msg_buf[MSG_SSC_CORE][13],
			data->msg_buf[MSG_SSC_CORE][14], data->msg_buf[MSG_SSC_CORE][15],
			data->msg_buf[MSG_SSC_CORE][16], data->msg_buf[MSG_SSC_CORE][17],
			data->msg_buf[MSG_SSC_CORE][18], data->msg_buf[MSG_SSC_CORE][19],
			data->msg_buf[MSG_SSC_CORE][20], data->msg_buf[MSG_SSC_CORE][21],
			data->msg_buf[MSG_SSC_CORE][22], data->msg_buf[MSG_SSC_CORE][23],
			data->msg_buf[MSG_SSC_CORE][24], data->msg_buf[MSG_SSC_CORE][25],
			data->msg_buf[MSG_SSC_CORE][26], data->msg_buf[MSG_SSC_CORE][27],
			data->msg_buf[MSG_SSC_CORE][28], data->msg_buf[MSG_SSC_CORE][29],
			data->msg_buf[MSG_SSC_CORE][30], 
			(data->msg_buf[MSG_SSC_CORE][31] + data->msg_buf[MSG_SSC_CORE][32] + 
			data->msg_buf[MSG_SSC_CORE][33] + data->msg_buf[MSG_SSC_CORE][34] + data->msg_buf[MSG_SSC_CORE][35]));
}

static ssize_t probe_fail_reason_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	adsp_unicast(NULL, 0, MSG_REG_SNS, 0, MSG_TYPE_OPTION_DEFINE);
	while (!(data->ready_flag[MSG_TYPE_OPTION_DEFINE] & 1 << MSG_REG_SNS) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_OPTION_DEFINE] &= ~(1 << MSG_REG_SNS);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "0\n");
	}

    if (!data->send_probe_fail_msg) {
		data->send_probe_fail_msg = true;
	    return snprintf(buf, PAGE_SIZE, "\
\"ACCEL_REASON\":\"%d\",\"ACCEL_INFO\":\"%d\",\"SUBACCEL_REASON\":\"%d\",\"SUBACCEL_INFO\":\"%d\",\
\"MAG_REASON\":\"%d\",\"MAG_INFO\":\"%d\",\
\"PROX_REASON\":\"%d\",\"PROX_INFO\":\"%d\",\"SUBPROX_REASON\":\"%d\",\"SUBPROX_INFO\":\"%d\",\
\"LIGHT_REASON\":\"%d\",\"LIGHT_INFO\":\"%d\",\"SUBLIGHT_REASON\":\"%d\",\"SUBLIGHT_INFO\":\"%d\",\
\"PRESSURE_REASON\":\"%d\",\"PREESURE_INFO\":\"%d\",\
\n",
				data->msg_buf[MSG_REG_SNS][1], data->msg_buf[MSG_REG_SNS][2],
				data->msg_buf[MSG_REG_SNS][4], data->msg_buf[MSG_REG_SNS][5],
				data->msg_buf[MSG_REG_SNS][7], data->msg_buf[MSG_REG_SNS][8],
				data->msg_buf[MSG_REG_SNS][10], data->msg_buf[MSG_REG_SNS][11],
				data->msg_buf[MSG_REG_SNS][13], data->msg_buf[MSG_REG_SNS][14],
				data->msg_buf[MSG_REG_SNS][16], data->msg_buf[MSG_REG_SNS][17],
				data->msg_buf[MSG_REG_SNS][19], data->msg_buf[MSG_REG_SNS][20],
				data->msg_buf[MSG_REG_SNS][22], data->msg_buf[MSG_REG_SNS][23]);
	} else {
		return snprintf(buf, PAGE_SIZE, "");
	}
}

#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE) && IS_ENABLED(CONFIG_SUPPORT_DUAL_OPTIC)
static ssize_t update_ssc_flip_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pdata_ssc_flip->only_update = true;
	queue_work(pdata_ssc_flip->ssc_flip_wq, &pdata_ssc_flip->work_ssc_flip);
	pr_info("[FACTORY] %s", __func__);

	return size;
}
#endif

static ssize_t support_dual_sensor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] %s: %s\n", __func__, SUPPORT_DUAL_SENSOR);
	return snprintf(buf, PAGE_SIZE, "%s\n", SUPPORT_DUAL_SENSOR);
}

static ssize_t algo_lcd_onoff_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int32_t msg_buf[2] = {0, };
	int cmd_type;

	if (kstrtoint(buf, 10, &cmd_type)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return size;
	}

	switch (cmd_type) {
	case COMMON_DATA_SET_ABS_ON:
	case COMMON_DATA_SET_ABS_OFF:
		msg_buf[0] = OPTION_TYPE_SSC_ABS_LCD_TYPE;
		msg_buf[1] = (cmd_type == COMMON_DATA_SET_ABS_ON) ? 1 : 0;
		break;
	case COMMON_DATA_SET_LCD_INTENT_ON:
	case COMMON_DATA_SET_LCD_INTENT_OFF:
		msg_buf[0] = OPTION_TYPE_SSC_LCD_INTENT_TYPE;
		msg_buf[1] = (cmd_type == COMMON_DATA_SET_LCD_INTENT_ON) ? 1 : 0;
		break;
	default:
		pr_info("[FACTORY] %s: Not support:%d \n", __func__, cmd_type);
		return size;
	}

	pr_info("[FACTORY] %s: lcd:%d,%d\n", __func__, msg_buf[0], msg_buf[1]);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);

	return size;
}

static void print_sensor_dump(struct adsp_data *data, int sensor)
{
	int i = 0;

	switch (sensor) {
	case MSG_ACCEL:
#if IS_ENABLED(CONFIG_SUPPORT_DUAL_6AXIS)
	case MSG_ACCEL_SUB:
#endif
		for (i = 0; i < 8; i++) {
			pr_info("[FACTORY] %s - %d: %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",
				__func__, sensor,
				data->msg_buf[sensor][i * 16 + 0],
				data->msg_buf[sensor][i * 16 + 1],
				data->msg_buf[sensor][i * 16 + 2],
				data->msg_buf[sensor][i * 16 + 3],
				data->msg_buf[sensor][i * 16 + 4],
				data->msg_buf[sensor][i * 16 + 5],
				data->msg_buf[sensor][i * 16 + 6],
				data->msg_buf[sensor][i * 16 + 7],
				data->msg_buf[sensor][i * 16 + 8],
				data->msg_buf[sensor][i * 16 + 9],
				data->msg_buf[sensor][i * 16 + 10],
				data->msg_buf[sensor][i * 16 + 11],
				data->msg_buf[sensor][i * 16 + 12],
				data->msg_buf[sensor][i * 16 + 13],
				data->msg_buf[sensor][i * 16 + 14],
				data->msg_buf[sensor][i * 16 + 15]);
		}
		break;
	case MSG_MAG:
		pr_info("[FACTORY] %s - %d: [00h-03h] %02x,%02x,%02x,%02x [10h-16h,18h] %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x [30h-32h] %02x,%02x,%02x [cal_id] %d\n",
			__func__, sensor,
			data->msg_buf[sensor][0], data->msg_buf[sensor][1],
			data->msg_buf[sensor][2], data->msg_buf[sensor][3],
			data->msg_buf[sensor][4], data->msg_buf[sensor][5],
			data->msg_buf[sensor][6], data->msg_buf[sensor][7],
			data->msg_buf[sensor][8], data->msg_buf[sensor][9],
			data->msg_buf[sensor][10], data->msg_buf[sensor][11],
			data->msg_buf[sensor][12], data->msg_buf[sensor][13],
			data->msg_buf[sensor][14], data->msg_buf[sensor][15]);
		break;
	case MSG_PRESSURE:
		for (i = 0; i < 7; i++) {
			pr_info("[FACTORY] %s - %d: %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",
				__func__, sensor,
				data->msg_buf[sensor][i * 16 + 0],
				data->msg_buf[sensor][i * 16 + 1],
				data->msg_buf[sensor][i * 16 + 2],
				data->msg_buf[sensor][i * 16 + 3],
				data->msg_buf[sensor][i * 16 + 4],
				data->msg_buf[sensor][i * 16 + 5],
				data->msg_buf[sensor][i * 16 + 6],
				data->msg_buf[sensor][i * 16 + 7],
				data->msg_buf[sensor][i * 16 + 8],
				data->msg_buf[sensor][i * 16 + 9],
				data->msg_buf[sensor][i * 16 + 10],
				data->msg_buf[sensor][i * 16 + 11],
				data->msg_buf[sensor][i * 16 + 12],
				data->msg_buf[sensor][i * 16 + 13],
				data->msg_buf[sensor][i * 16 + 14],
				data->msg_buf[sensor][i * 16 + 15]);
		}
		pr_info("[FACTORY] %s - %d: %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",
			__func__, sensor,
			data->msg_buf[sensor][i * 16 + 0],
			data->msg_buf[sensor][i * 16 + 1],
			data->msg_buf[sensor][i * 16 + 2],
			data->msg_buf[sensor][i * 16 + 3],
			data->msg_buf[sensor][i * 16 + 4],
			data->msg_buf[sensor][i * 16 + 5],
			data->msg_buf[sensor][i * 16 + 6],
			data->msg_buf[sensor][i * 16 + 7]);
		break;
	case MSG_LIGHT:
		pr_info("[FACTORY] %s - %d: %d,%d,%d,%d,%d,%d,%d [P]:%d,%d,%d [L]:%d,%d,%d [E]:%d,%d,%d\n",
			__func__, sensor,
			data->msg_buf[sensor][0], data->msg_buf[sensor][1],
			data->msg_buf[sensor][2], data->msg_buf[sensor][3],
			data->msg_buf[sensor][4], data->msg_buf[sensor][5],
			data->msg_buf[sensor][6], data->msg_buf[sensor][7],
			data->msg_buf[sensor][8], data->msg_buf[sensor][9],
			data->msg_buf[sensor][10], data->msg_buf[sensor][11],
			data->msg_buf[sensor][12], data->msg_buf[sensor][13],
			data->msg_buf[sensor][14], data->msg_buf[sensor][15]);
#if IS_ENABLED(CONFIG_SUPPORT_LIGHT_CALIBRATION) && IS_ENABLED(CONFIG_SUPPORT_PROX_CALIBRATION)
		pr_info("[FACTORY] %s - %d: [L] R:%d, %d, %d, W:%d, P:%d\n",
			__func__, sensor, data->light_cal_result,
			data->light_cal1, data->light_cal2,
			data->copr_w, data->prox_cal);
#endif
		break;
#if IS_ENABLED(CONFIG_SUPPORT_DUAL_OPTIC)
	case MSG_LIGHT_SUB:
		pr_info("[FACTORY] %s - %d: %d,%d,%d,%d,%d,%d,%d [P]:%d,%d,%d [L]:%d,%d,%d [E]:%d,%d,%d\n",
			__func__, sensor,
			data->msg_buf[sensor][0], data->msg_buf[sensor][1],
			data->msg_buf[sensor][2], data->msg_buf[sensor][3],
			data->msg_buf[sensor][4], data->msg_buf[sensor][5],
			data->msg_buf[sensor][6], data->msg_buf[sensor][7],
			data->msg_buf[sensor][8], data->msg_buf[sensor][9],
			data->msg_buf[sensor][10], data->msg_buf[sensor][11],
			data->msg_buf[sensor][12], data->msg_buf[sensor][13],
			data->msg_buf[sensor][14], data->msg_buf[sensor][15]);
#if IS_ENABLED(CONFIG_SUPPORT_LIGHT_CALIBRATION) && IS_ENABLED(CONFIG_SUPPORT_PROX_CALIBRATION)
		pr_info("[FACTORY] %s - %d: [L] R:%d, %d, %d, W:%d, P:%d\n",
			__func__, sensor, data->sub_light_cal_result,
			data->sub_light_cal1, data->sub_light_cal2,
			data->sub_copr_w, data->prox_sub_cal);
#endif
#endif
		break;
#if IS_ENABLED(CONFIG_SUPPORT_AK09973)
	case MSG_DIGITAL_HALL:
		pr_info("[FACTORY] %s - %d: ST: %02x, HX/HY/HZ: %d/%d/%d, CNTL: %02x/%02x/%02x, BOP/BRP: %d/%d\n",
			__func__, sensor,
			data->msg_buf[MSG_DIGITAL_HALL][0], data->msg_buf[MSG_DIGITAL_HALL][1],
			data->msg_buf[MSG_DIGITAL_HALL][2], data->msg_buf[MSG_DIGITAL_HALL][3],
			data->msg_buf[MSG_DIGITAL_HALL][4], data->msg_buf[MSG_DIGITAL_HALL][5],
			data->msg_buf[MSG_DIGITAL_HALL][6], data->msg_buf[MSG_DIGITAL_HALL][7],
			data->msg_buf[MSG_DIGITAL_HALL][8]);
		break;
#endif
	default:
		break;
	}
}

static void print_ssr_history(void)
{
	int i;

	if (ssr_idx == NO_SSR) {
		pr_info("[FACTORY] No SSR history\n");
		return;
	}

	pr_info("[FACTORY] Print SSR history\n");
	for (i = 0; i < HISTORY_CNT; i++) {
		if (ssr_history[i][0] == '\0')
			break;

		pr_info("[FACTORY] %s\n", ssr_history[i]);
	}
}

static BLOCKING_NOTIFIER_HEAD(sensordump_notifier_list);
int sensordump_notifier_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&sensordump_notifier_list, nb);
}
EXPORT_SYMBOL(sensordump_notifier_register);

int sensordump_notifier_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&sensordump_notifier_list, nb);
}
EXPORT_SYMBOL(sensordump_notifier_unregister);

int sensordump_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&sensordump_notifier_list, val, v);
}
EXPORT_SYMBOL(sensordump_notifier_call_chain);

static void sensor_get_dhr_info(struct adsp_data *data, int sensor_num)
{
	int cnt = 0;

	pr_info("[FACTORY] %s: %d\n", __func__, sensor_num);

	adsp_unicast(NULL, 0, sensor_num, 0, MSG_TYPE_GET_DHR_INFO);
	while (!(data->ready_flag[MSG_TYPE_GET_DHR_INFO] & 1 << sensor_num) &&
		cnt++ < TIMEOUT_DHR_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_GET_DHR_INFO] &= ~(1 << sensor_num);
	if (cnt >= TIMEOUT_DHR_CNT) {
		pr_err("[FACTORY] %s: %d Timeout!!!\n", __func__, sensor_num);
	} else {
		print_sensor_dump(data, sensor_num);
		msleep(200);
	}
}
#if IS_ENABLED(CONFIG_SENSORS_GRIP_FAILURE_DEBUG)
static void sensor_get_grip_info(void)
{
	int i = 0;
	if (pdata_sdump == NULL)
		return;

	for (i = 0; i < GRIP_MAX_CNT; i++)
		pr_err("GRIP%d_REASON : %d\n", i, pdata_sdump->grip_error[i]);
}
#endif
void sensor_dump_work_func(struct work_struct *work)
{
	struct sdump_data *sensor_dump_data = container_of((struct work_struct *)work,
		struct sdump_data, work_sdump);
	struct adsp_data *data = sensor_dump_data->dev_data;
#if IS_ENABLED(CONFIG_SUPPORT_AK09973)
	int cnt = 0;
#endif
#if IS_ENABLED(CONFIG_SUPPORT_DUAL_6AXIS)
	int sensor_type[SENSOR_DUMP_CNT] = { MSG_ACCEL, MSG_MAG, MSG_PRESSURE, MSG_LIGHT, MSG_ACCEL_SUB };
#else
	int sensor_type[SENSOR_DUMP_CNT] = { MSG_ACCEL, MSG_MAG, MSG_PRESSURE, MSG_LIGHT };
#endif
	int32_t msg_buf[2] = {OPTION_TYPE_SSC_DUMP_TYPE, 0};
	int i;

	sensordump_notifier_call_chain(1, NULL);

	for (i = 0; i < SENSOR_DUMP_CNT; i++) {
		if (!data->sysfs_created[sensor_type[i]]) {
			pr_info("[FACTORY] %s: %d was not probed\n",
				__func__, sensor_type[i]);
			continue;
		}

		sensor_get_dhr_info(data, sensor_type[i]);
	}

#if IS_ENABLED(CONFIG_SUPPORT_DUAL_OPTIC)
	sensor_get_dhr_info(data, MSG_LIGHT_SUB);
#endif
#if IS_ENABLED(CONFIG_SENSORS_GRIP_FAILURE_DEBUG)
	sensor_get_grip_info();
#endif
#if IS_ENABLED(CONFIG_SUPPORT_AK09973)
	if (data->sysfs_created[MSG_DIGITAL_HALL])
		sensor_get_dhr_info(data, MSG_DIGITAL_HALL);

	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < 3)
		msleep(30);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);

	if (cnt >= 3)
		pr_err("[FACTORY] %s: Read D/Hall Auto Cal Table Timeout!!!\n",
			__func__);

	pr_info("[FACTORY] %s: flg_update=%d\n",
		__func__, data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0]);
#endif

	adsp_unicast(msg_buf, sizeof(msg_buf),
		MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);

	adsp_unicast(NULL, 0, MSG_REG_SNS, 0, MSG_TYPE_OPTION_DEFINE);
	print_ssr_history();
}

#define MAX_DUMP_CNT 128
#define LEN_REGISTER 3 /* "ff " */
#define LEN_EXTRA 1 /* "\0" "\n" */
#define LEN_LINE_REG 16
#define LEN_PREFIX 11 /* "@@TYPE:%d##\n" */
#define DUMP_BUF_SIZE ((MAX_DUMP_CNT * LEN_REGISTER + LEN_PREFIX) * SENSOR_DUMP_CNT)
#define RBUF_SIZE (MAX_DUMP_CNT * LEN_REGISTER + LEN_PREFIX + LEN_EXTRA)

static int sensor_handle = -1;
static int sensor_idx = -1;
static int sensor_get_registerdump(struct adsp_data *data, int s_handle, int sensor, char *buf)
{
	int cnt = 0, i = 0;
	char dump_buff[MAX_DUMP_CNT * LEN_REGISTER + LEN_EXTRA];
	char temp_buff[LEN_REGISTER * LEN_LINE_REG + LEN_EXTRA];

	memset(dump_buff, 0, sizeof(dump_buff));
	memset(temp_buff, 0, sizeof(temp_buff));

	pr_info("[FACTORY] %s: %d\n", __func__, sensor);

	adsp_unicast(NULL, 0, sensor, 0, MSG_TYPE_GET_DHR_INFO);
	while (!(data->ready_flag[MSG_TYPE_GET_DHR_INFO] & 1 << sensor) &&
		cnt++ < TIMEOUT_DHR_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_GET_DHR_INFO] &= ~(1 << sensor);
	if (cnt >= TIMEOUT_DHR_CNT) {
		pr_err("[FACTORY] %s: %d Timeout!!!\n", __func__, sensor);
	} else {
		switch (sensor) {
		case MSG_ACCEL:
			for (i = 0; i < 8; i++) {
				snprintf(temp_buff, sizeof(temp_buff),
					"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
					data->msg_buf[MSG_ACCEL][i * 16 + 0],
					data->msg_buf[MSG_ACCEL][i * 16 + 1],
					data->msg_buf[MSG_ACCEL][i * 16 + 2],
					data->msg_buf[MSG_ACCEL][i * 16 + 3],
					data->msg_buf[MSG_ACCEL][i * 16 + 4],
					data->msg_buf[MSG_ACCEL][i * 16 + 5],
					data->msg_buf[MSG_ACCEL][i * 16 + 6],
					data->msg_buf[MSG_ACCEL][i * 16 + 7],
					data->msg_buf[MSG_ACCEL][i * 16 + 8],
					data->msg_buf[MSG_ACCEL][i * 16 + 9],
					data->msg_buf[MSG_ACCEL][i * 16 + 10],
					data->msg_buf[MSG_ACCEL][i * 16 + 11],
					data->msg_buf[MSG_ACCEL][i * 16 + 12],
					data->msg_buf[MSG_ACCEL][i * 16 + 13],
					data->msg_buf[MSG_ACCEL][i * 16 + 14],
					data->msg_buf[MSG_ACCEL][i * 16 + 15]);
				strlcat(dump_buff, temp_buff, sizeof(dump_buff));
			}
			break;
#if IS_ENABLED(CONFIG_SUPPORT_DUAL_6AXIS)
		case MSG_ACCEL_SUB:
			for (i = 0; i < 8; i++) {
				snprintf(temp_buff, sizeof(temp_buff),
					"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 0],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 1],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 2],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 3],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 4],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 5],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 6],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 7],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 8],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 9],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 10],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 11],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 12],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 13],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 14],
					data->msg_buf[MSG_ACCEL_SUB][i * 16 + 15]);
				strlcat(dump_buff, temp_buff, sizeof(dump_buff));
			}
			break;
#endif
		case MSG_MAG:
			snprintf(temp_buff, sizeof(temp_buff),
				"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
				data->msg_buf[MSG_MAG][0], data->msg_buf[MSG_MAG][1],
				data->msg_buf[MSG_MAG][2], data->msg_buf[MSG_MAG][3],
				data->msg_buf[MSG_MAG][4], data->msg_buf[MSG_MAG][5],
				data->msg_buf[MSG_MAG][6], data->msg_buf[MSG_MAG][7],
				data->msg_buf[MSG_MAG][8], data->msg_buf[MSG_MAG][9],
				data->msg_buf[MSG_MAG][10], data->msg_buf[MSG_MAG][11],
				data->msg_buf[MSG_MAG][12], data->msg_buf[MSG_MAG][13],
				data->msg_buf[MSG_MAG][14]);
			strlcat(dump_buff, temp_buff, sizeof(dump_buff));
			break;
		case MSG_PRESSURE:
			for (i = 0; i < 7; i++) {
				snprintf(temp_buff, sizeof(temp_buff),
					"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
					data->msg_buf[MSG_PRESSURE][i * 16 + 0],
					data->msg_buf[MSG_PRESSURE][i * 16 + 1],
					data->msg_buf[MSG_PRESSURE][i * 16 + 2],
					data->msg_buf[MSG_PRESSURE][i * 16 + 3],
					data->msg_buf[MSG_PRESSURE][i * 16 + 4],
					data->msg_buf[MSG_PRESSURE][i * 16 + 5],
					data->msg_buf[MSG_PRESSURE][i * 16 + 6],
					data->msg_buf[MSG_PRESSURE][i * 16 + 7],
					data->msg_buf[MSG_PRESSURE][i * 16 + 8],
					data->msg_buf[MSG_PRESSURE][i * 16 + 9],
					data->msg_buf[MSG_PRESSURE][i * 16 + 10],
					data->msg_buf[MSG_PRESSURE][i * 16 + 11],
					data->msg_buf[MSG_PRESSURE][i * 16 + 12],
					data->msg_buf[MSG_PRESSURE][i * 16 + 13],
					data->msg_buf[MSG_PRESSURE][i * 16 + 14],
					data->msg_buf[MSG_PRESSURE][i * 16 + 15]);
				strlcat(dump_buff, temp_buff, sizeof(dump_buff));
			}
			snprintf(temp_buff, sizeof(temp_buff),
					"%02x %02x %02x %02x %02x %02x %02x %02x\n",
					data->msg_buf[MSG_PRESSURE][i * 16 + 0],
					data->msg_buf[MSG_PRESSURE][i * 16 + 1],
					data->msg_buf[MSG_PRESSURE][i * 16 + 2],
					data->msg_buf[MSG_PRESSURE][i * 16 + 3],
					data->msg_buf[MSG_PRESSURE][i * 16 + 4],
					data->msg_buf[MSG_PRESSURE][i * 16 + 5],
					data->msg_buf[MSG_PRESSURE][i * 16 + 6],
					data->msg_buf[MSG_PRESSURE][i * 16 + 7]);
			strlcat(dump_buff, temp_buff, sizeof(dump_buff));
			break;
		case MSG_LIGHT:
			snprintf(temp_buff, sizeof(temp_buff),
				"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
				data->msg_buf[MSG_LIGHT][0], data->msg_buf[MSG_LIGHT][1],
				data->msg_buf[MSG_LIGHT][2], data->msg_buf[MSG_LIGHT][3],
				data->msg_buf[MSG_LIGHT][4], data->msg_buf[MSG_LIGHT][5],
				data->msg_buf[MSG_LIGHT][6], data->msg_buf[MSG_LIGHT][7],
				data->msg_buf[MSG_LIGHT][8], data->msg_buf[MSG_LIGHT][9],
				data->msg_buf[MSG_LIGHT][10], data->msg_buf[MSG_LIGHT][11],
				data->msg_buf[MSG_LIGHT][12], data->msg_buf[MSG_LIGHT][13],
				data->msg_buf[MSG_LIGHT][14], data->msg_buf[MSG_LIGHT][15]);
			strlcat(dump_buff, temp_buff, sizeof(dump_buff));
			break;
		}
		msleep(20);
	}

	return snprintf(buf, sizeof(dump_buff) + LEN_PREFIX,
			"@@TYPE:%d##\n%s", s_handle, dump_buff);
}

static ssize_t sensor_dump_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int size;
#if IS_ENABLED(CONFIG_SUPPORT_DUAL_6AXIS)
	int sensor_type[SENSOR_RDUMP_CNT] = { MSG_ACCEL, MSG_MAG, MSG_ACCEL, MSG_LIGHT, MSG_PRESSURE, MSG_ACCEL_SUB };
	int arr_handle[SENSOR_RDUMP_CNT] = {
				SENSOR_HANDLE_ACCELEROMETER,
				SENSOR_HANDLE_GEOMAGNETIC_FIELD,
				SENSOR_HANDLE_GYROSCOPE,
				SENSOR_HANDLE_LIGHT,
				SENSOR_HANDLE_PRESSURE,
				SENSOR_HANDLE_ACCELEROMETER_SUB };
#else
	int sensor_type[SENSOR_RDUMP_CNT] = { MSG_ACCEL, MSG_MAG, MSG_ACCEL, MSG_LIGHT, MSG_PRESSURE };
	int arr_handle[SENSOR_RDUMP_CNT] = {
				SENSOR_HANDLE_ACCELEROMETER,
				SENSOR_HANDLE_GEOMAGNETIC_FIELD,
				SENSOR_HANDLE_GYROSCOPE,
				SENSOR_HANDLE_LIGHT,
				SENSOR_HANDLE_PRESSURE };
#endif
	char *dump_buff, *rbuf;
	char prefix[] = "!@#REG_DUMP!@#\n";

	dump_buff = kzalloc(sizeof(char) * DUMP_BUF_SIZE, GFP_KERNEL);
	if (dump_buff == NULL) {
		pr_err("[FACTORY]: %s - dump_buff alloc failed\n", __func__);
		return -ENOMEM;
	}
	rbuf = kzalloc(sizeof(char) * RBUF_SIZE, GFP_KERNEL);
	if (rbuf == NULL) {
		pr_err("[FACTORY]: %s - rbuf alloc failed\n", __func__);
		kfree(dump_buff);
		return -ENOMEM;
	}

	mutex_lock(&data->sensors_dump_mutex);
	if (sensor_handle == -1) {
		pdata_sdump->dev_data = data;
		queue_work(pdata_sdump->sdump_wq, &pdata_sdump->work_sdump);
		size = snprintf(buf, PAGE_SIZE, "%s\n", SENSOR_DUMP_DONE);
	} else {
		switch (sensor_handle) {
		case SENSOR_HANDLE_ALL:
			for (int i = 0; i < SENSOR_RDUMP_CNT; i++) {
				if (!data->sysfs_created[sensor_type[i]]) {
					pr_info("[FACTORY] %s: %d was not probed\n",
					__func__, sensor_type[i]);
					continue;
				}
				sensor_get_registerdump(data, arr_handle[i], sensor_type[i], rbuf);
				strlcat(dump_buff, rbuf, DUMP_BUF_SIZE);
			}
			break;
		default:
			if (!data->sysfs_created[sensor_idx]) {
				pr_info("[FACTORY] %s: %d was not probed\n", __func__, sensor_idx);
				break;
			}
			sensor_get_registerdump(data, sensor_handle, sensor_idx, dump_buff);
			break;
		}

		sensor_handle = -1;
		size = snprintf(buf, PAGE_SIZE, "%s%s%s", prefix, dump_buff, prefix);
	}
	mutex_unlock(&data->sensors_dump_mutex);

	kfree(rbuf);
	kfree(dump_buff);
	return size;
}

static ssize_t sensor_dump_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	char name[SENSOR_NAME_MAX + 1] = {0,};
	struct adsp_data *data = dev_get_drvdata(dev);

	if (sscanf(buf, "%40s", name) != 1) {
		return -EINVAL;
	}

	mutex_lock(&data->sensors_dump_mutex);
	if (strcmp(name, "all") == 0) {
		sensor_handle = SENSOR_HANDLE_ALL;
	} else if (strcmp(name, "accelerometer") == 0) {
		sensor_handle = SENSOR_HANDLE_ACCELEROMETER;
		sensor_idx = MSG_ACCEL;
#if IS_ENABLED(CONFIG_SUPPORT_DUAL_6AXIS)
	} else if (strcmp(name, "accelerometer_sub") == 0) {
		sensor_handle = SENSOR_HANDLE_ACCELEROMETER_SUB;
		sensor_idx = MSG_ACCEL_SUB;
#endif
	} else if (strcmp(name, "gyro") == 0) {
		sensor_handle = SENSOR_HANDLE_GYROSCOPE;
		sensor_idx = MSG_ACCEL;
	} else if (strcmp(name, "magnetic") == 0) {
		sensor_handle = SENSOR_HANDLE_GEOMAGNETIC_FIELD;
		sensor_idx = MSG_MAG;
	} else if (strcmp(name, "pressure") == 0) {
		sensor_handle = SENSOR_HANDLE_PRESSURE;
		sensor_idx = MSG_PRESSURE;
	} else if (strcmp(name, "light") == 0) {
		sensor_handle = SENSOR_HANDLE_LIGHT;
		sensor_idx = MSG_LIGHT;
	} else {
		pr_info("[FACTORY] not supported : %s", name);
		sensor_handle = -1;
		sensor_idx = -1;
	}
	mutex_unlock(&data->sensors_dump_mutex);

	return size;
}

#if IS_ENABLED(CONFIG_SUPPORT_SSC_SPU)
static char ver_buf[SPU_VER_LEN];
static ssize_t ssc_firmware_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if (fw_idx == SSC_SPU) {
		return snprintf(buf, PAGE_SIZE, "idx:%d, CL:%s\n", fw_idx, ver_buf);
	} else {
		return snprintf(buf, PAGE_SIZE, "idx:1, CL:0, 0-0-0, 0:0:0.0\n");
	}
}

static ssize_t ssc_firmware_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int min_len = 0;
	int buf_len = strlen(buf);

	pr_info("[FACTORY] %s:buf:%s, %d\n", __func__, buf, buf_len);
	min_len = buf_len > SPU_VER_LEN ? SPU_VER_LEN : buf_len;
	memcpy(ver_buf, buf, min_len);

	return size;
}
#endif

#ifdef CONFIG_SUPPORT_SSC_MODE
static int ssc_mode;
static ssize_t ssc_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] ssc_mode:%d\n", ssc_mode);

	return snprintf(buf, PAGE_SIZE, "%d\n", ssc_mode);
}

static ssize_t ssc_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (kstrtoint(buf, 10, &ssc_mode)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return -EINVAL;
	}

	pr_info("[FACTORY] ssc_mode %d\n", ssc_mode);

	return size;
}
#endif

#if IS_ENABLED(CONFIG_SUPPORT_LIGHT_SEAMLESS)
static int light_seamless_lux_low, light_seamless_lux_high;
static int sub_light_seamless_lux_low, sub_light_seamless_lux_high;
void light_seamless_work_func(struct work_struct *work)
{
	if (light_seamless_lux_low != 0
		|| light_seamless_lux_high != 0
		|| sub_light_seamless_lux_low != 0
		|| sub_light_seamless_lux_high != 0) {
		int32_t msg_buf[5] = {0, };
		msg_buf[0] = OPTION_TYPE_SSC_LIGHT_SEAMLESS;
		msg_buf[1] = light_seamless_lux_low;
		msg_buf[2] = light_seamless_lux_high;
		msg_buf[3] = sub_light_seamless_lux_low;
		msg_buf[4] = sub_light_seamless_lux_high;
		adsp_unicast(msg_buf, sizeof(msg_buf),
			MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);
	}
	pr_info("[FACTORY] light seamless init, M%d,%d, S:%d,%d\n",
		light_seamless_lux_low, light_seamless_lux_high,
		sub_light_seamless_lux_low, sub_light_seamless_lux_high);
}

static ssize_t light_seamless_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] light seamless M%d,%d, S:%d,%d\n",
		light_seamless_lux_low, light_seamless_lux_high,
		sub_light_seamless_lux_low, sub_light_seamless_lux_high);
	return snprintf(buf, PAGE_SIZE, "M:%d,%d, S:%d,%d\n",
		light_seamless_lux_low, light_seamless_lux_high,
		sub_light_seamless_lux_low, sub_light_seamless_lux_high);
}

static ssize_t light_seamless_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int32_t msg_buf[5] = {0, };
	int32_t ret = 0;

	ret = sscanf(buf, "%5d,%5d,%5d,%5d", 
		&light_seamless_lux_low, &light_seamless_lux_high,
		&sub_light_seamless_lux_low, &sub_light_seamless_lux_high);
	if (ret != 4) {
		pr_err("[FACTORY]: %s - The number of data are wrong,%d\n",
			__func__, ret);
		return -EINVAL;
	}

	msg_buf[0] = OPTION_TYPE_SSC_LIGHT_SEAMLESS;
	msg_buf[1] = light_seamless_lux_low;
	msg_buf[2] = light_seamless_lux_high;
	msg_buf[3] = sub_light_seamless_lux_low;
	msg_buf[4] = sub_light_seamless_lux_high;
	adsp_unicast(msg_buf, sizeof(msg_buf),
		MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);
	pr_info("[FACTORY] light_seamless_lux, M:%d,%d, S:%d,%d\n",
		light_seamless_lux_low, light_seamless_lux_high,
		sub_light_seamless_lux_low, sub_light_seamless_lux_high);

	return size;
}

void light_seamless_init_work(struct adsp_data *data)
{
	schedule_delayed_work(&data->light_seamless_work, msecs_to_jiffies(5000));
}
#endif

static ssize_t ar_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int32_t msg_buf[2] = {OPTION_TYPE_SSC_AUTO_ROTATION_MODE, 0};

	msg_buf[1] = buf[0] - 48;
	pr_info("[FACTORY]%s: ar_mode:%d\n", __func__, msg_buf[1]);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);

	return size;
}

static int sbm_init;
static ssize_t sbm_init_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] %s : %d\n", __func__, sbm_init);

	return snprintf(buf, PAGE_SIZE, "%d\n", sbm_init);
}

static ssize_t sbm_init_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int32_t msg_buf[2] = {OPTION_TYPE_SSC_SBM_INIT, 0};

	if (kstrtoint(buf, 10, &sbm_init)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return -EINVAL;
	}

	if (sbm_init) {
		msg_buf[1] = sbm_init;
		pr_info("[FACTORY] %s : %d\n", __func__, sbm_init);
		adsp_unicast(msg_buf, sizeof(msg_buf),
			MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);
	}

	return size;
}

static ssize_t pocket_inject_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int32_t msg_buf[2] = {OPTION_TYPE_SSC_POCKET_INJECT, 0};
	int pocket_inject_data = 0;

	if (kstrtoint(buf, 10, &pocket_inject_data)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return -EINVAL;
	}

	if (pocket_inject_data) {
		msg_buf[1] = pocket_inject_data;
		adsp_unicast(msg_buf, sizeof(msg_buf),
			MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);
	}

	return size;
}

static ssize_t sns_crash_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int mode;

	if (kstrtoint(buf, 10, &mode)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return -EINVAL;
	}

	pr_info("[FACTORY] %s : %d\n", __func__, mode);
	if (mode == SC_MODE_PANIC) {
		panic("sensor is abnormal, force panic\n");
	} else if (mode == SC_MODE_SSR_DUMP_ON
		|| mode == SC_MODE_SSR_DUMP_OFF
		|| mode == SC_MODE_SSR_DUMP_COMPLETE) {
		if (mode == SC_MODE_SSR_DUMP_COMPLETE) {
			schedule_delayed_work(&ssr_dump_work, msecs_to_jiffies(7000));
		} else {
			int32_t msg_buf[2] = {0, };

			data->fssr_dump = (mode == SC_MODE_SSR_DUMP_ON) ? true : false;
			msg_buf[0] = OPTION_TYPE_SSC_SSR_DUMP;
			msg_buf[1] = (mode == SC_MODE_SSR_DUMP_ON) ?
				SNS_RECOVERY_SSR_DUMP_ON:SNS_RECOVERY_SSR_DUMP_OFF;
			pr_info("[FACTORY] %s: msg_buf = %d,%d\n",
				__func__, msg_buf[0], msg_buf[1]);
			adsp_unicast(msg_buf, sizeof(msg_buf),
					MSG_SSC_CORE, 0, MSG_TYPE_OPTION_DEFINE);
		}
	} else if (mode == SC_MODE_FSSR_IGNORE_TRUE
		|| mode == SC_MODE_FSSR_IGNORE_FALSE) {
		if (data == NULL) {
			pr_info("[FACTORY] %s, adsp remoteproc null\n", __func__);
			goto crash_return;
		}

		data->fssr_ignore = (mode == SC_MODE_FSSR_IGNORE_TRUE) ? true:false;
		pr_info("[FACTORY] %s, fssr_ignore:%d\n",
			__func__, (int)data->fssr_ignore);			
	} else if (mode == SC_MODE_RECOVERY_DISABLE_TRUE
		|| mode == SC_MODE_RECOVERY_DISABLE_FALSE) {
		struct rproc *adsp = get_adsp_remoteproc();

		if (adsp == NULL) {
			pr_info("[FACTORY] %s, adsp remoteproc null\n", __func__);
			goto crash_return;
		}		

		if (mode == SC_MODE_RECOVERY_DISABLE_TRUE) {
			adsp->recovery_disabled = true;
		} else {
			adsp->recovery_disabled = false;
		}
		pr_info("recovery_disabled:%d\n", (int)adsp->recovery_disabled);
	}
crash_return:
	return size;
}

static ssize_t sns_firm_idx_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct rproc *adsp = get_adsp_remoteproc();

	if (adsp == NULL) {
		pr_info("[FACTORY] %s, adsp remoteproc null\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	pr_info("[FACTORY] %s, firm index:%d\n", __func__, adsp->index);
	return snprintf(buf, PAGE_SIZE, "%d\n", adsp->index);
}


#if IS_ENABLED(CONFIG_SENSORS_GRIP_FAILURE_DEBUG)
void update_grip_error(u8 idx, u32 error_state)
{
	if ((pdata_sdump == NULL) || (idx >= GRIP_MAX_CNT)) {
		pr_info("[FACTORY] %s IC num %d or dump is NULL \n", __func__,
			idx);
		return;
	}
	pdata_sdump->grip_error[idx] = error_state;
	pr_info("[FACTORY] %s [IC num %d] grip_error %d\n", __func__,
		idx, error_state);
}
EXPORT_SYMBOL(update_grip_error);
static ssize_t grip_fail_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int i = 0, j = 0;

	if (pdata_sdump != NULL) {
		for (i = 0; i < GRIP_MAX_CNT; i++) {
			j += snprintf(buf + j, PAGE_SIZE - j,
				"\"GRIP%d_REASON\":\"%d\",",
				i, pdata_sdump->grip_error[i]);
			pr_info("[FACTORY] %s \"GRIP%d_REASON\":\"%d\",",
				__func__, i, pdata_sdump->grip_error[i]);
			pdata_sdump->grip_error[i] = 0;
		}
	}

	return j;
}
#endif

void ssr_dump_work_func(struct work_struct *work)
{
	send_ssc_recovery_command(SNS_RECOVERY_SSR_DUMP_COMPLETE);
}

#if IS_ENABLED(CONFIG_SENSORS_GRIP_FAILURE_DEBUG)
static DEVICE_ATTR(grip_fail, 0440, grip_fail_show, NULL);
#endif
static DEVICE_ATTR(dumpstate, 0440, dumpstate_show, NULL);
static DEVICE_ATTR(operation_mode, 0664,
	operation_mode_show, operation_mode_store);
static DEVICE_ATTR(mode, 0660, mode_show, mode_store);
static DEVICE_ATTR(ssc_pid, 0660, pid_show, pid_store);
static DEVICE_ATTR(remove_sysfs, 0220, NULL, remove_sensor_sysfs_store);
static DEVICE_ATTR(ssc_hw_rev, 0664, ssc_hw_rev_show, ssc_hw_rev_store);
static DEVICE_ATTR(ssr_msg, 0440, ssr_msg_show, NULL);
static DEVICE_ATTR(ssr_reset, 0440, ssr_reset_show, NULL);
static DEVICE_ATTR(support_algo, 0220, NULL, support_algo_store);
#if IS_ENABLED(CONFIG_SUPPORT_VIRTUAL_OPTIC)
static DEVICE_ATTR(fac_fstate, 0220, NULL, fac_fstate_store);
#endif
static DEVICE_ATTR(wakeup_reason, 0440, wakeup_reason_show, NULL);
static DEVICE_ATTR(probe_fail_reason, 0440, probe_fail_reason_show, NULL);
#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE) && IS_ENABLED(CONFIG_SUPPORT_DUAL_OPTIC)
static DEVICE_ATTR(update_ssc_flip, 0220, NULL, update_ssc_flip_store);
#endif
static DEVICE_ATTR(support_dual_sensor, 0440, support_dual_sensor_show, NULL);
static DEVICE_ATTR(algo_lcd_onoff, 0220, NULL, algo_lcd_onoff_store);
static DEVICE_ATTR(sensor_dump, 0664, sensor_dump_show, sensor_dump_store);
#if IS_ENABLED(CONFIG_SUPPORT_SSC_SPU)
static DEVICE_ATTR(ssc_firmware_info, 0660,
	ssc_firmware_info_show, ssc_firmware_info_store);
#endif
#ifdef CONFIG_SUPPORT_SSC_MODE
static DEVICE_ATTR(ssc_mode, 0664, ssc_mode_show, ssc_mode_store);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_LIGHT_SEAMLESS)
static DEVICE_ATTR(light_seamless, 0660,
	light_seamless_show, light_seamless_store);
#endif
static DEVICE_ATTR(ar_mode, 0220, NULL, ar_mode_store);
static DEVICE_ATTR(sbm_init, 0660, sbm_init_show, sbm_init_store);
static DEVICE_ATTR(pocket_inject, 0220, NULL, pocket_inject_store);
static DEVICE_ATTR(sns_crash, 0220, NULL, sns_crash_store);
static DEVICE_ATTR(sns_firm_idx, 0440, sns_firm_idx_show, NULL);

static struct device_attribute *core_attrs[] = {
	&dev_attr_dumpstate,
	&dev_attr_operation_mode,
	&dev_attr_mode,
	&dev_attr_ssc_pid,
	&dev_attr_remove_sysfs,
	&dev_attr_ssc_hw_rev,
	&dev_attr_ssr_msg,
	&dev_attr_ssr_reset,
	&dev_attr_support_algo,
#if IS_ENABLED(CONFIG_SUPPORT_VIRTUAL_OPTIC)
	&dev_attr_fac_fstate,
#endif
    &dev_attr_wakeup_reason,
    &dev_attr_probe_fail_reason,
#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE) && IS_ENABLED(CONFIG_SUPPORT_DUAL_OPTIC)
	&dev_attr_update_ssc_flip,
#endif
	&dev_attr_support_dual_sensor,
	&dev_attr_algo_lcd_onoff,
	&dev_attr_sensor_dump,
#if IS_ENABLED(CONFIG_SUPPORT_SSC_SPU)
	&dev_attr_ssc_firmware_info,
#endif
#ifdef CONFIG_SUPPORT_SSC_MODE
	&dev_attr_ssc_mode,
#endif
#if IS_ENABLED(CONFIG_SUPPORT_LIGHT_SEAMLESS)
	&dev_attr_light_seamless,
#endif
	&dev_attr_ar_mode,
	&dev_attr_sbm_init,
	&dev_attr_pocket_inject,
	&dev_attr_sns_crash,
	&dev_attr_sns_firm_idx,
#if IS_ENABLED(CONFIG_SENSORS_GRIP_FAILURE_DEBUG)
	&dev_attr_grip_fail,
#endif
	NULL,
};

#ifdef CONFIG_SUPPORT_SSC_MODE
static int ssc_core_probe(struct platform_device *pdev)
{
#ifdef CONFIG_SUPPORT_SSC_MODE_FOR_MAG
	struct device *dev = &pdev->dev;

#ifdef CONFIG_OF
	if (dev->of_node) {
		struct device_node *np = dev->of_node;
		int check_mst_gpio, check_nfc_gpio;
		int value_mst = 0, value_nfc = 0;

		check_mst_gpio = of_get_named_gpio_flags(np, "ssc_core,mst_gpio", 0, NULL);
		if(check_mst_gpio >= 0)
			value_mst = gpio_get_value(check_mst_gpio);

		if (value_mst == 1) {
			ssc_mode = ANT_NFC_MST;
			return 0;
		}

		check_nfc_gpio = of_get_named_gpio_flags(np, "ssc_core,nfc_gpio", 0, NULL);
		if(check_nfc_gpio >= 0)
			value_nfc = gpio_get_value(check_nfc_gpio);

		if (value_nfc == 1) {
			ssc_mode = ANT_NFC_ONLY;
			return 0;
		}
	}
#endif
	ssc_mode = ANT_DUMMY;
#endif
	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id ssc_core_dt_ids[] = {
	{ .compatible = "ssc_core" },
	{ },
};
MODULE_DEVICE_TABLE(of, ssc_core_dt_ids);
#endif /* CONFIG_OF */

static struct platform_driver ssc_core_driver = {
	.probe		= ssc_core_probe,
	.driver		= {
		.name	= "ssc_core",
		.owner	= THIS_MODULE,
#if defined(CONFIG_OF)
		.of_match_table	= ssc_core_dt_ids,
#endif /* CONFIG_OF */
	}
};
#endif

int __init core_factory_init(void)
{
#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE) || defined(CONFIG_VBUS_NOTIFIER) || IS_ENABLED(CONFIG_HALL_NOTIFIER)
	struct adsp_data *data = adsp_ssc_core_register(MSG_SSC_CORE, core_attrs);
	data->adsp_nb.notifier_call = sns_hall_notify,
	data->adsp_nb.priority = 1,
	pr_err("[FACTORY]: %s - hall_notifier_register\n", __func__);
	hall_notifier_register(&data->adsp_nb);
#else
	adsp_factory_register(MSG_SSC_CORE, core_attrs);
#endif
	sec_hw_rev_setup();
#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE)
	pdata_ssc_flip = kzalloc(sizeof(*pdata_ssc_flip), GFP_KERNEL);
	if (pdata_ssc_flip == NULL)
		return -ENOMEM;

	pdata_ssc_flip->ssc_flip_wq =
		create_singlethread_workqueue("ssc_flip_wq");
	if (pdata_ssc_flip->ssc_flip_wq == NULL) {
		pr_err("[FACTORY]: %s - couldn't create ssc charge wq\n", __func__);
		kfree(pdata_ssc_flip);
		return -ENOMEM;
	}
	INIT_WORK(&pdata_ssc_flip->work_ssc_flip, ssc_flip_work_func);
#endif

#ifdef CONFIG_SUPPORT_SSC_MODE
	platform_driver_register(&ssc_core_driver);
#endif

	pdata_sdump = kzalloc(sizeof(*pdata_sdump), GFP_KERNEL);
	if (pdata_sdump == NULL)
		return -ENOMEM;

	pdata_sdump->sdump_wq = create_singlethread_workqueue("sdump_wq");
	if (pdata_sdump->sdump_wq == NULL) {
		pr_err("[FACTORY]: %s - could not create sdump wq\n", __func__);
		kfree(pdata_sdump);
		return -ENOMEM;
	}
	INIT_WORK(&pdata_sdump->work_sdump, sensor_dump_work_func);

	INIT_DELAYED_WORK(&ssr_dump_work, ssr_dump_work_func);

#ifdef CONFIG_VBUS_NOTIFIER
	pdata_ssc_charge = kzalloc(sizeof(*pdata_ssc_charge), GFP_KERNEL);
	if (pdata_ssc_charge == NULL)
		return -ENOMEM;

	pdata_ssc_charge->ssc_charge_wq =
		create_singlethread_workqueue("ssc_charge_wq");
	if (pdata_ssc_charge->ssc_charge_wq == NULL) {
		pr_err("[FACTORY]: %s - couldn't create ssc charge wq\n", __func__);
		kfree(pdata_ssc_charge);
		return -ENOMEM;
	}
	INIT_WORK(&pdata_ssc_charge->work_ssc_charge, ssc_charge_work_func);
	pdata_ssc_charge->is_charging = false;
	vbus_notifier_register(&data->vbus_nb,
		ssc_core_vbus_notifier, VBUS_NOTIFY_DEV_CHARGER);
#endif
	pr_info("[FACTORY] %s\n", __func__);

	return 0;
}

void __exit core_factory_exit(void)
{
#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE) || defined(CONFIG_VBUS_NOTIFIER) || IS_ENABLED(CONFIG_HALL_NOTIFIER)
	struct adsp_data *data = adsp_ssc_core_unregister(MSG_SSC_CORE);;
#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
	hall_notifier_unregister(&data->adsp_nb);
#endif
#ifdef CONFIG_VBUS_NOTIFIER
	vbus_notifier_unregister(&data->vbus_nb);
#endif
#else
	adsp_factory_unregister(MSG_SSC_CORE);
#endif
#ifdef CONFIG_VBUS_NOTIFIER
	if (pdata_ssc_charge != NULL && pdata_ssc_charge->ssc_charge_wq != NULL) {
		cancel_work_sync(&pdata_ssc_charge->work_ssc_charge);
		destroy_workqueue(pdata_ssc_charge->ssc_charge_wq);
		pdata_ssc_charge->ssc_charge_wq = NULL;
	}
#endif

#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE)
	if (pdata_ssc_flip != NULL && pdata_ssc_flip->ssc_flip_wq != NULL) {
		cancel_work_sync(&pdata_ssc_flip->work_ssc_flip);
		destroy_workqueue(pdata_ssc_flip->ssc_flip_wq);
		pdata_ssc_flip->ssc_flip_wq = NULL;
	}
#endif

#ifdef CONFIG_SUPPORT_SSC_MODE
	platform_driver_unregister(&ssc_core_driver);
#endif
	if (pdata_sdump->sdump_wq)
		destroy_workqueue(pdata_sdump->sdump_wq);
	if (pdata_sdump)
		kfree(pdata_sdump);

	pr_info("[FACTORY] %s\n", __func__);
}
