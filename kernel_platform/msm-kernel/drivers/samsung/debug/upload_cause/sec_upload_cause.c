// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2020-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/panic_notifier.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/samsung/of_early_populate.h>
#include <linux/samsung/sec_kunit.h>

#include "sec_upload_cause.h"

static struct upload_cause_drvdata *upload_cause;

static __always_inline bool __upldc_is_probed(void)
{
	return !!upload_cause;
}

__ss_static __ss_inline int __upldc_add_cause(
		struct upload_cause_drvdata *drvdata,
		struct sec_upload_cause *uc)
{
	struct upload_cause_notify *notify;

	notify = &drvdata->notify;

	mutex_lock(&notify->lock);
	list_add(&uc->list, &notify->list);
	mutex_unlock(&notify->lock);

	return 0;
}

int sec_upldc_add_cause(struct sec_upload_cause *uc)
{
	if (!__upldc_is_probed())
		return -EBUSY;

	return __upldc_add_cause(upload_cause, uc);
}
EXPORT_SYMBOL_GPL(sec_upldc_add_cause);

__ss_static __ss_inline int __upldc_del_cause(
		struct upload_cause_drvdata *drvdata,
		struct sec_upload_cause *uc)
{
	struct upload_cause_notify *notify;

	notify = &drvdata->notify;

	mutex_lock(&notify->lock);
	list_del(&uc->list);
	mutex_unlock(&notify->lock);

	return 0;
}

int sec_upldc_del_cause(struct sec_upload_cause *uc)
{
	if (!__upldc_is_probed())
		return -EBUSY;

	return __upldc_del_cause(upload_cause, uc);
}
EXPORT_SYMBOL_GPL(sec_upldc_del_cause);

__ss_static __ss_inline int __upldc_set_default_cause(
		struct upload_cause_drvdata *drvdata,
		struct sec_upload_cause *uc)
{
	struct upload_cause_notify *notify;
	int err = 0;

	notify = &drvdata->notify;

	mutex_lock(&notify->lock);
	if (notify->dflt) {
		pr_warn("default handle is already registered. ignored!\n");
		pr_warn("Caller is %pS\n", __builtin_return_address(0));
		err = -EINVAL;
	} else {
		notify->dflt = uc;
	}
	mutex_unlock(&notify->lock);

	return err;
}

int sec_upldc_set_default_cause(struct sec_upload_cause *uc)
{
	if (!__upldc_is_probed())
		return -EBUSY;

	return __upldc_set_default_cause(upload_cause, uc);
}
EXPORT_SYMBOL_GPL(sec_upldc_set_default_cause);

__ss_static __ss_inline int __upldc_unset_default_cause(
		struct upload_cause_drvdata *drvdata,
		struct sec_upload_cause *uc)
{
	struct upload_cause_notify *notify;
	int err = 0;

	notify = &drvdata->notify;

	mutex_lock(&notify->lock);

	if (!notify->dflt) {
		pr_warn("default handle is already unset - ignored.\n");
		err = -ENOENT;
	} else if (notify->dflt != uc) {
		pr_warn("current default handle is different from you requested - ignored.\n");
		err = -EINVAL;
	} else
		notify->dflt = NULL;

	mutex_unlock(&notify->lock);

	return err;
}

int sec_upldc_unset_default_cause(struct sec_upload_cause *uc)
{
	if (!__upldc_is_probed())
		return -EBUSY;

	return __upldc_unset_default_cause(upload_cause, uc);
}
EXPORT_SYMBOL_GPL(sec_upldc_unset_default_cause);

__ss_static __ss_inline void __upldc_type_to_cause(
		struct upload_cause_drvdata *drvdata,
		unsigned int type, char *cause, size_t len)
{
	struct upload_cause_notify *notify = &drvdata->notify;
	struct sec_upload_cause *uc;

	mutex_lock(&notify->lock);
	list_for_each_entry(uc, &notify->list, list) {
		if (type == uc->type) {
			snprintf(cause, len, "%s", uc->cause);
			mutex_unlock(&notify->lock);
			return;
		}
	}
	mutex_unlock(&notify->lock);

	strlcpy(cause, "unknown_reset", len);
}

void sec_upldc_type_to_cause(unsigned int type, char *cause, size_t len)
{
	if (!__upldc_is_probed())
		return;

	__upldc_type_to_cause(upload_cause, type, cause, len);
}
EXPORT_SYMBOL_GPL(sec_upldc_type_to_cause);

