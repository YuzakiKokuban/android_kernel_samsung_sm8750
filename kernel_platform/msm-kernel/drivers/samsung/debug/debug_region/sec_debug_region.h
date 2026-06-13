#ifndef __INTERNAL__SEC_DEBUG_REGION_H__
#define __INTERNAL__SEC_DEBUG_REGION_H__

#include <linux/debugfs.h>
#include <linux/list.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>

#include <linux/samsung/builder_pattern.h>

struct dbg_region_drvdata;

struct dbg_region_pool {
	int (*probe)(struct dbg_region_drvdata *);
	void (*remove)(struct dbg_region_drvdata *);
	void *(*alloc)(struct dbg_region_drvdata *, size_t, phys_addr_t *);
	void (*free)(struct dbg_region_drvdata *, size_t, void *, phys_addr_t);
};

struct dbg_region_root {
	struct list_head clients;
	uint32_t magic;
	phys_addr_t __root;	/* physical address of myself */
} __packed __aligned(1);

enum {
	RMEM_TYPE_NOMAP = 0,
	RMEM_TYPE_MAPPED,
	RMEM_TYPE_REUSABLE,
	RMEM_TYPE_SLAB,
};

struct dbg_region_drvdata {
	struct builder bd;
	struct reserved_mem *rmem;
	unsigned int rmem_type;
	phys_addr_t phys;
	size_t size;
	struct mutex lock;
	unsigned long virt;
	const struct dbg_region_pool *pool;
	void *private;
	struct dbg_region_root *root;
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct dentry *dbgfs;
#endif
};

/* sec_debug_pool.c */
extern int __dbg_region_pool_init(struct builder *bd);
extern void __dbg_region_pool_exit(struct builder *bd);

/* sec_debug_region_gen_pool.c */
extern const struct dbg_region_pool *__dbg_region_gen_pool_creator(void);

/* sec_debug_region_cma_pool.c */
extern const struct dbg_region_pool *__dbg_region_cma_pool_creator(void);

/* sec_debug_region_slab_pool.c */
extern const struct dbg_region_pool *__dbg_region_slab_pool_creator(void);

#endif /* __INTERNAL__SEC_DEBUG_REGION_H__ */
