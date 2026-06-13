// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2020-2022, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/reset.h>
#include "msm_vidc_iris35.h"
#include "msm_vidc_buffer_iris35.h"
#include "msm_vidc_power_iris35.h"
#include "msm_vidc_inst.h"
#include "msm_vidc_core.h"
#include "msm_vidc_driver.h"
#include "msm_vidc_platform.h"
#include "msm_vidc_internal.h"
#include "msm_vidc_buffer.h"
#include "msm_vidc_state.h"
#include "msm_vidc_debug.h"
#include "msm_vidc_variant.h"
#include "venus_hfi.h"

#define VIDEO_ARCH_LX 1

/*
 * --------------------------------------------------------------------------
 * MODULE: VCODEC_CPU_CS
 * --------------------------------------------------------------------------
 */
#define VCODEC_CPU_CS_IRIS35                          0x000A0000
#define CPU_CS_A2HSOFTINTCLR_IRIS35                   (VCODEC_CPU_CS_IRIS35 + 0x1C)
#define VCODEC_VPU_CPU_CS_VCICMDARG0_IRIS35           (VCODEC_CPU_CS_IRIS35 + 0x24)
#define VCODEC_VPU_CPU_CS_VCICMDARG1_IRIS35           (VCODEC_CPU_CS_IRIS35 + 0x28)
#define VCODEC_VPU_CPU_CS_SCIACMD_IRIS35              (VCODEC_CPU_CS_IRIS35 + 0x48)
#define VCODEC_VPU_CPU_CS_SCIACMDARG0_IRIS35          (VCODEC_CPU_CS_IRIS35 + 0x4C)
#define VCODEC_VPU_CPU_CS_SCIACMDARG1_IRIS35          (VCODEC_CPU_CS_IRIS35 + 0x50)
#define VCODEC_VPU_CPU_CS_SCIACMDARG2_IRIS35          (VCODEC_CPU_CS_IRIS35 + 0x54)
#define VCODEC_VPU_CPU_CS_SCIBCMD_IRIS35              (VCODEC_CPU_CS_IRIS35 + 0x5C)
#define VCODEC_VPU_CPU_CS_SCIBCMDARG0_IRIS35          (VCODEC_CPU_CS_IRIS35 + 0x60)
#define VCODEC_VPU_CPU_CS_SCIBARG1_IRIS35             (VCODEC_CPU_CS_IRIS35 + 0x64)
#define VCODEC_VPU_CPU_CS_SCIBARG2_IRIS35             (VCODEC_CPU_CS_IRIS35 + 0x68)
#define CPU_CS_H2XSOFTINTEN_IRIS35                    (VCODEC_CPU_CS_IRIS35 + 0x148)
#define CPU_IC_SOFTINT_IRIS35                         (VCODEC_CPU_CS_IRIS35 + 0x150)
#define CPU_CS_AHB_BRIDGE_SYNC_RESET                  (VCODEC_CPU_CS_IRIS35 + 0x160)
#define CPU_CS_X2RPMh_IRIS35                          (VCODEC_CPU_CS_IRIS35 + 0x168)
#define CPU_IC_SOFTINT_H2A_SHFT_IRIS35                0x0

#define HFI_CTRL_INIT_IRIS35                          VCODEC_VPU_CPU_CS_SCIACMD_IRIS35
#define HFI_CTRL_STATUS_IRIS35                        VCODEC_VPU_CPU_CS_SCIACMDARG0_IRIS35
typedef enum {
    HFI_CTRL_NOT_INIT                   = 0x0,
    HFI_CTRL_READY                      = 0x1,
    HFI_CTRL_ERROR_FATAL                = 0x2,
    HFI_CTRL_ERROR_UC_REGION_NOT_SET    = 0x4,
    HFI_CTRL_ERROR_HW_FENCE_QUEUE       = 0x8,
    HFI_CTRL_PC_READY                   = 0x100,
    HFI_CTRL_VCODEC_IDLE                = 0x40000000
} hfi_ctrl_status_type;

#define HFI_QTBL_INFO_IRIS35                          VCODEC_VPU_CPU_CS_SCIACMDARG1_IRIS35
typedef enum {
    HFI_QTBL_DISABLED    = 0x00,
    HFI_QTBL_ENABLED     = 0x01,
} hfi_qtbl_status_type;

#define HFI_QTBL_ADDR_IRIS35                          VCODEC_VPU_CPU_CS_SCIACMDARG2_IRIS35
#define HFI_MMAP_ADDR_IRIS35                          VCODEC_VPU_CPU_CS_SCIBCMDARG0_IRIS35
#define HFI_UC_REGION_ADDR_IRIS35                     VCODEC_VPU_CPU_CS_SCIBARG1_IRIS35
#define HFI_UC_REGION_SIZE_IRIS35                     VCODEC_VPU_CPU_CS_SCIBARG2_IRIS35
#define HFI_DEVICE_REGION_ADDR_IRIS35                 VCODEC_VPU_CPU_CS_VCICMDARG0_IRIS35
#define HFI_DEVICE_REGION_SIZE_IRIS35                 VCODEC_VPU_CPU_CS_VCICMDARG1_IRIS35
#define HFI_SFR_ADDR_IRIS35                           VCODEC_VPU_CPU_CS_SCIBCMD_IRIS35

/*
 * --------------------------------------------------------------------------
 * MODULE: VCODEC_IRIS_WRAPPER_TOP
 * --------------------------------------------------------------------------
 */
#define WRAPPER_BASE_OFFS_IRIS35                      0x000B0000
#define WRAPPER_INTR_STATUS_IRIS35                    (WRAPPER_BASE_OFFS_IRIS35 + 0x0C)
#define WRAPPER_INTR_STATUS_A2HWD_BMSK_IRIS35         0x8
#define WRAPPER_INTR_STATUS_A2H_BMSK_IRIS35           0x4

#define WRAPPER_INTR_MASK_IRIS35                      (WRAPPER_BASE_OFFS_IRIS35 + 0x10)
#define WRAPPER_INTR_MASK_A2HWD_BMSK_IRIS35           0x8
#define WRAPPER_INTR_MASK_A2HCPU_BMSK_IRIS35          0x4

