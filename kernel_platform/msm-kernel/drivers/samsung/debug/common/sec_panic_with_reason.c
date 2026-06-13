// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include "sec_debug.h"

#define MAX_BUF_SIZE 64

static ssize_t panic_with_reason_trigger(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	ssize_t ret;
	char panicstr[MAX_BUF_SIZE];

	if (count > MAX_BUF_SIZE)
		return -EINVAL;

	/* copy data to kernel space from user space */
	ret = simple_write_to_buffer(panicstr, sizeof(panicstr), ppos, buf, count);
	if (ret < 0)
		return ret;

	panicstr[ret] = '\0';

	panic("%s", panicstr);

	return count;
}

static const struct file_operations panic_with_reason_fops = {
	.write = panic_with_reason_trigger,
	.open = simple_open,
	.llseek = default_llseek,
};


#if IS_ENABLED(CONFIG_DEBUG_FS)
static int __panic_with_reason_init(struct builder *bd)
{
	struct sec_debug_drvdata *drvdata =
			container_of(bd, struct sec_debug_drvdata, bd);

	drvdata->dbgfs_panic = debugfs_create_file("panic_with_reason", 0222,
			NULL, NULL, &panic_with_reason_fops);

	return 0;
}

static void __panic_with_reason_exit(struct builder *bd)
{
	struct sec_debug_drvdata *drvdata =
			container_of(bd, struct sec_debug_drvdata, bd);

	debugfs_remove(drvdata->dbgfs_panic);
}
#else
static int __panic_with_reason_init(struct builder *bd)
{
	return 0;
}

static void __panic_with_reason_exit(struct builder *bd)
{
}
#endif

int sec_panic_with_reason_init(struct builder *bd)
{
	if (!IS_ENABLED(CONFIG_SEC_FACTORY))
		return 0;

	return __panic_with_reason_init(bd);
}

void sec_panic_with_reason_exit(struct builder *bd)
{
	if (!IS_ENABLED(CONFIG_SEC_FACTORY))
		return;

	__panic_with_reason_exit(bd);
}
