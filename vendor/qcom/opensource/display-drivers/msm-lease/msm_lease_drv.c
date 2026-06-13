// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2016-2021, The Linux Foundation. All rights reserved.
 * Copyright (C) 2013 Red Hat
 * Author: Rob Clark <robdclark@gmail.com>
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright © 2017 Keith Packard <keithp@keithp.com>
 *
 *
 * Copyright (c) 2016 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission. The copyright holders make no representations
 * about the suitability of this software for any purpose. It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */
/*
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
 *
 * Author Rickard E. (Rik) Faith <faith@valinux.com>
 * Author Gareth Hughes <gareth@valinux.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * 32-bit ioctl compatibility routines for the DRM.
 *
 * \author Paul Mackerras <paulus@samba.org>
 *
 * Copyright (C) Paul Mackerras 2005.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <linux/of_address.h>
#include <linux/kthread.h>
#include <linux/kobject.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/of_graph.h>
#include <linux/of_device.h>
#include <linux/debugfs.h>
#include <linux/component.h>
#include <drm/drm_of.h>
#include <drm/drm_drv.h>
#include <drm/drm_connector.h>
#include <drm/drm_atomic_uapi.h>
#include <drm/drm_atomic.h>
#include <drm/drm_encoder.h>
#include <drm/drm_auth.h>
#include <drm/drm_ioctl.h>
#include <msm_drv.h>

#define MAX_LEASE_OBJECT_COUNT 64
#define MAX_HW_INSTANCES 2
static DEFINE_MUTEX(g_lease_mutex);
static LIST_HEAD(g_lease_list);
static int (*g_master_open)(struct drm_device *, struct drm_file *);
static void (*g_master_postclose)(struct drm_device *, struct drm_file *);
static const struct file_operations *g_master_ddev_fops;

struct msm_lease {
	struct device *dev;
	struct drm_device *drm_dev;
	struct drm_minor *minor;
	struct drm_device *minor_dev;
	struct drm_master *master;
	struct list_head head;
	struct notifier_block notifier;
	u32 object_ids[MAX_LEASE_OBJECT_COUNT];
	int obj_cnt;
	const char *dev_name;
};

struct msm_lease_master {
	struct drm_master *master;
	struct kref ref;
	bool name_overridden;
	char name[32];
};

struct msm_drm_vesion {
	int version_major;
	int version_minor;
	int version_patchlevel;
	u32 name_len;
	u32 name;
	u32 date_len;
	u32 date;
	u32 desc_len;
	u32 desc;
};

static struct msm_lease_master g_lease_master;
static struct drm_driver msm_lease_driver;

static inline struct msm_lease *_find_lease_from_minor(struct drm_minor *minor)
{
	struct msm_lease *lease;

	list_for_each_entry(lease, &g_lease_list, head) {
		if (lease->minor == minor)
			return lease;
	}

	return NULL;
}

static inline struct msm_lease *_find_lease_from_node(struct device_node *node)
{
	struct msm_lease *lease;

	list_for_each_entry(lease, &g_lease_list, head) {
		if (lease->dev->of_node == node)
			return lease;
	}

	return NULL;
}

static inline bool _find_obj_id(int id, u32 *object_ids, int object_count)
{
	int i;

	for (i = 0; i < object_count; i++) {
		if (object_ids[i] == id)
			return true;
	}

	return false;
}

static inline bool _obj_is_leased(int id, u32 *object_ids, int object_count)
{
	struct msm_lease *lease;

	list_for_each_entry(lease, &g_lease_list, head) {
		if (_find_obj_id(id, lease->object_ids, lease->obj_cnt))
			return true;
	}

	return _find_obj_id(id, object_ids, object_count);
}

static struct drm_master *msm_lease_master_create(struct drm_device *dev)
{
	struct drm_master *master;

	master = kzalloc(sizeof(*master), GFP_KERNEL);
	if (!master)
		return NULL;

	kref_init(&master->refcount);
	idr_init(&master->magic_map);
	master->dev = dev;

	INIT_LIST_HEAD(&master->lessees);
	INIT_LIST_HEAD(&master->lessee_list);
	idr_init(&master->leases);
	idr_init(&master->lessee_idr);

	return master;
}

static struct drm_master *msm_lease_get_dev_master(struct msm_lease *lease)
{
	struct drm_device *dev = lease->drm_dev;

	if (!g_lease_master.master) {
		g_lease_master.master = msm_lease_master_create(dev);
		if (!g_lease_master.master) {
			DRM_ERROR("failed to create dev master\n");
			return NULL;
		}

		if (dev->master) {
			DRM_WARN("card0 master already opened\n");
			drm_master_put(&dev->master);
		}
		dev->master = g_lease_master.master;
		kref_init(&g_lease_master.ref);
	} else {
		kref_get(&g_lease_master.ref);
	}
	return g_lease_master.master;
}

static void msm_lease_destroy_dev_master(struct kref *kref)
{
	struct drm_device *dev;
	struct msm_lease_master *lease_master;

	lease_master = container_of(kref, struct msm_lease_master, ref);
	if (lease_master->master) {
		dev = lease_master->master->dev;
		drm_master_put(&dev->master);
		kfree(lease_master->master);
		lease_master->master = NULL;
	} else {
		DRM_ERROR("global master doesn't exist\n");
	}
}

static void msm_lease_put_dev_master(struct msm_lease *lease)
{
	kref_put(&g_lease_master.ref, msm_lease_destroy_dev_master);
}

static const char *msm_lease_get_dev_name(struct drm_file *file)
{
	struct msm_lease *lease;
	const char *dev_name;

	mutex_lock(&g_lease_mutex);

	lease = _find_lease_from_minor(file->minor);
	if (!lease || !lease->dev_name) {
		if (file->minor->index < MAX_HW_INSTANCES && g_lease_master.name_overridden)
			dev_name = g_lease_master.name;
		else
			dev_name = file->minor->dev->driver->name;
	} else {
		dev_name = lease->dev_name;
	}
	mutex_unlock(&g_lease_mutex);

	return dev_name;
}

static int msm_lease_copy_field(const char *src, size_t *len, char __user *dst)
{
	u32 name_len;

	name_len = *len;

	*len = src ? strlen(src) : *len;

	if (*len < name_len)
		name_len = *len;

	if (dst && name_len)
		if (copy_to_user(dst, src, name_len))
			return -EFAULT;

	return 0;
}

static int msm_lease_get_version(struct drm_file *file, struct drm_version *version)
{
	struct drm_device *dev = file->minor->dev;
	int rc;

	version->version_major = dev->driver->major;
	version->version_minor = dev->driver->minor;
	version->version_patchlevel = dev->driver->patchlevel;

	rc = msm_lease_copy_field(msm_lease_get_dev_name(file), &version->name_len, version->name);
	if (rc)
		return rc;

	rc = msm_lease_copy_field(dev->driver->date, &version->date_len, version->date);
	if (rc)
		return rc;

	rc = msm_lease_copy_field(dev->driver->desc, &version->desc_len, version->desc);

	return rc;
}

static int msm_lease_open(struct drm_device *dev, struct drm_file *file)
{
	struct msm_lease *lease;
	struct drm_master *lessee;
	struct drm_master *dev_master;
	struct idr leases;
	int id, i, rc;

	if (!dev->registered)
		return -ENOENT;

	if ((!dev->mode_config.poll_enabled || drm_is_render_client(file)) && g_master_open)
		return g_master_open(dev, file);

	mutex_lock(&g_lease_mutex);

	lease = _find_lease_from_minor(file->minor);
	if (!lease) {
		rc = -ENODEV;
		goto end;
	}

	if (g_master_open) {
		rc = g_master_open(dev, file);
		if (rc)
			goto end;
	}

	mutex_lock(&dev->master_mutex);

	if (!lease->master) {
		/* get device master */
		dev_master = msm_lease_get_dev_master(lease);
		if (!dev_master) {
			rc = -EBUSY;
			goto out;
		}

		/* create local idr */
		idr_init(&leases);
		for (i = 0; i < lease->obj_cnt; i++) {
			id = idr_alloc(&leases, lease, lease->object_ids[i],
				       lease->object_ids[i] + 1, GFP_KERNEL);
			if (id < 0) {
				msm_lease_put_dev_master(lease);
				DRM_ERROR("create idr failed\n");
				rc = id;
				goto out;
			}
		}

		/* create lessee master */
		lessee = msm_lease_master_create(dev);
		if (!lessee) {
			msm_lease_put_dev_master(lease);
			DRM_ERROR("drm_master_create failed\n");
			idr_destroy(&leases);
			rc = -ENOMEM;
			goto out;
		}

		/* create lessee id */
		mutex_lock(&dev->mode_config.idr_mutex);
		id = idr_alloc(&dev_master->lessee_idr, lessee, 1, 0, GFP_KERNEL);
		if (id < 0) {
			mutex_unlock(&dev->mode_config.idr_mutex);
			msm_lease_put_dev_master(lease);
			DRM_ERROR("idr_alloc failed\n");
			idr_destroy(&leases);
			drm_master_put(&lessee);
			rc = id;
			goto out;
		}

		/* init lessee */
		lessee->lessee_id = id;
		lessee->lessor = drm_master_get(dev_master);
		list_add_tail(&lessee->lessee_list, &dev_master->lessees);
		lessee->leases = leases;
		mutex_unlock(&dev->mode_config.idr_mutex);

		/* set file as master */
		file->master = lessee;
		file->is_master = 1;
		file->authenticated = 1;
		lease->master = drm_master_get(lessee);
	} else {
		file->master = drm_master_get(lease->master);
	}
