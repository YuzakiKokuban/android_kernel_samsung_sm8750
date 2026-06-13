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

#include "LocIdlServiceLog.h"


static LocDiagInterface *locDiagIfaceHandle = nullptr;

bool LocIdlServiceLog::initializeDiagIface()
{
    LOC_LOGd("");
    bool retVal = true;
    if (nullptr == locDiagIfaceHandle) {
        locDiagIfaceHandle = loadLocDiagIfaceInterface();
        if (nullptr == locDiagIfaceHandle) {
            LOC_LOGe("Failed to loadLocDiagIfaceInterface!!");
            retVal = false;
        }
    }
    return retVal;
}
void LocIdlServiceLog::updateSystemHealth(uint32_t totalRss, bool gptpSyncStatus) {
    mTotalRss = totalRss;
    mGptpSyncStatus = gptpSyncStatus;
}

void LocIdlServiceLog::populateIdlDiagHeaderInfo(diagServiceGenericHeader& idlHeader) {
    idlHeader.version = LOG_LOCATION_IDL_SERVICE_REPORT_VERSION;
    idlHeader.idlServiceRssStats = mTotalRss;
    idlHeader.gptpSyncStatus = mGptpSyncStatus;
}

void LocIdlServiceLog::diagLogGnssReportInfo(uint8_t reportType, int16_t latencyMs,
                                                   uint32_t latentReportCount) {
    size_t size = 0;
    diagServiceInfoStruct*  gnssReportInfo = NULL;
    diagBuffSrc bufferSrc;
    size = sizeof(diagServiceInfoStruct)- sizeof(gnssReportInfo->serviceReport) +
            sizeof(diagOutputGnssReportInfo);
    if (locDiagIfaceHandle) {
        gnssReportInfo = (diagServiceInfoStruct*)locDiagIfaceHandle->logAlloc(
                LOG_LOCATION_IDL_SERVICE_REPORT_C,
                size, &bufferSrc);

        if (gnssReportInfo) {
            gnssReportInfo->serviceInfoType = GNSS_REPORT_INFO;
            populateIdlDiagHeaderInfo(gnssReportInfo->header);
            gnssReportInfo->serviceReport.reportInfo.reportType =
                    (diagServiceOutputReportType)reportType;
            gnssReportInfo->serviceReport.reportInfo.packetLatencyTime = latencyMs;
            gnssReportInfo->serviceReport.reportInfo.latentReportCount = latentReportCount;
            locDiagIfaceHandle->logCommit(gnssReportInfo, bufferSrc,
                    LOG_LOCATION_IDL_SERVICE_REPORT_VERSION, size);
        } else {
            LOC_LOGe(" logAlloc failed for Delete Aiding Request !! ");
        }
    } else {
        LOC_LOGe(" locDiagIfaceHandle is NULL ");
    }
}

void LocIdlServiceLog::diagLogConfigConstellationRequest(uint64_t clientIdentifier,
        vector< LocIdlAPI::IDLGnssSvIdInfo > svListSrc) {

    size_t size = 0;
    diagServiceInfoStruct*  svListInfo = NULL;
    diagBuffSrc bufferSrc;
    uint16_t numSvReceived = svListSrc.size();
    size = sizeof(diagServiceInfoStruct)- sizeof(svListInfo->serviceReport) +
                   sizeof(uint8_t) + sizeof(diagGnssSvIdInfo)*numSvReceived;

    if (locDiagIfaceHandle) {
        svListInfo = (diagServiceInfoStruct*)locDiagIfaceHandle->logAlloc(
                 LOG_LOCATION_IDL_SERVICE_REPORT_C,
                 size, &bufferSrc);

        if (svListInfo) {
            svListInfo->header.clientIdentifier = clientIdentifier;
            svListInfo->serviceInfoType = CONFIG_API_ACCESS_INFO;
            populateIdlDiagHeaderInfo(svListInfo->header);
            svListInfo->serviceReport.configApiInfo.requestType = CONFIG_CONSTELLATIONS_REQUEST;
            for (int i = 0; i < numSvReceived; i++) {
                svListInfo->serviceReport.configApiInfo.configData.svList[i].constellationType =
                        svListSrc[i].getConstellation();
                svListInfo->serviceReport.configApiInfo.configData.svList[i].svId =
                        svListSrc[i].getSvId();
            }
            locDiagIfaceHandle->logCommit(svListInfo, bufferSrc,
                    LOG_LOCATION_IDL_SERVICE_REPORT_VERSION, size);
        } else {
            LOC_LOGe(" logAlloc failed for Delete Aiding Request !! ");
        }
    } else {
        LOC_LOGe(" locDiagIfaceHandle is NULL ");
    }
}

void LocIdlServiceLog::diagLogDeleteAidingRequest (uint64_t clientIdentifier,
                                                           uint32_t aidingMask) {

    size_t size = 0;
    diagServiceInfoStruct*  aidingInfo = NULL;
    diagBuffSrc bufferSrc;
    size = sizeof(diagServiceInfoStruct)- sizeof(aidingInfo->serviceReport) +
                   sizeof(uint8_t) + sizeof(uint32_t);
    if (locDiagIfaceHandle) {
        aidingInfo = (diagServiceInfoStruct*)locDiagIfaceHandle->logAlloc(
                 LOG_LOCATION_IDL_SERVICE_REPORT_C,
                 size, &bufferSrc);
        if (aidingInfo) {
            aidingInfo->header.clientIdentifier = clientIdentifier;
            aidingInfo->serviceInfoType = CONFIG_API_ACCESS_INFO;
            populateIdlDiagHeaderInfo(aidingInfo->header);
            aidingInfo->serviceReport.configApiInfo.requestType = DELETE_AIDING_DATA;
            aidingInfo->serviceReport.configApiInfo.configData.deleteAidingMask = aidingMask;
            locDiagIfaceHandle->logCommit(aidingInfo, bufferSrc,
                    LOG_LOCATION_IDL_SERVICE_REPORT_VERSION, size);
        } else {
            LOC_LOGe(" logAlloc failed for Delete Aiding Request !! ");
        }
    } else {
        LOC_LOGe(" locDiagIfaceHandle is NULL ");
    }
}

