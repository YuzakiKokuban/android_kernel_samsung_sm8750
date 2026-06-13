/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

/**
 * AGM session modes
 */

@VintfStability
@Backing(type="int")
enum AgmSessionMode {
    AGM_SESSION_DEFAULT, /**< Normal agm tunnel session*/
    AGM_SESSION_NO_HOST, /**< Hostless mode */
    AGM_SESSION_NON_TUNNEL, /**< Non tunnel mode */
    AGM_SESSION_NO_CONFIG, /**< No Config mode*/
    AGM_SESSION_COMPRESS, /**< Compress mode*/
}
