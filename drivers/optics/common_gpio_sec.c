/*
 *  common_gpio_sec.c - Linux kernel modules for sensortek stk6d2x
 *  ambient light sensor (Common function)
 *
 *  Copyright (C) 2019 Bk, sensortek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/input.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/pm_wakeup.h>
#include <common_define.h>

typedef struct gpio_manager gpio_manager;

struct gpio_manager
{
	struct work_struct          stk_work;
	struct workqueue_struct     *stk_wq;

	stk_gpio_info                *gpio_info;
} gpio_mgr_default = {.gpio_info = 0};

#define MAX_LINUX_GPIO_MANAGER_NUM      5

gpio_manager linux_gpio_mgr[MAX_LINUX_GPIO_MANAGER_NUM];

static gpio_manager* parser_work(struct work_struct *work)
{
	int gpio_idx = 0;

	if (!work)
	{
		return NULL;
	}

	for (gpio_idx = 0; gpio_idx < MAX_LINUX_GPIO_MANAGER_NUM; gpio_idx ++)
	{
		if (&linux_gpio_mgr[gpio_idx].stk_work == work)
		{
			return &linux_gpio_mgr[gpio_idx];
		}
	}

	return NULL;
}

static void gpio_callback(struct work_struct *work)
{
	gpio_manager *gpio_mgr = parser_work(work);

	if (!gpio_mgr)
	{
		return;
	}

	gpio_mgr->gpio_info->gpio_cb(gpio_mgr->gpio_info);
	enable_irq(gpio_mgr->gpio_info->irq);
}

static irqreturn_t stk_gpio_irq_handler(int irq, void *data)
{
	gpio_manager *pData = data;
	disable_irq_nosync(irq);
	queue_work(pData->stk_wq, &pData->stk_work);
	return IRQ_HANDLED;
}

int register_gpio_irq(stk_gpio_info *gpio_info)
{
	int gpio_idx = 0;
	int irq;
	int err = 0;

	if (!gpio_info)
	{
		return -1;
	}

	for (gpio_idx = 0; gpio_idx < MAX_LINUX_GPIO_MANAGER_NUM; gpio_idx ++)
	{
		if (!linux_gpio_mgr[gpio_idx].gpio_info)
		{
			linux_gpio_mgr[gpio_idx].gpio_info = gpio_info;
			break;
		}
		else
		{
			if (linux_gpio_mgr[gpio_idx].gpio_info == gpio_info)
			{
				//already register
				return -1;
			}
		}
	}

	if (gpio_idx >= MAX_LINUX_GPIO_MANAGER_NUM)
	{
		printk(KERN_ERR "%s: proper gpio not found", __func__);
		return -1;
	}

	printk(KERN_INFO "%s: irq num = %d \n", __func__, gpio_info->int_pin);
	err = gpio_request(gpio_info->int_pin, "stk-int");

	if (err < 0)
	{
		printk(KERN_ERR "%s: gpio_request, err=%d", __func__, err);
		return err;
	}

	linux_gpio_mgr[gpio_idx].stk_wq = create_singlethread_workqueue(linux_gpio_mgr[gpio_idx].gpio_info->wq_name);
	INIT_WORK(&linux_gpio_mgr[gpio_idx].stk_work, gpio_callback);
	err = gpio_direction_input(linux_gpio_mgr[gpio_idx].gpio_info->int_pin);

	if (err < 0)
	{
		printk(KERN_ERR "%s: gpio_direction_input, err=%d", __func__, err);
		return err;
	}

	irq = gpio_to_irq(linux_gpio_mgr[gpio_idx].gpio_info->int_pin);
	printk(KERN_INFO "%s: int pin #=%d, irq=%d\n", __func__, linux_gpio_mgr[gpio_idx].gpio_info->int_pin, irq);

	if (irq < 0)
	{
		printk(KERN_ERR "irq number is not specified, irq # = %d, int pin=%d\n", irq, linux_gpio_mgr[gpio_idx].gpio_info->int_pin);
		return irq;
	}

	linux_gpio_mgr[gpio_idx].gpio_info->irq = irq;

	switch (linux_gpio_mgr[gpio_idx].gpio_info->trig_type)
	{
		case TRIGGER_RISING:
			err = request_any_context_irq(irq, stk_gpio_irq_handler, IRQF_TRIGGER_RISING, \
										  linux_gpio_mgr[gpio_idx].gpio_info->device_name, &linux_gpio_mgr[gpio_idx]);
			break;

		case TRIGGER_FALLING:
			err = request_any_context_irq(irq, stk_gpio_irq_handler, IRQF_TRIGGER_FALLING, \
										  linux_gpio_mgr[gpio_idx].gpio_info->device_name, &linux_gpio_mgr[gpio_idx]);
			break;

		case TRIGGER_HIGH:
		case TRIGGER_LOW:
			err = request_any_context_irq(irq, stk_gpio_irq_handler, IRQF_TRIGGER_LOW, \
										  linux_gpio_mgr[gpio_idx].gpio_info->device_name, &linux_gpio_mgr[gpio_idx]);
			break;

		default:
			break;
	}

	if (err < 0)
	{
		printk(KERN_WARNING "%s: request_any_context_irq(%d) failed for (%d)\n", __func__, irq, err);
		goto err_request_any_context_irq;
	}

	linux_gpio_mgr[gpio_idx].gpio_info->is_exist = true;
	return 0;
err_request_any_context_irq:
	gpio_free(linux_gpio_mgr[gpio_idx].gpio_info->int_pin);
	return err;
}

int start_gpio_irq(stk_gpio_info *gpio_info)
{
	int gpio_idx = 0;

	for (gpio_idx = 0; gpio_idx < MAX_LINUX_GPIO_MANAGER_NUM; gpio_idx ++)
	{
		if (linux_gpio_mgr[gpio_idx].gpio_info == gpio_info)
		{
			if (linux_gpio_mgr[gpio_idx].gpio_info->is_exist)
			{
				if (!linux_gpio_mgr[gpio_idx].gpio_info->is_active)
				{
					linux_gpio_mgr[gpio_idx].gpio_info->is_active = true;
				}
			}

			return 0;
		}
	}

	return -1;
}

int stop_gpio_irq(stk_gpio_info *gpio_info)
{
	int gpio_idx = 0;

	for (gpio_idx = 0; gpio_idx < MAX_LINUX_GPIO_MANAGER_NUM; gpio_idx ++)
	{
		if (linux_gpio_mgr[gpio_idx].gpio_info == gpio_info)
		{
			if (linux_gpio_mgr[gpio_idx].gpio_info->is_exist)
			{
				if (linux_gpio_mgr[gpio_idx].gpio_info->is_active)
				{
					linux_gpio_mgr[gpio_idx].gpio_info->is_active = false;
				}
			}

			return 0;
		}
	}

	return -1;
}

int remove_gpio_irq(stk_gpio_info *gpio_info)
{
	int gpio_idx = 0;

	for (gpio_idx = 0; gpio_idx < MAX_LINUX_GPIO_MANAGER_NUM; gpio_idx ++)
	{
		if (linux_gpio_mgr[gpio_idx].gpio_info == gpio_info)
		{
			if (linux_gpio_mgr[gpio_idx].gpio_info->is_exist)
			{
				if (linux_gpio_mgr[gpio_idx].gpio_info->is_active)
				{
					linux_gpio_mgr[gpio_idx].gpio_info->is_active = false;
					free_irq(linux_gpio_mgr[gpio_idx].gpio_info->irq, &linux_gpio_mgr[gpio_idx]);
					gpio_free(linux_gpio_mgr[gpio_idx].gpio_info->int_pin);
					cancel_work_sync(&linux_gpio_mgr[gpio_idx].stk_work);
				}
			}

			return 0;
		}
	}

	return -1;
}

const struct stk_gpio_ops stk_g_ops =
{
	.register_gpio_irq      = register_gpio_irq,
	.start_gpio_irq         = start_gpio_irq,
	.stop_gpio_irq          = stop_gpio_irq,
	.remove                 = remove_gpio_irq,

};
