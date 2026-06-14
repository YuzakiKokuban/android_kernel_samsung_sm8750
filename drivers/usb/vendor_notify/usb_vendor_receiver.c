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
#include <sound/pcm.h>
#include <linux/usb_notify.h>
#include <linux/usb_vendor_notify_defs.h>

struct blocking_notifier_head *nh;
struct notifier_block nb;

static void usb_vendor_receiver_pcm_info(struct data_pcm_info *data)
{
	int direction = data->direction;
	int enable = data->enable;
#if IS_ENABLED(CONFIG_USB_NOTIFY_PROC_LOG)
	int type = 0;

	if (direction == SNDRV_PCM_STREAM_PLAYBACK)
		type = NOTIFY_PCM_PLAYBACK;
	else
		type = NOTIFY_PCM_CAPTURE;
	store_usblog_notify(type, (void *)&enable, NULL);
#endif /* CONFIG_USB_NOTIFY_PROC_LOG */
	pr_info("%s: enable(%d) direction(%d)\n", __func__, enable, direction);
}

static void usb_vendor_receiver_cardnum(struct data_cardnum *data)
{
	int card_num = data->card_num;
	int bundle = data->bundle;
	int attach = data->attach;

	set_usb_audio_cardnum(card_num, bundle, attach);
}

static void usb_vendor_receiver_audio_uevent(struct data_audio_uevent *data)
{
	struct usb_device *dev = data->dev;
	int card_num = data->card_num;
	int attach = data->attach;

	send_usb_audio_uevent(dev, card_num, attach);
}

static void usb_vendor_receiver_new_device(struct data_new_device *data)
{
	struct usb_device *dev = data->dev;
	int ret = 0;

	ret = check_new_device_added(dev);
	if (ret)
		data->ret = ret;
}

static int usb_vendor_receiver_callback(struct notifier_block *nb,
		unsigned long action, void *data)
{
	switch (action) {
	case USB_VENDOR_NOTIFY_PCM_INFO:
		usb_vendor_receiver_pcm_info(data);
		break;
	case USB_VENDOR_NOTIFY_CARDNUM:
		usb_vendor_receiver_cardnum(data);
		break;
	case USB_VENDOR_NOTIFY_AUDIO_UEVENT:
		usb_vendor_receiver_audio_uevent(data);
		break;
	case USB_VENDOR_NOTIFY_NEW_DEVICE:
		usb_vendor_receiver_new_device(data);
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

#if IS_ENABLED(CONFIG_OF)
static struct platform_device *usb_vendor_receiver_get_notify(struct device *dev)
{
	struct platform_device *notify_pdev;
	struct device_node *notify_node;
	int ret = 0;

	notify_node = of_parse_phandle(dev->of_node, "notify", 0);
	if (!notify_node) {
		pr_err("%s: failed to get notify_node\n", __func__);
		ret = -ENODEV;
		goto out;
	}

	notify_pdev = of_find_device_by_node(notify_node);
	if (!notify_pdev) {
		pr_err("%s: failed to get notify_pdev\n", __func__);
		ret = -ENODEV;
		goto out;
	}

	return notify_pdev;
out:
	return ERR_PTR(ret);
}
#endif /* CONFIG_OF */

static int usb_vendor_receiver_probe(struct platform_device *pdev)
{
	struct platform_device *notify_pdev;
	int ret = 0;

#if IS_ENABLED(CONFIG_OF)
	notify_pdev = usb_vendor_receiver_get_notify(&pdev->dev);
	if (IS_ERR(notify_pdev)) {
		ret = -ENODEV;
		goto out;
	}
#else
	pr_err("%s: failed to get notify_pdev\n", __func__);
	ret = -ENODEV;
	goto out;
#endif /* CONFIG_OF */

	nh = platform_get_drvdata(notify_pdev);
	if (!nh) {
		pr_err("%s: failed to get notifier head\n", __func__);
		ret = -ENODEV;
		goto out;
	}

	nb.notifier_call = usb_vendor_receiver_callback;
	ret = blocking_notifier_chain_register(nh, &nb);
	if (ret) {
		pr_err("%s: failed blocking_notifier_chain_register(%d)\n",
				__func__, ret);
		nh = NULL;
		goto out;
	}
	pr_info("%s done\n", __func__);
out:
	return ret;
}

static int usb_vendor_receiver_remove(struct platform_device *pdev)
{
	if (nh)
		blocking_notifier_chain_unregister(nh, &nb);
	nh = NULL;
	return 0;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id usb_vendor_receiver_dt_ids[] = {
	{ .compatible = "samsung,usb_vendor_receiver" },
	{ }
};
MODULE_DEVICE_TABLE(of, usb_vendor_receiver_dt_ids);
#endif /* CONFIG_OF */

static struct platform_driver usb_vendor_receiver_driver = {
	.probe		= usb_vendor_receiver_probe,
	.remove		= usb_vendor_receiver_remove,
	.driver		= {
		.name		= "usb_vendor_receiver",
		.owner		= THIS_MODULE,
#if IS_ENABLED(CONFIG_OF)
		.of_match_table	= of_match_ptr(usb_vendor_receiver_dt_ids),
#endif /* CONFIG_OF */
	},
};

static int __init usb_vendor_receiver_init(void)
{
	return platform_driver_register(&usb_vendor_receiver_driver);
}

static void __exit usb_vendor_receiver_exit(void)
{
	platform_driver_unregister(&usb_vendor_receiver_driver);
}

module_init(usb_vendor_receiver_init);
module_exit(usb_vendor_receiver_exit);

MODULE_DESCRIPTION("usb vendor receiver");
MODULE_LICENSE("GPL");
