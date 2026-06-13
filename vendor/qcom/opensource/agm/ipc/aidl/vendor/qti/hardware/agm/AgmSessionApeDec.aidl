/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

@VintfStability
parcelable AgmSessionApeDec {
    int compatibleVersion; /**< Version */
    int compressionLevel; /**< Compression Level */
    int formatFlags; /**< Format flags */
    int blocksPerFrame; /**< Blocks per frame */
    int finalFrameBlocks; /**< Final frame blocks */
    int totalFrames; /**< Total frames */
    int bitWidth; /**< Bit width */
    int channels; /**< Number of channels */
    int sampleRate; /**< Sample rate */
    int seekTablePresent; /**< Seek table present */
}
