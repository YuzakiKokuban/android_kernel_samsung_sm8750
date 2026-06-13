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

#define LOG_TAG "LocSvc_LocationIntegrationApi"

#include <inttypes.h>
#include <LocationDataTypes.h>
#include <LocationIntegrationApi.h>
#include <LocationIntegrationApiImpl.h>
#ifndef _ANDROID_
#include <LocationIntegrationApiDiagLog.h>
#endif
#include <log_util.h>
#include <loc_pla.h>
namespace location_integration {


/******************************************************************************
LocationIntegrationApi
******************************************************************************/
LocationIntegrationApi::LocationIntegrationApi(
        const LocConfigPriorityMap& priorityMap,
        LocIntegrationCbs& integrationCbs) {

    mApiImpl = new LocationIntegrationApiImpl(integrationCbs);
}

LocationIntegrationApi::~LocationIntegrationApi() {

    LOC_LOGd("calling destructor of LocationIntegrationApi");
    if (mApiImpl) {
        mApiImpl->destroy();
    }
}

bool LocationIntegrationApi::configConstellations(
        const LocConfigBlacklistedSvIdList* blacklistedSvIds){

    if (nullptr == mApiImpl) {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }

    GnssSvTypeConfig constellationEnablementConfig = {};
    GnssSvIdConfig   blacklistSvConfig = {};
    if (nullptr == blacklistedSvIds) {
        // set size field in constellationEnablementConfig to 0 to indicate
        // to restore to modem default
        constellationEnablementConfig.size = 0;
        // all fields in blacklistSvConfig has already been initialized to 0
        blacklistSvConfig.size = sizeof(GnssSvIdConfig);
    } else {
        constellationEnablementConfig.size = sizeof(constellationEnablementConfig);
        constellationEnablementConfig.enabledSvTypesMask =
                GNSS_SV_TYPES_MASK_GPS_BIT|
                GNSS_SV_TYPES_MASK_GLO_BIT|GNSS_SV_TYPES_MASK_BDS_BIT|
                GNSS_SV_TYPES_MASK_QZSS_BIT|GNSS_SV_TYPES_MASK_GAL_BIT;
        blacklistSvConfig.size = sizeof(GnssSvIdConfig);

        for (GnssSvIdInfo it : *blacklistedSvIds) {
            LOC_LOGv("constellation %d, sv id %u", (int) it.constellation, it.svId);
            GnssSvTypesMask svTypeMask = (GnssSvTypesMask) 0;
            uint64_t* svMaskPtr = NULL;
            GnssSvId initialSvId = 0;
            uint16_t svIndexOffset = 0;
            switch (it.constellation) {
            case GNSS_CONSTELLATION_TYPE_GPS:
                svTypeMask = (GnssSvTypesMask) GNSS_SV_TYPES_MASK_GPS_BIT;
                svMaskPtr = &blacklistSvConfig.gpsBlacklistSvMask;
                initialSvId = GNSS_SV_CONFIG_GPS_INITIAL_SV_ID;
                break;
            case GNSS_CONSTELLATION_TYPE_GLONASS:
                svTypeMask = (GnssSvTypesMask) GNSS_SV_TYPES_MASK_GLO_BIT;
                svMaskPtr = &blacklistSvConfig.gloBlacklistSvMask;
                initialSvId = GNSS_SV_CONFIG_GLO_INITIAL_SV_ID;
                break;
            case GNSS_CONSTELLATION_TYPE_QZSS:
                svTypeMask = (GnssSvTypesMask) GNSS_SV_TYPES_MASK_QZSS_BIT;
                svMaskPtr = &blacklistSvConfig.qzssBlacklistSvMask;
                initialSvId = GNSS_SV_CONFIG_QZSS_INITIAL_SV_ID;
                break;
            case GNSS_CONSTELLATION_TYPE_BEIDOU:
                svTypeMask = (GnssSvTypesMask) GNSS_SV_TYPES_MASK_BDS_BIT;
                svMaskPtr = &blacklistSvConfig.bdsBlacklistSvMask;
                initialSvId = GNSS_SV_CONFIG_BDS_INITIAL_SV_ID;
                break;
            case GNSS_CONSTELLATION_TYPE_GALILEO:
                svTypeMask = (GnssSvTypesMask) GNSS_SV_TYPES_MASK_GAL_BIT;
                svMaskPtr = &blacklistSvConfig.galBlacklistSvMask;
                initialSvId = GNSS_SV_CONFIG_GAL_INITIAL_SV_ID;
                break;
            case GNSS_CONSTELLATION_TYPE_SBAS:
                // SBAS does not support enable/disable whole constellation
                // so do not set up svTypeMask for SBAS
                svMaskPtr = &blacklistSvConfig.sbasBlacklistSvMask;
                // SBAS currently has two ranges
                // range of SV id: 183 to 191
                if (GNSS_SV_ID_BLACKLIST_ALL == it.svId) {
                    LOC_LOGd("blacklist all SBAS SV");
                } else if (it.svId >= GNSS_SV_CONFIG_SBAS_INITIAL2_SV_ID) {
                    initialSvId = GNSS_SV_CONFIG_SBAS_INITIAL2_SV_ID;
                    svIndexOffset = GNSS_SV_CONFIG_SBAS_INITIAL_SV_LENGTH;
                } else if ((it.svId >= GNSS_SV_CONFIG_SBAS_INITIAL_SV_ID) &&
                           (it.svId < (GNSS_SV_CONFIG_SBAS_INITIAL_SV_ID +
                                       GNSS_SV_CONFIG_SBAS_INITIAL_SV_LENGTH))){
                    // range of SV id: 120 to 158
                    initialSvId = GNSS_SV_CONFIG_SBAS_INITIAL_SV_ID;
                } else {
                    LOC_LOGe("invalid SBAS sv id %d", it.svId);
                    svMaskPtr = nullptr;
                }
                break;
            case GNSS_CONSTELLATION_TYPE_NAVIC:
                svTypeMask = (GnssSvTypesMask) GNSS_SV_TYPES_MASK_NAVIC_BIT;
                svMaskPtr = &blacklistSvConfig.navicBlacklistSvMask;
                initialSvId = GNSS_SV_CONFIG_NAVIC_INITIAL_SV_ID;
                break;
            default:
                LOC_LOGe("blacklistedSv in constellation %d not supported", it.constellation);
                break;
            }

            if (nullptr == svMaskPtr) {
                LOC_LOGe("Invalid constellation %d", it.constellation);
            } else {
                // SV ID 0 = Blacklist All SVs
                if (GNSS_SV_ID_BLACKLIST_ALL == it.svId) {
                    // blacklist all SVs in this constellation
                    *svMaskPtr = GNSS_SV_CONFIG_ALL_BITS_ENABLED_MASK;
                    constellationEnablementConfig.enabledSvTypesMask &= ~svTypeMask;
                    constellationEnablementConfig.blacklistedSvTypesMask |= svTypeMask;
                } else if (it.svId < initialSvId || it.svId >= initialSvId + 64) {
                    LOC_LOGe ("SV TYPE %d, Invalid sv id %d ", it.constellation, it.svId);
                } else {
                    uint32_t shiftCnt = it.svId + svIndexOffset - initialSvId;
                    *svMaskPtr |= (1ULL << shiftCnt);
                }
            }
        }
    }

    LOC_LOGd("constellation config size=%d, enabledMask=0x%" PRIx64 ", disabledMask=0x%" PRIx64 ", "
             "gps blacklist mask =0x%" PRIx64 ", "
             "glo blacklist mask =0x%" PRIx64 ", qzss blacklist mask =0x%" PRIx64 ", "
             "bds blacklist mask =0x%" PRIx64 ", gal blacklist mask =0x%" PRIx64 ", "
             "sbas blacklist mask =0x%" PRIx64 ", Navic blacklist mask =0x%" PRIx64 ", ",
             constellationEnablementConfig.size, constellationEnablementConfig.enabledSvTypesMask,
             constellationEnablementConfig.blacklistedSvTypesMask,
             blacklistSvConfig.gpsBlacklistSvMask,
             blacklistSvConfig.gloBlacklistSvMask, blacklistSvConfig.qzssBlacklistSvMask,
             blacklistSvConfig.bdsBlacklistSvMask, blacklistSvConfig.galBlacklistSvMask,
             blacklistSvConfig.sbasBlacklistSvMask, blacklistSvConfig.navicBlacklistSvMask);

    mApiImpl->configConstellations(constellationEnablementConfig,
                                   blacklistSvConfig);
    return true;
}

bool LocationIntegrationApi::configConstellationSecondaryBand(
            const ConstellationSet* secondaryBandDisablementSet) {

    GnssSvTypeConfig secondaryBandConfig = {};
    if (nullptr == mApiImpl) {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }

    if (nullptr != secondaryBandDisablementSet) {
        for (GnssConstellationType disabledSecondaryBand : *secondaryBandDisablementSet) {
            LOC_LOGd("to disable secondary band for constellation %d", disabledSecondaryBand);
            secondaryBandConfig.enabledSvTypesMask =
                    (GNSS_SV_TYPES_MASK_GLO_BIT | GNSS_SV_TYPES_MASK_QZSS_BIT|
                     GNSS_SV_TYPES_MASK_BDS_BIT | GNSS_SV_TYPES_MASK_GAL_BIT|
                     GNSS_SV_TYPES_MASK_NAVIC_BIT | GNSS_SV_TYPES_MASK_GPS_BIT);

            switch (disabledSecondaryBand) {
            case GNSS_CONSTELLATION_TYPE_GLONASS:
                secondaryBandConfig.blacklistedSvTypesMask |= GNSS_SV_TYPES_MASK_GLO_BIT;
                break;
            case GNSS_CONSTELLATION_TYPE_QZSS:
                secondaryBandConfig.blacklistedSvTypesMask |= GNSS_SV_TYPES_MASK_QZSS_BIT;
                break;
            case GNSS_CONSTELLATION_TYPE_BEIDOU:
                secondaryBandConfig.blacklistedSvTypesMask |= GNSS_SV_TYPES_MASK_BDS_BIT;
                break;
            case GNSS_CONSTELLATION_TYPE_GALILEO:
                secondaryBandConfig.blacklistedSvTypesMask |= GNSS_SV_TYPES_MASK_GAL_BIT;
                break;
            case GNSS_CONSTELLATION_TYPE_NAVIC:
                secondaryBandConfig.blacklistedSvTypesMask |= GNSS_SV_TYPES_MASK_NAVIC_BIT;
                break;
            case GNSS_CONSTELLATION_TYPE_GPS:
                secondaryBandConfig.blacklistedSvTypesMask |= GNSS_SV_TYPES_MASK_GPS_BIT;
                break;
            default:
                LOC_LOGd("disabled secondary band for constellation %d not suported",
                         disabledSecondaryBand);
                break;
            }
        }
    }

    secondaryBandConfig.size = sizeof (secondaryBandConfig);
    secondaryBandConfig.enabledSvTypesMask =
            (GNSS_SV_TYPES_MASK_GLO_BIT | GNSS_SV_TYPES_MASK_BDS_BIT |
             GNSS_SV_TYPES_MASK_QZSS_BIT | GNSS_SV_TYPES_MASK_GAL_BIT |
             GNSS_SV_TYPES_MASK_NAVIC_BIT | GNSS_SV_TYPES_MASK_GPS_BIT);
    secondaryBandConfig.enabledSvTypesMask ^= secondaryBandConfig.blacklistedSvTypesMask;
    LOC_LOGd("secondary band config size=%d, enableMask=0x%" PRIx64 ", disabledMask=0x%" PRIx64 "",
            secondaryBandConfig.size, secondaryBandConfig.enabledSvTypesMask,
            secondaryBandConfig.blacklistedSvTypesMask);

    mApiImpl->configConstellationSecondaryBand(secondaryBandConfig);
    return true;
}

bool LocationIntegrationApi::getConstellationSecondaryBandConfig() {
    if (mApiImpl) {
        // mApiImpl->getConstellationSecondaryBandConfig returns
        // none-zero when there is no callback registered in the contructor
        return (mApiImpl->getConstellationSecondaryBandConfig() == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::configConstrainedTimeUncertainty(
        bool enable, float tuncThreshold, uint32_t energyBudget) {
    if (mApiImpl) {
        LOC_LOGd("enable %d, tuncThreshold %f, energyBudget %u",
                 enable, tuncThreshold, energyBudget);
        mApiImpl->configConstrainedTimeUncertainty(
                enable, tuncThreshold, energyBudget);
        return true;
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::configPositionAssistedClockEstimator(bool enable) {
    if (mApiImpl) {
        LOC_LOGd("enable %d", enable);
        mApiImpl->configPositionAssistedClockEstimator(enable);
        return true;
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::deleteAllAidingData() {

    if (mApiImpl) {
        GnssAidingData aidingData = {};
        aidingData.deleteAll = true;
        aidingData.posEngineMask = POSITION_ENGINE_MASK_ALL;
        aidingData.sv.svTypeMask = GNSS_AIDING_DATA_SV_TYPE_MASK_ALL;
        aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_EPHEMERIS_BIT;
        aidingData.dreAidingDataMask |= DR_ENGINE_AIDING_DATA_CALIBRATION_BIT;
        mApiImpl->gnssDeleteAidingData(aidingData);
        return true;
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::deleteAidingData(AidingDataDeletionMask aidingDataMask) {
    if (mApiImpl) {
        LOC_LOGd("aiding data mask 0x%x", aidingDataMask);
        GnssAidingData aidingData = {};
        aidingData.deleteAll = false;
        if (aidingDataMask & AIDING_DATA_DELETION_EPHEMERIS) {
            aidingData.sv.svTypeMask = GNSS_AIDING_DATA_SV_TYPE_MASK_ALL;
            aidingData.sv.svMask |= GNSS_AIDING_DATA_SV_EPHEMERIS_BIT;
            aidingData.posEngineMask = POSITION_ENGINE_MASK_ALL;
        }
        if (aidingDataMask & AIDING_DATA_DELETION_DR_SENSOR_CALIBRATION) {
            aidingData.dreAidingDataMask |= DR_ENGINE_AIDING_DATA_CALIBRATION_BIT;
            aidingData.posEngineMask |= DEAD_RECKONING_ENGINE;
        }
        mApiImpl->gnssDeleteAidingData(aidingData);
        return true;
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::configLeverArm(const LeverArmParamsMap& configInfo) {

    if (mApiImpl) {
        LeverArmConfigInfo halLeverArmConfigInfo = {};
        for (auto it = configInfo.begin(); it != configInfo.end(); ++it) {
            ::LeverArmParams* params = nullptr;
            LeverArmTypeMask mask = (LeverArmTypeMask) 0;
            switch (it->first){
            case LEVER_ARM_TYPE_GNSS_TO_VRP:
                mask = LEVER_ARM_TYPE_GNSS_TO_VRP_BIT;
                params = &halLeverArmConfigInfo.gnssToVRP;
                break;
            case LEVER_ARM_TYPE_DR_IMU_TO_GNSS:
                mask = LEVER_ARM_TYPE_DR_IMU_TO_GNSS_BIT;
                params = &halLeverArmConfigInfo.drImuToGnss;
                break;
            case LEVER_ARM_TYPE_VEPP_IMU_TO_GNSS:
                mask = LEVER_ARM_TYPE_VEPP_IMU_TO_GNSS_BIT;
                params = &halLeverArmConfigInfo.veppImuToGnss;
                break;
            default:
                break;
            }
            if (nullptr != params) {
                halLeverArmConfigInfo.leverArmValidMask |= mask;
                params->forwardOffsetMeters = it->second.forwardOffsetMeters;
                params->sidewaysOffsetMeters = it->second.sidewaysOffsetMeters;
                params->upOffsetMeters = it->second.upOffsetMeters;
                LOC_LOGd("mask 0x%x, forward %f, sidways %f, up %f",
                         mask, params->forwardOffsetMeters,
                         params->sidewaysOffsetMeters, params->upOffsetMeters);
            }
        }
        mApiImpl->configLeverArm(halLeverArmConfigInfo);
        return true;
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::configRobustLocation(bool enable, bool enableForE911) {

    if (mApiImpl) {
        LOC_LOGd("enable %d, enableForE911 %d", enable, enableForE911);
        mApiImpl->configRobustLocation(enable, enableForE911);
        return true;
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::getRobustLocationConfig() {
    if (mApiImpl) {
        // mApiImpl->getRobustLocationConfig returns none-zero when
        // there is no callback
        return (mApiImpl->getRobustLocationConfig() == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::configMinGpsWeek(uint16_t minGpsWeek) {
    if (mApiImpl && minGpsWeek != 0) {
        LOC_LOGd("min gps week %u", minGpsWeek);
        return (mApiImpl->configMinGpsWeek(minGpsWeek) == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::getMinGpsWeek() {
    if (mApiImpl) {
        return (mApiImpl->getMinGpsWeek() == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl or callback");
        return false;
    }
}

bool LocationIntegrationApi::configBodyToSensorMountParams(
        const BodyToSensorMountParams& b2sParams) {
    return false;
}

#define FLOAT_EPSILON 0.0000001
bool LocationIntegrationApi::configDeadReckoningEngineParams(
        const DeadReckoningEngineConfig& dreConfig) {

    if (mApiImpl) {
        LOC_LOGi("mask 0x%x, roll offset %f, pitch offset %f, yaw offset %f, offset unc %f, "
                 "vehicleSpeedScaleFactor %f, vehicleSpeedScaleFactorUnc %f, "
                 "gyroScaleFactor %f, gyroScaleFactorUnc %f",
                 dreConfig.validMask,
                 dreConfig.bodyToSensorMountParams.rollOffset,
                 dreConfig.bodyToSensorMountParams.pitchOffset,
                 dreConfig.bodyToSensorMountParams.yawOffset,
                 dreConfig.bodyToSensorMountParams.offsetUnc,
                 dreConfig.vehicleSpeedScaleFactor,
                 dreConfig.vehicleSpeedScaleFactorUnc,
                 dreConfig.gyroScaleFactor, dreConfig.gyroScaleFactorUnc);
        ::DeadReckoningEngineConfig halConfig = {};
        if (dreConfig.validMask & BODY_TO_SENSOR_MOUNT_PARAMS_VALID) {
            if (dreConfig.bodyToSensorMountParams.rollOffset < -180.0 ||
                    dreConfig.bodyToSensorMountParams.rollOffset > 180.0 ||
                    dreConfig.bodyToSensorMountParams.pitchOffset < -180.0 ||
                    dreConfig.bodyToSensorMountParams.pitchOffset > 180.0 ||
                    dreConfig.bodyToSensorMountParams.yawOffset < -180.0 ||
                    dreConfig.bodyToSensorMountParams.yawOffset > 180.0 ||
                    dreConfig.bodyToSensorMountParams.offsetUnc < -180.0 ||
                    dreConfig.bodyToSensorMountParams.offsetUnc > 180.0 ) {
                LOC_LOGe("invalid b2s parameter, range is [-180.0, 180.0]");
                return false;
            }

            halConfig.validMask |= ::BODY_TO_SENSOR_MOUNT_PARAMS_VALID;
            halConfig.bodyToSensorMountParams.rollOffset  =
                    dreConfig.bodyToSensorMountParams.rollOffset;
            halConfig.bodyToSensorMountParams.pitchOffset =
                    dreConfig.bodyToSensorMountParams.pitchOffset;
            halConfig.bodyToSensorMountParams.yawOffset   =
                    dreConfig.bodyToSensorMountParams.yawOffset;
            halConfig.bodyToSensorMountParams.offsetUnc   =
                    dreConfig.bodyToSensorMountParams.offsetUnc;
        }
        if (dreConfig.validMask & VEHICLE_SPEED_SCALE_FACTOR_VALID) {
            if (dreConfig.vehicleSpeedScaleFactor < (0.9 - FLOAT_EPSILON) ||
                    dreConfig.vehicleSpeedScaleFactor > (1.1 + FLOAT_EPSILON)) {
                LOC_LOGe("invalid vehicle speed scale factor, range is [0.9, 1,1]");
                return false;
            }
            halConfig.validMask |= ::VEHICLE_SPEED_SCALE_FACTOR_VALID;
            halConfig.vehicleSpeedScaleFactor = dreConfig.vehicleSpeedScaleFactor;
        }
        if (dreConfig.validMask & VEHICLE_SPEED_SCALE_FACTOR_UNC_VALID) {
            if (dreConfig.vehicleSpeedScaleFactorUnc < 0.0  ||
                    dreConfig.vehicleSpeedScaleFactorUnc > (0.1 + FLOAT_EPSILON)) {
                LOC_LOGe("invalid vehicle speed scale factor uncertainty %10f, range is [0.0, 0.1]",
                         dreConfig.vehicleSpeedScaleFactorUnc);
                return false;
            }
            halConfig.validMask |= ::VEHICLE_SPEED_SCALE_FACTOR_UNC_VALID;
            halConfig.vehicleSpeedScaleFactorUnc = dreConfig.vehicleSpeedScaleFactorUnc;
        }
        if (dreConfig.validMask & GYRO_SCALE_FACTOR_VALID) {
            if (dreConfig.gyroScaleFactor < (0.9 - FLOAT_EPSILON)||
                    dreConfig.gyroScaleFactor > (1.1 + FLOAT_EPSILON)) {
                LOC_LOGe("invalid gyro scale factor %10f, range is [0.9, 1,1]",
                         dreConfig.gyroScaleFactor);
                return false;
            }
            halConfig.validMask |= ::GYRO_SCALE_FACTOR_VALID;
            halConfig.gyroScaleFactor = dreConfig.gyroScaleFactor;
        }
        if (dreConfig.validMask & GYRO_SCALE_FACTOR_UNC_VALID) {
            if (dreConfig.gyroScaleFactorUnc < 0.0 ||
                    dreConfig.gyroScaleFactorUnc > (0.1 + FLOAT_EPSILON)) {
                LOC_LOGe("invalid gyro scale factor uncertainty, range is [0.0, 0.1]");
                return false;
            }
            halConfig.validMask |= ::GYRO_SCALE_FACTOR_UNC_VALID;
            halConfig.gyroScaleFactorUnc = dreConfig.gyroScaleFactorUnc;
        }
        LOC_LOGd("mask 0x%" PRIx64 ", roll offset %f, pitch offset %f, "
                  "yaw offset %f, offset unc %f vehicleSpeedScaleFactor %f, "
                  "vehicleSpeedScaleFactorUnc %f gyroScaleFactor %f gyroScaleFactorUnc %f",
                 halConfig.validMask,
                 halConfig.bodyToSensorMountParams.rollOffset,
                 halConfig.bodyToSensorMountParams.pitchOffset,
                 halConfig.bodyToSensorMountParams.yawOffset,
                 halConfig.bodyToSensorMountParams.offsetUnc,
                 halConfig.vehicleSpeedScaleFactor,
                 halConfig.vehicleSpeedScaleFactorUnc,
                 halConfig.gyroScaleFactor, halConfig.gyroScaleFactorUnc);
        mApiImpl->configDeadReckoningEngineParams(halConfig);
        return true;
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::configMinSvElevation(uint8_t minSvElevation) {
    if (mApiImpl) {
        if (minSvElevation <= 90) {
            LOC_LOGd("min sv elevation %u", minSvElevation);
            GnssConfig gnssConfig = {};
            gnssConfig.flags = GNSS_CONFIG_FLAGS_MIN_SV_ELEVATION_BIT;
            gnssConfig.minSvElevation = minSvElevation;
            mApiImpl->configMinSvElevation(minSvElevation);
            return true;
        } else {
            LOC_LOGe("invalid minSvElevation: %u, valid range is [0, 90]", minSvElevation);
            return false;
        }
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::getMinSvElevation() {
    if (mApiImpl) {
        return (mApiImpl->getMinSvElevation() == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

PositioningEngineMask getHalEngType(LocIntegrationEngineType engType) {
    PositioningEngineMask halEngType = (PositioningEngineMask)0;
    switch (engType) {
        case LOC_INT_ENGINE_SPE:
            halEngType = STANDARD_POSITIONING_ENGINE;
            break;
        case LOC_INT_ENGINE_DRE:
            halEngType = DEAD_RECKONING_ENGINE;
            break;
        case LOC_INT_ENGINE_PPE:
            halEngType = PRECISE_POSITIONING_ENGINE;
            break;
        case LOC_INT_ENGINE_VPE:
            halEngType = VP_POSITIONING_ENGINE;
            break;
        default:
            LOC_LOGe("unknown engine type of %d", engType);
        break;
    }
    return halEngType;
}

bool LocationIntegrationApi::configEngineRunState(LocIntegrationEngineType engType,
                                                  LocIntegrationEngineRunState engState) {
    if (mApiImpl) {
        PositioningEngineMask halEngType = getHalEngType(engType);
        if (halEngType == (PositioningEngineMask) 0) {
            return false;
        }

        LocEngineRunState halEngState = (LocEngineRunState)0;
        if (engState == LOC_INT_ENGINE_RUN_STATE_PAUSE) {
            halEngState = LOC_ENGINE_RUN_STATE_PAUSE;
        } else if (engState == LOC_INT_ENGINE_RUN_STATE_RESUME) {
            halEngState = LOC_ENGINE_RUN_STATE_RESUME;
        } else if (engState == LOC_INT_ENGINE_RUN_STATE_PAUSE_RETAIN) {
            halEngState = LOC_ENGINE_RUN_STATE_PAUSE_RETAIN;
        } else {
            LOC_LOGe("unknown engine state %d", engState);
            return false;
        }
        return (mApiImpl->configEngineRunState(halEngType, halEngState) == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::setUserConsentForTerrestrialPositioning(bool userConsent) {
    if (mApiImpl) {
        return (mApiImpl->setUserConsentForTerrestrialPositioning(userConsent) == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::configOutputNmeaTypes(NmeaTypesMask enabledNMEATypes,
                                                   GeodeticDatumType nmeaDatumType,
                            location_client::LocReqEngineTypeMask locIntEngineMask) {
    if (mApiImpl) {
        uint32_t halNmeaTypes = ::NMEA_TYPE_NONE;
        if (enabledNMEATypes & NMEA_TYPE_GGA) {
            halNmeaTypes |= ::NMEA_TYPE_GGA;
        }
        if (enabledNMEATypes & NMEA_TYPE_RMC) {
            halNmeaTypes |= ::NMEA_TYPE_RMC;
        }
        if (enabledNMEATypes & NMEA_TYPE_GSA) {
            halNmeaTypes |= ::NMEA_TYPE_GSA;
        }
        if (enabledNMEATypes & NMEA_TYPE_VTG) {
            halNmeaTypes |= ::NMEA_TYPE_VTG;
        }
        if (enabledNMEATypes & NMEA_TYPE_GNS) {
            halNmeaTypes |= ::NMEA_TYPE_GNS;
        }
        if (enabledNMEATypes & NMEA_TYPE_DTM) {
            halNmeaTypes |= ::NMEA_TYPE_DTM;
        }
        if (enabledNMEATypes & NMEA_TYPE_GPGSV) {
            halNmeaTypes |= ::NMEA_TYPE_GPGSV;
        }
        if (enabledNMEATypes & NMEA_TYPE_GLGSV) {
            halNmeaTypes |= ::NMEA_TYPE_GLGSV;
        }
        if (enabledNMEATypes & NMEA_TYPE_GAGSV) {
            halNmeaTypes |= ::NMEA_TYPE_GAGSV;
        }
        if (enabledNMEATypes & NMEA_TYPE_GQGSV) {
            halNmeaTypes |= ::NMEA_TYPE_GQGSV;
        }
        if (enabledNMEATypes & NMEA_TYPE_GBGSV) {
            halNmeaTypes |= ::NMEA_TYPE_GBGSV;
        }
        if (enabledNMEATypes & NMEA_TYPE_GIGSV) {
            halNmeaTypes |= ::NMEA_TYPE_GIGSV;
        }
        GnssGeodeticDatumType halDatumType = ::GEODETIC_TYPE_WGS_84;
        if (nmeaDatumType == GEODETIC_TYPE_PZ_90) {
            halDatumType = ::GEODETIC_TYPE_PZ_90;
        }
        uint32_t halEngineMask = ::LOC_REQ_ENGINE_FUSED_BIT;
        if (locIntEngineMask & LOC_REQ_ENGINE_SPE_BIT ) {
            halEngineMask |= ::LOC_REQ_ENGINE_SPE_BIT;
        }
        if (locIntEngineMask & LOC_REQ_ENGINE_PPE_BIT) {
            halEngineMask |= ::LOC_REQ_ENGINE_PPE_BIT;
        }
        LOC_LOGd("datum type 0x%x %d, 0x%x", halNmeaTypes, halDatumType, halEngineMask);
        return (mApiImpl->configOutputNmeaTypes((GnssNmeaTypesMask) halNmeaTypes, halDatumType,
                (LocReqEngineTypeMask)halEngineMask) == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::configEngineIntegrityRisk(
        LocIntegrationEngineType engType, uint32_t integrityRisk) {
    if (mApiImpl) {
        PositioningEngineMask halEngType = getHalEngType(engType);
        if (halEngType == (PositioningEngineMask) 0) {
            return false;
        }
        return (mApiImpl->configEngineIntegrityRisk(halEngType, integrityRisk) == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

// Convert LCA basic loation report to HAL location defined in
// LocationDataType.h
static void convertLocation(const location_client::Location& location,
                            ::Location& halLocation) {

    halLocation = {};
    halLocation.size = sizeof(halLocation);

    uint32_t flags = 0;
    halLocation.timestamp = location.timestamp;
    halLocation.timeUncMs = location.timeUncMs;
    halLocation.latitude = location.latitude;
    halLocation.longitude = location.longitude;
    halLocation.altitude = location.altitude;
    halLocation.speed = location.speed;
    halLocation.bearing = location.bearing;
    halLocation.accuracy = location.horizontalAccuracy;
    halLocation.verticalAccuracy = location.verticalAccuracy;
    halLocation.speedAccuracy = location.speedAccuracy;
    halLocation.bearingAccuracy = location.bearingAccuracy;

    if (location_client::LOCATION_HAS_TIME_UNC_BIT & location.flags) {
        flags |= ::LOCATION_HAS_TIME_UNC_BIT;
    }
    if (location_client::LOCATION_HAS_LAT_LONG_BIT & location.flags) {
        flags |= ::LOCATION_HAS_LAT_LONG_BIT;
    }
    if (location_client::LOCATION_HAS_ALTITUDE_BIT & location.flags) {
        flags |= ::LOCATION_HAS_ALTITUDE_BIT;
    }
    if (location_client::LOCATION_HAS_SPEED_BIT & location.flags) {
        flags |= ::LOCATION_HAS_SPEED_BIT;
    }
    if (location_client::LOCATION_HAS_BEARING_BIT & location.flags) {
        flags |= ::LOCATION_HAS_BEARING_BIT;
    }
    if (location_client::LOCATION_HAS_ACCURACY_BIT & location.flags) {
        flags |= ::LOCATION_HAS_ACCURACY_BIT;
    }
    if (location_client::LOCATION_HAS_VERTICAL_ACCURACY_BIT & location.flags) {
        flags |= ::LOCATION_HAS_VERTICAL_ACCURACY_BIT;
    }
    if (location_client::LOCATION_HAS_SPEED_ACCURACY_BIT & location.flags) {
        flags |= ::LOCATION_HAS_SPEED_ACCURACY_BIT;
    }
    if (location_client::LOCATION_HAS_BEARING_ACCURACY_BIT & location.flags) {
        flags |= ::LOCATION_HAS_BEARING_ACCURACY_BIT;
    }
    halLocation.flags = (::LocationFlagsMask)flags;

    flags = 0;
    if (location_client::LOCATION_TECHNOLOGY_GNSS_BIT & location.techMask) {
        flags |= ::LOCATION_TECHNOLOGY_GNSS_BIT;
    }
    if (location_client::LOCATION_TECHNOLOGY_CELL_BIT & location.techMask) {
        flags |= ::LOCATION_TECHNOLOGY_CELL_BIT;
    }
    if (location_client::LOCATION_TECHNOLOGY_WIFI_BIT & location.techMask) {
        flags |= ::LOCATION_TECHNOLOGY_WIFI_BIT;
    }
    if (location_client::LOCATION_TECHNOLOGY_SENSORS_BIT & location.techMask) {
        flags |= ::LOCATION_TECHNOLOGY_SENSORS_BIT;
    }
    if (location_client::LOCATION_TECHNOLOGY_REFERENCE_LOCATION_BIT & location.techMask) {
        flags |= ::LOCATION_TECHNOLOGY_REFERENCE_LOCATION_BIT;
    }
    if (location_client::LOCATION_TECHNOLOGY_INJECTED_COARSE_POSITION_BIT & location.techMask) {
        flags |= ::LOCATION_TECHNOLOGY_INJECTED_COARSE_POSITION_BIT;
    }
    if (location_client::LOCATION_TECHNOLOGY_AFLT_BIT & location.techMask) {
        flags |= ::LOCATION_TECHNOLOGY_AFLT_BIT;
    }
    if (location_client::LOCATION_TECHNOLOGY_HYBRID_BIT & location.techMask) {
        flags |= ::LOCATION_TECHNOLOGY_HYBRID_BIT;
    }
    if (location_client::LOCATION_TECHNOLOGY_PPE_BIT & location.techMask) {
        flags |= ::LOCATION_TECHNOLOGY_PPE_BIT;
    }
    if (location_client::LOCATION_TECHNOLOGY_VEH_BIT & location.techMask) {
        flags |= ::LOCATION_TECHNOLOGY_VEH_BIT;
    }
    if (location_client::LOCATION_TECHNOLOGY_VIS_BIT & location.techMask) {
        flags |= ::LOCATION_TECHNOLOGY_VIS_BIT;
    }
    halLocation.techMask = (::LocationTechnologyMask)flags;
}

#define VALID_INJECTED_LOCATION_FLAGS \
    (LOCATION_HAS_LAT_LONG_BIT | LOCATION_HAS_ACCURACY_BIT | LOCATION_HAS_TIME_UNC_BIT)

bool LocationIntegrationApi::injectLocation(const location_client::Location& lcaLocation) {
    ::Location halLocation{};

    convertLocation(lcaLocation, halLocation);
    if ((halLocation.flags & VALID_INJECTED_LOCATION_FLAGS) != VALID_INJECTED_LOCATION_FLAGS ||
            (halLocation.timestamp == 0)) {
        LOC_LOGe("location is invalid: flags=0x%x timestamp=%" PRIu64"",
                 lcaLocation.flags, lcaLocation.timestamp);
        return false;
    }
    if (mApiImpl) {
        mApiImpl->odcpiInject(halLocation);
        return true;
    } else {
        LOC_LOGe("NULL mApiImpl");
        return false;
    }
}

::DebugLogLevel getHalLogLevel (DebugLogLevel logLevel) {
    ::DebugLogLevel halLogLevel = ::DEBUG_LOG_LEVEL_NONE;
    switch (logLevel) {
    case DEBUG_LOG_LEVEL_ERROR:
        halLogLevel = ::DEBUG_LOG_LEVEL_ERROR;
        break;
    case DEBUG_LOG_LEVEL_WARNING:
        halLogLevel = ::DEBUG_LOG_LEVEL_WARNING;
        break;
    case DEBUG_LOG_LEVEL_INFO:
        halLogLevel = ::DEBUG_LOG_LEVEL_INFO;
        break;
    case DEBUG_LOG_LEVEL_DEBUG:
        halLogLevel = ::DEBUG_LOG_LEVEL_DEBUG;
        break;
    case DEBUG_LOG_LEVEL_VERBOSE:
        halLogLevel = ::DEBUG_LOG_LEVEL_VERBOSE;
        break;
    default:
        break;
    }
    return halLogLevel;
}

// check valid ntp url
// return, -1 if invalid url, 0 if empty and 1 if valid.
int ntpUrlHasPortNum(const char* ntpURL, int length) {
    if (0 == length) {
        return 0;
    }
    int index = length-1;

    // check for port number at the end of URL
    // ntp url: a.b.c:port, at least five characters before :port
    while (index > 5) {
        char c = ntpURL[index];
        if (c >= '0' && c <= '9') {
            index--;
        } else {
            break;
        }
    };

    if ((index >= 5) && (index < (length-1)) && ntpURL[index] == ':') {
        return 1;
    } else {
        return -1;
    }
}

// sanity check whether server URL is valid or not
// 1: starts with "https://"
// 2: has a port number, like "https://nts.xtracloud.net:4460"
// return, -1 if invalid url, 0 if empty and 1 if valid.
int isValidUrl(string& url) {
    int startPos = -1, endPos = url.length();
    if (0 == endPos) {
        return 0;
    }
    while (++startPos < endPos && isspace(url[startPos]));
    while (--endPos >= 0 && isspace(url[endPos]));
    string trimUrl = url.substr(startPos, endPos - startPos + 1);

    if (trimUrl.length() == 0) {
        LOC_LOGe("invalid input url - only spaces");
        return -1;
    }

    int retval = strncasecmp(trimUrl.c_str(), "https://", sizeof("https://")-1);
    if (retval != 0) {
        LOC_LOGe("url %s does not start with https://", trimUrl.c_str());
        return -1;
    }

    size_t posHost = strlen("https://");
    size_t posPath = trimUrl.length();
    if (posPath == string::npos) {
        LOC_LOGe("invalid url %s", trimUrl.c_str());
        return -1;
    }

    string hostname = trimUrl.substr(posHost, posPath - posHost);
    size_t posPort = hostname.find(':');
    if (posPort == string::npos) {
        LOC_LOGe("invalid url %s contains no port", trimUrl.c_str());
        return -1;
    }
    for (size_t index = posPort+1; index < hostname.size(); index++) {
        char digit = hostname[index];
        if (digit < '0' || digit > '9') {
            LOC_LOGe("url %s contains invalid port", trimUrl.c_str());
            return -1;
        }
    }

    return 1;
}

// sanity check whether XTRA server URL is valid or not
// 1: starts with "https://"
// 2: has a port number "https://path3.xtracloud.net:443/xtra3Mgrcej.bin"
// return, -1 if invalid url, 0 if empty and 1 if valid.
int isValidXtraUrl(string& url) {
    size_t posPath = url.rfind('/');
    string trimUrl = url.substr(0, posPath);
    return isValidUrl(trimUrl);
}

bool LocationIntegrationApi::configXtraParams(bool enable, XtraConfigParams* configParams) {
    bool validparam = false;
    ::XtraConfigParams halConfigParams = {};

    do {
        if (!mApiImpl) {
            break;
        } else if (enable == false) {
            validparam = true;
            break;
        } else if (configParams == nullptr) {
            LOC_LOGe("no valid param provided to enable xtra");
            break;
        }

        LOC_LOGd("xtra params,enable %d, download:%d %d, retry %d %d, debug: %d"
                 "ca path: %s, xtra server: %s %s %s, ntp server %s %s %s, nts ke server %s",
                 enable, configParams->xtraDownloadIntervalMinute,
                 configParams->xtraDownloadTimeoutSec,
                 configParams->xtraDownloadRetryIntervalMinute,
                 configParams->xtraDownloadRetryAttempts,
                 configParams->xtraDaemonDebugLogLevel,
                 configParams->xtraCaPath.c_str(),
                 configParams->xtraServerURLs[0].c_str(), configParams->xtraServerURLs[1].c_str(),
                 configParams->xtraServerURLs[2].c_str(), configParams->ntpServerURLs[0].c_str(),
                 configParams->ntpServerURLs[1].c_str(), configParams->ntpServerURLs[2].c_str(),
                 configParams->ntsKeServerURL.c_str());

        halConfigParams.xtraDownloadIntervalMinute =
                configParams->xtraDownloadIntervalMinute;

        halConfigParams.xtraDownloadTimeoutSec =
                configParams->xtraDownloadTimeoutSec;

        halConfigParams.xtraDownloadRetryIntervalMinute =
                configParams->xtraDownloadRetryIntervalMinute;

        halConfigParams.xtraDownloadRetryAttempts =
                configParams->xtraDownloadRetryAttempts;

        if ((halConfigParams.xtraDownloadRetryIntervalMinute == 0) ||
                (halConfigParams.xtraDownloadRetryAttempts == 0)) {
            halConfigParams.xtraDownloadRetryIntervalMinute = 0;
            halConfigParams.xtraDownloadRetryAttempts = 0;
        }

        // CA path
        strlcpy(halConfigParams.xtraCaPath, configParams->xtraCaPath.c_str(),
                sizeof(halConfigParams.xtraCaPath));

        uint32_t totalValidXtraServerURL = 0;
        for (int index = 0; index < 3; index++) {
            // check for valid server URL
            int xtraUrlValidity = isValidXtraUrl(configParams->xtraServerURLs[index]);
            if (xtraUrlValidity < 0) {
                return false;
            } else if (xtraUrlValidity > 0) {
                strlcpy(halConfigParams.xtraServerURLs[totalValidXtraServerURL++],
                        configParams->xtraServerURLs[index].c_str(),
                        sizeof(halConfigParams.xtraServerURLs[index]));
            }
        }
        halConfigParams.xtraServerURLsCount = totalValidXtraServerURL;

        uint32_t totalValidNtpServerURL = 0;
        for (int index = 0; index < 3; index++) {
            // check for valid server URL
            const char * ntpServerURL = configParams->ntpServerURLs[index].c_str();
            int length = configParams->ntpServerURLs[index].size();
            int ntpUrlValidity = ntpUrlHasPortNum(ntpServerURL, length);
            if (ntpUrlValidity < 0) {
                return false;
            } else if (ntpUrlValidity > 0) {
                strlcpy(halConfigParams.ntpServerURLs[totalValidNtpServerURL++],
                        configParams->ntpServerURLs[index].c_str(),
                        sizeof(halConfigParams.ntpServerURLs[index]));
            }
        }
        halConfigParams.ntpServerURLsCount = totalValidNtpServerURL;

        halConfigParams.xtraIntegrityDownloadEnable =
                configParams->xtraIntegrityDownloadEnable;
        if (halConfigParams.xtraIntegrityDownloadEnable == true) {
            halConfigParams.xtraIntegrityDownloadIntervalMinute =
                    configParams->xtraIntegrityDownloadIntervalMinute;
        }
        // NTS KE Server URL
        int ntsKeValidity = isValidUrl(configParams->ntsKeServerURL);
        if (ntsKeValidity > 0) {
            strlcpy(halConfigParams.ntsKeServerURL, configParams->ntsKeServerURL.c_str(),
                    sizeof(halConfigParams.ntsKeServerURL));
        } else if (ntsKeValidity < 0) {
            break;
        }
        halConfigParams.xtraDaemonDebugLogLevel =
                getHalLogLevel(configParams->xtraDaemonDebugLogLevel);
        halConfigParams.xtraDaemonDiagLoggingStatus =
                configParams->xtraDaemonDiagLoggingStatus;

        validparam = true;
    } while (0);

    if (validparam == true) {
        return (mApiImpl->configXtraParams(enable, halConfigParams) == 0);
    } else {
        return false;
    }
}

bool LocationIntegrationApi::getXtraStatus() {
    if (mApiImpl) {
        return (mApiImpl->getXtraStatus() == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::registerXtraStatusUpdate(bool registerUpdate) {
    if (mApiImpl) {
        return (mApiImpl->registerXtraStatusUpdate(registerUpdate) == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::configMerkleTree(const char * merkleTreeXml, int xmlSize) {
    if (mApiImpl) {
        return (mApiImpl->configMerkleTree(merkleTreeXml, xmlSize) == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::configOsnmaEnablement(bool isEnabled) {
    if (mApiImpl) {
        return (mApiImpl->configOsnmaEnablement(isEnabled) == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::registerGnssSignalTypesUpdate(bool registerUpdate) {
    if (mApiImpl) {
        return (mApiImpl->registerGnssSignalTypesUpdate(registerUpdate) == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}

bool LocationIntegrationApi::injectMapMatchedData(const mapMatchedFeedbackData& mapData) {

    if (mApiImpl) {
        GnssMapMatchedData mmfData = {};
        if (LOC_HAS_VALID_MMFD_UTC_TIME & mapData.validityMask) {
            mmfData.validityMask |= LDT_MMF_DATA_VALID_UTC_TIME;
            mmfData.utcTimestampMs = mapData.utcTimestampMs;
        }
        if (LOC_HAS_VALID_MMFD_LAT_DIFF & mapData.validityMask) {
            mmfData.validityMask |= LDT_MMF_DATA_VALID_LAT_DIFF;
            mmfData.mapMatchedLatitudeDifference = mapData.mapMatchedLatitudeDifference;
        }
        if (LOC_HAS_VALID_MMFD_LONG_DIFF & mapData.validityMask) {
            mmfData.validityMask |= LDT_MMF_DATA_VALID_LONG_DIFF;
            mmfData.mapMatchedLongitudeDifference = mapData.mapMatchedLongitudeDifference;
        }
        if (LOC_HAS_VALID_MMFD_TUNNEL & mapData.validityMask) {
            mmfData.validityMask |= LDT_MMF_DATA_VALID_TUNNEL;
            mmfData.isTunnel = mapData.isTunnel;
        }
        if (LOC_HAS_VALID_MMFD_BEARING & mapData.validityMask) {
            mmfData.validityMask |= LDT_MMF_DATA_VALID_BEARING;
            mmfData.bearing = mapData.bearing;
        }
        if (LOC_HAS_VALID_MMFD_ALTITUDE & mapData.validityMask) {
            mmfData.validityMask |= LDT_MMF_DATA_VALID_ALTITUDE;
            mmfData.altitude = mapData.altitude;
        }
        if (LOC_HAS_VALID_MMFD_HOR_ACC & mapData.validityMask) {
            mmfData.validityMask |= LDT_MMF_DATA_VALID_HOR_ACC;
            mmfData.horizontalAccuracy = mapData.horizontalAccuracy;
        }
        if (LOC_HAS_VALID_MMFD_ALT_ACC & mapData.validityMask) {
            mmfData.validityMask |= LDT_MMF_DATA_VALID_ALT_ACC;
            mmfData.altitudeAccuracy = mapData.altitudeAccuracy;
        }
        if (LOC_HAS_VALID_MMFD_BEARING_ACC & mapData.validityMask) {
            mmfData.validityMask |= LDT_MMF_DATA_VALID_BEARING_ACC;
            mmfData.bearingAccuracy = mapData.bearingAccuracy;
        }

#ifndef _ANDROID_
        LocationIntegrationApiDiagLog  mLogger;
        mLogger.diagLogMmfData(mapData);
#endif

        return (mApiImpl->gnssInjectMmfData(mmfData) == 0);
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }
}
} // namespace location_integration

