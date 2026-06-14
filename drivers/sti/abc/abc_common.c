/* abc_common.c
 *
 * Abnormal Behavior Catcher Common Driver
 *
 * Copyright (C) 2017 Samsung Electronics
 *
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
#include <linux/sti/abc_spec_manager.h>
#if IS_ENABLED(CONFIG_SEC_KUNIT)
#include <linux/sti/abc_kunit.h>
#endif
#define DEBUG_ABC
#define WARNING_REPORT
struct abc_info *pinfo;
EXPORT_SYMBOL_KUNIT(pinfo);
struct list_head abc_pre_event_list;
EXPORT_SYMBOL_KUNIT(abc_pre_event_list);
int abc_pre_event_cnt;
EXPORT_SYMBOL_KUNIT(abc_pre_event_cnt);
bool abc_save_pre_event;
EXPORT_SYMBOL_KUNIT(abc_save_pre_event);

struct device *sec_abc;
EXPORT_SYMBOL_KUNIT(sec_abc);
int abc_enable_mode;
EXPORT_SYMBOL_KUNIT(abc_enable_mode);
int abc_init;
EXPORT_SYMBOL_KUNIT(abc_init);
int REGISTERED_ABC_EVENT_TOTAL;
EXPORT_SYMBOL_KUNIT(REGISTERED_ABC_EVENT_TOTAL);

#if IS_ENABLED(CONFIG_SEC_KUNIT)
char abc_common_kunit_test_work_str[ABC_TEST_UEVENT_MAX][ABC_TEST_STR_MAX] = {"", };
EXPORT_SYMBOL_KUNIT(abc_common_kunit_test_work_str);

char sec_abc_kunit_test_log_str[ABC_TEST_STR_MAX];
EXPORT_SYMBOL_KUNIT(sec_abc_kunit_test_log_str);

char abc_hub_kunit_test_uevent_str[ABC_HUB_TEST_STR_MAX];
EXPORT_SYMBOL_KUNIT(abc_hub_kunit_test_uevent_str);
#endif

/* "module_name", "error_name", "host", on, singular_spec, error_count */
struct registered_abc_event_struct abc_event_list[] = {
	{"audio", "spk_amp", "it", true, true, 0, ABC_GROUP_NONE},
	{"audio", "spk_amp_short", "it", true, true, 0, ABC_GROUP_NONE},
	{"battery", "b2sovrc_rst", "it", true, true, 0, ABC_GROUP_NONE},
	{"battery", "dc_batt_ov", "it", true, true, 0, ABC_GROUP_NONE},
	{"battery", "dc_i2c_fail", "it", true, true, 0, ABC_GROUP_NONE},
	{"battery", "over_voltage", "it", true, true, 0, ABC_GROUP_NONE},
	{"battery", "pd_input_ocp", "it", true, true, 0, ABC_GROUP_NONE},
	{"battery", "safety_timer", "it", true, true, 0, ABC_GROUP_NONE},
	{"battery", "lim_stuck", "it", true, true, 0, ABC_GROUP_NONE},
	{"battery", "lim_i2c_fail", "it", true, true, 0, ABC_GROUP_NONE},
	{"battery", "vsys_ovp", "it", true, true, 0, ABC_GROUP_NONE},
	{"battery", "dc_current", "it", true, true, 0, ABC_GROUP_NONE},
	{"battery", "store_fg_asoc0", "it", true, true, 0, ABC_GROUP_NONE},
	{"battery", "show_fg_asoc0", "it", true, true, 0, ABC_GROUP_NONE},
	{"battery", "charger_irq_error", "", true, true, 0, ABC_GROUP_NONE},
	{"bootc", "boot_time_fail", "", true, true, 0, ABC_GROUP_NONE},
	{"camera", "camera_error", "", true, true, 0, ABC_GROUP_NONE},
	{"camera", "i2c_fail", "", true, false, 0, ABC_GROUP_NONE},
	{"camera", "icp_error", "", true, true, 0, ABC_GROUP_NONE},
	{"camera", "ipp_overflow", "", true, true, 0, ABC_GROUP_NONE},
	{"camera", "mipi_overflow", "", true, false, 0, ABC_GROUP_NONE},
#if IS_ENABLED(CONFIG_SEC_FACTORY)
	{"camera", "mipi_error_rw1", "", true, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_rs1", "", true, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_rt1", "", true, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_rt2", "", true, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_fw1", "", true, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_uw1", "", true, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_rm1", "", true, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_rb1", "", true, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_fs1", "", true, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
#else
	{"camera", "mipi_error_rw1", "", false, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_rs1", "", false, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_rt1", "", false, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_rt2", "", false, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_fw1", "", false, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_uw1", "", false, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_rm1", "", false, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_rb1", "", false, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
	{"camera", "mipi_error_fs1", "", false, false, 0, ABC_GROUP_CAMERA_MIPI_ERROR_ALL},
#endif
	{"cond", "CAM_CONNECT", "", true, true, 0, ABC_GROUP_NONE},
	{"cond", "LOWER_C2C_DETECT", "", true, true, 0, ABC_GROUP_NONE},
	{"cond", "MAIN_BAT_DETECT", "", true, true, 0, ABC_GROUP_NONE},
	{"cond", "MAIN_DIGITIZER_DETECT", "", true, true, 0, ABC_GROUP_NONE},
	{"cond", "SUB_BAT_DETECT", "", true, true, 0, ABC_GROUP_NONE},
	{"cond", "SUB_CONNECT", "", true, true, 0, ABC_GROUP_NONE},
	{"cond", "SUB_LOWER_DETECT", "", true, true, 0, ABC_GROUP_NONE},
	{"cond", "SUB_UB_DETECT", "", true, true, 0, ABC_GROUP_NONE},
	{"cond", "TOF_CONNECT", "", true, true, 0, ABC_GROUP_NONE},
	{"cond", "UPPER_C2C_DETECT", "", true, true, 0, ABC_GROUP_NONE},
	{"decon", "fence_timeout", "", true, true, 0, ABC_GROUP_NONE},
	{"display", "act_section_dsierr0", "", true, true, 0, ABC_GROUP_NONE},
	{"display", "act_section_dsierr1", "", true, true, 0, ABC_GROUP_NONE},
	{"display", "panel_main_no_te", "", true, true, 0, ABC_GROUP_NONE},
	{"display", "panel_sub_no_te", "", true, true, 0, ABC_GROUP_NONE},
	{"gpu", "gpu_fault", "", true, false, 0, ABC_GROUP_NONE},
	{"gpu", "gpu_job_timeout", "", true, false, 0, ABC_GROUP_NONE},
	{"gpu_qc", "gpu_fault", "", true, false, 0, ABC_GROUP_NONE},
	{"gpu_qc", "gpu_page_fault", "", true, false, 0, ABC_GROUP_NONE},
	{"mm", "venus_data_corrupt", "", true, true, 0, ABC_GROUP_NONE},
	{"mm", "venus_fw_load_fail", "", true, true, 0, ABC_GROUP_NONE},
	{"mm", "venus_hung", "", true, false, 0, ABC_GROUP_NONE},
	{"mm", "vidc_sys_err_type2", "", true, true, 0, ABC_GROUP_NONE},
	{"mm", "vidc_sys_err_type3", "", true, true, 0, ABC_GROUP_NONE},
	{"muic", "afc_hv_fail", "", true, true, 0, ABC_GROUP_NONE},
	{"muic", "cable_short", "", true, true, 0, ABC_GROUP_NONE},
	{"muic", "qc_hv_fail", "", true, true, 0, ABC_GROUP_NONE},
	{"npu", "npu_fw_warning", "", true, true, 0, ABC_GROUP_NONE},
	{"pdic", "i2c_fail", "it", true, true, 0, ABC_GROUP_NONE},
	{"pdic", "water_det", "", true, true, 0, ABC_GROUP_NONE},
	{"pdic", "ccic_stuck", "it", true, true, 0, ABC_GROUP_NONE},
	{"pmic", "s2dos05_bulk_read", "it", true, true, 0, ABC_GROUP_NONE},
	{"pmic", "s2dos05_bulk_write", "it", true, true, 0, ABC_GROUP_NONE},
	{"pmic", "s2dos05_read_reg", "it", true, true, 0, ABC_GROUP_NONE},
	{"pmic", "s2dos05_read_word", "it", true, true, 0, ABC_GROUP_NONE},
	{"pmic", "s2dos05_update_reg", "it", true, true, 0, ABC_GROUP_NONE},
	{"pmic", "s2dos05_write_reg", "it", true, true, 0, ABC_GROUP_NONE},
	{"pmic", "s2dos05_bulk_read_fail", "it", true, true, 0, ABC_GROUP_NONE},
	{"pmic", "s2dos05_bulk_write_fail", "it", true, true, 0, ABC_GROUP_NONE},
	{"pmic", "s2dos05_read_reg_fail", "it", true, true, 0, ABC_GROUP_NONE},
	{"pmic", "s2dos05_read_word_fail", "it", true, true, 0, ABC_GROUP_NONE},
	{"pmic", "s2dos05_update_reg_fail", "it", true, true, 0, ABC_GROUP_NONE},
	{"pmic", "s2dos05_write_reg_fail", "it", true, true, 0, ABC_GROUP_NONE},
	{"pmic", "s2dos05_scp", "", true, true, 0, ABC_GROUP_NONE},
	{"pmic", "s2dos05_ssd", "", true, true, 0, ABC_GROUP_NONE},
	{"storage", "mmc_hwreset_err", "", true, true, 0, ABC_GROUP_NONE},
	{"storage", "sd_removed_err", "", true, true, 0, ABC_GROUP_NONE},
	{"storage", "ufs_hwreset_err", "it", true, true, 0, ABC_GROUP_NONE},
	{"storage", "ufs_medium_err", "it", true, true, 0, ABC_GROUP_NONE},
	{"storage", "ufs_hardware_err", "it", true, true, 0, ABC_GROUP_NONE},
	{"tsp", "tsp_int_fault", "", true, false, 0, ABC_GROUP_NONE},
	{"tsp_sub", "tsp_int_fault", "", true, false, 0, ABC_GROUP_NONE},
	{"ub_main", "ub_disconnected", "it", true, true, 0, ABC_GROUP_NONE},
	{"ub_sub", "ub_disconnected", "it", true, true, 0, ABC_GROUP_NONE},
	{"vib", "fw_load_fail", "it", true, true, 0, ABC_GROUP_NONE},
	{"vib", "int_gnd_short", "", true, false, 0, ABC_GROUP_NONE},
	{"vib", "standby_error_recovered", "it", true, true, 0, ABC_GROUP_NONE},
	{"wacom", "digitizer_not_connected", "", true, true, 0, ABC_GROUP_NONE},
#if IS_ENABLED(CONFIG_SEC_KUNIT)
	{"kunit", "test_warn", "", true, true, 0, ABC_GROUP_NONE},
	{"kunit", "test_info", "", true, true, 0, ABC_GROUP_NONE},
	{"kunit", "test_error", "", true, true, 0, ABC_GROUP_NONE},
#endif
};
EXPORT_SYMBOL_KUNIT(abc_event_list);

struct abc_enable_cmd_struct enable_cmd_list[] = {
	{ERROR_REPORT_MODE_ENABLE, ERROR_REPORT_MODE_BIT, "ERROR_REPORT=1"},
	{ERROR_REPORT_MODE_DISABLE, ERROR_REPORT_MODE_BIT, "ERROR_REPORT=0"},
	{ALL_REPORT_MODE_ENABLE, ALL_REPORT_MODE_BIT, "ALL_REPORT=1"},
	{ALL_REPORT_MODE_DISABLE, ALL_REPORT_MODE_BIT, "ALL_REPORT=0"},
	{PRE_EVENT_ENABLE, PRE_EVENT_ENABLE_BIT, "PRE_EVENT=1"},
	{PRE_EVENT_DISABLE, PRE_EVENT_ENABLE_BIT, "PRE_EVENT=0"},
};

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id sec_abc_dt_match[] = {
	{ .compatible = "samsung,sec_abc" },
	{ }
};
#endif

#if IS_ENABLED(CONFIG_SEC_KUNIT)
void abc_common_test_get_work_str(char *utest_event_str[])
{
	int i;

	for (i = 0; i < ABC_TEST_UEVENT_MAX; i++) {
		if (utest_event_str[i]) {
			if (i >= 2 && !strncmp(utest_event_str[i], TIME_KEYWORD, strlen(TIME_KEYWORD)))
				strlcpy(abc_common_kunit_test_work_str[i],
						TIME_KEYWORD, ABC_TEST_STR_MAX);
			else
				strlcpy(abc_common_kunit_test_work_str[i],
						utest_event_str[i], ABC_TEST_STR_MAX);
		}
	}
}
EXPORT_SYMBOL_KUNIT(abc_common_test_get_work_str);

void abc_common_test_get_log_str(char *log_str)
{
	strlcpy(sec_abc_kunit_test_log_str, log_str, sizeof(sec_abc_kunit_test_log_str));
}
EXPORT_SYMBOL_KUNIT(abc_common_test_get_log_str);
#endif

static int sec_abc_resume(struct device *dev)
{
	return 0;
}

static int sec_abc_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct dev_pm_ops sec_abc_pm = {
	.resume = sec_abc_resume,
};

int sec_abc_get_idx_of_registered_event(char *module_name, char *error_name)
{
	int i;

	for (i = 0; i < REGISTERED_ABC_EVENT_TOTAL; i++)
		if (!strncmp(module_name, abc_event_list[i].module_name, ABC_EVENT_STR_MAX) &&
			!strncmp(error_name, abc_event_list[i].error_name, ABC_EVENT_STR_MAX))
			return i;

	return -1;
}
EXPORT_SYMBOL_KUNIT(sec_abc_get_idx_of_registered_event);

#if !IS_ENABLED(CONFIG_SEC_FACTORY) && !IS_ENABLED(CONFIG_SEC_KUNIT)
static int sec_abc_get_error_count(char *module_name, char *error_name)
{
	int i = sec_abc_get_idx_of_registered_event(module_name, error_name);

	if (i >= 0)
		return abc_event_list[i].error_count;
	return 0;
}

static void sec_abc_update_error_count(char *module_name, char *error_name)
{
	int i = sec_abc_get_idx_of_registered_event(module_name, error_name);

	if (i >= 0)
		abc_event_list[i].error_count++;
}

static void sec_abc_reset_error_count(void)
{
	int i;

	for (i = 0; i < REGISTERED_ABC_EVENT_TOTAL; i++)
		abc_event_list[i].error_count = 0;
}

static bool sec_abc_is_skip_event(char *abc_str)
{
	struct abc_key_data key_data;
	int count;

	if (sec_abc_make_key_data(&key_data, abc_str)) {
		ABC_PRINT("Event string isn't valid. Check Input : %s", abc_str);
		return true;
	}

	count = sec_abc_get_error_count(key_data.event_module, key_data.event_name);

	if (count >= ABC_SKIP_EVENT_COUNT_THRESHOLD) {
		if (count == ABC_SKIP_EVENT_COUNT_THRESHOLD) {
			ABC_PRINT("[%s-%s] ABC Error already detected %d times! It is skipped from now on!",
			  key_data.event_module, key_data.event_name, ABC_SKIP_EVENT_COUNT_THRESHOLD);
			sec_abc_update_error_count(key_data.event_module, key_data.event_name);
		}
		return true;
	}
	return false;
}
#endif

void sec_abc_send_uevent(struct abc_key_data *key_data, char *uevent_type)
{
	char *uevent_str[ABC_UEVENT_MAX] = {0,};
	char uevent_module_str[ABC_EVENT_STR_MAX + 7];
	char uevent_event_str[ABC_EVENT_STR_MAX + ABC_TYPE_STR_MAX];
	char uevent_host_str[ABC_EVENT_STR_MAX];
	char uevent_ext_log_str[ABC_EVENT_STR_MAX];
	char timestamp[TIME_STAMP_STR_MAX];
	int idx;

	if (!sec_abc_get_enabled()) {
		ABC_PRINT("ABC isn't enabled. Save pre_event");
		sec_abc_save_pre_events(key_data, uevent_type);
		return;
	}

	snprintf(uevent_module_str, ABC_EVENT_STR_MAX, "MODULE=%s", key_data->event_module);
	snprintf(uevent_event_str, ABC_EVENT_STR_MAX, "%s=%s", uevent_type, key_data->event_name);
	snprintf(timestamp, TIME_STAMP_STR_MAX, "TIMESTAMP=%d", key_data->cur_time);

	uevent_str[0] = uevent_module_str;
	uevent_str[1] = uevent_event_str;
	uevent_str[2] = timestamp;
	if (abc_event_list[key_data->idx].host[0]) {
		snprintf(uevent_host_str, ABC_EVENT_STR_MAX, "HOST=%s", abc_event_list[key_data->idx].host);
		uevent_str[3] = uevent_host_str;
	}

	if (key_data->ext_log[0]) {
		snprintf(uevent_ext_log_str, ABC_EVENT_STR_MAX, "EXT_LOG=%s", key_data->ext_log);
		uevent_str[4] = uevent_ext_log_str;
	}

	for (idx = 0; uevent_str[idx]; idx++)
		ABC_PRINT("%s", uevent_str[idx]);
#if IS_ENABLED(CONFIG_SEC_KUNIT)
	abc_common_test_get_work_str(uevent_str);
	complete(&pinfo->test_uevent_done);
#endif

#if !IS_ENABLED(CONFIG_SEC_FACTORY) && !IS_ENABLED(CONFIG_SEC_KUNIT)
	sec_abc_update_error_count(key_data->event_module, key_data->event_name);
#endif
	kobject_uevent_env(&sec_abc->kobj, KOBJ_CHANGE, uevent_str);
}
EXPORT_SYMBOL_KUNIT(sec_abc_send_uevent);

__visible_for_testing
struct abc_pre_event *sec_abc_get_pre_event_node(struct abc_key_data *key_data)
{
	struct abc_pre_event *pre_event;

	list_for_each_entry(pre_event, &abc_pre_event_list, node) {
		if (!strcmp(pre_event->key_data.event_module, key_data->event_module) &&
			!strcmp(pre_event->key_data.event_name, key_data->event_name)) {
			ABC_PRINT("return matched node");
			return pre_event;
		}
	}

	pre_event = NULL;

	if (abc_pre_event_cnt < ABC_PREOCCURRED_EVENT_MAX) {

		pre_event = kzalloc(sizeof(*pre_event), GFP_KERNEL);

		if (pre_event) {
			list_add_tail(&pre_event->node, &abc_pre_event_list);
			abc_pre_event_cnt++;
			ABC_PRINT("return new node");
		} else
			ABC_PRINT("failed to get node");
	}

	return pre_event;
}
EXPORT_SYMBOL_KUNIT(sec_abc_get_pre_event_node);

__visible_for_testing
int sec_abc_clear_pre_events(void)
{
	struct abc_pre_event *pre_event;
	int cnt = 0;

	ABC_PRINT("start");
	mutex_lock(&pinfo->pre_event_mutex);

	while (!list_empty(&abc_pre_event_list)) {
		pre_event = list_first_entry(&abc_pre_event_list,
										struct abc_pre_event,
										node);
		list_del(&pre_event->node);
		kfree(pre_event);
		cnt++;
	}

	abc_pre_event_cnt = 0;

	/* Once Pre_events were cleared, don't save pre_event anymore. */
	abc_save_pre_event = false;
	abc_enable_mode &= ~(PRE_EVENT_ENABLE_BIT);

#if IS_ENABLED(CONFIG_SEC_KUNIT)
	complete(&pinfo->test_work_done);
#endif
	mutex_unlock(&pinfo->pre_event_mutex);
	ABC_PRINT("end");
	return cnt;
}
EXPORT_SYMBOL_KUNIT(sec_abc_clear_pre_events);

__visible_for_testing
void sec_abc_adjust_pre_event_count(struct abc_pre_event *pre_event)
{
	int pre_event_max_count = ABC_SKIP_EVENT_COUNT_THRESHOLD;

	pre_event->error_cnt = min(pre_event->error_cnt, pre_event_max_count);
	pre_event_max_count -= pre_event->error_cnt;
	pre_event->all_cnt = min(pre_event->all_cnt, pre_event_max_count);
}
EXPORT_SYMBOL_KUNIT(sec_abc_adjust_pre_event_count);

__visible_for_testing
int sec_abc_process_pre_events(void)
{
	struct abc_pre_event *pre_event;
	int i, cnt = 0;

	ABC_PRINT("start");

	mutex_lock(&pinfo->pre_event_mutex);
	list_for_each_entry(pre_event, &abc_pre_event_list, node) {
		sec_abc_adjust_pre_event_count(pre_event);
		if (abc_enable_mode & ALL_REPORT_MODE_BIT) {
			ABC_PRINT("All report mode. Send uevent");
			cnt += pre_event->all_cnt;

			for (i = 0; i < pre_event->all_cnt; i++)
				sec_abc_send_uevent(&pre_event->key_data, pre_event->key_data.event_type);

		}

		cnt += pre_event->error_cnt;
		for (i = 0; i < pre_event->error_cnt; i++)
			if (abc_event_list[pre_event->key_data.idx].enabled)
				sec_abc_send_uevent(&pre_event->key_data, "ERROR");
	}
	mutex_unlock(&pinfo->pre_event_mutex);
	ABC_PRINT("pre_event cnt : %d end", cnt);

	return cnt;
}
EXPORT_SYMBOL_KUNIT(sec_abc_process_pre_events);

int sec_abc_save_pre_events(struct abc_key_data *key_data, char *uevent_type)
{
	struct abc_pre_event *pre_event;
	int ret = 0;

	mutex_lock(&pinfo->pre_event_mutex);

	ABC_PRINT("start Module(%s) Event(%s) Type(%s)",
			  key_data->event_module,
			  key_data->event_name,
			  uevent_type);

	pre_event = sec_abc_get_pre_event_node(key_data);

	if (!pre_event) {
		ABC_PRINT_KUNIT("Failed to add Pre_event");
		ret = -EINVAL;
		goto out;
	}

	pre_event->key_data = *key_data;

	if (!strncmp(uevent_type, "ERROR", 5))
		pre_event->error_cnt++;
	else
		pre_event->all_cnt++;

out:
	mutex_unlock(&pinfo->pre_event_mutex);
	ABC_PRINT("end");
	return ret;
}
EXPORT_SYMBOL_KUNIT(sec_abc_save_pre_events);

__visible_for_testing
void sec_abc_process_changed_enable_mode(void)
{
	if (sec_abc_get_enabled()) {
		if (abc_enable_mode & PRE_EVENT_ENABLE_BIT) {
			sec_abc_process_pre_events();
			ABC_PRINT_KUNIT("Pre_events processed");
		} else {
			ABC_PRINT_KUNIT("ABC enabled. Pre_event disabled");
		}
		complete(&pinfo->enable_done);
	} else {
		ABC_PRINT_KUNIT("ABC is disabled. Clear events");
		sec_abc_reset_all_buffer();
#if !IS_ENABLED(CONFIG_SEC_FACTORY) && !IS_ENABLED(CONFIG_SEC_KUNIT)
		sec_abc_reset_error_count();
#endif
	}

	ABC_PRINT("%d Pre_events cleared", sec_abc_clear_pre_events());
}
EXPORT_SYMBOL_KUNIT(sec_abc_process_changed_enable_mode);

/* Change ABC driver's enable mode.
 *
 *   Interface with ACT : write "1" or "0"
 *   Interfcae with LABO
 *   ex) "ERROR_REPORT=1,PRE_EVENT=1", "ALL_REPORT=1,ERROR_REPORT=0,PRE_EVENT=0" ...
 *
 */
__visible_for_testing
ssize_t enabled_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf,
					  size_t count)
{
	char *c, *p, *enable_cmd[ABC_CMD_MAX];
	char temp[ABC_CMD_STR_MAX * 3];
	int items, i, j, idx = 0;
	bool chk = false;
	int origin_enable_mode = abc_enable_mode;

	if (!strncmp(buf, "1", 1)) {
		/* Interface with ACT */
		ABC_PRINT("Error report mode enabled");
		abc_enable_mode |= ERROR_REPORT_MODE_BIT;
	} else if (!strncmp(buf, "0", 1)) {
		ABC_PRINT("Error report mode disabled");
		abc_enable_mode &= (~ERROR_REPORT_MODE_BIT);
	} else {

		strlcpy(temp, buf, ABC_CMD_STR_MAX * 3);
		p = temp;
		items = ARRAY_SIZE(enable_cmd_list);

		while ((c = strsep(&p, ",")) != NULL && idx < ABC_CMD_MAX) {
			enable_cmd[idx] = c;
			idx++;
		}

		for (i = 0; i < idx; i++) {
			chk = false;
			for (j = 0; j < items; j++) {
				if (!strncmp(enable_cmd[i],
					enable_cmd_list[j].abc_cmd_str,
					strlen(enable_cmd_list[j].abc_cmd_str))) {
					if (strstr(enable_cmd_list[j].abc_cmd_str, "=1"))
						abc_enable_mode |= enable_cmd_list[j].enable_value;
					else
						abc_enable_mode &= ~(enable_cmd_list[j].enable_value);
					chk = true;
				}
			}
			if (!chk) {
				ABC_PRINT_KUNIT("Invalid string. Check the Input");
				abc_enable_mode = origin_enable_mode;
				return count;
			}
		}
	}

	sec_abc_process_changed_enable_mode();
	return count;
}
EXPORT_SYMBOL_KUNIT(enabled_store);

__visible_for_testing
ssize_t enabled_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	if (sec_abc_get_enabled())
		return sprintf(buf, "1\n");
	else
		return sprintf(buf, "0\n");
}
EXPORT_SYMBOL_KUNIT(enabled_show);

