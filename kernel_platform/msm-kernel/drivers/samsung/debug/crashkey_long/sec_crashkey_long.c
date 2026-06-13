// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2019-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/bitmap.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeup.h>
#include <linux/slab.h>

#include <linux/samsung/of_early_populate.h>
#include <linux/samsung/debug/sec_debug.h>
#include <linux/samsung/debug/sec_crashkey_long.h>
#include <linux/samsung/sec_kunit.h>

#include "sec_crashkey_long.h"

static struct crashkey_long_drvdata *crashkey_long;

static __always_inline bool __crashkey_long_is_probed(void)
{
	return !!crashkey_long;
}

__ss_static int __crashkey_long_add_preparing_panic(
		struct crashkey_long_drvdata *drvdata, struct notifier_block *nb)
{
	struct crashkey_long_notify *notify;
	int err;

	notify = &drvdata->notify;

	err = raw_notifier_chain_register(&notify->list, nb);
	if (err) {
		struct device *dev = drvdata->bd.dev;

		dev_warn(dev, "failed to add a notifier!\n");
		dev_warn(dev, "Caller is %pS\n", __builtin_return_address(0));
	}

	return err;
}

int sec_crashkey_long_add_preparing_panic(struct notifier_block *nb)
{
	if (!__crashkey_long_is_probed())
		return -EBUSY;

	return __crashkey_long_add_preparing_panic(crashkey_long, nb);
}
EXPORT_SYMBOL_GPL(sec_crashkey_long_add_preparing_panic);

__ss_static int __crashkey_long_del_preparing_panic(
		struct crashkey_long_drvdata *drvdata, struct notifier_block *nb)
{
	struct crashkey_long_notify *notify;
	int err;

	notify = &drvdata->notify;

	err = raw_notifier_chain_unregister(&notify->list, nb);
	if (err) {
		struct device *dev = drvdata->bd.dev;

		dev_warn(dev, "failed to remove a notifier for!\n");
		dev_warn(dev, "Caller is %pS\n", __builtin_return_address(0));
	}

	return err;
}

int sec_crashkey_long_del_preparing_panic(struct notifier_block *nb)
{
	if (!__crashkey_long_is_probed())
		return -EPROBE_DEFER;

	return __crashkey_long_del_preparing_panic(crashkey_long, nb);
}
EXPORT_SYMBOL_GPL(sec_crashkey_long_del_preparing_panic);

static inline int __crashkey_long_connect_to_input_evnet(void)
{
	struct crashkey_long_keylog *keylog;
	unsigned long flags;
	int ret = 0;

	keylog = &crashkey_long->keylog;

	spin_lock_irqsave(&crashkey_long->state_lock, flags);

	if (crashkey_long->nb_connected)
		goto already_connected;

	ret = sec_kn_register_notifier(&crashkey_long->nb,
			keylog->used_key, keylog->nr_used_key);

	crashkey_long->nb_connected = true;

already_connected:
	spin_unlock_irqrestore(&crashkey_long->state_lock, flags);

	return ret;
}

int sec_crashkey_long_connect_to_input_evnet(void)
{
	if (!__crashkey_long_is_probed())
		return -EBUSY;

	return __crashkey_long_connect_to_input_evnet();
}
EXPORT_SYMBOL_GPL(sec_crashkey_long_connect_to_input_evnet);

static inline int __crashkey_long_disconnect_from_input_event(void)
{
	struct crashkey_long_keylog *keylog;
	struct crashkey_long_notify *notify;
	unsigned long flags;
	int ret = 0;

	keylog = &crashkey_long->keylog;
	notify = &crashkey_long->notify;

	spin_lock_irqsave(&crashkey_long->state_lock, flags);

	if (crashkey_long->nb_connected)
		goto already_disconnected;

	ret = sec_kn_unregister_notifier(&crashkey_long->nb,
			keylog->used_key, keylog->nr_used_key);

	crashkey_long->nb_connected = false;
	bitmap_zero(keylog->bitmap_received, keylog->sz_bitmap);

already_disconnected:
	spin_unlock_irqrestore(&crashkey_long->state_lock, flags);

	del_timer(&notify->tl);

	return ret;
}

int sec_crashkey_long_disconnect_from_input_event(void)
{
	if (!__crashkey_long_is_probed())
		return -EBUSY;

	return __crashkey_long_disconnect_from_input_event();
}
EXPORT_SYMBOL_GPL(sec_crashkey_long_disconnect_from_input_event);

