/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _CAM_IFE_CSID_LITE_1080_H_
#define _CAM_IFE_CSID_LITE_1080_H_

static const struct cam_ife_csid_irq_desc cam_ife_csid_lite_1080_rx_irq_desc[][32] = {
	{
		{
			.bitmask = BIT(0),
			.desc = "DL0_EOT",
		},
		{
			.bitmask = BIT(1),
			.desc = "DL1_EOT",
		},
		{
			.bitmask = BIT(2),
			.desc = "DL2_EOT",
		},
		{
			.bitmask = BIT(3),
			.desc = "DL3_EOT",
		},
		{
			.bitmask = BIT(4),
			.desc = "DL0_SOT",
		},
		{
			.bitmask = BIT(5),
			.desc = "DL1_SOT",
		},
		{
			.bitmask = BIT(6),
			.desc = "DL2_SOT",
		},
		{
			.bitmask = BIT(7),
			.desc = "DL3_SOT",
		},
		{
			.bitmask = BIT(8),
			.desc = "DPHY_PH_ECC_SEC",
		},
		{
			.bitmask = BIT(9),
			.desc = "SENSOR_MODE_ID_CHANGE",
		},
		{0},
		{0},
		{
			.bitmask = BIT(12),
			.desc =
				"DL0_EOT_LOST, Sensor: Issue is with the timing signals received in the cphy packet on lane 0 - Check phy/sensor config",
		},
		{
			.bitmask = BIT(13),
			.desc =
				"DL1_EOT_LOST, Sensor: Issue is with the timing signals received in the cphy packet on lane 1 - Check phy/sensor config",
		},
		{
			.bitmask = BIT(14),
			.desc =
				"DL2_EOT_LOST, Sensor: Issue is with the timing signals received in the cphy packet on lane 2 - Check phy/sensor config",
		},
		{
			.bitmask = BIT(15),
			.desc =
				"DL3_EOT_LOST, Sensor: Issue is with the timing signals received in the cphy packet on lane 3 - Check phy/sensor config",
		},
		{
			.bitmask = BIT(16),
			.desc =
				"DL0_SOT_LOST, Sensor: Timing signals are missed received in the cphy packet on lane 0 - Check phy/sensor config",
		},
		{
			.bitmask = BIT(17),
			.desc =
				"DL1_SOT_LOST, Sensor: Timing signals are missed received in the cphy packet on lane 1 - Check phy/sensor config",
		},
		{
			.bitmask = BIT(18),
			.desc =
				"DL2_SOT_LOST, Sensor: Timing signals are missed received in the cphy packet on lane 2 - Check phy/sensor config",
		},
		{
			.bitmask = BIT(19),
			.desc =
				"DL3_SOT_LOST, Sensor: Timing signals are missed received in the cphy packet on lane 3 - Check phy/sensor config",
		},
		{
			.bitmask = BIT(20),
			.desc =
				"DL0_FIFO_OVERFLOW, System: Data has been lost when transferring from PHY to CSID on Lane 0 - Check PHY clock, CSID clock and possible skew among the data lanes",
		},
		{
			.bitmask = BIT(21),
			.desc =
				"DL1_FIFO_OVERFLOW, System: Data has been lost when transferring from PHY to CSID on Lane 1 - Check PHY clock, CSID clock and possible skew among the data lanes",
		},
		{
			.bitmask = BIT(22),
			.desc =
				"DL2_FIFO_OVERFLOW, System: Data has been lost when transferring from PHY to CSID on Lane 2 - Check PHY clock, CSID clock and possible skew among the data lanes",
		},
		{
			.bitmask = BIT(23),
			.desc =
				"DL3_FIFO_OVERFLOW, System: Data has been lost when transferring from PHY to CSID on Lane 3 - Check PHY clock, CSID clock and possible skew among the data lanes",
		},
		{
			.bitmask = BIT(24),
			.desc =
				"CPHY_PH_CRC, Sensor: All CPHY packet headers received are corrupted - Check phy/sensor config",
		},
		{
			.bitmask = BIT(25),
			.desc =
				"PAYLOAD_CRC, Sensor: The calculated CRC of a long packet does not match the transmitted (expected) CRC, possible corruption - Check phy/sensor config",
		},
		{
			.bitmask = BIT(26),
			.desc =
				"DPHY_PH_ECC_DED, Sensor: A short or long packet is corrupted and cannot be recovered - Check phy/sensor config",
		},
		{
			.bitmask = BIT(27),
			.desc =
				"MMAPPED_VC_DT, SW: A long packet has a VC_DT combination that is configured for more than one IPP or RDIs",
		},
		{
			.bitmask = BIT(28),
			.desc =
				"UNMAPPED_VC_DT, Sensor: A long packet has a VC_DT combination that is not configured for IPP or RDIs",
		},
		{
			.bitmask = BIT(29),
			.desc =
				"STREAM_UNDERFLOW, Sensor: Long packet payload size is less than payload header size, resulting a corrupted frame - Perform PHY Tuning/Check sensor limitations",
		},
		{0},
		{
			.bitmask = BIT(31),
			.desc = "CSI2_RX_IRQ_STATUS_2",
		},
	},
	{
		{
			.bitmask = BIT(0),
			.desc =
				"LONG_PKT, Debug: The header of the first long pkt matching the configured vc-dt has been captured",
		},
		{
			.bitmask = BIT(1),
			.desc =
				"SHORT_PKT, Debug: The header of the first short pkt matching the configured vc-dt has been captured",
		},
		{
			.bitmask = BIT(2),
			.desc =
				"CPHY_PKT_HDR, Debug: The header of the first cphy pkt matching the configured vc-dt has been captured",
		},
		{
			.bitmask = BIT(3),
			.desc = "Illegal programming for next frame ID config",
		},
	},
};

static const uint32_t cam_ife_csid_lite_1080_num_rx_irq_desc[] = {
	ARRAY_SIZE(cam_ife_csid_lite_1080_rx_irq_desc[0]),
	ARRAY_SIZE(cam_ife_csid_lite_1080_rx_irq_desc[1]),
};

