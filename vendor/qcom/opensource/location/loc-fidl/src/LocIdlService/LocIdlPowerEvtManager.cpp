/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted (subject to the limitations in the
disclaimer below) provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "IDL_POWER_HANDLER"
#ifdef POWER_DAEMON_MGR_ENABLED
#include <unistd.h>
#include "LocIdlPowerEvtManager.h"

#define ACK_TIMEOUT_US 300000 // 300 msec

LocIdlAPIService* LocIdlPowerEvtHandler::mLocationApiService = nullptr;

LocIdlPowerEvtHandler* LocIdlPowerEvtHandler::getPwrEvtHandler(LocIdlAPIService* locServiceApiObj) {
    mLocationApiService = locServiceApiObj;
    static LocIdlPowerEvtHandler instance;
    return &instance;
}

LocIdlPowerEvtHandler::LocIdlPowerEvtHandler() {
    int ret = pwr_state_notification_register(LocIdlPowerEvtHandler::pwrMngrLibStateCb);
}

LocIdlPowerEvtHandler::~LocIdlPowerEvtHandler() {
}

int LocIdlPowerEvtHandler::pwrMngrLibStateCb(power_state_t pwr_state) {
    client_ack_t client_ack;
    client_ack.ack = ERR;
    IDLPowerStateType powerState = IDL_POWER_STATE_UNKNOWN;
    LOC_LOGe("Received powerState %d", pwr_state);
    switch (pwr_state.sys_state) {
        case SYS_SUSPEND:
            client_ack.ack = SUSPEND_ACK;
            powerState = IDL_POWER_STATE_SUSPEND;
            break;
        case SYS_RESUME:
            client_ack.ack = RESUME_ACK;
            powerState = IDL_POWER_STATE_RESUME;
            break;
        case SYS_SHUTDOWN:
            client_ack.ack = SHUTDOWN_ACK;
            powerState = IDL_POWER_STATE_SHUTDOWN;
            break;
    }

    if (powerState != IDL_POWER_STATE_UNKNOWN) {
        if (mLocationApiService) {
                mLocationApiService->onPowerEvent(powerState);
        }
    }

    //Allow some time to stop the session and write calibration data NVM.
    usleep(ACK_TIMEOUT_US);
    LOC_LOGd("LocIdlPowerEvtHandler: pwrStateCb sending ack");
    send_acknowledgement(client_ack);

    return 0;
}
#endif
