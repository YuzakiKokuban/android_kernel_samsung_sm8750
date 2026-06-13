/* Copyright (c) 2011-2016, 2018-2021 The Linux Foundation. All rights reserved.
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

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <pthread.h>
#include <stdbool.h>
#include <inttypes.h>

#include "qmi_client.h"
#include "qmi_idl_lib.h"
#include "qmi_cci_target_ext.h"

#if defined( _ANDROID_)
#include "qmi_cci_target.h"
#include "qmi_cci_common.h"
#define LOG_NDEBUG 0
#endif //_ANDROID_

#define LOG_TAG "LocSvc_api_v02"

#include <loc_pla.h>
#include "loc_api_v02_client.h"
#include "loc_util_log.h"
#include "loc_api_v02_log.h"

#include "loc_cfg.h"

#ifdef LOC_UTIL_TARGET_OFF_TARGET

// timeout in ms before send_msg_sync should return
#define LOC_CLIENT_ACK_TIMEOUT (5000)

#else

// timeout in ms before send_msg_sync should return
#define LOC_CLIENT_ACK_TIMEOUT (1000)

#endif //LOC_UTIL_TARGET_OFF_TARGET

#define LOC_CLIENT_MAX_OPEN_RETRIES (20)
#define LOC_CLIENT_TIME_BETWEEN_OPEN_RETRIES (1)

#define LOC_CLIENT_MAX_SYNC_RETRIES (50)
#define LOC_CLIENT_TIME_BETWEEN_SYNC_RETRIES (20*1000)

enum
{
  //! Special value for selecting any available service
  /** This value enables selection of any available service */
  eLOC_CLIENT_INSTANCE_ID_ANY = -1,
  //! qca1530 service id value
  /** qca1530 service daemon uses service id value 1 */
  eLOC_CLIENT_INSTANCE_ID_QCA1530 = 1,
  //! GSS service id value
  /* GSS service id value is 0, but here it is set to -1 for compatibitily */
  eLOC_CLIENT_INSTANCE_ID_GSS = eLOC_CLIENT_INSTANCE_ID_ANY,
  //! MSM service id value
  /** MSM service id value is 0, but here it is set to -1 for compatibitily */
  eLOC_CLIENT_INSTANCE_ID_MSM = eLOC_CLIENT_INSTANCE_ID_ANY,
  //! MDM service id value
  /** MDM connects using QMUXD and assigned a value of
      QMI_CLIENT_QMUX_RMNET_USB_INSTANCE_0 ("qmi_client_instance_defs.h", 37).
      -1 for compatibility */
  eLOC_CLIENT_INSTANCE_ID_MDM = eLOC_CLIENT_INSTANCE_ID_ANY,
  /*  GSS service id value is 0, for auto config  */
  eLOC_CLIENT_INSTANCE_ID_GSS_AUTO = 0,
  /* Loc Modem Emulator Service Instance */
  eLOC_CLIENT_INSTANCE_ID_MODEM_EMULATOR = 5,
};

/* Table to relate eventId, size and mask value used to enable the event*/
typedef struct
{
  uint32_t               eventId;
  size_t                 eventSize;
}locClientEventIndTableStructT;

pthread_mutex_t  loc_shutdown_mutex = PTHREAD_MUTEX_INITIALIZER;

static const locClientEventIndTableStructT locClientEventIndTable[]= {

  // position report ind
  { QMI_LOC_EVENT_POSITION_REPORT_IND_V02,
    sizeof(qmiLocEventPositionReportIndMsgT_v02) },

  // satellite report ind
  { QMI_LOC_EVENT_GNSS_SV_INFO_IND_V02,
    sizeof(qmiLocEventGnssSvInfoIndMsgT_v02) },

  //NI event ind
  { QMI_LOC_EVENT_NI_NOTIFY_VERIFY_REQ_IND_V02,
    sizeof(qmiLocEventNiNotifyVerifyReqIndMsgT_v02) },

  //Time Injection Request Ind
  { QMI_LOC_EVENT_INJECT_TIME_REQ_IND_V02,
    sizeof(qmiLocEventInjectTimeReqIndMsgT_v02) },

  //Predicted Orbits Injection Request
  { QMI_LOC_EVENT_INJECT_PREDICTED_ORBITS_REQ_IND_V02,
    sizeof(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02) },

  //Position Injection Request Ind
  { QMI_LOC_EVENT_INJECT_POSITION_REQ_IND_V02,
    sizeof(qmiLocEventInjectPositionReqIndMsgT_v02) },

  //Engine State Report Ind
  { QMI_LOC_EVENT_ENGINE_STATE_IND_V02,
    sizeof(qmiLocEventEngineStateIndMsgT_v02) },

  //Fix Session State Report Ind
  { QMI_LOC_EVENT_FIX_SESSION_STATE_IND_V02,
    sizeof(qmiLocEventFixSessionStateIndMsgT_v02) },

  //Wifi Request Indication
  { QMI_LOC_EVENT_WIFI_REQ_IND_V02,
    sizeof(qmiLocEventWifiReqIndMsgT_v02) },

  //Sensor Streaming Ready Status Ind
  { QMI_LOC_EVENT_SENSOR_STREAMING_READY_STATUS_IND_V02,
    sizeof(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02) },

  // Time Sync Request Indication
  { QMI_LOC_EVENT_TIME_SYNC_REQ_IND_V02,
    sizeof(qmiLocEventTimeSyncReqIndMsgT_v02) },

  //Set Spi Streaming Report Event
  { QMI_LOC_EVENT_SET_SPI_STREAMING_REPORT_IND_V02,
    sizeof(qmiLocEventSetSpiStreamingReportIndMsgT_v02) },

  //Location Server Connection Request event
  { QMI_LOC_EVENT_LOCATION_SERVER_CONNECTION_REQ_IND_V02,
    sizeof(qmiLocEventLocationServerConnectionReqIndMsgT_v02) },

  // NI Geofence Event
  { QMI_LOC_EVENT_NI_GEOFENCE_NOTIFICATION_IND_V02,
    sizeof(qmiLocEventNiGeofenceNotificationIndMsgT_v02) },

  // Geofence General Alert Event
  { QMI_LOC_EVENT_GEOFENCE_GEN_ALERT_IND_V02,
    sizeof(qmiLocEventGeofenceGenAlertIndMsgT_v02) },

  //Geofence Breach event
  { QMI_LOC_EVENT_GEOFENCE_BREACH_NOTIFICATION_IND_V02,
    sizeof(qmiLocEventGeofenceBreachIndMsgT_v02) },

  //Geofence Batched Breach event
  { QMI_LOC_EVENT_GEOFENCE_BATCHED_BREACH_NOTIFICATION_IND_V02,
    sizeof(qmiLocEventGeofenceBatchedBreachIndMsgT_v02) },

  //Pedometer Control event
  { QMI_LOC_EVENT_PEDOMETER_CONTROL_IND_V02,
    sizeof(qmiLocEventPedometerControlIndMsgT_v02) },

  //Motion Data Control event
  { QMI_LOC_EVENT_MOTION_DATA_CONTROL_IND_V02,
    sizeof(qmiLocEventMotionDataControlIndMsgT_v02) },

  //Wifi AP data request event
  { QMI_LOC_EVENT_INJECT_WIFI_AP_DATA_REQ_IND_V02,
    sizeof(qmiLocEventInjectWifiApDataReqIndMsgT_v02) },

  //Get Batching On Fix Event
  { QMI_LOC_EVENT_LIVE_BATCHED_POSITION_REPORT_IND_V02,
    sizeof(qmiLocEventLiveBatchedPositionReportIndMsgT_v02) },

  //Get Batching On Full Event
  { QMI_LOC_EVENT_BATCH_FULL_NOTIFICATION_IND_V02,
    sizeof(qmiLocEventBatchFullIndMsgT_v02) },

   //Vehicle Data Readiness event
   { QMI_LOC_EVENT_VEHICLE_DATA_READY_STATUS_IND_V02,
     sizeof(qmiLocEventVehicleDataReadyIndMsgT_v02) },

  //Geofence Proximity event
  { QMI_LOC_EVENT_GEOFENCE_PROXIMITY_NOTIFICATION_IND_V02,
    sizeof(qmiLocEventGeofenceProximityIndMsgT_v02) },

    //GNSS Measurement Indication
   { QMI_LOC_EVENT_GNSS_MEASUREMENT_REPORT_IND_V02,
     sizeof(qmiLocEventGnssSvMeasInfoIndMsgT_v02) },

    //GNSS Measurement Indication
   { QMI_LOC_EVENT_SV_POLYNOMIAL_REPORT_IND_V02,
    sizeof(qmiLocEventGnssSvPolyIndMsgT_v02) },

  // for GDT
  { QMI_LOC_EVENT_GDT_UPLOAD_BEGIN_STATUS_REQ_IND_V02,
    sizeof(qmiLocEventGdtUploadBeginStatusReqIndMsgT_v02) },

  { QMI_LOC_EVENT_GDT_UPLOAD_END_REQ_IND_V02,
    sizeof(qmiLocEventGdtUploadEndReqIndMsgT_v02) },

  { QMI_LOC_EVENT_DBT_POSITION_REPORT_IND_V02,
    sizeof(qmiLocEventDbtPositionReportIndMsgT_v02) },

  { QMI_LOC_EVENT_GEOFENCE_BATCHED_DWELL_NOTIFICATION_IND_V02,
    sizeof(qmiLocEventGeofenceBatchedDwellIndMsgT_v02) },

  { QMI_LOC_EVENT_GET_TIME_ZONE_INFO_IND_V02,
    sizeof(qmiLocEventGetTimeZoneReqIndMsgT_v02) },

  // Batching Status event
  { QMI_LOC_EVENT_BATCHING_STATUS_IND_V02,
    sizeof(qmiLocEventBatchingStatusIndMsgT_v02) },

  // TDP download
  { QMI_LOC_EVENT_GDT_DOWNLOAD_BEGIN_REQ_IND_V02,
    sizeof(qmiLocEventGdtDownloadBeginReqIndMsgT_v02) },

  { QMI_LOC_EVENT_GDT_RECEIVE_DONE_IND_V02,
    sizeof(qmiLocEventGdtReceiveDoneIndMsgT_v02) },

  { QMI_LOC_EVENT_GDT_DOWNLOAD_END_REQ_IND_V02,
    sizeof(qmiLocEventGdtDownloadEndReqIndMsgT_v02) },

  // SRN Ap data inject request
  { QMI_LOC_EVENT_INJECT_SRN_AP_DATA_REQ_IND_V02,
    sizeof(qmiLocEventInjectSrnApDataReqIndMsgT_v02) },

  { QMI_LOC_EVENT_FDCL_SERVICE_REQ_IND_V02,
    sizeof(qmiLocEventFdclServiceReqIndMsgT_v02) },

  // unpropagated position report ind
  { QMI_LOC_EVENT_UNPROPAGATED_POSITION_REPORT_IND_V02,
    sizeof(qmiLocEventPositionReportIndMsgT_v02) },

  { QMI_LOC_EVENT_BS_OBS_DATA_SERVICE_REQ_IND_V02,
    sizeof(qmiLocEventBsObsDataServiceReqIndMsgT_v02) },

   //GPS Ephemeris Indication
   { QMI_LOC_EVENT_GPS_EPHEMERIS_REPORT_IND_V02,
    sizeof(qmiLocGpsEphemerisReportIndMsgT_v02) },

   //GLONASS Ephemeris Indication
   { QMI_LOC_EVENT_GLONASS_EPHEMERIS_REPORT_IND_V02,
    sizeof(qmiLocGloEphemerisReportIndMsgT_v02)},

   //BDS Ephemeris Indication
   { QMI_LOC_EVENT_BDS_EPHEMERIS_REPORT_IND_V02,
    sizeof(qmiLocBdsEphemerisReportIndMsgT_v02)},

   //GAL Ephemeris Indication
   { QMI_LOC_EVENT_GALILEO_EPHEMERIS_REPORT_IND_V02,
    sizeof(qmiLocGalEphemerisReportIndMsgT_v02)},

   //QZSS Ephemeris Indication
   { QMI_LOC_EVENT_QZSS_EPHEMERIS_REPORT_IND_V02,
    sizeof(qmiLocQzssEphemerisReportIndMsgT_v02)},

    {QMI_LOC_EVENT_REPORT_IND_V02,
    sizeof(qmiLocEventReportIndMsgT_v02)},

  // loc system info event ind
  { QMI_LOC_SYSTEM_INFO_IND_V02,
    sizeof(qmiLocSystemInfoIndMsgT_v02)},

  // Power Metrics with multiband support
  { QMI_LOC_GET_BAND_MEASUREMENT_METRICS_IND_V02,
    sizeof(qmiLocGetBandMeasurementMetricsIndMsgT_v02)},

  // loc system info event ind
  { QMI_LOC_LOCATION_REQUEST_NOTIFICATION_IND_V02,
    sizeof(qmiLocLocationRequestNotificationIndMsgT_v02)},

  // XTRA config query request
  { QMI_LOC_EVENT_QUERY_XTRA_INFO_REQ_IND_V02,
    sizeof(qmiLocEventQueryXtraInfoReqIndMsgT_v02)},

  // Latency information ind
  { QMI_LOC_LATENCY_INFORMATION_IND_V02,
    sizeof(qmiLocLatencyInformationIndMsgT_v02)},

  // platform power state event ind
  { QMI_LOC_EVENT_PLATFORM_POWER_STATE_CHANGED_IND_V02,
    sizeof(qmiLocPlatformPowerStateChangedIndMsgT_v02)},

    // engine Lock information ind
  { QMI_LOC_EVENT_ENGINE_LOCK_STATE_IND_V02,
    sizeof(qmiLocEventEngineLockStateIndMsgT_v02)},

  // Engine Debug Data ind
  { QMI_LOC_ENGINE_DEBUG_DATA_IND_V02,
    sizeof(qmiLocEngineDebugDataIndMsgT_v02)},

  // disater and crisis report ind
  { QMI_LOC_DC_REPORT_IND_V02,
    sizeof(qmiLocEventDcReportIndMsgT_v02)},

  // supported bands and the preferred one ind
  { QMI_LOC_GNSS_BANDS_SUPPORTED_IND_V02,
    sizeof(qmiLocGnssBandsSupportedIndMsgT_v02)},
};

