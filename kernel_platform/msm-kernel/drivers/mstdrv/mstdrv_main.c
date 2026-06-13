/*
 * @file mstdrv.c
 * @brief MST drv Support
 * Copyright (c) 2015, Samsung Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/kern_levels.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/percpu-defs.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/threads.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#if defined(CONFIG_MST_ARCH_EXYNOS)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
#include <soc/samsung/exynos-pmu-if.h>
#endif
#endif
#if defined(CONFIG_MFC_CHARGER)
#include <linux/battery/sec_battery_common.h>
#endif
#include "mstdrv_main.h"

/* defines */
#if defined(CONFIG_MST_VOUT_SETTING)
#define WC_TX_VOUT_5000MV		5000
#define WC_TX_VOUT_5500MV		5500
#define WC_TX_VOUT_6000MV		6000
#define UNO						0x08
#define UNO_OTG					0x0B
#define UNO_BUCK				0x0C
#define UNO_CHARGING			0x02
#define UNO_BUCK_CHARGING		0x0D
#endif
#define WIRELESS_TX_IOUT_1000		1000
#define WIRELESS_TX_IOUT_1500		1500
#define WIRELESS_TX_IOUT_1600		1600
#define WIRELESS_TX_IOUT_2000		2000

#define	ON				1	// On state
#define	OFF				0	// Off state
#define	TRACK1				1	// Track1 data
#define	TRACK2				2	// Track2 data

#define CMD_MST_LDO_OFF			'0'	// MST LDO off
#define CMD_MST_LDO_ON			'1'	// MST LDO on
#define CMD_SEND_TRACK1_DATA		'2'	// Send track1 test data
#define CMD_SEND_TRACK2_DATA		'3'	// send track2 test data
#define ERROR_VALUE			-1	// Error value

#if !defined(CONFIG_MST_DUMMY_DRV)
/* global variables */
struct workqueue_struct *cluster_freq_ctrl_wq;
struct delayed_work dwork;
static struct class *mst_drv_class;
struct device *mst_drv_dev;
static int nfc_state;
#if !defined(CONFIG_MST_IF_PMIC)
static int mst_pwr_en;
#endif
#if defined(CONFIG_MST_SUPPORT_GPIO)
static int mst_support_check;
#endif
#if defined(CONFIG_MST_PCR)
static bool mst_pwr_use_uno;
#endif
#endif
/* function */
/**
 * mst_printk - print with mst tag
 */
int mst_log_level = MST_LOG_LEVEL;
void mst_printk(int level, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	if (mst_log_level < level)
		return;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	printk("%s %pV", TAG, &vaf);

	va_end(args);
}

#if !defined(CONFIG_MST_DUMMY_DRV)
#if defined(CONFIG_MST_ARCH_QCOM)
uint32_t ss_mst_bus_hdl;

DEFINE_MUTEX(mst_mutex);
#if defined(_ARCH_ARM_MACH_MSM_BUS_H) // build error
/**
 * boost_enable - set all CPUs freq upto Max
 * boost_disable - set all CPUs freq back to Normal
 */
static int boost_enable(void)
{
	mst_info("%s: bump up snoc clock request\n", __func__);
	if (0 == ss_mst_bus_hdl){
		ss_mst_bus_hdl = msm_bus_scale_register_client(&ss_mst_bus_client_pdata);
		if (ss_mst_bus_hdl) { 
			if(msm_bus_scale_client_update_request(ss_mst_bus_hdl, 1)) {
				mst_err("%s: fail to update request!\n", __func__);
				WARN_ON(1);
				msm_bus_scale_unregister_client(ss_mst_bus_hdl);
				ss_mst_bus_hdl = 0;
			}
		} else {
			mst_err("%s: fail to register client, ss_mst_bus_hdl = %d\n",
				__func__, ss_mst_bus_hdl);
		}
	}
	return ss_mst_bus_hdl;
}

