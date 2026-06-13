/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

@VintfStability
parcelable AgmSessionOpusDec {
    int bitStreamFormat;
    int type;
    byte version;
    byte channels;
    int preSkip;
    long sampleRate;
    int outputGain;
    byte mappingFamily;
    byte streamCount;
    byte coupledCount;
    byte [8] channelMap;
    byte [3] reserved;
}
