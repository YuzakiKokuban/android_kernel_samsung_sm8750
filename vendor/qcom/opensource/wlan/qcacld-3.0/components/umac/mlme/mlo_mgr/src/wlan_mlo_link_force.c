/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * DOC: contains ML STA link force active/inactive related functionality
 */
#include "wlan_mlo_link_force.h"
#include "wlan_mlo_mgr_sta.h"
#include "wlan_policy_mgr_api.h"
#include "wlan_policy_mgr_i.h"
#include "wlan_cm_roam_public_struct.h"
#include "wlan_cm_roam_api.h"
#include "wlan_mlo_mgr_roam.h"
#include "wlan_mlme_main.h"
#include "wlan_mlo_mgr_link_switch.h"
#include "target_if.h"
#include "wlan_nan_api.h"
#include "wlan_nan_api_i.h"

/* Exclude AP removed link */
#define NLINK_EXCLUDE_REMOVED_LINK      0x01
/* Include AP removed link only, can't work with other flags */
#define NLINK_INCLUDE_REMOVED_LINK_ONLY 0x02
/* Exclude QUITE link */
#define NLINK_EXCLUDE_QUIET_LINK        0x04
/* Exclude standby link information */
#define NLINK_EXCLUDE_STANDBY_LINK      0x08
/* Dump link information */
#define NLINK_DUMP_LINK                 0x10

/* Compute disallowed mode bitmap 2 links at a time */
#define MAX_DISALLOW_MODE_LINK_NUM     2

/* This defines current supported max legacy concurrent connections for
 * disallowed mode bitmap computation
 */
#define MAX_LEGACY_CONCURRENCT_MODES   2

static
void ml_nlink_get_link_info(struct wlan_objmgr_psoc *psoc,
			    struct wlan_objmgr_vdev *vdev,
			    uint8_t flag,
			    uint8_t ml_num_link_sz,
			    struct ml_link_info *ml_link_info,
			    qdf_freq_t *ml_freq_lst,
			    uint8_t *ml_vdev_lst,
			    uint8_t *ml_linkid_lst,
			    uint8_t *ml_num_link,
			    uint32_t *ml_link_bitmap);

static uint32_t
ml_nlink_set_emlsr_mode_disable_req(struct wlan_objmgr_psoc *psoc,
				    struct wlan_objmgr_vdev *vdev,
				    enum ml_emlsr_disable_request req_source);

enum home_channel_map_id {
	HC_NONE,
	HC_2G,
	HC_5GL,
	HC_5GH,
	HC_BAND_MAX,
	HC_5GL_5GH = HC_BAND_MAX,
	HC_5GH_5GH,
	HC_5GL_5GL,
	HC_2G_5GL,
	HC_2G_5GH,
	HC_2G_2G,
	HC_LEGACY_MAX,
	HC_2G_5GL_5GH = HC_LEGACY_MAX,
	HC_2G_5GL_5GL,
	HC_2G_5GH_5GH,
	HC_5GL_5GL_5GH,
	HC_5GL_5GH_5GH,
	HC_5GL_5GL_5GL,
	HC_5GH_5GH_5GH,
	HC_MAX_MAP_ID,
};

static inline const char *hc_id_to_string(uint32_t idx)
{
	switch (idx) {
	CASE_RETURN_STRING(HC_NONE);
	CASE_RETURN_STRING(HC_2G);
	CASE_RETURN_STRING(HC_5GL);
	CASE_RETURN_STRING(HC_5GH);
	CASE_RETURN_STRING(HC_5GL_5GH);
	CASE_RETURN_STRING(HC_5GH_5GH);
	CASE_RETURN_STRING(HC_5GL_5GL);
	CASE_RETURN_STRING(HC_2G_5GL);
	CASE_RETURN_STRING(HC_2G_5GH);
	CASE_RETURN_STRING(HC_2G_2G);
	CASE_RETURN_STRING(HC_2G_5GL_5GH);
	CASE_RETURN_STRING(HC_2G_5GL_5GL);
	CASE_RETURN_STRING(HC_2G_5GH_5GH);
	CASE_RETURN_STRING(HC_5GL_5GL_5GH);
	CASE_RETURN_STRING(HC_5GL_5GH_5GH);
	CASE_RETURN_STRING(HC_5GL_5GL_5GL);
	CASE_RETURN_STRING(HC_5GH_5GH_5GH);
	default:
		return "Unknown";
	}
};

#define MAX_DISALLOW_MODE (MAX_DISALLOW_BMAP_COMB + 1)

enum disallow_mlo_mode {
	NO_RESTRICTION = 0,
	EMLSR_5GL_5GH = 1 << 0,
	EMLSR_5GH_5GH = 1 << 1,
	EMLSR_5GL_5GL = 1 << 2,
	MLMR_5GL_5GH  = 1 << 3,
	MLMR_5GH_5GH  = 1 << 4,
	MLMR_5GL_5GL  = 1 << 5,
	MLMR_2G_5GL   = 1 << 6,
	MLMR_2G_5GH   = 1 << 7,
};

static inline const char *disallow_to_string(uint32_t disallow_id)
{
	switch (disallow_id) {
	CASE_RETURN_STRING(NO_RESTRICTION);
	CASE_RETURN_STRING(EMLSR_5GL_5GH);
	CASE_RETURN_STRING(EMLSR_5GH_5GH);
	CASE_RETURN_STRING(EMLSR_5GL_5GL);
	CASE_RETURN_STRING(MLMR_5GL_5GH);
	CASE_RETURN_STRING(MLMR_5GH_5GH);
	CASE_RETURN_STRING(MLMR_5GL_5GL);
	CASE_RETURN_STRING(MLMR_2G_5GL);
	CASE_RETURN_STRING(MLMR_2G_5GH);
	default:
		return "Unknown";
	}
};

/**
 * struct disallow_mlo_modes - ML disallow modes
 * @allow_mcc: disallow modes for concurrency interface which allow MCC,
 * such as p2p go/gc which has no miracast enabled.
 * @disallow_mcc: diallow modes for concurrency interface which don't allow
 * MCC such as SAP, STA or p2p go/gc which has miracast enabled.
 */
struct disallow_mlo_modes {
	uint32_t allow_mcc;
	uint32_t disallow_mcc;
};

typedef const struct disallow_mlo_modes
disallow_mlo_mode_table_type[HC_MAX_MAP_ID][HC_LEGACY_MAX];

static disallow_mlo_mode_table_type
disallow_mlo_mode_table_sbs_low_share = {
	/* MLO Links  |  Concurrency  | disallowed mode bitmap  */
	[HC_2G_5GL] = {[HC_NONE] =    {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, 0},
			[HC_5GH] =    {0, MLMR_2G_5GL},
			[HC_5GL_5GH] = {0, 0},
			[HC_5GH_5GH] = {MLMR_2G_5GL, MLMR_2G_5GL},
			[HC_5GL_5GL] = {0, 0},
			[HC_2G_5GL] = {0, 0},
			[HC_2G_5GH] = {0, 0},
			[HC_2G_2G] = {0, 0} },
	[HC_2G_5GH] = {[HC_NONE] =    {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, MLMR_2G_5GH},
			[HC_5GH] =    {0, 0},
			[HC_5GL_5GH] = {0, 0},
			[HC_5GH_5GH] = {0, 0},
			[HC_5GL_5GL] = {MLMR_2G_5GH, MLMR_2G_5GH},
			[HC_2G_5GL] = {0, 0},
			[HC_2G_5GH] = {0, 0},
			[HC_2G_2G] = {0, 0} },
	[HC_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, MLMR_5GL_5GH},
			[HC_5GL] =    {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_5GH] =    {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_5GL_5GH] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_5GH_5GH] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_5GL_5GL] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_2G_5GL] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_2G_5GH] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_2G_2G] = {MLMR_5GL_5GH, MLMR_5GL_5GH} },
	[HC_5GH_5GH] = {[HC_NONE] =   {MLMR_5GH_5GH, MLMR_5GH_5GH},
			[HC_2G]  =    {MLMR_5GH_5GH, MLMR_5GH_5GH},
			[HC_5GL] =    {MLMR_5GH_5GH, MLMR_5GH_5GH},
			[HC_5GH] =    {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_5GL_5GH] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_5GH_5GH] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_5GL_5GL] = {MLMR_5GH_5GH, MLMR_5GH_5GH},
			[HC_2G_5GL] = {MLMR_5GH_5GH, MLMR_5GH_5GH},
			[HC_2G_5GH] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_2G_2G] = {MLMR_5GH_5GH, MLMR_5GH_5GH} },
	[HC_5GL_5GL] = {[HC_NONE] =   {MLMR_5GL_5GL, MLMR_5GL_5GL},
			[HC_2G]  =    {MLMR_5GL_5GL, MLMR_5GL_5GL},
			[HC_5GL] =    {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_5GH] =    {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_5GL_5GH] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_5GH_5GH] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_5GL_5GL] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_2G_5GL] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_2G_5GH] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_2G_2G] = {MLMR_5GL_5GL, MLMR_5GL_5GL} },
};

static disallow_mlo_mode_table_type
disallow_mlo_mode_table_sbs_upper_share = {
	/* MLO Links  |  Concurrency | disallowed mode bitmap  */
	[HC_2G_5GL] = {[HC_NONE]  =   {0, 0},
			[HC_2G]   =   {0, 0},
			[HC_5GL]  =   {0, 0},
			[HC_5GH]  =   {0, MLMR_2G_5GL},
			[HC_5GL_5GH] = {0, 0},
			[HC_5GH_5GH] = {MLMR_2G_5GL, MLMR_2G_5GL},
			[HC_5GL_5GL] = {0, 0},
			[HC_2G_5GL] = {0, 0},
			[HC_2G_5GH] = {0, 0},
			[HC_2G_2G] = {0, 0} },
	[HC_2G_5GH] = {[HC_NONE]  =   {0, 0},
			[HC_2G]   =   {0, 0},
			[HC_5GL]  =   {0, MLMR_2G_5GH},
			[HC_5GH]  =   {0, 0},
			[HC_5GL_5GH] = {0, 0},
			[HC_5GH_5GH] = {0, 0},
			[HC_5GL_5GL] = {MLMR_2G_5GH, MLMR_2G_5GH},
			[HC_2G_5GL] = {0, 0},
			[HC_2G_5GH] = {0, 0},
			[HC_2G_2G] = {0, 0} },
	[HC_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]   =   {0, MLMR_5GL_5GH},
			[HC_5GL]  =   {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_5GH]  =   {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_5GL_5GH] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_5GH_5GH] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_5GL_5GL] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_2G_5GL] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_2G_5GH] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_2G_2G] = {MLMR_5GL_5GH, MLMR_5GL_5GH} },
	[HC_5GH_5GH] = {[HC_NONE] =   {MLMR_5GH_5GH, MLMR_5GH_5GH},
			[HC_2G]   =   {MLMR_5GH_5GH, MLMR_5GH_5GH},
			[HC_5GL]  =   {MLMR_5GH_5GH | EMLSR_5GH_5GH,
						MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_5GH]  =   {MLMR_5GH_5GH | EMLSR_5GH_5GH,
						MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_5GL_5GH] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_5GH_5GH] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_5GL_5GL] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_2G_5GL] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_2G_5GH] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_2G_2G] = {MLMR_5GH_5GH, MLMR_5GH_5GH} },
	[HC_5GL_5GL] = {[HC_NONE] =   {MLMR_5GL_5GL, MLMR_5GL_5GL},
			[HC_2G]   =   {MLMR_5GL_5GL, MLMR_5GL_5GL},
			[HC_5GL]  =   {MLMR_5GL_5GL | EMLSR_5GL_5GL,
						MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_5GH]  =   {MLMR_5GL_5GL, MLMR_5GL_5GL},
			[HC_5GL_5GH] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_5GH_5GH] = {MLMR_5GL_5GL, MLMR_5GL_5GL},
			[HC_5GL_5GL] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_2G_5GL] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_2G_5GH] = {MLMR_5GL_5GL, MLMR_5GL_5GL},
			[HC_2G_2G] = {MLMR_5GL_5GL, MLMR_5GL_5GL} },
};

static disallow_mlo_mode_table_type
disallow_mlo_mode_table_sbs_switchable = {
	/* MLO Links  |  Concurrency | disallowed mode bitmap  */
	[HC_2G_5GL] = {[HC_NONE]  =   {0, 0},
			[HC_2G]   =   {0, 0},
			[HC_5GL]  =   {0, 0},
			[HC_5GH]  =   {0, MLMR_2G_5GL},
			[HC_5GL_5GH] = {0, 0},
			[HC_5GH_5GH] = {MLMR_2G_5GL, MLMR_2G_5GL},
			[HC_5GL_5GL] = {0, 0},
			[HC_2G_5GL] = {0, 0},
			[HC_2G_5GH] = {0, 0},
			[HC_2G_2G] = {0, 0} },
	[HC_2G_5GH] = {[HC_NONE]  =   {0, 0},
			[HC_2G]   =   {0, 0},
			[HC_5GL]  =   {0, MLMR_2G_5GH},
			[HC_5GH]  =   {0, 0},
			[HC_5GL_5GH] = {0, 0},
			[HC_5GH_5GH] = {0, 0},
			[HC_5GL_5GL] = {MLMR_2G_5GH, MLMR_2G_5GH},
			[HC_2G_5GL] = {0, 0},
			[HC_2G_5GH] = {0, 0},
			[HC_2G_2G] = {0, 0} },
	[HC_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]   =   {0, MLMR_5GL_5GH},
			[HC_5GL]  =   {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_5GH]  =   {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_5GL_5GH] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_5GH_5GH] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_5GL_5GL] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_2G_5GL] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_2G_5GH] = {EMLSR_5GL_5GH, EMLSR_5GL_5GH},
			[HC_2G_2G] = {MLMR_5GL_5GH, MLMR_5GL_5GH} },
	[HC_5GH_5GH] = {[HC_NONE] =   {MLMR_5GH_5GH, MLMR_5GH_5GH},
			[HC_2G]   =   {MLMR_5GH_5GH, MLMR_5GH_5GH},
			[HC_5GL]  =   {MLMR_5GH_5GH, MLMR_5GH_5GH},
			[HC_5GH]  =   {MLMR_5GH_5GH | EMLSR_5GH_5GH,
						MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_5GL_5GH] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_5GH_5GH] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_5GL_5GL] = {MLMR_5GH_5GH, MLMR_5GH_5GH},
			[HC_2G_5GL] = {MLMR_5GH_5GH, MLMR_5GH_5GH},
			[HC_2G_5GH] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_2G_2G] = {MLMR_5GH_5GH, MLMR_5GH_5GH} },
	[HC_5GL_5GL] = {[HC_NONE] =   {MLMR_5GL_5GL, MLMR_5GL_5GL},
			[HC_2G]   =   {MLMR_5GL_5GL, MLMR_5GL_5GL},
			[HC_5GL]  =   {MLMR_5GL_5GL | EMLSR_5GL_5GL,
						MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_5GH]  =   {MLMR_5GL_5GL, MLMR_5GL_5GL},
			[HC_5GL_5GH] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_5GH_5GH] = {MLMR_5GL_5GL, MLMR_5GL_5GL},
			[HC_5GL_5GL] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_2G_5GL] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_2G_5GH] = {MLMR_5GL_5GL, MLMR_5GL_5GL},
			[HC_2G_2G] = {MLMR_5GL_5GL, MLMR_5GL_5GL} },
};

static disallow_mlo_mode_table_type
disallow_mlo_mode_table_dbs = {
	/* MLO Links  |  Concurrency | disallowed mode bitmap  */
	[HC_2G_5GL] = {[HC_NONE] =    {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, 0},
			[HC_5GH] =    {0, MLMR_2G_5GL},
			[HC_5GL_5GH] = {0, 0},
			[HC_5GH_5GH] = {MLMR_2G_5GL, MLMR_2G_5GL},
			[HC_5GL_5GL] = {0, 0},
			[HC_2G_5GL] = {0, 0},
			[HC_2G_5GH] = {0, 0},
			[HC_2G_2G] = {0, 0} },
	[HC_2G_5GH] = {[HC_NONE] =    {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, MLMR_2G_5GH},
			[HC_5GH] =    {0, 0},
			[HC_5GL_5GH] = {0, 0},
			[HC_5GH_5GH] = {0, 0},
			[HC_5GL_5GL] = {MLMR_2G_5GH, MLMR_2G_5GH},
			[HC_2G_5GL] = {0, 0},
			[HC_2G_5GH] = {0, 0},
			[HC_2G_2G] = {0, 0} },
	[HC_5GL_5GH] = {[HC_NONE] =   {MLMR_5GL_5GH, MLMR_5GL_5GH},
			[HC_2G]  =    {MLMR_5GL_5GH, MLMR_5GL_5GH},
			[HC_5GL] =    {MLMR_5GL_5GH | EMLSR_5GL_5GH,
						MLMR_5GL_5GH | EMLSR_5GL_5GH},
			[HC_5GH] =    {MLMR_5GL_5GH | EMLSR_5GL_5GH,
						MLMR_5GL_5GH | EMLSR_5GL_5GH},
			[HC_5GL_5GH] = {MLMR_5GL_5GH | EMLSR_5GL_5GH,
						MLMR_5GL_5GH | EMLSR_5GL_5GH},
			[HC_5GH_5GH] = {MLMR_5GL_5GH | EMLSR_5GL_5GH,
						MLMR_5GL_5GH | EMLSR_5GL_5GH},
			[HC_5GL_5GL] = {MLMR_5GL_5GH | EMLSR_5GL_5GH,
						MLMR_5GL_5GH | EMLSR_5GL_5GH},
			[HC_2G_5GL] = {MLMR_5GL_5GH | EMLSR_5GL_5GH,
						MLMR_5GL_5GH | EMLSR_5GL_5GH},
			[HC_2G_5GH] = {MLMR_5GL_5GH | EMLSR_5GL_5GH,
						MLMR_5GL_5GH | EMLSR_5GL_5GH},
			[HC_2G_2G] = {MLMR_5GL_5GH, MLMR_5GL_5GH} },
	[HC_5GH_5GH] = {[HC_NONE] =   {MLMR_5GH_5GH, MLMR_5GH_5GH},
			[HC_2G]  =    {MLMR_5GH_5GH, MLMR_5GH_5GH},
			[HC_5GL] =    {MLMR_5GH_5GH | EMLSR_5GH_5GH,
						MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_5GH] =    {MLMR_5GH_5GH | EMLSR_5GH_5GH,
						MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_5GL_5GH] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_5GH_5GH] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_5GL_5GL] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_2G_5GL] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_2G_5GH] = {MLMR_5GH_5GH | EMLSR_5GH_5GH,
					MLMR_5GH_5GH | EMLSR_5GH_5GH},
			[HC_2G_2G] = {MLMR_5GH_5GH, MLMR_5GH_5GH} },
	[HC_5GL_5GL] = {[HC_NONE] =   {MLMR_5GL_5GL, MLMR_5GL_5GL},
			[HC_2G]  =    {MLMR_5GL_5GL, MLMR_5GL_5GL},
			[HC_5GL] =    {MLMR_5GL_5GL | EMLSR_5GL_5GL,
						MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_5GH] =    {MLMR_5GL_5GL | EMLSR_5GL_5GL,
						MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_5GL_5GH] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_5GH_5GH] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_5GL_5GL] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_2G_5GL] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_2G_5GH] = {MLMR_5GL_5GL | EMLSR_5GL_5GL,
					MLMR_5GL_5GL | EMLSR_5GL_5GL},
			[HC_2G_2G] = {MLMR_5GL_5GL, MLMR_5GL_5GL} },
};

#define HC_MAP_DATA(_HC_2G_, _HC_5GL_, _HC_5GH_) \
	((_HC_2G_) & 0xFF | ((_HC_5GL_) & 0xFF) << 8 | \
				((_HC_5GH_) & 0xFF) << 16)

#define HC_MAP(_HC_, _HC_2G_, _HC_5GL_, _HC_5GH_) \
	[_HC_] = HC_MAP_DATA((_HC_2G_), (_HC_5GL_), (_HC_5GH_))

static const uint32_t home_channel_maps[HC_MAX_MAP_ID] = {
	HC_MAP(HC_NONE, 0, 0, 0),
	HC_MAP(HC_2G,   1, 0, 0),
	HC_MAP(HC_5GL,  0, 1, 0),
	HC_MAP(HC_5GH,  0, 0, 1),
	HC_MAP(HC_5GL_5GH,  0, 1, 1),
	HC_MAP(HC_5GH_5GH,  0, 0, 2),
	HC_MAP(HC_5GL_5GL,  0, 2, 0),
	HC_MAP(HC_2G_5GL,  1, 1, 0),
	HC_MAP(HC_2G_5GH,  1, 0, 1),
	HC_MAP(HC_2G_2G,  2, 0, 0),
	HC_MAP(HC_2G_5GL_5GH,  1, 1, 1),
	HC_MAP(HC_2G_5GL_5GL,  1, 2, 0),
	HC_MAP(HC_2G_5GH_5GH,  1, 0, 2),
	HC_MAP(HC_5GL_5GL_5GH,  0, 2, 1),
	HC_MAP(HC_5GL_5GH_5GH,  0, 1, 2),
	HC_MAP(HC_5GL_5GL_5GL,  0, 3, 0),
	HC_MAP(HC_5GH_5GH_5GH,  0, 0, 3),
};

static enum home_channel_map_id
get_hc_id(struct wlan_objmgr_psoc *psoc,
	  uint8_t num_freq, qdf_freq_t *freq_lst)
{
	uint8_t hc[HC_BAND_MAX];
	uint32_t hc_map;
	uint8_t i;
	qdf_freq_t sbs_cut_off_freq =
		policy_mgr_get_5g_low_high_cut_freq(psoc);

	qdf_mem_zero(hc, sizeof(hc));
	for (i = 0; i < num_freq; i++) {
		if (WLAN_REG_IS_24GHZ_CH_FREQ(freq_lst[i]))
			hc[HC_2G]++;
		else if (freq_lst[i] <= sbs_cut_off_freq)
			hc[HC_5GL]++;
		else
			hc[HC_5GH]++;
	}

	hc_map = HC_MAP_DATA(hc[HC_2G], hc[HC_5GL], hc[HC_5GH]);
	for (i = 0; i < HC_MAX_MAP_ID; i++)
		if (home_channel_maps[i] == hc_map)
			return i;

	return HC_MAX_MAP_ID;
}

static
uint32_t disallow_mode_2_link_bitmap(
			struct wlan_objmgr_psoc *psoc,
			uint8_t ml_num_link,
			qdf_freq_t ml_freq_lst[WLAN_MAX_ML_BSS_LINKS],
			uint8_t ml_linkid_lst[WLAN_MAX_ML_BSS_LINKS],
			enum disallow_mlo_mode mode,
			enum mlo_disallowed_mode *mlo_mode)
{
	uint32_t link_map[HC_BAND_MAX];
	uint8_t i;
	qdf_freq_t sbs_cut_off_freq =
		policy_mgr_get_5g_low_high_cut_freq(psoc);

	qdf_mem_zero(link_map, sizeof(link_map));

	for (i = 0; i < ml_num_link; i++) {
		if (WLAN_REG_IS_24GHZ_CH_FREQ(ml_freq_lst[i]))
			link_map[HC_2G] |= 1 << ml_linkid_lst[i];
		else if (ml_freq_lst[i] <= sbs_cut_off_freq)
			link_map[HC_5GL] |= 1 << ml_linkid_lst[i];
		else
			link_map[HC_5GH] |= 1 << ml_linkid_lst[i];
	}

	*mlo_mode = MLO_DISALLOWED_MODE_NO_MLMR;

	switch (mode) {
	case EMLSR_5GL_5GH:
		*mlo_mode = MLO_DISALLOWED_MODE_NO_EMLSR;
		fallthrough;
	case MLMR_5GL_5GH:
		return link_map[HC_5GL] | link_map[HC_5GH];

	case EMLSR_5GH_5GH:
		*mlo_mode = MLO_DISALLOWED_MODE_NO_EMLSR;
		fallthrough;
	case MLMR_5GH_5GH:
		return link_map[HC_5GH];

	case EMLSR_5GL_5GL:
		*mlo_mode = MLO_DISALLOWED_MODE_NO_EMLSR;
		fallthrough;
	case MLMR_5GL_5GL:
		return link_map[HC_5GL];

	case MLMR_2G_5GL:
		return link_map[HC_2G] | link_map[HC_5GL];

	case MLMR_2G_5GH:
		return link_map[HC_2G] | link_map[HC_5GH];

	default:
		*mlo_mode = MLO_DISALLOWED_MODE_NO_RESTRICTION;
		return link_map[HC_2G] | link_map[HC_5GL] | link_map[HC_5GH];
	}
}

static uint8_t
extract_disallow_mode(uint32_t disallow_mode_bitmap,
		      enum disallow_mlo_mode disallow_mode[MAX_DISALLOW_MODE])
{
	uint8_t i = 0;
	uint8_t n = 0;

	while (disallow_mode_bitmap) {
		if (disallow_mode_bitmap & 1) {
			if (n >= MAX_DISALLOW_MODE) {
				mlo_debug("unexpected disallow_mode_bitmap 0x%x",
					  disallow_mode_bitmap);
				return n;
			}
			disallow_mode[n++] = 1 << i;
		}
		i++;
		disallow_mode_bitmap >>= 1;
	}

	return n;
}

static disallow_mlo_mode_table_type *
get_disallow_mlo_mode_table(struct wlan_objmgr_psoc *psoc)
{
	enum pm_rd_type rd_type;
	disallow_mlo_mode_table_type *disallow_mlo_mode_table;

	rd_type = policy_mgr_get_rd_type(psoc);

	/* select DBS rd if NAN interface is present */
	if (wlan_nan_is_disc_active(psoc))
		rd_type = pm_rd_dbs;

	switch (rd_type) {
	case pm_rd_dbs:
		disallow_mlo_mode_table =
			&disallow_mlo_mode_table_dbs;
		break;
	case pm_rd_sbs_upper_share:
		disallow_mlo_mode_table =
			&disallow_mlo_mode_table_sbs_upper_share;
		break;
	case pm_rd_sbs_switchable:
		disallow_mlo_mode_table =
			&disallow_mlo_mode_table_sbs_switchable;
		break;
	case pm_rd_sbs_low_share:
	default:
		/* SBS lower share is the default RD for GNG */
		disallow_mlo_mode_table =
			&disallow_mlo_mode_table_sbs_low_share;
		break;
	}

	return disallow_mlo_mode_table;
}

static uint8_t
mlo_get_num_5gh_links(qdf_freq_t *ml_freq_lst, uint8_t num_link)
{
	uint8_t i, num_5g_links = 0;

	for (i = 0; i < num_link; i++)
		if (!WLAN_REG_IS_24GHZ_CH_FREQ(ml_freq_lst[i]))
			num_5g_links++;

	return num_5g_links;
}

static uint32_t
override_mlmr_disallow_mode(struct wlan_objmgr_psoc *psoc,
			    struct wlan_objmgr_vdev *vdev,
			    uint8_t ml_num_link,
			    qdf_freq_t ml_freq_lst[MAX_DISALLOW_MODE_LINK_NUM],
			    uint8_t ml_linkid_lst[MAX_DISALLOW_MODE_LINK_NUM],
			    uint32_t disallow_mode_bitmap,
			    uint32_t force_active_bitmap)
{
	uint8_t force_active_5g_num = 0;
	uint8_t i;

	for (i = 0; i < ml_num_link; i++) {
		if (WLAN_REG_IS_24GHZ_CH_FREQ(ml_freq_lst[i]))
			continue;
		if (force_active_bitmap & (1 << ml_linkid_lst[i]))
			force_active_5g_num++;
	}

	/* If force active the two 5G links is requested (by vendor
	 * command.e.g), and new disallow mode is EMLSR and MLMR, this
	 * is confliction. This happens when SAP CSA from 2G to 5G to
	 * disallow EMLSR while the vendor command has force enter EMLSR
	 * by disallow MLMR for ML STA 5GL+5GH. If this confliction
	 * happens, to release MLMR to continue CSA.
	 */
	if (((disallow_mode_bitmap & EMLSR_5GL_5GH) ||
	     (disallow_mode_bitmap & EMLSR_5GH_5GH) ||
	     (disallow_mode_bitmap & EMLSR_5GL_5GL)) &&
	    ((disallow_mode_bitmap & MLMR_5GL_5GH) ||
	     (disallow_mode_bitmap & MLMR_5GH_5GH) ||
	     (disallow_mode_bitmap & MLMR_5GL_5GL))) {
		if (force_active_5g_num > 1) {
			mlo_debug("disallow bitmap 0x%x conflict with force active bitmap 0x%x, allow MLMR",
				  disallow_mode_bitmap, force_active_bitmap);
			disallow_mode_bitmap &=
				~(MLMR_5GL_5GH | MLMR_5GH_5GH | MLMR_5GL_5GL);

			return disallow_mode_bitmap;
		}
	}

	/* If user vendor command force enter eMLSR, e.g. emlsr_mode_req is
	 * WLAN_EMLSR_MODE_ENTER, try to append MLMR to disallow
	 * bitmap
	 */
	if (vdev->mlo_dev_ctx->sta_ctx->emlsr_mode_req !=
	    WLAN_EMLSR_MODE_ENTER)
		return disallow_mode_bitmap;

	/* If EMLSR is disallowed by concurrency, no need to append MLMR
	 * disallow bitmap
	 */
	if ((disallow_mode_bitmap & EMLSR_5GL_5GH) ||
	    (disallow_mode_bitmap & EMLSR_5GH_5GH) ||
	    (disallow_mode_bitmap & EMLSR_5GL_5GL))
		return disallow_mode_bitmap;

	if ((disallow_mode_bitmap & MLMR_5GL_5GH) ||
	    (disallow_mode_bitmap & MLMR_5GH_5GH) ||
	    (disallow_mode_bitmap & MLMR_5GL_5GL))
		return disallow_mode_bitmap;

	if (mlo_get_num_5gh_links(ml_freq_lst, ml_num_link) < 2)
		return disallow_mode_bitmap;

	disallow_mode_bitmap |= MLMR_5GL_5GH;
	mlo_debug("disallow_mode_bitmap 0x%x user emlsr_mode_req 0x%x",
		  disallow_mode_bitmap,
		  vdev->mlo_dev_ctx->sta_ctx->emlsr_mode_req);

	return disallow_mode_bitmap;
}