static int boost_disable(void)
{
	mst_info("%s: bump up snoc clock remove\n", __func__);
	if (ss_mst_bus_hdl) {
		if(msm_bus_scale_client_update_request(ss_mst_bus_hdl, 0))
			WARN_ON(1);
		msm_bus_scale_unregister_client(ss_mst_bus_hdl);
		ss_mst_bus_hdl = 0;
	} else {
		mst_err("%s: fail to unregister client, ss_mst_bus_hdl = %d\n",
			__func__, ss_mst_bus_hdl);
	}
	return ss_mst_bus_hdl;
}
#endif
#endif

/**
 * mst_ctrl_of_mst_hw_onoff - function to disable/enable MST when NFC on
 * @on: on/off value
 */
extern void mst_ctrl_of_mst_hw_onoff(bool on)
{
#if defined(CONFIG_MFC_CHARGER)
	union power_supply_propval value;	/* power_supply prop */
#endif
#if defined(CONFIG_MST_SUPPORT_GPIO)
	uint8_t is_mst_support = 0;

	is_mst_support = gpio_get_value(mst_support_check);
	if (is_mst_support == 0) {
		mst_info("%s: MST not supported, no need nfc control, %d\n",
			 __func__, is_mst_support);
		return;
	}
#endif
	mst_info("%s: on = %d\n", on);

	if (on) {
		nfc_state = 0;
		mst_info("%s: nfc_state unlocked, %d\n", __func__, nfc_state);
	} else {
#if !defined(CONFIG_MST_IF_PMIC)
		gpio_set_value(mst_pwr_en, 0);
		mst_info("%s: mst_pwr_en LOW\n", __func__);
#if defined(CONFIG_MST_PCR)
		if (mst_pwr_use_uno) {
			value.intval = OFF;
			psy_do_property("battery", set,
					POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL, value);

			value.intval = WIRELESS_TX_IOUT_1000;
			psy_do_property("otg", set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_TX_IOUT, value);

#if defined(CONFIG_MST_VOUT_SETTING)
			mst_info("%s: Setting VOUT as 5000MV when MST is OFF, VOUT = %d\n", __func__, WC_TX_VOUT_5000MV);
			value.intval = WC_TX_VOUT_5000MV;								// Setting VOUT as 5000MV when MST is OFF
			psy_do_property("otg", set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);
#endif
		}
#endif
#endif
		usleep_range(800, 1000);

#if defined(CONFIG_MFC_CHARGER)
		value.intval = OFF;
		psy_do_property("mfc-charger", set,
				POWER_SUPPLY_PROP_TECHNOLOGY, value);
		mst_info("%s: MST_MODE notify : %d\n", __func__, value.intval);

		value.intval = 0;
		psy_do_property("mfc-charger", set, POWER_SUPPLY_EXT_PROP_WPC_EN_MST, value);
		mst_info("%s : MFC_IC Disable notify : %d\n", __func__, value.intval);
#endif
#if defined(CONFIG_MST_ARCH_QCOM)
#if defined(_ARCH_ARM_MACH_MSM_BUS_H) // build error
		/* Boost Disable */
		mst_info("%s: boost disable", __func__);
		mutex_lock(&mst_mutex);
		if (boost_disable())
			mst_err("%s: boost disable failed\n",__func__);
		mutex_unlock(&mst_mutex);
#endif
#endif
		nfc_state = 1;
		mst_info("%s: nfc_state locked, %d\n", __func__, nfc_state);
	}
}

/**
 * of_mst_hw_onoff - Enable/Disable MST LDO GPIO pin
 * @on: on/off value
 */
