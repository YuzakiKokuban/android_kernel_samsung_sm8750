/*
 * FST Manager: INI configuration file parser
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

#include <string.h>
#include <stdlib.h>
#include "utils/list.h"
#include "fst_ini_conf.h"
#include "ini.h"

#define FST_MGR_COMPONENT "INI_CONF"
#include "fst_manager.h"

struct fst_ini_config
{
	char *filename;
	struct cfg_cache *cache;
};

struct cfg_ctx {
	const char *section;
	const char *name;
	char       *buffer;
	int        buflen;
	Boolean    is_found;
};

/*
 * FST Manager standalone configuration
 */

/* matches the ini file limit */
#define MAX_CFG_STRING 80

struct cfg_cache {
	char ctrl_iface[MAX_CFG_STRING];
	char slave_ctrl_interface[MAX_CFG_STRING];
	struct dl_list groups;
};

struct cfg_group_info {
	char name[FST_MAX_GROUP_ID_SIZE];
	char rate_upgrade_master[MAX_CFG_STRING];
	char rate_upgrade_acl_fname[MAX_CFG_STRING];
	char mux_type[MAX_CFG_STRING];
	char mux_ifname[MAX_CFG_STRING];
	char l2da_ap_default_ifname[MAX_CFG_STRING];
	Boolean is_mux_managed;
	int txqueuelen;
	struct dl_list ifaces;
	struct dl_list list;
};

struct cfg_iface_info {
	struct fst_iface_info info;
	char group_cipher[MAX_CFG_STRING];
	char pairwise_cipher[MAX_CFG_STRING];
	char hw_mode[MAX_CFG_STRING];
	char channel[MAX_CFG_STRING];
	struct dl_list list;
};

int fst_ini_config_get_ctrl_iface(struct fst_ini_config *h, char *buf, int size)
{
	if (!h->cache || !h->cache->ctrl_iface[0]) {
		fst_mgr_printf(MSG_ERROR,
			"No ctrl_iface found in fst_manager config");
		return -1;
	}

	os_strlcpy(buf, h->cache->ctrl_iface, size);
	return 0;
}

static struct cfg_group_info *fst_ini_find_cfg_group(struct fst_ini_config *h,
						     const char *id)
{
	struct cfg_group_info *item;

	if (!h->cache)
		return NULL;

	dl_list_for_each(item, &h->cache->groups,
			 struct cfg_group_info, list) {
		if (!strncmp(item->name, id, FST_MAX_GROUP_ID_SIZE))
			return item;
	}

	return NULL;
}

int fst_ini_config_get_group_ifaces(struct fst_ini_config *h,
	const struct fst_group_info *group, struct fst_iface_info **ifaces)
{
	struct cfg_group_info *cginfo = NULL;
	struct cfg_iface_info *item;
	int i, cnt;

	if (!h->cache) {
		fst_mgr_printf(MSG_ERROR, "no config cache");
		return -1;
	}

	cginfo = fst_ini_find_cfg_group(h, group->id);
	if (!cginfo) {
		fst_mgr_printf(MSG_ERROR, "group not found");
		return -1;
	}

	cnt = dl_list_len(&cginfo->ifaces);
	if (!cnt) {
		fst_mgr_printf(MSG_ERROR,
			"No interfaces found for group %s", group->id);
		return -1;
	}

	*ifaces = (struct fst_iface_info*)os_zalloc(
		sizeof(struct fst_iface_info) * cnt);
	if (*ifaces == NULL) {
		fst_mgr_printf(MSG_ERROR,
			"Cannot allocate memory for interface");
		return -1;
	}
	item = dl_list_first(&cginfo->ifaces, struct cfg_iface_info, list);
	for (i = 0; i < cnt; i++) {
		(*ifaces)[i] = item->info;
		item = dl_list_entry(item->list.next, struct cfg_iface_info,
				     list);
	}
	return cnt;
}

int fst_ini_config_get_groups(struct fst_ini_config *h,
	struct fst_group_info **groups)
{
	int i, cnt;
	struct cfg_group_info *item;

	*groups = NULL;
	if (!h->cache) {
		fst_mgr_printf(MSG_ERROR, "no config cache");
		return -1;
	}

