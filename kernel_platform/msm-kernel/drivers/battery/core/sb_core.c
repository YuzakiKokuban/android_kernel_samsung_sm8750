/*
 *  sb_core.c
 *  Samsung Mobile Battery Core
 *
 *  Copyright (C) 2021 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/battery/sb_def.h>

static int __init sb_core_init(void)
{
	pr_info("%s:\n", __func__);
	return 0;
}
module_init(sb_core_init);

static void __exit sb_core_exit(void)
{
}
module_exit(sb_core_exit);

MODULE_DESCRIPTION("Samsung Battery Core");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
