#ifndef _CAM_HW_BIGDATA_TEST_H_
#define _CAM_HW_BIGDATA_TEST_H_

#include "cam_hw_bigdata.h"

int hw_bigdata_test_init(struct kunit *test);
void hw_bigdata_test_exit(struct kunit *test);
void hw_bigdata_i2c_sensor_rear_test(struct kunit *test);
void hw_bigdata_i2c_sensor_rear2_test(struct kunit *test);
void hw_bigdata_i2c_sensor_rear3_test(struct kunit *test);
void hw_bigdata_i2c_sensor_rear4_test(struct kunit *test);
void hw_bigdata_i2c_sensor_front_test(struct kunit *test);
void hw_bigdata_i2c_sensor_front_top_test(struct kunit *test);
void hw_bigdata_i2c_af_rear_test(struct kunit *test);
void hw_bigdata_i2c_af_rear2_test(struct kunit *test);
void hw_bigdata_i2c_af_rear3_test(struct kunit *test);
void hw_bigdata_i2c_af_rear4_test(struct kunit *test);
void hw_bigdata_i2c_af_front_test(struct kunit *test);
void hw_bigdata_i2c_af_front_top_test(struct kunit *test);
void hw_bigdata_i2c_ois_rear_test(struct kunit *test);
void hw_bigdata_i2c_ois_rear3_test(struct kunit *test);
void hw_bigdata_i2c_ois_rear4_test(struct kunit *test);
void hw_bigdata_mipi_sensor_rear_test(struct kunit *test);
void hw_bigdata_mipi_sensor_rear2_test(struct kunit *test);
void hw_bigdata_mipi_sensor_rear3_test(struct kunit *test);
void hw_bigdata_mipi_sensor_rear4_test(struct kunit *test);
void hw_bigdata_mipi_sensor_front_test(struct kunit *test);
void hw_bigdata_mipi_sensor_front_top_test(struct kunit *test);
void hw_bigdata_fill_test(struct kunit *test);
void hw_bigdata_mipi_init_param_test(struct kunit *test);
void hw_bigdata_file_test(struct kunit *test);

#endif /* _CAM_HW_BIGDATA_TEST_H_ */