static uint32_t
override_emlsr_disallow_mode(struct wlan_objmgr_psoc *psoc,
			     struct wlan_objmgr_vdev *vdev,
			     uint8_t ml_num_link,
			     qdf_freq_t ml_freq_lst[MAX_DISALLOW_MODE_LINK_NUM],
			     uint32_t disallow_mode_bitmap,
			     bool *conc_emlsr_allow)
{
	uint32_t emlsr_disable_req_flags;

	if ((disallow_mode_bitmap & EMLSR_5GL_5GH) ||
	    (disallow_mode_bitmap & EMLSR_5GH_5GH) ||
	    (disallow_mode_bitmap & EMLSR_5GL_5GL)) {
		*conc_emlsr_allow = false;
		return disallow_mode_bitmap;
	}

	if (mlo_get_num_5gh_links(ml_freq_lst, ml_num_link) < 2)
		return disallow_mode_bitmap;

	/* If eMLSR is disallowed by AP CSA or other part, force add
	 * eMLSR disallow bitmap always.
	 */
	emlsr_disable_req_flags =
	ml_nlink_get_emlsr_mode_disable_req(psoc, vdev);

	emlsr_disable_req_flags &= ML_EMLSR_DISABLE_MASK_ALL;
	if (emlsr_disable_req_flags & ~ML_EMLSR_DISALLOW_BY_CONCURENCY) {
		disallow_mode_bitmap |= EMLSR_5GL_5GH;
		mlo_debug("disallow_mode_bitmap 0x%x req 0x%x",
			  disallow_mode_bitmap,
			  emlsr_disable_req_flags);
	}

	return disallow_mode_bitmap;
}

static uint8_t
populate_disallow_modes(struct wlan_objmgr_psoc *psoc,
			struct wlan_objmgr_vdev *vdev,
			bool initial_connected,
			enum mlo_disallowed_mode
			mlo_disallow_mode[MAX_DISALLOW_MODE],
			uint32_t disallow_link_bitmap[MAX_DISALLOW_MODE],
			bool *conc_emlsr_allow,
			uint8_t ml_num_link,
			qdf_freq_t ml_freq_lst[MAX_DISALLOW_MODE_LINK_NUM],
			uint8_t ml_linkid_lst[MAX_DISALLOW_MODE_LINK_NUM],
			uint32_t force_active_bitmap)
{
	struct disallow_mlo_modes mlo_modes;
	uint8_t num_of_modes = 0;
	enum disallow_mlo_mode disallow_mode[MAX_DISALLOW_MODE];
	uint32_t disallow_mode_bitmap = 0;
	uint8_t i;
	enum home_channel_map_id ml_hc_id;
	enum home_channel_map_id legacy_hc_id;
	disallow_mlo_mode_table_type *disallow_tbl;
	uint8_t legacy_num;
	/* For NAN interface additional 5GHz connection is required  */
	qdf_freq_t legacy_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS + 1];
	uint8_t legacy_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	enum policy_mgr_con_mode mode_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t allow_mcc;
	struct policy_mgr_psoc_priv_obj *pm_ctx;

	pm_ctx = policy_mgr_get_context(psoc);
	if (!pm_ctx) {
		policy_mgr_err("context is NULL");
		return QDF_STATUS_E_INVAL;
	}

	if (initial_connected)
		goto no_legacy_intf;

	legacy_num = policy_mgr_get_legacy_conn_info(
					psoc, legacy_vdev_lst,
					legacy_freq_lst, mode_lst,
					QDF_ARRAY_SIZE(legacy_vdev_lst));

	/* Make sure NAN 5H freq is filled, so that it shall disallow MLMR and
	 * eMLSR if needed
	 */
	if (wlan_nan_is_disc_active(psoc)) {
		if (legacy_num < MAX_LEGACY_CONCURRENCT_MODES &&
		    wlan_nan_get_5ghz_social_ch_freq(pm_ctx->pdev)) {
			legacy_freq_lst[legacy_num] =
				wlan_nan_get_5ghz_social_ch_freq(pm_ctx->pdev);
			mode_lst[legacy_num++] = PM_NAN_DISC_MODE;
		}

		if (legacy_num < MAX_LEGACY_CONCURRENCT_MODES &&
		    wlan_nan_get_24ghz_social_ch_freq(pm_ctx->pdev)) {
			legacy_freq_lst[legacy_num] =
				wlan_nan_get_24ghz_social_ch_freq(pm_ctx->pdev);
			mode_lst[legacy_num++] = PM_NAN_DISC_MODE;
		}
	}
	if (!legacy_num)
		goto no_legacy_intf;

	allow_mcc = true;

	switch (mode_lst[0]) {
	case PM_P2P_CLIENT_MODE:
	case PM_P2P_GO_MODE:
		if (!policy_mgr_is_vdev_high_tput_or_low_latency(
					psoc, legacy_vdev_lst[0]))
			break;
		fallthrough;
	case PM_STA_MODE:
	case PM_SAP_MODE:
		allow_mcc = false;
		break;
	default:
		mlo_debug("mode type %d", mode_lst[0]);
		break;
	}

	ml_hc_id = get_hc_id(psoc, ml_num_link, ml_freq_lst);
	if (ml_hc_id >= HC_MAX_MAP_ID) {
		mlo_debug("invalid ml_hc_id, %d", ml_hc_id);
		goto end;
	}
	legacy_hc_id = get_hc_id(psoc, legacy_num, legacy_freq_lst);
	if (legacy_hc_id >= HC_LEGACY_MAX) {
		mlo_debug("invalid legacy_hc_id %d", legacy_hc_id);
		goto end;
	}

	disallow_tbl = get_disallow_mlo_mode_table(psoc);
	if (!disallow_tbl) {
		mlo_debug("invalid disallow_tbl rd %d",
			  policy_mgr_get_rd_type(psoc));
		goto end;
	}

	mlo_modes = (*disallow_tbl)[ml_hc_id][legacy_hc_id];
	if (allow_mcc)
		disallow_mode_bitmap = mlo_modes.allow_mcc;
	else
		disallow_mode_bitmap = mlo_modes.disallow_mcc;

	mlo_debug("ml %s legacy %s allow_mcc %d rd %d disallow_mode_bitmap 0x%x",
		  hc_id_to_string(ml_hc_id),
		  hc_id_to_string(legacy_hc_id),
		  allow_mcc,
		  policy_mgr_get_rd_type(psoc),
		  disallow_mode_bitmap);

no_legacy_intf:
	disallow_mode_bitmap =
	override_emlsr_disallow_mode(psoc, vdev, ml_num_link,
				     ml_freq_lst, disallow_mode_bitmap,
				     conc_emlsr_allow);
	disallow_mode_bitmap =
	override_mlmr_disallow_mode(psoc, vdev, ml_num_link,
				    ml_freq_lst, ml_linkid_lst,
				    disallow_mode_bitmap,
				    force_active_bitmap);

	num_of_modes = extract_disallow_mode(disallow_mode_bitmap,
					     disallow_mode);
	if (!num_of_modes) {
		num_of_modes = 1;
		disallow_mode[0] = NO_RESTRICTION;
	}

	for (i = 0; i < num_of_modes; i++) {
		disallow_link_bitmap[i] =
			disallow_mode_2_link_bitmap(
					psoc,
					ml_num_link,
					ml_freq_lst,
					ml_linkid_lst,
					disallow_mode[i],
					&mlo_disallow_mode[i]);
		mlo_debug("[%d] disallow_mode 0x%x %s link bitmap 0x%x",
			  i, disallow_mode[i],
			  disallow_to_string(disallow_mode[i]),
			  disallow_link_bitmap[i]);
	}

end:
	return num_of_modes;
}

static void
ml_nlink_update_disallow_modes(struct wlan_objmgr_psoc *psoc,
			       struct wlan_objmgr_vdev *vdev,
			       bool initial_connected,
			       bool *disallow_changed,
			       uint32_t force_active_bitmap)
{
	enum mlo_disallowed_mode mlo_disallow_mode[MAX_DISALLOW_MODE];
	enum mlo_disallowed_mode tmp_mlo_disallow_mode[MAX_DISALLOW_MODE];
	uint32_t disallow_link_bitmap[MAX_DISALLOW_MODE];
	uint32_t tmp_disallow_link_bitmap[MAX_DISALLOW_MODE];
	uint8_t num_of_modes = 0, tmp_num_of_modes, i, j, k;
	struct ml_link_disallow_mode_bitmap *ml_link_disallow;
	uint8_t link_ids[MAX_MLO_LINK_ID];
	uint8_t num_ids;
	uint32_t num_disallow_mode_comb = 0;
	struct ml_link_disallow_mode_bitmap
		disallow_mode_link_bmap[MAX_DISALLOW_BMAP_COMB];
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct wlan_link_force_context *link_force_ctx;
	bool disallow_mode_changed = false;
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0;
	qdf_freq_t ml_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t dual_freq_lst[MAX_DISALLOW_MODE_LINK_NUM];
	uint8_t ml_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t ml_linkid_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t dual_linkid_lst[MAX_DISALLOW_MODE_LINK_NUM];
	struct ml_link_info ml_link_info[MAX_NUMBER_OF_CONC_CONNECTIONS];
	bool conc_emlsr_allow = true;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	if (!policy_mgr_is_mlo_in_mode_emlsr(psoc, NULL, NULL))
		return;

	ml_nlink_get_link_info(psoc, vdev, NLINK_EXCLUDE_REMOVED_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);
	if (ml_num_link < 2)
		return;

	for (i = 0; i < ml_num_link; i++) {
		for (j = i + 1; j < ml_num_link; j++) {
			dual_freq_lst[0] = ml_freq_lst[i];
			dual_freq_lst[1] = ml_freq_lst[j];
			dual_linkid_lst[0] = ml_linkid_lst[i];
			dual_linkid_lst[1] = ml_linkid_lst[j];
			tmp_num_of_modes =
				populate_disallow_modes(
					psoc, vdev, initial_connected,
					tmp_mlo_disallow_mode,
					tmp_disallow_link_bitmap,
					&conc_emlsr_allow,
					MAX_DISALLOW_MODE_LINK_NUM,
					dual_freq_lst, dual_linkid_lst,
					force_active_bitmap);

			/* Combine the 3 disallowed mode bitmaps into one for 3-link */
			for (k = 0; k < tmp_num_of_modes; k++) {
				if (num_of_modes < MAX_DISALLOW_MODE) {
					disallow_link_bitmap[num_of_modes] =
						tmp_disallow_link_bitmap[k];
					mlo_disallow_mode[num_of_modes] =
						tmp_mlo_disallow_mode[k];
					num_of_modes++;
				} else {
					mlo_err("max disallowed modes exceeded limits");
					break;
				}
			}
		}
	}

	if (conc_emlsr_allow)
		ml_nlink_clr_emlsr_mode_disable_req(
				psoc, vdev,
				ML_EMLSR_DISALLOW_BY_CONCURENCY);
	else
		ml_nlink_set_emlsr_mode_disable_req(
				psoc, vdev,
				ML_EMLSR_DISALLOW_BY_CONCURENCY);

	/* Combine the MLO_DISALLOWED_MODE_NO_MLMR and
	 * MLO_DISALLOWED_MODE_NO_EMLSR to MLO_DISALLOWED_MODE_NO_MLMR_EMLSR
	 * if the link bitmap is same.
	 */
	qdf_mem_zero(&disallow_mode_link_bmap[0], sizeof(disallow_mode_link_bmap));
	for (i = 0; i < num_of_modes; i++) {
		if (!disallow_link_bitmap[i])
			continue;

		if (num_disallow_mode_comb >= MAX_DISALLOW_BMAP_COMB) {
			mlo_debug("unexpected num_disallow_mode_comb %d",
				  num_disallow_mode_comb);
			break;
		}

		for (j = i + 1; j < num_of_modes; j++) {
			if (disallow_link_bitmap[i] !=
			    disallow_link_bitmap[j])
				continue;
			if ((mlo_disallow_mode[i] ==
				MLO_DISALLOWED_MODE_NO_MLMR &&
			     mlo_disallow_mode[j] ==
				MLO_DISALLOWED_MODE_NO_EMLSR) ||
			    (mlo_disallow_mode[i] ==
				MLO_DISALLOWED_MODE_NO_EMLSR &&
			     mlo_disallow_mode[j] ==
				MLO_DISALLOWED_MODE_NO_MLMR)) {
				mlo_disallow_mode[i] =
					MLO_DISALLOWED_MODE_NO_MLMR_EMLSR;
				disallow_link_bitmap[j] = 0;
			}
		}

		ml_link_disallow =
		&disallow_mode_link_bmap[num_disallow_mode_comb];
		ml_link_disallow->disallowed_mode = mlo_disallow_mode[i];

		num_ids = convert_link_bitmap_to_link_ids(
						disallow_link_bitmap[i],
						QDF_ARRAY_SIZE(link_ids),
						link_ids);
		for (k = 0; k < 4; k++) {
			if (k >= num_ids)
				ml_link_disallow->ieee_link_id[k] = 0xff;
			else
				ml_link_disallow->ieee_link_id[k] =
								link_ids[k];
		}

		mlo_debug("[%d] mode %d ieee_link_id_comb 0x%0x",
			  num_disallow_mode_comb,
			  ml_link_disallow->disallowed_mode,
			  ml_link_disallow->ieee_link_id_comb);

		num_disallow_mode_comb++;
	}

	/* Update current disallow mode bitmap in dev ctx */
	mlo_dev_lock_acquire(mlo_dev_ctx);
	link_force_ctx = &mlo_dev_ctx->sta_ctx->link_force_ctx;
	if (num_disallow_mode_comb != link_force_ctx->num_disallow_mode_comb)
		disallow_mode_changed = true;
	if (qdf_mem_cmp(&disallow_mode_link_bmap[0],
			&link_force_ctx->disallow_mode_link_bmap[0],
			sizeof(disallow_mode_link_bmap)))
		disallow_mode_changed = true;
	link_force_ctx->num_disallow_mode_comb = num_disallow_mode_comb;
	qdf_mem_copy(&link_force_ctx->disallow_mode_link_bmap[0],
		     &disallow_mode_link_bmap[0],
		     sizeof(disallow_mode_link_bmap));
	mlo_dev_lock_release(mlo_dev_ctx);

	if (disallow_changed)
		*disallow_changed = disallow_mode_changed;
}

static void
ml_nlink_reset_disallow_modes(struct wlan_objmgr_psoc *psoc,
			      struct wlan_objmgr_vdev *vdev)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct wlan_link_force_context *link_force_ctx;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx)
		return;

	mlo_dev_lock_acquire(mlo_dev_ctx);
	link_force_ctx = &mlo_dev_ctx->sta_ctx->link_force_ctx;
	qdf_mem_zero(&link_force_ctx->disallow_mode_link_bmap[0],
		     sizeof(link_force_ctx->disallow_mode_link_bmap));
	link_force_ctx->num_disallow_mode_comb = 0;
	mlo_dev_lock_release(mlo_dev_ctx);
}

void
ml_nlink_populate_disallow_modes(struct wlan_objmgr_psoc *psoc,
				 struct wlan_objmgr_vdev *vdev,
				 struct mlo_link_set_active_req *req,
				 uint32_t link_control_flags)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct wlan_link_force_context *link_force_ctx;
	uint32_t force_active_bitmap;
	struct ml_link_force_state curr_force_state = {0};

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}
	ml_nlink_get_curr_force_state(psoc, vdev, &curr_force_state);
	force_active_bitmap = curr_force_state.force_active_bitmap;

	if (req->param.force_mode == MLO_LINK_FORCE_MODE_ACTIVE_INACTIVE ||
	    req->param.force_mode == MLO_LINK_FORCE_MODE_ACTIVE)
		force_active_bitmap = req->param.force_cmd.ieee_link_id_bitmap;

	if (!(link_control_flags & link_ctrl_f_dont_update_disallow_bitmap))
		ml_nlink_update_disallow_modes(psoc, vdev, false, NULL,
					       force_active_bitmap);

	mlo_dev_lock_acquire(mlo_dev_ctx);
	link_force_ctx = &mlo_dev_ctx->sta_ctx->link_force_ctx;
	req->param.num_disallow_mode_comb =
			link_force_ctx->num_disallow_mode_comb;
	qdf_mem_copy(&req->param.disallow_mode_link_bmap[0],
		     &link_force_ctx->disallow_mode_link_bmap[0],
		     sizeof(req->param.disallow_mode_link_bmap));
	mlo_dev_lock_release(mlo_dev_ctx);
}

static void
ml_nlink_init_disallow_modes(struct wlan_objmgr_psoc *psoc,
			     struct wlan_objmgr_vdev *vdev)
{
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0;
	uint8_t ml_vdev_lst[WLAN_MAX_ML_BSS_LINKS];
	qdf_freq_t ml_freq_lst[WLAN_MAX_ML_BSS_LINKS];
	uint8_t ml_linkid_lst[WLAN_MAX_ML_BSS_LINKS];
	struct ml_link_info ml_link_info[WLAN_MAX_ML_BSS_LINKS];

	if (!mlo_is_mld_sta(vdev))
		return;

	if (!policy_mgr_is_mlo_in_mode_emlsr(psoc, NULL, NULL) ||
	    !wlan_mlme_is_aux_emlsr_support(psoc))
		return;

	if (mlo_mgr_is_link_switch_in_progress(vdev))
		return;

	ml_nlink_get_link_info(psoc, vdev, NLINK_EXCLUDE_STANDBY_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);
	if (ml_num_link < 2)
		return;

	/* Fill an initial disallow mode bitmap to dev context after
	 * ML STA initial connect completion.
	 */
	if (policy_mgr_is_mlo_in_mode_emlsr(psoc, NULL, NULL) &&
	    wlan_mlme_is_aux_emlsr_support(psoc))
		ml_nlink_update_disallow_modes(psoc, vdev, true, NULL, 0);
}

static bool
ml_nlink_is_support_emlsr_with_freq(
		struct wlan_objmgr_psoc *psoc,
		qdf_freq_t ml_freq_lst[MAX_DISALLOW_MODE_LINK_NUM],
		qdf_freq_t legacy_intf_freq)
{
	enum home_channel_map_id ml_hc_id;
	enum home_channel_map_id legacy_hc_id;
	disallow_mlo_mode_table_type *disallow_tbl;
	struct disallow_mlo_modes mlo_modes;
	uint32_t disallow_mode_bitmap;

	ml_hc_id = get_hc_id(psoc, MAX_DISALLOW_MODE_LINK_NUM, ml_freq_lst);
	if (ml_hc_id >= HC_MAX_MAP_ID) {
		mlo_debug("invalid ml_hc_id, %d", ml_hc_id);
		return false;
	}

	if (ml_hc_id != HC_5GL_5GH &&
	    ml_hc_id != HC_5GH_5GH &&
	    ml_hc_id != HC_5GL_5GL) {
		mlo_debug("ml %s no emlsr support", hc_id_to_string(ml_hc_id));
		return false;
	}

	legacy_hc_id = get_hc_id(psoc, 1, &legacy_intf_freq);
	if (legacy_hc_id >= HC_LEGACY_MAX) {
		mlo_debug("invalid legacy_hc_id %d", legacy_hc_id);
		return false;
	}

	disallow_tbl = get_disallow_mlo_mode_table(psoc);
	if (!disallow_tbl) {
		mlo_debug("invalid disallow_tbl rd %d",
			  policy_mgr_get_rd_type(psoc));
		return false;
	}

	mlo_modes = (*disallow_tbl)[ml_hc_id][legacy_hc_id];
	disallow_mode_bitmap = mlo_modes.allow_mcc;
			;
	mlo_debug("ml %s legacy %s rd %d disallow_mode_bitmap 0x%x",
		  hc_id_to_string(ml_hc_id),
		  hc_id_to_string(legacy_hc_id),
		  policy_mgr_get_rd_type(psoc),
		  disallow_mode_bitmap);

	if (disallow_mode_bitmap & EMLSR_5GL_5GH ||
	    disallow_mode_bitmap & EMLSR_5GH_5GH ||
	    disallow_mode_bitmap & EMLSR_5GL_5GL)
		return false;

	return true;
}

void
ml_nlink_convert_linkid_bitmap_to_vdev_bitmap(
			struct wlan_objmgr_psoc *psoc,
			struct wlan_objmgr_vdev *vdev,
			uint32_t link_bitmap,
			uint32_t *associated_bitmap,
			uint32_t *vdev_id_bitmap_sz,
			uint32_t vdev_id_bitmap[MLO_VDEV_BITMAP_SZ],
			uint8_t *vdev_id_num,
			uint8_t vdev_ids[WLAN_MLO_MAX_VDEVS])
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct wlan_mlo_sta *sta_ctx;
	uint8_t i, j, bitmap_sz = 0, num_vdev = 0;
	uint16_t link_id;
	uint8_t vdev_id;
	uint32_t associated_link_bitmap = 0;
	uint8_t vdev_per_bitmap = MLO_MAX_VDEV_COUNT_PER_BIMTAP_ELEMENT;

	*vdev_id_bitmap_sz = 0;
	*vdev_id_num = 0;
	qdf_mem_zero(vdev_id_bitmap,
		     sizeof(vdev_id_bitmap[0]) * MLO_VDEV_BITMAP_SZ);
	qdf_mem_zero(vdev_ids,
		     sizeof(vdev_ids[0]) * WLAN_MLO_MAX_VDEVS);
	if (associated_bitmap)
		*associated_bitmap = 0;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	sta_ctx = mlo_dev_ctx->sta_ctx;
	for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
		if (!mlo_dev_ctx->wlan_vdev_list[i])
			continue;
		vdev_id = wlan_vdev_get_id(mlo_dev_ctx->wlan_vdev_list[i]);
		if (!qdf_test_bit(i, sta_ctx->wlan_connected_links)) {
			mlo_debug("vdev %d is not connected", vdev_id);
			continue;
		}

		link_id = wlan_vdev_get_link_id(
					mlo_dev_ctx->wlan_vdev_list[i]);
		if (link_id >= MAX_MLO_LINK_ID) {
			mlo_err("invalid link id %d", link_id);
			continue;
		}
		associated_link_bitmap |= 1 << link_id;
		/* If the link_id is not interested one which is specified
		 * in "link_bitmap", continue the search.
		 */
		if (!(link_bitmap & (1 << link_id)))
			continue;
		j = vdev_id / vdev_per_bitmap;
		if (j >= MLO_VDEV_BITMAP_SZ)
			break;
		vdev_id_bitmap[j] |= 1 << (vdev_id % vdev_per_bitmap);
		if (j + 1 > bitmap_sz)
			bitmap_sz = j + 1;

		if (num_vdev >= WLAN_MLO_MAX_VDEVS)
			break;
		vdev_ids[num_vdev++] = vdev_id;
	}
	mlo_dev_lock_release(mlo_dev_ctx);

	*vdev_id_bitmap_sz = bitmap_sz;
	*vdev_id_num = num_vdev;
	if (associated_bitmap)
		*associated_bitmap = associated_link_bitmap;

	mlo_debug("vdev %d link bitmap 0x%x vdev_bitmap 0x%x sz %d num %d assoc 0x%x for bitmap 0x%x",
		  wlan_vdev_get_id(vdev), link_bitmap & associated_link_bitmap,
		  vdev_id_bitmap[0], *vdev_id_bitmap_sz, num_vdev,
		  associated_link_bitmap, link_bitmap);
}

void
ml_nlink_convert_vdev_bitmap_to_linkid_bitmap(
				struct wlan_objmgr_psoc *psoc,
				struct wlan_objmgr_vdev *vdev,
				uint32_t vdev_id_bitmap_sz,
				uint32_t *vdev_id_bitmap,
				uint32_t *link_bitmap,
				uint32_t *associated_bitmap)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct wlan_mlo_sta *sta_ctx;
	uint8_t i, j;
	uint16_t link_id;
	uint8_t vdev_id;
	uint32_t associated_link_bitmap = 0;
	uint8_t vdev_per_bitmap = MLO_MAX_VDEV_COUNT_PER_BIMTAP_ELEMENT;

	*link_bitmap = 0;
	if (associated_bitmap)
		*associated_bitmap = 0;
	if (!vdev_id_bitmap_sz) {
		mlo_debug("vdev_id_bitmap_sz 0");
		return;
	}

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	sta_ctx = mlo_dev_ctx->sta_ctx;
	mlo_dev_lock_acquire(mlo_dev_ctx);
	for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
		if (!mlo_dev_ctx->wlan_vdev_list[i])
			continue;
		vdev_id = wlan_vdev_get_id(mlo_dev_ctx->wlan_vdev_list[i]);
		if (!qdf_test_bit(i, sta_ctx->wlan_connected_links)) {
			mlo_debug("vdev %d is not connected", vdev_id);
			continue;
		}

		link_id = wlan_vdev_get_link_id(
					mlo_dev_ctx->wlan_vdev_list[i]);
		if (link_id >= MAX_MLO_LINK_ID) {
			mlo_err("invalid link id %d", link_id);
			continue;
		}
		associated_link_bitmap |= 1 << link_id;
		j = vdev_id / vdev_per_bitmap;
		if (j >= vdev_id_bitmap_sz) {
			mlo_err("invalid vdev id %d", vdev_id);
			continue;
		}
		/* If the vdev_id is not interested one which is specified
		 * in "vdev_id_bitmap", continue the search.
		 */
		if (!(vdev_id_bitmap[j] & (1 << (vdev_id % vdev_per_bitmap))))
			continue;

		*link_bitmap |= 1 << link_id;
	}
	mlo_dev_lock_release(mlo_dev_ctx);

	if (associated_bitmap)
		*associated_bitmap = associated_link_bitmap;
	mlo_debug("vdev %d link bitmap 0x%x vdev_bitmap 0x%x sz %d assoc 0x%x",
		  wlan_vdev_get_id(vdev), *link_bitmap, vdev_id_bitmap[0],
		  vdev_id_bitmap_sz, associated_link_bitmap);
}

static uint32_t
ml_nlink_set_emlsr_mode_disable_req(struct wlan_objmgr_psoc *psoc,
				    struct wlan_objmgr_vdev *vdev,
				    enum ml_emlsr_disable_request req_source)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	uint32_t old_flags = 0;
	struct wlan_link_force_context *link_force_ctx;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return 0;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	link_force_ctx = &mlo_dev_ctx->sta_ctx->link_force_ctx;
	old_flags = link_force_ctx->emlsr_disable_req_flags;
	link_force_ctx->emlsr_disable_req_flags |= req_source;
	mlo_debug("set disable_req to 0x%x from 0x%x by req 0x%x",
		  link_force_ctx->emlsr_disable_req_flags,
		  old_flags, req_source);
	mlo_dev_lock_release(mlo_dev_ctx);

	return old_flags;
}

uint32_t
ml_nlink_clr_emlsr_mode_disable_req(struct wlan_objmgr_psoc *psoc,
				    struct wlan_objmgr_vdev *vdev,
				    enum ml_emlsr_disable_request req_source)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	uint32_t old_flags = 0;
	struct wlan_link_force_context *link_force_ctx;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx)
		return 0;

	mlo_dev_lock_acquire(mlo_dev_ctx);
	link_force_ctx = &mlo_dev_ctx->sta_ctx->link_force_ctx;
	old_flags = link_force_ctx->emlsr_disable_req_flags;
	link_force_ctx->emlsr_disable_req_flags &= ~req_source;
	mlo_debug("clr disable_req to 0x%x from 0x%x by req 0x%x",
		  link_force_ctx->emlsr_disable_req_flags,
		  old_flags, req_source);
	mlo_dev_lock_release(mlo_dev_ctx);

	return old_flags;
}

uint32_t
ml_nlink_get_emlsr_mode_disable_req(struct wlan_objmgr_psoc *psoc,
				    struct wlan_objmgr_vdev *vdev)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	uint32_t req_flags = 0;
	struct wlan_link_force_context *link_force_ctx;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx)
		return 0;

	mlo_dev_lock_acquire(mlo_dev_ctx);
	link_force_ctx = &mlo_dev_ctx->sta_ctx->link_force_ctx;
	req_flags = link_force_ctx->emlsr_disable_req_flags;
	mlo_debug("disable_req 0x%x", req_flags);
	mlo_dev_lock_release(mlo_dev_ctx);

	return req_flags;
}

void
ml_nlink_get_curr_force_state(struct wlan_objmgr_psoc *psoc,
			      struct wlan_objmgr_vdev *vdev,
			      struct ml_link_force_state *force_cmd)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	qdf_mem_copy(force_cmd,
		     &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state,
		     sizeof(*force_cmd));
	mlo_dev_lock_release(mlo_dev_ctx);
}

void
ml_nlink_clr_force_state(struct wlan_objmgr_psoc *psoc,
			 struct wlan_objmgr_vdev *vdev)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct ml_link_force_state *force_state;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx)
		return;

	mlo_dev_lock_acquire(mlo_dev_ctx);
	force_state = &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state;
	qdf_mem_zero(force_state, sizeof(*force_state));
	ml_nlink_dump_force_state(force_state, "");
	qdf_mem_zero(mlo_dev_ctx->sta_ctx->link_force_ctx.reqs,
		     sizeof(mlo_dev_ctx->sta_ctx->link_force_ctx.reqs));
	mlo_dev_lock_release(mlo_dev_ctx);
}

