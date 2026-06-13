/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

@VintfStability
parcelable AgmSessionWmaproDec {
    int formatTag; /**< Format Tag */
    int channels; /**< Number of channels */
    int sampleRate; /**< Sample rate */
    int averageBytesPerSecond; /**< Avg bytes per sec */
    int blockAlign; /**< Block align */
    int bitsPerSample; /**< Bits per sample */
    int channelMask; /**< Channel mask */
    int encoderOption; /**< Encoder options */
    int advancedEncoderOption; /**< Adv encoder options */
    int advancedEncoderOption2; /**< Adv encoder options2 */
}
