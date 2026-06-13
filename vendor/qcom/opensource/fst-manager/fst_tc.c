/*
 * FST TC related routines implementation
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
#include <string.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>

#include <netlink/netlink.h>
#include <linux/netlink.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include <linux/if_ether.h>
#include <linux/rtnetlink.h>
#include <linux/tc_act/tc_skbedit.h>
#include <linux/tc_act/tc_mirred.h>
#include <linux/tc_act/tc_gact.h>
#include <net/if.h>

#include "utils/list.h"
#define FST_MGR_COMPONENT "TC"
#include "fst_manager.h"
#include "fst_tc.h"

#define IF_INDEX_NONE (-1)
#define MULTIQ_QDISC_HANDLE 0x00010000
#define INGRESS_QDISC_HANDLE 0xFFFF0000

/* NOTE: we use priorities as an unique identifier of the filter in case we want
 * to modify/delete specific one.
 * This is because using filter's handle (struct tcmsg->tcm_handle) to identify
 * filters appeared to be very tricky, so sometimes kernel refuses the handles
 * provided by userspace applications.
 */
#define PRIO_BOND_TX_DUP_FILTER    1
#define PRIO_BOND_TX_BASE          PRIO_BOND_TX_DUP_FILTER
#define PRIO_WLAN_RX_EAPOL_FILTER  1
#define PRIO_WLAN_RX_DEDUP_FILTER  2

#define PRIO_MAX                   ((u16)-1)

#ifdef CONFIG_LIBNL20
#define nlmsg_datalen(hdr) nlmsg_len(hdr)
#define nl_send_auto(sk, msg) nl_send_auto_complete(sk, msg)
#endif

struct fst_tc_iface
{
	char ifname[IFNAMSIZ];
	int ifidx;
	struct dl_list ifaces_lentry;
};

struct fst_tc {
	struct nl_sock *nl;
	char ifname[IFNAMSIZ];
	int ifidx;
	Boolean is_sta;
	struct dl_list ifaces;
	struct dl_list filters;
};

static u16 fst_tc_get_lowest_unused_prio(struct fst_tc *f)
{
	struct fst_tc_filter_handle *h;
	u16 prio;
	int avail;

	for (prio = PRIO_BOND_TX_BASE; prio < PRIO_MAX; prio++) {
		avail = 1;
		dl_list_for_each(h, &f->filters, struct fst_tc_filter_handle,
			filters_lentry) {
			if (h->prio == prio) {
				avail = 0;
				break;
			}
		}
		if (avail)
			break;
	}

	return prio;
}

static void tc_set_ifidx(struct fst_tc *f, char *ifname, int ifidx)
{
	struct fst_tc_iface *i;

	if (!os_strcmp(f->ifname, ifname)) {
		f->ifidx = ifidx;
		return;
	}

	dl_list_for_each(i, &f->ifaces, struct fst_tc_iface, ifaces_lentry)
		if (!strcmp(ifname, i->ifname)) {
			i->ifidx = ifidx;
			break;
		}
}

static int cb_network_link(struct nl_msg *msg, void *arg)
{
	struct fst_tc *f = arg;
	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	struct ifinfomsg *ifm = nlmsg_data(hdr);
	struct nlattr *attr;
	int remaining;

	if(hdr->nlmsg_type != RTM_NEWLINK && hdr->nlmsg_type != RTM_DELLINK)
		return NL_OK;

	attr = nlmsg_attrdata(hdr, sizeof(struct ifinfomsg));
	remaining = nlmsg_attrlen(hdr, sizeof(struct ifinfomsg));
	while(nla_ok(attr, remaining)) {
		if(nla_type(attr) == IFLA_IFNAME)
			if(nla_len(attr) > 0)
				tc_set_ifidx(f, nla_data(attr), ifm->ifi_index);
		attr = nla_next(attr, &remaining);
	}

	return NL_OK;
}

