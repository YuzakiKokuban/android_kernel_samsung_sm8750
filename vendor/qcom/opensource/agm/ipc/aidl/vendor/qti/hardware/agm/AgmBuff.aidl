/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.AgmExternAllocBuffInfo;

@VintfStability
parcelable AgmBuff {
     /**< timestamp in micro-secs */
    long timestamp;
     /**< bitmasked flags for e.g. AGM_BUFF_FLAG_EOS */
    int flags;
     /**< size of buffer in bytes */
    int size;
     /**< offset at which audio data is present */
    int offset;
     /**< data buffer */
    byte[] buffer;
     /**< Blob for metadata */
    byte[] metadata;
     /**< holds info for extern buff */
    AgmExternAllocBuffInfo externalAllocInfo;
}
