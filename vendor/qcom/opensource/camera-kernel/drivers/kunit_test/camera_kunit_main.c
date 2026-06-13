// SPDX-License-Identifier: GPL-2.0
/*
 * TODO: Add test description.
 */
#include <linux/module.h>
#include "camera_kunit_main.h"
#include "cam_sensor_util.h"

/*
 * Here we make a list of all the test cases we want to add to the test module
 * below.
 */
struct kunit_case i2c_sensor_rear_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_sensor_rear_test),
	{},
};

struct kunit_case i2c_sensor_rear2_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_sensor_rear2_test),
	{},
};

struct kunit_case i2c_sensor_rear3_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_sensor_rear3_test),
	{},
};

struct kunit_case i2c_sensor_rear4_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_sensor_rear4_test),
	{},
};

struct kunit_case i2c_sensor_front_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_sensor_front_test),
	{},
};

struct kunit_case i2c_sensor_front_top_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_sensor_front_top_test),
	{},
};

struct kunit_case i2c_af_rear_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_af_rear_test),
	{},
};

struct kunit_case i2c_af_rear2_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_af_rear2_test),
	{},
};

struct kunit_case i2c_af_rear3_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_af_rear3_test),
	{},
};

struct kunit_case i2c_af_rear4_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_af_rear4_test),
	{},
};

struct kunit_case i2c_af_front_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_af_front_test),
	{},
};

struct kunit_case i2c_af_front_top_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_af_front_top_test),
	{},
};

struct kunit_case i2c_ois_rear_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_ois_rear_test),
	{},
};

struct kunit_case i2c_ois_rear3_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_ois_rear3_test),
	{},
};

struct kunit_case i2c_ois_rear4_test_cases[] = {
	KUNIT_CASE(hw_bigdata_i2c_ois_rear4_test),
	{},
};

struct kunit_case mipi_sensor_rear_test_cases[] = {
	KUNIT_CASE(hw_bigdata_mipi_sensor_rear_test),
	{},
};

struct kunit_case mipi_sensor_rear2_test_cases[] = {
	KUNIT_CASE(hw_bigdata_mipi_sensor_rear2_test),
	{},
};

struct kunit_case mipi_sensor_rear3_test_cases[] = {
	KUNIT_CASE(hw_bigdata_mipi_sensor_rear3_test),
	{},
};

struct kunit_case mipi_sensor_rear4_test_cases[] = {
	KUNIT_CASE(hw_bigdata_mipi_sensor_rear4_test),
	{},
};

struct kunit_case mipi_sensor_front_test_cases[] = {
	KUNIT_CASE(hw_bigdata_mipi_sensor_front_test),
	{},
};

struct kunit_case mipi_sensor_front_top_test_cases[] = {
	KUNIT_CASE(hw_bigdata_mipi_sensor_front_top_test),
	{},
};

struct kunit_case hw_bigdata_fill_test_cases[] = {
	KUNIT_CASE(hw_bigdata_fill_test),
	{},
};

struct kunit_case mipi_init_param_test_cases[] = {
	KUNIT_CASE(hw_bigdata_mipi_init_param_test),
	{},
};

struct kunit_case file_test_cases[] = {
	KUNIT_CASE(hw_bigdata_file_test),
	{},
};

struct kunit_case apply_cdr_value_test_cases[] = {
	KUNIT_CASE(cam_clock_data_recovery_apply_value_test),
	{},
};

struct kunit_case apply_result_value_test_cases[] = {
	KUNIT_CASE(cam_clock_data_recovery_apply_result_test),
	{},
};

struct kunit_case cdr_write_normal_test_cases[] = {
	KUNIT_CASE(cam_clock_data_recovery_write_normal_test),
	{},
};

struct kunit_case cdr_write_overflow1_test_cases[] = {
	KUNIT_CASE(cam_clock_data_recovery_write_overflow1_test),
	{},
};

struct kunit_case cdr_write_overflow2_test_cases[] = {
	KUNIT_CASE(cam_clock_data_recovery_write_overflow2_test),
	{},
};

struct kunit_case cdr_write_invalid_test_cases[] = {
	KUNIT_CASE(cam_clock_data_recovery_write_invalid_test),
	{},
};

struct kunit_case sensor_util_tc_get_seq_sen_id[] = {
	KUNIT_CASE(cam_sensor_util_tc_get_seq_sen_id),
	{},
};

struct kunit_case sensor_util_tc_parse_reg[] = {
	KUNIT_CASE(cam_sensor_util_tc_parse_reg),
	{},
};

struct kunit_case sensor_util_tc_check_aeb_on_test_ok[] = {
	KUNIT_CASE(cam_sensor_util_tc_check_aeb_on_test_ok),
	{},
};

struct kunit_case sensor_util_tc_check_aeb_on_test_ng[] = {
	KUNIT_CASE(cam_sensor_util_tc_check_aeb_on_test_ng),
	{},
};

