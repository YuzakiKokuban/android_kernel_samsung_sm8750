/*
 * FST L2DA + NETLINK based mux implementation
 *
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
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
 */
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/socket.h>
#include <netlink/attr.h>
#include <netlink/msg.h>
#include "utils/includes.h"
#include "utils/common.h"
#include "utils/list.h"
#include <linux/if_bonding_genl.h>
#include "common/defs.h"
#include "fst_mux.h"
#include "fst_cfgmgr.h"

#define FST_MGR_COMPONENT "MUX"
#include "fst_manager.h"

struct fst_mux
{
	char           bond_ifname[IFNAMSIZ + 1];
	char          *ap_default_ifname;
	struct nl_sock *nl;
	int            fam_id;
};

#define BOND_L2DA_STA_OPTS  (BOND_L2DA_OPT_DEDUP_RX | \
			     BOND_L2DA_OPT_REPLACE_MAC | \
			     BOND_L2DA_OPT_AUTO_ARP_ANNOUNCE )
#define BOND_L2DA_AP_OPTS  (BOND_L2DA_OPT_DUP_MC_TX | \
			    BOND_L2DA_OPT_FORWARD_RX)

static struct nl_msg * _init_genl_msg(struct fst_mux *ctx, int cmd)
{
	struct nl_msg *msg;
	void *hdr;
	int res;

	msg = nlmsg_alloc();
	if(msg == NULL) {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate nlmsg");
		goto fail_nlmsg;
	}

	hdr = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, ctx->fam_id,
			0, NLM_F_REQUEST | NLM_F_ACK, cmd, BOND_GENL_VERSION);
	if (hdr == NULL) {
		fst_mgr_printf(MSG_ERROR, "genlmsg_put failed");
		goto fail_nlmsg_put;
	}

	res = nla_put_string(msg, BOND_GENL_ATTR_BOND_NAME, ctx->bond_ifname);
	if (res != 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot put bond: %s", nl_geterror(res));
		goto fail_nlmsg_put;
	}

	fst_mgr_printf(MSG_DEBUG, " _init_genl_msg created msg for %d cmd", cmd);

	return msg;

fail_nlmsg_put:
	nlmsg_free(msg);
fail_nlmsg:
	return NULL;
}

static int _send_and_free_genl_msg(struct fst_mux *ctx, struct nl_msg *msg)
{
	int res;

	res = nl_send_auto(ctx->nl, msg);
	nlmsg_free(msg);
	if (res < 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot nl_send: %s", nl_geterror(res));
		return -1;
	}

	return 0;
}

static int _send_genl_reset_map_msg(struct fst_mux *ctx)
{
	struct nl_msg *msg;
	int res;

	msg = _init_genl_msg(ctx, BOND_GENL_CMD_L2DA_RESET_MAP);
	if (msg == NULL)
		return -1;

	res = _send_and_free_genl_msg(ctx, msg);
	if (!res)
		fst_mgr_printf(MSG_INFO, "L2DA map reset");
	else
		fst_mgr_printf(MSG_ERROR, "Cannot reset L2DA map");
	return res;
}

static int _send_genl_set_opts_msg(struct fst_mux *ctx, u32 opts)
{
	struct nl_msg *msg;
	int res;

	msg = _init_genl_msg(ctx, BOND_GENL_CMD_L2DA_SET_OPTS);
	if (msg == NULL)
		return -1;

	res = nla_put_u32(msg, BOND_GENL_ATTR_L2DA_OPTS, opts);
	if (res != 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot put opts: %s",
			nl_geterror(res));
		nlmsg_free(msg);
		return -1;
	}

	res = _send_and_free_genl_msg(ctx, msg);
	if (!res)
		fst_mgr_printf(MSG_INFO, "L2DA opts set to 0x%08x", opts);
	else
		fst_mgr_printf(MSG_ERROR, "Cannot set L2DA opts");
	return res;
}

static int _send_genl_set_def_slave_msg(struct fst_mux *ctx, const char *ifname)
{
	struct nl_msg *msg;
	int res;

	msg = _init_genl_msg(ctx, BOND_GENL_CMD_L2DA_SET_DEFAULT);
	if (msg == NULL)
		return -1;

	res = nla_put_string(msg, BOND_GENL_ATTR_SLAVE_NAME, ifname);
	if (res != 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot put slave: %s",
			nl_geterror(res));
		nlmsg_free(msg);
		return -1;
	}

	res = _send_and_free_genl_msg(ctx, msg);
	if (!res)
		fst_mgr_printf(MSG_INFO, "L2DA default slave set to %s",
			       ifname);
	else
		fst_mgr_printf(MSG_ERROR, "Cannot set L2DA default slave");
	return res;

}