void
ml_nlink_clr_requested_emlsr_mode(struct wlan_objmgr_psoc *psoc,
				  struct wlan_objmgr_vdev *vdev)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx)
		return;

	mlo_dev_lock_acquire(mlo_dev_ctx);
	mlo_sta_reset_requested_emlsr_mode(mlo_dev_ctx);
	mlo_dev_lock_release(mlo_dev_ctx);
}

static void
ml_nlink_update_link_bitmap(uint16_t *curr_link_bitmap,
			    uint16_t link_bitmap,
			    enum set_curr_control ctrl)
{
	switch (ctrl) {
	case LINK_OVERWRITE:
		*curr_link_bitmap = link_bitmap;
		break;
	case LINK_CLR:
		*curr_link_bitmap &= ~link_bitmap;
		break;
	case LINK_ADD:
		*curr_link_bitmap |= link_bitmap;
		break;
	default:
		mlo_err("unknown update ctrl %d", ctrl);
		return;
	}
}

void
ml_nlink_set_curr_force_active_state(struct wlan_objmgr_psoc *psoc,
				     struct wlan_objmgr_vdev *vdev,
				     uint16_t link_bitmap,
				     enum set_curr_control ctrl)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct ml_link_force_state *force_state;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	force_state = &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state;
	ml_nlink_update_link_bitmap(&force_state->force_active_bitmap,
				    link_bitmap, ctrl);
	ml_nlink_dump_force_state(force_state, ":ctrl %d bitmap 0x%x",
				  ctrl, link_bitmap);
	mlo_dev_lock_release(mlo_dev_ctx);
}

void
ml_nlink_set_curr_force_inactive_state(struct wlan_objmgr_psoc *psoc,
				       struct wlan_objmgr_vdev *vdev,
				       uint16_t link_bitmap,
				       enum set_curr_control ctrl)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct ml_link_force_state *force_state;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	force_state = &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state;
	ml_nlink_update_link_bitmap(&force_state->force_inactive_bitmap,
				    link_bitmap, ctrl);
	ml_nlink_dump_force_state(force_state, ":ctrl %d bitmap 0x%x", ctrl,
				  link_bitmap);
	mlo_dev_lock_release(mlo_dev_ctx);
}

void
ml_nlink_set_curr_force_active_num_state(struct wlan_objmgr_psoc *psoc,
					 struct wlan_objmgr_vdev *vdev,
					 uint8_t link_num,
					 uint16_t link_bitmap)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct ml_link_force_state *force_state;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	force_state = &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state;
	force_state->force_active_num = link_num;
	force_state->force_active_num_bitmap = link_bitmap;
	ml_nlink_dump_force_state(force_state, ":num %d bitmap 0x%x",
				  link_num, link_bitmap);
	mlo_dev_lock_release(mlo_dev_ctx);
}

void
ml_nlink_set_curr_force_inactive_num_state(struct wlan_objmgr_psoc *psoc,
					   struct wlan_objmgr_vdev *vdev,
					   uint8_t link_num,
					   uint16_t link_bitmap)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct ml_link_force_state *force_state;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	force_state = &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state;
	force_state->force_inactive_num = link_num;
	force_state->force_inactive_num_bitmap = link_bitmap;
	ml_nlink_dump_force_state(force_state, ":num %d bitmap 0x%x",
				  link_num, link_bitmap);
	mlo_dev_lock_release(mlo_dev_ctx);
}

void
ml_nlink_set_dynamic_inactive_links(struct wlan_objmgr_psoc *psoc,
				    struct wlan_objmgr_vdev *vdev,
				    uint16_t dynamic_link_bitmap)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct ml_link_force_state *force_state;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	force_state = &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state;
	force_state->curr_dynamic_inactive_bitmap = dynamic_link_bitmap;
	ml_nlink_dump_force_state(force_state, ":dynamic bitmap 0x%x",
				  dynamic_link_bitmap);
	mlo_dev_lock_release(mlo_dev_ctx);
}

void
ml_nlink_update_force_link_request(struct wlan_objmgr_psoc *psoc,
				   struct wlan_objmgr_vdev *vdev,
				   struct set_link_req *req,
				   enum set_link_source source)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct set_link_req *old;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}
	if (source >= SET_LINK_SOURCE_MAX || source < 0) {
		mlo_err("invalid source %d", source);
		return;
	}
	mlo_dev_lock_acquire(mlo_dev_ctx);
	old = &mlo_dev_ctx->sta_ctx->link_force_ctx.reqs[source];
	*old = *req;
	mlo_dev_lock_release(mlo_dev_ctx);
}

static void
ml_nlink_update_concurrency_link_request(
				struct wlan_objmgr_psoc *psoc,
				struct wlan_objmgr_vdev *vdev,
				struct ml_link_force_state *force_state,
				enum mlo_link_force_reason reason)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct set_link_req *req;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}
	mlo_dev_lock_acquire(mlo_dev_ctx);
	req =
	&mlo_dev_ctx->sta_ctx->link_force_ctx.reqs[SET_LINK_FROM_CONCURRENCY];
	req->reason = reason;
	req->force_active_bitmap = force_state->force_active_bitmap;
	req->force_inactive_bitmap = force_state->force_inactive_bitmap;
	req->force_active_num = force_state->force_active_num;
	req->force_inactive_num = force_state->force_inactive_num;
	req->force_inactive_num_bitmap =
		force_state->force_inactive_num_bitmap;
	mlo_dev_lock_release(mlo_dev_ctx);
}

void ml_nlink_init_concurrency_link_request(
	struct wlan_objmgr_psoc *psoc,
	struct wlan_objmgr_vdev *vdev)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct set_link_req *req;
	struct ml_link_force_state *force_state;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	force_state = &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state;
	req =
	&mlo_dev_ctx->sta_ctx->link_force_ctx.reqs[SET_LINK_FROM_CONCURRENCY];
	req->reason = MLO_LINK_FORCE_REASON_CONNECT;
	req->force_active_bitmap = force_state->force_active_bitmap;
	req->force_inactive_bitmap = force_state->force_inactive_bitmap;
	req->force_active_num = force_state->force_active_num;
	req->force_inactive_num = force_state->force_inactive_num;
	req->force_inactive_num_bitmap =
		force_state->force_inactive_num_bitmap;
	mlo_dev_lock_release(mlo_dev_ctx);
}

static void
ml_nlink_get_force_link_request(struct wlan_objmgr_psoc *psoc,
				struct wlan_objmgr_vdev *vdev,
				struct set_link_req *req,
				enum set_link_source source)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct set_link_req *old;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}
	if (source >= SET_LINK_SOURCE_MAX || source < 0) {
		mlo_err("invalid source %d", source);
		return;
	}
	mlo_dev_lock_acquire(mlo_dev_ctx);
	old = &mlo_dev_ctx->sta_ctx->link_force_ctx.reqs[source];
	*req = *old;
	mlo_dev_lock_release(mlo_dev_ctx);
}

static void
ml_nlink_clr_force_link_request(struct wlan_objmgr_psoc *psoc,
				struct wlan_objmgr_vdev *vdev,
				enum set_link_source source)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct set_link_req *req;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}
	if (source >= SET_LINK_SOURCE_MAX || source < 0) {
		mlo_err("invalid source %d", source);
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	req = &mlo_dev_ctx->sta_ctx->link_force_ctx.reqs[source];
	qdf_mem_zero(req, sizeof(*req));
	mlo_dev_lock_release(mlo_dev_ctx);
}

void
ml_nlink_get_dynamic_inactive_links(struct wlan_objmgr_psoc *psoc,
				    struct wlan_objmgr_vdev *vdev,
				    uint16_t *dynamic_link_bitmap,
				    uint16_t *force_link_bitmap)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct ml_link_force_state *force_state;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	force_state = &mlo_dev_ctx->sta_ctx->link_force_ctx.force_state;
	*dynamic_link_bitmap = force_state->curr_dynamic_inactive_bitmap;
	*force_link_bitmap = force_state->force_inactive_bitmap;
	mlo_dev_lock_release(mlo_dev_ctx);
}

/**
 * ml_nlink_get_affect_ml_sta() - Get ML STA whose link can be
 * force inactive
 * @psoc: PSOC object information
 *
 * At present we only support one ML STA. so ml_nlink_get_affect_ml_sta
 * is invoked to get one ML STA vdev from policy mgr table.
 * In future if ML STA+ML STA supported, we may need to extend it
 * to find one ML STA which is required to force inactve/active.
 *
 * Return: vdev object
 */
static struct wlan_objmgr_vdev *
ml_nlink_get_affect_ml_sta(struct wlan_objmgr_psoc *psoc)
{
	uint8_t num_ml_sta = 0, num_disabled_ml_sta = 0;
	uint8_t ml_sta_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS] = {0};
	qdf_freq_t ml_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS] = {0};
	struct wlan_objmgr_vdev *vdev;

	policy_mgr_get_ml_sta_info_psoc(psoc, &num_ml_sta,
					&num_disabled_ml_sta,
					ml_sta_vdev_lst, ml_freq_lst, NULL,
					NULL, NULL);
	if (!num_ml_sta || num_ml_sta > MAX_NUMBER_OF_CONC_CONNECTIONS)
		return NULL;

	if (num_ml_sta > MAX_NUMBER_OF_CONC_CONNECTIONS) {
		mlo_debug("unexpected num_ml_sta %d", num_ml_sta);
		return NULL;
	}
	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
						psoc, ml_sta_vdev_lst[0],
						WLAN_MLO_MGR_ID);
	if (!vdev) {
		mlo_err("invalid vdev for id %d",  ml_sta_vdev_lst[0]);
		return NULL;
	}

	return vdev;
}

bool ml_is_nlink_service_supported(struct wlan_objmgr_psoc *psoc)
{
	struct wmi_unified *wmi_handle;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		mlo_err("Invalid WMI handle");
		return false;
	}
	return wmi_service_enabled(
			wmi_handle,
			wmi_service_n_link_mlo_support);
}

static void
ml_nlink_get_standby_link_info(struct wlan_objmgr_psoc *psoc,
			       struct wlan_objmgr_vdev *vdev,
			       uint8_t flag,
			       uint8_t ml_num_link_sz,
			       struct ml_link_info *ml_link_info,
			       qdf_freq_t *ml_freq_lst,
			       uint8_t *ml_vdev_lst,
			       uint8_t *ml_linkid_lst,
			       uint8_t *ml_num_link,
			       uint32_t *ml_link_bitmap)
{
	struct mlo_link_info *link_info;
	uint8_t link_info_iter;

	link_info = mlo_mgr_get_ap_link(vdev);
	if (!link_info)
		return;

	for (link_info_iter = 0; link_info_iter < WLAN_MAX_ML_BSS_LINKS;
	     link_info_iter++) {
		if (qdf_is_macaddr_zero(&link_info->ap_link_addr))
			break;

		if (link_info->vdev_id == WLAN_INVALID_VDEV_ID) {
			if (*ml_num_link >= ml_num_link_sz) {
				mlo_debug("link lst overflow");
				break;
			}
			if (!link_info->link_chan_info->ch_freq) {
				mlo_debug("link freq 0!");
				break;
			}
			if (*ml_link_bitmap & (1 << link_info->link_id)) {
				mlo_debug("unexpected standby linkid %d",
					  link_info->link_id);
				break;
			}
			if (link_info->link_id >= MAX_MLO_LINK_ID) {
				mlo_debug("invalid standby link id %d",
					  link_info->link_id);
				break;
			}

			if ((flag & NLINK_EXCLUDE_REMOVED_LINK) &&
			    qdf_atomic_test_bit(
					LS_F_AP_REMOVAL_BIT,
					&link_info->link_status_flags)) {
				mlo_debug("standby link %d is removed",
					  link_info->link_id);
				continue;
			}
			if ((flag & NLINK_INCLUDE_REMOVED_LINK_ONLY) &&
			    !qdf_atomic_test_bit(
					LS_F_AP_REMOVAL_BIT,
					&link_info->link_status_flags)) {
				continue;
			}

			ml_freq_lst[*ml_num_link] =
				link_info->link_chan_info->ch_freq;
			ml_vdev_lst[*ml_num_link] = WLAN_INVALID_VDEV_ID;
			ml_linkid_lst[*ml_num_link] = link_info->link_id;
			*ml_link_bitmap |= 1 << link_info->link_id;
			if (flag & NLINK_DUMP_LINK)
				mlo_debug("vdev %d link %d freq %d bitmap 0x%x flag 0x%x",
					  ml_vdev_lst[*ml_num_link],
					  ml_linkid_lst[*ml_num_link],
					  ml_freq_lst[*ml_num_link],
					  *ml_link_bitmap, flag);
			(*ml_num_link)++;
		}

		link_info++;
	}
}

uint32_t
ml_nlink_get_standby_link_bitmap(struct wlan_objmgr_psoc *psoc,
				 struct wlan_objmgr_vdev *vdev)
{
	uint8_t ml_num_link = 0;
	uint32_t standby_link_bitmap = 0;
	uint8_t ml_vdev_lst[WLAN_MAX_ML_BSS_LINKS];
	qdf_freq_t ml_freq_lst[WLAN_MAX_ML_BSS_LINKS];
	uint8_t ml_linkid_lst[WLAN_MAX_ML_BSS_LINKS];
	struct ml_link_info ml_link_info[WLAN_MAX_ML_BSS_LINKS];

	ml_nlink_get_standby_link_info(psoc, vdev, NLINK_DUMP_LINK,
				       QDF_ARRAY_SIZE(ml_linkid_lst),
				       ml_link_info, ml_freq_lst, ml_vdev_lst,
				       ml_linkid_lst, &ml_num_link,
				       &standby_link_bitmap);

	return standby_link_bitmap;
}

qdf_freq_t
ml_nlink_get_standby_link_freq(struct wlan_objmgr_psoc *psoc,
			       struct wlan_objmgr_vdev *vdev,
			       uint32_t standby_link_bmap)
{
	uint8_t ml_num_link = 0;
	uint32_t standby_link_bitmap = 0;
	uint8_t ml_vdev_lst[WLAN_MAX_ML_BSS_LINKS];
	qdf_freq_t ml_freq_lst[WLAN_MAX_ML_BSS_LINKS];
	uint8_t ml_linkid_lst[WLAN_MAX_ML_BSS_LINKS];
	struct ml_link_info ml_link_info[WLAN_MAX_ML_BSS_LINKS];
	qdf_freq_t freq = 0;
	uint8_t i;

	ml_nlink_get_standby_link_info(psoc, vdev, 0,
				       QDF_ARRAY_SIZE(ml_linkid_lst),
				       ml_link_info, ml_freq_lst, ml_vdev_lst,
				       ml_linkid_lst, &ml_num_link,
				       &standby_link_bitmap);
	for (i = 0; i < ml_num_link; i++) {
		if (standby_link_bmap & (1 << ml_linkid_lst[i])) {
			freq = ml_freq_lst[i];
			break;
		}
	}

	return freq;
}

/**
 * ml_nlink_get_link_info() - Get ML STA link info
 * @psoc: PSOC object information
 * @vdev: ml sta vdev object
 * @flag: flag NLINK_* to specify what links should be returned
 * @ml_num_link_sz: input array size of ml_link_info and
 * other parameters.
 * @ml_link_info: ml link info array
 * @ml_freq_lst: channel frequency list
 * @ml_vdev_lst: vdev id list
 * @ml_linkid_lst: link id list
 * @ml_num_link: num of links
 * @ml_link_bitmap: link bitmaps.
 *
 * Return: void
 */
static void ml_nlink_get_link_info(struct wlan_objmgr_psoc *psoc,
				   struct wlan_objmgr_vdev *vdev,
				   uint8_t flag,
				   uint8_t ml_num_link_sz,
				   struct ml_link_info *ml_link_info,
				   qdf_freq_t *ml_freq_lst,
				   uint8_t *ml_vdev_lst,
				   uint8_t *ml_linkid_lst,
				   uint8_t *ml_num_link,
				   uint32_t *ml_link_bitmap)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct wlan_mlo_sta *sta_ctx;
	uint8_t i, num_link = 0;
	uint32_t link_bitmap = 0;
	uint16_t link_id;
	uint8_t vdev_id;
	bool connected = false;

	*ml_num_link = 0;
	*ml_link_bitmap = 0;
	qdf_mem_zero(ml_link_info, sizeof(*ml_link_info) * ml_num_link_sz);
	qdf_mem_zero(ml_freq_lst, sizeof(*ml_freq_lst) * ml_num_link_sz);
	qdf_mem_zero(ml_linkid_lst, sizeof(*ml_linkid_lst) * ml_num_link_sz);
	qdf_mem_zero(ml_vdev_lst, sizeof(*ml_vdev_lst) * ml_num_link_sz);

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	sta_ctx = mlo_dev_ctx->sta_ctx;

	link_bitmap = 0;
	for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
		if (!mlo_dev_ctx->wlan_vdev_list[i])
			continue;
		if (!qdf_test_bit(i, sta_ctx->wlan_connected_links))
			continue;

		if (!wlan_cm_is_vdev_connected(
				mlo_dev_ctx->wlan_vdev_list[i])) {
			mlo_debug("Vdev id %d is not in connected state",
				  wlan_vdev_get_id(
					mlo_dev_ctx->wlan_vdev_list[i]));
			continue;
		}
		connected = true;

		vdev_id = wlan_vdev_get_id(mlo_dev_ctx->wlan_vdev_list[i]);
		link_id = wlan_vdev_get_link_id(
					mlo_dev_ctx->wlan_vdev_list[i]);
		if (link_id >= MAX_MLO_LINK_ID) {
			mlo_debug("invalid link id %x for vdev %d",
				  link_id, vdev_id);
			continue;
		}

		if ((flag & NLINK_EXCLUDE_REMOVED_LINK) &&
		    wlan_get_vdev_link_removed_flag_by_vdev_id(
						psoc, vdev_id)) {
			mlo_debug("vdev id %d link %d is removed",
				  vdev_id, link_id);
			continue;
		}
		if ((flag & NLINK_INCLUDE_REMOVED_LINK_ONLY) &&
		    !wlan_get_vdev_link_removed_flag_by_vdev_id(
						psoc, vdev_id)) {
			continue;
		}
		if ((flag & NLINK_EXCLUDE_QUIET_LINK) &&
		    mlo_is_sta_in_quiet_status(mlo_dev_ctx, link_id)) {
			mlo_debug("vdev id %d link %d is quiet",
				  vdev_id, link_id);
			continue;
		}

		if (num_link >= ml_num_link_sz)
			break;
		ml_freq_lst[num_link] = wlan_get_operation_chan_freq(
					mlo_dev_ctx->wlan_vdev_list[i]);
		ml_vdev_lst[num_link] = vdev_id;
		ml_linkid_lst[num_link] = link_id;
		link_bitmap |= 1 << link_id;
		if (flag & NLINK_DUMP_LINK)
			mlo_debug("vdev %d link %d freq %d bitmap 0x%x flag 0x%x",
				  ml_vdev_lst[num_link],
				  ml_linkid_lst[num_link],
				  ml_freq_lst[num_link], link_bitmap, flag);
		num_link++;
	}
	/* Add standby link only if mlo sta is connected */
	if (connected && !(flag & NLINK_EXCLUDE_STANDBY_LINK))
		ml_nlink_get_standby_link_info(psoc, vdev, flag,
					       ml_num_link_sz,
					       ml_link_info,
					       ml_freq_lst,
					       ml_vdev_lst,
					       ml_linkid_lst,
					       &num_link,
					       &link_bitmap);

	mlo_dev_lock_release(mlo_dev_ctx);
	*ml_num_link = num_link;
	*ml_link_bitmap = link_bitmap;
}

static uint32_t
ml_nlink_get_available_link_bitmap(struct wlan_objmgr_psoc *psoc,
				   struct wlan_objmgr_vdev *vdev)
{
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0;
	uint8_t ml_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t ml_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t ml_linkid_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	struct ml_link_info ml_link_info[MAX_NUMBER_OF_CONC_CONNECTIONS];

	ml_nlink_get_link_info(psoc, vdev, NLINK_EXCLUDE_REMOVED_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);
	return ml_link_bitmap;
}

/**
 * convert_link_bitmap_to_link_ids() - Convert link bitmap to link ids
 * @link_bitmap: PSOC object information
 * @link_id_sz: link_ids array size
 * @link_ids: link id array
 *
 * Return: num of link id in link_ids array converted from link bitmap
 */
uint32_t
convert_link_bitmap_to_link_ids(uint32_t link_bitmap,
				uint8_t link_id_sz,
				uint8_t *link_ids)
{
	uint32_t i = 0;
	uint8_t id = 0;

	while (link_bitmap) {
		if (link_bitmap & 1) {
			if (id >= 15) {
				/* warning */
				mlo_err("linkid invalid %d 0x%x",
					id, link_bitmap);
				break;
			}
			if (link_ids) {
				if (i >= link_id_sz) {
					/* warning */
					mlo_err("linkid buff overflow 0x%x",
						link_bitmap);
					break;
				}
				link_ids[i] = id;
			}
			i++;
		}
		link_bitmap >>= 1;
		id++;
	}

	return i;
}

uint32_t
ml_nlink_convert_link_bitmap_to_ids(uint32_t link_bitmap,
				    uint8_t link_id_sz,
				    uint8_t *link_ids)
{
	return convert_link_bitmap_to_link_ids(link_bitmap, link_id_sz,
					       link_ids);
}

uint32_t
ml_nlink_convert_vdev_ids_to_link_bitmap(
	struct wlan_objmgr_psoc *psoc,
	uint8_t *mlo_vdev_lst,
	uint8_t num_ml_vdev)
{
	struct wlan_objmgr_vdev *tmp_vdev;
	uint8_t i, link_id;
	uint32_t link_bitmap = 0;

	for (i = 0; i < num_ml_vdev; i++) {
		tmp_vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
				psoc, mlo_vdev_lst[i],
				WLAN_POLICY_MGR_ID);
		if (!tmp_vdev) {
			policy_mgr_err("vdev not found for vdev_id %d ",
				       mlo_vdev_lst[i]);
			continue;
		}
		link_id = wlan_vdev_get_link_id(tmp_vdev);
		if (link_id == WLAN_INVALID_LINK_ID)
			policy_mgr_err("vdev %d has invalid link id %d",
				       mlo_vdev_lst[i], link_id);
		else
			link_bitmap |= 1 << link_id;
		wlan_objmgr_vdev_release_ref(tmp_vdev,
					     WLAN_POLICY_MGR_ID);
	}

	return link_bitmap;
}

/**
 * ml_nlink_sta_inactivity_allowed_with_quiet() - Check force inactive allowed
 * for links in bitmap
 * @psoc: PSOC object information
 * @vdev: vdev object
 * @force_inactive_bitmap: force inactive link bimap
 *
 * If left links (exclude removed link and QUITE link) are zero, the force
 * inactive bitmap is not allowed.
 *
 * Return: true if allow to force inactive links in force_inactive_bitmap
 */
static bool ml_nlink_sta_inactivity_allowed_with_quiet(
				struct wlan_objmgr_psoc *psoc,
				struct wlan_objmgr_vdev *vdev,
				uint16_t force_inactive_bitmap)
{
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0;
	uint8_t ml_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t ml_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t ml_linkid_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	struct ml_link_info ml_link_info[MAX_NUMBER_OF_CONC_CONNECTIONS];

	ml_nlink_get_link_info(psoc, vdev, (NLINK_EXCLUDE_REMOVED_LINK |
					    NLINK_EXCLUDE_QUIET_LINK),
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);
	ml_link_bitmap &= ~force_inactive_bitmap;
	if (!ml_link_bitmap) {
		mlo_debug("not allow - no active link after force inactive 0x%x",
			  force_inactive_bitmap);
		return false;
	}

	return true;
}

/**
 * ml_nlink_allow_conc() - Check force inactive allowed for links in bitmap
 * @psoc: PSOC object information
 * @vdev: vdev object
 * @no_forced_bitmap: no force link bitmap
 * @force_inactive_bitmap: force inactive link bimap
 *
 * Check the no force bitmap and force inactive bitmap are allowed to send
 * to firmware.
 *
 * Return: true if allow to "no force" and force inactive links.
 */
static bool
ml_nlink_allow_conc(struct wlan_objmgr_psoc *psoc,
		    struct wlan_objmgr_vdev *vdev,
		    uint16_t no_forced_bitmap,
		    uint16_t force_inactive_bitmap)
{
	if (!ml_nlink_sta_inactivity_allowed_with_quiet(
			psoc, vdev, force_inactive_bitmap))
		return false;

	return policy_mgr_allow_non_force_link_bitmap(
		psoc, vdev, no_forced_bitmap, force_inactive_bitmap);
}

/**
 * struct force_inactive_modes - force inactive links info
 * @force_inactive_hc_id: MCC links to be inactive by force bitmap
 * @force_inactive_num_hc_id: links to be inactive by force inactive num
 */
struct force_inactive_modes {
	enum home_channel_map_id force_inactive_hc_id;
	enum home_channel_map_id force_inactive_num_hc_id;
};

typedef const struct force_inactive_modes
force_inactive_table_type[HC_MAX_MAP_ID][HC_LEGACY_MAX];

static force_inactive_table_type
sap_force_inactive_table_lowshare_rd = {
	/* MLO Links  |  Concrrency  | force inactive mode bitmap */
	[HC_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, 0},
			[HC_5GH] =    {0, 0} },
	[HC_5GL_5GL] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GL, HC_5GL_5GL},
			[HC_5GH] =    {0, HC_5GL_5GL} },
	[HC_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, 0},
			[HC_5GH] =    {HC_5GH_5GH, HC_5GH_5GH} },
	[HC_2G_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, HC_2G_5GH},
			[HC_5GH] =    {0, HC_2G_5GL} },
	[HC_2G_5GL_5GL] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GL, HC_5GL_5GL},
			[HC_5GH] =    {0, HC_5GL_5GL} },
	[HC_2G_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, 0},
			[HC_5GH] =    {HC_5GH_5GH, HC_5GH_5GH} },
	[HC_5GL_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GL, HC_5GL_5GL},
			[HC_5GH] =    {0, 0} },
	[HC_5GL_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, 0},
			[HC_5GH] =    {HC_5GH_5GH, HC_5GH_5GH} },
	[HC_5GL_5GL_5GL] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GL_5GL, HC_5GL_5GL_5GL},
			[HC_5GH] =    {HC_5GL_5GL_5GL, HC_5GL_5GL_5GL} },
	[HC_5GH_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, 0},
			[HC_5GH] =    {HC_5GH_5GH_5GH, HC_5GH_5GH_5GH} },
};

static force_inactive_table_type
sap_force_inactive_table_dbs_rd = {
	/* MLO Links  |  Concrrency  | force inactive mode bitmap */
	[HC_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GH, HC_5GL_5GH},
			[HC_5GH] =    {HC_5GL_5GH, HC_5GL_5GH} },
	[HC_5GL_5GL] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GL, HC_5GL_5GL},
			[HC_5GH] =    {HC_5GL_5GL, HC_5GL_5GL} },
	[HC_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GH_5GH, HC_5GH_5GH},
			[HC_5GH] =    {HC_5GH_5GH, HC_5GH_5GH} },
	[HC_2G_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GH, HC_5GL_5GH},
			[HC_5GH] =    {HC_5GL_5GH, HC_5GL_5GH} },
	[HC_2G_5GL_5GL] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GL, HC_5GL_5GL},
			[HC_5GH] =    {0, HC_5GL_5GL} },
	[HC_2G_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, HC_5GH_5GH},
			[HC_5GH] =    {HC_5GH_5GH, HC_5GH_5GH} },
	[HC_5GL_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GL_5GH, HC_5GL_5GL_5GH},
			[HC_5GH] =    {HC_5GL_5GL_5GH, HC_5GL_5GL_5GH} },
	[HC_5GL_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GH_5GH, HC_5GL_5GH_5GH},
			[HC_5GH] =    {HC_5GL_5GH_5GH, HC_5GL_5GH_5GH} },
	[HC_5GL_5GL_5GL] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GL_5GL, HC_5GL_5GL_5GL},
			[HC_5GH] =    {HC_5GL_5GL_5GL, HC_5GL_5GL_5GL} },
	[HC_5GH_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GH_5GH_5GH, HC_5GH_5GH_5GH},
			[HC_5GH] =    {HC_5GH_5GH_5GH, HC_5GH_5GH_5GH} },
};

static force_inactive_table_type
sta_p2p_force_inactive_table_lowshare_rd = {
	/* MLO Links  |  Concurrency  | force inactive mode bitmap */
	[HC_2G_5GL] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {HC_2G, 0},
			[HC_5GL] =    {HC_5GL, 0},
			[HC_5GH] =    {0, HC_2G_5GL} },
	[HC_2G_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {HC_2G, 0},
			[HC_5GL] =    {0, HC_2G_5GH},
			[HC_5GH] =    {HC_5GH, 0} },
	[HC_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL, 0},
			[HC_5GH] =    {HC_5GH, 0} },
	[HC_5GL_5GL] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GL, HC_5GL_5GL},
			[HC_5GH] =    {0, HC_5GL_5GL} },
	[HC_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, 0},
			[HC_5GH] =    {HC_5GH_5GH, HC_5GH_5GH} },
	[HC_2G_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {HC_2G, 0},
			[HC_5GL] =    {HC_5GL, HC_2G_5GH},
			[HC_5GH] =    {HC_5GH, HC_2G_5GL} },
	[HC_2G_5GL_5GL] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {HC_2G, 0},
			[HC_5GL] =    {HC_5GL_5GL, 0},
			[HC_5GH] =    {0, HC_2G_5GL_5GL} },
	[HC_2G_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {HC_2G, 0},
			[HC_5GL] =    {HC_2G, 0},
			[HC_5GH] =    {HC_5GH_5GH, 0} },
	[HC_5GL_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GL, 0},
			[HC_5GH] =    {HC_5GH, HC_5GL_5GL} },
	[HC_5GL_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL, 0},
			[HC_5GH] =    {HC_5GH_5GH, 0} },
	[HC_5GL_5GL_5GL] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GL_5GL, HC_5GL_5GL_5GL},
			[HC_5GH] =    {0, HC_5GL_5GL_5GL} },
	[HC_5GH_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, 0},
			[HC_5GH] =    {HC_5GH_5GH_5GH, HC_5GH_5GH_5GH} },
};

