/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

import vendor.qti.hardware.pal.PalBuffer;

/**
 * Event payload passed to client with PAL_STREAM_CBK_EVENT_READ_DONE and
 * PAL_STREAM_CBK_EVENT_WRITE_READY events
 */
@VintfStability
parcelable PalEventReadWriteDonePayload {
    int tag;
    /**
     * < tag that was used to read/write this buffer
     */
    int status;
    /**
     * < data buffer status as defined in ar_osal_error.h
     */
    int mdStatus;
    /**
     * < meta-data status as defined in ar_osal_error.h
     */
    PalBuffer buff;
}
