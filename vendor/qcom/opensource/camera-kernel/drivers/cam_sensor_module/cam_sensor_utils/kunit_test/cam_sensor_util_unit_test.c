#include "camera_kunit_main.h"

struct camera_io_master sensor_io_master[e_seq_sensor_idn_max_invalid] = {0,};

void cam_sensor_util_tc_get_seq_sen_id(struct kunit *test)
{
	e_seq_sensor_idnum result;

	result = get_seq_sensor_id(SENSOR_ID_S5KHP2);
	KUNIT_ASSERT_EQ_MSG(test, e_seq_sensor_idn_s5khp2, result,
						"[KUNIT_ERR] func[%s] line[%d] expected %d, but %d",
						__FUNCTION__, __LINE__,
						e_seq_sensor_idn_s5kgn3, result);

	result = get_seq_sensor_id(SENSOR_ID_S5KGN3);
	KUNIT_ASSERT_EQ_MSG(test, e_seq_sensor_idn_s5kgn3, result,
						"[KUNIT_ERR] func[%s] line[%d] expected %d, but %d",
						__FUNCTION__, __LINE__,
						e_seq_sensor_idn_s5kgn3, result);

	result = get_seq_sensor_id(SENSOR_ID_S5K3LU);
	KUNIT_ASSERT_EQ_MSG(test, e_seq_sensor_idn_s5k3lu, result,
						"[KUNIT_ERR] func[%s] line[%d] expected %d, but %d",
						__FUNCTION__, __LINE__,
						e_seq_sensor_idn_s5k3lu, result);

	result = get_seq_sensor_id(SENSOR_ID_IMX564);
	KUNIT_ASSERT_EQ_MSG(test, e_seq_sensor_idn_imx564, result,
						"[KUNIT_ERR] func[%s] line[%d] expected %d, but %d",
						__FUNCTION__, __LINE__,
						e_seq_sensor_idn_imx564, result);

	result = get_seq_sensor_id(SENSOR_ID_IMX754);
	KUNIT_ASSERT_EQ_MSG(test, e_seq_sensor_idn_imx754, result,
						"[KUNIT_ERR] func[%s] line[%d] expected %d, but %d",
						__FUNCTION__, __LINE__,
						e_seq_sensor_idn_imx754, result);

	result = get_seq_sensor_id(SENSOR_ID_S5K2LD);
	KUNIT_ASSERT_EQ_MSG(test, e_seq_sensor_idn_s5k2ld, result,
						"[KUNIT_ERR] func[%s] line[%d] expected %d, but %d",
						__FUNCTION__, __LINE__,
						e_seq_sensor_idn_s5k2ld, result);

	result = get_seq_sensor_id(SENSOR_ID_S5K3K1);
	KUNIT_ASSERT_EQ_MSG(test, e_seq_sensor_idn_s5k3k1, result,
						"[KUNIT_ERR] func[%s] line[%d] expected %d, but %d",
						__FUNCTION__, __LINE__,
						e_seq_sensor_idn_s5k3k1, result);

	result = get_seq_sensor_id(9999);
	KUNIT_ASSERT_EQ_MSG(test, e_seq_sensor_idn_max_invalid, result,
						"[KUNIT_ERR] func[%s] line[%d] expected %d, but %d",
						__FUNCTION__, __LINE__,
						e_seq_sensor_idn_max_invalid, result);
}

const struct
{
	int32_t id;
	const char *name;
} supported_sensors[] = {
	{SENSOR_ID_S5KGN3, "S5KGN3"},
	{SENSOR_ID_S5K3K1, "S5K3K1"},
	{SENSOR_ID_IMX754, "IMX754"},
	{SENSOR_ID_S5K3LU, "S5K3LU"},
	{SENSOR_ID_IMX564, "IMX564"},
	{SENSOR_ID_S5KHP2, "S5KHP2"},
	{SENSOR_ID_IMX258, "IMX258"},
	{SENSOR_ID_IMX471, "IMX471"},
	{SENSOR_ID_IMX374, "IMX374"},
	{SENSOR_ID_S5K2LD, "S5K2LD"},
	{SENSOR_ID_S5K3J1, "S5K3J1"},
};

