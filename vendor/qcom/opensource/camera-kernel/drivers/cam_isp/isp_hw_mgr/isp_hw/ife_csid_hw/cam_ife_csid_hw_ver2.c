// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/iopoll.h>
#include <linux/slab.h>

#include <media/cam_isp.h>
#include <media/cam_defs.h>
#include <media/cam_req_mgr.h>

#include <dt-bindings/msm-camera.h>

#include "cam_ife_csid_common.h"
#include "cam_ife_csid_hw_ver2.h"
#include "cam_isp_hw.h"
#include "cam_isp_hw_mgr_intf.h"
#include "cam_soc_util.h"
#include "cam_io_util.h"
#include "cam_debug_util.h"
#include "cam_cpas_api.h"
#include "cam_irq_controller.h"
#include "cam_tasklet_util.h"
#include "cam_cdm_util.h"
#include "cam_common_util.h"
#include "cam_subdev.h"
#include "cam_compat.h"
#include "cam_vmrm_interface.h"
#include "cam_mem_mgr_api.h"
#if defined(CONFIG_CAMERA_CDR_TEST)
#include "cam_clock_data_recovery.h"
#endif
#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
#include "cam_sensor_cmn_header.h"
#include "cam_hw_bigdata.h"
#endif

#if defined(CONFIG_SAMSUNG_CAMERA)
#include "cam_csiphy_core.h"

// adb shell "echo 1,5 > /sys/module/camera/parameters/debug_hbi_vbi"
// need to setting csid max clock -> 480M
// adb shell mount -t debugfs none /sys/kernel/debug
// adb shell "echo 8 > /sys/kernel/debug/camera/clk/ad27000.qcom,csid0/clk_lvl_control"
// adb shell "echo 8 > /sys/kernel/debug/camera/clk/ad2a000.qcom,csid1/clk_lvl_control"
// adb shell "echo 8 > /sys/kernel/debug/camera/clk/ad2d000.qcom,csid2/clk_lvl_control"
static int debug_hbi_vbi_count;
static unsigned int debug_hbi_vbi[2]; //enable, csiphy index
module_param_array(debug_hbi_vbi, uint, &debug_hbi_vbi_count, 0644);
#endif

/* CSIPHY TPG VC/DT values */
#define CAM_IFE_CPHY_TPG_VC_VAL                         0x0
#define CAM_IFE_CPHY_TPG_DT_VAL                         0x2B

/* Timeout values in usec */
#define CAM_IFE_CSID_TIMEOUT_SLEEP_US                  1000
#define CAM_IFE_CSID_TIMEOUT_ALL_US                    100000

#define CAM_IFE_CSID_RESET_TIMEOUT_MS                  100

/*
 * Constant Factors needed to change QTimer ticks to nanoseconds
 * QTimer Freq = 19.2 MHz
 * Time(us) = ticks/19.2
 * Time(ns) = ticks/19.2 * 1000
 */
#define CAM_IFE_CSID_QTIMER_MUL_FACTOR                 10000
#define CAM_IFE_CSID_QTIMER_DIV_FACTOR                 192

/* Max number of sof irq's triggered in case of SOF freeze */
#define CAM_CSID_IRQ_SOF_DEBUG_CNT_MAX 12

/* Max CSI Rx irq error count threshold value */
#define CAM_IFE_CSID_MAX_IRQ_ERROR_COUNT               100

/* Max sensor switch out of sync threshold */
#define CAM_IFE_CSID_MAX_OUT_OF_SYNC_ERR_COUNT         6

#define CAM_CSID_IRQ_CTRL_NAME_LEN                     20

/* Trustedvm domain id 0x08 for all CSID path */
#define CAM_IFE_CSID_PATH_DOMAIN_ID                    0x08080808

char *cam_ife_csid_ver2_top_reg_name[] = {
	"top",
	"top2"
};

char *cam_ife_csid_ver2_rx_reg_name[] = {
	"rx",
	"rx2"
};

static void cam_ife_csid_ver2_print_debug_reg_status(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_isp_resource_node    *res);

static int cam_ife_csid_ver2_print_hbi_vbi(
	struct cam_ife_csid_ver2_hw  *csid_hw);


static void cam_ife_csid_ver2_reset_csid_params(struct cam_ife_csid_ver2_hw *csid_hw)
{
	memset(&csid_hw->rx_cfg, 0, sizeof(struct cam_ife_csid_ver2_rx_cfg));
	memset(&csid_hw->top_cfg, 0, sizeof(struct cam_ife_csid_ver2_top_cfg));
	memset(&csid_hw->debug_info, 0, sizeof(struct cam_ife_csid_ver2_debug_info));
	memset(&csid_hw->counters, 0, sizeof(struct cam_ife_csid_hw_counters));
	csid_hw->flags.pf_err_detected = false;
	csid_hw->standby_asserted = false;
	csid_hw->token = NULL;
}

static bool cam_ife_csid_ver2_cpas_cb(
	uint32_t handle, void *user_data, struct cam_cpas_irq_data *irq_data)
{
	bool handled = false;
	struct cam_ife_csid_ver2_hw *csid_hw = (struct cam_ife_csid_ver2_hw *)user_data;

	if (!csid_hw || !irq_data)
		return false;

	switch (irq_data->irq_type) {
	case CAM_CAMNOC_IRQ_SLAVE_ERROR:
		if (irq_data->u.slave_err.errlog0_low.err_code == CAM_CAMNOC_ADDRESS_DECODE_ERROR) {
			csid_hw->flags.pf_err_detected = true;
			CAM_DBG(CAM_ISP, "CPAS address decode error rxved for CSID[%u]",
				csid_hw->hw_intf->hw_idx);
		}
		handled = true;
		break;
	default:
		break;
	}

	return handled;
}

static void cam_ife_csid_ver2_decode_format_validate_on_err(
	struct cam_ife_csid_ver2_hw *csid_hw, uint32_t *vc,
	uint32_t *dt_arr, uint32_t *decode_fmt_arr, int num_dt)
{
	int i, j;
	bool payload_fmt = false, vc_match;
	uint32_t decode_fmt;
	const struct cam_ife_csid_ver2_reg_info *csid_reg =
		(struct cam_ife_csid_ver2_reg_info *)csid_hw->core_info->csid_reg;

	decode_fmt = csid_reg->cmn_reg->decode_format_payload_only;

	for (i = 0; i < num_dt; i++)
		payload_fmt |= (decode_fmt == decode_fmt_arr[i]);

	for (i = 0; payload_fmt && (i < num_dt); i++) {
		if (decode_fmt_arr[i] != decode_fmt) {
			CAM_ERR(CAM_ISP, "CSID:%u decode_fmt_%d mismatch: 0x%x",
				csid_hw->hw_intf->hw_idx, i, decode_fmt_arr[i]);
		}
	}

	vc_match = (vc[0] == vc[1]);

	for (i = 0; vc_match && (i < num_dt); i++) {
		for (j = 0; j < i; j++) {
			if ((dt_arr[i] == dt_arr[j]) && (decode_fmt_arr[i] != decode_fmt_arr[j])) {
				CAM_ERR(CAM_ISP, "CSID:%u Wrong multi VC-DT configuration",
					csid_hw->hw_intf->hw_idx);

				CAM_ERR(CAM_ISP,
					"CSID:%u fmt %d fmt1 %d vc %d vc1 %d dt%d %d dt%d %d",
					csid_hw->hw_intf->hw_idx,
					decode_fmt_arr[i], decode_fmt_arr[j],
					vc[0], vc[1], i, dt_arr[i],
					j, dt_arr[j]);
			}
		}
	}
}

static int cam_ife_csid_ver2_decode_format_validate(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_isp_resource_node    *res)
{
	int rc = 0, i, j;
	const struct cam_ife_csid_ver2_reg_info *csid_reg =
		(struct cam_ife_csid_ver2_reg_info *)csid_hw->core_info->csid_reg;
	struct cam_ife_csid_ver2_path_cfg *path_cfg =
		(struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
	struct cam_ife_csid_cid_data *cid_data = &csid_hw->cid_data[path_cfg->cid];
	bool payload_fmt = false;

	/* Validation is only required  for multi vc dt use case */
	if (!cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_1].valid)
		return rc;

	/**
	 * If any of the decode formats are for payload, HW requires all other
	 * decode formats to be for payload as well, loop over twice to check
	 */
	for (i = 0; i < CAM_ISP_VC_DT_CFG; i++) {
		if (!cid_data->vc_dt[i].valid)
			break;
		if (path_cfg->path_format[i].decode_fmt ==
			csid_reg->cmn_reg->decode_format_payload_only)
			payload_fmt = true;
	}

	for (i = 0; payload_fmt && (i < CAM_ISP_VC_DT_CFG); i++) {
		if (path_cfg->path_format[i].decode_fmt !=
			csid_reg->cmn_reg->decode_format_payload_only) {
			CAM_ERR(CAM_ISP, "CSID:%u payload decode_fmt_%d mismatch: 0x%x",
				csid_hw->hw_intf->hw_idx, i, path_cfg->path_format[i].decode_fmt);
			rc = -EINVAL;
			goto err;
		}
	}

	for (i = 0; i < CAM_ISP_VC_DT_CFG; i++) {
		if (!cid_data->vc_dt[i].valid)
			break;
		for (j = 0; j < i; j++) {
			if ((cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_1].vc ==
				cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_0].vc) &&
				(cid_data->vc_dt[i].dt == cid_data->vc_dt[j].dt)) {
				if (path_cfg->path_format[i].decode_fmt !=
					path_cfg->path_format[j].decode_fmt) {
					CAM_ERR(CAM_ISP,
						"CSID:%u Wrong multi VC-DT configuration",
							csid_hw->hw_intf->hw_idx);

					CAM_ERR(CAM_ISP,
						"CSID:%u fmt %d fmt1 %d vc %d vc1 %d dt%d %d dt%d %d",
						csid_hw->hw_intf->hw_idx,
						path_cfg->path_format[i].decode_fmt,
						path_cfg->path_format[j].decode_fmt,
						cid_data->vc_dt[0].vc,
						cid_data->vc_dt[1].vc,
						i, cid_data->vc_dt[i].dt,
						j, cid_data->vc_dt[j].dt);
					rc = -EINVAL;
					goto err;
				}
			}
		}
	}

	return rc;
err:
	CAM_ERR(CAM_ISP, "Invalid decode fmt cfg CSID[%u] res [id %d name %s] rc %d",
		csid_hw->hw_intf->hw_idx, res->res_id, res->res_name, rc);
	return rc;
}

static bool cam_ife_csid_ver2_disable_sof_retime(
	struct cam_ife_csid_ver2_hw     *csid_hw,
	struct cam_isp_resource_node    *res)
{
	struct cam_ife_csid_ver2_reg_info  *csid_reg = (struct cam_ife_csid_ver2_reg_info *)
								csid_hw->core_info->csid_reg;
	struct cam_ife_csid_ver2_path_cfg  *path_cfg = (struct cam_ife_csid_ver2_path_cfg *)
								res->res_priv;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = csid_reg->path_reg[res->res_id];

	if (!(path_reg->capabilities & CAM_IFE_CSID_CAP_SOF_RETIME_DIS))
		return false;

	if (path_reg->disable_sof_retime_default)
		return true;

	if (path_cfg->sfe_shdr || path_cfg->lcr_en)
		return true;

	if (csid_hw->flags.rdi_lcr_en && res->res_id == CAM_IFE_PIX_PATH_RES_PPP)
		return true;

	return false;
}

static uint64_t __cam_ife_csid_ver2_get_time_stamp(void __iomem *mem_base,
	uint32_t timestamp0_addr, uint32_t timestamp1_addr,
	bool ts_comb_vcdt_en, uint32_t ts_comb_vcdt_mask)
{
	uint64_t timestamp_val, time_hi, time_lo, mask;

	time_hi = cam_io_r_mb(mem_base + timestamp1_addr);
	time_lo = cam_io_r_mb(mem_base + timestamp0_addr);
	mask = (uint64_t)ts_comb_vcdt_mask;
	if (ts_comb_vcdt_en)
		time_lo &= ~mask;

	timestamp_val = (time_hi << 32) | time_lo;

	return mul_u64_u32_div(timestamp_val,
		CAM_IFE_CSID_QTIMER_MUL_FACTOR,
		CAM_IFE_CSID_QTIMER_DIV_FACTOR);
}

static void cam_ife_csid_ver2_print_camif_timestamps(
	struct cam_ife_csid_ver2_hw  *csid_hw)
{
	struct   cam_ife_csid_ver2_path_cfg    *path_cfg;
	struct   cam_isp_resource_node         *res;
	int                                     i;
	struct cam_hw_soc_info                 *soc_info = &csid_hw->hw_info->soc_info;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg;
	struct cam_ife_csid_ver2_reg_info *csid_reg =
		(struct cam_ife_csid_ver2_reg_info *)csid_hw->core_info->csid_reg;
	uint32_t                                j;

	for (i = CAM_IFE_PIX_PATH_RES_RDI_0; i < CAM_IFE_PIX_PATH_RES_MAX; i++) {
		struct cam_ife_csid_cid_data *cid_data;

		res = &csid_hw->path_res[i];
		path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
		if (!path_cfg || !path_cfg->irq_handle)
			continue;

		path_reg = csid_reg->path_reg[res->res_id];
		cid_data = &(csid_hw->cid_data[path_cfg->cid]);

		CAM_INFO(CAM_ISP,
				"CSID[%u] %s SOF[%lld:%lld] EPOCH[%lld:%lld] EOF[%lld:%lld]",
				csid_hw->hw_intf->hw_idx, res->res_name,
				path_cfg->sof_ts.tv_sec, path_cfg->sof_ts.tv_nsec,
				path_cfg->epoch_ts.tv_sec, path_cfg->epoch_ts.tv_nsec,
				path_cfg->eof_ts.tv_sec, path_cfg->eof_ts.tv_nsec);

		for (j = 0; j < cid_data->num_vc_dt; j++) {
			CAM_INFO(CAM_ISP,
					"cid:%u, vc[%u]:%u, dt[%u]:%u, valid[%u]:%u",
					j, j, cid_data->vc_dt[j].vc, j, cid_data->vc_dt[j].dt,
					j, cid_data->vc_dt[j].valid);
		}

		if (!path_reg) {
			CAM_ERR(CAM_ISP, "CSID:%d no path registers for res :%d",
					csid_hw->hw_intf->hw_idx, res->res_id);
			return;
		}

		CAM_INFO(CAM_ISP,
				"qtimer current [SOF: %lld EOF: %lld] prev [SOF: %lld EOF: %lld]",
				__cam_ife_csid_ver2_get_time_stamp(
					soc_info->reg_map[0].mem_base,
					path_reg->timestamp_curr0_sof_addr,
					path_reg->timestamp_curr1_sof_addr,
					path_cfg->ts_comb_vcdt_en,
					csid_reg->cmn_reg->ts_comb_vcdt_mask),
				__cam_ife_csid_ver2_get_time_stamp(
					soc_info->reg_map[0].mem_base,
					path_reg->timestamp_curr0_eof_addr,
					path_reg->timestamp_curr1_eof_addr,
					path_cfg->ts_comb_vcdt_en,
					csid_reg->cmn_reg->ts_comb_vcdt_mask),
				__cam_ife_csid_ver2_get_time_stamp(
					soc_info->reg_map[0].mem_base,
					path_reg->timestamp_perv0_sof_addr,
					path_reg->timestamp_perv1_sof_addr,
					path_cfg->ts_comb_vcdt_en,
					csid_reg->cmn_reg->ts_comb_vcdt_mask),
				__cam_ife_csid_ver2_get_time_stamp(
					soc_info->reg_map[0].mem_base,
					path_reg->timestamp_perv0_eof_addr,
					path_reg->timestamp_perv1_eof_addr,
					path_cfg->ts_comb_vcdt_en,
					csid_reg->cmn_reg->ts_comb_vcdt_mask));

	}

}

static void cam_ife_csid_ver2_update_event_ts(
	struct timespec64 *dst_ts, struct timespec64 *src_ts)
{
	dst_ts->tv_sec = src_ts->tv_sec;
	dst_ts->tv_nsec = src_ts->tv_nsec;
}

static void cam_ife_csid_ver2_send_cdr_sweep_csi2_rx_vals(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_ife_csid_ver2_reg_info *csid_reg,
	uint32_t csi2_rx_val)
{
	struct cam_subdev_msg_cdr_sweep_info csid_cdr_sweep_info;
	const struct cam_ife_csid_ver2_csi2_rx_reg_info *csi2_reg;
	struct cam_hw_soc_info *soc_info;
	void  __iomem *base;

	if (unlikely(!csid_hw || !csid_reg))
		return;

	if (!csid_hw->debug_info.cdr_sweep_debug_enabled)
		return;

	csi2_reg = csid_reg->csi2_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	base  = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	csid_cdr_sweep_info.phy_idx = (csid_hw->rx_cfg.phy_sel - 1);
	csid_cdr_sweep_info.csi2_rx_total_crc_err = cam_io_r_mb(base +
		csi2_reg->total_crc_err_addr);
	csid_cdr_sweep_info.csi2_rx_total_pkts_rcvd = cam_io_r_mb(base +
		csi2_reg->total_pkts_rcvd_addr);
	csid_cdr_sweep_info.csi2_rx_status = csi2_rx_val ? csi2_rx_val :
		cam_io_r_mb(base + csi2_reg->irq_status_addr[
			CAM_IFE_CSID_RX_IRQ_STATUS_REG0]);
	csid_cdr_sweep_info.lane_cfg = csid_hw->rx_cfg.lane_cfg;
	csid_cdr_sweep_info.epd_enabled = csid_hw->rx_cfg.epd_supported;
	csid_cdr_sweep_info.csi2_err_seen = (csi2_rx_val != 0);

	cam_subdev_notify_message(CAM_CSIPHY_DEVICE_TYPE, CAM_SUBDEV_MESSAGE_CDR_SWEEP,
		(void *)&csid_cdr_sweep_info);
}

static int cam_ife_csid_ver2_set_debug(
	struct cam_ife_csid_ver2_hw        *csid_hw,
	struct cam_ife_csid_debug_cfg_args *debug_args)
{
	int                                  bit_pos = 0;
	uint32_t                             val, debug_val;
	int                                  i = 0;
	const uint8_t                       *dbg_bit_pos = NULL;
	const uint64_t                      *evt_bitmap = NULL;

	struct cam_ife_csid_ver2_reg_info *csid_reg = (struct cam_ife_csid_ver2_reg_info *)
							csid_hw->core_info->csid_reg;
	memset(&csid_hw->debug_info, 0,
		sizeof(struct cam_ife_csid_ver2_debug_info));
	csid_hw->debug_info.debug_val = debug_args->csid_debug;
	csid_hw->debug_info.test_bus_val = debug_args->csid_testbus_debug;
	csid_hw->debug_info.set_domain_id_enabled = debug_args->set_domain_id_enabled;
	csid_hw->debug_info.domain_id_value = debug_args->domain_id_value;
	csid_hw->debug_info.cdr_sweep_debug_enabled = debug_args->enable_cdr_sweep_debug;
#if defined(CONFIG_SAMSUNG_CAMERA)
	csid_hw->debug_info.dbg_fps = debug_args->csid_dbg_fps;
#endif

	evt_bitmap = csid_reg->rx_debug_mask->evt_bitmap;
	dbg_bit_pos = csid_reg->rx_debug_mask->bit_pos;

	/*
	 * RX capture debug
	 * [0:3]   = rst strobes
	 * [4:11]  = vc for capture
	 * [12:19] = dt for capture
	 */
	csid_hw->debug_info.rst_capture_strobes = (debug_args->csid_rx_capture_debug &
		CAM_IFE_CSID_DEBUGFS_RST_STROBE_MASK);
	csid_hw->debug_info.rx_capture_vc = ((debug_args->csid_rx_capture_debug >>
		CAM_IFE_CSID_DEBUGFS_VC_SHIFT_MASK) & CAM_IFE_CSID_DEBUGFS_VC_DT_MASK);
	csid_hw->debug_info.rx_capture_dt = ((debug_args->csid_rx_capture_debug >>
		CAM_IFE_CSID_DEBUGFS_DT_SHIFT_MASK) & CAM_IFE_CSID_DEBUGFS_VC_DT_MASK);
	csid_hw->debug_info.rx_capture_debug_set = debug_args->rx_capture_debug_set;

	debug_val = csid_hw->debug_info.debug_val;
	while (debug_val) {

		if (!(debug_val & 0x1)) {
			debug_val >>= 1;
			bit_pos++;
			continue;
		}

		val = BIT(bit_pos);

		switch (val) {
		case CAM_IFE_CSID_DEBUG_ENABLE_SOF_IRQ:
			csid_hw->debug_info.path_mask |=
				IFE_CSID_VER2_PATH_INFO_INPUT_SOF;
			break;
		case CAM_IFE_CSID_DEBUG_ENABLE_EOF_IRQ:
			csid_hw->debug_info.path_mask |=
				IFE_CSID_VER2_PATH_INFO_INPUT_EOF;
			break;
		case CAM_IFE_CSID_DEBUG_ENABLE_SOT_IRQ:
			for (i = 0; i < csid_reg->num_rx_regs; i++) {
				if (evt_bitmap[i] & BIT_ULL(CAM_IFE_CSID_RX_DL0_SOT_CAPTURED)) {
					csid_hw->debug_info.rx_mask[i] |=
						BIT(dbg_bit_pos[CAM_IFE_CSID_RX_DL0_SOT_CAPTURED]) |
						BIT(dbg_bit_pos[CAM_IFE_CSID_RX_DL1_SOT_CAPTURED]) |
						BIT(dbg_bit_pos[CAM_IFE_CSID_RX_DL2_SOT_CAPTURED]) |
						BIT(dbg_bit_pos[CAM_IFE_CSID_RX_DL3_SOT_CAPTURED]);
				}
			}
			break;
		case CAM_IFE_CSID_DEBUG_ENABLE_EOT_IRQ:
			for (i = 0; i < csid_reg->num_rx_regs; i++) {
				if (evt_bitmap[i] & BIT_ULL(CAM_IFE_CSID_RX_DL0_EOT_CAPTURED)) {
					csid_hw->debug_info.rx_mask[i] |=
						BIT(dbg_bit_pos[CAM_IFE_CSID_RX_DL0_EOT_CAPTURED]) |
						BIT(dbg_bit_pos[CAM_IFE_CSID_RX_DL1_EOT_CAPTURED]) |
						BIT(dbg_bit_pos[CAM_IFE_CSID_RX_DL2_EOT_CAPTURED]) |
						BIT(dbg_bit_pos[CAM_IFE_CSID_RX_DL3_EOT_CAPTURED]);
				}
			}
			break;
		case CAM_IFE_CSID_DEBUG_ENABLE_SHORT_PKT_CAPTURE:
			for (i = 0; i < csid_reg->num_rx_regs; i++) {
				if (evt_bitmap[i] & BIT_ULL(CAM_IFE_CSID_RX_SHORT_PKT_CAPTURED)) {
					csid_hw->debug_info.rx_mask[i] |=
						BIT(dbg_bit_pos
							[CAM_IFE_CSID_RX_SHORT_PKT_CAPTURED]);
				}
			}
			break;
		case CAM_IFE_CSID_DEBUG_ENABLE_LONG_PKT_CAPTURE:
			for (i = 0; i < csid_reg->num_rx_regs; i++) {
				if (evt_bitmap[i] & BIT_ULL(CAM_IFE_CSID_RX_LONG_PKT_CAPTURED)) {
					csid_hw->debug_info.rx_mask[i] |=
						BIT(dbg_bit_pos[CAM_IFE_CSID_RX_LONG_PKT_CAPTURED]);
				}
			}
			break;
		case CAM_IFE_CSID_DEBUG_ENABLE_CPHY_PKT_CAPTURE:
			for (i = 0; i < csid_reg->num_rx_regs; i++) {
				if (evt_bitmap[i] &
					BIT_ULL(CAM_IFE_CSID_RX_CPHY_PKT_HDR_CAPTURED)) {
					csid_hw->debug_info.rx_mask[i] |=
						BIT(dbg_bit_pos
							[CAM_IFE_CSID_RX_CPHY_PKT_HDR_CAPTURED]);
				}
			}
			break;
		case CAM_IFE_DEBUG_ENABLE_UNMAPPED_VC_DT_IRQ:
			for (i = 0; i < csid_reg->num_rx_regs; i++) {
				if (evt_bitmap[i] & BIT_ULL(CAM_IFE_CSID_RX_UNMAPPED_VC_DT)) {
					csid_hw->debug_info.rx_mask[i] |=
						BIT(dbg_bit_pos[CAM_IFE_CSID_RX_UNMAPPED_VC_DT]);
				}
			}
			break;
		case CAM_IFE_CSID_DEBUG_ENABLE_VOTE_UP_IRQ:
			evt_bitmap = csid_reg->top_debug_mask->evt_bitmap;
			dbg_bit_pos = csid_reg->top_debug_mask->bit_pos;

			for (i = 0; i < csid_reg->num_top_regs; i++) {
				if (evt_bitmap[i] & BIT_ULL(CAM_IFE_CSID_TOP_INFO_VOTE_UP)) {
					csid_hw->debug_info.top_mask[i] |=
						BIT(dbg_bit_pos[CAM_IFE_CSID_TOP_INFO_VOTE_UP]);
				}
			}
			break;
		case CAM_IFE_CSID_DEBUG_ENABLE_VOTE_DN_IRQ:
			evt_bitmap = csid_reg->top_debug_mask->evt_bitmap;
			dbg_bit_pos = csid_reg->top_debug_mask->bit_pos;

			for (i = 0; i < csid_reg->num_top_regs; i++) {
				if (evt_bitmap[i] & BIT_ULL(CAM_IFE_CSID_TOP_INFO_VOTE_DN)) {
					csid_hw->debug_info.top_mask[i] |=
						BIT(dbg_bit_pos[CAM_IFE_CSID_TOP_INFO_VOTE_DN]);
				}
			}
			break;
		case CAM_IFE_CSID_DEBUG_ENABLE_ERR_NO_VOTE_DN_IRQ:
			evt_bitmap = csid_reg->top_debug_mask->evt_bitmap;
			dbg_bit_pos = csid_reg->top_debug_mask->bit_pos;

			for (i = 0; i < csid_reg->num_top_regs; i++) {
				if (evt_bitmap[i] & BIT_ULL(CAM_IFE_CSID_TOP_ERR_NO_VOTE_DN)) {
					csid_hw->debug_info.top_mask[i] |=
						BIT(dbg_bit_pos[CAM_IFE_CSID_TOP_ERR_NO_VOTE_DN]);
				}
			}
			break;
		default:
			break;
		}

		debug_val >>= 1;
		bit_pos++;
	}

	return 0;
}

static int cam_ife_csid_ver2_sof_irq_debug(
	struct cam_ife_csid_ver2_hw *csid_hw,
	void *cmd_args)
{
	int      i = 0;
	int      data_idx = 0;
	bool     sof_irq_enable = false;
	struct   cam_ife_csid_ver2_reg_info    *csid_reg;
	struct   cam_ife_csid_ver2_path_cfg    *path_cfg;
	struct   cam_isp_resource_node         *res;
	uint32_t irq_mask = 0;

	if (*((uint32_t *)cmd_args) == 1)
		sof_irq_enable = true;

	if (csid_hw->hw_info->hw_state ==
		CAM_HW_STATE_POWER_DOWN) {
		CAM_WARN(CAM_ISP,
			"CSID:%u powered down unable to %s sof irq",
			csid_hw->hw_intf->hw_idx,
			(sof_irq_enable) ? "enable" : "disable");
		return 0;
	}

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	for (i = CAM_IFE_PIX_PATH_RES_RDI_0; i < CAM_IFE_PIX_PATH_RES_MAX;
		i++) {

		res = &csid_hw->path_res[i];
		path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;

		if (!path_cfg || !path_cfg->irq_handle)
			continue;

		/* Assuming controller has only 1 register set */
		irq_mask = IFE_CSID_VER2_PATH_INFO_INPUT_SOF;
		cam_irq_controller_update_irq(
			csid_hw->path_irq_controller[res->res_id],
			path_cfg->irq_handle,
			sof_irq_enable, &irq_mask);
	}

	if (sof_irq_enable) {
		csid_hw->debug_info.path_mask |=
			IFE_CSID_VER2_PATH_INFO_INPUT_SOF;
		csid_hw->flags.sof_irq_triggered = true;
	} else {
		csid_hw->debug_info.path_mask &=
			~IFE_CSID_VER2_PATH_INFO_INPUT_SOF;
		csid_hw->flags.sof_irq_triggered = false;
	}

	CAM_INFO(CAM_ISP, "SOF freeze: CSID:%u SOF irq %s, Notify CSIPHY: %d",
		csid_hw->hw_intf->hw_idx,
		(sof_irq_enable) ? "enabled" : "disabled", csid_hw->rx_cfg.phy_sel - 1);

	data_idx = (int)(csid_hw->rx_cfg.phy_sel - csid_reg->cmn_reg->phy_sel_base_idx);
	if (sof_irq_enable) {
		if (data_idx < 0)
			CAM_WARN(CAM_ISP, "CSID:%u Can't notify csiphy, incorrect phy selected=%d",
			csid_hw->hw_intf->hw_idx, data_idx);
		else {
			CAM_INFO(CAM_ISP, "CSID:%u Notify CSIPHY: %d", csid_hw->hw_intf->hw_idx,
				data_idx);
			cam_subdev_notify_message(CAM_CSIPHY_DEVICE_TYPE,
				CAM_SUBDEV_MESSAGE_REG_DUMP, (void *)&data_idx);
#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
			hw_bigdata_mipi_from_ife_csid_ver2(csid_hw->rx_cfg.phy_sel - 1);
#endif
		}
	}

	return 0;
}

static int cam_ife_csid_ver2_get_evt_payload(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_ife_csid_ver2_evt_payload **evt_payload,
	struct list_head    *payload_list,
	spinlock_t          *lock)
{

	spin_lock(lock);

	if (list_empty(payload_list)) {
		*evt_payload = NULL;
		spin_unlock(lock);
		CAM_ERR_RATE_LIMIT(CAM_ISP, "No free payload core %d",
			csid_hw->hw_intf->hw_idx);
		return -ENOMEM;
	}

	*evt_payload = list_first_entry(payload_list,
			struct cam_ife_csid_ver2_evt_payload, list);
	list_del_init(&(*evt_payload)->list);
	spin_unlock(lock);

	return 0;
}

static int cam_ife_csid_ver2_put_evt_payload(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_ife_csid_ver2_evt_payload **evt_payload,
	struct list_head    *payload_list,
	spinlock_t          *lock)
{
	unsigned long flags;

	if (*evt_payload == NULL) {
		CAM_ERR_RATE_LIMIT(CAM_ISP, "Invalid payload core %d",
			csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	CAM_COMMON_SANITIZE_LIST_ENTRY((*evt_payload), struct cam_ife_csid_ver2_evt_payload);
	spin_lock_irqsave(lock, flags);
	list_add_tail(&(*evt_payload)->list, payload_list);
	*evt_payload = NULL;
	spin_unlock_irqrestore(lock, flags);

	return 0;
}

static int cam_ife_csid_ver2_top_info_irq_top_half(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	int rc = 0;
	struct cam_ife_csid_ver2_hw               *csid_hw = NULL;
	struct cam_ife_csid_ver2_evt_payload      *evt_payload;

	csid_hw = th_payload->handler_priv;

	rc  = cam_ife_csid_ver2_get_evt_payload(csid_hw, &evt_payload,
			&csid_hw->path_free_payload_list,
			&csid_hw->path_payload_lock);
	if (rc) {
		CAM_WARN_RATE_LIMIT(CAM_ISP, "CSID:%u get payload failed, TOP info status: 0x%x",
			csid_hw->hw_intf->hw_idx,
			th_payload->evt_status_arr[0]);
		return rc;
	}

	CAM_DBG(CAM_ISP, "CSID:%u TOP info status: 0x%x", csid_hw->hw_intf->hw_idx,
		th_payload->evt_status_arr[0]);

	evt_payload->irq_reg_val = th_payload->evt_status_arr[0];
	ktime_get_boottime_ts64(&evt_payload->timestamp);
	th_payload->evt_payload_priv = evt_payload;

	return 0;
}

static int cam_ife_csid_ver2_top2_info_irq_top_half(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	return cam_ife_csid_ver2_top_info_irq_top_half(evt_id, th_payload);
}


static int cam_ife_csid_ver2_top_err_irq_top_half(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	int rc = 0;
	struct cam_ife_csid_ver2_hw               *csid_hw = NULL;
	struct cam_ife_csid_ver2_evt_payload      *evt_payload;

	csid_hw = th_payload->handler_priv;

	rc  = cam_ife_csid_ver2_get_evt_payload(csid_hw, &evt_payload,
			&csid_hw->path_free_payload_list,
			&csid_hw->path_payload_lock);

	CAM_DBG(CAM_ISP, "CSID:%u TOP status: 0x%x", csid_hw->hw_intf->hw_idx,
		th_payload->evt_status_arr[0]);

	if (rc) {
		CAM_INFO_RATE_LIMIT(CAM_ISP, "CSID:%u TOP status: 0x%x",
			csid_hw->hw_intf->hw_idx, th_payload->evt_status_arr[0]);
		return rc;
	}

	evt_payload->irq_reg_val = th_payload->evt_status_arr[0];
	ktime_get_boottime_ts64(&evt_payload->timestamp);
	th_payload->evt_payload_priv = evt_payload;

	return 0;
}

static int cam_ife_csid_ver2_handle_buf_done_irq(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	struct cam_ife_csid_ver2_hw               *csid_hw = NULL;
	int rc = 0;

	csid_hw = th_payload->handler_priv;

	CAM_DBG(CAM_ISP, "CSID:%u Enter", csid_hw->hw_intf->hw_idx);
	rc = cam_irq_controller_handle_irq(evt_id,
		csid_hw->buf_done_irq_controller, CAM_IRQ_EVT_GROUP_0);
	CAM_DBG(CAM_ISP, "CSID:%u Exit (rc=%d)", csid_hw->hw_intf->hw_idx, rc);

	return (rc == IRQ_HANDLED) ? 0 : -EINVAL;
}

static int cam_ife_csid_ver2_handle_top2_irq(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	struct cam_ife_csid_ver2_hw       *csid_hw = th_payload->handler_priv;
	int rc = 0;

	rc = cam_irq_controller_handle_irq(evt_id,
		csid_hw->top_irq_controller[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1],
		CAM_IRQ_EVT_GROUP_0);
	CAM_DBG(CAM_ISP, "CSID:%d Exit (rc=%d)", csid_hw->hw_intf->hw_idx, rc);

	return (rc == IRQ_HANDLED) ? 0 : -EINVAL;
}

static int cam_ife_csid_ver2_handle_rx_irq(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	struct cam_ife_csid_ver2_hw               *csid_hw = NULL;
	int rc = 0;

	csid_hw = th_payload->handler_priv;

	rc = cam_irq_controller_handle_irq(evt_id,
		csid_hw->rx_irq_controller[CAM_IFE_CSID_RX_IRQ_STATUS_REG0],
		CAM_IRQ_EVT_GROUP_0);
	CAM_DBG(CAM_ISP, "CSID:%d Exit (rc=%d)", csid_hw->hw_intf->hw_idx, rc);

	return (rc == IRQ_HANDLED) ? 0 : -EINVAL;
}

static int cam_ife_csid_ver2_handle_rx2_irq(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	struct cam_ife_csid_ver2_hw               *csid_hw = NULL;
	int rc = 0;

	csid_hw = th_payload->handler_priv;

	rc = cam_irq_controller_handle_irq(evt_id,
		csid_hw->rx_irq_controller[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1],
		CAM_IRQ_EVT_GROUP_0);
	CAM_DBG(CAM_ISP, "CSID:%d Exit (rc=%d)", csid_hw->hw_intf->hw_idx, rc);

	return (rc == IRQ_HANDLED) ? 0 : -EINVAL;
}

static int cam_ife_csid_ver2_handle_path_irq(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	struct cam_isp_resource_node      *res = th_payload->handler_priv;
	struct cam_ife_csid_ver2_hw       *csid_hw = cam_isp_res_core_info(res);
	int rc = 0;

	CAM_DBG(CAM_ISP, "CSID:%u %s Enter", csid_hw->hw_intf->hw_idx, res->res_name);
	rc = cam_irq_controller_handle_irq(evt_id,
		csid_hw->path_irq_controller[res->res_id], CAM_IRQ_EVT_GROUP_0);
	CAM_DBG(CAM_ISP, "CSID:%u %s Exit (rc=%d)", csid_hw->hw_intf->hw_idx, res->res_name, rc);

	return (rc == IRQ_HANDLED) ? 0 : -EINVAL;
}

static int cam_ife_csid_ver2_path_err_top_half(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	int32_t                                    rc;
	struct cam_hw_info                        *hw_info;
	struct cam_ife_csid_ver2_hw               *csid_hw = NULL;
	struct cam_ife_csid_ver2_evt_payload      *evt_payload;
	struct cam_isp_resource_node              *res;
	const uint8_t                            **irq_reg_tag;
	struct cam_ife_csid_ver2_path_cfg         *path_cfg;

	res  = th_payload->handler_priv;

	if (!res) {
		CAM_ERR_RATE_LIMIT(CAM_ISP, "No private returned");
		return -ENODEV;
	}

	hw_info = (struct cam_hw_info *)res->hw_intf->hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;

	rc  = cam_ife_csid_ver2_get_evt_payload(csid_hw, &evt_payload,
			&csid_hw->path_free_payload_list,
			&csid_hw->path_payload_lock);
	irq_reg_tag = cam_ife_csid_get_irq_reg_tag_ptr();

	CAM_DBG(CAM_ISP, "CSID:%u %s status: 0x%x",
		csid_hw->hw_intf->hw_idx,
		irq_reg_tag[path_cfg->irq_reg_idx],
		th_payload->evt_status_arr[0]);

	if (rc) {
		CAM_INFO_RATE_LIMIT(CAM_ISP, "CSID:%u %s status: 0x%x",
			csid_hw->hw_intf->hw_idx,
			irq_reg_tag[path_cfg->irq_reg_idx],
			th_payload->evt_status_arr[0]);
		return rc;
	}

	evt_payload->irq_reg_val = th_payload->evt_status_arr[0];

	ktime_get_boottime_ts64(&path_cfg->error_ts);
	th_payload->evt_payload_priv = evt_payload;

	return 0;
}

static int cam_ife_csid_ver2_path_top_half(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	int32_t                                       rc;
	struct cam_hw_info                           *hw_info;
	struct cam_ife_csid_ver2_hw                  *csid_hw = NULL;
	struct cam_ife_csid_ver2_evt_payload         *evt_payload;
	struct cam_isp_resource_node                 *res;
	const uint8_t                                **irq_reg_tag;
	struct cam_ife_csid_ver2_path_cfg            *path_cfg;
	struct cam_ife_csid_ver2_reg_info            *csid_reg = NULL;
	struct cam_hw_soc_info                       *soc_info;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg;

	res  = th_payload->handler_priv;

	if (!res) {
		CAM_ERR_RATE_LIMIT(CAM_ISP, "No private returned");
		return -ENODEV;
	}

	hw_info = (struct cam_hw_info *)res->hw_intf->hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	path_reg = csid_reg->path_reg[res->res_id];
	soc_info = &csid_hw->hw_info->soc_info;

	rc  = cam_ife_csid_ver2_get_evt_payload(csid_hw, &evt_payload,
			&csid_hw->path_free_payload_list,
			&csid_hw->path_payload_lock);
	irq_reg_tag = cam_ife_csid_get_irq_reg_tag_ptr();

	CAM_DBG(CAM_ISP, "CSID:%u %s status: 0x%x",
		csid_hw->hw_intf->hw_idx,
		(th_payload->is_comp_irq ? "COMP_IRQ" :
		((char *) irq_reg_tag[path_cfg->irq_reg_idx])),
		th_payload->evt_status_arr[0]);

	if (rc) {
		CAM_INFO_RATE_LIMIT(CAM_ISP, "CSID:%u %s status: 0x%x",
			csid_hw->hw_intf->hw_idx,
			irq_reg_tag[path_cfg->irq_reg_idx],
			th_payload->evt_status_arr[0]);
		return rc;
	}

	evt_payload->irq_reg_val = th_payload->evt_status_arr[0];

	if (((evt_payload->irq_reg_val & path_reg->sof_irq_mask) ||
		((path_reg->capabilities & CAM_IFE_CSID_CAP_MULTI_CTXT) &&
		(evt_payload->irq_reg_val & csid_reg->ipp_mc_reg->comp_sof_mask)))
		&& path_cfg->handle_camif_irq) {
		evt_payload->sof_ts_reg_val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			path_reg->timestamp_curr1_sof_addr);
		evt_payload->sof_ts_reg_val = (evt_payload->sof_ts_reg_val << 32) |
			cam_io_r_mb(soc_info->reg_map[0].mem_base +
			path_reg->timestamp_curr0_sof_addr);
	}

	ktime_get_boottime_ts64(&evt_payload->timestamp);
	th_payload->evt_payload_priv = evt_payload;

	return 0;
}

static int cam_ife_csid_ver2_mc_top_half(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	int32_t                                    rc;
	struct cam_ife_csid_ver2_evt_payload      *evt_payload;
	struct cam_isp_resource_node              *res;

	res  = th_payload->handler_priv;

	if (!res) {
		CAM_ERR_RATE_LIMIT(CAM_ISP, "No private returned");
		return -ENODEV;
	}

	th_payload->is_comp_irq = true;
	rc = cam_ife_csid_ver2_path_top_half(evt_id, th_payload);
	evt_payload = (struct cam_ife_csid_ver2_evt_payload *)th_payload->evt_payload_priv;
	evt_payload->is_mc = true;

	if (rc) {
		CAM_ERR(CAM_ISP, "Multi context top half fail");
		return rc;
	}

	return 0;
}

static inline void cam_ife_csid_ver2_reset_discard_frame_cfg(
	struct cam_isp_resource_node                *res,
	struct cam_ife_csid_ver2_hw                 *csid_hw,
	struct cam_ife_csid_ver2_path_cfg           *path_cfg)
{
	int rc;

	/* Reset discard config params */
	path_cfg->discard_init_frames = false;
	path_cfg->skip_discard_frame_cfg = false;
	path_cfg->num_frames_discard = 0;
	path_cfg->sof_cnt = 0;

	/* Decrement discard frame ref cnt for this path */
	atomic_dec(&csid_hw->discard_frame_per_path);

	/* If input SOF irq is enabled explicitly - unsubscribe in th*/
	if (path_cfg->discard_irq_handle > 0) {
		rc = cam_irq_controller_unsubscribe_irq(
			csid_hw->path_irq_controller[res->res_id],
			path_cfg->discard_irq_handle);
		if (rc)
			CAM_WARN(CAM_ISP,
				"CSID[%u] Failed to unsubscribe input SOF for res: %s",
				csid_hw->hw_intf->hw_idx, res->res_name);

		path_cfg->discard_irq_handle = 0;
	}

	CAM_DBG(CAM_ISP, "CSID[%u] Reset discard frame config for res: %s discard_ref_cnt: %u",
		csid_hw->hw_intf->hw_idx, res->res_name,
		atomic_read(&csid_hw->discard_frame_per_path));
}

static int cam_ife_csid_ver2_discard_sof_top_half(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	struct cam_hw_info                        *hw_info;
	struct cam_ife_csid_ver2_hw               *csid_hw = NULL;
	struct cam_isp_resource_node              *res;
	struct cam_ife_csid_ver2_path_cfg         *path_cfg;

	res  = th_payload->handler_priv;

	if (!res) {
		CAM_ERR_RATE_LIMIT(CAM_ISP, "No private returned");
		return -ENODEV;
	}

	hw_info = (struct cam_hw_info *)res->hw_intf->hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;

	CAM_DBG(CAM_ISP, "CSID: %u status: 0x%x for res: %s",
		csid_hw->hw_intf->hw_idx,
		th_payload->evt_status_arr[path_cfg->irq_reg_idx],
		res->res_name);

	/* No need of payload since it's an exclusive th & bh */
	th_payload->evt_payload_priv = NULL;

	return 0;
}

static int cam_ife_csid_ver2_discard_sof_pix_bottom_half(
	void              *handler_priv,
	void              *evt_payload_priv)
{
	struct cam_hw_info                           *hw_info;
	struct cam_ife_csid_ver2_hw                  *csid_hw = NULL;
	struct cam_isp_resource_node                 *res;
	struct cam_ife_csid_ver2_reg_info            *csid_reg = NULL;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = NULL;
	struct cam_ife_csid_ver2_path_cfg            *path_cfg;
	struct cam_hw_soc_info                       *soc_info;
	void    __iomem                              *base;
	uint32_t                                      val;

	if (!handler_priv) {
		CAM_ERR(CAM_ISP, "Invalid handler_priv");
		return -EINVAL;
	}

	res   =  handler_priv;
	hw_info = (struct cam_hw_info *)res->hw_intf->hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
	soc_info = &csid_hw->hw_info->soc_info;
	base  = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	path_reg = csid_reg->path_reg[res->res_id];

	/* Count SOFs */
	path_cfg->sof_cnt++;

	CAM_DBG(CAM_ISP, "CSID[%u] Discard frame on %s path, num SOFs: %u",
		csid_hw->hw_intf->hw_idx, res->res_name, path_cfg->sof_cnt);

	spin_lock(&csid_hw->lock_state);
	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID[%u] powered down state",
			csid_hw->hw_intf->hw_idx);
		goto end;
	}

	/* Check with requested number of frames to be dropped */
	if (path_cfg->sof_cnt == path_cfg->num_frames_discard) {
		if (path_cfg->sync_mode == CAM_ISP_HW_SYNC_MASTER ||
			path_cfg->sync_mode == CAM_ISP_HW_SYNC_NONE) {
			val = cam_io_r_mb(base + path_reg->ctrl_addr);
			val |= path_reg->resume_frame_boundary;
			cam_io_w_mb(val, base + path_reg->ctrl_addr);
			CAM_DBG(CAM_ISP,
				"CSID[%u] start cmd programmed for %s sof_cnt %u",
				csid_hw->hw_intf->hw_idx,
				res->res_name,
				path_cfg->sof_cnt);
		}
		cam_ife_csid_ver2_reset_discard_frame_cfg(res, csid_hw, path_cfg);
	}
end:
	spin_unlock(&csid_hw->lock_state);
	return 0;
}

static int cam_ife_csid_ver2_discard_sof_rdi_bottom_half(
	void              *handler_priv,
	void              *evt_payload_priv)
{
	struct cam_hw_info                           *hw_info;
	struct cam_ife_csid_ver2_hw                  *csid_hw = NULL;
	struct cam_isp_resource_node                 *res;
	struct cam_ife_csid_ver2_reg_info            *csid_reg = NULL;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = NULL;
	struct cam_ife_csid_ver2_path_cfg            *path_cfg;
	struct cam_hw_soc_info                       *soc_info;
	void    __iomem                              *base;
	uint32_t                                      val;

	if (!handler_priv) {
		CAM_ERR(CAM_ISP, "Invalid handler_priv");
		return -EINVAL;
	}

	res   =  handler_priv;
	hw_info = (struct cam_hw_info *)res->hw_intf->hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
	soc_info = &csid_hw->hw_info->soc_info;
	base  = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;
	path_reg = csid_reg->path_reg[res->res_id];

	/* Count SOFs */
	path_cfg->sof_cnt++;
	CAM_DBG(CAM_ISP, "CSID[%u] Discard frame on %s path, num SOFs: %u",
		csid_hw->hw_intf->hw_idx, res->res_name, path_cfg->sof_cnt);

	spin_lock(&csid_hw->lock_state);
	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID[%u] powered down state",
			csid_hw->hw_intf->hw_idx);
		goto end;
	}

	/* Check with requested number of frames to be dropped */
	if (path_cfg->sof_cnt == path_cfg->num_frames_discard) {
		val = cam_io_r_mb(base + path_reg->ctrl_addr);
		val |= path_reg->resume_frame_boundary;
		cam_io_w_mb(val, base + path_reg->ctrl_addr);
		CAM_DBG(CAM_ISP,
			"CSID[%u] start cmd programmed for %s sof_cnt %u",
			csid_hw->hw_intf->hw_idx,
			res->res_name, path_cfg->sof_cnt);

		cam_ife_csid_ver2_reset_discard_frame_cfg(res, csid_hw, path_cfg);
	}
end:
	spin_unlock(&csid_hw->lock_state);
	return 0;
}

static int cam_ife_csid_ver2_stop_csi2_in_err(
	struct cam_ife_csid_ver2_hw  *csid_hw)
{
	struct cam_ife_csid_ver2_reg_info            *csid_reg = NULL;
	int                                           rx_idx;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	CAM_DBG(CAM_ISP, "CSID:%u Stop csi2 rx",
		csid_hw->hw_intf->hw_idx);

	if (csid_hw->rx_cfg.top_irq_handle)
		cam_irq_controller_disable_irq(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->rx_cfg.top_irq_handle);

	if (csid_reg->num_rx_regs > 1 && csid_hw->rx_cfg.rx2_irq_handle)
		cam_irq_controller_disable_irq(
			csid_hw->rx_irq_controller[CAM_IFE_CSID_RX_IRQ_STATUS_REG0],
			csid_hw->rx_cfg.rx2_irq_handle);

	for (rx_idx = CAM_IFE_CSID_RX_IRQ_STATUS_REG0; rx_idx < csid_reg->num_rx_regs; rx_idx++) {
		if (!csid_hw->rx_cfg.err_irq_handle[rx_idx])
			continue;

		cam_irq_controller_disable_irq(
			csid_hw->rx_irq_controller[rx_idx],
			csid_hw->rx_cfg.err_irq_handle[rx_idx]);
	}

	for (rx_idx = 0; rx_idx < csid_reg->num_rx_regs; rx_idx++) {
		if (!csid_hw->rx_cfg.irq_handle[rx_idx])
			continue;

		cam_irq_controller_disable_irq(
			csid_hw->rx_irq_controller[rx_idx],
			csid_hw->rx_cfg.irq_handle[rx_idx]);

	}

	return 0;
}

static inline void cam_ife_csid_ver2_maskout_rx_irqs(
	struct cam_ife_csid_ver2_hw *csid_hw)
{
	int                                      rc, rx_idx;
	const struct cam_ife_csid_ver2_reg_info *csid_reg;
	struct cam_hw_soc_info                  *soc_info;

	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	for (rx_idx = CAM_IFE_CSID_RX_IRQ_STATUS_REG0; rx_idx < csid_reg->num_rx_regs; rx_idx++) {
		if (!csid_hw->rx_cfg.err_irq_handle[rx_idx])
			continue;

		rc = cam_irq_controller_unsubscribe_irq(csid_hw->rx_irq_controller[rx_idx],
			csid_hw->rx_cfg.err_irq_handle[rx_idx]);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe rx err[%d] irq for CSID:%u rc:%d",
				rx_idx, csid_hw->hw_intf->hw_idx, rc);
		csid_hw->rx_cfg.err_irq_handle[rx_idx] = 0;
	}

	for (rx_idx = CAM_IFE_CSID_RX_IRQ_STATUS_REG0; rx_idx < csid_reg->num_rx_regs; rx_idx++) {
		if (!(csid_hw->rx_cfg.irq_handle[rx_idx]))
			continue;

		rc = cam_irq_controller_unsubscribe_irq(
			csid_hw->rx_irq_controller[rx_idx],
			csid_hw->rx_cfg.irq_handle[rx_idx]);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe rx info[%d] irq for CSID:%u rc:%d",
				rx_idx, csid_hw->hw_intf->hw_idx, rc);
		csid_hw->rx_cfg.irq_handle[rx_idx] = 0;
	}

	if (csid_reg->num_rx_regs > 1 &&
		csid_hw->rx_cfg.rx2_irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq(
			csid_hw->rx_irq_controller[CAM_IFE_CSID_RX_IRQ_STATUS_REG0],
			csid_hw->rx_cfg.rx2_irq_handle);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe rx2 irq evt in rx for CSID:%u rc:%d",
				csid_hw->hw_intf->hw_idx, rc);

		csid_hw->rx_cfg.rx2_irq_handle = 0;
		rc = cam_irq_controller_unregister_dependent(
			csid_hw->rx_irq_controller[CAM_IFE_CSID_RX_IRQ_STATUS_REG0],
			csid_hw->rx_irq_controller[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1]);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unregister rx2 dependent with rx CSID:%u rc: %d",
				csid_hw->hw_intf->hw_idx, rc);

	}

	if (csid_hw->rx_cfg.top_irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->rx_cfg.top_irq_handle);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe rx irq in top for CSID:%u rc:%d",
				csid_hw->hw_intf->hw_idx, rc);

		csid_hw->rx_cfg.top_irq_handle = 0;
		rc = cam_irq_controller_unregister_dependent(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->rx_irq_controller[CAM_IFE_CSID_RX_IRQ_STATUS_REG0]);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unregister rx dependent with top CSID:%u rc: %d",
				csid_hw->hw_intf->hw_idx, rc);
	}
}

static inline void cam_ife_csid_ver2_disable_rx_evts(
	struct cam_ife_csid_ver2_hw *csid_hw)
{
	int                                      rc, rx_idx;
	const struct cam_ife_csid_ver2_reg_info *csid_reg;
	struct cam_hw_soc_info                  *soc_info;

	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	for (rx_idx = CAM_IFE_CSID_RX_IRQ_STATUS_REG0; rx_idx < csid_reg->num_rx_regs; rx_idx++) {
		if (!csid_hw->rx_cfg.err_irq_handle[rx_idx])
			continue;

		rc = cam_irq_controller_unsubscribe_irq_evt(csid_hw->rx_irq_controller[rx_idx],
			csid_hw->rx_cfg.err_irq_handle[rx_idx]);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe rx err[%d] irq evt for CSID:%u rc:%d",
				rx_idx, csid_hw->hw_intf->hw_idx, rc);

		csid_hw->rx_cfg.err_irq_handle[rx_idx] = 0;

	}

	for (rx_idx = CAM_IFE_CSID_RX_IRQ_STATUS_REG0; rx_idx < csid_reg->num_rx_regs; rx_idx++) {
		if (!csid_hw->rx_cfg.irq_handle[rx_idx])
			continue;

		rc = cam_irq_controller_unsubscribe_irq_evt(csid_hw->rx_irq_controller[rx_idx],
			csid_hw->rx_cfg.irq_handle[rx_idx]);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe rx err[%d] irq evt for CSID:%u rc:%d",
				rx_idx, csid_hw->hw_intf->hw_idx, rc);

		csid_hw->rx_cfg.irq_handle[rx_idx] = 0;
	}

	if (csid_reg->num_rx_regs > 1 && csid_hw->rx_cfg.rx2_irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq_evt(
			csid_hw->rx_irq_controller[CAM_IFE_CSID_RX_IRQ_STATUS_REG0],
			csid_hw->rx_cfg.rx2_irq_handle);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe rx2 irq evt in rx for CSID:%u rc:%d",
				csid_hw->hw_intf->hw_idx, rc);

		csid_hw->rx_cfg.rx2_irq_handle = 0;
		cam_irq_controller_unregister_dependent(
			csid_hw->rx_irq_controller[CAM_IFE_CSID_RX_IRQ_STATUS_REG0],
			csid_hw->rx_irq_controller[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1]);
	}

	if (csid_hw->rx_cfg.top_irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq_evt(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->rx_cfg.top_irq_handle);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe rx irq evt in top for CSID:%u rc:%d",
				csid_hw->hw_intf->hw_idx, rc);

		csid_hw->rx_cfg.top_irq_handle = 0;
		cam_irq_controller_unregister_dependent(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->rx_irq_controller[CAM_IFE_CSID_RX_IRQ_STATUS_REG0]);
	}
}


static int cam_ife_csid_ver2_disable_csi2(
	bool maskout_irqs,
	struct cam_ife_csid_ver2_hw *csid_hw)
{
	CAM_DBG(CAM_ISP, "CSID:%u Disable csi2 rx",
		csid_hw->hw_intf->hw_idx);

	if (!csid_hw->flags.rx_enabled) {
		CAM_DBG(CAM_ISP, "CSID:%u Rx already disabled",
			csid_hw->hw_intf->hw_idx);
		return 0;
	}

	if (maskout_irqs)
		cam_ife_csid_ver2_maskout_rx_irqs(csid_hw);
	else
		cam_ife_csid_ver2_disable_rx_evts(csid_hw);

	csid_hw->flags.rx_enabled = false;

	return 0;
}

static int cam_ife_csid_ver2_top2_err_top_half(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	int32_t                                          rc = 0;
	struct cam_ife_csid_ver2_hw                     *csid_hw = NULL;
	struct cam_ife_csid_ver2_evt_payload            *evt_payload;

	csid_hw = th_payload->handler_priv;
	if (!csid_hw) {
		CAM_ERR_RATE_LIMIT(CAM_ISP, "No private returned");
		return -ENODEV;
	}

	rc  = cam_ife_csid_ver2_get_evt_payload(csid_hw, &evt_payload,
			&csid_hw->path_free_payload_list, &csid_hw->path_payload_lock);
	if (rc) {
		CAM_INFO_RATE_LIMIT(CAM_ISP, "CSID:%d TOP2 status: 0x%X",
			csid_hw->hw_intf->hw_idx,
			th_payload->evt_status_arr[0]);
	} else {
		evt_payload->irq_reg_val = th_payload->evt_status_arr[0];
		ktime_get_boottime_ts64(&evt_payload->timestamp);
		th_payload->evt_payload_priv = evt_payload;
	}
	return rc;
}

static inline int cam_ife_csid_ver2_rx_err_process_top_half(
	struct cam_ife_csid_ver2_hw        *csid_hw,
	uint32_t                           *evt_status_arr,
	enum cam_ife_csid_rx_irq_regs       rx_idx,
	uint32_t                           *status)
{
	int32_t                                               rc = 0;
	struct cam_ife_csid_ver2_reg_info                    *csid_reg;
	const struct cam_ife_csid_ver2_csi2_rx_reg_info      *csi2_reg;
	int                                                   discard_frame_count;
	const uint64_t                                       *evt_bitmap = NULL;
	const uint8_t                                        *bit_pos = NULL;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
				csid_hw->core_info->csid_reg;
	csi2_reg = csid_reg->csi2_reg;

	evt_bitmap = csid_reg->rx_debug_mask->evt_bitmap;
	bit_pos = csid_reg->rx_debug_mask->bit_pos;

	discard_frame_count = atomic_read(&csid_hw->discard_frame_per_path);
	if (discard_frame_count) {
		CAM_DBG(CAM_ISP, "ignoring rx error (error:0x%x, remaining frames:%d)",
			evt_status_arr[0], discard_frame_count);
		return -ENODEV;
	}

	if (csid_hw->flags.fatal_err_detected) {
		CAM_INFO_RATE_LIMIT(CAM_ISP,
			"CSID[%u] already handling fatal error",
			csid_hw->hw_intf->hw_idx);
		return -ENODEV;
	}

	*status = evt_status_arr[0];

	if (csid_hw->rx_cfg.epd_supported) {
		if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_CPHY_EOT_RECEPTION)) &&
			(*status & BIT(bit_pos[CAM_IFE_CSID_RX_CPHY_EOT_RECEPTION]))) {
			CAM_DBG(CAM_ISP, "CSID[%u] Rcvd Only ERROR_EOT for EPD sensor",
				csid_hw->hw_intf->hw_idx);
			return -ENODEV;
		}

		*status &= (~BIT(bit_pos[CAM_IFE_CSID_RX_CPHY_EOT_RECEPTION]));
	}

	if (*status & csi2_reg->fatal_err_mask[rx_idx]) {
		csid_hw->flags.fatal_err_detected = true;
		cam_ife_csid_ver2_stop_csi2_in_err(csid_hw);
		return rc;
	}

	if (*status & csi2_reg->part_fatal_err_mask[rx_idx]) {
		if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_CPHY_SOT_RECEPTION)) &&
			(*status & BIT(bit_pos[CAM_IFE_CSID_RX_CPHY_SOT_RECEPTION]))) {
			csid_hw->counters.error_irq_count++;
			CAM_DBG(CAM_ISP, "CSID[%u] Recoverable Error Count:%u",
				csid_hw->hw_intf->hw_idx,
				csid_hw->counters.error_irq_count);
		}

		if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_ERROR_CRC)) &&
			(*status & BIT(bit_pos[CAM_IFE_CSID_RX_ERROR_CRC]))) {
			csid_hw->counters.crc_error_irq_count++;
			if (csid_hw->counters.crc_error_irq_count > csid_hw->crc_error_threshold)
				CAM_DBG(CAM_ISP,
					"CSID[%u] Recoverable CRC Error Count: %u, CRC Error threshold: %u",
					csid_hw->hw_intf->hw_idx,
					csid_hw->counters.crc_error_irq_count,
					csid_hw->crc_error_threshold);
		}

		if ((csid_hw->counters.error_irq_count > CAM_IFE_CSID_MAX_ERR_COUNT) ||
			(csid_hw->counters.crc_error_irq_count > csid_hw->crc_error_threshold)) {
			csid_hw->flags.fatal_err_detected = true;
			cam_ife_csid_ver2_stop_csi2_in_err(csid_hw);
		}
	}

	return rc;
}

static int cam_ife_csid_ver2_rx_err_top_half(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	int32_t                                               rc = 0;
	uint32_t                                              status = 0;
	struct cam_ife_csid_ver2_hw                          *csid_hw = NULL;
	struct cam_ife_csid_ver2_evt_payload                 *evt_payload;

	csid_hw = th_payload->handler_priv;
	if (!csid_hw) {
		CAM_ERR_RATE_LIMIT(CAM_ISP, "No private returned");
		return -ENODEV;
	}
	rc = cam_ife_csid_ver2_rx_err_process_top_half(csid_hw, th_payload->evt_status_arr,
			CAM_IFE_CSID_RX_IRQ_STATUS_REG0, &status);
	if (rc) {
		CAM_ERR(CAM_ISP, "CSID:%d rx_idx:%d", csid_hw->hw_intf->hw_idx,
			CAM_IFE_CSID_RX_IRQ_STATUS_REG0);
		return rc;
	}

	rc  = cam_ife_csid_ver2_get_evt_payload(csid_hw, &evt_payload,
			&csid_hw->rx_free_payload_list,
			&csid_hw->rx_payload_lock);
	if (rc) {
		CAM_INFO_RATE_LIMIT(CAM_ISP, "CSID:%u status: 0x%X",
			csid_hw->hw_intf->hw_idx,
			th_payload->evt_status_arr[0]);
	} else {
		evt_payload->irq_reg_val = status;
		ktime_get_boottime_ts64(&evt_payload->timestamp);
		th_payload->evt_payload_priv = evt_payload;
	}
	return rc;
}

static int cam_ife_csid_ver2_rx2_err_top_half(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	int32_t                                               rc = 0;
	uint32_t                                              status = 0;
	struct cam_ife_csid_ver2_hw                          *csid_hw = NULL;
	struct cam_ife_csid_ver2_evt_payload                 *evt_payload;

	csid_hw = th_payload->handler_priv;
	if (!csid_hw) {
		CAM_ERR_RATE_LIMIT(CAM_ISP, "No private returned");
		return -ENODEV;
	}

	rc = cam_ife_csid_ver2_rx_err_process_top_half(csid_hw, th_payload->evt_status_arr,
			CAM_IFE_CSID_RX2_IRQ_STATUS_REG1, &status);
	if (rc) {
		CAM_ERR(CAM_ISP, "CSID:%d rx_idx:%d", csid_hw->hw_intf->hw_idx,
			CAM_IFE_CSID_RX2_IRQ_STATUS_REG1);
		return rc;
	}

	rc  = cam_ife_csid_ver2_get_evt_payload(csid_hw, &evt_payload,
			&csid_hw->rx_free_payload_list,
			&csid_hw->rx_payload_lock);
	if (rc) {
		CAM_INFO_RATE_LIMIT(CAM_ISP, "CSID:%d status: 0x%X",
			csid_hw->hw_intf->hw_idx,
			th_payload->evt_status_arr[0]);
	} else {
		evt_payload->irq_reg_val = status;
		ktime_get_boottime_ts64(&evt_payload->timestamp);
		th_payload->evt_payload_priv = evt_payload;
	}
	return rc;
}

static const char *__cam_ife_csid_ver2_parse_short_pkt_type(uint32_t data_type)
{
	switch (data_type) {
	case 0:
		return "Frame Start Code";
	case 1:
		return "Frame End Code";
	case 2:
		return "Line Start Code";
	case 3:
		return "Line End Code";
	case 4:
	case 5:
	case 6:
	case 7:
		return "Reserved";
	default:
		return "Invalid";
	}
}

static int cam_ife_csid_ver2_handle_rx_debug_event(
	struct cam_ife_csid_ver2_hw   *csid_hw,
	enum cam_ife_csid_rx_irq_regs  rx_idx,
	uint32_t                       bit_pos,
	uint32_t                      *rst_strobe_val)
{
	struct cam_hw_soc_info                           *soc_info;
	struct cam_ife_csid_ver2_reg_info                *csid_reg;
	const struct cam_ife_csid_ver2_csi2_rx_reg_info  *csi2_reg;
	uint32_t                                          mask, val, val2;
	const uint8_t                                    *dbg_bit_pos = NULL;
	const uint64_t                                   *evt_bitmap = NULL;
	struct timespec64                                 ts;


	csid_reg = (struct cam_ife_csid_ver2_reg_info *) csid_hw->core_info->csid_reg;
	csi2_reg = csid_reg->csi2_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	mask = BIT(bit_pos);
	dbg_bit_pos = csid_reg->rx_debug_mask->bit_pos;
	evt_bitmap = csid_reg->rx_debug_mask->evt_bitmap;

	ktime_get_boottime_ts64(&ts);

	if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_LONG_PKT_CAPTURED))
		&& (mask == BIT(dbg_bit_pos[CAM_IFE_CSID_RX_LONG_PKT_CAPTURED]))) {
		val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csi2_reg->captured_long_pkt_0_addr);
		val2 = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csi2_reg->captured_long_pkt_1_addr);
		CAM_INFO(CAM_ISP, "CSID[%u] LONG_PKT_CAPTURED occurred at [%llu: %09llu]",
			csid_hw->hw_intf->hw_idx, ts.tv_sec, ts.tv_nsec);
		CAM_INFO(CAM_ISP,
			"The header of the first long pkt matching the configured vc-dt has been captured");
		CAM_INFO(CAM_ISP,
			"Virtual Channel: %u, Data Type: %u, Word Count: %u, Error Correction Code: %u",
			((val & csi2_reg->vc_mask) >> csi2_reg->vc_shift),
			((val & csi2_reg->dt_mask) >> csi2_reg->dt_shift),
			((val & csi2_reg->wc_mask) >> csi2_reg->wc_shift), val2);

		/* Update reset long pkt strobe */
		*rst_strobe_val |= (1 << csi2_reg->long_pkt_strobe_rst_shift);
	} else if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_SHORT_PKT_CAPTURED)) &&
			(mask == BIT(dbg_bit_pos[CAM_IFE_CSID_RX_SHORT_PKT_CAPTURED]))) {
		val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csi2_reg->captured_short_pkt_0_addr);
		val2 = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csi2_reg->captured_short_pkt_1_addr);
		CAM_INFO(CAM_ISP, "CSID[%u] SHORT_PKT_CAPTURED occurred at [%llu: %09llu]",
			csid_hw->hw_intf->hw_idx, ts.tv_sec, ts.tv_nsec);
		CAM_INFO(CAM_ISP,
			"The header of the first short pkt matching the configured vc-dt has been captured");
		CAM_INFO(CAM_ISP,
			"Virtual Channel: %u, Data Type: %s, Frame Line Count: %u, Error Correction Code: %u",
			((val & csi2_reg->vc_mask) >> csi2_reg->vc_shift),
			__cam_ife_csid_ver2_parse_short_pkt_type(
			(val & csi2_reg->dt_mask) >> csi2_reg->dt_shift),
			((val & csi2_reg->wc_mask) >> csi2_reg->wc_shift), val2);

		/* Update reset short pkt strobe */
		*rst_strobe_val |= (1 << csi2_reg->short_pkt_strobe_rst_shift);
	} else if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_CPHY_PKT_HDR_CAPTURED)) &&
			(mask == BIT(dbg_bit_pos[CAM_IFE_CSID_RX_CPHY_PKT_HDR_CAPTURED]))) {
		val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csi2_reg->captured_cphy_pkt_hdr_addr);
		CAM_INFO(CAM_ISP, "CSID[%u] CPHY_PKT_HDR_CAPTURED occurred at [%llu: %09llu]",
			csid_hw->hw_intf->hw_idx, ts.tv_sec, ts.tv_nsec);
		CAM_INFO(CAM_ISP,
			"The header of the first cphy pkt matching the configured vc-dt has been captured");
		CAM_INFO(CAM_ISP,
			"Virtual Channel: %u, Data Type: %u, Word Count: %u",
			((val & csi2_reg->vc_mask) >> csi2_reg->vc_shift),
			((val & csi2_reg->dt_mask) >> csi2_reg->dt_shift),
			((val & csi2_reg->wc_mask) >> csi2_reg->wc_shift));

		/* Update reset phy pkt strobe */
		*rst_strobe_val |= (1 << csi2_reg->cphy_pkt_strobe_rst_shift);
	} else if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_UNMAPPED_VC_DT)) &&
			(mask == BIT(dbg_bit_pos[CAM_IFE_CSID_RX_UNMAPPED_VC_DT]))) {
		val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csi2_reg->cap_unmap_long_pkt_hdr_0_addr);

		CAM_ERR(CAM_ISP,
			"CSID[%u] UNMAPPED_VC_DT occurred at [%llu: %09llu]",
			csid_hw->hw_intf->hw_idx, ts.tv_sec, ts.tv_nsec);
		CAM_ERR(CAM_ISP,
			"Sensor: A long packet has a VC_DT combination that is not configured for IPP or RDIs");
		CAM_ERR(CAM_ISP,
			"Virtual Channel: %u, Data Type: %u, Word Count: %u",
			((val & csi2_reg->vc_mask) >> csi2_reg->vc_shift),
			((val & csi2_reg->dt_mask) >> csi2_reg->dt_shift),
			((val & csi2_reg->wc_mask) >> csi2_reg->wc_shift));
		CAM_ERR(CAM_ISP,
			"SW Programming: Populate or Disable the VC DT in sensor driver XML");

		csid_hw->counters.error_irq_count++;

		CAM_DBG(CAM_ISP, "CSID[%u] Recoverable Error Count:%u",
			csid_hw->hw_intf->hw_idx,
			csid_hw->counters.error_irq_count);

		/* Update reset unmapped long pkt strobe */
		*rst_strobe_val |= (1 << csi2_reg->unmapped_pkt_strobe_rst_shift);
	} else {
		CAM_DBG(CAM_ISP,
			"CSID[%u] RX_IRQ: %s",
			csid_hw->hw_intf->hw_idx,
			(*csid_reg->rx_irq_desc)[rx_idx][bit_pos].desc);
	}

	return 0;
}

static int cam_ife_csid_ver2_rx2_top_half(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	struct cam_ife_csid_ver2_hw                *csid_hw = NULL;
	const struct cam_ife_csid_ver2_csi2_rx_reg_info *csi2_reg;
	struct cam_ife_csid_ver2_reg_info          *csid_reg;
	uint32_t                                    irq_status;
	uint32_t                                    rst_strobe_val = 0;
	uint32_t                                    bit_pos = 0, bit_set = 0;

	csid_hw = th_payload->handler_priv;

	if (!csid_hw) {
		CAM_ERR_RATE_LIMIT(CAM_ISP, "No private returned");
		return -ENODEV;
	}

	irq_status = th_payload->evt_status_arr[0];

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	csi2_reg = csid_reg->csi2_reg;

	while (irq_status) {
		bit_set = irq_status & 1;
		if ((bit_set) && (BIT(bit_pos) &
			csid_hw->debug_info.rx_mask[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1]))
			cam_ife_csid_ver2_handle_rx_debug_event(csid_hw,
				CAM_IFE_CSID_RX2_IRQ_STATUS_REG1, bit_pos, &rst_strobe_val);
		bit_pos++;
		irq_status >>= 1;
	}

	/* Reset strobes for next set of pkts */
	if (rst_strobe_val && csid_hw->debug_info.rst_capture_strobes) {
		struct cam_hw_soc_info *soc_info = &csid_hw->hw_info->soc_info;

		cam_io_w_mb(rst_strobe_val, soc_info->reg_map[0].mem_base +
			csi2_reg->rst_strobes_addr);
	}
	return 0;
}

static int cam_ife_csid_ver2_rx_top_half(
	uint32_t                                   evt_id,
	struct cam_irq_th_payload                 *th_payload)
{
	struct cam_ife_csid_ver2_hw                *csid_hw = NULL;
	const struct cam_ife_csid_ver2_csi2_rx_reg_info *csi2_reg;
	struct cam_ife_csid_ver2_reg_info          *csid_reg;
	uint32_t                                    irq_status;
	uint32_t                                    rst_strobe_val = 0;
	uint32_t                                    bit_pos = 0, bit_set = 0;

	csid_hw = th_payload->handler_priv;

	if (!csid_hw) {
		CAM_ERR_RATE_LIMIT(CAM_ISP, "No private returned");
		return -ENODEV;
	}

	irq_status = th_payload->evt_status_arr[0];

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	csi2_reg = csid_reg->csi2_reg;

	while (irq_status) {
		bit_set = irq_status & 1;
		if ((bit_set) && (BIT(bit_pos) &
			csid_hw->debug_info.rx_mask[CAM_IFE_CSID_RX_IRQ_STATUS_REG0]))
			cam_ife_csid_ver2_handle_rx_debug_event(csid_hw,
				CAM_IFE_CSID_RX_IRQ_STATUS_REG0, bit_pos, &rst_strobe_val);
		bit_pos++;
		irq_status >>= 1;
	}

	/* Reset strobes for next set of pkts */
	if (rst_strobe_val && csid_hw->debug_info.rst_capture_strobes) {
		struct cam_hw_soc_info *soc_info = &csid_hw->hw_info->soc_info;

		cam_io_w_mb(rst_strobe_val, soc_info->reg_map[0].mem_base +
			csi2_reg->rst_strobes_addr);
	}
	return 0;
}

static inline uint32_t cam_ife_csid_ver2_input_core_to_hw_idx(int core_sel)
{
	switch (core_sel) {
	case CAM_IFE_CSID_INPUT_CORE_SEL_SFE_0: return 0;
	case CAM_IFE_CSID_INPUT_CORE_SEL_SFE_1: return 1;
	case CAM_IFE_CSID_INPUT_CORE_SEL_SFE_2: return 2;
	/**
	 * For all invalid cases, return a very large value
	 * that can never be a valid hw idx.
	 */
	default: return 0xFFFF;
	}
}

static void cam_ife_csid_ver2_read_debug_err_vectors(
	struct cam_ife_csid_ver2_hw  *csid_hw)
{
	int i, j, k;
	uint64_t timestamp;
	uint32_t temp, debug_vec_error_reg[CAM_IFE_CSID_DEBUG_VEC_ERR_REGS] = {0};
	uint8_t log_buf[CAM_IFE_CSID_LOG_BUF_LEN];
	size_t len = 0;
	struct cam_ife_csid_ver2_reg_info  *csid_reg;
	struct cam_hw_soc_info             *soc_info;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	if (!(csid_reg->cmn_reg->capabilities & CAM_IFE_CSID_CAP_DEBUG_ERR_VEC))
		return;

	for (i = 0; i < CAM_IFE_CSID_DEBUG_VEC_FIFO_SIZE; i++) {
		cam_io_w_mb((i << CAM_IFE_CSID_DEBUG_TIMESTAMP_IRQ_SEL_SHIFT),
			soc_info->reg_map[0].mem_base +
			csid_reg->cmn_reg->debug_err_vec_cfg);

		timestamp = __cam_ife_csid_ver2_get_time_stamp(
			soc_info->reg_map[0].mem_base,
			csid_reg->cmn_reg->debug_err_vec_ts_lb,
			csid_reg->cmn_reg->debug_err_vec_ts_mb,
			false, 0);

		if (!timestamp) {
			CAM_DBG(CAM_ISP, "Debug IRQ vectors already read, skip");
			return;
		}

		for (j = 0; j < CAM_IFE_CSID_DEBUG_VEC_ERR_REGS; j++) {
			if (csid_reg->cmn_reg->debug_err_vec_irq[j] == 0)
				break;

			temp = cam_io_r_mb(soc_info->reg_map[0].mem_base +
				csid_reg->cmn_reg->debug_err_vec_irq[j]);
			temp ^= debug_vec_error_reg[j];
			debug_vec_error_reg[j] |= temp;
			k = 0;

			while (temp) {
				if (temp & 0x1) {
					CAM_INFO_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN,
						&len, "%s ", (*csid_reg->debug_vec_desc)[j][k]);
				}
				temp >>= 1;
				k++;
			}
		}
		CAM_INFO(CAM_ISP, "Error(s) that occurred in time order %d at timestamp %lld: %s",
			i, timestamp, log_buf);
		memset(log_buf, 0x0, sizeof(uint8_t) * CAM_IFE_CSID_LOG_BUF_LEN);
	}

	cam_io_w_mb(0x1, soc_info->reg_map[0].mem_base +
		csid_reg->cmn_reg->debug_err_vec_cfg);
}

static int cam_ife_csid_ver2_handle_event_err(
	struct cam_ife_csid_ver2_hw  *csid_hw,
	uint32_t                      irq_status,
	uint32_t                      err_type,
	bool                          is_secondary,
	struct cam_isp_resource_node *res)
{
	struct cam_isp_hw_error_event_info   err_evt_info;
	struct cam_isp_hw_event_info         evt = {0};
	struct cam_ife_csid_ver2_path_cfg   *path_cfg;

	if (!csid_hw->event_cb) {
		CAM_ERR_RATE_LIMIT(CAM_ISP, "CSID[%u] event cb not registered",
			csid_hw->hw_intf->hw_idx);
		return 0;
	}

	/*
	 * If PF is encountered skip notifying error to ctx, PF
	 * handler will do the necessary notifications
	 */
	if (csid_hw->flags.pf_err_detected)
		return 0;

	evt.hw_idx   = csid_hw->hw_intf->hw_idx;
	evt.reg_val  = irq_status;
	evt.hw_type  = CAM_ISP_HW_TYPE_CSID;
	evt.is_secondary_evt = is_secondary;
	err_evt_info.err_type = err_type;
	evt.event_data = (void *)&err_evt_info;

#if defined(CONFIG_SAMSUNG_DEBUG_HW_INFO)
	cam_check_error_sensor_type(csid_hw->rx_cfg.phy_sel - 1);
#endif

	if (!is_secondary) {
		if (res) {
			cam_ife_csid_ver2_print_debug_reg_status(csid_hw, res);
			path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
			evt.res_id   = res->res_id;
			CAM_ERR(CAM_ISP,
				"csid[%u] Res:%s Err 0x%x status 0x%x time_stamp: %lld:%09lld",
				csid_hw->hw_intf->hw_idx, res->res_name, err_type,
				irq_status, path_cfg->error_ts.tv_sec,
				path_cfg->error_ts.tv_nsec);
		} else {
			CAM_ERR(CAM_ISP,
				"csid[%u] Rx Err: 0x%x status 0x%x",
				csid_hw->hw_intf->hw_idx, err_type, irq_status);
		}
	}

	evt.in_core_idx =
		cam_ife_csid_ver2_input_core_to_hw_idx(csid_hw->top_cfg.input_core_type);

	cam_ife_csid_ver2_print_camif_timestamps(csid_hw);

	cam_ife_csid_ver2_read_debug_err_vectors(csid_hw);

	csid_hw->event_cb(csid_hw->token, CAM_ISP_HW_EVENT_ERROR, (void *)&evt);

	return 0;
}

static int cam_ife_csid_ver2_rx_err_process_bottom_half(
	struct cam_ife_csid_ver2_hw          *csid_hw,
	struct cam_ife_csid_ver2_evt_payload *payload,
	enum cam_ife_csid_rx_irq_regs         rx_idx)
{
	const struct cam_ife_csid_ver2_csi2_rx_reg_info *csi2_reg = NULL;
	struct cam_ife_csid_ver2_reg_info               *csid_reg = NULL;
	uint32_t                                         irq_status = 0;
	struct cam_hw_soc_info                          *soc_info = NULL;
	uint8_t                                         *log_buf = NULL;
	uint32_t                                         rx_irq_status = 0;
	int                                              data_idx = 0;
	uint32_t                                         total_crc;
	uint32_t                                         long_pkt_ftr_val;
	uint32_t                                         event_type = 0;
	size_t                                           len = 0, i;
	uint32_t                                         val = 0;
	const uint64_t                                  *evt_bitmap = NULL;
	const uint8_t                                   *bit_pos = NULL;
	uint32_t                                         irq_reg_val = payload->irq_reg_val;

	soc_info = &csid_hw->hw_info->soc_info;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	csi2_reg = csid_reg->csi2_reg;

	evt_bitmap = csid_reg->rx_debug_mask->evt_bitmap;

	bit_pos = csid_reg->rx_debug_mask->bit_pos;

	irq_status = irq_reg_val & csi2_reg->fatal_err_mask[rx_idx];

	log_buf = csid_hw->log_buf;
	log_buf[0] = '\0';

	if (!csid_hw->flags.device_enabled) {
		CAM_DBG(CAM_ISP, "CSID[%u] bottom-half after stop [0x%x]",
			csid_hw->hw_intf->hw_idx, irq_status);
		return 0;
	}

	spin_lock(&csid_hw->lock_state);
	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID[%u] powered down state",
			csid_hw->hw_intf->hw_idx);
		goto unlock;
	}

	if (irq_status) {
		bool lane_overflow = false;
		bool lane_sot_lost = false;
		bool lane_eot_lost = false;
		char tmp_buf[10];
		int tmp_len = 0;

		for (i = 0; i < 4; i++) {
			/* NOTE: Hardware specific bits */
			if ((evt_bitmap[rx_idx] &
				(BIT_ULL(CAM_IFE_CSID_RX_LANE0_FIFO_OVERFLOW)<<i)) && (irq_status &
				(BIT(bit_pos[CAM_IFE_CSID_RX_LANE0_FIFO_OVERFLOW]) << i))) {
				tmp_len += scnprintf(tmp_buf + tmp_len, 10 - tmp_len, " %zu", i);
				lane_overflow = true;
			}
		}

		if (lane_overflow) {
			event_type |= CAM_ISP_HW_ERROR_CSID_LANE_FIFO_OVERFLOW;
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"DLn_FIFO_OVERFLOW, Lanes:%s, occurred at [%llu: %09llu]",
				tmp_buf, payload->timestamp.tv_sec, payload->timestamp.tv_nsec);
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"System: Data has been lost when transferring from PHY to CSID");
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Debug: Check PHY clock, CSID clock and possible skew among the data lanes");
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"DPHY: Enable skew calibration for datarate > 1.5Gbps/lane");
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"CPHY: Perform CDR tuning");
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Current CSID clock rate: %lluHz", csid_hw->clk_rate);
			CAM_ERR(CAM_ISP, "CSID[%u] Fatal Errors: %s",
				csid_hw->hw_intf->hw_idx, log_buf);
		}

		tmp_len = 0;
		len = 0;
		for (i = 0; i < 4; i++) {
			/* NOTE: Hardware specific bits */
			if ((evt_bitmap[rx_idx] &
				(BIT_ULL(CAM_IFE_CSID_RX_DL0_SOT_LOST)<<i)) && (irq_status &
				(BIT(bit_pos[CAM_IFE_CSID_RX_DL0_SOT_LOST]) << i))) {
				tmp_len += scnprintf(tmp_buf + tmp_len, 10 - tmp_len, " %zu", i);
				lane_sot_lost = true;
			}
		}

		if (lane_sot_lost) {
			event_type |= CAM_ISP_HW_ERROR_CSID_MISSING_SOT;
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"DLn_SOT_LOST, Lanes:%s, occurred at [%llu: %09llu]",
				tmp_buf, payload->timestamp.tv_sec, payload->timestamp.tv_nsec);
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Sensor: Timing signals are missed received in the CPHY packet");
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Debug: Check PHY/sensor config");
			CAM_ERR(CAM_ISP, "CSID[%u] Fatal Errors: %s",
				csid_hw->hw_intf->hw_idx, log_buf);
		}

		tmp_len = 0;
		len = 0;
		for (i = 0; i < 4; i++) {
			/* NOTE: Hardware specific bits */
			if ((evt_bitmap[rx_idx] &
				(BIT_ULL(CAM_IFE_CSID_RX_DL0_EOT_LOST)<<i)) && (irq_status &
				(BIT(bit_pos[CAM_IFE_CSID_RX_DL0_EOT_LOST]) << i))) {
				tmp_len += scnprintf(tmp_buf + tmp_len, 10 - tmp_len, " %zu", i);
				lane_eot_lost = true;
			}
		}

		if (lane_eot_lost) {
			event_type |= CAM_ISP_HW_ERROR_CSID_MISSING_EOT;
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"DLn_EOT_LOST, Lanes:%s, occurred at [%llu: %09llu]",
				tmp_buf, payload->timestamp.tv_sec, payload->timestamp.tv_nsec);
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Sensor: Timing signals are missed received in the CPHY packet");
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Debug: Check PHY/sensor config");
			CAM_ERR(CAM_ISP, "CSID[%u] Fatal Errors: %s",
				csid_hw->hw_intf->hw_idx, log_buf);
		}

		len = 0;
		if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_ERROR_CPHY_PH_CRC)) &&
			(irq_status & BIT(bit_pos[CAM_IFE_CSID_RX_ERROR_CPHY_PH_CRC]))) {
			event_type |= CAM_ISP_HW_ERROR_CSID_PKT_HDR_CORRUPTED;
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"CPHY_PACKET_HEADER_CRC occurred at [%llu: %09llu]",
				payload->timestamp.tv_sec, payload->timestamp.tv_nsec);
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Sensor: All CPHY packet headers received are corrupted");
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Debug: Check sensor data rate in output pixel clock in XML for proper PHY configuration");
			CAM_ERR(CAM_ISP, "CSID[%u] Fatal Errors: %s",
				csid_hw->hw_intf->hw_idx, log_buf);
		}

		len = 0;
		if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_STREAM_UNDERFLOW)) &&
			(irq_status & BIT(bit_pos[CAM_IFE_CSID_RX_STREAM_UNDERFLOW]))) {
			event_type |= CAM_ISP_HW_ERROR_CSID_MISSING_PKT_HDR_DATA;
			val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
				csi2_reg->captured_long_pkt_0_addr);

			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"STREAM_UNDERFLOW occurred at [%llu: %09llu]",
				payload->timestamp.tv_sec, payload->timestamp.tv_nsec);
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Sensor: Long packet payload size is less than payload header size, resulting a corrupted frame");
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Debug: Perform PHY Tuning/Check sensor limitations");
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Word Count: %u", val & 0xFFFF);
			CAM_ERR(CAM_ISP, "CSID[%u] Fatal Errors: %s",
				csid_hw->hw_intf->hw_idx, log_buf);
		}

		len = 0;
		if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_ERROR_ECC)) && (irq_status &
			BIT(bit_pos[CAM_IFE_CSID_RX_ERROR_ECC]))) {
			event_type |= CAM_ISP_HW_ERROR_CSID_PKT_HDR_CORRUPTED;
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"DPHY_PACKET_HEADER_ECC_DETECTED occurred on [%llu: %09llu]",
				payload->timestamp.tv_sec, payload->timestamp.tv_nsec);
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Sensor: A short or long packet is corrupted and cannot be recovered");
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Debug: Check sensor datarate in output pixel clock in XML for proper PHY configuration");
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Error Correction Code: 0x%x",
					cam_io_r_mb(soc_info->reg_map[0].mem_base +
					csi2_reg->captured_long_pkt_1_addr));
			CAM_ERR(CAM_ISP, "CSID[%u] Fatal Errors: %s",
				csid_hw->hw_intf->hw_idx, log_buf);
		}

		len = 0;
		if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_UNBOUNDED_FRAME)) &&
			(irq_status & BIT(bit_pos[CAM_IFE_CSID_RX_UNBOUNDED_FRAME]))) {
			event_type |= CAM_ISP_HW_ERROR_CSID_UNBOUNDED_FRAME;
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"UNBOUNDED_FRAME: Frame started with EOF or No EOF");
			CAM_ERR(CAM_ISP, "CSID[%u] Fatal Errors: %s",
				csid_hw->hw_intf->hw_idx, log_buf);
		}

		len = 0;
		if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_CPHY_EOT_RECEPTION)) &&
			(irq_status & (BIT(bit_pos[CAM_IFE_CSID_RX_CPHY_EOT_RECEPTION])))) {
			event_type |= CAM_ISP_HW_ERROR_CSID_MISSING_EOT;
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"CPHY_EOT_RECEPTION: No EOT on lane/s, is_EPD: %c, PHY_Type: %s(%u)",
				(csid_hw->rx_cfg.epd_supported & CAM_ISP_EPD_SUPPORT) ? 'Y' : 'N',
				(csid_hw->rx_cfg.lane_type) ? "cphy" : "dphy",
				csid_hw->rx_cfg.lane_type);
			CAM_ERR(CAM_ISP, "CSID[%u] Fatal Errors: %s",
				csid_hw->hw_intf->hw_idx, log_buf);
		}

		/**
		 * Starting from v880, CRC errors wil be treated as part fatal errors and
		 * only be flagged after happening several times. Leave this for backward
		 * compatibility.
		 */
		len = 0;
		if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_ERROR_CRC)) && (irq_status &
			BIT(bit_pos[CAM_IFE_CSID_RX_ERROR_CRC]))) {
			event_type |= CAM_ISP_HW_ERROR_CSID_PKT_PAYLOAD_CORRUPTED;
			long_pkt_ftr_val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
				csi2_reg->captured_long_pkt_ftr_addr);
			total_crc = cam_io_r_mb(soc_info->reg_map[0].mem_base +
				csi2_reg->total_crc_err_addr);

			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"LONG_PACKET_PAYLOAD_CRC occurred at [%llu: %09llu]",
				payload->timestamp.tv_sec, payload->timestamp.tv_nsec);
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Sensor: The calculated CRC of a long packet does not match the transmitted (expected) CRC, possible corruption");
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Debug: First frame CRC: Check sensor data rate / settle count in XML for proper PHY configuration; Streaming state: Check sensor constraints for exposure control else perform PHY CDR-EQ Tuning");

			if (csid_hw->rx_cfg.lane_type == CAM_ISP_LANE_TYPE_CPHY) {
				val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
					csi2_reg->captured_cphy_pkt_hdr_addr);

				CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
					"Total CRC Errors: %u, Virtual Channel: %u, Data Type: %u, Word Count: %u",
					total_crc, val >> 22, (val >> 16) & 0x3F, val & 0xFFFF);
			} else {
				CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
					"Total CRC Errors: %u", total_crc);
			}
			CAM_ERR(CAM_ISP, "CSID[%u] Fatal Errors: %s",
				csid_hw->hw_intf->hw_idx, log_buf);
		}

		rx_irq_status |= irq_status;
		csid_hw->flags.fatal_err_detected = true;

		cam_ife_csid_ver2_send_cdr_sweep_csi2_rx_vals(csid_hw, csid_reg, irq_reg_val);
	}

	irq_status = irq_reg_val &
			csi2_reg->part_fatal_err_mask[rx_idx];

	if (irq_status) {
		len = 0;
		if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_CPHY_SOT_RECEPTION)) &&
			(irq_status & BIT(bit_pos[CAM_IFE_CSID_RX_CPHY_SOT_RECEPTION]))) {
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"CPHY_SOT_RECEPTION: Less SOTs on lane/s");
			CAM_ERR(CAM_ISP, "CSID[%u] Partly fatal errors: %s",
				csid_hw->hw_intf->hw_idx, log_buf);
		}

		len = 0;
		if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_ERROR_CRC)) &&
			(irq_status & BIT(bit_pos[CAM_IFE_CSID_RX_ERROR_CRC]))) {
			event_type |= CAM_ISP_HW_ERROR_CSID_PKT_PAYLOAD_CORRUPTED;

			/* Only print the CRC error logs when reaching the threshold */
			if (csid_hw->counters.crc_error_irq_count > csid_hw->crc_error_threshold) {
				long_pkt_ftr_val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
					csi2_reg->captured_long_pkt_ftr_addr);
				total_crc = cam_io_r_mb(soc_info->reg_map[0].mem_base +
					csi2_reg->total_crc_err_addr);
				CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
					"LONG_PACKET_PAYLOAD_CRC occurred at [%llu: %09llu]",
					payload->timestamp.tv_sec, payload->timestamp.tv_nsec);
				CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
					"Sensor: The calculated CRC of a long packet does not match the transmitted (expected) CRC, possible corruption");
				CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
					"Debug: First frame CRC: Check sensor data rate / settle count in XML for proper PHY configuration; Streaming state: Check sensor constraints for exposure control else perform PHY CDR-EQ Tuning");

				if (csid_hw->rx_cfg.lane_type == CAM_ISP_LANE_TYPE_CPHY) {
					val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
						csi2_reg->captured_cphy_pkt_hdr_addr);

					CAM_ERR_BUF(CAM_ISP, log_buf,
						CAM_IFE_CSID_LOG_BUF_LEN, &len,
						"Total CRC Errors: %u, Virtual Channel: %u, Data Type: %u, Word Count: %u",
						total_crc, val >> 22, (val >> 16) & 0x3F,
						val & 0xFFFF);
				} else {
					CAM_ERR_BUF(CAM_ISP, log_buf,
						CAM_IFE_CSID_LOG_BUF_LEN, &len,
						"Total CRC Errors: %u", total_crc);
				}

				CAM_ERR(CAM_ISP, "CSID[%u] Partly fatal errors: %s",
					csid_hw->hw_intf->hw_idx, log_buf);
			}
		}

		rx_irq_status |= irq_status;

		cam_ife_csid_ver2_send_cdr_sweep_csi2_rx_vals(csid_hw, csid_reg, irq_reg_val);
	}

	irq_status = irq_reg_val &
			csi2_reg->non_fatal_err_mask[rx_idx];

	if (irq_status) {
		len = 0;
		if ((evt_bitmap[rx_idx] & BIT_ULL(CAM_IFE_CSID_RX_MMAPPED_VC_DT)) && (irq_status &
			BIT(bit_pos[CAM_IFE_CSID_RX_MMAPPED_VC_DT]))) {
			val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
				csi2_reg->captured_long_pkt_0_addr);

			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"MMAPPED_VC_DT occurred at [%llu: %09llu]",
				payload->timestamp.tv_sec, payload->timestamp.tv_nsec);
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"SW: A long packet has a VC_DT combination that is configured for more than one IPP or RDIs");
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"Virtual Channel: %u, Data Type: %u",
				(val >> 22), ((val >> 16) & 0x3F));
		}

		CAM_ERR(CAM_ISP, "CSID[%u] Non-fatal-errors: %s",
			csid_hw->hw_intf->hw_idx, log_buf);
	}

	CAM_ERR_RATE_LIMIT(CAM_ISP, "CSID[%u] Rx Status 0x%x",
		csid_hw->hw_intf->hw_idx,
		irq_reg_val);

	data_idx = (int)(csid_hw->rx_cfg.phy_sel - csid_reg->cmn_reg->phy_sel_base_idx);
	if ((!csid_hw->flags.reset_awaited) && csid_hw->flags.fatal_err_detected) {
		if (!event_type)
			event_type |= CAM_ISP_HW_ERROR_CSID_FATAL;

#if IS_ENABLED(CONFIG_SEC_ABC)
		cam_abc_send_event_mipi_error(csid_hw->rx_cfg.phy_sel - 1);
#if defined(CONFIG_SAMSUNG_DEBUG_HW_INFO)
		cam_check_error_sensor_type(csid_hw->rx_cfg.phy_sel - 1);
#endif
#endif

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
 		hw_bigdata_mipi_from_ife_csid_ver2(csid_hw->rx_cfg.phy_sel - 1);
#endif

#if defined(CONFIG_CAMERA_CDR_TEST)
 		cam_clock_data_recovery_set_result(cam_clock_data_get_camera_type(csid_hw->rx_cfg.phy_sel - 1), CDR_ERROR_MIPI);
#endif

		if (data_idx < 0)
			CAM_WARN(CAM_ISP, "Can't notify csiphy, incorrect phy selected=%d",
				data_idx);
		else
			cam_subdev_notify_message(CAM_CSIPHY_DEVICE_TYPE,
				CAM_SUBDEV_MESSAGE_APPLY_CSIPHY_AUX, (void *)&data_idx);

		cam_ife_csid_ver2_handle_event_err(csid_hw,
			rx_irq_status, event_type, false, NULL);
		csid_hw->flags.reset_awaited = true;
	}
unlock:
	spin_unlock(&csid_hw->lock_state);

	return 0;
}

static int cam_ife_csid_ver2_rx_err_bottom_half(
	void                                      *handler_priv,
	void                                      *evt_payload_priv)
{
	struct cam_ife_csid_ver2_evt_payload       *payload;
	struct cam_ife_csid_ver2_hw                *csid_hw = NULL;
	int                                         rc = 0;

	if (!handler_priv || !evt_payload_priv) {
		CAM_ERR(CAM_ISP, "Invalid params");
		return -EINVAL;
	}

	payload = evt_payload_priv;
	csid_hw = handler_priv;

	rc = cam_ife_csid_ver2_rx_err_process_bottom_half(csid_hw,
		payload, CAM_IFE_CSID_RX_IRQ_STATUS_REG0);

	cam_ife_csid_ver2_put_evt_payload(csid_hw, &payload,
		&csid_hw->rx_free_payload_list,
		&csid_hw->rx_payload_lock);

	return 0;
}

static int cam_ife_csid_ver2_rx2_err_bottom_half(
	void                                      *handler_priv,
	void                                      *evt_payload_priv)
{
	struct cam_ife_csid_ver2_evt_payload       *payload;
	struct cam_ife_csid_ver2_hw                *csid_hw = NULL;
	int                                         rc = 0;

	if (!handler_priv || !evt_payload_priv) {
		CAM_ERR(CAM_ISP, "Invalid params");
		return -EINVAL;
	}

	payload = evt_payload_priv;
	csid_hw = handler_priv;

	rc = cam_ife_csid_ver2_rx_err_process_bottom_half(csid_hw,
		payload, CAM_IFE_CSID_RX2_IRQ_STATUS_REG1);

	cam_ife_csid_ver2_put_evt_payload(csid_hw, &payload, &csid_hw->rx_free_payload_list,
		&csid_hw->rx_payload_lock);

	return rc;
}

void cam_ife_csid_hw_ver2_rdi_line_buffer_conflict_handler(
	void *csid)
{
	struct cam_ife_csid_ver2_hw       *csid_hw  = csid;
	struct cam_ife_csid_ver2_reg_info *csid_reg = csid_hw->core_info->csid_reg;
	struct cam_hw_soc_info            *soc_info = &csid_hw->hw_info->soc_info;
	void __iomem                      *base =
		soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg;
	uint32_t i = 0, rdi_cfg = 0;
	uint8_t *log_buf = csid_hw->log_buf;
	size_t len = 0;

	memset(log_buf, 0x0, sizeof(uint8_t) * CAM_IFE_CSID_LOG_BUF_LEN);

	for (i = CAM_IFE_PIX_PATH_RES_RDI_0; i < CAM_IFE_PIX_PATH_RES_RDI_4;
		i++) {
		path_reg = csid_reg->path_reg[i - CAM_IFE_PIX_PATH_RES_RDI_0];

		if (!(path_reg->capabilities &
			CAM_IFE_CSID_CAP_LINE_SMOOTHING_IN_RDI))
			continue;

		rdi_cfg = cam_io_r_mb(base + path_reg->cfg1_addr);

		if (rdi_cfg & path_reg->pix_store_en_shift_val)
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"LINE BUFFER ENABLED for RDI%d", (i - CAM_IFE_PIX_PATH_RES_RDI_0));
	}

	if (len)
		CAM_ERR(CAM_ISP, "CSID[%u] %s", csid_hw->hw_intf->hw_idx, log_buf);

}

void cam_ife_csid_hw_ver2_drv_err_handler(void *csid)
{
	struct cam_ife_csid_ver2_hw       *csid_hw  = csid;
	struct cam_ife_csid_ver2_reg_info *csid_reg = csid_hw->core_info->csid_reg;
	struct cam_hw_soc_info            *soc_info = &csid_hw->hw_info->soc_info;
	void __iomem                      *base =
		soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;
	uint32_t cfg0_val = 0, cfg1_val = 0, cfg2_val = 0, debug_drv_0_val = 0, debug_drv_1_val = 0;

	cfg0_val = cam_io_r_mb(base + csid_reg->cmn_reg->drv_cfg0_addr);
	cfg1_val = cam_io_r_mb(base + csid_reg->cmn_reg->drv_cfg1_addr);
	cfg2_val = cam_io_r_mb(base + csid_reg->cmn_reg->drv_cfg2_addr);
	debug_drv_0_val = cam_io_r_mb(base + csid_reg->cmn_reg->debug_drv_0_addr);
	debug_drv_1_val = cam_io_r_mb(base + csid_reg->cmn_reg->debug_drv_1_addr);

	CAM_INFO(CAM_ISP,
		"CSID[%u] DRV cfg0:0x%x cfg1:0x%x cfg2:0x%x qtimer_val [start:end] [0x%x : 0x%x]",
		csid_hw->hw_intf->hw_idx, cfg0_val, cfg1_val, cfg2_val, debug_drv_0_val,
		debug_drv_1_val);
}

void cam_ife_csid_hw_ver2_mup_mismatch_handler(
	void *csid, void *resource)
{
	int                                i;
	uint32_t                           idx = 0;
	struct timespec64                  current_ts;
	struct cam_ife_csid_ver2_hw       *csid_hw = csid;
	struct cam_isp_resource_node      *res = resource;
	struct cam_ife_csid_ver2_path_cfg *path_cfg =
		(struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
	struct cam_ife_csid_cid_data      *cid_data = &csid_hw->cid_data[path_cfg->cid];
	struct cam_hw_soc_info            *soc_info = &csid_hw->hw_info->soc_info;
	struct cam_ife_csid_ver2_reg_info *csid_reg = NULL;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = NULL;

	CAM_INFO(CAM_ISP, "CSID[%u] Last MUP value 0x%x programmed for res [id: %d name: %s]",
		csid_hw->hw_intf->hw_idx, csid_hw->rx_cfg.mup, res->res_id, res->res_name);

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	path_reg = csid_reg->path_reg[res->res_id];

	if (cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_1].valid) {
		for (i = 0; i < CAM_IFE_CSID_MULTI_VC_DT_GRP_MAX; i++) {
			if (!cid_data->vc_dt[i].valid)
				break;
			CAM_INFO(CAM_ISP, "CSID[%u] Virtual channel %d: %u, DT 0x%x",
				csid_hw->hw_intf->hw_idx, i, cid_data->vc_dt[i].vc,
				cid_data->vc_dt[i].dt);
		}

		if (path_cfg->ts_comb_vcdt_en) {
			ktime_get_boottime_ts64(&current_ts);

			idx = cam_io_r_mb(soc_info->reg_map[0].mem_base +
				path_reg->timestamp_curr0_sof_addr) &
				csid_reg->cmn_reg->ts_comb_vcdt_mask;

			if (idx < CAM_IFE_CSID_MULTI_VC_DT_GRP_MAX)
				CAM_DBG(CAM_ISP,
					"CSID[%d] Received frame on VC: %d [id: %d name: %s] timestamp: [%llu:%09llu]",
					csid_hw->hw_intf->hw_idx, idx,
					res->res_id, res->res_name, current_ts.tv_sec,
					current_ts.tv_nsec);
			else
				CAM_ERR(CAM_ISP,
					"CSID[%d] Get invalid virtual channel index: %d on [id: %d name: %s] timestamp: [%llu:%09llu]",
					csid_hw->hw_intf->hw_idx, idx, res->res_id,
					res->res_name, current_ts.tv_sec, current_ts.tv_nsec);
		}
	} else {
		CAM_ERR(CAM_ISP, "CSID[%u] Multi-VCDT is not enabled, virtual channel 0: %d",
			csid_hw->hw_intf->hw_idx,
			cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_0].vc);
	}
}

void cam_ife_csid_ver2_print_illegal_programming_irq_status(
	void *csid, void *resource)
{
	struct cam_ife_csid_ver2_hw                        *csid_hw = csid;
	struct cam_isp_resource_node                       *res = resource;
	struct cam_ife_csid_ver2_reg_info                  *csid_reg;
	struct cam_ife_csid_ver2_path_cfg                  *path_cfg;
	struct cam_ife_csid_cid_data                       *cid_data;
	struct cam_hw_soc_info                             *soc_info;
	void __iomem                                       *base;
	const struct cam_ife_csid_ver2_path_reg_info       *path_reg;
	uint32_t vcdt_cfg0 = 0, vcdt_cfg1 = 0, cfg0 = 0, cfg1 = 0;
	uint32_t decode_fmt[CAM_IFE_CSID_MULTI_VC_DT_GRP_MAX],
		vc[CAM_IFE_CSID_MAX_VALID_VC_NUM], dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_MAX];
	int i;
	size_t len = 0;
	uint8_t log_buf[100];

	csid_reg = csid_hw->core_info->csid_reg;
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
	if (!path_cfg || (path_cfg->cid >= CAM_IFE_CSID_CID_MAX)) {
		CAM_ERR(CAM_ISP, "CSID:%u Invalid params: path_cfg: %pK, num_cids: %d",
			 csid_hw->hw_intf->hw_idx, path_cfg, (path_cfg ? (path_cfg->cid) : -1));
		return;
	}

	cid_data = &csid_hw->cid_data[path_cfg->cid];
	soc_info = &csid_hw->hw_info->soc_info;
	base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;
	path_reg = csid_reg->path_reg[res->res_id];

	cfg0 = cam_io_r_mb(base + path_reg->cfg0_addr);
	cfg1 = cam_io_r_mb(base + path_reg->cfg1_addr);
	vcdt_cfg0 = cam_io_r_mb(base + path_reg->multi_vcdt_cfg0_addr);

	CAM_INFO(CAM_ISP, "cfg0 = %x cfg1 = %x vcdt_cfg0 = %x vcrop %x early eof %d",
		cfg0, cfg1, vcdt_cfg0, cam_io_r_mb(base + path_reg->vcrop_addr),
		(cfg1 & BIT(path_reg->early_eof_en_shift_val)));


	if (cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_1].valid) {
		decode_fmt[0] = ((cfg0 >>
			csid_reg->cmn_reg->decode_format_shift_val) &
			csid_reg->cmn_reg->decode_format_mask);
		decode_fmt[1] = ((vcdt_cfg0 >>
			csid_reg->cmn_reg->decode_format1_shift_val) &
			csid_reg->cmn_reg->decode_format_mask);
		vc[0] = ((cfg0 >> csid_reg->cmn_reg->vc_shift_val) &
			csid_reg->cmn_reg->vc_mask);
		dt[0] = ((cfg0 >> csid_reg->cmn_reg->dt_shift_val) &
			csid_reg->cmn_reg->dt_mask);
		vc[1] = ((vcdt_cfg0 >> csid_reg->cmn_reg->multi_vcdt_vc1_shift_val) &
			csid_reg->cmn_reg->vc_mask);
		dt[1] = ((vcdt_cfg0 >> csid_reg->cmn_reg->multi_vcdt_dt1_shift_val) &
			csid_reg->cmn_reg->dt_mask);

		if (cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_2].valid) {
			vcdt_cfg1 = cam_io_r_mb(base + path_reg->multi_vcdt_cfg1_addr);
			dt[2] = ((vcdt_cfg1 >> csid_reg->cmn_reg->multi_vcdt_dt2_shift_val) &
				csid_reg->cmn_reg->dt_mask);
			dt[3] = ((vcdt_cfg1 >> csid_reg->cmn_reg->multi_vcdt_dt3_shift_val) &
				csid_reg->cmn_reg->dt_mask);
			decode_fmt[2] =
				((vcdt_cfg1 >> csid_reg->cmn_reg->decode_format2_shift_val) &
				csid_reg->cmn_reg->decode_format_mask);
			decode_fmt[3] =
				((vcdt_cfg1 >> csid_reg->cmn_reg->decode_format3_shift_val) &
				csid_reg->cmn_reg->decode_format_mask);
		}

		for (i = 0; i < CAM_ISP_VC_DT_CFG; i++) {
			if (!cid_data->vc_dt[i].valid)
				break;
			if (i < CAM_IFE_CSID_MAX_VALID_VC_NUM)
				CAM_INFO_BUF(CAM_ISP, log_buf, 100, &len, "VC%d: 0x%x ", i, vc[i]);

			CAM_INFO_BUF(CAM_ISP, log_buf, 100, &len, "DT%d: 0x%x, decode_fmt: 0x%x",
				i, dt[i], decode_fmt[i]);
		}

		CAM_INFO(CAM_ISP, "%s", log_buf);

		cam_ife_csid_ver2_decode_format_validate_on_err(csid_hw, vc, dt,
			decode_fmt, cid_data->num_vc_dt);
	}

	if (!(csid_hw->debug_info.debug_val &
		CAM_IFE_CSID_DEBUG_DISABLE_EARLY_EOF) &&
		csid_reg->cmn_reg->early_eof_supported) {
		if ((cfg1 & BIT(path_reg->early_eof_en_shift_val)) &&
			!(cfg1 & BIT(path_reg->crop_v_en_shift_val))) {
			CAM_ERR(CAM_ISP,
				"CSID:%u Early EOF %d enabled without VCROP %d",
				csid_hw->hw_intf->hw_idx,
				cfg1 & path_reg->early_eof_en_shift_val,
				cfg1 & path_reg->crop_v_en_shift_val);

		}
	}

	CAM_INFO(CAM_ISP, "CSID:%u Illegal Programming for res [id: %d name: %s]",
		csid_hw->hw_intf->hw_idx, res->res_id, res->res_name);
}

static void cam_ife_csid_ver2_print_debug_reg_status(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_isp_resource_node    *res)
{
	const struct cam_ife_csid_ver2_reg_info *csid_reg;
	struct cam_hw_soc_info                   *soc_info;
	void __iomem *mem_base;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = NULL;
	uint32_t val0, val1, val2, val3 = 0;

	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
		csid_hw->core_info->csid_reg;

	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	path_reg = csid_reg->path_reg[res->res_id];

	val0 = cam_io_r_mb(mem_base +
		path_reg->debug_camif_0_addr);
	val1 = cam_io_r_mb(mem_base +
		path_reg->debug_camif_1_addr);
	val2 = cam_io_r_mb(mem_base +
		path_reg->debug_halt_status_addr);

	/* Read test bus if enabled */
	if (csid_hw->debug_info.test_bus_enabled)
		val3 = cam_io_r_mb(mem_base +
			csid_reg->cmn_reg->test_bus_debug);

	CAM_INFO(CAM_ISP,
		"CSID:%u debug_camif_0: 0x%x debug_camif_1: 0x%x halt_status: 0x%x test_bus: %s test_bus_val: 0x%x for res: %s ",
		csid_hw->hw_intf->hw_idx, val0, val1, val2,
		CAM_BOOL_TO_YESNO(csid_hw->debug_info.test_bus_enabled),
		val3, res->res_name);
}

#if defined(CONFIG_SAMSUNG_CAMERA)
#include <linux/ktime.h>
static ktime_t sof_ts, sof_curr_ts,	eof_ts;

static void cam_ife_csid_ver2_print_debug_sensor_timing(
	struct cam_ife_csid_ver2_hw* csid_hw,
	uint32_t port_index,
	uint32_t irq_status)
{
	uint32_t hw_idx	= (csid_hw->debug_info.dbg_fps >> 4) & 0xF;
	uint32_t port_id = csid_hw->debug_info.dbg_fps & 0xF;

	if (csid_hw->debug_info.dbg_fps	== 0)
		return;

	if ((csid_hw->hw_intf->hw_idx == hw_idx) &&	// csid	idx
		(port_index	== port_id)) //	RDI	0 (2), IPP(7), ref:	CAM_IFE_CSID_IRQ_REG_RDI_0
	{
		if (irq_status & IFE_CSID_VER2_PATH_INFO_INPUT_SOF)	 //	SOF
		{
			sof_curr_ts	= ktime_get();
			CAM_INFO(CAM_ISP, "sof-sof %d us",
				ktime_to_us(ktime_sub(sof_curr_ts, sof_ts)));
			sof_ts = sof_curr_ts;
		}
		else if	(irq_status & IFE_CSID_VER2_PATH_INFO_INPUT_EOF)
		{
			eof_ts = ktime_get();
			CAM_INFO(CAM_ISP, "vvalid %d us",
				ktime_to_us(ktime_sub(eof_ts, sof_ts)));
		}
	}
}
#endif


static int cam_ife_csid_ver2_parse_path_irq_status(
	struct cam_ife_csid_ver2_hw  *csid_hw,
	struct cam_isp_resource_node *res,
	uint32_t                     index,
	uint32_t                     err_mask,
	uint32_t                     irq_status,
	struct cam_ife_csid_ver2_evt_payload *evt_payload)
{
	const uint8_t                          **irq_reg_tag;
	const struct cam_ife_csid_ver2_reg_info *csid_reg;
	uint32_t                                 bit_pos = 0;
	uint32_t                                 status, err_type = 0;
	uint32_t                                 sof_irq_debug_en = 0;
	size_t                                   len;
	uint8_t                                 *log_buf = NULL;
	struct cam_ife_csid_ver2_path_cfg       *path_cfg = res->res_priv;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
		csid_hw->core_info->csid_reg;

	log_buf = csid_hw->log_buf;

	irq_reg_tag = cam_ife_csid_get_irq_reg_tag_ptr();

	status = irq_status & err_mask;
	while (status) {
		if (bit_pos >= csid_reg->num_path_err_irqs) {
			CAM_ERR(CAM_ISP, "Invalid bit position: %u for path reg", bit_pos);
			break;
		}

		len = 0;
		if ((csid_reg->path_irq_desc[bit_pos].bitmask != 0) && (status & 0x1)) {
			CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
				"CSID[%u] %s occurred at [%llu: %09llu]",
				csid_hw->hw_intf->hw_idx, csid_reg->path_irq_desc[bit_pos].irq_name,
				path_cfg->error_ts.tv_sec, path_cfg->error_ts.tv_nsec);
			if (csid_reg->path_irq_desc[bit_pos].desc)
				CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
					"%s", csid_reg->path_irq_desc[bit_pos].desc);
			if (csid_reg->path_irq_desc[bit_pos].debug)
				CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN, &len,
					"Debug: %s", csid_reg->path_irq_desc[bit_pos].debug);

			/* List possbilities for illegal programming */
			if (csid_reg->path_irq_desc[bit_pos].err_type) {
				if (csid_reg->path_irq_desc[bit_pos].err_type ==
					CAM_ISP_HW_ERROR_CSID_FATAL) {
					CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN,
						&len, "1. MUP programmed without RUP");
					CAM_ERR_BUF(CAM_ISP, log_buf, CAM_IFE_CSID_LOG_BUF_LEN,
						&len, "2. Early EOF enabled with vCrop disabled");
					CAM_ERR_BUF(CAM_ISP, log_buf,
						CAM_IFE_CSID_LOG_BUF_LEN, &len,
						"3. Mismatch in the decode formats configured for multi vc-dt");
				}

				err_type |=  csid_reg->path_irq_desc[bit_pos].err_type;
			}

			if (csid_reg->path_irq_desc[bit_pos].err_handler)
				csid_reg->path_irq_desc[bit_pos].err_handler(csid_hw, res);
		}

		if (len)
			CAM_ERR(CAM_ISP, "CSID[%u] %s status: 0x%x Errors:%s",
				csid_hw->hw_intf->hw_idx, irq_reg_tag[index],
				irq_status, log_buf);

		bit_pos++;
		status >>= 1;
	}

	status = irq_status & csid_hw->debug_info.path_mask;
	bit_pos = 0;
	while (status) {
		if (bit_pos >= csid_reg->num_path_err_irqs) {
			CAM_ERR(CAM_ISP, "Invalid bit position: %u for path reg", bit_pos);
			break;
		}

		if ((csid_reg->path_irq_desc[bit_pos].bitmask != 0) && (status & 0x1))
			CAM_INFO(CAM_ISP, "CSID[%u] IRQ %s %s timestamp: [%lld:%09lld]",
				csid_hw->hw_intf->hw_idx, irq_reg_tag[index],
				csid_reg->path_irq_desc[bit_pos].irq_name,
				evt_payload->timestamp.tv_sec,
				evt_payload->timestamp.tv_nsec);

#if defined(CONFIG_SAMSUNG_CAMERA)
		if (status & 0x1)
			cam_ife_csid_ver2_print_debug_sensor_timing(csid_hw, index, irq_status);
#endif

		bit_pos++;
		status >>= 1;
	}

	if (csid_hw->flags.sof_irq_triggered) {
		if ((irq_status & IFE_CSID_VER2_PATH_INFO_INPUT_SOF))
			csid_hw->counters.irq_debug_cnt++;

		if (csid_hw->counters.irq_debug_cnt >=
			CAM_CSID_IRQ_SOF_DEBUG_CNT_MAX) {
			cam_ife_csid_ver2_sof_irq_debug(csid_hw,
				&sof_irq_debug_en);
			csid_hw->counters.irq_debug_cnt = 0;
		}
	}

	return err_type;
}

static void cam_ife_csid_ver2_top_err_process_bottom_half(
	struct cam_ife_csid_ver2_hw                  *csid_hw,
	struct cam_ife_csid_ver2_evt_payload         *payload,
	enum cam_ife_csid_top_irq_regs                top_idx)
{
	struct cam_ife_csid_ver2_reg_info          *csid_reg;
	uint32_t                                    irq_status;
	uint32_t                                    event_type = 0;
	uint32_t                                    i = 0;
	uint32_t                                    irq_reg_val = payload->irq_reg_val;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *) csid_hw->core_info->csid_reg;

	irq_status = irq_reg_val & csid_reg->cmn_reg->top_err_irq_mask[top_idx];

	if (!irq_status) {
		CAM_ERR(CAM_ISP, "CSID:%u Unexpected Scenario top_idx:%u",
			csid_hw->hw_intf->hw_idx, top_idx);
		return;
	}

	for (i = 0; i < csid_reg->num_top_err_irqs[top_idx]; i++) {
		if (!csid_reg->top_irq_desc) {
			CAM_ERR(CAM_ISP, "CSID:%u Unexpected Scenario top irq descriptor empty",
				csid_hw->hw_intf->hw_idx);
			break;
		}

		if ((*csid_reg->top_irq_desc)[top_idx][i].bitmask & irq_status) {
			CAM_ERR(CAM_ISP, "CSID[%u] %s occurred at [%llu: %09llu]",
				csid_hw->hw_intf->hw_idx,
				(*csid_reg->top_irq_desc)[top_idx][i].err_name,
				payload->timestamp.tv_sec, payload->timestamp.tv_nsec);
			CAM_ERR(CAM_ISP, "%s", (*csid_reg->top_irq_desc)[top_idx][i].desc);
			if ((*csid_reg->top_irq_desc)[top_idx][i].debug)
				CAM_ERR(CAM_ISP, "Debug: %s",
					(*csid_reg->top_irq_desc)[top_idx][i].debug);

			if ((*csid_reg->top_irq_desc)[top_idx][i].err_handler)
				(*csid_reg->top_irq_desc)[top_idx][i].err_handler(csid_hw);

			event_type |= (*csid_reg->top_irq_desc)[top_idx][i].err_type;
		}
	}

	if (event_type)
		cam_ife_csid_ver2_handle_event_err(csid_hw,
			irq_status, event_type, false, NULL);

}

static int cam_ife_csid_ver2_top2_err_irq_bottom_half(
	void                                      *handler_priv,
	void                                      *evt_payload_priv)
{
	struct cam_ife_csid_ver2_evt_payload       *payload;
	struct cam_ife_csid_ver2_hw                *csid_hw = NULL;
	struct cam_ife_csid_ver2_reg_info          *csid_reg = NULL;

	if (!handler_priv || !evt_payload_priv) {
		CAM_ERR(CAM_ISP, "Invalid params");
		return -EINVAL;
	}

	payload = evt_payload_priv;
	csid_hw = handler_priv;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *) csid_hw->core_info->csid_reg;

	if (!csid_reg->num_top_err_irqs) {
		CAM_WARN_RATE_LIMIT_CUSTOM(CAM_ISP, 100, 1,
			"CSID:%u Unexpected Scenario no top error irqs", csid_hw->hw_intf->hw_idx);
		cam_ife_csid_ver2_put_evt_payload(csid_hw, &payload,
			&csid_hw->path_free_payload_list, &csid_hw->path_payload_lock);
		return 0;
	}

	cam_ife_csid_ver2_top_err_process_bottom_half(csid_hw, payload,
		CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1);

	cam_ife_csid_ver2_put_evt_payload(csid_hw, &payload,
		&csid_hw->path_free_payload_list,
		&csid_hw->path_payload_lock);

	return 0;
}

static int cam_ife_csid_ver2_process_top_info(
	struct cam_ife_csid_ver2_hw           *csid_hw,
	uint32_t                               irq_reg_val,
	enum cam_ife_csid_top_irq_regs         top_idx,
	struct timespec64                      timestamp)
{
	struct cam_ife_csid_ver2_reg_info                *csid_reg;
	uint32_t                                          irq_status;
	const uint64_t                                   *evt_bitmap;
	const uint8_t                                    *bit_pos;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *) csid_hw->core_info->csid_reg;
	evt_bitmap = csid_reg->top_debug_mask->evt_bitmap;
	bit_pos = csid_reg->top_debug_mask->bit_pos;

	irq_status = irq_reg_val & csid_hw->debug_info.top_mask[top_idx];

	if (!irq_status) {
		CAM_ERR(CAM_ISP, "CSID:%u Unexpected Scenario top_idx:%u",
			csid_hw->hw_intf->hw_idx, top_idx);
		return 0;
	}

	if ((evt_bitmap[top_idx] & BIT_ULL(CAM_IFE_CSID_TOP_INFO_VOTE_UP)) &&
		(irq_status & BIT(bit_pos[CAM_IFE_CSID_TOP_INFO_VOTE_UP]))) {
		cam_cpas_log_votes(true);
		CAM_INFO(CAM_ISP, "CSID:%d INFO_VOTE_UP timestamp:(%lld.%09lld)",
			csid_hw->hw_intf->hw_idx, timestamp.tv_sec,
			timestamp.tv_nsec);
	}

	if ((evt_bitmap[top_idx] & BIT_ULL(CAM_IFE_CSID_TOP_INFO_VOTE_DN)) &&
		(irq_status & BIT(bit_pos[CAM_IFE_CSID_TOP_INFO_VOTE_DN]))) {
		cam_cpas_log_votes(true);
		CAM_INFO(CAM_ISP, "CSID:%d INFO_VOTE_DN timestamp:(%lld.%09lld)",
			csid_hw->hw_intf->hw_idx, timestamp.tv_sec,
			timestamp.tv_nsec);
	}

	if ((evt_bitmap[top_idx] & BIT_ULL(CAM_IFE_CSID_TOP_ERR_NO_VOTE_DN)) &&
		(irq_status & BIT(bit_pos[CAM_IFE_CSID_TOP_ERR_NO_VOTE_DN]))) {
		CAM_INFO(CAM_ISP, "CSID:%d ERR_NO_VOTE_DN timestamp:(%lld.%09lld)",
			csid_hw->hw_intf->hw_idx, timestamp.tv_sec,
			timestamp.tv_nsec);

		cam_ife_csid_hw_ver2_drv_err_handler(csid_hw);
	}
	return 0;
}

static int cam_ife_csid_ver2_top2_info_irq_bottom_half(
	void                                      *handler_priv,
	void                                      *evt_payload_priv)
{
	struct cam_ife_csid_ver2_evt_payload       *payload;
	struct cam_ife_csid_ver2_hw                *csid_hw = NULL;

	if (!handler_priv || !evt_payload_priv) {
		CAM_ERR(CAM_ISP, "Invalid params");
		return -EINVAL;
	}

	payload = evt_payload_priv;
	csid_hw = handler_priv;

	cam_ife_csid_ver2_process_top_info(csid_hw, payload->irq_reg_val,
		CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1, payload->timestamp);

	cam_ife_csid_ver2_put_evt_payload(csid_hw, &payload,
		&csid_hw->path_free_payload_list,
		&csid_hw->path_payload_lock);

	return 0;
}

static int cam_ife_csid_ver2_top_info_irq_bottom_half(
	void                                      *handler_priv,
	void                                      *evt_payload_priv)
{
	struct cam_ife_csid_ver2_evt_payload       *payload;
	struct cam_ife_csid_ver2_hw                *csid_hw = NULL;

	if (!handler_priv || !evt_payload_priv) {
		CAM_ERR(CAM_ISP, "Invalid params");
		return -EINVAL;
	}

	payload = evt_payload_priv;
	csid_hw = handler_priv;

	cam_ife_csid_ver2_process_top_info(csid_hw, payload->irq_reg_val,
		CAM_IFE_CSID_TOP_IRQ_STATUS_REG0, payload->timestamp);

	cam_ife_csid_ver2_put_evt_payload(csid_hw, &payload,
		&csid_hw->path_free_payload_list,
		&csid_hw->path_payload_lock);

	return 0;
}

static int cam_ife_csid_ver2_top_err_irq_bottom_half(
	void                                      *handler_priv,
	void                                      *evt_payload_priv)
{
	struct cam_ife_csid_ver2_evt_payload       *payload;
	struct cam_ife_csid_ver2_hw                *csid_hw = NULL;
	struct cam_ife_csid_ver2_reg_info          *csid_reg = NULL;

	if (!handler_priv || !evt_payload_priv) {
		CAM_ERR(CAM_ISP, "Invalid params");
		return -EINVAL;
	}

	payload = evt_payload_priv;
	csid_hw = handler_priv;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
		csid_hw->core_info->csid_reg;

	if (!csid_reg->num_top_err_irqs) {
		CAM_WARN_RATE_LIMIT_CUSTOM(CAM_ISP, 100, 1,
			"CSID:%u Unexpected Scenario no top error irqs", csid_hw->hw_intf->hw_idx);
		cam_ife_csid_ver2_put_evt_payload(csid_hw, &payload,
			&csid_hw->path_free_payload_list, &csid_hw->path_payload_lock);
		return 0;
	}

	cam_ife_csid_ver2_top_err_process_bottom_half(csid_hw, payload,
		CAM_IFE_CSID_TOP_IRQ_STATUS_REG0);

	cam_ife_csid_ver2_put_evt_payload(csid_hw, &payload,
		&csid_hw->path_free_payload_list,
		&csid_hw->path_payload_lock);

	return 0;
}

void cam_ife_csid_ver2_print_format_measure_info(
	void *csid, void *resource)
{
	struct cam_ife_csid_ver2_hw       *csid_hw = csid;
	struct cam_isp_resource_node      *res = resource;
	struct cam_ife_csid_ver2_reg_info *csid_reg = csid_hw->core_info->csid_reg;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg =
		csid_reg->path_reg[res->res_id];
	struct cam_hw_soc_info *soc_info = &csid_hw->hw_info->soc_info;
	void __iomem *base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;
	uint32_t expected_frame = 0, actual_frame = 0, format_measure_cfg0 = 0;
	int data_idx = 0;

	actual_frame = cam_io_r_mb(base + path_reg->format_measure0_addr);
	expected_frame = cam_io_r_mb(base + path_reg->format_measure_cfg1_addr);
	format_measure_cfg0 = cam_io_r_mb(base + path_reg->format_measure_cfg0_addr);

	CAM_INFO(CAM_ISP, "CSID[%u] res [id: %d name: %s] format_measure_cfg0: 0x%x",
		csid_hw->hw_intf->hw_idx, res->res_id, res->res_name, format_measure_cfg0);
	CAM_ERR(CAM_ISP, "CSID[%u] Frame Size Error Expected[h: %u w: %u] Actual[h: %u w: %u]",
		csid_hw->hw_intf->hw_idx, ((expected_frame >>
		csid_reg->cmn_reg->format_measure_height_shift_val) &
		csid_reg->cmn_reg->format_measure_height_mask_val),
		expected_frame &
		csid_reg->cmn_reg->format_measure_width_mask_val,
		((actual_frame >>
		csid_reg->cmn_reg->format_measure_height_shift_val) &
		csid_reg->cmn_reg->format_measure_height_mask_val),
		actual_frame &
		csid_reg->cmn_reg->format_measure_width_mask_val);

	cam_ife_csid_ver2_print_hbi_vbi(csid_hw);

	/* AUX settings update to phy for pix and line count errors */
	data_idx = (int)(csid_hw->rx_cfg.phy_sel - csid_reg->cmn_reg->phy_sel_base_idx);
	if (data_idx < 0)
		CAM_WARN(CAM_ISP, "Can't notify csiphy, incorrect phy selected=%d",
			data_idx);
	else
		cam_subdev_notify_message(CAM_CSIPHY_DEVICE_TYPE,
			CAM_SUBDEV_MESSAGE_APPLY_CSIPHY_AUX, (void *)&data_idx);

}

static int cam_ife_csid_ver2_ipp_bottom_half(
	void                                      *handler_priv,
	void                                      *evt_payload_priv)
{
	struct cam_ife_csid_ver2_evt_payload         *payload;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg;
	struct cam_ife_csid_ver2_reg_info            *csid_reg;
	struct cam_isp_resource_node                 *res;
	struct cam_ife_csid_ver2_hw                  *csid_hw = NULL;
	struct cam_isp_hw_event_info                  evt_info;
	struct cam_hw_info                           *hw_info;
	struct cam_ife_csid_ver2_path_cfg            *path_cfg;
	struct cam_isp_sof_ts_data                    sof_and_boot_time;
	const uint8_t                               **irq_reg_tag;
	uint32_t                                      irq_status_ipp;
	uint32_t                                      err_mask;
	uint32_t                                      err_type = 0;
	uint32_t                                      sof_irq_mask = 0;
	uint32_t                                      eof_irq_mask = 0;
	uint32_t                                      epoch0_irq_mask = 0;
	uint32_t                                      rup_irq_mask = 0;
	int                                           rc = 0;
	bool                                          out_of_sync_fatal = false;

	if (!handler_priv || !evt_payload_priv) {
		CAM_ERR(CAM_ISP, "Invalid params. evt_payload_priv: %s, handler_priv: %s",
			CAM_IS_NULL_TO_STR(evt_payload_priv),
			CAM_IS_NULL_TO_STR(handler_priv));
		return -EINVAL;
	}

	payload = evt_payload_priv;
	res   =  handler_priv;
	hw_info = (struct cam_hw_info *)res->hw_intf->hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	csid_reg = csid_hw->core_info->csid_reg;
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
	irq_reg_tag = cam_ife_csid_get_irq_reg_tag_ptr();

	if (!path_cfg || (path_cfg->irq_reg_idx >= CAM_IFE_CSID_IRQ_REG_MAX)) {
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid params: path_cfg: %pK, irq_reg_idx: %d",
			csid_hw->hw_intf->hw_idx, path_cfg,
			(path_cfg ? (path_cfg->irq_reg_idx) : -1));
		rc = -EINVAL;
		goto end;
	}

	irq_status_ipp = payload->irq_reg_val;
	sof_and_boot_time.boot_time = payload->timestamp;
	sof_and_boot_time.sof_ts = payload->sof_ts_reg_val;

	CAM_DBG(CAM_ISP, "CSID[%u] %s status:0x%x", csid_hw->hw_intf->hw_idx,
		(payload->is_mc ? "COMP_IRQ" : ((char *)irq_reg_tag[path_cfg->irq_reg_idx])),
		irq_status_ipp);

	if (!csid_hw->flags.device_enabled) {
		CAM_DBG(CAM_ISP, "CSID[%u] bottom-half after stop [0x%x]",
			csid_hw->hw_intf->hw_idx, irq_status_ipp);
		goto end;
	}

	evt_info.hw_type  = CAM_ISP_HW_TYPE_CSID;
	evt_info.hw_idx   = csid_hw->hw_intf->hw_idx;
	evt_info.res_type = CAM_ISP_RESOURCE_PIX_PATH;
	evt_info.reg_val  = irq_status_ipp;
	evt_info.event_data = &sof_and_boot_time;

	if (!csid_hw->event_cb) {
		CAM_ERR_RATE_LIMIT(CAM_ISP, "CSID[%u] event cb not registered",
			csid_hw->hw_intf->hw_idx);
		goto end;
	}

	path_reg = csid_reg->path_reg[res->res_id];

	if (payload->is_mc) {
		sof_irq_mask = csid_reg->ipp_mc_reg->comp_sof_mask;
		eof_irq_mask = csid_reg->ipp_mc_reg->comp_eof_mask;
		rup_irq_mask = csid_reg->ipp_mc_reg->comp_rup_mask;
		epoch0_irq_mask = csid_reg->ipp_mc_reg->comp_epoch0_mask;
		evt_info.res_id   = CAM_IFE_PIX_PATH_RES_IPP;
	} else {
		sof_irq_mask = path_reg->sof_irq_mask;
		eof_irq_mask = path_reg->eof_irq_mask;
		rup_irq_mask = path_reg->rup_irq_mask;
		epoch0_irq_mask = path_reg->epoch0_irq_mask;
		evt_info.res_id   = res->res_id;
	}

	if (irq_status_ipp & eof_irq_mask) {
		if ((!csid_hw->flags.last_exp_valid) ||
			(csid_hw->flags.last_exp_valid && path_cfg->allow_epoch_eof_cb)) {
			cam_ife_csid_ver2_update_event_ts(&path_cfg->eof_ts, &payload->timestamp);
			csid_hw->event_cb(csid_hw->token, CAM_ISP_HW_EVENT_EOF, (void *)&evt_info);
		}
	}

	if (irq_status_ipp & sof_irq_mask) {
		cam_ife_csid_ver2_update_event_ts(&path_cfg->sof_ts, &payload->timestamp);
		csid_hw->event_cb(csid_hw->token, CAM_ISP_HW_EVENT_SOF, (void *)&evt_info);
	}

	if (irq_status_ipp & rup_irq_mask)
		csid_hw->event_cb(csid_hw->token, CAM_ISP_HW_EVENT_REG_UPDATE, (void *)&evt_info);

	if (irq_status_ipp & epoch0_irq_mask) {
		if ((!csid_hw->flags.last_exp_valid) ||
			(csid_hw->flags.last_exp_valid && path_cfg->allow_epoch_eof_cb)) {
			cam_ife_csid_ver2_update_event_ts(&path_cfg->epoch_ts, &payload->timestamp);
			csid_hw->event_cb(csid_hw->token, CAM_ISP_HW_EVENT_EPOCH,
				(void *)&evt_info);
		}
	}

	if (payload->is_mc) {
		payload->is_mc = false;
		goto end;
	}

	if (irq_status_ipp & IFE_CSID_VER2_PATH_SENSOR_SWITCH_OUT_OF_SYNC_FRAME_DROP) {
		atomic_inc(&path_cfg->switch_out_of_sync_cnt);
		/* If threshold is seen, notify error */
		if (atomic_read(&path_cfg->switch_out_of_sync_cnt) >=
			CAM_IFE_CSID_MAX_OUT_OF_SYNC_ERR_COUNT)
			out_of_sync_fatal = true;
	}

	err_mask = path_reg->fatal_err_mask | path_reg->non_fatal_err_mask;
	spin_lock(&csid_hw->lock_state);
	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID[%u] powered down state",
			csid_hw->hw_intf->hw_idx);
		goto unlock;
	}

	err_type = cam_ife_csid_ver2_parse_path_irq_status(
		csid_hw, res, path_cfg->irq_reg_idx,
		err_mask, irq_status_ipp, payload);

	if (err_type || out_of_sync_fatal) {
		if (out_of_sync_fatal)
			err_type = CAM_ISP_HW_ERROR_CSID_SENSOR_SWITCH_ERROR;

		cam_ife_csid_ver2_handle_event_err(csid_hw,
			irq_status_ipp, err_type, false, res);
	}

unlock:
	spin_unlock(&csid_hw->lock_state);
end:
	cam_ife_csid_ver2_put_evt_payload(csid_hw, &payload,
			&csid_hw->path_free_payload_list,
			&csid_hw->path_payload_lock);

	return rc;
}

static int cam_ife_csid_ver2_ppp_bottom_half(
	void                                      *handler_priv,
	void                                      *evt_payload_priv)
{
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = NULL;
	struct cam_ife_csid_ver2_evt_payload         *payload;
	struct cam_ife_csid_ver2_path_cfg            *path_cfg;
	struct cam_ife_csid_ver2_reg_info            *csid_reg;
	struct cam_isp_resource_node                 *res;
	struct cam_ife_csid_ver2_hw                  *csid_hw = NULL;
	struct cam_hw_info                           *hw_info;
	uint32_t                                      irq_status_ppp;
	uint32_t                                      err_mask;
	uint32_t                                      err_type = 0;
	int                                           rc = 0;

	if (!handler_priv || !evt_payload_priv) {
		CAM_ERR(CAM_ISP, "Invalid params. evt_payload_priv: %s, handler_priv: %s",
			CAM_IS_NULL_TO_STR(evt_payload_priv),
			CAM_IS_NULL_TO_STR(handler_priv));
		return -EINVAL;
	}

	payload = evt_payload_priv;
	res   =  handler_priv;
	hw_info = (struct cam_hw_info *)res->hw_intf->hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;

	if (!path_cfg || (path_cfg->irq_reg_idx >= CAM_IFE_CSID_IRQ_REG_MAX)) {
		CAM_ERR(CAM_ISP, "CSID:%u Invalid params: path_cfg: %pK, irq_reg_idx: %d",
			csid_hw->hw_intf->hw_idx, path_cfg,
			(path_cfg ? (path_cfg->irq_reg_idx) : -1));
		rc = -EINVAL;
		goto end;
	}

	irq_status_ppp = payload->irq_reg_val;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
		csid_hw->core_info->csid_reg;

	path_reg = csid_reg->path_reg[res->res_id];
	err_mask = path_reg->fatal_err_mask | path_reg->non_fatal_err_mask;

	CAM_DBG(CAM_ISP, "CSID[%u] PPP status:0x%x", csid_hw->hw_intf->hw_idx,
		irq_status_ppp);

	if (!csid_hw->flags.device_enabled) {
		CAM_DBG(CAM_ISP, "CSID[%u] bottom-half after stop [0x%x]",
			csid_hw->hw_intf->hw_idx, irq_status_ppp);
		goto end;
	}

	spin_lock(&csid_hw->lock_state);
	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID[%u] powered down state",
			csid_hw->hw_intf->hw_idx);
		goto unlock;
	}

	err_type = cam_ife_csid_ver2_parse_path_irq_status(
		csid_hw, res, CAM_IFE_CSID_IRQ_REG_PPP,
		err_mask, irq_status_ppp, payload);

	if (err_type)
		cam_ife_csid_ver2_handle_event_err(csid_hw,
			irq_status_ppp,
			err_type,
			false,
			res);
unlock:
	spin_unlock(&csid_hw->lock_state);
end:
	cam_ife_csid_ver2_put_evt_payload(csid_hw, &payload,
			&csid_hw->path_free_payload_list,
			&csid_hw->path_payload_lock);

	return rc;
}

static int cam_ife_csid_ver2_rdi_bottom_half(
	void                                      *handler_priv,
	void                                      *evt_payload_priv)
{
	struct cam_ife_csid_ver2_evt_payload         *payload;
	struct cam_ife_csid_ver2_hw                  *csid_hw = NULL;
	struct cam_ife_csid_ver2_reg_info            *csid_reg;
	struct cam_ife_csid_ver2_path_cfg            *path_cfg;
	struct cam_isp_resource_node                 *res;
	const struct cam_ife_csid_ver2_path_reg_info *rdi_reg;
	struct cam_hw_info                           *hw_info;
	uint32_t                                      irq_status_rdi;
	uint32_t                                      err_mask;
	uint32_t                                      err_type = 0;
	struct cam_isp_hw_event_info                  evt_info;
	struct cam_isp_sof_ts_data                    sof_and_boot_time;
	int                                           rc = 0;

	if (!handler_priv || !evt_payload_priv) {
		CAM_ERR(CAM_ISP, "Invalid params. evt_payload_priv: %s, handler_priv: %s",
			CAM_IS_NULL_TO_STR(evt_payload_priv),
			CAM_IS_NULL_TO_STR(handler_priv));
		return -EINVAL;
	}

	payload = evt_payload_priv;
	res   =  handler_priv;
	hw_info = (struct cam_hw_info *)res->hw_intf->hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;

	if (!path_cfg || (path_cfg->irq_reg_idx >= CAM_IFE_CSID_IRQ_REG_MAX)) {
		CAM_ERR(CAM_ISP, "CSID:%u Invalid params: path_cfg: %pK, irq_reg_idx: %d",
			csid_hw->hw_intf->hw_idx, path_cfg,
			(path_cfg ? (path_cfg->irq_reg_idx) : -1));
		rc = -EINVAL;
		goto end;
	}

	sof_and_boot_time.boot_time = payload->timestamp;
	sof_and_boot_time.sof_ts = payload->sof_ts_reg_val;

	evt_info.hw_idx   = csid_hw->hw_intf->hw_idx;
	evt_info.res_type = CAM_ISP_RESOURCE_PIX_PATH;
	evt_info.event_data = &sof_and_boot_time;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
		csid_hw->core_info->csid_reg;

	irq_status_rdi = payload->irq_reg_val;
	rdi_reg = csid_reg->path_reg[res->res_id];

	if (!rdi_reg)
		goto end;

	if (!csid_hw->flags.device_enabled) {
		CAM_DBG(CAM_ISP, "CSID:%u bottom-half after stop [0x%x]",
			csid_hw->hw_intf->hw_idx, irq_status_rdi);
		goto end;
	}

	CAM_DBG(CAM_ISP, "CSID[%u] RDI:%d status:0x%x",
			csid_hw->hw_intf->hw_idx,
			res->res_id, irq_status_rdi);
	err_mask = rdi_reg->non_fatal_err_mask |
		rdi_reg->fatal_err_mask;

	spin_lock(&csid_hw->lock_state);
	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID[%u] powered down state",
			csid_hw->hw_intf->hw_idx);
		spin_unlock(&csid_hw->lock_state);
		goto end;
	}

	err_type = cam_ife_csid_ver2_parse_path_irq_status(csid_hw, res,
		path_cfg->irq_reg_idx,
		err_mask, irq_status_rdi, payload);

	spin_unlock(&csid_hw->lock_state);
	if (err_type) {
		cam_ife_csid_ver2_handle_event_err(csid_hw,
			irq_status_rdi, err_type, false, res);
		goto end;
	}

	if (!csid_hw->event_cb) {
		CAM_DBG(CAM_ISP, "CSID[%u] no cb registered",
				csid_hw->hw_intf->hw_idx);
		goto end;
	}

	evt_info.res_id = res->res_id;
	evt_info.reg_val = irq_status_rdi;
	evt_info.hw_type = CAM_ISP_HW_TYPE_CSID;

	if (irq_status_rdi & IFE_CSID_VER2_PATH_SENSOR_SWITCH_OUT_OF_SYNC_FRAME_DROP) {
		bool is_secondary = true;
		bool do_notify = false;

		/* Only notify if secondary event is subscribed for */
		if ((path_cfg->sec_evt_config.en_secondary_evt) &&
			(path_cfg->sec_evt_config.evt_type &
			CAM_IFE_CSID_EVT_SENSOR_SYNC_FRAME_DROP))
			do_notify = true;

		/* Validate error threshold for primary RDI (master) */
		if (res->is_rdi_primary_res) {
			atomic_inc(&path_cfg->switch_out_of_sync_cnt);
			if (atomic_read(&path_cfg->switch_out_of_sync_cnt) >=
				CAM_IFE_CSID_MAX_OUT_OF_SYNC_ERR_COUNT) {
				do_notify = true;
				is_secondary = false;
			}
		}

		if (do_notify)
			cam_ife_csid_ver2_handle_event_err(csid_hw, irq_status_rdi,
				CAM_ISP_HW_ERROR_CSID_SENSOR_FRAME_DROP, is_secondary, res);
	}

	if (irq_status_rdi & rdi_reg->eof_irq_mask) {
		if ((!csid_hw->flags.last_exp_valid) ||
			(csid_hw->flags.last_exp_valid && path_cfg->allow_epoch_eof_cb)) {
			cam_ife_csid_ver2_update_event_ts(&path_cfg->eof_ts, &payload->timestamp);
			csid_hw->event_cb(csid_hw->token, CAM_ISP_HW_EVENT_EOF, (void *)&evt_info);
		}
	}

	if ((irq_status_rdi & rdi_reg->sof_irq_mask)) {
		if (path_cfg->sec_evt_config.en_secondary_evt &&
			(path_cfg->sec_evt_config.evt_type & CAM_IFE_CSID_EVT_SOF)) {
			evt_info.is_secondary_evt = true;
		}
		cam_ife_csid_ver2_update_event_ts(&path_cfg->sof_ts, &payload->timestamp);
		csid_hw->event_cb(csid_hw->token, CAM_ISP_HW_EVENT_SOF,	(void *)&evt_info);
	}

	if (irq_status_rdi & rdi_reg->rup_irq_mask)
		csid_hw->event_cb(csid_hw->token, CAM_ISP_HW_EVENT_REG_UPDATE, (void *)&evt_info);

	if ((irq_status_rdi & rdi_reg->epoch0_irq_mask)) {
		if (path_cfg->sec_evt_config.en_secondary_evt &&
			(path_cfg->sec_evt_config.evt_type & CAM_IFE_CSID_EVT_EPOCH)) {
			evt_info.is_secondary_evt = true;
		}

		if ((!csid_hw->flags.last_exp_valid) ||
			(csid_hw->flags.last_exp_valid && path_cfg->allow_epoch_eof_cb)) {
			CAM_INFO(CAM_ISP, "CSID: %u epoch recieved from rdi", csid_hw->hw_intf->hw_idx);
			cam_ife_csid_ver2_update_event_ts(&path_cfg->epoch_ts, &payload->timestamp);
			csid_hw->event_cb(csid_hw->token, CAM_ISP_HW_EVENT_EPOCH,
				(void *)&evt_info);
		}
	}
end:
	cam_ife_csid_ver2_put_evt_payload(csid_hw, &payload,
		&csid_hw->path_free_payload_list, &csid_hw->path_payload_lock);

	return rc;
}

int cam_ife_csid_ver2_get_hw_caps(void *hw_priv,
	void *get_hw_cap_args, uint32_t arg_size)
{
	int rc = 0;
	struct cam_ife_csid_hw_caps           *hw_caps;
	struct cam_ife_csid_ver2_hw           *csid_hw;
	struct cam_hw_info                    *hw_info;
	struct cam_csid_soc_private           *soc_private = NULL;
	struct cam_ife_csid_ver2_reg_info     *csid_reg;

	if (!hw_priv || !get_hw_cap_args) {
		CAM_ERR(CAM_ISP, "CSID: Invalid args");
		return -EINVAL;
	}

	hw_info = (struct cam_hw_info  *)hw_priv;

	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	hw_caps = (struct cam_ife_csid_hw_caps *) get_hw_cap_args;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	soc_private = (struct cam_csid_soc_private *)
			csid_hw->hw_info->soc_info.soc_private;

	hw_caps->num_rdis = csid_reg->cmn_reg->num_rdis;
	hw_caps->num_pix = csid_reg->cmn_reg->num_pix;
	hw_caps->num_ppp = csid_reg->cmn_reg->num_ppp;
	hw_caps->major_version = csid_reg->cmn_reg->major_version;
	hw_caps->minor_version = csid_reg->cmn_reg->minor_version;
	hw_caps->version_incr = csid_reg->cmn_reg->version_incr;
	hw_caps->global_reset_en = csid_reg->cmn_reg->global_reset;
	hw_caps->aup_rup_en = csid_reg->cmn_reg->aup_rup_supported;
	hw_caps->only_master_rup = csid_reg->cmn_reg->only_master_rup;
	hw_caps->is_lite = soc_private->is_ife_csid_lite;
	hw_caps->sfe_ipp_input_rdi_res = csid_reg->cmn_reg->sfe_ipp_input_rdi_res;
	hw_caps->camif_irq_support = csid_reg->cmn_reg->camif_irq_support;
	hw_caps->is_ife_sfe_mapped = csid_reg->is_ife_sfe_mapped;

	CAM_DBG(CAM_ISP,
		"CSID:%u num-rdis:%d, num-pix:%d, major:%d minor:%d ver:%d",
		csid_hw->hw_intf->hw_idx, hw_caps->num_rdis,
		hw_caps->num_pix, hw_caps->major_version,
		hw_caps->minor_version, hw_caps->version_incr);

	return rc;
}

static void cam_ife_csid_ver2_dump_imp_regs(
	struct cam_ife_csid_ver2_hw *csid_hw)
{
	void __iomem *mem_base;
	int top_irq_val[4] = {0};
	int reset_cfg = 0;
	int dual_csid_cfg = 0;
	uint32_t clk_lvl;
	uint32_t hw_idx;
	struct cam_ife_csid_ver2_reg_info *csid_reg;
	struct cam_hw_soc_info            *soc_info;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
		csid_hw->core_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;
	hw_idx = csid_hw->hw_intf->hw_idx;

	/* Dumping CSID top irq registers */
	top_irq_val[0] = cam_io_r_mb(mem_base +
		csid_reg->cmn_reg->top_irq_status_addr[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0]);
	top_irq_val[1] = cam_io_r_mb(mem_base +
		csid_reg->cmn_reg->top_irq_mask_addr[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0]);
	top_irq_val[2] = cam_io_r_mb(mem_base +
		csid_reg->cmn_reg->top_irq_clear_addr[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0]);
	top_irq_val[3] = cam_io_r_mb(mem_base +
		csid_reg->cmn_reg->top_irq_set_addr[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0]);
	CAM_INFO(CAM_ISP,
		"CSID[%d] csid top status 0x%x, mask 0x%x, clr 0x%x, set 0x%x",
		hw_idx, top_irq_val[0], top_irq_val[1],
		top_irq_val[2], top_irq_val[3]);

	/* Dumping CSID reset cfg and dual csid cfg */
	reset_cfg = cam_io_r_mb(mem_base + csid_reg->cmn_reg->reset_cfg_addr);

	if (csid_reg->top_reg)
		dual_csid_cfg = cam_io_r_mb(
			soc_info->reg_map[CAM_IFE_CSID_TOP_MEM_BASE_ID].mem_base +
			csid_reg->top_reg->dual_csid_cfg0_addr[hw_idx]);

	CAM_INFO(CAM_ISP, "CSID[%d] csid reset_cfg 0x%x, dual_csid_cfg 0x%x, is_dual_en %d",
		hw_idx, reset_cfg, dual_csid_cfg, csid_hw->top_cfg.dual_en);

	/* Dumping CSID Clock */
	cam_soc_util_get_clk_level(soc_info, csid_hw->clk_rate,
		soc_info->src_clk_idx, &clk_lvl);

	CAM_INFO(CAM_ISP,
		"CSID[%d] clk lvl %u received clk_rate %u applied clk_rate sw_client:%lu hw_client:[%lu %lu]",
		hw_idx, clk_lvl, csid_hw->clk_rate,
		soc_info->applied_src_clk_rates.sw_client,
		soc_info->applied_src_clk_rates.hw_client[hw_idx].high,
		soc_info->applied_src_clk_rates.hw_client[hw_idx].low);
}

static int cam_ife_csid_ver2_wait_for_reset(
	struct cam_ife_csid_ver2_hw *csid_hw)
{
	unsigned long rem_jiffies = 0;
	int rc = 0;

	rem_jiffies = cam_common_wait_for_completion_timeout(
		&csid_hw->hw_info->hw_complete,
		msecs_to_jiffies(CAM_IFE_CSID_RESET_TIMEOUT_MS));

	if (rem_jiffies == 0) {
		rc = -ETIMEDOUT;
		if (csid_hw->debug_info.test_bus_enabled) {
			struct cam_hw_soc_info *soc_info = &csid_hw->hw_info->soc_info;
			struct cam_ife_csid_ver2_reg_info *csid_reg =
				(struct cam_ife_csid_ver2_reg_info *) csid_hw->core_info->csid_reg;

			CAM_ERR(CAM_ISP,
				"CSID[%u], sync-mode[%d] test_bus: 0x%x reset timed out",
				csid_hw->hw_intf->hw_idx, csid_hw->sync_mode,
				cam_io_r_mb(
					soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base +
					csid_reg->cmn_reg->test_bus_debug));
		} else
			CAM_ERR(CAM_ISP, "CSID[%u], sync-mode[%d] reset timed out",
				csid_hw->hw_intf->hw_idx, csid_hw->sync_mode);

		cam_ife_csid_ver2_dump_imp_regs(csid_hw);
	} else
		CAM_DBG(CAM_ISP,
			"CSID[%u], sync-mode[%d] reset success",
			csid_hw->hw_intf->hw_idx,
			csid_hw->sync_mode);

	return rc;
}

static int cam_ife_csid_ver2_reset_irq_top_half(uint32_t    evt_id,
	struct cam_irq_th_payload         *th_payload)
{
	struct cam_ife_csid_ver2_hw *csid_hw;

	csid_hw = th_payload->handler_priv;

	CAM_DBG(CAM_ISP, "CSID[%u] TOP_IRQ_STATUS_0 = 0x%x", csid_hw->hw_intf->hw_idx,
		th_payload->evt_status_arr[0]);

	complete(&csid_hw->hw_info->hw_complete);

	return 0;
}

static int cam_ife_csid_ver2_internal_reset(
	struct cam_ife_csid_ver2_hw *csid_hw,
	uint32_t rst_cmd, uint32_t rst_location, uint32_t rst_mode)
{
	uint32_t val = 0;
	struct cam_ife_csid_ver2_reg_info *csid_reg;
	struct cam_hw_soc_info                *soc_info;
	const struct cam_ife_csid_ver2_csi2_rx_reg_info *csi2_reg;
	void __iomem *mem_base;
	int rc = 0;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	csi2_reg = csid_reg->csi2_reg;

	soc_info = &csid_hw->hw_info->soc_info;
	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID[%u] powered down state",
			csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	/* Input cut off prior to SW reset */
	if (rst_cmd == CAM_IFE_CSID_RESET_CMD_SW_RST) {
		cam_io_w_mb(0x0, mem_base + csi2_reg->cfg0_addr);
		cam_io_w_mb(0x0, mem_base + csi2_reg->cfg1_addr);
	}

	if (csid_hw->sync_mode == CAM_ISP_HW_SYNC_SLAVE)
		goto wait_only;

	reinit_completion(&csid_hw->hw_info->hw_complete);

	/* Program the reset location */
	if (rst_location == CAM_IFE_CSID_RESET_LOC_PATH_ONLY)
		val |= (csid_reg->cmn_reg->rst_loc_path_only_val <<
			   csid_reg->cmn_reg->rst_location_shift_val);
	else if (rst_location == CAM_IFE_CSID_RESET_LOC_COMPLETE)
		val |= (csid_reg->cmn_reg->rst_loc_complete_csid_val <<
			   csid_reg->cmn_reg->rst_location_shift_val);

	/*Program the mode */
	if (rst_mode == CAM_CSID_HALT_AT_FRAME_BOUNDARY)
		val |= (csid_reg->cmn_reg->rst_mode_frame_boundary_val <<
			csid_reg->cmn_reg->rst_mode_shift_val);
	else if (rst_mode == CAM_CSID_HALT_IMMEDIATELY)
		val |= (csid_reg->cmn_reg->rst_mode_immediate_val <<
			csid_reg->cmn_reg->rst_mode_shift_val);

	cam_io_w_mb(val, mem_base + csid_reg->cmn_reg->reset_cfg_addr);
	CAM_DBG(CAM_ISP, "CSID[%u] reset_cfg: 0x%x",
		csid_hw->hw_intf->hw_idx, val);
	val = 0;

	/*Program the cmd */
	if (rst_cmd == CAM_IFE_CSID_RESET_CMD_IRQ_CTRL)
		val = csid_reg->cmn_reg->rst_cmd_irq_ctrl_only_val;
	else if (rst_cmd == CAM_IFE_CSID_RESET_CMD_HW_RST)
		val = csid_reg->cmn_reg->rst_cmd_hw_reset_complete_val;
	else if (rst_cmd == CAM_IFE_CSID_RESET_CMD_SW_RST)
		val = csid_reg->cmn_reg->rst_cmd_sw_reset_complete_val;

	cam_io_w_mb(val, mem_base + csid_reg->cmn_reg->reset_cmd_addr);
	CAM_DBG(CAM_ISP, "CSID[%u] reset_cmd: 0x%x",
		csid_hw->hw_intf->hw_idx, val);

	if (rst_cmd == CAM_IFE_CSID_RESET_CMD_IRQ_CTRL)
		return 0;

wait_only:

	rc = cam_ife_csid_ver2_wait_for_reset(csid_hw);

	if (rc)
		CAM_ERR(CAM_ISP,
			"CSID[%u] Reset failed mode %d cmd %d loc %d",
			csid_hw->hw_intf->hw_idx,
			rst_mode, rst_cmd, rst_location);
	reinit_completion(&csid_hw->hw_info->hw_complete);
	return rc;
}

int cam_ife_csid_ver2_reset(void *hw_priv,
	void *reset_args, uint32_t arg_size)
{
	struct cam_hw_info *hw_info;
	struct cam_ife_csid_ver2_hw *csid_hw;
	struct cam_csid_reset_cfg_args  *reset;
	int rc = 0;

	hw_info = (struct cam_hw_info *)hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	reset   = (struct cam_csid_reset_cfg_args  *)reset_args;

	mutex_lock(&csid_hw->hw_info->hw_mutex);
	switch (reset->reset_type) {
	case CAM_IFE_CSID_RESET_GLOBAL:
		rc = cam_ife_csid_ver2_internal_reset(csid_hw,
			CAM_IFE_CSID_RESET_CMD_SW_RST,
			CAM_IFE_CSID_RESET_LOC_COMPLETE,
			CAM_CSID_HALT_IMMEDIATELY);
		break;

	case CAM_IFE_CSID_RESET_PATH:
		rc = cam_ife_csid_ver2_internal_reset(csid_hw,
			CAM_IFE_CSID_RESET_CMD_HW_RST,
			CAM_IFE_CSID_RESET_LOC_PATH_ONLY,
			CAM_CSID_HALT_IMMEDIATELY);
		break;

	case CAM_IFE_CSID_RESET_GLOBAL_HW_ONLY:
		rc = cam_ife_csid_ver2_internal_reset(csid_hw,
			CAM_IFE_CSID_RESET_CMD_HW_RST,
			CAM_IFE_CSID_RESET_LOC_COMPLETE,
			CAM_CSID_HALT_IMMEDIATELY);
		break;

	case CAM_IFE_CSID_RESET_GLOBAL_IRQ_CNTRL:
		rc = cam_ife_csid_ver2_internal_reset(csid_hw,
			CAM_IFE_CSID_RESET_CMD_IRQ_CTRL,
			CAM_IFE_CSID_RESET_LOC_COMPLETE,
			CAM_CSID_HALT_IMMEDIATELY);
		break;

	default:
		rc = -EINVAL;
		break;
	}

	if (rc)
		CAM_ERR(CAM_ISP, "CSID[%u] reset type: %s failed",
			csid_hw->hw_intf->hw_idx,
			cam_ife_csid_reset_type_to_string(reset->reset_type));
	else
		CAM_DBG(CAM_ISP, "CSID[%u] reset type: %s",
			csid_hw->hw_intf->hw_idx,
			cam_ife_csid_reset_type_to_string(reset->reset_type));

	mutex_unlock(&csid_hw->hw_info->hw_mutex);
	return rc;
}

static inline void cam_ife_csid_ver2_maskout_path_irqs(
	int32_t res_id,
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_ife_csid_ver2_path_cfg *path_cfg)
{
	int rc;

	if (path_cfg->err_irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq(
			csid_hw->path_irq_controller[res_id],
				path_cfg->err_irq_handle);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe path err irq CSID:%u rc: %d",
				csid_hw->hw_intf->hw_idx, rc);

		path_cfg->err_irq_handle = 0;
	}

	if (path_cfg->irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq(
			csid_hw->path_irq_controller[res_id],
			path_cfg->irq_handle);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe path info irq CSID:%u rc: %d",
				csid_hw->hw_intf->hw_idx, rc);

		path_cfg->irq_handle = 0;
	}

	if (path_cfg->discard_irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq(
			csid_hw->path_irq_controller[res_id],
			path_cfg->discard_irq_handle);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe path discard irq CSID:%u rc: %d",
				csid_hw->hw_intf->hw_idx, rc);

		path_cfg->discard_irq_handle = 0;
	}

	if (path_cfg->top_irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			path_cfg->top_irq_handle);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe path irq in top CSID:%u rc: %d",
				csid_hw->hw_intf->hw_idx, rc);

		path_cfg->top_irq_handle = 0;

		rc = cam_irq_controller_unregister_dependent(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->path_irq_controller[res_id]);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unregister path dependent with top CSID:%u rc: %d",
				csid_hw->hw_intf->hw_idx, rc);
	}
}

static inline void cam_ife_csid_ver2_disable_path_irqs_evts(
	int32_t res_id,
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_ife_csid_ver2_path_cfg *path_cfg)
{
	int rc;

	if (path_cfg->err_irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq_evt(
			csid_hw->path_irq_controller[res_id],
				path_cfg->err_irq_handle);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe path err irq evt CSID:%u rc: %d",
				csid_hw->hw_intf->hw_idx, rc);

		path_cfg->err_irq_handle = 0;
	}

	if (path_cfg->irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq_evt(
			csid_hw->path_irq_controller[res_id],
			path_cfg->irq_handle);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe path info irq evt CSID:%u rc: %d",
				csid_hw->hw_intf->hw_idx, rc);

		path_cfg->irq_handle = 0;
	}

	if (path_cfg->discard_irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq_evt(
			csid_hw->path_irq_controller[res_id],
			path_cfg->discard_irq_handle);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe path discard irq evt CSID:%u rc: %d",
				csid_hw->hw_intf->hw_idx, rc);

		path_cfg->discard_irq_handle = 0;
	}

	if (path_cfg->top_irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq_evt(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			path_cfg->top_irq_handle);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unsubscribe path irq in top evt CSID:%u rc: %d",
				csid_hw->hw_intf->hw_idx, rc);

		path_cfg->top_irq_handle = 0;

		rc =  cam_irq_controller_unregister_dependent(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->path_irq_controller[res_id]);
		if (rc)
			CAM_WARN(CAM_ISP,
				"Failed to unregister path dependent with top CSID:%u rc: %d",
				csid_hw->hw_intf->hw_idx, rc);
	}
}

static int cam_ife_csid_ver2_disable_path(
	bool                             maskout_irqs,
	struct cam_ife_csid_ver2_hw     *csid_hw,
	struct cam_isp_resource_node    *res,
	bool                             is_internal_stop)
{
	struct cam_ife_csid_ver2_path_cfg       *path_cfg;
	int                                      rc = 0;

	if (res->res_state != CAM_ISP_RESOURCE_STATE_STREAMING) {
		CAM_ERR(CAM_ISP,
			"CSID:%u path res type:%d res_id:%d Invalid state:%d",
			csid_hw->hw_intf->hw_idx,
			res->res_type, res->res_id, res->res_state);
		return -EINVAL;
	}

	if (res->res_id >= CAM_IFE_PIX_PATH_RES_MAX) {
		CAM_DBG(CAM_ISP, "CSID:%u Invalid res id%d",
			csid_hw->hw_intf->hw_idx, res->res_id);
		return -EINVAL;
	}

	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;

	if (maskout_irqs)
		cam_ife_csid_ver2_maskout_path_irqs(res->res_id,
			csid_hw, path_cfg);
	else
		cam_ife_csid_ver2_disable_path_irqs_evts(res->res_id,
			csid_hw, path_cfg);

	if (!is_internal_stop)
		path_cfg->allow_epoch_eof_cb = false;

	/* Reset frame drop fields at stream off */
	path_cfg->discard_init_frames = false;
	path_cfg->skip_discard_frame_cfg = false;
	path_cfg->num_frames_discard = 0;
	path_cfg->sof_cnt = 0;
	atomic_set(&path_cfg->switch_out_of_sync_cnt, 0);
	return rc;
}

static bool cam_ife_csid_hw_ver2_need_unpack_mipi(
	struct cam_ife_csid_ver2_hw                  *csid_hw,
	struct cam_csid_hw_reserve_resource_args     *reserve,
	const struct cam_ife_csid_ver2_path_reg_info *path_reg,
	uint32_t                                      format)
{
	bool  need_unpack = false;

	switch(format) {
	case CAM_FORMAT_MIPI_RAW_10:
	case CAM_FORMAT_MIPI_RAW_12:
	case CAM_FORMAT_MIPI_RAW_14:
	 /*
	  * CAM_FORMAT_PLAIN16_16 : can be removed? double check why default_out_format has it.
	  * default_out_format is used in xCFA usecases without real RDI0 out buffer.
	  * We still need to set need_unpack here so that we unpack incoming data (say MIPI10)
	  * into MSB. If default_out_format can be set to 16_10/16_12/16_14 - then we can remove
	  */
	case CAM_FORMAT_PLAIN16_10:
	case CAM_FORMAT_PLAIN16_12:
	case CAM_FORMAT_PLAIN16_14:
	case CAM_FORMAT_PLAIN16_16:
		need_unpack = (bool)(path_reg->capabilities & CAM_IFE_CSID_CAP_RDI_UNPACK_MSB);
		break;
	default:
		need_unpack = false;
		break;
	}

	CAM_DBG(CAM_ISP, "CSID[%u], RDI_%u format %u need_unpack %u sfe_shdr %u",
		csid_hw->hw_intf->hw_idx, reserve->res_id, format, need_unpack,
		reserve->sfe_inline_shdr);

	return need_unpack;
}

static int cam_ife_csid_hw_ver2_prepare_config_path_data(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_ife_csid_ver2_path_cfg *path_cfg,
	struct cam_csid_hw_reserve_resource_args  *reserve,
	uint32_t cid)
{
	int rc = 0, i = 0;
	const struct cam_ife_csid_ver2_reg_info *csid_reg =
		(struct cam_ife_csid_ver2_reg_info *)csid_hw->core_info->csid_reg;
	struct cam_ife_csid_cid_data *cid_data = &csid_hw->cid_data[cid];
	struct cam_isp_resource_node *res = &csid_hw->path_res[reserve->res_id];
	const struct cam_ife_csid_ver2_path_reg_info  *path_reg = NULL;

	for(i = 0; i < reserve->in_port->num_valid_vc_dt; i++)
		path_cfg->in_format[i] = reserve->in_port->format[i];

	path_cfg->cid = cid;
	path_cfg->sync_mode = reserve->sync_mode;
	path_cfg->height  = reserve->in_port->height;
	path_cfg->start_line = reserve->in_port->line_start;
	path_cfg->end_line = reserve->in_port->line_stop;
	path_cfg->crop_enable = reserve->crop_enable;
	path_cfg->drop_enable = reserve->drop_enable;
	path_cfg->horizontal_bin = reserve->in_port->horizontal_bin;
	path_cfg->vertical_bin = reserve->in_port->vertical_bin;
	path_cfg->qcfa_bin = reserve->in_port->qcfa_bin;
	path_cfg->num_bytes_out = reserve->in_port->num_bytes_out;
	path_cfg->sec_evt_config.en_secondary_evt = reserve->sec_evt_config.en_secondary_evt;
	path_cfg->sec_evt_config.evt_type = reserve->sec_evt_config.evt_type;
	path_reg = csid_reg->path_reg[res->res_id];

	if (reserve->out_port)
		path_cfg->out_format = reserve->out_port->format;
	else
		path_cfg->out_format = path_reg->default_out_format;

	if (reserve->sync_mode == CAM_ISP_HW_SYNC_MASTER) {
		path_cfg->start_pixel = reserve->in_port->left_start;
		path_cfg->end_pixel = reserve->in_port->left_stop;
		path_cfg->width  = reserve->in_port->left_width;

		if (reserve->res_id >= CAM_IFE_PIX_PATH_RES_RDI_0 &&
			reserve->res_id <= (CAM_IFE_PIX_PATH_RES_RDI_0 +
			CAM_IFE_CSID_RDI_MAX - 1)) {
			path_cfg->end_pixel = reserve->in_port->right_stop;
			path_cfg->width = path_cfg->end_pixel -
				path_cfg->start_pixel + 1;
		}
		CAM_DBG(CAM_ISP,
			"CSID:%u res:%d master:startpixel 0x%x endpixel:0x%x",
			csid_hw->hw_intf->hw_idx, reserve->res_id,
			path_cfg->start_pixel, path_cfg->end_pixel);
		CAM_DBG(CAM_ISP,
			"CSID:%u res:%d master:line start:0x%x line end:0x%x",
			csid_hw->hw_intf->hw_idx, reserve->res_id,
			path_cfg->start_line, path_cfg->end_line);
	} else if (reserve->sync_mode == CAM_ISP_HW_SYNC_SLAVE) {
		path_cfg->start_pixel = reserve->in_port->right_start;
		path_cfg->end_pixel = reserve->in_port->right_stop;
		path_cfg->width  = reserve->in_port->right_width;
		CAM_DBG(CAM_ISP,
			"CSID:%u res:%d slave:start:0x%x end:0x%x width 0x%x",
			csid_hw->hw_intf->hw_idx, reserve->res_id,
			path_cfg->start_pixel, path_cfg->end_pixel,
			path_cfg->width);
		CAM_DBG(CAM_ISP,
			"CSID:%u res:%d slave:line start:0x%x line end:0x%x",
			csid_hw->hw_intf->hw_idx, reserve->res_id,
			path_cfg->start_line, path_cfg->end_line);
	} else {
		path_cfg->width  = reserve->in_port->left_width;
		path_cfg->start_pixel = reserve->in_port->left_start;
		path_cfg->end_pixel = reserve->in_port->left_stop;
		CAM_DBG(CAM_ISP,
			"CSID:%u res:%d left width %d start: %d stop:%d start line :%d end line %d",
			csid_hw->hw_intf->hw_idx, reserve->res_id,
			reserve->in_port->left_width,
			reserve->in_port->left_start,
			reserve->in_port->left_stop,
			path_cfg->start_line,
			path_cfg->end_line);
	}

	switch (reserve->res_id) {
	case CAM_IFE_PIX_PATH_RES_RDI_0:
	case CAM_IFE_PIX_PATH_RES_RDI_1:
	case CAM_IFE_PIX_PATH_RES_RDI_2:
	case CAM_IFE_PIX_PATH_RES_RDI_3:
	case CAM_IFE_PIX_PATH_RES_RDI_4:
		/*
		 * Skip this if we are not configuring the unpacking at stream on.
		 * Also, we need to make sure wm parameters are not updated in this case
		 * for wm packing.
		 */
		if (!(csid_reg->cmn_reg->capabilities & CAM_IFE_CSID_CAP_SKIP_PATH_CFG1)) {
			path_cfg->csid_out_unpack_msb =
				cam_ife_csid_hw_ver2_need_unpack_mipi(csid_hw, reserve, path_reg,
					path_cfg->out_format);

			/*
			 * if csid gives unpacked msb out, packing needs to be done at
			 * WM side if needed, based on the format the decision is
			 * taken at WM side
			 */
			reserve->use_wm_pack = path_cfg->csid_out_unpack_msb;
		}

		for (i = 0; i < CAM_ISP_VC_DT_CFG; i++) {
			if (cid_data->vc_dt[i].valid) {
				rc = cam_ife_csid_get_format_rdi(
					path_cfg->in_format[i], path_cfg->out_format,
					&path_cfg->path_format[i], path_reg->mipi_pack_supported,
					path_cfg->csid_out_unpack_msb);
				if (rc) {
					CAM_ERR(CAM_ISP,
						"Invalid format cfg at idx: %d for res_id: 0x%x, in_fmt: 0x%x, out_fmt: 0x%x, mipi_pack_supported: %s, csid_unpack_msb: %s",
						i, res->res_id, path_cfg->in_format[i],
						path_cfg->out_format,
						CAM_BOOL_TO_YESNO(path_reg->mipi_pack_supported),
						CAM_BOOL_TO_YESNO(path_cfg->csid_out_unpack_msb));
					goto end;
				}
			}
		}
		break;
	case CAM_IFE_PIX_PATH_RES_IPP:
	case CAM_IFE_PIX_PATH_RES_IPP_1:
	case CAM_IFE_PIX_PATH_RES_IPP_2:
	case CAM_IFE_PIX_PATH_RES_PPP:

		for (i = 0; i < CAM_ISP_VC_DT_CFG; i++) {
			if (cid_data->vc_dt[i].valid) {
				rc = cam_ife_csid_get_format_ipp_ppp(
					path_cfg->in_format[i],
					&path_cfg->path_format[i]);
				if (rc) {
					CAM_ERR(CAM_ISP,
						"Invalid format cfg at idx: %d for res_id: 0x%x, in_fmt: 0x%x",
						i, res->res_id, path_cfg->in_format[i]);
					goto end;
				}
			}
		}

		break;
	default:
		rc = -EINVAL;
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid Res id %u",
			csid_hw->hw_intf->hw_idx, reserve->res_id);
		break;
	}

	rc = cam_ife_csid_ver2_decode_format_validate(csid_hw, res);
	if (rc)
		goto end;

end:
	return rc;
}

static int cam_ife_csid_hw_ver2_prepare_config_rx(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_csid_hw_reserve_resource_args  *reserve)
{

	/*Before calling this function we already validated the
	 * sancitity of in port args. If this function is called
	 * from somewhere else as well, please make sure  to validate the
	 * in_port args before coming here.
	 */
	if (csid_hw->counters.csi2_reserve_cnt) {
		CAM_DBG(CAM_ISP, "CSID %d Rx already reserved cnt %d",
			csid_hw->hw_intf->hw_idx,
			csid_hw->counters.csi2_reserve_cnt);
		csid_hw->counters.csi2_reserve_cnt++;
		return 0;
	}

	csid_hw->rx_cfg.lane_cfg =
		reserve->in_port->lane_cfg;
	csid_hw->rx_cfg.lane_type =
		reserve->in_port->lane_type;
	csid_hw->rx_cfg.lane_num =
		reserve->in_port->lane_num;
	csid_hw->res_type = reserve->in_port->res_type;
	csid_hw->rx_cfg.dynamic_sensor_switch_en =
		reserve->in_port->dynamic_sensor_switch_en;
	if (reserve->in_port->epd_supported)
		csid_hw->rx_cfg.epd_supported = 1;

	switch (reserve->in_port->res_type) {
	case CAM_ISP_IFE_IN_RES_TPG:
		csid_hw->rx_cfg.phy_sel = 0;
		csid_hw->rx_cfg.tpg_mux_sel = 0;
		break;
	case CAM_ISP_IFE_IN_RES_CPHY_TPG_0:
		csid_hw->rx_cfg.tpg_mux_sel = 1;
		csid_hw->rx_cfg.tpg_num_sel = 1;
		break;
	case CAM_ISP_IFE_IN_RES_CPHY_TPG_1:
		csid_hw->rx_cfg.tpg_mux_sel = 1;
		csid_hw->rx_cfg.tpg_num_sel = 2;
		break;
	case CAM_ISP_IFE_IN_RES_CPHY_TPG_2:
		csid_hw->rx_cfg.tpg_mux_sel = 1;
		csid_hw->rx_cfg.tpg_num_sel = 3;
		break;
	default:
		csid_hw->rx_cfg.tpg_mux_sel = 0;
		csid_hw->rx_cfg.phy_sel =
			(reserve->in_port->res_type & 0xFF);
		break;
	}

	csid_hw->counters.csi2_reserve_cnt++;
	CAM_DBG(CAM_ISP,
		"CSID:%u Rx lane param: cfg:%u type:%u num:%u res:%u rx reserve count: %u",
		csid_hw->hw_intf->hw_idx,
		reserve->in_port->lane_cfg, reserve->in_port->lane_type,
		reserve->in_port->lane_num, reserve->in_port->res_type,
		csid_hw->counters.csi2_reserve_cnt);

	return 0;

}

static int cam_ife_csid_ver2_prepare_camif_config(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_csid_hw_reserve_resource_args  *reserve,
	struct cam_ife_csid_ver2_path_cfg *path_cfg)
{
	struct cam_ife_csid_ver2_reg_info *csid_reg;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
		csid_hw->core_info->csid_reg;

	switch (reserve->res_id) {
	case CAM_IFE_PIX_PATH_RES_IPP:
	case CAM_IFE_PIX_PATH_RES_IPP_1:
	case CAM_IFE_PIX_PATH_RES_IPP_2:
	case CAM_IFE_PIX_PATH_RES_RDI_0:
	case CAM_IFE_PIX_PATH_RES_RDI_1:
	case CAM_IFE_PIX_PATH_RES_RDI_2:
	case CAM_IFE_PIX_PATH_RES_RDI_3:
	case CAM_IFE_PIX_PATH_RES_RDI_4:
		path_cfg->epoch_cfg = (path_cfg->end_line  - path_cfg->start_line) *
			csid_reg->cmn_reg->epoch_factor / 100;

		if (path_cfg->epoch_cfg > path_cfg->end_line)
			path_cfg->epoch_cfg = path_cfg->end_line;

		if (path_cfg->horizontal_bin || path_cfg->qcfa_bin)
			path_cfg->epoch_cfg >>= 1;

		CAM_DBG(CAM_ISP, "CSID[%u] res_id: %u epoch factor: 0x%x factor %u",
			csid_hw->hw_intf->hw_idx, reserve->res_id, path_cfg->epoch_cfg,
			csid_reg->cmn_reg->epoch_factor);
		break;
	default:
		CAM_DBG(CAM_ISP, "CSID[%u] No CAMIF epoch update for res: %u",
			csid_hw->hw_intf->hw_idx, reserve->res_id);
		break;
	}

	return 0;
}

int cam_ife_csid_hw_ver2_prepare_hw_cfg(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_ife_csid_ver2_path_cfg *path_cfg,
	struct cam_csid_hw_reserve_resource_args  *reserve,
	uint32_t cid)
{
	int rc = 0;

	rc = cam_ife_csid_hw_ver2_prepare_config_rx(csid_hw, reserve);
	if (rc) {
		CAM_ERR(CAM_ISP, "CSID[%u] rx config failed",
			csid_hw->hw_intf->hw_idx);
		return rc;
	}

	rc = cam_ife_csid_hw_ver2_prepare_config_path_data(csid_hw, path_cfg,
		reserve, cid);
	if (rc) {
		CAM_ERR(CAM_ISP, "CSID[%u] path data config failed",
			csid_hw->hw_intf->hw_idx);
		goto end;
	}

	rc = cam_ife_csid_ver2_prepare_camif_config(csid_hw, reserve, path_cfg);
	if (rc)
		CAM_ERR(CAM_ISP, "CSID[%u] camif config failed",
			csid_hw->hw_intf->hw_idx);

end:
	return rc;
}

static int cam_ife_csid_ver2_in_port_validate(
	struct cam_csid_hw_reserve_resource_args  *reserve,
	struct cam_ife_csid_ver2_hw     *csid_hw)
{
	int rc = 0;

	/* check in port args for RT streams*/
	if (!reserve->is_offline) {
		rc  = cam_ife_csid_check_in_port_args(reserve,
			csid_hw->hw_intf->hw_idx);
		if (rc)
			goto err;
	}

	if (csid_hw->counters.csi2_reserve_cnt) {

		if (csid_hw->token != reserve->cb_priv) {
			CAM_ERR(CAM_ISP,
				"CSID[%u] different Context for res %d rx reserve count: %u",
				csid_hw->hw_intf->hw_idx,
				reserve->res_id, csid_hw->counters.csi2_reserve_cnt);
			rc = -EINVAL;
			goto err;
		}

		if (csid_hw->res_type != reserve->in_port->res_type) {
			CAM_ERR(CAM_ISP,
				"CSID[%u] Invalid res[%d] in_res_type[%d]",
				csid_hw->hw_intf->hw_idx,
				csid_hw->res_type,
				reserve->in_port->res_type);
			rc = -EINVAL;
			goto err;
		}

		if (csid_hw->rx_cfg.lane_cfg !=
			reserve->in_port->lane_cfg  ||
			csid_hw->rx_cfg.lane_type !=
			reserve->in_port->lane_type ||
			csid_hw->rx_cfg.lane_num !=
			reserve->in_port->lane_num) {
			CAM_ERR(CAM_ISP,
				"CSID[%u] lane: num[%d %d] type[%d %d] cfg[%d %d]",
				csid_hw->hw_intf->hw_idx,
				csid_hw->rx_cfg.lane_num,
				reserve->in_port->lane_num,
				csid_hw->rx_cfg.lane_type,
				reserve->in_port->lane_type,
				csid_hw->rx_cfg.lane_cfg,
				reserve->in_port->lane_cfg);
			rc = -EINVAL;
			goto err;
		}
	}

	return rc;
err:
	CAM_ERR(CAM_ISP, "Invalid args CSID[%u] rc %d",
		csid_hw->hw_intf->hw_idx, rc);
	return rc;
}

int cam_ife_csid_ver2_reserve(void *hw_priv,
	void *reserve_args, uint32_t arg_size)
{

	struct cam_ife_csid_ver2_hw     *csid_hw;
	struct cam_hw_info              *hw_info;
	struct cam_isp_resource_node    *res = NULL;
	struct cam_csid_hw_reserve_resource_args  *reserve;
	struct cam_ife_csid_ver2_path_cfg    *path_cfg;
	const struct cam_ife_csid_ver2_reg_info *csid_reg;
	uint32_t cid;
	int rc = 0;

	reserve = (struct cam_csid_hw_reserve_resource_args  *)reserve_args;

	hw_info = (struct cam_hw_info *)hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	res = &csid_hw->path_res[reserve->res_id];
	if (res->res_state != CAM_ISP_RESOURCE_STATE_AVAILABLE) {
		/**
		 * intentionally set as DBG log to since this log gets printed when hw manager
		 * checks if resource is available
		 */
		CAM_DBG(CAM_ISP, "CSID %d Res_id %d state %d",
			csid_hw->hw_intf->hw_idx, reserve->res_id,
			res->res_state);
		return -EBUSY;
	}

	rc = cam_ife_csid_ver2_in_port_validate(reserve, csid_hw);
	if (rc) {
		CAM_ERR(CAM_ISP, "CSID %d Res_id %d port validation failed",
			csid_hw->hw_intf->hw_idx, reserve->res_id);
		return rc;
	}

	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
	if (!path_cfg) {
		CAM_ERR(CAM_ISP,
			"CSID %d Unallocated Res_id %d state %d",
			csid_hw->hw_intf->hw_idx, reserve->res_id,
			res->res_state);
		return -EINVAL;
	}

	rc = cam_ife_csid_cid_reserve(csid_hw->cid_data, &cid,
		csid_hw->hw_intf->hw_idx, reserve);

	if (rc) {
		CAM_ERR(CAM_ISP, "CSID %d Res_id %d state %d invalid cid %d",
			csid_hw->hw_intf->hw_idx, reserve->res_id,
			res->res_state, cid);
		return rc;
	}

	/* Skip rx and csid cfg for offline */
	if (!reserve->is_offline) {
		rc = cam_ife_csid_hw_ver2_prepare_hw_cfg(csid_hw, path_cfg,
			reserve, cid);
		if (rc) {
			CAM_ERR(CAM_ISP, "CSID[%u] res %d hw_cfg fail",
				csid_hw->hw_intf->hw_idx, reserve->res_id);
			goto release;
		}
	}

	/* Acquire ownership */
	rc = cam_vmrm_soc_acquire_resources(csid_hw->hw_info->soc_info.hw_id);
	if (rc) {
		CAM_ERR(CAM_ISP, "CSID[%u] acquire ownership failed",
			csid_hw->hw_intf->hw_idx);
		goto release;
	}

	reserve->node_res = res;
	res->res_state = CAM_ISP_RESOURCE_STATE_RESERVED;
	csid_hw->event_cb = reserve->event_cb;
	csid_hw->tasklet  = reserve->tasklet;
	csid_hw->token  = reserve->cb_priv;
	reserve->buf_done_controller = csid_hw->buf_done_irq_controller;
	reserve->mc_comp_buf_done_controller =
		csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0];
	res->cdm_ops = reserve->cdm_ops;
	csid_hw->flags.sfe_en = reserve->sfe_en;
	path_cfg->sfe_shdr = reserve->sfe_inline_shdr;
	path_cfg->handle_camif_irq = reserve->handle_camif_irq;
	path_cfg->is_aeb_en = reserve->in_port->aeb_mode;
	csid_hw->flags.offline_mode = reserve->is_offline;
	reserve->need_top_cfg = csid_reg->need_top_cfg;
	reserve->dynamic_drv_supported = csid_reg->dynamic_drv_supported;

	CAM_DBG(CAM_ISP, "CSID[%u] Resource[id: %d name:%s] state %d cid %d",
		csid_hw->hw_intf->hw_idx, reserve->res_id, res->res_name,
		res->res_state, cid);

	return rc;

release:
	cam_ife_csid_cid_release(&csid_hw->cid_data[cid], csid_hw->hw_intf->hw_idx, path_cfg->cid);
	cam_ife_csid_ver2_reset_csid_params(csid_hw);
	return rc;
}

int cam_ife_csid_ver2_release(void *hw_priv,
	void *release_args, uint32_t arg_size)
{
	struct cam_ife_csid_ver2_hw     *csid_hw;
	struct cam_hw_info              *hw_info;
	struct cam_isp_resource_node    *res = NULL;
	struct cam_ife_csid_ver2_path_cfg    *path_cfg;
	int rc = 0;

	if (!hw_priv || !release_args ||
		(arg_size != sizeof(struct cam_isp_resource_node))) {
		CAM_ERR(CAM_ISP, "CSID: Invalid args");
		return -EINVAL;
	}

	hw_info = (struct cam_hw_info *)hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	res = (struct cam_isp_resource_node *)release_args;

	if (res->res_type != CAM_ISP_RESOURCE_PIX_PATH) {
		CAM_ERR(CAM_ISP, "CSID:%u Invalid res type:%d res id%d",
			csid_hw->hw_intf->hw_idx, res->res_type,
			res->res_id);
		return -EINVAL;
	}

	mutex_lock(&csid_hw->hw_info->hw_mutex);

	if ((res->res_type == CAM_ISP_RESOURCE_PIX_PATH &&
		res->res_id >= CAM_IFE_PIX_PATH_RES_MAX)) {
		CAM_ERR(CAM_ISP, "CSID:%u Invalid res type:%d res id%d",
			csid_hw->hw_intf->hw_idx, res->res_type,
			res->res_id);
		rc = -EINVAL;
		goto end;
	}

	if ((res->res_state <= CAM_ISP_RESOURCE_STATE_AVAILABLE) ||
		(res->res_state >= CAM_ISP_RESOURCE_STATE_STREAMING)) {
		CAM_WARN(CAM_ISP,
			"CSID:%u res type:%d Res %d in state %d",
			csid_hw->hw_intf->hw_idx,
			res->res_type, res->res_id,
			res->res_state);
		goto end;
	}

	CAM_DBG(CAM_ISP, "CSID:%u res type :%d Resource [id:%d name:%s] rx reserve count: %u",
		csid_hw->hw_intf->hw_idx, res->res_type,
		res->res_id, res->res_name, csid_hw->counters.csi2_reserve_cnt);

	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;

	if (path_cfg->cid >= CAM_IFE_CSID_CID_MAX) {
		CAM_ERR(CAM_ISP, "CSID:%d Invalid cid:%d",
			csid_hw->hw_intf->hw_idx, path_cfg->cid);
		rc = -EINVAL;
		goto end;
	}

	cam_ife_csid_cid_release(&csid_hw->cid_data[path_cfg->cid],
		csid_hw->hw_intf->hw_idx,
		path_cfg->cid);

	memset(path_cfg, 0, sizeof(*path_cfg));

	csid_hw->sync_mode = CAM_ISP_HW_SYNC_NONE;

	if (csid_hw->counters.csi2_reserve_cnt)
		csid_hw->counters.csi2_reserve_cnt--;

	if (!csid_hw->counters.csi2_reserve_cnt)
		cam_ife_csid_ver2_reset_csid_params(csid_hw);

	res->res_state = CAM_ISP_RESOURCE_STATE_AVAILABLE;

	rc = cam_vmrm_soc_release_resources(csid_hw->hw_info->soc_info.hw_id);
	if (rc) {
		CAM_ERR(CAM_ISP, "CSID[%u] vmrm soc release resources failed",
			csid_hw->hw_intf->hw_idx);
	}

end:
	mutex_unlock(&csid_hw->hw_info->hw_mutex);
	return rc;
}

static bool cam_ife_csid_hw_ver2_use_master_slave_cfg(
		struct cam_ife_csid_ver2_hw *csid_hw,
		uint32_t res_id)
{
	struct cam_ife_csid_ver2_reg_info *csid_reg;
	bool                               ret = false;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = NULL;
	struct cam_ife_csid_ver2_path_cfg *path_cfg;
	struct cam_isp_resource_node    *res = NULL;
	struct cam_isp_resource_node    *ppp_res = NULL;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)csid_hw->core_info->csid_reg;
	path_reg = csid_reg->path_reg[res_id];
	res = &csid_hw->path_res[res_id];
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;

	switch (res_id) {
	case CAM_IFE_PIX_PATH_RES_RDI_0:
		if (path_cfg->is_aeb_en || path_cfg->sfe_shdr || csid_hw->flags.rdi_lcr_en)
			ret = true;

		if (path_reg->use_master_slave_default) {
			ppp_res = &csid_hw->path_res[CAM_IFE_PIX_PATH_RES_PPP];
			ret = ppp_res->res_state > CAM_ISP_RESOURCE_STATE_AVAILABLE ? true : false;
		}

		break;
	case CAM_IFE_PIX_PATH_RES_RDI_1:
	case CAM_IFE_PIX_PATH_RES_RDI_2:
	case CAM_IFE_PIX_PATH_RES_RDI_3:
		if (path_cfg->is_aeb_en || path_cfg->sfe_shdr)
			ret = true;
		break;
	case CAM_IFE_PIX_PATH_RES_PPP:
		if (csid_hw->flags.rdi_lcr_en || path_reg->use_master_slave_default)
			ret = true;
		break;
	default:
		break;
	}

	CAM_DBG(CAM_ISP, "CSID[%u] %s use master slave cfg %s", csid_hw->hw_intf->hw_idx,
		res->res_name, CAM_BOOL_TO_YESNO(ret));
	return ret;
}

static void cam_ife_csid_ver2_res_master_slave_cfg(struct cam_ife_csid_ver2_hw *csid_hw,
	uint32_t res_id)
{
	struct cam_ife_csid_ver2_reg_info *csid_reg;
	uint32_t val;
	void __iomem                      *mem_base;
	struct cam_hw_soc_info            *soc_info;


	if (!cam_ife_csid_hw_ver2_use_master_slave_cfg(csid_hw, res_id))
		return;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *) csid_hw->core_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;
	val = cam_io_r_mb(mem_base + csid_reg->cmn_reg->shdr_master_slave_cfg_addr);

	switch (res_id) {
	case CAM_IFE_PIX_PATH_RES_RDI_0:
		val |= BIT(csid_reg->cmn_reg->shdr_master_rdi0_shift);
		break;
	case CAM_IFE_PIX_PATH_RES_RDI_1:
		val |= BIT(csid_reg->cmn_reg->shdr_slave_rdi1_shift);
		break;
	case CAM_IFE_PIX_PATH_RES_RDI_2:
		val |= BIT(csid_reg->cmn_reg->shdr_slave_rdi2_shift);
		break;
	case CAM_IFE_PIX_PATH_RES_PPP:
		val |= BIT(csid_reg->cmn_reg->shdr_slave_ppp_shift);
		break;
	default:
		break;
	}

	val |= BIT(csid_reg->cmn_reg->shdr_master_slave_en_shift);

	cam_io_w_mb(val, mem_base + csid_reg->cmn_reg->shdr_master_slave_cfg_addr);

	CAM_DBG(CAM_ISP, "CSID %d res:%s master slave cfg 0x%x", csid_hw->hw_intf->hw_idx,
		csid_hw->path_res[res_id].res_name, val);
}

static int cam_ife_csid_ver2_init_config_rdi_path(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_isp_resource_node    *res)
{
	int rc = 0;
	const struct cam_ife_csid_ver2_reg_info *csid_reg;
	struct cam_hw_soc_info                   *soc_info;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = NULL;
	const struct cam_ife_csid_ver2_common_reg_info *cmn_reg = NULL;
	uint32_t  val, cfg0 = 0, cfg1 = 0;
	struct cam_ife_csid_ver2_path_cfg *path_cfg;
	struct cam_ife_csid_cid_data *cid_data;
	void __iomem *mem_base;

	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	if (!csid_reg->path_reg[res->res_id]) {
		CAM_ERR(CAM_ISP, "CSID:%u RDI:%d is not supported on HW",
			 csid_hw->hw_intf->hw_idx, res->res_id);
		return -EINVAL;
	}

	cmn_reg = csid_reg->cmn_reg;
	path_reg = csid_reg->path_reg[res->res_id];
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
	cid_data = &csid_hw->cid_data[path_cfg->cid];
	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	/* Enable client & cfg offline mode */
	if (csid_hw->flags.offline_mode) {
		val = (1 <<
			path_reg->offline_mode_en_shift_val);
		val |= (1 << cmn_reg->path_en_shift_val);
		cam_io_w_mb(val, mem_base + path_reg->cfg0_addr);
		CAM_DBG(CAM_ISP, "CSID:%u enabling res:%s cfg0: 0x%x for offline",
			csid_hw->hw_intf->hw_idx, res->res_name, val);
		return 0;
	}

	/*Configure cfg0:
	 * VC
	 * DT
	 * Timestamp enable and strobe selection for v780
	 * DT_ID cobination
	 * Decode Format
	 * Frame_id_dec_en
	 * VFR en
	 * offline mode
	 */
	cfg0 = (cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_0].vc <<
			cmn_reg->vc_shift_val) |
		(cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_0].dt <<
			cmn_reg->dt_shift_val) |
		(path_cfg->path_format[CAM_IFE_CSID_MULTI_VC_DT_GRP_0].decode_fmt <<
		 	cmn_reg->decode_format_shift_val);

	if (!csid_reg->cmn_reg->direct_cid_config)
		cfg0 |= (path_cfg->cid << cmn_reg->dt_id_shift_val);

	if (csid_reg->cmn_reg->vfr_supported)
		cfg0 |= path_cfg->vfr_en << cmn_reg->vfr_en_shift_val;

	if (csid_reg->cmn_reg->frame_id_dec_supported)
		cfg0 |= path_cfg->frame_id_dec_en <<
			cmn_reg->frame_id_decode_en_shift_val;

	if (cmn_reg->timestamp_enabled_in_cfg0)
		cfg0 |= (1 << path_reg->timestamp_en_shift_val) |
			(cmn_reg->timestamp_strobe_val <<
				cmn_reg->timestamp_stb_sel_shift_val);

	if (cam_ife_csid_ver2_disable_sof_retime(csid_hw, res))
		cfg0 |= 1 << path_reg->sof_retiming_dis_shift;

	cam_io_w_mb(cfg0, mem_base + path_reg->cfg0_addr);
	CAM_DBG(CAM_ISP, "CSID[%u] res:%s cfg0: 0x%x",
		csid_hw->hw_intf->hw_idx, res->res_name, cfg0);

	path_cfg->ts_comb_vcdt_en = false;

	/*Configure Multi VC DT combo */
	if (cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_1].valid) {
		val = (cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_1].vc <<
				cmn_reg->multi_vcdt_vc1_shift_val) |
			(cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_1].dt <<
				 cmn_reg->multi_vcdt_dt1_shift_val) |
			(1 << cmn_reg->multi_vcdt_en_shift_val);

		if (cmn_reg->ts_comb_vcdt_en) {
			val |= (1 << cmn_reg->multi_vcdt_ts_combo_en_shift_val);
			path_cfg->ts_comb_vcdt_en = true;
		}

		if (cmn_reg->decode_format1_supported)
			val |= (path_cfg->path_format[CAM_IFE_CSID_MULTI_VC_DT_GRP_1].decode_fmt <<
				cmn_reg->decode_format1_shift_val);

		cam_io_w_mb(val, mem_base + path_reg->multi_vcdt_cfg0_addr);
		CAM_DBG(CAM_ISP, "CSID:%u res:%s multi_vcdt_cfg0: 0x%x",
			csid_hw->hw_intf->hw_idx, res->res_name, val);
	}

	if (cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_2].valid) {
		val = ((cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_2].dt <<
				 cmn_reg->multi_vcdt_dt2_shift_val)) |
			((path_cfg->path_format[CAM_IFE_CSID_MULTI_VC_DT_GRP_2].decode_fmt <<
				cmn_reg->decode_format2_shift_val));

		if (cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_3].valid) {
			val |= ((cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_3].dt <<
				 cmn_reg->multi_vcdt_dt3_shift_val)) |
				 ((path_cfg->path_format[
					CAM_IFE_CSID_MULTI_VC_DT_GRP_3].decode_fmt <<
					cmn_reg->decode_format3_shift_val));
		}

		cam_io_w_mb(val, mem_base + path_reg->multi_vcdt_cfg1_addr);

		CAM_DBG(CAM_ISP, "CSID:%u res:%s multi_vcdt_cfg1: 0x%x",
			csid_hw->hw_intf->hw_idx, res->res_name, val);
	}

	/* For some targets, cfg1 register is programmed as part of cdm buffer, so skip here */
	if (!(csid_reg->cmn_reg->capabilities & CAM_IFE_CSID_CAP_SKIP_PATH_CFG1)) {

		/*configure cfg1 addr
		 * Crop/Drop parameters
		 * Timestamp enable and strobe selection
		 * Plain format
		 * Packing format
		 */
		cfg1 = (path_cfg->crop_enable << path_reg->crop_h_en_shift_val) |
			(path_cfg->crop_enable <<
			 path_reg->crop_v_en_shift_val);

		if (cmn_reg->drop_supported)
			cfg1 |= (path_cfg->drop_enable <<
					path_reg->drop_v_en_shift_val) |
				(path_cfg->drop_enable <<
					path_reg->drop_h_en_shift_val);

		if (path_reg->mipi_pack_supported)
			cfg1 |= path_cfg->path_format[CAM_IFE_CSID_MULTI_VC_DT_GRP_0].packing_fmt <<
				path_reg->packing_fmt_shift_val;

		cfg1 |= (path_cfg->path_format[CAM_IFE_CSID_MULTI_VC_DT_GRP_0].plain_fmt <<
				path_reg->plain_fmt_shift_val);

		/* Keep the data in MSB, IFE/SFE  pipeline, BUS expects data in MSB */
		if (path_cfg->csid_out_unpack_msb &&
			path_cfg->path_format[CAM_IFE_CSID_MULTI_VC_DT_GRP_0].plain_fmt)
			cfg1 |= (1 << path_reg->plain_alignment_shift_val);

		if (csid_hw->debug_info.debug_val &
			CAM_IFE_CSID_DEBUG_ENABLE_HBI_VBI_INFO)
			cfg1 |= 1 << path_reg->format_measure_en_shift_val;

		if (!cmn_reg->timestamp_enabled_in_cfg0)
			cfg1 |= (1 << path_reg->timestamp_en_shift_val) |
				(cmn_reg->timestamp_strobe_val <<
					cmn_reg->timestamp_stb_sel_shift_val);

		/* We use line smoothting only on RDI_0 in all usecases */
		if ((path_reg->capabilities &
			CAM_IFE_CSID_CAP_LINE_SMOOTHING_IN_RDI) &&
			(res->res_id == CAM_IFE_PIX_PATH_RES_RDI_0))
			cfg1 |= 1 << path_reg->pix_store_en_shift_val;

		cam_io_w_mb(cfg1, mem_base + path_reg->cfg1_addr);
		CAM_DBG(CAM_ISP, "CSID:%u res:%s cfg1: 0x%x",
			csid_hw->hw_intf->hw_idx, res->res_name, cfg1);
	}

	/* Enable the RDI path */
	cfg0 |= (1 << cmn_reg->path_en_shift_val);
	cam_io_w_mb(cfg0, mem_base + path_reg->cfg0_addr);
	CAM_DBG(CAM_ISP, "CSID[%u] enabling path: %s cfg0: 0x%x",
			csid_hw->hw_intf->hw_idx, res->res_name, cfg0);

	if (path_reg->overflow_ctrl_en) {
		val = path_reg->overflow_ctrl_en |
			path_reg->overflow_ctrl_mode_val;
		cam_io_w_mb(val, mem_base +
			path_reg->err_recovery_cfg0_addr);
	}

	cam_ife_csid_ver2_res_master_slave_cfg(csid_hw, res->res_id);

	if (csid_hw->debug_info.debug_val &
		CAM_IFE_CSID_DEBUG_ENABLE_HBI_VBI_INFO) {
		val = cam_io_r_mb(mem_base +
			path_reg->format_measure_cfg0_addr);
		val |= cmn_reg->measure_en_hbi_vbi_cnt_mask;
		cam_io_w_mb(val, mem_base +
			path_reg->format_measure_cfg0_addr);
	}

	return rc;
}

static int cam_ife_csid_ver2_init_config_pxl_path(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_isp_resource_node    *res)
{
	int rc = 0;
	const struct cam_ife_csid_ver2_reg_info *csid_reg;
	struct cam_hw_soc_info                   *soc_info;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = NULL;
	const struct cam_ife_csid_ver2_common_reg_info *cmn_reg = NULL;
	uint32_t val = 0, cfg0 = 0, cfg1 = 0;
	struct cam_ife_csid_ver2_path_cfg *path_cfg;
	struct cam_ife_csid_cid_data *cid_data;
	void __iomem *mem_base;

	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	path_reg = csid_reg->path_reg[res->res_id];

	if (!path_reg) {
		CAM_ERR(CAM_ISP,
			"CSID:%u path res reg not found, type:%d res_id:%d res state %d",
			csid_hw->hw_intf->hw_idx,
			res->res_type, res->res_id, res->res_state);
		return -EINVAL;
	}

	cmn_reg = csid_reg->cmn_reg;
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
	cid_data = &csid_hw->cid_data[path_cfg->cid];
	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	/*Configure:
	 * VC
	 * DT
	 * Timestamp enable and strobe selection
	 * DT_ID cobination
	 * Decode Format
	 * Frame_id_dec_en
	 * VFR en
	 */
	cfg0 |= (cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_0].vc <<
			cmn_reg->vc_shift_val) |
		(cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_0].dt <<
			cmn_reg->dt_shift_val) |
		(path_cfg->cid << cmn_reg->dt_id_shift_val) |
		(path_cfg->path_format[CAM_IFE_CSID_MULTI_VC_DT_GRP_0].decode_fmt <<
		 	cmn_reg->decode_format_shift_val);

	if (csid_reg->cmn_reg->vfr_supported)
		cfg0 |= path_cfg->vfr_en << cmn_reg->vfr_en_shift_val;

	if (csid_reg->cmn_reg->frame_id_dec_supported)
		cfg0 |= path_cfg->frame_id_dec_en <<
			cmn_reg->frame_id_decode_en_shift_val;

	if (cmn_reg->timestamp_enabled_in_cfg0)
		cfg0 |= (1 << path_reg->timestamp_en_shift_val) |
			(cmn_reg->timestamp_strobe_val <<
				cmn_reg->timestamp_stb_sel_shift_val);

	if (cam_ife_csid_ver2_disable_sof_retime(csid_hw, res))
		cfg0 |= 1 << path_reg->sof_retiming_dis_shift;

	cam_io_w_mb(cfg0, mem_base + path_reg->cfg0_addr);
	CAM_DBG(CAM_ISP, "CSID[%u] res:%s cfg0: 0x%x",
		csid_hw->hw_intf->hw_idx, res->res_name, cfg0);

	path_cfg->ts_comb_vcdt_en = false;

	/*Configure Multi VC DT combo */
	if (cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_1].valid) {
		val = (cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_1].vc <<
				cmn_reg->multi_vcdt_vc1_shift_val) |
			(cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_1].dt <<
				 cmn_reg->multi_vcdt_dt1_shift_val) |
			(1 << cmn_reg->multi_vcdt_en_shift_val);

		if (cmn_reg->ts_comb_vcdt_en) {
			val |= (1 << cmn_reg->multi_vcdt_ts_combo_en_shift_val);
			path_cfg->ts_comb_vcdt_en = true;
		}

		if (cmn_reg->decode_format1_supported)
			val |= (path_cfg->path_format[CAM_IFE_CSID_MULTI_VC_DT_GRP_1].decode_fmt <<
				cmn_reg->decode_format1_shift_val);

		cam_io_w_mb(val, mem_base + path_reg->multi_vcdt_cfg0_addr);
		CAM_DBG(CAM_ISP, "CSID:%u res:%s multi_vcdt_cfg0: 0x%x",
			csid_hw->hw_intf->hw_idx, res->res_name, val);
	}

	if (cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_2].valid) {
		val = ((cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_2].dt <<
				 cmn_reg->multi_vcdt_dt2_shift_val)) |
			((path_cfg->path_format[CAM_IFE_CSID_MULTI_VC_DT_GRP_2].decode_fmt <<
				cmn_reg->decode_format2_shift_val));

		if (cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_3].valid) {
			val |= ((cid_data->vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_3].dt <<
				 cmn_reg->multi_vcdt_dt3_shift_val)) |
				 ((path_cfg->path_format[
					CAM_IFE_CSID_MULTI_VC_DT_GRP_3].decode_fmt <<
					cmn_reg->decode_format3_shift_val));
		}

		cam_io_w_mb(val, mem_base + path_reg->multi_vcdt_cfg1_addr);

		CAM_DBG(CAM_ISP, "CSID:%u res:%s multi_vcdt_cfg1: 0x%x",
			csid_hw->hw_intf->hw_idx, res->res_name, val);
	}

	/* For some targets, cfg1 register is programmed as part of cdm buffer, so skip here */
	if (!(csid_reg->cmn_reg->capabilities & CAM_IFE_CSID_CAP_SKIP_PATH_CFG1)) {

		/*configure cfg1 addr
		 * Binning
		 * Crop/Drop parameters
		 * Early Eof
		 * Timestamp enable and strobe selection
		 * Pix store enable
		 */

		if (csid_hw->flags.binning_enabled) {

			if (path_reg->binning_supported & CAM_IFE_CSID_BIN_HORIZONTAL)
				cfg1 |= path_cfg->horizontal_bin <<
					path_reg->bin_h_en_shift_val;

			if (path_reg->binning_supported & CAM_IFE_CSID_BIN_VERTICAL)
				cfg1 |= path_cfg->vertical_bin <<
					path_reg->bin_v_en_shift_val;

			if (path_reg->binning_supported & CAM_IFE_CSID_BIN_QCFA)
				cfg1 |= path_cfg->qcfa_bin <<
					path_reg->bin_qcfa_en_shift_val;

			if (path_cfg->qcfa_bin || path_cfg->vertical_bin ||
					path_cfg->horizontal_bin)
				cfg1 |= 1  << path_reg->bin_en_shift_val;
		}

		cfg1 |= (path_cfg->crop_enable << path_reg->crop_h_en_shift_val) |
			(path_cfg->crop_enable <<
			 path_reg->crop_v_en_shift_val);

		if (cmn_reg->drop_supported)
			cfg1 |= (path_cfg->drop_enable <<
					path_reg->drop_v_en_shift_val) |
				(path_cfg->drop_enable <<
					path_reg->drop_h_en_shift_val);

		cfg1 |= 1 << path_reg->pix_store_en_shift_val;

		/*enable early eof based on crop enable */
		if (!(csid_hw->debug_info.debug_val &
				CAM_IFE_CSID_DEBUG_DISABLE_EARLY_EOF) &&
			cmn_reg->early_eof_supported && path_cfg->crop_enable &&
			!(csid_hw->flags.rdi_lcr_en && res->res_id == CAM_IFE_PIX_PATH_RES_PPP))
			cfg1 |= (1 << path_reg->early_eof_en_shift_val);

		if (csid_hw->debug_info.debug_val &
			CAM_IFE_CSID_DEBUG_ENABLE_HBI_VBI_INFO)
			cfg1 |= 1 << path_reg->format_measure_en_shift_val;

		if (!cmn_reg->timestamp_enabled_in_cfg0)
			cfg1 |= (1 << path_reg->timestamp_en_shift_val) |
				(cmn_reg->timestamp_strobe_val <<
					cmn_reg->timestamp_stb_sel_shift_val);

		cam_io_w_mb(cfg1, mem_base + path_reg->cfg1_addr);
		CAM_DBG(CAM_ISP, "CSID[%u] res:%s cfg1: 0x%x",
			csid_hw->hw_intf->hw_idx, res->res_name, cfg1);
	}

	/* Enable the Pxl path */
	cfg0 |= (1 << cmn_reg->path_en_shift_val);
	cam_io_w_mb(cfg0, mem_base + path_reg->cfg0_addr);
	CAM_DBG(CAM_ISP, "CSID[%u] enabling path: %s cfg0: 0x%x",
		csid_hw->hw_intf->hw_idx, res->res_name, cfg0);

	if (path_reg->overflow_ctrl_en) {
		val = path_reg->overflow_ctrl_en |
			path_reg->overflow_ctrl_mode_val;
		cam_io_w_mb(val, mem_base + path_reg->err_recovery_cfg0_addr);
	}

	if (csid_hw->debug_info.debug_val &
		CAM_IFE_CSID_DEBUG_ENABLE_HBI_VBI_INFO) {
		val = cam_io_r_mb(mem_base +
			path_reg->format_measure_cfg0_addr);
		val |= csid_reg->cmn_reg->measure_en_hbi_vbi_cnt_mask;
		cam_io_w_mb(val,
			mem_base + path_reg->format_measure_cfg0_addr);
	}

	cam_ife_csid_ver2_res_master_slave_cfg(csid_hw, res->res_id);

	res->res_state = CAM_ISP_RESOURCE_STATE_INIT_HW;
	return rc;
}

static inline __attribute_const__ CAM_IRQ_HANDLER_BOTTOM_HALF
cam_ife_csid_ver2_get_path_bh(int res_id)
{
	switch (res_id) {
	case CAM_IFE_PIX_PATH_RES_RDI_0:
	case CAM_IFE_PIX_PATH_RES_RDI_1:
	case CAM_IFE_PIX_PATH_RES_RDI_2:
	case CAM_IFE_PIX_PATH_RES_RDI_3:
	case CAM_IFE_PIX_PATH_RES_RDI_4:
		return cam_ife_csid_ver2_rdi_bottom_half;
	case CAM_IFE_PIX_PATH_RES_IPP:
	case CAM_IFE_PIX_PATH_RES_IPP_1:
	case CAM_IFE_PIX_PATH_RES_IPP_2:
		return cam_ife_csid_ver2_ipp_bottom_half;
	case CAM_IFE_PIX_PATH_RES_PPP:
		return cam_ife_csid_ver2_ppp_bottom_half;
	default:
		return NULL;
	}
}

static inline int cam_ife_csid_ver2_subscribe_sof_for_discard(
	struct cam_ife_csid_ver2_path_cfg *path_cfg,
	struct cam_ife_csid_ver2_hw       *csid_hw,
	struct cam_isp_resource_node      *res,
	CAM_IRQ_HANDLER_TOP_HALF           top_half_handler,
	CAM_IRQ_HANDLER_BOTTOM_HALF        bottom_half_handler)
{
	int rc = 0;
	uint32_t irq_mask = 0;

	irq_mask = IFE_CSID_VER2_PATH_INFO_INPUT_SOF;
	path_cfg->discard_irq_handle = cam_irq_controller_subscribe_irq(
		csid_hw->path_irq_controller[res->res_id],
		CAM_IRQ_PRIORITY_0,
		&irq_mask,
		res,
		top_half_handler,
		bottom_half_handler,
		csid_hw->tasklet,
		&tasklet_bh_api,
		CAM_IRQ_EVT_GROUP_0);

	if (path_cfg->discard_irq_handle < 1) {
		CAM_ERR(CAM_ISP,
			"CSID[%u] Subscribing input SOF failed for discarding %d",
			csid_hw->hw_intf->hw_idx, res->res_id);
		rc = -EINVAL;
	}

	CAM_DBG(CAM_ISP,
		"CSID[%u] Subscribing input SOF for discard done res: %s rc: %d",
		csid_hw->hw_intf->hw_idx, res->res_name, rc);
	return rc;
}

static int cam_ife_csid_ver2_mc_irq_subscribe(struct cam_ife_csid_ver2_hw  *csid_hw,
	struct cam_isp_resource_node *res, uint32_t top_index)
{
	uint32_t tmp_mask;
	struct cam_ife_csid_ver2_reg_info *csid_reg = csid_hw->core_info->csid_reg;

	if (csid_hw->top_mc_irq_handle >= 1)
		return 0;

	/* For multi context cases, we only need composite epoch and eof */
	tmp_mask = csid_reg->ipp_mc_reg->comp_eof_mask | csid_reg->ipp_mc_reg->comp_epoch0_mask;

	csid_hw->top_mc_irq_handle = cam_irq_controller_subscribe_irq(
		csid_hw->top_irq_controller[top_index],
		CAM_IRQ_PRIORITY_0,
		&tmp_mask,
		res,
		cam_ife_csid_ver2_mc_top_half,
		cam_ife_csid_ver2_get_path_bh(res->res_id),
		csid_hw->tasklet,
		&tasklet_bh_api,
		CAM_IRQ_EVT_GROUP_0);

	if (csid_hw->top_mc_irq_handle < 1) {
		CAM_ERR(CAM_ISP, "csid[%d] MC subscribe top irq fail %s",
			csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	return 0;

}

static int cam_ife_csid_ver2_path_irq_subscribe(
	struct cam_ife_csid_ver2_hw  *csid_hw,
	struct cam_isp_resource_node *res,
	uint32_t dbg_frm_irq_mask, uint32_t err_irq_mask)
{
	uint32_t num_register = 0;
	uint32_t *top_irq_mask = NULL;
	struct cam_ife_csid_ver2_path_cfg *path_cfg = res->res_priv;
	struct cam_ife_csid_ver2_reg_info *csid_reg = csid_hw->core_info->csid_reg;
	int i, rc;
	int top_index = -1;

	for (i = CAM_IFE_CSID_TOP_IRQ_STATUS_REG0; i < csid_reg->num_top_regs; i++) {
		if (csid_reg->path_reg[res->res_id]->top_irq_mask[i]) {
			top_index = i;
			break;
		}
	}

	if (top_index < 0 || top_index >= CAM_IFE_CSID_TOP_IRQ_STATUS_REG_MAX) {
		CAM_ERR(CAM_ISP, "csid[%d] %s Invalid top index %s index %d",
			csid_hw->hw_intf->hw_idx, res->res_name, top_index);
		return -EINVAL;
	}

	num_register = csid_reg->top_irq_reg_info[top_index].num_registers;
	top_irq_mask = vmalloc(sizeof(uint32_t) * num_register);
	if (!top_irq_mask) {
		CAM_ERR(CAM_ISP, "csid top_irq_mask allocation failed");
		return -ENOMEM;
	}

	top_irq_mask[0] = csid_reg->path_reg[res->res_id]->top_irq_mask[top_index];

	if ((res->res_id == CAM_IFE_PIX_PATH_RES_IPP) &&
		csid_reg->path_reg[res->res_id]->capabilities &
		CAM_IFE_CSID_CAP_MULTI_CTXT) {
		rc = cam_ife_csid_ver2_mc_irq_subscribe(csid_hw, res, top_index);
		if (rc || csid_hw->top_mc_irq_handle < 1) {
			CAM_ERR(CAM_ISP, "CSID[%u] Failed to subscribe MC top irq",
					csid_hw->hw_intf->hw_idx);
			goto end;
		}
	}

	path_cfg->top_irq_handle = cam_irq_controller_subscribe_irq(
		csid_hw->top_irq_controller[top_index],
		CAM_IRQ_PRIORITY_0,
		top_irq_mask,
		res,
		cam_ife_csid_ver2_handle_path_irq,
		NULL, NULL, NULL, CAM_IRQ_EVT_GROUP_0);

	if (path_cfg->top_irq_handle < 1) {
		CAM_ERR(CAM_ISP, "CSID[%u] subscribe top irq fail %s",
			csid_hw->hw_intf->hw_idx, res->res_name);
		rc = -EINVAL;
		goto unsub_mc;
	}

	rc = cam_irq_controller_register_dependent(csid_hw->top_irq_controller[top_index],
		csid_hw->path_irq_controller[res->res_id], top_irq_mask);

	if (rc)
		goto unsub_top;

	path_cfg->irq_handle = cam_irq_controller_subscribe_irq(
		csid_hw->path_irq_controller[res->res_id],
		CAM_IRQ_PRIORITY_1,
		&dbg_frm_irq_mask,
		res,
		cam_ife_csid_ver2_path_top_half,
		cam_ife_csid_ver2_get_path_bh(res->res_id),
		csid_hw->tasklet,
		&tasklet_bh_api,
		CAM_IRQ_EVT_GROUP_0);

	if (path_cfg->irq_handle < 1) {
		CAM_ERR(CAM_ISP, "CSID[%u] subscribe path irq fail %s",
			csid_hw->hw_intf->hw_idx, res->res_name);
		rc = -EINVAL;
		goto unreg_path;
	}

	path_cfg->err_irq_handle = cam_irq_controller_subscribe_irq(
			csid_hw->path_irq_controller[res->res_id],
			CAM_IRQ_PRIORITY_0,
			&err_irq_mask,
			res,
			cam_ife_csid_ver2_path_err_top_half,
			cam_ife_csid_ver2_get_path_bh(res->res_id),
			csid_hw->tasklet,
			&tasklet_bh_api,
			CAM_IRQ_EVT_GROUP_0);

	if (path_cfg->err_irq_handle < 1) {
		CAM_ERR(CAM_ISP, "CSID[%u] subscribe error irq fail %s",
			csid_hw->hw_intf->hw_idx, res->res_name);
		rc = -EINVAL;
		goto unsub_path;
	}
	vfree(top_irq_mask);
	return 0;

unsub_path:
	cam_irq_controller_unsubscribe_irq(csid_hw->path_irq_controller[res->res_id],
			path_cfg->irq_handle);
	path_cfg->irq_handle = 0;
unreg_path:
	cam_irq_controller_unregister_dependent(csid_hw->top_irq_controller[top_index],
		csid_hw->path_irq_controller[res->res_id]);
unsub_top:
	cam_irq_controller_unsubscribe_irq(csid_hw->top_irq_controller[top_index],
			path_cfg->top_irq_handle);
	path_cfg->top_irq_handle = 0;
unsub_mc:
	cam_irq_controller_unsubscribe_irq(csid_hw->top_irq_controller[top_index],
		csid_hw->top_mc_irq_handle);
	csid_hw->top_mc_irq_handle = 0;
end:
	vfree(top_irq_mask);
	return rc;
}

static void  cam_ife_csid_ver2_path_rup_aup(
	struct cam_ife_csid_ver2_hw           *csid_hw,
	struct cam_isp_resource_node          *res,
	struct cam_ife_csid_ver2_rup_aup_mask *rup_aup_mask)
{
	struct cam_ife_csid_ver2_reg_info *csid_reg;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	path_reg = csid_reg->path_reg[res->res_id];

	if (csid_reg->cmn_reg->capabilities & CAM_IFE_CSID_CAP_SPLIT_RUP_AUP) {
		rup_aup_mask->rup_mask |= path_reg->rup_mask;
		rup_aup_mask->aup_mask |= path_reg->aup_mask;
	} else {
		rup_aup_mask->rup_mask |= path_reg->rup_aup_mask;
	}

}

static int cam_ife_csid_ver2_program_rdi_path(
	struct cam_ife_csid_ver2_hw           *csid_hw,
	struct cam_isp_resource_node          *res,
	struct cam_ife_csid_ver2_rup_aup_mask *rup_aup_mask)
{
	int rc = 0;
	struct cam_ife_csid_ver2_reg_info *csid_reg;
	struct cam_hw_soc_info                   *soc_info;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg;
	void __iomem *mem_base;
	uint32_t dbg_frm_irq_mask = 0;
	uint32_t err_irq_mask = 0;
	struct cam_ife_csid_ver2_path_cfg *path_cfg;

	rc = cam_ife_csid_ver2_init_config_rdi_path(
		csid_hw, res);

	if (rc) {
		CAM_ERR(CAM_ISP,
			"CSID:%u %s path res type:%d res_id:%d %d",
			csid_hw->hw_intf->hw_idx,
			res->res_type, res->res_id, res->res_state);
		return rc;
	}

	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	path_reg = csid_reg->path_reg[res->res_id];

	if (!path_reg) {
		CAM_ERR(CAM_ISP, "CSID:%u RDI:%d is not supported on HW",
			 csid_hw->hw_intf->hw_idx, res->res_id);
		return -EINVAL;
	}

	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;

	/* Set crc error threshold for primary rdi path */
	if (res->is_rdi_primary_res)
		csid_hw->crc_error_threshold = path_cfg->height / CAM_IFE_CSID_MAX_CRC_ERR_DIVISOR;

	if (!csid_hw->flags.offline_mode && !(csid_reg->cmn_reg->capabilities &
		CAM_IFE_CSID_CAP_SKIP_EPOCH_CFG)) {
		CAM_DBG(CAM_ISP, "CSID:%u Rdi res: %d",
			csid_hw->hw_intf->hw_idx, res->res_id);

		/*Program the camif part */
		cam_io_w_mb(path_cfg->epoch_cfg << path_reg->epoch0_shift_val,
			mem_base + path_reg->epoch_irq_cfg_addr);
	}

	dbg_frm_irq_mask = csid_hw->debug_info.path_mask;

	if (res->is_rdi_primary_res) {
		dbg_frm_irq_mask |= path_reg->rup_irq_mask;
		if (path_cfg->handle_camif_irq)
			dbg_frm_irq_mask |= path_reg->sof_irq_mask | path_reg->eof_irq_mask |
				path_reg->epoch0_irq_mask;
	}

	/* Enable secondary events dictated by HW mgr for RDI paths */
	if (path_cfg->sec_evt_config.en_secondary_evt) {
		if (path_cfg->sec_evt_config.evt_type & CAM_IFE_CSID_EVT_SOF)
			dbg_frm_irq_mask |= path_reg->sof_irq_mask;

		if (path_cfg->sec_evt_config.evt_type & CAM_IFE_CSID_EVT_EPOCH)
			dbg_frm_irq_mask |= (path_reg->epoch0_irq_mask | path_reg->eof_irq_mask);

		CAM_DBG(CAM_ISP,
			"CSID:%u Enable camif: %d evt irq for res: %s",
			csid_hw->hw_intf->hw_idx, path_cfg->sec_evt_config.evt_type, res->res_name);
	}

	if ((csid_reg->cmn_reg->capabilities & CAM_IFE_CSID_CAP_MULTI_CTXT) &&
		path_cfg->is_aeb_en && (res->res_id > CAM_IFE_PIX_PATH_RES_RDI_0))
		dbg_frm_irq_mask |= (path_reg->epoch0_irq_mask | path_reg->eof_irq_mask);

	res->res_state = CAM_ISP_RESOURCE_STATE_STREAMING;
	path_cfg->irq_reg_idx =  cam_ife_csid_convert_res_to_irq_reg(res->res_id);
	if (path_cfg->discard_init_frames) {
		rc = cam_ife_csid_ver2_subscribe_sof_for_discard(
			path_cfg, csid_hw, res,
			cam_ife_csid_ver2_discard_sof_top_half,
			cam_ife_csid_ver2_discard_sof_rdi_bottom_half);
		if (rc)
			return rc;
	}

	err_irq_mask = path_reg->fatal_err_mask | path_reg->non_fatal_err_mask;
	rc = cam_ife_csid_ver2_path_irq_subscribe(csid_hw, res, dbg_frm_irq_mask, err_irq_mask);
	if (rc)
		return rc;
	cam_ife_csid_ver2_path_rup_aup(csid_hw, res, rup_aup_mask);
	return rc;
}


static int cam_ife_csid_ver2_program_ipp_path(
	struct cam_ife_csid_ver2_hw                 *csid_hw,
	struct cam_isp_resource_node                *res,
	struct cam_ife_csid_ver2_rup_aup_mask       *rup_aup_mask)
{
	int rc = 0;
	const struct cam_ife_csid_ver2_reg_info *csid_reg;
	struct cam_hw_soc_info                   *soc_info;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = NULL;
	uint32_t start_mode_val = 0, err_irq_mask = 0, dbg_frm_irq_mask = 0;
	void __iomem *mem_base;
	struct cam_ife_csid_ver2_path_cfg *path_cfg;

	rc = cam_ife_csid_ver2_init_config_pxl_path(
		csid_hw, res);
	if (rc) {
		CAM_ERR(CAM_ISP,
			"CSID:%u %s path res type:%d res_id:%d %d",
			csid_hw->hw_intf->hw_idx,
			res->res_type, res->res_id, res->res_state);
		return rc;
	}

	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	path_reg = csid_reg->path_reg[res->res_id];

	if (!path_reg) {
		CAM_ERR(CAM_ISP, "CSID:%u IPP is not supported on HW",
			 csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;

	/* Set crc error threshold for ipp path */
	if (res->res_id == CAM_IFE_PIX_PATH_RES_IPP)
		csid_hw->crc_error_threshold = path_cfg->height / CAM_IFE_CSID_MAX_CRC_ERR_DIVISOR;

	if (!(csid_reg->cmn_reg->capabilities & CAM_IFE_CSID_CAP_SKIP_EPOCH_CFG)) {
		cam_io_w_mb(path_cfg->epoch_cfg << path_reg->epoch0_shift_val,
			mem_base + path_reg->epoch_irq_cfg_addr);

		CAM_DBG(CAM_ISP, "CSID[%u] epoch_cfg 0x%x",
			csid_hw->hw_intf->hw_idx, path_cfg->epoch_cfg);
	}

	path_cfg->irq_reg_idx = cam_ife_csid_convert_res_to_irq_reg(res->res_id);

	dbg_frm_irq_mask = csid_hw->debug_info.path_mask;

	if (!(csid_reg->path_reg[res->res_id]->capabilities & CAM_IFE_CSID_CAP_MULTI_CTXT) &&
		(path_cfg->sync_mode == CAM_ISP_HW_SYNC_NONE ||
			path_cfg->sync_mode == CAM_ISP_HW_SYNC_MASTER)) {
		dbg_frm_irq_mask |= path_reg->rup_irq_mask;
		if (path_cfg->handle_camif_irq)
			dbg_frm_irq_mask |= (path_reg->sof_irq_mask | path_reg->epoch0_irq_mask |
				path_reg->eof_irq_mask);
	}

	/* For multi context cases, listen to leading exposure sof and rup done */
	if ((res->res_id == CAM_IFE_PIX_PATH_RES_IPP) &&
		(csid_reg->path_reg[res->res_id]->capabilities & CAM_IFE_CSID_CAP_MULTI_CTXT)) {
		dbg_frm_irq_mask |= path_reg->rup_irq_mask;
		if (path_cfg->handle_camif_irq)
			dbg_frm_irq_mask |= path_reg->sof_irq_mask;
	}

	err_irq_mask = path_reg->fatal_err_mask | path_reg->non_fatal_err_mask;
	if (path_cfg->discard_init_frames) {
		rc = cam_ife_csid_ver2_subscribe_sof_for_discard(
			path_cfg, csid_hw, res,
			cam_ife_csid_ver2_discard_sof_top_half,
			cam_ife_csid_ver2_discard_sof_pix_bottom_half);
		if (rc)
			return rc;
	}

	rc = cam_ife_csid_ver2_path_irq_subscribe(csid_hw, res, dbg_frm_irq_mask, err_irq_mask);
	if (rc)
		return rc;

	start_mode_val = path_reg->start_master_sel_val << path_reg->start_master_sel_shift;

	if (path_cfg->sync_mode == CAM_ISP_HW_SYNC_MASTER) {
		/* Set start mode as master */
		start_mode_val |= path_reg->start_mode_master << path_reg->start_mode_shift;
	} else if (path_cfg->sync_mode == CAM_ISP_HW_SYNC_SLAVE) {
		/* Set start mode as slave */
		start_mode_val |= path_reg->start_mode_slave << path_reg->start_mode_shift;
	} else {
		/* Default is internal halt mode */
		start_mode_val = 0;
	}

	cam_io_w_mb(start_mode_val, mem_base + path_reg->ctrl_addr);

	CAM_DBG(CAM_ISP, "CSID:%u Pix res: %d ctrl val: 0x%x", csid_hw->hw_intf->hw_idx,
		res->res_id, start_mode_val);

	if (path_cfg->sync_mode == CAM_ISP_HW_SYNC_MASTER ||
		path_cfg->sync_mode == CAM_ISP_HW_SYNC_NONE)
		cam_ife_csid_ver2_path_rup_aup(csid_hw, res, rup_aup_mask);

	res->res_state = CAM_ISP_RESOURCE_STATE_STREAMING;

	return rc;
}

static int cam_ife_csid_ver2_enable_path(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_isp_resource_node    *res)
{
	const struct cam_ife_csid_ver2_reg_info      *csid_reg;
	struct cam_hw_soc_info                       *soc_info;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = NULL;
	uint32_t val = 0;
	uint32_t ctrl_addr = 0;
	struct cam_ife_csid_ver2_path_cfg *path_cfg;
	void __iomem *mem_base;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
	soc_info = &csid_hw->hw_info->soc_info;
	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	if (path_cfg->discard_init_frames) {
		CAM_DBG(CAM_ISP, "CSID[%u] skip start cmd for res: %s",
			csid_hw->hw_intf->hw_idx, res->res_id);
		goto end;
	}

	path_reg = csid_reg->path_reg[res->res_id];
	val = path_reg->resume_frame_boundary;
	ctrl_addr = path_reg->ctrl_addr;

	switch (res->res_id) {
	case CAM_IFE_PIX_PATH_RES_IPP:
	case CAM_IFE_PIX_PATH_RES_IPP_1:
	case CAM_IFE_PIX_PATH_RES_IPP_2:
	case CAM_IFE_PIX_PATH_RES_PPP:
		if (path_cfg->sync_mode == CAM_ISP_HW_SYNC_SLAVE)
			return 0;
		break;
	case CAM_IFE_PIX_PATH_RES_RDI_0:
	case CAM_IFE_PIX_PATH_RES_RDI_1:
	case CAM_IFE_PIX_PATH_RES_RDI_2:
	case CAM_IFE_PIX_PATH_RES_RDI_3:
	case CAM_IFE_PIX_PATH_RES_RDI_4:
		if (csid_hw->flags.offline_mode)
			return 0;
		break;
	default:
		return -EINVAL;
	}

	/* For single CSID use-cases with lcr configure global resume */
	if (csid_hw->flags.rdi_lcr_en && !csid_hw->top_cfg.dual_en)
		val |= path_reg->start_mode_global << path_reg->start_mode_shift;
	else
		/* For dual preserve the master-slave config for IPP/PPP */
		val |= cam_io_r_mb(mem_base + ctrl_addr);

	cam_io_w_mb(val, mem_base + ctrl_addr);

	CAM_DBG(CAM_ISP, "CSID[%u] start cmd: 0x%x programmed for res: %s",
		csid_hw->hw_intf->hw_idx, val, res->res_name);
end:
	/* Change state even if we don't configure start cmd */
	res->res_state = CAM_ISP_RESOURCE_STATE_STREAMING;
	return 0;
}

static int cam_ife_csid_ver2_program_ppp_path(
	struct cam_ife_csid_ver2_hw              *csid_hw,
	struct cam_isp_resource_node             *res,
	struct cam_ife_csid_ver2_rup_aup_mask    *rup_aup_mask)
{
	int rc = 0;
	const struct cam_ife_csid_ver2_reg_info *csid_reg;
	struct cam_hw_soc_info                   *soc_info;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = NULL;
	uint32_t  val = 0;
	struct cam_ife_csid_ver2_path_cfg *path_cfg;
	uint32_t irq_mask = 0;
	void __iomem *mem_base;

	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	path_reg = csid_reg->path_reg[res->res_id];

	if (!path_reg) {
		CAM_ERR(CAM_ISP, "CSID:%u PPP is not supported on HW",
			 csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;

	if (csid_hw->flags.rdi_lcr_en && path_reg->crop_drop_enable) {
		path_cfg->drop_enable = true;
		path_cfg->crop_enable = true;
	}

	rc = cam_ife_csid_ver2_init_config_pxl_path(
		csid_hw, res);

	if (rc) {
		CAM_ERR(CAM_ISP,
			"CSID:%u %s path res type:%d res_id:%d %d",
			csid_hw->hw_intf->hw_idx,
			res->res_type, res->res_id, res->res_state);
		return rc;
	}

	path_cfg->irq_reg_idx = cam_ife_csid_convert_res_to_irq_reg(res->res_id);

	/* for dual case
	 * set ppp as slave
	 * if current csid is set as master set
	 * start_master_sel_val as 3
	 */

	if (path_cfg->sync_mode == CAM_ISP_HW_SYNC_NONE) {
		val = 0;
	} else {
		val = path_reg->start_mode_slave <<
			path_reg->start_mode_shift;
		/* Set halt mode as internal master */
		if (csid_hw->sync_mode == CAM_ISP_HW_SYNC_MASTER)
			val |= path_reg->start_master_sel_val <<
				path_reg->start_master_sel_shift;
	}

	cam_io_w_mb(val, mem_base + path_reg->ctrl_addr);

	CAM_DBG(CAM_ISP, "CSID:%u Pix res: %d ctrl val: 0x%x",
		csid_hw->hw_intf->hw_idx, res->res_id, val);

	if (path_cfg->sync_mode == CAM_ISP_HW_SYNC_MASTER ||
		path_cfg->sync_mode == CAM_ISP_HW_SYNC_NONE)
		cam_ife_csid_ver2_path_rup_aup(csid_hw, res, rup_aup_mask);

	val = csid_hw->debug_info.path_mask;

	if (path_cfg->discard_init_frames) {
		rc = cam_ife_csid_ver2_subscribe_sof_for_discard(
			path_cfg, csid_hw, res,
			cam_ife_csid_ver2_discard_sof_top_half,
			cam_ife_csid_ver2_discard_sof_pix_bottom_half);
		if (rc)
			return rc;
	}

	irq_mask = path_reg->fatal_err_mask | path_reg->non_fatal_err_mask;
	rc = cam_ife_csid_ver2_path_irq_subscribe(csid_hw, res, val, irq_mask);
	if (rc)
		return rc;

	return rc;
}

static int cam_ife_csid_ver2_rx_capture_config(
	struct cam_ife_csid_ver2_hw *csid_hw)
{
	const struct cam_ife_csid_ver2_reg_info   *csid_reg;
	struct cam_hw_soc_info                    *soc_info;
	struct cam_ife_csid_ver2_rx_cfg                *rx_cfg;
	uint32_t vc, dt, i;
	uint32_t val = 0;

	for (i = 0; i < CAM_IFE_CSID_CID_MAX; i++)
		if (csid_hw->cid_data[i].cid_cnt)
			break;

	if (i == CAM_IFE_CSID_CID_MAX) {
		CAM_WARN(CAM_ISP, "CSID[%u] no valid cid",
			csid_hw->hw_intf->hw_idx);
		return 0;
	}

	rx_cfg = &csid_hw->rx_cfg;
	if (csid_hw->debug_info.rx_capture_debug_set) {
		vc = csid_hw->debug_info.rx_capture_vc;
		dt = csid_hw->debug_info.rx_capture_dt;
	} else {
		vc  = csid_hw->cid_data[i].vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_0].vc;
		dt  = csid_hw->cid_data[i].vc_dt[CAM_IFE_CSID_MULTI_VC_DT_GRP_0].dt;
	}

	csid_reg = (struct cam_ife_csid_ver2_reg_info *) csid_hw->core_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	if (csid_hw->debug_info.debug_val &
			CAM_IFE_CSID_DEBUG_ENABLE_SHORT_PKT_CAPTURE)
		val = ((1 << csid_reg->csi2_reg->capture_short_pkt_en_shift) |
			(vc << csid_reg->csi2_reg->capture_short_pkt_vc_shift));

	/* CAM_IFE_CSID_DEBUG_ENABLE_LONG_PKT_CAPTURE */
	val |= ((1 << csid_reg->csi2_reg->capture_long_pkt_en_shift) |
		(dt << csid_reg->csi2_reg->capture_long_pkt_dt_shift) |
		(vc << csid_reg->csi2_reg->capture_long_pkt_vc_shift));

	/* CAM_IFE_CSID_DEBUG_ENABLE_CPHY_PKT_CAPTURE */
	if (rx_cfg->lane_type == CAM_ISP_LANE_TYPE_CPHY) {
		val |= ((1 << csid_reg->csi2_reg->capture_cphy_pkt_en_shift) |
			(dt << csid_reg->csi2_reg->capture_cphy_pkt_dt_shift) |
			(vc << csid_reg->csi2_reg->capture_cphy_pkt_vc_shift));
	}

	cam_io_w_mb(val, soc_info->reg_map[0].mem_base + csid_reg->csi2_reg->capture_ctrl_addr);

	CAM_DBG(CAM_ISP, "CSID[%u] rx capture_ctrl: 0x%x", csid_hw->hw_intf->hw_idx, val);

	return 0;
}

static int cam_ife_csid_ver2_top2_irq_subscribe(struct cam_ife_csid_ver2_hw *csid_hw,
	uint32_t irq_mask, uint32_t err_irq_mask)
{
	struct cam_ife_csid_ver2_reg_info *csid_reg = csid_hw->core_info->csid_reg;
	uint32_t                           top_top2_irq_mask = irq_mask;
	uint32_t                           top_info_irq_mask = 0, top_err_irq_mask = 0;
	int                                rc;

	csid_hw->top_top2_irq_handle = cam_irq_controller_subscribe_irq(
		csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
		CAM_IRQ_PRIORITY_0,
		&top_top2_irq_mask,
		csid_hw,
		cam_ife_csid_ver2_handle_top2_irq,
		NULL, NULL, NULL, CAM_IRQ_EVT_GROUP_0);

	if (csid_hw->top_top2_irq_handle < 1) {
		CAM_ERR(CAM_ISP, "CSID[%d] Top2 Subscribe Top Irq fail",
			csid_hw->hw_intf->hw_idx);
		rc = -EINVAL;
		return rc;
	}

	rc = cam_irq_controller_register_dependent(
		csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
		csid_hw->top_irq_controller[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1],
		&top_top2_irq_mask);

	if (rc)
		goto unsub_top;

	top_err_irq_mask = csid_reg->cmn_reg->top_err_irq_mask[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1];
	csid_hw->top_err_irq_handle[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1] =
						cam_irq_controller_subscribe_irq(
						csid_hw->top_irq_controller
						[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1],
						CAM_IRQ_PRIORITY_0,
						&top_err_irq_mask,
						csid_hw,
						cam_ife_csid_ver2_top2_err_top_half,
						cam_ife_csid_ver2_top2_err_irq_bottom_half,
						csid_hw->tasklet,
						&tasklet_bh_api,
						CAM_IRQ_EVT_GROUP_0);

	if (csid_hw->top_err_irq_handle[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1] < 1) {
		CAM_ERR(CAM_ISP, "CSID[%d] top2 err irq register fail",
			csid_hw->hw_intf->hw_idx);
		rc = -EINVAL;
		goto unreg_top2;
	}

	if (csid_hw->debug_info.top_mask[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1]) {
		top_info_irq_mask = csid_hw->debug_info.top_mask[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1];
		csid_hw->top_info_irq_handle[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1]
						= cam_irq_controller_subscribe_irq(
						csid_hw->top_irq_controller
						[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1],
						CAM_IRQ_PRIORITY_1,
						&top_info_irq_mask,
						csid_hw,
						cam_ife_csid_ver2_top2_info_irq_top_half,
						cam_ife_csid_ver2_top2_info_irq_bottom_half,
						csid_hw->tasklet,
						&tasklet_bh_api,
						CAM_IRQ_EVT_GROUP_0);

		if (csid_hw->top_info_irq_handle[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1] < 1) {
			CAM_ERR(CAM_ISP, "CSID[%d] Subscribe Top2 Info Irq fail",
				csid_hw->hw_intf->hw_idx);
			rc = -EINVAL;
			goto unsub_top2_err;
		}
	}

	return 0;
unsub_top2_err:
	if (csid_hw->top_err_irq_handle[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1])
		cam_irq_controller_unsubscribe_irq(
				csid_hw->top_irq_controller[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1],
				csid_hw->top_err_irq_handle[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1]);
	csid_hw->top_err_irq_handle[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1] = 0;
unreg_top2:
	cam_irq_controller_unregister_dependent(
		csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
		csid_hw->top_irq_controller[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1]);
unsub_top:
	cam_irq_controller_unsubscribe_irq(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->top_top2_irq_handle);
	csid_hw->top_top2_irq_handle = 0;
	return rc;
}

static int cam_ife_csid_ver2_csi2_rx2_irq_subscribe(struct cam_ife_csid_ver2_hw *csid_hw)
{
	struct cam_ife_csid_ver2_reg_info *csid_reg =
			(struct cam_ife_csid_ver2_reg_info *) csid_hw->core_info->csid_reg;
	uint32_t                           irq_mask, val = 0;
	uint32_t                           rx_rx2_irq_mask =
						csid_reg->csi2_reg->rx_rx2_irq_mask;
	int                                rc = 0;

	csid_hw->rx_cfg.rx2_irq_handle = cam_irq_controller_subscribe_irq(
						csid_hw->rx_irq_controller
						[CAM_IFE_CSID_RX_IRQ_STATUS_REG0],
						CAM_IRQ_PRIORITY_0,
						&rx_rx2_irq_mask,
						csid_hw,
						cam_ife_csid_ver2_handle_rx2_irq,
						NULL,
						NULL,
						NULL,
						CAM_IRQ_EVT_GROUP_0);

	if (csid_hw->rx_cfg.rx2_irq_handle < 1) {
		CAM_ERR(CAM_ISP, "CSID[%d] RX2 err irq register fail",
			csid_hw->hw_intf->hw_idx);
		rc = -EINVAL;
		return rc;
	}

	rc = cam_irq_controller_register_dependent(
		csid_hw->rx_irq_controller[CAM_IFE_CSID_RX_IRQ_STATUS_REG0],
		csid_hw->rx_irq_controller[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1],
		&rx_rx2_irq_mask);

	if (rc)
		goto unsub_rx2;

	irq_mask = csid_hw->debug_info.rx_mask[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1];
	if (irq_mask) {
		csid_hw->rx_cfg.irq_handle[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1] =
						cam_irq_controller_subscribe_irq(
						csid_hw->rx_irq_controller
						[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1],
						CAM_IRQ_PRIORITY_4,
						&irq_mask,
						csid_hw,
						cam_ife_csid_ver2_rx2_top_half,
						NULL,
						NULL,
						NULL,
						CAM_IRQ_EVT_GROUP_0);

		if (csid_hw->rx_cfg.irq_handle[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1] < 1) {
			CAM_ERR(CAM_ISP, "CSID[%d] RX debug irq register fail",
				csid_hw->hw_intf->hw_idx);
			rc = -EINVAL;
			goto unreg_rx2;
		}
	}

	val = csid_reg->csi2_reg->fatal_err_mask[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1] |
		csid_reg->csi2_reg->part_fatal_err_mask[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1] |
		csid_reg->csi2_reg->non_fatal_err_mask[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1];

	csid_hw->rx_cfg.err_irq_handle[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1] =
						cam_irq_controller_subscribe_irq(
						csid_hw->rx_irq_controller
						[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1],
						CAM_IRQ_PRIORITY_0,
						&val,
						csid_hw,
						cam_ife_csid_ver2_rx2_err_top_half,
						cam_ife_csid_ver2_rx2_err_bottom_half,
						csid_hw->tasklet,
						&tasklet_bh_api,
						CAM_IRQ_EVT_GROUP_0);

	if (csid_hw->rx_cfg.err_irq_handle[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1] < 1) {
		CAM_ERR(CAM_ISP, "CSID[%d] RX2 err irq register fail",
			csid_hw->hw_intf->hw_idx);
		rc = -EINVAL;
		goto unsub_rx2_info;
	}
	return 0;

unsub_rx2_info:
	if (csid_hw->rx_cfg.irq_handle[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1])
		cam_irq_controller_unsubscribe_irq(
			csid_hw->rx_irq_controller[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1],
			csid_hw->rx_cfg.irq_handle[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1]);
	csid_hw->rx_cfg.irq_handle[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1] = 0;
unreg_rx2:
	cam_irq_controller_unregister_dependent(
		csid_hw->rx_irq_controller[CAM_IFE_CSID_RX_IRQ_STATUS_REG0],
		csid_hw->rx_irq_controller[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1]);
unsub_rx2:
	cam_irq_controller_unsubscribe_irq(
			csid_hw->rx_irq_controller[CAM_IFE_CSID_RX_IRQ_STATUS_REG0],
			csid_hw->rx_cfg.rx2_irq_handle);
	csid_hw->rx_cfg.rx2_irq_handle = 0;
	return rc;

};

static int cam_ife_csid_ver2_csi2_irq_subscribe(struct cam_ife_csid_ver2_hw *csid_hw,
	uint32_t irq_mask, uint32_t err_irq_mask)
{
	struct cam_ife_csid_ver2_reg_info *csid_reg = csid_hw->core_info->csid_reg;
	uint32_t num_register = 0;
	uint32_t *top_irq_mask = NULL;
	int top_index = -1;
	int i, rc;

	for (i = CAM_IFE_CSID_TOP_IRQ_STATUS_REG0; i < csid_reg->num_top_regs; i++) {
		if (csid_reg->csi2_reg->top_irq_mask[i]) {
			top_index = i;
			break;
		}
	}

	if (top_index < 0) {
		CAM_ERR(CAM_ISP, "CSID[%d] RX Subscribe Top Irq fail due to invalid mask",
			csid_hw->hw_intf->hw_idx);
		rc = -EINVAL;
		return rc;
	}

	num_register = csid_reg->top_irq_reg_info[top_index].num_registers;
	top_irq_mask = vmalloc(sizeof(uint32_t) * num_register);
	if (!top_irq_mask) {
		CAM_ERR(CAM_ISP, "csid top_irq_mask allocation failed");
		return -ENOMEM;
	}

	top_irq_mask[0] = csid_reg->csi2_reg->top_irq_mask[top_index];

	csid_hw->rx_cfg.top_irq_handle = cam_irq_controller_subscribe_irq(
		csid_hw->top_irq_controller[top_index],
		CAM_IRQ_PRIORITY_0,
		top_irq_mask,
		csid_hw,
		cam_ife_csid_ver2_handle_rx_irq,
		NULL, NULL, NULL, CAM_IRQ_EVT_GROUP_0);

	if (csid_hw->rx_cfg.top_irq_handle < 1) {
		CAM_ERR(CAM_ISP, "CSID[%u] RX Subscribe Top Irq fail",
			csid_hw->hw_intf->hw_idx);
		rc = -EINVAL;
		goto err;
	}

	rc = cam_irq_controller_register_dependent(csid_hw->top_irq_controller[top_index],
		csid_hw->rx_irq_controller[CAM_IFE_CSID_RX_IRQ_STATUS_REG0],
		top_irq_mask);

	if (rc)
		goto unsub_top;

	if (irq_mask) {
		csid_hw->rx_cfg.irq_handle[CAM_IFE_CSID_RX_IRQ_STATUS_REG0] =
						cam_irq_controller_subscribe_irq(
						csid_hw->rx_irq_controller
						[CAM_IFE_CSID_RX_IRQ_STATUS_REG0],
						CAM_IRQ_PRIORITY_4,
						&irq_mask,
						csid_hw,
						cam_ife_csid_ver2_rx_top_half,
						NULL,
						NULL,
						NULL,
						CAM_IRQ_EVT_GROUP_0);

		if (csid_hw->rx_cfg.irq_handle[CAM_IFE_CSID_RX_IRQ_STATUS_REG0] < 1) {
			CAM_ERR(CAM_ISP, "CSID[%d] RX debug irq register fail",
				csid_hw->hw_intf->hw_idx);
			rc = -EINVAL;
			goto unreg_rx;
		}
	}

	csid_hw->rx_cfg.err_irq_handle[CAM_IFE_CSID_RX_IRQ_STATUS_REG0] =
					cam_irq_controller_subscribe_irq(
						csid_hw->rx_irq_controller
						[CAM_IFE_CSID_RX_IRQ_STATUS_REG0],
						CAM_IRQ_PRIORITY_0,
						&err_irq_mask,
						csid_hw,
						cam_ife_csid_ver2_rx_err_top_half,
						cam_ife_csid_ver2_rx_err_bottom_half,
						csid_hw->tasklet,
						&tasklet_bh_api,
						CAM_IRQ_EVT_GROUP_0);

	if (csid_hw->rx_cfg.err_irq_handle[CAM_IFE_CSID_RX_IRQ_STATUS_REG0] < 1) {
		CAM_ERR(CAM_ISP, "CSID[%d] RX err irq register fail",
			csid_hw->hw_intf->hw_idx);
		rc = -EINVAL;
		goto unsub_rx;
	}

	if (csid_reg->num_rx_regs > 1) {
		rc = cam_ife_csid_ver2_csi2_rx2_irq_subscribe(csid_hw);
		if (rc < 0)
			goto unsub_rx;
	}
	vfree(top_irq_mask);
	return 0;

unsub_rx:
	if (csid_hw->rx_cfg.irq_handle[CAM_IFE_CSID_RX_IRQ_STATUS_REG0])
		cam_irq_controller_unsubscribe_irq(
		csid_hw->rx_irq_controller[CAM_IFE_CSID_RX_IRQ_STATUS_REG0],
		csid_hw->rx_cfg.irq_handle[CAM_IFE_CSID_RX_IRQ_STATUS_REG0]);
	csid_hw->rx_cfg.irq_handle[CAM_IFE_CSID_RX_IRQ_STATUS_REG0] = 0;
unreg_rx:
	cam_irq_controller_unregister_dependent(csid_hw->top_irq_controller[top_index],
		csid_hw->rx_irq_controller[CAM_IFE_CSID_RX_IRQ_STATUS_REG0]);
unsub_top:
	cam_irq_controller_unsubscribe_irq(csid_hw->top_irq_controller[top_index],
			csid_hw->rx_cfg.top_irq_handle);
	csid_hw->rx_cfg.top_irq_handle = 0;
err:
	vfree(top_irq_mask);
	return rc;
}

static int cam_ife_csid_ver2_enable_csi2(struct cam_ife_csid_ver2_hw *csid_hw)
{
	int                                             rc = 0;
	struct cam_hw_soc_info                          *soc_info;
	const struct cam_ife_csid_ver2_reg_info         *csid_reg;
	const struct cam_ife_csid_ver2_csi2_rx_reg_info *csi2_reg;
	uint32_t                                        val = 0;
	void __iomem                                    *mem_base;
	struct cam_ife_csid_ver2_rx_cfg                 *rx_cfg;
	int                                             vc_full_width;

	if (csid_hw->flags.rx_enabled)
		return 0;

	if (csid_hw->flags.offline_mode)
		return 0;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	csi2_reg = csid_reg->csi2_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	rx_cfg = &csid_hw->rx_cfg;
	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	/*Configure Rx cfg0 */
	val |= ((rx_cfg->lane_cfg << csi2_reg->lane_cfg_shift) |
		((rx_cfg->lane_num - 1) << csi2_reg->lane_num_shift) |
		(rx_cfg->lane_type << csi2_reg->phy_type_shift));

	if (rx_cfg->tpg_mux_sel) {
		val |= ((rx_cfg->tpg_num_sel << csi2_reg->tpg_num_sel_shift) |
			(rx_cfg->tpg_mux_sel << csi2_reg->tpg_mux_en_shift));
	} else {
		val |= rx_cfg->phy_sel << csi2_reg->phy_num_shift;
	}

	cam_io_w_mb(val, mem_base + csi2_reg->cfg0_addr);

	CAM_DBG(CAM_ISP, "CSID[%u] rx_cfg0: 0x%x",
		csid_hw->hw_intf->hw_idx, val);

	/*Configure Rx cfg1*/
	val = 1 << csi2_reg->misr_enable_shift_val;
	val |= 1 << csi2_reg->ecc_correction_shift_en;
	val |= (rx_cfg->epd_supported << csi2_reg->epd_mode_shift_en);
	if (rx_cfg->dynamic_sensor_switch_en) {
		val |= 1 << csi2_reg->dyn_sensor_switch_shift_en;
		/* Disable rup_aup latch feature */
		if (csi2_reg->rup_aup_latch_supported)
			val |= 1 << csi2_reg->rup_aup_latch_shift;
	}

	vc_full_width = cam_ife_csid_is_vc_full_width(csid_hw->cid_data);

	if (vc_full_width == 1) {
		val |= 1 <<  csi2_reg->vc_mode_shift_val;
	} else if (vc_full_width < 0) {
		CAM_ERR(CAM_ISP, "CSID[%u] Error VC DT", csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	cam_io_w_mb(val, mem_base + csi2_reg->cfg1_addr);
	CAM_DBG(CAM_ISP, "CSID[%u] rx_cfg1: 0x%x",
		csid_hw->hw_intf->hw_idx, val);

	val = csi2_reg->fatal_err_mask[CAM_IFE_CSID_RX_IRQ_STATUS_REG0] |
		csi2_reg->part_fatal_err_mask[CAM_IFE_CSID_RX_IRQ_STATUS_REG0] |
		csi2_reg->non_fatal_err_mask[CAM_IFE_CSID_RX_IRQ_STATUS_REG0];

	if (csid_hw->rx_cfg.epd_supported &&
		(csid_hw->rx_cfg.lane_type == CAM_ISP_LANE_TYPE_DPHY))
		val &= ~BIT(csid_reg->rx_debug_mask->bit_pos[CAM_IFE_CSID_RX_CPHY_EOT_RECEPTION]);

	rc = cam_ife_csid_ver2_csi2_irq_subscribe(csid_hw,
		csid_hw->debug_info.rx_mask[CAM_IFE_CSID_RX_IRQ_STATUS_REG0], val);
	if (rc)
		return rc;

	csid_hw->flags.rx_enabled = true;

	cam_ife_csid_ver2_rx_capture_config(csid_hw);

	return rc;
}

static int cam_ife_csid_ver2_program_top(
	struct cam_ife_csid_ver2_hw *csid_hw)
{
	const struct cam_ife_csid_ver2_top_reg_info *top_reg;
	struct cam_ife_csid_ver2_reg_info *csid_reg;
	uint32_t val;
	struct cam_hw_soc_info              *soc_info;
	int input_core_sel;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	if (!csid_reg->need_top_cfg) {
		CAM_DBG(CAM_ISP, "CSID %d top not supported",
			csid_hw->hw_intf->hw_idx);
		return 0;
	}

	top_reg  = csid_reg->top_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	/* Porgram top parameters */
	input_core_sel = csid_reg->input_core_sel[csid_hw->hw_intf->hw_idx]
				[csid_hw->top_cfg.input_core_type];

	CAM_DBG(CAM_ISP, "CSID[%u] input_core_sel %d",
		csid_hw->hw_intf->hw_idx, input_core_sel);

	if (input_core_sel == -1) {
		CAM_ERR(CAM_ISP, "CSID[%u] invalid top input_core_type %u",
			csid_hw->hw_intf->hw_idx,
			csid_hw->top_cfg.input_core_type);
		return -EINVAL;
	}

	val = (uint32_t)input_core_sel << top_reg->input_core_type_shift_val;

	if ((csid_hw->top_cfg.offline_sfe_en) ||
		(top_reg->sfe_pipeline_bypassed && csid_hw->top_cfg.sfe_fs))
		val |= BIT(top_reg->sfe_offline_en_shift_val);

	val |= csid_hw->top_cfg.out_ife_en <<
			top_reg->out_ife_en_shift_val;

	val |= csid_hw->top_cfg.rdi_lcr;

	cam_io_w_mb(val,
		soc_info->reg_map[CAM_IFE_CSID_TOP_MEM_BASE_ID].mem_base +
		top_reg->io_path_cfg0_addr[csid_hw->hw_intf->hw_idx]);

	/*Program dual csid regs */

	if (csid_hw->sync_mode == CAM_ISP_HW_SYNC_NONE)
		return 0;

	val = csid_hw->top_cfg.dual_sync_core_sel <<
		top_reg->dual_sync_sel_shift_val;
	val |= csid_hw->top_cfg.dual_en <<
		top_reg->dual_en_shift_val;
	val |= csid_hw->top_cfg.master_slave_sel <<
		top_reg->master_slave_sel_shift_val;

	cam_io_w_mb(val,
		soc_info->reg_map[CAM_IFE_CSID_TOP_MEM_BASE_ID].mem_base +
		top_reg->dual_csid_cfg0_addr[csid_hw->hw_intf->hw_idx]);

	return 0;
}

static int cam_ife_csid_ver2_set_cesta_clk_rate(struct cam_ife_csid_ver2_hw *csid_hw,
	bool force_channel_switch, const char *identifier)
{
	struct cam_hw_soc_info *soc_info = &csid_hw->hw_info->soc_info;
	bool channel_switch = force_channel_switch;
	uint32_t lowest_clk_lvl = soc_info->lowest_clk_level;
	int rc = 0;

	CAM_DBG(CAM_ISP, "CSID:%d clk_rate=%llu, channel_switch=%d, identifier=%s",
		csid_hw->hw_intf->hw_idx, csid_hw->clk_rate, channel_switch, identifier);

	if (csid_hw->clk_rate) {
		rc = cam_soc_util_set_src_clk_rate(soc_info, csid_hw->hw_intf->hw_idx,
			csid_hw->clk_rate,
			soc_info->clk_rate[lowest_clk_lvl][soc_info->src_clk_idx]);
		if (rc) {
			CAM_ERR(CAM_ISP,
				"Failed in setting cesta clk rates[high low]:[%ld %ld] client_idx:%d rc:%d",
				csid_hw->clk_rate,
				soc_info->clk_rate[lowest_clk_lvl][soc_info->src_clk_idx],
				csid_hw->hw_intf->hw_idx, rc);
			return rc;
		}
		channel_switch = true;
	}

	if (channel_switch) {
		rc = cam_soc_util_cesta_channel_switch(csid_hw->hw_intf->hw_idx, identifier);
		if (rc) {
			CAM_ERR(CAM_CSIPHY,
				"Failed to apply power states for cesta client:%d rc:%d",
				csid_hw->hw_intf->hw_idx, rc);
			return rc;
		}
	}

	return rc;
}

static int cam_ife_csid_ver2_enable_core(struct cam_ife_csid_ver2_hw *csid_hw)
{
	int rc = 0;
	struct cam_hw_soc_info              *soc_info;
	const struct cam_ife_csid_ver2_reg_info *csid_reg;
	uint32_t clk_lvl;
	struct cam_csid_soc_private *soc_private;
	uint32_t irq_mask = 0;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)csid_hw->core_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	soc_private = (struct cam_csid_soc_private *)soc_info->soc_private;

	/* overflow check before increment */
	if (csid_hw->hw_info->open_count == UINT_MAX) {
		CAM_ERR(CAM_ISP, "CSID:%u Open count reached max",
			csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	/* Increment ref Count */
	csid_hw->hw_info->open_count++;

	if (csid_hw->hw_info->open_count > 1) {
		CAM_DBG(CAM_ISP, "CSID[%u] hw has already been enabled",
			csid_hw->hw_intf->hw_idx);
		return rc;
	}

	rc = cam_soc_util_get_clk_level(soc_info, csid_hw->clk_rate,
		soc_info->src_clk_idx, &clk_lvl);
	if (rc) {
		CAM_ERR(CAM_ISP,
			"CSID[%u] get clk level fail rate %u",
			csid_hw->clk_rate);
	}

	if (soc_private->is_ife_csid_lite)
		CAM_DBG(CAM_ISP,
			"CSID[%u] clk lvl %u received clk_rate %u applied clk_rate :%lu",
			csid_hw->hw_intf->hw_idx, clk_lvl, csid_hw->clk_rate,
			soc_info->applied_src_clk_rates.sw_client);
	else
		CAM_DBG(CAM_ISP,
			"CSID[%d] clk lvl %u received clk_rate %u applied clk_rate sw_client:%lu hw_client:[%lu %lu]",
			csid_hw->hw_intf->hw_idx, clk_lvl, csid_hw->clk_rate,
			soc_info->applied_src_clk_rates.sw_client,
			soc_info->applied_src_clk_rates.hw_client[csid_hw->hw_intf->hw_idx].high,
			soc_info->applied_src_clk_rates.hw_client[csid_hw->hw_intf->hw_idx].low);

	rc = cam_ife_csid_enable_soc_resources(soc_info, clk_lvl);
	if (rc) {
		CAM_ERR(CAM_ISP, "CSID[%d] Enable soc failed", csid_hw->hw_intf->hw_idx);
		goto err;
	}

	CAM_DBG(CAM_ISP, "csid %d is_clk_drv_en %d is_drv_config_en %d",
		csid_hw->hw_intf->hw_idx, soc_info->is_clk_drv_en, csid_hw->is_drv_config_en);

	/*
	 * cam_ife_csid_enable_soc_resources sets clk_lvl for high, low values, if the usecase
	 * enables drv config, i.e enabling csid DRV events, overwrite low value with LowSVS to get
	 * power benefit.
	 */
	if (soc_info->is_clk_drv_en && csid_hw->is_drv_config_en) {
		rc = cam_ife_csid_ver2_set_cesta_clk_rate(csid_hw, false, "csid_init");
		if (rc) {
			CAM_ERR(CAM_ISP, "Failed in setting cesta clk rates, client_idx:%d rc:%d",
				csid_hw->hw_intf->hw_idx, rc);
			return rc;
		}
	}

	irq_mask = csid_reg->cmn_reg->top_reset_irq_mask[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0];

	csid_hw->reset_irq_handle = cam_irq_controller_subscribe_irq(
		csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
		CAM_IRQ_PRIORITY_0,
		&irq_mask,
		csid_hw,
		cam_ife_csid_ver2_reset_irq_top_half,
		NULL,
		NULL,
		NULL,
		CAM_IRQ_EVT_GROUP_0);

	if (csid_hw->reset_irq_handle < 1) {
		CAM_ERR(CAM_ISP, "CSID[%u] reset irq subscribe fail",
			csid_hw->hw_intf->hw_idx);
		goto disable_res;
	}

	reinit_completion(&csid_hw->hw_info->hw_complete);
	cam_ife_csid_ver2_program_top(csid_hw);
	csid_hw->hw_info->hw_state = CAM_HW_STATE_POWER_UP;

	return rc;

disable_res:
	cam_ife_csid_disable_soc_resources(soc_info);

err:
	CAM_ERR(CAM_ISP, "CSID[%u] init hw fail rc %d",
		csid_hw->hw_intf->hw_idx, rc);
	csid_hw->hw_info->open_count--;
	return rc;
}

static int cam_ife_csid_ver2_enable_hw(
	struct cam_ife_csid_ver2_hw *csid_hw)
{
	struct cam_hw_soc_info              *soc_info;
	const struct cam_ife_csid_ver2_reg_info *csid_reg = NULL;
	uint32_t  val;
	int i, rc;
	void __iomem *mem_base;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = NULL;
	uint32_t num_register = 0;
	uint32_t top_err_irq_mask = 0;
	uint32_t *buf_done_irq_mask = NULL;
	uint32_t top_info_irq_mask = 0;
	uint32_t value = 0;
	uint32_t index = 0;

	if (csid_hw->flags.device_enabled) {
		CAM_DBG(CAM_ISP, "CSID[%u] hw has already been enabled",
			csid_hw->hw_intf->hw_idx);
		return 0;
	}

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	/* Clear IRQs */
	for (i = CAM_IFE_CSID_TOP_IRQ_STATUS_REG0; i < csid_reg->num_top_regs; i++)
		cam_io_w_mb(1, (mem_base + csid_reg->cmn_reg->top_irq_clear_addr[i]));

	for (i = CAM_IFE_CSID_RX_IRQ_STATUS_REG0; i < csid_reg->num_rx_regs; i++)
		cam_io_w_mb(csid_reg->csi2_reg->irq_mask_all,
			(mem_base + csid_reg->csi2_reg->irq_clear_addr[i]));

	for (i = 0; i < csid_reg->cmn_reg->num_pix; i++) {
		switch (i) {
		case 0:
			path_reg = csid_reg->path_reg[CAM_IFE_PIX_PATH_RES_IPP];
			break;
		case 1:
			path_reg = csid_reg->path_reg[CAM_IFE_PIX_PATH_RES_IPP_1];
			break;
		case 2:
			path_reg = csid_reg->path_reg[CAM_IFE_PIX_PATH_RES_IPP_2];
			break;
		default:
			path_reg = NULL;
			break;
		}

		if (!path_reg) {
			CAM_ERR(CAM_ISP, "Unsupported num_pix:%d", i);
			return -EINVAL;
		}
		cam_io_w_mb(csid_reg->cmn_reg->ipp_irq_mask_all,
			(mem_base + path_reg->irq_clear_addr));
	}

	if (csid_reg->num_rx_regs > 1) {
		cam_io_w_mb(1, mem_base +
			csid_reg->cmn_reg->top_irq_clear_addr[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1]);
		cam_io_w_mb(csid_reg->csi2_reg->irq_mask_all, mem_base +
			csid_reg->csi2_reg->irq_clear_addr[CAM_IFE_CSID_RX2_IRQ_STATUS_REG1]);
	}

	path_reg = csid_reg->path_reg[CAM_IFE_PIX_PATH_RES_PPP];
	if (csid_reg->cmn_reg->num_ppp)
		cam_io_w_mb(csid_reg->cmn_reg->ppp_irq_mask_all,
			(mem_base + path_reg->irq_clear_addr));

	for (i = 0; i < csid_reg->cmn_reg->num_rdis; i++) {
		path_reg = csid_reg->path_reg[CAM_IFE_PIX_PATH_RES_RDI_0 + i];
		cam_io_w_mb(csid_reg->cmn_reg->rdi_irq_mask_all,
			(mem_base + path_reg->irq_clear_addr));
	}

	cam_io_w_mb(1, (mem_base + csid_reg->cmn_reg->irq_cmd_addr));

	/* Read hw version */
	val = cam_io_r_mb(mem_base + csid_reg->cmn_reg->hw_version_addr);

	/*
	 * Set CSID path domain id enable, used for debug purpose only,
	 * register access is restricted in normal builds, disable by default,
	 * and we can setup domain id value through debugfs, such as we want to
	 * set CSID path as TVM non-secure(0x08), we can echo value 0x08080808.
	 */
	CAM_DBG(CAM_ISP, "Set CSID:%u force set enabled: %d domain id value: 0x%x",
		csid_hw->hw_intf->hw_idx,
		csid_hw->debug_info.set_domain_id_enabled,
		csid_hw->debug_info.domain_id_value);

	if (csid_hw->debug_info.set_domain_id_enabled) {
		value = CAM_IFE_CSID_PATH_DOMAIN_ID;

		if (csid_hw->debug_info.domain_id_value)
			value = csid_hw->debug_info.domain_id_value;

		rc = cam_common_util_get_string_index(soc_info->mem_block_name,
			soc_info->num_mem_block, "csid_sec", &index);

		if ((rc == 0) && (index < CAM_IFE_CSID_MAX_MEM_BASE_ID)) {
			CAM_DBG(CAM_ISP, "regbase found for CSID SEC, rc: %d, index: %d",
				rc, index);

			cam_io_w_mb(value,
				soc_info->reg_map[index].mem_base +
				csid_reg->cmn_reg->path_domain_id_cfg0);
			cam_io_w_mb(value,
				soc_info->reg_map[index].mem_base +
				csid_reg->cmn_reg->path_domain_id_cfg1);

			/* Some target may have 3 such registers, check and set cfg2 */
			if (csid_reg->cmn_reg->path_domain_id_cfg2)
				cam_io_w_mb(value,
					soc_info->reg_map[index].mem_base +
					csid_reg->cmn_reg->path_domain_id_cfg2);

			CAM_DBG(CAM_ISP, "Set CSID:%u domain id value: 0x%x",
				csid_hw->hw_intf->hw_idx, value);
		} else {
			CAM_WARN(CAM_ISP, "regbase not found for CSID SEC, rc: %d, index: %d",
				rc, index);
		}
	}

	num_register = csid_reg->top_irq_reg_info[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0].num_registers;
	buf_done_irq_mask = vmalloc(sizeof(uint32_t) * num_register);
	if (!buf_done_irq_mask) {
		CAM_ERR(CAM_ISP, "csid buf_done_irq_mask allocation failed");
		return -ENOMEM;
	}
	buf_done_irq_mask[0] = csid_reg->cmn_reg->top_buf_done_irq_mask;

	if (csid_reg->ipp_mc_reg)
		buf_done_irq_mask[0] |= csid_reg->ipp_mc_reg->comp_subgrp0_mask |
			csid_reg->ipp_mc_reg->comp_subgrp2_mask;

	csid_hw->buf_done_irq_handle = cam_irq_controller_subscribe_irq(
		csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
		CAM_IRQ_PRIORITY_4,
		buf_done_irq_mask,
		csid_hw,
		cam_ife_csid_ver2_handle_buf_done_irq,
		NULL,
		NULL,
		NULL,
		CAM_IRQ_EVT_GROUP_0);
	if (csid_hw->buf_done_irq_handle < 1) {
		CAM_ERR(CAM_ISP, "CSID[%u] buf done irq subscribe fail",
			csid_hw->hw_intf->hw_idx);
		rc = -EINVAL;
		goto free_buf_done_mask;
	}

	if (csid_reg->num_top_regs > 1)
		cam_ife_csid_ver2_top2_irq_subscribe(csid_hw, csid_reg->cmn_reg->top_top2_irq_mask,
			csid_reg->cmn_reg->top_err_irq_mask[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1]);

	top_err_irq_mask = csid_reg->cmn_reg->top_err_irq_mask[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0];
	csid_hw->top_err_irq_handle[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0] =
		cam_irq_controller_subscribe_irq(
		csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
		CAM_IRQ_PRIORITY_0,
		&top_err_irq_mask,
		csid_hw,
		cam_ife_csid_ver2_top_err_irq_top_half,
		cam_ife_csid_ver2_top_err_irq_bottom_half,
		csid_hw->tasklet,
		&tasklet_bh_api,
		CAM_IRQ_EVT_GROUP_0);

	if (csid_hw->top_err_irq_handle[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0] < 1) {
		CAM_ERR(CAM_ISP, "csid[%u] top error irq subscribe fail",
			csid_hw->hw_intf->hw_idx);
		rc = -EINVAL;
		goto unsubscribe_buf_done;
	}

	rc = cam_irq_controller_register_dependent(csid_hw->top_irq_controller
		[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
		csid_hw->buf_done_irq_controller, buf_done_irq_mask);

	if (rc) {
		cam_irq_controller_unsubscribe_irq(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->buf_done_irq_handle);
		rc = -EINVAL;
		goto unsubscribe_top_err;
	}

	if (csid_hw->debug_info.top_mask[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0]) {
		top_info_irq_mask = csid_hw->debug_info.top_mask[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0];
		csid_hw->top_info_irq_handle[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0]
						= cam_irq_controller_subscribe_irq(
						csid_hw->top_irq_controller
						[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
						CAM_IRQ_PRIORITY_1,
						&top_info_irq_mask,
						csid_hw,
						cam_ife_csid_ver2_top_info_irq_top_half,
						cam_ife_csid_ver2_top_info_irq_bottom_half,
						csid_hw->tasklet,
						&tasklet_bh_api,
						CAM_IRQ_EVT_GROUP_0);

		if (csid_hw->top_info_irq_handle[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0] < 1) {
			CAM_ERR(CAM_ISP, "CSID[%u] Subscribe Top Info Irq fail",
				csid_hw->hw_intf->hw_idx);
			rc = -EINVAL;
			goto unsubscribe_top_err;
		}
	}

	csid_hw->flags.device_enabled = true;
	csid_hw->flags.fatal_err_detected = false;
	CAM_DBG(CAM_ISP, "CSID:%u CSID HW version: 0x%x",
		csid_hw->hw_intf->hw_idx, val);
	vfree(buf_done_irq_mask);
	return 0;


unsubscribe_top_err:
	cam_irq_controller_unsubscribe_irq(
		csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
		csid_hw->top_err_irq_handle[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0]);
	csid_hw->top_err_irq_handle[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0] = 0;

unsubscribe_buf_done:
	cam_irq_controller_unsubscribe_irq(
		csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
		csid_hw->buf_done_irq_handle);
	csid_hw->buf_done_irq_handle = 0;
free_buf_done_mask:
	vfree(buf_done_irq_mask);
	return rc;
}

int cam_ife_csid_ver2_init_hw(void *hw_priv,
	void *init_args, uint32_t arg_size)
{
	struct cam_ife_csid_ver2_hw *csid_hw  = NULL;
	struct cam_hw_info *hw_info;
	int rc = 0;
	struct cam_isp_resource_node *res;

	if (!hw_priv || !init_args ||
		(arg_size != sizeof(struct cam_isp_resource_node))) {
		CAM_ERR(CAM_ISP, "CSID: Invalid args");
		return -EINVAL;
	}

	hw_info = (struct cam_hw_info *)hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;

	mutex_lock(&csid_hw->hw_info->hw_mutex);

	rc = cam_ife_csid_ver2_enable_core(csid_hw);

	if (rc) {
		CAM_ERR(CAM_ISP, "CSID[%u] init hw fail",
			csid_hw->hw_intf->hw_idx);
		goto end;
	}
	res  = (struct cam_isp_resource_node *)init_args;

	res->res_state = CAM_ISP_RESOURCE_STATE_INIT_HW;

	CAM_DBG(CAM_ISP, "CSID[%u] init hw",
		csid_hw->hw_intf->hw_idx);
end:
	mutex_unlock(&csid_hw->hw_info->hw_mutex);
	return rc;
}

static int cam_ife_csid_ver2_disable_core(
	struct cam_ife_csid_ver2_hw *csid_hw)
{
	const struct cam_ife_csid_ver2_reg_info *csid_reg;
	const struct cam_ife_csid_ver2_top_reg_info *top_reg = NULL;
	struct cam_hw_soc_info                   *soc_info;
	struct cam_csid_soc_private              *soc_private;
	int rc = 0;

	/* Check for refcount */
	if (!csid_hw->hw_info->open_count) {
		CAM_WARN(CAM_ISP, "CSID[%u] Unbalanced disable_hw", csid_hw->hw_intf->hw_idx);
		return rc;
	}

	/* Decrement ref Count */
	csid_hw->hw_info->open_count--;

	if (csid_hw->hw_info->open_count)
		return rc;

	soc_info = &csid_hw->hw_info->soc_info;
	soc_private = (struct cam_csid_soc_private *)
		soc_info->soc_private;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	top_reg = csid_reg->top_reg;
	cam_ife_csid_ver2_disable_csi2(true, csid_hw);

	/* Disable the top IRQ interrupt */
	cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
		csid_reg->cmn_reg->top_irq_mask_addr[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0]);

	if (csid_hw->reset_irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->reset_irq_handle);
		if (rc)
			CAM_WARN(CAM_ISP,
				"CSID:%d Failed to unsubscribe reset irq",
				csid_hw->hw_intf->hw_idx);
		csid_hw->reset_irq_handle = 0;
	}

	/*
	 * Wrapper config can be reset only by ares from
	 * camera subsystem power up or CSR ares bit
	 * in CSID clk branch both of which are not
	 * gauranteed at the end of a given CSID stream.
	 * Explicitly resetting the config for full CSIDs
	 * via AHB to avoid conflict on consecutive sessions
	 */
	if ((top_reg) && (!soc_private->is_ife_csid_lite)) {
		cam_io_w_mb(top_reg->io_path_cfg_rst_val,
			soc_info->reg_map[CAM_IFE_CSID_TOP_MEM_BASE_ID].mem_base +
			top_reg->io_path_cfg0_addr[csid_hw->hw_intf->hw_idx]);

		cam_io_w_mb(top_reg->dual_cfg_rst_val,
			soc_info->reg_map[CAM_IFE_CSID_TOP_MEM_BASE_ID].mem_base +
			top_reg->dual_csid_cfg0_addr[csid_hw->hw_intf->hw_idx]);
	}

	spin_lock_bh(&csid_hw->lock_state);
	csid_hw->flags.device_enabled = false;
	csid_hw->hw_info->hw_state = CAM_HW_STATE_POWER_DOWN;
	csid_hw->flags.rdi_lcr_en = false;
	spin_unlock_bh(&csid_hw->lock_state);
	rc = cam_ife_csid_disable_soc_resources(soc_info);
	if (rc)
		CAM_ERR(CAM_ISP, "CSID:%u Disable CSID SOC failed",
			csid_hw->hw_intf->hw_idx);

	csid_hw->counters.error_irq_count = 0;
	csid_hw->counters.crc_error_irq_count = 0;
	return rc;
}

int cam_ife_csid_ver2_deinit_hw(void *hw_priv,
	void *deinit_args, uint32_t arg_size)
{
	struct cam_ife_csid_ver2_hw *csid_hw  = NULL;
	struct cam_isp_resource_node           *res;
	struct cam_hw_info *hw_info;
	int rc = 0;

	if (!hw_priv || !deinit_args ||
		(arg_size != sizeof(struct cam_isp_resource_node))) {
		CAM_ERR(CAM_ISP, "CSID:Invalid arguments");
		return -EINVAL;
	}

	hw_info = (struct cam_hw_info *)hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	res = (struct cam_isp_resource_node *)deinit_args;

	if (csid_hw->hw_info->hw_state == CAM_HW_STATE_POWER_DOWN) {
		CAM_WARN(CAM_ISP, "CSID:%u already powered down",
			csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	if (res->res_type != CAM_ISP_RESOURCE_PIX_PATH) {
		CAM_ERR(CAM_ISP, "CSID:%u Invalid Res type %d",
			 csid_hw->hw_intf->hw_idx,
			res->res_type);
		return -EINVAL;
	}

	if (res->res_state == CAM_ISP_RESOURCE_STATE_RESERVED) {
		CAM_DBG(CAM_ISP, "CSID:%u Res:%d already in De-init state",
			csid_hw->hw_intf->hw_idx,
			res->res_id);
		return -EINVAL;
	}

	mutex_lock(&csid_hw->hw_info->hw_mutex);

	if (res->res_state == CAM_ISP_RESOURCE_STATE_INIT_HW)
		goto disable_hw;

	switch (res->res_id) {
	case  CAM_IFE_PIX_PATH_RES_IPP:
	case CAM_IFE_PIX_PATH_RES_IPP_1:
	case CAM_IFE_PIX_PATH_RES_IPP_2:
	case  CAM_IFE_PIX_PATH_RES_PPP:
	case CAM_IFE_PIX_PATH_RES_RDI_0:
	case CAM_IFE_PIX_PATH_RES_RDI_1:
	case CAM_IFE_PIX_PATH_RES_RDI_2:
	case CAM_IFE_PIX_PATH_RES_RDI_3:
	case CAM_IFE_PIX_PATH_RES_RDI_4:
		rc = cam_ife_csid_ver2_disable_path(true, csid_hw, res, false);
		break;
	default:
		CAM_ERR(CAM_ISP, "CSID:%u Invalid res type%d",
			csid_hw->hw_intf->hw_idx, res->res_type);
		break;
	}

disable_hw:
	res->res_state = CAM_ISP_RESOURCE_STATE_RESERVED;
	cam_ife_csid_ver2_disable_core(csid_hw);
	mutex_unlock(&csid_hw->hw_info->hw_mutex);
	CAM_DBG(CAM_ISP, "De-Init CSID %d Path: %d",
		csid_hw->hw_intf->hw_idx, res->res_id);

	return rc;
}

static void cam_ife_csid_ver2_testbus_config(
	struct cam_ife_csid_ver2_hw *csid_hw, uint32_t val)
{
	struct cam_hw_soc_info *soc_info = &csid_hw->hw_info->soc_info;
	struct cam_ife_csid_ver2_reg_info *csid_reg =
		(struct cam_ife_csid_ver2_reg_info *) csid_hw->core_info->csid_reg;

	cam_io_w_mb(val, soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base +
		csid_reg->cmn_reg->test_bus_ctrl);
	CAM_DBG(CAM_ISP, "CSID [%u] test_bus_ctrl: 0x%x", csid_hw->hw_intf->hw_idx, val);
}

static void cam_ife_csid_ver2_send_secure_info(
	struct cam_csid_hw_start_args *start_args,
	struct cam_ife_csid_ver2_hw    *csid_hw)
{
	struct cam_ife_csid_secure_info        secure_info;
	struct cam_ife_csid_ver2_reg_info     *csid_reg;
	int                                    phy_sel = 0;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	secure_info.lane_cfg = csid_hw->rx_cfg.lane_cfg;
	secure_info.cdm_hw_idx_mask = BIT(start_args->cdm_hw_idx);
	secure_info.vc_mask = 0;
	secure_info.csid_hw_idx_mask = BIT(csid_hw->hw_intf->hw_idx);

	CAM_DBG(CAM_ISP,
		"PHY secure info for CSID[%u], lane_cfg: 0x%x, ife: 0x%x, cdm: 0x%x, vc_mask: 0x%llx",
		csid_hw->hw_intf->hw_idx,
		secure_info.lane_cfg, secure_info.csid_hw_idx_mask,
		secure_info.cdm_hw_idx_mask, secure_info.vc_mask);

	/* Issue one call to PHY for dual IFE cases */
	phy_sel = (int)(csid_hw->rx_cfg.phy_sel - csid_reg->cmn_reg->phy_sel_base_idx);
	if (phy_sel < 0) {
		CAM_WARN(CAM_ISP, "Can't notify csiphy, incorrect phy selected=%d",
			phy_sel);
	} else {
		secure_info.phy_sel = (uint32_t)phy_sel;
		CAM_DBG(CAM_ISP, "Notify CSIPHY: %d", phy_sel);
		cam_subdev_notify_message(CAM_CSIPHY_DEVICE_TYPE,
			CAM_SUBDEV_MESSAGE_DOMAIN_ID_SECURE_PARAMS, (void *)&secure_info);
	}

}

int cam_ife_csid_ver2_start(void *hw_priv, void *args,
			uint32_t arg_size)
{
	struct cam_ife_csid_ver2_hw                 *csid_hw  = NULL;
	struct cam_isp_resource_node                *res;
	struct cam_csid_hw_start_args               *start_args;
	struct cam_ife_csid_ver2_reg_info           *csid_reg;
	struct cam_hw_soc_info                      *soc_info;
	struct cam_hw_info                          *hw_info;
	struct cam_ife_csid_ver2_rup_aup_mask        rup_aup_mask = {0};
	int                                          rc = 0, i;
	bool                                         delay_rdi0_enable = false;
	struct cam_subdev_msg_phy_conn_csid_info     conn_csid_info;
	struct cam_subdev_msg_phy_drv_info           drv_info;
	struct cam_subdev_msg_phy_halt_resume_info   halt_resume_info = {0};
	bool                                         cesta_enabled = false;
	void __iomem                                *csid_clc_membase = NULL;

	if (!hw_priv || !args) {
		CAM_ERR(CAM_ISP, "CSID Invalid params");
		return  -EINVAL;
	}

	hw_info = (struct cam_hw_info *)hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	start_args = (struct cam_csid_hw_start_args *)args;
	csid_clc_membase = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	if (csid_hw->hw_info->hw_state == CAM_HW_STATE_POWER_DOWN) {
		CAM_WARN(CAM_ISP, "CSID:%u already powered down",
			csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}


	mutex_lock(&csid_hw->hw_info->hw_mutex);
	csid_hw->flags.sof_irq_triggered = false;
	csid_hw->counters.irq_debug_cnt = 0;
	csid_hw->is_drv_config_en = start_args->is_drv_config_en;

	CAM_DBG(CAM_ISP,
		"csid %d is_drv_config_en %d start_only %d is_internal_start %d, clk_rate=%lld",
		csid_hw->hw_intf->hw_idx, csid_hw->is_drv_config_en,
		start_args->start_only, start_args->is_internal_start, csid_hw->clk_rate);

	if (start_args->start_only) {
		rc = cam_cpas_csid_process_resume(csid_hw->hw_intf->hw_idx);
		if (rc) {
			CAM_ERR(CAM_ISP, "CSID:%u Failed to process resume rc: %d",
				csid_hw->hw_intf->hw_idx, rc);
			goto end;
		}
	}

	if (soc_info->is_clk_drv_en && csid_hw->is_drv_config_en) {
		/*
		 * This will do 2 things :
		 * 1. At the time of enable_core, we do not have drv info yet populated into
		 *    csid layer, so we set high, low here will overwrite previously set high, low
		 * 2. We trigger a channel switch so that we go to high clocks for all including
		 *    csiphy before calling CSIPHY RESUME, so that we remove sw client vote from
		 *    csiphy device
		 */
		rc = cam_ife_csid_ver2_set_cesta_clk_rate(csid_hw, true, "csid_start");
		if (rc) {
			CAM_ERR(CAM_ISP, "Failed in setting cesta clk rates, client_idx:%d rc:%d",
				csid_hw->hw_intf->hw_idx, rc);
			goto end;
		}
		halt_resume_info.do_drv_ops = true;
	}

	if (halt_resume_info.do_drv_ops) {
		if (csid_hw->sync_mode != CAM_ISP_HW_SYNC_SLAVE) {
			halt_resume_info.phy_idx = (csid_hw->rx_cfg.phy_sel - 1);
			halt_resume_info.lane_cfg = csid_hw->rx_cfg.lane_cfg;
			halt_resume_info.csid_state = CAM_SUBDEV_PHY_CSID_RESUME;
			cam_subdev_notify_message(CAM_CSIPHY_DEVICE_TYPE,
				CAM_SUBDEV_MESSAGE_NOTIFY_HALT_RESUME,
				(void *)&halt_resume_info);
		}
	}

	rc = cam_ife_csid_ver2_enable_hw(csid_hw);

	for (i = 0; i < start_args->num_res; i++) {

		res = start_args->node_res[i];
		CAM_DBG(CAM_ISP, "CSID:%u res_type :%d res[id:%d name:%s]",
			csid_hw->hw_intf->hw_idx, res->res_type,
			res->res_id, res->res_name);

		if (res->res_id >= CAM_IFE_PIX_PATH_RES_MAX) {
			CAM_ERR(CAM_ISP, "CSID:%u Invalid res tpe:%d res id:%d",
				csid_hw->hw_intf->hw_idx, res->res_type,
				res->res_id);
			rc = -EINVAL;
			goto end;
		}

		switch (res->res_id) {
		case CAM_IFE_PIX_PATH_RES_IPP:
		case CAM_IFE_PIX_PATH_RES_IPP_1:
		case CAM_IFE_PIX_PATH_RES_IPP_2:
			rc = cam_ife_csid_ver2_program_ipp_path(csid_hw, res, &rup_aup_mask);
			if (rc)
				goto end;

			break;
		case  CAM_IFE_PIX_PATH_RES_PPP:
			rc = cam_ife_csid_ver2_program_ppp_path(csid_hw, res, &rup_aup_mask);
			if (rc)
				goto end;

			break;
		case CAM_IFE_PIX_PATH_RES_RDI_0:
		case CAM_IFE_PIX_PATH_RES_RDI_1:
		case CAM_IFE_PIX_PATH_RES_RDI_2:
		case CAM_IFE_PIX_PATH_RES_RDI_3:
		case CAM_IFE_PIX_PATH_RES_RDI_4:
			rc = cam_ife_csid_ver2_program_rdi_path(csid_hw, res, &rup_aup_mask);
			if (rc)
				goto end;

			break;
		default:
			CAM_ERR(CAM_ISP, "CSID:%u Invalid res type %d",
					csid_hw->hw_intf->hw_idx, res->res_type);
			break;
		}
	}

	/*
	 * For targets with domain-id support, hand over relevant parameters
	 * to phy driver
	 */
	if ((csid_hw->sync_mode != CAM_ISP_HW_SYNC_SLAVE) &&
		start_args->is_secure &&
		csid_hw->flags.domain_id_security)
		cam_ife_csid_ver2_send_secure_info(start_args, csid_hw);

	/*
	 * Configure RUP/AUP/MUP @ streamon for all enabled paths
	 * For internal recovery - skip this, CDM packet corresponding
	 * to the request being recovered will apply the appropriate RUP/AUP/MUP
	 */
	if (csid_reg->cmn_reg->capabilities & CAM_IFE_CSID_CAP_SPLIT_RUP_AUP) {
		rup_aup_mask.rup_aup_set_mask |=
			(csid_hw->rx_cfg.mup << csid_reg->cmn_reg->mup_shift_val) |
			BIT(csid_reg->cmn_reg->rup_aup_set_shift_val);
	} else {
		rup_aup_mask.rup_mask |=
			(csid_hw->rx_cfg.mup << csid_reg->cmn_reg->mup_shift_val);
	}

	if (!start_args->is_internal_start) {
		if (csid_reg->cmn_reg->capabilities & CAM_IFE_CSID_CAP_SPLIT_RUP_AUP) {
			cam_io_w_mb(rup_aup_mask.rup_mask,
				csid_clc_membase + csid_reg->cmn_reg->rup_cmd_addr);
			cam_io_w_mb(rup_aup_mask.aup_mask,
				csid_clc_membase + csid_reg->cmn_reg->aup_cmd_addr);
			cam_io_w_mb(rup_aup_mask.rup_aup_set_mask,
				csid_clc_membase + csid_reg->cmn_reg->rup_aup_cmd_addr);
		} else {
			cam_io_w_mb(rup_aup_mask.rup_mask,
				csid_clc_membase + csid_reg->cmn_reg->rup_aup_cmd_addr);

		}
	}

	CAM_DBG(CAM_ISP, "CSID:%u RUP:0x%x AUP: 0x%x MUP:0x%x at start updated: %s",
		csid_hw->hw_intf->hw_idx, rup_aup_mask.rup_mask, rup_aup_mask.aup_mask,
		rup_aup_mask.rup_aup_set_mask, CAM_BOOL_TO_YESNO(!start_args->is_internal_start));


	cam_ife_csid_ver2_enable_csi2(csid_hw);

	if (csid_hw->sync_mode != CAM_ISP_HW_SYNC_SLAVE) {
		struct cam_csid_soc_private *soc_private =
			(struct cam_csid_soc_private *)soc_info->soc_private;

		conn_csid_info.phy_idx = (csid_hw->rx_cfg.phy_sel - 1);
		conn_csid_info.lane_cfg = csid_hw->rx_cfg.lane_cfg;
		conn_csid_info.core_idx = csid_hw->hw_intf->hw_idx;
		cam_subdev_notify_message(CAM_CSIPHY_DEVICE_TYPE, CAM_SUBDEV_MESSAGE_CONN_CSID_INFO,
			(void *)&conn_csid_info);

		if (!soc_private->is_ife_csid_lite)
			cam_cpas_query_drv_enable(NULL, &cesta_enabled);

		if (cesta_enabled) {
			/* for ifelites, soc_info->is_clk_drv_en is false */
			drv_info.use_hw_client_voting = soc_info->is_clk_drv_en;
			drv_info.is_drv_config_en = csid_hw->is_drv_config_en;
		} else {
			drv_info.use_hw_client_voting = false;
			drv_info.is_drv_config_en = false;
		}

		drv_info.phy_idx = (csid_hw->rx_cfg.phy_sel - 1);
		drv_info.lane_cfg = csid_hw->rx_cfg.lane_cfg;

		/*
		 * send this even when cesta is not enabled - to support debugfs to switch between
		 * cesta and non-cesta between runs. CSIPHY might have stale information if we
		 * do not send this info everytime
		 */
		cam_subdev_notify_message(CAM_CSIPHY_DEVICE_TYPE,
			CAM_SUBDEV_MESSAGE_DRV_INFO, (void *)&drv_info);
	}

	for (i = 0; i < start_args->num_res; i++) {
		res = start_args->node_res[i];

		if ((csid_hw->flags.rdi_lcr_en) &&
			(res->res_id == CAM_IFE_PIX_PATH_RES_RDI_0)) {
			delay_rdi0_enable = true;
			continue;
		}

		cam_ife_csid_ver2_enable_path(csid_hw, res);
		CAM_DBG(CAM_ISP,
			"CSID[%u] res_type :%d res_id:%d enabled",
			csid_hw->hw_intf->hw_idx, res->res_type, res->res_id);
	}

	if (delay_rdi0_enable) {
		res = &csid_hw->path_res[CAM_IFE_PIX_PATH_RES_RDI_0];
		cam_ife_csid_ver2_enable_path(csid_hw, res);
		CAM_DBG(CAM_ISP, "CSID[%u] Enabling RDI0 after all paths for LCR-PD cases",
			csid_hw->hw_intf->hw_idx);
	}

	if (csid_hw->debug_info.test_bus_val) {
		cam_ife_csid_ver2_testbus_config(csid_hw, csid_hw->debug_info.test_bus_val);
		csid_hw->debug_info.test_bus_enabled = true;
	} else {
		cam_ife_csid_ver2_testbus_config(csid_hw,
			csid_reg->cmn_reg->sync_reset_ctrl_testbus_val);
		csid_hw->debug_info.test_bus_enabled = true;
	}

	/* Check for global resume */
	if (csid_hw->flags.rdi_lcr_en && !csid_hw->top_cfg.dual_en) {
		cam_io_w_mb(1,
			soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base +
			csid_reg->cmn_reg->global_cmd_addr);
		CAM_DBG(CAM_ISP, "CSID[%u] global start set",
			csid_hw->hw_intf->hw_idx);
	}

	csid_hw->flags.reset_awaited = false;

end:
	mutex_unlock(&csid_hw->hw_info->hw_mutex);

	/* Successful starts, release PHY from reset if standby was asserted */
	if (!rc && start_args->start_only && csid_hw->standby_asserted) {
		if (csid_hw->sync_mode != CAM_ISP_HW_SYNC_SLAVE) {
			halt_resume_info.reset_resume_phy = true;
			halt_resume_info.do_drv_ops = false;
			halt_resume_info.phy_idx = (csid_hw->rx_cfg.phy_sel - 1);
			halt_resume_info.lane_cfg = csid_hw->rx_cfg.lane_cfg;
			halt_resume_info.csid_state = CAM_SUBDEV_PHY_CSID_RESUME;
			cam_subdev_notify_message(CAM_CSIPHY_DEVICE_TYPE,
				CAM_SUBDEV_MESSAGE_NOTIFY_HALT_RESUME,
				(void *)&halt_resume_info);
			csid_hw->standby_asserted = false;
		}
	}

	return rc;
}

static void cam_ife_csid_ver2_maskout_all_irqs(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_csid_hw_stop_args *csid_stop)
{
	int i;
	struct cam_hw_soc_info *soc_info;
	const struct cam_ife_csid_ver2_reg_info *csid_reg;
	const struct cam_ife_csid_ver2_csi2_rx_reg_info *csi2_reg;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = NULL;
	void __iomem *mem_base;
	struct cam_isp_resource_node           *res;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
		csid_hw->core_info->csid_reg;
	csi2_reg  = csid_reg->csi2_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	if (csid_reg->cmn_reg->capabilities & CAM_IFE_CSID_CAP_TOP_MASK_ALL_IRQS)
		goto disable_top_irq_status_reg0;

	/* Disable rx */
	if (!csid_hw->flags.offline_mode)
		for (i = CAM_IFE_CSID_RX_IRQ_STATUS_REG0; i < csid_reg->num_rx_regs; i++)
			cam_io_w_mb(0x0, (mem_base + csi2_reg->irq_mask_addr[i]));

	for (i = 0; i < csid_stop->num_res; i++) {
		res = csid_stop->node_res[i];
		path_reg = csid_reg->path_reg[res->res_id];

		/* Disable path */
		cam_io_w_mb(0x0,
			mem_base + path_reg->irq_mask_addr);
	}

	/* Disable buf done */
	cam_io_w_mb(0x0, mem_base +
		csid_reg->cmn_reg->buf_done_irq_mask_addr);

	if (csid_reg->num_top_regs > 1) {
		cam_io_w_mb(0x0, (mem_base +
			csid_reg->cmn_reg->top_irq_mask_addr[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1]));
	}

disable_top_irq_status_reg0:
	/* Disable top except rst_done */
	cam_io_w_mb(csid_reg->cmn_reg->top_reset_irq_mask[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
		mem_base + csid_reg->cmn_reg->top_irq_mask_addr[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0]);
}

int cam_ife_csid_ver2_stop(void *hw_priv,
	void *stop_args, uint32_t arg_size)
{
	struct cam_ife_csid_ver2_hw                     *csid_hw  = NULL;
	struct cam_isp_resource_node                    *res;
	struct cam_hw_info                              *hw_info;
	int                                              rc = 0, i;
	struct cam_csid_hw_stop_args                    *csid_stop;
	struct cam_csid_reset_cfg_args                   reset = {0};
	struct cam_subdev_msg_phy_halt_resume_info       halt_resume_info = {0};
	struct cam_ife_csid_ver2_reg_info               *csid_reg;
	const struct cam_ife_csid_ver2_csi2_rx_reg_info *csi2_reg;
	void __iomem                                    *mem_base;
	struct cam_hw_soc_info                          *soc_info;

	if (!hw_priv || !stop_args ||
		(arg_size != sizeof(struct cam_csid_hw_stop_args))) {
		CAM_ERR(CAM_ISP, "CSID: Invalid args");
		return -EINVAL;
	}

	hw_info = (struct cam_hw_info *)hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)csid_hw->core_info->csid_reg;
	csi2_reg = csid_reg->csi2_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	if (csid_hw->hw_info->hw_state == CAM_HW_STATE_POWER_DOWN) {
		CAM_WARN(CAM_ISP, "CSID:%u already powered down",
			csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	csid_stop = (struct cam_csid_hw_stop_args  *) stop_args;

	if (!csid_stop->num_res) {
		CAM_ERR(CAM_ISP, "CSID:%u Invalid args", csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	CAM_DBG(CAM_ISP, "CSID:%u num_res %d", csid_hw->hw_intf->hw_idx,
		csid_stop->num_res);

	csid_hw->flags.device_enabled = false;
	csid_hw->standby_asserted = (csid_stop->standby_en &&
		(csid_hw->sync_mode != CAM_ISP_HW_SYNC_SLAVE));

	/* Mask out all irqs from HW */
	mutex_lock(&csid_hw->hw_info->hw_mutex);
	cam_ife_csid_ver2_maskout_all_irqs(csid_hw, csid_stop);
	mutex_unlock(&csid_hw->hw_info->hw_mutex);

	cam_ife_csid_ver2_send_cdr_sweep_csi2_rx_vals(csid_hw, csid_reg, 0);

	halt_resume_info.do_drv_ops = (csid_hw->hw_info->soc_info.is_clk_drv_en &&
		csid_hw->is_drv_config_en && (csid_hw->sync_mode != CAM_ISP_HW_SYNC_SLAVE));

	if (likely(halt_resume_info.do_drv_ops)) {
		halt_resume_info.phy_idx = (csid_hw->rx_cfg.phy_sel - 1);
		halt_resume_info.lane_cfg = csid_hw->rx_cfg.lane_cfg;
		halt_resume_info.csid_state = CAM_SUBDEV_PHY_CSID_HALT;
		cam_subdev_notify_message(CAM_CSIPHY_DEVICE_TYPE,
			CAM_SUBDEV_MESSAGE_NOTIFY_HALT_RESUME,
			(void *)&halt_resume_info);
	}

	/* Issue a halt & reset to ensure there is no HW activity post the halt block */
	reset.reset_type = CAM_IFE_CSID_RESET_PATH;
	rc = cam_ife_csid_ver2_reset(hw_priv, &reset,
		sizeof(struct cam_csid_reset_cfg_args));

	/* Ensure halt is successful prior to clearing phy select */
	if (csid_hw->standby_asserted && !rc) {
		/* Reset phy select */
		cam_io_w_mb(0, mem_base + csi2_reg->cfg0_addr);

		/* Issue a global HW reset to clean decoder state */
		reset.reset_type = CAM_IFE_CSID_RESET_GLOBAL_HW_ONLY;
		cam_ife_csid_ver2_reset(hw_priv, &reset,
			sizeof(struct cam_csid_reset_cfg_args));

		reset.reset_type = CAM_IFE_CSID_RESET_GLOBAL_IRQ_CNTRL;
		cam_ife_csid_ver2_reset(hw_priv, &reset,
			sizeof(struct cam_csid_reset_cfg_args));

		CAM_DBG(CAM_ISP,
			"CSID:%u global HW and irq reset issued at stop for standby",
			csid_hw->hw_intf->hw_idx);
	}

	halt_resume_info.reset_resume_phy = csid_hw->standby_asserted;
	halt_resume_info.do_drv_ops = false;
	if (likely(halt_resume_info.reset_resume_phy)) {
		halt_resume_info.phy_idx = (csid_hw->rx_cfg.phy_sel - 1);
		halt_resume_info.lane_cfg = csid_hw->rx_cfg.lane_cfg;
		halt_resume_info.csid_state = CAM_SUBDEV_PHY_CSID_HALT;
		cam_subdev_notify_message(CAM_CSIPHY_DEVICE_TYPE,
			CAM_SUBDEV_MESSAGE_NOTIFY_HALT_RESUME,
			(void *)&halt_resume_info);
	}

	atomic_set(&csid_hw->discard_frame_per_path, 0);
	mutex_lock(&csid_hw->hw_info->hw_mutex);
	for (i = 0; i < csid_stop->num_res; i++) {
		res = csid_stop->node_res[i];
		rc = cam_ife_csid_ver2_disable_path(false, csid_hw, res,
			csid_stop->is_internal_stop);
		res->res_state = CAM_ISP_RESOURCE_STATE_INIT_HW;
		CAM_DBG(CAM_ISP, "CSID:%u res_type %d Resource[id:%d name:%s]",
			csid_hw->hw_intf->hw_idx,
			res->res_type, res->res_id,
			res->res_name);
	}
	if (csid_hw->buf_done_irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq_evt(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->buf_done_irq_handle);
		csid_hw->buf_done_irq_handle = 0;

		cam_irq_controller_unregister_dependent(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->buf_done_irq_controller);
	}


	for (i = CAM_IFE_CSID_TOP_IRQ_STATUS_REG0; i < csid_reg->num_top_regs; i++) {
		if (!csid_hw->top_err_irq_handle[i])
			continue;
		rc = cam_irq_controller_unsubscribe_irq_evt(csid_hw->top_irq_controller[i],
				csid_hw->top_err_irq_handle[i]);
		csid_hw->top_err_irq_handle[i] = 0;
	}

	for (i = CAM_IFE_CSID_TOP_IRQ_STATUS_REG0; i < csid_reg->num_top_regs; i++) {
		if (!csid_hw->debug_info.top_mask[i])
			continue;
		rc = cam_irq_controller_unsubscribe_irq_evt(csid_hw->top_irq_controller[i],
				csid_hw->top_info_irq_handle[i]);
		csid_hw->top_info_irq_handle[i] = 0;
	}


	if (csid_hw->top_mc_irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq_evt(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->top_mc_irq_handle);
		csid_hw->top_mc_irq_handle = 0;
	}

	if (csid_reg->num_top_regs > 1 && csid_hw->top_top2_irq_handle) {
		rc = cam_irq_controller_unsubscribe_irq_evt(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->top_top2_irq_handle);
		csid_hw->top_top2_irq_handle = 0;

		cam_irq_controller_unregister_dependent(
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
			csid_hw->top_irq_controller[CAM_IFE_CSID_TOP2_IRQ_STATUS_REG1]);
	}

	cam_ife_csid_ver2_disable_csi2(false, csid_hw);
	if (csid_hw->debug_info.test_bus_enabled)
		cam_ife_csid_ver2_testbus_config(csid_hw, 0x0);

	if (!csid_stop->is_internal_stop)
		csid_hw->flags.last_exp_valid = false;

	csid_hw->debug_info.test_bus_enabled = false;
	csid_hw->flags.pf_err_detected = false;
	csid_hw->flags.rdi_lcr_en = false;
	mutex_unlock(&csid_hw->hw_info->hw_mutex);

	return rc;
}

int cam_ife_csid_ver2_read(void *hw_priv,
	void *read_args, uint32_t arg_size)
{
	CAM_ERR(CAM_ISP, "CSID: un supported");

	return -EINVAL;
}

int cam_ife_csid_ver2_write(void *hw_priv,
	void *write_args, uint32_t arg_size)
{
	CAM_ERR(CAM_ISP, "CSID: un supported");
	return -EINVAL;
}

static int cam_ife_csid_ver2_top_cfg(
	struct cam_ife_csid_ver2_hw *csid_hw, void *cmd_args)
{
	struct cam_ife_csid_top_config_args *top_args;
	struct cam_ife_csid_ver2_reg_info *csid_reg;
	uint32_t hw_idx;
	int rc = 0;

	if ((!csid_hw) || (!cmd_args))
		return -EINVAL;

	top_args = (struct cam_ife_csid_top_config_args *)cmd_args;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	hw_idx = csid_hw->hw_intf->hw_idx;
	csid_hw->top_cfg.out_ife_en = true;

	/* config out_core parameter*/

	switch (top_args->input_core_type) {
	case CAM_IFE_CSID_INPUT_CORE_NONE:
		csid_hw->top_cfg.input_core_type =
			CAM_IFE_CSID_INPUT_CORE_SEL_NONE;
		csid_hw->top_cfg.out_ife_en = false;
		break;

	case CAM_IFE_CSID_INPUT_CORE_IFE:
		csid_hw->top_cfg.input_core_type =
			CAM_IFE_CSID_INPUT_CORE_SEL_INTERNAL;
		break;

	case CAM_IFE_CSID_INPUT_CORE_SFE:
		csid_hw->top_cfg.out_ife_en = false;
		fallthrough;
	case CAM_IFE_CSID_INPUT_CORE_SFE_IFE:

		if (top_args->core_idx == 0) {
			csid_hw->top_cfg.input_core_type =
				CAM_IFE_CSID_INPUT_CORE_SEL_SFE_0;
		} else if (top_args->core_idx == 1) {
			csid_hw->top_cfg.input_core_type =
				CAM_IFE_CSID_INPUT_CORE_SEL_SFE_1;
		} else if (top_args->core_idx == 2) {
			csid_hw->top_cfg.input_core_type =
				CAM_IFE_CSID_INPUT_CORE_SEL_SFE_2;
		} else {
			rc = -EINVAL;
			CAM_ERR(CAM_ISP,
				"CSID: %u Invalid SFE node %d",
				hw_idx, top_args->core_idx);
		}

		break;

	case CAM_IFE_CSID_INPUT_CORE_CUST_IFE:

		if (!(csid_reg->csid_cust_node_map[hw_idx] &
			BIT(top_args->core_idx))) {
			CAM_ERR(CAM_ISP,
				"CSID: %u not supported for cust node %d",
				hw_idx, top_args->core_idx);
			rc = -EINVAL;
			break;
		}

		if (top_args->core_idx == 0) {
			csid_hw->top_cfg.input_core_type =
				CAM_IFE_CSID_INPUT_CORE_SEL_CUST_NODE_0;
		} else if (top_args->core_idx == 1) {
			csid_hw->top_cfg.input_core_type =
				CAM_IFE_CSID_INPUT_CORE_SEL_CUST_NODE_1;
		} else {
			rc = -EINVAL;
			CAM_ERR(CAM_ISP,
				"CSID: %u Invalid Cust node %d",
				hw_idx, top_args->core_idx);
		}
		break;
	default:
		break;
	}

	csid_hw->top_cfg.offline_sfe_en = top_args->is_sfe_offline;
	csid_hw->top_cfg.sfe_fs = top_args->is_sfe_fs;
	CAM_DBG(CAM_ISP,
		"CSID[%u] input_core_type:%d ife_out:%d sfe_offline:%d sfe_fs:%d",
		hw_idx, csid_hw->top_cfg.input_core_type,
		csid_hw->top_cfg.out_ife_en,
		csid_hw->top_cfg.offline_sfe_en,
		csid_hw->top_cfg.sfe_fs);
	CAM_DBG(CAM_ISP,
		"CSID[%u] Top config received: input_core_type%d core_idx:%d",
		hw_idx, top_args->input_core_type, top_args->core_idx);

	/*config dual sync params */
	if (csid_hw->sync_mode == CAM_ISP_HW_SYNC_NONE)
		return rc;
	else if (csid_hw->sync_mode == CAM_ISP_HW_SYNC_MASTER)
		csid_hw->top_cfg.master_slave_sel =
			csid_reg->top_reg->master_sel_val;
	else
		csid_hw->top_cfg.master_slave_sel =
			csid_reg->top_reg->slave_sel_val;

	csid_hw->top_cfg.dual_en = true;
	csid_hw->top_cfg.dual_sync_core_sel = csid_hw->dual_core_idx + 1;
	CAM_DBG(CAM_ISP,
		"CSID[%u] Top dual sync config core_sel: %d sync_mode: %d",
		hw_idx, csid_hw->sync_mode,
		csid_hw->top_cfg.dual_sync_core_sel);

	return rc;
}

static int cam_ife_csid_ver2_get_mc_reg_val_pair(
	struct cam_ife_csid_ver2_hw                *csid_hw,
	uint32_t                                   *reg_val_pair,
	struct cam_isp_csid_reg_update_args        *rup_args)
{
	struct cam_ife_csid_ver2_reg_info            *csid_reg;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg;
	struct cam_ife_csid_ver2_rup_aup_mask         rup_aup_mask = {0};
	int                                           rc = 0;
	uint32_t                                      i = 0;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	for (i = 0; i < rup_args->num_res; i++) {
		path_reg = csid_reg->path_reg[rup_args->res[i]->res_id];
		if (!path_reg) {
			CAM_ERR(CAM_ISP, "CSID:%d Invalid Path Resource [id %d name %s]",
				csid_hw->hw_intf->hw_idx,
				rup_args->res[i]->res_id,
				rup_args->res[i]->res_name);
			return -EINVAL;
		}
		rup_aup_mask.rup_mask |= path_reg->rup_mask;
		rup_aup_mask.aup_mask |= path_reg->aup_mask;
		rup_aup_mask.rup_aup_set_mask |= path_reg->rup_aup_set_mask;
	}

	reg_val_pair[0] = csid_reg->cmn_reg->rup_cmd_addr;
	reg_val_pair[1] = rup_aup_mask.rup_mask;
	reg_val_pair[2] = csid_reg->cmn_reg->aup_cmd_addr;
	reg_val_pair[3] = rup_aup_mask.aup_mask;
	reg_val_pair[4] = csid_reg->cmn_reg->rup_aup_cmd_addr;
	reg_val_pair[5] = rup_aup_mask.rup_aup_set_mask;

	/**
	 * If not an actual request, configure last applied MUP
	 */
	if (rup_args->reg_write)
		reg_val_pair[5] |= (rup_args->last_applied_mup <<
			csid_reg->cmn_reg->mup_shift_val);
	else {
		if (unlikely(rup_args->add_toggled_mup_entry))
			reg_val_pair[5] |= ((~csid_hw->rx_cfg.mup & 0x1) <<
				csid_reg->cmn_reg->mup_shift_val);
		else
			reg_val_pair[5] |= (csid_hw->rx_cfg.mup <<
				csid_reg->cmn_reg->mup_shift_val);
	}

	CAM_DBG(CAM_ISP,
		"CSID[%d] configure rup: 0x%x, offset: 0x%x, aup: 0x%x, offset: 0x%x toggle MUP: %s",
		csid_hw->hw_intf->hw_idx, reg_val_pair[1], reg_val_pair[0],
		reg_val_pair[3], reg_val_pair[2],
		CAM_BOOL_TO_YESNO(rup_args->add_toggled_mup_entry));
	CAM_DBG(CAM_ISP, "CSID[%d] rup_aup_set reg: 0x%x, offset: 0x%x via %s",
		csid_hw->hw_intf->hw_idx,reg_val_pair[5], reg_val_pair[4],
		(rup_args->reg_write ? "AHB" : "CDM"));
	return rc;
}

static int cam_ife_csid_ver2_get_sc_reg_val_pair(
	struct cam_ife_csid_ver2_hw            *csid_hw,
	uint32_t                               *reg_val_pair,
	struct cam_isp_csid_reg_update_args    *rup_args)
{
	uint32_t                                      i = 0;
	struct cam_ife_csid_ver2_reg_info            *csid_reg;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg;
	int                                           rc = 0;
	struct cam_ife_csid_ver2_rup_aup_mask         rup_aup_mask = {0};

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	for (i = 0; i < rup_args->num_res; i++) {
		path_reg = csid_reg->path_reg[rup_args->res[i]->res_id];
		if (!path_reg) {
			CAM_ERR(CAM_ISP, "CSID:%d Invalid Path Resource [id %d name %s]",
				csid_hw->hw_intf->hw_idx,
				rup_args->res[i]->res_id,
				rup_args->res[i]->res_name);

			return -EINVAL;
		}
		rup_aup_mask.rup_mask |= path_reg->rup_aup_mask;
	}

	reg_val_pair[0] = csid_reg->cmn_reg->rup_aup_cmd_addr;
	reg_val_pair[1] = rup_aup_mask.rup_mask;

	/**
	 * If not an actual request, configure last applied MUP
	 */
	if (rup_args->reg_write)
		reg_val_pair[1] |= (rup_args->last_applied_mup <<
			csid_reg->cmn_reg->mup_shift_val);
	else {
		if (unlikely(rup_args->add_toggled_mup_entry))
			reg_val_pair[1] |= ((~csid_hw->rx_cfg.mup & 0x1) <<
				csid_reg->cmn_reg->mup_shift_val);
		else
			reg_val_pair[1] |= (csid_hw->rx_cfg.mup <<
				csid_reg->cmn_reg->mup_shift_val);
	}

	CAM_DBG(CAM_ISP, "CSID[%d] configure rup_aup_mup: 0x%x offset: 0x%x via %s",
		csid_hw->hw_intf->hw_idx, reg_val_pair[1], reg_val_pair[0],
		(rup_args->reg_write ? "AHB" : "CDM"));
	return rc;
}

static int cam_ife_csid_ver2_reg_update(
	struct cam_ife_csid_ver2_hw   *csid_hw,
	void *cmd_args, uint32_t arg_size)
{
	struct cam_isp_csid_reg_update_args   *rup_args = cmd_args;
	struct cam_ife_csid_ver2_reg_info     *csid_reg;
	struct cam_hw_soc_info                *soc_info = &csid_hw->hw_info->soc_info;
	struct cam_cdm_utils_ops              *cdm_util_ops;
	uint32_t                               num_reg_val_pairs = 0;
	uint32_t                               reg_val_pair[6] = {0};
	int                                    rc = 0;
	uint32_t                               size, i = 0;
	void __iomem                          *csid_clc_membase = soc_info->reg_map
						[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
		csid_hw->core_info->csid_reg;

	if (arg_size != sizeof(struct cam_isp_csid_reg_update_args)) {
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid arg size: %d expected:%ld",
			csid_hw->hw_intf->hw_idx, arg_size,
			sizeof(struct cam_isp_csid_reg_update_args));
		return -EINVAL;
	}

	if (!rup_args) {
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid args", csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	if (!rup_args->num_res ||
		rup_args->num_res > CAM_IFE_PIX_PATH_RES_MAX) {
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid num_res %u",
			csid_hw->hw_intf->hw_idx, rup_args->num_res);
		return -EINVAL;
	}

	cdm_util_ops = (struct cam_cdm_utils_ops *)rup_args->res[0]->cdm_ops;
	if (!cdm_util_ops) {
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid CDM ops", csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	num_reg_val_pairs = (csid_reg->cmn_reg->capabilities & CAM_IFE_CSID_CAP_SPLIT_RUP_AUP) ?
		3 : 1;

	size = cdm_util_ops->cdm_required_size_reg_random(num_reg_val_pairs);
	/* since cdm returns dwords, we need to convert it into bytes */
	if ((!rup_args->reg_write) && ((size * 4) > rup_args->cmd.size)) {
		CAM_ERR(CAM_ISP, "CSID[%u] buf size:%d is not sufficient, expected: %d",
			csid_hw->hw_intf->hw_idx, rup_args->cmd.size, (size*4));
		return -EINVAL;
	}


	if (rup_args->mup_en) {
		csid_hw->rx_cfg.mup = rup_args->mup_val;
		CAM_DBG(CAM_ISP, "CSID[%u] MUP %u",
			csid_hw->hw_intf->hw_idx, csid_hw->rx_cfg.mup);
	}

	if (csid_reg->cmn_reg->capabilities & CAM_IFE_CSID_CAP_SPLIT_RUP_AUP)
		cam_ife_csid_ver2_get_mc_reg_val_pair(csid_hw, reg_val_pair, rup_args);
	else
		cam_ife_csid_ver2_get_sc_reg_val_pair(csid_hw, reg_val_pair, rup_args);

	if (rup_args->reg_write) {
		for (i = 0; i < (2 * num_reg_val_pairs); i = i + 2)
			cam_io_w_mb(reg_val_pair[i + 1], csid_clc_membase + reg_val_pair[i]);
	} else {
		cdm_util_ops->cdm_write_regrandom(rup_args->cmd.cmd_buf_addr,
			num_reg_val_pairs, reg_val_pair);
		rup_args->cmd.used_bytes = size * 4;
	}
	return rc;
}

static int cam_ife_csid_ver2_program_offline_go_cmd(
	struct cam_ife_csid_ver2_hw   *csid_hw,
	void *cmd_args, uint32_t arg_size)
{
	struct cam_ife_csid_offline_cmd_update_args *go_args = cmd_args;
	struct cam_cdm_utils_ops                    *cdm_util_ops;
	struct cam_ife_csid_ver2_reg_info           *csid_reg;
	uint32_t                                     size;
	uint32_t                                     reg_val_pair[2];

	if (!go_args) {
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid args", csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	if (arg_size !=
		sizeof(struct cam_ife_csid_offline_cmd_update_args)) {
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid arg size: %d expected:%ld",
			csid_hw->hw_intf->hw_idx, arg_size,
			sizeof(struct cam_ife_csid_offline_cmd_update_args));
		return -EINVAL;
	}

	cdm_util_ops = (struct cam_cdm_utils_ops *)go_args->res->cdm_ops;

	if (!cdm_util_ops) {
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid CDM ops", csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	size = cdm_util_ops->cdm_required_size_reg_random(1);
	/* since cdm returns dwords, we need to convert it into bytes */
	if ((size * 4) > go_args->cmd.size) {
		CAM_ERR(CAM_ISP, "CSID[%u] buf size:%d is not sufficient, expected: %d",
			csid_hw->hw_intf->hw_idx, go_args->cmd.size, (size*4));
		return -EINVAL;
	}

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	reg_val_pair[0] = csid_reg->cmn_reg->offline_cmd_addr;
	reg_val_pair[1] = 0x1;

	CAM_DBG(CAM_ISP, "CSID:%u offline_cmd 0x%x offset 0x%X",
		csid_hw->hw_intf->hw_idx,
		reg_val_pair[1], reg_val_pair[0]);

	cdm_util_ops->cdm_write_regrandom(go_args->cmd.cmd_buf_addr,
		1, reg_val_pair);

	go_args->cmd.used_bytes = size * 4;

	return 0;
}

static int cam_ife_csid_ver2_get_time_stamp(
	struct cam_ife_csid_ver2_hw  *csid_hw, void *cmd_args)
{
	const struct cam_ife_csid_ver2_path_reg_info *path_reg;
	struct cam_isp_resource_node                 *res = NULL;
	struct cam_ife_csid_ver2_path_cfg            *path_cfg = NULL;
	struct cam_hw_soc_info                       *soc_info;
	struct cam_csid_get_time_stamp_args          *timestamp_args;
	struct cam_ife_csid_ver2_reg_info            *csid_reg;
	struct timespec64                             ts;

	timestamp_args = (struct cam_csid_get_time_stamp_args *)cmd_args;
	res = timestamp_args->node_res;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	if (res->res_type != CAM_ISP_RESOURCE_PIX_PATH ||
		res->res_id >= CAM_IFE_PIX_PATH_RES_MAX) {
		CAM_DBG(CAM_ISP, "CSID:%u Invalid res_type:%d res id%d",
			csid_hw->hw_intf->hw_idx, res->res_type,
			res->res_id);
		return -EINVAL;
	}

	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID:%u Invalid dev state :%d",
			csid_hw->hw_intf->hw_idx,
			csid_hw->hw_info->hw_state);
		return -EINVAL;
	}

	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
	path_reg = csid_reg->path_reg[res->res_id];

	if (!path_reg) {
		CAM_ERR(CAM_ISP, "CSID:%u Invalid res :%d",
			csid_hw->hw_intf->hw_idx, res->res_id);
		return -EINVAL;
	}

	if (timestamp_args->get_prev_timestamp) {
		timestamp_args->prev_time_stamp_val = __cam_ife_csid_ver2_get_time_stamp(
			soc_info->reg_map[0].mem_base,
			path_reg->timestamp_perv0_sof_addr,
			path_reg->timestamp_perv1_sof_addr,
			path_cfg->ts_comb_vcdt_en,
			csid_reg->cmn_reg->ts_comb_vcdt_mask);
	}

	if (timestamp_args->get_curr_timestamp) {
		timestamp_args->time_stamp_val = __cam_ife_csid_ver2_get_time_stamp(
			soc_info->reg_map[0].mem_base,
			path_reg->timestamp_curr0_sof_addr,
			path_reg->timestamp_curr1_sof_addr,
			path_cfg->ts_comb_vcdt_en,
			csid_reg->cmn_reg->ts_comb_vcdt_mask);
	} else {
		if (path_cfg->ts_comb_vcdt_en)
			timestamp_args->time_stamp_val &=
				 ~(uint64_t)csid_reg->cmn_reg->ts_comb_vcdt_mask;
		timestamp_args->time_stamp_val = mul_u64_u32_div(timestamp_args->time_stamp_val,
			CAM_IFE_CSID_QTIMER_MUL_FACTOR,
			CAM_IFE_CSID_QTIMER_DIV_FACTOR);
	}

	if (qtime_to_boottime == 0) {
		if (timestamp_args->raw_boot_time)
			ts = *timestamp_args->raw_boot_time;
		else
			ktime_get_boottime_ts64(&ts);
		qtime_to_boottime =
			(uint64_t)((ts.tv_sec * 1000000000) +
			ts.tv_nsec) - (int64_t)timestamp_args->time_stamp_val;
	}

	timestamp_args->boot_timestamp = timestamp_args->time_stamp_val +
		qtime_to_boottime;
	CAM_DBG(CAM_ISP, "CSID:%u Resource[id:%d name:%s timestamp:%lld]",
		csid_hw->hw_intf->hw_idx, res->res_id, res->res_name,
		timestamp_args->boot_timestamp);
	csid_hw->timestamp.prev_sof_ts = timestamp_args->time_stamp_val;
	csid_hw->timestamp.prev_boot_ts = timestamp_args->boot_timestamp;

	return 0;
}

static int cam_ife_csid_ver2_print_hbi_vbi(
	struct cam_ife_csid_ver2_hw  *csid_hw)
{
	int i;
	struct cam_isp_resource_node *res;
	struct cam_hw_soc_info *soc_info;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg = NULL;
	struct cam_ife_csid_ver2_reg_info *csid_reg;
	uint32_t  hbi, vbi;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid dev state :%d",
			csid_hw->hw_intf->hw_idx,
			csid_hw->hw_info->hw_state);
		return -EINVAL;
	}

	for (i = CAM_IFE_PIX_PATH_RES_RDI_0; i < CAM_IFE_PIX_PATH_RES_MAX; i++) {
		res = &csid_hw->path_res[i];
		if (res->res_state != CAM_ISP_RESOURCE_STATE_STREAMING)
			continue;

		path_reg = csid_reg->path_reg[res->res_id];
		if (!path_reg) {
			CAM_ERR(CAM_ISP, "CSID:%u invalid res %d",
				csid_hw->hw_intf->hw_idx, res->res_id);
			return -EINVAL;
		}
		hbi = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			path_reg->format_measure1_addr);
		vbi = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			path_reg->format_measure2_addr);
		CAM_ERR(CAM_ISP,
			"CSID[%u] Resource[id:%d, name:%s, hbi: %d[%d, %d] cycles, vbi: %d cycles]",
			csid_hw->hw_intf->hw_idx, res->res_id, res->res_name, hbi, hbi>>16, hbi&0xFFF, vbi);

#if defined(CONFIG_SAMSUNG_CAMERA)
		if ((debug_hbi_vbi[1] == csid_hw->rx_cfg.phy_sel - 1)
			&& (i == CAM_IFE_PIX_PATH_RES_RDI_0 || i == CAM_IFE_PIX_PATH_RES_PPP)) {
/*
			minHorizontalBlanking= RoundUp(OutputPixClkRate*csidHBIcycles/CSIDclockRate)
			minCSIDVerticalBlanking=(csidVBIcycles/Ceiling((OutputWidthPixels+csidHBICycles),32)
			minVerticalBlanking= RoundUp(OutputPixClkRate*minCSIDVerticalBlanking)/CSIDClockRate)
*/
			uint64_t csidClockRate = 480000000;
			uint64_t measure_h_blank_10bit = 0;
			uint64_t measure_h_blank_12bit = 0;
			uint64_t measure_v_blank_10bit = 0;
			uint64_t measure_v_blank_12bit = 0;
			uint64_t min_csid_v_blank = 0;
			uint64_t csidHBIcycles = hbi & 0xFFF;
			uint64_t csidVBIcycles = vbi & 0xFFFFFFFF;
			uint64_t outputpixel_clk_10bit = get_cam_op_clk(csid_hw->rx_cfg.phy_sel - 1);
			uint64_t outputpixel_clk_12bit = outputpixel_clk_10bit / 12 * 10;
			uint64_t temp = 0;
			struct cam_ife_csid_ver2_path_cfg *path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;

			measure_h_blank_10bit = (outputpixel_clk_10bit * csidHBIcycles) / csidClockRate;
			measure_h_blank_12bit = (outputpixel_clk_12bit * csidHBIcycles) / csidClockRate;
			temp = (path_cfg->width + csidHBIcycles)/32;
			min_csid_v_blank = csidVBIcycles / ((temp + 1) * 32);
			measure_v_blank_10bit = (outputpixel_clk_10bit * min_csid_v_blank) / csidClockRate;
			measure_v_blank_12bit = (outputpixel_clk_12bit * min_csid_v_blank) / csidClockRate;

			CAM_INFO(CAM_ISP, "width : %d, 10bit op clk : %llu, 12bit op clk : %llu", path_cfg->width, outputpixel_clk_10bit, outputpixel_clk_12bit);

			if ((csid_hw->rx_cfg.phy_sel - 1) == WIDE_CAM)
				CAM_INFO(CAM_ISP, "WIDE_CAM : [10bit] measure_h_b %d, measure_v_b %d, [12bit] measure_h_b %d, measure_v_b %d",
				measure_h_blank_10bit, measure_v_blank_10bit, measure_h_blank_12bit, measure_v_blank_12bit);
			else if ((csid_hw->rx_cfg.phy_sel - 1) == UW_CAM)
				CAM_INFO(CAM_ISP, "UW_CAM : [10bit] measure_h_b %d, measure_v_b %d, [12bit] measure_h_b %d, measure_v_b %d",
				measure_h_blank_10bit, measure_v_blank_10bit, measure_h_blank_12bit, measure_v_blank_12bit);
			else if ((csid_hw->rx_cfg.phy_sel - 1) == TELE1_CAM)
				CAM_INFO(CAM_ISP, "TELE1_CAM : [10bit] measure_h_b %d, measure_v_b %d, [12bit] measure_h_b %d, measure_v_b %d",
				measure_h_blank_10bit, measure_v_blank_10bit, measure_h_blank_12bit, measure_v_blank_12bit);
			else if ((csid_hw->rx_cfg.phy_sel - 1) == TELE2_CAM)
				CAM_INFO(CAM_ISP, "TELE2_CAM : [10bit] measure_h_b %d, measure_v_b %d, [12bit] measure_h_b %d, measure_v_b %d",
				measure_h_blank_10bit, measure_v_blank_10bit, measure_h_blank_12bit, measure_v_blank_12bit);
			else if ((csid_hw->rx_cfg.phy_sel - 1) == FRONT_CAM)
				CAM_INFO(CAM_ISP, "FRONT_CAM : [10bit] measure_h_b %d, measure_v_b %d, [12bit] measure_h_b %d, measure_v_b %d",
				measure_h_blank_10bit, measure_v_blank_10bit, measure_h_blank_12bit, measure_v_blank_12bit);
			else if ((csid_hw->rx_cfg.phy_sel - 1) == COVER_CAM)
				CAM_INFO(CAM_ISP, "COVER_CAM : [10bit] measure_h_b %d, measure_v_b %d, [12bit] measure_h_b %d, measure_v_b %d",
				measure_h_blank_10bit, measure_v_blank_10bit, measure_h_blank_12bit, measure_v_blank_12bit);
			else if ((csid_hw->rx_cfg.phy_sel - 1) == FRONT_AUX)
				CAM_INFO(CAM_ISP, "FRONT_AUX : [10bit] measure_h_b %d, measure_v_b %d, [12bit] measure_h_b %d, measure_v_b %d",
				measure_h_blank_10bit, measure_v_blank_10bit, measure_h_blank_12bit, measure_v_blank_12bit);
			else
				CAM_ERR(CAM_ISP, "Unknown camera");
		}
#endif
	}

	return 0;
}

static int cam_ife_csid_ver2_set_dynamic_switch_config(
	struct cam_ife_csid_ver2_hw *csid_hw,
	void                        *cmd_args)
{
	struct cam_ife_csid_mode_switch_update_args *switch_update = NULL;

	if (!csid_hw)
		return -EINVAL;

	switch_update =
		(struct cam_ife_csid_mode_switch_update_args *)cmd_args;

	if (switch_update->mup_args.use_mup) {
		csid_hw->rx_cfg.mup = switch_update->mup_args.mup_val;
		CAM_DBG(CAM_ISP, "CSID[%u] MUP %u",
			csid_hw->hw_intf->hw_idx, csid_hw->rx_cfg.mup);
	}

	/* Handle number of frames to initially drop based on num starting exposures */
	if (switch_update->exp_update_args.reset_discard_cfg) {
		struct cam_ife_csid_discard_frame_cfg_update *exp_update_args;
		struct cam_ife_csid_ver2_path_cfg            *path_cfg;
		struct cam_isp_resource_node                 *res;
		int i;

		exp_update_args = &switch_update->exp_update_args;
		for (i = CAM_IFE_PIX_PATH_RES_RDI_0; i <= CAM_IFE_PIX_PATH_RES_RDI_2; i++) {
			res = &csid_hw->path_res[i];
			path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;

			/* Skip if path_cfg is NULL */
			if (!path_cfg)
				continue;

			/* Skip if res is not acquired or powered on */
			if ((res->res_state == CAM_ISP_RESOURCE_STATE_RESERVED) ||
				(res->res_state == CAM_ISP_RESOURCE_STATE_INIT_HW)) {
				if ((i - CAM_IFE_PIX_PATH_RES_RDI_0) >=
					exp_update_args->num_exposures) {
					path_cfg->skip_discard_frame_cfg = true;
					if (path_cfg->discard_init_frames) {
						path_cfg->discard_init_frames = false;
						path_cfg->num_frames_discard = 0;
						atomic_dec(&csid_hw->discard_frame_per_path);
						CAM_DBG(CAM_ISP,
							"CSID[%u] Reset discard config for %s",
							csid_hw->hw_intf->hw_idx, res->res_name);
					}
				}
			}
		}
	}

	return 0;
}

static int cam_ife_csid_ver2_set_csid_clock(
	struct cam_ife_csid_ver2_hw          *csid_hw,
	void *cmd_args)
{
	struct cam_ife_csid_clock_update_args *clk_update = NULL;

	if (!csid_hw)
		return -EINVAL;

	clk_update =
		(struct cam_ife_csid_clock_update_args *)cmd_args;

	csid_hw->clk_rate = clk_update->clk_rate;

	CAM_DBG(CAM_ISP, "CSID:%d, clk_rate=%lld", csid_hw->hw_intf->hw_idx, csid_hw->clk_rate);

	return 0;
}

static int cam_ife_csid_ver2_mini_dump(
	struct cam_ife_csid_ver2_hw  *csid_hw,
	void *cmd_args)
{
	struct cam_ife_csid_ver2_mini_dump_data *md;
	uint32_t                                 i  = 0;
	struct cam_ife_csid_ver2_path_cfg       *path_cfg;
	struct cam_ife_csid_ver2_res_mini_dump  *md_res;
	struct cam_isp_resource_node            *res;
	struct cam_hw_mini_dump_args            *md_args;

	md_args = (struct cam_hw_mini_dump_args *)cmd_args;
	if (md_args->len < sizeof(*md)) {
		md_args->bytes_written = 0;
		return 0;
	}

	md  = (struct cam_ife_csid_ver2_mini_dump_data *)
		((uint8_t *)md_args->start_addr);
	md->clk_rate = csid_hw->clk_rate;
	md->hw_state = csid_hw->hw_info->hw_state;

	for (i = 0; i < CAM_IFE_PIX_PATH_RES_MAX; i++) {
		res = &csid_hw->path_res[i];
		path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
		if (!path_cfg)
			continue;

		md_res = &md->res[i];
		md_res->res_id = res->res_id;
		scnprintf(md_res->res_name, CAM_ISP_RES_NAME_LEN, res->res_name);
		memcpy(&md_res->path_cfg, path_cfg, sizeof(*path_cfg));
	}

	memcpy(&md->rx_cfg, &csid_hw->rx_cfg, sizeof(struct cam_ife_csid_ver2_rx_cfg));
	memcpy(&md->flags, &csid_hw->flags, sizeof(struct cam_ife_csid_hw_flags));
	memcpy(md->cid_data, csid_hw->cid_data,
		sizeof(struct cam_ife_csid_cid_data) * CAM_IFE_CSID_CID_MAX);
	md_args->bytes_written = sizeof(*md);

	return 0;
}

static void *cam_ife_csid_ver2_user_dump_info(
	void *dump_struct, uint8_t *addr_ptr)
{
	struct cam_isp_resource_node       *res = NULL;
	struct cam_hw_info                 *hw_info = NULL;
	struct cam_ife_csid_ver2_hw        *csid_hw = NULL;
	struct cam_ife_csid_ver2_reg_info  *csid_reg = NULL;
	struct cam_hw_soc_info             *soc_info = NULL;
	uint32_t                           *addr;
	uint32_t                            frame = 0;
	uint32_t                            val0 = 0, val1 = 0, val2 = 0;

	res = (struct cam_isp_resource_node *)dump_struct;
	hw_info = (struct cam_hw_info *)res->hw_intf->hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;
	csid_reg = csid_hw->core_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	frame = cam_io_r_mb(soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base +
	csid_reg->path_reg[res->res_id]->format_measure0_addr);

	val0 = cam_io_r_mb(soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base +
		csid_reg->path_reg[res->res_id]->debug_camif_0_addr);
	val1 = cam_io_r_mb(soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base +
		csid_reg->path_reg[res->res_id]->debug_camif_1_addr);
	val2 = cam_io_r_mb(soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base +
		csid_reg->path_reg[res->res_id]->debug_halt_status_addr);

	addr = (uint32_t *)addr_ptr;

	*addr++ = ((frame >> csid_reg->cmn_reg->format_measure_height_shift_val) &
		csid_reg->cmn_reg->format_measure_height_mask_val);
	*addr++ = frame & csid_reg->cmn_reg->format_measure_width_mask_val;
	*addr++ = val0;
	*addr++ = val1;
	*addr++ = val2;

	return addr;
}

static int cam_ife_csid_ver2_user_dump(
	struct cam_ife_csid_ver2_hw  *csid_hw,
	void *cmd_args)
{
	uint32_t                                    i = 0;
	struct cam_ife_csid_ver2_path_cfg          *path_cfg;
	struct cam_isp_resource_node               *res;
	struct cam_isp_hw_dump_args                *dump_args;
	int                                         rc = 0;

	if (!csid_hw || !cmd_args) {
		CAM_ERR(CAM_ISP, "Invalid bus private data");
		return -EINVAL;
	} else if (csid_hw->hw_info->hw_state == CAM_HW_STATE_POWER_DOWN) {
		CAM_WARN(CAM_ISP,
			"CSID:%u powered down",
			csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	dump_args = (struct cam_isp_hw_dump_args *)cmd_args;

	rc = cam_common_user_dump_helper(dump_args, cam_common_user_dump_clock,
		csid_hw->hw_info, sizeof(uint64_t), "CLK_RATE_PRINT:");

	if (rc) {
		CAM_ERR(CAM_ISP, "CSID:%u VER2: Clock dump failed, rc: %d",
			csid_hw->hw_intf->hw_idx, rc);
		return rc;
	}

	/* Loop through CSID items */
	for (i = 0; i < CAM_IFE_PIX_PATH_RES_MAX; i++) {
		res = &csid_hw->path_res[i];

		if (res->res_state < CAM_ISP_RESOURCE_STATE_RESERVED) {
			CAM_DBG(CAM_ISP,
				"CSID:%u VER2: path inactive res ID: %d, continuing",
				csid_hw->hw_intf->hw_idx, res->res_id);
			continue;
		}

		path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
		if (!path_cfg)
			continue;

		rc = cam_common_user_dump_helper(dump_args, cam_ife_csid_ver2_user_dump_info,
			res, sizeof(uint32_t), "CSID2_PATH.%s:", res->res_name);

		if (rc) {
			CAM_ERR(CAM_ISP, "CSID:%u VER2: Info dump failed, rc:%d",
				csid_hw->hw_intf->hw_idx, rc);
			return rc;
		}

	}
	return 0;
}

static int cam_ife_csid_ver2_dual_sync_cfg(
	struct cam_ife_csid_ver2_hw  *csid_hw,
	void *cmd_args)
{
	struct cam_ife_csid_dual_sync_args  *dual_sync_args;

	if (!csid_hw || !cmd_args) {
		CAM_ERR(CAM_ISP, "Invalid args %pK  %pK",
			csid_hw, cmd_args);
		return -EINVAL;
	}

	dual_sync_args = (struct cam_ife_csid_dual_sync_args *)cmd_args;
	csid_hw->sync_mode = dual_sync_args->sync_mode;
	csid_hw->dual_core_idx = dual_sync_args->dual_core_id;

	CAM_DBG(CAM_ISP, "CSID[%u] sync_mode %d dual_core_idx: %d",
		csid_hw->hw_intf->hw_idx, csid_hw->sync_mode,
		csid_hw->dual_core_idx);

	return 0;
}

static int cam_ife_csid_ver2_dump_crop_reg(
	struct cam_hw_info  *hw_info, uint32_t *path_id)
{
	struct cam_ife_csid_ver2_hw                    *csid_hw;
	const struct cam_ife_csid_ver2_reg_info        *csid_reg;
	struct cam_hw_soc_info                         *soc_info;
	const struct cam_ife_csid_ver2_path_reg_info   *path_reg;
	struct cam_isp_resource_node                   *path_res;
	struct cam_ife_csid_ver2_path_cfg              *path_cfg;
	uint32_t hcrop = 0, vcrop = 0, cfg0 = 0, cfg1 = 0;
	int rc = 0;
	void __iomem *mem_base;

	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;

	spin_lock(&csid_hw->lock_state);
	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP ||
		*path_id >= CAM_IFE_PIX_PATH_RES_MAX || *path_id < 0) {
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid dev state :%d or path_id :%u",
			csid_hw->hw_intf->hw_idx,
			csid_hw->hw_info->hw_state);
		spin_unlock(&csid_hw->lock_state);
		return -EINVAL;
	}

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)csid_hw->core_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	CAM_INFO(CAM_ISP, "CSID[%u] Clock Name=%s Clock Rate=%u",
		csid_hw->hw_intf->hw_idx, soc_info->clk_name[soc_info->src_clk_idx],
		cam_soc_util_get_applied_src_clk(soc_info, true));

	path_res = &csid_hw->path_res[*path_id];
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)path_res->res_priv;
	if (path_res->res_state == CAM_ISP_RESOURCE_STATE_STREAMING && path_cfg) {
		path_reg = csid_reg->path_reg[*path_id];
		hcrop = cam_io_r_mb(mem_base + path_reg->hcrop_addr);
		vcrop = cam_io_r_mb(mem_base + path_reg->vcrop_addr);
		cfg0 = cam_io_r_mb(mem_base + path_reg->cfg0_addr);
		cfg1 = cam_io_r_mb(mem_base + path_reg->cfg1_addr);
		CAM_INFO(CAM_ISP, "CSID[%u] %s HCROP=0x%x VCROP=0x%x, CFG0=0x%x CFG1=0x%x",
			csid_hw->hw_intf->hw_idx, path_res->res_name, hcrop, vcrop, cfg0, cfg1);
		CAM_INFO(CAM_ISP, "CSID[%u] %s width=%d height=%d start line:%d end line:%d",
			csid_hw->hw_intf->hw_idx, path_res->res_name, path_cfg->width,
			path_cfg->height, path_cfg->start_line, path_cfg->end_line);
	}

	spin_unlock(&csid_hw->lock_state);
	return rc;
}

static int cam_ife_csid_ver2_set_discard_frame_cfg(
	struct cam_ife_csid_ver2_hw    *csid_hw,
	void                           *cmd_args)
{
	struct cam_isp_resource_node                 *res;
	struct cam_ife_csid_ver2_path_cfg            *path_cfg;
	struct cam_ife_csid_discard_init_frame_args  *discard_config = NULL;

	if (!csid_hw)
		return -EINVAL;

	discard_config =
		(struct cam_ife_csid_discard_init_frame_args *)cmd_args;

	if (discard_config->num_frames == 0xffffffff) {
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid number of frames: 0x%x",
			csid_hw->hw_intf->hw_idx, discard_config->num_frames);
		return -EINVAL;
	}

	if (!discard_config->num_frames) {
		CAM_DBG(CAM_ISP, "CSID[%u] No discard requested", csid_hw->hw_intf->hw_idx);
		return 0;
	}

	res = discard_config->res;
	if (res->res_type != CAM_ISP_RESOURCE_PIX_PATH ||
		res->res_id >= CAM_IFE_PIX_PATH_RES_MAX) {
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid res_type: %d res id: %d",
			csid_hw->hw_intf->hw_idx, res->res_type,
			res->res_id);
		return -EINVAL;
	}

	/* Handle first stream on and consecutive streamons post flush */
	if ((res->res_state == CAM_ISP_RESOURCE_STATE_RESERVED) ||
		(res->res_state == CAM_ISP_RESOURCE_STATE_INIT_HW)) {
		/* Skip if already set or need to skip based on stream on exposures */
		path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
		if (path_cfg->skip_discard_frame_cfg || path_cfg->discard_init_frames)
			goto end;

		path_cfg->discard_init_frames = true;
		path_cfg->sof_cnt = 0;
		path_cfg->num_frames_discard = discard_config->num_frames;
		atomic_inc(&csid_hw->discard_frame_per_path);
		CAM_DBG(CAM_ISP,
			"CSID[%u] discard num of frames: %u for path: %s discard_ref_cnt: %u",
			csid_hw->hw_intf->hw_idx, discard_config->num_frames, res->res_name,
			atomic_read(&csid_hw->discard_frame_per_path));
	}

end:
	return 0;
}

static int cam_ife_csid_ver2_rdi_lcr_cfg(
	struct cam_ife_csid_ver2_hw  *csid_hw, void *cmd_args)
{
	const struct cam_ife_csid_ver2_path_reg_info *path_reg;
	struct cam_ife_csid_ver2_reg_info            *csid_reg;
	struct cam_ife_csid_ver2_path_cfg            *path_cfg = NULL;
	struct cam_isp_resource_node                 *res = cmd_args;

	if (!csid_hw || !cmd_args) {
		CAM_ERR(CAM_ISP, "Invalid params");
		return -EINVAL;
	}

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	path_reg = csid_reg->path_reg[res->res_id];
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
	if (!path_cfg || !path_reg || !path_reg->capabilities ||
		!(path_reg->capabilities & CAM_IFE_CSID_CAP_INPUT_LCR)) {
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid res %s",
			csid_hw->hw_intf->hw_idx, res->res_name);
		return -EINVAL;
	}

	if (!path_cfg->sfe_shdr && (res->res_id != CAM_IFE_PIX_PATH_RES_RDI_0)) {
		CAM_ERR(CAM_ISP, "CSID[%u] Invalid res: %s, capabilities 0x%x sfe_shdr: %u",
			csid_hw->hw_intf->hw_idx, res->res_name,
			path_reg->capabilities, path_cfg->sfe_shdr);
		return -EINVAL;
	}

	/*
	 * LCR should not be on for a resource if CSID is giving packed data
	 * this case would come for formats which are not supported.
	 */
	if (path_cfg->path_format[CAM_IFE_CSID_MULTI_VC_DT_GRP_0].packing_fmt) {
		CAM_ERR(CAM_ISP, "CSID[%u] [%s] LCR not supported in_format %d out_format %d",
			csid_hw->hw_intf->hw_idx, res->res_name,
			path_cfg->in_format[CAM_IFE_CSID_MULTI_VC_DT_GRP_0],
			path_cfg->out_format);
		return -EINVAL;
	}

	/* if CSID unpacked data is not in MSB, we loose few bits going into PDAF, warn for now */
	if (!path_cfg->csid_out_unpack_msb)
		CAM_WARN(CAM_ISP,
			"CSID[%u] [%s] Input data to LCR is in LSB, in_format %d out_format %d",
			csid_hw->hw_intf->hw_idx, res->res_name,
			path_cfg->in_format[CAM_IFE_CSID_MULTI_VC_DT_GRP_0],
			path_cfg->out_format);

	if (csid_hw->flags.sfe_en)
		csid_hw->top_cfg.rdi_lcr |= BIT(res->res_id) <<
			csid_reg->top_reg->rdi_lcr_shift_val;

	csid_hw->flags.rdi_lcr_en = true;
	path_cfg->lcr_en = true;

	CAM_DBG(CAM_ISP, "CSID[%u] %s top_cfg %u",
		csid_hw->hw_intf->hw_idx, res->res_name, csid_hw->top_cfg.rdi_lcr);

	return 0;
}

static int cam_ife_csid_init_config_update(
	void *cmd_args, uint32_t arg_size)
{
	struct cam_isp_hw_init_config_update *init_cfg = cmd_args;
	struct cam_isp_resource_node *res = init_cfg->node_res;
	struct cam_ife_csid_ver2_path_cfg *path_cfg = NULL;

	if (arg_size != sizeof(struct cam_isp_hw_init_config_update)) {
		CAM_ERR(CAM_ISP, "Invalid args size expected: %zu actual: %zu",
			sizeof(struct cam_isp_hw_init_config_update),
			arg_size);
		return -EINVAL;
	}

	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;
	/* Skip epoch update if resource does not handle camif IRQs */
	if (!path_cfg->handle_camif_irq)
		goto end;

	path_cfg->epoch_cfg = (path_cfg->end_line - path_cfg->start_line) *
		init_cfg->init_config->epoch_cfg.epoch_factor / 100;

	if (path_cfg->epoch_cfg > path_cfg->end_line)
		path_cfg->epoch_cfg = path_cfg->end_line;

	if (path_cfg->horizontal_bin || path_cfg->qcfa_bin)
		path_cfg->epoch_cfg >>= 1;

	CAM_DBG(CAM_ISP,
		"Init Update for res_name: %s epoch_factor: %x",
		res->res_name, path_cfg->epoch_cfg);
end:
	return 0;
}

static int cam_ife_csid_ver2_dump_irq_desc(
	struct cam_ife_csid_ver2_hw  *csid_hw, void *args)
{
	int                                     i, j, offset = 0;
	struct cam_isp_irq_inject_param         *inject_params = NULL;
	const struct cam_ife_csid_ver2_reg_info *csid_reg = NULL;

	if (!args) {
		CAM_ERR(CAM_ISP, "Invalid params");
		return -EINVAL;
	}

	inject_params = (struct cam_isp_irq_inject_param *)args;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	offset += scnprintf(inject_params->line_buf + offset,
		LINE_BUFFER_LEN - offset,
		"Printing executable IRQ for hw_type: CSID reg_unit: %d\n",
		inject_params->reg_unit);

	switch (inject_params->reg_unit) {
	case CAM_ISP_CSID_TOP_REG:
		for (i = CAM_IFE_CSID_TOP_IRQ_STATUS_REG0;
			i < CAM_IFE_CSID_TOP_IRQ_STATUS_REG_MAX; i++) {
			offset += scnprintf(inject_params->line_buf + offset,
				LINE_BUFFER_LEN - offset,
				"TOP_IRQ_STATUS_REG[%d]:\n", i);

			for (j = 0; j < csid_reg->num_top_err_irqs[i]; j++) {
				if (!(*csid_reg->top_irq_desc)[i][j].bitmask)
					break;

				offset += scnprintf(inject_params->line_buf + offset,
					LINE_BUFFER_LEN - offset, "%#12x : %s - %s\n",
					(*csid_reg->top_irq_desc)[i][j].bitmask,
					(*csid_reg->top_irq_desc)[i][j].err_name,
					(*csid_reg->top_irq_desc)[i][j].desc);
			}
		}
		break;
	case CAM_ISP_CSID_RX_REG:
		for (i = CAM_IFE_CSID_RX_IRQ_STATUS_REG0;
			i < CAM_IFE_CSID_RX_IRQ_STATUS_REG_MAX; i++) {
			offset += scnprintf(inject_params->line_buf + offset,
				LINE_BUFFER_LEN - offset,
				"RX_IRQ_STATUS_REG[%d]:\n", i);

			for (j = 0; j < csid_reg->num_rx_err_irqs[i]; j++) {
				if (!(*csid_reg->rx_irq_desc)[i][j].bitmask)
					break;

				offset += scnprintf(inject_params->line_buf + offset,
					LINE_BUFFER_LEN - offset, "%#12x : %s\n",
					(*csid_reg->rx_irq_desc)[i][j].bitmask,
					(*csid_reg->rx_irq_desc)[i][j].desc);
			}
		}
		break;
	case CAM_ISP_CSID_PATH_IPP_REG:
	case CAM_ISP_CSID_PATH_PPP_REG:
	case CAM_ISP_CSID_PATH_RDI0_REG:
	case CAM_ISP_CSID_PATH_RDI1_REG:
	case CAM_ISP_CSID_PATH_RDI2_REG:
	case CAM_ISP_CSID_PATH_RDI3_REG:
	case CAM_ISP_CSID_PATH_RDI4_REG:
		for (i = 0; i < csid_reg->num_path_err_irqs; i++)
			offset += scnprintf(inject_params->line_buf + offset,
				LINE_BUFFER_LEN - offset, "%#12x : %s\n",
				csid_reg->path_irq_desc[i].bitmask,
				csid_reg->path_irq_desc[i].desc);
		break;
	default:
		scnprintf(inject_params->line_buf + offset,
			LINE_BUFFER_LEN - offset,
			"No matched reg unit for injection\n");
		return -EINVAL;
	}

	return 0;
}

static int cam_ife_csid_ver2_irq_inject(
	struct cam_ife_csid_ver2_hw  *csid_hw, void *args)
{
	int                                     i, j;
	uint32_t                                irq_set_addr = 0;
	struct cam_hw_soc_info                  *soc_info;
	struct cam_isp_irq_inject_param         *inject_params = NULL;
	const struct cam_ife_csid_ver2_reg_info *csid_reg = NULL;
	void __iomem *mem_base;

	if (!args) {
		CAM_ERR(CAM_ISP, "Invalid params");
		return -EINVAL;
	}

	inject_params = (struct cam_isp_irq_inject_param *)args;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	switch (inject_params->reg_unit) {
	case CAM_ISP_CSID_TOP_REG: {
		for (i = CAM_IFE_CSID_TOP_IRQ_STATUS_REG0;
			i < CAM_IFE_CSID_TOP_IRQ_STATUS_REG_MAX; i++) {
			for (j = 0; j < csid_reg->num_top_err_irqs[i]; j++) {
				if ((*csid_reg->top_irq_desc)[i][j].bitmask ==
					inject_params->irq_mask) {
					irq_set_addr = csid_reg->cmn_reg->top_irq_set_addr[i];
					break;
				}
			}
		}
		break;
	}
	case CAM_ISP_CSID_RX_REG: {
		for (i = CAM_IFE_CSID_RX_IRQ_STATUS_REG0;
			i < CAM_IFE_CSID_RX_IRQ_STATUS_REG_MAX; i++) {
			for (j = 0; j < csid_reg->num_rx_err_irqs[i]; j++) {
				if ((*csid_reg->rx_irq_desc)[i][j].bitmask ==
					inject_params->irq_mask) {
					irq_set_addr = csid_reg->csi2_reg->irq_set_addr[i];
					break;
				}
			}
		}
		break;
	}
	case CAM_ISP_CSID_PATH_IPP_REG: {
		irq_set_addr =
			csid_reg->path_reg[CAM_IFE_PIX_PATH_RES_IPP]->irq_set_addr;
		break;
	}
	case CAM_ISP_CSID_PATH_PPP_REG: {
		irq_set_addr =
			csid_reg->path_reg[CAM_IFE_PIX_PATH_RES_PPP]->irq_set_addr;
		break;
	}
	case CAM_ISP_CSID_PATH_RDI0_REG: {
		irq_set_addr =
			csid_reg->path_reg[CAM_IFE_PIX_PATH_RES_RDI_0]->irq_set_addr;
		break;
	}
	case CAM_ISP_CSID_PATH_RDI1_REG: {
		irq_set_addr =
			csid_reg->path_reg[CAM_IFE_PIX_PATH_RES_RDI_1]->irq_set_addr;
		break;
	}
	case CAM_ISP_CSID_PATH_RDI2_REG: {
		irq_set_addr =
			csid_reg->path_reg[CAM_IFE_PIX_PATH_RES_RDI_2]->irq_set_addr;
		break;
	}
	case CAM_ISP_CSID_PATH_RDI3_REG: {
		irq_set_addr =
			csid_reg->path_reg[CAM_IFE_PIX_PATH_RES_RDI_3]->irq_set_addr;
		break;
	}
	case CAM_ISP_CSID_PATH_RDI4_REG: {
		irq_set_addr =
			csid_reg->path_reg[CAM_IFE_PIX_PATH_RES_RDI_4]->irq_set_addr;
		break;
	}
	default:
		CAM_INFO(CAM_ISP, "No matched reg unit for injection");
		return -EINVAL;
	}

	cam_io_w_mb(inject_params->irq_mask, mem_base + irq_set_addr);
	cam_io_w_mb(0x10, mem_base + csid_reg->cmn_reg->irq_cmd_addr);
	CAM_INFO(CAM_ISP, "Injected : irq_mask %#x set_reg_offset %#x",
		inject_params->irq_mask, irq_set_addr);

	return 0;
}

static int cam_ife_csid_ver2_reset_out_of_sync_cnt(
	struct cam_ife_csid_ver2_hw  *csid_hw, void *args)
{
	struct cam_ife_csid_ver2_path_cfg  *path_cfg = NULL;
	struct cam_isp_resource_node       *res = NULL;

	res = ((struct cam_csid_reset_out_of_sync_count_args *)args)->node_res;
	path_cfg = (struct cam_ife_csid_ver2_path_cfg *)res->res_priv;

	if (!path_cfg) {
		CAM_ERR(CAM_ISP, "Invalid res %s", res->res_name);
		return -EINVAL;
	}

	atomic_set(&path_cfg->switch_out_of_sync_cnt, 0);

	CAM_DBG(CAM_ISP,
		"Reset out of sync cnt for res:%s", res->res_name);

	return 0;
}

static int cam_ife_csid_ver2_drv_config(
	struct cam_ife_csid_ver2_hw  *csid_hw, void *cmd_args)
{
	struct cam_hw_soc_info *soc_info;
	struct cam_ife_csid_ver2_reg_info *csid_reg;
	struct cam_ife_csid_drv_config_args *drv_config;
	uint32_t cfg0_val = 0, cfg1_val = 0;
	void __iomem *mem_base;
	int i;

	if (!csid_hw || !cmd_args) {
		CAM_ERR(CAM_ISP, "Invalid params");
		return -EINVAL;
	}

	drv_config = (struct cam_ife_csid_drv_config_args *) cmd_args;
	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = (struct cam_ife_csid_ver2_reg_info *) csid_hw->core_info->csid_reg;
	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	cfg0_val = drv_config->drv_en << csid_reg->cmn_reg->drv_en_shift;

	/* Configure DRV RUP EN for init request, one time */
	if (drv_config->is_init_config) {
		if (csid_hw->path_res[CAM_IFE_PIX_PATH_RES_IPP].res_state >=
			CAM_ISP_RESOURCE_STATE_RESERVED) {
			cfg1_val = csid_reg->cmn_reg->drv_rup_en_val_map[CAM_IFE_PIX_PATH_RES_IPP];
		} else if (csid_hw->path_res[CAM_IFE_PIX_PATH_RES_RDI_0].res_state >=
			CAM_ISP_RESOURCE_STATE_RESERVED) {
			cfg1_val =
				csid_reg->cmn_reg->drv_rup_en_val_map[CAM_IFE_PIX_PATH_RES_RDI_0];
		} else if (csid_hw->path_res[CAM_IFE_PIX_PATH_RES_PPP].res_state >=
			CAM_ISP_RESOURCE_STATE_RESERVED) {
			cfg1_val = csid_reg->cmn_reg->drv_rup_en_val_map[CAM_IFE_PIX_PATH_RES_PPP];
		} else {
			CAM_ERR(CAM_ISP, "CSID:%u Failed to configure rup_en for drv",
				csid_hw->hw_intf->hw_idx);
			return -EINVAL;
		}

		cam_io_w_mb(cfg1_val, mem_base + csid_reg->cmn_reg->drv_cfg1_addr);
		csid_hw->drv_init_done = true;
	}

	if (!csid_hw->drv_init_done) {
		CAM_ERR(CAM_ISP, "CSID:%u Failed to update drv config, init config not done",
			csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	for (i = 0; i < CAM_ISP_MAX_PATHS; i++)
		cfg0_val |= ((drv_config->path_idle_en & BIT(i)) ?
			csid_reg->cmn_reg->drv_path_idle_en_val_map[i] : 0);

	cam_io_w_mb(cfg0_val, mem_base + csid_reg->cmn_reg->drv_cfg0_addr);

	cam_io_w_mb(drv_config->timeout_val, mem_base + csid_reg->cmn_reg->drv_cfg2_addr);

	if (debug_drv)
		CAM_INFO(CAM_ISP,
			"CSID[%u] sfe_en:%s DRV config init_req:%s cfg0_val:0x%x cfg1_val:0x%x cfg2_val:0x%x",
			csid_hw->hw_intf->hw_idx, CAM_BOOL_TO_YESNO(csid_hw->flags.sfe_en),
			CAM_BOOL_TO_YESNO(drv_config->is_init_config), cfg0_val, cfg1_val,
			drv_config->timeout_val);

	CAM_DBG(CAM_ISP,
		"CSID[%u] sfe_en:%s DRV config init_req:%s cfg0_val:0x%x cfg1_val:0x%x cfg2_val:0x%x",
		csid_hw->hw_intf->hw_idx, CAM_BOOL_TO_YESNO(csid_hw->flags.sfe_en),
		CAM_BOOL_TO_YESNO(drv_config->is_init_config), cfg0_val, cfg1_val,
		drv_config->timeout_val);

	return 0;
}

static int cam_ife_csid_ver2_irq_comp_cfg(
	struct cam_ife_csid_ver2_hw *csid_hw, void *cmd_args, uint32_t arg_size)
{
	uint32_t                                    comp_cfg_mask = 0;
	size_t                                      size = 0;
	struct cam_isp_irq_comp_cfg                *comp_cfg;
	const struct cam_ife_csid_ver2_mc_reg_info *reg_info = NULL;
	struct cam_cdm_utils_ops                   *cdm_util_ops;
	struct cam_isp_hw_get_cmd_update           *cdm_args = NULL;
	uint32_t                                    reg_val_pair[2];

	if (arg_size != sizeof(struct cam_isp_hw_get_cmd_update)) {
		CAM_ERR(CAM_ISP, "CSID %u Invalid args size expected: %zu actual: %zu",
			csid_hw->hw_intf->hw_idx, sizeof(struct cam_isp_hw_get_cmd_update),
			arg_size);
		return -EINVAL;
	}

	cdm_args = (struct cam_isp_hw_get_cmd_update *)cmd_args;
	if (!cdm_args) {
		CAM_ERR(CAM_ISP, "CSID:%u Error, Invalid args", csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	comp_cfg = (struct cam_isp_irq_comp_cfg *)cdm_args->data;

	reg_info = ((struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg)->ipp_mc_reg;
	if (!reg_info) {
		CAM_DBG(CAM_ISP, "No multi-context support in CSID[%u]", csid_hw->hw_intf->hw_idx);
		return 0;
	}

	comp_cfg_mask = (comp_cfg->ipp_src_ctxt_mask << reg_info->ipp_src_ctxt_mask_shift) |
		(comp_cfg->ipp_dst_comp_mask << reg_info->ipp_dst_ctxt_mask_shift);

	cdm_util_ops = (struct cam_cdm_utils_ops *)cdm_args->res->cdm_ops;
	size = cdm_util_ops->cdm_required_size_reg_random(1);

	if ((size * 4) > cdm_args->cmd.size) {
		CAM_ERR(CAM_ISP, "CSID:%u buf size:%d is not sufficient, expected: %d",
			csid_hw->hw_intf->hw_idx, cdm_args->cmd.size, (size*4));
		return -EINVAL;
	}

	reg_val_pair[0] = reg_info->irq_comp_cfg0_addr;
	reg_val_pair[1] = comp_cfg_mask;
	cdm_util_ops->cdm_write_regrandom(cdm_args->cmd.cmd_buf_addr,
		1, reg_val_pair);
	cdm_args->cmd.used_bytes = size * 4;

	return 0;
}

static int cam_ife_csid_ver2_get_primary_sof_timer_reg_addr(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_ife_csid_ts_reg_addr *sof_addr)
{
	struct cam_hw_soc_info                       *soc_info;
	struct cam_isp_resource_node                 *csid_res;
	struct cam_ife_csid_ver2_reg_info            *csid_reg;
	const struct cam_ife_csid_ver2_path_reg_info *path_reg;

	if (!csid_hw || sof_addr->res_id >= CAM_IFE_PIX_PATH_RES_MAX) {
		CAM_ERR(CAM_ISP, "Invalid params, csid_hw is null: %s, res_id: %u",
			CAM_IS_NULL_TO_STR(csid_hw), sof_addr->res_id);
		return -EINVAL;
	}

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	csid_res = &csid_hw->path_res[sof_addr->res_id];
	path_reg = csid_reg->path_reg[sof_addr->res_id];
	soc_info = &csid_hw->hw_info->soc_info;

	sof_addr->curr0_ts_addr = soc_info->reg_map[0].mem_base +
		path_reg->timestamp_curr0_sof_addr;
	sof_addr->curr1_ts_addr = soc_info->reg_map[0].mem_base +
		path_reg->timestamp_curr1_sof_addr;

	return 0;
}

static int cam_ife_csid_ver2_get_num_dt_supported(
	struct cam_ife_csid_ver2_hw *csid_hw)
{
	struct cam_ife_csid_ver2_reg_info *csid_reg;
	int num_dt;

	if (unlikely(!csid_hw))
		return 0;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	if (!csid_reg->cmn_reg->multi_vcdt_supported)
		return 1;

	num_dt = csid_reg->cmn_reg->num_dt_supported ?
		csid_reg->cmn_reg->num_dt_supported : CAM_IFE_CSID_DEFAULT_NUM_DT;

	return num_dt;
}

static int cam_ife_csid_ver2_path_exp_info_update(
	struct cam_ife_csid_ver2_hw *csid_hw,
	struct cam_ife_csid_exp_info_update_args *exp_info)
{
	struct cam_ife_csid_ver2_path_cfg *path_cfg = NULL;
	int i;

	if (!csid_hw || !exp_info) {
		CAM_ERR(CAM_ISP, "Invalid params, csid_hw is null: %s, exp_info is null: %s",
			CAM_IS_NULL_TO_STR(csid_hw), CAM_IS_NULL_TO_STR(exp_info));
		return -EINVAL;
	}

	for (i = 0; i < CAM_IFE_PIX_PATH_RES_MAX; i++) {
		path_cfg =  (struct cam_ife_csid_ver2_path_cfg *)
				csid_hw->path_res[i].res_priv;
		if ((csid_hw->path_res[i].res_state == CAM_ISP_RESOURCE_STATE_STREAMING) &&
			path_cfg && path_cfg->allow_epoch_eof_cb){
			path_cfg->allow_epoch_eof_cb = false;
			CAM_WARN(CAM_ISP, "CSID: %u disabled epoch for res: %s, camif_irq_en:%d, irq_handle: %d aeb:%d",
					csid_hw->hw_intf->hw_idx, csid_hw->path_res[i].res_name, path_cfg->handle_camif_irq,
					path_cfg->irq_handle, path_cfg->is_aeb_en);
		}
	}

	if ((exp_info->num_sensor_out_exp > 1) &&
		(exp_info->num_sensor_out_exp > exp_info->num_process_exp)) {
		path_cfg =  (struct cam_ife_csid_ver2_path_cfg *)
			csid_hw->path_res[exp_info->last_exp_res_id].res_priv;
			CAM_INFO(CAM_ISP, "CSID: %u enabled epoch for res: %s",
					csid_hw->hw_intf->hw_idx,
					csid_hw->path_res[exp_info->last_exp_res_id].res_name);
	}
	else {
		path_cfg =  (struct cam_ife_csid_ver2_path_cfg *)
			csid_hw->path_res[CAM_IFE_PIX_PATH_RES_IPP].res_priv;
			CAM_INFO(CAM_ISP, "CSID: %u enabled epoch for ipp",
				csid_hw->hw_intf->hw_idx);
	}

	if (!path_cfg) {
		CAM_ERR(CAM_ISP, "NULL path_cfg for exp info update");
		return -EINVAL;
	}

	csid_hw->flags.last_exp_valid = exp_info->last_exp_valid;
	path_cfg->allow_epoch_eof_cb = true;
	CAM_INFO(CAM_ISP, "CSID: %u camif_irq_en:%d, irq_handle: %d aeb:%d",
		csid_hw->hw_intf->hw_idx, path_cfg->handle_camif_irq,
		path_cfg->irq_handle, path_cfg->is_aeb_en);

	return 0;
}

static int cam_ife_csid_ver2_process_cmd(void *hw_priv,
	uint32_t cmd_type, void *cmd_args, uint32_t arg_size)
{
	int rc = 0;
	struct cam_ife_csid_ver2_hw          *csid_hw;
	struct cam_hw_info                   *hw_info;

	if (!hw_priv || !cmd_args) {
		CAM_ERR(CAM_ISP, "CSID: Invalid arguments");
		return -EINVAL;
	}

	hw_info = (struct cam_hw_info  *)hw_priv;
	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_info->core_info;

	switch (cmd_type) {
	case CAM_IFE_CSID_CMD_GET_TIME_STAMP:
		rc = cam_ife_csid_ver2_get_time_stamp(csid_hw, cmd_args);

		if (csid_hw->debug_info.debug_val &	CAM_IFE_CSID_DEBUG_ENABLE_HBI_VBI_INFO)
			cam_ife_csid_ver2_print_hbi_vbi(csid_hw);
#if defined(CONFIG_SAMSUNG_CAMERA)
		else if (debug_hbi_vbi[0] > 0) {
			cam_ife_csid_ver2_print_hbi_vbi(csid_hw);
		}
#endif

		/* Reset CRC error count during SOF */
		csid_hw->counters.crc_error_irq_count = 0;
		break;
	case CAM_IFE_CSID_SET_CSID_DEBUG:
		rc = cam_ife_csid_ver2_set_debug(csid_hw,
			(struct cam_ife_csid_debug_cfg_args *)cmd_args);
		break;
	case CAM_IFE_CSID_SOF_IRQ_DEBUG:
		rc = cam_ife_csid_ver2_sof_irq_debug(csid_hw, cmd_args);
		break;
	case CAM_ISP_HW_CMD_CSID_CLOCK_UPDATE:
		rc = cam_ife_csid_ver2_set_csid_clock(csid_hw, cmd_args);
		break;
	case CAM_ISP_HW_CMD_DUMP_HW:
		break;
	case CAM_IFE_CSID_TOP_CONFIG:
		rc = cam_ife_csid_ver2_top_cfg(csid_hw, cmd_args);
		break;
	case CAM_ISP_HW_CMD_GET_CHANGE_BASE:
		rc = cam_ife_csid_get_base(&hw_info->soc_info,
			CAM_IFE_CSID_CLC_MEM_BASE_ID,
			cmd_args, arg_size);
		break;
	case CAM_ISP_HW_CMD_GET_REG_UPDATE:
		rc = cam_ife_csid_ver2_reg_update(csid_hw,
			cmd_args, arg_size);
		break;
	case CAM_IFE_CSID_SET_DUAL_SYNC_CONFIG:
		rc = cam_ife_csid_ver2_dual_sync_cfg(csid_hw,
			cmd_args);
		break;
	case CAM_ISP_HW_CSID_MINI_DUMP:
		rc  = cam_ife_csid_ver2_mini_dump(csid_hw, cmd_args);
		break;
	case CAM_ISP_HW_USER_DUMP:
		rc = cam_ife_csid_ver2_user_dump(csid_hw, cmd_args);
		break;
	case CAM_IFE_CSID_PROGRAM_OFFLINE_CMD:
		rc = cam_ife_csid_ver2_program_offline_go_cmd(
			csid_hw, cmd_args, arg_size);
		break;
	case CAM_ISP_HW_CMD_CSID_DYNAMIC_SWITCH_UPDATE:
		rc = cam_ife_csid_ver2_set_dynamic_switch_config(csid_hw, cmd_args);
		break;
	case CAM_ISP_HW_CMD_CSID_CHANGE_HALT_MODE:
		break;
	case CAM_ISP_HW_CMD_QUERY_REGSPACE_DATA: {
		struct cam_hw_soc_info *soc_info;

		soc_info = &csid_hw->hw_info->soc_info;
		*((struct cam_hw_soc_info **)cmd_args) = soc_info;
		break;
	}
	case CAM_ISP_HW_CMD_CSID_DISCARD_INIT_FRAMES:
		rc = cam_ife_csid_ver2_set_discard_frame_cfg(csid_hw, cmd_args);
		break;
	case CAM_ISP_HW_CMD_RDI_LCR_CFG:
		rc = cam_ife_csid_ver2_rdi_lcr_cfg(csid_hw, cmd_args);
		break;
	case CAM_IFE_CSID_LOG_ACQUIRE_DATA:
		/* Not supported on ver2 */
		rc = 0;
		break;
	case CAM_ISP_HW_CMD_INIT_CONFIG_UPDATE:
		rc = cam_ife_csid_init_config_update(cmd_args, arg_size);
		break;
	case CAM_ISP_HW_CMD_DRV_CONFIG:
		rc = cam_ife_csid_ver2_drv_config(csid_hw, cmd_args);
		break;
	case CAM_IFE_CSID_RESET_OUT_OF_SYNC_CNT:
		rc = cam_ife_csid_ver2_reset_out_of_sync_cnt(csid_hw, cmd_args);
		break;
	case CAM_ISP_HW_CMD_CSID_DUMP_CROP_REG:
		rc = cam_ife_csid_ver2_dump_crop_reg(hw_info, (uint32_t *)cmd_args);
		break;
	case CAM_ISP_HW_CMD_IRQ_COMP_CFG:
		rc = cam_ife_csid_ver2_irq_comp_cfg(csid_hw, cmd_args, arg_size);
		break;
	case CAM_ISP_HW_CMD_IRQ_INJECTION:
		rc = cam_ife_csid_ver2_irq_inject(csid_hw, cmd_args);
		break;
	case CAM_ISP_HW_CMD_DUMP_IRQ_DESCRIPTION:
		rc = cam_ife_csid_ver2_dump_irq_desc(csid_hw, cmd_args);
		break;
	case CAM_ISP_HW_CMD_GET_SET_PRIM_SOF_TS_ADDR: {
		struct cam_ife_csid_ts_reg_addr  *sof_addr_args =
			(struct cam_ife_csid_ts_reg_addr *)cmd_args;

		if (!sof_addr_args->get_addr) {
			CAM_ERR(CAM_ISP, "CSID does not support set of primary SOF ts addr");
			rc = -EINVAL;
		} else
			rc = cam_ife_csid_ver2_get_primary_sof_timer_reg_addr(csid_hw,
				sof_addr_args);
	}
		break;
	case CAM_ISP_HW_CMD_QUERY_CAP: {
		struct cam_isp_hw_cap *csid_cap = (struct cam_isp_hw_cap *) cmd_args;

		csid_cap->max_dt_supported = cam_ife_csid_ver2_get_num_dt_supported(csid_hw);

		if (!csid_cap->max_dt_supported)
			rc = -EINVAL;
	}
		break;
	case CAM_ISP_HW_CMD_EXP_INFO_UPDATE:
		rc = cam_ife_csid_ver2_path_exp_info_update(csid_hw, cmd_args);
		break;
	default:
		CAM_ERR(CAM_ISP, "CSID:%u unsupported cmd:%d",
			csid_hw->hw_intf->hw_idx, cmd_type);
		rc = -EINVAL;
		break;
	}
	return rc;

}

static irqreturn_t cam_ife_csid_irq(int irq_num, void *data)
{
	struct cam_ife_csid_ver2_hw *csid_hw = data;

	if (!csid_hw)
		return IRQ_NONE;

	return cam_irq_controller_handle_irq(irq_num,
		csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
		CAM_IRQ_EVT_GROUP_0);
}

static void cam_ife_csid_ver2_free_res(struct cam_ife_csid_ver2_hw *csid_hw)
{

	struct cam_isp_resource_node *res;
	uint32_t num_paths;
	int i;
	struct cam_ife_csid_ver2_reg_info *csid_reg;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;
	num_paths = csid_reg->cmn_reg->num_udis;

	for (i = 0; i < num_paths; i++) {
		res = &csid_hw->path_res[CAM_IFE_PIX_PATH_RES_UDI_0 + i];
		CAM_MEM_FREE(res->res_priv);
		res->res_priv = NULL;
	}

	num_paths = csid_reg->cmn_reg->num_rdis;

	for (i = 0; i < num_paths; i++) {
		res = &csid_hw->path_res[CAM_IFE_PIX_PATH_RES_RDI_0 + i];
		CAM_MEM_FREE(res->res_priv);
		res->res_priv = NULL;
	}

	CAM_MEM_FREE(csid_hw->path_res[CAM_IFE_PIX_PATH_RES_IPP].res_priv);
	csid_hw->path_res[CAM_IFE_PIX_PATH_RES_IPP].res_priv = NULL;
	CAM_MEM_FREE(csid_hw->path_res[CAM_IFE_PIX_PATH_RES_IPP_1].res_priv);
	csid_hw->path_res[CAM_IFE_PIX_PATH_RES_IPP_1].res_priv = NULL;
	CAM_MEM_FREE(csid_hw->path_res[CAM_IFE_PIX_PATH_RES_IPP_2].res_priv);
	csid_hw->path_res[CAM_IFE_PIX_PATH_RES_IPP_2].res_priv = NULL;
	CAM_MEM_FREE(csid_hw->path_res[CAM_IFE_PIX_PATH_RES_PPP].res_priv);
	csid_hw->path_res[CAM_IFE_PIX_PATH_RES_PPP].res_priv = NULL;
}

static int cam_ife_ver2_hw_alloc_res(
	struct cam_isp_resource_node *res,
	uint32_t res_type,
	struct cam_hw_intf   *hw_intf,
	uint32_t res_id)

{
	struct cam_ife_csid_ver2_path_cfg *path_cfg = NULL;

	path_cfg = CAM_MEM_ZALLOC(sizeof(*path_cfg), GFP_KERNEL);

	if (!path_cfg)
		return -ENOMEM;

	res->res_id = res_id;
	res->res_type = res_type;
	res->res_state = CAM_ISP_RESOURCE_STATE_AVAILABLE;
	res->hw_intf = hw_intf;
	res->res_priv = path_cfg;

	return 0;
}

static int cam_ife_csid_ver2_hw_init_path_res(
	struct cam_ife_csid_ver2_hw   *csid_hw)
{
	int rc = 0;
	int i;
	struct cam_ife_csid_ver2_reg_info *csid_reg;
	struct cam_isp_resource_node *res;
	int32_t res_id = -1;

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	/* Initialize the IPP resources */
	for (i = 0; i < csid_reg->cmn_reg->num_pix; i++) {
		switch (i) {
		case 0:
			res_id = CAM_IFE_PIX_PATH_RES_IPP;
			break;
		case 1:
			res_id = CAM_IFE_PIX_PATH_RES_IPP_1;
			break;
		case 2:
			res_id = CAM_IFE_PIX_PATH_RES_IPP_2;
			break;
		default:
			res_id = -1;
			break;
		}

		if (res_id == -1) {
			CAM_ERR(CAM_ISP, "CSID: %d Invalid res_id idx:%d",
				csid_hw->hw_intf->hw_idx, i);
			goto free_res;
		}

		res = &csid_hw->path_res[res_id];
		rc = cam_ife_ver2_hw_alloc_res(res, CAM_ISP_RESOURCE_PIX_PATH,
			csid_hw->hw_intf, res_id);
		if (rc) {
			CAM_ERR(CAM_ISP, "CSID: %d IPP_%d res init fail",
				csid_hw->hw_intf->hw_idx, i);
			goto free_res;
		}

		scnprintf(csid_hw->path_res[res_id].res_name, CAM_ISP_RES_NAME_LEN, "IPP_%d", i);
	}

	/* Initialize PPP resource */
	if (csid_reg->cmn_reg->num_ppp) {
		res = &csid_hw->path_res[CAM_IFE_PIX_PATH_RES_PPP];
		rc = cam_ife_ver2_hw_alloc_res(
			res,
			CAM_ISP_RESOURCE_PIX_PATH,
			csid_hw->hw_intf,
			CAM_IFE_PIX_PATH_RES_PPP);
		if (rc) {
			CAM_ERR(CAM_ISP, "CSID: %u PPP res init fail",
				csid_hw->hw_intf->hw_idx);
			goto free_res;
		}
		scnprintf(csid_hw->path_res[CAM_IFE_PIX_PATH_RES_PPP].res_name,
			CAM_ISP_RES_NAME_LEN, "PPP");
	}

	/* Initialize the RDI resource */
	for (i = 0; i < csid_reg->cmn_reg->num_rdis; i++) {
		/* res type is from RDI 0 to RDI3 */
		res = &csid_hw->path_res[CAM_IFE_PIX_PATH_RES_RDI_0 + i];
		rc = cam_ife_ver2_hw_alloc_res(
			res,
			CAM_ISP_RESOURCE_PIX_PATH,
			csid_hw->hw_intf,
			CAM_IFE_PIX_PATH_RES_RDI_0 + i);
		if (rc) {
			CAM_ERR(CAM_ISP, "CSID: %u RDI[%d] res init fail",
				csid_hw->hw_intf->hw_idx, i);
			goto free_res;
		}
		scnprintf(res->res_name, CAM_ISP_RES_NAME_LEN, "RDI_%d", i);
	}

	/* Initialize the UDI resource */
	for (i = 0; i < csid_reg->cmn_reg->num_udis; i++) {
		/* res type is from UDI0 to UDI3 */
		res = &csid_hw->path_res[CAM_IFE_PIX_PATH_RES_UDI_0 + i];
		rc = cam_ife_ver2_hw_alloc_res(
			res,
			CAM_ISP_RESOURCE_PIX_PATH,
			csid_hw->hw_intf,
			CAM_IFE_PIX_PATH_RES_UDI_0 + i);
		if (rc) {
			CAM_ERR(CAM_ISP, "CSID: %u UDI[%d] res init fail",
				csid_hw->hw_intf->hw_idx, i);
			goto free_res;
		}
		scnprintf(res->res_name, CAM_ISP_RES_NAME_LEN, "UDI_%d", i);
	}

	return rc;

free_res:
	cam_ife_csid_ver2_free_res(csid_hw);
	return rc;
}

static void cam_ife_csid_hw_deinit_irq(
	struct cam_ife_csid_ver2_hw *csid_hw)
{
	unsigned long flags;
	int i;
	struct cam_ife_csid_ver2_reg_info *csid_reg = (struct cam_ife_csid_ver2_reg_info *)
							csid_hw->core_info->csid_reg;

	for (i = CAM_IFE_CSID_TOP_IRQ_STATUS_REG0; i < csid_reg->num_top_regs; i++)
		cam_irq_controller_deinit(&csid_hw->top_irq_controller[i]);

	for (i = CAM_IFE_CSID_RX_IRQ_STATUS_REG0; i < csid_reg->num_rx_regs; i++)
		cam_irq_controller_deinit(&csid_hw->rx_irq_controller[i]);

	cam_irq_controller_deinit(&csid_hw->buf_done_irq_controller);

	for (i = 0; i < CAM_IFE_PIX_PATH_RES_MAX; i++) {
		if (csid_hw->path_irq_controller[i])
			cam_irq_controller_deinit(&csid_hw->path_irq_controller[i]);
	}

	spin_lock_irqsave(&csid_hw->path_payload_lock, flags);
	INIT_LIST_HEAD(&csid_hw->path_free_payload_list);
	for (i = 0; i < CAM_IFE_CSID_VER2_PAYLOAD_MAX; i++)
		INIT_LIST_HEAD(&csid_hw->path_evt_payload[i].list);
	spin_unlock_irqrestore(&csid_hw->path_payload_lock, flags);

	spin_lock_irqsave(&csid_hw->rx_payload_lock, flags);
	INIT_LIST_HEAD(&csid_hw->rx_free_payload_list);
	for (i = 0; i < CAM_IFE_CSID_VER2_PAYLOAD_MAX; i++)
		INIT_LIST_HEAD(&csid_hw->rx_evt_payload[i].list);
	spin_unlock_irqrestore(&csid_hw->rx_payload_lock, flags);
}

static int cam_ife_csid_hw_init_irq(
	struct cam_ife_csid_ver2_hw *csid_hw)
{
	int                                rc = 0;
	int                                i;
	int                                ipp_res_id;
	struct cam_hw_soc_info            *soc_info;
	void __iomem                      *mem_base;
	struct cam_ife_csid_ver2_reg_info *csid_reg;
	char                               name[CAM_CSID_IRQ_CTRL_NAME_LEN];

	csid_reg = (struct cam_ife_csid_ver2_reg_info *)
			csid_hw->core_info->csid_reg;

	soc_info = &csid_hw->hw_info->soc_info;
	mem_base = soc_info->reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;

	for (i = CAM_IFE_CSID_TOP_IRQ_STATUS_REG0; i < csid_reg->num_top_regs; i++) {
		snprintf(name, CAM_CSID_IRQ_CTRL_NAME_LEN, "csid%d_%s", csid_hw->hw_intf->hw_idx,
			cam_ife_csid_ver2_top_reg_name[i]);
		rc = cam_irq_controller_init(name, mem_base, &csid_reg->top_irq_reg_info[i],
			&csid_hw->top_irq_controller[i]);

		if (rc) {
			CAM_ERR(CAM_ISP, "CSID:%u Failed to init CSID %s irq controller rc = %d",
				csid_hw->hw_intf->hw_idx, cam_ife_csid_ver2_top_reg_name[i], rc);
			goto deinit_controller;
		}
	}

	for (i = CAM_IFE_CSID_RX_IRQ_STATUS_REG0; i < csid_reg->num_rx_regs; i++) {
		snprintf(name, CAM_CSID_IRQ_CTRL_NAME_LEN, "csid%d_%s", csid_hw->hw_intf->hw_idx,
			cam_ife_csid_ver2_rx_reg_name[i]);
		rc = cam_irq_controller_init(name, mem_base, &csid_reg->rx_irq_reg_info[i],
			&csid_hw->rx_irq_controller[i]);

		if (rc) {
			CAM_ERR(CAM_ISP, "CSID:%u Failed to init CSID rx irq controller rc = %d",
				csid_hw->hw_intf->hw_idx, rc);
			goto deinit_controller;
		}
	}

	snprintf(name, CAM_CSID_IRQ_CTRL_NAME_LEN, "csid%d_buf_done", csid_hw->hw_intf->hw_idx);
	rc = cam_irq_controller_init(name, mem_base, csid_reg->buf_done_irq_reg_info,
		&csid_hw->buf_done_irq_controller);
	if (rc) {
		CAM_ERR(CAM_ISP, "CSID:%u Failed to init CSID buf_done irq controller rc = %d",
			csid_hw->hw_intf->hw_idx, rc);
		goto deinit_controller;
	}

	for (i = 0; i < csid_reg->cmn_reg->num_pix; i++) {
		switch (i) {
		case 0:
			ipp_res_id = CAM_IFE_PIX_PATH_RES_IPP;
			break;
		case 1:
			ipp_res_id = CAM_IFE_PIX_PATH_RES_IPP_1;
			break;
		case 2:
			ipp_res_id = CAM_IFE_PIX_PATH_RES_IPP_2;
			break;
		default:
			ipp_res_id = -1;
			break;
		}

		if (ipp_res_id == -1) {
			CAM_ERR(CAM_ISP, "CSID: %d Invalid res_id idx:%d",
				csid_hw->hw_intf->hw_idx, i);
			goto deinit_controller;

		}

		snprintf(name, CAM_CSID_IRQ_CTRL_NAME_LEN, "csid%d_ipp%d",
			csid_hw->hw_intf->hw_idx, i);
		rc = cam_irq_controller_init(name, mem_base,
			csid_reg->path_irq_reg_info[ipp_res_id],
			&csid_hw->path_irq_controller[ipp_res_id]);

		if (rc) {
			CAM_ERR(CAM_ISP, "CSID:%u Failed to init CSID ipp%d irq controller rc = %d",
				csid_hw->hw_intf->hw_idx, i, rc);
			goto deinit_controller;
		}
	}

	if (csid_reg->cmn_reg->num_ppp) {
		snprintf(name, CAM_CSID_IRQ_CTRL_NAME_LEN, "csid%d_ppp", csid_hw->hw_intf->hw_idx);
		rc = cam_irq_controller_init(name, mem_base,
			csid_reg->path_irq_reg_info[CAM_IFE_PIX_PATH_RES_PPP],
			&csid_hw->path_irq_controller[CAM_IFE_PIX_PATH_RES_PPP]);

		if (rc) {
			CAM_ERR(CAM_ISP,
				"CSID:%u Failed to init CSID ppp irq controller rc = %d",
				csid_hw->hw_intf->hw_idx, rc);
			goto deinit_controller;
		}
	}

	for (i = 0; i < csid_reg->cmn_reg->num_rdis; i++) {
		snprintf(name, CAM_CSID_IRQ_CTRL_NAME_LEN, "csid%d_rdi%d",
			csid_hw->hw_intf->hw_idx, i);
		rc = cam_irq_controller_init(name, mem_base,
			csid_reg->path_irq_reg_info[CAM_IFE_PIX_PATH_RES_RDI_0 + i],
			&csid_hw->path_irq_controller[CAM_IFE_PIX_PATH_RES_RDI_0 + i]);
		if (rc) {
			CAM_ERR(CAM_ISP,
				"CSID:%u Failed to init CSID rdi%d irq controller rc = %d",
				csid_hw->hw_intf->hw_idx, i, rc);
			goto deinit_controller;
		}
	}

	spin_lock_init(&csid_hw->path_payload_lock);
	INIT_LIST_HEAD(&csid_hw->path_free_payload_list);
	for (i = 0; i < CAM_IFE_CSID_VER2_PAYLOAD_MAX; i++) {
		INIT_LIST_HEAD(&csid_hw->path_evt_payload[i].list);
		list_add_tail(&csid_hw->path_evt_payload[i].list,
			&csid_hw->path_free_payload_list);
	}
	spin_lock_init(&csid_hw->rx_payload_lock);
	INIT_LIST_HEAD(&csid_hw->rx_free_payload_list);
	for (i = 0; i < CAM_IFE_CSID_VER2_PAYLOAD_MAX; i++) {
		INIT_LIST_HEAD(&csid_hw->rx_evt_payload[i].list);
		list_add_tail(&csid_hw->rx_evt_payload[i].list,
			&csid_hw->rx_free_payload_list);
	}

	return 0;

deinit_controller:
	cam_ife_csid_hw_deinit_irq(csid_hw);
	return rc;
}

int cam_ife_csid_ver2_irq_line_test(void *hw_priv)
{
	struct cam_ife_csid_ver2_hw *csid_hw;
	struct cam_hw_soc_info *soc_info;
	int rc = 0;
	void __iomem *mem_base;
	struct cam_ife_csid_ver2_reg_info *csid_reg;

	if (!hw_priv) {
		CAM_ERR(CAM_ISP, "CSID: Invalid args");
		return -EINVAL;
	}

	csid_hw = ((struct cam_hw_info *)hw_priv)->core_info;
	soc_info = &csid_hw->hw_info->soc_info;

	mem_base = csid_hw->hw_info->soc_info.reg_map[CAM_IFE_CSID_CLC_MEM_BASE_ID].mem_base;
	csid_reg = csid_hw->core_info->csid_reg;
	rc = cam_ife_csid_enable_soc_resources(soc_info, soc_info->lowest_clk_level);
	if (rc) {
		CAM_ERR(CAM_ISP, "CSID[%u] Enable soc failed", csid_hw->hw_intf->hw_idx);
		return rc;
	}

	CAM_DBG(CAM_ISP, "CSID[%u] hw-version:0x%x",
		csid_hw->hw_intf->hw_idx,
		cam_io_r_mb(mem_base + csid_reg->cmn_reg->hw_version_addr));

	rc = cam_irq_controller_test_irq_line(
		csid_hw->top_irq_controller[CAM_IFE_CSID_TOP_IRQ_STATUS_REG0],
		"CSID:%d", csid_hw->hw_intf->hw_idx);
	if (rc)
		CAM_ERR(CAM_ISP, "CSID[%u] IRQ line test failed", csid_hw->hw_intf->hw_idx);

	cam_ife_csid_disable_soc_resources(soc_info);
	return rc;
}

int cam_ife_csid_hw_ver2_init(struct cam_hw_intf *hw_intf,
	struct cam_ife_csid_core_info *core_info,
	bool is_custom)
{
	int rc = -EINVAL;
	struct cam_hw_info                   *hw_info;
	struct cam_ife_csid_ver2_hw          *csid_hw = NULL;

	if (!hw_intf || !core_info) {
		CAM_ERR(CAM_ISP, "Invalid parameters intf: %pK hw_info: %pK",
			hw_intf, core_info);
		return rc;
	}

	hw_info = (struct cam_hw_info  *)hw_intf->hw_priv;

	csid_hw = CAM_MEM_ZALLOC(sizeof(struct cam_ife_csid_ver2_hw), GFP_KERNEL);

	if (!csid_hw) {
		CAM_ERR(CAM_ISP, "Csid core %d hw allocation fails",
			hw_intf->hw_idx);
		return -ENOMEM;
	}

	hw_info->core_info = csid_hw;
	csid_hw->hw_intf = hw_intf;
	csid_hw->hw_info = hw_info;
	csid_hw->core_info = core_info;
	CAM_DBG(CAM_ISP, "type %d index %d",
		hw_intf->hw_type,
		hw_intf->hw_idx);

	csid_hw->flags.device_enabled = false;
	csid_hw->hw_info->hw_state = CAM_HW_STATE_POWER_DOWN;
	mutex_init(&csid_hw->hw_info->hw_mutex);
	spin_lock_init(&csid_hw->hw_info->hw_lock);
	spin_lock_init(&csid_hw->lock_state);
	init_completion(&csid_hw->hw_info->hw_complete);
	atomic_set(&csid_hw->discard_frame_per_path, 0);

	rc = cam_ife_csid_init_soc_resources(&csid_hw->hw_info->soc_info,
			cam_ife_csid_irq, cam_ife_csid_ver2_cpas_cb, csid_hw, is_custom);
	if (rc < 0) {
		CAM_ERR(CAM_ISP, "CSID:%u Failed to init_soc",
			hw_intf->hw_idx);
		return rc;
	}

	if (cam_cpas_is_feature_supported(CAM_CPAS_QCFA_BINNING_ENABLE,
		CAM_CPAS_HW_IDX_ANY, NULL))
		csid_hw->flags.binning_enabled = true;

	if (cam_cpas_query_domain_id_security_support())
		csid_hw->flags.domain_id_security = true;

	csid_hw->hw_intf->hw_ops.get_hw_caps   = cam_ife_csid_ver2_get_hw_caps;
	csid_hw->hw_intf->hw_ops.init          = cam_ife_csid_ver2_init_hw;
	csid_hw->hw_intf->hw_ops.deinit        = cam_ife_csid_ver2_deinit_hw;
	csid_hw->hw_intf->hw_ops.reset         = cam_ife_csid_ver2_reset;
	csid_hw->hw_intf->hw_ops.reserve       = cam_ife_csid_ver2_reserve;
	csid_hw->hw_intf->hw_ops.release       = cam_ife_csid_ver2_release;
	csid_hw->hw_intf->hw_ops.start         = cam_ife_csid_ver2_start;
	csid_hw->hw_intf->hw_ops.stop          = cam_ife_csid_ver2_stop;
	csid_hw->hw_intf->hw_ops.read          = cam_ife_csid_ver2_read;
	csid_hw->hw_intf->hw_ops.write         = cam_ife_csid_ver2_write;
	csid_hw->hw_intf->hw_ops.process_cmd   = cam_ife_csid_ver2_process_cmd;
	csid_hw->hw_intf->hw_ops.test_irq_line = cam_ife_csid_ver2_irq_line_test;

	rc = cam_ife_csid_hw_init_irq(csid_hw);
	if (rc) {
		CAM_ERR(CAM_ISP, "Failed to init CSID irq");
		return rc;
	}

	rc = cam_ife_csid_ver2_hw_init_path_res(csid_hw);

	if (rc) {
		CAM_ERR(CAM_ISP, "CSID[%u] Probe Init failed",
			hw_intf->hw_idx);
		return rc;
	}
	csid_hw->debug_info.debug_val = 0;
	csid_hw->counters.error_irq_count = 0;
	csid_hw->debug_info.set_domain_id_enabled = false;
	csid_hw->debug_info.domain_id_value = 0;

	return 0;

}
EXPORT_SYMBOL(cam_ife_csid_hw_ver2_init);

int cam_ife_csid_hw_ver2_deinit(struct cam_hw_info *hw_priv)
{
	struct cam_ife_csid_ver2_hw   *csid_hw;
	int rc = -EINVAL;

	csid_hw = (struct cam_ife_csid_ver2_hw *)hw_priv->core_info;

	if (!csid_hw) {
		CAM_ERR(CAM_ISP, "Invalid param");
		return rc;
	}

	cam_ife_csid_hw_deinit_irq(csid_hw);

	/* release the privdate data memory from resources */
	cam_ife_csid_ver2_free_res(csid_hw);

	cam_ife_csid_deinit_soc_resources(&csid_hw->hw_info->soc_info);

	return 0;
}
EXPORT_SYMBOL(cam_ife_csid_hw_ver2_deinit);