static const struct cam_ife_csid_irq_desc cam_ife_csid_lite_1080_path_irq_desc[] = {
	{
		.bitmask = BIT(0),
		.err_type = CAM_ISP_HW_ERROR_CSID_FATAL,
		.irq_name = "ILLEGAL_PROGRAMMING",
		.desc = "SW: Illegal programming sequence",
		.debug = "Check the following possiblities:",
		.err_handler = cam_ife_csid_ver2_print_illegal_programming_irq_status,
	},
	{0},
	{
		.bitmask = BIT(2),
		.irq_name = "INFO_DATA_FIFO_FULL",
	},
	{
		.bitmask = BIT(3),
		.irq_name = "CAMIF_EOF",
	},
	{
		.bitmask = BIT(4),
		.irq_name = "CAMIF_SOF",
	},
	{0},
	{0},
	{
		.bitmask = BIT(7),
		.err_type = CAM_ISP_HW_ERROR_CSID_ILLEGAL_DT_SWITCH,
		.irq_name = "ILLEGAL_DT_SWITCH",
		.desc = "SW: Change in DT without MUP",
		.debug = "Check sensor configuration",
		.err_handler = cam_ife_csid_hw_ver2_mup_mismatch_handler,
	},
	{0},
	{
		.bitmask = BIT(9),
		.irq_name = "INFO_INPUT_EOF",
	},
	{
		.bitmask = BIT(10),
		.irq_name = "INFO_INPUT_EOL",
	},
	{
		.bitmask = BIT(11),
		.irq_name = "INFO_INPUT_SOL",
	},
	{
		.bitmask = BIT(12),
		.irq_name = "INFO_INPUT_SOF",
	},
	{
		.bitmask = BIT(13),
		.err_type = CAM_ISP_HW_ERROR_CSID_FRAME_SIZE,
		.irq_name = "ERROR_PIX_COUNT",
		.desc = "SW: Mismatch in expected versus received number of pixels per line",
		.debug = "Check SW config/sensor stream",
		.err_handler = cam_ife_csid_ver2_print_format_measure_info,
	},
	{
		.bitmask = BIT(14),
		.err_type = CAM_ISP_HW_ERROR_CSID_FRAME_SIZE,
		.irq_name = "ERROR_LINE_COUNT",
		.desc = "SW: Mismatch in expected versus received number of lines",
		.debug = "Check SW config/sensor stream",
		.err_handler = cam_ife_csid_ver2_print_format_measure_info,
	},
	{
		.bitmask = BIT(15),
		.irq_name = "VCDT_GRP1_SEL",
	},
	{
		.bitmask = BIT(16),
		.irq_name = "VCDT_GRP0_SEL",
	},
	{
		.bitmask = BIT(17),
		.irq_name = "VCDT_GRP_CHANGE",
	},
	{
		.bitmask = BIT(18),
		.err_type = CAM_ISP_HW_ERROR_CSID_CAMIF_FRAME_DROP,
		.irq_name = "CAMIF_FRAME_DROP",
		.desc =
			"Sensor: The pre CAMIF frame drop would drop a frame in case the new frame starts prior to the end of the previous frame",
		.debug = "Slower downstream processing or faster frame generation from sensor",
	},
	{
		.bitmask = BIT(19),
		.err_type = CAM_ISP_HW_ERROR_RECOVERY_OVERFLOW,
		.irq_name = "OVERFLOW_RECOVERY",
		.desc = "Detected by the overflow recovery block",
		.debug = "Backpressure downstream",
	},
	{
		.bitmask = BIT(20),
		.irq_name = "ERROR_REC_CCIF_VIOLATION",
		.desc = "Output CCIF has a violation with respect to frame timing",
	},
	{
		.bitmask = BIT(21),
		.irq_name = "CAMIF_EPOCH0",
	},
	{
		.bitmask = BIT(22),
		.irq_name = "CAMIF_EPOCH1",
	},
	{
		.bitmask = BIT(23),
		.irq_name = "RUP_DONE",
	},
	{
		.bitmask = BIT(24),
		.irq_name = "ILLEGAL_BATCH_ID",
		.desc = "SW: Decoded frame ID does not match with any of the programmed batch IDs",
		.debug = "Check frame ID and all available batch IDs",
	},
	{
		.bitmask = BIT(25),
		.irq_name = "BATCH_END_MISSING_VIOLATION",
		.desc = "SW: Input number of frames is not a multiple of the batch size",
		.debug = "Check the configured pattern/period for batching",
	},
	{
		.bitmask = BIT(26),
		.irq_name = "UNBOUNDED_FRAME",
		.desc = "Sensor: Frame end or frame start is missing",
		.debug = "Check the settle count in sensor driver XML",
	},
	{
		.bitmask = BIT(27),
		.irq_name = "ERROR_SER_INVALID_CTXT",
		.desc = "Indicates invalid context on CCIF",

	},
	{
		.bitmask = BIT(28),
		.irq_name = "SENSOR_SWITCH_OUT_OF_SYNC_FRAME_DROP",
		.desc =
			"Sensor/SW: The programmed MUP is out of sync with the VC of the incoming frame",
		.err_handler = cam_ife_csid_hw_ver2_mup_mismatch_handler,
	},
	{
		.bitmask = BIT(29),
		.irq_name = "CCIF_VIOLATION",
		.desc =
			"The output CCIF from the serializer has a violation with respect to frame timing",
	},
};

static const struct cam_ife_csid_top_irq_desc cam_ife_csid_lite_1080_top_irq_desc[][32] = {
	{
		{
			.bitmask  = BIT(1),
			.err_type = CAM_ISP_HW_ERROR_CSID_SENSOR_SWITCH_ERROR,
			.err_name = "FATAL_SENSOR_SWITCHING_IRQ",
			.desc =
				"Sensor/SW: Minimum VBI period between dynamically switching between two sensor modes was either violated or the downstream pipe was not active when the switch was made",
		},
	},
	{
		{
			.bitmask = BIT(5),
			.err_name = "ERROR_SENSOR_HBI",
			.desc = "Sensor: Sensor HBI is less than expected HBI",
			.debug = "Check sensor configuration",
		},
	},
};

static const uint32_t cam_ife_csid_lite_1080_num_top_irq_desc[] = {
	ARRAY_SIZE(cam_ife_csid_lite_1080_top_irq_desc[0]),
	ARRAY_SIZE(cam_ife_csid_lite_1080_top_irq_desc[1]),
};

static const char *cam_ife_csid_lite_1080_debug_vec_desc[][32] = {
	{
		"ERROR_UNBOUNDED_FRAME_RDI1",
		"ERROR_SER_INVALID_CTXT_RDI1",
		"ERROR_SER_CCIF_VIOLATION_RDI1",
		"FATAL_SENSOR_SWITCH_OUT_OF_SYNC_FRAME_DROP_RDI1",
		"ERROR_REC_FRAME_DROP_RDI1",
		"ERROR_REC_OVERFLOW_RDI0",
		"ERROR_CAMIF_CCIF_VIOLATION_RDI0",
		"ERROR_ILLEGAL_BATCH_ID_RDI0",
		"ERROR_UNBOUNDED_FRAME_RDI0",
		"ERROR_SER_INVALID_CTXT_RDI0",
		"ERROR_SER_CCIF_VIOLATION_RDI0",
		"FATAL_SENSOR_SWITCH_OUT_OF_SYNC_FRAME_DROP_RDI0",
		"ERROR_REC_FRAME_DROP_RDI0",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"ERROR_DL0_FIFO_OVERFLOW",
		"ERROR_DL1_FIFO_OVERFLOW",
		"ERROR_DL2_FIFO_OVERFLOW",
		"ERROR_DL3_FIFO_OVERFLOW",
		"ERROR_CPHY_PH_CRC",
		"ERROR_DPHY_PH_ECC_DED",
		"ERROR_STREAM_UNDERFLOW",
		"ERROR_NO_VOTE_DN",
		"ERROR_VOTE_UP_LATE",
		"ERROR_RDI_LINE_BUFFER_CONFLICT",
		"ERROR_SENSOR_HBI"
	},
	{
		"ERROR_UNBOUNDED_FRAME_IPP0",
		"ERROR_SER_INVALID_CTXT_IPP0",
		"ERROR_SER_CCIF_VIOLATION_IPP0",
		"FATAL_SENSOR_SWITCH_OUT_OF_SYNC_FRAME_DROP_IPP0",
		"ERROR_REC_FRAME_DROP_IPP0",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"ERROR_REC_OVERFLOW_RDI3",
		"ERROR_CAMIF_CCIF_VIOLATION_RDI3",
		"ERROR_ILLEGAL_BATCH_ID_RDI3",
		"ERROR_UNBOUNDED_FRAME_RDI3",
		"ERROR_SER_INVALID_CTXT_RDI3",
		"ERROR_SER_CCIF_VIOLATION_RDI3",
		"FATAL_SENSOR_SWITCH_OUT_OF_SYNC_FRAME_DROP_RDI3",
		"ERROR_REC_FRAME_DROP_RDI3",
		"ERROR_REC_OVERFLOW_RDI2",
		"ERROR_CAMIF_CCIF_VIOLATION_RDI2",
		"ERROR_ILLEGAL_BATCH_ID_RDI2",
		"ERROR_UNBOUNDED_FRAME_RDI2",
		"ERROR_SER_INVALID_CTXT_RDI2",
		"ERROR_SER_CCIF_VIOLATION_RDI2",
		"FATAL_SENSOR_SWITCH_OUT_OF_SYNC_FRAME_DROP_RDI2",
		"ERROR_REC_FRAME_DROP_RDI2",
		"ERROR_REC_OVERFLOW_RDI1",
		"ERROR_CAMIF_CCIF_VIOLATION_RDI1",
		"ERROR_ILLEGAL_BATCH_ID_RDI1"
	},
	{
		"ERROR_REC_OVERFLOW_IPP0",
		"ERROR_CAMIF_CCIF_VIOLATION_IPP0",
		"ERROR_ILLEGAL_BATCH_ID_IPP0",
	}
};

