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

#ifndef __FST_INI_CONF_H__
#define __FST_INI_CONF_H__
#include "utils/common.h"
#include "fst_ctrl.h"

struct fst_ini_config;

struct fst_ini_config *fst_ini_config_init(const char *filename);
void fst_ini_config_deinit(struct fst_ini_config *h);

int fst_ini_config_get_ctrl_iface(struct fst_ini_config *h,
	char *buf, int size);
int fst_ini_config_get_group_ifaces(struct fst_ini_config *h,
	const struct fst_group_info *group,
	struct fst_iface_info **ifaces);
int fst_ini_config_get_groups(struct fst_ini_config *h,
	struct fst_group_info **groups);
char *fst_ini_config_get_rate_upgrade_master(struct fst_ini_config *h,
	const char *groupname);
char *fst_ini_config_get_rate_upgrade_acl_fname(struct fst_ini_config *h,
	const char *groupname);
int fst_ini_config_get_group_slave_ifaces(struct fst_ini_config *h,
	const struct fst_group_info *group, const char *master,
	struct fst_iface_info **ifaces);
int fst_ini_config_get_mux_type(struct fst_ini_config *h,
	const char *gname, char *buf, int buflen);
int fst_ini_config_get_mux_ifname(struct fst_ini_config *h,
	const char *gname, char *buf, int buflen);
int fst_ini_config_get_l2da_ap_default_ifname(struct fst_ini_config *h,
	const char *gname, char *buf, int buflen);
Boolean fst_ini_config_is_mux_managed(struct fst_ini_config *h,
	const char *gname);
int fst_ini_config_get_iface_group_cipher(struct fst_ini_config *h,
	const struct fst_iface_info *iface, char *buf, int len);
int fst_ini_config_get_iface_pairwise_cipher(struct fst_ini_config *h,
	const struct fst_iface_info *iface, char *buf, int len);
int fst_ini_config_get_iface_hw_mode(struct fst_ini_config *h,
	const struct fst_iface_info *iface, char *buf, int len);
int fst_ini_config_get_iface_channel(struct fst_ini_config *h,
	const struct fst_iface_info *iface, char *buf, int len);
int fst_ini_config_get_txqueuelen(struct fst_ini_config *h, const char *gname);
int fst_ini_config_get_slave_ctrl_interface(struct fst_ini_config *h,
	char *buf, int len);

int fst_ini_config_set_mux_ifname(struct fst_ini_config *h,
	const char *gname, const char *newname);
int fst_ini_config_rename_interface(struct fst_ini_config *h,
	const char *gname, const char *ifname, const char *newname);

#endif /*  __FST_INI_CONF_H__ */

