/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _CPASTOP_V980_100_H_
#define _CPASTOP_V980_100_H_

static struct cam_camnoc_irq_sbm cam_cpas_v980_100_irq_sbm_rt = {
	.sbm_enable = {
		.access_type = CAM_REG_TYPE_READ_WRITE,
		.enable = true,
		.offset = 0x240,  /* CAM_NOC_RT_SBM_FAULTINEN0_LOW */
		.value = 0x01 |   /* RT_SBM_FAULTINEN0_LOW_PORT0_MASK - Slave error IRQ */
			0x02,    /* RT_SBM_FAULTINEN0_LOW_PORT1_MASK - TFE UBWC Encoder Error IRQ */
	},
	.sbm_status = {
		.access_type = CAM_REG_TYPE_READ,
		.enable = true,
		.offset = 0x248, /* CAM_NOC_RT_SBM_FAULTINSTATUS0_LOW */
	},
	.sbm_clear = {
		.access_type = CAM_REG_TYPE_WRITE,
		.enable = true,
		.offset = 0x280, /* CAM_NOC_RT_SBM_FLAGOUTCLR0_LOW */
		.value = 0x7,
	}
};

static struct cam_camnoc_irq_sbm cam_cpas_v980_100_irq_sbm_nrt = {
	.sbm_enable = {
		.access_type = CAM_REG_TYPE_READ_WRITE,
		.enable = true,
		.offset = 0x240,  /* CAM_NOC_NRT_SBM_FAULTINEN0_LOW */
		.value = 0x01 |   /* NRT_SBM_FAULTINEN0_LOW_PORT0_MASK - Slave Error */
			0x02 |    /* NRT_SBM_FAULTINEN0_LOW_PORT1_MASK - IPE WR UBWC En */
			0x04 |    /* NRT_SBM_FAULTINEN0_LOW_PORT2_MASK - OFE WR UBWC En */
			0x08 |    /* NRT_SBM_FAULTINEN0_LOW_PORT3_MASK - OFE RD UBWC De */
			0x10 |    /* NRT_SBM_FAULTINEN0_LOW_PORT4_MASK - IPE RD UBWC En */
			0x20,     /* NRT_SBM_FAULTINEN0_LOW_PORT5_MASK - IPE RD UBWC En */
	},
	.sbm_status = {
		.access_type = CAM_REG_TYPE_READ,
		.enable = true,
		.offset = 0x248, /* CAM_NOC_NRT_SBM_FAULTINSTATUS0_LOW */
	},
	.sbm_clear = {
		.access_type = CAM_REG_TYPE_WRITE,
		.enable = true,
		.offset = 0x280, /* CAM_NOC_NRT_SBM_FLAGOUTCLR0_LOW */
		.value = 0x7,
	}
};

static struct cam_camnoc_irq_err
	cam_cpas_v980_100_irq_err_rt[] = {
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_SLAVE_ERROR,
		.enable = true,
		.sbm_port = 0x1, /* RT_SBM_FAULTINSTATUS0_LOW_PORT0_MASK */
		.err_enable = {
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.enable = true,
			.offset = 0x8, /* CAM_NOC_RT_ERL_MAINCTL_LOW */
			.value = 0x1,
		},
		.err_status = {
			.access_type = CAM_REG_TYPE_READ,
			.enable = true,
			.offset = 0x10, /* CAM_NOC_RT_ERL_ERRVLD_LOW */
		},
		.err_clear = {
			.access_type = CAM_REG_TYPE_WRITE,
			.enable = true,
			.offset = 0x18, /* CAM_NOC_RT_ERL_ERRCLR_LOW */
			.value = 0x1,
		},
	},
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_TFE_UBWC_ENCODE_ERROR,
		.enable = false,
		.sbm_port = 0x2, /* RT_SBM_FAULTINSTATUS0_LOW_PORT1_MASK */
		.err_enable = {
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.enable = false,
			.offset = 0x47A0, /* TFE_UBWC : RT_0_NIU_ENCERREN_LOW */
			.value = 0xF,
		},
		.err_status = {
			.access_type = CAM_REG_TYPE_READ,
			.enable = false,
			.offset = 0x4790, /* IFE_UBWC : RT_0_NIU_ENCERRSTATUS_LOW */
		},
		.err_clear = {
			.access_type = CAM_REG_TYPE_WRITE,
			.enable = false,
			.offset = 0x4798, /* IFE_UBWC : RT_0_NIU_ENCERRCLR_LOW */
			.value = 0x1,
		},
	},
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_RESERVED1,
		.enable = false,
	},
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_RESERVED2,
		.enable = false,
	},
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_CAMNOC_TEST,
		.enable = false,
		.sbm_port = 0x20, /* RT_SBM_FAULTINSTATUS0_LOW_PORT5_MASK */
		.err_enable = {
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.enable = true,
			.offset = 0x288, /* RT_CAM_NOC_RT_SBM_FLAGOUTSET0_LOW */
			.value = 0x1,
		},
		.err_status = {
			.access_type = CAM_REG_TYPE_READ,
			.enable = true,
			.offset = 0x290, /* CAM_NOC_RT_SBM_FLAGOUTSTATUS0_LOW */
		},
		.err_clear = {
			.enable = false, /* CAM_NOC_RT_SBM_FLAGOUTCLR0_LOW */
		},
	},
};