	cnt = dl_list_len(&h->cache->groups);
	if (cnt == 0) {
		fst_mgr_printf(MSG_ERROR,
			"No groups defined in config");
		return -1;
	}
	*groups = (struct fst_group_info*)malloc(
		sizeof(struct fst_group_info) * cnt);
	if (*groups == NULL) {
		fst_mgr_printf(MSG_ERROR,"Cannot allocate memory for groups");
		return -1;
	}

	item = dl_list_first(&h->cache->groups, struct cfg_group_info, list);
	for (i = 0; i < cnt; i++) {
		os_strlcpy((*groups)[i].id, item->name,
			   sizeof((*groups)[i].id));
		item = dl_list_entry(item->list.next, struct cfg_group_info,
				     list);
	}

	return cnt;
}

char *fst_ini_config_get_rate_upgrade_master(struct fst_ini_config *h,
	const char *groupname)
{
	struct cfg_group_info *cginfo;

	cginfo = fst_ini_find_cfg_group(h, groupname);
	if (!cginfo) {
		fst_mgr_printf(MSG_ERROR, "group not found");
		return NULL;
	}

	if (!cginfo->rate_upgrade_master[0])
		return NULL;

	return strdup(cginfo->rate_upgrade_master);
}

char *fst_ini_config_get_rate_upgrade_acl_fname(struct fst_ini_config *h,
	const char *groupname)
{
	struct cfg_group_info *cginfo;

	cginfo = fst_ini_find_cfg_group(h, groupname);
	if (!cginfo) {
		fst_mgr_printf(MSG_ERROR, "group not found");
		return NULL;
	}

	if (!cginfo->rate_upgrade_acl_fname[0])
		return NULL;

	return strdup(cginfo->rate_upgrade_acl_fname);
}

int fst_ini_config_get_mux_type(struct fst_ini_config *h,
	const char *gname, char *buf, int buflen)
{
	struct cfg_group_info *cginfo;

	cginfo = fst_ini_find_cfg_group(h, gname);
	if (!cginfo) {
		fst_mgr_printf(MSG_ERROR, "group not found");
		return 0;
	}

	os_strlcpy(buf, cginfo->mux_type, buflen);

	return strlen(buf);
}


int fst_ini_config_get_mux_ifname(struct fst_ini_config *h,
	const char *gname, char *buf, int buflen)
{
	struct cfg_group_info *cginfo;

	cginfo = fst_ini_find_cfg_group(h, gname);
	if (!cginfo) {
		fst_mgr_printf(MSG_ERROR, "group not found");
		return 0;
	}

	os_strlcpy(buf, cginfo->mux_ifname, buflen);

	return strlen(buf);
}

int fst_ini_config_get_l2da_ap_default_ifname(struct fst_ini_config *h,
	const char *gname, char *buf, int buflen)
{
	struct cfg_group_info *cginfo;

	cginfo = fst_ini_find_cfg_group(h, gname);
	if (!cginfo) {
		fst_mgr_printf(MSG_ERROR, "group not found");
		return 0;
	}

	os_strlcpy(buf, cginfo->l2da_ap_default_ifname, buflen);

	return strlen(buf);
}

Boolean fst_ini_config_is_mux_managed(struct fst_ini_config *h, const char *gname)
{
	struct cfg_group_info *cginfo;

	cginfo = fst_ini_find_cfg_group(h, gname);
	if (!cginfo) {
		fst_mgr_printf(MSG_ERROR, "group not found");
		return FALSE;
	}

	return cginfo->is_mux_managed;
}

static struct cfg_iface_info *fst_ini_find_cfg_iface(struct fst_ini_config *h,
	const char *ifname)
{
	struct cfg_group_info *cginfo;
	struct cfg_iface_info *ifinfo;

	if (!h->cache)
		return NULL;

	dl_list_for_each(cginfo, &h->cache->groups,
			 struct cfg_group_info, list) {
		dl_list_for_each(ifinfo, &cginfo->ifaces,
				 struct cfg_iface_info, list) {
			if (!strncmp(ifname, ifinfo->info.name,
				     FST_MAX_INTERFACE_SIZE))
				return ifinfo;
		}
	}

	return NULL;
}

int fst_ini_config_get_iface_group_cipher(struct fst_ini_config *h,
	const struct fst_iface_info *iface, char *buf, int len)
{
	struct cfg_iface_info *ifinfo;

	ifinfo = fst_ini_find_cfg_iface(h, iface->name);
	if (!ifinfo) {
		fst_mgr_printf(MSG_ERROR, "interface not found");
		return 0;
	}

	os_strlcpy(buf, ifinfo->group_cipher, len);
	return strlen(buf);
}

