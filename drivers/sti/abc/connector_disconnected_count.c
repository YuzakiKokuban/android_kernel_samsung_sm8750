
/*
 * connector_disconnected_count.c
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

static int connector_disconnected_count[DET_CONN_MAX_NUM_GPIOS];

/*
 * Increase disconnect count
 */
void increase_connector_disconnected_count(int index, struct sec_det_conn_info *pinfo)
{
	if ((index >= DET_CONN_MAX_NUM_GPIOS) || (index < 0))
		return;
	if (!gpio_get_value(pinfo->pdata->irq_gpio[index]))
		return;

	SEC_CONN_PRINT("%s status changed [disconnected]\n",
			pinfo->pdata->name[index]);
	if ((connector_disconnected_count[index] >= 0) && (connector_disconnected_count[index] < 9999))
		connector_disconnected_count[index]++;
}

/*
 * Get disconnect count
 */
int get_connector_disconnected_count(int index)
{
	if ((index >= DET_CONN_MAX_NUM_GPIOS) || (index < 0))
		return 0;

	return connector_disconnected_count[index];
}

/*
 * Clear Accumulated all disconnected count when character 'c','C' is stored.
 */
__visible_for_testing ssize_t connector_disconnected_count_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct sec_det_conn_p_data *pdata;
	int i;
	int buf_len;

	if (gpinfo == 0)
		return count;
	pdata = gpinfo->pdata;

	buf_len = strlen(buf);
	SEC_CONN_PRINT("buf = %s, buf_len = %d\n", buf, buf_len);

	/* disable irq when "enabled" value set to 0*/
	if ((strncmp(buf, "c", 1) == 0) || (strncmp(buf, "C", 1) == 0)) {
		for (i = 0; i < pdata->gpio_total_cnt; i++)
			connector_disconnected_count[i] = 0;
	}
	return count;
}

/*
 * Return the "name":"value" pair for connector disconnected count.
 * value 1 or more means connector disconnected, and 0 means connected.
 * "SUB_CONNECTOR":"3","UPPER_C2C_DET":"0"
 */
__visible_for_testing ssize_t connector_disconnected_count_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sec_det_conn_p_data *pdata;
	char connector_disconnected_count_str[1024] = {0, };
	char gpio_value_str[12] = {0, };
	int i;

	if (gpinfo == 0)
		return 0;
	pdata = gpinfo->pdata;

	for (i = 0; i < pdata->gpio_total_cnt; i++) {
		if (i > 0)
			strlcat(connector_disconnected_count_str, ",", 1024);
		strlcat(connector_disconnected_count_str, "\"", 1024);
		strlcat(connector_disconnected_count_str, pdata->name[i], 1024);
		strlcat(connector_disconnected_count_str, "\":\"", 1024);
		sprintf(gpio_value_str, "%d", get_connector_disconnected_count(i));
		strlcat(connector_disconnected_count_str, gpio_value_str, 1024);
		strlcat(connector_disconnected_count_str, "\"", 1024);
	}
	connector_disconnected_count_str[strlen(connector_disconnected_count_str)] = '\0';
	SEC_CONN_PRINT("connector_disconnected_count : %s\n", connector_disconnected_count_str);

	return snprintf(buf, 1020, "%s", connector_disconnected_count_str);
}
static DEVICE_ATTR_RW(connector_disconnected_count);

/*
 * Create sys node for connector disconnected count for bigdata
 */
void create_connector_disconnected_count_sysnode_file(struct sec_det_conn_info *pinfo)
{
	int ret = 0;

	/* Create sys node /sys/class/sec/sec_detect_conn/connector_disconnected_count */
	ret = device_create_file(pinfo->dev, &dev_attr_connector_disconnected_count);

	if (ret)
		pr_err("%s: Failed to create connector_disconnected_count.\n", __func__);
}
