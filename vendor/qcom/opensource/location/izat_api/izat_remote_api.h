/*=============================================================================
  Copyright (c) 2016, The Linux Foundation. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:
  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials provided
    with the distribution.
  * Neither the name of The Linux Foundation nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  =============================================================================

  Changes from Qualcomm Innovation Center are provided under the following license:
  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause-Clear

  =============================================================================*/

/*
Changes from Qualcomm Innovation Center are provided under the following license:
Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __IZAT_REMOTE_APIS_H__
#define __IZAT_REMOTE_APIS_H__

#include <gps_extended.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*locationUpdateCb)(UlpLocation *location,
                                 GpsLocationExtended *locExtended,
                                 void* clientData);

typedef void (*svRptUpdateCb)(GnssSvNotification *svNotify,
                                 void* clientData);

typedef void (*nmeaUpdateCb)(UlpNmea *nmea,  void* clientData);

typedef struct {

    locationUpdateCb    locCb;
    svRptUpdateCb       svReportCb;
    nmeaUpdateCb        nmeaCb;

}remoteClientInfo;
/* registers a client callback structure for listening to final location updates
    pClientInfo - pointer to remoteClientInfo structure
                Can not be NULL.
   clientData - an opaque data pointer from client. This pointer will be
                provided back when the callbacak is called.
                This can be used by client to store application context
                or statemachine etc.
                Can be NULL.
   return: an opaque pointer that serves as a request handle. This handle
           to be fed to the unregisterLocationUpdater() call.
*/
void* registerLocationUpdater(remoteClientInfo *pClientInfo, void* clientData);

/* unregisters the client callback
   locationUpdaterHandle - the opaque pointer from the return of
                           registerLocationUpdater()
*/
void unregisterLocationUpdater(void* locationUpdaterHandle);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__IZAT_REMOTE_APIS_H__