#define WRAPPER_DEBUG_BRIDGE_LPI_CONTROL_IRIS35       (WRAPPER_BASE_OFFS_IRIS35 + 0x54)
#define WRAPPER_DEBUG_BRIDGE_LPI_STATUS_IRIS35        (WRAPPER_BASE_OFFS_IRIS35 + 0x58)
#define WRAPPER_IRIS_CPU_NOC_LPI_CONTROL              (WRAPPER_BASE_OFFS_IRIS35 + 0x5C)
#define WRAPPER_IRIS_CPU_NOC_LPI_STATUS               (WRAPPER_BASE_OFFS_IRIS35 + 0x60)
#define WRAPPER_IRIS_VCODEC_VPU_WRAPPER_SPARE_0       (WRAPPER_BASE_OFFS_IRIS35 + 0x78)
#define WRAPPER_CORE_POWER_STATUS                     (WRAPPER_BASE_OFFS_IRIS35 + 0x80)
#define WRAPPER_CORE_POWER_CONTROL                    (WRAPPER_BASE_OFFS_IRIS35 + 0x84)
#define WRAPPER_CORE_CLOCK_CONFIG_IRIS35              (WRAPPER_BASE_OFFS_IRIS35 + 0x88)

/*
 * --------------------------------------------------------------------------
 * MODULE: TZ_WRAPPER
 * --------------------------------------------------------------------------
 */
#define WRAPPER_TZ_BASE_OFFS                          0x000C0000
#define WRAPPER_TZ_CPU_STATUS                         (WRAPPER_TZ_BASE_OFFS + 0x10)
#define WRAPPER_TZ_CTL_AXI_CLOCK_CONFIG               (WRAPPER_TZ_BASE_OFFS + 0x14)
#define WRAPPER_TZ_QNS4PDXFIFO_RESET                  (WRAPPER_TZ_BASE_OFFS + 0x18)

/*
 * --------------------------------------------------------------------------
 * MODULE: AON_WRAPPER
 * --------------------------------------------------------------------------
 */
#define AON_BASE_OFFS                                 0x000E0000
#define AON_WRAPPER_MVP_NOC_LPI_CONTROL               (AON_BASE_OFFS)
#define AON_WRAPPER_MVP_NOC_LPI_STATUS                (AON_BASE_OFFS + 0x4)
#define AON_WRAPPER_MVP_NOC_CORE_SW_RESET             (AON_BASE_OFFS + 0x18)
#define AON_WRAPPER_MVP_NOC_CORE_CLK_CONTROL          (AON_BASE_OFFS + 0x20)
#define AON_WRAPPER_SPARE                             (AON_BASE_OFFS + 0x28)
#define AON_WRAPPER_MVP_VIDEO_CTL_NOC_LPI_CONTROL     (AON_BASE_OFFS + 0x2C)
#define AON_WRAPPER_MVP_VIDEO_CTL_NOC_LPI_STATUS      (AON_BASE_OFFS + 0x30)

/*
 * --------------------------------------------------------------------------
 * MODULE: VCODEC_SS registers
 * --------------------------------------------------------------------------
 */
#define VCODEC_BASE_OFFS_IRIS35                       0x00000000
#define VCODEC_SS_IDLE_STATUSn                        (VCODEC_BASE_OFFS_IRIS35 + 0x70)

/*
 * --------------------------------------------------------------------------
 * MODULE: VCODEC_NOC
 * --------------------------------------------------------------------------
 */
#define NOC_BASE_OFFS                                      0x00010000
#define NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_MAINCTL_LOW   (NOC_BASE_OFFS + 0xA008)
#define NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRCLR_LOW    (NOC_BASE_OFFS + 0xA018)
#define NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG0_LOW   (NOC_BASE_OFFS + 0xA020)
#define NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG0_HIGH  (NOC_BASE_OFFS + 0xA024)
#define NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG1_LOW   (NOC_BASE_OFFS + 0xA028)
#define NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG1_HIGH  (NOC_BASE_OFFS + 0xA02C)
#define NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG2_LOW   (NOC_BASE_OFFS + 0xA030)
#define NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG2_HIGH  (NOC_BASE_OFFS + 0xA034)
#define NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG3_LOW   (NOC_BASE_OFFS + 0xA038)
#define NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG3_HIGH  (NOC_BASE_OFFS + 0xA03C)
#define NOC_SIDEBANDMANAGER_MAIN_SIDEBANDMANAGER_FAULTINEN0_LOW (NOC_BASE_OFFS + 0x7040)

static int __interrupt_init_iris35(struct msm_vidc_core *core)
{
	u32 mask_val = 0;
	int rc = 0;

	/* All interrupts should be disabled initially 0x1F6 : Reset value */
	rc = __read_register(core, WRAPPER_INTR_MASK_IRIS35, &mask_val);
	if (rc)
		return rc;

	/* Write 0 to unmask CPU and WD interrupts */
	mask_val &= ~(WRAPPER_INTR_MASK_A2HWD_BMSK_IRIS35|
			WRAPPER_INTR_MASK_A2HCPU_BMSK_IRIS35);
	rc = __write_register(core, WRAPPER_INTR_MASK_IRIS35, mask_val);
	if (rc)
		return rc;

	return 0;
}

static int __raise_interrupt_iris35(struct msm_vidc_core *core)
{
	int rc = 0;

	rc = __write_register(core, CPU_IC_SOFTINT_IRIS35, 1 << CPU_IC_SOFTINT_H2A_SHFT_IRIS35);
	if (rc)
		return rc;

	return 0;
}

static int __clear_interrupt_iris35(struct msm_vidc_core *core)
{
	u32 intr_status = 0, mask = 0;
	int rc = 0;

	rc = __read_register(core, WRAPPER_INTR_STATUS_IRIS35, &intr_status);
	if (rc)
		return rc;

	mask = (WRAPPER_INTR_STATUS_A2H_BMSK_IRIS35 |
		WRAPPER_INTR_STATUS_A2HWD_BMSK_IRIS35 |
		HFI_CTRL_VCODEC_IDLE);

	if (intr_status & mask) {
		core->intr_status |= intr_status;
		core->reg_count++;
		d_vpr_l("INTERRUPT: times: %d interrupt_status: %d\n",
			core->reg_count, intr_status);
	} else {
		core->spur_count++;
	}

	rc = __write_register(core, CPU_CS_A2HSOFTINTCLR_IRIS35, 1);
	if (rc)
		return rc;

	return 0;
}

static int __get_device_region_info(struct msm_vidc_core *core,
	u32 *min_dev_addr, u32 *dev_reg_size)
{
	struct device_region_set *dev_set;
	u32 min_addr, max_addr, count = 0;
	int rc = 0;

	dev_set = &core->resource->device_region_set;

	if (!dev_set->count) {
		d_vpr_h("%s: device region not available\n", __func__);
		return 0;
	}

	min_addr = 0xFFFFFFFF;
	max_addr = 0x0;
	for (count = 0; count < dev_set->count; count++) {
		if (dev_set->device_region_tbl[count].dev_addr > max_addr)
			max_addr = dev_set->device_region_tbl[count].dev_addr +
				dev_set->device_region_tbl[count].size;
		if (dev_set->device_region_tbl[count].dev_addr < min_addr)
			min_addr = dev_set->device_region_tbl[count].dev_addr;
	}
	if (min_addr == 0xFFFFFFFF || max_addr == 0x0) {
		d_vpr_e("%s: invalid device region\n", __func__);
		return -EINVAL;
	}

	*min_dev_addr = min_addr;
	*dev_reg_size = max_addr - min_addr;

	return rc;
}

