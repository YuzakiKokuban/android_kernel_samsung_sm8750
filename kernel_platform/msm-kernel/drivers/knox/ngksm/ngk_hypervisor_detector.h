/*
 * Copyright (c) 2020-2021 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef _NGK_HYPERVISOR_DETECTOR_H
#define _NGK_HYPERVISOR_DETECTOR_H

#ifdef NGK_PAD
extern int cmd_hyp_detector(long subsystem);
extern int __init ngk_hyp_detector_init(void);
extern void __exit ngk_hyp_detector_exit(void);
#else
static inline int cmd_hyp_detector(long subsystem)
{
	return 0;
}
static inline int ngk_hyp_detector_init(void)
{
	return 0;
}

static inline void ngk_hyp_detector_exit(void)
{
}
#endif



#endif /* _NGK_HYPERVISOR_DETECTOR_H */
