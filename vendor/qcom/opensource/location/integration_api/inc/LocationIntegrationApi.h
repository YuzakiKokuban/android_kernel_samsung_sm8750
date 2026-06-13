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

#ifndef LOCATION_INTEGRATION_API_H
#define LOCATION_INTEGRATION_API_H

#include <loc_pla.h>
#include <LocationClientApi.h>

#include <array>
#include <unordered_set>
#include <unordered_map>

namespace location_integration
{
/**
 * Configuration API types that are currently supported
 */
enum LocConfigTypeEnum{
    /** Blacklist some SV constellations from being used by the GNSS
     *  standard position engine (SPE). <br/> */
    CONFIG_CONSTELLATIONS = 1,
    /** Enable/disable the constrained time uncertainty feature and
     *  configure related parameters when the feature is
     *  enabled. <br/> */
    CONFIG_CONSTRAINED_TIME_UNCERTAINTY = 2,
    /** Enable/disable the position assisted clock estimator
     *  feature. <br/> */
    CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR = 3,
    /** Delete aiding data. This enum is applicable for
     *  deleteAllAidingData() and deleteAidingData(). <br/> */
    CONFIG_AIDING_DATA_DELETION = 4,
    /** Config lever arm parameters. <br/> */
    CONFIG_LEVER_ARM = 5,
    /** Config robust location feature. <br/> */
    CONFIG_ROBUST_LOCATION = 6,
    /** Config minimum GPS week used by the GNSS standard
     *  position engine (SPE). <br/> */
    CONFIG_MIN_GPS_WEEK = 7,
    /** Config vehicle Body-to-Sensor mount angles for dead
     *  reckoning position engine. <br/> */
    CONFIG_BODY_TO_SENSOR_MOUNT_PARAMS = 8,
    /** Config various parameters for dead reckoning position
     *  engine. <br/> */
    CONFIG_DEAD_RECKONING_ENGINE = 9,
    /** Config minimum SV elevation angle setting used by the GNSS
     *  standard position engine (SPE).
     *  <br/> */
    CONFIG_MIN_SV_ELEVATION = 10,
    /** Config the secondary band for configurations used by the GNSS
     *  standard position engine (SPE).
     *  <br/> */
    CONFIG_CONSTELLATION_SECONDARY_BAND = 11,
    /** Config the run state, e.g.: pause/resume, of the position
     * engine <br/> */
    CONFIG_ENGINE_RUN_STATE = 12,
    /** Config user consent to use GTP terrestrial positioning
     *  service. <br/> */
    CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING = 13,
    /** Config the output nmea sentence types. <br/> */
    CONFIG_OUTPUT_NMEA_TYPES = 14,
    /** Config the integrity risk level of the position engine.
     *  <br/> */
    CONFIG_ENGINE_INTEGRITY_RISK = 15,
    /** Config the xtra parameters used by the standard position
     *  engine (SPE). <br/> */
    CONFIG_XTRA_PARAMS = 16,
    /** Inject the MerkleTree used by the standard position
        engine (SPE). <br/> */
    CONFIG_MERKLE_TREE = 17,
    /** Config OSNMA enablement status used by the standard
     *  position engine (SPE). <br/> */
    CONFIG_OSNMA_ENABLEMENT = 18,
    /** Config Map Matched Feedback used by DRE/PPE engine */
    CONFIG_MAP_MATCHED_FEEDBACK = 19,
    /** Max config enum supported. <br/> */
    CONFIG_ENUM_MAX = 99,

    /** Get configuration regarding robust location setting used by
     *  the GNSS standard position engine (SPE).  <br/> */
    GET_ROBUST_LOCATION_CONFIG = 100,
    /** Get minimum GPS week configuration used by the GNSS standard
     *  position engine (SPE).
     *  <br/> */
    GET_MIN_GPS_WEEK = 101,
    /** Get minimum SV elevation angle setting used by the GNSS
     *  standard position engine (SPE). <br/> */
    GET_MIN_SV_ELEVATION = 102,
    /** Get the secondary band configuration for constellation
     *  used by the GNSS standard position engine (SPE). <br/> */
    GET_CONSTELLATION_SECONDARY_BAND_CONFIG = 103,
    /** Query xtra feature setting and xtra assistance data
     *  status. <br/> */
    GET_XTRA_STATUS = 104,
    /** Register the callback to get update on xtra feature setting
     *  and xtra assistance data status. <br/> */
    REGISTER_XTRA_STATUS_UPDATE = 105,
    /** Register the callback to get update on GNSS signal type
     *  capabilities. <br/> */
    REGISTER_SIGNAL_TYPES_UPDATE = 106,
} ;

/**
 *  Specify the asynchronous response when calling location
 *  integration API. */
enum LocIntegrationResponse {
    /** Location integration API request is processed
     *  successfully. <br/>  */
    LOC_INT_RESPONSE_SUCCESS = 1,
    /** Location integration API request is not processed
     *  successfully. <br/>  */
    LOC_INT_RESPONSE_FAILURE = 2,
    /** Location integration API request is not supported. <br/> */
    LOC_INT_RESPONSE_NOT_SUPPORTED = 3,
    /** Location integration API request has invalid parameter.
     *  <br/> */
    LOC_INT_RESPONSE_PARAM_INVALID = 4,
} ;

/** Specify the position engine types used in location
 *  integration api module. <br/> */
enum LocIntegrationEngineType {
    /** This is the standard GNSS position engine. <br/> */
    LOC_INT_ENGINE_SPE   = 1,
    /** This is the precise position engine. <br/> */
    LOC_INT_ENGINE_PPE   = 2,
    /** This is the dead reckoning position engine. <br/> */
    LOC_INT_ENGINE_DRE   = 3,
    /** This is the vision positioning engine. <br/>  */
    LOC_INT_ENGINE_VPE   = 4,
};

/** Specify the position engine run state. <br/> */
enum LocIntegrationEngineRunState {
    /** Request the position engine to be put into pause state.
     *  <br/> */
    LOC_INT_ENGINE_RUN_STATE_PAUSE   = 1,
    /** Request the position engine to be put into resume state.
     *  <br/> */
    LOC_INT_ENGINE_RUN_STATE_RESUME   = 2,
    /** Request the selected position engine to be put into pause state
     *  while retaining of any useful state data.
     *  This engine run state is currently applicable to QDR engine only.
     *  It is strongly advised to link this state to a vehicle state in which
     *  the vehicle is expected to be stationary at the time of invocation of API
     *  and subsequently, until the state is changed to Running.
     *  For QDR, transition out of PAUSE_RETAIN happens
     *  when either the state is changed to RESUME state via same command OR
     *  when the device taken through suspend/resume or reboot power-state cycles.
     *  <br/> */
    LOC_INT_ENGINE_RUN_STATE_PAUSE_RETAIN   = 3,
};

/**
 * Define the priority to be used when the corresponding
 * configuration API specified by type is invoked. Priority is
 * specified via uint32_t and lower number means lower
 * priority.  <br/>
 *
 * Currently, all configuration requests, regardless of
 * priority, will be honored. Priority based request
 * honoring will come in subsequent phases and more
 * detailed description on this will be available then. <br/>
 */
typedef std::unordered_map<LocConfigTypeEnum, uint32_t>
        LocConfigPriorityMap;

/** Gnss constellation type mask. <br/> */
typedef uint32_t GnssConstellationMask;

/**
 *  Specify SV Constellation types that is used in
 *  constellation configuration. <br/> */
enum GnssConstellationType {
    /** GLONASS SV system  <br/> */
    GNSS_CONSTELLATION_TYPE_GLONASS  = 1,
    /** QZSS SV system <br/> */
    GNSS_CONSTELLATION_TYPE_QZSS     = 2,
    /** BEIDOU SV system <br/> */
    GNSS_CONSTELLATION_TYPE_BEIDOU   = 3,
    /** GALILEO SV system <br/> */
    GNSS_CONSTELLATION_TYPE_GALILEO  = 4,
    /** SBAS SV system <br/> */
    GNSS_CONSTELLATION_TYPE_SBAS     = 5,
    /** NAVIC SV system <br/> */
    GNSS_CONSTELLATION_TYPE_NAVIC    = 6,
    /** GPS SV system <br/> */
    GNSS_CONSTELLATION_TYPE_GPS      = 7,
    /** Maximum constellatoin system */
    GNSS_CONSTELLATION_TYPE_MAX      = 7,
};

/**
 * Specify the constellation and sv id for an SV. This struct
 * is used to specify the blacklisted SVs in
 * configConstellations(). <br/> */
struct GnssSvIdInfo {
    /** constellation for the sv <br/>  */
    GnssConstellationType constellation;
    /** sv id range for the constellation: <br/>
     * GPS SV id range: 1 to 32 <br/>
     * GLONASS SV id range: 65 to 96 <br/>
     * QZSS SV id range: 193 to 197 <br/>
     * BDS SV id range: 201 to 263 <br/>
     * GAL SV id range: 301 to 336 <br/>
     * SBAS SV id range: 120 to 158 and 183 to 191 <br/>
     * NAVIC SV id range: 401 to 414 <br/>
     */
    uint32_t              svId;
};

/**
 *  Mask used to specify the set of aiding data that can be
 *  deleted via deleteAidingData(). <br/> */
enum AidingDataDeletionMask {
    /** Mask to delete ephemeris aiding data <br/> */
    AIDING_DATA_DELETION_EPHEMERIS  = (1 << 0),
    /** Mask to delete calibration data from dead reckoning position
     *  engine.<br/> */
    AIDING_DATA_DELETION_DR_SENSOR_CALIBRATION  = (1 << 1),
};

/**
 *  Lever ARM type <br/> */
enum LeverArmType {
    /** Lever arm parameters regarding the VRP (Vehicle Reference
     *  Point) w.r.t the origin (at the GNSS Antenna). <br/> */
    LEVER_ARM_TYPE_GNSS_TO_VRP = 1,
    /** Lever arm regarding GNSS Antenna w.r.t the origin at the
     *  IMU (inertial measurement unit) for DR (dead reckoning
     *  engine). <br/> */
    LEVER_ARM_TYPE_DR_IMU_TO_GNSS = 2,
    /** Lever arm regarding GNSS Antenna w.r.t the origin at the
     *  IMU (inertial measurement unit) for VEPP (vision enhanced
     *  precise positioning engine). <br/>
     *  Note: this enum will be deprecated. <br/> */
    LEVER_ARM_TYPE_VEPP_IMU_TO_GNSS = 3,
    /** Lever arm regarding GNSS Antenna w.r.t the origin at the
     *  IMU (inertial measurement unit) for VPE (vision positioning
     *  engine). <br/> */
    LEVER_ARM_TYPE_VPE_IMU_TO_GNSS = 3,
};

/**
 * Specify parameters related to lever arm. <br/> */
struct LeverArmParams {
    /** Offset along the vehicle forward axis, in unit of meters
     *  <br/> */
    float forwardOffsetMeters;
    /** Offset along the vehicle starboard axis, in unit of meters.
     *  Left side offset is negative, and right side offset is
     *  positive. <br/> */
    float sidewaysOffsetMeters;
    /** Offset along the vehicle up axis, in unit of meters. <br/> */
    float upOffsetMeters;
};

/**
 * Specify vehicle body-to-Sensor mount parameters for use by
 * dead reckoning positioning engine. <br/> */
struct BodyToSensorMountParams {
    /** The misalignment of the sensor board along the
     *  horizontal plane of the vehicle chassis measured looking
     *  from the vehicle to forward direction. <br/>
     *  In unit of degrees. <br/>
     *  Range [-180.0, 180.0]. <br/> */
    float rollOffset;
    /** The misalignment along the horizontal plane of the vehicle
     *  chassis measured looking from the vehicle to the right
     *  side. Positive pitch indicates vehicle is inclined such
     *  that forward wheels are at higher elevation than rear
     *  wheels. <br/>
     *  In unit of degrees. <br/>
     *  Range [-180.0, 180.0]. <br/> */
    float yawOffset;
    /** The angle between the vehicle forward direction and the
     *  sensor axis as seen from the top of the vehicle, and
     *  measured in counterclockwise direction. <br/>
     *  In unit of degrees. <br/>
     *  Range [-180.0, 180.0]. <br/> */
    float pitchOffset;
    /** Single uncertainty number that may be the largest of the
     *  uncertainties for roll offset, pitch offset and yaw
     *  offset. <br/>
     *  In unit of degrees. <br/>
     *  Range [-180.0, 180.0]. <br/> */
    float offsetUnc;
};

/** Specify the valid mask for the configuration paramters of
 *  dead reckoning position engine <br/> */
enum DeadReckoningEngineConfigValidMask {
    /** DeadReckoningEngineConfig has valid
     *  DeadReckoningEngineConfig::bodyToSensorMountParams. */
    BODY_TO_SENSOR_MOUNT_PARAMS_VALID    = (1<<0),
    /** DeadReckoningEngineConfig has valid
     *  DeadReckoningEngineConfig::vehicleSpeedScaleFactor. */
    VEHICLE_SPEED_SCALE_FACTOR_VALID     = (1<<1),
    /** DeadReckoningEngineConfig has valid
     *  DeadReckoningEngineConfig::vehicleSpeedScaleFactorUnc. */
    VEHICLE_SPEED_SCALE_FACTOR_UNC_VALID = (1<<2),
    /** DeadReckoningEngineConfig has valid
     *  DeadReckoningEngineConfig::gyroScaleFactor. */
    GYRO_SCALE_FACTOR_VALID              = (1<<3),
    /** DeadReckoningEngineConfig has valid
     *  DeadReckoningEngineConfig::gyroScaleFactorUnc. */
    GYRO_SCALE_FACTOR_UNC_VALID          = (1<<4),
};

/** Specify the valid mask for the dead reckoning position
 *  engine <br/> */
struct DeadReckoningEngineConfig{
    /** Specify the valid fields in the config. */
    DeadReckoningEngineConfigValidMask validMask;
    /** Body to sensor mount parameters for use by dead reckoning
     *  positioning engine */
    BodyToSensorMountParams bodyToSensorMountParams;

