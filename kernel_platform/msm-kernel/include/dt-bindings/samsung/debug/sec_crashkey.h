#ifndef __DT_BINDINGS__SEC_CRASHKEY_H__
#define __DT_BINDINGS__SEC_CRASHKEY_H__

#define EVENT_KEY_PRESS(__keycode)					\
	<__keycode 1>
#define EVENT_KEY_RELEASE(__keycode)					\
	<__keycode 0>
#define EVENT_KEY_PRESS_AND_RELEASE(__keycode)				\
	EVENT_KEY_PRESS(__keycode),					\
	EVENT_KEY_RELEASE(__keycode)

#endif /* __DT_BINDINGS__SEC_CRASHKEY_H__ */
