/*
 * FST Manager implementation
 *
 * Copyright (c) 2015-2016, 2019-2020 The Linux Foundation. All rights reserved.
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
#include "common/ieee802_11_defs.h"
#include "fst_ctrl.h"
#include "fst_mux.h"
#include "fst/fst_ctrl_defs.h"
#include "fst_cfgmgr.h"
#define FST_MGR_COMPONENT "MGR"
#include "fst_manager.h"
#include "fst_rateupg.h"
#include <stdbool.h>

#define FST_LLT_SWITCH_IMMEDIATELY 0
#define LLT_UNIT_US        32 /* See 10.32.2.2  Transitioning between states */
#define MS_TO_LLT_VALUE(l) (((l) * 1000) / LLT_UNIT_US)

struct fst_mgr
{
	struct dl_list groups;
};

struct fst_mgr_group
{
	struct fst_group_info info;
	struct fst_mux       *drv;
	struct dl_list        sessions;
	struct dl_list        ifaces;
	struct dl_list        peers;
	struct dl_list        mgr_lentry;
};

struct fst_mgr_iface
{
	struct fst_iface_info info;
	struct dl_list        grp_lentry;
};

enum fst_mgr_session_state
{
	FST_MGR_SESSION_STATE_IDLE,
	FST_MGR_SESSION_STATE_INITIATED,
	FST_MGR_SESSION_STATE_ESTABLISHED,
	FST_MGR_SESSION_STATE_IN_TRANSITION,
	FST_MGR_SESSION_STATE_LAST
};

struct fst_mgr_session
{
	enum fst_mgr_session_state state;
	u32                        id;
	struct fst_mgr_group      *group;
	struct fst_mgr_iface      *old_iface;
	struct fst_mgr_iface      *new_iface;
	u32                        llt;
	Boolean                    non_compliant;
	struct dl_list             grp_lentry;
};

struct fst_mgr_peer_iface
{
	struct fst_mgr_iface   *iface;
	u8			addr[ETH_ALEN];
	struct dl_list      	peer_lentry;
};

struct fst_mgr_peer
{
	struct fst_mgr_session *session;
	struct fst_mgr_iface   *active_iface;
	struct dl_list          ifaces;
	struct dl_list          grp_lentry;
};

extern unsigned int fst_num_of_retries;
extern Boolean fst_force_nc;

#define _fst_mgr_foreach_grp(m, g) \
	dl_list_for_each((g), &(m)->groups, struct fst_mgr_group, mgr_lentry)

#define _fst_grp_foreach_iface(g, i) \
	dl_list_for_each((i), &(g)->ifaces, struct fst_mgr_iface, grp_lentry)

#define _fst_grp_foreach_peer(g, p) \
	dl_list_for_each((p), &(g)->peers, struct fst_mgr_peer, grp_lentry)

#define _fst_grp_foreach_session(g, s) \
	dl_list_for_each((s), &(g)->sessions, struct fst_mgr_session, grp_lentry)

#define _fst_peer_foreach_iface(p, i) \
	dl_list_for_each((i), &(p)->ifaces, struct fst_mgr_peer_iface, peer_lentry)

static void _fst_mgr_on_peer_connected(struct fst_mgr *mgr, const char *ifname,
				       const u8* addr);

static int _fst_mgr_peer_set_active_iface(struct fst_mgr_peer *p,
					  struct fst_mgr_iface *i,
					  struct fst_mux *drv);

static void _fst_mgr_peer_check_compliance(struct fst_mgr_peer *p);

static const u8 *_fst_mgr_peer_get_addr_of_iface(struct fst_mgr_peer *p,
					   struct fst_mgr_iface *iface);

/* helpers */
static const char *state_name(enum fst_mgr_session_state state)
{
	static const char *state_names[] = {
		[FST_MGR_SESSION_STATE_IDLE] = "IDLE",
		[FST_MGR_SESSION_STATE_INITIATED] = "INITIATED",
		[FST_MGR_SESSION_STATE_ESTABLISHED] = "ESTABLISHED",
		[FST_MGR_SESSION_STATE_IN_TRANSITION] = "IN TRANSITION",
	};

	if (state >= sizeof(state_names)/sizeof(state_names[0]) ||
	    !state_names[state])
		return "UNKNOWN_STATE";

	return state_names[state];
}

static inline void fst_mgr_printf_session_info(struct fst_mgr_session *s)
{
	if (s) {
		fst_mgr_printf(MSG_INFO, "****** session %u info begin",s->id);
		fst_mgr_printf(MSG_INFO, "****** old_i = %s",
				s->old_iface ? s->old_iface->info.name : "NULL" );
		fst_mgr_printf(MSG_INFO, "****** new_i = %s",
				s->new_iface ? s->new_iface->info.name : "NULL" );
		fst_mgr_printf(MSG_INFO, "****** llt   = %u",
				s->llt);
		fst_mgr_printf(MSG_INFO, "****** state = %s", state_name(s->state));
		fst_mgr_printf(MSG_INFO, "****** session %u info end", s->id);
	}
}

/*
 * FST Manager Session
 */
static inline Boolean _fst_mgr_session_is_in_progress(struct fst_mgr_session *s)
{
	return (s->state != FST_MGR_SESSION_STATE_IDLE);
}

static inline int _fst_mgr_session_is_ready(struct fst_mgr_session *s)
{
	return (s->state == FST_MGR_SESSION_STATE_ESTABLISHED);
}

static inline struct fst_mgr_iface *
_fst_mgr_session_get_old_iface(struct fst_mgr_session *s)
{
	return s->old_iface;
}

static inline struct fst_mgr_iface *
_fst_mgr_session_get_new_iface(struct fst_mgr_session *s)
{
	return s->new_iface;
}

static int _fst_mgr_session_set_old_iface(struct fst_mgr_session *s,
		struct fst_mgr_iface *i)
{
	if (!s->non_compliant)
		if (fst_session_set(s->id, FST_CSS_PNAME_OLD_IFNAME, i->info.name)) {
			fst_mgr_printf(MSG_ERROR, "session %u: cannot set old iface to %s",
					s->id, i->info.name);
			return -1;
		}

	s->old_iface = i;
	return 0;
}

static int _fst_mgr_session_set_new_iface(struct fst_mgr_session *s,
		struct fst_mgr_iface *i)
{
	if (!s->non_compliant)
		if (fst_session_set(s->id, FST_CSS_PNAME_NEW_IFNAME, i->info.name)) {
			fst_mgr_printf(MSG_ERROR, "session %u: cannot set new iface to %s",
					s->id, i->info.name);
			return -1;
		}

	s->new_iface = i;
	return 0;
}

static int _fst_mgr_session_set_peer_addr(struct fst_mgr_peer *p)
{
	struct fst_mgr_session *s = p->session;

	if (s == NULL) {
		fst_mgr_printf(MSG_ERROR, "Session does not exist");
		return -1;
	}

	if (s->non_compliant)
		return 0;

	char pval[] = "XX:XX:XX:XX:XX:XX";
	const u8 *old_addr, *new_addr;

	old_addr = _fst_mgr_peer_get_addr_of_iface(p, s->old_iface);
	new_addr = _fst_mgr_peer_get_addr_of_iface(p, s->new_iface);
	if (!old_addr || !new_addr) {
		fst_mgr_printf(MSG_ERROR, "session %u: cannot set addr for %s and %s",
				s->id, s->old_iface->info.name,
				s->new_iface->info.name);
		return -1;
	}

	snprintf(pval, sizeof(pval), MACSTR, MAC2STR(old_addr));

	if (fst_session_set(s->id, FST_CSS_PNAME_OLD_PEER_ADDR, pval)) {
		fst_mgr_printf(MSG_ERROR, "session %u: cannot set old addr to %s",
				s->id, pval);
		return -1;
	}

	snprintf(pval, sizeof(pval), MACSTR, MAC2STR(new_addr));

	if (fst_session_set(s->id, FST_CSS_PNAME_NEW_PEER_ADDR, pval)) {
		fst_mgr_printf(MSG_ERROR, "session %u: cannot set new addr to %s",
				s->id, pval);
		return -1;
	}

	return 0;
}

