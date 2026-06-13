/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

import android.hardware.common.NativeHandle;
/**
 * Externally allocated buffer info
 */
@VintfStability
parcelable AgmExternAllocBuffInfo {
    // unique native handle identifying extern memory allocation
    NativeHandle allocHandle;
    // size of external allocation
    int allocatedSize;
    //  offset of buffer within extern allocation
    int offset;
}