static DEVICE_ATTR_RW(enabled);

__visible_for_testing
ssize_t spec_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf,
				   size_t count)
{
	mutex_lock(&pinfo->spec_mutex);

	sec_abc_change_spec(buf);

	mutex_unlock(&pinfo->spec_mutex);

	return count;
}
EXPORT_SYMBOL_KUNIT(spec_store);

__visible_for_testing
ssize_t spec_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	int count = 0;

	mutex_lock(&pinfo->spec_mutex);

	count = sec_abc_read_spec(buf);

	mutex_unlock(&pinfo->spec_mutex);

	return count;
}
EXPORT_SYMBOL_KUNIT(spec_show);

static DEVICE_ATTR_RW(spec);

__visible_for_testing
ssize_t features_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf,
				   size_t count)
{
	return count;
}

__visible_for_testing
ssize_t features_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	int count = 0;

	count += scnprintf(buf, PAGE_SIZE, "spec_control\nhost\n");

	return count;
}
EXPORT_SYMBOL_KUNIT(features_show);

static DEVICE_ATTR_RW(features);

__visible_for_testing
ssize_t list_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf,
				   size_t count)
{
	return count;
}
EXPORT_SYMBOL_KUNIT(list_store);

__visible_for_testing
ssize_t list_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	int count = 0;
	int i;

	for (i = 0; i < REGISTERED_ABC_EVENT_TOTAL; i++)
	{
		count += scnprintf(buf + count, PAGE_SIZE - count, "%s,%s\n",
			abc_event_list[i].module_name, abc_event_list[i].error_name);
	}
	ABC_PRINT("%d", count);

	return count;
}
EXPORT_SYMBOL_KUNIT(list_show);

