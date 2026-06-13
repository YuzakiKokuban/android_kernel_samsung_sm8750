/* Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 */

 /*
Changes from Qualcomm Innovation Center are provided under the following license:

Copyright (c) 2022, 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.

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
#define LOG_NDEBUG 0
#define LOG_TAG "SynergyLoc_Api"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <math.h>
#include <unistd.h>
#include <dlfcn.h>
#include <algorithm>
#include <cutils/sched_policy.h>

#include <synergy_loc_util_log.h>
#include <gps_extended.h>
#include "loc_pla.h"
#include <loc_cfg.h>
#include <LocContext.h>
#include <SynergyLocApi.h>

using namespace std;
using namespace loc_core;


#define SL_MAX_SV_CNT_SUPPORTED_IN_ONE_CONSTELLATION (64)
#define SL_NO_FEATURE_SUPPORTED (0)
#define SLL_CORE_LIB_NAME       "libloc_sll_impl.so"
#define SLL_CORE_SIM_LIB_NAME   "libloc_sll_sim.so"

#define SLL_DEFAULT_IMPL()                                    \
{                                                             \
    LOC_LOGd("%s: default implementation invoked", __func__); \
    return LOC_API_ADAPTER_ERR_UNSUPPORTED;                   \
}

typedef const SllInterfaceReq* (*get_sll_if_api_t)
            (const SllInterfaceEvent* eventCallback, void *context);

/**
   Engine Up Event, this is receive from SLL Hardware.
   This event indicates Engine is ready to accept command
   such as start, delete assistance data.

   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllEngineUp(void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportStatus(LOC_GPS_STATUS_ENGINE_ON);
    } else {
        LOC_LOGw ("Context is NULL");
    }
}


/**
   Engine Down Event, this is receive from SLL Hardware.
   This event indicates Engine is not ready.

   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllEngineDown(void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportStatus(LOC_GPS_STATUS_ENGINE_OFF);
    } else {
        LOC_LOGw ("Context is NULL");
    }
}

/**
   Position report Event, this is receive from SLL Hardware.
   This event sends Position report of SSL Hardware.

   @param UlpLocation[Input]   Position reprot.
   @param GpsLocationExtended[Input]   Extended Position reprot.
   @param loc_sess_status[Input]   Indicates Position report status
                                   as Final or intermediate
   @param LocPosTechMask[Input]         Indiacte Fix type.
   @param GnssDataNotification[Input]   Indicate Jammer and AGC info.
   @param msInWeek[Input]   Indicate time in milliseconds.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportPosition(UlpLocation& location,
                    GpsLocationExtended& locationExtended,
                    enum loc_sess_status status,
                    LocPosTechMask loc_technology_mask,
                    GnssDataNotification* pDataNotify,
                    int msInWeek, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportPosition(location, locationExtended,
                            status, loc_technology_mask, pDataNotify);
    } else {
        LOC_LOGw ("Context is NULL");
    }
}

/**
   SV Report Event, this is receive from SLL Hardware.
   This event indicates SV Info.

   @param GnssSvNotification[Input]    SV Info in track.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportSv(GnssSvNotification& svNotify, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportSv(svNotify);
    } else {
        LOC_LOGw ("Context is NULL");
    }

}

/**
   SV Measurement Event, this is receive from SLL Hardware.
   This event indicates SV Measurement.

   @param GnssSvMeasurementSet[Input]    SV Measurement
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportSvMeasurement(GnssMeasurements &svMeasurementSet, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportGnssMeasurements(svMeasurementSet);
    } else {
        LOC_LOGw ("Context is NULL");
    }
}

/**
   SV Polynomial Event, this is receive from SLL Hardware.
   This event indicates SV Polynomial.

   @param GnssSvPolynomial[Input]    SV Polynomial
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void hanldeSllReportSvPolynomial(GnssSvPolynomial &svPolynomial, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportSvPolynomial(svPolynomial);
    } else {
        LOC_LOGw ("Context is NULL");
    }
}

/**
   SV Ephemeris Event, this is receive from SLL Hardware.
   This event indicates SV Ephemeris.

   @param GnssSvEphemerisReport[Input]    SV Ephemeris
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportSvEphemeris(GnssSvEphemerisReport &svEphemeris, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportSvEphemeris(svEphemeris);
    } else {
        LOC_LOGw ("Context is NULL");
    }
}

/**
   Engine and Session Status Event, this is receive from SLL Hardware.
   This event indicates Engine and Session Status.

   @param LocGpsStatusValue[Input]    SV Ephemeris
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void hanldeSllReportStatus(LocGpsStatusValue status, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportStatus(status);
    } else {
        LOC_LOGw ("Context is NULL");
    }
}

/**
   Report NMEA String, this is receive from SLL Hardware.
   This event indicates NMEA string which generated by Hardware.

   @param nmea[Input]       NMEA Strings
   @param nmea[Input]       NMEA Strings Length
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void hanldeSllReportNmea(const char* nmea, int length, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportNmea(nmea, length);
    } else {
        LOC_LOGw ("Context is NULL");
    }

}

/**
   Report AGC and Jammer Info, this is received from SLL Hardware.
   This event indicates AGC and Jammer Info which generated by Hardware.

   @param GnssDataNotification[Input]   GNSS Data Notification.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportData(GnssDataNotification& dataNotify, int msInWeek,
    void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportData(dataNotify);
    } else {
        LOC_LOGw ("Context is NULL");
    }

}

/**
   Report XTRA Server Info, this is received from SLL Hardware.
   This event indicates XTRA Server URL info.

   @param url1[Input]    XTRA Server URL.
   @param url2[Input]    XTRA Server URL.
   @param url3[Input]    XTRA Server URL.
   @param maxlength[Input]    Max length of URL.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void hanldeSllReportXtraServer(const char* url1, const char* url2,
    const char* url3, const int maxlength, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportXtraServer(url1, url2, url3, maxlength);
    } else {
        LOC_LOGw ("Context is NULL");
    }

}

/**
   Report Location System Info, this is received from SLL Hardware.
   This event indicates LEAP second related Info.

   @param LocationSystemInfo[Input]    Leap Second related info.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportLocationSystemInfo(const LocationSystemInfo& locationSystemInfo,
    void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportLocationSystemInfo(locationSystemInfo);
    } else {
        LOC_LOGw ("Context is NULL");
    }

}

/**
   Request for XTRA Server Info, this is received from SLL Hardware.
   This event is to request to provide XTRA Server URL info.

   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllRequestXtraData(void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->requestXtraData();
    } else {
        LOC_LOGw ("Context is NULL");
    }

}

/**
   Request for Time Info, this is received from SLL Hardware.
   This event is to request to provide Time info.

   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllRequestTime(void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->requestTime();
    } else {
        LOC_LOGw ("Context is NULL");
    }

}

/**
   Request for Location Info, this is received from SLL Hardware.
   This event is to request to provide Location info.

   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllRequestLocation(void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->requestLocation();
    } else {
        LOC_LOGw ("Context is NULL");
    }
}

/**
   Request for ATL Info, this is received from SLL Hardware.
   This event is to request to provide ATL info.

   @param connHandle[Input]   Connection Handle of ATL.
   @param agps_type[Input]    AGPS Type such as SUPL/Wifi/SUPL ES
   @param apn_type_mask[Input]  APN Type.

   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllRequestATL(int connHandle, LocAGpsType agps_type,
        LocApnTypeMask apn_type_mask, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->requestATL(connHandle, agps_type, apn_type_mask);
    } else {
        LOC_LOGw ("Context is NULL");
    }

}

/**
   Request to release ATL Connection, this is received from SLL Hardware.
   This event is to request to release ATL connection.

   @param connHandle[Input]   Connection Handle of ATL.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReleaseATL(int connHandle, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->releaseATL(connHandle);
    } else {
        LOC_LOGw ("Context is NULL");
    }
}

/**
   Request to indicate NI-SUPL User notification, this is received from SLL Hardware.
   This event is to indicate NI-SUPL User notification.

   @param notify[Input]   NI SUPL User indication.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllRequestNiNotify(GnssNiNotification &notify, const void* data,
        void *context) {

    if (nullptr != context) {
        LOC_LOGd ("handleSllRequestNiNotify");
    } else {
        LOC_LOGw ("Context is NULL");
    }
}

/**
   Request to indicate measurement report, this is received from SLL Hardware.
   This event is to indicate measurement reprot.

   @param GnssMeasurementsNotification[Input]   measurement report.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportGnssMeasurementData(GnssMeasurements &measurements,
    int msInWeek, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportGnssMeasurements(measurements);
    } else {
        LOC_LOGw ("Context is NULL");
    }

}

/**
   Request to report WWAN ZPP Position report, this is received from SLL Hardware.
   This event is to report ZPP Position report.

   @param LocGpsLocation[Input]   Position report.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportWwanZppFix(LocGpsLocation &zppLoc, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportWwanZppFix(zppLoc);
    } else {
        LOC_LOGw ("Context is NULL");
    }
}

/**
   Report the Best ZPP Position, this is received from SLL Hardware.
   This event is to report Best ZPP Position.

   @param LocGpsLocation[Input]   Position report.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportZppBestAvailableFix(LocGpsLocation &zppLoc,
    GpsLocationExtended &location_extended, LocPosTechMask tech_mask, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportZppBestAvailableFix(zppLoc,
                    location_extended, tech_mask);
    } else {
        LOC_LOGw ("Context is NULL");
    }

}

/**
   Report the Black listed SV configuration, this is received from SLL Hardware.
   This event is to report Black listed SV ID.

   @param GnssSvIdConfig[Input]   Black listed SV configuration.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportGnssSvIdConfig(const GnssSvIdConfig& config, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportGnssSvIdConfig(config);
    } else {
        LOC_LOGw ("Context is NULL");
    }

}

/**
   Report the Blacklisted Constellation, this is received from SLL Hardware.
   This event is to report Blacklisted Constellation.

   @param GnssSvIdConfig[Input]   Blacklisted Constellation.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportGnssSvTypeConfig(const GnssSvTypeConfig& config, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportGnssSvTypeConfig(config);
    } else {
        LOC_LOGw ("Context is NULL");
    }

}

/**
   Request the ODCPI, this is received from SLL Hardware.
   This event is to request ODCPI.

   @param OdcpiRequestInfo[Input]   Request for ODCPI.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllRequestOdcpi(OdcpiRequestInfo& request, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->requestOdcpi(request);
    } else {
        LOC_LOGw ("Context is NULL");
    }

}

/**
   Report the Energy Consumed, this is received from SLL Hardware.
   This event is to report Engery Consumed by HW from Boot.

   @param GnssSvIdConfig[Input]   Engery Consumed Since First Boot,
                                 in units of 0.1 milli watt seconds
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportGnssEngEnergyConsumedEvent(uint64_t energyConsumedSinceFirstBoot,
    void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportGnssEngEnergyConsumedEvent(energyConsumedSinceFirstBoot);
    } else {
        LOC_LOGw ("Context is NULL");
    }

}

/**
   Report the Delete Aiding Data Event, this is received from SLL Hardware.
   This event is to report delete aiding data event.

   @param GnssAidingData[Input]   Indicate GNSS Aiding Data,
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportDeleteAidingDataEvent(GnssAidingData& aidingData, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportDeleteAidingDataEvent(aidingData);
    } else {
        LOC_LOGw ("Context is NULL");
    }
}

/**
   Report the IONO model, this is received from SLL Hardware.
   This event is to report IONO Model.

   @param GnssKlobucharIonoModel[Input]   Report IONO Model,
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportKlobucharIonoModel(GnssKlobucharIonoModel& ionoModel, void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportKlobucharIonoModel(ionoModel);

    } else {
        LOC_LOGw ("Context is NULL");
    }
}

/**
   Report the Additional System Info, this is received from SLL Hardware.
   This event is to report Additional System Info which includes system time.

   @param GnssAdditionalSystemInfo[Input]   Report Additional System Info
                                            which includes system time.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       None.

   @dependencies
       None.
*/
void handleSllReportGnssAdditionalSystemInfo(GnssAdditionalSystemInfo& additionalSystemInfo,
    void *context) {

    if (nullptr != context) {
        SynergyLocApi *synergyLocApiInstance = (SynergyLocApi*)context;
        synergyLocApiInstance->reportGnssAdditionalSystemInfo(additionalSystemInfo);
    } else {
        LOC_LOGw ("Context is NULL");
    }
}

