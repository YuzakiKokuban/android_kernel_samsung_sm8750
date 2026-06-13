/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
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

#ifndef __LIM_PROCESS_FILS_H
#define __LIM_PROCESS_FILS_H

#include <cds_api.h>
#include <lim_global.h>
#include <ani_global.h>
#include <lim_ser_des_utils.h>
#include <lim_security_utils.h>

#ifdef WLAN_FEATURE_FILS_SK

/**
 * lim_process_fils_auth_frame2()- This API processes fils data from auth resp
 * @mac_ctx: mac context
 * @session: PE session
 * @rx_auth_frm_body: pointer to auth frame
 *
 * Return: true if fils data needs to be processed else false
 */
bool lim_process_fils_auth_frame2(struct mac_context *mac_ctx,
				struct pe_session *pe_session,
				tSirMacAuthFrameBody * rx_auth_frm_body);

/**
 * lim_add_fils_data_to_auth_frame()- This API adds fils data to auth frame.
 * Following will be added in this.
 *     1. RSNIE
 *     2. SNonce
 *     3. Session
 *     4. Wrapped data
 * @session: PE session
 * @body: pointer to auth frame where data needs to be added
 *
 * Return: None
 */
void lim_add_fils_data_to_auth_frame(struct pe_session *session, uint8_t *body);

#ifdef WLAN_FEATURE_FILS_SK_SAP
/**
 * lim_process_fils_auth_frame1()- This API processes fils data from auth req
 * @mac_ctx: mac context
 * @session: PE session
 * @rx_auth_frm_body: pointer to auth frame
 * @peer_mac_addr: mac address for peer
 *
 * Return: true if FILS data needs to be processed else false
 */
bool lim_process_fils_auth_frame1(struct mac_context *mac_ctx,
				  struct pe_session *pe_session,
				  tSirMacAuthFrameBody *rx_auth_frm_body,
				  tSirMacAddr peer_mac_addr);

/**
 * lim_add_fils_data_to_auth_rsp_frame()- This API adds FILS data to auth frame.
 * Following will be added in this.
 *     1. RSNIE
 *     2. SNonce
 *     3. Session
 *     4. Wrapped data
 *
 * @session: PE session
 * @body: pointer to auth frame where data needs to be added
 * @peer_mac_addr: mac address for peer
 *
 * Return: None
 */
void lim_add_fils_data_to_auth_rsp_frame(struct pe_session *session,
					 uint8_t *body,
					 tSirMacAddr peer_mac_addr);

#else
static inline bool
lim_process_fils_auth_frame1(struct mac_context *mac_ctx,
			     struct pe_session *pe_session,
			     tSirMacAuthFrameBody *rx_auth_frm_body,
			     tSirMacAddr peer_mac_addr)
{
	return true;
}

static inline void
lim_add_fils_data_to_auth_rsp_frame(struct pe_session *session,
				    uint8_t *body,
				    tSirMacAddr peer_mac_addr)
{ }
#endif
/**
 * lim_is_valid_fils_auth_frame()- This API checks whether auth frame is a
 * valid frame.
 * @mac_ctx: mac context
 * @pe_session: pe session pointer
 * @rx_auth_frm_body: pointer to autherntication frame
 *
 * Return: true if frame is valid or fils is disable, false otherwise
 */
bool lim_is_valid_fils_auth_frame(struct mac_context *mac_ctx,
	struct pe_session *pe_session, tSirMacAuthFrameBody *rx_auth_frm_body);

/**
 * lim_create_fils_rik()- This API create rik using rrk coming from
 * supplicant.
 * @rrk: input rrk
 * @rrk_len: rrk length
 * @rik: Created rik
 * @rik_len: rik length to be filled
 *
 * rIK = KDF (K, S), where
 * K = rRK and
 * S = rIK Label + "\0" + cryptosuite + length
 * The rIK Label is the 8-bit ASCII string:
 * Re-authentication Integrity Key@ietf.org
 *
 * Return: QDF_STATUS
 */
QDF_STATUS lim_create_fils_rik(uint8_t *rrk, uint8_t rrk_len,
			       uint8_t *rik, uint32_t *rik_len);

/**
 * lim_update_fils_config()- This API updates fils session info to csr config
 * from join request.
 * @mac_ctx: pointer to mac context
 * @session: PE session
 * @join_req: pointer to join request
 *
 * Return: None
 */
