#ifndef __SEC_CRASHKEY_H__
#define __SEC_CRASHKEY_H__

#if IS_ENABLED(CONFIG_SEC_CRASHKEY)
extern int sec_crashkey_add_preparing_panic(struct notifier_block *nb, const char *name);
extern int sec_crashkey_del_preparing_panic(struct notifier_block *nb, const char *name);
#else
static inline int sec_crashkey_add_preparing_panic(struct notifier_block *nb, const char *name) { return -ENODEV; }
static inline int sec_crashkey_del_preparing_panic(struct notifier_block *nb, const char *name) { return -ENODEV; }
#endif

#endif /* __SEC_CRASHKEY_H__ */
