/* Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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

#define LOG_TAG "LocSvc_LocationClientApi"

#include <inttypes.h>
#include <loc_cfg.h>
#include <cmath>
#include <LocationDataTypes.h>
#include <LocationClientApi.h>
#include <LocationClientApiImpl.h>
#include <LocationApiMsg.h>
#include <log_util.h>
#include <loc_log.h>
#include <gps_extended_c.h>

using std::string;

namespace location_client {

bool Geofence::operator==(Geofence& other) {
    return mGeofenceImpl != nullptr && other.mGeofenceImpl != nullptr &&
            mGeofenceImpl == other.mGeofenceImpl;
}

class TrackingSessCbHandler {
    public:
        TrackingSessCbHandler(LocationClientApiImpl *pClientApiImpl, ResponseCb rspCb,
                GnssReportCbs gnssReportCbs, uint32_t intervalInMs) {

            memset(&mCallbackOptions, 0, sizeof(LocationCallbacks));
            mCallbackOptions.size =  sizeof(LocationCallbacks);
            if (gnssReportCbs.gnssLocationCallback) {
                mCallbackOptions.gnssLocationInfoCb =
                        [pClientApiImpl, gnssLocCb=gnssReportCbs.gnssLocationCallback]
                        (::GnssLocationInfoNotification n) {
                    GnssLocation gnssLocation =
                            LocationClientApiImpl::parseLocationInfo(n);
                    gnssLocCb(gnssLocation);
                    pClientApiImpl->logLocation(gnssLocation,
                                                LOC_REPORT_TRIGGER_DETAILED_TRACKING_SESSION);
                };
            }

            initializeCommonCbs(pClientApiImpl, rspCb, gnssReportCbs.gnssSvCallback,
                    gnssReportCbs.gnssNmeaCallback,
                    gnssReportCbs.gnssDataCallback,
                    gnssReportCbs.gnssMeasurementsCallback,
                    gnssReportCbs.gnssNHzMeasurementsCallback,
                    gnssReportCbs.gnssDcReportCallback,
                    gnssReportCbs.gnssEphReportCallback,
                    intervalInMs);

            if (gnssReportCbs.nmeaSentencesCallback) {
                mCallbackOptions.gnssNmeaCb =
                        [pClientApiImpl, nmeaSentencesCallback =
                                gnssReportCbs.nmeaSentencesCallback](
                                ::GnssNmeaNotification n) {
                    uint64_t timestamp = n.timestamp;
                    LocOutputEngineType locOutputEngType = (LocOutputEngineType)n.locOutputEngType;
                    std::string nmea(n.nmea);
                    LOC_LOGv("<<< message = nmea[%s]", nmea.c_str());
                    nmeaSentencesCallback(locOutputEngType, timestamp, nmea);
                    pClientApiImpl->getLogger().log(
                            timestamp, nmea.size(), nmea.c_str(), locOutputEngType);
                };
            }
        }

        TrackingSessCbHandler(LocationClientApiImpl *pClientApiImpl, ResponseCb rspCb,
                EngineReportCbs engineReportCbs, uint32_t intervalInMs) {

            memset(&mCallbackOptions, 0, sizeof(LocationCallbacks));
            mCallbackOptions.size =  sizeof(LocationCallbacks);
            if (engineReportCbs.engLocationsCallback) {
                mCallbackOptions.engineLocationsInfoCb =
                        [pClientApiImpl, engineLocCb=engineReportCbs.engLocationsCallback]
                        (uint32_t count,
                        ::GnssLocationInfoNotification* engineLocationInfoNotification) {

                    std::vector<GnssLocation> engLocationsVector;
                    for (int i=0; i< count; i++) {
                        GnssLocation gnssLocation =
                            LocationClientApiImpl::parseLocationInfo(
                                    engineLocationInfoNotification[i]);
                        engLocationsVector.push_back(gnssLocation);
                        pClientApiImpl->logLocation(gnssLocation,
                                                    LOC_REPORT_TRIGGER_ENGINE_TRACKING_SESSION);
                    }
                    engineLocCb(engLocationsVector);
                };
            }
            if (!engineReportCbs.nmeaSentencesCallback && !engineReportCbs.engineNmeaCallback
                    && engineReportCbs.gnssNmeaCallback) {
                initializeCommonCbs(pClientApiImpl, rspCb,
                    engineReportCbs.gnssSvCallback,
                    engineReportCbs.gnssNmeaCallback,
                    engineReportCbs.gnssDataCallback,
                    engineReportCbs.gnssMeasurementsCallback,
                    engineReportCbs.gnssNHzMeasurementsCallback,
                    engineReportCbs.gnssDcReportCallback,
                    engineReportCbs.gnssEphReportCallback,
                    intervalInMs);
            } else {
                initializeCommonCbs(pClientApiImpl, rspCb,
                    engineReportCbs.gnssSvCallback, 0,
                    engineReportCbs.gnssDataCallback,
                    engineReportCbs.gnssMeasurementsCallback,
                    engineReportCbs.gnssNHzMeasurementsCallback,
                    engineReportCbs.gnssDcReportCallback,
                    engineReportCbs.gnssEphReportCallback,
                    intervalInMs);
            }

            if (!engineReportCbs.nmeaSentencesCallback && engineReportCbs.engineNmeaCallback) {
                mCallbackOptions.engineNmeaCb =
                [pClientApiImpl, engineNmeaCallback = engineReportCbs.engineNmeaCallback](
                        const ::GnssNmeaNotification& n) {
                    uint64_t timestamp = n.timestamp;
                    LocOutputEngineType locOutputEngType = (LocOutputEngineType)n.locOutputEngType;
                    std::string nmea(n.nmea);
                    LOC_LOGv("<<< message = nmea[%s] locOutputEngType = %d", nmea.c_str(),
                        locOutputEngType);
                    std::stringstream ss(nmea);
                    std::string each;
                    while (std::getline(ss, each, '\n')) {
                        each += '\n';
                        engineNmeaCallback(locOutputEngType, timestamp, each);
                    }
                    pClientApiImpl->getLogger().log(timestamp, nmea.size(), nmea.c_str(),
                            locOutputEngType);
                };
            } else if (engineReportCbs.nmeaSentencesCallback) {
                mCallbackOptions.engineNmeaCb =
                [pClientApiImpl, nmeaSentencesCallback =
                        engineReportCbs.nmeaSentencesCallback](
                        ::GnssNmeaNotification n) {
                    uint64_t timestamp = n.timestamp;
                    LocOutputEngineType locOutputEngType = (LocOutputEngineType)n.locOutputEngType;
                    std::string nmea(n.nmea);
                    LOC_LOGv("<<< message = nmea[%s] locOutputEngType = %d", nmea.c_str(),
                        locOutputEngType);
                    nmeaSentencesCallback(locOutputEngType, timestamp, nmea);
                    pClientApiImpl->getLogger().log(timestamp, nmea.size(), nmea.c_str(),
                            locOutputEngType);
               };
            }
        }

        LocationCallbacks& getLocationCbs() { return mCallbackOptions; }

    private:
        LocationCallbacks mCallbackOptions;
        void initializeCommonCbs(LocationClientApiImpl *pClientApiImpl,
                                 ResponseCb rspCb,
                                 GnssSvCb gnssSvCallback,
                                 GnssNmeaCb gnssNmeaCallback,
                                 GnssDataCb gnssDataCallback,
                                 GnssMeasurementsCb gnssMeasurementsCallback,
                                 GnssMeasurementsCb gnssNHzMeasurementsCallback,
                                 GnssDcReportCb gnssDcReportCallback,
                                 GnssEphReportCb gnssEphReportCallback,
                                 uint32_t intervalInMs);
};

void TrackingSessCbHandler::initializeCommonCbs(LocationClientApiImpl *pClientApiImpl,
        ResponseCb rspCb, GnssSvCb gnssSvCallback, GnssNmeaCb gnssNmeaCallback,
        GnssDataCb gnssDataCallback, GnssMeasurementsCb gnssMeasurementsCallback,
        GnssMeasurementsCb gnssNHzMeasurementsCallback,
        GnssDcReportCb gnssDcReportCallback, GnssEphReportCb gnssEphReportCallback,
        uint32_t intervalInMs) {
    // callback masks
    if (rspCb) {
        mCallbackOptions.responseCb = [rspCb](::LocationError err, uint32_t id) {
            LocationResponse response = LocationClientApiImpl::parseLocationError(err);
            rspCb(response);
        };
    }

    if (gnssSvCallback) {
        mCallbackOptions.gnssSvCb =
                [pClientApiImpl, gnssSvCallback](::GnssSvNotification n) {
            std::vector<GnssSv> gnssSvsVector;
            for (int i=0; i< n.count; i++) {
                GnssSv gnssSv;
                gnssSv = LocationClientApiImpl::parseGnssSv(n.gnssSvs[i]);
                gnssSvsVector.push_back(gnssSv);
            }
            gnssSvCallback(gnssSvsVector);
            pClientApiImpl->getLogger().log(gnssSvsVector);
        };
    }
    if (gnssNmeaCallback) {
        mCallbackOptions.gnssNmeaCb =
                [pClientApiImpl, gnssNmeaCallback](const ::GnssNmeaNotification& n) {
            uint64_t timestamp = n.timestamp;
            LocOutputEngineType locOutputEngType = (LocOutputEngineType)n.locOutputEngType;
            std::string nmea(n.nmea);
            LOC_LOGv("<<< message = nmea[%s]", nmea.c_str());
            std::stringstream ss(nmea);
            std::string each;
            while (std::getline(ss, each, '\n')) {
                each += '\n';
                gnssNmeaCallback(timestamp, each);
            }
            pClientApiImpl->getLogger().log(timestamp, nmea.size(), nmea.c_str(), locOutputEngType);
        };
    }
    if (gnssDataCallback) {
        mCallbackOptions.gnssDataCb =
                [pClientApiImpl, gnssDataCallback] (const ::GnssDataNotification& n) {
            GnssData gnssData = LocationClientApiImpl::parseGnssData(n);
            gnssDataCallback(gnssData);
            pClientApiImpl->getLogger().log(gnssData);
       };
    }
    if (gnssMeasurementsCallback) {
        mCallbackOptions.gnssMeasurementsCb =
                [pClientApiImpl, gnssMeasurementsCallback](
                    const ::GnssMeasurementsNotification &n) {
            GnssMeasurements gnssMeasurements =
                        LocationClientApiImpl::parseGnssMeasurements(n);
            gnssMeasurementsCallback(gnssMeasurements);
            pClientApiImpl->getLogger().log(gnssMeasurements);
        };
    }
    if (gnssNHzMeasurementsCallback) {
        if (intervalInMs > 100) {
            LOC_LOGe("nHz measurement not supported with TBF of %d", intervalInMs);
        } else {
            mCallbackOptions.gnssNHzMeasurementsCb =
                    [pClientApiImpl, gnssNHzMeasurementsCallback](
                    const ::GnssMeasurementsNotification &n) {
                GnssMeasurements gnssMeasurements =
                        LocationClientApiImpl::parseGnssMeasurements(n);
                gnssNHzMeasurementsCallback(gnssMeasurements);
                pClientApiImpl->getLogger().log(gnssMeasurements);
            };
        }
    }
    if (gnssDcReportCallback) {
        mCallbackOptions.gnssDcReportCb =
                [pClientApiImpl, gnssDcReportCallback](::GnssDcReportInfo n) {
            GnssDcReport gnssDcReport =
                        LocationClientApiImpl::parseDcReport(n);
            gnssDcReportCallback(gnssDcReport);
            pClientApiImpl->getLogger().log(gnssDcReport);
        };
    }

    if (gnssEphReportCallback) {
        mCallbackOptions.svEphemerisCb =
                [pClientApiImpl, gnssEphReportCallback](
                const ::GnssSvEphemerisReport &n) {
            GnssEphemeris gnssEphInfo =
                    LocationClientApiImpl::parseGnssEphemerisInfo(n);
            gnssEphReportCallback(gnssEphInfo);
            pClientApiImpl->getLogger().log(gnssEphInfo);
        };
    }
}

/******************************************************************************
LocationClientApi
******************************************************************************/
LocationClientApi::LocationClientApi(CapabilitiesCb capaCb) {
    capabilitiesCallback capabilitiesCb = nullptr;
    if (capaCb) {
        capabilitiesCb = [capaCb] (LocationCapabilitiesMask capabilitiesMask) {
           LocationCapabilitiesMask capsMask =
                LocationClientApiImpl::parseCapabilitiesMask(capabilitiesMask);
           capaCb(capsMask);
        };
    }
    mApiImpl = new LocationClientApiImpl(capabilitiesCb);
    if (!mApiImpl) {
        LOC_LOGe ("mApiImpl creation failed.");
    }
}