static struct cam_irq_register_set cam_ife_csid_lite_1080_irq_reg_set[CAM_IFE_CSID_IRQ_REG_MAX] = {
	/* Top */
	{
		.mask_reg_offset   = 0x00000184,
		.clear_reg_offset  = 0x00000188,
		.status_reg_offset = 0x00000180,
		.set_reg_offset    = 0x0000018C,
		.test_set_val      = BIT(0),
		.test_sub_val      = BIT(0),
	},
	/* RX */
	{
		.mask_reg_offset   = 0x000001B4,
		.clear_reg_offset  = 0x000001B8,
		.status_reg_offset = 0x000001B0,
	},
	/* RDI0 */
	{
		.mask_reg_offset   = 0x00000228,
		.clear_reg_offset  = 0x0000022C,
		.status_reg_offset = 0x00000224,
	},
	/* RDI1 */
	{
		.mask_reg_offset   = 0x00000238,
		.clear_reg_offset  = 0x0000023C,
		.status_reg_offset = 0x00000234,
	},
	/* RDI2 */
	{
		.mask_reg_offset   = 0x00000248,
		.clear_reg_offset  = 0x0000024C,
		.status_reg_offset = 0x00000244,
	},
	/* RDI3 */
	{
		.mask_reg_offset   = 0x00000258,
		.clear_reg_offset  = 0x0000025C,
		.status_reg_offset = 0x00000254,
	},
	{}, /* no RDI4 */
	/* IPP */
	{
		.mask_reg_offset   = 0x000001E8,
		.clear_reg_offset  = 0x000001EC,
		.status_reg_offset = 0x000001E4,
	},
	{0}, /* no PPP */
	/* UDI_0 */
	{0},
	/* UDI_1 */
	{0},
	/* UDI_2 */
	{0},
	/* Top_2 */
	{
		.mask_reg_offset   = 0x00000194,
		.clear_reg_offset  = 0x00000198,
		.status_reg_offset = 0x00000190,
		.set_reg_offset    = 0x0000019C,
	},
	/* RX_2 */
	{
		.mask_reg_offset   = 0x000001C4,
		.clear_reg_offset  = 0x000001C8,
		.status_reg_offset = 0x000001C0,
	},
};

static struct cam_irq_controller_reg_info cam_ife_csid_lite_1080_top_irq_reg_info[] = {
	{
	.num_registers = 1,
	.irq_reg_set = &cam_ife_csid_lite_1080_irq_reg_set[CAM_IFE_CSID_IRQ_REG_TOP],
	.global_irq_cmd_offset = 0x00000110,
	.global_set_bitmask    = 0x00000010,
	.global_clear_bitmask  = 0x00000001,
	.clear_all_bitmask     = 0xFFFFFFFF,
	},
	{
	.num_registers = 1,
	.irq_reg_set = &cam_ife_csid_lite_1080_irq_reg_set[CAM_IFE_CSID_IRQ_REG_TOP_2],
	.global_irq_cmd_offset = 0, /* intentionally set to zero */
	},

};

static struct cam_irq_controller_reg_info cam_ife_csid_lite_1080_rx_irq_reg_info[] = {
	{
	.num_registers = 1,
	.irq_reg_set = &cam_ife_csid_lite_1080_irq_reg_set[CAM_IFE_CSID_IRQ_REG_RX], /* RX */
	.global_irq_cmd_offset = 0,
	},
	{
	.num_registers = 1,
	.irq_reg_set = &cam_ife_csid_lite_1080_irq_reg_set[CAM_IFE_CSID_IRQ_REG_RX_2],
	.global_irq_cmd_offset = 0, /* intentionally set to zero */

	},
};

static struct cam_irq_controller_reg_info cam_ife_csid_lite_1080_path_irq_reg_info[6] = {
	{
		.num_registers = 1,
		.irq_reg_set = &cam_ife_csid_lite_1080_irq_reg_set[CAM_IFE_CSID_IRQ_REG_RDI_0],
		.global_irq_cmd_offset = 0,
	},
	{
		.num_registers = 1,
		.irq_reg_set = &cam_ife_csid_lite_1080_irq_reg_set[CAM_IFE_CSID_IRQ_REG_RDI_1],
		.global_irq_cmd_offset = 0,
	},
	{
		.num_registers = 1,
		.irq_reg_set = &cam_ife_csid_lite_1080_irq_reg_set[CAM_IFE_CSID_IRQ_REG_RDI_2],
		.global_irq_cmd_offset = 0,
	},
	{
		.num_registers = 1,
		.irq_reg_set = &cam_ife_csid_lite_1080_irq_reg_set[CAM_IFE_CSID_IRQ_REG_RDI_3],
		.global_irq_cmd_offset = 0,
	},
	{}, /* no RDI4 */
	{
		.num_registers = 1,
		.irq_reg_set = &cam_ife_csid_lite_1080_irq_reg_set[CAM_IFE_CSID_IRQ_REG_IPP],
		.global_irq_cmd_offset = 0,
	},
};

static struct cam_irq_register_set cam_ife_csid_lite_1080_buf_done_irq_reg_set[1] = {
	{
		.mask_reg_offset   = 0x000001A4,
		.clear_reg_offset  = 0x000001A8,
		.status_reg_offset = 0x000001A0,
	},
};

static struct cam_irq_controller_reg_info
	cam_ife_csid_lite_1080_buf_done_irq_reg_info = {
	.num_registers = 1,
	.irq_reg_set = cam_ife_csid_lite_1080_buf_done_irq_reg_set,
	.global_irq_cmd_offset  = 0, /* intentionally set to zero */
};

static struct cam_ife_csid_ver2_common_reg_info
			cam_ife_csid_lite_1080_cmn_reg_info = {
	.hw_version_addr                              = 0x0000,
	.cfg0_addr                                    = 0x0100,
	.global_cmd_addr                              = 0x0104,
	.reset_cfg_addr                               = 0x0108,
	.reset_cmd_addr                               = 0x010C,
	.irq_cmd_addr                                 = 0x0110,
	.rup_cmd_addr                                 = 0x0114,
	.aup_cmd_addr                                 = 0x0118,
	.rup_aup_cmd_addr                             = 0x011C,
	.top_irq_status_addr                          = {0x0180, 0x0190},
	.top_irq_mask_addr                            = {0x0184, 0x0194},
	.top_irq_clear_addr                           = {0x0188, 0x0198},
	.top_irq_set_addr                             = {0x018C, 0x019C},
	.buf_done_irq_status_addr                     = 0x01A0,
	.buf_done_irq_mask_addr                       = 0x01A4,
	.buf_done_irq_clear_addr                      = 0x01A8,
	.buf_done_irq_set_addr                        = 0x01AC,
	.debug_sensor_hbi_irq_vcdt_addr               = 0x29C,
	.debug_violation_addr                         = 0x7E0,
	.debug_cfg_addr                               = 0x7E4,
	.debug_err_vec_irq                            = {0x2D4, 0x2D8, 0x2DC},
	.debug_err_vec_cfg                            = 0x2D0,
	.debug_err_vec_ts_lb                          = 0x2E0,
	.debug_err_vec_ts_mb                          = 0x2E4,

	/*configurations */
	.major_version                                = 10,
	.minor_version                                = 8,
	.version_incr                                 = 0,
	.num_rdis                                     = 4,
	.num_pix                                      = 1,
	.num_ppp                                      = 0,
	.rst_done_shift_val                           = 1,
	.path_en_shift_val                            = 31,
	.vc_shift_val                                 = 22,
	.vc_mask                                      = 0x1F,
	.dt_shift_val                                 = 16,
	.dt_mask                                      = 0x3F,
	.crop_shift_val                               = 16,
	.decode_format_shift_val                      = 12,
	.decode_format1_shift_val                     = 16,
	.decode_format2_shift_val                     = 0,
	.decode_format3_shift_val                     = 4,
	.decode_format_mask                           = 0xF,
	.decode_format1_supported                     = true,
	.frame_id_decode_en_shift_val                 = 1,
	.multi_vcdt_vc1_shift_val                     = 2,
	.multi_vcdt_dt1_shift_val                     = 7,
	.multi_vcdt_dt2_shift_val                     = 8,
	.multi_vcdt_dt3_shift_val                     = 16,
	.multi_vcdt_ts_combo_en_shift_val             = 13,
	.multi_vcdt_en_shift_val                      = 0,
	.timestamp_stb_sel_shift_val                  = 8,
	.vfr_en_shift_val                             = 0,
	.mup_shift_val                                = 28,
	.early_eof_supported                          = 1,
	.vfr_supported                                = 1,
	.multi_vcdt_supported                         = 1,
	.num_dt_supported                             = 4,
	.ts_comb_vcdt_en                              = true,
	.direct_cid_config                            = true,
	.ts_comb_vcdt_mask                            = 3,
	.frame_id_dec_supported                       = 1,
	.measure_en_hbi_vbi_cnt_mask                  = 0xc,
	.measure_pixel_line_en_mask                   = 0x3,
	.crop_pix_start_mask                          = 0x3fff,
	.crop_pix_end_mask                            = 0xffff,
	.crop_line_start_mask                         = 0x3fff,
	.crop_line_end_mask                           = 0xffff,
	.drop_supported                               = 1,
	.ipp_irq_mask_all                             = 0x7FFF,
	.rdi_irq_mask_all                             = 0x7FFF,
	.top_err_irq_mask                             = {0x00000002, 0x20},
	.rst_loc_path_only_val                        = 0x0,
	.rst_location_shift_val                       = 4,
	.rst_loc_complete_csid_val                    = 0x1,
	.rst_mode_frame_boundary_val                  = 0x0,
	.rst_mode_immediate_val                       = 0x1,
	.rst_cmd_hw_reset_complete_val                = 0x1,
	.rst_cmd_sw_reset_complete_val                = 0x2,
	.rst_cmd_irq_ctrl_only_val                    = 0x4,
	.timestamp_strobe_val                         = 0x2,
	.global_reset                                 = 1,
	.aup_rup_supported                            = 1,
	.only_master_rup                              = 1,
	.format_measure_height_mask_val               = 0xFFFF,
	.format_measure_height_shift_val              = 0x10,
	.format_measure_width_mask_val                = 0xFFFF,
	.format_measure_width_shift_val               = 0x0,
	.top_reset_irq_mask                           = {0x1,},
	.top_buf_done_irq_mask                        = 0x2000,
	.decode_format_payload_only                   = 0xF,
	.phy_sel_base_idx                             = 1,
	.timestamp_enabled_in_cfg0                    = true,
	.camif_irq_support                            = true,
	.capabilities                                 = CAM_IFE_CSID_CAP_SKIP_PATH_CFG1 |
								CAM_IFE_CSID_CAP_SPLIT_RUP_AUP |
								CAM_IFE_CSID_CAP_SKIP_EPOCH_CFG |
								CAM_IFE_CSID_CAP_DEBUG_ERR_VEC,

	.top_top2_irq_mask                            = 0x80000000,
};

