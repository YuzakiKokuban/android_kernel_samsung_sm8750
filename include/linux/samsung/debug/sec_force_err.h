#ifndef __SEC_FORCE_ERR_H__
#define __SEC_FORCE_ERR_H__

struct force_err_handle {
	struct hlist_node node;
	const char *val;
	const char *msg;
	void (*func)(struct force_err_handle *);
};

#define FORCE_ERR_HANDLE(__val, __msg, __func) \
	{ .val = __val, .msg = __msg, .func = __func }

#if IS_ENABLED(CONFIG_SEC_FORCE_ERR)
extern int sec_force_err_add_custom_handle(struct force_err_handle *h);
extern int sec_force_err_del_custom_handle(struct force_err_handle *h);
#else
static inline int sec_force_err_add_custom_handle(struct force_err_handle *h) { return -ENODEV; }
static inline int sec_force_err_del_custom_handle(struct force_err_handle *h) { return -ENODEV; }
#endif

#endif /* __SEC_FORCE_ERR_H__ */
