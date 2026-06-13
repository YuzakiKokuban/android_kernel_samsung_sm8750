/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * COPR :
 * Author: QC LCD driver <kr0124.cho@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SS_COPR_COMMON_H__
#define __SS_COPR_COMMON_H__

/* MAX COPR ROI is from copr ver. 3.0 */
#define MAX_COPR_ROI_CNT 6

enum COPR_CD_INDEX {
	COPR_CD_INDEX_0,	/* for copr show - SSRM */
	COPR_CD_INDEX_1,	/* for brt_avg show - mDNIe */
	MAX_COPR_CD_INDEX,
};

struct COPR_CD {
	s64 cd_sum;
	int cd_avr;

	ktime_t cur_t;
	ktime_t last_t;
	s64 total_t;
};

struct COPR {
	/* read data */
	struct mutex copr_lock;
	struct mutex copr_val_lock;

	struct COPR_CD copr_cd[MAX_COPR_CD_INDEX];
};

void ss_set_copr_sum(struct samsung_display_driver_data *vdd, enum COPR_CD_INDEX idx);

int ss_copr_init(struct samsung_display_driver_data *vdd);

#endif /* __SS_COPR_COMMON_H__ */
