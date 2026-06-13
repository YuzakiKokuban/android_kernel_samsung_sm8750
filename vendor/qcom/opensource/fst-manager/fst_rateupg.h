/*
 * FST Manager: Rate Upgrade
 *
 * Copyright (c) 2015-2016, 2019, The Linux Foundation. All rights reserved.
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

#ifndef __FST_RATEUPG_H__
#define __FST_RATEUPG_H__
#include "fst_ini_conf.h"

int fst_rate_upgrade_init(struct fst_ini_config *h);
void fst_rate_upgrade_deinit();
int fst_rate_upgrade_add_group(const struct fst_group_info *group);
int fst_rate_upgrade_del_group(const struct fst_group_info *group);
int fst_rate_upgrade_on_connect(const struct fst_group_info *group,
	const char *iface, const u8* addr);
int fst_rate_upgrade_on_disconnect(const struct fst_group_info *group,
	const char *iface, const u8* addr);
/**
 * fst_rate_upgrade_on_switch_completed - called after successful session
 * switch. This function will trigger disconnect from peer on old_iface if it's
 * not the group's master interface
 *
 * @group: FST group of the session
 * @old_iface: interface that we switch from
 * @new_iface: interface that we switch to
 * @old_peer_addr: peer's MAC address on old_iface
 */
void fst_rate_upgrade_on_switch_completed(const struct fst_group_info *group,
	const char *old_iface, const char *new_iface, const u8* old_peer_addr);

void fst_rate_upgrade_on_release(const struct fst_group_info *group,
	const char *iface);
void fst_rate_upgrade_rename_interface(const char *gname, const char *ifname, const char *newifname);
void fst_rate_upgrade_high_traffic(const struct fst_group_info *group);
void fst_rate_upgrade_low_traffic(const struct fst_group_info *group);
void fst_rate_upgrade_on_scan_started(const struct fst_group_info *group,
	const char *iface);
void fst_rate_upgrade_on_scan_completed(const struct fst_group_info *group,
	const char *iface);
void fst_rate_upgrade_on_signal_change(const struct fst_group_info *group,
	const char *iface);

#endif /* __FST_RATEUPG_H */