    /** Vehicle Speed Scale Factor configuration input for the dead
      * reckoning positioning engine. The multiplicative scale
      * factor is applied to received Vehicle Speed value (in m/s)
      * to obtain the true Vehicle Speed.
      *
      * Range is [0.9 to 1.1].
      *
      * Note: The scale factor is specific to a given vehicle
      * make & model. */
    float vehicleSpeedScaleFactor;
    /** Vehicle Speed Scale Factor Uncertainty (68% confidence)
      * configuration input for the dead reckoning positioning
      * engine.
      *
      * Range is [0.0 to 0.1].
      *
      * Note: The scale factor unc is specific to a given vehicle
      * make & model. */
    float vehicleSpeedScaleFactorUnc;

    /** Gyroscope Scale Factor configuration input for the dead
      * reckoning positioning engine. The multiplicative scale
      * factor is applied to received gyroscope value to obtain the
      * true value.
      *
      * Range is [0.9 to 1.1].
      *
      * Note: The scale factor is specific to the Gyroscope sensor
      * and typically derived from either sensor data-sheet or
      * from actual calibration. */
    float gyroScaleFactor;

    /** Gyroscope Scale Factor uncertainty (68% confidence)
      * configuration input for the dead reckoning positioning
      * engine.
      *
      * Range is [0.0 to 0.1].
      * engine.
      *
      * Note: The scale factor unc is specific to the make & model
      * of Gyroscope sensor and typically derived from either
      * sensor data-sheet or from actual calibration. */
    float gyroScaleFactorUnc;
};

/**
 * Define the lever arm parameters to be used with
 * configLeverArm(). <br/>
 *
 * For the types of lever arm parameters that can be configured,
 * refer to LeverArmType. <br/>
 */
typedef std::unordered_map<LeverArmType, LeverArmParams> LeverArmParamsMap;

/**
 * Specify the absolute set of constellations and SVs
 * that should not be used by the GNSS standard position engine
 * (SPE). <br/>
 *
 * To blacklist all SVs from one constellation, use
 * GNSS_SV_ID_BLACKLIST_ALL as sv id for that constellation.
 * <br/>
 *
 * To specify only a subset of the SVs to be blacklisted, for
 * each SV, specify its constelaltion and the SV id and put in
 * the vector. <br/>
 *
 * All SVs being blacklisted should not be used in positioning.
 * For SBAS, SVs are not used in positioning by the GNSS
 * standard position engine (SPE) by default. Blacklisting SBAS
 * SV only blocks SBAS data demod and will not disable SBAS
 * cross-correlation detection algorithms as they are necessary
 * for optimal GNSS standard position engine (SPE) performance.
 * Also, if SBAS is disabld via NV in modem, then it can not be
 * re-enabled via location integration API. <br/>
 *
 * For SV id range, refer documentation of GnssSvIdInfo::svId. <br/>
 */
#define GNSS_SV_ID_BLACKLIST_ALL (0)
typedef std::vector<GnssSvIdInfo> LocConfigBlacklistedSvIdList;

/**
 * Define the constellation set.
 */
typedef std::unordered_set<GnssConstellationType> ConstellationSet;

/** @brief
    Used to get the asynchronous notification of the processing
    status of the configuration APIs. <br/>

    In order to get the notification, an instantiation
    LocConfigCb() need to be passed to the constructor of
    LocationIntegration API. Please refer to each function for
    details regarding how this callback will be invoked. <br/>

    @param
    response: if the response is not LOC_INT_API_RESPONSE_SUCCESS,
    then the integration API of requestType has failed. <br/>
*/
typedef std::function<void(
    /** location configuration request type <br/> */
    LocConfigTypeEnum      configType,
    /** processing status for the location configuration request
     *  <br/> */
    LocIntegrationResponse response
)> LocConfigCb;

/** Specify the valid mask for robust location configuration
 *  used by the GNSS standard position engine (SPE). The robust
 *  location configuraiton can be retrieved by invoking
 *  getRobustLocationConfig(). <br/> */
enum RobustLocationConfigValidMask {
    /** RobustLocationConfig has valid
     *  RobustLocationConfig::enabled. <br/> */
    ROBUST_LOCATION_CONFIG_VALID_ENABLED          = (1<<0),
    /** RobustLocationConfig has valid
     *  RobustLocationConfig::enabledForE911. <br/> */
    ROBUST_LOCATION_CONFIG_VALID_ENABLED_FOR_E911 = (1<<1),
    /** RobustLocationConfig has valid
     *  RobustLocationConfig::version. <br/> */
    ROBUST_LOCATION_CONFIG_VALID_VERSION = (1<<2),
};

/** Specify the versioning info of robust location module for
 *  the GNSS standard position engine (SPE). The versioning info
 *  is part of RobustLocationConfig and will be returned when
 *  invoking getRobustLocationConfig(). RobustLocationConfig()
 *  will be returned via
 *  LocConfigGetRobustLocationConfigCb(). <br/> */
struct RobustLocationVersion {
    /** Major version number. <br/> */
    uint8_t major;
    /** Minor version number. <br/> */
    uint16_t minor;
};

/** Specify the robust location configuration used by the GNSS
 *  standard position engine (SPE) that will be returned when
 *  invoking getRobustLocationConfig(). The configuration will
 *  be returned via LocConfigGetRobustLocationConfigCb().
 *  <br/> */
struct RobustLocationConfig {
    /** Bitwise OR of RobustLocationConfigValidMask to specify
     *  the valid fields. <br/> */
    RobustLocationConfigValidMask validMask;
    /** Specify whether robust location feature is enabled or
     *  not. <br/> */
    bool enabled;
    /** Specify whether robust location feature is enabled or not
     *  when device is on E911 call. <br/> */
    bool enabledForE911;
    /** Specify the version info of robust location module used
     *  by the GNSS standard position engine (SPE). <br/> */
    RobustLocationVersion version;
};

/**
 *  Specify the callback to retrieve the robust location setting
 *  used by the GNSS standard position engine (SPE). The
 *  callback will be invoked for successful processing of
 *  getRobustLocationConfig(). <br/>
 *
 *  In order to receive the robust location configuration,
 *  client shall instantiate the callback and pass it to the
 *  LocationIntegrationApi constructor and then invoke
 *  getRobustLocationConfig(). <br/> */
typedef std::function<void(
    RobustLocationConfig robustLocationConfig
)> LocConfigGetRobustLocationConfigCb;

/**
 *  Specify the callback to retrieve the minimum GPS week
 *  configuration used by the GNSS standard position engine
 *  (SPE). The callback will be invoked for successful
 *  processing of getMinGpsWeek(). The callback shall be passed
 *  to the LocationIntegrationApi constructor. <br/> */
typedef std::function<void(
   uint16_t minGpsWeek
)> LocConfigGetMinGpsWeekCb;

/**
 *  Specify the callback to retrieve the minimum SV elevation
 *  angle setting used by the GNSS standard position engine
 *  (SPE). The callback will be invoked for successful
 *  processing of getMinSvElevation(). The callback shall be
 *  passed to the LocationIntegrationApi constructor. <br/> */
typedef std::function<void(
   uint8_t minSvElevation
)> LocConfigGetMinSvElevationCb;

/** @brief
    LocConfigGetConstellationSecondaryBandConfigCb is for
    receiving the GNSS secondary band configuration for
    constellation. <br/>

    @param secondaryBandDisablementSet: GNSS secondary
    band control configuration. <br/>
    An empty set means secondary bands are enabled for every
    supported constellation. <br/>
*/
typedef std::function<void(
    const ConstellationSet& secondaryBandDisablementSet
)> LocConfigGetConstellationSecondaryBandConfigCb;

/** Specify the XTRA status update trigger. <br/>
 *  The XTRA status update can be sent in two scenarios: by
 *  calling getXtraStatus() to get one time XTRA status update
 *  or by registering the callback via
 *  registerXtraStatusUpdate() to get asynchronous XTRA
 *  status update. <br/> */
enum XtraStatusUpdateTrigger {
    /** XTRA status update due to invoke getXtraStatus(). <br/> */
    XTRA_STATUS_UPDATE_UPON_QUERY = 1,
    /** XTRA status update due to first invokation of
     *  registerXtraStatusUpdate(). <br/> */
    XTRA_STATUS_UPDATE_UPON_REGISTRATION = 2,
    /** XTRA status update due to status change due to enablement
     *  and disablement and change in xtra assistance data status,
     *  e.g.: from unknown to known during device bootup, or when
     *  XTRA data gets downloaded. <br/> */
    XTRA_STATUS_UPDATE_UPON_STATUS_CHANGE = 3,
};

/** Specify the XTRA assistance data status. */
enum XtraDataStatus {
    /** If XTRA feature is disabled or if XTRA feature is enabled,
     *  but XTRA daemon has not yet retrieved the assistance data
     *  status from modem on early stage of device bootup, xtra data
     *  status will be unknown.  <br/>   */
    XTRA_DATA_STATUS_UNKNOWN = 0,
    /** If XTRA feature is enabled, but XTRA data is not present
     *  on the device. <br/>   */
    XTRA_DATA_STATUS_NOT_AVAIL = 1,
    /** If XTRA feature is enabled, XTRA data has been downloaded
     *  but it is no longer valid. <br/>   */
    XTRA_DATA_STATUS_NOT_VALID = 2,
    /** If XTRA feature is enabled, XTRA data has been downloaded
     *  and is currently valid. <br/>   */
    XTRA_DATA_STATUS_VALID = 3,
};

struct XtraStatus {
    /** XTRA assistance data and NTP time download is enabled or
     *  disabled. <br/> */
    bool featureEnabled;
    /** XTRA assistance data status. If XTRA assistance data
     *  download is not enabled, this field will be set to
     *  XTRA_DATA_STATUS_UNKNOWN. */
    XtraDataStatus xtraDataStatus;
    /** Number of hours that xtra assistance data will remain valid.
     *  <br/>
     *  This field will be valid when xtraDataStatus is set to
     *  XTRA_DATA_STATUS_VALID. <br/>
     *  For all other XtraDataStatus, this field will be set to
     *  0. <br/> */
    uint32_t xtraValidForHours;
};

/**
 *  Specify the callback to retrieve the xtra feature settings
 *  and xtra assistance data status. The callback will be
 *  invoked for either successful processing of getXtraStatus()
 *  and registerXtraStatusUpdate() or when the trigger for xtra
 *  status update gets fired for registerXtraStatusUpdate().
 *  <br/>
 *
 *  In order to receive the xtra status settings and xtra
 *  assistance data status, client shall first instantiate the
 *  callback and pass it to the LocationIntegrationApi
 *  constructor and then invoke getXtraStatus() or
 *  registerXtraStatusUpdate(). <br/> */
typedef std::function<void(
   /** Specify the update trigger, whether this is due to one time
    *  query of getXtraStatus(), or due to the xtra status update
    *  callback gets called due to registerXtraStatusUpdate().
    *  <br/> */
   XtraStatusUpdateTrigger updateTrigger,
   /** Specify the xtra feature status and xtra assistance data
    *  validity info. */
   const XtraStatus&    xtraStatus
)> LocConfigGetXtraStatusCb;

/**
 *  Specify the callback to receive GNSS signal type capabilities
 *  that modem supports. These capabilities represent the
 *  signal types the GNSS implementation supports with
 *  temporarily disabled signal types taken into account,
 *  such as the blocklisted satellites/constellations or
 *  the constellations disabled by regional restrictions. <br/>
 *
 *  In order to receive the GNSS signal type capabilities,
 *  client shall first instantiate the callback and
 *  pass it to the LocationIntegrationApi constructor
 *  and then invoke registerGnssSignalTypesUpdate() <br/> */
typedef std::function<void(
    /**  GNSS signal type capabilities with RF band. <br/> */
    location_client::GnssSignalTypeMask signalType
)> LocConfigGnssSignalTypesCb;

/**
 *  Specify the set of callbacks that can be passed to
 *  LocationIntegrationAPI constructor to receive configuration
 *  command processing status and the requested data. <br/>
 */
struct LocIntegrationCbs {
    /** Callback to receive the procesings status, e.g.: success
     *  or failure.  <br/> */
    LocConfigCb configCb;
    /** Callback to receive the robust location setting.  <br/> */
    LocConfigGetRobustLocationConfigCb getRobustLocationConfigCb;
    /** Callback to receive the minimum GPS week setting used by
     *  the GNSS standard position engine (SPE). <br/> */
    LocConfigGetMinGpsWeekCb getMinGpsWeekCb;
    /** Callback to receive the minimum SV elevation angle setting
     *  used by the GNSS standard position engine (SPE). <br/> */
    LocConfigGetMinSvElevationCb getMinSvElevationCb;
    /** Callback to receive the secondary band configuration for
     *  constellation. <br/> */
    LocConfigGetConstellationSecondaryBandConfigCb getConstellationSecondaryBandConfigCb;
    /** Callback to receive the xtra feature enablement setting and
     *  xtra assistance data status. <br/> */
    LocConfigGetXtraStatusCb getXtraStatusCb;
    /** Callback to receive the supported GNSS signal type
     *  capabilities. <br/> */
    LocConfigGnssSignalTypesCb gnssSignalTypesCb;
};

/** Specify the NMEA sentence types that the device will output
 *  via location_client::startPositionSession(). <br/>
 *
 *  Please note that this setting is only applicable if
 *  NMEA_PROVIDER in gps.conf is set to 0 to use HLOS
 *  generated NMEA. <br/> */
enum NmeaTypesMask {
    /** Enable HLOS to generate and output GGA NMEA sentence.
     *  <br/> */
    NMEA_TYPE_GGA      = (1<<0),
    /** Enable HLOS to generate and output RMC NMEA sentence.
     *  <br/> */
    NMEA_TYPE_RMC      = (1<<1),
    /** Enable HLOS to generate and output GSA NMEA sentence.
     *  <br/> */
    NMEA_TYPE_GSA      = (1<<2),
    /** Enable HLOS to generate and output VTG NMEA sentence.
     *  <br/> */
    NMEA_TYPE_VTG      = (1<<3),
    /** Enable HLOS to generate and output GNS NMEA sentence.
     *  <br/> */
    NMEA_TYPE_GNS      = (1<<4),
    /** Enable HLOS to generate and output DTM NMEA sentence.
     *  <br/> */
    NMEA_TYPE_DTM      = (1<<5),
    /** Enable HLOS to generate and output GPGSV NMEA sentence for
     *  SVs from GPS constellation. <br/> */
    NMEA_TYPE_GPGSV    = (1<<6),
    /** Enable HLOS to generate and output GLGSV NMEA sentence for
     *  SVs from GLONASS constellation. <br/> */
    NMEA_TYPE_GLGSV    = (1<<7),
    /** Enable HLOS to generate and output GAGSV NMEA sentence for
     *  SVs from GALILEO constellation. <br/> */
    NMEA_TYPE_GAGSV    = (1<<8),
    /** Enable HLOS to generate and output GQGSV NMEA sentence for
     *  SVs from QZSS constellation. <br/> */
    NMEA_TYPE_GQGSV    = (1<<9),
    /** Enable HLOS to generate and output GBGSV NMEA sentence for
     *  SVs from BEIDOU constellation. <br/> */
    NMEA_TYPE_GBGSV    = (1<<10),
    /** Enable HLOS to generate and output GIGSV NMEA sentence for
     *  SVs from NAVIC constellation. <br/> */
    NMEA_TYPE_GIGSV    = (1<<11),
    /** Enable HLOS to generate and output all supported NMEA
     *  sentences. <br/> */
    NMEA_TYPE_ALL        = 0xffffffff,
};

/**  Specify the Geodetic datum for NMEA sentence types that
 *  are generated by GNSS stack on HLOS. <br/>
 */
enum GeodeticDatumType {
    /** Geodetic datum defined in World Geodetic System 1984 (WGS84)
     *  format. <br/>
     */
    GEODETIC_TYPE_WGS_84 = 0,
    /** Geodetic datum defined for use in the GLONASS system. <br/>*/
    GEODETIC_TYPE_PZ_90 = 1,
};

/** Specify the logcat debug level. Currently, only XTRA
 *  daemon will support the runtime configure of debug log
 *  level. <br/>   */
enum DebugLogLevel {
    /** No debug message will be outputed. <br/>   */
    DEBUG_LOG_LEVEL_NONE = 0,
    /** Only error level debug messages will get logged. <br/>   */
    DEBUG_LOG_LEVEL_ERROR = 1,
    /** Only warning/error level debug messages will get logged.
     *  <br/> */
    DEBUG_LOG_LEVEL_WARNING = 2,
    /** Only info/wanring/error level debug messages will get
     *  logged. <br/>   */
    DEBUG_LOG_LEVEL_INFO = 3,
    /** Only debug/info/wanring/error level debug messages will
     *  get logged. <br/> */
    DEBUG_LOG_LEVEL_DEBUG = 4,
    /** Verbose/debug/info/wanring/error level debug messages will
     *  get logged. <br/>   */
    DEBUG_LOG_LEVEL_VERBOSE = 5,
};

/** Xtra feature configuration parameters */
struct XtraConfigParams {
    /** Number of minutes between periodic, consecutive successful
     *  XTRA assistance data downloads. <br/>
     *
     *  If 0 is specified, modem default download for XTRA
     *  assistance data will be performed. <br/>
     *
     *  If none-zero value is specified, the configured value is in
     *  unit of 1 minute and will be capped at a maximum of 168
     *  hours and minimum of 48 hours. <br/>
     */
    uint32_t xtraDownloadIntervalMinute;
    /** Connection timeout when connecting backend for both xtra
     *  assistance data download and NTP time downlaod.  <br/>
     *
     *  If 0 is specified, the download timeout value will use
     *  device default values. <br/>
     *
     *  If none-zero value is specified, the configured value is in
     *  in unit of 1 second and should be capped at maximum of 300
     *  secs (not indefinite) and minimum of 3 secs. <br/> */
    uint32_t xtraDownloadTimeoutSec;
    /** Interval to wait before retrying xtra assistance data
     *  download in case of device error. <br/>
     *
     *  If 0 is specified, XTRA download retry will follow device
     *  default behavior. <br/>
     *
     *  If none-zero value is specified, the config value is in unit
     *  of 1 minute and should be capped with maximum of 1 day and
     *  minimum of 3 minutes. <br/>
     *
     *  If zero is specified for xtraDownloadRetryIntervalMinute,
     *  then xtraDownloadRetryAttempts will also use device default
     *  value. <br/> */
    uint32_t xtraDownloadRetryIntervalMinute;
    /** Total number of allowed retry attempts for assistance data
     *  download in case of device error. <br/>
     *
     *  If 0 is specified, XTRA download retry will follow device
     *  default behavior. <br/>
     *
     *  The configured value is in unit of 1 retry and max number of
     *  allowed retry is 6 per download interval. <br/>
     *
     *  If zero is specified for xtraDownloadRetryAttempts, then
     *  xtraDownloadRetryIntervalMinute will also use device default
     *  value. <br/> */
    uint32_t xtraDownloadRetryAttempts;
    /** Path to the certificate authority (CA) repository that need
     *  to be used for XTRA assistance data download. <br/>
     *
     *  Max of 128 bytes, including null-terminating byte will be
     *  supported. <br/>
     *
     *  If empty string is specified, device default CA repositaory
     *  will be used. <br/>
     *
     *  Please note that this parameter does not apply to NTP time
     *  download. <br/> */
    std::string xtraCaPath;
    /** URLs from which XTRA assistance data will be fetched. <br/>
     *
     *  The URLs, if provided, shall be complete and shall include
     *  the port number to be used for download. <br/>
     *
     *  Max of 128 bytes, including null-terminating byte will be
     *  supported. <br/>
     *
     *  Valid xtra server URLs should start with "https://".
     *  <br/>
     *
     *  If XTRA server URLs are not specified, then device will use
     *  the default XTRA server from modem. <br/>
     */
    std::array<std::string, 3> xtraServerURLs;
    /** URLs for NTP server to fetch current time. <br/>
     *
     *  If no NTP server URL is provided, then device will use the
     *  default NTP server. <br/>
     *
     *  The URLs, if provided, shall include the port number to be
     *  used for download. <br/>
     *
     *  Max of 128 bytes, including null-terminating byte will be
     *  supported. <br/>
     *
     *  Example of valid ntp server URL is:
     *  ntp.exampleserver.com:123. <br/> */
    std::array<std::string, 3> ntpServerURLs;