struct kunit_case sensor_util_tc_get_vc_pick_idx[] = {
	KUNIT_CASE(cam_sensor_util_tc_get_vc_pick_idx),
	{},
};

struct kunit_suite cam_kunit_i2c_sensor_rear = {
	.name = "cam_kunit_i2c_sensor_rear_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_sensor_rear_test_cases,
};

struct kunit_suite cam_kunit_i2c_sensor_rear2 = {
	.name = "cam_kunit_i2c_sensor_rear2_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_sensor_rear2_test_cases,
};

struct kunit_suite cam_kunit_i2c_sensor_rear3 = {
	.name = "cam_kunit_i2c_sensor_rear3_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_sensor_rear3_test_cases,
};

struct kunit_suite cam_kunit_i2c_sensor_rear4 = {
	.name = "cam_kunit_i2c_sensor_rear4_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_sensor_rear4_test_cases,
};

struct kunit_suite cam_kunit_i2c_sensor_front = {
	.name = "cam_kunit_i2c_sensor_front_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_sensor_front_test_cases,
};

struct kunit_suite cam_kunit_i2c_sensor_front_top = {
	.name = "cam_kunit_i2c_sensor_front_top_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_sensor_front_top_test_cases,
};

struct kunit_suite cam_kunit_i2c_af_rear = {
	.name = "cam_kunit_i2c_af_rear_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_af_rear_test_cases,
};

struct kunit_suite cam_kunit_i2c_af_rear2 = {
	.name = "cam_kunit_i2c_af_rear2_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_af_rear2_test_cases,
};

struct kunit_suite cam_kunit_i2c_af_rear3 = {
	.name = "cam_kunit_i2c_af_rear3_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_af_rear3_test_cases,
};

struct kunit_suite cam_kunit_i2c_af_rear4 = {
	.name = "cam_kunit_i2c_af_rear4_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_af_rear4_test_cases,
};

struct kunit_suite cam_kunit_i2c_af_front = {
	.name = "cam_kunit_i2c_af_front_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_af_front_test_cases,
};

struct kunit_suite cam_kunit_i2c_af_front_top = {
	.name = "cam_kunit_i2c_af_front_top_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_af_front_top_test_cases,
};

struct kunit_suite cam_kunit_i2c_ois_rear = {
	.name = "cam_kunit_i2c_ois_rear_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_ois_rear_test_cases,
};

struct kunit_suite cam_kunit_i2c_ois_rear3 = {
	.name = "cam_kunit_i2c_ois_rear3_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_ois_rear3_test_cases,
};

struct kunit_suite cam_kunit_i2c_ois_rear4 = {
	.name = "cam_kunit_i2c_ois_rear4_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = i2c_ois_rear4_test_cases,
};

struct kunit_suite cam_kunit_mipi_sensor_rear = {
	.name = "cam_kunit_mipi_sensor_rear_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = mipi_sensor_rear_test_cases,
};

struct kunit_suite cam_kunit_mipi_sensor_rear2 = {
	.name = "cam_kunit_mipi_sensor_rear2_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = mipi_sensor_rear2_test_cases,
};

struct kunit_suite cam_kunit_mipi_sensor_rear3 = {
	.name = "cam_kunit_mipi_sensor_rear3_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = mipi_sensor_rear3_test_cases,
};

struct kunit_suite cam_kunit_mipi_sensor_rear4 = {
	.name = "cam_kunit_mipi_sensor_rear4_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = mipi_sensor_rear4_test_cases,
};

struct kunit_suite cam_kunit_mipi_sensor_front = {
	.name = "cam_kunit_mipi_sensor_front_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = mipi_sensor_front_test_cases,
};

struct kunit_suite cam_kunit_mipi_sensor_front_top = {
	.name = "cam_kunit_mipi_sensor_front_top_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = mipi_sensor_front_top_test_cases,
};

struct kunit_suite cam_kunit_hw_bigdata_fill = {
	.name = "cam_kunit_hw_bigdata_fill_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = hw_bigdata_fill_test_cases,
};

struct kunit_suite cam_kunit_mipi_init_param = {
	.name = "cam_kunit_mipi_init_param_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = mipi_init_param_test_cases,
};

struct kunit_suite cam_kunit_file = {
	.name = "cam_kunit_file_test",
	.init = hw_bigdata_test_init,
	.exit = hw_bigdata_test_exit,
	.test_cases = file_test_cases,
};

struct kunit_suite cam_kunit_apply_cdr_value = {
	.name = "cam_kunit_apply_cdr_value_test",
	.test_cases = apply_cdr_value_test_cases,
};

struct kunit_suite cam_kunit_apply_result_value = {
	.name = "cam_kunit_apply_result_value_test",
	.test_cases = apply_result_value_test_cases,
};

struct kunit_suite cam_kunit_cdr_write_normal = {
	.name = "cam_kunit_cdr_write_normal_test",
	.test_cases = cdr_write_normal_test_cases,
};