static struct cam_camnoc_irq_err
	cam_cpas_v980_100_irq_err_nrt[] = {
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_SLAVE_ERROR,
		.enable = true,
		.sbm_port = 0x1, /* NRT_SBM_FAULTINSTATUS0_LOW_PORT0_MASK */
		.err_enable = {
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.enable = true,
			.offset = 0x8, /* CAM_NOC_NRT_ERL_MAINCTL_LOW */
			.value = 0x1,
		},
		.err_status = {
			.access_type = CAM_REG_TYPE_READ,
			.enable = true,
			.offset = 0x10, /* CAM_NOC_NRT_ERL_ERRVLD_LOW */
		},
		.err_clear = {
			.access_type = CAM_REG_TYPE_WRITE,
			.enable = true,
			.offset = 0x18, /* CAM_NOC_NRT_ERL_ERRCLR_LOW */
			.value = 0x1,
		},
	},
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_OFE_UBWC_WRITE_ENCODE_ERROR,
		.enable = true,
		.sbm_port = 0x4, /* NRT_SBM_FAULTINSTATUS0_LOW_PORT2_MASK */
		.err_enable = {
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.enable = true,
			.offset = 0x63A0, /* OFE_WR : NRT_3_NIU_ENCERREN_LOW */
			.value = 0xF,
		},
		.err_status = {
			.access_type = CAM_REG_TYPE_READ,
			.enable = true,
			.offset = 0x6390, /* OFE_WR : NRT_3_NIU_ENCERRSTATUS_LOW */
		},
		.err_clear = {
			.access_type = CAM_REG_TYPE_WRITE,
			.enable = true,
			.offset = 0x6398, /* OFE_WR : NRT_3_NIU_ENCERRCLR_LOW */
			.value = 0x1,
		},
	},
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_OFE_UBWC_READ_DECODE_ERROR,
		.enable = true,
		.sbm_port = 0x8, /* NRT_SBM_FAULTINSTATUS0_LOW_PORT3_MASK */
		.err_enable = {
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.enable = true,
			.offset = 0x6520, /* OFE_RD : NRT_4_NIU_DECERREN_LOW */
			.value = 0xFF,
		},
		.err_status = {
			.access_type = CAM_REG_TYPE_READ,
			.enable = true,
			.offset = 0x6510, /* OFE_RD : NRT_4_NIU_DECERRSTATUS_LOW */
		},
		.err_clear = {
			.access_type = CAM_REG_TYPE_WRITE,
			.enable = true,
			.offset = 0x6518, /* OFE_RD : NRT_4_NIU_DECERRCLR_LOW */
			.value = 0x1,
		},
	},
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_IPE_UBWC_ENCODE_ERROR,
		.enable = true,
		.sbm_port = 0x2, /* NRT_SBM_FAULTINSTATUS0_LOW_PORT1_MASK */
		.err_enable = {
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.enable = true,
			.offset = 0x59A0, /* IPE_WR : NRT_1_NIU_ENCERREN_LOW */
			.value = 0xF,
		},
		.err_status = {
			.access_type = CAM_REG_TYPE_READ,
			.enable = true,
			.offset = 0x5990, /* IPE_WR : NRT_1_NIU_ENCERRSTATUS_LOW */
		},
		.err_clear = {
			.access_type = CAM_REG_TYPE_WRITE,
			.enable = true,
			.offset = 0x5998, /* IPE_WR : NRT_1_NIU_ENCERRCLR_LOW */
			.value = 0x1,
		},
	},
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_IPE0_UBWC_DECODE_ERROR,
		.enable = true,
		.sbm_port = 0x20, /* NRT_SBM_FAULTINSTATUS0_LOW_PORT5_MASK */
		.err_enable = {
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.enable = true,
			.offset = 0x6D20, /* IPE_0_RD : NRT_8_NIU_DECERREN_LOW */
			.value = 0xFF,
		},
		.err_status = {
			.access_type = CAM_REG_TYPE_READ,
			.enable = true,
			.offset = 0x6D10, /* IPE_0_RD : NRT_8_NIU_DECERRSTATUS_LOW */
		},
		.err_clear = {
			.access_type = CAM_REG_TYPE_WRITE,
			.enable = true,
			.offset = 0x6D18, /* IPE_0_RD : NRT_8_NIU_DECERRCLR_LOW */
			.value = 0x1,
		},
	},
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_IPE1_UBWC_DECODE_ERROR,
		.enable = true,
		.sbm_port = 0x10, /* NRT_SBM_FAULTINSTATUS0_LOW_PORT4_MASK */
		.err_enable = {
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.enable = true,
			.offset = 0x6B20, /* IPE_1_RD : NRT_7_NIU_DECERREN_LOW */
			.value = 0xFF,
		},
		.err_status = {
			.access_type = CAM_REG_TYPE_READ,
			.enable = true,
			.offset = 0x6B10, /* IPE_1_RD : NRT_7_NIU_DECERRSTATUS_LOW */
		},
		.err_clear = {
			.access_type = CAM_REG_TYPE_WRITE,
			.enable = true,
			.offset = 0x6B18, /* IPE_1_RD : NRT_7_NIU_DECERRCLR_LOW */
			.value = 0xFF,
		},
	},
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_AHB_TIMEOUT,
		.enable = false,
		.sbm_port = 0x40, /* NRT_SBM_FAULTINSTATUS0_LOW_PORT6_MASK */
		.err_enable = {
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.enable = true,
			.offset = 0x288, /* CAM_NOC_NRT_SBM_FLAGOUTSET0_LOW */
			.value = 0xE,
		},
		.err_status = {
			.access_type = CAM_REG_TYPE_READ,
			.enable = true,
			.offset = 0x290, /* CAM_NOC_NRT_SBM_FLAGOUTSTATUS0_LOW */
		},
		.err_clear = {
			.enable = false, /* CAM_NOC_NRT_SBM_FLAGOUTCLR0_LOW */
		},
	},
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_RESERVED1,
		.enable = false,
	},
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_RESERVED2,
		.enable = false,
	},
	{
		.irq_type = CAM_CAMNOC_HW_IRQ_CAMNOC_TEST,
		.enable = false,
		.sbm_port = 0x400, /* NRT_SBM_FAULTINSTATUS0_LOW_PORT10_MASK */
		.err_enable = {
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.enable = true,
			.offset = 0x288, /* CAM_NOC_NRT_SBM_FLAGOUTSET0_LOW */
			.value = 0x2,
		},
		.err_status = {
			.access_type = CAM_REG_TYPE_READ,
			.enable = true,
			.offset = 0x290, /* CAM_NOC_NRT_SBM_FLAGOUTSTATUS0_LOW */
		},
		.err_clear = {
			.enable = false, /* CAM_NOC_NRT_SBM_FLAGOUTCLR0_LOW */
		},
	},
};