static DEVICE_ATTR_RW(list);

static struct attribute *sec_abc_attr[] = {
	&dev_attr_enabled.attr,
	&dev_attr_spec.attr,
	&dev_attr_features.attr,
	&dev_attr_list.attr,
	NULL,
};

static struct attribute_group sec_abc_attr_group = {
	.attrs = sec_abc_attr,
};

int sec_abc_get_enabled(void)
{
	return (abc_enable_mode & (ERROR_REPORT_MODE_BIT | ALL_REPORT_MODE_BIT));
}
EXPORT_SYMBOL(sec_abc_get_enabled);

static void sec_abc_work_func_clear_pre_events(struct work_struct *work)
{
	ABC_DEBUG("start");

	sec_abc_clear_pre_events();

	ABC_DEBUG("end");
}

#if IS_ENABLED(CONFIG_UML)
static void sec_abc_init_key_data(struct abc_key_data *key_data)
{
	memset(key_data->event_module, 0, sizeof(key_data->event_module));
	memset(key_data->event_name, 0, sizeof(key_data->event_name));
	memset(key_data->event_type, 0, sizeof(key_data->event_type));
	memset(key_data->ext_log, 0, sizeof(key_data->ext_log));
}
#endif

static void sec_abc_work_func(struct work_struct *work)
{
	struct abc_event_work *event_work_data;
	struct abc_key_data key_data;
	int idx;

	mutex_lock(&pinfo->work_mutex);

	event_work_data = container_of(work, struct abc_event_work, work);

	ABC_DEBUG("work start. str : %s", event_work_data->abc_str);

#if IS_ENABLED(CONFIG_UML)
	sec_abc_init_key_data(&key_data);
#endif

	if (sec_abc_make_key_data(&key_data, event_work_data->abc_str)) {
		ABC_PRINT("Event string isn't valid. Check Input : %s", event_work_data->abc_str);
		goto abc_work_end;
	}

	idx = sec_abc_get_idx_of_registered_event(key_data.event_module, key_data.event_name);

	if (idx < 0) {
		ABC_PRINT_KUNIT("%s : %s isn't registered", key_data.event_module, key_data.event_name);
		goto abc_work_end;
	}

	key_data.idx = idx;

#if IS_ENABLED(CONFIG_SEC_ABC_MOTTO)
#if !IS_ENABLED(CONFIG_UML)
	motto_send_device_info(key_data.event_module, key_data.event_name);
#endif
#endif

	if (abc_enable_mode & (ALL_REPORT_MODE_BIT | PRE_EVENT_ENABLE_BIT)) {
		ABC_PRINT("All report mode may be enabled. Send uevent");
		sec_abc_send_uevent(&key_data, key_data.event_type);
	}

	if (!abc_event_list[idx].enabled || !strncmp(key_data.event_type, "INFO", 4)) {
		ABC_PRINT_KUNIT("Don't send error report");
		goto abc_work_end;
	}

	if (abc_event_list[idx].singular_spec) {
		ABC_PRINT("Send uevent : %s", event_work_data->abc_str);
		sec_abc_send_uevent(&key_data, "ERROR");
	} else {

		sec_abc_enqueue_event_data(&key_data);

		if (sec_abc_reached_spec(&key_data)) {
			ABC_PRINT("Send uevent : %s", event_work_data->abc_str);
			sec_abc_send_uevent(&key_data, "ERROR");
			sec_abc_reset_event_buffer(&key_data);
		}
	}

abc_work_end:
	ABC_DEBUG("work done");
	mutex_unlock(&pinfo->work_mutex);
#if IS_ENABLED(CONFIG_SEC_KUNIT)
	complete(&pinfo->test_work_done);
#endif
}

