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

#ifndef LOC_IDL_SERVICE_LOG_H
#define LOC_IDL_SERVICE_LOG_H

#include <stdint.h>
#include <inttypes.h>
#include <vector>
#include "log_util.h"

#include "LocDiagIfaceApi.h"
#include "LocIdlAPIStubImpl.hpp"
#include <v0/com/qualcomm/qti/location/LocIdlAPIStub.hpp>

using namespace std;

#ifndef LOG_1X_BASE_C
#define LOG_1X_BASE_C        ((uint16_t) 0x1000)
#endif

// DIAG ID's and Version Information
#define LOG_LOCATION_IDL_SERVICE_REPORT_C ((0xE6A) + LOG_1X_BASE_C)
#define LOG_LOCATION_IDL_SERVICE_REPORT_VERSION 0

#define IDL_DIAG_MAX_SV (176)

#ifdef __linux__
#define PACKED
#define PACKED_POST __attribute__((__packed__))
#endif

#if defined(__linux__) || defined(USE_GLIB) || defined(__ANDROID__)
#define TYPEDEF_PACKED_STRUCT typedef PACKED struct PACKED_POST
#else
#define TYPEDEF_PACKED_STRUCT typedef struct
#endif

#if !defined(USE_GLIB) && !defined(__ANDROID__)
#ifndef _LOG_HDR_TYPE_
#define _LOG_HDR_TYPE_
typedef struct PACKED_POST {
    uint16_t len;    /* Specifies the length, in bytes of the
                     entry, including this header. */

    uint16_t code;   /* Specifies the log code for the entry as
                     enumerated above. Note: This is
                     specified as word to guarantee size. */
                     /*upper 48 bits represent elapsed time since
                     6 Jan 1980 00:00:00 in 1.25 ms units. The
                     low order 16 bits represent elapsed time
                     since the last 1.25 ms tick in 1/32 chip
                     units (this 16 bit counter wraps at the
                     value 49152). */
    uint32_t ts_lo; /* Time stamp */
    uint32_t ts_hi;
  } log_hdr_type;
 #endif
 #endif

/** @brief Generic Diag logger header for all IDL Service messages. */
TYPEDEF_PACKED_STRUCT {
    /** Used by Logging Module */
    log_hdr_type logHeader;
    /** Message Version */
    uint8_t version;
    /** Client Hashcode */
    uint64_t clientIdentifier;
    /** Log RSS memory stats for FIDL service */
    uint32_t idlServiceRssStats;
    /** GPTP sync status */
    bool gptpSyncStatus;
    /** Reserve field */
    uint32_t reserved1;
} diagServiceGenericHeader;

/** Logging report Type */
enum diagServiceInfoType {
    INFO_TYPE_UNKNOWN    = 0,
    GNSS_REPORT_INFO     = 1,
    SESSION_CONTROL_INFO = 2,
    CONFIG_API_ACCESS_INFO  = 3,
    POWER_EVENT_INFO     = 4,
    CAPS_EVENT_INFO      = 5,
};

/** Enum to specify report Type */
enum diagServiceOutputReportType {
    OUTPUT_REPORT_UNKOWN    = 0,
    OUTPUT_PVT_REPORT       = 1,
    OUTPUT_MEAS_REPORT      = 2,
    OUTPUT_NHZ_MEAS_REPORT  = 3,
    OUTPUT_NMEA_REPORT      = 4,
    OUTPUT_SV_INFO_REPORT   = 5,
    OUTPUT_GNSS_DATA_REPORT = 6,
};

/** To store inof about GNSS reports  being sent out */
TYPEDEF_PACKED_STRUCT {
    /** Gnss report type defined by diagServiceOutputReportType   */
    uint8_t reportType;
    /** Time Difference between Time of genration of packet
     * and time at which report is sent over SOMEIP */
    int16_t packetLatencyTime;
    /** Count of postion reports latent by 20 msec */
    uint32_t latentReportCount;
} diagOutputGnssReportInfo;

/** Seerion Control command*/
enum diagServiceSessionRequestType {
    SESSION_REQUEST_UNKNOWN  = 0,
    SESSION_START_REQUEST    = 1,
    SESSION_STOP_REQUEST     = 2,
};

/** Session Control Command related Info  */
TYPEDEF_PACKED_STRUCT {
    /**Type of request Start/Stop defined by diagServiceSessionRequestType */
    uint8_t sessionRequestType;
    /** Requested Time interval between fixes */
    uint32_t intervalMs;
    /** Requested subscription mask */
    uint32_t requestedCallbackMask;
    /** IDL service subscription mask */
    uint32_t updatedCallbackMask;
    /** Number of session control requests */
    uint32_t numControlRequests;
} diagControlCommandInfo;

/** Config API types  */
enum diagServiceLIARequestType {
    LIA_REQUEST_UNKNOWN   = 0,
    DELETE_AIDING_DATA    = 1,
    CONFIG_CONSTELLATIONS_REQUEST = 2,
};

