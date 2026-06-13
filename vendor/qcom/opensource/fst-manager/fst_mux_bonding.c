/*
 * FST bonding + TC based mux implementation
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
 *
 */

#include "utils/includes.h"
#include "utils/common.h"
#include "utils/list.h"
#include "common/defs.h"
#include "fst_mux.h"
#include "fst_tc.h"
#include "fst_cfgmgr.h"
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_bonding.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>

#define FST_MGR_COMPONENT "MUX"
#include "fst_manager.h"

#define BOND_ABI_VERSION 2
#define BOND_SYSFS_ROOT    "/sys/class/net"
#define BOND_SYSFS_QUEUEID "bonding/queue_id"

struct fst_mux_iface
{
	char           ifname[IFNAMSIZ + 1];
	u8             priority;
	int            queue_id;
	struct dl_list filters;
	struct dl_list lentry;
};

struct fst_mux_filter
{
	u8                          da[ETH_ALEN];
	struct fst_tc_filter_handle filter_handle;
	struct fst_mux_iface       *iface;
	struct dl_list              lentry;
};

struct fst_mux
{
	char           bond_ifname[IFNAMSIZ + 1];
	struct dl_list ifaces;
	int            skfd;
	int            queue_id;
	struct fst_tc *tc;
};

struct fst_mux_iface * _drv_get_iface_by_name(struct fst_mux *ctx,
		const char *ifname)
{
	struct fst_mux_iface *i;
	dl_list_for_each(i, &ctx->ifaces, struct fst_mux_iface, lentry) {
		if (!os_strcmp(i->ifname, ifname))
			return i;
	}

	return NULL;
}

struct fst_mux_filter *_drv_get_filter_by_da(struct fst_mux *ctx, const u8 *da)
{
	struct fst_mux_iface *i;
	dl_list_for_each(i, &ctx->ifaces, struct fst_mux_iface, lentry) {
		struct fst_mux_filter *f;
		dl_list_for_each(f, &i->filters, struct fst_mux_filter, lentry) {
			if (os_memcmp(f->da, da, ETH_ALEN) == 0) {
				return f;
			}
		}
	}

	return NULL;
}

static int
__mux_bond_get_info(struct fst_mux *ctx)
{
	struct ifreq ifr;
	struct ethtool_drvinfo info;
	char *endptr;
	int abi_ver;

	os_memset(&ifr, 0, sizeof(ifr));
	os_strlcpy(ifr.ifr_name, ctx->bond_ifname, IFNAMSIZ);
	ifr.ifr_data = (caddr_t)&info;

	info.cmd = ETHTOOL_GDRVINFO;
	os_strlcpy(info.driver, "ifenslave", 32);
	os_snprintf(info.fw_version, 32, "%d", BOND_ABI_VERSION);

	if (ioctl(ctx->skfd, SIOCETHTOOL, &ifr) < 0) {
		fst_mgr_printf(MSG_ERROR, "SIOCETHTOOL failed on %s: %s",
			ctx->bond_ifname, strerror(errno));
		return -1;
	}

	abi_ver = strtoul(info.fw_version, &endptr, 0);
	if (*endptr) {
		fst_mgr_printf(MSG_ERROR, "Invalid ABI version arrived from %s",
			ctx->bond_ifname);
		return -1;
	}

	fst_mgr_printf(MSG_DEBUG, "ABI ver is %d", abi_ver);
	return 0;
}

static int _mux_bond_assign_queue_id(struct fst_mux *ctx,
		const char *slave_ifname, int queue_id)
{
	FILE *f;
	char fname[256];
	snprintf(fname, sizeof(fname), BOND_SYSFS_ROOT "/%s/" BOND_SYSFS_QUEUEID,
			ctx->bond_ifname);
	f = fopen(fname, "w");
	if (!f) {
		fst_mgr_printf(MSG_ERROR, "cannot open queu_id file: %s",
			strerror(errno));
		return -1;
	}

	fprintf(f, "%s:%d", slave_ifname, queue_id);
	fclose(f);
	return 0;
}

