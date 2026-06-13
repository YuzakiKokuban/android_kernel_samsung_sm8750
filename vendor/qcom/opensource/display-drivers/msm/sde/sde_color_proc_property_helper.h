/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _SDE_COLOR_PROC_PROPERTY_HELPER_H
#define _SDE_COLOR_PROC_PROPERTY_HELPER_H

#include <drm/drm_crtc.h>

#define INIT_PROP_ATTACH(p, crtc, prop, node, feature, val) \
	do { \
		(p)->crtc = crtc; \
		(p)->prop = prop; \
		(p)->prop_node = node; \
		(p)->feature = feature; \
		(p)->val = val; \
	} while (0)

/**
 * struct sde_cp_prop_attach - structure to attach a property to drm crtc object
 * @crtc: pointer to crtc
 * @prop: pointer to property to attach
 * @prop_node: pointer to color processing property node
 * @feature: cp crtc feature enum
 * @val: property value
 */
struct sde_cp_prop_attach {
	struct drm_crtc *crtc;
	struct drm_property *prop;
	struct sde_cp_node *prop_node;
	u32 feature;
	uint64_t val;
};

/**
 * _sde_cp_crtc_attach_property - attach property to drm modeset object
 * @prop_attach - pointer to structure that attaches crtc prop to drm obj
 */
void _sde_cp_crtc_attach_property(struct sde_cp_prop_attach *prop_attach);

/**
 * sde_cp_crtc_install_immutable_property - install immutable property
 * @crtc: pointer to crtc
 * @name: property name
 * @feature: cp crtc feature enum
 */
void _sde_cp_crtc_install_immutable_property(struct drm_crtc *crtc,
	char *name, u32 feature);

/**
 * sde_cp_crtc_install_range_property - install range property
 * @crtc: pointer to crtc
 * @name: property name
 * @feature: cp crtc feature enum
 * @min: min value of property
 * @max: max value of property
 * @val: property value
 */
void _sde_cp_crtc_install_range_property(struct drm_crtc *crtc,
	char *name, u32 feature, uint64_t min, uint64_t max, uint64_t val);

/**
 * sde_cp_crtc_install_bitmask_property - install bitmask property
 * @crtc: pointer to crtc
 * @name: property name
 * @feature: cp crtc feature enum
 * @immutable: is immutable property
 * @list: enum lists with propert bitmasks
 * @enum_sz: size of enum list
 * @supported bits: bitmask of all supported enum values
 */
void _sde_cp_crtc_install_bitmask_property(struct drm_crtc *crtc,
	char *name, u32 feature, bool immutable,
	const struct drm_prop_enum_list *list, u32 enum_sz,
	u64 supported_bits);

/**
 * sde_cp_crtc_install_blob_property - install blob property
 * @crtc: pointer to crtc
 * @name: property name
 * @feature: cp crtc feature enum
 * @blob_sz: size of blob to allocate
 */
void _sde_cp_crtc_install_blob_property(struct drm_crtc *crtc, char *name,
	u32 feature, u32 blob_sz);

/**
 * sde_cp_crtc_install_enum_property - install enum property
 * @crtc: pointer to crtc
 * @feature: cp crtc feature enum
 * @list: enum list with property values
 * @enum_sz: size of enum property
 * @name: property name
 */
void _sde_cp_crtc_install_enum_property(struct drm_crtc *crtc,
	u32 feature, const struct drm_prop_enum_list *list, u32 enum_sz,
	char *name);

/**
 * sde_cp_crtc_create_local_blob - creates local blob for use as backing for a range property
 * @crtc: pointer to crtc
 * @feature: cp crtc feature enum
 * @len: length to allocate for blob data
 */
int _sde_cp_create_local_blob(struct drm_crtc *crtc, u32 feature, int len);

#endif /* _SDE_COLOR_PROC_PROPERTY_HELPER_H */