    /** Enable or disable XTRA integrity download. Parameter is only
     *  applicable if XTRA data download is enabled. <br/>
     *
     *  true: enable XTRA integrity download. <br/>
     *  false: disable XTRA integrity download. <br/> */
    bool xtraIntegrityDownloadEnable;

    /** XTRA integrity download interval, only applicable if XTRA
     *  integrity download is enabled. <br/>
     *
     *  If 0 is specified, the download timeout value will use
     *  device default values. <br/>
     *
     *  Valid range is 360 minutes (6 hours) to 2880 minutes
     *  (48 hours), in unit of minutes. <br/> */
    uint32_t xtraIntegrityDownloadIntervalMinute;

    /** Level of debug log messages that will be logged. <br/> */
    DebugLogLevel xtraDaemonDebugLogLevel;

    /** URL of NTS KE Server. <br/>
     *
     *  The URL, if provided, shall be complete and shall include
     *  the port number. <br/>
     *
     *  Max of 128 bytes, including null-terminating byte will be
     *  supported. <br/>
     *
     *  Valid NTS KE server URL should start with "https://".
     *  <br/>
     *
     *  If NTS KE server URL is not specified, then device will use
     *  the default URL of https://nts.xtracloud.net:4460. <br/>
     */
    std::string ntsKeServerURL;