LocationClientApi::~LocationClientApi() {
    if (mApiImpl) {
        // two steps processes due to asynchronous message processing
        mApiImpl->destroy();
        // deletion of mApiImpl will be done after messages in the queue are processed
    }
}

bool LocationClientApi::startPositionSession(
        uint32_t intervalInMs,
        uint32_t distanceInMeters,
        LocationCb locationCallback,
        ResponseCb responseCallback) {

    loc_boot_kpi_marker("L - LCA ST-SPS %s %d",
            getprogname(), intervalInMs);
    //Input parameter check
    if (!locationCallback) {
        LOC_LOGe ("NULL locationCallback");
        return false;
    }

    if (!mApiImpl) {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }

    // callback masks
    LocationCallbacks callbacksOption = {};
    callbacksOption.size =  sizeof(LocationCallbacks);

    if (responseCallback) {
        callbacksOption.responseCb = [responseCallback](::LocationError err, uint32_t id) {
            LocationResponse response = LocationClientApiImpl::parseLocationError(err);
            responseCallback(response);
        };
    }

    callbacksOption.trackingCb = [this, locationCallback](const ::Location& loc) {
        Location location = LocationClientApiImpl::parseLocation(loc);
        locationCallback(location);
        mApiImpl->logLocation(location, LOC_REPORT_TRIGGER_SIMPLE_TRACKING_SESSION);
    };

    // options
    LocationOptions locationOption;
    locationOption.size = sizeof(locationOption);
    locationOption.minInterval = intervalInMs;
    locationOption.minDistance = distanceInMeters;
    locationOption.qualityLevelAccepted = QUALITY_ANY_OR_FAILED_FIX;

    TrackingOptions trackingOption(locationOption);
    mApiImpl->startPositionSession(callbacksOption, trackingOption);
    return true;
}

bool LocationClientApi::startPositionSession(
        uint32_t intervalInMs,
        const GnssReportCbs& gnssReportCallbacks,
        ResponseCb responseCallback) {

    loc_boot_kpi_marker("L - LCA EX-SPS %s %d",
            getprogname(), intervalInMs);

    if (!mApiImpl) {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }

    TrackingSessCbHandler cbHandler(mApiImpl, responseCallback, gnssReportCallbacks,
            intervalInMs);

    // options
    LocationOptions locationOption;
    locationOption.size = sizeof(locationOption);
    locationOption.minInterval = intervalInMs;
    locationOption.minDistance = 0;
    locationOption.qualityLevelAccepted = QUALITY_ANY_OR_FAILED_FIX;

    TrackingOptions trackingOption(locationOption);
    mApiImpl->startPositionSession(cbHandler.getLocationCbs(), trackingOption);
    return true;
}

bool LocationClientApi::startPositionSession(
        uint32_t intervalInMs,
        LocReqEngineTypeMask locEngReqMask,
        const EngineReportCbs& engReportCallbacks,
        ResponseCb responseCallback) {

    loc_boot_kpi_marker("L - LCA Fused-SPS %s %d",
            getprogname(), intervalInMs);
    if (!mApiImpl) {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }

    TrackingSessCbHandler cbHandler(mApiImpl, responseCallback, engReportCallbacks, intervalInMs);

    // options
    LocationOptions locationOption;
    locationOption.size = sizeof(locationOption);
    locationOption.minInterval = intervalInMs;
    locationOption.minDistance = 0;
    locationOption.locReqEngTypeMask =(::LocReqEngineTypeMask)locEngReqMask;
    locationOption.qualityLevelAccepted = QUALITY_ANY_OR_FAILED_FIX;

    TrackingOptions trackingOption(locationOption);
    mApiImpl->startPositionSession(cbHandler.getLocationCbs(), trackingOption);
    return true;
}

void LocationClientApi::stopPositionSession() {
    if (mApiImpl) {
        mApiImpl->stopTrackingAndClearSubscriptions(0);
    }
}

bool LocationClientApi::startTripBatchingSession(uint32_t minInterval, uint32_t tripDistance,
        BatchingCb batchingCb, ResponseCb rspCb) {
    //Input parameter check
    if (!batchingCb) {
        LOC_LOGe ("NULL batchingCallback");
        return false;
    }

    if (!mApiImpl) {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }

    // callback masks
    LocationCallbacks callbacksOption = {};
    callbacksOption.size = sizeof(LocationCallbacks);

    if (rspCb) {
        callbacksOption.responseCb = [rspCb] (::LocationError err, uint32_t id) {
                LocationResponse response = LocationClientApiImpl::parseLocationError(err);
                rspCb(response);
        };
    }

    callbacksOption.batchingCb = [batchingCb] (size_t count, ::Location* location,
            const BatchingOptions& batchingOptions) {
        std::vector<Location> locationVector;
        BatchingStatus status = ((count != 0 )? BATCHING_STATUS_ACTIVE : BATCHING_STATUS_INACTIVE);
        LOC_LOGd("Batch count : %zu", count);
        for (int i=0; i < count; i++) {
            locationVector.push_back(LocationClientApiImpl::parseLocation(
                        location[i]));
        }
        batchingCb(locationVector, status);
    };

    callbacksOption.batchingStatusCb = [batchingCb](const BatchingStatusInfo& batchingSt,
            std::list<uint32_t>& listOfcompletedTrips) {
        if (BATCHING_STATUS_TRIP_COMPLETED == batchingSt.batchingStatus) {
            std::vector<Location> locationVector;
            BatchingStatus status = BATCHING_STATUS_DONE;
            batchingCb(locationVector, status);
        }
    };

    LocationOptions locOption = {};
    locOption.size = sizeof(locOption);
    locOption.minInterval = minInterval;
    locOption.minDistance = tripDistance;
    locOption.mode = GNSS_SUPL_MODE_STANDALONE;

    BatchingOptions     batchOption = {};
    batchOption.size = sizeof(batchOption);
    batchOption.batchingMode = BATCHING_MODE_TRIP;
    batchOption.setLocationOptions(locOption);

    mApiImpl->startBatchingSession(callbacksOption, batchOption);
    return true;
}

bool LocationClientApi::startRoutineBatchingSession(uint32_t minInterval, uint32_t minDistance,
        BatchingCb batchingCb, ResponseCb rspCb) {
    //Input parameter check
    if (!batchingCb) {
        LOC_LOGe ("NULL batchingCallback");
        return false;
    }

    if (!mApiImpl) {
        LOC_LOGe ("NULL mApiImpl");
        return false;
    }

    // callback masks
    LocationCallbacks callbacksOption = {};
    callbacksOption.size =  sizeof(LocationCallbacks);

    if (rspCb) {
        callbacksOption.responseCb = [rspCb](::LocationError err, uint32_t id) {
            LocationResponse response = LocationClientApiImpl::parseLocationError(err);
            rspCb(response);
        };
    }

    callbacksOption.batchingCb = [batchingCb](size_t count, ::Location* location,
            const BatchingOptions& batchingOptions) {
        std::vector<Location> locationVector;
        BatchingStatus status = BATCHING_STATUS_INACTIVE;
        LOC_LOGd("Batch count : %zu", count);
        if (count) {
            for (int i=0; i < count; i++) {
                locationVector.push_back(LocationClientApiImpl::parseLocation(
                            location[i]));
            }
            status = BATCHING_STATUS_ACTIVE;
        }
        batchingCb(locationVector, status);
    };

    LocationOptions locOption = {};
    locOption.size = sizeof(locOption);
    locOption.minInterval = minInterval;
    locOption.minDistance = minDistance;
    locOption.mode = GNSS_SUPL_MODE_STANDALONE;

    BatchingOptions     batchOption = {};
    batchOption.size = sizeof(batchOption);
    batchOption.batchingMode = BATCHING_MODE_ROUTINE;
    batchOption.setLocationOptions(locOption);
    mApiImpl->startBatchingSession(callbacksOption, batchOption);
    return true;
}

void LocationClientApi::stopBatchingSession() {
    if (mApiImpl) {
        mApiImpl->stopBatching(0);
    }
}

void LocationClientApi::addGeofences(std::vector<Geofence>& geofences,
        GeofenceBreachCb gfBreachCb,
        CollectiveResponseCb collRspCb) {
    //Input parameter check
    if (!gfBreachCb) {
        LOC_LOGe ("NULL GeofenceBreachCb");
        return;
    }
    if (!mApiImpl) {
        LOC_LOGe ("NULL mApiImpl");
        return;
    }

    // callback masks
    LocationCallbacks callbacksOption = {};
    callbacksOption.size =  sizeof(LocationCallbacks);

    callbacksOption.responseCb = [](LocationError err, uint32_t id) {};

    if (collRspCb) {
        callbacksOption.collectiveResponseCb = [this, collRspCb](size_t count,
                LocationError* errs, uint32_t* ids) {
                std::vector<pair<Geofence, LocationResponse>> responses;
                LOC_LOGd("CollectiveRes Pload count: %zu", count);
                if (errs != nullptr && ids != nullptr) {
                    for (int i=0; i < count; i++) {
                        responses.push_back(make_pair(
                                    mApiImpl->getMappedGeofence(ids[i]),
                                    LocationClientApiImpl::parseLocationError(errs[i])));
                    }
                    collRspCb(responses);
                }
        };
    }

    callbacksOption.geofenceBreachCb =
            [this, gfBreachCb](const GeofenceBreachNotification& geofenceBreachNotification) {
        std::vector<Geofence> geofences;
        int gfBreachCnt = geofenceBreachNotification.count;
        for (int i=0; i < gfBreachCnt; i++) {
            geofences.push_back(mApiImpl->getMappedGeofence(
                                    geofenceBreachNotification.ids[i]));
        }

        gfBreachCb(geofences,
                LocationClientApiImpl::parseLocation(geofenceBreachNotification.location),
                LocationClientApiImpl::parseGeofenceBreachType(geofenceBreachNotification.type),
                geofenceBreachNotification.timestamp);
    };

    std::vector<Geofence> geofencesToAdd;
    for (int i = 0; i < geofences.size(); ++i) {
        if (!geofences[i].mGeofenceImpl) {
            std::shared_ptr<GeofenceImpl> gfImpl(new GeofenceImpl(&geofences[i]));
            gfImpl->bindGeofence(&geofences[i]);
            geofencesToAdd.emplace_back(geofences[i]);
        }
    }

    if (!geofencesToAdd.size()) {
        LOC_LOGe ("Empty geofences");
        return;
    }
    mApiImpl->addGeofences(callbacksOption, geofencesToAdd);
}
void LocationClientApi::removeGeofences(std::vector<Geofence>& geofences) {
    if (!mApiImpl) {
        LOC_LOGe ("NULL mApiImpl");
        return;
    }
    size_t count = geofences.size();
    if (count > 0) {
        uint32_t* gfIds = (uint32_t*)malloc(sizeof(uint32_t) * count);
        if (nullptr == gfIds) {
            LOC_LOGe("Failed to allocate memory for Geofence Id's");
            return;
        }

        for (int i=0; i<count; ++i) {
            if (!geofences[i].mGeofenceImpl) {
                LOC_LOGe ("Geofence not added yet");
                free(gfIds);
                return;
            }
            gfIds[i] = geofences[i].mGeofenceImpl->getClientId();
            LOC_LOGd("removeGeofences id : %d", gfIds[i]);
        }
        mApiImpl->removeGeofences(count, gfIds);
    }
}
void LocationClientApi::modifyGeofences(std::vector<Geofence>& geofences) {
    if (!mApiImpl) {
        LOC_LOGe ("NULL mApiImpl");
        return;
    }
    size_t count = geofences.size();
    if (count > 0) {
        GeofenceOption* gfOptions = (GeofenceOption*)malloc(sizeof(GeofenceOption) * count);
        if (nullptr == gfOptions) {
            LOC_LOGe("Failed to allocate memory for Geofence Options");
            return;
        }

        uint32_t* gfIds = (uint32_t*)malloc(sizeof(uint32_t) * count);
        if (nullptr == gfIds) {
            LOC_LOGe("Failed to allocate memory for Geofence Id's");
            free(gfOptions);
            return;
        }

        for (int i=0; i<count; ++i) {
            gfOptions[i].breachTypeMask = geofences[i].getBreachType();
            gfOptions[i].responsiveness = geofences[i].getResponsiveness();
            gfOptions[i].dwellTime = geofences[i].getDwellTime();
            gfOptions[i].size = sizeof(gfOptions[i]);
            if (!geofences[i].mGeofenceImpl) {
                LOC_LOGe ("Geofence not added yet");
                free(gfIds);
                free(gfOptions);
                return;
            }
            gfIds[i] = geofences[i].mGeofenceImpl->getClientId();
            LOC_LOGd("modifyGeofences id : %d", gfIds[i]);
        }

        mApiImpl->modifyGeofences(geofences.size(), const_cast<uint32_t*>(gfIds),
                reinterpret_cast<GeofenceOption*>(gfOptions));
    }
}

