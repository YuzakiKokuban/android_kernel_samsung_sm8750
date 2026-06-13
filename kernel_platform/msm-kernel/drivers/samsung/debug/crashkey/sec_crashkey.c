// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2019-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/syscore_ops.h>

#include <linux/samsung/bsp/sec_key_notifier.h>
#include <linux/samsung/debug/sec_debug.h>
#include <linux/samsung/debug/sec_log_buf.h>
#include <linux/samsung/of_early_populate.h>
#include <linux/samsung/sec_kunit.h>
#include <linux/samsung/sec_of.h>

#include "sec_crashkey.h"

static DEFINE_MUTEX(crashkey_list_lock);
static LIST_HEAD(crashkey_dev_list);

__ss_static struct crashkey_drvdata *__crashkey_find_by_name_locked(
	const char *name, struct list_head *head)
{
	struct crashkey_drvdata *drvdata;
	size_t name_len = strlen(name);

	list_for_each_entry(drvdata, head, list) {
		size_t len = strnlen(drvdata->name, name_len + 1);

		if (len != name_len)
			continue;

		if (!strncmp(drvdata->name, name, name_len))
			return drvdata;
	}

	return ERR_PTR(-ENOENT);
}

__ss_static int __crashkey_add_preparing_panic_locked(
		struct crashkey_drvdata *drvdata, struct notifier_block *nb)
{
	struct crashkey_notify *notify = &drvdata->notify;

	return raw_notifier_chain_register(&notify->list, nb);
}