out:
	mutex_unlock(&dev->master_mutex);
end:
	mutex_unlock(&g_lease_mutex);
	return rc;
}

static int msm_lease_lastclose(struct msm_lease *lease)
{
	struct drm_modeset_acquire_ctx ctx;
	struct drm_atomic_state *state;
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	struct drm_connector *connector;
	struct drm_connector_state *conn_state;
	struct drm_connector_list_iter conn_iter;
	struct drm_plane *plane;
	struct drm_plane_state *plane_state;
	int ret;

	state = drm_atomic_state_alloc(lease->drm_dev);
	if (!state)
		return -ENOMEM;

	drm_modeset_acquire_init(&ctx, 0);
	state->acquire_ctx = &ctx;
retry:
	drm_for_each_crtc(crtc, lease->drm_dev) {
		if (!_find_obj_id(crtc->base.id, lease->object_ids, lease->obj_cnt))
			continue;

		crtc_state = drm_atomic_get_crtc_state(state, crtc);
		if (IS_ERR(crtc_state)) {
			ret = PTR_ERR(crtc_state);
			goto end;
		}

		/* disable connectors */
		drm_connector_list_iter_begin(lease->drm_dev, &conn_iter);
		drm_for_each_connector_iter(connector, &conn_iter) {
			if (!(drm_connector_mask(connector) &
					crtc_state->connector_mask))
				continue;

			conn_state = drm_atomic_get_connector_state(state, connector);
			if (IS_ERR(conn_state)) {
				ret = PTR_ERR(conn_state);
				goto end;
			}

			ret = drm_atomic_set_crtc_for_connector(conn_state, NULL);
			if (ret)
				goto end;
		}
		drm_connector_list_iter_end(&conn_iter);

		/* disable mode */
		ret = drm_atomic_set_mode_for_crtc(crtc_state, NULL);
		if (ret)
			goto end;

		/* disable planes */
		drm_for_each_plane_mask(plane, lease->drm_dev, crtc_state->plane_mask) {
			plane_state = drm_atomic_get_plane_state(state, plane);
			if (IS_ERR(plane_state)) {
				ret = PTR_ERR(plane_state);
				goto end;
			}

			ret = drm_atomic_set_crtc_for_plane(plane_state, NULL);
			if (ret)
				goto end;

			drm_atomic_set_fb_for_plane(plane_state, NULL);
		}

		/* disable crtc */
		crtc_state->active = false;
	}

	ret = drm_atomic_commit(state);
end:
	if (ret == -EDEADLK) {
		drm_atomic_state_clear(state);
		drm_modeset_backoff(&ctx);
		goto retry;
	}

	drm_atomic_state_put(state);
	drm_modeset_drop_locks(&ctx);
	drm_modeset_acquire_fini(&ctx);

	return ret;
}

