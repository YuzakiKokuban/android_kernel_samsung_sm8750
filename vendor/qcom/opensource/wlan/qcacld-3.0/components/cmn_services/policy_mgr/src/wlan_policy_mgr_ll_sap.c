/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

/**
 * DOC: contains policy manager ll_sap definitions specific to the ll_sap module
 */

#include "wlan_policy_mgr_ll_sap.h"
#include "wlan_policy_mgr_public_struct.h"
#include "wlan_policy_mgr_api.h"
#include "wlan_policy_mgr_i.h"
#include "wlan_cmn.h"
#include "wlan_ll_sap_api.h"
#include "wlan_policy_mgr_api.h"

uint8_t wlan_policy_mgr_get_ll_lt_sap_vdev_id(struct wlan_objmgr_psoc *psoc)
{
	uint8_t ll_lt_sap_cnt;
	uint8_t vdev_id_list[MAX_NUMBER_OF_CONC_CONNECTIONS];

	ll_lt_sap_cnt = policy_mgr_get_mode_specific_conn_info(psoc, NULL,
							vdev_id_list,
							PM_LL_LT_SAP_MODE);

	/* Currently only 1 ll_lt_sap is supported */
	if (!ll_lt_sap_cnt)
		return WLAN_INVALID_VDEV_ID;

	return vdev_id_list[0];
}

bool __policy_mgr_is_ll_lt_sap_restart_required(struct wlan_objmgr_psoc *psoc,
						const char *func)
{
	qdf_freq_t ll_lt_sap_freq = 0;
	uint8_t scc_vdev_id;
	bool is_scc = false;
	uint8_t conn_idx = 0;
	struct policy_mgr_psoc_priv_obj *pm_ctx;

	pm_ctx = policy_mgr_get_context(psoc);
	if (!pm_ctx) {
		policy_mgr_err("Invalid pm ctx");
		return false;
	}

	ll_lt_sap_freq = policy_mgr_get_ll_lt_sap_freq(psoc);

	if (!ll_lt_sap_freq)
		return false;

	/*
	 * Restart ll_lt_sap if any other interface is present in SCC
	 * with LL_LT_SAP.
	 */
	qdf_mutex_acquire(&pm_ctx->qdf_conc_list_lock);
	for (conn_idx = 0; conn_idx < MAX_NUMBER_OF_CONC_CONNECTIONS;
	     conn_idx++) {
		if (pm_conc_connection_list[conn_idx].mode ==
		      PM_LL_LT_SAP_MODE)
			continue;

		if (ll_lt_sap_freq == pm_conc_connection_list[conn_idx].freq) {
			scc_vdev_id = pm_conc_connection_list[conn_idx].vdev_id;
			is_scc = true;
			break;
		}
	}
	qdf_mutex_release(&pm_ctx->qdf_conc_list_lock);

	if (is_scc) {
		uint8_t ll_lt_sap_vdev_id =
				wlan_policy_mgr_get_ll_lt_sap_vdev_id(psoc);

		policy_mgr_rl_nofl_debug("%s ll_lt_sap vdev %d with freq %d is in scc with vdev %d",
					 func, ll_lt_sap_vdev_id,
					 ll_lt_sap_freq, scc_vdev_id);
		return true;
	}

	return false;
}

/**
 * policy_mgr_ll_lt_sap_get_restart_freq_for_concurent_sap() - Get restart frequency
 * for concurrent SAP which is in concurrency with LL_LT_SAP
 * @pm_ctx: Policy manager context
 * @vdev_id: Vdev id of the SAP for which restart freq is required
 * @curr_freq: Current frequency of the SAP for which restart freq is required
 * @event: Indicates if ll_lt_sap is getting enabled or disabled
 * @mode: Vdev mode
 *
 * This API returns user configured frequency if ll_lt_sap is going down and
 * if ll_lt_sap is coming up it returns frequency according to ll_lt_sap
 * concurrency.
 *
 * Return: Restart frequency
 */
static qdf_freq_t
policy_mgr_ll_lt_sap_get_restart_freq_for_concurent_sap(
					struct policy_mgr_psoc_priv_obj *pm_ctx,
					uint8_t vdev_id,
					qdf_freq_t curr_freq,
					enum ll_lt_sap_event event,
					enum policy_mgr_con_mode mode)
{
	qdf_freq_t user_config_freq;
	uint8_t i;
	QDF_STATUS status;
	qdf_freq_t restart_freq = 0, intf_ch_freq = 0;
	uint8_t mcc_to_scc_switch;
	struct policy_mgr_pcl_list *pcl;
	uint32_t channel_list[NUM_CHANNELS];
	uint32_t num_channels;

	policy_mgr_get_mcc_scc_switch(pm_ctx->psoc, &mcc_to_scc_switch);

