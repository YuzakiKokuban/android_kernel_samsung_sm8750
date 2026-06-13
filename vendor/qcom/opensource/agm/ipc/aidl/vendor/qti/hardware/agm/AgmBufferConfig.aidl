/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

@VintfStability
parcelable AgmBufferConfig {
    /**< number of buffers */
    int count;
    /**< size of each buffer */
    int size;
    /**< max metadata size a client attaches to a buffer */
    int maxMetadataSize;
}
