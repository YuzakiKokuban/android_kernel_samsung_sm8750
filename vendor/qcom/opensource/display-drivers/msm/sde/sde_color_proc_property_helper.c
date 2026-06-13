// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "sde_color_processing.h"
#include "sde_kms.h"
#include "sde_crtc.h"
#include "sde_color_proc_property_helper.h"

void _sde_cp_crtc_attach_property(
	struct sde_cp_prop_attach *prop_attach)
{

	struct sde_crtc *sde_crtc = to_sde_crtc(prop_attach->crtc);

	drm_object_attach_property(&prop_attach->crtc->base,
				   prop_attach->prop, prop_attach->val);

	INIT_LIST_HEAD(&prop_attach->prop_node->cp_active_list);
	INIT_LIST_HEAD(&prop_attach->prop_node->cp_dirty_list);

	prop_attach->prop_node->property_id = prop_attach->prop->base.id;
	prop_attach->prop_node->prop_flags = prop_attach->prop->flags;
	prop_attach->prop_node->feature = prop_attach->feature;

	if (prop_attach->feature < SDE_CP_CRTC_DSPP_MAX)
		prop_attach->prop_node->is_dspp_feature = true;
	else
		prop_attach->prop_node->is_dspp_feature = false;

	list_add(&prop_attach->prop_node->cp_feature_list,
		 &sde_crtc->cp_feature_list);
}

void _sde_cp_crtc_install_immutable_property(struct drm_crtc *crtc,
	char *name, u32 feature)
{
	struct drm_property *prop;
	struct sde_cp_node *prop_node = NULL;
	struct msm_drm_private *priv;
	struct sde_cp_prop_attach prop_attach;
	uint64_t val = 0;

	if (feature >=  SDE_CP_CRTC_MAX_FEATURES) {
		DRM_ERROR("invalid feature %d max %d\n", feature,
		       SDE_CP_CRTC_MAX_FEATURES);
		return;
	}

	prop_node = kzalloc(sizeof(*prop_node), GFP_KERNEL);
	if (!prop_node)
		return;

	priv = crtc->dev->dev_private;
	prop = priv->cp_property[feature];

	if (!prop) {
		prop = drm_property_create_range(crtc->dev,
				DRM_MODE_PROP_IMMUTABLE, name, 0, 1);
		if (!prop) {
			DRM_ERROR("property create failed: %s\n", name);
			kfree(prop_node);
			return;
		}
		priv->cp_property[feature] = prop;
	}

	INIT_PROP_ATTACH(&prop_attach, crtc, prop, prop_node,
				feature, val);
	_sde_cp_crtc_attach_property(&prop_attach);
}

void _sde_cp_crtc_install_range_property(struct drm_crtc *crtc,
	char *name, u32 feature, uint64_t min, uint64_t max, uint64_t val)
{
	struct drm_property *prop;
	struct sde_cp_node *prop_node = NULL;
	struct msm_drm_private *priv;
	struct sde_cp_prop_attach prop_attach;

	if (feature >=  SDE_CP_CRTC_MAX_FEATURES) {
		DRM_ERROR("invalid feature %d max %d\n", feature,
			  SDE_CP_CRTC_MAX_FEATURES);
		return;
	}

	prop_node = kzalloc(sizeof(*prop_node), GFP_KERNEL);
	if (!prop_node)
		return;

	priv = crtc->dev->dev_private;
	prop = priv->cp_property[feature];

	if (!prop) {
		prop = drm_property_create_range(crtc->dev, 0, name, min, max);
		if (!prop) {
			DRM_ERROR("property create failed: %s\n", name);
			kfree(prop_node);
			return;
		}
		priv->cp_property[feature] = prop;
	}

	INIT_PROP_ATTACH(&prop_attach, crtc, prop, prop_node,
				feature, val);

	_sde_cp_crtc_attach_property(&prop_attach);
}