void handleSllRequestEedGpsData(void *context) {
   (void *)(context);
   LOC_LOGw ("Default Implementation");
}

void handleSllRequestEedGloData(void *context) {
   (void *)(context);
   LOC_LOGw ("Default Implementation");
}

/* Init SLL Event Callbacks */
const SllInterfaceEvent sllEventCb = {
    handleSllEngineUp,
    handleSllEngineDown,
    handleSllReportPosition,
    handleSllReportSv,
    handleSllReportSvMeasurement,
    hanldeSllReportSvPolynomial,
    handleSllReportSvEphemeris,
    hanldeSllReportStatus,
    hanldeSllReportNmea,
    handleSllReportData,
    hanldeSllReportXtraServer,
    handleSllReportLocationSystemInfo,
    handleSllRequestXtraData,
    handleSllRequestTime,
    handleSllRequestLocation,
    handleSllRequestATL,
    handleSllReleaseATL,
    handleSllRequestNiNotify,
    handleSllReportGnssMeasurementData,
    handleSllReportWwanZppFix,
    handleSllReportZppBestAvailableFix,
    handleSllReportGnssSvIdConfig,
    handleSllReportGnssSvTypeConfig,
    handleSllRequestOdcpi,
    handleSllReportGnssEngEnergyConsumedEvent,
    handleSllReportDeleteAidingDataEvent,
    handleSllReportKlobucharIonoModel,
    handleSllReportGnssAdditionalSystemInfo,
    handleSllRequestEedGpsData,
    handleSllRequestEedGloData
};