int sec_crashkey_add_preparing_panic(struct notifier_block *nb,
		const char *name)
{
	struct crashkey_drvdata *drvdata;
	int err;

	mutex_lock(&crashkey_list_lock);

	drvdata = __crashkey_find_by_name_locked(name, &crashkey_dev_list);
	if (IS_ERR(drvdata)) {
		pr_warn("%s is not a valid drvdata device!\n", name);
		err = -ENODEV;
		goto err_invalid_name;
	}

	err = __crashkey_add_preparing_panic_locked(drvdata, nb);
	if (err) {
		struct device *dev = drvdata->bd.dev;

		dev_warn(dev, "failed to add a notifier for %s (%d)!\n",
				name, err);
		dev_warn(dev, "Caller is %pS\n", __builtin_return_address(0));
	}

err_invalid_name:
	mutex_unlock(&crashkey_list_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sec_crashkey_add_preparing_panic);

__ss_static int __crashkey_del_preparing_panic_locked(
		struct crashkey_drvdata *drvdata, struct notifier_block *nb)
{
	struct crashkey_notify *notify = &drvdata->notify;

	return raw_notifier_chain_unregister(&notify->list, nb);
}

int sec_crashkey_del_preparing_panic(struct notifier_block *nb,
		const char *name)
{
	struct crashkey_drvdata *drvdata;
	int err;

	mutex_lock(&crashkey_list_lock);

	drvdata = __crashkey_find_by_name_locked(name, &crashkey_dev_list);
	if (IS_ERR(drvdata)) {
		pr_warn("%s is not a valid drvdata device!\n", name);
		err = -ENODEV;
		goto err_invalid_name;
	}

	err = __crashkey_del_preparing_panic_locked(drvdata, nb);
	if (err) {
		struct device *dev = drvdata->bd.dev;

		dev_warn(dev, "failed to remove a notifier for %s!\n", name);
		dev_warn(dev, "Caller is %pS\n", __builtin_return_address(0));
	}

err_invalid_name:
	mutex_unlock(&crashkey_list_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sec_crashkey_del_preparing_panic);

static __always_inline bool __crashkey_is_same_pattern(
		struct crashkey_drvdata *drvdata,
		const struct sec_key_notifier_param *param)
{
	struct crashkey_kelog *keylog = &drvdata->keylog;
	const struct sec_key_notifier_param *desired =
			&keylog->desired[keylog->sequence];

	if (param->keycode == desired->keycode && param->down == desired->down) {
		keylog->sequence++;
		return true;
	}

	return false;
}

static __always_inline void __crashkey_clear_received_state(
		struct crashkey_drvdata *drvdata,
		const struct sec_key_notifier_param *param)
{
	struct crashkey_kelog *keylog = &drvdata->keylog;
	struct crashkey_timer *timer = &drvdata->timer;

	keylog->sequence = 0;
	ratelimit_state_init(&timer->rs, timer->interval,
			keylog->nr_pattern - 1);

	/* NOTE: if the current pattern is same as the 1st one of desried,
	 * advande a 'keylog->sequence'.
	 */
	if (param && __crashkey_is_same_pattern(drvdata, param))
		__ratelimit(&timer->rs);
}

static __always_inline void __crashkey_call_crashkey_notify(
		struct crashkey_drvdata *drvdata)
{
	struct crashkey_notify *notify = &drvdata->notify;

	raw_notifier_call_chain(&notify->list, 0, NULL);
}

__ss_static int __crashkey_notifier_call(struct crashkey_drvdata *drvdata,
		const struct sec_key_notifier_param *param)
{
	struct crashkey_kelog *keylog = &drvdata->keylog;
	struct crashkey_timer *timer = &drvdata->timer;

	if (!__crashkey_is_same_pattern(drvdata, param))
		goto clear_state;

	if (!timer->interval || !__ratelimit(&timer->rs)) {
		if (keylog->sequence == keylog->nr_pattern)
			__crashkey_call_crashkey_notify(drvdata);
		else if (timer->interval)
			goto clear_state;
	}

	return NOTIFY_OK;

clear_state:
	__crashkey_clear_received_state(drvdata, param);
	return NOTIFY_OK;
}

static int sec_crashkey_notifier_call(struct notifier_block *this,
		unsigned long type, void *data)
{
	struct crashkey_drvdata *drvdata =
			container_of(this, struct crashkey_drvdata, nb);
	struct sec_key_notifier_param *param = data;

	return __crashkey_notifier_call(drvdata, param);
}

__ss_static noinline int __crashkey_parse_dt_name(struct builder *bd,
		struct device_node *np)
{
	struct crashkey_drvdata *drvdata =
			container_of(bd, struct crashkey_drvdata, bd);

	return of_property_read_string(np, "sec,name", &drvdata->name);
}

__ss_static int __crashkey_test_dt_debug_level(struct crashkey_drvdata *drvdata,
		struct device_node *np, unsigned int sec_dbg_level)
{
	struct device *dev = drvdata->bd.dev;
	int err;

	err = sec_of_test_debug_level(np, "sec,debug_level", sec_dbg_level);
	if (err == -ENOENT) {
		dev_warn(dev, "this crashkey_dev (%s) will be enabled all sec debug levels!\n",
				drvdata->name);
		return 0;
	} else if (err < 0)
		return -ENODEV;

	return 0;
}

static noinline int __crashkey_parse_dt_debug_level(struct builder *bd,
		struct device_node *np)
{
	struct crashkey_drvdata *drvdata =
			container_of(bd, struct crashkey_drvdata, bd);
	unsigned int sec_dbg_level = sec_debug_level();

	return __crashkey_test_dt_debug_level(drvdata, np, sec_dbg_level);
}

__ss_static noinline int __crashkey_parse_dt_panic_msg(struct builder *bd,
		struct device_node *np)
{
	struct crashkey_drvdata *drvdata =
			container_of(bd, struct crashkey_drvdata, bd);
	struct crashkey_notify *notify = &drvdata->notify;

	return of_property_read_string(np, "sec,panic_msg", &notify->panic_msg);
}

__ss_static noinline int __crashkey_parse_dt_interval(struct builder *bd,
		struct device_node *np)
{
	struct crashkey_drvdata *drvdata =
			container_of(bd, struct crashkey_drvdata, bd);
	struct crashkey_timer *timer = &drvdata->timer;
	s32 interval;
	int err;

	err = of_property_read_s32(np, "sec,interval", &interval);
	if (err)
		return -EINVAL;

	timer->interval = (int)interval * HZ;

	return 0;
}

__ss_static noinline int __crashkey_parse_dt_desired_pattern(struct builder *bd,
		struct device_node *np)
{
	struct crashkey_drvdata *drvdata =
			container_of(bd, struct crashkey_drvdata, bd);
	struct device *dev = bd->dev;
	struct crashkey_kelog *keylog = &drvdata->keylog;
	struct sec_key_notifier_param *desired;
	int nr_pattern;
	u32 keycode, down;
	int i;

	nr_pattern = of_property_count_u32_elems(np, "sec,desired_pattern");
	nr_pattern /= 2;	/* <keycode, down> */
	if (nr_pattern <= 0)
		return -EINVAL;

	desired = devm_kmalloc_array(dev,
			nr_pattern, sizeof(*desired), GFP_KERNEL);
	if (!desired)
		return -ENOMEM;

	keylog->nr_pattern = (size_t)nr_pattern;
	keylog->desired = desired;

	for (i = 0; i < nr_pattern; i++) {
		of_property_read_u32_index(np, "sec,desired_pattern",
				2 * i, &keycode);
		of_property_read_u32_index(np, "sec,desired_pattern",
				2 * i + 1, &down);

		desired[i].keycode = keycode;
		desired[i].down = down;
	}

	return 0;
}

static const struct dt_builder __crashkey_dt_builder[] = {
	DT_BUILDER(__crashkey_parse_dt_name),
	DT_BUILDER(__crashkey_parse_dt_debug_level),
	DT_BUILDER(__crashkey_parse_dt_panic_msg),
	DT_BUILDER(__crashkey_parse_dt_interval),
	DT_BUILDER(__crashkey_parse_dt_desired_pattern),
};

static noinline int __crashkey_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __crashkey_dt_builder,
			ARRAY_SIZE(__crashkey_dt_builder));
}

