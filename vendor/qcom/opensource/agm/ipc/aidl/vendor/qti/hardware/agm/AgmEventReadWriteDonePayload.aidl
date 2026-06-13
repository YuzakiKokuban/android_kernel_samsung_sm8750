/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.AgmBuff;

@VintfStability
parcelable AgmEventReadWriteDonePayload {
    // tag that was used to read/write this buffer
    int tag;
    // data buffer status
    int status;
    // metadata status
    int metadataStatus;
    // buffer that was passed to agm_read/agm_write
    AgmBuff buffer;
}
