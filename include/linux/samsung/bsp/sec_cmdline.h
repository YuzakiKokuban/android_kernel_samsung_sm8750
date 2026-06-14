#ifndef __SEC_CMDLINE_H__
#define __SEC_CMDLINE_H__

#if IS_ENABLED(CONFIG_SEC_CMDLINE)
extern const char *sec_cmdline_get_val(const char *param);
#else
static inline char *sec_cmdline_get_val(const char *param) { return NULL; }
#endif

#endif /* __SEC_CMDLINE_H__ */