static int _mux_bond_connect(struct fst_mux *ctx)
{
	short ifru_flags;

	if ((ctx->skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fst_mgr_printf(MSG_ERROR, "cannot open socket: %s",
			strerror(errno));
		goto fail;
	}

	if (__mux_bond_get_info(ctx)) {
		fst_mgr_printf(MSG_ERROR, "cannot get bond info");
		goto fail;
	}

	if (get_iface_flags(ctx->bond_ifname, &ifru_flags)) {
		fst_mgr_printf(MSG_ERROR, "cannot get bond flags");
		goto fail;
	}

	if (!(ifru_flags & IFF_MASTER)) {
		fst_mgr_printf(MSG_ERROR, "%s isn't a master",
			ctx->bond_ifname);
		goto fail;
	}

	if (!(ifru_flags & IFF_UP)) {
		fst_mgr_printf(MSG_ERROR, "%s is down",
			ctx->bond_ifname);
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int _drv_del_filter(struct fst_mux *ctx, struct fst_mux_filter *filter,
	Boolean force_removal)
{
	if (!fst_tc_del_l2da_filter(ctx->tc, &filter->filter_handle))
		fst_mgr_printf(MSG_DEBUG,
			"TC filter deleted for [" MACSTR "]",
			MAC2STR(filter->da));
	else {
		fst_mgr_printf(MSG_ERROR,
			"Cannot delete TC filter for [" MACSTR "]",
			MAC2STR(filter->da));
		if (!force_removal)
			return -1;
	}

	dl_list_del(&filter->lentry);

	/* STA can only have 1 peer (AP), so it should have no filters
	 * installed after we removed the previous one */
	WPA_ASSERT(!fst_is_supplicant() ||
		   dl_list_empty(&filter->iface->filters));

	os_free(filter);

	return 0;
}

static void _drv_purge_iface_filters(struct fst_mux *ctx,
	struct fst_mux_iface *iface)
{
	while (1) {
		struct fst_mux_filter *filter =
			dl_list_first(&iface->filters, struct fst_mux_filter, lentry);
		if (!filter)
			break;
		_drv_del_filter(ctx, filter, TRUE);
	}
}

static void _drv_purge_filters(struct fst_mux *ctx)
{
	struct fst_mux_iface *iface;
	dl_list_for_each(iface, &ctx->ifaces, struct fst_mux_iface, lentry)
		_drv_purge_iface_filters(ctx, iface);
}

void _drv_del_iface(struct fst_mux *ctx, struct fst_mux_iface *iface)
{
	dl_list_del(&iface->lentry);
	_drv_purge_iface_filters(ctx, iface);
	os_free(iface);
}

static void _drv_bond_disconnect(struct fst_mux *ctx)
{
	close(ctx->skfd);
	ctx->skfd = -1;
}

struct fst_mux *fst_mux_init(const char *group_name)
{
	struct fst_mux *ctx = NULL;
	int len;
	char buf[80];

	len = fst_cfgmgr_get_mux_type(group_name, buf, sizeof(buf)-1);
	if (len) {
		if (os_strcmp(buf, "bonding")) {
			fst_mgr_printf(MSG_ERROR, "Unsupported mux type: %s", buf);
			goto fail_mux_type;
		}
	}

	ctx = os_zalloc(sizeof(*ctx));
	if (!ctx) {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate driver for %s",
			group_name);
		goto fail_no_ctx;
	}

	len = fst_cfgmgr_get_mux_ifname(group_name, ctx->bond_ifname,
	   sizeof(ctx->bond_ifname)-1);
	if (len == 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot get mux ifname");
		goto fail_mux_name;
	}

	dl_list_init(&ctx->ifaces);
	ctx->skfd     = -1;
	ctx->queue_id = 0;

	if (_mux_bond_connect(ctx)) {
		fst_mgr_printf(MSG_ERROR, "Cannot connect to bond#%s",
			ctx->bond_ifname);
		goto fail_connect;
	}

	ctx->tc = fst_tc_create(fst_is_supplicant());
	if (!ctx->tc) {
		fst_mgr_printf(MSG_ERROR, "Cannot create FST TC bond#%s",
			ctx->bond_ifname);
		goto fail_tc_create;
	}

	fst_mgr_printf(MSG_DEBUG, "driver initialized for %s",
		ctx->bond_ifname);

	return ctx;

fail_tc_create:
	_drv_bond_disconnect(ctx);
fail_connect:
fail_mux_name:
	os_free(ctx);
fail_no_ctx:
fail_mux_type:
	return NULL;
}

int fst_mux_start(struct fst_mux *ctx)
{
	struct fst_mux_iface *i;

	dl_list_for_each(i, &ctx->ifaces, struct fst_mux_iface, lentry) {
		i->queue_id = ++ctx->queue_id;

		if (_mux_bond_assign_queue_id(ctx, i->ifname, i->queue_id)) {
			fst_mgr_printf(MSG_ERROR, "Cannot assign queue_id#%d for %s",
					i->queue_id, i->ifname);
			goto fail;

		}
	}

	if (fst_tc_start(ctx->tc, ctx->bond_ifname) != 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot start FST TC bond#%s",
			ctx->bond_ifname);
		goto fail;
	}

	fst_mgr_printf(MSG_DEBUG, "%s initialized",
		ctx->bond_ifname);
	return 0;

fail:
	dl_list_for_each(i, &ctx->ifaces, struct fst_mux_iface, lentry)
		_mux_bond_assign_queue_id(ctx, i->ifname, 0);
	return -1;
}

int fst_mux_register_iface(struct fst_mux *ctx, const char *iface_name,
		u8 priority)
{
	struct fst_mux_iface *iface;

	iface = _drv_get_iface_by_name(ctx, iface_name);
	if (iface) {
		fst_mgr_printf(MSG_ERROR, "Slave (%s) already regirestred with bond#%s",
				iface_name, ctx->bond_ifname);
		goto fail;
	}

	iface = os_zalloc(sizeof(*iface));
	if (!iface) {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate object for %s",
				iface_name);
		goto fail;
	}

	if (fst_tc_register_iface(ctx->tc, iface_name)) {
		fst_mgr_printf(MSG_ERROR, "Cannot register TC for %s",
				iface_name);
		goto fail_tc_register;
	}

	iface->priority = priority;

	dl_list_init(&iface->filters);
	os_strlcpy(iface->ifname, iface_name, sizeof(iface->ifname));

	dl_list_add_tail(&ctx->ifaces, &iface->lentry);

	fst_mgr_printf(MSG_DEBUG, "interface %s registered with %s",
		iface_name, ctx->bond_ifname);

	return 0;

fail_tc_register:
	os_free(iface);
fail:
	return -1;
}

