/*
 * sb_pqueue.h
 * Samsung Mobile Priority Queue Header
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

#ifndef __SB_PQUEUE_H
#define __SB_PQUEUE_H __FILE__

#include <linux/err.h>
#include <linux/battery/sb_def.h>

#define PQF_REMOVE		0x1
#define PQF_PRIORITY	0x2

struct sb_pqueue;

/* priority condition : data1 > data2 = true, data1 < data2 = false */
typedef bool (*cmp_func)(sb_data *data1, sb_data *data2);

#define SB_PQUEUE_DISABLE	(-3660)
#if IS_ENABLED(CONFIG_SB_PQUEUE)
struct sb_pqueue *sb_pq_create(unsigned int flag, unsigned int size, cmp_func cmp);
void sb_pq_destroy(struct sb_pqueue *pq);

sb_data *sb_pq_pop(struct sb_pqueue *pq);
int sb_pq_push(struct sb_pqueue *pq, unsigned int idx, sb_data *data);

int sb_pq_get_en(struct sb_pqueue *pq, unsigned int idx);

int sb_pq_set_pri(struct sb_pqueue *pq, unsigned int idx, int pri);
int sb_pq_get_pri(struct sb_pqueue *pq, unsigned int idx);

int sb_pq_set_data(struct sb_pqueue *pq, unsigned int idx, sb_data *data);
sb_data *sb_pq_get_data(struct sb_pqueue *pq, unsigned int idx);

sb_data *sb_pq_top(struct sb_pqueue *pq);
int sb_pq_remove(struct sb_pqueue *pq, unsigned int idx);
#else
static inline struct sb_pqueue *sb_pq_create(unsigned int flag, unsigned int size, cmp_func cmp)
{ return ERR_PTR(SB_PQUEUE_DISABLE); }
static inline void sb_pq_destroy(struct sb_pqueue *pq) {}

static inline sb_data *sb_pq_pop(struct sb_pqueue *pq)
{ return ERR_PTR(SB_PQUEUE_DISABLE); }
static inline int sb_pq_push(struct sb_pqueue *pq, unsigned int idx, sb_data *data)
{ return SB_PQUEUE_DISABLE; }

static inline int sb_pq_get_en(struct sb_pqueue *pq, unsigned int idx)
{ return SB_PQUEUE_DISABLE; }

static inline int sb_pq_set_pri(struct sb_pqueue *pq, unsigned int idx, int pri)
{ return SB_PQUEUE_DISABLE; }
static inline int sb_pq_get_pri(struct sb_pqueue *pq, unsigned int idx)
{ return SB_PQUEUE_DISABLE; }

static inline int sb_pq_set_data(struct sb_pqueue *pq, unsigned int idx, sb_data *data)
{ return SB_PQUEUE_DISABLE; }
static inline sb_data *sb_pq_get_data(struct sb_pqueue *pq, unsigned int idx)
{ return ERR_PTR(SB_PQUEUE_DISABLE); }

static inline sb_data *sb_pq_top(struct sb_pqueue *pq)
{ return ERR_PTR(SB_PQUEUE_DISABLE); }
static inline int sb_pq_remove(struct sb_pqueue *pq, unsigned int idx)
{ return SB_PQUEUE_DISABLE; }
#endif

#endif /* __SB_PQUEUE_H */