    /** To set the diag logging status for XTRA. <br/>
     *
     * 0 to disable diag logging <br/>
     * 1 to enable diag logging <br/> */
    uint32_t xtraDaemonDiagLoggingStatus;
};

enum mmfDataValidity {
    LOC_HAS_VALID_MMFD_UTC_TIME     = (1<<0),
    LOC_HAS_VALID_MMFD_LAT_DIFF     = (1<<1),
    LOC_HAS_VALID_MMFD_LONG_DIFF    = (1<<2),
    LOC_HAS_VALID_MMFD_TUNNEL       = (1<<3),
    LOC_HAS_VALID_MMFD_BEARING      = (1<<4),
    LOC_HAS_VALID_MMFD_ALTITUDE     = (1<<5),
    LOC_HAS_VALID_MMFD_HOR_ACC      = (1<<6),
    LOC_HAS_VALID_MMFD_ALT_ACC      = (1<<7),
    LOC_HAS_VALID_MMFD_BEARING_ACC  = (1<<8),
};

struct mapMatchedFeedbackData {
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

};

class LocationIntegrationApiImpl;
class LocationIntegrationApi
{
public:

    /** @brief
        Creates an instance of LocationIntegrationApi object with
        the specified priority map and callback functions. For this
        phase, the priority map will be ignored. <br/>

        @param
        priorityMap: specify the priority for each of the
        configuration type that this integration API client would
        like to control. <br/>

        @param
        integrationCbs: set of callbacks to receive info from
        location integration API. For example, client can pass
        LocConfigCb() to receive the asynchronous processing status
        of configuration command. <br/>
    */
    LocationIntegrationApi(const LocConfigPriorityMap& priorityMap,
                           LocIntegrationCbs& integrationCbs);

    /** @brief Default destructor <br/> */
    ~LocationIntegrationApi();

    /** @brief
        Blacklist some constellations or subset of SVs from the
        constellation from being used by the GNSS standard
        position engine (SPE). <br/>

        Please note this API call is not incremental and the new
        setting will completely overwrite the previous call.
        blacklistedSvList shall contain the complete list
        of blacklisted constellations and blacklisted SVs.
        Constellations and SVs not specified in the parameter will
        be considered to be allowed to get used by GNSS standard
        position engine (SPE).
        <br/>

        Client should wait for the command to finish, e.g.: via
        LocConfigCb() received before issuing a second
        configConstellations() command. Behavior is not defined if
        client issues a second request of configConstellations()
        without waiting for the finish of the previous
        configConstellations() request. <br/>

        @param
        blacklistedSvList, if not set to nullptr, shall contain
        the complete list of blacklisted constellations and
        blacklisted SVs. Constellations and SVs not specified in
        this parameter will be considered to be allowed to get used
        by GNSS standard position engine (SPE). <br/>

        Nullptr of blacklistedSvList will be interpreted as to reset
        the constellation configuration to device default. <br/>

        Empty blacklistedSvList will be interpreted as to not
        disable any constellation and not to blacklist any SV, which
        means all SVs from all constellations are allowed to be used
        by SPE. <br/>

        For example, if client wants to disable data demod for all
        SBAS SVs, client need to specify SBAS to be blacklisted by
        adding (GNSS_CONSTELLATION_TYPE_SBAS, 0) to param
        blacklistedSvList. Another example, if client wants to
        enable only GPS and GAL constellations to be used by SPE,
        then client needs to specify all the other constellations
        defined in GnssConstellationType as to be blacklisted, this
        means adding (GNSS_CONSTELLATION_TYPE_QZSS, 0),
        (GNSS_CONSTELLATION_TYPE_GPS, 0),
        (GNSS_CONSTELLATION_TYPE_GLONASS, 0),
        (GNSS_CONSTELLATION_TYPE_BEIDOU, 0),
        (GNSS_CONSTELLATION_TYPE_SBAS, 0) and
        (GNSS_CONSTELLATION_TYPE_NAVIC, 0) to param
        blacklistedSvList. <br/>

        For blacklist one or more SVs in one constellation,
        if client only wants to disale sv id 1 from GPS
        constellation, then client needs to add
        (GNSS_CONSTELLATION_TYPE_GPS, 1) to param blacklistedSvList.
        By doing this, all other SVs in GPS constellation whose SV
        id is not 1 and all SVs in non-GPS consteallations are
        allowed to be used by SPE. <br/>

        @return true, if request is successfully processed as
                requested. When returning true, LocConfigCb() will
                be invoked to deliver asynchronous processing
                status. <br/>

        @return false, if request is not successfully processed as
                requested. When returning false, LocConfigCb() will
                not be invoked. <br/>
    */
    bool configConstellations(const LocConfigBlacklistedSvIdList*
                              blacklistedSvList=nullptr);