static struct cam_camnoc_specific
	cam_cpas_v980_100_camnoc_specific_rt[] = {
	/* RT ports */
	{
		.port_name = "RT0-TFE0,1_FULL_DS2_PDAF1_TFE2_RDI_IR_DS4_DS16",
		.enable = true,
		.priority_lut_low = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4630, /* NOC_RT_0_NIU_PRIORITYLUT_LOW */
			.value = 0x66655554,
		},
		.priority_lut_high = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4634, /* NOC_RT_0_NIU_PRIORITYLUT_HIGH */
			.value = 0x66666666,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4638, /* NOC_RT_0_NIU_URGENCY_LOW */
			.value = 0x00001E40,
		},
		.danger_lut = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4640, /* NOC_RT_0_NIU_DANGERLUT_LOW */
			.value = 0xFFFFFF00,
		},
		.safe_lut = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4648, /* NOC_RT_0_NIU_SAFELUT_LOW */
			.value = 0x0000000F,
		},
		.ubwc_ctl = {
			/*
			 * Do not explicitly set ubwc config register.
			 * Power on default values are taking care of required
			 * register settings.
			 */
			.enable = false,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4E08, /* NOC_RT_0_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4008, /* NOC_RT_0_QOSGEN_MAINCTL */
			.value = 0x0,
		},
		.qosgen_shaping_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4020, /* NOC_RT_0_QOSGEN_SHAPING_LOW */
			.value = 0x0,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4024, /* NOC_RT_0_QOSGEN_SHAPING_HIGH */
			.value = 0x0,
		},
		.maxwr_low = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ,
			.masked_value = 0,
			.offset = 0x4620, /* NOC_RT_0_NIU_MAXWR_LOW */
			.value = 0x0,
		},
	},
	{
		.port_name = "RT1-TFE0,1,2_PDAF_FD_IFE_LITE_0,1_RDI_GAMMA_STATS",
		.enable = true,
		.priority_lut_low = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4830, /* NOC_RT_1_NIU_PRIORITYLUT_LOW */
			.value = 0x55544444,
		},
		.priority_lut_high = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4834, /* NOC_RT_1_NIU_PRIORITYLUT_HIGH */
			.value = 0x66666665,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4838, /* NOC_RT_1_NIU_URGENCY_LOW */
			.value = 0x00001C40,
		},
		.danger_lut = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4840, /* NOC_RT_1_NIU_DANGERLUT_LOW */
			.value = 0xFFFFFF00,
		},
		.safe_lut = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4848, /* NOC_RT_1_NIU_SAFELUT_LOW */
			.value = 0x0000000F,
		},
		.ubwc_ctl = {
			/*
			 * Do not explicitly set ubwc config register.
			 * Power on default values are taking care of required
			 * register settings.
			 */
			.enable = false,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4E88, /* NOC_RT_1_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4088, /* NOC_RT_1_QOSGEN_MAINCTL */
			.value = 0x0,
		},
		.qosgen_shaping_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x40A0, /* NOC_RT_1_QOSGEN_SHAPING_LOW */
			.value = 0x0,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x40A4, /* NOC_RT_1_QOSGEN_SHAPING_HIGH */
			.value = 0x0,
		},
		.maxwr_low = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ,
			.masked_value = 0,
			.offset = 0x4820, /* NOC_RT_1_NIU_MAXWR_LOW */
			.value = 0x0,
		},
	},
	{
		.port_name = "RT2-TFE0,1,2_STATS",
		.enable = true,
		.priority_lut_low = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4A30, /* NOC_RT_2_NIU_PRIORITYLUT_LOW */
			.value = 0x55554444,
		},
		.priority_lut_high = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4A34, /* NOC_RT_2_NIU_PRIORITYLUT_HIGH */
			.value = 0x66666665,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4A38, /* NOC_RT_2_NIU_URGENCY_LOW */
			.value = 0x00001C40,
		},
		.danger_lut = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4A40, /* NOC_RT_2_NIU_DANGERLUT_LOW */
			.value = 0xFFFFFF00,
		},
		.safe_lut = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4A48, /* NOC_RT_2_NIU_SAFELUT_LOW */
			.value = 0x0000000F,
		},
		.ubwc_ctl = {
			/*
			 * Do not explicitly set ubwc config register.
			 * Power on default values are taking care of required
			 * register settings.
			 */
			.enable = false,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4F08, /* NOC_RT_2_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4108, /* NOC_RT_2_QOSGEN_MAINCTL */
			.value = 0x0,
		},
		.qosgen_shaping_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4120, /* NOC_RT_2_QOSGEN_SHAPING_LOW */
			.value = 0x0,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4124, /* NOC_RT_2_QOSGEN_SHAPING_HIGH */
			.value = 0x0,
		},
		.maxwr_low = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ,
			.masked_value = 0,
			.offset = 0x4A20, /* NOC_RT_2_NIU_MAXWR_LOW */
			.value = 0x0,
		},
	},
	{
		.port_name = "RT3-TFE0,1,2_IFE_LITE0,1_CDM",
		.enable = true,
		.priority_lut_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4C30, /* NOC_RT_3_NIU_PRIORITYLUT_LOW */
			.value = 0x33333333,
		},
		.priority_lut_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4C34, /* NOC_RT_3_NIU_PRIORITYLUT_HIGH */
			.value = 0x33333333,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4C38, /* NOC_RT_3_NIU_URGENCY_LOW */
			.value = 0x3,
		},
		.danger_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4C40, /* NOC_RT_3_NIU_DANGERLUT_LOW */
			.value = 0x0,
		},
		.safe_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4C48, /* NOC_RT_3_NIU_SAFELUT_LOW */
			.value = 0xF,
		},
		.ubwc_ctl = {
			/*
			 * Do not explicitly set ubwc config register.
			 * Power on default values are taking care of required
			 * register settings.
			 */
			.enable = false,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4F88, /* NOC_RT_3_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4188, /* NOC_RT_3_QOSGEN_MAINCTL */
			.value = 0x0,
		},
		.qosgen_shaping_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x41A0, /* NOC_RT_3_QOSGEN_SHAPING_LOW */
			.value = 0x0,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x41A4, /* NOC_RT_3_QOSGEN_SHAPING_HIGH */
			.value = 0x0,
		},
	},
	{
		.port_name = "RT4-TFE0,1_RDI_IR_DS4_DS16_TFE2_FULL_DS2_PDAF1",
		.enable = true,
		.priority_lut_low = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5230, /* NOC_RT_4_NIU_PRIORITYLUT_LOW */
			.value = 0x65555544,
		},
		.priority_lut_high = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5234, /* NOC_RT_4_NIU_PRIORITYLUT_HIGH */
			.value = 0x66666666,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5238, /* NOC_RT_4_NIU_URGENCY_LOW */
			.value = 0x00001E40,
		},
		.danger_lut = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5240, /* NOC_RT_4_NIU_DANGERLUT_LOW */
			.value = 0xFFFFFF00,
		},
		.safe_lut = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5248, /* NOC_RT_4_NIU_SAFELUT_LOW */
			.value = 0x0000000F,
		},
		.ubwc_ctl = {
			/*
			 * Do not explicitly set ubwc config register.
			 * Power on default values are taking care of required
			 * register settings.
			 */
			.enable = false,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5408, /* NOC_RT_4_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.maxwr_low = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ,
			.masked_value = 0,
			.offset = 0x5220, /* NOC_RT_4_NIU_MAXWR_LOW */
			.value = 0x0,
		},
	},
};

