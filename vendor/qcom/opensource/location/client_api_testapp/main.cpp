/* Copyright (c) 2019-2021 The Linux Foundation. All rights reserved.
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
 *     * Neither the name of The Linux Foundation, nor the names of its
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
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string>
#include <sstream>
#include <grp.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/capability.h>
#include <semaphore.h>
#include <getopt.h>
#include <loc_pla.h>
#include <loc_cfg.h>
#include <loc_misc_utils.h>
#include <thread>
#include <unordered_map>
#include <algorithm>

#include <LocationClientApi.h>
#include <LocationIntegrationApi.h>

using namespace location_client;
using namespace location_integration;
using std::vector;

static bool     outputEnabled = true;
static bool     detailedOutputEnabled = false;
// debug events counter
static uint32_t numFixes = 0;
static uint32_t numValidFixes = 0;
static uint32_t numGnssSvCb = 0;
static uint32_t numGnssNmeaCb = 0;
static uint32_t numEngineNmeaCb = 0;
static uint32_t numDataCb         = 0;
static uint32_t numGnssMeasurementsCb = 0;
static uint32_t numGnssEphemerisCb = 0;
static uint32_t  numNmeaSentencesCb = 0;

static LocationClientApi* pLcaClient = nullptr;
static location_integration::LocationIntegrationApi* pIntClient = nullptr;
static sem_t semCompleted;
static sem_t semGfCompleted;
static sem_t semBatchingCompleted;
static int fixCnt = 0x7fffffff;
static uint64_t autoTestStartTimeMs = 0;
static uint64_t autoTestStartGfTimeMs = 0;
static uint64_t autoTestStartBatchingTimeMs = 0;
static int autoTestTimeoutSec = 0x7FFFFFFF;
static uint32_t gtpFixCnt = 0;
static uint32_t singleShotFixCnt = 0;
vector<Geofence> sGeofences;

static char NMEA_PORT[] = "/dev/at_usb1";
static int ttyFd = -1;
static int routeToNMEAPort = 0;
enum ReportType {
    POSITION_REPORT    = 1 << 0,
    NMEA_REPORT        = 1 << 1,
    SV_REPORT          = 1 << 2,
    DATA_REPORT        = 1 << 3,
    MEAS_REPORT        = 1 << 4,
    NHZ_MEAS_REPORT    = 1 << 5,
    DC_REPORT          = 1 << 6,
    ENGINE_NMEA_REPORT = 1 << 7,
    EPHEMERIS_REPORT   = 1 << 8,
    NMEA_SENTENCES_REPORT = 1 << 9,
};

enum TrackingSessionType {
    NO_TRACKING = 0,
    SIMPLE_REPORT_TRACKING = 1,
    DETAILED_REPORT_TRACKING = 2,
    ENGINE_REPORT_TRACKING = 3,
};

#define DISABLE_REPORT_OUTPUT "disableReportOutput"
#define ENABLE_REPORT_OUTPUT  "enableReportOutput"
#define ENABLE_DETAILED_REPORT_OUTPUT  "enableDetailedReportOutput"
#define DISABLE_TUNC          "disableTunc"
#define ENABLE_TUNC           "enableTunc"
#define DISABLE_PACE          "disablePACE"
#define ENABLE_PACE           "enablePACE"
#define RESET_SV_CONFIG       "resetSVConfig"
#define CONFIG_SV             "configSV"
#define CONFIG_SECONDARY_BAND      "configSecondaryBand"
#define GET_SECONDARY_BAND_CONFIG  "getSecondaryBandConfig"
#define MULTI_CONFIG_SV            "multiConfigSV"
#define DELETE_ALL                 "deleteAll"
#define DELETE_AIDING_DATA         "deleteAidingData"
#define CONFIG_LEVER_ARM           "configLeverArm"
#define CONFIG_ROBUST_LOCATION     "configRobustLocation"
#define GET_ROBUST_LOCATION_CONFIG "getRobustLocationConfig"
#define CONFIG_MIN_GPS_WEEK        "configMinGpsWeek"
#define GET_MIN_GPS_WEEK           "getMinGpsWeek"
#define CONFIG_DR_ENGINE           "configDrEngine"
#define CONFIG_MIN_SV_ELEVATION    "configMinSvElevation"
#define GET_MIN_SV_ELEVATION       "getMinSvElevation"
#define CONFIG_ENGINE_RUN_STATE    "configEngineRunState"
#define CONFIG_ENGINE_INTEGRITY_RISK "configEngineIntegrityRisk"
#define SET_USER_CONSENT           "setUserConsentForTerrestrialPositioning"
#define GET_SINGLE_GTP_WWAN_FIX    "getSingleGtpWwanFix"
#define GET_MULTIPLE_GTP_WWAN_FIXES  "getMultipleGtpWwanFixes"
#define CANCEL_SINGLE_GTP_WWAN_FIX "cancelSingleGtpWwanFix"
#define CONFIG_NMEA_TYPES          "configOutputNmeaTypes"
#define GET_ENERGY_CONSUMED        "getEnergyConsumed"
#define INJECT_LOCATION            "injectLocation"
#define GET_SINGLE_FUSED_FIX       "getSingleFusedFix"
#define CANCEL_SINGLE_FUSED_FIX    "cancelSingleFusedFix"
#define CONFIG_XTRA_PARAMS         "configXtraParams"
#define GET_XTRA_STATUS             "getXtraStatus"
#define CONFIG_MERKLE_TREE          "configMerkleTree"
#define CONFIG_OSNMA_ENABLEMENT     "configOsnmaEnablement"
#define REGISTER_XTRA_STATUS_UPDATE "registerXtraUpdateStatus"
#define ENABLE_XTRA_ON_DEMAND_DOWNLOAD "enableXtraOnDemandDownload"
#define REGISTER_SIGNAL_TYPES_UPDATE "registerGnssSignalTypesUpdate"
#define ADD_GEOFENCES               "addGeofences"
#define PAUSE_GEOFENCES             "pauseGeofences"
#define RESUME_GEOFENCES            "resumeGeofences"
#define MODIFY_GEOFENCES            "modifyGeofences"
#define REMOVE_GEOFENCES            "removeGeofences"

static bool openPort(void)
{
    bool retVal = true;

    if (ttyFd == -1) {
        printf("opening NMEA port %s ", NMEA_PORT);
        ttyFd = open(NMEA_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
        if (ttyFd == -1) {
            /* Could not open the port. */
            printf("Unable to open %s \n", NMEA_PORT);
            retVal = false;
        } else {
            printf("openPort success ttyFd: %d\n", ttyFd);
        }
    }
    return retVal;
}

static bool sendNMEAToTty(const std::string& nmea)
{
    int n;
    char buffer[201] = { 0 };
    bool retVal = true;
    strlcpy(buffer, nmea.c_str(), sizeof(buffer));
    if (1 < nmea.length() && sizeof(buffer) > nmea.length()) {
        n = write(ttyFd, buffer, nmea.length());
        if (n < 0) {
            printf("write() of %d bytes failed!\n", n);
            retVal = false;
        } else if (0 == n) {
            printf("write() of %d bytes returned 0, errno:%d [%s]\n",
                nmea.length(), errno, strerror(errno));
            /* Sleep of 0.1 msec and reattempt to write*/
            usleep(100);
            n = write(ttyFd, buffer, nmea.length() - 1);
            if (n < 0) {
                printf("reattempt write() failed! errno:%d [%s] \n", errno, strerror(errno));
                retVal = false;
            } else if (0 == n) {
                printf("reattempt write() of %d bytes returned 0, errno:%d [%s]\n",
                    nmea.length(), errno, strerror(errno));
            }
        }
    } else {
        printf("Failed to write Len: %d %s \n", nmea.length(), nmea.c_str());
    }
    return true;
}

// debug utility
static uint64_t getTimestampMs() {
    struct timespec ts = {};
    clock_gettime(CLOCK_BOOTTIME, &ts);
    uint64_t msec =
            ((uint64_t)(ts.tv_sec)) * 1000ULL + ((uint64_t)(ts.tv_nsec)) / 1000000ULL;
    return msec;
}

static void resetCounters() {
    numFixes = 0;
    numValidFixes = 0;
    numGnssSvCb = 0;
    numGnssNmeaCb = 0;
    numEngineNmeaCb = 0;
    numDataCb = 0;
    numGnssMeasurementsCb = 0;
    numNmeaSentencesCb = 0;
}

static void cleanupAfterAutoStart() {
    if (pLcaClient) {
        printf("calling stopPosition and delete LCA client\n");
        pLcaClient->stopPositionSession();
        if (autoTestStartGfTimeMs != 0) {
            pLcaClient->removeGeofences(sGeofences);
        }
        if (autoTestStartBatchingTimeMs != 0) {
            pLcaClient->stopBatchingSession();
        }
        delete pLcaClient;
        pLcaClient = nullptr;
    }
    if (pIntClient) {
        printf("calling delete LIA client \n");
        delete pIntClient;
        pIntClient = nullptr;
    }
    // wait one second for stop request to reach hal daemon
    sleep(1);

    printf("\n\n summary: received %d fixes\n", numValidFixes);
}

/******************************************************************************
Callback functions
******************************************************************************/
static void onCapabilitiesCb(location_client::LocationCapabilitiesMask mask) {
    printf("<<< onCapabilitiesCb mask=0x%" PRIx64 "\n", mask);
    printf("<<< onCapabilitiesCb mask string=%s",
            LocationClientApi::capabilitiesToString(mask).c_str());
}

static void onResponseCb(location_client::LocationResponse response) {
    printf("<<< onResponseCb err=%u\n", response);
}

static void onLocationCb(const location_client::Location& location) {
    numFixes++;
    if (LOC_SESS_SUCCESS == location.sessionStatus) {
         numValidFixes++;
    }
    if (outputEnabled) {
        if (detailedOutputEnabled) {
            printf("<<< onLocationCb cnt=(%u/%u): %s\n", numValidFixes, numFixes,
                    location.toString().c_str());
        } else {
            printf("<<< onLocationCb cnt=(%u/%u): session status %d time=%" PRIu64
                   " mask=0x%x lat=%f lon=%f alt=%f horzacc=%f\n",
                   numValidFixes, numFixes,
                   location.sessionStatus,
                   location.timestamp,
                   location.flags,
                   location.latitude,
                   location.longitude,
                   location.altitude,
                   location.horizontalAccuracy);
        }
    }
    if (numValidFixes >= fixCnt) {
        printf("<<< onLocationCb: numValidFixes:%u exceeds fixCnt:%u\n", numValidFixes, fixCnt);
        sem_post(&semCompleted);
    }
}

static void onGtpResponseCb(location_client::LocationResponse response) {
    printf("<<< onGtpResponseCb err=%u\n", response);
    if (response != LOCATION_RESPONSE_SUCCESS) {
        sem_post(&semCompleted);
    }
}

static void onGtpLocationCb(const location_client::Location& location) {
    sem_post(&semCompleted);

    if (!outputEnabled) {
        return;
    }
    if (detailedOutputEnabled) {
        printf("<<< onGtpLocationCb: %s\n", location.toString().c_str());
    } else {
        printf("<<< onGtpLocationCb time=%" PRIu64" mask=0x%x lat=%f lon=%f alt=%f\n",
               location.timestamp,
               location.flags,
               location.latitude,
               location.longitude,
               location.altitude);
    }
}

static void onSingleShotResponseCb(location_client::LocationResponse response) {
    printf("<<< onSingleShotResponseCb err=%u\n", response);
    if (response != LOCATION_RESPONSE_SUCCESS && response != LOCATION_RESPONSE_TIMEOUT) {
        sem_post(&semCompleted);
    }
}

static void onSingleShotLocationCb(const location_client::Location& location) {
    sem_post(&semCompleted);

    if (!outputEnabled) {
        return;
    }
    if (detailedOutputEnabled) {
        printf("<<< onSingleShotLocationCb: %s\n", location.toString().c_str());
    } else {
        printf("<<< onSingleShotLocationCb time=%" PRIu64" mask=0x%x lat=%f lon=%f "
               "alt=%f accuracy=%f\n",
               location.timestamp,
               location.flags,
               location.latitude,
               location.longitude,
               location.altitude,
               location.horizontalAccuracy);
    }
}

static void onGnssLocationCb(const location_client::GnssLocation& location) {
    numFixes++;
    if (LOC_SESS_SUCCESS == location.sessionStatus) {
        numValidFixes++;
    }
    if (outputEnabled) {
        if (detailedOutputEnabled) {
            printf("<<< onGnssLocationCb cnt=(%u/%u): %s\n", numValidFixes, numFixes,
                    location.toString().c_str());
        } else {
            printf("<<< onGnssLocationCb cnt=(%u/%u): session status %d time=%" PRIu64
                   " mask=0x%x lat=%f lon=%f alt=%f\n",
                    numValidFixes, numFixes,
                    location.sessionStatus,
                    location.timestamp,
                    location.flags,
                    location.latitude,
                    location.longitude,
                    location.altitude);
        }
    }
    if (numValidFixes >= fixCnt) {
        printf("<<< onGnssLocationCb: numValidFixes:%u exceeds fixCnt:%u\n", numValidFixes, fixCnt);
        sem_post(&semCompleted);
    }
}