    /** @brief
        This API configures the secondary band constellations used
        by the GNSS standard position engine. <br/>

        Please note this API call is not incremental and the new
        setting will completely overwrite the previous call. </br>

        secondaryBandDisablementSet contains
        the enable/disable secondary band info for supported
        constellations. If a constellation is not specified in the
        set, it will be treated as to enable the secondary bands
        for that constellation. Also, please note that the secondary
        bands can only be disabled then re-enabled for the
        constellation via this API if the secondary bands are
        enabled in NV in modem. If the NV in modem is set to disable
        the secondary bands for a particular constellation, then
        attempt to enable the secondary bands for this constellation
        via this API will be no-op. <br/>

        Client should wait for the command to finish, e.g.: via
        LocConfigCb() received before issuing a second
        configConstellationSecondaryBand() command. Behavior is not
        defined if client issues a second request of
        configConstellationSecondaryBand() without waiting for the
        finish of the previous configConstellationSecondaryBand()
        request. <br/>

        @param
        secondaryBandDisablementSet: specify the set of
        constellations whose secondary bands need to be
        disabled. <br/>

        Nullptr and empty secondaryBandDisablementSet will be
        interpreted as to enable the secondary bands for all
        supported constellations. Please note that if the secondary
        band for the constellation is disabled via modem NV, then it
        can not be enabled via this API. <br/>

        @return true, if request is successfully processed as
                requested. When returning true, LocConfigCb() will
                be invoked to deliver asynchronous processing
                status. <br/>

        @return false, if request is not successfully processed as
                requested. When returning false, LocConfigCb() will
                not be invoked. <br/>
    */
    bool configConstellationSecondaryBand(
            const ConstellationSet* secondaryBandDisablementSet);

    /** @brief
        Retrieve the secondary band config for constellation used by
        the standard GNSS engine (SPE). <br/>

        @return true, if the API request has been accepted. The
                successful status will be returned via configCB, and
                secondary band configuration used by the GNSS
                standard position engine (SPE) will be returned via
                LocConfigGetConstellationSecondaryBandConfigCb()
                passed via the constructor. <br/>

        @return false, if the API request has not been accepted for
                further processing. When returning false, LocConfigCb()
                and LocConfigGetConstellationSecondaryBandConfigCb()
                will not be invoked. <br/>
    */
    bool getConstellationSecondaryBandConfig();

     /** @brief
         Enable or disable the constrained time uncertainty feature.
         <br/>

         Client should wait for the command to finish, e.g.:
         via LocConfigCb() received before issuing a second
         configConstrainedTimeUncertainty() command. Behavior is not
         defined if client issues a second request of
         configConstrainedTimeUncertainty() without waiting for
         the finish of the previous
         configConstrainedTimeUncertainty() request. <br/>

         @param
         enable: true to enable the constrained time uncertainty
         feature and false to disable the constrainted time
         uncertainty feature. <br/>

         @param
         tuncThresholdMs: this specifies the time uncertainty
         threshold that GNSS standard position engine (SPE) need to
         maintain, in units of milli-seconds. Default is 0.0 meaning
         that default value of time uncertainty threshold will be
         used. This parameter is ignored when request is to disable
         this feature. <br/>

         @param
         energyBudget: this specifies the power budget that the GNSS
         standard position engine (SPE) is allowed to spend to
         maintain the time uncertainty. Default is 0 meaning that GPS
         engine is not constained by power budget and can spend as
         much power as needed. The parameter need to be specified in
         units of 0.1 milli watt second, e.g.: an energy budget of
         2.0 milli watt will be of 20 units. This parameter is
         ignored when request is to disable this feature. <br/>

        @return true, if the constrained time uncertainty feature is
                successfully enabled or disabled as requested.
                When returning true, LocConfigCb() will be invoked to
                deliver asynchronous processing status. <br/>

        @return false, if the constrained time uncertainty feature is
                not successfully enabled or disabled as requested.
                When returning false, LocConfigCb() will not be
                invoked. <br/>
    */
    bool configConstrainedTimeUncertainty(bool enable,
                                          float tuncThresholdMs = 0.0,
                                          uint32_t energyBudget = 0);

    /** @brief
        Enable or disable position assisted clock estimator feature.
        <br/>

        Client should wait for the command to finish, e.g.: via
        LocConfigCb() received before issuing a second
        configPositionAssistedClockEstimator(). Behavior is
        not defined if client issues a second request of
        configPositionAssistedClockEstimator() without waiting for
        the finish of the previous
        configPositionAssistedClockEstimator() request. <br/>

        @param
        enable: true to enable position assisted clock estimator and
        false to disable the position assisted clock estimator
        feature. <br/>

        @return true, if position assisted clock estimator is
                successfully enabled or disabled as requested. When
                returning true, LocConfigCb() will be invoked to
                deliver asynchronous processing status. <br/>

        @return false, if position assisted clock estimator is not
                successfully enabled or disabled as requested. When
                returning false, LocConfigCb() will not be invoked. <br/>
    */
    bool configPositionAssistedClockEstimator(bool enable);

   /** @brief
        Request deletion of all aiding data from all position
        engines on the device. <br/>

        Invoking this API will trigger cold start of all position
        engines on the device. This will cause significant delay
        for the position engines to produce next fix and may have
        other performance impact. So, this API should only be
        exercised with caution and only for very limited usage
        scenario, e.g.: for performance test and certification
        process. <br/>

        @return true, if the API request has been accepted for
                further processing. When returning true, LocConfigCb()
                with configType set to CONFIG_AIDING_DATA_DELETION
                will be invoked to deliver the asynchronous
                processing status. <br/>

        @return false, if the API request has not been accepted for
                further processing. When returning false, LocConfigCb()
                will not be invoked. <br/>
    */
    bool deleteAllAidingData();


   /** @brief
        Request deletion of the specified aiding data from all
        position engines on the device. <br/>

        Invoking this API may cause noticeable delay for the
        position engine to produce first fix and may have other
        performance impact. For example, remove ephemeris data may
        trigger the GNSS standard position engine (SPE) to do warm
        start. So, this API should only be exercised with caution
        and only for very limited usage scenario, e.g.: for
        performance test and certification process. <br/>

        @param aidingDataMask, specify the set of aiding data to
                be deleted from all position engines. Currently,
                only ephemeris deletion is supported. <br/>

        @return true, if the API request has been accepted for
                further processing. When returning true, LocConfigCb()
                with configType set to CONFIG_AIDING_DATA_DELETION
                will be invoked to deliver the asynchronous
                processing status. <br/>

        @return false, if the API request has not been accepted for
                further processing. When returning false, LocConfigCb()
                will not be invoked. <br/>
    */
    bool deleteAidingData(AidingDataDeletionMask aidingDataMask);


    /** @brief
        Sets the lever arm parameters for the vehicle. <br/>

        LeverArm is sytem level parameters and it is not expected to
        change. So, it is needed to issue configLeverArm() once for
        every application processor boot-up. For multiple
        invocations of this API, client should wait
        for the command to finish, e.g.: via LocConfigCb() received
        before issuing a second configLeverArm(). Behavior is not
        defined if client issues a second request of cconfigLeverArm
        without waiting for the finish of the previous
        configLeverArm() request. <br/>

        @param
        configInfo: lever arm configuration info regarding below two
        types of lever arm info: <br/>
        1: GNSS Antenna w.r.t the origin at the IMU (inertial
        measurement unit) for DR engine <br/>
        2: GNSS Antenna w.r.t the origin at the IMU (inertial
        measurement unit) for VEPP engine <br/>
        3: VRP (Vehicle Reference Point) w.r.t the origin (at the
        GNSS Antenna). Vehicle manufacturers prefer the position
        output to be tied to a specific point in the vehicle rather
        than where the antenna is placed (midpoint of the rear axle
        is typical). <br/>

        Caller can specify which types of lever arm info to
        configure via the leverMarkTypeMask. <br/>

        @return true, if lever arm parameters are successfully
                configured as requested. When returning true,
                LocConfigCb() will be invoked to deliver asynchronous
                processing status. <br/>

        @return false, if lever arm parameters are not successfully
                configured as requested. When returning false,
                LocConfigCb() will not be invoked. <br/>
    */
    bool configLeverArm(const LeverArmParamsMap& configInfo);


    /** @brief
        Enable or disable robust location 2.0 feature. When enabled,
        location_client::GnssLocation shall report conformity index
        of the GNSS navigation solution, which is a measure of
        robustness of the underlying navigation solution. It
        indicates how well the various input data considered for
        navigation solution conform to expectations. In the presence
        of detected spoofed inputs, the navigation solution may take
        corrective actions to mitigate the spoofed inputs and
        improve robustness of the solution. <br/>

        @param
        enable: true to enable robust location and false to disable
        robust location. <br/>

        @param
        enableForE911: true to enable robust location when device is
        on E911 session and false to disable on E911 session. <br/>
        This parameter is only valid if robust location is enabled.
        <br/>

        @return true, if robust location are successfully configured
                as requested. When returning true, LocConfigCb() will be
                invoked to deliver asynchronous processing status.
                <br/>

        @return false, if robust location are not successfully
                configured as requested. When returning false,
                LocConfigCb() will not be invoked. <br/>
    */
    bool configRobustLocation(bool enable, bool enableForE911=false);

    /** @brief
        Query robust location 2.0 setting and version info used by
        the GNSS standard position engine (SPE). <br/>

        If processing of the command fails, the failure status will
        be returned via LocConfigCb(). If the processing of the command
        is successful, the successful status will be returned via
        configCB, and the robust location config info will be
        returned via LocConfigGetRobustLocationConfigCb() passed via
        the constructor. <br/>

        @return true, if the API request has been accepted. <br/>

        @return false, if the API request has not been accepted for
                further processing. When returning false, LocConfigCb()
                and LocConfigGetRobustLocationConfigCb() will not be
                invoked. <br/>
    */
    bool getRobustLocationConfig();

