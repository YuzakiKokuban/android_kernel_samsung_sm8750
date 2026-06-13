/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
 */

#ifndef _SDE_HW_REG_DMA_V1_COLOR_PROC_H
#define _SDE_HW_REG_DMA_V1_COLOR_PROC_H

#include "sde_hw_util.h"
#include "sde_hw_catalog.h"
#include "sde_hw_dspp.h"
#include "sde_hw_sspp.h"

#define LOG_FEATURE_OFF SDE_EVT32(ctx->idx, ctx->dpu_idx, 0)
#define LOG_FEATURE_ON SDE_EVT32(ctx->idx, ctx->dpu_idx, 1)

#define REG_DMA_INIT_OPS(cfg, block, reg_dma_feature, feature_dma_buf) \
	do { \
		memset(&cfg, 0, sizeof(cfg)); \
		(cfg).blk = block; \
		(cfg).feature = reg_dma_feature; \
		(cfg).dma_buf = feature_dma_buf; \
	} while (0)

#define REG_DMA_SETUP_OPS(cfg, block_off, data_ptr, data_len, op, \
		wrap_sz, wrap_inc, reg_mask) \
	do { \
		(cfg).ops = op; \
		(cfg).blk_offset = block_off; \
		(cfg).data_size = data_len; \
		(cfg).data = data_ptr; \
		(cfg).inc = wrap_inc; \
		(cfg).wrap_size = wrap_sz; \
		(cfg).mask = reg_mask; \
	} while (0)

#define REG_DMA_SETUP_KICKOFF(cfg, hw_ctl, feature_dma_buf, ops, ctl_q, \
		mode, reg_dma_feature) \
	do { \
		memset(&cfg, 0, sizeof(cfg)); \
		(cfg).ctl = hw_ctl; \
		(cfg).dma_buf = feature_dma_buf; \
		(cfg).op = ops; \
		(cfg).dma_type = REG_DMA_TYPE_DB; \
		(cfg).queue_select = ctl_q; \
		(cfg).trigger_mode = mode; \
		(cfg).feature = reg_dma_feature; \
	} while (0)

extern struct sde_reg_dma_buffer *dspp_buf[REG_DMA_FEATURES_MAX][DSPP_MAX][DPU_MAX];
extern u32 dspp_mapping[DSPP_MAX];

/**
 * reg_dma_buf_init - regdma buffer initialization
 * @buff - pointer to regdma buffer structure
 * @sz - regdma buffer size
 * @dpu_idx - index of target dpu
 */
int reg_dma_buf_init(struct sde_reg_dma_buffer **buf, u32 sz, u32 dpu_idx);

/**
 * reg_dma_dspp_check - basic validation for dspp features using regdma
 * @ctx - pointer to dspp object
 * @cfg - pointer to sde_hw_cp_cfg
 * @feature - dspp feature using regdma
 */
int reg_dma_dspp_check(struct sde_hw_dspp *ctx, void *cfg,
		enum sde_reg_dma_features feature);

/**
 * reg_dmav1_init_dspp_op_v4() - initialize the dspp feature op for sde v4
 *                               using reg dma v1.
 * @feature: dspp feature
 * @ctx: dspp instance
 */
int reg_dmav1_init_dspp_op_v4(int feature, struct sde_hw_dspp *ctx);

/**
 * reg_dmav1_setup_dspp_vlutv18() - vlut v18 implementation using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_vlutv18(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_3d_gamutv4() - gamut v4 implementation using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_3d_gamutv4(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_3d_gamutv41() - gamut v4_1 implementation using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_3d_gamutv41(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_3d_gamutv42() - gamut v4_2 implementation using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_3d_gamutv42(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_dspp_gcv18() - gc v18 implementation using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_gcv18(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_dspp_gcv2() - gc v2 implementation using reg dma v1.
 * @ctx: dspp ctx info
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_gcv2(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_dspp_igcv31() - igc v31 implementation using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_igcv31(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_dspp_pccv4() - pcc v4 implementation using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_pccv4(struct sde_hw_dspp *ctx, void *cfg);


/**
 * reg_dmav1_setup_dspp_pccv5() - pcc v5 implementation using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_pccv5(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_dspp_pa_hsicv17() - pa hsic v17 impl using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_pa_hsicv17(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_dspp_sixzonev17() - sixzone v17 impl using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_sixzonev17(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav2_setup_dspp_sixzonev2() - sixzone v2 impl using reg dma v2.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav2_setup_dspp_sixzonev2(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_dspp_memcol_skinv17() - memcol skin v17 impl using
 * reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_memcol_skinv17(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_dspp_memcol_skyv17() - memcol sky v17 impl using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_memcol_skyv17(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_dspp_memcol_folv17() - memcol foliage v17 impl using
 * reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_memcol_folv17(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_dspp_memcol_protv17() - memcol prot v17 impl using
 * reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_dspp_memcol_protv17(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_deinit_dspp_ops() - deinitialize the dspp feature op for sde v4
 *                               which were initialized.
 * @ctx: dspp instance
 */