static int _fst_mgr_session_set_llt(struct fst_mgr_session *s,
		u32 llt)
{
	if (!s->non_compliant) {
		char pval[32];

		snprintf(pval, sizeof(pval), "%u", llt);

		if (fst_session_set(s->id, FST_CSS_PNAME_LLT, pval)) {
			fst_mgr_printf(MSG_ERROR, "session %u: cannot set LLT to %s",
					s->id, pval);
			return -1;
		}
	}

	s->llt = llt;
	return 0;
}

static int _fst_mgr_session_initiate_setup(struct fst_mgr_session *s)
{
	WPA_ASSERT(!s->non_compliant);

	if (fst_session_initiate(s->id)) {
		fst_mgr_printf(MSG_ERROR, "session %u: cannot initiate setup",
				s->id);
		return -1;
	}

	fst_mgr_printf(MSG_INFO, "session %u: setup initiated",
			s->id);
	s->state = FST_MGR_SESSION_STATE_INITIATED;
	return 0;
}


static void _fst_mgr_session_nc_transfer(struct fst_mgr_session *s,
					 struct fst_mgr_peer *p)
{
	WPA_ASSERT(s->non_compliant);

	fst_mgr_printf(MSG_INFO, "session %u: performing non-compliant transfer"
		" for: old_iface=%s new_iface=%s",
		s->id, s->old_iface->info.name,
		s->new_iface->info.name);
	_fst_mgr_peer_set_active_iface(p, s->new_iface, s->group->drv);
	s->state = FST_MGR_SESSION_STATE_IDLE;
}

static void _fst_mgr_session_check_for_nc_transfer(struct fst_mgr_session *s,
						   struct fst_mgr_peer *p)
{
	if (p->active_iface->info.priority < s->new_iface->info.priority)
		_fst_mgr_session_nc_transfer(s, p);
}

static int _fst_mgr_session_transfer(struct fst_mgr_session *s)
{
	WPA_ASSERT(!s->non_compliant);
	if (fst_session_transfer(s->id)) {
		fst_mgr_printf(MSG_ERROR, "session %u: cannot transfer",
				s->id);
		return -1;
	}
	fst_mgr_printf(MSG_INFO, "session %u: transfer initiated",
			s->id);
	s->state = FST_MGR_SESSION_STATE_IN_TRANSITION;

	return 0;
}

static struct fst_mgr_peer *
_fst_mgr_group_peer_by_session(struct fst_mgr_group *g,
			       struct fst_mgr_session *s)
{
	struct fst_mgr_peer *p;

	_fst_grp_foreach_peer(g, p)
		if (p->session == s)
			return p;

	return NULL;
}

static int
_fst_mgr_set_link_loss(const char *ifname, const u8 *addr, bool fst_link_loss)
{
	char fname[128];
	FILE *f;

	if (ifname == NULL)
		return -1;

	if (snprintf(fname, sizeof(fname), "/sys/class/net/%s/device/wil6210/fst_link_loss",
		     ifname) < 0)
		return -1;

	f = fopen(fname, "r+");
	if (!f) {
		fst_mgr_printf(MSG_ERROR, "failed to open: %s", fname);
		return -1;
	}

	if (fprintf(f, MACSTR " %d\n", MAC2STR(addr), fst_link_loss) < 0) {
		fclose(f);
		return -1;
	}
	fclose(f);

	return 0;
}

static void
_fst_mgr_session_set_link_loss(struct fst_mgr_session *s, bool fst_link_loss)
{
	int ret;
	struct fst_mgr_peer *p = NULL;
	struct fst_mgr_peer_iface *pi;

	p = _fst_mgr_group_peer_by_session(s->group, s);
	if (!p) {
		fst_mgr_printf(MSG_WARNING, "couldn't find peer");
		return;
	}

	_fst_peer_foreach_iface(p, pi) {
		if (pi->iface == s->old_iface) {
			ret = _fst_mgr_set_link_loss(s->old_iface->info.name,
						     pi->addr, fst_link_loss);
			if (ret < 0)
				fst_mgr_printf(MSG_WARNING, "failed to set fst link loss %s",
					       fst_link_loss ? "On" : "Off");
			else
				fst_mgr_printf(MSG_INFO, "fst link loss %s for peer "
					       MACSTR " iface %s",
					       fst_link_loss ? "enabled" : "disabled",
					       MAC2STR(pi->addr),
					       s->old_iface->info.name);
			break;
		}
	}
}

static int _fst_mgr_session_respond(struct fst_mgr_session *s, Boolean accept)
{
	const char *responce_status =
		accept ? FST_CS_PVAL_RESPONSE_ACCEPT :
			 FST_CS_PVAL_RESPONSE_REJECT;
	if (fst_session_respond(s->id, responce_status)) {
		fst_mgr_printf(MSG_ERROR, "session %u: cannot respond",
				s->id);
		return -1;
	}

	s->state = accept ?
			FST_MGR_SESSION_STATE_ESTABLISHED :
			FST_MGR_SESSION_STATE_IDLE;

	if (accept && s->llt > 0)
		/*
		 * at this point the active interface is the one with highest
		 * priority and we have a backup interface. Set active interface
		 * to aggressive link loss detection for fast switching to
		 * backup once signal quality is low
		 */
		_fst_mgr_session_set_link_loss(s, true);
	return 0;
}

static void _fst_mgr_session_reset(struct fst_mgr_session *s,
	Boolean allow_tear_down)
{
	if (!s->non_compliant && allow_tear_down && _fst_mgr_session_is_ready(s)) {
		if (fst_session_teardown(s->id))
			fst_mgr_printf(MSG_WARNING, "session %u: cannot reset", s->id);
	}

	s->state = FST_MGR_SESSION_STATE_IDLE;
}

static void _fst_mgr_session_deinit(struct fst_mgr_session *s)
{
	dl_list_del(&s->grp_lentry);
	fst_session_remove(s->id);
	os_free(s);
}

static int _fst_mgr_session_init(struct fst_mgr_group *g,
		struct fst_mgr_session **_s, u32 session_id)
{
	struct fst_mgr_session *s;

	if (session_id == FST_INVALID_SESSION_ID &&
		fst_session_add(g->info.id, &session_id)) {
		fst_mgr_printf(MSG_ERROR, "group %s: cannot add session ",
				g->info.id);
		goto error_add;
	}

	s = os_malloc(sizeof(*s));
	if (!s) {
		fst_mgr_printf(MSG_ERROR, "group %s: cannot allocate session ",
				g->info.id);
		goto error_alloc;
	}

	os_memset(s, 0, sizeof(*s));

	s->state = FST_MGR_SESSION_STATE_IDLE;
	s->group = g;
	s->id    = session_id;
	s->non_compliant = fst_force_nc;
	dl_list_add_tail(&g->sessions, &s->grp_lentry);

	fst_mgr_printf(MSG_INFO, "group %s: session %u added",
			g->info.id, session_id);

	if (_s)
		*_s = s;

	return 0;

error_alloc:
	fst_session_remove(session_id);
error_add:
	return -1;

}

/*
 * FST Manager Peer
 */

static void _fst_mgr_peer_print_connected_addr(struct fst_mgr_peer *p)
{
	if (dl_list_empty(&p->ifaces)) {
		fst_mgr_printf(MSG_DEBUG, "peer %p has no connections", p);
		return;
	}

	struct fst_mgr_peer_iface *pi;
	_fst_peer_foreach_iface(p, pi) {
		fst_mgr_printf(MSG_DEBUG, "peer %p has addr " MACSTR " / %s",
			p, MAC2STR(pi->addr), pi->iface->info.name);
	}
}

