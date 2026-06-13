/* Copyright (c) 2011-2020, The Linux Foundation. All rights reserved.
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

/** @file loc-sll-interface.h
*/

#ifndef LOC_SLL_INTERFACE_H
#define LOC_SLL_INTERFACE_H


/*=============================================================================
 *============================================================================*/
#include <stdbool.h>
#include <stdint.h>
#include "gps_extended_c.h"
#include "LocationDataTypes.h"
#include "loc_gps.h"
#include "loc_pla.h"
#include "log_util.h"


#define SLL_LOC_SUPPORTED_FEATURE_LENGTH_V02 (100)

typedef struct {
    LocPositionMode mode;
    LocGpsPositionRecurrence recurrence;
    uint32_t min_interval;
    uint32_t preferred_accuracy;
    uint32_t preferred_time;
    bool share_position;
    char credentials[14];
    char provider[8];
    GnssPowerMode powerMode;
    uint32_t timeBetweenMeasurements;
} sllPosMode;


/* SLL Supported Features */
typedef struct {
    /**< Must be set to # of elements in feature */
    uint32_t feature_len;
/**<   This field describes which features are supported in the running
     QMI_LOC service. The array of unit8 is the bitmask where each bit
     represents a feature enum. Bit 0 represents feature enum ID 0,
     bit 1 represents feature enum ID 1, etc.
     For example, if QMI_LOC supports feature enum 0,1,2,8,
     feature_len is 2, and
     feature array is [7,1]. \n
     - Type: Array of uint8
     - Maximum array length: 100
     - enum is "loc_supported_feature_enum" which declared in gps_extended_c.h file.
*/
    uint8_t feature[SLL_LOC_SUPPORTED_FEATURE_LENGTH_V02];
} sllFeatureList;


/** SLL module’s Event Interface.
    These Event shall be implement by Synergy Location API module.
    SLL module shall be store and call these Event APIs.
 */
