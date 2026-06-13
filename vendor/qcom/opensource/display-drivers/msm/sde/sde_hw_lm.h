/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2015-2019, 2021, The Linux Foundation. All rights reserved.
 */

#ifndef _SDE_HW_LM_H
#define _SDE_HW_LM_H

#include "sde_hw_mdss.h"
#include "sde_hw_util.h"

struct sde_hw_mixer;

struct sde_hw_mixer_cfg {
	u32 out_width;
	u32 out_height;
	bool right_mixer;
	int flags;
};

struct sde_hw_color3_cfg {
	u8 keep_fg[SDE_STAGE_MAX];
};

/**
 *
 * struct sde_hw_lm_ops : Interface to the mixer Hw driver functions
 *  Assumption is these functions will be called after clocks are enabled
 */
struct sde_hw_lm_ops {
	/*
	 * Sets up mixer output width and height
	 * and border color if enabled
	 */
	void (*setup_mixer_out)(struct sde_hw_mixer *ctx,
		struct sde_hw_mixer_cfg *cfg);

	/*
	 * Alpha blending configuration
	 * for the specified stage
	 */
	void (*setup_blend_config)(struct sde_hw_mixer *ctx, u32 stage,
		u32 fg_alpha, u32 bg_alpha, u32 blend_op);

	/*
	 * Alpha color component selection from either fg or bg
	 */
	void (*setup_alpha_out)(struct sde_hw_mixer *ctx, u32 mixer_op);

	/**
	 * setup_border_color : enable/disable border color
	 */
	void (*setup_border_color)(struct sde_hw_mixer *ctx,
		struct sde_mdss_color *color,
		u8 border_en);
	/**
	 * setup_gc : enable/disable gamma correction feature
	 */
	void (*setup_gc)(struct sde_hw_mixer *mixer,
			void *cfg);

	/**
	 * setup_dim_layer: configure dim layer settings
	 * @ctx: Pointer to layer mixer context
	 * @dim_layer: dim layer configs
	 */
	void (*setup_dim_layer)(struct sde_hw_mixer *ctx,
			struct sde_hw_dim_layer *dim_layer);

	/**
	 * clear_dim_layer: clear dim layer settings
	 * @ctx: Pointer to layer mixer context
	 */
	void (*clear_dim_layer)(struct sde_hw_mixer *ctx);

	/* setup_misr: enables/disables MISR in HW register */
	void (*setup_misr)(struct sde_hw_mixer *ctx,
			bool enable, u32 frame_count);

	/* collect_misr: reads and stores MISR data from HW register */
	int (*collect_misr)(struct sde_hw_mixer *ctx, bool nonblock,
			u32 *misr_value);

	/* setup_noise_layer: enables/disables noise layer */
	int (*setup_noise_layer)(struct sde_hw_mixer *ctx,
		struct sde_hw_noise_layer_cfg *cfg);

	/**
	 * Configure layer mixer to pipe configuration
	 * @ctx: Pointer to layer mixer context
	 * @lm:  layer mixer enumeration
	 * @cfg: blend stage configuration
	 * @disable_border: if true disable border, else enable border out
	 */
	int (*setup_blendstage)(struct sde_hw_mixer *ctx,
		enum sde_lm lm, struct sde_hw_stage_cfg *cfg,
		bool disable_border);

	/**
	 * Clear layer mixer to pipe configuration
	 * @ctx: Pointer to layer mixer context
	 */
	int (*clear_all_blendstages)(struct sde_hw_mixer *ctx);

	/**
	 * Get all the sspp staged on a layer mixer
	 * @ctx: Pointer to layer mixer context
	 * @lm: layer mixer enumeration
	 * @info: structure to populate connected sspp index info
	 */
	int (*get_staged_sspp)(struct sde_hw_mixer *ctx,
		enum sde_lm lm, struct sde_sspp_index_info *info);

};

struct sde_hw_mixer {
	struct sde_hw_blk_reg_map hw;

	/* lm */
	enum sde_lm  idx;
	const struct sde_lm_cfg   *cap;
	const struct sde_mdp_cfg  *mdp;
	const struct sde_ctl_cfg  *ctl;

	/* ops */
	struct sde_hw_lm_ops ops;

	/* store mixer info specific to display */
	struct sde_hw_mixer_cfg cfg;
};

/**
 * to_sde_hw_mixer - convert base hw object to sde_hw_mixer container
 * @hw: Pointer to hardware block register map object
 * return: Pointer to hardware block container
 */
static inline struct sde_hw_mixer *to_sde_hw_mixer(struct sde_hw_blk_reg_map *hw)
{
	return container_of(hw, struct sde_hw_mixer, hw);
}

/**
 * sde_hw_lm_init(): Initializes the mixer hw driver object.
 * should be called once before accessing every mixer.
 * @idx:  mixer index for which driver object is required
 * @addr: mapped register io address of MDP
 * @m :   pointer to mdss catalog data
 */
struct sde_hw_blk_reg_map *sde_hw_lm_init(enum sde_lm idx,
		void __iomem *addr,
		struct sde_mdss_cfg *m);

/**
 * sde_hw_lm_destroy(): Destroys layer mixer driver context
 * @hw: Pointer to hardware block register map object
 */
void sde_hw_lm_destroy(struct sde_hw_blk_reg_map *hw);

#endif /*_SDE_HW_LM_H */
