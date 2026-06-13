#ifndef __SEC_DEBUG_REGION_H__
#define __SEC_DEBUG_REGION_H__

/* crc32 <(echo -n "SEC_DBG_REGION_ROOT_MAGIC") | tr '[a-z]' '[A-Z]' */
#define SEC_DBG_REGION_ROOT_MAGIC	0xC317F7EF
/* crc32 <(echo -n "SEC_DBG_REGION_CLIENT_MAGIC") | tr '[a-z]' '[A-Z]' */
#define SEC_DBG_REGION_CLIENT_MAGIC	0x3EF44F24

struct sec_dbg_region_client {
	struct list_head list;
	uint32_t magic;
	uint32_t unique_id;
	phys_addr_t __client;	/* physical address of myself */
	phys_addr_t phys;
	size_t size;
	unsigned long virt;
	const char *name;
} __packed __aligned(1);

#if IS_ENABLED(CONFIG_SEC_DEBUG_REGION)
extern struct sec_dbg_region_client *sec_dbg_region_alloc(uint32_t unique_id, size_t size);
extern int sec_dbg_region_free(struct sec_dbg_region_client *client);

extern const struct sec_dbg_region_client *sec_dbg_region_find(uint32_t unique_id);
#else
static inline struct sec_dbg_region_client *sec_dbg_region_alloc(uint32_t unique_id, size_t size) { return ERR_PTR(-ENODEV); }
static inline int sec_dbg_region_free(struct sec_dbg_region_client *client) { return -ENODEV; }

static inline const struct sec_dbg_region_client *sec_dbg_region_find(uint32_t unique_id) { return ERR_PTR(-ENODEV); }
#endif

#endif /* __SEC_DEBUG_REGION_H__ */