	/*
	 * If ll_lt_sap is stopped, return user configured frequency
	 * for concurrent SAP restart, if user configured frequency is not valid
	 * frequency, remain on the same frequency and do not restart the SAP
	 */
	if (event == LL_LT_SAP_EVENT_STOPPED) {
		user_config_freq = policy_mgr_get_user_config_sap_freq(
								pm_ctx->psoc,
								vdev_id);

		if (user_config_freq == curr_freq)
			return user_config_freq;

		if (mode == PM_SAP_MODE ||
		    (mode == PM_P2P_GO_MODE &&
		     policy_mgr_go_scc_enforced(pm_ctx->psoc))) {
			policy_mgr_check_scc_channel(pm_ctx->psoc,
						     &intf_ch_freq,
						     user_config_freq,
						     vdev_id,
						     mcc_to_scc_switch);
		} else if (mode == PM_P2P_GO_MODE &&
			  !policy_mgr_allow_concurrency(pm_ctx->psoc, mode,
							 user_config_freq,
							 HW_MODE_20_MHZ, 0,
							 vdev_id)) {
			policy_mgr_debug("vdev %d User freq %d not allowed, keep current freq %d",
					 vdev_id, user_config_freq, curr_freq);
			return curr_freq;
		}
		if (intf_ch_freq && (intf_ch_freq != user_config_freq))
			user_config_freq = intf_ch_freq;
		return user_config_freq;
	}

	pcl = qdf_mem_malloc(sizeof(*pcl));
	if (!pcl)
		return 0;

	status = policy_mgr_get_pcl_for_vdev_id(pm_ctx->psoc, mode,
						pcl->pcl_list,
						&pcl->pcl_len,
						pcl->weight_list,
						QDF_ARRAY_SIZE(pcl->weight_list),
						vdev_id);
	if (QDF_IS_STATUS_ERROR(status))
		goto free_pcl;

	/* Return first valid 2.4 GHz or SCC freq for SAP frequency */
	for (i = 0; i < pcl->pcl_len; i++) {
		if (!wlan_reg_is_24ghz_ch_freq(pcl->pcl_list[i]) ||
		    !policy_mgr_is_safe_channel(pm_ctx->psoc,
					       pcl->pcl_list[i]))
			continue;
		if (!restart_freq)
			restart_freq = pcl->pcl_list[i];
		/* Prefer SCC frequency */
		if ((mode == PM_SAP_MODE ||
		     policy_mgr_go_scc_enforced(pm_ctx->psoc)) &&
		     policy_mgr_get_connection_count_with_ch_freq(
							pcl->pcl_list[i])) {
			restart_freq = pcl->pcl_list[i];
			break;
		}
	}

free_pcl:
	qdf_mem_free(pcl);

	if (restart_freq)
		goto ret_freq;

	status = policy_mgr_get_valid_chans(pm_ctx->psoc, channel_list,
					    &num_channels);
	if (QDF_IS_STATUS_ERROR(status)) {
		policy_mgr_err("Error in getting valid channels");
		return restart_freq;
	}

	/* return first valid 2.4 GHz frequency */
	for (i = 0; i < num_channels; i++) {
		if (wlan_reg_is_24ghz_ch_freq(channel_list[i]) &&
		    policy_mgr_is_safe_channel(pm_ctx->psoc,
					       channel_list[i])) {
			restart_freq = channel_list[i];
			policy_mgr_debug("vdev %d Selected first valid 2.4 Ghz freq %d",
					 vdev_id, restart_freq);
			break;
		}
	}

ret_freq:
	return restart_freq;
}

/**
 * policy_mgr_ll_lt_sap_check_for_restart_sap_go() - Check if freq change is
 * required for concurrent SAP/GO
 * @pm_ctx: Policy manager context
 * @sap_info: SAP info
 * @event: Indicates if ll_lt_sap is getting enabled or disabled
 *
 * Return: void
 */
static void
policy_mgr_ll_lt_sap_check_for_restart_sap_go(
			struct policy_mgr_psoc_priv_obj *pm_ctx,
			struct policy_mgr_conc_connection_info *sap_info,
			enum ll_lt_sap_event event)
{
	qdf_freq_t restart_freq;
	struct ch_params ch_params = {0};
	enum sap_csa_reason_code csa_reason = CSA_REASON_LL_LT_SAP_EVENT;
	QDF_STATUS status;