/* event string format
 *
 * ex)
 *	 MODULE=tsp@WARN=power_status_mismatch
 *   MODULE=gpu@INFO=gpu_fault
 *	 MODULE=tsp@ERROR=power_status_mismatch@EXT_LOG=fw_ver(0108)
 *
 */
__visible_for_testing
void sec_abc_enqueue_work(struct abc_event_work work_data[], char *str)
{
	int idx;

	for (idx = 0; idx < ABC_WORK_MAX; idx++) {
		if (!work_pending(&work_data[idx].work)) {
			ABC_DEBUG("Event %s use work[%d]", str, idx);
			strlcpy(work_data[idx].abc_str, str, ABC_BUFFER_MAX);
			queue_work(pinfo->workqueue, &work_data[idx].work);
			return;
		}
	}

	ABC_PRINT("Failed. All works are in queue");
}

void sec_abc_send_event(char *str)
{
#if !IS_ENABLED(CONFIG_SEC_FACTORY) && !IS_ENABLED(CONFIG_SEC_KUNIT)
	if (sec_abc_is_skip_event(str))
		return;
#endif

	if (!abc_init) {
		ABC_PRINT_KUNIT("ABC driver is not initialized!(%s)", str);
		return;
	}

	if (!sec_abc_get_enabled() && !abc_save_pre_event) {
		ABC_PRINT_KUNIT("ABC is disabled and pre_event is disabled.(%s)", str);
		return;
	}

	ABC_PRINT("ABC is working. Queue work.(%s)", str);
	sec_abc_enqueue_work(pinfo->event_work_data, str);

}
EXPORT_SYMBOL(sec_abc_send_event);