static void onEngLocationsCb(const std::vector<location_client::GnssLocation>& locations) {
    for (auto gnssLocation : locations) {
        if (outputEnabled) {
            if (detailedOutputEnabled) {
                printf("<<< onEngLocationsCb cnt=%u: %s\n", numFixes,
                       gnssLocation.toString().c_str());
            } else {
                printf("<<< onEngLocationsCb cnt=%u: time=%" PRIu64" mask=0x%x lat=%f lon=%f \n"
                       "alt=%f info mask=0x%" PRIx64 ", nav solution maks = 0x%x, eng type %d, "
                       "eng mask 0x%x, session status %d\n",
                       numFixes,
                       gnssLocation.timestamp,
                       gnssLocation.flags,
                       gnssLocation.latitude,
                       gnssLocation.longitude,
                       gnssLocation.altitude,
                       gnssLocation.gnssInfoFlags,
                       gnssLocation.navSolutionMask,
                       gnssLocation.locOutputEngType,
                       gnssLocation.locOutputEngMask,
                       gnssLocation.sessionStatus);
            }
        }
        if (gnssLocation.sessionStatus == LOC_SESS_SUCCESS &&
            gnssLocation.locOutputEngType == LOC_OUTPUT_ENGINE_FUSED) {
            numValidFixes++;
        }
    }
    printf("<<< onEngLocationsCb cnt:(%u/%u)", numValidFixes, numFixes);

    if (numValidFixes >= fixCnt) {
        printf("<<< onEngLocationsCb: numValidFixes:%u exceeds fixCnt:%u\n", numValidFixes, fixCnt);
        sem_post(&semCompleted);
    }
}

static void onBatchingCb(const std::vector<Location>& locations,
                         BatchingStatus batchStatus) {
    printf("<<< onBatchingCb, batching status: %d, pos cnt %d", batchStatus, locations.size());
    for (Location location : locations) {
        printf("<<< onBatchingCb time=%" PRIu64" mask=0x%x lat=%f lon=%f alt=%f\n",
               location.timestamp,
               location.flags,
               location.latitude,
               location.longitude,
               location.altitude);
    }
}

static void onGeofenceBreachCb( const vector<Geofence>& geofences, Location location,
        GeofenceBreachTypeMask type, uint64_t timestamp) {
    printf("<<< onGeofenceBreachCb, breach type: %d, gf cnt: %d, timestamp: %" PRIu64, type,
           geofences.size(), timestamp);
    for (Geofence gf : geofences) {
        printf("<<< onGeofenceBreachCb, lat=%f lon=%f rad=%f, responsiveness: %u, dwellTime: %u\n",
               gf.getLatitude(), gf.getLongitude(), gf.getRadius(), gf.getResponsiveness(),
               gf.getDwellTime());
    }
}
static void onCollectiveResponseCb(vector<std::pair<Geofence, LocationResponse>>& responses) {
    printf("<<< onCollectiveResponseCb, geofence cnt: %d\n", responses.size());
    for (std::pair<Geofence, LocationResponse> pair : responses) {
        printf("<<< onCollectiveResponseCb, lat=%f lon=%f rad=%f, breachType: %d, "
                "responsiveness: %u, dwellTime: %u, response: %d\n",
               pair.first.getLatitude(), pair.first.getLongitude(), pair.first.getRadius(),
               pair.first.getBreachType(), pair.first.getResponsiveness(),
               pair.first.getDwellTime(), pair.second);
    }
}
static void onGnssSvCb(const std::vector<location_client::GnssSv>& gnssSvs) {
    numGnssSvCb++;

    // we are in auto-test mode, check whether we have completed the test
    if (autoTestStartTimeMs != 0) {
        uint64_t nowMs = getTimestampMs();
        if (nowMs-autoTestStartTimeMs >= autoTestTimeoutSec * 1000) {
            printf("complete due to run time exceeded %d sec\n", autoTestTimeoutSec);
            sem_post(&semCompleted);
        }
    }

    if (!outputEnabled) {
        return;
    }

    if (detailedOutputEnabled) {
        printf("<<< onGnssSvCb cnt=%d\n", numGnssSvCb);
        for (auto sv : gnssSvs) {
            printf("<<< %s\n", sv.toString().c_str());
        }
    } else {
        std::stringstream ss;
        ss << "<<< onGnssSvCb c=" << numGnssSvCb << " s=" << gnssSvs.size();
        for (auto sv : gnssSvs) {
            ss << " " << sv.type << ":" << sv.svId << "/" << (uint32_t)sv.cN0Dbhz;
        }
        printf("%s\n", ss.str().c_str());
    }
}

static void onGnssNmeaCb(uint64_t timestamp, const std::string& nmea) {
    numGnssNmeaCb++;
    if (!outputEnabled) {
        return;
    }
    printf("<<< onGnssNmeaCb cnt=%u time=%" PRIu64" nmea=%s",
            numGnssNmeaCb, timestamp, nmea.c_str());
    if (routeToNMEAPort && openPort()) {
                sendNMEAToTty(nmea);
    }
}

static void onEngineNmeaCb(LocOutputEngineType engType,
                           uint64_t timestamp,
                           const std::string& nmea) {
    numEngineNmeaCb++;
    if (outputEnabled) {
        printf("<<< onEngineNmeaCb cnt=%u engine type=%u time=%" PRIu64" nmea=%s",
            numEngineNmeaCb, engType, timestamp, nmea.c_str());
    }
    if (routeToNMEAPort && openPort()) {
                sendNMEAToTty(nmea);
    }
}

static void onNmeaSentencesCb(LocOutputEngineType engType,
                           uint64_t timestamp,
                           const std::string& nmea) {
    numNmeaSentencesCb++;
    if (outputEnabled) {
        printf("<<< onNmeaSentencesCb cnt=%u engine type=%u time=%" PRIu64" nmea=%s \n",
            numNmeaSentencesCb, engType, timestamp, nmea.c_str());
    }
    if (routeToNMEAPort && openPort()) {
                sendNMEAToTty(nmea);
    }
}

static void onGnssDataCb(const location_client::GnssData& gnssData) {
    numDataCb++;
    if (!outputEnabled) {
        return;
    }
    if (detailedOutputEnabled) {
        printf("<<< gnssDataCb cnt=%u: %s\n", numDataCb, gnssData.toString().c_str());
    } else {
        printf("<<< gnssDataCb cnt=%u\n", numDataCb);
    }
}

static void onGnssMeasurementsCb(const location_client::GnssMeasurements& gnssMeasurements) {
    numGnssMeasurementsCb++;
    if (!outputEnabled) {
        return;
    }

    if (detailedOutputEnabled) {
        printf("<<< onGnssMeasurementsCb cnt=%u, %s ",numGnssMeasurementsCb,
               gnssMeasurements.toString().c_str());
    } else {
        printf("<<< onGnssMeasurementsCb cnt=%u, num of meas %d, nHz %d\n",
               numGnssMeasurementsCb, gnssMeasurements.measurements.size(),
               gnssMeasurements.isNhz);
    }
}

static void onGnssDcReportCb(const location_client::GnssDcReport & dcReport) {
    if (detailedOutputEnabled) {
        printf("<<< DC report %s\n", dcReport.toString().c_str());
    } else {
        printf("DC report type %d, valid bits cnt %d, data byte cnt %d\n",
               dcReport.dcReportType, dcReport.numValidBits, dcReport.dcReportData.size());
    }
}

static void onConfigResponseCb(location_integration::LocConfigTypeEnum    requestType,
                               location_integration::LocIntegrationResponse response) {
    printf("<<< onConfigResponseCb, type %d, err %d\n", requestType, response);
}

static void onGetRobustLocationConfigCb(RobustLocationConfig robustLocationConfig) {
    printf("<<< onGetRobustLocationConfigCb, valid flags 0x%x, enabled %d, enabledForE911 %d, "
           "version (major %u, minor %u)\n",
           robustLocationConfig.validMask, robustLocationConfig.enabled,
           robustLocationConfig.enabledForE911, robustLocationConfig.version.major,
           robustLocationConfig.version.minor);
}

static void onGetMinGpsWeekCb(uint16_t minGpsWeek) {
    printf("<<< onGetMinGpsWeekCb, minGpsWeek %d\n", minGpsWeek);
}

static void onGetMinSvElevationCb(uint8_t minSvElevation) {
    printf("<<< onGetMinSvElevationCb, minSvElevationAngleSetting: %d\n", minSvElevation);
}

static void onGetSecondaryBandConfigCb(const ConstellationSet& secondaryBandDisablementSet) {
    for (GnssConstellationType disabledSecondaryBand : secondaryBandDisablementSet) {
        printf("<<< disabled secondary band for constellation %d\n", disabledSecondaryBand);
    }
}

static void onGetGnssEnergyConsumedCb(const GnssEnergyConsumedInfo& gnssEneryConsumed) {
    printf("<<< onGetGnssEnergyConsumedCb energy: (valid=%d, value=%" PRIu64")",
            (gnssEneryConsumed.flags & ENERGY_CONSUMED_SINCE_FIRST_BOOT_BIT) != 0,
            gnssEneryConsumed.totalEnergyConsumedSinceFirstBoot);
}

static void onGetXtraStatusCb(XtraStatusUpdateTrigger updateTrigger, const XtraStatus& xtraStatus) {
    printf("<<< onXtraStatusCb, update trigger %d, enable %d, status %d, valid hours %d\n",
           updateTrigger, xtraStatus.featureEnabled, xtraStatus.xtraDataStatus,
           xtraStatus.xtraValidForHours);
}

static void onGnssSignalTypesCb(GnssSignalTypeMask signalType) {
    printf("<<< onGnssSignalTypesCb, supported signalType mask %x \n", signalType);
}

static void onGnssEphemerisCb(const location_client::GnssEphemeris& ephInfo) {
    numGnssEphemerisCb++;
    printf("<<< onGnssEphemerisCb  cnt=%u Constellation=%d \n", numGnssEphemerisCb,
            ephInfo.gnssConstellation);
}

static void printHelp() {
    printf("\n************* options *************\n");
    printf("e reprottype tbf: Concurrent engine report session with 100 ms interval\n");
    printf("g reporttype tbf engmask: Gnss report session with 100 ms interval\n");
    printf("u: Update a session with 2000 ms interval\n");
    printf("m: Interleaving fix session with 1000 and 2000 ms interval, change every 3 seconds\n");
    printf("s: Stop a session \n");
    printf("q: Quit\n");
    printf("r: delete client\n");
    printf("%s: supress output from various reports\n", DISABLE_REPORT_OUTPUT);
    printf("%s: enable output from various reports\n", ENABLE_REPORT_OUTPUT);
    printf("%s: enable detailed output from various reports\n", ENABLE_DETAILED_REPORT_OUTPUT);
    printf("%s tuncThreshold energyBudget: enable tunc\n", ENABLE_TUNC);
    printf("%s: disable tunc\n", DISABLE_TUNC);
    printf("%s: enable PACE\n", ENABLE_PACE);
    printf("%s: disable PACE\n", DISABLE_PACE);
    printf("%s: reset sv config to default\n", RESET_SV_CONFIG);
    printf("%s: configure sv \n", CONFIG_SV);
    printf("%s: configure secondary band\n", CONFIG_SECONDARY_BAND);
    printf("%s: get secondary band configure \n", GET_SECONDARY_BAND_CONFIG);
    printf("%s: mulitple config SV \n", MULTI_CONFIG_SV);
    printf("%s: delete all aiding data\n", DELETE_ALL);
    printf("%s: delete ephemeris/calibration data\n", DELETE_AIDING_DATA);
    printf("%s: config lever arm\n", CONFIG_LEVER_ARM);
    printf("%s: config robust location\n", CONFIG_ROBUST_LOCATION);
    printf("%s: get robust location config\n", GET_ROBUST_LOCATION_CONFIG);
    printf("%s: set min gps week\n", CONFIG_MIN_GPS_WEEK);
    printf("%s: get min gps week\n", GET_MIN_GPS_WEEK);
    printf("%s: config DR engine\n", CONFIG_DR_ENGINE);
    printf("%s: set min sv elevation angle\n", CONFIG_MIN_SV_ELEVATION);
    printf("%s: get min sv elevation angle\n", GET_MIN_SV_ELEVATION);
    printf("%s: config engine run state\n", CONFIG_ENGINE_RUN_STATE);
    printf("%s: set user consent for terrestrial positioning 0/1\n", SET_USER_CONSENT);
    printf("%s: get single shot wwan fix, timeout_msec 0.0 1\n", GET_SINGLE_GTP_WWAN_FIX);
    printf("%s: get multiple wan fix: numOfFixes tbfMsec timeout_msec 0.0 1\n",
           GET_MULTIPLE_GTP_WWAN_FIXES);
    printf("%s: config nmea types \n", CONFIG_NMEA_TYPES);
    printf("%s: config engine integrity risk \n", CONFIG_ENGINE_INTEGRITY_RISK);
    printf("%s: get gnss energy consumed \n", GET_ENERGY_CONSUMED);
    printf("%s: inject location \n", INJECT_LOCATION);
    printf("%s: get single shot fix with qos and timeout \n", GET_SINGLE_FUSED_FIX );
    printf("%s: cancle single shot fix \n", CANCEL_SINGLE_FUSED_FIX );
    printf("%s: config xtra params \n", CONFIG_XTRA_PARAMS);
    printf("%s: get xtra status \n", GET_XTRA_STATUS);
    printf("%s: config merkle tree \n", CONFIG_MERKLE_TREE);
    printf("%s: config osnam enablement \n", CONFIG_OSNMA_ENABLEMENT);
    printf("%s: register xtra status update \n", REGISTER_XTRA_STATUS_UPDATE);
    printf("%s: enable xtra on demand download \n", ENABLE_XTRA_ON_DEMAND_DOWNLOAD);
    printf("%s: register GNSS signal types update \n", REGISTER_SIGNAL_TYPES_UPDATE);
    printf("%s: add geofences with lat/lon/radius/breachtype/responsiveness/dwelltime\n",
            ADD_GEOFENCES);
    printf("%s: pause geofences with indexes\n",  PAUSE_GEOFENCES );
    printf("%s: resume geofences with indexes\n", RESUME_GEOFENCES );
    printf("%s: modify geofences with index/breachtype/responsiveness/dwelltime\n",
            MODIFY_GEOFENCES );
    printf("%s: remove geofences with indexes\n", REMOVE_GEOFENCES );
}

