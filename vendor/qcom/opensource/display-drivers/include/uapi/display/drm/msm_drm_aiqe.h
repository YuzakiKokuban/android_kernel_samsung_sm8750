/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#ifndef _MSM_DRM_AIQE_H_
#define _MSM_DRM_AIQE_H_

#include <linux/types.h>

#define AIQE_MDNIE_SUPPORTED
#define AIQE_MDNIE_PARAM_LEN 118
/**
 * struct drm_msm_mdnie - mDNIe feature structure
 * @flags - Setting flags for mDNIe feature
 * @param - Parameters for mDNIe feature
 */
struct drm_msm_mdnie {
	__u64 flags;
	__u32 param[AIQE_MDNIE_PARAM_LEN];
};

/**
 * struct drm_msm_mdnie_art - mDNIe ART feature structure
 * @flags - Setting flags for mDNIe ART feature
 * @param - mDNIe ART parameter
 */
struct drm_msm_mdnie_art {
	__u64 flags;
	__u32 param;
};

/**
 * struct drm_msm_mdnie_art_done - mDNIe ART INTR structure
 * @art_done - mDNIe ART done parameter
 */
struct drm_msm_mdnie_art_done {
	__u32 art_done;
};


#define AIQE_SSRC_SUPPORTED
/*
 * struct drm_msm_ssrc_config - AIQE SSRC configuration structure
 * @flags - Configuration flags for AIQE SSRC
 * @config - Configuration data
 */
#define AIQE_SSRC_PARAM_LEN 16
struct drm_msm_ssrc_config {
	__u32 flags;
	__u32 config[AIQE_SSRC_PARAM_LEN];
};

/*
 * struct drm_msm_ssrc_data - AIQE SSRC data update structure
 * @data_size - Size of total region data
 * @data - Region data for SRAM. Format is as follows:
 *  Addr 0 - Region A size
 *  Addr 1:{Region A size} -  SRAM data
 *  Addr {Region A size + 1} - Region B size
 *  ...
 *
 * Data description must match size reported in data_size.
 */
#define AIQE_SSRC_DATA_LEN 5128
struct drm_msm_ssrc_data {
	__u32 data_size;
	__u32 data[AIQE_SSRC_DATA_LEN];
};

#define AIQE_COPR_PARAM_LEN 17
/**
 * struct drm_msm_copr - COPR feature structure
 * @flags - Setting flags for COPR feature
 * @param - Parameters for COPR feature
 */
struct drm_msm_copr {
	__u64 flags;
	__u32 param[AIQE_COPR_PARAM_LEN];
};

#define AIQE_COPR_STATUS_LEN 10
/**
 * struct drm_msm_copr_status - COPR read only status structure
 * @status - Parameters for COPR statistics read registers
 */
struct drm_msm_copr_status {
	__u32 status[AIQE_COPR_STATUS_LEN];
};

#define AIQE_AI_SCALER_PARAM_LEN 485
/**
 * struct drm_msm_ai_scaler - AI Scaler configuration structure
 * @flags - Setting flags. Currently unused
 * @config - configuration data
 * @src_w - AI Scaler input width
 * @src_h - AI Scaler input height
 * @dst_w - AI Scaler output width
 * @dst_h - AI Scaler output height
 * @param - parameter data
 */
struct drm_msm_ai_scaler {
	__u64 flags;
	__u32 config;
	__u32 src_w;
	__u32 src_h;
	__u32 dst_w;
	__u32 dst_h;
	__u32 param[AIQE_AI_SCALER_PARAM_LEN];
};

#define AIQE_ABC_SUPPORTED
#define AIQE_ABC_PARAM_LEN 44
#define AIQE_ABC_SRC_SEL_DMA1 1
#define AIQE_ABC_SRC_SEL_DMA3 3
/**
 * struct drm_msm_abc - abc feature structure
 * @flags - flags for abc feature
 * @src_sel - pipe selected for abc feature
 * @param - Parameters for abc feature
 */
struct drm_msm_abc {
	__u64 flags;
	__u32 src_sel;
	__u32 param[AIQE_ABC_PARAM_LEN];
};

#endif /* _MSM_DRM_AIQE_H_ */