void LocationClientApi::pauseGeofences(std::vector<Geofence>& geofences) {
    if (!mApiImpl) {
        LOC_LOGe ("NULL mApiImpl");
        return;
    }
    size_t count = geofences.size();
    if (count > 0) {
        uint32_t* gfIds = (uint32_t*)malloc(sizeof(uint32_t) * count);
        if (nullptr == gfIds) {
            LOC_LOGe("Failed to allocate memory for Geofence Id's");
            return;
        }

        for (int i=0; i<count; ++i) {
            if (!geofences[i].mGeofenceImpl) {
                LOC_LOGe ("Geofence not added yet");
                free(gfIds);
                return;
            }
            gfIds[i] = geofences[i].mGeofenceImpl->getClientId();
            LOC_LOGd("pauseGeofences id : %d", gfIds[i]);
        }
        mApiImpl->pauseGeofences(count, gfIds);
    }
}

void LocationClientApi::resumeGeofences(std::vector<Geofence>& geofences) {
    if (!mApiImpl) {
        LOC_LOGe ("NULL mApiImpl");
        return;
    }
    size_t count = geofences.size();
    if (count > 0) {
        uint32_t* gfIds = (uint32_t*)malloc(sizeof(uint32_t) * count);
        if (nullptr == gfIds) {
            LOC_LOGe("Failed to allocate memory for Geofence Id's");
            return;
        }

        for (int i=0; i<count; ++i) {
            if (!geofences[i].mGeofenceImpl) {
                LOC_LOGe ("Geofence not added yet");
                free(gfIds);
                return;
            }
            gfIds[i] = geofences[i].mGeofenceImpl->getClientId();
            LOC_LOGd("resumeGeofences id : %d", gfIds[i]);
        }
        mApiImpl->resumeGeofences(count, gfIds);
    }
}

void LocationClientApi::updateNetworkAvailability(bool available) {
    if (mApiImpl) {
        mApiImpl->updateNetworkAvailability(available);
    }
}

void LocationClientApi::getGnssEnergyConsumed(
        GnssEnergyConsumedCb gnssEnergyConsumedCb,
        ResponseCb responseCb) {

    if (!gnssEnergyConsumedCb) {
        if (responseCb) {
            responseCb(LOCATION_RESPONSE_PARAM_INVALID);
        }
    } else if (mApiImpl) {
        responseCallback responseCbFn = nullptr;

        if (responseCb) {
            responseCbFn = [responseCb](::LocationError err, uint32_t id) {
                LocationResponse response = LocationClientApiImpl::parseLocationError(err);
                responseCb(response);
            };
        }

        gnssEnergyConsumedCallback gnssEnergyConsumedCbFn = [this, gnssEnergyConsumedCb] (
                const ::GnssEnergyConsumedInfo& gnssEnergyConsumed) {
            GnssEnergyConsumedInfo gnssEnergyConsumedInfo =
                    LocationClientApiImpl::parseGnssConsumedInfo(gnssEnergyConsumed);
            gnssEnergyConsumedCb(gnssEnergyConsumedInfo);
        };

        mApiImpl->getGnssEnergyConsumed(gnssEnergyConsumedCbFn, responseCbFn);
    } else {
        LOC_LOGe ("NULL mApiImpl");
    }
}

void LocationClientApi::updateLocationSystemInfoListener(
    LocationSystemInfoCb locSystemInfoCb,
    ResponseCb responseCb) {

    if (mApiImpl) {
        responseCallback responseCbFn = nullptr;
        locationSystemInfoCallback locSystemInfoCbFn = nullptr;

        if (responseCb) {
            responseCbFn = [responseCb](::LocationError err, uint32_t id) {
                LocationResponse response = LocationClientApiImpl::parseLocationError(err);
                responseCb(response);
            };
        }

        if (locSystemInfoCb) {
            locSystemInfoCbFn = [locSystemInfoCb] (::LocationSystemInfo locationSystem) {
                LocationSystemInfo locationSystemInfo =
                        LocationClientApiImpl::parseLocationSystemInfo(locationSystem);
                locSystemInfoCb(locationSystemInfo);
            };
        }

        mApiImpl->updateLocationSystemInfoListener(locSystemInfoCbFn, responseCbFn);
    } else {
        LOC_LOGe ("NULL mApiImpl");
    }
}

uint16_t LocationClientApi::getYearOfHw() {
    if (mApiImpl) {
        return mApiImpl->getYearOfHw();
    } else {
        LOC_LOGe ("NULL mApiImpl");
        return 0;
    }
}

void LocationClientApi::getSingleTerrestrialPosition(
    uint32_t timeoutMsec,
    TerrestrialTechnologyMask techMask,
    float horQoS,
    LocationCb terrestrialPositionCb,
    ResponseCb responseCb) {

    LOC_LOGd("timeout msec = %u, horQoS = %f,"
             "techMask = 0x%x", timeoutMsec, horQoS, techMask);

    // null terrestrialPositionCallback means cancelling request
    if ((terrestrialPositionCb != nullptr) &&
            ((timeoutMsec == 0) || (techMask != TERRESTRIAL_TECH_GTP_WWAN) ||
             (horQoS != 0.0))) {
        LOC_LOGe("invalid parameter: timeout %d msec, tech mask 0x%x, horQoS %f",
                 timeoutMsec, techMask, horQoS);
        if (responseCb) {
            responseCb(LOCATION_RESPONSE_PARAM_INVALID);
        }
    } else if (mApiImpl) {
        responseCallback responseCbFn = nullptr;
        trackingCallback trackingCbFn = nullptr;

        if (responseCb) {
            responseCbFn = [responseCb](::LocationError err, uint32_t id) {
                LocationResponse response = LocationClientApiImpl::parseLocationError(err);
                responseCb(response);
            };
        }

        if (terrestrialPositionCb) {
            trackingCbFn = [this, terrestrialPositionCb](::Location loc) {
                Location location = LocationClientApiImpl::parseLocation(loc);
                terrestrialPositionCb(location);
                mApiImpl->logLocation(location, LOC_REPORT_TRIGGER_SINGLE_TERRESTRIAL_FIX);
            };
        }

        mApiImpl->getSingleTerrestrialPos(timeoutMsec, ::TERRESTRIAL_TECH_GTP_WWAN, horQoS,
                trackingCbFn, responseCbFn);
    } else {
        LOC_LOGe ("NULL mApiImpl");
    }
}

void LocationClientApi::getSinglePosition(uint32_t timeoutMsec,
                                          float horQoS,
                                          LocationCb positionCb,
                                          ResponseCb responseCb) {

    LOC_LOGd("timeout msec = %u, horQoS = %f", timeoutMsec, horQoS);

    if (positionCb != nullptr) {
        if ((timeoutMsec == 0) || (horQoS == 0)) {
            LOC_LOGd("invalid parameter to request single shot fix:"
                     "timeout %d msec, horQoS %f", timeoutMsec, horQoS);
            if (responseCb) {
                responseCb(LOCATION_RESPONSE_PARAM_INVALID);
            }
            return;
        }
    } else {
        LOC_LOGd("null pos cb, cancel the requeset");
        // null positionCallback means cancelling callback
        // set below two parameters to zero when cancelling the request
        timeoutMsec = 0;
        horQoS = 0;
    }

    if (mApiImpl) {
        responseCallback responseCbFn = [responseCb](::LocationError err, uint32_t id) {
            LocationResponse response = LocationClientApiImpl::parseLocationError(err);
            if (responseCb){
                responseCb(response);
            }
        };

        if (positionCb) {
            trackingCallback trackingCbFn = [this, positionCb](::Location loc) {
                Location location = LocationClientApiImpl::parseLocation(loc);
                positionCb(location);

                // log the location
                mApiImpl->logLocation(location, LOC_REPORT_TRIGGER_SINGLE_FIX);
            };
            mApiImpl->getSinglePos(timeoutMsec, horQoS,
                                   trackingCbFn, responseCbFn);
        } else {
            mApiImpl->getSinglePos(timeoutMsec, horQoS,
                                   nullptr, responseCbFn);
        }
    } else {
        LOC_LOGe ("NULL mApiImpl");
    }
}

// ============ below Section implements toString() methods of data structs ==============
static string maskToVals(uint64_t mask, int64_t baseNum) {
    string out;
    while (mask > 0) {
        baseNum += log2(loc_get_least_bit(mask));
        out += baseNum;
        out += " ";
    }
    return out;
}