static void msm_lease_postclose(struct drm_device *dev, struct drm_file *file)
{
	struct msm_lease *lease;

	if (g_master_postclose)
		g_master_postclose(dev, file);

	mutex_lock(&g_lease_mutex);

	lease = _find_lease_from_minor(file->minor);
	if (!lease)
		goto out;

	if (drm_is_current_master(file))
		msm_lease_lastclose(lease);

	mutex_lock(&dev->master_mutex);
	if (drm_is_current_master(file)) {
		drm_master_put(&lease->master);
		msm_lease_put_dev_master(lease);
	}
	if (file->master)
		drm_master_put(&file->master);
	mutex_unlock(&dev->master_mutex);

out:
	mutex_unlock(&g_lease_mutex);
}

static long msm_lease_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	if (cmd == DRM_IOCTL_VERSION) {
		struct drm_version v;

		if (copy_from_user(&v, (void __user *)arg, sizeof(v)))
			return -EFAULT;

		if (msm_lease_get_version(filp->private_data, &v))
			return -EFAULT;

		if (copy_to_user((void __user *)arg, &v, sizeof(v)))
			return -EFAULT;

		return 0;
	} else if (cmd == DRM_IOCTL_DROP_MASTER) {
		return -EINVAL;
	}

	return g_master_ddev_fops->unlocked_ioctl(filp, cmd, arg);
}