static void of_mst_hw_onoff(bool on)
{
#if defined(CONFIG_MFC_CHARGER)
	union power_supply_propval value;	/* power_supply prop */
	int retry_cnt = 8;
#endif
	mst_info("%s: on = %d\n", __func__, on);

	if (nfc_state == 1) {
		mst_info("%s: nfc_state on!!!\n", __func__);
		return;
	}

	if (on) {
#if defined(CONFIG_MFC_CHARGER)
		mst_info("%s : MFC_IC Enable notify start\n", __func__);
		value.intval = 1;
		psy_do_property("mfc-charger", set, POWER_SUPPLY_EXT_PROP_WPC_EN_MST, value);
		mst_info("%s : MFC_IC Enable notified : %d\n", __func__, value.intval);

		value.intval = ON;
		psy_do_property("mfc-charger", set,
				POWER_SUPPLY_PROP_TECHNOLOGY, value);
		mst_info("%s: MST_MODE notify : %d\n", __func__, value.intval);

		psy_do_property("mfc-charger", get, POWER_SUPPLY_EXT_PROP_MST_DELAY, value);
		mode_set_wait = value.intval;
		mst_info("%s: Delay for MST : %d ms\n", __func__, mode_set_wait);
#endif

#if !defined(CONFIG_MST_IF_PMIC)
#if defined(CONFIG_MST_PCR)
		if (mst_pwr_use_uno) {
#if !defined(CONFIG_MST_VOUT_SETTING)
			value.intval = WIRELESS_TX_IOUT_1500;
			psy_do_property("otg", set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_TX_IOUT, value);
#endif
			value.intval = ON;
			psy_do_property("battery", set,
					POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL, value);

#if defined(CONFIG_MST_VOUT_SETTING)
			psy_do_property("otg", get, 
				POWER_SUPPLY_EXT_PROP_CHG_MODE, value);

			if(value.intval == UNO || value.intval == UNO_BUCK){
				mst_info("%s : MST using UNO \n", __func__);
				value.intval = WIRELESS_TX_IOUT_2000;											// Setting IOUT as 2A when MST (UNO) is ON
				psy_do_property("otg", set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_TX_IOUT, value);

				value.intval = WC_TX_VOUT_6000MV;								// Setting VOUT as 6000MV when MST (UNO) is ON
				psy_do_property("otg", set,
						POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);
			}
			else if(value.intval == UNO_OTG || value.intval == UNO_BUCK_CHARGING || value.intval == UNO_CHARGING){
				mst_info("%s : MST using UNO+OTG \n", __func__);
				value.intval = WC_TX_VOUT_5000MV;								// Setting VOUT as 5000MV when MST (UNO+OTG) is ON
				psy_do_property("otg", set,
						POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);

				value.intval = WIRELESS_TX_IOUT_1600;											// Setting IOUT as 1.6A when MST (UNO+OTG) is ON
				psy_do_property("otg", set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_TX_IOUT, value);
			}
#endif
		}
#endif
#if defined(CONFIG_MST_ARCH_EXYNOS)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
		exynos_pmu_update(0x1960, 0x00000800, 0x00000800);	
#endif
#endif
		gpio_set_value(mst_pwr_en, 1);
		mst_info("%s: mst_pwr_en HIGH\n", __func__);
#endif

		cancel_delayed_work_sync(&dwork);
		queue_delayed_work(cluster_freq_ctrl_wq, &dwork, 90 * HZ);

#if defined(CONFIG_MST_ARCH_QCOM)
#if defined(_ARCH_ARM_MACH_MSM_BUS_H) // build error
		/* Boost Enable */
		mst_info("%s: boost enable for performacne", __func__);
		mutex_lock(&mst_mutex);
		if (!boost_enable())
			mst_err("%s: boost enable is failed\n", __func__);
		mutex_unlock(&mst_mutex);
#endif
#endif
		msleep(mode_set_wait);

#if defined(CONFIG_MFC_CHARGER)
		while (--retry_cnt) {
			psy_do_property("mfc-charger", get, POWER_SUPPLY_EXT_PROP_MST_MODE, value);
			if (value.intval > 0) {
				mst_info("%s: mst mode set!!! : %d\n", __func__,
					 value.intval);
				retry_cnt = 1;
				break;
			}
			usleep_range(3600, 4000);
		}

		if (!retry_cnt) {
			mst_info("%s: timeout !!! : %d\n", __func__,
				 value.intval);
		}
#endif
	} else {
#if !defined(CONFIG_MST_IF_PMIC)
		gpio_set_value(mst_pwr_en, 0);
		mst_info("%s: mst_pwr_en LOW\n", __func__);
#if defined(CONFIG_MST_PCR)
		if (mst_pwr_use_uno) {
			value.intval = OFF;
			psy_do_property("battery", set,
					POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL, value);

			value.intval = WIRELESS_TX_IOUT_1000;
			psy_do_property("otg", set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_TX_IOUT, value);

#if defined(CONFIG_MST_VOUT_SETTING)
			mst_info("%s: Setting VOUT as 5000MV when MST is OFF, VOUT = %d\n", __func__, WC_TX_VOUT_5000MV);
			value.intval = WC_TX_VOUT_5000MV;								// Setting VOUT as 5000MV when MST is OFF
			psy_do_property("otg", set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);
#endif
		}
#endif
#endif
		usleep_range(800, 1000);

#if defined(CONFIG_MFC_CHARGER)
		value.intval = OFF;
		psy_do_property("mfc-charger", set,
				POWER_SUPPLY_PROP_TECHNOLOGY, value);
		mst_info("%s: MST_MODE notify : %d\n", __func__,
			 value.intval);

		value.intval = 0;
		psy_do_property("mfc-charger", set, POWER_SUPPLY_EXT_PROP_WPC_EN_MST, value);
		mst_info("%s : MFC_IC Disable notify : %d\n", __func__, value.intval);
#endif

#if defined(CONFIG_MST_ARCH_QCOM)
#if defined(_ARCH_ARM_MACH_MSM_BUS_H) // build error
		/* Boost Disable */
		mst_info("%s: boost disable", __func__);
		mutex_lock(&mst_mutex);
		if (boost_disable())
			mst_err("%s: boost disable is failed\n", __func__);
		mutex_unlock(&mst_mutex);
#endif
#endif
	}
}