/* table to relate the respInd Id with its size */
typedef struct
{
  uint32_t respIndId;
  size_t   respIndSize;
}locClientRespIndTableStructT;

static const locClientRespIndTableStructT locClientRespIndTable[]= {

  // get service revision ind
  { QMI_LOC_GET_SERVICE_REVISION_IND_V02,
    sizeof(qmiLocGetServiceRevisionIndMsgT_v02)},

  // Get Fix Criteria Resp Ind
  { QMI_LOC_GET_FIX_CRITERIA_IND_V02,
     sizeof(qmiLocGetFixCriteriaIndMsgT_v02)},

  // NI User Resp In
  { QMI_LOC_NI_USER_RESPONSE_IND_V02,
    sizeof(qmiLocNiUserRespIndMsgT_v02)},

  //Inject Predicted Orbits Data Resp Ind
  { QMI_LOC_INJECT_PREDICTED_ORBITS_DATA_IND_V02,
    sizeof(qmiLocInjectPredictedOrbitsDataIndMsgT_v02)},

  //Get Predicted Orbits Data Src Resp Ind
  { QMI_LOC_GET_PREDICTED_ORBITS_DATA_SOURCE_IND_V02,
    sizeof(qmiLocGetPredictedOrbitsDataSourceIndMsgT_v02)},

  // Get Predicted Orbits Data Validity Resp Ind
   { QMI_LOC_GET_PREDICTED_ORBITS_DATA_VALIDITY_IND_V02,
     sizeof(qmiLocGetPredictedOrbitsDataValidityIndMsgT_v02)},

   // Inject UTC Time Resp Ind
   { QMI_LOC_INJECT_UTC_TIME_IND_V02,
     sizeof(qmiLocInjectUtcTimeIndMsgT_v02)},

   //Inject Position Resp Ind
   { QMI_LOC_INJECT_POSITION_IND_V02,
     sizeof(qmiLocInjectPositionIndMsgT_v02)},

   //Set Engine Lock Resp Ind
   { QMI_LOC_SET_ENGINE_LOCK_IND_V02,
     sizeof(qmiLocSetEngineLockIndMsgT_v02)},

   //Get Engine Lock Resp Ind
   { QMI_LOC_GET_ENGINE_LOCK_IND_V02,
     sizeof(qmiLocGetEngineLockIndMsgT_v02)},

   //Set SBAS Config Resp Ind
   { QMI_LOC_SET_SBAS_CONFIG_IND_V02,
     sizeof(qmiLocSetSbasConfigIndMsgT_v02)},

   //Get SBAS Config Resp Ind
   { QMI_LOC_GET_SBAS_CONFIG_IND_V02,
     sizeof(qmiLocGetSbasConfigIndMsgT_v02)},

   //Set Low Power Mode Resp Ind
   { QMI_LOC_SET_LOW_POWER_MODE_IND_V02,
     sizeof(qmiLocSetLowPowerModeIndMsgT_v02)},

   //Get Low Power Mode Resp Ind
   { QMI_LOC_GET_LOW_POWER_MODE_IND_V02,
     sizeof(qmiLocGetLowPowerModeIndMsgT_v02)},

   //Set Server Resp Ind
   { QMI_LOC_SET_SERVER_IND_V02,
     sizeof(qmiLocSetServerIndMsgT_v02)},

   //Get Server Resp Ind
   { QMI_LOC_GET_SERVER_IND_V02,
     sizeof(qmiLocGetServerIndMsgT_v02)},

    //Delete Assist Data Resp Ind
   { QMI_LOC_DELETE_ASSIST_DATA_IND_V02,
     sizeof(qmiLocDeleteAssistDataIndMsgT_v02)},

   //Set AP cache injection Resp Ind
   { QMI_LOC_INJECT_APCACHE_DATA_IND_V02,
     sizeof(qmiLocInjectApCacheDataIndMsgT_v02)},

   //Set No AP cache injection Resp Ind
   { QMI_LOC_INJECT_APDONOTCACHE_DATA_IND_V02,
     sizeof(qmiLocInjectApDoNotCacheDataIndMsgT_v02)},

   //Set XTRA-T Session Control Resp Ind
   { QMI_LOC_SET_XTRA_T_SESSION_CONTROL_IND_V02,
     sizeof(qmiLocSetXtraTSessionControlIndMsgT_v02)},

   //Get XTRA-T Session Control Resp Ind
   { QMI_LOC_GET_XTRA_T_SESSION_CONTROL_IND_V02,
     sizeof(qmiLocGetXtraTSessionControlIndMsgT_v02)},

   //Inject Wifi Position Resp Ind
   { QMI_LOC_INJECT_WIFI_POSITION_IND_V02,
     sizeof(qmiLocInjectWifiPositionIndMsgT_v02)},

   //Notify Wifi Status Resp Ind
   { QMI_LOC_NOTIFY_WIFI_STATUS_IND_V02,
     sizeof(qmiLocNotifyWifiStatusIndMsgT_v02)},

   //Get Registered Events Resp Ind
   { QMI_LOC_GET_REGISTERED_EVENTS_IND_V02,
     sizeof(qmiLocGetRegisteredEventsIndMsgT_v02)},

   //Set Operation Mode Resp Ind
   { QMI_LOC_SET_OPERATION_MODE_IND_V02,
     sizeof(qmiLocSetOperationModeIndMsgT_v02)},

   //Get Operation Mode Resp Ind
   { QMI_LOC_GET_OPERATION_MODE_IND_V02,
     sizeof(qmiLocGetOperationModeIndMsgT_v02)},

   //Set SPI Status Resp Ind
   { QMI_LOC_SET_SPI_STATUS_IND_V02,
     sizeof(qmiLocSetSpiStatusIndMsgT_v02)},

   //Inject Sensor Data Resp Ind
   { QMI_LOC_INJECT_SENSOR_DATA_IND_V02,
     sizeof(qmiLocInjectSensorDataIndMsgT_v02)},

   //Inject Time Sync Data Resp Ind
   { QMI_LOC_INJECT_TIME_SYNC_DATA_IND_V02,
     sizeof(qmiLocInjectTimeSyncDataIndMsgT_v02)},

   //Set Cradle Mount config Resp Ind
   { QMI_LOC_SET_CRADLE_MOUNT_CONFIG_IND_V02,
     sizeof(qmiLocSetCradleMountConfigIndMsgT_v02)},

   //Get Cradle Mount config Resp Ind
   { QMI_LOC_GET_CRADLE_MOUNT_CONFIG_IND_V02,
     sizeof(qmiLocGetCradleMountConfigIndMsgT_v02)},

   //Set External Power config Resp Ind
   { QMI_LOC_SET_EXTERNAL_POWER_CONFIG_IND_V02,
     sizeof(qmiLocSetExternalPowerConfigIndMsgT_v02)},

   //Get External Power config Resp Ind
   { QMI_LOC_GET_EXTERNAL_POWER_CONFIG_IND_V02,
     sizeof(qmiLocGetExternalPowerConfigIndMsgT_v02)},

   //Location server connection status
   { QMI_LOC_INFORM_LOCATION_SERVER_CONN_STATUS_IND_V02,
     sizeof(qmiLocInformLocationServerConnStatusIndMsgT_v02)},

   //Set Protocol Config Parameters
   { QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_IND_V02,
     sizeof(qmiLocSetProtocolConfigParametersIndMsgT_v02)},

   //Get Protocol Config Parameters
   { QMI_LOC_GET_PROTOCOL_CONFIG_PARAMETERS_IND_V02,
     sizeof(qmiLocGetProtocolConfigParametersIndMsgT_v02)},

   //Set Sensor Control Config
   { QMI_LOC_SET_SENSOR_CONTROL_CONFIG_IND_V02,
     sizeof(qmiLocSetSensorControlConfigIndMsgT_v02)},

   //Get Sensor Control Config
   { QMI_LOC_GET_SENSOR_CONTROL_CONFIG_IND_V02,
     sizeof(qmiLocGetSensorControlConfigIndMsgT_v02)},

   //Set Sensor Properties
   { QMI_LOC_SET_SENSOR_PROPERTIES_IND_V02,
     sizeof(qmiLocSetSensorPropertiesIndMsgT_v02)},

   //Get Sensor Properties
   { QMI_LOC_GET_SENSOR_PROPERTIES_IND_V02,
     sizeof(qmiLocGetSensorPropertiesIndMsgT_v02)},

   //Set Sensor Performance Control Config
   { QMI_LOC_SET_SENSOR_PERFORMANCE_CONTROL_CONFIGURATION_IND_V02,
     sizeof(qmiLocSetSensorPerformanceControlConfigIndMsgT_v02)},

   //Get Sensor Performance Control Config
   { QMI_LOC_GET_SENSOR_PERFORMANCE_CONTROL_CONFIGURATION_IND_V02,
     sizeof(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02)},
   //Inject SUPL certificate
   { QMI_LOC_INJECT_SUPL_CERTIFICATE_IND_V02,
     sizeof(qmiLocInjectSuplCertificateIndMsgT_v02) },

   //Delete SUPL certificate
   { QMI_LOC_DELETE_SUPL_CERTIFICATE_IND_V02,
     sizeof(qmiLocDeleteSuplCertificateIndMsgT_v02) },

   // Set Position Engine Config
   { QMI_LOC_SET_POSITION_ENGINE_CONFIG_PARAMETERS_IND_V02,
     sizeof(qmiLocSetPositionEngineConfigParametersIndMsgT_v02)},

   // Get Position Engine Config
   { QMI_LOC_GET_POSITION_ENGINE_CONFIG_PARAMETERS_IND_V02,
     sizeof(qmiLocGetPositionEngineConfigParametersIndMsgT_v02)},

   //Add a Circular Geofence
   { QMI_LOC_ADD_CIRCULAR_GEOFENCE_IND_V02,
     sizeof(qmiLocAddCircularGeofenceIndMsgT_v02)},

   //Delete a Geofence
   { QMI_LOC_DELETE_GEOFENCE_IND_V02,
     sizeof(qmiLocDeleteGeofenceIndMsgT_v02)} ,

   //Query a Geofence
   { QMI_LOC_QUERY_GEOFENCE_IND_V02,
     sizeof(qmiLocQueryGeofenceIndMsgT_v02)},

   //Edit a Geofence
   { QMI_LOC_EDIT_GEOFENCE_IND_V02,
     sizeof(qmiLocEditGeofenceIndMsgT_v02)},

   //Get best available position
   { QMI_LOC_GET_BEST_AVAILABLE_POSITION_IND_V02,
     sizeof(qmiLocGetBestAvailablePositionIndMsgT_v02)},

   //Secure Get available position
   { QMI_LOC_SECURE_GET_AVAILABLE_POSITION_IND_V02,
     sizeof(qmiLocSecureGetAvailablePositionIndMsgT_v02)},

   //Inject motion data
   { QMI_LOC_INJECT_MOTION_DATA_IND_V02,
     sizeof(qmiLocInjectMotionDataIndMsgT_v02)},

   //Get NI Geofence list
   { QMI_LOC_GET_NI_GEOFENCE_ID_LIST_IND_V02,
     sizeof(qmiLocGetNiGeofenceIdListIndMsgT_v02)},

   //Inject GSM Cell Info
   { QMI_LOC_INJECT_GSM_CELL_INFO_IND_V02,
     sizeof(qmiLocInjectGSMCellInfoIndMsgT_v02)},

   //Inject Network Initiated Message
   { QMI_LOC_INJECT_NETWORK_INITIATED_MESSAGE_IND_V02,
     sizeof(qmiLocInjectNetworkInitiatedMessageIndMsgT_v02)},

   //WWAN Out of Service Notification
   { QMI_LOC_WWAN_OUT_OF_SERVICE_NOTIFICATION_IND_V02,
     sizeof(qmiLocWWANOutOfServiceNotificationIndMsgT_v02)},

   //Pedomete Report
   { QMI_LOC_PEDOMETER_REPORT_IND_V02,
     sizeof(qmiLocPedometerReportIndMsgT_v02)},

   { QMI_LOC_INJECT_WCDMA_CELL_INFO_IND_V02,
     sizeof(qmiLocInjectWCDMACellInfoIndMsgT_v02)},

   { QMI_LOC_INJECT_TDSCDMA_CELL_INFO_IND_V02,
     sizeof(qmiLocInjectTDSCDMACellInfoIndMsgT_v02)},

   { QMI_LOC_INJECT_SUBSCRIBER_ID_IND_V02,
     sizeof(qmiLocInjectSubscriberIDIndMsgT_v02)},

   //Inject Wifi AP data Resp Ind
   { QMI_LOC_INJECT_WIFI_AP_DATA_IND_V02,
     sizeof(qmiLocInjectWifiApDataIndMsgT_v02)},

   { QMI_LOC_START_BATCHING_IND_V02,
     sizeof(qmiLocStartBatchingIndMsgT_v02)},

   { QMI_LOC_STOP_BATCHING_IND_V02,
     sizeof(qmiLocStopBatchingIndMsgT_v02)},

   { QMI_LOC_GET_BATCH_SIZE_IND_V02,
     sizeof(qmiLocGetBatchSizeIndMsgT_v02)},

   { QMI_LOC_EVENT_LIVE_BATCHED_POSITION_REPORT_IND_V02,
     sizeof(qmiLocEventLiveBatchedPositionReportIndMsgT_v02)},

   { QMI_LOC_EVENT_BATCH_FULL_NOTIFICATION_IND_V02,
     sizeof(qmiLocEventBatchFullIndMsgT_v02)},

   { QMI_LOC_READ_FROM_BATCH_IND_V02,
     sizeof(qmiLocReadFromBatchIndMsgT_v02)},

   { QMI_LOC_RELEASE_BATCH_IND_V02,
     sizeof(qmiLocReleaseBatchIndMsgT_v02)},

   { QMI_LOC_SET_XTRA_VERSION_CHECK_IND_V02,
     sizeof(qmiLocSetXtraVersionCheckIndMsgT_v02)},

    //Vehicle Sensor Data
    { QMI_LOC_INJECT_VEHICLE_SENSOR_DATA_IND_V02,
      sizeof(qmiLocInjectVehicleSensorDataIndMsgT_v02)},

   { QMI_LOC_NOTIFY_WIFI_ATTACHMENT_STATUS_IND_V02,
     sizeof(qmiLocNotifyWifiAttachmentStatusIndMsgT_v02)},

   { QMI_LOC_NOTIFY_WIFI_ENABLED_STATUS_IND_V02,
     sizeof(qmiLocNotifyWifiEnabledStatusIndMsgT_v02)},

   { QMI_LOC_SET_PREMIUM_SERVICES_CONFIG_IND_V02,
     sizeof(qmiLocSetPremiumServicesCfgIndMsgT_v02)},

   { QMI_LOC_GET_AVAILABLE_WWAN_POSITION_IND_V02,
     sizeof(qmiLocGetAvailWwanPositionIndMsgT_v02)},

   // for TDP
   { QMI_LOC_INJECT_GTP_CLIENT_DOWNLOADED_DATA_IND_V02,
     sizeof(qmiLocInjectGtpClientDownloadedDataIndMsgT_v02) },

   // for GDT
   { QMI_LOC_GDT_UPLOAD_BEGIN_STATUS_IND_V02,
     sizeof(qmiLocGdtUploadBeginStatusIndMsgT_v02) },

   { QMI_LOC_GDT_UPLOAD_END_IND_V02,
     sizeof(qmiLocGdtUploadEndIndMsgT_v02) },

   { QMI_LOC_SET_GNSS_CONSTELL_REPORT_CONFIG_IND_V02,
     sizeof(qmiLocSetGNSSConstRepConfigIndMsgT_v02)},

   { QMI_LOC_START_DBT_IND_V02,
     sizeof(qmiLocStartDbtIndMsgT_v02)},

   { QMI_LOC_STOP_DBT_IND_V02,
     sizeof(qmiLocStopDbtIndMsgT_v02)},

   { QMI_LOC_INJECT_TIME_ZONE_INFO_IND_V02,
     sizeof(qmiLocInjectTimeZoneInfoIndMsgT_v02)},

   { QMI_LOC_QUERY_AON_CONFIG_IND_V02,
     sizeof(qmiLocQueryAonConfigIndMsgT_v02)},

    // for GTP
   { QMI_LOC_GTP_AP_STATUS_IND_V02,
     sizeof(qmiLocGtpApStatusIndMsgT_v02) },

    // for GDT
   { QMI_LOC_GDT_DOWNLOAD_BEGIN_STATUS_IND_V02,
     sizeof(qmiLocGdtDownloadBeginStatusIndMsgT_v02) },

   { QMI_LOC_GDT_DOWNLOAD_READY_STATUS_IND_V02,
    sizeof(qmiLocGdtDownloadReadyStatusIndMsgT_v02) },

   { QMI_LOC_GDT_RECEIVE_DONE_STATUS_IND_V02,
    sizeof(qmiLocGdtReceiveDoneStatusIndMsgT_v02) },

   { QMI_LOC_GDT_DOWNLOAD_END_STATUS_IND_V02,
     sizeof(qmiLocGdtDownloadEndStatusIndMsgT_v02) },

   { QMI_LOC_GET_SUPPORTED_FEATURE_IND_V02,
     sizeof(qmiLocGetSupportedFeatureIndMsgT_v02) },

   //Delete Gnss Service Data Resp Ind
   { QMI_LOC_DELETE_GNSS_SERVICE_DATA_IND_V02,
     sizeof(qmiLocDeleteGNSSServiceDataIndMsgT_v02) },

   // for XTRA Client 2.0
   { QMI_LOC_INJECT_XTRA_DATA_IND_V02,
     sizeof(qmiLocInjectXtraDataIndMsgT_v02) },

   { QMI_LOC_INJECT_XTRA_PCID_IND_V02,
     sizeof(qmiLocInjectXtraPcidIndMsgT_v02) },

   // SRN Ap data inject
   { QMI_LOC_INJECT_SRN_AP_DATA_IND_V02,
     sizeof(qmiLocInjectSrnApDataIndMsgT_v02) },

  // for Fusion CSM
   { QMI_LOC_CROWDSOURCE_MANAGER_CONTROL_IND_V02,
     sizeof(qmiLocCrowdSourceManagerControlIndMsgT_v02) },

   //xtra config data
   { QMI_LOC_QUERY_XTRA_INFO_IND_V02,
     sizeof(qmiLocQueryXtraInfoIndMsgT_v02) },

   { QMI_LOC_START_OUTDOOR_TRIP_BATCHING_IND_V02,
     sizeof(qmiLocStartOutdoorTripBatchingIndMsgT_v02) },

   { QMI_LOC_QUERY_OTB_ACCUMULATED_DISTANCE_IND_V02,
     sizeof(qmiLocQueryOTBAccumulatedDistanceIndMsgT_v02) },

   { QMI_LOC_GET_FDCL_BS_LIST_IND_V02,
     sizeof(qmiLocGetFdclBsListIndMsgT_v02) },

   { QMI_LOC_INJECT_FDCL_DATA_IND_V02,
     sizeof(qmiLocInjectFdclDataIndMsgT_v02) },

   { QMI_LOC_GET_BS_OBS_DATA_IND_V02,
     sizeof(qmiLocGetBsObsDataIndMsgT_v02) },

   { QMI_LOC_SET_BLACKLIST_SV_IND_V02,
     sizeof(qmiLocGenReqStatusIndMsgT_v02) },

    // register master
   { QMI_LOC_REGISTER_MASTER_CLIENT_IND_V02,
     sizeof(qmiLocRegisterMasterClientIndMsgT_v02) },

   { QMI_LOC_GET_BLACKLIST_SV_IND_V02,
     sizeof(qmiLocGetBlacklistSvIndMsgT_v02) },

   { QMI_LOC_SET_CONSTELLATION_CONTROL_IND_V02,
     sizeof(qmiLocGenReqStatusIndMsgT_v02) },

   { QMI_LOC_GET_CONSTELLATION_CONTROL_IND_V02,
     sizeof(qmiLocGetConstellationConfigIndMsgT_v02) },

   { QMI_LOC_SET_CONSTRAINED_TUNC_MODE_IND_V02,
     sizeof(qmiLocSetConstrainedTuncModeIndMsgT_v02) },

   { QMI_LOC_ENABLE_POSITION_ASSISTED_CLOCK_EST_IND_V02,
     sizeof(qmiLocEnablePositionAssistedClockEstIndMsgT_v02) },

   { QMI_LOC_QUERY_GNSS_ENERGY_CONSUMED_IND_V02,
     sizeof(qmiLocQueryGNSSEnergyConsumedIndMsgT_v02) },

   { QMI_LOC_INJECT_PLATFORM_POWER_STATE_IND_V02,
     sizeof(qmiLocInjectPlatformPowerStateIndMsgT_v02) },

   { QMI_LOC_SET_ROBUST_LOCATION_CONFIG_IND_V02,
     sizeof(qmiLocGenReqStatusIndMsgT_v02) },

   { QMI_LOC_GET_ROBUST_LOCATION_CONFIG_IND_V02,
     sizeof(qmiLocGetRobustLocationConfigIndMsgT_v02) },

   { QMI_LOC_INJECT_ENV_AIDING_IND_V02,
     sizeof(qmiLocGenReqStatusIndMsgT_v02) },

   { QMI_LOC_SET_MIN_GPS_WEEK_NUMBER_IND_V02,
     sizeof(qmiLocGenReqStatusIndMsgT_v02) },

   { QMI_LOC_GET_MIN_GPS_WEEK_NUMBER_IND_V02,
     sizeof(qmiLocGetMinGpsWeekNumberIndMsgT_v02) },

   { QMI_LOC_SET_PARAMETER_IND_V02,
     sizeof(qmiLocGenReqStatusIndMsgT_v02) },

   { QMI_LOC_GET_PARAMETER_IND_V02,
     sizeof(qmiLocGetParameterIndMsgT_v02) },

   { QMI_LOC_SET_MULTIBAND_CONFIG_IND_V02,
     sizeof(qmiLocGenReqStatusIndMsgT_v02) },

   { QMI_LOC_GET_MULTIBAND_CONFIG_IND_V02,
     sizeof(qmiLocGetMultibandConfigIndMsgT_v02) },

   { QMI_LOC_INJECT_LOCATION_CIVIC_ADDRESS_IND_V02,
     sizeof(qmiLocGenReqStatusIndMsgT_v02) },

   { QMI_LOC_SET_TRIBAND_STATE_IND_V02,
     sizeof(qmiLocGenReqStatusIndMsgT_v02) },

   { QMI_LOC_GET_TRIBAND_STATE_IND_V02,
     sizeof(qmiLocGetTribandStateIndMsgT_v02) },

   { QMI_LOC_SET_SDK_FEATURE_CONFIG_IND_V02,
     sizeof(qmiLocSetSdkFeatureConfigIndMsgT_v02) },

   { QMI_LOC_OSNMA_PUBLIC_KEY_MERKLE_TREE_IND_V02,
     sizeof(qmiLocGenReqStatusIndMsgT_v02) },

   { QMI_LOC_SET_OSNMA_STATE_IND_V02,
     sizeof(qmiLocGenReqStatusIndMsgT_v02) },

   { QMI_LOC_SET_NTN_STATUS_IND_V02,
     sizeof(qmiLocSetNtnStatusIndMsgT_v02) },

   { QMI_LOC_GET_NTN_STATUS_IND_V02,
     sizeof(qmiLocGetNtnStatusIndMsgT_v02) },

   { QMI_LOC_NTN_CONFIG_UPDATE_IND_V02,
     sizeof(qmiLocNtnConfigUpdateIndMsgT_v02) },
};