    /** @brief
        Config the minimum GPS week used by the GNSS standard
        position engine (SPE). <br/>

        This API shall not be called while GNSS standard position
        engine(SPE) is in the middle of a session. Customer needs to
        assure that there is no active GNSS SPE session prior to
        issuing this command. Additionally the specified minimum GPS
        week number shall NEVER be in the future of the current GPS
        Week.

        Client should wait for the command to finish, e.g.: via
        LocConfigCb() received before issuing a second configMinGpsWeek()
        command. Behavior is not defined if client issues a second
        request of configMinGpsWeek() without waiting for the
        previous configMinGpsWeek() to finish. <br/>

        @param
        minGpsWeek: minimum GPS week to be used by the GNSS standard
        position engine (SPE). This value shall NEVER be in the
        future of the current GPS Week <br/>

        @return true, if minimum GPS week configuration has been
                accepted for further processing. When returning
                true, LocConfigCb() will be invoked to deliver
                asynchronous processing status. <br/>

        @return false, if configuring minimum GPS week is not
                accepted for further processing. When returning
                false, LocConfigCb() will not be invoked. <br/>
    */
    bool configMinGpsWeek(uint16_t minGpsWeek);

    /** @brief
        Retrieve minimum GPS week configuration used by the GNSS
        standard position engine (SPE). If processing of the command
        fails, the failure status will be returned via
        LocConfigCb(). If the processing of the command is
        successful, the successful status will be returned via
        configCB, and the minimum GPS week info will be returned via
        LocConfigGetMinGpsWeekCb() passed via the constructor. <br/>

        Also, if this API is called right after configMinGpsWeek(),
        the returned setting may not match the one specified in
        configMinGpsWeek(), as the setting configured via
        configMinGpsWeek() can not be applied to the GNSS standard
        position engine(SPE) when the engine is in middle of a
        session. In poor GPS signal condition, the session may take
        up to 255 seconds to finish. If after 255 seconds of
        invoking configMinGpsWeek(), the returned value still does
        not match, then the caller need to reapply the setting by
        invoking configMinGpsWeek() again. <br/>

        @return true, if the API request has been accepted. <br/>

        @return false, if the API request has not been accepted for
                further processing. When returning false, LocConfigCb()
                and LocConfigGetMinGpsWeekCb() will not be invoked.
                <br/>
    */
    bool getMinGpsWeek();

    /** @brief
        Configure the vehicle body-to-Sensor mount parameters
        for dead reckoning position engine. <br/>

        Client should wait for the command to finish, e.g.:
        via LocConfigCb() received before issuing a second
        configBodyToSensorMountParams() command. Behavior is not
        defined if client issues a second request of
        configBodyToSensorMountParams() without waiting for the
        finish of the previous configBodyToSensorMountParams()
        request. <br/>

        @param
        b2sParams: vehicle body-to-Sensor mount angles and
        uncertainty. <br/>

        @return true, if the request is accepted for further
                processing. When returning true, LocConfigCb() will be
                invoked to deliver asynchronous processing status. <br/>

        @return false, if the request is not accepted for further
                processing. When returning false, LocConfigCb() will not
                be invoked. <br/>
    */
    bool configBodyToSensorMountParams(const BodyToSensorMountParams& b2sParams);

    /** @brief
        Configure various parameters for dead reckoning position engine. <br/>

        Client should wait for the command to finish, e.g.:
        via LocConfigCb() received before issuing a second
        configDeadReckoningEngineParams() command. Behavior is not
        defined if client issues a second request of
        configDeadReckoningEngineParams() without waiting for the
        finish of the previous configDeadReckoningEngineParams()
        request. <br/>

        @param
        config: dead reckoning engine configuration. <br/>

        @return true, if the request is accepted for further
                processing. When returning true, LocConfigCb() will be
                invoked to deliver asynchronous processing status. <br/>

        @return false, if the request is not accepted for further
                processing. When returning false, LocConfigCb() will not
                be invoked. <br/>
    */
    bool configDeadReckoningEngineParams(const DeadReckoningEngineConfig& config);

    /** @brief
        Configure the minimum SV elevation angle setting used by the
        GNSS standard position engine (SPE). Configuring minimum SV
        elevation setting will not cause SPE to stop tracking low
        elevation SVs. It only controls the list of SVs that are
        used in the filtered position solution, so SVs with
        elevation below the setting will be excluded from use in the
        filtered position solution. Configuring this setting to
        large angle will cause more SVs to get filtered out in the
        filtered position solution and will have negative
        performance impact. <br/>

        Also, the SV info report as specified in
        location_client::GnssSv and SV measurement report as
        specified in location_client::GnssMeasurementsData will not
        be filtered based on the minimum SV elevation angle setting. <br/>

        To apply the setting, the GNSS standard position engine(SPE)
        will require MGP to be turned off briefly. This may cause
        glitch for on-going tracking session and may have other
        performance impact. So, it is advised to use this API with
        caution and only for very limited usage scenario, e.g.: for
        performance test and certification process and for one-time
        device configuration. <br/>

        Also, if this API is called while the GNSS standard position
        engine(SPE) is in middle of a session, LocConfigCb() will still
        be invoked shortly after to indicate the setting has been
        accepted by SPE engine, however the actual setting can not
        be applied until the current session ends, and this may take
        up to 255 seconds in poor GPS signal condition. <br/>

        Client should wait for the command to finish, e.g.: via
        LocConfigCb() received before issuing a second
        configMinSvElevation() command. Behavior is not defined if
        client issues a second request of configMinSvElevation()
        without waiting for the previous configMinSvElevation() to
        finish. <br/>

        @param
        minSvElevation: minimum SV elevation to be used by the GNSS
        standard position engine (SPE). Valid range is [0, 90] in
        unit of degree. <br/>

        @return true, if minimum SV elevation setting has been
                accepted for further processing. When returning
                true, LocConfigCb() will be invoked to deliver
                asynchronous processing status. <br/>

        @return false, if configuring minimum SV elevation is not
                accepted for further processing. When returning
                false, LocConfigCb() will not be invoked. <br/>
    */
    bool configMinSvElevation(uint8_t minSvElevation);

    /** @brief
        Retrieve minimum SV elevation angle setting used by the GNSS
        standard position engine (SPE). <br/>

        Also, if this API is called right after
        configMinSvElevation(), the returned setting may not match
        the one specified in configMinSvElevation(), as the setting
        configured via configMinSvElevation() can not be applied to
        the GNSS standard position engine(SPE) when the engine is in
        middle of a session. In poor GPS signal condition, the
        session may take up to 255 seconds to finish. If after 255
        seconds of invoking configMinSvElevation(), the returned
        value still does not match, then the caller need to reapply
        the setting by invoking configMinSvElevation() again. <br/>

        @return true, if the API request has been accepted. The
                successful status will be returned via configCB, and the
                minimum SV elevation angle setting will be returned
                via LocConfigGetMinSvElevationCb() passed via the
                constructor. <br/>

        @return false, if the API request has not been accepted for
                further processing. When returning false, LocConfigCb()
                and LocConfigGetMinSvElevationCb() will not be
                invoked. <br/>
    */
    bool getMinSvElevation();

    /** @brief
        This API is used to instruct the specified engine to be in
        the pause/resume state. <br/>

        When the engine is placed in paused state, the engine will
        stop. If there is an on-going session, engine will no longer
        produce fixes. In the paused state, calling API to delete
        aiding data from the paused engine may not have effect.
        Request to delete Aiding data shall be issued after
        engine resume. <br/>

        Currently, only DRE engine will support pause/resume
        request. LocConfigCb() will return
        LOC_INT_RESPONSE_NOT_SUPPORTED when request is made to
        pause/resume none-DRE engine. <br/>

        Request to pause/resume DRE engine can be made with or
        without an on-going session. With QDR engine, on resume,
        GNSS position & heading re-acquisition is needed for DR
        engine to engage. If DR engine is already in the requested
        state, the request will be no-op and the API call will
        return success and LocConfigCb() will return
        LOC_INT_RESPONSE_SUCCESS. <br/>

        @param
        engType: the engine that is instructed to change its run
        state. <br/>

        engState: the new engine run state that the engine is
        instructed to be in. <br/>

        @return true, if the API request has been accepted. The
                status will be returned via configCB. When returning
                true, LocConfigCb() will be invoked to deliver
                asynchronous processing status.
                <br/>

        @return false, if the API request has not been accepted for
                further processing. <br/>
    */
    bool configEngineRunState(LocIntegrationEngineType engType,
                              LocIntegrationEngineRunState engState);


    /** @brief
        Set client consent to use terrestrial positioning. <br/>

        Client must call this API with userConsent set to true in order
        to retrieve positions via
        LocationClientApi::getSingleTerrestrialPosition(),
        LocationClientApi::getSinglePosition(). <br/>

        The consent will remain effective across power cycles, until
        this API is called with a different value.  <br/>

        @param
        true: client agrees to the privacy entailed when using terrestrial
        positioning.
        false: client does not agree to the privacy entailed when using
        terrestrial positioning. Due to this, client will not be able to
        retrieve terrestrial position.

        @return true, if client constent has been accepted for further processing.
                When returning true, LocConfigCb() will be invoked to deliver
                asynchronous processing status. <br/>

        @return false, if client constent has not been accepted for further
                processing. When returning false, no further processing
                will be performed and LocConfigCb() will not be invoked.
                <br/>
    */
    bool setUserConsentForTerrestrialPositioning(bool userConsent);