static int __program_bootup_registers_iris35(struct msm_vidc_core *core)
{
	u32 min_dev_reg_addr = 0, dev_reg_size = 0;
	struct device *dev = NULL;
	u32 value;
	int rc = 0;

	dev = &core->pdev->dev;

	value = (u32)core->iface_q_table.align_device_addr;
	rc = __write_register(core, HFI_UC_REGION_ADDR_IRIS35, value);
	if (rc)
		return rc;

	value = SHARED_QSIZE;
	rc = __write_register(core, HFI_UC_REGION_SIZE_IRIS35, value);
	if (rc)
		return rc;

	value = (u32)core->iface_q_table.align_device_addr;
	rc = __write_register(core, HFI_QTBL_ADDR_IRIS35, value);
	if (rc)
		return rc;

	rc = __write_register(core, HFI_QTBL_INFO_IRIS35, HFI_QTBL_ENABLED);
	if (rc)
		return rc;

	if (core->mmap_buf.align_device_addr) {
		value = (u32)core->mmap_buf.align_device_addr;
		rc = __write_register(core, HFI_MMAP_ADDR_IRIS35, value);
		if (rc)
			return rc;
	} else {
		d_vpr_e("%s: skip mmap buffer programming\n", __func__);
		/* ignore the error for now for backward compatibility */
		/* return -EINVAL; */
	}

	rc = __get_device_region_info(core, &min_dev_reg_addr, &dev_reg_size);
	if (rc)
		return rc;

	if (min_dev_reg_addr && dev_reg_size) {
		rc = __write_register(core, HFI_DEVICE_REGION_ADDR_IRIS35, min_dev_reg_addr);
		if (rc)
			return rc;

		rc = __write_register(core, HFI_DEVICE_REGION_SIZE_IRIS35, dev_reg_size);
		if (rc)
			return rc;
	} else {
		d_vpr_e("%s: skip device region programming\n", __func__);
		/* ignore the error for now for backward compatibility */
		/* return -EINVAL; */
	}

	if (core->sfr.align_device_addr) {
		value = (u32)core->sfr.align_device_addr + VIDEO_ARCH_LX;
		rc = __write_register(core, HFI_SFR_ADDR_IRIS35, value);
		if (rc)
			return rc;
	}

	/* Based on below register programming, firmware WA for sm8750-v2 would be enabled */
	if (of_device_is_compatible(dev->of_node, "qcom,sm8750-vidc-v2")) {
		rc = __write_register(core, WRAPPER_IRIS_VCODEC_VPU_WRAPPER_SPARE_0, 0x1);
		if (rc)
			return rc;
	} else {
		rc = __write_register(core, WRAPPER_IRIS_VCODEC_VPU_WRAPPER_SPARE_0, 0x0);
		if (rc)
			return rc;
	}

	return 0;
}

static int __boot_firmware_iris35(struct msm_vidc_core *core)
{
	int rc = 0;
	u32 ctrl_init_val = 0, ctrl_status = 0, count = 0, max_tries = 1000;

	rc = __program_bootup_registers_iris35(core);
	if (rc)
		return rc;

	ctrl_init_val = BIT(0);

	rc = __write_register(core, HFI_CTRL_INIT_IRIS35, ctrl_init_val);
	if (rc)
		return rc;

	while (count < max_tries) {
		rc = __read_register(core, HFI_CTRL_STATUS_IRIS35, &ctrl_status);
		if (rc)
			return rc;

		if ((ctrl_status & HFI_CTRL_ERROR_FATAL) ||
		    (ctrl_status & HFI_CTRL_ERROR_UC_REGION_NOT_SET) ||
		    (ctrl_status & HFI_CTRL_ERROR_HW_FENCE_QUEUE)) {
			d_vpr_e("%s: boot firmware failed, ctrl status %#x\n",
				__func__, ctrl_status);
			return -EINVAL;
		} else if (ctrl_status & HFI_CTRL_READY) {
			d_vpr_h("%s: boot firmware is successful, ctrl status %#x\n",
				__func__, ctrl_status);
			break;
		}

		usleep_range(50, 100);
		count++;
	}

	if (count >= max_tries) {
		d_vpr_e(FMT_STRING_BOOT_FIRMWARE_ERROR,
			ctrl_status, ctrl_init_val);
		return -ETIME;
	}

	/* Enable interrupt before sending commands to venus */
	rc = __write_register(core, CPU_CS_H2XSOFTINTEN_IRIS35, 0x1);
	if (rc)
		return rc;

	rc = __write_register(core, CPU_CS_X2RPMh_IRIS35, 0x0);
	if (rc)
		return rc;

	return rc;
}

static bool is_iris35_hw_power_collapsed(struct msm_vidc_core *core)
{
	int rc = 0;
	u32 value = 0, pwr_status = 0;

	rc = __read_register(core, WRAPPER_CORE_POWER_STATUS, &value);
	if (rc)
		return false;

	/* if BIT(1) is 1 then video hw power is on else off */
	pwr_status = value & BIT(1);
	return pwr_status ? false : true;
}

