#ifndef _CAM_CLOCK_DATA_RECOVERY_TEST_H_
#define _CAM_CLOCK_DATA_RECOVERY_TEST_H_

#include "cam_clock_data_recovery.h"

void cam_clock_data_recovery_apply_value_test(struct kunit *test);
void cam_clock_data_recovery_apply_result_test(struct kunit *test);

void cam_clock_data_recovery_write_normal_test(struct kunit *test);
void cam_clock_data_recovery_write_overflow1_test(struct kunit *test);
void cam_clock_data_recovery_write_overflow2_test(struct kunit *test);
void cam_clock_data_recovery_write_invalid_test(struct kunit *test);

#endif /* _CAM_CLOCK_DATA_RECOVERY_TEST_H_ */
