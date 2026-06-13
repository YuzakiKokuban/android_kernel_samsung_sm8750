// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2016-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/module.h>
#include <linux/input.h>
#include <linux/notifier.h>
#include <linux/slab.h>

#include <linux/samsung/bsp/sec_key_notifier.h>

static DEFINE_SPINLOCK(sec_kn_event_lock);
static RAW_NOTIFIER_HEAD(sec_kn_notifier_list);
static int sec_kn_acceptable_event[KEY_MAX] __read_mostly;

static void inline update_acceptable_event(unsigned int event_code, bool is_add)
{
	if (unlikely(event_code >= KEY_MAX)) {
		pr_warn("event_code (%d) must be less thant KEY_MAX!\n",
				event_code);
		pr_warn("Caller is %pS\n", __builtin_return_address(0));
		return;
	}

	if (is_add)
		sec_kn_acceptable_event[event_code]++;
	else
		sec_kn_acceptable_event[event_code]--;

	BUG_ON(sec_kn_acceptable_event[event_code] < 0);
}

static inline void increase_num_of_acceptable_event(unsigned int event_code)
{
	update_acceptable_event(event_code, true);
}

int sec_kn_register_notifier(struct notifier_block *nb,
		const unsigned int *events, const size_t nr_events)
{
	unsigned long flags;
	size_t i;
	int err;

	spin_lock_irqsave(&sec_kn_event_lock, flags);

	for (i = 0; i < nr_events; i++)
		increase_num_of_acceptable_event(events[i]);

	err = raw_notifier_chain_register(&sec_kn_notifier_list, nb);

	spin_unlock_irqrestore(&sec_kn_event_lock, flags);

	return err;
}
EXPORT_SYMBOL_GPL(sec_kn_register_notifier);

static inline void decrease_num_of_acceptable_event(unsigned int event_code)
{
	update_acceptable_event(event_code, false);
}

int sec_kn_unregister_notifier(struct notifier_block *nb,
		const unsigned int *events, const size_t nr_events)
{
	unsigned long flags;
	size_t i;
	int err;

	spin_lock_irqsave(&sec_kn_event_lock, flags);

	for (i = 0; i < nr_events; i++)
		decrease_num_of_acceptable_event(events[i]);

	err = raw_notifier_chain_unregister(&sec_kn_notifier_list, nb);

	spin_unlock_irqrestore(&sec_kn_event_lock, flags);

	return err;
}
EXPORT_SYMBOL_GPL(sec_kn_unregister_notifier);

static inline bool is_event_supported_locked(unsigned int event_type,
		unsigned int event_code)
{
	if (event_type != EV_KEY || event_code >= KEY_MAX)
		return false;

	return !!sec_kn_acceptable_event[event_code];
}

static void sec_kn_event(struct input_handle *handle, unsigned int event_type,
		unsigned int event_code, int value)
{
	struct sec_key_notifier_param param = {
		.keycode = event_code,
		.down = value,
	};

	spin_lock(&sec_kn_event_lock);

	if (!is_event_supported_locked(event_type, event_code)) {
		spin_unlock(&sec_kn_event_lock);
		return;
	}

	raw_notifier_call_chain(&sec_kn_notifier_list, 0, &param);

	spin_unlock(&sec_kn_event_lock);
}

static int sec_kn_connect(struct input_handler *handler, struct input_dev *dev,
		const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "sec_key_notifier";

	error = input_register_handle(handle);
	if (error)
		goto err_free_handle;

	error = input_open_device(handle);
	if (error)
		goto err_unregister_handle;

	return 0;

err_unregister_handle:
	input_unregister_handle(handle);
err_free_handle:
	kfree(handle);
	return error;
}

static void sec_kn_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id sec_kn_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
	},
	{},
};

static struct input_handler sec_kn_handler = {
	.event = sec_kn_event,
	.connect = sec_kn_connect,
	.disconnect = sec_kn_disconnect,
	.name = "sec_key_notifier",
	.id_table = sec_kn_ids,
};

static int __init sec_kn_init(void)
{
	return input_register_handler(&sec_kn_handler);
}
#if IS_BUILTIN(CONFIG_SEC_KEY_NOTIFIER)
pure_initcall(sec_kn_init);
#else
module_init(sec_kn_init);
#endif

static void __exit sec_kn_exit(void)
{
	input_unregister_handler(&sec_kn_handler);
}
module_exit(sec_kn_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Atomic keyboard notifier");
MODULE_LICENSE("GPL v2");