static int __power_off_iris35_hardware(struct msm_vidc_core *core)
{
	int rc = 0, i;
	u32 value = 0;
	bool pwr_collapsed = false;
	u32 count = 0;

	/*
	 * Incase hw power control is enabled, for any error case
	 * CPU WD, video hw unresponsive cases, NOC error case etc,
	 * execute NOC reset sequence before disabling power. If there
	 * is no CPU WD and hw power control is enabled, fw is expected
	 * to power collapse video hw always.
	 */
	if (is_core_sub_state(core, CORE_SUBSTATE_FW_PWR_CTRL)) {
		pwr_collapsed = is_iris35_hw_power_collapsed(core);
		if (pwr_collapsed) {
			d_vpr_h("%s: video hw power collapsed %s\n",
				__func__, core->sub_state_name);
			goto disable_power;
		} else {
			d_vpr_e("%s: video hw is power ON, try power collpase hw %s\n",
				__func__, core->sub_state_name);
		}
	}

	/*
	 * check to make sure core clock branch enabled else
	 * we cannot read vcodec top idle register
	 */
	rc = __read_register(core, WRAPPER_CORE_CLOCK_CONFIG_IRIS35, &value);
	if (rc)
		return rc;

	if (value) {
		d_vpr_e("%s: core clock config not enabled, enabling it to read vcodec registers\n",
			__func__);
		rc = __write_register(core, WRAPPER_CORE_CLOCK_CONFIG_IRIS35, 0);
		if (rc)
			return rc;
	}

	/*
	 * add MNoC idle check before collapsing MVS0 per HPG update
	 * poll for NoC DMA idle -> HPG 3.4.4
	 */
	for (i = 0; i < core->capabilities[NUM_VPP_PIPE].value; i++) {
		rc = __read_register_with_poll_timeout(core, VCODEC_SS_IDLE_STATUSn + 4*i,
				0x400000, 0x400000, 2000, 20000);
		if (rc)
			d_vpr_e("%s: VCODEC_SS_IDLE_STATUSn (%d) is not idle (%#x)\n",
				__func__, i, value);
	}

	/* set MNoC to low power, set PD_NOC_QREQ (bit 0) */
	rc = __write_register_masked(core, AON_WRAPPER_MVP_NOC_LPI_CONTROL,
					0x1, BIT(0));
	if (rc)
		return rc;

	rc = __read_register(core, AON_WRAPPER_MVP_NOC_LPI_STATUS, &value);
	if (rc)
		return rc;

	while ((!(value & BIT(0))) && (value & BIT(1) || value & BIT(2))) {
		rc = __write_register_masked(core, AON_WRAPPER_MVP_NOC_LPI_CONTROL,
					     0x0, BIT(0));
		if (rc)
			return rc;

		usleep_range(10, 20);

		rc = __write_register_masked(core, AON_WRAPPER_MVP_NOC_LPI_CONTROL,
					     0x1, BIT(0));
		if (rc)
			return rc;

		rc = __read_register(core, AON_WRAPPER_MVP_NOC_LPI_STATUS, &value);
		if (rc)
			return rc;

		++count;
		if (count >= 1000) {
			d_vpr_e("%s: AON_WRAPPER_MVP_NOC_LPI_CONTROL failed\n", __func__);
			break;
		}
	}

	rc = __read_register_with_poll_timeout(core, AON_WRAPPER_MVP_NOC_LPI_STATUS,
					       0x1, 0x1, 200, 2000);
	if (rc)
		d_vpr_e("%s: AON_WRAPPER_MVP_NOC_LPI_CONTROL failed1\n", __func__);

	rc = __write_register_masked(core, AON_WRAPPER_MVP_NOC_LPI_CONTROL,
					0x0, BIT(0));
	if (rc)
		return rc;

	/*
	 * Reset both sides of 2 ahb2ahb_bridges (TZ and non-TZ)
	 * do we need to check status register here?
	 */
	rc = __write_register(core, CPU_CS_AHB_BRIDGE_SYNC_RESET, 0x3);
	if (rc)
		return rc;
	rc = __write_register(core, CPU_CS_AHB_BRIDGE_SYNC_RESET, 0x2);
	if (rc)
		return rc;
	rc = __write_register(core, CPU_CS_AHB_BRIDGE_SYNC_RESET, 0x0);
	if (rc)
		return rc;

disable_power:
	/* power down process */
	rc = call_res_op(core, gdsc_off, core, "vcodec");
	if (rc) {
		d_vpr_e("%s: disable regulator vcodec failed\n", __func__);
		rc = 0;
	}

	rc = call_res_op(core, clk_disable, core, "video_cc_mvs0_clk");
	if (rc) {
		d_vpr_e("%s: disable unprepare video_cc_mvs0_clk failed\n", __func__);
		rc = 0;
	}

	return rc;
}

