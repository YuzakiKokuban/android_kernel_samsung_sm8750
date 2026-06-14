// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2020-2021 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#ifndef __OF_EARLY_POPULATE_H__
#define __OF_EARLY_POPULATE_H__

/* FIXME: This is tricky! */
/* NOTE: A helper function to enable the device before
 * 'of_platform_default_populate' is called.
 * The idea is inspired from 'of_platform_default_populate_init' that
 * this function calls a 'of_platform_device_create' before calling
 * 'of_platform_default_populate_init' to make some devices earlier.
 */
static __always_inline int __of_platform_early_populate_init(
		const struct of_device_id *matches)
{
#if IS_BUILTIN(CONFIG_SEC_DEBUG)
	struct device_node *np;
	struct platform_device *pdev;

	np = of_find_matching_node(NULL, matches);
	if (!np)
		return -EINVAL;

	pdev = of_platform_device_create(np, NULL, NULL);
	if (!pdev)
		return -ENODEV;
#endif

	return 0;
}

#endif /* __OF_EARLY_POPULATE_H__ */
