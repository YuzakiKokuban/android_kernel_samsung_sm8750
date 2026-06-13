/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
 /**
  * DOC: os_if_telemetry.c
  *
  *
  */
#include "os_if_telemetry.h"
#include "wlan_cfg80211.h"
#include "wlan_dp_ucfg_api.h"

#ifdef WLAN_DP_FEATURE_STC

const struct nla_policy
stats_service_policy[STATS_POLICY_MAX + 1] = {
	[STATS_POLICY_TYPE] = { .type = NLA_U8 },
	[STATS_POLICY_ACTION] = { .type = NLA_U8 },
	[STATS_POLICY_PERIODICITY] = { .type = NLA_U32 },
};

QDF_STATUS os_if_telemetry_stats_service(struct wlan_objmgr_vdev *vdev,
					 const void *data, int data_len)
{
	struct nlattr *tb[STATS_POLICY_MAX + 1];
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	uint8_t spt, action;
	uint32_t periodicity;

	if (wlan_cfg80211_nla_parse(tb, STATS_POLICY_MAX,
				    data, data_len, stats_service_policy)) {
		osif_err("Invalid attr");
		goto error;
	}

	spt = 0;
	if (tb[STATS_POLICY_TYPE]) {
		spt = nla_get_u8(tb[STATS_POLICY_TYPE]);

		if (spt != QCA_ASYNC_STATS_TYPE_FLOW_STATS &&
		    spt != QCA_ASYNC_STATS_TYPE_CLASSIFIED_FLOW_STATS &&
		    spt != QCA_ASYNC_STATS_TYPE_POWERSAVE) {
			osif_err("Invalid stats policy type: %d", spt);
			goto error;
		}
	}

	action = 0;
	if (tb[STATS_POLICY_ACTION]) {
		action = nla_get_u8(tb[STATS_POLICY_ACTION]);

		if (action != QCA_ASYNC_STATS_ACTION_START &&
		    action != QCA_ASYNC_STATS_ACTION_STOP) {
			osif_err("Invalid action: %d expected: %d or %d",
				 action,
				 QCA_ASYNC_STATS_ACTION_START,
				 QCA_ASYNC_STATS_ACTION_STOP);
			goto error;
		}
	}

	periodicity = 0;
	if (tb[STATS_POLICY_PERIODICITY]) {
		periodicity = nla_get_u32(tb[STATS_POLICY_PERIODICITY]);

		if (periodicity == 0) {
			osif_err("Invalid periodicity: %u", periodicity);
			goto error;
		}
	} else {
		if (spt == QCA_ASYNC_STATS_TYPE_POWERSAVE) {
			osif_err("Periodicity field (Mandatory) missing");
			goto error;
		}
	}

	osif_debug("spt: %d action: %d periodicity: %u",
		   spt, action, periodicity);

	switch (spt) {
	case QCA_ASYNC_STATS_TYPE_FLOW_STATS:
	case QCA_ASYNC_STATS_TYPE_CLASSIFIED_FLOW_STATS:
		ucfg_dp_flow_stats_policy(spt, action);
		break;

	case QCA_ASYNC_STATS_TYPE_POWERSAVE:
		if (action == QCA_ASYNC_STATS_ACTION_START)
			ucfg_telemetry_start_opm_stats(vdev, periodicity);
		else
			ucfg_telemetry_stop_opm_stats(vdev);

		break;

	default:
		osif_warn("Unsupported stats type: %d", spt);
		break;
	}

	return status;

error:
	status = QDF_STATUS_E_INVAL;
	return status;
}
#endif
