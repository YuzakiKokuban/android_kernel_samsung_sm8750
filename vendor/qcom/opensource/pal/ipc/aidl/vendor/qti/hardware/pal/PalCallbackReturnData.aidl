/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

import vendor.qti.hardware.pal.PalReadWriteDoneResult;
import vendor.qti.hardware.pal.PalReadWriteDoneCommand;
import android.hardware.common.fmq.MQDescriptor;
import android.hardware.common.fmq.SynchronizedReadWrite;

@VintfStability
parcelable PalCallbackReturnData {
    PalReadWriteDoneResult ret;
    MQDescriptor<byte, SynchronizedReadWrite> mqDataDesc;
    MQDescriptor<PalReadWriteDoneCommand, SynchronizedReadWrite> mqCommandDesc;
}