// LocationCapabilitiesMask
DECLARE_TBL(LocationCapabilitiesMask) = {
    {LOCATION_CAPS_TIME_BASED_TRACKING_BIT, "TIME_BASED_TRACKING"},
    {LOCATION_CAPS_TIME_BASED_BATCHING_BIT, "TIME_BASED_BATCHING"},
    {LOCATION_CAPS_DISTANCE_BASED_TRACKING_BIT, "DIST_BASED_TRACKING"},
    {LOCATION_CAPS_DISTANCE_BASED_BATCHING_BIT, "DIST_BASED_BATCHING"},
    {LOCATION_CAPS_GEOFENCE_BIT, "GEOFENCE"},
    {LOCATION_CAPS_OUTDOOR_TRIP_BATCHING_BIT, "OUTDOOR_TRIP_BATCHING"},
    {LOCATION_CAPS_GNSS_MEASUREMENTS_BIT, "GNSS_MEASUREMENTS"},
    {LOCATION_CAPS_CONSTELLATION_ENABLEMENT_BIT, "CONSTELLATION_ENABLE"},
    {LOCATION_CAPS_CARRIER_PHASE_BIT, "CARRIER_PHASE"},
    {LOCATION_CAPS_SV_POLYNOMIAL_BIT, "SV_POLY"},
    {LOCATION_CAPS_QWES_GNSS_SINGLE_FREQUENCY, "GNSS_SINGLE_FREQ"},
    {LOCATION_CAPS_QWES_GNSS_MULTI_FREQUENCY, "GNSS_MULTI_FREQ"},
    {LOCATION_CAPS_QWES_VPE, "VPE"},
    {LOCATION_CAPS_QWES_CV2X_LOCATION_BASIC, "CV2X_LOC_BASIC"},
    {LOCATION_CAPS_QWES_CV2X_LOCATION_PREMIUM, "CV2X_LOC_PREMIUM"},
    {LOCATION_CAPS_QWES_PPE, "PPE"},
    {LOCATION_CAPS_QWES_QDR2, "QDR2"},
    {LOCATION_CAPS_QWES_QDR3, "QDR3"},
    {LOCATION_CAPS_NLOS_ML20, "NLOS_ML20"},
};
// GnssSvOptionsMask
DECLARE_TBL(GnssSvOptionsMask) = {
    {GNSS_SV_OPTIONS_HAS_EPHEMER_BIT, "EPH"},
    {GNSS_SV_OPTIONS_HAS_ALMANAC_BIT, "ALM"},
    {GNSS_SV_OPTIONS_USED_IN_FIX_BIT, "USED_IN_FIX"},
    {GNSS_SV_OPTIONS_HAS_CARRIER_FREQUENCY_BIT, "CARRIER_FREQ"},
    {GNSS_SV_OPTIONS_HAS_GNSS_SIGNAL_TYPE_BIT, "SIG_TYPES"},
    {GNSS_SV_OPTIONS_HAS_BASEBAND_CARRIER_TO_NOISE_BIT, "BASEBAND_CARRIER_TO_NOISE"},
    {GNSS_SV_OPTIONS_HAS_ELEVATION_BIT, "ELEVATION"},
    {GNSS_SV_OPTIONS_HAS_AZIMUTH_BIT,   "AZIMUTH"},
};
// LocationFlagsMask
DECLARE_TBL(LocationFlagsMask) = {
    {LOCATION_HAS_LAT_LONG_BIT, "LAT_LON"},
    {LOCATION_HAS_ALTITUDE_BIT, "ALT"},
    {LOCATION_HAS_SPEED_BIT, "SPEED"},
    {LOCATION_HAS_BEARING_BIT, "FEARING"},
    {LOCATION_HAS_ACCURACY_BIT, "ACCURACY"},
    {LOCATION_HAS_VERTICAL_ACCURACY_BIT, "VERT_ACCURACY"},
    {LOCATION_HAS_SPEED_ACCURACY_BIT, "SPEED_ACCURACY"},
    {LOCATION_HAS_BEARING_ACCURACY_BIT, "BEARING_ACCURACY"},
    {LOCATION_HAS_TIMESTAMP_BIT, "TS"},
    {LOCATION_HAS_ELAPSED_REAL_TIME_BIT, "ELAPSED_REAL_TIME"},
    {LOCATION_HAS_ELAPSED_REAL_TIME_UNC_BIT, "ELAPSED_REAL_TIME_UNC"},
    {LOCATION_HAS_TIME_UNC_BIT, "TIME_UNC"},
    {LOCATION_HAS_GPTP_TIME_BIT, "GPTP_TIME"},
    {LOCATION_HAS_GPTP_TIME_UNC_BIT, "GPTP_TIME_UNC"},
    {LOCATION_HAS_SESSION_STATUS_BIT, "SESSION_STATUS"},
};
// LocationTechnologyMask
DECLARE_TBL(LocationTechnologyMask) = {
    {LOCATION_TECHNOLOGY_GNSS_BIT, "GNSS"},
    {LOCATION_TECHNOLOGY_CELL_BIT, "CELL"},
    {LOCATION_TECHNOLOGY_WIFI_BIT, "WIFI"},
    {LOCATION_TECHNOLOGY_SENSORS_BIT, "SENSOR"},
    {LOCATION_TECHNOLOGY_REFERENCE_LOCATION_BIT, "REF_LOC"},
    {LOCATION_TECHNOLOGY_INJECTED_COARSE_POSITION_BIT, "CPI"},
    {LOCATION_TECHNOLOGY_AFLT_BIT, "AFLT"},
    {LOCATION_TECHNOLOGY_HYBRID_BIT, "HYBRID"},
    {LOCATION_TECHNOLOGY_PPE_BIT, "PPE"},
    {LOCATION_TECHNOLOGY_VEH_BIT, "VEH"},
    {LOCATION_TECHNOLOGY_VIS_BIT, "VIS"},
    {LOCATION_TECHNOLOGY_PROPAGATED_BIT, "PROPAGATED"}
};
// GnssLocationNavSolutionMask
DECLARE_TBL(GnssLocationNavSolutionMask) = {
    {LOCATION_SBAS_CORRECTION_IONO_BIT, "SBAS_CORR_IONO"},
    {LOCATION_SBAS_CORRECTION_FAST_BIT, "SBAS_CORR_FAST"},
    {LOCATION_SBAS_CORRECTION_LONG_BIT, "SBAS_CORR_LON"},
    {LOCATION_SBAS_INTEGRITY_BIT, "SBAS_INTEGRITY"},
    {LOCATION_NAV_CORRECTION_DGNSS_BIT, "NAV_CORR_DGNSS"},
    {LOCATION_NAV_CORRECTION_RTK_BIT, "NAV_CORR_RTK"},
    {LOCATION_NAV_CORRECTION_PPP_BIT, "NAV_CORR_PPP"},
    {LOCATION_NAV_CORRECTION_RTK_FIXED_BIT, "NAV_CORR_RTK_FIXED"},
    {LOCATION_NAV_CORRECTION_ONLY_SBAS_CORRECTED_SV_USED_BIT,
            "NAV_CORR_ONLY_SBAS_CORRECTED_SV_USED"}
};
// GnssLocationPosDataMask
DECLARE_TBL(GnssLocationPosDataMask) = {
    {LOCATION_NAV_DATA_HAS_LONG_ACCEL_BIT, "LONG_ACCEL"},
    {LOCATION_NAV_DATA_HAS_LAT_ACCEL_BIT, "LAT_ACCEL"},
    {LOCATION_NAV_DATA_HAS_VERT_ACCEL_BIT, "VERT_ACCEL"},
    {LOCATION_NAV_DATA_HAS_YAW_RATE_BIT, "YAW_RATE"},
    {LOCATION_NAV_DATA_HAS_PITCH_BIT, "PITCH"},
    {LOCATION_NAV_DATA_HAS_LONG_ACCEL_UNC_BIT, "LONG_ACCEL_UNC"},
    {LOCATION_NAV_DATA_HAS_LAT_ACCEL_UNC_BIT, "LAT_ACCEL_UNC"},
    {LOCATION_NAV_DATA_HAS_VERT_ACCEL_UNC_BIT, "VERT_ACCEL_UNC"},
    {LOCATION_NAV_DATA_HAS_YAW_RATE_UNC_BIT, "YAW_RATE_UNC"},
    {LOCATION_NAV_DATA_HAS_PITCH_UNC_BIT, "PITCH_UNC"}
};
// GnssSignalTypeMask
DECLARE_TBL(GnssSignalTypeMask) = {
    {GNSS_SIGNAL_GPS_L1CA_BIT, "GPS_L1CA"},
    {GNSS_SIGNAL_GPS_L1C_BIT, "GPS_L1C"},
    {GNSS_SIGNAL_GPS_L2_BIT, "GPS_L2"},
    {GNSS_SIGNAL_GPS_L5_BIT, "GPS_L5"},
    {GNSS_SIGNAL_GLONASS_G1_BIT, "GLO_G1"},
    {GNSS_SIGNAL_GLONASS_G2_BIT, "GLO_G2"},
    {GNSS_SIGNAL_GALILEO_E1_BIT, "GAL_E1"},
    {GNSS_SIGNAL_GALILEO_E5A_BIT, "GAL_E5A"},
    {GNSS_SIGNAL_GALILEO_E5B_BIT, "GAL_E5B"},
    {GNSS_SIGNAL_BEIDOU_B1_BIT, "BDS_B1"},
    {GNSS_SIGNAL_BEIDOU_B2_BIT, "BDS_B2"},
    {GNSS_SIGNAL_QZSS_L1CA_BIT, "QZSS_L1CA"},
    {GNSS_SIGNAL_QZSS_L1S_BIT, "QZSS_L1S"},
    {GNSS_SIGNAL_QZSS_L2_BIT, "QZSS_L2"},
    {GNSS_SIGNAL_QZSS_L2_BIT, "QZSS_L5"},
    {GNSS_SIGNAL_SBAS_L1_BIT, "SBAS_L1"},
    {GNSS_SIGNAL_BEIDOU_B1I_BIT, "BDS_B1I"},
    {GNSS_SIGNAL_BEIDOU_B1C_BIT, "BDS_B1C"},
    {GNSS_SIGNAL_BEIDOU_B2I_BIT, "BDS_B2I"},
    {GNSS_SIGNAL_BEIDOU_B2AI_BIT, "BDS_B2AI"},
    {GNSS_SIGNAL_NAVIC_L5_BIT, "NAVIC_L5"},
    {GNSS_SIGNAL_BEIDOU_B2AQ_BIT, "BDS_B2AQ"},
    {GNSS_SIGNAL_BEIDOU_B2BI_BIT, "BDS_B2BI"},
    {GNSS_SIGNAL_BEIDOU_B2BQ_BIT, "BDS_B2BQ"}
};
// GnssSignalTypes
DECLARE_TBL(GnssSignalTypes) = {
    {GNSS_SIGNAL_TYPE_GPS_L1CA, "GPS_L1CA"},
    {GNSS_SIGNAL_TYPE_GPS_L1C, "GPS_L1C"},
    {GNSS_SIGNAL_TYPE_GPS_L2C_L, "GPS_L2_L"},
    {GNSS_SIGNAL_TYPE_GPS_L5_Q, "GPS_L5_Q"},
    {GNSS_SIGNAL_TYPE_GLONASS_G1, "GLO_G1"},
    {GNSS_SIGNAL_TYPE_GLONASS_G2, "GLO_G2"},
    {GNSS_SIGNAL_TYPE_GALILEO_E1_C, "GAL_E1_C"},
    {GNSS_SIGNAL_TYPE_GALILEO_E5A_Q, "GAL_E5A_Q"},
    {GNSS_SIGNAL_TYPE_GALILEO_E5B_Q, "GAL_E5B_Q"},
    {GNSS_SIGNAL_TYPE_BEIDOU_B1_I, "BDS_B1_I"},
    {GNSS_SIGNAL_TYPE_BEIDOU_B1C, "BDS_B1C"},
    {GNSS_SIGNAL_TYPE_BEIDOU_B1C, "BDS_B2I"},
    {GNSS_SIGNAL_TYPE_BEIDOU_B2A_I, "BDS_B2AI"},
    {GNSS_SIGNAL_TYPE_QZSS_L1CA, "QZSS_L1CA"},
    {GNSS_SIGNAL_TYPE_QZSS_L1S, "QZSS_L1S"},
    {GNSS_SIGNAL_TYPE_QZSS_L2C_L, "QZSS_L2C_L"},
    {GNSS_SIGNAL_TYPE_QZSS_L5_Q, "QZSS_L5"},
    {GNSS_SIGNAL_TYPE_SBAS_L1_CA, "SBAS_L1_CA"},
    {GNSS_SIGNAL_TYPE_NAVIC_L5, "NAVIC_L5"},
    {GNSS_SIGNAL_TYPE_BEIDOU_B2A_Q, "BDS_B2AQ"},
    {GNSS_SIGNAL_TYPE_BEIDOU_B2B_I, "BDS_B2BI"},
    {GNSS_SIGNAL_TYPE_BEIDOU_B2B_Q, "BDS_B2BQ"},
    {GNSS_SIGNAL_TYPE_NAVIC_L1, "NAVIC_L1"}
};
// GnssSvType
DECLARE_TBL(GnssSvType) = {
    {GNSS_SV_TYPE_UNKNOWN, "UNKNOWN"},
    {GNSS_SV_TYPE_GPS, "GPS"},
    {GNSS_SV_TYPE_SBAS, "SBAS"},
    {GNSS_SV_TYPE_GLONASS, "GLO"},
    {GNSS_SV_TYPE_QZSS, "QZSS"},
    {GNSS_SV_TYPE_BEIDOU, "BDS"},
    {GNSS_SV_TYPE_GALILEO, "GAL"},
    {GNSS_SV_TYPE_NAVIC, "NAVIC"}
};
// Gnss_LocSvSystemEnumType
DECLARE_TBL(Gnss_LocSvSystemEnumType) = {
    {GNSS_LOC_SV_SYSTEM_GPS,     "GPS"},
    {GNSS_LOC_SV_SYSTEM_GALILEO, "GAL"},
    {GNSS_LOC_SV_SYSTEM_SBAS,    "SBAS"},
    {GNSS_LOC_SV_SYSTEM_GLONASS, "GLO"},
    {GNSS_LOC_SV_SYSTEM_BDS,     "BDS"},
    {GNSS_LOC_SV_SYSTEM_QZSS,    "QZSS"},
    {GNSS_LOC_SV_SYSTEM_NAVIC,   "NAVIC"}
};
// LCAGnssLocationInfoFlagMask
DECLARE_TBL(LCAGnssLocationInfoFlagMask) = {
    {LCA_GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT, "ALT_SEA_LEVEL"},
    {LCA_GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT, "DOP"},
    {LCA_GNSS_LOCATION_INFO_MAGNETIC_DEVIATION_BIT, "MAG_DEV"},
    {LCA_GNSS_LOCATION_INFO_HOR_RELIABILITY_BIT, "HOR_RELIAB"},
    {LCA_GNSS_LOCATION_INFO_VER_RELIABILITY_BIT, "VER_RELIAB"},
    {LCA_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MAJOR_BIT, "HOR_ACCU_ELIP_SEMI_MAJOR"},
    {LCA_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MINOR_BIT, "HOR_ACCU_ELIP_SEMI_MINOR"},
    {LCA_GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_AZIMUTH_BIT, "HOR_ACCU_ELIP_AZIMUTH"},
    {LCA_GNSS_LOCATION_INFO_GNSS_SV_USED_DATA_BIT, "GNSS_SV_USED"},
    {LCA_GNSS_LOCATION_INFO_NAV_SOLUTION_MASK_BIT, "NAV_SOLUTION"},
    {LCA_GNSS_LOCATION_INFO_POS_TECH_MASK_BIT, "POS_TECH"},
    {LCA_GNSS_LOCATION_INFO_SV_SOURCE_INFO_BIT, "SV_SOURCE"},
    {LCA_GNSS_LOCATION_INFO_POS_DYNAMICS_DATA_BIT, "POS_DYNAMICS"},
    {LCA_GNSS_LOCATION_INFO_EXT_DOP_BIT, "EXT_DOP"},
    {LCA_GNSS_LOCATION_INFO_NORTH_STD_DEV_BIT, "NORTH_STD_DEV"},
    {LCA_GNSS_LOCATION_INFO_EAST_STD_DEV_BIT, "EAST_STD_DEV"},
    {LCA_GNSS_LOCATION_INFO_EAST_STD_DEV_BIT, "NORTH_VEL"},
    {LCA_GNSS_LOCATION_INFO_EAST_VEL_BIT, "EAST_VEL"},
    {LCA_GNSS_LOCATION_INFO_UP_VEL_BIT, "UP_VEL"},
    {LCA_GNSS_LOCATION_INFO_NORTH_VEL_UNC_BIT, "NORTH_VEL_UNC"},
    {LCA_GNSS_LOCATION_INFO_EAST_VEL_UNC_BIT, "EAST_VEL_UNC"},
    {LCA_GNSS_LOCATION_INFO_UP_VEL_UNC_BIT, "UP_VEL_UNC"},
    {LCA_GNSS_LOCATION_INFO_LEAP_SECONDS_BIT, "LEAP_SECONDS"},
    {LCA_GNSS_LOCATION_INFO_TIME_UNC_BIT, "TIME_UNC"},
    {LCA_GNSS_LOCATION_INFO_NUM_SV_USED_IN_POSITION_BIT, "NUM_SV_USED_IN_FIX"},
    {LCA_GNSS_LOCATION_INFO_CALIBRATION_CONFIDENCE_PERCENT_BIT, "CAL_CONF_PRECENT"},
    {LCA_GNSS_LOCATION_INFO_CALIBRATION_STATUS_BIT, "CAL_STATUS"},
    {LCA_GNSS_LOCATION_INFO_OUTPUT_ENG_TYPE_BIT, "OUTPUT_ENG_TYPE"},
    {LCA_GNSS_LOCATION_INFO_OUTPUT_ENG_MASK_BIT, "OUTPUT_ENG_MASK"},
    {LCA_GNSS_LOCATION_INFO_CONFORMITY_INDEX_BIT, "CONFORMITY_INDEX"},
    {LCA_GNSS_LOCATION_INFO_LLA_VRP_BASED_BIT, "LLA_VRP_BASED"},
    {LCA_GNSS_LOCATION_INFO_ENU_VELOCITY_VRP_BASED_BIT, "ENU_VELOCITY_VRP_BASED"},
    {LCA_GNSS_LOCATION_INFO_DR_SOLUTION_STATUS_MASK_BIT, "DR_SOLUTION_STATUS_MASK"},
    {LCA_GNSS_LOCATION_INFO_ALTITUDE_ASSUMED_BIT, "ALTITUDE_ASSUMED"},
    {LCA_GNSS_LOCATION_INFO_SESSION_STATUS_BIT, "SESSION_STATUS"},
    {LCA_GNSS_LOCATION_INFO_INTEGRITY_RISK_USED_BIT, "INTEGRITY_RISK_USED"},
    {LCA_GNSS_LOCATION_INFO_PROTECT_ALONG_TRACK_BIT, "PROTECT_ALONG_TRACK"},
    {LCA_GNSS_LOCATION_INFO_PROTECT_CROSS_TRACK_BIT, "PROTECT_CROSS_TRACK"},
    {LCA_GNSS_LOCATION_INFO_PROTECT_VERTICAL_BIT, "PROTECT_VERTICA"},
    {LCA_GNSS_LOCATION_INFO_DGNSS_STATION_ID_BIT, "DGNSS_STATION_ID"}
};
// LocationReliability
DECLARE_TBL(LocationReliability) = {
    {LOCATION_RELIABILITY_NOT_SET, "NOT_SET"},
    {LOCATION_RELIABILITY_VERY_LOW, "VERY_LOW"},
    {LOCATION_RELIABILITY_LOW, "LOW"},
    {LOCATION_RELIABILITY_MEDIUM, "MED"},
    {LOCATION_RELIABILITY_HIGH, "HI"}
};
// GnssSystemTimeStructTypeFlags
DECLARE_TBL(GnssSystemTimeStructTypeFlags) = {
    {GNSS_SYSTEM_TIME_WEEK_VALID, "WEEK"},
    {GNSS_SYSTEM_TIME_WEEK_MS_VALID, "WEEK_MS"},
    {GNSS_SYSTEM_CLK_TIME_BIAS_VALID, "CLK_BIAS"},
    {GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID, "CLK_BIAS_UNC"},
    {GNSS_SYSTEM_REF_FCOUNT_VALID, "REF_COUNT"},
    {GNSS_SYSTEM_NUM_CLOCK_RESETS_VALID, "CLK_RESET"}
};
// GnssGloTimeStructTypeFlags
DECLARE_TBL(GnssGloTimeStructTypeFlags) = {
    {GNSS_CLO_DAYS_VALID, "DAYS"},
    {GNSS_GLO_MSEC_VALID, "MS"},
    {GNSS_GLO_CLK_TIME_BIAS_VALID, "CLK_BIAS"},
    {GNSS_GLO_CLK_TIME_BIAS_UNC_VALID, "CLK_BIAS_UNC"},
    {GNSS_GLO_REF_FCOUNT_VALID, "REF_COUNT"},
    {GNSS_GLO_NUM_CLOCK_RESETS_VALID, "CLK_RESET"},
    {GNSS_GLO_FOUR_YEAR_VALID, "YEAR"}
};
// DrCalibrationStatusMask
DECLARE_TBL(DrCalibrationStatusMask) = {
    {DR_ROLL_CALIBRATION_NEEDED, "ROLL"},
    {DR_PITCH_CALIBRATION_NEEDED, "PITCH"},
    {DR_YAW_CALIBRATION_NEEDED, "YAW"},
    {DR_ODO_CALIBRATION_NEEDED, "ODO"},
    {DR_GYRO_CALIBRATION_NEEDED, "GYRO"},
    {DR_TURN_CALIBRATION_LOW, "TURN_LOW"},
    {DR_TURN_CALIBRATION_MEDIUM, "TURN_MEDIUM"},
    {DR_TURN_CALIBRATION_HIGH, "TURN_HIGH"},
    {DR_LINEAR_ACCEL_CALIBRATION_LOW, "LINEAR_ACCEL_LOW"},
    {DR_LINEAR_ACCEL_CALIBRATION_MEDIUM, "LINEAR_ACCEL_MEDIUM"},
    {DR_LINEAR_ACCEL_CALIBRATION_HIGH, "LINEAR_ACCEL_HIGH"},
    {DR_LINEAR_MOTION_CALIBRATION_LOW, "LINEAR_MOTION_LOW"},
    {DR_LINEAR_MOTION_CALIBRATION_MEDIUM, "LINEAR_MOTION_MEDIUM"},
    {DR_LINEAR_MOTION_CALIBRATION_HIGH, "LINEAR_MOTION_HIGH"},
    {DR_STATIC_CALIBRATION_LOW, "STATIC_LOW"},
    {DR_STATIC_CALIBRATION_MEDIUM, "STATIC_MEDIUM"},
    {DR_STATIC_CALIBRATION_HIGH, "STATIC_HIGH"}
};
// LocReqEngineTypeMask
DECLARE_TBL(LocReqEngineTypeMask) = {
    {LOC_REQ_ENGINE_FUSED_BIT, "FUSED"},
    {LOC_REQ_ENGINE_SPE_BIT, "SPE"},
    {LOC_REQ_ENGINE_PPE_BIT, "PPE"},
    {LOC_REQ_ENGINE_VPE_BIT, "VPE"}
};
// LocOutputEngineType
DECLARE_TBL(LocOutputEngineType) = {
    {LOC_OUTPUT_ENGINE_FUSED, "FUSED"},
    {LOC_OUTPUT_ENGINE_SPE, "SPE"},
    {LOC_OUTPUT_ENGINE_PPE, "PPE"},
    {LOC_OUTPUT_ENGINE_VPE, "VPE"},
    {LOC_OUTPUT_ENGINE_COUNT, "COUNT"}
};
// PositioningEngineMask
DECLARE_TBL(PositioningEngineMask) = {
    {STANDARD_POSITIONING_ENGINE, "SPE"},
    {DEAD_RECKONING_ENGINE, "DRE"},
    {PRECISE_POSITIONING_ENGINE, "PPE"},
    {VP_POSITIONING_ENGINE, "VPE"}
};
// GnssDataMask
DECLARE_TBL(GnssDataMask) = {
    {GNSS_DATA_JAMMER_IND_BIT, "JAMMER"},
    {GNSS_DATA_AGC_BIT, "AGC"}
};
// GnssMeasurementsDataFlagsMask
DECLARE_TBL(GnssMeasurementsDataFlagsMask) = {
    {GNSS_MEASUREMENTS_DATA_SV_ID_BIT, "svId"},
    {GNSS_MEASUREMENTS_DATA_SV_TYPE_BIT, "svType"},
    {GNSS_MEASUREMENTS_DATA_STATE_BIT, "stateMask"},
    {GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_BIT, "receivedSvTimeNs"},
    {GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_UNCERTAINTY_BIT, "receivedSvTimeUncertaintyNs"},
    {GNSS_MEASUREMENTS_DATA_CARRIER_TO_NOISE_BIT, "carrierToNoiseDbHz"},
    {GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_BIT, "pseudorangeRateMps"},
    {GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_UNCERTAINTY_BIT, "pseudorangeRateUncertaintyMps"},
    {GNSS_MEASUREMENTS_DATA_ADR_STATE_BIT, "adrStateMask"},
    {GNSS_MEASUREMENTS_DATA_ADR_BIT, "adrMeters"},
    {GNSS_MEASUREMENTS_DATA_ADR_UNCERTAINTY_BIT, "adrUncertaintyMeters"},
    {GNSS_MEASUREMENTS_DATA_CARRIER_FREQUENCY_BIT, "carrierFrequencyHz"},
    {GNSS_MEASUREMENTS_DATA_CARRIER_CYCLES_BIT, "carrierCycles"},
    {GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_BIT, "carrierPhase"},
    {GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_UNCERTAINTY_BIT, "carrierPhaseUncertainty"},
    {GNSS_MEASUREMENTS_DATA_MULTIPATH_INDICATOR_BIT, "multipathIndicator"},
    {GNSS_MEASUREMENTS_DATA_SIGNAL_TO_NOISE_RATIO_BIT, "signalToNoiseRatioDb"},
    {GNSS_MEASUREMENTS_DATA_AUTOMATIC_GAIN_CONTROL_BIT, "agcLevelDb"},
    {GNSS_MEASUREMENTS_DATA_FULL_ISB_BIT, "fullInterSignalBiasNs"},
    {GNSS_MEASUREMENTS_DATA_FULL_ISB_UNCERTAINTY_BIT, "fullInterSignalBiasUncertaintyNs"},
    {GNSS_MEASUREMENTS_DATA_CYCLE_SLIP_COUNT_BIT, "cycleSlipCount"},
    {GNSS_MEASUREMENTS_DATA_GNSS_SIGNAL_TYPE_BIT, "gnssSignalType"},
    {GNSS_MEASUREMENTS_DATA_BASEBAND_CARRIER_TO_NOISE_BIT, "basebandCarrierToNoiseDbHz"}
};
// GnssMeasurementsStateMask
DECLARE_TBL(GnssMeasurementsStateMask) = {
    {GNSS_MEASUREMENTS_STATE_CODE_LOCK_BIT, "CODE_LOCK"},
    {GNSS_MEASUREMENTS_STATE_BIT_SYNC_BIT, "BIT_SYNC"},
    {GNSS_MEASUREMENTS_STATE_SUBFRAME_SYNC_BIT, "SUBFRAME_SYNC"},
    {GNSS_MEASUREMENTS_STATE_TOW_DECODED_BIT, "TOW_DECODED"},
    {GNSS_MEASUREMENTS_STATE_MSEC_AMBIGUOUS_BIT, "MSEC_AMBIGUOUS"},
    {GNSS_MEASUREMENTS_STATE_SYMBOL_SYNC_BIT, "SYMBOL_SYNC"},
    {GNSS_MEASUREMENTS_STATE_GLO_STRING_SYNC_BIT, "GLO_STRING_SYNC"},
    {GNSS_MEASUREMENTS_STATE_GLO_TOD_DECODED_BIT, "GLO_TOD_DECODED"},
    {GNSS_MEASUREMENTS_STATE_BDS_D2_BIT_SYNC_BIT, "BDS_D2_BIT_SYNC"},
    {GNSS_MEASUREMENTS_STATE_BDS_D2_SUBFRAME_SYNC_BIT, "BDS_D2_SUBFRAME_SYNC"},
    {GNSS_MEASUREMENTS_STATE_GAL_E1BC_CODE_LOCK_BIT, "GAL_E1BC_CODE_LOCK"},
    {GNSS_MEASUREMENTS_STATE_GAL_E1C_2ND_CODE_LOCK_BIT, "GAL_E1C_2ND_CODE_LOCK"},
    {GNSS_MEASUREMENTS_STATE_GAL_E1B_PAGE_SYNC_BIT, "GAL_E1B_PAGE_SYNC"},
    {GNSS_MEASUREMENTS_STATE_SBAS_SYNC_BIT, "SBAS_SYNC"}
};
// GnssMeasurementsAdrStateMask
DECLARE_TBL(GnssMeasurementsAdrStateMask) = {
    {GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_VALID_BIT, "VALID"},
    {GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_RESET_BIT, "RESET"},
    {GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_CYCLE_SLIP_BIT, "CYCLE_SLIP"},
    {GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_HALF_CYCLE_RESOLVED_BIT, "HALF_CYCLE"}
};
// GnssMeasurementsMultipathIndicator
DECLARE_TBL(GnssMeasurementsMultipathIndicator) = {
    {GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_UNKNOWN, "UNKNOWN"},
    {GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_PRESENT, "PRESENT"},
    {GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_NOT_PRESENT, "NOT_PRESENT"}
};
// GnssMeasurementsClockFlagsMask
DECLARE_TBL(GnssMeasurementsClockFlagsMask) = {
    {GNSS_MEASUREMENTS_CLOCK_FLAGS_LEAP_SECOND_BIT, "LEAP_SECOND"},
    {GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_BIT, "TIME"},
    {GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_UNCERTAINTY_BIT, "TIME_UNC"},
    {GNSS_MEASUREMENTS_CLOCK_FLAGS_FULL_BIAS_BIT, "FULL_BIAS"},
    {GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_BIT, "BIAS"},
    {GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_UNCERTAINTY_BIT, "BIAS_UNC"},
    {GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_BIT, "DRIFT"},
    {GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_UNCERTAINTY_BIT, "DRIFT_UNC"},
    {GNSS_MEASUREMENTS_CLOCK_FLAGS_HW_CLOCK_DISCONTINUITY_COUNT_BIT, "HW_CLK_DISCONTINUITY_CNT"},
    {GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_REAL_TIME_BIT, "ELAPSED_REAL_TIME"},
    {GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_REAL_TIME_UNC_BIT, "ELAPSED_REAL_TIME_UNC"},
    {GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_GPTP_TIME_BIT, "ELAPSED_GPTP_TIME"},
    {GNSS_MEASUREMENTS_CLOCK_FLAGS_ELAPSED_GPTP_TIME_UNC_BIT, "ELAPSED_GPTP_TIME_UNC"}
};
// LeapSecondSysInfoMask
DECLARE_TBL(LeapSecondSysInfoMask) = {
    {LEAP_SECOND_SYS_INFO_CURRENT_LEAP_SECONDS_BIT, "CUR_LEAP_SEC"},
    {LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT, "LEAP_SEC_CHANGE"}
};
// LocationSystemInfoMask
DECLARE_TBL(LocationSystemInfoMask) = {
    {LOC_SYS_INFO_LEAP_SECOND, "LEAP_SEC"}
};

