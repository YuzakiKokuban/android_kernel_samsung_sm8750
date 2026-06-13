/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

@VintfStability
@Backing(type="int")
enum AgmDataMode {
    AGM_DATA_INVALID,
    AGM_DATA_BLOCKING,
    AGM_DATA_NON_BLOCKING,
    AGM_DATA_PUSH_PULL,
    AGM_DATA_EXTERN_MEM,
    AGM_DATA_MODE_MAX,
}