static long msm_lease_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	if (DRM_IOCTL_NR(cmd) == DRM_IOCTL_NR(DRM_IOCTL_VERSION)) {
		struct drm_version v;
		struct msm_drm_vesion v32;

		if (copy_from_user(&v32, (void __user *)arg, sizeof(v32)))
			return -EFAULT;

		v.name_len = v32.name_len;
		v.name = compat_ptr(v32.name);
		v.date_len = v32.date_len;
		v.date = compat_ptr(v32.date);
		v.desc_len = v32.desc_len;
		v.desc = compat_ptr(v32.desc);

		if (msm_lease_get_version(filp->private_data, &v))
			return -EFAULT;

		v32.version_major = v.version_major;
		v32.version_minor = v.version_minor;
		v32.version_patchlevel = v.version_patchlevel;
		v32.name_len = v.name_len;
		v32.date_len = v.date_len;
		v32.desc_len = v.desc_len;

		if (copy_to_user((void __user *)arg, &v32, sizeof(v32)))
			return -EFAULT;

		return 0;
	} else if (DRM_IOCTL_NR(cmd) == DRM_IOCTL_NR(DRM_IOCTL_DROP_MASTER)) {
		return -EINVAL;
	}

	return g_master_ddev_fops->compat_ioctl(filp, cmd, arg);
}

static int msm_lease_add_connector(struct drm_device *dev, const char *name, u32 *object_ids,
				   int *object_count)
{
	struct drm_connector *connector;
	struct drm_encoder *encoder;
	struct drm_crtc *crtc;
	struct drm_connector_list_iter conn_iter;
	int conn_id = -1, crtc_id = -1;
	int rc = 0;

	if (*object_count >= MAX_LEASE_OBJECT_COUNT - 1) {
		DRM_ERROR("too many objects added %d\n", *object_count);
		return -EINVAL;
	}

	mutex_lock(&dev->mode_config.mutex);

	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(connector, &conn_iter) {
		if (!strcmp(connector->name, name)) {
			conn_id = connector->base.id;
			break;
		}
	}
	drm_connector_list_iter_end(&conn_iter);

	if (conn_id < 0) {
		DRM_ERROR("failed to find connector %s\n", name);
		rc = -ENOENT;
		goto out;
	}

	if (_obj_is_leased(conn_id, object_ids, *object_count)) {
		DRM_ERROR("connector %s is already leased\n", name);
		rc = -EBUSY;
		goto out;
	}

	drm_connector_for_each_possible_encoder(connector, encoder)
		break;

	if (!encoder) {
		DRM_ERROR("failed to find encoder for %s\n", name);
		rc = -ENOENT;
		goto out;
	}

	drm_for_each_crtc(crtc, dev) {
		if (!(encoder->possible_crtcs & drm_crtc_mask(crtc)))
			continue;

		if (_obj_is_leased(crtc->base.id, object_ids, *object_count))
			continue;

		crtc_id = crtc->base.id;
		break;
	}

	if (crtc_id < 0) {
		DRM_ERROR("failed to find crtc for %s\n", name);
		rc = -ENOENT;
		goto out;
	}

	/* unique connector-crtc mapping is required by cont splash */
	encoder->possible_crtcs = drm_crtc_mask(crtc);

	object_ids[(*object_count)++] = conn_id;
	object_ids[(*object_count)++] = crtc_id;

out:
	mutex_unlock(&dev->mode_config.mutex);

	return rc;
}

