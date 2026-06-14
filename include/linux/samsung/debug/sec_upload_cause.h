#ifndef __SEC_UPLOAD_CAUSE_H__
#define __SEC_UPLOAD_CAUSE_H__

/* similar to NOTIFY_flags */
#define SEC_UPLOAD_CAUSE_HANDLE_STOP_MASK	0x80000000	/* Don't call further */
#define SEC_UPLOAD_CAUSE_HANDLE_DONE		0x00000000	/* Don't care */
#define __SEC_UPLOAD_CAUSE_HANDLE_OK		0x00000001	/* OK. It's handled by me. */
#define SEC_UPLOAD_CAUSE_HANDLE_OK		(SEC_UPLOAD_CAUSE_HANDLE_STOP_MASK | __SEC_UPLOAD_CAUSE_HANDLE_OK)
#define __SEC_UPLOAD_CAUSE_HANDLE_BAD		0x00000002	/* Bad. It's mine but some error occur. */
#define SEC_UPLOAD_CAUSE_HANDLE_BAD		(SEC_UPLOAD_CAUSE_HANDLE_STOP_MASK | __SEC_UPLOAD_CAUSE_HANDLE_BAD)

struct sec_upload_cause {
	struct list_head list;
	const char *cause;
	unsigned int type;
	int (*func)(const struct sec_upload_cause *, const char *);
};

#if IS_ENABLED(CONFIG_SEC_UPLOAD_CAUSE)
extern int sec_upldc_add_cause(struct sec_upload_cause *uc);
extern int sec_upldc_del_cause(struct sec_upload_cause *uc);

extern int sec_upldc_set_default_cause(struct sec_upload_cause *uc);
extern int sec_upldc_unset_default_cause(struct sec_upload_cause *uc);

extern void sec_upldc_type_to_cause(unsigned int type, char *cause, size_t len);
#else
static inline int sec_upldc_add_cause(struct sec_upload_cause *uc) { return -ENODEV; }
static inline int sec_upldc_del_cause(struct sec_upload_cause *uc) { return -ENODEV; }

static inline int sec_upldc_set_default_cause(struct sec_upload_cause *uc) { return -ENODEV; }
static inline int sec_upldc_unset_default_cause(struct sec_upload_cause *uc) { return -ENODEV; }

static inline void sec_upldc_type_to_cause(unsigned int type, char *cause, size_t len) {}
#endif

#endif /* #define __SEC_UPLOAD_CAUSE_H__ */
