/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

import android.hardware.common.NativeHandle;

@VintfStability
parcelable MmapBufInfo {
    NativeHandle dataFdHandle;
    int dataSize;
    NativeHandle positionFdHandle;
    int posSize;
}