static int msm_lease_add_plane(struct drm_device *dev, const char *name, u32 *object_ids,
			       int *object_count)
{
	struct drm_plane *plane, *added_plane;
	int plane_id = -1;

	if (*object_count >= MAX_LEASE_OBJECT_COUNT) {
		DRM_ERROR("too many objects %d\n", *object_count);
		return -EINVAL;
	}
	mutex_lock(&dev->mode_config.mutex);

	drm_for_each_plane(plane, dev) {
		if (!strcmp(plane->name, name)) {
			plane_id = plane->base.id;
			added_plane = plane;
			break;
		}
	}
	mutex_unlock(&dev->mode_config.mutex);

	if (_obj_is_leased(plane_id, object_ids, *object_count)) {
		DRM_ERROR("plane %s is already leased\n", name);
		return -EBUSY;
	}

	if (plane_id < 0) {
		DRM_ERROR("failed to find plane for %s\n", name);
		return -ENOENT;
	}

	object_ids[(*object_count)++] = plane_id;

	return 0;
}

static void msm_lease_fixup_crtc_primary(struct drm_device *dev, u32 *object_ids, int object_count)
{
	struct drm_mode_object *obj;
	struct drm_plane *planes[MAX_LEASE_OBJECT_COUNT];
	struct drm_crtc *crtcs[MAX_LEASE_OBJECT_COUNT];
	struct drm_plane *plane;
	struct drm_crtc *crtc;
	int i, plane_count = 0, crtc_count = 0;

	if (!object_count)
		return;

	/* get all the leased crtcs and planes */
	for (i = 0; i < object_count; i++) {
		obj = drm_mode_object_find(dev, NULL, object_ids[i], DRM_MODE_OBJECT_ANY);
		if (!obj)
			continue;

		if (obj->type == DRM_MODE_OBJECT_PLANE)
			planes[plane_count++] = obj_to_plane(obj);
		else if (obj->type == DRM_MODE_OBJECT_CRTC)
			crtcs[crtc_count++] = obj_to_crtc(obj);
	}

	/* reset previous primary planes */
	for (i = 0; i < plane_count; i++) {
		if (planes[i]->type == DRM_PLANE_TYPE_PRIMARY) {
			drm_for_each_crtc(crtc, dev) {
				if (crtc->primary == planes[i]) {
					crtc->primary = NULL;
					break;
				}
			}
			planes[i]->type = DRM_PLANE_TYPE_OVERLAY;
		}
	}

	/* setup new primary planes */
	for (i = 0; i < crtc_count && i < plane_count; i++) {
		if (crtcs[i]->primary)
			crtcs[i]->primary->type = DRM_PLANE_TYPE_OVERLAY;
		crtcs[i]->primary = planes[i];
		planes[i]->type = DRM_PLANE_TYPE_PRIMARY;
	}

	/* assign primary planes for reset crtcs */
	drm_for_each_crtc(crtc, dev) {
		if (crtc->primary)
			continue;

		drm_for_each_plane(plane, dev) {
			if (plane->type == DRM_PLANE_TYPE_OVERLAY) {
				crtc->primary = plane;
				plane->type = DRM_PLANE_TYPE_PRIMARY;
				break;
			}
		}
	}
}

static int msm_lease_parse_objs(struct drm_device *dev, struct msm_lease *lease_drv,
				u32 *object_ids, int *object_count)
{
	const char *name;
	int count, rc, i;
	struct device_node *of_node;

	of_node = lease_drv->dev->of_node;
	if (!of_node)
		return -EINVAL;

	count = of_property_count_strings(of_node, "qcom,lease-planes");
	if (count <= 0)
		return 0;

	for (i = 0; i < count; i++) {
		of_property_read_string_index(of_node, "qcom,lease-planes", i, &name);
		rc = msm_lease_add_plane(dev, name, object_ids, object_count);
		if (rc)
			return rc;
	}

	count = of_property_count_strings(of_node, "qcom,lease-connectors");

	if (count <= 0) {
		*object_count = 0;
		return 0;
	}

	if (count > *object_count) {
		DRM_ERROR("connectors are more than planes\n");
		return -EINVAL;
	}

	for (i = 0; i < count; i++) {
		of_property_read_string_index(of_node, "qcom,lease-connectors", i, &name);
		rc = msm_lease_add_connector(dev, name, object_ids, object_count);
		if (rc)
			return rc;
	}

	return 0;
}

