/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.AgmEventCallbackParameter;
import vendor.qti.hardware.agm.AgmReadWriteEventCallbackParams;

/*
* Interface used to register callback for events
*/
@VintfStability
interface IAGMCallback {
    /**
    * callback triggerred on receiving generic events from native agm/gsl
    * @param eventParam contains AgmEventCallbackParameter
    */
    void eventCallback(in AgmEventCallbackParameter eventParam);
    /**
    * callback triggerred on READ_DONE/WRITE_DONE events from native agm/gsl
    * mainly used for non tunnel.
    * @param rwDonePayload contains AgmReadWriteEventCallbackParams
    */
    void eventCallbackReadWriteDone(in AgmReadWriteEventCallbackParams rwDonePayload);
}
