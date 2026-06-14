/*
 * sec_thermistor.c - SEC Thermistor
 *
 *  Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *  Minsung Kim <ms925.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/iio/consumer.h>
#include <linux/platform_data/sec_thermistor.h>
#include <linux/sec_class.h>

#define ADC_SAMPLING_CNT	1
#define THERMISTOR_NAME_LEN	32
#define FAKE_TEMP	300

struct sec_therm_info {
	int id;
	struct device *dev;
	struct device *sec_dev;
	struct sec_therm_platform_data *pdata;
	struct iio_channel *chan;
	char name[THERMISTOR_NAME_LEN];
	struct device_node *np;
	unsigned int sampling_cnt;
};

#ifdef CONFIG_OF
static const struct of_device_id sec_therm_match[] = {
	{ .compatible = "samsung,sec-thermistor", },
	{ },
};
MODULE_DEVICE_TABLE(of, sec_therm_match);

static int sec_therm_parse_dt(struct platform_device *pdev)
{
	struct sec_therm_info *info = platform_get_drvdata(pdev);
	struct sec_therm_platform_data *pdata;
	const char *name;
	int adc_arr_len, temp_arr_len;
	int i;
	u32 adc, tp;

	if (!info || !pdev->dev.of_node)
		return -ENODEV;

	info->np = pdev->dev.of_node;

	if (of_property_read_u32(info->np, "id", &info->id)) {
		dev_err(info->dev, "failed to get thermistor ID\n");
		return -EINVAL;
	}

	if (!of_property_read_string(info->np, "thermistor_name", &name)) {
		strlcpy(info->name, name, sizeof(info->name));
	} else {
		dev_err(info->dev, "failed to get thermistor name\n");
		return -EINVAL;
	}

	if (of_property_read_u32(info->np, "sampling_cnt", &info->sampling_cnt)) {
		dev_info(info->dev, "set sampling_cnt by default: %d\n",
				ADC_SAMPLING_CNT);
		info->sampling_cnt = ADC_SAMPLING_CNT;
	}

	pdata = devm_kzalloc(info->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	if (!of_get_property(info->np, "adc_array", &adc_arr_len))
		return -ENOENT;
	if (!of_get_property(info->np, "temp_array", &temp_arr_len))
		return -ENOENT;

	if (adc_arr_len != temp_arr_len) {
		dev_err(info->dev, "%s: invalid array length(%u,%u)\n",
				__func__, adc_arr_len, temp_arr_len);
		return -EINVAL;
	}

	pdata->iio_processed = of_property_read_bool(info->np, "use_iio_processed");
	pdata->adc_arr_size = adc_arr_len / sizeof(u32);
	pdata->adc_table = devm_kzalloc(&pdev->dev,
			sizeof(*pdata->adc_table) * pdata->adc_arr_size,
			GFP_KERNEL);
	if (!pdata->adc_table)
		return -ENOMEM;

	for (i = 0; i < pdata->adc_arr_size; i++) {
		if (of_property_read_u32_index(info->np, "adc_array", i, &adc))
			return -EINVAL;
		if (of_property_read_u32_index(info->np, "temp_array", i, &tp))
			return -EINVAL;

		pdata->adc_table[i].adc = (int)adc;
		pdata->adc_table[i].temperature = (int)tp;
	}

	info->pdata = pdata;

	return 0;
}
#else
static int sec_therm_parse_dt(struct platform_device *pdev) { return -ENODEV; }
#endif

static int sec_therm_read_adc_data(struct sec_therm_info *info, int *adc_data)
{
	int ret;

	if (info->pdata->iio_processed)
		ret = iio_read_channel_processed(info->chan, adc_data);
	else
		ret = iio_read_channel_raw(info->chan, adc_data);

	if (ret < 0) {
		dev_err(info->dev, "%s : err(%d), adc_data(%d) returned, skip read\n",
			__func__, ret, *adc_data);
	}

	return ret;
}

static int sec_therm_get_adc_data(struct sec_therm_info *info)
{
	int adc_data, ret;
	int adc_max = 0, adc_min = 0, adc_total = 0;
	int i;

	for (i = 0; i < info->sampling_cnt; i++) {
		ret = sec_therm_read_adc_data(info, &adc_data);
		if (ret < 0)
			return ret;

		if (info->sampling_cnt < 3)
			return adc_data;

		if (i != 0) {
			if (adc_data > adc_max)
				adc_max = adc_data;
			else if (adc_data < adc_min)
				adc_min = adc_data;
		} else {
			adc_max = adc_data;
			adc_min = adc_data;
		}
		adc_total += adc_data;
	}

	return (adc_total - adc_max - adc_min) / (info->sampling_cnt - 2);
}

static bool is_using_fake_temp(struct sec_therm_info *info)
{
	return !info->pdata->adc_table || !info->pdata->adc_arr_size;
}

static int get_closest_adc_table_idx(struct sec_therm_info *info, unsigned int adc)
{
	int low = 0;
	int high = info->pdata->adc_arr_size - 1;

	if (info->pdata->adc_table[low].adc >= adc)
		return low;
	else if (info->pdata->adc_table[high].adc <= adc)
		return high;

	return -1;
}

static int find_appropriate_temp(int *low, int *high, int adc, struct sec_therm_info *info)
{
	int mid;
	struct sec_therm_adc_table *mid_table;

	while (*low <= *high) {
		mid = (*low + *high) / 2;
		mid_table = &info->pdata->adc_table[mid];

		if (mid_table->adc > adc)
			*high = mid - 1;
		else if (mid_table->adc < adc)
			*low = mid + 1;
		else
			return mid_table->temperature;
	}

	return -1; // Not found
}

static int calculate_temp(int low, int high, int adc, struct sec_therm_info *info)
{
	int temp = info->pdata->adc_table[high].temperature;
	int temp_diff = (info->pdata->adc_table[low].temperature -
		info->pdata->adc_table[high].temperature) *
		(adc - info->pdata->adc_table[high].adc);

	temp += temp_diff /
		(info->pdata->adc_table[low].adc -
			info->pdata->adc_table[high].adc);

	return temp;
}

static int convert_adc_to_temp(struct sec_therm_info *info, unsigned int adc)
{
	int low = 0;
	int high = info->pdata->adc_arr_size - 1;
	int temp = 0;
	int idx = 0;

	if (is_using_fake_temp(info))
		return FAKE_TEMP;

	idx = get_closest_adc_table_idx(info, adc);
	if (idx != -1)
		return info->pdata->adc_table[idx].temperature;

	temp = find_appropriate_temp(&low, &high, adc, info);
	if (temp != -1)
		return temp;

	temp = calculate_temp(low, high, adc, info);

	return temp;
}

static ssize_t sec_therm_show_temperature(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sec_therm_info *info = dev_get_drvdata(dev);
	int adc, temp;

	adc = sec_therm_get_adc_data(info);

	if (adc >= 0)
		temp = convert_adc_to_temp(info, adc);
	else
		return adc;

	return sprintf(buf, "%d\n", temp);
}

static ssize_t sec_therm_show_temp_adc(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sec_therm_info *info = dev_get_drvdata(dev);
	int adc;

	adc = sec_therm_get_adc_data(info);

	return sprintf(buf, "%d\n", adc);
}

static ssize_t sec_therm_show_name(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_therm_info *info = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", info->name);
}

static DEVICE_ATTR(temperature, 0444, sec_therm_show_temperature, NULL);
static DEVICE_ATTR(temp_adc, 0444, sec_therm_show_temp_adc, NULL);
static DEVICE_ATTR(name, 0444, sec_therm_show_name, NULL);

static struct attribute *sec_therm_attrs[] = {
	&dev_attr_temperature.attr,
	&dev_attr_temp_adc.attr,
	&dev_attr_name.attr,
	NULL
};

static const struct attribute_group sec_therm_group = {
	.attrs = sec_therm_attrs,
};

static struct sec_therm_info *g_ap_therm_info;
int sec_therm_get_ap_temperature(void)
{
	int adc;
	int temp;

	if (unlikely(!g_ap_therm_info))
		return -ENODEV;

	adc = sec_therm_get_adc_data(g_ap_therm_info);

	if (adc >= 0)
		temp = convert_adc_to_temp(g_ap_therm_info, adc);
	else
		return adc;

	return temp;
}

static int sec_therm_probe(struct platform_device *pdev)
{
	struct sec_therm_info *info;
	int ret;

	dev_dbg(&pdev->dev, "%s: SEC Thermistor Driver Loading\n", __func__);

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	platform_set_drvdata(pdev, info);
	info->dev = &pdev->dev;

	ret = sec_therm_parse_dt(pdev);
	if (ret) {
		dev_err(info->dev, "%s: fail to parse dt\n", __func__);
		return ret;
	}

	info->chan = iio_channel_get(info->dev, NULL);
	if (IS_ERR(info->chan)) {
		dev_err(info->dev, "%s: fail to get iio channel(%lu)\n",
				__func__, PTR_ERR(info->chan));
		return PTR_ERR(info->chan);
	}

	info->sec_dev = sec_device_create(info, info->name);
	if (IS_ERR(info->sec_dev)) {
		dev_err(info->dev, "%s: fail to create sec_dev\n", __func__);
		return PTR_ERR(info->sec_dev);
	}

	ret = sysfs_create_group(&info->sec_dev->kobj, &sec_therm_group);
	if (ret) {
		dev_err(info->dev, "failed to create sysfs group\n");
		goto err_create_sysfs;
	}

	if (info->id == 0)
		g_ap_therm_info = info;

	dev_info(info->dev, "%s successfully probed.\n", info->name);

	return 0;

err_create_sysfs:
	sec_device_destroy(info->sec_dev->devt);
	return ret;
}

static int sec_therm_remove(struct platform_device *pdev)
{
	struct sec_therm_info *info = platform_get_drvdata(pdev);

	if (!info)
		return 0;

	if (info->id == 0)
		g_ap_therm_info = NULL;

	sysfs_remove_group(&info->sec_dev->kobj, &sec_therm_group);
	iio_channel_release(info->chan);
	sec_device_destroy(info->sec_dev->devt);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver sec_thermistor_driver = {
	.driver = {
		.name = "sec-thermistor",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(sec_therm_match),
	},
	.probe = sec_therm_probe,
	.remove = sec_therm_remove,
};

module_platform_driver(sec_thermistor_driver);

MODULE_DESCRIPTION("SEC Thermistor Driver");
MODULE_AUTHOR("Minsung Kim <ms925.kim@samsung.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sec-thermistor");