static void msm_lease_parse_remain_objs(void)
{
	struct device_node *of_node;
	struct msm_lease *lease, *target = NULL;
	struct drm_device *dev;
	struct drm_plane *plane;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	struct drm_connector_list_iter conn_iter;
	struct drm_crtc *crtc;
	u32 object_ids[MAX_LEASE_OBJECT_COUNT];
	int object_count = 0;
	const char *name;
	int count, rc, i;
	int crtc_count;
	bool found;

	list_for_each_entry(lease, &g_lease_list, head) {
		if (!lease->minor)
			continue;

		if (!lease->obj_cnt)
			target = lease;
	}

	if (!target)
		return;

	of_node = target->dev->of_node;
	dev = target->drm_dev;

	count = of_property_count_strings(of_node, "qcom,lease-planes");
	if (count > 0) {
		for (i = 0; i < count; i++) {
			of_property_read_string_index(of_node, "qcom,lease-planes", i, &name);
			rc = msm_lease_add_plane(dev, name, object_ids, &object_count);
			if (rc)
				break;
		}
	} else {
		drm_for_each_plane(plane, dev) {
			if (object_count >= MAX_LEASE_OBJECT_COUNT)
				break;

			if (_obj_is_leased(plane->base.id, object_ids, object_count))
				continue;

			object_ids[object_count++] = plane->base.id;
		}
	}

	count = of_property_count_strings(of_node, "qcom,lease-connectors");
	if (count > 0) {
		for (i = 0; i < count; i++) {
			of_property_read_string_index(of_node, "qcom,lease-connectors", i, &name);
			rc = msm_lease_add_connector(dev, name, object_ids, &object_count);
			if (rc)
				break;
		}
	} else {
		/*
		 * we'll add dedicated crtc first to make sure they
		 * have valid primary plane to support legacy drm call.
		 */
		drm_for_each_crtc(crtc, dev) {
			if (object_count >= MAX_LEASE_OBJECT_COUNT)
				break;

			if (_obj_is_leased(crtc->base.id, object_ids, object_count))
				continue;

			found = false;
			drm_for_each_encoder(encoder, dev) {
				if (encoder->encoder_type == DRM_MODE_ENCODER_VIRTUAL)
					continue;
				if ((encoder->possible_crtcs & drm_crtc_mask(crtc)) &&
				    encoder->possible_crtcs != drm_crtc_mask(crtc)) {
					found = true;
					break;
				}
			}

			/*
			 * if there are encoders not dedicated to crtc,
			 * we leave it to the next round of leasing
			 */
			if (found)
				continue;

			drm_for_each_plane(plane, dev) {
				if (plane == crtc->primary) {
					found = true;
					break;
				}
			}

			/*
			 * if crtc's primary plane is not available,
			 * do not lease it.
			 */
			if (!found)
				continue;

			object_ids[object_count++] = crtc->base.id;
		}

		drm_for_each_crtc(crtc, dev) {
			if (object_count >= MAX_LEASE_OBJECT_COUNT)
				break;

			if (_obj_is_leased(crtc->base.id, object_ids, object_count))
				continue;

			found = false;
			drm_for_each_plane(plane, dev) {
				if (plane == crtc->primary) {
					found = true;
					break;
				}
			}

			/*
			 * if crtc's primary plane is not available,
			 * do not lease it.
			 */
			if (!found)
				continue;

			object_ids[object_count++] = crtc->base.id;
		}

		crtc_count = object_count;

		drm_connector_list_iter_begin(dev, &conn_iter);
		drm_for_each_connector_iter(connector, &conn_iter) {
			if (object_count >= MAX_LEASE_OBJECT_COUNT)
				break;

			if (_obj_is_leased(connector->base.id, object_ids, object_count))
				continue;

			drm_connector_for_each_possible_encoder(connector, encoder)
				break;

			if (!encoder)
				continue;

			found = false;
			for (i = 0; i < crtc_count; i++) {
				crtc = drm_crtc_find(dev, NULL, object_ids[i]);
				if (!crtc)
					continue;

				if (!(encoder->possible_crtcs & drm_crtc_mask(crtc)))
					continue;

				found = true;
				break;
			}

			/*
			 * if there is no valid crtc for connector,
			 * do not lease it.
			 */
			if (!found)
				continue;

			object_ids[object_count++] = connector->base.id;
		}
		drm_connector_list_iter_end(&conn_iter);
	}

	target->obj_cnt = object_count;
	memcpy(target->object_ids, object_ids, sizeof(u32) * object_count);
	msm_lease_fixup_crtc_primary(dev, object_ids, object_count);
}