	/*
	 * On LL LT SAP Starting, if SAP is on 5 Ghz, or GO has some other vdev
	 * on 5 Ghz with it, restart SAP/GO.
	 * On LL LT SAP stopping, and SAP/GO on 2.4 GHz try to move to 5 Ghz.
	 */
	if (sap_info->mode == PM_SAP_MODE || sap_info->mode == PM_P2P_GO_MODE) {
		/*
		 * No need to change channel for dynamic SBS
		 * OR SAP/GO is on 2.4 Ghz channel, when LL_LT_SAP is
		 * starting/started,
		 * OR LL_LT_SAP has stopped and SAP/GO is on 5Ghz.
		 */
		if ((policy_mgr_is_dynamic_sbs_enabled(pm_ctx->psoc) ||
		    ((event == LL_LT_SAP_EVENT_STARTING ||
		     event == LL_LT_SAP_EVENT_STARTED) &&
		     wlan_reg_is_24ghz_ch_freq(sap_info->freq))) ||
		    (event == LL_LT_SAP_EVENT_STOPPED &&
		     !wlan_reg_is_24ghz_ch_freq(sap_info->freq)))
			return;

		/*
		 * If GO is alone on the 5 Ghz mac and LL_LT_SAP is starting OR
		 * LL_LT_SAP is already up, no switch required.
		 */
		if (sap_info->mode == PM_P2P_GO_MODE &&
		    (event == LL_LT_SAP_EVENT_STARTED ||
		     (event == LL_LT_SAP_EVENT_STARTING &&
		    !policy_mgr_any_other_vdev_on_same_mac_as_freq(pm_ctx->psoc,
				sap_info->freq, sap_info->vdev_id))))
			return;

		restart_freq =
			policy_mgr_ll_lt_sap_get_restart_freq_for_concurent_sap(
							pm_ctx,
							sap_info->vdev_id,
							sap_info->freq,
							event,
							sap_info->mode);
	} else {
		restart_freq = wlan_get_ll_lt_sap_restart_freq(pm_ctx->pdev,
							sap_info->freq,
							sap_info->vdev_id,
							&csa_reason);
	}

	if (!restart_freq) {
		policy_mgr_debug("vdev %d Restart freq not found, curr freq %d",
				 sap_info->vdev_id, sap_info->freq);
		return;
	}
	if (restart_freq == sap_info->freq) {
		policy_mgr_debug("vdev %d restart freq %d same as current freq",
				 sap_info->vdev_id, restart_freq);
		return;
	}

	if (policy_mgr_is_chan_switch_in_progress(pm_ctx->psoc))
		return;

	ch_params.ch_width = CH_WIDTH_MAX;
	wlan_reg_set_channel_params_for_pwrmode(pm_ctx->pdev, restart_freq,
						0, &ch_params,
						REG_CURRENT_PWR_MODE);
	policy_mgr_debug("vdev %d event %d mode %d, %d -> %d, width %d",
			 sap_info->vdev_id, event, sap_info->mode,
			 sap_info->freq, restart_freq, ch_params.ch_width);

	if (pm_ctx->hdd_cbacks.wlan_hdd_set_sap_csa_reason)
		pm_ctx->hdd_cbacks.wlan_hdd_set_sap_csa_reason(pm_ctx->psoc,
							      sap_info->vdev_id,
							      csa_reason);

	policy_mgr_change_sap_channel_with_csa(pm_ctx->psoc, sap_info->vdev_id,
					       restart_freq,
					       ch_params.ch_width, true);

	if (policy_mgr_is_chan_switch_in_progress(pm_ctx->psoc)) {
		status = policy_mgr_wait_chan_switch_complete_evt(pm_ctx->psoc);
		if (!QDF_IS_STATUS_SUCCESS(status)) {
			policy_mgr_err("Vdev %d wait for csa event failed!!",
				       sap_info->vdev_id);
			return;
		}
	}
}

void policy_mgr_ll_lt_sap_restart_concurrent_sap(struct wlan_objmgr_psoc *psoc,
						 enum ll_lt_sap_event event)
{
	struct policy_mgr_psoc_priv_obj *pm_ctx;
	struct policy_mgr_conc_connection_info sap_info[MAX_NUMBER_OF_CONC_CONNECTIONS] = {0};
	uint8_t i;

	pm_ctx = policy_mgr_get_context(psoc);
	if (!pm_ctx) {
		policy_mgr_err("Invalid pm context");
		return;
	}

	qdf_mutex_acquire(&pm_ctx->qdf_conc_list_lock);
	qdf_mem_copy(&sap_info, &pm_conc_connection_list, sizeof(sap_info));
	qdf_mutex_release(&pm_ctx->qdf_conc_list_lock);

	for (i = 0; i < MAX_NUMBER_OF_CONC_CONNECTIONS; i++) {
		if (!sap_info[i].in_use)
			continue;
		if (sap_info[i].mode == PM_SAP_MODE ||
		    sap_info[i].mode == PM_LL_LT_SAP_MODE ||
		    sap_info[i].mode == PM_P2P_GO_MODE)
			policy_mgr_ll_lt_sap_check_for_restart_sap_go(pm_ctx,
					&sap_info[i], event);
	}
}