/** whether indication is an event or a response */
typedef enum { eventIndType =0, respIndType = 1 } locClientIndEnumT;


/** @struct locClientInternalState
 */

typedef struct locClientCbDataStructT locClientCallbackDataType;

struct locClientCbDataStructT
{
 // client cookie
  void *pClientCookie;
  //QCCI handle for this control point
  qmi_client_type userHandle;

  // callbacks registered by the clients
  locClientEventIndCbType eventCallback;
  locClientRespIndCbType respCallback;
  locClientErrorCbType   errorCallback;

  // the event mask the client has registered for
  locClientEventMaskType eventRegMask;

  //pointer to itself for checking consistency data
   locClientCallbackDataType *pMe;
};

static uint32_t LOC_MODEM_EMULATOR = 0;
static const loc_param_s_type loc_cfgs[] =
{
    {"LOC_MODEM_EMULATOR", &LOC_MODEM_EMULATOR, NULL,    'n'},
};

static int getEmulatorCfg() {
    static bool getEmulatorCfg_called = false;
    if (!getEmulatorCfg_called) {
        getEmulatorCfg_called = true;
        UTIL_READ_CONF(LOC_PATH_GPS_CONF, loc_cfgs);
    }
    return LOC_MODEM_EMULATOR;
}

/*===========================================================================
 *
 *                          FUNCTION DECLARATION
 *
 *==========================================================================*/

