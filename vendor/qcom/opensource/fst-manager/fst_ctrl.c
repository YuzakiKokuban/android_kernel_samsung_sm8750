/*
 * FST Manager: hostapd/wpa_supplicant control interface wrapper: CLI based
 * implementation
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define FST_MGR_COMPONENT "CTRL"
#include "fst_manager.h"

#include "utils/common.h"
#include "common/defs.h"
#include "utils/eloop.h"

#include "common/wpa_ctrl.h"
#include "fst/fst_ctrl_defs.h"
#include "fst_ctrl.h"
#include "fst_cfgmgr.h"
#include "fst_capconfigstore.h"

#ifndef DEFAULT_WPAS_CLI_DIR
#define DEFAULT_WPAS_CLI_DIR "/var/run/wpa_supplicant"
#endif
#ifndef DEFAULT_HAPD_CLI_DIR
#define DEFAULT_HAPD_CLI_DIR "/var/run/hostapd"
#endif

static struct wpa_ctrl *ctrl_evt;
static struct wpa_ctrl *ctrl_cmd;
static unsigned int     ctrl_ping_interval;
static Boolean          ctrl_is_supplicant;

static const char FST_CONFIG_SCAN_INTERVAL_KEY_NAME[] = "fst.config.scan_interval";
#define FST_CONFIG_SCAN_INTERVAL "1" /* Sec */
static int scan_interval;

static fst_notification_cb_func global_ntfy_cb = NULL;
static void *global_ntfy_cb_ctx = NULL;

#define defstrcmp(s, d) strncmp((s), (d), sizeof(d) - 1)

#ifndef CONFIG_CTRL_IFACE_DIR
#define CONFIG_CTRL_IFACE_DIR "/var/run/hostapd"
#endif /* CONFIG_CTRL_IFACE_DIR */

static char *get_pval(char *str, const char *pdesc)
{
	char *p = strchr(str, '=');
	if (p) {
		*p++ = '\0';
		/* strip leading whitespaces */
		while (*p == ' ' || *p == '\t')
			++p;
	}
	else
		fst_mgr_printf(MSG_ERROR,
			"bad %s parameter string reported \'%s\'",
			pdesc, str);
	return p;
}

/* parsers */
static int parse_list(char *p, char *res[], size_t size)
{
	int ret = 0;

	for (;;) {
		while (isspace(*p))
			p++;
		if (!*p)
			return ret;

		if ((size_t)ret == size)
			return -1;

		res[ret++] = p;
		while (*p && !isspace(*p))
			p++;
		if (isspace(*p))
			*p++ = '\0';
	}

	return ret;
}

struct parse_list_proc_ctx {
	size_t item_size;
	int (*item_parser) (char *str, void *data);
	void **output;
	void *current;
};

static int parse_list_proc(char *buf, void *data)
{
	char *list[256];
	struct parse_list_proc_ctx *c = data;
	int ret, i;

	if (!strncmp(buf, "FAIL", 4))
		return -1;

	ret = parse_list(buf, list, ARRAY_SIZE(list));
	if (!ret) {
		fst_mgr_printf(MSG_DEBUG, "str list is empty");
		return 0;
	} else if (ret < 0) {
		fst_mgr_printf(MSG_ERROR, "str list overflow");
		return -1;
	}

	if (!c->current &&
	    !(*c->output = c->current = malloc(c->item_size * ret))) {
		fst_mgr_printf(MSG_ERROR, "cannot allocate mem for %u items", ret);
		return -1;
	}

	for (i = 0; i < ret; i++) {
		c->item_parser(list[i], c->current);
		c->current = (u8*)c->current + c->item_size;
	}

	return ret;
}

/* notifications */
static int peer_event_parser(char *str, void *data)
{
	union fst_event_extra *ev = data;

	if (!defstrcmp(str, FST_CEP_PNAME_CONNECTED))
		ev->peer_state.connected = 1;
	else if (!defstrcmp(str, FST_CEP_PNAME_DISCONNECTED))
		ev->peer_state.connected = 0;
	else {
		char *p = get_pval(str, "Peer Event");
		if (!p)
			return -1;

		if (!defstrcmp(str, FST_CEP_PNAME_IFNAME))
			os_strlcpy(ev->peer_state.ifname, p,
				sizeof(ev->peer_state.ifname));
		else if (!defstrcmp(str, FST_CEP_PNAME_ADDR)) {
			if (hwaddr_aton(p, ev->peer_state.addr))
				fst_mgr_printf(MSG_ERROR,
					"bad peer address string \'%s\'", p);
		}
	}

	return 0;
}

struct session_event_data {
	enum fst_event_type event_type;
	uint32_t session_id;
	union fst_event_extra extra;
};