static int tc_qdisc_modify(struct fst_tc *f, unsigned add, const char *kind,
	int ifindex, u32 handle, u32 parent)
{
	struct tc_multiq_qopt opt;
	struct tcmsg t;
	int res=0;
	struct nl_msg *msg;
	int nlmsgtype = RTM_DELQDISC;
	int nlmsflags = NLM_F_REQUEST | NLM_F_ACK;

	memset(&opt, 0, sizeof(opt));

	if (add) {
		nlmsgtype = RTM_NEWQDISC;
		nlmsflags |= NLM_F_EXCL | NLM_F_CREATE;
	}

	msg = nlmsg_alloc_simple(nlmsgtype, nlmsflags);
	if(msg == NULL) {
		fst_mgr_printf(MSG_ERROR, "nlmsg_alloc_simple failed");
		return -1;
	}

	t.tcm_family = AF_UNSPEC;
	t.tcm_handle = handle;
	t.tcm_parent = parent;
	t.tcm_ifindex = ifindex;
	nlmsg_append(msg, &t, sizeof(t), NLMSG_ALIGNTO);

	nla_put(msg, TCA_OPTIONS, sizeof(opt), &opt);
	nla_put(msg, TCA_KIND, os_strlen(kind) + 1, kind);

	res = nl_send_auto(f->nl, msg);
	if(res < 0)  {
		fst_mgr_printf(MSG_ERROR, "nl_send_auto failed: %s",
			nl_geterror(res));
		res = -1;
		goto tmqm_ret;
	}

	res = nl_recvmsgs_default(f->nl);
	if(res < 0) {
		if (add && errno == EEXIST)
			fst_mgr_printf(MSG_WARNING, "%s qdisc already exists",
				kind);
		else if (!add && errno == ENOENT)
			fst_mgr_printf(MSG_WARNING, "%s qdisc doesn't exist",
				kind);
		else {
			fst_mgr_printf(MSG_ERROR, "nl_recvmsgs_default failed: %s",
				nl_geterror(res));
			res = -1;
			goto tmqm_ret;
		}
	}

tmqm_ret:
	nlmsg_free(msg);
	return res;
}

static inline int tc_multiq_qdisc_modify(struct fst_tc *f, unsigned add)
{
	int res = tc_qdisc_modify(f, add, "multiq", f->ifidx, MULTIQ_QDISC_HANDLE,
		TC_H_ROOT);
	if (res)
		fst_mgr_printf(MSG_ERROR, "%s: cannot %s multiq qdisc",
			f->ifname, add ? "add": "remove");
	else
		fst_mgr_printf(MSG_DEBUG, "%s: multiq qdisc %s",
			f->ifname, add ? "added": "removed");
	return res;
}

static int tc_ingress_qdisc_modify(struct fst_tc *f, unsigned add,
	struct fst_tc_iface *i)
{
	int res = tc_qdisc_modify(f, add, "ingress", i->ifidx,
				INGRESS_QDISC_HANDLE,
				TC_H_INGRESS);
	if (res)
		fst_mgr_printf(MSG_ERROR, "%s: cannot %s ingress qdisc",
			i->ifname, add ? "add": "remove");
	else
		fst_mgr_printf(MSG_DEBUG, "%s: ingress qdisc %s",
			i->ifname, add ? "added": "removed");
	return res;
}

typedef int (*tc_filter_fill_clb)(struct fst_tc *f,
	unsigned add, struct nl_msg *msg, void *ctx);

static int tc_filter_modify(struct fst_tc *f, unsigned add,
			u32 parent,
			int ifidx,
			uint16_t prio,
			const char *classifier,
			tc_filter_fill_clb clb,
			void *clb_ctx)
{
	struct tcmsg t;
	int res=0;
	struct nl_msg *msg;
	int nlmsgtype = RTM_DELTFILTER;
	int nlmsflags = NLM_F_REQUEST | NLM_F_ACK;

	if (add) {
		nlmsgtype = RTM_NEWTFILTER;
		nlmsflags |= NLM_F_EXCL | NLM_F_CREATE;
	}

	msg = nlmsg_alloc_simple(nlmsgtype, nlmsflags);
	if(msg == NULL) {
		fst_mgr_printf(MSG_ERROR, "nlmsg_alloc_simple failed");
		return -1;
	}

	os_memset(&t, 0 , sizeof(t));

	t.tcm_family = AF_UNSPEC;
	t.tcm_parent = parent;
	t.tcm_ifindex = ifidx;
	t.tcm_info = TC_H_MAKE(((uint32_t) prio) << 16, htons(ETH_P_ALL));
	nlmsg_append(msg, &t, sizeof(t), NLMSG_ALIGNTO);

	nla_put(msg, TCA_KIND, os_strlen(classifier) + 1, classifier);

	if (clb) {
		res = clb(f, add, msg, clb_ctx);
		if (res < 0) {
			fst_mgr_printf(MSG_ERROR, "clb failed: %d", res);
			goto tfm_ret;
		}
	}

	res = nl_send_auto(f->nl, msg);
	if(res < 0)  {
		fst_mgr_printf(MSG_ERROR, "nl_send_auto failed: %s",
			nl_geterror(res));
		res = -1;
		goto tfm_ret;
	}

	res = nl_recvmsgs_default(f->nl);
	if(res < 0) {
		fst_mgr_printf(MSG_ERROR, "nl_recvmsgs_default failed: %s",
			nl_geterror(res));
		res = -1;
		goto tfm_ret;
	}

tfm_ret:
	nlmsg_free(msg);
	return res;
}

