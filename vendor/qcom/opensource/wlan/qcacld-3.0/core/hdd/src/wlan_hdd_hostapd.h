/*
 * Copyright (c) 2013-2021 The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#if !defined(WLAN_HDD_HOSTAPD_H)
#define WLAN_HDD_HOSTAPD_H

/**
 * DOC: wlan_hdd_hostapd.h
 *
 * WLAN Host Device driver hostapd header file
 */

/* Include files */

#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <qdf_list.h>
#include <qdf_types.h>
#include <wlan_hdd_main.h>

/* Preprocessor definitions and constants */

struct hdd_adapter *hdd_wlan_create_ap_dev(struct hdd_context *hdd_ctx,
				      tSirMacAddr macAddr,
				      unsigned char name_assign_type,
				      uint8_t *name);

enum csr_akm_type
hdd_translate_rsn_to_csr_auth_type(uint8_t auth_suite[4]);

/**
 * hdd_filter_ft_info() -
 * This function to filter fast BSS transition related IE
 * @frame: pointer to the input frame.
 * @len: input frame length.
 * @ft_info_len: store the total length of FT related IE.
 *
 * Return: pointer to a buffer which stored the FT related IE
 * This is a malloced memory that must be freed by the caller
 */

void *hdd_filter_ft_info(const uint8_t *frame,
			 size_t len, uint32_t *ft_info_len);

/**
 * hdd_softap_set_channel_change() -
 * This function to support SAP channel change with CSA IE
 * set in the beacons.
 *
 * @link_info: pointer to hdd link info.
 * @target_chan_freq: target channel frequency.
 * @ccfs1:  Value of CCFS1 in MHz
 * @target_bw: Target bandwidth to move.
 * If no bandwidth is specified, the value is CH_WIDTH_MAX
 * @punct_bitmap: Puncturing bitmap of CSA, follows same convention as
 * Disabled Subchannel Bitmap in 802.11be EHT-OP IE
 * @forced: Force to switch channel, ignore SCC/MCC check
 * @allow_blocking: the calling thread allows be blocked
 *
 * Return: 0 for success, non zero for failure
 */
int hdd_softap_set_channel_change(struct wlan_hdd_link_info *link_info,
				  int target_chan_freq, uint32_t ccfs1,
				  enum phy_ch_width target_bw,
				  uint32_t punct_bitmap,
				  bool forced,
				  bool allow_blocking);
/**
 * hdd_stop_sap_set_tx_power() - Function to set tx power
 * for unsafe channel if restriction bit mask is set else stop the SAP.
 * @psoc: PSOC object information
 * @link_info: pointer of link info
 *
 * This function set tx power/stop the SAP interface
 *
 * Return:
 *
 */
void hdd_stop_sap_set_tx_power(struct wlan_objmgr_psoc *psoc,
			       struct wlan_hdd_link_info *link_info);

#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH
/**
 * hdd_sap_restart_with_channel_switch() - SAP channel change with E/CSA
 * @psoc: psoc common object
 * @link_info: hdd link info
 * @target_chan_freq: Channel frequency to which switch must happen
 * @target_bw: Bandwidth of the target channel
 * @forced: Force to switch channel, ignore SCC/MCC check
 *
 * Invokes the necessary API to perform channel switch for the SAP or GO
 *
 * Return: QDF_STATUS_SUCCESS if successfully
 */
QDF_STATUS hdd_sap_restart_with_channel_switch(struct wlan_objmgr_psoc *psoc,
					struct wlan_hdd_link_info *link_info,
					uint32_t target_chan_freq,
					uint32_t target_bw,
					bool forced);

/**
 * hdd_sap_restart_chan_switch_cb() - Function to restart SAP with
 * a different channel
 * @psoc: PSOC object information
 * @vdev_id: vdev id
 * @ch_freq: channel to switch
 * @channel_bw: channel bandwidth
 * @forced: Force to switch channel, ignore SCC/MCC check
 *
 * This function restarts SAP with a different channel
 *
 * Return: QDF_STATUS_SUCCESS if successfully
 *
 */
QDF_STATUS hdd_sap_restart_chan_switch_cb(struct wlan_objmgr_psoc *psoc,
					  uint8_t vdev_id, uint32_t ch_freq,
					  uint32_t channel_bw, bool forced);

