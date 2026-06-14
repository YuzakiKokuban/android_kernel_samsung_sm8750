// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/notifier.h>
#include <linux/of_platform.h>
#include <linux/usb_vendor_notify.h>
#include <linux/usb_vendor_notify_defs.h>

static struct blocking_notifier_head *usb_vendor_notifier;

int send_usb_vendor_notify_pcm_info(int direction, int enable)
{
	int ret = 0;
	int action;
	struct data_pcm_info data;

	if (!usb_vendor_notifier) {
		pr_err("%s: not initialized\n", __func__);
		return -ENODATA;
	}

	action = USB_VENDOR_NOTIFY_PCM_INFO;
	data.direction = direction;
	data.enable = enable;

	ret = blocking_notifier_call_chain(usb_vendor_notifier, action, &data);
	if (ret != NOTIFY_DONE && ret != NOTIFY_OK)
		pr_err("%s: err(%d)\n", __func__, ret);

	return ret;
}
EXPORT_SYMBOL(send_usb_vendor_notify_pcm_info);

int send_usb_vendor_notify_cardnum(int card_num, int bundle, int attach)
{
	int ret = 0;
	int action;
	struct data_cardnum data;

	if (!usb_vendor_notifier) {
		pr_err("%s: not initialized\n", __func__);
		return -ENODATA;
	}

	action = USB_VENDOR_NOTIFY_CARDNUM;
	data.card_num = card_num;
	data.bundle = bundle;
	data.attach = attach;

	ret = blocking_notifier_call_chain(usb_vendor_notifier, action, &data);
	if (ret != NOTIFY_DONE && ret != NOTIFY_OK)
		pr_err("%s: err(%d)\n", __func__, ret);

	return ret;
}
EXPORT_SYMBOL(send_usb_vendor_notify_cardnum);

int send_usb_vendor_notify_audio_uevent(struct usb_device *dev, int card_num,
		int attach)
{
	int ret = 0;
	int action;
	struct data_audio_uevent data;

	if (!usb_vendor_notifier) {
		pr_err("%s: not initialized\n", __func__);
		return -ENODATA;
	}

	action = USB_VENDOR_NOTIFY_AUDIO_UEVENT;
	data.dev = dev;
	data.card_num = card_num;
	data.attach = attach;

	ret = blocking_notifier_call_chain(usb_vendor_notifier, action, &data);
	if (ret != NOTIFY_DONE && ret != NOTIFY_OK)
		pr_err("%s: err(%d)\n", __func__, ret);

	return ret;
}
EXPORT_SYMBOL(send_usb_vendor_notify_audio_uevent);

int send_usb_vendor_notify_new_device(struct usb_device *dev)
{
	int ret = 0;
	int action;
	struct data_new_device data;

	if (!usb_vendor_notifier) {
		pr_err("%s: not initialized\n", __func__);
		return -ENODATA;
	}

	action = USB_VENDOR_NOTIFY_NEW_DEVICE;
	data.dev = dev;
	data.ret = 0;

	ret = blocking_notifier_call_chain(usb_vendor_notifier, action, &data);
	if (ret != NOTIFY_DONE && ret != NOTIFY_OK)
		pr_err("%s: err(%d)\n", __func__, ret);

	if (data.ret)
		ret = data.ret;

	return ret;
}
EXPORT_SYMBOL(send_usb_vendor_notify_new_device);

static int usb_vendor_notify_probe(struct platform_device *pdev)
{
	int ret = 0;

	usb_vendor_notifier = devm_kzalloc(&pdev->dev,
			sizeof(struct blocking_notifier_head), GFP_KERNEL);
	if (!usb_vendor_notifier) {
		ret = -ENOMEM;
		goto out;
	}
	BLOCKING_INIT_NOTIFIER_HEAD(usb_vendor_notifier);
	platform_set_drvdata(pdev, usb_vendor_notifier);
	pr_info("%s done\n", __func__);
out:
	return ret;
}

static int usb_vendor_notify_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);
	usb_vendor_notifier = NULL;
	return 0;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id usb_vendor_notify_dt_ids[] = {
	{ .compatible = "samsung,usb_vendor_notify" },
	{ }
};
MODULE_DEVICE_TABLE(of, usb_vendor_notify_dt_ids);
#endif /* CONFIG_OF */

static struct platform_driver usb_vendor_notify_driver = {
	.probe		= usb_vendor_notify_probe,
	.remove		= usb_vendor_notify_remove,
	.driver		= {
		.name		= "usb_vendor_notify",
		.owner		= THIS_MODULE,
#if IS_ENABLED(CONFIG_OF)
		.of_match_table	= of_match_ptr(usb_vendor_notify_dt_ids),
#endif /* CONFIG_OF */
	},
};

static int __init usb_vendor_notify_init(void)
{
	return platform_driver_register(&usb_vendor_notify_driver);
}

static void __exit usb_vendor_notify_exit(void)
{
	platform_driver_unregister(&usb_vendor_notify_driver);
}

module_init(usb_vendor_notify_init);
module_exit(usb_vendor_notify_exit);

MODULE_DESCRIPTION("usb vendor notify");
MODULE_LICENSE("GPL");