/**
    Default Implantation of Open Command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllOpen(uint64_t requestedMask, bool isMaster,
             uint64_t *supportedMask, sllFeatureList * sllFeatures,
             void *context) {
    (void*)supportedMask;
    (void*)sllFeatures;
    (void*)context;
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of Close Command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllClose(void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of Start Session Command;
    to indicate the open command is not supported.
*/
enum loc_api_adapter_err defaultSllStartFix(sllPosMode& fixCriteria, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of Stop Session Command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllStopFix(void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of Delet Aiding Data Command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllDeleteAidingData(const GnssAidingData& data, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of Inject Position Command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllInjectPosition(const GnssLocationInfoNotification &locationInfo,
        bool onDemandCpi, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of Set Time Command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetTime(LocGpsUtcTime time, int64_t timeReference,
        int uncertainty, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of Set XTRA Data Command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetXtraData(char* data, int length, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of Request XTRA Server Command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllRequestXtraServer(void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of ATL Open Status Command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllAtlOpenStatus(int handle, int is_succ, char* apn,
            uint32_t apnLen, AGpsBearerType bear, LocAGpsType agpsType, LocApnTypeMask mask,
            void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of ATL Close Status Command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllAtlCloseStatus(int handle, int is_succ, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of Set Position Mode Command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetPositionMode(sllPosMode& posMode, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of Set Server Sync Command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetServerSync(const char* url, int len, LocServerType type,
        void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of inform NI reponse command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllInformNiResponse(GnssNiResponse userResponse,
        const void* passThroughData, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of inform NI reponse commnad;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetSUPLVersionSync
        (GnssConfigSuplVersion version, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of set NMEA Types command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetNMEATypesSync(uint32_t typesMask, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of set LPP Configuration command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetLPPConfigSync(GnssConfigLppProfileMask profileMask,
        void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of set Sensor Properties command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetSensorPropertiesSync
                       (bool gyroBiasVarianceRandomWalk_valid,
                        float gyroBiasVarianceRandomWalk,
                        bool accelBiasVarianceRandomWalk_valid,
                        float accelBiasVarianceRandomWalk,
                        bool angleBiasVarianceRandomWalk_valid,
                        float angleBiasVarianceRandomWalk,
                        bool rateBiasVarianceRandomWalk_valid,
                        float rateBiasVarianceRandomWalk,
                        bool velocityBiasVarianceRandomWalk_valid,
                        float velocityBiasVarianceRandomWalk, void *context) {
    SLL_DEFAULT_IMPL();

}

/**
    Default Implantation of set Sensor Preferred Control configure command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetSensorPerfControlConfigSync(int controlMode,
                        int accelSamplesPerBatch,
                        int accelBatchesPerSec,
                        int gyroSamplesPerBatch,
                        int gyroBatchesPerSec,
                        int accelSamplesPerBatchHigh,
                        int accelBatchesPerSecHigh,
                        int gyroSamplesPerBatchHigh,
                        int gyroBatchesPerSecHigh,
                        int algorithmConfig, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of set AGLONASS Protocol Sync command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetAGLONASSProtocolSync
        (GnssConfigAGlonassPositionProtocolMask aGlonassProtocol, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of set LPPe Protocol Control Sync command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetLPPeProtocolCpSync(GnssConfigLppeControlPlaneMask lppeCP,
        void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of set LPPe Protocol UP Sync command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetLPPeProtocolUpSync(GnssConfigLppeUserPlaneMask lppeUP,
        void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of convert SUPL Version command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllConvertSuplVersion(const uint32_t suplVersion,
        GnssConfigSuplVersion *suplConfig, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of convert LPP Profile command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllConvertLppProfile(const uint32_t lppProfile,
        GnssConfigLppProfileMask *gnssLppProfileMask, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of convert LPPe Control Plan command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllConvertLppeCp(const uint32_t lppeControlPlaneMask,
        GnssConfigLppeControlPlaneMask *gnssLppControlPlanMask, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of convert LPPe User Plan command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllConvertLppeUp(const uint32_t lppeUserPlaneMask,
        GnssConfigLppeUserPlaneMask *gnssLppUpMask, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of get WWAN ZPP Fix command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllGetWwanZppFix(void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of best Available ZPP Fix command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllGetBestAvailableZppFix(void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of Set GPS Lock Sync command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetGpsLockSync(GnssConfigGpsLock lock, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of Request for aiding data command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllRequestForAidingData(GnssAidingDataSvMask svDataMask,
        void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of Request for get GPS Lock command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllGetGpsLock(uint8_t subType, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of Request for get GPS Lock command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetXtraVersionCheckSync(uint32_t check, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of set AGPS certificate command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllInstallAGpsCert(const LocDerEncodedCertificate* pData,
                             size_t length,
                             uint32_t slotBitMask, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of set Constrained Tunc Mode command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetConstrainedTuncMode(bool enabled, float tuncConstraint,
                              uint32_t energyBudget, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of set Position Assisted Clock Estimator Mode command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllSetPositionAssistedClockEstimatorMode(bool enabled,
                             void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of get GNSS Energy Consumed command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultSllGetGnssEnergyConsumed(void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of set Black List SV command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultsllSetBlacklistSv(const GnssSvIdConfig& config, void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of get Black List SV command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultsllGetBlacklistSv(void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of set constellation Control command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultsllSetConstellationControl(const GnssSvTypeConfig& config,
        void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of get constellation Control command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultsllGetConstellationControl(void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of reset constellation Control command;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultsllResetConstellationControl(void *context) {
    SLL_DEFAULT_IMPL();
}

/**
    Default Implantation of request XTRA configuration Info;
    to indicate the command is not supported.
*/
enum loc_api_adapter_err defaultsllRequestXtraConfigInfo(const uint32_t configMask,
                                         const uint32_t configReqSource, void *context) {
    SLL_DEFAULT_IMPL();
}

/* Default Implantation of SLL Commands */
const SllInterfaceReq sllDefultReq = {
    defaultSllOpen,
    defaultSllClose,
    defaultSllStartFix,
    defaultSllStopFix,
    defaultSllDeleteAidingData,
    defaultSllInjectPosition,
    defaultSllSetTime,
    defaultSllSetXtraData,
    defaultSllRequestXtraServer,
    defaultSllAtlOpenStatus,
    defaultSllAtlCloseStatus,
    defaultSllSetPositionMode,
    defaultSllSetServerSync,
    defaultSllInformNiResponse,
    defaultSllSetSUPLVersionSync,
    defaultSllSetNMEATypesSync,
    defaultSllSetLPPConfigSync,
    defaultSllSetSensorPropertiesSync,
    defaultSllSetSensorPerfControlConfigSync,
    defaultSllSetAGLONASSProtocolSync,
    defaultSllSetLPPeProtocolCpSync,
    defaultSllSetLPPeProtocolUpSync,
    defaultSllConvertSuplVersion,
    defaultSllConvertLppProfile,
    defaultSllConvertLppeCp,
    defaultSllConvertLppeUp,
    defaultSllGetWwanZppFix,
    defaultSllGetBestAvailableZppFix,
    defaultSllSetGpsLockSync,
    defaultSllRequestForAidingData,
    defaultSllGetGpsLock,
    defaultSllSetXtraVersionCheckSync,
    defaultSllInstallAGpsCert,
    defaultSllSetConstrainedTuncMode,
    defaultSllSetPositionAssistedClockEstimatorMode,
    defaultSllGetGnssEnergyConsumed,
    defaultsllSetBlacklistSv,
    defaultsllGetBlacklistSv,
    defaultsllSetConstellationControl,
    defaultsllGetConstellationControl,
    defaultsllResetConstellationControl,
    defaultsllRequestXtraConfigInfo
};

/* Constructor for SynergyLocApi */
SynergyLocApi::SynergyLocApi(LOC_API_ADAPTER_EVENT_MASK_T exMask,
                       ContextBase* context) :
    LocApiBase(exMask, context),
    mSlMask(0), mInSession(false), mPowerMode(GNSS_POWER_MODE_DEFAULT),
    mEngineOn(false), mMeasurementsStarted(false),
    mIsMasterRegistered(false), mMasterRegisterNotSupported(false),
    mSvMeasurementSet(nullptr)
{
    const char * libName = nullptr;
    void *handle = nullptr;
    int isSllSimEnabled = 0;
    const char *error = nullptr;

    loc_param_s_type gps_conf_param_table[] =
    {
        {"IS_SLL_SIM_ENABLED", &isSllSimEnabled, NULL, 'n'}
    };

    UTIL_READ_CONF(LOC_PATH_GPS_CONF, gps_conf_param_table);

    if (isSllSimEnabled) {
        libName = SLL_CORE_SIM_LIB_NAME;
    } else {
        libName = SLL_CORE_LIB_NAME;
    }

    if ((handle = dlopen(libName, RTLD_NOW)) != nullptr) {
        LOC_LOGv("%s is present", libName);
        get_sll_if_api_t getter = (get_sll_if_api_t)dlsym(handle, "get_sll_if_api");

        if ((error = dlerror()) != nullptr) {
            LOC_LOGe("dlsym for %s get_sll_if_api failed, error = %s", libName, error);
            getter = nullptr;
        }

        if (getter != nullptr) {
            sllReqIf = (getter)(&sllEventCb, ((void *)this));
            if (sllReqIf != nullptr) {
                return;
            } else {
                LOC_LOGe("%s SLL lib provided Command Interface as NULL", libName);
            }
        }
    } else {
        char *errorDll = dlerror();

        LOC_LOGe("dlopen for %s failed, handle %p error: %s", libName, handle,
             ((nullptr != errorDll) ? errorDll : "No Error"))
    }

    sllReqIf = &sllDefultReq;

}

/* Destructor for SynergyLocApi */
SynergyLocApi::~SynergyLocApi()
{
    close();
}

/**
   The API to Call of Synergy Location Layer.

   @param LOC_API_ADAPTER_EVENT_MASK_T[Input]  Configure Event Mask.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       LocApiBase: Pointer of Constructor of Synergy Location API.

   @dependencies
       None.
*/
LocApiBase* getLocApi(LOC_API_ADAPTER_EVENT_MASK_T exMask,
                      ContextBase* context)
{
    return (LocApiBase*)SynergyLocApi::createSynergyLocApi(exMask, context);
}


/**
   The API to call Constructor of Synergy Location API.

   @param LOC_API_ADAPTER_EVENT_MASK_T[Input]  Configure Event Mask.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       LocApiBase: Pointer of Constructor of Synergy Location API.

   @dependencies
       None.
*/
LocApiBase* SynergyLocApi::createSynergyLocApi(LOC_API_ADAPTER_EVENT_MASK_T exMask,
                                       ContextBase* context)
{
    return new SynergyLocApi(exMask, context);
}


/**
   Initialize a synergy loc api client AND
   check which loc message are supported by SLL.

   @param LOC_API_ADAPTER_EVENT_MASK_T[Input]  Configure Event Mask.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
       loc_api_adapter_err.

   @dependencies
       None.
*/
enum loc_api_adapter_err
SynergyLocApi::open(LOC_API_ADAPTER_EVENT_MASK_T mask) {
    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_UNKNOWN;
    uint64_t supportedMask = 0;
    uint64_t requestedMask = (uint64_t)mask;
    bool gnssMeasurementSupported = false;
    bool isMasterSession = false;
    sllFeatureList  getSllFeatures;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllOpen)) {
            //Check for Master and Measurement
        if (isMaster()) {
            isMasterSession = true;
        }

        memset(&getSllFeatures, 0, sizeof(getSllFeatures));
        rtv = sllReqIf->sllOpen(requestedMask, isMasterSession, &supportedMask,
                    &getSllFeatures, (void *)this);
        if (((LOC_API_ADAPTER_ERR_SUCCESS == rtv) ||
            (LOC_API_ADAPTER_ERR_UNSUPPORTED == rtv)) &&
            (SL_NO_FEATURE_SUPPORTED != supportedMask)) {
                LOC_LOGd ("SLL Requested Configuration 0x%" PRIx64 ","
                        " Supported Configuration 0x%" PRIx64 "",
                        mask, supportedMask);
                if ((LOC_API_ADAPTER_BIT_GNSS_MEASUREMENT ==
                        (supportedMask & LOC_API_ADAPTER_BIT_GNSS_MEASUREMENT)) ||
                    (LOC_API_ADAPTER_BIT_GNSS_MEASUREMENT_REPORT ==
                        (supportedMask & LOC_API_ADAPTER_BIT_GNSS_MEASUREMENT))) {
                    gnssMeasurementSupported = true;
                }
            mContext->setEngineCapabilities(supportedMask, (getSllFeatures.feature_len != 0 ?
                getSllFeatures.feature : NULL), gnssMeasurementSupported);
        }
    }
    return rtv;
}

/**
   Register Event Mask

   @param LOC_API_ADAPTER_EVENT_MASK_T[Input]  Configure Event Mask.
   @param context[Input]    Context Pointer of Synergy Location API.

   @return
        None.

   @dependencies
       None.
*/
void SynergyLocApi::registerEventMask(LOC_API_ADAPTER_EVENT_MASK_T adapterMask) {

    mMask = adapterMask;
    return;
}

/**
   Request to Close SLL Hardware

   @param
        none

   @return
       loc_api_adapter_err.

   @dependencies
       None.
*/
enum loc_api_adapter_err SynergyLocApi::close() {

    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllClose)) {
        rtv = sllReqIf->sllClose((void *)this);
        if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
            LOC_LOGw ("Close Error 0x%x:", rtv);
        }
    } else {
        rtv = LOC_API_ADAPTER_ERR_UNKNOWN;
    }
    return rtv;
}

/**
   set the positioning fix criteria

   @param LocPosMode[Input]  Session configuration paramter.

   @return
        None.

   @dependencies
       None.
*/
void SynergyLocApi::setPositionMode(const LocPosMode& posMode) {

    sendMsg(new LocApiMsg([this, posMode] () {
        LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
        enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;
        sllPosMode LocSlPosMode;

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetPositionMode)) {
            LocSlPosMode.mode = posMode.mode;
            LocSlPosMode.recurrence = posMode.recurrence;
            LocSlPosMode.min_interval = posMode.min_interval;
            LocSlPosMode.preferred_accuracy = posMode.preferred_accuracy;
            LocSlPosMode.preferred_time = posMode.preferred_time;
            LocSlPosMode.share_position = posMode.share_position;
            LocSlPosMode.powerMode = posMode.powerMode;
            LocSlPosMode.timeBetweenMeasurements = posMode.timeBetweenMeasurements;

            rtv = sllReqIf->sllSetPositionMode(LocSlPosMode, ((void *)this));
            if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
                err = LOCATION_ERROR_SUCCESS;
            }
        } else {
            err = LOCATION_ERROR_NOT_SUPPORTED;
        }
    }));
}

/**
   Inject time into the position engine

   @param LocGpsUtcTime[Input]  Location GPS UTC Time.
   @param timeReference[Input]  Reference Time.
   @param uncertainty[Input]    Time Uncertainty.
                                Units: Milliseconds.

   @return
        None.

   @dependencies
       None.
*/
void SynergyLocApi::
    setTime(LocGpsUtcTime time, int64_t timeReference, int uncertainty) {

    sendMsg(new LocApiMsg([this, time, timeReference, uncertainty] () {

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetTime)) {
            enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;
            rtv = sllReqIf->sllSetTime(time, timeReference, uncertainty, ((void *)this));
            if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
                LOC_LOGe ("Error: %d", rtv);
            }
        }

    }));

}

/**
   inject position into the position engine

   @param Location[Input]   Position report.
   @param onDemandCpi[Input]  Indicate CPI demanded or not.

   @return
        None.

   @dependencies
       None.
*/
void SynergyLocApi::injectPosition(const Location& location, bool onDemandCpi) {

    sendMsg(new LocApiMsg([this, location, onDemandCpi] () {

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllInjectPosition)) {
            enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;
            GnssLocationInfoNotification locInfo;
            locInfo.location = location;
            rtv = sllReqIf->sllInjectPosition(locInfo, onDemandCpi, ((void *)this));
            if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
                LOC_LOGe ("Error: %d", rtv);
            }
        }

    }));

}

/**
    This API provides number of SV used in the fix

   @param LocGpsUtcTime[Input]  Location GPS UTC Time.
   @param timeReference[Input]  Reference Time.
   @param uncertainty[Input]    Time Uncertainty.
                                Units: Milliseconds.

   @return
        None.

   @dependencies
       None.
*/
int SynergyLocApi::getNumSvUsed (uint64_t svUsedIdsMask, int totalSvCntInOneConstellation) {

    if (totalSvCntInOneConstellation > SL_MAX_SV_CNT_SUPPORTED_IN_ONE_CONSTELLATION) {
        LOC_LOGe ("error: total SV count in one constellation %d exceeded limit of 64",
                  totalSvCntInOneConstellation);
        return 0;
    }

    int numSvUsed = 0;
    uint64_t mask = 0x1;
    for (int i = 0; i < totalSvCntInOneConstellation; i++) {
        if (svUsedIdsMask & mask) {
            numSvUsed++;
        }
        mask <<= 1;
    }

    return numSvUsed;
}


/**
    This API provides Position report

   @param GnssLocationInfoNotification[Input]  recevied Position report.
   @param onDemandCpi[Input]  Indicate CPI demanded or not.

   @return
        None.

   @dependencies
       None.
*/
void SynergyLocApi::injectPosition(const GnssLocationInfoNotification &locationInfo,
    bool onDemandCpi) {

    sendMsg(new LocApiMsg([this, locationInfo, onDemandCpi] () {
        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllInjectPosition)){
            enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;
            rtv = sllReqIf->sllInjectPosition(locationInfo, onDemandCpi, ((void *)this));
            if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
                LOC_LOGe ("Error: %d", rtv);
            }
        }

    }));
}



