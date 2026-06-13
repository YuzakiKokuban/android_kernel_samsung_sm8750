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

#include <dp_types.h>
#include <dp_internal.h>
#include <wlan_dp_priv.h>
#include <qdf_status.h>
#include <qdf_periodic_work.h>
#include <wlan_dp_telemetry_priv.h>
#include <target_if.h>

#ifdef WLAN_DP_FEATURE_STC
/**
 * dp_telemetry_opm_stats_work_handler() - OPM stats work handler
 * @context: context
 *
 * This is the periodic work handler which will issue OPM Stats
 * WMI command to firmware.
 *
 * Return: None
 */
static void dp_telemetry_opm_stats_work_handler(void *context)
{
	struct wlan_dp_intf *dp_intf = (struct wlan_dp_intf *)context;
	uint8_t pdev_id = 0;

	if (!dp_intf)
		return;

	pdev_id = wlan_objmgr_pdev_get_pdev_id(dp_intf->dp_ctx->pdev);

	if (dp_intf->dp_ctx->sb_ops.dp_send_opm_stats_cmd)
		dp_intf->dp_ctx->sb_ops.dp_send_opm_stats_cmd(
			dp_intf->dp_ctx->psoc, pdev_id);
}

static QDF_STATUS
dp_telemetry_opm_stats_create_work(struct wlan_dp_intf *dp_intf)
{
	return qdf_periodic_work_create(&dp_intf->opm_stats_work,
					dp_telemetry_opm_stats_work_handler,
					dp_intf);
}

static void
dp_telemetry_opm_stats_destroy_work(struct wlan_dp_intf *dp_intf)
{
	qdf_periodic_work_destroy(&dp_intf->opm_stats_work);
}

static bool
dp_telemetry_opm_stats_start_work(struct wlan_dp_intf *dp_intf,
				  uint32_t periodicity)
{
	return qdf_periodic_work_start(&dp_intf->opm_stats_work,
				       periodicity);
}

static bool
dp_telemetry_opm_stats_stop_work(struct wlan_dp_intf *dp_intf)
{
	return qdf_periodic_work_stop_sync(&dp_intf->opm_stats_work);
}
#else
static QDF_STATUS
dp_telemetry_opm_stats_create_work(struct wlan_dp_intf *dp_intf)
{
	return QDF_STATUS_SUCCESS;
}

static void
dp_telemetry_opm_stats_destroy_work(struct wlan_dp_intf *dp_intf)
{
}

static bool
dp_telemetry_opm_stats_start_work(struct wlan_dp_intf *dp_intf,
				  uint32_t periodicity)
{
	return true;
}

static bool
dp_telemetry_opm_stats_stop_work(struct wlan_dp_intf *dp_intf)
{
	return true;
}

#endif

QDF_STATUS dp_telemetry_init(struct wlan_dp_intf *dp_intf)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	enum QDF_OPMODE mode;

	mode = dp_intf->device_mode;
	if (mode != QDF_STA_MODE) {
		dp_err("stats not supported in device mode: %d", mode);
		return QDF_STATUS_E_INVAL;
	}

	status = dp_telemetry_opm_stats_create_work(dp_intf);

	return status;
}

QDF_STATUS dp_telemetry_deinit(struct wlan_dp_intf *dp_intf)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	enum QDF_OPMODE mode;

	mode = dp_intf->device_mode;
	if (mode != QDF_STA_MODE) {
		dp_err("stats not supported in device mode: %d", mode);
		return QDF_STATUS_E_INVAL;
	}

	dp_telemetry_opm_stats_destroy_work(dp_intf);

	return status;
}

QDF_STATUS dp_telemetry_start_opm_stats(struct wlan_dp_intf *dp_intf,
					uint32_t periodicity)
{
	bool status;

	status = dp_telemetry_opm_stats_start_work(dp_intf, periodicity);

	return status ? QDF_STATUS_SUCCESS : QDF_STATUS_E_FAILURE;
}

QDF_STATUS dp_telemetry_stop_opm_stats(struct wlan_dp_intf *dp_intf)
{
	bool status;

	status = dp_telemetry_opm_stats_stop_work(dp_intf);

	return status ? QDF_STATUS_SUCCESS : QDF_STATUS_E_FAILURE;
}