static int __power_off_iris35_controller(struct msm_vidc_core *core)
{
	int rc = 0;
	int value = 0;
	u32 count = 0;

	/*
	 * mask fal10_veto QLPAC error since fal10_veto can go 1
	 * when pwwait == 0 and clamped to 0 -> HPG 3.7.4
	 */
	rc = __write_register(core, CPU_CS_X2RPMh_IRIS35, 0x3);
	if (rc)
		return rc;

	/* Set Iris CPU NoC to Low power */
	rc = __write_register_masked(core, WRAPPER_IRIS_CPU_NOC_LPI_CONTROL,
			0x1, BIT(0));
	if (rc)
		return rc;

	rc = __read_register_with_poll_timeout(core, WRAPPER_IRIS_CPU_NOC_LPI_STATUS,
			0x1, 0x1, 200, 2000);
	if (rc)
		d_vpr_e("%s: WRAPPER_IRIS_CPU_NOC_LPI_CONTROL failed\n", __func__);

	rc = __write_register_masked(core, WRAPPER_IRIS_CPU_NOC_LPI_CONTROL,
				     0x0, BIT(0));
	if (rc)
		return rc;

	rc = __write_register_masked(core, AON_WRAPPER_MVP_VIDEO_CTL_NOC_LPI_CONTROL,
				     0x1, BIT(0));
	if (rc)
		return rc;

	rc = __read_register(core, AON_WRAPPER_MVP_VIDEO_CTL_NOC_LPI_STATUS, &value);
	if (rc)
		return rc;

	while ((!(value & BIT(0))) && (value & BIT(1) || value & BIT(2))) {
		rc = __write_register_masked(core, AON_WRAPPER_MVP_VIDEO_CTL_NOC_LPI_CONTROL,
					     0x0, BIT(0));
		if (rc)
			return rc;

		usleep_range(10, 20);

		rc = __write_register_masked(core, AON_WRAPPER_MVP_VIDEO_CTL_NOC_LPI_CONTROL,
					     0x1, BIT(0));
		if (rc)
			return rc;

		rc = __read_register(core, AON_WRAPPER_MVP_VIDEO_CTL_NOC_LPI_STATUS, &value);
		if (rc)
			return rc;

		++count;
		if (count >= 1000) {
			d_vpr_e("%s: AON_WRAPPER_MVP_VIDEO_CTL_NOC_LPI_CONTROL failed\n", __func__);
			break;
		}
	}

	rc = __read_register_with_poll_timeout(core, AON_WRAPPER_MVP_VIDEO_CTL_NOC_LPI_STATUS,
					       0x1, 0x1, 200, 2000);
	if (rc)
		d_vpr_e("%s: AON_WRAPPER_MVP_VIDEO_CTL_NOC_LPI_CONTROL failed\n", __func__);

	rc = __write_register_masked(core, AON_WRAPPER_MVP_VIDEO_CTL_NOC_LPI_CONTROL,
				     0x0, BIT(0));
	if (rc)
		return rc;

	/* Debug bridge LPI release */
	rc = __write_register(core, WRAPPER_DEBUG_BRIDGE_LPI_CONTROL_IRIS35, 0x0);
	if (rc)
		return rc;

	rc = __read_register_with_poll_timeout(core, WRAPPER_DEBUG_BRIDGE_LPI_STATUS_IRIS35,
					       0xffffffff, 0x0, 200, 2000);
	if (rc)
		d_vpr_e("%s: debug bridge release failed\n", __func__);

	/* power down process */
	rc = call_res_op(core, gdsc_off, core, "iris-ctl");
	if (rc) {
		d_vpr_e("%s: disable regulator iris-ctl failed\n", __func__);
		rc = 0;
	}

	rc = call_res_op(core, clk_disable, core, "gcc_video_axi1_clk");
	if (rc) {
		d_vpr_e("%s: disable unprepare gcc_video_axi1_clk failed\n", __func__);
		rc = 0;
	}

	rc = call_res_op(core, clk_disable, core, "gcc_video_axi0_clk");
	if (rc) {
		d_vpr_e("%s: disable unprepare gcc_video_axi0_clk failed\n", __func__);
		rc = 0;
	}

	rc = call_res_op(core, clk_disable, core, "video_cc_mvs0c_freerun_clk");
	if (rc) {
		d_vpr_e("%s: disable unprepare video_cc_mvs0c_freerun_clk failed\n", __func__);
		rc = 0;
	}

	rc = call_res_op(core, clk_disable, core, "video_cc_mvs0_freerun_clk");
	if (rc) {
		d_vpr_e("%s: disable unprepare video_cc_mvs0_freerun_clk failed\n", __func__);
		rc = 0;
	}

	rc = call_res_op(core, clk_disable, core, "video_cc_mvs0c_clk");
	if (rc) {
		d_vpr_e("%s: disable unprepare video_cc_mvs0c_clk failed\n", __func__);
		rc = 0;
	}

	rc = call_res_op(core, reset_control_assert, core, "video_axi1_reset");
	if (rc)
		d_vpr_e("%s: assert video_axi1_reset failed\n", __func__);

	rc = call_res_op(core, reset_control_assert, core, "video_axi0_reset");
	if (rc)
		d_vpr_e("%s: assert video_axi0_reset failed\n", __func__);

	rc = call_res_op(core, reset_control_assert, core, "video_mvs0c_freerun_reset");
	if (rc)
		d_vpr_e("%s: assert video_mvs0c_reset failed\n", __func__);

	rc = call_res_op(core, reset_control_assert, core, "video_mvs0_freerun_reset");
	if (rc)
		d_vpr_e("%s: assert video_mvs0_reset failed\n", __func__);

	usleep_range(400, 500);

	rc = call_res_op(core, reset_control_deassert, core, "video_mvs0_freerun_reset");
	if (rc)
		d_vpr_e("%s: deassert video_mvs0_reset failed\n", __func__);

	rc = call_res_op(core, reset_control_deassert, core, "video_mvs0c_freerun_reset");
	if (rc)
		d_vpr_e("%s: deassert video_mvs0c_reset failed\n", __func__);

	rc = call_res_op(core, reset_control_deassert, core, "video_axi0_reset");
	if (rc)
		d_vpr_e("%s: deassert video_axi0_reset failed\n", __func__);

	rc = call_res_op(core, reset_control_deassert, core, "video_axi1_reset");
	if (rc)
		d_vpr_e("%s: deassert video_axi1_reset failed\n", __func__);

	return rc;
}

static int __power_off_iris35(struct msm_vidc_core *core)
{
	int rc = 0;

	if (!is_core_sub_state(core, CORE_SUBSTATE_POWER_ENABLE))
		return 0;

	/**
	 * Reset video_cc_mvs0_clk_src value to resolve MMRM high video
	 * clock projection issue.
	 */
	rc = call_res_op(core, set_clks, core, 0);
	if (rc)
		d_vpr_e("%s: resetting clocks failed\n", __func__);

	if (__power_off_iris35_hardware(core))
		d_vpr_e("%s: failed to power off hardware\n", __func__);

	if (__power_off_iris35_controller(core))
		d_vpr_e("%s: failed to power off controller\n", __func__);

	rc = call_res_op(core, set_bw, core, 0, 0);
	if (rc)
		d_vpr_e("%s: failed to unvote buses\n", __func__);

	if (!call_venus_op(core, watchdog, core, core->intr_status))
		disable_irq_nosync(core->resource->irq);

	msm_vidc_change_core_sub_state(core, CORE_SUBSTATE_POWER_ENABLE, 0, __func__);

	return rc;
}

static int __power_on_iris35_controller(struct msm_vidc_core *core)
{
	int rc = 0;

	rc = call_res_op(core, gdsc_on, core, "iris-ctl");
	if (rc)
		goto fail_regulator;

	rc = call_res_op(core, clk_enable, core, "gcc_video_axi1_clk");
	if (rc)
		goto fail_clk_axi;

	rc = call_res_op(core, clk_enable, core, "video_cc_mvs0c_freerun_clk");
	if (rc)
		goto fail_clk_freerun;

	rc = call_res_op(core, clk_enable, core, "video_cc_mvs0c_clk");
	if (rc)
		goto fail_clk_controller;

	return 0;

fail_clk_controller:
	call_res_op(core, clk_disable, core, "video_cc_mvs0c_freerun_clk");
fail_clk_freerun:
	call_res_op(core, clk_disable, core, "gcc_video_axi1_clk");
fail_clk_axi:
	call_res_op(core, gdsc_off, core, "iris-ctl");
fail_regulator:
	return rc;
}

static int __power_on_iris35_hardware(struct msm_vidc_core *core)
{
	int rc = 0;

	rc = call_res_op(core, gdsc_on, core, "vcodec");
	if (rc)
		goto fail_regulator;

	/* video controller and hardware powered on successfully */
	rc = msm_vidc_change_core_sub_state(core, 0, CORE_SUBSTATE_POWER_ENABLE, __func__);
	if (rc)
		goto fail_power_on_substate;

	rc = call_res_op(core, gdsc_sw_ctrl, core);
	if (rc)
		goto fail_sw_ctrl;

	rc = call_res_op(core, clk_enable, core, "gcc_video_axi0_clk");
	if (rc)
		goto fail_clk_axi;

	rc = call_res_op(core, clk_enable, core, "video_cc_mvs0_freerun_clk");
	if (rc)
		goto fail_clk_freerun;

	rc = call_res_op(core, clk_enable, core, "video_cc_mvs0_clk");
	if (rc)
		goto fail_clk_controller;

	return 0;

fail_clk_controller:
	call_res_op(core, clk_disable, core, "video_cc_mvs0_freerun_clk");
fail_clk_freerun:
	call_res_op(core, clk_disable, core, "gcc_video_axi0_clk");
fail_clk_axi:
	call_res_op(core, gdsc_hw_ctrl, core);
fail_sw_ctrl:
fail_power_on_substate:
	call_res_op(core, gdsc_off, core, "vcodec");
fail_regulator:
	return rc;
}