static struct cam_camnoc_specific
	cam_cpas_v980_100_camnoc_specific_nrt[] = {
	/* NRT ports */
	{
		.port_name = "NRT0-IPE_WR_1",
		.enable = true,
		.priority_lut_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5630, /* IPE_WR_1 : NOC_NRT_0_NIU_PRIORITYLUT_LOW */
			.value = 0x0,
		},
		.priority_lut_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5634, /* IPE_WR_1 : NOC_NRT_0_NIU_PRIORITYLUT_HIGH */
			.value = 0x0,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5638, /* IPE_WR_1 : NOC_NRT_0_NIU_URGENCY_LOW */
			.value = 0x30,
		},
		.danger_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5640, /* IPE_WR_1 : NOC_NRT_0_NIU_DANGERLUT_LOW */
			.value = 0x0,
		},
		.safe_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5648, /* IPE_WR_1 : NOC_NRT_0_NIU_SAFELUT_LOW */
			.value = 0x0000FFFF,
		},
		.ubwc_ctl = {
			.enable = false,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x7008, /* IPE_WR_1 : NOC_NRT_0_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4808, /* IPE_WR_1 : NOC_NRT_0_QOSGEN_MAINCTL */
			.value = 0x0,
		},
		.qosgen_shaping_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4820, /* IPE_WR_1 : NOC_NRT_0_QOSGEN_SHAPING_LOW */
			.value = 0x0,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4824, /* IPE_WR_1 : NOC_NRT_0_QOSGEN_SHAPING_HIGH */
			.value = 0x0,
		},
	},
	{
		.port_name = "NRT1-IPE_WR_0",
		.enable = true,
		.priority_lut_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5830, /* IPE_WR_0 : NOC_NRT_1_NIU_PRIORITYLUT_LOW */
			.value = 0x0,
		},
		.priority_lut_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5834, /* IPE_WR_0 : NOC_NRT_1_NIU_PRIORITYLUT_HIGH */
			.value = 0x0,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5838, /* IPE_WR_0 : NOC_NRT_1_NIU_URGENCY_LOW */
			.value = 0x30,
		},
		.danger_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5840, /* IPE_WR_0 : NOC_NRT_1_NIU_DANGERLUT_LOW */
			.value = 0x0,
		},
		.safe_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x5848, /* IPE_WR_0 : NOC_NRT_1_NIU_SAFELUT_LOW */
			.value = 0x0000FFFF,
		},
		.ubwc_ctl = {
			.enable = false,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x7088, /* IPE_WR_0 : NOC_NRT_1_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4888, /* IPE_WR_0 : NOC_NRT_1_QOSGEN_MAINCTL */
			.value = 0x0,
		},
		.qosgen_shaping_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x48A0, /* IPE_WR_0 : NOC_NRT_1_QOSGEN_SHAPING_LOW */
			.value = 0x0,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x48A4, /* IPE_WR_0 : NOC_NRT_1_QOSGEN_SHAPING_HIGH */
			.value = 0x0,
		},
		.maxwr_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ,
			.masked_value = 0,
			.offset = 0x5820, /* IPE_WR_0 : NOC_NRT_1_NIU_MAXWR_LOW */
			.value = 0x0,
		},
	},
	{
		.port_name = "NRT2-OFE_WR_1-CRE_WR",
		.enable = true,
		.priority_lut_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6030, /* OFE_WR_1-CRE_WR : NOC_NRT_2_NIU_PRIORITYLUT_LOW */
			.value = 0x0,
		},
		.priority_lut_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6034, /* OFE_WR_1-CRE_WR : NOC_NRT_2_NIU_PRIORITYLUT_HIGH */
			.value = 0x0,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6038, /* OFE_WR_1-CRE_WR : NOC_NRT_2_NIU_URGENCY_LOW */
			.value = 0x30,
		},
		.danger_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6040, /* OFE_WR_1-CRE_WR : NOC_NRT_2_NIU_DANGERLUT_LOW */
			.value = 0x0,
		},
		.safe_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6048, /* OFE_WR_1-CRE_WR : NOC_NRT_2_NIU_SAFELUT_LOW */
			.value = 0x0000FFFF,
		},
		.ubwc_ctl = {
			.enable = false,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x7108, /* OFE_WR_1-CRE_WR : NOC_NRT_2_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4908, /* OFE_WR_1-CRE_WR : NOC_NRT_2_QOSGEN_MAINCTL */
			.value = 0x0,
		},
		.qosgen_shaping_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4920, /* OFE_WR_1-CRE_WR : NOC_NRT_2_QOSGEN_SHAPING_LOW */
			.value = 0x0,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4924, /* OFE_WR_1-CRE_WR:NOC_NRT_2_BPS_WR_QOSGEN_SHAPING_HIGH */
			.value = 0x0,
		},
		.maxwr_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ,
			.masked_value = 0,
			.offset = 0x6020, /* OFE_WR_1-CRE_WR : NOC_NRT_2_NIU_MAXWR_LOW */
			.value = 0x0,
		},
	},
	{
		.port_name = "NRT3-OFE_WR_0",
		.enable = true,
		.priority_lut_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6230, /* OFE_WR_0 : NOC_NRT_3_NIU_PRIORITYLUT_LOW */
			.value = 0x0,
		},
		.priority_lut_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6234, /* OFE_WR_0 : NOC_NRT_3_NIU_PRIORITYLUT_HIGH */
			.value = 0x0,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6238, /* OFE_WR_0 : NOC_NRT_3_NIU_URGENCY_LOW */
			.value = 0x30,
		},
		.danger_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6240, /* OFE_WR_0 : NOC_NRT_3_NIU_DANGERLUT_LOW */
			.value = 0x0,
		},
		.safe_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6248, /* OFE_WR_0 : NOC_NRT_3_NIU_SAFELUT_LOW */
			.value = 0x0000FFFF,
		},
		.ubwc_ctl = {
			.enable = false,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x7188, /* OFE_WR_0 : NOC_NRT_3_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4988, /* OFE_WR_0 : NOC_NRT_3_QOSGEN_MAINCTL */
			.value = 0x0,
		},
		.qosgen_shaping_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x49A0, /* OFE_WR_0 : NOC_NRT_3_QOSGEN_SHAPING_LOW */
			.value = 0x0,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x49A4, /* OFE_WR_0 : NOC_NRT_3_QOSGEN_SHAPING_HIGH */
			.value = 0x0,
		},
	},
	{
		.port_name = "NRT4-OFE_RD",
		.enable = true,
		.priority_lut_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6430, /* OFE_RD : NOC_NRT_4_NIU_PRIORITYLUT_LOW */
			.value = 0x0,
		},
		.priority_lut_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6434, /* OFE_RD : NOC_NRT_4_NIU_PRIORITYLUT_HIGH */
			.value = 0x0,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6438, /* OFE_RD : NOC_NRT_4_NIU_URGENCY_LOW */
			.value = 0x3,
		},
		.danger_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6440, /* OFE_RD : NOC_NRT_4_NIU_DANGERLUT_LOW */
			.value = 0x0,
		},
		.safe_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6448, /* OFE_RD : NOC_NRT_4_NIU_SAFELUT_LOW */
			.value = 0x0000FFFF,
		},
		.ubwc_ctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6508, /* OFE_RD : NOC_NRT_4_NIU_DECCTL_LOW */
			.value = 1,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x7208, /* OFE_RD : NOC_NRT_4_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4A08, /* OFE_RD : NOC_NRT_4_QOSGEN_MAINCTL */
			.value = 0x0,
		},
		.qosgen_shaping_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4A20, /* OFE_RD : NOC_NRT_4_QOSGEN_SHAPING_LOW */
			.value = 0x0,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4A24, /* OFE_RD : NOC_NRT_4_QOSGEN_SHAPING_HIGH */
			.value = 0x0,
		},
		.maxrd_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ,
			.masked_value = 0,
			.offset = 0x6410, /* OFE_RD : NOC_NRT_4_NIU_MAXRD_LOW */
			.value = 0x0,
		},
	},
	{
		.port_name = "NRT5-CRE_RD",
		.enable = true,
		.priority_lut_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6630, /* CRE_RD : NOC_NRT_5_NIU_PRIORITYLUT_LOW */
			.value = 0x0,
		},
		.priority_lut_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6634, /* CRE_RD : NOC_NRT_5_NIU_PRIORITYLUT_HIGH */
			.value = 0x0,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6638, /* CRE_RD : NOC_NRT_5_NIU_URGENCY_LOW */
			.value = 0x3,
		},
		.danger_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6640, /* CRE_RD : NOC_NRT_5_NIU_DANGERLUT_LOW */
			.value = 0x0,
		},
		.safe_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6648, /* CRE_RD : NOC_NRT_5_NIU_SAFELUT_LOW */
			.value = 0x0000FFFF,
		},
		.ubwc_ctl = {
			.enable = false,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x7288, /* CRE_RD : NOC_NRT_5_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4A88, /* CRE_RD : NOC_NRT_5_QOSGEN_MAINCTL */
			.value = 0x2,
		},
		.qosgen_shaping_low = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4AA0, /* CRE_RD : NOC_NRT_5_QOSGEN_SHAPING_LOW */
			.value = 0x14141414,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4AA4, /* CRE_RD : NOC_NRT_5_QOSGEN_SHAPING_HIGH */
			.value = 0x14141414,
		},
		.maxrd_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ,
			.masked_value = 0,
			.offset = 0x6610, /* CRE_RD : NOC_NRT_5_NIU_MAXRD_LOW */
			.value = 0x0,
		},
	},
	{
		.port_name = "NRT6-JPEG0,1,2,3_RD_WR",
		.enable = true,
		.priority_lut_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6830, /* JPEG_RD_WR : NOC_NRT_6_NIU_PRIORITYLUT_LOW */
			.value = 0x0,
		},
		.priority_lut_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6834, /* JPEG_RD_WR : NOC_NRT_6_NIU_PRIORITYLUT_HIGH */
			.value = 0x0,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6838, /* JPEG_RD_WR : NOC_NRT_6_NIU_URGENCY_LOW */
			.value = 0x22,
		},
		.danger_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6840, /* JPEG_RD_WR : NOC_NRT_6_NIU_DANGERLUT_LOW */
			.value = 0x0,
		},
		.safe_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6848, /* JPEG_RD_WR : NOC_NRT_6_NIU_SAFELUT_LOW */
			.value = 0xFFFF,
		},
		.ubwc_ctl = {
			.enable = false,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x7308, /* JPEG_RD_WR : NOC_NRT_6_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4B08, /* JPEG_RD_WR : NOC_NRT_6_QOSGEN_MAINCTL */
			.value = 0x2,
		},
		.qosgen_shaping_low = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4B20, /* JPEG_RD_WR : NOC_NRT_6_QOSGEN_SHAPING_LOW */
			.value = 0x10101010,
		},
		.qosgen_shaping_high = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4B24, /* JPEG_RD_WR : NOC_NRT_6_QOSGEN_SHAPING_HIGH */
			.value = 0x10101010,
		},
		.maxwr_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ,
			.masked_value = 0,
			.offset = 0x6820, /* JPEG_RD_WR : NOC_NRT_6_NIU_MAXWR_LOW */
			.value = 0x0,
		},
		.maxrd_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ,
			.masked_value = 0,
			.offset = 0x6810, /* JPEG_RD_WR : NOC_NRT_6_NIU_MAXRD_LOW */
			.value = 0x0,
		},
	},
	{
		.port_name = "NRT7-IPE_RD_1",
		.enable = true,
		.priority_lut_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6A30, /* IPE_WR_1 : NOC_NRT_7_NIU_PRIORITYLUT_LOW */
			.value = 0x0,
		},
		.priority_lut_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6A34, /* IPE_WR_1 : NOC_NRT_7_NIU_PRIORITYLUT_HIGH */
			.value = 0x0,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6A38, /* IPE_WR_1 : NOC_NRT_7_NIU_URGENCY_LOW */
			.value = 0x3,
		},
		.danger_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6A40, /* IPE_WR_1 : NOC_NRT_7_NIU_DANGERLUT_LOW */
			.value = 0x0,
		},
		.safe_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6A48, /* IPE_WR_1 : NOC_NRT_7_NIU_SAFELUT_LOW */
			.value = 0x0000FFFF,
		},
		.ubwc_ctl = {
			.enable = false,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x7388, /* IPE_WR_1 : NOC_NRT_7_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4B88, /* IPE_WR_1 : NOC_NRT_7_QOSGEN_MAINCTL */
			.value = 0x0,
		},
		.qosgen_shaping_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4BA0, /* IPE_WR_1 : NOC_NRT_7_QOSGEN_SHAPING_LOW */
			.value = 0x0,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4BA4, /* IPE_WR_1 : NOC_NRT_7_QOSGEN_SHAPING_HIGH */
			.value = 0x0,
		},
		.maxrd_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ,
			.masked_value = 0,
			.offset = 0x6A10, /* IPE_WR_1 : NOC_NRT_7_NIU_MAXRD_LOW */
			.value = 0x0,
		},
	},
	{
		.port_name = "NRT8-IPE_RD_0",
		.enable = true,
		.priority_lut_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6C30, /* IPE_RD_0 : NOC_NRT_8_NIU_PRIORITYLUT_LOW */
			.value = 0x0,
		},
		.priority_lut_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6C34, /* IPE_RD_0 : NOC_NRT_8_NIU_PRIORITYLUT_HIGH */
			.value = 0x0,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6C38, /* IPE_RD_0 : NOC_NRT_8_NIU_URGENCY_LOW */
			.value = 0x3,
		},
		.danger_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6C40, /* IPE_RD_0 : NOC_NRT_8_NIU_DANGERLUT_LOW */
			.value = 0x0,
		},
		.safe_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6C48, /* IPE_RD_0 : NOC_NRT_8_NIU_SAFELUT_LOW */
			.value = 0x0000FFFF,
		},
		.ubwc_ctl = {
			.enable = false,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x7408, /* IPE_RD_0 : NOC_NRT_8_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4C08, /* IPE_RD_0 : NOC_NRT_8_QOSGEN_MAINCTL */
			.value = 0x0,
		},
		.qosgen_shaping_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4C20, /* IPE_RD_0 : NOC_NRT_8_QOSGEN_SHAPING_LOW */
			.value = 0x0,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4C24, /* IPE_RD_0 : NOC_NRT_8_QOSGEN_SHAPING_HIGH */
			.value = 0x0,
		},
		.maxrd_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ,
			.masked_value = 0,
			.offset = 0x6C10, /* IPE_RD_0 : NOC_NRT_8_NIU_MAXRD_LOW */
			.value = 0x0,
		},
	},
	{
		.port_name = "NRT9-CDM_IPE_OFE",
		.enable = true,
		.priority_lut_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6E30, /* CDM_IPE_OFE : NOC_NRT_9_NIU_PRIORITYLUT_LOW */
			.value = 0x0,
		},
		.priority_lut_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6E34, /* CDM_IPE_OFE : NOC_NRT_9_NIU_PRIORITYLUT_HIGH */
			.value = 0x0,
		},
		.urgency = {
			.enable = true,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6E38, /* CDM_IPE_OFE : NOC_NRT_9_NIU_URGENCY_LOW */
			.value = 0x3,
		},
		.danger_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6E40, /* CDM_IPE_OFE : NOC_NRT_9_NIU_DANGERLUT_LOW */
			.value = 0x0,
		},
		.safe_lut = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x6E48, /* CDM_IPE_OFE : NOC_NRT_9_NIU_SAFELUT_LOW */
			.value = 0x0000FFFF,
		},
		.ubwc_ctl = {
			.enable = false,
		},
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x7488, /* CDM_IPE_OFE : NOC_NRT_9_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4C88, /* CDM_IPE_OFE : NOC_NRT_9_QOSGEN_MAINCTL */
			.value = 0x0,
		},
		.qosgen_shaping_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4CA0, /* CDM_IPE_OFE : NOC_NRT_9_QOSGEN_SHAPING_LOW */
			.value = 0x0,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4CA4, /* CDM_IPE_OFE : NOC_NRT_9_QOSGEN_SHAPING_HIGH */
			.value = 0x0,
		},
		.maxrd_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ,
			.masked_value = 0,
			.offset = 0x6E10, /* CDM_IPE_OFE : NOC_NRT_10_NIU_MAXRD_LOW */
			.value = 0x0,
		},
	},
	{
		.port_name = "ICP_0_RD_WR",
		.enable = false,
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x7508, /* ICP_RD_WR : NOC_XM_ICP_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4D08, /* ICP_RD_WR : NOC_XM_ICP_QOSGEN_MAINCTL */
			.value = 0x0,
		},
		.qosgen_shaping_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4D20, /* ICP_RD_WR : NOC_XM_ICP_QOSGEN_SHAPING_LOW */
			.value = 0x0,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4D24, /* ICP_RD_WR : NOC_XM_ICP_QOSGEN_SHAPING_HIGH */
			.value = 0x0,
		},
	},
	{
		.port_name = "ICP_1_RD_WR",
		.enable = false,
		.dynattr_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x7588, /* ICP_RD_WR : NOC_XM_ICP_DYNATTR_MAINCTL */
			.value = 0x0,
		},
		.qosgen_mainctl = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4D88, /* ICP_RD_WR : NOC_XM_ICP_QOSGEN_MAINCTL */
			.value = 0x0,
		},
		.qosgen_shaping_low = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4DA0, /* ICP_RD_WR : NOC_XM_ICP_QOSGEN_SHAPING_LOW */
			.value = 0x0,
		},
		.qosgen_shaping_high = {
			.enable = false,
			.access_type = CAM_REG_TYPE_READ_WRITE,
			.masked_value = 0,
			.offset = 0x4DA4, /* ICP_RD_WR : NOC_XM_ICP_QOSGEN_SHAPING_HIGH */
			.value = 0x0,
		},
	},
};

