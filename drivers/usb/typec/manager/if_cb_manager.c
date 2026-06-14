/*
 * Copyright (C) 2018-2019 Samsung Electronics
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
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/usb/typec/manager/if_cb_manager.h>
#include <linux/slab.h>
#include <linux/module.h>

static struct if_cb_manager *man_core;

struct if_cb_manager *create_alloc_if_cb_manager(void)
{
	man_core = kzalloc(sizeof(struct if_cb_manager), GFP_KERNEL);

	return man_core;
}

struct if_cb_manager *get_if_cb_manager(void)
{
	if (!man_core)
		create_alloc_if_cb_manager();

	return man_core;
}

struct if_cb_manager *register_usb(struct usb_dev *usb)
{
	struct if_cb_manager *man_core;

	man_core = get_if_cb_manager();
	man_core->usb_d = usb;

	return man_core;
}
EXPORT_SYMBOL(register_usb);

struct if_cb_manager *register_muic(struct muic_dev *muic)
{
	struct if_cb_manager *man_core;

	man_core = get_if_cb_manager();
	man_core->muic_d = muic;

	return man_core;
}
EXPORT_SYMBOL(register_muic);

struct if_cb_manager *register_usbpd(struct usbpd_dev *usbpd)
{
	struct if_cb_manager *man_core;

	man_core = get_if_cb_manager();
	man_core->usbpd_d = usbpd;

	return man_core;
}
EXPORT_SYMBOL(register_usbpd);

struct if_cb_manager *register_lvs(struct lvs_dev *lvs)
{
	struct if_cb_manager *man_core;

	man_core = get_if_cb_manager();
	man_core->lvs_d = lvs;

	return man_core;
}
EXPORT_SYMBOL(register_lvs);

void usb_set_vbus_current(struct if_cb_manager *man_core, int state)
{
	if (man_core == NULL || man_core->usb_d == NULL ||
			man_core->usb_d->ops == NULL ||
			man_core->usb_d->ops->usb_set_vbus_current == NULL) {
		pr_err("%s : Member of if_cb_manager is NULL\n", __func__);
		return;
	}

	man_core->usb_d->ops->usb_set_vbus_current(
			man_core->usb_d->data, state);
}
EXPORT_SYMBOL(usb_set_vbus_current);

int usb_restart_host_mode(struct if_cb_manager *man_core, int lanes)
{
	if (man_core == NULL || man_core->usb_d == NULL ||
			man_core->usb_d->ops == NULL ||
			man_core->usb_d->ops->usb_restart_host_mode == NULL) {
		pr_err("%s : Member of if_cb_manager is NULL\n", __func__);
		return -ENXIO;
	}

	return man_core->usb_d->ops->usb_restart_host_mode(
			man_core->usb_d->data, lanes);
}
EXPORT_SYMBOL(usb_restart_host_mode);

int muic_check_usb_killer(struct if_cb_manager *man_core)
{
	if (man_core == NULL || man_core->muic_d == NULL ||
			man_core->muic_d->ops == NULL ||
			man_core->muic_d->ops->muic_check_usb_killer == NULL) {
		pr_err("%s : Member of if_cb_manager is NULL\n", __func__);
		return 0;
	}

	return man_core->muic_d->ops->muic_check_usb_killer(
			man_core->muic_d->data);
}
EXPORT_SYMBOL(muic_check_usb_killer);

void muic_set_bc12(struct if_cb_manager *man_core, int enable)
{
	if (man_core == NULL || man_core->muic_d == NULL ||
			man_core->muic_d->ops == NULL ||
			man_core->muic_d->ops->muic_set_bc12 == NULL) {
		pr_err("%s : Member of if_cb_manager is NULL\n", __func__);
		return;
	}

	man_core->muic_d->ops->muic_set_bc12(
			man_core->muic_d->data, enable);
}
EXPORT_SYMBOL(muic_set_bc12);

int usbpd_sbu_test_read(struct if_cb_manager *man_core)
{
	if (man_core == NULL || man_core->usbpd_d == NULL ||
			man_core->usbpd_d->ops == NULL ||
			man_core->usbpd_d->ops->usbpd_sbu_test_read == NULL) {
		pr_err("%s : Member of if_cb_manager is NULL\n", __func__);
		return -ENXIO;
	}

	return man_core->usbpd_d->ops->usbpd_sbu_test_read(
			man_core->usbpd_d->data);
}
EXPORT_SYMBOL(usbpd_sbu_test_read);

void usbpd_set_host_on(struct if_cb_manager *man_core, int mode)
{
	if (man_core == NULL || man_core->usbpd_d == NULL ||
			man_core->usbpd_d->ops == NULL ||
			man_core->usbpd_d->ops->usbpd_set_host_on == NULL) {
		pr_err("%s : Member of if_cb_manager is NULL\n", __func__);
		return;
	}

	man_core->usbpd_d->ops->usbpd_set_host_on(
		man_core->usbpd_d->data, mode);
}
EXPORT_SYMBOL(usbpd_set_host_on);

void usbpd_cc_control_command(struct if_cb_manager *man_core, int is_off)
{
	if (man_core == NULL || man_core->usbpd_d == NULL ||
			man_core->usbpd_d->ops == NULL ||
			man_core->usbpd_d->ops->usbpd_cc_control_command == NULL) {
		pr_err("%s : Member of if_cb_manager is NULL\n", __func__);
		return;
	}

	man_core->usbpd_d->ops->usbpd_cc_control_command(
		man_core->usbpd_d->data, is_off);
}
EXPORT_SYMBOL(usbpd_cc_control_command);

void usbpd_wait_entermode(struct if_cb_manager *man_core, int on)
{
	if (man_core == NULL || man_core->usbpd_d == NULL ||
			man_core->usbpd_d->ops == NULL ||
			man_core->usbpd_d->ops->usbpd_wait_entermode == NULL) {
		pr_err("%s : Member of if_cb_manager is NULL\n", __func__);
		return;
	}

	man_core->usbpd_d->ops->usbpd_wait_entermode(man_core->usbpd_d->data, on);
}
EXPORT_SYMBOL(usbpd_wait_entermode);

void usbpd_sbu_switch_control(int on)
{
	struct if_cb_manager *man_core = get_if_cb_manager();

	if (!IS_ENABLED(CONFIG_SBU_SWITCH_CONTROL) || man_core == NULL ||
			man_core->usbpd_d == NULL || man_core->usbpd_d->ops == NULL ||
			man_core->usbpd_d->ops->usbpd_sbu_switch_control == NULL) {
		pr_err("%s : This function isn't supported\n", __func__);
		return;
	}

	man_core->usbpd_d->ops->usbpd_sbu_switch_control(man_core->usbpd_d->data, on);
}
EXPORT_SYMBOL(usbpd_sbu_switch_control);

void usbpd_water_pd_ta_notify(struct if_cb_manager *man_core)
{
	if (man_core == NULL || man_core->usbpd_d == NULL ||
			man_core->usbpd_d->ops == NULL ||
			man_core->usbpd_d->ops->usbpd_water_pd_ta_notify == NULL) {
		pr_err("%s : Member of if_cb_manager is NULL\n", __func__);
		return;
	}

	man_core->usbpd_d->ops->usbpd_water_pd_ta_notify(
		man_core->usbpd_d->data);
}
EXPORT_SYMBOL(usbpd_water_pd_ta_notify);

static int __init if_cb_manager_init(void)
{
	if (!man_core)
		create_alloc_if_cb_manager();
	return 0;
}

static void __exit if_cb_manager_exit(void)
{
	kfree(man_core);
	man_core = NULL;
}

module_init(if_cb_manager_init);
module_exit(if_cb_manager_exit);

MODULE_AUTHOR("Samsung USB Team");
MODULE_DESCRIPTION("Interface Callback Manager");
MODULE_LICENSE("GPL");