static int __power_on_iris35(struct msm_vidc_core *core)
{
	struct frequency_table *freq_tbl;
	u32 freq = 0;
	int rc = 0;

	if (is_core_sub_state(core, CORE_SUBSTATE_POWER_ENABLE))
		return 0;

	if (!core_in_valid_state(core)) {
		d_vpr_e("%s: invalid core state %s\n",
			__func__, core_state_name(core->state));
		return -EINVAL;
	}

	/* Vote for all hardware resources */
	rc = call_res_op(core, set_bw, core, INT_MAX, INT_MAX);
	if (rc) {
		d_vpr_e("%s: failed to vote buses, rc %d\n", __func__, rc);
		goto fail_vote_buses;
	}

	rc = __power_on_iris35_controller(core);
	if (rc) {
		d_vpr_e("%s: failed to power on iris35 controller\n", __func__);
		goto fail_power_on_controller;
	}

	rc = __power_on_iris35_hardware(core);
	if (rc) {
		d_vpr_e("%s: failed to power on iris35 hardware\n", __func__);
		goto fail_power_on_hardware;
	}

	freq_tbl = core->resource->freq_set.freq_tbl;
	freq = core->power.clk_freq ? core->power.clk_freq :
				      freq_tbl[0].freq;

	rc = call_res_op(core, set_clks, core, freq);
	if (rc) {
		d_vpr_e("%s: failed to scale clocks\n", __func__);
		rc = 0;
	}

	__set_registers(core);

	__interrupt_init_iris35(core);
	core->intr_status = 0;
	enable_irq(core->resource->irq);

	return rc;

fail_power_on_hardware:
	__power_off_iris35_controller(core);
fail_power_on_controller:
	call_res_op(core, set_bw, core, 0, 0);
fail_vote_buses:
	msm_vidc_change_core_sub_state(core, CORE_SUBSTATE_POWER_ENABLE, 0, __func__);

	return rc;
}

static int __prepare_pc_iris35(struct msm_vidc_core *core)
{
	int rc = 0;
	u32 wfi_status = 0, idle_status = 0, pc_ready = 0;
	u32 ctrl_status = 0;

	rc = __read_register(core, HFI_CTRL_STATUS_IRIS35, &ctrl_status);
	if (rc)
		return rc;

	pc_ready = ctrl_status & HFI_CTRL_PC_READY;
	idle_status = ctrl_status & BIT(30);

	if (pc_ready) {
		d_vpr_h("Already in pc_ready state\n");
		return 0;
	}
	rc = __read_register(core, WRAPPER_TZ_CPU_STATUS, &wfi_status);
	if (rc)
		return rc;

	wfi_status &= BIT(0);
	if (!wfi_status || !idle_status) {
		d_vpr_e("Skipping PC, wfi status not set\n");
		goto skip_power_off;
	}

	rc = __prepare_pc(core);
	if (rc) {
		d_vpr_e("Failed __prepare_pc %d\n", rc);
		goto skip_power_off;
	}

	rc = __read_register_with_poll_timeout(core, HFI_CTRL_STATUS_IRIS35,
			HFI_CTRL_PC_READY, HFI_CTRL_PC_READY, 250, 2500);
	if (rc) {
		d_vpr_e("%s: Skip PC. Ctrl status not set\n", __func__);
		goto skip_power_off;
	}

	rc = __read_register_with_poll_timeout(core, WRAPPER_TZ_CPU_STATUS,
			BIT(0), 0x1, 250, 2500);
	if (rc) {
		d_vpr_e("%s: Skip PC. Wfi status not set\n", __func__);
		goto skip_power_off;
	}
	return rc;

skip_power_off:
	rc = __read_register(core, HFI_CTRL_STATUS_IRIS35, &ctrl_status);
	if (rc)
		return rc;
	rc = __read_register(core, WRAPPER_TZ_CPU_STATUS, &wfi_status);
	if (rc)
		return rc;
	wfi_status &= BIT(0);
	d_vpr_e("Skip PC, wfi=%#x, idle=%#x, pcr=%#x, ctrl=%#x)\n",
		wfi_status, idle_status, pc_ready, ctrl_status);
	return -EAGAIN;
}

static int __watchdog_iris35(struct msm_vidc_core *core, u32 intr_status)
{
	int rc = 0;

	if (intr_status & WRAPPER_INTR_STATUS_A2HWD_BMSK_IRIS35) {
		d_vpr_e("%s: received watchdog interrupt\n", __func__);
		rc = 1;
	}

	return rc;
}

static int __noc_error_info_iris35(struct msm_vidc_core *core)
{
	u32 value;
	int rc = 0;

	if (is_iris35_hw_power_collapsed(core)) {
		d_vpr_e("%s: video hardware already power collapsed\n", __func__);
		return rc;
	}

	rc = __read_register(core,
			NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG0_LOW, &value);
	if (!rc)
		d_vpr_e("%s: NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG0_LOW:  %#x\n",
			__func__, value);
	rc = __read_register(core,
			NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG0_HIGH, &value);
	if (!rc)
		d_vpr_e("%s: NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG0_HIGH:  %#x\n",
			__func__, value);
	rc = __read_register(core,
			NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG1_LOW, &value);
	if (!rc)
		d_vpr_e("%s: NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG1_LOW:  %#x\n",
			__func__, value);
	rc = __read_register(core,
			NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG1_HIGH, &value);
	if (!rc)
		d_vpr_e("%s: NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG1_HIGH:  %#x\n",
			__func__, value);
	rc = __read_register(core,
			NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG2_LOW, &value);
	if (!rc)
		d_vpr_e("%s: NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG2_LOW:  %#x\n",
			__func__, value);
	rc = __read_register(core,
			NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG2_HIGH, &value);
	if (!rc)
		d_vpr_e("%s: NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG2_HIGH:  %#x\n",
			__func__, value);
	rc = __read_register(core,
			NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG3_LOW, &value);
	if (!rc)
		d_vpr_e("%s: NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG3_LOW:  %#x\n",
			__func__, value);
	rc = __read_register(core,
			NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG3_HIGH, &value);
	if (!rc)
		d_vpr_e("%s: NOC_ERL_ERRORLOGGER_MAIN_ERRORLOGGER_ERRLOG3_HIGH:  %#x\n",
			__func__, value);

	return rc;
}

