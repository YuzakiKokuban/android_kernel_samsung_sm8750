/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _CAM_SEC_ACTUATOR_CORE_H_
#define _CAM_SEC_ACTUATOR_CORE_H_

#include "cam_actuator_dev.h"

#if defined(CONFIG_SAMSUNG_ACTUATOR_READ_HALL_VALUE)
int32_t cam_sec_actuator_read_hall_value(struct cam_actuator_ctrl_t *a_ctrl, uint16_t* af_hall_value);
#endif

#endif /* _CAM_SEC_ACTUATOR_CORE_H_ */
