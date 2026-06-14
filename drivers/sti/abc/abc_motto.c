/* abc_motto.c
 *
 * Abnormal Behavior Catcher MOTTO Support
 *
 * Copyright (C) 2020 Samsung Electronics
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
#include <linux/sti/abc_common.h>

extern struct device *sec_abc;

#if defined(CONFIG_ARCH_QCOM)
static unsigned int qet_loaded;
module_param(qet_loaded, uint, 0440);
#endif

struct motto_event_type motto_event_type_list[] = {
	{MOTTO_MODULE_GPU, "gpu", "gpu_fault"},
	{MOTTO_MODULE_GPU, "gpu_qc", "gpu_fault"},
	{MOTTO_MODULE_DECON, "decon", "fence_timeout"},
	{MOTTO_MODULE_CAMERA, "camera", "camera_error"},
	{MOTTO_MODULE_CAMERA, "camera", "i2c_fail"},
	{MOTTO_MODULE_NPU, "npu", "npu_fw_warning"},
	{MOTTO_MODULE_NPU, "camera", "mipi_overflow"},
	// we can add more like
	// {MOTTO_MODULE_GPU, "gpu_qc", "gpu_new_event"}
	// {MOTTO_MODULE_CAMERA, "camera", "camera_new_event"},
	// {MOTTO_MODULE_NEW, "new", "new_something"}
};

static void motto_update_event(enum motto_event_module module)
{
	struct abc_info *pinfo;
	struct abc_motto_data *cmotto;

	pinfo = dev_get_drvdata(sec_abc);
	cmotto = pinfo->pdata->motto_data;

	if (cmotto->dev_err_count[module] == 0xff) // 0xff means max value of 8bit variable dev_err_count[module]
		return;

	cmotto->dev_err_count[module]++;

	return;
}

static enum motto_event_module motto_event_to_idx(char *module_str, char *event)
{
	int i, items = sizeof(motto_event_type_list) / sizeof(motto_event_type_list[0]);
	size_t module_str_len = strlen(module_str);

	for (i = 0; i < items; i++) {
		if (strlen(motto_event_type_list[i].motto_event_module_name) == module_str_len &&
			!strncmp(motto_event_type_list[i].motto_event_module_name, module_str, module_str_len) &&
			!strcmp(motto_event_type_list[i].motto_event_type_str, event))
			return motto_event_type_list[i].module;
	}
	return MOTTO_MODULE_NONE;
}
void get_motto_uevent_str(char *uevent_str)
{
	struct abc_info *pinfo;
	struct abc_motto_data *cmotto;
	char temp[9];
	int i;

	pinfo = dev_get_drvdata(sec_abc);
	cmotto = pinfo->pdata->motto_data;

	sprintf(uevent_str,"AME=%08x", cmotto->boot_time);

	for (i=0;i<MOTTO_MODULE_MAX;i++)
	{
		snprintf(temp,9,"%08x", cmotto->dev_err_count[i]);
		strcat(uevent_str, temp);
	}

	return;
}
void motto_send_uevent(void)
{
	char temp[ABC_BUFFER_MAX];
	char *uevent_str[3] = {0,};

	get_motto_uevent_str(temp);

	uevent_str[0] = temp;
	//uevent_str[1] = &timestamp[0];
	uevent_str[2] = NULL;

	kobject_uevent_env(&sec_abc->kobj, KOBJ_CHANGE, uevent_str);
}
void motto_send_device_info(char *module_str, char *event_type)
{
	enum motto_event_module module = motto_event_to_idx(module_str, event_type);

	if (module > MOTTO_MODULE_NONE) {
		ABC_PRINT("%s\n", event_type);
		motto_update_event(module);

		motto_send_uevent();
	}
}
static ssize_t show_abc_motto_info(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct abc_motto_data *cmotto;
	struct abc_info *pinfo = dev_get_drvdata(dev);
	char temp[ABC_BUFFER_MAX];
	char retstr[ABC_BUFFER_MAX], tempcat[24];
	int i;

	get_motto_uevent_str(temp);

	cmotto = pinfo->pdata->motto_data;
	ABC_PRINT("%s\n", temp);
	sprintf(retstr,"%d boot %d", sec_abc_get_enabled(), cmotto->boot_time);
	for (i=0;i<MOTTO_MODULE_MAX;i++)
	{
		snprintf(tempcat,24,", dev%d %d", i, cmotto->dev_err_count[i]);
		strcat(retstr, tempcat);
	}
	ABC_PRINT("%s\n", retstr);

	return sprintf(buf, "%s\n", retstr);
}
static DEVICE_ATTR(motto_info, 0444, show_abc_motto_info, NULL);
#if defined(CONFIG_ARCH_QCOM)
static ssize_t show_abc_motto_motto_qet_loaded(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%d\n", qet_loaded);
}
static DEVICE_ATTR(motto_qc_qet_loaded, 0444, show_abc_motto_motto_qet_loaded, NULL);
#endif
void motto_init(struct platform_device *pdev)
{
	struct abc_info *pinfo;
	int ret = 0;

	pinfo = dev_get_drvdata(sec_abc);

	ret = device_create_file(pinfo->dev, &dev_attr_motto_info);
	if (ret) {
		pr_err("%s: Failed to create device motto_info file\n", __func__);
		goto err_create_abc_motto_info_sysfs;
	}
#if defined(CONFIG_ARCH_QCOM)
	ret = device_create_file(pinfo->dev, &dev_attr_motto_qc_qet_loaded);
	if (ret) {
		pr_err("%s: Failed to create device motto_qc_qet_loaded file\n", __func__);
		goto err_create_abc_motto_info_sysfs;
	}
#endif
	pinfo->pdata->motto_data = devm_kzalloc(&pdev->dev,
					 sizeof(struct abc_motto_data), GFP_KERNEL);
	if (pinfo->pdata->motto_data == NULL)
		goto err_alloc_motto_data;

	return;

err_alloc_motto_data:
	device_remove_file(pinfo->dev, &dev_attr_motto_info);
err_create_abc_motto_info_sysfs:
	return;
}

void motto_send_bootcheck_info(int boot_time)
{
	struct abc_info *pinfo;
	struct abc_motto_data *cmotto;

	ABC_PRINT("%d\n", boot_time);
	pinfo = dev_get_drvdata(sec_abc);
	cmotto = pinfo->pdata->motto_data;
	cmotto->boot_time = (boot_time >= 0xFF) ? 0xFF : (u32)boot_time;

	motto_send_uevent();
}
EXPORT_SYMBOL(motto_send_bootcheck_info);

MODULE_DESCRIPTION("Samsung ABC Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
