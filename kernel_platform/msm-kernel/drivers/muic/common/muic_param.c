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
#define pr_fmt(fmt) "muic_param: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/muic/common/muic_param.h>

static int muic_param_pmic_info = -1;
module_param(muic_param_pmic_info, int, 0444);

static int muic_param_uart_sel = -1;
module_param(muic_param_uart_sel, int, 0444);

static int muic_param_afc_mode = -1;
module_param(muic_param_afc_mode, int, 0444);

static int muic_param_pdic_info = -1;
module_param(muic_param_pdic_info, int, 0444);

#if ((IS_MODULE(CONFIG_SEC_PARAM) || IS_ENABLED(CONFIG_SEC_MPARAM)) && !IS_ENABLED(CONFIG_MUIC_USE_MODULE_PARAM))
extern int pmic_info;
extern unsigned int charging_mode;
extern int ccic_info;
#else
static int pmic_info = -1;
static unsigned int charging_mode;
static int ccic_info = 1;
#endif

#if IS_BUILTIN(CONFIG_MUIC_NOTIFIER)
static int __init set_switch_sel(char *str)
{
	get_option(&str, &muic_param_pmic_info);
	pr_info("%s: pmic_info 0x%x\n", __func__, muic_param_pmic_info);

	return 1;
}
__setup("pmic_info=", set_switch_sel);

/* func : set_uart_sel for QC boot command
 * uart_sel value get from bootloader command line
 */
static int __init set_uart_sel(char *str)
{
	get_option(&str, &muic_param_uart_sel);
	pr_info("%s: uart_sel is 0x%02x\n", __func__, muic_param_uart_sel);

	return 0;
}
early_param("uart_sel", set_uart_sel);

/* afc_mode:
 *   0x31 : Disabled
 *   0x30 : Enabled
 */
/* for LSI boot command */
static int __init set_charging_mode(char *str)
{
	int mode;
	get_option(&str, &mode);
	muic_param_afc_mode = (mode & 0x0000FF00) >> 8;
	pr_info("%s: afc_mode is 0x%02x\n", __func__, muic_param_afc_mode);

	return 0;
}
early_param("charging_mode", set_charging_mode);

/* for QC boot command */
static int __init set_afc_disable(char *str)
{
	get_option(&str, &muic_param_afc_mode);
	pr_info("%s: afc_mode is 0x%02x\n", __func__, muic_param_afc_mode);

	return 0;
}
early_param("afc_disable", set_afc_disable);
/*
 * __pdic_info :
 * b'0: 1 if an active pdic is present,
 *        0 when muic works without pdic chip or
 *              no pdic Noti. registration is needed
 *              even though a pdic chip is present.
 */
static int __init set_pdic_info(char *str)
{
	get_option(&str, &muic_param_pdic_info);

	pr_info("%s: pdic_info: 0x%04x\n", __func__, muic_param_pdic_info);

	return 1;
}
__setup("ccic_info=", set_pdic_info);

#endif /* BUILTIN CONFIG_MUIC_NOTIFIER */

/*
 * switch_sel value get from bootloader command line
 * switch_sel data consist 8 bits (xxxxyyyyzzzz)
 * first 4bits(zzzz) mean path information.
 * next 4bits(yyyy) mean if pmic version info
 * next 4bits(xxxx) mean afc disable info
 */
int get_switch_sel(void)
{
	int local_pmic_info = 0, local_switch_sel = 0;
	
	if (muic_param_pmic_info != -1) {
		local_pmic_info = muic_param_pmic_info;
		goto out;
	}

	local_pmic_info = pmic_info;
out:
	local_switch_sel = local_pmic_info & 0xfff;
	pr_info("%s: switch_sel: 0x%03x\n", __func__, local_switch_sel);
	
	return local_switch_sel;
}
EXPORT_SYMBOL_GPL(get_switch_sel);

int get_uart_sel(void)
{
	int local_uart_sel = -1;

	if (muic_param_uart_sel != -1) {
		local_uart_sel = muic_param_uart_sel;
		goto out;
	}
out:
	pr_info("%s: get_uart_sel 0x%x\n", __func__, local_uart_sel);

	return local_uart_sel;
}
EXPORT_SYMBOL_GPL(get_uart_sel);

int get_afc_mode(void)
{
	int local_afc_mode = 0, local_charging_mode = 0;
	
	if (muic_param_afc_mode != -1) {
		local_afc_mode = muic_param_afc_mode;
		goto out;
	}

	local_charging_mode = charging_mode;
	local_afc_mode = (local_charging_mode & 0x0000FF00) >> 8;
out:
	pr_info("%s: afc_mode is 0x%02x\n", __func__, local_afc_mode);
	
	return local_afc_mode;
}
EXPORT_SYMBOL_GPL(get_afc_mode);

int get_pdic_info(void)
{
	int local_pdic_info = 1;
	
	if (muic_param_pdic_info != -1) {
		local_pdic_info = muic_param_pdic_info;
		goto out;
	}

	local_pdic_info = ccic_info;	
out:
	pr_info("%s: ccic_info: 0x%04x\n", __func__, local_pdic_info);
	
	return local_pdic_info;
}
EXPORT_SYMBOL_GPL(get_pdic_info);