static const u8 *_fst_mgr_peer_get_addr_of_iface(struct fst_mgr_peer *p,
					   struct fst_mgr_iface *iface)
{
	struct fst_mgr_peer_iface *pi;

	_fst_peer_foreach_iface(p, pi) {
		if (pi->iface == iface)
			return pi->addr;
	}

	return NULL;
}

static int _fst_mgr_peer_set_active_iface(struct fst_mgr_peer *p,
		struct fst_mgr_iface *i,
		struct fst_mux *drv)
{
	int res = 0;

	if (i && p->active_iface == i) {
		fst_mgr_printf(MSG_INFO, "%s is already active for peer %p",
			i->info.name, p);
		return 0;
	}

	if (p->active_iface) {
		const u8 *addr = _fst_mgr_peer_get_addr_of_iface(p, p->active_iface);
		if (addr) {
			fst_mux_del_map_entry(drv, addr);
			fst_mgr_printf(MSG_INFO,
				       "Map entry removed: " MACSTR " via %s",
				       MAC2STR(addr),
				       p->active_iface->info.name);
			p->active_iface = NULL;
		}
	}

	if (!i)
		return 0;

	const u8 *addr = _fst_mgr_peer_get_addr_of_iface(p, i);
	if (!addr) {
		fst_mgr_printf(MSG_ERROR, "Peer is not connected via %s",
			i->info.name);
		return -1;
	}

	res = fst_mux_add_map_entry(drv, addr, i->info.name);
	if (!res) {
		/* Set iface as an active */
		p->active_iface = i;
		fst_mgr_printf(MSG_INFO,
			"Map entry added: " MACSTR " via %s",
			MAC2STR(addr), i->info.name);
	} else
		fst_mgr_printf(MSG_ERROR,
			"Cannot add map entry: " MACSTR " via %s",
			MAC2STR(addr), i->info.name);
	return res;
}

static struct fst_mgr_iface *
_fst_mgr_peer_get_next_iface(struct fst_mgr_peer *p)
{
	struct fst_mgr_peer_iface *pi;
	struct fst_mgr_iface *_i;
	struct fst_mgr_iface *i = NULL;
	int                   max_priority = -1;

	_fst_peer_foreach_iface(p, pi) {
		_i = pi->iface;
		if (_i == p->active_iface)
			continue;
		if (max_priority == -1 || max_priority < _i->info.priority) {
			max_priority = _i->info.priority;
			i = _i;
		}
	}

	return i;
}

static void _fst_mgr_peer_try_to_initiate_next_setup(struct fst_mgr_peer *p,
	struct fst_mgr_group *g)
{
	struct fst_mgr_iface *new_i;
	u32 llt;

	if (!fst_is_supplicant())
		/* AP doesn't initiate sessions */
		return;

	if (p->session && _fst_mgr_session_is_in_progress(p->session)) {
		fst_mgr_printf(MSG_WARNING,
			"peer %p: Cannot initiate next setup: "
			"another session is in progress", p);
		return;
	}

	if (!p->active_iface) {
		fst_mgr_printf(MSG_WARNING,
			"peer %p: Cannot initiate next setup: "
			"no active iface", p);
		return;
	}

	if (!p->session &&
		_fst_mgr_session_init(g, &p->session, FST_INVALID_SESSION_ID)) {
		fst_mgr_printf(MSG_ERROR, "group %s: cannot initialize session with peer %p",
				g->info.id, p);
		return;
	}

	new_i = _fst_mgr_peer_get_next_iface(p);
	if (new_i == NULL) {
		fst_mgr_printf(MSG_WARNING,
			"peer %p: Cannot initiate next setup: "
			"no backup iface connected", p);
		return;
	}

	_fst_mgr_peer_check_compliance(p);
	llt = MS_TO_LLT_VALUE(new_i->info.llt);
	if (p->active_iface &&
		p->active_iface->info.priority < new_i->info.priority) {
		llt = FST_LLT_SWITCH_IMMEDIATELY;
	}

	if (_fst_mgr_session_set_old_iface(p->session, p->active_iface) ||
		_fst_mgr_session_set_new_iface(p->session, new_i) ||
		_fst_mgr_session_set_peer_addr(p) ||
		_fst_mgr_session_set_llt(p->session, llt)) {
		fst_mgr_printf(MSG_WARNING,
			"peer %p: Cannot initiate next setup: "
			"session %u configuration failed", p, p->session->id);
		return;
	}

	if (!p->session->non_compliant) {
		fst_mgr_printf(MSG_INFO,
			"peer %p: session %u: initiating setup: "
			"old_iface=%s new_iface=%s llt=%d",
			p, p->session->id,
			p->active_iface->info.name, new_i->info.name, llt);
		_fst_mgr_session_initiate_setup(p->session);
	}
}

static void _fst_mgr_peer_check_compliance(struct fst_mgr_peer *p)
{
	struct fst_mgr_peer_iface *pi;

	WPA_ASSERT(p->session != NULL);
	if (p->session == NULL) {
		fst_mgr_printf(MSG_ERROR, "peer session is invalid");
		return;
	}

	if (fst_force_nc) {
		p->session->non_compliant = TRUE;
		return;
	}

	p->session->non_compliant = TRUE;
	_fst_peer_foreach_iface(p, pi) {
		if (fst_get_peer_mbies(pi->iface->info.name,
			pi->addr, NULL) > 0) {
			p->session->non_compliant = FALSE;
			break;
		}
	}
	fst_mgr_printf(MSG_INFO, "peer %p: non_compliant: %d",
		p, p->session->non_compliant);
}

static Boolean _fst_mgr_peer_add_iface(struct fst_mgr_peer *p,
		struct fst_mgr_iface *i, const u8 *addr)
{
	struct fst_mgr_peer_iface *pi = os_zalloc(sizeof(*pi));
	if (!pi)
		return FALSE;

	pi->iface = i;
	os_memcpy(pi->addr, addr, ETH_ALEN);

	dl_list_add_tail(&p->ifaces, &pi->peer_lentry);

	return TRUE;
}

static void _fst_mgr_peer_del_iface(struct fst_mgr_peer *p,
	struct fst_mgr_iface *i)
{
	struct fst_mgr_peer_iface *pi;
	dl_list_for_each(pi, &p->ifaces, struct fst_mgr_peer_iface, peer_lentry)
		if (pi->iface == i) {
			dl_list_del(&pi->peer_lentry);
			os_free(pi);
			break;
		}
}

static void _fst_mgr_peer_session_deinit(struct fst_mgr_peer *p,
	Boolean allow_tear_down)
{
	if (p->session) {
		_fst_mgr_session_reset(p->session, allow_tear_down);
		_fst_mgr_session_deinit(p->session);
		p->session = NULL;
	}
}

static void _fst_mgr_peer_deinit(struct fst_mgr_peer *p)
{
	dl_list_del(&p->grp_lentry);
	while (!dl_list_empty(&p->ifaces)) {
		struct fst_mgr_peer_iface *pi = dl_list_first(&p->ifaces,
				struct fst_mgr_peer_iface, peer_lentry);
		dl_list_del(&pi->peer_lentry);
		os_free(pi);
	}
	if (p->session)
		_fst_mgr_session_deinit(p->session);
	os_free(p);
}

static int _fst_mgr_peer_init(struct fst_mgr_group *g, const u8 *addr,
		struct fst_mgr_iface *i)
{
	struct fst_mgr_peer    *p;
	struct fst_mgr_session *s;

	if (_fst_mgr_session_init(g, &s, FST_INVALID_SESSION_ID)) {
		fst_mgr_printf(MSG_ERROR, "group %s: cannot initialize session " MACSTR,
				g->info.id, MAC2STR(addr));
		goto error_session_init;
	}

	p = os_malloc(sizeof(*p));
	if (!p) {
		fst_mgr_printf(MSG_ERROR, "group %s: cannot allocate peer " MACSTR,
				g->info.id, MAC2STR(addr));
		goto error_alloc;
	}