#if !defined(CONFIG_MST_NONSECURE)
#if defined(CONFIG_MST_TEEGRIS)
int transmit_mst_data(uint32_t track_number)
{
	TEEC_Context context;
	TEEC_Session session_ta;
	TEEC_Operation operation;
	TEEC_Result ret;

	uint32_t origin;

	origin = 0x0;
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE,
						TEEC_NONE, TEEC_NONE);

	mst_info("%s: TEEC_InitializeContext\n", __func__);
	ret = TEEC_InitializeContext(NULL, &context);
	if (ret != TEEC_SUCCESS) {
		mst_err("%s: InitializeContext failed, %d\n", __func__, ret);
		goto exit;
	}

	mst_info("%s: TEEC_OpenSession\n", __func__);
	ret = TEEC_OpenSession(&context, &session_ta, &uuid_ta, 0,
			       NULL, &operation, &origin);
	if (ret != TEEC_SUCCESS) {
		mst_err("%s: OpenSession(ta) failed, %d\n", __func__, ret);
		goto finalize_context;
	}

	mst_info("%s: TEEC_InvokeCommand (CMD_OPEN)\n", __func__);
	ret = TEEC_InvokeCommand(&session_ta, CMD_OPEN, &operation, &origin);
	if (ret != TEEC_SUCCESS) {
		mst_err("%s: InvokeCommand(OPEN) failed, %d\n", __func__, ret);
		goto ta_close_session;
	}

	/* MST IOCTL - transmit track data */
	mst_info("tracing mark write: MST transmission Start\n");
	mst_info("%s: TEEC_InvokeCommand (TRACK1 or TRACK2)\n", __func__);
	ret = TEEC_InvokeCommand(&session_ta, track_number + 2, &operation, &origin);
	if (ret != TEEC_SUCCESS) {
		mst_err("%s: InvokeCommand failed, %d\n", __func__, ret);
		goto ta_close_session;
	}
	mst_info("tracing mark write: MST transmission End\n");

	if (ret) {
		mst_info("%s: Send track%d data --> failed\n", __func__, track_number);
	} else {
		mst_info("%s: Send track%d data --> success\n", __func__, track_number);
	}

	mst_info("%s: TEEC_InvokeCommand (CMD_CLOSE)\n", __func__);
	ret = TEEC_InvokeCommand(&session_ta, CMD_CLOSE, &operation, &origin);
	if (ret != TEEC_SUCCESS) {
		mst_err("%s: InvokeCommand(CLOSE) failed, %d\n", __func__, ret);
		goto ta_close_session;
	}

ta_close_session:
	TEEC_CloseSession(&session_ta);
finalize_context:
	TEEC_FinalizeContext(&context);
