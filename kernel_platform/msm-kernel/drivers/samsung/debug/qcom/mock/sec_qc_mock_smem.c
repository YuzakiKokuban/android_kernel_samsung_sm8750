// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/hashtable.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mm.h>

struct mock_smem_entry {
	struct hlist_node hlist;
	struct list_head list;
	unsigned item;
	void *virt;
	size_t size;
};

static DEFINE_HASHTABLE(mock_smem_htbl, 3);
static LIST_HEAD(mock_smem_list);

static struct mock_smem_entry *__mock_smem_find(unsigned item)
{
	struct mock_smem_entry *h;

	hash_for_each_possible(mock_smem_htbl, h, hlist, item) {
		if (h->item == item)
			return h;
	}

	return ERR_PTR(-ENOENT);
}

int __weak qcom_smem_alloc(unsigned host, unsigned item, size_t size)
{
	struct mock_smem_entry *h;
	size_t alloc_size;

	h = __mock_smem_find(item);
	if (!IS_ERR_OR_NULL(h))
		return 0;

	h = kzalloc(sizeof(*h), GFP_KERNEL);
	if (!h)
		return -ENOMEM;

	alloc_size = PAGE_ALIGN(size);
	h->virt = kzalloc(alloc_size, GFP_KERNEL);
	if (!h->virt) {
		kfree(h);
		return -ENOMEM;
	}

	INIT_HLIST_NODE(&h->hlist);
	h->size = alloc_size;
	h->item = item;
	hash_add(mock_smem_htbl, &h->hlist, item);
	list_add(&h->list, &mock_smem_list);

	return 0;
}
EXPORT_SYMBOL_GPL(qcom_smem_alloc);

void * __weak qcom_smem_get(unsigned host, unsigned item, size_t *size)
{
	struct mock_smem_entry *h;

	h = __mock_smem_find(item);
	if (IS_ERR_OR_NULL(h))
		return ERR_PTR(-ENOENT);

	*size = h->size;

	return h->virt;
}
EXPORT_SYMBOL_GPL(qcom_smem_get);

phys_addr_t __weak qcom_smem_virt_to_phys(void *p)
{
	return virt_to_phys(p);
}
EXPORT_SYMBOL_GPL(qcom_smem_virt_to_phys);

void __init __qc_mock_smem_init(void)
{
	hash_init(mock_smem_htbl);
	INIT_LIST_HEAD(&mock_smem_list);
}

void __exit __qc_mock_smem_exit(void)
{
	struct mock_smem_entry *pos;
	struct mock_smem_entry *tmp;

	list_for_each_entry_safe(pos, tmp, &mock_smem_list, list) {
		list_del(&pos->list);
		kfree(pos);
	}
}