void setRequiredPermToRunAsLocClient() {
#ifdef USE_GLIB
    if (0 == getuid()) {
        char groupNames[LOC_MAX_PARAM_NAME] = "diag locclient";
        printf("group ids: diag locclient\n");

        gid_t groupIds[LOC_PROCESS_MAX_NUM_GROUPS] = {};
        char *splitGrpString[LOC_PROCESS_MAX_NUM_GROUPS];
        int numGrps = loc_util_split_string(groupNames, splitGrpString,
                LOC_PROCESS_MAX_NUM_GROUPS, ' ');

        int numGrpIds=0;
        for (int i = 0; i < numGrps; i++) {
            struct group* grp = getgrnam(splitGrpString[i]);
            if (grp) {
                groupIds[numGrpIds] = grp->gr_gid;
                printf("Group %s = %d\n", splitGrpString[i], groupIds[numGrpIds]);
                numGrpIds++;
            }
        }
        if (0 != numGrpIds) {
            if (-1 == setgroups(numGrpIds, groupIds)) {
                printf("Error: setgroups failed %s", strerror(errno));
            }
        }
        // Set the group id first and then set the effective userid, to locclient.
        if (-1 == setgid(GID_LOCCLIENT)) {
            printf("Error: setgid failed. %s", strerror(errno));
        }
        // Set user id to locclient
        if (-1 == setuid(UID_LOCCLIENT)) {
            printf("Error: setuid failed. %s", strerror(errno));
        }

        // Set capabilities
        struct __user_cap_header_struct cap_hdr = {};
        cap_hdr.version = _LINUX_CAPABILITY_VERSION;
        cap_hdr.pid = getpid();
        if (prctl(PR_SET_KEEPCAPS, 1) < 0) {
            printf("Error: prctl failed. %s", strerror(errno));
        }

        // Set access to CAP_NET_BIND_SERVICE
        struct __user_cap_data_struct cap_data = {};
        cap_data.permitted = (1 << CAP_NET_BIND_SERVICE);
        cap_data.effective = cap_data.permitted;
        printf("cap_data.permitted: %d", (int)cap_data.permitted);
        if (capset(&cap_hdr, &cap_data)) {
            printf("Error: capset failed. %s", strerror(errno));
        }
    } else {
        int userId = getuid();
        if (UID_LOCCLIENT == userId) {
            printf("Test app started as locclient user: %d\n", userId);
        } else {
            printf("ERROR! Test app started as user: %d\n", userId);
            printf("Start the test app from shell running as root OR\n");
            printf("Start the test app as locclient user from shell\n");
            exit(0);
        }
    }
#endif// USE_GLIB
}

void parseSVConfig(char* buf, bool & resetConstellation,
                   LocConfigBlacklistedSvIdList &svList) {
    static char *save = nullptr;
    char* token = strtok_r(buf, " ", &save); // skip first one of "configSV"
    token = strtok_r(NULL, " ", &save);
    if (token == nullptr) {
        printf("empty sv blacklist\n");
        return;
    }

    if (strncmp(token, "reset", strlen("reset")) == 0) {
        resetConstellation = true;
        printf("reset sv constellation\n");
        return;
    }

    while (token != nullptr) {
        GnssSvIdInfo svIdInfo = {};
        svIdInfo.constellation = (GnssConstellationType) atoi(token);
        token = strtok_r(NULL, " ", &save);
        if (token == NULL) {
            break;
        }
        svIdInfo.svId = atoi(token);
        svList.push_back(svIdInfo);
        token = strtok_r(NULL, " ", &save);
    }

    printf("parse sv config:\n");
    for (GnssSvIdInfo it : svList) {
        printf("\t\tblacklisted SV: %d, sv id %d\n", (int) it.constellation, it.svId);
    }
}

void parseSecondaryBandConfig(char* buf, bool &nullSecondaryBandConfig,
                              ConstellationSet& secondaryBandDisablementSet) {
    static char *save = nullptr;
    char* token = strtok_r(buf, " ", &save); // skip first one of "configSeconaryBand"
    token = strtok_r(NULL, " ", &save);

    if (token == nullptr) {
        printf("empty secondary band disablement set\n");
        return;
    }

    if (strncmp(token, "null", strlen("null")) == 0) {
        nullSecondaryBandConfig = true;
        printf("null secondary band disablement set\n");
        return;
    }

    while (token != nullptr) {
        GnssConstellationType secondaryBandDisabled = (GnssConstellationType) atoi(token);
        secondaryBandDisablementSet.emplace(secondaryBandDisabled);
        token = strtok_r(NULL, " ", &save);
    }

    printf("\t\tnull SecondaryBandConfig %d\n", nullSecondaryBandConfig);
    for (GnssConstellationType disabledSecondaryBand : secondaryBandDisablementSet) {
        printf("\t\tdisabled secondary constellation %d\n", disabledSecondaryBand);
    }
}

void parseLeverArm (char* buf, LeverArmParamsMap &leverArmMap) {
    static char *save = nullptr;
    char* token = strtok_r(buf, " ", &save); // skip first one of "configLeverArm"
    token = strtok_r(NULL, " ", &save);
    while (token != nullptr) {
        LeverArmParams leverArm = {};
        LeverArmType type = (LeverArmType) 0;
        if (strcmp(token, "vrp") == 0) {
            type = location_integration::LEVER_ARM_TYPE_GNSS_TO_VRP;
        } else if (strcmp(token, "drimu") == 0) {
            type = location_integration::LEVER_ARM_TYPE_DR_IMU_TO_GNSS;
        } else if (strcmp(token, "veppimu") == 0) {
            type = location_integration::LEVER_ARM_TYPE_VEPP_IMU_TO_GNSS;
        } else {
            break;
        }
        token = strtok_r(NULL, " ", &save);
        if (token == NULL) {
            break;
        }
        leverArm.forwardOffsetMeters = atof(token);

        token = strtok_r(NULL, " ", &save);
        if (token == NULL) {
            break;
        }
        leverArm.sidewaysOffsetMeters = atof(token);

        token = strtok_r(NULL, " ", &save);
        if (token == NULL) {
            break;
        }
        leverArm.upOffsetMeters = atof(token);

        printf("type %d, %f %f %f\n", type, leverArm.forwardOffsetMeters,
               leverArm.sidewaysOffsetMeters, leverArm.upOffsetMeters);

        leverArmMap.emplace(type, leverArm);
        token = strtok_r(NULL, " ", &save);
    }
}

void parseDreConfig (char* buf, DeadReckoningEngineConfig& dreConfig) {
    static char *save = nullptr;
    char* token = strtok_r(buf, " ", &save); // skip first one of "configDrEngine"
    printf("Usage: configDrEngine b2s roll pitch yaw unc speed scaleFactor scaleFactorUnc "
           "gyro scaleFactor ScaleFactorUnc\n");

    uint32_t validMask = 0;
    do {
        token = strtok_r(NULL, " ", &save);
        if (token == NULL) {
            if (validMask == 0) {
                 printf("missing b2s/speed/gyro");
            }
            break;
        }

        if (strncmp(token, "b2s", strlen("b2s"))==0) {
            token = strtok_r(NULL, " ", &save); // skip the token of "b2s"
            if (token == NULL) {
                printf("missing roll offset\n");
                break;
            }
            dreConfig.bodyToSensorMountParams.rollOffset = atof(token);

            token = strtok_r(NULL, " ", &save);
            if (token == NULL) {
                printf("missing pitch offset\n");
                break;
            }
            dreConfig.bodyToSensorMountParams.pitchOffset = atof(token);

            token = strtok_r(NULL, " ", &save);
            if (token == NULL) {
                printf("missing yaw offset\n");
                break;
            }
            dreConfig.bodyToSensorMountParams.yawOffset = atof(token);

            token = strtok_r(NULL, " ", &save);
            if (token == NULL) {
                printf("missing offset uncertainty\n");
                break;
            }
            dreConfig.bodyToSensorMountParams.offsetUnc = atof(token);

            validMask |= BODY_TO_SENSOR_MOUNT_PARAMS_VALID;
        } else if (strncmp(token, "speed", strlen("speed"))==0) {
            token = strtok_r(NULL, " ", &save);
            if (token == NULL) {
                printf("missing speed scale factor\n");
                break;
            }
            dreConfig.vehicleSpeedScaleFactor = atof(token);
            validMask |= VEHICLE_SPEED_SCALE_FACTOR_VALID;

            token = strtok_r(NULL, " ", &save);
            if (token == NULL) {
                printf("missing speed scale factor uncertainty\n");
                break;
            }
            dreConfig.vehicleSpeedScaleFactorUnc = atof(token);
            validMask |= VEHICLE_SPEED_SCALE_FACTOR_UNC_VALID;
        } else if (strncmp(token, "gyro", strlen("gyro"))==0) {
            token = strtok_r(NULL, " ", &save);
            if (token == NULL) {
                printf("missing gyro scale factor\n");
                break;
            }
            dreConfig.gyroScaleFactor = atof(token);
            validMask |= GYRO_SCALE_FACTOR_VALID;

            token = strtok_r(NULL, " ", &save);
            if (token == NULL) {
                printf("missing gyro scale factor uncertainty\n");
                break;
            }
            dreConfig.gyroScaleFactorUnc = atof(token);
            validMask |= GYRO_SCALE_FACTOR_UNC_VALID;
        } else {
            printf("unknown token %s\n", token);
        }
    } while (1);

    dreConfig.validMask = (DeadReckoningEngineConfigValidMask)validMask;
}

// This function retrieves location info from the buf
// format is: lat lon alt horizontalaccuracy verticalaccuracy timestamp timestampunc
void parseLocation(char* buf, location_client::Location& location) {
    static char *save = nullptr;
    char* token = strtok_r(buf, " ", &save); // skip header
    uint32_t flags = 0;

    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        location.latitude = atof(token);
    }
    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        location.longitude = atof(token);
    }
    flags |= LOCATION_HAS_LAT_LONG_BIT;

    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        location.altitude = atof(token);
    }
    flags |= LOCATION_HAS_ALTITUDE_BIT;

    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        location.horizontalAccuracy = atof(token);
    }
    flags |= LOCATION_HAS_ACCURACY_BIT;

    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        location.verticalAccuracy = atof(token);
    }
    flags |= LOCATION_HAS_VERTICAL_ACCURACY_BIT;

    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        location.timestamp = (uint64_t) atof(token);
    }
    flags |= LOCATION_HAS_TIMESTAMP_BIT;

    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        location.timeUncMs = atof(token);
    }
    flags |= LOCATION_HAS_TIME_UNC_BIT;
    location.flags = (LocationFlagsMask) flags;
}