exit:
	return ret;
}
#elif defined(CONFIG_MST_ARCH_QCOM)
int transmit_mst_data(int track_num)
{
	int ret = 0;
	int qsee_ret = 0;
	int retry = 5;
	bool is_loaded = false;
	mst_req_t *kreq = NULL;
	mst_rsp_t *krsp = NULL;
	int req_len = 0, rsp_len = 0;
	// Core Affinity
	struct cpumask cpumask;
	uint32_t cpu;

	if (!mutex_trylock(&transmit_mutex)) {
		printk("[MST] failed to acquire transmit_mutex!\n");
		return ERROR_VALUE;
	}
	if (NULL == qhandle) {
		while(retry > 0) {
			/* start the mst tzapp only when it is not loaded. */
			qsee_ret = qseecom_start_app(&qhandle, MST_TA, 1024);
			if (qsee_ret == 0) {
				is_loaded = true;
				break;
			}
			mst_err("%s: failed to qseecom_start_app with %d, retry_cnt = %d\n", __func__, qsee_ret, retry);
			retry--;
		}
		if (!is_loaded) {
			/* It seems we couldn't start mst tzapp. */
			mst_err("%s: failed to load MST TA, %d\n", __func__, qsee_ret);
			ret = ERROR_VALUE;
			goto exit;	/* leave the function now. */
		}
	}

	kreq = (struct mst_req_s *)qhandle->sbuf;

	switch (track_num) {
	case TRACK1:
		kreq->cmd_id = MST_CMD_TRACK1_TEST;
		break;

	case TRACK2:
		kreq->cmd_id = MST_CMD_TRACK2_TEST;
		break;

	default:
		ret = ERROR_VALUE;
		goto exit;
		break;
	}

	req_len = sizeof(mst_req_t);
	krsp = (struct mst_rsp_s *)(qhandle->sbuf + req_len);
	rsp_len = sizeof(mst_rsp_t);

	// Core Affinity
	printk("[MST] sched_setaffinity not to run on core0");
	if (num_online_cpus() < 2) {
		cpumask_setall(&cpumask);
		for_each_cpu(cpu, &cpumask) {
		if (cpu == 0)
			continue;
		ADD_CPU(cpu);
		break;
		}
	}
	cpumask_clear(&cpumask);
	cpumask_copy(&cpumask, cpu_online_mask);
	cpumask_clear_cpu(0, &cpumask);

	SCHED_SETAFFINITY(current, cpumask);

	mst_info("%s: cmd_id = %x, req_len = %d, rsp_len = %d\n", __func__,
		 kreq->cmd_id, req_len, rsp_len);

	mst_info("tracing mark write: MST transmission Start\n");
	qsee_ret = qseecom_send_command(qhandle, kreq, req_len, krsp, rsp_len);
	mst_info("tracing mark write: MST transmission End\n");
	if (qsee_ret) {
		ret = ERROR_VALUE;
		mst_err("%s: failed to send cmd, %d\n", __func__, qsee_ret);
	}

	if (krsp->status) {
		ret = ERROR_VALUE;
		mst_err("%s: generate track data from TZ failed, %d\n",
			__func__, krsp->status);
	}

	if (ret) {
		mst_info("%s: Send track%d data --> failed\n", __func__, track_num);
	} else {
		mst_info("%s: Send track%d data --> success\n", __func__, track_num);
	}

	mst_info("%s: shutting down the tzapp\n", __func__);
	qsee_ret = qseecom_shutdown_app(&qhandle);
	if (qsee_ret) {
		mst_err("%s: failed to shut down the tzapp\n", __func__);
	} else {
		qhandle = NULL;
	}
exit:
        mutex_unlock(&transmit_mutex);
	return ret;
}
#else
int transmit_mst_data(int track_num)
{
	int ret = 0;
#if defined(CONFIG_MST_ARCH_MTK)
	struct arm_smccc_res res;
#endif

#if defined(CONFIG_MST_ARCH_EXYNOS)
	ret = exynos_smc((0x8300000f), track_num, 0, 0);
#elif defined(CONFIG_MST_ARCH_MTK)
	arm_smccc_smc(MTK_SIP_KERNEL_MST_TEST_TRANSMIT, track_num, 0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	mst_debug("%s: result of ATF SMC call for MST transmit(0x%x, %d), ret0 : %lu\n", __func__, MTK_SIP_KERNEL_MST_TEST_TRANSMIT, track_num, res.a0);
#endif
	
	return ret;
}
#endif
#endif

#if !defined(CONFIG_MST_IF_PMIC)
int init_mst_pwr_en_gpio(struct device *dev)
{
	int ret = 0;
	mst_pwr_en = of_get_named_gpio(dev->of_node, "sec-mst,mst-pwr-gpio", 0);
	if (mst_pwr_en < 0) {
		mst_err("%s: fail to get pwr gpio, %d\n", __func__, mst_pwr_en);
		ret = -1;
		return ret;
	}

	ret = gpio_request(mst_pwr_en, "sec-mst,mst-pwr-gpio");
	if (ret) {
		mst_err("%s: failed to request pwr gpio, %d, %d\n",
			__func__, ret, mst_pwr_en);
	}

	mst_info("%s: gpio pwr en inited. Data_Value_ mst_pwr_en : %d\n", __func__,mst_pwr_en);
	
	if (!(ret < 0) && (mst_pwr_en > 0)) {
		gpio_direction_output(mst_pwr_en, 0);
		mst_info("%s: mst_pwr_en output\n", __func__);
	}

	return ret;
}
#endif


/**
 * show_mst_drv - device attribute show sysfs operation
 */
static ssize_t show_mst_drv(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	if (!dev)
		return -ENODEV;

	return sprintf(buf, "%s\n", "waiting");
}

/**
 * store_mst_drv - device attribute store sysfs operation
 */
static ssize_t store_mst_drv(struct device *dev,
			     struct device_attribute *attr, const char *buf,
			     size_t count)
{
	char in = 0;
	int ret = 0;

	sscanf(buf, "%c\n", &in);
	mst_info("%s: in = %c\n", __func__, in);

	switch (in) {
	case CMD_MST_LDO_OFF:
		of_mst_hw_onoff(OFF);
		break;

	case CMD_MST_LDO_ON:
		of_mst_hw_onoff(ON);
		break;

	case CMD_SEND_TRACK1_DATA:
#if !defined(CONFIG_MFC_CHARGER)
		of_mst_hw_onoff(ON);
#endif
		mst_info("%s: send track1 data\n", __func__);
		ret = transmit_mst_data(TRACK1);
#if !defined(CONFIG_MFC_CHARGER)
		of_mst_hw_onoff(OFF);
#endif
		break;

	case CMD_SEND_TRACK2_DATA:
#if !defined(CONFIG_MFC_CHARGER)
		of_mst_hw_onoff(ON);
#endif
		mst_info("%s: send track2 data\n", __func__);
		ret = transmit_mst_data(TRACK2);
#if !defined(CONFIG_MFC_CHARGER)
		of_mst_hw_onoff(OFF);
#endif
		break;

	default:
		mst_err("%s: invalid value : %c\n", __func__, in);
		break;
	}
	return count;
}
static DEVICE_ATTR(transmit, 0770, show_mst_drv, store_mst_drv);

/* support node */
static ssize_t show_support(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_MST_SUPPORT_GPIO)
	uint8_t is_mst_support = 0;
#endif

	if (!dev)
		return -ENODEV;

#if defined(CONFIG_MST_SUPPORT_GPIO)
	is_mst_support = gpio_get_value(mst_support_check);
	if (is_mst_support == 1) {
		mst_info("%s: Support MST!, %d\n", __func__, is_mst_support);
		return sprintf(buf, "%d\n", 1);
	} else {
		mst_info("%s: Not support MST!, %d\n", __func__, is_mst_support);
		return sprintf(buf, "%d\n", 0);
	}
#else
	mst_info("%s: MST_LDO enabled, support MST\n", __func__);
	return sprintf(buf, "%d\n", 1);
#endif
}

