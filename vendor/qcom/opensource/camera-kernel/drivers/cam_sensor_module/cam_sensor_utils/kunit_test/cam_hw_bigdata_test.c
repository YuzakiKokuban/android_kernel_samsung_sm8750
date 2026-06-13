// SPDX-License-Identifier: GPL-2.0
/*
 * TODO: Add test description.
 */

#include <linux/module.h>
#include "camera_kunit_main.h"
#include "cam_hw_bigdata_test.h"

#define REAR_OIS_X_Y_ERR_REG  0x0600
#define REAR3_OIS_X_Y_ERR_REG 0x1800
#define REAR4_OIS_X_Y_ERR_REG 0x6000

struct cam_sensor_ctrl_t *s_ctrl;
struct cam_actuator_ctrl_t *a_ctrl;
struct cam_hw_param *hw_param;

int current_err_cnt;
int next_err_cnt;

int hw_bigdata_test_init(struct kunit *test)
{
	s_ctrl = kmalloc(sizeof(struct cam_sensor_ctrl_t), GFP_KERNEL);
	a_ctrl = kmalloc(sizeof(struct cam_actuator_ctrl_t), GFP_KERNEL);

	hw_param = NULL;
	current_err_cnt = 0;
	next_err_cnt = 0;

	return 0;
}

void hw_bigdata_test_exit(struct kunit *test)
{
	if (s_ctrl) {
		kfree(s_ctrl);
		s_ctrl = NULL;
	}

	if (a_ctrl) {
		kfree(a_ctrl);
		a_ctrl = NULL;
	}
}

void hw_bigdata_i2c_sensor_rear_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR);
	s_ctrl->id = SEC_WIDE_SENSOR;

	hw_bigdata_i2c_from_sensor(s_ctrl);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_i2c_sensor_rear2_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR2);
	s_ctrl->id = SEC_ULTRA_WIDE_SENSOR;

	hw_bigdata_i2c_from_sensor(s_ctrl);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_i2c_sensor_rear3_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR3);
	s_ctrl->id = SEC_TELE_SENSOR;

	hw_bigdata_i2c_from_sensor(s_ctrl);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_i2c_sensor_rear4_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR4);
	s_ctrl->id = SEC_TELE2_SENSOR;

	hw_bigdata_i2c_from_sensor(s_ctrl);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_i2c_sensor_front_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_FRONT);
	s_ctrl->id = SEC_FRONT_SENSOR;

	hw_bigdata_i2c_from_sensor(s_ctrl);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_i2c_sensor_front_top_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_FRONT);
	s_ctrl->id = SEC_FRONT_TOP_SENSOR;

	hw_bigdata_i2c_from_sensor(s_ctrl);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}


void hw_bigdata_i2c_af_rear_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR);
	a_ctrl->soc_info.index = SEC_WIDE_SENSOR;

	hw_bigdata_i2c_from_actuator(a_ctrl);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_AF_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_i2c_af_rear2_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR2);
	a_ctrl->soc_info.index = SEC_ULTRA_WIDE_SENSOR;

	hw_bigdata_i2c_from_actuator(a_ctrl);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_AF_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_i2c_af_rear3_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR3);
	a_ctrl->soc_info.index = SEC_TELE_SENSOR;

	hw_bigdata_i2c_from_actuator(a_ctrl);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_AF_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_i2c_af_rear4_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR4);
	a_ctrl->soc_info.index = SEC_TELE2_SENSOR;

	hw_bigdata_i2c_from_actuator(a_ctrl);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_AF_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_i2c_af_front_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_FRONT);
	a_ctrl->soc_info.index = SEC_FRONT_SENSOR;

	hw_bigdata_i2c_from_actuator(a_ctrl);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_AF_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_i2c_af_front_top_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_FRONT);
	a_ctrl->soc_info.index = SEC_FRONT_TOP_SENSOR;

	hw_bigdata_i2c_from_actuator(a_ctrl);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_AF_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_i2c_ois_rear_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR);

	hw_bigdata_i2c_from_ois_status_reg(SEC_WIDE_SENSOR);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_OIS_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
	current_err_cnt = next_err_cnt;

	hw_bigdata_i2c_from_ois_error_reg(REAR_OIS_X_Y_ERR_REG);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_OIS_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_i2c_ois_rear3_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR3);

	hw_bigdata_i2c_from_ois_status_reg(SEC_TELE_SENSOR);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_OIS_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
	current_err_cnt = next_err_cnt;

	hw_bigdata_i2c_from_ois_error_reg(REAR3_OIS_X_Y_ERR_REG);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_OIS_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_i2c_ois_rear4_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR4);

	hw_bigdata_i2c_from_ois_status_reg(SEC_TELE2_SENSOR);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_OIS_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
	current_err_cnt = next_err_cnt;

	hw_bigdata_i2c_from_ois_error_reg(REAR4_OIS_X_Y_ERR_REG);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, I2C_OIS_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_mipi_init_param_test(struct kunit *test)
{
	s_ctrl->id = SEC_WIDE_SENSOR;
	hw_bigdata_init_mipi_param_sensor(s_ctrl);
	hw_bigdata_deinit_mipi_param_sensor(s_ctrl);
}

