/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.AgmEventReadWriteDonePayload;

@VintfStability
parcelable AgmReadWriteEventCallbackParams {
    // identifies the module which generated event
    int sourceModuleId;
    // identifies the event
    int eventId;
    // payload associated with the event if any
    AgmEventReadWriteDonePayload payload;
}