static int msm_lease_parse_misc(struct msm_lease *lease_drv)
{
	of_property_read_string(lease_drv->dev->of_node, "qcom,dev-name", &lease_drv->dev_name);

	return 0;
}

static int msm_lease_release(struct inode *inode, struct file *filp)
{
	return g_master_ddev_fops->release(inode, filp);
}

static int msm_lease_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return g_master_ddev_fops->mmap(filp, vma);
}

static const struct file_operations msm_lease_fops = {
	.owner              = THIS_MODULE,
	.open               = drm_open,
	.release            = msm_lease_release,
	.unlocked_ioctl     = msm_lease_ioctl,
	.compat_ioctl       = msm_lease_compat_ioctl,
	.poll               = drm_poll,
	.read               = drm_read,
	.llseek             = no_llseek,
	.mmap               = msm_lease_mmap,
};

static int msm_lease_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
	struct msm_lease *lease_drv;
	struct drm_device *master_ddev;
	struct drm_driver *ddev_drv;
	u32 object_ids[MAX_LEASE_OBJECT_COUNT] = {0};
	int object_count = 0;
	int ret;

	if (action != MSM_COMP_OBJECT_CREATED)
		return 0;

	lease_drv = container_of(nb, struct msm_lease, notifier);
	master_ddev = lease_drv->drm_dev;
	ddev_drv = (struct drm_driver *)master_ddev->driver;

	/* parse misc options */
	msm_lease_parse_misc(lease_drv);

	/* parse lease resources */
	ret = msm_lease_parse_objs(master_ddev, lease_drv, object_ids, &object_count);
	if (ret)
		goto fail;

	/* create temporary device */
	lease_drv->minor_dev = drm_dev_alloc(&msm_lease_driver, master_ddev->dev);
	if (!lease_drv->minor_dev) {
		dev_err(lease_drv->dev, "failed to allocate drm_device\n");
		goto fail;
	}

	/* update ids list */
	lease_drv->minor = lease_drv->minor_dev->primary;
	lease_drv->obj_cnt = object_count;
	if (object_count > 0)
		memcpy(lease_drv->object_ids, object_ids, sizeof(u32) * object_count);

	/* fixup crtcs' primary planes */
	msm_lease_fixup_crtc_primary(master_ddev, object_ids, object_count);

	/* hook open/close function */
	if (!g_master_open && !g_master_postclose) {
		g_master_open = ddev_drv->open;
		g_master_postclose = ddev_drv->postclose;
		ddev_drv->open = msm_lease_open;
		ddev_drv->postclose = msm_lease_postclose;
	}

	/* hook ioctl function if dev_name is defined */
	if (!g_master_ddev_fops && lease_drv->dev_name) {
		g_master_ddev_fops = ddev_drv->fops;
		ddev_drv->fops = &msm_lease_fops;
	}

	/* if lease device has the same name, hide the original name */
	if (lease_drv->dev_name &&
	    !strcmp(lease_drv->dev_name, ddev_drv->name)) {
		g_lease_master.name_overridden = true;
		snprintf(g_lease_master.name, sizeof(g_lease_master.name),
			 "%s_orig", ddev_drv->name);
	}
	/* redirect primary minor to master dev */
	lease_drv->minor_dev->primary->dev = master_ddev;
	lease_drv->minor_dev->primary->type = -1;

	/* register primary minor */
	ret = drm_dev_register(lease_drv->minor_dev, 0);
	if (ret) {
		dev_err(lease_drv->dev, "failed to register drm device\n");
		lease_drv->minor_dev->primary->dev = lease_drv->minor_dev;
		drm_dev_put(lease_drv->minor_dev);
		goto fail;
	}

	/* check if there are remaining objs */
	msm_lease_parse_remain_objs();
