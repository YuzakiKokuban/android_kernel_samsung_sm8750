/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _SDE_COLOR_PROCESSING_AIQE_H
#define _SDE_COLOR_PROCESSING_AIQE_H

/**
 * set_mdnie_feature - setup dspp ops for mdnie
 * @hw_dspp: pointer to dspp hardware description
 * @hw_cfg: pointer to feature configuration
 * @hw_crtc: pointer to sde crtc data structure
 */
int set_mdnie_feature(struct sde_hw_dspp *hw_dspp, struct sde_hw_cp_cfg *hw_cfg,
			 struct sde_crtc *hw_crtc);

/**
 * set_mdnie_art_feature - setup dspp ops for mdnie art
 * @hw_dspp: pointer to dspp hardware description
 * @hw_cfg: pointer to feature configuration
 * @hw_crtc: pointer to sde crtc data structure
 */
int set_mdnie_art_feature(struct sde_hw_dspp *hw_dspp, struct sde_hw_cp_cfg *hw_cfg,
			     struct sde_crtc *hw_crtc);

/**
 * check_aiqe_ssrc_data - validate SSRC data payload
 * @hw_dspp: pointer to dspp hardware description
 * @hw_cfg: pointer to feature configuration
 * @hw_crtc: pointer to virtualized crtc data structure
 */
int check_aiqe_ssrc_data(struct sde_hw_dspp *hw_dspp, struct sde_hw_cp_cfg *hw_cfg,
		struct sde_crtc *hw_crtc);
/**
 * set_aiqe_ssrc_config - set SSRC config to HW
 * @hw_dspp: pointer to dspp hardware description
 * @hw_cfg: pointer to feature configuration
 * @hw_crtc: pointer to virtualized crtc data structure
 */
int set_aiqe_ssrc_config(struct sde_hw_dspp *hw_dspp, struct sde_hw_cp_cfg *hw_cfg,
		struct sde_crtc *hw_crtc);
/**
 * set_aiqe_ssrc_data - set SSRC data to HW
 * @hw_dspp: pointer to dspp hardware description
 * @hw_cfg: pointer to feature configuration
 * @hw_crtc: pointer to virtualized crtc data structure
 */
int set_aiqe_ssrc_data(struct sde_hw_dspp *hw_dspp, struct sde_hw_cp_cfg *hw_cfg,
		struct sde_crtc *hw_crtc);

/**
 * set_copr_feature - setup dspp ops for copr
 * @hw_dspp: pointer to dspp hardware description
 * @hw_cfg: pointer to feature configuration
 * @hw_crtc: pointer to sde crtc data structure
 */
int set_copr_feature(struct sde_hw_dspp *hw_dspp, struct sde_hw_cp_cfg *hw_cfg,
			 struct sde_crtc *hw_crtc);

/**
 * sde_dspp_copr_read_status - read copr_status registers
 * @hw_dspp: pointer to dspp hardware description
 * @copr_status: pointer to copr status values
 */
int sde_dspp_copr_read_status(struct sde_hw_dspp *hw_dspp,
			struct drm_msm_copr_status *copr_status);

/**
 * _aiqe_caps_update - update hw capabilities for aiqe block
 * @crtc: pointer to crtc
 * @info: pointer to connector information structure container
 */
void _aiqe_caps_update(struct sde_crtc *crtc, struct sde_kms_info *info);

/**
 * _dspp_aiqe_install_property - install aiqe drm properties
 * @crtc: pointer to drm crtc
 */
void _dspp_aiqe_install_property(struct drm_crtc *crtc);

/**
 * sde_set_mdnie_psr - set mdnie panel self refresh flag
 * @sde_crtc: pointer to sde crtc
 */
void sde_set_mdnie_psr(struct sde_crtc *sde_crtc);

/**
 * _dspp_ai_scaler_install_property - install ai scaler drm properties
 * @crtc: pointer to drm crtc
 */
void _dspp_ai_scaler_install_property(struct drm_crtc *crtc);

/**
 * check_ai_scaler_feature - check ai scaler feature config
 * @hw_dspp: pointer to dspp hardware description
 * @hw_cfg: pointer to feature configuration
 * @hw_crtc: pointer to virtualized crtc data structure
 */
int check_ai_scaler_feature(struct sde_hw_dspp *hw_dspp, struct sde_hw_cp_cfg *hw_cfg,
			struct sde_crtc *hw_crtc);

/**
 * set_ai_scaler_feature - setup ai scaler feature config
 * @hw_dspp: pointer to dspp hardware description
 * @hw_cfg: pointer to feature configuration
 * @hw_crtc: pointer to virtualized crtc data structure
 */
int set_ai_scaler_feature(struct sde_hw_dspp *hw_dspp, struct sde_hw_cp_cfg *hw_cfg,
		    struct sde_crtc *hw_crtc);

/**
 * set_aiqe_abc_feature - setup ops for aiqe abc feature
 * @hw_dspp: pointer to dspp hardware description
 * @hw_cfg: pointer to feature configuration
 * @hw_crtc: pointer to virtualized crtc data structure
 */
int set_aiqe_abc_feature(struct sde_hw_dspp *hw_dspp, struct sde_hw_cp_cfg *hw_cfg,
						struct sde_crtc *hw_crtc);

#endif /* _SDE_COLOR_PROCESSING_AIQE_H */