static ssize_t store_support(struct device *dev,
			     struct device_attribute *attr, const char *buf,
			     size_t count)
{
	return count;
}
static DEVICE_ATTR(support, 0444, show_support, store_support);

/* mfc_charger node */
#if defined(CONFIG_MFC_CHARGER)
static ssize_t show_mfc(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	if (!dev)
		return -ENODEV;

	return sprintf(buf, "%s\n", "mfc_charger");
}

static ssize_t store_mfc(struct device *dev,
			 struct device_attribute *attr, const char *buf,
			 size_t count)
{
	return count;
}
static DEVICE_ATTR(mfc, 0770, show_mfc, store_mfc);
#endif

/**
 * sec_mst_gpio_init - Initialize GPIO pins used by driver
 * @dev: driver handle
 */
static int sec_mst_gpio_init(struct device *dev)
{
	int ret = 0;
#if !defined(CONFIG_MST_IF_PMIC)
	ret = init_mst_pwr_en_gpio(dev);
	if (ret != 0)
		return ret;
#endif

#if defined(CONFIG_MST_NONSECURE)
	ret = init_mst_en_gpio(dev);
	if (ret != 0)
		return ret;
	ret = init_mst_data_gpio(dev);
	if (ret != 0)
		return ret;
#endif
	return ret;
}