// LocationSystemInfoMask
DECLARE_TBL(DrSolutionStatusMask) = {
    {DR_SOLUTION_STATUS_VEHICLE_SENSOR_SPEED_INPUT_DETECTED, "VEHICLE_SENSOR_SPEED_INPUT_DETECTED"},
    {DR_SOLUTION_STATUS_VEHICLE_SENSOR_SPEED_INPUT_USED, "VEHICLE_SENSOR_SPEED_INPUT_USED"},
    {DR_SOLUTION_STATUS_WARNING_UNCALIBRATED, "WARNING_UNCALIBRATED"},
    {DR_SOLUTION_STATUS_WARNING_GNSS_QUALITY_INSUFFICIENT, "WARNING_GNSS_QUALITY_INSUFFICIENT"},
    {DR_SOLUTION_STATUS_WARNING_FERRY_DETECTED, "WARNING_FERRY_DETECTED"},
    {DR_SOLUTION_STATUS_ERROR_6DOF_SENSOR_UNAVAILABLE, "ERROR_6DOF_SENSOR_UNAVAILABLE"},
    {DR_SOLUTION_STATUS_ERROR_VEHICLE_SPEED_UNAVAILABLE, "ERROR_VEHICLE_SPEED_UNAVAILABLE"},
    {DR_SOLUTION_STATUS_ERROR_GNSS_EPH_UNAVAILABLE, "ERROR_GNSS_EPH_UNAVAILABLE"},
    {DR_SOLUTION_STATUS_ERROR_GNSS_MEAS_UNAVAILABLE, "ERROR_GNSS_MEAS_UNAVAILABLE"},
    {DR_SOLUTION_STATUS_WARNING_INIT_POSITION_INVALID, "WARNING_INIT_POSITION_INVALID"},
    {DR_SOLUTION_STATUS_WARNING_INIT_POSITION_UNRELIABLE, "WARNING_INIT_POSITION_UNRELIABLE"},
    {DR_SOLUTION_STATUS_WARNING_POSITON_UNRELIABLE, "WARNING_POSITON_UNRELIABLE"},
    {DR_SOLUTION_STATUS_ERROR_GENERIC, "ERROR_GENERIC"},
    {DR_SOLUTION_STATUS_WARNING_SENSOR_TEMP_OUT_OF_RANGE, "WARNING_SENSOR_TEMP_OUT_OF_RANGE"},
    {DR_SOLUTION_STATUS_WARNING_USER_DYNAMICS_INSUFFICIENT, "WARNING_USER_DYNAMICS_INSUFFICIENT"},
    {DR_SOLUTION_STATUS_WARNING_FACTORY_DATA_INCONSISTENT, "WARNING_FACTORY_DATA_INCONSISTENT"},
    {DR_SOLUTION_STATUS_WARNING_MMF_UNAVAILABLE, "WARNING_MMF_UNAVAILABLE"},
    {DR_SOLUTION_STATUS_WARNING_MMF_NOT_USABLE, "MMF_NOT_USABLE"},
};