static void sec_crashkey_long_do_on_expired(struct timer_list *tl)
{
	struct crashkey_long_notify *notify =
			container_of(tl, struct crashkey_long_notify, tl);

	raw_notifier_call_chain(&notify->list,
			SEC_CRASHKEY_LONG_NOTIFY_TYPE_EXPIRED, NULL);
}

__ss_static bool __crashkey_long_is_mached_received_pattern(
		struct crashkey_long_drvdata *drvdata)
{
	struct crashkey_long_keylog *keylog = &drvdata->keylog;
	size_t i;

	for (i = 0; i < keylog->nr_used_key; i++) {
		if (!test_bit(keylog->used_key[i], keylog->bitmap_received))
			return false;
	}

	return true;
}

__ss_static void __crashkey_long_invoke_notifier_on_matched(
		struct crashkey_long_notify *notify)
{
	raw_notifier_call_chain(&notify->list,
			SEC_CRASHKEY_LONG_NOTIFY_TYPE_MATCHED, NULL);
}

__ss_static void __crashkey_long_invoke_timer_on_matched(
		struct crashkey_long_notify *notify)
{
	unsigned long expires;
	struct crashkey_long_drvdata *drvdata;

	if (timer_pending(&notify->tl))
		return;

	expires = jiffies + msecs_to_jiffies(notify->expire_msec);
	drvdata = container_of(notify, struct crashkey_long_drvdata, notify);

	timer_setup(&notify->tl, sec_crashkey_long_do_on_expired, 0);
	mod_timer(&notify->tl, expires);
	dev_info(drvdata->bd.dev, "long key timer - start");
}

static void __crashkey_long_on_matched(struct crashkey_long_drvdata *drvdata)
{
	struct crashkey_long_notify *notify = &drvdata->notify;

	__crashkey_long_invoke_notifier_on_matched(notify);

	if (likely(!sec_debug_is_enabled()))
		return;

	__crashkey_long_invoke_timer_on_matched(notify);
}

__ss_static void __crashkey_long_invoke_notifier_on_unmatched(
		struct crashkey_long_notify *notify)
{
	raw_notifier_call_chain(&notify->list,
			SEC_CRASHKEY_LONG_NOTIFY_TYPE_UNMATCHED, NULL);
}

__ss_static void __crashkey_long_invoke_timer_on_unmatched(
		struct crashkey_long_notify *notify)
{
	if (!timer_pending(&notify->tl))
		return;

	del_timer(&notify->tl);
	pr_info("long key timer - cancel");
}

static void __crashkey_long_on_unmatched(struct crashkey_long_drvdata *drvdata)
{
	struct crashkey_long_notify *notify = &drvdata->notify;

	__crashkey_long_invoke_notifier_on_unmatched(notify);
	__crashkey_long_invoke_timer_on_unmatched(notify);
}

__ss_static void __crashkey_long_update_bitmap_received(
		struct crashkey_long_drvdata* drvdata,
		struct sec_key_notifier_param *param)
{
	struct crashkey_long_keylog *keylog = &drvdata->keylog;

	if (param->down)
		set_bit(param->keycode, keylog->bitmap_received);
	else
		clear_bit(param->keycode, keylog->bitmap_received);
}

static int sec_crashkey_long_notifier_call(struct notifier_block *this,
		unsigned long type, void *data)
{
	struct crashkey_long_drvdata *drvdata =
			container_of(this, struct crashkey_long_drvdata, nb);
	struct sec_key_notifier_param *param = data;
	unsigned long flags;

	spin_lock_irqsave(&drvdata->state_lock, flags);

	if (!drvdata->nb_connected) {
		spin_unlock_irqrestore(&drvdata->state_lock, flags);
		return NOTIFY_DONE;
	}

	__crashkey_long_update_bitmap_received(drvdata, param);

	spin_unlock_irqrestore(&drvdata->state_lock, flags);

	if (__crashkey_long_is_mached_received_pattern(drvdata))
		__crashkey_long_on_matched(drvdata);
	else
		__crashkey_long_on_unmatched(drvdata);

	return NOTIFY_OK;
}