static struct cam_ife_csid_ver2_csi2_rx_reg_info
	cam_ife_csid_lite_1080_csi2_reg_info = {
		.irq_status_addr                      = {0x01B0, 0x01C0},
		.irq_mask_addr                        = {0x01B4, 0x01C4},
		.irq_clear_addr                       = {0x01B8, 0x01C8},
		.irq_set_addr                         = {0x01BC, 0x01CC},
		/*CSI2 rx control */
		.cfg0_addr                            = 0x0880,
		.cfg1_addr                            = 0x0884,
		.capture_ctrl_addr                    = 0x0888,
		.rst_strobes_addr                     = 0x088C,
		.cap_unmap_long_pkt_hdr_0_addr        = 0x0890,
		.cap_unmap_long_pkt_hdr_1_addr        = 0x0894,
		.captured_short_pkt_0_addr            = 0x0898,
		.captured_short_pkt_1_addr            = 0x089C,
		.captured_long_pkt_0_addr             = 0x08A0,
		.captured_long_pkt_1_addr             = 0x08A4,
		.captured_long_pkt_ftr_addr           = 0x08A8,
		.captured_cphy_pkt_hdr_addr           = 0x08AC,
		.lane0_misr_addr                      = 0x08B0,
		.lane1_misr_addr                      = 0x08B4,
		.lane2_misr_addr                      = 0x08B8,
		.lane3_misr_addr                      = 0x08BC,
		.total_pkts_rcvd_addr                 = 0x08C0,
		.stats_ecc_addr                       = 0x08C4,
		.total_crc_err_addr                   = 0x08C8,
		.de_scramble_type3_cfg0_addr          = 0x08CC,
		.de_scramble_type3_cfg1_addr          = 0x08D0,
		.de_scramble_type2_cfg0_addr          = 0x08D4,
		.de_scramble_type2_cfg1_addr          = 0x08D8,
		.de_scramble_type1_cfg0_addr          = 0x08DC,
		.de_scramble_type1_cfg1_addr          = 0x08E0,
		.de_scramble_type0_cfg0_addr          = 0x08E4,
		.de_scramble_type0_cfg1_addr          = 0x08E8,

		.rst_done_shift_val                   = 27,
		.irq_mask_all                         = 0xFFFFFFF,
		.misr_enable_shift_val                = 6,
		.vc_mode_shift_val                    = 2,
		.capture_long_pkt_en_shift            = 0,
		.capture_short_pkt_en_shift           = 1,
		.capture_cphy_pkt_en_shift            = 2,
		.capture_long_pkt_dt_shift            = 4,
		.capture_long_pkt_vc_shift            = 10,
		.capture_short_pkt_vc_shift           = 15,
		.capture_cphy_pkt_dt_shift            = 20,
		.capture_cphy_pkt_vc_shift            = 26,
		.phy_num_mask                         = 0xf,
		.vc_mask                              = 0x7C00000,
		.dt_mask                              = 0x3f0000,
		.wc_mask                              = 0xffff,
		.vc_shift                             = 0x16,
		.dt_shift                             = 0x10,
		.wc_shift                             = 0,
		.calc_crc_mask                        = 0xffff,
		.expected_crc_mask                    = 0xffff,
		.calc_crc_shift                       = 0x10,
		.ecc_correction_shift_en              = 0,
		.lane_num_shift                       = 0,
		.lane_cfg_shift                       = 4,
		.phy_type_shift                       = 24,
		.phy_num_shift                        = 20,
		.tpg_mux_en_shift                     = 27,
		.tpg_num_sel_shift                    = 28,
		.phy_bist_shift_en                    = 7,
		.epd_mode_shift_en                    = 8,
		.eotp_shift_en                        = 9,
		.dyn_sensor_switch_shift_en           = 10,
		.rup_aup_latch_shift                  = 13,
		.rup_aup_latch_supported              = true,
		.long_pkt_strobe_rst_shift            = 0,
		.short_pkt_strobe_rst_shift           = 1,
		.cphy_pkt_strobe_rst_shift            = 2,
		.unmapped_pkt_strobe_rst_shift        = 3,
		.fatal_err_mask                       = {0x25fff000, 0x0},
		.part_fatal_err_mask                  = {0x02000000, 0x0},
		.non_fatal_err_mask                   = {0x08000000, 0x0},
		.top_irq_mask                         = {0x4, 0x0},
		.rx_rx2_irq_mask                      = 0x80000000,
};

