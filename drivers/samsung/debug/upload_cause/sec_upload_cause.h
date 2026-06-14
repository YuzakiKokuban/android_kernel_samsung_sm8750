#ifndef __INTERNAL__SEC_UPLOAD_CAUSE_H__
#define __INTERNAL__SEC_UPLOAD_CAUSE_H__

#include <linux/debugfs.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/notifier.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_upload_cause.h>

struct upload_cause_notify {
	struct mutex lock;
	struct list_head list;
	struct sec_upload_cause *dflt;
	struct notifier_block nb;
};

struct upload_cause_drvdata {
	struct builder bd;
	struct upload_cause_notify notify;
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct dentry *dbgfs;
#endif
};

#endif /* __INTERNAL__SEC_UPLOAD_CAUSE_H__ */