static int session_event_parser(char *str, void *data)
{
	struct session_event_data *ev = data;
	char *p = get_pval(str, "Session Event");

	if (!p)
		return -1;

	if (!defstrcmp(p, FST_CTRL_PVAL_NONE))
		;
	else if (!defstrcmp(str, FST_CES_PNAME_EVT_TYPE))
		ev->event_type = fst_session_event_num(p);
	else if (!defstrcmp(str, FST_CES_PNAME_SESSION_ID))
		ev->session_id = strtoul(p, NULL, 0);
	else if (!defstrcmp(str, FST_CES_PNAME_OLD_STATE))
		ev->extra.session_state.old_state = fst_session_state_num(p);
	else if (!defstrcmp(str, FST_CES_PNAME_NEW_STATE))
		ev->extra.session_state.new_state = fst_session_state_num(p);
	else if (!defstrcmp(str, FST_CES_PNAME_REASON))
		ev->extra.session_state.extra.to_initial.reason =
					fst_reason_num(p);
	else if (!defstrcmp(str, FST_CES_PNAME_REJECT_CODE))
		ev->extra.session_state.extra.to_initial.reject_code =
			strtoul(p, NULL, 0);
	else if (!defstrcmp(str, FST_CES_PNAME_INITIATOR))
		ev->extra.session_state.extra.to_initial.initiator =
			defstrcmp(p, FST_CS_PVAL_INITIATOR_LOCAL) ?
				FST_INITIATOR_REMOTE : FST_INITIATOR_LOCAL;

	return 0;
}

static Boolean fst_ctrl_notify(char *buf, size_t len)
{
	struct parse_list_proc_ctx ctx;
	char *p = buf;

	memset(&ctx, 0, sizeof(ctx));

	fst_mgr_printf(MSG_DEBUG, "recv: %s", buf);

	if (*p == '<' && (p = strchr(p, '>')))
		p++;

	if (p == NULL) {
		fst_mgr_printf(MSG_ERROR, "Bad notification level format");
		return FALSE;
	}

	if (!strncmp(p, WPA_EVENT_TERMINATING, sizeof(WPA_EVENT_TERMINATING) - 1)) {
		fst_mgr_printf(MSG_ERROR, WPA_EVENT_TERMINATING
				"received: terminating");
		return TRUE;
	}

	if (!global_ntfy_cb)
		return FALSE;

	if (!strncmp(p, FST_CTRL_EVENT_PEER " ", sizeof(FST_CTRL_EVENT_PEER))) {
		union fst_event_extra evt_data = { };

		ctx.item_parser = peer_event_parser;
		ctx.current = &evt_data;

		parse_list_proc(p + sizeof(FST_CTRL_EVENT_PEER), &ctx);

		global_ntfy_cb(global_ntfy_cb_ctx, FST_INVALID_SESSION_ID,
			       EVENT_PEER_STATE_CHANGED, &evt_data);
	} else if (!strncmp(p, FST_CTRL_EVENT_SESSION " ",
			    sizeof(FST_CTRL_EVENT_SESSION))) {
		struct session_event_data evt_data;

		memset(&evt_data, 0, sizeof(evt_data));

		ctx.item_parser = session_event_parser;
		ctx.current = &evt_data;

		parse_list_proc(p + sizeof(FST_CTRL_EVENT_SESSION), &ctx);

		global_ntfy_cb(global_ntfy_cb_ctx, evt_data.session_id,
			       evt_data.event_type, &evt_data.extra);
	} else if (os_strstr(p, WPA_EVENT_SCAN_STARTED)) {
		char *ifname;

		if (defstrcmp(p, "IFNAME"))
			return FALSE;
		ifname = get_pval(p, "Ifname");
		if (!ifname)
			return FALSE;
		p = strchr(ifname, ' ');
		if (p)
			*p = '\0';

		global_ntfy_cb(global_ntfy_cb_ctx, FST_INVALID_SESSION_ID,
			EVENT_FST_SCAN_STARTED, ifname);
	} else if (os_strstr(p, WPA_EVENT_SCAN_RESULTS)) {
		char *ifname;

		if (defstrcmp(p, "IFNAME"))
			return FALSE;
		ifname = get_pval(p, "Ifname");
		if (!ifname)
			return FALSE;
		p = strchr(ifname, ' ');
		if (p)
			*p = '\0';

		global_ntfy_cb(global_ntfy_cb_ctx, FST_INVALID_SESSION_ID,
			EVENT_FST_SCAN_COMPLETED, ifname);
	} else if (os_strstr(p, WPA_EVENT_SIGNAL_CHANGE)) {
		/* above=%d signal=%d noise=%d txrate=%d */
		char *ifname;

		if (defstrcmp(p, "IFNAME"))
			return FALSE;
		ifname = get_pval(p, "Ifname");
		if (!ifname)
			return FALSE;
		p = strchr(ifname, ' ');
		if (p)
			*p = '\0';

		global_ntfy_cb(global_ntfy_cb_ctx, FST_INVALID_SESSION_ID,
			EVENT_FST_SIGNAL_CHANGE, ifname);
	}

	return FALSE;
}

int fst_set_notify_cb(fst_notification_cb_func ntfy_cb, void *ntfy_cb_ctx)
{
	global_ntfy_cb = ntfy_cb;
	global_ntfy_cb_ctx = ntfy_cb_ctx;
	return 0;
}

/* commands */

static int do_hostap_command(const char* cmd, size_t cmd_len,
	char* resp, size_t* resp_len)
{
	int ret;

	fst_mgr_printf(MSG_DEBUG, "send: %s", cmd);

	ret = wpa_ctrl_request(ctrl_cmd, cmd, cmd_len, resp, resp_len, NULL);
	if (ret < 0) {
		fst_mgr_printf(MSG_ERROR, "command '%s' %s.", cmd,
		    ret == -2 ? "timed out" : "failed");
		return ret;
	}
	resp[*resp_len] = '\0';
	fst_mgr_printf(MSG_DEBUG, "recv (len %zu): %s", *resp_len, resp);

	return 0;
}

