/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "wlan_dp_priv.h"
#include "wlan_dp_fim.h"
#include "wlan_dp_ucfg_api.h"

#if defined(WLAN_FEATURE_SAWFISH) || defined(WLAN_DP_FEATURE_STC)
static inline void ucfg_dp_update_sawf_metadata(struct wlan_dp_intf *dp_intf,
						qdf_nbuf_t nbuf)
{
	wlan_dp_sawfish_update_metadata(dp_intf, nbuf);

	return;
}
#else
static inline void ucfg_dp_update_sawf_metadata(struct wlan_dp_intf *dp_intf,
						qdf_nbuf_t nbuf)
{
}
#endif

#ifdef CONFIG_WLAN_SUPPORT_LAPB
static inline void ucfg_dp_update_lapb_metadata(struct wlan_dp_intf *dp_intf,
						qdf_nbuf_t nbuf)
{
	QDF_STATUS status;

	status = dp_fim_update_metadata(dp_intf, nbuf);
	if (qdf_unlikely(status == QDF_STATUS_SUCCESS))
		ucfg_dp_lapb_handle_app_ind(nbuf);
}
#else
static inline void ucfg_dp_update_lapb_metadata(struct wlan_dp_intf *dp_intf,
						qdf_nbuf_t nbuf)
{
}
#endif

#if defined(WLAN_SUPPORT_FLOW_PRIORTIZATION) || defined(WLAN_FEATURE_SAWFISH) \
						|| defined(WLAN_DP_FEATURE_STC)
/*
 * ucfg_dp_fim_update_metadata() - Update skb with metadata
 * @nbuf: skb
 * @vdev:vdev
 *
 * Return: None
 */
void ucfg_dp_fim_update_metadata(qdf_nbuf_t nbuf, struct wlan_objmgr_vdev *vdev)
{
	struct wlan_dp_intf *dp_intf;
	struct wlan_dp_link *dp_link;

	if (qdf_unlikely(!vdev))
		return;

	dp_link = dp_get_vdev_priv_obj(vdev);
	if (qdf_unlikely(!dp_link)) {
		dp_err_rl("DP link not found");
		return;
	}

	dp_intf = dp_link->dp_intf;
	if (qdf_unlikely(!dp_intf)) {
		dp_err_rl("DP interface not found");
		return;
	}

	ucfg_dp_update_sawf_metadata(dp_intf, nbuf);
	ucfg_dp_update_lapb_metadata(dp_intf, nbuf);
}
#endif

#ifdef WLAN_SUPPORT_FLOW_PRIORTIZATION
void ucfg_dp_fim_display_hash_table(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_dp_intf *dp_intf;
	struct wlan_dp_link *dp_link;

	dp_link = dp_get_vdev_priv_obj(vdev);
	if (qdf_unlikely(!dp_link)) {
		dp_err_rl("DP link not found");
		return;
	}

	dp_intf = dp_link->dp_intf;
	if (qdf_unlikely(!dp_intf)) {
		dp_err_rl("DP interface not found");
		return;
	}

	dp_fim_display_hash_table(dp_intf);
}

void ucfg_dp_fim_display_stats(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_dp_intf *dp_intf;
	struct wlan_dp_link *dp_link;

	dp_link = dp_get_vdev_priv_obj(vdev);
	if (qdf_unlikely(!dp_link)) {
		dp_err_rl("DP link not found");
		return;
	}

	dp_intf = dp_link->dp_intf;
	if (qdf_unlikely(!dp_intf)) {
		dp_err_rl("DP interface not found");
		return;
	}

	dp_fim_display_stats(dp_intf);
}

void ucfg_dp_fim_clear_stats(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_dp_intf *dp_intf;
	struct wlan_dp_link *dp_link;

	dp_link = dp_get_vdev_priv_obj(vdev);
	if (qdf_unlikely(!dp_link)) {
		dp_err_rl("DP link not found");
		return;
	}

	dp_intf = dp_link->dp_intf;
	if (qdf_unlikely(!dp_intf)) {
		dp_err_rl("DP interface not found");
		return;
	}

	dp_fim_clear_stats(dp_intf);
}

void ucfg_dp_fim_clear_hash_table(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_dp_intf *dp_intf;
	struct wlan_dp_link *dp_link;

	dp_link = dp_get_vdev_priv_obj(vdev);
	if (qdf_unlikely(!dp_link)) {
		dp_err_rl("DP link not found");
		return;
	}

	dp_intf = dp_link->dp_intf;
	if (qdf_unlikely(!dp_intf)) {
		dp_err_rl("DP interface not found");
		return;
	}

	dp_fim_clear_hash_table(dp_intf);
}

bool ucfg_dp_fpm_check_tid_override_tagged(qdf_nbuf_t nbuf)
{
	return fpm_check_tid_override_tagged(nbuf);
}

struct fpm_table *
ucfg_fpm_policy_get_ctx_by_vdev(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_dp_intf *dp_intf;
	struct wlan_dp_link *dp_link;

	dp_link = dp_get_vdev_priv_obj(vdev);
	if (qdf_unlikely(!dp_link)) {
		dp_err_rl("DP link not found");
		return NULL;
	}

	dp_intf = dp_link->dp_intf;
	if (qdf_unlikely(!dp_intf)) {
		dp_err_rl("DP interface not found");
		return NULL;
	}

	return dp_intf->fpm_ctx;
}

QDF_STATUS
ucfg_fpm_policy_update(struct fpm_table *fpm, struct dp_policy *policy)
{
	return fpm_policy_update(fpm, policy);
}

QDF_STATUS
ucfg_fpm_policy_add(struct fpm_table *fpm, struct dp_policy *policy)
{
	return fpm_policy_add(fpm, policy);
}

QDF_STATUS ucfg_fpm_policy_rem(struct fpm_table *fpm, uint64_t policy_id)
{
	return fpm_policy_rem(fpm, policy_id);
}

uint8_t
ucfg_fpm_policy_get(struct fpm_table *fpm, struct dp_policy *policies,
		    uint8_t max_count)
{
	return fpm_policy_get(fpm, policies, max_count);
}

void ucfg_dp_fpm_display_policy(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_dp_intf *dp_intf;
	struct wlan_dp_link *dp_link;

	dp_link = dp_get_vdev_priv_obj(vdev);
	if (qdf_unlikely(!dp_link)) {
		dp_err_rl("DP link not found");
		return;
	}

	dp_intf = dp_link->dp_intf;
	if (qdf_unlikely(!dp_intf)) {
		dp_err_rl("DP interface not found");
		return;
	}

	dp_fpm_display_policy(dp_intf);
}
#endif