	os_memset(p, 0, sizeof(*p));

	dl_list_init(&p->ifaces);

	p->active_iface  = i;
	p->session       = s;

	dl_list_add_tail(&g->peers, &p->grp_lentry);

	if (!_fst_mgr_peer_add_iface(p, i, addr)) {
		fst_mgr_printf(MSG_ERROR, "Peer interface allocation error");
		goto error_add_iface;
	}

	fst_mgr_printf(MSG_INFO, "group %s: peer " MACSTR ": iface %s added",
			g->info.id, MAC2STR(addr), i->info.name);

	_fst_mgr_peer_print_connected_addr(p);
	return 0;

error_add_iface:
	os_free(p);
error_alloc:
	_fst_mgr_session_deinit(s);
error_session_init:
	return -1;
}

/*
 * FST Manager Interface
 */
static void _fst_mgr_iface_deinit(struct fst_mgr_iface *i, struct fst_mux *drv)
{
	dl_list_del(&i->grp_lentry);
	fst_mux_unregister_iface(drv, i->info.name);
	fst_cfgmgr_on_iface_deinit(&i->info);
	os_free(i);
}

static int _fst_mgr_iface_init(struct fst_mgr_group *g,
		struct fst_iface_info *finfo, struct fst_mux *drv)
{
	struct fst_mgr_iface *i;

	if (fst_cfgmgr_on_iface_init(&g->info, finfo)) {
		fst_mgr_printf(MSG_ERROR, "Cannot init iface %s", finfo->name);
		goto error_init;
	}

	if (fst_mux_register_iface(drv, finfo->name, finfo->priority)) {
		fst_mgr_printf(MSG_ERROR, "Cannot register iface %s with driver",
				finfo->name);
		goto error_slave;

	}

	i = os_malloc(sizeof(*i));
	if (!i) {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate object for iface %s",
				finfo->name);
		goto error_alloc;
	}

	os_memset(i, 0, sizeof(*i));

	i->info = *finfo;
	dl_list_add_tail(&g->ifaces, &i->grp_lentry);

	return 0;

error_alloc:
	fst_mux_unregister_iface(drv, finfo->name);
error_slave:
	fst_cfgmgr_on_iface_deinit(finfo);
error_init:
	return -1;
}

/*
 * FST Manager Group
 */

static struct fst_mgr_peer *_fst_mgr_group_peer_by_addr(struct fst_mgr_group *g,
		const u8 *addr)
{
	struct fst_mgr_peer *p;

	_fst_grp_foreach_peer(g, p) {
		struct fst_mgr_peer_iface *pi;
		_fst_peer_foreach_iface(p, pi) {
			if (!os_memcmp(addr, pi->addr, ETH_ALEN))
				return p;
		}
	}

	return NULL;
}

const u8 *fst_mgr_get_addr_from_mbie(struct multi_band_ie *mbie)
{
	const u8 *addr = NULL;

	switch (MB_CTRL_ROLE(mbie->mb_ctrl)) {
	case MB_STA_ROLE_AP:
		addr = mbie->bssid;
		break;
	case MB_STA_ROLE_NON_PCP_NON_AP:
		if (mbie->mb_ctrl & MB_CTRL_STA_MAC_PRESENT &&
			(size_t) 2 + mbie->len >= sizeof(*mbie) + ETH_ALEN)
			addr = (const u8 *) &mbie[1];
		break;
	default:
		break;
	}

	return addr;
}

static Boolean _fst_mgr_is_other_addr_in_mbies(struct fst_iface_info *info,
					const u8 *addr, const u8 *other_addr)
{
	char *str_mbies = NULL;
	int str_mbies_size;
	u8 *mbies = NULL, *mbies_iter;
	int mbies_size;
	Boolean result = FALSE;

	str_mbies_size = fst_get_peer_mbies(info->name, addr, &str_mbies);
	if (str_mbies_size < 2 || str_mbies_size & 1)
		goto finish;

	mbies_size = str_mbies_size / 2;
	mbies = os_malloc(mbies_size);
	if (!mbies)
		goto finish;
	if (hexstr2bin(str_mbies, mbies, mbies_size))
		goto finish;

	mbies_iter = mbies;
	while (mbies_size >= 2) {
		struct multi_band_ie *mbie = (struct multi_band_ie *) mbies_iter;
		const u8 *mbie_addr;

		if (mbie->eid != WLAN_EID_MULTI_BAND ||
			(size_t) 2 + mbie->len < sizeof(*mbie))
			break;

		mbie_addr = fst_mgr_get_addr_from_mbie(mbie);
		if (mbie_addr && !os_memcmp(mbie_addr, other_addr, ETH_ALEN)) {
			result = TRUE;
			break;
		}

		mbies_iter += mbie->len + 2;
		mbies_size -= mbie->len + 2;
	}
finish:
	if (str_mbies)
		os_free(str_mbies);
	if (mbies)
		os_free(mbies);
	return result;
}

static struct fst_mgr_peer *
_fst_mgr_group_peer_by_other_addr(struct fst_mgr_group *g,
				  const u8 *other_addr,
				  struct fst_iface_info *other_iface_info)
{
	struct fst_mgr_peer *p;

	/* check if MAC address of new connection can be found in the MB IE of
	 * the existing connection under the peer or if the MAC address of the
	 * existing connections under the peer can be found in the MB IE of
	 * the new connection.
	 */
	_fst_grp_foreach_peer(g, p) {
		struct fst_mgr_peer_iface *pi;
		_fst_peer_foreach_iface(p, pi) {
			if (os_strncmp(pi->iface->info.name,
				       other_iface_info->name,
				       FST_MAX_INTERFACE_SIZE) &&
			    (_fst_mgr_is_other_addr_in_mbies(
				&pi->iface->info, pi->addr, other_addr) ||
			     _fst_mgr_is_other_addr_in_mbies(
				other_iface_info, other_addr, pi->addr)))
				return p;
		}
	}
	return NULL;
}

static Boolean _fst_mgr_is_peer_connected(struct fst_mgr_group *g,
					const char *ifname,
					const u8 *addr)
{
	struct fst_mgr_peer *p = NULL;

	_fst_grp_foreach_peer(g, p) {
		struct fst_mgr_peer_iface *pi;
		_fst_peer_foreach_iface(p, pi) {
			if (!os_strncmp(pi->iface->info.name, ifname, FST_MAX_INTERFACE_SIZE) &&
				!os_memcmp(pi->addr, addr, ETH_ALEN))
				return TRUE;

		}
	}
	return FALSE;
}

static void _fst_mgr_group_deinit(struct fst_mgr_group *g)
{
	fst_mux_stop(g->drv);
	while (!dl_list_empty(&g->peers)) {
		struct fst_mgr_peer *p = dl_list_first(&g->peers,
				struct fst_mgr_peer, grp_lentry);
		_fst_mgr_peer_deinit(p);
	}
	while (!dl_list_empty(&g->sessions)) {
		struct fst_mgr_session *s = dl_list_first(&g->sessions,
				struct fst_mgr_session, grp_lentry);
		_fst_mgr_session_deinit(s);
	}
	while (!dl_list_empty(&g->ifaces)) {
		struct fst_mgr_iface *i = dl_list_first(&g->ifaces,
				struct fst_mgr_iface, grp_lentry);
		_fst_mgr_iface_deinit(i, g->drv);
	}
	fst_mux_cleanup(g->drv);
	dl_list_del(&g->mgr_lentry);
	fst_cfgmgr_on_group_deinit(&g->info);
	os_free(g);
}