/**
 * wlan_hdd_check_cc_intf_cb() - Check force SCC for vdev interface.
 * @psoc: PSOC object information
 * @vdev_id: vdev id
 * @ch_freq: channel frequency to switch to
 *
 * This function will return a channel frequency to avoid MCC for SAP/GO.
 *
 * Return: QDF_STATUS_SUCCESS if successfully
 *
 */
QDF_STATUS wlan_hdd_check_cc_intf_cb(struct wlan_objmgr_psoc *psoc,
				     uint8_t vdev_id, uint32_t *ch_freq);

/**
 * wlan_hdd_get_channel_for_sap_restart() - Function to get
 * suitable channel and restart SAP
 * @psoc: PSOC object information
 * @vdev_id: vdev id
 * @ch_freq: channel to be returned
 *
 * This function gets the channel parameters to restart SAP
 *
 * Return: None
 *
 */
QDF_STATUS wlan_hdd_get_channel_for_sap_restart(
				struct wlan_objmgr_psoc *psoc,
				uint8_t vdev_id, uint32_t *ch_freq);

/**
 * wlan_get_sap_acs_band() - Get  sap acs band
 *
 * @psoc: pointer to psoc
 * @vdev_id: vdev id
 * @acs_band: Pointer to acs_band
 *
 * This function is used to get sap acs band from sap config
 *
 * Return: QDF_STATUS_SUCCESS if successful
 */
uint32_t
wlan_get_sap_acs_band(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id,
		      uint32_t *acs_band);

/**
 * wlan_get_ap_prefer_conc_ch_params() - Get prefer sap target channel
 *  bw parameters
 * @psoc: pointer to psoc
 * @vdev_id: vdev id
 * @chan_freq: sap channel
 * @ch_params: output channel parameters
 *
 * This function is used to get prefer sap target channel bw during sap force
 * scc CSA. The new bw will not exceed the original bw during start ap
 * request.
 *
 * Return: QDF_STATUS_SUCCESS if successfully
 */
QDF_STATUS
wlan_get_ap_prefer_conc_ch_params(
		struct wlan_objmgr_psoc *psoc,
		uint8_t vdev_id, uint32_t chan_freq,
		struct ch_params *ch_params);

/**
 * hdd_get_ap_6ghz_capable() - Get ap vdev 6ghz capable flags
 * @psoc: PSOC object information
 * @vdev_id: vdev id
 *
 * This function gets 6ghz capable information based on hdd ap adapter
 * context.
 *
 * Return: uint32_t, vdev 6g capable flags from enum conn_6ghz_flag
 */
uint32_t hdd_get_ap_6ghz_capable(struct wlan_objmgr_psoc *psoc,
				 uint8_t vdev_id);
#endif

/**
 * wlan_hdd_set_sap_csa_reason() - Function to set
 * sap csa reason
 * @psoc: PSOC object information
 * @vdev_id: vdev id
 * @reason: reason to be updated
 *
 * This function sets the reason for SAP channel switch
 *
 * Return: None
 *
 */
void wlan_hdd_set_sap_csa_reason(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id,
				 uint8_t reason);
eCsrEncryptionType
hdd_translate_rsn_to_csr_encryption_type(uint8_t cipher_suite[4]);

eCsrEncryptionType
hdd_translate_rsn_to_csr_encryption_type(uint8_t cipher_suite[4]);

enum csr_akm_type
hdd_translate_wpa_to_csr_auth_type(uint8_t auth_suite[4]);

eCsrEncryptionType
hdd_translate_wpa_to_csr_encryption_type(uint8_t cipher_suite[4]);

/**
 * hdd_softap_sta_deauth() - handle deauth req from HDD
 * @link_info: Pointer to hdd link info
 * @param: Params to the operation
 *
 * This to take counter measure to handle deauth req from HDD
 *
 * Return: QDF_STATUS
 */
QDF_STATUS hdd_softap_sta_deauth(struct wlan_hdd_link_info *link_info,
				 struct csr_del_sta_params *param);
void hdd_softap_sta_disassoc(struct hdd_adapter *adapter,
			     struct csr_del_sta_params *param);

/**
 * hdd_hostapd_sap_event_cb() - callback to process sap event
 * @sap_ctx: SAP context
 * @sap_event: SAP event buffer
 *
 * Function for HDD to process event notification from sap
 * module
 * return: QDF_STATUS
 */