static force_inactive_table_type
sta_p2p_force_inactive_table_dbs_rd = {
	/* MLO Links  |  Concrrency  | force inactive mode bitmap */
	[HC_2G_5GL] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {HC_2G, 0},
			[HC_5GL] =    {HC_5GL, 0},
			[HC_5GH] =    {HC_5GL, 0} },
	[HC_2G_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {HC_2G, 0},
			[HC_5GL] =    {HC_5GH, 0},
			[HC_5GH] =    {HC_5GH, 0} },
	[HC_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GH, HC_5GL_5GH},
			[HC_5GH] =    {HC_5GL_5GH, HC_5GL_5GH} },
	[HC_5GL_5GL] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GL, HC_5GL_5GL},
			[HC_5GH] =    {0, HC_5GL_5GL} },
	[HC_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, HC_5GH_5GH},
			[HC_5GH] =    {HC_5GH_5GH, HC_5GH_5GH} },
	[HC_2G_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {HC_2G, 0},
			[HC_5GL] =    {HC_5GL_5GH, 0},
			[HC_5GH] =    {HC_5GL_5GH, 0} },
	[HC_2G_5GL_5GL] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {HC_2G, 0},
			[HC_5GL] =    {HC_5GL_5GL, 0},
			[HC_5GH] =    {HC_5GL_5GL, 0} },
	[HC_2G_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {HC_2G, 0},
			[HC_5GL] =    {HC_5GH_5GH, 0},
			[HC_5GH] =    {HC_5GH_5GH, 0} },
	[HC_5GL_5GL_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GL_5GH, HC_5GL_5GL_5GH},
			[HC_5GH] =    {HC_5GL_5GL_5GH, HC_5GL_5GL_5GH} },
	[HC_5GL_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GH_5GH, HC_5GL_5GH_5GH},
			[HC_5GH] =    {HC_5GL_5GH_5GH, HC_5GL_5GH_5GH} },
	[HC_5GL_5GL_5GL] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {HC_5GL_5GL_5GL, HC_5GL_5GL_5GL},
			[HC_5GH] =    {0, HC_5GL_5GL_5GL} },
	[HC_5GH_5GH_5GH] = {[HC_NONE] =   {0, 0},
			[HC_2G]  =    {0, 0},
			[HC_5GL] =    {0, HC_5GH_5GH_5GH},
			[HC_5GH] =    {HC_5GH_5GH_5GH, HC_5GH_5GH_5GH} },
};

static force_inactive_table_type *sap_tbl[] = {
	NULL,
	&sap_force_inactive_table_dbs_rd,
	&sap_force_inactive_table_lowshare_rd,
	NULL, /* todo: high share*/
	NULL, /* todo: switchable*/
};

static force_inactive_table_type *sta_p2p_tbl[] = {
	NULL,
	&sta_p2p_force_inactive_table_dbs_rd,
	&sta_p2p_force_inactive_table_lowshare_rd,
	NULL, /* todo: high share*/
	NULL, /* todo: switchable*/
};

static force_inactive_table_type *
get_force_inactive_table(struct wlan_objmgr_psoc *psoc,
			 enum policy_mgr_con_mode pm_mode)
{
	enum pm_rd_type rd_type;
	force_inactive_table_type *force_inactive_table = NULL;

	rd_type = policy_mgr_get_rd_type(psoc);
	if (pm_mode == PM_SAP_MODE)
		force_inactive_table = sap_tbl[rd_type];
	else if (pm_mode == PM_STA_MODE ||
		 pm_mode == PM_P2P_CLIENT_MODE ||
		 pm_mode == PM_P2P_GO_MODE)
		force_inactive_table = sta_p2p_tbl[rd_type];

	return force_inactive_table;
}

static void hc_id_2_link_bitmap(
	struct wlan_objmgr_psoc *psoc,
	uint8_t ml_num_link,
	qdf_freq_t ml_freq_lst[WLAN_MAX_ML_BSS_LINKS],
	uint8_t ml_linkid_lst[WLAN_MAX_ML_BSS_LINKS],
	enum home_channel_map_id hc_id,
	uint32_t *link_bitmap)
{
	uint32_t link_map[HC_BAND_MAX];
	uint8_t i;
	qdf_freq_t sbs_cut_off_freq =
		policy_mgr_get_5g_low_high_cut_freq(psoc);

	qdf_mem_zero(link_map, sizeof(link_map));

	for (i = 0; i < ml_num_link; i++) {
		if (WLAN_REG_IS_24GHZ_CH_FREQ(ml_freq_lst[i]))
			link_map[HC_2G] |= 1 << ml_linkid_lst[i];
		else if (ml_freq_lst[i] <= sbs_cut_off_freq)
			link_map[HC_5GL] |= 1 << ml_linkid_lst[i];
		else
			link_map[HC_5GH] |= 1 << ml_linkid_lst[i];
	}

	switch (hc_id) {
	case HC_2G:
		*link_bitmap = link_map[HC_2G];
		break;
	case HC_5GL:
		*link_bitmap = link_map[HC_5GL];
		break;
	case HC_5GH:
		*link_bitmap = link_map[HC_5GH];
		break;
	case HC_5GL_5GH:
		*link_bitmap = link_map[HC_5GL] | link_map[HC_5GH];
		break;
	case HC_5GH_5GH:
		*link_bitmap = link_map[HC_5GH];
		break;
	case HC_5GL_5GL:
		*link_bitmap = link_map[HC_5GL];
		break;
	case HC_2G_5GL:
		*link_bitmap = link_map[HC_2G] | link_map[HC_5GL];
		break;
	case HC_2G_5GH:
		*link_bitmap = link_map[HC_2G] | link_map[HC_5GH];
		break;
	case HC_2G_5GL_5GH:
		*link_bitmap = link_map[HC_2G] | link_map[HC_5GL] | link_map[HC_5GH];
		break;
	case HC_2G_5GL_5GL:
		*link_bitmap = link_map[HC_2G] | link_map[HC_5GL];
		break;
	case HC_2G_5GH_5GH:
		*link_bitmap = link_map[HC_2G] | link_map[HC_5GH];
		break;
	case HC_5GL_5GL_5GH:
		*link_bitmap = link_map[HC_5GL] | link_map[HC_5GH];
		break;
	case HC_5GL_5GH_5GH:
		*link_bitmap = link_map[HC_5GL] | link_map[HC_5GH];
		break;
	case HC_5GL_5GL_5GL:
		*link_bitmap = link_map[HC_5GL];
		break;
	case HC_5GH_5GH_5GH:
		*link_bitmap = link_map[HC_5GH];
		break;
	default:
		return;
	}
}

static void
ml_nlink_handle_removal_links(struct wlan_objmgr_psoc *psoc,
			      struct wlan_objmgr_vdev *vdev,
			      struct ml_link_force_state *force_cmd)
{
	uint8_t ml_num_link = 0;
	uint32_t force_inactive_link_bitmap = 0;
	uint8_t ml_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t ml_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t ml_linkid_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	struct ml_link_info ml_link_info[MAX_NUMBER_OF_CONC_CONNECTIONS];

	ml_nlink_get_link_info(psoc, vdev, NLINK_INCLUDE_REMOVED_LINK_ONLY |
						NLINK_DUMP_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &force_inactive_link_bitmap);
	if (force_inactive_link_bitmap) {
		/* AP removed link will be force inactive always */
		force_cmd->force_inactive_bitmap = force_inactive_link_bitmap;
		mlo_debug("AP removed link 0x%x", force_inactive_link_bitmap);
	}
}

static void
ml_nlink_handle_comm_intf_emlsr(struct wlan_objmgr_psoc *psoc,
				struct wlan_objmgr_vdev *vdev,
				struct ml_link_force_state *force_cmd,
				uint8_t legacy_vdev_id,
				qdf_freq_t legacy_freq,
				enum policy_mgr_con_mode pm_mode)
{
	enum home_channel_map_id ml_link_hc_id;
	enum home_channel_map_id non_ml_hc_id;
	enum home_channel_map_id force_inactive_hc_id;
	enum home_channel_map_id force_inactive_num_hc_id;
	uint32_t force_inactive_bitmap = 0;
	uint32_t force_inactive_num_bitmap = 0;
	uint32_t scc_link_bitmap = 0;
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0;
	uint8_t ml_vdev_lst[WLAN_MAX_ML_BSS_LINKS];
	qdf_freq_t ml_freq_lst[WLAN_MAX_ML_BSS_LINKS];
	uint8_t ml_linkid_lst[WLAN_MAX_ML_BSS_LINKS];
	struct ml_link_info ml_link_info[WLAN_MAX_ML_BSS_LINKS];
	struct force_inactive_modes force_modes;
	uint8_t i;
	force_inactive_table_type *force_inactive_tlb;
	bool force_link_required = false;

	ml_nlink_get_link_info(psoc, vdev, NLINK_EXCLUDE_REMOVED_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);
	if (ml_num_link < 2)
		return;

	switch (pm_mode) {
	case PM_P2P_CLIENT_MODE:
	case PM_P2P_GO_MODE:
		if (!policy_mgr_is_vdev_high_tput_or_low_latency(
					psoc, legacy_vdev_id))
			break;
		fallthrough;
	case PM_STA_MODE:
	case PM_SAP_MODE:
		force_link_required = true;
		break;
	default:
		/* unexpected legacy connection mode */
		mlo_debug("unexpected legacy intf mode %d",
			  pm_mode);
		return;
	}
	if (!force_link_required &&
	    policy_mgr_is_ml_sta_links_in_mcc(psoc, ml_freq_lst,
					      ml_vdev_lst,
					      ml_linkid_lst,
					      ml_num_link,
					      NULL))
		force_link_required = true;
	if (!force_link_required)
		return;

	for (i = 0; i < ml_num_link; i++) {
		if (ml_freq_lst[i] == legacy_freq) {
			scc_link_bitmap = 1 << ml_linkid_lst[i];
			break;
		}
	}

	ml_link_hc_id = get_hc_id(psoc, ml_num_link, ml_freq_lst);
	if (ml_link_hc_id >= HC_MAX_MAP_ID) {
		mlo_debug("invalid ml_link_hc_id");
		return;
	}

	non_ml_hc_id = get_hc_id(psoc, 1, &legacy_freq);
	if (non_ml_hc_id >= HC_LEGACY_MAX) {
		mlo_debug("invalid non_ml_hc_id");
		return;
	}

	force_inactive_tlb = get_force_inactive_table(psoc, pm_mode);
	if (!force_inactive_tlb) {
		mlo_debug("unable to get force inactive tbl for %s rd %d",
			  device_mode_to_string(pm_mode),
			  policy_mgr_get_rd_type(psoc));
		return;
	}

	force_modes =
	(*force_inactive_tlb)[ml_link_hc_id][non_ml_hc_id];

	force_inactive_hc_id = force_modes.force_inactive_hc_id;
	force_inactive_num_hc_id = force_modes.force_inactive_num_hc_id;

	hc_id_2_link_bitmap(psoc, ml_num_link, ml_freq_lst, ml_linkid_lst,
			    force_inactive_hc_id, &force_inactive_bitmap);
	hc_id_2_link_bitmap(psoc, ml_num_link, ml_freq_lst, ml_linkid_lst,
			    force_inactive_num_hc_id,
			    &force_inactive_num_bitmap);

	mlo_debug("ml %s legacy %s %s inactive mcc %s inactive num %s rd %d",
		  hc_id_to_string(ml_link_hc_id),
		  hc_id_to_string(non_ml_hc_id),
		  device_mode_to_string(pm_mode),
		  hc_id_to_string(force_inactive_hc_id),
		  hc_id_to_string(force_inactive_num_hc_id),
		  policy_mgr_get_rd_type(psoc));
	mlo_debug("scc link 0x%x force_inactive_bitmap 0x%x force_inactive_num_bitmap 0x%x",
		  scc_link_bitmap, force_inactive_bitmap,
		  force_inactive_num_bitmap);

	if (force_inactive_bitmap) {
		if (scc_link_bitmap)
			force_inactive_bitmap =
				force_inactive_bitmap & ~scc_link_bitmap;
		/* Apply the force inactive bitmap only when there are
		 * available ml links after force inactive the bitmap.
		 */
		if (ml_link_bitmap & ~force_inactive_bitmap)
			force_cmd->force_inactive_bitmap =
				force_inactive_bitmap;
	}
	if (force_inactive_num_bitmap) {
		force_cmd->force_inactive_num =
			convert_link_bitmap_to_link_ids(
				force_inactive_num_bitmap, 0, NULL);

		if (force_cmd->force_inactive_num > 1) {
			force_cmd->force_inactive_num--;
			force_cmd->force_inactive_num_bitmap =
						force_inactive_num_bitmap;
		} else {
			force_cmd->force_inactive_num = 0;
		}
	}
}

/**
 * ml_nlink_handle_mcc_links() - Check force inactive needed
 * if ML STA links are in MCC channels
 * @psoc: PSOC object information
 * @vdev: vdev object
 * @force_cmd: force command to be returned
 *
 * This API will return force inactive number 1 in force_cmd
 * if STA links are in MCC channels with the link bitmap including
 * the MCC links id.
 * If the link is marked removed by AP MLD, return force inactive
 * bitmap with removed link id bitmap as well.
 *
 * Return: void
 */
static void
ml_nlink_handle_mcc_links(struct wlan_objmgr_psoc *psoc,
			  struct wlan_objmgr_vdev *vdev,
			  struct ml_link_force_state *force_cmd)
{
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0, affected_link_bitmap = 0;
	uint32_t force_inactive_link_bitmap = 0;
	uint8_t ml_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t ml_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t ml_linkid_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	struct ml_link_info ml_link_info[MAX_NUMBER_OF_CONC_CONNECTIONS];

	ml_nlink_get_link_info(psoc, vdev, NLINK_INCLUDE_REMOVED_LINK_ONLY |
						NLINK_DUMP_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &force_inactive_link_bitmap);
	if (force_inactive_link_bitmap) {
		/* AP removed link will be force inactive always */
		force_cmd->force_inactive_bitmap = force_inactive_link_bitmap;
		mlo_debug("AP removed link 0x%x", force_inactive_link_bitmap);
	}

	ml_nlink_get_link_info(psoc, vdev, NLINK_EXCLUDE_REMOVED_LINK |
						NLINK_DUMP_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);
	if (ml_num_link < 2)
		return;

	policy_mgr_is_ml_sta_links_in_mcc(psoc, ml_freq_lst,
					  ml_vdev_lst,
					  ml_linkid_lst,
					  ml_num_link,
					  &affected_link_bitmap);
	if (affected_link_bitmap) {
		force_cmd->force_inactive_num =
			convert_link_bitmap_to_link_ids(
				affected_link_bitmap, 0, NULL);
		if (force_cmd->force_inactive_num > 1) {
			force_cmd->force_inactive_num--;
			force_cmd->force_inactive_num_bitmap =
						affected_link_bitmap;
		} else {
			force_cmd->force_inactive_num = 0;
		}
	}
	if (force_inactive_link_bitmap || affected_link_bitmap)
		ml_nlink_dump_force_state(force_cmd, "");
}

/**
 * ml_nlink_handle_legacy_sta_intf() - Check force inactive needed
 * with legacy STA
 * @psoc: PSOC object information
 * @vdev: vdev object
 * @force_cmd: force command to be returned
 * @sta_vdev_id: legacy STA vdev id
 * @non_ml_sta_freq: legacy STA channel frequency
 *
 * If legacy STA is MCC with any link of MLO STA, the mlo link
 * will be forced inactive. And if 3 link MLO case, the left
 * 2 links have to be force inactive with num 1. For example,
 * ML STA 2+5+6, legacy STA on MCC channel of 5G link, then
 * 5G will be force inactive, and left 2+6 link will be force
 * inactive by inactive link num = 1 (with link bitmap 2+6).
 *
 * Return: void
 */