/** locClientGetSizeAndTypeByIndId
 *  @brief this function gets the size and the type (event,
 *         response)of the indication structure from its ID
 *  @param [in]  indId  ID of the indication
 *  @param [out] type   event or response indication
 *  @param [out] size   size of the indications
 *
 *  @return true if the ID was found, false otherwise */

static bool locClientGetSizeAndTypeByIndId (uint32_t indId, size_t *pIndSize,
                                         locClientIndEnumT *pIndType)
{
  // look in the event table
  if(true == locClientGetSizeByEventIndId(indId, pIndSize))
  {
    *pIndType = eventIndType;

    return true;
  }

  //else look in response table
  if(true == locClientGetSizeByRespIndId(indId, pIndSize))
  {
    *pIndType = respIndType;

    return true;
  }

  // Id not found
  LOC_LOGw("indId %d not found\n", indId);
  return false;
}

/** checkQmiMsgsSupported
 @brief check the qmi service is supported or not.
 @param [in] pResponse  pointer to the response received from
        QMI_LOC service.
*/
static void checkQmiMsgsSupported(
  const uint32_t*          reqIdArray,
  int                      reqIdArrayLength,
  qmiLocGetSupportMsgT_v02 *pResponse,
  uint64_t*                supportedMsg)
{
    uint64_t result = 0;
    if (pResponse->resp.supported_msgs_valid) {

        /* For example, if a service supports exactly four messages with
        IDs 0, 1, 30, and 31 (decimal), the array (in hexadecimal) is
        4 bytes [03 00 00 c0]. */

        size_t idx = 0;
        uint32_t reqId = 0;
        uint32_t length = 0;
        uint32_t supportedMsgsLen = pResponse->resp.supported_msgs_len;

        // every bit saves a checked message result
        uint32_t maxCheckedMsgsSavedNum = sizeof(result)<<3;

        uint32_t loopSize = reqIdArrayLength;
        loopSize =
            loopSize < supportedMsgsLen ? loopSize : supportedMsgsLen;
        loopSize =
            loopSize < maxCheckedMsgsSavedNum ? loopSize : maxCheckedMsgsSavedNum;

        for (idx = 0; idx < loopSize; idx++) {
            reqId = reqIdArray[idx];
            length = reqId >> 3;
            if(supportedMsgsLen > length) {
                uint32_t bit = reqId & ((uint32_t)7);
                if (pResponse->resp.supported_msgs[length] & (1<<bit)) {
                    result |= ( 1 << idx ) ;
                }
            }
        }
    } else {
        LOC_LOGe("Invalid supported message list.");
    }
    *supportedMsg = result;
}

/** convertQmiResponseToLocStatus
 @brief converts a qmiLocGenRespMsgT to locClientStatusEnumType*
 @param [in] pResponse; pointer to the response received from
        QMI_LOC service.
 @return locClientStatusEnumType corresponding to the
         response.
*/

static locClientStatusEnumType convertQmiResponseToLocStatus(
  qmiLocGenRespMsgT_v02 *pResponse)
{
  locClientStatusEnumType status =  eLOC_CLIENT_FAILURE_INTERNAL;

  // if result == SUCCESS don't look at error code
  if(pResponse->resp.result == QMI_RESULT_SUCCESS_V01 )
  {
    status  = eLOC_CLIENT_SUCCESS;
  }
  else
  {
    switch(pResponse->resp.error)
    {
      case QMI_ERR_MALFORMED_MSG_V01:
      case QMI_ERR_INVALID_ARG_V01:
        status = eLOC_CLIENT_FAILURE_INVALID_PARAMETER;
        break;

      case QMI_ERR_DEVICE_IN_USE_V01:
        status = eLOC_CLIENT_FAILURE_ENGINE_BUSY;
        break;

      case QMI_ERR_NOT_SUPPORTED_V01:
        status = eLOC_CLIENT_FAILURE_UNSUPPORTED;
        break;

      case QMI_ERR_INVALID_MESSAGE_ID_V01:
        status = eLOC_CLIENT_FAILURE_INVALID_MESSAGE_ID;
        break;

      default:
        status = eLOC_CLIENT_FAILURE_INTERNAL;
        break;
    }
  }
  return status;
}

/** convertQmiErrorToLocError
 @brief converts a qmi service error type to
        locClientErrorEnumType
 @param [in] error received QMI service.
 @return locClientErrorEnumType corresponding to the error.
*/

static locClientErrorEnumType convertQmiErrorToLocError(
  qmi_client_error_type error)
{
  locClientErrorEnumType locError;
  switch(error)
  {
    case QMI_SERVICE_ERR:
      locError = eLOC_CLIENT_ERROR_SERVICE_UNAVAILABLE;
      break;

    default:
      locError = eLOC_CLIENT_ERROR_SERVICE_UNAVAILABLE;
      break;
  }
  return locError;
}

/** locClientErrorCb
 *  @brief handles the QCCI error events, this is called by the
 *         QCCI infrastructure when the service is no longer
 *         available.
 *  @param [in] user handle
 *  @param [in] error
 *  @param [in] *err_cb_data
 */

static void locClientErrorCb
(
  qmi_client_type user_handle,
  qmi_client_error_type error,
  void *err_cb_data
)
{
  (void)user_handle;

  locClientCallbackDataType* pCallbackData =
        (locClientCallbackDataType *)err_cb_data;
  locClientErrorCbType localErrorCallback = NULL;

  /* copy the errorCallback function pointer from the callback
   * data to local variable. This is to protect against the race
   * condition between open/close and error callback.
   */
  if(NULL != pCallbackData)
  {
    localErrorCallback = pCallbackData->errorCallback;
  }

  LOC_LOGd("Service Error %d received, pCallbackData = %p",
           error, err_cb_data);

  /* call the error callback
   * To avoid calling the errorCallback after locClientClose
   * is called, check pCallbackData->errorCallback again here
   */

  if( (NULL != pCallbackData) &&
      (NULL != localErrorCallback) &&
      (NULL != pCallbackData->errorCallback) &&
      (pCallbackData == pCallbackData->pMe)  )
  {
    pthread_mutex_lock(&loc_shutdown_mutex);
    //invoke the error callback for the corresponding client
    localErrorCallback(
        (locClientHandleType)pCallbackData,
        convertQmiErrorToLocError(error),
        pCallbackData->pClientCookie);
    pthread_mutex_unlock(&loc_shutdown_mutex);
  }
}


