#ifndef __INTERNAL_SEC_QC_RST_EXINFO_DIE_H__
#define __INTERNAL_SEC_QC_RST_EXINFO_DIE_H__

enum {
	FAULT_HANDLER_do_bad,
	FAULT_HANDLER_do_translation_fault,
	FAULT_HANDLER_do_page_fault,
	FAULT_HANDLER_do_tag_check_fault,
	FAULT_HANDLER_do_sea,
	FAULT_HANDLER_do_alignment_fault,
	FAULT_HANDLER_early_brk64,
	/* */
	FAULT_HANDLER_UNKNOWN,
};

struct msg_to_fault_handler {
	struct hlist_node node;
	const char *msg;
	size_t msg_len;
	const char *handler_name;
	unsigned int handler_code;
};

#endif /* __INTERNAL_SEC_QC_RST_EXINFO_DIE_H__ */
