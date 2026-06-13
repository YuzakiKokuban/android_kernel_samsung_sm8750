/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.AgmMediaFormat;

/**
 * Media Config
 */
@VintfStability
parcelable AgmMediaConfig {
    // sample rate
    int rate;
    // number of channels
    int channels;
    // media format in agm_media_format
    AgmMediaFormat format;
    // data format
    int dataFormat;
}