void LocIdlServiceLog::diagLogSessionInfo (diagControlCommandInfo idlSessionInfo,
                                                uint64_t clientIdentifier) {

    size_t size = 0;
    diagServiceInfoStruct*  sessionInfo = NULL;
    diagBuffSrc bufferSrc;
    size = sizeof(diagServiceInfoStruct)- sizeof(sessionInfo->serviceReport) +
                   sizeof(diagControlCommandInfo);
        if (locDiagIfaceHandle) {
        sessionInfo = (diagServiceInfoStruct*)locDiagIfaceHandle->logAlloc(
                 LOG_LOCATION_IDL_SERVICE_REPORT_C,
                 size, &bufferSrc);

        if (sessionInfo) {
            sessionInfo->header.clientIdentifier = clientIdentifier;
            sessionInfo->serviceInfoType = SESSION_CONTROL_INFO;
            populateIdlDiagHeaderInfo(sessionInfo->header);
            sessionInfo->serviceReport.cmdInfo.sessionRequestType =
                    idlSessionInfo.sessionRequestType;
            sessionInfo->serviceReport.cmdInfo.intervalMs = idlSessionInfo.intervalMs;
            sessionInfo->serviceReport.cmdInfo.requestedCallbackMask =
                    idlSessionInfo.requestedCallbackMask;
            sessionInfo->serviceReport.cmdInfo.updatedCallbackMask =
                    idlSessionInfo.updatedCallbackMask;
            sessionInfo->serviceReport.cmdInfo.numControlRequests =
                    idlSessionInfo.numControlRequests;
            locDiagIfaceHandle->logCommit(sessionInfo, bufferSrc,
                    LOG_LOCATION_IDL_SERVICE_REPORT_VERSION, size);
        } else {
            LOC_LOGe(" logAlloc failed for session event Info !! ");
        }
    } else {
        LOC_LOGe(" locDiagIfaceHandle is NULL ");
    }
}

void LocIdlServiceLog::diagLogPowerEventInfo(uint8_t powerEvent, uint8_t serviceStatus) {
    size_t size = 0;
    diagServiceInfoStruct*  powerEventInfo = NULL;
    diagBuffSrc bufferSrc;
    size = sizeof(diagServiceInfoStruct)- sizeof(powerEventInfo->serviceReport) +
                   sizeof(diagPowerEventInfo);
    if (locDiagIfaceHandle) {
        powerEventInfo = (diagServiceInfoStruct*)locDiagIfaceHandle->logAlloc(
                 LOG_LOCATION_IDL_SERVICE_REPORT_C,
                 size, &bufferSrc);

        if (powerEventInfo) {
            powerEventInfo->serviceInfoType = POWER_EVENT_INFO;
            populateIdlDiagHeaderInfo(powerEventInfo->header);
            powerEventInfo->serviceReport.powerEvent.powerEventType = powerEvent;
            powerEventInfo->serviceReport.powerEvent.serviceStatus  = serviceStatus;
            locDiagIfaceHandle->logCommit(powerEventInfo, bufferSrc,
                   LOG_LOCATION_IDL_SERVICE_REPORT_VERSION, size);
        } else {
            LOC_LOGe(" logAlloc failed for Power Event Info !! ");
        }
    } else {
        LOC_LOGe(" locDiagIfaceHandle is NULL ");
    }
}
void LocIdlServiceLog::diagLogCapabilityInfo(string capabilityMask) {
    size_t size = 0;
    diagServiceInfoStruct*  capsInfo = NULL;
    diagBuffSrc bufferSrc;
    size = sizeof(diagServiceInfoStruct)- sizeof(capsInfo->serviceReport) +
                   sizeof(diagCapabilityReceivedInfo) +  capabilityMask.size() -1;
    if (locDiagIfaceHandle) {
        capsInfo = (diagServiceInfoStruct*)locDiagIfaceHandle->logAlloc(
                 LOG_LOCATION_IDL_SERVICE_REPORT_C,
                 size, &bufferSrc);

        if (capsInfo) {
            capsInfo->serviceInfoType = CAPS_EVENT_INFO;
            populateIdlDiagHeaderInfo(capsInfo->header);
            capsInfo->serviceReport.capabiltiyInfo.capabilityStringLength = capabilityMask.size();
            memcpy(&capsInfo->serviceReport.capabiltiyInfo.capabilitiesReceived,
                    capabilityMask.c_str(),
                    capsInfo->serviceReport.capabiltiyInfo.capabilityStringLength);
            locDiagIfaceHandle->logCommit(capsInfo, bufferSrc,
                   LOG_LOCATION_IDL_SERVICE_REPORT_VERSION, size);
        } else {
            LOC_LOGe(" logAlloc failed for Capability Info !! ");
        }
    } else {
        LOC_LOGe(" locDiagIfaceHandle is NULL ");
    }
}

LocIdlServiceLog::LocIdlServiceLog() {

}

LocIdlServiceLog::~LocIdlServiceLog() {

}