static struct cam_ife_csid_ver2_path_reg_info
	cam_ife_csid_lite_1080_ipp_reg_info = {
		.irq_status_addr                      = 0x01E4,
		.irq_mask_addr                        = 0x01E8,
		.irq_clear_addr                       = 0x01EC,
		.irq_set_addr                         = 0x01F0,
		.cfg0_addr                            = 0x0C80,
		.ctrl_addr                            = 0x0C88,
		.debug_clr_cmd_addr                   = 0x0C8C,
		.multi_vcdt_cfg0_addr                 = 0x0C90,
		.multi_vcdt_cfg1_addr                 = 0x0C84,
		.cfg1_addr                            = 0,
		.err_recovery_cfg0_addr               = 0,
		.err_recovery_cfg1_addr               = 0,
		.err_recovery_cfg2_addr               = 0,
		.camif_frame_cfg_addr                 = 0x0C94,
		.epoch_irq_cfg_addr                   = 0,
		.epoch0_subsample_ptrn_addr           = 0,
		.epoch1_subsample_ptrn_addr           = 0,
		.debug_rup_aup_status                 = 0x0C98,
		.debug_camif_1_addr                   = 0x0C9C,
		.debug_camif_0_addr                   = 0x0CA0,
		.debug_halt_status_addr               = 0x0CA4,
		.debug_misr_val0_addr                 = 0x0CA8,
		.debug_misr_val1_addr                 = 0x0CAC,
		.debug_misr_val2_addr                 = 0x0CB0,
		.debug_misr_val3_addr                 = 0x0CB4,
		.hcrop_addr                           = 0,
		.vcrop_addr                           = 0,
		.pix_drop_pattern_addr                = 0,
		.pix_drop_period_addr                 = 0,
		.line_drop_pattern_addr               = 0,
		.line_drop_period_addr                = 0,
		.frm_drop_pattern_addr                = 0x0CB8,
		.frm_drop_period_addr                 = 0x0CBC,
		.irq_subsample_pattern_addr           = 0x0CC0,
		.irq_subsample_period_addr            = 0x0CC4,
		.format_measure_cfg0_addr             = 0x0CC8,
		.format_measure_cfg1_addr             = 0x0CCC,
		.format_measure_cfg2_addr             = 0x0CD0,
		.format_measure0_addr                 = 0x0CD4,
		.format_measure1_addr                 = 0x0CD8,
		.format_measure2_addr                 = 0x0CDC,
		.timestamp_curr0_sof_addr             = 0x0CE0,
		.timestamp_curr1_sof_addr             = 0x0CE4,
		.timestamp_perv0_sof_addr             = 0x0CE8,
		.timestamp_perv1_sof_addr             = 0x0CEC,
		.timestamp_curr0_eof_addr             = 0x0CF0,
		.timestamp_curr1_eof_addr             = 0x0CF4,
		.timestamp_perv0_eof_addr             = 0x0CF8,
		.timestamp_perv1_eof_addr             = 0x0CFC,
		.batch_period_cfg_addr                = 0x0D00,
		.batch_stream_id_cfg_addr             = 0x0D04,
		.epoch0_cfg_batch_id0_addr            = 0x0D10,
		.epoch1_cfg_batch_id0_addr            = 0x0D14,
		.epoch0_cfg_batch_id1_addr            = 0x0D18,
		.epoch1_cfg_batch_id1_addr            = 0x0D1C,
		.epoch0_cfg_batch_id2_addr            = 0x0D20,
		.epoch1_cfg_batch_id2_addr            = 0x0D24,
		.epoch0_cfg_batch_id3_addr            = 0x0D28,
		.epoch1_cfg_batch_id3_addr            = 0x0D2C,
		.epoch0_cfg_batch_id4_addr            = 0x0D30,
		.epoch1_cfg_batch_id4_addr            = 0x0D34,
		.epoch0_cfg_batch_id5_addr            = 0x0D38,
		.epoch1_cfg_batch_id5_addr            = 0x0D3C,

		/* configurations */
		.start_mode_internal                  = 0x0,
		.start_mode_global                    = 0x1,
		.start_mode_master                    = 0x2,
		.start_mode_slave                     = 0x3,
		.start_mode_shift                     = 2,
		.start_master_sel_val                 = 0,
		.start_master_sel_shift               = 4,
		.resume_frame_boundary                = 1,
		.crop_v_en_shift_val                  = 13,
		.crop_h_en_shift_val                  = 12,
		.drop_v_en_shift_val                  = 11,
		.drop_h_en_shift_val                  = 10,
		.pix_store_en_shift_val               = 14,
		.early_eof_en_shift_val               = 16,
		.format_measure_en_shift_val          = 8,
		.timestamp_en_shift_val               = 6,
		.overflow_ctrl_en                     = 1,
		.overflow_ctrl_mode_val               = 0x8,
		.min_hbi_shift_val                    = 4,
		.start_master_sel_shift_val           = 4,
		.fatal_err_mask                       = 0x2c1c6081,
		.non_fatal_err_mask                   = 0x12000000,
		.sof_irq_mask                         = 0x10,
		.rup_irq_mask                         = 0x800000,
		.epoch0_irq_mask                      = 0x200000,
		.epoch1_irq_mask                      = 0x400000,
		.eof_irq_mask                         = 0x8,
		.rup_mask                             = 0x1,
		.aup_mask                             = 0x1,
		.rup_aup_set_mask                     = 0x1,
		.top_irq_mask                         = {0x100,},
};

static struct cam_ife_csid_ver2_path_reg_info
	cam_ife_csid_lite_1080_rdi_0_reg_info = {
		.irq_status_addr                     = 0x0224,
		.irq_mask_addr                       = 0x0228,
		.irq_clear_addr                      = 0x022C,
		.irq_set_addr                        = 0x0230,
		.cfg0_addr                           = 0x3080,
		.ctrl_addr                           = 0x3088,
		.debug_clr_cmd_addr                  = 0x308C,
		.multi_vcdt_cfg0_addr                = 0x3090,
		.multi_vcdt_cfg1_addr                = 0x3084,
		.cfg1_addr                           = 0x3094,
		.debug_byte_cntr_ping_addr           = 0x30A8,
		.debug_byte_cntr_pong_addr           = 0x30AC,
		.camif_frame_cfg_addr                = 0x30B0,
		.epoch_irq_cfg_addr                  = 0x30B4,
		.epoch0_subsample_ptrn_addr          = 0x30B8,
		.epoch1_subsample_ptrn_addr          = 0x30BC,
		.debug_rup_aup_status                = 0x30C0,
		.debug_camif_1_addr                  = 0x30C4,
		.debug_camif_0_addr                  = 0x30C8,
		.frm_drop_pattern_addr               = 0x30CC,
		.frm_drop_period_addr                = 0x30D0,
		.irq_subsample_pattern_addr          = 0x30D4,
		.irq_subsample_period_addr           = 0x30D8,
		.hcrop_addr                          = 0x30DC,
		.vcrop_addr                          = 0x30E0,
		.pix_drop_pattern_addr               = 0x30E4,
		.pix_drop_period_addr                = 0x30E8,
		.line_drop_pattern_addr              = 0x30EC,
		.line_drop_period_addr               = 0x30F0,
		.debug_halt_status_addr              = 0x30F8,
		.debug_misr_val0_addr                = 0x30FC,
		.debug_misr_val1_addr                = 0x3100,
		.debug_misr_val2_addr                = 0x3104,
		.debug_misr_val3_addr                = 0x3108,
		.format_measure_cfg0_addr            = 0x310C,
		.format_measure_cfg1_addr            = 0x3110,
		.format_measure_cfg2_addr            = 0x3114,
		.format_measure0_addr                = 0x3118,
		.format_measure1_addr                = 0x311C,
		.format_measure2_addr                = 0x3120,
		.timestamp_curr0_sof_addr            = 0x3124,
		.timestamp_curr1_sof_addr            = 0x3128,
		.timestamp_perv0_sof_addr            = 0x312C,
		.timestamp_perv1_sof_addr            = 0x3130,
		.timestamp_curr0_eof_addr            = 0x3134,
		.timestamp_curr1_eof_addr            = 0x3138,
		.timestamp_perv0_eof_addr            = 0x313C,
		.timestamp_perv1_eof_addr            = 0x3140,
		.batch_period_cfg_addr               = 0,
		.batch_stream_id_cfg_addr            = 0x3150,
		.epoch0_cfg_batch_id0_addr           = 0x3154,
		.epoch1_cfg_batch_id0_addr           = 0x3158,
		.epoch0_cfg_batch_id1_addr           = 0x315C,
		.epoch1_cfg_batch_id1_addr           = 0x3160,
		.epoch0_cfg_batch_id2_addr           = 0x3164,
		.epoch1_cfg_batch_id2_addr           = 0x3168,
		.epoch0_cfg_batch_id3_addr           = 0x316C,
		.epoch1_cfg_batch_id3_addr           = 0x3170,
		.epoch0_cfg_batch_id4_addr           = 0x3174,
		.epoch1_cfg_batch_id4_addr           = 0x3178,
		.epoch0_cfg_batch_id5_addr           = 0x317C,
		.epoch1_cfg_batch_id5_addr           = 0x3180,

		/* configurations */
		.resume_frame_boundary               = 1,
		.overflow_ctrl_en                    = 1,
		.overflow_ctrl_mode_val              = 0x8,
		.mipi_pack_supported                 = 1,
		.packing_fmt_shift_val               = 15,
		.plain_alignment_shift_val           = 11,
		.plain_fmt_shift_val                 = 12,
		.crop_v_en_shift_val                 = 8,
		.crop_h_en_shift_val                 = 7,
		.drop_v_en_shift_val                 = 6,
		.drop_h_en_shift_val                 = 5,
		.early_eof_en_shift_val              = 14,
		.format_measure_en_shift_val         = 3,
		.timestamp_en_shift_val              = 6,
		.debug_byte_cntr_rst_shift_val       = 2,
		.ccif_violation_en                   = 1,
		.fatal_err_mask                      = 0x2c1c6081,
		.non_fatal_err_mask                  = 0x12000000,
		.sof_irq_mask                        = 0x10,
		.rup_irq_mask                        = 0x800000,
		.epoch0_irq_mask                     = 0x200000,
		.epoch1_irq_mask                     = 0x400000,
		.eof_irq_mask                        = 0x8,
		.rup_mask                            = 0x100,
		.aup_mask                            = 0x100,
		.rup_aup_set_mask                    = 0x1,
		.top_irq_mask                        = {0x10000,},
		.rup_aup_mask                        = 0x100010,
		.epoch0_shift_val                    = 16,
};