QDF_STATUS hdd_hostapd_sap_event_cb(struct sap_context *sap_ctx,
				    struct sap_event *sap_event);
/**
 * hdd_init_ap_mode() - to init the AP adaptor
 * @link_info: pointer of link_info
 * @reinit: true if re-init, otherwise initial init
 *
 * This API can be called to open the SAP session as well as
 * to create and store the vdev object. It also initializes necessary
 * SAP adapter related params.
 */
QDF_STATUS hdd_init_ap_mode(struct wlan_hdd_link_info *link_info,
			    bool reinit);

/**
 * hdd_indicate_peers_deleted() - indicate peer delete for vdev
 * @psoc: PSOC object information
 * @vdev_id: vdev id
 *
 * This is callback for PE to call deauth from HDD layer.
 * return: void
 */
void
hdd_indicate_peers_deleted(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id);

/**
 * hdd_deinit_ap_mode() - to deinit the AP adaptor
 * @link_info: Link info pointer in HDD adapter
 *
 * This API can be called to close the SAP session as well as
 * release the vdev object completely. It also deinitializes necessary
 * SAP adapter related params.
 */
void hdd_deinit_ap_mode(struct wlan_hdd_link_info *link_info);

void hdd_set_ap_ops(struct net_device *dev);
/**
 * hdd_sap_create_ctx() - Wrapper API to create SAP context
 * @link_info: pointer to link info
 *
 * This wrapper API can be called to create the sap context. It will
 * eventually calls SAP API to create the sap context
 *
 * Return: true or false based on overall success or failure
 */
bool hdd_sap_create_ctx(struct wlan_hdd_link_info *link_info);
/**
 * hdd_sap_destroy_ctx() - Wrapper API to destroy SAP context
 * @link_info: Pointer of link_info in adapter
 *
 * This wrapper API can be called to destroy the sap context. It will
 * eventually calls SAP API to destroy the sap context
 *
 * Return: true or false based on overall success or failure
 */
bool hdd_sap_destroy_ctx(struct wlan_hdd_link_info *link_info);
/**
 * hdd_sap_destroy_ctx_all() - Wrapper API to destroy all SAP context
 * @hdd_ctx: pointer to HDD context
 * @is_ssr: true if SSR is in progress
 *
 * This wrapper API can be called to destroy all the sap context.
 * if is_ssr is true, it will return as sap_ctx will be used when
 * restart sap.
 *
 * Return: none
 */
void hdd_sap_destroy_ctx_all(struct hdd_context *hdd_ctx, bool is_ssr);

/**
 * hdd_hostapd_stop_no_trans() - hdd stop function for hostapd interface
 * @dev: pointer to net_device structure
 *
 * This is called in response to ifconfig down. Vdev sync transaction
 * should be started before calling this API.
 *
 * Return - 0 for success non-zero for failure
 */
int hdd_hostapd_stop_no_trans(struct net_device *dev);

int hdd_hostapd_stop(struct net_device *dev);
int hdd_sap_context_init(struct hdd_context *hdd_ctx);
void hdd_sap_context_destroy(struct hdd_context *hdd_ctx);
#ifdef QCA_HT_2040_COEX
/**
 * hdd_set_sap_ht2040_mode() - set ht2040 mode
 * @link_info: pointer to link_info
 * @channel_type: given channel type
 *
 * Return: QDF_STATUS_SUCCESS if successfully
 */
QDF_STATUS hdd_set_sap_ht2040_mode(struct wlan_hdd_link_info *link_info,
				   uint8_t channel_type);

/**
 * hdd_get_sap_ht2040_mode() - get ht2040 mode
 * @link_info: pointer to link_info
 * @channel_type: given channel type
 *
 * Return: QDF_STATUS_SUCCESS if successfully
 */
QDF_STATUS hdd_get_sap_ht2040_mode(struct wlan_hdd_link_info *link_info,
				   enum eSirMacHTChannelType *channel_type);
#else
static inline QDF_STATUS
hdd_set_sap_ht2040_mode(struct wlan_hdd_link_info *link_info,
			uint8_t channel_type)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS hdd_get_sap_ht2040_mode(
				struct wlan_hdd_link_info *link_info,
				enum eSirMacHTChannelType *channel_type)
{
	return QDF_STATUS_E_FAILURE;
}
#endif