void parseXtraConfig(char* buf, bool &xtraEnabled, XtraConfigParams& oemConfig) {
    static char *save = nullptr;
    char* token = strtok_r(buf, " ", &save); // skip header

    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        xtraEnabled = atoi(token);
    }

    if (xtraEnabled) {
        token = strtok_r(NULL, " ", &save);
        if (token != NULL) {
            oemConfig.xtraDownloadIntervalMinute = atoi(token);
        }

        token = strtok_r(NULL, " ", &save);
        if (token != NULL) {
            oemConfig.xtraDownloadTimeoutSec = atoi(token);
        }

        token = strtok_r(NULL, " ", &save);
        if (token != NULL) {
            oemConfig.xtraDownloadRetryIntervalMinute = atoi(token);
        }

        token = strtok_r(NULL, " ", &save);
        if (token != NULL) {
            oemConfig.xtraDownloadRetryAttempts = atoi(token);
        }

        token = strtok_r(NULL, " ", &save);
        if (token != NULL) {
            oemConfig.xtraCaPath = std::string(token);
        }

        uint32_t serverCnt = 0;
        token = strtok_r(NULL, " ", &save);
        if (token != NULL) {
            serverCnt = atoi(token);
        }

        for (int i = 0; i < serverCnt; i++) {
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                oemConfig.xtraServerURLs[i] = std::string(token);
            }
        }

        serverCnt = 0;
        token = strtok_r(NULL, " ", &save);
        if (token != NULL) {
            serverCnt = atoi(token);
        }

        for (int i = 0; i < serverCnt; i++) {
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                oemConfig.ntpServerURLs[i] = std::string(token);
            }
        }

        token = strtok_r(NULL, " ", &save);
        if (token != NULL) {
            oemConfig.xtraIntegrityDownloadEnable = ((atoi(token) == 0) ? false : true);
        }

        token = strtok_r(NULL, " ", &save);
        if (token != NULL) {
            oemConfig.xtraIntegrityDownloadIntervalMinute = atoi(token);
        }

        token = strtok_r(NULL, " ", &save);
        if (token != NULL) {
            oemConfig.xtraDaemonDebugLogLevel = (DebugLogLevel) atoi(token);
        }

        token = strtok_r(NULL, " ", &save);
        if (token != NULL) {
            oemConfig.ntsKeServerURL = std::string(token);
        }

        token = strtok_r(NULL, " ", &save);
        if (token != NULL) {
            oemConfig.xtraDaemonDiagLoggingStatus = atoi(token);
        }

        printf ("xtra config: enabled %d, %d %d %d %d ca path: %s, "
                "xtra url: %s %s %s, ntp url: %s %s %s, nts ke server url : %s\n",
                xtraEnabled, oemConfig.xtraDownloadIntervalMinute,
                oemConfig.xtraDownloadTimeoutSec,
                oemConfig.xtraDownloadRetryIntervalMinute,
                oemConfig.xtraDownloadRetryAttempts,
                oemConfig.xtraCaPath.c_str(),
                oemConfig.xtraServerURLs[0].c_str(), oemConfig.xtraServerURLs[1].c_str(),
                oemConfig.xtraServerURLs[2].c_str(), oemConfig.ntpServerURLs[0].c_str(),
                oemConfig.ntpServerURLs[1].c_str(), oemConfig.ntpServerURLs[2].c_str(),
                oemConfig.ntsKeServerURL.c_str());
        printf("integerity download %d %d\n", oemConfig.xtraIntegrityDownloadEnable,
               oemConfig.xtraIntegrityDownloadIntervalMinute);
        printf("DebugLogLevel %d DiagLoggingStatus%d\n", oemConfig.xtraDaemonDebugLogLevel,
               oemConfig.xtraDaemonDiagLoggingStatus);

    } else {
        printf("xtra disabled \n");
    }
}

void getGtpWwanFixes (bool multipleFixes, char* buf) {
    uint32_t gtpFixTbfMsec  = 0;
    uint32_t gtpTimeoutMsec = 0;
    float    gtpHorQoS      = 0.0;
    uint32_t gtpTechMask    = 0x0;

    static char *save = nullptr;
    char* token = strtok_r(buf, " ", &save); // skip first token
    if (multipleFixes == true) {
        // get fix cnt
        token = strtok_r(NULL, " ", &save);
        if (token != NULL) {
            gtpFixCnt = atoi(token);
        }
        if (gtpFixCnt == 0) {
            return;
        }

        // get tbf
        token = strtok_r(NULL, " ", &save);
        if (token != NULL) {
            gtpFixTbfMsec = atoi(token);
        }
    } else {
        gtpFixCnt = 1;
    }

    // get timeout
    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        gtpTimeoutMsec = atoi(token);
    }
    // get qos
    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        gtpHorQoS = atof(token);
    }
    // tech mask
    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        gtpTechMask = atoi(token);
    }
    printf("number of fixes %d, timeout msec %d, horQoS %f, techMask 0x%x\n",
           gtpFixCnt, gtpTimeoutMsec, gtpHorQoS, gtpTechMask);

    if (!pLcaClient) {
        pLcaClient = new LocationClientApi(onCapabilitiesCb);
    }

    if (pLcaClient) {
        while (gtpFixCnt > 0) {
            pLcaClient->getSingleTerrestrialPosition(gtpTimeoutMsec,
                                                     (TerrestrialTechnologyMask) gtpTechMask,
                                                     gtpHorQoS, onGtpLocationCb, onGtpResponseCb);
            sem_wait(&semCompleted);
            gtpFixCnt--;
            sleep(gtpFixTbfMsec/1000);
        }
    }
}

static void setupGnssReportCbs(uint32_t reportType, GnssReportCbs& reportcbs) {
    if (reportType & POSITION_REPORT) {
        reportcbs.gnssLocationCallback = GnssLocationCb(onGnssLocationCb);
    }
    if (reportType & NMEA_REPORT) {
        reportcbs.gnssNmeaCallback = GnssNmeaCb(onGnssNmeaCb);
    }
    if (reportType & SV_REPORT) {
        reportcbs.gnssSvCallback = GnssSvCb(onGnssSvCb);
    }
    if (reportType & DATA_REPORT) {
        reportcbs.gnssDataCallback = GnssDataCb(onGnssDataCb);
    }
    if (reportType & MEAS_REPORT) {
        reportcbs.gnssMeasurementsCallback = GnssMeasurementsCb(onGnssMeasurementsCb);
    }
    if (reportType & NHZ_MEAS_REPORT) {
        reportcbs.gnssNHzMeasurementsCallback = GnssMeasurementsCb(onGnssMeasurementsCb);
    }
    if (reportType & DC_REPORT) {
        reportcbs.gnssDcReportCallback = GnssDcReportCb(onGnssDcReportCb);
    }
    if (reportType & EPHEMERIS_REPORT) {
        reportcbs.gnssEphReportCallback = GnssEphReportCb(onGnssEphemerisCb);
    }
    if (reportType & NMEA_SENTENCES_REPORT) {
        reportcbs.nmeaSentencesCallback = NmeaSentencesCb(onNmeaSentencesCb);
    }
}

static void setupEngineReportCbs(uint32_t reportType, EngineReportCbs& reportcbs) {
    if (reportType & POSITION_REPORT) {
        reportcbs.engLocationsCallback = EngineLocationsCb(onEngLocationsCb);
    }
    if (reportType & NMEA_REPORT) {
        reportcbs.gnssNmeaCallback = GnssNmeaCb(onGnssNmeaCb);
    }
    if (reportType & SV_REPORT) {
        reportcbs.gnssSvCallback = GnssSvCb(onGnssSvCb);
    }
    if (reportType & DATA_REPORT) {
        reportcbs.gnssDataCallback = GnssDataCb(onGnssDataCb);
    }
    if (reportType & MEAS_REPORT) {
        reportcbs.gnssMeasurementsCallback = GnssMeasurementsCb(onGnssMeasurementsCb);
    }
    if (reportType & NHZ_MEAS_REPORT) {
        reportcbs.gnssNHzMeasurementsCallback = GnssMeasurementsCb(onGnssMeasurementsCb);
    }
    if (reportType & DC_REPORT) {
        reportcbs.gnssDcReportCallback = GnssDcReportCb(onGnssDcReportCb);
    }
    if (reportType & ENGINE_NMEA_REPORT) {
        reportcbs.engineNmeaCallback = EngineNmeaCb(onEngineNmeaCb);
    }
    if (reportType & EPHEMERIS_REPORT) {
        reportcbs.gnssEphReportCallback = GnssEphReportCb(onGnssEphemerisCb);
    }
    if (reportType & NMEA_SENTENCES_REPORT) {
        reportcbs.nmeaSentencesCallback = NmeaSentencesCb(onNmeaSentencesCb);
    }
}

void getMultipleFusedFixes(uint32_t timeoutMsec, float horQoS,
                           uint32_t fixTbfMsec) {
    while (singleShotFixCnt > 0) {
        printf("fix cnt needed: %d, sleep %d seconds",
               singleShotFixCnt, fixTbfMsec/1000);
        pLcaClient->getSinglePosition(timeoutMsec, horQoS,
                                      onSingleShotLocationCb,
                                      onSingleShotResponseCb);
        sem_wait(&semCompleted);
        singleShotFixCnt--;
        sleep(fixTbfMsec/1000);
    }
}

void getFusedFixes(char* buf) {
    uint32_t fixTbfMsec  = 5000;
    uint32_t timeoutMsec = 60000;
    float    horQoS      = 1000;

    static char *save = nullptr;
    char* token = strtok_r(buf, " ", &save); // skip first token

    // get timeout
    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        timeoutMsec = atoi(token);
    }

    // get qos
    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        horQoS = atof(token);
    }

    // get fix cnt
    singleShotFixCnt = 1;
    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        singleShotFixCnt = atoi(token);
    } else {
        singleShotFixCnt = 1;
    }

    // get tbf
    token = strtok_r(NULL, " ", &save);
    if (token != NULL) {
        fixTbfMsec = atoi(token);
    } else {
        fixTbfMsec = 1000;
    }

    printf("timeout msec %d, horQoS %f, number of fixes %d, tbf %d msec\n",
           timeoutMsec, horQoS, singleShotFixCnt, fixTbfMsec);

    if (!pLcaClient) {
        pLcaClient = new LocationClientApi(onCapabilitiesCb);
    }

    if (pLcaClient) {
        if (singleShotFixCnt == 1) {
            pLcaClient->getSinglePosition(timeoutMsec, horQoS,
                                          onSingleShotLocationCb,
                                          onSingleShotResponseCb);
        } else {
            std::thread t([timeoutMsec, horQoS, fixTbfMsec] {
                getMultipleFusedFixes(timeoutMsec, horQoS, fixTbfMsec);
            });
            t.detach();
        }
    }
}

void addGeofences(char* buf) {
    double latitude = 32.896535;
    double longitude = -117.201025;
    double radiusM = 50;
    GeofenceBreachTypeMask breachType = (GeofenceBreachTypeMask) (
            GEOFENCE_BREACH_ENTER_BIT | GEOFENCE_BREACH_EXIT_BIT);
    uint32_t responsivenessMs = 4000;
    uint32_t dwellTimeSec = 4;
    vector<Geofence> addGfVec;

    static char *save = nullptr;
    static char *inner = nullptr;
    char* token = strtok_r(buf, " ", &save); // skip first token ;
    string paramStr;
    int index = sGeofences.size();

    while ((token = strtok_r(NULL, ",", &save)) != NULL) {
        buf = token;
        try {
            // get latitude
            token = strtok_r(buf, " ", &inner);
            if (token != NULL) {
                paramStr.assign(token);
                latitude = ::std::stof(paramStr);
            }
            // get longitude
            token = strtok_r(NULL, " ", &inner);
            if (token != NULL) {
                paramStr.assign(token);
                longitude = ::std::stof(paramStr);
            }
        } catch (std::exception const & e) {
            printf("invalid input.\n");
            return;
        }
        // get radius
        token = strtok_r(NULL, " ", &inner);
        if (token != NULL && atof(token) >= 0) {
            radiusM = atof(token);
        }
        // get breachType
        token = strtok_r(NULL, " ", &inner);
        if (token != NULL && atoi(token) >= 0) {
            breachType = (GeofenceBreachTypeMask)atoi(token);
        } else {
            breachType = (GeofenceBreachTypeMask)(
                    GEOFENCE_BREACH_ENTER_BIT | GEOFENCE_BREACH_EXIT_BIT);
        }
        // get responsiveness
        token = strtok_r(NULL, " ", &inner);
        if (token != NULL && atoi(token) >= 0) {
            responsivenessMs = atoi(token);
        }
        // get dwellTime
        token = strtok_r(NULL, " ", &inner);
        if (token != NULL && atoi(token) >= 0) {
            dwellTimeSec = atoi(token);
        }
        Geofence gf(latitude, longitude, radiusM, breachType, responsivenessMs,
                dwellTimeSec);
        if (addGfVec.size() < 20) {
            printf("addGeofences, index: %d, latitude: %f, longitude: %f, radiusM: %f, "
                    "breachType: %d, responsivenessMs: %d, dwellTimeSec: %d\n", index++,
                    latitude, longitude, radiusM, breachType, responsivenessMs, dwellTimeSec);
            addGfVec.push_back(gf);
        } else {
            printf("Input more than 20 geofences. Only the first 20 would be added\n");
            break;
        }
    }

    if (!pLcaClient) {
        pLcaClient = new LocationClientApi(onCapabilitiesCb);
        sleep(1); // wait for capability callback
    }
    pLcaClient->addGeofences(addGfVec, onGeofenceBreachCb, onCollectiveResponseCb);
    sGeofences.insert(sGeofences.end(), addGfVec.begin(), addGfVec.end());
    printf("currently there are %d geofences available, please input index and the "
            "latitude/longitude/radius/breachType/responsiveness/dwelltime\n", sGeofences.size());
}