/** locClientIndCb
 *  @brief handles the indications sent from the service, if a
 *         response indication was received then the it is sent
 *         to the response callback. If a event indication was
 *         received then it is sent to the event callback
 *  @param [in] user handle
 *  @param [in] msg_id
 *  @param [in] ind_buf
 *  @param [in] ind_buf_len
 *  @param [in] ind_cb_data */

static void locClientIndCb
(
 qmi_client_type                user_handle,
 unsigned int                   msg_id,
 void                           *ind_buf,
 unsigned int                   ind_buf_len,
 void                           *ind_cb_data
)
{
  locClientIndEnumT indType;
  size_t indSize = 0;
  qmi_client_error_type rc ;

  locClientCallbackDataType* pCallbackData =
      (locClientCallbackDataType *)ind_cb_data;

  // check callback data
  if(NULL == pCallbackData ||(pCallbackData != pCallbackData->pMe))
  {
    LOC_LOGe("invalid callback data");
    return;
  }

  // check user handle
  if(memcmp(&pCallbackData->userHandle, &user_handle, sizeof(user_handle)))
  {
    LOC_LOGe("invalid user_handle got %p expected %p\n",
             user_handle, pCallbackData->userHandle);
    return;
  }
  // Get the indication size and type ( eventInd or respInd)
  if( true == locClientGetSizeAndTypeByIndId(msg_id, &indSize, &indType))
  {
    void *indBuffer = NULL;

    // decode the indication
    indBuffer = malloc(indSize);

    if(NULL == indBuffer)
    {
      LOC_LOGe("memory allocation failed");
      return;
    }
    memset(indBuffer, 0, indSize);

    rc = QMI_NO_ERR;

    if (ind_buf_len > 0)
    {
        // decode the indication
        rc = qmi_client_message_decode(
            user_handle,
            QMI_IDL_INDICATION,
            msg_id,
            ind_buf,
            ind_buf_len,
            indBuffer,
            indSize);
    }

    if( rc == QMI_NO_ERR )
    {
      if(eventIndType == indType)
      {
        locClientEventIndUnionType eventIndUnion;

        /* copy the eventCallback function pointer from the callback
         * data to local variable. This is to protect against the race
         * condition between open/close and indication callback.
         */
        locClientEventIndCbType localEventCallback =
            pCallbackData->eventCallback;

        // dummy event
        eventIndUnion.pPositionReportEvent =
            (qmiLocEventPositionReportIndMsgT_v02 *)indBuffer;

        /* call the event callback
         * To avoid calling the eventCallback after locClientClose
         * is called, check pCallbackData->eventCallback again here
         */
        if((NULL != localEventCallback) &&
           (NULL != pCallbackData->eventCallback))
        {
          pthread_mutex_lock(&loc_shutdown_mutex);
          localEventCallback(
              (locClientHandleType)pCallbackData,
              msg_id,
              eventIndUnion,
              pCallbackData->pClientCookie);
          pthread_mutex_unlock(&loc_shutdown_mutex);
        }
      }
      else if(respIndType == indType)
      {
        locClientRespIndUnionType respIndUnion;

        /* copy the respCallback function pointer from the callback
         * data to local variable. This is to protect against the race
         * condition between open/close and indication callback.
         */
        locClientRespIndCbType localRespCallback =
            pCallbackData->respCallback;

        // dummy to suppress compiler warnings
        respIndUnion.pDeleteAssistDataInd =
            (qmiLocDeleteAssistDataIndMsgT_v02 *)indBuffer;

        /* call the response callback
         * To avoid calling the respCallback after locClientClose
         * is called, check pCallbackData->respCallback again here
         */
        if((NULL != localRespCallback) &&
           (NULL != pCallbackData->respCallback))
        {
          pthread_mutex_lock(&loc_shutdown_mutex);
          localRespCallback(
              (locClientHandleType)pCallbackData,
              msg_id,
              respIndUnion,
              indSize,
              pCallbackData->pClientCookie);
          pthread_mutex_unlock(&loc_shutdown_mutex);
        }
      }
    }
    else
    {
      LOC_LOGe("Error decoding indication error: %d", rc);
    }
    if(indBuffer)
    {
      free (indBuffer);
    }
  }
  else // Id not found
  {
    LOC_LOGe("Error indication not found for msg id %d", (uint32_t)msg_id);
  }
  return;
}


/** locClientRegisterEventMask
 *  @brief registers the event mask with loc service
 *  @param [in] clientHandle
 *  @param [in] eventRegMask
 *  @return true if indication was validated; else false */

bool locClientRegisterEventMask(
    locClientHandleType clientHandle,
    locClientEventMaskType eventRegMask,
    bool bIsMaster)
{
  locClientStatusEnumType status = eLOC_CLIENT_SUCCESS;
  locClientReqUnionType reqUnion;
  qmiLocRegEventsReqMsgT_v02 regEventsReq;

  memset(&regEventsReq, 0, sizeof(regEventsReq));

  regEventsReq.eventRegMask = eventRegMask;
  regEventsReq.clientStrId_valid = true;
  if (bIsMaster) {
      LOC_LOGv("master hal %s", MASTER_HAL);
      strlcpy(regEventsReq.clientStrId, MASTER_HAL,
              sizeof(regEventsReq.clientStrId));
  }
  else {
      LOC_LOGv("hal %s", HAL);
      strlcpy(regEventsReq.clientStrId, HAL,
              sizeof(regEventsReq.clientStrId));
  }

  regEventsReq.clientType_valid = true;
  regEventsReq.clientType = eQMI_LOC_CLIENT_AFW_V02;
  regEventsReq.enablePosRequestNotification_valid = true;
  regEventsReq.enablePosRequestNotification = false;

  reqUnion.pRegEventsReq = &regEventsReq;

  status = locClientSendReq(clientHandle,
                            QMI_LOC_REG_EVENTS_REQ_V02,
                            reqUnion);

  if(eLOC_CLIENT_SUCCESS != status )
  {
    LOC_LOGe("locClientSendReq status: %s", loc_get_v02_client_status_name(status) );
    return false;
  }

  return true;
}

/**  validateRequest
  @brief validates the input request
  @param [in] reqId       request ID
  @param [in] reqPayload  Union of pointers to message payload
  @param [out] ppOutData  Pointer to void *data if successful
  @param [out] pOutLen    Pointer to length of data if succesful.
  @return false on failure, true on Success
*/

bool validateRequest(
  uint32_t                    reqId,
  const locClientReqUnionType reqPayload,
  void                        **ppOutData,
  uint32_t                    *pOutLen )