/**Fields specifiying Aiding Data deletion */
typedef enum {
    /** Aiding Mask Unknown */
    NO_AIDING_DELETION  = 0,
    /** Mask to delete all aiding data from all position
    engines on the device <br/> */
    DELETE_ALL_AIDING_DATA  = 0x0001,
    /** Mask to delete ephemeris aiding data <br/> */
    DELETE_EPHEMERIS_DATA   = 0x0002,
    /** Mask to delete calibration data from dead reckoning position
     *  engine.<br/> */
    DELETE_DR_SENSOR_CALIBRATION  = 0x0004,
}  diagDeleteAidingMask;

/**Sv Info to store configConstellation request */
TYPEDEF_PACKED_STRUCT {
    /** Gnss Constellation */
    uint8_t constellationType;
    /** SV ID */
    uint16_t svId;
} diagGnssSvIdInfo;

/** Config API info */
TYPEDEF_PACKED_STRUCT {
    /** Config API type defined by  diagServiceLIARequestType */
    uint8_t requestType;
    union {
        /** Delete Aiding Mask diagDeleteAidingMask */
        uint32_t deleteAidingMask;
        /** Config Constellation Info diagGnssSvIdInfo*/
        diagGnssSvIdInfo svList[IDL_DIAG_MAX_SV];
    } configData;
} diagConfigApiRequest;

// Enum to define supported Power states in power-daemon
enum diagPowerStateType {
    POWER_STATE_UNKNOWN          = 0,
    POWER_STATE_SUSPEND          = 1,
    POWER_STATE_RESUME           = 2,
    POWER_STATE_SHUTDOWN         = 3,
    POWER_STATE_DEEP_SLEEP_ENTRY = 4,
    POWER_STATE_DEEP_SLEEP_EXIT  = 5
};

/**IDL service registeration status */
enum diagServiceStatus {
    SERVICE_STAUS_UNKNOWN      = 0,
    REGISTER_SERVICE_SUCCESS   = 1,
    REGISTER_SERVICE_FAILED    = 2,
    UNREGISTER_SERVICE_SUCCESS = 3,
    UNREGISTER_SERVICE_FAILED  = 4,
};

/* Platform power event Info **/
TYPEDEF_PACKED_STRUCT {
    /** Platform define power states:: diagPowerStateType */
    uint8_t powerEventType;
    /** FIDL service registeration status:: diagServiceStatus*/
    uint8_t serviceStatus;
} diagPowerEventInfo;

/* Platform capability Info **/
TYPEDEF_PACKED_STRUCT {
    /** Length of capability string */
    uint32_t capabilityStringLength;
    /** capabilities  Received*/
    uint8_t capabilitiesReceived[1];
} diagCapabilityReceivedInfo;


TYPEDEF_PACKED_STRUCT {
    /** IDL Service Generic Info */
    diagServiceGenericHeader header;
    /** IDL report type defined by diagServiceInfoType */
    uint8_t serviceInfoType;
    /** Union for various reports that shall be captured in IDL Service Context */
    union {
        /** System Capabilities Info */
        diagCapabilityReceivedInfo capabiltiyInfo;
        /** Captures Type of GNSS report Sent out and associated info */
        diagOutputGnssReportInfo reportInfo;
        /** Captures Type of Session Control Commands received by LocIdlService */
        diagControlCommandInfo cmdInfo;
        /** Captures Type of configuration requests received by LocIdlService */
        diagConfigApiRequest configApiInfo;
        /** Captures Platform power events received by LocIdlService */
        diagPowerEventInfo powerEvent;
    } serviceReport;
} diagServiceInfoStruct;

class LocIdlServiceLog {

public:
    LocIdlServiceLog();
    ~LocIdlServiceLog();
    bool initializeDiagIface();
    void populateIdlDiagHeaderInfo(diagServiceGenericHeader& idlHeader);
    void diagLogCapabilityInfo(string capabilitiesReceived);
    void diagLogDeleteAidingRequest (uint32_t aidingMask);
    void diagLogSessionInfo (diagControlCommandInfo idlSessionInfo, uint64_t clientIdentifier);
    void diagLogGnssReportInfo(uint8_t reportType, int16_t latencyMs, uint32_t latentReportCount);
    void diagLogPowerEventInfo(uint8_t powerEvent, uint8_t serviceStatus);
    void diagLogDeleteAidingRequest (uint64_t clientIdentifier, uint32_t aidingMask);
    void diagLogConfigConstellationRequest(uint64_t clientIdentifier,
            vector< LocIdlAPI::IDLGnssSvIdInfo > svListSrc);
    void updateSystemHealth(uint32_t totalRss, bool gptpSyncStatus);
private:
    uint32_t mTotalRss;
    bool mGptpSyncStatus;
};

#endif //LOC_IDL_SERVICE_LOG_H