void modifyGeofences(char* buf) {
    int32_t seqNum = 0;
    GeofenceBreachTypeMask breachType = (GeofenceBreachTypeMask) (
            GEOFENCE_BREACH_ENTER_BIT | GEOFENCE_BREACH_EXIT_BIT);
    uint32_t responsivenessMs = 4000;
    uint32_t dwellTimeSec = 4;
    vector<Geofence> modifyGfVec;

    static char *save = nullptr;
    static char *inner = nullptr;
    char* token = strtok_r(buf, " ", &save); // skip first token

    printf("currently there are %d geofences available, please input index and "
            "breachType/responsiveness/dwelltime\n", sGeofences.size());
    while ((token = strtok_r(NULL, ",", &save)) != NULL) {
        buf = token;
        // get index
        token = strtok_r(buf, " ", &inner);
        if (token != NULL) {
            seqNum = atoi(token);
        }
        // get breachType
        token = strtok_r(NULL, " ", &inner);
        if (token != NULL && atoi(token) >= 0) {
            breachType = (GeofenceBreachTypeMask)atoi(token);
        } else {
            breachType = (GeofenceBreachTypeMask)(
                    GEOFENCE_BREACH_ENTER_BIT | GEOFENCE_BREACH_EXIT_BIT);
        }
        // get responsiveness
        token = strtok_r(NULL, " ", &inner);
        if (token != NULL && atoi(token) >= 0) {
            responsivenessMs = atoi(token);
        }
        // get dwellTime
        token = strtok_r(NULL, " ", &inner);
        if (token != NULL && atoi(token) >= 0) {
            dwellTimeSec = atoi(token);
        }
        if (sGeofences.size() <= seqNum) {
            printf("index not exists!! \n");
            return;
        }
        printf("modifyGeofences seqNum: %d, latitude: %f, longitude: %f, radiusM: %f,"
               " Updated breachType: %d, responsivenessMs: %d, dwellTimeSec: %d\n", seqNum,
               sGeofences[seqNum].getLatitude(), sGeofences[seqNum].getLongitude(),
               sGeofences[seqNum].getRadius(), breachType, responsivenessMs, dwellTimeSec);
        sGeofences[seqNum].setBreachType(breachType);
        sGeofences[seqNum].setResponsiveness(responsivenessMs);
        sGeofences[seqNum].setDwellTime(dwellTimeSec);
        modifyGfVec.push_back(sGeofences[seqNum]);
    }

    if (!pLcaClient) {
        pLcaClient = new LocationClientApi(onCapabilitiesCb);
    }
    pLcaClient->modifyGeofences(modifyGfVec);
}
void pauseGeofences(char* buf) {
    static char *save = nullptr;
    char* token = strtok_r(buf, " ", &save); // skip first token
    int32_t seqNum = 0;
    vector<Geofence> pauseGfVec;

    printf("currently there are %d geofences available, please input index \n",
            sGeofences.size());
    // get index
    while ((token = strtok_r(NULL, " ", &save)) != NULL) {
        seqNum = atoi(token);
        if (sGeofences.size() <= seqNum) {
            printf("index not exists!! \n");
            return;
        }
        printf("pauseGeofences seqNum: %d, lat: %f, lon: %f, radiusM: %f, breachType: %d,"
                "responsivenessMs: %d, dwellTimeSec: %d\n", seqNum,
                sGeofences[seqNum].getLatitude(), sGeofences[seqNum].getLongitude(),
                sGeofences[seqNum].getRadius(), sGeofences[seqNum].getBreachType(),
                sGeofences[seqNum].getResponsiveness(), sGeofences[seqNum].getDwellTime());
        pauseGfVec.push_back(sGeofences[seqNum]);
    }

    if (!pLcaClient) {
        pLcaClient = new LocationClientApi(onCapabilitiesCb);
    }
    pLcaClient->pauseGeofences(pauseGfVec);
}

void resumeGeofences(char* buf) {
    static char *save = nullptr;
    char* token = strtok_r(buf, " ", &save); // skip first token
    int32_t seqNum = 0;
    vector<Geofence> resumeGfVec;

    printf("currently there are %d geofences available, please input index \n",
            sGeofences.size());
    while ((token = strtok_r(NULL, " ", &save)) != NULL) {
        // get index
        seqNum = atoi(token);
        if (sGeofences.size() <= seqNum) {
            printf("index not exists!! \n");
            return;
        }
        printf("resumeGeofences seqNum: %d, lat: %f, lon: %f, radiusM: %f, breachType: %d,"
                "responsivenessMs: %d, dwellTimeSec: %d\n", seqNum,
                sGeofences[seqNum].getLatitude(), sGeofences[seqNum].getLongitude(),
                sGeofences[seqNum].getRadius(), sGeofences[seqNum].getBreachType(),
                sGeofences[seqNum].getResponsiveness(), sGeofences[seqNum].getDwellTime());
        resumeGfVec.push_back(sGeofences[seqNum]);
    }

    if (!pLcaClient) {
        pLcaClient = new LocationClientApi(onCapabilitiesCb);
    }
    pLcaClient->resumeGeofences(resumeGfVec);
}
void removeGeofences(char* buf) {
    static char *save = nullptr;
    char* token = strtok_r(buf, " ", &save); // skip first token
    int32_t seqNum = 0;
    vector<Geofence> removeGfVec;
    vector<int> seqNumVec;

    printf("currently there are %d geofences available, please input index \n",
            sGeofences.size());
    while ((token = strtok_r(NULL, " ", &save)) != NULL) {
        // get index
        seqNum = atoi(token);
        if (sGeofences.size() <= seqNum) {
            printf("index not exists!! \n");
            return;
        }
        printf("removeGeofences seqNum: %d, lat: %f, long: %f, radiusM: %f, breachType: %d,"
                "responsivenessMs: %d, dwellTimeSec: %d\n", seqNum,
                sGeofences[seqNum].getLatitude(), sGeofences[seqNum].getLongitude(),
                sGeofences[seqNum].getRadius(), sGeofences[seqNum].getBreachType(),
                sGeofences[seqNum].getResponsiveness(), sGeofences[seqNum].getDwellTime());
        removeGfVec.push_back(sGeofences[seqNum]);
        seqNumVec.push_back(seqNum);
    }

    if (!pLcaClient) {
        pLcaClient = new LocationClientApi(onCapabilitiesCb);
    }
    pLcaClient->removeGeofences(removeGfVec);

    std::sort(seqNumVec.begin(), seqNumVec.end());
    std::reverse(seqNumVec.begin(), seqNumVec.end());
    for (int seq: seqNumVec) {
        printf("geofenceSeq: %d \n", seq);
        sGeofences.erase(sGeofences.begin()+seq);
    }
}

static bool checkForAutoStart(int argc, char *argv[]) {
    bool autoRun = false;
    bool deleteAll = false;
    bool addGfs = false;
    string gfStr = "-g";
    uint32_t aidingDataMask = 0;
    int interval = 100;
    LocReqEngineTypeMask reqEngMask = (LocReqEngineTypeMask) 0x7;
    uint32_t reportType =  0x2fd;
    TrackingSessionType trackingType = NO_TRACKING;

    //Specifying the expected options
    //The two options l and b expect numbers as argument
    static struct option long_options[] = {
        {"auto",      no_argument,       0,  'a' },
        {"verbose",   no_argument,       0,  'V' },
        {"nooutput",   no_argument,      0,  'N' },
        {"deleteAll", no_argument,       0,  'D' },
        {"delete",    required_argument, 0,  'd' },
        {"session",   required_argument, 0,  's' },
        {"engine",    required_argument, 0,  'e' },
        {"interval",  required_argument, 0,  'i' },
        {"timeout", required_argument,   0,  't' },
        {"fixcnt",   required_argument,  0,  'l' },
        {"reportType", required_argument, 0,  'r' },
        {"addgeofences", required_argument, 0,  'g' },
        {"startBatching", required_argument, 0,  'b' },
        {0,           0,                 0,   0  }
    };

    int long_index =0;
    int opt = -1;
    while ((opt = getopt_long(argc, argv, "aVNDd:s:e:i:t:l:r:U:z:g:b:",
                   long_options, &long_index)) != -1) {
        switch (opt) {
             case 'a' :
                 autoRun = true;
                 break;
             case 'V' :
                 detailedOutputEnabled = true;
                 break;
             case 'N' :
                 outputEnabled = false;
                 break;
             case 'D':
                 deleteAll = true;
                 break;
             case 'd' :
                 aidingDataMask = atoi(optarg);
                 break;
             case 's':
                 printf("session type: %s\n", optarg);
                 if (optarg[0] == 'l') {
                     trackingType = SIMPLE_REPORT_TRACKING;
                 } else if (optarg[0] == 'g') {
                     trackingType = DETAILED_REPORT_TRACKING;
                 } else{
                     trackingType = ENGINE_REPORT_TRACKING;
                 }
                 break;
             case 'e' :
                 printf("report mask: %s\n", optarg);
                 reqEngMask = (LocReqEngineTypeMask) atoi(optarg);
                 trackingType = ENGINE_REPORT_TRACKING;
                 break;
             case 'l':
                 printf("fix cnt: %s\n", optarg);
                 fixCnt = atoi(optarg);
                 if (fixCnt <= 0) {
                     fixCnt = 1;
                     printf("setting fixCnt to %d", fixCnt);
                 }
                 if (trackingType == NO_TRACKING) {
                    trackingType = ENGINE_REPORT_TRACKING;
                 }
                 break;
            case 'i':
                 printf("interval: %s\n", optarg);
                 interval = atoi(optarg);
                 if (interval <= 0) {
                     interval = 1000;
                     printf("setting interval to %d", interval);
                 }
                 if (trackingType == NO_TRACKING) {
                     trackingType = ENGINE_REPORT_TRACKING;
                 }
                 break;
             case 't':
                 printf("tiemout: %s\n", optarg);
                 autoTestTimeoutSec = atoi(optarg);
                 if (autoTestTimeoutSec <= 0) {
                     autoTestTimeoutSec = 30;
                     printf("setting autoTestTimeoutSec to %d", autoTestTimeoutSec);
                 }
                 break;
             case 'r' :
                 printf("report type: %s\n", optarg);
                 reportType = atoi(optarg);
                 if (reportType <= 0) {
                     reportType = 1;
                     printf("setting reportType to %d", reportType);
                 }
                 break;
             case 'U' :
                 printf("route to NMEA port: %s\n", optarg);
                 routeToNMEAPort = atoi(optarg);
                 break;
             case 'g' :
                 for (int i = optind-1; i< argc &&
                         !(argv[i][0] == '-' && (argv[i][1] < '0' || argv[i][1] > '9')); ++i) {
                     gfStr.append(" ");
                     gfStr.append(argv[i]);
                 }
                 addGfs = true;
                 break;
             case 'b' :
                 if (!pLcaClient) {
                    pLcaClient = new LocationClientApi(onCapabilitiesCb);
                 }
                 sleep(1);
                 if (pLcaClient) {
                     int intervalmsec = 60000;
                     int distance = 0;
                     int duration = 600; // default test duration is 10mins
                     intervalmsec = atoi(argv[optind - 1]);
                     distance = atoi(argv[optind]);
                     duration = atoi(argv[optind + 1]);
                     printf("start routine batching with interval %d msec, distance %d meters, "
                             "duration %d seconds\n",
                             intervalmsec, distance, duration);
                     autoTestStartBatchingTimeMs = getTimestampMs();
                     resetCounters();
                     pLcaClient->startRoutineBatchingSession(intervalmsec, distance,
                             onBatchingCb, onResponseCb);
                     std::thread t([duration] {
                             usleep(duration * 1000000);
                             sem_post(&semBatchingCompleted);
                             });
                     t.detach();
                 }
                 break;
             default:
                 printf("unsupported args provided\n");
                 break;
        }
    }

    printf("auto run %d, deleteAll %d, delete mask 0x%x, session type %d,"
           "outputEnabled %d, detailedOutputEnabled %d routeToNMEAPort %d",
           autoRun, deleteAll, aidingDataMask, trackingType,
           outputEnabled, detailedOutputEnabled, routeToNMEAPort);

    // check for auto-start option
    if (autoRun) {
        uint32_t pid = (uint32_t)getpid();
        printf("pid: %d\n", pid);

        if (deleteAll == true || aidingDataMask != 0) {
            // create location integratin API
            LocIntegrationCbs intCbs;
            intCbs.configCb = LocConfigCb(onConfigResponseCb);
            LocConfigPriorityMap priorityMap;
            pIntClient = new LocationIntegrationApi(priorityMap, intCbs);
            if (!pIntClient) {
                printf("can not create Location integration API");
                exit(1);
            }
            sleep(1); // wait for capability callback
            if (deleteAll) {
                pIntClient->deleteAllAidingData();
            } else {
                pIntClient->deleteAidingData((AidingDataDeletionMask) aidingDataMask);
            }
            // wait for config response callback to be received
            sleep(1);
        }

        if (trackingType != NO_TRACKING) {
            pLcaClient = new LocationClientApi(onCapabilitiesCb);
            sleep(1); // wait for capability callback
            if (nullptr == pLcaClient) {
                printf("can not create Location client API");
                exit(1);
            }

            resetCounters();
            if (trackingType == SIMPLE_REPORT_TRACKING) {
                pLcaClient->startPositionSession(interval, 0, onLocationCb, onResponseCb);
            } else if (trackingType == DETAILED_REPORT_TRACKING) {
                GnssReportCbs reportcbs = {};
                setupGnssReportCbs(reportType, reportcbs);
                pLcaClient->startPositionSession(interval, reportcbs, onResponseCb);
            } else if (reqEngMask != 0) {
                EngineReportCbs reportcbs = {};
                setupEngineReportCbs(reportType, reportcbs);
                pLcaClient->startPositionSession(interval, reqEngMask,
                                                       reportcbs, onResponseCb);
            }
            autoTestStartTimeMs = getTimestampMs();
        }

    }
    if (addGfs) {
        printf("addGeofence option param %s: \n", gfStr.c_str());
        sleep(1);
        autoTestStartGfTimeMs = getTimestampMs();
        addGeofences(const_cast<char*>(gfStr.c_str()));
        std::thread t([autoTestTimeoutSec] {
                usleep(autoTestTimeoutSec * 1000000);
                sem_post(&semGfCompleted);
                });
        t.detach();
    }
    if (pLcaClient || pIntClient) {
        if (trackingType != NO_TRACKING && autoTestStartTimeMs != 0) {
            sem_wait(&semCompleted);
        }
        if (autoTestStartGfTimeMs != 0) {
            sem_wait(&semGfCompleted);
        }
        if (autoTestStartBatchingTimeMs != 0) {
            sem_wait(&semBatchingCompleted);
        }
        cleanupAfterAutoStart();
        exit(0);
    }
    return autoRun;
}

