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

#include "wlan_ll_lt_sap_main.h"
#include "wlan_reg_services_api.h"
#include "wlan_ll_sap_public_structs.h"
#include "wlan_policy_mgr_i.h"
#include "wlan_mlme_vdev_mgr_interface.h"
#include "wlan_ll_sap_main.h"
#include "wlan_ll_lt_sap_bearer_switch.h"
#include "wlan_scan_api.h"
#include "target_if.h"
#include "wlan_twt_cfg_ext_api.h"

bool ll_lt_sap_is_supported(struct wlan_objmgr_psoc *psoc)
{
	struct ll_sap_psoc_priv_obj *psoc_ll_sap_obj;

	psoc_ll_sap_obj =
		wlan_objmgr_psoc_get_comp_private_obj(psoc,
						      WLAN_UMAC_COMP_LL_SAP);
	if (!psoc_ll_sap_obj) {
		ll_sap_err("Invalid psoc ll sap priv obj");
		return false;
	}

	return psoc_ll_sap_obj->is_ll_lt_sap_supported;
}

void ll_lt_sap_extract_ll_sap_cap(struct wlan_objmgr_psoc *psoc)
{
	struct wmi_unified *wmi_handle;
	struct ll_sap_psoc_priv_obj *psoc_ll_sap_obj;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		ll_sap_err("Invalid WMI handle");
		return;
	}

	psoc_ll_sap_obj =
		wlan_objmgr_psoc_get_comp_private_obj(psoc,
						      WLAN_UMAC_COMP_LL_SAP);
	if (!psoc_ll_sap_obj) {
		ll_sap_err("Invalid psoc ll sap priv obj");
		return;
	}

	psoc_ll_sap_obj->is_ll_lt_sap_supported =
		wmi_service_enabled(wmi_handle, wmi_service_xpan_support);

	/**
	 * This will be filled based on firmware service capability
	 * later when firmware provides it.
	 */
	psoc_ll_sap_obj->is_beared_switch_required = true;
}

bool ll_lt_sap_get_bearer_switch_cap_for_csa(struct wlan_objmgr_psoc *psoc)
{
	struct ll_sap_psoc_priv_obj *psoc_ll_sap_obj;

	psoc_ll_sap_obj =
		wlan_objmgr_psoc_get_comp_private_obj(psoc,
						      WLAN_UMAC_COMP_LL_SAP);

	if (!psoc_ll_sap_obj) {
		ll_sap_err("Invalid psoc ll sap priv obj");
		return false;
	}

	return psoc_ll_sap_obj->is_beared_switch_required;
}

/**
 * ll_lt_sap_get_sorted_user_config_acs_ch_list() - API to get sorted user
 * configured ACS channel list
 * @psoc: Pointer to psoc object
 * @vdev_id: Vdev Id
 * @ch_info: Pointer to ch_info
 *
 * Return: None
 */
static
QDF_STATUS ll_lt_sap_get_sorted_user_config_acs_ch_list(
					struct wlan_objmgr_psoc *psoc,
					uint8_t vdev_id,
					struct sap_sel_ch_info *ch_info)
{
	struct scan_filter *filter;
	qdf_list_t *list = NULL;
	struct wlan_objmgr_pdev *pdev;
	struct wlan_objmgr_vdev *vdev;
	QDF_STATUS status = QDF_STATUS_E_FAILURE;

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, vdev_id,
						    WLAN_LL_SAP_ID);

	if (!vdev) {
		ll_sap_err("Invalid vdev for vdev_id %d", vdev_id);
		return QDF_STATUS_E_FAILURE;
	}

	pdev = wlan_vdev_get_pdev(vdev);

	filter = qdf_mem_malloc(sizeof(*filter));
	if (!filter)
		goto rel_ref;

	status = wlan_sap_get_user_config_acs_ch_list(vdev_id, filter);
	if (QDF_IS_STATUS_ERROR(status)) {
		ll_sap_err("vdev %d failed to get user config acs ch_list",
			   vdev_id);
		qdf_mem_free(filter);
		goto rel_ref;
	}

	list = wlan_scan_get_result(pdev, filter);

	qdf_mem_free(filter);

	if (!list)
		goto rel_ref;

	status = wlan_ll_sap_sort_channel_list(vdev_id, list, ch_info);
	if (QDF_IS_STATUS_ERROR(status)) {
		ll_sap_err("vdev %d failed to sort sap channel list",
			   vdev_id);
		goto rel_ref;
	}

	wlan_scan_purge_results(list);

rel_ref:
	wlan_objmgr_vdev_release_ref(vdev, WLAN_LL_SAP_ID);

	return status;
}

