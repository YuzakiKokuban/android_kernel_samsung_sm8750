#ifndef __SEC_REBOOT_CMD_H__
#define __SEC_REBOOT_CMD_H__

#include <linux/list.h>

enum sec_rbcmd_stage {
	SEC_RBCMD_STAGE_1ST = 0,
	SEC_RBCMD_STAGE_REBOOT_NOTIFIER = 0,
	SEC_RBCMD_STAGE_RESTART_HANDLER,
	SEC_RBCMD_STAGE_MAX,
};

/* similar to NOTIFY_flags */
#define SEC_RBCMD_HANDLE_STOP_MASK	0x80000000	/* Don't call further */
#define SEC_RBCMD_HANDLE_ONESHOT_MASK	0x40000000	/* Don't call further during handling multi cmd */
#define SEC_RBCMD_HANDLE_DONE		0x00000000	/* Don't care */
#define __SEC_RBCMD_HANDLE_OK		0x00000001	/* OK. It's handled by me. */
#define SEC_RBCMD_HANDLE_OK		(SEC_RBCMD_HANDLE_STOP_MASK | __SEC_RBCMD_HANDLE_OK)
#define __SEC_RBCMD_HANDLE_BAD		0x00000002	/* Bad. It's mine but some error occur. */
#define SEC_RBCMD_HANDLE_BAD		(SEC_RBCMD_HANDLE_STOP_MASK | __SEC_RBCMD_HANDLE_BAD)

struct sec_reboot_param {
	const char *cmd;
	unsigned long mode;
};

struct sec_reboot_cmd {
	struct list_head list;
	const char *cmd;
	int (*func)(const struct sec_reboot_cmd *, struct sec_reboot_param *, bool);
};

#if IS_ENABLED(CONFIG_SEC_REBOOT_CMD)
extern int sec_rbcmd_add_cmd(enum sec_rbcmd_stage s, struct sec_reboot_cmd *rc);
extern int sec_rbcmd_del_cmd(enum sec_rbcmd_stage s, struct sec_reboot_cmd *rc);

extern int sec_rbcmd_set_default_cmd(enum sec_rbcmd_stage s, struct sec_reboot_cmd *rc);
extern int sec_rbcmd_unset_default_cmd(enum sec_rbcmd_stage s, struct sec_reboot_cmd *rc);
#else
static inline int sec_rbcmd_add_cmd(enum sec_rbcmd_stage s, struct sec_reboot_cmd *rc) { return -ENODEV; }
static inline int sec_rbcmd_del_cmd(enum sec_rbcmd_stage s, struct sec_reboot_cmd *rc) { return -ENODEV; }

static inline int sec_rbcmd_set_default_cmd(enum sec_rbcmd_stage s, struct sec_reboot_cmd *rc) { return -ENODEV; }
static inline int sec_rbcmd_unset_default_cmd(enum sec_rbcmd_stage s, struct sec_reboot_cmd *rc) { return -ENODEV; }
#endif

#endif /* __SEC_REBOOT_CMD_H__ */