int fst_ini_config_get_iface_pairwise_cipher(struct fst_ini_config *h,
	const struct fst_iface_info *iface, char *buf, int len)
{
	struct cfg_iface_info *ifinfo;

	ifinfo = fst_ini_find_cfg_iface(h, iface->name);
	if (!ifinfo) {
		fst_mgr_printf(MSG_ERROR, "interface not found");
		return 0;
	}

	os_strlcpy(buf, ifinfo->pairwise_cipher, len);
	return strlen(buf);
}

int fst_ini_config_get_iface_hw_mode(struct fst_ini_config *h,
	const struct fst_iface_info *iface, char *buf, int len)
{
	struct cfg_iface_info *ifinfo;

	ifinfo = fst_ini_find_cfg_iface(h, iface->name);
	if (!ifinfo) {
		fst_mgr_printf(MSG_ERROR, "interface not found");
		return 0;
	}

	os_strlcpy(buf, ifinfo->hw_mode, len);
	return strlen(buf);
}

int fst_ini_config_get_iface_channel(struct fst_ini_config *h,
	const struct fst_iface_info *iface, char *buf, int len)
{
	struct cfg_iface_info *ifinfo;

	ifinfo = fst_ini_find_cfg_iface(h, iface->name);
	if (!ifinfo) {
		fst_mgr_printf(MSG_ERROR, "interface not found");
		return 0;
	}

	os_strlcpy(buf, ifinfo->channel, len);
	return strlen(buf);
}

int fst_ini_config_get_txqueuelen(struct fst_ini_config *h, const char *gname)
{
	struct cfg_group_info *cginfo;

	cginfo = fst_ini_find_cfg_group(h, gname);
	if (!cginfo) {
		fst_mgr_printf(MSG_ERROR, "group not found");
		return -1;
	}

	return cginfo->txqueuelen;
}

int fst_ini_config_get_slave_ctrl_interface(struct fst_ini_config *h, char *buf, int len)
{
	if (!h->cache || !h->cache->ctrl_iface[0]) {
		fst_mgr_printf(MSG_ERROR,
			"No slave_ctrl_iface found in fst_manager config");
		return 0;
	}

	os_strlcpy(buf, h->cache->slave_ctrl_interface, len);
	return 0;
}

int fst_ini_config_set_mux_ifname(struct fst_ini_config *h,
	const char *gname, const char *newname)
{
	struct cfg_group_info *cginfo;

	cginfo = fst_ini_find_cfg_group(h, gname);
	if (!cginfo) {
		fst_mgr_printf(MSG_ERROR, "group not found");
		return -1;
	}

	os_strlcpy(cginfo->mux_ifname, newname, sizeof(cginfo->mux_ifname));

	return 0;
}

int fst_ini_config_rename_interface(struct fst_ini_config *h,
	const char *gname, const char *ifname, const char *newname)
{
	struct cfg_iface_info *ifinfo;
	struct cfg_group_info *cginfo;

	cginfo = fst_ini_find_cfg_group(h, gname);
	if (!cginfo) {
		fst_mgr_printf(MSG_ERROR, "group not found");
		return -1;
	}

	ifinfo = fst_ini_find_cfg_iface(h, newname);
	if (ifinfo) {
		fst_mgr_printf(MSG_ERROR, "new interface %s already exists",
			       newname);
		return -1;
	}
	ifinfo = fst_ini_find_cfg_iface(h, ifname);
	if (!ifinfo) {
		fst_mgr_printf(MSG_ERROR, "interface %s not found", ifname);
		return -1;
	}

	os_strlcpy(ifinfo->info.name, newname, sizeof(ifinfo->info.name));
	if (!os_strcmp(cginfo->rate_upgrade_master, ifname)) {
		os_strlcpy(cginfo->rate_upgrade_master, newname, sizeof(cginfo->rate_upgrade_master));
	}

	return 0;
}

/*
 * Reading the INI file
 */

static int ini_handler(void* user, const char* section, const char* name,
                   const char* value)
{
	struct cfg_ctx *ctx = (struct cfg_ctx*)user;
	if (!ctx->is_found) {
		if (strncmp(section, ctx->section, strlen(section)) == 0 &&
			strncmp(name, ctx->name, strlen(name)) == 0) {
			ctx->is_found = TRUE;
			if (ctx->buffer && ctx->buflen) {
				os_strlcpy(ctx->buffer, value, ctx->buflen);
			}
		}
	}
	return 1;
}