#ifdef CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT
/**
 * wlan_hdd_cfg80211_stop_ap() - stop sap
 * @wiphy: Pointer to wiphy
 * @dev: Pointer to netdev
 * @link_id: Link id for which this stop_ap is received.
 *
 * Return: zero for success non-zero for failure
 */
int wlan_hdd_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *dev,
			      unsigned int link_id);
#else
int wlan_hdd_cfg80211_stop_ap(struct wiphy *wiphy,
			      struct net_device *dev);
#endif

int wlan_hdd_cfg80211_start_ap(struct wiphy *wiphy,
			       struct net_device *dev,
			       struct cfg80211_ap_settings *params);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 7, 0)
int wlan_hdd_cfg80211_change_beacon(struct wiphy *wiphy,
				    struct net_device *dev,
				    struct cfg80211_ap_update *params);
#else
int wlan_hdd_cfg80211_change_beacon(struct wiphy *wiphy,
				    struct net_device *dev,
				    struct cfg80211_beacon_data *params);
#endif
/**
 * hdd_is_peer_associated - is peer connected to softap
 * @adapter: pointer to softap adapter
 * @mac_addr: address to check in peer list
 *
 * This function has to be invoked only when bss is started and is used
 * to check whether station with specified addr is peer or not
 *
 * Return: true if peer mac, else false
 */
bool hdd_is_peer_associated(struct hdd_adapter *adapter,
			    struct qdf_mac_addr *mac_addr);

int hdd_destroy_acs_timer(struct hdd_adapter *adapter);

QDF_STATUS wlan_hdd_config_acs(struct hdd_context *hdd_ctx,
			       struct hdd_adapter *adapter);

void hdd_sap_indicate_disconnect_for_sta(struct hdd_adapter *adapter);

/**
 * hdd_handle_acs_2g_preferred_sap_conc() - Handle 2G pereferred SAP
 * concurrency with GO
 * @psoc: soc object
 * @adapter: HDD adapter context
 * @sap_config: sap config
 *
 * In SAP+GO concurrency, if GO is started on 2G and SAP is
 * doing ACS with 2G preferred channel list, then we will
 * move GO to 5G band. The purpose is to have more choice
 * in SAP ACS instead of starting on GO home channel for SCC.
 * This API is to check such condition and move GO to 5G.
 *
 * Return: void
 */
void
hdd_handle_acs_2g_preferred_sap_conc(struct wlan_objmgr_psoc *psoc,
				     struct hdd_adapter *adapter,
				     struct sap_config *sap_config);

/**
 * wlan_hdd_disable_channels() - Cache the channels
 * and current state of the channels from the channel list
 * received in the command and disable the channels on the
 * wiphy and reg table.
 * @hdd_ctx: Pointer to hdd context
 *
 * Return: 0 on success, Error code on failure
 */
int wlan_hdd_disable_channels(struct hdd_context *hdd_ctx);

/*
 * hdd_check_and_disconnect_sta_on_invalid_channel() - Disconnect STA if it is
 * on invalid channel
 * @hdd_ctx: pointer to hdd context
 * @reason: Mac Disconnect reason code as per @enum wlan_reason_code
 *
 * STA should be disconnected before starting the SAP if it is on indoor
 * channel.
 *
 * Return: void
 */
void
hdd_check_and_disconnect_sta_on_invalid_channel(struct hdd_context *hdd_ctx,
						enum wlan_reason_code reason);

/**
 * hdd_convert_dot11mode_from_phymode() - get dot11 mode from phymode
 * @phymode: phymode of sta associated to SAP
 *
 * The function is to convert the phymode to corresponding dot11 mode
 *
 * Return: dot11mode.
 */
enum qca_wlan_802_11_mode hdd_convert_dot11mode_from_phymode(int phymode);

/**
 * hdd_stop_sap_due_to_invalid_channel() - to stop sap in case of invalid chnl
 * @work: pointer to work structure
 *
 * Let's say SAP detected RADAR and trying to select the new channel and if no
 * valid channel is found due to none of the channels are available or
 * regulatory restriction then SAP needs to be stopped. so SAP state-machine
 * will create a work to stop the bss
 *
 * stop bss has to happen through worker thread because radar indication comes
 * from FW through mc thread or main host thread and if same thread is used to
 * do stopbss then waiting for stopbss to finish operation will halt mc thread
 * to freeze which will trigger stopbss timeout. Instead worker thread can do
 * the stopbss operation while mc thread waits for stopbss to finish.
 *
 * Return: none
 */