static struct cam_ife_csid_ver2_path_reg_info
	cam_ife_csid_lite_1080_rdi_1_reg_info = {
		.irq_status_addr                     = 0x0234,
		.irq_mask_addr                       = 0x0238,
		.irq_clear_addr                      = 0x023C,
		.irq_set_addr                        = 0x0240,
		.cfg0_addr                           = 0x3280,
		.ctrl_addr                           = 0x3288,
		.debug_clr_cmd_addr                  = 0x328C,
		.multi_vcdt_cfg0_addr                = 0x3290,
		.multi_vcdt_cfg1_addr                = 0x3284,
		.cfg1_addr                           = 0x3294,
		.debug_byte_cntr_ping_addr           = 0x32A8,
		.debug_byte_cntr_pong_addr           = 0x32AC,
		.camif_frame_cfg_addr                = 0x32B0,
		.epoch_irq_cfg_addr                  = 0x32B4,
		.epoch0_subsample_ptrn_addr          = 0x32B8,
		.epoch1_subsample_ptrn_addr          = 0x32BC,
		.debug_rup_aup_status                = 0x32C0,
		.debug_camif_1_addr                  = 0x32C4,
		.debug_camif_0_addr                  = 0x32C8,
		.frm_drop_pattern_addr               = 0x32CC,
		.frm_drop_period_addr                = 0x32D0,
		.irq_subsample_pattern_addr          = 0x32D4,
		.irq_subsample_period_addr           = 0x32D8,
		.hcrop_addr                          = 0x32DC,
		.vcrop_addr                          = 0x32E0,
		.pix_drop_pattern_addr               = 0x32E4,
		.pix_drop_period_addr                = 0x32E8,
		.line_drop_pattern_addr              = 0x32EC,
		.line_drop_period_addr               = 0x32F0,
		.debug_halt_status_addr              = 0x32F8,
		.debug_misr_val0_addr                = 0x32FC,
		.debug_misr_val1_addr                = 0x3300,
		.debug_misr_val2_addr                = 0x3304,
		.debug_misr_val3_addr                = 0x3308,
		.format_measure_cfg0_addr            = 0x330C,
		.format_measure_cfg1_addr            = 0x3310,
		.format_measure_cfg2_addr            = 0x3314,
		.format_measure0_addr                = 0x3318,
		.format_measure1_addr                = 0x331C,
		.format_measure2_addr                = 0x3320,
		.timestamp_curr0_sof_addr            = 0x3324,
		.timestamp_curr1_sof_addr            = 0x3328,
		.timestamp_perv0_sof_addr            = 0x332C,
		.timestamp_perv1_sof_addr            = 0x3330,
		.timestamp_curr0_eof_addr            = 0x3334,
		.timestamp_curr1_eof_addr            = 0x3338,
		.timestamp_perv0_eof_addr            = 0x333C,
		.timestamp_perv1_eof_addr            = 0x3340,
		.batch_period_cfg_addr               = 0,
		.batch_stream_id_cfg_addr            = 0x3350,
		.epoch0_cfg_batch_id0_addr           = 0x3354,
		.epoch1_cfg_batch_id0_addr           = 0x3358,
		.epoch0_cfg_batch_id1_addr           = 0x335C,
		.epoch1_cfg_batch_id1_addr           = 0x3360,
		.epoch0_cfg_batch_id2_addr           = 0x3364,
		.epoch1_cfg_batch_id2_addr           = 0x3368,
		.epoch0_cfg_batch_id3_addr           = 0x336C,
		.epoch1_cfg_batch_id3_addr           = 0x3370,
		.epoch0_cfg_batch_id4_addr           = 0x3374,
		.epoch1_cfg_batch_id4_addr           = 0x3378,
		.epoch0_cfg_batch_id5_addr           = 0x337C,
		.epoch1_cfg_batch_id5_addr           = 0x3380,

		/* configurations */
		.resume_frame_boundary               = 1,
		.overflow_ctrl_en                    = 1,
		.overflow_ctrl_mode_val              = 0x8,
		.mipi_pack_supported                 = 1,
		.packing_fmt_shift_val               = 15,
		.plain_alignment_shift_val           = 11,
		.plain_fmt_shift_val                 = 12,
		.crop_v_en_shift_val                 = 8,
		.crop_h_en_shift_val                 = 7,
		.drop_v_en_shift_val                 = 6,
		.drop_h_en_shift_val                 = 5,
		.early_eof_en_shift_val              = 14,
		.format_measure_en_shift_val         = 3,
		.timestamp_en_shift_val              = 6,
		.debug_byte_cntr_rst_shift_val       = 2,
		.ccif_violation_en                   = 1,
		.fatal_err_mask                      = 0x2c1c6081,
		.non_fatal_err_mask                  = 0x12000000,
		.sof_irq_mask                        = 0x10,
		.rup_irq_mask                        = 0x800000,
		.epoch0_irq_mask                     = 0x200000,
		.epoch1_irq_mask                     = 0x400000,
		.eof_irq_mask                        = 0x8,
		.rup_mask                            = 0x200,
		.aup_mask                            = 0x200,
		.rup_aup_set_mask                    = 0x1,
		.top_irq_mask                        = {0x20000,},
		.top_irq_mask                        = {0x200,},
		.epoch0_shift_val                    = 16,
};

static struct cam_ife_csid_ver2_path_reg_info
	cam_ife_csid_lite_1080_rdi_2_reg_info = {
		.irq_status_addr                     = 0x0244,
		.irq_mask_addr                       = 0x0248,
		.irq_clear_addr                      = 0x024C,
		.irq_set_addr                        = 0x0250,
		.cfg0_addr                           = 0x3480,
		.ctrl_addr                           = 0x3488,
		.debug_clr_cmd_addr                  = 0x348C,
		.multi_vcdt_cfg0_addr                = 0x3490,
		.multi_vcdt_cfg1_addr                = 0x3484,
		.cfg1_addr                           = 0x3494,
		.debug_byte_cntr_ping_addr           = 0x34A8,
		.debug_byte_cntr_pong_addr           = 0x34AC,
		.camif_frame_cfg_addr                = 0x34B0,
		.epoch_irq_cfg_addr                  = 0x34B4,
		.epoch0_subsample_ptrn_addr          = 0x34B8,
		.epoch1_subsample_ptrn_addr          = 0x34BC,
		.debug_rup_aup_status                = 0x34C0,
		.debug_camif_1_addr                  = 0x34C4,
		.debug_camif_0_addr                  = 0x34C8,
		.frm_drop_pattern_addr               = 0x34CC,
		.frm_drop_period_addr                = 0x34D0,
		.irq_subsample_pattern_addr          = 0x34D4,
		.irq_subsample_period_addr           = 0x34D8,
		.hcrop_addr                          = 0x34DC,
		.vcrop_addr                          = 0x34E0,
		.pix_drop_pattern_addr               = 0x34E4,
		.pix_drop_period_addr                = 0x34E8,
		.line_drop_pattern_addr              = 0x34EC,
		.line_drop_period_addr               = 0x34F0,
		.debug_halt_status_addr              = 0x34F8,
		.debug_misr_val0_addr                = 0x34FC,
		.debug_misr_val1_addr                = 0x3500,
		.debug_misr_val2_addr                = 0x3504,
		.debug_misr_val3_addr                = 0x3508,
		.format_measure_cfg0_addr            = 0x350C,
		.format_measure_cfg1_addr            = 0x3510,
		.format_measure_cfg2_addr            = 0x3514,
		.format_measure0_addr                = 0x3518,
		.format_measure1_addr                = 0x351C,
		.format_measure2_addr                = 0x3520,
		.timestamp_curr0_sof_addr            = 0x3524,
		.timestamp_curr1_sof_addr            = 0x3528,
		.timestamp_perv0_sof_addr            = 0x352C,
		.timestamp_perv1_sof_addr            = 0x3530,
		.timestamp_curr0_eof_addr            = 0x3534,
		.timestamp_curr1_eof_addr            = 0x3538,
		.timestamp_perv0_eof_addr            = 0x353C,
		.timestamp_perv1_eof_addr            = 0x3540,
		.batch_period_cfg_addr               = 0,
		.batch_stream_id_cfg_addr            = 0x3550,
		.epoch0_cfg_batch_id0_addr           = 0x3554,
		.epoch1_cfg_batch_id0_addr           = 0x3558,
		.epoch0_cfg_batch_id1_addr           = 0x355C,
		.epoch1_cfg_batch_id1_addr           = 0x3560,
		.epoch0_cfg_batch_id2_addr           = 0x3564,
		.epoch1_cfg_batch_id2_addr           = 0x3568,
		.epoch0_cfg_batch_id3_addr           = 0x356C,
		.epoch1_cfg_batch_id3_addr           = 0x3570,
		.epoch0_cfg_batch_id4_addr           = 0x3574,
		.epoch1_cfg_batch_id4_addr           = 0x3578,
		.epoch0_cfg_batch_id5_addr           = 0x357C,
		.epoch1_cfg_batch_id5_addr           = 0x3580,

		/* configurations */
		.resume_frame_boundary               = 1,
		.overflow_ctrl_en                    = 1,
		.overflow_ctrl_mode_val              = 0x8,
		.mipi_pack_supported                 = 1,
		.packing_fmt_shift_val               = 15,
		.plain_alignment_shift_val           = 11,
		.plain_fmt_shift_val                 = 12,
		.crop_v_en_shift_val                 = 8,
		.crop_h_en_shift_val                 = 7,
		.drop_v_en_shift_val                 = 6,
		.drop_h_en_shift_val                 = 5,
		.early_eof_en_shift_val              = 14,
		.format_measure_en_shift_val         = 3,
		.timestamp_en_shift_val              = 6,
		.debug_byte_cntr_rst_shift_val       = 2,
		.ccif_violation_en                   = 1,
		.fatal_err_mask                      = 0x2c1c6081,
		.non_fatal_err_mask                  = 0x12000000,
		.sof_irq_mask                        = 0x10,
		.rup_irq_mask                        = 0x800000,
		.epoch0_irq_mask                     = 0x200000,
		.epoch1_irq_mask                     = 0x400000,
		.eof_irq_mask                        = 0x8,
		.rup_mask                            = 0x400,
		.aup_mask                            = 0x400,
		.rup_aup_set_mask                    = 0x1,
		.top_irq_mask                        = {0x40000,},
		.epoch0_shift_val                    = 16,
};