{
  bool noPayloadFlag = false;

  switch(reqId)
  {
    case QMI_LOC_INFORM_CLIENT_REVISION_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInformClientRevisionReqMsgT_v02);
      break;
    }

    case QMI_LOC_REG_EVENTS_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocRegEventsReqMsgT_v02);
       break;
    }

    case QMI_LOC_START_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocStartReqMsgT_v02);
       break;
    }

    case QMI_LOC_STOP_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocStopReqMsgT_v02);
       break;
    }

    case QMI_LOC_NI_USER_RESPONSE_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocNiUserRespReqMsgT_v02);
       break;
    }

    case QMI_LOC_INJECT_PREDICTED_ORBITS_DATA_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectPredictedOrbitsDataReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_UTC_TIME_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectUtcTimeReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_POSITION_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectPositionReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_ENGINE_LOCK_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetEngineLockReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_SBAS_CONFIG_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetSbasConfigReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_LOW_POWER_MODE_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetLowPowerModeReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_SERVER_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetServerReqMsgT_v02);
      break;
    }

    case QMI_LOC_DELETE_ASSIST_DATA_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocDeleteAssistDataReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_APCACHE_DATA_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectApCacheDataReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_APDONOTCACHE_DATA_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectApDoNotCacheDataReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_XTRA_T_SESSION_CONTROL_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetXtraTSessionControlReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_WIFI_POSITION_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectWifiPositionReqMsgT_v02);
      break;
    }

    case QMI_LOC_NOTIFY_WIFI_STATUS_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocNotifyWifiStatusReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_OPERATION_MODE_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetOperationModeReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_SPI_STATUS_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetSpiStatusReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_SENSOR_DATA_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectSensorDataReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_TIME_SYNC_DATA_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectTimeSyncDataReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_CRADLE_MOUNT_CONFIG_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetCradleMountConfigReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_EXTERNAL_POWER_CONFIG_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetExternalPowerConfigReqMsgT_v02);
      break;
    }

    case QMI_LOC_INFORM_LOCATION_SERVER_CONN_STATUS_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInformLocationServerConnStatusReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetProtocolConfigParametersReqMsgT_v02);
      break;
    }

    case QMI_LOC_GET_PROTOCOL_CONFIG_PARAMETERS_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocGetProtocolConfigParametersReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_SENSOR_CONTROL_CONFIG_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetSensorControlConfigReqMsgT_v02);
      break;
    }

    case QMI_LOC_GET_SENSOR_PROPERTIES_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocGetSensorPropertiesReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_SENSOR_PROPERTIES_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetSensorPropertiesReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_SENSOR_PERFORMANCE_CONTROL_CONFIGURATION_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_SUPL_CERTIFICATE_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectSuplCertificateReqMsgT_v02);
      break;
    }
    case QMI_LOC_DELETE_SUPL_CERTIFICATE_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocDeleteSuplCertificateReqMsgT_v02);
      break;
    }
    case QMI_LOC_SET_POSITION_ENGINE_CONFIG_PARAMETERS_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetPositionEngineConfigParametersReqMsgT_v02);
      break;
    }
    case QMI_LOC_GET_POSITION_ENGINE_CONFIG_PARAMETERS_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocGetPositionEngineConfigParametersReqMsgT_v02);
      break;
    }
    case QMI_LOC_ADD_CIRCULAR_GEOFENCE_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocAddCircularGeofenceReqMsgT_v02);
      break;
    }
    case QMI_LOC_DELETE_GEOFENCE_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocDeleteGeofenceReqMsgT_v02);
      break;
    }
    case QMI_LOC_QUERY_GEOFENCE_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocQueryGeofenceReqMsgT_v02);
      break;
    }
    case QMI_LOC_EDIT_GEOFENCE_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocEditGeofenceReqMsgT_v02);
      break;
    }
    case QMI_LOC_GET_BEST_AVAILABLE_POSITION_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocGetBestAvailablePositionReqMsgT_v02);
      break;
    }

    case QMI_LOC_SECURE_GET_AVAILABLE_POSITION_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSecureGetAvailablePositionReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_MOTION_DATA_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectMotionDataReqMsgT_v02);
      break;
    }

    case QMI_LOC_GET_NI_GEOFENCE_ID_LIST_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocGetNiGeofenceIdListReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_GSM_CELL_INFO_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectGSMCellInfoReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_NETWORK_INITIATED_MESSAGE_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectNetworkInitiatedMessageReqMsgT_v02);
      break;
    }

    case QMI_LOC_PEDOMETER_REPORT_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocPedometerReportReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_WCDMA_CELL_INFO_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectWCDMACellInfoReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_TDSCDMA_CELL_INFO_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectTDSCDMACellInfoReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_SUBSCRIBER_ID_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectSubscriberIDReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_WIFI_AP_DATA_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectWifiApDataReqMsgT_v02);
      break;
    }

    case QMI_LOC_GET_BATCH_SIZE_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocGetBatchSizeReqMsgT_v02);
      break;
    }

    case QMI_LOC_START_BATCHING_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocStartBatchingReqMsgT_v02);
      break;
    }

    case QMI_LOC_READ_FROM_BATCH_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocReadFromBatchReqMsgT_v02);
      break;
    }

    case QMI_LOC_STOP_BATCHING_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocStopBatchingReqMsgT_v02);
      break;
    }

    case QMI_LOC_RELEASE_BATCH_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocReleaseBatchReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_XTRA_VERSION_CHECK_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocSetXtraVersionCheckReqMsgT_v02);
        break;
    }

    case QMI_LOC_INJECT_VEHICLE_SENSOR_DATA_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectVehicleSensorDataReqMsgT_v02);
      break;
    }

    case QMI_LOC_NOTIFY_WIFI_ATTACHMENT_STATUS_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocNotifyWifiAttachmentStatusReqMsgT_v02);
      break;
    }

    case QMI_LOC_NOTIFY_WIFI_ENABLED_STATUS_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocNotifyWifiEnabledStatusReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_PREMIUM_SERVICES_CONFIG_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocSetPremiumServicesCfgReqMsgT_v02);
        break;
    }

    case QMI_LOC_GET_AVAILABLE_WWAN_POSITION_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocGetAvailWwanPositionReqMsgT_v02);
        break;
    }

    case QMI_LOC_INJECT_GTP_CLIENT_DOWNLOADED_DATA_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocInjectGtpClientDownloadedDataReqMsgT_v02);
        break;
    }

    case QMI_LOC_GDT_UPLOAD_BEGIN_STATUS_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocGdtUploadBeginStatusReqMsgT_v02);
        break;
    }

    case QMI_LOC_GDT_UPLOAD_END_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocGdtUploadEndReqMsgT_v02);
        break;
    }

    case QMI_LOC_SET_GNSS_CONSTELL_REPORT_CONFIG_V02:
    {
        *pOutLen = sizeof(qmiLocSetGNSSConstRepConfigReqMsgT_v02);
        break;
    }

    case QMI_LOC_START_DBT_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocStartDbtReqMsgT_v02);
        break;
    }

    case QMI_LOC_STOP_DBT_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocStopDbtReqMsgT_v02);
        break;
    }

    case QMI_LOC_INJECT_TIME_ZONE_INFO_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocInjectTimeZoneInfoReqMsgT_v02);
        break;
    }

    case QMI_LOC_QUERY_AON_CONFIG_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocQueryAonConfigReqMsgT_v02);
        break;
    }

    case QMI_LOC_GTP_AP_STATUS_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocGtpApStatusReqMsgT_v02);
        break;
    }

    case QMI_LOC_GDT_DOWNLOAD_BEGIN_STATUS_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocGdtDownloadBeginStatusReqMsgT_v02);
        break;
    }

    case QMI_LOC_GDT_DOWNLOAD_READY_STATUS_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocGdtDownloadReadyStatusReqMsgT_v02);
        break;
    }

    case QMI_LOC_GDT_RECEIVE_DONE_STATUS_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocGdtReceiveDoneStatusReqMsgT_v02);
        break;
    }

    case QMI_LOC_GDT_DOWNLOAD_END_STATUS_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocGdtDownloadEndStatusReqMsgT_v02);
        break;
    }

    case QMI_LOC_GET_SUPPORTED_FEATURE_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocGetSupportedFeatureReqMsgT_v02);
        break;
    }

    case QMI_LOC_DELETE_GNSS_SERVICE_DATA_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocDeleteGNSSServiceDataReqMsgT_v02);
      break;
    }

    // XTRA Client 2.0
    case QMI_LOC_INJECT_XTRA_DATA_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocInjectXtraDataReqMsgT_v02);
        break;
    }

    case QMI_LOC_INJECT_XTRA_PCID_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocInjectXtraPcidReqMsgT_v02);
        break;
    }

    // SRN AP data injection
    case QMI_LOC_INJECT_SRN_AP_DATA_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocInjectSrnApDataReqMsgT_v02);
        break;
    }

    case QMI_LOC_CROWDSOURCE_MANAGER_CONTROL_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocCrowdSourceManagerControlReqMsgT_v02);
        break;
    }

    case QMI_LOC_CROWDSOURCE_MANAGER_READ_DATA_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocCrowdSourceManagerReadDataReqMsgT_v02);
        break;
    }

    // Query Xtra config data
    case QMI_LOC_QUERY_XTRA_INFO_REQ_V02 :
    {
        *pOutLen = sizeof(qmiLocQueryXtraInfoReqMsgT_v02);
        break;
    }

    case QMI_LOC_START_OUTDOOR_TRIP_BATCHING_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocStartOutdoorTripBatchingReqMsgT_v02);
        break;
    }

    case QMI_LOC_GET_FDCL_BS_LIST_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocGetFdclBsListReqMsgT_v02);
        break;
    }

    case QMI_LOC_INJECT_FDCL_DATA_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocInjectFdclDataReqMsgT_v02);
        break;
    }

    case QMI_LOC_SET_BLACKLIST_SV_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocSetBlacklistSvReqMsgT_v02);
        break;
    }

    case QMI_LOC_SET_CONSTELLATION_CONTROL_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocSetConstellationConfigReqMsgT_v02);
        break;
    }

    case QMI_LOC_REGISTER_MASTER_CLIENT_REQ_V02 :
    {
        *pOutLen = sizeof(qmiLocRegisterMasterClientReqMsgT_v02);
        break;
    }

    case QMI_LOC_GET_BS_OBS_DATA_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocGetBsObsDataReqMsgT_v02);
        break;
    }

    case QMI_LOC_SET_CONSTRAINED_TUNC_MODE_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocSetConstrainedTuncModeReqMsgT_v02);
        break;
    }

    case QMI_LOC_ENABLE_POSITION_ASSISTED_CLOCK_EST_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocEnablePositionAssistedClockEstReqMsgT_v02);
        break;
    }

    case QMI_LOC_QUERY_GNSS_ENERGY_CONSUMED_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocQueryGNSSEnergyConsumedReqMsgT_v02);
        break;
    }

    case QMI_LOC_INJECT_PLATFORM_POWER_STATE_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocInjectPlatformPowerStateReqMsgT_v02);
        break;
    }

    case QMI_LOC_SET_ROBUST_LOCATION_CONFIG_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocSetRobustLocationReqMsgT_v02);
        break;
    }

    case QMI_LOC_INJECT_ENV_AIDING_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocEventInjectEnvAidingReqMsgT_v02);
        break;
    }

    case QMI_LOC_SET_MIN_GPS_WEEK_NUMBER_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocSetMinGpsWeekNumberReqMsgT_v02);
        break;
    }

    case QMI_LOC_SET_PARAMETER_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocSetParameterReqMsgT_v02);
        break;
    }

    case QMI_LOC_GET_PARAMETER_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocGetParameterReqMsgT_v02);
        break;
    }

    case QMI_LOC_SET_MULTIBAND_CONFIG_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocSetMultibandConfigReqMsgT_v02);
        break;
    }

    case QMI_LOC_INJECT_LOCATION_CIVIC_ADDRESS_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocInjectLocationCivicAddressReqMsgT_v02);
        break;
    }

    case QMI_LOC_INJECT_RAW_DATA_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocInjectRawDataReqMsgT_v02);
        break;
    }

    case QMI_LOC_SET_TRIBAND_STATE_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocSetTribandStateReqMsgT_v02);
        break;
    }

    case QMI_LOC_SET_SDK_FEATURE_CONFIG_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocSetSdkFeatureConfigReqMsgT_v02);
        break;
    }
    case QMI_LOC_OSNMA_PUBLIC_KEY_MERKLE_TREE_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02);
        break;
    }
    case QMI_LOC_SET_OSNMA_STATE_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocSetOsnmaStateReqMsgT_v02);
        break;
    }
    case QMI_LOC_SET_NTN_STATUS_REQ_V02:
    {
        *pOutLen = sizeof(qmiLocSetNtnStatusReqMsgT_v02);
        break;
    }
    // ALL requests with no payload
    case QMI_LOC_GET_SERVICE_REVISION_REQ_V02:
    case QMI_LOC_GET_FIX_CRITERIA_REQ_V02:
    case QMI_LOC_GET_PREDICTED_ORBITS_DATA_SOURCE_REQ_V02:
    case QMI_LOC_GET_PREDICTED_ORBITS_DATA_VALIDITY_REQ_V02:
    case QMI_LOC_GET_ENGINE_LOCK_REQ_V02:
    case QMI_LOC_GET_SBAS_CONFIG_REQ_V02:
    case QMI_LOC_GET_LOW_POWER_MODE_REQ_V02:
    case QMI_LOC_GET_SERVER_REQ_V02:
    case QMI_LOC_GET_XTRA_T_SESSION_CONTROL_REQ_V02:
    case QMI_LOC_GET_REGISTERED_EVENTS_REQ_V02:
    case QMI_LOC_GET_OPERATION_MODE_REQ_V02:
    case QMI_LOC_GET_CRADLE_MOUNT_CONFIG_REQ_V02:
    case QMI_LOC_GET_EXTERNAL_POWER_CONFIG_REQ_V02:
    case QMI_LOC_GET_SENSOR_CONTROL_CONFIG_REQ_V02:
    case QMI_LOC_GET_SENSOR_PERFORMANCE_CONTROL_CONFIGURATION_REQ_V02:
    case QMI_LOC_WWAN_OUT_OF_SERVICE_NOTIFICATION_REQ_V02:
    case QMI_LOC_GET_SUPPORTED_MSGS_REQ_V02:
    case QMI_LOC_GET_SUPPORTED_FIELDS_REQ_V02:
    case QMI_LOC_QUERY_OTB_ACCUMULATED_DISTANCE_REQ_V02:
    case QMI_LOC_GET_BLACKLIST_SV_REQ_V02:
    case QMI_LOC_GET_CONSTELLATION_CONTROL_REQ_V02:
    case QMI_LOC_GET_ROBUST_LOCATION_CONFIG_REQ_V02:
    case QMI_LOC_GET_MIN_GPS_WEEK_NUMBER_REQ_V02:
    case QMI_LOC_GET_MULTIBAND_CONFIG_REQ_V02:
    case QMI_LOC_GET_NTN_STATUS_REQ_V02:
    {
      noPayloadFlag = true;
      break;
    }

    default:
      LOC_LOGw("Unknown reqId=%d, name=%s", reqId, loc_get_v02_event_name(reqId));
      return false;
  }
  if(true == noPayloadFlag)
  {
    *ppOutData = NULL;
    *pOutLen = 0;
  }
  else
  {
    //set dummy pointer for request union
    *ppOutData = (void*) reqPayload.pInformClientRevisionReq;
  }
  LOC_LOGa("reqId=%d, len = %d", reqId, *pOutLen);
  return true;
}

