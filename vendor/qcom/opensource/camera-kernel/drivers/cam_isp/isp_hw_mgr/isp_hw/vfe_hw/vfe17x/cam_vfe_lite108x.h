/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */


#ifndef _CAM_VFE_LITE108X_H_
#define _CAM_VFE_LITE108X_H_
#include "cam_vfe_camif_ver3.h"
#include "cam_vfe_top_ver4.h"
#include "cam_vfe_core.h"
#include "cam_vfe_bus_ver3.h"
#include "cam_irq_controller.h"

#define CAM_VFE_108X_NUM_DBG_REG 5

static struct cam_vfe_top_ver4_module_desc vfe_lite108x_ipp_mod_desc[] = {
	{
		.id = 0,
		.desc = "CLC_BLS",
	},
	{
		.id = 1,
		.desc = "CLC_GLUT",
	},
	{
		.id = 2,
		.desc = "CLC_STATS_BG",
	},
};

static struct cam_vfe_top_ver4_wr_client_desc vfe_lite108x_wr_client_desc[] = {
	{
		.wm_id = 0,
		.desc = "RDI_0",
	},
	{
		.wm_id = 1,
		.desc = "RDI_1",
	},
	{
		.wm_id = 2,
		.desc = "RDI_2",
	},
	{
		.wm_id = 3,
		.desc = "RDI_3",
	},
	{
		.wm_id = 4,
		.desc = "GAMMA",
	},
	{
		.wm_id = 5,
		.desc = "BE",
	},
};

/* All the Offsets are relative to ifelite start address */

static struct cam_irq_register_set vfe_lite108x_top_irq_reg_set[2] = {
	{
		.mask_reg_offset   = 0x00000164,
		.clear_reg_offset  = 0x00000174,
		.status_reg_offset = 0x00000154,
		.set_reg_offset    = 0x00000184,
		.test_set_val      = BIT(0),
		.test_sub_val      = BIT(0),
	},
	{
		.mask_reg_offset   = 0x00000168,
		.clear_reg_offset  = 0x00000178,
		.status_reg_offset = 0x00000158,
	},
};

static struct cam_irq_controller_reg_info vfe_lite108x_top_irq_reg_info = {
	.num_registers = 2,
	.irq_reg_set = vfe_lite108x_top_irq_reg_set,
	.global_irq_cmd_offset = 0x00000188,
	.global_clear_bitmask  = 0x00000001,
	.global_set_bitmask    = 0x00000010,
	.clear_all_bitmask     = 0xFFFFFFFF,
};

static uint32_t vfe_lite108x_top_debug_reg[] = {
	0x0000038C,
	0x00000390,
	0x00000394,
	0x00000398,
	0x0000039C,
};

static struct cam_vfe_top_ver4_reg_offset_common vfe_lite108x_top_common_reg = {
	.hw_version               = 0x00000000,
	.hw_capability            = 0x0,
	.core_cgc_ovd_0           = 0x00000104,
	.ahb_cgc_ovd              = 0x00000108,
	.core_cfg_0               = 0x00000114,
	.diag_config              = 0x00000254,
	.diag_config_1            = 0x00000258,
	.diag_sensor_status       = {0x0000025C, 0x00000260},
	.diag_frm_cnt_status      = {0x00000264, 0x00000268},
	.ipp_violation_status     = 0x000002A4,
	.bus_violation_status     = 0x00000864,
	.bus_overflow_status      = 0x00000868,
	.top_debug_cfg            = 0x000003DC,
	.num_top_debug_reg        = CAM_VFE_108X_NUM_DBG_REG,
	.top_debug                = vfe_lite108x_top_debug_reg,
};