fail:
	return ret;
}

static int msm_lease_bind(struct device *dev, struct device *master, void *data)
{
	int rc = 0;
	struct msm_lease *lease_drv;
	struct drm_device *drm;
	struct platform_device *pdev = to_platform_device(dev);
	struct msm_drm_private *priv;

	if (!dev || !pdev || !master) {
		pr_err("invalid param(s), dev %pK, pdev %pK, master %pK\n",
		       dev, pdev, master);
		rc = -EINVAL;
		goto end;
	}

	drm = dev_get_drvdata(master);
	priv = drm->dev_private;
	lease_drv = platform_get_drvdata(pdev);
	if (!drm || !lease_drv || !priv) {
		pr_err("invalid param(s), drm %pK, lease_drv %pK, priv %pK\n",
		       drm, lease_drv, priv);
		rc = -EINVAL;
		goto end;
	}

	lease_drv->drm_dev = drm;
	lease_drv->notifier.notifier_call = msm_lease_notifier;

	rc = msm_drm_register_component(drm, &lease_drv->notifier);
	if (rc) {
		pr_err("failed to register component notifier\n");
		goto end;
	}

end:
	return rc;
}

static void msm_lease_unbind(struct device *dev, struct device *master, void *data)
{
	struct msm_lease *lease_drv;
	struct platform_device *pdev = to_platform_device(dev);

	if (!dev || !pdev) {
		pr_err("invalid param\n");
		return;
	}

	lease_drv = platform_get_drvdata(pdev);
	if (!lease_drv) {
		pr_err("invalid param\n");
		return;
	}

	msm_drm_unregister_component(lease_drv->drm_dev, &lease_drv->notifier);

	/* unregister lease driver */
	lease_drv->minor_dev->primary->dev = lease_drv->minor_dev;
	drm_dev_unregister(lease_drv->minor_dev);
	drm_dev_put(lease_drv->minor_dev);

	mutex_lock(&g_lease_mutex);
	list_del_init(&lease_drv->head);
	mutex_unlock(&g_lease_mutex);
}

static const struct component_ops msm_lease_comp_ops = {
	.bind = msm_lease_bind,
	.unbind = msm_lease_unbind,
};

static int msm_lease_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct msm_lease *lease_drv;
	int ret;

	lease_drv = devm_kzalloc(dev, sizeof(*lease_drv), GFP_KERNEL);
	if (!lease_drv)
		return -ENOMEM;

	platform_set_drvdata(pdev, lease_drv);
	lease_drv->dev = dev;

	mutex_lock(&g_lease_mutex);
	list_add_tail(&lease_drv->head, &g_lease_list);
	mutex_unlock(&g_lease_mutex);

	ret = component_add(&pdev->dev, &msm_lease_comp_ops);
	if (ret) {
		pr_err("component add failed, rc=%d\n", ret);
		return ret;
	}

	return 0;
}

static int msm_lease_remove(struct platform_device *pdev)
{
	struct msm_lease *lease_drv;

	lease_drv = platform_get_drvdata(pdev);
	if (!lease_drv)
		return 0;
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id dt_match[] = {
	{ .compatible = "qcom,sde-kms-lease" },
	{}
};

static struct platform_driver msm_lease_platform_driver = {
	.probe      = msm_lease_probe,
	.remove     = msm_lease_remove,
	.driver     = {
		.name   = "msm_lease_drm",
		.of_match_table = dt_match,
	},
};

void __init msm_lease_drm_register(void)
{
	platform_driver_register(&msm_lease_platform_driver);
}

void __exit msm_lease_drm_unregister(void)
{
	platform_driver_unregister(&msm_lease_platform_driver);
}

MODULE_DESCRIPTION("MSM LEASE DRM Driver");
MODULE_LICENSE("GPL");
