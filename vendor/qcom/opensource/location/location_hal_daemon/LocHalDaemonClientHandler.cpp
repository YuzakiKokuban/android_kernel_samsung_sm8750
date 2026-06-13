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

#include <cinttypes>
#include <gps_extended_c.h>
#include <LocationApiMsg.h>
#include <LocHalDaemonClientHandler.h>
#include <LocationApiService.h>

shared_ptr<LocIpcSender> LocHalDaemonClientHandler::createSender(const string socket) {
    SockNode sockNode(SockNode::create(socket));
    return sockNode.createSender();
}

static GeofenceBreachTypeMask parseClientGeofenceBreachType(GeofenceBreachType type);

/******************************************************************************
LocHalDaemonClientHandler - updateSubscriptionMask
******************************************************************************/
void LocHalDaemonClientHandler::updateSubscription(uint32_t mask) {

    // update my subscription mask
    mSubscriptionMask = mask;

    // set callback functions for Location API
    mCallbacks.size = sizeof(mCallbacks);

    // mandatory callback
    mCallbacks.capabilitiesCb = [this](LocationCapabilitiesMask mask) {
        onCapabilitiesCallback(mask);
    };
    mCallbacks.responseCb = [this](LocationError err, uint32_t id) {
        onResponseCb(err, id);
    };
    mCallbacks.collectiveResponseCb =
            [this](size_t count, LocationError* errs, uint32_t* ids) {
        onCollectiveResponseCallback(count, errs, ids);
    };

    // update optional callback - following four callbacks can be controlable
    // tracking
    if (mSubscriptionMask &
            (E_LOC_CB_TRACKING_BIT | E_LOC_CB_SIMPLE_LOCATION_INFO_BIT)) {
        mCallbacks.trackingCb = [this](const Location& location) {
            onTrackingCb(location);
        };
    } else {
        mCallbacks.trackingCb = nullptr;
    }

    // batching
    if (mSubscriptionMask & E_LOC_CB_BATCHING_BIT) {
        mCallbacks.batchingCb = [this](size_t count, Location* location,
                const BatchingOptions& batchingOptions) {
            onBatchingCb(count, location, batchingOptions);
        };
    } else {
        mCallbacks.batchingCb = nullptr;
    }
    // batchingStatus
    if (mSubscriptionMask & E_LOC_CB_BATCHING_STATUS_BIT) {
        mCallbacks.batchingStatusCb = [this](const BatchingStatusInfo& batchingStatus,
                std::list<uint32_t>& listOfCompletedTrips) {
            onBatchingStatusCb(batchingStatus, listOfCompletedTrips);
        };
    } else {
        mCallbacks.batchingStatusCb = nullptr;
    }
    //Geofence Breach
    if (mSubscriptionMask & E_LOC_CB_GEOFENCE_BREACH_BIT) {
        mCallbacks.geofenceBreachCb = [this](
                const GeofenceBreachNotification& geofenceBreachNotif) {
            onGeofenceBreachCb(geofenceBreachNotif);
        };
    } else {
        mCallbacks.geofenceBreachCb = nullptr;
    }

    // location info
    if (mSubscriptionMask & E_LOC_CB_GNSS_LOCATION_INFO_BIT) {
        mCallbacks.gnssLocationInfoCb = [this](const GnssLocationInfoNotification& notification) {
            onGnssLocationInfoCb(notification);
        };
    } else {
        mCallbacks.gnssLocationInfoCb = nullptr;
    }

    // engine locations info
    if (mSubscriptionMask & E_LOC_CB_ENGINE_LOCATIONS_INFO_BIT) {
        mCallbacks.engineLocationsInfoCb =
                [this](uint32_t count, GnssLocationInfoNotification* notificationArr) {
            onEngLocationsInfoCb(count, notificationArr);
        };
    } else {
        mCallbacks.engineLocationsInfoCb = nullptr;
    }

    // sv info
    if (mSubscriptionMask & E_LOC_CB_GNSS_SV_BIT) {
        mCallbacks.gnssSvCb = [this](const GnssSvNotification &notification) {
            onGnssSvCb(notification);
        };
    } else {
        mCallbacks.gnssSvCb = nullptr;
    }

    // nmea
    if (mSubscriptionMask & E_LOC_CB_GNSS_NMEA_BIT) {
        mCallbacks.gnssNmeaCb = [this](const GnssNmeaNotification& notification) {
            onGnssNmeaCb(notification);
        };
    } else {
        mCallbacks.gnssNmeaCb = nullptr;
    }

   // engine nmea
    if (mSubscriptionMask & E_LOC_CB_ENGINE_NMEA_BIT) {
        mCallbacks.engineNmeaCb = [this](const GnssNmeaNotification& notification) {
            onEngineNmeaCb(notification);
        };
    } else {
        mCallbacks.engineNmeaCb = nullptr;
    }


    // data
    if (mSubscriptionMask & E_LOC_CB_GNSS_DATA_BIT) {
        mCallbacks.gnssDataCb = [this](const GnssDataNotification& notification) {
            onGnssDataCb(notification);
        };
    } else {
        mCallbacks.gnssDataCb = nullptr;
    }

    // measurements
    if (mSubscriptionMask & E_LOC_CB_GNSS_MEAS_BIT) {
        mCallbacks.gnssMeasurementsCb = [this](const GnssMeasurementsNotification &notification) {
            onGnssMeasurementsCb(notification);
        };
    } else {
        mCallbacks.gnssMeasurementsCb = nullptr;
    }

    // nHz measurements
    if (mSubscriptionMask & E_LOC_CB_GNSS_NHZ_MEAS_BIT) {
        mCallbacks.gnssNHzMeasurementsCb = [this](
                const GnssMeasurementsNotification &notification) {
            onGnssMeasurementsCb(notification);
        };
    } else {
        mCallbacks.gnssNHzMeasurementsCb = nullptr;
    }

    // system info
    if (mSubscriptionMask & E_LOC_CB_SYSTEM_INFO_BIT) {
        mCallbacks.locationSystemInfoCb = [this](const LocationSystemInfo& notification) {
            onLocationSystemInfoCb(notification);
        };
    } else {
        mCallbacks.locationSystemInfoCb = nullptr;
    }

    // dc report
    if (mSubscriptionMask & E_LOC_CB_GNSS_DC_REPORT_BIT) {
        mCallbacks.gnssDcReportCb = [this](const GnssDcReportInfo& dcReportInfo) {
            onDcReportCb(dcReportInfo);
        };
    } else {
        mCallbacks.gnssDcReportCb = nullptr;
    }

    // Ephemeris Info
    if (mSubscriptionMask & E_LOC_CB_GNSS_EPH_BIT) {
        mCallbacks.svEphemerisCb = [this](const GnssSvEphemerisReport &notification) {
            onGnssSvEphemerisCb(notification);
        };
    } else {
        mCallbacks.svEphemerisCb = nullptr;
    }

    // following callbacks are not supported
    mCallbacks.gnssNiCb = nullptr;
    mCallbacks.geofenceStatusCb = nullptr;

    // call location API if already created
    if (mLocationApi) {
        LOC_LOGd("--> updateCallbacks mask=0x%x", mask);
        mLocationApi->updateCallbacks(mCallbacks);
    }
}