void lim_update_fils_config(struct mac_context *mac_ctx,
			    struct pe_session *session,
			    struct cm_vdev_join_req *join_req);

/**
 * lim_create_fils_auth_data()- This API creates the fils auth data
 * which needs to be sent in auth req.
 * @mac_ctx: mac context
 * @auth_frame: pointer to auth frame
 * @session: PE session
 * @frame_len: Length of FILS frame
 * @peer_mac_addr: Mac Address of Peer Station
 *
 * Return: length of fils data
 */
QDF_STATUS lim_create_fils_auth_data(struct mac_context *mac_ctx,
				     tpSirMacAuthFrameBody auth_frame,
				     struct pe_session *session,
				     uint32_t *frame_len,
				     tSirMacAddr peer_mac_addr);

/**
 * lim_increase_fils_sequence_number: this API increases fils sequence number in
 * the event of resending auth packet
 * @session_entry: pointer to PE session
 *
 * Return: None
 */
static inline void lim_increase_fils_sequence_number(struct pe_session *session_entry)
{
	if (!session_entry->fils_info)
		return;

	if (session_entry->fils_info->is_fils_connection)
		session_entry->fils_info->sequence_number++;
}

/**
 * populate_fils_connect_params() - Populate FILS connect params to join rsp
 * @mac_ctx: Mac context
 * @session: PE session
 * @connect_rsp: connect join rsp
 *
 * This API copies the FILS connect params from PE session to SME join rsp
 *
 * Return: None
 */
void
populate_fils_connect_params(struct mac_context *mac_ctx,
			     struct pe_session *session,
			     struct wlan_cm_connect_resp *connect_rsp);

/**
 * lim_update_fils_hlp_data() - Update the hlp data from association
 * response frame to PE session.
 * @hlp_frm_src_mac: SRC mac address in HLP IE from assoc frame
 * @hlp_frm_dst_mac: DST mac address in HLP IE from assoc frame
 * @frm_hlp_len: HLP data length
 * @frm_hlp_data: Pointer to hlp data
 * @pe_session: Pointer to pe_session
 *
 * Return: None
 */
void lim_update_fils_hlp_data(struct qdf_mac_addr *hlp_frm_src_mac,
			      struct qdf_mac_addr *hlp_frm_dest_mac,
			      uint16_t frm_hlp_len, uint8_t *frm_hlp_data,
			      struct pe_session *pe_session);

/**
 * aead_encrypt_assoc_req() - Encrypt FILS IE's in assoc request
 * @mac_ctx: mac context
 * @pe_session: PE session
 * @frame: packed frame buffer
 * @payload: length of @frame
 *
 * This API is used to encrypt the all the IE present after FILS session IE
 * in Association request frame
 *
 * Return: QDF_STATUS
 */
QDF_STATUS aead_encrypt_assoc_req(struct mac_context *mac_ctx,
				  struct pe_session *pe_session,
				  uint8_t *frame, uint32_t *payload);

/**
 * aead_decrypt_assoc_rsp() - API for AEAD decryption in FILS connection
 * @mac_ctx: MAC context
 * @session: PE session
 * @ar: Assoc response frame structure
 * @p_frame: frame buffer received
 * @n_frame: length of @p_frame
 *
 * This API is used to decrypt the AEAD encrypted part of FILS assoc response
 * and populate the decrypted FILS IE's to Assoc response frame structure(ar)
 *
 * Return: QDF_STATUS
 */
QDF_STATUS aead_decrypt_assoc_rsp(struct mac_context *mac_ctx,
				  struct pe_session *session,
				  tDot11fAssocResponse *ar,
				  uint8_t *p_frame, uint32_t *n_frame);
/**
 * lim_is_fils_connection() - Check if it is FILS connection
 * @pe_session: PE session
 *
 * This API is used to check if current PE session is FILS connection
 *
 * Return: True if FILS connection, false if not
 */
static inline bool lim_is_fils_connection(struct pe_session *pe_session)
{
	if (pe_session->fils_info->is_fils_connection)
		return true;
	return false;
}

