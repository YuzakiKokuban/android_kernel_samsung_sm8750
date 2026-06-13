/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

/**
 * Event registration structure.
 */
@VintfStability
parcelable AgmEventRegistrationConfig {
    // Valid instance ID of module
    int moduleInstanceId;
    // Valid event ID of the module
    int eventId;
    // register or unregister 1 for register, 0 for unregister
    byte registerEvent;
    // module specifc event registration payload
    byte[] eventConfigPayload;
}