static void _fst_mgr_group_check_connections(struct fst_mgr *mgr, struct fst_group_info *ginfo)
{
	int i, nof_ifaces;
	struct fst_iface_info *ifaces;

	nof_ifaces = fst_cfgmgr_get_group_ifaces(ginfo, &ifaces);
	if (nof_ifaces < 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot get ifaces for group %s", ginfo->id);
		return;
	}

	for (i = 0; i < nof_ifaces; i++) {
		uint8_t *peers = NULL, *p;
		int nof_peers;
		nof_peers = fst_get_iface_peers(ginfo, &ifaces[i], &peers);
		if (nof_peers < 0) {
			fst_mgr_printf(MSG_ERROR,
						"Cannot get peers for iface \'%s\'",
						ifaces[i].name);
			continue;
		}
		p = peers;
		while (nof_peers--) {
			_fst_mgr_on_peer_connected(mgr, ifaces[i].name, p);
			p += ETH_ALEN;
		}
		if (peers)
			fst_free(peers);
	}

	fst_free(ifaces);
}

static int _fst_mgr_group_init(struct fst_mgr *mgr,
		struct fst_group_info *ginfo)
{
	int i, nof_ifaces;
	struct fst_iface_info *ifaces;
	struct fst_mux        *drv;
	struct fst_mgr_group  *g;

	if (fst_cfgmgr_on_group_init(ginfo)) {
		fst_mgr_printf(MSG_ERROR, "Cannot init group %s", ginfo->id);
		goto error_group_init;
	}

	nof_ifaces = fst_cfgmgr_get_group_ifaces(ginfo, &ifaces);
	if (nof_ifaces < 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot get ifaces for group %s", ginfo->id);
		goto error_group_ifaces;
	}

	fst_mgr_printf(MSG_DEBUG, "group %s: %d ifaces found",
			ginfo->id, nof_ifaces);

	drv = fst_mux_init(ginfo->id);
	if (!drv) {
		fst_mgr_printf(MSG_ERROR, "Cannot initiate driver for group %s",
				ginfo->id);
		goto error_drv;
	}

	g = os_malloc(sizeof(*g));
	if (!g) {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate object for group %s",
				ginfo->id);
		goto error_alloc;
	}

	os_memset(g, 0, sizeof(*g));

	dl_list_init(&g->sessions);
	dl_list_init(&g->ifaces);
	dl_list_init(&g->peers);

	g->drv  = drv;
	g->info = *ginfo;

	for (i = 0; i < nof_ifaces; i++)
		if (!ifaces[i].manual_enslave && _fst_mgr_iface_init(g, &ifaces[i], drv)) {
			fst_mgr_printf(MSG_ERROR, "Cannot init iface for group %s",
					ginfo->id);
			goto error_iface;
		}

	if (fst_mux_start(drv) != 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot start driver for group %s",
				ginfo->id);
		goto error_drv_start;
	}

	dl_list_add_tail(&mgr->groups, &g->mgr_lentry);

	fst_mgr_printf(MSG_INFO, "group %s with %d ifaces initialized",
			ginfo->id, nof_ifaces);

	_fst_mgr_group_check_connections(mgr, ginfo);

	fst_free(ifaces);

	return 0;

error_drv_start:
error_iface:
	while (!dl_list_empty(&g->ifaces)) {
		struct fst_mgr_iface *i = dl_list_first(&g->ifaces,
				struct fst_mgr_iface, grp_lentry);
		_fst_mgr_iface_deinit(i, drv);
	}
error_alloc:
	fst_mux_cleanup(drv);
error_drv:
	fst_free(ifaces);
error_group_ifaces:
	fst_cfgmgr_on_group_deinit(ginfo);
error_group_init:
	return -1;
}

/*
 * FST Manager
 */
static struct fst_mgr_group *_fst_mgr_group_by_name(struct fst_mgr *mgr,
		const char *name)
{
	struct fst_mgr_group *g;

	_fst_mgr_foreach_grp(mgr, g) {
		if (!os_strcmp(g->info.id, name)) {
			return g;
		}
	}

	return NULL;
}

static struct fst_mgr_group *_fst_mgr_group_by_ifname(struct fst_mgr *mgr,
		const char *ifname, struct fst_mgr_iface **iface)
{
	struct fst_mgr_group *g;

	_fst_mgr_foreach_grp(mgr, g) {
		struct fst_mgr_iface *i;
		_fst_grp_foreach_iface(g, i) {
			if (!os_strcmp(ifname, i->info.name)) {
				if (iface)
					*iface = i;
				return g;
			}
		}
	}

	return NULL;
}

static struct fst_mgr_group *_fst_mgr_group_by_session_id(struct fst_mgr *mgr,
		u32 session_id, struct fst_mgr_session **session)
{
	struct fst_mgr_group *g;

	_fst_mgr_foreach_grp(mgr, g) {
		struct fst_mgr_session *s;
		_fst_grp_foreach_session(g, s) {
			if (s->id == session_id) {
				if (session)
					*session = s;
				return g;
			}
		}
	}

	return NULL;
}

static void _fst_mgr_on_peer_connected(struct fst_mgr *mgr,
		const char *ifname,
		const u8* addr)
{
	struct fst_mgr_group   *g;
	struct fst_mgr_peer    *p;
	struct fst_mgr_iface   *i, *tmp_i;
	int num_ifaces = 0;

	g = _fst_mgr_group_by_ifname(mgr, ifname, &i);
	if (!g) {
		fst_mgr_printf(MSG_ERROR, "iface %s: no group found",
				ifname);
		return;
	}

	if (_fst_mgr_is_peer_connected(g, ifname, addr)) {
		fst_mgr_printf(MSG_INFO, "peer already connected on iface %s",
				   ifname);
		return;
	}

	_fst_grp_foreach_iface(g, tmp_i) {
		num_ifaces++;
	}
	if (num_ifaces < 2) {
		fst_mgr_printf(MSG_INFO, "not enough group interfaces to do FST");
		return;
	}

	if (fst_cfgmgr_on_connect(&g->info, ifname, addr))
		return;

	p = _fst_mgr_group_peer_by_other_addr(g, addr, &i->info);
	if (!p) {
		if (!fst_mux_add_map_entry(g->drv, addr, i->info.name))
			_fst_mgr_peer_init(g, addr, i);

		/* We have not more than 1 iface connected to this peer, so session
		 * cannot be established right now
		 */
		return;
	}

	if(!_fst_mgr_peer_add_iface(p, i, addr)) {
		fst_mgr_printf(MSG_ERROR, "Peer interface allocation error");
		return;
	}

	if (p->session) {
		struct fst_mgr_iface *csi =
			_fst_mgr_session_get_new_iface(p->session);
		if (csi && (i->info.priority > csi->info.priority)) {
			fst_mgr_printf(MSG_WARNING,
				"iface %s: higher priority resets session",
				ifname);
			_fst_mgr_session_reset(p->session, TRUE);
		}
	}
	_fst_mgr_peer_try_to_initiate_next_setup(p, g);

	if (p->session && p->session->non_compliant)
		_fst_mgr_session_check_for_nc_transfer(p->session, p);

	_fst_mgr_peer_print_connected_addr(p);
}

