#ifndef _CAM_SENSOR_DEBUG_H_
#define _CAM_SENSOR_DEBUG_H_

struct st_exposure_reg_dump_addr {
	uint32_t addr;
	enum camera_sensor_i2c_type	data_sz;
	const char* addr_name;
};

#endif //_CAM_SENSOR_DEBUG_H_