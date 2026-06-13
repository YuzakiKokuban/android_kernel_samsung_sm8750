/*
 * connection_state.c
 *
 * Copyright (C) 2022 Samsung Electronics
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
 */

#include <linux/sti/sec_abc_detect_conn.h>

#if defined(CONFIG_SEC_KUNIT)
#define __visible_for_testing
#else
#define __visible_for_testing static
#endif

/*
 * Get gpio value.
 */
int get_gpio_value(int gpio)
{
#if defined(CONFIG_SEC_KUNIT)
	return 1;
#else
	return gpio_get_value(gpio);
#endif
}

/*
 * Return connector total count.
 */
__visible_for_testing ssize_t connector_count_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sec_det_conn_p_data *pdata;

	if (gpinfo == 0)
		return 0;
	pdata = gpinfo->pdata;

	SEC_CONN_PRINT("read connector_count = %d\n", pdata->gpio_total_cnt);
	return snprintf(buf, 12, "%d\n", pdata->gpio_total_cnt);
}
static DEVICE_ATTR_RO(connector_count);

/*
 * Check all connector gpio state and return the "name":"value" pair.
 * value 1 means disconnected, and 0 means connected.
 * "SUB_CONNECTOR":"1","UPPER_C2C_DET":"0"
 */
__visible_for_testing ssize_t connector_state_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sec_det_conn_p_data *pdata;
	char connector_state[1024] = {0, };
	char gpio_value_str[12] = {0, };
	int i;

	if (gpinfo == 0)
		return 0;
	pdata = gpinfo->pdata;

	for (i = 0; i < pdata->gpio_total_cnt; i++) {
		if (i > 0)
			strlcat(connector_state, ",", 1024);
		strlcat(connector_state, "\"", 1024);
		strlcat(connector_state, pdata->name[i], 1024);
		strlcat(connector_state, "\":\"", 1024);
		SEC_CONN_PRINT("get value : %s[%d]\n", pdata->name[i], pdata->irq_gpio[i]);
		sprintf(gpio_value_str, "%d", get_gpio_value(pdata->irq_gpio[i]));
		strlcat(connector_state, gpio_value_str, 1024);
		strlcat(connector_state, "\"", 1024);
	}
	connector_state[strlen(connector_state)] = '\0';
	SEC_CONN_PRINT("connector_state : %s\n", connector_state);

	return snprintf(buf, 1024, "%s\n", connector_state);
}
static DEVICE_ATTR_RO(connector_state);

/*
 * Create sys node for connector connection state and connector count.
 */
void create_current_connection_state_sysnode_files(struct sec_det_conn_info *pinfo)
{
	int ret = 0;

	/* Create sys node /sys/class/sec/sec_detect_conn/connector_count */
	ret = device_create_file(pinfo->dev, &dev_attr_connector_count);

	if (ret)
		pr_err("%s: Failed to create connector_count.\n", __func__);

	/* Create sys node /sys/class/sec/sec_detect_conn/connector_state */
	ret = device_create_file(pinfo->dev, &dev_attr_connector_state);

	if (ret)
		pr_err("%s: Failed to create connector_state.\n", __func__);
}
