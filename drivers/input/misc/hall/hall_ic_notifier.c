/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2019 Samsung Electronics
 */

#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
#include <linux/hall/hall_ic_notifier.h>
#include <linux/module.h>

static struct hall_notifier_context hall_notifier;
static struct blocking_notifier_head hall_nb_head = BLOCKING_NOTIFIER_INIT(hall_nb_head);

int hall_notifier_register(struct notifier_block *n)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	ret = blocking_notifier_chain_register(&hall_nb_head, n);
	if (ret < 0)
		pr_err("%s: failed(%d)\n", __func__, ret);

	return ret;
}
EXPORT_SYMBOL(hall_notifier_register);

int hall_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	ret = blocking_notifier_chain_unregister(&hall_nb_head, nb);
	if (ret < 0)
		pr_err("%s: failed(%d)\n", __func__, ret);

	return ret;
}
EXPORT_SYMBOL(hall_notifier_unregister);

int hall_notifier_notify(const char *hall_name, int hall_value)
{
	int ret = 0;

	pr_info("%s: name: %s value: %d\n", __func__, hall_name, hall_value);

	hall_notifier.name = hall_name;
	hall_notifier.value = hall_value;

	ret = blocking_notifier_call_chain(&hall_nb_head, hall_value, &hall_notifier);

	switch (ret) {
	case NOTIFY_DONE:
	case NOTIFY_OK:
		pr_info("%s done(0x%x)\n", __func__, ret);
		break;
	default:
		pr_info("%s failed(0x%x)\n", __func__, ret);
		break;
	}

	return ret;
}
EXPORT_SYMBOL(hall_notifier_notify);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hall IC notifier");

#endif /* if IS_ENABLED(CONFIG_HALL_NOTIFIER) */