    /** @brief
        This API is used to config the NMEA sentence types that
        clients will receive via
        location_client::startPositionSession(). <br/>

        Without prior calling this API, all NMEA sentences supported
        in the system, as defined in NmeaTypesMask, will get
        generated and delivered to all the location api clients that
        register to receive NMEA sentences. <br/>

        The NMEA sentence types are per-device setting and calling
        this API will impact all the location api clients that
        register to receive NMEA sentences. This API call is not
        incremental and all the settings in the API will completely
        overwrite the previous call. <br/>

        If one or more unspecified bits are set in the NMEA mask,
        those bits will be ignored, but the rest of the
        configuration will get applied. <br/>

        Please note that the configured NMEA sentence types are not
        persistent. Upon reboot of the processor that hosts the
        location hal daemon, if the client process that configures
        the NMEA sentence types resides on the same processor as
        location hal daemon, it is expected that the client process
        to get re-launched and reconfigure the NMEA sentence types
        if the desired NMEA sentence types are different from
        default. If the client process that configures the NMEA
        sentence types resides on a diffrent processor as the
        location hal daemon, upon location hal daemon restarts,
        location hal daemon will receive the configured NMEA
        sentence types again from location integration api library
        running on the client process on the different processor.
        <br/>

        Please note that both output nmea types, datum type and engine
        mask is only applicable if NMEA_PROVIDER in gps.conf is set to
        0 to use HLOS generated NMEA. <br/>

        @param
        enabledNmeaTypes: specify the set of NMEA sentences the
        device will generate and deliver to the location api clients
        that register to receive NMEA sentences. <br/>

        Please note that the configured output nmea types is only
        applicable if NMEA_PROVIDER in gps.conf is set to 0 to use
        HLOS generated NMEA. <br/>

        @param
        nmeaDatumType: specify the geodetic datum type to be used
        when generating NMEA sentences. If this parameter is not
        specified, it will default to WGS-84. <br/>

        @param
        locReqEngineMask: specify the set of position engines that NMEA
        are generated from. If this parameter is not
        specified, it will default to LOC_REQ_ENGINE_FUSED_BIT.
        <br/>


        Please note that the configured nmeaDatumType is only
        applicable if NMEA_PROVIDER in gps.conf is set to 0 to use
        HLOS generated NMEA. NMEA dataum type specified in this API
        will overwrite DATUM_TYPE set in gps.conf. <br/>

        @return true, if the API request has been accepted. The
                status will be returned via configCB. When returning
                true, LocConfigCb() will be invoked to deliver
                asynchronous processing status. <br/>

        @return false, if the API request has not been accepted for
                further processing. <br/>
    */
    bool configOutputNmeaTypes(NmeaTypesMask enabledNmeaTypes,
                               GeodeticDatumType nmeaDatumType = GEODETIC_TYPE_WGS_84,
            location_client::LocReqEngineTypeMask locReqEngineMask =
            location_client::LOC_REQ_ENGINE_FUSED_BIT);

   /** @brief
        This API is used to instruct the specified engine to use
        the provided integrity risk level for protection level
        calculation in position report. This API can be called via
        a position session is in progress.  <br/>

        Prior to calling this API for a particular engine, the
        engine shall not calcualte the protection levels and shall
        not include the protection levels in its position report.
        <br/>

        Currently, only PPE engine will support this function.
        LocConfigCb() will return LOC_INT_RESPONSE_NOT_SUPPORTED
        when request is made to none-PPE engines. <br/>

        Please note that the configured integrity risk level is not
        persistent. Upon reboot of the processor that hosts the
        location hal daemon, if the client process that configures
        the integrity risk level resides on the same processor as
        location hal daemon, it is expected that the client process
        to get re-launched and reconfigure the integrity risk
        level. If the client process that configures the integrity
        risk level resides on a diffrent processor as the location
        hal daemon, upon location hal daemon restarts, location hal
        daemon will receive the configured integrity risk level
        automatically again from location integration api library
        running on the client process on the different processor
        and thus the client process does not need to call this API
        again. <br/>

        @param
        engType: the engine that is instructed to use the specified
        integrity risk level for protection level calculation. The
        protection level will be returned back in
        LocationClientApi::GnssLocation. <br/>

        @param
        integrityRisk: the integrity risk level used for
        calculating protection level in
        LocationClientApi::GnssLocation. <br/>

        The integrity risk is defined as a probability per epoch,
        in unit of 2.5e-10. The valid range for actual integrity is
        [2.5e-10, 1-2.5e-10]), this corresponds to range of [1,
        4e9-1] of this parameter. <br/>

        If the specified value of integrityRisk is NOT in the valid
        range of [1, 4e9-1], the engine shall disable/invalidate
        the protection levels in the position report. <br/>

        @return true, if the API request has been accepted. The
                status will be returned via configCB. When returning
                true, LocConfigCb() will be invoked to deliver
                asynchronous processing status.
                <br/>

        @return false, if the API request has not been accepted for
                further processing. <br/>
    */
    bool configEngineIntegrityRisk(LocIntegrationEngineType engineType,
                                   uint32_t integrityRisk);

    /** @brief
        Inject location <br/>

        The LIA client should only call this API as per defined use
        cases. If the LIA client doesnt follow the defined use case
        and instead calling injectLocation randomly, it may cause
        poor performance of the GNSS engine and break the defined
        use case. <br/>

        Please note that LocConfigCb() will not be invoked. <br/>

        @param location location to be injected.<br/>

        @return true, if the injected location is accepted. <br/>
        @return false, if the injected location is not accepted.
                Please note that the injected location will not be
                accepted if does not have valid
                latitude/longitude/horizontal accuracy or timestamp
                info. <br/>
    */
    bool injectLocation(const location_client::Location& location);

   /** @brief
        This API is used to enable/disable the XTRA (Predicted GNSS
        Satellite Orbit Data) feature on device. If XTRA feature is
        to be enabled, this API is also used to configure the
        various XTRA settings in the device.  <br/>

        Client should wait for the command to finish, e.g.: via
        configCb received before issuing a second configXtraParams
        command. Behavior is not defined if client issues a second
        request of configXtraParams without waiting for the finish of the
        previous configXtraParams request.  <br/>

        Please note that if configXtraParams has never been called
        since device first time bootup, the default behavior will be
        maintained. Also, configXtraParams is not incremental, as a
        successful call of configXtraParams will always overwrite
        the settings in the previous call. In addition, the
        configured xtra parameters will be made persistent. However,
        to be consistent with other location integration API, it is
        recommended to config xtra params using location integration
        API upon every device bootup. <br/>

        @param
        enable: true to enable XTRA feature on the device
                false to disable XTRA feature on the device. When
                setting to false, both XTRA assistance data and NTP
                time download will be disabled.  <br/>

        @param
        configParams:pointer to XtraConfigParams to be used by XTRA
        daemon module when enabling XTRA feature on the device.
        if xtra feature is to be disabled, this parameter should be
        set to NULL. If it is not set to NULL, the parameter will be
        ignored.  <br/>

        @return true, if the request is accepted for further
                processing. When returning true, configCb will be
                invoked to deliver asynchronous processing status.
                If this API is called when XTRA feature is disabled via
                modem NV, the API will return
                LOC_INT_RESPONSE_NOT_SUPPORTED. <br/>

        @return false, if the request is not accepted for further
                processing. When returning false, configCb will not
                be invoked.  <br/>
    */
    bool configXtraParams(bool enable, XtraConfigParams* configParams);


    /** @brief
        Query xtra feature setting and xtra assistance data status
        used by the GNSS standard position engine (SPE). <br/>

        If processing of the command fails, the failure status will
        be returned via LocConfigCb(). If the processing of the command
        is successful, the successful command status will be
        returned via configCB, and xtra setting and xtra assistance
        data status will be returned via LocConfigGetXtraStatusCb()
        that is passed via the Location Integration API constructor.
        If XTRA_DATA_STATUS_UNKNOWN is returned but XTRA feature is
        enabled, the client shall wait a few seconds before calling
        this API again. <br/>.

        @return true, if the API request has been accepted. <br/>

        @return false, if the API request has not been accepted for
                further processing. When returning false, LocConfigCb()
                and LocConfigGetXtraStatusCb() will not be invoked.
                <br/>
    */
    bool getXtraStatus();

    /** @brief
        Register the callback to get update on xtra feature setting
        and xtra assistance data status used by the GNSS standard
        position engine (SPE). The callback to receive the status
        update, e.g.: LocConfigGetXtraStatusCb() shall be
        instantiated and passed via the Location Integration API
        constructor. <br/>

        If processing of the command fails, the failure status will
        be returned via LocConfigCb(). If the processing of the command
        is successful, the command successful status will be
        returned via configCB. The xtra setting and assistance data
        status update will be returned via
        LocConfigGetXtraStatusCb() passed via the constructor. <br/>

        Please see below for some triggers that
        LocConfigGetXtraStatusCb() will be invoked: <br/>
        (1) upon successful registering the API <br/>
        (2) upon xtra feature been enabled/disabled via the
        configXtraParams() <br/>
        (3) upon successful xtra assistance data download
        (4) when XTRA assistance data is downloaded <br/>

        Please note if registerXtraStatusUpdate is called to with
        register setting to true again, the
        LocConfigGetXtraStatusCb() will ve invoked first with
        updateTrigger set to XTRA_STATUS_UPDATE_UPON_REGISTRATION,
        but subsequent update will only happen once per
        enable/disable of the feature and per download. <br/>

        @param
        registerUpdate: true, to register for xtra status update
                        false, to un-register for xtra stauts update
                        <br/>

        @return true, if the API request has been accepted. <br/>

        @return false, if the API request has not been accepted for
                further processing. When returning false, LocConfigCb()
                and LocConfigGetXtraStatusCb() will not be invoked.
                <br/>
    */
    bool registerXtraStatusUpdate(bool registerUpdate);

    /** @brief
        Inject Merkle tree configure buffer which reads from a .xml configure file.
        Configure file contains Merkle Root, Merkle Nodes and information for
        up to 2 public keys. This merkle tree is used by the standard position
        engine (SPE).
        Client should wait for the command to finish, e.g.: via
        LocConfigCb() received before issuing a second configMerkleTree()
        command. Behavior is not defined if client issues a second
        request of configMerkleTree() without waiting for the
        previous configMerkleTree() to finish. <br/>
        Please note that caller should free the merkleTreeXml. <br/>
        @param
        merkleTreeXml: char buffer read from Merkle Tree configure file <br/>

        @param
        xmlSize: the length of char buffer
        @return true, if the API request has been accepted. The
                status will be returned via configCb. When returning
                true, LocConfigCb() will be invoked to deliver
                asynchronous processing status.
                <br/>

        @return false, if the API request has not been accepted for
                further processing. <br/>
    */
    bool configMerkleTree(const char * merkleTreeXml, int xmlSize);

