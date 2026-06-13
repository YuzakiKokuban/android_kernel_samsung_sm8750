#ifndef CAM_SENSOR_UTIL_UNIT_TEST_H
#define CAM_SENSOR_UTIL_UNIT_TEST_H

#include "cam_sensor_util.h"

// kunit
void cam_sensor_util_tc_get_seq_sen_id(struct kunit *test);
void cam_sensor_util_tc_parse_reg(struct kunit *test);
void cam_sensor_util_tc_check_aeb_on_test_ok(struct kunit* test);
void cam_sensor_util_tc_check_aeb_on_test_ng(struct kunit* test);
void cam_sensor_util_tc_get_vc_pick_idx(struct kunit* test);

// external interface
void cam_sensor_hook_io_client(struct cam_sensor_ctrl_t* s_ctrl);
void cam_sensor_parse_reg(
	struct cam_sensor_ctrl_t* s_ctrl,
	struct i2c_settings_list* i2c_list,
	int32_t *debug_sen_id,
	e_sensor_reg_upd_event_type *sen_update_type);

int cam_sensor_check_aeb_on(struct cam_sensor_ctrl_t* s_ctrl);
int32_t get_vc_pick_idx(uint32_t sensor_id);

#endif /* CAM_SENSOR_UTIL_UNIT_TEST_H */