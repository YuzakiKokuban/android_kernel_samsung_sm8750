#ifndef _CAMERA_KUNIT_MAIN_H_
#define _CAMERA_KUNIT_MAIN_H_

#include <kunit/test.h>
#include <kunit/mock.h>
#include "cam_hw_bigdata_test.h"
#include "cam_clock_data_recovery_test.h"
#include "cam_sensor_util_unit_test.h"
#include "cam_sec_eeprom_core_test.h"

extern int cam_kunit_hw_bigdata_test(void);
extern int cam_kunit_clock_data_recovery_test(void);
extern int cam_kunit_sensor_util_test(void);

#endif /* _CAMERA_KUNIT_MAIN_H_ */