/** locClientQmiCtrlPointInit
 @brief wait for the service to come up or timeout; when the
        service comes up initialize the control point and set
        internal handle and indication callback.
 @param pQmiClient,
*/

static locClientStatusEnumType locClientQmiCtrlPointInit(
    locClientCallbackDataType *pLocClientCbData,
    int instanceId)
{
  qmi_client_type clnt, notifier = nullptr;
  bool notifierInitFlag = false;
  locClientStatusEnumType status = eLOC_CLIENT_SUCCESS;
  // os_params must stay in the same scope as notifier
  // because when notifier is initialized, the pointer
  // of os_params is retained in QMI framework, and it
  // used when notifier is released.
  qmi_client_os_params os_params;
  // instances of this service
  qmi_service_info serviceInfo = {};

  do
  {
    qmi_client_error_type rc = QMI_NO_ERR;

    // Get the service object for the qmiLoc Service
    qmi_idl_service_object_type locClientServiceObject =
      loc_get_service_object_v02();

    // Verify that qmiLoc_get_service_object did not return NULL
    if (NULL == locClientServiceObject)
    {
       LOC_LOGe("qmiLoc_get_service_object_v02 failed, null locClientServiceObject");
       status = eLOC_CLIENT_FAILURE_INTERNAL;
       break;
    }

    // register for service notification
    rc = qmi_client_notifier_init(locClientServiceObject, &os_params, &notifier);
    notifierInitFlag = (NULL != notifier);

    if (rc != QMI_NO_ERR) {
        LOC_LOGe("qmi_client_notifier_init failed rc = %d", rc);
        status = eLOC_CLIENT_FAILURE_INTERNAL;
        break;
    }

    while (1) {
        QMI_CCI_OS_SIGNAL_CLEAR(&os_params);

        if (instanceId >= 0) {
            // use instance-specific lookup
            rc = qmi_client_get_service_instance(locClientServiceObject, instanceId, &serviceInfo);
        } else {
            // lookup service with any instance id
            rc = qmi_client_get_any_service(locClientServiceObject, &serviceInfo);
        }

        if (rc == QMI_NO_ERR) {
            break;
        } else {
            LOC_LOGe(" qmi_client_get_service() rc: %d ", rc);
        }

        QMI_CCI_OS_SIGNAL_WAIT(&os_params, 0);
    }

    // initialize the client
    //sent the address of the first service found
    // if IPC router is present, this will go to the service instance
    // enumerated over IPC router, else it will go over the next transport where
    // the service was enumerated.
    rc = qmi_client_init(&serviceInfo, locClientServiceObject,
                         locClientIndCb, (void *) pLocClientCbData,
                         NULL, &clnt);

    if(rc != QMI_NO_ERR)
    {
      LOC_LOGe("qmi_client_init error %d", rc);

      status = eLOC_CLIENT_FAILURE_INTERNAL;
      break;
    }

    // register error callback
    rc  = qmi_client_register_error_cb(clnt,
        locClientErrorCb, (void *) pLocClientCbData);

    if( QMI_NO_ERR != rc)
    {
      LOC_LOGe("qmi_client_register_error_cb error:%d", rc);

      status = eLOC_CLIENT_FAILURE_INTERNAL;
      break;
    }

    // copy the clnt handle returned in qmi_client_init
    memcpy(&(pLocClientCbData->userHandle), &clnt, sizeof(qmi_client_type));

    status = eLOC_CLIENT_SUCCESS;

  } while(0);

  /* release the notifier handle */
  if(true == notifierInitFlag)
  {
    qmi_client_release(notifier);
  }

  return status;
}
//----------------------- END INTERNAL FUNCTIONS ----------------------------------------

/** locClientOpenInstance
  @brief Connects a location client to the location engine. If the connection
         is successful, returns a handle that the location client uses for
         future location operations.

  @param [in] eventRegMask     Mask of asynchronous events the client is
                               interested in receiving
  @param [in] instanceId       Value of QMI service instance id to use.
  @param [in] eventIndCb       Function to be invoked to handle an event.
  @param [in] respIndCb        Function to be invoked to handle a response
                               indication.
  @param [out] locClientHandle Handle to be used by the client
                               for any subsequent requests.

  @return
  One of the following error codes:
  - eLOC_CLIENT_SUCCESS  -- If the connection is opened.
  - non-zero error code(see locClientStatusEnumType)--  On failure.
*/
locClientStatusEnumType locClientOpenInstance (
  locClientEventMaskType         eventRegMask,
  int                            instanceId,
  const locClientCallbacksType*  pLocClientCallbacks,
  locClientHandleType*           pLocClientHandle,
  const void*                    pClientCookie)
{
  locClientStatusEnumType status = eLOC_CLIENT_SUCCESS;
  locClientCallbackDataType *pCallbackData = NULL;

  // check input parameters
  if( (NULL == pLocClientCallbacks) || (NULL == pLocClientHandle)
      || (NULL == pLocClientCallbacks->respIndCb) ||
      (pLocClientCallbacks->size != sizeof(locClientCallbacksType)))
  {
    LOC_LOGe("Invalid parameters in locClientOpen");
    return eLOC_CLIENT_FAILURE_INVALID_PARAMETER;
  }

  do
  {
    // Allocate memory for the callback data
    pCallbackData =
        ( locClientCallbackDataType*)calloc(
            1, sizeof(locClientCallbackDataType));

    if(NULL == pCallbackData)
    {
      LOC_LOGe("Could not allocate memory for callback data");
      status = eLOC_CLIENT_FAILURE_INTERNAL;
      break;
    }
    memset(pCallbackData, 0, sizeof(locClientCallbackDataType));

    /* Initialize the QMI control point; this function will block
     * until a service is up or a timeout occurs. If the connection to
     * the service succeeds the callback data will be filled in with
     * a qmi_client value.
     */

    EXIT_LOG_CALLFLOW(%s, "loc client open");
    status = locClientQmiCtrlPointInit(pCallbackData, instanceId);

    if(status != eLOC_CLIENT_SUCCESS)
    {
      free(pCallbackData);
      pCallbackData = NULL;
      LOC_LOGe ("locClientQmiCtrlPointInit failed, qmi status %d", status);
      break;
    }
     // set the self pointer
    pCallbackData->pMe = pCallbackData;
     // set the handle to the callback data
    *pLocClientHandle = (locClientHandleType)pCallbackData;

    if (true != locClientRegisterEventMask(*pLocClientHandle, eventRegMask, false))
    {
      // release the client
      locClientClose(pLocClientHandle);

      status = eLOC_CLIENT_FAILURE_INTERNAL;
      break;
    }

    /* Initialize rest of the client structure now that the connection
     * to the service has been created successfully.
     */

    //fill in the event callback
     pCallbackData->eventCallback = pLocClientCallbacks->eventIndCb;

     //fill in the response callback
     pCallbackData->respCallback = pLocClientCallbacks->respIndCb;

     //fill in the error callback
     pCallbackData->errorCallback = pLocClientCallbacks->errorCb;

     //set the client event registration mask
     pCallbackData->eventRegMask = eventRegMask;

     // set the client cookie
     pCallbackData->pClientCookie = (void *)pClientCookie;

  }while(0);

  if(eLOC_CLIENT_SUCCESS != status)
  {
    *pLocClientHandle = LOC_CLIENT_INVALID_HANDLE_VALUE;
    LOC_LOGe("Error! status = %d", status);
  }
  else
  {
    LOC_LOGd("returning handle = %p, user_handle=%p, status = %d",
             *pLocClientHandle, pCallbackData->userHandle, status);
  }

  return(status);
}

/** locClientOpen
  @brief Connects a location client to the location engine. If the connection
         is successful, returns a handle that the location client uses for
         future location operations.

  @param [in] eventRegMask     Mask of asynchronous events the client is
                               interested in receiving
  @param [in] eventIndCb       Function to be invoked to handle an event.
  @param [in] respIndCb        Function to be invoked to handle a response
                               indication.
  @param [out] locClientHandle Handle to be used by the client
                               for any subsequent requests.

  @return
  One of the following error codes:
  - eLOC_CLIENT_SUCCESS  -- If the connection is opened.
  - non-zero error code(see locClientStatusEnumType)--  On failure.
*/

locClientStatusEnumType locClientOpen (
  locClientEventMaskType         eventRegMask,
  const locClientCallbacksType*  pLocClientCallbacks,
  locClientHandleType*           pLocClientHandle,
  const void*                    pClientCookie)
{
  int instanceId;
  locClientStatusEnumType status;
  int tries = 1;

  if (getEmulatorCfg()) {
      instanceId = eLOC_CLIENT_INSTANCE_ID_MODEM_EMULATOR;
  } else {
    #ifdef _ANDROID_
      switch (getTargetGnssType(loc_get_target()))
      {
      case GNSS_GSS:
        instanceId = eLOC_CLIENT_INSTANCE_ID_GSS;
        break;
      case GNSS_MSM:
        instanceId = eLOC_CLIENT_INSTANCE_ID_MSM;
        break;
      case GNSS_MDM:
        instanceId = eLOC_CLIENT_INSTANCE_ID_MDM;
        break;
      case GNSS_AUTO:
        instanceId = eLOC_CLIENT_INSTANCE_ID_GSS_AUTO;
        break;
      default:
        instanceId = eLOC_CLIENT_INSTANCE_ID_ANY;
        break;
      }
    #else
      instanceId = eLOC_CLIENT_INSTANCE_ID_ANY;
    #endif
  }

  LOC_LOGi("Service instance id is %d", instanceId);

  while ((status = locClientOpenInstance(eventRegMask, instanceId, pLocClientCallbacks,
          pLocClientHandle, pClientCookie)) != eLOC_CLIENT_SUCCESS) {
    if (tries <= LOC_CLIENT_MAX_OPEN_RETRIES) {
      LOC_LOGe("locClientOpenInstance: failed with status=%d on try %d", status, tries);
      tries++;
      sleep(LOC_CLIENT_TIME_BETWEEN_OPEN_RETRIES);
    } else {
      LOC_LOGe("locClientOpenInstance: failed with status=%d Aborting...", status);
      break;
    }
  }

  return status;
}

/** locClientClose
  @brief Disconnects a client from the location engine.
  @param [in] pLocClientHandle  Pointer to the handle returned by the
                                locClientOpen() function.
  @return
  One of the following error codes:
  - 0 (eLOC_CLIENT_SUCCESS) - On success.
  - non-zero error code(see locClientStatusEnumType) - On failure.
*/

