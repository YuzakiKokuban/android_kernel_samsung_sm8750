#ifndef	_RBIN_REGION_H
#define	_RBIN_REGION_H

#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/genalloc.h>
#include <linux/highmem.h>
#include "page_pool.h"

#define E_NOREGION 55 // returned when region is disabled

struct rr_handle {
	int pool_id;	/* Pool index         : corresponds to filesystem */
	int rb_index;	/* Redblack tree index: value equals inode number */
	int ra_index;	/* Radix tree index   : value equals page->index  */
	int usage;	/* indicates current usage of corresponding page  */
	struct list_head lru;		/* use lru of struct page instead */
};

enum usage {
	RC_FREED,
	RC_INUSE,
	DMABUF_FREED,
	DMABUF_INUSE,
};

struct region_ops {
	void (*evict)(unsigned long raw_handle);
};

struct rbin_region {
	unsigned long start_pfn;
	unsigned long nr_pages;
	struct rr_handle *handles;
	struct list_head freelist;	/* protected by lru_lock. handle->lru */
	struct list_head usedlist;	/* protected by lru_lock. handle->lru */
	const struct region_ops *ops;

	spinlock_t lru_lock;
	spinlock_t region_lock;
	struct gen_pool *pool;
	int dmabuf_inflight;
	int rc_inflight;
	bool rc_disabled;
	unsigned long timeout;		/* timeout for rbincache enable */
};

extern atomic_t rbin_alloced_pages;
extern atomic_t rbin_cached_pages;
extern atomic_t rbin_free_pages;
extern atomic_t rbin_pool_pages;

struct rr_handle *region_store_cache(struct page *src, int pool_id,
				     int rb_index, int ra_index);
int region_load_cache(struct rr_handle *handle, struct page *dst,
		      int pool_id, int rb_index, int ra_index);
int region_flush_cache(struct rr_handle *handle);
phys_addr_t dmabuf_rbin_allocate(unsigned long size);
void dmabuf_rbin_free(phys_addr_t addr, unsigned long size);
void wake_dmabuf_rbin_heap_shrink(void);
void init_region(unsigned long pfn, unsigned long nr_pages,
		const struct region_ops *ops);
int init_rbincache(struct kobject *kobj,
		   unsigned long pfn, unsigned long nr_pages);
int init_rbinregion(struct kobject *kobj,
		    unsigned long base, unsigned long size);

#endif	/* _RBIN_REGION_H*/