static struct cam_camnoc_err_logger_info cam980_cpas100_err_logger_offsets = {
	.mainctrl     =  0x08, /* NOC_ERL_MAINCTL_LOW */
	.errvld       =  0x10, /* NOC_ERL_ERRVLD_LOW */
	.errlog0_low  =  0x20, /* NOC_ERL_ERRLOG0_LOW */
	.errlog0_high =  0x24, /* NOC_ERL_ERRLOG0_HIGH */
	.errlog1_low  =  0x28, /* NOC_ERL_ERRLOG1_LOW */
	.errlog1_high =  0x2C, /* NOC_ERL_ERRLOG1_HIGH */
	.errlog2_low  =  0x30, /* NOC_ERL_ERRLOG2_LOW */
	.errlog2_high =  0x34, /* NOC_ERL_ERRLOG2_HIGH */
	.errlog3_low  =  0x38, /* NOC_ERL_ERRLOG3_LOW */
	.errlog3_high =  0x3C, /* NOC_ERL_ERRLOG3_HIGH */
};

static struct cam_cpas_hw_errata_wa_list cam980_cpas100_errata_wa_list = {
	.camnoc_flush_slave_pending_trans = {
		.enable = false,
		.data.reg_info = {
			.access_type = CAM_REG_TYPE_READ,
			.offset = 0x300, /* sbm_SenseIn0_Low */
			.mask = 0xE0000, /* Bits 17, 18, 19 */
			.value = 0, /* expected to be 0 */
		},
	},
	.enable_icp_clk_for_qchannel = {
		.enable = false,
	},
};

