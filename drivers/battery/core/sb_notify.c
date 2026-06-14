/*
 *  sb_notify.c
 *  Samsung Mobile Battery NotifY
 *
 *  Copyright (C) 2021 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/device.h>
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
#include <linux/sec_class.h>
#endif
#include <linux/battery/sb_notify.h>

#define SB_NOTIFY_NAME	"sb-notify"

struct sbn_dev {
	const char *name;
	enum sb_dev_type type;

	struct notifier_block *nb;
	struct list_head list;
};

struct sbn {
	struct blocking_notifier_head nb_head;

	struct list_head dev_list;
	unsigned int dev_count;
};

static struct sbn sb_notify;
static BLOCKING_NOTIFIER_HEAD(ocp_warn_chain_head);

struct device *sb_device;
EXPORT_SYMBOL(sb_device);

static DEFINE_MUTEX(noti_lock);

static struct sbn_dev *sbn_create_device(const char *name, enum sb_dev_type type)
{
	struct sbn_dev *ndev;

	ndev = kzalloc(sizeof(struct sbn_dev), GFP_KERNEL);
	if (!ndev)
		return NULL;

	ndev->name = name;
	ndev->type = type;
	return ndev;
}

static void sbn_destroy_device(struct sbn_dev *ndev)
{
	kfree(ndev);
}

static struct sbn_dev *sbn_find_device(struct notifier_block *nb)
{
	struct sbn_dev *ndev = NULL;

	if (!nb)
		return NULL;

	mutex_lock(&noti_lock);

	list_for_each_entry(ndev, &sb_notify.dev_list, list) {
		if (ndev->nb == nb)
			break;
	}

	mutex_unlock(&noti_lock);

	return ndev;
}

static int sb_notify_init(void)
{
	static bool init_state;
	int ret = 0;

	mutex_lock(&noti_lock);

	if (init_state)
		goto skip_init;

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	sb_device = sec_device_create(NULL, SB_NOTIFY_NAME);
	if (IS_ERR(sb_device)) {
		pr_err("%s Failed to create device(%s)!\n", __func__, SB_NOTIFY_NAME);
		ret = -ENODEV;
	}
#endif

	BLOCKING_INIT_NOTIFIER_HEAD(&(sb_notify.nb_head));

	INIT_LIST_HEAD(&sb_notify.dev_list);

	init_state = true;
	pr_info("%s: init = %d\n", __func__, init_state);

skip_init:
	mutex_unlock(&noti_lock);
	return ret;
}

int sb_notify_call(enum sbn_type ntype, sb_data *ndata)
{
	int ret = 0;

	if (ntype <= SB_NOTIFY_UNKNOWN || ntype >= SB_NOTIFY_MAX)
		return -EINVAL;

	mutex_lock(&noti_lock);

	ret = blocking_notifier_call_chain(&(sb_notify.nb_head),
		ntype, ndata);

	mutex_unlock(&noti_lock);

	return 0;
}
EXPORT_SYMBOL(sb_notify_call);

int sb_notify_register(struct notifier_block *nb, notifier_fn_t notifier,
		const char *name, enum sb_dev_type type)
{
	struct sbn_dev *ndev;
	int ret = 0;

	if (!nb || !notifier || !name)
		return -EINVAL;

	if (sb_notify_init() < 0)
		return -ENOENT;

	ndev = sbn_create_device(name, type);
	if (!ndev)
		return -ENOMEM;

	nb->notifier_call = notifier;
	nb->priority = 0;
	ndev->nb = nb;

	ret = sb_notify_call(SB_NOTIFY_DEV_PROBE, cast_to_sb_pdata(name));
	if (ret < 0)
		pr_err("%s: failed to broadcast dev_probe, ret = %d\n",
			__func__, ret);

	mutex_lock(&noti_lock);

	ret = blocking_notifier_chain_register(&(sb_notify.nb_head), nb);
	if (ret < 0) {
		pr_err("%s: failed to register nb(%s, %d)",
			__func__, name, ret);
		sbn_destroy_device(ndev);
		goto skip_register;
	}

	if (sb_notify.dev_count > 0) {
		struct sbn_dev_list dev_list;
		struct sbn_dev *tmp_dev;

		dev_list.list = kcalloc(sb_notify.dev_count, sizeof(char *), GFP_KERNEL);
		if (dev_list.list) {
			int i = 0;

			list_for_each_entry(tmp_dev, &sb_notify.dev_list, list) {
				dev_list.list[i++] = tmp_dev->name;
			}

			dev_list.count = sb_notify.dev_count;
			nb->notifier_call(nb, SB_NOTIFY_DEV_LIST, cast_to_sb_pdata(&dev_list));

			kfree(dev_list.list);
		}
	}

	list_add(&ndev->list, &sb_notify.dev_list);
	sb_notify.dev_count++;

skip_register:
	mutex_unlock(&noti_lock);
	return ret;
}
EXPORT_SYMBOL(sb_notify_register);

int sb_notify_unregister(struct notifier_block *nb)
{
	struct sbn_dev *ndev;
	int ret = 0;

	ndev = sbn_find_device(nb);
	if (!ndev)
		return -ENODEV;

	mutex_lock(&noti_lock);

	list_del(&ndev->list);
	sb_notify.dev_count--;

	ret = blocking_notifier_chain_unregister(&sb_notify.nb_head, nb);
	if (ret < 0)
		pr_err("%s: failed to unregister nb(%s, %d)",
			__func__, ndev->name, ret);

	nb->notifier_call = NULL;
	nb->priority = -1;

	mutex_unlock(&noti_lock);

	ret = sb_notify_call(SB_NOTIFY_DEV_SHUTDOWN, (void *)ndev->name);
	kfree(ndev);
	return ret;
}
EXPORT_SYMBOL(sb_notify_unregister);

int register_ocp_warn_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&ocp_warn_chain_head, nb);
}
EXPORT_SYMBOL_GPL(register_ocp_warn_notifier);

int unregister_ocp_warn_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&ocp_warn_chain_head, nb);
}
EXPORT_SYMBOL_GPL(unregister_ocp_warn_notifier);

int ocp_warn_notifier_call_chain(unsigned long val)
{
	return blocking_notifier_call_chain(&ocp_warn_chain_head, val, NULL);
}
EXPORT_SYMBOL(ocp_warn_notifier_call_chain);