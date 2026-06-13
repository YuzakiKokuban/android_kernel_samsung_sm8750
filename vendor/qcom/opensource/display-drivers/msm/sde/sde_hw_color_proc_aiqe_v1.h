/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#ifndef _SDE_HW_COLOR_PROC_AIQE_H_
#define _SDE_HW_COLOR_PROC_AIQE_H_

#include "sde_hw_dspp.h"

/**
 * sde_read_mdnie_art_done - api to read art done value
 * @ctx: pointer to dspp object.
 * @art_done: Pointer to art done.
 */
int sde_read_mdnie_art_done(struct sde_hw_dspp *ctx, uint32_t *art_done);

/**
 * sde_read_copr_status - api to read copr status
 * @ctx: pointer to dspp object.
 * @copr_status: Pointer to copr status.
 */
int sde_read_copr_status(struct sde_hw_dspp *ctx, struct drm_msm_copr_status *copr_status);

/**
 * sde_setup_mdnie_art_v1 - api to setup mdnie art programming
 * @ctx: pointer to dspp object.
 * @cfg: Pointer to sde_hw_cp_cfg.
 * @aiqe_top: Pointer to aiqe top level structure
 */
void sde_setup_mdnie_art_v1(struct sde_hw_dspp *ctx, void *cfg, void *aiqe_top);

/**
 * sde_reset_mdnie_art - api to reset mdnie art after art done
 * @ctx: pointer to dspp object.
 */
void sde_reset_mdnie_art(struct sde_hw_dspp *ctx);

/**
 * sde_setup_aiqe_abc_v1 - api to setup aiqe abc programming
 * @ctx: pointer to dspp object.
 * @cfg: pointer to sde_hw_cp_cfg.
 * @aiqe_top: pointer to aiqe top level structure
 */
void sde_setup_aiqe_abc_v1(struct sde_hw_dspp *ctx, void *cfg, void *aiqe_top);

/**
 * sde_setup_copr_v1 - api to setup copr programming
 * @ctx: pointer to dspp object.
 * @cfg: Pointer to sde_hw_cp_cfg.
 * @aiqe_top: Pointer to aiqe top level structure
 */
void sde_setup_copr_v1(struct sde_hw_dspp *ctx, void *cfg, void *aiqe_top);

/**
 * sde_setup_mdnie_psr - api to setup mdnie psr programming
 * @ctx: pointer to dspp object.
 */
void sde_setup_mdnie_psr(struct sde_hw_dspp *ctx);

/**
 * reg_dmav1_setup_mdnie_v1 - api to setup mdnie programming
 * @ctx: pointer to dspp object.
 * @cfg: Pointer to sde_hw_cp_cfg.
 * @aiqe_top: Pointer to aiqe top level structure
 */
void reg_dmav1_setup_mdnie_v1(struct sde_hw_dspp *ctx, void *cfg, void *aiqe_top);

/**
 * sde_check_ai_scaler_v1 - api to check ai scaler config
 * @ctx: pointer to dspp object.
 * @cfg: Pointer to sde_hw_cp_cfg.
 */
int sde_check_ai_scaler_v1(struct sde_hw_dspp *ctx, void *cfg);

/**
 * sde_setup_ai_scaler_v1 - api to setup ai scaler programming throgh AHB
 * @ctx: pointer to dspp object.
 * @cfg: Pointer to sde_hw_cp_cfg.
 */
int sde_setup_ai_scaler_v1(struct sde_hw_dspp *ctx, void *cfg);

/**
 * sde_validate_aiqe_ssrc_data_v1 - api to validate SSRC data for SSRC V1
 * @ctx: pointer to dspp information structure
 * @cfg: pointer to sde_hw_cp_cfg payload
 * @mdnie_top: Pointer to top level mdnie structure
 */
int sde_validate_aiqe_ssrc_data_v1(struct sde_hw_dspp *ctx, void *cfg, void *mdnie_top);

/**
 * reg_dmav1_setup_aiqe_ssrc_config_v1 - api to setup SSRC config for SSRC V1
 * @ctx: pointer to dspp information structure
 * @cfg: pointer to sde_hw_cp_cfg payload
 * @mdnie_top: Pointer to top level mdnie structure
 */
void reg_dmav1_setup_aiqe_ssrc_config_v1(struct sde_hw_dspp *ctx, void *cfg, void *mdnie_top);

/**
 * reg_dmav1_setup_aiqe_ssrc_data_v1 - api to setup SSRC data for SSRC V1
 * @ctx: pointer to dspp information structure
 * @cfg: pointer to sde_hw_cp_cfg payload
 * @mdnie_top: Pointer to top level mdnie structure
 */
void reg_dmav1_setup_aiqe_ssrc_data_v1(struct sde_hw_dspp *ctx, void *cfg, void *mdnie_top);

#endif /* _SDE_HW_COLOR_PROC_AIQE_H_ */
