/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef LOCATION_INTEGRATION_API_DIAG_LOG_H
#define LOCATION_INTEGRATION_API_DIAG_LOG_H

#include <inttypes.h>

#include "LocationIntegrationApi.h"
#include "LocDiagIfaceApi.h"

/** Packet Versions */
#define LOG_CLIENT_MMF_DIAG_MSG_VERSION             (0)

/** Packet Log Codes */
#ifndef LOG_GNSS_LIA_API_MMF_REPORT_C
#define LOG_GNSS_LIA_API_MMF_REPORT_C (0x1E78)
#endif

#ifdef __linux__
#define PACKED
#define PACKED_POST __attribute__((__packed__))
#endif

#if defined(__linux__) || defined(USE_GLIB) || defined(__ANDROID__)
#define TYPEDEF_PACKED_STRUCT typedef PACKED struct PACKED_POST
#else
#define TYPEDEF_PACKED_STRUCT typedef struct
#endif

#ifndef __LOG_HDR_TYPE__
#define __LOG_HDR_TYPE__
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

 enum diagMmfDataValidity {

    DIAG_MMF_VALID_UTC_TIME     = (1<<0),
    DIAG_MMF_VALID_LAT_DIFF     = (1<<1),
    DIAG_MMF_VALID_LONG_DIFF    = (1<<2),
    DIAG_MMF_VALID_TUNNEL       = (1<<3),
    DIAG_MMF_VALID_BEARING      = (1<<4),
    DIAG_MMF_VALID_ALTITUDE     = (1<<5),
    DIAG_MMF_VALID_HOR_ACC      = (1<<6),
    DIAG_MMF_VALID_ALT_ACC      = (1<<7),
    DIAG_MMF_VALID_BEARING_ACC  = (1<<8),
};

typedef PACKED struct PACKED_POST {
    /** Used by Logging Module
      *  Mandatory field */
    log_hdr_type logHeader;
    /** clientDiag Message Version
     *  Mandatory field */
    uint8_t version;
    /** Validity fields for MMF data fields to follow
     *  Flags defined uisng enum mmfDataValidity */
    uint64_t validityMask;

    /** Unix epoch time of the location fix for which map-match
     *  feedback is being sent, since the start of the Unix epoch
     *  (00:00:00 January 1, 1970 UTC).
     *  Unit: Milli-seconds */
    uint64_t utcTimestampMs;

    /** Latitude difference = map matched latitude - reported latitude
     *  Unit: Degrees
     *  Range: [-90.0, 90.0] */
    double mapMatchedLatitudeDifference;

    /** Longitude difference = map matched longitude - reported longitude
     *  Unit: Degrees
     *  Range: [-180.0, 180.0] */
    double mapMatchedLongitudeDifference;

    /** Bearing: The horizontal direction of travel of the device with
     *  respect to north and is unrelated to the device orientation.
     *  Unit: Degrees
     *  range: [0, 360) */
    float bearing;

    /** Absolute Altitude above the WGS 84 reference ellipsoid
        Unit: meters */
    double altitude;

    /** Horizontal accuracy radius defined with the
     *  68th percentile confidence level.
     *  Unit: meter
     *  Range: 0 or greater */
    float horizontalAccuracy;

    /** Altitude accuracy. Defined with 68% confidence level.
     *  Unit:meter
     *  Range: 0 or greater */
    float altitudeAccuracy;

    /** Estimated bearing accuracy defined with
     *  68 percentile confidence level (1 sigma).
     *  Unit: Degrees
     *  Range [0, 360) */
    float bearingAccuracy;

    /** Road Type. Decision to use the MMF data depends on isTunnel
     *  Value: True or False */
    bool isTunnel;

} diagMapMatchedFeedbackData;

namespace location_integration {

class LocationIntegrationApiDiagLog {

private:
    // Loc-diag-iface handle
    LocDiagInterface *mDiagIface = NULL;

    /** Convert MMF Data from LIA to Diag structure */
    void fillDiagMmfDataInfo(diagMapMatchedFeedbackData* out,
        const mapMatchedFeedbackData& inMmfData);

public:
    LocationIntegrationApiDiagLog();
    ~LocationIntegrationApiDiagLog();

     void diagLogMmfData(const mapMatchedFeedbackData& inMmfData);

};
} // namespace location_integration

#endif // LOCATION_INTEGRATION_API_DIAG_LOG_H
