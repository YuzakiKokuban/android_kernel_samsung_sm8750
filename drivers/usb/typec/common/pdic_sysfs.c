// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (C) 2017-2021 Samsung Electronics
 *
 * Author:Wookwang Lee. <wookwang.lee@samsung.com>,
 * Author:Guneet Singh Khurana  <gs.khurana@samsung.com>,
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

#include <linux/types.h>
#include <linux/device.h>
#include <linux/usb/typec/common/pdic_core.h>
#include <linux/usb/typec/common/pdic_sysfs.h>
#if defined(CONFIG_SEC_KUNIT)
#include "kunit_test/pdic_sysfs_test.h"
#else
#define __visible_for_testing static
#endif

static ssize_t pdic_sysfs_show_property(struct device *dev,
				struct device_attribute *attr, char *buf);

static ssize_t pdic_sysfs_store_property(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define PDIC_SYSFS_ATTR(_name)				\
{							\
	.attr = { .name = #_name },			\
	.show = pdic_sysfs_show_property,		\
	.store = pdic_sysfs_store_property,		\
}

static struct device_attribute pdic_attributes[] = {
	PDIC_SYSFS_ATTR(chip_name),
	PDIC_SYSFS_ATTR(cur_version),
	PDIC_SYSFS_ATTR(src_version),
	PDIC_SYSFS_ATTR(lpm_mode),
	PDIC_SYSFS_ATTR(state),
	PDIC_SYSFS_ATTR(rid),
	PDIC_SYSFS_ATTR(ccic_control_option),
	PDIC_SYSFS_ATTR(booting_dry),
	PDIC_SYSFS_ATTR(fw_update),
	PDIC_SYSFS_ATTR(fw_update_status),
	PDIC_SYSFS_ATTR(water),
	PDIC_SYSFS_ATTR(dex_fan_uvdm),
	PDIC_SYSFS_ATTR(acc_device_version),
	PDIC_SYSFS_ATTR(debug_opcode),
	PDIC_SYSFS_ATTR(control_gpio),
	PDIC_SYSFS_ATTR(usbpd_ids),
	PDIC_SYSFS_ATTR(usbpd_type),
	PDIC_SYSFS_ATTR(cc_pin_status),
	PDIC_SYSFS_ATTR(ram_test),
	PDIC_SYSFS_ATTR(sbu_adc),
	PDIC_SYSFS_ATTR(cc_adc),
	PDIC_SYSFS_ATTR(vsafe0v_status),
	PDIC_SYSFS_ATTR(ovp_ic_shutdown),
	PDIC_SYSFS_ATTR(hmd_power),
	PDIC_SYSFS_ATTR(water_threshold),
	PDIC_SYSFS_ATTR(water_check),
	PDIC_SYSFS_ATTR(15mode_watertest_type),
	PDIC_SYSFS_ATTR(vbus_adc),
	PDIC_SYSFS_ATTR(usb_boot_mode),
	PDIC_SYSFS_ATTR(dp_sbu_sw_sel),
	PDIC_SYSFS_ATTR(novbus_rp22k),
	PDIC_SYSFS_ATTR(cable_connect_ctrl),
};

__visible_for_testing ssize_t get_pdic_sysfs_property(ppdic_data_t ppdic_data,
	const char *attr_name, ptrdiff_t off, char *buf)
{
	ssize_t ret = 0;
	ppdic_sysfs_property_t ppdic_sysfs;

	if (!ppdic_data || !attr_name || off < 0 || !buf)
		return -EINVAL;

	if (off == PDIC_SYSFS_PROP_CHIP_NAME)
		return snprintf(buf, PAGE_SIZE, "%s\n", ppdic_data->name);

	ppdic_sysfs = (ppdic_sysfs_property_t)ppdic_data->pdic_sysfs_prop;
	ret = ppdic_sysfs->get_property(ppdic_data, off, buf);
	if (ret < 0)
		pr_err("%s : driver failed to report `%s' property: %zd\n",
				__func__, attr_name, ret);

	return ret;
}

static ssize_t pdic_sysfs_show_property(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ppdic_data_t ppdic_data = dev_get_drvdata(dev);
	const ptrdiff_t off = attr - pdic_attributes;

	return get_pdic_sysfs_property(ppdic_data, attr->attr.name, off, buf);
}

__visible_for_testing ssize_t set_pdic_sysfs_property(ppdic_data_t ppdic_data,
	const char *attr_name, ptrdiff_t off, const char *buf, size_t count)
{
	ssize_t ret = 0;
	ppdic_sysfs_property_t ppdic_sysfs;

	if (!ppdic_data || !attr_name || off < 0 || !buf)
		return -EINVAL;

	ppdic_sysfs = (ppdic_sysfs_property_t)ppdic_data->pdic_sysfs_prop;
	ret = ppdic_sysfs->set_property(ppdic_data, off, buf, count);
	if (ret < 0)
		pr_err("%s : driver failed to set `%s' property: %zd\n",
			__func__, attr_name, ret);

	return ret;
}

static ssize_t pdic_sysfs_store_property(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ppdic_data_t ppdic_data = dev_get_drvdata(dev);
	const ptrdiff_t off = attr - pdic_attributes;

	return set_pdic_sysfs_property(ppdic_data, attr->attr.name, off, buf, count);
}

__visible_for_testing umode_t get_pdic_sysfs_attr(ppdic_data_t ppdic_data,
	int attrno)
{
	ppdic_sysfs_property_t ppdic_sysfs;
	umode_t mode = RO_PERM;
	int cnt, num_properties, property;

	if (!ppdic_data || attrno < 0)
		return 0;

	ppdic_sysfs = (ppdic_sysfs_property_t)ppdic_data->pdic_sysfs_prop;
	num_properties = ppdic_sysfs->num_properties;

	for (cnt = 0; cnt < num_properties; cnt++) {
		property = ppdic_sysfs->properties[cnt];
		if (property != attrno)
			continue;
		if (ppdic_sysfs->property_is_writeable &&
			ppdic_sysfs->property_is_writeable(ppdic_data, property))
			mode |= WO_PERM;
		if (ppdic_sysfs->property_is_writeonly &&
			ppdic_sysfs->property_is_writeonly(ppdic_data, property))
			mode = WO_PERM;
		return mode;
	}
	return 0;
}

static umode_t pdic_sysfs_attr_is_visible(struct kobject *kobj,
					struct attribute *attr, int attrno)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	ppdic_data_t ppdic_data = dev_get_drvdata(dev);

	return get_pdic_sysfs_attr(ppdic_data, attrno);
}

static struct attribute *__pdic_sysfs_attrs[ARRAY_SIZE(pdic_attributes) + 1];

const struct attribute_group pdic_sysfs_group = {
	.attrs = __pdic_sysfs_attrs,
	.is_visible = pdic_sysfs_attr_is_visible,
};

void pdic_sysfs_init_attrs(void)
{
	int i;

	for (i = 0; i < (int)ARRAY_SIZE(pdic_attributes); i++)
		__pdic_sysfs_attrs[i] = &pdic_attributes[i].attr;
}