static int mst_ldo_device_probe(struct platform_device *pdev)
{
	int retval = 0;
#if defined(CONFIG_MST_SUPPORT_GPIO) || defined(CONFIG_MST_PCR)
	struct device *dev = &pdev->dev;
#endif
#if defined(CONFIG_MST_SUPPORT_GPIO)
	uint8_t is_mst_support = 0;
#endif

	mst_info("%s: probe start\n", __func__);

#if defined(CONFIG_MST_SUPPORT_GPIO)
	/* MST support/non-support node check gpio */
	mst_support_check = of_get_named_gpio(dev->of_node, "sec-mst,mst-support-gpio", 0);
	mst_info("%s: mst_support_check, %d\n", __func__, mst_support_check);
	if (mst_support_check < 0) {
		mst_err("%s: fail to get support gpio, %d\n",
			__func__, mst_support_check);
		return -1;
	}
	mst_info("%s: gpio support_check inited\n", __func__);

	is_mst_support = gpio_get_value(mst_support_check);
	if (is_mst_support == 1) {
		mst_info("%s: Support MST!, %d\n", __func__, is_mst_support);
	} else {
		mst_info("%s: Not support MST!, %d\n", __func__, is_mst_support);

		mst_info("%s: create sysfs node\n", __func__);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0))
		mst_drv_class = class_create("mstldo");
#else
		mst_drv_class = class_create(THIS_MODULE, "mstldo");
#endif
		if (IS_ERR(mst_drv_class)) {
			retval = PTR_ERR(mst_drv_class);
			goto error;
		}

		mst_drv_dev = device_create(mst_drv_class,
					    NULL /* parent */, 0 /* dev_t */,
					    NULL /* drvdata */,
					    MST_DRV_DEV);
		if (IS_ERR(mst_drv_dev)) {
			retval = PTR_ERR(mst_drv_dev);
			goto error_destroy;
		}

		retval = device_create_file(mst_drv_dev, &dev_attr_support);
		if (retval)
			goto error_destroy;

		return -1;
	}
#endif

#if defined(CONFIG_MST_PCR)
	mst_pwr_use_uno = of_property_read_bool(dev->of_node, "sec-mst,mst-use-uno-power");
	if (mst_pwr_use_uno) {
		mst_info("%s: support UNO for mst power\n", __func__);
	} else {
		mst_info("%s: Not support UNO. use VPH_PWR\n", __func__);
	}
#endif

#if defined(CONFIG_MST_NONSECURE)
	init_spin_lock();
#endif

	if (sec_mst_gpio_init(&pdev->dev))
		return -1;

	mst_info("%s: create sysfs node\n", __func__);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0))
	mst_drv_class = class_create("mstldo");
#else
	mst_drv_class = class_create(THIS_MODULE, "mstldo");
