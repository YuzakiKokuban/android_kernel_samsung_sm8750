/* Copyright (c) 2011-2022, The Linux Foundation. All rights reserved.
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
 *     * Neither the name of The Linux Foundatoin, nor the names of its
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

#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_ApiV02"

#include <syslog.h>
#undef LOG_PRI
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <math.h>
#include <dlfcn.h>
#include <algorithm>
#include <cfloat>

#include <LocApiV02.h>
#include <loc_api_v02_log.h>
#include <loc_api_sync_req.h>
#include <loc_api_v02_client.h>
#include <loc_util_log.h>
#include <gps_extended.h>
#include "loc_pla.h"
#include <loc_cfg.h>
#include <LocContext.h>
#include <LocIpc.h> // SEC

#ifdef PTP_SUPPORTED
#include <gptp_helper.h>
#endif

using namespace std;
using namespace loc_core;

/* Doppler Conversion from M/S to NS/S */
#define MPS_TO_NSPS         (1.0/0.299792458)

// < SEC_GPS
#include "qmi_secgps_clnt.h"
#include <cutils/properties.h>
#include <SemNativeCarrierFeature.h>
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "LocSvc_ApiV02"
#define MAX_SUPL_URL_LEN 256

#define NO_SERVICE_THRESHOLD 20
#define VALID_DELAY_CNT_BIT 0x07
#define RF_LOSS   (5)
// SEC_GPS >

/* Default session id ; TBD needs incrementing for each */
#define LOC_API_V02_DEF_SESSION_ID (1)

/* UMTS CP Address key*/
#define LOC_NI_NOTIF_KEY_ADDRESS           "Address"

/* GPS SV Id offset */
#define GPS_SV_ID_OFFSET        (1)

/* GLONASS SV Id offset */
#define GLONASS_SV_ID_OFFSET    (65)

/* SV ID range */
#define SV_ID_RANGE             (32)

#define BDS_SV_ID_OFFSET         (201)

/* BeiDou SV ID RANGE*/
#define BDS_SV_ID_RANGE          QMI_LOC_DELETE_MAX_BDS_SV_INFO_LENGTH_V02

// < SEC_GPS
/* Galileo SV ID OFFSET*/
#define GAL_SV_ID_OFFSET         (301)

/* Galileo SV ID RANGE*/
#define GAL_SV_ID_RANGE          QMI_LOC_DELETE_MAX_GAL_SV_INFO_LENGTH_V02

#define BATTERY_GPS "/sys/class/power_supply/battery/gps"
// SEC_GPS >

/* GPS week unknown*/
#define C_GPS_WEEK_UNKNOWN      (65535)

/* seconds per week*/
#define WEEK_MSECS              (60*60*24*7*1000LL)
#define DAY_MSECS               (60*60*24*1000LL)
#define NSEC_IN_MSEC            (1000000LL)
#define MSEC_IN_ONE_SEC         (1000ULL)

/* Num days elapsed since GLONASS started from GPS start day 1980->1996 */
#define GPS_GLONASS_DAYS_DIFF    5838
/* number of glonass days in a 4-year interval */
#define GLONASS_DAYS_IN_4YEARS   1461
/* glonass and UTC offset: 3 hours */
#define GLONASS_UTC_OFFSET_HOURS 3
/* Num days elapsed since BDS started from GPS start day 1980->2006 */
#define GPS_BDS_DAYS_DIFF           9492 // 1356 weeks
#define GPS_BDS_LEAP_SECONDS_DIFF   14   // 14 leap seconds from 1980 to 2006

/* speed of light */
#define SPEED_OF_LIGHT          299792458.0

#define MAX_SV_CNT_SUPPORTED_IN_ONE_CONSTELLATION 64

/* number of QMI_LOC messages that need to be checked*/
#define NUMBER_OF_MSG_TO_BE_CHECKED        (3)

/* the time, in seconds, to wait for user response for NI  */
#define LOC_NI_NO_RESPONSE_TIME 20

#define LAT_LONG_TO_RADIANS .000005364418
#define GF_RESPONSIVENESS_THRESHOLD_MSEC_HIGH   120000 //2 mins
#define GF_RESPONSIVENESS_THRESHOLD_MSEC_MEDIUM 900000 //15 mins

#define FLP_BATCHING_MINIMUN_INTERVAL           (1000) // in msec
#define FLP_BATCHING_MIN_TRIP_DISTANCE           1 // 1 meter
#define MEAS_STATUS_DONT_USE (0xFFC0000000000000)

#define MAX_REFOUNT_DIFF_FOR_1HZ (1100)
#define MAX_REFOUNT_DIFF_FOR_NHZ (110)


template struct loc_core::LocApiResponseData<LocApiBatchData>;
template struct loc_core::LocApiResponseData<LocApiGeofenceData>;
template struct loc_core::LocApiResponseData<LocGpsLocation>;

// Leap Second Uncertainity value in seconds during leap Second roll-over
#define LOC_LEAP_SEC_UNC_GAURD_VALUE (3)

/* minimum number of measurements with
  mask QMI_LOC_MASK_MEAS_STATUS_GNSS_FRESH_MEAS_VALID */
#define MIN_REFRESH_MEASUREMENTS (3)

// SEC
typedef void  (createOSFramework)();

const double CarrierFrequencies[] = {
    0.0,                                // UNKNOWN
    GPS_L1C_CARRIER_FREQUENCY,          // L1C
    SBAS_L1_CA_CARRIER_FREQUENCY,       // SBAS_L1
    GLONASS_G1_CARRIER_FREQUENCY,       // GLONASS_G1
    QZSS_L1CA_CARRIER_FREQUENCY,        // QZSS_L1CA
    BEIDOU_B1_I_CARRIER_FREQUENCY,      // BEIDOU_B1
    GALILEO_E1_C_CARRIER_FREQUENCY,     // GALILEO_E1
    NAVIC_L5_CARRIER_FREQUENCY };       // NAVIC_L5

/* Gaussian 2D scaling table - scale from x% to 68% confidence */
struct conf_scaler_to_68_pair {
    uint8_t confidence;
    float scaler_to_68;
};
/* length of confScalers array */
#define CONF_SCALER_ARRAY_MAX   (3)
const struct conf_scaler_to_68_pair confScalers[CONF_SCALER_ARRAY_MAX] = {
    {39, 1.517}, // 0 - 39 . Index 0
    {50, 1.287}, // 40 - 50. Index 1
    {63, 1.072}, // 51 - 63. Index 2
};

/*fixed timestamp uncertainty 10 milli second */
static int ap_timestamp_uncertainty = 0;

typedef enum {
    RF_LOSS_GPS_CONF        = 0,
    RF_LOSS_GPS_L5_CONF     = 1,
    RF_LOSS_GLO_LEFT_CONF   = 2,
    RF_LOSS_GLO_CENTER_CONF = 3,
    RF_LOSS_GLO_RIGHT_CONF  = 4,
    RF_LOSS_BDS_CONF        = 5,
    RF_LOSS_BDS_B2A_CONF    = 6,
    RF_LOSS_GAL_CONF        = 7,
    RF_LOSS_GAL_E5_CONF     = 8,
    RF_LOSS_NAVIC_CONF      = 9,
    RF_LOSS_BDS_B2B_CONF    = 10,
    RF_LOSS_MAX_CONF        = 11
} rfLossConf;

static uint32_t rfLossNV[RF_LOSS_MAX_CONF] = { 0 };

static loc_param_s_type gps_conf_param_table[] =
{
    { "AP_TIMESTAMP_UNCERTAINTY",   &ap_timestamp_uncertainty,          NULL, 'n' },
    { "RF_LOSS_GPS",                &rfLossNV[RF_LOSS_GPS_CONF],        NULL, 'n' },
    { "RF_LOSS_GPS_L5",             &rfLossNV[RF_LOSS_GPS_L5_CONF],     NULL, 'n' },
    { "RF_LOSS_GLO_LEFT",           &rfLossNV[RF_LOSS_GLO_LEFT_CONF],   NULL, 'n' },
    { "RF_LOSS_GLO_CENTER",         &rfLossNV[RF_LOSS_GLO_CENTER_CONF], NULL, 'n' },
    { "RF_LOSS_GLO_RIGHT",          &rfLossNV[RF_LOSS_GLO_RIGHT_CONF],  NULL, 'n' },
    { "RF_LOSS_BDS",                &rfLossNV[RF_LOSS_BDS_CONF],        NULL, 'n' },
    { "RF_LOSS_BDS_B2A",            &rfLossNV[RF_LOSS_BDS_B2A_CONF],    NULL, 'n' },
    { "RF_LOSS_GAL",                &rfLossNV[RF_LOSS_GAL_CONF],        NULL, 'n' },
    { "RF_LOSS_GAL_E5",             &rfLossNV[RF_LOSS_GAL_E5_CONF],     NULL, 'n' },
    { "RF_LOSS_NAVIC",              &rfLossNV[RF_LOSS_NAVIC_CONF],      NULL, 'n' },
    { "RF_LOSS_BDS_B2B",            &rfLossNV[RF_LOSS_BDS_B2B_CONF],    NULL, 'n' },
};

// < SEC_GPS
#define MAX_SILENT_RESET_MSG_SIZE    100
#define CP_SILENT_RESET_DONE    0
#define CP_SILENT_RESET    1

/* Fix information mask */
#define MAG_CALIBRATED      (0x8)
#define GYRO_UNCALIBRATED   (0x10)
#define GYRO_CALIBRATED     (0x20)

enum LastGyroCalStatus {
    GYRO_UNKNOWN            = 0,
    GYRO_UNCAL              = 1,
    GYRO_CAL                = 2
};

static int AGNSS_CONFIG_EXCEPTION_GPS_LOCK = 0x0004;
loc_ftm_system_e_type system_type = FTM_NONE_SYSTEM;
timer_t factoryTimerId = 0;
static LocApiBase* sLocApi = NULL;
static int sec_client_status = QMI_NO_ERR;
static int report_sv_cnt;
static int positionfix_delay_config = 0;
static int sgps_position_delay_cnt = 0;
static int agps_position_delay_cnt = 0;
static bool first_fix;
static uint32_t gpsLockState = 0;
static int sim_slotId = 0;

static bool checkSmdFactoryMode();
static loc_ftm_system_e_type checkFactoryMode();
static int performFactoryMode(loc_ftm_action_e_type action, loc_ftm_system_e_type system);
static int createTimerFactoryMode();
static void handlefactoryReadTimer(sigval_t user_data);
static void setAgpsMode(int agps_mode);
static int setXtraEnable(int enable_xtra);
static int setGnssRfConfig(int gnss_cfg);
static int setSpirentType(int spirent_type);
static void setGpsBatteryFlag(int connect);
static int setEngineOnlyMode(uint8_t enabled);
static int setXtraThrottleEnable(bool enabled);
static bool checkPositionFixDelayMode();
static int setAgnssConfigMsg(uint8_t* msg_sentence);
static void setL5EnableConfig(unsigned int ENABLE_L5, bool set_L5_flag, unsigned int ENABLE_L5_TIS, bool set_L5_TIS_flag);
static void refreshSecConfFile (const char* sec_ext_config, int32_t length);
static void deleteSecConfFile();
static void requestSetSecGnssParams();
void handleAgnssConfigIndMsg(uint8_t* ind_msg, uint32_t length);
static void secGnssInit();
static int setGnssConfig(const char* sec_ext_config, int32_t length);
static int injectNetworkInitiatedMessage(char* msg, int32_t msg_len);
static void updateSecgpsCallback(SecGpsCallbacks& callbacks);
static void sendBDQuery();
static void sendLidState();
static void sendUpdateNv();
static void sendEmergencySMS();
static void setPrintNavmsgConfig();
static void sendSilentResetMessage(int value); 
void sendPSMSG();

static SecGpsInterface gSecGnssInterface = {
    secGnssInit,
    setGnssConfig,
    injectNetworkInitiatedMessage,
    updateSecgpsCallback
};
static SecGpsCallbacks secGpsCb = {};

static bool isBlueskyInjected = false;
static LastGyroCalStatus isLastGyroCalDone = GYRO_UNKNOWN;
static uint32_t secFixInfoMask = 0;
// SEC_GPS >

/* static event callbacks that call the LocApiV02 callbacks*/

/* global event callback, call the eventCb function in loc api adapter v02
   instance */
static void globalEventCb(locClientHandleType clientHandle,
                          uint32_t eventId,
                          const locClientEventIndUnionType eventPayload,
                          void*  pClientCookie)
{
    switch (eventId) {
    case QMI_LOC_EVENT_POSITION_REPORT_IND_V02:
    case QMI_LOC_EVENT_UNPROPAGATED_POSITION_REPORT_IND_V02:
    case QMI_LOC_EVENT_GNSS_SV_INFO_IND_V02:
    case QMI_LOC_EVENT_GNSS_MEASUREMENT_REPORT_IND_V02:
    case QMI_LOC_EVENT_SV_POLYNOMIAL_REPORT_IND_V02:
    case QMI_LOC_EVENT_GPS_EPHEMERIS_REPORT_IND_V02:
    case QMI_LOC_EVENT_GLONASS_EPHEMERIS_REPORT_IND_V02:
    case QMI_LOC_EVENT_BDS_EPHEMERIS_REPORT_IND_V02:
    case QMI_LOC_EVENT_GALILEO_EPHEMERIS_REPORT_IND_V02:
    case QMI_LOC_EVENT_QZSS_EPHEMERIS_REPORT_IND_V02:
    case QMI_LOC_LATENCY_INFORMATION_IND_V02:
    case QMI_LOC_ENGINE_DEBUG_DATA_IND_V02:
        MODEM_LOG_CALLFLOW_DEBUG(%s, loc_get_v02_event_name(eventId));
        break;
    default:
        MODEM_LOG_CALLFLOW(%s, loc_get_v02_event_name(eventId));
        break;
    }

    LocApiV02 *locApiV02Instance = (LocApiV02 *)pClientCookie;

    // return if null is passed
    if (NULL == locApiV02Instance)
    {
        LOC_LOGe ("NULL object passed : client = %p, event id = 0x%X",
                  clientHandle, eventId);
        return;
    }
    locApiV02Instance->eventCb(clientHandle, eventId, eventPayload);
}

/* global response callback, it calls the sync request process
   indication function to unblock the request that is waiting on this
   response indication*/
static void globalRespCb(locClientHandleType clientHandle,
                         uint32_t respId,
                         const locClientRespIndUnionType respPayload,
                         uint32_t respPayloadSize,
                         void*  pClientCookie)
{
  MODEM_LOG_CALLFLOW(%s, loc_get_v02_event_name(respId));
  LocApiV02 *locApiV02Instance =
        (LocApiV02 *)pClientCookie;

  if (NULL == locApiV02Instance)
  {
    LOC_LOGe ("NULL object passed : client = %p, resp id = %d",
              clientHandle, respId);
    return;
  }

  switch(respId)
  {
    case QMI_LOC_GET_AVAILABLE_WWAN_POSITION_IND_V02:
      if (respPayload.pGetAvailWwanPositionInd != NULL) {
          locApiV02Instance->handleWwanZppFixIndication(*respPayload.pGetAvailWwanPositionInd);
      }
      break;
    case QMI_LOC_GET_BEST_AVAILABLE_POSITION_IND_V02:
        if (respPayload.pGetBestAvailablePositionInd != NULL) {
            locApiV02Instance->handleZppBestAvailableFixIndication(
                    *respPayload.pGetBestAvailablePositionInd);
        }
        [[fallthrough]];
       // Call loc_sync_process_ind below also
    default:
      // process the sync call
      // use pDeleteAssistDataInd as a dummy pointer
      loc_sync_process_ind(clientHandle, respId,
          (void *)respPayload.pDeleteAssistDataInd, respPayloadSize);
      break;
  }
}

/* global error callback, it will call the handle service down
   function in the loc api adapter instance. */
static void globalErrorCb (locClientHandleType clientHandle,
                           locClientErrorEnumType errorId,
                           void *pClientCookie)
{
  LocApiV02 *locApiV02Instance =
          (LocApiV02 *)pClientCookie;

  LOC_LOGv ("client = %p, error id = %d, client cookie ptr = %p",
            clientHandle, errorId, pClientCookie);
  if (NULL == locApiV02Instance)
  {
    LOC_LOGe ("NULL object passed : client = %p, error id = %d",
               clientHandle, errorId);
    return;
  }
  locApiV02Instance->errorCb(clientHandle, errorId);
}

/* global structure containing the callbacks */
locClientCallbacksType globalCallbacks =
{
    sizeof(locClientCallbacksType),
    globalEventCb,
    globalRespCb,
    globalErrorCb
};

static void getInterSystemTimeBias(const char* interSystem,
                                   Gnss_InterSystemBiasStructType &interSystemBias,
                                   const qmiLocInterSystemBiasStructT_v02* pInterSysBias)
{
    LOC_LOGa("interSystem: %s, Mask:0x%x, TimeBias:%f, TimeBiasUnc:%f",
             interSystem, pInterSysBias->validMask, pInterSysBias->timeBias,
             pInterSysBias->timeBiasUnc);

    interSystemBias.validMask    = pInterSysBias->validMask;
    interSystemBias.timeBias     = pInterSysBias->timeBias;
    interSystemBias.timeBiasUnc  = pInterSysBias->timeBiasUnc;
}

// < SEC GPS
static const char* getSalesCode()
{
    return SemNativeCarrierFeature::getInstance()->getString(sim_slotId, "CarrierFeature_GPS_ConfigAgpsSetting", "", false);
}
// SEC GPS >
/* Constructor for LocApiV02 */
LocApiV02 :: LocApiV02(LOC_API_ADAPTER_EVENT_MASK_T exMask,
                       ContextBase* context):
    LocApiBase(exMask, context),
    clientHandle(LOC_CLIENT_INVALID_HANDLE_VALUE),
    mQmiMask(0), mInSession(false), mPowerMode(GNSS_POWER_MODE_DEFAULT),
    mEngineOn(false), mFirstMeasurementOfSessionReceived(false),
    mMasterRegisterNotSupported(false),
    mCounter(0), mMinInterval(1000),
    mGnssMeasurements(nullptr),
    mBatchSize(0), mDesiredBatchSize(0),
    mTripBatchSize(0), mDesiredTripBatchSize(0),
    mIsFirstFinalFixReported(false),
    mIsFirstStartFixReq(false),
    mHlosQtimer1(0),
    mHlosQtimer2(0),
    mRefFCount(0),
    mMeasElapsedRealTimeCal(600000000),
    mTimeBiases{},
    mPlatformPowerState(eQMI_LOC_POWER_STATE_UNKNOWN_V02),
    mIsFullTracking(true),
    mPreferredSignalType(QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L1CA_V02),
    mQesdkFeatureMask(0),
    mIsGptpInitialized(false),
    mSecDefaultInitDone(false), mIsSsrHappened(false), mPendingSetParam(false), mSentCI(false) // SEC_GPS
{
  // initialize loc_sync_req interface
  loc_sync_req_init();
  m1HzMeasurementsInfo = {};
  mPrev1HzSlipCountMap.clear();
  mPrevNhzSlipCountMap.clear();
  mCurrentCycleSlipCountMap1Hz.clear();
  mCurrentCycleSlipCountMapNHz.clear();

  mReferenceSignalTypeForIsb.svType = GNSS_SV_TYPE_GPS;
  mReferenceSignalTypeForIsb.carrierFrequencyHz = GPS_L1CA_CARRIER_FREQUENCY;
  mReferenceSignalTypeForIsb.codeType = GNSS_MEASUREMENTS_CODE_TYPE_C;

  UTIL_READ_CONF(LOC_PATH_GPS_CONF, gps_conf_param_table);

  // < SEC
  char carrier_code[PROPERTY_VALUE_MAX];
  property_get("ro.carrier", carrier_code, "");
  if (!strcmp(carrier_code, "wifi-only")) {
    mIsWifiOnly = true;
  } else {
    mIsWifiOnly = false;
  }
  // SEC >
}

/* Destructor for LocApiV02 */
LocApiV02 :: ~LocApiV02()
{
    close();
    if (mGnssMeasurements) {
        free(mGnssMeasurements);
        mGnssMeasurements = nullptr;
    }

#ifdef PTP_SUPPORTED
         if (mIsGptpInitialized) {
             mIsGptpInitialized = false;
             gptpDeinit();
         }
#endif

}

LocApiBase* getLocApi(LOC_API_ADAPTER_EVENT_MASK_T exMask,
                      ContextBase* context)
{
    return (LocApiBase*)LocApiV02::createLocApiV02(exMask, context);
}

// < SEC_GPS
#ifndef DEBUG_X86
extern "C" const SecGpsInterface* getSecGnssInterface()
#else
const SecGpsInterface* getSecGnssInterface()
#endif // DEBUG_X86
{
    return &gSecGnssInterface;
}
// SEC_GPS >

LocApiBase* LocApiV02::createLocApiV02(LOC_API_ADAPTER_EVENT_MASK_T exMask,
                                       ContextBase* context)
{
    LOC_LOGd(" Creating new LocApiV02");
    return new LocApiV02(exMask, context);
}

/* Initialize a loc api v02 client AND
   check which loc message are supported by modem */
enum loc_api_adapter_err
LocApiV02 :: open(LOC_API_ADAPTER_EVENT_MASK_T mask)
{
  enum loc_api_adapter_err rtv = LOC_API_ADAPTER_ERR_SUCCESS;
  locClientStatusEnumType status = eLOC_CLIENT_SUCCESS;

  LOC_API_ADAPTER_EVENT_MASK_T newMask = mask & ~mExcludedMask;

  syslog(LOG_INFO, "%p loc_open: pid %d, enter mask 0x%" PRIx64 ", "
         "mExcludedMask: 0x%" PRIx64", current mMask: 0x%" PRIx64 ", "
         "newMask: 0x%" PRIx64 ",  mQmiMask: 0x%" PRIx64"",
         clientHandle, (uint32_t)getpid(), mask, mExcludedMask, mMask, newMask, mQmiMask);
  LOC_LOGi("%p Enter mask 0x%" PRIx64 ", mExcludedMask: 0x%" PRIx64","
           "mMask: 0x%" PRIx64 ", newMask: 0x%" PRIx64 ",  mQmiMask: 0x%" PRIx64"",
           clientHandle, mask, mExcludedMask, mMask, newMask, mQmiMask);

  /* If the client is not already open, create one */
  if(LOC_CLIENT_INVALID_HANDLE_VALUE == clientHandle)
  {
    /* initialize the loc api v02 interface, note that
       the locClientOpen() function will block if the
       service is unavailable for a fixed time out */

    // it is important to cap the mask here, because not all LocApi's
    // can enable the same bits, e.g. foreground and bckground.
    status = locClientOpen(0, &globalCallbacks, &clientHandle, (void *)this);
    // < SEC_GPS
    if (!mSecDefaultInitDone){
      sec_client_status = qmi_secgps_client_init();
    }
    //  SEC_GPS >

    if (eLOC_CLIENT_SUCCESS != status ||
        clientHandle == LOC_CLIENT_INVALID_HANDLE_VALUE )
    {
      mMask = 0;
      mQmiMask = 0;
      LOC_LOGe ("locClientOpen failed, status = %s",
                loc_get_v02_client_status_name(status));
      rtv = LOC_API_ADAPTER_ERR_FAILURE;
    } else {
        uint64_t supportedMsgList = 0;
        bool gnssMeasurementSupported = false;
        const uint32_t msgArray[NUMBER_OF_MSG_TO_BE_CHECKED] =
        {
            // For - LOC_API_ADAPTER_MESSAGE_LOCATION_BATCHING
            QMI_LOC_GET_BATCH_SIZE_REQ_V02,

            // For - LOC_API_ADAPTER_MESSAGE_BATCHED_GENFENCE_BREACH
            QMI_LOC_EVENT_GEOFENCE_BATCHED_BREACH_NOTIFICATION_IND_V02,

            // For - LOC_API_ADAPTER_MESSAGE_DISTANCE_BASE_TRACKING
            QMI_LOC_START_DBT_REQ_V02
        };

        if (isMaster()) {
            registerMasterClient();
            gnssMeasurementSupported = cacheGnssMeasurementSupport();
            LocContext::injectFeatureConfig(mContext);
        }

        // check the modem
        status = locClientSupportMsgCheck(clientHandle,
                                          msgArray,
                                          NUMBER_OF_MSG_TO_BE_CHECKED,
                                          &supportedMsgList);
        if (eLOC_CLIENT_SUCCESS != status) {
            LOC_LOGe("Failed to checking QMI_LOC message supported.");
        }

        /** if batching is supported , check if the adaptive batching or
            distance-based batching is supported. */
        uint32_t messageChecker = 1 << LOC_API_ADAPTER_MESSAGE_LOCATION_BATCHING;
        if ((messageChecker & supportedMsgList) == messageChecker) {
            locClientReqUnionType req_union;
            locClientStatusEnumType status = eLOC_CLIENT_SUCCESS;
            qmiLocQueryAonConfigReqMsgT_v02 queryAonConfigReq;
            qmiLocQueryAonConfigIndMsgT_v02 queryAonConfigInd;

            memset(&queryAonConfigReq, 0, sizeof(queryAonConfigReq));
            memset(&queryAonConfigInd, 0, sizeof(queryAonConfigInd));
            queryAonConfigReq.transactionId = LOC_API_V02_DEF_SESSION_ID;

            req_union.pQueryAonConfigReq = &queryAonConfigReq;
            status = locSyncSendReq(QMI_LOC_QUERY_AON_CONFIG_REQ_V02,
                                    req_union,
                                    LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                                    QMI_LOC_QUERY_AON_CONFIG_IND_V02,
                                    &queryAonConfigInd);

            if (status == eLOC_CLIENT_FAILURE_UNSUPPORTED) {
                LOC_LOGe("Query AON config is not supported.");
            } else {
                if (status != eLOC_CLIENT_SUCCESS ||
                    queryAonConfigInd.status != eQMI_LOC_SUCCESS_V02) {
                    LOC_LOGe("Query AON config failed. status: %s, ind status:%s",
                             loc_get_v02_client_status_name(status),
                             loc_get_v02_qmi_status_name(queryAonConfigInd.status));
                } else {
                    LOC_LOGd("Query AON config succeeded. aonCapability valid %d, cap 0x%x.",
                             queryAonConfigInd.aonCapability_valid,
                             queryAonConfigInd.aonCapability);
                    if (queryAonConfigInd.aonCapability_valid) {
                        if (queryAonConfigInd.aonCapability &
                                QMI_LOC_MASK_AON_TIME_BASED_BATCHING_SUPPORTED_V02) {
                            LOC_LOGd("LB 1.0 is supported.\n");
                        }
                        if (queryAonConfigInd.aonCapability &
                                QMI_LOC_MASK_AON_AUTO_BATCHING_SUPPORTED_V02) {
                            supportedMsgList |=
                                (1 << LOC_API_ADAPTER_MESSAGE_ADAPTIVE_LOCATION_BATCHING);
                        }
                        if (queryAonConfigInd.aonCapability &
                                QMI_LOC_MASK_AON_DISTANCE_BASED_BATCHING_SUPPORTED_V02) {
                            supportedMsgList |=
                                (1 << LOC_API_ADAPTER_MESSAGE_DISTANCE_BASE_LOCATION_BATCHING);
                        }
                        if (queryAonConfigInd.aonCapability &
                            QMI_LOC_MASK_AON_DISTANCE_BASED_TRACKING_SUPPORTED_V02) {
                        }
                        if (queryAonConfigInd.aonCapability &
                                QMI_LOC_MASK_AON_UPDATE_TBF_SUPPORTED_V02) {
                            supportedMsgList |=
                                (1 << LOC_API_ADAPTER_MESSAGE_UPDATE_TBF_ON_THE_FLY);
                        }
                        if (queryAonConfigInd.aonCapability &
                                QMI_LOC_MASK_AON_OUTDOOR_TRIP_BATCHING_SUPPORTED_V02) {
                            supportedMsgList |=
                                (1 << LOC_API_ADAPTER_MESSAGE_OUTDOOR_TRIP_BATCHING);
                        }
                    }
                }
            }
        }
        LOC_LOGi("supportedMsgList is %" PRIu64 ".", supportedMsgList);

        // Query for supported feature list
        locClientReqUnionType req_union;
        locClientStatusEnumType status = eLOC_CLIENT_SUCCESS;
        qmiLocGetSupportedFeatureReqMsgT_v02 getSupportedFeatureList_req;
        qmiLocGetSupportedFeatureIndMsgT_v02 getSupportedFeatureList_ind;

        memset(&getSupportedFeatureList_req, 0, sizeof(getSupportedFeatureList_req));
        memset(&getSupportedFeatureList_ind, 0, sizeof(getSupportedFeatureList_ind));

        req_union.pGetSupportedFeatureReq = &getSupportedFeatureList_req;
        status = locSyncSendReq(QMI_LOC_GET_SUPPORTED_FEATURE_REQ_V02,
                                req_union,
                                LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                                QMI_LOC_GET_SUPPORTED_FEATURE_IND_V02,
                                &getSupportedFeatureList_ind);
        IF_LOC_LOGD {
            if (eLOC_CLIENT_SUCCESS == status) {
                LOC_LOGd("Got list of features supported of length:%d ",
                         getSupportedFeatureList_ind.feature_len);
                for (uint32_t i = 0; i < getSupportedFeatureList_ind.feature_len; i++) {
                    LOC_LOGd("Bit-mask of supported features at index:%d is %d", i,
                             getSupportedFeatureList_ind.feature[i]);
                }
            }
        }

        // cache the mpss engine capabilities
        mContext->setEngineCapabilities(supportedMsgList,
            (getSupportedFeatureList_ind.feature_len != 0 ? getSupportedFeatureList_ind.feature:
            NULL), gnssMeasurementSupported);

        // Parse the QWES features
        LOC_LOGd("featureStatusReport_valid %d",
                 getSupportedFeatureList_ind.featureStatusReport_valid);
        if (getSupportedFeatureList_ind.featureStatusReport_valid) {
           std::unordered_map<LocationQwesFeatureType, bool> featureMap;
           populateFeatureStatusReport(getSupportedFeatureList_ind.featureStatusReport,
                                       featureMap);
           LocApiBase::reportQwesCapabilities(featureMap);
        }
        getEngineLockStateSync();
    }
  }

  if ((eLOC_CLIENT_SUCCESS == status) && (LOC_CLIENT_INVALID_HANDLE_VALUE != clientHandle)) {
    mMask = newMask;
    registerEventMask();
  }
  LOC_LOGd("clientHandle = %p Exit mMask: 0x%" PRIx64 " mQmiMask: 0x%" PRIx64 "",
           clientHandle, mMask, mQmiMask);

  // < SEC_GPS
  if ((sec_client_status == eLOC_CLIENT_SUCCESS) && !mSecDefaultInitDone &&
      (mQmiMask & QMI_LOC_EVENT_MASK_LOCATION_SERVER_CONNECTION_REQ_V02))
  {
    char default_data_prop[PROPERTY_VALUE_MAX];
    property_get("persist.sys.gps.dds.subId", default_data_prop, "");
    if (!strcmp(default_data_prop, "")) {
      LOC_LOGd("sim slot ID property is null");
    } else {
      sim_slotId = atoi(default_data_prop);
      LOC_LOGd("get default data subscription ID from Property");
    }
    if (!mIsSsrHappened) {
        loc_secgps_default_parameters(sim_slotId);
    }
    else {
        mIsSsrHappened = false;
        sendSilentResetMessage(CP_SILENT_RESET_DONE);
    }
    loc_read_sec_gps_conf();
    setSecGnssParams();
    sec_gps_conf.USE_SECGPS_CONF = SEC_CONF_NONE;
    sLocApi = this;
    mSecDefaultInitDone = true;

    //init izatManager
    void* libHandle = nullptr;
    createOSFramework* getter = (createOSFramework*)dlGetSymFromLib(libHandle,
            "liblocationservice_glue.so", "createOSFramework");
    if (getter != nullptr) {
        (*getter)();
    } else {
        LOC_LOGe("dlGetSymFromLib failed for liblocationservice_glue.so");
    }
  }
  // SEC_GPS >

  return rtv;
}

void LocApiV02 :: registerEventMask()
{
  locClientEventMaskType qmiMask = adjustLocClientEventMask(convertLocClientEventMask(mMask));

  if ((qmiMask != mQmiMask) &&
      (locClientRegisterEventMask(clientHandle, qmiMask, isMaster()))) {
    locClientEventMaskType maskDiff = qmiMask ^ mQmiMask;
    if (maskDiff & qmiMask & QMI_LOC_EVENT_MASK_WIFI_REQ_V02) {
      wifiStatusInformSync();
    }

    if (isMaster()) {
      /* Set the SV Measurement Constellation when Measurement Report or Polynomial report is set */
      /* Check if either measurement report or sv polynomial report bit is different in the new
         mask compared to the old mask. If yes then turn that report on or off as requested */
      /* Later change: we need to set the SV Measurement Constellation whenever measurements
         are supported, and that is because other clients (e.g. CHRE need to have measurements
         enabled and those clients cannot set the SV Measurement Constellation since they are
         not master */
      locClientEventMaskType measOrSvPoly = QMI_LOC_EVENT_MASK_GNSS_SV_POLYNOMIAL_REPORT_V02;
      LOC_LOGd("clientHandle: %p isMaster: %d measOrSvPoly: 0x%" PRIx64 " maskDiff: 0x%" PRIx64 "",
               clientHandle, isMaster(), measOrSvPoly, maskDiff);
      if (((maskDiff & measOrSvPoly) != 0)) {
        locClientEventMaskType measMask = QMI_LOC_EVENT_MASK_GNSS_MEASUREMENT_REPORT_V02 |
                                          QMI_LOC_EVENT_MASK_GNSS_NHZ_MEASUREMENT_REPORT_V02;
        if (mContext->gnssConstellationConfig()) {
          setSvMeasurementConstellation(qmiMask | measMask);
        } else {
          setSvMeasurementConstellation(qmiMask & ~measMask);
        }
      }
    }
    mQmiMask = qmiMask;
  }

    syslog(LOG_INFO, "registerEventMask:  mMask: 0x%" PRIx64 " "
           "mQmiMask=0x%" PRIx64 " qmiMask=0x%" PRIx64"",
           mMask, mQmiMask, qmiMask);

    LOC_LOGi("mMask: 0x%" PRIx64 " mQmiMask=0x%" PRIx64 " qmiMask=0x%" PRIx64"",
            mMask, mQmiMask, qmiMask);
}

bool LocApiV02::sendRequestForAidingData(locClientEventMaskType qmiMask) {

    qmiLocSetGNSSConstRepConfigReqMsgT_v02 aidingDataReq;
    qmiLocSetGNSSConstRepConfigIndMsgT_v02 aidingDataReqInd;
    locClientStatusEnumType status;
    locClientReqUnionType req_union;

    LOC_LOGd("qmiMask = 0x%" PRIx64 "\n", qmiMask);

    memset(&aidingDataReq, 0, sizeof(aidingDataReq));

    if (qmiMask & QMI_LOC_EVENT_MASK_GNSS_SV_POLYNOMIAL_REPORT_V02) {
        aidingDataReq.reportFullSvPolyDb_valid = true;
        aidingDataReq.reportFullSvPolyDb = true;
    }

    if (qmiMask & QMI_LOC_EVENT_MASK_EPHEMERIS_REPORT_V02) {
        aidingDataReq.reportFullEphemerisDb_valid = true;
        aidingDataReq.reportFullEphemerisDb = true;
    }

    /* Note: Requesting for full KlobucharIonoModel report is based on
       QMI_LOC_EVENT_MASK_GNSS_EVENT_REPORT_V02 bit as there is no separate QMI subscription bit for
       iono model.*/
    if (qmiMask & QMI_LOC_EVENT_MASK_GNSS_EVENT_REPORT_V02) {
        aidingDataReq.reportFullIonoDb_valid = true;
        aidingDataReq.reportFullIonoDb = true;
    }

    req_union.pSetGNSSConstRepConfigReq = &aidingDataReq;
    memset(&aidingDataReqInd, 0, sizeof(aidingDataReqInd));

    status = locSyncSendReq(QMI_LOC_SET_GNSS_CONSTELL_REPORT_CONFIG_V02,
        req_union,
        LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
        QMI_LOC_SET_GNSS_CONSTELL_REPORT_CONFIG_IND_V02,
        &aidingDataReqInd);

    return true;

}

locClientEventMaskType LocApiV02 :: adjustLocClientEventMask(locClientEventMaskType qmiMask)
{
    locClientEventMaskType oldQmiMask = qmiMask;

    if (!mInSession) {
        locClientEventMaskType clearMask = QMI_LOC_EVENT_MASK_POSITION_REPORT_V02 |
                                           QMI_LOC_EVENT_MASK_UNPROPAGATED_POSITION_REPORT_V02 |
                                           QMI_LOC_EVENT_MASK_GNSS_SV_INFO_V02 |
                                           QMI_LOC_EVENT_MASK_ENGINE_STATE_V02 |
                                           QMI_LOC_EVENT_MASK_GNSS_MEASUREMENT_REPORT_V02 |
                                           QMI_LOC_EVENT_MASK_GNSS_NHZ_MEASUREMENT_REPORT_V02 |
                                           QMI_LOC_EVENT_MASK_GNSS_SV_POLYNOMIAL_REPORT_V02 |
                                           QMI_LOC_EVENT_MASK_EPHEMERIS_REPORT_V02 |
#ifdef __ANDROID__
                                           QMI_LOC_EVENT_MASK_NEXT_LS_INFO_REPORT_V02 |
#endif
                                           QMI_LOC_EVENT_MASK_LATENCY_INFORMATION_REPORT_V02 |
                                           QMI_LOC_EVENT_MASK_ENGINE_DEBUG_DATA_REPORT_V02 |
                                           QMI_LOC_EVENT_MASK_NEXT_LS_INFO_REPORT_V02; //SEC
        // clear GNSS_EVENT_REPORT mask because QMI_LOC_EVENT_MASK_FEATURE_STATUS_V02 is set
        // when LOC_SUPPORTED_FEATURE_DYNAMIC_FEATURE_STATUS is supported
        if (ContextBase::isFeatureSupported(LOC_SUPPORTED_FEATURE_DYNAMIC_FEATURE_STATUS)) {
            clearMask |= QMI_LOC_EVENT_MASK_GNSS_EVENT_REPORT_V02;
        }
        qmiMask = qmiMask & ~clearMask;
    }

#ifdef __ANDROID__
    if (mInSession || mEngineOn) {
        // if device is in session, always register for engine state
        // so that we can support full tracking mode in concurrent
        // measurement and position session
        qmiMask |= QMI_LOC_EVENT_MASK_ENGINE_STATE_V02;
    }
#endif

    // SEC: manage NMEA listener.
    if (!sec_gps_conf.NMEA_ALLOWED) {
        locClientEventMaskType clearMask = QMI_LOC_EVENT_MASK_NMEA_V02;
        qmiMask = qmiMask & ~clearMask;
    }

    //Enable feature status report for master client
    if (isMaster()) {
        qmiMask |= QMI_LOC_EVENT_MASK_FEATURE_STATUS_V02;
    }
    // By default, every loc api client will need to registers for power state event
    qmiMask |= QMI_LOC_EVENT_MASK_PLATFORM_POWER_STATE_CHANGED_V02;

    if ((eQMI_LOC_POWER_STATE_SUSPENDED_V02 == mPlatformPowerState) ||
            (eQMI_LOC_POWER_STATE_SHUTDOWN_V02 == mPlatformPowerState) ||
                (eQMI_LOC_POWER_STATE_DEEP_SLEEP_ENTRY_V02 == mPlatformPowerState)) {
        // device in suspended/shutdown state, clear the engine state and leap second info mask
        // to avoid wake up
        qmiMask &= ~(QMI_LOC_EVENT_MASK_ENGINE_STATE_V02 |
                QMI_LOC_EVENT_MASK_NEXT_LS_INFO_REPORT_V02);
        syslog(LOG_INFO, "adjustLocClientEventMask, oldQmiMask=0x%" PRIx64 " "
               "qmiMask=0x%" PRIx64 " mInSession: %d, power state %d, retry queue empty %d",
               oldQmiMask, qmiMask, mInSession, mPlatformPowerState, mResenders.empty());
    } else if (mResenders.empty() == false) {
        qmiMask |= QMI_LOC_EVENT_MASK_ENGINE_STATE_V02;
    }

    LOC_LOGi("oldQmiMask=0x%" PRIx64 " qmiMask=0x%" PRIx64 " mInSession: %d, "
             "power state %d, retry queue empty %d, mEngineOn: %d, qmi mask has engine state %d",
             oldQmiMask, qmiMask, mInSession, mPlatformPowerState, mResenders.empty(), mEngineOn,
             (qmiMask & QMI_LOC_EVENT_MASK_ENGINE_STATE_V02) != 0);

    return qmiMask;
}

enum loc_api_adapter_err LocApiV02 :: close()
{
  enum loc_api_adapter_err rtv =
      // success if either client is already invalid, or
      // we successfully close the handle
      (LOC_CLIENT_INVALID_HANDLE_VALUE == clientHandle ||
       eLOC_CLIENT_SUCCESS == locClientClose(&clientHandle)) ?
      LOC_API_ADAPTER_ERR_SUCCESS : LOC_API_ADAPTER_ERR_FAILURE;

  mMask = 0;
  mQmiMask = 0;
  mInSession = false;
  clientHandle = LOC_CLIENT_INVALID_HANDLE_VALUE;

  // < SEC_GPS
  mSecDefaultInitDone = false;
  qmi_secgps_client_deinit();
  // SEC_GPS >

  return rtv;
}

/* inject time into the position engine */
void LocApiV02 ::
    setTime(LocGpsUtcTime time, int64_t timeReference, int uncertainty)
{
  sendMsg(new LocApiMsg([this, time, timeReference, uncertainty] () {

  locClientReqUnionType req_union;
  locClientStatusEnumType status;
  qmiLocInjectUtcTimeReqMsgT_v02  inject_time_msg;
  qmiLocInjectUtcTimeIndMsgT_v02 inject_time_ind;

  memset(&inject_time_msg, 0, sizeof(inject_time_msg));

  inject_time_ind.status = eQMI_LOC_GENERAL_FAILURE_V02;

  inject_time_msg.timeUtc = time;

  inject_time_msg.timeUtc += (int64_t)(elapsedRealtime() - timeReference);

  inject_time_msg.timeUnc = uncertainty;

  req_union.pInjectUtcTimeReq = &inject_time_msg;

  status = locSyncSendReq(QMI_LOC_INJECT_UTC_TIME_REQ_V02,
                          req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                          QMI_LOC_INJECT_UTC_TIME_IND_V02,
                          &inject_time_ind);
  }));
}

/* inject position into the position engine */
void LocApiV02 ::
    injectPosition(double latitude, double longitude, float accuracy, bool onDemandCpi)
{
    Location location = {};

    location.flags |= LOCATION_HAS_LAT_LONG_BIT;
    location.latitude = latitude;
    location.longitude = longitude;

    location.flags |= LOCATION_HAS_ACCURACY_BIT;
    location.accuracy = accuracy;

    struct timespec time_info_current = {};
    if(clock_gettime(CLOCK_REALTIME,&time_info_current) == 0) //success
    {
        location.timestamp = (time_info_current.tv_sec)*1e3 +
            (time_info_current.tv_nsec)/1e6;
    }

    //Use this bit to indicate the injected position source is NLP
    location.techMask |= LOCATION_TECHNOLOGY_WIFI_BIT;
    injectPosition(location, onDemandCpi);
}

void LocApiV02::injectPosition(const Location& location, bool onDemandCpi)
{
    sendMsg(new LocApiMsg([this, location, onDemandCpi] () {

    qmiLocInjectPositionReqMsgT_v02 injectPositionReq;
    memset(&injectPositionReq, 0, sizeof(injectPositionReq));

    if (location.timestamp > 0) {
        injectPositionReq.timestampUtc_valid = 1;
        injectPositionReq.timestampUtc = location.timestamp;
    }

    if (LOCATION_HAS_LAT_LONG_BIT & location.flags) {
        injectPositionReq.latitude_valid = 1;
        injectPositionReq.longitude_valid = 1;
        injectPositionReq.latitude = location.latitude;
        injectPositionReq.longitude = location.longitude;
    }

    if (LOCATION_HAS_ACCURACY_BIT & location.flags) {
        injectPositionReq.horUncCircular_valid = 1;
        injectPositionReq.horUncCircular = location.accuracy;
        injectPositionReq.horConfidence_valid = 1;
        injectPositionReq.horConfidence = 68;
        injectPositionReq.rawHorUncCircular_valid = 1;
        injectPositionReq.rawHorUncCircular = location.accuracy;
        injectPositionReq.rawHorConfidence_valid = 1;
        injectPositionReq.rawHorConfidence = 68;

        // We don't wish to advertise accuracy better than 1000 meters to Modem
        if (injectPositionReq.horUncCircular < 1000) {
            injectPositionReq.horUncCircular = 1000;
        }
    }

    // Altitude is vaild only if Vert Unc is vaild and visa-versa
    if ((LOCATION_HAS_ALTITUDE_BIT & location.flags) &&
            (LOCATION_HAS_VERTICAL_ACCURACY_BIT & location.flags)) {
        injectPositionReq.altitudeWrtEllipsoid_valid = 1;
        injectPositionReq.altitudeWrtEllipsoid = location.altitude;
        injectPositionReq.altSourceInfo_valid = 1;
        injectPositionReq.altSourceInfo.source = eQMI_LOC_ALT_SRC_OTHER_V02;
        injectPositionReq.altSourceInfo.linkage = eQMI_LOC_ALT_SRC_LINKAGE_FULLY_INTERDEPENDENT_V02;
        injectPositionReq.vertUnc_valid = 1;
        injectPositionReq.vertUnc = location.verticalAccuracy;
        injectPositionReq.vertConfidence_valid = 1;
        injectPositionReq.vertConfidence = 68;
    }

    if (LOCATION_HAS_TIME_UNC_BIT & location.flags) {
        injectPositionReq.timeUnc_valid = 1;
        injectPositionReq.timeUnc = location.timeUncMs;
    }

    injectPositionReq.positionSrc_valid = 1;
    if (ContextBase::isFeatureSupported(LOC_SUPPORTED_FEATURE_QMI_FLP_NLP_SOURCE)) {
        if (LOCATION_TECHNOLOGY_HYBRID_ALE_BIT & location.techMask) {
            injectPositionReq.positionSrc = eQMI_LOC_POSITION_SRC_FLP_ALE_V02;
        } else if (LOCATION_TECHNOLOGY_HYBRID_BIT & location.techMask) {
            injectPositionReq.positionSrc = eQMI_LOC_POSITION_SRC_FLP_V02;
        } else if (LOCATION_TECHNOLOGY_WIFI_BIT & location.techMask) {
            injectPositionReq.positionSrc = eQMI_LOC_POSITION_SRC_NLP_V02;
        } else {
            injectPositionReq.positionSrc = eQMI_LOC_POSITION_SRC_OTHER_V02;
        }
    } else {
        if (LOCATION_TECHNOLOGY_WIFI_BIT & location.techMask) {
            injectPositionReq.positionSrc = eQMI_LOC_POSITION_SRC_WIFI_V02;
        } else {
            injectPositionReq.positionSrc = eQMI_LOC_POSITION_SRC_OTHER_V02;
        }
    }

    if (onDemandCpi) {
        injectPositionReq.onDemandCpi_valid = 1;
        injectPositionReq.onDemandCpi = 1;
    }

    LOC_LOGv("Lat=%lf, Lon=%lf, Acc=%.2lf rawAcc=%.2lf horConfidence=%d"
             "rawHorConfidence=%d onDemandCpi=%d, positionSrc: %d, location.techMask: %u",
             injectPositionReq.latitude, injectPositionReq.longitude,
             injectPositionReq.horUncCircular, injectPositionReq.rawHorUncCircular,
             injectPositionReq.horConfidence, injectPositionReq.rawHorConfidence,
             injectPositionReq.onDemandCpi, injectPositionReq.positionSrc, location.techMask);

    LOC_SEND_SYNC_REQ(InjectPosition, INJECT_POSITION, injectPositionReq);

    }));
}

void LocApiV02::injectPosition(const GnssLocationInfoNotification &locationInfo, bool onDemandCpi)
{
    sendMsg(new LocApiMsg([this, locationInfo, onDemandCpi] () {

    qmiLocInjectPositionReqMsgT_v02 injectPositionReq;
    memset(&injectPositionReq, 0, sizeof(injectPositionReq));
    const Location &location = locationInfo.location;

    if (location.timestamp > 0) {
        injectPositionReq.timestampUtc_valid = 1;
        injectPositionReq.timestampUtc = location.timestamp;
    }

    // time uncertainty
    if (LOCATION_HAS_TIME_UNC_BIT & location.flags) {
        injectPositionReq.timeUnc_valid = 1;
        injectPositionReq.timeUnc = location.timeUncMs;
    }

    if (LOCATION_HAS_LAT_LONG_BIT & location.flags) {
        injectPositionReq.latitude_valid = 1;
        injectPositionReq.longitude_valid = 1;
        injectPositionReq.latitude = location.latitude;
        injectPositionReq.longitude = location.longitude;
    }

    if (LOCATION_HAS_ACCURACY_BIT & location.flags) {
        injectPositionReq.horUncCircular_valid = 1;
        injectPositionReq.horUncCircular = location.accuracy;
        injectPositionReq.horConfidence_valid = 1;
        injectPositionReq.horConfidence = 68;
        injectPositionReq.rawHorUncCircular_valid = 1;
        injectPositionReq.rawHorUncCircular = location.accuracy;
        injectPositionReq.rawHorConfidence_valid = 1;
        injectPositionReq.rawHorConfidence = 68;

        // We don't wish to advertise accuracy better than 1000 meters to Modem
        if (injectPositionReq.horUncCircular < 1000) {
            injectPositionReq.horUncCircular = 1000;
        }
    }

    if (LOCATION_HAS_ALTITUDE_BIT & location.flags) {
        injectPositionReq.altitudeWrtEllipsoid_valid = 1;
        injectPositionReq.altitudeWrtEllipsoid = location.altitude;
    }

    if (LOCATION_HAS_VERTICAL_ACCURACY_BIT & location.flags) {
        injectPositionReq.vertUnc_valid = 1;
        injectPositionReq.vertUnc = location.verticalAccuracy;
        injectPositionReq.vertConfidence_valid = 1;
        injectPositionReq.vertConfidence = 68;
    }

    // GPS time
    if (locationInfo.gnssSystemTime.gnssSystemTimeSrc == GNSS_LOC_SV_SYSTEM_GPS) {
        injectPositionReq.gpsTime_valid = 1;
        injectPositionReq.gpsTime.gpsWeek =
                locationInfo.gnssSystemTime.u.gpsSystemTime.systemWeek;
        injectPositionReq.gpsTime.gpsTimeOfWeekMs =
                locationInfo.gnssSystemTime.u.gpsSystemTime.systemMsec;
    } else if (locationInfo.gnssSystemTime.gnssSystemTimeSrc == GNSS_LOC_SV_SYSTEM_GALILEO) {
        injectPositionReq.gpsTime_valid = 1;
        injectPositionReq.gpsTime.gpsWeek =
                locationInfo.gnssSystemTime.u.galSystemTime.systemWeek;
        injectPositionReq.gpsTime.gpsTimeOfWeekMs =
                locationInfo.gnssSystemTime.u.gpsSystemTime.systemMsec;
    } else if (locationInfo.gnssSystemTime.gnssSystemTimeSrc == GNSS_LOC_SV_SYSTEM_BDS) {
        injectPositionReq.gpsTime_valid = 1;
        injectPositionReq.gpsTime.gpsWeek =
                locationInfo.gnssSystemTime.u.bdsSystemTime.systemWeek;
        injectPositionReq.gpsTime.gpsTimeOfWeekMs =
                locationInfo.gnssSystemTime.u.bdsSystemTime.systemMsec;
    } else if (locationInfo.gnssSystemTime.gnssSystemTimeSrc == GNSS_LOC_SV_SYSTEM_QZSS) {
        injectPositionReq.gpsTime_valid = 1;
        injectPositionReq.gpsTime.gpsWeek =
                locationInfo.gnssSystemTime.u.qzssSystemTime.systemWeek;
        injectPositionReq.gpsTime.gpsTimeOfWeekMs =
                locationInfo.gnssSystemTime.u.qzssSystemTime.systemMsec;
     } else if (locationInfo.gnssSystemTime.gnssSystemTimeSrc == GNSS_LOC_SV_SYSTEM_GLONASS) {
         if (LDT_GNSS_LOCATION_INFO_LEAP_SECONDS_BIT & locationInfo.flags) {
             const GnssGloTimeStructType &gloSystemTime =
                     locationInfo.gnssSystemTime.u.gloSystemTime;
             unsigned long long msecTotal =
                     gloSystemTime.gloFourYear * GLONASS_DAYS_IN_4YEARS * DAY_MSECS +
                     gloSystemTime.gloDays * DAY_MSECS +
                     gloSystemTime.gloMsec;
             // compensate the glonnass msec with difference between GPS time
             msecTotal += (GPS_GLONASS_DAYS_DIFF * DAY_MSECS -
                           GLONASS_UTC_OFFSET_HOURS * 3600 * 1000 +
                           locationInfo.leapSeconds * 1000);
             injectPositionReq.gpsTime_valid = 1;
             injectPositionReq.gpsTime.gpsWeek = msecTotal / WEEK_MSECS;
             injectPositionReq.gpsTime.gpsTimeOfWeekMs = msecTotal % WEEK_MSECS;
         }
    } else if (locationInfo.gnssSystemTime.gnssSystemTimeSrc == GNSS_LOC_SV_SYSTEM_NAVIC) {
        injectPositionReq.gpsTime_valid = 1;
        injectPositionReq.gpsTime.gpsWeek =
                locationInfo.gnssSystemTime.u.navicSystemTime.systemWeek;
        injectPositionReq.gpsTime.gpsTimeOfWeekMs =
                locationInfo.gnssSystemTime.u.navicSystemTime.systemMsec;
    }

    // velocity enu
    if ((LDT_GNSS_LOCATION_INFO_EAST_VEL_BIT & locationInfo.flags) &&
        (LDT_GNSS_LOCATION_INFO_NORTH_VEL_BIT & locationInfo.flags) &&
        (LDT_GNSS_LOCATION_INFO_UP_VEL_BIT & locationInfo.flags)) {
        injectPositionReq.velEnu_valid = 1;
        injectPositionReq.velEnu[0] = locationInfo.eastVelocity;
        injectPositionReq.velEnu[1] = locationInfo.northVelocity;
        injectPositionReq.velEnu[2] = locationInfo.upVelocity;
    }

    // velocity uncertainty enu
    if ((LDT_GNSS_LOCATION_INFO_EAST_VEL_UNC_BIT & locationInfo.flags) &&
        (LDT_GNSS_LOCATION_INFO_NORTH_VEL_UNC_BIT & locationInfo.flags) &&
        (LDT_GNSS_LOCATION_INFO_UP_VEL_UNC_BIT & locationInfo.flags)) {
        injectPositionReq.velUncEnu_valid = 1;
        injectPositionReq.velUncEnu[0] = locationInfo.eastVelocityStdDeviation;
        injectPositionReq.velUncEnu[1] = locationInfo.northVelocityStdDeviation;
        injectPositionReq.velUncEnu[2]  = locationInfo.upVelocityStdDeviation;
    }

    // number of SV used info, this replaces expandedGnssSvUsedList as the payload
    // for expandedGnssSvUsedList is too large
    if (LDT_GNSS_LOCATION_INFO_NUM_SV_USED_IN_POSITION_BIT & locationInfo.flags) {
        injectPositionReq.numSvInFix = locationInfo.numSvUsedInPosition;
        injectPositionReq.numSvInFix_valid = 1;
    }

    // mark fix as from DRE for modem to distinguish from others
    if (LOCATION_TECHNOLOGY_SENSORS_BIT & locationInfo.location.techMask) {
        injectPositionReq.positionSrc_valid = 1;
        injectPositionReq.positionSrc = eQMI_LOC_POSITION_SRC_DRE_V02;
    }

    if (onDemandCpi) {
        injectPositionReq.onDemandCpi_valid = 1;
        injectPositionReq.onDemandCpi = 1;
    }

    LOC_LOGv("Lat=%lf, Lon=%lf, Acc=%.2lf rawAcc=%.2lf horConfidence=%d"
             "rawHorConfidence=%d onDemandCpi=%d, position src=%d, numSVs=%d",
             injectPositionReq.latitude, injectPositionReq.longitude,
             injectPositionReq.horUncCircular, injectPositionReq.rawHorUncCircular,
             injectPositionReq.horConfidence, injectPositionReq.rawHorConfidence,
             injectPositionReq.onDemandCpi, injectPositionReq.positionSrc,
             injectPositionReq.numSvInFix);

    LOC_SEND_SYNC_REQ(InjectPosition, INJECT_POSITION, injectPositionReq);

    }));
}

void LocApiV02::injectPositionAndCivicAddress(const Location& location,
          const GnssCivicAddress& addr) {
    sendMsg(new LocApiMsg([this, location, addr] () {
    locClientStatusEnumType status = eLOC_CLIENT_FAILURE_GENERAL;
    locClientReqUnionType req_union = {};

    qmiLocInjectLocationCivicAddressReqMsgT_v02 injectPosAndAddrReq;
    qmiLocGenReqStatusIndMsgT_v02 genReqStatusIndMsg;
    memset(&injectPosAndAddrReq, 0, sizeof(injectPosAndAddrReq));
    memset(&genReqStatusIndMsg, 0, sizeof(genReqStatusIndMsg));

    if (addr.hasLatitude) {
        injectPosAndAddrReq.latitude_valid = 1;
        injectPosAndAddrReq.latitude = addr.latitude;
    }

    if (addr.hasLongitude) {
        injectPosAndAddrReq.longitude_valid = 1;
        injectPosAndAddrReq.longitude = addr.longitude;
    }

    int len;
    len = addr.countryCode.length();
    if (len != 0) {
        injectPosAndAddrReq.country_valid = 1;
        strlcpy(injectPosAndAddrReq.country, addr.countryCode.c_str(),
                sizeof(injectPosAndAddrReq.country));
    }

    len = addr.adminArea.length();
    if (len != 0) {
        injectPosAndAddrReq.subdivA1_valid = 1;
        strlcpy(injectPosAndAddrReq.subdivA1, addr.adminArea.c_str(),
                sizeof(injectPosAndAddrReq.subdivA1));
    }

    len = addr.subAdminArea.length();
    if (len != 0) {
        injectPosAndAddrReq.subdivA2_valid = 1;
        strlcpy(injectPosAndAddrReq.subdivA2, addr.subAdminArea.c_str(),
                sizeof(injectPosAndAddrReq.subdivA2));
    }

    len = addr.locality.length();
    if (len != 0) {
        injectPosAndAddrReq.city_valid = 1;
        strlcpy(injectPosAndAddrReq.city, addr.locality.c_str(),
                sizeof(injectPosAndAddrReq.city));
    }

    len = addr.subLocality.length();
    if (len != 0) {
        injectPosAndAddrReq.cityDiv_valid = 1;
        strlcpy(injectPosAndAddrReq.cityDiv, addr.subLocality.c_str(),
                sizeof(injectPosAndAddrReq.cityDiv));
    }

    len = addr.thoroughfare.length();
    if (len != 0) {
        injectPosAndAddrReq.street_valid = 1;
        strlcpy(injectPosAndAddrReq.street, addr.thoroughfare.c_str(),
                sizeof(injectPosAndAddrReq.street));
    }

    len = addr.featureName.length();
    if (len != 0) {
        injectPosAndAddrReq.landmark_valid = 1;
        strlcpy(injectPosAndAddrReq.landmark, addr.featureName.c_str(),
                sizeof(injectPosAndAddrReq.landmark));
    }

    len = addr.postalCode.length();
    if (len != 0) {
        injectPosAndAddrReq.postalCode_valid = 1;
        strlcpy(injectPosAndAddrReq.postalCode, addr.postalCode.c_str(),
                sizeof(injectPosAndAddrReq.postalCode));
    }

    len = addr.premises.length();
    if (len != 0) {
        injectPosAndAddrReq.building_valid = 1;
        strlcpy(injectPosAndAddrReq.building, addr.premises.c_str(),
                sizeof(injectPosAndAddrReq.building));
    }

    len = addr.thoroughfare.length();
    if (len != 0) {
        injectPosAndAddrReq.primaryRoad_valid = 1;
        strlcpy(injectPosAndAddrReq.primaryRoad, addr.thoroughfare.c_str(),
                sizeof(injectPosAndAddrReq.primaryRoad));
    }

    len = addr.subThoroughfare.length();
    if (len != 0) {
        injectPosAndAddrReq.houseNumber_valid = 1;
        strlcpy(injectPosAndAddrReq.houseNumber, addr.subThoroughfare.c_str(),
                sizeof(injectPosAndAddrReq.houseNumber));
    }

    LOC_LOGv("QMI Civic Address: countryCode: %s, SubdivA1: %s,\n"
            "SubdivA2: %s, City: %s, CityDiv: %s\n"
            "Street: %s, landmark: %s, postalCode: %s\n"
            "Building: %s, PrimaryRoad: %s, houseNumber: %s",
            injectPosAndAddrReq.country,
            injectPosAndAddrReq.subdivA1,
            injectPosAndAddrReq.subdivA2,
            injectPosAndAddrReq.city,
            injectPosAndAddrReq.cityDiv,
            injectPosAndAddrReq.street,
            injectPosAndAddrReq.landmark,
            injectPosAndAddrReq.postalCode,
            injectPosAndAddrReq.building,
            injectPosAndAddrReq.primaryRoad,
            injectPosAndAddrReq.houseNumber);

    req_union.pInjectLocationCivicAddressReq = &injectPosAndAddrReq;

    status = loc_sync_send_req(clientHandle,
                               QMI_LOC_INJECT_LOCATION_CIVIC_ADDRESS_REQ_V02,
                               req_union,
                               LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                               QMI_LOC_INJECT_LOCATION_CIVIC_ADDRESS_IND_V02,
                               &genReqStatusIndMsg);

    if (status != eLOC_CLIENT_SUCCESS ||
            genReqStatusIndMsg.status != eQMI_LOC_SUCCESS_V02) {
        LOC_LOGe("Inject Civic address failed. status: %s ind status %s",
                loc_get_v02_client_status_name(status),
                loc_get_v02_qmi_status_name(genReqStatusIndMsg.status));
    }

    }));

}

/* delete assistance date */
void
LocApiV02::deleteAidingData(const GnssAidingData& data, LocApiResponse *adapterResponse)
{
  sendMsg(new LocApiMsg([this, data, adapterResponse] () {

  static bool isNewApiSupported = true;
  locClientReqUnionType req_union;
  locClientStatusEnumType status = eLOC_CLIENT_FAILURE_UNSUPPORTED;
  LocationError err = LOCATION_ERROR_SUCCESS;

  // Use the new API first
  qmiLocDeleteGNSSServiceDataReqMsgT_v02 delete_gnss_req;
  qmiLocDeleteGNSSServiceDataIndMsgT_v02 delete_gnss_resp;

  memset(&delete_gnss_req, 0, sizeof(delete_gnss_req));
  memset(&delete_gnss_resp, 0, sizeof(delete_gnss_resp));

  if (isNewApiSupported) {
      if (data.deleteAll) {
          delete_gnss_req.deleteAllFlag = true;
      } else {
          if (GNSS_AIDING_DATA_SV_EPHEMERIS_BIT & data.sv.svMask) {
              delete_gnss_req.deleteSatelliteData_valid = 1;
              delete_gnss_req.deleteSatelliteData.deleteSatelliteDataMask |=
                  QMI_LOC_DELETE_DATA_MASK_EPHEMERIS_V02;
              // < SEC_GPS : delete XTRA data in the case of deleting Ephemeris.
              delete_gnss_req.deleteSatelliteData.deleteSatelliteDataMask |=
                  QMI_LOC_DELETE_DATA_MASK_ALM_CORR_V02;
              // SEC_GPS >
          }
          if (GNSS_AIDING_DATA_SV_ALMANAC_BIT & data.sv.svMask) {
              delete_gnss_req.deleteSatelliteData_valid = 1;
              delete_gnss_req.deleteSatelliteData.deleteSatelliteDataMask |=
                  QMI_LOC_DELETE_DATA_MASK_ALMANAC_V02;
          }
          if (GNSS_AIDING_DATA_SV_HEALTH_BIT & data.sv.svMask) {
              delete_gnss_req.deleteSatelliteData_valid = 1;
              delete_gnss_req.deleteSatelliteData.deleteSatelliteDataMask |=
                  QMI_LOC_DELETE_DATA_MASK_SVHEALTH_V02;
          }
          if (GNSS_AIDING_DATA_SV_DIRECTION_BIT & data.sv.svMask) {
              delete_gnss_req.deleteSatelliteData_valid = 1;
              delete_gnss_req.deleteSatelliteData.deleteSatelliteDataMask |=
                  QMI_LOC_DELETE_DATA_MASK_SVDIR_V02;
          }
          if (GNSS_AIDING_DATA_SV_STEER_BIT & data.sv.svMask) {
              delete_gnss_req.deleteSatelliteData_valid = 1;
              delete_gnss_req.deleteSatelliteData.deleteSatelliteDataMask |=
                  QMI_LOC_DELETE_DATA_MASK_SVSTEER_V02;
          }
          if (GNSS_AIDING_DATA_SV_ALMANAC_CORR_BIT & data.sv.svMask) {
              delete_gnss_req.deleteSatelliteData_valid = 1;
              delete_gnss_req.deleteSatelliteData.deleteSatelliteDataMask |=
                  QMI_LOC_DELETE_DATA_MASK_ALM_CORR_V02;
          }
          if (GNSS_AIDING_DATA_SV_BLACKLIST_BIT & data.sv.svMask) {
              delete_gnss_req.deleteSatelliteData_valid = 1;
              delete_gnss_req.deleteSatelliteData.deleteSatelliteDataMask |=
                  QMI_LOC_DELETE_DATA_MASK_BLACKLIST_V02;
          }
          if (GNSS_AIDING_DATA_SV_SA_DATA_BIT & data.sv.svMask) {
              delete_gnss_req.deleteSatelliteData_valid = 1;
              delete_gnss_req.deleteSatelliteData.deleteSatelliteDataMask |=
                  QMI_LOC_DELETE_DATA_MASK_SA_DATA_V02;
          }
          if (GNSS_AIDING_DATA_SV_NO_EXIST_BIT & data.sv.svMask) {
              delete_gnss_req.deleteSatelliteData_valid = 1;
              delete_gnss_req.deleteSatelliteData.deleteSatelliteDataMask |=
                  QMI_LOC_DELETE_DATA_MASK_SV_NO_EXIST_V02;
          }
          if (GNSS_AIDING_DATA_SV_IONOSPHERE_BIT & data.sv.svMask) {
              delete_gnss_req.deleteSatelliteData_valid = 1;
              delete_gnss_req.deleteSatelliteData.deleteSatelliteDataMask |=
                  QMI_LOC_DELETE_DATA_MASK_IONO_V02;
          }
          if (GNSS_AIDING_DATA_SV_TIME_BIT & data.sv.svMask) {
              delete_gnss_req.deleteSatelliteData_valid = 1;
              delete_gnss_req.deleteSatelliteData.deleteSatelliteDataMask |=
                  QMI_LOC_DELETE_DATA_MASK_TIME_V02;
          }
          if (GNSS_AIDING_DATA_SV_MB_DATA & data.sv.svMask) {
              delete_gnss_req.deleteSatelliteData_valid = 1;
              delete_gnss_req.deleteSatelliteData.deleteSatelliteDataMask |=
                  QMI_LOC_DELETE_DATA_MASK_MB_DATA_V02;
          }
          if (delete_gnss_req.deleteSatelliteData_valid) {
              if (GNSS_AIDING_DATA_SV_TYPE_GPS_BIT & data.sv.svTypeMask) {
                  delete_gnss_req.deleteSatelliteData.system |= QMI_LOC_SYSTEM_GPS_V02;
              }
              if (GNSS_AIDING_DATA_SV_TYPE_GLONASS_BIT & data.sv.svTypeMask) {
                  delete_gnss_req.deleteSatelliteData.system |= QMI_LOC_SYSTEM_GLO_V02;
              }
              if (GNSS_AIDING_DATA_SV_TYPE_QZSS_BIT & data.sv.svTypeMask) {
                  delete_gnss_req.deleteSatelliteData.system |= QMI_LOC_SYSTEM_BDS_V02;
              }
              if (GNSS_AIDING_DATA_SV_TYPE_BEIDOU_BIT & data.sv.svTypeMask) {
                  delete_gnss_req.deleteSatelliteData.system |= QMI_LOC_SYSTEM_GAL_V02;
              }
              if (GNSS_AIDING_DATA_SV_TYPE_GALILEO_BIT & data.sv.svTypeMask) {
                  delete_gnss_req.deleteSatelliteData.system |= QMI_LOC_SYSTEM_QZSS_V02;
              }
              if (GNSS_AIDING_DATA_SV_TYPE_NAVIC_BIT & data.sv.svTypeMask) {
                  delete_gnss_req.deleteSatelliteData.system |= QMI_LOC_SYSTEM_NAVIC_V02;
              }
          }

          if (GNSS_AIDING_DATA_COMMON_POSITION_BIT & data.common.mask) {
              delete_gnss_req.deleteCommonDataMask_valid = 1;
              delete_gnss_req.deleteCommonDataMask |= QMI_LOC_DELETE_COMMON_MASK_POS_V02;
          }
          if (GNSS_AIDING_DATA_COMMON_TIME_BIT & data.common.mask) {
              delete_gnss_req.deleteCommonDataMask_valid = 1;
              delete_gnss_req.deleteCommonDataMask |= QMI_LOC_DELETE_COMMON_MASK_TIME_V02;
          }
          if (GNSS_AIDING_DATA_COMMON_UTC_BIT & data.common.mask) {
              delete_gnss_req.deleteCommonDataMask_valid = 1;
              delete_gnss_req.deleteCommonDataMask |= QMI_LOC_DELETE_COMMON_MASK_UTC_V02;
          }
          if (GNSS_AIDING_DATA_COMMON_RTI_BIT & data.common.mask) {
              delete_gnss_req.deleteCommonDataMask_valid = 1;
              delete_gnss_req.deleteCommonDataMask |= QMI_LOC_DELETE_COMMON_MASK_RTI_V02;
          }
          if (GNSS_AIDING_DATA_COMMON_FREQ_BIAS_EST_BIT & data.common.mask) {
              delete_gnss_req.deleteCommonDataMask_valid = 1;
              delete_gnss_req.deleteCommonDataMask |= QMI_LOC_DELETE_COMMON_MASK_FREQ_BIAS_EST_V02;
          }
          if (GNSS_AIDING_DATA_COMMON_CELLDB_BIT & data.common.mask) {
              delete_gnss_req.deleteCellDbDataMask_valid = 1;
              delete_gnss_req.deleteCellDbDataMask =
                  (QMI_LOC_MASK_DELETE_CELLDB_POS_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_LATEST_GPS_POS_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_OTA_POS_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_EXT_REF_POS_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_TIMETAG_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_CELLID_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_CACHED_CELLID_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_LAST_SRV_CELL_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_CUR_SRV_CELL_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_NEIGHBOR_INFO_V02);
          }
      }

      req_union.pDeleteGNSSServiceDataReq = &delete_gnss_req;

      status = locSyncSendReq(QMI_LOC_DELETE_GNSS_SERVICE_DATA_REQ_V02,
                              req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                              QMI_LOC_DELETE_GNSS_SERVICE_DATA_IND_V02,
                              &delete_gnss_resp);
  }

  if (eLOC_CLIENT_FAILURE_UNSUPPORTED == status ||
      eLOC_CLIENT_FAILURE_INTERNAL == status || eQMI_LOC_SUCCESS_V02 != delete_gnss_resp.status) {
      // If the new API is not supported we fall back on the old one
      // The error could be eLOC_CLIENT_FAILURE_INTERNAL if
      // QMI_LOC_DELETE_GNSS_SERVICE_DATA_REQ_V02 is not in the .idl file
      LOC_LOGD("deleteAidingData: QMI_LOC_DELETE_GNSS_SERVICE_DATA_REQ_V02 not supported, "
               "use QMI_LOC_DELETE_ASSIST_DATA_REQ_V02");
      isNewApiSupported = false;

      qmiLocDeleteAssistDataReqMsgT_v02 delete_req;
      qmiLocDeleteAssistDataIndMsgT_v02 delete_resp;

      memset(&delete_req, 0, sizeof(delete_req));
      memset(&delete_resp, 0, sizeof(delete_resp));

      if (data.deleteAll) {
          delete_req.deleteAllFlag = true;
      } else {
          /* to keep track of svInfoList for GPS and GLO*/
          uint32_t curr_sv_len = 0;
          uint32_t curr_sv_idx = 0;
          uint32_t sv_id = 0;

          if ((GNSS_AIDING_DATA_SV_EPHEMERIS_BIT & data.sv.svMask ||
              GNSS_AIDING_DATA_SV_ALMANAC_BIT & data.sv.svMask) &&
              GNSS_AIDING_DATA_SV_TYPE_GPS_BIT & data.sv.svTypeMask) {

              /* do delete for all GPS SV's */
              curr_sv_len += SV_ID_RANGE;

              sv_id = GPS_SV_ID_OFFSET;

              delete_req.deleteSvInfoList_valid = 1;

              delete_req.deleteSvInfoList_len = curr_sv_len;

              LOC_LOGD("deleteAidingData: Delete GPS SV info for index %d to %d "
                       "and sv id %d to %d",
                       curr_sv_idx, curr_sv_len - 1,
                       sv_id, sv_id + SV_ID_RANGE - 1);

              for (uint32_t i = curr_sv_idx; i < curr_sv_len; i++, sv_id++) {
                  delete_req.deleteSvInfoList[i].gnssSvId = sv_id;

                  delete_req.deleteSvInfoList[i].system = eQMI_LOC_SV_SYSTEM_GPS_V02;

                  if (GNSS_AIDING_DATA_SV_EPHEMERIS_BIT & data.sv.svMask) {
                      // set ephemeris mask for all GPS SV's
                      delete_req.deleteSvInfoList[i].deleteSvInfoMask |=
                          QMI_LOC_MASK_DELETE_EPHEMERIS_V02;
                  }

                  if (GNSS_AIDING_DATA_SV_ALMANAC_BIT & data.sv.svMask) {
                      delete_req.deleteSvInfoList[i].deleteSvInfoMask |=
                          QMI_LOC_MASK_DELETE_ALMANAC_V02;
                  }
              }
              // increment the current index
              curr_sv_idx += SV_ID_RANGE;

              //<<< SEC: delete all ephemeris and almanac correlation data for delete request of GPS ephemeris.
              /* do delete for all GLONASS SV's (65 - 96) */
              curr_sv_len += SV_ID_RANGE;

              sv_id = GLONASS_SV_ID_OFFSET;

              delete_req.deleteSvInfoList_valid = 1;

              delete_req.deleteSvInfoList_len = curr_sv_len;

              LOC_LOGV("%s:%d]: Delete GLO SV info for index %d to %d"
                            "and sv id %d to %d \n",
                            __func__, __LINE__, curr_sv_idx, curr_sv_len - 1,
                            sv_id, sv_id+SV_ID_RANGE-1);


              for( uint32_t i = curr_sv_idx; i< curr_sv_len ; i++, sv_id++ )
              {
                  delete_req.deleteSvInfoList[i].gnssSvId = sv_id;

                  delete_req.deleteSvInfoList[i].system = eQMI_LOC_SV_SYSTEM_GLONASS_V02;

                  // set ephemeris mask for all GLO SV's
                  if(GNSS_AIDING_DATA_SV_EPHEMERIS_BIT & data.sv.svMask)
                      delete_req.deleteSvInfoList[i].deleteSvInfoMask |=
                          QMI_LOC_MASK_DELETE_EPHEMERIS_V02;
                  // set almanac mask for all GLO SV's
                  if(GNSS_AIDING_DATA_SV_ALMANAC_BIT & data.sv.svMask)
                      delete_req.deleteSvInfoList[i].deleteSvInfoMask |=
                          QMI_LOC_MASK_DELETE_ALMANAC_V02;
          }
              curr_sv_idx += SV_ID_RANGE;

              /*Delete BeiDou SV info*/

              sv_id = BDS_SV_ID_OFFSET;

              delete_req.deleteBdsSvInfoList_valid = 1;

              delete_req.deleteBdsSvInfoList_len = BDS_SV_ID_RANGE;

              LOC_LOGV("%s:%d]: Delete BDS SV info for index 0 to %d"
                       "and sv id %d to %d \n",
                       __func__, __LINE__,
                       BDS_SV_ID_RANGE - 1,
                       sv_id, sv_id+BDS_SV_ID_RANGE - 1);

              for( uint32_t i = 0; i < BDS_SV_ID_RANGE; i++, sv_id++ )
              {
                  delete_req.deleteBdsSvInfoList[i].gnssSvId = sv_id;

                   // set ephemeris mask for all BDS SV's
                   if(GNSS_AIDING_DATA_SV_EPHEMERIS_BIT & data.sv.svMask)
                      delete_req.deleteBdsSvInfoList[i].deleteSvInfoMask |=
                          QMI_LOC_MASK_DELETE_EPHEMERIS_V02;
                  if(GNSS_AIDING_DATA_SV_ALMANAC_BIT & data.sv.svMask)
                      delete_req.deleteBdsSvInfoList[i].deleteSvInfoMask |=
                          QMI_LOC_MASK_DELETE_ALMANAC_V02;
              }
              curr_sv_idx += BDS_SV_ID_RANGE;

              /* Delete Galileo SV info*/
              sv_id = GAL_SV_ID_OFFSET;

              delete_req.deleteGalSvInfoList_valid = 1;

              delete_req.deleteGalSvInfoList_len = GAL_SV_ID_RANGE;

              LOC_LOGV("%s:%d]: Delete GAL SV info for index 0 to %d"
                       "and sv id %d to %d \n",
                       __func__, __LINE__,
                       GAL_SV_ID_RANGE - 1,
                       sv_id, sv_id+GAL_SV_ID_RANGE - 1);

              for( uint32_t i = 0; i < GAL_SV_ID_RANGE; i++, sv_id++ )
              {
                  delete_req.deleteGalSvInfoList[i].gnssSvId = sv_id;

                  // set ephemeris mask for all GAL SV's
                  if(GNSS_AIDING_DATA_SV_EPHEMERIS_BIT & data.sv.svMask)
                      delete_req.deleteGalSvInfoList[i].deleteSvInfoMask |=
                          QMI_LOC_MASK_DELETE_EPHEMERIS_V02;
                  if(GNSS_AIDING_DATA_SV_ALMANAC_BIT & data.sv.svMask)
                      delete_req.deleteGalSvInfoList[i].deleteSvInfoMask |=
                          QMI_LOC_MASK_DELETE_ALMANAC_V02;
              }
              curr_sv_idx += GAL_SV_ID_RANGE;

              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_GPS_ALM_CORR_V02;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_GLO_ALM_CORR_V02;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_BDS_ALM_CORR_V02;
			  delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_GAL_ALM_CORR_V02;
              // SEC >>>
          }

          if (GNSS_AIDING_DATA_COMMON_POSITION_BIT & data.common.mask) {
              delete_req.deleteGnssDataMask_valid = 1;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_POSITION_V02;
          }
          if (GNSS_AIDING_DATA_COMMON_TIME_BIT & data.common.mask) {
              delete_req.deleteGnssDataMask_valid = 1;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_TIME_V02;
              // < SEC_GPS : delete all time data.
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_GPS_TIME_V02;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_GLO_TIME_V02;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_BDS_TIME_V02;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_GAL_TIME_V02;
              // SEC_GPS >
          }
          if ((GNSS_AIDING_DATA_SV_IONOSPHERE_BIT & data.sv.svMask) &&
              (GNSS_AIDING_DATA_SV_TYPE_GPS_BIT & data.sv.svTypeMask)) {
              delete_req.deleteGnssDataMask_valid = 1;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_IONO_V02;
          }
          if (GNSS_AIDING_DATA_COMMON_UTC_BIT & data.common.mask)
          {
              delete_req.deleteGnssDataMask_valid = 1;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_UTC_V02;
          }
          if ((GNSS_AIDING_DATA_SV_HEALTH_BIT & data.sv.svMask) &&
              (GNSS_AIDING_DATA_SV_TYPE_GPS_BIT & data.sv.svTypeMask)) {
              delete_req.deleteGnssDataMask_valid = 1;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_HEALTH_V02;
          }
          if ((GNSS_AIDING_DATA_SV_DIRECTION_BIT & data.sv.svMask) &&
              (GNSS_AIDING_DATA_SV_TYPE_GPS_BIT & data.sv.svTypeMask)) {
              delete_req.deleteGnssDataMask_valid = 1;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_GPS_SVDIR_V02;
              // < SEC_GPS : delete all SVDIR data.
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_GLO_SVDIR_V02;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_BDS_SVDIR_V02;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_GAL_SVDIR_V02;
              // SEC_GPS >
          }
          if ((GNSS_AIDING_DATA_SV_SA_DATA_BIT & data.sv.svMask) &&
              (GNSS_AIDING_DATA_SV_TYPE_GPS_BIT & data.sv.svTypeMask)) {
              delete_req.deleteGnssDataMask_valid = 1;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_SADATA_V02;
          }
          if (GNSS_AIDING_DATA_COMMON_RTI_BIT & data.common.mask) {
              delete_req.deleteGnssDataMask_valid = 1;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_RTI_V02;
          }
          if (GNSS_AIDING_DATA_COMMON_CELLDB_BIT & data.common.mask) {
              delete_req.deleteCellDbDataMask_valid = 1;
              delete_req.deleteCellDbDataMask =
                  (QMI_LOC_MASK_DELETE_CELLDB_POS_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_LATEST_GPS_POS_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_OTA_POS_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_EXT_REF_POS_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_TIMETAG_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_CELLID_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_CACHED_CELLID_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_LAST_SRV_CELL_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_CUR_SRV_CELL_V02 |
                      QMI_LOC_MASK_DELETE_CELLDB_NEIGHBOR_INFO_V02);

          }
          // < SEC_GPS : delete all SVSTEER data.
          if(data.sv.svMask & GNSS_AIDING_DATA_SV_STEER_BIT)
          {
              delete_req.deleteGnssDataMask_valid = 1;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_GPS_SVSTEER_V02;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_GLO_SVSTEER_V02;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_BDS_SVSTEER_V02;
              delete_req.deleteGnssDataMask |= QMI_LOC_MASK_DELETE_GAL_SVSTEER_V02;
          }
          // SEC_GPS >
      }

      req_union.pDeleteAssistDataReq = &delete_req;

      status = locSyncSendReq(QMI_LOC_DELETE_ASSIST_DATA_REQ_V02,
                              req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                              QMI_LOC_DELETE_ASSIST_DATA_IND_V02,
                              &delete_resp);

      if (status != eLOC_CLIENT_SUCCESS ||
            eQMI_LOC_SUCCESS_V02 != delete_resp.status)
      {
          err = LOCATION_ERROR_GENERAL_FAILURE;
      }
  }

  if (adapterResponse != NULL) {
      adapterResponse->returnToSender(err);
  }
  }));
}

/* send NI user repsonse to the engine */
void
LocApiV02::informNiResponse(GnssNiResponse userResponse, const void* passThroughData)
{
    sendMsg(new LocApiMsg([this, userResponse, passThroughData] () {

        LocationError err = LOCATION_ERROR_SUCCESS;
        locClientReqUnionType req_union;
        locClientStatusEnumType status;
        qmiLocNiUserRespReqMsgT_v02 ni_resp;
        qmiLocNiUserRespIndMsgT_v02 ni_resp_ind;

        qmiLocEventNiNotifyVerifyReqIndMsgT_v02 *request_pass_back =
            (qmiLocEventNiNotifyVerifyReqIndMsgT_v02 *)passThroughData;

        memset(&ni_resp,0, sizeof(ni_resp));

        memset(&ni_resp_ind,0, sizeof(ni_resp_ind));

        switch (userResponse) {
        case GNSS_NI_RESPONSE_ACCEPT:
            ni_resp.userResp = eQMI_LOC_NI_LCS_NOTIFY_VERIFY_ACCEPT_V02;
            break;
        case GNSS_NI_RESPONSE_DENY:
            ni_resp.userResp = eQMI_LOC_NI_LCS_NOTIFY_VERIFY_DENY_V02;
            break;
        case GNSS_NI_RESPONSE_NO_RESPONSE:
            ni_resp.userResp = eQMI_LOC_NI_LCS_NOTIFY_VERIFY_NORESP_V02;
            break;
        default:
            err = LOCATION_ERROR_INVALID_PARAMETER;
            free((void *)passThroughData);
            return;
        }

        LOC_LOGV("informNiResponse: user response: %d", ni_resp.userResp);

        ni_resp.notificationType = request_pass_back->notificationType;

        // copy SUPL payload from request
        if (1 == request_pass_back->NiSuplInd_valid) {
            ni_resp.NiSuplPayload_valid = 1;
            memcpy(&(ni_resp.NiSuplPayload),
                   &(request_pass_back->NiSuplInd),
                   sizeof(qmiLocNiSuplNotifyVerifyStructT_v02));
        }
        // should this be an "else if"?? we don't need to decide

        // copy UMTS-CP payload from request
        if (1 == request_pass_back->NiUmtsCpInd_valid) {
            ni_resp.NiUmtsCpPayload_valid = 1;
            memcpy(&(ni_resp.NiUmtsCpPayload),
                   &(request_pass_back->NiUmtsCpInd),
                   sizeof(qmiLocNiUmtsCpNotifyVerifyStructT_v02));
        }

        //copy Vx payload from the request
        if (1 == request_pass_back->NiVxInd_valid) {
            ni_resp.NiVxPayload_valid = 1;
            memcpy(&(ni_resp.NiVxPayload),
                   &(request_pass_back->NiVxInd),
                   sizeof(qmiLocNiVxNotifyVerifyStructT_v02));
        }

        // copy Vx service interaction payload from the request
        if (1 == request_pass_back->NiVxServiceInteractionInd_valid) {
            ni_resp.NiVxServiceInteractionPayload_valid = 1;
            memcpy(&(ni_resp.NiVxServiceInteractionPayload),
                   &(request_pass_back->NiVxServiceInteractionInd),
                   sizeof(qmiLocNiVxServiceInteractionStructT_v02));
        }

        // copy Network Initiated SUPL Version 2 Extension
        if (1 == request_pass_back->NiSuplVer2ExtInd_valid) {
            ni_resp.NiSuplVer2ExtPayload_valid = 1;
            memcpy(&(ni_resp.NiSuplVer2ExtPayload),
                   &(request_pass_back->NiSuplVer2ExtInd),
                   sizeof(qmiLocNiSuplVer2ExtStructT_v02));
        }

        // copy SUPL Emergency Notification
        if (request_pass_back->suplEmergencyNotification_valid) {
            ni_resp.suplEmergencyNotification_valid = 1;
            memcpy(&(ni_resp.suplEmergencyNotification),
                   &(request_pass_back->suplEmergencyNotification),
                   sizeof(qmiLocEmergencyNotificationStructT_v02));
        }

        req_union.pNiUserRespReq = &ni_resp;

        status = locSyncSendReq(QMI_LOC_NI_USER_RESPONSE_REQ_V02,
                                req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                                QMI_LOC_NI_USER_RESPONSE_IND_V02,
                                &ni_resp_ind);

        if (status != eLOC_CLIENT_SUCCESS ||
                eQMI_LOC_SUCCESS_V02 != ni_resp_ind.status) {
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }

        free((void *)passThroughData);
    }));
}

void
LocApiV02::registerMasterClient()
{
  LocationError err = LOCATION_ERROR_SUCCESS;
  locClientReqUnionType req_union;
  locClientStatusEnumType status;
  qmiLocRegisterMasterClientReqMsgT_v02 reg_master_client_req;
  qmiLocRegisterMasterClientIndMsgT_v02 reg_master_client_ind;

  memset(&reg_master_client_req, 0, sizeof(reg_master_client_req));
  memset(&reg_master_client_ind, 0, sizeof(reg_master_client_ind));

  reg_master_client_req.key = 0xBAABCDEF;

  req_union.pRegisterMasterClientReq = &reg_master_client_req;

  status = locSyncSendReq(QMI_LOC_REGISTER_MASTER_CLIENT_REQ_V02,
                          req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                          QMI_LOC_REGISTER_MASTER_CLIENT_IND_V02,
                          &reg_master_client_ind);

  if (eLOC_CLIENT_SUCCESS != status ||
        eQMI_LOC_REGISTER_MASTER_CLIENT_SUCCESS_V02 != reg_master_client_ind.status) {
    if (eLOC_CLIENT_FAILURE_INVALID_MESSAGE_ID == status ||
        eLOC_CLIENT_FAILURE_UNSUPPORTED == status) {
      mMasterRegisterNotSupported = true;
    } else {
      mMasterRegisterNotSupported = false;
    }
    LOC_LOGv("mMasterRegisterNotSupported = %d", mMasterRegisterNotSupported);
    err = LOCATION_ERROR_GENERAL_FAILURE;
  }
}

/* Set UMTs SLP server URL */
LocationError
LocApiV02::setServerSync(const char* url, int len, LocServerType type)
{
  LocationError err = LOCATION_ERROR_SUCCESS;
  locClientReqUnionType req_union;
  locClientStatusEnumType status;
  qmiLocSetServerReqMsgT_v02 set_server_req;
  qmiLocSetServerIndMsgT_v02 set_server_ind;

  if(len < 0 || (size_t)len > sizeof(set_server_req.urlAddr))
  {
    LOC_LOGe("len = %d greater than max allowed url length", len);

    return LOCATION_ERROR_INVALID_PARAMETER;
  }

  memset(&set_server_req, 0, sizeof(set_server_req));
  memset(&set_server_ind, 0, sizeof(set_server_ind));

  LOC_LOGd("url = %s, len = %d type=%d", url, len, type);

  if (LOC_AGPS_MO_SUPL_SERVER == type) {
      set_server_req.serverType = eQMI_LOC_SERVER_TYPE_CUSTOM_SLP_V02;
  } else {
      set_server_req.serverType = eQMI_LOC_SERVER_TYPE_UMTS_SLP_V02;
  }

  set_server_req.urlAddr_valid = 1;

  strlcpy(set_server_req.urlAddr, url, sizeof(set_server_req.urlAddr));

  req_union.pSetServerReq = &set_server_req;

  status = locSyncSendReq(QMI_LOC_SET_SERVER_REQ_V02,
                          req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                          QMI_LOC_SET_SERVER_IND_V02,
                          &set_server_ind);

  if (status != eLOC_CLIENT_SUCCESS ||
         eQMI_LOC_SUCCESS_V02 != set_server_ind.status)
  {
    err = LOCATION_ERROR_GENERAL_FAILURE;
  }

  return err;
}

LocationError
LocApiV02::setServerSync(unsigned int ip, int port, LocServerType type)
{
  LocationError err = LOCATION_ERROR_SUCCESS;
  locClientReqUnionType req_union;
  locClientStatusEnumType status;
  qmiLocSetServerReqMsgT_v02 set_server_req;
  qmiLocSetServerIndMsgT_v02 set_server_ind;
  qmiLocServerTypeEnumT_v02 set_server_cmd;

  switch (type) {
  case LOC_AGPS_MPC_SERVER:
      set_server_cmd = eQMI_LOC_SERVER_TYPE_CDMA_MPC_V02;
      break;
  case LOC_AGPS_CUSTOM_PDE_SERVER:
      set_server_cmd = eQMI_LOC_SERVER_TYPE_CUSTOM_PDE_V02;
      break;
  default:
      set_server_cmd = eQMI_LOC_SERVER_TYPE_CDMA_PDE_V02;
      break;
  }

  memset(&set_server_req, 0, sizeof(set_server_req));
  memset(&set_server_ind, 0, sizeof(set_server_ind));

  LOC_LOGd("ip = %u, port = %d", ip, port);

  set_server_req.serverType = set_server_cmd;
  set_server_req.ipv4Addr_valid = 1;
  set_server_req.ipv4Addr.addr = ip;
  set_server_req.ipv4Addr.port = port;

  req_union.pSetServerReq = &set_server_req;

  status = locSyncSendReq(QMI_LOC_SET_SERVER_REQ_V02,
                          req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                          QMI_LOC_SET_SERVER_IND_V02,
                          &set_server_ind);

  if (status != eLOC_CLIENT_SUCCESS ||
      eQMI_LOC_SUCCESS_V02 != set_server_ind.status)
  {
    err = LOCATION_ERROR_GENERAL_FAILURE;
  }

  return err;
}

void LocApiV02 :: atlOpenStatus(
  int handle, int is_succ, char* apn, uint32_t apnLen, AGpsBearerType bear,
  LocAGpsType /*agpsType*/, LocApnTypeMask apnTypeMask)
{
  sendMsg(new LocApiMsg([this, handle, is_succ,
                        apnStr=std::string(apn, apnLen),
                        bear, apnTypeMask] () {

  locClientStatusEnumType result = eLOC_CLIENT_SUCCESS;
  locClientReqUnionType req_union;
  qmiLocInformLocationServerConnStatusReqMsgT_v02 conn_status_req;
  qmiLocInformLocationServerConnStatusIndMsgT_v02 conn_status_ind;

  LOC_LOGD("atlOpenStatus: handle = %d, is_succ = %d, APN = [%s], bearer = %d, "
           "apnTypeMask 0x%X" PRIx32, handle, is_succ, apnStr.c_str(),
           bear, apnTypeMask);

  memset(&conn_status_req, 0, sizeof(conn_status_req));
  memset(&conn_status_ind, 0, sizeof(conn_status_ind));

  // Fill in data
  conn_status_req.connHandle = handle;

  conn_status_req.requestType = eQMI_LOC_SERVER_REQUEST_OPEN_V02;

  if(is_succ)
  {
    conn_status_req.statusType = eQMI_LOC_SERVER_REQ_STATUS_SUCCESS_V02;

    if(!apnStr.empty()) {
        strlcpy(conn_status_req.apnProfile.apnName, apnStr.c_str(),
                sizeof(conn_status_req.apnProfile.apnName) );
    }

    switch(bear)
    {
    case AGPS_APN_BEARER_IPV4:
        conn_status_req.apnProfile.pdnType =
            eQMI_LOC_APN_PROFILE_PDN_TYPE_IPV4_V02;
        conn_status_req.apnProfile_valid = 1;
        break;

    case AGPS_APN_BEARER_IPV6:
        conn_status_req.apnProfile.pdnType =
            eQMI_LOC_APN_PROFILE_PDN_TYPE_IPV6_V02;
        conn_status_req.apnProfile_valid = 1;
        break;

    case AGPS_APN_BEARER_IPV4V6:
        conn_status_req.apnProfile.pdnType =
            eQMI_LOC_APN_PROFILE_PDN_TYPE_IPV4V6_V02;
        conn_status_req.apnProfile_valid = 1;
        break;

    case AGPS_APN_BEARER_INVALID:
        conn_status_req.apnProfile_valid = 0;
        break;

    default:
        LOC_LOGE("atlOpenStatus: invalid bearer type");
        conn_status_req.apnProfile_valid = 0;
        return;
    }

    // Populate apnTypeMask
    if (0 != apnTypeMask) {
        conn_status_req.apnTypeMask_valid = true;
        conn_status_req.apnTypeMask = convertLocApnTypeMask(apnTypeMask);
    }

  }
  else
  {
    conn_status_req.statusType = eQMI_LOC_SERVER_REQ_STATUS_FAILURE_V02;
  }

  req_union.pInformLocationServerConnStatusReq = &conn_status_req;

  result = locSyncSendReq(QMI_LOC_INFORM_LOCATION_SERVER_CONN_STATUS_REQ_V02,
                          req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                          QMI_LOC_INFORM_LOCATION_SERVER_CONN_STATUS_IND_V02,
                          &conn_status_ind);
  }));
}


/* close atl connection */
void LocApiV02 :: atlCloseStatus(int handle, int is_succ)
{
  sendMsg(new LocApiMsg([this, handle, is_succ] () {

  locClientStatusEnumType result = eLOC_CLIENT_SUCCESS;
  locClientReqUnionType req_union;
  qmiLocInformLocationServerConnStatusReqMsgT_v02 conn_status_req;
  qmiLocInformLocationServerConnStatusIndMsgT_v02 conn_status_ind;

  LOC_LOGD("atlCloseStatus: handle = %d, is_succ = %d", handle, is_succ);

  memset(&conn_status_req, 0, sizeof(conn_status_req));
  memset(&conn_status_ind, 0, sizeof(conn_status_ind));

  // Fill in data
  conn_status_req.connHandle = handle;

  conn_status_req.requestType = eQMI_LOC_SERVER_REQUEST_CLOSE_V02;

  if(is_succ)
  {
    conn_status_req.statusType = eQMI_LOC_SERVER_REQ_STATUS_SUCCESS_V02;
  }
  else
  {
    conn_status_req.statusType = eQMI_LOC_SERVER_REQ_STATUS_FAILURE_V02;
  }

  req_union.pInformLocationServerConnStatusReq = &conn_status_req;

  result = locSyncSendReq(QMI_LOC_INFORM_LOCATION_SERVER_CONN_STATUS_REQ_V02,
                          req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                          QMI_LOC_INFORM_LOCATION_SERVER_CONN_STATUS_IND_V02,
                          &conn_status_ind);
  }));
}

/* set the SUPL version */
LocationError
LocApiV02::setSUPLVersionSync(GnssConfigSuplVersion version)
{
  LocationError err = LOCATION_ERROR_SUCCESS;
  locClientStatusEnumType result = eLOC_CLIENT_SUCCESS;
  locClientReqUnionType req_union;

  qmiLocSetProtocolConfigParametersReqMsgT_v02 supl_config_req;
  qmiLocSetProtocolConfigParametersIndMsgT_v02 supl_config_ind;

  LOC_LOGd("supl version = %d", version);

  memset(&supl_config_req, 0, sizeof(supl_config_req));
  memset(&supl_config_ind, 0, sizeof(supl_config_ind));

  supl_config_req.suplVersion_valid = 1;

  ////SEC: customize enum for SUPL version
  switch (version) {
    case GNSS_CONFIG_SUPL_VERSION_2_0_4:
      supl_config_req.suplVersion = eQMI_LOC_SUPL_VERSION_2_0_4_V02;
      break;
    case GNSS_CONFIG_SUPL_VERSION_1_0_0:
      supl_config_req.suplVersion = eQMI_LOC_SUPL_VERSION_1_0_V02;
      break;
    case GNSS_CONFIG_SUPL_VERSION_2_0_2:
    case GNSS_CONFIG_SUPL_VERSION_2_0_2_AGNSS:
      supl_config_req.suplVersion = eQMI_LOC_SUPL_VERSION_2_0_2_V02;
      break;
    case GNSS_CONFIG_SUPL_VERSION_2_0_1:
    case GNSS_CONFIG_SUPL_VERSION_2_0_1_AGNSS:
      supl_config_req.suplVersion = eQMI_LOC_SUPL_VERSION_2_0_1_V02;
      break;
    case GNSS_CONFIG_SUPL_VERSION_2_0_0:
    case GNSS_CONFIG_SUPL_VERSION_2_0_0_AGNSS:
    default:
      supl_config_req.suplVersion = eQMI_LOC_SUPL_VERSION_2_0_V02;
      break;
  }

  req_union.pSetProtocolConfigParametersReq = &supl_config_req;

  result = locSyncSendReq(QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_REQ_V02,
                          req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                          QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_IND_V02,
                          &supl_config_ind);

  if(result != eLOC_CLIENT_SUCCESS ||
     eQMI_LOC_SUCCESS_V02 != supl_config_ind.status)
  {
     err = LOCATION_ERROR_GENERAL_FAILURE;
  }

  return err;
}

/* set the configuration for LTE positioning profile (LPP) */
LocationError
LocApiV02::setLPPConfigSync(GnssConfigLppProfileMask profileMask)
{
  LocationError err = LOCATION_ERROR_SUCCESS;
  locClientStatusEnumType result = eLOC_CLIENT_SUCCESS;
  locClientReqUnionType req_union;
  qmiLocSetProtocolConfigParametersReqMsgT_v02 lpp_config_req;
  qmiLocSetProtocolConfigParametersIndMsgT_v02 lpp_config_ind;

  LOC_LOGd("lpp profile mask = 0x%x", profileMask);

  memset(&lpp_config_req, 0, sizeof(lpp_config_req));
  memset(&lpp_config_ind, 0, sizeof(lpp_config_ind));

  lpp_config_req.lppConfig_valid = 1;
  if (profileMask & GNSS_CONFIG_LPP_PROFILE_USER_PLANE_BIT) {
      lpp_config_req.lppConfig |= QMI_LOC_LPP_CONFIG_ENABLE_USER_PLANE_V02;
  }
  if (profileMask & GNSS_CONFIG_LPP_PROFILE_CONTROL_PLANE_BIT) {
      lpp_config_req.lppConfig |= QMI_LOC_LPP_CONFIG_ENABLE_CONTROL_PLANE_V02;
  }
  if (profileMask & GNSS_CONFIG_LPP_PROFILE_USER_PLANE_OVER_NR5G_SA_BIT) {
      lpp_config_req.lppConfig |= QMI_LOC_LPP_CONFIG_ENABLE_USER_PLANE_OVER_NR5G_SA_V02;
  }
  if (profileMask & GNSS_CONFIG_LPP_PROFILE_CONTROL_PLANE_OVER_NR5G_SA_BIT) {
      lpp_config_req.lppConfig |= QMI_LOC_LPP_CONFIG_ENABLE_CONTROL_PLANE_OVER_NR5G_SA_V02;
  }

  req_union.pSetProtocolConfigParametersReq = &lpp_config_req;

  result = locSyncSendReq(QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_REQ_V02,
                          req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                          QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_IND_V02,
                          &lpp_config_ind);

  if(result != eLOC_CLIENT_SUCCESS ||
     eQMI_LOC_SUCCESS_V02 != lpp_config_ind.status)
  {
     err = LOCATION_ERROR_GENERAL_FAILURE;
  }

  return err;
}


/* set the Sensor Properties */
enum loc_api_adapter_err LocApiV02 :: setSensorPropertiesSync(
        bool gyroBiasVarianceRandomWalk_valid, float gyroBiasVarianceRandomWalk,
        bool accelBiasVarianceRandomWalk_valid, float accelBiasVarianceRandomWalk,
        bool angleBiasVarianceRandomWalk_valid, float angleBiasVarianceRandomWalk,
        bool rateBiasVarianceRandomWalk_valid, float rateBiasVarianceRandomWalk,
        bool velocityBiasVarianceRandomWalk_valid, float velocityBiasVarianceRandomWalk)
{
  locClientStatusEnumType result = eLOC_CLIENT_SUCCESS;
  locClientReqUnionType req_union;

  qmiLocSetSensorPropertiesReqMsgT_v02 sensor_prop_req;
  qmiLocSetSensorPropertiesIndMsgT_v02 sensor_prop_ind;

  LOC_LOGi("sensors prop: gyroBiasRandomWalk = %f, accelRandomWalk = %f, "
           "angleRandomWalk = %f, rateRandomWalk = %f, velocityRandomWalk = %f",
           gyroBiasVarianceRandomWalk, accelBiasVarianceRandomWalk,
           angleBiasVarianceRandomWalk, rateBiasVarianceRandomWalk, velocityBiasVarianceRandomWalk);

  memset(&sensor_prop_req, 0, sizeof(sensor_prop_req));
  memset(&sensor_prop_ind, 0, sizeof(sensor_prop_ind));

  /* Set the validity bit and value for each sensor property */
  sensor_prop_req.gyroBiasVarianceRandomWalk_valid = gyroBiasVarianceRandomWalk_valid;
  sensor_prop_req.gyroBiasVarianceRandomWalk = gyroBiasVarianceRandomWalk;

  sensor_prop_req.accelerationRandomWalkSpectralDensity_valid = accelBiasVarianceRandomWalk_valid;
  sensor_prop_req.accelerationRandomWalkSpectralDensity = accelBiasVarianceRandomWalk;

  sensor_prop_req.angleRandomWalkSpectralDensity_valid = angleBiasVarianceRandomWalk_valid;
  sensor_prop_req.angleRandomWalkSpectralDensity = angleBiasVarianceRandomWalk;

  sensor_prop_req.rateRandomWalkSpectralDensity_valid = rateBiasVarianceRandomWalk_valid;
  sensor_prop_req.rateRandomWalkSpectralDensity = rateBiasVarianceRandomWalk;

  sensor_prop_req.velocityRandomWalkSpectralDensity_valid = velocityBiasVarianceRandomWalk_valid;
  sensor_prop_req.velocityRandomWalkSpectralDensity = velocityBiasVarianceRandomWalk;

  req_union.pSetSensorPropertiesReq = &sensor_prop_req;

  result = locSyncSendReq(QMI_LOC_SET_SENSOR_PROPERTIES_REQ_V02,
                          req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                          QMI_LOC_SET_SENSOR_PROPERTIES_IND_V02,
                          &sensor_prop_ind);

  return convertErr(result);
}

/* set the Sensor Performance Config */
enum loc_api_adapter_err LocApiV02 :: setSensorPerfControlConfigSync(int controlMode,
        int accelSamplesPerBatch, int accelBatchesPerSec,
        int gyroSamplesPerBatch, int gyroBatchesPerSec,
        int accelSamplesPerBatchHigh, int accelBatchesPerSecHigh,
        int gyroSamplesPerBatchHigh, int gyroBatchesPerSecHigh,
        int algorithmConfig)
{
  locClientStatusEnumType result = eLOC_CLIENT_SUCCESS;
  locClientReqUnionType req_union;

  qmiLocSetSensorPerformanceControlConfigReqMsgT_v02 sensor_perf_config_req;
  qmiLocSetSensorPerformanceControlConfigIndMsgT_v02 sensor_perf_config_ind;

  LOC_LOGd("Sensor Perf Control Config (performanceControlMode)(%u) "
           "accel(#smp,#batches) (%u,%u) gyro(#smp,#batches) (%u,%u) "
           "accel_high(#smp,#batches) (%u,%u) gyro_high(#smp,#batches) (%u,%u) "
           "algorithmConfig(%u)\n",
           controlMode,
           accelSamplesPerBatch,
           accelBatchesPerSec,
           gyroSamplesPerBatch,
           gyroBatchesPerSec,
           accelSamplesPerBatchHigh,
           accelBatchesPerSecHigh,
           gyroSamplesPerBatchHigh,
           gyroBatchesPerSecHigh,
           algorithmConfig);

  memset(&sensor_perf_config_req, 0, sizeof(sensor_perf_config_req));
  memset(&sensor_perf_config_ind, 0, sizeof(sensor_perf_config_ind));

  sensor_perf_config_req.performanceControlMode_valid = (controlMode == 2) ? 0 : 1;
  sensor_perf_config_req.performanceControlMode = (qmiLocSensorPerformanceControlModeEnumT_v02)controlMode;
  sensor_perf_config_req.accelSamplingSpec_valid = 1;
  sensor_perf_config_req.accelSamplingSpec.batchesPerSecond = accelBatchesPerSec;
  sensor_perf_config_req.accelSamplingSpec.samplesPerBatch = accelSamplesPerBatch;
  sensor_perf_config_req.gyroSamplingSpec_valid = 1;
  sensor_perf_config_req.gyroSamplingSpec.batchesPerSecond = gyroBatchesPerSec;
  sensor_perf_config_req.gyroSamplingSpec.samplesPerBatch = gyroSamplesPerBatch;
  sensor_perf_config_req.accelSamplingSpecHigh_valid = 1;
  sensor_perf_config_req.accelSamplingSpecHigh.batchesPerSecond = accelBatchesPerSecHigh;
  sensor_perf_config_req.accelSamplingSpecHigh.samplesPerBatch = accelSamplesPerBatchHigh;
  sensor_perf_config_req.gyroSamplingSpecHigh_valid = 1;
  sensor_perf_config_req.gyroSamplingSpecHigh.batchesPerSecond = gyroBatchesPerSecHigh;
  sensor_perf_config_req.gyroSamplingSpecHigh.samplesPerBatch = gyroSamplesPerBatchHigh;
  sensor_perf_config_req.algorithmConfig_valid = 1;
  sensor_perf_config_req.algorithmConfig = algorithmConfig;

  req_union.pSetSensorPerformanceControlConfigReq = &sensor_perf_config_req;

  result = locSyncSendReq(QMI_LOC_SET_SENSOR_PERFORMANCE_CONTROL_CONFIGURATION_REQ_V02,
                          req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                          QMI_LOC_SET_SENSOR_PERFORMANCE_CONTROL_CONFIGURATION_IND_V02,
                          &sensor_perf_config_ind);

  return convertErr(result);
}

/* set the Positioning Protocol on A-GLONASS system */
LocationError
LocApiV02::setAGLONASSProtocolSync(GnssConfigAGlonassPositionProtocolMask aGlonassProtocol)
{
  LocationError err = LOCATION_ERROR_SUCCESS;
  locClientStatusEnumType result = eLOC_CLIENT_SUCCESS;
  locClientReqUnionType req_union;
  qmiLocSetProtocolConfigParametersReqMsgT_v02 aGlonassProtocol_req;
  qmiLocSetProtocolConfigParametersIndMsgT_v02 aGlonassProtocol_ind;

  memset(&aGlonassProtocol_req, 0, sizeof(aGlonassProtocol_req));
  memset(&aGlonassProtocol_ind, 0, sizeof(aGlonassProtocol_ind));

  aGlonassProtocol_req.assistedGlonassProtocolMask_valid = 1;
  if (GNSS_CONFIG_RRC_CONTROL_PLANE_BIT & aGlonassProtocol) {
      aGlonassProtocol_req.assistedGlonassProtocolMask |=
          QMI_LOC_ASSISTED_GLONASS_PROTOCOL_MASK_RRC_CP_V02 ;
  }
  if (GNSS_CONFIG_RRLP_USER_PLANE_BIT & aGlonassProtocol) {
      aGlonassProtocol_req.assistedGlonassProtocolMask |=
          QMI_LOC_ASSISTED_GLONASS_PROTOCOL_MASK_RRLP_UP_V02;
  }
  if (GNSS_CONFIG_LLP_USER_PLANE_BIT & aGlonassProtocol) {
      aGlonassProtocol_req.assistedGlonassProtocolMask |=
          QMI_LOC_ASSISTED_GLONASS_PROTOCOL_MASK_LPP_UP_V02;
  }
  if (GNSS_CONFIG_LLP_CONTROL_PLANE_BIT & aGlonassProtocol) {
      aGlonassProtocol_req.assistedGlonassProtocolMask |=
          QMI_LOC_ASSISTED_GLONASS_PROTOCOL_MASK_LPP_CP_V02;
  }

  req_union.pSetProtocolConfigParametersReq = &aGlonassProtocol_req;

  LOC_LOGd("aGlonassProtocolMask = 0x%x",
           aGlonassProtocol_req.assistedGlonassProtocolMask);

  result = locSyncSendReq(QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_REQ_V02,
                          req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                          QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_IND_V02,
                          &aGlonassProtocol_ind);

  if(result != eLOC_CLIENT_SUCCESS ||
     eQMI_LOC_SUCCESS_V02 != aGlonassProtocol_ind.status)
  {
    err = LOCATION_ERROR_GENERAL_FAILURE;
  }

  return err;
}

LocationError
LocApiV02::setLPPeProtocolCpSync(GnssConfigLppeControlPlaneMask lppeCP)
{
  LocationError err = LOCATION_ERROR_SUCCESS;
  locClientStatusEnumType result = eLOC_CLIENT_SUCCESS;
  locClientReqUnionType req_union;
  qmiLocSetProtocolConfigParametersReqMsgT_v02 lppe_req;
  qmiLocSetProtocolConfigParametersIndMsgT_v02 lppe_ind;

  memset(&lppe_req, 0, sizeof(lppe_req));
  memset(&lppe_ind, 0, sizeof(lppe_ind));

  lppe_req.lppeCpConfig_valid = 1;
  if (GNSS_CONFIG_LPPE_CONTROL_PLANE_DBH_BIT & lppeCP) {
      lppe_req.lppeCpConfig |= QMI_LOC_LPPE_MASK_CP_DBH_V02;
  }
  if (GNSS_CONFIG_LPPE_CONTROL_PLANE_WLAN_AP_MEASUREMENTS_BIT & lppeCP) {
      lppe_req.lppeCpConfig |= QMI_LOC_LPPE_MASK_CP_AP_WIFI_MEASUREMENT_V02;
  }
  if (GNSS_CONFIG_LPPE_CONTROL_PLANE_SENSOR_BARO_MEASUREMENTS_BIT & lppeCP) {
      lppe_req.lppeCpConfig |= QMI_LOC_LPPE_MASK_CP_UBP_V02;
  }
  if (GNSS_CONFIG_LPPE_CONTROL_PLANE_NON_E911_BIT & lppeCP) {
      lppe_req.lppeCpConfig |= QMI_LOC_LPPE_MASK_CP_NON_E911_V02;
  }
  if (GNSS_CONFIG_LPPE_CONTROL_PLANE_CIV_ADDRESS_BIT & lppeCP) {
      lppe_req.lppeCpConfig |= QMI_LOC_LPPE_MASK_CP_CIV_ADDRESS_V02;
  }

  req_union.pSetProtocolConfigParametersReq = &lppe_req;

  LOC_LOGd("lppeCpConfig = 0x%" PRIx64, lppe_req.lppeCpConfig);

  result = locSyncSendReq(QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_REQ_V02,
                          req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                          QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_IND_V02,
                          &lppe_ind);

  if(result != eLOC_CLIENT_SUCCESS ||
     eQMI_LOC_SUCCESS_V02 != lppe_ind.status)
  {
    err = LOCATION_ERROR_GENERAL_FAILURE;
  }

  return err;
}

LocationError
LocApiV02::setLPPeProtocolUpSync(GnssConfigLppeUserPlaneMask lppeUP)
{
  LocationError err = LOCATION_ERROR_SUCCESS;
  locClientStatusEnumType result = eLOC_CLIENT_SUCCESS;
  locClientReqUnionType req_union;
  qmiLocSetProtocolConfigParametersReqMsgT_v02 lppe_req;
  qmiLocSetProtocolConfigParametersIndMsgT_v02 lppe_ind;

  memset(&lppe_req, 0, sizeof(lppe_req));
  memset(&lppe_ind, 0, sizeof(lppe_ind));
  memset(&req_union, 0, sizeof(req_union));


  lppe_req.lppeUpConfig_valid = 1;
  if (GNSS_CONFIG_LPPE_USER_PLANE_DBH_BIT & lppeUP) {
      lppe_req.lppeUpConfig |= QMI_LOC_LPPE_MASK_UP_DBH_V02;
  }
  if (GNSS_CONFIG_LPPE_USER_PLANE_WLAN_AP_MEASUREMENTS_BIT & lppeUP) {
      lppe_req.lppeUpConfig |= QMI_LOC_LPPE_MASK_UP_AP_WIFI_MEASUREMENT_V02;
  }
  if (GNSS_CONFIG_LPPE_USER_PLANE_SENSOR_BARO_MEASUREMENTS_BIT & lppeUP) {
      lppe_req.lppeUpConfig |= QMI_LOC_LPPE_MASK_UP_UBP_V02;
  }
  if (GNSS_CONFIG_LPPE_USER_PLANE_NON_E911_BIT & lppeUP) {
      lppe_req.lppeUpConfig |= QMI_LOC_LPPE_MASK_UP_NON_E911_V02;
  }
  if (GNSS_CONFIG_LPPE_USER_PLANE_CIV_ADDRESS_BIT & lppeUP) {
      lppe_req.lppeUpConfig |= QMI_LOC_LPPE_MASK_UP_CIV_ADDRESS_V02;
  }

  req_union.pSetProtocolConfigParametersReq = &lppe_req;

  LOC_LOGd("lppeUpConfig = 0x%" PRIx64, lppe_req.lppeUpConfig);

  result = locSyncSendReq(QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_REQ_V02,
                          req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                          QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_IND_V02,
                          &lppe_ind);

  if(result != eLOC_CLIENT_SUCCESS ||
     eQMI_LOC_SUCCESS_V02 != lppe_ind.status)
  {
    err = LOCATION_ERROR_GENERAL_FAILURE;
  }

  return err;
}

/* Convert event mask from loc eng to loc_api_v02 format */
locClientEventMaskType LocApiV02 :: convertLocClientEventMask(
  LOC_API_ADAPTER_EVENT_MASK_T mask)
{
  locClientEventMaskType eventMask = 0;

  if (mask & LOC_API_ADAPTER_BIT_PARSED_POSITION_REPORT)
      eventMask |= QMI_LOC_EVENT_MASK_POSITION_REPORT_V02;

  if (mask & LOC_API_ADAPTER_BIT_PARSED_UNPROPAGATED_POSITION_REPORT)
      eventMask |= QMI_LOC_EVENT_MASK_UNPROPAGATED_POSITION_REPORT_V02;

  if (mask & LOC_API_ADAPTER_BIT_SATELLITE_REPORT)
      eventMask |= QMI_LOC_EVENT_MASK_GNSS_SV_INFO_V02;

  /* treat NMEA_1Hz and NMEA_POSITION_REPORT the same*/
  if ((mask & LOC_API_ADAPTER_BIT_NMEA_POSITION_REPORT) ||
      (mask & LOC_API_ADAPTER_BIT_NMEA_1HZ_REPORT) ) {
      eventMask |= QMI_LOC_EVENT_MASK_NMEA_V02;
  }

  if (mask & LOC_API_ADAPTER_BIT_NI_NOTIFY_VERIFY_REQUEST)
      eventMask |= QMI_LOC_EVENT_MASK_NI_NOTIFY_VERIFY_REQ_V02;

  if (mask & LOC_API_ADAPTER_BIT_ASSISTANCE_DATA_REQUEST)
  {
    eventMask |= QMI_LOC_EVENT_MASK_INJECT_PREDICTED_ORBITS_REQ_V02;
    eventMask |= QMI_LOC_EVENT_MASK_QUERY_XTRA_INFO_V02;
  }

  if (mask & LOC_API_ADAPTER_BIT_ASSISTANCE_TIME_REQUEST)
  {
    eventMask |= QMI_LOC_EVENT_MASK_INJECT_TIME_REQ_V02;
    eventMask |= QMI_LOC_EVENT_MASK_QUERY_XTRA_INFO_V02;
  }

  if (mask & LOC_API_ADAPTER_BIT_POSITION_INJECTION_REQUEST)
      eventMask |= QMI_LOC_EVENT_MASK_INJECT_POSITION_REQ_V02;

  if (mask & LOC_API_ADAPTER_BIT_STATUS_REPORT)
  {
      eventMask |= (QMI_LOC_EVENT_MASK_ENGINE_STATE_V02);
  }

  if (mask & LOC_API_ADAPTER_BIT_LOCATION_SERVER_REQUEST)
      eventMask |= QMI_LOC_EVENT_MASK_LOCATION_SERVER_CONNECTION_REQ_V02;

  if (mask & LOC_API_ADAPTER_BIT_REQUEST_WIFI)
      eventMask |= QMI_LOC_EVENT_MASK_WIFI_REQ_V02;

  if (mask & LOC_API_ADAPTER_BIT_SENSOR_STATUS)
      eventMask |= QMI_LOC_EVENT_MASK_SENSOR_STREAMING_READY_STATUS_V02;

  if (mask & LOC_API_ADAPTER_BIT_REQUEST_TIME_SYNC)
      eventMask |= QMI_LOC_EVENT_MASK_TIME_SYNC_REQ_V02;

  if (mask & LOC_API_ADAPTER_BIT_REPORT_SPI)
      eventMask |= QMI_LOC_EVENT_MASK_SET_SPI_STREAMING_REPORT_V02;

  if (mask & LOC_API_ADAPTER_BIT_REPORT_NI_GEOFENCE)
      eventMask |= QMI_LOC_EVENT_MASK_NI_GEOFENCE_NOTIFICATION_V02;

  if (mask & LOC_API_ADAPTER_BIT_GEOFENCE_GEN_ALERT)
      eventMask |= QMI_LOC_EVENT_MASK_GEOFENCE_GEN_ALERT_V02;

  if (mask & LOC_API_ADAPTER_BIT_REPORT_GENFENCE_BREACH)
      eventMask |= QMI_LOC_EVENT_MASK_GEOFENCE_BREACH_NOTIFICATION_V02;

  if (mask & LOC_API_ADAPTER_BIT_BATCHED_GENFENCE_BREACH_REPORT) {
      if (ContextBase::isMessageSupported(LOC_API_ADAPTER_MESSAGE_BATCHED_GENFENCE_BREACH)) {
          eventMask |= QMI_LOC_EVENT_MASK_GEOFENCE_BATCH_BREACH_NOTIFICATION_V02;
      } else {
          eventMask |= QMI_LOC_EVENT_MASK_GEOFENCE_BREACH_NOTIFICATION_V02;
      }
  }

  if (mask & LOC_API_ADAPTER_BIT_PEDOMETER_CTRL)
      eventMask |= QMI_LOC_EVENT_MASK_PEDOMETER_CONTROL_V02;

  if (mask & LOC_API_ADAPTER_BIT_REPORT_GENFENCE_DWELL)
      eventMask |= QMI_LOC_EVENT_MASK_GEOFENCE_BATCH_DWELL_NOTIFICATION_V02;

  if (mask & LOC_API_ADAPTER_BIT_MOTION_CTRL)
      eventMask |= QMI_LOC_EVENT_MASK_MOTION_DATA_CONTROL_V02;

  if (mask & LOC_API_ADAPTER_BIT_REQUEST_WIFI_AP_DATA)
      eventMask |= QMI_LOC_EVENT_MASK_INJECT_WIFI_AP_DATA_REQ_V02;

  if(mask & LOC_API_ADAPTER_BIT_BATCH_FULL)
      eventMask |= QMI_LOC_EVENT_MASK_BATCH_FULL_NOTIFICATION_V02;

  if(mask & LOC_API_ADAPTER_BIT_BATCH_STATUS)
      eventMask |= QMI_LOC_EVENT_MASK_BATCHING_STATUS_V02;

  if(mask & LOC_API_ADAPTER_BIT_BATCHED_POSITION_REPORT)
      eventMask |= QMI_LOC_EVENT_MASK_LIVE_BATCHED_POSITION_REPORT_V02;

  if(mask & LOC_API_ADAPTER_BIT_GNSS_MEASUREMENT_REPORT)
        eventMask |= QMI_LOC_EVENT_MASK_GNSS_MEASUREMENT_REPORT_V02;

  if(mask & LOC_API_ADAPTER_BIT_GNSS_SV_POLYNOMIAL_REPORT)
      eventMask |= QMI_LOC_EVENT_MASK_GNSS_SV_POLYNOMIAL_REPORT_V02;

  if(mask & LOC_API_ADAPTER_BIT_GNSS_SV_EPHEMERIS_REPORT)
        eventMask |= QMI_LOC_EVENT_MASK_EPHEMERIS_REPORT_V02;

  // for GDT
  if(mask & LOC_API_ADAPTER_BIT_GDT_UPLOAD_BEGIN_REQ)
      eventMask |= QMI_LOC_EVENT_MASK_GDT_UPLOAD_BEGIN_REQ_V02;

  if(mask & LOC_API_ADAPTER_BIT_GDT_UPLOAD_END_REQ)
      eventMask |= QMI_LOC_EVENT_MASK_GDT_UPLOAD_END_REQ_V02;

  if (mask & LOC_API_ADAPTER_BIT_GNSS_MEASUREMENT)
      eventMask |= QMI_LOC_EVENT_MASK_GNSS_MEASUREMENT_REPORT_V02;

  if(mask & LOC_API_ADAPTER_BIT_REQUEST_TIMEZONE)
      eventMask |= QMI_LOC_EVENT_MASK_GET_TIME_ZONE_REQ_V02;

  if (mask & LOC_API_ADAPTER_BIT_FDCL_SERVICE_REQ)
      eventMask |= QMI_LOC_EVENT_MASK_FDCL_SERVICE_REQ_V02;

  if (mask & LOC_API_ADAPTER_BIT_BS_OBS_DATA_SERVICE_REQ)
      eventMask |= QMI_LOC_EVENT_MASK_BS_OBS_DATA_SERVICE_REQ_V02;

  if (mask & LOC_API_ADAPTER_BIT_LOC_SYSTEM_INFO)
      eventMask |= QMI_LOC_EVENT_MASK_NEXT_LS_INFO_REPORT_V02;

  if (mask & LOC_API_ADAPTER_BIT_EVENT_REPORT_INFO)
      eventMask |= QMI_LOC_EVENT_MASK_GNSS_EVENT_REPORT_V02;

  if (mask & LOC_API_ADAPTER_BIT_GNSS_NHZ_MEASUREMENT) {
      eventMask |= QMI_LOC_EVENT_MASK_GNSS_NHZ_MEASUREMENT_REPORT_V02;
  }

  if (mask & LOC_API_ADAPTER_BIT_LATENCY_INFORMATION) {
      eventMask |= QMI_LOC_EVENT_MASK_LATENCY_INFORMATION_REPORT_V02;
  }

  if (mask & LOC_API_ADAPTER_BIT_ENGINE_DEBUG_DATA_REPORT) {
     eventMask |= QMI_LOC_EVENT_MASK_ENGINE_DEBUG_DATA_REPORT_V02;
  }

  if (mask & LOC_API_ADAPTER_BIT_DISASTER_CRISIS_REPORT) {
      eventMask |= QMI_LOC_EVENT_MASK_DC_REPORT_V02;
  }

  if (mask & LOC_API_ADAPTER_BIT_ENGINE_LOCK_STATE_DATA_REPORT) {
     eventMask |= QMI_LOC_EVENT_MASK_ENGINE_LOCK_STATE_V02;
  }

  if (mask & LOC_API_ADAPTER_BIT_FEATURE_STATUS_UPDATE) {
      eventMask |= QMI_LOC_EVENT_MASK_FEATURE_STATUS_V02;
  }

  if (mask & LOC_API_ADAPTER_BIT_GNSS_BANDS_SUPPORTED) {
      eventMask |= QMI_LOC_EVENT_MASK_GNSS_BANDS_SUPPORTED_V02;
  }

  if (mask & LOC_API_ADAPTER_BIT_NTN_CONFIG_UPDATE) {
      eventMask |= QMI_LOC_EVENT_MASK_NTN_CONFIG_UPDATE_V02;
  }

  return eventMask;
}

qmiLocLockEnumT_v02 LocApiV02 ::convertGpsLockFromAPItoQMI(GnssConfigGpsLock lock)
{
    bool isAfwLocked = (lock & GNSS_CONFIG_GPS_LOCK_MO);
    bool areCpAndSuplLocked;
    if (ContextBase::isFeatureSupported(LOC_SUPPORTED_FEATURE_MULTIPLE_ATTRIBUTION_APPS)) {
        areCpAndSuplLocked = (lock & GNSS_CONFIG_GPS_LOCK_NFW_CP) &&
                             (lock & GNSS_CONFIG_GPS_LOCK_NFW_SUPL);
        LOC_LOGv("Multiple attribution apps supported isAfwLocked = %d "
                 "areCpAndSuplLocked = %d",
                 isAfwLocked, areCpAndSuplLocked);
    } else {
        areCpAndSuplLocked = (lock & GNSS_CONFIG_GPS_LOCK_NFW_ALL);
        LOC_LOGv("Multiple attribution apps not supported isAfwLocked = %d "
                 "areCpAndSuplLocked = %d",
                 isAfwLocked, areCpAndSuplLocked);
    }

    if (areCpAndSuplLocked) {
        if (isAfwLocked) {
            return eQMI_LOC_LOCK_ALL_V02;
        } else {
            return eQMI_LOC_LOCK_MT_V02;
        }
    } else {
        if (isAfwLocked) {
            return eQMI_LOC_LOCK_MI_V02;
        } else {
            return eQMI_LOC_LOCK_NONE_V02;
        }
    }
}

// QC CR3933472
qmiLocClientsMaskT_v02 LocApiV02::convertGpsLock(GnssConfigGpsLock lock) {
    qmiLocClientsMaskT_v02 nfwControlBits = 0;
    if (lock & GNSS_CONFIG_GPS_LOCK_NFW_IMS) {
        nfwControlBits |= QMI_LOC_MASK_UTH_CLIENT_IMS_V02;
    }
    if (lock & GNSS_CONFIG_GPS_LOCK_NFW_SIM) {
        nfwControlBits |= QMI_LOC_MASK_UTH_CLIENT_SIM_V02;
    }
    if (lock & GNSS_CONFIG_GPS_LOCK_NFW_MDT) {
        nfwControlBits |= QMI_LOC_MASK_UTH_CLIENT_MDT_V02;
    }
    if (lock & GNSS_CONFIG_GPS_LOCK_NFW_TLOC) {
        nfwControlBits |= QMI_LOC_MASK_UTH_CLIENT_TLOC_V02;
    }
    if (lock & GNSS_CONFIG_GPS_LOCK_NFW_RLOC) {
        nfwControlBits |= QMI_LOC_MASK_UTH_CLIENT_RLOC_V02;
    }
    if (lock & GNSS_CONFIG_GPS_LOCK_NFW_V2X) {
        nfwControlBits |= QMI_LOC_MASK_UTH_CLIENT_V2X_V02;
    }
    if (lock & GNSS_CONFIG_GPS_LOCK_NFW_R1) {
        nfwControlBits |= QMI_LOC_MASK_OEM_CLIENT_R1_V02;
    }
    if (lock & GNSS_CONFIG_GPS_LOCK_NFW_R2) {
        nfwControlBits |= QMI_LOC_MASK_OEM_CLIENT_R2_V02;
    }
    if (lock & GNSS_CONFIG_GPS_LOCK_NFW_R3) {
        nfwControlBits |= QMI_LOC_MASK_OEM_CLIENT_R3_V02;
    }
    if (lock & GNSS_CONFIG_GPS_LOCK_NFW_NTN) {
        nfwControlBits |= QMI_LOC_MASK_UTH_CLIENT_NTN_V02;
    }
    return nfwControlBits;
}

EngineLockState LocApiV02::convertEngineLockState(qmiLocEngineLockStateEnumT_v02 LockState)
{
    switch (LockState) {
      case eQMI_LOC_ENGINE_LOCK_STATE_ENABLED_V02:
        return ENGINE_LOCK_STATE_ENABLED;
      case eQMI_LOC_ENGINE_LOCK_STATE_DISABLED_V02:
        return ENGINE_LOCK_STATE_DISABLED;
      default:
        return ENGINE_LOCK_STATE_INVALID;
    }
}

/* Convert error from loc_api_v02 to loc eng format*/
enum loc_api_adapter_err LocApiV02 :: convertErr(
  locClientStatusEnumType status)
{
  switch( status)
  {
    case eLOC_CLIENT_SUCCESS:
      return LOC_API_ADAPTER_ERR_SUCCESS;

    case eLOC_CLIENT_FAILURE_GENERAL:
      return LOC_API_ADAPTER_ERR_GENERAL_FAILURE;

    case eLOC_CLIENT_FAILURE_UNSUPPORTED:
      return LOC_API_ADAPTER_ERR_UNSUPPORTED;

    case eLOC_CLIENT_FAILURE_INVALID_PARAMETER:
      return LOC_API_ADAPTER_ERR_INVALID_PARAMETER;

    case eLOC_CLIENT_FAILURE_ENGINE_BUSY:
      return LOC_API_ADAPTER_ERR_ENGINE_BUSY;

    case eLOC_CLIENT_FAILURE_PHONE_OFFLINE:
      return LOC_API_ADAPTER_ERR_PHONE_OFFLINE;

    case eLOC_CLIENT_FAILURE_TIMEOUT:
      return LOC_API_ADAPTER_ERR_TIMEOUT;

    case eLOC_CLIENT_FAILURE_INVALID_HANDLE:
      return LOC_API_ADAPTER_ERR_INVALID_HANDLE;

    case eLOC_CLIENT_FAILURE_SERVICE_NOT_PRESENT:
      return LOC_API_ADAPTER_ERR_SERVICE_NOT_PRESENT;

    case eLOC_CLIENT_FAILURE_INTERNAL:
      return LOC_API_ADAPTER_ERR_INTERNAL;

    default:
      return LOC_API_ADAPTER_ERR_FAILURE;
  }
}

bool LocApiV02::isMeasurementRefreshForSv(uint16_t gnssSvId,
        GnssSignalTypeMask gnssSignalTypeMask)
{
    if (m1HzMeasurementsInfo.measurements.size() > 0) {
        for (auto measurement : m1HzMeasurementsInfo.measurements) {
            if (measurement.svId == gnssSvId &&
                    measurement.gnssSignalType == gnssSignalTypeMask) {
                return true;
            }
        }
    }

    return false;
}

bool LocApiV02::isTOAValid(const qmiLocEventPositionReportIndMsgT_v02 *location_report_ptr,
        const GnssBasicMeasurementsInfo *pOneHzMeasurements)
{
    if (nullptr == location_report_ptr ||
        nullptr == pOneHzMeasurements) {
        LOC_LOGe("nullptr");
        return false;
    }

    if (!location_report_ptr->gpsTime_valid) {
        LOC_LOGw("location_report_ptr->gpsTime_valid = false");
        return false;
    }

    if (!(pOneHzMeasurements->clock.flags & GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_BIT)) {
        LOC_LOGw("GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_BIT not set");
        return false;
    }

    // PVT TOA in ms = (gpsWeek x WEEK_MSECS) + gpsTimeOfWeekMs
    uint64_t gpsTimePosReport = location_report_ptr->gpsTime.gpsWeek * WEEK_MSECS +
                                location_report_ptr->gpsTime.gpsTimeOfWeekMs;

    // Meas TOA in ms = (systemWeek x WEEK_MSECS) + systemMsec - systemClkTimeBias
    // clock.timeNs = (systemWeek x WEEK_MSECS) + systemMsec
    // clock.fullBiasNs = systemClkTimeBias
    int64_t gpsTimeNs = pOneHzMeasurements->clock.timeNs - pOneHzMeasurements->clock.fullBiasNs;
    uint64_t gpsTimeMeasReport = gpsTimeNs/NSEC_IN_MSEC;
    LOC_LOGa("gpsTimePosReport %" PRIu64 "gpsTimeMeasReport %" PRIu64 " ",
            gpsTimePosReport, gpsTimeMeasReport);

    // PVT TOA > Meas TOA && PVT TOA < Meas TOA+1.5sec
    if ((gpsTimePosReport > gpsTimeMeasReport) &&
        (gpsTimePosReport < gpsTimeMeasReport + 1500)) {
        return true;
    }

    return false;
}

/* convert position report to loc eng format and send the converted
   position to loc eng */

void LocApiV02 :: reportPosition (
  const qmiLocEventPositionReportIndMsgT_v02 *location_report_ptr,
  bool unpropagatedPosition)
{
    UlpLocation location;

    mHlosQtimer2 = getQTimerTickCount();
    LOC_LOGd("Reporting position from V2 Adapter, qtimer %" PRIu64 " ",
             mHlosQtimer2);
    memset(&location, 0, sizeof (UlpLocation));
    location.size = sizeof(location);
    location.unpropagatedPosition = unpropagatedPosition;
    GnssDataNotification dataNotify = {};
    uint16_t meaAvailForPVT[eQMI_LOC_SV_SYSTEM_NAVIC_V02] = {};

    GpsLocationExtended locationExtended;
    memset(&locationExtended, 0, sizeof (GpsLocationExtended));
    locationExtended.size = sizeof(locationExtended);

    locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_OUTPUT_ENG_TYPE;
    locationExtended.locOutputEngType = LOC_OUTPUT_ENGINE_SPE;
    locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_OUTPUT_ENG_MASK;
    locationExtended.locOutputEngMask = STANDARD_POSITIONING_ENGINE;

    struct timespec apTimestamp = {};
    if( clock_gettime( CLOCK_BOOTTIME, &apTimestamp)== 0)
    {
       locationExtended.timeStamp.apTimeStamp.tv_sec = apTimestamp.tv_sec;
       locationExtended.timeStamp.apTimeStamp.tv_nsec = apTimestamp.tv_nsec;
       locationExtended.timeStamp.apTimeStampUncertaintyMs = (float)ap_timestamp_uncertainty;
    }
    else
    {
       locationExtended.timeStamp.apTimeStampUncertaintyMs = FLT_MAX;
       LOC_LOGe("Error in clock_gettime() ");
    }
    LOC_LOGa("QMI_PosPacketTime %" PRIu64 "(sec) %" PRIu64 "(nsec), QMI_spoofReportMask %" PRIu64,
                 locationExtended.timeStamp.apTimeStamp.tv_sec,
                 locationExtended.timeStamp.apTimeStamp.tv_nsec,
                 location_report_ptr->spoofReportMask);

    // Process the position from final and intermediate reports
    memset(&dataNotify, 0, sizeof(dataNotify));

    if (location_report_ptr->jammerIndicatorListExt_valid) {
        for (uint32_t i = 0; i < location_report_ptr->jammerIndicatorListExt_len; i++) {
            int signalId = log2(location_report_ptr->jammerIndicatorListExt[i].gnssSignalType);
            if (signalId < GNSS_LOC_MAX_NUMBER_OF_SIGNAL_TYPES) {
                LOC_LOGa("signal type %d, agcMetricDb=%d, bpMetricDb=%d",
                        signalId, -location_report_ptr->jammerIndicatorListExt[i].bpMetricDb,
                        location_report_ptr->jammerIndicatorListExt[i].bpMetricDb);
                dataNotify.gnssDataMask[signalId] |=
                    GNSS_LOC_DATA_AGC_BIT | GNSS_LOC_DATA_JAMMER_IND_BIT;
                dataNotify.agc[signalId] =
                    -(double)location_report_ptr->jammerIndicatorListExt[i].bpMetricDb / 100.0;
                dataNotify.jammerInd[signalId] =
                    (double)location_report_ptr->jammerIndicatorListExt[i].bpMetricDb / 100.0;
            }
        }
    } else if (location_report_ptr->jammerIndicatorList_valid) {
        LOC_LOGa("jammerIndicator is present len=%d",
                 location_report_ptr->jammerIndicatorList_len);
        for (uint32_t i = 1; i < location_report_ptr->jammerIndicatorList_len; i++) {
            dataNotify.gnssDataMask[i-1] = 0;
            dataNotify.agc[i-1] = 0.0;
            dataNotify.jammerInd[i-1] = 0.0;
            if (GNSS_INVALID_JAMMER_IND !=
                location_report_ptr->jammerIndicatorList[i].bpMetricDb) {
                LOC_LOGa("agcMetricDb[%d]=%d; bpMetricDb[%d]=%d",
                         i, -location_report_ptr->jammerIndicatorList[i].bpMetricDb,
                         i, location_report_ptr->jammerIndicatorList[i].bpMetricDb);
                dataNotify.gnssDataMask[i-1] |=
                        GNSS_LOC_DATA_AGC_BIT | GNSS_LOC_DATA_JAMMER_IND_BIT;
                dataNotify.agc[i-1] =
                        -(double)location_report_ptr->jammerIndicatorList[i].bpMetricDb / 100.0;
                dataNotify.jammerInd[i-1] =
                        (double)location_report_ptr->jammerIndicatorList[i].bpMetricDb / 100.0;
            }
        }
    } else {
        LOC_LOGa("jammerIndicator is not present");
    }

    if ((false == mIsFirstFinalFixReported) &&
            (eQMI_LOC_SESS_STATUS_SUCCESS_V02 == location_report_ptr->sessionStatus)) {
        loc_boot_kpi_marker("L - LocApiV02 First Final Fix");
        mIsFirstFinalFixReported = true;
    }
    if((location_report_ptr->sessionStatus == eQMI_LOC_SESS_STATUS_SUCCESS_V02) ||
       (location_report_ptr->sessionStatus == eQMI_LOC_SESS_STATUS_IN_PROGRESS_V02)) {
        // Latitude & Longitude
        if ((1 == location_report_ptr->latitude_valid) &&
            (1 == location_report_ptr->longitude_valid))
        {
            location.gpsLocation.flags  |= LOC_GPS_LOCATION_HAS_LAT_LONG;
            location.gpsLocation.latitude  = location_report_ptr->latitude;
            location.gpsLocation.longitude = location_report_ptr->longitude;
            if (location_report_ptr->altitudeWrtEllipsoid_valid) {
                LocApiProxyBase* locApiProxyObj = getLocApiProxy();
                float geoidalSeparation = 0.0;
                if (nullptr != locApiProxyObj) {
                    geoidalSeparation = locApiProxyObj->getGeoidalSeparation(
                            location_report_ptr->latitude, location_report_ptr->longitude);
                    locationExtended.altitudeMeanSeaLevel =
                            location_report_ptr->altitudeWrtEllipsoid - geoidalSeparation;
                    locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_ALTITUDE_MEAN_SEA_LEVEL;
                }
            }
        } else {
            LocApiBase::reportData(dataNotify);
        }

        // Time stamp (UTC)
        if (location_report_ptr->timestampUtc_valid == 1)
        {
            location.gpsLocation.timestamp = location_report_ptr->timestampUtc;
        }

        // Altitude
        if (location_report_ptr->altitudeWrtEllipsoid_valid == 1)
        {
            location.gpsLocation.flags  |= LOC_GPS_LOCATION_HAS_ALTITUDE;
            location.gpsLocation.altitude = location_report_ptr->altitudeWrtEllipsoid;
        }

        // Altitude assumed or calculated info
        if (location_report_ptr->altitudeAssumed_valid == 1) {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_ALTITUDE_ASSUMED;
            locationExtended.altitudeAssumed = location_report_ptr->altitudeAssumed;
        }

        // Speed
        if (location_report_ptr->speedHorizontal_valid == 1)
        {
            location.gpsLocation.flags  |= LOC_GPS_LOCATION_HAS_SPEED;
            location.gpsLocation.speed = location_report_ptr->speedHorizontal;
        }

        // Heading
        if (location_report_ptr->heading_valid == 1)
        {
            location.gpsLocation.flags  |= LOC_GPS_LOCATION_HAS_BEARING;
            location.gpsLocation.bearing = location_report_ptr->heading;
        }

        // Uncertainty (circular)
        if (location_report_ptr->horUncCircular_valid) {
            location.gpsLocation.flags |= LOC_GPS_LOCATION_HAS_ACCURACY;
            location.gpsLocation.accuracy = location_report_ptr->horUncCircular;
        } else if (location_report_ptr->horUncEllipseSemiMinor_valid &&
                   location_report_ptr->horUncEllipseSemiMajor_valid) {
            location.gpsLocation.flags |= LOC_GPS_LOCATION_HAS_ACCURACY;
            location.gpsLocation.accuracy =
                sqrt((location_report_ptr->horUncEllipseSemiMinor *
                      location_report_ptr->horUncEllipseSemiMinor) +
                     (location_report_ptr->horUncEllipseSemiMajor *
                      location_report_ptr->horUncEllipseSemiMajor));
        } else {  // SEC
            LOC_LOGi("Location Accuracy is not valid. Discard this GNSS Location.");
            return;
        }

        // If horConfidence_valid is true, and horConfidence value is less than 68%
        // then scale the accuracy value to 68% confidence.
        if (location_report_ptr->horConfidence_valid)
        {
            bool is_CircUnc = (location_report_ptr->horUncCircular_valid) ?
                                                                    true : false;
            scaleAccuracyTo68PercentConfidence(location_report_ptr->horConfidence,
                                               location.gpsLocation,
                                               is_CircUnc);
        }

        // Technology Mask
        locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_POS_TECH_MASK;
        locationExtended.tech_mask = convertPosTechMask(location_report_ptr->technologyMask);
        location.tech_mask = locationExtended.tech_mask;

        //Mark the location source as from GNSS
        location.gpsLocation.flags |= LOC_GPS_LOCATION_HAS_SOURCE_INFO;
        location.position_source = ULP_LOCATION_IS_FROM_GNSS;

        if (location_report_ptr->spoofReportMask_valid)
        {
            location.gpsLocation.flags |= LOC_GPS_LOCATION_HAS_SPOOF_MASK;
            location.gpsLocation.spoof_mask = (uint32_t)location_report_ptr->spoofReportMask;
        }

        if (location_report_ptr->magneticDeviation_valid)
        {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_MAG_DEV;
            locationExtended.magneticDeviation = location_report_ptr->magneticDeviation;
        }

        if (location_report_ptr->conformityIndex_valid) {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_CONFORMITY_INDEX;
            locationExtended.conformityIndex = location_report_ptr->conformityIndex;
        }

        if (location_report_ptr->DOP_valid)
        {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_DOP;
            locationExtended.pdop = location_report_ptr->DOP.PDOP;
            locationExtended.hdop = location_report_ptr->DOP.HDOP;
            locationExtended.vdop = location_report_ptr->DOP.VDOP;
        }

        if (location_report_ptr->vertUnc_valid)
        {
           locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_VERT_UNC;
           locationExtended.vert_unc = location_report_ptr->vertUnc;
        }

        if (location_report_ptr->velUncEnu_valid)
        {
           locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_SPEED_UNC;
           locationExtended.speed_unc = sqrt(location_report_ptr->velUncEnu[0] *
                                             location_report_ptr->velUncEnu[0] +
                                             location_report_ptr->velUncEnu[1] *
                                             location_report_ptr->velUncEnu[1]);
        }
        if (location_report_ptr->headingUnc_valid)
        {
           locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_BEARING_UNC;
           locationExtended.bearing_unc = location_report_ptr->headingUnc;
        }
        if (location_report_ptr->horReliability_valid)
        {
           locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_HOR_RELIABILITY;
           switch (location_report_ptr->horReliability)
           {
              case eQMI_LOC_RELIABILITY_NOT_SET_V02 :
                locationExtended.horizontal_reliability = LOC_RELIABILITY_NOT_SET;
                break;
              case eQMI_LOC_RELIABILITY_VERY_LOW_V02 :
                locationExtended.horizontal_reliability = LOC_RELIABILITY_VERY_LOW;
                break;
              case eQMI_LOC_RELIABILITY_LOW_V02 :
                locationExtended.horizontal_reliability = LOC_RELIABILITY_LOW;
                break;
              case eQMI_LOC_RELIABILITY_MEDIUM_V02 :
                locationExtended.horizontal_reliability = LOC_RELIABILITY_MEDIUM;
                break;
              case eQMI_LOC_RELIABILITY_HIGH_V02 :
                locationExtended.horizontal_reliability = LOC_RELIABILITY_HIGH;
                break;
              default:
                locationExtended.horizontal_reliability = LOC_RELIABILITY_NOT_SET;
                break;
           }
        }
        if (location_report_ptr->vertReliability_valid)
        {
           locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_VERT_RELIABILITY;
           switch (location_report_ptr->vertReliability)
           {
              case eQMI_LOC_RELIABILITY_NOT_SET_V02 :
                locationExtended.vertical_reliability = LOC_RELIABILITY_NOT_SET;
                break;
              case eQMI_LOC_RELIABILITY_VERY_LOW_V02 :
                locationExtended.vertical_reliability = LOC_RELIABILITY_VERY_LOW;
                break;
              case eQMI_LOC_RELIABILITY_LOW_V02 :
                locationExtended.vertical_reliability = LOC_RELIABILITY_LOW;
                break;
              case eQMI_LOC_RELIABILITY_MEDIUM_V02 :
                locationExtended.vertical_reliability = LOC_RELIABILITY_MEDIUM;
                break;
              case eQMI_LOC_RELIABILITY_HIGH_V02 :
                locationExtended.vertical_reliability = LOC_RELIABILITY_HIGH;
                break;
              default:
                locationExtended.vertical_reliability = LOC_RELIABILITY_NOT_SET;
                break;
           }
        }

        if (location_report_ptr->horUncEllipseSemiMajor_valid)
        {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_HOR_ELIP_UNC_MAJOR;
            locationExtended.horUncEllipseSemiMajor = location_report_ptr->horUncEllipseSemiMajor;
        }
        if (location_report_ptr->horUncEllipseSemiMinor_valid)
        {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_HOR_ELIP_UNC_MINOR;
            locationExtended.horUncEllipseSemiMinor = location_report_ptr->horUncEllipseSemiMinor;
        }
        if (location_report_ptr->horUncEllipseOrientAzimuth_valid)
        {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_HOR_ELIP_UNC_AZIMUTH;
            locationExtended.horUncEllipseOrientAzimuth =
                location_report_ptr->horUncEllipseOrientAzimuth;
        }

        // If the horizontal uncertainty ellipse info is available,
        // calculate the horizontal uncertainty along north and east
        if (location_report_ptr->horUncEllipseSemiMajor_valid &&
            location_report_ptr->horUncEllipseSemiMinor_valid &&
            location_report_ptr->horUncEllipseOrientAzimuth_valid)
        {
            double cosVal = cos((double)locationExtended.horUncEllipseOrientAzimuth * M_PI / 180.0);
            double sinVal = sin((double)locationExtended.horUncEllipseOrientAzimuth * M_PI / 180.0);
            double major = locationExtended.horUncEllipseSemiMajor;
            double minor = locationExtended.horUncEllipseSemiMinor;

            double northSquare = major*major * cosVal*cosVal + minor*minor * sinVal*sinVal;
            double eastSquare =  major*major * sinVal*sinVal + minor*minor * cosVal*cosVal;

            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_NORTH_STD_DEV;
            locationExtended.northStdDeviation = sqrt(northSquare);

            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_EAST_STD_DEV;
            locationExtended.eastStdDeviation  = sqrt(eastSquare);
        }

        bool updateMeaAvailForPVTArray = !unpropagatedPosition &&
                mQmiMask & QMI_LOC_EVENT_MASK_GNSS_MEASUREMENT_REPORT_V02 &&
                isTOAValid(location_report_ptr, &m1HzMeasurementsInfo);

        if (((location_report_ptr->expandedGnssSvUsedList_valid) &&
                (location_report_ptr->expandedGnssSvUsedList_len != 0)) ||
                ((location_report_ptr->gnssSvUsedList_valid) &&
                (location_report_ptr->gnssSvUsedList_len != 0)))
        {
            uint32_t idx=0;
            uint32_t gnssSvUsedList_len = 0;
            uint16_t gnssSvIdUsed = 0;
            const uint16_t *svUsedList;
            bool multiBandTypesAvailable = false;

            if (location_report_ptr->expandedGnssSvUsedList_valid)
            {
                gnssSvUsedList_len = location_report_ptr->expandedGnssSvUsedList_len;
                svUsedList = location_report_ptr->expandedGnssSvUsedList;
            } else if (location_report_ptr->gnssSvUsedList_valid)
            {
                gnssSvUsedList_len = location_report_ptr->gnssSvUsedList_len;
                svUsedList = location_report_ptr->gnssSvUsedList;
            }

            // If multifreq data is not available then default to L1 for all constellations.
            if ((location_report_ptr->gnssSvUsedSignalTypeList_valid) &&
                    (location_report_ptr->gnssSvUsedSignalTypeList_len != 0)) {
                multiBandTypesAvailable = true;
            }

            LOC_LOGa("sv length %d ", gnssSvUsedList_len);

            locationExtended.numOfMeasReceived = gnssSvUsedList_len;
            memset(locationExtended.measUsageInfo, 0, sizeof(locationExtended.measUsageInfo));
            // Set of used_in_fix SV ID
            bool reported = LocApiBase::needReport(location,
                                                   (eQMI_LOC_SESS_STATUS_IN_PROGRESS_V02 ==
                                                   location_report_ptr->sessionStatus ?
                                                   LOC_SESS_INTERMEDIATE : LOC_SESS_SUCCESS),
                                                   locationExtended.tech_mask);

            if (unpropagatedPosition || reported)
            {
                locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_GNSS_SV_USED_DATA;
                if (multiBandTypesAvailable) {
                     locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_MULTIBAND;
                 }
            }

            for (idx = 0; idx < gnssSvUsedList_len; idx++)
            {
                gnssSvIdUsed = svUsedList[idx];
                qmiLocGnssSignalTypeMaskT_v02 qmiGnssSignalType =
                        location_report_ptr->gnssSvUsedSignalTypeList[idx];
                GnssSignalTypeMask gnssSignalTypeMask =
                        convertQmiGnssSignalType(qmiGnssSignalType);

                if (unpropagatedPosition || reported)
                {
                    locationExtended.measUsageInfo[idx].gnssSvId = gnssSvIdUsed;
                    locationExtended.measUsageInfo[idx].carrierPhaseAmbiguityType =
                            CARRIER_PHASE_AMBIGUITY_RESOLUTION_NONE;
                    locationExtended.measUsageInfo[idx].measUsageStatusMask =
                            GNSS_MEAS_USED_IN_PVT;

                    if (gnssSvIdUsed <= GPS_SV_PRN_MAX)
                    {
                        uint64_t bit = (1ULL << (gnssSvIdUsed - GPS_SV_PRN_MIN));
                        locationExtended.gnss_sv_used_ids.gps_sv_used_ids_mask |= bit;
                        locationExtended.measUsageInfo[idx].gnssConstellation =
                                GNSS_LOC_SV_SYSTEM_GPS;
                        if (multiBandTypesAvailable) {
                            locationExtended.measUsageInfo[idx].gnssSignalType =
                                    gnssSignalTypeMask;
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_GPS_L1CA) {
                                locationExtended.gnss_mb_sv_used_ids.gps_l1ca_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_GPS_L1C) {
                                locationExtended.gnss_mb_sv_used_ids.gps_l1c_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_GPS_L2) {
                                locationExtended.gnss_mb_sv_used_ids.gps_l2_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_GPS_L5) {
                                locationExtended.gnss_mb_sv_used_ids.gps_l5_sv_used_ids_mask
                                        |= bit;
                            }
                        } else {
                            locationExtended.measUsageInfo[idx].gnssSignalType =
                                    GNSS_SIGNAL_GPS_L1CA;
                        }
                    }
                    else if ((gnssSvIdUsed >= GLO_SV_PRN_MIN) &&
                             (gnssSvIdUsed <= GLO_SV_PRN_MAX))
                    {
                        uint64_t bit = (1ULL << (gnssSvIdUsed - GLO_SV_PRN_MIN));
                        locationExtended.gnss_sv_used_ids.glo_sv_used_ids_mask |= bit;
                        locationExtended.measUsageInfo[idx].gnssConstellation =
                                GNSS_LOC_SV_SYSTEM_GLONASS;
                        if (multiBandTypesAvailable) {
                            locationExtended.measUsageInfo[idx].gnssSignalType =
                                    gnssSignalTypeMask;

                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_GLONASS_G1) {
                                locationExtended.gnss_mb_sv_used_ids.glo_g1_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_GLONASS_G2) {
                                locationExtended.gnss_mb_sv_used_ids.glo_g2_sv_used_ids_mask
                                        |= bit;
                            }
                        } else {
                            locationExtended.measUsageInfo[idx].gnssSignalType =
                                    GNSS_SIGNAL_GLONASS_G1;
                        }
                    }
                    else if ((gnssSvIdUsed >= BDS_SV_PRN_MIN) &&
                             (gnssSvIdUsed <= BDS_SV_PRN_MAX))
                    {
                        uint64_t bit = (1ULL << (gnssSvIdUsed - BDS_SV_PRN_MIN));
                        locationExtended.gnss_sv_used_ids.bds_sv_used_ids_mask |= bit;
                        locationExtended.measUsageInfo[idx].gnssConstellation =
                                GNSS_LOC_SV_SYSTEM_BDS;
                        if (multiBandTypesAvailable) {
                            locationExtended.measUsageInfo[idx].gnssSignalType =
                                    gnssSignalTypeMask;
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_BEIDOU_B1I) {
                                locationExtended.gnss_mb_sv_used_ids.bds_b1i_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_BEIDOU_B1C) {
                                locationExtended.gnss_mb_sv_used_ids.bds_b1c_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_BEIDOU_B2I) {
                                locationExtended.gnss_mb_sv_used_ids.bds_b2i_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_BEIDOU_B2AI) {
                                locationExtended.gnss_mb_sv_used_ids.bds_b2ai_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_BEIDOU_B2AQ) {
                                locationExtended.gnss_mb_sv_used_ids.bds_b2aq_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_BEIDOU_B2BI) {
                                locationExtended.gnss_mb_sv_used_ids.bds_b2bi_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_BEIDOU_B2BQ) {
                                locationExtended.gnss_mb_sv_used_ids.bds_b2bq_sv_used_ids_mask
                                        |= bit;
                            }
                        } else {
                            locationExtended.measUsageInfo[idx].gnssSignalType =
                                    GNSS_SIGNAL_BEIDOU_B1I;
                        }
                    }
                    else if ((gnssSvIdUsed >= GAL_SV_PRN_MIN) &&
                             (gnssSvIdUsed <= GAL_SV_PRN_MAX))
                    {
                        uint64_t bit = (1ULL << (gnssSvIdUsed - GAL_SV_PRN_MIN));
                        locationExtended.gnss_sv_used_ids.gal_sv_used_ids_mask |= bit;
                        locationExtended.measUsageInfo[idx].gnssConstellation =
                                GNSS_LOC_SV_SYSTEM_GALILEO;
                        if (multiBandTypesAvailable) {
                            locationExtended.measUsageInfo[idx].gnssSignalType =
                                    gnssSignalTypeMask;

                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_GALILEO_E1) {
                                locationExtended.gnss_mb_sv_used_ids.gal_e1_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_GALILEO_E5A) {
                                locationExtended.gnss_mb_sv_used_ids.gal_e5a_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_GALILEO_E5B) {
                                locationExtended.gnss_mb_sv_used_ids.gal_e5b_sv_used_ids_mask
                                        |= bit;
                            }
                        } else {
                            locationExtended.measUsageInfo[idx].gnssSignalType =
                                    GNSS_SIGNAL_GALILEO_E1;
                        }
                    }
                    else if ((gnssSvIdUsed >= QZSS_SV_PRN_MIN) &&
                             (gnssSvIdUsed <= QZSS_SV_PRN_MAX))
                    {
                        uint64_t bit = (1ULL << (gnssSvIdUsed - QZSS_SV_PRN_MIN));
                        locationExtended.gnss_sv_used_ids.qzss_sv_used_ids_mask |= bit;
                        locationExtended.measUsageInfo[idx].gnssConstellation =
                                GNSS_LOC_SV_SYSTEM_QZSS;
                        if (multiBandTypesAvailable) {
                            locationExtended.measUsageInfo[idx].gnssSignalType =
                                    gnssSignalTypeMask;

                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_QZSS_L1CA) {
                                locationExtended.gnss_mb_sv_used_ids.qzss_l1ca_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_QZSS_L1S) {
                                locationExtended.gnss_mb_sv_used_ids.qzss_l1s_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_QZSS_L2) {
                                locationExtended.gnss_mb_sv_used_ids.qzss_l2_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_QZSS_L5) {
                                locationExtended.gnss_mb_sv_used_ids.qzss_l5_sv_used_ids_mask
                                        |= bit;
                            }
                        } else {
                                locationExtended.measUsageInfo[idx].gnssSignalType =
                                        GNSS_SIGNAL_QZSS_L1CA;
                        }
                    }
                    else if ((gnssSvIdUsed >= NAVIC_SV_PRN_MIN) &&
                             (gnssSvIdUsed <= NAVIC_SV_PRN_MAX))
                    {
                        uint64_t bit = (1ULL << (gnssSvIdUsed - NAVIC_SV_PRN_MIN));
                        locationExtended.gnss_sv_used_ids.navic_sv_used_ids_mask |= bit;
                        locationExtended.measUsageInfo[idx].gnssConstellation =
                                GNSS_LOC_SV_SYSTEM_NAVIC;

                        locationExtended.measUsageInfo[idx].gnssSignalType = GNSS_SIGNAL_NAVIC_L5;
                        if (multiBandTypesAvailable) {
                            locationExtended.measUsageInfo[idx].gnssSignalType =
                                    gnssSignalTypeMask;

                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_NAVIC_L5) {
                                locationExtended.gnss_mb_sv_used_ids.navic_l5_sv_used_ids_mask
                                        |= bit;
                            }
                            if (locationExtended.measUsageInfo[idx].gnssSignalType &
                                    GNSS_SIGNAL_NAVIC_L1) {
                                locationExtended.gnss_mb_sv_used_ids.navic_l1_sv_used_ids_mask
                                        |= bit;
                            }
                        }
                    }
                }

                // For each used SV in propagated final positions, check if it
                // has measuremnt with QMI_LOC_MASK_MEAS_STATUS_GNSS_FRESH_MEAS_VALID
                if (updateMeaAvailForPVTArray &&
                        isMeasurementRefreshForSv(gnssSvIdUsed, gnssSignalTypeMask))
                {
                    if (gnssSvIdUsed <= GPS_SV_PRN_MAX)
                    {
                        meaAvailForPVT[eQMI_LOC_SV_SYSTEM_GPS_V02 - 1]++;
                    }
                    else if ((gnssSvIdUsed >= GLO_SV_PRN_MIN) &&
                             (gnssSvIdUsed <= GLO_SV_PRN_MAX))
                    {
                        meaAvailForPVT[eQMI_LOC_SV_SYSTEM_GLONASS_V02 - 1]++;
                    }
                    else if ((gnssSvIdUsed >= BDS_SV_PRN_MIN) &&
                             (gnssSvIdUsed <= BDS_SV_PRN_MAX))
                    {
                        meaAvailForPVT[eQMI_LOC_SV_SYSTEM_BDS_V02 - 1]++;
                    }
                    else if ((gnssSvIdUsed >= GAL_SV_PRN_MIN) &&
                             (gnssSvIdUsed <= GAL_SV_PRN_MAX))
                    {
                        meaAvailForPVT[eQMI_LOC_SV_SYSTEM_GALILEO_V02 - 1]++;
                    }
                    else if ((gnssSvIdUsed >= QZSS_SV_PRN_MIN) &&
                             (gnssSvIdUsed <= QZSS_SV_PRN_MAX))
                    {
                        meaAvailForPVT[eQMI_LOC_SV_SYSTEM_QZSS_V02 - 1]++;
                    }
                    else if ((gnssSvIdUsed >= NAVIC_SV_PRN_MIN) &&
                             (gnssSvIdUsed <= NAVIC_SV_PRN_MAX))
                    {
                        meaAvailForPVT[eQMI_LOC_SV_SYSTEM_NAVIC_V02 - 1]++;
                    }
                }
            }
        }

        if (updateMeaAvailForPVTArray) {
            // Check Each satellite refreshed meaurement amount for propagated final positions,
            // If not enough satellites are used for the position calculation,
            // set PROPAGATED to indicate a tunnel entry situation
            bool lowSatelliteVisibility =
                (meaAvailForPVT[eQMI_LOC_SV_SYSTEM_GPS_V02 - 1] < MIN_REFRESH_MEASUREMENTS) &&
                (meaAvailForPVT[eQMI_LOC_SV_SYSTEM_GALILEO_V02 - 1] < MIN_REFRESH_MEASUREMENTS) &&
                (meaAvailForPVT[eQMI_LOC_SV_SYSTEM_GLONASS_V02 - 1] < MIN_REFRESH_MEASUREMENTS) &&
                (meaAvailForPVT[eQMI_LOC_SV_SYSTEM_BDS_V02 - 1] < MIN_REFRESH_MEASUREMENTS) &&
                (meaAvailForPVT[eQMI_LOC_SV_SYSTEM_QZSS_V02 - 1] < MIN_REFRESH_MEASUREMENTS) &&
                (meaAvailForPVT[eQMI_LOC_SV_SYSTEM_NAVIC_V02 - 1] < MIN_REFRESH_MEASUREMENTS);

            LOC_LOGa("GPS %d, Gal %d, Glo %d, BDS %d, QZS %d, NAV %d, lowSatVisibility %d",
                    meaAvailForPVT[eQMI_LOC_SV_SYSTEM_GPS_V02 - 1],
                    meaAvailForPVT[eQMI_LOC_SV_SYSTEM_GALILEO_V02 - 1],
                    meaAvailForPVT[eQMI_LOC_SV_SYSTEM_GLONASS_V02 - 1],
                    meaAvailForPVT[eQMI_LOC_SV_SYSTEM_BDS_V02 - 1],
                    meaAvailForPVT[eQMI_LOC_SV_SYSTEM_QZSS_V02 - 1],
                    meaAvailForPVT[eQMI_LOC_SV_SYSTEM_NAVIC_V02 - 1],
                    lowSatelliteVisibility);

            if (lowSatelliteVisibility) {
                locationExtended.tech_mask |= LOC_POS_TECH_MASK_PROPAGATED;
            }
        }

        if (location_report_ptr->navSolutionMask_valid)
        {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_NAV_SOLUTION_MASK;
            locationExtended.navSolutionMask =
                convertNavSolutionMask(location_report_ptr->navSolutionMask);
        }

        if (location_report_ptr->gpsTime_valid)
        {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_GPS_TIME;
            locationExtended.gpsTime.gpsWeek = location_report_ptr->gpsTime.gpsWeek;
            locationExtended.gpsTime.gpsTimeOfWeekMs = location_report_ptr->gpsTime.gpsTimeOfWeekMs;
        }

        if (location_report_ptr->extDOP_valid)
        {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_EXT_DOP;
            locationExtended.extDOP.PDOP = location_report_ptr->extDOP.PDOP;
            locationExtended.extDOP.HDOP = location_report_ptr->extDOP.HDOP;
            locationExtended.extDOP.VDOP = location_report_ptr->extDOP.VDOP;
            locationExtended.extDOP.GDOP = location_report_ptr->extDOP.GDOP;
            locationExtended.extDOP.TDOP = location_report_ptr->extDOP.TDOP;
        }

        if (location_report_ptr->velEnu_valid)
        {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_EAST_VEL;
            locationExtended.eastVelocity = location_report_ptr->velEnu[0];
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_NORTH_VEL;
            locationExtended.northVelocity = location_report_ptr->velEnu[1];
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_UP_VEL;
            locationExtended.upVelocity = location_report_ptr->velEnu[2];
        }

        if (location_report_ptr->velUncEnu_valid)
        {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_EAST_VEL_UNC;
            locationExtended.eastVelocityStdDeviation = location_report_ptr->velUncEnu[0];
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_NORTH_VEL_UNC;
            locationExtended.northVelocityStdDeviation = location_report_ptr->velUncEnu[1];
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_UP_VEL_UNC;
            locationExtended.upVelocityStdDeviation = location_report_ptr->velUncEnu[2];
        }
        // fill in GnssSystemTime based on gps timestamp and time uncertainty
        locationExtended.gnssSystemTime.gnssSystemTimeSrc = (Gnss_LocSvSystemEnumType)0;
        if (location_report_ptr->gpsTime_valid)
        {
            locationExtended.gnssSystemTime.gnssSystemTimeSrc = GNSS_LOC_SV_SYSTEM_GPS;
            locationExtended.gnssSystemTime.u.gpsSystemTime.validityMask = 0x0;

            locationExtended.gnssSystemTime.u.gpsSystemTime.systemWeek =
                    locationExtended.gpsTime.gpsWeek;
            locationExtended.gnssSystemTime.u.gpsSystemTime.validityMask |=
                    GNSS_SYSTEM_TIME_WEEK_VALID;

            locationExtended.gnssSystemTime.u.gpsSystemTime.systemMsec =
                    locationExtended.gpsTime.gpsTimeOfWeekMs;
            locationExtended.gnssSystemTime.u.gpsSystemTime.validityMask |=
                    GNSS_SYSTEM_TIME_WEEK_MS_VALID;

            if (location_report_ptr->systemClkTimeBias_valid) {
                locationExtended.gnssSystemTime.u.gpsSystemTime.systemClkTimeBias =
                        location_report_ptr->systemClkTimeBias;
                locationExtended.gnssSystemTime.u.gpsSystemTime.validityMask |=
                        GNSS_SYSTEM_CLK_TIME_BIAS_VALID;
            } else {
                locationExtended.gnssSystemTime.u.gpsSystemTime.systemClkTimeBias = 0.0f;
                locationExtended.gnssSystemTime.u.gpsSystemTime.validityMask |=
                        GNSS_SYSTEM_CLK_TIME_BIAS_VALID;
            }

            if (location_report_ptr->systemClkTimeBiasUnc_valid) {
                locationExtended.gnssSystemTime.u.gpsSystemTime.systemClkTimeUncMs =
                        location_report_ptr->systemClkTimeBiasUnc;
                locationExtended.gnssSystemTime.u.gpsSystemTime.validityMask |=
                        GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID;
            } else if (location_report_ptr->timeUnc_valid) {
                locationExtended.gnssSystemTime.u.gpsSystemTime.systemClkTimeUncMs =
                        location_report_ptr->timeUnc;
                locationExtended.gnssSystemTime.u.gpsSystemTime.validityMask |=
                        GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID;
            }
        }

        if (location_report_ptr->timeUnc_valid)
        {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_TIME_UNC;
            locationExtended.timeUncMs = location_report_ptr->timeUnc;
        }

        if (location_report_ptr->leapSeconds_valid)
        {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_LEAP_SECONDS;
            locationExtended.leapSeconds = location_report_ptr->leapSeconds;
        }

        if (location_report_ptr->dgnssCorrectionSource_valid) {
            locationExtended.flags |=
                    GPS_LOCATION_EXTENDED_HAS_DGNSS_CORRECTION_SOURCE_TYPE;
            locationExtended.dgnssCorrectionSourceType = (LocDgnssCorrectionSourceType)
                    location_report_ptr->dgnssCorrectionSource;
        }

        if (location_report_ptr->dgnssCorrectionSourceID_valid) {
            locationExtended.flags |=
                    GPS_LOCATION_EXTENDED_HAS_DGNSS_CORRECTION_SOURCE_ID;
            locationExtended.dgnssCorrectionSourceID =
                    location_report_ptr->dgnssCorrectionSourceID;
        }

        if (location_report_ptr->dgnssConstellationUsage_valid) {
            locationExtended.flags |=
                    GPS_LOCATION_EXTENDED_HAS_DGNSS_CONSTELLATION_USAGE;
            convertGnssConestellationMask(location_report_ptr->dgnssConstellationUsage,
                                          locationExtended.dgnssConstellationUsage);
        }

        if (location_report_ptr->dgnssRefStationId_valid) {
            locationExtended.flags |=
                    GPS_LOCATION_EXTENDED_HAS_DGNSS_REF_STATION_ID;
            locationExtended.dgnssRefStationId =
                    location_report_ptr->dgnssRefStationId;
        }

        if (location_report_ptr->dgnssDataAgeMsec_valid) {
            locationExtended.flags |=
                    GPS_LOCATION_EXTENDED_HAS_DGNSS_DATA_AGE;
            locationExtended.dgnssDataAgeMsec =
                    location_report_ptr->dgnssDataAgeMsec;
        }

        if (location_report_ptr->dgnssStationId_valid) {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_DGNSS_STATION_ID;
            uint32_t cnt = location_report_ptr->dgnssStationId_len;
            uint32_t i = 0;
            for (i = 0; i < cnt && i < DGNSS_STATION_ID_MAX; i++) {
                locationExtended.dgnssStationId[i] = location_report_ptr->dgnssStationId[i];
            }
            locationExtended.numOfDgnssStationId = i;
        } else {
            LOC_LOGa("no dgnss station id");
        }

        if (location_report_ptr->systemTick_valid &&
                location_report_ptr->systemTickUnc_valid) {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_SYSTEM_TICK;
            locationExtended.systemTick = location_report_ptr->systemTick;
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_SYSTEM_TICK_UNC;
            locationExtended.systemTickUnc = location_report_ptr->systemTickUnc;
        }
        if (location_report_ptr->leapSecUnc_valid) {
            locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_LEAP_SECONDS_UNC;
            if (location_report_ptr->leapSecUnc == LOC_LEAP_SEC_UNC_GAURD_VALUE &&
                    location_report_ptr->timestampUtc_valid) {
                locationExtended.leapSecondsUnc = 0;
            } else {
                locationExtended.leapSecondsUnc = location_report_ptr->leapSecUnc;
            }
        }

        loc_sess_status sessStatus =
                (location_report_ptr->sessionStatus == eQMI_LOC_SESS_STATUS_IN_PROGRESS_V02) ?
                LOC_SESS_INTERMEDIATE : LOC_SESS_SUCCESS;

        // if for some reason, modem did not set valid lat/lon/accuracy,
        // mark session status as failure
        if ((location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_LAT_LONG) == 0 ||
                (location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_ACCURACY) == 0 ||
                (location.gpsLocation.accuracy == 0.0f)) {
            LOC_LOGe("fix of sess status %d but with invalid info, valid flags 0x%x, "
                     "lat: %f, lon: %f, accuracy %f",
                     sessStatus, location.gpsLocation.flags, location.gpsLocation.latitude,
                     location.gpsLocation.longitude, location.gpsLocation.accuracy);
            sessStatus = LOC_SESS_FAILURE;
        }

        LocApiBase::reportPosition(location,
                                   locationExtended,
                                   sessStatus,
                                   locationExtended.tech_mask, &dataNotify);
        // < SEC_GPS
        if (sec_gps_conf.NMEA_ALLOWED) {
            sendPSMSG();
        }
        // SEC_GPS >
    }
    else
    {
        LocApiBase::reportPosition(location,
                                   locationExtended,
                                   LOC_SESS_FAILURE,
                                   LOC_POS_TECH_MASK_DEFAULT,
                                   &dataNotify);
    }
}

/*convert signal type to carrier frequency*/
double LocApiV02::convertSignalTypeToCarrierFrequency(
    qmiLocGnssSignalTypeMaskT_v02 signalType,
    uint8_t gloFrequency)
{
    double carrierFrequency = 0.0;

    switch (signalType) {
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L1CA_V02:
        carrierFrequency = GPS_L1CA_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L1C_V02:
        carrierFrequency = GPS_L1C_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L2C_L_V02:
        carrierFrequency = GPS_L2C_L_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L5_Q_V02:
        carrierFrequency = GPS_L5_Q_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GLONASS_G1_V02:
        carrierFrequency = GLONASS_G1_CARRIER_FREQUENCY;
        if ((gloFrequency >= 1 && gloFrequency <= 14)) {
            carrierFrequency += ((gloFrequency - 8) * 562500.0);
        }
        LOC_LOGa("GLO carFreq after conversion = %f", carrierFrequency);
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GLONASS_G2_V02:
        carrierFrequency = GLONASS_G2_CARRIER_FREQUENCY;
        if ((gloFrequency >= 1 && gloFrequency <= 14)) {
            carrierFrequency += ((gloFrequency - 8) * 437500.0);
        }
        LOC_LOGa("GLO carFreq after conversion = %f", carrierFrequency);
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E1_C_V02:
        carrierFrequency = GALILEO_E1_C_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E5A_Q_V02:
        carrierFrequency = GALILEO_E5A_Q_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E5B_Q_V02:
        carrierFrequency = GALILEO_E5B_Q_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B1_I_V02:
        carrierFrequency = BEIDOU_B1_I_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B1C_V02:
        carrierFrequency = BEIDOU_B1C_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2_I_V02:
        carrierFrequency = BEIDOU_B2_I_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2A_I_V02:
        carrierFrequency = BEIDOU_B2A_I_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2B_I_V02:
        carrierFrequency = BEIDOU_B2B_I_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2B_Q_V02:
        carrierFrequency = BEIDOU_B2B_Q_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L1CA_V02:
        carrierFrequency = QZSS_L1CA_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L1S_V02:
        carrierFrequency = QZSS_L1S_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L2C_L_V02:
        carrierFrequency = QZSS_L2C_L_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L5_Q_V02:
        carrierFrequency = QZSS_L5_Q_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_SBAS_L1_CA_V02:
        carrierFrequency = SBAS_L1_CA_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2A_Q_V02:
        carrierFrequency = BEIDOU_B2A_Q_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_NAVIC_L5_V02:
        carrierFrequency = NAVIC_L5_CARRIER_FREQUENCY;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_NAVIC_L1_V02:
        carrierFrequency = NAVIC_L1_CARRIER_FREQUENCY;
        break;

    default:
        break;
    }
    return carrierFrequency;
}

static GnssSignalTypeMask getDefaultGnssSignalTypeMask(qmiLocSvSystemEnumT_v02 qmiSvSystemType)
{
    GnssSignalTypeMask gnssSignalType = 0;

    switch (qmiSvSystemType) {
    case eQMI_LOC_SV_SYSTEM_GPS_V02:
        gnssSignalType = GNSS_SIGNAL_GPS_L1CA;
        break;
    case eQMI_LOC_SV_SYSTEM_GALILEO_V02:
        gnssSignalType = GNSS_SIGNAL_GALILEO_E1;
        break;
    case eQMI_LOC_SV_SYSTEM_SBAS_V02:
        gnssSignalType = GNSS_SIGNAL_SBAS_L1;
        break;
    case eQMI_LOC_SV_SYSTEM_GLONASS_V02:
        gnssSignalType = GNSS_SIGNAL_GLONASS_G1;
        break;
    case eQMI_LOC_SV_SYSTEM_BDS_V02:
    case eQMI_LOC_SV_SYSTEM_COMPASS_V02:
        gnssSignalType = GNSS_SIGNAL_BEIDOU_B1I;
        break;
    case eQMI_LOC_SV_SYSTEM_QZSS_V02:
        gnssSignalType = GNSS_SIGNAL_QZSS_L1CA;
        break;
    default:
        break;
    }

    return gnssSignalType;
}

/* convert satellite report to location api format and send the converted
   report to base */
void  LocApiV02 :: reportSv (
    const qmiLocEventGnssSvInfoIndMsgT_v02 *gnss_report_ptr)
{
    GnssSvNotification SvNotify = {};
    int              num_svs_max, i;
    const qmiLocSvInfoStructT_v02 *sv_info_ptr;
    uint8_t gloFrequency = 0;

    num_svs_max = 0;
    if (1 == gnss_report_ptr->expandedSvList_valid) {
        num_svs_max = std::min((uint32_t)QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
                                gnss_report_ptr->expandedSvList_len);
    }
    else if (1 == gnss_report_ptr->svList_valid) {
        num_svs_max = std::min((uint32_t)QMI_LOC_MAX_SV_USED_LIST_LENGTH_V02,
                                gnss_report_ptr->svList_len);
    }
    num_svs_max = std::min(num_svs_max, GNSS_SV_MAX);

    SvNotify.size = sizeof(GnssSvNotification);
    if (gnss_report_ptr->gnssSignalTypeList_valid) {
        SvNotify.gnssSignalTypeMaskValid = true;
    }

    if (1 == gnss_report_ptr->svList_valid ||
        1 == gnss_report_ptr->expandedSvList_valid) {
        SvNotify.count = 0;
        if (0 == gnss_report_ptr->expandedSvList_valid ||
            0 == gnss_report_ptr->rfLoss_valid) {
            /*  For modems that don's send rfLoss in the QMI LOC message
                we need to read this info from gps.conf, this is done
                in the constructor */
            for (int i = RF_LOSS_GPS_CONF; i < RF_LOSS_MAX_CONF; i++) {
                if (0 == rfLossNV[i]) {
                    LOC_LOGw("At least one RF_LOSS is 0 in gps.conf, please configure it");
                    break;
                }
            }
        }
        for(i = 0; i < num_svs_max; i++) {
            if (1 == gnss_report_ptr->expandedSvList_valid) {
                sv_info_ptr = &(gnss_report_ptr->expandedSvList[i].svInfo);
            }
            else {
                sv_info_ptr = &(gnss_report_ptr->svList[i]);
            }
            if((sv_info_ptr->validMask & QMI_LOC_SV_INFO_MASK_VALID_SYSTEM_V02) &&
               (sv_info_ptr->validMask & QMI_LOC_SV_INFO_MASK_VALID_GNSS_SVID_V02)
                && (sv_info_ptr->gnssSvId != 0 ))
            {
                GnssSvOptionsMask mask = 0;

                GnssSv &gnssSv_ref = SvNotify.gnssSvs[SvNotify.count];
                bool bSvIdIsValid = false;

                gnssSv_ref.size = sizeof(GnssSv);
                gnssSv_ref.svId = sv_info_ptr->gnssSvId;
                if (1 == gnss_report_ptr->expandedSvList_valid) {
                    gnssSv_ref.gloFrequency = gnss_report_ptr->expandedSvList[i].gloFrequency;
                }

                switch (sv_info_ptr->system) {
                case eQMI_LOC_SV_SYSTEM_GPS_V02:
                    gnssSv_ref.type = GNSS_SV_TYPE_GPS;
                    bSvIdIsValid = isValInRangeInclusive(gnssSv_ref.svId,
                                                         GPS_SV_PRN_MIN, GPS_SV_PRN_MAX);
                    break;

                case eQMI_LOC_SV_SYSTEM_GALILEO_V02:
                    gnssSv_ref.type = GNSS_SV_TYPE_GALILEO;
                    bSvIdIsValid = isValInRangeInclusive(gnssSv_ref.svId,
                                                         GAL_SV_PRN_MIN, GAL_SV_PRN_MAX);
                    break;

                case eQMI_LOC_SV_SYSTEM_SBAS_V02:
                    gnssSv_ref.type = GNSS_SV_TYPE_SBAS;
                    bSvIdIsValid = isValInRangeInclusive(gnssSv_ref.svId,
                                                         SBAS_SV_PRN_MIN, SBAS_SV_PRN_MAX);
                    break;

                // Glonass in SV report comes in range of [1, 32] or 255,
                // convert to [65, 96] or 255
                case eQMI_LOC_SV_SYSTEM_GLONASS_V02:
                    gnssSv_ref.type = GNSS_SV_TYPE_GLONASS;
                    if (!isGloSlotUnknown(sv_info_ptr->gnssSvId)) {
                        gnssSv_ref.svId = sv_info_ptr->gnssSvId + GLO_SV_PRN_MIN - 1;
                    }
                    bSvIdIsValid = isValInRangeInclusive(gnssSv_ref.svId, GLO_SV_PRN_MIN,
                                                         GLO_SV_PRN_MAX) ||
                                   isGloSlotUnknown(gnssSv_ref.svId);
                    break;

                case eQMI_LOC_SV_SYSTEM_BDS_V02:
                case eQMI_LOC_SV_SYSTEM_COMPASS_V02:
                    gnssSv_ref.type = GNSS_SV_TYPE_BEIDOU;
                    bSvIdIsValid = isValInRangeInclusive(gnssSv_ref.svId,
                                                         BDS_SV_PRN_MIN, BDS_SV_PRN_MAX);
                    break;

                case eQMI_LOC_SV_SYSTEM_QZSS_V02:
                    gnssSv_ref.type = GNSS_SV_TYPE_QZSS;
                    bSvIdIsValid = isValInRangeInclusive(gnssSv_ref.svId,
                                                         QZSS_SV_PRN_MIN, QZSS_SV_PRN_MAX);
                    break;

                case eQMI_LOC_SV_SYSTEM_NAVIC_V02:
                    gnssSv_ref.type = GNSS_SV_TYPE_NAVIC;
                    bSvIdIsValid = isValInRangeInclusive(gnssSv_ref.svId,
                                                         NAVIC_SV_PRN_MIN, NAVIC_SV_PRN_MAX);
                    break;

                default:
                    gnssSv_ref.type = GNSS_SV_TYPE_UNKNOWN;
                    break;
                }
                if (!bSvIdIsValid) {
                    memset(&gnssSv_ref, 0, sizeof(gnssSv_ref));
                    continue;
                }

                if (sv_info_ptr->validMask & QMI_LOC_SV_INFO_MASK_VALID_ELEVATION_V02) {
                    gnssSv_ref.elevation = sv_info_ptr->elevation;
                    mask |= GNSS_SV_OPTIONS_HAS_ELEVATION_BIT;
                }

                if (sv_info_ptr->validMask & QMI_LOC_SV_INFO_MASK_VALID_AZIMUTH_V02) {
                    gnssSv_ref.azimuth = sv_info_ptr->azimuth;
                    mask |= GNSS_SV_OPTIONS_HAS_AZIMUTH_BIT;
                }

                if (sv_info_ptr->validMask &
                    QMI_LOC_SV_INFO_MASK_VALID_SVINFO_MASK_V02) {
                    if (sv_info_ptr->svInfoMask &
                        QMI_LOC_SVINFO_MASK_HAS_EPHEMERIS_V02) {
                        mask |= GNSS_SV_OPTIONS_HAS_EPHEMER_BIT;
                    }
                    if (sv_info_ptr->svInfoMask &
                        QMI_LOC_SVINFO_MASK_HAS_ALMANAC_V02) {
                        mask |= GNSS_SV_OPTIONS_HAS_ALMANAC_BIT;
                    }
                }

                mask |= GNSS_SV_OPTIONS_HAS_GNSS_SIGNAL_TYPE_BIT;
                if (gnss_report_ptr->gnssSignalTypeList_valid) {
                    if (SvNotify.count > gnss_report_ptr->gnssSignalTypeList_len - 1) {
                        LOC_LOGw("Frequency not available for this SV");
                    }
                    else {
                        if (1 == gnss_report_ptr->expandedSvList_valid) {
                            gloFrequency = gnss_report_ptr->expandedSvList[i].gloFrequency;
                        }

                        if (gnss_report_ptr->gnssSignalTypeList[i] != 0) {
                            gnssSv_ref.carrierFrequencyHz =
                                    convertSignalTypeToCarrierFrequency(
                                        gnss_report_ptr->gnssSignalTypeList[i],
                                        gloFrequency);
                            mask |= GNSS_SV_OPTIONS_HAS_CARRIER_FREQUENCY_BIT;
                            gnssSv_ref.gnssSignalTypeMask = convertQmiGnssSignalType(
                                    gnss_report_ptr->gnssSignalTypeList[i]);
                        }
                    }
                } else {
                    gnssSv_ref.gnssSignalTypeMask =
                            getDefaultGnssSignalTypeMask(sv_info_ptr->system);
                    mask |= GNSS_SV_OPTIONS_HAS_CARRIER_FREQUENCY_BIT;
                    gnssSv_ref.carrierFrequencyHz = CarrierFrequencies[gnssSv_ref.type];
                }

                if (sv_info_ptr->validMask & QMI_LOC_SV_INFO_MASK_VALID_SNR_V02) {
                    gnssSv_ref.cN0Dbhz = sv_info_ptr->snr;
                    double rfLoss = gnss_report_ptr->rfLoss[i];
                    if ((1 != gnss_report_ptr->expandedSvList_valid) ||
                        (1 != gnss_report_ptr->rfLoss_valid)) {
                        switch (gnssSv_ref.gnssSignalTypeMask) {
                        case GNSS_SIGNAL_GPS_L1CA:
                        case GNSS_SIGNAL_GPS_L1C:
                        case GNSS_SIGNAL_QZSS_L1CA:
                            rfLoss = rfLossNV[RF_LOSS_GPS_CONF]/10.0;
                            break;
                        case GNSS_SIGNAL_GPS_L5:
                        case GNSS_SIGNAL_QZSS_L5:
                            rfLoss = rfLossNV[RF_LOSS_GPS_L5_CONF]/10.0;
                            break;
                        case GNSS_SIGNAL_BEIDOU_B1I:
                            rfLoss = rfLossNV[RF_LOSS_BDS_CONF]/10.0;
                            break;
                        case GNSS_SIGNAL_BEIDOU_B2AI:
                            rfLoss = rfLossNV[RF_LOSS_BDS_B2A_CONF]/10.0;
                            break;
                        case GNSS_SIGNAL_BEIDOU_B2BI:
                            rfLoss = rfLossNV[RF_LOSS_BDS_B2B_CONF]/10.0;
                            break;
                        case GNSS_SIGNAL_GLONASS_G1:
                        case GNSS_SIGNAL_GLONASS_G2:
                            {
                                /* compute rfLoss based on rfLossNV[RF_LOSS_GLO_LEFT_CONF],
                                   rfLossNV[RF_LOSS_GLO_CENTER_CONF],
                                   rfLossNV[RF_LOSS_GLO_RIGHT_CONF] and gloFrequency */
                                LocApiProxyBase* locApiProxyObj = getLocApiProxy();
                                if (nullptr != locApiProxyObj &&
                                    ((gloFrequency >= 1 && gloFrequency <= 14))) {
                                    rfLoss = locApiProxyObj->getGloRfLoss(
                                                    rfLossNV[RF_LOSS_GLO_LEFT_CONF],
                                                    rfLossNV[RF_LOSS_GLO_CENTER_CONF],
                                                    rfLossNV[RF_LOSS_GLO_RIGHT_CONF],
                                                    gloFrequency);
                                }
                            }
                            break;
                        case GNSS_SIGNAL_GALILEO_E1:
                            rfLoss = rfLossNV[RF_LOSS_GAL_CONF]/10.0;
                            break;
                        case GNSS_SIGNAL_GALILEO_E5A:
                        case GNSS_SIGNAL_GALILEO_E5B:
                            rfLoss = rfLossNV[RF_LOSS_GAL_E5_CONF]/10.0;
                            break;
                        case GNSS_SIGNAL_NAVIC_L5:
                        case GNSS_SIGNAL_NAVIC_L1:
                            rfLoss = rfLossNV[RF_LOSS_NAVIC_CONF]/10.0;
                            break;
                        default:
                            break;
                        }
                    }
#if 1 //< SEC_GPS : Set the basebandCNo and compensate RF loss (fixed value)
                    gnssSv_ref.basebandCarrierToNoiseDbHz = gnssSv_ref.cN0Dbhz;
                    if (gnssSv_ref.cN0Dbhz > 0) {
                        gnssSv_ref.cN0Dbhz += RF_LOSS;
                    }
#else //QC original code
                    if (gnssSv_ref.cN0Dbhz > rfLoss) {
                        gnssSv_ref.basebandCarrierToNoiseDbHz = gnssSv_ref.cN0Dbhz - rfLoss;
                    }
#endif // SEC_GPS >
                }

                gnssSv_ref.gnssSvOptionsMask = mask;
                SvNotify.count++;
            }
        }
    }

    LocApiBase::reportSv(SvNotify);
}

static Gnss_LocSvSystemEnumType getLocApiSvSystemType (qmiLocSvSystemEnumT_v02 qmiSvSystemType) {
    Gnss_LocSvSystemEnumType locSvSystemType = GNSS_LOC_SV_SYSTEM_UNKNOWN;

    switch (qmiSvSystemType) {
    case eQMI_LOC_SV_SYSTEM_GPS_V02:
        locSvSystemType = GNSS_LOC_SV_SYSTEM_GPS;
        break;

    case eQMI_LOC_SV_SYSTEM_GALILEO_V02:
        locSvSystemType = GNSS_LOC_SV_SYSTEM_GALILEO;
        break;

    case eQMI_LOC_SV_SYSTEM_SBAS_V02:
        locSvSystemType = GNSS_LOC_SV_SYSTEM_SBAS;
        break;

    case eQMI_LOC_SV_SYSTEM_GLONASS_V02:
        locSvSystemType = GNSS_LOC_SV_SYSTEM_GLONASS;
        break;

    case eQMI_LOC_SV_SYSTEM_BDS_V02:
    case eQMI_LOC_SV_SYSTEM_COMPASS_V02:
        locSvSystemType = GNSS_LOC_SV_SYSTEM_BDS;
        break;

    case eQMI_LOC_SV_SYSTEM_QZSS_V02:
        locSvSystemType = GNSS_LOC_SV_SYSTEM_QZSS;
        break;

    case eQMI_LOC_SV_SYSTEM_NAVIC_V02:
        locSvSystemType = GNSS_LOC_SV_SYSTEM_NAVIC;
        break;

    default:
        break;
    }

    return locSvSystemType;
}
static GnssRfBand getLocApiSvRfBand(qmiLocGnssSignalTypeMaskT_v02 qmiGnssSignalType) {
    GnssRfBand rfBand = GNSS_RF_BAND_UNKNOWN;
    switch (qmiGnssSignalType) {
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L1CA_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L1C_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GLONASS_G1_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E1_C_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B1_I_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B1C_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L1CA_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L1S_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_SBAS_L1_CA_V02:
            rfBand = GNSS_RF_BAND_L1;
            break;
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L2C_L_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GLONASS_G2_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2_I_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2A_I_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L2C_L_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2A_Q_V02:
            rfBand = GNSS_RF_BAND_L2;
            break;
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L5_Q_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E5A_Q_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E5B_Q_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L5_Q_V02:
        case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_NAVIC_L5_V02:
            rfBand = GNSS_RF_BAND_L5;
            break;
        default:
            break;
    }
    return rfBand;
}

/* convert satellite polynomial to loc eng format and  send the converted
   report to loc eng */
void  LocApiV02 :: reportSvPolynomial(const qmiLocEventGnssSvPolyIndMsgT_v02 *gnss_sv_poly_ptr)
{
    GnssSvPolynomial  svPolynomial;

    memset(&svPolynomial, 0, sizeof(GnssSvPolynomial));
    svPolynomial.size = sizeof(GnssSvPolynomial);
    svPolynomial.is_valid = 0;

    if (0 != gnss_sv_poly_ptr->gnssSvId) {
        svPolynomial.gnssSvId       = gnss_sv_poly_ptr->gnssSvId;
        svPolynomial.T0             = gnss_sv_poly_ptr->T0;
        svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_T0;

        if (1 == gnss_sv_poly_ptr->gloFrequency_valid) {
            svPolynomial.is_valid  |= ULP_GNSS_SV_POLY_BIT_GLO_FREQ;
            svPolynomial.freqNum   = gnss_sv_poly_ptr->gloFrequency;
        }
        if (1 == gnss_sv_poly_ptr->IODE_valid) {
            svPolynomial.is_valid  |= ULP_GNSS_SV_POLY_BIT_IODE;
            svPolynomial.iode       = gnss_sv_poly_ptr->IODE;
        }

        if (1 == gnss_sv_poly_ptr->svPosUnc_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_SV_POSUNC;
            svPolynomial.svPosUnc = gnss_sv_poly_ptr->svPosUnc;
        }

        if (0 != gnss_sv_poly_ptr->svPolyFlagValid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_FLAG;
            svPolynomial.svPolyStatusMaskValidity = gnss_sv_poly_ptr->svPolyFlagValid;
            svPolynomial.svPolyStatusMask = gnss_sv_poly_ptr->svPolyFlags;
        }

        if ((svPolynomial.svPolyStatusMaskValidity & GNSS_SV_POLY_DELETE_VALID_V02) &&
            (svPolynomial.svPolyStatusMask & GNSS_SV_POLY_DELETE_V02)) {
            svPolynomial.is_valid &= ~ULP_GNSS_SV_POLY_BIT_T0;
        }

        if (1 == gnss_sv_poly_ptr->polyCoeffXYZ0_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_POLYCOEFF_XYZ0;
            for (int i = 0; i < GNSS_SV_POLY_XYZ_0_TH_ORDER_COEFF_MAX_SIZE; i++) {
                svPolynomial.polyCoeffXYZ0[i] = gnss_sv_poly_ptr->polyCoeffXYZ0[i];
            }
        }

        if (1 == gnss_sv_poly_ptr->polyCoefXYZN_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_POLYCOEFF_XYZN;
            for (int i = 0; i < GNSS_SV_POLY_XYZ_N_TH_ORDER_COEFF_MAX_SIZE; i++) {
                svPolynomial.polyCoefXYZN[i] = gnss_sv_poly_ptr->polyCoefXYZN[i];
        }
        }

        if (1 == gnss_sv_poly_ptr->polyCoefClockBias_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_POLYCOEFF_OTHER;
            for (int i = 0; i < GNSS_SV_POLY_SV_CLKBIAS_COEFF_MAX_SIZE; i++) {
                svPolynomial.polyCoefOther[i] = gnss_sv_poly_ptr->polyCoefClockBias[i];
            }
        }

        if (1 == gnss_sv_poly_ptr->ionoDot_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_IONODOT;
            svPolynomial.ionoDot = gnss_sv_poly_ptr->ionoDot;
        }
        if (1 == gnss_sv_poly_ptr->ionoDelay_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_IONODELAY;
            svPolynomial.ionoDelay = gnss_sv_poly_ptr->ionoDelay;
        }

        if (1 == gnss_sv_poly_ptr->sbasIonoDot_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_SBAS_IONODOT;
            svPolynomial.sbasIonoDot = gnss_sv_poly_ptr->sbasIonoDot;
        }
        if (1 == gnss_sv_poly_ptr->sbasIonoDelay_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_SBAS_IONODELAY;
            svPolynomial.sbasIonoDelay = gnss_sv_poly_ptr->sbasIonoDelay;
        }
        if (1 == gnss_sv_poly_ptr->tropoDelay_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_TROPODELAY;
            svPolynomial.tropoDelay = gnss_sv_poly_ptr->tropoDelay;
        }
        if (1 == gnss_sv_poly_ptr->elevation_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_ELEVATION;
            svPolynomial.elevation = gnss_sv_poly_ptr->elevation;
        }
        if (1 == gnss_sv_poly_ptr->elevationDot_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_ELEVATIONDOT;
            svPolynomial.elevationDot = gnss_sv_poly_ptr->elevationDot;
        }
        if (1 == gnss_sv_poly_ptr->elenationUnc_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_ELEVATIONUNC;
            svPolynomial.elevationUnc = gnss_sv_poly_ptr->elenationUnc;
        }
        if (1 == gnss_sv_poly_ptr->velCoef_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_VELO_COEFF;
            for (int i = 0; i < GNSS_SV_POLY_VELOCITY_COEF_MAX_SIZE; i++) {
                svPolynomial.velCoef[i] = gnss_sv_poly_ptr->velCoef[i];
            }
        }
        if (1 == gnss_sv_poly_ptr->enhancedIOD_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_ENHANCED_IOD;
            svPolynomial.enhancedIOD = gnss_sv_poly_ptr->enhancedIOD;
        }

        if (1 == gnss_sv_poly_ptr->gpsIscL1ca_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_GPS_ISC_L1CA;
            svPolynomial.gpsIscL1ca = gnss_sv_poly_ptr->gpsIscL1ca;
        }

        if (1 == gnss_sv_poly_ptr->gpsIscL2c_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_GPS_ISC_L2C;
            svPolynomial.gpsIscL2c = gnss_sv_poly_ptr->gpsIscL2c;
        }

        if (1 == gnss_sv_poly_ptr->gpsIscL5I5_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_GPS_ISC_L5I5;
            svPolynomial.gpsIscL5I5 = gnss_sv_poly_ptr->gpsIscL5I5;
        }

        if (1 == gnss_sv_poly_ptr->gpsIscL5Q5_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_GPS_ISC_L5Q5;
            svPolynomial.gpsIscL5Q5 = gnss_sv_poly_ptr->gpsIscL5Q5;
        }

        if (1 == gnss_sv_poly_ptr->gpsTgd_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_GPS_TGD;
            svPolynomial.gpsTgd = gnss_sv_poly_ptr->gpsTgd;
        }

        if (1 == gnss_sv_poly_ptr->gloTgdG1G2_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_GLO_TGD_G1G2;
            svPolynomial.gloTgdG1G2 = gnss_sv_poly_ptr->gloTgdG1G2;
        }

        if (1 == gnss_sv_poly_ptr->bdsTgdB1_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_BDS_TGD_B1;
            svPolynomial.bdsTgdB1 = gnss_sv_poly_ptr->bdsTgdB1;
        }

        if (1 == gnss_sv_poly_ptr->bdsTgdB2_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_BDS_TGD_B2;
            svPolynomial.bdsTgdB2= gnss_sv_poly_ptr->bdsTgdB2;
        }

        if (1 == gnss_sv_poly_ptr->bdsTgdB2a_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_BDS_TGD_B2A;
            svPolynomial.bdsTgdB2a = gnss_sv_poly_ptr->bdsTgdB2a;
        }

        if (1 == gnss_sv_poly_ptr->bdsIscB2a_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_BDS_ISC_B2A;
            svPolynomial.bdsIscB2a = gnss_sv_poly_ptr->bdsIscB2a;
        }

        if (1 == gnss_sv_poly_ptr->galBgdE1E5a_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_GAL_BGD_E1E5A;
            svPolynomial.galBgdE1E5a = gnss_sv_poly_ptr->galBgdE1E5a;
        }

        if (1 == gnss_sv_poly_ptr->galBgdE1E5b_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_GAL_BGD_E1E5B;
            svPolynomial.galBgdE1E5b = gnss_sv_poly_ptr->galBgdE1E5b;
        }

        if (1 == gnss_sv_poly_ptr->navicTgdL5_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_NAVIC_TGD_L5;
            svPolynomial.navicTgdL5 = gnss_sv_poly_ptr->navicTgdL5;
        }

        if (1 == gnss_sv_poly_ptr->bdsTgdB1c_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_BDS_TGD_B1C;
            svPolynomial.bdsTgdB1c = gnss_sv_poly_ptr->bdsTgdB1c;
        }

        if (1 == gnss_sv_poly_ptr->bdsIscB1c_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_BDS_ISC_B1C;
            svPolynomial.bdsIscB1c = gnss_sv_poly_ptr->bdsIscB1c;
        }

        if (1 == gnss_sv_poly_ptr->toc_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_TOC;
            svPolynomial.toc = gnss_sv_poly_ptr->toc;
        }

        if (1 == gnss_sv_poly_ptr->IODC_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_IODC;
            svPolynomial.iodc = gnss_sv_poly_ptr->IODC;
        }

        if (1 == gnss_sv_poly_ptr->toe_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_TOE;
            svPolynomial.toe = gnss_sv_poly_ptr->toe;
        }

        if (1 == gnss_sv_poly_ptr->ephemerisSrc_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_EPHEMERIS_SOURCE;
            svPolynomial.gnssLocEphemerisSource = gnss_sv_poly_ptr->ephemerisSrc;
        }

        if (1 == gnss_sv_poly_ptr->bdsTgdB2bi_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_BDS_TGD_B2BI;
            svPolynomial.bdsTgdB2bi = gnss_sv_poly_ptr->bdsTgdB2bi;
        }

        if (1 == gnss_sv_poly_ptr->bdsIscB2bi_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_BDS_ISC_B2BI;
            svPolynomial.bdsIscB2bi = gnss_sv_poly_ptr->bdsIscB2bi;
        }

        if (1 == gnss_sv_poly_ptr->polyOrder_valid) {
            svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_POLY_ORDER;
            svPolynomial.polyOrder = gnss_sv_poly_ptr->polyOrder;
        }
        if (GNSS_SV_POLY_ORDER_SIZE_DEFAULT < svPolynomial.polyOrder &&
                GNSS_SV_POLY_ORDER_SIZE_MAX >= svPolynomial.polyOrder) {
            if (1 == gnss_sv_poly_ptr->polyCoeffXYZ_valid) {
                svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_POLYCOEFF_XYZ;
                for (int i = 0; i < gnss_sv_poly_ptr->polyCoeffXYZ_len &&
                        i < GNSS_SV_POLY_XYZ_COEFF_SIZE_MAX; i++) {
                    svPolynomial.polyCoeffXYZ[i] = gnss_sv_poly_ptr->polyCoeffXYZ[i];
                }
            }

            if (1 == gnss_sv_poly_ptr->polyClockBias_valid) {
                svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_POLYCOEFF_CLKBIAS;
                for (int i =0; i < gnss_sv_poly_ptr->polyClockBias_len &&
                        i < GNSS_SV_POLY_CLKBIAS_COEFF_SIZE_MAX; i++) {
                    svPolynomial.polyClockBias[i] = gnss_sv_poly_ptr->polyClockBias[i];
                }
            }

            if (1 == gnss_sv_poly_ptr->validDuration_valid) {
                svPolynomial.is_valid |= ULP_GNSS_SV_POLY_BIT_POLY_DURATION;
                svPolynomial.validDuration = gnss_sv_poly_ptr->validDuration;
            }
        }

        //Report SV Poly
        LocApiBase::reportSvPolynomial(svPolynomial);

        // need to save svPolynomial in mSvPolynomialMap
        /* For GAL we can get the poly message twice for the same SV,
        since it could be FNAV or INAV, based on svPolyFlags
        To simplify the implementation and have just one key, for
        GAL we leave svId unchanged if INAV, but add 50 to it if
        FNAV. Since GAL svId is between GAL_SV_PRN_MIN = 301 and
        GAL_SV_PRN_MAX = 336 it follows that those will be the limits
        for INAV and 351 and 386 will be the limits for FNAV. When extracting
        an entry from the map we will adjust the svId in the opposite direction */

        if (svPolynomial.gnssSvId >= GAL_SV_PRN_MIN &&
            svPolynomial.gnssSvId <= GAL_SV_PRN_MAX) {
            if ((svPolynomial.svPolyStatusMaskValidity &
                        QMI_LOC_SV_POLY_SRC_GAL_FNAV_OR_INAV_VALID_V02) &&
                (svPolynomial.svPolyStatusMask & QMI_LOC_SV_POLY_SRC_GAL_FNAV_OR_INAV_V02)) {
                svPolynomial.gnssSvId += 50;
            }
        }
        mSvPolynomialMap[svPolynomial.gnssSvId] = svPolynomial;
    }
} //reportSvPolynomial

void LocApiV02::reportLocEvent(const qmiLocEventReportIndMsgT_v02 *event_report_ptr)
{
    GnssAidingData aidingData;
    memset(&aidingData, 0, sizeof(aidingData));
    LOC_LOGv("Loc event report: %" PRIu64 " KlobucharIonoMode_valid:%d: leapSec_valid:%d: "
             "tauC_valid:%d featureStatusReport_valid: %d featureStatusReport: %" PRIu64 "",
            event_report_ptr->eventReport, event_report_ptr->klobucharIonoModel_valid,
            event_report_ptr->leapSec_valid, event_report_ptr->tauC_valid,
            event_report_ptr->featureStatusReport_valid, event_report_ptr->featureStatusReport);

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_GPS_EPHEMERIS_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_EPHEMERIS_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_GPS_BIT;
    }

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_GLO_EPHEMERIS_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_EPHEMERIS_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_GLONASS_BIT;
    }

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_BDS_EPHEMERIS_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_EPHEMERIS_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_BEIDOU_BIT;
    }

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_GAL_EPHEMERIS_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_EPHEMERIS_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_GALILEO_BIT;
    }

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_QZSS_EPHEMERIS_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_EPHEMERIS_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_QZSS_BIT;
    }

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_GPS_SV_POLY_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_POLY_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_GPS_BIT;
    }

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_GLO_SV_POLY_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_POLY_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_GLONASS_BIT;
    }

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_BDS_SV_POLY_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_POLY_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_BEIDOU_BIT;
    }

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_GAL_SV_POLY_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_POLY_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_GALILEO_BIT;
    }

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_QZSS_SV_POLY_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_POLY_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_QZSS_BIT;
    }

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_GPS_IONO_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_IONOSPHERE_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_GPS_BIT;
    }

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_GLO_IONO_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_IONOSPHERE_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_GLONASS_BIT;
    }

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_BDS_IONO_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_IONOSPHERE_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_BEIDOU_BIT;
    }

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_GAL_IONO_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_IONOSPHERE_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_GALILEO_BIT;
    }

    if (event_report_ptr->eventReport & QMI_LOC_DELETE_QZSS_IONO_ALL_V02) {
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_IONOSPHERE_BIT;
        aidingData.sv.svTypeMask |= GNSS_AIDING_DATA_SV_TYPE_QZSS_BIT;
    }

    if ((aidingData.sv.svMask != 0) && (aidingData.sv.svTypeMask != 0)) {
        LocApiBase::reportDeleteAidingDataEvent(aidingData);
    }

    // report Klobuchar IonoModel
    if (event_report_ptr->klobucharIonoModel_valid) {
        GnssKlobucharIonoModel klobucharIonoModel;
        memset(&klobucharIonoModel, 0, sizeof(klobucharIonoModel));

        if (event_report_ptr->gpsSystemTime_valid) {
            klobucharIonoModel.isSystemTimeValid = true;
            populateGpsTimeOfReport(event_report_ptr->gpsSystemTime, klobucharIonoModel.systemTime);
        }

        switch(event_report_ptr->klobucharIonoModel.dataSource) {
            case eQMI_LOC_SV_SYSTEM_GPS_V02:
                klobucharIonoModel.gnssConstellation = GNSS_LOC_SV_SYSTEM_GPS;
                break;
            case eQMI_LOC_SV_SYSTEM_GALILEO_V02:
                klobucharIonoModel.gnssConstellation = GNSS_LOC_SV_SYSTEM_GALILEO;
                break;
            case eQMI_LOC_SV_SYSTEM_SBAS_V02:
                klobucharIonoModel.gnssConstellation = GNSS_LOC_SV_SYSTEM_SBAS;
                break;
            case eQMI_LOC_SV_SYSTEM_GLONASS_V02:
                klobucharIonoModel.gnssConstellation = GNSS_LOC_SV_SYSTEM_GLONASS;
                break;
            case eQMI_LOC_SV_SYSTEM_BDS_V02:
            case eQMI_LOC_SV_SYSTEM_COMPASS_V02:
                klobucharIonoModel.gnssConstellation = GNSS_LOC_SV_SYSTEM_BDS;
                break;
            case eQMI_LOC_SV_SYSTEM_QZSS_V02:
                klobucharIonoModel.gnssConstellation = GNSS_LOC_SV_SYSTEM_QZSS;
                break;
            default:
                break;
        }

        klobucharIonoModel.alpha0 = event_report_ptr->klobucharIonoModel.alpha0;
        klobucharIonoModel.alpha1 = event_report_ptr->klobucharIonoModel.alpha1;
        klobucharIonoModel.alpha2 = event_report_ptr->klobucharIonoModel.alpha2;
        klobucharIonoModel.alpha3 = event_report_ptr->klobucharIonoModel.alpha3;
        klobucharIonoModel.beta0  = event_report_ptr->klobucharIonoModel.beta0;
        klobucharIonoModel.beta1 = event_report_ptr->klobucharIonoModel.beta1;
        klobucharIonoModel.beta2 = event_report_ptr->klobucharIonoModel.beta2;
        klobucharIonoModel.beta3 = event_report_ptr->klobucharIonoModel.beta3;
        LOC_LOGa("iono model: %d:%f:%f:%f:%f:%f:%f:%f:%f:%d",
            klobucharIonoModel.gnssConstellation,  klobucharIonoModel.alpha0,
            klobucharIonoModel.alpha1, klobucharIonoModel.alpha2, klobucharIonoModel.alpha3,
            klobucharIonoModel.beta0, klobucharIonoModel.beta1, klobucharIonoModel.beta2,
            klobucharIonoModel.beta3, klobucharIonoModel.isSystemTimeValid);

        LocApiBase::reportKlobucharIonoModel(klobucharIonoModel);
    }

    // report gnss additional system info
    GnssAdditionalSystemInfo additionalSystemInfo;
    memset(&additionalSystemInfo, 0, sizeof(additionalSystemInfo));
    if (event_report_ptr->leapSec_valid) {
        additionalSystemInfo.validityMask |= GNSS_ADDITIONAL_SYSTEMINFO_HAS_LEAP_SEC;
        additionalSystemInfo.leapSec = event_report_ptr->leapSec;
        LOC_LOGa("LeapSec: %d", event_report_ptr->leapSec);
    }
    if (event_report_ptr->tauC_valid) {
        additionalSystemInfo.validityMask |= GNSS_ADDITIONAL_SYSTEMINFO_HAS_TAUC;
        additionalSystemInfo.tauC = event_report_ptr->tauC;
        LOC_LOGa("tauC: %lf", event_report_ptr->tauC);
    }
    if (additionalSystemInfo.validityMask && event_report_ptr->gpsSystemTime_valid) {
        additionalSystemInfo.isSystemTimeValid = true;
        populateGpsTimeOfReport(event_report_ptr->gpsSystemTime, additionalSystemInfo.systemTime);
        LocApiBase::reportGnssAdditionalSystemInfo(additionalSystemInfo);
    }
    if (event_report_ptr->featureStatusReport_valid) {
        std::unordered_map<LocationQwesFeatureType, bool> featureMap;
        populateFeatureStatusReport(event_report_ptr->featureStatusReport, featureMap);
        LocApiBase::reportQwesCapabilities(featureMap);
    }
}

void LocApiV02::populateFeatureStatusReport
(
        const qmiLocFeaturesStatusMaskT_v02 &featureStatusReport,
        std::unordered_map<LocationQwesFeatureType, bool> &featureMap
)
{
    if (featureStatusReport & QMI_LOC_FEATURE_STATUS_CARRIER_PHASE_V02) {
        featureMap[LOCATION_QWES_FEATURE_TYPE_CARRIER_PHASE] = true;
    } else {
        featureMap[LOCATION_QWES_FEATURE_TYPE_CARRIER_PHASE] = false;
    }
    if (featureStatusReport & QMI_LOC_FEATURE_STATUS_SV_POLYNOMIALS_V02) {
        featureMap[LOCATION_QWES_FEATURE_TYPE_SV_POLYNOMIAL] = true;
    } else {
        featureMap[LOCATION_QWES_FEATURE_TYPE_SV_POLYNOMIAL] = false;
    }
    if (featureStatusReport & QMI_LOC_FEATURE_STATUS_SV_EPHEMERIS_V02) {
        featureMap[LOCATION_QWES_FEATURE_TYPE_SV_EPH] = true;
    } else {
        featureMap[LOCATION_QWES_FEATURE_TYPE_SV_EPH] = false;
    }
    if (featureStatusReport & QMI_LOC_FEATURE_STATUS_SINGLE_FREQUENCY_V02) {
        featureMap[LOCATION_QWES_FEATURE_TYPE_GNSS_SINGLE_FREQUENCY] = true;
    } else {
        featureMap[LOCATION_QWES_FEATURE_TYPE_GNSS_SINGLE_FREQUENCY] = false;
    }
    if (featureStatusReport & QMI_LOC_FEATURE_STATUS_MULTI_FREQUENCY_V02) {
        featureMap[LOCATION_QWES_FEATURE_TYPE_GNSS_MULTI_FREQUENCY] = true;
    } else {
        featureMap[LOCATION_QWES_FEATURE_TYPE_GNSS_MULTI_FREQUENCY] = false;
    }
    if (featureStatusReport & QMI_LOC_FEATURE_STATUS_TIME_FREQUENCY_V02) {
        featureMap[LOCATION_QWES_FEATURE_TYPE_TIME_FREQUENCY] = true;
    } else {
        featureMap[LOCATION_QWES_FEATURE_TYPE_TIME_FREQUENCY] = false;
    }
    if (featureStatusReport & QMI_LOC_FEATURE_STATUS_TIME_UNCERTAINTY_V02) {
        featureMap[LOCATION_QWES_FEATURE_TYPE_TIME_UNCERTAINTY] = true;
    } else {
        featureMap[LOCATION_QWES_FEATURE_TYPE_TIME_UNCERTAINTY] = false;
    }
    if (featureStatusReport & QMI_LOC_FEATURE_STATUS_CLOCK_ESTIMATE_V02) {
        featureMap[LOCATION_QWES_FEATURE_TYPE_CLOCK_ESTIMATE] = true;
    } else {
        featureMap[LOCATION_QWES_FEATURE_TYPE_CLOCK_ESTIMATE] = false;
    }
    if (featureStatusReport & QMI_LOC_FEATURE_STATUS_DGNSS_V02) {
        featureMap[LOCATION_QWES_FEATURE_TYPE_DGNSS] = true;
    } else {
        featureMap[LOCATION_QWES_FEATURE_TYPE_DGNSS] = false;
    }
    if (featureStatusReport & QMI_LOC_FEATURE_STATUS_NLOS_ML20_V02) {
        featureMap[LOCATION_QWES_FEATURE_NLOS_ML20] = true;
    } else {
        featureMap[LOCATION_QWES_FEATURE_NLOS_ML20] = false;
    }
    if (featureStatusReport & QMI_LOC_FEATURE_STATUS_GNSS_NHZ_V02) {
        featureMap[LOCATION_QWES_FEATURE_STATUS_GNSS_NHZ] = true;
    } else {
        featureMap[LOCATION_QWES_FEATURE_STATUS_GNSS_NHZ] = false;
    }
}

void LocApiV02::reportSvEphemeris (
        uint32_t eventId, const locClientEventIndUnionType &eventPayload) {

    GnssSvEphemerisReport svEphemeris;
    memset(&svEphemeris, 0, sizeof(svEphemeris));

    switch (eventId)
    {
        case QMI_LOC_EVENT_GPS_EPHEMERIS_REPORT_IND_V02:
            svEphemeris.gnssConstellation = GNSS_LOC_SV_SYSTEM_GPS;
            populateGpsEphemeris(eventPayload.pGpsEphemerisReportEvent, svEphemeris);
            break;
        case QMI_LOC_EVENT_GLONASS_EPHEMERIS_REPORT_IND_V02:
            svEphemeris.gnssConstellation = GNSS_LOC_SV_SYSTEM_GLONASS;
            populateGlonassEphemeris(eventPayload.pGloEphemerisReportEvent, svEphemeris);
            break;
        case QMI_LOC_EVENT_BDS_EPHEMERIS_REPORT_IND_V02:
            svEphemeris.gnssConstellation = GNSS_LOC_SV_SYSTEM_BDS;
            populateBdsEphemeris(eventPayload.pBdsEphemerisReportEvent, svEphemeris);
            break;
        case QMI_LOC_EVENT_GALILEO_EPHEMERIS_REPORT_IND_V02:
            svEphemeris.gnssConstellation = GNSS_LOC_SV_SYSTEM_GALILEO;
            populateGalEphemeris(eventPayload.pGalEphemerisReportEvent, svEphemeris);
            break;
        case QMI_LOC_EVENT_QZSS_EPHEMERIS_REPORT_IND_V02:
            svEphemeris.gnssConstellation = GNSS_LOC_SV_SYSTEM_QZSS;
            populateQzssEphemeris(eventPayload.pQzssEphemerisReportEvent, svEphemeris);
    }

    LocApiBase::reportSvEphemeris(svEphemeris);
}

void LocApiV02::populateGpsTimeOfReport(const qmiLocGnssTimeStructT_v02 &inGpsSystemTime,
        GnssSystemTimeStructType& outGpsSystemTime) {

    outGpsSystemTime.validityMask = GNSS_SYSTEM_TIME_WEEK_VALID |  GNSS_SYSTEM_TIME_WEEK_MS_VALID |
            GNSS_SYSTEM_CLK_TIME_BIAS_VALID | GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID;

    outGpsSystemTime.systemWeek = inGpsSystemTime.systemWeek;
    outGpsSystemTime.systemMsec = inGpsSystemTime.systemMsec;
    outGpsSystemTime.systemClkTimeBias = inGpsSystemTime.systemClkTimeBias;
    outGpsSystemTime.systemClkTimeUncMs = inGpsSystemTime.systemClkTimeUncMs;
}

void LocApiV02::populateCommonEphemeris(const qmiLocEphGnssDataStructT_v02 &receivedEph,
        GnssEphCommon &ephToFill)
{
    LOC_LOGv("Eph received for sv-id: %d action:%d", receivedEph.gnssSvId,
            receivedEph.updateAction);

    ephToFill.gnssSvId = receivedEph.gnssSvId;
    switch(receivedEph.updateAction) {
        case eQMI_LOC_UPDATE_EPH_SRC_UNKNOWN_V02:
            ephToFill.updateAction = GNSS_EPH_ACTION_UPDATE_SRC_UNKNOWN_V02;
            break;
        case eQMI_LOC_UPDATE_EPH_SRC_OTA_V02:
            ephToFill.updateAction = GNSS_EPH_ACTION_UPDATE_SRC_OTA_V02;
            break;
        case eQMI_LOC_UPDATE_EPH_SRC_NETWORK_V02:
            ephToFill.updateAction = GNSS_EPH_ACTION_UPDATE_SRC_NETWORK_V02;
            break;
        case eQMI_LOC_DELETE_EPH_SRC_UNKNOWN_V02:
            ephToFill.updateAction = GNSS_EPH_ACTION_DELETE_SRC_UNKNOWN_V02;
            break;
        case eQMI_LOC_DELETE_EPH_SRC_NETWORK_V02:
            ephToFill.updateAction = GNSS_EPH_ACTION_DELETE_SRC_NETWORK_V02;
            break;
        case eQMI_LOC_DELETE_EPH_SRC_OTA_V02:
            ephToFill.updateAction = GNSS_EPH_ACTION_DELETE_SRC_OTA_V02;
            break;
        default:
            break;
    }

    ephToFill.IODE = receivedEph.IODE;
    ephToFill.aSqrt = receivedEph.aSqrt;
    ephToFill.deltaN = receivedEph.deltaN;
    ephToFill.m0 = receivedEph.m0;
    ephToFill.eccentricity = receivedEph.eccentricity;
    ephToFill.omega0 = receivedEph.omega0;
    ephToFill.i0 = receivedEph.i0;
    ephToFill.omega = receivedEph.omega;
    ephToFill.omegaDot = receivedEph.omegaDot;
    ephToFill.iDot = receivedEph.iDot;
    ephToFill.cUc = receivedEph.cUc;
    ephToFill.cUs = receivedEph.cUs;
    ephToFill.cRc = receivedEph.cRc;
    ephToFill.cRs = receivedEph.cRs;
    ephToFill.cIc = receivedEph.cIc;
    ephToFill.cIs = receivedEph.cIs;
    ephToFill.toe = receivedEph.toe;
    ephToFill.toc = receivedEph.toc;
    ephToFill.af0 = receivedEph.af0;
    ephToFill.af1 = receivedEph.af1;
    ephToFill.af2 = receivedEph.af2;
}


void LocApiV02::populateGpsEphemeris(
        const qmiLocGpsEphemerisReportIndMsgT_v02 *gpsEphemeris,
        GnssSvEphemerisReport &svEphemeris)
{
    LOC_LOGv("GPS Ephemeris Received: Len= %d: systemTime_valid%d",
            gpsEphemeris->gpsEphemerisList_len, gpsEphemeris->gpsSystemTime_valid);
    svEphemeris.ephInfo.gpsEphemeris.numOfEphemeris = gpsEphemeris->gpsEphemerisList_len;

    if (gpsEphemeris->gpsSystemTime_valid) {
        svEphemeris.isSystemTimeValid = true;
        populateGpsTimeOfReport(gpsEphemeris->gpsSystemTime, svEphemeris.systemTime);
    }

    for (uint32_t i =0; i < gpsEphemeris->gpsEphemerisList_len; i++) {
        const qmiLocGpsEphemerisT_v02 &receivedGpsEphemeris = gpsEphemeris->gpsEphemerisList[i];
        GpsEphemeris &gpsEphemerisToFill = svEphemeris.ephInfo.gpsEphemeris.gpsEphemerisData[i];

        populateCommonEphemeris(receivedGpsEphemeris.commonEphemerisData,
                gpsEphemerisToFill.commonEphemerisData);

        gpsEphemerisToFill.signalHealth = receivedGpsEphemeris.signalHealth;
        gpsEphemerisToFill.URAI = receivedGpsEphemeris.URAI;
        gpsEphemerisToFill.codeL2 = receivedGpsEphemeris.codeL2;
        gpsEphemerisToFill.dataFlagL2P = receivedGpsEphemeris.dataFlagL2P;
        gpsEphemerisToFill.tgd = receivedGpsEphemeris.tgd;
        gpsEphemerisToFill.fitInterval = receivedGpsEphemeris.fitInterval;
        gpsEphemerisToFill.IODC = receivedGpsEphemeris.IODC;
    }
}

void LocApiV02::populateGlonassEphemeris(const qmiLocGloEphemerisReportIndMsgT_v02 *gloEphemeris,
        GnssSvEphemerisReport &svEphemeris)
{
    LOC_LOGv("GLO Ephemeris Received: Len= %d: systemTime_valid%d",
             gloEphemeris->gloEphemerisList_len, gloEphemeris->gpsSystemTime_valid);
    svEphemeris.ephInfo.glonassEphemeris.numOfEphemeris = gloEphemeris->gloEphemerisList_len;

    if (gloEphemeris->gpsSystemTime_valid) {
        svEphemeris.isSystemTimeValid = true;
        populateGpsTimeOfReport(gloEphemeris->gpsSystemTime, svEphemeris.systemTime);
    }

    for (uint32_t i =0; i < gloEphemeris->gloEphemerisList_len; i++) {
        const qmiLocGloEphemerisT_v02 &receivedGloEphemeris = gloEphemeris->gloEphemerisList[i];
        GlonassEphemeris &gloEphemerisToFill = svEphemeris.ephInfo.glonassEphemeris.gloEphemerisData[i];

        LOC_LOGv("Eph received for sv-id: %d action:%d", receivedGloEphemeris.gnssSvId,
                 receivedGloEphemeris.updateAction);

        gloEphemerisToFill.gnssSvId = receivedGloEphemeris.gnssSvId;
        switch(receivedGloEphemeris.updateAction) {
            case eQMI_LOC_UPDATE_EPH_SRC_UNKNOWN_V02:
                gloEphemerisToFill.updateAction = GNSS_EPH_ACTION_UPDATE_SRC_UNKNOWN_V02;
                break;
            case eQMI_LOC_UPDATE_EPH_SRC_OTA_V02:
                gloEphemerisToFill.updateAction = GNSS_EPH_ACTION_UPDATE_SRC_OTA_V02;
                break;
            case eQMI_LOC_UPDATE_EPH_SRC_NETWORK_V02:
                gloEphemerisToFill.updateAction = GNSS_EPH_ACTION_UPDATE_SRC_NETWORK_V02;
                break;
            case eQMI_LOC_DELETE_EPH_SRC_UNKNOWN_V02:
                gloEphemerisToFill.updateAction = GNSS_EPH_ACTION_DELETE_SRC_UNKNOWN_V02;
                break;
            case eQMI_LOC_DELETE_EPH_SRC_NETWORK_V02:
                gloEphemerisToFill.updateAction = GNSS_EPH_ACTION_DELETE_SRC_NETWORK_V02;
                break;
            case eQMI_LOC_DELETE_EPH_SRC_OTA_V02:
                gloEphemerisToFill.updateAction = GNSS_EPH_ACTION_DELETE_SRC_OTA_V02;
                break;
            default:
                break;
        }

        gloEphemerisToFill.bnHealth = receivedGloEphemeris.bnHealth;
        gloEphemerisToFill.lnHealth = receivedGloEphemeris.lnHealth;
        gloEphemerisToFill.tb = receivedGloEphemeris.tb;
        gloEphemerisToFill.ft = receivedGloEphemeris.ft;
        gloEphemerisToFill.gloM = receivedGloEphemeris.gloM;
        gloEphemerisToFill.enAge = receivedGloEphemeris.enAge;
        gloEphemerisToFill.gloFrequency = receivedGloEphemeris.gloFrequency;
        gloEphemerisToFill.p1 = receivedGloEphemeris.p1;
        gloEphemerisToFill.p2 = receivedGloEphemeris.p2;
        gloEphemerisToFill.deltaTau = receivedGloEphemeris.deltaTau;
        memcpy(gloEphemerisToFill.position, receivedGloEphemeris.position,
              sizeof(*receivedGloEphemeris.position) * 3);
        memcpy(gloEphemerisToFill.velocity, receivedGloEphemeris.velocity,
              sizeof(*receivedGloEphemeris.velocity) * 3);
        memcpy(gloEphemerisToFill.acceleration, receivedGloEphemeris.acceleration,
              sizeof(*receivedGloEphemeris.acceleration) * 3);
        gloEphemerisToFill.tauN = receivedGloEphemeris.tauN;
        gloEphemerisToFill.gamma = receivedGloEphemeris.gamma;
        gloEphemerisToFill.toe = receivedGloEphemeris.toe;
        gloEphemerisToFill.nt = receivedGloEphemeris.nt;
    }
}

void LocApiV02::populateBdsEphemeris(const qmiLocBdsEphemerisReportIndMsgT_v02 *bdsEphemeris,
        GnssSvEphemerisReport &svEphemeris)
{
    LOC_LOGv("BDS Ephemeris Received: Len= %d: systemTime_valid%d",
            bdsEphemeris->bdsEphemerisList_len, bdsEphemeris->gpsSystemTime_valid);
    svEphemeris.ephInfo.bdsEphemeris.numOfEphemeris = bdsEphemeris->bdsEphemerisList_len;

    if (bdsEphemeris->gpsSystemTime_valid) {
        svEphemeris.isSystemTimeValid = true;
        populateGpsTimeOfReport(bdsEphemeris->gpsSystemTime, svEphemeris.systemTime);
    }


    for (uint32_t i =0; i < bdsEphemeris->bdsEphemerisList_len; i++) {
        const qmiLocBdsEphemerisT_v02 &receivedBdsEphemeris = bdsEphemeris->bdsEphemerisList[i];
        BdsEphemeris &bdsEphemerisToFill = svEphemeris.ephInfo.bdsEphemeris.bdsEphemerisData[i];

        populateCommonEphemeris(receivedBdsEphemeris.commonEphemerisData,
                bdsEphemerisToFill.commonEphemerisData);

        bdsEphemerisToFill.svHealth = receivedBdsEphemeris.svHealth;
        bdsEphemerisToFill.AODC = receivedBdsEphemeris.AODC;
        bdsEphemerisToFill.tgd1 = receivedBdsEphemeris.tgd1;
        bdsEphemerisToFill.tgd2 = receivedBdsEphemeris.tgd2;
        bdsEphemerisToFill.URAI = receivedBdsEphemeris.URAI;
    }
}

void LocApiV02::populateGalEphemeris(const qmiLocGalEphemerisReportIndMsgT_v02 *galEphemeris,
        GnssSvEphemerisReport &svEphemeris)
{
    LOC_LOGv("GAL Ephemeris Received: Len= %d: systemTime_valid%d",
            galEphemeris->galEphemerisList_len, galEphemeris->gpsSystemTime_valid);
    svEphemeris.ephInfo.galileoEphemeris.numOfEphemeris = galEphemeris->galEphemerisList_len;

    if (galEphemeris->gpsSystemTime_valid) {
        svEphemeris.isSystemTimeValid = true;
        populateGpsTimeOfReport(galEphemeris->gpsSystemTime, svEphemeris.systemTime);
    }


    for (uint32_t i =0; i < galEphemeris->galEphemerisList_len; i++) {
        const qmiLocGalEphemerisT_v02 &receivedGalEphemeris = galEphemeris->galEphemerisList[i];
        GalileoEphemeris &galEphemerisToFill = svEphemeris.ephInfo.galileoEphemeris.galEphemerisData[i];

        populateCommonEphemeris(receivedGalEphemeris.commonEphemerisData,
                galEphemerisToFill.commonEphemerisData);

        switch (receivedGalEphemeris.dataSourceSignal) {
            case eQMI_LOC_GAL_EPH_SIGNAL_SRC_UNKNOWN_V02:
                galEphemerisToFill.dataSourceSignal = GAL_EPH_SIGNAL_SRC_UNKNOWN_V02;
                break;
            case eQMI_LOC_GAL_EPH_SIGNAL_SRC_E1B_V02:
                galEphemerisToFill.dataSourceSignal = GAL_EPH_SIGNAL_SRC_E1B_V02;
                break;
            case eQMI_LOC_GAL_EPH_SIGNAL_SRC_E5A_V02:
                galEphemerisToFill.dataSourceSignal = GAL_EPH_SIGNAL_SRC_E5A_V02;
                break;
            case eQMI_LOC_GAL_EPH_SIGNAL_SRC_E5B_V02:
                galEphemerisToFill.dataSourceSignal = GAL_EPH_SIGNAL_SRC_E5B_V02;
                break;
            default:
                break;
        }

        galEphemerisToFill.sisIndex = receivedGalEphemeris.sisIndex;
        galEphemerisToFill.bgdE1E5a = receivedGalEphemeris.bgdE1E5a;
        galEphemerisToFill.bgdE1E5b = receivedGalEphemeris.bgdE1E5b;
        galEphemerisToFill.svHealth = receivedGalEphemeris.svHealth;
    }
}

void LocApiV02::populateQzssEphemeris(const qmiLocQzssEphemerisReportIndMsgT_v02 *qzssEphemeris,
        GnssSvEphemerisReport &svEphemeris)
{
    LOC_LOGv("QZSS Ephemeris Received: Len= %d: systemTime_valid%d",
            qzssEphemeris->qzssEphemerisList_len, qzssEphemeris->gpsSystemTime_valid);
    svEphemeris.ephInfo.qzssEphemeris.numOfEphemeris = qzssEphemeris->qzssEphemerisList_len;

    if (qzssEphemeris->gpsSystemTime_valid) {
        svEphemeris.isSystemTimeValid = true;
        populateGpsTimeOfReport(qzssEphemeris->gpsSystemTime, svEphemeris.systemTime);
    }


    for (uint32_t i =0; i < qzssEphemeris->qzssEphemerisList_len; i++) {
        const qmiLocGpsEphemerisT_v02 &receivedQzssEphemeris = qzssEphemeris->qzssEphemerisList[i];
        GpsEphemeris &qzssEphemerisToFill = svEphemeris.ephInfo.qzssEphemeris.qzssEphemerisData[i];

        populateCommonEphemeris(receivedQzssEphemeris.commonEphemerisData,
                qzssEphemerisToFill.commonEphemerisData);

        qzssEphemerisToFill.signalHealth = receivedQzssEphemeris.signalHealth;
        qzssEphemerisToFill.URAI = receivedQzssEphemeris.URAI;
        qzssEphemerisToFill.codeL2 = receivedQzssEphemeris.codeL2;
        qzssEphemerisToFill.dataFlagL2P = receivedQzssEphemeris.dataFlagL2P;
        qzssEphemerisToFill.tgd = receivedQzssEphemeris.tgd;
        qzssEphemerisToFill.fitInterval = receivedQzssEphemeris.fitInterval;
        qzssEphemerisToFill.IODC = receivedQzssEphemeris.IODC;
    }
}

/* convert system info to GnssDataNotification format and dispatch
   to the registered adapter */
void  LocApiV02 :: reportSystemInfo(
    const qmiLocSystemInfoIndMsgT_v02* system_info_ptr){

    if (system_info_ptr == nullptr) {
        return;
    }

    LOC_LOGv("system info type: %d, leap second valid: %d "
             "current gps time:valid:%d, week: %d, msec: %d,"
             "current leap second:valid %d, seconds %d, "
             "next gps time: valid %d, week: %d, msec: %d,"
             "next leap second: valid %d, seconds %d",
             system_info_ptr->systemInfo,
             system_info_ptr->nextLeapSecondInfo_valid,
             system_info_ptr->nextLeapSecondInfo.gpsTimeCurrent_valid,
             system_info_ptr->nextLeapSecondInfo.gpsTimeCurrent.gpsWeek,
             system_info_ptr->nextLeapSecondInfo.gpsTimeCurrent.gpsTimeOfWeekMs,
             system_info_ptr->nextLeapSecondInfo.leapSecondsCurrent_valid,
             system_info_ptr->nextLeapSecondInfo.leapSecondsCurrent,
             system_info_ptr->nextLeapSecondInfo.gpsTimeNextLsEvent_valid,
             system_info_ptr->nextLeapSecondInfo.gpsTimeNextLsEvent.gpsWeek,
             system_info_ptr->nextLeapSecondInfo.gpsTimeNextLsEvent.gpsTimeOfWeekMs,
             system_info_ptr->nextLeapSecondInfo.leapSecondsNext_valid,
             system_info_ptr->nextLeapSecondInfo.leapSecondsNext);

    LocationSystemInfo systemInfo = {};
    if ((system_info_ptr->systemInfo == eQMI_LOC_NEXT_LEAP_SECOND_INFO_V02) &&
        (system_info_ptr->nextLeapSecondInfo_valid == 1)) {

        const qmiLocNextLeapSecondInfoStructT_v02 &nextLeapSecondInfo =
            system_info_ptr->nextLeapSecondInfo;

        if (nextLeapSecondInfo.gpsTimeNextLsEvent_valid &&
                nextLeapSecondInfo.leapSecondsCurrent_valid &&
                nextLeapSecondInfo.leapSecondsNext_valid) {

            systemInfo.systemInfoMask |= LOCATION_SYS_INFO_LEAP_SECOND;
            systemInfo.leapSecondSysInfo.leapSecondInfoMask |=
                    LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT;

            LeapSecondChangeInfo &leapSecondChangeInfo =
                    systemInfo.leapSecondSysInfo.leapSecondChangeInfo;
            leapSecondChangeInfo.gpsTimestampLsChange.systemWeek =
                    nextLeapSecondInfo.gpsTimeNextLsEvent.gpsWeek;
            leapSecondChangeInfo.gpsTimestampLsChange.systemMsec =
                    nextLeapSecondInfo.gpsTimeNextLsEvent.gpsTimeOfWeekMs;
            leapSecondChangeInfo.gpsTimestampLsChange.validityMask =
                    (GNSS_SYSTEM_TIME_WEEK_VALID | GNSS_SYSTEM_TIME_WEEK_MS_VALID);

            leapSecondChangeInfo.leapSecondsBeforeChange =
                    nextLeapSecondInfo.leapSecondsCurrent;
            leapSecondChangeInfo.leapSecondsAfterChange =
                    nextLeapSecondInfo.leapSecondsNext;
        }

        if (nextLeapSecondInfo.leapSecondsCurrent_valid) {
            systemInfo.systemInfoMask |= LOCATION_SYS_INFO_LEAP_SECOND;
            systemInfo.leapSecondSysInfo.leapSecondInfoMask |=
                    LEAP_SECOND_SYS_INFO_CURRENT_LEAP_SECONDS_BIT;
            systemInfo.leapSecondSysInfo.leapSecondCurrent =
                    nextLeapSecondInfo.leapSecondsCurrent;
        }
    }

    if (systemInfo.systemInfoMask) {
        LocApiBase::reportLocationSystemInfo(systemInfo);
    }
}

void LocApiV02::reportLocationRequestNotification(
    const qmiLocLocationRequestNotificationIndMsgT_v02* loc_req_notif)
{
    GnssNfwNotification notification = {};

    LOC_LOGv("IN: protocolStack=%d"
             " ,clientStrId_valid=%d"
             " ,clientStrId=%s"
             " ,requestor=%d"
             " ,requestorId=%s"
             " ,responseType=%d"
             " ,inEmergencyMode=%d"
             " ,isCachedLocation=%u",
             loc_req_notif->protocolStack,
             loc_req_notif->clientStrId_valid,
             loc_req_notif->clientStrId,
             loc_req_notif->requestor,
             loc_req_notif->requestorId,
             loc_req_notif->responseType,
             loc_req_notif->inEmergencyMode,
             loc_req_notif->isCachedLocation);

    switch (loc_req_notif->protocolStack) {
    case eQMI_LOC_CTRL_PLANE_V02:
        notification.protocolStack = GNSS_NFW_CTRL_PLANE;
        break;
    case eQMI_LOC_SUPL_V02:
        notification.protocolStack = GNSS_NFW_SUPL;
        break;
    case eQMI_LOC_IMS_V02:
        notification.protocolStack = GNSS_NFW_IMS;
        break;
    case eQMI_LOC_SIM_V02:
        notification.protocolStack = GNSS_NFW_SIM;
        break;
    case eQMI_LOC_MDT_V02:
    case eQMI_LOC_TLOC_V02:
    case eQMI_LOC_OTHER_V02:
    default:
        notification.protocolStack = GNSS_NFW_OTHER_PROTOCOL_STACK;
        if (loc_req_notif->clientStrId_valid) {
            strlcpy(notification.otherProtocolStackName,
                    loc_req_notif->clientStrId,
                    sizeof(notification.otherProtocolStackName));
        } else {
            strlcpy(notification.otherProtocolStackName,
                    "NFW Client",
                    sizeof(notification.otherProtocolStackName));
        }
        break;
    }
    switch (loc_req_notif->requestor) {
    case eQMI_LOC_REQUESTOR_CARRIER_V02:
        notification.requestor = GNSS_NFW_CARRIER;
        break;
    case eQMI_LOC_REQUESTOR_OEM_V02:
        notification.requestor = GNSS_NFW_OEM;
        break;
    case eQMI_LOC_REQUESTOR_MODEM_CHIPSET_VENDOR_V02:
        notification.requestor = GNSS_NFW_MODEM_CHIPSET_VENDOR;
        break;
    case eQMI_LOC_REQUESTOR_GNSS_CHIPSET_VENDOR_V02:
        notification.requestor = GNSS_NFW_GNSS_CHIPSET_VENDOR;
        break;
    case eQMI_LOC_REQUESTOR_OTHER_CHIPSET_VENDOR_V02:
        notification.requestor = GNSS_NFW_OTHER_CHIPSET_VENDOR;
        break;
    case eQMI_LOC_REQUESTOR_AUTOMOBILE_CLIENT_V02:
        notification.requestor = GNSS_NFW_AUTOMOBILE_CLIENT;
        break;
    case eQMI_LOC_REQUESTOR_OTHER_V02:
    default:
        notification.requestor = GNSS_NFW_OTHER_REQUESTOR;
        break;
    }
    switch (loc_req_notif->responseType) {
    case eQMI_LOC_REJECTED_V02:
        notification.responseType = GNSS_NFW_REJECTED;
        break;
    case eQMI_LOC_ACCEPTED_NO_LOCATION_PROVIDED_V02:
        notification.responseType = GNSS_NFW_ACCEPTED_NO_LOCATION_PROVIDED;
        break;
    case eQMI_LOC_ACCEPTED_LOCATION_PROVIDED_V02:
        notification.responseType = GNSS_NFW_ACCEPTED_LOCATION_PROVIDED;
        break;
    default:
        break;
    }
    notification.inEmergencyMode = (bool)loc_req_notif->inEmergencyMode;
    notification.isCachedLocation = (bool)loc_req_notif->isCachedLocation;

    bool isImpossibleScenario = false;
    // we need to reject various impossible scenarios
    if (notification.inEmergencyMode) {
        if (notification.protocolStack != GNSS_NFW_CTRL_PLANE &&
            notification.protocolStack != GNSS_NFW_SUPL) {
            isImpossibleScenario = true;
            LOC_LOGe("inEmergencyMode is true, but protocolStack=%d",
                notification.protocolStack);
        }
        if (GNSS_NFW_REJECTED == notification.responseType) {
            isImpossibleScenario = true;
            LOC_LOGe("inEmergencyMode is true, but responseType is REJECTED");
        }
    // SEC : upload package name when proxy app permission is granted.
    //} else {
    }
    if ((gpsLockState & GNSS_CONFIG_GPS_LOCK_NFW_ALL) == 0) {
        // requestorId is "" for emergency
        strlcpy(notification.requestorId,
                loc_req_notif->requestorId,
                sizeof(notification.requestorId));

        const char* nfwClient[] = { "NFW_CLIENT_CP", "NFW_CLIENT_SUPL", "NFW_CLIENT_IMS",
                                    "NFW_CLIENT_SIM", "NFW_CLIENT_MDT", "NFW_CLIENT_TLOC",
                                    "NFW_CLIENT_OTHER", "NFW_CLIENT_RLOC", "NFW_CLIENT_V2X",
                                    "NFW_CLIENT_R1", "NFW_CLIENT_R2", "NFW_CLIENT_R3",
                                    "NFW_CLIENT_NTN" };
        char packageName[LOC_MAX_PARAM_STRING];

        // proxyAppPackageName is "" for emergency
        if (ContextBase::isFeatureSupported(LOC_SUPPORTED_FEATURE_MULTIPLE_ATTRIBUTION_APPS) &&
            loc_req_notif->protocolStack >= eQMI_LOC_CTRL_PLANE_V02 &&
            loc_req_notif->protocolStack <= eQMI_LOC_NTN_V02 &&
            eQMI_LOC_OTHER_V02 != loc_req_notif->protocolStack) {

            if (mPackageName[loc_req_notif->protocolStack].empty()) {
                const loc_param_s_type nfw_packages_table[] =
                {
                    {nfwClient[loc_req_notif->protocolStack], &packageName,  NULL, 's' },
                };
                UTIL_READ_CONF(LOC_PATH_GPS_CONF_STR, nfw_packages_table);
                mPackageName[loc_req_notif->protocolStack] = packageName;
            }
            strlcpy(notification.proxyAppPackageName,
                    mPackageName[loc_req_notif->protocolStack].c_str(),
                    sizeof(notification.proxyAppPackageName));
        } else {
            // we need any of the apps in this case, all should
            // be provisioned with the same package name
            // so we will use NFW_CLIENT_CP for simplicity
            if (mPackageName[eQMI_LOC_CTRL_PLANE_V02].empty()) {
                const loc_param_s_type nfw_packages_table[] =
                {
                    {nfwClient[eQMI_LOC_CTRL_PLANE_V02], &packageName,  NULL, 's' },
                };
                UTIL_READ_CONF(LOC_PATH_GPS_CONF_STR, nfw_packages_table);
                mPackageName[eQMI_LOC_CTRL_PLANE_V02] = packageName;
            }
            strlcpy(notification.proxyAppPackageName,
                    mPackageName[eQMI_LOC_CTRL_PLANE_V02].c_str(),
                    sizeof(notification.proxyAppPackageName));
        }
    }

    if (!isImpossibleScenario) {
        LOC_LOGv("OUT: proxyAppPackageName=%s"
                 " ,protocolStack=%d"
                 " ,otherProtocolStackName=%s"
                 " ,requestor=%d"
                 " ,requestorId=%s"
                 " ,responseType=%d"
                 " ,inEmergencyMode=%d"
                 " ,isCachedLocation=%u",
                 notification.proxyAppPackageName,
                 notification.protocolStack,
                 notification.otherProtocolStackName,
                 notification.requestor,
                 notification.requestorId,
                 notification.responseType,
                 notification.inEmergencyMode,
                 notification.isCachedLocation);

        LocApiBase::sendNfwNotification(notification);
    }
}

/* convert engine state report to loc eng format and send the converted
   report to loc eng */
void LocApiV02 :: reportEngineState (
    const qmiLocEventEngineStateIndMsgT_v02 *engine_state_ptr)
{
  struct MsgUpdateEngineState : public LocMsg {
      LocApiV02* mpLocApiV02;
      bool mEngineOn;
      inline MsgUpdateEngineState(LocApiV02* pLocApiV02, bool engineOn) :
                 LocMsg(), mpLocApiV02(pLocApiV02), mEngineOn(engineOn) {}
      inline virtual void proc() const {

          LOC_LOGi("current engine state %d, old engine state %d, in session %d",
                   mEngineOn, mpLocApiV02->mEngineOn, mpLocApiV02->mInSession);

          // During quick session stop/start, there is a chance of left
          // over GNSS measurement from previous session gets received at
          // current session, when this happens, we need to set
          // mFirstMeasurementOfSessionReceived to false, so, this measurement
          // will not be used to determine whether device is in full cycle or not
          if (mpLocApiV02->mEngineOn != mEngineOn) {
              mpLocApiV02->mFirstMeasurementOfSessionReceived = false;
          }
          // Call registerEventMask so that if qmi mask has changed,
          // it will register the new qmi mask to the modem
          mpLocApiV02->mEngineOn = mEngineOn;

          if (!mEngineOn) {
              for (auto resender : mpLocApiV02->mResenders) {
                  LOC_LOGV("%s:%d]: resend failed command.", __func__, __LINE__);
                  resender();
              }
              mpLocApiV02->mResenders.clear();
              // update the registration mask upon receiving Engine state off
              // as we have already flushed out the queue, and can un-register
              // the ENGINE_STATE event
              mpLocApiV02->registerEventMask();
          }
      }
  };

  if (eQMI_LOC_ENGINE_STATE_ON_V02 == engine_state_ptr->engineState) {
      sendMsg(new MsgUpdateEngineState(this, true));
  }
  else if (eQMI_LOC_ENGINE_STATE_OFF_V02 == engine_state_ptr->engineState) {
      sendMsg(new MsgUpdateEngineState(this, false));
      // < SEC_GPS
      if (mPendingSetParam) {
          requestSetSecGnssParams();
          LOC_LOGD("Set sec param for pending session");
          mPendingSetParam = false;
      }
      // SEC_GPS >
  }

}

/* convert fix session state report to loc eng format and send the converted
   report to loc eng */
void LocApiV02 :: reportFixSessionState (
    const qmiLocEventFixSessionStateIndMsgT_v02 *fix_session_state_ptr)
{
  LocGpsStatusValue status;
  LOC_LOGd("state = %d", fix_session_state_ptr->sessionState);

  status = LOC_GPS_STATUS_NONE;
  if (fix_session_state_ptr->sessionState == eQMI_LOC_FIX_SESSION_STARTED_V02)
  {
    status = LOC_GPS_STATUS_SESSION_BEGIN;
  }
  else if (fix_session_state_ptr->sessionState
           == eQMI_LOC_FIX_SESSION_FINISHED_V02)
  {
    status = LOC_GPS_STATUS_SESSION_END;
  }
  reportStatus(status);
}

/* convert and report an ATL request to loc engine */
void LocApiV02 :: reportAtlRequest(
  const qmiLocEventLocationServerConnectionReqIndMsgT_v02 * server_request_ptr)
{
  uint32_t connHandle = server_request_ptr->connHandle;

  if (server_request_ptr->requestType == eQMI_LOC_SERVER_REQUEST_OPEN_V02)
  {
    LocAGpsType agpsType = LOC_AGPS_TYPE_ANY;
#if 1 //SEC_GPS: set default value to SUPL because apnTypeMask_valid of old chipset is false
    LocApnTypeMask apnTypeMask = LOC_APN_TYPE_MASK_SUPL;
#else //QC original
    LocApnTypeMask apnTypeMask = 0;
#endif

    switch (server_request_ptr->wwanType)
    {
      case eQMI_LOC_WWAN_TYPE_INTERNET_V02:
        agpsType = LOC_AGPS_TYPE_WWAN_ANY;
        break;
      case eQMI_LOC_WWAN_TYPE_AGNSS_V02:
        agpsType = LOC_AGPS_TYPE_SUPL;
        break;
      case eQMI_LOC_WWAN_TYPE_AGNSS_EMERGENCY_V02:
        agpsType = LOC_AGPS_TYPE_SUPL_ES;
        break;
      default:
        agpsType = LOC_AGPS_TYPE_WWAN_ANY;
        break;
    }

    if (server_request_ptr->apnTypeMask_valid) {
        apnTypeMask = convertQmiLocApnTypeMask(server_request_ptr->apnTypeMask);
    }
    // < SEC_GPS
    else{
        if((agpsType == LOC_AGPS_TYPE_SUPL_ES) && (apnTypeMask == LOC_APN_TYPE_MASK_SUPL)){
            apnTypeMask = convertQmiLocApnTypeMask(QMI_LOC_APN_TYPE_MASK_EMERGENCY_V02);
        }
        LOC_LOGd("apnTypeMask_valid was false. Convert to agpsType=0x%X apnTypeMask=0x%X",agpsType, apnTypeMask);
    }
    // SEC_GPS >

    LOC_LOGd("handle=%d agpsType=0x%X apnTypeMask=0x%X",
        connHandle, agpsType, apnTypeMask);

    SubId agpsSubId = DEFAULT_SUB;
    if (server_request_ptr->subId_valid) {
        switch (server_request_ptr->subId) {
        case eQMI_LOC_SYS_MODEM_AS_ID_1_V02:
            agpsSubId = PRIMARY_SUB;
            break;
        case eQMI_LOC_SYS_MODEM_AS_ID_2_V02:
            agpsSubId = SECONDARY_SUB;
            break;
        case eQMI_LOC_SYS_MODEM_AS_ID_3_V02:
            agpsSubId = TERTIARY_SUB;
            break;
        default:
            agpsSubId = DEFAULT_SUB;
            break;
        }
    }
    LOC_LOGd("agpsSubId=%d", agpsSubId);
    requestATL(connHandle, agpsType, apnTypeMask, agpsSubId);
  }
  // service the ATL close request
  else if (server_request_ptr->requestType == eQMI_LOC_SERVER_REQUEST_CLOSE_V02)
  {
    releaseATL(connHandle);
  }
}

/* conver the NI report to loc eng format and send t loc engine */
void LocApiV02 :: reportNiRequest(
    const qmiLocEventNiNotifyVerifyReqIndMsgT_v02 *ni_req_ptr)
{
    GnssNiNotification notif = {};
    notif.messageEncoding = GNSS_NI_ENCODING_TYPE_NONE ;
    notif.requestorEncoding = GNSS_NI_ENCODING_TYPE_NONE;
    notif.timeoutResponse = GNSS_NI_RESPONSE_NO_RESPONSE;
    notif.timeout = LOC_NI_NO_RESPONSE_TIME;

    /* Handle Vx request */
    if (1 == ni_req_ptr->NiVxInd_valid) {
        const qmiLocNiVxNotifyVerifyStructT_v02 *vx_req = &(ni_req_ptr->NiVxInd);

        notif.type = GNSS_NI_TYPE_VOICE;

        // Requestor ID, the requestor id recieved is NULL terminated
        hexcode(notif.requestor, sizeof notif.requestor,
                (char *)vx_req->requestorId, vx_req->requestorId_len );
       // < SEC_GPS
       /* convert encodings */
       if (strncmp(getSalesCode(), "CTC", 3)==0) {   //KHEE convert requestor ID encoding to UTF8
            notif.messageEncoding = GNSS_NI_ENCODING_TYPE_UTF8;
       }
       // SEC_GPS >
    }

    /* Handle UMTS CP request*/
    else if (1 == ni_req_ptr->NiUmtsCpInd_valid) {
        const qmiLocNiUmtsCpNotifyVerifyStructT_v02 *umts_cp_req =
            &ni_req_ptr->NiUmtsCpInd;

        notif.type = GNSS_NI_TYPE_CONTROL_PLANE;

        /* notificationText should always be a NULL terminated string */
        hexcode(notif.message, sizeof notif.message,
                (char *)umts_cp_req->notificationText,
                umts_cp_req->notificationText_len);

        /* Store requestor ID */
        hexcode(notif.requestor, sizeof(notif.requestor),
                (char *)umts_cp_req->requestorId.codedString,
                umts_cp_req->requestorId.codedString_len);

        /* convert encodings */
        notif.messageEncoding = convertNiEncoding(umts_cp_req->dataCodingScheme);

        notif.requestorEncoding =
            convertNiEncoding(umts_cp_req->requestorId.dataCodingScheme);

        /* LCS address (using extras field) */
        if (0 != umts_cp_req->clientAddress_len) {
            char lcs_addr[32]; // Decoded LCS address for UMTS CP NI

            // Copy LCS Address into notif.extras in the format: Address = 012345
            strlcat(notif.extras, LOC_NI_NOTIF_KEY_ADDRESS, sizeof (notif.extras));
            strlcat(notif.extras, " = ", sizeof notif.extras);
            int addr_len = 0;
            const char *address_source = NULL;
            address_source = (char *)umts_cp_req->clientAddress;
            // client Address is always NULL terminated
            addr_len = decodeAddress(lcs_addr, sizeof(lcs_addr), address_source,
                                     umts_cp_req->clientAddress_len);

            // The address is ASCII string
            if (addr_len) {
                strlcat(notif.extras, lcs_addr, sizeof notif.extras);
            }
        }
    } else if (1 == ni_req_ptr->NiSuplInd_valid) {
        const qmiLocNiSuplNotifyVerifyStructT_v02 *supl_req =
            &ni_req_ptr->NiSuplInd;

        notif.type = GNSS_NI_TYPE_SUPL;

        // Client name
        if (supl_req->valid_flags & QMI_LOC_SUPL_CLIENT_NAME_MASK_V02) {
            hexcode(notif.message, sizeof(notif.message),
                    (char *)supl_req->clientName.formattedString,
                    supl_req->clientName.formattedString_len);
            LOC_LOGv("SUPL NI: client_name: %s \n", notif.message);
        } else {
            LOC_LOGv("SUPL NI: client_name not present.");
        }

        // Requestor ID
        if (supl_req->valid_flags & QMI_LOC_SUPL_REQUESTOR_ID_MASK_V02) {
            hexcode(notif.requestor, sizeof notif.requestor,
                    (char*)supl_req->requestorId.formattedString,
                    supl_req->requestorId.formattedString_len);

            LOC_LOGv("SUPL NI: requestor: %s", notif.requestor);
        } else {
            LOC_LOGv("SUPL NI: requestor not present.");
        }

        // Encoding type
        if (supl_req->valid_flags & QMI_LOC_SUPL_DATA_CODING_SCHEME_MASK_V02) {
            notif.messageEncoding = convertNiEncoding(supl_req->dataCodingScheme);
            notif.requestorEncoding = convertNiEncoding(supl_req->dataCodingScheme);
        } else {
            notif.messageEncoding = notif.requestorEncoding = GNSS_NI_ENCODING_TYPE_NONE;
        }

        // ES SUPL
        if (1 == ni_req_ptr->suplEmergencyNotification_valid) {
            notif.type = GNSS_NI_TYPE_EMERGENCY_SUPL;
        }
    } //ni_req_ptr->NiSuplInd_valid == 1
    else {
        LOC_LOGe("unknown request event");
        return;
    }

    // Set default_response & notify_flags
    convertNiNotifyVerifyType(&notif, ni_req_ptr->notificationType);

    qmiLocEventNiNotifyVerifyReqIndMsgT_v02 *ni_req_copy_ptr =
        (qmiLocEventNiNotifyVerifyReqIndMsgT_v02 *)malloc(sizeof(*ni_req_copy_ptr));

    LocInEmergency emergencyState = ni_req_ptr->isInEmergencySession_valid ?
            (ni_req_ptr->isInEmergencySession ? LOC_IN_EMERGENCY_SET : LOC_IN_EMERGENCY_NOT_SET) :
            LOC_IN_EMERGENCY_UNKNOWN;
    if (NULL != ni_req_copy_ptr) {
        memcpy(ni_req_copy_ptr, ni_req_ptr, sizeof(*ni_req_copy_ptr));
        requestNiNotify(notif, (const void*)ni_req_copy_ptr, emergencyState);
    } else {
        LOC_LOGe("Error copying NI request");
    }
}

/* If Confidence value is less than 68%, then scale the accuracy value to
   68%.confidence.*/
void LocApiV02 :: scaleAccuracyTo68PercentConfidence(
        const uint8_t confidenceValue, LocGpsLocation &gpsLocation,
        const bool isCircularUnc)
{
  if (confidenceValue < 68)
  {
    // Circular uncertainty is at 63%.confidence. Scale factor should be
    // 1.072(from 63% -> 68%)
    uint8_t realConfidence = (isCircularUnc) ? 63:confidenceValue;
    // get scaling value based on 2D% confidence scaling table
    for (uint8_t iter = 0; iter < CONF_SCALER_ARRAY_MAX; iter++)
    {
      if (realConfidence <= confScalers[iter].confidence)
      {
        LOC_LOGA("Confidence: %d, Scaler value:%f",
                realConfidence,confScalers[iter].scaler_to_68);
        gpsLocation.accuracy *= confScalers[iter].scaler_to_68;
        break;
      }
    }
  }
}

/* Report the Xtra Server Url from the modem to HAL*/
void LocApiV02 :: reportXtraServerUrl(
                const qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02*
                server_request_ptr)
{

  if (server_request_ptr->serverList.serverList_len == 1)
  {
    reportXtraServer(server_request_ptr->serverList.serverList[0].serverUrl,
                     "",
                     "",
                     QMI_LOC_MAX_SERVER_ADDR_LENGTH_V02);
  }
  else if (server_request_ptr->serverList.serverList_len == 2)
  {
    reportXtraServer(server_request_ptr->serverList.serverList[0].serverUrl,
                     server_request_ptr->serverList.serverList[1].serverUrl,
                     "",
                     QMI_LOC_MAX_SERVER_ADDR_LENGTH_V02);
  }
  else
  {
    reportXtraServer(server_request_ptr->serverList.serverList[0].serverUrl,
                     server_request_ptr->serverList.serverList[1].serverUrl,
                     server_request_ptr->serverList.serverList[2].serverUrl,
                     QMI_LOC_MAX_SERVER_ADDR_LENGTH_V02);
  }

}

/* convert Ni Encoding type from QMI_LOC to loc eng format */
GnssNiEncodingType LocApiV02 ::convertNiEncoding(
  qmiLocNiDataCodingSchemeEnumT_v02 loc_encoding)
{
   GnssNiEncodingType enc = GNSS_NI_ENCODING_TYPE_NONE;

   switch (loc_encoding)
   {
     case eQMI_LOC_NI_SUPL_UTF8_V02:
       enc = GNSS_NI_ENCODING_TYPE_UTF8;
       break;
     case eQMI_LOC_NI_SUPL_UCS2_V02:
       enc = GNSS_NI_ENCODING_TYPE_UCS2;
       break;
     case eQMI_LOC_NI_SUPL_GSM_DEFAULT_V02:
       enc = GNSS_NI_ENCODING_TYPE_GSM_DEFAULT;
       break;
     case eQMI_LOC_NI_SS_LANGUAGE_UNSPEC_V02:
       enc = GNSS_NI_ENCODING_TYPE_GSM_DEFAULT; // SS_LANGUAGE_UNSPEC = GSM
       break;
     default:
       break;
   }

   return enc;
}

/*convert NI notify verify type from QMI LOC to loc eng format*/
bool LocApiV02 :: convertNiNotifyVerifyType (
  GnssNiNotification *notif,
  qmiLocNiNotifyVerifyEnumT_v02 notif_priv)
{
  switch (notif_priv)
   {
   case eQMI_LOC_NI_USER_NO_NOTIFY_NO_VERIFY_V02:
      notif->options = 0;
      break;

   case eQMI_LOC_NI_USER_NOTIFY_ONLY_V02:
      notif->options = GNSS_NI_OPTIONS_NOTIFICATION_BIT;
      break;

   case eQMI_LOC_NI_USER_NOTIFY_VERIFY_ALLOW_NO_RESP_V02:
      notif->options = GNSS_NI_OPTIONS_NOTIFICATION_BIT | GNSS_NI_OPTIONS_VERIFICATION_BIT;
      notif->timeoutResponse = GNSS_NI_RESPONSE_ACCEPT;
      break;

   case eQMI_LOC_NI_USER_NOTIFY_VERIFY_NOT_ALLOW_NO_RESP_V02:
      notif->options = GNSS_NI_OPTIONS_NOTIFICATION_BIT | GNSS_NI_OPTIONS_VERIFICATION_BIT;
      notif->timeoutResponse = GNSS_NI_RESPONSE_DENY;
      break;

   case eQMI_LOC_NI_USER_NOTIFY_VERIFY_PRIVACY_OVERRIDE_V02:
      notif->options = GNSS_NI_OPTIONS_PRIVACY_OVERRIDE_BIT;
      break;

   default:
      return false;
   }

   return true;
}

/* convert and report GNSS measurement data to loc eng */
void LocApiV02::reportGnssMeasurementData(
  const qmiLocEventGnssSvMeasInfoIndMsgT_v02& gnss_measurement_report_ptr)
{
    static uint32_t prevRefFCount = 0;
    static bool newMeasProcessed = false;
    uint8_t maxSubSeqNum = 0;
    uint8_t subSeqNum = 0;

    if (gnss_measurement_report_ptr.maxSubSeqNum_valid &&
        gnss_measurement_report_ptr.subSeqNum_valid) {
        maxSubSeqNum = gnss_measurement_report_ptr.maxSubSeqNum;
        subSeqNum = gnss_measurement_report_ptr.subSeqNum;
    } else {
        maxSubSeqNum = 0;
        subSeqNum = 0;
    }

    LOC_LOGd("[SvMeas] nHz (%d, %d), SeqNum: %d, MaxMsgNum: %d, "
             "SubSeqNum: %d, MaxSubMsgNum: %d, "
             "SvSystem: %d SignalType: %" PRIu64 " MeasValid: %d, #of SV: %d",
             gnss_measurement_report_ptr.nHzMeasurement_valid,
             gnss_measurement_report_ptr.nHzMeasurement,
             gnss_measurement_report_ptr.seqNum,
             gnss_measurement_report_ptr.maxMessageNum,
             subSeqNum,
             maxSubSeqNum,
             gnss_measurement_report_ptr.system,
             gnss_measurement_report_ptr.gnssSignalType,
             gnss_measurement_report_ptr.svMeasurement_valid,
             gnss_measurement_report_ptr.svMeasurement_len);
    if (!mGnssMeasurements) {
        mGnssMeasurements = (GnssMeasurements*)malloc(sizeof(GnssMeasurements));
        if (!mGnssMeasurements) {
            LOC_LOGe("Malloc failed to allocate heap memory for mGnssMeasurements");
            return;
        }
        resetSvMeasurementReport();
        newMeasProcessed = false;
    }

    if (gnss_measurement_report_ptr.seqNum > gnss_measurement_report_ptr.maxMessageNum ||
        subSeqNum > maxSubSeqNum) {
        LOC_LOGe("Invalid seqNum or subSeqNum, do not proceed");
        return;
    }

    GnssMeasurementsNotification& measData = mGnssMeasurements->gnssMeasNotification;
    // in case the measurement with seqNum of 1 is dropped, we will use ref count
    // to reset the measurement
    if ((gnss_measurement_report_ptr.seqNum == 1 && subSeqNum <= 1) ||
        (gnss_measurement_report_ptr.systemTimeExt_valid &&
         gnss_measurement_report_ptr.systemTimeExt.refFCount != prevRefFCount)) {
        // we have received some valid info since we last reported when
        // seq num matches with max seq num
        if (true == newMeasProcessed) {
            LOC_LOGe ("report due to seq number jump");
            reportSvMeasurementInternal();
            resetSvMeasurementReport();
            newMeasProcessed = false;
        }

        mHlosQtimer1 = getQTimerTickCount();
        mRefFCount = gnss_measurement_report_ptr.systemTimeExt.refFCount;
        prevRefFCount = gnss_measurement_report_ptr.systemTimeExt.refFCount;
        if (gnss_measurement_report_ptr.nHzMeasurement_valid &&
            gnss_measurement_report_ptr.nHzMeasurement) {
            mGnssMeasurements->gnssSvMeasurementSet.isNhz = true;
            measData.isNhz = true;
        }
        mCounter++;
    }

    Gnss_LocSvSystemEnumType locSvSystemType =
        getLocApiSvSystemType(gnss_measurement_report_ptr.system);
    if (GNSS_LOC_SV_SYSTEM_UNKNOWN == locSvSystemType) {
        LOC_LOGi("Unknown sv system");
        return;
    }

    // set up indication that we have processed some new measurement
    newMeasProcessed = true;

    if (subSeqNum <= 1) {
        convertGnssMeasurementsHeader(locSvSystemType, gnss_measurement_report_ptr);
    }

    // check whether we have valid dgnss measurement
    bool validDgnssMeas = false;
    if ((gnss_measurement_report_ptr.dgnssSvMeasurement_valid) &&
        (gnss_measurement_report_ptr.dgnssSvMeasurement_len != 0)) {
        uint32_t totalSvMeasLen = 0;
        if (gnss_measurement_report_ptr.svMeasurement_valid) {
            totalSvMeasLen = gnss_measurement_report_ptr.svMeasurement_len;
            if (gnss_measurement_report_ptr.extSvMeasurement_valid) {
                totalSvMeasLen += gnss_measurement_report_ptr.extSvMeasurement_len;
            }
        }
        if (totalSvMeasLen == gnss_measurement_report_ptr.dgnssSvMeasurement_len) {
            validDgnssMeas = true;
        }
    }

    // check whether we have valid ML inference Data
    bool validMlInference = false;
    if ((gnss_measurement_report_ptr.mlInferSvMeasurement_valid) &&
        (gnss_measurement_report_ptr.mlInferSvMeasurement_len != 0)) {
        uint32_t totalSvMeasLen = 0;
        if (gnss_measurement_report_ptr.svMeasurement_valid) {
            totalSvMeasLen = gnss_measurement_report_ptr.svMeasurement_len;
            if (gnss_measurement_report_ptr.extSvMeasurement_valid) {
                totalSvMeasLen += gnss_measurement_report_ptr.extSvMeasurement_len;
            }
        }
        if (totalSvMeasLen == gnss_measurement_report_ptr.mlInferSvMeasurement_len) {
            validMlInference = true;
        }
    }

    // number of measurements
    if (gnss_measurement_report_ptr.svMeasurement_valid) {
        if (gnss_measurement_report_ptr.svMeasurement_len != 0 &&
            gnss_measurement_report_ptr.svMeasurement_len <= QMI_LOC_SV_MEAS_LIST_MAX_SIZE_V02) {
            for (uint32_t index = 0; index < gnss_measurement_report_ptr.svMeasurement_len &&
                    measData.count < GNSS_MEASUREMENTS_MAX; index++) {
                // convert refreshed measurements and save to measurementsNotify's array
                if ((gnss_measurement_report_ptr.svMeasurement[index].validMeasStatusMask &
                     QMI_LOC_MASK_MEAS_STATUS_GNSS_FRESH_MEAS_STAT_BIT_VALID_V02) &&
                    (gnss_measurement_report_ptr.svMeasurement[index].measurementStatus &
                         QMI_LOC_MASK_MEAS_STATUS_GNSS_FRESH_MEAS_VALID_V02) &&
                     !(gnss_measurement_report_ptr.svMeasurement[index].measurementStatus &
                         MEAS_STATUS_DONT_USE)) {
                     convertGnssMeasurements(gnss_measurement_report_ptr,
                                             index, false, validDgnssMeas, validMlInference);
                     measData.count++;
                } else {
                    LOC_LOGd("Measurements are stale, do not report");
                }
            }

            /* now check if more measurements are available (some constellations such
            as BDS have more measurements available in extSvMeasurement)
            */
            if (gnss_measurement_report_ptr.extSvMeasurement_valid &&
                gnss_measurement_report_ptr.extSvMeasurement_len != 0 &&
                gnss_measurement_report_ptr.extSvMeasurement_len <=
                    QMI_LOC_EXT_SV_MEAS_LIST_MAX_SIZE_V02) {
                // the array of measurements
                for (uint32_t index = 0; index < gnss_measurement_report_ptr.extSvMeasurement_len
                        && measData.count < GNSS_MEASUREMENTS_MAX;
                        index++) {
                    // convert refreshed measurements and save to measurementsNotify's array
                    if ((gnss_measurement_report_ptr.extSvMeasurement[index].validMeasStatusMask &
                         QMI_LOC_MASK_MEAS_STATUS_GNSS_FRESH_MEAS_STAT_BIT_VALID_V02) &&
                        (gnss_measurement_report_ptr.extSvMeasurement[index].measurementStatus &
                            QMI_LOC_MASK_MEAS_STATUS_GNSS_FRESH_MEAS_VALID_V02) &&
                        !(gnss_measurement_report_ptr.svMeasurement[index].measurementStatus &
                            MEAS_STATUS_DONT_USE)) {
                        convertGnssMeasurements(gnss_measurement_report_ptr,
                                                index, true, validDgnssMeas, validMlInference);
                        measData.count++;
                    }
                    else {
                        LOC_LOGa("Measurements are stale, do not report");
                    }
                }
            }
        }
    }

    // the GPS clock time reading
    if (mPreferredSignalType == gnss_measurement_report_ptr.gnssSignalType &&
        subSeqNum <= 1 &&
        false == mPreferredSignalTypeReceived) {
            mPreferredSignalTypeReceived = true;
            convertGnssClock(measData.clock, gnss_measurement_report_ptr);
    }
    // AGC
    uint32_t temp;
    uint32_t agcCount = measData.agcCount;
    convertJammerIndicator(gnss_measurement_report_ptr,
                           measData.gnssAgc[agcCount].agcLevelDb,
                           temp, false);
    convertSvType(gnss_measurement_report_ptr,
                  measData.gnssAgc[agcCount].svType);

    if (gnss_measurement_report_ptr.gnssSignalType_valid) {
        measData.gnssAgc[agcCount].carrierFrequencyHz =
               convertSignalTypeToCarrierFrequency(gnss_measurement_report_ptr.gnssSignalType, 8);
    } else {
        measData.gnssAgc[agcCount].carrierFrequencyHz =
            CarrierFrequencies[measData.gnssAgc[agcCount].svType];
    }
    LOC_LOGa("agcCount = %d, agcLevelDb = %.2f, svType = %d, carrierFrequencyHz = %.2f",
             agcCount, measData.gnssAgc[agcCount].agcLevelDb,
             measData.gnssAgc[agcCount].svType,
             measData.gnssAgc[agcCount].carrierFrequencyHz);
    measData.agcCount++;
    if (gnss_measurement_report_ptr.maxMessageNum == gnss_measurement_report_ptr.seqNum &&
            !mGnssMeasurements->gnssSvMeasurementSet.isNhz) {
        // Copy only required information
        GnssMeasurementsNotification &measInfo = measData;
        m1HzMeasurementsInfo.clock.flags = measInfo.clock.flags;
        m1HzMeasurementsInfo.clock.timeNs = measInfo.clock.timeNs;
        m1HzMeasurementsInfo.clock.fullBiasNs = measInfo.clock.fullBiasNs;
        // Clear previous measurement data.
        m1HzMeasurementsInfo.measurements.clear();
        for (int meas = 0; (meas < measInfo.count) && (meas < GNSS_MEASUREMENTS_MAX); meas++) {
            GnssBasicMeasurementsData measurement = {};
            measurement.svId = measInfo.measurements[meas].svId;
            measurement.gnssSignalType = measInfo.measurements[meas].gnssSignalType;
            m1HzMeasurementsInfo.measurements.push_back(std::move(measurement));
        }
    }
    if (gnss_measurement_report_ptr.maxMessageNum == gnss_measurement_report_ptr.seqNum &&
        maxSubSeqNum == subSeqNum) {
        int64_t elapsedRealTime = -1;
        int64_t unc;
        if (gnss_measurement_report_ptr.refCountTicks_valid &&
            gnss_measurement_report_ptr.refCountTicksUnc_valid) {
            /* deal with Qtimer for ElapsedRealTimeNanos */
            elapsedRealTime = RealtimeEstimator::getElapsedRealtimeQtimer(
                    gnss_measurement_report_ptr.refCountTicks);

            /* Uncertainty on HLOS time is 0, so the uncertainty of the difference
            is the uncertainty of the Qtimer in the modem
            Note that gnss_measurement_report_ptr.refCountTicksUncis in msec */
            unc = gnss_measurement_report_ptr.refCountTicksUnc * 1000000;
#ifdef PTP_SUPPORTED
            uint64_t elapsedgPTPTime = 0;
            /* deal with gPTP time */
            /* Fill PTP time corresponding to Time of generation of meas packet */
            if (mIsGptpInitialized) {
                bool gotMPQTickPtpTime = gptpGetPtpTimeFromQTimeTickCount(&elapsedgPTPTime,
                        gnss_measurement_report_ptr.refCountTicks);
                if (gotMPQTickPtpTime) {
                    measData.clock.flags |= GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_GPTP_TIME_BIT;
                    measData.clock.elapsedgPTPTime = elapsedgPTPTime;
                    measData.clock.elapsedgPTPTimeUnc = unc;
                    measData.clock.flags |= GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_GPTP_TIME_UNC_BIT;
                }
            }
#endif
        } else {
            //If Qtimer isn't valid, estimate the elapsedRealTime
            GnssMeasurementsNotification& in = mGnssMeasurements->gnssMeasNotification;
            const uint32_t UTC_TO_GPS_SECONDS = 315964800;
            int64_t measTimeNanos = (int64_t)in.clock.timeNs - (int64_t)in.clock.fullBiasNs
                    - (int64_t)in.clock.biasNs - (int64_t)in.clock.leapSecond * 1000000000
                    + (int64_t)UTC_TO_GPS_SECONDS * 1000000000;
            bool isCurDataTimeTrustable =
                    in.clock.flags & GNSS_MEASUREMENTS_CLOCK_FLAGS_LEAP_SECOND_BIT &&
                    in.clock.flags & GNSS_MEASUREMENTS_CLOCK_FLAGS_FULL_BIAS_BIT &&
                    in.clock.flags & GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_BIT &&
                    in.clock.flags & GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_UNCERTAINTY_BIT;
            elapsedRealTime = mMeasElapsedRealTimeCal.
                    getElapsedRealtimeEstimateNanos(measTimeNanos, isCurDataTimeTrustable, 0);
            unc = mMeasElapsedRealTimeCal.getElapsedRealtimeUncNanos();
        }

        if (-1 == elapsedRealTime) {
            elapsedRealTime = getBootTimeMilliSec() * 1000000;
            unc = mMeasElapsedRealTimeCal.getElapsedRealtimeUncNanos();
        }
        measData.clock.flags |= GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_REAL_TIME_BIT;
        measData.clock.elapsedRealTime = elapsedRealTime;
        measData.clock.flags |= GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_REAL_TIME_UNC_BIT;
        measData.clock.elapsedRealTimeUnc = unc;
        measData.isFullTracking = mIsFullTracking;

        //AGC Status
        GnssMeasurementsNotification& measurementsNotify = mGnssMeasurements->gnssMeasNotification;
        if (gnss_measurement_report_ptr.agcStatus_valid) {
            GnssRfBand bandType = getLocApiSvRfBand(gnss_measurement_report_ptr.gnssSignalType);
            switch (bandType) {
                case GNSS_RF_BAND_L1:
                    measurementsNotify.agcStatusL1 = convertQmiAgcStatusType(
                            gnss_measurement_report_ptr.agcStatus);
                    break;
                case GNSS_RF_BAND_L2:
                    measurementsNotify.agcStatusL2 = convertQmiAgcStatusType(
                            gnss_measurement_report_ptr.agcStatus);
                    break;
                case GNSS_RF_BAND_L5:
                    measurementsNotify.agcStatusL5 = convertQmiAgcStatusType(
                            gnss_measurement_report_ptr.agcStatus);
                    break;
            }
        }
        LOC_LOGv("agcStatusL1: %d, agcStatusL2: %d, agcStatusL5: %d",
                measurementsNotify.agcStatusL1, measurementsNotify.agcStatusL2,
                measurementsNotify.agcStatusL5);

        reportSvMeasurementInternal();
        resetSvMeasurementReport();
        // set up flag to indicate that no new info in mGnssMeasurements
        newMeasProcessed = false;
    }
}

void LocApiV02::setGnssBiasesForL1CA() {
    GnssMeasurementsData* measData;
    uint64_t tempFlag, tempFlagUnc;

    for (uint32_t i = 0; i < mGnssMeasurements->gnssMeasNotification.count; i++) {
        measData = &mGnssMeasurements->gnssMeasNotification.measurements[i];
        switch (measData->gnssSignalType) {
        case GNSS_SIGNAL_GPS_L1CA:
            measData->fullInterSignalBiasNs = 0.0;
            measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            measData->fullInterSignalBiasUncertaintyNs = 0.0;
            measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            break;

        case GNSS_SIGNAL_GPS_L2:
        case GNSS_SIGNAL_QZSS_L2:
            if (mTimeBiases.flags & BIAS_GPSL1_GPSL2C_VALID) {
                measData->fullInterSignalBiasNs = mTimeBiases.gpsL1_gpsL2c;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (mTimeBiases.flags & BIAS_GPSL1_GPSL2C_UNC_VALID) {
                measData->fullInterSignalBiasUncertaintyNs = mTimeBiases.gpsL1_gpsL2cUnc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_GPS_L5:
        case GNSS_SIGNAL_QZSS_L5:
            if (mTimeBiases.flags & BIAS_GPSL1_GPSL5_VALID) {
                measData->fullInterSignalBiasNs = -mTimeBiases.gpsL1_gpsL5;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (mTimeBiases.flags & BIAS_GPSL1_GPSL5_UNC_VALID) {
                measData->fullInterSignalBiasUncertaintyNs = mTimeBiases.gpsL1_gpsL5Unc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_GLONASS_G1:
            if (mTimeBiases.flags & BIAS_GPSL1_GLOG1_VALID) {
                measData->fullInterSignalBiasNs = -mTimeBiases.gpsL1_gloG1;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (mTimeBiases.flags & BIAS_GPSL1_GLOG1_UNC_VALID) {
                measData->fullInterSignalBiasUncertaintyNs = mTimeBiases.gpsL1_gloG1Unc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_GALILEO_E1:
            if (mTimeBiases.flags & BIAS_GPSL1_GALE1_VALID) {
                measData->fullInterSignalBiasNs = -mTimeBiases.gpsL1_galE1;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (mTimeBiases.flags & BIAS_GPSL1_GALE1_UNC_VALID) {
                measData->fullInterSignalBiasUncertaintyNs = mTimeBiases.gpsL1_galE1Unc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_GALILEO_E5A:
            tempFlag = BIAS_GPSL1_VALID | BIAS_GALE1_VALID | BIAS_GALE1_GALE5A_VALID;
            tempFlagUnc =
                    BIAS_GPSL1_UNC_VALID | BIAS_GALE1_UNC_VALID | BIAS_GALE1_GALE5A_UNC_VALID;
            if (tempFlag == (mTimeBiases.flags & tempFlag)) {
                measData->fullInterSignalBiasNs =
                        -mTimeBiases.gpsL1 + mTimeBiases.galE1 - mTimeBiases.galE1_galE5a;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (tempFlagUnc == (mTimeBiases.flags & tempFlagUnc)) {
                measData->fullInterSignalBiasUncertaintyNs =
                        mTimeBiases.gpsL1Unc + mTimeBiases.galE1Unc + mTimeBiases.galE1_galE5aUnc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_GALILEO_E5B:
            tempFlag = BIAS_GPSL1_VALID | BIAS_GALE1_VALID | BIAS_GALE1_GALE5B_VALID;
            tempFlagUnc =
                    BIAS_GPSL1_UNC_VALID | BIAS_GALE1_UNC_VALID | BIAS_GALE1_GALE5B_UNC_VALID;
            if (tempFlag == (mTimeBiases.flags & tempFlag)) {
                measData->fullInterSignalBiasNs =
                        -mTimeBiases.gpsL1 + mTimeBiases.galE1 + mTimeBiases.galE1_galE5b;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (tempFlagUnc == (mTimeBiases.flags & tempFlagUnc)) {
                measData->fullInterSignalBiasUncertaintyNs =
                        mTimeBiases.gpsL1Unc + mTimeBiases.galE1Unc + mTimeBiases.galE1_galE5bUnc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_BEIDOU_B1I:
            if (mTimeBiases.flags & BIAS_GPSL1_BDSB1_VALID) {
                measData->fullInterSignalBiasNs = -mTimeBiases.gpsL1_bdsB1;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (mTimeBiases.flags & BIAS_GPSL1_BDSB1_UNC_VALID) {
                measData->fullInterSignalBiasUncertaintyNs = mTimeBiases.gpsL1_bdsB1Unc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_QZSS_L1CA:
        case GNSS_SIGNAL_QZSS_L1S:
        case GNSS_SIGNAL_SBAS_L1:
            if (mTimeBiases.flags & BIAS_GPSL1_VALID) {
                measData->fullInterSignalBiasNs = 0;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (mTimeBiases.flags & BIAS_GPSL1_UNC_VALID) {
                measData->fullInterSignalBiasUncertaintyNs = 0;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_BEIDOU_B1C:
            tempFlag = BIAS_GPSL1_VALID | BIAS_BDSB1_VALID | BIAS_BDSB1_BDSB1C_VALID;
            tempFlagUnc =
                    BIAS_GPSL1_UNC_VALID | BIAS_BDSB1_UNC_VALID | BIAS_BDSB1_BDSB1C_UNC_VALID;
            if (tempFlag == (mTimeBiases.flags & tempFlag)) {
                measData->fullInterSignalBiasNs =
                        -mTimeBiases.gpsL1 + mTimeBiases.bdsB1 - mTimeBiases.bdsB1_bdsB1c;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (tempFlagUnc == (mTimeBiases.flags & tempFlagUnc)) {
                measData->fullInterSignalBiasUncertaintyNs =
                        mTimeBiases.gpsL1Unc + mTimeBiases.bdsB1Unc + mTimeBiases.bdsB1_bdsB1cUnc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_NAVIC_L5:
            if (mTimeBiases.flags & BIAS_GPSL1_NAVIC_VALID) {
                measData->fullInterSignalBiasNs = -mTimeBiases.gpsL1_navic;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (mTimeBiases.flags & BIAS_GPSL1_NAVIC_UNC_VALID) {
                measData->fullInterSignalBiasUncertaintyNs = mTimeBiases.gpsL1_navicUnc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_BEIDOU_B2AQ:
            tempFlag = BIAS_GPSL1_VALID | BIAS_BDSB1_VALID | BIAS_BDSB1_BDSB2A_VALID;
            tempFlagUnc =
                    BIAS_GPSL1_UNC_VALID | BIAS_BDSB1_UNC_VALID | BIAS_BDSB1_BDSB2A_UNC_VALID;
            if (tempFlag == (mTimeBiases.flags & tempFlag)) {
                measData->fullInterSignalBiasNs =
                        -mTimeBiases.gpsL1 + mTimeBiases.bdsB1 - mTimeBiases.bdsB1_bdsB2a;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (tempFlagUnc == (mTimeBiases.flags & tempFlagUnc)) {
                measData->fullInterSignalBiasUncertaintyNs =
                        mTimeBiases.gpsL1Unc + mTimeBiases.bdsB1Unc + mTimeBiases.bdsB1_bdsB2aUnc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;
        case GNSS_SIGNAL_BEIDOU_B2BI:
            tempFlag = BIAS_GPSL1_VALID | BIAS_BDSB1_VALID | BIAS_BDSB1_BDSB2BI_VALID;
            tempFlagUnc =
                    BIAS_GPSL1_UNC_VALID | BIAS_BDSB1_UNC_VALID | BIAS_BDSB1_BDSB2BI_UNC_VALID;
            if (tempFlag == (mTimeBiases.flags & tempFlag)) {
                measData->fullInterSignalBiasNs =
                        -mTimeBiases.gpsL1 + mTimeBiases.bdsB1 + mTimeBiases.bdsB1_bdsB2bi;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (tempFlagUnc == (mTimeBiases.flags & tempFlagUnc)) {
                measData->fullInterSignalBiasUncertaintyNs =
                        mTimeBiases.gpsL1Unc + mTimeBiases.bdsB1Unc + mTimeBiases.bdsB1_bdsB2biUnc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;
        /* not supported */
        case GNSS_SIGNAL_GPS_L1C:
        case GNSS_SIGNAL_GLONASS_G2:
        case GNSS_SIGNAL_BEIDOU_B2I:
        case GNSS_SIGNAL_BEIDOU_B2AI:
        default:
            break;
        }
    }
}

void LocApiV02::setGnssBiasesForB1I() {
    GnssMeasurementsData* measData;
    uint64_t tempFlag, tempFlagUnc;

    for (uint32_t i = 0; i < mGnssMeasurements->gnssMeasNotification.count; i++) {
        measData = &mGnssMeasurements->gnssMeasNotification.measurements[i];
        switch (measData->gnssSignalType) {
        case GNSS_SIGNAL_GPS_L1CA:
            tempFlag = BIAS_GPSL1_VALID | BIAS_BDSB1_VALID;
            tempFlagUnc = BIAS_GPSL1_UNC_VALID | BIAS_BDSB1_UNC_VALID;
            if (tempFlag == (mTimeBiases.flags & tempFlag)) {
                measData->fullInterSignalBiasNs = mTimeBiases.gpsL1 - mTimeBiases.bdsB1;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (tempFlagUnc == (mTimeBiases.flags & tempFlagUnc)) {
                measData->fullInterSignalBiasUncertaintyNs =
                        mTimeBiases.gpsL1Unc + mTimeBiases.bdsB1Unc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_GLONASS_G1:
            tempFlag = BIAS_GLOG1_VALID | BIAS_BDSB1_VALID;
            tempFlagUnc = BIAS_GLOG1_UNC_VALID | BIAS_BDSB1_UNC_VALID;
            if (tempFlag == (mTimeBiases.flags & tempFlag)) {
                measData->fullInterSignalBiasNs = mTimeBiases.gloG1 - mTimeBiases.bdsB1;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (tempFlagUnc == (mTimeBiases.flags & tempFlagUnc)) {
                measData->fullInterSignalBiasUncertaintyNs =
                        mTimeBiases.gloG1Unc + mTimeBiases.bdsB1Unc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_GALILEO_E1:
            tempFlag = BIAS_GALE1_VALID | BIAS_BDSB1_VALID;
            tempFlagUnc = BIAS_GALE1_UNC_VALID | BIAS_BDSB1_UNC_VALID;
            if (tempFlag == (mTimeBiases.flags & tempFlag)) {
                measData->fullInterSignalBiasNs = mTimeBiases.galE1 - mTimeBiases.bdsB1;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (tempFlagUnc == (mTimeBiases.flags & tempFlagUnc)) {
                measData->fullInterSignalBiasUncertaintyNs =
                        mTimeBiases.galE1Unc + mTimeBiases.bdsB1Unc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_GPS_L5:
            tempFlag = BIAS_BDSB1_VALID | BIAS_GPSL1_VALID | BIAS_GPSL1_GPSL5_VALID;
            tempFlagUnc =
                    BIAS_BDSB1_UNC_VALID | BIAS_GPSL1_UNC_VALID | BIAS_GPSL1_GPSL5_UNC_VALID;
            if (tempFlag == (mTimeBiases.flags & tempFlag)) {
                measData->fullInterSignalBiasNs =
                        -mTimeBiases.bdsB1 + mTimeBiases.gpsL1 - mTimeBiases.gpsL1_gpsL5;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (tempFlagUnc == (mTimeBiases.flags & tempFlagUnc)) {
                measData->fullInterSignalBiasUncertaintyNs =
                        mTimeBiases.bdsB1Unc + mTimeBiases.gpsL1Unc + mTimeBiases.gpsL1_gpsL5Unc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_GALILEO_E5A:
            tempFlag = BIAS_BDSB1_VALID | BIAS_GALE1_VALID | BIAS_GALE1_GALE5A_VALID;
            tempFlagUnc =
                    BIAS_BDSB1_UNC_VALID | BIAS_GALE1_UNC_VALID | BIAS_GALE1_GALE5A_UNC_VALID;
            if (tempFlag == (mTimeBiases.flags & tempFlag)) {
                measData->fullInterSignalBiasNs =
                        -mTimeBiases.bdsB1 + mTimeBiases.galE1 - mTimeBiases.galE1_galE5a;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (tempFlagUnc == (mTimeBiases.flags & tempFlagUnc)) {
                measData->fullInterSignalBiasUncertaintyNs =
                        mTimeBiases.bdsB1Unc + mTimeBiases.galE1Unc + mTimeBiases.galE1_galE5aUnc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_GALILEO_E5B:
            tempFlag = BIAS_BDSB1_VALID | BIAS_GALE1_VALID | BIAS_GALE1_GALE5B_VALID;
            tempFlagUnc =
                    BIAS_BDSB1_UNC_VALID | BIAS_GALE1_UNC_VALID | BIAS_GALE1_GALE5B_UNC_VALID;
            if (tempFlag == (mTimeBiases.flags & tempFlag)) {
                measData->fullInterSignalBiasNs =
                        -mTimeBiases.bdsB1 + mTimeBiases.galE1 + mTimeBiases.galE1_galE5b;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (tempFlagUnc == (mTimeBiases.flags & tempFlagUnc)) {
                measData->fullInterSignalBiasUncertaintyNs =
                        mTimeBiases.bdsB1Unc + mTimeBiases.galE1Unc + mTimeBiases.galE1_galE5bUnc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_BEIDOU_B1I:
            measData->fullInterSignalBiasNs = 0.0;
            measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            measData->fullInterSignalBiasUncertaintyNs = 0.0;
            measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            break;

        case GNSS_SIGNAL_BEIDOU_B1C:
            if (mTimeBiases.flags & BIAS_BDSB1_BDSB1C_VALID) {
                measData->fullInterSignalBiasNs = -mTimeBiases.bdsB1_bdsB1c;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (mTimeBiases.flags & BIAS_BDSB1_BDSB1C_UNC_VALID) {
                measData->fullInterSignalBiasUncertaintyNs = mTimeBiases.bdsB1_bdsB1cUnc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_BEIDOU_B2AQ:
            if (mTimeBiases.flags & BIAS_BDSB1_BDSB2A_VALID) {
                measData->fullInterSignalBiasNs = -mTimeBiases.bdsB1_bdsB2a;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (mTimeBiases.flags & BIAS_BDSB1_BDSB2A_UNC_VALID) {
                measData->fullInterSignalBiasUncertaintyNs = mTimeBiases.bdsB1_bdsB2aUnc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        case GNSS_SIGNAL_BEIDOU_B2BI:
            if (mTimeBiases.flags & BIAS_BDSB1_BDSB2BI_VALID) {
                measData->fullInterSignalBiasNs = mTimeBiases.bdsB1_bdsB2bi;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT;
            }
            if (mTimeBiases.flags & BIAS_BDSB1_BDSB2BI_UNC_VALID) {
                measData->fullInterSignalBiasUncertaintyNs = mTimeBiases.bdsB1_bdsB2biUnc;
                measData->flags |= GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT;
            }
            break;

        default:
            break;
        }
    }
}

void LocApiV02::setGnssBiases() {
    switch (mPreferredSignalType) {
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L1CA_V02:
        setGnssBiasesForL1CA();
        break;
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B1_I_V02:
        setGnssBiasesForB1I();
        break;
    default:
        LOC_LOGe("Wrong mPreferredSignalType %" PRIi64 " ", mPreferredSignalType);
        break;
    }
}

void LocApiV02 ::reportSvMeasurementInternal() {

    if (mGnssMeasurements) {
        if (mGnssMeasurements->gnssSvMeasurementSet.isNhz) {
            mPrevNhzSlipCountMap = mCurrentCycleSlipCountMapNHz;
            //Clear current epoch data
            mCurrentCycleSlipCountMapNHz.clear();
        } else {
            mPrev1HzSlipCountMap = mCurrentCycleSlipCountMap1Hz;
            //Clear current epoch data
            mCurrentCycleSlipCountMap1Hz.clear();
        }

        mGnssMeasurements->gnssSvMeasurementSet.svMeasCount = mGnssMeasurements->gnssMeasNotification.count;
        GnssSvMeasurementHeader &svMeasSetHead =
            mGnssMeasurements->gnssSvMeasurementSet.svMeasSetHeader;

        // when we received the last sequence, timestamp the packet with AP time
        struct timespec apTimestamp = {};
        if (clock_gettime(CLOCK_BOOTTIME, &apTimestamp)== 0) {
            svMeasSetHead.apBootTimeStamp.apTimeStamp.tv_sec = apTimestamp.tv_sec;
            svMeasSetHead.apBootTimeStamp.apTimeStamp.tv_nsec = apTimestamp.tv_nsec;

            // use the time uncertainty configured in gps.conf
            svMeasSetHead.apBootTimeStamp.apTimeStampUncertaintyMs =
                    (float)ap_timestamp_uncertainty;
            svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_AP_TIMESTAMP;
        } else {
            svMeasSetHead.apBootTimeStamp.apTimeStampUncertaintyMs = FLT_MAX;
            LOC_LOGe("Error in clock_gettime() ");
        }

        setGnssBiases();
        LOC_LOGd("report %d sv in sv meas",
                 mGnssMeasurements->gnssMeasNotification.count);

        for (uint32_t i = 0; i < mGnssMeasurements->gnssMeasNotification.count; i++) {
           LOC_LOGa("measurements[%d].flags=0x%08x "
                    "measurements[%d].gnssSignalType=%d "
                    "measurements[%d].fullInterSignalBiasNs=%.2f "
                    "measurements[%d].fullInterSignalBiasUncertaintyNs=%.2f",
                    i, mGnssMeasurements->gnssMeasNotification.measurements[i].flags,
                    i, mGnssMeasurements->gnssMeasNotification.measurements[i].gnssSignalType,
                    i, mGnssMeasurements->gnssMeasNotification.
                            measurements[i].fullInterSignalBiasNs,
                    i, mGnssMeasurements->gnssMeasNotification.
                            measurements[i].fullInterSignalBiasUncertaintyNs);
        }
        LocApiBase::reportGnssMeasurements(*mGnssMeasurements);
    }
}

void LocApiV02::convertJammerIndicator(
        const qmiLocEventGnssSvMeasInfoIndMsgT_v02& gnss_measurement_report_ptr,
        double& agcLevelDb,
        GnssMeasurementsDataFlagsMask& flags,
        bool updateFlags) {

    if (gnss_measurement_report_ptr.jammerIndicator_valid) {
        if (GNSS_INVALID_JAMMER_IND !=
            gnss_measurement_report_ptr.jammerIndicator.bpMetricDb) {
            LOC_LOGa("AGC is valid: agcMetricDb = %d",
                -gnss_measurement_report_ptr.jammerIndicator.bpMetricDb);

            agcLevelDb = -(double)gnss_measurement_report_ptr.jammerIndicator.bpMetricDb / 100.0;
            if (updateFlags) {
                flags |= GNSS_MEASUREMENTS_DATA_AUTOMATIC_GAIN_CONTROL_BIT;
            }
        } else {
            LOC_LOGa("AGC is invalid: bpMetricDb = 0x%X",
                gnss_measurement_report_ptr.jammerIndicator.bpMetricDb);
        }
    }
    else {
        LOC_LOGa("AGC is not present");
    }
}

void LocApiV02::convertSvType(
        const qmiLocEventGnssSvMeasInfoIndMsgT_v02& gnss_measurement_report_ptr,
        GnssSvType& svType) {
    switch (gnss_measurement_report_ptr.system)
    {
    case eQMI_LOC_SV_SYSTEM_GPS_V02:
        svType = GNSS_SV_TYPE_GPS;
        break;

    case eQMI_LOC_SV_SYSTEM_GALILEO_V02:
        svType = GNSS_SV_TYPE_GALILEO;
        break;

    case eQMI_LOC_SV_SYSTEM_SBAS_V02:
        svType = GNSS_SV_TYPE_SBAS;
        break;

    case eQMI_LOC_SV_SYSTEM_GLONASS_V02:
        svType = GNSS_SV_TYPE_GLONASS;
        break;

    case eQMI_LOC_SV_SYSTEM_BDS_V02:
        svType = GNSS_SV_TYPE_BEIDOU;
        break;

    case eQMI_LOC_SV_SYSTEM_QZSS_V02:
        svType = GNSS_SV_TYPE_QZSS;
        break;

    case eQMI_LOC_SV_SYSTEM_NAVIC_V02:
        svType = GNSS_SV_TYPE_NAVIC;
        break;

    default:
        svType = GNSS_SV_TYPE_UNKNOWN;
        break;
    }
}

void LocApiV02::convertGnssMeasurementsHeader(const Gnss_LocSvSystemEnumType locSvSystemType,
    const qmiLocEventGnssSvMeasInfoIndMsgT_v02& gnss_measurement_info)
{
    GnssSvMeasurementHeader &svMeasSetHead =
        mGnssMeasurements->gnssSvMeasurementSet.svMeasSetHeader;

    // The refCountTicks for each constellation sent for one meas report is the same
    // always. It does not matter if it gets overwritten.
    if (gnss_measurement_info.refCountTicks_valid) {
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_REF_COUNT_TICKS;
        svMeasSetHead.refCountTicks = gnss_measurement_info.refCountTicks;
    }
    if (gnss_measurement_info.refCountTicksUnc_valid) {
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_REF_COUNT_TICKS_UNC;
        svMeasSetHead.refCountTicksUnc = gnss_measurement_info.refCountTicksUnc;
    }

    // clock frequency
    if (1 == gnss_measurement_info.rcvrClockFrequencyInfo_valid) {
        svMeasSetHead.clockFreq.size = sizeof(Gnss_LocRcvrClockFrequencyInfoStructType);
        svMeasSetHead.clockFreq.clockDrift =
            gnss_measurement_info.rcvrClockFrequencyInfo.clockDrift;
        svMeasSetHead.clockFreq.clockDriftUnc =
            gnss_measurement_info.rcvrClockFrequencyInfo.clockDriftUnc;
        svMeasSetHead.clockFreq.sourceOfFreq = (Gnss_LocSourceofFreqEnumType)
            gnss_measurement_info.rcvrClockFrequencyInfo.sourceOfFreq;

        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_CLOCK_FREQ;
        LOC_LOGa("FreqInfo:: Drift: %f, DriftUnc: %f, sourceoffreq: %d",
            svMeasSetHead.clockFreq.clockDrift, svMeasSetHead.clockFreq.clockDriftUnc,
            svMeasSetHead.clockFreq.sourceOfFreq);
    }

    if ((1 == gnss_measurement_info.leapSecondInfo_valid) &&
        (0 == gnss_measurement_info.leapSecondInfo.leapSecUnc)) {
        svMeasSetHead.leapSec.size = sizeof(Gnss_LeapSecondInfoStructType);
        svMeasSetHead.leapSec.leapSec = gnss_measurement_info.leapSecondInfo.leapSec;
        svMeasSetHead.leapSec.leapSecUnc = gnss_measurement_info.leapSecondInfo.leapSecUnc;

        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_LEAP_SECOND;
        LOC_LOGa("leapSecondInfo:: leapSec: %d, leapSecUnc: %d",
            svMeasSetHead.leapSec.leapSec, svMeasSetHead.leapSec.leapSecUnc);
    }

    if (1 == gnss_measurement_info.gpsGloInterSystemBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
            (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.gpsGloInterSystemBias;

        getInterSystemTimeBias("gpsGloInterSystemBias",
            svMeasSetHead.gpsGloInterSystemBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_GPS_GLO_INTER_SYSTEM_BIAS;

        if (gnss_measurement_info.gpsGloInterSystemBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_VALID_V02) {
            mTimeBiases.gpsL1_gloG1 =
                    -gnss_measurement_info.gpsGloInterSystemBias.timeBias * 1000000;
            mTimeBiases.flags |= BIAS_GPSL1_GLOG1_VALID;
        }
        if (gnss_measurement_info.gpsGloInterSystemBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_UNC_VALID_V02) {
            mTimeBiases.gpsL1_gloG1Unc =
                    gnss_measurement_info.gpsGloInterSystemBias.timeBiasUnc * 1000000;
            mTimeBiases.flags |= BIAS_GPSL1_GLOG1_UNC_VALID;
        }
    }

    if (1 == gnss_measurement_info.gpsBdsInterSystemBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.gpsBdsInterSystemBias;

        getInterSystemTimeBias("gpsBdsInterSystemBias",
                                svMeasSetHead.gpsBdsInterSystemBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_GPS_BDS_INTER_SYSTEM_BIAS;

        if (gnss_measurement_info.gpsBdsInterSystemBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_VALID_V02) {
            mTimeBiases.gpsL1_bdsB1 =
                    -gnss_measurement_info.gpsBdsInterSystemBias.timeBias * 1000000;
            mTimeBiases.flags |= BIAS_GPSL1_BDSB1_VALID;
        }
        if (gnss_measurement_info.gpsBdsInterSystemBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_UNC_VALID_V02) {
            mTimeBiases.gpsL1_bdsB1Unc =
                    gnss_measurement_info.gpsBdsInterSystemBias.timeBiasUnc * 1000000;
            mTimeBiases.flags |= BIAS_GPSL1_BDSB1_UNC_VALID;
        }
    }

    if (1 == gnss_measurement_info.gpsGalInterSystemBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.gpsGalInterSystemBias;

        getInterSystemTimeBias("gpsGalInterSystemBias",
                               svMeasSetHead.gpsGalInterSystemBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_GPS_GAL_INTER_SYSTEM_BIAS;

        if (gnss_measurement_info.gpsGalInterSystemBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_VALID_V02) {
            mTimeBiases.gpsL1_galE1 =
                    -gnss_measurement_info.gpsGalInterSystemBias.timeBias * 1000000;
            mTimeBiases.flags |= BIAS_GPSL1_GALE1_VALID;
        }
        if (gnss_measurement_info.gpsGalInterSystemBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_UNC_VALID_V02) {
            mTimeBiases.gpsL1_galE1Unc =
                    gnss_measurement_info.gpsGalInterSystemBias.timeBiasUnc * 1000000;
            mTimeBiases.flags |= BIAS_GPSL1_GALE1_UNC_VALID;
        }
    }

    if (1 == gnss_measurement_info.bdsGloInterSystemBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.bdsGloInterSystemBias;

        getInterSystemTimeBias("bdsGloInterSystemBias",
                               svMeasSetHead.bdsGloInterSystemBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_BDS_GLO_INTER_SYSTEM_BIAS;
    }

    if (1 == gnss_measurement_info.galGloInterSystemBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.galGloInterSystemBias;

        getInterSystemTimeBias("galGloInterSystemBias",
                                svMeasSetHead.galGloInterSystemBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_GAL_GLO_INTER_SYSTEM_BIAS;
    }

    if (1 == gnss_measurement_info.galBdsInterSystemBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.galBdsInterSystemBias;

        getInterSystemTimeBias("galBdsInterSystemBias",
                                svMeasSetHead.galBdsInterSystemBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_GAL_BDS_INTER_SYSTEM_BIAS;
    }

    if (1 == gnss_measurement_info.gpsNavicInterSystemBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.gpsNavicInterSystemBias;

        getInterSystemTimeBias("gpsNavicInterSystemBias",
                                svMeasSetHead.gpsNavicInterSystemBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_GPS_NAVIC_INTER_SYSTEM_BIAS;

        if (gnss_measurement_info.gpsNavicInterSystemBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_VALID_V02) {
            mTimeBiases.gpsL1_navic =
                    -gnss_measurement_info.gpsNavicInterSystemBias.timeBias * 1000000;
            mTimeBiases.flags |= BIAS_GPSL1_NAVIC_VALID;
        }
        if (gnss_measurement_info.gpsNavicInterSystemBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_UNC_VALID_V02) {
            mTimeBiases.gpsL1_navicUnc =
                    gnss_measurement_info.gpsNavicInterSystemBias.timeBiasUnc * 1000000;
            mTimeBiases.flags |= BIAS_GPSL1_NAVIC_UNC_VALID;
        }
    }

    if (1 == gnss_measurement_info.galNavicInterSystemBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.galNavicInterSystemBias;

        getInterSystemTimeBias("galNavicInterSystemBias",
                                svMeasSetHead.galNavicInterSystemBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_GAL_NAVIC_INTER_SYSTEM_BIAS;
    }

    if (1 == gnss_measurement_info.gloNavicInterSystemBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.gloNavicInterSystemBias;

        getInterSystemTimeBias("gloNavicInterSystemBias",
                                svMeasSetHead.gloNavicInterSystemBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_GLO_NAVIC_INTER_SYSTEM_BIAS;
    }

    if (1 == gnss_measurement_info.bdsNavicInterSystemBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.bdsNavicInterSystemBias;

        getInterSystemTimeBias("bdsNavicInterSystemBias",
                                svMeasSetHead.bdsNavicInterSystemBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_BDS_NAVIC_INTER_SYSTEM_BIAS;
    }

    if (1 == gnss_measurement_info.GpsL1L5TimeBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.GpsL1L5TimeBias;

        getInterSystemTimeBias("gpsL1L5TimeBias",
                                svMeasSetHead.gpsL1L5TimeBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_GPSL1L5_TIME_BIAS;

        if (gnss_measurement_info.GpsL1L5TimeBias.validMask & QMI_LOC_SYS_TIME_BIAS_VALID_V02) {
            mTimeBiases.gpsL1_gpsL5 = -gnss_measurement_info.GpsL1L5TimeBias.timeBias * 1000000;
            mTimeBiases.flags |= BIAS_GPSL1_GPSL5_VALID;
        }
        if (gnss_measurement_info.GpsL1L5TimeBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_UNC_VALID_V02) {
            mTimeBiases.gpsL1_gpsL5Unc =
                    gnss_measurement_info.GpsL1L5TimeBias.timeBiasUnc * 1000000;
            mTimeBiases.flags |= BIAS_GPSL1_GPSL5_UNC_VALID;
        }
    }

    if (1 == gnss_measurement_info.GalE1E5aTimeBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.GalE1E5aTimeBias;

        getInterSystemTimeBias("galE1E5aTimeBias",
                                svMeasSetHead.galE1E5aTimeBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_GALE1E5A_TIME_BIAS;

        if (gnss_measurement_info.GalE1E5aTimeBias.validMask & QMI_LOC_SYS_TIME_BIAS_VALID_V02) {
            mTimeBiases.galE1_galE5a = -gnss_measurement_info.GalE1E5aTimeBias.timeBias * 1000000;
            mTimeBiases.flags |= BIAS_GALE1_GALE5A_VALID;
        }
        if (gnss_measurement_info.GalE1E5aTimeBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_UNC_VALID_V02) {
            mTimeBiases.galE1_galE5aUnc =
                    gnss_measurement_info.GalE1E5aTimeBias.timeBiasUnc * 1000000;
            mTimeBiases.flags |= BIAS_GALE1_GALE5A_UNC_VALID;
        }
    }


    if (1 == gnss_measurement_info.BdsB1iB2aTimeBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.BdsB1iB2aTimeBias;

        getInterSystemTimeBias("bdsB1iB2aTimeBias",
                               svMeasSetHead.bdsB1iB2aTimeBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_BDSB1IB2A_TIME_BIAS;

        if (gnss_measurement_info.BdsB1iB2aTimeBias.validMask & QMI_LOC_SYS_TIME_BIAS_VALID_V02) {
            mTimeBiases.bdsB1_bdsB2a = -gnss_measurement_info.BdsB1iB2aTimeBias.timeBias * 1000000;
            mTimeBiases.flags |= BIAS_BDSB1_BDSB2A_VALID;
        }
        if (gnss_measurement_info.BdsB1iB2aTimeBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_UNC_VALID_V02) {
            mTimeBiases.bdsB1_bdsB2aUnc =
                    gnss_measurement_info.BdsB1iB2aTimeBias.timeBiasUnc * 1000000;
            mTimeBiases.flags |= BIAS_BDSB1_BDSB2A_UNC_VALID;
        }
    }

    if (1 == gnss_measurement_info.BdsB1iB2biTimeBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.BdsB1iB2biTimeBias;

        getInterSystemTimeBias("bdsB1iB2biTimeBias",
                               svMeasSetHead.bdsB1iB2biTimeBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_BDSB1IB2BI_TIME_BIAS;

        if (gnss_measurement_info.BdsB1iB2biTimeBias.validMask & QMI_LOC_SYS_TIME_BIAS_VALID_V02) {
            mTimeBiases.bdsB1_bdsB2bi = gnss_measurement_info.BdsB1iB2biTimeBias.timeBias * 1000000;
            mTimeBiases.flags |= BIAS_BDSB1_BDSB2BI_VALID;
        }
        if (gnss_measurement_info.BdsB1iB2biTimeBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_UNC_VALID_V02) {
            mTimeBiases.bdsB1_bdsB2biUnc =
                    gnss_measurement_info.BdsB1iB2biTimeBias.timeBiasUnc * 1000000;
            mTimeBiases.flags |= BIAS_BDSB1_BDSB2BI_UNC_VALID;
        }
    }


    if (1 == gnss_measurement_info.BdsB1iB1cTimeBias_valid) {
        if (gnss_measurement_info.BdsB1iB1cTimeBias.validMask & QMI_LOC_SYS_TIME_BIAS_VALID_V02) {
            mTimeBiases.bdsB1_bdsB1c = -gnss_measurement_info.BdsB1iB1cTimeBias.timeBias * 1000000;
            mTimeBiases.flags |= BIAS_BDSB1_BDSB1C_VALID;
        }
        if (gnss_measurement_info.BdsB1iB1cTimeBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_UNC_VALID_V02) {
            mTimeBiases.bdsB1_bdsB1cUnc =
                    gnss_measurement_info.BdsB1iB1cTimeBias.timeBiasUnc * 1000000;
            mTimeBiases.flags |= BIAS_BDSB1_BDSB1C_UNC_VALID;
        }
    }

    if (1 == gnss_measurement_info.GpsL1L2cTimeBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.GpsL1L2cTimeBias;

        getInterSystemTimeBias("gpsL1L2cTimeBias",
                               svMeasSetHead.gpsL1L2cTimeBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_GPSL1L2C_TIME_BIAS;
        if (gnss_measurement_info.GpsL1L2cTimeBias.validMask & QMI_LOC_SYS_TIME_BIAS_VALID_V02) {
            mTimeBiases.gpsL1_gpsL2c = gnss_measurement_info.GpsL1L2cTimeBias.timeBias * 1000000;
            mTimeBiases.flags |= BIAS_GPSL1_GPSL2C_VALID;
        }
        if (gnss_measurement_info.GpsL1L2cTimeBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_UNC_VALID_V02) {
            mTimeBiases.gpsL1_gpsL2cUnc =
                    gnss_measurement_info.GpsL1L2cTimeBias.timeBiasUnc * 1000000;
            mTimeBiases.flags |= BIAS_GPSL1_GPSL2C_UNC_VALID;
        }
    }

    if (1 == gnss_measurement_info.GloG1G2TimeBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.GloG1G2TimeBias;

        getInterSystemTimeBias("gloG1G2TimeBias",
                               svMeasSetHead.gloG1G2TimeBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_GLOG1G2_TIME_BIAS;
    }

    if (1 == gnss_measurement_info.BdsB1iB1cTimeBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.BdsB1iB1cTimeBias;

        getInterSystemTimeBias("bdsB1iB1cTimeBias",
                               svMeasSetHead.bdsB1iB1cTimeBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_BDSB1IB1C_TIME_BIAS;
    }

    if (1 == gnss_measurement_info.GalE1E5bTimeBias_valid) {
        qmiLocInterSystemBiasStructT_v02* interSystemBias =
                (qmiLocInterSystemBiasStructT_v02*)&gnss_measurement_info.GalE1E5bTimeBias;

        getInterSystemTimeBias("galE1E5bTimeBias",
                               svMeasSetHead.galE1E5bTimeBias, interSystemBias);
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_GALE1E5B_TIME_BIAS;
        if (gnss_measurement_info.GalE1E5bTimeBias.validMask & QMI_LOC_SYS_TIME_BIAS_VALID_V02) {
            mTimeBiases.galE1_galE5b = gnss_measurement_info.GalE1E5bTimeBias.timeBias * 1000000;
            mTimeBiases.flags |= BIAS_GALE1_GALE5B_VALID;
        }
        if (gnss_measurement_info.GalE1E5bTimeBias.validMask &
            QMI_LOC_SYS_TIME_BIAS_UNC_VALID_V02) {
            mTimeBiases.galE1_galE5bUnc =
                    gnss_measurement_info.GalE1E5bTimeBias.timeBiasUnc * 1000000;
            mTimeBiases.flags |= BIAS_GALE1_GALE5B_UNC_VALID;
        }
    }

    if (1 == gnss_measurement_info.gloTime_valid) {
        GnssGloTimeStructType & gloSystemTime = svMeasSetHead.gloSystemTime;

        gloSystemTime.gloFourYear = gnss_measurement_info.gloTime.gloFourYear;
        gloSystemTime.gloDays = gnss_measurement_info.gloTime.gloDays;
        gloSystemTime.gloMsec = gnss_measurement_info.gloTime.gloMsec;
        gloSystemTime.gloClkTimeBias = gnss_measurement_info.gloTime.gloClkTimeBias;
        gloSystemTime.gloClkTimeUncMs = gnss_measurement_info.gloTime.gloClkTimeUncMs;
        gloSystemTime.validityMask |= (GNSS_GLO_FOUR_YEAR_VALID |
            GNSS_CLO_DAYS_VALID |
            GNSS_GLO_MSEC_VALID |
            GNSS_GLO_CLK_TIME_BIAS_VALID |
            GNSS_GLO_CLK_TIME_BIAS_UNC_VALID);

        if (gnss_measurement_info.systemTimeExt_valid) {
            gloSystemTime.refFCount = gnss_measurement_info.systemTimeExt.refFCount;
            gloSystemTime.validityMask |= GNSS_GLO_REF_FCOUNT_VALID;
        }

        if (gnss_measurement_info.numClockResets_valid) {
            gloSystemTime.numClockResets = gnss_measurement_info.numClockResets;
            gloSystemTime.validityMask |= GNSS_GLO_NUM_CLOCK_RESETS_VALID;
        }
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_GLO_SYSTEM_TIME;

        mTimeBiases.gloG1 = gnss_measurement_info.gloTime.gloClkTimeBias * 1000000;
        mTimeBiases.gloG1Unc = gnss_measurement_info.gloTime.gloClkTimeUncMs * 1000000;
        mTimeBiases.flags |= BIAS_GLOG1_VALID;
        mTimeBiases.flags |= BIAS_GLOG1_UNC_VALID;
    }

    if ((1 == gnss_measurement_info.systemTime_valid) ||
        (1 == gnss_measurement_info.systemTimeExt_valid)) {

        GnssSystemTimeStructType* systemTimePtr = nullptr;
        Gnss_LocGnssTimeExtStructType* systemTimeExtPtr = nullptr;
        GpsSvMeasHeaderFlags systemTimeFlags = 0x0;
        GpsSvMeasHeaderFlags systemTimeExtFlags = 0x0;

        switch (locSvSystemType) {
        case GNSS_LOC_SV_SYSTEM_GPS:
            systemTimePtr = &svMeasSetHead.gpsSystemTime;
            systemTimeExtPtr = &svMeasSetHead.gpsSystemTimeExt;
            systemTimeFlags = GNSS_SV_MEAS_HEADER_HAS_GPS_SYSTEM_TIME;
            systemTimeExtFlags = GNSS_SV_MEAS_HEADER_HAS_GPS_SYSTEM_TIME_EXT;
            if (gnss_measurement_info.systemTime_valid) {
                mTimeBiases.gpsL1 = gnss_measurement_info.systemTime.systemClkTimeBias * 1000000;
                mTimeBiases.gpsL1Unc =
                        gnss_measurement_info.systemTime.systemClkTimeUncMs * 1000000;
                mTimeBiases.flags |= BIAS_GPSL1_VALID;
                mTimeBiases.flags |= BIAS_GPSL1_UNC_VALID;
            }
            break;
        case GNSS_LOC_SV_SYSTEM_GALILEO:
            systemTimePtr = &svMeasSetHead.galSystemTime;
            systemTimeExtPtr = &svMeasSetHead.galSystemTimeExt;
            systemTimeFlags = GNSS_SV_MEAS_HEADER_HAS_GAL_SYSTEM_TIME;
            systemTimeExtFlags = GNSS_SV_MEAS_HEADER_HAS_GAL_SYSTEM_TIME_EXT;
            if (gnss_measurement_info.systemTime_valid) {
                mTimeBiases.galE1 = gnss_measurement_info.systemTime.systemClkTimeBias * 1000000;
                mTimeBiases.galE1Unc =
                    gnss_measurement_info.systemTime.systemClkTimeUncMs * 1000000;
                mTimeBiases.flags |= BIAS_GALE1_VALID;
                mTimeBiases.flags |= BIAS_GALE1_UNC_VALID;
            }
            break;
        case GNSS_LOC_SV_SYSTEM_BDS:
            systemTimePtr = &svMeasSetHead.bdsSystemTime;
            systemTimeExtPtr = &svMeasSetHead.bdsSystemTimeExt;
            systemTimeFlags = GNSS_SV_MEAS_HEADER_HAS_BDS_SYSTEM_TIME;
            systemTimeExtFlags = GNSS_SV_MEAS_HEADER_HAS_BDS_SYSTEM_TIME_EXT;
            if (gnss_measurement_info.systemTime_valid) {
                mTimeBiases.bdsB1 = gnss_measurement_info.systemTime.systemClkTimeBias * 1000000;
                mTimeBiases.bdsB1Unc =
                    gnss_measurement_info.systemTime.systemClkTimeUncMs * 1000000;
                mTimeBiases.flags |= BIAS_BDSB1_VALID;
                mTimeBiases.flags |= BIAS_BDSB1_UNC_VALID;
            }
            break;
        case GNSS_LOC_SV_SYSTEM_QZSS:
            systemTimePtr = &svMeasSetHead.qzssSystemTime;
            systemTimeExtPtr = &svMeasSetHead.qzssSystemTimeExt;
            systemTimeFlags = GNSS_SV_MEAS_HEADER_HAS_QZSS_SYSTEM_TIME;
            systemTimeExtFlags = GNSS_SV_MEAS_HEADER_HAS_QZSS_SYSTEM_TIME_EXT;
            break;
        case GNSS_LOC_SV_SYSTEM_GLONASS:
            systemTimeExtPtr = &svMeasSetHead.gloSystemTimeExt;
            systemTimeExtFlags = GNSS_SV_MEAS_HEADER_HAS_GLO_SYSTEM_TIME_EXT;
            break;
        case GNSS_LOC_SV_SYSTEM_NAVIC:
            systemTimePtr = &svMeasSetHead.navicSystemTime;
            systemTimeExtPtr = &svMeasSetHead.navicSystemTimeExt;
            systemTimeFlags = GNSS_SV_MEAS_HEADER_HAS_NAVIC_SYSTEM_TIME;
            systemTimeExtFlags = GNSS_SV_MEAS_HEADER_HAS_NAVIC_SYSTEM_TIME_EXT;
            break;
        default:
            break;
        }

        if (systemTimePtr) {
            if (gnss_measurement_info.systemTime_valid) {
                systemTimePtr->systemWeek =
                    gnss_measurement_info.systemTime.systemWeek;
                systemTimePtr->systemMsec =
                    gnss_measurement_info.systemTime.systemMsec;
                systemTimePtr->systemClkTimeBias =
                    gnss_measurement_info.systemTime.systemClkTimeBias;
                systemTimePtr->systemClkTimeUncMs =
                    gnss_measurement_info.systemTime.systemClkTimeUncMs;
                systemTimePtr->validityMask |= (GNSS_SYSTEM_TIME_WEEK_VALID |
                    GNSS_SYSTEM_TIME_WEEK_MS_VALID |
                    GNSS_SYSTEM_CLK_TIME_BIAS_VALID |
                    GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID);
                svMeasSetHead.flags |= systemTimeFlags;
            }

            if (gnss_measurement_info.systemTimeExt_valid) {
                systemTimePtr->refFCount = gnss_measurement_info.systemTimeExt.refFCount;
                systemTimePtr->validityMask |= GNSS_SYSTEM_REF_FCOUNT_VALID;
                svMeasSetHead.flags |= systemTimeFlags;
            }

            if (gnss_measurement_info.numClockResets_valid) {
                systemTimePtr->numClockResets = gnss_measurement_info.numClockResets;
                systemTimePtr->validityMask |= GNSS_SYSTEM_NUM_CLOCK_RESETS_VALID;
                svMeasSetHead.flags |= systemTimeFlags;
            }
        }

        if (systemTimeExtPtr) {
            if (gnss_measurement_info.systemTimeExt_valid) {
                systemTimeExtPtr->size = sizeof(Gnss_LocGnssTimeExtStructType);

                systemTimeExtPtr->systemRtc_valid =
                    gnss_measurement_info.systemTimeExt.systemRtc_valid;
                systemTimeExtPtr->systemRtcMs =
                    gnss_measurement_info.systemTimeExt.systemRtcMs;

                svMeasSetHead.flags |= systemTimeExtFlags;
            }
        }
    }

    if (gnss_measurement_info.dgnssCorrectionSourceT_valid) {
        svMeasSetHead.dgnssCorrectionSourceType =
                (LocDgnssCorrectionSourceType)gnss_measurement_info.dgnssCorrectionSourceT;
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_DGNSS_CORRECTION_SOURCE_TYPE;
    }
    if (gnss_measurement_info.dgnssCorrectionSourceID_valid) {
        svMeasSetHead.dgnssCorrectionSourceID =
                gnss_measurement_info.dgnssCorrectionSourceID;
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_DGNSS_CORRECTION_SOURCE_ID;

    }
    if (gnss_measurement_info.dgnssRefStationId_valid) {
        svMeasSetHead.dgnssRefStationId =
                gnss_measurement_info.dgnssRefStationId;
        svMeasSetHead.flags |= GNSS_SV_MEAS_HEADER_HAS_DGNSS_REF_STATION_ID;
    }
}

/* convert and report ODCPI request */
void LocApiV02::requestOdcpi(const qmiLocEventWifiReqIndMsgT_v02& qmiReq)
{
    LOC_LOGd("ODCPI Request: requestType %d", qmiReq.requestType);

    OdcpiRequestInfo req = {};
    req.size = sizeof(OdcpiRequestInfo);

    if (eQMI_LOC_WIFI_START_PERIODIC_HI_FREQ_FIXES_V02 == qmiReq.requestType ||
            eQMI_LOC_WIFI_START_PERIODIC_KEEP_WARM_V02 == qmiReq.requestType) {
        req.type = ODCPI_REQUEST_TYPE_START;
    } else if (eQMI_LOC_WIFI_STOP_PERIODIC_FIXES_V02 == qmiReq.requestType){
        req.type = ODCPI_REQUEST_TYPE_STOP;
    } else {
        LOC_LOGe("Invalid request type");
        return;
    }

    if (qmiReq.e911Mode_valid) {
        req.isEmergencyMode = qmiReq.e911Mode == 1 ? true : false;
    }

    if (qmiReq.tbfInMs_valid) {
        req.tbfMillis = qmiReq.tbfInMs;
    }

    if (qmiReq.civicAddressNeeded_valid) {
        req.isCivicAddressRequired = qmiReq.civicAddressNeeded == 1 ? true : false;
    }

    LocApiBase::requestOdcpi(req);
}

/* report disaster and crisis message */
void LocApiV02 :: reportDcMessage(const qmiLocEventDcReportIndMsgT_v02* pDcReportIndMsg)
{
    if (pDcReportIndMsg->msgType_valid &&
            pDcReportIndMsg->numValidBits_valid && (pDcReportIndMsg->numValidBits > 0) &&
            pDcReportIndMsg->dcReportData_valid && (pDcReportIndMsg->dcReportData_len > 0)) {
        GnssDcReportInfo dcReportInfo = {};
        LOC_LOGi("dc report type %d, num bits %d, num bytes %d",
                 pDcReportIndMsg->msgType, (uint32_t)pDcReportIndMsg->numValidBits,
                 pDcReportIndMsg->dcReportData_len);

        switch (pDcReportIndMsg->msgType) {
        case eQMI_LOC_QZSS_JMA_DISASTER_PREVENTION_INFO_V02:
            dcReportInfo.dcReportType = QZSS_JMA_DISASTER_PREVENTION_INFO;
            break;
        case eQMI_LOC_QZSS_NON_JMA_DISASTER_PREVENTION_INFO_V02:
            dcReportInfo.dcReportType = QZSS_NON_JMA_DISASTER_PREVENTION_INFO;
            break;
        default:
            LOC_LOGe("unknown qmi dc report type: %d", pDcReportIndMsg->msgType);
            return;
        }
        dcReportInfo.numValidBits = pDcReportIndMsg->numValidBits;
        dcReportInfo.dcReportData.resize(pDcReportIndMsg->dcReportData_len);
        for (uint32_t i = 0; i < pDcReportIndMsg->dcReportData_len; i++) {
            dcReportInfo.dcReportData[i] = pDcReportIndMsg->dcReportData[i];
        }
        LocApiBase::reportDcMessage(dcReportInfo);
    }
}

void LocApiV02::processGnssBandsSupportedInd(
        const qmiLocGnssBandsSupportedIndMsgT_v02* pGnssBandsSupportedIndMsg) {
    LOC_LOGd("primaryGnssSignalType: %" PRIu64 ", supported signals: %" PRIu64,
            pGnssBandsSupportedIndMsg->primaryGnssSignalType,
            pGnssBandsSupportedIndMsg->gnssSupportedSignals);

    if (pGnssBandsSupportedIndMsg->primaryGnssSignalType_valid) {
        mPreferredSignalType = pGnssBandsSupportedIndMsg->primaryGnssSignalType;
    }

    if (pGnssBandsSupportedIndMsg->gnssSupportedSignals_valid) {
        GnssCapabNotification gnssCapabNotification = {};
        gnssCapabNotification.gnssSupportedSignals =
                convertQmiGnssSignalType(pGnssBandsSupportedIndMsg->gnssSupportedSignals);
        LOC_LOGd("supported signals in hal: 0x%x", gnssCapabNotification.gnssSupportedSignals);

        if (pGnssBandsSupportedIndMsg->gnssSupportedSignals_valid) {
            for (qmiLocGnssSignalTypeMaskT_v02 sig = QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L1CA_V02;
                 sig <= QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2B_Q_V02;
                 sig <<= 1) {

                if (pGnssBandsSupportedIndMsg->gnssSupportedSignals & sig) {
                    updateGnssCapabNotification(gnssCapabNotification, sig);
                }
            }
        }

        IF_LOC_LOGV {
            for (int i = 0; i < gnssCapabNotification.count; i++) {
                LOC_LOGv("cap[%d] sv type=%d freq=%.2f code type=%d",
                        i, gnssCapabNotification.gnssSignalType[i].svType,
                        gnssCapabNotification.gnssSignalType[i].carrierFrequencyHz,
                        gnssCapabNotification.gnssSignalType[i].codeType);
            }
        }

        LocApiBase::reportSignalTypeCapabilities(gnssCapabNotification);
    }
}

void LocApiV02::updateGnssCapabNotification(GnssCapabNotification& gnssCapabNotification,
                                            qmiLocGnssSignalTypeMaskT_v02 gnssSignalType) {

    switch (gnssSignalType) {
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L1CA_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L1C_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L2C_L_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L5_Q_V02:
        gnssCapabNotification.gnssSignalType[gnssCapabNotification.count].svType =
                GNSS_SV_TYPE_GPS;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GLONASS_G1_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GLONASS_G2_V02:
        gnssCapabNotification.gnssSignalType[gnssCapabNotification.count].svType =
                GNSS_SV_TYPE_GLONASS;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E1_C_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E5A_Q_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E5B_Q_V02:
        gnssCapabNotification.gnssSignalType[gnssCapabNotification.count].svType =
                GNSS_SV_TYPE_GALILEO;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B1_I_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B1C_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2_I_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2A_I_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2A_Q_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2B_I_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2B_Q_V02:
        gnssCapabNotification.gnssSignalType[gnssCapabNotification.count].svType =
                GNSS_SV_TYPE_BEIDOU;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L1CA_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L1S_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L2C_L_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L5_Q_V02:
        gnssCapabNotification.gnssSignalType[gnssCapabNotification.count].svType =
                GNSS_SV_TYPE_QZSS;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_SBAS_L1_CA_V02:
        gnssCapabNotification.gnssSignalType[gnssCapabNotification.count].svType =
                GNSS_SV_TYPE_SBAS;
        break;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_NAVIC_L5_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_NAVIC_L1_V02:
        gnssCapabNotification.gnssSignalType[gnssCapabNotification.count].svType =
                GNSS_SV_TYPE_NAVIC;
        break;
    }
    gnssCapabNotification.gnssSignalType[gnssCapabNotification.count].carrierFrequencyHz =
            convertSignalTypeToCarrierFrequency(gnssSignalType, 8);
    gnssCapabNotification.gnssSignalType[gnssCapabNotification.count].codeType =
            getCodeType(gnssSignalType);
    gnssCapabNotification.count++;
}

GnssMeasurementsCodeType LocApiV02::getCodeType(qmiLocGnssSignalTypeMaskT_v02 gnssSignalType) {

    switch (gnssSignalType) {
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L1CA_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GLONASS_G1_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GLONASS_G2_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E1_C_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L1CA_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_SBAS_L1_CA_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_NAVIC_L5_V02:
        return GNSS_MEASUREMENTS_CODE_TYPE_C;
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L1S_V02:
        return GNSS_MEASUREMENTS_CODE_TYPE_Z;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L2C_L_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L2C_L_V02:
        return GNSS_MEASUREMENTS_CODE_TYPE_L;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L5_Q_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E5A_Q_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E5B_Q_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L5_Q_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2B_Q_V02:
        return GNSS_MEASUREMENTS_CODE_TYPE_Q;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2B_I_V02:
        return GNSS_MEASUREMENTS_CODE_TYPE_D;
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B1_I_V02:
        return GNSS_MEASUREMENTS_CODE_TYPE_I;

    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B1C_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2A_Q_V02:
    // this one is not yet supported
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L1C_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_NAVIC_L1_V02:
        return GNSS_MEASUREMENTS_CODE_TYPE_P;

    /* no plan to support  */
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2_I_V02:
    case QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2A_I_V02:
    default:
        return GNSS_MEASUREMENTS_CODE_TYPE_OTHER;
    }
}

void LocApiV02::wifiStatusInformSync()
{
    qmiLocNotifyWifiStatusReqMsgT_v02 wifiStatusReq;
    memset(&wifiStatusReq, 0, sizeof(wifiStatusReq));
    wifiStatusReq.wifiStatus = eQMI_LOC_WIFI_STATUS_AVAILABLE_V02;

    LOC_LOGv("Informing wifi status available.");
    LOC_SEND_SYNC_REQ(NotifyWifiStatus, NOTIFY_WIFI_STATUS, wifiStatusReq);
}

#define FIRST_BDS_D2_SV_PRN 1
#define LAST_BDS_D2_SV_PRN  5
#define IS_BDS_GEO_SV(svId, gnssType) ( ((gnssType == GNSS_SV_TYPE_BEIDOU) && \
                                        (svId <= LAST_BDS_D2_SV_PRN) && \
                                        (svId >= FIRST_BDS_D2_SV_PRN)) ? true : false )

/*convert GnssMeasurement type from QMI LOC to loc eng format*/
void LocApiV02 :: convertGnssMeasurements(
    const qmiLocEventGnssSvMeasInfoIndMsgT_v02& gnss_measurement_report_ptr,
    int index, bool isExt, bool validDgnssSvMeas, bool validMlInference)
{
    const qmiLocSVMeasurementStructT_v02 &gnss_measurement_info = isExt ?
            gnss_measurement_report_ptr.extSvMeasurement[index] :
            gnss_measurement_report_ptr.svMeasurement[index];
    uint32_t count = mGnssMeasurements->gnssMeasNotification.count;

    GnssMeasurementsData& measurementData =
        mGnssMeasurements->gnssMeasNotification.measurements[count];

    Gnss_SVMeasurementStructType& svMeas =
        mGnssMeasurements->gnssSvMeasurementSet.svMeas[count];

    svMeas.size = sizeof(Gnss_SVMeasurementStructType);
    svMeas.gnssSystem = getLocApiSvSystemType(gnss_measurement_report_ptr.system);
    svMeas.gnssSvId = gnss_measurement_info.gnssSvId;
    svMeas.gloFrequency = gnss_measurement_info.gloFrequency;
    if (gnss_measurement_report_ptr.gnssSignalType_valid) {
        svMeas.gnssSignalTypeMask =
                convertQmiGnssSignalType(gnss_measurement_report_ptr.gnssSignalType);
    } else {
        svMeas.gnssSignalTypeMask =
                getDefaultGnssSignalTypeMask(gnss_measurement_report_ptr.system);
    }
    if (gnss_measurement_info.validMask & QMI_LOC_SV_LOSSOFLOCK_VALID_V02) {
        svMeas.lossOfLock = (bool)gnss_measurement_info.lossOfLock;
    }

    svMeas.svStatus = (Gnss_LocSvSearchStatusEnumT)gnss_measurement_info.svStatus;

    if (gnss_measurement_info.validMask & QMI_LOC_SV_HEALTH_VALID_V02) {
        svMeas.healthStatus_valid = 1;
        svMeas.healthStatus = (uint8_t)gnss_measurement_info.healthStatus;
    }

    svMeas.svInfoMask = (Gnss_LocSvInfoMaskT)gnss_measurement_info.svInfoMask;
    svMeas.CNo = gnss_measurement_info.CNo;
    //svMeas.gloRfLoss = gnss_measurement_info.gloRfLoss;
	svMeas.gloRfLoss = RF_LOSS; // SEC : compensate RF loss (fixed value)
    svMeas.measLatency = gnss_measurement_info.measLatency;

    // SVTimeSpeed
    svMeas.svTimeSpeed.size = sizeof(Gnss_LocSVTimeSpeedStructType);
    svMeas.svTimeSpeed.svMs = gnss_measurement_info.svTimeSpeed.svTimeMs;
    svMeas.svTimeSpeed.svSubMs = gnss_measurement_info.svTimeSpeed.svTimeSubMs;
    svMeas.svTimeSpeed.svTimeUncMs = gnss_measurement_info.svTimeSpeed.svTimeUncMs;
    svMeas.svTimeSpeed.dopplerShift = gnss_measurement_info.svTimeSpeed.dopplerShift;
    svMeas.svTimeSpeed.dopplerShiftUnc = gnss_measurement_info.svTimeSpeed.dopplerShiftUnc;

    svMeas.dopplerAccelValid = gnss_measurement_info.svTimeSpeed.dopplerAccel_valid;
    svMeas.dopplerAccel = gnss_measurement_info.svTimeSpeed.dopplerAccel;

    svMeas.validMeasStatusMask = gnss_measurement_info.validMeasStatusMask;
    qmiLocSvMeasStatusMaskT_v02 measStatus = gnss_measurement_info.measurementStatus;
    svMeas.measurementStatus = GNSS_LOC_MEAS_STATUS_NULL;
    // Convert qmiSvMeas.measurementStatus to svMeas.measurementStatus
    if (QMI_LOC_MASK_MEAS_STATUS_SM_VALID_V02 & measStatus) {
        svMeas.measurementStatus |= GNSS_LOC_MEAS_STATUS_SM_VALID;
    }
    if (QMI_LOC_MASK_MEAS_STATUS_SB_VALID_V02 & measStatus) {
        svMeas.measurementStatus |= GNSS_LOC_MEAS_STATUS_SB_VALID;
    }
    if (QMI_LOC_MASK_MEAS_STATUS_MS_VALID_V02 & measStatus) {
        svMeas.measurementStatus |= GNSS_LOC_MEAS_STATUS_MS_VALID;
    }
    if (QMI_LOC_MASK_MEAS_STATUS_BE_CONFIRM_V02 & measStatus) {
        svMeas.measurementStatus |= GNSS_LOC_MEAS_STATUS_BE_CONFIRM;
    }
    if (QMI_LOC_MASK_MEAS_STATUS_VELOCITY_VALID_V02 & measStatus) {
        svMeas.measurementStatus |= GNSS_LOC_MEAS_STATUS_VELOCITY_VALID;
    }
    if (QMI_LOC_MASK_MEAS_STATUS_VELOCITY_FINE_V02 & measStatus) {
        svMeas.measurementStatus |= GNSS_LOC_MEAS_STATUS_VELOCITY_FINE;
    }
    if (QMI_LOC_MASK_MEAS_STATUS_LP_VALID_V02 & measStatus) {
        svMeas.measurementStatus |= GNSS_LOC_MEAS_STATUS_LP_VALID;
    }
    if (QMI_LOC_MASK_MEAS_STATUS_LP_POS_VALID_V02 & measStatus) {
        svMeas.measurementStatus |= GNSS_LOC_MEAS_STATUS_LP_POS_VALID;
    }
    if (QMI_LOC_MASK_MEAS_STATUS_FROM_RNG_DIFF_V02 & measStatus) {
        svMeas.measurementStatus |= GNSS_LOC_MEAS_STATUS_FROM_RNG_DIFF;
    }
    if (QMI_LOC_MASK_MEAS_STATUS_FROM_VE_DIFF_V02 & measStatus) {
        svMeas.measurementStatus |= GNSS_LOC_MEAS_STATUS_FROM_VE_DIFF;
    }
    if (QMI_LOC_MASK_MEAS_STATUS_GNSS_FRESH_MEAS_VALID_V02 & measStatus) {
        svMeas.measurementStatus |= GNSS_LOC_MEAS_STATUS_GNSS_FRESH_MEAS;
    }
    if (gnss_measurement_info.validMask & QMI_LOC_SV_MULTIPATH_EST_VALID_V02) {
        svMeas.multipathEstValid = 1;
        svMeas.multipathEstimate = gnss_measurement_info.multipathEstimate;
    }

    if (gnss_measurement_info.validMask & QMI_LOC_SV_FINE_SPEED_VALID_V02) {
        svMeas.fineSpeedValid = 1;
        svMeas.fineSpeed = gnss_measurement_info.fineSpeed;
    }
    if (gnss_measurement_info.validMask & QMI_LOC_SV_FINE_SPEED_UNC_VALID_V02) {
        svMeas.fineSpeedUncValid = 1;
        svMeas.fineSpeedUnc = gnss_measurement_info.fineSpeedUnc;
    }
    if (gnss_measurement_info.validMask & QMI_LOC_SV_CARRIER_PHASE_VALID_V02) {
        svMeas.carrierPhaseValid = 1;
        svMeas.carrierPhase = gnss_measurement_info.carrierPhase;
    }
    if (gnss_measurement_info.validMask & QMI_LOC_SV_SV_DIRECTION_VALID_V02) {
        svMeas.svDirectionValid = 1;
        svMeas.svElevation = gnss_measurement_info.svElevation;
        svMeas.svAzimuth = gnss_measurement_info.svAzimuth;
    }
    if (gnss_measurement_info.validMask & QMI_LOC_SV_CYCLESLIP_COUNT_VALID_V02) {
        svMeas.cycleSlipCountValid = 1;
        svMeas.cycleSlipCount = gnss_measurement_info.cycleSlipCount;
    }

    if (validDgnssSvMeas) {
        int dgnssSvMeasIndex = index;
        // adjust the dgnss meas index if we are processing external SV measurement
        if (isExt) {
            dgnssSvMeasIndex += gnss_measurement_report_ptr.svMeasurement_len;
        }
        const qmiLocDgnssSVMeasurementStructT_v02* dgnss_sv_meas_ptr =
                &gnss_measurement_report_ptr.dgnssSvMeasurement[dgnssSvMeasIndex];
        svMeas.dgnssSvMeas.dgnssMeasStatus = (LocSvDgnssMeasStatusMask)
                dgnss_sv_meas_ptr->dgnssMeasStatus,
        svMeas.dgnssSvMeas.diffDataEpochTimeMsec =
                dgnss_sv_meas_ptr->diffDataEpochTimeMsec;
        svMeas.dgnssSvMeas.prCorrMeters =
                dgnss_sv_meas_ptr->prCorrMeters;
        svMeas.dgnssSvMeas.prrCorrMetersPerSec =
                dgnss_sv_meas_ptr->prrCorrMetersPerSec;
    }

    // Populate ML Inference data for GNSS HAL sturctures
    if (validMlInference) {
        int mlInferenceIndex = index;
        // Adjust index to support ML inference for Extended Meas
        if (isExt) {
            mlInferenceIndex += gnss_measurement_report_ptr.svMeasurement_len;
        }
        if (gnss_measurement_report_ptr.mlInferSvMeasurement[mlInferenceIndex].mlInfer_valid) {
            svMeas.mlInferSvMeasurement.prMlInferValid =
                gnss_measurement_report_ptr.mlInferSvMeasurement[mlInferenceIndex].mlInfer_valid;
            svMeas.mlInferSvMeasurement.prMlInfer =
                gnss_measurement_report_ptr.mlInferSvMeasurement[mlInferenceIndex].mlInfer;
        }
    }

    if (!isExt) {
        uint32_t svMeas_len = gnss_measurement_report_ptr.svMeasurement_len;
        uint32_t svCarPhUnc_len = gnss_measurement_report_ptr.svCarrierPhaseUncertainty_len;
        if ((1 == gnss_measurement_report_ptr.svCarrierPhaseUncertainty_valid) &&
            (svMeas_len == svCarPhUnc_len)) {
            svMeas.carrierPhaseUncValid = 1;
            svMeas.carrierPhaseUnc =
                    gnss_measurement_report_ptr.svCarrierPhaseUncertainty[index];
        }
    } else {
        uint32_t extSvMeas_len = gnss_measurement_report_ptr.extSvMeasurement_len;
        uint32_t extSvCarPhUnc_len = gnss_measurement_report_ptr.extSvCarrierPhaseUncertainty_len;
        if ((1 == gnss_measurement_report_ptr.extSvCarrierPhaseUncertainty_valid) &&
            (extSvMeas_len == extSvCarPhUnc_len)) {
            svMeas.carrierPhaseUncValid = 1;
            svMeas.carrierPhaseUnc =
                    gnss_measurement_report_ptr.extSvCarrierPhaseUncertainty[index];
        }
    }

    // size
    measurementData.size = sizeof(GnssMeasurementsData);

    // flag initiation
    measurementData.flags = 0;

    // svid
    measurementData.svId = gnss_measurement_info.gnssSvId;
    measurementData.flags |= GNSS_MEASUREMENTS_DATA_SV_ID_BIT | GNSS_MEASUREMENTS_DATA_SV_TYPE_BIT;

    // constellation
    convertSvType(gnss_measurement_report_ptr, measurementData.svType);

    if (GNSS_SV_TYPE_GLONASS == measurementData.svType) {
        measurementData.gloFrequency = gnss_measurement_info.gloFrequency;
        measurementData.flags |= GNSS_MEASUREMENTS_DATA_GLO_FREQUENCY_BIT;
    }

    measurementData.timeOffsetNs = 0.0;

    // stateMask & receivedSvTimeNs & received_gps_tow_uncertainty_ns
    uint64_t validMeasStatus = gnss_measurement_info.measurementStatus &
                               gnss_measurement_info.validMeasStatusMask;
    uint64_t bitSynMask = QMI_LOC_MASK_MEAS_STATUS_BE_CONFIRM_V02 |
                          QMI_LOC_MASK_MEAS_STATUS_SB_VALID_V02;
    double gpsTowUncNs = (double)gnss_measurement_info.svTimeSpeed.svTimeUncMs * 1e6;

    bool bBandNotAvailable = !gnss_measurement_report_ptr.gnssSignalType_valid;
    qmiLocGnssSignalTypeMaskT_v02 gnssBand;

    if (gnss_measurement_report_ptr.gnssSignalType_valid) {
        gnssBand = gnss_measurement_report_ptr.gnssSignalType;
    }

    measurementData.stateMask = GNSS_MEASUREMENTS_STATE_UNKNOWN_BIT;
    measurementData.receivedSvTimeNs = 0;
    measurementData.receivedSvTimeUncertaintyNs = 0;

    // Deal with Galileo first since it is special
    uint64_t galSVstateMask = 0;
    if (GNSS_SV_TYPE_GALILEO == measurementData.svType &&
        (bBandNotAvailable ||
        (QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E1_C_V02 == gnssBand))) {

        galSVstateMask = GNSS_MEASUREMENTS_STATE_GAL_E1BC_CODE_LOCK_BIT;

        if (gnss_measurement_info.measurementStatus &
            QMI_LOC_MASK_MEAS_STATUS_100MS_STAT_BIT_VALID_V02) {
            galSVstateMask |= GNSS_MEASUREMENTS_STATE_GAL_E1C_2ND_CODE_LOCK_BIT;
        }

        if (gnss_measurement_info.measurementStatus &
            QMI_LOC_MASK_MEAS_STATUS_2S_STAT_BIT_VALID_V02) {
            galSVstateMask |= GNSS_MEASUREMENTS_STATE_GAL_E1B_PAGE_SYNC_BIT;
        }
    }
    // now deal with all constellations
    bool bIsL5orE5 = false;
    // L5
    if (GNSS_SV_TYPE_GPS == measurementData.svType &&
        (bBandNotAvailable ||
        (QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L5_Q_V02 == gnssBand))) {
        bIsL5orE5 = true;
    }
    // E5
    if (GNSS_SV_TYPE_GALILEO == measurementData.svType &&
        (bBandNotAvailable ||
        (QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E5A_Q_V02 == gnssBand))) {
        bIsL5orE5 = true;
    }

    if (validMeasStatus & QMI_LOC_MASK_MEAS_STATUS_MS_VALID_V02) {
        /* sub-frame decode & TOW decode */
        measurementData.stateMask |= (GNSS_MEASUREMENTS_STATE_SUBFRAME_SYNC_BIT |
                                      GNSS_MEASUREMENTS_STATE_TOW_DECODED_BIT |
                                      GNSS_MEASUREMENTS_STATE_TOW_KNOWN_BIT |
                                      GNSS_MEASUREMENTS_STATE_BIT_SYNC_BIT |
                                      GNSS_MEASUREMENTS_STATE_CODE_LOCK_BIT);
        // GLO
        if (GNSS_SV_TYPE_GLONASS == measurementData.svType &&
            (bBandNotAvailable ||
            (QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GLONASS_G1_V02 == gnssBand))) {

            measurementData.stateMask |= (GNSS_MEASUREMENTS_STATE_GLO_TOD_KNOWN_BIT |
                                          GNSS_MEASUREMENTS_STATE_GLO_TOD_DECODED_BIT |
                                          GNSS_MEASUREMENTS_STATE_GLO_STRING_SYNC_BIT);
        }

        // GAL
        measurementData.stateMask |= galSVstateMask;

        // BDS
        if (IS_BDS_GEO_SV(measurementData.svId, measurementData.svType)) {
            /* BDS_GEO SV transmitting D2 signal */
            measurementData.stateMask |= (GNSS_MEASUREMENTS_STATE_BDS_D2_BIT_SYNC_BIT |
                                          GNSS_MEASUREMENTS_STATE_BDS_D2_SUBFRAME_SYNC_BIT);
        }

        // L5 or E5
        if (bIsL5orE5) {
            measurementData.stateMask |= GNSS_MEASUREMENTS_STATE_2ND_CODE_LOCK_BIT;
        }

        const qmiLocSVTimeSpeedStructT_v02 &svTimeSpeed = gnss_measurement_info.svTimeSpeed;
        double svTimeSubMsToNs = ((double)svTimeSpeed.svTimeSubMs) * NSEC_IN_MSEC;
        measurementData.receivedSvTimeNs =
                (int64_t)gnss_measurement_info.svTimeSpeed.svTimeMs * NSEC_IN_MSEC +
                (int64_t)svTimeSubMsToNs;
        measurementData.receivedSvTimeSubNs = svTimeSubMsToNs -
                (int64_t)svTimeSubMsToNs;

        measurementData.receivedSvTimeUncertaintyNs = (int64_t)gpsTowUncNs;

        measurementData.flags |= (GNSS_MEASUREMENTS_DATA_STATE_BIT |
                                  GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_BIT |
                                  GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_UNCERTAINTY_BIT);
    } else if ((validMeasStatus & bitSynMask) == bitSynMask) {
        /* bit sync */
        measurementData.stateMask |= (GNSS_MEASUREMENTS_STATE_BIT_SYNC_BIT |
                                      GNSS_MEASUREMENTS_STATE_SYMBOL_SYNC_BIT |
                                      GNSS_MEASUREMENTS_STATE_CODE_LOCK_BIT);
        measurementData.stateMask |= galSVstateMask;

        // L5 or E5
        if (bIsL5orE5) {
            measurementData.stateMask |= GNSS_MEASUREMENTS_STATE_2ND_CODE_LOCK_BIT;
        }
        double svTimeNs = fmod(((double)gnss_measurement_info.svTimeSpeed.svTimeMs +
                  (double)gnss_measurement_info.svTimeSpeed.svTimeSubMs), 20) * NSEC_IN_MSEC;
        measurementData.receivedSvTimeNs = (int64_t)svTimeNs;
        measurementData.receivedSvTimeSubNs = svTimeNs -(int64_t)svTimeNs;

        measurementData.receivedSvTimeUncertaintyNs = (int64_t)gpsTowUncNs;
        measurementData.flags |= (GNSS_MEASUREMENTS_DATA_STATE_BIT |
                                  GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_BIT |
                                  GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_UNCERTAINTY_BIT);
    } else if (validMeasStatus & QMI_LOC_MASK_MEAS_STATUS_SM_VALID_V02) {
        /* code lock */
        measurementData.stateMask |= GNSS_MEASUREMENTS_STATE_CODE_LOCK_BIT;
        measurementData.stateMask |= galSVstateMask;

        measurementData.receivedSvTimeNs =
             (int64_t)((double)gnss_measurement_info.svTimeSpeed.svTimeSubMs * 1e6);
        const qmiLocSVTimeSpeedStructT_v02 &svTimeSpeed = gnss_measurement_info.svTimeSpeed;
        double svTimeSubMsToNs = ((double)svTimeSpeed.svTimeSubMs) * NSEC_IN_MSEC;
        measurementData.receivedSvTimeNs = (int64_t)svTimeSubMsToNs;
        measurementData.receivedSvTimeSubNs = svTimeSubMsToNs -
                (int64_t)svTimeSubMsToNs;

        measurementData.receivedSvTimeUncertaintyNs = (int64_t)gpsTowUncNs;

        measurementData.flags |= (GNSS_MEASUREMENTS_DATA_STATE_BIT |
                                  GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_BIT |
                                  GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_UNCERTAINTY_BIT);
    }

    // carrierToNoiseDbHz
    measurementData.carrierToNoiseDbHz = gnss_measurement_info.CNo/10.0;
    measurementData.flags |= GNSS_MEASUREMENTS_DATA_CARRIER_TO_NOISE_BIT;
    
    if (QMI_LOC_MASK_MEAS_STATUS_VELOCITY_FINE_V02 ==
        (gnss_measurement_info.measurementStatus & QMI_LOC_MASK_MEAS_STATUS_VELOCITY_FINE_V02)) {

        // pseudorangeRateMps
        measurementData.pseudorangeRateMps = gnss_measurement_info.fineSpeed;

        // pseudorangeRateUncertaintyMps
        measurementData.pseudorangeRateUncertaintyMps = gnss_measurement_info.fineSpeedUnc;
    }
    else
    {
        // pseudorangeRateMps
        measurementData.pseudorangeRateMps = gnss_measurement_info.svTimeSpeed.dopplerShift;

        // pseudorangeRateUncertaintyMps
        measurementData.pseudorangeRateUncertaintyMps = gnss_measurement_info.svTimeSpeed.dopplerShiftUnc;
    }
    measurementData.flags |= (GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_BIT |
                              GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_UNCERTAINTY_BIT);

    // carrier frequency
    if (gnss_measurement_report_ptr.gnssSignalType_valid) {
        measurementData.carrierFrequencyHz = convertSignalTypeToCarrierFrequency(
                gnss_measurement_report_ptr.gnssSignalType, gnss_measurement_info.gloFrequency);
        measurementData.flags |= GNSS_MEASUREMENTS_DATA_CARRIER_FREQUENCY_BIT;
    } else {
        measurementData.carrierFrequencyHz = 0;
        // GLONASS is FDMA system, so each channel has its own carrier frequency
        // The formula is f(k) = fc + k * 0.5625;
        // This is applicable for GLONASS G1 only, where fc = 1602MHz
        if ((gnss_measurement_info.gloFrequency >= 1 && gnss_measurement_info.gloFrequency <= 14)) {
            measurementData.carrierFrequencyHz +=
                    (gnss_measurement_info.gloFrequency - 8) * 562500.0;
        }
        measurementData.carrierFrequencyHz += CarrierFrequencies[measurementData.svType];
        measurementData.flags |= GNSS_MEASUREMENTS_DATA_CARRIER_FREQUENCY_BIT;
    }

    // accumulatedDeltaRangeM
    if (gnss_measurement_info.validMask & QMI_LOC_SV_CARRIER_PHASE_VALID_V02) {
        measurementData.carrierPhase = gnss_measurement_info.carrierPhase;
        measurementData.flags |= GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_BIT;
        if ((validMeasStatus & QMI_LOC_MASK_MEAS_STATUS_LP_VALID_V02) &&
            (validMeasStatus & QMI_LOC_MASK_MEAS_STATUS_LP_POS_VALID_V02)) {
            measurementData.carrierPhase += 0.5;
        }
        measurementData.adrMeters =
            (SPEED_OF_LIGHT / measurementData.carrierFrequencyHz) * measurementData.carrierPhase;
        measurementData.flags |= GNSS_MEASUREMENTS_DATA_ADR_BIT;
    } else {
        measurementData.adrMeters = 0.0;
    }

    // carrierPhaseUncertainty
    if (1 == svMeas.carrierPhaseUncValid) {
        measurementData.carrierPhaseUncertainty = svMeas.carrierPhaseUnc;
        measurementData.flags |= GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_UNCERTAINTY_BIT;
    }

    // accumulatedDeltaRangeUncertaintyM
    if (!isExt) {
        if (gnss_measurement_report_ptr.svCarrierPhaseUncertainty_valid) {
            measurementData.adrUncertaintyMeters =
                (SPEED_OF_LIGHT / measurementData.carrierFrequencyHz) *
                gnss_measurement_report_ptr.svCarrierPhaseUncertainty[index];
            measurementData.flags |= GNSS_MEASUREMENTS_DATA_ADR_UNCERTAINTY_BIT;
        } else {
            measurementData.adrUncertaintyMeters = 0.0;
        }
    } else {
        if (gnss_measurement_report_ptr.extSvCarrierPhaseUncertainty_valid) {
            measurementData.adrUncertaintyMeters =
                (SPEED_OF_LIGHT / measurementData.carrierFrequencyHz) *
                gnss_measurement_report_ptr.extSvCarrierPhaseUncertainty[index];
            measurementData.flags |= GNSS_MEASUREMENTS_DATA_ADR_UNCERTAINTY_BIT;
        } else {
            measurementData.adrUncertaintyMeters = 0.0;
        }
    }

    measurementData.flags |= GNSS_MEASUREMENTS_DATA_ADR_STATE_BIT;

    if ((gnss_measurement_info.validMask & QMI_LOC_SV_CARRIER_PHASE_VALID_V02) &&
        (gnss_measurement_info.carrierPhase != 0.0)) {
        measurementData.adrStateMask = (GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_VALID_BIT |
                GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_RESET_BIT);

        uint8_t svIdFound = 1;
        int32_t refCountDiff = 0;
        stringstream ss;
        ss << gnss_measurement_report_ptr.gnssSignalType;
        ss << "-";
        ss << gnss_measurement_info.gnssSvId;
        CycleSlipCountMapItr iter;
        CycleSlipCountMap &prevCntMapToUse = gnss_measurement_report_ptr.nHzMeasurement ? \
                mPrevNhzSlipCountMap : mPrev1HzSlipCountMap;
        int32_t maxRefCntDiff = gnss_measurement_report_ptr.nHzMeasurement ? \
                MAX_REFOUNT_DIFF_FOR_NHZ : MAX_REFOUNT_DIFF_FOR_1HZ;

        iter = prevCntMapToUse.find(ss.str());
        if (iter == prevCntMapToUse.end()) {
            svIdFound = 0;
        } else {
            // found the SV ID with previous nhz data base
            // Consider it as not available, if the abs(difference from prev FCount)
            // is > 110 msec for Nhz
            refCountDiff = gnss_measurement_report_ptr.systemTimeExt.refFCount -
                    iter->second.refFCount;
            if (refCountDiff > maxRefCntDiff) {
                svIdFound = 0;
                LOC_LOGi("svID: %d refCountDiff: %d > %d , Not consider it",
                       gnss_measurement_info.gnssSvId, refCountDiff, maxRefCntDiff);
            }
        }
        if (svIdFound) {
            measurementData.adrStateMask &=
                    ~GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_RESET_BIT;
        }
        if ((gnss_measurement_info.validMask & QMI_LOC_SV_CYCLESLIP_COUNT_VALID_V02) && svIdFound &&
            (iter->second.cycleSlipCount != gnss_measurement_info.cycleSlipCount)) {
            measurementData.adrStateMask |=
                    GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_CYCLE_SLIP_BIT;
        } else if (!svIdFound) {
            measurementData.adrStateMask |=
                    GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_CYCLE_SLIP_BIT;

        }
        MeasCacheInfo measInfo = {};
        measInfo.cycleSlipCount = gnss_measurement_info.cycleSlipCount;
        measInfo.refFCount = gnss_measurement_report_ptr.systemTimeExt.refFCount;

        if (gnss_measurement_report_ptr.nHzMeasurement) {
            mCurrentCycleSlipCountMapNHz[ss.str()] = measInfo;
        } else {
            mCurrentCycleSlipCountMap1Hz[ss.str()] = measInfo;
        }

        if (validMeasStatus & QMI_LOC_MASK_MEAS_STATUS_LP_VALID_V02) {
            measurementData.adrStateMask |=
                    GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_HALF_CYCLE_RESOLVED_BIT;
        }
        LOC_LOGa("adrStateMask = 0x%02x", measurementData.adrStateMask);
    } else {
        measurementData.adrStateMask = GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_UNKNOWN;
    }

    // cycleSlipCount
    if (1 == svMeas.cycleSlipCountValid) {
        measurementData.cycleSlipCount = svMeas.cycleSlipCount;
        measurementData.flags |= GNSS_MEASUREMENTS_DATA_CYCLE_SLIP_COUNT_BIT;
    }

    // multipath_indicator
    measurementData.multipathIndicator = GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_UNKNOWN;
    measurementData.flags |= GNSS_MEASUREMENTS_DATA_MULTIPATH_INDICATOR_BIT;

    // AGC
    convertJammerIndicator(gnss_measurement_report_ptr,
                           measurementData.agcLevelDb,
                           measurementData.flags, true);

    if (gnss_measurement_report_ptr.gnssSignalType_valid) {
        measurementData.gnssSignalType =
                convertQmiGnssSignalType(gnss_measurement_report_ptr.gnssSignalType);
        measurementData.flags |= GNSS_MEASUREMENTS_DATA_GNSS_SIGNAL_TYPE_BIT;
    }

    // code type
    if (gnss_measurement_report_ptr.measurementCodeType_valid) {
        switch (gnss_measurement_report_ptr.measurementCodeType)
        {
        case eQMI_LOC_GNSS_CODE_TYPE_A_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_A; break;
        case eQMI_LOC_GNSS_CODE_TYPE_B_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_B; break;
        case eQMI_LOC_GNSS_CODE_TYPE_C_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_C; break;
        case eQMI_LOC_GNSS_CODE_TYPE_I_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_I; break;
        case eQMI_LOC_GNSS_CODE_TYPE_L_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_L; break;
        case eQMI_LOC_GNSS_CODE_TYPE_M_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_M; break;
        case eQMI_LOC_GNSS_CODE_TYPE_P_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_P; break;
        case eQMI_LOC_GNSS_CODE_TYPE_Q_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_Q; break;
        case eQMI_LOC_GNSS_CODE_TYPE_S_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_S; break;
        case eQMI_LOC_GNSS_CODE_TYPE_W_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_W; break;
        case eQMI_LOC_GNSS_CODE_TYPE_X_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_X; break;
        case eQMI_LOC_GNSS_CODE_TYPE_Y_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_Y; break;
        case eQMI_LOC_GNSS_CODE_TYPE_Z_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_Z; break;
        case eQMI_LOC_GNSS_CODE_TYPE_N_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_N; break;
        case eQMI_LOC_GNSS_CODE_TYPE_D_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_D; break;
        case eQMI_LOC_GNSS_CODE_TYPE_E_V02:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_E; break;
        default:
            measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_OTHER; break;
        }
    } else {
        measurementData.codeType = GNSS_MEASUREMENTS_CODE_TYPE_OTHER;
    }

    memset(measurementData.otherCodeTypeName, 0, GNSS_MAX_NAME_LENGTH);
    if (GNSS_MEASUREMENTS_CODE_TYPE_OTHER == measurementData.codeType) {
        if (gnss_measurement_report_ptr.otherCodeTypeName_valid) {
            strlcpy(measurementData.otherCodeTypeName,
                    gnss_measurement_report_ptr.otherCodeTypeName,
                    std::min((uint32_t)sizeof(measurementData.otherCodeTypeName),
                             (uint32_t)gnss_measurement_report_ptr.otherCodeTypeName_len+1));
        }
    }

    // basebandCarrierToNoiseDbHz
    measurementData.basebandCarrierToNoiseDbHz = 0.0;
    if (measurementData.carrierToNoiseDbHz > gnss_measurement_info.gloRfLoss / 10.0) {
        measurementData.basebandCarrierToNoiseDbHz = measurementData.carrierToNoiseDbHz -
                gnss_measurement_info.gloRfLoss / 10.0;
        measurementData.flags |= GNSS_MEASUREMENTS_DATA_BASEBAND_CARRIER_TO_NOISE_BIT;
    }

    // intersignal bias
    if (gnss_measurement_report_ptr.gnssSignalType_valid &&
        (gnss_measurement_report_ptr.systemTime_valid ||
         gnss_measurement_report_ptr.gloTime_valid)) {
        measurementData.gnssSignalType =
                convertQmiGnssSignalType(gnss_measurement_report_ptr.gnssSignalType);
    }

    // satellite PVT
    // find in svPolynomial in mSvPolynomialMap and extract it
    GnssSvPolynomial  svPolynomial = {};
    std::unordered_map<uint16_t, GnssSvPolynomial>::iterator it;
    bool bFound = false;
    LocApiProxyBase* locApiProxyObj = getLocApiProxy();

    it = mSvPolynomialMap.find(gnss_measurement_info.gnssSvId);
    if (it != mSvPolynomialMap.end()) {
        svPolynomial = it->second;
        bFound = true;
    }

    /* For GAL E5 (code type Q) svId could be +50 */
    if (gnss_measurement_info.gnssSvId >= GAL_SV_PRN_MIN &&
        gnss_measurement_info.gnssSvId <= GAL_SV_PRN_MAX &&
        GNSS_MEASUREMENTS_CODE_TYPE_Q == measurementData.codeType) {
        it = mSvPolynomialMap.find(gnss_measurement_info.gnssSvId + 50);
        if (it != mSvPolynomialMap.end()) {
            svPolynomial = it->second;
            bFound = true;
            if (svPolynomial.gnssSvId > GAL_SV_PRN_MAX) {
                svPolynomial.gnssSvId -= 50;
            }
        }
    }

    if (bFound && nullptr != locApiProxyObj) {
        bool ret = locApiProxyObj->getSatellitePVT(
                        svPolynomial,
                        mGnssMeasurements->gnssSvMeasurementSet.svMeasSetHeader,
                        measurementData);
        if (ret) {
            measurementData.flags |= GNSS_MEASUREMENTS_DATA_SATELLITE_PVT_BIT;
        }
    }

    if (mPreferredSignalType == gnss_measurement_report_ptr.gnssSignalType) {
        mReferenceSignalTypeForIsb.svType = measurementData.svType;
        mReferenceSignalTypeForIsb.carrierFrequencyHz = measurementData.carrierFrequencyHz;
        mReferenceSignalTypeForIsb.codeType = measurementData.codeType;
    }

    LOC_LOGa(" GNSS measurement raw data received from modem:\n"
             " Input => gnssSvId=%d validMask=0x%04x validMeasStatus=0x%" PRIx64
             "  CNo=%d gloRfLoss=%d dopplerShift=%.2f dopplerShiftUnc=%.2f"
             "  fineSpeed=%.2f fineSpeedUnc=%.2f"
             "  svTimeMs=%u svTimeSubMs=%.2f svTimeUncMs=%.2f codeType=%d"
             "  carrierPhase=%.2f carrierPhaseUnc=%.6f cycleSlipCount=%u\n"
             " GNSS measurement data after conversion:"
             " Output => size=%" PRIu32 "svid=%d time_offset_ns=%.2f stateMask=0x%08x"
             "  received_sv_time_in_ns=%" PRIu64 " received_sv_time_uncertainty_in_ns=%" PRIu64
             "  c_n0_dbhz=%.2f baseband_c_n0_dbhz=%.2f"
             "  pseudorange_rate_mps=%.2f pseudorange_rate_uncertainty_mps=%.2f"
             "  adrStateMask=0x%02x adrMeters=%.2f adrUncertaintyMeters=%.6f"
             "  carrierFrequencyHz=%.2f codeType=%d",
             gnss_measurement_info.gnssSvId,                                    // %d
             gnss_measurement_info.validMask,                                   // 0x%4x
             validMeasStatus,                                                   // %PRIx64
             gnss_measurement_info.CNo,                                         // %d
             gnss_measurement_info.gloRfLoss,                                   // %d
             gnss_measurement_info.svTimeSpeed.dopplerShift,                    // %f
             gnss_measurement_info.svTimeSpeed.dopplerShiftUnc,                 // %f
             gnss_measurement_info.fineSpeed,                                   // %f
             gnss_measurement_info.fineSpeedUnc,                                // %f
             gnss_measurement_info.svTimeSpeed.svTimeMs,                        // %u
             gnss_measurement_info.svTimeSpeed.svTimeSubMs,                     // %f
             gnss_measurement_info.svTimeSpeed.svTimeUncMs,                     // %f
             gnss_measurement_report_ptr.measurementCodeType,                   // %d
             gnss_measurement_info.carrierPhase,                                // %f
             gnss_measurement_report_ptr.svCarrierPhaseUncertainty[index],      // %f
             gnss_measurement_info.cycleSlipCount,                              // %u
             measurementData.size,                                              // %zu
             measurementData.svId,                                              // %d
             measurementData.timeOffsetNs,                                      // %f
             measurementData.stateMask,                                         // 0x%8x
             measurementData.receivedSvTimeNs,                                  // %PRIu64
             measurementData.receivedSvTimeUncertaintyNs,                       // %PRIu64
             measurementData.carrierToNoiseDbHz,                                // %g
             measurementData.basebandCarrierToNoiseDbHz,                        // %g
             measurementData.pseudorangeRateMps,                                // %g
             measurementData.pseudorangeRateUncertaintyMps,                     // %g
             measurementData.adrStateMask,                                      // 0x%2x
             measurementData.adrMeters,                                         // %f
             measurementData.adrUncertaintyMeters,                              // %f
             measurementData.carrierFrequencyHz,                                // %f
             measurementData.codeType);                                         // %d
}

/*convert GnssMeasurementsClock type from QMI LOC to loc eng format*/
void LocApiV02 :: convertGnssClock (GnssMeasurementsClock& clock,
    const qmiLocEventGnssSvMeasInfoIndMsgT_v02& gnss_measurement_info)
{
    static uint32_t oldRefFCount = 0;
    static uint32_t newRefFCount = 0;
    static uint32_t oldDiscCount = 0;
    static uint32_t newDiscCount = 0;
    static uint32_t localDiscCount = 0;

    // size
    clock.size = sizeof(GnssMeasurementsClock);

    // flag initiation
    GnssMeasurementsClockFlagsMask flags = 0;

    if (gnss_measurement_info.systemTimeExt_valid && gnss_measurement_info.numClockResets_valid) {
        newRefFCount = gnss_measurement_info.systemTimeExt.refFCount;
        newDiscCount = gnss_measurement_info.numClockResets;
        LOC_LOGa("mFirstMeasurementOfSessionReceived %d,"
                 "new: ref cnt %d, disc count %d, old: ref cnt %d, disc count %d",
                 mFirstMeasurementOfSessionReceived,
                 newRefFCount, newDiscCount, oldRefFCount, oldDiscCount);

        mIsFullTracking = true;
        if ((true == mFirstMeasurementOfSessionReceived) &&
                (newDiscCount != oldDiscCount)) {
            mIsFullTracking = false;
        }

        // refFCount roll over, increment local clock dist count to signal this
        if (newRefFCount <= oldRefFCount) {
           localDiscCount++;
        } else if ((false == mFirstMeasurementOfSessionReceived) ||
                   (newDiscCount != oldDiscCount)) {
            // do not increment in full power mode
           if (GNSS_POWER_MODE_M1 != mPowerMode) {
               localDiscCount++;
           }
        }

        mFirstMeasurementOfSessionReceived = true;
        oldDiscCount = newDiscCount;
        oldRefFCount = newRefFCount;

        // timeNs & timeUncertaintyNs
        clock.timeNs = (int64_t)gnss_measurement_info.systemTimeExt.refFCount * 1e6;
        flags |= GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_BIT;
        clock.hwClockDiscontinuityCount = localDiscCount;
        clock.timeUncertaintyNs = 0.0;

        flags |= (GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_BIT |
                  GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_UNCERTAINTY_BIT |
                  GNSS_MEASUREMENTS_CLOCK_FLAGS_HW_CLOCK_DISCONTINUITY_COUNT_BIT);
        // we only support GPS preferred or BDS preferred
        qmiLocSvSystemEnumT_v02 system = gnss_measurement_info.systemTime.system;
        if (gnss_measurement_info.systemTime_valid &&
               ((system == eQMI_LOC_SV_SYSTEM_GPS_V02) || (system == eQMI_LOC_SV_SYSTEM_BDS_V02))) {
            uint16_t systemWeek = gnss_measurement_info.systemTime.systemWeek;
            uint32_t systemMsec = gnss_measurement_info.systemTime.systemMsec;
            float sysClkBias = gnss_measurement_info.systemTime.systemClkTimeBias;
            float sysClkUncMs = gnss_measurement_info.systemTime.systemClkTimeUncMs;
            bool isTimeValid = (sysClkUncMs <= 16.0f); // 16ms

            if (systemWeek != C_GPS_WEEK_UNKNOWN && isTimeValid) {
                // fullBiasNs, biasNs & biasUncertaintyNs
               int64_t totalMs = (int64_t)systemWeek * WEEK_MSECS + (int64_t)systemMsec;
               int64_t gpsTimeNs = totalMs * NSEC_IN_MSEC - (int64_t)(sysClkBias * NSEC_IN_MSEC);
               if (system == eQMI_LOC_SV_SYSTEM_BDS_V02) {
                  gpsTimeNs += (GPS_BDS_DAYS_DIFF * DAY_MSECS * NSEC_IN_MSEC +
                              GPS_BDS_LEAP_SECONDS_DIFF * MSEC_IN_ONE_SEC * NSEC_IN_MSEC);
                  if (gnss_measurement_info.gpsBdsInterSystemBias_valid) {
                      float timeBias = gnss_measurement_info.gpsBdsInterSystemBias.timeBias;
                      gpsTimeNs += (int64_t)timeBias * NSEC_IN_MSEC;
                  }
               }
                clock.fullBiasNs = clock.timeNs - gpsTimeNs;
                clock.biasNs = sysClkBias * 1e6 - (double)((int64_t)(sysClkBias * 1e6));
                clock.biasUncertaintyNs = (double)sysClkUncMs * 1e6;
                flags |= (GNSS_MEASUREMENTS_CLOCK_FLAGS_FULL_BIAS_BIT |
                          GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_BIT |
                          GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_UNCERTAINTY_BIT);

               LOC_LOGa("system %d, week %d, msec %d, sysClkBias %f, gpsbds bias %f, "
                        "gps time ns%" PRIu64 " ",
                        system, systemWeek, systemMsec, sysClkBias,
                        gnss_measurement_info.gpsBdsInterSystemBias.timeBias,
                        gpsTimeNs);

                if (mGnssMeasurements->gnssSvMeasurementSet.svMeasSetHeader.flags &
                    GNSS_SV_MEAS_HEADER_HAS_LEAP_SECOND) {
                    clock.leapSecond = mGnssMeasurements->
                            gnssSvMeasurementSet.svMeasSetHeader.leapSec.leapSec;
                    flags |= GNSS_MEASUREMENTS_CLOCK_FLAGS_LEAP_SECOND_BIT;
                    LOC_LOGa("clock.leapSecond: %d", clock.leapSecond);
                }
            }
        }
    }

    // driftNsps & driftUncertaintyNsps
    if (gnss_measurement_info.rcvrClockFrequencyInfo_valid)
    {
        double driftMPS = gnss_measurement_info.rcvrClockFrequencyInfo.clockDrift;
        double driftUncMPS = gnss_measurement_info.rcvrClockFrequencyInfo.clockDriftUnc;

        clock.driftNsps = driftMPS * MPS_TO_NSPS;
        clock.driftUncertaintyNsps = driftUncMPS * MPS_TO_NSPS;

        flags |= (GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_BIT |
                  GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_UNCERTAINTY_BIT);
    }

    // referenceSignalTypeForIsb
    clock.referenceSignalTypeForIsb.svType = mReferenceSignalTypeForIsb.svType;
    clock.referenceSignalTypeForIsb.carrierFrequencyHz =
                mReferenceSignalTypeForIsb.carrierFrequencyHz;
    clock.referenceSignalTypeForIsb.codeType = mReferenceSignalTypeForIsb.codeType;
    clock.referenceSignalTypeForIsb.otherCodeTypeName[0] = '\0';

    if ((1 == gnss_measurement_info.leapSecondInfo_valid) &&
        (0 == gnss_measurement_info.leapSecondInfo.leapSecUnc)) {
        clock.leapSecond = gnss_measurement_info.leapSecondInfo.leapSec;
        flags |= GNSS_MEASUREMENTS_CLOCK_FLAGS_LEAP_SECOND_BIT ;
    }

    clock.flags = flags;

    LOC_LOGa(" GNSS measurement clock data received from modem: \n");
    LOC_LOGa(" Input => systemTime_valid=%d systemTimeExt_valid=%d numClockResets_valid=%d\n",
             gnss_measurement_info.systemTime_valid,                      // %d
             gnss_measurement_info.systemTimeExt_valid,                   // %d
             gnss_measurement_info.numClockResets_valid);                 // %d

    LOC_LOGa("  systemWeek=%d systemMsec=%d systemClkTimeBias=%f\n",
             gnss_measurement_info.systemTime.systemWeek,                 // %d
             gnss_measurement_info.systemTime.systemMsec,                 // %d
             gnss_measurement_info.systemTime.systemClkTimeBias);         // %f

    LOC_LOGa("  systemClkTimeUncMs=%f refFCount=%d numClockResets=%d\n",
             gnss_measurement_info.systemTime.systemClkTimeUncMs,         // %f
             gnss_measurement_info.systemTimeExt.refFCount,               // %d
             gnss_measurement_info.numClockResets);                       // %d

    LOC_LOGa("  clockDrift=%f clockDriftUnc=%f\n",
             gnss_measurement_info.rcvrClockFrequencyInfo.clockDrift,     // %f
             gnss_measurement_info.rcvrClockFrequencyInfo.clockDriftUnc); // %f


    LOC_LOGa(" GNSS measurement clock after conversion: \n");
    LOC_LOGa(" Output => timeNs=%" PRId64 "\n",
        clock.timeNs);                       // %PRId64

    LOC_LOGa("  fullBiasNs=%" PRId64 " biasNs=%g bias_uncertainty_ns=%g\n",
             clock.fullBiasNs,                    // %PRId64
             clock.biasNs,                        // %g
             clock.biasUncertaintyNs);            // %g

    LOC_LOGa("  driftNsps=%g drift_uncertainty_nsps=%g\n",
             clock.driftNsps,                     // %g
             clock.driftUncertaintyNsps);         // %g

    LOC_LOGa("  hw_clock_discontinuity_count=%d flags=0x%04x\n",
             clock.hwClockDiscontinuityCount,     // %lld
             clock.flags);                        // %04x

}

/* event callback registered with the loc_api v02 interface */
void LocApiV02 :: eventCb(locClientHandleType /*clientHandle*/,
  uint32_t eventId, locClientEventIndUnionType eventPayload)
{
  if ((eQMI_LOC_POWER_STATE_SUSPENDED_V02 == mPlatformPowerState) ||
        (eQMI_LOC_POWER_STATE_DEEP_SLEEP_ENTRY_V02 == mPlatformPowerState) ||
            (eQMI_LOC_POWER_STATE_SHUTDOWN_V02 == mPlatformPowerState)) {
      syslog(LOG_INFO, "eventCb: event id = 0x%X, event name %s",
             eventId, loc_get_v02_event_name(eventId));
  }

  switch(eventId)
  {
    //Position Report
    case QMI_LOC_EVENT_POSITION_REPORT_IND_V02:
      reportPosition(eventPayload.pPositionReportEvent);
      break;

    // Satellite report
    case QMI_LOC_EVENT_GNSS_SV_INFO_IND_V02:
      reportSv(eventPayload.pGnssSvInfoReportEvent);
      break;

    // Status report
    case QMI_LOC_EVENT_ENGINE_STATE_IND_V02:
      reportEngineState(eventPayload.pEngineState);
      break;

    case QMI_LOC_EVENT_FIX_SESSION_STATE_IND_V02:
      reportFixSessionState(eventPayload.pFixSessionState);
      break;

    // XTRA request
    case QMI_LOC_EVENT_INJECT_PREDICTED_ORBITS_REQ_IND_V02:
      reportXtraServerUrl(eventPayload.pInjectPredictedOrbitsReqEvent);
      requestXtraData();
      break;

    // time request
    case QMI_LOC_EVENT_INJECT_TIME_REQ_IND_V02:
      requestTime();
      break;

    //position request
    case QMI_LOC_EVENT_INJECT_POSITION_REQ_IND_V02:
      requestLocation();
      break;

    // NI request
    case QMI_LOC_EVENT_NI_NOTIFY_VERIFY_REQ_IND_V02:
      reportNiRequest(eventPayload.pNiNotifyVerifyReqEvent);
      break;

    // AGPS connection request
    case QMI_LOC_EVENT_LOCATION_SERVER_CONNECTION_REQ_IND_V02:
      reportAtlRequest(eventPayload.pLocationServerConnReqEvent);
      break;

    case QMI_LOC_EVENT_GNSS_MEASUREMENT_REPORT_IND_V02:
      if (mInSession) {
          reportGnssMeasurementData(*eventPayload.pGnssSvRawInfoEvent);
      }
      break;

    case QMI_LOC_EVENT_SV_POLYNOMIAL_REPORT_IND_V02:
      reportSvPolynomial(eventPayload.pGnssSvPolyInfoEvent);
      break;

    case QMI_LOC_EVENT_GPS_EPHEMERIS_REPORT_IND_V02:
    case QMI_LOC_EVENT_GLONASS_EPHEMERIS_REPORT_IND_V02:
    case QMI_LOC_EVENT_BDS_EPHEMERIS_REPORT_IND_V02:
    case QMI_LOC_EVENT_GALILEO_EPHEMERIS_REPORT_IND_V02:
    case QMI_LOC_EVENT_QZSS_EPHEMERIS_REPORT_IND_V02:
      reportSvEphemeris(eventId, eventPayload);
      break;

    //Unpropagated position report
    case QMI_LOC_EVENT_UNPROPAGATED_POSITION_REPORT_IND_V02:
      reportPosition(eventPayload.pPositionReportEvent, true);
      break;

    case QMI_LOC_GET_BLACKLIST_SV_IND_V02:
      reportGnssSvIdConfig(*eventPayload.pGetBlacklistSvEvent);
      break;

    case QMI_LOC_GET_CONSTELLATION_CONTROL_IND_V02:
      reportGnssSvTypeConfig(*eventPayload.pGetConstellationConfigEvent);
      break;

    case  QMI_LOC_EVENT_WIFI_REQ_IND_V02:
      requestOdcpi(*eventPayload.pWifiReqEvent);
      break;

    case QMI_LOC_EVENT_REPORT_IND_V02:
      reportLocEvent(eventPayload.pLocEvent);
      break;

    // System info event regarding next leap second
    case QMI_LOC_SYSTEM_INFO_IND_V02:
      reportSystemInfo(eventPayload.pLocSystemInfoEvent);
      break;

    case QMI_LOC_LOCATION_REQUEST_NOTIFICATION_IND_V02:
      reportLocationRequestNotification(eventPayload.pLocReqNotifEvent);
      break;

    case QMI_LOC_EVENT_GEOFENCE_BREACH_NOTIFICATION_IND_V02:
      geofenceBreachEvent(eventPayload.pGeofenceBreachEvent);
      break;

    case QMI_LOC_EVENT_GEOFENCE_BATCHED_BREACH_NOTIFICATION_IND_V02:
      geofenceBreachEvent(eventPayload.pGeofenceBatchedBreachEvent);
      break;

    case QMI_LOC_EVENT_GEOFENCE_GEN_ALERT_IND_V02:
      geofenceStatusEvent(eventPayload.pGeofenceGenAlertEvent);
      break;

    case QMI_LOC_EVENT_GEOFENCE_BATCHED_DWELL_NOTIFICATION_IND_V02:
      geofenceDwellEvent(eventPayload.pGeofenceBatchedDwellEvent);
      break;

    case QMI_LOC_EVENT_BATCH_FULL_NOTIFICATION_IND_V02:
      batchFullEvent(eventPayload.pBatchCount);
      break;

    case QMI_LOC_EVENT_BATCHING_STATUS_IND_V02:
      batchStatusEvent(eventPayload.pBatchingStatusEvent);
      break;

    case QMI_LOC_EVENT_DBT_POSITION_REPORT_IND_V02:
      onDbtPosReportEvent(eventPayload.pDbtPositionReportEvent);
      break;

    case QMI_LOC_LATENCY_INFORMATION_IND_V02:
      reportLatencyInfo(eventPayload.pLocLatencyInfoIndMsg);
      break;
    case QMI_LOC_ENGINE_DEBUG_DATA_IND_V02:
      reportEngDebugDataInfo(eventPayload.pLocEngDbgDataInfoIndMsg);
      break;

    case QMI_LOC_EVENT_PLATFORM_POWER_STATE_CHANGED_IND_V02:
      reportPowerStateChangeInfo(eventPayload.pPowerStateChangedIndMsg);
      break;

    case QMI_LOC_EVENT_ENGINE_LOCK_STATE_IND_V02:
      LOC_LOGd("Got QMI_LOC_EVENT_ENGINE_STATE_IND_V02");
      reportEngineLockStatus(eventPayload.pEngineLockStateIndMsg->engineLockState);
      break;

    case QMI_LOC_DC_REPORT_IND_V02:
      reportDcMessage(eventPayload.pDcReportIndMsg);
      break;

    case QMI_LOC_GNSS_BANDS_SUPPORTED_IND_V02:
      processGnssBandsSupportedInd(eventPayload.pGnssBandsSupportedIndMsg);
      break;

    case QMI_LOC_NTN_CONFIG_UPDATE_IND_V02:
      LocApiBase::reportNtnConfigUpdateEvent(eventPayload.pNtnConfigUpdateIndMsg->signalType);
      break;
  }
}

/* Call the service LocAdapterBase down event*/
void LocApiV02 :: errorCb(locClientHandleType /*handle*/,
                             locClientErrorEnumType errorId)
{
  if(errorId == eLOC_CLIENT_ERROR_SERVICE_UNAVAILABLE)
  {
    LOC_LOGe("Service unavailable");

    // << SEC_GPS
    mSecDefaultInitDone = false;
    mIsSsrHappened = true;
    sendSilentResetMessage(CP_SILENT_RESET);

    // SEC_GPS >>
    handleEngineDownEvent();
  }
}

void LocApiV02 ::getWwanZppFix()
{
    sendMsg(new LocApiMsg([this] () {

    locClientReqUnionType req_union;
    qmiLocGetAvailWwanPositionReqMsgT_v02 zpp_req;
    memset(&zpp_req, 0, sizeof(zpp_req));

    req_union.pGetAvailWwanPositionReq = &zpp_req;

    locClientSendReq(QMI_LOC_GET_AVAILABLE_WWAN_POSITION_REQ_V02, req_union);
    }));
}

void LocApiV02 ::getBestAvailableZppFix()
{
    sendMsg(new LocApiMsg([this] () {

    locClientReqUnionType req_union;
    qmiLocGetBestAvailablePositionReqMsgT_v02 zpp_req;

    memset(&zpp_req, 0, sizeof(zpp_req));
    req_union.pGetBestAvailablePositionReq = &zpp_req;

    locClientSendReq(QMI_LOC_GET_BEST_AVAILABLE_POSITION_REQ_V02, req_union);
    }));
}

bool LocApiV02::getBestAvailableZppFixSync(LocGpsLocation &zppLoc,
        LocPosTechMask &tech_mask) {

    qmiLocGetBestAvailablePositionReqMsgT_v02 zpp_req;
    qmiLocGetBestAvailablePositionIndMsgT_v02 zpp_ind;
    locClientReqUnionType req_union;
    locClientStatusEnumType status;

    LOC_LOGd("Get ZPP Fix from best available source\n");
    memset(&zpp_req, 0, sizeof(zpp_req));
    memset(&zpp_ind, 0, sizeof(zpp_ind));
    memset(&zppLoc, 0, sizeof(zppLoc));
    zppLoc.size = sizeof(zppLoc);
    tech_mask = 0;

    req_union.pGetBestAvailablePositionReq = &zpp_req;
    zpp_req.transactionId = 1;
    status = locSyncSendReq(QMI_LOC_GET_BEST_AVAILABLE_POSITION_REQ_V02,
                            req_union,
                            LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                            QMI_LOC_GET_BEST_AVAILABLE_POSITION_IND_V02,
                            &zpp_ind);

    if ((eLOC_CLIENT_SUCCESS != status) ||
        (eQMI_LOC_SUCCESS_V02 != zpp_ind.status))
    {
        LOC_LOGe("error! status = %d, zpp_ind.status = %d\n",
                  (status),
                  (zpp_ind.status));
        return false;
    } else {
        if (zpp_ind.timestampUtc_valid) {
            zppLoc.timestamp = zpp_ind.timestampUtc;
        } else {
            /* The UTC time from modem is not valid.
                    In this case, we use current system time instead.*/

          struct timespec time_info_current = {};
          clock_gettime(CLOCK_REALTIME, &time_info_current);
          zppLoc.timestamp = (time_info_current.tv_sec)*1e3 +
                  (time_info_current.tv_nsec)/1e6;
          LOC_LOGd("zpp timestamp got from system: %" PRIu64, zppLoc.timestamp);
        }

        if (zpp_ind.latitude_valid && zpp_ind.longitude_valid &&
                zpp_ind.horUncCircular_valid ) {
            zppLoc.flags = LOC_GPS_LOCATION_HAS_LAT_LONG | LOC_GPS_LOCATION_HAS_ACCURACY;
            zppLoc.latitude = zpp_ind.latitude;
            zppLoc.longitude = zpp_ind.longitude;
            zppLoc.accuracy = zpp_ind.horUncCircular;

            // If horCircularConfidence_valid is true, and horCircularConfidence value
            // is less than 68%, then scale the accuracy value to 68% confidence.
            if (zpp_ind.horCircularConfidence_valid)
            {
                scaleAccuracyTo68PercentConfidence(zpp_ind.horCircularConfidence,
                                                   zppLoc, true);
            }

            if (zpp_ind.altitudeWrtEllipsoid_valid) {
                zppLoc.flags |= LOC_GPS_LOCATION_HAS_ALTITUDE;
                zppLoc.altitude = zpp_ind.altitudeWrtEllipsoid;
            }

            if (zpp_ind.technologyMask_valid) {
                tech_mask = zpp_ind.technologyMask;
            }
        }
        return true;
    }
}

LocationError LocApiV02 :: setGpsLockSync(GnssConfigGpsLock lock)
{
    LocationError err = LOCATION_ERROR_SUCCESS;
    qmiLocSetEngineLockReqMsgT_v02 setEngineLockReq;
    qmiLocSetEngineLockIndMsgT_v02 setEngineLockInd;
    locClientStatusEnumType status;
    locClientReqUnionType req_union;
    uint32_t nfwControlBits = convertGpsLock(lock); // QC CR3933472
    gpsLockState = lock; //SEC

    memset(&setEngineLockReq, 0, sizeof(setEngineLockReq));
    setEngineLockReq.lockType = convertGpsLockFromAPItoQMI((GnssConfigGpsLock)lock);
    setEngineLockReq.subType_valid = true;
    setEngineLockReq.subType = eQMI_LOC_LOCK_ALL_SUB_V02;
    // < SEC : This feature is exception against location privacy requirement of Android Q
    //        which is accepted by Google for the request from the carrier(or government)
    //        This feature should be enabled only for the models that has lower first API level than 29,
    //        or the legacy chipsets which can't support NFW_LOCK feature such as SM8150.
    //        As of first API level 29(which means launched with Android Q), modem will handle it with NFW_LOCK.
    //        NFW_LOCK will be set without CP_NILR/SUPL_NI BITMASK at the oem_factoryreset(NV 74235).

    bool gpsLockException = false;
    if((SemNativeCarrierFeature::getInstance()->getInt(sim_slotId, "CarrierFeature_GPS_ConfigExceptionMaskForAGNSS", -1, false) > 0) &&
            ((SemNativeCarrierFeature::getInstance()->getInt(sim_slotId, "CarrierFeature_GPS_ConfigExceptionMaskForAGNSS", -1, false)
            & AGNSS_CONFIG_EXCEPTION_GPS_LOCK) != 0)) {
        gpsLockException = true;
    }

    if (gpsLockException)
    {
        setEngineLockReq.lockType = eQMI_LOC_LOCK_NONE_V02;
    }
    // SEC >

    setEngineLockReq.lockClient_valid = false;
    setEngineLockReq.clientsConfig_valid = true;
    setEngineLockReq.clientsConfig = (uint64_t)nfwControlBits;
    req_union.pSetEngineLockReq = &setEngineLockReq;

    LOC_LOGd("API lock type = 0x%X QMI lockType = %d "
             "nfwControlBits = 0x%X clientsConfig = 0x%" PRIx64"",
             lock, setEngineLockReq.lockType, nfwControlBits,
             setEngineLockReq.clientsConfig);
    memset(&setEngineLockInd, 0, sizeof(setEngineLockInd));
    status = locSyncSendReq(QMI_LOC_SET_ENGINE_LOCK_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                            QMI_LOC_SET_ENGINE_LOCK_IND_V02,
                            &setEngineLockInd);
    if (eLOC_CLIENT_SUCCESS != status || eQMI_LOC_SUCCESS_V02 != setEngineLockInd.status) {
        err = LOCATION_ERROR_GENERAL_FAILURE;
    }
    return err;
}

void LocApiV02::requestForAidingData(GnssAidingDataSvMask svDataMask)
{
    sendMsg(new LocApiMsg([this, svDataMask] () {
        locClientEventMaskType qmiMask = 0;

        if (svDataMask & GNSS_AIDING_DATA_SV_POLY_BIT)
            qmiMask |= QMI_LOC_EVENT_MASK_GNSS_SV_POLYNOMIAL_REPORT_V02;

        if (svDataMask & GNSS_AIDING_DATA_SV_EPHEMERIS_BIT)
            qmiMask |= QMI_LOC_EVENT_MASK_EPHEMERIS_REPORT_V02;

        if (svDataMask & GNSS_AIDING_DATA_SV_IONOSPHERE_BIT)
            qmiMask |= QMI_LOC_EVENT_MASK_GNSS_EVENT_REPORT_V02;

        sendRequestForAidingData(qmiMask);
    }));
}

void LocApiV02 :: getEngineLockStateSync() {

    qmiLocGetEngineLockReqMsgT_v02 getEngineLockReq;
    qmiLocGetEngineLockIndMsgT_v02 getEngineLockInd;
    locClientStatusEnumType status;
    locClientReqUnionType req_union;
    qmiLocEngineLockStateEnumT_v02 ret = QMILOCENGINELOCKSTATEENUMT_MIN_ENUM_VAL_V02;
    LocationError err = LOCATION_ERROR_SUCCESS;

    memset(&getEngineLockInd, 0, sizeof(getEngineLockInd));

    /*Passing req_union as a parameter even though this request has no payload
    since NULL or 0 gives an error during compilation*/
    getEngineLockReq.subType_valid = true;
    getEngineLockReq.subType = eQMI_LOC_LOCK_ALL_SUB_V02;
    req_union.pGetEngineLockReq = &getEngineLockReq;

    status = locSyncSendReq(QMI_LOC_GET_ENGINE_LOCK_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                            QMI_LOC_GET_ENGINE_LOCK_IND_V02,
                            &getEngineLockInd);

    if (status != eLOC_CLIENT_SUCCESS || getEngineLockInd.status != eQMI_LOC_SUCCESS_V02) {
        ret = QMILOCENGINELOCKSTATEENUMT_MIN_ENUM_VAL_V02;
        err = LOCATION_ERROR_GENERAL_FAILURE;
    } else {
        if (getEngineLockInd.engineLockState_valid) {
            ret = getEngineLockInd.engineLockState;
        } else {
            LOC_LOGe("Engine Lock State not valid");
            ret = QMILOCENGINELOCKSTATEENUMT_MIN_ENUM_VAL_V02;
        }
    }
    EngineLockState lockState = convertEngineLockState(ret);
    setEngineLockState(lockState);
    LocApiBase::reportEngineLockStatus(lockState);
}

LocationError
LocApiV02:: setXtraVersionCheckSync(uint32_t check)
{
    LocationError err = LOCATION_ERROR_SUCCESS;
    qmiLocSetXtraVersionCheckReqMsgT_v02 req;
    qmiLocSetXtraVersionCheckIndMsgT_v02 ind;
    locClientStatusEnumType status;
    locClientReqUnionType req_union;

    LOC_LOGd("check: %u", check);
    memset(&req, 0, sizeof(req));
    memset(&ind, 0, sizeof(ind));
    switch (check) {
    case 0:
        req.xtraVersionCheckMode = eQMI_LOC_XTRA_VERSION_CHECK_DISABLE_V02;
        break;
    case 1:
        req.xtraVersionCheckMode = eQMI_LOC_XTRA_VERSION_CHECK_AUTO_V02;
        break;
    case 2:
        req.xtraVersionCheckMode = eQMI_LOC_XTRA_VERSION_CHECK_XTRA2_V02;
        break;
    case 3:
        req.xtraVersionCheckMode = eQMI_LOC_XTRA_VERSION_CHECK_XTRA3_V02;
        break;
    default:
        req.xtraVersionCheckMode = eQMI_LOC_XTRA_VERSION_CHECK_DISABLE_V02;
        break;
    }

    req_union.pSetXtraVersionCheckReq = &req;
    status = locSyncSendReq(QMI_LOC_SET_XTRA_VERSION_CHECK_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                            QMI_LOC_SET_XTRA_VERSION_CHECK_IND_V02,
                            &ind);
    if(status != eLOC_CLIENT_SUCCESS || ind.status != eQMI_LOC_SUCCESS_V02) {
        err = LOCATION_ERROR_GENERAL_FAILURE;
    }

    return err;
}

int LocApiV02::setSvMeasurementConstellation(const locClientEventMaskType mask)
{
    enum loc_api_adapter_err ret_val = LOC_API_ADAPTER_ERR_SUCCESS;
    qmiLocSetGNSSConstRepConfigReqMsgT_v02 setGNSSConstRepConfigReq;
    qmiLocSetGNSSConstRepConfigIndMsgT_v02 setGNSSConstRepConfigInd;
    locClientStatusEnumType status;
    locClientReqUnionType req_union;

    qmiLocGNSSConstellEnumT_v02 svConstellation = eQMI_SYSTEM_GPS_V02 |
                                                eQMI_SYSTEM_GLO_V02 |
                                                eQMI_SYSTEM_BDS_V02 |
                                                eQMI_SYSTEM_GAL_V02 |
                                                eQMI_SYSTEM_QZSS_V02 |
                                                eQMI_SYSTEM_NAVIC_V02;
    LOC_LOGd("set GNSS measurement to report constellation: %" PRIx64 " "
            "report mask = 0x%" PRIx64 "\n", svConstellation, mask);

    memset(&setGNSSConstRepConfigReq, 0, sizeof(setGNSSConstRepConfigReq));

    setGNSSConstRepConfigReq.measReportConfig_valid = true;
    if ((mask & (QMI_LOC_EVENT_MASK_GNSS_MEASUREMENT_REPORT_V02 |
                 QMI_LOC_EVENT_MASK_GNSS_NHZ_MEASUREMENT_REPORT_V02)) ||
        mMasterRegisterNotSupported) {
        setGNSSConstRepConfigReq.measReportConfig = svConstellation;
    }

    setGNSSConstRepConfigReq.svPolyReportConfig_valid = true;
    if (mask & QMI_LOC_EVENT_MASK_GNSS_SV_POLYNOMIAL_REPORT_V02) {
        setGNSSConstRepConfigReq.svPolyReportConfig = svConstellation;
    }
    req_union.pSetGNSSConstRepConfigReq = &setGNSSConstRepConfigReq;
    memset(&setGNSSConstRepConfigInd, 0, sizeof(setGNSSConstRepConfigInd));

    status = locSyncSendReq(QMI_LOC_SET_GNSS_CONSTELL_REPORT_CONFIG_V02,
        req_union,
        LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
        QMI_LOC_SET_GNSS_CONSTELL_REPORT_CONFIG_IND_V02,
        &setGNSSConstRepConfigInd);

    if (status != eLOC_CLIENT_SUCCESS ||
        (setGNSSConstRepConfigInd.status != eQMI_LOC_SUCCESS_V02 &&
            setGNSSConstRepConfigInd.status != eQMI_LOC_ENGINE_BUSY_V02)) {
        ret_val = LOC_API_ADAPTER_ERR_GENERAL_FAILURE;
    }
    else {
        LOC_LOGd("Set GNSS constellation succeeded.");
    }

    return ret_val;
}

void LocApiV02::setConstrainedTuncMode(bool enabled,
                                       float tuncConstraint,
                                       uint32_t energyBudget,
                                       LocApiResponse *adapterResponse) {
    sendMsg(new LocApiMsg([this, enabled, tuncConstraint, energyBudget, adapterResponse] () {

    LocationError err = LOCATION_ERROR_SUCCESS;
    qmiLocSetConstrainedTuncModeReqMsgT_v02 req;
    qmiLocSetConstrainedTuncModeIndMsgT_v02 ind;
    locClientStatusEnumType status;
    locClientReqUnionType req_union;

    memset(&req, 0, sizeof(req));
    memset(&ind, 0, sizeof(ind));
    req.tuncConstraintOn = enabled;
    if (tuncConstraint > 0.0) {
        req.tuncConstraint_valid = true;
        req.tuncConstraint = tuncConstraint;
    }
    if (energyBudget != 0) {
        req.energyBudget_valid = true;
        req.energyBudget = energyBudget;
    }

    LOC_LOGD("setConstrainedTuncMode: enabled %d, tuncConstraint (%d, %f),"
             "energyBudget (%d, %d)", req.tuncConstraintOn,
             req.tuncConstraint_valid, req.tuncConstraint,
             req.energyBudget_valid, req.energyBudget);

    req_union.pSetConstrainedTuncModeReq = &req;
    status = locSyncSendReq(QMI_LOC_SET_CONSTRAINED_TUNC_MODE_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                            QMI_LOC_SET_CONSTRAINED_TUNC_MODE_IND_V02,
                            &ind);
    if(status != eLOC_CLIENT_SUCCESS || ind.status != eQMI_LOC_SUCCESS_V02) {
        err = LOCATION_ERROR_GENERAL_FAILURE;
    }

    if (adapterResponse) {
        adapterResponse->returnToSender(err);
    }
  }));
}

void LocApiV02::setPositionAssistedClockEstimatorMode
        (bool enabled, LocApiResponse *adapterResponse) {

    sendMsg(new LocApiMsg([this, enabled, adapterResponse] () {

    LocationError err = LOCATION_ERROR_SUCCESS;
    qmiLocEnablePositionAssistedClockEstReqMsgT_v02 req;
    qmiLocEnablePositionAssistedClockEstIndMsgT_v02 ind;
    locClientStatusEnumType status;
    locClientReqUnionType req_union;

    LOC_LOGD("setPositionAssistedClockEstimatorMode: enabled %d", enabled);
    memset(&req, 0, sizeof(req));
    memset(&ind, 0, sizeof(ind));
    req.enablePositionAssistedClockEst = enabled;

    req_union.pSetEnablePositionAssistedClockEstReq = &req;
    status = locSyncSendReq(QMI_LOC_ENABLE_POSITION_ASSISTED_CLOCK_EST_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                            QMI_LOC_ENABLE_POSITION_ASSISTED_CLOCK_EST_IND_V02,
                            &ind);
    if(status != eLOC_CLIENT_SUCCESS || ind.status != eQMI_LOC_SUCCESS_V02) {
        err = LOCATION_ERROR_GENERAL_FAILURE;
    }
    if (adapterResponse) {
        adapterResponse->returnToSender(err);
    }
    }));
}

void LocApiV02::getGnssEnergyConsumed() {
    sendMsg(new LocApiMsg([this] {

    LocationError err = LOCATION_ERROR_SUCCESS;

    qmiLocQueryGNSSEnergyConsumedReqMsgT_v02 req;
    qmiLocQueryGNSSEnergyConsumedIndMsgT_v02 ind;
    locClientStatusEnumType status;
    locClientReqUnionType req_union;

    memset(&req, 0, sizeof(req));
    memset(&ind, 0, sizeof(ind));

    req_union.pQueryGNSSEnergyConsumedReq = &req;
    status = locSyncSendReq(QMI_LOC_QUERY_GNSS_ENERGY_CONSUMED_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                            QMI_LOC_QUERY_GNSS_ENERGY_CONSUMED_IND_V02,
                            &ind);

    if(status != eLOC_CLIENT_SUCCESS) {
        // report invalid read to indicate error (based on QMI message)
        LocApiBase::reportGnssEngEnergyConsumedEvent(0xffffffffffffffff);
        err = LOCATION_ERROR_GENERAL_FAILURE;
    } else {
        LocApiBase::reportGnssEngEnergyConsumedEvent(ind.energyConsumedSinceFirstBoot);
    }

    }));
}

void LocApiV02 :: updateSystemPowerState(PowerStateType powerState){
    sendMsg(new LocApiMsg([this, powerState] () {

    LocationError err = LOCATION_ERROR_SUCCESS;
    qmiLocInjectPlatformPowerStateReqMsgT_v02 req;
    qmiLocInjectPlatformPowerStateIndMsgT_v02 ind;

    locClientStatusEnumType status;
    locClientReqUnionType req_union;

    syslog(LOG_INFO, "updatePowerState: power state %d", powerState);
    qmiLocPlatformPowerStateEnumT_v02 qmiPowerState = eQMI_LOC_POWER_STATE_UNKNOWN_V02;
    switch (powerState) {
    case POWER_STATE_SUSPEND:
        qmiPowerState = eQMI_LOC_POWER_STATE_SUSPENDED_V02;
        break;
    case POWER_STATE_RESUME:
        qmiPowerState = eQMI_LOC_POWER_STATE_RESUME_V02;
        break;
    case POWER_STATE_SHUTDOWN:
        qmiPowerState = eQMI_LOC_POWER_STATE_SHUTDOWN_V02;
        break;
    case POWER_STATE_DEEP_SLEEP_ENTRY:
        qmiPowerState = eQMI_LOC_POWER_STATE_DEEP_SLEEP_ENTRY_V02;
        break;
    case POWER_STATE_DEEP_SLEEP_EXIT:
        qmiPowerState = eQMI_LOC_POWER_STATE_DEEP_SLEEP_EXIT_V02;
        break;
    default:
        break;
    }

    // unknown power state will not be injected to modem
    if (eQMI_LOC_POWER_STATE_UNKNOWN_V02 != qmiPowerState) {
        memset(&req, 0, sizeof(req));
        memset(&ind, 0, sizeof(ind));
        req.powerState = qmiPowerState;
        req_union.pInjectPowerStateReq = &req;

        status = locSyncSendReq(QMI_LOC_INJECT_PLATFORM_POWER_STATE_REQ_V02,
                                req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                                QMI_LOC_INJECT_PLATFORM_POWER_STATE_IND_V02,
                                &ind);
        if (status != eLOC_CLIENT_SUCCESS || ind.status != eQMI_LOC_SUCCESS_V02) {
             err = LOCATION_ERROR_GENERAL_FAILURE;
        }
    }

    }));
}

void LocApiV02::updatePowerConnectState(bool connected)
{
    LOC_LOGd("power connected %d", connected);
    sendMsg(new LocApiMsg([this, connected] () {
    qmiLocSetExternalPowerConfigReqMsgT_v02 chargerStatus = {};

    if (connected) {
        chargerStatus.externalPowerState = eQMI_LOC_EXTERNAL_POWER_CONNECTED_V02;
    } else {
        chargerStatus.externalPowerState = eQMI_LOC_EXTERNAL_POWER_NOT_CONNECTED_V02;
    }

    LOC_SEND_SYNC_REQ(SetExternalPowerConfig, SET_EXTERNAL_POWER_CONFIG, chargerStatus);
    }));
}

void LocApiV02::reportLatencyInfo(const qmiLocLatencyInformationIndMsgT_v02* pLocLatencyInfo)
{
    GnssLatencyInfo gnssLatencyInfo = {};

    if (nullptr == pLocLatencyInfo) {
        LOC_LOGe("pLocLatencyInfo is nullptr");
        return;
    }

    if (eQMI_LOC_LATENCY_INFO_TYPE_MEASUREMENT_V02 != pLocLatencyInfo->latencyInfo) {
        LOC_LOGe("Invalid Latency Info Type");
        return;
    }

    /* check refCount */
    if (pLocLatencyInfo->fCountOfMeasBlk != mRefFCount) {
        LOC_LOGw("FCount mismatch: Latency Fcount=%d Meas. FCount=%d",
                 pLocLatencyInfo->fCountOfMeasBlk,
                 mRefFCount);
        return;
    }

    if (pLocLatencyInfo->sysTickAtChkPt1_valid) {
        gnssLatencyInfo.meQtimer1 = pLocLatencyInfo->sysTickAtChkPt1;
    }
    if (pLocLatencyInfo->sysTickAtChkPt2_valid) {
        gnssLatencyInfo.meQtimer2 = pLocLatencyInfo->sysTickAtChkPt2;
    }
    if (pLocLatencyInfo->sysTickAtChkPt3_valid) {
        gnssLatencyInfo.meQtimer3 = pLocLatencyInfo->sysTickAtChkPt3;
    }
    if (pLocLatencyInfo->sysTickAtChkPt4_valid) {
        gnssLatencyInfo.peQtimer1 = pLocLatencyInfo->sysTickAtChkPt4;
    }
    if (pLocLatencyInfo->sysTickAtChkPt5_valid) {
        gnssLatencyInfo.peQtimer2 = pLocLatencyInfo->sysTickAtChkPt5;
    }
    if (pLocLatencyInfo->sysTickAtChkPt6_valid) {
        gnssLatencyInfo.peQtimer3 = pLocLatencyInfo->sysTickAtChkPt6;
    }
    if (pLocLatencyInfo->sysTickAtChkPt7_valid) {
        gnssLatencyInfo.smQtimer1 = pLocLatencyInfo->sysTickAtChkPt7;
    }
    if (pLocLatencyInfo->sysTickAtChkPt8_valid) {
        gnssLatencyInfo.smQtimer2 = pLocLatencyInfo->sysTickAtChkPt8;
    }
    if (pLocLatencyInfo->sysTickAtChkPt9_valid) {
        gnssLatencyInfo.smQtimer3 = pLocLatencyInfo->sysTickAtChkPt9;
    }
    if (pLocLatencyInfo->sysTickAtChkPt10_valid) {
        gnssLatencyInfo.locMwQtimer = pLocLatencyInfo->sysTickAtChkPt10;
    }
    gnssLatencyInfo.hlosQtimer1 = mHlosQtimer1;
    gnssLatencyInfo.hlosQtimer2 = mHlosQtimer2;
    LOC_LOGa("meQtimer1=%" PRIi64 " "
             "meQtimer2=%" PRIi64 " "
             "meQtimer3=%" PRIi64 " "
             "peQtimer1=%" PRIi64 " "
             "peQtimer2=%" PRIi64 " "
             "peQtimer3=%" PRIi64 " "
             "smQtimer1=%" PRIi64 " "
             "smQtimer2=%" PRIi64 " "
             "smQtimer3=%" PRIi64 " "
             "locMwQtimer=%" PRIi64 " "
             "hlosQtimer1=%" PRIi64 " "
             "hlosQtimer2=%" PRIi64 " ",
             gnssLatencyInfo.meQtimer1, gnssLatencyInfo.meQtimer2,
             gnssLatencyInfo.meQtimer3, gnssLatencyInfo.peQtimer1, gnssLatencyInfo.peQtimer2,
             gnssLatencyInfo.peQtimer3, gnssLatencyInfo.smQtimer1, gnssLatencyInfo.smQtimer2,
             gnssLatencyInfo.smQtimer3, gnssLatencyInfo.locMwQtimer,
             gnssLatencyInfo.hlosQtimer1, gnssLatencyInfo.hlosQtimer2);

    LocApiBase::reportLatencyInfo(gnssLatencyInfo);
}

void LocApiV02::reportEngineLockStatus(const qmiLocEngineLockStateEnumT_v02 engineLockState)
{
    LOC_LOGd("Engine Lock State %d", engineLockState);
    EngineLockState lockState = convertEngineLockState(engineLockState);
    // allowing to set engine lock state to INVALID to
    // handle backward compatibility in cases of older modem
    if (lockState != getEngineLockState()) {
        setEngineLockState(lockState);
        LocApiBase::reportEngineLockStatus(lockState);
    }
}

void LocApiV02::reportEngDebugDataInfo(const qmiLocEngineDebugDataIndMsgT_v02*
        pLocEngDbgDataInfoIndMsg) {
    GnssEngineDebugDataInfo gnssEngineDebugDataInfo = {};

    if (pLocEngDbgDataInfoIndMsg->week_valid) {
        gnssEngineDebugDataInfo.timeValid = 1;
        gnssEngineDebugDataInfo.gpsWeek = pLocEngDbgDataInfoIndMsg->week;
    }

    if (pLocEngDbgDataInfoIndMsg->timeOfWeek_valid) {
        gnssEngineDebugDataInfo.gpsTowMs = pLocEngDbgDataInfoIndMsg->timeOfWeek;
    }

    if (pLocEngDbgDataInfoIndMsg->sourceOfTime_valid) {
        gnssEngineDebugDataInfo.sourceOfTime = pLocEngDbgDataInfoIndMsg->sourceOfTime;
    }

    if (pLocEngDbgDataInfoIndMsg->clkTimeUnc_valid) {
        gnssEngineDebugDataInfo.clkTimeUnc = pLocEngDbgDataInfoIndMsg->clkTimeUnc;
    }

    if (pLocEngDbgDataInfoIndMsg->clkFreqBias_valid) {
        gnssEngineDebugDataInfo.clkFreqBias = pLocEngDbgDataInfoIndMsg->clkFreqBias;
    }

    if (pLocEngDbgDataInfoIndMsg->clkFreqUnc_valid) {
        gnssEngineDebugDataInfo.clkFreqUnc = pLocEngDbgDataInfoIndMsg->clkFreqUnc;
    }

    if (pLocEngDbgDataInfoIndMsg->xoState_valid) {
        gnssEngineDebugDataInfo.xoState = pLocEngDbgDataInfoIndMsg->xoState;
    }

    if (pLocEngDbgDataInfoIndMsg->rcvrErrRecovery_valid) {
        gnssEngineDebugDataInfo.rcvrErrRecovery = pLocEngDbgDataInfoIndMsg->rcvrErrRecovery;
    }

    if (pLocEngDbgDataInfoIndMsg->leapSecondInfo_valid) {
        gnssEngineDebugDataInfo.leapSecondInfo.size = sizeof(Gnss_LeapSecondInfoStructType);
        gnssEngineDebugDataInfo.leapSecondInfo.leapSec =
            pLocEngDbgDataInfoIndMsg->leapSecondInfo.leapSec;
        gnssEngineDebugDataInfo.leapSecondInfo.leapSecUnc =
            pLocEngDbgDataInfoIndMsg->leapSecondInfo.leapSecUnc;
    }

    if (pLocEngDbgDataInfoIndMsg->jammedSignalsMask_valid) {
        gnssEngineDebugDataInfo.jammedSignalsMask = pLocEngDbgDataInfoIndMsg->jammedSignalsMask;
    }

    if (pLocEngDbgDataInfoIndMsg->jammerIndicatorList_valid) {
        // initalize the bpMetricDb as invalid for all signal types
        // modem does not report jammerData for signal types if they are invalid as earlier.
        gnssEngineDebugDataInfo.jammerInd.resize(GNSS_LOC_MAX_NUMBER_OF_SIGNAL_TYPES, INT32_MAX);
        for (int i = 0; i < pLocEngDbgDataInfoIndMsg->jammerIndicatorList_len; i++) {
            // get the signal type index from the signalTypeMask
            int signalTypeIndex =
                log2(pLocEngDbgDataInfoIndMsg->jammerIndicatorList[i].gnssSignalType);
                if (gnssEngineDebugDataInfo.jammerInd.size() > signalTypeIndex) {
                    gnssEngineDebugDataInfo.jammerInd[signalTypeIndex] =
                        pLocEngDbgDataInfoIndMsg->jammerIndicatorList[i].bpMetricDb;
                }
        }
    }

    if (pLocEngDbgDataInfoIndMsg->epiTime_valid) {
        gnssEngineDebugDataInfo.epiTime.hours = pLocEngDbgDataInfoIndMsg->epiTime.hours;
        gnssEngineDebugDataInfo.epiTime.mins = pLocEngDbgDataInfoIndMsg->epiTime.mins;
        gnssEngineDebugDataInfo.epiTime.secs = pLocEngDbgDataInfoIndMsg->epiTime.secs;
    }

    if (pLocEngDbgDataInfoIndMsg->epiLat_valid && pLocEngDbgDataInfoIndMsg->epiLon_valid
            && pLocEngDbgDataInfoIndMsg->epiHepe_valid) {
        gnssEngineDebugDataInfo.epiValidity |= 1 << 0;
        gnssEngineDebugDataInfo.epiLat = pLocEngDbgDataInfoIndMsg->epiLat;
        gnssEngineDebugDataInfo.epiLon = pLocEngDbgDataInfoIndMsg->epiLon;
        gnssEngineDebugDataInfo.epiHepe = pLocEngDbgDataInfoIndMsg->epiHepe;
    }

    if (pLocEngDbgDataInfoIndMsg->epiAlt_valid && pLocEngDbgDataInfoIndMsg->epiAltUnc_valid) {
        gnssEngineDebugDataInfo.epiValidity |= 1 << 1;
        gnssEngineDebugDataInfo.epiAlt = pLocEngDbgDataInfoIndMsg->epiAlt;
        gnssEngineDebugDataInfo.epiAltUnc = pLocEngDbgDataInfoIndMsg->epiAltUnc;
    }

    if (pLocEngDbgDataInfoIndMsg->epiSrc_valid) {
        gnssEngineDebugDataInfo.epiValidity |= 1 << 2;
        gnssEngineDebugDataInfo.epiSrc = pLocEngDbgDataInfoIndMsg->epiSrc;
    }

    if (pLocEngDbgDataInfoIndMsg->bestPosTime_valid) {
        gnssEngineDebugDataInfo.bestPosTime.hours = pLocEngDbgDataInfoIndMsg->bestPosTime.hours;
        gnssEngineDebugDataInfo.bestPosTime.mins = pLocEngDbgDataInfoIndMsg->bestPosTime.mins;
        gnssEngineDebugDataInfo.bestPosTime.secs = pLocEngDbgDataInfoIndMsg->bestPosTime.secs;
    }

    if (pLocEngDbgDataInfoIndMsg->bestPosLat_valid) {
        gnssEngineDebugDataInfo.bestPosLat = pLocEngDbgDataInfoIndMsg->bestPosLat;
    }

    if (pLocEngDbgDataInfoIndMsg->bestPosLon_valid) {
        gnssEngineDebugDataInfo.bestPosLon = pLocEngDbgDataInfoIndMsg->bestPosLon;
    }

    if (pLocEngDbgDataInfoIndMsg->bestPosAlt_valid) {
        gnssEngineDebugDataInfo.bestPosAlt = pLocEngDbgDataInfoIndMsg->bestPosAlt;
    }

    if (pLocEngDbgDataInfoIndMsg->bestPosHepe_valid) {
        gnssEngineDebugDataInfo.bestPosHepe = pLocEngDbgDataInfoIndMsg->bestPosHepe;
    }

    if (pLocEngDbgDataInfoIndMsg->bestPosAltUnc_valid) {
        gnssEngineDebugDataInfo.bestPosAltUnc = pLocEngDbgDataInfoIndMsg->bestPosAltUnc;
    }

    if (pLocEngDbgDataInfoIndMsg->xtraInfoTime_valid) {
        gnssEngineDebugDataInfo.xtraInfoTime.hours = pLocEngDbgDataInfoIndMsg->xtraInfoTime.hours;
        gnssEngineDebugDataInfo.xtraInfoTime.mins = pLocEngDbgDataInfoIndMsg->xtraInfoTime.mins;
        gnssEngineDebugDataInfo.xtraInfoTime.secs = pLocEngDbgDataInfoIndMsg->xtraInfoTime.secs;
    }

    if (pLocEngDbgDataInfoIndMsg->gpsXtraAge_valid) {
        gnssEngineDebugDataInfo.gpsXtraAge = pLocEngDbgDataInfoIndMsg->gpsXtraAge;
    }

    if (pLocEngDbgDataInfoIndMsg->gloXtraAge_valid) {
        gnssEngineDebugDataInfo.gloXtraAge = pLocEngDbgDataInfoIndMsg->gloXtraAge;
    }

    if (pLocEngDbgDataInfoIndMsg->bdsXtraAge_valid) {
        gnssEngineDebugDataInfo.bdsXtraAge = pLocEngDbgDataInfoIndMsg->bdsXtraAge;
    }

    if (pLocEngDbgDataInfoIndMsg->galXtraAge_valid) {
        gnssEngineDebugDataInfo.galXtraAge = pLocEngDbgDataInfoIndMsg->galXtraAge;
    }

    if (pLocEngDbgDataInfoIndMsg->qzssXtraAge_valid) {
        gnssEngineDebugDataInfo.qzssXtraAge = pLocEngDbgDataInfoIndMsg->qzssXtraAge;
    }

    if (pLocEngDbgDataInfoIndMsg->navicXtraAge_valid) {
        gnssEngineDebugDataInfo.navicXtraAge = pLocEngDbgDataInfoIndMsg->navicXtraAge;
    }

    if (pLocEngDbgDataInfoIndMsg->gpsXtraMask_valid) {
        gnssEngineDebugDataInfo.xtraValidMask =
            gnssEngineDebugDataInfo.xtraValidMask | GPS_XTRA_VALID_BIT;
        gnssEngineDebugDataInfo.gpsXtraMask = pLocEngDbgDataInfoIndMsg->gpsXtraMask;
    }

    if (pLocEngDbgDataInfoIndMsg->gloXtraMask_valid) {
        gnssEngineDebugDataInfo.xtraValidMask =
            gnssEngineDebugDataInfo.xtraValidMask | GLONASS_XTRA_VALID_BIT;
        gnssEngineDebugDataInfo.gloXtraMask = pLocEngDbgDataInfoIndMsg->gloXtraMask;
    }

    if (pLocEngDbgDataInfoIndMsg->bdsXtraMask_valid) {
        gnssEngineDebugDataInfo.xtraValidMask =
            gnssEngineDebugDataInfo.xtraValidMask | BDS_XTRA_VALID_BIT;
        gnssEngineDebugDataInfo.bdsXtraMask = pLocEngDbgDataInfoIndMsg->bdsXtraMask;
    }

    if (pLocEngDbgDataInfoIndMsg->galXtraMask_valid) {
        gnssEngineDebugDataInfo.xtraValidMask =
            gnssEngineDebugDataInfo.xtraValidMask | GAL_XTRA_VALID_BIT;
        gnssEngineDebugDataInfo.galXtraMask = pLocEngDbgDataInfoIndMsg->galXtraMask;
    }

    if (pLocEngDbgDataInfoIndMsg->qzssXtraMask_valid) {
        gnssEngineDebugDataInfo.xtraValidMask =
            gnssEngineDebugDataInfo.xtraValidMask | QZSS_XTRA_VALID_BIT;
        gnssEngineDebugDataInfo.qzssXtraMask = pLocEngDbgDataInfoIndMsg->qzssXtraMask;
    }

    if (pLocEngDbgDataInfoIndMsg->navicXtraMask_valid) {
        gnssEngineDebugDataInfo.xtraValidMask =
            gnssEngineDebugDataInfo.xtraValidMask | NAVIC_XTRA_VALID_BIT;
        gnssEngineDebugDataInfo.navicXtraMask = pLocEngDbgDataInfoIndMsg->navicXtraMask;
    }

    if (pLocEngDbgDataInfoIndMsg->ephInfoTime_valid) {
        gnssEngineDebugDataInfo.ephInfoTime.hours = pLocEngDbgDataInfoIndMsg->ephInfoTime.hours;
        gnssEngineDebugDataInfo.ephInfoTime.mins = pLocEngDbgDataInfoIndMsg->ephInfoTime.mins;
        gnssEngineDebugDataInfo.ephInfoTime.secs = pLocEngDbgDataInfoIndMsg->ephInfoTime.secs;
    }

    if (pLocEngDbgDataInfoIndMsg->gpsEphMask_valid) {
        gnssEngineDebugDataInfo.gpsEphMask = pLocEngDbgDataInfoIndMsg->gpsEphMask;
    }

    if (pLocEngDbgDataInfoIndMsg->gloEphMask_valid) {
        gnssEngineDebugDataInfo.gloEphMask = pLocEngDbgDataInfoIndMsg->gloEphMask;
    }

    if (pLocEngDbgDataInfoIndMsg->bdsEphMask_valid) {
        gnssEngineDebugDataInfo.bdsEphMask = pLocEngDbgDataInfoIndMsg->bdsEphMask;
    }

    if (pLocEngDbgDataInfoIndMsg->galEphMask_valid) {
        gnssEngineDebugDataInfo.galEphMask = pLocEngDbgDataInfoIndMsg->galEphMask;
    }

    if (pLocEngDbgDataInfoIndMsg->qzssEphMask_valid) {
        gnssEngineDebugDataInfo.qzssEphMask = pLocEngDbgDataInfoIndMsg->qzssEphMask;
    }

    if (pLocEngDbgDataInfoIndMsg->navicEphMask_valid) {
        gnssEngineDebugDataInfo.navicEphMask = pLocEngDbgDataInfoIndMsg->navicEphMask;
    }

    if (pLocEngDbgDataInfoIndMsg->healthInfoTime_valid) {
        gnssEngineDebugDataInfo.healthInfoTime.hours =
            pLocEngDbgDataInfoIndMsg->healthInfoTime.hours;
        gnssEngineDebugDataInfo.healthInfoTime.mins =
            pLocEngDbgDataInfoIndMsg->healthInfoTime.mins;
        gnssEngineDebugDataInfo.healthInfoTime.secs =
            pLocEngDbgDataInfoIndMsg->healthInfoTime.secs;
    }

    if (pLocEngDbgDataInfoIndMsg->gpsHealthUnknownMask_valid) {
        gnssEngineDebugDataInfo.gpsHealthUnknownMask =
                pLocEngDbgDataInfoIndMsg->gpsHealthUnknownMask;
    }

    if (pLocEngDbgDataInfoIndMsg->gloHealthUnknownMask_valid) {
        gnssEngineDebugDataInfo.gloHealthUnknownMask =
                pLocEngDbgDataInfoIndMsg->gloHealthUnknownMask;
    }

    if (pLocEngDbgDataInfoIndMsg->bdsHealthUnknownMask_valid) {
        gnssEngineDebugDataInfo.bdsHealthUnknownMask =
                pLocEngDbgDataInfoIndMsg->bdsHealthUnknownMask;
    }

    if (pLocEngDbgDataInfoIndMsg->galHealthUnknownMask_valid) {
        gnssEngineDebugDataInfo.galHealthUnknownMask =
                pLocEngDbgDataInfoIndMsg->galHealthUnknownMask;
    }

    if (pLocEngDbgDataInfoIndMsg->qzssHealthUnknownMask_valid) {
        gnssEngineDebugDataInfo.qzssHealthUnknownMask =
                pLocEngDbgDataInfoIndMsg->qzssHealthUnknownMask;
    }

    if (pLocEngDbgDataInfoIndMsg->navicHealthUnknownMask_valid) {
        gnssEngineDebugDataInfo.navicHealthUnknownMask =
                pLocEngDbgDataInfoIndMsg->navicHealthUnknownMask;
    }

    if (pLocEngDbgDataInfoIndMsg->gpsHealthGoodMask_valid) {
        gnssEngineDebugDataInfo.gpsHealthGoodMask = pLocEngDbgDataInfoIndMsg->gpsHealthGoodMask;
    }

    if (pLocEngDbgDataInfoIndMsg->gloHealthGoodMask_valid) {
        gnssEngineDebugDataInfo.gloHealthGoodMask = pLocEngDbgDataInfoIndMsg->gloHealthGoodMask;
    }

    if (pLocEngDbgDataInfoIndMsg->bdsHealthGoodMask_valid) {
        gnssEngineDebugDataInfo.bdsHealthGoodMask = pLocEngDbgDataInfoIndMsg->bdsHealthGoodMask;
    }

    if (pLocEngDbgDataInfoIndMsg->galHealthGoodMask_valid) {
        gnssEngineDebugDataInfo.galHealthGoodMask = pLocEngDbgDataInfoIndMsg->galHealthGoodMask;
    }

    if (pLocEngDbgDataInfoIndMsg->qzssHealthGoodMask_valid) {
        gnssEngineDebugDataInfo.qzssHealthGoodMask = pLocEngDbgDataInfoIndMsg->qzssHealthGoodMask;
    }

    if (pLocEngDbgDataInfoIndMsg->navicHealthGoodMask_valid) {
        gnssEngineDebugDataInfo.navicHealthGoodMask = pLocEngDbgDataInfoIndMsg->navicHealthGoodMask;
    }


    if (pLocEngDbgDataInfoIndMsg->gpsHealthBadMask_valid) {
        gnssEngineDebugDataInfo.gpsHealthBadMask = pLocEngDbgDataInfoIndMsg->gpsHealthBadMask;
    }

    if (pLocEngDbgDataInfoIndMsg->gloHealthBadMask_valid) {
        gnssEngineDebugDataInfo.gloHealthBadMask = pLocEngDbgDataInfoIndMsg->gloHealthBadMask;
    }

    if (pLocEngDbgDataInfoIndMsg->bdsHealthBadMask_valid) {
        gnssEngineDebugDataInfo.bdsHealthBadMask = pLocEngDbgDataInfoIndMsg->bdsHealthBadMask;
    }

    if (pLocEngDbgDataInfoIndMsg->galHealthBadMask_valid) {
        gnssEngineDebugDataInfo.galHealthBadMask = pLocEngDbgDataInfoIndMsg->galHealthBadMask;
    }

    if (pLocEngDbgDataInfoIndMsg->qzssHealthBadMask_valid) {
        gnssEngineDebugDataInfo.qzssHealthBadMask = pLocEngDbgDataInfoIndMsg->qzssHealthBadMask;
    }

    if (pLocEngDbgDataInfoIndMsg->navicHealthBadMask_valid) {
        gnssEngineDebugDataInfo.navicHealthBadMask = pLocEngDbgDataInfoIndMsg->navicHealthBadMask;
    }

    if (pLocEngDbgDataInfoIndMsg->fixInfoTime_valid) {
        gnssEngineDebugDataInfo.fixInfoTime.hours = pLocEngDbgDataInfoIndMsg->fixInfoTime.hours;
        gnssEngineDebugDataInfo.fixInfoTime.mins = pLocEngDbgDataInfoIndMsg->fixInfoTime.mins;
        gnssEngineDebugDataInfo.fixInfoTime.secs = pLocEngDbgDataInfoIndMsg->fixInfoTime.secs;
    }

    if (pLocEngDbgDataInfoIndMsg->fixInfoMask_valid) {
        gnssEngineDebugDataInfo.fixInfoMask = pLocEngDbgDataInfoIndMsg->fixInfoMask;

        secFixInfoMask = gnssEngineDebugDataInfo.fixInfoMask; //SEC
    }

    if (pLocEngDbgDataInfoIndMsg->navDataTime_valid) {
        gnssEngineDebugDataInfo.navDataTime.hours = pLocEngDbgDataInfoIndMsg->navDataTime.hours;
        gnssEngineDebugDataInfo.navDataTime.mins = pLocEngDbgDataInfoIndMsg->navDataTime.mins;
        gnssEngineDebugDataInfo.navDataTime.secs = pLocEngDbgDataInfoIndMsg->navDataTime.secs;
    }

    gnssEngineDebugDataInfo.navDataLen = 0;
    if (pLocEngDbgDataInfoIndMsg->navData_valid) {
        for (int i = 0; i < pLocEngDbgDataInfoIndMsg->navData_len ; i++) {
            gnssEngineDebugDataInfo.navData[i].gnssSvId =
                pLocEngDbgDataInfoIndMsg->navData[i].gnssSvId;
            gnssEngineDebugDataInfo.navData[i].type = pLocEngDbgDataInfoIndMsg->navData[i].type;
            gnssEngineDebugDataInfo.navData[i].src = pLocEngDbgDataInfoIndMsg->navData[i].src;
            gnssEngineDebugDataInfo.navData[i].age = pLocEngDbgDataInfoIndMsg->navData[i].age;
        }
        gnssEngineDebugDataInfo.navDataLen = pLocEngDbgDataInfoIndMsg->navData_len;
    }

    if (pLocEngDbgDataInfoIndMsg->fixStatusTime_valid) {
        gnssEngineDebugDataInfo.fixStatusTime.hours = pLocEngDbgDataInfoIndMsg->fixStatusTime.hours;
        gnssEngineDebugDataInfo.fixStatusTime.mins = pLocEngDbgDataInfoIndMsg->fixStatusTime.mins;
        gnssEngineDebugDataInfo.fixStatusTime.secs = pLocEngDbgDataInfoIndMsg->fixStatusTime.secs;
    }

    if (pLocEngDbgDataInfoIndMsg->fixStatusMask_valid) {
        gnssEngineDebugDataInfo.fixStatusMask = pLocEngDbgDataInfoIndMsg->fixStatusMask;
    }

    if (pLocEngDbgDataInfoIndMsg->fixHepeLimit_valid) {
        gnssEngineDebugDataInfo.fixHepeLimit = pLocEngDbgDataInfoIndMsg->fixHepeLimit;
    }

    LocApiBase::reportEngDebugDataInfo(gnssEngineDebugDataInfo);
}

void LocApiV02::configRobustLocation
        (bool enable, bool enableForE911, LocApiResponse *adapterResponse,
            bool enableForE911Valid) {

    sendMsg(new LocApiMsg([this, enable, enableForE911, adapterResponse, enableForE911Valid] () {

    LocationError err = LOCATION_ERROR_SUCCESS;
    qmiLocSetRobustLocationReqMsgT_v02 req;
    qmiLocGenReqStatusIndMsgT_v02 ind;
    locClientStatusEnumType status;
    locClientReqUnionType req_union;

    LOC_LOGD("configRobustLocation: enabled %d, enableForE911 %d", enable, enableForE911);
    memset(&req, 0, sizeof(req));
    memset(&ind, 0, sizeof(ind));
    req.enable = enable;
    req.enableForE911_valid = enableForE911Valid;
    req.enableForE911 = enableForE911;
    if (enable == false && enableForE911 == true) {
        LOC_LOGI("configRobustLocation: enableForE911 is not allowed when "
                 "enable is set to false");
        // change enableForE911 to false to simplify processing
        req.enableForE911 = false;
    }

    req_union.pSetRobustLocationReq = &req;
    status = locSyncSendReq(QMI_LOC_SET_ROBUST_LOCATION_CONFIG_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                            QMI_LOC_SET_ROBUST_LOCATION_CONFIG_IND_V02,
                            &ind);
    if (status != eLOC_CLIENT_SUCCESS || ind.status != eQMI_LOC_SUCCESS_V02) {
        if (status == eLOC_CLIENT_FAILURE_UNSUPPORTED ||
                status == eLOC_CLIENT_FAILURE_INVALID_MESSAGE_ID) {
            err = LOCATION_ERROR_NOT_SUPPORTED;
        } else {
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }
    }
    if (adapterResponse) {
        adapterResponse->returnToSender(err);
    }
    }));
}

void LocApiV02 :: getRobustLocationConfig(uint32_t sessionId, LocApiResponse *adapterResponse)
{
    sendMsg(new LocApiMsg([this, sessionId, adapterResponse] () {

    LocationError err = LOCATION_ERROR_SUCCESS;
    locClientStatusEnumType status = eLOC_CLIENT_FAILURE_GENERAL;
    locClientReqUnionType req_union = {};
    qmiLocGetRobustLocationConfigIndMsgT_v02 getRobustLocationConfigInd = {};

    status = locSyncSendReq(QMI_LOC_GET_ROBUST_LOCATION_CONFIG_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                            QMI_LOC_GET_ROBUST_LOCATION_CONFIG_IND_V02,
                            &getRobustLocationConfigInd);

    if ((status == eLOC_CLIENT_SUCCESS) &&
        (getRobustLocationConfigInd.status == eQMI_LOC_SUCCESS_V02)) {
        err = LOCATION_ERROR_SUCCESS;
    }else {
        if (status == eLOC_CLIENT_FAILURE_UNSUPPORTED ||
                status == eLOC_CLIENT_FAILURE_INVALID_MESSAGE_ID) {
            err = LOCATION_ERROR_NOT_SUPPORTED;
        } else {
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }
    }

    if (LOCATION_ERROR_SUCCESS != err) {
        adapterResponse->returnToSender(err);
    } else {
        GnssConfig config = {};
        uint32_t robustLocationValidMask = 0;
        config.flags |= GNSS_CONFIG_FLAGS_ROBUST_LOCATION_BIT;
        if (getRobustLocationConfigInd.isEnabled_valid) {
            robustLocationValidMask |= GNSS_CONFIG_ROBUST_LOCATION_ENABLED_VALID_BIT;
        }
        if (getRobustLocationConfigInd.isEnabledForE911_valid) {
            robustLocationValidMask |= GNSS_CONFIG_ROBUST_LOCATION_ENABLED_FOR_E911_VALID_BIT;
        }
        if (getRobustLocationConfigInd.robustLocationVersion_valid) {
            robustLocationValidMask |= GNSS_CONFIG_ROBUST_LOCATION_VERSION_VALID_BIT ;
        }

        config.robustLocationConfig.validMask =
                (GnssConfigRobustLocationValidMask) robustLocationValidMask;
        config.robustLocationConfig.enabled = getRobustLocationConfigInd.isEnabled;
        config.robustLocationConfig.enabledForE911 = getRobustLocationConfigInd.isEnabledForE911;
        config.robustLocationConfig.version.major =
                getRobustLocationConfigInd.robustLocationVersion.major;
        config.robustLocationConfig.version.minor =
                getRobustLocationConfigInd.robustLocationVersion.minor;

        LOC_LOGd("session id: %d, mask: 0x%x, enabled: %d, enabledForE911: %d, version: %d %d",
                 sessionId, config.robustLocationConfig.validMask,
                 config.robustLocationConfig.enabled,
                 config.robustLocationConfig.enabledForE911,
                 config.robustLocationConfig.version.major,
                 config.robustLocationConfig.version.minor);

        LocApiBase::reportGnssConfig(sessionId, config);
    }
    }));
}

void LocApiV02::configMinGpsWeek(uint16_t minGpsWeek, LocApiResponse *adapterResponse) {

    sendMsg(new LocApiMsg([this, minGpsWeek, adapterResponse] () {

    LOC_LOGD("configMinGpsWeek: minGpsWeek %d", minGpsWeek);

    LocationError err = LOCATION_ERROR_SUCCESS;
    qmiLocSetMinGpsWeekNumberReqMsgT_v02 req = {};
    qmiLocGenReqStatusIndMsgT_v02 ind = {};
    locClientStatusEnumType status;
    locClientReqUnionType req_union = {};

    req.minGpsWeekNumber = minGpsWeek;
    req_union.pSetMinGpsWeekReq = &req;

    status = locSyncSendReq(QMI_LOC_SET_MIN_GPS_WEEK_NUMBER_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                            QMI_LOC_SET_MIN_GPS_WEEK_NUMBER_IND_V02,
                            &ind);
    if (status != eLOC_CLIENT_SUCCESS || ind.status != eQMI_LOC_SUCCESS_V02) {
        if (status == eLOC_CLIENT_FAILURE_UNSUPPORTED ||
                status == eLOC_CLIENT_FAILURE_INVALID_MESSAGE_ID) {
            err = LOCATION_ERROR_NOT_SUPPORTED;
        } else {
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }
    }
    if (adapterResponse) {
        adapterResponse->returnToSender(err);
    }
    }));
}

void LocApiV02 :: getMinGpsWeek(uint32_t sessionId, LocApiResponse *adapterResponse)
{
    sendMsg(new LocApiMsg([this, sessionId, adapterResponse] () {

    LocationError err = LOCATION_ERROR_SUCCESS;
    locClientStatusEnumType status = eLOC_CLIENT_FAILURE_GENERAL;
    locClientReqUnionType req_union = {};
    qmiLocGetMinGpsWeekNumberIndMsgT_v02 getInd = {};
    uint16_t minGpsWeek = 0;

    status = locSyncSendReq(QMI_LOC_GET_MIN_GPS_WEEK_NUMBER_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                            QMI_LOC_GET_MIN_GPS_WEEK_NUMBER_IND_V02,
                            &getInd);

    if ((status == eLOC_CLIENT_SUCCESS) && (getInd.status == eQMI_LOC_SUCCESS_V02) &&
            getInd.minGpsWeekNumber_valid) {
        minGpsWeek = getInd.minGpsWeekNumber;
        err = LOCATION_ERROR_SUCCESS;
    }else {
        if (status == eLOC_CLIENT_FAILURE_UNSUPPORTED ||
                status == eLOC_CLIENT_FAILURE_INVALID_MESSAGE_ID) {
            err = LOCATION_ERROR_NOT_SUPPORTED;
        } else {
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }
    }

    if (LOCATION_ERROR_SUCCESS != err) {
        adapterResponse->returnToSender(err);
    } else {
        GnssConfig config = {};
        config.flags |= GNSS_CONFIG_FLAGS_MIN_GPS_WEEK_BIT;
        config.minGpsWeek = minGpsWeek;
        LOC_LOGD("getMinGpsWeek: session id %d, minGpsWeek %d",
                 sessionId, minGpsWeek);
        LocApiBase::reportGnssConfig(sessionId, config);
    }

    }));
}

LocationError LocApiV02::setParameterSync(const GnssConfig & gnssConfig) {
    LocationError err = LOCATION_ERROR_NOT_SUPPORTED;
    qmiLocSetParameterReqMsgT_v02 req = {};
    qmiLocGenReqStatusIndMsgT_v02 ind = {};
    locClientStatusEnumType status = eLOC_CLIENT_FAILURE_GENERAL;
    locClientReqUnionType req_union = {};

    req.paramType = eQMI_LOC_PARAMETER_TYPE_RESERVED_V02;
    if (gnssConfig.flags & GNSS_CONFIG_FLAGS_MIN_SV_ELEVATION_BIT) {
        req.paramType = eQMI_LOC_PARAMETER_TYPE_MINIMUM_SV_ELEVATION_V02;
        req.minSvElevation_valid = 1;
        req.minSvElevation = gnssConfig.minSvElevation;
        LOC_LOGd("set min sv elevation to %d", req.minSvElevation);
    }

    if (req.paramType != eQMI_LOC_PARAMETER_TYPE_RESERVED_V02) {
        req_union.pSetParameterReq = &req;
        status = locSyncSendReq(QMI_LOC_SET_PARAMETER_REQ_V02,
                                req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                                QMI_LOC_SET_PARAMETER_IND_V02,
                                &ind);
        if (status != eLOC_CLIENT_SUCCESS || ind.status != eQMI_LOC_SUCCESS_V02) {
            if (status == eLOC_CLIENT_FAILURE_UNSUPPORTED ||
                    status == eLOC_CLIENT_FAILURE_INVALID_MESSAGE_ID) {
                err = LOCATION_ERROR_NOT_SUPPORTED;
            } else {
                err = LOCATION_ERROR_GENERAL_FAILURE;
            }
        } else {
            err = LOCATION_ERROR_SUCCESS;
        }
    }

    return err;
}

void LocApiV02 :: getParameter(uint32_t sessionId, GnssConfigFlagsMask flags,
                               LocApiResponse* adapterResponse) {

    LOC_LOGd ("get parameter 0x%x", flags);
    sendMsg(new LocApiMsg([this, sessionId, flags, adapterResponse] () {

    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
    locClientStatusEnumType status = eLOC_CLIENT_FAILURE_GENERAL;
    locClientReqUnionType req_union = {};
    qmiLocGetParameterReqMsgT_v02 getParameterReq = {};
    qmiLocGetParameterIndMsgT_v02 getParameterInd = {};
    GnssConfig config = {};

    do {
        getParameterReq.paramType = eQMI_LOC_PARAMETER_TYPE_RESERVED_V02;
        switch (flags) {
        case GNSS_CONFIG_FLAGS_MIN_SV_ELEVATION_BIT:
            getParameterReq.paramType = eQMI_LOC_PARAMETER_TYPE_MINIMUM_SV_ELEVATION_V02;
            break;
        default:
            break;
        }

        if (getParameterReq.paramType == eQMI_LOC_PARAMETER_TYPE_RESERVED_V02) {
            err = LOCATION_ERROR_NOT_SUPPORTED;
            break;
        }

        req_union.pGetParameterReq = &getParameterReq;
        status = locSyncSendReq(QMI_LOC_GET_PARAMETER_REQ_V02,
                                req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                                QMI_LOC_GET_PARAMETER_IND_V02,
                                &getParameterInd);

        if (!((status == eLOC_CLIENT_SUCCESS) &&
              (getParameterInd.status == eQMI_LOC_SUCCESS_V02))) {
            if (status == eLOC_CLIENT_FAILURE_UNSUPPORTED ||
                    status == eLOC_CLIENT_FAILURE_INVALID_MESSAGE_ID) {
                err = LOCATION_ERROR_NOT_SUPPORTED;
            } else {
                err = LOCATION_ERROR_GENERAL_FAILURE;
            }
            break;
        }

        switch (flags) {
        case GNSS_CONFIG_FLAGS_MIN_SV_ELEVATION_BIT:
            if ((getParameterInd.paramType == eQMI_LOC_PARAMETER_TYPE_MINIMUM_SV_ELEVATION_V02) &&
                    (getParameterInd.minSvElevation_valid == 1)) {
                config.flags = GNSS_CONFIG_FLAGS_MIN_SV_ELEVATION_BIT;
                config.minSvElevation = getParameterInd.minSvElevation;
                err = LOCATION_ERROR_SUCCESS;
            }
            break;
        default:
            break;
        }
    } while (0);

    if (config.flags) {
        LocApiBase::reportGnssConfig(sessionId, config);
    } else {
        // return to the caller on failure
        if (adapterResponse) {
            adapterResponse->returnToSender(err);
        }
    }

    }));
}

void LocApiV02 :: setTribandState(bool enabled) {
    sendMsg(new LocApiMsg([this, enabled] () {
    locClientStatusEnumType result = eLOC_CLIENT_SUCCESS;
    locClientReqUnionType req_union;

    qmiLocSetTribandStateReqMsgT_v02 triband_set_req;
    qmiLocGenReqStatusIndMsgT_v02 triband_set_ind;

    LOC_LOGD("setTribandState: enabled = %d", enabled);
    memset(&triband_set_req, 0, sizeof(triband_set_req));
    memset(&triband_set_ind, 0, sizeof(triband_set_ind));

    triband_set_req.activate_valid = true;
    triband_set_req.activate = enabled;

    req_union.pSetTribandStateReq = &triband_set_req;
    result = locSyncSendReq(QMI_LOC_SET_TRIBAND_STATE_REQ_V02,
                          req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                          QMI_LOC_SET_TRIBAND_STATE_IND_V02,
                          &triband_set_ind);
    }));
}

bool LocApiV02 :: cacheGnssMeasurementSupport()
{
    bool gnssMeasurementSupported = false;

    /*for GNSS Measurement service, use
      QMI_LOC_SET_GNSS_CONSTELL_REPORT_CONFIG_V02
      to check if modem support this feature or not*/

    if (LOC_API_ADAPTER_ERR_SUCCESS ==
        setSvMeasurementConstellation(QMI_LOC_EVENT_MASK_GNSS_MEASUREMENT_REPORT_V02 |
                                      QMI_LOC_EVENT_MASK_GNSS_NHZ_MEASUREMENT_REPORT_V02)) {
        gnssMeasurementSupported = true;
    }

    LOC_LOGd("gnssMeasurementSupported is %d\n", gnssMeasurementSupported);

    return gnssMeasurementSupported;
}

locClientStatusEnumType LocApiV02::locSyncSendReq(uint32_t req_id,
        locClientReqUnionType req_payload, uint32_t timeout_msec,
        uint32_t ind_id, void* ind_payload_ptr) {

    LOC_LOGd("send req: %s, timeout msec %d", loc_get_v02_event_name(req_id),
             timeout_msec);

    locClientStatusEnumType status = loc_sync_send_req(clientHandle, req_id, req_payload,
            timeout_msec, ind_id, ind_payload_ptr);

    qmiLocStatusEnumT_v02 ind_status = eQMI_LOC_SUCCESS_V02;
    if (nullptr != ind_payload_ptr) {
        ind_status = *(qmiLocStatusEnumT_v02*)ind_payload_ptr;
    }

    if (eLOC_CLIENT_FAILURE_ENGINE_BUSY == status ||
            (eLOC_CLIENT_SUCCESS == status && eQMI_LOC_ENGINE_BUSY_V02 == ind_status)) {
        if (((eQMI_LOC_POWER_STATE_RESUME_V02 == mPlatformPowerState) ||
                (eQMI_LOC_POWER_STATE_DEEP_SLEEP_EXIT_V02 == mPlatformPowerState)) &&
                mResenders.empty() && ((mQmiMask & QMI_LOC_EVENT_MASK_ENGINE_STATE_V02) == 0)) {
            locClientRegisterEventMask(clientHandle,
                                       mQmiMask | QMI_LOC_EVENT_MASK_ENGINE_STATE_V02, isMaster());
        }
        LOC_LOGd("Engine busy, cache req: %d", req_id);
        uint32_t reqLen = 0;
        void* pReqData = nullptr;
        locClientReqUnionType req_payload_copy = {nullptr};
        validateRequest(req_id, req_payload, &pReqData, &reqLen);
        if (nullptr != pReqData) {
            req_payload_copy.pReqData = malloc(reqLen);
            if (nullptr != req_payload_copy.pReqData) {
                memcpy(req_payload_copy.pReqData, pReqData, reqLen);
            }
        }
        // something would be wrong if (nullptr != pReqData && nullptr == req_payload_copy)
        if (nullptr == pReqData || nullptr != req_payload_copy.pReqData) {
            mResenders.push_back([=](){
                    // ignore indicator, we use nullptr as the last parameter
                    loc_sync_send_req(clientHandle, req_id, req_payload_copy,
                                      timeout_msec, ind_id, nullptr);
                    if (nullptr != req_payload_copy.pReqData) {
                        free(req_payload_copy.pReqData);
                    }
                });
        }
    }

    if (status != eLOC_CLIENT_SUCCESS || eQMI_LOC_SUCCESS_V02 != ind_status) {
        LOC_LOGe("req %s, status %s, qmi ind status %s",
                 loc_get_v02_event_name(req_id),
                 loc_get_v02_client_status_name(status),
                 loc_get_v02_qmi_status_name(ind_status));
    }
    return status;
}

void LocApiV02 ::
handleWwanZppFixIndication(const qmiLocGetAvailWwanPositionIndMsgT_v02& zpp_ind)
{
    LocGpsLocation zppLoc;
    memset(&zppLoc, 0, sizeof(zppLoc));

    LOC_LOGD("Got Wwan Zpp fix location validity (lat:%d, lon:%d, timestamp:%d accuracy:%d)\n "
             "(%.7f, %.7f), timestamp %" PRIu64 ", accuracy %f",
             zpp_ind.latitude_valid,
             zpp_ind.longitude_valid,
             zpp_ind.timestampUtc_valid,
             zpp_ind.horUncCircular_valid,
             zpp_ind.latitude,
             zpp_ind.longitude,
             zpp_ind.timestampUtc,
             zpp_ind.horUncCircular);

    if ((zpp_ind.latitude_valid == false) ||
        (zpp_ind.longitude_valid == false) ||
        (zpp_ind.horUncCircular_valid == false)) {
        LOC_LOGE(" Location not valid lat=%u lon=%u unc=%u",
                 zpp_ind.latitude_valid,
                 zpp_ind.longitude_valid,
                 zpp_ind.horUncCircular_valid);
    } else {

        zppLoc.size = sizeof(LocGpsLocation);
        if (zpp_ind.timestampUtc_valid) {
            zppLoc.timestamp = zpp_ind.timestampUtc;
        } else {
            /* The UTC time from modem is not valid.
            In this case, we use current system time instead.*/

            struct timespec time_info_current = {};
            clock_gettime(CLOCK_REALTIME,&time_info_current);
            zppLoc.timestamp = (time_info_current.tv_sec)*1e3 +
                               (time_info_current.tv_nsec)/1e6;
            LOC_LOGD("zpp timestamp got from system: %" PRIu64, zppLoc.timestamp);
        }

        zppLoc.flags = LOC_GPS_LOCATION_HAS_LAT_LONG | LOC_GPS_LOCATION_HAS_ACCURACY;
        zppLoc.latitude = zpp_ind.latitude;
        zppLoc.longitude = zpp_ind.longitude;
        zppLoc.accuracy = zpp_ind.horUncCircular;

        // If horCircularConfidence_valid is true, and horCircularConfidence value
        // is less than 68%, then scale the accuracy value to 68% confidence.
        if (zpp_ind.horCircularConfidence_valid)
        {
            scaleAccuracyTo68PercentConfidence(zpp_ind.horCircularConfidence,
                                               zppLoc, true);
        }

        if (zpp_ind.altitudeWrtEllipsoid_valid) {
            zppLoc.flags |= LOC_GPS_LOCATION_HAS_ALTITUDE;
            zppLoc.altitude = zpp_ind.altitudeWrtEllipsoid;
        }

        if (zpp_ind.vertUnc_valid) {
            zppLoc.flags |= LOC_GPS_LOCATION_HAS_VERT_UNCERTAINITY;
            zppLoc.vertUncertainity = zpp_ind.vertUnc;
        }
    }

    LocApiBase::reportWwanZppFix(zppLoc);
}

void LocApiV02::
    handleZppBestAvailableFixIndication(const qmiLocGetBestAvailablePositionIndMsgT_v02 &zpp_ind)
{
    LocGpsLocation zppLoc;
    GpsLocationExtended location_extended;
    LocPosTechMask tech_mask;

    memset(&zppLoc, 0, sizeof(zppLoc));
    zppLoc.size = sizeof(zppLoc);

    memset(&location_extended, 0, sizeof(location_extended));
    location_extended.size = sizeof(location_extended);

    tech_mask = LOC_POS_TECH_MASK_DEFAULT;

    LOC_LOGD("Got Zpp fix location validity (lat:%d, lon:%d, timestamp:%d accuracy:%d)"
            " (%.7f, %.7f), timestamp %" PRIu64 ", accuracy %f",
            zpp_ind.latitude_valid,
            zpp_ind.longitude_valid,
            zpp_ind.timestampUtc_valid,
            zpp_ind.horUncCircular_valid,
            zpp_ind.latitude,
            zpp_ind.longitude,
            zpp_ind.timestampUtc,
            zpp_ind.horUncCircular);

        if (zpp_ind.timestampUtc_valid) {
            zppLoc.timestamp = zpp_ind.timestampUtc;
        } else {
            /* The UTC time from modem is not valid.
                    In this case, we use current system time instead.*/

          struct timespec time_info_current = {};
          clock_gettime(CLOCK_REALTIME,&time_info_current);
          zppLoc.timestamp = (time_info_current.tv_sec)*1e3 +
                  (time_info_current.tv_nsec)/1e6;
          LOC_LOGD("zpp timestamp got from system: %" PRIu64, zppLoc.timestamp);
        }

        if (zpp_ind.latitude_valid && zpp_ind.longitude_valid &&
                zpp_ind.horUncCircular_valid ) {
            zppLoc.flags = LOC_GPS_LOCATION_HAS_LAT_LONG | LOC_GPS_LOCATION_HAS_ACCURACY;
            zppLoc.latitude = zpp_ind.latitude;
            zppLoc.longitude = zpp_ind.longitude;
            zppLoc.accuracy = zpp_ind.horUncCircular;

            // If horCircularConfidence_valid is true, and horCircularConfidence value
            // is less than 68%, then scale the accuracy value to 68% confidence.
            if (zpp_ind.horCircularConfidence_valid)
            {
                scaleAccuracyTo68PercentConfidence(zpp_ind.horCircularConfidence,
                                                   zppLoc, true);
            }

            if (zpp_ind.altitudeWrtEllipsoid_valid) {
                zppLoc.flags |= LOC_GPS_LOCATION_HAS_ALTITUDE;
                zppLoc.altitude = zpp_ind.altitudeWrtEllipsoid;
            }

            if (zpp_ind.horSpeed_valid) {
                zppLoc.flags |= LOC_GPS_LOCATION_HAS_SPEED;
                zppLoc.speed = zpp_ind.horSpeed;
            }

            if (zpp_ind.heading_valid) {
                zppLoc.flags |= LOC_GPS_LOCATION_HAS_BEARING;
                zppLoc.bearing = zpp_ind.heading;
            }

            if (zpp_ind.vertUnc_valid) {
                location_extended.flags |= GPS_LOCATION_EXTENDED_HAS_VERT_UNC;
                location_extended.vert_unc = zpp_ind.vertUnc;
            }

            if (zpp_ind.horSpeedUnc_valid) {
                location_extended.flags |= GPS_LOCATION_EXTENDED_HAS_SPEED_UNC;
                location_extended.speed_unc = zpp_ind.horSpeedUnc;
            }

            if (zpp_ind.headingUnc_valid) {
                location_extended.flags |= GPS_LOCATION_EXTENDED_HAS_BEARING_UNC;
                location_extended.bearing_unc = zpp_ind.headingUnc;
            }

            if (zpp_ind.technologyMask_valid) {
                tech_mask = zpp_ind.technologyMask;
            }

            if(zpp_ind.spoofReportMask_valid) {
                zppLoc.flags |= LOC_GPS_LOCATION_HAS_SPOOF_MASK;
                zppLoc.spoof_mask = (uint32_t)zpp_ind.spoofReportMask;
                LOC_LOGd("QMI_spoofReportMask:0x%x", (uint8_t)zppLoc.spoof_mask);
            }
        }

        LocApiBase::reportZppBestAvailableFix(zppLoc, location_extended, tech_mask);
}

LocPosTechMask LocApiV02 :: convertPosTechMask(
  qmiLocPosTechMaskT_v02 mask)
{
   LocPosTechMask locTechMask = LOC_POS_TECH_MASK_DEFAULT;

   if (mask & QMI_LOC_POS_TECH_MASK_SATELLITE_V02)
      locTechMask |= LOC_POS_TECH_MASK_SATELLITE;

   if (mask & QMI_LOC_POS_TECH_MASK_CELLID_V02)
      locTechMask |= LOC_POS_TECH_MASK_CELLID;

   if (mask & QMI_LOC_POS_TECH_MASK_WIFI_V02)
      locTechMask |= LOC_POS_TECH_MASK_WIFI;

   if (mask & QMI_LOC_POS_TECH_MASK_SENSORS_V02)
      locTechMask |= LOC_POS_TECH_MASK_SENSORS;

   if (mask & QMI_LOC_POS_TECH_MASK_REFERENCE_LOCATION_V02)
      locTechMask |= LOC_POS_TECH_MASK_REFERENCE_LOCATION;

   if (mask & QMI_LOC_POS_TECH_MASK_INJECTED_COARSE_POSITION_V02)
      locTechMask |= LOC_POS_TECH_MASK_INJECTED_COARSE_POSITION;

   if (mask & QMI_LOC_POS_TECH_MASK_AFLT_V02)
      locTechMask |= LOC_POS_TECH_MASK_AFLT;

   if (mask & QMI_LOC_POS_TECH_MASK_HYBRID_V02)
      locTechMask |= LOC_POS_TECH_MASK_HYBRID;

   if (mask & QMI_LOC_POS_TECH_MASK_INS_V02)
      locTechMask |= LOC_POS_TECH_MASK_INS;

   if (mask & QMI_LOC_POS_TECH_MASK_PDR_V02)
      locTechMask |= LOC_POS_TECH_MASK_PDR;

   if (mask & QMI_LOC_POS_TECH_MASK_PROPAGATED_V02)
      locTechMask |= LOC_POS_TECH_MASK_PROPAGATED;

   return locTechMask;
}

LocNavSolutionMask LocApiV02 :: convertNavSolutionMask(
  qmiLocNavSolutionMaskT_v02 mask)
{
   LocNavSolutionMask locNavMask = 0;

   if (mask & QMI_LOC_NAV_MASK_SBAS_CORRECTION_IONO_V02)
      locNavMask |= LOC_NAV_MASK_SBAS_CORRECTION_IONO;

   if (mask & QMI_LOC_NAV_MASK_SBAS_CORRECTION_FAST_V02)
      locNavMask |= LOC_NAV_MASK_SBAS_CORRECTION_FAST;

   if (mask & QMI_LOC_POS_TECH_MASK_WIFI_V02)
      locNavMask |= LOC_POS_TECH_MASK_WIFI;

   if (mask & QMI_LOC_NAV_MASK_SBAS_CORRECTION_LONG_V02)
      locNavMask |= LOC_NAV_MASK_SBAS_CORRECTION_LONG;

   if (mask & QMI_LOC_NAV_MASK_SBAS_INTEGRITY_V02)
      locNavMask |= LOC_NAV_MASK_SBAS_INTEGRITY;

   if (mask & QMI_LOC_NAV_MASK_CORRECTION_DGNSS_V02)
      locNavMask |= LOC_NAV_MASK_DGNSS_CORRECTION;

   if (mask & QMI_LOC_NAV_MASK_ONLY_SBAS_CORRECTED_SV_USED_V02)
      locNavMask |= LOC_NAV_MASK_ONLY_SBAS_CORRECTED_SV_USED;

   return locNavMask;
}

qmiLocApnTypeMaskT_v02 LocApiV02::convertLocApnTypeMask(LocApnTypeMask mask)
{
    qmiLocApnTypeMaskT_v02 qmiMask = 0;

    if (mask & LOC_APN_TYPE_MASK_DEFAULT) {
        qmiMask |= QMI_LOC_APN_TYPE_MASK_DEFAULT_V02;
    }
    if (mask & LOC_APN_TYPE_MASK_IMS) {
        qmiMask |= QMI_LOC_APN_TYPE_MASK_IMS_V02;
    }
    if (mask & LOC_APN_TYPE_MASK_MMS) {
        qmiMask |= QMI_LOC_APN_TYPE_MASK_MMS_V02;
    }
    if (mask & LOC_APN_TYPE_MASK_DUN) {
        qmiMask |= QMI_LOC_APN_TYPE_MASK_DUN_V02;
    }
    if (mask & LOC_APN_TYPE_MASK_SUPL) {
        qmiMask |= QMI_LOC_APN_TYPE_MASK_SUPL_V02;
    }
    if (mask & LOC_APN_TYPE_MASK_HIPRI) {
        qmiMask |= QMI_LOC_APN_TYPE_MASK_HIPRI_V02;
    }
    if (mask & LOC_APN_TYPE_MASK_FOTA) {
        qmiMask |= QMI_LOC_APN_TYPE_MASK_FOTA_V02;
    }
    if (mask & LOC_APN_TYPE_MASK_CBS) {
        qmiMask |= QMI_LOC_APN_TYPE_MASK_CBS_V02;
    }
    if (mask & LOC_APN_TYPE_MASK_IA) {
        qmiMask |= QMI_LOC_APN_TYPE_MASK_IA_V02;
    }
    if (mask & LOC_APN_TYPE_MASK_EMERGENCY) {
        qmiMask |= QMI_LOC_APN_TYPE_MASK_EMERGENCY_V02;
    }

    return qmiMask;
}

LocApnTypeMask LocApiV02::convertQmiLocApnTypeMask(qmiLocApnTypeMaskT_v02 qmiMask)
{
    LocApnTypeMask mask = 0;

    if (qmiMask & QMI_LOC_APN_TYPE_MASK_DEFAULT_V02) {
        mask |= LOC_APN_TYPE_MASK_DEFAULT;
    }
    if (qmiMask & QMI_LOC_APN_TYPE_MASK_IMS_V02) {
        mask |= LOC_APN_TYPE_MASK_IMS;
    }
    if (qmiMask & QMI_LOC_APN_TYPE_MASK_MMS_V02) {
        mask |= LOC_APN_TYPE_MASK_MMS;
    }
    if (qmiMask & QMI_LOC_APN_TYPE_MASK_DUN_V02) {
        mask |= LOC_APN_TYPE_MASK_DUN;
    }
    if (qmiMask & QMI_LOC_APN_TYPE_MASK_SUPL_V02) {
        mask |= LOC_APN_TYPE_MASK_SUPL;
    }
    if (qmiMask & QMI_LOC_APN_TYPE_MASK_HIPRI_V02) {
        mask |= LOC_APN_TYPE_MASK_HIPRI;
    }
    if (qmiMask & QMI_LOC_APN_TYPE_MASK_FOTA_V02) {
        mask |= LOC_APN_TYPE_MASK_FOTA;
    }
    if (qmiMask & QMI_LOC_APN_TYPE_MASK_CBS_V02) {
        mask |= LOC_APN_TYPE_MASK_CBS;
    }
    if (qmiMask & QMI_LOC_APN_TYPE_MASK_IA_V02) {
        mask |= LOC_APN_TYPE_MASK_IA;
    }
    if (qmiMask & QMI_LOC_APN_TYPE_MASK_EMERGENCY_V02) {
        mask |= LOC_APN_TYPE_MASK_EMERGENCY;
    }

    return mask;
}

GnssConfigSuplVersion
LocApiV02::convertSuplVersion(const uint32_t suplVersion)
{
    switch (suplVersion) {
        case 0x00020004:
            return GNSS_CONFIG_SUPL_VERSION_2_0_4;
        case 0x00020002:
            return GNSS_CONFIG_SUPL_VERSION_2_0_2;
        case 0x00020000:
            return GNSS_CONFIG_SUPL_VERSION_2_0_0;
        case 0x00010000:
        default:
            return GNSS_CONFIG_SUPL_VERSION_1_0_0;
    }
}

GnssConfigLppeControlPlaneMask
LocApiV02::convertLppeCp(const uint32_t lppeControlPlaneMask)
{
    GnssConfigLppeControlPlaneMask mask = 0;
    if ((1<<0) & lppeControlPlaneMask) {
        mask |= GNSS_CONFIG_LPPE_CONTROL_PLANE_DBH_BIT;
    }
    if ((1<<1) & lppeControlPlaneMask) {
        mask |= GNSS_CONFIG_LPPE_CONTROL_PLANE_WLAN_AP_MEASUREMENTS_BIT;
    }
    if ((1<<3) & lppeControlPlaneMask) {
        mask |= GNSS_CONFIG_LPPE_CONTROL_PLANE_SENSOR_BARO_MEASUREMENTS_BIT;
    }
    if ((1<<4) & lppeControlPlaneMask) {
        mask |= GNSS_CONFIG_LPPE_CONTROL_PLANE_NON_E911_BIT;
    }
    if ((1 << 5) & lppeControlPlaneMask) {
        mask |= GNSS_CONFIG_LPPE_CONTROL_PLANE_CIV_ADDRESS_BIT;
    }
    return mask;
}

GnssConfigLppeUserPlaneMask
LocApiV02::convertLppeUp(const uint32_t lppeUserPlaneMask)
{
    GnssConfigLppeUserPlaneMask mask = 0;
    if ((1 << 0) & lppeUserPlaneMask) {
        mask |= GNSS_CONFIG_LPPE_USER_PLANE_DBH_BIT;
    }
    if ((1 << 1) & lppeUserPlaneMask) {
        mask |= GNSS_CONFIG_LPPE_USER_PLANE_WLAN_AP_MEASUREMENTS_BIT;
    }
    if ((1 << 3) & lppeUserPlaneMask) {
        mask |= GNSS_CONFIG_LPPE_USER_PLANE_SENSOR_BARO_MEASUREMENTS_BIT;
    }
    if ((1 << 4) & lppeUserPlaneMask) {
        mask |= GNSS_CONFIG_LPPE_USER_PLANE_NON_E911_BIT;
    }
    if ((1 << 5) & lppeUserPlaneMask) {
        mask |= GNSS_CONFIG_LPPE_USER_PLANE_CIV_ADDRESS_BIT;
    }
    return mask;
}

LocationError
LocApiV02::setEmergencyExtensionWindowSync(const uint32_t emergencyExtensionSeconds)
{
    LocationError err = LOCATION_ERROR_SUCCESS;
    locClientStatusEnumType result = eLOC_CLIENT_SUCCESS;
    locClientReqUnionType req_union;
    qmiLocSetProtocolConfigParametersReqMsgT_v02 eCbW_req;
    qmiLocSetProtocolConfigParametersIndMsgT_v02 eCbW_ind;

    memset(&eCbW_req, 0, sizeof(eCbW_req));
    memset(&eCbW_ind, 0, sizeof(eCbW_ind));

    eCbW_req.emergencyCallbackWindow_valid = 1;
    eCbW_req.emergencyCallbackWindow = emergencyExtensionSeconds;

    req_union.pSetProtocolConfigParametersReq = &eCbW_req;

    LOC_LOGd("emergencyCallbackWindow = %d", emergencyExtensionSeconds);

    result = locSyncSendReq(QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_REQ_V02,
        req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
        QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_IND_V02,
        &eCbW_ind);

    if (result != eLOC_CLIENT_SUCCESS ||
        eQMI_LOC_SUCCESS_V02 != eCbW_ind.status)
    {
        err = LOCATION_ERROR_GENERAL_FAILURE;
    }

    return err;
}

void
LocApiV02::setMeasurementCorrections(const GnssMeasurementCorrections& gnssMeasurementCorrections)
{
    sendMsg(new LocApiMsg([this, gnssMeasurementCorrections] {

    locClientStatusEnumType result = eLOC_CLIENT_SUCCESS;
    locClientReqUnionType req_union = {};

    qmiLocEventInjectEnvAidingReqMsgT_v02 setEnvAidingReqMsg;
    qmiLocGenReqStatusIndMsgT_v02 genReqStatusIndMsg;

    memset(&setEnvAidingReqMsg, 0, sizeof(setEnvAidingReqMsg));
    memset(&genReqStatusIndMsg, 0, sizeof(genReqStatusIndMsg));

    LOC_LOGV("GnssMeasurementCorrections from modem:\n"
             " hasEnvironmentBearing = %d"
             " environmentBearingDegrees = %.2f"
             " environmentBearingUncertaintyDegrees = %.2f"
             " latitudeDegrees = %.2f"
             " longitudeDegrees = %.2f"
             " horizontalPositionUncertaintyMeters = %.2f"
             " altitudeMeters = %.2f"
             " verticalPositionUncertaintyMeters = %.2f"
             " toaGpsNanosecondsOfWeek = %" PRIu64,
             gnssMeasurementCorrections.hasEnvironmentBearing,
             gnssMeasurementCorrections.environmentBearingDegrees,
             gnssMeasurementCorrections.environmentBearingUncertaintyDegrees,
             gnssMeasurementCorrections.latitudeDegrees,
             gnssMeasurementCorrections.longitudeDegrees,
             gnssMeasurementCorrections.horizontalPositionUncertaintyMeters,
             gnssMeasurementCorrections.altitudeMeters,
             gnssMeasurementCorrections.verticalPositionUncertaintyMeters,
             gnssMeasurementCorrections.toaGpsNanosecondsOfWeek);

    for (int i = 0; i < gnssMeasurementCorrections.satCorrections.size(); i++) {
        LOC_LOGV("gnssMeasurementCorrections.satCorrections:\n"
            "satCorrections[%d].svType = %d "
            "satCorrections[%d].svId = %d "
            "satCorrections[%d].carrierFrequencyHz = %.2f "
            "satCorrections[%d].probSatIsLos = %.2f "
            "satCorrections[%d].excessPathLengthMeters = %.2f "
            "satCorrections[%d].excessPathLengthUncertaintyMeters = %.2f "
            "satCorrections[%d].reflectingPlane.latitudeDegree = %.2f "
            "satCorrections[%d].reflectingPlane.longitudeDegrees = %.2f "
            "satCorrections[%d].reflectingPlane.altitudeMeters = %.2f "
            "satCorrections[%d].reflectingPlane.azimuthDegrees = %.2f ",
            i, gnssMeasurementCorrections.satCorrections[i].svType,
            i, gnssMeasurementCorrections.satCorrections[i].svId,
            i, gnssMeasurementCorrections.satCorrections[i].carrierFrequencyHz,
            i, gnssMeasurementCorrections.satCorrections[i].probSatIsLos,
            i, gnssMeasurementCorrections.satCorrections[i].excessPathLengthMeters,
            i, gnssMeasurementCorrections.satCorrections[i].excessPathLengthUncertaintyMeters,
            i, gnssMeasurementCorrections.satCorrections[i].reflectingPlane.latitudeDegrees,
            i, gnssMeasurementCorrections.satCorrections[i].reflectingPlane.longitudeDegrees,
            i, gnssMeasurementCorrections.satCorrections[i].reflectingPlane.altitudeMeters,
            i, gnssMeasurementCorrections.satCorrections[i].reflectingPlane.azimuthDegrees);
    }

    setEnvAidingReqMsg.maxMessageNum = 1;
    setEnvAidingReqMsg.seqNum = 1;

    setEnvAidingReqMsg.envBearingValidity_valid = true;
    setEnvAidingReqMsg.envBearingValidity = gnssMeasurementCorrections.hasEnvironmentBearing;
    setEnvAidingReqMsg.envBearingDegrees_valid = true;
    setEnvAidingReqMsg.envBearingDegrees = gnssMeasurementCorrections.environmentBearingDegrees;
    setEnvAidingReqMsg.envBearingUncDegrees_valid = true;
    setEnvAidingReqMsg.envBearingUncDegrees =
            gnssMeasurementCorrections.environmentBearingUncertaintyDegrees;

    setEnvAidingReqMsg.latitudeDegrees_valid = true;
    setEnvAidingReqMsg.latitudeDegrees = gnssMeasurementCorrections.latitudeDegrees;
    setEnvAidingReqMsg.longitudeDegrees_valid = true;
    setEnvAidingReqMsg.longitudeDegrees = gnssMeasurementCorrections.longitudeDegrees;
    setEnvAidingReqMsg.horizontalPositionUncMeters_valid = true;
    setEnvAidingReqMsg.horizontalPositionUncMeters =
            gnssMeasurementCorrections.horizontalPositionUncertaintyMeters;
    setEnvAidingReqMsg.altitudeMeters_valid = true;
    setEnvAidingReqMsg.altitudeMeters = gnssMeasurementCorrections.altitudeMeters;
    setEnvAidingReqMsg.altitudeUncMeters_valid = true;
    setEnvAidingReqMsg.altitudeUncMeters =
            gnssMeasurementCorrections.verticalPositionUncertaintyMeters;
    setEnvAidingReqMsg.toaGpsNanosecondsOfWeek_valid = true;
    setEnvAidingReqMsg.toaGpsNanosecondsOfWeek =
            gnssMeasurementCorrections.toaGpsNanosecondsOfWeek;

    uint32_t len = gnssMeasurementCorrections.satCorrections.size();
    if (len > 0) {
        if (len >= QMI_LOC_ENV_AIDING_CORRECTION_MAX_SV_USED_V02) {
            setEnvAidingReqMsg.svCorrection_len = QMI_LOC_ENV_AIDING_CORRECTION_MAX_SV_USED_V02;
        } else {
            setEnvAidingReqMsg.svCorrection_len = len;
        }
        setEnvAidingReqMsg.svCorrection_valid = true;
        for (int i = 0; i < setEnvAidingReqMsg.svCorrection_len; i++) {
            setEnvAidingReqMsg.svCorrection[i].svCorrectionFlags = 0;
            if (gnssMeasurementCorrections.satCorrections[i].flags &
                (GNSS_MEAS_CORR_HAS_SAT_IS_LOS_PROBABILITY_BIT)) {
                setEnvAidingReqMsg.svCorrection[i].probabilitySvIsLineofSight =
                        gnssMeasurementCorrections.satCorrections[i].probSatIsLos;
                setEnvAidingReqMsg.svCorrection[i].svCorrectionFlags |=
                        QMI_LOC_ENV_AIDING_SV_CORRECTION_LINE_OF_SIGHT_PROBABILITY_VALID_V02;
            }
            if (gnssMeasurementCorrections.satCorrections[i].flags &
                (GNSS_MEAS_CORR_HAS_EXCESS_PATH_LENGTH_BIT)) {
                setEnvAidingReqMsg.svCorrection[i].excessPathLengthMeters =
                        gnssMeasurementCorrections.satCorrections[i].excessPathLengthMeters;
                setEnvAidingReqMsg.svCorrection[i].svCorrectionFlags |=
                        QMI_LOC_ENV_AIDING_SV_CORRECTION_EXCESS_PATH_LENGTH_VALID_V02;
            }
            if (gnssMeasurementCorrections.satCorrections[i].flags &
                (GNSS_MEAS_CORR_HAS_EXCESS_PATH_LENGTH_UNC_BIT)) {
                setEnvAidingReqMsg.svCorrection[i].excessPathLengthUncMeters =
                        gnssMeasurementCorrections.satCorrections[i].
                                excessPathLengthUncertaintyMeters;
                setEnvAidingReqMsg.svCorrection[i].svCorrectionFlags |=
                        QMI_LOC_ENV_AIDING_SV_CORRECTION_EXCESS_PATH_LENGTH_UNC_VALID_V02;
            }
            if (gnssMeasurementCorrections.satCorrections[i].flags &
                (GNSS_MEAS_CORR_HAS_REFLECTING_PLANE_BIT)) {
                setEnvAidingReqMsg.svCorrection[i].reflectingPlane.latitudeDegrees =
                        gnssMeasurementCorrections.satCorrections[i].
                                reflectingPlane.latitudeDegrees;
                setEnvAidingReqMsg.svCorrection[i].reflectingPlane.longitudeDegrees =
                        gnssMeasurementCorrections.satCorrections[i].
                                reflectingPlane.longitudeDegrees;
                setEnvAidingReqMsg.svCorrection[i].reflectingPlane.altitudeMeters =
                        gnssMeasurementCorrections.satCorrections[i].
                                reflectingPlane.altitudeMeters;
                setEnvAidingReqMsg.svCorrection[i].reflectingPlane.azimuthDegrees =
                        gnssMeasurementCorrections.satCorrections[i].
                                reflectingPlane.azimuthDegrees;
                setEnvAidingReqMsg.svCorrection[i].svCorrectionFlags |=
                        QMI_LOC_ENV_AIDING_SV_CORRECTION_REFLECTING_PLANE_VALID_V02;
            }
            switch (gnssMeasurementCorrections.satCorrections[i].svType) {
            case GNSS_SV_TYPE_GPS:
                setEnvAidingReqMsg.svCorrection[i].constellation =
                        eQMI_LOC_SV_SYSTEM_GPS_V02;
                break;

            case GNSS_SV_TYPE_SBAS:
                setEnvAidingReqMsg.svCorrection[i].constellation =
                        eQMI_LOC_SV_SYSTEM_SBAS_V02;
                break;

            case GNSS_SV_TYPE_GLONASS:
                setEnvAidingReqMsg.svCorrection[i].constellation =
                        eQMI_LOC_SV_SYSTEM_GLONASS_V02;
                break;

            case GNSS_SV_TYPE_QZSS:
                setEnvAidingReqMsg.svCorrection[i].constellation =
                        eQMI_LOC_SV_SYSTEM_QZSS_V02;
                break;

            case GNSS_SV_TYPE_BEIDOU:
                setEnvAidingReqMsg.svCorrection[i].constellation =
                        eQMI_LOC_SV_SYSTEM_BDS_V02;
                break;

            case GNSS_SV_TYPE_GALILEO:
                setEnvAidingReqMsg.svCorrection[i].constellation =
                        eQMI_LOC_SV_SYSTEM_GALILEO_V02;
                break;

            case GNSS_SV_TYPE_NAVIC:
                setEnvAidingReqMsg.svCorrection[i].constellation =
                        eQMI_LOC_SV_SYSTEM_NAVIC_V02;
                break;

            case GNSS_SV_TYPE_UNKNOWN:
            default:
                break;
            }
            setEnvAidingReqMsg.svCorrection[i].svid =
                    gnssMeasurementCorrections.satCorrections[i].svId;
            setEnvAidingReqMsg.svCorrection[i].carrierFrequencyHz =
                    gnssMeasurementCorrections.satCorrections[i].carrierFrequencyHz;
        }
    }

    req_union.pEnvAidingReqMsg = &setEnvAidingReqMsg;

    result = locSyncSendReq(QMI_LOC_INJECT_ENV_AIDING_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                            QMI_LOC_INJECT_ENV_AIDING_IND_V02,
                            &genReqStatusIndMsg);

    // < SEC_GPS
    if (result == eLOC_CLIENT_SUCCESS &&
        eQMI_LOC_SUCCESS_V02 == genReqStatusIndMsg.status)
    {
        isBlueskyInjected = true;
    }
    // SEC_GPS >

    }));
}

LocationError
LocApiV02::setBlacklistSvSync(const GnssSvIdConfig& config)
{
    locClientStatusEnumType status = eLOC_CLIENT_FAILURE_GENERAL;
    locClientReqUnionType req_union = {};

    qmiLocSetBlacklistSvReqMsgT_v02 setBlacklistSvMsg;
    qmiLocGenReqStatusIndMsgT_v02 genReqStatusIndMsg;

    // Clear all fields
    memset(&setBlacklistSvMsg, 0, sizeof(setBlacklistSvMsg));
    memset(&genReqStatusIndMsg, 0, sizeof(genReqStatusIndMsg));

    // Fill in the request details
    setBlacklistSvMsg.gps_persist_blacklist_sv_valid = true;
    setBlacklistSvMsg.gps_persist_blacklist_sv = config.gpsBlacklistSvMask,
    setBlacklistSvMsg.gps_clear_persist_blacklist_sv_valid = true;
    setBlacklistSvMsg.gps_clear_persist_blacklist_sv = ~config.gpsBlacklistSvMask;

    setBlacklistSvMsg.glo_persist_blacklist_sv_valid = true;
    setBlacklistSvMsg.glo_persist_blacklist_sv = config.gloBlacklistSvMask;
    setBlacklistSvMsg.glo_clear_persist_blacklist_sv_valid = true;
    setBlacklistSvMsg.glo_clear_persist_blacklist_sv = ~config.gloBlacklistSvMask;

    setBlacklistSvMsg.bds_persist_blacklist_sv_valid = true;
    setBlacklistSvMsg.bds_persist_blacklist_sv = config.bdsBlacklistSvMask;
    setBlacklistSvMsg.bds_clear_persist_blacklist_sv_valid = true;
    setBlacklistSvMsg.bds_clear_persist_blacklist_sv = ~config.bdsBlacklistSvMask;

    setBlacklistSvMsg.qzss_persist_blacklist_sv_valid = true;
    setBlacklistSvMsg.qzss_persist_blacklist_sv = config.qzssBlacklistSvMask;
    setBlacklistSvMsg.qzss_clear_persist_blacklist_sv_valid = true;
    setBlacklistSvMsg.qzss_clear_persist_blacklist_sv = ~config.qzssBlacklistSvMask;

    setBlacklistSvMsg.gal_persist_blacklist_sv_valid = true;
    setBlacklistSvMsg.gal_persist_blacklist_sv = config.galBlacklistSvMask;
    setBlacklistSvMsg.gal_clear_persist_blacklist_sv_valid = true;
    setBlacklistSvMsg.gal_clear_persist_blacklist_sv = ~config.galBlacklistSvMask;

    setBlacklistSvMsg.sbas_persist_blacklist_sv_valid = true;
    setBlacklistSvMsg.sbas_persist_blacklist_sv = config.sbasBlacklistSvMask,
    setBlacklistSvMsg.sbas_clear_persist_blacklist_sv_valid = true;
    setBlacklistSvMsg.sbas_clear_persist_blacklist_sv = ~config.sbasBlacklistSvMask;

    setBlacklistSvMsg.navic_persist_blacklist_sv_valid = true;
    setBlacklistSvMsg.navic_persist_blacklist_sv = config.navicBlacklistSvMask,
    setBlacklistSvMsg.navic_clear_persist_blacklist_sv_valid = true;
    setBlacklistSvMsg.navic_clear_persist_blacklist_sv = ~config.navicBlacklistSvMask;

    LOC_LOGd(">>> configConstellations, "
             "gps blacklist mask =0x%" PRIx64 ", "
             "glo blacklist mask =0x%" PRIx64 ", "
             "qzss blacklist mask =0x%" PRIx64 ",\n"
             "bds blacklist mask =0x%" PRIx64 ", "
             "gal blacklist mask =0x%" PRIx64 ",\n"
             "sbas blacklist mask =0x%" PRIx64 ", "
             "navic blacklist mask =0x%" PRIx64 ",\n",
             setBlacklistSvMsg.gps_persist_blacklist_sv,
             setBlacklistSvMsg.glo_persist_blacklist_sv,
             setBlacklistSvMsg.qzss_persist_blacklist_sv,
             setBlacklistSvMsg.bds_persist_blacklist_sv,
             setBlacklistSvMsg.gal_persist_blacklist_sv,
             setBlacklistSvMsg.sbas_persist_blacklist_sv,
             setBlacklistSvMsg.navic_persist_blacklist_sv);
    // Update in request union
    req_union.pSetBlacklistSvReq = &setBlacklistSvMsg;

    // Send the request
    status = loc_sync_send_req(clientHandle,
                               QMI_LOC_SET_BLACKLIST_SV_REQ_V02,
                               req_union,
                               LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                               QMI_LOC_SET_BLACKLIST_SV_IND_V02,
                               &genReqStatusIndMsg);
    if(status != eLOC_CLIENT_SUCCESS ||
            genReqStatusIndMsg.status != eQMI_LOC_SUCCESS_V02) {
        LOC_LOGe("Set Blacklist SV failed. status: %s ind status %s",
                 loc_get_v02_client_status_name(status),
                 loc_get_v02_qmi_status_name(genReqStatusIndMsg.status));
        return LOCATION_ERROR_GENERAL_FAILURE;
    }

    return LOCATION_ERROR_SUCCESS;
}

void
LocApiV02::setBlacklistSv(const GnssSvIdConfig& config, LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, config, adapterResponse] () {
                LocationError err = setBlacklistSvSync(config);
                if (adapterResponse) {
                    adapterResponse->returnToSender(err);
                }
            }));
}

void LocApiV02::getBlacklistSv()
{
    sendMsg(new LocApiMsg([this] () {

    locClientStatusEnumType status = eLOC_CLIENT_FAILURE_GENERAL;
    locClientReqUnionType req_union = {};

    // Send the request
    status = locClientSendReq(QMI_LOC_GET_BLACKLIST_SV_REQ_V02, req_union);

    }));
}

void
LocApiV02::setConstellationControl(const GnssSvTypeConfig& config,
                                   LocApiResponse *adapterResponse)
{
    sendMsg(new LocApiMsg([this, config, adapterResponse] () {

    locClientStatusEnumType status = eLOC_CLIENT_FAILURE_GENERAL;
    locClientReqUnionType req_union = {};

    qmiLocSetConstellationConfigReqMsgT_v02 setConstellationConfigMsg;
    qmiLocGenReqStatusIndMsgT_v02 genReqStatusIndMsg;

    // Clear all fields
    memset (&setConstellationConfigMsg, 0, sizeof(setConstellationConfigMsg));
    memset(&genReqStatusIndMsg, 0, sizeof(genReqStatusIndMsg));

    // Fill in the request details
    setConstellationConfigMsg.resetConstellations = false;

    setConstellationConfigMsg.enableMask_valid = true;
    setConstellationConfigMsg.enableMask = config.enabledSvTypesMask;

    bool disableSupported = ContextBase::isFeatureSupported(
            LOC_SUPPORTED_FEATURE_CONSTELLATION_DISABLEMENT);

    if (disableSupported) {
       setConstellationConfigMsg.disableMask_valid = true;
       setConstellationConfigMsg.disableMask = config.blacklistedSvTypesMask;
    }

    LOC_LOGI("setConstellationControl:disableSupported %d,"
             "enable: %d 0x%" PRIx64 ", blacklisted: %d 0x%" PRIx64 "",
             disableSupported,
             setConstellationConfigMsg.enableMask_valid,
             setConstellationConfigMsg.enableMask,
             setConstellationConfigMsg.disableMask_valid,
             setConstellationConfigMsg.disableMask);

    // Update in request union
    req_union.pSetConstellationConfigReq = &setConstellationConfigMsg;

    // Send the request
    status = loc_sync_send_req(clientHandle,
                               QMI_LOC_SET_CONSTELLATION_CONTROL_REQ_V02,
                               req_union,
                               LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                               QMI_LOC_SET_CONSTELLATION_CONTROL_IND_V02,
                               &genReqStatusIndMsg);

    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
    if (eLOC_CLIENT_SUCCESS == status) {
        err = LOCATION_ERROR_SUCCESS;
    }

    if (adapterResponse != NULL) {
        adapterResponse->returnToSender(err);
    }

    }));
}

void
LocApiV02::getConstellationControl()
{
    sendMsg(new LocApiMsg([this] () {

    locClientStatusEnumType status = eLOC_CLIENT_FAILURE_GENERAL;
    locClientReqUnionType req_union = {};

    qmiLocGetConstellationConfigIndMsgT_v02 getConstIndMsg;
    memset(&getConstIndMsg, 0, sizeof(getConstIndMsg));

    // Nothing to update in request union

    // Send the request
    status = loc_sync_send_req(clientHandle,
                               QMI_LOC_GET_CONSTELLATION_CONTROL_REQ_V02,
                               req_union,
                               LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                               QMI_LOC_GET_CONSTELLATION_CONTROL_IND_V02,
                               &getConstIndMsg);
    if(status == eLOC_CLIENT_SUCCESS &&
            getConstIndMsg.status == eQMI_LOC_SUCCESS_V02) {
        reportGnssSvTypeConfig(getConstIndMsg);
    }
    }));
}

void
LocApiV02::resetConstellationControl(LocApiResponse *adapterResponse)
{
    sendMsg(new LocApiMsg([this, adapterResponse] () {

    locClientStatusEnumType status = eLOC_CLIENT_FAILURE_GENERAL;
    locClientReqUnionType req_union = {};

    qmiLocSetConstellationConfigReqMsgT_v02 setConstellationConfigMsg;
    qmiLocGenReqStatusIndMsgT_v02 genReqStatusIndMsg;

    // Clear all fields
    memset (&setConstellationConfigMsg, 0, sizeof(setConstellationConfigMsg));
    memset(&genReqStatusIndMsg, 0, sizeof(genReqStatusIndMsg));

    // Fill in the request details
    setConstellationConfigMsg.resetConstellations = true;

    // Update in request union
    req_union.pSetConstellationConfigReq = &setConstellationConfigMsg;

    // Send the request
    status = loc_sync_send_req(clientHandle,
                               QMI_LOC_SET_CONSTELLATION_CONTROL_REQ_V02,
                               req_union,
                               LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                               QMI_LOC_SET_CONSTELLATION_CONTROL_IND_V02,
                               &genReqStatusIndMsg);
    if(status != eLOC_CLIENT_SUCCESS ||
            genReqStatusIndMsg.status != eQMI_LOC_SUCCESS_V02) {
        LOC_LOGe("Reset Constellation Config failed. "
                 "status: %s ind status %s",
                 loc_get_v02_client_status_name(status),
                 loc_get_v02_qmi_status_name(genReqStatusIndMsg.status));
    }

    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
    if (eLOC_CLIENT_SUCCESS == status) {
        err = LOCATION_ERROR_SUCCESS;
    }
    if (adapterResponse) {
        adapterResponse->returnToSender(err);
    }
    }));
}

void
LocApiV02::reportGnssSvIdConfig(
        const qmiLocGetBlacklistSvIndMsgT_v02& ind)
{
    // Validate status
    if (ind.status != eQMI_LOC_SUCCESS_V02) {
        LOC_LOGe("Ind failure status %d", ind.status);
        return;
    }

    // Parse all fields
    GnssSvIdConfig config = {};
    convertQmiBlacklistedSvConfigToGnssConfig(ind, config);
    // Pass on GnssSvConfig
    LocApiBase::reportGnssSvIdConfig(config);
}

void
LocApiV02::reportGnssSvTypeConfig(
        const qmiLocGetConstellationConfigIndMsgT_v02& ind)
{
    // Validate status
    if (ind.status != eQMI_LOC_SUCCESS_V02) {
        LOC_LOGe("Ind failure status %d", ind.status);
        return;
    }

    // Parse all fields
    GnssSvTypeConfig config = {};
    config.size = sizeof(GnssSvTypeConfig);
    convertToGnssSvTypeConfig(ind, config);

    // Pass on GnssSvConfig
    LocApiBase::reportGnssSvTypeConfig(config);
}

void
LocApiV02::convertToGnssSvTypeConfig(
        const qmiLocGetConstellationConfigIndMsgT_v02& ind,
        GnssSvTypeConfig& config)
{
    // Enabled Mask
    if (ind.gps_status_valid &&
            (ind.gps_status == eQMI_LOC_CONSTELLATION_ENABLED_MANDATORY_V02 ||
                    ind.gps_status == eQMI_LOC_CONSTELLATION_ENABLED_INTERNALLY_V02 ||
                    ind.gps_status == eQMI_LOC_CONSTELLATION_ENABLED_BY_CLIENT_V02)) {
        config.enabledSvTypesMask |= GNSS_SV_TYPES_MASK_GPS_BIT;
    }
    if (ind.bds_status_valid &&
            (ind.bds_status == eQMI_LOC_CONSTELLATION_ENABLED_MANDATORY_V02 ||
                    ind.bds_status == eQMI_LOC_CONSTELLATION_ENABLED_INTERNALLY_V02 ||
                    ind.bds_status == eQMI_LOC_CONSTELLATION_ENABLED_BY_CLIENT_V02)) {
        config.enabledSvTypesMask |= GNSS_SV_TYPES_MASK_BDS_BIT;
    }
    if (ind.glonass_status_valid &&
            (ind.glonass_status == eQMI_LOC_CONSTELLATION_ENABLED_MANDATORY_V02 ||
                    ind.glonass_status == eQMI_LOC_CONSTELLATION_ENABLED_INTERNALLY_V02 ||
                    ind.glonass_status == eQMI_LOC_CONSTELLATION_ENABLED_BY_CLIENT_V02)) {
        config.enabledSvTypesMask |= GNSS_SV_TYPES_MASK_GLO_BIT;
    }
    if (ind.galileo_status_valid &&
            (ind.galileo_status == eQMI_LOC_CONSTELLATION_ENABLED_MANDATORY_V02 ||
                    ind.galileo_status == eQMI_LOC_CONSTELLATION_ENABLED_INTERNALLY_V02 ||
                    ind.galileo_status == eQMI_LOC_CONSTELLATION_ENABLED_BY_CLIENT_V02)) {
        config.enabledSvTypesMask |= GNSS_SV_TYPES_MASK_GAL_BIT;
    }
    if (ind.qzss_status_valid &&
            (ind.qzss_status == eQMI_LOC_CONSTELLATION_ENABLED_MANDATORY_V02 ||
                    ind.qzss_status == eQMI_LOC_CONSTELLATION_ENABLED_INTERNALLY_V02 ||
                    ind.qzss_status == eQMI_LOC_CONSTELLATION_ENABLED_BY_CLIENT_V02)) {
        config.enabledSvTypesMask |= GNSS_SV_TYPES_MASK_QZSS_BIT;
    }
    if (ind.navic_status_valid &&
            (ind.navic_status == eQMI_LOC_CONSTELLATION_ENABLED_MANDATORY_V02 ||
                    ind.navic_status == eQMI_LOC_CONSTELLATION_ENABLED_INTERNALLY_V02 ||
                    ind.navic_status == eQMI_LOC_CONSTELLATION_ENABLED_BY_CLIENT_V02)) {
        config.enabledSvTypesMask |= GNSS_SV_TYPES_MASK_NAVIC_BIT;
    }

    // Disabled Mask
    if (ind.gps_status_valid &&
            (ind.gps_status == eQMI_LOC_CONSTELLATION_DISABLED_NOT_SUPPORTED_V02 ||
                    ind.gps_status == eQMI_LOC_CONSTELLATION_DISABLED_INTERNALLY_V02 ||
                    ind.gps_status == eQMI_LOC_CONSTELLATION_DISABLED_BY_CLIENT_V02 ||
                    ind.gps_status == eQMI_LOC_CONSTELLATION_DISABLED_NO_MEMORY_V02)) {
        config.blacklistedSvTypesMask |= GNSS_SV_TYPES_MASK_GPS_BIT;
    }
    if (ind.bds_status_valid &&
            (ind.bds_status == eQMI_LOC_CONSTELLATION_DISABLED_NOT_SUPPORTED_V02 ||
                    ind.bds_status == eQMI_LOC_CONSTELLATION_DISABLED_INTERNALLY_V02 ||
                    ind.bds_status == eQMI_LOC_CONSTELLATION_DISABLED_BY_CLIENT_V02 ||
                    ind.bds_status == eQMI_LOC_CONSTELLATION_DISABLED_NO_MEMORY_V02)) {
        config.blacklistedSvTypesMask |= GNSS_SV_TYPES_MASK_BDS_BIT;
    }
    if (ind.glonass_status_valid &&
            (ind.glonass_status == eQMI_LOC_CONSTELLATION_DISABLED_NOT_SUPPORTED_V02 ||
                    ind.glonass_status == eQMI_LOC_CONSTELLATION_DISABLED_INTERNALLY_V02 ||
                    ind.glonass_status == eQMI_LOC_CONSTELLATION_DISABLED_BY_CLIENT_V02 ||
                    ind.glonass_status == eQMI_LOC_CONSTELLATION_DISABLED_NO_MEMORY_V02)) {
        config.blacklistedSvTypesMask |= GNSS_SV_TYPES_MASK_GLO_BIT;
    }
    if (ind.galileo_status_valid &&
            (ind.galileo_status == eQMI_LOC_CONSTELLATION_DISABLED_NOT_SUPPORTED_V02 ||
                    ind.galileo_status == eQMI_LOC_CONSTELLATION_DISABLED_INTERNALLY_V02 ||
                    ind.galileo_status == eQMI_LOC_CONSTELLATION_DISABLED_BY_CLIENT_V02 ||
                    ind.galileo_status == eQMI_LOC_CONSTELLATION_DISABLED_NO_MEMORY_V02)) {
        config.blacklistedSvTypesMask |= GNSS_SV_TYPES_MASK_GAL_BIT;
    }
    if (ind.qzss_status_valid &&
            (ind.qzss_status == eQMI_LOC_CONSTELLATION_DISABLED_NOT_SUPPORTED_V02 ||
                    ind.qzss_status == eQMI_LOC_CONSTELLATION_DISABLED_INTERNALLY_V02 ||
                    ind.qzss_status == eQMI_LOC_CONSTELLATION_DISABLED_BY_CLIENT_V02 ||
                    ind.qzss_status == eQMI_LOC_CONSTELLATION_DISABLED_NO_MEMORY_V02)) {
        config.blacklistedSvTypesMask |= GNSS_SV_TYPES_MASK_QZSS_BIT;
    }
    if (ind.navic_status_valid &&
            (ind.navic_status == eQMI_LOC_CONSTELLATION_DISABLED_NOT_SUPPORTED_V02 ||
                    ind.navic_status == eQMI_LOC_CONSTELLATION_DISABLED_INTERNALLY_V02 ||
                    ind.navic_status == eQMI_LOC_CONSTELLATION_DISABLED_BY_CLIENT_V02 ||
                    ind.navic_status == eQMI_LOC_CONSTELLATION_DISABLED_NO_MEMORY_V02)) {
        config.blacklistedSvTypesMask |= GNSS_SV_TYPES_MASK_NAVIC_BIT;
    }
}

void LocApiV02::reportPowerStateChangeInfo(
        const qmiLocPlatformPowerStateChangedIndMsgT_v02 *pPowerStateChangedInfo) {

    struct MsgUpdatePowerState : public LocMsg {
        LocApiV02* mpLocApiV02;
        qmiLocPlatformPowerStateEnumT_v02 mNewPowerState;

        inline MsgUpdatePowerState(LocApiV02* pLocApiV02,
                                   qmiLocPlatformPowerStateEnumT_v02 newPowerState) :
                LocMsg(), mpLocApiV02(pLocApiV02),
                mNewPowerState(newPowerState) {}
        inline virtual void proc() const {
            mpLocApiV02->mPlatformPowerState = mNewPowerState;
            mpLocApiV02->registerEventMask();
        }
    };

    syslog(LOG_INFO, "reportPowerStateChangeInfo, old state: %d %d, new state: %d, %d",
             pPowerStateChangedInfo->powerStateOld_valid,
             pPowerStateChangedInfo->powerStateOld,
             pPowerStateChangedInfo->powerStateNew_valid,
             pPowerStateChangedInfo->powerStateNew);

    if (pPowerStateChangedInfo->powerStateNew_valid) {
        sendMsg(new MsgUpdatePowerState(this, pPowerStateChangedInfo->powerStateNew));
    }
}

void LocApiV02::onDbtPosReportEvent(const qmiLocEventDbtPositionReportIndMsgT_v02* pDbtPosReport)
{
    UlpLocation location;
    memset (&location, 0, sizeof (location));
    location.size = sizeof(location);
    const qmiLocDbtPositionStructT_v02 *pReport = &pDbtPosReport->dbtPosition;

    // time stamp
    location.gpsLocation.timestamp = pReport->timestampUtc;
    // latitude & longitude
    location.gpsLocation.flags |= LOC_GPS_LOCATION_HAS_LAT_LONG;
    location.gpsLocation.latitude  = pReport->latitude;
    location.gpsLocation.longitude = pReport->longitude;
    // Altitude
    if (pReport->altitudeWrtEllipsoid_valid == 1) {
        location.gpsLocation.flags |= LOC_GPS_LOCATION_HAS_ALTITUDE;
        location.gpsLocation.altitude = pReport->altitudeWrtEllipsoid;
    }
    // Speed
    if (pReport->speedHorizontal_valid == 1) {
        location.gpsLocation.flags |= LOC_GPS_LOCATION_HAS_SPEED;
        location.gpsLocation.speed = pReport->speedHorizontal;
    }
    // Heading
    if (pReport->heading_valid == 1) {
        location.gpsLocation.flags |= LOC_GPS_LOCATION_HAS_BEARING;
        location.gpsLocation.bearing = pReport->heading;
    }
    // Accuracy
    location.gpsLocation.flags |= LOC_GPS_LOCATION_HAS_ACCURACY;
    location.gpsLocation.accuracy = sqrt(pReport->horUncEllipseSemiMinor*
                                         pReport->horUncEllipseSemiMinor +
                                         pReport->horUncEllipseSemiMajor*
                                         pReport->horUncEllipseSemiMajor);
    // Source used
    LocPosTechMask loc_technology_mask = LOC_POS_TECH_MASK_DEFAULT;
    if (pDbtPosReport->positionSrc_valid) {
        switch (pDbtPosReport->positionSrc) {
            case eQMI_LOC_POSITION_SRC_GNSS_V02:
                loc_technology_mask = LOC_POS_TECH_MASK_SATELLITE;
                break;
            case eQMI_LOC_POSITION_SRC_CELLID_V02:
                loc_technology_mask = LOC_POS_TECH_MASK_CELLID;
                break;
            case eQMI_LOC_POSITION_SRC_ENH_CELLID_V02:
                loc_technology_mask = LOC_POS_TECH_MASK_CELLID;
                break;
            case eQMI_LOC_POSITION_SRC_WIFI_V02:
                loc_technology_mask = LOC_POS_TECH_MASK_WIFI;
                break;
            case eQMI_LOC_POSITION_SRC_TERRESTRIAL_V02:
                loc_technology_mask = LOC_POS_TECH_MASK_REFERENCE_LOCATION;
                break;
            case eQMI_LOC_POSITION_SRC_GNSS_TERRESTRIAL_HYBRID_V02:
                loc_technology_mask = LOC_POS_TECH_MASK_HYBRID;
                break;
            default:
                loc_technology_mask = LOC_POS_TECH_MASK_DEFAULT;
        }
    }

    GpsLocationExtended locationExtended;
    memset(&locationExtended, 0, sizeof (GpsLocationExtended));
    locationExtended.size = sizeof(locationExtended);

    if (pReport->vertUnc_valid) {
       locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_VERT_UNC;
       locationExtended.vert_unc = pReport->vertUnc;
    }
    if (pDbtPosReport->speedUnc_valid) {
        locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_SPEED_UNC;
        locationExtended.speed_unc = pDbtPosReport->speedUnc;
    }
    if (pDbtPosReport->headingUnc_valid) {
       locationExtended.flags |= GPS_LOCATION_EXTENDED_HAS_BEARING_UNC;
       locationExtended.bearing_unc = pDbtPosReport->headingUnc;
    }
    // Calling the base
    reportDBTPosition(location,
                      locationExtended,
                      LOC_SESS_INTERMEDIATE,
                      loc_technology_mask);
}

void LocApiV02::batchFullEvent(const qmiLocEventBatchFullIndMsgT_v02* batchFullInfo)
{
    struct MsgGetBatchedLocations : public LocMsg {
        LocApiBase& mApi;
        size_t mCount;
        uint32_t mAccumulatedDistance;
        qmiLocBatchingTypeEnumT_v02 mBatchType;
        inline MsgGetBatchedLocations(LocApiV02& api,
                                      size_t count,
                                      uint32_t accumulatedDistance,
                                      qmiLocBatchingTypeEnumT_v02 batchType) :
            LocMsg(),
            mApi(api),
            mCount(count),
            mAccumulatedDistance(accumulatedDistance),
            mBatchType(batchType) {}
        inline virtual void proc() const {
            // Explicit check for if OTB is supported or not to work around an
            // issue with older modems where uninitialized batchInd is being sent
            if ((mBatchType == eQMI_LOC_OUTDOOR_TRIP_BATCHING_V02) &&
                (ContextBase::isMessageSupported(LOC_API_ADAPTER_MESSAGE_OUTDOOR_TRIP_BATCHING))) {
                mApi.getBatchedTripLocationsSync(mCount, mAccumulatedDistance);
            } else {
                mApi.getBatchedLocationsSync(mCount);
            }
        }
    };

    sendMsg(new MsgGetBatchedLocations(*this,
                                        batchFullInfo->batchCount,
                                        (batchFullInfo->accumulatedDistance_valid ?
                                                batchFullInfo->accumulatedDistance: 0),
                                        (batchFullInfo->batchType_valid ? batchFullInfo->batchType :
                                                eQMI_LOC_LOCATION_BATCHING_V02)));
}

void LocApiV02::batchStatusEvent(const qmiLocEventBatchingStatusIndMsgT_v02* batchStatusInfo)
{
    BatchingStatus batchStatus;

    switch(batchStatusInfo->batchingStatus)
    {
        case eQMI_LOC_BATCH_POS_UNAVAILABLE_V02:
            batchStatus = BATCHING_STATUS_POSITION_UNAVAILABLE;
            break;
        case eQMI_LOC_BATCH_POS_AVAILABLE_V02:
            batchStatus = BATCHING_STATUS_POSITION_AVAILABE;
            break;
        default:
            batchStatus = BATCHING_STATUS_POSITION_UNAVAILABLE;
    }

    handleBatchStatusEvent(batchStatus);
}

// For Geofence
void LocApiV02::geofenceBreachEvent(const qmiLocEventGeofenceBreachIndMsgT_v02* breachInfo)
{
    uint32_t hwId = breachInfo->geofenceId;
    int64_t timestamp = time(NULL); // get the current time
    Location location;
    memset(&location, 0, sizeof(Location));
    location.size = sizeof(Location);

    if (breachInfo->geofencePosition_valid) {
        // Latitude & Longitude
        location.flags |= LOCATION_HAS_LAT_LONG_BIT;
        if (breachInfo->geofencePosition.latitude >= -90 &&
            breachInfo->geofencePosition.latitude <= 90 &&
            breachInfo->geofencePosition.longitude >= -180 &&
            breachInfo->geofencePosition.longitude <= 180) {
            // latitude and longitude look to be in the expected format
            location.latitude  = breachInfo->geofencePosition.latitude;
            location.longitude = breachInfo->geofencePosition.longitude;
        }
        else {
            // latitude and longitude must be in wrong format, so convert
            location.latitude  = breachInfo->geofencePosition.latitude * LAT_LONG_TO_RADIANS;
            location.longitude = breachInfo->geofencePosition.longitude * LAT_LONG_TO_RADIANS;
        }

        // Time stamp (UTC)
        location.timestamp = breachInfo->geofencePosition.timestampUtc;

        // Altitude
        location.flags |= LOCATION_HAS_ALTITUDE_BIT;
        location.altitude = breachInfo->geofencePosition.altitudeWrtEllipsoid;

        // Speed
        if (breachInfo->geofencePosition.speedHorizontal_valid == 1) {
            location.flags |= LOCATION_HAS_SPEED_BIT;
            location.speed = breachInfo->geofencePosition.speedHorizontal;
        }

        // Heading
        if (breachInfo->geofencePosition.heading_valid == 1) {
            location.flags |= LOCATION_HAS_BEARING_BIT;
            location.bearing = breachInfo->geofencePosition.heading;
        }

        // Uncertainty (circular)
        location.flags |= LOCATION_HAS_ACCURACY_BIT;
        location.accuracy = sqrt(
            (breachInfo->geofencePosition.horUncEllipseSemiMinor *
            breachInfo->geofencePosition.horUncEllipseSemiMinor) +
            (breachInfo->geofencePosition.horUncEllipseSemiMajor *
            breachInfo->geofencePosition.horUncEllipseSemiMajor));

        location.techMask = LOCATION_TECHNOLOGY_GNSS_BIT;

        LOC_LOGv("Location lat=%8.2f long=%8.2f ",
                 location.latitude, location.longitude);

    } else {
       LOC_LOGe("NO Location ");
    }

    GeofenceBreachType breachType;
    switch (breachInfo->breachType) {
    case eQMI_LOC_GEOFENCE_BREACH_TYPE_ENTERING_V02:
        breachType = GEOFENCE_BREACH_ENTER;
        break;
    case eQMI_LOC_GEOFENCE_BREACH_TYPE_LEAVING_V02:
        breachType = GEOFENCE_BREACH_EXIT;
        break;
    default:
        breachType = GEOFENCE_BREACH_UNKNOWN;
        break;
    }

    // calling the base
    geofenceBreach(1, &hwId, location, breachType, timestamp);
}

void
LocApiV02::geofenceBreachEvent(const qmiLocEventGeofenceBatchedBreachIndMsgT_v02* breachInfo)
{
    if (NULL == breachInfo)
        return;

    int64_t timestamp = time(NULL); // get the current time
    Location location;
    memset(&location, 0, sizeof(Location));
    location.size = sizeof(Location);

    if (breachInfo->geofencePosition_valid) {
        // Latitude & Longitude
        location.flags |= LOCATION_HAS_LAT_LONG_BIT;
        if (breachInfo->geofencePosition.latitude >= -90 &&
            breachInfo->geofencePosition.latitude <= 90 &&
            breachInfo->geofencePosition.longitude >= -180 &&
            breachInfo->geofencePosition.longitude <= 180) {
            // latitude and longitude look to be in the expected format
            location.latitude  = breachInfo->geofencePosition.latitude;
            location.longitude = breachInfo->geofencePosition.longitude;
        }
        else {
            // latitdue and longitude must be in wrong format, so convert
            location.latitude  = breachInfo->geofencePosition.latitude *
                                 LAT_LONG_TO_RADIANS;
            location.longitude = breachInfo->geofencePosition.longitude *
                                 LAT_LONG_TO_RADIANS;
        }

        // Time stamp (UTC)
        location.timestamp = breachInfo->geofencePosition.timestampUtc;

        // Altitude
        location.flags |= LOCATION_HAS_ALTITUDE_BIT;
        location.altitude = breachInfo->geofencePosition.altitudeWrtEllipsoid;

        // Speed
        if (breachInfo->geofencePosition.speedHorizontal_valid == 1) {
            location.flags |= LOCATION_HAS_SPEED_BIT;
            location.speed = breachInfo->geofencePosition.speedHorizontal;
        }

        // Heading
        if (breachInfo->geofencePosition.heading_valid == 1) {
            location.flags |= LOCATION_HAS_BEARING_BIT;
            location.bearing = breachInfo->geofencePosition.heading;
        }

        // Uncertainty (circular)
        location.flags |= LOCATION_HAS_ACCURACY_BIT;
        location.accuracy = sqrt(
            (breachInfo->geofencePosition.horUncEllipseSemiMinor *
            breachInfo->geofencePosition.horUncEllipseSemiMinor) +
            (breachInfo->geofencePosition.horUncEllipseSemiMajor *
            breachInfo->geofencePosition.horUncEllipseSemiMajor));

        location.techMask = LOCATION_TECHNOLOGY_GNSS_BIT;

        LOC_LOGv("latitude=%8.2f longitude=%8.2f ",
                 location.latitude, location.longitude);

    } else {
       LOC_LOGe("NO Location ");
    }

    GeofenceBreachType breachType;
    switch (breachInfo->breachType) {
    case eQMI_LOC_GEOFENCE_BREACH_TYPE_ENTERING_V02:
        breachType = GEOFENCE_BREACH_ENTER;
        break;
    case eQMI_LOC_GEOFENCE_BREACH_TYPE_LEAVING_V02:
        breachType = GEOFENCE_BREACH_EXIT;
        break;
    default:
        breachType = GEOFENCE_BREACH_UNKNOWN;
        break;
    }

    size_t count = 0;
    if (1 == breachInfo->geofenceIdDiscreteList_valid) {
        count += breachInfo->geofenceIdDiscreteList_len;
    }

    if (1 == breachInfo->geofenceIdContinuousList_valid) {
        for (uint32_t i = 0; i < breachInfo->geofenceIdContinuousList_len; i++) {
            count += breachInfo->geofenceIdContinuousList[i].idHigh -
                     breachInfo->geofenceIdContinuousList[i].idLow + 1;
        }
    }

    if (0 == count) {
        return;
    }

    uint32_t* hwIds = new uint32_t[count];
    if (hwIds == nullptr) {
        LOC_LOGE("new allocation failed, fatal error.");
        return;
    }
    uint32_t index = 0;
    if (1 == breachInfo->geofenceIdDiscreteList_valid) {
        for (uint32_t i = 0; i<breachInfo->geofenceIdDiscreteList_len; i++) {
            LOC_LOGv("discrete hwID %u breachType %u",
                     breachInfo->geofenceIdDiscreteList[i], breachType);
             if (index < count) {
                 hwIds[index++] = breachInfo->geofenceIdDiscreteList[i];
             }
        }
    }

    if (1 == breachInfo->geofenceIdContinuousList_valid) {
        for (uint32_t i = 0; i < breachInfo->geofenceIdContinuousList_len; i++) {
            for (uint32_t j = breachInfo->geofenceIdContinuousList[i].idLow;
                 j <= breachInfo->geofenceIdContinuousList[i].idHigh;
                 j++) {
                     LOC_LOGv("continuous hwID %u breachType %u", j, breachType);
                if (index < count) {
                    hwIds[index++] = j;
                }
            }
        }
    }

    // calling the base
    geofenceBreach(count, hwIds, location, breachType, timestamp);

    delete[] hwIds;
}

void LocApiV02::geofenceStatusEvent(const qmiLocEventGeofenceGenAlertIndMsgT_v02* alertInfo)
{
    const char* names[] = {
        "bad value",
        "GEOFENCE_GEN_ALERT_GNSS_UNAVAILABLE",
        "GEOFENCE_GEN_ALERT_GNSS_AVAILABLE",
        "GEOFENCE_GEN_ALERT_OOS",
        "GEOFENCE_GEN_ALERT_TIME_INVALID",
        "GEOFENCE_GEN_ALERT_REQUESTED_GNSS_FIX",
        "GEOFENCE_GEN_ALERT_REQUESTED_CPI_FIX"
    };
    int index = alertInfo->geofenceAlert;
    int lenOfArray = (sizeof(names) / sizeof(names[0]));
    if (index < 0 || index >= lenOfArray) {
        index = 0;
    }
    LOC_LOGd("GEOFENCE_GEN_ALERT - %s", names[index]);

    GeofenceStatusAvailable available = GEOFENCE_STATUS_AVAILABILE_NO;
    switch (alertInfo->geofenceAlert) {
    case eQMI_LOC_GEOFENCE_GEN_ALERT_GNSS_UNAVAILABLE_V02:
        available = GEOFENCE_STATUS_AVAILABILE_NO;
        break;
    case eQMI_LOC_GEOFENCE_GEN_ALERT_GNSS_AVAILABLE_V02:
        available = GEOFENCE_STATUS_AVAILABILE_YES;
        break;
    default:
        return;
        break;
    }

    // calling the base
    geofenceStatus(available);
}

void
LocApiV02::geofenceDwellEvent(const qmiLocEventGeofenceBatchedDwellIndMsgT_v02 *dwellInfo)
{
    if (NULL == dwellInfo)
        return;

    int64_t timestamp = time(NULL); // get the current time
    GeofenceBreachType breachType;
    if (eQMI_LOC_GEOFENCE_DWELL_TYPE_INSIDE_V02 == dwellInfo->dwellType) {
        breachType = GEOFENCE_BREACH_DWELL_IN;
    } else if (eQMI_LOC_GEOFENCE_DWELL_TYPE_OUTSIDE_V02 == dwellInfo->dwellType) {
        breachType = GEOFENCE_BREACH_DWELL_OUT;
    } else {
        LOC_LOGw("unknown dwell type %d", dwellInfo->dwellType);
        breachType = GEOFENCE_BREACH_UNKNOWN;
    }

    Location location;
    memset(&location, 0, sizeof(Location));
    location.size = sizeof(Location);

    if (dwellInfo->geofencePosition_valid) {
        // Latitude & Longitude
        location.flags |= LOCATION_HAS_LAT_LONG_BIT;
        if (dwellInfo->geofencePosition.latitude >= -90 &&
            dwellInfo->geofencePosition.latitude <= 90 &&
            dwellInfo->geofencePosition.longitude >= -180 &&
            dwellInfo->geofencePosition.longitude <= 180) {
            // latitude and longitude look to be in the expected format
            location.latitude  = dwellInfo->geofencePosition.latitude;
            location.longitude = dwellInfo->geofencePosition.longitude;
        } else {
            // latitude and longitude must be in wrong format, so convert
            location.latitude = dwellInfo->geofencePosition.latitude *
                                   LAT_LONG_TO_RADIANS;
            location.longitude = dwellInfo->geofencePosition.longitude *
                                    LAT_LONG_TO_RADIANS;
        }

        // Time stamp (UTC)
        location.timestamp = dwellInfo->geofencePosition.timestampUtc;

        // Altitude
        location.flags |= LOCATION_HAS_ALTITUDE_BIT;
        location.altitude = dwellInfo->geofencePosition.altitudeWrtEllipsoid;

        // Speed
        if (dwellInfo->geofencePosition.speedHorizontal_valid == 1) {
            location.flags |= LOCATION_HAS_SPEED_BIT;
            location.speed = dwellInfo->geofencePosition.speedHorizontal;
        }

        // Heading
        if (dwellInfo->geofencePosition.heading_valid == 1) {
            location.flags |= LOCATION_HAS_BEARING_BIT;
            location.bearing = dwellInfo->geofencePosition.heading;
        }

        // Uncertainty (circular)
        location.flags |= LOCATION_HAS_ACCURACY_BIT;
        location.accuracy = sqrt(
            (dwellInfo->geofencePosition.horUncEllipseSemiMinor *
            dwellInfo->geofencePosition.horUncEllipseSemiMinor) +
            (dwellInfo->geofencePosition.horUncEllipseSemiMajor *
            dwellInfo->geofencePosition.horUncEllipseSemiMajor));

            LOC_LOGv("latitude=%8.2f longitude=%8.2f ",
                     location.latitude, location.longitude);
    } else {
       LOC_LOGe("NO Location");
    }

    size_t count = 0;
    if (1 == dwellInfo->geofenceIdDiscreteList_valid) {
        count += dwellInfo->geofenceIdDiscreteList_len;
    }

    if (1 == dwellInfo->geofenceIdContinuousList_valid) {
        for (uint32_t i = 0; i < dwellInfo->geofenceIdContinuousList_len; i++) {
            count += dwellInfo->geofenceIdContinuousList[i].idHigh -
                     dwellInfo->geofenceIdContinuousList[i].idLow + 1;
        }
    }

    if (0 == count) {
        return;
    }

    uint32_t* hwIds = new uint32_t[count];
    if (hwIds == nullptr) {
        LOC_LOGE("new allocation failed, fatal error.");
        return;
    }
    uint32_t index = 0;
    if (1 == dwellInfo->geofenceIdDiscreteList_valid) {
        for (uint32_t i = 0; i<dwellInfo->geofenceIdDiscreteList_len; i++) {
            LOC_LOGv("discrete hwID %u breachType %u",
                     dwellInfo->geofenceIdDiscreteList[i], breachType);
             if (index < count) {
                 hwIds[index++] = dwellInfo->geofenceIdDiscreteList[i];
             }
        }
    }

    if (1 == dwellInfo->geofenceIdContinuousList_valid) {
        for (uint32_t i = 0; i < dwellInfo->geofenceIdContinuousList_len; i++) {
            for (uint32_t j = dwellInfo->geofenceIdContinuousList[i].idLow;
                 j <= dwellInfo->geofenceIdContinuousList[i].idHigh;
                 j++) {
                LOC_LOGv("continuous hwID %u breachType %u", j, breachType);
                if (index < count) {
                    hwIds[index++] = j;
                }
            }
        }
    }

    // calling the base
    geofenceBreach(count, hwIds, location, breachType, timestamp);

    delete[] hwIds;
}

void
LocApiV02::addGeofence(uint32_t clientId,
                        const GeofenceOption& options,
                        const GeofenceInfo& info,
                        LocApiResponseData<LocApiGeofenceData>* adapterResponseData)
{
    sendMsg(new LocApiMsg([this, clientId, options, info, adapterResponseData] () {

    LOC_LOGD("addGeofence: lat=%8.2f long=%8.2f radius %8.2f breach=%u respon=%u dwell=%u",
             info.latitude, info.longitude, info.radius,
             options.breachTypeMask, options.responsiveness, options.dwellTime);
    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;

    qmiLocAddCircularGeofenceReqMsgT_v02 addReq;
    memset(&addReq, 0, sizeof(addReq));

    if (options.breachTypeMask & GEOFENCE_BREACH_ENTER_BIT)
        addReq.breachMask |= QMI_LOC_GEOFENCE_BREACH_ENTERING_MASK_V02;
    if (options.breachTypeMask & GEOFENCE_BREACH_EXIT_BIT)
        addReq.breachMask |= QMI_LOC_GEOFENCE_BREACH_LEAVING_MASK_V02;

    // confidence
    addReq.confidence_valid = true;
    switch (options.confidence) {
    case GEOFENCE_CONFIDENCE_LOW:
        addReq.confidence = eQMI_LOC_GEOFENCE_CONFIDENCE_LOW_V02;
        break;
    case GEOFENCE_CONFIDENCE_MEDIUM:
        addReq.confidence = eQMI_LOC_GEOFENCE_CONFIDENCE_MED_V02;
        break;
    case GEOFENCE_CONFIDENCE_HIGH:
        addReq.confidence = eQMI_LOC_GEOFENCE_CONFIDENCE_HIGH_V02;
        break;
    default: // default to HIGH
        addReq.confidence = eQMI_LOC_GEOFENCE_CONFIDENCE_HIGH_V02;
    }

    // custom responsiveness
    addReq.customResponsivenessValue_valid = true;
    // The (min,max) custom responsiveness we support in seconds is (1, 65535)
    addReq.customResponsivenessValue =
            ((options.responsiveness < 1000U) ? 1 :
            std::min((options.responsiveness / 1000U), (uint32_t)UINT16_MAX));

    // dwell time
    if (options.dwellTime > 0) {
        addReq.dwellTime_valid = 1;
        addReq.dwellTime = options.dwellTime;
        addReq.dwellTypeMask_valid = 1;
        if (options.breachTypeMask & GEOFENCE_BREACH_DWELL_IN_BIT) {
            addReq.dwellTypeMask |= QMI_LOC_GEOFENCE_DWELL_TYPE_INSIDE_MASK_V02;
        }
        if (options.breachTypeMask & GEOFENCE_BREACH_DWELL_OUT_BIT) {
            addReq.dwellTypeMask |= QMI_LOC_GEOFENCE_DWELL_TYPE_OUTSIDE_MASK_V02;
        }
    }
    addReq.circularGeofenceArgs.latitude = info.latitude;
    addReq.circularGeofenceArgs.longitude = info.longitude;
    addReq.circularGeofenceArgs.radius = info.radius;
    addReq.includePosition = true;
    addReq.transactionId = clientId;

    LOC_SEND_SYNC_REQ(AddCircularGeofence, ADD_CIRCULAR_GEOFENCE, addReq);

    LocApiGeofenceData data;
    if (rv && ind.geofenceId_valid != 0) {
        data.hwId = ind.geofenceId;
        err = LOCATION_ERROR_SUCCESS;
    } else {
        if (eQMI_LOC_MAX_GEOFENCE_PROGRAMMED_V02 == ind.status) {
            err = LOCATION_ERROR_GEOFENCES_AT_MAX;
        }
        LOC_LOGE("addGeofence: failed! rv is %d, ind.geofenceId_valid is %d",
                 rv, ind.geofenceId_valid);
    }

    if (adapterResponseData != NULL) {
        adapterResponseData->returnToSender(err, data);
    }
    }));
}

void
LocApiV02::removeGeofence(uint32_t hwId, uint32_t clientId, LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, hwId, clientId, adapterResponse] () {

    LOC_LOGD("removeGeofence: hwId %u", hwId);
    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;

    qmiLocDeleteGeofenceReqMsgT_v02 deleteReq;
    memset(&deleteReq, 0, sizeof(deleteReq));

    deleteReq.geofenceId = hwId;
    deleteReq.transactionId = clientId;

    LOC_SEND_SYNC_REQ(DeleteGeofence, DELETE_GEOFENCE, deleteReq);

    if (rv) {
        err = LOCATION_ERROR_SUCCESS;
    } else {
        LOC_LOGE("removeGeofence: failed! rv is %d", rv);
    }

    if (adapterResponse != NULL) {
        adapterResponse->returnToSender(err);
    }
    }));
}

void
LocApiV02::pauseGeofence(uint32_t hwId, uint32_t clientId, LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, hwId, clientId, adapterResponse] () {

    LOC_LOGD("pauseGeofence: hwId %u", hwId);
    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;

    qmiLocEditGeofenceReqMsgT_v02 editReq;
    memset(&editReq, 0, sizeof(editReq));

    editReq.geofenceId = hwId;
    editReq.transactionId = clientId;
    editReq.geofenceState_valid = 1;
    editReq.geofenceState = eQMI_LOC_GEOFENCE_STATE_SUSPEND_V02;

    LOC_SEND_SYNC_REQ(EditGeofence, EDIT_GEOFENCE, editReq);

    if (rv) {
        err = LOCATION_ERROR_SUCCESS;
    } else {
        LOC_LOGE("pauseGeofence: failed! rv is %d", rv);
    }

    if (adapterResponse != NULL) {
        adapterResponse->returnToSender(err);
    }
    }));
}

void
LocApiV02::resumeGeofence(uint32_t hwId, uint32_t clientId, LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, hwId, clientId, adapterResponse] () {

    LOC_LOGD("resumeGeofence: hwId %u",  hwId);
    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;

    qmiLocEditGeofenceReqMsgT_v02 editReq;
    memset(&editReq, 0, sizeof(editReq));

    editReq.geofenceId = hwId;
    editReq.transactionId = clientId;
    editReq.geofenceState_valid = 1;
    editReq.geofenceState = eQMI_LOC_GEOFENCE_STATE_ACTIVE_V02;

    LOC_SEND_SYNC_REQ(EditGeofence, EDIT_GEOFENCE, editReq);

    if (rv) {
        err = LOCATION_ERROR_SUCCESS;
    } else {
        LOC_LOGE("resumeGeofence: failed! rv is %d", rv);
    }

    if (adapterResponse != NULL) {
        adapterResponse->returnToSender(err);
    }
    }));
}

void
LocApiV02::modifyGeofence(uint32_t hwId,
                           uint32_t clientId,
                           const GeofenceOption& options, LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, hwId, clientId, options, adapterResponse] () {

    LOC_LOGD("modifyGeofence: breach=%u respon=%u dwell=%u",
             options.breachTypeMask, options.responsiveness, options.dwellTime);
    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;

    qmiLocEditGeofenceReqMsgT_v02 editReq;
    memset(&editReq, 0, sizeof(editReq));

    editReq.geofenceId = hwId;
    editReq.transactionId = clientId;

    editReq.breachMask_valid = 1;
    if (options.breachTypeMask & GEOFENCE_BREACH_ENTER_BIT) {
        editReq.breachMask |= QMI_LOC_GEOFENCE_BREACH_ENTERING_MASK_V02;
    }
    if (options.breachTypeMask & GEOFENCE_BREACH_EXIT_BIT) {
        editReq.breachMask |= QMI_LOC_GEOFENCE_BREACH_LEAVING_MASK_V02;
    }

    editReq.responsiveness_valid = 1;
    if (options.responsiveness <= GF_RESPONSIVENESS_THRESHOLD_MSEC_HIGH) {
        editReq.responsiveness = eQMI_LOC_GEOFENCE_RESPONSIVENESS_HIGH_V02;
    } else if (options.responsiveness <= GF_RESPONSIVENESS_THRESHOLD_MSEC_MEDIUM) {
        editReq.responsiveness = eQMI_LOC_GEOFENCE_RESPONSIVENESS_MED_V02;
    } else {
        editReq.responsiveness = eQMI_LOC_GEOFENCE_RESPONSIVENESS_LOW_V02;
    }

    LOC_SEND_SYNC_REQ(EditGeofence, EDIT_GEOFENCE, editReq);

    if (rv) {
        err = LOCATION_ERROR_SUCCESS;
    } else {
        LOC_LOGE("modifyGeofence: failed! rv is %d", rv);
    }
    if (adapterResponse != NULL) {
        adapterResponse->returnToSender(err);
    }
    }));

}

void LocApiV02::setBatchSize(size_t size)
{
    LOC_LOGd("mDesiredBatchSize %zu", size);
    mDesiredBatchSize = size;
    // set to zero so the actual batch size will be queried from modem on first startBatching call
    mBatchSize = 0;
}

void LocApiV02::setTripBatchSize(size_t size)
{
    mDesiredTripBatchSize = size;
    // set to zero so the actual trip batch size will be queried from modem on
    // first startOutdoorTripBatching call
    mTripBatchSize = 0;
}

LocationError
LocApiV02::queryBatchBuffer(size_t desiredSize,
        size_t &allocatedSize, BatchingMode batchMode)
{
    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;
    qmiLocGetBatchSizeReqMsgT_v02 batchSizeReq;

    memset(&batchSizeReq, 0, sizeof(batchSizeReq));
    batchSizeReq.transactionId = 1;
    batchSizeReq.batchSize = desiredSize;

    batchSizeReq.batchType_valid = 1;
    batchSizeReq.batchType = (batchMode == BATCHING_MODE_ROUTINE ? eQMI_LOC_LOCATION_BATCHING_V02 :
            eQMI_LOC_OUTDOOR_TRIP_BATCHING_V02);
    LOC_SEND_SYNC_REQ(GetBatchSize, GET_BATCH_SIZE, batchSizeReq);

    if (rv) {
        allocatedSize = ind.batchSize;
        LOC_LOGv("get batching size succeeded. The modem batch size for"
                " batch mode %u is %zu. Desired batch size : %zu.",
                batchMode, allocatedSize, desiredSize);
        if (allocatedSize != 0) {
            err = LOCATION_ERROR_SUCCESS;
        }
    } else {
        if ((batchMode == BATCHING_MODE_TRIP) &&
            (ind.status == eQMI_LOC_INVALID_PARAMETER_V02) &&
            (ind.batchSize > 0)) {

            LOC_LOGw("get batching size failed. The modem max threshold batch size "
                    "for batch mode %u is %u. Desired batch size : %zu. "
                    "Retrying with max threshold size ...",
                    batchMode, ind.batchSize, desiredSize);

            desiredSize = ind.batchSize;
            batchSizeReq.batchSize = ind.batchSize;
            LOC_SEND_SYNC_REQ(GetBatchSize, GET_BATCH_SIZE, batchSizeReq);

            if (rv) {
                allocatedSize = ind.batchSize;
                LOC_LOGv("get batching size succeeded. The modem batch size for"
                        " batch mode %u is %zu. Desired batch size : %zu.",
                        batchMode, allocatedSize, desiredSize);
                if (allocatedSize != 0) {
                    err = LOCATION_ERROR_SUCCESS;
                }
                return err;
            }
        }

        allocatedSize = 0;
        LOC_LOGe("get batching size failed for batch mode %u and desired batch size %zu"
                 "Or modem does not support batching",
                 batchMode, desiredSize);
    }

    return err;
}

LocationError
LocApiV02::releaseBatchBuffer(BatchingMode batchMode) {

    LocationError err = LOCATION_ERROR_GENERAL_FAILURE;

    qmiLocReleaseBatchReqMsgT_v02 batchReleaseReq;
    memset(&batchReleaseReq, 0, sizeof(batchReleaseReq));
    batchReleaseReq.transactionId = 1;

    batchReleaseReq.batchType_valid = 1;
    switch (batchMode) {
        case BATCHING_MODE_ROUTINE:
            batchReleaseReq.batchType = eQMI_LOC_LOCATION_BATCHING_V02;
        break;

        case BATCHING_MODE_TRIP:
            batchReleaseReq.batchType = eQMI_LOC_OUTDOOR_TRIP_BATCHING_V02;
        break;

        default:
            err = LOCATION_ERROR_INVALID_PARAMETER;
            LOC_LOGe("release batch failed for batch mode %u", batchMode);
            return err;
    }

    LOC_SEND_SYNC_REQ(ReleaseBatch, RELEASE_BATCH, batchReleaseReq);

    if (rv) {
        LOC_LOGv("release batch succeeded for batch mode %u", batchMode);
        mTripBatchSize = 0;
        err = LOCATION_ERROR_SUCCESS;
    } else {
        LOC_LOGe("release batch failed for batch mode %u",
                batchMode);
    }

    return err;
}


void
LocApiV02::setOperationMode(GnssSuplMode mode)
{
    locClientStatusEnumType status;
    locClientReqUnionType req_union;

    qmiLocSetOperationModeReqMsgT_v02 set_mode_msg;
    qmiLocSetOperationModeIndMsgT_v02 set_mode_ind;

    memset (&set_mode_msg, 0, sizeof(set_mode_msg));
    memset (&set_mode_ind, 0, sizeof(set_mode_ind));

    // < SEC_GPS
    /*Set SEC Operation Mode*/
    bool isKorFeature = false;
    const char* sec_sales_code = getSalesCode();

    isKorFeature = (strncmp(sec_sales_code, "SKT", 3) == 0) || (strncmp(sec_sales_code, "KTT", 3) == 0)
            || (strncmp(sec_sales_code, "LGU", 3) == 0) || (strncmp(sec_sales_code, "KOO", 3) == 0);
    GnssSuplMode sec_mode = mode;
    
    //Force to change MS-Assisted operation mode to MS-Based for Google SUPL Server.
    //Google SUPL Server doesn't support MS-Assisted MO session.
    if ((sec_mode == GNSS_SUPL_MODE_MSA)&& (strncmp(sec_gps_conf.SUPL_HOST, "supl.google.com", sizeof(sec_gps_conf.SUPL_HOST))==0)) {
        sec_mode = GNSS_SUPL_MODE_MSB;
        LOC_LOGD("%s:%d]: Change Position Mode to MS-Based for Google SUPL Server", __func__, __LINE__);
    }

    if ((isKorFeature == false) && (strncmp(sec_sales_code, "CTC", 3) != 0) && (strncmp(sec_sales_code, "CHM", 3) != 0)
            && (strncmp(sec_sales_code, "DCM", 3) != 0) && (strncmp(sec_sales_code, "KDI", 3) != 0)) {
        if (sec_mode == GNSS_SUPL_MODE_MSA) {
            sec_mode = GNSS_SUPL_MODE_MSB;
            LOC_LOGD("%s:%d]: Change Position Mode to MS-Based", __func__, __LINE__);
        }
    }
    if ((strncmp(sec_sales_code, "VZW", 3) == 0) || (strncmp(sec_sales_code, "SPR", 3) == 0) || (strncmp(sec_sales_code, "USC", 3) == 0) || (strncmp(sec_sales_code, "LRA", 3) == 0) ||
         (strncmp(sec_sales_code, "BST", 3) == 0) || (strncmp(sec_sales_code, "VMU", 3) == 0) || (strncmp(sec_sales_code, "XAS", 3) == 0) || (strncmp(sec_sales_code, "ACG", 3) == 0) ||
         (strncmp(sec_sales_code, "TFNVZW", 6) == 0)) {
        if (strcmp(sec_gps_conf.OPERATION_MODE, "STANDALONE")==0) {
            sec_mode = GNSS_SUPL_MODE_STANDALONE;
        }
    }

    if (Sec_Configuration) { // if secgps.conf is exist.
        if (strcmp(sec_sales_code, "CTC") != 0) {
            if (strcmp(sec_gps_conf.OPERATION_MODE, "STANDALONE")==0) { // standalone
                sec_mode = GNSS_SUPL_MODE_STANDALONE;
            } else if(strcmp(sec_gps_conf.OPERATION_MODE, "MSBASED")==0) { // msb
                sec_mode = GNSS_SUPL_MODE_MSB;
            } else if(strcmp(sec_gps_conf.OPERATION_MODE, "MSASSISTED")==0) { // msa
                sec_mode = GNSS_SUPL_MODE_MSA;
            } else{ // default
                LOC_LOGE("operation mode from secgps.conf is not valid.");
                sec_mode = mode; // if value read from secgps.conf is not valid, keep application's request.
            }
        } else {
            sec_mode = mode; // if value read from secgps.conf is not valid, keep application's request.
        }
    }

    // SEC: Operation mode should be standalone for wifi models.
    if (mIsWifiOnly) {
        sec_mode = GNSS_SUPL_MODE_STANDALONE;
    }

    if (GNSS_SUPL_MODE_MSB == sec_mode) {
        set_mode_msg.operationMode = eQMI_LOC_OPER_MODE_MSB_V02;
    } else if (GNSS_SUPL_MODE_MSA == sec_mode) {
        set_mode_msg.operationMode = eQMI_LOC_OPER_MODE_MSA_V02;
    } else {
        set_mode_msg.operationMode = eQMI_LOC_OPER_MODE_STANDALONE_V02;
    }

    LOC_LOGD("SEC: operationMode - (%d)", set_mode_msg.operationMode); //SEC

    req_union.pSetOperationModeReq = &set_mode_msg;

    status = locSyncSendReq(QMI_LOC_SET_OPERATION_MODE_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                            QMI_LOC_SET_OPERATION_MODE_IND_V02,
                            &set_mode_ind);
}

void
LocApiV02::startTimeBasedTracking(const TrackingOptions& options, LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, options, adapterResponse] () {
    // < SEC_GPS
    if(mIsWifiOnly){
        system_type = checkFactoryMode();
        if((checkSmdFactoryMode() == false) && (system_type != FTM_NONE_SYSTEM))
        {
            LOC_LOGI("startFix : system type = %d", system_type);
            performFactoryMode(FTM_OPERATION_ACTIVATE, system_type);
            createTimerFactoryMode();

            LocationError err = LOCATION_ERROR_SUCCESS;

            if (adapterResponse != NULL) {
                adapterResponse->returnToSender(err);
            }
            return;
        }
    }
    // SEC_GPS >
    LOC_LOGI("startTimeBasedTracking: minInterval %u, mode %u",
             options.minInterval, options.mode);
    LocationError err = LOCATION_ERROR_SUCCESS;

    // < SEC_GPS
    if (loc_read_sec_gps_conf() == 0){ // if secgps.conf is exist.
        setSecGnssParams();
    }
    // SEC_GPS >

    if (!mInSession) {
        mMeasElapsedRealTimeCal.reset();
    }

    // BOOT KPI marker, print only once for a session
    if (false == mInSession) {
        loc_boot_kpi_marker("L - LocApiV02 startFix, tbf %d", options.minInterval);
    }
    mInSession = true;
    registerEventMask();
    setOperationMode(options.mode);

    locClientStatusEnumType status;
    locClientReqUnionType req_union;
    qmiLocStartReqMsgT_v02 start_msg;
    memset(&start_msg, 0, sizeof(start_msg));

    start_msg.sessionId = 1; // dummy session id

    // interval
    uint32_t minInterval = options.minInterval;
    mMinInterval = minInterval;

    /*set interval for intermediate fixes*/
    start_msg.minIntermediatePositionReportInterval_valid = 1;
    start_msg.minIntermediatePositionReportInterval = minInterval;

    /*set interval for final fixes*/
    start_msg.minInterval_valid = 1;
    start_msg.minInterval = minInterval;

    // accuracy
    start_msg.horizontalAccuracyLevel_valid = 1;
    start_msg.horizontalAccuracyLevel = eQMI_LOC_ACCURACY_HIGH_V02;

    // recurrence
    start_msg.fixRecurrence_valid = 1;
    start_msg.fixRecurrence = eQMI_LOC_RECURRENCE_PERIODIC_V02;

    // altitude assumed
    start_msg.configAltitudeAssumed_valid = 1;
    start_msg.configAltitudeAssumed =
        eQMI_LOC_ALTITUDE_ASSUMED_IN_GNSS_SV_INFO_DISABLED_V02;

    // power mode
    mPowerMode = options.powerMode;

    start_msg.powerMode_valid = 1;
    start_msg.powerMode.powerMode = convertPowerMode(options.powerMode);
    // Force low accuracy for background power modes
    if (GNSS_POWER_MODE_M3 == options.powerMode ||
            GNSS_POWER_MODE_M4 == options.powerMode ||
            GNSS_POWER_MODE_M5 == options.powerMode) {
        start_msg.horizontalAccuracyLevel =  eQMI_LOC_ACCURACY_LOW_V02;
    }

    start_msg.powerMode.timeBetweenMeasurement = start_msg.minInterval;

    //special req type
    if (SPECIAL_REQ_INVALID != options.specialReq) {
        if (SPECIAL_REQ_SHORT_CODE == options.specialReq) {
            start_msg.specialReqType_valid = 1;
            start_msg.specialReqType = eQMI_LOC_SPECIAL_REQUEST_SHORT_CODE_V02;
        }
    }

    req_union.pStartReq = &start_msg;
    status = locClientSendReq(QMI_LOC_START_REQ_V02, req_union);
    if (eLOC_CLIENT_SUCCESS != status) {
        if (ENGINE_LOCK_STATE_DISABLED == getEngineLockState()) {
            LOC_LOGD("startTimeBasedTracking: engine state disabled");
            err = LOCATION_ERROR_TZ_LOCKED;
        } else {
            LOC_LOGE("startTimeBasedTracking: failed! status %d", status);
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }
    }
    // < SEC_GPS
    setGpsBatteryFlag(true);
    // SEC_GPS >
    if (adapterResponse != NULL) {
        adapterResponse->returnToSender(err);
    }
    }));

#ifdef PTP_SUPPORTED
    if (false == mIsGptpInitialized) {
        if (gptpInit()) {
            mIsGptpInitialized = true;
            LOC_LOGd(" GPTP initialization success ");
        } else {
            LOC_LOGe(" GPTP initialization failed ");
        }
    }
#endif
}

void LocApiV02::configConstellationMultiBand(const GnssSvTypeConfig& secondaryBandConfig,
                                             LocApiResponse* adapterResponse) {

    sendMsg(new LocApiMsg([this, secondaryBandConfig, adapterResponse] () {

    LocationError err = LOCATION_ERROR_SUCCESS;
    locClientStatusEnumType status = eLOC_CLIENT_FAILURE_GENERAL;
    locClientReqUnionType req_union = {};

    qmiLocSetMultibandConfigReqMsgT_v02 reqMsg = {};
    qmiLocGenReqStatusIndMsgT_v02 genReqStatusIndMsg = {};

    // Fill in the request details
    reqMsg.secondaryGnssConfig_valid = true;
    if (secondaryBandConfig.enabledSvTypesMask & GNSS_SV_TYPES_MASK_GPS_BIT) {
        reqMsg.secondaryGnssConfig |= eQMI_SYSTEM_GPS_V02;
    }
    if (secondaryBandConfig.enabledSvTypesMask & GNSS_SV_TYPES_MASK_GLO_BIT) {
        reqMsg.secondaryGnssConfig |= eQMI_SYSTEM_GLO_V02;
    }
    if (secondaryBandConfig.enabledSvTypesMask & GNSS_SV_TYPES_MASK_BDS_BIT) {
        reqMsg.secondaryGnssConfig |= eQMI_SYSTEM_BDS_V02;
    }
    if (secondaryBandConfig.enabledSvTypesMask & GNSS_SV_TYPES_MASK_QZSS_BIT) {
        reqMsg.secondaryGnssConfig |= eQMI_SYSTEM_QZSS_V02;
    }
    if (secondaryBandConfig.enabledSvTypesMask & GNSS_SV_TYPES_MASK_GAL_BIT) {
        reqMsg.secondaryGnssConfig |= eQMI_SYSTEM_GAL_V02;
    }
    if (secondaryBandConfig.enabledSvTypesMask & GNSS_SV_TYPES_MASK_NAVIC_BIT) {
        reqMsg.secondaryGnssConfig |= eQMI_SYSTEM_NAVIC_V02;
    }

    LOC_LOGd("hal secondary band enabled constellation: 0x%" PRIx64 ", "
             "qmi secondary band disabed constellation: 0x%" PRIx64 "",
             secondaryBandConfig.enabledSvTypesMask, reqMsg.secondaryGnssConfig);

    // Update in request union
    req_union.pSetMultibandConfigReq = &reqMsg;

    // Send the request
    status = loc_sync_send_req(clientHandle,
                               QMI_LOC_SET_MULTIBAND_CONFIG_REQ_V02,
                               req_union,
                               LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                               QMI_LOC_SET_MULTIBAND_CONFIG_IND_V02,
                               &genReqStatusIndMsg);
    if (status != eLOC_CLIENT_SUCCESS ||
            genReqStatusIndMsg.status != eQMI_LOC_SUCCESS_V02) {
        LOC_LOGe("Config multiband failed. status: %s ind status %s",
                 loc_get_v02_client_status_name(status),
                 loc_get_v02_qmi_status_name(genReqStatusIndMsg.status));

        if (status == eLOC_CLIENT_FAILURE_UNSUPPORTED ||
                status == eLOC_CLIENT_FAILURE_INVALID_MESSAGE_ID) {
            err = LOCATION_ERROR_NOT_SUPPORTED;
        } else {
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }
    }

    if (adapterResponse) {
        adapterResponse->returnToSender(err);
    }
    }));
}

void LocApiV02::getConstellationMultiBandConfig(
        uint32_t sessionId, LocApiResponse* adapterResponse) {

    sendMsg(new LocApiMsg([this, sessionId, adapterResponse] () {

    LocationError err = LOCATION_ERROR_SUCCESS;
    locClientStatusEnumType status = eLOC_CLIENT_FAILURE_GENERAL;
    locClientReqUnionType req_union = {};

    qmiLocGetMultibandConfigIndMsgT_v02 getMultibandConfigInd = {};
    GnssConfig gnssConfig = {};

    // get multiband config
    status = locSyncSendReq(QMI_LOC_GET_MULTIBAND_CONFIG_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                            QMI_LOC_GET_MULTIBAND_CONFIG_IND_V02,
                            &getMultibandConfigInd);
    if ((status == eLOC_CLIENT_SUCCESS) && (getMultibandConfigInd.status == eQMI_LOC_SUCCESS_V02) &&
            getMultibandConfigInd.secondaryGnssConfig_valid) {
        convertQmiSecondaryConfigToGnssConfig(getMultibandConfigInd.secondaryGnssConfig,
                                              gnssConfig.secondaryBandConfig);
        gnssConfig.flags |= GNSS_CONFIG_FLAGS_CONSTELLATION_SECONDARY_BAND_BIT;
    } else {
        if (status == eLOC_CLIENT_FAILURE_UNSUPPORTED ||
                status == eLOC_CLIENT_FAILURE_INVALID_MESSAGE_ID) {
            err = LOCATION_ERROR_NOT_SUPPORTED;
        } else {
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }
    }

    if (gnssConfig.flags) {
        LocApiBase::reportGnssConfig(sessionId, gnssConfig);
    } else {
        if (err != LOCATION_ERROR_NOT_SUPPORTED) {
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }
        adapterResponse->returnToSender(err);
    }
    }));
}

void LocApiV02::convertOsnmaTreeNode(qmiLocOsnmaTreeNodeT_v02& out, mgpOsnmaTreeNodeT& in) {
    out.height = in.uj;
    out.position = in.ui;
    out.hash_len = in.wLengthInBits / 8; // hash_len is the # of hash elements in uint8_t
    LOC_LOGa("in.wLengthInBits : %d, out.hash_len: %d", in.wLengthInBits, out.hash_len);
    memcpy(out.hash, in.uHash, sizeof(in.uHash));
}

void LocApiV02::convertPublicKeyAndMerkleTreeStruct(
        qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02& qmiOut,
        mgpOsnmaPublicKeyAndMerkleTreeStruct& in) {
    qmiOut.publicKeyType_valid = true;
    qmiOut.publicKeyType = (qmiLocPublicKeyTypeEnumT_v02)in.zPublicKey.eNpkt;
    qmiOut.publicKeyId = in.zPublicKey.uNpkId;
    qmiOut.publicKeyId_valid = true;
    qmiOut.publicKey_valid = true;
    qmiOut.publicKey_len = in.zPublicKey.wKeyLen / 8; // publicKey_len is the # of keys in uint8_t
    LOC_LOGa("in.zPublicKey.wKeyLen : %d, qmiOut.publicKey_len: %d", in.zPublicKey.wKeyLen,
             qmiOut.publicKey_len);
    memcpy(qmiOut.publicKey, in.zPublicKey.uKey, sizeof(in.zPublicKey.uKey));
    for (int i = 0; i < QMI_LOC_MERKLE_TREE_NODE_ARRAY_LENGTH_V02; ++i) {
        convertOsnmaTreeNode(qmiOut.intermediateNodes[i], in.zPublicKey.zNodes[i]);
    }
    qmiOut.intermediateNodes_valid = true;
    qmiOut.hashFunctionType_valid = true;
    qmiOut.hashFunctionType = (qmiLocHashFunctionTypeEnumT_v02)in.zMerkleTree.eHfType;
    qmiOut.rootNode_valid = true;
    convertOsnmaTreeNode(qmiOut.rootNode, in.zMerkleTree.zRootNode);
}

void LocApiV02::configMerkleTree(mgpOsnmaPublicKeyAndMerkleTreeStruct* merkleTree,
            LocApiResponse* adapterResponse) {
    sendMsg(new LocApiMsg([this, merkleTree, adapterResponse] () {
    LocationError err = LOCATION_ERROR_SUCCESS;

    qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02 req;
    qmiLocGenReqStatusIndMsgT_v02 ind;
    locClientStatusEnumType status;
    locClientReqUnionType req_union;

    memset(&req, 0, sizeof(req));
    memset(&ind, 0, sizeof(ind));
    convertPublicKeyAndMerkleTreeStruct(req, *merkleTree);

    req_union.pOsnmaPublicKeyMerkleTreeReq = &req;
    status = locSyncSendReq(QMI_LOC_OSNMA_PUBLIC_KEY_MERKLE_TREE_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                            QMI_LOC_OSNMA_PUBLIC_KEY_MERKLE_TREE_IND_V02,
                            &ind);
    if (status != eLOC_CLIENT_SUCCESS || ind.status != eQMI_LOC_SUCCESS_V02) {
        if (status == eLOC_CLIENT_FAILURE_UNSUPPORTED ||
                status == eLOC_CLIENT_FAILURE_INVALID_MESSAGE_ID) {
            err = LOCATION_ERROR_NOT_SUPPORTED;
        } else {
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }
    }
    if (adapterResponse) {
        adapterResponse->returnToSender(err);
    }
    }));
}

void LocApiV02::configOsnmaEnablement(bool enable, LocApiResponse* adapterResponse) {
    sendMsg(new LocApiMsg([this, enable, adapterResponse] () {
    LocationError err = LOCATION_ERROR_SUCCESS;

    qmiLocSetOsnmaStateReqMsgT_v02 req;
    qmiLocGenReqStatusIndMsgT_v02 ind;
    locClientStatusEnumType status;
    locClientReqUnionType req_union;

    memset(&req, 0, sizeof(req));
    memset(&ind, 0, sizeof(ind));
    req.enable = enable;

    req_union.pOsnmaEnablementReq = &req;
    status = locSyncSendReq(QMI_LOC_SET_OSNMA_STATE_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                            QMI_LOC_SET_OSNMA_STATE_IND_V02,
                            &ind);
    if (status != eLOC_CLIENT_SUCCESS || ind.status != eQMI_LOC_SUCCESS_V02) {
        if (status == eLOC_CLIENT_FAILURE_UNSUPPORTED ||
                status == eLOC_CLIENT_FAILURE_INVALID_MESSAGE_ID) {
            err = LOCATION_ERROR_NOT_SUPPORTED;
        } else {
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }
    }
    if (adapterResponse) {
        adapterResponse->returnToSender(err);
    }
    }));
}

void LocApiV02::getNtnConfigSignalMask(LocApiResponse* adapterResponse) {
    sendMsg(new LocApiMsg([this, adapterResponse] () {
    LocationError err = LOCATION_ERROR_SUCCESS;
    locClientStatusEnumType status = eLOC_CLIENT_FAILURE_GENERAL;
    locClientReqUnionType req_union = {};
    qmiLocGetNtnStatusIndMsgT_v02 getNtnStatusInd = {};

    status = locSyncSendReq(QMI_LOC_GET_NTN_STATUS_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                            QMI_LOC_GET_NTN_STATUS_IND_V02,
                            &getNtnStatusInd);

    GnssSignalTypeMask returnedMask = 0;
    if ((status == eLOC_CLIENT_SUCCESS) &&
        (getNtnStatusInd.status == eQMI_LOC_SUCCESS_V02)) {
        // Modem return NTN status successfully
        err = LOCATION_ERROR_SUCCESS;
        returnedMask = getNtnStatusInd.signalType;
    } else {
        // Modem doesn't response this QMI for some reason
        err = LOCATION_ERROR_NOT_SUPPORTED;
    }

    LocApiBase::reportNtnStatusEvent(err, returnedMask, false);
    if (adapterResponse != NULL) {
        adapterResponse->returnToSender(err);
    }
    }));
}

void LocApiV02::setNtnConfigSignalMask(GnssSignalTypeMask gpsSignalTypeConfigMask,
        LocApiResponse* adapterResponse) {
    sendMsg(new LocApiMsg([this, gpsSignalTypeConfigMask, adapterResponse] () {
    LOC_LOGd("gpsSignalTypeConfigMask = %d", gpsSignalTypeConfigMask);
    LocationError err = LOCATION_ERROR_SUCCESS;
    locClientStatusEnumType status = eLOC_CLIENT_FAILURE_GENERAL;
    locClientReqUnionType req_union = {};
    qmiLocSetNtnStatusReqMsgT_v02 setNtnStatusReq = {};
    qmiLocSetNtnStatusIndMsgT_v02 setNtnStatusInd = {};

    setNtnStatusReq.signalType = gpsSignalTypeConfigMask;
    req_union.pSetNtnStatusReq = &setNtnStatusReq;

    status = locSyncSendReq(QMI_LOC_SET_NTN_STATUS_REQ_V02,
                            req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                            QMI_LOC_SET_NTN_STATUS_IND_V02,
                            &setNtnStatusInd);

    GnssSignalTypeMask returnedMask = 0;
    if ((status == eLOC_CLIENT_SUCCESS) &&
        (setNtnStatusInd.status == eQMI_LOC_SUCCESS_V02)) {
        // Modem set NTN status successfully
        returnedMask = setNtnStatusInd.signalType;
        err = LOCATION_ERROR_SUCCESS;
    } else if (setNtnStatusInd.status == eQMI_LOC_GENERAL_FAILURE_V02) {
        // Modem cannot set NTN status exactly folllow gpsSignalTypeConfigMask
        err = LOCATION_ERROR_GENERAL_FAILURE;
        returnedMask = setNtnStatusInd.signalType;
    } else {
        //Modem doesn't response this QMI for some reason
        err = LOCATION_ERROR_NOT_SUPPORTED;
    }
    LocApiBase::reportNtnStatusEvent(err, returnedMask, true);
    if (adapterResponse != NULL) {
        adapterResponse->returnToSender(err);
    }
    }));
}

void LocApiV02::convertQmiSecondaryConfigToGnssConfig(
        qmiLocGNSSConstellEnumT_v02 qmiSecondaryBandConfig,
        GnssSvTypeConfig& secondaryBandConfig) {

    memset(&secondaryBandConfig, 0, sizeof(secondaryBandConfig));
    secondaryBandConfig.size = sizeof(secondaryBandConfig);

    if (qmiSecondaryBandConfig & eQMI_SYSTEM_GPS_V02) {
        secondaryBandConfig.enabledSvTypesMask |= GNSS_SV_TYPES_MASK_GPS_BIT;
    }
    if (qmiSecondaryBandConfig & eQMI_SYSTEM_GLO_V02) {
        secondaryBandConfig.enabledSvTypesMask |= GNSS_SV_TYPES_MASK_GLO_BIT;
    }
    if (qmiSecondaryBandConfig &eQMI_SYSTEM_BDS_V02) {
        secondaryBandConfig.enabledSvTypesMask |= GNSS_SV_TYPES_MASK_BDS_BIT;
    }
    if (qmiSecondaryBandConfig & eQMI_SYSTEM_QZSS_V02) {
        secondaryBandConfig.enabledSvTypesMask |= GNSS_SV_TYPES_MASK_QZSS_BIT;
    }
    if (qmiSecondaryBandConfig & eQMI_SYSTEM_GAL_V02) {
        secondaryBandConfig.enabledSvTypesMask |= GNSS_SV_TYPES_MASK_GAL_BIT;
    }
    if (qmiSecondaryBandConfig & eQMI_SYSTEM_NAVIC_V02) {
        secondaryBandConfig.enabledSvTypesMask |= GNSS_SV_TYPES_MASK_NAVIC_BIT;
    }

    secondaryBandConfig.blacklistedSvTypesMask =
            (~secondaryBandConfig.enabledSvTypesMask & GNSS_SV_TYPES_MASK_ALL);
}

void LocApiV02::convertQmiBlacklistedSvConfigToGnssConfig(
        const qmiLocGetBlacklistSvIndMsgT_v02& qmiBlacklistConfig,
        GnssSvIdConfig& gnssBlacklistConfig) {

    gnssBlacklistConfig.size = sizeof(gnssBlacklistConfig);

    if (qmiBlacklistConfig.glo_persist_blacklist_sv_valid) {
        gnssBlacklistConfig.gloBlacklistSvMask = qmiBlacklistConfig.glo_persist_blacklist_sv;
    }

    if (qmiBlacklistConfig.bds_persist_blacklist_sv_valid) {
        gnssBlacklistConfig.bdsBlacklistSvMask = qmiBlacklistConfig.bds_persist_blacklist_sv;
    }

    if (qmiBlacklistConfig.qzss_persist_blacklist_sv_valid) {
        gnssBlacklistConfig.qzssBlacklistSvMask = qmiBlacklistConfig.qzss_persist_blacklist_sv;
    }

    if (qmiBlacklistConfig.gal_persist_blacklist_sv_valid) {
        gnssBlacklistConfig.galBlacklistSvMask = qmiBlacklistConfig.gal_persist_blacklist_sv;
    }

    if (qmiBlacklistConfig.sbas_persist_blacklist_sv_valid) {
        gnssBlacklistConfig.sbasBlacklistSvMask = qmiBlacklistConfig.sbas_persist_blacklist_sv;
    }

    if (qmiBlacklistConfig.navic_persist_blacklist_sv_valid) {
        gnssBlacklistConfig.navicBlacklistSvMask = qmiBlacklistConfig.navic_persist_blacklist_sv;
    }

    if (qmiBlacklistConfig.gps_persist_blacklist_sv_valid) {
        gnssBlacklistConfig.gpsBlacklistSvMask = qmiBlacklistConfig.gps_persist_blacklist_sv;
    }

    LOC_LOGd("%d %d %d %d %d %d %d, blacklist gps 0x%" PRIx64 ", "
             "bds 0x%" PRIx64", glo 0x%" PRIx64 ", "
             "qzss 0x%" PRIx64", gal 0x%" PRIx64 ", "
             "sbas 0x%" PRIx64 ", navic 0x%" PRIx64 "",
             qmiBlacklistConfig.gps_persist_blacklist_sv_valid,
             qmiBlacklistConfig.glo_persist_blacklist_sv_valid,
             qmiBlacklistConfig.bds_persist_blacklist_sv_valid,
             qmiBlacklistConfig.qzss_persist_blacklist_sv_valid,
             qmiBlacklistConfig.gal_persist_blacklist_sv_valid,
             qmiBlacklistConfig.sbas_persist_blacklist_sv_valid,
             qmiBlacklistConfig.navic_persist_blacklist_sv_valid,
             gnssBlacklistConfig.gpsBlacklistSvMask,
             gnssBlacklistConfig.bdsBlacklistSvMask, gnssBlacklistConfig.gloBlacklistSvMask,
             gnssBlacklistConfig.qzssBlacklistSvMask, gnssBlacklistConfig.galBlacklistSvMask,
             gnssBlacklistConfig.sbasBlacklistSvMask, gnssBlacklistConfig.navicBlacklistSvMask);
}

void LocApiV02::configPrecisePositioning(uint32_t featureId, bool enable,
        const std::string& appHash, LocApiResponse* adapterResponse) {
    sendMsg(new LocApiMsg([this, featureId, enable, appHash, adapterResponse] () {
        LocationError err = LOCATION_ERROR_SUCCESS;

        qmiLocSetSdkFeatureConfigReqMsgT_v02 req;
        qmiLocSetSdkFeatureConfigIndMsgT_v02 ind;
        locClientStatusEnumType status;
        locClientReqUnionType req_union;

        LOC_LOGD("configPrecisePositioning: featureId %d, enable %d, app hash: %s",
                featureId, enable, appHash.c_str());
        memset(&req, 0, sizeof(req));
        memset(&ind, 0, sizeof(ind));
        req.featureConfig_valid = true;
        req.featureConfig = enable ? eQMI_LOC_SDK_FEATURE_CONFIG_ENABLE_V02 :
                eQMI_LOC_SDK_FEATURE_CONFIG_DISABLE_V02;
        req.appHash_valid = true;
        req.appHash_len = 32;
        for (int i = 0, j = 0; i < appHash.length(); i+=2, j++) {
            string byteString = appHash.substr(i, 2);
            req.appHash[j] = (uint8_t) strtol(byteString.c_str(), nullptr, 16);
        }
        req.featureStatusReport_valid = false;

        switch (featureId) {
        case QESDK_FEATURE_ID_RTK:
            req.featureStatusReport_valid = true;
            req.featureStatusReport |= QMI_LOC_FEATURE_STATUS_CARRIER_PHASE_V02;
            req.featureStatusReport |= QMI_LOC_FEATURE_STATUS_SV_POLYNOMIALS_V02;
            req.featureStatusReport |= QMI_LOC_FEATURE_STATUS_DGNSS_V02;
            req.featureStatusReport |= QMI_LOC_FEATURE_STATUS_QPPE_V02;
            break;

        case QESDK_FEATURE_ID_EDGNSS:
            req.featureStatusReport_valid = true;
            req.featureStatusReport |= QMI_LOC_FEATURE_STATUS_DGNSS_V02;
            break;

        case QESDK_FEATURE_ID_RL:
            req.featureStatusReport_valid = true;
            req.featureStatusReport |= QMI_LOC_FEATURE_STATUS_ROBUST_LOCATION_V02;
            break;

        default:
            LOC_LOGe("Invalid feature id %d", featureId);
            break;
        }

        req_union.pLocSetSdkFeatureConfigReq = &req;
        status = locSyncSendReq(QMI_LOC_SET_SDK_FEATURE_CONFIG_REQ_V02,
                                req_union, LOC_ENGINE_SYNC_REQUEST_LONG_TIMEOUT,
                                QMI_LOC_SET_SDK_FEATURE_CONFIG_IND_V02,
                                &ind);
        LOC_LOGD("configPrecisePositioning: Ind. featureStatusReport_valid %d, "
                "featureStatusReport %" PRIx64"", ind.featureStatusReport_valid,
                ind.featureStatusReport);
        if (status != eLOC_CLIENT_SUCCESS || ind.status != eQMI_LOC_SUCCESS_V02) {
            if (status == eLOC_CLIENT_FAILURE_UNSUPPORTED ||
                    status == eLOC_CLIENT_FAILURE_INVALID_MESSAGE_ID) {
                err = LOCATION_ERROR_NOT_SUPPORTED;
            } else {
                err = LOCATION_ERROR_GENERAL_FAILURE;
            }
        } else if (ind.featureStatusReport_valid) {
            //Report Modem side QESDK feature status to Adapters
            if (featureId == QESDK_FEATURE_ID_EDGNSS || featureId == QESDK_FEATURE_ID_RTK) {
                if (ind.featureStatusReport & QMI_LOC_FEATURE_STATUS_DGNSS_V02) {
                    mQesdkFeatureMask |= MODEM_QESDK_FEATURE_DGNSS;
                } else {
                    mQesdkFeatureMask &= (~MODEM_QESDK_FEATURE_DGNSS);
                }
            }
            if (featureId == QESDK_FEATURE_ID_RL) {
                if (ind.featureStatusReport & QMI_LOC_FEATURE_STATUS_ROBUST_LOCATION_V02) {
                    mQesdkFeatureMask |= MODEM_QESDK_FEATURE_ROBUST_LOCATION;
                } else {
                    mQesdkFeatureMask &= (~MODEM_QESDK_FEATURE_ROBUST_LOCATION);
                }
            }
            LocApiBase::reportModemGnssQesdkFeatureStatus(mQesdkFeatureMask);
        }
        if (adapterResponse) {
            adapterResponse->returnToSender(err);
        }
    }));
}

qmiLocPowerModeEnumT_v02
LocApiV02::convertPowerMode(GnssPowerMode powerMode)
{
    switch(powerMode) {
    case GNSS_POWER_MODE_M1:
        return eQMI_LOC_POWER_MODE_IMPROVED_ACCURACY_V02;
    case GNSS_POWER_MODE_M2:
        return eQMI_LOC_POWER_MODE_NORMAL_V02;
    case GNSS_POWER_MODE_M3:
        return eQMI_LOC_POWER_MODE_BACKGROUND_DEFINED_POWER_V02;
    case GNSS_POWER_MODE_M4:
        return eQMI_LOC_POWER_MODE_BACKGROUND_DEFINED_TIME_V02;
    case GNSS_POWER_MODE_M5:
        return eQMI_LOC_POWER_MODE_BACKGROUND_KEEP_WARM_V02;
    default:
        LOC_LOGE("Invalid power mode %d", powerMode);
    }

    return QMILOCPOWERMODEENUMT_MIN_ENUM_VAL_V02;
}

void
LocApiV02::stopTimeBasedTracking(LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, adapterResponse] () {

    // < SEC_GPS
    if(mIsWifiOnly) {
        if(system_type != FTM_NONE_SYSTEM)
        {
            if (factoryTimerId != 0)
            {
                timer_delete(factoryTimerId);
                factoryTimerId = 0;
            }
            performFactoryMode(FTM_OPERATION_DEACTIVATE, system_type);
            system_type = FTM_NONE_SYSTEM;
        
            LocationError err = LOCATION_ERROR_SUCCESS;
            adapterResponse->returnToSender(err);
            return;
        }
    }
    // SEC_GPS >

    LOC_LOGD("stopTimeBasedTracking enter");
    loc_boot_kpi_marker("L - LocApiV02 stop Fix session");
    LocationError err = LOCATION_ERROR_SUCCESS;

    locClientStatusEnumType status;
    locClientReqUnionType req_union;
    qmiLocStopReqMsgT_v02 stop_msg;
    memset(&stop_msg, 0, sizeof(stop_msg));
    stop_msg.sessionId = LOC_API_V02_DEF_SESSION_ID; // dummy session id
    req_union.pStopReq = &stop_msg;

    status = locClientSendReq(QMI_LOC_STOP_REQ_V02, req_union);
    if (status != eLOC_CLIENT_SUCCESS) {
        LOC_LOGE ("stopTimeBasedTracking failed! status %d", status);
        err = LOCATION_ERROR_GENERAL_FAILURE;
    } else {
        mIsFirstFinalFixReported = false;
        mInSession = false;
        mPowerMode = GNSS_POWER_MODE_DEFAULT;
        registerEventMask();
    }

    // < SEC_GPS
    setGpsBatteryFlag(false);
    // SEC_GPS >

    if (adapterResponse != NULL) {
        adapterResponse->returnToSender(err);
    }
    }));
}

void
LocApiV02::startDistanceBasedTracking(uint32_t sessionId,
                                       const LocationOptions& options,
                                       LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, sessionId, options, adapterResponse] () {

    // < SEC_GPS
    if(mIsWifiOnly){
        system_type = checkFactoryMode();
        if((checkSmdFactoryMode() == false) && (system_type != FTM_NONE_SYSTEM))
        {
            LOC_LOGI("startFix : system type = %d", system_type);
            performFactoryMode(FTM_OPERATION_ACTIVATE, system_type);
            createTimerFactoryMode();
            
            LocationError err = LOCATION_ERROR_SUCCESS;
            
            if (adapterResponse != NULL) {
                adapterResponse->returnToSender(err);
            }
            return;
        }
    }
    // SEC_GPS >
	
    LOC_LOGD("startDistanceBasedTracking: session id %u, minInterval %u minDistance %u",
             sessionId, options.minInterval, options.minDistance);
    LocationError err = LOCATION_ERROR_SUCCESS;

    // < SEC_GPS
    if (loc_read_sec_gps_conf() == 0){ // if secgps.conf is exist.
        setSecGnssParams();
    }
    // SEC_GPS >

    /** start distance based tracking session*/
    locClientStatusEnumType status;
    locClientReqUnionType req_union;
    qmiLocStartDbtReqMsgT_v02 start_dbt_req;
    qmiLocStartDbtIndMsgT_v02 start_dbt_ind;
    memset(&start_dbt_req, 0, sizeof(start_dbt_req));
    memset(&start_dbt_ind, 0, sizeof(start_dbt_ind));

    // request id
    start_dbt_req.reqId = (uint8_t)sessionId;

    // distance
    start_dbt_req.minDistance = options.minDistance;

    // time
    uint32_t minInterval = options.minInterval;
    start_dbt_req.maxLatency_valid = 1;
    start_dbt_req.maxLatency = minInterval/1000; //in seconds
    if (0 == start_dbt_req.maxLatency) {
        start_dbt_req.maxLatency = 1; //in seconds
    }

    // type
    start_dbt_req.distanceType = eQMI_LOC_DBT_DISTANCE_TYPE_STRAIGHT_LINE_V02;

    /* original location disabled by default, as the original location is
       the one cached in the modem buffer and its timestamps is not fresh.*/
    start_dbt_req.needOriginLocation = 0;

    start_dbt_req.usageType_valid = 1;
    start_dbt_req.usageType = eQMI_LOC_DBT_USAGE_NAVIGATION_V02;
    req_union.pStartDbtReq = &start_dbt_req;

    status = locSyncSendReq(QMI_LOC_START_DBT_REQ_V02,
                            req_union,
                            LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                            QMI_LOC_START_DBT_IND_V02,
                            &start_dbt_ind);

    if (eLOC_CLIENT_SUCCESS != status ||
            eQMI_LOC_SUCCESS_V02 != start_dbt_ind.status) {
        if (ENGINE_LOCK_STATE_DISABLED == getEngineLockState()) {
            LOC_LOGD("startDistanceBasedTracking: engine state disabled");
            err = LOCATION_ERROR_TZ_LOCKED;
        } else {
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }
    }
    // < SEC_GPS
    setGpsBatteryFlag(true);
    // SEC_GPS >
    if (adapterResponse != NULL) {
        adapterResponse->returnToSender(err);
    }
    }));
}

void
LocApiV02::stopDistanceBasedTracking(uint32_t sessionId, LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, sessionId, adapterResponse] () {

    // < SEC_GPS
    if(mIsWifiOnly) {
        if(system_type != FTM_NONE_SYSTEM)
        {
            if (factoryTimerId != 0)
            {
                timer_delete(factoryTimerId);
                factoryTimerId = 0;
            }
            performFactoryMode(FTM_OPERATION_DEACTIVATE, system_type);
            system_type = FTM_NONE_SYSTEM;
        
            LocationError err = LOCATION_ERROR_SUCCESS;
            adapterResponse->returnToSender(err);
            return;
        }
    }
    // SEC_GPS >

    LOC_LOGD("stopDistanceBasedTracking: session id %u", sessionId);
    LocationError err = LOCATION_ERROR_SUCCESS;

    locClientStatusEnumType status;
    locClientReqUnionType req_union;
    qmiLocStopDbtReqMsgT_v02 stop_dbt_req;
    qmiLocStopDbtIndMsgT_v02 stop_dbt_Ind;
    memset(&stop_dbt_req, 0, sizeof(stop_dbt_req));
    memset(&stop_dbt_Ind, 0, sizeof(stop_dbt_Ind));

    stop_dbt_req.reqId = sessionId;

    req_union.pStopDbtReq = &stop_dbt_req;

    status = locSyncSendReq(QMI_LOC_STOP_DBT_REQ_V02,
                            req_union,
                            LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                            QMI_LOC_STOP_DBT_IND_V02,
                            &stop_dbt_Ind);

    if (eLOC_CLIENT_SUCCESS != status ||
            eQMI_LOC_SUCCESS_V02 != stop_dbt_Ind.status) {
        err = LOCATION_ERROR_GENERAL_FAILURE;
    }

    // < SEC_GPS
    setGpsBatteryFlag(false);
    // SEC_GPS >

    adapterResponse->returnToSender(err);
    }));
}

void
LocApiV02::startBatching(uint32_t sessionId,
                          const LocationOptions& options,
                          uint32_t accuracy,
                          uint32_t timeout,
                          LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, sessionId, options, accuracy, timeout, adapterResponse] () {

    LOC_LOGD("startBatching: session id %u minInterval %u minDistance %u "
             "accuracy %u timeout %u",
             sessionId, options.minInterval, options.minDistance, accuracy, timeout);
    LocationError err = LOCATION_ERROR_SUCCESS;

    setOperationMode(options.mode);

    //get batch size if needs
    if (mBatchSize == 0) {
        if (LOCATION_ERROR_SUCCESS != queryBatchBuffer(mDesiredBatchSize,
                mBatchSize, BATCHING_MODE_ROUTINE)) {
            if (adapterResponse != NULL) {
                adapterResponse->returnToSender(LOCATION_ERROR_GENERAL_FAILURE);
            }
            return;
        }
    }

    qmiLocStartBatchingReqMsgT_v02 startBatchReq;
    memset(&startBatchReq, 0, sizeof(startBatchReq));

    // interval
    startBatchReq.minInterval_valid = 1;
    if (options.minInterval >= FLP_BATCHING_MINIMUN_INTERVAL) {
        startBatchReq.minInterval = options.minInterval;
    } else {
        startBatchReq.minInterval = FLP_BATCHING_MINIMUN_INTERVAL; // 1 second
    }

    // accuracy
    startBatchReq.horizontalAccuracyLevel_valid = 1;
    switch(accuracy) {
    case 0:
        startBatchReq.horizontalAccuracyLevel = eQMI_LOC_ACCURACY_LOW_V02;
        break;
    case 1:
        startBatchReq.horizontalAccuracyLevel = eQMI_LOC_ACCURACY_MED_V02;
        break;
    case 2:
        startBatchReq.horizontalAccuracyLevel = eQMI_LOC_ACCURACY_HIGH_V02;
        break;
    default:
        startBatchReq.horizontalAccuracyLevel = eQMI_LOC_ACCURACY_LOW_V02;
    }

    // time out
    if (timeout > 0) {
        startBatchReq.fixSessionTimeout_valid = 1;
        startBatchReq.fixSessionTimeout = timeout;
    } else {
        // modem will use the default time out (20 seconds)
        startBatchReq.fixSessionTimeout_valid = 0;
    }

    // batch request id
    startBatchReq.requestId_valid = 1;
    startBatchReq.requestId = sessionId;

    // batch all fixes always
    startBatchReq.batchAllPos_valid = 1;
    startBatchReq.batchAllPos = true;

    LOC_SEND_SYNC_REQ(StartBatching, START_BATCHING, startBatchReq);

    if (!rv) {
        if (ENGINE_LOCK_STATE_DISABLED == getEngineLockState()) {
            err = LOCATION_ERROR_TZ_LOCKED;
        } else {
            LOC_LOGE("startBatching: failed");
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }
    }

    if (adapterResponse != NULL) {
        adapterResponse->returnToSender(err);
    }

    }));
}

void
LocApiV02::stopBatching(uint32_t sessionId, LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, sessionId, adapterResponse] () {

    LOC_LOGD("stopBatching: session id %u", sessionId);
    LocationError err = LOCATION_ERROR_SUCCESS;

    qmiLocStopBatchingReqMsgT_v02 stopBatchingReq;
    memset(&stopBatchingReq, 0, sizeof(stopBatchingReq));

    stopBatchingReq.requestId_valid = 1;
    stopBatchingReq.requestId = sessionId;

    stopBatchingReq.batchType_valid = 1;
    stopBatchingReq.batchType = eQMI_LOC_LOCATION_BATCHING_V02;

    LOC_SEND_SYNC_REQ(StopBatching, STOP_BATCHING, stopBatchingReq);

    if (!rv) {
        LOC_LOGe("failed, rv =%d", rv);
        err = LOCATION_ERROR_GENERAL_FAILURE;
    }
    if (adapterResponse != NULL) {
        adapterResponse->returnToSender(err);
    }
    }));
}

LocationError
LocApiV02::startOutdoorTripBatchingSync(uint32_t tripDistance, uint32_t tripTbf, uint32_t timeout)
{
    LOC_LOGd("minInterval %u minDistance %u timeout %u",
             tripTbf, tripDistance, timeout);
    LocationError err = LOCATION_ERROR_SUCCESS;

    //get batch size if needs
    if (mTripBatchSize == 0) {
        if (LOCATION_ERROR_SUCCESS != queryBatchBuffer(mDesiredTripBatchSize,
            mTripBatchSize, BATCHING_MODE_TRIP)) {
            return LOCATION_ERROR_GENERAL_FAILURE;
        }
    }

    qmiLocStartOutdoorTripBatchingReqMsgT_v02 startOutdoorTripBatchReq;
    memset(&startOutdoorTripBatchReq, 0, sizeof(startOutdoorTripBatchReq));

    if (tripDistance > 0) {
        startOutdoorTripBatchReq.batchDistance = tripDistance;
    } else {
        startOutdoorTripBatchReq.batchDistance = FLP_BATCHING_MIN_TRIP_DISTANCE; // 1 meter
    }

    if (tripTbf >= FLP_BATCHING_MINIMUN_INTERVAL) {
        startOutdoorTripBatchReq.minTimeInterval = tripTbf;
    } else {
        startOutdoorTripBatchReq.minTimeInterval = FLP_BATCHING_MINIMUN_INTERVAL; // 1 second
    }

    // batch all fixes always
    startOutdoorTripBatchReq.batchAllPos_valid = 1;
    startOutdoorTripBatchReq.batchAllPos = true;

    // time out
    if (timeout > 0) {
        startOutdoorTripBatchReq.fixSessionTimeout_valid = 1;
        startOutdoorTripBatchReq.fixSessionTimeout = timeout;
    } else {
        // modem will use the default time out (20 seconds)
        startOutdoorTripBatchReq.fixSessionTimeout_valid = 0;
    }

    LOC_SEND_SYNC_REQ(StartOutdoorTripBatching, START_OUTDOOR_TRIP_BATCHING,
            startOutdoorTripBatchReq);

    if (!rv) {
        if (ENGINE_LOCK_STATE_DISABLED == getEngineLockState()) {
            LOC_LOGd("engine state disabled");
            err = LOCATION_ERROR_TZ_LOCKED;
        } else {
            LOC_LOGe("failed!");
            err = LOCATION_ERROR_GENERAL_FAILURE;
        }
    }
    return err;
}

void
LocApiV02::startOutdoorTripBatching(uint32_t tripDistance, uint32_t tripTbf, uint32_t timeout,
                                          LocApiResponse* adapterResponse) {
    sendMsg(new LocApiMsg([this, tripDistance, tripTbf, timeout, adapterResponse] () {
        if (adapterResponse != NULL) {
            adapterResponse->returnToSender(startOutdoorTripBatchingSync(tripDistance,
                                                                         tripTbf,
                                                                         timeout));
        }
    }));
}

void
LocApiV02::reStartOutdoorTripBatching(uint32_t ongoingTripDistance,
                                            uint32_t ongoingTripInterval,
                                            uint32_t batchingTimeout,
                                            LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, ongoingTripDistance, ongoingTripInterval, batchingTimeout,
                           adapterResponse] () {
        LocationError err = LOCATION_ERROR_SUCCESS;

        uint32_t accumulatedDistance = 0;
        uint32_t numOfBatchedPositions = 0;
        queryAccumulatedTripDistanceSync(accumulatedDistance, numOfBatchedPositions);

        if (numOfBatchedPositions) {
            getBatchedTripLocationsSync(numOfBatchedPositions, 0);
        }

        stopOutdoorTripBatchingSync(false);

        err = startOutdoorTripBatchingSync(ongoingTripDistance,
                                           ongoingTripInterval,
                                           batchingTimeout);
        if (adapterResponse != NULL) {
            adapterResponse->returnToSender(err);
        }
    }));
}

LocationError
LocApiV02::stopOutdoorTripBatchingSync(bool deallocBatchBuffer)
{
    LOC_LOGd("dellocBatchBuffer : %d", deallocBatchBuffer);
    LocationError err = LOCATION_ERROR_SUCCESS;

    qmiLocStopBatchingReqMsgT_v02 stopBatchingReq;
    memset(&stopBatchingReq, 0, sizeof(stopBatchingReq));

    stopBatchingReq.batchType_valid = 1;
    stopBatchingReq.batchType = eQMI_LOC_OUTDOOR_TRIP_BATCHING_V02;
    LOC_SEND_SYNC_REQ(StopBatching, STOP_BATCHING, stopBatchingReq);

    if (!rv) {
        err = LOCATION_ERROR_GENERAL_FAILURE;
        return err;
    }

    if (deallocBatchBuffer) {
        err = releaseBatchBuffer(BATCHING_MODE_TRIP);
    }

    return err;
}

void
LocApiV02::stopOutdoorTripBatching(bool deallocBatchBuffer, LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, deallocBatchBuffer, adapterResponse] () {
        if (adapterResponse != NULL) {
            adapterResponse->returnToSender(stopOutdoorTripBatchingSync(deallocBatchBuffer));
        }
    }));
}

LocationError
LocApiV02::getBatchedLocationsSync(size_t count)
{
    LOC_LOGd("count %zu.", count);
    LocationError err = LOCATION_ERROR_SUCCESS;

    size_t entriesToReadInTotal = std::min(mBatchSize,count);
    if (entriesToReadInTotal == 0) {
        LOC_LOGd("No batching memory allocated in modem or nothing to read");
        // calling the base class
        reportLocations(NULL, 0, BATCHING_MODE_ROUTINE);
    } else {
        size_t entriesToRead =
            std::min(entriesToReadInTotal, (size_t)QMI_LOC_READ_FROM_BATCH_MAX_SIZE_V02);
        size_t entriesGotInTotal = 0;
        size_t entriesGotInEachTime = 0;
        //allocate +1 just in case we get one more location added during read
        Location* pLocationsFromModem = new Location[entriesToReadInTotal+1]();
        if (pLocationsFromModem == nullptr) {
            LOC_LOGE("new allocation failed, fatal error.");
            return LOCATION_ERROR_GENERAL_FAILURE;
        }
        memset(pLocationsFromModem, 0, sizeof(Location)*(entriesToReadInTotal+1));
        Location* tempLocationP = new Location[QMI_LOC_READ_FROM_BATCH_MAX_SIZE_V02]();
        if (tempLocationP == nullptr) {
            LOC_LOGE("new allocation failed, fatal error.");
            return LOCATION_ERROR_GENERAL_FAILURE;
        }
        do {
            memset(tempLocationP, 0, QMI_LOC_READ_FROM_BATCH_MAX_SIZE_V02*sizeof(Location));
            readModemLocations(tempLocationP,
                               entriesToRead,
                               BATCHING_MODE_ROUTINE,
                               entriesGotInEachTime);
            for (size_t i = 0; i<entriesGotInEachTime; i++) {
                size_t index = entriesToReadInTotal-entriesGotInTotal-i;

                // make sure index is not too large or small to fit in the array
                if (index <= entriesToReadInTotal && (int)index >= 0) {
                    pLocationsFromModem[index] = tempLocationP[i];
                } else {
                    LOC_LOGw("dropped an unexpected location.");
                }
            }
            entriesGotInTotal += entriesGotInEachTime;
            entriesToRead = std::min(entriesToReadInTotal - entriesGotInTotal,
                                     (size_t)QMI_LOC_READ_FROM_BATCH_MAX_SIZE_V02);
        } while (entriesGotInEachTime > 0 && entriesToRead > 0);
        delete[] tempLocationP;

        LOC_LOGd("Read out %zu batched locations from modem in total.",
                 entriesGotInTotal);

        // offset is by default 1 because we allocated one extra location during read
        uint32_t offset = 1;
        if (entriesToReadInTotal > entriesGotInTotal) {
            offset = 1 + entriesToReadInTotal - entriesGotInTotal;
            LOC_LOGd("offset %u", offset);
        } else if (entriesGotInTotal > entriesToReadInTotal) {
            // offset is zero because we need one extra slot for the extra location
            offset = 0;
            LOC_LOGd("Read %zu extra location(s) than expected.",
                     entriesGotInTotal - entriesToReadInTotal);
            // we got one extra location added during modem read, so one location will
            // be out of order and needs to be found and put in order
            int64_t currentTimeStamp = pLocationsFromModem[entriesToReadInTotal].timestamp;
            for (int i=entriesToReadInTotal-1; i >= 0; i--) {
                // find the out of order location
                if (currentTimeStamp < pLocationsFromModem[i].timestamp) {
                    LOC_LOGd("Out of order location is index %d timestamp %" PRIu64,
                             i, pLocationsFromModem[i].timestamp);
                    // save the out of order location to be put at end of array
                    Location tempLocation(pLocationsFromModem[i]);
                    // shift the remaining locations down one
                    for (size_t j=i; j<entriesToReadInTotal; j++) {
                        pLocationsFromModem[j] = pLocationsFromModem[j+1];
                    }
                    // put the out of order location at end of array now
                    pLocationsFromModem[entriesToReadInTotal] = tempLocation;
                    break;
                } else {
                    currentTimeStamp = pLocationsFromModem[i].timestamp;
                }
            }
        }

        LOC_LOGd("Calling reportLocations with count:%zu and entriesGotInTotal:%zu",
                 count, entriesGotInTotal);

        // calling the base class
        reportLocations(pLocationsFromModem+offset, entriesGotInTotal, BATCHING_MODE_ROUTINE);
        delete[] pLocationsFromModem;
    }
    return err;

}

void
LocApiV02::getBatchedLocations(size_t count, LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, count, adapterResponse] () {
          if (adapterResponse != NULL) {
              adapterResponse->returnToSender(getBatchedLocationsSync(count));
          }
    }));
}

LocationError LocApiV02::getBatchedTripLocationsSync(size_t count, uint32_t accumulatedDistance)
{
    LocationError err = LOCATION_ERROR_SUCCESS;
    size_t idxLocationFromModem = 0;

    size_t entriesToReadInTotal = std::min(mTripBatchSize, count);
    if (entriesToReadInTotal == 0) {
        LOC_LOGd("No trip batching memory allocated in modem or nothing to read");
        // calling the base class
        reportLocations(NULL, 0, BATCHING_MODE_TRIP);
    } else {
        size_t entriesToRead =
                std::min(entriesToReadInTotal, (size_t)QMI_LOC_READ_FROM_BATCH_MAX_SIZE_V02);
        size_t entriesGotInTotal = 0;
        size_t entriesGotInEachTime = 0;

        Location* pLocationsFromModem = new Location[entriesToReadInTotal]();
        if (pLocationsFromModem == nullptr) {
            LOC_LOGE("new allocation failed, fatal error.");
            return LOCATION_ERROR_GENERAL_FAILURE;
        }
        memset(pLocationsFromModem, 0, sizeof(Location)*(entriesToReadInTotal));
        Location* tempLocationP = new Location[QMI_LOC_READ_FROM_BATCH_MAX_SIZE_V02]();
        if (tempLocationP == nullptr) {
            LOC_LOGE("new allocation failed, fatal error.");
            return LOCATION_ERROR_GENERAL_FAILURE;
        }

        do {
            memset(tempLocationP, 0, QMI_LOC_READ_FROM_BATCH_MAX_SIZE_V02*sizeof(Location));
            readModemLocations(tempLocationP,
                               entriesToRead,
                               BATCHING_MODE_TRIP,
                               entriesGotInEachTime);
            for (size_t iEntryIndex = 0; iEntryIndex<entriesGotInEachTime;
                    iEntryIndex++, idxLocationFromModem++) {
                // make sure index is not too large fit in the array
                if (idxLocationFromModem < entriesToReadInTotal) {
                    pLocationsFromModem[idxLocationFromModem] = tempLocationP[iEntryIndex];
                } else {
                    LOC_LOGw("dropped an unexpected location.");
                }
            }
            entriesGotInTotal += entriesGotInEachTime;
            entriesToRead = std::min(entriesToReadInTotal - entriesGotInTotal,
                                     (size_t)QMI_LOC_READ_FROM_BATCH_MAX_SIZE_V02);
        } while (entriesGotInEachTime > 0 && entriesToRead > 0);
        delete[] tempLocationP;

        LOC_LOGd("Calling reportLocations with count:%zu and entriesGotInTotal:%zu",
                 count, entriesGotInTotal);

        // calling the base class
        reportLocations(pLocationsFromModem, entriesGotInTotal, BATCHING_MODE_TRIP);

        if (accumulatedDistance != 0) {
            LOC_LOGd("Calling reportCompletedTrips with distance %u:", accumulatedDistance);
            reportCompletedTrips(accumulatedDistance);
        }

        delete[] pLocationsFromModem;
    }

    return err;
}

void LocApiV02::getBatchedTripLocations(size_t count, uint32_t accumulatedDistance,
                                              LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([this, count, accumulatedDistance, adapterResponse] () {
        if (adapterResponse != NULL) {
            adapterResponse->returnToSender(getBatchedTripLocationsSync(count, accumulatedDistance));
        }
    }));
}

void
LocApiV02::readModemLocations(Location* pLocationPiece,
                              size_t count,
                              BatchingMode batchingMode,
                              size_t& numbOfEntries)
{
    LOC_LOGd("count %zu.", count);
    numbOfEntries = 0;
    qmiLocReadFromBatchReqMsgT_v02 getBatchLocatonReq;
    memset(&getBatchLocatonReq, 0, sizeof(getBatchLocatonReq));

    if (count != 0) {
        getBatchLocatonReq.numberOfEntries = count;
        getBatchLocatonReq.transactionId = 1;
    }

    getBatchLocatonReq.batchType_valid = 1;
    getBatchLocatonReq.batchType = ((batchingMode == BATCHING_MODE_ROUTINE) ?
            eQMI_LOC_LOCATION_BATCHING_V02: eQMI_LOC_OUTDOOR_TRIP_BATCHING_V02);

    LOC_SEND_SYNC_REQ(ReadFromBatch, READ_FROM_BATCH, getBatchLocatonReq);

    if (!rv) {
        return;
    }
    if (ind.numberOfEntries_valid != 0 &&
        ind.batchedReportList_valid != 0) {
        for (uint32_t i=0; i<ind.numberOfEntries; i++) {
            Location temp;
            memset (&temp, 0, sizeof(temp));
            temp.size = sizeof(Location);
            if ((ind.batchedReportList[i].validFields &
                 QMI_LOC_BATCHED_REPORT_MASK_VALID_LATITUDE_V02) &&
                (ind.batchedReportList[i].validFields &
                 QMI_LOC_BATCHED_REPORT_MASK_VALID_LONGITUDE_V02)) {
                temp.latitude = ind.batchedReportList[i].latitude;
                temp.longitude = ind.batchedReportList[i].longitude;
                temp.flags |= LOCATION_HAS_LAT_LONG_BIT;
            }
            if (ind.batchedReportList[i].validFields &
                QMI_LOC_BATCHED_REPORT_MASK_VALID_ALT_WRT_ELP_V02) {
                temp.altitude = ind.batchedReportList[i].altitudeWrtEllipsoid;
                temp.flags |= LOCATION_HAS_ALTITUDE_BIT;
            }
            if (ind.batchedReportList[i].validFields &
                QMI_LOC_BATCHED_REPORT_MASK_VALID_TIMESTAMP_UTC_V02) {
                temp.timestamp = ind.batchedReportList[i].timestampUtc;
            }
            if (ind.batchedReportList[i].validFields &
                QMI_LOC_BATCHED_REPORT_MASK_VALID_HOR_CIR_UNC_V02) {
                temp.accuracy = ind.batchedReportList[i].horUncCircular;
                temp.flags |= LOCATION_HAS_ACCURACY_BIT;
            }
            if (ind.batchedReportList[i].validFields &
                QMI_LOC_BATCHED_REPORT_MASK_VALID_HEADING_V02) {
                temp.bearing = ind.batchedReportList[i].heading;
                temp.flags |= LOCATION_HAS_BEARING_BIT;
            }
            if (ind.batchedReportList[i].validFields &
                QMI_LOC_BATCHED_REPORT_MASK_VALID_SPEED_HOR_V02) {
                temp.speed = ind.batchedReportList[i].speedHorizontal;
                temp.flags |= LOCATION_HAS_SPEED_BIT;
            }
            if (ind.batchedReportList[i].validFields &
                QMI_LOC_BATCHED_REPORT_MASK_VALID_TECH_MASK_V02) {
                if (QMI_LOC_POS_TECH_MASK_SATELLITE_V02 & ind.batchedReportList[i].technologyMask) {
                    temp.techMask |= LOCATION_TECHNOLOGY_GNSS_BIT;
                }
                if (QMI_LOC_POS_TECH_MASK_CELLID_V02 & ind.batchedReportList[i].technologyMask) {
                    temp.techMask |= LOCATION_TECHNOLOGY_CELL_BIT;
                }
                if (QMI_LOC_POS_TECH_MASK_WIFI_V02 & ind.batchedReportList[i].technologyMask) {
                    temp.techMask |= LOCATION_TECHNOLOGY_WIFI_BIT;
                }
                if (QMI_LOC_POS_TECH_MASK_SENSORS_V02 & ind.batchedReportList[i].technologyMask) {
                    temp.techMask |= LOCATION_TECHNOLOGY_SENSORS_BIT;
                }
            }
            if (ind.batchedReportList[i].validFields &
                QMI_LOC_BATCHED_REPORT_MASK_VALID_VERT_UNC_V02) {
                temp.verticalAccuracy = ind.batchedReportList[i].vertUnc;
                temp.flags |= LOCATION_HAS_VERTICAL_ACCURACY_BIT;
            }
            if (ind.batchedReportList[i].validFields &
                QMI_LOC_BATCHED_REPORT_MASK_VALID_SPEED_UNC_V02) {
                temp.speedAccuracy = ind.batchedReportList[i].speedUnc;
                temp.flags |= LOCATION_HAS_SPEED_ACCURACY_BIT;
            }
            if (ind.batchedReportList[i].validFields &
                QMI_LOC_BATCHED_REPORT_MASK_VALID_HEADING_UNC_V02) {
                temp.bearingAccuracy = ind.batchedReportList[i].headingUnc;
                temp.flags |= LOCATION_HAS_BEARING_ACCURACY_BIT;
            }
            pLocationPiece[i] = temp;
        }
        numbOfEntries = ind.numberOfEntries;
        LOC_LOGd("read out %zu batched locations from modem.", numbOfEntries);
    } else {
        LOC_LOGd("Modem does not return batched location.");
    }
}

LocationError LocApiV02::queryAccumulatedTripDistanceSync(uint32_t &accumulatedTripDistance,
        uint32_t &numOfBatchedPositions)
{
    locClientReqUnionType req_union;
    locClientStatusEnumType status = eLOC_CLIENT_SUCCESS;

    qmiLocQueryOTBAccumulatedDistanceReqMsgT_v02 accumulated_distance_req;
    qmiLocQueryOTBAccumulatedDistanceIndMsgT_v02 accumulated_distance_ind;

    memset(&accumulated_distance_req,0,sizeof(accumulated_distance_req));
    memset(&accumulated_distance_ind, 0, sizeof(accumulated_distance_ind));

    req_union.pQueryOTBAccumulatedDistanceReq = &accumulated_distance_req;

    status = locSyncSendReq(QMI_LOC_QUERY_OTB_ACCUMULATED_DISTANCE_REQ_V02,
                            req_union,
                            LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                            QMI_LOC_QUERY_OTB_ACCUMULATED_DISTANCE_IND_V02,
                            &accumulated_distance_ind);
    if ((eLOC_CLIENT_SUCCESS != status) ||
        (eQMI_LOC_SUCCESS_V02 != accumulated_distance_ind.status))
    {
        return LOCATION_ERROR_GENERAL_FAILURE;
    }
    else
    {
        LOC_LOGD("Got accumulated distance: %u number of accumulated positions %u",
        accumulated_distance_ind.accumulatedDistance, accumulated_distance_ind.batchedPosition);

        accumulatedTripDistance = accumulated_distance_ind.accumulatedDistance;
        numOfBatchedPositions = accumulated_distance_ind.batchedPosition;

        return LOCATION_ERROR_SUCCESS;
    }
}

void LocApiV02::queryAccumulatedTripDistance(
        LocApiResponseData<LocApiBatchData>* adapterResponseData)
{
    sendMsg(new LocApiMsg([this, adapterResponseData] () {
        LocationError err = LOCATION_ERROR_SUCCESS;
        LocApiBatchData data;
        err = queryAccumulatedTripDistanceSync(data.accumulatedDistance,
                                               data.numOfBatchedPositions);
        if (adapterResponseData != NULL) {
            adapterResponseData->returnToSender(err, data);
        }
    }));
}

void LocApiV02::addToCallQueue(LocApiResponse* adapterResponse)
{
    sendMsg(new LocApiMsg([adapterResponse] () {
        if (adapterResponse != NULL) {
            adapterResponse->returnToSender(LOCATION_ERROR_SUCCESS);
        }
    }));
}

/* convert dgnss constellation mask from QMI loc to loc eng format */
void LocApiV02::convertGnssConestellationMask(
        qmiLocGNSSConstellEnumT_v02 qmiConstellationEnum,
        GnssConstellationTypeMask& constellationMask) {

    constellationMask = 0x0;
    if (qmiConstellationEnum & eQMI_SYSTEM_GPS_V02) {
        constellationMask |= GNSS_CONSTELLATION_TYPE_GPS_BIT;
    }

    if (qmiConstellationEnum & eQMI_SYSTEM_GLO_V02) {
        constellationMask |= GNSS_CONSTELLATION_TYPE_GLONASS_BIT;
    }

    if (qmiConstellationEnum & eQMI_SYSTEM_BDS_V02) {
        constellationMask |= GNSS_CONSTELLATION_TYPE_BEIDOU_BIT;
    }

    if (qmiConstellationEnum & eQMI_SYSTEM_GAL_V02) {
        constellationMask |= GNSS_CONSTELLATION_TYPE_GALILEO_BIT;
    }

    if (qmiConstellationEnum & eQMI_SYSTEM_QZSS_V02) {
        constellationMask |= GNSS_CONSTELLATION_TYPE_QZSS_BIT;
    }

    if (qmiConstellationEnum & eQMI_SYSTEM_NAVIC_V02) {
        constellationMask |= GNSS_CONSTELLATION_TYPE_NAVIC_BIT;
    }
}

GnssSignalTypeMask LocApiV02::convertQmiGnssSignalType(
        qmiLocGnssSignalTypeMaskT_v02 qmiGnssSignalType) {
    GnssSignalTypeMask gnssSignalType = (GnssSignalTypeMask)0;

    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L1CA_V02) {
        gnssSignalType |= GNSS_SIGNAL_GPS_L1CA;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L1C_V02) {
        gnssSignalType |= GNSS_SIGNAL_GPS_L1C;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L2C_L_V02) {
        gnssSignalType |= GNSS_SIGNAL_GPS_L2;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GPS_L5_Q_V02) {
        gnssSignalType |= GNSS_SIGNAL_GPS_L5;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GLONASS_G1_V02) {
        gnssSignalType |= GNSS_SIGNAL_GLONASS_G1;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GLONASS_G2_V02) {
        gnssSignalType |= GNSS_SIGNAL_GLONASS_G2;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E1_C_V02) {
        gnssSignalType |= GNSS_SIGNAL_GALILEO_E1;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E5A_Q_V02) {
        gnssSignalType |= GNSS_SIGNAL_GALILEO_E5A;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_GALILEO_E5B_Q_V02) {
        gnssSignalType |= GNSS_SIGNAL_GALILEO_E5B;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B1_I_V02) {
        gnssSignalType |= GNSS_SIGNAL_BEIDOU_B1I;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B1C_V02) {
        gnssSignalType |= GNSS_SIGNAL_BEIDOU_B1C;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2_I_V02) {
        gnssSignalType |= GNSS_SIGNAL_BEIDOU_B2I;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2A_I_V02) {
        gnssSignalType |= GNSS_SIGNAL_BEIDOU_B2AI;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2B_I_V02) {
        gnssSignalType |= GNSS_SIGNAL_BEIDOU_B2BI;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2B_Q_V02) {
        gnssSignalType |= GNSS_SIGNAL_BEIDOU_B2BQ;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L1CA_V02) {
        gnssSignalType |= GNSS_SIGNAL_QZSS_L1CA;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L1S_V02) {
        gnssSignalType |=  GNSS_SIGNAL_QZSS_L1S;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L2C_L_V02) {
        gnssSignalType |= GNSS_SIGNAL_QZSS_L2;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_QZSS_L5_Q_V02) {
        gnssSignalType |= GNSS_SIGNAL_QZSS_L5;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_SBAS_L1_CA_V02) {
        gnssSignalType |= GNSS_SIGNAL_SBAS_L1;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_NAVIC_L5_V02) {
        gnssSignalType |= GNSS_SIGNAL_NAVIC_L5;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_NAVIC_L1_V02) {
        gnssSignalType |= GNSS_SIGNAL_NAVIC_L1;
    }
    if (qmiGnssSignalType & QMI_LOC_MASK_GNSS_SIGNAL_TYPE_BEIDOU_B2A_Q_V02) {
        gnssSignalType |= GNSS_SIGNAL_BEIDOU_B2AQ;
    }

    return gnssSignalType;
}

AgcStatus LocApiV02::convertQmiAgcStatusType(qmiLocAgcStatusEnumT_v02 qmiAgcStatus) {
    AgcStatus agcStatus = AGC_STATUS_UNKNOWN;
    switch (qmiAgcStatus) {
        case eQMI_LOC_NO_SATURATION_V02:
            agcStatus = AGC_STATUS_NO_SATURATION;
            break;
        case eQMI_LOC_FRONT_END_GAIN_MAXIMUM_SATURATION_V02:
            agcStatus = AGC_STATUS_FRONT_END_GAIN_MAXIMUM_SATURATION;
            break;
        case eQMI_LOC_FRONT_END_GAIN_MINIMUM_SATURATION_V02:
            agcStatus = AGC_STATUS_FRONT_END_GAIN_MINIMUM_SATURATION;
            break;
        default:
            break;
    }
    return agcStatus;
}

// < SEC_GPS
int LocApiV02 :: setSecGnssParams()
{
  LOC_LOGD("%s:%d]: Entered. \n", __func__, __LINE__);
  char url[MAX_SUPL_URL_LEN];
  int len = 0;

  len = snprintf(url, sizeof(url), "%s:%u", sec_gps_conf.SUPL_HOST, (unsigned) sec_gps_conf.SUPL_PORT);
  if (setServerSync(url, len, LOC_AGPS_SUPL_SERVER)) {
    LOC_LOGE("fail to setServer");
  }


  if (setSuplSecurity(sec_gps_conf.SSL) < 0) {
    LOC_LOGE("fail to setSsl");
  }

  if (setSUPLVersionSync((GnssConfigSuplVersion)sec_gps_conf.SUPL_VERSION) < 0) {
    LOC_LOGE("fail to setSUPLVersion");
  }

  if (setAGLONASSProtocolSync((GnssConfigAGlonassPositionProtocolMask)sec_gps_conf.AGNSS_PROTOCOL) < 0) {
    LOC_LOGE("fail to setAGLONASSProtocol");
  }

  if (setLPPConfigSync((GnssConfigLppProfileMask)sec_gps_conf.LPP_PROFILE) < 0) {
    LOC_LOGE("fail to setLPPConfig");
  }

  if (setLPPeProtocolCpSync((GnssConfigLppeControlPlaneMask)sec_gps_conf.LPPE_CP_TECHNOLOGY) < 0) {
    LOC_LOGE("fail to setLPPeProtocolCp");
  }

  if (setLPPeProtocolUpSync((GnssConfigLppeUserPlaneMask)sec_gps_conf.LPPE_UP_TECHNOLOGY) < 0) {
    LOC_LOGE("fail to setLPPeProtocolUp");
  }

  if (sec_gps_conf.WEEK_NUMBER > 0) {
    configMinGpsWeek((uint16_t)sec_gps_conf.WEEK_NUMBER, NULL);
  }

  if (sec_gps_conf.SPIRENT == 0 || sec_gps_conf.SPIRENT == 1) {
    if (setSpirentType(sec_gps_conf.SPIRENT) < 0) {
      LOC_LOGE("fail to setSpirentYype");
    }
  }

  if (sec_gps_conf.AGPS_MODE > 0) {
    setAgpsMode(sec_gps_conf.AGPS_MODE);
  }

  if (setXtraEnable(sec_gps_conf.ENABLE_XTRA) < 0)
  {
    LOC_LOGE("fail to setXtraEnable");
  }

  if (mIsWifiOnly) {
    const int GNSS_RF_GPS_GLO = 3;
    if (strncmp(getSalesCode(), "XAR", 3) == 0) {
      if (setGnssRfConfig(GNSS_RF_GPS_GLO) < 0) {
        LOC_LOGE("fail to setGnssRfConfig");
      }
    } else {
      if (setGnssRfConfig(sec_gps_conf.GNSS_RF_CONFIG) < 0) {
        LOC_LOGE("fail to setGnssRfConfig");
      }
    }
  } else {
    if (setGnssRfConfig(sec_gps_conf.GNSS_RF_CONFIG) < 0) {
      LOC_LOGE("fail to setGnssRfConfig");
    }
  }

  setL5EnableConfig(sec_gps_conf.ENABLE_L5, true, sec_gps_conf.ENABLE_L5_TIS, true);

  setPrintNavmsgConfig();

  return 0;
}

enum loc_api_adapter_err LocApiV02 :: setSuplSecurity(int enable)
{
  locClientStatusEnumType result = eLOC_CLIENT_SUCCESS;
  locClientReqUnionType req_union;

  qmiLocSetProtocolConfigParametersReqMsgT_v02 supl_config_req;
  qmiLocSetProtocolConfigParametersIndMsgT_v02 supl_config_ind;

  LOC_LOGD("%s:%d]: supl security = %d\n",  __func__, __LINE__, enable);

  memset(&supl_config_req, 0, sizeof(supl_config_req));
  memset(&supl_config_ind, 0, sizeof(supl_config_ind));

  supl_config_req.suplSecurity_valid = 1;

  supl_config_req.suplSecurity= (enable == 1)? 0x01 : 0x00;

  req_union.pSetProtocolConfigParametersReq = &supl_config_req;

  result = loc_sync_send_req(clientHandle,
                             QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_REQ_V02,
                             req_union, LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
                             QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_IND_V02,
                             &supl_config_ind);

  if (result != eLOC_CLIENT_SUCCESS ||
     eQMI_LOC_SUCCESS_V02 != supl_config_ind.status) {
    LOC_LOGE ("%s:%d]: Error status = %s, ind..status = %s ",
              __func__, __LINE__,
              loc_get_v02_client_status_name(result),
              loc_get_v02_qmi_status_name(supl_config_ind.status));

    return LOC_API_ADAPTER_ERR_GENERAL_FAILURE;
  }

  return LOC_API_ADAPTER_ERR_SUCCESS;
}

void LocApiV02 :: setSecGnssConfiguration (const char* sec_ext_config, int32_t length)
{
  char url[MAX_SUPL_URL_LEN];
  int len = 0;

  LOC_LOGD("%s:%d]: length = %d\n",  __func__, __LINE__, length);

  sec_gps_conf.USE_SECGPS_CONF = SEC_CONF_NONE;
  if (sec_ext_config && length > 0) {
    loc_sec_gps_cfg_s sec_gps_conf_tmp = sec_gps_conf;
    uint32_t table_length = 0;
    loc_param_s_type* sec_param_table = loc_secgps_get_params_table(&table_length);
    loc_update_conf(sec_ext_config, length, sec_param_table, table_length);

    if (sec_gps_conf_tmp.AGPS_TYPE != sec_gps_conf.AGPS_TYPE
       ||(strcmp(sec_gps_conf_tmp.SUPL_HOST, sec_gps_conf.SUPL_HOST)!=0)
       ||sec_gps_conf_tmp.SUPL_PORT != sec_gps_conf.SUPL_PORT) {
      len = snprintf(url, sizeof(url), "%s:%u", sec_gps_conf.SUPL_HOST, (unsigned) sec_gps_conf.SUPL_PORT);
      setServerSync(url, len, LOC_AGPS_SUPL_SERVER);
    }
    if (sec_gps_conf_tmp.SUPL_VERSION != sec_gps_conf.SUPL_VERSION) {
      setSUPLVersionSync((GnssConfigSuplVersion)sec_gps_conf.SUPL_VERSION);
    }
    if (sec_gps_conf_tmp.SSL != sec_gps_conf.SSL) {
      setSuplSecurity(sec_gps_conf.SSL);
    }
    if (sec_gps_conf_tmp.AGNSS_PROTOCOL != sec_gps_conf.AGNSS_PROTOCOL) {
      setAGLONASSProtocolSync((GnssConfigAGlonassPositionProtocolMask)sec_gps_conf.AGNSS_PROTOCOL);
    }
    if (sec_gps_conf_tmp.AGPS_MODE != sec_gps_conf.AGPS_MODE) {
      setAgpsMode(sec_gps_conf.AGPS_MODE);
    }
    if (sec_gps_conf_tmp.LPP_PROFILE != sec_gps_conf.LPP_PROFILE) {
      setLPPConfigSync((GnssConfigLppProfileMask)sec_gps_conf.LPP_PROFILE);
    }
    if (sec_gps_conf_tmp.GNSS_RF_CONFIG != sec_gps_conf.GNSS_RF_CONFIG) {
      setGnssRfConfig(sec_gps_conf.GNSS_RF_CONFIG);
    }
    if (sec_gps_conf_tmp.ENGINE_ONLY_MODE != sec_gps_conf.ENGINE_ONLY_MODE) {
      setEngineOnlyMode(sec_gps_conf.ENGINE_ONLY_MODE);
    }
    if (strcmp(sec_gps_conf_tmp.BARO_CAL, sec_gps_conf.BARO_CAL) !=0) {
      char baro_cal_sentence[20] = "BARO_CAL=";
      strcat(baro_cal_sentence, sec_gps_conf.BARO_CAL);
      setAgnssConfigMsg((uint8_t*)baro_cal_sentence);
    }
    if (sec_gps_conf_tmp.LPPE_CP_TECHNOLOGY != sec_gps_conf.LPPE_CP_TECHNOLOGY) {
      setLPPeProtocolCpSync((GnssConfigLppeControlPlaneMask)sec_gps_conf.LPPE_CP_TECHNOLOGY);
    }
    if (sec_gps_conf_tmp.LPPE_UP_TECHNOLOGY != sec_gps_conf.LPPE_UP_TECHNOLOGY) {
      setLPPeProtocolUpSync((GnssConfigLppeUserPlaneMask)sec_gps_conf.LPPE_UP_TECHNOLOGY);
    }
    if (sec_gps_conf.USE_SECGPS_CONF == SEC_CONF_ON) {
      refreshSecConfFile(sec_ext_config, length);
    } else if (sec_gps_conf.USE_SECGPS_CONF == SEC_CONF_OFF) {
      deleteSecConfFile();
    }
    if(sec_gps_conf.BD_QUERY >=0){
      sendBDQuery();
    }
    if (sec_gps_conf_tmp.ENABLE_L5 != sec_gps_conf.ENABLE_L5){
      setL5EnableConfig(sec_gps_conf.ENABLE_L5, true, 0, false);
    }
    if (sec_gps_conf_tmp.ENABLE_L5_TIS != sec_gps_conf.ENABLE_L5_TIS){
      setL5EnableConfig(0, false, sec_gps_conf.ENABLE_L5_TIS, true);
    }
    if (strcmp(sec_gps_conf_tmp.CTS_RESTRICTMODE, sec_gps_conf.CTS_RESTRICTMODE) !=0) {
      if(strcmp(sec_gps_conf.CTS_RESTRICTMODE,"restrict")==0){
        sendSAPconfigtoCP();
      }
    }
    if(sec_gps_conf.LID_STATE >= 0) {
      sendLidState();
    }
    if(sec_gps_conf.UPDATE_QCOM_NV_ITEM[0] != '\0'){
      sendUpdateNv();
    }
    if (sec_gps_conf.EMERGENCY_SMS >= 0) {
      sendEmergencySMS();
    }
    // SEC: manage NMEA listener.
    if (sec_gps_conf_tmp.NMEA_ALLOWED != sec_gps_conf.NMEA_ALLOWED) {
      registerEventMask();
    }
    if(sec_gps_conf.SIM_SLOT_ID >= 0) {
      sim_slotId = sec_gps_conf.SIM_SLOT_ID;
    }

    if(sec_gps_conf_tmp.XTRA_THROTTLE != sec_gps_conf.XTRA_THROTTLE) {
      if(sec_gps_conf.XTRA_THROTTLE == 0) {
        setXtraThrottleEnable(false);
      } else if(sec_gps_conf.XTRA_THROTTLE == 1) {
        setXtraThrottleEnable(true);
      }
    }
    if (sec_gps_conf_tmp.PRINT_NAVMSG != sec_gps_conf.PRINT_NAVMSG){
      setPrintNavmsgConfig();
    }
    if (sec_gps_conf.CARRIER_STATE_CHANGED >= 0 && sim_slotId == sec_gps_conf.CARRIER_STATE_CHANGED) {
        requestSetSecGnssParams();
        sec_gps_conf.CARRIER_STATE_CHANGED = -1;
    }
  }
}

void LocApiV02 :: sendSAPconfigtoCP()
{
  LOC_LOGD("%s:%d]: sendSAPconfigtoCP for disabling SAP \n",  __func__, __LINE__);

  locClientStatusEnumType status;
  locClientReqUnionType req_union;

  qmiLocSetPremiumServicesCfgReqMsgT_v02  preminum_config_req;
  qmiLocSetPremiumServicesCfgIndMsgT_v02  preminum_config_ind;

  memset(&preminum_config_req, 0, sizeof(preminum_config_req));
  memset(&preminum_config_ind, 0, sizeof(preminum_config_ind));
 
  preminum_config_req.premiumServiceCfg = eQMI_LOC_PREMIUM_SERVICE_DISABLED_V02;
  preminum_config_req.premiumServiceType = eQMI_LOC_PREMIUM_SERVICE_SAP_V02;
 
  req_union.pSetPremiumServicesCfgReq = &preminum_config_req;

  status = locSyncSendReq(QMI_LOC_SET_PREMIUM_SERVICES_CONFIG_REQ_V02,
      req_union,
      LOC_ENGINE_SYNC_REQUEST_TIMEOUT,
      QMI_LOC_SET_PREMIUM_SERVICES_CONFIG_IND_V02,
      &preminum_config_ind);
  
  if (status != eLOC_CLIENT_SUCCESS ||
      (preminum_config_ind.status != eQMI_LOC_SUCCESS_V02 &&
          preminum_config_ind.status != eQMI_LOC_ENGINE_BUSY_V02)) {
      LOC_LOGE("%s:%d]: sendSAPconfigtoCP failed. status: %s, ind status:%s\n",
          __func__, __LINE__,
          loc_get_v02_client_status_name(status),
          loc_get_v02_qmi_status_name(preminum_config_ind.status));
  }
  else {
      LOC_LOGD("%s:%d]: sendSAPconfigtoCP succeeded.\n",
          __func__, __LINE__);
  }
}

void LocApiV02 :: requestSetSecGnssParams()
{
  if (mInSession) {
      mPendingSetParam = true;
      LOC_LOGD("GPS session is on going, set param is pending");
  } else {
      mSecDefaultInitDone = false;
      handleEngineDownEvent();
      LOC_LOGD("sec set param by modem request");
  }
}

/*===========================================================================
FUNCTION    loc_eng_check_smd_mode

DESCRIPTION
    Figure out if SMD command is triggered.

DEPENDENCIES
    None

RETURN VALUE
    bool

SIDE EFFECTS
    N/A
===========================================================================*/
static bool checkSmdFactoryMode()
{
  bool isSMDCmd = false;
  char szSmdPath[124] = {0,};
  char szRespPath[124] = {0,};
  FILE *fpr = NULL;
  FILE *fpw = NULL;

  strncpy(szSmdPath, SMD_FTM_FILE_PATH, strlen(SMD_FTM_FILE_PATH));
  strncpy(szRespPath, GPS_CNO_FILE_PATH, strlen(GPS_CNO_FILE_PATH));
  fpr = fopen(szSmdPath, "r");

  if (fpr != NULL) {
    fpw = fopen(szRespPath, "w");
    if(fpw != NULL) {
      fprintf(fpw, "%3d\n", SMD_FTM_RESP_OK);
      isSMDCmd = true;
      fclose(fpw);
    }
    fclose(fpr);
  }

  return isSMDCmd;
}

/*===========================================================================
FUNCTION    loc_eng_check_factory_mode

DESCRIPTION
    Figure out which mode this session is.

DEPENDENCIES
    None

RETURN VALUE
    loc_ftm_system_e_type

SIDE EFFECTS
    N/A
===========================================================================*/
static loc_ftm_system_e_type checkFactoryMode()
{
  loc_ftm_system_e_type system_type = FTM_NONE_SYSTEM;
  FILE *fp  = NULL;
  char szGpsPath[124] = {0,};
  char szGloPath[124] = {0,};
  char szBdsPath[124] = {0,};

  strncpy(szGpsPath, GPS_FTM_FILE_PATH, strlen(GPS_FTM_FILE_PATH));
  strncpy(szGloPath, GLONASS_FTM_FILE_PATH, strlen(GLONASS_FTM_FILE_PATH));
  strncpy(szBdsPath, BEIDOU_FTM_FILE_PATH, strlen(BEIDOU_FTM_FILE_PATH));

  if ((fp = fopen(szGpsPath, "r")) != NULL) {
    system_type = FTM_GPS_SYSTEM;
    fclose(fp);
  } else if ((fp = fopen(szGloPath, "r")) != NULL) {
    system_type = FTM_GLONASS_SYSTEM;
    fclose(fp);
  }
  else if ((fp = fopen(szBdsPath, "r")) != NULL) {
    system_type = FTM_BEIDOU_SYSTEM;
    fclose(fp);
  }
  return system_type;
}

/*===========================================================================
FUNCTION    loc_eng_init_factory_mode

DESCRIPTION
    Initialize VSS QCCI associated with factory command Samsung designed.

DEPENDENCIES
    None

RETURN VALUE
    TRUE : SUCCESS

SIDE EFFECTS
    N/A
===========================================================================*/
static int performFactoryMode(loc_ftm_action_e_type action, loc_ftm_system_e_type system)
{
  LOC_LOGI("performFactoryMode action = %d, System = %d", action, system);

  int freq = 0;

  if ((action == FTM_OPERATION_ACTIVATE) || (action == FTM_OPERATION_READ)) {
    if(system == FTM_GLONASS_SYSTEM) {
      FILE *fp;
      char szGloMultiChPath[124] = {0,};

      strncpy(szGloMultiChPath, GLONASS_FTM_MULTI_CN_PATH, strlen(GLONASS_FTM_MULTI_CN_PATH));

      fp = fopen(szGloMultiChPath, "r");

      if(fp != NULL) {
        int ret;
        ret = fscanf(fp, "%2d", &freq);
        fclose(fp);
        if (ret < 1) {
          LOC_LOGE("performFactoryMode : fscanf failed = %d", ret);
          return 0;
        }
        LOC_LOGI("performFactoryMode : read Glonass freq = %d", freq);
      }
    }
  }
  secgps_operation_ftm_mode(action, system, freq);
  return 0;
}

static int createTimerFactoryMode()
{
  int status;
  struct sigevent ev;
  struct itimerspec t;

  memset(&ev, 0, sizeof(ev));
  ev.sigev_notify = SIGEV_THREAD;
  ev.sigev_notify_function = handlefactoryReadTimer;
  ev.sigev_notify_attributes = NULL;
  ev.sigev_value.sival_ptr = NULL;

  memset(&t, 0, sizeof(t));
  t.it_value.tv_sec = 1; // 1second
  t.it_interval.tv_sec = t.it_value.tv_sec;

  if(0 == timer_create(CLOCK_REALTIME, &ev, &factoryTimerId)) {
    LOC_LOGI("createTimerFactoryMode - create timer successfully");
    status = timer_settime(factoryTimerId, 0, &t, NULL);
      if (-1 == status) {
        LOC_LOGE("Set timer Failed");
      }
  }
  return 0;
}

static void handlefactoryReadTimer(sigval_t user_data)
{
  LOC_LOGD("handlefactoryReadTimer");
  performFactoryMode(FTM_OPERATION_READ, system_type);
}

static void setAgpsMode(int agps_mode)
{
  char agpsmode_sentence[20] = "AGPS_MODE=";
  char agpsmode_value[3];
  snprintf(agpsmode_value,sizeof(agpsmode_value),"%d",sec_gps_conf.AGPS_MODE);
  strcat(agpsmode_sentence, agpsmode_value);
  setAgnssConfigMsg((uint8_t*)agpsmode_sentence);
}

static int setXtraEnable(int enable_xtra)
{
  LOC_LOGD("setXtraEnable, %d", enable_xtra);
  secgps_set_xtra_enable(enable_xtra);
  return 0;
}

static int setGnssRfConfig(int gnss_cfg)
{
  LOC_LOGD("setGnssRfConfig, %d", gnss_cfg);
  secgps_set_gnss_rf_config(gnss_cfg);
  return 0;
}

static int setSpirentType(int spirent_type)
{
  LOC_LOGD("setSpirentType, %d", spirent_type);
  secgps_set_spirent_type(spirent_type);
  return 0;
}

static void setGpsBatteryFlag(int connect)
{
  FILE *fp = fopen(BATTERY_GPS, "w");
  LOC_LOGD("setGpsBatteryFlag, connect=%d", connect);
  if (fp != NULL) {
    fprintf(fp, "%d", (connect == 1)? 1:0);
    fclose(fp);
  } else {
    LOC_LOGE("setGpsBatteryFlag() : file open error");
  }
}

static int setAgnssConfigMsg(uint8_t* msg_sentence)
{
  LOC_LOGD("setAgnssConfigMsg, %s", msg_sentence);
  secgps_agnss_config_message(msg_sentence, (uint32_t)strlen((char*)msg_sentence));
  return 0;
}

static void setL5EnableConfig(unsigned int ENABLE_L5, bool set_L5_flag, unsigned int ENABLE_L5_TIS, bool set_L5_TIS_flag)
{
  if(set_L5_flag == true){
    char Enable_L5_sentence[20] = "ENABLE_L5=";
    char Enable_L5_type[2];
    snprintf(Enable_L5_type,sizeof(Enable_L5_type),"%d",sec_gps_conf.ENABLE_L5);
    strcat(Enable_L5_sentence, Enable_L5_type);
    setAgnssConfigMsg((uint8_t*)Enable_L5_sentence);
  }

  if(set_L5_TIS_flag == true){
    char Enable_L5_TIS_sentence[20] = "ENABLE_L5_TIS=";
    char Enable_L5_TIS_type[2];
    snprintf(Enable_L5_TIS_type,sizeof(Enable_L5_TIS_type),"%d",sec_gps_conf.ENABLE_L5_TIS);
    strcat(Enable_L5_TIS_sentence, Enable_L5_TIS_type);
    setAgnssConfigMsg((uint8_t*)Enable_L5_TIS_sentence);
  }
}

static int setEngineOnlyMode(uint8_t enabled)
{
  LOC_LOGD("setEngineOnlyMode, enabled = %d", enabled);
  secgps_change_engine_only_mode(enabled);
  return 0;
}

static int setXtraThrottleEnable(bool enabled)
{
  LOC_LOGI("%s, enabled: %d", __FUNCTION__, enabled);

  shared_ptr<LocIpcSender> xtraSender;
  xtraSender = LocIpc::getLocIpcLocalSender(LOC_IPC_XTRA);
  
  stringstream ss;
  ss <<  "xtrathrottle";
  ss << " " << (enabled ? 1 : 0);
  string s = ss.str();
  int ret = LocIpc::send(*xtraSender, (const uint8_t*)s.data(), s.size());
  
  LOC_LOGD("%s, sending ipc: %d", __FUNCTION__, ret);
  return ret;
}

static void secGnssInit()
{
  LOC_LOGD("%s", __FUNCTION__);
}

static int setGnssConfig(const char* sec_ext_config, int32_t length)
{
  LOC_LOGD("%s", __FUNCTION__);
  if (sLocApi != NULL) {
    sLocApi->setSecGnssConfiguration(sec_ext_config, length);
  } else {
    LOC_LOGE("GNSS locapi hasn't be set yet");
  }
  return 0;
}

static void sendBDQuery(){
  char BD_query_sentence[20] = "BD_QUERY=";
  char BD_type[2];
  snprintf(BD_type,sizeof(BD_type),"%d",sec_gps_conf.BD_QUERY);
  strcat(BD_query_sentence, BD_type);
  setAgnssConfigMsg((uint8_t*)BD_query_sentence);
  sec_gps_conf.BD_QUERY = -1;
}

static void sendLidState(){
  char lid_state_sentence[20] = "LID_STATE=";
  char lid_state[2];
  snprintf(lid_state,sizeof(lid_state),"%d",sec_gps_conf.LID_STATE);
  strcat(lid_state_sentence, lid_state);
  setAgnssConfigMsg((uint8_t*)lid_state_sentence);
  sec_gps_conf.LID_STATE = -1;
}

static void sendUpdateNv(){
  char updateQcomNvSentence[150] ="UPDATE_QCOM_NV_ITEM=";
  strcat(updateQcomNvSentence, sec_gps_conf.UPDATE_QCOM_NV_ITEM);
  setAgnssConfigMsg((uint8_t*)updateQcomNvSentence);
  sec_gps_conf.UPDATE_QCOM_NV_ITEM[0] = '\0';
}

static void sendEmergencySMS(){
  char emergency_sms_sentence[20] = "EMERGENCY_SMS=";
  char emergency_sms[2];
  snprintf(emergency_sms,sizeof(emergency_sms),"%d",sec_gps_conf.EMERGENCY_SMS);
  strcat(emergency_sms_sentence, emergency_sms);
  setAgnssConfigMsg((uint8_t*)emergency_sms_sentence);
  sec_gps_conf.EMERGENCY_SMS = -1;
}
static void setPrintNavmsgConfig()
{
  char print_navmsg_sentence[20] = "PRINT_NAVMSG=";
  char print_navmsg_type[2];
  snprintf(print_navmsg_type,sizeof(print_navmsg_type),"%d",sec_gps_conf.PRINT_NAVMSG);
  strcat(print_navmsg_sentence, print_navmsg_type);
  setAgnssConfigMsg((uint8_t*)print_navmsg_sentence);
}

static void sendSilentResetMessage(int value) {
    char msg[MAX_SILENT_RESET_MSG_SIZE];

    memset(msg, NULL, sizeof(msg));
    snprintf(msg, sizeof(msg), "BD_IND=$SRMSG,%d", value);

    handleAgnssConfigIndMsg((uint8_t*)msg, (uint32_t)sizeof(msg));
}

static int injectNetworkInitiatedMessage(char* msg, int32_t msg_len)
{
  LOC_LOGD("%s subId = %d", __FUNCTION__,sec_gps_conf.NI_SUPL_SLOTID);
  secgps_inject_ni_message((uint8_t*)msg, (uint32_t)msg_len,(int8_t)sec_gps_conf.NI_SUPL_SLOTID);
  sec_gps_conf.NI_SUPL_SLOTID = -1;
  return 0;
}

static void updateSecgpsCallback(SecGpsCallbacks& callbacks) {
  LOC_LOGD("%s", __FUNCTION__);
  secGpsCb.bdmsg_cb= callbacks.bdmsg_cb;
}

static bool checkPositionFixDelayMode()
{
  int ret = false;
  if (sgps_position_delay_cnt > 0 && sec_gps_conf.START_MODE==0 && report_sv_cnt > NO_SERVICE_THRESHOLD) {
    sgps_position_delay_cnt--;
    LOC_LOGD("SGPSPositionFixDelayMode is enabled");
    ret = true;
  } else if (agps_position_delay_cnt > 0 && sec_gps_conf.START_MODE==0) {
    agps_position_delay_cnt--;
    LOC_LOGD("AGPSPositionFixDelayMode is enabled");
    ret = true;
  } else {
    LOC_LOGD("PositionFixDelayMode is disabled");
  }
  return ret;
}

static void refreshSecConfFile(const char* sec_ext_config, int32_t length) {
  LOC_LOGD("%s", __FUNCTION__);
  int fd;
  if ((fd = creat(SEC_GPS_CONF_FILE, 0644)) < 0) {
    LOC_LOGE("Failed to create SEC GPS CONFIG file");
  } else {
    LOC_LOGD("Writing SEC GPS CONFIG file");
    write(fd, sec_ext_config, length);
    close(fd);
  }
}

static void deleteSecConfFile() {
  LOC_LOGD("%s", __FUNCTION__);
  int res = remove(SEC_GPS_CONF_FILE);
  if (res == 0) {
    LOC_LOGD("Deleted SEC GPS CONFIG file");
  } else {
    LOC_LOGE("Failed to deleted SEC GPS CONFIG file");
  }
}

void handleAgnssConfigIndMsg(uint8_t* ind_msg, uint32_t length) {
  LOC_LOGV("%s  ind_msg = %s", __FUNCTION__,ind_msg);

  char *lasts = NULL;
  char *config_name = NULL;

  uint8_t AgnssConfigInd_msg[2048];
  char bdmsg[2048];
  memset(AgnssConfigInd_msg, 0, sizeof(AgnssConfigInd_msg));
  memset(bdmsg, 0, sizeof(bdmsg));
  memcpy(AgnssConfigInd_msg, ind_msg, length);
  config_name = strtok_r((char *)AgnssConfigInd_msg, "=", &lasts);
  if (config_name == NULL) {
    LOC_LOGE("ind_msg is not available");
    return;  
  }  
  if(strcmp(config_name,"FTM_IND")==0)
  {
    LOC_LOGE("%s", ind_msg);
    return;
  }
  if (strcmp(config_name,"BD_IND") == 0 && length > 6) {
    memcpy(bdmsg, lasts, length - strlen(config_name));
    if (secGpsCb.bdmsg_cb != NULL) {
      secGpsCb.bdmsg_cb(bdmsg, strlen(bdmsg));
    } else {
      LOC_LOGE("secGpsCb hasn't been set yet");
    }

    char bdmsg_copy[2048] = {0,};
    strcpy(bdmsg_copy, bdmsg);
          
    char *msg_name = NULL;
    char *next_ptr = NULL;
    strtok_r(bdmsg_copy, ",", &next_ptr);
    msg_name = strtok_r(NULL, ",", &next_ptr);
     
    if (sec_gps_conf.NMEA_ALLOWED && strcmp(msg_name,"JAMMER_MSG") == 0) {
      if (sLocApi != NULL) {
        sLocApi->reportNmea(bdmsg, strlen(bdmsg));
      } else {
        LOC_LOGE("GNSS locapi hasn't be set yet");
      }
    }
  } 
  else if (strcmp(config_name,"BD_MASK") == 0 && length > 7) {
    memcpy(bdmsg, lasts ,length - strlen(config_name));
    if (sLocApi != NULL) {
      sLocApi->reportNmea(bdmsg, strlen(bdmsg));
    } else {
      LOC_LOGE("GNSS locapi hasn't be set yet");
    }
  } 
  else if (strcmp(config_name,"BD_SIM")==0 && length >= 7 && length <= 11) {
    if (sLocApi != NULL) {
      sLocApi->requestSetSecGnssParams();
    } else {
      LOC_LOGE("GNSS locapi hasn't be set yet");
    }
  }
}

void sendPSMSG(void){

    char bd_msg[80];
    char bd_checksum_sentence[6] = "*00\n";
    char isMagcalDone[10] = "Mag=F";
    char isGyrocalDone[10] = "Gyro=UN";
    
    memset(bd_msg, 0, sizeof(bd_msg));

    if (secFixInfoMask & MAG_CALIBRATED){
        strlcpy(isMagcalDone , "Mag=P", sizeof(isMagcalDone));
    }

    if (isLastGyroCalDone == GYRO_CAL){
        strlcpy(isGyrocalDone , "Gyro=P", sizeof(isGyrocalDone));
    }
    else if (isLastGyroCalDone == GYRO_UNCAL){
        strlcpy(isGyrocalDone , "Gyro=F", sizeof(isGyrocalDone));
    }

    if (secFixInfoMask & GYRO_UNCALIBRATED){
        strlcpy(isGyrocalDone , "Gyro=F", sizeof(isGyrocalDone));
        isLastGyroCalDone = GYRO_UNCAL;
    }
    else if (secFixInfoMask & GYRO_CALIBRATED){
        strlcpy(isGyrocalDone , "Gyro=P", sizeof(isGyrocalDone));
        isLastGyroCalDone = GYRO_CAL;
    }

    snprintf (bd_msg, sizeof(bd_msg), "BD_MASK=$PSMSG,%s,%s,bluesky=%d%s", isMagcalDone, isGyrocalDone, isBlueskyInjected, bd_checksum_sentence);

    handleAgnssConfigIndMsg((uint8_t*)bd_msg, (uint32_t)sizeof(bd_msg));
    isBlueskyInjected = false;

}
// SEC_GPS >