__ss_static noinline int __crashkey_probe_prolog(struct builder *bd)
{
	struct crashkey_drvdata *drvdata =
			container_of(bd, struct crashkey_drvdata, bd);
	struct crashkey_notify *notify = &drvdata->notify;
	struct crashkey_kelog *keylog = &drvdata->keylog;
	struct crashkey_timer *timer = &drvdata->timer;

	RAW_INIT_NOTIFIER_HEAD(&notify->list);

	ratelimit_state_init(&timer->rs, timer->interval,
			keylog->nr_pattern - 1);

	return 0;
}

__ss_static void __crashkey_add_to_crashkey_dev_list_locked(
		struct crashkey_drvdata *drvdata, struct list_head *head)
{
	list_add(&drvdata->list, head);
}

static int __crashkey_add_to_crashkey_dev_list(struct builder *bd)
{
	struct crashkey_drvdata *drvdata =
			container_of(bd, struct crashkey_drvdata, bd);

	mutex_lock(&crashkey_list_lock);
	__crashkey_add_to_crashkey_dev_list_locked(drvdata, &crashkey_dev_list);
	mutex_unlock(&crashkey_list_lock);

	return 0;
}

__ss_static void __crashkey_del_from_crashkey_dev_list_locked(
		struct crashkey_drvdata *drvdata)
{
	list_del(&drvdata->list);
}

static void __crashkey_del_from_crashkey_dev_list(struct builder *bd)
{
	struct crashkey_drvdata *drvdata =
			container_of(bd, struct crashkey_drvdata, bd);

	mutex_lock(&crashkey_list_lock);
	__crashkey_del_from_crashkey_dev_list_locked(drvdata);
	mutex_unlock(&crashkey_list_lock);
}

static int __crashkey_panic_on_matched(struct notifier_block *this,
		unsigned long type, void *data)
{
	struct crashkey_notify *notify =
			container_of(this, struct crashkey_notify, panic);

	/* NOTE: this is an intended operation to reset the device with errors. */
	panic("%s", notify->panic_msg);

	return NOTIFY_OK;
}

static int __crashkey_set_panic_on_matched(struct builder *bd)
{
	struct crashkey_drvdata *drvdata =
			container_of(bd, struct crashkey_drvdata, bd);
	struct crashkey_notify *notify = &drvdata->notify;
	int err;

	/* NOTE: register a calling kernel panic in the end of notifier chain */
	notify->panic.notifier_call = __crashkey_panic_on_matched;
	notify->panic.priority = INT_MIN;

	err = sec_crashkey_add_preparing_panic(&notify->panic, drvdata->name);
	if (err)
		return err;

	return 0;
}

static void __crashkey_unset_panic_on_matched(struct builder *bd)
{
	struct crashkey_drvdata *drvdata =
			container_of(bd, struct crashkey_drvdata, bd);
	struct crashkey_notify *notify = &drvdata->notify;

	sec_crashkey_del_preparing_panic(&notify->panic, drvdata->name);
}