DECLARE_TBL(GnssDcReportType) = {
    {QZSS_JMA_DISASTER_PREVENTION_INFO, "QZSS_JMA_DISASTER_PREVENTION_INFO"},
    {QZSS_NON_JMA_DISASTER_PREVENTION_INFO, "QZSS_NON_JMA_DISASTER_PREVENTION_INFO"}
};

string LocationClientApi::capabilitiesToString(LocationCapabilitiesMask capabMask) {
    string out;
    out.reserve(256);

    out += FIELDVAL_MASK(capabMask, LocationCapabilitiesMask_tbl);

    return out;
}

// LocSessionStatus
DECLARE_TBL(LocSessionStatus) = {
    {LOC_SESS_SUCCESS, "LOC_SESS_SUCCESS"},
    {LOC_SESS_INTERMEDIATE, "LOC_SESS_INTERMEDIATE"},
    {LOC_SESS_FAILURE, "LOC_SESS_FAILURE" }
};

string GnssLocationSvUsedInPosition::toString() const {
    string out;
    out.reserve(256);

    // gpsSvUsedIdsMask
    out += FIELDVAL_HEX(gpsSvUsedIdsMask);
    out += loc_parenthesize(maskToVals(gpsSvUsedIdsMask, GPS_SV_PRN_MIN)) + "\n";

    out += FIELDVAL_HEX(gloSvUsedIdsMask);
    out += loc_parenthesize(maskToVals(gloSvUsedIdsMask, GLO_SV_PRN_MIN)) + "\n";

    out += FIELDVAL_HEX(galSvUsedIdsMask);
    out += loc_parenthesize(maskToVals(galSvUsedIdsMask, GAL_SV_PRN_MIN)) + "\n";

    out += FIELDVAL_HEX(bdsSvUsedIdsMask);
    out += loc_parenthesize(maskToVals(bdsSvUsedIdsMask, BDS_SV_PRN_MIN)) + "\n";

    out += FIELDVAL_HEX(qzssSvUsedIdsMask);
    out += loc_parenthesize(maskToVals(qzssSvUsedIdsMask, QZSS_SV_PRN_MIN)) + "\n";

    out += FIELDVAL_HEX(navicSvUsedIdsMask);
    out += loc_parenthesize(maskToVals(navicSvUsedIdsMask, NAVIC_SV_PRN_MIN)) + "\n";

    return out;
}