static int parse_csv(char *string, char **tokens, int tokenslen)
{
	const char *delim = " ,\t";
	char *tokbuf;
	int count = 0;
	char *token = strtok_r(string, delim, &tokbuf);
	while (token != NULL && count < tokenslen) {
		count++;
		*tokens++ = token;
		token = strtok_r(NULL, delim, &tokbuf);
	}
	return count;
}

static Boolean fst_ini_config_read(struct fst_ini_config *h, const char *s,
	const char *k, char *buf, int buflen)
{
	struct cfg_ctx ctx;
	ctx.section = s;
	ctx.name = k;
	ctx.buffer = buf;
	ctx.buflen = buflen;
	ctx.is_found = FALSE;

	if (ini_parse(h->filename, ini_handler, &ctx) < 0) {
		fst_mgr_printf(MSG_ERROR, "Error parsing configuration file %s",
			h->filename);
	}
	return ctx.is_found;
}

static int fst_ini_cache_config(struct fst_ini_config *h);
static void fst_ini_free_config_cache(struct cfg_cache *cache);

struct fst_ini_config *fst_ini_config_init(const char *filename)
{
	struct fst_ini_config *h = (struct fst_ini_config*)os_zalloc(
		sizeof(struct fst_ini_config));
	if (h == NULL) {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate memory for config");
		return NULL;
	}
	h->filename = strdup(filename);
	if (h->filename == NULL) {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate memory for filename");
		goto out_err;
	}
	if (fst_ini_cache_config(h) < 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot cache ini configuration");
		goto out_err;
	}

	return h;

out_err:
	fst_ini_config_deinit(h);
	return NULL;
}

void fst_ini_config_deinit(struct fst_ini_config *h)
{
	if (!h)
		return;

	fst_ini_free_config_cache(h->cache);
	free(h->filename);
	free(h);
}

/*
 * FST Manager standalone configuration - used for caching only
 */
#define INI_MAX_STRING 80
#define INI_MAX_TOKENS 16

static int __fst_ini_config_get_ctrl_iface(struct fst_ini_config *h,
	char *buf, int size)
{
	if (!fst_ini_config_read(h, "fst_manager", "ctrl_iface",
		buf, size)) {
		fst_mgr_printf(MSG_ERROR,
			"No ctrl_iface found in fst_manager config");
		return -1;
	}

	return 0;
}

static int __fst_ini_config_get_group_ifaces(struct fst_ini_config *h,
	const struct fst_group_info *group, struct fst_iface_info **ifaces)
{
	char buf[INI_MAX_STRING+1], buf_ifaces[INI_MAX_STRING+1];
	char *tokens[INI_MAX_TOKENS];
	int i, cnt;

	if (!fst_ini_config_read(h, (const char*)group->id, "interfaces",
		buf_ifaces, INI_MAX_STRING)) {
		fst_mgr_printf(MSG_ERROR,
			"No interfaces key for group %s", group->id);
		return -1;
	}

	cnt = parse_csv(buf_ifaces, tokens, INI_MAX_TOKENS);
	if (cnt == 0) {
		fst_mgr_printf(MSG_ERROR,
			"No interfaces found for group %s", group->id);
		return -1;
	}
	fst_mgr_printf(MSG_INFO,"%d interfaces found", cnt);

	*ifaces = (struct fst_iface_info*)os_zalloc(
		sizeof(struct fst_iface_info) * cnt);
	if (*ifaces == NULL) {
		fst_mgr_printf(MSG_ERROR,
			"Cannot allocate memory for interface");
		return -1;
	}
	for (i = 0; i < cnt; i++) {
		os_strlcpy((*ifaces)[i].name, tokens[i], sizeof((*ifaces)[i].name));
		if (fst_ini_config_read(h, (const char*)tokens[i], "priority",
			buf, INI_MAX_STRING))
			(*ifaces)[i].priority = strtoul(buf, NULL, 0);
		if (fst_ini_config_read(h, (const char*)tokens[i], "default_llt",
			buf, INI_MAX_STRING))
			(*ifaces)[i].llt = strtoul(buf, NULL, 0);
		if (fst_ini_config_read(h, (const char*)tokens[i], "manual_enslave",
			buf, INI_MAX_STRING))
			(*ifaces)[i].manual_enslave = strtoul(buf, NULL, 0);
		fst_mgr_printf(MSG_DEBUG,
			"iface %s (pri=%d, llt=%d manual_enslave=%d) has been parsed",
			(*ifaces)[i].name, (*ifaces)[i].priority, (*ifaces)[i].llt, (*ifaces)[i].manual_enslave);
	}
	return cnt;
}

