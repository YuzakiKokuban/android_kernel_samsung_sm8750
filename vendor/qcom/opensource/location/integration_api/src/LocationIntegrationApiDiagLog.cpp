/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "LocationIntegrationApiDiagLog.h"
#include <log_util.h>

namespace location_integration {

void LocationIntegrationApiDiagLog::fillDiagMmfDataInfo(diagMapMatchedFeedbackData* out,
        const mapMatchedFeedbackData& inMmfData) {
        if (LOC_HAS_VALID_MMFD_UTC_TIME & inMmfData.validityMask) {
            out->validityMask |= DIAG_MMF_VALID_UTC_TIME;
            out->utcTimestampMs = inMmfData.utcTimestampMs;
        }
        if (LOC_HAS_VALID_MMFD_LAT_DIFF & inMmfData.validityMask) {
            out->validityMask |= DIAG_MMF_VALID_LAT_DIFF;
            out->mapMatchedLatitudeDifference = inMmfData.mapMatchedLatitudeDifference;
        }
        if (LOC_HAS_VALID_MMFD_LONG_DIFF & inMmfData.validityMask) {
            out->validityMask |= DIAG_MMF_VALID_LONG_DIFF;
            out->mapMatchedLongitudeDifference = inMmfData.mapMatchedLongitudeDifference;
        }
        if (LOC_HAS_VALID_MMFD_TUNNEL & inMmfData.validityMask) {
            out->validityMask |= DIAG_MMF_VALID_TUNNEL;
            out->isTunnel = inMmfData.isTunnel;
        }
        if (LOC_HAS_VALID_MMFD_BEARING & inMmfData.validityMask) {
            out->validityMask |= DIAG_MMF_VALID_BEARING;
            out->bearing = inMmfData.bearing;
        }
        if (LOC_HAS_VALID_MMFD_ALTITUDE & inMmfData.validityMask) {
            out->validityMask |= DIAG_MMF_VALID_ALTITUDE;
            out->altitude = inMmfData.altitude;
        }
        if (LOC_HAS_VALID_MMFD_HOR_ACC & inMmfData.validityMask) {
            out->validityMask |= DIAG_MMF_VALID_HOR_ACC;
            out->horizontalAccuracy = inMmfData.horizontalAccuracy;
        }
        if (LOC_HAS_VALID_MMFD_ALT_ACC & inMmfData.validityMask) {
            out->validityMask |= DIAG_MMF_VALID_ALT_ACC;
            out->altitudeAccuracy = inMmfData.altitudeAccuracy;
        }
        if (LOC_HAS_VALID_MMFD_BEARING_ACC & inMmfData.validityMask) {
            out->validityMask |= DIAG_MMF_VALID_BEARING_ACC;
            out->bearingAccuracy = inMmfData.bearingAccuracy;
        }
}


void LocationIntegrationApiDiagLog::diagLogMmfData(const mapMatchedFeedbackData& inMmfData) {
    if (mDiagIface) {
        size_t size = 0;
        diagMapMatchedFeedbackData*  mmfDataInfo = NULL;
        diagBuffSrc bufferSrc;
        size = sizeof(diagMapMatchedFeedbackData);
        mmfDataInfo = (diagMapMatchedFeedbackData*)mDiagIface->logAlloc(
                LOG_GNSS_LIA_API_MMF_REPORT_C, size, &bufferSrc);
        if (mmfDataInfo) {
            mmfDataInfo->version = LOG_CLIENT_MMF_DIAG_MSG_VERSION;
            fillDiagMmfDataInfo(mmfDataInfo, inMmfData);
            mDiagIface->logCommit(mmfDataInfo, bufferSrc,
                    LOG_GNSS_LIA_API_MMF_REPORT_C,
                    sizeof(diagMapMatchedFeedbackData));
        } else {
            LOC_LOGe(" Failed to allocate buffer for MMF data !! ");
        }
    }
}


LocationIntegrationApiDiagLog::LocationIntegrationApiDiagLog() {
    if (NULL == mDiagIface) {
        mDiagIface = loadLocDiagIfaceInterface();
        if (nullptr == mDiagIface) {
            LOC_LOGe("Failed to loadLocDiagIfaceInterface!!");
        }
    }
}

LocationIntegrationApiDiagLog::~LocationIntegrationApiDiagLog() {

}
} //  namespace location_integration {