string GnssMeasUsageInfo::toString() const {
    string out;
    out.reserve(256);

    out += FIELDVAL_ENUM(gnssConstellation, Gnss_LocSvSystemEnumType_tbl);
    out += FIELDVAL_DEC(gnssSvId);
    out += FIELDVAL_MASK(gnssSignalType, GnssSignalTypeMask_tbl);

    return out;
}

string GnssLocationPositionDynamics::toString() const {
    string out;
    out.reserve(256);

    out += FIELDVAL_MASK(bodyFrameDataMask, GnssLocationPosDataMask_tbl);
    out += FIELDVAL_DEC(longAccel);
    out += FIELDVAL_DEC(latAccel);
    out += FIELDVAL_DEC(vertAccel);
    out += FIELDVAL_DEC(longAccelUnc);
    out += FIELDVAL_DEC(latAccelUnc);
    out += FIELDVAL_DEC(vertAccelUnc);
    out += FIELDVAL_DEC(pitch);
    out += FIELDVAL_DEC(pitchUnc);
    out += FIELDVAL_DEC(pitchRate);
    out += FIELDVAL_DEC(pitchRateUnc);
    out += FIELDVAL_DEC(roll);
    out += FIELDVAL_DEC(rollUnc);
    out += FIELDVAL_DEC(rollRate);
    out += FIELDVAL_DEC(rollRateUnc);
    out += FIELDVAL_DEC(yaw);
    out += FIELDVAL_DEC(yawUnc);
    out += FIELDVAL_DEC(yawRate);
    out += FIELDVAL_DEC(yawRateUnc);

    return out;
}

string GnssSystemTimeStructType::toString() const {
    string out;
    out.reserve(256);

    out += FIELDVAL_MASK(validityMask, GnssSystemTimeStructTypeFlags_tbl);
    out += FIELDVAL_DEC(systemWeek);
    out += FIELDVAL_DEC(systemMsec);
    out += FIELDVAL_DEC(systemClkTimeBias);
    out += FIELDVAL_DEC(systemClkTimeUncMs);
    out += FIELDVAL_DEC(refFCount);
    out += FIELDVAL_DEC(numClockResets);

    return out;
}

string GnssGloTimeStructType::toString() const {
    string out;
    out.reserve(256);

    out += FIELDVAL_MASK(validityMask, GnssGloTimeStructTypeFlags_tbl);
    out += FIELDVAL_DEC(gloFourYear);
    out += FIELDVAL_DEC(gloDays);
    out += FIELDVAL_DEC(gloMsec);
    out += FIELDVAL_DEC(gloClkTimeBias);
    out += FIELDVAL_DEC(gloClkTimeUncMs);
    out += FIELDVAL_DEC(refFCount);
    out += FIELDVAL_DEC(numClockResets);

    return out;
}

string GnssSystemTime::toString() const {
    switch (gnssSystemTimeSrc) {
    case GNSS_LOC_SV_SYSTEM_GPS:
        return u.gpsSystemTime.toString();
    case GNSS_LOC_SV_SYSTEM_GALILEO:
        return u.galSystemTime.toString();
    case GNSS_LOC_SV_SYSTEM_GLONASS:
        return u.gloSystemTime.toString();
    case GNSS_LOC_SV_SYSTEM_BDS:
        return u.bdsSystemTime.toString();
    case GNSS_LOC_SV_SYSTEM_QZSS:
        return u.qzssSystemTime.toString();
    case GNSS_LOC_SV_SYSTEM_NAVIC:
        return u.navicSystemTime.toString();
    default:
        return "Unknown System ID: " + to_string(gnssSystemTimeSrc);
    }
}

string LLAInfo::toString() const {
    string out;
    out.reserve(256);
    out +=  "VRP based " + FIELDVAL_DEC(latitude);
    out +=  "VRP based " + FIELDVAL_DEC(longitude);
    out +=  "VRP based " + FIELDVAL_DEC(altitude);
    return out;
}

string Location::toString() const {
    string out;
    out.reserve(512);

    out += FIELDVAL_MASK(flags, LocationFlagsMask_tbl);
    out += FIELDVAL_DEC(timestamp);
    out += FIELDVAL_DEC(latitude);
    out += FIELDVAL_DEC(longitude);
    out += FIELDVAL_DEC(altitude);
    out += FIELDVAL_DEC(speed);
    out += FIELDVAL_DEC(bearing);
    out += FIELDVAL_DEC(horizontalAccuracy);
    out += FIELDVAL_DEC(verticalAccuracy);
    out += FIELDVAL_DEC(speedAccuracy);
    out += FIELDVAL_DEC(bearingAccuracy);
    out += FIELDVAL_MASK(techMask, LocationTechnologyMask_tbl);
    out += FIELDVAL_DEC(elapsedgPTPTime);
    out += FIELDVAL_DEC(elapsedgPTPTimeUnc);
    out += FIELDVAL_MASK(sessionStatus, LocSessionStatus_tbl);
    return out;
}

string GnssLocation::toString() const {
    string out;
    out.reserve(8096);

    out += Location::toString();
    out += FIELDVAL_MASK(gnssInfoFlags, LCAGnssLocationInfoFlagMask_tbl);
    out += FIELDVAL_DEC(altitudeMeanSeaLevel);
    out += FIELDVAL_DEC(pdop);
    out += FIELDVAL_DEC(hdop);
    out += FIELDVAL_DEC(vdop);
    out += FIELDVAL_DEC(gdop);
    out += FIELDVAL_DEC(tdop);
    out += FIELDVAL_DEC(magneticDeviation);
    out += FIELDVAL_ENUM(horReliability, LocationReliability_tbl);
    out += FIELDVAL_ENUM(verReliability, LocationReliability_tbl);
    out += FIELDVAL_DEC(horUncEllipseSemiMajor);
    out += FIELDVAL_DEC(horUncEllipseSemiMinor);
    out += FIELDVAL_DEC(horUncEllipseOrientAzimuth);
    out += FIELDVAL_DEC(northStdDeviation);
    out += FIELDVAL_DEC(eastStdDeviation);
    out += FIELDVAL_DEC(northVelocity);
    out += FIELDVAL_DEC(eastVelocity);
    out += FIELDVAL_DEC(upVelocity);
    out += FIELDVAL_DEC(northVelocityStdDeviation);
    out += FIELDVAL_DEC(eastVelocityStdDeviation);
    out += FIELDVAL_DEC(upVelocityStdDeviation);
    out += FIELDVAL_DEC(numSvUsedInPosition);
    out += svUsedInPosition.toString();
    out += FIELDVAL_MASK(navSolutionMask, GnssLocationNavSolutionMask_tbl);
    out += bodyFrameData.toString();
    out += gnssSystemTime.toString();

    uint32_t ind = 0;
    for (auto measUsage : measUsageInfo) {
        out += "measUsageInfo[";
        out += to_string(ind);
        out += "]: ";
        out += measUsage.toString();
        ind++;
    }

    out += FIELDVAL_DEC(leapSeconds);
    out += FIELDVAL_DEC(timeUncMs);
    out += FIELDVAL_DEC(calibrationConfidencePercent);
    out += FIELDVAL_MASK(calibrationStatus, DrCalibrationStatusMask_tbl);
    out += FIELDVAL_ENUM(locOutputEngType, LocOutputEngineType_tbl);
    out += FIELDVAL_MASK(locOutputEngMask, PositioningEngineMask_tbl);
    out += FIELDVAL_DEC(conformityIndex);
    out += llaVRPBased.toString();
    out += FIELDVAL_DEC(enuVelocityVRPBased[0]);
    out += FIELDVAL_DEC(enuVelocityVRPBased[1]);
    out += FIELDVAL_DEC(enuVelocityVRPBased[2]);
    out += FIELDVAL_MASK(drSolutionStatusMask, DrSolutionStatusMask_tbl);
    out += FIELDVAL_DEC(altitudeAssumed);
    out += FIELDVAL_MASK(sessionStatus, LocSessionStatus_tbl);
    out += FIELDVAL_DEC(integrityRiskUsed);
    out += FIELDVAL_DEC(protectAlongTrack);
    out += FIELDVAL_DEC(protectCrossTrack);
    out += FIELDVAL_DEC(protectVertical);
    out += FIELDVAL_DEC(elapsedRealTimeNs);
    out += FIELDVAL_DEC(elapsedRealTimeUncNs);
    out += FIELDVAL_DEC(timeUncMs);
    uint32_t count = 0;
    for (auto dgnssId : dgnssStationId) {
        out += "dgnssStationId[";
        out += to_string(count);
        out += "]: ";
        out += to_string(dgnssId);
        count++;
    }
    out += FIELDVAL_DEC(baseLineLength);
    out += FIELDVAL_DEC(ageMsecOfCorrections);
    out += FIELDVAL_DEC(leapSecondsUnc);
    return out;
}

string GnssSv::toString() const {
    string out;
    out.reserve(256);

    out += FIELDVAL_DEC(svId);
    out += FIELDVAL_ENUM(type, GnssSvType_tbl);
    out += FIELDVAL_DEC(cN0Dbhz);
    out += FIELDVAL_DEC(elevation);
    out += FIELDVAL_DEC(azimuth);
    out += FIELDVAL_MASK(gnssSvOptionsMask, GnssSvOptionsMask_tbl);
    out += FIELDVAL_DEC(carrierFrequencyHz);
    out += FIELDVAL_MASK(gnssSignalTypeMask, GnssSignalTypeMask_tbl);
    out += FIELDVAL_DEC(basebandCarrierToNoiseDbHz);
    out += FIELDVAL_DEC(gloFrequency);

    return out;
}

string GnssData::toString() const {
    string out;
    out.reserve(4096);

    for (int i = 0; i < GNSS_MAX_NUMBER_OF_SIGNAL_TYPES; i++) {
        out += FIELDVAL_MASK(i, GnssSignalTypes_tbl);
        out += FIELDVAL_MASK(gnssDataMask[i], GnssDataMask_tbl);
        out += FIELDVAL_DEC(jammerInd[i]);
        out += FIELDVAL_DEC(agc[i]);
    }
    out += FIELDVAL_DEC(agcStatusL1);
    out += FIELDVAL_DEC(agcStatusL2);
    out += FIELDVAL_DEC(agcStatusL5);

    return out;
}

string GnssMeasurementsData::toString() const {
    string out;
    out.reserve(256);

    out += FIELDVAL_MASK(flags, GnssMeasurementsDataFlagsMask_tbl);
    out += FIELDVAL_DEC(svId);
    out += FIELDVAL_ENUM(svType, GnssSvType_tbl);
    out += FIELDVAL_DEC(timeOffsetNs);
    out += FIELDVAL_MASK(stateMask, GnssMeasurementsStateMask_tbl);
    out += FIELDVAL_DEC(receivedSvTimeNs);
    out += FIELDVAL_DEC(receivedSvTimeSubNs);
    out += FIELDVAL_DEC(receivedSvTimeUncertaintyNs);
    out += FIELDVAL_DEC(carrierToNoiseDbHz);
    out += FIELDVAL_DEC(pseudorangeRateMps);
    out += FIELDVAL_DEC(pseudorangeRateUncertaintyMps);
    out += FIELDVAL_MASK(adrStateMask, GnssMeasurementsAdrStateMask_tbl);
    out += FIELDVAL_DEC(adrMeters);
    out += FIELDVAL_DEC(adrUncertaintyMeters);
    out += FIELDVAL_DEC(carrierFrequencyHz);
    out += FIELDVAL_DEC(carrierCycles);
    out += FIELDVAL_DEC(carrierPhase);
    out += FIELDVAL_DEC(carrierPhaseUncertainty);
    out += FIELDVAL_ENUM(multipathIndicator, GnssMeasurementsMultipathIndicator_tbl);
    out += FIELDVAL_DEC(signalToNoiseRatioDb);
    out += FIELDVAL_DEC(agcLevelDb);
    out += FIELDVAL_DEC(basebandCarrierToNoiseDbHz);
    out += FIELDVAL_MASK(gnssSignalType, GnssSignalTypeMask_tbl);
    out += FIELDVAL_DEC(fullInterSignalBiasNs);
    out += FIELDVAL_DEC(fullInterSignalBiasUncertaintyNs);
    out += FIELDVAL_DEC(cycleSlipCount);
    out += FIELDVAL_DEC(basebandCarrierToNoiseDbHz);

    return out;
}