// get report type for 'g' and 'e' option
void getTrackingParams(char *buf, uint32_t *reportTypePtr, uint32_t *tbfMsecPtr,
                       LocReqEngineTypeMask* reqEngMaskPtr) {
    static char *save = nullptr;
    char* token = strtok_r(buf, " ", &save); // skip first token
    token = strtok_r(NULL, " ", &save);
    if (token != nullptr) {
        if (reportTypePtr) {
            *reportTypePtr = strtoul(token, NULL, 10);
            if (0 == *reportTypePtr) {
                *reportTypePtr = strtoul(token, NULL, 16);
            }
        }
    }
    token = strtok_r(NULL, " ", &save);
    if (token != nullptr) {
        if (tbfMsecPtr) {
            *tbfMsecPtr = atoi(token);
        }
    }
    token = strtok_r(NULL, " ", &save);
    if (token != nullptr) {
        if (reqEngMaskPtr) {
            *reqEngMaskPtr = (LocReqEngineTypeMask) atoi(token);
        }
    }

    // initialize to default value in case of invalid input
    if (reportTypePtr) {
        if (*reportTypePtr == 0) {
            *reportTypePtr = 0x1FF;
        }
    }

    if (tbfMsecPtr) {
        if (*tbfMsecPtr == 0) {
            *tbfMsecPtr = 100;
        }
    }
    if (reqEngMaskPtr) {
        if (*reqEngMaskPtr == (LocReqEngineTypeMask) 0) {
            *reqEngMaskPtr = (LocReqEngineTypeMask)
                    (LOC_REQ_ENGINE_FUSED_BIT|LOC_REQ_ENGINE_SPE_BIT|
                     LOC_REQ_ENGINE_PPE_BIT);
        }
    }
}

int getGeofenceCount() {
    int count = 1;
    char buf[16], *p;
    printf ("\nEnter number of geofences (default %d):", 1);
    fflush (stdout);
    p = fgets (buf, 16, stdin);
    if (p == nullptr) {
        printf("Error: fgets returned nullptr !!");
        return count;
    }
    if (atoi(p) != 0) {
        count = atoi(p);
    }
    return count;
}

void menuAddGeofence() {
    uint32_t count = getGeofenceCount();
    double latitude = 32.896535;
    double longitude = -117.201025;
    double radiusM = 50;
    GeofenceBreachTypeMask breachType = (GeofenceBreachTypeMask) (
            GEOFENCE_BREACH_ENTER_BIT | GEOFENCE_BREACH_EXIT_BIT);
    uint32_t responsivenessMs = 4000;
    uint32_t dwellTimeSec = 4;
    char buf[16], *p;
    vector<Geofence> addGfVec;
    for (int i=0; i<count; ++i) {
        printf ("\nEntering geofence of serial number %d ):", sGeofences.size());

        printf ("\nEnter latitude (default %f):", 32.896535);
        fflush (stdout);
        p = fgets (buf, 16, stdin);
        if (p == nullptr) {
            printf("Error: fgets returned nullptr !!");
            return;
        }
        if (atof(p) != 0) {
            latitude = atof(p);
        }
        printf ("\nEnter longitude (default %f):", -117.201025);
        fflush (stdout);
        p = fgets (buf, 16, stdin);
        if (p == nullptr) {
            printf("Error: fgets returned nullptr !!");
            return;
        }
        if (atof(p) != 0) {
            longitude = atof(p);
        }
        printf ("\nEnter radius (default %f):", 50.0);
        fflush (stdout);
        p = fgets (buf, 16, stdin);
        if (p == nullptr) {
            printf("Error: fgets returned nullptr !!");
            return;
        }
        if (atof(p) != 0) {
            radiusM = atof(p);
        }
        printf ("\nEnter breachType: (default %x):", 0x11);
        fflush (stdout);
        p = fgets (buf, 16, stdin);
        if (p == nullptr) {
            printf("Error: fgets returned nullptr !!");
            return;
        }
        if (atoi(p) != 0) {
            breachType = (GeofenceBreachTypeMask)atoi(p);
        }
        printf ("\nEnter responsiveness in seconds: (default %d):", 4);
        fflush (stdout);
        p = fgets (buf, 16, stdin);
        if (p == nullptr) {
            printf("Error: fgets returned nullptr !!");
            return;
        }
        if (atoi(p) != 0) {
            responsivenessMs = atoi(p) * 1000;
        }
        printf ("\nEnter dwelltime in seconds: (default %d):", 4);
        fflush (stdout);
        p = fgets (buf, 16, stdin);
        if (p == nullptr) {
            printf("Error: fgets returned nullptr !!");
            return;
        }
        if (atoi(p) != 0) {
            dwellTimeSec = atoi(p);
        }
        Geofence gf(latitude, longitude, radiusM, breachType, responsivenessMs,
                dwellTimeSec);
        addGfVec.push_back(gf);
    }
    sGeofences.insert(sGeofences.end(), addGfVec.begin(), addGfVec.end());
    pLcaClient->addGeofences(addGfVec, onGeofenceBreachCb, onCollectiveResponseCb);
}

void menuModifyGeofence() {
    uint32_t count = getGeofenceCount();
    int32_t seqNum = 0;
    GeofenceBreachTypeMask breachType = (GeofenceBreachTypeMask) (
            GEOFENCE_BREACH_ENTER_BIT | GEOFENCE_BREACH_EXIT_BIT);
    uint32_t responsivenessMs = 4000;
    uint32_t dwellTimeSec = 4;
    char buf[16], *p;
    vector<Geofence> modifyGfVec;

    for (int i=0; i<count; ++i) {
        printf ("\nEnter geofence serial number, (default %d):", 0);
        fflush (stdout);
        p = fgets (buf, 16, stdin);
        if (p == nullptr) {
            printf("Error: fgets returned nullptr !!");
            return;
        }
        if (atoi(p) != 0) {
            seqNum = atoi(p);
        }
        printf ("\nEnter breachType: (default 0x%x):", 0x11);
        fflush (stdout);
        p = fgets (buf, 16, stdin);
        if (p == nullptr) {
            printf("Error: fgets returned nullptr !!");
            return;
        }
        if (atoi(p) != 0) {
            breachType = (GeofenceBreachTypeMask)atoi(p);
        }
        printf ("\nEnter responsiveness in seconds: (default %d):",
                responsivenessMs / 1000);
        fflush (stdout);
        p = fgets (buf, 16, stdin);
        if (p == nullptr) {
            printf("Error: fgets returned nullptr !!");
            return;
        }
        if (atoi(p) != 0) {
            responsivenessMs = atoi(p) * 1000;
        }
        printf ("\nEnter dwelltime in seconds: (default %d):",
                dwellTimeSec);
        fflush (stdout);
        p = fgets (buf, 16, stdin);
        if (p == nullptr) {
            printf("Error: fgets returned nullptr !!");
            return;
        }
        if (atoi(p) != 0) {
            dwellTimeSec = atoi(p);
        }
        sGeofences[seqNum].setBreachType(breachType);
        sGeofences[seqNum].setResponsiveness(responsivenessMs);
        sGeofences[seqNum].setDwellTime(dwellTimeSec);
        modifyGfVec.push_back(sGeofences[seqNum]);
    }
    pLcaClient->modifyGeofences(modifyGfVec);
}

void menuPauseGeofence() {
    int32_t seqNum = 0;
    uint32_t count = getGeofenceCount();
    char buf[16], *p;
    vector<Geofence> pauseGfVec;

    for (int i=0; i<count; ++i) {
        printf ("\nEnter geofence serial number, (default %d):", 0);
        fflush (stdout);
        p = fgets (buf, 16, stdin);
        if (p == nullptr) {
            printf("Error: fgets returned nullptr !!");
            return;
        }
        if (atoi(p) != 0) {
            seqNum = atoi(p);
        }
        pauseGfVec.push_back(sGeofences[seqNum]);
    }
    pLcaClient->pauseGeofences(pauseGfVec);
}

void menuResumeGeofence() {
    int32_t seqNum = 0;
    uint32_t count = getGeofenceCount();
    char buf[16], *p;
    vector<Geofence> resumeGfVec;
    for (int i=0; i<count; ++i) {
        printf ("\nEnter geofence serial number, (default %d):", 0);
        fflush (stdout);
        p = fgets (buf, 16, stdin);
        if (p == nullptr) {
            printf("Error: fgets returned nullptr !!");
            return;
        }
        if (atoi(p) != 0) {
            seqNum = atoi(p);
        }
        resumeGfVec.push_back(sGeofences[seqNum]);
    }
    pLcaClient->resumeGeofences(resumeGfVec);
}

void menuRemoveGeofence() {
    int32_t seqNum = 0;
    uint32_t count = getGeofenceCount();
    char buf[16], *p;
    vector<Geofence> removeGfVec;
    for (int i=0; i<count; ++i) {
        printf ("\nEnter geofence serial number, (default %d):", 0);
        fflush (stdout);
        p = fgets (buf, 16, stdin);
        if (p == nullptr) {
            printf("Error: fgets returned nullptr !!");
            return;
        }
        if (atoi(p) != 0) {
            seqNum = atoi(p);
        }
        removeGfVec.push_back(sGeofences[seqNum]);
    }
    pLcaClient->removeGeofences(removeGfVec);
}
void geofenceTestMenu() {
    char buf[16], *p = NULL;
    bool exit_loop = false;

    while (!exit_loop)
    {
        printf ("\n\n"
            "1: add_geofence\n"
            "2: pause_geofence\n"
            "3: resume geofence\n"
            "4: modify geofence\n"
            "5: remove geofence\n"
            "b: back\n"
            "q: quit\n\n"
            "Enter Command:");
        fflush (stdout);
        p = fgets (buf, 16, stdin);
        if (p == nullptr) {
            printf("Error: fgets returned nullptr !!");
            exit_loop = true;
            continue;
        }
        switch (p[0]) {
        case '1':
            menuAddGeofence();
            break;
        case '2':
            menuPauseGeofence();
            break;
        case '3':
            menuResumeGeofence();
            break;
        case '4':
            menuModifyGeofence();
            break;
        case '5':
            menuRemoveGeofence();
            break;
        case 'b':
            exit_loop = true;
            break;
        case 'q':
            return;
        default:
            printf("\ninvalid command\n");
        }
    }
}