static void _fst_mgr_on_peer_disconnected(struct fst_mgr *mgr,
		const char *ifname,
		const u8* addr)
{
	struct fst_mgr_group   *g;
	struct fst_mgr_peer    *p;
	struct fst_mgr_iface   *i;
	Boolean                 switch_initiated = FALSE;

	g = _fst_mgr_group_by_ifname(mgr, ifname, &i);
	if (!g) {
		fst_mgr_printf(MSG_ERROR, "iface %s: no group found",
				ifname);
		return;
	}

	p = _fst_mgr_group_peer_by_addr(g, addr);
	if (!p) {
		fst_mgr_printf(MSG_ERROR, "group %s: peer " MACSTR
				": not found",
				g->info.id, MAC2STR(addr));
		return;
	}

	fst_mgr_printf(MSG_INFO, "group %s: peer " MACSTR
			": disconnected (session=%p i=%s)",
			g->info.id, MAC2STR(addr),
			p->session, i->info.name);

	fst_mgr_printf_session_info(p->session);

	if (!p->session)
		fst_mgr_printf(MSG_INFO, "group %s: peer " MACSTR
				": disconnect ignored (no session)",
				g->info.id, MAC2STR(addr));
	else if (i == _fst_mgr_session_get_old_iface(p->session) &&
		_fst_mgr_session_is_ready(p->session)) {
		fst_mgr_printf(MSG_INFO, "group %s: peer " MACSTR
				": initiating switch",
				g->info.id, MAC2STR(addr));
		if (p->session->non_compliant) {
			_fst_mgr_session_nc_transfer(p->session, p);
			switch_initiated = TRUE;
		}
		else if (!_fst_mgr_session_transfer(p->session))
			switch_initiated = TRUE;
		else {
			fst_mgr_printf(MSG_ERROR, "group %s: peer " MACSTR
					": switch failed, deinitializing session",
					g->info.id, MAC2STR(addr));
			_fst_mgr_peer_session_deinit(p, TRUE);
		}
	} else if (i == _fst_mgr_session_get_old_iface(p->session) ||
			   i == _fst_mgr_session_get_new_iface(p->session)) {
		fst_mgr_printf(MSG_INFO, "group %s: peer " MACSTR
				": deinitializing session",
				g->info.id, MAC2STR(addr));
		_fst_mgr_peer_session_deinit(p, TRUE);
	}

	Boolean force_set_active = FALSE;
	if (i == p->active_iface) {
		_fst_mgr_peer_set_active_iface(p, NULL, g->drv);
		force_set_active = TRUE;
	}
	_fst_mgr_peer_del_iface(p, i);
	_fst_mgr_peer_print_connected_addr(p);

	if (switch_initiated)
		_fst_mgr_peer_set_active_iface(p, p->session->new_iface,
			g->drv);
	else if (dl_list_empty(&p->ifaces)) {
		fst_mgr_printf(MSG_INFO, "group %s: peer " MACSTR
				   ": deinitializing peer (no more interfaces)",
				   g->info.id, MAC2STR(addr));
		_fst_mgr_peer_deinit(p);
		fst_mux_del_map_entry(g->drv, addr);
	} else {
		if (force_set_active) {
			struct fst_mgr_iface *new_i =
					_fst_mgr_peer_get_next_iface(p);
			fst_mgr_printf(MSG_INFO, "group %s: peer " MACSTR
					": setting new active iface to %s",
					g->info.id, MAC2STR(addr),
					new_i ? new_i->info.name : "NULL");
			_fst_mgr_peer_set_active_iface(p, new_i, g->drv);
		}

		_fst_mgr_peer_try_to_initiate_next_setup(p, g);
	}

	fst_cfgmgr_on_disconnect(&g->info, ifname, addr);
}

static void _fst_mgr_on_peer_state_changed(struct fst_mgr *mgr,
		const union fst_event_extra *data)
{
	if (data->peer_state.connected)
		_fst_mgr_on_peer_connected(mgr,
			data->peer_state.ifname,
			data->peer_state.addr);
	else
		_fst_mgr_on_peer_disconnected(mgr,
			data->peer_state.ifname,
			data->peer_state.addr);
}

static void _fst_mgr_on_ctrl_notification_state_change(struct fst_mgr *mgr,
		struct fst_mgr_group *g, struct fst_mgr_session *s,
		union fst_event_extra *evext)
{
	static unsigned retry_counter = 0;
	struct fst_mgr_peer *p;

	fst_mgr_printf(MSG_INFO, "session %u: state %s => %s",
			s->id,
			fst_session_state_name(evext->session_state.old_state),
			fst_session_state_name(evext->session_state.new_state));

	if (evext->session_state.new_state != FST_SESSION_STATE_INITIAL)
		return;

	p = _fst_mgr_group_peer_by_session(g, s);
	WPA_ASSERT(p != NULL);
	if (p == NULL) {
		fst_mgr_printf(MSG_ERROR, "peer not found for group %s, session %u",
				   g->info.id, s->id);
		return;
	}

	switch (evext->session_state.extra.to_initial.reason) {
	case REASON_SETUP:
		break;
	case REASON_SWITCH:
		WPA_ASSERT(evext->session_state.extra.to_initial.initiator !=
			FST_INITIATOR_UNDEFINED);
		fst_mgr_printf(MSG_INFO, "session %u: switched by %s side",
			s->id,
			(evext->session_state.extra.to_initial.initiator ==
				FST_INITIATOR_LOCAL) ? "local" : "remote");
		_fst_mgr_peer_set_active_iface(p, s->new_iface, g->drv);
		s->state = FST_MGR_SESSION_STATE_IDLE;

		const u8 *old_addr = _fst_mgr_peer_get_addr_of_iface(p, s->old_iface);
		if (old_addr)
			fst_cfgmgr_on_switch_completed(&g->info,
						       s->old_iface->info.name,
						       s->new_iface->info.name,
						       old_addr);

		break;
	case REASON_TEARDOWN:
	case REASON_STT:
	case REASON_REJECT:
	case REASON_ERROR_PARAMS:
	case REASON_RESET:
		break;
	default:
		fst_mgr_printf(MSG_ERROR, "session %u: unknown reset reason %d",
			s->id, evext->session_state.extra.to_initial.reason);
		break;
	}

	/* delete old session */
	_fst_mgr_peer_session_deinit(p, TRUE);

	if (evext->session_state.extra.to_initial.reason == REASON_STT)
		retry_counter++;
	else
		retry_counter = 0;

	if (evext->session_state.extra.to_initial.reason == REASON_SETUP)
		/* session requested by peer. We're done */
		return;

	/* session setup (initiate_next_setup) is invoked in following cases:
	 * 1. following STT (retry)
	 * 2. following other error cases like reject (TODO: should this be
	      considered as retry???)
	 * 3. following successful session switch
	 */
	if (retry_counter < fst_num_of_retries) {
		fst_mgr_printf(MSG_INFO, "initiating setup. retry %d", retry_counter);
		_fst_mgr_peer_try_to_initiate_next_setup(p, g);
	} else {
		fst_mgr_printf(MSG_INFO, "no more retries. give up");
		retry_counter = 0;
	}
}