static int __switch_gdsc_mode_iris35(struct msm_vidc_core *core, bool sw_mode)
{
	int rc;

	if (sw_mode) {
		rc = __write_register(core, WRAPPER_CORE_POWER_CONTROL, 0x0);
		if (rc)
			return rc;
		rc = __read_register_with_poll_timeout(core, WRAPPER_CORE_POWER_STATUS,
						       BIT(1), 0x2, 200, 2000);
		if (rc) {
			d_vpr_e("%s: Failed to read WRAPPER_CORE_POWER_STATUS register to 0x1\n",
				__func__);
			return rc;
		}
	} else {
		rc = __write_register(core, WRAPPER_CORE_POWER_CONTROL, 0x1);
		if (rc)
			return rc;
		rc = __read_register_with_poll_timeout(core, WRAPPER_CORE_POWER_STATUS,
						       BIT(1), 0x0, 200, 2000);
		if (rc) {
			d_vpr_e("%s: Failed to read WRAPPER_CORE_POWER_STATUS register to 0x0\n",
				__func__);
			return rc;
		}
	}

	return 0;
}

int msm_vidc_decide_work_mode_iris35(struct msm_vidc_inst *inst)
{
	u32 work_mode;
	struct v4l2_format *inp_f;
	u32 width, height;
	bool res_ok = false;

	work_mode = MSM_VIDC_STAGE_2;
	inp_f = &inst->fmts[INPUT_PORT];

	if (is_image_decode_session(inst))
		work_mode = MSM_VIDC_STAGE_1;

	if (is_image_session(inst))
		goto exit;

	if (is_decode_session(inst)) {
		height = inp_f->fmt.pix_mp.height;
		width = inp_f->fmt.pix_mp.width;
		res_ok = res_is_less_than(width, height, 1280, 720);
		if (inst->capabilities[CODED_FRAMES].value ==
				CODED_FRAMES_INTERLACE ||
			inst->capabilities[LOWLATENCY_MODE].value ||
			res_ok) {
			work_mode = MSM_VIDC_STAGE_1;
		}
	} else if (is_encode_session(inst)) {
		height = inst->crop.height;
		width = inst->crop.width;
		res_ok = !res_is_greater_than(width, height, 4096, 2160);
		if (res_ok &&
			(inst->capabilities[LOWLATENCY_MODE].value)) {
			work_mode = MSM_VIDC_STAGE_1;
		}
		if (inst->capabilities[SLICE_MODE].value ==
			V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_BYTES) {
			work_mode = MSM_VIDC_STAGE_1;
		}
		if (inst->capabilities[LOSSLESS].value)
			work_mode = MSM_VIDC_STAGE_2;

		if (!inst->capabilities[GOP_SIZE].value)
			work_mode = MSM_VIDC_STAGE_2;
	} else {
		i_vpr_e(inst, "%s: invalid session type\n", __func__);
		return -EINVAL;
	}

exit:
	i_vpr_h(inst, "Configuring work mode = %u low latency = %llu, gop size = %llu\n",
		work_mode, inst->capabilities[LOWLATENCY_MODE].value,
		inst->capabilities[GOP_SIZE].value);
	msm_vidc_update_cap_value(inst, STAGE, work_mode, __func__);

	return 0;
}

int msm_vidc_decide_work_route_iris35(struct msm_vidc_inst *inst)
{
	u32 work_route;
	struct msm_vidc_core *core;

	core = inst->core;
	work_route = core->capabilities[NUM_VPP_PIPE].value;

	if (is_image_session(inst))
		goto exit;

	if (is_decode_session(inst)) {
		if (inst->capabilities[CODED_FRAMES].value ==
				CODED_FRAMES_INTERLACE)
			work_route = MSM_VIDC_PIPE_1;
	} else if (is_encode_session(inst)) {
		u32 slice_mode;

		slice_mode = inst->capabilities[SLICE_MODE].value;

		/*TODO Pipe=1 for legacy CBR*/
		if (slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_BYTES)
			work_route = MSM_VIDC_PIPE_1;

	} else {
		i_vpr_e(inst, "%s: invalid session type\n", __func__);
		return -EINVAL;
	}

exit:
	i_vpr_h(inst, "Configuring work route = %u", work_route);
	msm_vidc_update_cap_value(inst, PIPE, work_route, __func__);

	return 0;
}

int msm_vidc_decide_quality_mode_iris35(struct msm_vidc_inst *inst)
{
	struct msm_vidc_core *core;
	u32 mbpf, mbps, max_hq_mbpf, max_hq_mbps;
	u32 mode = MSM_VIDC_POWER_SAVE_MODE;

	if (!is_encode_session(inst))
		return 0;

	/* image or lossless or all intra runs at quality mode */
	if (is_image_session(inst) || inst->capabilities[LOSSLESS].value ||
		inst->capabilities[ALL_INTRA].value) {
		mode = MSM_VIDC_MAX_QUALITY_MODE;
		goto decision_done;
	}

	/* for lesser complexity, make LP for all resolution */
	if (inst->capabilities[COMPLEXITY].value < DEFAULT_COMPLEXITY) {
		mode = MSM_VIDC_POWER_SAVE_MODE;
		goto decision_done;
	}

	mbpf = msm_vidc_get_mbs_per_frame(inst);
	mbps = mbpf * msm_vidc_get_fps(inst);
	core = inst->core;
	max_hq_mbpf = core->capabilities[MAX_MBPF_HQ].value;;
	max_hq_mbps = core->capabilities[MAX_MBPS_HQ].value;;

	if (!is_realtime_session(inst)) {
		if (((inst->capabilities[COMPLEXITY].flags & CAP_FLAG_CLIENT_SET) &&
			(inst->capabilities[COMPLEXITY].value >= DEFAULT_COMPLEXITY)) ||
			mbpf <= max_hq_mbpf) {
			mode = MSM_VIDC_MAX_QUALITY_MODE;
			goto decision_done;
		}
	}

	if (mbpf <= max_hq_mbpf && mbps <= max_hq_mbps)
		mode = MSM_VIDC_MAX_QUALITY_MODE;

decision_done:
	msm_vidc_update_cap_value(inst, QUALITY_MODE, mode, __func__);

	return 0;
}