static struct cam_vfe_ver4_path_reg_data vfe_lite108x_ipp_reg_data = {
	.sof_irq_mask                    = 0x1,
	.eof_irq_mask                    = 0x2,
	.error_irq_mask                  = 0x6,
	.enable_diagnostic_hw            = 0x1,
	.top_debug_cfg_en                = 0x3,
	.ipp_violation_mask              = 0x10,
	.diag_violation_mask             = 0x4,
	.diag_sensor_sel_mask            = 0x40,
	.diag_frm_count_mask_1           = 0x100,
};

static struct cam_vfe_ver4_path_reg_data vfe_lite108x_rdi_reg_data[4] = {

	{
		.sof_irq_mask                    = 0x4,
		.eof_irq_mask                    = 0x8,
		.error_irq_mask                  = 0x0,
		.diag_sensor_sel_mask            = 0x0,
		.diag_frm_count_mask_0           = 0x80,
		.enable_diagnostic_hw            = 0x1,
		.top_debug_cfg_en                = 0x3,
	},
	{
		.sof_irq_mask                    = 0x10,
		.eof_irq_mask                    = 0x20,
		.error_irq_mask                  = 0x0,
		.diag_sensor_sel_mask            = 0x2,
		.diag_frm_count_mask_0           = 0x100,
		.enable_diagnostic_hw            = 0x1,
		.top_debug_cfg_en                = 0x3,
	},
	{
		.sof_irq_mask                    = 0x40,
		.eof_irq_mask                    = 0x80,
		.error_irq_mask                  = 0x0,
		.diag_sensor_sel_mask            = 0x4,
		.diag_frm_count_mask_0           = 0x200,
		.enable_diagnostic_hw            = 0x1,
		.top_debug_cfg_en                = 0x3,
	},
	{
		.sof_irq_mask                    = 0x100,
		.eof_irq_mask                    = 0x200,
		.error_irq_mask                  = 0x0,
		.diag_sensor_sel_mask            = 0x6,
		.diag_frm_count_mask_0           = 0x400,
		.enable_diagnostic_hw            = 0x1,
		.top_debug_cfg_en                = 0x3,
	},
};

static struct cam_vfe_ver4_path_hw_info
	vfe_lite108x_rdi_hw_info[] = {
	{
		.common_reg     = &vfe_lite108x_top_common_reg,
		.reg_data       = &vfe_lite108x_rdi_reg_data[0],
	},
	{
		.common_reg     = &vfe_lite108x_top_common_reg,
		.reg_data       = &vfe_lite108x_rdi_reg_data[1],
	},
	{
		.common_reg     = &vfe_lite108x_top_common_reg,
		.reg_data       = &vfe_lite108x_rdi_reg_data[2],
	},
	{
		.common_reg     = &vfe_lite108x_top_common_reg,
		.reg_data       = &vfe_lite108x_rdi_reg_data[3],
	},
};

static struct cam_vfe_top_ver4_debug_reg_info vfe108x_dbg_reg_info[CAM_VFE_108X_NUM_DBG_REG][8] = {
	VFE_DBG_INFO_ARRAY_4bit(
		"test_bus_reserved",
		"test_bus_reserved",
		"test_bus_reserved",
		"test_bus_reserved",
		"test_bus_reserved",
		"test_bus_reserved",
		"test_bus_reserved",
		"test_bus_reserved"
	),
	{
		VFE_DBG_INFO(32, "non-CLC info"),
		VFE_DBG_INFO(32, "non-CLC info"),
		VFE_DBG_INFO(32, "non-CLC info"),
		VFE_DBG_INFO(32, "non-CLC info"),
		VFE_DBG_INFO(32, "non-CLC info"),
		VFE_DBG_INFO(32, "non-CLC info"),
		VFE_DBG_INFO(32, "non-CLC info"),
		VFE_DBG_INFO(32, "non-CLC info"),
	},
	VFE_DBG_INFO_ARRAY_4bit(
		"PP_THROTTLE",
		"STATS_BG_THROTTLE",
		"STATS_BG",
		"BLS",
		"GLUT",
		"unused",
		"unused",
		"unused"
	),
	VFE_DBG_INFO_ARRAY_4bit(
		"RDI_0",
		"RDI_1",
		"RDI_2",
		"RDI_3",
		"PP_STATS_BG",
		"PP_GLUT",
		"PP_STATS_BG",
		"PP_GLUT"
	),
	VFE_DBG_INFO_ARRAY_4bit(
		"unused",
		"unused",
		"unused",
		"unused",
		"unused",
		"unused",
		"unused",
		"unused"
	),
};