static int __upldc_handle_each_cause(const struct sec_upload_cause *uc,
		const char *cause)
{
	return uc->func(uc, cause);
}

static int __upldc_handle_single_cause(
		struct upload_cause_notify *notify, const char *cause)
{
	struct sec_upload_cause *uc;
	int handled = SEC_UPLOAD_CAUSE_HANDLE_DONE;

	/* NOTE: mutex_lock/_unlock may not be needed when this
	 * function works.
	 */
	list_for_each_entry(uc, &notify->list, list) {
		handled = __upldc_handle_each_cause(uc, cause);
		if (handled & SEC_UPLOAD_CAUSE_HANDLE_STOP_MASK)
			break;
	}

	return handled;
}

__ss_static int __upldc_handle(struct upload_cause_notify *notify,
		const char *cause)
{
	int handled;

	handled = __upldc_handle_single_cause(notify, cause);

	if ((handled & SEC_UPLOAD_CAUSE_HANDLE_OK) == SEC_UPLOAD_CAUSE_HANDLE_OK)
		return handled;

	if (notify->dflt) {
		pr_info("call default handler\n");
		handled = notify->dflt->func(notify->dflt, cause);
	}

	return handled;
}

static int sec_upldc_notifier_call(struct notifier_block *this,
		unsigned long type, void *data)
{
	struct upload_cause_notify *notify =
			container_of(this, struct upload_cause_notify, nb);
	const char *cause = data;
	int err;
	int ret = NOTIFY_DONE;

	pr_info("cause : %s\n", cause);

	err = __upldc_handle(notify, cause);
	switch (err) {
	case SEC_UPLOAD_CAUSE_HANDLE_DONE:
		pr_warn("any reboot reason did not handle this command - %s",
				cause);
		break;
	case SEC_UPLOAD_CAUSE_HANDLE_OK:
		ret = NOTIFY_OK;
		break;
	default:
		if (err & SEC_UPLOAD_CAUSE_HANDLE_BAD)
			pr_err("error occured during iteration\n");
		else
			pr_err("unknown return value - %d\n", err);
		break;
	}

	return ret;
}

#if IS_ENABLED(CONFIG_DEBUG_FS)
static void __upldc_dbgfs_show_each_cause_locked(struct seq_file *m,
		struct sec_upload_cause *uc)
{
	seq_printf(m, "[<%p>] %s\n", uc, uc->cause);
	seq_printf(m, "  - func : [<%p>] %ps\n", uc->func, uc->func);

	seq_puts(m, "\n");
}

static void __upldc_dbgfs_show_default_cause_locked(struct seq_file *m,
		struct upload_cause_notify *notify)
{
	seq_puts(m, "+ Default :\n");

	if (notify->dflt)
		__upldc_dbgfs_show_each_cause_locked(m, notify->dflt);
	else
		seq_puts(m, "[WARN] default cause is not set\n");

	seq_puts(m, "\n");
}

static int sec_upldc_dbgfs_show_all(struct seq_file *m, void *unsed)
{
	struct upload_cause_drvdata *drvdata = m->private;
	struct upload_cause_notify *notify = &drvdata->notify;
	struct sec_upload_cause *uc;

	seq_printf(m, "+ Priority : %d\n", notify->nb.priority);

	mutex_lock(&notify->lock);

	__upldc_dbgfs_show_default_cause_locked(m, notify);

	list_for_each_entry(uc, &notify->list, list) {
		__upldc_dbgfs_show_each_cause_locked(m, uc);
	}

	mutex_unlock(&notify->lock);

	return 0;
}

static int sec_upldc_dbgfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_upldc_dbgfs_show_all,
			inode->i_private);
}

static const struct file_operations sec_upldc_dgbfs_fops = {
	.open = sec_upldc_dbgfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int __upldc_debugfs_create(struct builder *bd)
{
	struct upload_cause_drvdata *drvdata =
			container_of(bd, struct upload_cause_drvdata, bd);

	drvdata->dbgfs = debugfs_create_file("sec_upload_cause", 0440,
			NULL, drvdata, &sec_upldc_dgbfs_fops);

	return 0;
}

static void __upldc_debugfs_remove(struct builder *bd)
{
	struct upload_cause_drvdata *drvdata =
			container_of(bd, struct upload_cause_drvdata, bd);

	debugfs_remove(drvdata->dbgfs);
}
#else
static int __upldc_debugfs_create(struct builder *bd) { return 0; }
static void __upldc_debugfs_remove(struct builder *bd) {}
#endif

__ss_static noinline int __upldc_parse_dt_panic_notifier_priority(struct builder *bd,
		struct device_node *np)
{
	struct upload_cause_drvdata *drvdata =
			container_of(bd, struct upload_cause_drvdata, bd);
	struct upload_cause_notify *notify = &drvdata->notify;
	s32 priority;
	int err;

	err = of_property_read_s32(np, "sec,panic_notifier-priority",
			&priority);
	if (err)
		return -EINVAL;

	notify->nb.priority = (int)priority;

	return 0;
}

static const struct dt_builder __upldc_dt_builder[] = {
	DT_BUILDER(__upldc_parse_dt_panic_notifier_priority),
};

static noinline int __upldc_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __upldc_dt_builder,
			ARRAY_SIZE(__upldc_dt_builder));
}