int reg_dmav1_deinit_dspp_ops(struct sde_hw_dspp *ctx);

/**
 * reg_dmav1_init_sspp_op_v4() - initialize the sspp feature op for sde v4
 * @feature: sspp feature
 * @ctx: sspp instance
 */
int reg_dmav1_init_sspp_op_v4(int feature, struct sde_hw_pipe *ctx);

/**
 * reg_dmav1_setup_vig_gamutv5() - VIG 3D lut gamut v5 implementation
 *                                 using reg dma v1.
 * @ctx: sspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_vig_gamutv5(struct sde_hw_pipe *ctx, void *cfg);

/**
 * reg_dmav1_setup_vig_gamutv6() - VIG 3D lut gamut v6 implementation
 *                                 using reg dma v1.
 * @ctx: sspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_vig_gamutv6(struct sde_hw_pipe *ctx, void *cfg);

/**
 * reg_dmav1_setup_vig_igcv5() - VIG 1D lut IGC v5 implementation
 *                               using reg dma v1.
 * @ctx: sspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_vig_igcv5(struct sde_hw_pipe *ctx, void *cfg);

/**
 * reg_dmav1_setup_dma_igcv5() - DMA 1D lut IGC v5 implementation
 *                               using reg dma v1.
 * @ctx: sspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 * @idx: multirect index
 */
void reg_dmav1_setup_dma_igcv5(struct sde_hw_pipe *ctx, void *cfg,
			enum sde_sspp_multirect_index idx);
/**
 * reg_dmav1_setup_vig_igcv6() - VIG ID lut IGC v6 implementation
 *				 using reg dma v1.
 * @ctx: sspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_vig_igcv6(struct sde_hw_pipe *ctx, void *cfg);

/**
 * reg_dmav1_setup_dma_gcv5() - DMA 1D lut GC v5 implementation
 *                              using reg dma v1.
 * @ctx: sspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 * @idx: multirect index
 */
void reg_dmav1_setup_dma_gcv5(struct sde_hw_pipe *ctx, void *cfg,
			enum sde_sspp_multirect_index idx);

/**
 * reg_dmav1_setup_vig_qseed3 - Qseed3 implementation using reg dma v1.
 * @ctx: sspp instance
 * @sspp: pointer to sspp hw config
 * @pe: pointer to pixel extension config
 * @scaler_cfg: pointer to scaler config
 */

void reg_dmav1_setup_vig_qseed3(struct sde_hw_pipe *ctx,
	struct sde_hw_pipe_cfg *sspp, struct sde_hw_pixel_ext *pe,
	void *scaler_cfg);

/**reg_dmav1_setup_scaler3_lut - Qseed3 lut coefficient programming
 * @buf: defines structure for reg dma ops on the reg dma buffer.
 * @scaler3_cfg: QSEEDv3 configuration
 * @offset: Scaler Offest
 * @dpu_idx: dpu index
 */

void reg_dmav1_setup_scaler3_lut(struct sde_reg_dma_setup_ops_cfg *buf,
		struct sde_hw_scaler3_cfg *scaler3_cfg, u32 offset,
		u32 dpu_idx);

/**reg_dmav1_setup_scaler3lite_lut - Qseed3lite lut coefficient programming
 * @buf: defines structure for reg dma ops on the reg dma buffer.
 * @scaler3_cfg: QSEEDv3 configuration
 * @offset: Scaler Offest
 * @dpu_idx: dpu index
 */

void reg_dmav1_setup_scaler3lite_lut(struct sde_reg_dma_setup_ops_cfg *buf,
		struct sde_hw_scaler3_cfg *scaler3_cfg, u32 offset,
		u32 dpu_idx);

/**
 * reg_dmav1_deinit_sspp_ops() - deinitialize the sspp feature op for sde v4
 *                               which were initialized.
 * @ctx: sspp instance
 */
int reg_dmav1_deinit_sspp_ops(struct sde_hw_pipe *ctx);

/**
 * reg_dmav1_init_ltm_op_v6() - initialize the ltm feature op for sde v6
 * @feature: ltm feature
 * @ctx: dspp instance
 */