// Round input TBF to 100ms, 200ms, 500ms, and integer senconds
// input tbf < 200 msec, round to 100 msec, else
// input tbf < 500 msec, round to 200 msec, else
// input tbf < 1000 msec, round to 500 msec, else
// round up input tbf to the closet integer seconds
uint32_t LocHalDaemonClientHandler::startTracking(LocationOptions & locOptions) {
    LOC_LOGd("distance %d, internal %d, req mask %x",
             locOptions.minDistance, locOptions.minInterval,
             locOptions.locReqEngTypeMask);
    if (mSessionId == 0 && mLocationApi) {
        // update option
        mOptions.setLocationOptions(locOptions);
        // set interval to engine supported interval
        mOptions.minInterval = getSupportedTbf(mOptions.minInterval);
        mSessionId = mLocationApi->startTracking(mOptions);
        mPendingMessages.push(E_LOCAPI_START_TRACKING_MSG_ID);
        mTracking = true;
    }

    return mSessionId;
}

void LocHalDaemonClientHandler::stopTracking(bool clientExpectingResp) {
    if (mSessionId != 0 && mLocationApi) {
        mLocationApi->stopTracking(mSessionId);
        mSessionId = 0;
        if (clientExpectingResp) {
            mPendingMessages.push(E_LOCAPI_STOP_TRACKING_MSG_ID);
        }
    }

    mTracking = false;
}

void LocHalDaemonClientHandler::unsubscribeLocationSessionCb() {

    uint32_t subscriptionMask = mSubscriptionMask;

    subscriptionMask &= ~LOCATION_SESSON_ALL_INFO_MASK;
    updateSubscription(subscriptionMask);
}

void LocHalDaemonClientHandler::updateTrackingOptions(LocationOptions & locOptions) {
    if (mSessionId != 0 && mLocationApi) {
        LOC_LOGe("distance %d, internal %d, req mask %x",
             locOptions.minDistance, locOptions.minInterval,
             locOptions.locReqEngTypeMask);

        TrackingOptions trackingOption(locOptions);
        // set tbf to device supported tbf
        trackingOption.minInterval = getSupportedTbf(trackingOption.minInterval);
        mLocationApi->updateTrackingOptions(mSessionId, trackingOption);

        // save the trackingOption: eng req type that will be used in filtering
        mOptions = trackingOption;
    }
}

uint32_t LocHalDaemonClientHandler::startBatching(uint32_t minInterval, uint32_t minDistance,
        BatchingMode batchMode) {
    if (mBatchingId == 0 && mLocationApi) {
        // update option
        LocationOptions locOption = {};
        locOption.size = sizeof(locOption);
        locOption.minInterval = minInterval;
        locOption.minDistance = minDistance;
        locOption.mode = GNSS_SUPL_MODE_STANDALONE;
        mBatchOptions.size = sizeof(mBatchOptions);
        mBatchOptions.batchingMode = batchMode;
        mBatchOptions.setLocationOptions(locOption);

        mBatchingId = mLocationApi->startBatching(mBatchOptions);
    }
    return mBatchingId;

}

void LocHalDaemonClientHandler::stopBatching() {
    if (mBatchingId != 0 && mLocationApi) {
        mLocationApi->stopBatching(mBatchingId);
        mBatchingId = 0;
    }
}

void LocHalDaemonClientHandler::updateBatchingOptions(uint32_t minInterval, uint32_t minDistance,
        BatchingMode batchMode) {
    if (mBatchingId != 0 && mLocationApi) {
        // update option
        LocationOptions locOption = {};
        locOption.size = sizeof(locOption);
        locOption.minInterval = minInterval;
        locOption.minDistance = minDistance;
        locOption.mode = GNSS_SUPL_MODE_STANDALONE;
        mBatchOptions.size = sizeof(mBatchOptions);
        mBatchOptions.batchingMode = batchMode;
        mBatchOptions.setLocationOptions(locOption);

        mLocationApi->updateBatchingOptions(mBatchingId, mBatchOptions);
    }
}
void LocHalDaemonClientHandler::setGeofenceIds(size_t count, uint32_t* clientIds,
        uint32_t* sessionIds) {
    for (int i=0; i<count; ++i) {
        mGfIdsMap[clientIds[i]] = sessionIds[i];
    }
}

void LocHalDaemonClientHandler::eraseGeofenceIds(size_t count, uint32_t* clientIds) {
    for (int i=0; i<count; ++i) {
        mGfIdsMap.erase(clientIds[i]);
    }
}
uint32_t* LocHalDaemonClientHandler::getSessionIds(size_t count, uint32_t* clientIds) {
    uint32_t* sessionIds = nullptr;
    if (count > 0) {
        sessionIds = (uint32_t*)malloc(sizeof(uint32_t) * count);
        if (nullptr == sessionIds) {
            return nullptr;
        }
        memset(sessionIds, 0, sizeof(uint32_t) * count);
        for (int i=0; i<count; ++i) {
            sessionIds[i] = mGfIdsMap[clientIds[i]];
        }
    }
    return sessionIds;
}

