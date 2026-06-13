/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _RESOURCES_EXT_H_
#define _RESOURCES_EXT_H_

struct msm_vidc_resources_ops;

const struct msm_vidc_resources_ops *get_res_ops_ext(struct msm_vidc_core *core);

#endif // _RESOURCES_EXT_H_