struct tc_l2da_filter_modify_ctx
{
	const uint8_t * mac;
	uint16_t        queue_id;
};

static int tc_l2da_filter_modify_clb(struct fst_tc *f, unsigned add,
	struct nl_msg *msg, void *ctx)
{
	const char qdisc_action[] = "skbedit";
	if (add) {
		struct tc_l2da_filter_modify_ctx *c = ctx;
		struct {
			struct tc_u32_sel sel;
			struct tc_u32_key keys[2];
		} sel;
		struct tc_skbedit skbsel;

		memset(&sel, 0, sizeof(sel));
		memset(&skbsel, 0, sizeof(skbsel));

		if (c->mac) {
			uint32_t mac32;
			uint16_t mac16;

			mac16 = ((uint16_t) c->mac[0] << 8) | c->mac[1];
			mac32 = ((uint32_t) c->mac[2] << 24) |
				((uint32_t) c->mac[3] << 16) |
				((uint32_t) c->mac[4] << 8) |
				c->mac[5];

			sel.keys[0].val = htonl(mac32);
			sel.keys[0].mask = htonl((uint32_t) 0xFFFFFFFF);
			sel.keys[0].off = -12;
			sel.keys[0].offmask = 0;
			sel.keys[1].val = htonl((uint32_t) mac16);
			sel.keys[1].mask = htonl((uint32_t) 0xFFFF);
			sel.keys[1].off = -16;
			sel.keys[1].offmask = 0;
			sel.sel.flags |= TC_U32_TERMINAL;
			sel.sel.nkeys = 2;
		}
		else {
			/* NOTE: NULL mac means filter all packets. This is
			 *       achieved by ((h_proto & 0) == 0) which is
			 *       always true.
			 */
			sel.keys[0].val = 0;
			sel.keys[0].mask = 0;
			sel.keys[0].off = -2; /* h_proto */
			sel.keys[0].offmask = 0;
			sel.sel.flags |= TC_U32_TERMINAL;
			sel.sel.nkeys = 1;
		}

		struct nlattr *t_opt, *t_act, *t_1, *t_act_opt;

		t_opt = nla_nest_start(msg, TCA_OPTIONS);
		if (t_opt == NULL) {
			fst_mgr_printf(MSG_ERROR, "nla_nest_start failed");
			return -1;
		}
		t_act = nla_nest_start(msg, TCA_U32_ACT);
		if (t_act == NULL) {
			fst_mgr_printf(MSG_ERROR, "nla_nest_start failed");
			return -1;
		}
		t_1 = nla_nest_start(msg, 1);
		if (t_1 == NULL) {
			fst_mgr_printf(MSG_ERROR, "nla_nest_start failed");
			return -1;
		}
		nla_put(msg, TCA_ACT_KIND, sizeof(qdisc_action), qdisc_action);
		t_act_opt = nla_nest_start(msg, TCA_ACT_OPTIONS);
		if (t_act_opt == NULL) {
			fst_mgr_printf(MSG_ERROR, "nla_nest_start failed");
			return -1;
		}
		nla_put(msg, TCA_SKBEDIT_PARMS, sizeof(skbsel), &skbsel);
		nla_put(msg, TCA_SKBEDIT_QUEUE_MAPPING,
			sizeof(c->queue_id), &c->queue_id);
		nla_nest_end(msg, t_act_opt);
		nla_nest_end(msg, t_1);
		nla_nest_end(msg, t_act);
		nla_put(msg, TCA_U32_SEL, sizeof(sel), &sel);
		nla_nest_end(msg, t_opt);
	}

	return 0;
}

static int tc_l2da_filter_modify(struct fst_tc *f, unsigned add,
	const uint8_t * mac, uint16_t queue_id, u16 prio)
{
	struct tc_l2da_filter_modify_ctx ctx = {
		.mac = mac,
		.queue_id = queue_id,
	};
	int res;

	res = tc_filter_modify(f, add, MULTIQ_QDISC_HANDLE,
			f->ifidx, prio, "u32", tc_l2da_filter_modify_clb, &ctx);
	if (res)
		fst_mgr_printf(MSG_ERROR,
			"%s: cannot %s L2DA filter#%u",
			f->ifname, add ? "add" : "remove",
			prio);
	else if (add && mac)
		fst_mgr_printf(MSG_DEBUG,
				"%s: L2DA filter#%u for " MACSTR " added",
				f->ifname, prio, MAC2STR(mac));
	else if (add)
		fst_mgr_printf(MSG_DEBUG,
				"%s: L2DA filter#%u (universal) added",
				f->ifname, prio);
	else
		fst_mgr_printf(MSG_DEBUG,
				"%s: L2DA filter#%u removed",
				f->ifname, prio);

	return res;
}