uint32_t* LocHalDaemonClientHandler::getClientIds(size_t count, uint32_t* sessionIds) {
    uint32_t* clientIds = nullptr;
    if (count > 0) {
        clientIds = (uint32_t*)malloc(sizeof(uint32_t) * count);
        if (nullptr == clientIds) {
            return nullptr;
        }
        memset(clientIds, 0, sizeof(uint32_t) * count);
        for (int i=0; i<count; ++i) {
            for(auto itor = mGfIdsMap.begin(); itor != mGfIdsMap.end(); itor++) {
                if (itor->second == sessionIds[i]) {
                    clientIds[i] = itor->first;
                }
            }
        }
    }
    return clientIds;
}

uint32_t* LocHalDaemonClientHandler::addGeofences(size_t count, GeofenceOption* options,
        GeofenceInfo* info) {
    if (count > 0 && mLocationApi) {
        mGeofenceIds = mLocationApi->addGeofences(count, options, info);
        if (mGeofenceIds) {
            LOC_LOGi("start new geofence sessions: %p", mGeofenceIds);
        }
    }
    return mGeofenceIds;
}

void LocHalDaemonClientHandler::removeGeofences(size_t count, uint32_t* ids) {
    if (count > 0 && mLocationApi) {
        mLocationApi->removeGeofences(count, ids);
    }
}

void LocHalDaemonClientHandler::modifyGeofences(size_t count, uint32_t* ids,
        GeofenceOption* options) {
    if (count >0 && mLocationApi) {
        mLocationApi->modifyGeofences(count, ids, options);
    }
}
void LocHalDaemonClientHandler::pauseGeofences(size_t count, uint32_t* ids) {
    if (count > 0 && mLocationApi) {
        mLocationApi->pauseGeofences(count, ids);
    }
}
void LocHalDaemonClientHandler::resumeGeofences(size_t count, uint32_t* ids) {
    if (count > 0 && mLocationApi) {
        mLocationApi->resumeGeofences(count, ids);
    }
}