static struct cam_vfe_top_ver4_diag_reg_info vfe_lite108x_diag_reg_info[] = {
	{
		.bitmask = 0x3FFF,
		.name    = "SENSOR_HBI",
	},
	{
		.bitmask = 0x4000,
		.name    = "SENSOR_NEQ_HBI",
	},
	{
		.bitmask = 0x8000,
		.name    = "SENSOR_HBI_MIN_ERROR",
	},
	{
		.bitmask = 0xFFFFFF,
		.name    = "SENSOR_VBI",
	},
	{
		.bitmask = 0xFF,
		.name    = "FRAME_CNT_RDI_0_PIPE",
	},
	{
		.bitmask = 0xFF00,
		.name    = "FRAME_CNT_RDI_1_PIPE",
	},
	{
		.bitmask = 0xFF0000,
		.name    = "FRAME_CNT_RDI_2_PIPE",
	},
	{
		.bitmask = 0xFF000000,
		.name    = "FRAME_CNT_RDI_3_PIPE",
	},
	{
		.bitmask = 0xFF,
		.name    = "FRAME_CNT_IPP_PIPE",
	},
};

static struct cam_vfe_top_ver4_diag_reg_fields vfe_lite108x_diag_sensor_field[] = {
	{
		.num_fields = 3,
		.field      = &vfe_lite108x_diag_reg_info[0],
	},
	{
		.num_fields = 1,
		.field      = &vfe_lite108x_diag_reg_info[3],
	},
};

static struct cam_vfe_top_ver4_diag_reg_fields vfe_lite108x_diag_frame_field[] = {
	{
		.num_fields = 4,
		.field      = &vfe_lite108x_diag_reg_info[4],
	},
	{
		.num_fields = 1,
		.field      = &vfe_lite108x_diag_reg_info[8],
	},
};

static struct cam_vfe_top_ver4_hw_info vfe_lite108x_top_hw_info = {
	.common_reg = &vfe_lite108x_top_common_reg,
	.rdi_hw_info = vfe_lite108x_rdi_hw_info,
	.vfe_full_hw_info = {
		.common_reg     = &vfe_lite108x_top_common_reg,
		.reg_data       = &vfe_lite108x_ipp_reg_data,
	},
	.ipp_module_desc        = vfe_lite108x_ipp_mod_desc,
	.wr_client_desc         = vfe_lite108x_wr_client_desc,
	.num_mux = 5,
	.mux_type = {
		CAM_VFE_CAMIF_VER_4_0,
		CAM_VFE_RDI_VER_1_0,
		CAM_VFE_RDI_VER_1_0,
		CAM_VFE_RDI_VER_1_0,
		CAM_VFE_RDI_VER_1_0,
	},
	.top_debug_reg_info = &vfe108x_dbg_reg_info,
	.num_rdi        = ARRAY_SIZE(vfe_lite108x_rdi_hw_info),
	.diag_sensor_info = vfe_lite108x_diag_sensor_field,
	.diag_frame_info  = vfe_lite108x_diag_frame_field,
};

static struct cam_irq_register_set vfe_lite108x_bus_irq_reg[1] = {
	{
		.mask_reg_offset   = 0x00000818,
		.clear_reg_offset  = 0x00000820,
		.status_reg_offset = 0x00000828,
	},
};