typedef struct {
    /**
       Engine Up Event. SLL shall indicate this event when SLL engine is ready to accept
       command such as start session, delete assistance.
       SLL context will be used to process this event and will call Adapter APIs in the
       same context. SLL need to support multiple execution contexts. The execution context
       will identify using ‘void *context’ variable.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllHandleEngineUpEvent)(void *context);

    /**
       The SLL event to indicate as Engine Down. This interface API should be call by SLL.
       The Event processing is done in SLL context and same context Adapter API will called.
       SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllHandleEngineDownEvent)(void *context);

    /**
       The SLL event to indicate as PVT report. This interface API should be call by SLL.
       The Event processing is done in SLL context and same context Adapter API will called.
       SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #UlpLocation \n
       #GpsLocationExtended \n
       #LocPosTechMask \n
       #GnssDataNotification

       @param location[Input]           Ulp Location Report.

       @param locationExtended[Input]   Extended location report.

       @param status                    Session Status.

       @param loc_technology_mask       Location Technology Mask.

       @param pDataNotify               GNSS Data Notification.

       @param msInWeek                  Week in milliseconds.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportPosition)(UlpLocation& location,
                        GpsLocationExtended& locationExtended,
                        enum loc_sess_status status,
                        LocPosTechMask loc_technology_mask,
                        GnssDataNotification* pDataNotify,
                        int msInWeek, void *context);

    /**
       The SLL event to indicate as GNSS SV Notification. This interface API should
       call by SLL. The Event processing is done in SLL context and same context
       Adapter API will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssSvNotification \n

       @param GnssSvNotification[Input]  GNSS SV Data.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportSv)(GnssSvNotification& svNotify, void *context);

    /**
       The SLL event to report as GNSS SV Measurement. This interface API should
       call by SLL. The Event processing is done in SLL context and same context
       Adapter API will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssSvMeasurementSet \n

       @param GnssSvMeasurementSet[Input]  GNSS SV Measurement Data.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportSvMeasurement)(GnssMeasurements &svMeasurementSet, void *context);

    /**
       The SLL event to report as GNSS SV Polynomial. This interface API should
       call by SLL. The Event processing is done in SLL context and same context
       Adapter API will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssSvPolynomial \n

       @param svPolynomial[Input]  GNSS SV Polynomial Data.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportSvPolynomial)(GnssSvPolynomial &svPolynomial, void *context);

    /**
       The SLL event to report as GNSS SV Ephemeris. This interface API should
       call by SLL. The Event processing is done in SLL context and same context
       Adapter API will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssSvEphemerisReport \n

       @param svEphemeris[Input]  GNSS SV Ephemeris Report.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportSvEphemeris)(GnssSvEphemerisReport &svEphemeris, void *context);

    /**
       The SLL event to report as GNSS Status Value. This interface API should
       call by SLL. The Event processing is done in SLL context and same context
       Adapter API will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #LocGpsStatusValue \n

       @param LocGpsStatusValue[Input]  GPS Status Value.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportStatus)(LocGpsStatusValue status, void *context);

    /**
       The SLL event to report NMEA Sentences. This interface API should
       call by SLL. The Event processing is done in SLL context and same context
       Adapter API will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @param nmea[Input]  NMEA Sentence.

       @param length[Input]  Length of NMEA Sentence.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportNmea)(const char* nmea, int length, void *context);

    /**
       The SLL event to report GNSS Data Notification. This interface API should
       call by SLL. The Event processing is done in SLL context and same context
       Adapter API will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssDataNotification

       @param dataNotify[Input]  GNSS Data Notification.

       @param msInWeek[Input]    Week in milliseconds.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportData)(GnssDataNotification& dataNotify, int msInWeek, void *context);

    /**
       The SLL event to report XTRA Server details. This interface API should
       call by SLL. The Event processing is done in SLL context and same context
       Adapter API will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @param url1[Input]     XTRA Server URL -1.

       @param url2[Input]     XTRA Server URL -2.

       @param url3[Input]     XTRA Server URL -3.

       @param maxlength[Input]  Max length of URLs.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportXtraServer)(const char* url1, const char* url2,
            const char* url3, const int maxlength, void *context);

    /**
       The SLL event to report Location System Info. This interface API should
       call by SLL. The Event processing is done in SLL context and same context
       Adapter API will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #LocationSystemInfo

       @param locationSystemInfo[Input]  location System Info.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportLocationSystemInfo)(const LocationSystemInfo& locationSystemInfo,
            void *context);

    /**
       The SLL event to request XTRA Data. This interface API should be call by SLL.
       The Event processing is done in SLL context and same context Adapter API
       will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllRequestXtraData)(void *context);

    /**
       The SLL event to request Time info. This interface API should be call by SLL.
       The Event processing is done in SLL context and same context Adapter API
       will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllRequestTime)(void *context);

    /**
       The SLL event to request LOcation info. This interface API should be call by SLL.
       The Event processing is done in SLL context and same context Adapter API
       will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllRequestLocation)(void *context);

    /**
       The SLL event to request for ATL. This interface API should be call by SLL.
       The Event processing is done in SLL context and same context Adapter API
       will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #LocAGpsType \n
       #LocApnTypeMask

       @param connHandle[Input]   ATL Connection handle.

       @param agps_type[Input]    AGPS Type.

       @param apn_type_mask[Input] APN Type Mask.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllRequestATL)(int connHandle, LocAGpsType agps_type, LocApnTypeMask apn_type_mask,
            void *context);

    /**
       The SLL event to request for release ATL. This interface API should be call by SLL.
       The Event processing is done in SLL context and same context Adapter API
       will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
        #LocAGpsType \n
        #LocApnTypeMask

       @param connHandle[Input]   ATL Connection handle.


       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReleaseATL)(int connHandle, void *context);

    /**
       The SLL event to indicate NI Notification. This interface API should be call by SLL.
       The Event processing is done in SLL context and same context Adapter API
       will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssNiNotification

       @param notify[Input]   GNSS Ni Notification.

       @param data[Input]     Ni Data Pointer.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllRequestNiNotify)(GnssNiNotification &notify, const void* data, void *context);

    /**
       The SLL event to report GNSS Measurement Data. This interface API should be call by SLL.
       The Event processing is done in SLL context and same context Adapter API
       will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssMeasurementsNotification

       @param measurements[Input]   GNSS measurements Data.

       @param msInWeek[Input]     Week in milliseconds.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.

       @Note
           This API do not have intra-constellation information.
    */
    void (*sllReportGnssMeasurementData)(GnssMeasurements& measurements,
            int msInWeek, void *context);
    /**
       The SLL event to report WWAN ZPP Fix. This interface API should be call by SLL.
       The Event processing is done in SLL context and same context Adapter API
       will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #LocGpsLocation

       @param zppLoc[Input]    ZPP PVT report.

       @param msInWeek[Input]     Week in milliseconds.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportWwanZppFix)(LocGpsLocation &zppLoc, void *context);

    /**
       The SLL event to report ZPP Best Available Fix. This interface API should be call by SLL.
       The Event processing is done in SLL context and same context Adapter API
       will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #LocGpsLocation \n
       #GpsLocationExtended \n
       #LocPosTechMask

       @param zppLoc[Input]     ZPP PVT report.

       @param location_extended[Input]  Extended Location PVT report.

       @param tech_mask[Input]  Tech Mask.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportZppBestAvailableFix) (LocGpsLocation &zppLoc,
            GpsLocationExtended &location_extended, LocPosTechMask tech_mask, void *context);

    /**
       The SLL event to report GNSS SV ID Configure. This event is the response of
       sllGetBlacklistSv request. This interface API should be call by SLL.
       The Event processing is done in SLL context and same context Adapter API
       will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssSvIdConfig

       @param config[Input]     GNSS SV ID configuration.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportGnssSvIdConfig)(const GnssSvIdConfig& config, void *context);

    /**
       The SLL event to report GNSS SV Type Configure. This event is response of
       sllGetConstellationControl request. This interface API should be call by SLL.
       The Event processing is done in SLL context and same context Adapter API
       will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssSvTypeConfig

       @param config[Input]     GNSS SV Type configuration.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportGnssSvTypeConfig)(const GnssSvTypeConfig& config, void *context);

    /**
       The SLL event to Request for ODCPI. This interface API should be call by SLL.
       The Event processing is done in SLL context and same context Adapter API
       will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #OdcpiRequestInfo

       @param request[Input]    Request for ODCPI.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllRequestOdcpi)(OdcpiRequestInfo& request, void *context);

    /**
       The SLL event to Report GNSS Engine Energy Consumed. This interface API
       call by SLL. The Event processing is done in SLL context and same context
       Adapter API will called. SLL need to support multiple execution contexts.
       The execution context will identify using
       ‘void *context’ variable.

       @param energyConsumedSinceFirstBoot[Input] Energy Consumed
                   Since from first Boot.

       @param context[Input]   Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportGnssEngEnergyConsumedEvent)(uint64_t energyConsumedSinceFirstBoot,
            void *context);

    /**
       The SLL event to Indicate Deleted Aiding Data. This interface API call by SLL.
       The Event processing is done in SLL context and same context Adapter API will
       called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssAidingData

       @param aidingData[Input] Deleted Aiding Data.

       @param context[Input]   Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportDeleteAidingDataEvent)(GnssAidingData& aidingData, void *context);

    /**
       The SLL event to GNSS Klobuchar Iono Model. This interface API call by SLL.
       The Event processing is done in SLL context and same context Adapter API
       will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssKlobucharIonoModel

       @param ionoModel[Input] GNSS Klobuchar Iono Model.

       @param context[Input]   Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportKlobucharIonoModel)(GnssKlobucharIonoModel& ionoModel, void *context);

    /**
       The SLL event to Report GNSS Additional System Info. This interface API
       call by SLL. The Event processing is done in SLL context and same context
       Adapter API will called. SLL need to support multiple execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssAdditionalSystemInfo

       @param additionalSystemInfo[Input] Additional System Info.

       @param context[Input]   Context Pointer of Synergy Location API.

       @return
           None.

       @dependencies
           None.
    */
    void (*sllReportGnssAdditionalSystemInfo)(GnssAdditionalSystemInfo& additionalSystemInfo,
            void *context);
    void (*sllRequestEedGpsData) (void *context);
    void (*sllRequestEedGloData) (void *context);

} SllInterfaceEvent;