static inline struct pe_fils_session *
lim_get_fils_info(struct pe_session *pe_session,
		  tSirMacAddr peer_mac_addr)
{
	struct tLimPreAuthNode *sta_pre_auth_ctx;

	if (LIM_IS_AP_ROLE(pe_session) && peer_mac_addr) {
		/*
		 * Extract pre-auth context for the STA
		 */
		sta_pre_auth_ctx = lim_search_pre_auth_list(pe_session->mac_ctx,
							    peer_mac_addr);

		if (!sta_pre_auth_ctx) {
			pe_debug("No preauth node created for "
				 QDF_MAC_ADDR_FMT,
				 QDF_MAC_ADDR_REF(peer_mac_addr));
			return NULL;
		}
		return sta_pre_auth_ctx->fils_info;
	} else {
		return pe_session->fils_info;
	}
}

/**
 * lim_verify_fils_params_assoc_rsp() - Verify FILS params in assoc rsp
 * @mac_ctx: Mac context
 * @session_entry: PE session
 * @assoc_rsp: Assoc response received
 * @assoc_cnf: Assoc cnf msg to be sent to MLME
 *
 * This API is used to match FILS params received in Assoc response
 * with Assoc params received/derived at the Authentication stage
 *
 * Return: True, if successfully matches. False, otherwise
 */
bool lim_verify_fils_params_assoc_rsp(struct mac_context *mac_ctx,
				      struct pe_session *session_entry,
				      tpSirAssocRsp assoc_rsp,
				      tLimMlmAssocCnf * assoc_cnf);
#else
static inline bool lim_process_fils_auth_frame2(struct mac_context *mac_ctx,
		struct pe_session *pe_session, tSirMacAuthFrameBody *rx_auth_frm_body)
{
	return false;
}

static inline void
lim_increase_fils_sequence_number(struct pe_session *session_entry)
{ }

static inline void
lim_add_fils_data_to_auth_frame(struct pe_session *session, uint8_t *body)
{
}

static inline bool lim_is_valid_fils_auth_frame(struct mac_context *mac_ctx,
	struct pe_session *pe_session, tSirMacAuthFrameBody *rx_auth_frm_body)
{
	return true;
}

static inline void lim_update_fils_config(struct mac_context *mac_ctx,
			    struct pe_session *session,
			    struct cm_vdev_join_req *join_req)
{}

static inline
QDF_STATUS lim_create_fils_auth_data(struct mac_context *mac_ctx,
				     tpSirMacAuthFrameBody auth_frame,
				     struct pe_session *session,
				     uint32_t *frame_len,
				     tSirMacAddr peer_mac_addr);
{
	return QDF_STATUS_SUCCESS;
}

static inline bool lim_is_fils_connection(struct pe_session *pe_session)
{
	return false;
}

static inline void
populate_fils_connect_params(struct mac_context *mac_ctx,
			     struct pe_session *session,
			     struct wlan_cm_connect_resp *connect_rsp)
{ }

static inline
void lim_update_fils_hlp_data(struct qdf_mac_addr *hlp_frm_src_mac,
			      struct qdf_mac_addr *hlp_frm_dest_mac,
			      uint16_t frm_hlp_len, uint8_t *frm_hlp_data,
			      struct pe_session *pe_session)
{}

static inline QDF_STATUS aead_encrypt_assoc_req(struct mac_context *mac_ctx,
						struct pe_session *pe_session,
						uint8_t *frame,
						uint32_t *payload)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS aead_decrypt_assoc_rsp(struct mac_context *mac_ctx,
						struct pe_session *session,
						tDot11fAssocResponse *ar,
						uint8_t *p_frame,
						uint32_t *n_frame)
{
	return QDF_STATUS_SUCCESS;
}

static inline bool
lim_verify_fils_params_assoc_rsp(struct mac_context *mac_ctx,
				 struct pe_session *session_entry,
				 tpSirAssocRsp assoc_rsp,
				 tLimMlmAssocCnf *assoc_cnf)

{
	return true;
}
#endif /* WLAN_FEATURE_FILS_SK */

#ifdef WLAN_FEATURE_FILS_SK_SAP
/**
 * lim_cache_fils_key() - Cache FILS temporal key for FILS connection.
 * @pe_session: PE Session
 * @unicast: 1 for unicast, 0 for broadcast.
 * @key_id: Index on which key needs to cached.
 * @key_length: Length of temporal key to be cached.
 * @key: Key to be cached.
 * @mac_addr: MAC Address for peer.
 *
 * Return: QDF_STATUS
 */

