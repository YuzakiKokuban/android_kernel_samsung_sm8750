/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

/**
 * Event types
 */
@VintfStability
@Backing(type="int")
enum EventType {
    AGM_EVENT_DATA_PATH = 1,
    AGM_EVENT_MODULE,
}