static void
ml_nlink_handle_legacy_sta_intf(struct wlan_objmgr_psoc *psoc,
				struct wlan_objmgr_vdev *vdev,
				struct ml_link_force_state *force_cmd,
				uint8_t sta_vdev_id,
				qdf_freq_t non_ml_sta_freq)
{
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0, affected_link_bitmap = 0;
	uint32_t force_inactive_link_bitmap = 0;
	uint8_t ml_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t ml_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t ml_linkid_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	struct ml_link_info ml_link_info[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t i = 0;
	uint32_t scc_link_bitmap = 0;

	ml_nlink_get_link_info(psoc, vdev, NLINK_EXCLUDE_REMOVED_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);
	if (ml_num_link < 2)
		return;

	for (i = 0; i < ml_num_link; i++) {
		if (ml_freq_lst[i] == non_ml_sta_freq) {
			scc_link_bitmap = 1 << ml_linkid_lst[i];
		} else if (policy_mgr_2_freq_always_on_same_mac(
				psoc, ml_freq_lst[i], non_ml_sta_freq)) {
			force_inactive_link_bitmap |= 1 << ml_linkid_lst[i];
		} else if (!wlan_cm_same_band_sta_allowed(psoc) &&
			   (wlan_reg_is_24ghz_ch_freq(ml_freq_lst[i]) ==
			    wlan_reg_is_24ghz_ch_freq(non_ml_sta_freq)) &&
			   !policy_mgr_are_sbs_chan(psoc, ml_freq_lst[i],
						     non_ml_sta_freq)) {
			force_inactive_link_bitmap |= 1 << ml_linkid_lst[i];
		}
	}

	/* If no left active link, don't send the force inactive command for
	 * concurrency purpose.
	 */
	if (!(ml_link_bitmap & ~force_inactive_link_bitmap)) {
		mlo_debug("unexpected ML conc with legacy STA freq %d",
			  non_ml_sta_freq);
		return;
	}

	if (force_inactive_link_bitmap) {
		/* for example SBS rd, ML 2G+5G high, Legacy intf on 5G high,
		 * set force inactive with bitmap of 5g link.
		 *
		 * for SBS rd, ML 2G + 5G low + 6G, Legacy intf on 2G.
		 * set force inactive with bitmap 2G link,
		 * and set force inactive link num to 1 for left 5g and 6g
		 * link.
		 * for SBS rd, ML 2G + 5G low + 6G, Legacy intf on 5G low.
		 * set force inactive with bitmap 5G low link,
		 * and set force inactive link num to 1 for left 2g and 6g
		 * link.
		 * for SBS rd, ML 2G + 5G low + 6G, Legacy intf on 5G high.
		 * set force inactive with bitmap 6G link,
		 * and set force inactive link num to 1 for left 2g and 5g
		 * link.
		 * In above 3 link cases, if legacy intf is SCC with ml link
		 * don't force inactive by bitmap, only send force inactive
		 * num with bitmap
		 */
		force_cmd->force_inactive_bitmap = force_inactive_link_bitmap;

		affected_link_bitmap =
			ml_link_bitmap & ~force_inactive_link_bitmap;
		affected_link_bitmap &= ~scc_link_bitmap;
		force_cmd->force_inactive_num =
			convert_link_bitmap_to_link_ids(
				affected_link_bitmap, 0, NULL);
		if (force_cmd->force_inactive_num > 1) {
			force_cmd->force_inactive_num--;
			force_cmd->force_inactive_num_bitmap =
						affected_link_bitmap;

		} else {
			force_cmd->force_inactive_num = 0;
		}
	} else {
		/* for example SBS rd, ML 2G+5G high, Legacy intf on 5G low,
		 * set force inactive num to 1 with bitmap of 2g+5g link.
		 *
		 * for SBS rd, ML 2G + 5G low + 6G, Legacy intf on 5G low SCC.
		 * set force inactive link num to 1 for left 2g and 6g
		 * link.
		 */
		affected_link_bitmap = ml_link_bitmap;
		affected_link_bitmap &= ~scc_link_bitmap;

		force_cmd->force_inactive_num =
			convert_link_bitmap_to_link_ids(
				affected_link_bitmap, 0, NULL);
		if (force_cmd->force_inactive_num > 1) {
			force_cmd->force_inactive_num--;
			force_cmd->force_inactive_num_bitmap =
				affected_link_bitmap;
		} else {
			force_cmd->force_inactive_num = 0;
		}
	}
}

/**
 * ml_nlink_handle_legacy_sap_intf() - Check force inactive needed
 * with legacy SAP
 * @psoc: PSOC object information
 * @vdev: vdev object
 * @force_cmd: force command to be returned
 * @sap_vdev_id: legacy SAP vdev id
 * @sap_freq: legacy SAP channel frequency
 *
 * If legacy SAP is 2g only SAP and MLO STA is 5+6,
 * 2 links have to be force inactive with num 1.
 *
 * Return: void
 */
static void
ml_nlink_handle_legacy_sap_intf(struct wlan_objmgr_psoc *psoc,
				struct wlan_objmgr_vdev *vdev,
				struct ml_link_force_state *force_cmd,
				uint8_t sap_vdev_id,
				qdf_freq_t sap_freq)
{
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0, affected_link_bitmap = 0;
	uint8_t ml_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t ml_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t ml_linkid_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	struct ml_link_info ml_link_info[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t i = 0;
	bool sap_2g_only = false, is_nan_present = false;
	uint32_t list_nan_disc[MAX_NUMBER_OF_CONC_CONNECTIONS] = {0};

	ml_nlink_get_link_info(psoc, vdev, NLINK_EXCLUDE_REMOVED_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);

	if (ml_num_link < 2)
		return;

	/* SAP MCC with MLO STA link is not preferred.
	 * If SAP is 2Ghz only by ACS and two ML link are
	 * 5/6 band, then force SCC may not happen. In such
	 * case inactive one link.
	 */
	if (policy_mgr_check_2ghz_only_sap_affected_link(
				psoc, sap_vdev_id, sap_freq,
				ml_num_link, ml_freq_lst)) {
		mlo_debug("2G only SAP vdev %d ch freq %d is not SCC with any MLO STA link",
			  sap_vdev_id, sap_freq);
		sap_2g_only = true;
	}

	is_nan_present = policy_mgr_mode_specific_connection_count(
					psoc, PM_NAN_DISC_MODE, list_nan_disc);

	/*
	 * Force inctive 2GHz link if SAP + ML-sta is causing MCC
	 */

	for (i = 0; i < ml_num_link; i++) {
		/*
		 * check if NAN is present and causing mcc with SAP
		 */
		if (wlan_reg_is_24ghz_ch_freq(ml_freq_lst[i]) &&
		    ml_freq_lst[i] != NAN_2GHZ_SOCIAL_CH_FREQ &&
		    is_nan_present)
			force_cmd->force_inactive_bitmap |= 1 << ml_linkid_lst[i];
	}

	/*
	 * If SAP is on 5G or 6G, SAP can always force SCC to 5G/6G ML STA or
	 * 2G ML STA, no need force SCC link.
	 */
	if (!sap_2g_only)
		return;

	for (i = 0; i < ml_num_link; i++) {
		if (!wlan_reg_is_24ghz_ch_freq(ml_freq_lst[i]))
			affected_link_bitmap |= 1 << ml_linkid_lst[i];
	}

	if (affected_link_bitmap) {
		/* for SBS rd, ML 2G + 5G low, Legacy SAP on 2G.
		 * no force any link
		 * for SBS rd, ML 5G low + 5G high/6G, Legacy SAP on 2G.
		 * set force inactive num 1 with bitmap 5g and 6g.
		 *
		 * for SBS rd, ML 2G + 5G low + 6G, Legacy SAP on 2G.
		 * set force inactive link num to 1 for 5g and 6g
		 * link.
		 */
		force_cmd->force_inactive_num =
			convert_link_bitmap_to_link_ids(
				affected_link_bitmap, 0, NULL);
		if (force_cmd->force_inactive_num > 1) {
			force_cmd->force_inactive_num--;
			force_cmd->force_inactive_num_bitmap =
					affected_link_bitmap;
		} else {
			force_cmd->force_inactive_num = 0;
		}
	}
}

/**
 * ml_nlink_handle_legacy_p2p_intf() - Check force inactive needed
 * with p2p
 * @psoc: PSOC object information
 * @vdev: vdev object
 * @force_cmd: force command to be returned
 * @p2p_vdev_id: p2p vdev id
 * @p2p_freq: p2p channel frequency
 *
 * If P2P has low latency flag and MCC with any link of MLO STA, the mlo link
 * will be forced inactive. And if 3 link MLO case, the left 2 links have to
 * be force inactive with num 1.
 *
 * Return: void
 */
static void
ml_nlink_handle_legacy_p2p_intf(struct wlan_objmgr_psoc *psoc,
				struct wlan_objmgr_vdev *vdev,
				struct ml_link_force_state *force_cmd,
				uint8_t p2p_vdev_id,
				qdf_freq_t p2p_freq)
{
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0, affected_link_bitmap = 0;
	uint32_t force_inactive_link_bitmap = 0;
	uint8_t ml_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t ml_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t ml_linkid_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	struct ml_link_info ml_link_info[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t i = 0;
	uint32_t scc_link_bitmap = 0;

	/* If high tput or low latency is not set, mcc is allowed for p2p */
	if (!policy_mgr_is_vdev_high_tput_or_low_latency(
				psoc, p2p_vdev_id))
		return;
	ml_nlink_get_link_info(psoc, vdev, NLINK_EXCLUDE_REMOVED_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);
	if (ml_num_link < 2)
		return;

	for (i = 0; i < ml_num_link; i++) {
		if (ml_freq_lst[i] == p2p_freq) {
			scc_link_bitmap = 1 << ml_linkid_lst[i];
		} else if (policy_mgr_2_freq_always_on_same_mac(
				psoc, ml_freq_lst[i], p2p_freq)) {
			force_inactive_link_bitmap |= 1 << ml_linkid_lst[i];
		}
	}
	/* If no left active link, don't send the force inactive command for
	 * concurrency purpose.
	 */
	if (!(ml_link_bitmap & ~force_inactive_link_bitmap)) {
		mlo_debug("unexpected ML conc with legacy P2P freq %d",
			  p2p_freq);
		return;
	}

	if (force_inactive_link_bitmap) {
		/* for example SBS rd, ML 2G+5G high, Legacy intf on 5G high,
		 * set force inactive with bitmap of 5g link.
		 *
		 * for SBS rd, ML 2G + 5G low + 6G, Legacy intf on 2G.
		 * set force inactive with bitmap 2G link,
		 * and set force inactive link num to 1 for left 5g and 6g
		 * link.
		 * for SBS rd, ML 2G + 5G low + 6G, Legacy intf on 5G low.
		 * set force inactive with bitmap 5G low link,
		 * and set force inactive link num to 1 for left 2g and 6g
		 * link.
		 * for SBS rd, ML 2G + 5G low + 6G, Legacy intf on 5G high..
		 * set force inactive with bitmap 6G low link,
		 * and set force inactive link num to 1 for left 2g and 5g
		 * link.
		 */
		force_cmd->force_inactive_bitmap = force_inactive_link_bitmap;

		affected_link_bitmap =
			ml_link_bitmap & ~force_inactive_link_bitmap;
		affected_link_bitmap &= ~scc_link_bitmap;
		force_cmd->force_inactive_num =
			convert_link_bitmap_to_link_ids(
				affected_link_bitmap, 0, NULL);
		if (force_cmd->force_inactive_num > 1) {
			force_cmd->force_inactive_num--;
			force_cmd->force_inactive_num_bitmap =
					affected_link_bitmap;

		} else {
			force_cmd->force_inactive_num = 0;
		}
	} else {
		/* for example SBS rd, ML 2G+5G high, Legacy intf on 5G low,
		 * set force inactive num to 1 with bitmap of 2g+5g link.
		 *
		 * for SBS rd, ML 2G + 5G low + 6G, Legacy intf on 5G low SCC.
		 * set force inactive link num to 1 for left 2g and 6g
		 * link.
		 */
		affected_link_bitmap = ml_link_bitmap;
		affected_link_bitmap &= ~scc_link_bitmap;

		force_cmd->force_inactive_num =
			convert_link_bitmap_to_link_ids(
				affected_link_bitmap, 0, NULL);
		if (force_cmd->force_inactive_num > 1) {
			force_cmd->force_inactive_num--;
			force_cmd->force_inactive_num_bitmap =
					affected_link_bitmap;
		} else {
			force_cmd->force_inactive_num = 0;
		}
	}
}

static void
ml_nlink_handle_legacy_nan_intf(struct wlan_objmgr_psoc *psoc,
				struct ml_link_force_state *force_cmd,
				struct wlan_objmgr_vdev *vdev)
{
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0;
	uint8_t ml_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t ml_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t ml_linkid_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	struct ml_link_info ml_link_info[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t i, j;

	if (!wlan_nan_is_disc_active(psoc))
		return;

	ml_nlink_get_link_info(psoc, vdev, NLINK_EXCLUDE_REMOVED_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);
	/* No need to send FORCE INACTIVE/INACTIVE_NUM in case of one link */
	if (ml_num_link < 2)
		return;
	/*
	 * Check if the 2 GHz ml link is leading to mcc with 2 GHz NAN
	 * discovery channel and consider it for INACTIVE
	 */
	for (i = 0; i < ml_num_link; i++) {
		if (!policy_mgr_get_sap_mode_count(psoc, NULL))
			break;

		if (wlan_reg_is_24ghz_ch_freq(ml_freq_lst[i]) &&
		    ml_freq_lst[i] != NAN_2GHZ_SOCIAL_CH_FREQ)
			force_cmd->force_inactive_bitmap |=
						1 << ml_linkid_lst[i];
	}

	/*
	 * Check if both the 5 GHz ML links are on same mac when NAN is present.
	 * This is possible when ML-STA is connected in 5L+5H and NAN
	 * is enabled. As NAN is not supported in SBS, hw_mode will be
	 * switched to DBS and both the ML links would be moved to 5GHz mac.
	 */
	for (i = 0; i < ml_num_link; i++) {
		for (j = i + 1; j < ml_num_link; j++) {
			if (wlan_reg_is_24ghz_ch_freq(ml_freq_lst[i]) ||
			    wlan_reg_is_24ghz_ch_freq(ml_freq_lst[j]))
				continue;
			if (policy_mgr_2_freq_same_mac_in_dbs(psoc,
							      ml_freq_lst[i],
							      ml_freq_lst[j])) {
				force_cmd->force_inactive_num_bitmap |=
					1 << ml_linkid_lst[i];
				force_cmd->force_inactive_num_bitmap |=
					1 << ml_linkid_lst[j];
				force_cmd->force_inactive_num++;
			}
		}
	}

	if (force_cmd->force_inactive_num >=
			mlo_get_num_5gh_links(ml_freq_lst, ml_num_link))
		force_cmd->force_inactive_num =
			mlo_get_num_5gh_links(ml_freq_lst, ml_num_link) - 1;
}

static void
ml_nlink_handle_legacy_intf_3_ports(struct wlan_objmgr_psoc *psoc,
				    struct wlan_objmgr_vdev *vdev,
				    struct ml_link_force_state *force_cmd,
				    bool emlsr_conn_with_aux,
				    uint8_t num_legacy_vdev,
				    uint8_t *vdev_lst,
				    qdf_freq_t *freq_lst,
				    enum policy_mgr_con_mode *mode_lst)
{
	enum home_channel_map_id ml_link_hc_id = HC_NONE;
	enum home_channel_map_id non_ml_hc_id = HC_NONE;
	enum home_channel_map_id force_inactive_hc_id = HC_NONE;
	enum home_channel_map_id force_inactive_num_hc_id = HC_NONE;
	struct force_inactive_modes force_modes;
	force_inactive_table_type *force_inactive_tlb;
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0;
	uint32_t force_inactive_link_bitmap = 0;
	uint32_t force_inactive_link_bitmap2 = 0;
	uint32_t force_inactive_link_bitmap3 = 0;
	uint32_t force_inactive_num_bitmap = 0;
	uint8_t ml_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t ml_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t ml_linkid_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	struct ml_link_info ml_link_info[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t i = 0, j = 0;
	uint32_t scc_link_bitmap = 0;
	uint32_t vdev_inactive_links = 0;
	bool disallow_mcc = false;
	qdf_freq_t legacy_intf_freq1;
	qdf_freq_t legacy_intf_freq2;
	enum policy_mgr_con_mode legacy_pm_mode;
	uint8_t legacy_vdev_id;
	enum home_channel_map_id force_inact_hc_id;
	enum home_channel_map_id force_inact_hc_id2;
	enum pm_rd_type rd_type = policy_mgr_get_rd_type(psoc);
	bool legacy_2_intf_mcc_scc;

	if (num_legacy_vdev < 2)
		return;

	legacy_intf_freq1 = freq_lst[0];
	legacy_intf_freq2 = freq_lst[1];
	legacy_pm_mode = mode_lst[0];
	legacy_vdev_id = vdev_lst[0];

	ml_nlink_get_link_info(psoc, vdev, NLINK_EXCLUDE_REMOVED_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);
	if (ml_num_link < 2)
		return;

	switch (legacy_pm_mode) {
	case PM_P2P_CLIENT_MODE:
	case PM_P2P_GO_MODE:
		if (!policy_mgr_is_vdev_high_tput_or_low_latency(
					psoc, legacy_vdev_id))
			break;
		fallthrough;
	case PM_STA_MODE:
		disallow_mcc = true;
		break;
	case PM_SAP_MODE:
	case PM_NAN_DISC_MODE:
		ml_nlink_handle_legacy_nan_intf(psoc, force_cmd, vdev);
		break;
	default:
		/* unexpected legacy connection mode */
		mlo_debug("unexpected legacy intf mode %d",
			  legacy_pm_mode);
		return;
	}

	for (i = 0; i < ml_num_link; i++) {
		if (ml_freq_lst[i] == legacy_intf_freq1 ||
		    ml_freq_lst[i] == legacy_intf_freq2) {
			scc_link_bitmap |= 1 << ml_linkid_lst[i];
		} else if (policy_mgr_2_freq_always_on_same_mac(
			   psoc, ml_freq_lst[i], legacy_intf_freq1) ||
			   policy_mgr_2_freq_always_on_same_mac(
			   psoc, ml_freq_lst[i], legacy_intf_freq2)){
			force_inactive_link_bitmap |= 1 << ml_linkid_lst[i];
		}
		if (policy_mgr_vdev_is_force_inactive(psoc, ml_vdev_lst[i]))
			vdev_inactive_links |= 1 << ml_linkid_lst[i];
	}

	ml_link_hc_id = get_hc_id(psoc, ml_num_link, ml_freq_lst);
	if (ml_link_hc_id >= HC_MAX_MAP_ID) {
		mlo_debug("invalid ml_link_hc_id");
		return;
	}

	/* 1. Handle the 2 legacy intf are SCC or MCC firstly. */
	legacy_2_intf_mcc_scc = policy_mgr_2_freq_always_on_same_mac(
			psoc, legacy_intf_freq1, legacy_intf_freq2);
	if (legacy_2_intf_mcc_scc) {
		if (emlsr_conn_with_aux) {
			/* If the legacy intf home channel are always in same
			 * suband (SCC or MCC), such as 2G + 2G, 5GL + 5GL, or
			 * 5GH + 5GH, for EMLSR ML STA case the force inactive
			 * action is similar to one home channel case in same
			 * subband.
			 */
			non_ml_hc_id = get_hc_id(psoc, 1, freq_lst);
			if (non_ml_hc_id >= HC_BAND_MAX) {
				mlo_debug("invalid non_ml_hc_id %d",
					  non_ml_hc_id);
				return;
			}

			force_inactive_tlb =
				get_force_inactive_table(psoc, PM_STA_MODE);
			if (!force_inactive_tlb) {
				mlo_debug("unable to get force inactive tbl");
				return;
			}
			force_modes =
			(*force_inactive_tlb)[ml_link_hc_id][non_ml_hc_id];

			force_inactive_hc_id =
					force_modes.force_inactive_hc_id;
			force_inactive_num_hc_id =
					force_modes.force_inactive_num_hc_id;
			force_inactive_link_bitmap = 0;
			hc_id_2_link_bitmap(psoc, ml_num_link, ml_freq_lst,
					    ml_linkid_lst,
					    force_inactive_hc_id,
					    &force_inactive_link_bitmap);
			hc_id_2_link_bitmap(psoc, ml_num_link, ml_freq_lst,
					    ml_linkid_lst,
					    force_inactive_num_hc_id,
					    &force_inactive_num_bitmap);

			mlo_debug("ml %s legacy %s mode %s %s inact %s inact num %s rd %d",
				  hc_id_to_string(ml_link_hc_id),
				  hc_id_to_string(non_ml_hc_id),
				  device_mode_to_string(mode_lst[0]),
				  device_mode_to_string(mode_lst[1]),
				  hc_id_to_string(force_inactive_hc_id),
				  hc_id_to_string(force_inactive_num_hc_id),
				  policy_mgr_get_rd_type(psoc));
		} else {
			/* force_inactive_link_bitmap contains all ML links
			 * in same subband (includes SCC links).
			 * force_inactive_num_bitmap will contain all other
			 * band's ML links. For non EMLSR case, the links in
			 * force_inactive_num_bitmap have to be forced to
			 * MLSR by force inactive num command.
			 * for example, ML STA 5GL_5GH + SAP 2G + GO 2G: force
			 * inactive num 1 for ML STA 5GL+5GH.
			 */
			force_inactive_link_bitmap |= scc_link_bitmap;
			force_inactive_num_bitmap =
				ml_link_bitmap & ~force_inactive_link_bitmap;
		}
		mlo_debug("scc link 0x%x force inact 0x%x force inact num 0x%x",
			  scc_link_bitmap, force_inactive_link_bitmap,
			  force_inactive_num_bitmap);

		/* 3 vdev on same mac MCC is not supported, if 2 legacy intf
		 * are MCC already, then to force inactive all MCC and SCC ML
		 * links.
		 * If 2 legacy intf are SCC, and SCC links not allowed by
		 * concurrency, force inactive SCC links as well.
		 */
		if (force_inactive_link_bitmap) {
			if (legacy_intf_freq1 != legacy_intf_freq2)
				force_inactive_link_bitmap |= scc_link_bitmap;
			else if (scc_link_bitmap &&
				 !ml_nlink_allow_conc(
						psoc, vdev, scc_link_bitmap,
						force_inactive_link_bitmap &
						~scc_link_bitmap))
				force_inactive_link_bitmap |= scc_link_bitmap;
			else
				force_inactive_link_bitmap &= ~scc_link_bitmap;

			/* Apply the force inactive bitmap only when there are
			 * available ml links after force inactive the bitmap.
			 */
			if (ml_link_bitmap & ~force_inactive_link_bitmap)
				force_cmd->force_inactive_bitmap |=
					force_inactive_link_bitmap;
			else
				mlo_debug("unexpected no left links: avail 0x%x inact 0x%x",
					  ml_link_bitmap,
					  force_inactive_link_bitmap);
		}

		if (force_inactive_num_bitmap) {
			force_cmd->force_inactive_num =
				convert_link_bitmap_to_link_ids(
					force_inactive_num_bitmap, 0, NULL);

			if (force_cmd->force_inactive_num > 1) {
				force_cmd->force_inactive_num--;
				force_cmd->force_inactive_num_bitmap =
						force_inactive_num_bitmap;
			} else {
				force_cmd->force_inactive_num = 0;
			}
		}

		mlo_debug("avail 0x%x disallow_mcc %d final 0x%x inact num 0x%x",
			  ml_link_bitmap, disallow_mcc,
			  force_cmd->force_inactive_bitmap,
			  force_cmd->force_inactive_num_bitmap);
		return;
	}

	/* 2. Handle the 2 legacy intf are DBS or SBS, such as 2G + 5L,
	 * 2G + 5GH, 5GL + 5GH in SBS hw, or 2G + 5G in DBS HW.
	 */
	force_inactive_link_bitmap = 0;

	/* If 2 legacy intf's frequency are on DBS or SBS mode, to avoid
	 * MCC, ML STA will not support EMLSR.
	 * All MCC ML STA links have to be forced to MLSR.
	 */
	if (emlsr_conn_with_aux)
		ml_nlink_handle_mcc_links(psoc, vdev, force_cmd);

	/* 2 legacy intf are DBS or SBS channels,
	 * Basic rule is :
	 * 1.try to force inactive MCC links in same sub band of
	 * legacy intf 1.
	 * 2.try to force inactive links which will cause the 2
	 * legacy intf working in same mac (MCC).
	 * for example GC and GO, here is expected result.
	 * legacy intf 1: GC Low latecy (Need avoid MCC)
	 * Legacy intf 2: GO
	 * GC dbs with GO in SBS RD
	 * 2G		 | 5GL		| 5GH		 | force inactive links
	 * -------------------------------------------------------------------
	 * ML STA+GC MCC | ML STA	| GO		 | 2G link
	 * ML STA+GC SCC | ML STA	| GO		 | 5GL link
	 * ML STA+GC MCC |		| GO + ML STA	 | 2G link
	 * ML STA+GC SCC |		| GO + ML STA	 | none
	 * GC		 | ML STA	| GO + ML STA	 | 5GL link
	 * 2 ML STA+GC	 |		| GO		 | 2G MCC link if one
	 *						 | link SCC or none if
	 *						 | MCC with both links
	 * GC		 | 2 ML STA	| GO		 | none
	 * GC		 |		| GO + 2 ML STA  | none
	 *
	 * ML STA+GC MCC | ML STA + GO	|		 | 2G link
	 * ML STA+GC SCC | ML STA + GO	|		 | none
	 * ML STA+GC MCC | GO		| ML STA	 | 2G link
	 * ML STA+GC SCC | GO		| ML STA	 | 5GH link
	 * GC		 | ML STA + GO	| ML STA	 | 5GH link
	 * 2 ML STA+GC	 | GO		|		 | 2G MCC link if one
	 *						 | link SCC or none if
	 *						 | MCC with both links
	 * GC		 | 2 ML STA + GO|		 | none
	 * GC		 | GO		| 2 ML STA	 | none
	 *
	 * GO		 | ML STA	| ML STA+GC MCC  | 5GH link
	 * GO		 | ML STA	| ML STA+GC SCC  | 5GL link
	 * GO + ML STA	 |		| ML STA+GC MCC  | 5GH link
	 * GO + ML STA	 |		| ML STA+GC SCC  | none
	 * GO + ML STA	 | ML STA	| GC		 | 5GL link
	 * 2 ML STA+GO	 |		| GC		 | none
	 * GO		 | 2 ML STA	| GC		 | none
	 * GO		 |		| GC + 2 ML STA  | 5GH MCC link if one
	 *						 | link SCC or none if
	 *						 | MCC with both links
	 * GO		 | ML STA+GC MCC| ML STA	 | 5GL link
	 * GO		 | ML STA+GC SCC| ML STA	 | 5GH link
	 * GO + ML STA	 | ML STA+GC MCC|		 | 5GL link
	 * GO + ML STA	 | ML STA+GC SCC|		 | none
	 * GO + ML STA	 | GC		| ML STA	 | 5GH link
	 * 2 ML STA+GO	 | GC		|		 | none
	 * GO		 | GC + 2 ML STA|		 | 5GL MCC link if one
	 *						 | link SCC or none if
	 *						 | MCC with both links
	 * GO		 | GC		| 2 ML STA	 | none
	 *
	 * GC SBS with GO in SBS RD
	 * ML STA	 | ML STA+GC MCC| GO		 | 5GL link
	 * ML STA	 | ML STA+GC SCC| GO		 | 2G link
	 *		 | ML STA+GC MCC| GO + ML STA	 | 5GL link
	 *		 | ML STA+GC SCC| GO + ML STA	 | none
	 * ML STA	 | GC		| GO + ML STA	 | 2G link
	 * 2 ML STA	 | GC		| GO		 | none
	 *		 | GC+2 ML STA	| GO		 | 5GL MCC link if one
	 *						 | link SCC or none if
	 *						 | MCC with both links
	 *		 | GC		| GO + 2 ML STA  | none
	 *
	 * ML STA	 | GO		| GC + ML STA MCC| 5GH link
	 * ML STA	 | GO		| GC + ML STA SCC| 2G link
	 *		 | GO + ML STA	| GC + ML STA MCC| 5GH link
	 *		 | GO + ML STA	| GC + ML STA SCC| none
	 * ML STA	 | ML STA+GO	| GC		 | 2G link
	 * 2 ML STA	 | GO		| GC		 | none
	 *		 | 2 ML STA+GO	| GC		 | none
	 *		 | GO		| GC + 2 ML STA  | 5GH MCC link if one
	 *						 | link SCC or none if
	 *						 | MCC with both links
	 */
	non_ml_hc_id = get_hc_id(psoc, num_legacy_vdev, freq_lst);
	if (non_ml_hc_id >= HC_LEGACY_MAX) {
		mlo_debug("unexpected non_ml_hc_id %d", non_ml_hc_id);
		return;
	}

	force_inact_hc_id = HC_NONE;
	force_inact_hc_id2 = HC_NONE;

	if (rd_type == pm_rd_sbs_low_share ||
	    rd_type == pm_rd_sbs_upper_share ||
	    rd_type == pm_rd_sbs_switchable) {
		/* SBS rd */
		if (mode_lst[0] == PM_SAP_MODE &&
		    mode_lst[1] == PM_SAP_MODE) {
			if (non_ml_hc_id == HC_5GL_5GH)
				force_inact_hc_id2 = HC_2G;
			else if (non_ml_hc_id == HC_2G_5GL)
				force_inact_hc_id2 = HC_5GH;
			else if (non_ml_hc_id == HC_2G_5GH) {
				if (ml_link_hc_id == HC_2G_5GL)
					mlo_debug("Skip nlink force for STA(2G+5GL)+SAP(2G+5GH)");
				else
					force_inact_hc_id2 = HC_5GL;
			} else {
				mlo_debug("SAP+SAP, unexpected non_ml_hc_id %d",
					  non_ml_hc_id);
			}
		} else if (disallow_mcc) {
			force_inact_hc_id =
				get_hc_id(psoc, 1, freq_lst);
			if (force_inact_hc_id >= HC_LEGACY_MAX) {
				mlo_debug("disallow_mcc %d invalid force_inact_hc_id %d",
					  disallow_mcc,
					  force_inact_hc_id);
				return;
			}

			if (non_ml_hc_id == HC_5GL_5GH)
				force_inact_hc_id2 = HC_2G;
			else if (non_ml_hc_id == HC_2G_5GL)
				force_inact_hc_id2 = HC_5GH;
			else if (non_ml_hc_id == HC_2G_5GH)
				force_inact_hc_id2 = HC_5GL;
			else
				mlo_debug("disallow_mcc %d unexpected non_ml_hc_id %d",
					  disallow_mcc,
					  non_ml_hc_id);
		} else if (mode_lst[0] == PM_SAP_MODE ||
			   mode_lst[1] == PM_SAP_MODE) {
			if (non_ml_hc_id == HC_5GL_5GH)
				force_inact_hc_id2 = HC_2G;
			else if (non_ml_hc_id == HC_2G_5GL)
				force_inact_hc_id2 = HC_5GH;
			else if (non_ml_hc_id == HC_2G_5GH)
				force_inact_hc_id2 = HC_5GL;
			else
				mlo_debug("one SAP unexpected non_ml_hc_id %d",
					  non_ml_hc_id);
		} else {
			/* HC_NONE for force_inact_hc_id, force_inact_hc_id2 */
		}
	} else {
		/* DBS rd */
		if (mode_lst[0] == PM_SAP_MODE &&
		    mode_lst[1] == PM_SAP_MODE) {
			if (non_ml_hc_id == HC_2G_5GL)
				force_inact_hc_id2 = HC_NONE;
			else if (non_ml_hc_id == HC_2G_5GH)
				force_inact_hc_id2 = HC_NONE;
			else
				mlo_debug("SAP+SAP, unexpected non_ml_hc_id %d",
					  non_ml_hc_id);
		} else if (disallow_mcc) {
			force_inact_hc_id =
				get_hc_id(psoc, 1, freq_lst);
			if (force_inact_hc_id >= HC_LEGACY_MAX) {
				mlo_debug("disallow_mcc %d invalid force_inact_hc_id %d",
					  disallow_mcc,
					  force_inact_hc_id);
				return;
			}

			if (non_ml_hc_id == HC_2G_5GL)
				force_inact_hc_id2 = HC_NONE;
			else if (non_ml_hc_id == HC_2G_5GH)
				force_inact_hc_id2 = HC_NONE;
			else
				mlo_debug("disallow_mcc %d unexpected non_ml_hc_id %d",
					  disallow_mcc,
					  non_ml_hc_id);
		} else if (mode_lst[0] == PM_SAP_MODE ||
			   mode_lst[1] == PM_SAP_MODE) {
			if (non_ml_hc_id == HC_2G_5GL)
				force_inact_hc_id2 = HC_NONE;
			else if (non_ml_hc_id == HC_2G_5GH)
				force_inact_hc_id2 = HC_NONE;
			else
				mlo_debug("one SAP unexpected non_ml_hc_id %d",
					  non_ml_hc_id);
		} else {
			/* HC_NONE for force_inact_hc_id, force_inact_hc_id2 */
		}
	}

	hc_id_2_link_bitmap(psoc, ml_num_link, ml_freq_lst, ml_linkid_lst,
			    force_inact_hc_id,
			    &force_inactive_link_bitmap);
	hc_id_2_link_bitmap(psoc, ml_num_link, ml_freq_lst, ml_linkid_lst,
			    force_inact_hc_id2,
			    &force_inactive_link_bitmap2);

	/* SAP on 5GH and ML STA on 6G, force inactive 6G to avoid MCC */
	for (i = 0; i < num_legacy_vdev; i++) {
		if (mode_lst[i] != PM_SAP_MODE ||
		    !WLAN_REG_IS_5GHZ_CH_FREQ(freq_lst[i]))
			continue;
		for (j = 0; j < ml_num_link; j++) {
			if (WLAN_REG_IS_6GHZ_CHAN_FREQ(ml_freq_lst[j]) &&
			    policy_mgr_2_freq_always_on_same_mac(
					psoc, ml_freq_lst[j], freq_lst[i])) {
				force_inactive_link_bitmap3 |=
					1 << ml_linkid_lst[j];
				break;
			}
		}
		if (force_inactive_link_bitmap3)
			break;
	}

	mlo_debug("ml %s legacy %s mode %s %s inact %s inact2 %s rd %d",
		  hc_id_to_string(ml_link_hc_id),
		  hc_id_to_string(non_ml_hc_id),
		  device_mode_to_string(mode_lst[0]),
		  device_mode_to_string(mode_lst[1]),
		  hc_id_to_string(force_inact_hc_id),
		  hc_id_to_string(force_inact_hc_id2),
		  policy_mgr_get_rd_type(psoc));
	mlo_debug("scc link 0x%x force inact 0x%x inact2 0x%x inact3 0x%x",
		  scc_link_bitmap, force_inactive_link_bitmap,
		  force_inactive_link_bitmap2, force_inactive_link_bitmap3);

	force_inactive_link_bitmap &= ~scc_link_bitmap;
	force_inactive_link_bitmap2 &= ~scc_link_bitmap;
	force_inactive_link_bitmap3 &= ~scc_link_bitmap;

	if (ml_link_bitmap & ~force_inactive_link_bitmap)
		force_cmd->force_inactive_bitmap |=
			force_inactive_link_bitmap;

	if (ml_link_bitmap & ~force_cmd->force_inactive_bitmap &
	    ~force_inactive_link_bitmap2)
		force_cmd->force_inactive_bitmap |=
			force_inactive_link_bitmap2;

	if (ml_link_bitmap & ~force_cmd->force_inactive_bitmap &
	    ~force_inactive_link_bitmap3)
		force_cmd->force_inactive_bitmap |=
			force_inactive_link_bitmap3;

	if (vdev_inactive_links &&
	    !(force_cmd->force_inactive_bitmap & vdev_inactive_links) &&
	    !ml_nlink_allow_conc(psoc, vdev, vdev_inactive_links,
				 force_cmd->force_inactive_bitmap)) {
		force_cmd->force_inactive_bitmap |= vdev_inactive_links;
		force_inactive_link_bitmap |= vdev_inactive_links;
	}

	mlo_debug("avail 0x%x disallow_mcc %d inact vdev link 0x%x final 0x%x inact num 0x%x",
		  ml_link_bitmap, disallow_mcc,
		  vdev_inactive_links,
		  force_cmd->force_inactive_bitmap,
		  force_cmd->force_inactive_num_bitmap);
}

static void
ml_nlink_handle_standby_link_3_ports(
		struct wlan_objmgr_psoc *psoc,
		struct wlan_objmgr_vdev *vdev,
		struct ml_link_force_state *force_cmd,
		uint8_t num_legacy_vdev,
		uint8_t *vdev_lst,
		qdf_freq_t *freq_lst,
		enum policy_mgr_con_mode *mode_lst)
{
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0;
	uint32_t force_inactive_link_bitmap = 0;
	uint8_t ml_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t ml_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t ml_linkid_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	struct ml_link_info ml_link_info[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t i, j;

	if (num_legacy_vdev < 2)
		return;

	ml_nlink_get_link_info(psoc, vdev, NLINK_EXCLUDE_REMOVED_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);
	if (ml_num_link < 2)
		return;

	if (policy_mgr_2_freq_always_on_same_mac(
				psoc, freq_lst[0], freq_lst[1]))
		return;

	for (i = 0; i < ml_num_link; i++) {
		if (ml_vdev_lst[i] != WLAN_INVALID_VDEV_ID)
			continue;

		for (j = 0; j < num_legacy_vdev; j++) {
			if (ml_freq_lst[i] == freq_lst[j])
				break;
		}
		/* standby link will be force inactive if not SCC with
		 * any legacy interface, for example SAP 2G, GO 5GL, standby
		 * link on 5GH
		 */
		if (j == num_legacy_vdev)
			force_inactive_link_bitmap |= 1 << ml_linkid_lst[i];
	}

	if (ml_link_bitmap & ~force_cmd->force_inactive_bitmap &
	    ~force_inactive_link_bitmap)
		force_cmd->force_inactive_bitmap |= force_inactive_link_bitmap;
}

/**
 * ml_nlink_handle_legacy_intf_emlsr() - Check force inactive needed
 * with legacy interface for eMLSR connection
 * @psoc: PSOC object information
 * @vdev: vdev object
 * @force_cmd: force command to be returned
 *
 * Return: void
 */
static void
ml_nlink_handle_legacy_intf_emlsr(struct wlan_objmgr_psoc *psoc,
				  struct wlan_objmgr_vdev *vdev,
				  struct ml_link_force_state *force_cmd)
{
	uint8_t vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	enum policy_mgr_con_mode mode_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t num_legacy_vdev;
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);

	num_legacy_vdev = policy_mgr_get_legacy_conn_info(
					psoc, vdev_lst,
					freq_lst, mode_lst,
					QDF_ARRAY_SIZE(vdev_lst));
	if (wlan_nan_is_disc_active(psoc)) {
		if (num_legacy_vdev < MAX_LEGACY_CONCURRENCT_MODES &&
		    wlan_nan_get_5ghz_social_ch_freq(pdev)) {
			freq_lst[num_legacy_vdev] =
				wlan_nan_get_5ghz_social_ch_freq(pdev);
			mode_lst[num_legacy_vdev++] = PM_NAN_DISC_MODE;
		}

		if (num_legacy_vdev < MAX_LEGACY_CONCURRENCT_MODES &&
		    wlan_nan_get_24ghz_social_ch_freq(pdev)) {
			freq_lst[num_legacy_vdev] =
				wlan_nan_get_24ghz_social_ch_freq(pdev);
			mode_lst[num_legacy_vdev++] = PM_NAN_DISC_MODE;
		}
	}

	if (!num_legacy_vdev)
		return;

	/* handle 2 port case with 2 ml sta links.
	 */
	if (num_legacy_vdev == 1) {
		switch (mode_lst[0]) {
		case PM_P2P_CLIENT_MODE:
		case PM_P2P_GO_MODE:
		case PM_STA_MODE:
		case PM_SAP_MODE:
			ml_nlink_handle_comm_intf_emlsr(
				psoc, vdev, force_cmd, vdev_lst[0],
				freq_lst[0], mode_lst[0]);
			break;
		default:
			/* unexpected legacy connection mode */
			mlo_debug("unexpected legacy intf mode %d",
				  mode_lst[0]);
			return;
		}
		ml_nlink_dump_force_state(force_cmd, "");
		return;
	}

	/* handle 3 port case with 2 ml sta links.
	 */
	switch (mode_lst[0]) {
	case PM_P2P_CLIENT_MODE:
	case PM_P2P_GO_MODE:
	case PM_STA_MODE:
	case PM_SAP_MODE:
	case PM_NAN_DISC_MODE:
		ml_nlink_handle_legacy_intf_3_ports(
			psoc, vdev, force_cmd, true, num_legacy_vdev,
			vdev_lst, freq_lst, mode_lst);
		break;
	default:
		/* unexpected legacy connection count */
		mlo_debug("unexpected legacy intf mode %d", mode_lst[0]);
		return;
	}
	ml_nlink_handle_standby_link_3_ports(psoc, vdev, force_cmd,
					     num_legacy_vdev,
					     vdev_lst,
					     freq_lst,
					     mode_lst);

	ml_nlink_dump_force_state(force_cmd, "");
}

/**
 * mlo_is_valid_nan_p2p_concurrency() - Check if NAN+P2P concurrency is valid
 * @psoc: PSOC object information
 * @mode_lst: Legacy active modes present in policy mgr
 * @vdev_lst: vdev_lst corresponds to @mode_lst
 * @num_legacy_vdev: Number of legacy vdevs present in policy mgr
 *
 * This API is to decide whether any ML-STA links to be made inactive inorder
 * to allow the concurrency.
 * NAN+P2P+ML-STA can be supported on a mac even if these lead to 3-ch MCC.
 * So, don't make any link inactive when NAN + non-LL P2P is present
 *
 * Return: True when ML-STA+NAN+P2P is a valid concurrency
 */
static bool
mlo_is_valid_nan_p2p_concurrency(struct wlan_objmgr_psoc *psoc,
				 enum policy_mgr_con_mode *mode_lst,
				 uint8_t *vdev_lst,
				 uint8_t num_legacy_vdev)
{
	uint8_t i;

	if (!wlan_nan_is_sta_p2p_ndp_supported(psoc) ||
	    !wlan_nan_is_disc_active(psoc))
		return false;

	for (i = 0; i < num_legacy_vdev; i++)
		if ((mode_lst[i] == PM_P2P_GO_MODE ||
		     mode_lst[i] == PM_P2P_CLIENT_MODE) &&
		    !policy_mgr_is_vdev_high_tput_or_low_latency(psoc,
								 vdev_lst[i]))
			return true;
	return false;
}

/**
 * ml_nlink_handle_legacy_intf() - Check force inactive needed
 * with legacy interface
 * @psoc: PSOC object information
 * @vdev: vdev object
 * @force_cmd: force command to be returned
 *
 * Return: void
 */
static void
ml_nlink_handle_legacy_intf(struct wlan_objmgr_psoc *psoc,
			    struct wlan_objmgr_vdev *vdev,
			    struct ml_link_force_state *force_cmd)
{
	uint8_t vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	enum policy_mgr_con_mode mode_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t num_legacy_vdev;
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);

	num_legacy_vdev = policy_mgr_get_legacy_conn_info(
					psoc, vdev_lst,
					freq_lst, mode_lst,
					QDF_ARRAY_SIZE(vdev_lst));

	if (mlo_is_valid_nan_p2p_concurrency(psoc, mode_lst, vdev_lst,
					     num_legacy_vdev))
		return;

	if (wlan_nan_is_disc_active(psoc)) {
		if (num_legacy_vdev < MAX_LEGACY_CONCURRENCT_MODES &&
		    wlan_nan_get_5ghz_social_ch_freq(pdev)) {
			freq_lst[num_legacy_vdev] =
				wlan_nan_get_5ghz_social_ch_freq(pdev);
			mode_lst[num_legacy_vdev++] = PM_NAN_DISC_MODE;
		}

		if (num_legacy_vdev < MAX_LEGACY_CONCURRENCT_MODES &&
		    wlan_nan_get_24ghz_social_ch_freq(pdev)) {
			freq_lst[num_legacy_vdev] =
				wlan_nan_get_24ghz_social_ch_freq(pdev);
			mode_lst[num_legacy_vdev++] = PM_NAN_DISC_MODE;
		}
	}
	if (!num_legacy_vdev)
		return;
	/* 2 port case with 2 ml sta links or
	 * 2 port case with 3 ml sta links
	 */
	if (num_legacy_vdev == 1) {
		switch (mode_lst[0]) {
		case PM_STA_MODE:
			ml_nlink_handle_legacy_sta_intf(
				psoc, vdev, force_cmd, vdev_lst[0],
				freq_lst[0]);
			break;
		case PM_SAP_MODE:
			ml_nlink_handle_legacy_sap_intf(
				psoc, vdev, force_cmd, vdev_lst[0],
				freq_lst[0]);
			break;
		case PM_P2P_CLIENT_MODE:
		case PM_P2P_GO_MODE:
			ml_nlink_handle_legacy_p2p_intf(
				psoc, vdev, force_cmd, vdev_lst[0],
				freq_lst[0]);
			break;
		case PM_NAN_DISC_MODE:
			break;
		default:
			/* unexpected legacy connection count */
			mlo_debug("unexpected legacy intf mode %d",
				  mode_lst[0]);
			return;
		}
		ml_nlink_dump_force_state(force_cmd, "");
		return;
	}
	/* 3 ports case with ml sta 2 or 3 links, suppose port 3 vdev is
	 * low latency legacy vdev:
	 * 6G: ML Link + Port2 + Port3 | 5G: ML Link
	 *	=> no op
	 * 6G: ML Link + Port2	       | 5G: ML Link + Port3
	 *	=> disable 5G link if 5G mcc
	 * 6G: ML Link + Port3	       | 5G: ML Link + Port2
	 *	=> disable 6G link if 6G mcc
	 * 6G: ML Link		       | 5G: ML Link + Port3 | 2G: Port2
	 *	=> disable 5G link if 5G mcc.
	 * 6G: ML Link		       | 5G: ML Link + Port2 | 2G: Port3
	 *	=> disable 6g link.
	 * 6G: ML Link + Port3	       | 5G: ML Link | 2G: Port2
	 *	=> disable 6G link if 6G mcc.
	 * 6G: ML Link + Port2	       | 5G: ML Link | 2G: Port3
	 *	=> disable 6g link.
	 * 6G: ML Link + Port2 + Port3 | 2G: ML Link
	 *	=> no op
	 * 6G: ML Link + Port2	       | 2G: ML Link + Port3
	 *	=> disable 2G link if 2G mcc
	 * 6G: ML Link + Port3	       | 2G: ML Link + Port2
	 *	=> disable 6G link if 6G mcc
	 * 6G: ML Link		       | 2G: ML Link + Port3 | 5G: Port2
	 *	=> disable 2G link if 2G mcc.
	 * 6G: ML Link		       | 2G: ML Link + Port2 | 5GL: Port3
	 *	=> disable 6G link
	 * 6G: ML Link + Port3	       | 2G: ML Link | 5G: Port2
	 *	=> disable 6G link if 6G mcc.
	 * 6G: ML Link + Port2	       | 2G: ML Link | 5GL: Port3
	 *	=> disable 2G link
	 * general rule:
	 * If any link is always mcc with Port3, then force inactive the link.
	 * And if standby link is MCC with any Legacy Ports, then disable
	 * standby link as well.
	 */
	switch (mode_lst[0]) {
	case PM_P2P_CLIENT_MODE:
	case PM_P2P_GO_MODE:
	case PM_STA_MODE:
	case PM_SAP_MODE:
	case PM_NAN_DISC_MODE:
		ml_nlink_handle_legacy_intf_3_ports(
			psoc, vdev, force_cmd, false, num_legacy_vdev,
			vdev_lst, freq_lst, mode_lst);
		break;
	default:
		/* unexpected legacy connection count */
		mlo_debug("unexpected legacy intf mode %d", mode_lst[0]);
		return;
	}

	ml_nlink_handle_standby_link_3_ports(psoc, vdev, force_cmd,
					     num_legacy_vdev,
					     vdev_lst,
					     freq_lst,
					     mode_lst);
	ml_nlink_dump_force_state(force_cmd, "");
}

/**
 * ml_nlink_handle_dynamic_inactive() - Handle dynamic force inactive num
 * with legacy SAP
 * @psoc: PSOC object information
 * @vdev: vdev object
 * @curr: current force command state
 * @new: new force command
 *
 * If ML STA 2 or 3 links are present and force inactive num = 1 with dynamic
 * flag enabled for some reason, FW will report the current inactive links,
 * host will select one and save to curr_dynamic_inactive_bitmap.
 * If SAP starting on channel which is same mac as links in
 * the curr_dynamic_inactive_bitmap, host will force inactive the links in
 * curr_dynamic_inactive_bitmap to avoid FW link switch between the dynamic
 * inactive links.
 *
 * Return: void
 */
static void
ml_nlink_handle_dynamic_inactive(struct wlan_objmgr_psoc *psoc,
				 struct wlan_objmgr_vdev *vdev,
				 struct ml_link_force_state *curr,
				 struct ml_link_force_state *new)
{
	uint8_t vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	enum policy_mgr_con_mode mode_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t num;
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap;
	uint32_t force_inactive_link_bitmap = 0;
	uint8_t ml_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t ml_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t ml_linkid_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	struct ml_link_info ml_link_info[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint32_t i, j;

	/* If force inactive num wasn't sent to fw, no need to handle
	 * dynamic inactive links.
	 */
	if (!curr->force_inactive_num ||
	    !curr->force_inactive_num_bitmap ||
	    !curr->curr_dynamic_inactive_bitmap)
		return;
	if (curr->force_inactive_num != new->force_inactive_num ||
	    curr->force_inactive_num_bitmap !=
				new->force_inactive_num_bitmap)
		return;
	/* If links have been forced inactive by bitmap, no need to force
	 * again.
	 */
	if ((new->force_inactive_bitmap &
	     curr->curr_dynamic_inactive_bitmap) ==
	    curr->curr_dynamic_inactive_bitmap)
		return;

	num = policy_mgr_get_legacy_conn_info(
					psoc, vdev_lst,
					freq_lst, mode_lst,
					QDF_ARRAY_SIZE(vdev_lst));
	if (!num)
		return;
	ml_nlink_get_link_info(psoc, vdev, NLINK_EXCLUDE_REMOVED_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);
	if (ml_num_link < 2)
		return;
	for (i = 0; i < ml_num_link; i++) {
		if (!((1 << ml_linkid_lst[i]) &
		      curr->curr_dynamic_inactive_bitmap))
			continue;
		for (j = 0; j < num; j++) {
			if (mode_lst[j] != PM_SAP_MODE)
				continue;
			if (policy_mgr_2_freq_always_on_same_mac(
				psoc, freq_lst[j], ml_freq_lst[i])) {
				force_inactive_link_bitmap |=
					1 << ml_linkid_lst[i];
				mlo_debug("force dynamic inactive link id %d freq %d for sap freq %d",
					  ml_linkid_lst[i], ml_freq_lst[i],
					  freq_lst[j]);
			} else if (num > 1 &&
				   policy_mgr_are_2_freq_on_same_mac(
					psoc, freq_lst[j], ml_freq_lst[i])) {
				force_inactive_link_bitmap |=
					1 << ml_linkid_lst[i];
				mlo_debug("force dynamic inactive link id %d freq %d for sap freq %d",
					  ml_linkid_lst[i], ml_freq_lst[i],
					  freq_lst[j]);
			}
		}
	}
	if (force_inactive_link_bitmap) {
		new->force_inactive_bitmap |= force_inactive_link_bitmap;
		ml_nlink_dump_force_state(new, "");
	}
}

static bool
ml_nlink_check_allow_link_req(struct wlan_objmgr_psoc *psoc,
			      struct wlan_objmgr_vdev *vdev,
			      struct ml_link_force_state *curr,
			      struct set_link_req *new)
{
	uint16_t no_force_links;

	if (!new->force_inactive_bitmap &&
	    !new->force_inactive_num &&
	    !new->force_active_bitmap &&
	    !new->force_active_num &&
	    (curr->force_inactive_bitmap ||
	     curr->force_inactive_num ||
	     curr->force_active_bitmap ||
	     curr->force_active_num)) {
		/* If link is force inactive already, but new command will
		 * mark it non-force, need to check conc allow or not.
		 */
		no_force_links = curr->force_inactive_bitmap;
		/* Check non forced links allowed by conc */
		if (!ml_nlink_allow_conc(psoc, vdev, no_force_links, 0))
			return false;
	}

	if (new->force_inactive_bitmap != curr->force_inactive_bitmap) {
		/* If link is force inactive already, but new command will
		 * mark it non-force, need to check conc allow or not.
		 */
		no_force_links = curr->force_inactive_bitmap &
				 new->force_inactive_bitmap;
		no_force_links ^= curr->force_inactive_bitmap;

		/* Check non forced links allowed by conc */
		if (!ml_nlink_allow_conc(psoc, vdev, no_force_links,
					 new->force_inactive_bitmap))
			return false;
	}

	if (new->force_active_bitmap != curr->force_active_bitmap) {
		/* Check forced active links allowed by conc */
		if (!ml_nlink_allow_conc(psoc, vdev, new->force_active_bitmap,
					 new->force_inactive_bitmap))
			return false;
	}

	return true;
}

static bool
ml_nlink_validate_request(struct wlan_objmgr_psoc *psoc,
			  struct wlan_objmgr_vdev *vdev,
			  struct set_link_req *combined,
			  enum set_link_source source,
			  struct set_link_req *request,
			  struct set_link_req *combined_new)

{
	uint32_t available_links;
	uint32_t link_bitmap, link_num;
	struct set_link_req tmp;
	struct ml_link_force_state curr;

	if (!request->force_active_bitmap &&
	    !request->force_inactive_bitmap &&
	    !request->force_active_num &&
	    !request->force_active_num_bitmap &&
	    !request->force_inactive_num &&
	    !request->force_inactive_num_bitmap) {
		qdf_mem_copy(combined_new, combined, sizeof(*combined));
		return true;
	}

	available_links = ml_nlink_get_available_link_bitmap(psoc, vdev);

	/* validate request conflict with itself,
	 * For an external request, we support either force
	 * bitmap(USER mode) or force num(MIX mode) but not
	 * support both at same time.
	 */
	if (request->force_active_bitmap &
	    request->force_inactive_bitmap) {
		mlo_debug("req source %d not supported: force_active_bitmap 0x%x inact 0x%x conflict",
			  source, request->force_active_bitmap,
			  request->force_inactive_bitmap);
		return false;
	}
	if (request->force_active_bitmap &&
	    (request->force_active_num ||
	     request->force_active_num_bitmap ||
	     request->force_inactive_num ||
	     request->force_inactive_num_bitmap)) {
		mlo_debug("req source %d not supported: force_active_bitmap 0x%x conflict with force num",
			  source, request->force_active_bitmap);
		return false;
	}
	if (request->force_inactive_bitmap &&
	    (request->force_active_num ||
	     request->force_active_num_bitmap ||
	     request->force_inactive_num ||
	     request->force_inactive_num_bitmap)) {
		mlo_debug("req source %d not supported: force_inactive_bitmap 0x%x conflict with force num",
			  source, request->force_inactive_bitmap);
		return false;
	}
	if ((request->force_active_num &&
	     !request->force_active_num_bitmap) ||
	    (!request->force_active_num &&
	    request->force_active_num_bitmap)) {
		mlo_debug("req source %d not supported: force act num bitmap 0",
			  source);
		return false;
	}
	if ((request->force_inactive_num &&
	     !request->force_inactive_num_bitmap) ||
	    (!request->force_inactive_num &&
	     request->force_inactive_num_bitmap)) {
		mlo_debug("req source %d not supported: force inact num bitmap 0",
			  source);
		return false;
	}
	if (request->force_inactive_num &&
	    request->force_active_num) {
		mlo_debug("req source %d not supported: force inact num %d mix with act num %d in req",
			  source, request->force_inactive_num,
			  request->force_active_num);
		return false;
	}

	/* validate force active bitmap */
	if (request->force_active_bitmap) {
		if (request->force_active_bitmap &
		    combined->force_inactive_bitmap) {
			mlo_debug("req source %d not supported: act bitmap 0x%x conflict with high priority inact bitmap 0x%x",
				  source, request->force_active_bitmap,
				  combined->force_inactive_bitmap);
			return false;
		}

		link_bitmap = ~request->force_active_bitmap &
			combined->force_inactive_num_bitmap;
		if (combined->force_inactive_num_bitmap) {
			if (!link_bitmap) {
				mlo_debug("req source %d not supported: act bitmap 0x%x conflict with high priority forc inact num",
					  source,
					  request->force_active_bitmap);
				return false;
			}
			link_num =
			convert_link_bitmap_to_link_ids(link_bitmap,
							0, NULL);
			if (link_num < combined->force_inactive_num) {
				mlo_debug("req source %d not supported: act bitmap 0x%x, can't force inact num for high priority",
					  source,
					  request->force_active_bitmap);
				return false;
			}
		}
	}

	/* validate force inactive bitmap */
	if (request->force_inactive_bitmap) {
		if (request->force_inactive_bitmap &
		    combined->force_active_bitmap) {
			mlo_debug("req source %d not supported: inact bitmap 0x%x conflict with high priority act bitmap 0x%x",
				  source, request->force_inactive_bitmap,
				  combined->force_active_bitmap);
			return false;
		}
		/* if no available link after force inactive links,
		 * reject it.
		 */
		if (!(available_links &
		      ~(request->force_inactive_bitmap |
			combined->force_inactive_bitmap))) {
			mlo_debug("req source %d not supported: no available link after force inact bitmap 0x%x",
				  source, request->force_inactive_bitmap);
			return false;
		}
		link_bitmap = ~request->force_inactive_bitmap &
			combined->force_active_num_bitmap;
		if (combined->force_active_num_bitmap) {
			if (!link_bitmap) {
				mlo_debug("req source %d not supported: can't force act number bitmap 0x%x for high priority",
					  source,
					  combined->force_active_num_bitmap);
				return false;
			}
			link_num =
			convert_link_bitmap_to_link_ids(link_bitmap,
							0, NULL);
			if (link_num < combined->force_active_num) {
				mlo_debug("req source %d not supported: no available link to force act num",
					  source);
				return false;
			}
		}
	}

	/* validate force active num */
	if (request->force_active_num_bitmap) {
		if (!request->force_active_num) {
			mlo_debug("req source %d not supported: req force act num 0",
				  source);
			return false;
		}
		link_bitmap = request->force_active_num_bitmap &
			~combined->force_inactive_bitmap;
		if (combined->force_inactive_bitmap) {
			if (!link_bitmap) {
				mlo_debug("req source %d not supported: no available link to force act num bitmap 0x%x",
					  source,
					  request->force_active_num_bitmap);
				return false;
			}
			link_num =
			convert_link_bitmap_to_link_ids(link_bitmap,
							0, NULL);
			if (link_num < request->force_active_num) {
				mlo_debug("req source %d not supported: no available link to force act num %d",
					  source, request->force_active_num);
				return false;
			}
		}
		/* Too much combinations: But in real world
		 * vendor command may send MIX mode with force
		 * active num 1 or 2.
		 * In DBS rd, internal force command is:
		 *  force inactive num 1 with 5G+6G bitmap.
		 * Then if receive Mix mode force active num 1, we can accept.
		 * Then if receive Mix mode force active num 2:
		 *   Bitmap 2G+5G or 2G+6G, we accept.
		 *   Bitmap 5G+6G, we reject.
		 */
		if (request->force_active_num > 2) {
			mlo_debug("req source %d not supported: force_active_num %d > 2",
				  source, request->force_active_num);
			return false;
		}
		if (request->force_active_num > 1 &&
		    combined->force_inactive_num) {
			link_bitmap = ~request->force_active_num_bitmap &
				combined->force_inactive_num_bitmap;
			if (!link_bitmap) {
				mlo_debug("req source %d not supported: force act num %d conflict with inact num %d",
					  source, request->force_active_num,
					  combined->force_inactive_num);
				return false;
			}
			link_num =
			convert_link_bitmap_to_link_ids(link_bitmap,
							0, NULL);
			if (link_num < combined->force_inactive_num) {
				mlo_debug("req source %d not supported: no available link to force inact",
					  source);
				return false;
			}
		}
	}

	/* validate force inactive num. */
	if (request->force_inactive_num_bitmap) {
		if (!request->force_inactive_num) {
			mlo_debug("req source %d not supported: req force inact num 0",
				  source);
			return false;
		}
		link_bitmap = ~combined->force_active_bitmap &
			request->force_inactive_num_bitmap;
		if (combined->force_active_bitmap) {
			if (!link_bitmap) {
				mlo_debug("req source %d not supported: no available link to force inact num",
					  source);
				return false;
			}
			link_num =
			convert_link_bitmap_to_link_ids(link_bitmap,
							0, NULL);
			if (link_num < request->force_inactive_num) {
				mlo_debug("req source %d not supported: no available link to force inact num bitmap 0x%x",
					  source,
					  request->force_inactive_num_bitmap);
				return false;
			}
		}
		if (request->force_inactive_num > 2) {
			mlo_debug("req source %d not supported: force_inactive_num %d",
				  source, request->force_inactive_num);
			return false;
		}
	}

	/* force num can't be combined, the request can be honored
	 * only if there is no force num in higher priority request.
	 */
	if (combined->force_active_num &&
	    request->force_active_num) {
		mlo_debug("req source %d not supported: high priority force active num present",
			  source);
		return false;
	}
	if (combined->force_inactive_num &&
	    request->force_inactive_num) {
		mlo_debug("req source %d not supported: high priority force inactive num present",
			  source);
		return false;
	}

	qdf_mem_copy(&tmp, combined, sizeof(tmp));
	tmp.force_active_bitmap |= request->force_active_bitmap;
	tmp.force_inactive_bitmap |=
			request->force_inactive_bitmap;
	if (!tmp.force_active_num) {
		tmp.force_active_num = request->force_active_num;
		tmp.force_active_num_bitmap =
			request->force_active_num_bitmap;
	}
	if (!tmp.force_inactive_num) {
		tmp.force_inactive_num = request->force_inactive_num;
		tmp.force_inactive_num_bitmap =
			request->force_inactive_num_bitmap;
	}

	ml_nlink_get_curr_force_state(psoc, vdev, &curr);

	/* If concurrency not allowed after apply external request,
	 * reject the external request.
	 */
	if (!ml_nlink_check_allow_link_req(psoc, vdev, &curr, &tmp)) {
		mlo_debug("req source %d not supported by conc check",
			  source);
		return false;
	}

	qdf_mem_copy(combined_new, &tmp, sizeof(tmp));

	mlo_debug("source %d act 0x%x inact 0x%x %d 0x%x %d 0x%x",
		  source, request->force_active_bitmap,
		  request->force_inactive_bitmap,
		  request->force_active_num,
		  request->force_active_num_bitmap,
		  request->force_inactive_num,
		  request->force_inactive_num_bitmap);

	return true;
}

static bool
ml_nlink_validate_link_request(struct wlan_objmgr_psoc *psoc,
			       struct wlan_objmgr_vdev *vdev,
			       enum set_link_source source,
			       struct set_link_req *link_request)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct set_link_req request, combined, combined_new;
	uint8_t i;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return false;
	}
	if (source <= 0 || source >= SET_LINK_SOURCE_MAX) {
		mlo_err("invalid src %d", source);
		return false;
	}
	qdf_mem_zero(&combined, sizeof(combined));
	ml_nlink_get_force_link_request(psoc, vdev, &combined, 0);
	for (i = 1; i < source; i++) {
		qdf_mem_zero(&request, sizeof(request));
		ml_nlink_get_force_link_request(psoc, vdev, &request,
						i);

		if (!ml_nlink_validate_request(psoc, vdev, &combined,
					       i, &request, &combined_new))
			continue;

		qdf_mem_copy(&combined, &combined_new, sizeof(combined_new));
	}

	if (!ml_nlink_validate_request(psoc, vdev, &combined,
				       source, link_request, &combined_new))
		return false;

	return true;
}

static void
ml_nlink_handle_all_link_request(struct wlan_objmgr_psoc *psoc,
				 struct wlan_objmgr_vdev *vdev,
				 struct ml_link_force_state *new)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct set_link_req request, combined, combined_new;
	uint8_t i;

	mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
	if (!mlo_dev_ctx || !mlo_dev_ctx->sta_ctx) {
		mlo_err("mlo_ctx or sta_ctx null");
		return;
	}

	qdf_mem_zero(&combined, sizeof(combined));
	ml_nlink_get_force_link_request(psoc, vdev, &combined, 0);
	for (i = 1; i < SET_LINK_SOURCE_MAX; i++) {
		qdf_mem_zero(&request, sizeof(request));
		ml_nlink_get_force_link_request(psoc, vdev, &request, i);

		if (!ml_nlink_validate_request(psoc, vdev, &combined,
					       i, &request, &combined_new))
			continue;

		qdf_mem_copy(&combined, &combined_new, sizeof(combined_new));
	}

	new->force_active_bitmap = combined.force_active_bitmap;
	new->force_inactive_bitmap = combined.force_inactive_bitmap;
	new->force_active_num = combined.force_active_num;
	new->force_active_num_bitmap =
		combined.force_active_num_bitmap;
	new->force_inactive_num = combined.force_inactive_num;
	new->force_inactive_num_bitmap =
		combined.force_inactive_num_bitmap;

	ml_nlink_dump_force_state(new, "");
}

static QDF_STATUS
ml_nlink_update_no_force_for_all(struct wlan_objmgr_psoc *psoc,
				 struct wlan_objmgr_vdev *vdev,
				 struct ml_link_force_state *curr,
				 struct ml_link_force_state *new,
				 enum mlo_link_force_reason reason,
				 uint32_t link_control_flags)

{
	uint16_t no_force_links;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	/* Special handling for clear all force mode in target.
	 * send MLO_LINK_FORCE_MODE_NO_FORCE to clear "all"
	 * to target
	 */
	if (!new->force_inactive_bitmap &&
	    !new->force_inactive_num &&
	    !new->force_active_bitmap &&
	    !new->force_active_num &&
	    (curr->force_inactive_bitmap ||
	     curr->force_inactive_num ||
	     curr->force_active_bitmap ||
	     curr->force_active_num)) {
		/* If link is force inactive already, but new command will
		 * mark it non-force, need to check conc allow or not.
		 */
		no_force_links = curr->force_inactive_bitmap;
		/* Check non forced links allowed by conc */
		if (!ml_nlink_allow_conc(psoc, vdev, no_force_links, 0)) {
			status = QDF_STATUS_E_INVAL;
			goto end;
		}

		link_control_flags &= ~link_ctrl_f_post_re_evaluate;

		status = policy_mgr_mlo_sta_set_nlink(
						psoc, wlan_vdev_get_id(vdev),
						reason,
						MLO_LINK_FORCE_MODE_NO_FORCE,
						0, 0, 0, link_control_flags);
		if ((link_control_flags & link_ctrl_f_sync_set_link) &&
		    status == QDF_STATUS_E_PENDING)
			status = policy_mgr_wait_for_set_link_update(psoc);
	}

end:
	return status;
}

static QDF_STATUS
ml_nlink_update_force_active_inactive(struct wlan_objmgr_psoc *psoc,
				      struct wlan_objmgr_vdev *vdev,
				      struct ml_link_force_state *curr,
				      struct ml_link_force_state *new,
				      enum mlo_link_force_reason reason,
				      uint32_t link_control_flags,
				      enum ml_nlink_change_event_type evt)
{
	uint16_t no_force_links;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	bool update = false;
	uint16_t new_force_inactive_bitmap = curr->force_inactive_bitmap;
	uint16_t new_force_active_bitmap = curr->force_active_bitmap;

	if (new->force_inactive_bitmap != curr->force_inactive_bitmap) {
		/* If link is force inactive already, but new command will
		 * mark it non-force, need to check conc allow or not.
		 */
		no_force_links = curr->force_inactive_bitmap &
				 new->force_inactive_bitmap;
		no_force_links ^= curr->force_inactive_bitmap;

		/* Check non forced links allowed by conc */
		if (ml_nlink_allow_conc(psoc, vdev, no_force_links,
					 new->force_inactive_bitmap)) {
			update = true;
			new_force_inactive_bitmap = new->force_inactive_bitmap;
		} else {
			status = QDF_STATUS_E_NOSUPPORT;
		}
	}

	if (new->force_active_bitmap != curr->force_active_bitmap) {
		/* Check forced active links allowed by conc */
		if (ml_nlink_allow_conc(psoc, vdev, new->force_active_bitmap,
					 new->force_inactive_bitmap)) {
			update = true;
			new_force_active_bitmap = new->force_active_bitmap;
		} else {
			status = QDF_STATUS_E_NOSUPPORT;
		}
	}

	if (evt == ml_nlink_vendor_cmd_request_evt &&
	    status == QDF_STATUS_SUCCESS)
		update = true;

	if (!update)
		goto end;

	status = policy_mgr_mlo_sta_set_nlink(
			psoc, wlan_vdev_get_id(vdev), reason,
			MLO_LINK_FORCE_MODE_ACTIVE_INACTIVE,
			0,
			new_force_active_bitmap,
			new_force_inactive_bitmap,
			link_control_flags |
			link_ctrl_f_overwrite_active_bitmap |
			link_ctrl_f_overwrite_inactive_bitmap);

	if ((link_control_flags & link_ctrl_f_sync_set_link) &&
	    status == QDF_STATUS_E_PENDING)
		status = policy_mgr_wait_for_set_link_update(psoc);

end:
	return status;
}

static QDF_STATUS
ml_nlink_update_force_inactive_num(struct wlan_objmgr_psoc *psoc,
				   struct wlan_objmgr_vdev *vdev,
				   struct ml_link_force_state *curr,
				   struct ml_link_force_state *new,
				   enum mlo_link_force_reason reason,
				   uint32_t link_control_flags)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (new->force_inactive_num !=
			curr->force_inactive_num ||
	    new->force_inactive_num_bitmap !=
			curr->force_inactive_num_bitmap) {
		status = policy_mgr_mlo_sta_set_nlink(
					psoc, wlan_vdev_get_id(vdev), reason,
					MLO_LINK_FORCE_MODE_INACTIVE_NUM,
					new->force_inactive_num,
					new->force_inactive_num_bitmap,
					0,
					link_control_flags |
					link_ctrl_f_dynamic_force_link_num);

		if ((link_control_flags & link_ctrl_f_sync_set_link) &&
		    status == QDF_STATUS_E_PENDING)
			status = policy_mgr_wait_for_set_link_update(psoc);
	}

	return status;
}

static QDF_STATUS
ml_nlink_update_force_active_num(struct wlan_objmgr_psoc *psoc,
				 struct wlan_objmgr_vdev *vdev,
				 struct ml_link_force_state *curr,
				 struct ml_link_force_state *new,
				 enum mlo_link_force_reason reason,
				 uint32_t link_control_flags)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (new->force_active_num !=
			curr->force_active_num ||
	    new->force_active_num_bitmap !=
			curr->force_active_num_bitmap) {
		status = policy_mgr_mlo_sta_set_nlink(
					psoc, wlan_vdev_get_id(vdev), reason,
					MLO_LINK_FORCE_MODE_ACTIVE_NUM,
					new->force_active_num,
					new->force_active_num_bitmap,
					0,
					link_control_flags |
					link_ctrl_f_dynamic_force_link_num);

		if ((link_control_flags & link_ctrl_f_sync_set_link) &&
		    status == QDF_STATUS_E_PENDING)
			status = policy_mgr_wait_for_set_link_update(psoc);
	}

	return status;
}

