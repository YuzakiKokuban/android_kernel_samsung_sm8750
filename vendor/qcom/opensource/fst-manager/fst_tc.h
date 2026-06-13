/*
 * FST TC related routines definitions
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

#ifndef __FST_TC_H__
#define __FST_TC_H__

#include "common/defs.h"

struct fst_tc;

struct fst_tc * fst_tc_create(Boolean is_sta);
int fst_tc_start(struct fst_tc *f, const char *ifname);
void fst_tc_stop(struct fst_tc *f);
void fst_tc_delete(struct fst_tc *f);

int fst_tc_register_iface(struct fst_tc *f, const char *ifname);
void fst_tc_unregister_iface(struct fst_tc *f, const char *ifname);

struct fst_tc_filter_handle {
	u16  prio;
	char ifname[IFNAMSIZ + 1];
	struct dl_list filters_lentry;
};

int fst_tc_add_l2da_filter(struct fst_tc *f, const u8 *mac, int queue_id,
	const char *ifname, struct fst_tc_filter_handle *filter_handle);
int fst_tc_del_l2da_filter(struct fst_tc *f,
	struct fst_tc_filter_handle *filter_handle);

#endif /* __FST_TC_H__ */