static int _send_genl_set_change_map_msg(struct fst_mux *ctx, const u8 *da,
		const char *ifname)
{
	struct nl_msg *msg;
	int res;

	res = ifname ? BOND_GENL_CMD_L2DA_ADD_MAP_ENTRY :
		       BOND_GENL_CMD_L2DA_DEL_MAP_ENTRY;

	msg = _init_genl_msg(ctx, res);
	if (msg == NULL)
		return -1;

	res = nla_put(msg, BOND_GENL_ATTR_MAC, ETH_ALEN, da);
	if (res != 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot put address: %s", nl_geterror(res));
		goto fail_nlmsg_put;
	}

	if (ifname) {
		res = nla_put_string(msg, BOND_GENL_ATTR_SLAVE_NAME, ifname);
		if (res != 0) {
			fst_mgr_printf(MSG_ERROR, "Cannot put slave: %s",
				nl_geterror(res));
			goto fail_nlmsg_put;
		}
	}

	res = _send_and_free_genl_msg(ctx, msg);
	if (!res)
		fst_mgr_printf(MSG_INFO, "L2DA map entry changed ["MACSTR",%s]",
				MAC2STR(da), ifname ? ifname : "NULL");
	else
		fst_mgr_printf(MSG_ERROR,
			       "Cannot change L2DA map entry  ["MACSTR", %s]",
				MAC2STR(da), ifname ? ifname : "NULL");
	return res;


fail_nlmsg_put:
	nlmsg_free(msg);
	return -1;
}

struct fst_mux *fst_mux_init(const char *group_name)
{
	struct fst_mux *ctx = NULL;
	int len;
	char buf[80];
	char ifname[IFNAMSIZ + 1];

	len = fst_cfgmgr_get_mux_type(group_name, buf, sizeof(buf)-1);
	if (len) {
		if (os_strcmp(buf, "bonding_l2da")) {
			fst_mgr_printf(MSG_ERROR, "Unsupported mux type: %s", buf);
			return NULL;
		}
	}

	ctx = os_zalloc(sizeof(*ctx));
	if (!ctx) {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate driver for %s",
			group_name);
		return NULL;
	}

	len = fst_cfgmgr_get_mux_ifname(group_name, ctx->bond_ifname,
	   sizeof(ctx->bond_ifname)-1);
	if (len == 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot get mux ifname");
		goto fail_mux_name;
	}

	if (!fst_is_supplicant() &&
	    fst_cfgmgr_get_l2da_ap_default_ifname(group_name, ifname,
					     sizeof(ifname)-1) > 0) {
		ctx->ap_default_ifname = os_strdup(ifname);
		if (!ctx->ap_default_ifname) {
			fst_mgr_printf(MSG_ERROR, "Cannot dup ap default ifname");
			goto fail_ap_default_name;
		}
	}

	ctx->nl = nl_socket_alloc();
	if (!ctx->nl) {
		fst_mgr_printf(MSG_ERROR, "Cannot alloc nl socket");
		goto fail_nl_socket;
	}

	if (genl_connect(ctx->nl)) {
		fst_mgr_printf(MSG_ERROR, "genl_connect failed");
		goto fail_nl_connect;
	}

	nl_socket_disable_seq_check(ctx->nl);

	ctx->fam_id = genl_ctrl_resolve(ctx->nl, BOND_GENL_NAME);
	if (ctx->fam_id < 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot get nl family id");
		goto fail_nl_famid;
	}

	fst_mgr_printf(MSG_DEBUG, "mux_l2da initialized for %s",
		ctx->bond_ifname);

	return ctx;

fail_nl_famid:
fail_nl_connect:
	nl_socket_free(ctx->nl);
fail_nl_socket:
	os_free(ctx->ap_default_ifname);
fail_ap_default_name:
fail_mux_name:
	os_free(ctx);
	return NULL;
}

int fst_mux_start(struct fst_mux *ctx)
{
	u32 opts;

	if(_send_genl_reset_map_msg(ctx) != 0) {
		fst_mgr_printf(MSG_ERROR, "Error starting mux: reset map");
		return -1;
	}

	opts = fst_is_supplicant() ? BOND_L2DA_STA_OPTS : BOND_L2DA_AP_OPTS;
	if (_send_genl_set_opts_msg(ctx, opts)) {
		fst_mgr_printf(MSG_ERROR, "Error starting mux: set opts");
		return -1;
	}

	if (!fst_is_supplicant() && ctx->ap_default_ifname &&
	    _send_genl_set_def_slave_msg(ctx, ctx->ap_default_ifname)) {
		fst_mgr_printf(MSG_ERROR, "Error starting mux: set ap default iface");
		return -1;
	}

	return 0;
}

int fst_mux_add_map_entry(struct fst_mux *ctx, const u8 *da,
		const char *iface_name)
{
	return fst_is_supplicant() ?
			_send_genl_set_def_slave_msg(ctx, iface_name) :
			_send_genl_set_change_map_msg(ctx, da, iface_name);
}

int fst_mux_del_map_entry(struct fst_mux *ctx, const u8 *da)
{
	/* "No need to del map entry for STA as we use default slave */
	return fst_is_supplicant() ?
			0 : _send_genl_set_change_map_msg(ctx, da, NULL);
}

int fst_mux_register_iface(struct fst_mux *ctx, const char *iface_name,
		u8 priority)
{
	return 0;	/* Left for mux API compatibility */
}

void fst_mux_unregister_iface(struct fst_mux *ctx, const char *iface_name)
{
	/* Left for mux API compatibility */
}

void fst_mux_stop(struct fst_mux *ctx)
{
	if(_send_genl_reset_map_msg(ctx) != 0)
		fst_mgr_printf(MSG_WARNING, "Error stopping mux");
}

void fst_mux_cleanup(struct fst_mux *ctx)
{
	nl_socket_free(ctx->nl);
	os_free(ctx->ap_default_ifname);
	os_free(ctx);
}
