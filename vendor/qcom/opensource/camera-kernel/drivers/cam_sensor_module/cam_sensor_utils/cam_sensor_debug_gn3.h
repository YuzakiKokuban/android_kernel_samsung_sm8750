#ifndef _CAM_SENSOR_DEBUG_GN3_H_
#define _CAM_SENSOR_DEBUG_GN3_H_

const struct st_exposure_reg_dump_addr gn3_dump_addr_arr[] = {
	{ 0x034c, CAMERA_SENSOR_I2C_TYPE_WORD, "width" },
	{ 0x034e, CAMERA_SENSOR_I2C_TYPE_WORD, "height" },
};

#endif /* _CAM_SENSOR_DEBUG_GN3_H_ */