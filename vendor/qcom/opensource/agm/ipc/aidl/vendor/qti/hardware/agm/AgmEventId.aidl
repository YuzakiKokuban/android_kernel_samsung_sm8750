/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

/**
 * Data Events that will be notified to client from AGM
 */
@VintfStability
@Backing(type="int")
enum AgmEventId {
    // Indicates EOS rendered event
    AGMEVENTEOSRENDERED = 0x0,
    // Indicates buffer provided as part of read call has been filled.
    AGMEVENTREADDONE = 0x1,
    // Indicates buffer provided as part of write has been consumed
    AGMEVENTWRITEDONE = 0x2,
    // Indicates early EOS event
    AGM_EVENT_EARLY_EOS = 0x08001126,
    AGMEVENTIDMAX,
}