static int __fst_ini_config_get_groups(struct fst_ini_config *h,
	struct fst_group_info **groups)
{
	char buf[INI_MAX_STRING+1];
	char *tokens[INI_MAX_TOKENS];
	int i, cnt;

	*groups = NULL;
	if (!fst_ini_config_read(h, "fst_manager", "groups",
		buf, INI_MAX_STRING)) {
		fst_mgr_printf(MSG_ERROR,
			"No groups key found in fst_manager config");
		return -1;
	}

	cnt = parse_csv(buf, tokens, INI_MAX_TOKENS);
	if (cnt == 0) {
		fst_mgr_printf(MSG_ERROR,
			"No groups defined in config");
		return -1;
	}
	*groups = (struct fst_group_info*)malloc(
		sizeof(struct fst_group_info) * cnt);
	if (*groups == NULL) {
		fst_mgr_printf(MSG_ERROR,"Cannot allocate memory for groups");
		return -1;
	}

	for (i = 0; i < cnt; i++) {
		os_strlcpy((*groups)[i].id, tokens[i], sizeof((*groups)[i].id));
		fst_mgr_printf(MSG_DEBUG,"Config: group %s has been parsed", tokens[i]);
	}
	return cnt;
}

static char *__fst_ini_config_get_rate_upgrade_master(struct fst_ini_config *h,
	const char *groupname)
{
	char buf[INI_MAX_STRING+1];
	if(!fst_ini_config_read(h, groupname, "rate_upgrade_master", buf,
	   INI_MAX_STRING))
		return NULL;
	return strdup(buf);
}

static char *__fst_ini_config_get_rate_upgrade_acl_fname(struct fst_ini_config *h,
	const char *groupname)
{
	char buf[INI_MAX_STRING+1];
	if(!fst_ini_config_read(h, groupname, "rate_upgrade_acl_file", buf,
	   INI_MAX_STRING))
		return NULL;
	return strdup(buf);
}

/* this is part of the public API, it does not access the INI directly */
int fst_ini_config_get_group_slave_ifaces(struct fst_ini_config *h,
	const struct fst_group_info *group, const char *master,
	struct fst_iface_info **ifaces)
{
	int i, cnt;

	fst_mgr_printf(MSG_INFO,"group %s master %s", group->id, master);

	cnt = fst_ini_config_get_group_ifaces(h, group, ifaces);
	if (cnt < 0)
		return cnt;

	for (i=0; i < cnt; i++) {
		if (!strncmp((*ifaces)[i].name, master, strlen(master))) {
			/* Skip the master interface */
			cnt--;
			if (i < cnt)
				os_memmove(&(*ifaces)[i], &(*ifaces)[i+1],
					(cnt-i) * sizeof(struct fst_iface_info));
			return cnt;
		}
	}
	return cnt;
}

static int __fst_ini_config_get_mux_type(struct fst_ini_config *h,
	const char *gname, char *buf, int buflen)
{
	if(!fst_ini_config_read(h, gname, "mux_type", buf, buflen))
		return 0;
	return strlen(buf);
}


static int __fst_ini_config_get_mux_ifname(struct fst_ini_config *h,
	const char *gname, char *buf, int buflen)
{
	if(!fst_ini_config_read(h, gname, "mux_ifname", buf, buflen))
		return 0;
	return strlen(buf);
}

static int __fst_ini_config_get_l2da_ap_default_ifname(struct fst_ini_config *h,
	const char *gname, char *buf, int buflen)
{
	if(!fst_ini_config_read(h, gname, "l2da_ap_default_ifname", buf, buflen))
		return 0;
	return strlen(buf);
}

static Boolean __fst_ini_config_is_mux_managed(struct fst_ini_config *h, const char *gname)
{
	char buf[INI_MAX_STRING+1];
	if(!fst_ini_config_read(h, gname, "mux_managed", buf,
	   INI_MAX_STRING))
		return FALSE;
	if (buf[0] == '1' || buf[0] == 'y' || buf[0] == 'Y')
		return TRUE;
	else if (buf[0] == '0' || buf[0] == 'n' || buf[0] == 'N')
		return FALSE;
	else {
		fst_mgr_printf(MSG_ERROR,
			"mux_managed wrong boolean %s, assuming false", buf);
		return FALSE;
	}
}