__ss_static noinline int __upldc_probe_prolog(struct builder *bd)
{
	struct upload_cause_drvdata *drvdata =
			container_of(bd, struct upload_cause_drvdata, bd);
	struct upload_cause_notify *notify = &drvdata->notify;

	mutex_init(&notify->lock);
	INIT_LIST_HEAD(&notify->list);

	return 0;
}

static noinline void __upldc_remove_epilog(struct builder *bd)
{
	struct upload_cause_drvdata *drvdata =
			container_of(bd, struct upload_cause_drvdata, bd);
	struct upload_cause_notify *notify = &drvdata->notify;

	mutex_destroy(&notify->lock);
}

static int __upldc_register_panic_notifier(struct builder *bd)
{
	struct upload_cause_drvdata *drvdata =
			container_of(bd, struct upload_cause_drvdata, bd);
	struct upload_cause_notify *notify = &drvdata->notify;

	notify->nb.notifier_call = sec_upldc_notifier_call;

	return atomic_notifier_chain_register(&panic_notifier_list,
			&notify->nb);
}

static void __upldc_unregister_panic_notifier(struct builder *bd)
{
	struct upload_cause_drvdata *drvdata =
			container_of(bd, struct upload_cause_drvdata, bd);
	struct upload_cause_notify *notify = &drvdata->notify;

	atomic_notifier_chain_unregister(&panic_notifier_list, &notify->nb);
}

static int __upldc_probe_epilog(struct builder *bd)
{
	struct upload_cause_drvdata *drvdata =
			container_of(bd, struct upload_cause_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);
	upload_cause = drvdata;	/* set a singleton */

	return 0;
}

static void __upldc_remove_prolog(struct builder *bd)
{
	/* FIXME: This is not a graceful exit. */
	upload_cause = NULL;
}

static int __upldc_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct upload_cause_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __upldc_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct upload_cause_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static const struct dev_builder __upldc_dev_builder[] = {
	DEVICE_BUILDER(__upldc_parse_dt, NULL),
	DEVICE_BUILDER(__upldc_probe_prolog, __upldc_remove_epilog),
	DEVICE_BUILDER(__upldc_register_panic_notifier,
		       __upldc_unregister_panic_notifier),
	DEVICE_BUILDER(__upldc_debugfs_create,
		       __upldc_debugfs_remove),
	DEVICE_BUILDER(__upldc_probe_epilog,
		       __upldc_remove_prolog),
};

static int sec_upldc_probe(struct platform_device *pdev)
{
	return __upldc_probe(pdev, __upldc_dev_builder,
			ARRAY_SIZE(__upldc_dev_builder));
}

static int sec_upldc_remove(struct platform_device *pdev)
{
	return __upldc_remove(pdev, __upldc_dev_builder,
			ARRAY_SIZE(__upldc_dev_builder));
}

static const struct of_device_id sec_upldc_match_table[] = {
	{ .compatible = "samsung,upload_cause" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_upldc_match_table);

static struct platform_driver sec_upldc_driver = {
	.driver = {
		.name = "sec,upload_cause",
		.of_match_table = of_match_ptr(sec_upldc_match_table),
	},
	.probe = sec_upldc_probe,
	.remove = sec_upldc_remove,
};

static __init int sec_upldc_init(void)
{
	int err;

	err = platform_driver_register(&sec_upldc_driver);
	if (err)
		return err;

	err = __of_platform_early_populate_init(sec_upldc_match_table);
	if (err)
		return err;

	return 0;
}
core_initcall(sec_upldc_init);

static __exit void sec_upldc_exit(void)
{
	platform_driver_unregister(&sec_upldc_driver);
}
module_exit(sec_upldc_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Panic Notifier Updating Upload Cause");
MODULE_LICENSE("GPL v2");