static QDF_STATUS
ml_nlink_update_non_force_disallow_bitmap(
				struct wlan_objmgr_psoc *psoc,
				struct wlan_objmgr_vdev *vdev,
				enum ml_nlink_change_event_type evt,
				struct ml_nlink_change_event *data,
				enum mlo_link_force_reason reason,
				uint32_t link_control_flags)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	bool disallow_mode_changed = false;
	uint32_t force_active_bitmap;
	struct ml_link_force_state curr_force_state = {0};

	ml_nlink_get_curr_force_state(psoc, vdev, &curr_force_state);
	force_active_bitmap = curr_force_state.force_active_bitmap;

	ml_nlink_update_disallow_modes(psoc, vdev, false,
				       &disallow_mode_changed,
				       force_active_bitmap);
	link_control_flags |= link_ctrl_f_dont_update_disallow_bitmap;

	if (evt == ml_nlink_nan_pre_enable_evt)
		link_control_flags |= link_ctrl_f_sync_set_link;

	/* In middle of transition, no need schedule force scc workqueue */
	if (evt == ml_nlink_ap_start_evt ||
	    evt == ml_nlink_ap_csa_start_evt ||
	    evt == ml_nlink_ap_start_failed_evt ||
	    evt == ml_nlink_ap_csa_end_evt ||
	    evt == ml_nlink_connect_pre_start_evt ||
	    evt == ml_nlink_connect_failed_evt ||
	    evt == ml_nlink_nan_pre_enable_evt)
		link_control_flags |= link_ctrl_f_dont_reschedule_workqueue;

	/* Send "no force update" to clear any MLMR EMLSR restriction
	 * when become MLO standalone
	 */
	if (disallow_mode_changed) {
		status = policy_mgr_mlo_sta_set_nlink(
					psoc, wlan_vdev_get_id(vdev), reason,
					MLO_LINK_FORCE_MODE_NON_FORCE_UPDATE,
					0,
					0,
					0,
					link_control_flags);
		if ((link_control_flags & link_ctrl_f_sync_set_link) &&
		    status == QDF_STATUS_E_PENDING)
			status = policy_mgr_wait_for_set_link_update(psoc);
	}

	return status;
}