/**
 * sec_abc_wait_enable() - wait for abc enable done
 * Return : 0 for success, -1 for fail(timeout or abc not initialized)
 */
int sec_abc_wait_enabled(void)
{
	unsigned long timeout;

	if (!abc_init) {
		ABC_PRINT_KUNIT("ABC driver is not initialized!");
		return -1;
	}

	if (sec_abc_get_enabled())
		return 0;

	reinit_completion(&pinfo->enable_done);

	timeout = wait_for_completion_timeout(&pinfo->enable_done,
						  msecs_to_jiffies(ABC_WAIT_ENABLE_TIMEOUT));

	if (timeout == 0) {
		ABC_PRINT_KUNIT("timeout!");
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(sec_abc_wait_enabled);

__visible_for_testing
void sec_abc_init_work(struct abc_info *pinfo)
{
	int idx;

	pinfo->workqueue = create_singlethread_workqueue("sec_abc_wq");
	INIT_DELAYED_WORK(&pinfo->clear_pre_events, sec_abc_work_func_clear_pre_events);

	/* After timeout clear abc events occurred when abc disalbed. */
	queue_delayed_work(pinfo->workqueue,
					   &pinfo->clear_pre_events,
					   msecs_to_jiffies(ABC_CLEAR_EVENT_TIMEOUT));

	/* Work for abc_events & pre_events (events occurred before enabled) */
	for (idx = 0; idx < ABC_WORK_MAX; idx++)
		INIT_WORK(&pinfo->event_work_data[idx].work, sec_abc_work_func);
}
EXPORT_SYMBOL_KUNIT(sec_abc_init_work);

__visible_for_testing
int sec_abc_get_registered_abc_event_total(void)
{
	return ARRAY_SIZE(abc_event_list);
}
EXPORT_SYMBOL_KUNIT(sec_abc_get_registered_abc_event_total);

static int sec_abc_probe(struct platform_device *pdev)
{
	struct abc_platform_data *pdata;
	int ret = 0;

	ABC_PRINT("start");

	abc_init = false;
	REGISTERED_ABC_EVENT_TOTAL = sec_abc_get_registered_abc_event_total();

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
					 sizeof(struct abc_platform_data), GFP_KERNEL);

		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate platform data");
			ret = -ENOMEM;
			goto out;
		}

		pdev->dev.platform_data = pdata;
		ret = abc_parse_dt(&pdev->dev);
		if (ret) {
			dev_err(&pdev->dev, "Failed to parse dt data");
			goto err_parse_dt;
		}

		ABC_PRINT("parse dt done");
	} else {
		pdata = pdev->dev.platform_data;
	}

	if (!pdata) {
		dev_err(&pdev->dev, "There are no platform data");
		ret = -EINVAL;
		goto out;
	}

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);

	if (!pinfo) {
		ret = -ENOMEM;
		goto err_alloc_pinfo;
	}

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	pinfo->dev = sec_device_create(pinfo, "sec_abc");
#else
	pinfo->dev = device_create(sec_class, NULL, 0, NULL, "sec_abc");