static struct cam_cpas_cesta_vcd_reg_info cam_cpas_v980_100_cesta_reg_info = {
	.vcd_currol = {
		.reg_offset = 0x266C,
		.vcd_base_inc = 0x210,
		.num_vcds = 8,
	},
};

static struct cam_cpas_vcd_info cam_v980_100_vcd_info[] = {
	{
		.index = 0, .type = CAM_CESTA_CRMC, .clk = "cam_cc_tfe_0_clk_src",
	},
	{
		.index = 1, .type = CAM_CESTA_CRMC, .clk = "cam_cc_tfe_1_clk_src",
	},
	{
		.index = 2, .type = CAM_CESTA_CRMC, .clk = "cam_cc_tfe_2_clk_src",
	},
	{
		.index = 6, .type = CAM_CESTA_CRMC, .clk = "cam_cc_csid_clk_src",
	},
	{
		.index = 7, .type = CAM_CESTA_CRMC, .clk = "cam_cc_cphy_rx_clk_src",
	},
	{
		.index = 8, .type = CAM_CESTA_CRMB, .clk = "cam_cc_camnoc_axi_rt_clk_src",
	},
};

static struct cam_cpas_cesta_info cam_v980_cesta_info = {
	 .vcd_info = &cam_v980_100_vcd_info[0],
	 .num_vcds = ARRAY_SIZE(cam_v980_100_vcd_info),
	 .cesta_reg_info = &cam_cpas_v980_100_cesta_reg_info,
};