static struct cam_ife_csid_ver2_path_reg_info
	cam_ife_csid_lite_1080_rdi_3_reg_info = {
		.irq_status_addr                     = 0x0254,
		.irq_mask_addr                       = 0x0258,
		.irq_clear_addr                      = 0x025C,
		.irq_set_addr                        = 0x0260,
		.cfg0_addr                           = 0x3680,
		.ctrl_addr                           = 0x3688,
		.debug_clr_cmd_addr                  = 0x368C,
		.multi_vcdt_cfg0_addr                = 0x3690,
		.multi_vcdt_cfg1_addr                = 0x3684,
		.cfg1_addr                           = 0x3694,
		.debug_byte_cntr_ping_addr           = 0x36A8,
		.debug_byte_cntr_pong_addr           = 0x36AC,
		.camif_frame_cfg_addr                = 0x36B0,
		.epoch_irq_cfg_addr                  = 0x36B4,
		.epoch0_subsample_ptrn_addr          = 0x36B8,
		.epoch1_subsample_ptrn_addr          = 0x36BC,
		.debug_rup_aup_status                = 0x36C0,
		.debug_camif_1_addr                  = 0x36C4,
		.debug_camif_0_addr                  = 0x36C8,
		.frm_drop_pattern_addr               = 0x36CC,
		.frm_drop_period_addr                = 0x36D0,
		.irq_subsample_pattern_addr          = 0x36D4,
		.irq_subsample_period_addr           = 0x36D8,
		.hcrop_addr                          = 0x36DC,
		.vcrop_addr                          = 0x36E0,
		.pix_drop_pattern_addr               = 0x36E4,
		.pix_drop_period_addr                = 0x36E8,
		.line_drop_pattern_addr              = 0x36EC,
		.line_drop_period_addr               = 0x36F0,
		.debug_halt_status_addr              = 0x36F8,
		.debug_misr_val0_addr                = 0x36FC,
		.debug_misr_val1_addr                = 0x3700,
		.debug_misr_val2_addr                = 0x3704,
		.debug_misr_val3_addr                = 0x3708,
		.format_measure_cfg0_addr            = 0x370C,
		.format_measure_cfg1_addr            = 0x3710,
		.format_measure_cfg2_addr            = 0x3714,
		.format_measure0_addr                = 0x3718,
		.format_measure1_addr                = 0x371C,
		.format_measure2_addr                = 0x3720,
		.timestamp_curr0_sof_addr            = 0x3724,
		.timestamp_curr1_sof_addr            = 0x3728,
		.timestamp_perv0_sof_addr            = 0x372C,
		.timestamp_perv1_sof_addr            = 0x3730,
		.timestamp_curr0_eof_addr            = 0x3734,
		.timestamp_curr1_eof_addr            = 0x3738,
		.timestamp_perv0_eof_addr            = 0x373C,
		.timestamp_perv1_eof_addr            = 0x3740,
		.batch_period_cfg_addr               = 0,
		.batch_stream_id_cfg_addr            = 0x3750,
		.epoch0_cfg_batch_id0_addr           = 0x3754,
		.epoch1_cfg_batch_id0_addr           = 0x3758,
		.epoch0_cfg_batch_id1_addr           = 0x375C,
		.epoch1_cfg_batch_id1_addr           = 0x3760,
		.epoch0_cfg_batch_id2_addr           = 0x3764,
		.epoch1_cfg_batch_id2_addr           = 0x3768,
		.epoch0_cfg_batch_id3_addr           = 0x376C,
		.epoch1_cfg_batch_id3_addr           = 0x3770,
		.epoch0_cfg_batch_id4_addr           = 0x3774,
		.epoch1_cfg_batch_id4_addr           = 0x3778,
		.epoch0_cfg_batch_id5_addr           = 0x377C,
		.epoch1_cfg_batch_id5_addr           = 0x3780,

		/* configurations */
		.resume_frame_boundary               = 1,
		.overflow_ctrl_en                    = 1,
		.overflow_ctrl_mode_val              = 0x8,
		.mipi_pack_supported                 = 1,
		.packing_fmt_shift_val               = 15,
		.plain_alignment_shift_val           = 11,
		.plain_fmt_shift_val                 = 12,
		.crop_v_en_shift_val                 = 8,
		.crop_h_en_shift_val                 = 7,
		.drop_v_en_shift_val                 = 6,
		.drop_h_en_shift_val                 = 5,
		.early_eof_en_shift_val              = 14,
		.format_measure_en_shift_val         = 3,
		.timestamp_en_shift_val              = 6,
		.debug_byte_cntr_rst_shift_val       = 2,
		.ccif_violation_en                   = 1,
		.fatal_err_mask                      = 0x2c1c6081,
		.non_fatal_err_mask                  = 0x12000000,
		.sof_irq_mask                        = 0x10,
		.rup_irq_mask                        = 0x800000,
		.epoch0_irq_mask                     = 0x200000,
		.epoch1_irq_mask                     = 0x400000,
		.eof_irq_mask                        = 0x8,
		.rup_mask                            = 0x800,
		.aup_mask                            = 0x800,
		.rup_aup_set_mask                    = 0x1,
		.top_irq_mask                        = {0x80000,},
		.epoch0_shift_val                    = 16,
};

static struct cam_ife_csid_rx_debug_mask cam_ife_csid_lite_1080_rx_debug_mask = {