int msm_vidc_adjust_min_quality_iris35(void *instance, struct v4l2_ctrl *ctrl)
{
	s32 adjusted_value;
	struct msm_vidc_inst *inst = (struct msm_vidc_inst *)instance;
	s64 rc_type = -1, enh_layer_count = -1, pix_fmts = -1;
	u32 width, height, frame_rate;
	struct v4l2_format *f;

	adjusted_value = ctrl ? ctrl->val : inst->capabilities[MIN_QUALITY].value;

	/*
	 * Although MIN_QUALITY is static, one of its parents,
	 * ENH_LAYER_COUNT is dynamic cap. Hence, dynamic call
	 * may be made for MIN_QUALITY via ENH_LAYER_COUNT.
	 * Therefore, below streaming check is required to avoid
	 * runtime modification of MIN_QUALITY.
	 */
	if (inst->bufq[OUTPUT_PORT].vb2q->streaming)
		return 0;

	if (msm_vidc_get_parent_value(inst, MIN_QUALITY,
				      BITRATE_MODE, &rc_type, __func__) ||
	    msm_vidc_get_parent_value(inst, MIN_QUALITY,
				      ENH_LAYER_COUNT, &enh_layer_count, __func__))
		return -EINVAL;

	/*
	 * Min Quality is supported only for VBR rc type.
	 * Hence, do not adjust or set to firmware for non VBR rc's
	 */
	if (rc_type != HFI_RC_VBR_CFR) {
		adjusted_value = 0;
		goto update_and_exit;
	}

	frame_rate = inst->capabilities[FRAME_RATE].value >> 16;
	f = &inst->fmts[OUTPUT_PORT];
	width = f->fmt.pix_mp.width;
	height = f->fmt.pix_mp.height;

	/*
	 * VBR Min Quality not supported for:
	 * - HEVC 10bit
	 * - ROI support
	 * - HP encoding
	 * - External Blur
	 * - Resolution beyond 1080P
	 * (It will fall back to CQCAC 25% or 0% (CAC) or CQCAC-OFF)
	 */
	if (inst->codec == MSM_VIDC_HEVC) {
		if (msm_vidc_get_parent_value(inst, MIN_QUALITY,
					      PIX_FMTS, &pix_fmts, __func__))
			return -EINVAL;

		if (is_10bit_colorformat(pix_fmts)) {
			i_vpr_h(inst,
				"%s: min quality is supported only for 8 bit\n",
				__func__);
			adjusted_value = 0;
			goto update_and_exit;
		}
	}

	if (res_is_greater_than(width, height, 1920, 1080)) {
		i_vpr_h(inst, "%s: unsupported res, wxh %ux%u\n",
			__func__, width, height);
		adjusted_value = 0;
		goto update_and_exit;
	}

	if (frame_rate > 60) {
		i_vpr_h(inst, "%s: unsupported fps %u\n",
			__func__, frame_rate);
		adjusted_value = 0;
		goto update_and_exit;
	}

	if (enh_layer_count > 0 && inst->hfi_layer_type != HFI_HIER_B) {
		i_vpr_h(inst,
			"%s: min quality not supported for HP encoding\n",
			__func__);
		adjusted_value = 0;
		goto update_and_exit;
	}

	/* Above conditions are met. Hence enable min quality */
	adjusted_value = MAX_SUPPORTED_MIN_QUALITY;

update_and_exit:
	msm_vidc_update_cap_value(inst, MIN_QUALITY, adjusted_value, __func__);

	return 0;
}

int msm_vidc_adjust_bitrate_boost_iris35(void *instance, struct v4l2_ctrl *ctrl)
{
	s32 adjusted_value;
	struct msm_vidc_inst *inst = (struct msm_vidc_inst *)instance;
	s64 rc_type = -1;
	u32 width, height, frame_rate;
	struct v4l2_format *f;
	u32 max_bitrate = 0, bitrate = 0;

	adjusted_value = ctrl ? ctrl->val :
		inst->capabilities[BITRATE_BOOST].value;

	if (inst->bufq[OUTPUT_PORT].vb2q->streaming)
		return 0;

	if (msm_vidc_get_parent_value(inst, BITRATE_BOOST,
		BITRATE_MODE, &rc_type, __func__))
		return -EINVAL;

	/*
	 * Bitrate Boost are supported only for VBR rc type.
	 * Hence, do not adjust or set to firmware for non VBR rc's
	 */
	if (rc_type != HFI_RC_VBR_CFR) {
		adjusted_value = 0;
		goto adjust;
	}

	frame_rate = inst->capabilities[FRAME_RATE].value >> 16;
	f = &inst->fmts[OUTPUT_PORT];
	width = f->fmt.pix_mp.width;
	height = f->fmt.pix_mp.height;

	/*
	 * honor client set bitrate boost
	 * if client did not set, keep max bitrate boost upto 4k@60fps
	 * and remove bitrate boost after 4k@60fps
	 */
	if (inst->capabilities[BITRATE_BOOST].flags & CAP_FLAG_CLIENT_SET) {
		/* accept client set bitrate boost value as is */
	} else {
		if (res_is_less_than_or_equal_to(width, height, 4096, 2176) &&
			frame_rate <= 60)
			adjusted_value = MAX_BITRATE_BOOST;
		else
			adjusted_value = 0;
	}

	max_bitrate = msm_vidc_get_max_bitrate(inst);
	bitrate = inst->capabilities[BIT_RATE].value;
	if (adjusted_value) {
		if ((bitrate + bitrate / (100 / adjusted_value)) > max_bitrate) {
			i_vpr_h(inst,
				"%s: bitrate %d is beyond max bitrate %d, remove bitrate boost\n",
				__func__, max_bitrate, bitrate);
			adjusted_value = 0;
		}
	}
adjust:
	msm_vidc_update_cap_value(inst, BITRATE_BOOST, adjusted_value, __func__);

	return 0;
}

static struct msm_vidc_venus_ops iris35_ops = {
	.raise_interrupt = __raise_interrupt_iris35,
	.clear_interrupt = __clear_interrupt_iris35,
	.boot_firmware = __boot_firmware_iris35,
	.power_on = __power_on_iris35,
	.power_off = __power_off_iris35,
	.prepare_pc = __prepare_pc_iris35,
	.watchdog = __watchdog_iris35,
	.noc_error_info = __noc_error_info_iris35,
	.switch_gdsc_mode = __switch_gdsc_mode_iris35,
};

static struct msm_vidc_session_ops msm_session_ops = {
	.buffer_size = msm_buffer_size_iris35,
	.min_count = msm_buffer_min_count_iris35,
	.extra_count = msm_buffer_extra_count_iris35,
	.ring_buf_count = msm_vidc_ring_buf_count_iris35,
	.calc_freq = msm_vidc_calc_freq_iris35,
	.calc_bw = msm_vidc_calc_bw_iris35,
	.decide_work_route = msm_vidc_decide_work_route_iris35,
	.decide_work_mode = msm_vidc_decide_work_mode_iris35,
	.decide_quality_mode = msm_vidc_decide_quality_mode_iris35,
};

int msm_vidc_init_iris35(struct msm_vidc_core *core)
{
	d_vpr_h("%s()\n", __func__);
	core->venus_ops = &iris35_ops;
	core->session_ops = &msm_session_ops;

	return 0;
}
