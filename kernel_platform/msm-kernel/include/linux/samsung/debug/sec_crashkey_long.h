#ifndef __SEC_CRASHKEY_LONG_H__
#define __SEC_CRASHKEY_LONG_H__

#define SEC_CRASHKEY_LONG_NOTIFY_TYPE_MATCHED	0
#define SEC_CRASHKEY_LONG_NOTIFY_TYPE_UNMATCHED	1
#define SEC_CRASHKEY_LONG_NOTIFY_TYPE_EXPIRED	2

#if IS_ENABLED(CONFIG_SEC_CRASHKEY_LONG)
extern int sec_crashkey_long_add_preparing_panic(struct notifier_block *nb);
extern int sec_crashkey_long_del_preparing_panic(struct notifier_block *nb);

extern int sec_crashkey_long_connect_to_input_evnet(void);
extern int sec_crashkey_long_disconnect_from_input_event(void);
#else
static inline int sec_crashkey_long_add_preparing_panic(struct notifier_block *nb) { return -ENODEV; }
static inline int sec_crashkey_long_del_preparing_panic(struct notifier_block *nb) { return -ENODEV; }

static inline int sec_crashkey_long_connect_to_input_evnet(void) { return -ENODEV; }
static inline int sec_crashkey_long_disconnect_from_input_event(void) { return -ENODEV; }
#endif

#endif /* __SEC_CRASHKEY_LONG_H__ */
