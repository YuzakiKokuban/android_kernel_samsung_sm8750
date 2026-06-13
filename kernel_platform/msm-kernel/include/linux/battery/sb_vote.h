/*
 * sb_vote.h
 * Samsung Mobile Battery Vote Header
 *
 * Copyright (C) 2021 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SB_VOTE_H
#define __SB_VOTE_H __FILE__

#include <linux/err.h>
#include <linux/battery/sb_def.h>

enum {
	SB_VOTE_MIN,
	SB_VOTE_MAX,
	SB_VOTE_EN,
	SB_VOTE_DATA,
};

enum {
	VOTE_PRI_0 = 0,
	VOTE_PRI_1,
	VOTE_PRI_2,
	VOTE_PRI_3,
	VOTE_PRI_4,
	VOTE_PRI_5,
	VOTE_PRI_6,
	VOTE_PRI_7,
	VOTE_PRI_8,
	VOTE_PRI_9,
	VOTE_PRI_10,
};
#define VOTE_PRI_MIN	VOTE_PRI_0
#define VOTE_PRI_MAX	VOTE_PRI_10

struct sb_vote;

typedef int (*cb_vote)(void *pdata, sb_data vdata);
typedef bool (*cmp_vote)(sb_data *vdata1, sb_data *vdata2);

struct sb_vote_cfg {
	const char	*name;
	int			type;

	const char	**voter_list;
	int			voter_num;

	cb_vote		cb;
	cmp_vote	cmp;
};

#define SB_VOTE_DISABLE		(-3663)
#define SB_VOTE_DISABLE_STR	"voteoff"

#if IS_ENABLED(CONFIG_SB_VOTE)
struct sb_vote *sb_vote_create(const struct sb_vote_cfg *vote_cfg, void *pdata, sb_data init_data);
void sb_vote_destroy(struct sb_vote *vote);

struct sb_vote *sb_vote_find(const char *name);

int sb_vote_get(struct sb_vote *vote, int event, sb_data *data);
int sb_vote_get_result(struct sb_vote *vote, sb_data *data);
int _sb_vote_set(struct sb_vote *vote, int event, bool en, sb_data data, const char *fname, int line);
int sb_vote_set_pri(struct sb_vote *vote, int event, int pri);
int sb_vote_refresh(struct sb_vote *vote);
#else
static inline struct sb_vote *sb_vote_create(const struct sb_vote_cfg *vote_cfg, void *pdata, sb_data init_data)
{ return ERR_PTR(SB_VOTE_DISABLE); }
static inline void sb_vote_destroy(struct sb_vote *vote) {}

static inline struct sb_vote *sb_vote_find(const char *name)
{ return ERR_PTR(SB_VOTE_DISABLE); }

static inline int sb_vote_get(struct sb_vote *vote, int event, sb_data *data)
{ return SB_VOTE_DISABLE; }
static inline int sb_vote_get_result(struct sb_vote *vote, sb_data *data)
{ return SB_VOTE_DISABLE; }
static inline int _sb_vote_set(struct sb_vote *vote, int event, bool en, sb_data data, const char *fname, int linee)
{ return SB_VOTE_DISABLE; }
static inline int sb_vote_set_pri(struct sb_vote *vote, int event, int pri)
{ return SB_VOTE_DISABLE; }
static inline int sb_vote_refresh(struct sb_vote *vote)
{ return SB_VOTE_DISABLE; }
#endif

#define sb_vote_set(vote, event, en, value)	_sb_vote_set(vote, event, en, value, __func__, __LINE__)

#endif /* __SB_VOTE_H */

