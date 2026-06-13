/*
 *  sb_pqueue.c
 *  Samsung Mobile Priority Queue
 *
 *  Copyright (C) 2021 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/battery/sb_pqueue.h>

#define MAX_SIZE		1024
#define PQF_USING_ARRAY	(PQF_REMOVE | PQF_PRIORITY)

struct sb_pq_data {
	unsigned index : 32;
	signed priority : 31;
	unsigned enable : 1;

	sb_data *data;
};

struct sb_pqueue {
	struct mutex lock;

	unsigned int flag;

	struct sb_pq_data **heap;
	struct sb_pq_data *data;
	unsigned int size;
	unsigned int count;

	cmp_func cmp;
};

static bool tmp_cmp(sb_data *data1, sb_data *data2) { return true; }
struct sb_pqueue *sb_pq_create(unsigned int flag, unsigned int size, cmp_func cmp)
{
	struct sb_pqueue *pq;
	int ret = -ENOMEM;

	if (size > MAX_SIZE)
		return ERR_PTR(-EINVAL);

	if (!(flag & PQF_USING_ARRAY) && (cmp == NULL))
		return ERR_PTR(-EINVAL);

	pq = kzalloc(sizeof(struct sb_pqueue), GFP_KERNEL);
	if (!pq)
		goto err_alloc_pq;

	pq->heap = kcalloc(size, sizeof(struct sb_pq_data *), GFP_KERNEL);
	if (!pq->heap)
		goto err_alloc_heap;

	pq->data = kcalloc(size, sizeof(struct sb_pq_data), GFP_KERNEL);
	if (!pq->data)
		goto err_alloc_data;

	pq->flag = flag;
	pq->size = size;
	pq->count = 0;
	pq->cmp = (cmp) ? cmp : tmp_cmp;

	mutex_init(&pq->lock);

	return pq;

err_alloc_data:
	kfree(pq->heap);
err_alloc_heap:
	kfree(pq);
err_alloc_pq:
	return ERR_PTR(ret);
}
EXPORT_SYMBOL(sb_pq_create);

void sb_pq_destroy(struct sb_pqueue *pq)
{
	if (pq == NULL)
		return;

	mutex_destroy(&pq->lock);

	if (pq->size > 0) {
		kfree(pq->data);
		kfree(pq->heap);
	}

	kfree(pq);
}
EXPORT_SYMBOL(sb_pq_destroy);

static void swap_pq_data(struct sb_pq_data **heap, unsigned int idx1, unsigned int idx2)
{
	struct sb_pq_data *tmp = heap[idx1];

	heap[idx1] = heap[idx2];
	heap[idx2] = tmp;

	heap[idx1]->index = idx1;
	heap[idx2]->index = idx2;
}

static bool cmp_pq_data(struct sb_pqueue *pq, unsigned int idx1, unsigned int idx2)
{
	struct sb_pq_data *pd1 = pq->heap[idx1], *pd2 = pq->heap[idx2];

	if (pd1->priority == pd2->priority)
		return pq->cmp(pd1->data, pd2->data);

	return (pd1->priority > pd2->priority);
}

static int _sb_pq_up(struct sb_pqueue *pq, unsigned int pos)
{
	unsigned int idx, prt_idx;

	idx = pos;
	prt_idx = (pos - 1) / 2;

	while ((idx > 0) && cmp_pq_data(pq, idx, prt_idx)) {
		swap_pq_data(pq->heap, idx, prt_idx);

		idx = prt_idx;
		prt_idx = (idx - 1) / 2;
	}

	return idx;
}

static int _sb_pq_down(struct sb_pqueue *pq, unsigned int pos)
{
	unsigned int idx, chd_idx;

	idx = pos;
	chd_idx = (pos * 2) + 1;

	while (chd_idx < pq->count) {
		int chd;

		if (chd_idx + 1 == pq->count)
			chd = chd_idx;
		else
			chd = (cmp_pq_data(pq, chd_idx, chd_idx + 1)) ?
				(chd_idx) : (chd_idx + 1);

		if (cmp_pq_data(pq, idx, chd))
			break;

		swap_pq_data(pq->heap, idx, chd);

		idx = chd;
		chd_idx = (idx * 2) + 1;
	}

	return idx;
}

static sb_data *_sb_pq_pop(struct sb_pqueue *pq)
{
	struct sb_pq_data *pd = NULL;

	if (pq->count <= 0)
		return ERR_PTR(-ENOENT);

	pd = pq->heap[0];
	swap_pq_data(pq->heap, 0, --pq->count);
	_sb_pq_down(pq, 0);

	pd->enable = false;
	return pd->data;
}

static int _sb_pq_push(struct sb_pqueue *pq, unsigned int idx, sb_data *data)
{
	struct sb_pq_data *pd = NULL;

	if (pq->size <= pq->count)
		return -ENOMEM;

	pd = &pq->data[idx];
	pd->data = data;
	pd->enable = true;

	pq->heap[pq->count] = pd;
	pd->index = pq->count;

	return _sb_pq_up(pq, pq->count++);
}

static int _sb_pq_remove(struct sb_pqueue *pq, unsigned int idx)
{
	struct sb_pq_data *pd = NULL;
	int ret = 0;

	if (pq->count <= idx)
		return -EINVAL;

	ret = idx;
	if (idx == 0) {
		_sb_pq_pop(pq);
		return 0;
	}

	if (idx == pq->count - 1) {
		pd = pq->heap[--pq->count];
		pd->enable = false;
		return 0;
	}

	pd = pq->heap[idx];
	pd->enable = false;
	swap_pq_data(pq->heap, idx, --pq->count);

	ret = _sb_pq_down(pq, idx);
	ret = _sb_pq_up(pq, idx);
	return ret;
}


sb_data *sb_pq_pop(struct sb_pqueue *pq)
{
	sb_data *data = NULL;

	if (pq == NULL)
		return ERR_PTR(-EINVAL);

	mutex_lock(&pq->lock);
	data = _sb_pq_pop(pq);
	mutex_unlock(&pq->lock);

	return data;
}
EXPORT_SYMBOL(sb_pq_pop);

int sb_pq_push(struct sb_pqueue *pq, unsigned int idx, sb_data *data)
{
	int ret = -ENOMEM;

	if ((pq == NULL) ||
		(idx >= pq->size) ||
		(data == NULL))
		return -EINVAL;

	mutex_lock(&pq->lock);
	if (!(pq->flag & PQF_USING_ARRAY)) {
		idx = 0;
		while ((idx < pq->size) && (pq->data[idx].enable))
			idx++;
	}

	ret = _sb_pq_push(pq, idx, data);
	mutex_unlock(&pq->lock);

	return ret;
}
EXPORT_SYMBOL(sb_pq_push);

int sb_pq_get_en(struct sb_pqueue *pq, unsigned int idx)
{
	if ((pq == NULL) || (idx >= pq->size))
		return -EINVAL;

	if (!(pq->flag & PQF_USING_ARRAY))
		return -EPERM;

	return pq->data[idx].enable;
}
EXPORT_SYMBOL(sb_pq_get_en);

int sb_pq_set_pri(struct sb_pqueue *pq, unsigned int idx, int pri)
{
	struct sb_pq_data *pd = NULL;
	int ret = 0;

	if ((pq == NULL) || (idx >= pq->size))
		return -EINVAL;

	if (!(pq->flag & PQF_PRIORITY))
		return -EPERM;

	mutex_lock(&pq->lock);

	pd = &pq->data[idx];
	if (!pd->enable) {
		pd->priority = pri;
		goto skip_set_pri;
	} else if (pd->priority == pri) {
		goto skip_set_pri;
	}
	pd->priority = pri;

	ret = _sb_pq_remove(pq, pd->index);
	ret = _sb_pq_push(pq, idx, pd->data);

skip_set_pri:
	mutex_unlock(&pq->lock);
	return ret;
}
EXPORT_SYMBOL(sb_pq_set_pri);

int sb_pq_get_pri(struct sb_pqueue *pq, unsigned int idx)
{
	if ((pq == NULL) || (idx >= pq->size))
		return -EINVAL;

	if (!(pq->flag & PQF_PRIORITY))
		return -EPERM;

	return pq->data[idx].priority;
}
EXPORT_SYMBOL(sb_pq_get_pri);

int sb_pq_set_data(struct sb_pqueue *pq, unsigned int idx, sb_data *data)
{
	struct sb_pq_data *pd = NULL;
	int ret = 0;

	if ((pq == NULL) || (idx >= pq->size))
		return -EINVAL;

	if (!(pq->flag & PQF_USING_ARRAY))
		return -EPERM;

	mutex_lock(&pq->lock);

	pd = &pq->data[idx];
	pd->data = data;

	if (!pd->enable)
		goto skip_set_data;

	ret = _sb_pq_remove(pq, pd->index);
	ret = _sb_pq_push(pq, idx, pd->data);

skip_set_data:
	mutex_unlock(&pq->lock);
	return ret;
}
EXPORT_SYMBOL(sb_pq_set_data);

sb_data *sb_pq_get_data(struct sb_pqueue *pq, unsigned int idx)
{
	if ((pq == NULL) || (idx >= pq->size))
		return ERR_PTR(-EINVAL);

	if (!(pq->flag & PQF_USING_ARRAY))
		return ERR_PTR(-EPERM);

	return pq->data[idx].data;
}
EXPORT_SYMBOL(sb_pq_get_data);

sb_data *sb_pq_top(struct sb_pqueue *pq)
{
	sb_data *data = NULL;

	if (pq == NULL)
		return ERR_PTR(-EINVAL);

	mutex_lock(&pq->lock);

	if (pq->count > 0)
		data = pq->heap[0]->data;
	else
		data = ERR_PTR(-ENOENT);

	mutex_unlock(&pq->lock);

	return data;
}
EXPORT_SYMBOL(sb_pq_top);

int sb_pq_remove(struct sb_pqueue *pq, unsigned int idx)
{
	int ret = 0;

	if ((pq == NULL) || (idx >= pq->size))
		return -EINVAL;

	if (!(pq->flag & PQF_REMOVE))
		return -EPERM;

	mutex_lock(&pq->lock);
	ret = _sb_pq_remove(pq, pq->data[idx].index);
	mutex_unlock(&pq->lock);

	return ret;
}
EXPORT_SYMBOL(sb_pq_remove);

