/*
 * @file mstdrv.h
 * @brief Header file for MST driver
 * Copyright (c) 2015~2019, Samsung Electronics Corporation. All rights reserved.
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

#ifndef MST_DRV_H
#define MST_DRV_H

#include <linux/of_gpio.h>
#if defined(CONFIG_MST_ARCH_QCOM)
#if defined(_ARCH_ARM_MACH_MSM_BUS_H) // build error
#include <linux/msm-bus.h>
#endif
#if IS_MODULE(CONFIG_SCHED_WALT)	// QC recommend (CN 05456519)
#include <linux/sched/walt.h>
#else
#include <linux/sched/core_ctl.h>
#endif
#include <linux/msm_pcie.h>
#if IS_ENABLED(CONFIG_QSEECOM_PROXY)
#include <linux/qseecom_kernel.h>
#endif
#endif

#if defined(CONFIG_MST_ARCH_EXYNOS)
#include <linux/smc.h> // for Kinibi
#endif

#if defined(CONFIG_MST_ARCH_MTK) && !defined(CONFIG_MST_TEEGRIS) && !defined(CONFIG_MST_NONSECURE) // for Kinibi
#include <linux/arm-smccc.h>
#include <mt-plat/mtk_secure_api.h>
#endif

#if defined(CONFIG_MST_TEEGRIS)
#if defined(CONFIG_TEEGRIS_VERSION) && (CONFIG_TEEGRIS_VERSION == 6)
#include "../../drivers/tee/tzdev/include/tzdev/tee_client_api.h"
#else
#include "../../drivers/misc/tzdev/include/tzdev/tee_client_api.h"
#endif
#endif

#if defined(CONFIG_MST_NONSECURE)
#include "mstdrv_transmit_nonsecure.h"
#endif

/* for logging */
#include <linux/printk.h>
void mst_printk(int level, const char *fmt, ...);
#define DEV_ERR			(1)
#define DEV_WARN		(2)
#define DEV_NOTI		(3)
#define DEV_INFO		(4)
#define DEV_DEBUG		(5)
#define MST_LOG_LEVEL		DEV_INFO
#define MST_DRV_DEV			"mst_drv"
#define TAG				"[sec_mst]"

#define mst_err(fmt, ...)	mst_printk(DEV_ERR, fmt, ## __VA_ARGS__);
#define mst_warn(fmt, ...)	mst_printk(DEV_WARN, fmt, ## __VA_ARGS__);
#define mst_noti(fmt, ...)	mst_printk(DEV_NOTI, fmt, ## __VA_ARGS__);
#define mst_info(fmt, ...)	mst_printk(DEV_INFO, fmt, ## __VA_ARGS__);
#define mst_debug(fmt, ...)	mst_printk(DEV_DEBUG, fmt, ## __VA_ARGS__);

static uint32_t mode_set_wait = 40;

typedef enum {
	MFC_CHIP_ID_IDT = 1,
	MFC_CHIP_ID_LSI,
} mfc_chip_vendor;

#if defined(CONFIG_MST_ARCH_QCOM)
#if !defined(CONFIG_MST_NONSECURE)
#define SVC_MST_ID			0x000A0000	// need to check ID
#define MST_CREATE_CMD(x)		(SVC_MST_ID | x)	// Create MST commands
#define MST_TA				"mst"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 27))
#define ADD_CPU(cpu) add_cpu(cpu)
#else
#define ADD_CPU(cpu) cpu_up(cpu)
#endif

#if IS_MODULE(CONFIG_MST_LDO)
#define SCHED_SETAFFINITY(current, cpumask) set_cpus_allowed_ptr(current, &cpumask)
#else
#define SCHED_SETAFFINITY(current, cpumask) sched_setaffinity(0, &cpumask)
#endif

/* enum definitions */
typedef enum {
	MST_CMD_TRACK1_TEST = MST_CREATE_CMD(0x00000000),
	MST_CMD_TRACK2_TEST = MST_CREATE_CMD(0x00000001),
	MST_CMD_UNKNOWN = MST_CREATE_CMD(0x7FFFFFFF)
} mst_cmd_type;

#if !IS_ENABLED(CONFIG_QSEECOM_PROXY)
/* struct definitions */
struct qseecom_handle {
	void *dev;		/* in/out */
	unsigned char *sbuf;	/* in/out */
	uint32_t sbuf_len;	/* in/out */
};
#endif
static struct qseecom_handle *qhandle;

typedef struct mst_req_s {
	mst_cmd_type cmd_id;
	uint32_t data;
} __attribute__ ((packed)) mst_req_t;

typedef struct mst_rsp_s {
	uint32_t data;
	uint32_t status;
} __attribute__ ((packed)) mst_rsp_t;

#if !IS_ENABLED(CONFIG_QSEECOM_PROXY)
/* extern function declarations */
extern int qseecom_start_app(struct qseecom_handle **handle, char *app_name,
			     uint32_t size);
extern int qseecom_shutdown_app(struct qseecom_handle **handle);
extern int qseecom_send_command(struct qseecom_handle *handle, void *send_buf,
				uint32_t sbuf_len, void *resp_buf,
				uint32_t rbuf_len);
#endif
DEFINE_MUTEX(transmit_mutex);
#endif

#if defined(_ARCH_ARM_MACH_MSM_BUS_H) // build error
static struct msm_bus_paths ss_mst_usecases[] = {
	{
		.vectors = (struct msm_bus_vectors[]){
			{
				.src = 1,
				.dst = 512,
				.ab = 0,
				.ib = 0,
			},
		},
		.num_paths = 1,
	},
	{
		.vectors = (struct msm_bus_vectors[]){
			{
				.src = 1,
				.dst = 512,
				.ab = 4068000000,
				.ib = 4068000000,
			},
		},
		.num_paths = 1,
	},
};

static struct msm_bus_scale_pdata ss_mst_bus_client_pdata = {
	.usecase = ss_mst_usecases,
	.num_usecases = ARRAY_SIZE(ss_mst_usecases),
	.name = "ss_mst",
};
#endif
#endif

#if defined(CONFIG_MST_TEEGRIS)
/* enum definitions */
enum cmd_drv_client
{
	/* ta cmd */
	CMD_OPEN,
	CMD_CLOSE,
	CMD_IOCTL,
	CMD_TRACK1,
	CMD_TRACK2,
	CMD_MAX
};

/* ta uuid definition */
static TEEC_UUID uuid_ta = {
	0,0,0,{0,0,0x6d,0x73,0x74,0x5f,0x54,0x41}
};
#endif

#endif /* MST_DRV_H */