#endif
	if (IS_ERR(mst_drv_class)) {
		retval = PTR_ERR(mst_drv_class);
		goto error;
	}

	mst_drv_dev = device_create(mst_drv_class,
				    NULL /* parent */, 0 /* dev_t */,
				    NULL /* drvdata */,
				    MST_DRV_DEV);
	if (IS_ERR(mst_drv_dev)) {
		retval = PTR_ERR(mst_drv_dev);
		goto error_destroy;
	}

	/* register this mst device with the driver core */
	retval = device_create_file(mst_drv_dev, &dev_attr_transmit);
	if (retval)
		goto error_destroy;

	retval = device_create_file(mst_drv_dev, &dev_attr_support);
	if (retval)
		goto error_destroy;

#if defined(CONFIG_MFC_CHARGER)
	retval = device_create_file(mst_drv_dev, &dev_attr_mfc);
	if (retval)
		goto error_destroy;
#endif

	mst_info("%s: MST driver(%s) initialized\n", __func__, MST_DRV_DEV);
	return 0;

error_destroy:
	kfree(mst_drv_dev);
	device_destroy(mst_drv_class, 0);
error:
	mst_info("%s: MST driver(%s) init failed\n", __func__, MST_DRV_DEV);
	return retval;
}
EXPORT_SYMBOL_GPL(mst_drv_dev);

/**
 * device suspend - function called device goes suspend
 */
static int mst_ldo_device_suspend(struct platform_device *dev,
				  pm_message_t state)
{
#if !defined(CONFIG_MST_IF_PMIC)
	uint8_t is_mst_pwr_on;
	is_mst_pwr_on = gpio_get_value(mst_pwr_en);
	if (is_mst_pwr_on == 1) {
		mst_info("%s: mst power is on, %d\n", __func__, is_mst_pwr_on);
		of_mst_hw_onoff(0);
		mst_info("%s: mst power off\n", __func__);
	} else {
		mst_info("%s: mst power is off, %d\n", __func__, is_mst_pwr_on);
	}
#endif
	return 0;
}

static struct of_device_id mst_match_ldo_table[] = {
	{.compatible = "sec-mst",},
	{},
};

static struct platform_driver sec_mst_ldo_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "mstldo",
		.of_match_table = mst_match_ldo_table,
	},
	.probe = mst_ldo_device_probe,
	.suspend = mst_ldo_device_suspend,
};

/**
 * mst delayed work
 */
static void mst_cluster_freq_ctrl_worker(struct work_struct *work)
{
#if !defined(CONFIG_MST_IF_PMIC)
	uint8_t is_mst_pwr_on;
	is_mst_pwr_on = gpio_get_value(mst_pwr_en);
	if (is_mst_pwr_on == 1) {
		mst_info("%s: mst power is on, %d\n", __func__, is_mst_pwr_on);
		of_mst_hw_onoff(0);
		mst_info("%s: mst power off\n", __func__);
	} else {
		mst_info("%s: mst power is off, %d\n", __func__, is_mst_pwr_on);
	}
#endif
	return;
}

#endif
/**
 * mst_drv_init - Driver init function
 */
static int __init mst_drv_init(void)
{
	int ret = 0;
#if !defined(CONFIG_MST_DUMMY_DRV)
	mst_info("%s\n", __func__);
	ret = platform_driver_register(&sec_mst_ldo_driver);
	mst_info("%s: init , ret : %d\n", __func__, ret);

	cluster_freq_ctrl_wq =
		create_singlethread_workqueue("mst_cluster_freq_ctrl_wq");
	INIT_DELAYED_WORK(&dwork, mst_cluster_freq_ctrl_worker);
#else
	mst_info("%s: create dummy driver\n", __func__);
#endif
	return ret;
}

/**
 * mst_drv_exit - Driver exit function
 */
static void __exit mst_drv_exit(void)
{
#if !defined(CONFIG_MST_DUMMY_DRV)
	class_destroy(mst_drv_class);
	mst_info("%s\n", __func__);
#else
	mst_info("%s: destroy dummy driver\n", __func__);
#endif

}

MODULE_AUTHOR("yurak.choe@samsung.com");
MODULE_DESCRIPTION("MST QC/LSI combined driver");
MODULE_VERSION("1.0");
late_initcall(mst_drv_init);
module_exit(mst_drv_exit);
MODULE_LICENSE("GPL v2");