	.evt_bitmap = {
		BIT_ULL(CAM_IFE_CSID_RX_DL0_EOT_CAPTURED) |
			BIT_ULL(CAM_IFE_CSID_RX_DL1_EOT_CAPTURED) |
			BIT_ULL(CAM_IFE_CSID_RX_DL2_EOT_CAPTURED) |
			BIT_ULL(CAM_IFE_CSID_RX_DL3_EOT_CAPTURED) |
			BIT_ULL(CAM_IFE_CSID_RX_DL0_SOT_CAPTURED) |
			BIT_ULL(CAM_IFE_CSID_RX_DL1_SOT_CAPTURED) |
			BIT_ULL(CAM_IFE_CSID_RX_DL2_SOT_CAPTURED) |
			BIT_ULL(CAM_IFE_CSID_RX_DL3_SOT_CAPTURED) |
			BIT_ULL(CAM_IFE_CSID_RX_LONG_PKT_CAPTURED) |
			BIT_ULL(CAM_IFE_CSID_RX_SHORT_PKT_CAPTURED) |
			BIT_ULL(CAM_IFE_CSID_RX_CPHY_PKT_HDR_CAPTURED) |
			BIT_ULL(CAM_IFE_CSID_RX_CPHY_EOT_RECEPTION) |
			BIT_ULL(CAM_IFE_CSID_RX_CPHY_SOT_RECEPTION) |
			BIT_ULL(CAM_IFE_CSID_RX_ERROR_CPHY_PH_CRC) |
			BIT_ULL(CAM_IFE_CSID_RX_WARNING_ECC) |
			BIT_ULL(CAM_IFE_CSID_RX_LANE0_FIFO_OVERFLOW) |
			BIT_ULL(CAM_IFE_CSID_RX_LANE1_FIFO_OVERFLOW) |
			BIT_ULL(CAM_IFE_CSID_RX_LANE2_FIFO_OVERFLOW) |
			BIT_ULL(CAM_IFE_CSID_RX_LANE3_FIFO_OVERFLOW) |
			BIT_ULL(CAM_IFE_CSID_RX_ERROR_CRC) |
			BIT_ULL(CAM_IFE_CSID_RX_ERROR_ECC) |
			BIT_ULL(CAM_IFE_CSID_RX_MMAPPED_VC_DT) |
			BIT_ULL(CAM_IFE_CSID_RX_UNMAPPED_VC_DT) |
			BIT_ULL(CAM_IFE_CSID_RX_STREAM_UNDERFLOW) |
			BIT_ULL(CAM_IFE_CSID_RX_UNBOUNDED_FRAME),
	},

	.bit_pos[CAM_IFE_CSID_RX_DL0_EOT_CAPTURED] = 0,
	.bit_pos[CAM_IFE_CSID_RX_DL1_EOT_CAPTURED] = 1,
	.bit_pos[CAM_IFE_CSID_RX_DL2_EOT_CAPTURED] = 2,
	.bit_pos[CAM_IFE_CSID_RX_DL3_EOT_CAPTURED] = 3,
	.bit_pos[CAM_IFE_CSID_RX_DL0_SOT_CAPTURED] = 4,
	.bit_pos[CAM_IFE_CSID_RX_DL1_SOT_CAPTURED] = 5,
	.bit_pos[CAM_IFE_CSID_RX_DL2_SOT_CAPTURED] = 6,
	.bit_pos[CAM_IFE_CSID_RX_DL3_SOT_CAPTURED] = 7,
	.bit_pos[CAM_IFE_CSID_RX_LONG_PKT_CAPTURED] = 8,
	.bit_pos[CAM_IFE_CSID_RX_SHORT_PKT_CAPTURED] = 9,
	.bit_pos[CAM_IFE_CSID_RX_CPHY_PKT_HDR_CAPTURED] = 10,
	.bit_pos[CAM_IFE_CSID_RX_CPHY_EOT_RECEPTION] = 11,
	.bit_pos[CAM_IFE_CSID_RX_CPHY_SOT_RECEPTION] = 12,
	.bit_pos[CAM_IFE_CSID_RX_ERROR_CPHY_PH_CRC] = 13,
	.bit_pos[CAM_IFE_CSID_RX_WARNING_ECC] = 14,
	.bit_pos[CAM_IFE_CSID_RX_LANE0_FIFO_OVERFLOW] = 15,
	.bit_pos[CAM_IFE_CSID_RX_LANE1_FIFO_OVERFLOW] = 16,
	.bit_pos[CAM_IFE_CSID_RX_LANE2_FIFO_OVERFLOW] = 17,
	.bit_pos[CAM_IFE_CSID_RX_LANE3_FIFO_OVERFLOW] = 18,
	.bit_pos[CAM_IFE_CSID_RX_ERROR_CRC] = 19,
	.bit_pos[CAM_IFE_CSID_RX_ERROR_ECC] = 20,
	.bit_pos[CAM_IFE_CSID_RX_MMAPPED_VC_DT] = 21,
	.bit_pos[CAM_IFE_CSID_RX_UNMAPPED_VC_DT] = 22,
	.bit_pos[CAM_IFE_CSID_RX_STREAM_UNDERFLOW] = 23,
	.bit_pos[CAM_IFE_CSID_RX_UNBOUNDED_FRAME] = 24,
};

static struct cam_ife_csid_top_debug_mask cam_ife_csid_lite_1080_top_debug_mask = {

	.evt_bitmap = {
		BIT_ULL(CAM_IFE_CSID_TOP_INFO_VOTE_UP) |
			BIT_ULL(CAM_IFE_CSID_TOP_INFO_VOTE_DN) |
			BIT_ULL(CAM_IFE_CSID_TOP_ERR_NO_VOTE_DN),
	},

	.bit_pos[CAM_IFE_CSID_TOP_INFO_VOTE_UP] = 16,
	.bit_pos[CAM_IFE_CSID_TOP_INFO_VOTE_DN] = 17,
	.bit_pos[CAM_IFE_CSID_TOP_ERR_NO_VOTE_DN] = 18,
};


static struct cam_ife_csid_ver2_reg_info cam_ife_csid_lite_1080_reg_info = {
	.top_irq_reg_info      = cam_ife_csid_lite_1080_top_irq_reg_info,
	.rx_irq_reg_info       = cam_ife_csid_lite_1080_rx_irq_reg_info,
	.path_irq_reg_info     = {
		&cam_ife_csid_lite_1080_path_irq_reg_info[CAM_IFE_PIX_PATH_RES_RDI_0],
		&cam_ife_csid_lite_1080_path_irq_reg_info[CAM_IFE_PIX_PATH_RES_RDI_1],
		&cam_ife_csid_lite_1080_path_irq_reg_info[CAM_IFE_PIX_PATH_RES_RDI_2],
		&cam_ife_csid_lite_1080_path_irq_reg_info[CAM_IFE_PIX_PATH_RES_RDI_3],
		NULL,
		&cam_ife_csid_lite_1080_path_irq_reg_info[CAM_IFE_PIX_PATH_RES_IPP],
		},
	.buf_done_irq_reg_info = &cam_ife_csid_lite_1080_buf_done_irq_reg_info,
	.cmn_reg               = &cam_ife_csid_lite_1080_cmn_reg_info,
	.csi2_reg              = &cam_ife_csid_lite_1080_csi2_reg_info,
	.path_reg[CAM_IFE_PIX_PATH_RES_IPP]   = &cam_ife_csid_lite_1080_ipp_reg_info,
	.path_reg[CAM_IFE_PIX_PATH_RES_PPP]   = NULL,
	.path_reg[CAM_IFE_PIX_PATH_RES_RDI_0] = &cam_ife_csid_lite_1080_rdi_0_reg_info,
	.path_reg[CAM_IFE_PIX_PATH_RES_RDI_1] = &cam_ife_csid_lite_1080_rdi_1_reg_info,
	.path_reg[CAM_IFE_PIX_PATH_RES_RDI_2] = &cam_ife_csid_lite_1080_rdi_2_reg_info,
	.path_reg[CAM_IFE_PIX_PATH_RES_RDI_3] = &cam_ife_csid_lite_1080_rdi_3_reg_info,
	.need_top_cfg = 0,
	.top_irq_desc       = &cam_ife_csid_lite_1080_top_irq_desc,
	.rx_irq_desc        = &cam_ife_csid_lite_1080_rx_irq_desc,
	.debug_vec_desc     = &cam_ife_csid_lite_1080_debug_vec_desc,
	.path_irq_desc      = cam_ife_csid_lite_1080_path_irq_desc,
	.num_top_err_irqs   = cam_ife_csid_lite_1080_num_top_irq_desc,
	.num_rx_err_irqs    = cam_ife_csid_lite_1080_num_rx_irq_desc,
	.num_path_err_irqs  = ARRAY_SIZE(cam_ife_csid_lite_1080_path_irq_desc),
	.top_debug_mask     = &cam_ife_csid_lite_1080_top_debug_mask,
	.rx_debug_mask      = &cam_ife_csid_lite_1080_rx_debug_mask,
	.num_top_regs       = 2,
	.num_rx_regs        = 2,
};

#endif /* _CAM_IFE_CSID_LITE_1080_H_ */
