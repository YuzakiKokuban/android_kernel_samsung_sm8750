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
#include "wlan_dp_telemetry_priv.h"

#ifdef WLAN_DP_FEATURE_STC
QDF_STATUS
ucfg_telemetry_start_opm_stats(struct wlan_objmgr_vdev *vdev,
			       uint32_t periodicity)
{
	struct wlan_dp_intf *dp_intf;
	struct wlan_dp_link *dp_link;

	dp_link = dp_get_vdev_priv_obj(vdev);
	if (qdf_unlikely(!dp_link)) {
		dp_err("DP link not found");
		return QDF_STATUS_E_INVAL;
	}

	dp_intf = dp_link->dp_intf;
	if (qdf_unlikely(!dp_intf)) {
		dp_err_rl("DP interface not found");
		return QDF_STATUS_E_INVAL;
	}

	return dp_telemetry_start_opm_stats(dp_intf, periodicity);
}

QDF_STATUS
ucfg_telemetry_stop_opm_stats(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_dp_intf *dp_intf;
	struct wlan_dp_link *dp_link;

	dp_link = dp_get_vdev_priv_obj(vdev);
	if (qdf_unlikely(!dp_link)) {
		dp_err("DP link not found");
		return QDF_STATUS_E_INVAL;
	}

	dp_intf = dp_link->dp_intf;
	if (qdf_unlikely(!dp_intf)) {
		dp_err_rl("DP interface not found");
		return QDF_STATUS_E_INVAL;
	}

	return dp_telemetry_stop_opm_stats(dp_intf);
}
#endif