static struct cam_camnoc_addr_trans_client_info
	cam980_cpas100_addr_trans_client_info[] = {
	{
		.client_name = "icp1",
		.reg_enable = 0x5508,
		.reg_offset0 = 0x5518,
		.reg_base1 = 0x5520,
		.reg_offset1 = 0x5528,
		.reg_base2 = 0x5530,
		.reg_offset2 = 0x5538,
		.reg_base3 = 0x5540,
		.reg_offset3 = 0x5548,
	},
};

static struct cam_camnoc_addr_trans_info cam980_cpas100_addr_trans_info = {
	.num_supported_clients = ARRAY_SIZE(cam980_cpas100_addr_trans_client_info),
	.addr_trans_client_info = &cam980_cpas100_addr_trans_client_info[0],
};

static struct cam_camnoc_info cam980_cpas100_camnoc_info_rt = {
	.specific = &cam_cpas_v980_100_camnoc_specific_rt[0],
	.specific_size = ARRAY_SIZE(cam_cpas_v980_100_camnoc_specific_rt),
	.irq_sbm = &cam_cpas_v980_100_irq_sbm_rt,
	.irq_err = &cam_cpas_v980_100_irq_err_rt[0],
	.irq_err_size = ARRAY_SIZE(cam_cpas_v980_100_irq_err_rt),
	.err_logger = &cam980_cpas100_err_logger_offsets,
	.errata_wa_list = &cam980_cpas100_errata_wa_list,
	.test_irq_info = {
		.sbm_enable_mask = 0x20,
		.sbm_clear_mask = 0x4,
	},
};