void hdd_stop_sap_due_to_invalid_channel(struct work_struct *work);

/**
 * hdd_is_sta_connect_or_link_switch_in_prog() - check if any sta is connecting
 * or in the middle of a link switch
 * @hdd_ctx: hdd context
 *
 * Return: true if any sta is connecting/in link switch
 */
bool hdd_is_sta_connect_or_link_switch_in_prog(struct hdd_context *hdd_ctx);

/**
 * wlan_hdd_configure_twt_responder() - configure twt responder in sap_config
 * @hdd_ctx: Pointer to hdd context
 * @twt_responder: twt responder configure value
 * @vdev_id: Vdev id
 *
 * Return: none
 */
void
wlan_hdd_configure_twt_responder(struct hdd_context *hdd_ctx,
				 bool twt_responder, uint8_t vdev_id);
#ifdef WLAN_FEATURE_11BE_MLO
#ifdef WLAN_FEATURE_MULTI_LINK_SAP
/**
 * hdd_multi_link_sap_vdev_attach() - attach vdev with link_id
 * update multi link parameter to vdev and sap_config.
 * @link_info: Pointer to wlan_hdd_link_info
 * @link_id: link id
 *
 * Return: QDF_STATUS
 */
QDF_STATUS hdd_multi_link_sap_vdev_attach(struct wlan_hdd_link_info *link_info,
					  unsigned int link_id);
#else
static inline QDF_STATUS
hdd_multi_link_sap_vdev_attach(struct wlan_hdd_link_info *link_info,
			       unsigned int link_id)
{
	return QDF_STATUS_SUCCESS;
}
#endif

/**
 * wlan_hdd_mlo_reset() - reset mlo configuration if start bss fails
 * @link_info: Pointer to link_info in hostapd adapter
 *
 * Return: void
 */
void wlan_hdd_mlo_reset(struct wlan_hdd_link_info *link_info);
#else
static inline void wlan_hdd_mlo_reset(struct wlan_hdd_link_info *link_info)
{
}
#endif /* end WLAN_FEATURE_11BE_MLO */

#ifdef WLAN_FEATURE_SAP_ACS_OPTIMIZE
/**
 * hdd_sap_is_acs_in_progress() - API to return if ACS is in progress
 * @vdev: pointer t vdev object
 *
 * Return: bool
 */
bool hdd_sap_is_acs_in_progress(struct wlan_objmgr_vdev *vdev);
#else
static inline
bool hdd_sap_is_acs_in_progress(struct wlan_objmgr_vdev *vdev)
{
	return false;
}
#endif

#ifdef WLAN_FEATURE_MULTI_LINK_SAP
/**
 * hdd_mlosap_check_support_link_num() - mlo sap to check if link
 *                               number exceed max support link number
 * @adapter: pointer to adapter
 * Return: true if not exceed max support num.
 */
bool hdd_mlosap_check_support_link_num(struct hdd_adapter *adapter);
#endif

#ifdef WLAN_CHIPSET_STATS
/*
 * hdd_cp_stats_cstats_sap_go_start_event() - chipset stats for sap/go start
 * event
 *
 * @link_info: pointer to link_info object
 * @sap_event: pointer to sap_event object
 *
 * Return : void
 */
void
hdd_cp_stats_cstats_sap_go_start_event(struct wlan_hdd_link_info *link_info,
				       struct sap_event *sap_event);

/**
 * hdd_cp_stats_cstats_sap_go_stop_event() - chipset stats for sap/go stop event
 *
 * @link_info: pointer to link_info object
 * @sap_event: pointer to sap_event object
 *
 * Return : void
 */
void
hdd_cp_stats_cstats_sap_go_stop_event(struct wlan_hdd_link_info *link_info,
				      struct sap_event *sap_event);

/**
 * hdd_cp_stats_cstats_log_sap_go_sta_disassoc_event() - chipset stats for
 * sap/go STA disconnect event
 *
 * @li: pointer to link_info object
 * @sap_evt: pointer to sap_event object
 *
 * Return : void
 */