static bool
ml_nlink_all_links_ready_for_state_change(struct wlan_objmgr_psoc *psoc,
					  struct wlan_objmgr_vdev *vdev,
					  enum ml_nlink_change_event_type evt)
{
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0;
	uint8_t ml_vdev_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	qdf_freq_t ml_freq_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	uint8_t ml_linkid_lst[MAX_NUMBER_OF_CONC_CONNECTIONS];
	struct ml_link_info ml_link_info[MAX_NUMBER_OF_CONC_CONNECTIONS];

	if (!mlo_check_if_all_links_up(vdev))
		return false;

	if (mlo_mgr_is_link_switch_in_progress(vdev) &&
	    evt != ml_nlink_connect_completion_evt) {
		mlo_debug("mlo vdev %d link switch in progress!",
			  wlan_vdev_get_id(vdev));
		return false;
	}
	if (policy_mgr_is_set_link_in_progress(psoc)) {
		mlo_debug("mlo vdev %d not ready due to set link in progress",
			  wlan_vdev_get_id(vdev));
		return false;
	}

	/* For initial connecting to 2 or 3 links ML ap, assoc link and
	 * non assoc link connected one by one, avoid changing link state
	 * before link vdev connect completion, to check connected link count.
	 * If < 2, means non assoc link connect is not completed, disallow
	 * link state change.
	 */
	if (!mlo_mgr_is_link_switch_in_progress(vdev) &&
	    evt == ml_nlink_connect_completion_evt) {
		ml_nlink_get_link_info(psoc, vdev, NLINK_EXCLUDE_STANDBY_LINK,
				       QDF_ARRAY_SIZE(ml_linkid_lst),
				       ml_link_info, ml_freq_lst, ml_vdev_lst,
				       ml_linkid_lst, &ml_num_link,
				       &ml_link_bitmap);
		if (ml_num_link < 2)
			return false;
	}

	return true;
}

static QDF_STATUS
ml_nlink_update_force_command_target(struct wlan_objmgr_psoc *psoc,
				     struct wlan_objmgr_vdev *vdev,
				     enum mlo_link_force_reason reason,
				     enum ml_nlink_change_event_type evt,
				     struct ml_nlink_change_event *data,
				     struct ml_link_force_state *curr,
				     struct ml_link_force_state *new)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	uint32_t link_control_flags = 0;

	if (evt == ml_nlink_post_set_link_evt && data)
		SET_POST_RE_EVALUATE_LOOPS(link_control_flags,
					   data->evt.post_set_link.post_re_evaluate_loops);

	if (evt == ml_nlink_ap_start_evt ||
	    evt == ml_nlink_ap_csa_start_evt ||
	    evt == ml_nlink_nan_pre_enable_evt ||
	    evt == ml_nlink_connect_pre_start_evt)
		link_control_flags |= link_ctrl_f_sync_set_link;
	else
		link_control_flags |= link_ctrl_f_post_re_evaluate;

	/* In middle of transition, no need schedule force scc workqueue */
	if (evt == ml_nlink_ap_start_evt ||
	    evt == ml_nlink_ap_csa_start_evt ||
	    evt == ml_nlink_ap_start_failed_evt ||
	    evt == ml_nlink_ap_csa_end_evt ||
	    evt == ml_nlink_connect_pre_start_evt ||
	    evt == ml_nlink_connect_failed_evt ||
	    evt == ml_nlink_nan_pre_enable_evt)
		link_control_flags |= link_ctrl_f_dont_reschedule_workqueue;

	status = ml_nlink_update_no_force_for_all(psoc, vdev,
						  curr,
						  new,
						  reason,
						  link_control_flags);
	if (status == QDF_STATUS_E_PENDING || status != QDF_STATUS_SUCCESS)
		goto end;

	status =
	ml_nlink_update_force_active_inactive(psoc, vdev,
					      curr,
					      new,
					      reason,
					      link_control_flags,
					      evt);
	if (status == QDF_STATUS_E_PENDING ||
	    (status != QDF_STATUS_SUCCESS && status != QDF_STATUS_E_NOSUPPORT))
		goto end;

	status = ml_nlink_update_force_inactive_num(psoc, vdev,
						    curr,
						    new,
						    reason,
						    link_control_flags);

	if (status == QDF_STATUS_E_PENDING || status != QDF_STATUS_SUCCESS)
		goto end;

	status = ml_nlink_update_force_active_num(psoc, vdev,
						  curr,
						  new,
						  reason,
						  link_control_flags);
end:
	return status;
}

/**
 * ml_nlink_state_change_emlsr() - Handle ML STA link force
 * with concurrency internal function (HW EMLSR conc supported)
 * @psoc: PSOC object information
 * @reason: reason code of trigger force mode change.
 * @evt: event type
 * @data: event data
 *
 * This API handle link force for connected ML STA associated as MLMR and
 * eMLSR.
 * At present we only support one ML STA. so ml_nlink_get_affect_ml_sta
 * is invoked to get one ML STA vdev from policy mgr table.
 *
 * The flow is to get current force command which has been sent to target
 * and compute a new force command based on current connection table.
 * If any difference between "current" and "new", driver sends update
 * command to target. Driver will update the current force command
 * record after get successful respone from target.
 *
 * Return: QDF_STATUS_SUCCESS if no new command updated to target.
 *	   QDF_STATUS_E_PENDING if new command is sent to target.
 *	   otherwise QDF_STATUS error code
 */
static QDF_STATUS
ml_nlink_state_change_emlsr(struct wlan_objmgr_psoc *psoc,
			    enum mlo_link_force_reason reason,
			    enum ml_nlink_change_event_type evt,
			    struct ml_nlink_change_event *data)
{
	struct ml_link_force_state force_state = {0};
	struct ml_link_force_state legacy_intf_force_state = {0};
	struct ml_link_force_state curr_force_state = {0};
	struct wlan_objmgr_vdev *vdev = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	vdev = ml_nlink_get_affect_ml_sta(psoc);
	if (!vdev)
		goto end;
	if (!ml_nlink_all_links_ready_for_state_change(psoc, vdev, evt))
		goto end;

	ml_nlink_get_curr_force_state(psoc, vdev, &curr_force_state);

	ml_nlink_handle_removal_links(psoc, vdev, &force_state);

	ml_nlink_handle_legacy_intf_emlsr(psoc, vdev,
					  &legacy_intf_force_state);

	force_state.force_inactive_bitmap |=
		legacy_intf_force_state.force_inactive_bitmap;

	if (legacy_intf_force_state.force_inactive_num &&
	    legacy_intf_force_state.force_inactive_num >=
			force_state.force_inactive_num) {
		force_state.force_inactive_num =
			legacy_intf_force_state.force_inactive_num;
		force_state.force_inactive_num_bitmap =
		legacy_intf_force_state.force_inactive_num_bitmap;
	}

	ml_nlink_handle_dynamic_inactive(psoc, vdev, &curr_force_state,
					 &force_state);


	ml_nlink_update_concurrency_link_request(psoc, vdev,
						 &force_state,
						 reason);
	ml_nlink_handle_all_link_request(psoc, vdev, &force_state);

	status =
	ml_nlink_update_force_command_target(psoc, vdev, reason, evt,
					     data, &curr_force_state,
					     &force_state);

	if (status == QDF_STATUS_E_PENDING || status != QDF_STATUS_SUCCESS)
		goto end;

	status = ml_nlink_update_non_force_disallow_bitmap(psoc, vdev,
							   evt,
							   data,
							   reason,
							   0);
end:
	if (vdev) {
		wlan_objmgr_vdev_release_ref(vdev, WLAN_MLO_MGR_ID);

		if (status == QDF_STATUS_SUCCESS)
			mlo_debug("exit no force state change");
		else if (status == QDF_STATUS_E_PENDING)
			mlo_debug("exit pending force state change");
		else
			mlo_err("exit err %d state change", status);
	}

	return status;
}

/**
 * ml_nlink_state_change_emlsr_no_conc() - Handle eMLSR STA link force
 * with vendor command request (HW no EMSR conc supported)
 * @psoc: PSOC object information
 * @reason: reason code of trigger force mode change.
 * @evt: event type
 * @data: event data
 *
 * This API will handle vendor command set link request for standalone
 * eMLSR STA for HMT. The target doesn't support concurrency in eMLSR
 * mode.
 * Since we don't support eMLSR STA+Legacy Interfaces, the eMLSR sta
 * has been changed to MLSR by wlan_handle_emlsr_sta_concurrency if any
 * concurrent interface up. So this API will do no-op for any vendor
 * command request if eMLSR concurrency present.
 *
 * Return: QDF_STATUS_SUCCESS if no new command updated to target.
 *	   QDF_STATUS_E_PENDING if new command is sent to target.
 *	   otherwise QDF_STATUS error code
 */
static QDF_STATUS
ml_nlink_state_change_emlsr_no_conc(struct wlan_objmgr_psoc *psoc,
				    enum mlo_link_force_reason reason,
				    enum ml_nlink_change_event_type evt,
				    struct ml_nlink_change_event *data)
{
	struct ml_link_force_state force_state = {0};
	struct ml_link_force_state curr_force_state = {0};
	struct wlan_objmgr_vdev *vdev = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	switch (evt) {
	case ml_nlink_vendor_cmd_request_evt:
	case ml_nlink_post_set_link_evt:
		break;
	default:
		mlo_debug("Don't disable eMLSR links");
		return QDF_STATUS_SUCCESS;
	}
	if (policy_mgr_is_emlsr_sta_concurrency_present(psoc)) {
		mlo_debug("eMLSR concurrency not allow to set link");
		return QDF_STATUS_E_INVAL;
	}

	vdev = ml_nlink_get_affect_ml_sta(psoc);
	if (!vdev)
		goto end;
	if (!ml_nlink_all_links_ready_for_state_change(psoc, vdev, evt))
		goto end;

	ml_nlink_get_curr_force_state(psoc, vdev, &curr_force_state);

	ml_nlink_handle_all_link_request(psoc, vdev, &force_state);

	status =
	ml_nlink_update_force_command_target(psoc, vdev, reason, evt,
					     data, &curr_force_state,
					     &force_state);
end:
	if (vdev) {
		wlan_objmgr_vdev_release_ref(vdev, WLAN_MLO_MGR_ID);

		if (status == QDF_STATUS_SUCCESS)
			mlo_debug("exit no force state change");
		else if (status == QDF_STATUS_E_PENDING)
			mlo_debug("exit pending force state change");
		else
			mlo_err("exit err %d state change", status);
	}

	return status;
}

/**
 * ml_nlink_state_change_mlmr() - Handle MLMR STA link force
 * with concurrency internal function (no EMLSR Conc hw support)
 * @psoc: PSOC object information
 * @reason: reason code of trigger force mode change.
 * @evt: event type
 * @data: event data
 *
 * This API handle link force for connected ML STA associated as MLMR only
 * (not EMLSR Conc hw support).
 * At present we only support one ML STA. so ml_nlink_get_affect_ml_sta
 * is invoked to get one ML STA vdev from policy mgr table.
 *
 * The flow is to get current force command which has been sent to target
 * and compute a new force command based on current connection table.
 * If any difference between "current" and "new", driver sends update
 * command to target. Driver will update the current force command
 * record after get successful respone from target.
 *
 * Return: QDF_STATUS_SUCCESS if no new command updated to target.
 *	   QDF_STATUS_E_PENDING if new command is sent to target.
 *	   otherwise QDF_STATUS error code
 */
static QDF_STATUS
ml_nlink_state_change_mlmr(struct wlan_objmgr_psoc *psoc,
			   enum mlo_link_force_reason reason,
			   enum ml_nlink_change_event_type evt,
			   struct ml_nlink_change_event *data)
{
	struct ml_link_force_state force_state = {0};
	struct ml_link_force_state legacy_intf_force_state = {0};
	struct ml_link_force_state curr_force_state = {0};
	struct wlan_objmgr_vdev *vdev = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	vdev = ml_nlink_get_affect_ml_sta(psoc);
	if (!vdev)
		goto end;
	if (!ml_nlink_all_links_ready_for_state_change(psoc, vdev, evt))
		goto end;

	ml_nlink_get_curr_force_state(psoc, vdev, &curr_force_state);

	ml_nlink_handle_mcc_links(psoc, vdev, &force_state);

	ml_nlink_handle_legacy_intf(psoc, vdev, &legacy_intf_force_state);

	force_state.force_inactive_bitmap |=
		legacy_intf_force_state.force_inactive_bitmap;

	if (legacy_intf_force_state.force_inactive_num &&
	    legacy_intf_force_state.force_inactive_num >=
			force_state.force_inactive_num) {
		force_state.force_inactive_num =
			legacy_intf_force_state.force_inactive_num;
		force_state.force_inactive_num_bitmap =
		legacy_intf_force_state.force_inactive_num_bitmap;
	}

	ml_nlink_handle_dynamic_inactive(psoc, vdev, &curr_force_state,
					 &force_state);

	ml_nlink_update_concurrency_link_request(psoc, vdev,
						 &force_state,
						 reason);
	ml_nlink_handle_all_link_request(psoc, vdev, &force_state);

	status =
	ml_nlink_update_force_command_target(psoc, vdev, reason, evt,
					     data, &curr_force_state,
					     &force_state);
end:
	if (vdev) {
		wlan_objmgr_vdev_release_ref(vdev, WLAN_MLO_MGR_ID);

		if (status == QDF_STATUS_SUCCESS)
			mlo_debug("exit no force state change");
		else if (status == QDF_STATUS_E_PENDING)
			mlo_debug("exit pending force state change");
		else
			mlo_err("exit err %d state change", status);
	}

	return status;
}

/**
 * ml_nlink_state_change() - Handle ML STA link force
 * with concurrency internal function
 * @psoc: PSOC object information
 * @reason: reason code of trigger force mode change.
 * @evt: event type
 * @data: event data
 *
 * Return: QDF_STATUS_SUCCESS if no new command updated to target.
 *	   QDF_STATUS_E_PENDING if new command is sent to target.
 *	   otherwise QDF_STATUS error code
 */
static QDF_STATUS ml_nlink_state_change(struct wlan_objmgr_psoc *psoc,
					enum mlo_link_force_reason reason,
					enum ml_nlink_change_event_type evt,
					struct ml_nlink_change_event *data)
{
	QDF_STATUS status;

	if (policy_mgr_is_mlo_in_mode_emlsr(psoc, NULL, NULL)) {
		if (wlan_mlme_is_aux_emlsr_support(psoc))
			status = ml_nlink_state_change_emlsr(
					psoc, reason, evt, data);
		else
			status = ml_nlink_state_change_emlsr_no_conc(
					psoc, reason, evt, data);
	} else {
		status = ml_nlink_state_change_mlmr(psoc, reason,
						    evt, data);
	}

	return status;
}

/**
 * ml_nlink_state_change_handler() - Handle ML STA link force
 * with concurrency
 * @psoc: PSOC object information
 * @vdev: ml sta vdev object
 * @reason: reason code of trigger force mode change.
 * @evt: event type
 * @data: event data
 *
 * Return: QDF_STATUS_SUCCESS if successfully
 */
static QDF_STATUS
ml_nlink_state_change_handler(struct wlan_objmgr_psoc *psoc,
			      struct wlan_objmgr_vdev *vdev,
			      enum mlo_link_force_reason reason,
			      enum ml_nlink_change_event_type evt,
			      struct ml_nlink_change_event *data)
{
	enum QDF_OPMODE mode = wlan_vdev_mlme_get_opmode(vdev);
	uint8_t vdev_id = wlan_vdev_get_id(vdev);
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	/* If WMI_SERVICE_N_LINK_MLO_SUPPORT = 381 is enabled,
	 * indicate FW support N MLO link & vdev re-purpose between links,
	 * host will use linkid bitmap to force inactive/active links
	 * by API ml_nlink_state_change.
	 * Otherwise, use legacy policy mgr API to inactive/active based
	 * on vdev id bitmap.
	 */
	if (ml_is_nlink_service_supported(psoc))
		status = ml_nlink_state_change(psoc, reason, evt, data);
	else if (reason == MLO_LINK_FORCE_REASON_CONNECT)
		policy_mgr_handle_ml_sta_links_on_vdev_up_csa(psoc, mode,
							      vdev_id);
	else
		policy_mgr_handle_ml_sta_links_on_vdev_down(psoc, mode,
							    vdev_id);

	return status;
}

static QDF_STATUS
ml_nlink_emlsr_downgrade_handler(struct wlan_objmgr_psoc *psoc,
				 struct wlan_objmgr_vdev *vdev,
				 enum ml_nlink_change_event_type evt,
				 struct ml_nlink_change_event *data)
{
	struct set_link_req req;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_vdev *ml_vdev = NULL;
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0;
	uint8_t ml_vdev_lst[WLAN_MAX_ML_BSS_LINKS];
	qdf_freq_t ml_freq_lst[WLAN_MAX_ML_BSS_LINKS];
	uint8_t ml_linkid_lst[WLAN_MAX_ML_BSS_LINKS];
	struct ml_link_info ml_link_info[WLAN_MAX_ML_BSS_LINKS];
	uint8_t i, force_inactive_num = 0;
	uint32_t force_inactive_num_bitmap = 0;
	uint32_t force_inactive_bitmap = 0;
	enum QDF_OPMODE op_mode;
	uint32_t old_emlsr_disable_req;
	enum ml_emlsr_disable_request disable_request = 0;
	uint32_t tgt_ch_freq = 0;
	bool csa_to_scc = false;

	if (mlo_is_mld_sta(vdev))
		return QDF_STATUS_SUCCESS;

	if (!wlan_mlme_is_aux_emlsr_support(psoc) ||
	    !policy_mgr_is_mlo_in_mode_emlsr(psoc, NULL, NULL))
		return QDF_STATUS_SUCCESS;

	op_mode = wlan_vdev_mlme_get_opmode(vdev);
	if (!(op_mode == QDF_SAP_MODE ||
	      op_mode == QDF_P2P_GO_MODE ||
	      op_mode == QDF_STA_MODE ||
	      op_mode == QDF_P2P_CLIENT_MODE ||
	      op_mode == QDF_NAN_DISC_MODE)) {
		mlo_debug("op_mode: %d", op_mode);
		goto end;
	}

	ml_vdev = ml_nlink_get_affect_ml_sta(psoc);
	if (!ml_vdev)
		goto end;

	ml_nlink_get_link_info(psoc, ml_vdev, NLINK_EXCLUDE_REMOVED_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);
	if (ml_num_link < 2)
		goto end;

	if (evt == ml_nlink_ap_csa_start_evt)
		tgt_ch_freq = data->evt.csa_start.tgt_ch_freq;

	for (i = 0; i < ml_num_link; i++) {
		/* force inactive num = 2 amongst all links for 3-link ML STA */
		force_inactive_num_bitmap |= 1 << ml_linkid_lst[i];
		force_inactive_num++;

		if ((ml_num_link > 2) || (ml_vdev_lst[i] ==
		    WLAN_INVALID_VDEV_ID) ||
		    policy_mgr_vdev_is_force_inactive(psoc, ml_vdev_lst[i]))
			continue;

		if (!WLAN_REG_IS_24GHZ_CH_FREQ(ml_freq_lst[i])) {
			/* ML STA 5GH+5GH, if SAP CSA to SCC channel of ML STA,
			 * downgrade EMLSR to MLSR by force inactive the other
			 * MCC link.
			 */
			if (tgt_ch_freq &&
			    policy_mgr_2_freq_always_on_same_mac(
				psoc, ml_freq_lst[i], tgt_ch_freq)) {
				if (ml_freq_lst[i] == tgt_ch_freq)
					csa_to_scc = true;
				else
					force_inactive_bitmap |=
					1 << ml_linkid_lst[i];
			}
		}
	}
	/* small optimization, if 5G links num is less then 2, eMLSR is
	 * not available, no need to send command.
	 */
	if (force_inactive_num < 2)
		goto end;

	/* eMLSR downgrade to single link for below event */
	if (evt == ml_nlink_ap_csa_start_evt) {
		if (!data->evt.csa_start.wait_set_link) {
			status = QDF_STATUS_E_PENDING;
			goto end;
		}
		disable_request = ML_EMLSR_DOWNGRADE_BY_AP_CSA;
		if (!csa_to_scc)
			force_inactive_bitmap = 0;
	} else if (evt == ml_nlink_ap_start_evt) {
		disable_request = ML_EMLSR_DOWNGRADE_BY_AP_START;
	} else if (evt == ml_nlink_connect_pre_start_evt) {
		disable_request = ML_EMLSR_DOWNGRADE_BY_STA_START;
	} else {
		mlo_debug("not handled for evt %d", evt);
		goto end;
	}

	/* Check if any set link is already in progress and thus wait */
	policy_mgr_wait_for_set_link_update(psoc);

	old_emlsr_disable_req =
	ml_nlink_set_emlsr_mode_disable_req(psoc, ml_vdev, disable_request);

	qdf_mem_zero(&req, sizeof(req));
	ml_nlink_get_force_link_request(psoc, ml_vdev, &req,
					SET_LINK_FROM_EMLSR_DOWNGRADE);
	if (req.force_inactive_num_bitmap ||
	    req.force_inactive_bitmap) {
		mlo_debug("eMLSR downgrade req is present");
		goto end;
	}

	qdf_mem_zero(&req, sizeof(req));
	if (force_inactive_bitmap) {
		req.mode = MLO_LINK_FORCE_MODE_INACTIVE;
		req.reason = MLO_LINK_FORCE_REASON_CONNECT;
		req.force_inactive_bitmap = force_inactive_bitmap;
	} else {
		force_inactive_num--;
		req.mode = MLO_LINK_FORCE_MODE_INACTIVE_NUM;
		req.reason = MLO_LINK_FORCE_REASON_CONNECT;
		req.force_inactive_num = force_inactive_num;
		req.force_inactive_num_bitmap = force_inactive_num_bitmap;
	}
	ml_nlink_update_force_link_request(psoc, ml_vdev, &req,
					   SET_LINK_FROM_EMLSR_DOWNGRADE);

	status = ml_nlink_state_change(psoc, MLO_LINK_FORCE_REASON_CONNECT,
				       evt, data);

	if (status == QDF_STATUS_E_PENDING)
		status = QDF_STATUS_SUCCESS;
end:
	if (ml_vdev)
		wlan_objmgr_vdev_release_ref(ml_vdev, WLAN_MLO_MGR_ID);

	return status;
}

static QDF_STATUS
ml_nlink_undo_emlsr_downgrade_handler(struct wlan_objmgr_psoc *psoc,
				      struct wlan_objmgr_vdev *vdev,
				      enum ml_nlink_change_event_type evt,
				      struct ml_nlink_change_event *data)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_vdev *ml_vdev = NULL;
	enum QDF_OPMODE op_mode;
	uint32_t old_emlsr_disable_req;
	enum ml_emlsr_disable_request request = 0;

	if (mlo_is_mld_sta(vdev))
		return QDF_STATUS_SUCCESS;

	if (!wlan_mlme_is_aux_emlsr_support(psoc) ||
	    !policy_mgr_is_mlo_in_mode_emlsr(psoc, NULL, NULL))
		return QDF_STATUS_SUCCESS;

	op_mode = wlan_vdev_mlme_get_opmode(vdev);
	if (!(op_mode == QDF_SAP_MODE ||
	      op_mode == QDF_P2P_GO_MODE ||
	      op_mode == QDF_STA_MODE ||
	      op_mode == QDF_P2P_CLIENT_MODE ||
	      op_mode == QDF_NAN_DISC_MODE)) {
		mlo_debug("op_mode: %d", op_mode);
		goto end;
	}

	ml_vdev = ml_nlink_get_affect_ml_sta(psoc);
	if (!ml_vdev)
		goto end;

	/* eMLSR link downgrade to single link */
	if (evt == ml_nlink_ap_started_evt ||
	    evt == ml_nlink_ap_start_failed_evt) {
		request = ML_EMLSR_DOWNGRADE_BY_AP_START;
	} else if (evt == ml_nlink_connect_completion_evt ||
		   evt == ml_nlink_connect_failed_evt) {
		request = ML_EMLSR_DOWNGRADE_BY_STA_START;
	} else {
		mlo_debug("not handled for evt %d", evt);
		goto end;
	}

	old_emlsr_disable_req =
	ml_nlink_clr_emlsr_mode_disable_req(psoc, ml_vdev, request);
	if (old_emlsr_disable_req & request)
		ml_nlink_set_emlsr_mode_disable_req(
					psoc, ml_vdev,
					ML_EMLSR_DOWNGRADE_BY_OPP_TIMER);

end:
	if (ml_vdev)
		wlan_objmgr_vdev_release_ref(ml_vdev, WLAN_MLO_MGR_ID);

	return status;
}

static QDF_STATUS
ml_nlink_tdls_event_handler(struct wlan_objmgr_psoc *psoc,
			    struct wlan_objmgr_vdev *vdev,
			    enum ml_nlink_change_event_type evt,
			    struct ml_nlink_change_event *data)
{
	struct ml_link_force_state curr_force_state = {0};
	QDF_STATUS status;
	struct set_link_req vendor_req = {0};

	ml_nlink_get_curr_force_state(psoc, vdev, &curr_force_state);

	switch (data->evt.tdls.mode) {
	case MLO_LINK_FORCE_MODE_ACTIVE:
		if ((data->evt.tdls.link_bitmap &
		    curr_force_state.force_active_bitmap) ==
		    data->evt.tdls.link_bitmap) {
			mlo_debug("link_bitmap 0x%x already active, 0x%x",
				  data->evt.tdls.link_bitmap,
				  curr_force_state.force_active_bitmap);
			return QDF_STATUS_SUCCESS;
		}
		if (data->evt.tdls.link_bitmap &
		    (curr_force_state.force_inactive_bitmap |
		     curr_force_state.curr_dynamic_inactive_bitmap)) {
			mlo_debug("link_bitmap 0x%x can't be active due to concurrency, 0x%x 0x%x",
				  data->evt.tdls.link_bitmap,
				  curr_force_state.force_inactive_bitmap,
				  curr_force_state.
				  curr_dynamic_inactive_bitmap);
			return QDF_STATUS_E_INVAL;
		}
		break;
	case MLO_LINK_FORCE_MODE_NO_FORCE:
		if (data->evt.tdls.link_bitmap &
		    curr_force_state.force_inactive_bitmap) {
			mlo_debug("link_bitmap 0x%x can't be no_force due to concurrency, 0x%x",
				  data->evt.tdls.link_bitmap,
				  curr_force_state.force_inactive_bitmap);
			return QDF_STATUS_E_INVAL;
		}
		if (!(data->evt.tdls.link_bitmap &
			curr_force_state.force_active_bitmap)) {
			mlo_debug("link_bitmap 0x%x already no force, 0x%x",
				  data->evt.tdls.link_bitmap,
				  curr_force_state.force_active_bitmap);
			return QDF_STATUS_SUCCESS;
		}
		ml_nlink_get_force_link_request(psoc, vdev, &vendor_req,
						SET_LINK_FROM_VENDOR_CMD);
		if (data->evt.tdls.link_bitmap &
		    vendor_req.force_active_bitmap) {
			mlo_debug("link_bitmap 0x%x active hold by vendor cmd, 0x%x",
				  data->evt.tdls.link_bitmap,
				  vendor_req.force_active_bitmap);
			return QDF_STATUS_SUCCESS;
		}
		break;
	default:
		mlo_err("unhandled for tdls force mode %d",
			data->evt.tdls.mode);
		return QDF_STATUS_E_INVAL;
	}

	if (ml_is_nlink_service_supported(psoc))
		status = policy_mgr_mlo_sta_set_nlink(
				psoc, wlan_vdev_get_id(vdev),
				data->evt.tdls.reason,
				data->evt.tdls.mode,
				0,
				data->evt.tdls.link_bitmap,
				0,
				0);
	else
		status =
		policy_mgr_mlo_sta_set_link(psoc,
					    data->evt.tdls.reason,
					    data->evt.tdls.mode,
					    data->evt.tdls.vdev_count,
					    data->evt.tdls.mlo_vdev_lst);

	return status;
}

static QDF_STATUS
ml_nlink_vendor_cmd_handler(struct wlan_objmgr_psoc *psoc,
			    struct wlan_objmgr_vdev *vdev,
			    enum ml_nlink_change_event_type evt,
			    struct ml_nlink_change_event *data)
{
	struct set_link_req req = {0};
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	mlo_debug("link ctrl %d mode %d reason %d num %d 0x%x 0x%x",
		  data->evt.vendor.link_ctrl_mode,
		  data->evt.vendor.mode,
		  data->evt.vendor.reason,
		  data->evt.vendor.link_num,
		  data->evt.vendor.link_bitmap,
		  data->evt.vendor.link_bitmap2);