static int tc_filter_add_mirred_action(struct nl_msg *msg,
	const char *ifname, int prio)
{
	const char kind[] = "mirred";
	struct nlattr *t_prio, *t_act_opt;
	struct tc_mirred p;

	memset(&p, 0, sizeof(p));

	p.eaction = TCA_EGRESS_MIRROR;
	p.action = TC_ACT_PIPE;
	p.ifindex = if_nametoindex(ifname);

	t_prio = nla_nest_start(msg, prio);
	if (t_prio == NULL) {
		fst_mgr_printf(MSG_ERROR, "nla_nest_start failed");
		return -1;
	}
	nla_put(msg, TCA_ACT_KIND, sizeof(kind), kind);

	t_act_opt = nla_nest_start(msg, TCA_ACT_OPTIONS);
	if (t_act_opt == NULL) {
		fst_mgr_printf(MSG_ERROR, "nla_nest_start failed");
		return -1;
	}

	nla_put(msg, TCA_MIRRED_PARMS, sizeof(p), &p);
	nla_nest_end(msg, t_act_opt);
	nla_nest_end(msg, t_prio);

	return 0;
}

static int tc_filter_add_generic_action(struct nl_msg *msg, int prio,
	int action)
{
	const char kind[] = "gact";
	struct nlattr *t_prio, *t_act_opt;
	struct tc_gact p;

	memset(&p, 0, sizeof(p));

	p.action = action;

	t_prio = nla_nest_start(msg, prio);
	if (t_prio == NULL) {
		fst_mgr_printf(MSG_ERROR, "nla_nest_start failed");
		return -1;
	}
	nla_put(msg, TCA_ACT_KIND, sizeof(kind), kind);

	t_act_opt = nla_nest_start(msg, TCA_ACT_OPTIONS);
	if (t_act_opt == NULL) {
		fst_mgr_printf(MSG_ERROR, "nla_nest_start failed");
		return -1;
	}
	nla_put(msg, TCA_GACT_PARMS, sizeof(p), &p);
	nla_nest_end(msg, t_act_opt);
	nla_nest_end(msg, t_prio);

	return 0;
}

static int tc_mc_filter_modify_clb(struct fst_tc *f, unsigned add,
	struct nl_msg *msg, void *ctx)
{
	if (add) {
		struct fst_tc_iface *i;
		struct nlattr *t_opt, *t_act;
		int prio = 1;
		struct {
			struct tc_u32_sel sel;
			struct tc_u32_key keys[1];
		} sel;
		int res;

		memset(&sel, 0, sizeof(sel));

		sel.keys[0].val = htonl((uint32_t) 0x0100);
		sel.keys[0].mask = htonl((uint32_t) 0x0100);
		sel.keys[0].off = -16;
		sel.keys[0].offmask = 0;
		sel.sel.flags |= TC_U32_TERMINAL;
		sel.sel.nkeys = 1;

		t_opt = nla_nest_start(msg, TCA_OPTIONS);
		if (t_opt == NULL) {
			fst_mgr_printf(MSG_ERROR, "nla_nest_start failed");
			return -1;
		}

		t_act = nla_nest_start(msg, TCA_U32_ACT);
		if (t_act == NULL) {
			fst_mgr_printf(MSG_ERROR, "nla_nest_start failed");
			return -1;
		}

		dl_list_for_each(i, &f->ifaces, struct fst_tc_iface,
			ifaces_lentry) {
			res = tc_filter_add_mirred_action(msg, i->ifname, prio);
			if (res < 0)
				return res;
			++prio;
		}
		res = tc_filter_add_generic_action(msg, prio, TC_ACT_SHOT);
		if (res < 0)
			return res;
		nla_nest_end(msg, t_act);

		nla_put(msg, TCA_U32_SEL, sizeof(sel), &sel);
		nla_nest_end(msg, t_opt);
	}

	return 0;
}

