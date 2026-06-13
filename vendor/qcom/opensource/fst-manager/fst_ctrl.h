/*
 * FST Manager: hostapd/wpa_supplicant control interface wrapper definitions
 *
 * Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __FST_CTRL_H__
#define __FST_CTRL_H__
#include <errno.h>
#include "fst/fst_ctrl_aux.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * fst_notification_cb_func - FST notification callback function
 * @cb_ctx: %ntfy_cb_ctx as passed to fst_set_notify_cb()
 * @group_id: FST group ID
 * @session_id: FST session ID
 * @event_type: kind of FST event for the call
 * @extra: event-specific auxiliary information.
 * TODO: Define extra information for each event type
 */
typedef void (*fst_notification_cb_func)(void *cb_ctx,
    u32 session_id,
	enum fst_event_type event_type,
	void *extra);

/**
 * fst_set_notify_cb - set FST control callback
 * @ntfy_cb: FST notification callback function, %NULL to disable
 * @ntfy_cb_ctx: FST notification callback function's context to be passed to %ntfy_cb
 * Returns: 0 if success or negative error code
 */
int fst_set_notify_cb(fst_notification_cb_func ntfy_cb, void *ntfy_cb_ctx);

struct fst_group_info {
	char id[FST_MAX_GROUP_ID_SIZE];
};

/**
 * fst_get_groups - get list of FST groups
 * @groups: Array of fst_group_info elements allocated by the function.
 *				The array should be freed by caller.
 * Returns: Number of allocated elements if success or negative error code
 */
int fst_get_groups(struct fst_group_info **groups);

struct fst_iface_info{
	char name[FST_MAX_INTERFACE_SIZE];
	u8   addr[ETH_ALEN];
	u8   priority;
	u32  llt;
	u8   manual_enslave;
};

/**
 * fst_get_iface_peers - get list of connected peers for this interface
 * @group: FST group info from fst_get_groups()
 * @iface: FST interface info fst_get_group_ifaces()
 * @peers: The list of peers MAC addresses allocated by the function.
 *				The array should be freed by caller.
 * Returns: Number of allocated elements if success or negative error code
 */
int fst_get_iface_peers(const struct fst_group_info *group,
			struct fst_iface_info *iface, uint8_t **peers);

/**
 * fst_get_peer_mbies - get the hexadecimal dump of the peer's MBIEs on a given
 * interface
 * @ifname: FST interface name
 * @peer:  The peer's MAC address
 * @mbies: The hexadecimal dump of the peer's MBIEs on the given interface
 *	if not NULL. The array should be freed by caller.
 * Returns: Number of allocated bytes if success or negative error code
 */
int fst_get_peer_mbies(const char *ifname, const uint8_t *peer, char **mbies);

/**
 * fst_get_group_ifaces - get interfaces for the provided group
 * @group_id: FST group ID from fst_get_groups()
 * @ifaces: Array of fst_iface_info elements allocated by the function.
 *				The array should be freed by caller.
 * Returns: Number of allocated elements if success or negative error code
 */
int fst_get_group_ifaces(const struct fst_group_info *group,
		struct fst_iface_info **ifaces);

/**
 * fst_detach_iface - Detaches FST interface
 * @iface: FST interface info for the interface to be detached
 *
 * Retunrs: 0 if success or a negative error code
 */
int fst_detach_iface(const struct fst_iface_info *iface);

/**
 * fst_attach_iface - Attaches FST interface
 * @group: FST group info for the group the interface to be attached to
 * @iface: FST interface info for the interface to be attached
 *
 * Retunrs: 0 if success or a negative error code
 */
int fst_attach_iface(const struct fst_group_info *group,
	const struct fst_iface_info *iface);

/**
 * fst_is_supplicant - Returns true if the hostap application is
 * the wpa_supplicant, FALSE  - if the hostapd
 */
Boolean fst_is_supplicant(void);

/**
 * fst_add_iface - Add interfaces to Hostap
 * @master: Master interface name
 * @iface: Interface name
 * @acl_file: ACL file name for accept_mac_file (see hostapd.conf), can be NULL
 * @ctrl_interface: Control interface path, can be NULL
 */
int fst_add_iface(const char *master, const struct fst_iface_info *iface,
	const char *acl_file, const char *ctrl_interface);

/**
 * fst_del_iface - Delete interface from Hostap
 * @iface: Interface name
 */