#endif
	if (IS_ERR(pinfo->dev)) {
		pr_err("%s Failed to create device(sec_abc)!", __func__);
		ret = -ENODEV;
		goto err_create_device;
	}

	sec_abc = pinfo->dev;

	ret = sysfs_create_group(&pinfo->dev->kobj, &sec_abc_attr_group);
	if (ret) {
		pr_err("%s: Failed to create device attribute group", __func__);
		goto err_create_abc_attr_group;
	}

	INIT_LIST_HEAD(&abc_pre_event_list);

	sec_abc_init_work(pinfo);

	if (!pinfo->workqueue)
		goto err_create_abc_wq;

#if IS_ENABLED(CONFIG_SEC_KUNIT)
	init_completion(&pinfo->test_uevent_done);
	init_completion(&pinfo->test_work_done);
#endif
	init_completion(&pinfo->enable_done);
	mutex_init(&pinfo->pre_event_mutex);
	mutex_init(&pinfo->enable_mutex);
	mutex_init(&pinfo->work_mutex);
	mutex_init(&pinfo->spec_mutex);
	pinfo->pdata = pdata;
	platform_set_drvdata(pdev, pinfo);
#if IS_ENABLED(CONFIG_SEC_ABC_MOTTO)
	motto_init(pdev);
#endif
	abc_init = true;
	abc_enable_mode |= PRE_EVENT_ENABLE_BIT;
	abc_save_pre_event = true;
	ABC_PRINT("success");
	return ret;
