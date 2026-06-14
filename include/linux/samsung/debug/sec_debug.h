#ifndef __SEC_DEBUG_H__
#define __SEC_DEBUG_H__

#include <dt-bindings/samsung/debug/sec_debug.h>

#define SEC_DEBUG_CP_DEBUG_ON		0x5500
#define SEC_DEBUG_CP_DEBUG_OFF		0x55ff

#if IS_ENABLED(CONFIG_SEC_DEBUG)
extern unsigned int sec_debug_level(void);
extern bool sec_debug_is_enabled(void);
extern phys_addr_t sec_debug_get_dump_sink_phys(void);
#else
static inline unsigned int sec_debug_level(void) { return SEC_DEBUG_LEVEL_LOW; }
static inline bool sec_debug_is_enabled(void) { return false; }
static inline phys_addr_t sec_debug_get_dump_sink_phys(void) { return 0; }
#endif

/* FIXME: 'sec_debug_level' and 'sec_debug_is_enabled' will be deprecated in
 * android16-x.x kernel.
 * | As-Is                | To-Be                      |
 * |----------------------+----------------------------|
 * | sec_debug_level      | sec_debug_get_debug_level  |
 * | sec_debug_is_enabled | sec_debug_get_force_upload |
 */

static inline unsigned int sec_debug_get_debug_level(void)
{
	return sec_debug_level();
}

static inline bool sec_debug_get_force_upload(void)
{
	return sec_debug_is_enabled();
}

#endif	/* __SEC_DEBUG_H__ */