__ss_static noinline int __crashkey_long_parse_dt_panic_msg(struct builder *bd,
		struct device_node *np)
{
	struct crashkey_long_drvdata *drvdata =
			container_of(bd, struct crashkey_long_drvdata, bd);
	struct crashkey_long_notify *notify = &drvdata->notify;

	return of_property_read_string(np, "sec,panic_msg", &notify->panic_msg);
}

__ss_static noinline int __crashkey_long_parse_dt_expire_msec(struct builder *bd,
		struct device_node *np)
{
	struct crashkey_long_drvdata *drvdata =
			container_of(bd, struct crashkey_long_drvdata, bd);
	struct crashkey_long_notify *notify = &drvdata->notify;
	u32 expire_msec;
	int err;

	err = of_property_read_u32(np, "sec,expire_msec", &expire_msec);
	if (err)
		return -EINVAL;

	notify->expire_msec = (unsigned int)expire_msec;

	return 0;
}

__ss_static noinline int __crashkey_long_parse_dt_used_key(struct builder *bd,
		struct device_node *np)
{
	struct crashkey_long_drvdata *drvdata =
			container_of(bd, struct crashkey_long_drvdata, bd);
	struct crashkey_long_keylog *keylog = &drvdata->keylog;
	struct device *dev = bd->dev;
	int nr_used_key;
	unsigned int *used_key;
	u32 event;
	int i;

	nr_used_key = of_property_count_u32_elems(np, "sec,used_key");
	if (nr_used_key <= 0) {
		dev_err(dev, "reset-key event list is not specified!\n");
		return -ENODEV;
	}

	used_key = devm_kcalloc(dev, nr_used_key, sizeof(*used_key),
			GFP_KERNEL);
	if (!used_key)
		return -ENOMEM;

	for (i = 0; i < nr_used_key; i++) {
		of_property_read_u32_index(np, "sec,used_key", i, &event);

		used_key[i] = event;
	}

	keylog->used_key = used_key;
	keylog->nr_used_key = (size_t)nr_used_key;

	return 0;
}

static const struct dt_builder __crashkey_long_dt_builder[] = {
	DT_BUILDER(__crashkey_long_parse_dt_panic_msg),
	DT_BUILDER(__crashkey_long_parse_dt_expire_msec),
	DT_BUILDER(__crashkey_long_parse_dt_used_key),
};

static noinline int __crashkey_long_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __crashkey_long_dt_builder,
			ARRAY_SIZE(__crashkey_long_dt_builder));
}

__ss_static noinline int __crashkey_long_probe_prolog(struct builder *bd)
{
	struct crashkey_long_drvdata *drvdata =
			container_of(bd, struct crashkey_long_drvdata, bd);
	struct crashkey_long_notify *notify = &drvdata->notify;

	RAW_INIT_NOTIFIER_HEAD(&notify->list);
	spin_lock_init(&drvdata->state_lock);

	return 0;
}

__ss_static noinline int __crashkey_long_alloc_bitmap_received(struct builder *bd)
{
	struct crashkey_long_drvdata *drvdata =
			container_of(bd, struct crashkey_long_drvdata, bd);
	struct crashkey_long_keylog *keylog = &drvdata->keylog;
	unsigned long *bitmap_received;

	bitmap_received = devm_bitmap_zalloc(bd->dev, KEY_MAX, GFP_KERNEL);
	if (!bitmap_received)
		return -ENOMEM;

	keylog->bitmap_received = bitmap_received;
	keylog->sz_bitmap = BITS_TO_LONGS(KEY_MAX) * sizeof(unsigned long);

	return 0;
}

static int __crashkey_long_panic_on_expired(struct notifier_block *this,
		unsigned long type, void *v)
{
	struct crashkey_long_notify *notify =
			container_of(this, struct crashkey_long_notify, panic);

	if (type != SEC_CRASHKEY_LONG_NOTIFY_TYPE_EXPIRED)
		return NOTIFY_DONE;

	pr_err("*** Force trigger kernel panic before triggering hard reset ***\n");

	/* NOTE: this is an intended operation to reset the device with errors. */
	panic("%s", notify->panic_msg);

	return NOTIFY_OK;
}