struct kunit_suite cam_kunit_cdr_write_overflow1 = {
	.name = "cam_kunit_cdr_write_overflow1_test",
	.test_cases = cdr_write_overflow1_test_cases,
};

struct kunit_suite cam_kunit_cdr_write_overflow2 = {
	.name = "cam_kunit_cdr_write_overflow2_test",
	.test_cases = cdr_write_overflow2_test_cases,
};

struct kunit_suite cam_kunit_cdr_write_invalid = {
	.name = "cam_kunit_cdr_write_invalid_test",
	.test_cases = cdr_write_invalid_test_cases,
};

struct kunit_suite cam_kunit_sen_util__get_seq_sen_id = {
	.name = "get_seq_sensor_id test",
	.test_cases = sensor_util_tc_get_seq_sen_id,
};

struct kunit_suite cam_kunit_sen_util__parse_reg = {
	.name = "cam_sensor_parse_reg test",
	.test_cases = sensor_util_tc_parse_reg,
};

struct kunit_suite cam_kunit_sen_util__check_aeb_on_ok = {
	.name = "cam_sensor_check_aeb_on OK test",
	.test_cases = sensor_util_tc_check_aeb_on_test_ok,
};

struct kunit_suite cam_kunit_sen_util__check_aeb_on_ng = {
	.name = "cam_sensor_check_aeb_on NG test",
	.test_cases = sensor_util_tc_check_aeb_on_test_ng,
};

struct kunit_suite cam_kunit_sen_util__get_vc_pick_idx = {
	.name = "get_vc_pick_idx test",
	.test_cases = sensor_util_tc_get_vc_pick_idx,
};

int cam_kunit_hw_bigdata_test(void)
{
	CAM_INFO(CAM_UTIL, "Start");

	kunit_run_tests(&cam_kunit_i2c_sensor_rear);
	kunit_run_tests(&cam_kunit_i2c_sensor_rear2);
	kunit_run_tests(&cam_kunit_i2c_sensor_rear3);
	kunit_run_tests(&cam_kunit_i2c_sensor_rear4);
	kunit_run_tests(&cam_kunit_i2c_sensor_front);
	kunit_run_tests(&cam_kunit_i2c_sensor_front_top);

	kunit_run_tests(&cam_kunit_i2c_af_rear);
	kunit_run_tests(&cam_kunit_i2c_af_rear2);
	kunit_run_tests(&cam_kunit_i2c_af_rear3);
	kunit_run_tests(&cam_kunit_i2c_af_rear4);
	kunit_run_tests(&cam_kunit_i2c_af_front);
	kunit_run_tests(&cam_kunit_i2c_af_front_top);

	kunit_run_tests(&cam_kunit_i2c_ois_rear);
	kunit_run_tests(&cam_kunit_i2c_ois_rear3);
	kunit_run_tests(&cam_kunit_i2c_ois_rear4);

	kunit_run_tests(&cam_kunit_mipi_sensor_rear);
	kunit_run_tests(&cam_kunit_mipi_sensor_rear2);
	kunit_run_tests(&cam_kunit_mipi_sensor_rear3);
	kunit_run_tests(&cam_kunit_mipi_sensor_rear4);
	kunit_run_tests(&cam_kunit_mipi_sensor_front);
	kunit_run_tests(&cam_kunit_mipi_sensor_front_top);

	kunit_run_tests(&cam_kunit_hw_bigdata_fill);
	kunit_run_tests(&cam_kunit_mipi_init_param);
	kunit_run_tests(&cam_kunit_file);

	CAM_INFO(CAM_UTIL, "End");

	return 0;
}

int cam_kunit_clock_data_recovery_test(void)
{
	CAM_INFO(CAM_UTIL, "Start");

	kunit_run_tests(&cam_kunit_apply_cdr_value);
	kunit_run_tests(&cam_kunit_apply_result_value);

	kunit_run_tests(&cam_kunit_cdr_write_normal);
	kunit_run_tests(&cam_kunit_cdr_write_overflow1);
	kunit_run_tests(&cam_kunit_cdr_write_overflow2);
	kunit_run_tests(&cam_kunit_cdr_write_invalid);

	CAM_INFO(CAM_UTIL, "End");

	return 0;
}

int cam_kunit_sensor_util_test(void)
{
	CAM_INFO(CAM_UTIL, "Start");

	kunit_run_tests(&cam_kunit_sen_util__get_seq_sen_id);
	kunit_run_tests(&cam_kunit_sen_util__parse_reg);
	kunit_run_tests(&cam_kunit_sen_util__check_aeb_on_ok);
	kunit_run_tests(&cam_kunit_sen_util__check_aeb_on_ng);
	kunit_run_tests(&cam_kunit_sen_util__get_vc_pick_idx);

	CAM_INFO(CAM_UTIL, "End");
	return 0;
}

MODULE_LICENSE("GPL v2");