/**
    This API provides delete assistance data

   @param GnssAidingData[Input]  GNSS Aiding Data.
   @param LocApiResponse[Input]  Adapter Response.

   @return
        None.

   @dependencies
       None.
*/
void
SynergyLocApi::deleteAidingData(const GnssAidingData& data, LocApiResponse *adapterResponse) {

    sendMsg(new LocApiMsg([this, data, adapterResponse] () {
        LocationError err = LOCATION_ERROR_GENERAL_FAILURE;

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllDeleteAidingData)) {
            enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;
            rtv = sllReqIf->sllDeleteAidingData(data, ((void *)this));
            if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
                err = LOCATION_ERROR_SUCCESS;
            }

        } else {
            err = LOCATION_ERROR_NOT_SUPPORTED;
        }
        if (adapterResponse != NULL) {
            adapterResponse->returnToSender(err);
        }
    }));
}


/**
   This API provides NI user response to the engine

   @param GnssNiResponse[Input]  GNSS NI Response.
   @param LocApiResponse[Input]  Pass Through Data.

   @return
        None.

   @dependencies
       None.
*/
void
SynergyLocApi::informNiResponse(GnssNiResponse userResponse, const void* passThroughData) {

    sendMsg(new LocApiMsg([this, userResponse, passThroughData] () {

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllInformNiResponse)) {
            enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;
            rtv = sllReqIf->sllInformNiResponse(userResponse, passThroughData, ((void *)this));
            if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
                LOC_LOGe ("Error: %d", rtv);
            }
        }
        free((void *)passThroughData);
    }));

}