static uint32_t vfe_lite108x_out_port_mid[][4] = {
	{32, 0, 0, 0},
	{33, 0, 0, 0},
	{34, 0, 0, 0},
	{35, 0, 0, 0},
	{36, 0, 0, 0},
	{37, 0, 0, 0},
};

static struct cam_vfe_bus_ver3_hw_info vfe_lite108x_bus_hw_info = {
	.common_reg = {
		.hw_version                       = 0x00000800,
		.cgc_ovd                          = 0x00000808,
		.pwr_iso_cfg                      = 0x0000085C,
		.overflow_status_clear            = 0x00000860,
		.ccif_violation_status            = 0x00000864,
		.overflow_status                  = 0x00000868,
		.image_size_violation_status      = 0x00000870,
		.debug_status_top_cfg             = 0x000008F0,
		.debug_status_top                 = 0x000008F4,
		.test_bus_ctrl                    = 0x00000928,
		.wm_mode_shift                    = 16,
		.wm_mode_val                      = { 0x0, 0x1, 0x2 },
		.wm_en_shift                      = 0,
		.frmheader_en_shift               = 2,
		.virtual_frm_en_shift             = 1,
		.irq_reg_info = {
			.num_registers            = 1,
			.irq_reg_set              = vfe_lite108x_bus_irq_reg,
			.global_irq_cmd_offset    = 0x00000830,
			.global_clear_bitmask     = 0x00000001,
		},
	},
	.num_client = 6,
	.bus_client_reg = {
		/* BUS Client 0 RDI0 */
		{
			.cfg                      = 0x00000D00,
			.image_addr               = 0x00000D04,
			.frame_incr               = 0x00000D08,
			.image_cfg_0              = 0x00000D0C,
			.image_cfg_1              = 0x00000D10,
			.image_cfg_2              = 0x00000D14,
			.packer_cfg               = 0x00000D18,
			.frame_header_addr        = 0x00000D20,
			.frame_header_incr        = 0x00000D24,
			.frame_header_cfg         = 0x00000D28,
			.line_done_cfg            = 0x00000D2C,
			.irq_subsample_period     = 0x00000D30,
			.irq_subsample_pattern    = 0x00000D34,
			.mmu_prefetch_cfg         = 0x00000D60,
			.mmu_prefetch_max_offset  = 0x00000D64,
			.system_cache_cfg         = 0x00000D68,
			.addr_cfg                 = 0x00000D70,
			.addr_status_0            = 0x00000D90,
			.addr_status_1            = 0x00000D94,
			.addr_status_2            = 0x00000D98,
			.addr_status_3            = 0x00000D9C,
			.debug_status_cfg         = 0x00000D7C,
			.debug_status_0           = 0x00000D80,
			.debug_status_1           = 0x00000D84,
			.comp_group               = CAM_VFE_BUS_VER3_COMP_GRP_1,
			.ubwc_regs                = NULL,
			.supported_formats        = BIT_ULL(CAM_FORMAT_MIPI_RAW_10) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_6) | BIT_ULL(CAM_FORMAT_MIPI_RAW_8) |
				BIT_ULL(CAM_FORMAT_YUV422) | BIT_ULL(CAM_FORMAT_MIPI_RAW_12) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_14) | BIT_ULL(CAM_FORMAT_MIPI_RAW_16) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_20) | BIT_ULL(CAM_FORMAT_PLAIN128) |
				BIT_ULL(CAM_FORMAT_PLAIN32_20) | BIT_ULL(CAM_FORMAT_PLAIN8) |
				BIT_ULL(CAM_FORMAT_PLAIN16_10) | BIT_ULL(CAM_FORMAT_PLAIN16_12) |
				BIT_ULL(CAM_FORMAT_PLAIN16_14) | BIT_ULL(CAM_FORMAT_PLAIN16_16) |
				BIT_ULL(CAM_FORMAT_PLAIN64) | BIT_ULL(CAM_FORMAT_YUV422_10),
		},
		/* BUS Client 1 RDI1 */
		{
			.cfg                      = 0x00000E00,
			.image_addr               = 0x00000E04,
			.frame_incr               = 0x00000E08,
			.image_cfg_0              = 0x00000E0C,
			.image_cfg_1              = 0x00000E10,
			.image_cfg_2              = 0x00000E14,
			.packer_cfg               = 0x00000E18,
			.frame_header_addr        = 0x00000E20,
			.frame_header_incr        = 0x00000E24,
			.frame_header_cfg         = 0x00000E28,
			.line_done_cfg            = 0x00000E2C,
			.irq_subsample_period     = 0x00000E30,
			.irq_subsample_pattern    = 0x00000E34,
			.mmu_prefetch_cfg         = 0x00000E60,
			.mmu_prefetch_max_offset  = 0x00000E64,
			.system_cache_cfg         = 0x00000E68,
			.addr_cfg                 = 0x00000E70,
			.addr_status_0            = 0x00000E90,
			.addr_status_1            = 0x00000E94,
			.addr_status_2            = 0x00000E98,
			.addr_status_3            = 0x00000E9C,
			.debug_status_cfg         = 0x00000E7C,
			.debug_status_0           = 0x00000E80,
			.debug_status_1           = 0x00000E84,
			.comp_group               = CAM_VFE_BUS_VER3_COMP_GRP_2,
			.ubwc_regs                = NULL,
			.supported_formats        = BIT_ULL(CAM_FORMAT_MIPI_RAW_10) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_6) | BIT_ULL(CAM_FORMAT_MIPI_RAW_8) |
				BIT_ULL(CAM_FORMAT_YUV422) | BIT_ULL(CAM_FORMAT_MIPI_RAW_12) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_14) | BIT_ULL(CAM_FORMAT_MIPI_RAW_16) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_20) | BIT_ULL(CAM_FORMAT_PLAIN128) |
				BIT_ULL(CAM_FORMAT_PLAIN32_20) | BIT_ULL(CAM_FORMAT_PLAIN8) |
				BIT_ULL(CAM_FORMAT_PLAIN16_10) | BIT_ULL(CAM_FORMAT_PLAIN16_12) |
				BIT_ULL(CAM_FORMAT_PLAIN16_14) | BIT_ULL(CAM_FORMAT_PLAIN16_16) |
				BIT_ULL(CAM_FORMAT_PLAIN64) | BIT_ULL(CAM_FORMAT_YUV422_10),
		},
		/* BUS Client 2 RDI2 */
		{
			.cfg                      = 0x00000F00,
			.image_addr               = 0x00000F04,
			.frame_incr               = 0x00000F08,
			.image_cfg_0              = 0x00000F0C,
			.image_cfg_1              = 0x00000F10,
			.image_cfg_2              = 0x00000F14,
			.packer_cfg               = 0x00000F18,
			.frame_header_addr        = 0x00000F20,
			.frame_header_incr        = 0x00000F24,
			.frame_header_cfg         = 0x00000F28,
			.line_done_cfg            = 0x00000F2C,
			.irq_subsample_period     = 0x00000F30,
			.irq_subsample_pattern    = 0x00000F34,
			.mmu_prefetch_cfg         = 0x00000F60,
			.mmu_prefetch_max_offset  = 0x00000F64,
			.system_cache_cfg         = 0x00000F68,
			.addr_cfg                 = 0x00000F70,
			.addr_status_0            = 0x00000F90,
			.addr_status_1            = 0x00000F94,
			.addr_status_2            = 0x00000F98,
			.addr_status_3            = 0x00000F9C,
			.debug_status_cfg         = 0x00000F7C,
			.debug_status_0           = 0x00000F80,
			.debug_status_1           = 0x00000F84,
			.comp_group               = CAM_VFE_BUS_VER3_COMP_GRP_3,
			.ubwc_regs                = NULL,
			.supported_formats        = BIT_ULL(CAM_FORMAT_MIPI_RAW_10) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_6) | BIT_ULL(CAM_FORMAT_MIPI_RAW_8) |
				BIT_ULL(CAM_FORMAT_YUV422) | BIT_ULL(CAM_FORMAT_MIPI_RAW_12) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_14) | BIT_ULL(CAM_FORMAT_MIPI_RAW_16) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_20) | BIT_ULL(CAM_FORMAT_PLAIN128) |
				BIT_ULL(CAM_FORMAT_PLAIN32_20) | BIT_ULL(CAM_FORMAT_PLAIN8) |
				BIT_ULL(CAM_FORMAT_PLAIN16_10) | BIT_ULL(CAM_FORMAT_PLAIN16_12) |
				BIT_ULL(CAM_FORMAT_PLAIN16_14) | BIT_ULL(CAM_FORMAT_PLAIN16_16) |
				BIT_ULL(CAM_FORMAT_PLAIN64) | BIT_ULL(CAM_FORMAT_YUV422_10),
		},
		/* BUS Client 3 RDI3 */
		{
			.cfg                      = 0x00001000,
			.image_addr               = 0x00001004,
			.frame_incr               = 0x00001008,
			.image_cfg_0              = 0x0000100C,
			.image_cfg_1              = 0x00001010,
			.image_cfg_2              = 0x00001014,
			.packer_cfg               = 0x00001018,
			.frame_header_addr        = 0x00001020,
			.frame_header_incr        = 0x00001024,
			.frame_header_cfg         = 0x00001028,
			.line_done_cfg            = 0x0000102C,
			.irq_subsample_period     = 0x00001030,
			.irq_subsample_pattern    = 0x00001034,
			.mmu_prefetch_cfg         = 0x00001060,
			.mmu_prefetch_max_offset  = 0x00001064,
			.system_cache_cfg         = 0x00001068,
			.addr_cfg                 = 0x00001070,
			.addr_status_0            = 0x00001090,
			.addr_status_1            = 0x00001094,
			.addr_status_2            = 0x00001098,
			.addr_status_3            = 0x0000109C,
			.debug_status_cfg         = 0x0000107C,
			.debug_status_0           = 0x00001080,
			.debug_status_1           = 0x00001084,
			.comp_group               = CAM_VFE_BUS_VER3_COMP_GRP_4,
			.ubwc_regs                = NULL,
			.supported_formats        = BIT_ULL(CAM_FORMAT_MIPI_RAW_10) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_6) | BIT_ULL(CAM_FORMAT_MIPI_RAW_8) |
				BIT_ULL(CAM_FORMAT_YUV422) | BIT_ULL(CAM_FORMAT_MIPI_RAW_12) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_14) | BIT_ULL(CAM_FORMAT_MIPI_RAW_16) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_20) | BIT_ULL(CAM_FORMAT_PLAIN128) |
				BIT_ULL(CAM_FORMAT_PLAIN32_20) | BIT_ULL(CAM_FORMAT_PLAIN8) |
				BIT_ULL(CAM_FORMAT_PLAIN16_10) | BIT_ULL(CAM_FORMAT_PLAIN16_12) |
				BIT_ULL(CAM_FORMAT_PLAIN16_14) | BIT_ULL(CAM_FORMAT_PLAIN16_16) |
				BIT_ULL(CAM_FORMAT_PLAIN64) | BIT_ULL(CAM_FORMAT_YUV422_10),
		},
		/* BUS Client 4 Gamma */
		{
			.cfg                      = 0x00001100,
			.image_addr               = 0x00001104,
			.frame_incr               = 0x00001108,
			.image_cfg_0              = 0x0000110C,
			.image_cfg_1              = 0x00001110,
			.image_cfg_2              = 0x00001114,
			.packer_cfg               = 0x00001118,
			.frame_header_addr        = 0x00001120,
			.frame_header_incr        = 0x00001124,
			.frame_header_cfg         = 0x00001128,
			.line_done_cfg            = 0x0000112C,
			.irq_subsample_period     = 0x00001130,
			.irq_subsample_pattern    = 0x00001134,
			.mmu_prefetch_cfg         = 0x00001160,
			.mmu_prefetch_max_offset  = 0x00001164,
			.system_cache_cfg         = 0x00001168,
			.addr_cfg                 = 0x00001170,
			.addr_status_0            = 0x00001190,
			.addr_status_1            = 0x00001194,
			.addr_status_2            = 0x00001198,
			.addr_status_3            = 0x0000119C,
			.debug_status_cfg         = 0x0000117C,
			.debug_status_0           = 0x00001180,
			.debug_status_1           = 0x00001184,
			.comp_group               = CAM_VFE_BUS_VER3_COMP_GRP_0,
			.ubwc_regs                = NULL,
			.supported_formats        = BIT_ULL(CAM_FORMAT_MIPI_RAW_10) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_6) | BIT_ULL(CAM_FORMAT_MIPI_RAW_8) |
				BIT_ULL(CAM_FORMAT_YUV422) | BIT_ULL(CAM_FORMAT_MIPI_RAW_12) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_14) | BIT_ULL(CAM_FORMAT_MIPI_RAW_16) |
				BIT_ULL(CAM_FORMAT_MIPI_RAW_20) | BIT_ULL(CAM_FORMAT_PLAIN128) |
				BIT_ULL(CAM_FORMAT_PLAIN32_20) | BIT_ULL(CAM_FORMAT_PLAIN8) |
				BIT_ULL(CAM_FORMAT_PLAIN16_10) | BIT_ULL(CAM_FORMAT_PLAIN16_12) |
				BIT_ULL(CAM_FORMAT_PLAIN16_14) | BIT_ULL(CAM_FORMAT_PLAIN16_16) |
				BIT_ULL(CAM_FORMAT_PLAIN16_8) | BIT_ULL(CAM_FORMAT_PLAIN64) |
				BIT_ULL(CAM_FORMAT_YUV422_10),
		},
		/* BUS Client 5 Stats BE */
		{
			.cfg                      = 0x00001200,
			.image_addr               = 0x00001204,
			.frame_incr               = 0x00001208,
			.image_cfg_0              = 0x0000120C,
			.image_cfg_1              = 0x00001210,
			.image_cfg_2              = 0x00001214,
			.packer_cfg               = 0x00001218,
			.frame_header_addr        = 0x00001220,
			.frame_header_incr        = 0x00001224,
			.frame_header_cfg         = 0x00001228,
			.line_done_cfg            = 0x0000122C,
			.irq_subsample_period     = 0x00001230,
			.irq_subsample_pattern    = 0x00001234,
			.mmu_prefetch_cfg         = 0x00001260,
			.mmu_prefetch_max_offset  = 0x00001264,
			.system_cache_cfg         = 0x00001268,
			.addr_cfg                 = 0x00001270,
			.addr_status_0            = 0x00001290,
			.addr_status_1            = 0x00001294,
			.addr_status_2            = 0x00001298,
			.addr_status_3            = 0x0000129C,
			.debug_status_cfg         = 0x0000127C,
			.debug_status_0           = 0x00001280,
			.debug_status_1           = 0x00001284,
			.comp_group               = CAM_VFE_BUS_VER3_COMP_GRP_0,
			.ubwc_regs                = NULL,
			.supported_formats        = BIT_ULL(CAM_FORMAT_PLAIN64),
		},
	},
	.num_out = 6,
	.vfe_out_hw_info = {
		{
			.vfe_out_type  = CAM_VFE_BUS_VER3_VFE_OUT_RDI0,
			.max_width     = -1,
			.max_height    = -1,
			.source_group  = CAM_VFE_BUS_VER3_SRC_GRP_1,
			.num_wm        = 1,
			.line_based    = 1,
			.mid           = vfe_lite108x_out_port_mid[0],
			.num_mid       = 1,
			.wm_idx        = {
				0,
			},
			.name          = {
				"LITE_0",
			},
		},
		{
			.vfe_out_type  = CAM_VFE_BUS_VER3_VFE_OUT_RDI1,
			.max_width     = -1,
			.max_height    = -1,
			.source_group  = CAM_VFE_BUS_VER3_SRC_GRP_2,
			.num_wm        = 1,
			.line_based    = 1,
			.mid           = vfe_lite108x_out_port_mid[1],
			.num_mid       = 1,
			.wm_idx        = {
				1,
			},
			.name          = {
				"LITE_1",
			},
		},
		{
			.vfe_out_type  = CAM_VFE_BUS_VER3_VFE_OUT_RDI2,
			.max_width     = -1,
			.max_height    = -1,
			.source_group  = CAM_VFE_BUS_VER3_SRC_GRP_3,
			.num_wm        = 1,
			.line_based    = 1,
			.mid           = vfe_lite108x_out_port_mid[2],
			.num_mid       = 1,
			.wm_idx        = {
				2,
			},
			.name          = {
				"LITE_2",
			},
		},
		{
			.vfe_out_type  = CAM_VFE_BUS_VER3_VFE_OUT_RDI3,
			.max_width     = -1,
			.max_height    = -1,
			.source_group  = CAM_VFE_BUS_VER3_SRC_GRP_4,
			.num_wm        = 1,
			.line_based    = 1,
			.mid           = vfe_lite108x_out_port_mid[3],
			.num_mid       = 1,
			.wm_idx        = {
				3,
			},
			.name          = {
				"LITE_3",
			},
		},
		{
			.vfe_out_type  =
				CAM_VFE_BUS_VER3_VFE_OUT_PREPROCESS_RAW,
			.max_width     = 1920,
			.max_height    = 1080,
			.source_group  = CAM_VFE_BUS_VER3_SRC_GRP_0,
			.num_wm        = 1,
			.mid           = vfe_lite108x_out_port_mid[4],
			.num_mid       = 1,
			.wm_idx        = {
				4,
			},
			.name          = {
				"PREPROCESS_RAW",
			},
		},
		{
			.vfe_out_type  = CAM_VFE_BUS_VER3_VFE_OUT_STATS_BG,
			.max_width     = -1,
			.max_height    = -1,
			.source_group  = CAM_VFE_BUS_VER3_SRC_GRP_0,
			.num_wm        = 1,
			.mid           = vfe_lite108x_out_port_mid[5],
			.num_mid       = 1,
			.wm_idx        = {
				5,
			},
			.name          = {
				"STATS_BG",
			},
		},
	},
	.num_comp_grp    = 5,
	.support_consumed_addr = true,
	.comp_done_mask = {
		BIT(0), BIT(1), BIT(2), BIT(3), BIT(4),
	},
	.top_irq_shift   = 0,
	.max_out_res = CAM_ISP_IFE_OUT_RES_BASE + 34,
};

static struct cam_vfe_irq_hw_info vfe_lite108x_irq_hw_info = {
	.reset_mask    = 0,
	.supported_irq = CAM_VFE_HW_IRQ_CAP_LITE_EXT_CSID,
	.top_irq_reg   = &vfe_lite108x_top_irq_reg_info,
};

static struct cam_vfe_hw_info cam_vfe_lite108x_hw_info = {
	.irq_hw_info                   = &vfe_lite108x_irq_hw_info,

	.bus_version                   = CAM_VFE_BUS_VER_3_0,
	.bus_hw_info                   = &vfe_lite108x_bus_hw_info,

	.top_version                   = CAM_VFE_TOP_VER_4_0,
	.top_hw_info                   = &vfe_lite108x_top_hw_info,
};

#endif /* _CAM_VFE_LITE108X_H_ */