void LocHalDaemonClientHandler::pingTest() {

    if (nullptr != mIpcSender) {
        string pbStr;
        LocAPIPingTestIndMsg msg(SERVICE_NAME, &mService->mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPIPingTestIndMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::getAntennaInfo() {
    if (mLocationApi) {
        mLocationApi->getAntennaInfo(&mAntennaInfoCb);
    } else {
        LOC_LOGe("null mLocationApi");
    }
}

void LocHalDaemonClientHandler::cleanup(bool forceRemove) {
    // please do not attempt to hold the lock, as the caller of this function
    // already holds the lock

    // set the ptr to null to prevent further sending out message to the
    // remote client that is no longer reachable
    mIpcSender = nullptr;

    if (forceRemove && 0 != remove(mName.c_str())) {
        LOC_LOGw("<-- failed to remove file %s error %s", mName.c_str(), strerror(errno));
    }

    if (mLocationApi) {
        mLocationApi->destroy([this]() {onLocationApiDestroyCompleteCb();});
        mLocationApi = nullptr;
    } else {
        // For location integration api client handler, it does not
        // instantiate LocationApi interface and can be freed right away
        LOC_LOGe("delete LocHalDaemonClientHandler");
        delete this;
    }
}

/******************************************************************************
LocHalDaemonClientHandler - Location API response callback functions
******************************************************************************/
void LocHalDaemonClientHandler::onResponseCb(LocationError err, uint32_t id) {

    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);

    if (nullptr != mIpcSender) {
        LOC_LOGd("--< onResponseCb err=%u id=%u client %s", err, id, mName.c_str());

        ELocMsgID pendingMsgId = E_LOCAPI_UNDEFINED_MSG_ID;
        if (!mPendingMessages.empty()) {
            pendingMsgId = mPendingMessages.front();
            mPendingMessages.pop();
        }

        bool rc = false;
        ELocMsgID eLocMsgId = E_LOCAPI_UNDEFINED_MSG_ID;
        string pbStr;
        eLocMsgId = pendingMsgId;
        // send corresponding indication message if pending
        switch (pendingMsgId) {
            case E_LOCAPI_START_TRACKING_MSG_ID: {
                LOC_LOGd("<-- start resp err=%u id=%u pending=%u", err, id, pendingMsgId);
                break;
            }
            case E_LOCAPI_STOP_TRACKING_MSG_ID: {
                LOC_LOGd("<-- stop resp err=%u id=%u pending=%u", err, id, pendingMsgId);
                break;
            }
            case E_LOCAPI_UPDATE_TRACKING_OPTIONS_MSG_ID: {
                LOC_LOGd("<-- update resp err=%u id=%u pending=%u", err, id, pendingMsgId);
                break;
            }
            case E_LOCAPI_START_BATCHING_MSG_ID : {
                LOC_LOGd("<-- start batching resp err=%u id=%u pending=%u", err, id, pendingMsgId);
                break;
            }
            case E_LOCAPI_STOP_BATCHING_MSG_ID: {
                LOC_LOGd("<-- stop batching resp err=%u id=%u pending=%u", err, id, pendingMsgId);
                break;
            }
            case E_LOCAPI_UPDATE_BATCHING_OPTIONS_MSG_ID: {
                LOC_LOGd("<-- update batching options resp err=%u id=%u pending=%u", err, id,
                        pendingMsgId);
                break;
            }
            default: {
                LOC_LOGe("no pending message for %s", mName.c_str());
                return;
            }
        }

        LocAPIGenericRespMsg msg(SERVICE_NAME, eLocMsgId, err, &mService->mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            rc = sendMessage(pbStr.c_str(), pbStr.size(), eLocMsgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPIGenericRespMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::onCollectiveResponseCallback(
        size_t count, LocationError *errs, uint32_t *ids) {
    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onCollectiveResponseCallback, client name %s, count: %zu", mName.c_str(), count);

    if (nullptr == mIpcSender) {
        LOC_LOGe("mIpcSender is null");
        return;
    }

    ELocMsgID pendingMsgId = E_LOCAPI_UNDEFINED_MSG_ID;
    if (!mGfPendingMessages.empty()) {
        pendingMsgId = mGfPendingMessages.front();
        mGfPendingMessages.pop();
    }
    if (0 == count) {
        LOC_LOGd("count is 0");
        return;
    }

    uint32_t* clientIds = getClientIds(count, ids);
    if (nullptr == clientIds) {
        LOC_LOGe("clientIds is null");
        return;
    }

    // serialize LocationError and ids into ipc message payload
    // send corresponding indication message if pending
    ELocMsgID eLocMsgId  = E_LOCAPI_UNDEFINED_MSG_ID;
    switch (pendingMsgId) {
        case E_LOCAPI_ADD_GEOFENCES_MSG_ID: {
            LOC_LOGd("<-- addGeofence resp pending=%u", pendingMsgId);
            eLocMsgId = E_LOCAPI_ADD_GEOFENCES_MSG_ID;
            break;
        }
        case E_LOCAPI_REMOVE_GEOFENCES_MSG_ID: {
            LOC_LOGd("<-- removeGeofence resp pending=%u", pendingMsgId);
            eLocMsgId = E_LOCAPI_REMOVE_GEOFENCES_MSG_ID;
            eraseGeofenceIds(count, clientIds);
            break;
        }
        case E_LOCAPI_MODIFY_GEOFENCES_MSG_ID: {
            LOC_LOGd("<-- modifyGeofence resp pending=%u", pendingMsgId);
            eLocMsgId = E_LOCAPI_MODIFY_GEOFENCES_MSG_ID;
            break;
        }
        case E_LOCAPI_PAUSE_GEOFENCES_MSG_ID: {
            LOC_LOGd("<-- pauseGeofence resp pending=%u", pendingMsgId);
            eLocMsgId = E_LOCAPI_PAUSE_GEOFENCES_MSG_ID;
            break;
        }
        case E_LOCAPI_RESUME_GEOFENCES_MSG_ID: {
            LOC_LOGd("<-- resumeGeofence resp pending=%u", pendingMsgId);
            eLocMsgId = E_LOCAPI_RESUME_GEOFENCES_MSG_ID;
            break;
        }
        default: {
            LOC_LOGe("no pending geofence message for %s", mName.c_str());
            free(clientIds);
            return;
        }
    }

    LocAPICollectiveRespMsg msg(SERVICE_NAME, eLocMsgId, &mService->mPbufMsgConv);
    string pbStr;
    uint32_t collctResCount = count;
    LOC_LOGd("Gf Resp count: %u", collctResCount);
    for (uint32_t i=0; i < collctResCount; i++) {
        GeofenceResponse gfResp;
        gfResp.clientId = clientIds[i];
        gfResp.error = errs[i];
        msg.collectiveRes.resp.push_back(gfResp);
        if (errs[i] != LOCATION_ERROR_SUCCESS) {
            eraseGeofenceIds(1, &clientIds[i]);
        }
    }

    if (msg.serializeToProtobuf(pbStr)) {
        bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
        // purge this client if failed
        if (!rc) {
            LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
            mService->deleteClientbyName(mName);
        }
    } else {
        LOC_LOGe("LocAPICollectiveRespMsg serializeToProtobuf failed");
    }

    free(clientIds);
}


/******************************************************************************
LocHalDaemonClientHandler - Location Control API response callback functions
******************************************************************************/
void LocHalDaemonClientHandler::onControlResponseCb(LocationError err, ELocMsgID msgId) {
    // no need to hold the lock, as lock is already held at the caller
    if (nullptr != mIpcSender) {
        LOC_LOGi("--< onControlResponseCb err=%u msgId=%u, client name=%s",
                 err, msgId, mName.c_str());
        string pbStr;
        LocAPIGenericRespMsg msg(SERVICE_NAME, msgId, err, &mService->mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPIGenericRespMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::sendTerrestrialFix(LocationError error,
                                                   const Location& location) {
    LocAPIGetSingleTerrestrialPosRespMsg msg(SERVICE_NAME,
            error, location,  &mService->mPbufMsgConv);

    const char* msgStream = nullptr;
    size_t msgLen = 0;
    string pbStr;
    if (msg.serializeToProtobuf(pbStr)) {
        msgStream = pbStr.c_str();
        msgLen = pbStr.size();
        sendMessage(msgStream, msgLen, E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_RESP_MSG_ID);
    }
}

void LocHalDaemonClientHandler::sendSingleFusedFix(LocationError error,
                                                   const Location& location) {
    LocAPIGetSinglePosRespMsg msg(SERVICE_NAME, error,
                                  location, &mService->mPbufMsgConv);

    const char* msgStream = nullptr;
    size_t msgLen = 0;
    string pbStr;
    if (msg.serializeToProtobuf(pbStr)) {
        msgStream = pbStr.c_str();
        msgLen = pbStr.size();
        sendMessage(msgStream, msgLen, E_LOCAPI_GET_SINGLE_POS_RESP_MSG_ID);
    }
}

void LocHalDaemonClientHandler::onGnssConfigCb(ELocMsgID configMsgId,
                                               const GnssConfig & gnssConfig) {
    string pbStr;

    switch (configMsgId) {
    case E_INTAPI_GET_ROBUST_LOCATION_CONFIG_REQ_MSG_ID:
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_ROBUST_LOCATION_BIT) {
            LocConfigGetRobustLocationConfigRespMsg msg(SERVICE_NAME,
                    gnssConfig.robustLocationConfig,
                    &mService->mPbufMsgConv);
            msg.serializeToProtobuf(pbStr);
        }
        break;
    case E_INTAPI_GET_MIN_GPS_WEEK_REQ_MSG_ID:
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_MIN_GPS_WEEK_BIT) {
            LOC_LOGd("--< onGnssConfigCb, minGpsWeek = %d", gnssConfig.minGpsWeek);
            LocConfigGetMinGpsWeekRespMsg msg(SERVICE_NAME, gnssConfig.minGpsWeek,
                    &mService->mPbufMsgConv);
            msg.serializeToProtobuf(pbStr);
        }
        break;
    case E_INTAPI_GET_MIN_SV_ELEVATION_REQ_MSG_ID:
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_MIN_SV_ELEVATION_BIT) {
            LOC_LOGd("--< onGnssConfigCb, minSvElevation = %d", gnssConfig.minSvElevation);
            LocConfigGetMinSvElevationRespMsg msg(SERVICE_NAME, gnssConfig.minSvElevation,
                    &mService->mPbufMsgConv);
            msg.serializeToProtobuf(pbStr);
        }
        break;

    case E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_REQ_MSG_ID:
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_CONSTELLATION_SECONDARY_BAND_BIT)
        {
            LocConfigGetConstellationSecondaryBandConfigRespMsg msg(SERVICE_NAME,
                    gnssConfig.secondaryBandConfig, &mService->mPbufMsgConv);
            msg.serializeToProtobuf(pbStr);
        }
        break;

    case E_INTAPI_GET_XTRA_STATUS_REQ_MSG_ID:
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_XTRA_STATUS_BIT)
        {
            LOC_LOGd("--< onGnssConfigCb, xtra status received, %d %d %d",
                     gnssConfig.xtraStatus.featureEnabled,
                     gnssConfig.xtraStatus.xtraDataStatus,
                     gnssConfig.xtraStatus.xtraValidForHours);
            LocConfigGetXtraStatusRespMsg msg(SERVICE_NAME,
                                              XTRA_STATUS_UPDATE_UPON_QUERY,
                                              gnssConfig.xtraStatus,
                                              &mService->mPbufMsgConv);
            msg.serializeToProtobuf(pbStr);
        }
        break;

    case E_INTAPI_REGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID:
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_XTRA_STATUS_BIT)
        {
            LocConfigGetXtraStatusRespMsg msg(SERVICE_NAME,
                                              XTRA_STATUS_UPDATE_UPON_REGISTRATION,
                                              gnssConfig.xtraStatus,
                                              &mService->mPbufMsgConv);
            msg.serializeToProtobuf(pbStr);
        }
        break;

    default:
        break;
    }

    if ((nullptr != mIpcSender) && (pbStr.size() != 0)) {
        bool rc = sendMessage(pbStr.c_str(), pbStr.size(), configMsgId);
        // purge this client if failed
        if (!rc) {
            LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
            mService->deleteClientbyName(mName);
        }
    } else {
        LOC_LOGe("mIpcSender or msgStream is null!!");
    }
}

void LocHalDaemonClientHandler::onXtraStatusUpdateCb(const XtraStatus &xtraStatus) {
    string pbStr;

    LocConfigGetXtraStatusRespMsg msg(SERVICE_NAME, XTRA_STATUS_UPDATE_UPON_STATUS_CHANGE,
                                      xtraStatus, &mService->mPbufMsgConv);
    msg.serializeToProtobuf(pbStr);
    if ((nullptr != mIpcSender) && (pbStr.size() != 0)) {
        bool rc = sendMessage(pbStr.c_str(), pbStr.size(), E_INTAPI_GET_XTRA_STATUS_RESP_MSG_ID );
        // purge this client if failed
        if (!rc) {
            LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
            mService->deleteClientbyName(mName);
        }
    } else {
        LOC_LOGe("mIpcSender or msgStream is null!! size %d", pbStr.size());
    }
}

/******************************************************************************
LocHalDaemonClientHandler - Location API callback functions
******************************************************************************/
void LocHalDaemonClientHandler::onCapabilitiesCallback(LocationCapabilitiesMask mask) {

    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGi("--< onCapabilitiesCallback client name %s, "
             "new mask=0x%" PRIx64", old capability is 0x%" PRIx64"",
             mName.c_str(), mask, mCapabilityMask);

    if (mask != mCapabilityMask) {
        mCapabilityMask = mask;
        sendCapabilitiesMsg();
    }
}

void LocHalDaemonClientHandler::onTrackingCb(const Location& location) {

    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onTrackingCb, client name %s, ipc valid %d, sub mask 0x%x", mName.c_str(),
             (nullptr != mIpcSender), mSubscriptionMask);

    if ((nullptr != mIpcSender) &&
        (mSubscriptionMask & (E_LOC_CB_TRACKING_BIT | E_LOC_CB_SIMPLE_LOCATION_INFO_BIT))) {
        // broadcast
        string pbStr;
        LocAPILocationIndMsg msg(SERVICE_NAME, location, &mService->mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPILocationIndMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::onBatchingCb(size_t count, Location* location,
        const BatchingOptions& batchOptions) {
    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onBatchingCb, client name %s", mName.c_str());

    if ((nullptr != mIpcSender) && (mSubscriptionMask & E_LOC_CB_BATCHING_BIT)) {
        if (0 == count) {
            return;
        }

        // serialize locations in batch into ipc message payload
        LocAPIBatchingIndMsg msg(SERVICE_NAME, &mService->mPbufMsgConv);
        LOC_LOGd("Batch count: %u", (uint32_t)count);
        msg.batchNotification.status = BATCHING_STATUS_POSITION_AVAILABE;
        msg.batchingMode = batchOptions.batchingMode;
        for (int i = 0; i < count; i++) {
            msg.batchNotification.location.push_back(location[i]);
        }

        string pbStr;
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), E_LOCAPI_BATCHING_MSG_ID);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPIBatchingIndMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::onBatchingStatusCb(const BatchingStatusInfo& batchingStatus,
                std::list<uint32_t>& listOfCompletedTrips) {
    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onBatchingStatusCb");
    if ((nullptr != mIpcSender) && (mSubscriptionMask & E_LOC_CB_BATCHING_STATUS_BIT) &&
                (BATCHING_MODE_TRIP == mBatchingMode) &&
                (BATCHING_STATUS_TRIP_COMPLETED == batchingStatus.batchingStatus)) {
        // For trip batching, notify client to stop session when BATCHING_STATUS_TRIP_COMPLETED
        LocAPIBatchNotification batchNotif = {};
        batchNotif.status = BATCHING_STATUS_TRIP_COMPLETED;
        string pbStr;
        LocAPIBatchingIndMsg msg(SERVICE_NAME, batchNotif, mBatchingMode, &mService->mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPIBatchingIndMsg serializeToProtobuf failed");
        }

    }
}

void LocHalDaemonClientHandler::onGeofenceBreachCb(
        const GeofenceBreachNotification& gfBreachNotif) {
    LOC_LOGd("--< onGeofenceBreachCallback");
    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);

    if ((nullptr != mIpcSender) &&
            (mSubscriptionMask & E_LOC_CB_GEOFENCE_BREACH_BIT)) {

        uint32_t* clientIds = getClientIds(gfBreachNotif.count, gfBreachNotif.ids);
        if (nullptr == clientIds) {
            LOC_LOGe("Failed to alloc %zu bytes",
                     sizeof(uint32_t) * gfBreachNotif.count);
            return;
        }
        // serialize GeofenceBreachNotification into ipc message payload
        LocAPIGeofenceBreachIndMsg msg(SERVICE_NAME, &mService->mPbufMsgConv);
        LOC_LOGd("Gf Breach Notif count: %u, breachType: %d", gfBreachNotif.count,
                gfBreachNotif.type);
        msg.gfBreachNotification.timestamp = gfBreachNotif.timestamp;
        msg.gfBreachNotification.location = gfBreachNotif.location;
        msg.gfBreachNotification.type = parseClientGeofenceBreachType(gfBreachNotif.type);
        for (uint32_t i = 0; i < gfBreachNotif.count; i++) {
            msg.gfBreachNotification.id.push_back(clientIds[i]);
        }

        string pbStr;
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), E_LOCAPI_GEOFENCE_BREACH_MSG_ID);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPIGeofenceBreachIndMsg serializeToProtobuf failed");
        }

        free(clientIds);
    }
}

void LocHalDaemonClientHandler::onGnssLocationInfoCb(
        const GnssLocationInfoNotification& notification) {

    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onGnssLocationInfo, client name %s, ipc valid %d, sub mask 0x%x", mName.c_str(),
             (nullptr != mIpcSender), mSubscriptionMask);

    if ((nullptr != mIpcSender) &&
        (mSubscriptionMask & E_LOC_CB_GNSS_LOCATION_INFO_BIT)) {
        string pbStr;
        LocAPILocationInfoIndMsg msg(SERVICE_NAME, notification, &mService->mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPILocationInfoIndMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::onEngLocationsInfoCb(
        uint32_t count,
        GnssLocationInfoNotification* engLocationsInfoNotification
) {
    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onEngLocationInfoCb count: %d, locReqEngTypeMask 0x%x,"
             "client name %s, ipc valid %d, sub mask 0x%x",
             count, mOptions.locReqEngTypeMask, mName.c_str(),
             (nullptr != mIpcSender), mSubscriptionMask);

    if ((nullptr != mIpcSender) &&
        (mSubscriptionMask & E_LOC_CB_ENGINE_LOCATIONS_INFO_BIT)) {

        int reportCount = 0;
        GnssLocationInfoNotification engineLocationInfoNotification[LOC_OUTPUT_ENGINE_COUNT];
        memset(engineLocationInfoNotification, 0, sizeof(engineLocationInfoNotification));
        for (int i = 0; i < count; i++) {
            GnssLocationInfoNotification* locPtr = engLocationsInfoNotification+i;

            LOC_LOGv("--< onEngLocationInfoCb i %d, type %d", i, locPtr->locOutputEngType);
            if (((locPtr->locOutputEngType == LOC_OUTPUT_ENGINE_FUSED) &&
                 (mOptions.locReqEngTypeMask & LOC_REQ_ENGINE_FUSED_BIT)) ||
                ((locPtr->locOutputEngType == LOC_OUTPUT_ENGINE_SPE) &&
                 (mOptions.locReqEngTypeMask & LOC_REQ_ENGINE_SPE_BIT)) ||
                ((locPtr->locOutputEngType == LOC_OUTPUT_ENGINE_PPE) &&
                 (mOptions.locReqEngTypeMask & LOC_REQ_ENGINE_PPE_BIT )) ||
                ((locPtr->locOutputEngType == LOC_OUTPUT_ENGINE_VPE) &&
                 (mOptions.locReqEngTypeMask & LOC_REQ_ENGINE_VPE_BIT))) {
                engineLocationInfoNotification[reportCount++] = *locPtr;
            }
        }

        if (reportCount > 0 ) {
            string pbStr;
            LocAPIEngineLocationsInfoIndMsg msg(SERVICE_NAME, reportCount,
                                                engineLocationInfoNotification,
                                                &mService->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
                // purge this client if failed
                if (!rc) {
                    LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                    mService->deleteClientbyName(mName);
                }
            } else {
                LOC_LOGe("LocAPIEngineLocationsInfoIndMsg serializeToProtobuf failed");
            }
        }
    }
}

void LocHalDaemonClientHandler::onGnssNiCb(uint32_t id, GnssNiNotification gnssNiNotification) {

    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onGnssNiCb");
}

void LocHalDaemonClientHandler::onGnssSvCb(const GnssSvNotification &notification) {
    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onGnssSvCb, client name %s, ipc valid %d, sub mask 0x%x",
             mName.c_str(), (nullptr != mIpcSender), mSubscriptionMask);

    if ((nullptr != mIpcSender) &&
            (mSubscriptionMask & E_LOC_CB_GNSS_SV_BIT)) {
        // broadcast
        string pbStr;
        LocAPISatelliteVehicleIndMsg msg(SERVICE_NAME, notification, &mService->mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPISatelliteVehicleIndMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::onGnssNmeaCb(const GnssNmeaNotification& notification) {

    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);

    LOC_LOGd("--< onGnssNmeaCb, client name %s, ipc valid %d, sub mask 0x%x",
             mName.c_str(), (nullptr != mIpcSender), mSubscriptionMask);

    if ((nullptr != mIpcSender) &&
            (mSubscriptionMask & E_LOC_CB_GNSS_NMEA_BIT) &&
            ((notification.locOutputEngType == LOC_OUTPUT_ENGINE_FUSED) || notification.isSvNmea)) {
        LOC_LOGd("--< onGnssNmeaCb[%s] t=%" PRIu64" l=%zu engType=%u isSvNmea=%u nmea=%s",
                mName.c_str(),
                notification.timestamp,
                notification.length,
                notification.locOutputEngType,
                notification.isSvNmea,
                notification.nmea);
        // serialize nmea string into ipc message payload
        string nmeaStr(notification.nmea, notification.length);
        LocAPINmeaIndMsg msg(SERVICE_NAME, &mService->mPbufMsgConv);
        msg.gnssNmeaNotification.timestamp = notification.timestamp;
        msg.gnssNmeaNotification.nmea = nmeaStr;
        msg.gnssNmeaNotification.locOutputEngType = notification.locOutputEngType;

        string pbStr;
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), E_LOCAPI_NMEA_MSG_ID);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPINmeaIndMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::onEngineNmeaCb(const GnssNmeaNotification& notification) {

    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);

    LOC_LOGd("--< onEngineNmeaCb, client name %s, ipc valid %d, sub mask 0x%x",
             mName.c_str(), (nullptr != mIpcSender), mSubscriptionMask);

    if ((nullptr != mIpcSender) &&
            (mSubscriptionMask & E_LOC_CB_ENGINE_NMEA_BIT) &&
            (notification.isSvNmea ||
            ((notification.locOutputEngType == LOC_OUTPUT_ENGINE_FUSED) &&
            (mOptions.locReqEngTypeMask & LOC_REQ_ENGINE_FUSED_BIT)) ||
            ((notification.locOutputEngType == LOC_OUTPUT_ENGINE_SPE) &&
            (mOptions.locReqEngTypeMask & LOC_REQ_ENGINE_SPE_BIT)) ||
            ((notification.locOutputEngType == LOC_OUTPUT_ENGINE_PPE) &&
            (mOptions.locReqEngTypeMask & LOC_REQ_ENGINE_PPE_BIT)))) {
        LOC_LOGd("--< onEngineNmeaCb[%s] t=%" PRIu64" l=%zu engType=%u isSvNmea=%u nmea=%s",
                mName.c_str(),
                notification.timestamp,
                notification.length,
                notification.locOutputEngType,
                notification.isSvNmea,
                notification.nmea);
        // serialize nmea string into ipc message payload
        string nmeaStr(notification.nmea, notification.length);
        LocAPINmeaIndMsg msg(SERVICE_NAME, &mService->mPbufMsgConv);
        msg.gnssNmeaNotification.timestamp = notification.timestamp;
        msg.gnssNmeaNotification.nmea = nmeaStr;
        msg.gnssNmeaNotification.locOutputEngType = notification.locOutputEngType;

        string pbStr;
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), E_LOCAPI_NMEA_MSG_ID);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPINmeaIndMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::onGnssDataCb(const GnssDataNotification& notification) {

    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onGnssDataCb, client name %s, ipc valid %d, sub mask 0x%x",
             mName.c_str(), (nullptr != mIpcSender), mSubscriptionMask);

    if ((nullptr != mIpcSender) &&
            (mSubscriptionMask & E_LOC_CB_GNSS_DATA_BIT)) {
        for (int sig = 0; sig < GNSS_LOC_MAX_NUMBER_OF_SIGNAL_TYPES; sig++) {
            if (GNSS_LOC_DATA_JAMMER_IND_BIT ==
                (notification.gnssDataMask[sig] & GNSS_LOC_DATA_JAMMER_IND_BIT)) {
                LOC_LOGv("jammerInd[%d]=%f", sig, notification.jammerInd[sig]);
            }
            if (GNSS_LOC_DATA_AGC_BIT ==
                (notification.gnssDataMask[sig] & GNSS_LOC_DATA_AGC_BIT)) {
                LOC_LOGv("agc[%d]=%f", sig, notification.agc[sig]);
            }
        }

        string pbStr;
        LocAPIDataIndMsg msg(SERVICE_NAME, notification, &mService->mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            LOC_LOGv("Sending data message");
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPIDataIndMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::onGnssMeasurementsCb(
        const GnssMeasurementsNotification &notification) {
    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onGnssMeasurementsCb, client name %s, ipc valid %d, sub mask 0x%x",
             mName.c_str(), (nullptr != mIpcSender), mSubscriptionMask);

    if ((nullptr != mIpcSender) &&
            (mSubscriptionMask & (E_LOC_CB_GNSS_MEAS_BIT | E_LOC_CB_GNSS_NHZ_MEAS_BIT))) {
        string pbStr;
        LocAPIMeasIndMsg msg(SERVICE_NAME, notification, &mService->mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            LOC_LOGv("Sending meas message");
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPIMeasIndMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::onLocationSystemInfoCb(const LocationSystemInfo& notification) {

    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onLocationSystemInfoCb, client name %s, ipc valid %d, sub mask 0x%x",
             mName.c_str(), (nullptr != mIpcSender), mSubscriptionMask);

    if ((nullptr != mIpcSender) &&
            (mSubscriptionMask & E_LOC_CB_SYSTEM_INFO_BIT)) {
        string pbStr;
        LocAPILocationSystemInfoIndMsg msg(SERVICE_NAME, notification, &mService->mPbufMsgConv);
        LOC_LOGv("Sending location system info message");
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPILocationSystemInfoIndMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::onDcReportCb(const GnssDcReportInfo& dcReportInfo) {
    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onDcReportCb, client name %s, ipc valid %d, sub mask 0x%x",
             mName.c_str(), (nullptr != mIpcSender), mSubscriptionMask);

    if ((nullptr != mIpcSender) && (mSubscriptionMask & E_LOC_CB_GNSS_DC_REPORT_BIT)) {
        string pbStr;
        LocAPIDcReportIndMsg msg(SERVICE_NAME, dcReportInfo, &mService->mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPIDcReportIndMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::onGnssSignalTypesCb(GnssSignalTypeMask signalType) {
    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< client name %s, ipc valid %d, supported GNSS signal types 0x%x",
             mName.c_str(), (nullptr != mIpcSender), signalType);

    if (nullptr != mIpcSender) {
        string pbStr;
        LocConfigRegisterGnssSignalTypesUpdateRespMsg msg(SERVICE_NAME, signalType,
                &mService->mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("serializeToProtobuf failed, resp size: %d", pbStr.size());
        }
    }
}

void LocHalDaemonClientHandler::onLocationApiDestroyCompleteCb() {
    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);

    LOC_LOGe("delete LocHalDaemonClientHandler, client name %s", mName.c_str());
    delete this;
    // PLEASE NOTE: no more code after this, including print for class variable
}

void LocHalDaemonClientHandler::getDebugReport() {

    if (nullptr != mIpcSender) {
        string pbStr;
        GnssDebugReport report;

        mLocationApi->getDebugReport(report);
        LocAPIGetDebugRespMsg msg(SERVICE_NAME, report, &mService->mPbufMsgConv);
        LOC_LOGv("Sending LocAPIGetDebugRespMsg to %s", mName.c_str());
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPIGetDebugRespMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::onAntennaInfoCb(
        std::vector<GnssAntennaInformation>& gnssAntennaInformations) {

    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    if (nullptr != mIpcSender) {
        string pbStr;
        AntennaInformation antennaInfo;
        antennaInfo.antennaInfos = gnssAntennaInformations;
        LocAPIAntennaInfoMsg msg(SERVICE_NAME, antennaInfo, &mService->mPbufMsgConv);
        LOC_LOGv("Sending LocAPIAntennaInfoMsg to %s", mName.c_str());
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPIAntennaInfoMsg serializeToProtobuf failed");
        }
    }
}

/******************************************************************************
LocHalDaemonClientHandler - Engine info related functionality
******************************************************************************/
// called to deliver GNSS energy consumed info to the requesting client
// as this is single shot request, the corresponding mask will be cleared
// as well
void LocHalDaemonClientHandler::onGnssEnergyConsumedInfoAvailable(
        LocAPIGnssEnergyConsumedIndMsg &msg) {
    // no need to hold the lock as lock is already held by the caller
    if ((nullptr != mIpcSender) &&
            (mEngineInfoRequestMask & E_ENGINE_INFO_CB_GNSS_ENERGY_CONSUMED_BIT)) {
        string pbStr;
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            mEngineInfoRequestMask &= ~E_ENGINE_INFO_CB_GNSS_ENERGY_CONSUMED_BIT;

            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPIGnssEnergyConsumedIndMsg serializeToProtobuf failed");
        }
    }
}

void LocHalDaemonClientHandler::sendCapabilitiesMsg() {
    LOC_LOGd("--< sendCapabilitiesMsg=0x%" PRIx64, mCapabilityMask);

    if ((nullptr != mIpcSender) && (0 != mCapabilityMask)) {
        string pbStr;
        LocAPICapabilitiesIndMsg msg(SERVICE_NAME, mCapabilityMask, &mService->mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPICapabilitiesIndMsg serializeToProtobuf failed");
        }
    } else {
        LOC_LOGe("mIpcSender is NULL or mCapabilityMask is %d", mCapabilityMask);
    }
}

// return true if the client has pending request to retrieve
// GNSS energy consumed
bool LocHalDaemonClientHandler::hasPendingEngineInfoRequest(uint32_t mask) {
    if (mEngineInfoRequestMask & E_ENGINE_INFO_CB_GNSS_ENERGY_CONSUMED_BIT) {
        return true;
    } else {
        return false;
    }
}

// set up the bit to indicating the engine info request
// is pending.
void LocHalDaemonClientHandler::addEngineInfoRequst(uint32_t mask) {
    mEngineInfoRequestMask |= E_ENGINE_INFO_CB_GNSS_ENERGY_CONSUMED_BIT;
}

// Round input TBF to 100ms, 200ms, 500ms, and integer senconds that engine supports
// input tbf < 200 msec, round to 100 msec, else
// input tbf < 500 msec, round to 200 msec, else
// input tbf < 1000 msec, round to 500 msec, else
// round up input tbf to the closet integer seconds
uint32_t LocHalDaemonClientHandler::getSupportedTbf(uint32_t tbfMsec) {
    uint32_t supportedTbfMsec = 0;

    if (tbfMsec < 200) {
        supportedTbfMsec = 100;
    } else if (tbfMsec < 500) {
        supportedTbfMsec = 200;
    } else if (tbfMsec < 1000) {
        supportedTbfMsec = 500;
    } else {
        if (tbfMsec > (UINT32_MAX - 999)) {
            supportedTbfMsec = UINT32_MAX / 1000 * 1000;
        } else {
            // round up to the next integer second
            supportedTbfMsec = (tbfMsec+999) / 1000 * 1000;
        }
    }

    return supportedTbfMsec;
}

static GeofenceBreachTypeMask parseClientGeofenceBreachType(GeofenceBreachType type) {
    GeofenceBreachTypeMask mask = 0;
    switch (type) {
        case GEOFENCE_BREACH_ENTER:
            mask |= GEOFENCE_BREACH_ENTER_BIT;
            break;
        case GEOFENCE_BREACH_EXIT:
            mask |= GEOFENCE_BREACH_EXIT_BIT;
            break;
        case GEOFENCE_BREACH_DWELL_IN:
            mask |= GEOFENCE_BREACH_DWELL_IN_BIT;
            break;
        case GEOFENCE_BREACH_DWELL_OUT:
            mask |= GEOFENCE_BREACH_DWELL_OUT_BIT;
            break;
        default:
            mask = 0;
    }
    return mask;
}

void LocHalDaemonClientHandler::onGnssSvEphemerisCb(
        const GnssSvEphemerisReport &notification) {
    std::lock_guard<std::recursive_mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onGnssSvEphemerisCb, client name %s, ipc valid %d ",
             mName.c_str(), (nullptr != mIpcSender));

    if ((nullptr != mIpcSender) &&
            (mSubscriptionMask & (E_LOC_CB_GNSS_EPH_BIT))) {
        string pbStr;
        LocAPIEphIndMsg msg(SERVICE_NAME, notification, &mService->mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            LOC_LOGv("Sending eph message");
            bool rc = sendMessage(pbStr.c_str(), pbStr.size(), msg.msgId);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        } else {
            LOC_LOGe("LocAPIEphIndMsg serializeToProtobuf failed");
        }
    }
}
