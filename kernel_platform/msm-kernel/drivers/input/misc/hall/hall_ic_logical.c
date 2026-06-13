/*
 *
 * Copyright 2021 SEC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include <linux/pm_wakeup.h>
#include <linux/input/sec_input.h>

#if IS_ENABLED(CONFIG_KEYBOARD_STM32_POGO_V3)
#define POGO_NOTIFIER_ENABLED
#include <linux/input/pogo_notifier_v3.h>
#endif

/*
 * Switch events
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#define SW_FLIP				0x10  /* set = flip cover open, close*/
#define SW_HALL_LOGICAL		0x0a  /* set = logical hall ic attach/detach */
#else
#define SW_FLIP				0x15  /* set = flip cover open, close*/
#define SW_HALL_LOGICAL		0x1f  /* set = logical hall ic attach/detach */
#endif
#define SW_POGO_HALL		0x0d  /* set = pogo cover attach/detach */

enum LID_POSITION {
	E_LID_0 = 1,
	E_LID_NORMAL = 2,
	E_LID_360 = 3,
};

enum LOGICAL_HALL_STATUS {
	LOGICAL_HALL_OPEN = 0,
	LOGICAL_HALL_CLOSE = 1,
	LOGICAL_HALL_BACK = 2,
};

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
struct device *hall_logical;
#endif

struct hall_drvdata {
	struct input_dev		*input;
#ifdef POGO_NOTIFIER_ENABLED
	struct notifier_block		pogo_nb;
#endif
};

static int hall_logical_status;
static int hall_backflip_status;
static int pogo_cover_status;

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
static ssize_t hall_logical_detect_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	if (hall_logical_status == LOGICAL_HALL_OPEN)
		sprintf(buf, "OPEN\n");
	else if (hall_logical_status == LOGICAL_HALL_CLOSE)
		sprintf(buf, "CLOSE\n");
	else
		sprintf(buf, "BACK\n");

	return strlen(buf);
}
static DEVICE_ATTR_RO(hall_logical_detect);
#endif

static int hall_logical_open(struct input_dev *input)
{
	/* Report current state of buttons that are connected to GPIOs */
	input_report_switch(input, SW_FLIP, hall_logical_status);
	input_sync(input);

	return 0;
}

static int of_hall_data_parsing_dt(struct device *dev, struct hall_drvdata *ddata)
{
	return 0;
}

#ifdef POGO_NOTIFIER_ENABLED
static int hall_logical_notifier_handler(struct notifier_block *nb,
		unsigned long action, void *data)
{
	struct hall_drvdata *hall_logical_dev = container_of(nb, struct hall_drvdata, pogo_nb);
	struct pogo_data_struct pogo_data =  *(struct pogo_data_struct *)data;
	int hall_status;

	switch (action) {
	case POGO_NOTIFIER_ID_DETACHED:
		hall_logical_status = LOGICAL_HALL_OPEN;
		input_report_switch(hall_logical_dev->input, SW_FLIP, hall_logical_status);
		input_sync(hall_logical_dev->input);
		break;
	case POGO_NOTIFIER_EVENTID_HALL:
		if (pogo_data.size != 1) {
			pr_info("%s size is wrong. size=%d!\n", __func__, pogo_data.size);
			break;
		}

		hall_status = *pogo_data.data;

		if (hall_status == E_LID_0) {
			hall_logical_status = LOGICAL_HALL_CLOSE;
			input_info(true, &hall_logical_dev->input->dev, "%s hall_status = %d (CLOSE)\n", __func__, hall_status);
			input_report_switch(hall_logical_dev->input, SW_FLIP, hall_logical_status);
			input_sync(hall_logical_dev->input);
		} else if (hall_status == E_LID_NORMAL) {
			hall_logical_status = LOGICAL_HALL_OPEN;
			hall_backflip_status = 0;
			input_info(true, &hall_logical_dev->input->dev,
								"%s hall_status = %d (NORMAL)\n", __func__, hall_status);
			input_report_switch(hall_logical_dev->input, SW_FLIP, hall_logical_status);
			input_report_switch(hall_logical_dev->input, SW_HALL_LOGICAL, hall_backflip_status);
			input_sync(hall_logical_dev->input);
		} else if (hall_status == E_LID_360) {
			hall_backflip_status = 1;
			input_info(true, &hall_logical_dev->input->dev,
								"%s hall_status = %d (BACK)\n", __func__, hall_status);
			input_report_switch(hall_logical_dev->input, SW_HALL_LOGICAL, hall_backflip_status);
			input_sync(hall_logical_dev->input);
		}
		break;
#if IS_ENABLED(CONFIG_KEYBOARD_STM32_POGO_V3)
	case POGO_NOTIFIER_EVENTID_ACESSORY:
		if (pogo_data.size != 2) {
			pr_info("%s size is wrong. size=%d!\n", __func__, pogo_data.size);
			break;
		}
		pogo_cover_status = pogo_data.data[1];
		input_info(true, &hall_logical_dev->input->dev, "%s pogo_cover_status = %d\n",
					__func__, pogo_cover_status);
		input_report_switch(hall_logical_dev->input, SW_POGO_HALL, pogo_cover_status);
		input_sync(hall_logical_dev->input);
		break;
#endif
	default:
		break;
	};

	return NOTIFY_DONE;
}
#endif