string GnssMeasurementsClock::toString() const {
    string out;
    out.reserve(256);

    out += FIELDVAL_MASK(flags, GnssMeasurementsClockFlagsMask_tbl);
    out += FIELDVAL_DEC(leapSecond);
    out += FIELDVAL_DEC(timeNs);
    out += FIELDVAL_DEC(timeUncertaintyNs);
    out += FIELDVAL_DEC(fullBiasNs);
    out += FIELDVAL_DEC(biasNs);
    out += FIELDVAL_DEC(biasUncertaintyNs);
    out += FIELDVAL_DEC(driftNsps);
    out += FIELDVAL_DEC(driftUncertaintyNsps);
    out += FIELDVAL_DEC(hwClockDiscontinuityCount);
    out += FIELDVAL_DEC(elapsedRealTime);
    out += FIELDVAL_DEC(elapsedRealTimeUnc);
    out += FIELDVAL_DEC(elapsedgPTPTime);
    out += FIELDVAL_DEC(elapsedgPTPTimeUnc);

    return out;
}

string GnssMeasurements::toString() const {
    string out;
    // (number of GnssMeasurementsData in the vector + GnssMeasurementsClock) * 256
    out.reserve((measurements.size() + 1) << 8);

    out += clock.toString();
    for (auto meas : measurements) {
        out += meas.toString();
    }

    out += FIELDVAL_DEC(isNhz);
    out += FIELDVAL_DEC(agcStatusL1);
    out += FIELDVAL_DEC(agcStatusL2);
    out += FIELDVAL_DEC(agcStatusL5);
    return out;
}

string LeapSecondChangeInfo::toString() const {
    string out;
    out.reserve(256);

    out += gpsTimestampLsChange.toString();
    out += FIELDVAL_DEC(leapSecondsBeforeChange);
    out += FIELDVAL_DEC(leapSecondsAfterChange);

    return out;
}

string LeapSecondSystemInfo::toString() const {
    string out;
    out.reserve(256);

    out += FIELDVAL_MASK(leapSecondInfoMask, LeapSecondSysInfoMask_tbl);
    out += FIELDVAL_DEC(leapSecondCurrent);

    return out;
}

string LocationSystemInfo::toString() const {
    string out;
    out.reserve(256);

    out += FIELDVAL_MASK(systemInfoMask, LocationSystemInfoMask_tbl);
    out += leapSecondSysInfo.toString();

    return out;
}

string GnssDcReport::toString() const {
    string out;
    out.reserve(256);

    out += FIELDVAL_ENUM(dcReportType, GnssDcReportType_tbl);
    out += FIELDVAL_DEC(numValidBits);

    size_t bufSize = dcReportData.size() * 5 + 1;
    char *ptr = (char*) malloc(bufSize);

    if (ptr != NULL) {
        char *ptrCopy = ptr;
        for (uint8_t byte : dcReportData) {
            int numbytes = snprintf(ptrCopy, bufSize, "0x%02X ", byte);
            ptrCopy += numbytes;
        }
        *ptrCopy = '\0';
        out.append(ptr);

        free(ptr);
    }

    return out;
}

DECLARE_TBL(GnssEphSource) = {
    {GNSS_EPH_SRC_UNKNOWN, "SRC_UNKNOWN"},
    {GNSS_EPH_SRC_OTA, "SRC_OTA"},
    {GNSS_EPH_SRC_MAX, "SRC_MAX"},
};

DECLARE_TBL(GnssEphAction) = {
    {GNSS_EPH_ACTION_UNKNOWN, "ACTION_UNKNOWN"},
    {GNSS_EPH_ACTION_UPDATE, "ACTION_UPDATE"},
    {GNSS_EPH_ACTION_DELETE, "ACTION_DELETE"},
    {GNSS_EPH_ACTION_MAX, "ACTION_MAX"}
};

DECLARE_TBL(GalEphSignalSource) = {
    {GAL_EPH_SIGNAL_SRC_UNKNOWN, "UNKNOWN"},
    {GAL_EPH_SIGNAL_SRC_E1B, "SRC_E1B"},
    {GAL_EPH_SIGNAL_SRC_E5A, "SRC_E5A"},
    {GAL_EPH_SIGNAL_SRC_E5B, "SRC_E5B"}
};

string GnssEphCommonInfo::toString() const {
    string out;
    out.reserve(8096);
    out += FIELDVAL_DEC(gnssSvId);
    out += FIELDVAL_DEC(ephSource);
    out += FIELDVAL_DEC(action);
    out += FIELDVAL_DEC(IODE);
    out += FIELDVAL_DEC(aSqrt);
    out += FIELDVAL_DEC(deltaN);
    out += FIELDVAL_DEC(m0);
    out += FIELDVAL_DEC(eccentricity);
    out += FIELDVAL_DEC(omega0);
    out += FIELDVAL_DEC(i0);
    out += FIELDVAL_DEC(omega);
    out += FIELDVAL_DEC(omegaDot);
    out += FIELDVAL_DEC(iDot);
    out += FIELDVAL_DEC(cUc);
    out += FIELDVAL_DEC(cUs);
    out += FIELDVAL_DEC(cRc);
    out += FIELDVAL_DEC(cRs);
    out += FIELDVAL_DEC(cIc);
    out += FIELDVAL_DEC(cIs);
    out += FIELDVAL_DEC(toe);
    out += FIELDVAL_DEC(toc);
    out += FIELDVAL_DEC(af0);
    out += FIELDVAL_DEC(af1);
    out += FIELDVAL_DEC(af2);
    return out;
}

string GpsQzssEphemeris::toString() const {

    string out;
    out.reserve(256);
    out += commonEphemerisData.toString();
    out += FIELDVAL_DEC(signalHealth);
    out += FIELDVAL_DEC(URAI);
    out += FIELDVAL_DEC(codeL2);
    out += FIELDVAL_DEC(dataFlagL2P);
    out += FIELDVAL_DEC(tgd);
    out += FIELDVAL_DEC(fitInterval);
    out += FIELDVAL_DEC(IODC);
    return out;
}

string GlonassEphemeris::toString() const {

    string out;
    out.reserve(256);
    out += FIELDVAL_DEC(gnssSvId);
    out += FIELDVAL_DEC(ephSource);
    out += FIELDVAL_DEC(action);
    out += FIELDVAL_DEC(bnHealth);
    out += FIELDVAL_DEC(lnHealth);
    out += FIELDVAL_DEC(tb);
    out += FIELDVAL_DEC(ft);
    out += FIELDVAL_DEC(gloM);
    out += FIELDVAL_DEC(enAge);
    out += FIELDVAL_DEC(gloFrequency);
    out += FIELDVAL_DEC(p1);
    out += FIELDVAL_DEC(p2);
    out += FIELDVAL_DEC(deltaTau);
    out += FIELDVAL_DEC(position[0]);
    out += FIELDVAL_DEC(position[1]);
    out += FIELDVAL_DEC(position[2]);
    out += FIELDVAL_DEC(velocity[0]);
    out += FIELDVAL_DEC(velocity[1]);
    out += FIELDVAL_DEC(velocity[2]);
    out += FIELDVAL_DEC(acceleration[0]);
    out += FIELDVAL_DEC(acceleration[1]);
    out += FIELDVAL_DEC(acceleration[2]);
    out += FIELDVAL_DEC(tauN);
    out += FIELDVAL_DEC(gamma);
    out += FIELDVAL_DEC(toe);
    out += FIELDVAL_DEC(nt);
    return out;
}

string BdsEphemeris::toString() const {

    string out;
    out.reserve(256);
    out += commonEphemerisData.toString();
    out += FIELDVAL_DEC(svHealth);
    out += FIELDVAL_DEC(AODC);
    out += FIELDVAL_DEC(tgd1);
    out += FIELDVAL_DEC(tgd2);
    out += FIELDVAL_DEC(URAI);
    return out;
}

string GalileoEphemeris::toString() const {

    string out;
    out.reserve(256);
    out += commonEphemerisData.toString();
    out += FIELDVAL_DEC(dataSourceSignal);
    out += FIELDVAL_DEC(sisIndex);
    out += FIELDVAL_DEC(bgdE1E5a);
    out += FIELDVAL_DEC(bgdE1E5b);
    out += FIELDVAL_DEC(svHealth);
    return out;
}

string QzssEphemeris::toString() const {
    string out;
    out.reserve(256);
    out += qzssEphData.toString();
    return out;
}

string NavicEphemeris::toString() const {

    string out;
    out.reserve(256);
    out += commonEphemerisData.toString();
    out += FIELDVAL_DEC(weekNum);
    out += FIELDVAL_DEC(iodec);
    out += FIELDVAL_DEC(l5Health);
    out += FIELDVAL_DEC(sHealth);
    out += FIELDVAL_DEC(inclinationAngleRad);
    out += FIELDVAL_DEC(urai);
    out += FIELDVAL_DEC(tgd);
    return out;
}

string GnssEphemeris::toString() const {
    string out;
    out.reserve(8192);

    out += FIELDVAL_ENUM(gnssConstellation, Gnss_LocSvSystemEnumType_tbl);
    out += FIELDVAL_DEC(isSystemTimeValid);
    out += systemTime.toString();

    uint32_t ind = 0;
    switch (gnssConstellation) {
        case GNSS_LOC_SV_SYSTEM_GPS:
            for (auto eph : gpsEphemerisData) {
                out += "gpsEphemerisData[";
                out += to_string(ind);
                out += "]: ";
                out += eph.toString();
                ind++;
            }
            break;
        case GNSS_LOC_SV_SYSTEM_GALILEO:
            for (auto eph : galEphemerisData) {
                out += "galileoEphemerisData[";
                out += to_string(ind);
                out += "]: ";
                out += eph.toString();
                ind++;
            }
            break;
        case GNSS_LOC_SV_SYSTEM_GLONASS:
            for (auto eph : gloEphemerisData) {
                out += "glonassEphemerisData[";
                out += to_string(ind);
                out += "]: ";
                out += eph.toString();
                ind++;
            }
            break;
        case GNSS_LOC_SV_SYSTEM_BDS:
            for (auto eph : bdsEphemerisData) {
                out += "bdsEphemerisData[";
                out += to_string(ind);
                out += "]: ";
                out += eph.toString();
                ind++;
            }
            break;
        case GNSS_LOC_SV_SYSTEM_QZSS:
            for (auto eph : qzssEphemerisData) {
                out += "qzssEphemerisData[";
                out += to_string(ind);
                out += "]: ";
                out += eph.toString();
                ind++;
            }
            break;
        case GNSS_LOC_SV_SYSTEM_NAVIC:
            for (auto eph : navicEphemerisData) {
                out += "navicEphemerisData[";
                out += to_string(ind);
                out += "]: ";
                out += eph.toString();
                ind++;
            }
            break;
    }
    return out;
}

} // namespace location_client