	switch (data->evt.vendor.link_ctrl_mode) {
	case LINK_CONTROL_MODE_DEFAULT:
		ml_nlink_clr_force_link_request(psoc, vdev,
						SET_LINK_FROM_VENDOR_CMD);
		break;
	case LINK_CONTROL_MODE_USER:
		req.mode = data->evt.vendor.mode;
		req.reason = data->evt.vendor.reason;
		req.force_active_bitmap = data->evt.vendor.link_bitmap;
		req.force_inactive_bitmap = data->evt.vendor.link_bitmap2;
		if (!ml_nlink_validate_link_request(psoc, vdev,
						    SET_LINK_FROM_VENDOR_CMD,
						    &req)) {
			mlo_debug("not supported");
			return QDF_STATUS_E_INVAL;
		}
		ml_nlink_update_force_link_request(psoc, vdev, &req,
						   SET_LINK_FROM_VENDOR_CMD);
		break;
	case LINK_CONTROL_MODE_MIXED:
		req.mode = data->evt.vendor.mode;
		req.reason = data->evt.vendor.reason;
		req.force_active_num = data->evt.vendor.link_num;
		req.force_active_num_bitmap = data->evt.vendor.link_bitmap;
		if (!ml_nlink_validate_link_request(psoc, vdev,
						    SET_LINK_FROM_VENDOR_CMD,
						    &req)) {
			mlo_debug("not supported");
			return QDF_STATUS_E_INVAL;
		}
		ml_nlink_update_force_link_request(psoc, vdev, &req,
						   SET_LINK_FROM_VENDOR_CMD);
		break;
	default:
		status = QDF_STATUS_E_INVAL;
		return status;
	}

	status = ml_nlink_state_change(psoc, MLO_LINK_FORCE_REASON_CONNECT,
				       evt, data);

	if (status == QDF_STATUS_E_PENDING)
		status = QDF_STATUS_SUCCESS;
	else if (status != QDF_STATUS_SUCCESS)
		ml_nlink_clr_force_link_request(psoc, vdev,
						SET_LINK_FROM_VENDOR_CMD);

	return status;
}

static QDF_STATUS
ml_nlink_swtich_dynamic_inactive_link(struct wlan_objmgr_psoc *psoc,
				      struct wlan_objmgr_vdev *vdev)
{
	uint8_t link_id;
	uint32_t standby_link_bitmap, dynamic_inactive_bitmap;
	struct ml_link_force_state curr_force_state = {0};
	uint8_t link_ids[MAX_MLO_LINK_ID];
	uint8_t num_ids;

	link_id = wlan_vdev_get_link_id(vdev);
	if (link_id >= MAX_MLO_LINK_ID) {
		mlo_err("invalid link id %d", link_id);
		return QDF_STATUS_E_INVAL;
	}

	ml_nlink_get_curr_force_state(psoc, vdev, &curr_force_state);
	standby_link_bitmap = ml_nlink_get_standby_link_bitmap(psoc, vdev);
	standby_link_bitmap &= curr_force_state.force_inactive_num_bitmap &
				~(1 << link_id);
	/* In DBS RD, ML STA 2+5+6(standby link), force inactive num = 1 and
	 * force inactive bitmap with 5 + 6 links will be sent to FW, host
	 * will select 6G as dynamic inactive link, 5G vdev will be kept in
	 * policy mgr active connection table.
	 * If FW link switch and repurpose 5G vdev to 6G, host will need to
	 * select 5G standby link as dynamic inactive.
	 * Then 6G vdev can be moved to policy mgr active connection table.
	 */
	if (((1 << link_id) & curr_force_state.curr_dynamic_inactive_bitmap) &&
	    ((1 << link_id) & curr_force_state.force_inactive_num_bitmap) &&
	    !(standby_link_bitmap &
			curr_force_state.curr_dynamic_inactive_bitmap) &&
	    (standby_link_bitmap &
			curr_force_state.force_inactive_num_bitmap)) {
		num_ids = convert_link_bitmap_to_link_ids(
						standby_link_bitmap,
						QDF_ARRAY_SIZE(link_ids),
						link_ids);
		if (!num_ids) {
			mlo_err("unexpected 0 link ids for bitmap 0x%x",
				standby_link_bitmap);
			return QDF_STATUS_E_INVAL;
		}
		/* Remove the link from dynamic inactive bitmap,
		 * add the standby link to dynamic inactive bitmap.
		 */
		dynamic_inactive_bitmap =
			curr_force_state.curr_dynamic_inactive_bitmap &
						~(1 << link_id);
		dynamic_inactive_bitmap |= 1 << link_ids[0];
		mlo_debug("move out vdev %d link id %d from dynamic inactive, add standby link id %d",
			  wlan_vdev_get_id(vdev), link_id, link_ids[0]);
		ml_nlink_set_dynamic_inactive_links(psoc, vdev,
						    dynamic_inactive_bitmap);
	}

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
ml_nlink_ap_csa_start_handler(struct wlan_objmgr_psoc *psoc,
			      struct wlan_objmgr_vdev *vdev,
			      enum ml_nlink_change_event_type evt,
			      struct ml_nlink_change_event *data)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_vdev *ml_vdev = NULL;
	uint32_t old_emlsr_disable_req;
	uint8_t ml_num_link = 0;
	uint32_t ml_link_bitmap = 0;
	uint8_t ml_vdev_lst[WLAN_MAX_ML_BSS_LINKS];
	qdf_freq_t ml_freq_lst[WLAN_MAX_ML_BSS_LINKS];
	uint8_t ml_linkid_lst[WLAN_MAX_ML_BSS_LINKS];
	struct ml_link_info ml_link_info[WLAN_MAX_ML_BSS_LINKS];
	qdf_freq_t ml_5g_freq_lst[WLAN_MAX_ML_BSS_LINKS];
	uint8_t i, force_inactive_num = 0;
	uint32_t force_inactive_num_bitmap = 0;
	bool csa_to_scc = false;
	uint32_t tgt_ch_freq = data->evt.csa_start.tgt_ch_freq;
	uint32_t link_5g_bitmap = 0;
	uint32_t link_5g_num = 0;
	bool use_non_force_update_cmd = true;

	mlo_debug("csa from %d to %d wait_set_link %d",
		  data->evt.csa_start.curr_ch_freq,
		  data->evt.csa_start.tgt_ch_freq,
		  data->evt.csa_start.wait_set_link);

	if (!policy_mgr_is_mlo_in_mode_emlsr(psoc, NULL, NULL) ||
	    !wlan_mlme_is_aux_emlsr_support(psoc))
		return QDF_STATUS_SUCCESS;

	ml_vdev = ml_nlink_get_affect_ml_sta(psoc);
	if (!ml_vdev)
		goto end;

	ml_nlink_get_link_info(psoc, ml_vdev, NLINK_EXCLUDE_REMOVED_LINK,
			       QDF_ARRAY_SIZE(ml_linkid_lst),
			       ml_link_info, ml_freq_lst, ml_vdev_lst,
			       ml_linkid_lst, &ml_num_link,
			       &ml_link_bitmap);
	if (ml_num_link < 2)
		goto end;

	for (i = 0; i < ml_num_link; i++) {
		if (ml_vdev_lst[i] == WLAN_INVALID_VDEV_ID)
			continue;
		if (policy_mgr_vdev_is_force_inactive(psoc, ml_vdev_lst[i]))
			continue;
		if (!WLAN_REG_IS_24GHZ_CH_FREQ(ml_freq_lst[i])) {
			if (policy_mgr_2_freq_always_on_same_mac(
				psoc, ml_freq_lst[i], tgt_ch_freq)) {
				force_inactive_num_bitmap |=
					1 << ml_linkid_lst[i];
				force_inactive_num++;
				if (ml_freq_lst[i] == tgt_ch_freq)
					csa_to_scc = true;
			}
			link_5g_bitmap |= 1 << ml_linkid_lst[i];
			ml_5g_freq_lst[link_5g_num] = ml_freq_lst[i];
			link_5g_num++;
		}
	}
	/* 5G link num < 2, emlsr is not available, no need to disallow it */
	if (link_5g_num < 2)
		goto end;

	/* SBS RD ML STA 5GL+5GL, 5GH+5GH, or DBS RD ML STA 5G+5G/6G,
	 * and SAP CSA to 5G same subband, we need to force inactive num 1 to
	 * disable EMLSR (EMLSR downgrade to MLSR).
	 * And in such case, if CSA to MCC channel of both ML STA links,
	 * reject it, otherwise that will cause MCC 3 home channel in same
	 * subband.
	 *
	 * For other cases, to only update mlo disallow mode bitmap to
	 * disable EMLSR.
	 */
	if (force_inactive_num > 1) {
		if (!csa_to_scc) {
			mlo_debug("Reject AP CSA to MCC chan of ML STA in same subband");
			status = QDF_STATUS_E_INVAL;
			goto end;
		}
		use_non_force_update_cmd = false;
	}

	/* eMLSR disable by downgrade to single link */
	if (!use_non_force_update_cmd) {
		status = ml_nlink_emlsr_downgrade_handler(psoc,
							  vdev,
							  evt,
							  data);
		goto end;
	}

	old_emlsr_disable_req =
	ml_nlink_get_emlsr_mode_disable_req(psoc, ml_vdev);
	if (old_emlsr_disable_req) {
		mlo_debug("emlsr is disabled already");
		goto end;
	}

	if (ml_nlink_is_support_emlsr_with_freq(psoc, ml_5g_freq_lst,
						tgt_ch_freq)) {
		mlo_debug("emlsr is allowed after CSA to %d, no need to disable",
			  tgt_ch_freq);
		goto end;
	}

	if (!data->evt.csa_start.wait_set_link) {
		status = QDF_STATUS_E_PENDING;
		goto end;
	}

	/* eMLSR disable by disallow_bitmap update */
	old_emlsr_disable_req =
	ml_nlink_set_emlsr_mode_disable_req(
			psoc, ml_vdev, ML_EMLSR_DISALLOW_BY_AP_CSA);
	if (old_emlsr_disable_req) {
		mlo_debug("emlsr is disabled already");
		goto end;
	}

	status = ml_nlink_update_non_force_disallow_bitmap(
				psoc, ml_vdev, evt, data,
				MLO_LINK_FORCE_REASON_CONNECT,
				link_ctrl_f_sync_set_link);
	if (status == QDF_STATUS_E_PENDING)
		status = QDF_STATUS_SUCCESS;

end:
	if (ml_vdev)
		wlan_objmgr_vdev_release_ref(ml_vdev, WLAN_MLO_MGR_ID);

	return status;
}

static QDF_STATUS
ml_nlink_ap_csa_end_handler(struct wlan_objmgr_psoc *psoc,
			    struct wlan_objmgr_vdev *vdev,
			    enum ml_nlink_change_event_type evt,
			    struct ml_nlink_change_event *data)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_vdev *ml_vdev = NULL;
	uint32_t old_emlsr_disable_req;

	mlo_debug("vdev %d csa result %d ", wlan_vdev_get_id(vdev),
		  data->evt.csa_end.csa_failed);

	if (!policy_mgr_is_mlo_in_mode_emlsr(psoc, NULL, NULL) ||
	    !wlan_mlme_is_aux_emlsr_support(psoc))
		return QDF_STATUS_SUCCESS;

	ml_vdev = ml_nlink_get_affect_ml_sta(psoc);
	if (!ml_vdev)
		goto end;

	old_emlsr_disable_req =
	ml_nlink_clr_emlsr_mode_disable_req(
			psoc, ml_vdev,
			ML_EMLSR_DISALLOW_BY_AP_CSA |
			ML_EMLSR_DOWNGRADE_BY_AP_CSA);

	if (old_emlsr_disable_req & ML_EMLSR_DOWNGRADE_BY_AP_CSA)
		ml_nlink_set_emlsr_mode_disable_req(
					psoc, ml_vdev,
					ML_EMLSR_DOWNGRADE_BY_OPP_TIMER);

	if (old_emlsr_disable_req & ML_EMLSR_DISALLOW_BY_AP_CSA)
		ml_nlink_set_emlsr_mode_disable_req(
					psoc, ml_vdev,
					ML_EMLSR_DISALLOW_BY_OPP_TIMER);

end:
	if (ml_vdev)
		wlan_objmgr_vdev_release_ref(ml_vdev, WLAN_MLO_MGR_ID);

	return status;
}

/**
 * ml_nlink_nan_pre_enable_handler() - Disable eMLSR in case of nan pre-enable
 * @psoc: psoc object
 * @vdev:vdev object
 * @evt: event type
 * @data: event data
 *
 * Return: success
 */
static QDF_STATUS
ml_nlink_nan_pre_enable_handler(struct wlan_objmgr_psoc *psoc,
				struct wlan_objmgr_vdev *vdev,
				enum ml_nlink_change_event_type evt,
				struct ml_nlink_change_event *data)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_vdev *ml_vdev = NULL;

	if (!policy_mgr_is_mlo_in_mode_emlsr(psoc, NULL, NULL) ||
	    !wlan_mlme_is_aux_emlsr_support(psoc))
		return QDF_STATUS_SUCCESS;

	ml_vdev = ml_nlink_get_affect_ml_sta(psoc);
	if (!ml_vdev)
		goto end;

	/* Check if any set link is already in progress and thus wait */
	policy_mgr_wait_for_set_link_update(psoc);

	/* eMLSR disable by nan start */
	ml_nlink_set_emlsr_mode_disable_req(psoc, ml_vdev,
					    ML_EMLSR_DISALLOW_BY_NAN_DISC);

end:
	if (ml_vdev)
		wlan_objmgr_vdev_release_ref(ml_vdev, WLAN_MLO_MGR_ID);

	return status;
}

/**
 * ml_nlink_nan_post_disable_handler() - Re-evaluate eMLSR in case of
 * nan pre-enable
 * @psoc: psoc object
 * @vdev:vdev object
 * @evt: event type
 * @data: event data
 *
 * Return: success
 */

static QDF_STATUS
ml_nlink_nan_post_disable_handler(struct wlan_objmgr_psoc *psoc,
				  struct wlan_objmgr_vdev *vdev,
				  enum ml_nlink_change_event_type evt,
				  struct ml_nlink_change_event *data)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_vdev *ml_vdev = NULL;
	uint32_t old_emlsr_disable_req;

	if (!policy_mgr_is_mlo_in_mode_emlsr(psoc, NULL, NULL) ||
	    !wlan_mlme_is_aux_emlsr_support(psoc))
		return QDF_STATUS_SUCCESS;
	     ml_vdev = ml_nlink_get_affect_ml_sta(psoc);
	if (!ml_vdev)
		goto end;

	old_emlsr_disable_req =
	ml_nlink_clr_emlsr_mode_disable_req(
			psoc, ml_vdev,
			ML_EMLSR_DISALLOW_BY_NAN_DISC);
end:
	if (ml_vdev)
		wlan_objmgr_vdev_release_ref(ml_vdev, WLAN_MLO_MGR_ID);

	return status;
}

static QDF_STATUS ml_nlink_emlsr_opportunistic_timer_handler(
		struct wlan_objmgr_psoc *psoc,
		struct wlan_objmgr_vdev *vdev,
		enum ml_nlink_change_event_type evt,
		struct ml_nlink_change_event *data)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_vdev *ml_vdev = NULL;
	uint32_t disable_req;
	struct set_link_req req;

	if (!psoc) {
		mlo_err("psoc null");
		return QDF_STATUS_E_INVAL;
	}

	if (!policy_mgr_is_mlo_in_mode_emlsr(psoc, NULL, NULL) ||
	    !wlan_mlme_is_aux_emlsr_support(psoc))
		return QDF_STATUS_SUCCESS;

	ml_vdev = ml_nlink_get_affect_ml_sta(psoc);
	if (!ml_vdev)
		goto end;

	disable_req = ml_nlink_get_emlsr_mode_disable_req(psoc, ml_vdev);
	if (disable_req &
	    ~(ML_EMLSR_DISALLOW_BY_OPP_TIMER |
	      ML_EMLSR_DOWNGRADE_BY_OPP_TIMER |
	      ML_EMLSR_DISALLOW_BY_CONCURENCY |
	      ML_EMLSR_DISALLOW_BY_NAN_DISC)) {
		mlo_debug("emlsr disabled by req 0x%x", disable_req);
		goto end;
	}

	disable_req =
	ml_nlink_clr_emlsr_mode_disable_req(
			psoc, ml_vdev,
			ML_EMLSR_DISALLOW_BY_OPP_TIMER |
			ML_EMLSR_DOWNGRADE_BY_OPP_TIMER);

	if (disable_req & ML_EMLSR_DOWNGRADE_BY_OPP_TIMER) {
		qdf_mem_zero(&req, sizeof(req));
		ml_nlink_update_force_link_request(
				psoc, ml_vdev, &req,
				SET_LINK_FROM_EMLSR_DOWNGRADE);
		status = ml_nlink_state_change(
					psoc,
					MLO_LINK_FORCE_REASON_CONNECT,
					evt, data);
	} else if ((disable_req & ML_EMLSR_DISALLOW_BY_OPP_TIMER) &&
		   !(disable_req & ML_EMLSR_DISALLOW_BY_CONCURENCY)) {
		status = ml_nlink_update_non_force_disallow_bitmap(
					psoc, ml_vdev, evt, data,
					MLO_LINK_FORCE_REASON_CONNECT,
					0);
	}
end:
	if (ml_vdev)
		wlan_objmgr_vdev_release_ref(ml_vdev, WLAN_MLO_MGR_ID);

	return status;
}

static void ml_nlink_check_stop_start_emlsr_timer(
		struct wlan_objmgr_psoc *psoc,
		struct wlan_objmgr_vdev *vdev,
		enum ml_nlink_change_event_type evt,
		struct ml_nlink_change_event *data)
{
	struct wlan_objmgr_vdev *ml_vdev = NULL;
	uint32_t old_emlsr_disable_req;

	if (!policy_mgr_is_mlo_in_mode_emlsr(psoc, NULL, NULL) ||
	    !wlan_mlme_is_aux_emlsr_support(psoc))
		return;

	switch (evt) {
	case ml_nlink_roam_sync_start_evt:
	case ml_nlink_roam_sync_completion_evt:
	case ml_nlink_connect_pre_start_evt:
	case ml_nlink_connect_start_evt:
	case ml_nlink_disconnect_start_evt:
	case ml_nlink_disconnect_completion_evt:
	case ml_nlink_connect_failed_evt:
		if (!mlo_is_mld_sta(vdev))
			break;
		/* MLO STA association is changed, link state
		 * should be reset, so stop emlsr timer
		 */
		policy_mgr_stop_emlsr_opportunistic_timer(psoc);
		return;
	case ml_nlink_link_switch_start_evt:
	case ml_nlink_link_switch_pre_completion_evt:
	case ml_nlink_connect_completion_evt:
	case ml_nlink_ap_start_evt:
	case ml_nlink_ap_started_evt:
	case ml_nlink_ap_stopped_evt:
	case ml_nlink_ap_csa_start_evt:
	case ml_nlink_ap_csa_end_evt:
	case ml_nlink_ap_start_failed_evt:
	case ml_nlink_nan_pre_enable_evt:
	case ml_nlink_nan_post_enable_evt:
	case ml_nlink_nan_post_disable_evt:
		break;
	default:
		return;
	}

	ml_vdev = ml_nlink_get_affect_ml_sta(psoc);
	if (!ml_vdev)
		goto end;

	old_emlsr_disable_req =
		ml_nlink_get_emlsr_mode_disable_req(psoc, ml_vdev);

	if (old_emlsr_disable_req & ML_EMLSR_DISABLE_MASK_ALL &
	    ~ML_EMLSR_DISALLOW_BY_CONCURENCY)
		policy_mgr_restart_emlsr_opportunistic_timer(psoc);

end:
	if (ml_vdev)
		wlan_objmgr_vdev_release_ref(ml_vdev, WLAN_MLO_MGR_ID);
}

QDF_STATUS
ml_nlink_conn_change_notify(struct wlan_objmgr_psoc *psoc,
			    uint8_t vdev_id,
			    enum ml_nlink_change_event_type evt,
			    struct ml_nlink_change_event *data)
{
	struct wlan_objmgr_vdev *vdev;
	enum QDF_OPMODE mode;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct ml_link_force_state curr_force_state = {0};
	bool is_set_link_in_progress = policy_mgr_is_set_link_in_progress(psoc);
	bool is_host_force;

	mlo_debug("vdev %d %s(%d)", vdev_id, link_evt_to_string(evt),
		  evt);
	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, vdev_id,
						    WLAN_MLO_MGR_ID);
	if (!vdev) {
		mlo_err("invalid vdev id %d ", vdev_id);
		return QDF_STATUS_E_INVAL;
	}
	mode = wlan_vdev_mlme_get_opmode(vdev);

	switch (evt) {
	case ml_nlink_link_switch_start_evt:
		if (data->evt.link_switch.reason ==
		    MLO_LINK_SWITCH_REASON_HOST_FORCE) {
			is_host_force = true;
		} else {
			is_host_force = false;
		}

		mlo_debug("set_link_in_prog %d reason %d",
			  is_set_link_in_progress,
			  data->evt.link_switch.reason);

		if (is_set_link_in_progress) {
			/* If set active is in progress then only accept host
			 * force link switch requests from FW
			 */
			if (is_host_force)
				status = QDF_STATUS_SUCCESS;
			else
				status = QDF_STATUS_E_INVAL;
			break;
		} else if (is_host_force) {
			/* If set active is not in progress but FW sent host
			 * force then reject the link switch
			 */
			status = QDF_STATUS_E_INVAL;
			break;
		}

		ml_nlink_get_curr_force_state(psoc, vdev, &curr_force_state);
		if ((1 << data->evt.link_switch.new_ieee_link_id) &
		    curr_force_state.force_inactive_bitmap) {
			mlo_debug("target link %d is force inactive, don't switch to it",
				  data->evt.link_switch.new_ieee_link_id);
			status = QDF_STATUS_E_INVAL;
		}
		break;
	case ml_nlink_link_switch_pre_completion_evt:
		status = ml_nlink_swtich_dynamic_inactive_link(
				psoc, vdev);
		break;
	case ml_nlink_roam_sync_start_evt:
		ml_nlink_clr_requested_emlsr_mode(psoc, vdev);
		ml_nlink_clr_force_state(psoc, vdev);
		ml_nlink_clr_emlsr_mode_disable_req(
			psoc, vdev, ML_EMLSR_DISABLE_MASK_ALL);
		ml_nlink_reset_disallow_modes(psoc, vdev);
		break;
	case ml_nlink_roam_sync_completion_evt:
		status = ml_nlink_state_change_handler(
			psoc, vdev, MLO_LINK_FORCE_REASON_CONNECT,
			evt, data);
		break;
	case ml_nlink_connect_start_evt:
		ml_nlink_clr_requested_emlsr_mode(psoc, vdev);
		ml_nlink_clr_force_state(psoc, vdev);
		ml_nlink_clr_emlsr_mode_disable_req(
			psoc, vdev, ML_EMLSR_DISABLE_MASK_ALL);
		ml_nlink_reset_disallow_modes(psoc, vdev);
		break;
	case ml_nlink_connect_completion_evt:
		ml_nlink_init_disallow_modes(psoc, vdev);
		status = ml_nlink_undo_emlsr_downgrade_handler(
			psoc, vdev, evt, data);
		status = ml_nlink_state_change_handler(
			psoc, vdev, MLO_LINK_FORCE_REASON_DISCONNECT,
			evt, data);
		break;
	case ml_nlink_disconnect_start_evt:
		ml_nlink_clr_requested_emlsr_mode(psoc, vdev);
		ml_nlink_clr_force_state(psoc, vdev);
		ml_nlink_clr_emlsr_mode_disable_req(
			psoc, vdev, ML_EMLSR_DISABLE_MASK_ALL);
		ml_nlink_reset_disallow_modes(psoc, vdev);
		break;
	case ml_nlink_disconnect_completion_evt:
		status = ml_nlink_state_change_handler(
			psoc, vdev, MLO_LINK_FORCE_REASON_DISCONNECT,
			evt, data);
		break;
	case ml_nlink_connect_pre_start_evt:
	case ml_nlink_ap_start_evt:
		status = ml_nlink_emlsr_downgrade_handler(psoc, vdev, evt,
							  data);
		break;
	case ml_nlink_connect_failed_evt:
	case ml_nlink_ap_start_failed_evt:
		status = ml_nlink_undo_emlsr_downgrade_handler(
			psoc, vdev, evt, data);
		break;
	case ml_nlink_ap_started_evt:
		status = ml_nlink_undo_emlsr_downgrade_handler(
			psoc, vdev, evt, data);
		status = ml_nlink_state_change_handler(
			psoc, vdev, MLO_LINK_FORCE_REASON_DISCONNECT,
			evt, data);
		break;
	case ml_nlink_ap_stopped_evt:
		status = ml_nlink_state_change_handler(
			psoc, vdev, MLO_LINK_FORCE_REASON_DISCONNECT,
			evt, data);
		break;
	case ml_nlink_ap_csa_start_evt:
		status = ml_nlink_ap_csa_start_handler(
			psoc, vdev, evt, data);
		break;
	case ml_nlink_ap_csa_end_evt:
		status = ml_nlink_ap_csa_end_handler(
			psoc, vdev, evt, data);
		break;
	case ml_nlink_emlsr_timeout_evt:
		status = ml_nlink_emlsr_opportunistic_timer_handler(
			psoc, vdev, evt, data);
		break;
	case ml_nlink_connection_updated_evt:
	case ml_nlink_post_set_link_evt:
		if (mode == QDF_STA_MODE &&
		    (MLME_IS_ROAM_SYNCH_IN_PROGRESS(psoc, vdev_id) ||
		     MLME_IS_MLO_ROAM_SYNCH_IN_PROGRESS(psoc, vdev_id))) {
			mlo_debug("vdev id %d in roam sync", vdev_id);
			break;
		}
		status = ml_nlink_state_change_handler(
			psoc, vdev, MLO_LINK_FORCE_REASON_CONNECT,
			evt, data);
		break;
	case ml_nlink_tdls_request_evt:
		status = ml_nlink_tdls_event_handler(
			psoc, vdev, evt, data);
		break;
	case ml_nlink_vendor_cmd_request_evt:
		status = ml_nlink_vendor_cmd_handler(
			psoc, vdev, evt, data);
		break;
	case ml_nlink_nan_pre_enable_evt:
		mlo_debug("NAN Pre-enable");
		status = ml_nlink_nan_pre_enable_handler(
			psoc, vdev, evt, data);
		if (QDF_IS_STATUS_SUCCESS(status))
			status = ml_nlink_state_change_handler(psoc, vdev,
				MLO_LINK_FORCE_REASON_CONNECT, evt, data);
		break;
	case ml_nlink_nan_post_enable_evt:
		status = ml_nlink_state_change_handler(
			psoc, vdev, MLO_LINK_FORCE_REASON_CONNECT,
			evt, data);
		mlo_debug("NAN Post-enable");
		break;
	case ml_nlink_nan_post_disable_evt:
		/* reset NAN eMLSR disallowed bit */
		status = ml_nlink_nan_post_disable_handler(
			psoc, vdev, evt, data);
		/* re-evaluate inactive link */
		status = ml_nlink_state_change_handler(
			psoc, vdev, MLO_LINK_FORCE_REASON_DISCONNECT,
			evt, data);
		mlo_debug("NAN Post-disable");
		break;
	default:
		break;
	}

	ml_nlink_check_stop_start_emlsr_timer(psoc, vdev, evt, data);

	if (vdev)
		wlan_objmgr_vdev_release_ref(vdev, WLAN_MLO_MGR_ID);

	return status;
}

QDF_STATUS
ml_nlink_vendor_command_set_link(struct wlan_objmgr_psoc *psoc,
				 uint8_t vdev_id,
				 enum link_control_modes link_control_mode,
				 enum mlo_link_force_reason reason,
				 enum mlo_link_force_mode mode,
				 uint8_t link_num,
				 uint16_t link_bitmap,
				 uint16_t link_bitmap2)
{
	struct ml_nlink_change_event data;

	if (policy_mgr_is_emlsr_sta_concurrency_present(psoc) &&
	    !wlan_mlme_is_aux_emlsr_support(psoc)) {
		mlo_debug("eMLSR concurrency not allow to set link");
		return QDF_STATUS_E_INVAL;
	}

	qdf_mem_zero(&data, sizeof(data));
	data.evt.vendor.link_ctrl_mode = link_control_mode;
	data.evt.vendor.mode = mode;
	data.evt.vendor.reason = reason;
	data.evt.vendor.link_num = link_num;
	data.evt.vendor.link_bitmap = link_bitmap;
	data.evt.vendor.link_bitmap2 = link_bitmap2;

	return ml_nlink_conn_change_notify(
			psoc, vdev_id,
			ml_nlink_vendor_cmd_request_evt, &data);
}

#if defined(WLAN_FEATURE_11BE_MLO) && defined(FEATURE_DENYLIST_MGR)
uint8_t
mlo_get_curr_link_combination(struct wlan_objmgr_vdev *vdev)
{
	struct mlo_link_info link_info;
	uint8_t i = 0;
	uint8_t curr_link_comb = 0;

	/* Iterate from 1st index, because assoc link is at 0th index */
	for (i = 1; i < WLAN_MAX_ML_BSS_LINKS; i++) {
		link_info = vdev->mlo_dev_ctx->link_ctx->links_info[i];
		if (qdf_is_macaddr_zero(&link_info.ap_link_addr))
			break;

		if (!link_info.is_link_active)
			continue;

		curr_link_comb |= 1 << i;
	}
	return curr_link_comb;
}
#endif