static int do_command_ex(int (*res_proc) (char *, void *), void *res_data,
		      const char *prefix, const char *fmt, ...)
{
	char buf[1024] = {0};
	char cmd[256];
	size_t buf_len = sizeof(buf) - 1;
	int ret = 0;
	va_list ap;

	if (prefix)
		ret = snprintf(cmd, sizeof(cmd), "%s", prefix);
	va_start(ap, fmt);
	ret += vsnprintf(cmd + ret, sizeof(cmd) - ret, fmt, ap);
	va_end(ap);

	ret = do_hostap_command(cmd, ret, buf, &buf_len);
	if (ret < 0) {
		return ret;
	}

	if (buf_len == 0)
		return 0;

	if (res_proc)
		return res_proc(buf, res_data);

	return strncmp(buf, "OK", 2) ? -1 : 0;
}

#define do_command(proc, data, fmt, ...) \
	do_command_ex(proc, data, "FST-MANAGER ", fmt, ##__VA_ARGS__)

#define do_simple_command(fmt, ...) \
	do_command_ex(NULL, NULL, NULL, fmt, ##__VA_ARGS__)

static int session_info_parser(char *str, void *data)
{
	struct fst_session_info *si = data;
	char *p = get_pval(str, "Session Info");

	if (!p)
		return -1;

	if (!defstrcmp(p, FST_CTRL_PVAL_NONE))
		;
	else if (!defstrcmp(str, FST_CSG_PNAME_OLD_PEER_ADDR)) {
		if (hwaddr_aton(p, si->old_peer_addr))
			fst_mgr_printf(MSG_ERROR,
				"bad own address string \'%s\'", p);
	} else if (!defstrcmp(str, FST_CSG_PNAME_NEW_PEER_ADDR)) {
		if (hwaddr_aton(p, si->new_peer_addr))
			fst_mgr_printf(MSG_ERROR,
				"bad peer address string \'%s\'", p);
	} else if (!defstrcmp(str, FST_CSG_PNAME_NEW_IFNAME)) {
		os_strlcpy(si->new_ifname, p, sizeof(si->new_ifname));
	} else if (!defstrcmp(str, FST_CSG_PNAME_OLD_IFNAME)) {
		os_strlcpy(si->old_ifname, p, sizeof(si->old_ifname));
	} else if (!defstrcmp(str, FST_CSG_PNAME_LLT)) {
		si->llt = strtoul(p, NULL, 0);
	} else if (!defstrcmp(str, FST_CSG_PNAME_STATE)) {
		si->state = fst_session_state_num(p);
	} else {
		fst_mgr_printf(MSG_ERROR, "unknown session parameter \'%s\'", str);
	}

	return 0;
}

int fst_session_get_info(u32 session_id, struct fst_session_info *si)
{
	struct parse_list_proc_ctx ctx;
	int ret;

	ctx.item_size = 0;
	ctx.item_parser = session_info_parser;
	ctx.current = si;

	si->session_id = session_id;

	ret = do_command(parse_list_proc, &ctx, FST_CMD_SESSION_GET " %u",
			 session_id);

	return ret >= 0 ? 0 : -1;
}

int fst_session_respond(u32 session_id, const char *response_status)
{
	return do_command(NULL, NULL, FST_CMD_SESSION_RESPOND " %u %s",
			  session_id, response_status);
}

int fst_session_set(u32 session_id, const char *pname, const char *pval)
{
	return do_command(NULL, NULL, FST_CMD_SESSION_SET " %u %s=%s",
			  session_id, pname, pval);
}

static int session_id_parser(char *str, void *data)
{
	uint32_t *session_id = data;
	char *end;

	*session_id = strtoul(str, &end, 0);

	return *session_id == (uint32_t) - 1 ||
	    (*end && !isspace(*end)) ? -1 : 0;
}

int fst_session_add(const char *group_id, uint32_t * session_id)
{
	return do_command(session_id_parser, session_id,
			  FST_CMD_SESSION_ADD " %s", group_id);
}

int fst_session_remove(uint32_t session_id)
{
	return do_command(NULL, NULL, FST_CMD_SESSION_REMOVE " %u", session_id);
}

int fst_session_initiate(u32 session_id)
{
	return do_command(NULL, NULL, FST_CMD_SESSION_INITIATE " %u",
			  session_id);
}

int fst_session_transfer(u32 session_id)
{
	return do_command(NULL, NULL, FST_CMD_SESSION_TRANSFER " %u",
			  session_id);
}

int fst_session_teardown(u32 session_id)
{
	return do_command(NULL, NULL, FST_CMD_SESSION_TEARDOWN " %u",
			  session_id);
}

int fst_get_sessions(const struct fst_group_info *group, uint32_t ** sessions)
{
	struct parse_list_proc_ctx ctx;

	ctx.item_size = sizeof(*sessions[0]);
	ctx.item_parser = session_id_parser;
	ctx.output = (void *)sessions;
	ctx.current = NULL;

	return do_command(parse_list_proc, &ctx, FST_CMD_LIST_SESSIONS " %s", group->id);
}

static int iface_peer_parser(char *str, void *data)
{
	uint8_t *addr = data;

	if (hwaddr_aton(str, addr))
		fst_mgr_printf(MSG_ERROR, "bad peer address string \'%s\'", str);

	return 0;
}

int fst_get_iface_peers(const struct fst_group_info *group, struct fst_iface_info *iface, uint8_t **peers)
{
	struct parse_list_proc_ctx ctx;

	ctx.item_size = ETH_ALEN;
	ctx.item_parser = iface_peer_parser;
	ctx.output = (void *)peers;
	ctx.current = NULL;

	return do_command(parse_list_proc, &ctx, FST_CMD_IFACE_PEERS " %s %s",
			  group->id, iface->name);
}

static int parse_peer_mbies(char *buf, void *data)
{
	char **mbies = data;

	if (data != NULL)
		*mbies = NULL;

	if (!strncmp(buf, "FAIL", 4))
		return -EINVAL;

	if (data != NULL) {
		*mbies = os_strdup(buf);
		if (!*mbies)
			return -ENOMEM;
	}

	return os_strlen(buf);
}

int fst_get_peer_mbies(const char *ifname, const uint8_t *peer, char **mbies)
{
	return do_command(parse_peer_mbies, mbies, FST_CMD_GET_PEER_MBIES " %s "
			  MACSTR, ifname, MAC2STR(peer));
}

static int iface_parser(char *str, void *data)
{
	struct fst_iface_info *ii = data;
	char *p, *e, *tokbuf;
	const char delims[] = "|";

	p = strtok_r(str, delims, &tokbuf);
	if (!p) {
		fst_mgr_printf(MSG_ERROR, "bad iface name reported \'%s\'", str);
		return -1;
	}
	os_strlcpy(ii->name, p, sizeof(ii->name));

	p = strtok_r(NULL, delims, &tokbuf);
	if (!p) {
		fst_mgr_printf(MSG_ERROR, "bad iface address reported \'%s\'", str);
		return -1;
	}
	if (hwaddr_aton(p, ii->addr)) {
		fst_mgr_printf(MSG_ERROR, "bad addr %s: invalid addr string",
			   p);
		return -1;
	}

	p = strtok_r(NULL, delims, &tokbuf);
	if (!p) {
		fst_mgr_printf(MSG_ERROR, "bad iface priority reported \'%s\'", str);
		return -1;
	}
	ii->priority = strtoul(p, &e, 0);
	if (*e != '\0') {
		fst_mgr_printf(MSG_ERROR, "bad iface string reported \'%s\'", p);
		return -1;
	}

	p = strtok_r(NULL, delims, &tokbuf);
	if (!p) {
		fst_mgr_printf(MSG_ERROR, "bad iface llt reported \'%s\'", str);
		return -1;
	}
	ii->llt = strtoul(p, &e, 0);
	if (*e != '\0') {
		fst_mgr_printf(MSG_ERROR, "bad iface llt reported \'%s\'", p);
		return -1;
	}

	return 0;
}

int fst_get_group_ifaces(const struct fst_group_info *group,
			 struct fst_iface_info **ifaces)
{
	struct parse_list_proc_ctx ctx;

	ctx.item_size = sizeof(*ifaces[0]);
	ctx.item_parser = iface_parser;
	ctx.output = (void *)ifaces;
	ctx.current = NULL;

	return do_command(parse_list_proc, &ctx, FST_CMD_LIST_IFACES " %s",
			  group->id);
}

int fst_detach_iface(const struct fst_iface_info *iface)
{
	return do_command_ex(NULL, NULL, "FST-DETACH", " %s", iface->name);
}

int fst_attach_iface(const struct fst_group_info *group,
	const struct fst_iface_info *iface)
{
	return do_command_ex(NULL, NULL, "FST-ATTACH",
		" %s %s llt=%d priority=%d", iface->name, group->id,
		iface->llt, iface->priority);
}

static int group_id_parser(char *str, void *data)
{
	os_strlcpy(((struct fst_group_info *)data)->id, str,
		sizeof(((struct fst_group_info *) data)->id));
	return 0;
}

int fst_get_groups(struct fst_group_info **groups)
{
	struct parse_list_proc_ctx ctx;

	ctx.item_size = sizeof(*groups[0]);
	ctx.item_parser = group_id_parser;
	ctx.output = (void *)groups;
	ctx.current = NULL;

	return do_command(parse_list_proc, &ctx, FST_CMD_LIST_GROUPS);
}

Boolean fst_is_supplicant(void)
{
	return ctrl_is_supplicant;
}

static int fst_detect_ctrl_type(void)
{
	char buf[4096];
	size_t buf_len = sizeof(buf) - 1;
	const char *cmd = "INTERFACE_LIST";
	int ret = 0;

	ret = do_hostap_command(cmd, os_strlen(cmd), buf, &buf_len);
	if (ret < 0) {
		return -1;
	}

	if (buf_len < 3)
		ctrl_is_supplicant = TRUE;
	else if (strncmp(buf, "BAD", 3) && strncmp(buf, "FAI", 3))
		ctrl_is_supplicant = TRUE;
	else
		ctrl_is_supplicant = FALSE;

	return 0;
}

static int fst_dup_ap_wpa_psk(const char *master,
	const struct fst_iface_info *iface)
{
	int ret;
	char buf[64];

	ret = do_simple_command("DUP_NETWORK %s %s wpa",
		master, iface->name);
	if (ret < 0) {
		fst_mgr_printf(MSG_ERROR, "Dup wpa failed");
		return -1;
	}

	ret = do_simple_command("DUP_NETWORK %s %s wpa_passphrase",
		master, iface->name);
	if (ret < 0) {
		fst_mgr_printf(MSG_ERROR, "Dup wpa_passphrase failed");
		/* wpa_passphrase cannot be duplicated, try wpa_psk */
		ret = do_simple_command("DUP_NETWORK %s %s wpa_psk",
			master, iface->name);
		if (ret < 0) {
			fst_mgr_printf(MSG_ERROR, "Dup wpa_psk failed");
			return -1;
		}
	}

	if (fst_cfgmgr_get_iface_pairwise_cipher(iface,
		buf, sizeof(buf)-1) > 0) {
		ret = do_simple_command("IFNAME=%s SET rsn_pairwise %s",
			iface->name, buf);
		if (ret < 0) {
			fst_mgr_printf(MSG_ERROR,
				"Set rsn_pairwise failed");
			return -1;
		}
	}
	else {
		ret = do_simple_command("DUP_NETWORK %s %s rsn_pairwise",
			master, iface->name);
		if (ret < 0) {
			fst_mgr_printf(MSG_ERROR,
				"Dup rsn_pairwise failed");
			return -1;
		}
	}

	return 0;
}

static int fst_dup_ap_handle_config(const char *master,
	const struct fst_iface_info *iface, const char *name, const char *val)
{
	if (!os_strcmp(name, "key_mgmt")) {
		/* key_mgmt returned from GET_CONFIG means it is a secured AP */
		if (!os_strstr(val, "WPA-PSK")) {
			fst_mgr_printf(MSG_ERROR,
				"key_mgmt=%s is not supported", val);
			return -1;
		}

		if (do_simple_command("IFNAME=%s SET wpa_key_mgmt WPA-PSK",
			iface->name) < 0) {
			fst_mgr_printf(MSG_ERROR, "Set wpa_key_mgmt failed");
			return -1;
		}

		return fst_dup_ap_wpa_psk(master, iface);
	}
	else if (!os_strcmp(name, "ssid") ||
		 !os_strcmp(name, "bssid")) {
		if (do_simple_command("IFNAME=%s SET %s %s",
			iface->name, name, val) < 0) {
			fst_mgr_printf(MSG_ERROR, "Set %s failed", name);
			return -1;
		}
	}

	return 0;
}

static int fst_ap_update_acl_file(const struct fst_iface_info *iface,
	const char *acl_file)
{
	return do_simple_command("IFNAME=%s SET accept_mac_file %s",
		iface->name, acl_file);
}

static int fst_dup_ap_config(const char *master,
	const struct fst_iface_info *iface, const char *acl_file)
{
	char buf[2048];
	char cmd[256];
	char *strstart, *strend, *tok;
	size_t buf_len = sizeof(buf) - 1;
	int ret;

	ret = snprintf(cmd, sizeof(cmd), "IFNAME=%s GET_CONFIG", master);
	ret = do_hostap_command(cmd, ret, buf, &buf_len);
	if (ret < 0)
		goto error_getconfig;

	strstart = buf;
	strend = os_strchr(strstart, '\n');
	while (strend) {
		*strend = '\0';
		tok = os_strchr(strstart, '=');
		if (tok) {
			*tok++ = '\0';
			if (fst_dup_ap_handle_config(master, iface, strstart, tok))
				goto error_setconfig;
		}
		strstart = strend + 1;
		strend = os_strchr(strstart, '\n');
	}

	if (fst_cfgmgr_get_iface_hw_mode(iface, buf, sizeof(buf) - 1) > 0) {
		ret = do_simple_command("IFNAME=%s SET hw_mode %s", iface->name, buf);
		if (ret < 0) {
			fst_mgr_printf(MSG_ERROR, "Set hw_mode failed");
			goto error_setconfig;
		}
	}
	if (fst_cfgmgr_get_iface_channel(iface, buf, sizeof(buf) - 1) > 0) {
		ret = do_simple_command("IFNAME=%s SET channel %s", iface->name, buf);
		if (ret < 0) {
			fst_mgr_printf(MSG_ERROR, "Set channel failed");
			goto error_setconfig;
		}
	}

	if (acl_file) {
		ret = do_simple_command("IFNAME=%s SET macaddr_acl 1", iface->name);
		if (ret < 0) {
			fst_mgr_printf(MSG_ERROR, "Set macaddr_acl failed");
			goto error_setconfig;
		}

		ret = fst_ap_update_acl_file(iface, acl_file);
		if (ret < 0) {
			fst_mgr_printf(MSG_ERROR, "Update acl file failed");
			goto error_setconfig;
		}
	}

	ret = do_simple_command("IFNAME=%s ENABLE", iface->name);
	if (ret < 0) {
		fst_mgr_printf(MSG_ERROR, "Enabling AP failed");
		goto error_setconfig;
	}

	return 0;

error_setconfig:
error_getconfig:
	return -1;
}

int fst_add_iface(const char *master, const struct fst_iface_info *iface,
	const char *acl_file, const char *ctrl_interface)
{
	int res;
	if (fst_is_supplicant()) {
		/* INTERFACE_ADD <ifname>TAB<confname>TAB<driver>TAB<ctrl_interface> */
		res = do_command_ex(NULL, NULL, "INTERFACE_ADD",
			" %s\t\t\t%s", iface->name,
			ctrl_interface ? ctrl_interface : DEFAULT_WPAS_CLI_DIR);
	}
	else {
		res = do_command_ex(NULL, NULL, "ADD", " %s %s",
			iface->name,
			ctrl_interface ? ctrl_interface : DEFAULT_HAPD_CLI_DIR);
		if (res < 0) {
			fst_mgr_printf(MSG_ERROR, "Failed add AP iface %s",
			iface->name);
			return -1;
		}
		res = fst_dup_ap_config(master, iface, acl_file);
		if (res < 0) {
			fst_mgr_printf(MSG_ERROR, "fst_dup_ap failed");
			fst_del_iface(iface);
		}

	}
	return res;
}

int fst_del_iface(const struct fst_iface_info *iface)
{
	int res;
	if (fst_is_supplicant()) {
		res = do_command_ex(NULL, NULL, "INTERFACE_REMOVE", " %s", iface->name);
	}
	else {
		res = do_command_ex(NULL, NULL, "REMOVE", " %s", iface->name);
	}
	return res;
}

static int fst_dedup_station(const struct fst_iface_info *iface)
{
	return do_command_ex(NULL, NULL, "IFNAME=", "%s REMOVE_NETWORK ALL",
		iface->name);
}

static int fst_dup_station(const char *master,
	const struct fst_iface_info *iface, const u8 *bssid, u32 freq)
{
	char buf[4096];
	char cmd[256];
	char *strstart, *strend;
	const char *pos;
	char key_mgmt[10] = "NONE";
	size_t buf_len = sizeof(buf) - 1;
	int ret, srcnetid=-1, netid=-1;

	ret = snprintf(cmd, sizeof(cmd), "IFNAME=%s LIST_NETWORKS", master);
	ret = do_hostap_command(cmd, ret, buf, &buf_len);
	if (ret < 0)
		goto error_master_status;

	strstart = buf;
	strend = os_strchr(strstart, '\n');
	while (strend) {
		*strend = '\0';
		if (os_strstr(strstart, "CURRENT")) {
			srcnetid = (int)strtoul(strstart, NULL, 0);
			break;
		}
		strstart = strend + 1;
		strend = os_strchr(strstart, '\n');
	}

	if (srcnetid == -1) {
		fst_mgr_printf(MSG_ERROR, "Cannot find master %s network id",
			master);
		goto error_no_netid;
	}

	buf_len = sizeof(buf) - 1;
	ret = snprintf(cmd, sizeof(cmd), "IFNAME=%s ADD_NETWORK", iface->name);
	ret = do_hostap_command(cmd, ret, buf, &buf_len);
	if (ret < 0) {
		goto error_add_network;
	}

	if (!strncmp(buf, "FAIL", 4)) {
		fst_mgr_printf(MSG_ERROR, "Failed add_network for %s: %s",
			iface->name, buf);
		goto error_add_network;
	}
	netid = (int)strtoul(buf, NULL, 0);
	fst_mgr_printf(MSG_DEBUG, "Added network %d for %s", netid, iface->name);

	ret = do_simple_command("IFNAME=%s SET_NETWORK %d bssid " MACSTR,
		       iface->name, netid, MAC2STR(bssid));
	if (ret < 0) {
		fst_mgr_printf(MSG_ERROR, "Set bssid for %d failed",
			netid);
		goto error_set;
	}

	buf_len = sizeof(buf) - 1;
	ret = snprintf(cmd, sizeof(cmd),
		"IFNAME=%s GET_NETWORK %d key_mgmt", master, srcnetid);
	ret = do_hostap_command(cmd, ret, buf, &buf_len);
	if (ret < 0) {
		goto error_set;
	}
	strstart = os_strchr(buf, '\n');
	if (!strstart)
		strstart = buf;
	else
		strstart++;
	fst_mgr_printf(MSG_DEBUG,
		"key_mgmt for net %d is %s", srcnetid, strstart);

	if ((pos = os_strstr(strstart, "WPA-PSK")) &&
	    ((pos[os_strlen("WPA-PSK")] == ' ') ||
	     (pos[os_strlen("WPA-PSK")] == '\0'))) {
		os_strlcpy(key_mgmt, "WPA-PSK", sizeof(key_mgmt));
	} else if ((pos = os_strstr(strstart, "WPA2-PSK")) &&
		   ((pos[os_strlen("WPA2-PSK")] == ' ') ||
		    (pos[os_strlen("WPA2-PSK")] == '\0'))) {
		os_strlcpy(key_mgmt, "WPA2-PSK", sizeof(key_mgmt));
	} else if (!((pos = os_strstr(strstart, "NONE")))) {
		fst_mgr_printf(MSG_ERROR, "Unsupported key_mgmt, %s",
			       strstart);
		goto error_set;
	}

	fst_mgr_printf(MSG_DEBUG,
		"updated key_mgmt for net %d is %s", srcnetid, key_mgmt);

	ret = do_simple_command("IFNAME=%s SET_NETWORK %d key_mgmt %s",
		iface->name, netid, key_mgmt);
	if (ret < 0) {
		fst_mgr_printf(MSG_ERROR,
		  "Set wpa key_mgmt for %d failed", netid);
		goto error_set;
	}

	if (freq != 0) {
		ret = do_simple_command("IFNAME=%s SET_NETWORK %d scan_freq %d",
			iface->name, netid, freq);
		if (ret < 0) {
			fst_mgr_printf(MSG_ERROR, "Set frequency for %d failed", netid);
			goto error_set;
		}
	}

	if (os_strcmp(key_mgmt, "NONE")) {
		/* The target network is WPA-PSK */
		strstart = buf;
		if (fst_cfgmgr_get_iface_group_cipher(iface, buf,
			sizeof(buf) - 1) == 0) {
			buf_len = sizeof(buf) - 1;
			ret = snprintf(cmd, sizeof(cmd),
				"IFNAME=%s GET_NETWORK %d group",
				master, srcnetid);
			ret = do_hostap_command(cmd, ret, buf, &buf_len);
			if (ret < 0) {
				goto error_set;
			}
			strstart = os_strchr(buf, '\n');
			if (!strstart)
				strstart = buf;
			else
				strstart++;
		}
		ret = do_simple_command("IFNAME=%s SET_NETWORK %d group %s",
			iface->name, netid, strstart);
		if (ret < 0) {
			fst_mgr_printf(MSG_ERROR,
			  "Set group for %d failed", netid);
			goto error_set;
		}

		strstart = buf;
		if (fst_cfgmgr_get_iface_pairwise_cipher(iface, buf,
			sizeof(buf) - 1) == 0) {
			buf_len = sizeof(buf) - 1;
			ret = snprintf(cmd, sizeof(cmd),
				"IFNAME=%s GET_NETWORK %d pairwise",
				master, srcnetid);
			ret = do_hostap_command(cmd, ret, buf, &buf_len);
			if (ret < 0) {
				goto error_set;
			}
			strstart = os_strchr(buf, '\n');
			if (!strstart)
				strstart = buf;
			else
				strstart++;
		}
		ret = do_simple_command("IFNAME=%s SET_NETWORK %d pairwise %s",
			iface->name, netid, strstart);
		if (ret < 0) {
			fst_mgr_printf(MSG_ERROR,
			  "Set pairwise for %d failed", netid);
			goto error_set;
		}

		ret = do_simple_command("DUP_NETWORK %s %s %d %d psk", master,
			iface->name, srcnetid, netid);
		if (ret < 0) {
			fst_mgr_printf(MSG_ERROR,
			  "Dup PSK for %d failed", netid);
			goto error_set;
		}
	}

	do_command_ex(NULL, NULL, "IFNAME=", "%s SET_NETWORK %d enable_edmg 1", iface->name, netid);

	do_command_ex(NULL, NULL, "IFNAME=", "%s SCAN_INTERVAL %d", iface->name, scan_interval);

	ret = do_simple_command("IFNAME=%s SELECT_NETWORK %d", iface->name, netid);
	if (ret < 0) {
		fst_mgr_printf(MSG_ERROR, "Select network for %d failed",
			netid);
		goto error_set;
	}
	return 0;

error_set:
	if (netid  != -1)
		fst_dedup_station(iface);
error_add_network:
error_no_netid:
error_master_status:
	return -1;
}

int fst_dup_connection(const struct fst_iface_info *iface,
	const char *master, const u8 *addr, u32 freq, const char *acl_file)
{
	int res = 0;
	if ( fst_is_supplicant()) {
		res = fst_dup_station(master, iface, addr, freq);
	}
	else {
		if (acl_file && fst_ap_update_acl_file(iface, acl_file) < 0)
			fst_mgr_printf(MSG_WARNING,
			   "cannot update ACL file for %s", iface->name);
	}
	return res;
}

int fst_dedup_connection(const struct fst_iface_info *iface, const char *acl_file)
{
	int res = 0;
	if ( fst_is_supplicant()) {
		res = fst_dedup_station(iface);
	}
	else {
		if (acl_file && fst_ap_update_acl_file(iface, acl_file) < 0)
			fst_mgr_printf(MSG_WARNING,
			   "cannot update ACL file for %s", iface->name);
	}
	return res;
}

int fst_disconnect_peer(const char *ifname, const u8 *peer_addr)
{
	int ret = -1;

	if (fst_is_supplicant()) {
		fst_mgr_printf(MSG_INFO, "ifname=%s", ifname);
		ret = do_simple_command("IFNAME=%s DISCONNECT", ifname);
	}
	else if (peer_addr) {
		fst_mgr_printf(MSG_INFO, "ifname=%s, peer=" MACSTR, ifname,
			       MAC2STR(peer_addr));
		ret = do_simple_command("IFNAME=%s DISASSOCIATE " MACSTR, ifname, MAC2STR(peer_addr));
	}

	if (ret < 0) {
		fst_mgr_printf(MSG_ERROR, "failed to disconnect peer");
		return ret;
	}

	return 0;
}

void fst_free(void *p)
{
	free(p);
}

/* wpa ctrl */
static void fst_ctrl_receiver(int sock, void *eloop_ctx, void *sock_ctx)
{
	char buf[4096];
	struct wpa_ctrl *ctrl = sock_ctx;
	size_t len;

	while (wpa_ctrl_pending(ctrl) > 0) {
		len = sizeof(buf) - 1;
		if (wpa_ctrl_recv(ctrl, buf, &len)) {
			fst_mgr_printf(MSG_ERROR, "wpa_ctrl_recv");
			break;
		}
		buf[len] = '\0';
		if (fst_ctrl_notify(buf, len)) {
			fst_mgr_printf(MSG_ERROR, "fst_ctrl_notify");
			goto cli_disconnected;
		}
	}

	if (wpa_ctrl_pending(ctrl) < 0) {
		fst_mgr_printf(MSG_ERROR, "wpa_ctrl_pending: connection lost: "
			"terminating");
		goto cli_disconnected;
	}

	return;

cli_disconnected:
	eloop_terminate();
}

static struct wpa_ctrl *try_to_open_wpa_ctrl(const char *name)
{
	WPA_ASSERT(name != NULL);

	if (name && *name) {
#ifndef ANDROID
		struct stat stat_buf;
		if (stat(name, &stat_buf) < 0) {
			fst_mgr_printf(MSG_ERROR, "cannot stat \'%s\'", name);
			return NULL;
		}
#endif

		return wpa_ctrl_open(name);
	}

	return NULL;
}

static void fst_ping(void *eloop_ctx, void *timeout_ctx)
{
	char buf[4096];
	size_t buf_len = sizeof(buf) - 1;
	int ret;

	fst_mgr_printf(MSG_EXCESSIVE, "send: PING");

	ret = wpa_ctrl_request(ctrl_cmd, "PING", 4, buf, &buf_len, NULL);
	if (ret < 0) {
		fst_mgr_printf(MSG_ERROR, "PING %s",
		    ret == -2 ? "timed out" : "failed");
		goto error_ping;
	}
	buf[buf_len] = '\0';

	if (os_strncmp(buf, "PONG", 4)) {
		fst_mgr_printf(MSG_ERROR, "Wrong PING response: %s", buf);
		goto error_pong;
	}

	eloop_register_timeout(ctrl_ping_interval, 0, fst_ping, NULL, NULL);

	return;

error_ping:
error_pong:
	eloop_terminate();
}

Boolean fst_ctrl_create(const char *ctrl_iface, unsigned int ping_interval)
{
	char buf[64] = { '\0' };

	WPA_ASSERT(ctrl_iface != NULL);
	WPA_ASSERT(ctrl_evt == NULL);
	WPA_ASSERT(ctrl_cmd == NULL);

	if (ctrl_iface == NULL) {
		fst_mgr_printf(MSG_ERROR, "ctrl_iface is invalid");
		return FALSE;
	}

	ctrl_evt = try_to_open_wpa_ctrl(ctrl_iface);
	if (!ctrl_evt) {
		fst_mgr_printf(MSG_ERROR, "cannot open control iface (evt): %s",
			ctrl_iface);
		goto error_open_evt;
	}

	if (wpa_ctrl_attach(ctrl_evt)) {
		fst_mgr_printf(MSG_ERROR, "cannot attach control iface: %s",
			ctrl_iface);
		goto error_attach_evt;
	}

	ctrl_cmd = try_to_open_wpa_ctrl(ctrl_iface);
	if (!ctrl_cmd) {
		fst_mgr_printf(MSG_ERROR, "cannot open control iface (cmd): %s",
			ctrl_iface);
		goto error_open_cmd;
	}

	if (eloop_register_read_sock(wpa_ctrl_get_fd(ctrl_evt),
		fst_ctrl_receiver, NULL, ctrl_evt)) {
		fst_mgr_printf(MSG_ERROR, "eloop_register_read_sock");
		goto error_eloop_register_read_sock;
	}

	if (fst_detect_ctrl_type()) {
		fst_mgr_printf(MSG_ERROR, "cannot detect CTRL type");
		goto error_detect_cli_type;
	}

	ctrl_ping_interval = ping_interval;
	if (ctrl_ping_interval)
		eloop_register_timeout(ctrl_ping_interval, 0, fst_ping, NULL, NULL);

	fst_get_config_string(FST_CONFIG_SCAN_INTERVAL_KEY_NAME,
		FST_CONFIG_SCAN_INTERVAL, buf, sizeof(buf));
	scan_interval = strtoul(buf, NULL, 0);

	fst_mgr_printf(MSG_INFO, "connected to control iface: %s",
			ctrl_iface);

	return TRUE;

error_detect_cli_type:
	eloop_unregister_read_sock(wpa_ctrl_get_fd(ctrl_evt));
error_eloop_register_read_sock:
	wpa_ctrl_close(ctrl_cmd);
	ctrl_cmd = NULL;
error_open_cmd:
	wpa_ctrl_detach(ctrl_evt);
error_attach_evt:
	wpa_ctrl_close(ctrl_evt);
	ctrl_evt = NULL;
error_open_evt:
	return FALSE;
}

void fst_ctrl_free(void)
{
	WPA_ASSERT(ctrl_evt != NULL);
	WPA_ASSERT(ctrl_cmd != NULL);

	if (ctrl_evt != NULL)
		eloop_unregister_read_sock(wpa_ctrl_get_fd(ctrl_evt));

	eloop_cancel_timeout(fst_ping, NULL, NULL);

	if (ctrl_cmd != NULL) {
		wpa_ctrl_close(ctrl_cmd);
		ctrl_cmd = NULL;
	}

	if (ctrl_evt != NULL) {
		wpa_ctrl_detach(ctrl_evt);
		wpa_ctrl_close(ctrl_evt);
		ctrl_evt = NULL;
	}
}

int fst_signal_monitor(const char *iface, int thresh, int hyst)
{
	return do_simple_command("IFNAME=%s SIGNAL_MONITOR THRESHOLD=%d HYSTERESIS=%d",
		iface, thresh, hyst);
}

void fst_ctrl_bss_flush(const char *iface)
{
	do_command_ex(NULL, NULL, "IFNAME=", "%s BSS_FLUSH 0", iface);
}