static int __fst_ini_config_get_iface_group_cipher(struct fst_ini_config *h,
	const struct fst_iface_info *iface, char *buf, int len)
{
	if(!fst_ini_config_read(h, iface->name, "wpa_group", buf, len))
		return 0;
	return strlen(buf);
}

static int __fst_ini_config_get_iface_pairwise_cipher(struct fst_ini_config *h,
	const struct fst_iface_info *iface, char *buf, int len)
{
	if(!fst_ini_config_read(h, iface->name, "wpa_pairwise", buf, len))
		return 0;
	return strlen(buf);
}

static int __fst_ini_config_get_iface_hw_mode(struct fst_ini_config *h,
	const struct fst_iface_info *iface, char *buf, int len)
{
	if(!fst_ini_config_read(h, iface->name, "hw_mode", buf, len))
		return 0;
	return strlen(buf);
}

static int __fst_ini_config_get_iface_channel(struct fst_ini_config *h,
	const struct fst_iface_info *iface, char *buf, int len)
{
	if(!fst_ini_config_read(h, iface->name, "channel", buf, len))
		return 0;
	return strlen(buf);
}

static int __fst_ini_config_get_txqueuelen(struct fst_ini_config *h, const char *gname)
{
	char buf[INI_MAX_STRING + 1];

	if (!fst_ini_config_read(h, gname, "txqueuelen", buf, INI_MAX_STRING))
		return -1;

	return atoi(buf);
}

static int __fst_ini_config_get_slave_ctrl_interface(struct fst_ini_config *h,
	char *buf, int len)
{
	if(!fst_ini_config_read(h, "fst_manager", "slave_ctrl_iface_dir", buf, len))
		return 0;
	return strlen(buf);
}

/*
 * Caching the INI file
 */

static int fst_ini_cache_iface(struct fst_ini_config *h,
    struct fst_iface_info *fiinfo, struct cfg_iface_info **out_ciinfo)
{
	struct cfg_iface_info *ciinfo;

	ciinfo = (struct cfg_iface_info *)os_zalloc(
	    sizeof(struct cfg_iface_info));
	if (!ciinfo) {
		fst_mgr_printf(MSG_ERROR, "cache_iface: fail to allocate cfg_iface_info");
		return -1;
	}

	ciinfo->info = *fiinfo;
	__fst_ini_config_get_iface_group_cipher(h, fiinfo, ciinfo->group_cipher,
		sizeof(ciinfo->group_cipher));
	__fst_ini_config_get_iface_pairwise_cipher(h, fiinfo,
		ciinfo->pairwise_cipher, sizeof(ciinfo->pairwise_cipher));
	__fst_ini_config_get_iface_hw_mode(h, fiinfo, ciinfo->hw_mode, sizeof(ciinfo->hw_mode));
	__fst_ini_config_get_iface_channel(h, fiinfo, ciinfo->channel, sizeof(ciinfo->channel));

	*out_ciinfo = ciinfo;
	return 0;
}

static int fst_ini_cache_ifaces(struct fst_ini_config *h,
    struct fst_group_info *fginfo, struct cfg_group_info *cginfo)
{
	struct fst_iface_info *ifaces = NULL;
	int nof_ifaces, i, res;
	struct cfg_iface_info *ifinfo = NULL;

	res = __fst_ini_config_get_group_ifaces(h, fginfo, &ifaces);
	if (res < 0) {
		fst_mgr_printf(MSG_ERROR, "cache_ifaces: fail to get ifaces");
		return -1;
	}

	nof_ifaces = res;
	res = 0;
	for (i = 0; i < nof_ifaces; i++) {
		if (fst_ini_cache_iface(h, &ifaces[i], &ifinfo) < 0) {
			res = -1;
			break;
		}
		dl_list_add_tail(&cginfo->ifaces, &ifinfo->list);
	}

	free(ifaces);
	/* on error, anything already added to cache will be freed by caller */
	return res;
}

