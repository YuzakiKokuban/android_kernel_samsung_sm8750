/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

/**
 * Gapless playback Silence type
 */
@VintfStability
@Backing(type="int")
enum AgmGaplessSilenceType {
    // Initial silence sample to be removed
    INITIAL_SILENCE,
    // Trailing silence sample to be removed
    TRAILING_SILENCE,
}