static int tc_mc_filter_modify(struct fst_tc *f, unsigned add)
{
	int res = tc_filter_modify(f, add,  MULTIQ_QDISC_HANDLE,
			f->ifidx, PRIO_BOND_TX_DUP_FILTER, "u32",
			tc_mc_filter_modify_clb, NULL);
	if (res)
		fst_mgr_printf(MSG_ERROR, "%s: cannot %s MC filter",
			f->ifname, add ? "add": "remove");
	else
		fst_mgr_printf(MSG_DEBUG, "%s: MC filter %s",
			f->ifname, add ? "added": "removed");
	return res;
}

static int fst_tc_get_iface_idxs(struct fst_tc *f)
{
	int res;
	struct rtgenmsg rt_hdr = { .rtgen_family = AF_UNSPEC, };
	struct fst_tc_iface *i;

	/* Set the receiving messages callback to get and process the links */
	nl_socket_modify_cb(f->nl, NL_CB_VALID, NL_CB_CUSTOM, cb_network_link,
		(void*)f);

	res = nl_send_simple(f->nl, RTM_GETLINK, NLM_F_REQUEST | NLM_F_DUMP,
		&rt_hdr, sizeof(rt_hdr));
	if (res < 0) {
		fst_mgr_printf(MSG_ERROR, "nl_send_simple failed: %s",
			nl_geterror(res));
		return -1;
	}

	res = nl_recvmsgs_default(f->nl);
	if (res < 0) {
		fst_mgr_printf(MSG_ERROR, "nl_recvmsgs_default failed: %s",
			nl_geterror(res));
		return -1;
	}
	/* We've done with the receiving callback */
	nl_socket_modify_cb(f->nl, NL_CB_VALID, NL_CB_DEFAULT , NULL, NULL);

	if (f->ifidx == IF_INDEX_NONE) {
		fst_mgr_printf(MSG_ERROR, "Cannot find interface %s index",
			f->ifname);
		return -1;
	}

	fst_mgr_printf(MSG_INFO, "Interface %s index = %d",
		f->ifname, f->ifidx);

	dl_list_for_each(i, &f->ifaces, struct fst_tc_iface, ifaces_lentry) {
		if (i->ifidx == IF_INDEX_NONE) {
			fst_mgr_printf(MSG_ERROR,
				"Cannot find interface %s index", i->ifname);
			return -1;
		}
		fst_mgr_printf(MSG_INFO, "Interface %s index = %d",
			i->ifname, i->ifidx);
	}

	return 0;
}

static int fst_tc_add_multiq_qdisc(struct fst_tc *f)
{
	return tc_multiq_qdisc_modify(f, 1);
}

static int fst_tc_del_multiq_qdisc(struct fst_tc *f)
{
	return tc_multiq_qdisc_modify(f, 0);
}

static int fst_tc_add_ingress_qdisc(struct fst_tc *f)
{
	struct fst_tc_iface *i, *j;

	dl_list_for_each(i, &f->ifaces, struct fst_tc_iface, ifaces_lentry) {
		if (!tc_ingress_qdisc_modify(f, 1, i))
			fst_mgr_printf(MSG_WARNING, "%s: ingress qdisc added",
				i->ifname);
		else {
			fst_mgr_printf(MSG_WARNING,
				"%s: cannot remove ingress qdisc",
				i->ifname);
			goto add_failed;
		}
	}

	return 0;

add_failed:
	dl_list_for_each(j, &f->ifaces, struct fst_tc_iface, ifaces_lentry)
		tc_ingress_qdisc_modify(f, 0, j);
	return -1;
}

static void fst_tc_del_ingress_qdisc(struct fst_tc *f)
{
	struct fst_tc_iface *i;

	dl_list_for_each(i, &f->ifaces, struct fst_tc_iface, ifaces_lentry)
		if (!tc_ingress_qdisc_modify(f, 0, i))
			fst_mgr_printf(MSG_WARNING, "%s: ingress qdisc removed",
				i->ifname);
		else
			fst_mgr_printf(MSG_WARNING,
				"%s: cannot remove ingress qdisc",
				i->ifname);
}

struct tc_rx_mc_filter_modify_ctx
{
	const uint8_t       *mac;
};