    /** @brief
        API to Enable/Disable OSNMA (Open Source Navigation Message Authentication)
        operation in standard position engine (SPE).
        Client should wait for the command to finish, e.g.: via
        LocConfigCb() received before issuing a second configOsnmaEnablement()
        command. Behavior is not defined if client issues a second
        request of configOsnmaEnablement() without waiting for the
        previous configOsnmaEnablement() to finish. <br/>
        @param
        isEnabled - The flag to indicate enable or disable OSNMA
        @return true, if the API request has been accepted. The
                status will be returned via configCb. When returning
                true, LocConfigCb() will be invoked to deliver
                asynchronous processing status.
                <br/>

        @return false, if the API request has not been accepted for
                further processing. <br/>
    */
    bool configOsnmaEnablement(bool isEnabled);

    /** @brief
        Register the callback to get update on GNSS signal type
        capabilities that modem supports. These capabilities
        represent the supported signal types with
        temporarily disabled signal types excluded,
        like the blocklisted satellites/constellations or
        the constellations disabled by regional restrictions.
        The callback to receive the signal types update,
        e.g.: LocConfigGnssSignalTypesCb() shall be
        instantiated and passed via the Location Integration API
        constructor. <br/>

        If the processing of this command is successful,
        the GNSS signal type capabilities update will be returned
        via LocConfigGnssSignalTypesCb() which is passed via
        the constructor. <br/>

        Please see below for some triggers that
        LocConfigGnssSignalTypesCb() will be invoked: <br/>
        (1) upon successful registering the API <br/>
        (2) upon change in the active GNSS/bands configuration <br/>

        @param
        registerUpdate: true, to register for GNSS signal types update
                        false, to un-register for GNSS signal types update
                        <br/>

        @return true, if the API request has been accepted.
                LocConfigCb() will be invoked to deliver asynchronous
                processing status. The GNSS signal type capabilities will
                be returned via LocConfigGnssSignalTypesCb().
                <br/>

        @return false, if the API request has not been accepted for
                further processing. When returning false,
                LocConfigGnssSignalTypesCb() will not be invoked.
                <br/>

    */
    bool registerGnssSignalTypesUpdate(bool registerUpdate);

    /** @brief
        API to support passing map-matched Feedback data to the DRE engine.
        Clients should use the below API to pass Map-matched data that will
        be used by the DRE engine to compute position reports.<br/>

        @param
        mapMatchedFeedbackData: structure to store MMF data fields
                        <br/>

        @return true, if the API request has been accepted.
                <br/>

        @return false, if the API request has not been accepted for
                further processing. <br/>
    */
    bool injectMapMatchedData (const mapMatchedFeedbackData& mapData);

    /** @example example1:testGetConfigApi
    * <pre>
    * <code>
    *    // Sample Code
    * static void onConfigResponseCb(location_integration::LocConfigTypeEnum requestType,
    *                               location_integration::LocIntegrationResponse response) {
    *     if (response == LOCATION_RESPONSE_SUCCESS) {
    *         // request to retrieve device setting has been accepted
    *         // expect to receive the requested device setting via the registered callback
    *     } else {
    *         // request to retrieve device setting has failed
    *         // no further callback will be delivered
    *     }
    * }
    * static void onGetRobustLocationConfigCb(RobustLocationConfig robustLocationConfig) {
    *     //...
    * }
    * static void onGetMinGpsWeekCb(uint16_t minGpsWeek) {
    *     //...
    * }
    * static void onGetMinSvElevationCb(uint8_t minSvElevation) {
    *     //...
    * }
    * static void onGetSecondaryBandConfigCb(const ConstellationSet& secondaryBandDisablementSet) {
    *     //...
    * }
    * void testGetConfigApi() {
    *   LocIntegrationCbs intCbs;
    *   // Initialzie the callback needed to receive the configuration
    *   intCbs.configCb = LocConfigCb(onConfigResponseCb);
    *   intCbs.getRobustLocationConfigCb =
    *       LocConfigGetRobustLocationConfigCb(onGetRobustLocationConfigCb);
    *   intCbs.getMinGpsWeekCb = LocConfigGetMinGpsWeekCb(onGetMinGpsWeekCb);
    *   intCbs.getMinSvElevationCb = LocConfigGetMinSvElevationCb(onGetMinSvElevationCb);
    *   intCbs.getConstellationSecondaryBandConfigCb =
    *           LocConfigGetConstellationSecondaryBandConfigCb(onGetSecondaryBandConfigCb);
    *   LocConfigPriorityMap priorityMap;
    *   // Create location integration api
    *   pIntClient = new LocationIntegrationApi(priorityMap, intCbs);
    *   bool retVal = false;
    *
    *   // Get robust location config
    *   // If retVal is true, then retrieve the config in the callback
    *   reVal = pIntClient->getRobustLocationConfig();
    *
    *   // Get min gps week
    *   // If retVal is true, then retrieve the config in the callback
    *   reVal = pIntClient->getMinGpsWeek();
    *
    *   // get min sv elevation
    *   // If retVal is true, then retrieve the config in the callback
    *   reVal = pIntClient->getMinSvElevation();
    *
    *   // get constellation config
    *   // If retVal is true, then retrieve the config in the callback
    *   reVal = pIntClient->getConstellationSecondaryBandConfig();
    *   //...
    * }
    **
    * </code>
    * </pre>
    */

   /** @example example2:testSetConfigApi
    * <pre>
    * <code>
    *    // Sample Code
    * static void onConfigResponseCb(location_integration::LocConfigTypeEnum requestType,
    *                               location_integration::LocIntegrationResponse response) {
    *     if (response == LOCATION_RESPONSE_SUCCESS) {
    *         // successfully configured the device for the specified setting
    *     } else {
    *         // failed to configure the device for the specified setting
    *     }
    * }
    * void testSetConfigApi() {
    *   LocIntegrationCbs intCbs;
    *   // Initialzie the callback to receive the processing status
    *   intCbs.configCb = LocConfigCb(onConfigResponseCb);
    *   LocConfigPriorityMap priorityMap;
    *   // Create location integration api
    *   pIntClient = new LocationIntegrationApi(priorityMap, intCbs);
    *
    *   boot retVal;

    *   // Enable TUNC mode with default threadhold and power budget
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->configConstrainedTimeUncertainty(true, 0.0, 0.0);
    *
    *   // Disable TUNC mode
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->configConstrainedTimeUncertainty(false);
    *
    *   // Enable position assisted clock estimator feature
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->configPositionAssistedClockEstimator(true)
    *
    *   // Delete all aiding data
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->deleteAllAidingData();
    *
    *   // Delete ephemeris
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->deleteAidingData(
    *            (AidingDataDeletionMask) AIDING_DATA_DELETION_EPHEMERIS);
    *
    *   // Delete DR sensor calibartion data
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->deleteAidingData(
    *           (AidingDataDeletionMask) AIDING_DATA_DELETION_DR_SENSOR_CALIBRATION);
    *
    *   // Get min gps week
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->getMinGpsWeek();
    *
    *   // restore sv constellation enablement/disablement and blacklisting setting to default
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->configConstellations(nullptr);
    *
    *   LocConfigBlacklistedSvIdList svList;
    *   // disable usage of GLONASS system
    *   svList.push_back(GNSS_CONSTELLATION_TYPE_GLONASS, 0);
    *   // blacklist SBAS SV 120
    *   svList.push_back(GNSS_CONSTELLATION_TYPE_SBAS, 120);
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->configConstellations(&svList);
    *
    *   // restore sv constellation enablement/disablement and blacklisting setting to default
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->configConstellations(nullptr);
    *
    *   // Config constellation secondary band
    *   ConstellationSet secondaryBandDisablementSet;
    *   // Disable secondary band for GLONASS
    *   secondaryBandDisablementSet.emplace(GNSS_CONSTELLATION_TYPE_GLONASS);
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->configConstellationSecondaryBand(secondaryBandDisablementSetPtr);
    *   // Restore the secondary band config to device default
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->configConstellationSecondaryBand(nullptr);
    *
    *   // Configure lever arm info
    *   LeverArmParamsMap leverArmMap;
    *   LeverArmParams leverArm = {};
    *   leverArm.forwardOffsetMeters = 1.0;
    *   leverArm.sidewayOffsetMeters = -0.1;
    *   leverArm.upOffsetMeters = -0.8;
    *   leverArmMap.emplace(LEVER_ARM_TYPE_GNSS_TO_VRP, leverArm);
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->configLeverArm(everArmMap);
    *
    *   // Config robust location
    *   // Enable robust location to be used for none E-911 and E911 GPS session
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->configRobustLocation(true, true);
    *   // Disable robust location to be used for all GPS sessions
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->configRobustLocation(false);
    *
    *   Config min gps week for date correponding to February 4, 2020
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->configMinGpsWeek(2091);
    *
    *   Config min SV elevation of 15 degree
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->configMinGpsWeek(15);
    *
    *   // Config dead reckoning engine, e.g.: botdy to sensor mount parameter
    *   DeadReckoningEngineConfig dreConfig = {};
    *   // Config body to sensor mount parameter
    *   dreConfig.validMask = BODY_TO_SENSOR_MOUNT_PARAMS_VALID;
    *   dreConfig.rollOffset = 60.0;
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   reVal = pIntClient->configDeadReckoningEngineParams(dreConfig);
    *
    *   // Pause DR engine
    *   LocIntegrationEngineType engType = LOC_INT_ENGINE_DRE;
    *   LocIntegrationEngineRunState engState = LOC_INT_ENGINE_RUN_STATE_PAUSE;
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   pIntClient->configEngineRunState(engType, engState);
    *   engState = LOC_INT_ENGINE_RUN_STATE_RESUME;
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   retVal = pIntClient->configEngineRunState(engType, engState);
    *
    *   Set user constent for terrestrial positioning
    *   // User gives consent to use terrestrial positioning
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   retVal = pIntClient->setUserConsentForTerrestrialPositioning(true);
    *   // User does not give consent to use terrestrial positioning
    *   // If retVal is true, then check the processing status in onConfigResponseCb()
    *   retVal = pIntClient->setUserConsentForTerrestrialPositioning(false);
    *   // ...
    * }
    **
    * </code>
    * </pre>
    */

private:
    LocationIntegrationApiImpl* mApiImpl;
};

} // namespace location_integration

#endif /* LOCATION_INTEGRATION_API_H */
