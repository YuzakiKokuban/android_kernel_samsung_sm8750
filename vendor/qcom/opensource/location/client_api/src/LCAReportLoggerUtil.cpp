/* Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
Changes from Qualcomm Innovation Center are provided under the following license:

Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.

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

#include "LCAReportLoggerUtil.h"
#include <loc_cfg.h>
#include <log_util.h>

namespace location_client {

LCAReportLoggerUtil::LCAReportLoggerUtil():
        mLogLocation(nullptr),
        mLogSv(nullptr),
        mLogNmea(nullptr),
        mLogMeas(nullptr),
        mLogDcReport(nullptr),
        mLogEph(nullptr),
        mLogGnssData(nullptr){

    int loadDiagIfaceLib = 1;
    const loc_param_s_type gps_conf_params[] = {
        {"LOC_DIAGIFACE_ENABLED", &loadDiagIfaceLib, nullptr, 'n'}
    };
    UTIL_READ_CONF(LOC_PATH_GPS_CONF, gps_conf_params);

    LOC_LOGi("Loc_DiagIface_enabled: %d", loadDiagIfaceLib);

    if (0 != loadDiagIfaceLib) {
        const char* libname = "liblocdiagiface.so";
        void* libHandle = nullptr;
        mLogLocation = (LogGnssLocation)dlGetSymFromLib(
                libHandle, libname, "LogGnssLocation");
        if (nullptr == mLogLocation) {
            LOC_LOGw("DiagIface mLogLocation is null");
        }
        mLogSv = (LogGnssSv)dlGetSymFromLib(
                libHandle, libname, "LogGnssSv");
        if (nullptr == mLogSv) {
            LOC_LOGw("DiagIface mLogSv is null");
        }
        mLogNmea = (LogGnssNmea)dlGetSymFromLib(
                libHandle, libname, "LogGnssNmea");
        if (nullptr == mLogNmea) {
            LOC_LOGw("DiagIface mLogNmea is null");
        }
        mLogMeas = (LogGnssMeas)dlGetSymFromLib(
                libHandle, libname, "LogGnssMeas");
        if (nullptr == mLogMeas) {
            LOC_LOGw("DiagIface mLogMeas is null");
        }
        mLogDcReport = (LogGnssDcReport)dlGetSymFromLib(
                libHandle, libname, "LogGnssDcReport");
        if (nullptr == mLogDcReport) {
            LOC_LOGw("DiagIface mLogDcReport is null");
        }
        mLogGeofenceBreach = (LogGeofenceBreach)dlGetSymFromLib(
                libHandle, libname, "LogGeofenceBreach");
        if (nullptr == mLogGeofenceBreach) {
            LOC_LOGw("DiagIface mLogGeofenceBreach is null");
        }
        mLogEph = (LogGnssEphemeris)dlGetSymFromLib(
                libHandle, libname, "LogGnssEphemeris");
        if (nullptr == mLogEph) {
            LOC_LOGw("DiagIface mLogEph is null");
        }
        mLogGnssData = (LogGnssData)dlGetSymFromLib(
                libHandle, libname, "LogGnssData");
        if (nullptr == mLogGnssData) {
            LOC_LOGw("DiagIface mLogGnssData is null");
        }
    }
}

void LCAReportLoggerUtil::log(const GnssLocation& gnssLocation,
                              const DiagLocationInfoExt& diagInfoExt) {
    if (mLogLocation != nullptr) {
        mLogLocation(gnssLocation, diagInfoExt);
    }
}

void LCAReportLoggerUtil::log(const std::vector<GnssSv>& gnssSvsVector) {
    if (mLogSv != nullptr) {
        mLogSv(gnssSvsVector);
    }
}

void LCAReportLoggerUtil::log(
        uint64_t timestamp, uint32_t length, const char* nmea, LocOutputEngineType engType) {
    if (mLogNmea != nullptr) {
        mLogNmea(timestamp, length, nmea, engType);
    }
}

void LCAReportLoggerUtil::log(const GnssMeasurements& gnssMeasurements) {
    if (mLogMeas != nullptr) {
        mLogMeas(gnssMeasurements);
    }
}

void LCAReportLoggerUtil::log(const GnssDcReport& gnssDcReport) {
    if (mLogDcReport != nullptr) {
        mLogDcReport(gnssDcReport);
    }
}
void LCAReportLoggerUtil::log(const GeofenceBreachNotification& breachNotif,
            const std::vector<Geofence> &geofences) {
    if (mLogGeofenceBreach != nullptr) {
        mLogGeofenceBreach(breachNotif, geofences);
    }
}

void LCAReportLoggerUtil::log(const GnssEphemeris& ephInfo) {
    if (mLogEph != nullptr) {
        mLogEph(ephInfo);
    }
}

void LCAReportLoggerUtil::log(const GnssData& gnssData) {
    if (mLogGnssData != nullptr) {
        mLogGnssData(gnssData);
    }
}
} // namespace loc_client