static int tc_rx_mc_filter_modify_clb(struct fst_tc *f, unsigned add,
	struct nl_msg *msg, void *ctx)
{
	if (add) {
		struct tc_l2da_filter_modify_ctx *c = ctx;
		uint32_t mac32;
		uint16_t mac16;
		struct {
			struct tc_u32_sel sel;
			struct tc_u32_key keys[2];
		} sel;
		int res;

		memset(&sel, 0, sizeof(sel));

		mac32 = ((uint32_t) c->mac[0] << 24) |
			((uint32_t) c->mac[1] << 16) |
			((uint32_t) c->mac[2] << 8) |
			c->mac[3];
		mac16 = ((uint16_t) c->mac[4] << 8) | c->mac[5];

		sel.keys[0].val = htonl(mac32);
		sel.keys[0].mask = htonl((uint32_t) 0xFFFFFFFF);
		sel.keys[0].off = -8;
		sel.keys[0].offmask = 0;
		sel.keys[1].val = htonl((uint32_t) mac16);
		sel.keys[1].mask = htonl((uint32_t) 0xFFFF);
		sel.keys[1].off = -4;
		sel.keys[1].offmask = 0;
		sel.sel.flags |= TC_U32_TERMINAL;
		sel.sel.nkeys = 2;

		struct nlattr *t_opt, *t_act;

		t_opt = nla_nest_start(msg, TCA_OPTIONS);
		if (t_opt == NULL) {
			fst_mgr_printf(MSG_ERROR, "nla_nest_start failed");
			return -1;
		}

		t_act = nla_nest_start(msg, TCA_U32_ACT);
		if (t_act == NULL) {
			fst_mgr_printf(MSG_ERROR, "nla_nest_start failed");
			return -1;
		}

		res = tc_filter_add_generic_action(msg, 1, TC_ACT_SHOT);
		if (res < 0)
			return res;
		nla_nest_end(msg, t_act);

		nla_put(msg, TCA_U32_SEL, sizeof(sel), &sel);
		nla_nest_end(msg, t_opt);
	}

	return 0;
}

static int fst_tc_modify_rx_mc_filters(struct fst_tc *f, unsigned add,
	const u8 * mac, const char *active_ifname,
	u16 prio)
{
	struct fst_tc_iface *i;
	struct tc_rx_mc_filter_modify_ctx ctx = {
		.mac = mac,
	};

	dl_list_for_each(i, &f->ifaces, struct fst_tc_iface, ifaces_lentry) {
		int res;

		if (!os_strcmp(i->ifname, active_ifname))
			continue;

		res = tc_filter_modify(f, add, INGRESS_QDISC_HANDLE,
			i->ifidx, prio, "u32", tc_rx_mc_filter_modify_clb, &ctx);

		if (res)
			fst_mgr_printf(MSG_WARNING,
				"%s: cannot %s ingress filter#%u",
				i->ifname, add ? "add" : "remove",
				prio);
		else if (add)
			fst_mgr_printf(MSG_DEBUG,
				"%s: ingress filter#%u for " MACSTR " added",
				i->ifname, prio, MAC2STR(mac));
		else
			fst_mgr_printf(MSG_DEBUG,
				"%s: ingress filter#%u removed",
				i->ifname, prio);
	}

	return 0;
}

static int tc_rx_eapol_filter_modify_clb(struct fst_tc *f, unsigned add,
	struct nl_msg *msg, void *ctx)
{
	if (add) {
		struct nlattr *t_opt, *t_act;
		struct {
			struct tc_u32_sel sel;
			struct tc_u32_key keys[1];
		} sel;
		int res;

		memset(&sel, 0, sizeof(sel));

		sel.keys[0].val = htonl((uint32_t) ETH_P_PAE);
		sel.keys[0].mask = htonl((uint32_t) 0xFFFF);
		sel.keys[0].off = -2;
		sel.keys[0].offmask = 0;
		sel.sel.flags |= TC_U32_TERMINAL;
		sel.sel.nkeys = 1;

		t_opt = nla_nest_start(msg, TCA_OPTIONS);
		if (t_opt == NULL) {
			fst_mgr_printf(MSG_ERROR, "nla_nest_start failed");
			return -1;
		}

		t_act = nla_nest_start(msg, TCA_U32_ACT);
		if (t_act == NULL) {
			fst_mgr_printf(MSG_ERROR, "nla_nest_start failed");
			return -1;
		}

		res = tc_filter_add_generic_action(msg, 1, TC_ACT_OK);
		if (res < 0)
			return res;
		nla_nest_end(msg, t_act);

		nla_put(msg, TCA_U32_SEL, sizeof(sel), &sel);
		nla_nest_end(msg, t_opt);
	}

	return 0;
}