int fst_mux_add_map_entry(struct fst_mux *ctx, const u8 *da,
		const char *iface_name)
{
	struct fst_mux_iface  *iface;
	struct fst_mux_filter *filter;

	iface = _drv_get_iface_by_name(ctx, iface_name);
	if (!iface) {
		fst_mgr_printf(MSG_ERROR, "Cannot find interface %s", iface_name);
		return -1;
	}

	fst_mux_del_map_entry(ctx, da);

	filter = os_zalloc(sizeof(*filter));
	if (!filter) {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate filter "
			"for [" MACSTR ",%s]",
			MAC2STR(da), iface_name);
		return -1;
	}

	if (fst_tc_add_l2da_filter(ctx->tc, da, iface->queue_id, iface_name,
			&filter->filter_handle)) {
		fst_mgr_printf(MSG_ERROR, "Cannot add TC filter for [" MACSTR ",%s]",
			MAC2STR(da), iface_name);
		os_free(filter);
		return -1;
	}

	os_memcpy(filter->da, da, ETH_ALEN);
	filter->iface = iface;
	dl_list_add_tail(&iface->filters, &filter->lentry);

	fst_mgr_printf(MSG_DEBUG, "TC filter added for [" MACSTR ",%s]",
		MAC2STR(da), iface_name);

	return 0;
}

int fst_mux_del_map_entry(struct fst_mux *ctx, const u8 *da)
{
	struct fst_mux_filter *filter;

	filter = _drv_get_filter_by_da(ctx, da);
	if (!filter) {
		fst_mgr_printf(MSG_ERROR, "Cannot find TC filter for [" MACSTR "]",
			MAC2STR(da));
		return -1;
	}

	return _drv_del_filter(ctx, filter, FALSE);
}

void fst_mux_unregister_iface(struct fst_mux *ctx, const char *iface_name)
{
	struct fst_mux_iface *iface;

	fst_tc_unregister_iface(ctx->tc, iface_name);

	iface = _drv_get_iface_by_name(ctx, iface_name);
	if (iface) {
		_drv_del_iface(ctx, iface);
		fst_mgr_printf(MSG_DEBUG, "interface %s unregistered with %s",
			iface_name, ctx->bond_ifname);
	}
}

void fst_mux_stop(struct fst_mux *ctx)
{
	struct fst_mux_iface *i;
	_drv_purge_filters(ctx);
	fst_tc_stop(ctx->tc);
	dl_list_for_each(i, &ctx->ifaces, struct fst_mux_iface, lentry) {
		_mux_bond_assign_queue_id(ctx, i->ifname, 0);
	}
	ctx->queue_id = 0;
}

void fst_mux_cleanup(struct fst_mux *ctx)
{
	while (TRUE) {
		struct fst_mux_iface *iface =
			dl_list_first(&ctx->ifaces, struct fst_mux_iface, lentry);
		if (!iface)
			break;
		_drv_del_iface(ctx, iface);
	}
	fst_tc_delete(ctx->tc);
	_drv_bond_disconnect(ctx);
	fst_mgr_printf(MSG_DEBUG, "driver cleaned up for %s",
		ctx->bond_ifname);
	os_free(ctx);
}
