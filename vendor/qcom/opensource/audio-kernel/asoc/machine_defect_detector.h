/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 20204, The Linux Foundation. All rights reserved.
*/

#ifndef __MACHINE_DEFECT_DETECTOR_H__
#define __MACHINE_DEFECT_DETECTOR_H__

enum amp_result {
    NOT_SUPPORTED = -1,
    INIT_FAIL = 0,
    INIT_SUCCESS = 1,
};

void check_snd_component(struct snd_soc_card *card, int max_defer_count);
int register_amp_callback(void);

#endif