static int fst_tc_modify_rx_eapol_filters(struct fst_tc *f, unsigned add)
{
	struct fst_tc_iface *i;

	dl_list_for_each(i, &f->ifaces, struct fst_tc_iface, ifaces_lentry) {
		int res;

		res = tc_filter_modify(f, add, INGRESS_QDISC_HANDLE,
			i->ifidx, PRIO_WLAN_RX_EAPOL_FILTER, "u32",
			tc_rx_eapol_filter_modify_clb, NULL);

		if (res)
			fst_mgr_printf(MSG_WARNING,
				"%s: cannot %s ingress EAPOL filter",
				i->ifname, add ? "add" : "remove");
		else if (add)
			fst_mgr_printf(MSG_DEBUG,
				"%s: ingress EAPOL filter added",
				i->ifname);
		else
			fst_mgr_printf(MSG_DEBUG,
				"%s: ingress EAPOL filter removed",
				i->ifname);
	}

	return 0;
}

struct fst_tc *fst_tc_create(Boolean is_sta)
{
	struct fst_tc *f;
	int res;

	f = malloc(sizeof(*f));
	if (!f) {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate TC Ctrl");
		goto alloc_failed;
	}

	f->ifidx = IF_INDEX_NONE;

	f->nl = nl_socket_alloc();
	if (f->nl == NULL) {
		fst_mgr_printf(MSG_ERROR, "nl_socket_alloc failed");
		goto socket_alloc_failed;
	}

	nl_socket_disable_seq_check(f->nl);
	res = nl_connect(f->nl, NETLINK_ROUTE);
	if(res != 0) {
		fst_mgr_printf(MSG_ERROR, "nl_connect failed: %s",
			nl_geterror(res));
		goto connect_failed;
	}

	dl_list_init(&f->ifaces);
	dl_list_init(&f->filters);
	f->is_sta = is_sta;

	return f;

connect_failed:
	nl_socket_free(f->nl);
socket_alloc_failed:
	os_free(f);
alloc_failed:
	return NULL;
}

int fst_tc_start(struct fst_tc *f, const char *ifname)
{
	if (!ifname || f->ifidx != IF_INDEX_NONE)
		return -1;

	os_strlcpy(f->ifname, ifname, sizeof(f->ifname));

	if (fst_tc_get_iface_idxs(f) != 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot get iface indexes for bond#%s",
			ifname);
		goto fail_get_iface_idxs;
	}

	/* cleanup previously installed qdisc if any. ignore errors */
	fst_tc_del_multiq_qdisc(f);
	fst_tc_del_ingress_qdisc(f);

	if (fst_tc_add_multiq_qdisc(f)) {
		fst_mgr_printf(MSG_ERROR, "Cannot add multiq qdisc for bond#%s",
			ifname);
		goto fail_add_muliq;
	}

	if (!f->is_sta) {
		/* AP can have multiple STAs connected over multiple interfaces.
		 * Thus it needs to duplicate multicast frames to all interfaces
		 * to make sure that all the STAs receive it, no matter which
		 * interfaces are currently to which STA.
		 */
		if (tc_mc_filter_modify(f, 1)) {
			fst_mgr_printf(MSG_ERROR, "Cannot set MC filter");
			goto fail_l2mc_filter;
		}
	} else {
		/* STA can only be connected to one AP, so there's no need for
		 * duplication. However, it has to de-duplicate RX in order to
		 * drop out packets sent by AP over inactive interface(s) due to
		 * AP side duplication.
		 */
		if (fst_tc_add_ingress_qdisc(f)) {
			fst_mgr_printf(MSG_ERROR, "Cannot add ingress qdisc for bond#%s",
				ifname);
			goto fail_add_ingress;
		}

		if (fst_tc_modify_rx_eapol_filters(f, 1)) {
			fst_mgr_printf(MSG_ERROR, "Cannot add RX EAPOL filter");
			goto fail_add_rx_eapol_filter;
		}
	}

	return 0;

fail_add_rx_eapol_filter:
	if (f->is_sta)
		fst_tc_del_ingress_qdisc(f);
fail_add_ingress:
	if (!f->is_sta)
		tc_mc_filter_modify(f, 0);
fail_l2mc_filter:
	fst_tc_del_multiq_qdisc(f);
fail_add_muliq:
	f->ifidx = IF_INDEX_NONE;
fail_get_iface_idxs:
	memset(f->ifname, 0, sizeof(f->ifname));
	return -1;
}

void fst_tc_stop(struct fst_tc *f)
{
	if (f->is_sta) {
		fst_tc_modify_rx_eapol_filters(f, 0);
		fst_tc_del_ingress_qdisc(f);
	}
	else
		tc_mc_filter_modify(f, 0);
	fst_tc_del_multiq_qdisc(f);
	f->ifidx = IF_INDEX_NONE;
	memset(f->ifname, 0, sizeof(f->ifname));
}