__ss_static noinline int __crashkey_init_used_key(struct builder *bd)
{
	struct crashkey_drvdata *drvdata =
			container_of(bd, struct crashkey_drvdata, bd);
	struct device *dev = bd->dev;
	struct crashkey_kelog *keylog = &drvdata->keylog;
	const struct sec_key_notifier_param *desired = keylog->desired;
	static unsigned int *used_key;
	size_t nr_used_key;
	bool is_new;
	size_t i, j;

	used_key = devm_kmalloc_array(dev,
			keylog->nr_pattern, sizeof(*used_key), GFP_KERNEL);
	if (!used_key)
		return -ENOMEM;

	used_key[0] = desired[0].keycode;
	nr_used_key = 1;

	for (i = 1; i < keylog->nr_pattern; i++) {
		for (j = 0, is_new = true; j < nr_used_key; j++) {
			if (used_key[j] == desired[i].keycode)
				is_new = false;
		}

		if (is_new)
			used_key[nr_used_key++] = desired[i].keycode;
	}

	keylog->used_key = used_key;
	keylog->nr_used_key = nr_used_key;

	return 0;
}

static int __crashkey_install_keyboard_notifier(struct builder *bd)
{
	struct crashkey_drvdata *drvdata =
			container_of(bd, struct crashkey_drvdata, bd);
	struct crashkey_kelog *keylog = &drvdata->keylog;
	int err;

	drvdata->nb.notifier_call = sec_crashkey_notifier_call;
	err = sec_kn_register_notifier(&drvdata->nb,
			keylog->used_key, keylog->nr_used_key);

	return err;
}

static void __crashkey_uninstall_keyboard_notifier(struct builder *bd)
{
	struct crashkey_drvdata *drvdata =
			container_of(bd, struct crashkey_drvdata, bd);
	struct crashkey_kelog *keylog = &drvdata->keylog;

	sec_kn_unregister_notifier(&drvdata->nb,
			keylog->used_key, keylog->nr_used_key);
}

static noinline int __crashkey_probe_epilog(struct builder *bd)
{
	struct crashkey_drvdata *drvdata =
			container_of(bd, struct crashkey_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);

	return 0;
}

static int __crashkey_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct crashkey_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __crashkey_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct crashkey_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static const struct dev_builder __crashkey_dev_builder[] = {
	DEVICE_BUILDER(__crashkey_parse_dt, NULL),
	DEVICE_BUILDER(__crashkey_probe_prolog, NULL),
	DEVICE_BUILDER(__crashkey_add_to_crashkey_dev_list,
		       __crashkey_del_from_crashkey_dev_list),
	DEVICE_BUILDER(__crashkey_set_panic_on_matched,
		       __crashkey_unset_panic_on_matched),
	DEVICE_BUILDER(__crashkey_init_used_key, NULL),
	DEVICE_BUILDER(__crashkey_install_keyboard_notifier,
		       __crashkey_uninstall_keyboard_notifier),
	DEVICE_BUILDER(__crashkey_probe_epilog, NULL),
};

static int sec_crashkey_probe(struct platform_device *pdev)
{
	return __crashkey_probe(pdev, __crashkey_dev_builder,
			ARRAY_SIZE(__crashkey_dev_builder));
}

static int sec_crashkey_remove(struct platform_device *pdev)
{
	return __crashkey_remove(pdev, __crashkey_dev_builder,
			ARRAY_SIZE(__crashkey_dev_builder));
}

static const struct of_device_id sec_crashkey_match_table[] = {
	{ .compatible = "samsung,crashkey" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_crashkey_match_table);

static struct platform_driver sec_crashkey_driver = {
	.driver = {
		.name = "sec,crashkey",
		.of_match_table = of_match_ptr(sec_crashkey_match_table),
	},
	.probe = sec_crashkey_probe,
	.remove = sec_crashkey_remove,
};

static int sec_crashkey_suspend(void)
{
	struct crashkey_drvdata *drvdata;

	list_for_each_entry(drvdata, &crashkey_dev_list, list) {
		__crashkey_clear_received_state(drvdata, NULL);
	}

	return 0;
}

static struct syscore_ops sec_crashkey_syscore_ops = {
	.suspend = sec_crashkey_suspend,
};

static int __init sec_crashkey_init(void)
{
	int err;

	err = platform_driver_register(&sec_crashkey_driver);
	if (err)
		return err;

	err = __of_platform_early_populate_init(sec_crashkey_match_table);
	if (err)
		return err;

	register_syscore_ops(&sec_crashkey_syscore_ops);

	return 0;
}
core_initcall(sec_crashkey_init);

static void __exit sec_crashkey_exit(void)
{
	unregister_syscore_ops(&sec_crashkey_syscore_ops);
	platform_driver_unregister(&sec_crashkey_driver);
}
module_exit(sec_crashkey_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Force key crash driver");
MODULE_LICENSE("GPL v2");
