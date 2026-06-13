// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SEC_PANEL_NOTIFIER_V2_H__
#define __SEC_PANEL_NOTIFIER_V2_H__
#include <linux/notifier.h>

enum panel_notifier_event_t {
	PANEL_EVENT_BL_STATE_CHANGED,
	PANEL_EVENT_VRR_STATE_CHANGED,
	PANEL_EVENT_LFD_STATE_CHANGED,
	PANEL_EVENT_PANEL_STATE_CHANGED,
	PANEL_EVENT_UB_CON_STATE_CHANGED,
	PANEL_EVENT_COPR_STATE_CHANGED,
	PANEL_EVENT_TEST_MODE_STATE_CHANGED,
	PANEL_EVENT_SCREEN_MODE_STATE_CHANGED,
	PANEL_EVENT_ESD_STATE_CHANGED,
	MAX_PANEL_EVENT,
};

enum panel_notifier_event_state_t {
	PANEL_EVENT_STATE_NONE,

	/* PANEL_EVENT_BL_STATE_CHANGED */

	/* PANEL_EVENT_VRR_STATE_CHANGED */

	/* PANEL_EVENT_LFD_STATE_CHANGED */

	/* PANEL_EVENT_PANEL_STATE_CHANGED */
	PANEL_EVENT_PANEL_STATE_OFF,
	PANEL_EVENT_PANEL_STATE_ON,
	PANEL_EVENT_PANEL_STATE_LPM,

	/* PANEL_EVENT_UB_CON_STATE_CHANGED */
	PANEL_EVENT_UB_CON_STATE_CONNECTED,
	PANEL_EVENT_UB_CON_STATE_DISCONNECTED,

	/* PANEL_EVENT_COPR_STATE_CHANGED */
	PANEL_EVENT_COPR_STATE_DISABLED,
	PANEL_EVENT_COPR_STATE_ENABLED,

	/* PANEL_EVENT_TEST_MODE_STATE_CHANGED */
	PANEL_EVENT_TEST_MODE_STATE_NONE,
	PANEL_EVENT_TEST_MODE_STATE_GCT,

	/* PANEL_EVENT_SCREEN_MODE_STATE_CHANGED */

	/* PANEL_EVENT_ESD_STATE_CHANGED */

	MAX_PANEL_EVENT_STATE,
};

struct panel_event_bl_data {
	int level;
	int aor;
	int finger_mask_hbm_on;
	int acl_status;
	int gradual_acl_val;
};

/* dms: display modes */
struct panel_event_dms_data {
	int fps;
	int lfd_min_freq;
	int lfd_max_freq;
};

#define MAX_COPR_ROI (5)

enum panel_event_copr_wrgb {
	PANEL_EVENT_COPR_WRGB_W,
	PANEL_EVENT_COPR_WRGB_R,
	PANEL_EVENT_COPR_WRGB_G,
	PANEL_EVENT_COPR_WRGB_B,
	MAX_PANEL_EVENT_COPR_WRGB,
};

struct panel_event_copr_data {
	unsigned int stat[MAX_COPR_ROI][MAX_PANEL_EVENT_COPR_WRGB];
	unsigned int nr_stat;
};

struct panel_notifier_event_data {
	/* base */
	unsigned int display_index;
	/* state */
	enum panel_notifier_event_state_t state;
	/* additional data */
	union {
		struct panel_event_bl_data bl;
		struct panel_event_dms_data dms;
		struct panel_event_copr_data copr;
		unsigned int screen_mode;
	} d;
};

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
extern int panel_notifier_register(struct notifier_block *nb);
extern int panel_notifier_unregister(struct notifier_block *nb);
extern int panel_notifier_call_chain(unsigned long val, void *v);
#else
static inline int panel_notifier_register(struct notifier_block *nb)
{
	return 0;
};

static inline int panel_notifier_unregister(struct notifier_block *nb)
{
	return 0;
};

static inline int panel_notifier_call_chain(unsigned long val, void *v)
{
	return 0;
};
#endif /* CONFIG_SEC_PANEL_NOTIFIER_V2 */
#endif /* __SEC_PANEL_NOTIFIER_V2_H__ */