/**
   This API provides Set UMTs SLP server URL

   @param const char*[Input]   Server URL Configuration.
   @param Length[Input]        Server URL Lenght
   @param LocServerType[Input] Server Type

   @return
        LocationError.

   @dependencies
       None.
*/
LocationError
SynergyLocApi::setServerSync(const char* url, int len, LocServerType type) {

    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;
    LocationError err = LOCATION_ERROR_SUCCESS;
    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetServerSync)) {
        rtv = sllReqIf->sllSetServerSync (url, len, type, ((void *)this));
        if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }
    } else {
        err = LOCATION_ERROR_CALLBACK_MISSING;
    }

    return err;
}

/**
   This API configure Server using IP Address and Port Number

   @param unsigned int ip[Input]    Server IP Configuration.
   @param int port[Input]           Port number
   @param LocServerType[Input]      Server Type

   @return
        LocationError.

   @dependencies
       None.
*/
LocationError
SynergyLocApi::setServerSync(unsigned int ip, int port, LocServerType type) {

    return LOCATION_ERROR_CALLBACK_MISSING;
}


/**
   This API request to OPEN ATL Status

   @param handle[Input]     ATL Connection handle
   @param is_succ[Input]    ATL connection status
   @param APN[Input]        ATL APN buffer
   @param AGpsBearerType[Input]   APN Type
   @param LocAGpsType[Input]   AGPS Type
   @param LocApnTypeMask[Input]   AGPS Configuration Mask

   @return
        None.

   @dependencies
       None.
*/
void SynergyLocApi::atlOpenStatus(
  int handle, int is_succ, char* apn, uint32_t apnLen, AGpsBearerType bear,
  LocAGpsType agpsType, LocApnTypeMask apnTypeMask) {

    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllAtlOpenStatus)) {

        rtv= sllReqIf->sllAtlOpenStatus(handle, is_succ, apn, apnLen, bear, agpsType,
                apnTypeMask, (void *)this);
        if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
             LOC_LOGe ("Error: %d", rtv);
        }

    } else {
        rtv = LOC_API_ADAPTER_ERR_UNSUPPORTED;
    }

    LOC_LOGd("Error: %d", rtv);

    return;
}


/**
   This API request to close ATL connection

   @param handle[Input]     ATL Connection handle
   @param is_succ[Input]    ATL connection status

   @return
        None.

   @dependencies
       None.
*/
void SynergyLocApi::atlCloseStatus(int handle, int is_succ) {
    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllAtlCloseStatus)) {

        rtv= sllReqIf->sllAtlCloseStatus(handle, is_succ, (void *)this);
        if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
             LOC_LOGe ("Error: %d", rtv);
        }
    }

    return;
}


/**
   set the SUPL version

   @param GnssConfigSuplVersion[Input]     ATL Connection handle
   @param is_succ[Input]    ATL connection status

   @return
        LocationError.

   @dependencies
       None.
*/
LocationError
SynergyLocApi::setSUPLVersionSync(GnssConfigSuplVersion version) {

    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetSUPLVersionSync)) {

        rtv = sllReqIf->sllSetSUPLVersionSync(version, ((void *)this));
        if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
            err = LOCATION_ERROR_SUCCESS;
        }
    } else {
        err = LOCATION_ERROR_NOT_SUPPORTED;
    }

    return err;
}