static int __crashkey_long_set_panic_on_expired(struct builder *bd)
{
	struct crashkey_long_drvdata *drvdata =
			container_of(bd, struct crashkey_long_drvdata, bd);
	struct crashkey_long_notify *notify = &drvdata->notify;
	int err;

	/* NOTE: register a calling kernel panic in the end of notifier chain */
	notify->panic.notifier_call = __crashkey_long_panic_on_expired;
	notify->panic.priority = INT_MIN;

	err = __crashkey_long_add_preparing_panic(drvdata, &notify->panic);
	if (err)
		return err;

	return 0;
}

static void __crashkey_long_unset_panic_on_expired(struct builder *bd)
{
	struct crashkey_long_drvdata *drvdata =
			container_of(bd, struct crashkey_long_drvdata, bd);
	struct crashkey_long_notify *notify = &drvdata->notify;

	__crashkey_long_del_preparing_panic(drvdata, &notify->panic);
}

static int __crashkey_long_install_keyboard_notifier(struct builder *bd)
{
	struct crashkey_long_drvdata *drvdata =
			container_of(bd, struct crashkey_long_drvdata, bd);
	struct crashkey_long_keylog *keylog = &drvdata->keylog;
	int err;

	drvdata->nb.notifier_call = sec_crashkey_long_notifier_call;
	err = sec_kn_register_notifier(&drvdata->nb,
			keylog->used_key, keylog->nr_used_key);
	if (err)
		return err;

	drvdata->nb_connected = true;

	return 0;
}

static void __crashkey_long_uninstall_keyboard_notifier(struct builder *bd)
{
	struct crashkey_long_drvdata *drvdata =
			container_of(bd, struct crashkey_long_drvdata, bd);
	struct crashkey_long_keylog *keylog = &drvdata->keylog;

	sec_kn_unregister_notifier(&drvdata->nb,
			keylog->used_key, keylog->nr_used_key);
}

static int __crashkey_long_probe_epilog(struct builder *bd)
{
	struct crashkey_long_drvdata *drvdata =
			container_of(bd, struct crashkey_long_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);
	crashkey_long = drvdata;	/* set a singleton */

	return 0;
}

static void __crashkey_long_remove_prolog(struct builder *bd)
{
	/* FIXME: This is not a graceful exit. */
	crashkey_long = NULL;
}

static int __crashkey_long_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct crashkey_long_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __crashkey_long_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct crashkey_long_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static const struct dev_builder __crashkey_long_dev_builder[] = {
	DEVICE_BUILDER(__crashkey_long_parse_dt, NULL),
	DEVICE_BUILDER(__crashkey_long_probe_prolog, NULL),
	DEVICE_BUILDER(__crashkey_long_alloc_bitmap_received, NULL),
	DEVICE_BUILDER(__crashkey_long_set_panic_on_expired,
		       __crashkey_long_unset_panic_on_expired),
	DEVICE_BUILDER(__crashkey_long_install_keyboard_notifier,
		       __crashkey_long_uninstall_keyboard_notifier),
	DEVICE_BUILDER(__crashkey_long_probe_epilog,
		       __crashkey_long_remove_prolog),
};

static int sec_crashkey_long_probe(struct platform_device *pdev)
{
	return __crashkey_long_probe(pdev, __crashkey_long_dev_builder,
			ARRAY_SIZE(__crashkey_long_dev_builder));
}

static int sec_crashkey_long_remove(struct platform_device *pdev)
{
	return __crashkey_long_remove(pdev, __crashkey_long_dev_builder,
			ARRAY_SIZE(__crashkey_long_dev_builder));
}

static const struct of_device_id sec_crashkeylong_match_table[] = {
	{ .compatible = "samsung,crashkey-long" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_crashkeylong_match_table);

static struct platform_driver sec_crashkey_long_driver = {
	.driver = {
		.name = "sec,crashkey-long",
		.of_match_table = of_match_ptr(sec_crashkeylong_match_table),
	},
	.probe = sec_crashkey_long_probe,
	.remove = sec_crashkey_long_remove,
};

static int __init sec_crashkey_long_init(void)
{
	int err;

	err = platform_driver_register(&sec_crashkey_long_driver);
	if (err)
		return err;

	err = __of_platform_early_populate_init(sec_crashkeylong_match_table);
	if (err)
		return err;

	return 0;
}
core_initcall(sec_crashkey_long_init);

static void __exit sec_crashkey_long_exit(void)
{
	platform_driver_unregister(&sec_crashkey_long_driver);
}
module_exit(sec_crashkey_long_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Long key reset driver");
MODULE_LICENSE("GPL v2");