locClientStatusEnumType locClientClose(
  locClientHandleType* pLocClientHandle)
{
  // convert handle to callback data
  locClientCallbackDataType *pCallbackData;
  qmi_client_error_type rc = QMI_NO_ERR; //No error

  if(NULL == pLocClientHandle)
  {
    // invalid handle
    LOC_LOGe("null loc client handle pointer");

    return(eLOC_CLIENT_FAILURE_INVALID_PARAMETER);
  }

  pCallbackData = (locClientCallbackDataType *)(*pLocClientHandle);

  // check the input handle for sanity
  if(NULL == pCallbackData ||
     NULL == pCallbackData->userHandle ||
     pCallbackData != pCallbackData->pMe )
  {
    // invalid handle
    LOC_LOGe("invalid callback data");

    return(eLOC_CLIENT_FAILURE_INVALID_HANDLE);
  }

  LOC_LOGV("locClientClose releasing handle %p, user handle %p\n",
      *pLocClientHandle, pCallbackData->userHandle );

  // NEXT call goes out to modem. We log the callflow before it
  // actually happens to ensure the this comes before resp callflow
  // back from the modem, to avoid confusing log order. We trust
  // that the QMI framework is robust.
  EXIT_LOG_CALLFLOW(%s, "loc client close");

  // release the handle
  rc = qmi_client_release(pCallbackData->userHandle);
  if(QMI_NO_ERR != rc )
  {
    LOC_LOGw("qmi_client_release error %d for client %p",
             rc, pCallbackData->userHandle);
    return(eLOC_CLIENT_FAILURE_INTERNAL);
  }

  /* clear the memory allocated to callback data to minimize the chances
   *  of a race condition occurring between close and the indication
   *  callback
   */
  pthread_mutex_lock(&loc_shutdown_mutex);
  memset(pCallbackData, 0, sizeof(*pCallbackData));
  pthread_mutex_unlock(&loc_shutdown_mutex);

  // free the memory assigned in locClientOpen
  free(pCallbackData);
  pCallbackData= NULL;

  // set the handle to invalid value
  *pLocClientHandle = LOC_CLIENT_INVALID_HANDLE_VALUE;
  return eLOC_CLIENT_SUCCESS;
}

/** locClientSendReq
  @brief Sends a message to the location engine. If the locClientSendMsg()
         function is successful, the client should expect an indication
         (except start, stop, event reg and sensor injection messages),
         through the registered callback in the locOpen() function. The
         indication will contain the status of the request and if status is a
         success, indication also contains the payload
         associated with response.
  @param [in] handle Handle returned by the locClientOpen()
              function.
  @param [in] reqId         message ID of the request
  @param [in] reqPayload   Payload of the request, can be NULL
                            if request has no payload

  @return
  One of the following error codes:
  - 0 (eLOC_CLIENT_SUCCESS ) - On success.
  - non-zero error code (see locClientStatusEnumType) - On failure.
*/

locClientStatusEnumType locClientSendReq(
  locClientHandleType      handle,
  uint32_t                 reqId,
  locClientReqUnionType    reqPayload )
{
  locClientStatusEnumType status = eLOC_CLIENT_SUCCESS;
  qmi_client_error_type rc = QMI_NO_ERR; //No error
  qmiLocGenRespMsgT_v02 resp;
  uint32_t reqLen = 0;
  void *pReqData = NULL;
  locClientCallbackDataType *pCallbackData =
        (locClientCallbackDataType *)handle;
  int tries;

  // check the input handle for sanity
   if(NULL == pCallbackData ||
      NULL == pCallbackData->userHandle ||
      pCallbackData != pCallbackData->pMe )
   {
     // did not find the handle in the client List
     LOC_LOGe("invalid callback data");

     return(eLOC_CLIENT_FAILURE_INVALID_HANDLE);
   }

  // validate that the request is correct
  if (validateRequest(reqId, reqPayload, &pReqData, &reqLen) == false)
  {
    return(eLOC_CLIENT_FAILURE_INVALID_PARAMETER);
  }

  LOC_LOGd("sending reqId= %d, len = %d", reqId, reqLen);
  for (tries = 1; tries <= LOC_CLIENT_MAX_SYNC_RETRIES; tries++) {
    // NEXT call goes out to modem. We log the callflow before it
    // actually happens to ensure the this comes before resp callflow
    // back from the modem, to avoid confusing log order. We trust
    // that the QMI framework is robust.
    EXIT_LOG_CALLFLOW(%s, loc_get_v02_event_name(reqId));
    memset(&resp, 0, sizeof(resp));
    rc = qmi_client_send_msg_sync(
           pCallbackData->userHandle,
           reqId,
           pReqData,
           reqLen,
           &resp,
           sizeof(resp),
           LOC_CLIENT_ACK_TIMEOUT);

    if (QMI_SERVICE_ERR == rc)
    {
      LOC_LOGe("send_msg_sync error: QMI_SERVICE_ERR");
      return(eLOC_CLIENT_FAILURE_PHONE_OFFLINE);
    }
    else if (rc != QMI_NO_ERR)
    {
      LOC_LOGe("send_msg_sync error: %d\n", rc);
      return(eLOC_CLIENT_FAILURE_INTERNAL);
    }

    if (QMI_ERR_SESSION_OWNERSHIP_V01 != resp.resp.error) {
      break;
    }
    usleep(LOC_CLIENT_TIME_BETWEEN_SYNC_RETRIES);
  }
  if (tries > 1) {
      LOC_LOGe("failed with QMI_ERR_SESSION_OWNERSHIP_V01 on try %d", tries);
  }

  // map the QCCI response to Loc API v02 status
  status = convertQmiResponseToLocStatus(&resp);

  // if the request is to change registered events, update the
  // loc api copy of that
  if(eLOC_CLIENT_SUCCESS == status &&
      QMI_LOC_REG_EVENTS_REQ_V02 == reqId)
  {
    if(NULL != reqPayload.pRegEventsReq )
    {
      pCallbackData->eventRegMask =
        (locClientEventMaskType)(reqPayload.pRegEventsReq->eventRegMask);
    }
  }

  return(status);
}

/** locClientSupportMsgCheck
  @brief Sends a QMI_LOC_GET_SUPPORTED_MSGS_REQ_V02 message to the
         location engine, and then receives a list of all services supported
         by the engine. This function will check if the input service(s) form
         the client is in the list or not. If the locClientSupportMsgCheck()
         function is successful, the client should expect an result of
         the service is supported or not recorded in supportedMsg.
  @param [in] handle Handle returned by the locClientOpen()
              function.
  @param [in] supportedMsg   an integer used to record which
                             message is supported

  @return
  One of the following error codes:
  - 0 (eLOC_CLIENT_SUCCESS) -- On success.
  - Non-zero error code (see \ref locClientStatusEnumType) -- On failure.
*/

locClientStatusEnumType locClientSupportMsgCheck(
     locClientHandleType      handle,
     const uint32_t*          msgArray,
     uint32_t                 msgArrayLength,
     uint64_t*                supportedMsg)
{

  // set to true if one client has checked the modem capability.
  static bool isCheckedAlready = false;
  /*
  The 1st bit in supportedMsgChecked indicates if
      QMI_LOC_EVENT_GEOFENCE_BATCHED_BREACH_NOTIFICATION_IND_V02
      is supported or not;
  The 2ed bit in supportedMsgChecked indicates if
      QMI_LOC_GET_BATCH_SIZE_REQ_V02
      is supported or not;
  */
  static uint64_t supportedMsgChecked = 0;

  // Validate input arguments
  if(msgArray == NULL || supportedMsg == NULL) {

    LOC_LOGe("Input argument is NULL");
    return eLOC_CLIENT_FAILURE_INVALID_PARAMETER;
  }

  if (isCheckedAlready) {
    // already checked modem
    LOC_LOGv("Already checked. The supportedMsgChecked is %" PRId64 "",
             supportedMsgChecked);
    *supportedMsg = supportedMsgChecked;
    return eLOC_CLIENT_SUCCESS;
  }

  locClientStatusEnumType status = eLOC_CLIENT_SUCCESS;
  qmi_client_error_type rc = QMI_NO_ERR; //No error
  qmiLocGetSupportMsgT_v02 resp;

  uint32_t reqLen = 0;
  void *pReqData = NULL;
  locClientCallbackDataType *pCallbackData =
        (locClientCallbackDataType *)handle;

  // check the input handle for sanity
   if( NULL == pCallbackData ||
       NULL == pCallbackData->userHandle ||
       pCallbackData != pCallbackData->pMe ) {
     // did not find the handle in the client List
     LOC_LOGe("invalid handle");

     return eLOC_CLIENT_FAILURE_GENERAL;
   }

  // NEXT call goes out to modem. We log the callflow before it
  // actually happens to ensure the this comes before resp callflow
  // back from the modem, to avoid confusing log order. We trust
  // that the QMI framework is robust.

  EXIT_LOG_CALLFLOW(%s, loc_get_v02_event_name(QMI_LOC_GET_SUPPORTED_MSGS_REQ_V02));
  rc = qmi_client_send_msg_sync(
      pCallbackData->userHandle,
      QMI_LOC_GET_SUPPORTED_MSGS_REQ_V02,
      pReqData,
      reqLen,
      &resp,
      sizeof(resp),
      LOC_CLIENT_ACK_TIMEOUT);

  if (rc != QMI_NO_ERR)
  {
    LOC_LOGe("send_msg_sync error: %d", rc);
    return eLOC_CLIENT_FAILURE_GENERAL;
  }

  // map the QCCI response to Loc API v02 status
  status = convertQmiResponseToLocStatus((qmiLocGenRespMsgT_v02*)&resp);

  if(eLOC_CLIENT_SUCCESS == status)
  {
    // check every message listed in msgArray supported by modem or not
    checkQmiMsgsSupported(msgArray, msgArrayLength, &resp, &supportedMsgChecked);

    LOC_LOGa("supportedMsgChecked is %" PRId64 "", supportedMsgChecked);
    *supportedMsg = supportedMsgChecked;
    isCheckedAlready = true;
    return status;
  } else {
    LOC_LOGe("convertQmiResponseToLocStatus error: %d", status);
    return eLOC_CLIENT_FAILURE_GENERAL;
  }
}

/** locClientGetSizeByRespIndId
 *  @brief Get the size of the response indication structure,
 *         from a specified id
 *  @param [in]  respIndId
 *  @param [out] pRespIndSize
 *  @return true if resp ID was found; else false
*/

bool locClientGetSizeByRespIndId(uint32_t respIndId, size_t *pRespIndSize)
{
  size_t idx = 0, respIndTableSize = 0;

  // Validate input arguments
  if(pRespIndSize == NULL)
  {
    LOC_LOGe("size argument NULL !");
    return false;
  }

  respIndTableSize = (sizeof(locClientRespIndTable)/sizeof(locClientRespIndTableStructT));
  for(idx=0; idx<respIndTableSize; idx++ )
  {
    if(respIndId == locClientRespIndTable[idx].respIndId)
    {
      // found
      *pRespIndSize = locClientRespIndTable[idx].respIndSize;
      return true;
    }
  }

  LOC_LOGd("resp ind Id %d not found", respIndId);
  //not found
  return false;
}


/** locClientGetSizeByEventIndId
 *  @brief Gets the size of the event indication structure, from
 *         a specified id
 *  @param [in]  eventIndId
 *  @param [out] pEventIndSize
 *  @return true if event ID was found; else false
*/
bool locClientGetSizeByEventIndId(uint32_t eventIndId, size_t *pEventIndSize)
{
  size_t idx = 0, eventIndTableSize = 0;

  // Validate input arguments
  if(pEventIndSize == NULL)
  {
    LOC_LOGe("size argument NULL !");
    return false;
  }

  // look in the event table
  eventIndTableSize =
    (sizeof(locClientEventIndTable)/sizeof(locClientEventIndTableStructT));

  for(idx=0; idx<eventIndTableSize; idx++ )
  {
    if(eventIndId == locClientEventIndTable[idx].eventId)
    {
      // found
      *pEventIndSize = locClientEventIndTable[idx].eventSize;

      LOC_LOGa("event ind Id %d size = %d",
               eventIndId, (uint32_t)*pEventIndSize);
      return true;
    }
  }

  LOC_LOGd("event ind Id %d not found", eventIndId);

  // not found
  return false;
}
