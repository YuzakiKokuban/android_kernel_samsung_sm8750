/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

@VintfStability
parcelable AgmEventCallbackParameter {
    /**< identifies the module which generated event */
    int sourceModuleId;
    /**< identifies the event */
    int eventId;
    /**< payload associated with the event if any */
    byte[] eventPayload;
}