/******************************************************************************
Main function
******************************************************************************/
int main(int argc, char *argv[]) {

    setRequiredPermToRunAsLocClient();
    checkForAutoStart(argc, argv);

    // create Location client API
    pLcaClient = new LocationClientApi(onCapabilitiesCb);
    if (!pLcaClient) {
        printf("failed to create client, return");
        return -1;
    }

    // gnss callbacks
    GnssReportCbs reportcbs = {};

    // engine callbacks
    EngineReportCbs enginecbs = {};

    // create location integratin API
    LocIntegrationCbs intCbs = {};

    intCbs.configCb = LocConfigCb(onConfigResponseCb);
    intCbs.getRobustLocationConfigCb =
            LocConfigGetRobustLocationConfigCb(onGetRobustLocationConfigCb);
    intCbs.getMinGpsWeekCb = LocConfigGetMinGpsWeekCb(onGetMinGpsWeekCb);
    intCbs.getMinSvElevationCb = LocConfigGetMinSvElevationCb(onGetMinSvElevationCb);
    intCbs.getConstellationSecondaryBandConfigCb =
            LocConfigGetConstellationSecondaryBandConfigCb(onGetSecondaryBandConfigCb);
    intCbs.getXtraStatusCb = LocConfigGetXtraStatusCb(onGetXtraStatusCb);
    intCbs.gnssSignalTypesCb = LocConfigGnssSignalTypesCb(onGnssSignalTypesCb);

    LocConfigPriorityMap priorityMap;
    pIntClient = new LocationIntegrationApi(priorityMap, intCbs);

    printHelp();
    sleep(1); // wait for capability callback if you don't like sleep

    // main loop
    while (1) {
        bool retVal = true;
        char buf[1500];
        memset (buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), stdin);

        printf("execute command %s\n", buf);
        if (!pIntClient) {
            pIntClient = new LocationIntegrationApi(priorityMap, intCbs);
            if (nullptr == pIntClient) {
                printf("failed to create integration client\n");
                break;
            }
            sleep(1); // wait for capability callback if you don't like sleep
        }

        if (strncmp(buf, ENABLE_REPORT_OUTPUT, strlen(ENABLE_REPORT_OUTPUT)) == 0) {
            outputEnabled = true;
            detailedOutputEnabled = false;
        } else if (strncmp(buf, ENABLE_DETAILED_REPORT_OUTPUT,
                           strlen(ENABLE_DETAILED_REPORT_OUTPUT)) == 0) {
            outputEnabled = true;
            detailedOutputEnabled = true;
        } else if (strncmp(buf, DISABLE_REPORT_OUTPUT, strlen(DISABLE_REPORT_OUTPUT)) == 0) {
            outputEnabled = false;
        } else if (strncmp(buf, DISABLE_TUNC, strlen(DISABLE_TUNC)) == 0) {
            pIntClient->configConstrainedTimeUncertainty(false);
        } else if (strncmp(buf, ENABLE_TUNC, strlen(ENABLE_TUNC)) == 0) {
            // get tuncThreshold and energyBudget from the command line
            static char *save = nullptr;
            float tuncThreshold = 0.0;
            int   energyBudget = 0;
            char* token = strtok_r(buf, " ", &save); // skip first one
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                tuncThreshold = atof(token);
                token = strtok_r(NULL, " ", &save);
                if (token != NULL) {
                    energyBudget = atoi(token);
                }
            }
            printf("tuncThreshold %f, energyBudget %d\n", tuncThreshold, energyBudget);
            retVal = pIntClient->configConstrainedTimeUncertainty(
                    true, tuncThreshold, energyBudget);
        } else if (strncmp(buf, DISABLE_PACE, strlen(DISABLE_TUNC)) == 0) {
            retVal = pIntClient->configPositionAssistedClockEstimator(false);
        } else if (strncmp(buf, ENABLE_PACE, strlen(ENABLE_TUNC)) == 0) {
            retVal = pIntClient->configPositionAssistedClockEstimator(true);
        } else if (strncmp(buf, DELETE_ALL, strlen(DELETE_ALL)) == 0) {
            retVal = pIntClient->deleteAllAidingData();
        } else if (strncmp(buf, DELETE_AIDING_DATA, strlen(DELETE_AIDING_DATA)) == 0) {
            uint32_t aidingDataMask = 0;
            printf("deleteAidingData 1 (eph) 2 (qdr calibration data) 3 (eph+calibraiton dat)\n");
            static char *save = nullptr;
            char* token = strtok_r(buf, " ", &save); // skip first one
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                aidingDataMask = atoi(token);
            }
            retVal = pIntClient->deleteAidingData((AidingDataDeletionMask) aidingDataMask);
        } else if (strncmp(buf, RESET_SV_CONFIG, strlen(RESET_SV_CONFIG)) == 0) {
            retVal = pIntClient->configConstellations(nullptr);
        } else if (strncmp(buf, CONFIG_SV, strlen(CONFIG_SV)) == 0) {
            bool resetConstellation = false;
            LocConfigBlacklistedSvIdList svList;
            LocConfigBlacklistedSvIdList* svListPtr = &svList;
            parseSVConfig(buf, resetConstellation, svList);
            if (resetConstellation) {
                svListPtr = nullptr;
            }
            retVal = pIntClient->configConstellations(svListPtr);
        } else if (strncmp(buf, CONFIG_SECONDARY_BAND, strlen(CONFIG_SECONDARY_BAND)) == 0) {
            bool nullSecondaryConfig = false;
            ConstellationSet secondaryBandDisablementSet;
            ConstellationSet* secondaryBandDisablementSetPtr = &secondaryBandDisablementSet;
            parseSecondaryBandConfig(buf, nullSecondaryConfig, secondaryBandDisablementSet);
            if (nullSecondaryConfig) {
                secondaryBandDisablementSetPtr = nullptr;
                printf("setting secondary band config to null\n");
            }
            retVal = pIntClient->configConstellationSecondaryBand(secondaryBandDisablementSetPtr);
        } else if (strncmp(buf, GET_SECONDARY_BAND_CONFIG,
                           strlen(GET_SECONDARY_BAND_CONFIG)) == 0) {
            retVal = pIntClient->getConstellationSecondaryBandConfig();
        } else if (strncmp(buf, MULTI_CONFIG_SV, strlen(MULTI_CONFIG_SV)) == 0) {
            // reset
            retVal = pIntClient->configConstellations(nullptr);
            // disable GAL
            LocConfigBlacklistedSvIdList galSvList;
            GnssSvIdInfo svIdInfo = {};
            svIdInfo.constellation = GNSS_CONSTELLATION_TYPE_GALILEO;
            svIdInfo.svId = 0;
            galSvList.push_back(svIdInfo);
            retVal = pIntClient->configConstellations(&galSvList);

            // disable SBAS
            LocConfigBlacklistedSvIdList sbasSvList;
            svIdInfo.constellation = GNSS_CONSTELLATION_TYPE_SBAS;
            svIdInfo.svId = 0;
            sbasSvList.push_back(svIdInfo);
            retVal = pIntClient->configConstellations(&sbasSvList);
        } else if (strncmp(buf, CONFIG_LEVER_ARM, strlen(CONFIG_LEVER_ARM)) == 0) {
            LeverArmParamsMap configInfo;
            parseLeverArm(buf, configInfo);
            retVal = pIntClient->configLeverArm(configInfo);
        } else if (strncmp(buf, CONFIG_ROBUST_LOCATION, strlen(CONFIG_ROBUST_LOCATION)) == 0) {
            // get enable and enableForE911
            static char *save = nullptr;
            bool enable = false;
            bool enableForE911 = false;
            // skip first argument of configRobustLocation
            char* token = strtok_r(buf, " ", &save);
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                enable = (atoi(token) == 1);
                token = strtok_r(NULL, " ", &save);
                if (token != NULL) {
                    enableForE911 = (atoi(token) == 1);
                }
            }
            printf("enable %d, enableForE911 %d\n", enable, enableForE911);
            retVal = pIntClient->configRobustLocation(enable, enableForE911);
        } else if (strncmp(buf, GET_ROBUST_LOCATION_CONFIG,
                           strlen(GET_ROBUST_LOCATION_CONFIG)) == 0) {
            retVal = pIntClient->getRobustLocationConfig();
        } else if (strncmp(buf, CONFIG_MIN_GPS_WEEK, strlen(CONFIG_MIN_GPS_WEEK)) == 0) {
            static char *save = nullptr;
            uint16_t gpsWeekNum = 0;
            // skip first argument of configMinGpsWeek
            char* token = strtok_r(buf, " ", &save);
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                gpsWeekNum = (uint16_t) atoi(token);
            }
            printf("gps week num %d\n", gpsWeekNum);
            retVal = pIntClient->configMinGpsWeek(gpsWeekNum);
        } else if (strncmp(buf, GET_MIN_GPS_WEEK, strlen(GET_MIN_GPS_WEEK)) == 0) {
            retVal = pIntClient->getMinGpsWeek();
        } else if (strncmp(buf, CONFIG_DR_ENGINE, strlen(CONFIG_DR_ENGINE)) == 0) {
            DeadReckoningEngineConfig dreConfig = {};
            parseDreConfig(buf, dreConfig);
            printf("mask 0x%x, roll offset %f, pitch offset %f, yaw offset %f, offset unc, %f\n"
                   "speed scale factor %f, speed scale factor unc %10f,\n"
                   "dreConfig.gyroScaleFactor %f, dreConfig.gyroScaleFactorUnc %10f\n",
                   dreConfig.validMask,
                   dreConfig.bodyToSensorMountParams.rollOffset,
                   dreConfig.bodyToSensorMountParams.pitchOffset,
                   dreConfig.bodyToSensorMountParams.yawOffset,
                   dreConfig.bodyToSensorMountParams.offsetUnc,
                   dreConfig.vehicleSpeedScaleFactor, dreConfig.vehicleSpeedScaleFactorUnc,
                   dreConfig.gyroScaleFactor, dreConfig.gyroScaleFactorUnc);
           retVal = pIntClient->configDeadReckoningEngineParams(dreConfig);
        } else if (strncmp(buf, CONFIG_MIN_SV_ELEVATION, strlen(CONFIG_MIN_SV_ELEVATION)) == 0) {
            static char *save = nullptr;
            uint8_t minSvElevation = 0;
            // skip first argument of configMinSvElevation
            char* token = strtok_r(buf, " ", &save);
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                minSvElevation = (uint16_t) atoi(token);
            }
            printf("min Sv elevation %d\n", minSvElevation);
            retVal = pIntClient->configMinSvElevation(minSvElevation);
        } else if (strncmp(buf, GET_MIN_SV_ELEVATION, strlen(GET_MIN_SV_ELEVATION)) == 0) {
            retVal = pIntClient->getMinSvElevation();
        } else if (strncmp(buf, CONFIG_ENGINE_RUN_STATE, strlen(CONFIG_ENGINE_RUN_STATE)) == 0) {
            printf("%s 3(DRE) 1(pause)/2(resume)/3(pause_retain)", CONFIG_ENGINE_RUN_STATE);
            static char *save = nullptr;
            LocIntegrationEngineType engType = (LocIntegrationEngineType)0;
            LocIntegrationEngineRunState engState = (LocIntegrationEngineRunState) 0;
            // skip first argument of configEngineRunState
            char* token = strtok_r(buf, " ", &save);
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
               engType = (LocIntegrationEngineType) atoi(token);
                token = strtok_r(NULL, " ", &save);
                if (token != NULL) {
                    engState = (LocIntegrationEngineRunState) atoi(token);
                }
            }
            printf("eng type %d, eng state %d\n", engType, engState);
            retVal = pIntClient->configEngineRunState(engType, engState);
        } else if (strncmp(buf, CONFIG_ENGINE_INTEGRITY_RISK,
                           strlen(CONFIG_ENGINE_INTEGRITY_RISK)) == 0) {
            printf("%s 1(SPE)/2(PPE)/3(DRE)/4(VPE) integrity_risk_level\n",
                   CONFIG_ENGINE_INTEGRITY_RISK);
            static char *save = nullptr;
            LocIntegrationEngineType engType = (LocIntegrationEngineType)0;
            uint32_t integrityRisk = (LocIntegrationEngineRunState) 0;
            // skip first argument of configEngineRunState
            char* token = strtok_r(buf, " ", &save);
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
               engType = (LocIntegrationEngineType) atoi(token);
                token = strtok_r(NULL, " ", &save);
                if (token != NULL) {
                    integrityRisk =  atoi(token);
                }
            }
            printf("eng type %d, integrity risk %u\n", engType, integrityRisk);
            retVal = pIntClient->configEngineIntegrityRisk(engType, integrityRisk);
        } else if (strncmp(buf, SET_USER_CONSENT, strlen(SET_USER_CONSENT)) == 0) {
            static char *save = nullptr;
            bool userConsent = false;
            char* token = strtok_r(buf, " ", &save);
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                userConsent = (atoi(token) != 0);
            }
            printf("userConsent %d\n", userConsent);
            retVal = pIntClient->setUserConsentForTerrestrialPositioning(userConsent);
        } else if (strncmp(buf, GET_SINGLE_GTP_WWAN_FIX, strlen(GET_SINGLE_GTP_WWAN_FIX)) == 0) {
            // get single-shot gtp fixes
            getGtpWwanFixes(false, buf);
        } else if (strncmp(buf, GET_MULTIPLE_GTP_WWAN_FIXES,
                           strlen(GET_MULTIPLE_GTP_WWAN_FIXES)) == 0) {
            // get multiple gtp fixes
            getGtpWwanFixes(true, buf);
        } else if (strncmp(buf, CANCEL_SINGLE_GTP_WWAN_FIX,
                           strlen(CANCEL_SINGLE_GTP_WWAN_FIX)) == 0) {
            if (!pLcaClient) {
                pLcaClient = new LocationClientApi(onCapabilitiesCb);
            }
            if (pLcaClient) {
                pLcaClient->getSingleTerrestrialPosition(
                        0, TERRESTRIAL_TECH_GTP_WWAN, 0.0, nullptr, onGtpResponseCb);
            }
        } else if (strncmp(buf, GET_ENERGY_CONSUMED,
                           strlen(GET_ENERGY_CONSUMED)) == 0) {
            if (!pLcaClient) {
                pLcaClient = new LocationClientApi(onCapabilitiesCb);
            }
            if (pLcaClient) {
                pLcaClient->getGnssEnergyConsumed(onGetGnssEnergyConsumedCb, onResponseCb);
            }
        } else if (strncmp(buf, CONFIG_NMEA_TYPES, strlen(CONFIG_NMEA_TYPES)) == 0) {
            static char *save = nullptr;
            NmeaTypesMask nmeaTypes = (NmeaTypesMask) NMEA_TYPE_ALL;
            char* token = strtok_r(buf, " ", &save);
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                nmeaTypes = (NmeaTypesMask) strtoul(token, NULL, 10);
                if (nmeaTypes == 0) {
                    nmeaTypes = (NmeaTypesMask) strtoul(token, NULL, 16);
                }
            }
            GeodeticDatumType nmeaDatumType = GEODETIC_TYPE_WGS_84;
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                if (strtoul(token, NULL, 10) == 1) {
                    nmeaDatumType = GEODETIC_TYPE_PZ_90;
                }
            }
            LocReqEngineTypeMask engTypeMask = LOC_REQ_ENGINE_FUSED_BIT;
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                engTypeMask = (LocReqEngineTypeMask) strtoul(token, NULL, 10);
                if (0 == engTypeMask) {
                    engTypeMask = (LocReqEngineTypeMask) strtoul(token, NULL, 16);
                }
            }
            printf("nmeaTypes 0x%x, geodetic type %d engineTypeMask 0x%x\n", nmeaTypes,
                    nmeaDatumType, engTypeMask);
            if (0 == engTypeMask) {
                pIntClient->configOutputNmeaTypes(nmeaTypes, nmeaDatumType);
            } else {
                pIntClient->configOutputNmeaTypes(nmeaTypes, nmeaDatumType, engTypeMask);
            }
        } else if (strncmp(buf, INJECT_LOCATION,
                           strlen(INJECT_LOCATION)) == 0) {
            location_client::Location injectLocation = {};
            parseLocation(buf, injectLocation);
            printf("Injected location info: %s\n", injectLocation.toString().c_str());
            pIntClient->injectLocation(injectLocation);
        } else if (strncmp(buf, GET_SINGLE_FUSED_FIX,
                           strlen(GET_SINGLE_FUSED_FIX)) == 0) {
            printf("usage: getSingleFusedFix timeout qos fixcnt tbf\n");
            if (!pLcaClient) {
                pLcaClient = new LocationClientApi(onCapabilitiesCb);
            }
            if (pLcaClient) {
                getFusedFixes(buf);
            }
        } else if (strncmp(buf, CANCEL_SINGLE_FUSED_FIX,
                           strlen(CANCEL_SINGLE_FUSED_FIX)) == 0) {
            printf("usage: cancelSingleFusedFix\n");
            if (!pLcaClient) {
                pLcaClient = new LocationClientApi(onCapabilitiesCb);
            }
            if (pLcaClient) {
                pLcaClient->getSinglePosition(0, 0, nullptr, onSingleShotResponseCb);
            }
            singleShotFixCnt = 0;
        } else if (strncmp(buf, ENABLE_XTRA_ON_DEMAND_DOWNLOAD,
                           strlen(ENABLE_XTRA_ON_DEMAND_DOWNLOAD)) == 0) {
            printf("usage: enableXtraOnDemandDownload 0/1 (enable/disable xtra download) "
                   "0/1 (enable/disable) integrity\n");
            bool enableXtra = false;
            XtraConfigParams xtraConfig = {};

            static char *save = nullptr;
            char* token = strtok_r(buf, " ", &save); // skip header

            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                enableXtra = atoi(token);
            }

            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                if (enableXtra) {
                    xtraConfig.xtraIntegrityDownloadEnable = atoi(token);
                }
            }
            xtraConfig.xtraDaemonDebugLogLevel = DEBUG_LOG_LEVEL_VERBOSE;
            printf("xtra enabled %d, integrity enabled %d, debug level set to 5",
                   enableXtra, xtraConfig.xtraIntegrityDownloadEnable);
            bool retval = pIntClient->configXtraParams(enableXtra, &xtraConfig);
            if (retval == false) {
                printf("config xtra params failed\n");
            } else {
                printf("config xtra successful\n");
            }
        } else if (strncmp(buf, CONFIG_XTRA_PARAMS, strlen(CONFIG_XTRA_PARAMS)) == 0) {
            bool enableXtra = false;
            XtraConfigParams xtraConfig = {};
            parseXtraConfig(buf, enableXtra, xtraConfig);
            bool retval = pIntClient->configXtraParams(enableXtra, &xtraConfig);
            if (retval == false) {
                printf("config xtra params failed\n");
            }
        } else if (strncmp(buf, GET_XTRA_STATUS, strlen(GET_XTRA_STATUS)) == 0) {
            pIntClient->getXtraStatus();
        } else if (strncmp(buf, REGISTER_XTRA_STATUS_UPDATE,
                           strlen(REGISTER_XTRA_STATUS_UPDATE)) == 0) {
            bool registerUpdate = false;
            static char *save = nullptr;
            char* token = strtok_r(buf, " ", &save);
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                registerUpdate = (atoi(token) != 0);
            }
            printf("register update %d\n", registerUpdate);
            pIntClient->registerXtraStatusUpdate(registerUpdate);
        } else if (strncmp(buf, CONFIG_MERKLE_TREE, strlen(CONFIG_MERKLE_TREE)) == 0) {
            FILE *xmlFile = fopen("/etc/merkletree.xml", "rb");
            if (nullptr == xmlFile) {
                printf("failed to open merkletree config file\n");
                break;
            }
            // File opened. Read it
            fseek(xmlFile, 0, SEEK_END);
            int xmlSize = (int)ftell(xmlFile);
            fseek(xmlFile, 0, SEEK_SET);
            char* buffer = new char[xmlSize+1];
            if (buffer != nullptr) {
                fread(buffer, 1, xmlSize, xmlFile);
                buffer[xmlSize] = 0;

                printf("config merkle tree, file size %d, buffer: %p\n", xmlSize, buffer);
                pIntClient->configMerkleTree(buffer, xmlSize);
                delete[] buffer;
            }
        } else if (strncmp(buf, CONFIG_OSNMA_ENABLEMENT, strlen(CONFIG_OSNMA_ENABLEMENT)) == 0) {
            bool enable = false;
            static char *save = nullptr;
            char* token = strtok_r(buf, " ", &save);
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                enable = (atoi(token) != 0);
            }
            printf("config osnma enablement %d\n", enable);
            pIntClient->configOsnmaEnablement(enable);
        } else if (strncmp(buf, REGISTER_SIGNAL_TYPES_UPDATE,
                           strlen(REGISTER_SIGNAL_TYPES_UPDATE)) == 0) {
            bool registerUpdate = false;
            static char *save = nullptr;
            char* token = strtok_r(buf, " ", &save);
            token = strtok_r(NULL, " ", &save);
            if (token != NULL) {
                registerUpdate = (atoi(token) != 0);
            }
            printf("register GNSS signal types update %d\n", registerUpdate);
            pIntClient->registerGnssSignalTypesUpdate(registerUpdate);
        } else if (strncmp(buf, ADD_GEOFENCES,
                           strlen(ADD_GEOFENCES)) == 0) {
            printf("usage: addGeofences "
                    "[lat lon radius breachType responsiveness dwellTime ] ...\n");
            if (!pLcaClient) {
                pLcaClient = new LocationClientApi(onCapabilitiesCb);
            }
            if (pLcaClient) {
                addGeofences(buf);
            }
        } else if (strncmp(buf, PAUSE_GEOFENCES,
                           strlen(PAUSE_GEOFENCES)) == 0) {
            printf("usage: pauseGeofences gfIndex ... \n");
            if (!pLcaClient) {
                pLcaClient = new LocationClientApi(onCapabilitiesCb);
            }
            if (pLcaClient) {
                pauseGeofences(buf);
            }
        } else if (strncmp(buf, RESUME_GEOFENCES,
                           strlen(RESUME_GEOFENCES)) == 0) {
            printf("usage: resumeGeofences gfIndex ... \n");
            if (!pLcaClient) {
                pLcaClient = new LocationClientApi(onCapabilitiesCb);
            }
            if (pLcaClient) {
                resumeGeofences(buf);
            }
        } else if (strncmp(buf, REMOVE_GEOFENCES,
                           strlen(REMOVE_GEOFENCES)) == 0) {
            printf("usage: removeGeofences gfIndex ... \n");
            if (!pLcaClient) {
                pLcaClient = new LocationClientApi(onCapabilitiesCb);
            }
            if (pLcaClient) {
                removeGeofences(buf);
            }
        } else if (strncmp(buf, MODIFY_GEOFENCES,
                           strlen(MODIFY_GEOFENCES)) == 0) {
            printf("usage: modifyGeofences "
                    "[gfIndex breachType responsiveness dwellTime] ... \n");
            if (!pLcaClient) {
                pLcaClient = new LocationClientApi(onCapabilitiesCb);
            }
            if (pLcaClient) {
                modifyGeofences(buf);
            }
        } else {
            int command = buf[0];
            switch(command) {
            case 'e':
                if (!pLcaClient) {
                    pLcaClient = new LocationClientApi(onCapabilitiesCb);
                }
                if (pLcaClient) {
                    uint32_t reportType = 0x2fd;
                    uint32_t tbfMsec = 100;
                    LocReqEngineTypeMask reqEngMask = (LocReqEngineTypeMask)
                        (LOC_REQ_ENGINE_FUSED_BIT|LOC_REQ_ENGINE_SPE_BIT|
                         LOC_REQ_ENGINE_PPE_BIT);
                    getTrackingParams(buf, &reportType, &tbfMsec, &reqEngMask);
                    enginecbs = {};
                    setupEngineReportCbs(reportType, enginecbs);
                    printf("tbf %d, reprot type 0x%x, engine mask 0x%x\n",
                           tbfMsec, reportType, reqEngMask);
                    resetCounters();
                    retVal = pLcaClient->startPositionSession(100, reqEngMask,
                                                              enginecbs, onResponseCb);
                }
                break;
            case 'g':
                if (!pLcaClient) {
                    pLcaClient = new LocationClientApi(onCapabilitiesCb);
                }
                if (pLcaClient) {
                    uint32_t reportType = 0x2ff;
                    uint32_t tbfMsec = 100;
                    getTrackingParams(buf, &reportType, &tbfMsec, nullptr);
                    reportcbs = {};
                    setupGnssReportCbs(reportType, reportcbs);
                    resetCounters();
                    retVal = pLcaClient->startPositionSession(tbfMsec, reportcbs, onResponseCb);
                }
                break;
            case 'b':
                if (!pLcaClient) {
                    pLcaClient = new LocationClientApi(onCapabilitiesCb);
                }
                if (pLcaClient) {
                    int intervalmsec = 60000;
                    int distance = 0;
                    static char *save = nullptr;
                    char* token = strtok_r(buf, " ", &save); // skip first token
                    token = strtok_r(NULL, " ", &save);
                    if (token != nullptr) {
                        intervalmsec = atoi(token);
                    }
                    token = strtok_r(NULL, " ", &save);
                    if (token != nullptr) {
                        distance = atoi(token);
                    }
                    printf("start routine batching with interval %d msec, distance %d meters\n",
                           intervalmsec, distance);
                    resetCounters();
                    retVal = pLcaClient->startRoutineBatchingSession(intervalmsec, distance,
                                                                     onBatchingCb, onResponseCb);
                }
                break;
            case 'G':
                if (!pLcaClient) {
                    pLcaClient = new LocationClientApi(onCapabilitiesCb);
                }
                if (pLcaClient) {
                    geofenceTestMenu();
                }
                break;
            case 'u':
                if (!pLcaClient) {
                    pLcaClient = new LocationClientApi(onCapabilitiesCb);
                }
                if (pLcaClient) {
                    resetCounters();
                    retVal = pLcaClient->startPositionSession(2000, reportcbs, onResponseCb);
                }
                break;
            case 's':
                if (pLcaClient) {
                    pLcaClient->stopPositionSession();
                    pLcaClient->stopBatchingSession();
                    delete pLcaClient;
                    pLcaClient = NULL;
                }
                break;
            case 'm':
                if (!pLcaClient) {
                    pLcaClient = new LocationClientApi(onCapabilitiesCb);
                }
                if (pLcaClient) {
                    int i = 0;
                    do {
                        resetCounters();
                        if (i%2 == 0) {
                            pLcaClient->startPositionSession(2000, reportcbs, onResponseCb);
                        } else {
                            pLcaClient->startPositionSession(1000, reportcbs, onResponseCb);
                        }
                        i++;
                        sleep(3);
                    } while (1);
                }
                break;
            case 'r':
                if (nullptr != pLcaClient) {
                    delete pLcaClient;
                    pLcaClient = nullptr;
                }
                if (nullptr != pIntClient) {
                    delete pIntClient;
                    pIntClient = nullptr;
                }
                break;
            case 'q':
                goto EXIT;
                break;
            default:
                printf("unknown command %s\n", buf);
                break;
            }
        }
        if (retVal == false) {
            printf("command failed: %s", buf);
        }
    }//while(1)

EXIT:
    if (nullptr != pLcaClient) {
        pLcaClient->stopPositionSession();
        delete pLcaClient;
        pLcaClient = nullptr;
    }

    if (nullptr != pIntClient) {
        delete pIntClient;
        pIntClient = nullptr;
    }

    printf("Done\n");
    return 0;
}
