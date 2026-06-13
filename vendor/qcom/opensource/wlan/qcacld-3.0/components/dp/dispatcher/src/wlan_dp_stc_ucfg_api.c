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

#include "wlan_dp_ucfg_api.h"
#include "wlan_dp_stc.h"

QDF_STATUS ucfg_dp_flow_classify_result(
		struct wlan_dp_stc_flow_classify_result *flow_classify_result)
{
	wlan_dp_stc_handle_flow_classify_result(flow_classify_result);
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS ucfg_dp_flow_stats_policy(enum qca_async_stats_type type,
				     enum qca_async_stats_action action)
{
	return wlan_dp_stc_handle_flow_stats_policy(type, action);
}

uint32_t ucfg_dp_stc_get_logmask(struct wlan_objmgr_psoc *psoc)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_psoc_get_priv(psoc);

	if (!dp_ctx) {
		dp_err("Unable to get DP context");
		return 0;
	}

	return wlan_dp_stc_get_logmask(dp_ctx);
}

void ucfg_dp_stc_update_logmask(struct wlan_objmgr_psoc *psoc, uint32_t mask)
{
	struct wlan_dp_psoc_context *dp_ctx = dp_psoc_get_priv(psoc);

	if (!dp_ctx) {
		dp_err("Unable to get DP context");
		return;
	}

	wlan_dp_stc_update_logmask(dp_ctx, mask);
}