int fst_del_iface(const struct fst_iface_info *iface);

/**
 * fst_dup_connection - Copy the connection params from master interface and
 * initiate a hostap connection for the interface iface
 * @iface: Interface info for the interface to connect
 * @master: Master interface name the connection parameters to be copied from
 * @addr: Address (BSSID) to connect to
 * @freq: Frequency to scan for the AP
 * @acl_file: ACL file name for accept_mac_file (see hostapd.conf), can be NULL
 */
int fst_dup_connection(const struct fst_iface_info *iface,
	const char *master, const u8 *addr, u32 freq, const char *acl_file);

/**
 * fst_dedup_connection - disallow connection on interface iface
 * @iface: Interface
 * @acl_file: ACL file name accept_mac_file (see hostapd.conf), can be NULL
 */
int fst_dedup_connection(const struct fst_iface_info *iface, const char *acl_file);

/**
 * fst_disconnect_peer - Disconnects from peer over specific interface
 * @ifname: Interface to disconnect
 * @peer_addr: peer to disconnect from
 */
int fst_disconnect_peer(const char *ifname, const u8 *peer_addr);

/**
 * fst_get_sessions - Iterate FST session within a group
 * @group_id: FST group ID from fst_get_groups()
 * @sessions: Array of fst_sessions allocated by the function.
 *				The array should be freed by caller.
 * Returns: Number of allocated elements if success or negative error code
 */
int fst_get_sessions(const struct fst_group_info *group, u32 **sessions);

struct fst_session_info
{
	u8   old_peer_addr[ETH_ALEN];
	u8   new_peer_addr[ETH_ALEN];
	char old_ifname[FST_MAX_INTERFACE_SIZE];
	char new_ifname[FST_MAX_INTERFACE_SIZE];
	u32  llt;
	enum fst_session_state state;
	u32 session_id;
};

/**
 * fst_free - free an array allocated by fst_get_groups, fst_get_group_ifaces or
 *            fst_get_sessions
 * @p: an array allocated by fst_get_groups, fst_get_group_ifaces,
 *            fst_get_iface_peers or fst_get_sessions
 */
void fst_free(void *p);

/**
 * fst_session_get_info - Get FST session within a group
* @session_id: FST Session id as received from fst_get_sessions.
 * @info: structure to fill if call succeeds
 * Returns: 0 on success, negative error codes on errors
 */
int fst_session_get_info(u32 session_id, struct fst_session_info *info);

/**
 * fst_session_add - Create FST session context
 * @session_id: FST Session id as received from fst_get_sessions.
 * Returns: 0 on success, negative error codes on errors
 */
int fst_session_add(const char * group_id, u32 * session_id);

/**
 * fst_session_remove - Remove FST session
 * @session_id: FST Session id as received from fst_get_sessions.
 * Returns: 0 on success, supported error codes on failure.
 */
int fst_session_remove(u32 session_id);

/**
 * fst_session_set - Set FST session parameter
 * @session_id: FST Session id as received from fst_get_sessions.
 * @pname: parameter name to set
 * @pval: parameter value to set
 * Returns: 0 on success, negative error codes on failures
 */
int fst_session_set(u32 session_id, const char *pname, const char * pval);


/**
 * fst_session_initiate - Initiate FST session
 * @session_id: FST Session id as received from fst_get_sessions.
 * Returns: 0 on success, negative error code on failure
 */
int fst_session_initiate(u32 session_id);

/**
 * fst_session_respond - Respond to FST session request
 * @session_id: FST Session id as received from fst_get_sessions.
 * @response_status: response status string
 * Returns: 0 on success, supported error codes on failure.
 */

int fst_session_respond(u32 session_id, const char *response_status);

/**
 * fst_session_transfer - Transfer established FST session
 * @session_id: FST Session id as received from fst_get_sessions.
 * Returns: 0 on success, supported error codes on failure.
 */
int fst_session_transfer(u32 session_id);

/**
 * fst_session_teardown - Tear down established FST session
 * @session_id: FST Session id as received from fst_get_sessions.
 * Returns: 0 on success, supported error codes on failure.
 */
int fst_session_teardown(u32 session_id);

int fst_signal_monitor(const char *iface, int thresh, int hyst);
void fst_ctrl_bss_flush(const char *iface);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*  __FST_CTRL_H__ */

