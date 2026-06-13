/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

@VintfStability
parcelable AgmSessionAacEnc {
    // encoder bitrate
    int bitRate;
    // global cutoff frequency
    int globalCutOffFrequency;
    // encoder mode
    int mode;
    // format flags
    int formatFlags;
}