/** SLL module’s Interface commands.
    These commands should implement by SLL module.
    Synergy Location API module should store and call these command APIs.
 */
typedef struct {
   /**
       The SLL Open interface function request to open/active device.
       This interface API should be call by Synergy LOC API.
       This API will be called multiple times in different execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #LOC_API_ADAPTER_EVENT_MASK_T \n
       #loc_api_adapter_err

       @param requestedMask [Input]
                                This input variable indicates the requested
                                configuration from the GNSS hardware.

       @param isMaster [Input]   'isMaster' indicates as the master session.

       @param supportedMask[out] The GNSS HW provides its supported configuration.

       @param sllFeatures[out]   The GNSS HW provides its supported feature list.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllOpen)(uint64_t requestedMask, bool isMaster,
                uint64_t *supportedMask, sllFeatureList * sllFeatures,
                void *context);

   /**
       The SLL Close interface function request to close/de-active device.
       This interface API should be call by Synergy LOC API.
       This API will be called multiple times in different execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #loc_api_adapter_err

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllClose)(void *context);

   /**
       The SLL Start fix interface function request to start a session of SLL.
       This interface API should be call by Synergy LOC API.
       This API will be called multiple times in different execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #loc_api_adapter_err

       @param sllPosMode[Input] Provide Session parameters to a start session.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllStartFix)(sllPosMode& fixCriteria, void *context);
   /**
       The SLL Stop fix interface function request to stop a ongoing session of SLL.
       This interface API should be call by Synergy LOC API.
       This API will be called multiple times in different execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #loc_api_adapter_err

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllStopFix)(void *context);

   /**
       The SLL Delete Aiding Data interface function request delete assistance data of SLL.
       This interface API should be call by Synergy LOC API.
       This API will be called multiple times in different execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #loc_api_adapter_err

       @param GnssAidingData[Input] Provide Session parameters to a start session.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllDeleteAidingData)(const GnssAidingData& data, void *context);

   /**
       The interface function to inject Position to SLL. This interface API should be call by
       Synergy LOC API. This API will be called multiple times in different execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssLocationInfoNotification
       #loc_api_adapter_err

       @param locationInfo[Input] Position to inject.

       @param onDemandCpi[Input]  Flag to indicate on demand.

       @param context[Input]      Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllInjectPosition)(const GnssLocationInfoNotification &locationInfo,
            bool onDemandCpi, void *context);

   /**
       The interface function to set time of SLL. This interface API should be call by
       Synergy LOC API. This API will be called multiple times in different execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #LocGpsUtcTime
       #loc_api_adapter_err

       @param time[Input]           Time to inject.

       @param timeReference[Input]  Indicate time reference.

       @param uncertainty[Input]    Time uncertainty.

       @param onDemandCpi[Input]    Flag to indicate on demand.

       @param context[Input]        Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllSetTime)(LocGpsUtcTime time, int64_t timeReference,
            int uncertainty, void *context);

   /**
       The interface function to inject XTRA Data to SLL. This interface API should be call by
       Synergy LOC API. This API will be called multiple times in different execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #loc_api_adapter_err

       @param data[Input]     XTRA data.

       @param length[Input]   Indicate length of XTRA data.

       @param context[Input]  Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                              The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                              is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllSetXtraData)(char* data, int length, void *context);

   /**
       The interface function to request for XTRA Server to SLL. This interface API should be call by
       Synergy LOC API. This API will be called multiple times in different execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
    #loc_api_adapter_err

       @param context[Input]  Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                              The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                              is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllRequestXtraServer)(void *context);


   /**
       The interface function to indicate status of open ATL connection.
       This interface API should be call by Synergy LOC API. This API will be called multiple
       times in different execution contexts. The execution context will identify using
       ‘void *context’ variable.

       @datatypes
       #AGpsBearerType
       #LocAGpsType
       #LocApnTypeMask
       #loc_api_adapter_err

       @param handle[Input]    handle of ATL connection status.

       @param is_succ[Input]   Indicate status of ATL Connection.

       @param apn[Input]       Provide APN details.

       @param apnLen[Input]    Length of APN data.

       @param bear[Input]      Indicate bearer type.

       @param agpsType[Input]  Indicate AGPS type.

       @param mask[Input]      Indicate APN type.

       @param context[Input]   Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllAtlOpenStatus)(int handle, int is_succ, char* apn,
            uint32_t apnLen, AGpsBearerType bear, LocAGpsType agpsType, LocApnTypeMask mask,
            void *context);

   /**
       The interface function to indicate status of close ATL connection.
       This interface API should be call by Synergy LOC API. This API will be called multiple
       times in different execution contexts. The execution context will identify using
       ‘void *context’ variable.

       @datatypes
       #AGpsBearerType
       #LocAGpsType
       #LocApnTypeMask
       #loc_api_adapter_err

       @param handle[Input]    handle of ATL connection status.

       @param is_succ[Input]   Indicate status of ATL Connection.

       @param context[Input]   Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllAtlCloseStatus)(int handle, int is_succ, void *context);

   /**
       The SLL Set Position Mode interface function to update session parameters of
       ongoing session or upcoming session. This interface API should be call by Synergy LOC API.
       This API will be called multiple times in different execution contexts.
       The execution context will identify using ‘void *context’ variable.

       @datatypes
       #loc_api_adapter_err

       @param sllPosMode[Input] Provide Session parameters to a start session.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllSetPositionMode)(sllPosMode& posMode, void *context);

   /**
       The SLL interface function to Set Sync Server URL. This interface API should be call
       by Synergy LOC API. This API will be called multiple times in different execution
       contexts. The execution context will identify using ‘void *context’ variable.

       @datatypes
       #LocServerType
       #loc_api_adapter_err

       @param url[Input]        Indicate URL.

       @param len[Input]        Indicate URL length.

       @param type[Input]       Indicate Location Server Type.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllSetServerSync)(const char* url, int len, LocServerType type,
            void *context);

   /**
       The SLL interface function to inform NI Response. This interface API should be call
       by Synergy LOC API. This API will be called multiple times in different execution
       contexts. The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssNiResponse
       #loc_api_adapter_err

       @param userResponse[Input]     Ni User Response.

       @param passThroughData[Input]  Pass Through Data.

       @param context[Input]          Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllInformNiResponse)(GnssNiResponse userResponse,
            const void* passThroughData, void *context);

   /**
       The SLL interface function to configure SUPL version. This interface API should be call
       by Synergy LOC API. This API will be called multiple times in different execution
       contexts. The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssConfigSuplVersion
       #loc_api_adapter_err

       @param version[Input]    Indicate SUPL version.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllSetSUPLVersionSync)(GnssConfigSuplVersion version, void *context);

   /**
       The SLL interface function to configure NMEA Type. This interface API should be call
       by Synergy LOC API. This API will be called multiple times in different execution
       contexts. The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssConfigSuplVersion
       #loc_api_adapter_err

       @param typesMask[Input]  Configure of Type Mask.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllSetNMEATypesSync)(uint32_t typesMask, void *context);

   /**
       The SLL interface function to configure LPP sync. This interface API should be call by
       Synergy LOC API. This API will be called multiple times in different execution
       contexts. The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssConfigLppProfileMask
       #loc_api_adapter_err

       @param profile[Input]    Configure of LPP Profile.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllSetLPPConfigSync)(GnssConfigLppProfileMask profileMask,
           void *context);

   /**
       The SLL interface function to configure Sensor Properties. This interface API should
       call by Synergy LOC API. This API will be called multiple times in different execution
       contexts. The execution context will identify using ‘void *context’ variable.

       @datatypes
       #loc_api_adapter_err

       @param gyroBiasVarianceRandomWalk_valid[Input] Flag to indicate Gyro Bias.

       @param gyroBiasVarianceRandomWalk[Input] Gyro Bias.

       @param accelBiasVarianceRandomWalk_valid[Input] Flag to indicate ACCEL Bias.

       @param accelBiasVarianceRandomWalk[Input] ACCEL Bias.

       @param angleBiasVarianceRandomWalk_valid[Input] Flag to indicateAngle Bias.

       @param angleBiasVarianceRandomWalk[Input] Angle Bias.

       @param rateBiasVarianceRandomWalk_valid[Input] Flag to indicate Angle Bias.

       @param rateBiasVarianceRandomWalk[Input] Rate Bias.

       @param velocityBiasVarianceRandomWalk_valid[Input] Flag to indicate Velocity Bias.

       @param velocityBiasVarianceRandomWalk[Input] Velocity Bias.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllSetSensorPropertiesSync)
                           (bool gyroBiasVarianceRandomWalk_valid,
                            float gyroBiasVarianceRandomWalk,
                            bool accelBiasVarianceRandomWalk_valid,
                            float accelBiasVarianceRandomWalk,
                            bool angleBiasVarianceRandomWalk_valid,
                            float angleBiasVarianceRandomWalk,
                            bool rateBiasVarianceRandomWalk_valid,
                            float rateBiasVarianceRandomWalk,
                            bool velocityBiasVarianceRandomWalk_valid,
                            float velocityBiasVarianceRandomWalk, void *context);
   /**
       The SLL interface function to configure Set Sensor Preference Control.
       This interface API should be call by Synergy LOC API. This API will be called
       multiple times in different execution contexts. The execution context will
       identify using ‘void *context’ variable.

       @datatypes
       #loc_api_adapter_err

       @param accelSamplesPerBatch[Input] ACCEL Samples per batch.

       @param accelBatchesPerSec[Input]   ACCEL Batches per seconds.

       @param gyroSamplesPerBatch[Input]  Gyro Samples per batch.

       @param gyroBatchesPerSec[Input]    Gyro Batches per seconds.

       @param accelSamplesPerBatchHigh[Input] ACCEL Sample per batch High.

       @param accelBatchesPerSecHigh[Input]   ACCEL batches per Seconds High.

       @param gyroSamplesPerBatchHigh[Input]  Gyro Samples per Batch High.

       @param gyroBatchesPerSecHigh[Input]    Gyro Batch Per Sec High.

       @param algorithmConfig[Input] Algorithm Configure.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllSetSensorPerfControlConfigSync)(int controlMode,
                            int accelSamplesPerBatch,
                            int accelBatchesPerSec,
                            int gyroSamplesPerBatch,
                            int gyroBatchesPerSec,
                            int accelSamplesPerBatchHigh,
                            int accelBatchesPerSecHigh,
                            int gyroSamplesPerBatchHigh,
                            int gyroBatchesPerSecHigh,
                            int algorithmConfig, void *context);
   /**
       The SLL interface function to configure AGLONASS Protocol Sync. This interface
       API should be call by Synergy LOC API. This API will be called multiple times in
       different execution contexts. The execution context will identify using
       ‘void *context’ variable.

       @datatypes
       #GnssConfigAGlonassPositionProtocolMask
       #loc_api_adapter_err

       @param aGlonassProtocol[Input]    Configure AGLONASS Protocol sync.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllSetAGLONASSProtocolSync)
            (GnssConfigAGlonassPositionProtocolMask aGlonassProtocol, void *context);

   /**
       The SLL interface function to Set LPP protocol Control Plan Sync Mask. This interface API
       should be call by Synergy LOC API. This API will be called multiple times in different
       execution contexts. The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssConfigLppeControlPlaneMask
       #loc_api_adapter_err

       @param lppeCP[Input]     Configure of LPP protocol of Control Plan Mask.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllSetLPPeProtocolCpSync)(GnssConfigLppeControlPlaneMask lppeCP,
            void *context);

   /**
       The SLL interface function to Set LPP protocol User Plan Sync Mask. This interface API should
       call by Synergy LOC API. This API will be called multiple times in different
       execution contexts. The execution context will identify using ‘void *context’ variable.

       @datatypes
       #GnssConfigLppeUserPlaneMask
       #loc_api_adapter_err

       @param profile[Input]    Configure of LPP User Plan Mask Profile.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllSetLPPeProtocolUpSync)(GnssConfigLppeUserPlaneMask lppeUP,
            void *context);

   /**
       The SLL interface function to get GNSS SUPL Version. This interface API should be call by
       Synergy LOC API. This API will be called multiple times in different execution contexts.
       The execution context will identify using ‘void *context’ variable.
       This is blocking call.

       @datatypes
       #GnssConfigSuplVersion
       #loc_api_adapter_err

       @param suplVersion[Input] SUPL version.

       @param suplConfig[Output] SUPL Version configuration.

       @param context[Input]     Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllConvertSuplVersion)(const uint32_t suplVersion,
            GnssConfigSuplVersion *suplConfig, void *context);

   /**
       The SLL interface function to get GNSS LPP profile. This interface API should
       call by Synergy LOC API. This API will be called multiple times in different
       execution contexts. The execution context will identify using ‘void *context’ variable.
       This is blocking call.

       @datatypes
       #GnssConfigLppProfileMask
       #loc_api_adapter_err

       @param lppProfile[Input] LPP Profile.

       @param gnssLppProfile[Output] GNSS LPP Profile configuration.

       @param context[Input]    Context Pointer of Synergy Location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllConvertLppProfile)(const uint32_t lppProfile,
            GnssConfigLppProfileMask *gnssLppProfileMask, void *context);

   /**
       The SLL interface function to get GNSS LPP Control Plan Mask. This interface API should
       call by Synergy LOC API. This API will be called multiple times in different
       execution contexts. The execution context will identify using ‘void *context’ variable.
       This is blocking call.

       @datatypes
       #GnssConfigLppeControlPlaneMask
       #loc_api_adapter_err

       @param lppeControlPlaneMask[Input] LPP Control Plan Mask.

       @param lppControlPlanMask[Output] GNSS LPP Control Plan Mask configuration.

       @param context[Input]    Context Pointer of Synergy Location API. SLL should store
                                and use any future interaction with Synergy location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllConvertLppeCp)(const uint32_t lppeControlPlaneMask,
            GnssConfigLppeControlPlaneMask *gnssLppControlPlanMask, void *context);

   /**
       The SLL interface function to get GNSS LPP User Plan Mask. This interface API should
       call by Synergy LOC API. This API will be called multiple times in different
       execution contexts. The execution context will identify using ‘void *context’ variable.
       This is blocking call.

       @datatypes
       #GnssConfigLppeUserPlaneMask
       #loc_api_adapter_err

       @param lppeUserPlaneMask[Input] LPP User Plan Mask.

       @param lppControlPlanMask[Output] GNSS LPP User Plan Mask configuration.

       @param context[Input]    Context Pointer of Synergy Location API. SLL should store
                                and use any future interaction with Synergy location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllConvertLppeUp)(const uint32_t lppeUserPlaneMask,
            GnssConfigLppeUserPlaneMask *gnssLppUpMask, void *context);

   /**
       The SLL interface function to request for WWAN ZPP Fix. This interface API should
       call by Synergy LOC API. This API will be called multiple times in different
       execution contexts. The execution context will identify using ‘void *context’ variable.

       @datatypes
       #loc_api_adapter_err

       @param context[Input]    Context Pointer of Synergy Location API. SLL should store
                                and use any future interaction with Synergy location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllGetWwanZppFix)(void *context);

   /**
       The SLL interface function to request for  Best Available ZPP Fix. This interface
       API should be call by Synergy LOC API. This API will be called multiple times in different
       execution contexts. The execution context will identify using ‘void *context’ variable.

       @datatypes
       #loc_api_adapter_err

       @param context[Input]    Context Pointer of Synergy Location API. SLL should store
                                and use any future interaction with Synergy location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllGetBestAvailableZppFix)(void *context);

   /**
       The SLL interface to request GPS Lock incase of SUPL when GNSS disabled. This interface
       API should be call by Synergy LOC API. This API will be called multiple times in different
       execution contexts. The execution context will identify using ‘void *context’ variable.

       @datatypes
        #GnssConfigGpsLock
        #loc_api_adapter_err

       @param context[Input]    Context Pointer of Synergy Location API. SLL should store
                                and use any future interaction with Synergy location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllSetGpsLockSync)(GnssConfigGpsLock lock, void *context);

   /**
       The SLL interface to request For Aiding Data. This interface API should be call by
       Synergy LOC API. This API will be called multiple times in different execution
       contexts.The execution context will identify using ‘void *context’ variable.

       @datatypes
        #GnssAidingDataSvMask
        #loc_api_adapter_err

       @GnssConfigGpsLock[Input]  GNSS Aiding Data SV Mask.

       @param context[Input]    Context Pointer of Synergy Location API. SLL should store
                                and use any future interaction with Synergy location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllRequestForAidingData)(GnssAidingDataSvMask svDataMask,
            void *context);

   /**
       The SLL interface to Get GPS Lock incase of SUPL. This interface API should be call
       by Synergy LOC API. This API will be called multiple times in different execution
       contexts. The execution context will identify using 'void *context' variable.

       @datatypes
        #loc_api_adapter_err

       @param subType[Input]    Subscription that is associated with the engine lock state
                                being requested

       @param context[Input]    Context Pointer of Synergy Location API. SLL should store
                                and use any future interaction with Synergy location API.

       @return
           loc_api_adapter_err[Output]
                            The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                            is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllGetGpsLock)(uint8_t subType, void *context);


   /**
       The SLL interface to Set XTRA Veresion Check. This interface API should be call
       by Synergy LOC API. This API will be called multiple times in different execution
       contexts. The execution context will identify using 'void *context' variable.

       @datatypes
          #loc_api_adapter_err

       @param check[Input]      Check XTRA File Version.

       @param context[Input]    Context Pointer of Synergy Location API. SLL should store
                                and use any future interaction with Synergy location API.

       @return
           loc_api_adapter_err[Output]
                            The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                            is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllSetXtraVersionCheckSync)(uint32_t check, void *context);


   /**
       The SLL interface function to Install AGPS Certificate. This interface API should be call
       by Synergy LOC API. This API will be called multiple times in different execution
       contexts. The execution context will identify using ‘void *context’ variable.

       @datatypes
       #LocDerEncodedCertificate
       #loc_api_adapter_err

       @param pData[Input]      Encoded AGPS Certificate.

       @param length[Input]     Length of AGPS Certificate.

       @param slotBitMask[Input] Slot Bit mask.

       @param context[Input]    Context Pointer of Synergy Location API. SLL should store
                                and use any future interaction with Synergy location API.

       @return
           loc_api_adapter_err[Output]
                                The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                is successfully received and other status indicate as failure.

       @dependencies
           None.
   */
   enum loc_api_adapter_err (*sllInstallAGpsCert)(const LocDerEncodedCertificate* pData,
                                 size_t length,
                                 uint32_t slotBitMask, void *context);

    /**
        The SLL interface function to set constraint for time uncertainty and energy Budget.
        This interface API should be call by Synergy LOC API. This API will be called multiple
        times in different execution contexts. The execution context will identify using
        'void *context' variable.

        @datatypes
        #loc_api_adapter_err

        @param enabled[Input]           Enabled or Disabled Constrained.

        @param tuncConstraint[Input]    If constraint is enabled, the engine maintains its
                                        time uncertainty below the specified constraint in
                                        tuncConstraint. The units are in ms.

        @param energyBudget[Input]      If constraint is enabled, and if energyBudget is
                                        specified, the engine uses this as the maximum energy
                                        to be used while keeping the engine in constrained tunc
                                        mode. Units: 0.1 milliwatt second.

        @param context[Input]    Context Pointer of Synergy Location API. SLL should store
                                 and use any future interaction with Synergy location API.

        @return
            loc_api_adapter_err[Output]
                                 The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                 is successfully received and other status indicate as failure.

        @dependencies
            None.
    */
    enum loc_api_adapter_err (*sllSetConstrainedTuncMode)(bool enabled, float tuncConstraint,
                                  uint32_t energyBudget, void *context);

    /**
        The SLL interface function to enable position assisted clock estimation mode.
        This interface API should be call by Synergy LOC API. This API will be called multiple
        times in different execution contexts. The execution context will identify using
        'void *context' variable.

        @datatypes
        #loc_api_adapter_err

        @param enabled[Input]    Enabled or Disabled position assisted clock estimation mode.

        @param context[Input]    Context Pointer of Synergy Location API. SLL should store
                                 and use any future interaction with Synergy location API.

        @return
            loc_api_adapter_err[Output]
                                 The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                 is successfully received and other status indicate as failure.

        @dependencies
            None.
    */
    enum loc_api_adapter_err (*sllSetPositionAssistedClockEstimatorMode)(bool enabled,
                                 void *context);

    /**
        The SLL interface function to get GNSS Energy Consumed. This interface API should be call
        by Synergy LOC API. This API will be called multiple times in different execution
        contexts. The execution context will identify using 'void *context' variable.

        @datatypes
        #loc_api_adapter_err

        @param context[Input]    Context Pointer of Synergy Location API. SLL should store
                                 and use any future interaction with Synergy location API.

        @return
            loc_api_adapter_err[Output]
                                 The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                 is successfully received and other status indicate as failure.

        @dependencies
            None.
    */
    enum loc_api_adapter_err (*sllGetGnssEnergyConsumed)(void *context);

    /**
        This API used to indicate Blacklisted SV to the GNSS hardware.
        GNSS hardware can avoid these blacklisted SV in position report calculation.
        This interface API should be call by Synergy LOC API.
        This API will be called multiple times in different execution contexts.
        The execution context will identify using ‘void *context’ variable.

        @datatypes
            #loc_api_adapter_err

        @param GnssSvIdConfig[Input] GNSS SV ID Configuration.

        @param context[Input]    Context Pointer of Synergy Location API.

        @return
            loc_api_adapter_err[Output]
                                 The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                 is successfully received and other status indicate as failure.

        @dependencies
            None.
    */
    enum loc_api_adapter_err (*sllSetBlacklistSv)(const GnssSvIdConfig& config, void *context);

    /**
        This API used to request to get Blacklisted SV from GNSS hardware.
        GNSS hardware indicate blacklisted SV using sllReportGnssSvIdConfig event.
        This interface API should be call by Synergy LOC API.
        This API will be called multiple times in different execution contexts.
        The execution context will identify using ‘void *context’ variable.

        @datatypes
        #loc_api_adapter_err

        @param context[Input]    Context Pointer of Synergy Location API.

        @return
            loc_api_adapter_err[Output]
                                 The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                 is successfully received and other status indicate as failure.

        @dependencies
            None.
    */
    enum loc_api_adapter_err (*sllGetBlacklistSv)(void *context);

    /**
        This API used to request to set supported constellation to GNSS hardware.
        This interface API should be call by Synergy LOC API.
        This API will be called multiple times in different execution contexts.
        The execution context will identify using ‘void *context’ variable.

        @datatypes
            #loc_api_adapter_err

        @param context[Input]    Context Pointer of Synergy Location API.

        @return
            loc_api_adapter_err[Output]
                                 The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                 is successfully received and other status indicate as failure.

        @dependencies
            None.
    */
    enum loc_api_adapter_err (*sllSetConstellationControl)(const GnssSvTypeConfig& config,
            void *context);

    /**
        This API used to request to get supported constellation from GNSS hardware.
        GNSS hardware indicate supported constellation using sllReportGnssSvTypeConfig event.
        This interface API should be call by Synergy LOC API.
        This API will be called multiple times in different execution contexts.
        The execution context will identify using ‘void *context’ variable.

        @datatypes
    #loc_api_adapter_err

        @param context[Input]    Context Pointer of Synergy Location API.

        @return
            loc_api_adapter_err[Output]
                                 The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                 is successfully received and other status indicate as failure.

        @dependencies
            None.
    */
    enum loc_api_adapter_err (*sllGetConstellationControl)(void *context);

    /**
        This API used to request to reset supported constellation from GNSS hardware.
        This interface API should be call by Synergy LOC API.
        This API will be called multiple times in different execution contexts.
        The execution context will identify using ‘void *context’ variable.

        @datatypes
            #loc_api_adapter_err

        @param context[Input]    Context Pointer of Synergy Location API.

        @return
            loc_api_adapter_err[Output]
                                 The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                 is successfully received and other status indicate as failure.

        @dependencies
            None.
    */
    enum loc_api_adapter_err (*sllResetConstellationControl)(void *context);


    /**
        This API used to request to XTRA configuration to GNSS hardware.
        This interface API should be call by Synergy LOC API.
        This API will be called multiple times in different execution contexts.
        The execution context will identify using ‘void *context’ variable.

        @datatypes
              #loc_api_adapter_err

        @param configMask[Input]      Configuration Mask.
        @param configReqSource[Input] Configuration Request Source.
        @param context[Input]         Context Pointer of Synergy Location API.

        @return
            loc_api_adapter_err[Output]
                                 The return status LOC_API_ADAPTER_ERR_SUCCESS indicate command
                                 is successfully received and other status indicate as failure.

        @dependencies
            None.
    */
    enum loc_api_adapter_err (*sllRequestXtraConfigInfo)(const uint32_t configMask,
                                             const uint32_t configReqSource, void *context);

} SllInterfaceReq;


