#ifndef __INTERNAL__SEC_REBOOT_CMD_H__
#define __INTERNAL__SEC_REBOOT_CMD_H__

#include <linux/debugfs.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/notifier.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_reboot_cmd.h>

struct reboot_cmd_stage {
	struct mutex lock;
	struct list_head list;
	struct sec_reboot_cmd *dflt;
	struct notifier_block nb;
};

struct reboot_cmd_drvdata {
	struct builder bd;
	struct reboot_cmd_stage stage[SEC_RBCMD_STAGE_MAX];
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct dentry *dbgfs;
#endif
};

#endif // __INTERNAL__SEC_REBOOT_CMD_H__
