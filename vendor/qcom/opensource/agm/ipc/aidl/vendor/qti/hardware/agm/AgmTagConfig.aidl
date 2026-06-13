/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.AgmKeyValue;

@VintfStability
parcelable AgmTagConfig {
    // tag id
    int tag;
    //tag key vector;
    AgmKeyValue[] kv;
}