void _sde_cp_crtc_install_bitmask_property(struct drm_crtc *crtc,
	char *name, u32 feature, bool immutable,
	const struct drm_prop_enum_list *list, u32 enum_sz,
	u64 supported_bits)
{
	struct drm_property *prop;
	struct sde_cp_node *prop_node = NULL;
	struct msm_drm_private *priv;
	struct sde_cp_prop_attach prop_attach;
	int flags = immutable ? DRM_MODE_PROP_IMMUTABLE : 0;
	uint64_t val = 0;

	if (feature >=  SDE_CP_CRTC_MAX_FEATURES) {
		DRM_ERROR("invalid feature %d max %d\n", feature,
			  SDE_CP_CRTC_MAX_FEATURES);
		return;
	}

	prop_node = kzalloc(sizeof(*prop_node), GFP_KERNEL);
	if (!prop_node)
		return;

	priv = crtc->dev->dev_private;
	prop = priv->cp_property[feature];

	if (!prop) {
		prop = drm_property_create_bitmask(crtc->dev, flags, name, list,
				enum_sz, supported_bits);
		if (!prop) {
			DRM_ERROR("property create failed: %s\n", name);
			kfree(prop_node);
			return;
		}
		priv->cp_property[feature] = prop;
	}

	INIT_PROP_ATTACH(&prop_attach, crtc, prop, prop_node, feature, val);
	_sde_cp_crtc_attach_property(&prop_attach);
}

void _sde_cp_crtc_install_blob_property(struct drm_crtc *crtc, char *name,
	u32 feature, u32 blob_sz)
{
	struct drm_property *prop;
	struct sde_cp_node *prop_node = NULL;
	struct msm_drm_private *priv;
	uint64_t val = 0;
	struct sde_cp_prop_attach prop_attach;

	if (feature >=  SDE_CP_CRTC_MAX_FEATURES) {
		DRM_ERROR("invalid feature %d max %d\n", feature,
		       SDE_CP_CRTC_MAX_FEATURES);
		return;
	}

	prop_node = kzalloc(sizeof(*prop_node), GFP_KERNEL);
	if (!prop_node)
		return;

	priv = crtc->dev->dev_private;
	prop = priv->cp_property[feature];

	if (!prop) {
		prop = drm_property_create(crtc->dev,
					   DRM_MODE_PROP_BLOB, name, 0);
		if (!prop) {
			DRM_ERROR("property create failed: %s\n", name);
			kfree(prop_node);
			return;
		}
		priv->cp_property[feature] = prop;
	}

	INIT_PROP_ATTACH(&prop_attach, crtc, prop, prop_node,
				feature, val);
	prop_node->prop_blob_sz = blob_sz;

	_sde_cp_crtc_attach_property(&prop_attach);
}

void _sde_cp_crtc_install_enum_property(struct drm_crtc *crtc,
	u32 feature, const struct drm_prop_enum_list *list, u32 enum_sz,
	char *name)
{
	struct drm_property *prop;
	struct sde_cp_node *prop_node = NULL;
	struct msm_drm_private *priv;
	uint64_t val = 0;
	struct sde_cp_prop_attach prop_attach;

	if (feature >=  SDE_CP_CRTC_MAX_FEATURES) {
		DRM_ERROR("invalid feature %d max %d\n", feature,
		       SDE_CP_CRTC_MAX_FEATURES);
		return;
	}

	prop_node = kzalloc(sizeof(*prop_node), GFP_KERNEL);
	if (!prop_node)
		return;

	priv = crtc->dev->dev_private;
	prop = priv->cp_property[feature];

	if (!prop) {
		prop = drm_property_create_enum(crtc->dev, 0, name,
			list, enum_sz);
		if (!prop) {
			DRM_ERROR("property create failed: %s\n", name);
			kfree(prop_node);
			return;
		}
		priv->cp_property[feature] = prop;
	}

	INIT_PROP_ATTACH(&prop_attach, crtc, prop, prop_node,
				feature, val);

	_sde_cp_crtc_attach_property(&prop_attach);
}

int _sde_cp_create_local_blob(struct drm_crtc *crtc, u32 feature, int len)
{
	int ret = -EINVAL;
	bool found = false;
	struct sde_cp_node *prop_node = NULL;
	struct drm_property_blob *blob_ptr;
	struct sde_crtc *sde_crtc = to_sde_crtc(crtc);

	list_for_each_entry(prop_node, &sde_crtc->cp_feature_list, cp_feature_list) {
		if (prop_node->feature == feature) {
			found = true;
			break;
		}
	}

	if (!found || !(prop_node->prop_flags & DRM_MODE_PROP_RANGE)) {
		DRM_ERROR("local blob create failed prop found %d flags %d\n",
		       found, prop_node->prop_flags);
		return ret;
	}

	blob_ptr = drm_property_create_blob(crtc->dev, len, NULL);
	ret = (IS_ERR_OR_NULL(blob_ptr)) ? PTR_ERR(blob_ptr) : 0;
	if (!ret)
		prop_node->blob_ptr = blob_ptr;

	return ret;
}