static int hall_logical_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hall_drvdata *ddata;
	struct input_dev *input;
	int error;

	pr_info("%s called", __func__);
	ddata = kzalloc(sizeof(struct hall_drvdata), GFP_KERNEL);
	if (!ddata) {
		dev_err(dev, "failed to allocate state\n");
		return -ENOMEM;
	}

	if (dev->of_node) {
		error = of_hall_data_parsing_dt(dev, ddata);
		if (error < 0) {
			pr_info("%s : fail to get the dt (HALL)\n", __func__);
			goto fail1;
		}
	}

	input = input_allocate_device();
	if (!input) {
		dev_err(dev, "failed to allocate state\n");
		error = -ENOMEM;
		goto fail1;
	}

	ddata->input = input;

	platform_set_drvdata(pdev, ddata);
	input_set_drvdata(input, ddata);

	input->name = "hall_logical";
	input->phys = "hall_logical";
	input->dev.parent = &pdev->dev;

	input->evbit[0] |= BIT_MASK(EV_SW);
	input_set_capability(input, EV_SW, SW_FLIP);
	input_set_capability(input, EV_SW, SW_HALL_LOGICAL);
	input_set_capability(input, EV_SW, SW_POGO_HALL);

	input->open = hall_logical_open;

	/* Enable auto repeat feature of Linux input subsystem */
	__set_bit(EV_REP, input->evbit);

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	hall_logical = sec_device_create(ddata, "hall_logical");
	if (IS_ERR(hall_logical))
		dev_err(dev, "%s: failed to create device for the sysfs\n", __func__);

	error = device_create_file(hall_logical, &dev_attr_hall_logical_detect);
	if (error < 0) {
		pr_err("Failed to create device file(%s)!, error: %d\n",
		dev_attr_hall_logical_detect.attr.name, error);
	}
#endif
	error = input_register_device(input);
	if (error) {
		dev_err(dev, "Unable to register input device, error: %d\n",
			error);
		goto fail1;
	}

#ifdef POGO_NOTIFIER_ENABLED
	pogo_notifier_register(&ddata->pogo_nb,
			hall_logical_notifier_handler, POGO_NOTIFY_DEV_HALLIC);
#endif

	input_report_switch(input, SW_FLIP, hall_logical_status);
	input_report_switch(input, SW_HALL_LOGICAL, hall_backflip_status);
	input_report_switch(input, SW_POGO_HALL, pogo_cover_status);
	input_info(true, dev, "%s hall_status = %d backflip_status = %d pogo_cover_status = %d\n",
			__func__, hall_logical_status, hall_backflip_status, pogo_cover_status);

	pr_info("%s end", __func__);
	return 0;

fail1:
	kfree(ddata);

	return error;
}

static int hall_logical_remove(struct platform_device *pdev)
{
	struct hall_drvdata *ddata = platform_get_drvdata(pdev);
	struct input_dev *input = ddata->input;

	pr_info("%s start\n", __func__);

	input_unregister_device(input);
#ifdef POGO_NOTIFIER_ENABLED
	pogo_notifier_unregister(&ddata->pogo_nb);
#endif

	kfree(ddata);

	return 0;
}

static const struct of_device_id hall_logical_dt_ids[] = {
	{ .compatible = "hall_logical" },
	{ },
};
MODULE_DEVICE_TABLE(of, hall_logical_dt_ids);

static int hall_logical_suspend(struct device *dev)
{
	pr_info("%s start\n", __func__);

	return 0;
}

static int hall_logical_resume(struct device *dev)
{
	struct hall_drvdata *ddata = dev_get_drvdata(dev);
	struct input_dev *input = ddata->input;

	pr_info("%s start\n", __func__);
	input_sync(input);
	return 0;
}

static SIMPLE_DEV_PM_OPS(hall_logical_pm_ops, hall_logical_suspend, hall_logical_resume);

static struct platform_driver hall_device_driver = {
	.probe		= hall_logical_probe,
	.remove		= hall_logical_remove,
	.driver		= {
		.name	= "hall_logical",
		.owner	= THIS_MODULE,
		.pm	= &hall_logical_pm_ops,
		.of_match_table	= hall_logical_dt_ids,
	}
};

int hall_logical_init(void)
{
	pr_info("%s start\n", __func__);

	return platform_driver_probe(&hall_device_driver, hall_logical_probe);
}
EXPORT_SYMBOL(hall_logical_init);


void hall_logical_exit(void)
{
	pr_info("%s start\n", __func__);
	platform_driver_unregister(&hall_device_driver);
}
EXPORT_SYMBOL(hall_logical_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("Hall IC logical driver for GPIOs");