QDF_STATUS ll_lt_sap_init(struct wlan_objmgr_vdev *vdev)
{
	struct ll_sap_vdev_priv_obj *ll_sap_obj;
	QDF_STATUS status;
	uint8_t i, j;
	struct bearer_switch_info *bs_ctx;

	ll_sap_obj = ll_sap_get_vdev_priv_obj(vdev);

	if (!ll_sap_obj) {
		ll_sap_err("vdev %d ll_sap obj null",
			   wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_INVAL;
	}

	for (i = 0; i < MAX_HIGH_AP_AVAILABILITY_REQUESTS; i++)
		ll_sap_obj->high_ap_availability_cookie[i] =
							LL_SAP_INVALID_COOKIE;

	bs_ctx = qdf_mem_malloc(sizeof(struct bearer_switch_info));
	if (!bs_ctx)
		return QDF_STATUS_E_NOMEM;

	ll_sap_obj->bearer_switch_ctx = bs_ctx;

	qdf_atomic_init(&bs_ctx->request_id);

	for (i = 0; i < WLAN_UMAC_PSOC_MAX_VDEVS; i++)
		for (j = 0; j < BEARER_SWITCH_REQ_MAX; j++)
			qdf_atomic_init(&bs_ctx->ref_count[i][j]);

	qdf_atomic_init(&bs_ctx->fw_ref_count);
	qdf_atomic_init(&bs_ctx->total_ref_count);

	bs_ctx->vdev = vdev;

	status = bs_sm_create(bs_ctx);

	if (QDF_IS_STATUS_ERROR(status))
		goto bs_sm_failed;

	ll_sap_debug("vdev %d", wlan_vdev_get_id(vdev));

	return QDF_STATUS_SUCCESS;

bs_sm_failed:
	qdf_mem_free(ll_sap_obj->bearer_switch_ctx);
	ll_sap_obj->bearer_switch_ctx = NULL;
	return status;
}

QDF_STATUS ll_lt_sap_deinit(struct wlan_objmgr_vdev *vdev)
{
	struct ll_sap_vdev_priv_obj *ll_sap_obj;

	ll_sap_obj = ll_sap_get_vdev_priv_obj(vdev);

	if (!ll_sap_obj) {
		ll_sap_err("vdev %d ll_sap obj null",
			   wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_INVAL;
	}

	if (!ll_sap_obj->bearer_switch_ctx) {
		ll_sap_debug("vdev %d Bearer switch context is NULL",
			     wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_INVAL;
	}

	bs_sm_destroy(ll_sap_obj->bearer_switch_ctx);
	qdf_mem_free(ll_sap_obj->bearer_switch_ctx);
	ll_sap_obj->bearer_switch_ctx = NULL;

	ll_sap_debug("vdev %d", wlan_vdev_get_id(vdev));

	return QDF_STATUS_SUCCESS;
}

static
uint8_t ll_lt_sap_get_oldest_entry_index(struct wlan_ll_lt_sap_avoid_freq *list)
{
	uint8_t oldest_entry_idx = 0, i;
	qdf_time_t oldest_entry_timestamp;

	oldest_entry_timestamp = list->freq_list[0].timestamp;
	for (i = 1; i < MAX_CHAN_TO_STORE; i++) {
		if (qdf_system_time_after(
				oldest_entry_timestamp,
				list->freq_list[i].timestamp)) {
			oldest_entry_idx = i;
			oldest_entry_timestamp = list->freq_list[i].timestamp;
		}
	}

	return oldest_entry_idx;
}

static
void ll_lt_sap_store_curr_chan_in_db(
			struct ll_sap_psoc_priv_obj *ll_sap_psoc_obj,
			qdf_freq_t freq)
{
	uint8_t i;

	if (ll_sap_psoc_obj->avoid_freq.stored_num_ch >= MAX_CHAN_TO_STORE) {
		i = ll_lt_sap_get_oldest_entry_index(
					&ll_sap_psoc_obj->avoid_freq);
		ll_sap_psoc_obj->avoid_freq.freq_list[i].freq = freq;
		ll_sap_psoc_obj->avoid_freq.freq_list[i].timestamp =
						qdf_get_time_of_the_day_ms();
		return;
	}

	for (i = 0; i < MAX_CHAN_TO_STORE; i++) {
		if (!ll_sap_psoc_obj->avoid_freq.freq_list[i].freq &&
		    !ll_sap_psoc_obj->avoid_freq.freq_list[i].timestamp) {
			ll_sap_psoc_obj->avoid_freq.freq_list[i].freq = freq;
			ll_sap_psoc_obj->avoid_freq.freq_list[i].timestamp =
						qdf_get_time_of_the_day_ms();
			ll_sap_psoc_obj->avoid_freq.stored_num_ch++;
			return;
		}
	}
}

bool ll_lt_sap_is_freq_in_avoid_list(
			struct ll_sap_psoc_priv_obj *ll_sap_psoc_obj,
			qdf_freq_t freq)
{
	uint8_t i;

	for (i = 0; i < MAX_CHAN_TO_STORE; i++) {
		if (freq == ll_sap_psoc_obj->avoid_freq.freq_list[i].freq)
			return true;
	}

	return false;
}

/* DCS database ageout time in ms to store channel */
#define DCS_DB_AGEOUT_TIME (5 * 60 * 1000)
static
void ll_lt_sap_flush_old_entries(struct ll_sap_psoc_priv_obj *ll_sap_psoc_obj)
{
	uint8_t i;

	for (i = 0; i < MAX_CHAN_TO_STORE; i++) {
		if (ll_sap_psoc_obj->avoid_freq.freq_list[i].timestamp &&
		    (qdf_system_time_after(
			       qdf_get_time_of_the_day_ms(),
			       ll_sap_psoc_obj->avoid_freq.freq_list[i].timestamp +
			       DCS_DB_AGEOUT_TIME))) {
			ll_sap_psoc_obj->avoid_freq.freq_list[i].timestamp = 0;
			ll_sap_psoc_obj->avoid_freq.freq_list[i].freq = 0;
			ll_sap_psoc_obj->avoid_freq.stored_num_ch--;
		}
	}
}

#define LL_SAP_MAX_FREQ_LIST_INFO_LOG 100
static void
ll_lt_sap_dump_stored_freq_list(struct ll_sap_psoc_priv_obj *ll_sap_psoc_obj)
{
	uint8_t *freq_list;
	uint8_t i = 0, len = 0;

	freq_list = qdf_mem_malloc(LL_SAP_MAX_FREQ_LIST_INFO_LOG);
	if (!freq_list)
		return;

	for (i = 0; i < MAX_CHAN_TO_STORE; i++) {
		if (ll_sap_psoc_obj->avoid_freq.freq_list[i].freq)
			len += qdf_scnprintf(
				freq_list + len,
				LL_SAP_MAX_FREQ_LIST_INFO_LOG - len,
				"%d[%lu] ",
				ll_sap_psoc_obj->avoid_freq.freq_list[i].freq,
				ll_sap_psoc_obj->avoid_freq.freq_list[i].timestamp);
	}

	ll_sap_nofl_debug("freq[timestamp]: %s", freq_list);

	qdf_mem_free(freq_list);
}

void ll_lt_store_to_avoid_list_and_flush_old(struct wlan_objmgr_psoc *psoc,
					     qdf_freq_t freq,
					     enum ll_sap_csa_source csa_src)
{
	struct ll_sap_psoc_priv_obj *ll_sap_psoc_obj;
	uint8_t num_ch;

	ll_sap_psoc_obj = wlan_objmgr_psoc_get_comp_private_obj(
							psoc,
							WLAN_UMAC_COMP_LL_SAP);
	if (!ll_sap_psoc_obj) {
		ll_sap_err("psoc_ll_sap_obj is null");
		return;
	}

	num_ch = ll_sap_psoc_obj->avoid_freq.stored_num_ch;
	/*
	 * Reset frequency and it's timestamp if stored channel
	 * timestamp is greater than DCS_DB_AGEOUT_TIME
	 */
	ll_lt_sap_flush_old_entries(ll_sap_psoc_obj);

	if (freq && csa_src == LL_SAP_CSA_DCS)
		ll_lt_sap_store_curr_chan_in_db(ll_sap_psoc_obj, freq);

	if ((csa_src != LL_SAP_CSA_DCS && num_ch) ||
	    num_ch != ll_sap_psoc_obj->avoid_freq.stored_num_ch) {
		ll_sap_debug("src %d old count %d new count %d",
			     csa_src, num_ch,
			     ll_sap_psoc_obj->avoid_freq.stored_num_ch);
		ll_lt_sap_dump_stored_freq_list(ll_sap_psoc_obj);
	}
}

static
void ll_lt_sap_update_mac_freq(struct wlan_ll_lt_sap_mac_freq *freq_list,
			       qdf_freq_t sbs_cut_off_freq,
			       qdf_freq_t given_freq, uint32_t given_weight)
{
	if (WLAN_REG_IS_5GHZ_CH_FREQ(given_freq) &&
	    ((sbs_cut_off_freq && given_freq < sbs_cut_off_freq) ||
	    !sbs_cut_off_freq) && !freq_list->freq_5GHz_low) {
		freq_list->freq_5GHz_low = given_freq;
		freq_list->weight_5GHz_low = given_weight;

		/*
		 * Update same freq in 5GHz high freq if sbs_cut_off_freq
		 * is not present
		 */
		if (!sbs_cut_off_freq) {
			freq_list->freq_5GHz_high = given_freq;
			freq_list->weight_5GHz_high = given_weight;
		}
	} else if (WLAN_REG_IS_5GHZ_CH_FREQ(given_freq) &&
		   ((sbs_cut_off_freq && given_freq > sbs_cut_off_freq) ||
		   !sbs_cut_off_freq) && !freq_list->freq_5GHz_high) {
			freq_list->freq_5GHz_high = given_freq;
			freq_list->weight_5GHz_high = given_weight;

		/*
		 * Update same freq for 5GHz low freq if sbs_cut_off_freq
		 * is not present
		 */
		if (!sbs_cut_off_freq) {
			freq_list->freq_5GHz_low = given_freq;
			freq_list->weight_5GHz_low = given_weight;
		}
	} else if (WLAN_REG_IS_6GHZ_CHAN_FREQ(given_freq) &&
		   !freq_list->freq_6GHz) {
		freq_list->freq_6GHz = given_freq;
		freq_list->weight_6GHz = given_weight;
	}
}

/* Threshold value of channel weight */
#define CHANNEL_WEIGHT_THRESHOLD_VALUE 20000
static
void ll_lt_sap_update_freq_list(struct wlan_objmgr_psoc *psoc,
				struct sap_sel_ch_info *ch_param,
				struct wlan_ll_lt_sap_freq_list *freq_list,
				struct policy_mgr_pcl_list *pcl,
				struct connection_info *info,
				uint8_t connection_count,
				uint8_t vdev_id)
{
	qdf_freq_t freq, sbs_cut_off_freq;
	qdf_freq_t same_mac_freq, standalone_mac_freq;
	uint8_t i = 0, count;
	uint32_t weight;
	enum policy_mgr_con_mode con_mode = PM_MAX_NUM_OF_MODE;

	sbs_cut_off_freq = policy_mgr_get_sbs_cut_off_freq(psoc);

	for (i = 0; i < ch_param->num_ch; i++) {
		if (!ch_param->ch_info[i].valid)
			continue;

		freq = ch_param->ch_info[i].chan_freq;
		weight = ch_param->ch_info[i].weight;

		/*
		 * Do not select same channel where LL_LT_SAP was
		 * present earlier
		 */
		if (freq_list->prev_freq == freq)
			continue;

		/* Check if channel is present in PCL or not */
		if (!wlan_ll_sap_freq_present_in_pcl(pcl, freq))
			continue;

		/*
		 * Store first valid best channel from ACS final list
		 * This will be used if there is no valid freq present
		 * within threshold value or no concurrency present
		 */
		if (!freq_list->best_freq) {
			freq_list->best_freq = freq;
			freq_list->weight_best_freq = weight;
		}

		if (!connection_count)
			break;

		/*
		 * Instead of selecting random channel, select those
		 * channel whose weight is less than
		 * CHANNEL_WEIGHT_THRESHOLD_VALUE value. This will help
		 * to get the best channel compartively and avoid multiple
		 * times of channel switch.
		 */
		if (weight > CHANNEL_WEIGHT_THRESHOLD_VALUE)
			continue;

		same_mac_freq = 0;
		standalone_mac_freq = 0;

		/*
		 * Loop through all existing connections before updating
		 * channels for LL_LT_SAP.
		 */
		for (count = 0; count < connection_count; count++) {
			/* Do not select SCC channel to update freq_list */
			if (freq == info[count].ch_freq) {
				same_mac_freq = 0;
				standalone_mac_freq = 0;
				break;
			}
			/*
			 * Check whether ch_param frequency is in same mac
			 * or not.
			 */
			if (policy_mgr_2_freq_always_on_same_mac(
							psoc, freq,
							info[count].ch_freq)) {
				con_mode = policy_mgr_get_mode_by_vdev_id(
						psoc, info[count].vdev_id);
				/*
				 * Check whether SAP is present in same mac or
				 * not. If yes then do not select that frequency
				 * because two beacon entity can't be in same
				 * mac.
				 * Also, do not fill same_mac_freq if it's
				 * already filled i.e two existing connection
				 * are present on same mac.
				 */
				if (con_mode == PM_SAP_MODE || same_mac_freq) {
					same_mac_freq = 0;
					standalone_mac_freq = 0;
					break;
				}
				same_mac_freq = freq;
			} else if (!standalone_mac_freq) {
				/*
				 * Fill standalone_mac_freq only if it's not
				 * filled
				 */
				standalone_mac_freq = freq;
			}
		}

		/*
		 * Scenario: Let say two concurrent connection(other than
		 * LL_LT_SAP) are present in both mac and one channel from
		 * ch_param structure needs to select to fill in freq_list for
		 * LL_LT_SAP.
		 * Since, there is an existing connection present in both mac
		 * then there is chance that channel from ch_param structure
		 * may get select for both same_mac_freq and standalone_mac_freq
		 *
		 * But ideally standalone_mac_freq is not free, some existing
		 * connection is already present in it.
		 * So, instead of giving priority to fill standalone_mac_freq
		 * first, fill same_mac_freq first.
		 *
		 * Example: Let say 2 concurrent interface present in channel
		 * 36 and 149. Now channel 48 from ch_param structure needs to
		 * be validate and fill based on existing interface.
		 * With respect to channel 36, it will fit to same_mac_freq
		 * and with respect to channel 149, it will fit to
		 * standalone_mac_freq. But in standalone_mac_freq, there is
		 * already existing interface present. So, give priority to
		 * same_mac_freq to fill the freq list.
		 */
		if (same_mac_freq)
			ll_lt_sap_update_mac_freq(&freq_list->shared_mac,
						  sbs_cut_off_freq,
						  same_mac_freq,
						  weight);
		else if (standalone_mac_freq)
			ll_lt_sap_update_mac_freq(&freq_list->standalone_mac,
						  sbs_cut_off_freq,
						  standalone_mac_freq,
						  weight);

		if (freq_list->shared_mac.freq_5GHz_low &&
		    freq_list->shared_mac.freq_5GHz_high &&
		    freq_list->shared_mac.freq_6GHz &&
		    freq_list->standalone_mac.freq_5GHz_low &&
		    freq_list->standalone_mac.freq_5GHz_high &&
		    freq_list->standalone_mac.freq_6GHz)
			break;
	}

	ll_sap_debug("vdev %d, best %d[%d] Shared: low %d[%d] high %d[%d] 6Ghz %d[%d], Standalone: low %d[%d] high %d[%d] 6Ghz %d[%d], prev %d, existing connection cnt %d",
		     vdev_id, freq_list->best_freq,
		     freq_list->weight_best_freq,
		     freq_list->shared_mac.freq_5GHz_low,
		     freq_list->shared_mac.weight_5GHz_low,
		     freq_list->shared_mac.freq_5GHz_high,
		     freq_list->shared_mac.weight_5GHz_high,
		     freq_list->shared_mac.freq_6GHz,
		     freq_list->shared_mac.weight_6GHz,
		     freq_list->standalone_mac.freq_5GHz_low,
		     freq_list->standalone_mac.weight_5GHz_low,
		     freq_list->standalone_mac.freq_5GHz_high,
		     freq_list->standalone_mac.weight_5GHz_high,
		     freq_list->standalone_mac.freq_6GHz,
		     freq_list->standalone_mac.weight_6GHz,
		     freq_list->prev_freq, connection_count);
}

static
QDF_STATUS ll_lt_sap_get_allowed_freq_list(
				struct wlan_objmgr_psoc *psoc,
				uint8_t vdev_id,
				struct sap_sel_ch_info *ch_param,
				struct wlan_ll_lt_sap_freq_list *freq_list,
				enum ll_sap_csa_source csa_src)
{
	struct connection_info conn_info[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t count;
	struct policy_mgr_pcl_list *pcl;
	QDF_STATUS status;

	pcl = qdf_mem_malloc(sizeof(*pcl));
	if (!pcl)
		return QDF_STATUS_E_FAILURE;

	ll_lt_store_to_avoid_list_and_flush_old(psoc, freq_list->prev_freq,
						csa_src);

	status = policy_mgr_get_pcl_ch_list_for_ll_sap(psoc, pcl, vdev_id,
						       conn_info, &count);
	if (QDF_IS_STATUS_ERROR(status))
		goto end;

	ll_lt_sap_update_freq_list(psoc, ch_param, freq_list, pcl,
				   conn_info, count, vdev_id);

	status = QDF_STATUS_SUCCESS;
end:
	qdf_mem_free(pcl);
	return status;
}

QDF_STATUS ll_lt_sap_get_freq_list(struct wlan_objmgr_psoc *psoc,
				   struct wlan_ll_lt_sap_freq_list *freq_list,
				   uint8_t vdev_id,
				   enum ll_sap_csa_source csa_src)
{
	struct sap_sel_ch_info ch_param = { NULL, 0 };
	QDF_STATUS status;

	/*
	 * This memory will be allocated in sap_chan_sel_init() as part
	 * of sap_sort_channel_list(). But the caller has to free up the
	 * allocated memory
	 */

	status = ll_lt_sap_get_sorted_user_config_acs_ch_list(psoc, vdev_id,
							      &ch_param);

	if (!ch_param.num_ch || QDF_IS_STATUS_ERROR(status))
		goto release_mem;

	status = ll_lt_sap_get_allowed_freq_list(psoc, vdev_id, &ch_param,
						 freq_list, csa_src);

release_mem:
	wlan_ll_sap_free_chan_info(&ch_param);
	return status;
}

qdf_freq_t ll_lt_sap_get_valid_freq(struct wlan_objmgr_psoc *psoc,
				    uint8_t vdev_id, qdf_freq_t curr_freq,
				    enum ll_sap_csa_source csa_src)
{
	struct wlan_ll_lt_sap_freq_list freq_list;

	qdf_mem_zero(&freq_list, sizeof(freq_list));

	freq_list.prev_freq = curr_freq;
	ll_lt_sap_get_freq_list(psoc, &freq_list, vdev_id, csa_src);

	if (freq_list.standalone_mac.freq_5GHz_low)
		return freq_list.standalone_mac.freq_5GHz_low;
	if (freq_list.shared_mac.freq_5GHz_low)
		return freq_list.shared_mac.freq_5GHz_low;
	if (freq_list.standalone_mac.freq_6GHz)
		return freq_list.standalone_mac.freq_6GHz;
	if (freq_list.standalone_mac.freq_5GHz_high)
		return freq_list.standalone_mac.freq_5GHz_high;
	if (freq_list.shared_mac.freq_6GHz)
		return freq_list.shared_mac.freq_6GHz;
	if (freq_list.shared_mac.freq_5GHz_high)
		return freq_list.shared_mac.freq_5GHz_high;

	return freq_list.best_freq;
}

QDF_STATUS
ll_lt_sap_high_ap_availability(struct wlan_objmgr_vdev *vdev,
			       enum high_ap_availability_operation operation,
			       uint32_t duration, uint16_t cookie)
{
	struct ll_sap_vdev_priv_obj *ll_sap_obj;
	uint8_t i;
	uint16_t new_cookie;
	struct ll_sap_ops *osif_cbk;
	struct wlan_objmgr_psoc *psoc;
	struct ll_sap_psoc_priv_obj *psoc_ll_sap_obj;
	struct ll_sap_oob_connect_request req;
	struct wlan_ll_sap_tx_ops *tx_ops;

	if (operation == HiGH_AP_AVAILABILITY_OPERATION_INVALID) {
		ll_sap_err("Invalid operation value received");
		return QDF_STATUS_E_INVAL;
	}

	ll_sap_obj = ll_sap_get_vdev_priv_obj(vdev);

	if (!ll_sap_obj) {
		ll_sap_err("vdev %d ll_sap obj null",
			   wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_INVAL;
	}

	psoc = wlan_vdev_get_psoc(vdev);
	psoc_ll_sap_obj = wlan_objmgr_psoc_get_comp_private_obj(
							psoc,
							WLAN_UMAC_COMP_LL_SAP);
	if (!psoc_ll_sap_obj) {
		ll_sap_err("psoc_ll_sap_obj is null");
		return QDF_STATUS_E_INVAL;
	}

	/* Send this request to FW */
	req.connect_req_type = operation;
	req.vdev_available_duration = duration;
	req.vdev_id = wlan_vdev_get_id(vdev);

	tx_ops = &psoc_ll_sap_obj->tx_ops;
	if (!tx_ops->send_oob_connect_request) {
		ll_sap_err("send_oob_connect_request op is NULL");
		return QDF_STATUS_E_INVAL;
	}

	tx_ops->send_oob_connect_request(psoc, req);

	/*
	 * If operation is HIGH_AP_AVAILABILITY_OPERATION_REQUEST, means this
	 * is the first request, assign the cookie for this request and send
	 * this cookie to userspace
	 */
	if (operation == HIGH_AP_AVAILABILITY_OPERATION_REQUEST) {
		for (i = 0; i < MAX_HIGH_AP_AVAILABILITY_REQUESTS; i++) {
			if (ll_sap_obj->high_ap_availability_cookie[i] ==
							LL_SAP_INVALID_COOKIE)
				break;
		}
		if (i == MAX_HIGH_AP_AVAILABILITY_REQUESTS) {
			ll_sap_debug("Invalid high AP availability request");
			return QDF_STATUS_E_INVAL;
		}
		qdf_get_random_bytes(&new_cookie, sizeof(new_cookie));
		ll_sap_debug("LL_LT_SAP vdev %d high ap availability cookie %d",
			     wlan_vdev_get_id(vdev), new_cookie);
		ll_sap_obj->high_ap_availability_cookie[i] = new_cookie;

		/* Send event to userspace with cookie */
		osif_cbk = ll_sap_get_osif_cbk();
		if (osif_cbk &&
		    osif_cbk->ll_sap_send_high_ap_availability_resp_cb)
			osif_cbk->ll_sap_send_high_ap_availability_resp_cb(
						vdev, operation, new_cookie);
	}
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS ll_lt_sap_get_tsf_stats_for_csa(
				struct wlan_objmgr_psoc *psoc,
				uint8_t vdev_id)
{
	struct ll_sap_psoc_priv_obj *psoc_ll_sap_obj;
	struct wlan_ll_sap_tx_ops *tx_ops;

	psoc_ll_sap_obj = wlan_objmgr_psoc_get_comp_private_obj(
						psoc,
						WLAN_UMAC_COMP_LL_SAP);
	if (!psoc_ll_sap_obj) {
		ll_sap_err("psoc_ll_sap_obj is null");
		return QDF_STATUS_E_INVAL;
	}

	tx_ops = &psoc_ll_sap_obj->tx_ops;
	if (!tx_ops->get_tsf_stats_for_csa) {
		ll_sap_err("get_tsf_stats_for_csa op is NULL");
		return QDF_STATUS_E_INVAL;
	}

	return tx_ops->get_tsf_stats_for_csa(psoc, vdev_id);
}

/**
 * ll_lt_sap_get_vdev_peer_entries() - Get vdev peer entries
 * @vdev: vdev object
 * @object: object pointer
 * @arg: argument pointer
 *
 * Return: None
 */
static void ll_lt_sap_get_vdev_peer_entries(struct wlan_objmgr_vdev *vdev,
					    void *object, void *arg)
{
	struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)object;
	struct ll_sap_vdev_peer_entry *peer_entry =
				(struct ll_sap_vdev_peer_entry *)arg;

	if (wlan_peer_get_peer_type(peer) != WLAN_PEER_STA)
		return;

	peer_entry->num_peer++;
	qdf_mem_copy(peer_entry->macaddr[peer_entry->num_peer].bytes,
		     peer->macaddr, QDF_MAC_ADDR_SIZE);
}

/**
 * ll_lt_sap_sent_ecsa_and_vdev_restart() - Send ecsa and vdev rstart for
 * LL_LT_SAP
 * @psoc: pointer to psoc object
 * @vdev: pointer to vdev object
 * @twt_target_tsf: TWT target tsf
 *
 * Return: QDF_STATUS
 */
static
QDF_STATUS ll_lt_sap_sent_ecsa_and_vdev_restart(struct wlan_objmgr_psoc *psoc,
						struct wlan_objmgr_vdev *vdev,
						uint64_t twt_target_tsf)
{
	struct ll_sap_vdev_peer_entry peer_entry;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	int i;

	wlan_ll_sap_notify_chan_switch_started(vdev);
	peer_entry.num_peer = 0;
	wlan_objmgr_iterate_peerobj_list(vdev, ll_lt_sap_get_vdev_peer_entries,
					 &peer_entry, WLAN_LL_SAP_ID);

	for (i = 1; i <= peer_entry.num_peer; i++) {
		if (wlan_is_twt_session_present(
				psoc, peer_entry.macaddr[i].bytes) &&
				twt_target_tsf)
			wlan_ll_sap_send_action_frame(
				vdev, peer_entry.macaddr[i].bytes);
	}

	wlan_ll_sap_send_continue_vdev_restart(vdev);

	return status;
}

#define NON_TWT_TSF 500000
/**
 * ll_lt_sap_calculate_target_tsf() - Calculate target_tsf
 * @psoc: pointer to psoc object
 * @vdev_id: vdev_id
 * @peer_mac: peer macaddr for twt_stats received from firmware
 * @tsf: next_sp_start_tsf or curr_tsf passed by caller
 * @dialog_id: dialog_id of peer
 * @get_twt_target_tsf: flag to check whether to calculate
 * tsf based on twt or non twt
 *
 * Return: uint64_t
 */
static
uint64_t ll_lt_sap_calculate_target_tsf(struct wlan_objmgr_psoc *psoc,
					uint8_t vdev_id,
					struct qdf_mac_addr peer_mac,
					uint32_t *dialog_id, uint64_t tsf,
					bool get_twt_target_tsf)
{
	uint32_t wake_dur = 0, wake_interval = 0;
	uint64_t target_tsf;

	if (get_twt_target_tsf) {
		wlan_twt_get_wake_dur_and_interval(psoc, vdev_id, &peer_mac,
						   dialog_id, &wake_dur,
						   &wake_interval);
		/* target_tsf will be next_sp_start_tsf + 2TWT SI */
		target_tsf = tsf + (2 * wake_interval);
	} else {
		/*
		 * TWT session is not present with any EB
		 * target_tsf will be curr_tsf + 500ms
		 */
		target_tsf = tsf + NON_TWT_TSF;
	}

	return target_tsf;
}

/**
 * ll_lt_sap_obtain_target_tsf() - Obtain target tsf to perform CSA
 * @psoc: psoc object
 * @twt_params: twt params
 * @twt_target_tsf: twt target tsf to fill
 * @non_twt_target_tsf: non twt target tsf to fill
 *
 * Return: QDF_STATUS
 */
static
QDF_STATUS ll_lt_sap_obtain_target_tsf(
				struct wlan_objmgr_psoc *psoc,
				struct twt_session_stats_info *twt_params,
				uint64_t *twt_target_tsf,
				uint64_t *non_twt_target_tsf)
{
	uint64_t next_sp_start_tsf = 0, curr_tsf = 0;

	next_sp_start_tsf = (uint64_t)twt_params->sp_tsf_us_hi << 32 |
				twt_params->sp_tsf_us_lo;

	curr_tsf = ((uint64_t)twt_params->curr_tsf_us_hi << 32 |
			twt_params->curr_tsf_us_lo);

	if (next_sp_start_tsf && !ll_lt_sap_get_bearer_switch_cap_for_csa(psoc))
		/* TWT session is present with atleast one EB */
		*twt_target_tsf = ll_lt_sap_calculate_target_tsf(
					psoc, twt_params->vdev_id,
					twt_params->peer_mac,
					&twt_params->dialog_id,
					next_sp_start_tsf, true);

	*non_twt_target_tsf = ll_lt_sap_calculate_target_tsf(
				psoc, twt_params->vdev_id,
				twt_params->peer_mac,
				&twt_params->dialog_id,
				curr_tsf, false);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS ll_lt_sap_continue_csa_after_tsf_rsp(struct ll_sap_csa_tsf_rsp *rsp)
{
	struct ll_sap_vdev_priv_obj *ll_sap_vdev_obj;
	struct ll_sap_psoc_priv_obj *psoc_ll_sap_obj;
	struct wlan_objmgr_vdev *vdev;
	uint64_t twt_target_tsf = 0;
	uint64_t non_twt_target_tsf = 0;

	if (!rsp) {
		ll_sap_err("rsp is null");
		return QDF_STATUS_E_FAILURE;
	}

	psoc_ll_sap_obj = wlan_objmgr_psoc_get_comp_private_obj(
							rsp->psoc,
							WLAN_UMAC_COMP_LL_SAP);
	if (!psoc_ll_sap_obj) {
		ll_sap_err("psoc_ll_sap_obj is null");
		return QDF_STATUS_E_FAILURE;
	}

	/*
	 * stop target_tsf timer after getting response from firmware
	 */
	if (QDF_TIMER_STATE_RUNNING == qdf_mc_timer_get_current_state(
					&psoc_ll_sap_obj->tsf_timer))
		qdf_mc_timer_stop(&psoc_ll_sap_obj->tsf_timer);

	if (rsp->twt_params.vdev_id == WLAN_INVALID_VDEV_ID) {
		ll_sap_err("Invalid vdev %d", rsp->twt_params.vdev_id);
		return QDF_STATUS_E_INVAL;
	}

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(rsp->psoc,
						    rsp->twt_params.vdev_id,
						    WLAN_LL_SAP_ID);

	if (!vdev) {
		ll_sap_err("vdev %d is null", rsp->twt_params.vdev_id);
		return QDF_STATUS_E_FAILURE;
	}

	ll_sap_vdev_obj = ll_sap_get_vdev_priv_obj(vdev);
	if (!ll_sap_vdev_obj) {
		ll_sap_err("vdev %d ll_sap obj null", rsp->twt_params.vdev_id);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LL_SAP_ID);
		return QDF_STATUS_E_INVAL;
	}

	ll_lt_sap_obtain_target_tsf(rsp->psoc, &rsp->twt_params,
				    &twt_target_tsf, &non_twt_target_tsf);

	/*
	 * Store target_tsf in ll_sap vdev private object. This can be use
	 * to by multiple place to perform ll_sap CSA.
	 */
	ll_sap_vdev_obj->target_tsf.twt_target_tsf = twt_target_tsf;
	ll_sap_vdev_obj->target_tsf.non_twt_target_tsf = non_twt_target_tsf;

	ll_sap_debug("vdev_id %d twt_target_tsf %llu and non_twt_target_tsf %llu",
		     rsp->twt_params.vdev_id,
		     ll_sap_vdev_obj->target_tsf.twt_target_tsf,
		     ll_sap_vdev_obj->target_tsf.non_twt_target_tsf);

	/* send csa param via action frame */
	ll_lt_sap_sent_ecsa_and_vdev_restart(
				rsp->psoc, vdev,
				ll_sap_vdev_obj->target_tsf.twt_target_tsf);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_LL_SAP_ID);

	return QDF_STATUS_SUCCESS;
}