int reg_dmav1_init_ltm_op_v6(int feature, struct sde_hw_dspp *ctx);

/**
 * reg_dmav1_setup_ltm_initv1() - LTM INIT v1 implementation using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_ltm_initv1(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_ltm_roiv1() - LTM ROI v1 implementation using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_ltm_roiv1(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_ltm_roiv1_3() - LTM ROI v1.3 implementation using reg dma v1.
 * @ctx: dspp ctx info
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_ltm_roiv1_3(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_ltm_vlutv1() - LTM VLUT v1 implementation using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_ltm_vlutv1(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_ltm_vlutv1_2() - Same as v1 except ltm merge mode setting.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_ltm_vlutv1_2(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_rc_pu_configv1() - RC PU CFG v1 implementation using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 * @returns: 0 if success, non-zero otherwise
 */
int reg_dmav1_setup_rc_pu_configv1(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_rc_mask_configv1() - RC Mask CFG v1 implementation using reg dma v1.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 * @returns: 0 if success, non-zero otherwise
 */
int reg_dmav1_setup_rc_mask_configv1(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_deinit_ltm_ops() - deinitialize the ltm feature op for sde v4
 *                               which were initialized.
 * @ctx: dspp instance
 */
int reg_dmav1_deinit_ltm_ops(struct sde_hw_dspp *ctx);

/**
 * reg_dmav2_init_dspp_op_v4() - initialize the dspp feature op for sde v4
 *                               using reg dma v2.
 * @feature: dspp feature
 * @ctx: dspp instance
 */
int reg_dmav2_init_dspp_op_v4(int feature, struct sde_hw_dspp *ctx);

/**
 * reg_dmav2_setup_dspp_igcv4() - igc v4 implementation using reg dma v2.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav2_setup_dspp_igcv4(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav2_setup_dspp_igcv5() - igc v5 implementation using reg dma v2.
 * @ctx: dspp ctx info
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav2_setup_dspp_igcv5(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav2_setup_3d_gamutv43() - gamut v4_3 implementation using reg dma v2.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav2_setup_dspp_3d_gamutv43(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav2_setup_vig_gamutv61() - VIG 3D lut gamut v61 implementation
 *                                 using reg dma v2.
 * @ctx: sspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav2_setup_vig_gamutv61(struct sde_hw_pipe *ctx, void *cfg);

/**
 * reg_dmav2_init_spr_op_v1() - initialize spr sub-feature buffer for reg dma v2.
 * @feature: SPR sub-feature
 * @ctx: dspp instance
 */
int reg_dmav2_init_spr_op_v1(int feature, struct sde_hw_dspp *ctx);

/**
 * reg_dmav1_setup_spr_init_cfgv1 - function to configure spr through LUTDMA
 * @ctx: Pointer to dspp context
 * @cfg: Pointer to configuration
 */
void reg_dmav1_setup_spr_init_cfgv1(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_spr_init_cfgv2 - function to configure spr v2 through LUTDMA
 * @ctx: Pointer to dspp context
 * @cfg: Pointer to configuration
 */
void reg_dmav1_setup_spr_init_cfgv2(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_spr_udc_cfgv2 - function to configure spr v2 UDC through LUTDMA
 * @ctx: Pointer to dspp context
 * @cfg: Pointer to configuration
 */
void reg_dmav1_setup_spr_udc_cfgv2(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_spr_pu_cfgv1 - function to configure spr pu through LUTDMA
 * @ctx: Pointer to dspp context
 * @cfg: Pointer to configuration
 */
void reg_dmav1_setup_spr_pu_cfgv1(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_spr_pu_cfgv2 - function to configure spr v2 pu through LUTDMA
 * @ctx: Pointer to dspp context
 * @cfg: Pointer to configuration
 */
void reg_dmav1_setup_spr_pu_cfgv2(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_demurav1() - function to set up the demurav1 configuration.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_demurav1(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_demurav2() - function to set up the demurav2 configuration.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_demurav2(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_demura_cfg0_param2() - function to set up the demura cfg0 param2 configuration.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_demura_cfg0_param2(struct sde_hw_dspp *ctx, void *cfg);

/**
 * reg_dmav1_setup_demurav3() - function to set up the demura v3 configuration.
 * @ctx: dspp instance
 * @cfg: pointer to struct sde_hw_cp_cfg
 */
void reg_dmav1_setup_demurav3(struct sde_hw_dspp *ctx, void *cfg);

#endif /* _SDE_HW_REG_DMA_V1_COLOR_PROC_H */
