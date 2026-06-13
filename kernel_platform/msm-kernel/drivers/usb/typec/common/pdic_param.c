// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (C) 2021 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */
#define pr_fmt(fmt) "pdic_param: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/usb/typec/common/pdic_param.h>

static const char * const pdic_param_mode[] = {
	[PDIC_PARAM_MODE_NO]	= "Normal",
	[PDIC_PARAM_MODE_OB]	= "OB",
	[PDIC_PARAM_MODE_IB]	= "IB",
	[PDIC_PARAM_MODE_DL]	= "DL",
	[PDIC_PARAM_MODE_LC]	= "LC",
};

#if IS_ENABLED(CONFIG_USB_FACTORY_MODE) && ((IS_MODULE(CONFIG_SEC_PARAM) || IS_ENABLED(CONFIG_SEC_MPARAM)) && !IS_ENABLED(CONFIG_PDIC_USE_MODULE_PARAM))
extern char *f_mode;
#else
static char *f_mode;
#endif

#if IS_ENABLED(CONFIG_IFPMIC_BRINGUP)
extern unsigned int lpcharge;
extern int factory_mode;
#else
static unsigned int lpcharge;
static int factory_mode;
#endif

static int pdic_param_lpcharge = -1;
module_param(pdic_param_lpcharge, int, 0444);

static int pdic_param_factory_mode = -1;
module_param(pdic_param_factory_mode, int, 0444);

static char __read_mostly *f_usb_mode;
module_param(f_usb_mode, charp, 0444);

static int pdic_param_recovery_mode;
module_param(pdic_param_recovery_mode, int, 0444);

static unsigned int usb_mode = PDIC_PARAM_MODE_NO;
#if IS_BUILTIN(CONFIG_PDIC_NOTIFIER)
static int __init read_f_mode(char *str)
{
	if (strncmp(str, "LC", 2) == 0)
		usb_mode = PDIC_PARAM_MODE_LC;
	else if (strncmp(str, "IB", 2) == 0)
		usb_mode = PDIC_PARAM_MODE_IB;
	else if (strncmp(str, "OB", 2) == 0)
		usb_mode = PDIC_PARAM_MODE_OB;
	else if (strncmp(str, "DL", 2) == 0)
		usb_mode = PDIC_PARAM_MODE_DL;
	else
		usb_mode = PDIC_PARAM_MODE_NO;
	return 0;
}
early_param("f_mode", read_f_mode);

static int __init set_pdic_param_lpm_charge(char *str)
{
	if (strncmp(str, "charger", 7) == 0)
		pdic_param_lpcharge = 1;
	else
		pdic_param_lpcharge = 0;
	pr_info("%s: Low power charging mode: %d\n",
		__func__, pdic_param_lpcharge);

	return 0;
}
early_param("androidboot.mode", set_pdic_param_lpm_charge);
#endif

int check_factory_mode_boot(void)
{
	int factory = 0;

	if (f_mode) {
		if (!strncmp(f_mode, "LC", 2))
			factory = 1;
		else if (!strncmp(f_mode, "IB", 2))
			factory = 1;
		else if (!strncmp(f_mode, "OB", 2))
			factory = 1;
		else if (!strncmp(f_mode, "DL", 2))
			factory = 1;
		else
			factory = 0;
		goto ret;
	}

	if (usb_mode != PDIC_PARAM_MODE_NO)
		factory = 1;

	if (f_usb_mode) {
		pr_info("%s f_usb_mode=%s\n", __func__, f_usb_mode);		
		factory = 1;
	}

ret:
	pr_info("%s factory=%d\n", __func__, factory);
	return factory;
}
EXPORT_SYMBOL_GPL(check_factory_mode_boot);

int get_usb_factory_mode(void)
{
	int mode = PDIC_PARAM_MODE_NO;

	if (f_mode) {
		if (!strncmp(f_mode, "LC", 2))
			mode = PDIC_PARAM_MODE_LC;
		else if (!strncmp(f_mode, "IB", 2))
			mode = PDIC_PARAM_MODE_IB;
		else if (!strncmp(f_mode, "OB", 2))
			mode = PDIC_PARAM_MODE_OB;
		else if (!strncmp(f_mode, "DL", 2))
			mode = PDIC_PARAM_MODE_DL;
		else
			mode = PDIC_PARAM_MODE_NO;
		goto ret;
	}

	mode = usb_mode;

ret:
	pr_info("%s mode=%s\n", __func__, pdic_param_mode[mode]);
	return mode;
}
EXPORT_SYMBOL_GPL(get_usb_factory_mode);

int is_lpcharge_pdic_param(void)
{
	int ret = 0;

	if (pdic_param_lpcharge != -1) {
		ret = pdic_param_lpcharge;
		goto out;
	}

	ret = lpcharge;

out:
	pr_info("%s lpcharge=%d\n", __func__, ret);
	return ret;
}
EXPORT_SYMBOL_GPL(is_lpcharge_pdic_param);

int is_factory_mode_pdic_param(void)
{
	int ret = 0;

	if (pdic_param_factory_mode != -1) {
		ret = pdic_param_factory_mode;
		goto out;
	}

	ret = factory_mode;

out:
	pr_info("%s factory_mode=%d\n", __func__, ret);
	return ret;
}
EXPORT_SYMBOL_GPL(is_factory_mode_pdic_param);

int is_recovery_mode_pdic_param(void)
{
	if (pdic_param_recovery_mode)
		pr_info("%s recovery_mode=%d\n", __func__, pdic_param_recovery_mode);
	return pdic_param_recovery_mode;
}
EXPORT_SYMBOL_GPL(is_recovery_mode_pdic_param);