void fst_tc_delete(struct fst_tc *f)
{
	struct fst_tc_iface *i;
	nl_close(f->nl);
	nl_socket_free(f->nl);
	while ((i = dl_list_first(&f->ifaces,
			struct fst_tc_iface,
			ifaces_lentry)) != NULL)
		fst_tc_unregister_iface(f, i->ifname);
	free(f);
}

int fst_tc_register_iface(struct fst_tc *f, const char *ifname)
{
	struct fst_tc_iface *i;

	i = os_zalloc(sizeof(*i));
	if (!i) {
		fst_mgr_printf(MSG_ERROR, "Cannot register iface %s", ifname);
		return -1;
	}

	os_strlcpy(i->ifname, ifname, sizeof(i->ifname));
	i->ifidx = IF_INDEX_NONE;
	dl_list_add_tail(&f->ifaces, &i->ifaces_lentry);

	return 0;
}

void fst_tc_unregister_iface(struct fst_tc *f, const char *ifname)
{
	struct fst_tc_iface *i;

	dl_list_for_each(i, &f->ifaces, struct fst_tc_iface, ifaces_lentry) {
		if (!os_strcmp(i->ifname, ifname)) {
			dl_list_del(&i->ifaces_lentry);
			os_free(i);
			break;
		}
	}
}

int fst_tc_add_l2da_filter(struct fst_tc *f, const uint8_t * mac, int queue_id,
	const char *ifname, struct fst_tc_filter_handle *filter_handle)
{
	int res;

	filter_handle->prio = fst_tc_get_lowest_unused_prio(f);
	if (filter_handle->prio == PRIO_MAX)  {
		fst_mgr_printf(MSG_ERROR, "%s: cannot find prio for " MACSTR,
			ifname, MAC2STR(mac));
		goto get_avail_prio_fail;
	}

	if (!f->is_sta) {
		/* AP:
		 * - can have many peers (STAs) connected and, in turn, filters
		 * - traffic should be redirected on per-STA basis
		 * - shouldn't de-duplicate RX, as STAs don't duplicate it
		 */

		res = tc_l2da_filter_modify(f, 1, mac, queue_id,
			filter_handle->prio);
		if (res) {
			fst_mgr_printf(MSG_ERROR, "%s: cannot add UC filter for " MACSTR,
				ifname, MAC2STR(mac));
			goto l2da_filter_fail;
		}
	}
	else {
		/* STA:
		 * - only has 1 peer (AP), so it should only have one filter
		 *   installed
		 * - all the traffic should be redirected to the active
		 *   interface (NULL indicates this).
		 * - should de-duplicate RX, as AP duplicates it
		 */

		WPA_ASSERT(filter_handle->prio == PRIO_BOND_TX_BASE);

		res = tc_l2da_filter_modify(f, 1, NULL, queue_id,
			filter_handle->prio);
		if (res) {
			fst_mgr_printf(MSG_ERROR, "%s: cannot add universal filter",
				ifname);
			goto l2da_filter_fail;
		}

		res = fst_tc_modify_rx_mc_filters(f, 1, mac, ifname,
			filter_handle->prio);
		if (res != 0)  {
			fst_mgr_printf(MSG_ERROR, "%s: cannot add RX MC filter",
				ifname);
			goto rx_mc_filter_fail;
		}
	}

	os_strlcpy(filter_handle->ifname, ifname,
		sizeof(filter_handle->ifname));
	dl_list_add(&f->filters, &filter_handle->filters_lentry);

	return 0;

rx_mc_filter_fail:
	if (f->is_sta)
		tc_l2da_filter_modify(f, 0, NULL, 0, filter_handle->prio);
l2da_filter_fail:
get_avail_prio_fail:
	os_memset(filter_handle, 0, sizeof(*filter_handle));
	return -1;
}

int fst_tc_del_l2da_filter(struct fst_tc *f,
	struct fst_tc_filter_handle *filter_handle)
{
	int res = 0;

	if (tc_l2da_filter_modify(f, 0, NULL, 0, filter_handle->prio)) {
		fst_mgr_printf(MSG_ERROR, "%s: cannot del UC filter#%u",
			filter_handle->ifname, filter_handle->prio);
		res = -1;
	}

	if (f->is_sta && fst_tc_modify_rx_mc_filters(f, 0, NULL,
		filter_handle->ifname, filter_handle->prio) != 0) {
		fst_mgr_printf(MSG_ERROR, "%s: cannot del MC RX filter#%u",
			filter_handle->ifname, filter_handle->prio);
		res = -1;
	}

	dl_list_del(&filter_handle->filters_lentry);
	os_memset(filter_handle, 0, sizeof(*filter_handle));

	return res;
}

