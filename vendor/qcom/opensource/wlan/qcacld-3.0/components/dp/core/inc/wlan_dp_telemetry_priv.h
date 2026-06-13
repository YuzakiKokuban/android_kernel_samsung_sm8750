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

#ifndef _WLAN_DP_TELEMETRY_PRIV_H_
#define _WLAN_DP_TELEMETRY_PRIV_H_

#include <qdf_status.h>

/**
 * dp_telemetry_start_opm_stats() - Handle start OPM stats command
 * @dp_intf: Pointer to DP interface structure
 * @periodicity: periodicity from the user space
 *
 * Return: %QDF_STATUS_SUCCESS on success, error status otherwise
 */
QDF_STATUS dp_telemetry_start_opm_stats(struct wlan_dp_intf *dp_intf,
					uint32_t periodicity);

/**
 * dp_telemetry_stop_opm_stats() - Handle stop OPM stats command
 * @dp_intf: Pointer to DP interface structure
 *
 * Return: %QDF_STATUS_SUCCESS on success, error status otherwise
 */
QDF_STATUS dp_telemetry_stop_opm_stats(struct wlan_dp_intf *dp_intf);

#ifdef WLAN_FEATURE_TELEMETRY
/**
 * dp_telemetry_init() - Initialize DP telemetry
 * @dp_intf: Pointer to DP interface structure
 *
 * Return: %QDF_STATUS_SUCCESS on success, error status otherwise
 */
QDF_STATUS dp_telemetry_init(struct wlan_dp_intf *dp_intf);

/**
 * dp_telemetry_deinit() - De-initialize DP telemetry
 * @dp_intf: Pointer to DP interface structure
 *
 * Return: %QDF_STATUS_SUCCESS on success, error status otherwise
 */
QDF_STATUS dp_telemetry_deinit(struct wlan_dp_intf *dp_intf);
#else
static inline
QDF_STATUS dp_telemetry_init(struct wlan_dp_intf *dp_intf)
{
	return QDF_STATUS_SUCCESS;
}

static inline
QDF_STATUS dp_telemetry_deinit(struct wlan_dp_intf *dp_intf)
{
	return QDF_STATUS_SUCCESS;
}
#endif
#endif