/**
   set the NMEA types mask

   @param typesMask[Input]    Set NMEA Mask

   @return
        enum loc_api_adapter_err

   @dependencies
       None.
*/
enum loc_api_adapter_err SynergyLocApi::setNMEATypesSync(uint32_t typesMask) {

    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetNMEATypesSync)) {
        rtv= sllReqIf->sllSetNMEATypesSync(typesMask, (void *)this);
        if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
             LOC_LOGe ("Error: %d", rtv);
        }

    } else {
        rtv = LOC_API_ADAPTER_ERR_UNSUPPORTED;
    }

    return rtv;
}

/**
   set the configuration for LTE positioning profile (LPP).

   @param GnssConfigLppProfileMask[Input]    Configur LPP Profile.

   @return
        LocationError

   @dependencies
       None.
*/
LocationError
SynergyLocApi::setLPPConfigSync(GnssConfigLppProfileMask profileMask) {

    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;

    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetLPPConfigSync)) {

        rtv = sllReqIf->sllSetLPPConfigSync(profileMask, ((void *)this));
        if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
            err = LOCATION_ERROR_SUCCESS;
        }
    } else {
        err = LOCATION_ERROR_NOT_SUPPORTED;
    }

    return err;
}


/**
   Set the Sensor Properties configuration.

   @param gyroBiasVarianceRandomWalk_valid[Input]   Is vaild Gyro Bias Varianace
   @param gyroBiasVarianceRandomWalk[Input]         Gyro Bias Variance.
   @param accelBiasVarianceRandomWalk_valid[Input]  Is vaild Accel Bias Varianace
   @param accelBiasVarianceRandomWalk[Input]        Accel Bias Variance.
   @param angleBiasVarianceRandomWalk_valid[Input]  Is vaild Angle Bias Varianace
   @param angleBiasVarianceRandomWalk[Input]        Angle Bias Variance.
   @param velocityBiasVarianceRandomWalk_valid[Input]   Is vaild Velocity Bias Varianace
   @param velocityBiasVarianceRandomWalk[Input]         Velocity Bias Variance.

   @return
        enum loc_api_adapter_err

   @dependencies
       None.
*/
enum loc_api_adapter_err SynergyLocApi::setSensorPropertiesSync(
        bool gyroBiasVarianceRandomWalk_valid, float gyroBiasVarianceRandomWalk,
        bool accelBiasVarianceRandomWalk_valid, float accelBiasVarianceRandomWalk,
        bool angleBiasVarianceRandomWalk_valid, float angleBiasVarianceRandomWalk,
        bool rateBiasVarianceRandomWalk_valid, float rateBiasVarianceRandomWalk,
        bool velocityBiasVarianceRandomWalk_valid, float velocityBiasVarianceRandomWalk) {

    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetSensorPropertiesSync)) {

        rtv= sllReqIf->sllSetSensorPropertiesSync(gyroBiasVarianceRandomWalk_valid,
                            gyroBiasVarianceRandomWalk,
                            accelBiasVarianceRandomWalk_valid,
                            accelBiasVarianceRandomWalk,
                            angleBiasVarianceRandomWalk_valid,
                            angleBiasVarianceRandomWalk,
                            rateBiasVarianceRandomWalk_valid,
                            rateBiasVarianceRandomWalk,
                            velocityBiasVarianceRandomWalk_valid,
                            velocityBiasVarianceRandomWalk,
                            (void *)this);
        if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
             LOC_LOGe ("Error: %d", rtv);
        }
    } else {
        rtv = LOC_API_ADAPTER_ERR_UNSUPPORTED;
    }
    return rtv;

}


/**
   Set the Sensor Performance Configuration.

   @param controlMode[Input]   Control Mode
   @param accelSamplesPerBatch[Input]       Accel Sample Per Batch.
   @param accelBatchesPerSec[Input]         Accel Batch Per Seconds.
   @param gyroSamplesPerBatch[Input]        Gyro Sample Per Batch.
   @param gyroBatchesPerSec[Input]          Gyro Batch Per Seconds.
   @param accelSamplesPerBatchHigh[Input]   Max Accel Sample Per Batches.
   @param accelBatchesPerSecHigh[Input]     Max Accel Batches Per Second.
   @param gyroSamplesPerBatchHigh[Input]    Max Gyro Sample Per Batches.
   @param gyroBatchesPerSecHigh[Input]      Max Gyro Batches Per Second.
   @param algorithmConfig[Input]            Configure Algorithm.

   @return
        enum loc_api_adapter_err

   @dependencies
       None.
*/
enum loc_api_adapter_err SynergyLocApi::setSensorPerfControlConfigSync(int controlMode,
        int accelSamplesPerBatch, int accelBatchesPerSec,
        int gyroSamplesPerBatch, int gyroBatchesPerSec,
        int accelSamplesPerBatchHigh, int accelBatchesPerSecHigh,
        int gyroSamplesPerBatchHigh, int gyroBatchesPerSecHigh,
        int algorithmConfig) {

    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetSensorPerfControlConfigSync)) {

        rtv= sllReqIf->sllSetSensorPerfControlConfigSync(controlMode,
                            accelSamplesPerBatch,
                            accelBatchesPerSec,
                            gyroSamplesPerBatch,
                            gyroBatchesPerSec,
                            accelSamplesPerBatchHigh,
                            accelBatchesPerSecHigh,
                            gyroSamplesPerBatchHigh,
                            gyroBatchesPerSecHigh,
                            algorithmConfig,
                            (void *)this);
        if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
             LOC_LOGe ("Error: %d", rtv);
        }
    } else {
        rtv = LOC_API_ADAPTER_ERR_UNSUPPORTED;
    }
    return rtv;

}


/**
   Set the Positioning Protocol on A-GLONASS system.

   @param GnssConfigAGlonassPositionProtocolMask[Input]   AGLONASS Protocol configure

   @return
        LocationError

   @dependencies
       None.
*/
LocationError
SynergyLocApi::setAGLONASSProtocolSync(GnssConfigAGlonassPositionProtocolMask aGlonassProtocol) {

    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetAGLONASSProtocolSync)) {

        rtv = sllReqIf->sllSetAGLONASSProtocolSync(aGlonassProtocol, ((void *)this));
        if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
            err = LOCATION_ERROR_SUCCESS;
        }
    } else {
        err = LOCATION_ERROR_NOT_SUPPORTED;
    }

    return err;
}


/**
   Set the LPPe Protocol control plan synchronize.

   @param GnssConfigLppeControlPlaneMask[Input]   GNSS Configure Control Plan Mask.

   @return
        LocationError

   @dependencies
       None.
*/
LocationError
SynergyLocApi::setLPPeProtocolCpSync(GnssConfigLppeControlPlaneMask lppeCP) {

    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;


    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetLPPeProtocolCpSync)) {

        rtv = sllReqIf->sllSetLPPeProtocolCpSync(lppeCP, ((void *)this));
        if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
            err = LOCATION_ERROR_SUCCESS;
        }
    } else {
        err = LOCATION_ERROR_NOT_SUPPORTED;
    }

    return err;
}


/**
   Set the LPPe Protocol User plan synchronize.

   @param GnssConfigLppeUserPlaneMask[Input]   GNSS Configure User Plan Mask.

   @return
        LocationError

   @dependencies
       None.
*/
LocationError
SynergyLocApi::setLPPeProtocolUpSync(GnssConfigLppeUserPlaneMask lppeUP) {

    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetLPPeProtocolUpSync)) {

        rtv = sllReqIf->sllSetLPPeProtocolUpSync(lppeUP, ((void *)this));
        if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
            err = LOCATION_ERROR_SUCCESS;
        }
    } else {
        err = LOCATION_ERROR_NOT_SUPPORTED;
    }

    return err;

}


