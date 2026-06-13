/*
 * ALSA SoC Texas Instruments TAS25XX High Performance Smart Amplifier
 *
 * Copyright (C) 2024 Texas Instruments, Inc.
 *
 * Author: Niranjan H Y
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __TAS25XX_EXT_H__
#define __TAS25XX_EXT_H__

enum tas_amp_status_t {
	TAS_AMP_ERR_EINVAL = -3,  /* invalid parameter */
	TAS_AMP_ERR_FW_LOAD = -2, /* amp fw download failure */
	TAS_AMP_ERR_I2C	= -1,     /* amp i2c error */
	TAS_AMP_STATE_UNINIT = 0, /* uninitialized state */
	TAS_AMP_STATE_BOOT_SUCCESS = 1, /* amp first i2c successful */
	TAS_AMP_STATE_FW_LOAD_SUCCESS = 2, /* amp fully initialized for playback */
};

enum tas_amp_error_t {
	TAS_AMP_NO_ERR = 0,
	TAS_AMP_SHORTCIRCUIT_ERR = 1,
	TAS_AMP_ERR_MAX,
};

/* get the current amp status */
enum tas_amp_status_t tas25xx_get_state(uint32_t id);

/* error callback for i2c error */
void tas25xx_register_i2c_error_callback(void (*i2c_err_cb)(uint32_t i2caddr));
void tas25xx_register_amp_error_callback(void (*amp_err_cb)(int32_t i2caddr, int32_t err));

#endif /* __TAS25XX_EXT_H__ */