static struct cam_camnoc_info cam980_cpas100_camnoc_info_nrt = {
	.specific = &cam_cpas_v980_100_camnoc_specific_nrt[0],
	.specific_size = ARRAY_SIZE(cam_cpas_v980_100_camnoc_specific_nrt),
	.irq_sbm = &cam_cpas_v980_100_irq_sbm_nrt,
	.irq_err = &cam_cpas_v980_100_irq_err_nrt[0],
	.irq_err_size = ARRAY_SIZE(cam_cpas_v980_100_irq_err_nrt),
	.err_logger = &cam980_cpas100_err_logger_offsets,
	.errata_wa_list = &cam980_cpas100_errata_wa_list,
	.test_irq_info = {
		.sbm_enable_mask = 0x400,
		.sbm_clear_mask = 0x1,
	},
	.addr_trans_info = &cam980_cpas100_addr_trans_info,
};

static struct cam_cpas_camnoc_qchannel cam980_cpas100_qchannel_info_rt = {
	.qchannel_ctrl   = 0xEC,
	.qchannel_status = 0xF0,
};

static struct cam_cpas_camnoc_qchannel cam980_cpas100_qchannel_info_nrt = {
	.qchannel_ctrl   = 0xF4,
	.qchannel_status = 0xF8,
};

/*
 * struct cam_cpas_secure_info: CPAS secure information are used only for
 * debug purpose, Register access is restricted in normal builds.
 *
 */
static struct cam_cpas_secure_info cam980_cpas100_secure_info = {
	.secure_access_ctrl_offset = 0x1C,
	.secure_access_ctrl_value = 0xFFFFFFFF,
};

static struct cam_cpas_subpart_info cam980_cpas_camera_subpart_info = {
	.num_bits = 3,
	/*
	 * Below fuse indexing is based on software fuse definition which is in SMEM and provided
	 * by XBL team.
	 */
	.hw_bitmap_mask = {
		{CAM_CPAS_ISP_FUSE, BIT(0)},
		{CAM_CPAS_ISP_FUSE, BIT(1)},
		{CAM_CPAS_ISP_FUSE, BIT(2)},
	}
};

static struct cam_cpas_info cam980_cpas100_cpas_info = {
	.hw_caps_info = {
		.num_caps_registers = 2,
		.hw_caps_offsets = {0x8, 0xDC},
	},
	.qchannel_info = {&cam980_cpas100_qchannel_info_rt,
		&cam980_cpas100_qchannel_info_nrt},
	.num_qchannel = 2,
	.hw_caps_secure_info = &cam980_cpas100_secure_info,
	.subpart_info = &cam980_cpas_camera_subpart_info,
};

#endif /* _CPASTOP_V980_100_H_ */