QDF_STATUS lim_cache_fils_key(struct pe_session *pe_session, bool unicast,
			      uint8_t key_id, uint16_t key_length,
			      uint8_t *key, struct qdf_mac_addr *mac_addr);

/**
 * lim_set_fils_key() - Set FILS temporal key for FILS connection.
 * @pe_session: PE Session
 * @unicast: 1 for unicast, 0 for broadcast.
 * @key_idx: Index on which key needs to cached.
 * @mac_addr: MAC Address for peer.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS lim_set_fils_key(struct pe_session *pe_session, bool unicast,
			    uint8_t key_idx);

/**
 * lim_install_fils_key() - Install FILS temporal key for FILS connection.
 * @pe_session: PE Session
 * @mac_addr: MAC Address for peer.
 *
 * Return: QDF_STATUS
 */

QDF_STATUS lim_install_fils_key(struct pe_session *pe_session,
				const void *mac_addr);
/**
 * aead_encrypt_assoc_rsp() - Encrypt FILS IE's in Assoc Response
 * @mac_ctx: mac context
 * @pe_session: PE session
 * @frame: packed frame buffer
 * @payload: length of @frame
 * @peer_addr Peer Mac Address of STA
 *
 * This API is used to encrypt all the IEs present after FILS session IE
 * in Association response frame
 *
 * Return: QDF_STATUS
 */
QDF_STATUS aead_encrypt_assoc_rsp(struct mac_context *mac_ctx,
				  struct pe_session *pe_session,
				  uint8_t *frame, uint32_t *payload,
				  tSirMacAddr peer_addr);

/**
 * aead_decrypt_assoc_req() - API for AEAD decryption in FILS connection
 * @mac_ctx: MAC context
 * @session: PE session
 * @assoc_req: Assoc Request frame structure
 * @p_frame: frame buffer received
 * @n_frame: length of @p_frame
 * @peer_addr Peer Mac Address of STA
 *
 * This API is used to decrypt the AEAD encrypted part of FILS Assoc Request
 * and populate the decrypted FILS IE's to Assoc request frame structure.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS aead_decrypt_assoc_req(struct mac_context *mac_ctx,
				  struct pe_session *session,
				  tDot11fAssocRequest *assoc_req,
				  uint8_t *p_frame, uint32_t *n_frame,
				  tSirMacAddr peer_mac_addr);
/**
 * lim_verify_fils_params_assoc_req() - Verify FILS params in assoc request
 * @mac_ctx: Mac context
 * @session_entry: PE session
 * @assoc_req: Assoc request received
 * @peer_mac_addr: Peer Mac Address
 *
 * This API is used to match FILS params received in Assoc request
 * with Assoc params received/derived at the Authentication stage
 *
 * Return: True, if successfully matches. False, otherwise
 */
bool lim_verify_fils_params_assoc_req(struct mac_context *mac_ctx,
				      struct pe_session *session_entry,
				      tpSirAssocReq assoc_req,
				      tSirMacAddr peer_mac_addr);

#else
static inline QDF_STATUS lim_cache_fils_key(struct pe_session *pe_session,
					    bool unicast, uint8_t key_id,
					    uint16_t key_length, uint8_t *key,
					    struct qdf_mac_addr *mac_addr)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS lim_set_fils_key(struct pe_session *pe_session,
					  bool unicast, uint8_t key_idx)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS lim_install_fils_key(struct pe_session *pe_session,
					       const void *mac_addr)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS aead_encrypt_assoc_rsp(struct mac_context *mac_ctx,
						struct pe_session *pe_session,
						uint8_t *frame,
						uint32_t *payload,
						tSirMacAddr peer_addr)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS aead_decrypt_assoc_req(struct mac_context *mac_ctx,
						struct pe_session *session,
						tDot11fAssocRequest *assoc_req,
						uint8_t *p_frame,
						uint32_t *n_frame,
						tSirMacAddr peer_addr)
{
	return QDF_STATUS_SUCCESS;
}

static inline bool
lim_verify_fils_params_assoc_req(struct mac_context *mac_ctx,
				 struct pe_session *session_entry,
				 tpSirAssocReq assoc_req,
				 tSirMacAddr peer_mac_addr)

{
	return true;
}
#endif /* WLAN_FEATURE_FILS_SK_SAP */
#endif