err_create_abc_wq:
	sysfs_remove_group(&pinfo->dev->kobj, &sec_abc_attr_group);
err_create_abc_attr_group:
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	sec_device_destroy(sec_abc->devt);
#else
	device_destroy(sec_class, sec_abc->devt);
#endif
err_create_device:
	kfree(pinfo);
err_alloc_pinfo:
err_parse_dt:
	pdev->dev.platform_data = NULL;
out:
	return ret;
}

void sec_abc_free_pre_events(void)
{
	struct abc_pre_event *pre_event;

	while (!list_empty(&abc_pre_event_list)) {

		pre_event = list_first_entry(
							 &abc_pre_event_list,
							 struct abc_pre_event,
							 node);

		list_del(&pre_event->node);
		kfree(pre_event);
	}
}
EXPORT_SYMBOL_KUNIT(sec_abc_free_pre_events);

__visible_for_testing
void sec_abc_free_allocated_memory(void)
{
	sec_abc_free_pre_events();
	sec_abc_free_spec_buffer();
}
EXPORT_SYMBOL_KUNIT(sec_abc_free_allocated_memory);

static struct platform_driver sec_abc_driver = {
	.probe = sec_abc_probe,
	.remove = sec_abc_remove,
	.driver = {
		.name = "sec_abc",
		.owner = THIS_MODULE,
#if IS_ENABLED(CONFIG_PM)
		.pm	= &sec_abc_pm,
#endif
#if IS_ENABLED(CONFIG_OF)
		.of_match_table = of_match_ptr(sec_abc_dt_match),
#endif
	},
};

static int __init sec_abc_init(void)
{
	ABC_PRINT("start");

	return platform_driver_register(&sec_abc_driver);
}

static void __exit sec_abc_exit(void)
{
	ABC_PRINT("exit");
	sec_abc_free_allocated_memory();

	return platform_driver_unregister(&sec_abc_driver);
}

module_init(sec_abc_init);
module_exit(sec_abc_exit);

MODULE_DESCRIPTION("Samsung ABC Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
