#ifndef __SEC_KEY_NOTIFIER_H__
#define __SEC_KEY_NOTIFIER_H__

#include <linux/notifier.h>

struct sec_key_notifier_param {
	unsigned int keycode;
	int down;
};

#define SEC_KEY_EVENT_KEY_PRESS(__keycode) \
        { .keycode = __keycode, .down = 1, }
#define SEC_KEY_EVENT_KEY_RELEASE(__keycode) \
        { .keycode = __keycode, .down = 0, }
#define SEC_KEY_EVENT_KEY_PRESS_AND_RELEASE(__keycode) \
        SEC_KEY_EVENT_KEY_PRESS(__keycode), \
        SEC_KEY_EVENT_KEY_RELEASE(__keycode)

#if IS_ENABLED(CONFIG_SEC_KEY_NOTIFIER)
extern int sec_kn_register_notifier(struct notifier_block *nb, const unsigned int *events, const size_t nr_events);
extern int sec_kn_unregister_notifier(struct notifier_block *nb, const unsigned int *events, const size_t nr_events);
#else
static inline int sec_kn_register_notifier(struct notifier_block *nb, const unsigned int *events, const size_t nr_events) { return 0; }
static inline int sec_kn_unregister_notifier(struct notifier_block *nb, const unsigned int *events, const size_t nr_events) { return 0; }
#endif

#endif /* __SEC_KEY_NOTIFIER_H__ */