void
hdd_cp_stats_cstats_log_sap_go_sta_disassoc_event(struct wlan_hdd_link_info *li,
						  struct sap_event *sap_evt);

/**
 * hdd_cp_stats_cstats_log_sap_go_sta_assoc_reassoc_event() - chipset stats for
 * sap/go STA assoc event
 *
 * @li: pointer to link_info object
 * @sap_evt: pointer to sap_event object
 *
 * Return : void
 */
void
hdd_cp_stats_cstats_log_sap_go_sta_assoc_reassoc_event
		     (struct wlan_hdd_link_info *li, struct sap_event *sap_evt);

/**
 * hdd_cp_stats_cstats_log_sap_go_dfs_event() - chipset stats for
 * sap/go dfs event
 *
 * @li: pointer to link_info object
 * @event_id: eSapHddEvent event
 *
 * Return : void
 */
void hdd_cp_stats_cstats_log_sap_go_dfs_event(struct wlan_hdd_link_info *li,
					      eSapHddEvent event_id);
#else
static inline void
hdd_cp_stats_cstats_sap_go_start_event(struct wlan_hdd_link_info *link_info,
				       struct sap_event *sap_event)
{
}

static inline void
hdd_cp_stats_cstats_sap_go_stop_event(struct wlan_hdd_link_info *link_info,
				      struct sap_event *sap_event)
{
}

static inline void
hdd_cp_stats_cstats_log_sap_go_sta_disassoc_event(struct wlan_hdd_link_info *li,
						  struct sap_event *sap_evt)
{
}

static inline void
hdd_cp_stats_cstats_log_sap_go_sta_assoc_reassoc_event
		     (struct wlan_hdd_link_info *li, struct sap_event *sap_evt)
{
}

static inline void
hdd_cp_stats_cstats_log_sap_go_dfs_event(struct wlan_hdd_link_info *li,
					 eSapHddEvent event_id)
{
}
#endif /* WLAN_CHIPSET_STATS */

#ifdef WLAN_FEATURE_FILS_SK_SAP
void hdd_hlp_work_queue(struct work_struct *work);
void hdd_fils_hlp_rx(uint8_t vdev_id, hdd_cb_handle ctx, qdf_nbuf_t netbuf);
static inline void hdd_fils_hlp_init(struct hdd_context *hdd_ctx)
{
	qdf_spinlock_create(&hdd_ctx->hdd_hlp_data_lock);
	qdf_list_create(&hdd_ctx->hdd_hlp_data_list, 0);
}

static inline void hdd_fils_hlp_deinit(struct hdd_context *hdd_ctx)
{
	qdf_list_destroy(&hdd_ctx->hdd_hlp_data_list);
	qdf_spinlock_destroy(&hdd_ctx->hdd_hlp_data_lock);
}

static inline void hdd_fils_hlp_workqueue_init(struct hdd_context *hdd_ctx)
{
	hdd_debug("HLP Processing WorkQueue Initialised");
	INIT_WORK(&hdd_ctx->hlp_processing_work,
		  hdd_hlp_work_queue);
}
#else
static inline void hdd_fils_hlp_init(struct hdd_context *hdd_ctx)
{}

static inline void hdd_fils_hlp_deinit(struct hdd_context *hdd_ctx)
{}

static inline void hdd_fils_hlp_rx(uint8_t vdev_id, hdd_cb_handle ctx,
				   qdf_nbuf_t netbuf)
{}

static inline void hdd_fils_hlp_workqueue_init(struct hdd_context *hdd_ctx)
{}
#endif

/**
 * hdd_ssr_restart_sap_cac_link() - Whether postpone sap link or not for SSR
 * @adapter: adapter structure
 * @link_info: link info structure
 *
 * This API use to check if the DFS sap link need to be postponed start or not
 * in the SSR case if there is another partner link.
 * And it can cover below cases:
 * 1. If there is only one created/remaining DFS sap link not started, do not postpone.
 * 2. If there is another 6GHz sap link not started, postpone the DFS sap link.
 * 3. If there is another non-6GHz sap link not started, do not postpone.
 *
 * Return: True if need postpone otherwise false.
 */
bool
hdd_ssr_restart_sap_cac_link(struct hdd_adapter *adapter,
			     struct wlan_hdd_link_info *link_info);

#endif /* end #if !defined(WLAN_HDD_HOSTAPD_H) */