#ifdef __cplusplus
extern "C" {
#endif
    /**
      The Synergy location layer(SLL) interface function provide commands and events callbacks to
      interact with SLL module.
      This interface API should be call by Synergy LOC API module and it should implement by SLL.
      This API will be called multiple times in different execution contexts.
      The execution context will identify using ‘void *context’ variable.

      @datatypes
      #SllInterfaceReq \n
      #SllInterfaceEvent

      @param eventCallback[Input]   These are event callbacks of SLL module. These events should
                                    implement by Synergy Location API.
                                    SLL module should store and call these event APIs.
      @param context[Input]         Context Pointer of Synergy Location API. SLL should store and
                                    use any future interaction with Synergy location API.

      @return
           SllInterfaceReq[Output]  These are Interface commands of SLL module. These commands
                                    should implement by SLL module.
                                    Synergy Location API module should store and call these
                                    command APIs.

      @dependencies
      None.
    */
    const SllInterfaceReq* get_sll_if_api(const SllInterfaceEvent* eventCallback, void *context);

    /**
      The Synergy location layer(SLL) interface function pointer provide commands and events
      callbacks to interact with SLL module. This interface API should be call by Synergy LOC
      API module, and it should implement by SLL.
      This API will be called multiple times in different execution contexts.
      The execution context will identify using ‘void *context’ variable.

      @datatypes
      #SllInterfaceReq \n
      #SllInterfaceEvent

      @param eventCallback[Input]   These are event callbacks of SLL module. These events
                                    should implement by Synergy Location API.
                                    SLL module should store and call these event APIs.
      @param context[Input]         Context Pointer of Synergy Location API. SLL should store
                                    and use any future interaction with Synergy location API.

      @return
           SllInterfaceReq[Output]  These are Interface commands of SLL module. These commands
                                    should implement by SLL module.
                                    Synergy Location API module should store and call these
                                    command APIs.

      @dependencies
           None.
    */
    typedef const SllInterfaceReq* (*get_sll_if_api_t)
                                       (const SllInterfaceEvent* eventCallback, void *context);

#ifdef __cplusplus
}
#endif

#endif /* LOC_SLL_INTERFACE_H */