/**
   Request for WWAN ZPP Fix- Position reprot.

   @param
        None

   @return
        None

   @dependencies
       None.
*/
void SynergyLocApi::getWwanZppFix() {

    sendMsg(new LocApiMsg([this] () {
        enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllGetWwanZppFix)) {
            rtv= sllReqIf->sllGetWwanZppFix((void *)this);
            if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
                 LOC_LOGe("Error: %d", rtv);
            }
        } else {
            rtv = LOC_API_ADAPTER_ERR_UNSUPPORTED;
        }
        LOC_LOGd("Status: %d", rtv);
    }));

}

/**
   Request for Best Available ZPP Fix- Position reprot.

   @param
        None

   @return
        None

   @dependencies
       None.
*/
void SynergyLocApi::getBestAvailableZppFix() {

    sendMsg(new LocApiMsg([this] () {
        enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllGetWwanZppFix)) {
            rtv= sllReqIf->sllGetWwanZppFix((void *)this);
            if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
                 LOC_LOGe("Error: %d", rtv);
            }
        } else {
            rtv = LOC_API_ADAPTER_ERR_UNSUPPORTED;
        }
        LOC_LOGd("Status: %d", rtv);
    }));

}


/**
   Set GPS Lock.

   @param GnssConfigGpsLock[Input]  Configure GPS Lock

   @return
        LocationError

   @dependencies
       None.
*/
LocationError SynergyLocApi::setGpsLockSync(GnssConfigGpsLock lock) {

    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetGpsLockSync)) {

        rtv = sllReqIf->sllSetGpsLockSync(lock, ((void *)this));
        if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
            err = LOCATION_ERROR_SUCCESS;
        }
    } else {
        err = LOCATION_ERROR_NOT_SUPPORTED;
    }

    return err;
}


/**
   Configure Aiding Data

   @param GnssAidingDataSvMask[Input]  GNSS Aiding Data SV Mask

   @return
        None.

   @dependencies
       None.
*/
void SynergyLocApi::requestForAidingData(GnssAidingDataSvMask svDataMask) {

    sendMsg(new LocApiMsg([this, svDataMask] () {

        enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllRequestForAidingData)) {

            rtv = sllReqIf->sllRequestForAidingData(svDataMask, ((void *)this));
            if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
               LOC_LOGe ("Error: %d", rtv);
            }
        } else {
            rtv = LOC_API_ADAPTER_ERR_UNSUPPORTED;
        }
    }));
}


/**
   Check XTRA Version Check

   @param uint32_t[Input]  XTRA Version Check

   @return
        LocationError.

   @dependencies
       None.
*/
LocationError
SynergyLocApi::setXtraVersionCheckSync(uint32_t check) {

    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetXtraVersionCheckSync)) {

        rtv = sllReqIf->sllSetXtraVersionCheckSync(check, ((void *)this));
        if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
            err = LOCATION_ERROR_SUCCESS;
        }
    } else {
        err = LOCATION_ERROR_NOT_SUPPORTED;
    }
    return err;
}


/**
   Set Constrained Tunc Mode

   @param enabled[Input]                     Enabled OR Disabled
   @param tuncConstraint[Input]              Number of AGPS Certificates
   @param energyBudget[Input]                Bit mask of Certificates

   @return
        None.

   @dependencies
       None.
*/
void SynergyLocApi::setConstrainedTuncMode(bool enabled,
                                           float tuncConstraint,
                                           uint32_t energyBudget,
                                           LocApiResponse* adapterResponse) {

    sendMsg(new LocApiMsg([this, enabled, tuncConstraint, energyBudget, adapterResponse] () {
    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetConstrainedTuncMode)) {

        rtv = sllReqIf->sllSetConstrainedTuncMode(enabled, tuncConstraint,
                    energyBudget, ((void *)this));
        if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
            err = LOCATION_ERROR_SUCCESS;
        }
    } else {
        err = LOCATION_ERROR_NOT_SUPPORTED;
    }
    if (adapterResponse != NULL) {
        adapterResponse->returnToSender(err);
    }
    }));
}


/**
   Enable Position Assisted Clock Estimator Mode

   @param enabled[Input]    Enable or disable Clock estimator.

   @return
        LocationError.

   @dependencies
       None.
*/
void SynergyLocApi::setPositionAssistedClockEstimatorMode(bool enabled,
                                                          LocApiResponse* adapterResponse) {
    sendMsg(new LocApiMsg([this, enabled, adapterResponse] () {

    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) &&
            (nullptr != sllReqIf->sllSetPositionAssistedClockEstimatorMode)) {
        rtv = sllReqIf->sllSetPositionAssistedClockEstimatorMode(enabled, ((void *)this));
        if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
            err = LOCATION_ERROR_SUCCESS;
        }
    } else {
        err = LOCATION_ERROR_NOT_SUPPORTED;
    }

    if (adapterResponse != NULL) {
            adapterResponse->returnToSender(err);
        }

    }));
}


/**
   Get GNSS Engery Consumed

   @param
        None.

   @return
        LocationError.

   @dependencies
        None.
*/
void SynergyLocApi::getGnssEnergyConsumed() {

    sendMsg(new LocApiMsg([this] {

    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllGetGnssEnergyConsumed)) {
        rtv = sllReqIf->sllGetGnssEnergyConsumed((void *)this);
        if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
            err = LOCATION_ERROR_SUCCESS;
        }
    } else {
        err = LOCATION_ERROR_NOT_SUPPORTED;
    }

    }));
}


/**
    Convert SUPL Version into GNSS SUPL Version format

    @param suplVersion[Input]     SUPL Version

    @return
        GnssConfigSuplVersion.

    @dependencies
        None.
*/
GnssConfigSuplVersion
SynergyLocApi::convertSuplVersion(const uint32_t suplVersion) {

    GnssConfigSuplVersion configSuplVersion = GNSS_CONFIG_SUPL_VERSION_1_0_0;
    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllConvertSuplVersion)) {
        rtv = sllReqIf->sllConvertSuplVersion(suplVersion, &configSuplVersion, (void *)this);
        if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
            configSuplVersion = GNSS_CONFIG_SUPL_VERSION_1_0_0;
        }
    }

    return configSuplVersion;
}


/**
    Convert LPPe Control Plan Profile vesrion in GNSS LPP Control Plan profile format


    @param lppeControlPlaneMask[Input]     LPPe Control Plan Mask


    @return
        GnssConfigLppeControlPlaneMask.

    @dependencies
        None.
*/
GnssConfigLppeControlPlaneMask
SynergyLocApi::convertLppeCp(const uint32_t lppeControlPlaneMask) {
    GnssConfigLppeControlPlaneMask configLppeCp = 0;
    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllConvertLppeCp)) {
        rtv = sllReqIf->sllConvertLppeCp(lppeControlPlaneMask, &configLppeCp, (void *)this);
        if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
            configLppeCp = 0;
        }
    }

    return configLppeCp;
}


/**
   Convert LPPe Control Plan Profile vesrion in GNSS LPP User Plan profile format

    @param lppeUserPlaneMask[Input]     LPPe User Plan Mask


    @return
        GnssConfigLppeUserPlaneMask.

    @dependencies
        None.
*/
GnssConfigLppeUserPlaneMask
SynergyLocApi::convertLppeUp(const uint32_t lppeUserPlaneMask) {
    GnssConfigLppeUserPlaneMask configLppeUp = 0;
    enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllConvertLppeUp)) {
        rtv = sllReqIf->sllConvertLppeUp(lppeUserPlaneMask, &configLppeUp, (void *)this);
        if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
            configLppeUp = 0;
        }
    }
    return configLppeUp;
}