static void _fst_mgr_on_setup(struct fst_mgr *mgr, u32 session_id)
{
	struct fst_mgr_group   *g, *g_new;
	struct fst_mgr_session *s;
	struct fst_mgr_iface   *old_i;
	struct fst_mgr_iface   *new_i;
	struct fst_mgr_peer    *p;
	struct fst_session_info sinfo;

	if (fst_is_supplicant()) {
		fst_mgr_printf(MSG_ERROR, "drop unexpected FST setup request from AP");
		return;
	}

	memset(&sinfo, 0, sizeof(sinfo));

	if (fst_session_get_info(session_id, &sinfo)) {
		fst_mgr_printf(MSG_ERROR, "session %u: cannot get info",
			session_id);
	}

	g = _fst_mgr_group_by_ifname(mgr, sinfo.old_ifname, &old_i);
	if (!g) {
		fst_mgr_printf(MSG_ERROR, "session %u: no group found for iface %s",
			session_id, sinfo.old_ifname);
		return;
	}

	g_new = _fst_mgr_group_by_ifname(mgr, sinfo.new_ifname, &new_i);
	if (!g_new) {
		fst_mgr_printf(MSG_ERROR, "session %u: no group found for iface %s",
			session_id, sinfo.new_ifname);
		return;
	}

	if (g != g_new) {
		fst_mgr_printf(MSG_ERROR, "session %u: ifaces %s and %s belong to "
				"different groups",
			session_id, sinfo.old_ifname, sinfo.new_ifname);
		return;
	}

	p = _fst_mgr_group_peer_by_addr(g, sinfo.old_peer_addr);
	if (!p) {
		fst_mgr_printf(MSG_ERROR, "session %u: no peer found for old mac "
			MACSTR, session_id, MAC2STR(sinfo.old_peer_addr));
		return;
	}

	if (p != _fst_mgr_group_peer_by_addr(g, sinfo.new_peer_addr)) {
		fst_mgr_printf(MSG_ERROR,
			"session %u: mismatch of peer with old mac (" MACSTR
			") and peer with new mac (" MACSTR ")", session_id,
			MAC2STR(sinfo.old_peer_addr), MAC2STR(sinfo.new_peer_addr));
		return;
	}

	if (p->active_iface != old_i) {
		fst_mgr_printf(MSG_WARNING, "session %u: sync active iface: "
			"%s => %s",
			session_id,
			p->active_iface ? p->active_iface->info.name : "NONE",
			sinfo.old_ifname);
		if (_fst_mgr_peer_set_active_iface(p, old_i, g->drv)) {
			fst_mgr_printf(MSG_ERROR, "session %u: cannot sync "
				"active iface. Rejecting.",
				session_id);
			return;
		}
	}

	if (_fst_mgr_session_init(g, &s, session_id)) {
		fst_mgr_printf(MSG_ERROR, "session %u: cannot init session",
			session_id);
		return;
	}

	s->old_iface     = old_i;
	s->new_iface     = new_i;
	s->llt           = sinfo.llt;
	s->state         = FST_MGR_SESSION_STATE_INITIATED;
	s->non_compliant = FALSE;

	if (new_i->info.priority > old_i->info.priority) {
		_fst_mgr_session_set_llt(s, FST_LLT_SWITCH_IMMEDIATELY);
		s->llt = FST_LLT_SWITCH_IMMEDIATELY;
	}

	if (p->session) {
		fst_mgr_printf(MSG_WARNING, "session %u: deinit due to new setup",
			p->session->id);
		_fst_mgr_peer_session_deinit(p, TRUE);
	}

	p->session = s;

	_fst_mgr_session_respond(s , TRUE);

	fst_mgr_printf(MSG_ERROR, "session %u: established (responder)",
		s->id);
}


static void _fst_mgr_ctrl_notification_cb_func(void *cb_ctx,
	u32 session_id, enum fst_event_type event_type, void *extra)
{
	struct fst_mgr         *mgr = cb_ctx;
	struct fst_mgr_group   *g   = NULL;
	struct fst_mgr_session *s   = NULL;
	struct fst_mgr_iface   *iface;

	if (event_type != EVENT_FST_SETUP &&
		session_id != FST_INVALID_SESSION_ID) {
		g = _fst_mgr_group_by_session_id(mgr, session_id, &s);
		if (!g) {
			fst_mgr_printf(MSG_ERROR, "session %u: no group found",
					session_id);
			return;
		}
	}

	switch (event_type) {
	case EVENT_FST_ESTABLISHED:
		if (s != NULL) {
			fst_mgr_printf(MSG_WARNING, "session %u: established (initiator)",
					session_id);
			s->state = FST_MGR_SESSION_STATE_ESTABLISHED;

			if (s->llt > 0)
				/*
				 * at this point the active interface is the one
				 * with highest priority and we have a backup
				 * interface. Set active interface to aggressive
				 * link loss detection for fast switching to
				 * backup once signal quality is low.
				 */
				_fst_mgr_session_set_link_loss(s, true);
		}
		else
			fst_mgr_printf(MSG_ERROR, "Cannot find session object");
		break;
	case EVENT_FST_SETUP:
		_fst_mgr_on_setup(mgr, session_id);
		break;
	case EVENT_FST_SESSION_STATE_CHANGED:
		if (g != NULL && s != NULL)
			_fst_mgr_on_ctrl_notification_state_change(mgr, g, s, extra);
		else
			fst_mgr_printf(MSG_ERROR, "Cannot find group/session object");
		break;
	case EVENT_PEER_STATE_CHANGED:
		_fst_mgr_on_peer_state_changed(mgr, extra);
		break;
	case EVENT_FST_SCAN_STARTED:
		fst_mgr_printf(MSG_INFO, "scan started on %s", (char *)extra);

		g = _fst_mgr_group_by_ifname(mgr, extra, &iface);
		if (!g) {
			fst_mgr_printf(MSG_ERROR, "no group found for iface %s", extra);
			break;
		}

		fst_cfgmgr_on_scan_started(&g->info, extra);
		break;
	case EVENT_FST_SCAN_COMPLETED:
		fst_mgr_printf(MSG_INFO, "scan complete on %s", (char *)extra);
		g = _fst_mgr_group_by_ifname(mgr, extra, &iface);
		if (!g) {
			fst_mgr_printf(MSG_ERROR, "no group found for iface %s", extra);
			break;
		}

		fst_cfgmgr_on_scan_completed(&g->info, extra);
		break;
	case EVENT_FST_SIGNAL_CHANGE:
		fst_mgr_printf(MSG_INFO, "signal change on %s", (char *)extra);
		g = _fst_mgr_group_by_ifname(mgr, extra, &iface);
		if (!g) {
			fst_mgr_printf(MSG_ERROR, "no group found for iface %s", (char *)extra);
			break;
		}

		fst_cfgmgr_on_signal_change(&g->info, extra);
		break;
	default:
		fst_mgr_printf(MSG_WARNING, "session %u: unknown event #%d",
				session_id, event_type);
		break;
	}
}

/*
 * FST Manager public API
 */
static struct fst_mgr g_fst_mgr;
static int            g_fst_mgr_initalized = 0;

int fst_manager_init(void)
{
	int res, i, nof_groups;
	struct fst_group_info *groups = NULL;

	os_memset(&g_fst_mgr, 0, sizeof(g_fst_mgr));

	dl_list_init(&g_fst_mgr.groups);

	res = fst_set_notify_cb(_fst_mgr_ctrl_notification_cb_func, &g_fst_mgr);
	if (res != 0) {
		goto finish;
	}

	res = fst_cfgmgr_on_global_init();
	if (res < 0) {
		goto finish;
	}

	res = fst_cfgmgr_get_groups(&groups);
	if (res < 0) {
		goto finish;
	}

	nof_groups = res;
	for (i = 0; i < nof_groups; i++) {
		res = _fst_mgr_group_init(&g_fst_mgr, &groups[i]);
		if (res < 0) {
			goto finish;
		}
	}

	res = 0;
	fst_mgr_printf(MSG_INFO, "manager with %d groups initialized",
			nof_groups);

	g_fst_mgr_initalized = 1;

finish:
	if (groups)
		fst_free(groups);

	if (res)
		fst_manager_deinit();

	return res;
}

void fst_manager_deinit(void)
{
	if (g_fst_mgr_initalized) {
		fst_set_notify_cb(NULL, NULL);
		while (!dl_list_empty(&g_fst_mgr.groups)) {
			struct fst_mgr_group *g = dl_list_first(&g_fst_mgr.groups,
					struct fst_mgr_group, mgr_lentry);
			_fst_mgr_group_deinit(g);
		}
		os_memset(&g_fst_mgr, 0, sizeof(g_fst_mgr));
		g_fst_mgr_initalized = 0;
		fst_cfgmgr_on_global_deinit();
	}
}

static int _fst_mgr_get_iface_info(const char* ifname, struct fst_iface_info *iface)
{
	struct fst_mgr_group *g;
	int i, nof_ifaces;
	struct fst_iface_info *ifaces = NULL;

	_fst_mgr_foreach_grp(&g_fst_mgr, g) {
		nof_ifaces = fst_cfgmgr_get_group_ifaces(&g->info, &ifaces);
		for (i = 0; i < nof_ifaces; i++) {
			if (!os_strcmp(ifaces[i].name, ifname)) {
				*iface = ifaces[i];
				os_free(ifaces);
				return 0;
			}
		}
		os_free(ifaces);
		ifaces = NULL;
	}

	return -1;
}

static int _fst_mgr_group_enslaved_count(struct fst_mgr_group *g)
{
	return dl_list_len(&g->ifaces);
}