void cam_sensor_util_tc_parse_reg(struct kunit *test)
{
	int i = 0;
	struct cam_sensor_ctrl_t s_ctrl = {0};
	struct cam_sensor_board_info sensor_data = {0};
	struct i2c_settings_list i2c_list;
	int32_t dbg_sen_id = -1;
	e_sensor_reg_upd_event_type update_type = 0;
	struct cam_sensor_i2c_reg_array i2c_reg_set[] = {
		{
			.reg_addr = 0x0e00,
			.reg_data = 0x0000,
		},
	};
	s_ctrl.sensordata = &sensor_data;

	i2c_list.op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
	i2c_list.i2c_settings.reg_setting = i2c_reg_set;
	i2c_list.i2c_settings.size = 1;

	for (i = 0; i < ARRAY_SIZE(supported_sensors); i++)
	{
		s_ctrl.sensordata->slave_info.sensor_id = supported_sensors[i].id;
		cam_sensor_parse_reg(&s_ctrl, &i2c_list, &dbg_sen_id, &update_type);

		KUNIT_EXPECT_EQ_MSG(test, dbg_sen_id, s_ctrl.sensordata->slave_info.sensor_id,
							"[KUNIT_ERR] %s Expected sensor ID %x, but got %x",
							supported_sensors[i].name,
							s_ctrl.sensordata->slave_info.sensor_id, dbg_sen_id);

		KUNIT_EXPECT_EQ_MSG(test, update_type, e_sensor_upd_event_vc,
							"[KUNIT_ERR] Expected update_type %d, but got %d",
							e_sensor_upd_event_vc, update_type);
	}
}


void cam_sensor_util_tc_check_aeb_on_test_ok(struct kunit *test)
{
	int i = 0;
	struct cam_sensor_ctrl_t s_ctrl;
	struct cam_sensor_board_info sensor_data = {0};
	int aeb_on_value = -1;

	s_ctrl.sensordata = &sensor_data;

	for (i = 0; i < ARRAY_SIZE(supported_sensors); i++)
	{
		s_ctrl.sensordata->slave_info.sensor_id = supported_sensors[i].id;
		s_ctrl.io_master_info = sensor_io_master[get_seq_sensor_id(s_ctrl.sensordata->slave_info.sensor_id)];
		aeb_on_value = cam_sensor_check_aeb_on(&s_ctrl);

		KUNIT_EXPECT_GE_MSG(test, aeb_on_value, 0,
							"[KUNIT_ERR] %s Expected ret > %d , but got %d",
							supported_sensors[i].name,
							0, aeb_on_value);
	}
}

void cam_sensor_util_tc_check_aeb_on_test_ng(struct kunit *test)
{
	int i = 0;
	struct cam_sensor_ctrl_t s_ctrl;
	struct cam_sensor_board_info sensor_data = {0};
	int aeb_on_value = -1;

	s_ctrl.sensordata = &sensor_data;

	for (i = 0; i < ARRAY_SIZE(supported_sensors); i++)
	{
		s_ctrl.sensordata->slave_info.sensor_id = supported_sensors[i].id;
		s_ctrl.io_master_info = sensor_io_master[get_seq_sensor_id(s_ctrl.sensordata->slave_info.sensor_id)];
		aeb_on_value = cam_sensor_check_aeb_on(&s_ctrl);

		KUNIT_EXPECT_EQ_MSG(test, aeb_on_value, -1,
							"[KUNIT_ERR] %s Expected ret > %d , but got %d",
							supported_sensors[i].name,
							0, aeb_on_value);
	}
}


void cam_sensor_util_tc_get_vc_pick_idx(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, e_vc_addr_260_264_1b_read, get_vc_pick_idx(SENSOR_ID_S5KGN3));
	KUNIT_EXPECT_EQ(test, e_vc_addr_260_264_1b_read, get_vc_pick_idx(SENSOR_ID_S5K3LU));
	KUNIT_EXPECT_EQ(test, e_vc_addr_110_264, get_vc_pick_idx(SENSOR_ID_S5KHP2));
	KUNIT_EXPECT_EQ(test, e_vc_addr_110_30b0, get_vc_pick_idx(SENSOR_ID_IMX564));
	KUNIT_EXPECT_EQ(test, e_vc_addr_110_30b0, get_vc_pick_idx(SENSOR_ID_IMX754));
	KUNIT_EXPECT_EQ(test, e_vc_addr_602a_6f12, get_vc_pick_idx(SENSOR_ID_S5K2LD));
	KUNIT_EXPECT_EQ(test, e_vc_addr_invalid, get_vc_pick_idx(999));
}



void cam_sensor_hook_io_client(struct cam_sensor_ctrl_t* s_ctrl)
{
	e_seq_sensor_idnum id = get_seq_sensor_id(s_ctrl->sensordata->slave_info.sensor_id);
	if (id != e_seq_sensor_idn_max_invalid) {
		sensor_io_master[id] = s_ctrl->io_master_info;
	}
}