void hw_bigdata_mipi_sensor_rear_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR);

	hw_param->mipi_chk = FALSE;

	hw_bigdata_mipi_from_ife_csid_ver1(SEC_WIDE_SENSOR);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, MIPI_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
	current_err_cnt = next_err_cnt;

	hw_param->mipi_chk = FALSE;

	hw_bigdata_mipi_from_ife_csid_ver2(WIDE_CAM);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, MIPI_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_mipi_sensor_rear2_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR2);

	hw_param->mipi_chk = FALSE;

	hw_bigdata_mipi_from_ife_csid_ver1(SEC_ULTRA_WIDE_SENSOR);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, MIPI_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
	current_err_cnt = next_err_cnt;

	hw_param->mipi_chk = FALSE;

	hw_bigdata_mipi_from_ife_csid_ver2(UW_CAM);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, MIPI_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_mipi_sensor_rear3_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR3);

	hw_param->mipi_chk = FALSE;

	hw_bigdata_mipi_from_ife_csid_ver1(SEC_TELE_SENSOR);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, MIPI_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
	current_err_cnt = next_err_cnt;

	hw_param->mipi_chk = FALSE;

	hw_bigdata_mipi_from_ife_csid_ver2(TELE1_CAM);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, MIPI_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_mipi_sensor_rear4_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR4);

	hw_param->mipi_chk = FALSE;

	hw_bigdata_mipi_from_ife_csid_ver1(SEC_TELE2_SENSOR);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, MIPI_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
	current_err_cnt = next_err_cnt;

	hw_param->mipi_chk = FALSE;

	hw_bigdata_mipi_from_ife_csid_ver2(TELE2_CAM);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, MIPI_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_mipi_sensor_front_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_FRONT);

	hw_param->mipi_chk = FALSE;

	hw_bigdata_mipi_from_ife_csid_ver1(SEC_FRONT_SENSOR);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, MIPI_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
	current_err_cnt = next_err_cnt;

	hw_param->mipi_chk = FALSE;

	hw_bigdata_mipi_from_ife_csid_ver2(FRONT_CAM);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, MIPI_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_mipi_sensor_front_top_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_FRONT);

	hw_param->mipi_chk = FALSE;

	hw_bigdata_mipi_from_ife_csid_ver1(SEC_FRONT_TOP_SENSOR);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, MIPI_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
	current_err_cnt = next_err_cnt;

	hw_param->mipi_chk = FALSE;

	hw_bigdata_mipi_from_ife_csid_ver2(FRONT_CAM);
	next_err_cnt = hw_bigdata_get_error_cnt(hw_param, MIPI_SENSOR_ERROR);

	KUNIT_EXPECT_EQ(test, (next_err_cnt > current_err_cnt), TRUE);
}

void hw_bigdata_fill_test(struct kunit *test)
{
	char output[4096] = { 0, };
	int hw_param_id = HW_PARAM_REAR;
	uint8_t mId[FROM_MODULE_ID_SIZE + 1] = { 0, };
	char wifi[128] = "53206";
	struct cam_hw_param ec_param;

#if defined(CONFIG_SAMSUNG_ACTUATOR_READ_HALL_VALUE)
	char* expect_str = "\"CAMIR_ID\":\"SVQH8XX000317\",\"I2CR_AF\":\"1\"," \
						"\"I2CR_OIS\":\"2\",\"I2CR_SEN\":\"3\",\"MIPIR_SEN\":\"4\"," \
						"\"MIPIR_INFO\":\"1,2,3\",\"I2CR_EEP\":\"5\",\"CRCR_EEP\":\"6\"," \
						"\"CAMR_CNT\":\"10\",\"WIFIR_INFO\":\"53206\"," \
						"\"AFR_FAIL\":\"0\",\"AFR_INFO\":\"0,0\"\n";
#else
	char* expect_str = "\"CAMIR_ID\":\"SVQH8XX000317\",\"I2CR_AF\":\"1\"," \
						"\"I2CR_OIS\":\"2\",\"I2CR_SEN\":\"3\",\"MIPIR_SEN\":\"4\"," \
						"\"MIPIR_INFO\":\"1,2,3\",\"I2CR_EEP\":\"5\",\"CRCR_EEP\":\"6\"," \
						"\"CAMR_CNT\":\"10\",\"WIFIR_INFO\":\"53206\"\n";
#endif

	mId[0] = 'S';
	mId[1] = 'V';
	mId[2] = 'Q';
	mId[3] = 'H';
	mId[4] = '8';
	mId[5] = 0x1F;
	mId[6] = 0xBA;
	mId[7] = 0x00;
	mId[8] = 0x03;
	mId[9] = 0x17;

	ec_param.err_cnt[I2C_AF_ERROR] = 1;
	ec_param.err_cnt[I2C_OIS_ERROR] = 2;
	ec_param.err_cnt[I2C_SENSOR_ERROR] = 3;
	ec_param.err_cnt[MIPI_SENSOR_ERROR] = 4;
	ec_param.rf_rat = 1;
	ec_param.rf_band = 2;
	ec_param.rf_channel = 3;

	ec_param.err_cnt[I2C_EEPROM_ERROR] = 5;
	ec_param.err_cnt[CRC_EEPROM_ERROR] = 6;
	ec_param.cam_entrance_cnt = 10;

	hw_bigdata_fill(output, &ec_param, mId, "0,0|0", hw_param_id, wifi);

	KUNIT_EXPECT_STREQ(test, expect_str, output);
}


void hw_bigdata_file_test(struct kunit *test)
{
	hw_bigdata_get_hw_param(&hw_param, HW_PARAM_REAR);
	hw_bigdata_init_all_cnt();
	hw_bigdata_init_err_cnt_file(hw_param);
	hw_bigdata_copy_err_cnt_from_file();
}

MODULE_LICENSE("GPL v2");