/**
   Set Black listed SV.

    @param GnssSvIdConfig[Input]     Black listed SV ID


    @return
        LocationError.

    @dependencies
        None.
*/
LocationError
SynergyLocApi::setBlacklistSvSync(const GnssSvIdConfig& config) {

    LocationError rtv = LOCATION_ERROR_SUCCESS;
    enum loc_api_adapter_err adapRtv = LOC_API_ADAPTER_ERR_SUCCESS;

    if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetBlacklistSv)) {
        adapRtv = sllReqIf->sllSetBlacklistSv(config, ((void *)this));
        if (LOC_API_ADAPTER_ERR_SUCCESS != adapRtv) {
           LOC_LOGe ("Error: %d", adapRtv);
           rtv = LOCATION_ERROR_GENERAL_FAILURE;
        }
    } else {
        rtv = LOCATION_ERROR_NOT_SUPPORTED;
    }

    return rtv;
}


/**
   Set Black listed SV.

    @param GnssSvIdConfig[Input]     Black listed SV ID


    @return
        None.

    @dependencies
        None.
*/
void
SynergyLocApi::setBlacklistSv(const GnssSvIdConfig& config, LocApiResponse* adapterResponse) {
    sendMsg(new LocApiMsg([this, config, adapterResponse] () {
        LocationError err = setBlacklistSvSync(config);
        if (adapterResponse) {
            adapterResponse->returnToSender(err);
        }
    }));
}


/**
   Get Black listed SV.

   @param
        None.


   @return
        None.

   @dependencies
        None.
*/
void SynergyLocApi::getBlacklistSv() {
    sendMsg(new LocApiMsg([this] () {
        enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllGetBlacklistSv)) {
            rtv = sllReqIf->sllGetBlacklistSv((void *)this);
            if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
               LOC_LOGe ("Error: %d", rtv);
            }
        }
    }));
}


/**
   Configure Constellation Control.

   @param   GnssSvTypeConfig[Input]  GNSS Constellation configuration.


   @return
        None.

   @dependencies
        None.
*/
void
SynergyLocApi::setConstellationControl(const GnssSvTypeConfig& config,
                                       LocApiResponse *adapterResponse) {

    sendMsg(new LocApiMsg([this, config, adapterResponse] () {
        LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
        enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllSetConstellationControl)) {
            rtv = sllReqIf->sllSetConstellationControl(config, (void *)this);
            if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
                err = LOCATION_ERROR_SUCCESS;
            } else {
               LOC_LOGe ("Error: %d", rtv);
            }
        }
        if (adapterResponse != NULL) {
            adapterResponse->returnToSender(err);
        }
    }));
}

/**
   Request get configured Constellation from Hardware.

   @param
        None.

   @return
        None.

   @dependencies
        None.
*/
void
SynergyLocApi::getConstellationControl() {

    sendMsg(new LocApiMsg([this] () {
        enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllGetConstellationControl)) {
            rtv = sllReqIf->sllGetConstellationControl((void *)this);
            if (LOC_API_ADAPTER_ERR_SUCCESS != rtv) {
               LOC_LOGe ("Error: %d", rtv);
            }
        }
    }));
}


/**
   Request to reset configured Constellation from Hardware.

   @param
        None.

   @return
        None.

   @dependencies
        None.
*/
void
SynergyLocApi::resetConstellationControl(LocApiResponse *adapterResponse) {
    sendMsg(new LocApiMsg([this, adapterResponse] () {
        enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;
        LocationError err = LOCATION_ERROR_GENERAL_FAILURE;

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllResetConstellationControl)) {
            rtv = sllReqIf->sllResetConstellationControl((void *)this);
            if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
                err = LOCATION_ERROR_SUCCESS;
            } else {
               LOC_LOGe ("Error: %d", rtv);
            }
        }
        if (adapterResponse != NULL) {
            adapterResponse->returnToSender(err);
        }
    }));
}

void
SynergyLocApi::startTimeBasedTracking(const TrackingOptions& options,
         LocApiResponse* adapterResponse) {

    sendMsg(new LocApiMsg([this, options, adapterResponse] () {
        LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
        enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;
        sllPosMode posMode;

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllStartFix)) {

            posMode.mode = LOC_POSITION_MODE_STANDALONE;
            posMode.recurrence = LOC_GPS_POSITION_RECURRENCE_PERIODIC;
            posMode.min_interval = options.minInterval;
            posMode.preferred_accuracy = 100;
            posMode.preferred_time = 120000;
            posMode.share_position = true;
            posMode.powerMode = GNSS_POWER_MODE_M2;
            posMode.timeBetweenMeasurements = 1000;

            rtv = sllReqIf->sllStartFix(posMode, ((void *)this));
            if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
                err = LOCATION_ERROR_SUCCESS;
            }
        } else {
            err = LOCATION_ERROR_NOT_SUPPORTED;
        }
        if (adapterResponse != NULL) {
            adapterResponse->returnToSender(err);
        }
    }));

}

void
SynergyLocApi::stopTimeBasedTracking(LocApiResponse* adapterResponse){

    sendMsg(new LocApiMsg([this, adapterResponse] () {
        LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
        enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllStopFix)) {

            rtv = sllReqIf->sllStopFix((void *)this);
            if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
                err = LOCATION_ERROR_SUCCESS;
            }
        } else {
            err = LOCATION_ERROR_NOT_SUPPORTED;
        }

        if (adapterResponse != NULL) {
            adapterResponse->returnToSender(err);
        }
    }));
}

void
SynergyLocApi::startDistanceBasedTracking(uint32_t sessionId,
        const LocationOptions& options, LocApiResponse* adapterResponse) {

    sendMsg(new LocApiMsg([this, options, adapterResponse] () {
        LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
        enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;
        sllPosMode posMode;

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllStartFix)) {

            posMode.mode = LOC_POSITION_MODE_STANDALONE;
            posMode.recurrence = LOC_GPS_POSITION_RECURRENCE_PERIODIC;
            posMode.min_interval = options.minInterval;
            posMode.preferred_accuracy = 100;
            posMode.preferred_time = 120000;
            posMode.share_position = true;
            posMode.powerMode = GNSS_POWER_MODE_M2;
            posMode.timeBetweenMeasurements = 1000;

            rtv = sllReqIf->sllStartFix(posMode, ((void *)this));
            if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
                err = LOCATION_ERROR_SUCCESS;
            }
        } else {
            err = LOCATION_ERROR_NOT_SUPPORTED;
        }
        if (adapterResponse != NULL) {
            adapterResponse->returnToSender(err);
        }
    }));
}

void
SynergyLocApi::stopDistanceBasedTracking(uint32_t sessionId,
         LocApiResponse* adapterResponse) {

    sendMsg(new LocApiMsg([this, adapterResponse] () {
        LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
        enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;

        if ((nullptr != sllReqIf) && (nullptr != sllReqIf->sllStopFix)) {

            rtv = sllReqIf->sllStopFix((void *)this);
            if (LOC_API_ADAPTER_ERR_SUCCESS == rtv) {
                err = LOCATION_ERROR_SUCCESS;
            }
        } else {
            err = LOCATION_ERROR_NOT_SUPPORTED;
        }

        if (adapterResponse != NULL) {
            adapterResponse->returnToSender(err);
        }
    }));
}