int fst_manager_enslave(const char *gname, const char* ifname, Boolean enslave)
{
	struct fst_mgr_group *g, *g2;
	int res, enslaved_count;

	if (!g_fst_mgr_initalized)
		return -1;

	g = _fst_mgr_group_by_name(&g_fst_mgr, gname);
	if (!g) {
		fst_mgr_printf(MSG_ERROR, "group %s not found", gname);
		return -1;
	}

	g2 = _fst_mgr_group_by_ifname(&g_fst_mgr, ifname, NULL);
	if (enslave && g2) {
		fst_mgr_printf(MSG_DEBUG, "iface %s: already enslaved", ifname);
		return 0;
	} else if (!enslave && !g2) {
		fst_mgr_printf(MSG_DEBUG, "iface %s: already released", ifname);
		return 0;
	}

	if (!enslave)
		fst_rate_upgrade_on_release(&g->info, ifname);

	enslaved_count = _fst_mgr_group_enslaved_count(g);
	res = fst_iface_enslave(&g->info, ifname, enslave);
	if (res)
		return res;

	res = fst_cfgmgr_update_txqueuelen(&g->info);
	if (res) {
		fst_mgr_printf(MSG_ERROR, "group %s: could not set txqueuelen, continue anyway\n",
			       gname);
		// continue anyway
	}

	if (enslave && enslaved_count == 0) {
		res = fst_cfgmgr_set_mux_iface_up(&g->info, TRUE);
		if (res)
			return res;
	} else if (!enslave && enslaved_count == 1) {
		res = fst_cfgmgr_set_mux_iface_up(&g->info, FALSE);
		if (res)
			return res;
	}

	if (!enslave) {
		// in manual enslave, set interface up after release
		set_iface_up(ifname, TRUE);
	}

	if (enslave) {
		struct fst_iface_info ifinfo;

		res = _fst_mgr_get_iface_info(ifname, &ifinfo);
		if (res) {
			fst_mgr_printf(MSG_ERROR, "iface %s not found", ifname);
			return -1;
		}

		res = _fst_mgr_iface_init(g, &ifinfo, g->drv);
		if (res) {
			fst_mgr_printf(MSG_ERROR, "Cannot init iface %s", ifinfo.name);
			return -1;
		}
	} else {
		struct fst_mgr_iface *iface;
		struct fst_mgr_peer *p, *p2;

		dl_list_for_each_safe(p, p2, &g->peers, struct fst_mgr_peer, grp_lentry) {
			struct fst_mgr_peer_iface *pi, *pi2;

			dl_list_for_each_safe(pi, pi2, &p->ifaces, struct fst_mgr_peer_iface, peer_lentry) {
				struct fst_iface_info *ifinfo = &pi->iface->info;

				if (!os_strcmp(ifinfo->name, ifname)) {
					fst_mgr_printf(MSG_INFO, "cleaning peer " MACSTR, MAC2STR(pi->addr));
					_fst_mgr_on_peer_disconnected(&g_fst_mgr, ifinfo->name, pi->addr);
				}
			}
		}


		_fst_grp_foreach_iface(g, iface) {
			if (!os_strcmp(iface->info.name, ifname)) {
				fst_mgr_printf(MSG_INFO, "iface %s deinit", ifname);
				_fst_mgr_iface_deinit(iface, g->drv);
				break;
			}
		}
	}

	fst_mux_stop(g->drv);
	if (fst_mux_start(g->drv) != 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot start driver for group %s",
				g->info.id);
		return -1;
	}

	if (enslave)
		_fst_mgr_group_check_connections(&g_fst_mgr, &g->info);

	return 0;
}

int fst_manager_is_enslaved(const char *gname, const char* ifname,
	Boolean *isEnslaved)
{
	struct fst_mgr_group *g, *g2;

	if (!g_fst_mgr_initalized || !isEnslaved)
		return -1;

	g = _fst_mgr_group_by_name(&g_fst_mgr, gname);
	if (!g) {
		fst_mgr_printf(MSG_ERROR, "group %s not found", gname);
		return -1;
	}

	g2 = _fst_mgr_group_by_ifname(&g_fst_mgr, ifname, NULL);
	*isEnslaved = ((g2 != NULL) && (g == g2));
	return 0;
}

int fst_manager_set_mac_address(const char *gname, const u8 *mac)
{
	struct fst_mgr_group *g;
	struct fst_mgr_iface *i;
	int res;

	if (!g_fst_mgr_initalized)
		return -1;

	g = _fst_mgr_group_by_name(&g_fst_mgr, gname);
	if (!g) {
		fst_mgr_printf(MSG_ERROR, "group name %s not found", gname);
		return -1;
	}

	_fst_grp_foreach_iface(g, i) {
		set_iface_up(i->info.name, FALSE);
	}

	res = fst_set_mac_address(&g->info, mac);

	_fst_grp_foreach_iface(g, i) {
		set_iface_up(i->info.name, TRUE);
	}

	return res;
}

int fst_manager_set_mux_iface_name(const char *gname, const char *ifname)
{
	fst_mgr_printf(MSG_ERROR, "STUB: set_mux_iface_name, group %s ifname %s",
			gname, ifname);
	return -1;
}

int fst_manager_rename_group_interface(const char *gname, const char *ifname,
	const char *newifname)
{
	struct fst_mgr_group *g;
	struct fst_mgr_iface *i;

	if (!g_fst_mgr_initalized)
		return -1;

	fst_mgr_printf(MSG_INFO, "group %s rename interface %s => %s", gname, ifname, newifname);

	g = _fst_mgr_group_by_name(&g_fst_mgr, gname);
	if (!g) {
		fst_mgr_printf(MSG_ERROR, "group name %s not found", gname);
		return -1;
	}

	_fst_grp_foreach_iface(g, i) {
		if (!os_strcmp(ifname, i->info.name)) {
			fst_mgr_printf(MSG_INFO, "cannot rename enslaved interface %s", ifname);
			return -1;
		}
	}

	return fst_cfgmgr_rename_interface(gname, ifname, newifname);
}

int fst_manager_session_transfer(const char *old_ifname, const u8 *peer_addr)
{
	struct fst_mgr_group *g;
	struct fst_mgr_peer *p;
	struct fst_mgr_iface *i;

	g = _fst_mgr_group_by_ifname(&g_fst_mgr, old_ifname, &i);
	if (!g) {
		fst_mgr_printf(MSG_ERROR, "iface %s: no group found", old_ifname);
		return -1;
	}

	p = _fst_mgr_group_peer_by_addr(g, peer_addr);
	if (!p) {
		fst_mgr_printf(MSG_ERROR, "group %s: peer " MACSTR ": not found",
			g->info.id, MAC2STR(peer_addr));
		return -1;
	}

	if (!p->session) {
		fst_mgr_printf(MSG_INFO, "no session for peer " MACSTR, MAC2STR(peer_addr));
		return -1;
	}

	if (i != _fst_mgr_session_get_old_iface(p->session) ||
		!_fst_mgr_session_is_ready(p->session)) {
		fst_mgr_printf(MSG_INFO, "session doesn't match for peer " MACSTR, MAC2STR(peer_addr));
		return -1;
	}

	fst_mgr_printf(MSG_INFO, "group %s: peer " MACSTR ": initiating switch",
		g->info.id, MAC2STR(peer_addr));
	fst_mgr_printf_session_info(p->session);

	if (!_fst_mgr_session_transfer(p->session)) {
		_fst_mgr_peer_set_active_iface(p, p->session->new_iface, g->drv);
	} else {
		fst_mgr_printf(MSG_ERROR, "group %s: peer " MACSTR
			": switch failed, deinitializing session",
			g->info.id, MAC2STR(peer_addr));
		_fst_mgr_peer_session_deinit(p, TRUE);
	}

	_fst_mgr_peer_try_to_initiate_next_setup(p, g);

	return 0;
}
