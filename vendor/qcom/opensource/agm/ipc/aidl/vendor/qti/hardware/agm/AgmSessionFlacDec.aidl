/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

@VintfStability
parcelable AgmSessionFlacDec {
    int channels; /**< Number of channels */
    int sampleSize; /**< Sample size */
    int minBlockSize; /**< Minimum block size */
    int maxBlockSize; /**< Maximum block size */
    int sampleRate; /**< Sample rate */
    int minFrameSize; /**< Minimum frame size */
    int maxFrameSize; /**< Maximum frame size */
}