static void fst_ini_free_group_info(struct cfg_group_info *cginfo)
{
	struct cfg_iface_info *ifinfo, *tmp;

	if (!cginfo)
		return;

	dl_list_for_each_safe(ifinfo, tmp, &cginfo->ifaces,
			      struct cfg_iface_info, list) {
		dl_list_del(&ifinfo->list);
		free(ifinfo);
	}
	free(cginfo);
}

static int fst_ini_cache_group(
    struct fst_ini_config *h, struct fst_group_info *fginfo,
    struct cfg_group_info **out_cginfo)
{
	struct cfg_group_info *cginfo;
	char *str;

	cginfo = (struct cfg_group_info *)os_zalloc(
	    sizeof(struct cfg_group_info));
	if (!cginfo) {
		fst_mgr_printf(MSG_ERROR, "fail to allocate cfg_group_info");
		return -1;
	}

	dl_list_init(&cginfo->ifaces);

	os_strlcpy(cginfo->name, fginfo->id, sizeof(cginfo->name));

	str = __fst_ini_config_get_rate_upgrade_master(h, cginfo->name);
	if (str) {
		os_strlcpy(cginfo->rate_upgrade_master, str,
			   sizeof(cginfo->rate_upgrade_master));
		free(str);
	}
	str = __fst_ini_config_get_rate_upgrade_acl_fname(h, cginfo->name);
	if (str) {
		os_strlcpy(cginfo->rate_upgrade_acl_fname, str,
			   sizeof(cginfo->rate_upgrade_acl_fname));
		free(str);
	}
	__fst_ini_config_get_mux_type(h, cginfo->name, cginfo->mux_type,
		sizeof(cginfo->mux_type));
	__fst_ini_config_get_mux_ifname(h, cginfo->name, cginfo->mux_ifname,
		sizeof(cginfo->mux_ifname));
	__fst_ini_config_get_l2da_ap_default_ifname(h, cginfo->name,
		cginfo->l2da_ap_default_ifname,
		sizeof(cginfo->l2da_ap_default_ifname));
	cginfo->is_mux_managed = __fst_ini_config_is_mux_managed(h, cginfo->name);
	cginfo->txqueuelen = __fst_ini_config_get_txqueuelen(h, cginfo->name);

	if (fst_ini_cache_ifaces(h, fginfo, cginfo) < 0)
		goto out_err;

	*out_cginfo = cginfo;
	return 0;

out_err:
	fst_ini_free_group_info(cginfo);
	return -1;
}

static int fst_ini_cache_groups(struct fst_ini_config *h, struct cfg_cache *cache)
{
	struct fst_group_info *groups = NULL;
	struct cfg_group_info *ginfo = NULL;
	int res, nof_groups, i;

	res = __fst_ini_config_get_groups(h, &groups);
	if (res < 0) {
		fst_mgr_printf(MSG_ERROR, "fail to get groups");
		return -1;
	}

	nof_groups = res;
	res = 0;
	for (i = 0; i < nof_groups; i++) {
		res = fst_ini_cache_group(h, &groups[i], &ginfo);
		if (res < 0)
			break;
		dl_list_add_tail(&cache->groups, &ginfo->list);
	}

	free(groups);
	/* on error, anything already added to cache will be freed by caller */
	return res;
}

static void fst_ini_free_config_cache(struct cfg_cache *cache)
{
	struct cfg_group_info *cginfo, *tmp;

	if (!cache)
		return;

	dl_list_for_each_safe(cginfo, tmp, &cache->groups,
			      struct cfg_group_info, list) {
		dl_list_del(&cginfo->list);
		fst_ini_free_group_info(cginfo);
	}
	free(cache);
}

static int fst_ini_cache_config(struct fst_ini_config *h)
{
	struct cfg_cache *cache;

	cache = (struct cfg_cache *)os_zalloc(sizeof(struct cfg_cache));
	if (!cache) {
		fst_mgr_printf(MSG_ERROR, "fail to allocate cfg_cache");
		return -1;
	}

	dl_list_init(&cache->groups);

	__fst_ini_config_get_ctrl_iface(h, cache->ctrl_iface,
		sizeof(cache->ctrl_iface));

	__fst_ini_config_get_slave_ctrl_interface(h,
		cache->slave_ctrl_interface,
		sizeof(cache->slave_ctrl_interface));

	if (fst_ini_cache_groups(h, cache) < 0)
		goto out_err;

	h->cache = cache;
	return 0;

out_err:
	fst_ini_free_config_cache(cache);
	return -1;
}
