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

#define LOG_TAG "LocSvc_LocationIntegrationApiImpl"

#include <sys/types.h>
#include <unistd.h>
#include <loc_cfg.h>
#include <LocationIntegrationApiImpl.h>
#include <log_util.h>
#include <gps_extended_c.h>
#include <inttypes.h>
#include <loc_misc_utils.h>

static uint32_t sXtraTestEnabled = 0;
static uint32_t sSleepTime = 800000;
static const loc_param_s_type gConfigTable[] = {
    {"XTRA_TEST_ENABLED", &sXtraTestEnabled, NULL, 'n'},
    {"QRTRWATCHER_DELAY_MICROSECOND", &sSleepTime, NULL, 'n'}
};

namespace location_integration {

/******************************************************************************
Utilities
******************************************************************************/

static LocConfigTypeEnum getLocConfigTypeFromMsgId(ELocMsgID  msgId) {
    LocConfigTypeEnum configType = (LocConfigTypeEnum) 0;
    switch (msgId) {
    case E_INTAPI_CONFIG_CONSTRAINTED_TUNC_MSG_ID:
        configType = CONFIG_CONSTRAINED_TIME_UNCERTAINTY;
        break;
    case E_INTAPI_CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR_MSG_ID:
        configType = CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR;
        break;
    case E_INTAPI_CONFIG_SV_CONSTELLATION_MSG_ID:
        configType = CONFIG_CONSTELLATIONS;
        break;
    case E_INTAPI_CONFIG_CONSTELLATION_SECONDARY_BAND_MSG_ID:
        configType = CONFIG_CONSTELLATION_SECONDARY_BAND;
        break;
    case E_INTAPI_CONFIG_AIDING_DATA_DELETION_MSG_ID:
        configType = CONFIG_AIDING_DATA_DELETION;
        break;
    case E_INTAPI_CONFIG_LEVER_ARM_MSG_ID:
        configType = CONFIG_LEVER_ARM;
        break;
    case E_INTAPI_CONFIG_ROBUST_LOCATION_MSG_ID:
        configType = CONFIG_ROBUST_LOCATION;
        break;
    case E_INTAPI_CONFIG_MIN_GPS_WEEK_MSG_ID:
        configType = CONFIG_MIN_GPS_WEEK;
        break;
    case E_INTAPI_CONFIG_DEAD_RECKONING_ENGINE_MSG_ID:
        configType = CONFIG_DEAD_RECKONING_ENGINE;
        break;
    case E_INTAPI_CONFIG_MIN_SV_ELEVATION_MSG_ID:
        configType = CONFIG_MIN_SV_ELEVATION;
        break;
    case E_INTAPI_CONFIG_ENGINE_RUN_STATE_MSG_ID:
        configType = CONFIG_ENGINE_RUN_STATE;
        break;
    case E_INTAPI_CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING_MSG_ID:
        configType = CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING;
        break;
    case E_INTAPI_CONFIG_OUTPUT_NMEA_TYPES_MSG_ID:
        configType = CONFIG_OUTPUT_NMEA_TYPES;
        break;
    case E_INTAPI_CONFIG_ENGINE_INTEGRITY_RISK_MSG_ID:
        configType = CONFIG_ENGINE_INTEGRITY_RISK;
        break;
    case E_INTAPI_CONFIG_XTRA_PARAMS_MSG_ID:
        configType = CONFIG_XTRA_PARAMS;
        break;
    case E_INTAPI_CONFIG_MERKLE_TREE_MSG_ID:
        configType = CONFIG_MERKLE_TREE;
        break;
    case E_INTAPI_CONFIG_OSNMA_ENABLEMENT_MSG_ID:
        configType = CONFIG_OSNMA_ENABLEMENT;
        break;
    case E_INTAPI_GET_ROBUST_LOCATION_CONFIG_REQ_MSG_ID:
    case E_INTAPI_GET_ROBUST_LOCATION_CONFIG_RESP_MSG_ID:
        configType = GET_ROBUST_LOCATION_CONFIG;
        break;
    case E_INTAPI_GET_MIN_GPS_WEEK_REQ_MSG_ID:
    case E_INTAPI_GET_MIN_GPS_WEEK_RESP_MSG_ID:
        configType = GET_MIN_GPS_WEEK;
        break;
    case E_INTAPI_GET_MIN_SV_ELEVATION_REQ_MSG_ID:
    case E_INTAPI_GET_MIN_SV_ELEVATION_RESP_MSG_ID:
        configType = GET_MIN_SV_ELEVATION;
        break;
    case E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_REQ_MSG_ID:
    case E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_RESP_MSG_ID:
        configType = GET_CONSTELLATION_SECONDARY_BAND_CONFIG;
        break;
    case E_INTAPI_GET_XTRA_STATUS_REQ_MSG_ID:
    case E_INTAPI_GET_XTRA_STATUS_RESP_MSG_ID:
        configType = GET_XTRA_STATUS;
        break;
    case E_INTAPI_REGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID:
    case E_INTAPI_DEREGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID:
        configType = REGISTER_XTRA_STATUS_UPDATE;
        break;
    case E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_REQ_MSG_ID:
    case E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_RESP_MSG_ID:
        configType = REGISTER_SIGNAL_TYPES_UPDATE;
        break;
    default:
        break;
    }
    return configType;
}

static LocIntegrationResponse getLocIntegrationResponse(::LocationError error) {
    LocIntegrationResponse response;

    switch (error) {
        case LOCATION_ERROR_SUCCESS:
            response = LOC_INT_RESPONSE_SUCCESS;
            break;
        case LOCATION_ERROR_NOT_SUPPORTED:
            response = LOC_INT_RESPONSE_NOT_SUPPORTED;
            break;
        case LOCATION_ERROR_INVALID_PARAMETER:
            response = LOC_INT_RESPONSE_PARAM_INVALID;
            break;
        default:
            response = LOC_INT_RESPONSE_FAILURE;
            break;
    }

    return response;
}

/******************************************************************************
ILocIpcListener override
******************************************************************************/
class IpcListener : public ILocIpcListener {
    MsgTask& mMsgTask;
    LocationIntegrationApiImpl& mApiImpl;
    const SockNode::Type mSockTpye;
public:
    inline IpcListener(LocationIntegrationApiImpl& apiImpl, MsgTask& msgTask,
                       const SockNode::Type sockType) :
            mMsgTask(msgTask), mApiImpl(apiImpl), mSockTpye(sockType) {}
    virtual void onListenerReady() override;
    virtual void onReceive(const char* data, uint32_t length,
                           const LocIpcRecver* recver) override;
};

/******************************************************************************
LocIpcQrtrWatcher override
******************************************************************************/
class HalDaemonQrtrWatcher : public LocIpcQrtrWatcher {
    const weak_ptr<IpcListener> mIpcListener;
    const weak_ptr<LocIpcSender> mIpcSender;
    LocIpcQrtrWatcher::ServiceStatus mKnownStatus;
    LocationApiPbMsgConv mPbufMsgConv;
    MsgTask& mMsgTask;

public:
    inline HalDaemonQrtrWatcher(shared_ptr<IpcListener>& listener, shared_ptr<LocIpcSender>& sender,
                          LocationApiPbMsgConv& pbMsgConv, MsgTask& msgTask) :
            LocIpcQrtrWatcher({LOCATION_CLIENT_API_QSOCKET_HALDAEMON_SERVICE_ID}),
            mIpcListener(listener), mIpcSender(sender), mPbufMsgConv(pbMsgConv),
            mMsgTask(msgTask), mKnownStatus(LocIpcQrtrWatcher::ServiceStatus::DOWN) {
    }
    inline virtual void onServiceStatusChange(int serviceId, int instanceId,
            LocIpcQrtrWatcher::ServiceStatus status, uint32_t nodeId, uint32_t portId) {

        struct onHalServiceStatusChangeHandler : public LocMsg {
            onHalServiceStatusChangeHandler(HalDaemonQrtrWatcher& watcher,
                                            LocIpcQrtrWatcher::ServiceStatus status,
                                            uint32_t nodeId, uint32_t portId) :
                mWatcher(watcher), mStatus(status), mNodeId(nodeId), mPortId(portId) {}

            virtual ~onHalServiceStatusChangeHandler() {}
            void proc() const {
                if (LocIpcQrtrWatcher::ServiceStatus::UP == mStatus) {
                    LOC_LOGi("LocIpcQrtrWatcher:: HAL Daemon ServiceStatus::UP");
                    auto sender = mWatcher.mIpcSender.lock();
                    if (nullptr != sender && sender->updateDestAddr(mNodeId, mPortId)) {
                        usleep(sSleepTime);
                        auto listener = mWatcher.mIpcListener.lock();
                        if (nullptr != listener) {
                            LocAPIHalReadyIndMsg msg(SERVICE_NAME, &mWatcher.mPbufMsgConv);
                            string pbStr;
                            if (msg.serializeToProtobuf(pbStr)) {
                                listener->onReceive(pbStr.c_str(), pbStr.size(), nullptr);
                            } else {
                                LOC_LOGe("LocAPIHalReadyIndMsg serializeToProtobuf failed");
                            }
                        }
                    }
                }
                mWatcher.mKnownStatus = mStatus;
            }

            HalDaemonQrtrWatcher& mWatcher;
            LocIpcQrtrWatcher::ServiceStatus mStatus;
            uint32_t mNodeId;
            uint32_t mPortId;
        };

        if (LOCATION_CLIENT_API_QSOCKET_HALDAEMON_SERVICE_ID == serviceId &&
            LOCATION_CLIENT_API_QSOCKET_HALDAEMON_INSTANCE_ID == instanceId) {
            mMsgTask.sendMsg(new (nothrow)
                     onHalServiceStatusChangeHandler(*this, status, nodeId, portId));
        }
    }
};

/******************************************************************************
LocationIntegrationApiImpl
******************************************************************************/
mutex LocationIntegrationApiImpl::mMutex;
bool LocationIntegrationApiImpl::mClientRunning = false;

/******************************************************************************
LocationIntegrationApiImpl - constructors
******************************************************************************/
LocationIntegrationApiImpl::LocationIntegrationApiImpl(LocIntegrationCbs& integrationCbs) :
        mHalRegistered(false),
        mIntegrationCbs(integrationCbs),
        mTuncConfigInfo{},
        mPaceConfigInfo{},
        mSvConfigInfo{},
        mLeverArmConfigInfo{},
        mRobustLocationConfigInfo{},
        mDreConfigInfo{},
        mMsgTask("IntApiMsgTask"),
        mGtpUserConsentConfigInfo{},
        mNmeaConfigInfo{},
        mRegisterXtraUpdate(false),
        mXtraUpdateUponRegisterPending(false) {

    if (integrationClientAllowed() == false) {
        return;
    }

    // read configuration file
    UTIL_READ_CONF(LOC_PATH_GPS_CONF, gConfigTable);
    LOC_LOGd("sXtraTestEnabled=%u", sXtraTestEnabled);

    // get pid to generate sokect name
    uint32_t pid = (uint32_t)getpid();

#ifdef FEATURE_EXTERNAL_AP
    SockNodeEap sock(LOCATION_CLIENT_API_QSOCKET_CLIENT_SERVICE_ID,
                     pid * 100);
    size_t pathNameLength = (size_t) strlcpy(mSocketName, sock.getNodePathname().c_str(),
                                    sizeof(mSocketName));
    if (pathNameLength >= sizeof(mSocketName)) {
        LOC_LOGe("socket name length exceeds limit of %d bytes", sizeof(mSocketName));
        return;
    }
    // establish an ipc sender to the hal daemon
    mIpcSender = LocIpc::getLocIpcQrtrSender(LOCATION_CLIENT_API_QSOCKET_HALDAEMON_SERVICE_ID,
                                             LOCATION_CLIENT_API_QSOCKET_HALDAEMON_INSTANCE_ID);
    if (mIpcSender == nullptr) {
        LOC_LOGe("create sender socket failed for service id: %d instance id: %d",
                 LOCATION_CLIENT_API_QSOCKET_HALDAEMON_SERVICE_ID,
                 LOCATION_CLIENT_API_QSOCKET_HALDAEMON_INSTANCE_ID);
        return;
    }
    shared_ptr<IpcListener> listener(make_shared<IpcListener>(*this, mMsgTask, SockNode::Eap));
    unique_ptr<LocIpcRecver> recver = LocIpc::getLocIpcQrtrRecver(listener,
            sock.getId1(), sock.getId2(),
            make_shared<HalDaemonQrtrWatcher>(listener, mIpcSender, mPbufMsgConv, mMsgTask));
#else
    SockNodeLocal sock(LOCATION_INTEGRATION_API, pid, 0);
    size_t pathNameLength = (size_t) strlcpy(mSocketName, sock.getNodePathname().c_str(),
                                    sizeof(mSocketName));
    if (pathNameLength >= sizeof(mSocketName)) {
        LOC_LOGe("socket name length exceeds limit of %zu bytes", sizeof(mSocketName));
        return;
    }

    LOC_LOGd("create sender socket: %s", mSocketName);
    locUtilWaitForDir(SOCKET_LOC_CLIENT_DIR, "gps");

    // establish an ipc sender to the hal daemon
    mIpcSender = LocIpc::getLocIpcLocalSender(SOCKET_TO_LOCATION_HAL_DAEMON);
    if (mIpcSender == nullptr) {
        LOC_LOGe("create sender socket failed %s", SOCKET_TO_LOCATION_HAL_DAEMON);
        return;
    }
    unique_ptr<LocIpcRecver> recver = LocIpc::getLocIpcLocalRecver(
            make_shared<IpcListener>(*this, mMsgTask, SockNode::Local), mSocketName);
#endif //  FEATURE_EXTERNAL_AP

    LOC_LOGd("listen on socket: %s", mSocketName);
    mIpc.startNonBlockingListening(recver);
}

LocationIntegrationApiImpl::~LocationIntegrationApiImpl() {
}

void LocationIntegrationApiImpl::destroy() {

    struct DestroyReq : public LocMsg {
        DestroyReq(LocationIntegrationApiImpl* apiImpl) :
                mApiImpl(apiImpl) {}
        virtual ~DestroyReq() {}
        void proc() const {
            // deregister
            if (mApiImpl->mHalRegistered && (nullptr != mApiImpl->mIpcSender)) {
                string pbStr;
                LocAPIClientDeregisterReqMsg msg(mApiImpl->mSocketName, &mApiImpl->mPbufMsgConv);
                if (msg.serializeToProtobuf(pbStr)) {
                    bool rc = mApiImpl->sendMessage(
                            reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()), pbStr.size());
                    LOC_LOGd(">>> DeregisterReq rc=%d", rc);
                    mApiImpl->mIpcSender = nullptr;
                } else {
                    LOC_LOGe("LocAPIClientDeregisterReqMsg serializeToProtobuf failed");
                }

            }

            // delete the integration client, so another integration client can be set
            {
                lock_guard<mutex> lock(mMutex);
                mApiImpl->mClientRunning = false;
            }
            usleep(50000); //give 50ms for socket clean up

            delete mApiImpl;
        }
        LocationIntegrationApiImpl* mApiImpl;
    };

    mMsgTask.sendMsg(new (nothrow) DestroyReq(this));
    usleep(100000); //100ms for handling onReceive() messages
}

bool LocationIntegrationApiImpl::integrationClientAllowed() {
    lock_guard<mutex> lock(mMutex);

    // allow only one instance of integration client running per process
    if (!mClientRunning) {
        mClientRunning = true;
        return true;
    } else {
         LOC_LOGe("there is already a running location integration api client in the process");
         return false;
    }
}

/******************************************************************************
LocationIntegrationApiImpl -ILocIpcListener
******************************************************************************/
void IpcListener::onListenerReady() {
    struct ClientRegisterReq : public LocMsg {
        ClientRegisterReq(LocationIntegrationApiImpl& apiImpl) : mApiImpl(apiImpl) {}
        void proc() const {
            if (mApiImpl.sendClientRegMsgToHalDaemon()) {
                // lia is able to send msg to hal daemon,
                // send queued command to hal daemon if there is any
                mApiImpl.processQueuedReqs();
            }
        }
        LocationIntegrationApiImpl& mApiImpl;
    };
    if (SockNode::Local == mSockTpye) {
        if (0 != chown(mApiImpl.mSocketName, getuid(), GID_LOCCLIENT)) {
            LOC_LOGe("chown to group locclient failed %s", strerror(errno));
        }
    }
    mMsgTask.sendMsg(new (nothrow) ClientRegisterReq(mApiImpl));
}

void IpcListener::onReceive(const char* data, uint32_t length,
                            const LocIpcRecver* recver) {
    struct OnReceiveHandler : public LocMsg {
        OnReceiveHandler(LocationIntegrationApiImpl& apiImpl, IpcListener& listener,
                         const char* data, uint32_t length) :
                mApiImpl(apiImpl), mListener(listener), mMsgData(data, length) {}


        virtual ~OnReceiveHandler() {}
        void proc() const {
            // Protobuff Encoding enabled, so we need to convert the message from proto
            // encoded format to local structure
            PBLocAPIMsgHeader pbLocApiMsg;
            if (0 == pbLocApiMsg.ParseFromString(mMsgData)) {
                LOC_LOGe("Failed to parse pbLocApiMsg from input stream!! length: %zu",
                        mMsgData.length());
                return;
            }

            ELocMsgID eLocMsgid = mApiImpl.mPbufMsgConv.getEnumForPBELocMsgID(pbLocApiMsg.msgid());
            string sockName = pbLocApiMsg.msocketname();
            uint32_t payloadSize = pbLocApiMsg.payloadsize();
            // pbLocApiMsg.payload() contains the payload data.

            LOC_LOGi(">-- onReceive Rcvd msg id: %d, sockname: %s, payload size: %d", eLocMsgid,
                    sockName.c_str(), payloadSize);
            LocAPIMsgHeader locApiMsg(sockName.c_str(), eLocMsgid);

            // throw away message that does not come from location hal daemon
            if (false == locApiMsg.isValidServerMsg(payloadSize)) {
                return;
            }

            switch (locApiMsg.msgId) {
            case E_LOCAPI_HAL_READY_MSG_ID:
                LOC_LOGd("<<< HAL ready");
                // location hal daemon crashed and re-started
                mApiImpl.processHalReadyMsg();
                break;

            case E_INTAPI_CONFIG_CONSTRAINTED_TUNC_MSG_ID:
            case E_INTAPI_CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR_MSG_ID:
            case E_INTAPI_CONFIG_SV_CONSTELLATION_MSG_ID:
            case E_INTAPI_CONFIG_CONSTELLATION_SECONDARY_BAND_MSG_ID:
            case E_INTAPI_CONFIG_AIDING_DATA_DELETION_MSG_ID:
            case E_INTAPI_CONFIG_LEVER_ARM_MSG_ID:
            case E_INTAPI_CONFIG_ROBUST_LOCATION_MSG_ID:
            case E_INTAPI_CONFIG_MIN_GPS_WEEK_MSG_ID:
            case E_INTAPI_CONFIG_DEAD_RECKONING_ENGINE_MSG_ID:
            case E_INTAPI_CONFIG_MIN_SV_ELEVATION_MSG_ID:
            case E_INTAPI_CONFIG_ENGINE_RUN_STATE_MSG_ID:
            case E_INTAPI_CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING_MSG_ID:
            case E_INTAPI_CONFIG_OUTPUT_NMEA_TYPES_MSG_ID:
            case E_INTAPI_CONFIG_ENGINE_INTEGRITY_RISK_MSG_ID:
            case E_INTAPI_CONFIG_XTRA_PARAMS_MSG_ID:
            case E_INTAPI_CONFIG_MERKLE_TREE_MSG_ID:
            case E_INTAPI_CONFIG_OSNMA_ENABLEMENT_MSG_ID:
            case E_INTAPI_GET_ROBUST_LOCATION_CONFIG_REQ_MSG_ID:
            case E_INTAPI_GET_MIN_GPS_WEEK_REQ_MSG_ID:
            case E_INTAPI_GET_MIN_SV_ELEVATION_REQ_MSG_ID:
            case E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_REQ_MSG_ID:
            case E_INTAPI_GET_XTRA_STATUS_REQ_MSG_ID:
            case E_INTAPI_REGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID:
            case E_INTAPI_DEREGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID:
            case E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_REQ_MSG_ID:
            case E_INTAPI_CONFIG_MAP_MATCHED_FEEDBACK_MSG_ID:
            {
                PBLocAPIGenericRespMsg pbLocApiGenericRsp;
                if (0 == pbLocApiGenericRsp.ParseFromString(pbLocApiMsg.payload())) {
                    LOC_LOGe("Failed to parse pbLocApiGenericRsp from payload!!");
                    return;
                }
                LocAPIGenericRespMsg msg(sockName.c_str(), eLocMsgid, pbLocApiGenericRsp,
                        &mApiImpl.mPbufMsgConv);
                mApiImpl.processConfigRespCb((LocAPIGenericRespMsg*)&msg);
                break;
            }

            case E_INTAPI_GET_MIN_GPS_WEEK_RESP_MSG_ID:
            {
                PBLocConfigGetMinGpsWeekRespMsg configGeMinGpsWeek;
                if (0 == configGeMinGpsWeek.ParseFromString(pbLocApiMsg.payload())) {
                    LOC_LOGe("Failed to parse configGeMinGpsWeek from payload!!");
                    return;
                }
                LocConfigGetMinGpsWeekRespMsg msg(sockName.c_str(), configGeMinGpsWeek,
                        &mApiImpl.mPbufMsgConv);
                mApiImpl.processGetMinGpsWeekRespCb((LocConfigGetMinGpsWeekRespMsg*)&msg);
                break;
            }

            case E_INTAPI_GET_ROBUST_LOCATION_CONFIG_RESP_MSG_ID:
            {
                PBLocConfigGetRobustLocationConfigRespMsg configGetRobustLoc;
                if (0 == configGetRobustLoc.ParseFromString(pbLocApiMsg.payload())) {
                    LOC_LOGe("Failed to parse configGetRobustLoc from payload!!");
                    return;
                }

                LocConfigGetRobustLocationConfigRespMsg msg(sockName.c_str(), configGetRobustLoc,
                        &mApiImpl.mPbufMsgConv);
                mApiImpl.processGetRobustLocationConfigRespCb(
                        (LocConfigGetRobustLocationConfigRespMsg*)&msg);
                break;
            }

            case E_INTAPI_GET_MIN_SV_ELEVATION_RESP_MSG_ID:
            {
                PBLocConfigGetMinSvElevationRespMsg configGetMinSvElev;
                if (0 == configGetMinSvElev.ParseFromString(pbLocApiMsg.payload())) {
                    LOC_LOGe("Failed to parse configGetMinSvElev from payload!!");
                    return;
                }

                LocConfigGetMinSvElevationRespMsg msg(sockName.c_str(), configGetMinSvElev,
                        &mApiImpl.mPbufMsgConv);
                mApiImpl.processGetMinSvElevationRespCb((LocConfigGetMinSvElevationRespMsg*)&msg);
                break;
            }

            case E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_RESP_MSG_ID:
            {
                PBLocConfigGetConstltnSecondaryBandConfigRespMsg cfgGetConstlnSecBandCfgRespMsg;
                if (0 == cfgGetConstlnSecBandCfgRespMsg.ParseFromString(pbLocApiMsg.payload())) {
                    LOC_LOGe("Failed to parse cfgGetConstlnSecBandCfgRespMsg from payload!!");
                    return;
                }

                LocConfigGetConstellationSecondaryBandConfigRespMsg msg(sockName.c_str(),
                        cfgGetConstlnSecBandCfgRespMsg, &mApiImpl.mPbufMsgConv);
                mApiImpl.processGetConstellationSecondaryBandConfigRespCb(
                        (LocConfigGetConstellationSecondaryBandConfigRespMsg*)&msg);
                break;
            }

            case E_INTAPI_GET_XTRA_STATUS_RESP_MSG_ID:
            {
                PBLocConfigGetXtraStatusRespMsg respMsg;
                if (0 == respMsg.ParseFromString(pbLocApiMsg.payload())) {
                    LOC_LOGe("Failed to parse LocConfigGetXtraStatusRespMsg from payload!!");
                    return;
                }

                LocConfigGetXtraStatusRespMsg msg(sockName.c_str(), respMsg,
                                                  &mApiImpl.mPbufMsgConv);
                mApiImpl.processGetXtraStatusRespCb((LocConfigGetXtraStatusRespMsg*)&msg);
                break;
            }

            case E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_RESP_MSG_ID:
            {
                PBLocConfigRegisterGnssSignalTypesUpdateRespMsg respMsg;
                if (0 == respMsg.ParseFromString(pbLocApiMsg.payload())) {
                    LOC_LOGe("Failed to parse RegisterGnssSignalTypesUpdateRespMsg from payload!!");
                    return;
                }

                LocConfigRegisterGnssSignalTypesUpdateRespMsg msg(sockName.c_str(), respMsg,
                                                            &mApiImpl.mPbufMsgConv);
                mApiImpl.processRegisterGnssSignalTypesRespCb(
                        (LocConfigRegisterGnssSignalTypesUpdateRespMsg*)&msg);
                break;
            }

            default:
                LOC_LOGw("<<< unknown message %d", locApiMsg.msgId);
                break;
            }
        }
        LocationIntegrationApiImpl& mApiImpl;
        IpcListener& mListener;
        const string mMsgData;
    };

    mMsgTask.sendMsg(new (nothrow) OnReceiveHandler(mApiImpl, *this, data, length));
}

/******************************************************************************
LocationIntegrationApi - integration API implementation
******************************************************************************/
uint32_t LocationIntegrationApiImpl::configConstellations(
        const GnssSvTypeConfig& constellationEnablementConfig,
        const GnssSvIdConfig& blacklistSvConfig) {

    struct ConfigConstellationsReq : public LocMsg {
        ConfigConstellationsReq(LocationIntegrationApiImpl* apiImpl,
                                const GnssSvTypeConfig& constellationEnablementConfig,
                                const GnssSvIdConfig& blacklistSvConfig) :
                mApiImpl(apiImpl),
                mConstellationEnablementConfig(constellationEnablementConfig),
                mBlacklistSvConfig(blacklistSvConfig) {}
        virtual ~ConfigConstellationsReq() {}
        void proc() const {
            string pbStr;
            LocConfigSvConstellationReqMsg msg(mApiImpl->mSocketName,
                                               mConstellationEnablementConfig,
                                               mBlacklistSvConfig,
                                               &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                if (mApiImpl->sendConfigMsgToHalDaemon(CONFIG_CONSTELLATIONS, pbStr)) {
                    // cache the last config to be used when hal daemon restarts
                    mApiImpl->mSvConfigInfo.isValid = true;
                    mApiImpl->mSvConfigInfo.constellationEnablementConfig =
                            mConstellationEnablementConfig;
                    mApiImpl->mSvConfigInfo.blacklistSvConfig = mBlacklistSvConfig;
                }
            } else {
                LOC_LOGe("LocConfigSvConstellationReqMsg serializeToProtobuf failed");
            }
        }

        LocationIntegrationApiImpl* mApiImpl;
        GnssSvTypeConfig mConstellationEnablementConfig;
        GnssSvIdConfig mBlacklistSvConfig;
    };
    mMsgTask.sendMsg(new (nothrow) ConfigConstellationsReq(
            this, constellationEnablementConfig, blacklistSvConfig));
    return 0;
}

uint32_t LocationIntegrationApiImpl::configConstellationSecondaryBand(
        const GnssSvTypeConfig& secondaryBandConfig) {

    struct ConfigConstellationSecondaryBandReq : public LocMsg {
        ConfigConstellationSecondaryBandReq(LocationIntegrationApiImpl* apiImpl,
                                            const GnssSvTypeConfig& secondaryBandConfig) :
                mApiImpl(apiImpl),
                mSecondaryBandConfig(secondaryBandConfig) {}
        virtual ~ConfigConstellationSecondaryBandReq() {}
        void proc() const {
            string pbStr;
            LocConfigConstellationSecondaryBandReqMsg msg(
                    mApiImpl->mSocketName,
                    mSecondaryBandConfig,
                    &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                if (mApiImpl->sendConfigMsgToHalDaemon(
                        CONFIG_CONSTELLATION_SECONDARY_BAND, pbStr)) {
                    // cache the last config to be used when hal daemon restarts
                    mApiImpl->mSvConfigInfo.isValid = true;
                    mApiImpl->mSvConfigInfo.secondaryBandConfig = mSecondaryBandConfig;
                }
            } else {
                LOC_LOGe("LocConfigConstellationSecondaryBandReqMsg serializeToProtobuf failed");
            }
        }

        LocationIntegrationApiImpl* mApiImpl;
        GnssSvTypeConfig mSecondaryBandConfig;
    };
    mMsgTask.sendMsg(new (nothrow) ConfigConstellationSecondaryBandReq(this, secondaryBandConfig));
    return 0;
}

uint32_t LocationIntegrationApiImpl::getConstellationSecondaryBandConfig() {

    struct GetConstellationSecondaryBandConfigReq : public LocMsg {
        GetConstellationSecondaryBandConfigReq(LocationIntegrationApiImpl* apiImpl) :
                mApiImpl(apiImpl) {}
        virtual ~GetConstellationSecondaryBandConfigReq() {}
        void proc() const {
            string pbStr;
            LocConfigGetConstellationSecondaryBandConfigReqMsg msg(mApiImpl->mSocketName,
                    &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendConfigMsgToHalDaemon(
                        GET_CONSTELLATION_SECONDARY_BAND_CONFIG, pbStr);
            } else {
                LOC_LOGe("LocCgGetCnstlSecondaryBandConfigReqMsg serializeToProtobuf failed");
            }
        }
        LocationIntegrationApiImpl* mApiImpl;
    };

    if (mIntegrationCbs.getConstellationSecondaryBandConfigCb == nullptr) {
        LOC_LOGd("no callback in constructor to receive secondary band config");
        // return 1 to signal error
        return 1;
    }
    mMsgTask.sendMsg(new (nothrow) GetConstellationSecondaryBandConfigReq(this));

    return 0;
}

uint32_t LocationIntegrationApiImpl::configConstrainedTimeUncertainty(
        bool enable, float tuncThreshold, uint32_t energyBudget) {

    struct ConfigConstrainedTuncReq : public LocMsg {
        ConfigConstrainedTuncReq(LocationIntegrationApiImpl* apiImpl,
                                 bool enable,
                                 float tuncThreshold,
                                 uint32_t energyBudget) :
                mApiImpl(apiImpl),
                mEnable(enable),
                mTuncThreshold(tuncThreshold),
                mEnergyBudget(energyBudget) {}
        virtual ~ConfigConstrainedTuncReq() {}
        void proc() const {
            string pbStr;
            LocConfigConstrainedTuncReqMsg msg(mApiImpl->mSocketName,
                                               mEnable, mTuncThreshold, mEnergyBudget,
                                               &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                if (mApiImpl->sendConfigMsgToHalDaemon(
                        CONFIG_CONSTRAINED_TIME_UNCERTAINTY, pbStr)) {
                    // cache the last config to be used when hal daemon restarts
                    mApiImpl->mTuncConfigInfo.isValid = true;
                    mApiImpl->mTuncConfigInfo.enable = mEnable;
                    mApiImpl->mTuncConfigInfo.tuncThresholdMs = mTuncThreshold;
                    mApiImpl->mTuncConfigInfo.energyBudget = mEnergyBudget;
                }
            } else {
                LOC_LOGe("LocConfigConstrainedTuncReqMsg serializeToProtobuf failed");
            }
        }
        LocationIntegrationApiImpl* mApiImpl;
        bool mEnable;
        float mTuncThreshold;
        uint32_t mEnergyBudget;
    };
    mMsgTask.sendMsg(new (nothrow)ConfigConstrainedTuncReq(
            this, enable, tuncThreshold, energyBudget));

    return 0;
}

uint32_t LocationIntegrationApiImpl::configPositionAssistedClockEstimator(bool enable) {

    struct ConfigPositionAssistedClockEstimatorReq : public LocMsg {
        ConfigPositionAssistedClockEstimatorReq(LocationIntegrationApiImpl* apiImpl,
                                                bool enable) :
                mApiImpl(apiImpl),
                mEnable(enable) {}
        virtual ~ConfigPositionAssistedClockEstimatorReq() {}
        void proc() const {
            string pbStr;
            LocConfigPositionAssistedClockEstimatorReqMsg msg(mApiImpl->mSocketName,
                                                              mEnable,
                                                              &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                if (mApiImpl->sendConfigMsgToHalDaemon(
                            CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR, pbStr)) {
                    // cache the last config to be used when hal daemon restarts
                    mApiImpl->mPaceConfigInfo.isValid = true;
                    mApiImpl->mPaceConfigInfo.enable = mEnable;
                }
            } else {
                LOC_LOGe("LocConfigPositionAssistedClockEstimatorReqMsg serializeToProtobuf fail");
            }
        }
        LocationIntegrationApiImpl* mApiImpl;
        bool mEnable;
    };
    mMsgTask.sendMsg(new (nothrow)
            ConfigPositionAssistedClockEstimatorReq(this, enable));

    return 0;
}

uint32_t LocationIntegrationApiImpl::gnssDeleteAidingData(
        const GnssAidingData& aidingData) {
    struct DeleteAidingDataReq : public LocMsg {
        DeleteAidingDataReq(LocationIntegrationApiImpl* apiImpl,
                            const GnssAidingData& aidingData) :
                mApiImpl(apiImpl),
                mAidingData(aidingData) {}
        virtual ~DeleteAidingDataReq() {}
        void proc() const {
            string pbStr;
            LocConfigAidingDataDeletionReqMsg msg(mApiImpl->mSocketName,
                                                  const_cast<GnssAidingData&>(mAidingData),
                                                  &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendConfigMsgToHalDaemon(CONFIG_AIDING_DATA_DELETION, pbStr);
            } else {
                LOC_LOGe("LocConfigAidingDataDeletionReqMsg serializeToProtobuf failed");
            }
        }
        LocationIntegrationApiImpl* mApiImpl;
        GnssAidingData mAidingData;
    };
    mMsgTask.sendMsg(new (nothrow) DeleteAidingDataReq(this, aidingData));

    return 0;
}

uint32_t LocationIntegrationApiImpl::configLeverArm(
        const LeverArmConfigInfo& configInfo) {
    struct ConfigLeverArmReq : public LocMsg {
        ConfigLeverArmReq(LocationIntegrationApiImpl* apiImpl,
                          const LeverArmConfigInfo& configInfo) :
                mApiImpl(apiImpl),
                mConfigInfo(configInfo) {}
        virtual ~ConfigLeverArmReq() {}
        void proc() const {
            string pbStr;
            LocConfigLeverArmReqMsg msg(mApiImpl->mSocketName, mConfigInfo,
                    &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                if (mApiImpl->sendConfigMsgToHalDaemon(CONFIG_LEVER_ARM, pbStr)) {
                    // cache the last config to be used when hal daemon restarts
                    mApiImpl->mLeverArmConfigInfo = mConfigInfo;
                }
            } else {
                LOC_LOGe("LocConfigLeverArmReqMsg serializeToProtobuf failed");
            }
        }
        LocationIntegrationApiImpl* mApiImpl;
        LeverArmConfigInfo mConfigInfo;
    };

    if (configInfo.leverArmValidMask) {
        mMsgTask.sendMsg(new (nothrow) ConfigLeverArmReq(this, configInfo));
    }

    return 0;
}

uint32_t LocationIntegrationApiImpl::configRobustLocation(
        bool enable, bool enableForE911) {
    struct ConfigRobustLocationReq : public LocMsg {
        ConfigRobustLocationReq(LocationIntegrationApiImpl* apiImpl,
                                bool enable,
                                bool enableForE911) :
                mApiImpl(apiImpl),
                mEnable(enable),
                mEnableForE911(enableForE911){}
        virtual ~ConfigRobustLocationReq() {}
        void proc() const {
            string pbStr;
            LocConfigRobustLocationReqMsg msg(mApiImpl->mSocketName, mEnable,
                    mEnableForE911, &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                if (mApiImpl->sendConfigMsgToHalDaemon(CONFIG_ROBUST_LOCATION, pbStr)) {
                    mApiImpl->mRobustLocationConfigInfo.isValid = true;
                    mApiImpl->mRobustLocationConfigInfo.enable = mEnable;
                    mApiImpl->mRobustLocationConfigInfo.enableForE911 = mEnableForE911;
                }
            } else {
                LOC_LOGe("LocConfigRobustLocationReqMsg serializeToProtobuf failed");
            }
        }
        LocationIntegrationApiImpl* mApiImpl;
        bool mEnable;
        bool mEnableForE911;
    };

    mMsgTask.sendMsg(new (nothrow)
                      ConfigRobustLocationReq(this, enable, enableForE911));

    return 0;
}

uint32_t LocationIntegrationApiImpl::getRobustLocationConfig() {

    struct GetRobustLocationConfigReq : public LocMsg {
        GetRobustLocationConfigReq(LocationIntegrationApiImpl* apiImpl) :
                mApiImpl(apiImpl) {}
        virtual ~GetRobustLocationConfigReq() {}
        void proc() const {
            string pbStr;
            LocConfigGetRobustLocationConfigReqMsg msg(mApiImpl->mSocketName,
                    &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendConfigMsgToHalDaemon(GET_ROBUST_LOCATION_CONFIG, pbStr);
            } else {
                LOC_LOGe("LocConfigGetRobustLocationConfigReqMsg serializeToProtobuf failed");
            }
        }
        LocationIntegrationApiImpl* mApiImpl;
    };

    if (mIntegrationCbs.getRobustLocationConfigCb == nullptr) {
        LOC_LOGe("no callback passed in constructor to receive robust location config");
        // return 1 to signal error
        return 1;
    }
    mMsgTask.sendMsg(new (nothrow) GetRobustLocationConfigReq(this));

    return 0;
}

uint32_t LocationIntegrationApiImpl::configMinGpsWeek(uint16_t minGpsWeek) {
    struct ConfigMinGpsWeekReq : public LocMsg {
        ConfigMinGpsWeekReq(LocationIntegrationApiImpl* apiImpl,
                            uint16_t minGpsWeek) :
                mApiImpl(apiImpl),
                mMinGpsWeek(minGpsWeek) {}
        virtual ~ConfigMinGpsWeekReq() {}
        void proc() const {
            string pbStr;
            LocConfigMinGpsWeekReqMsg msg(mApiImpl->mSocketName,
                    mMinGpsWeek, &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendConfigMsgToHalDaemon(CONFIG_MIN_GPS_WEEK, pbStr);
            } else {
                LOC_LOGe("LocConfigMinGpsWeekReqMsg serializeToProtobuf failed");
            }
        }
        LocationIntegrationApiImpl* mApiImpl;
        uint16_t mMinGpsWeek;
    };

    mMsgTask.sendMsg(new (nothrow) ConfigMinGpsWeekReq(this, minGpsWeek));
    return 0;
}

uint32_t LocationIntegrationApiImpl::getMinGpsWeek() {

    struct GetMinGpsWeekReq : public LocMsg {
        GetMinGpsWeekReq(LocationIntegrationApiImpl* apiImpl) :
                mApiImpl(apiImpl) {}
        virtual ~GetMinGpsWeekReq() {}
        void proc() const {
            string pbStr;
            LocConfigGetMinGpsWeekReqMsg msg(mApiImpl->mSocketName, &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendConfigMsgToHalDaemon(GET_MIN_GPS_WEEK, pbStr);
            } else {
                LOC_LOGe("LocConfigGetMinGpsWeekReqMsg serializeToProtobuf failed");
            }
        }
        LocationIntegrationApiImpl* mApiImpl;
    };

    if (mIntegrationCbs.getMinGpsWeekCb == nullptr) {
        LOC_LOGe("no callback passed in constructor to receive gps week info");
        // return 1 to signal error
        return 1;
    }
    mMsgTask.sendMsg(new (nothrow) GetMinGpsWeekReq(this));

    return 0;
}

uint32_t LocationIntegrationApiImpl::configDeadReckoningEngineParams(
        const ::DeadReckoningEngineConfig& dreConfig) {
    struct ConfigDrEngineParamsReq : public LocMsg {
        ConfigDrEngineParamsReq(LocationIntegrationApiImpl* apiImpl,
                                ::DeadReckoningEngineConfig dreConfig) :
                mApiImpl(apiImpl),
                mDreConfig(dreConfig){}
        virtual ~ConfigDrEngineParamsReq() {}
        void proc() const {
            string pbStr;
            LocConfigDrEngineParamsReqMsg msg(mApiImpl->mSocketName,
                                              mDreConfig,
                                              &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                if (mApiImpl->sendConfigMsgToHalDaemon(CONFIG_DEAD_RECKONING_ENGINE, pbStr)) {
                    mApiImpl->mDreConfigInfo.isValid = true;
                    mApiImpl->mDreConfigInfo.dreConfig = mDreConfig;
                }
            } else {
                LOC_LOGe("LocConfigDrEngineParamsReqMsg serializeToProtobuf failed");
            }
        }
        LocationIntegrationApiImpl* mApiImpl;
        ::DeadReckoningEngineConfig mDreConfig;
    };

    mMsgTask.sendMsg(new (nothrow) ConfigDrEngineParamsReq(this, dreConfig));

    return 0;
}

uint32_t LocationIntegrationApiImpl::configMinSvElevation(uint8_t minSvElevation) {

        struct ConfigMinSvElevationReq : public LocMsg {
        ConfigMinSvElevationReq(LocationIntegrationApiImpl* apiImpl,
                                uint8_t minSvElevation) :
                mApiImpl(apiImpl), mMinSvElevation(minSvElevation){}
        virtual ~ConfigMinSvElevationReq() {}
        void proc() const {
            string pbStr;
            LocConfigMinSvElevationReqMsg msg(mApiImpl->mSocketName,
                    mMinSvElevation, &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendConfigMsgToHalDaemon(CONFIG_MIN_SV_ELEVATION, pbStr);
            } else {
                LOC_LOGe("LocConfigMinSvElevationReqMsg serializeToProtobuf failed");
            }
        }
        LocationIntegrationApiImpl* mApiImpl;
        uint8_t mMinSvElevation;
    };

    mMsgTask.sendMsg(new (nothrow) ConfigMinSvElevationReq(this, minSvElevation));
    return 0;
}

uint32_t LocationIntegrationApiImpl::getMinSvElevation() {

    struct GetMinSvElevationReq : public LocMsg {
        GetMinSvElevationReq(LocationIntegrationApiImpl* apiImpl) :
                mApiImpl(apiImpl) {}
        virtual ~GetMinSvElevationReq() {}
        void proc() const {
            string pbStr;
            LocConfigGetMinSvElevationReqMsg msg(mApiImpl->mSocketName, &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendConfigMsgToHalDaemon(GET_MIN_SV_ELEVATION, pbStr);
            } else {
                LOC_LOGe("LocConfigGetMinSvElevationReqMsg serializeToProtobuf failed");
            }
        }
        LocationIntegrationApiImpl* mApiImpl;
    };

    if (mIntegrationCbs.getMinSvElevationCb == nullptr) {
        LOC_LOGe("no callback passed in constructor to receive min sv elevation info");
        // return 1 to signal error
        return 1;
    }
    mMsgTask.sendMsg(new (nothrow) GetMinSvElevationReq(this));

    return 0;
}

uint32_t LocationIntegrationApiImpl::configEngineRunState(
        PositioningEngineMask engType, LocEngineRunState engState) {

    struct ConfigEngineRunStateReq : public LocMsg {
        ConfigEngineRunStateReq(LocationIntegrationApiImpl* apiImpl,
                                PositioningEngineMask engType,
                                LocEngineRunState engState) :
                mApiImpl(apiImpl), mEngType(engType), mEngState(engState) {}
        virtual ~ConfigEngineRunStateReq() {}
        void proc() const {
            string pbStr;
            LocConfigEngineRunStateReqMsg msg(mApiImpl->mSocketName,
                    mEngType, mEngState, &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                if (mApiImpl->sendConfigMsgToHalDaemon(CONFIG_ENGINE_RUN_STATE, pbStr)) {
                    if (mApiImpl->mEngRunStateConfigMap.find(mEngType) ==
                                std::end(mApiImpl->mEngRunStateConfigMap)) {
                        mApiImpl->mEngRunStateConfigMap.emplace(mEngType, mEngState);
                    } else {
                        // change the state for the eng
                        mApiImpl->mEngRunStateConfigMap[mEngType] = mEngState;
                    }
                }
            } else {
                LOC_LOGe("LocConfigEngineRunStateReqMsg serializeToProtobuf failed");
            }
        }

        LocationIntegrationApiImpl* mApiImpl;
        PositioningEngineMask mEngType;
        LocEngineRunState mEngState;
    };

    mMsgTask.sendMsg(new (nothrow) ConfigEngineRunStateReq(this, engType, engState));
    return 0;
}

uint32_t LocationIntegrationApiImpl::setUserConsentForTerrestrialPositioning(bool userConsent) {
    struct SetUserConsentReq : public LocMsg {
        SetUserConsentReq(LocationIntegrationApiImpl* apiImpl,
                          bool userConsent) :
                mApiImpl(apiImpl), mUserConsent(userConsent) {}
        virtual ~SetUserConsentReq() {}
        void proc() const {
            string pbStr;
            LocConfigUserConsentTerrestrialPositioningReqMsg msg(
                    mApiImpl->mSocketName, mUserConsent, &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                if (mApiImpl->sendConfigMsgToHalDaemon(
                        CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING, pbStr)) {
                    mApiImpl->mGtpUserConsentConfigInfo.isValid = true;
                    mApiImpl->mGtpUserConsentConfigInfo.userConsent = mUserConsent;
                }
            } else {
                LOC_LOGe("serializeToProtobuf failed");
            }
        }

        LocationIntegrationApiImpl* mApiImpl;
        bool mUserConsent;
    };

    mMsgTask.sendMsg(new (nothrow) SetUserConsentReq(this, userConsent));
    return 0;
}

uint32_t LocationIntegrationApiImpl::configOutputNmeaTypes(
        GnssNmeaTypesMask enabledNmeaTypes,
        GnssGeodeticDatumType nmeaDatumType,
        LocReqEngineTypeMask locReqEngMask) {
    struct ConfigOutputNmeaReq : public LocMsg {
        ConfigOutputNmeaReq(LocationIntegrationApiImpl* apiImpl,
                            GnssNmeaTypesMask enabledNmeaTypes,
                            GnssGeodeticDatumType nmeaDatumType,
                            LocReqEngineTypeMask locReqEngMask) :
                mApiImpl(apiImpl), mEnabledNmeaTypes(enabledNmeaTypes),
                mNmeaDatumType(nmeaDatumType), mLocReqEngMask(locReqEngMask) {}
        virtual ~ConfigOutputNmeaReq() {}
        void proc() const {
            string pbStr;
            LocConfigOutputNmeaTypesReqMsg msg(
                    mApiImpl->mSocketName, mEnabledNmeaTypes,
                    mNmeaDatumType, mLocReqEngMask, &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                if (mApiImpl->sendConfigMsgToHalDaemon(CONFIG_OUTPUT_NMEA_TYPES, pbStr)) {
                    mApiImpl->mNmeaConfigInfo.isValid = true;
                    mApiImpl->mNmeaConfigInfo.enabledNmeaTypes = mEnabledNmeaTypes;
                    mApiImpl->mNmeaConfigInfo.nmeaDatumType = mNmeaDatumType;
                    mApiImpl->mNmeaConfigInfo.locReqEngMask = mLocReqEngMask;
                }
            } else {
                LOC_LOGe("serializeToProtobuf failed");
            }
        }

        LocationIntegrationApiImpl* mApiImpl;
        GnssNmeaTypesMask mEnabledNmeaTypes;
        GnssGeodeticDatumType mNmeaDatumType;
        LocReqEngineTypeMask mLocReqEngMask;
    };

    LOC_LOGi("nmea output type: 0x%x, datum type: %d, request engine mask: 0x%x", enabledNmeaTypes,
            nmeaDatumType, locReqEngMask);
    mMsgTask.sendMsg(new (nothrow) ConfigOutputNmeaReq(this, enabledNmeaTypes, nmeaDatumType,
            locReqEngMask));
    return 0;
}

uint32_t LocationIntegrationApiImpl::configEngineIntegrityRisk(
        PositioningEngineMask engType, uint32_t integrityRisk) {

    struct ConfigEngineIntegrityRiskReq : public LocMsg {
        ConfigEngineIntegrityRiskReq(LocationIntegrationApiImpl* apiImpl,
                                     PositioningEngineMask engType,
                                     uint32_t integrityRisk) :
                mApiImpl(apiImpl), mEngType(engType), mIntegrityRisk(integrityRisk) {}
        virtual ~ConfigEngineIntegrityRiskReq() {}
        void proc() const {
            LOC_LOGd("eng type %d, integrity risk %u", mEngType, mIntegrityRisk);
            string pbStr;
            LocConfigEngineIntegrityRiskReqMsg msg(mApiImpl->mSocketName,
                    mEngType, mIntegrityRisk, &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                if (mApiImpl->sendConfigMsgToHalDaemon(CONFIG_ENGINE_INTEGRITY_RISK, pbStr)) {
                    if (mApiImpl->mEngIntegrityRiskConfigMap.find(mEngType) ==
                            std::end(mApiImpl->mEngIntegrityRiskConfigMap)) {
                        mApiImpl->mEngIntegrityRiskConfigMap.emplace(mEngType, mIntegrityRisk);
                    } else {
                        // change the state for the eng
                        mApiImpl->mEngIntegrityRiskConfigMap[mEngType] = mIntegrityRisk;
                    }
                }
            } else {
                LOC_LOGe("LocConfigEngineIntegrityRiskReqMsg serializeToProtobuf failed");
            }
        }

        LocationIntegrationApiImpl* mApiImpl;
        PositioningEngineMask mEngType;
        uint32_t mIntegrityRisk;
    };

    mMsgTask.sendMsg(new (nothrow) ConfigEngineIntegrityRiskReq(this, engType, integrityRisk));
    return 0;
}

void LocationIntegrationApiImpl::odcpiInject(const ::Location& location) {
    struct InjectLocationReq : public LocMsg {
        InjectLocationReq(LocationIntegrationApiImpl* apiImpl,
                const ::Location &location) : mApiImpl(apiImpl), mLocation(location) {}
        virtual ~InjectLocationReq() {}
        void proc() const {
            LocIntApiInjectLocationMsg msg(mApiImpl->mSocketName, mLocation,
                                           &mApiImpl->mPbufMsgConv);
            string pbStr;
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendConfigMsgToHalDaemon((LocConfigTypeEnum)0, /* no response cb */
                                                   pbStr);
            } else {
                LOC_LOGe("serializeToProtobuf failed");
            }
        }

        LocationIntegrationApiImpl* mApiImpl;
        const ::Location            mLocation;
    };

    mMsgTask.sendMsg(new (nothrow) InjectLocationReq(this, location));
}

uint32_t LocationIntegrationApiImpl::configXtraParams(bool enable,
                                                      const ::XtraConfigParams& configParams) {

    struct ConfigXtraReq : public LocMsg {
        ConfigXtraReq(LocationIntegrationApiImpl* apiImpl,
                      bool enable, const ::XtraConfigParams& configParams) :
                mApiImpl(apiImpl), mEnable(enable), mConfigParams(configParams) {}
        virtual ~ConfigXtraReq() {}
        void proc() const {
            if (1 != sXtraTestEnabled) {
                // download interval: 48 hours and 168 hours
                if (mConfigParams.xtraDownloadIntervalMinute != 0) {
                    if (mConfigParams.xtraDownloadIntervalMinute < 48 * 60) {
                        mConfigParams.xtraDownloadIntervalMinute = 48 * 60;
                    } else if (mConfigParams.xtraDownloadIntervalMinute > 168 * 60) {
                        mConfigParams.xtraDownloadIntervalMinute = 168 * 60;
                    }
                }

                // download timeout: maximum of 300 secs and minimum of 3 secs
                if (mConfigParams.xtraDownloadTimeoutSec != 0) {
                    if (mConfigParams.xtraDownloadTimeoutSec < 3) {
                        mConfigParams.xtraDownloadTimeoutSec = 3;
                    } else if (mConfigParams.xtraDownloadTimeoutSec > 300) {
                        mConfigParams.xtraDownloadTimeoutSec = 300;
                    }
                }

                // retry interval: a maximum of 1 day and a minimum of 3 minutes.
                if (mConfigParams.xtraDownloadRetryIntervalMinute != 0) {
                    if (mConfigParams.xtraDownloadRetryIntervalMinute < 3) {
                        mConfigParams.xtraDownloadRetryIntervalMinute = 3;
                    } else if (mConfigParams.xtraDownloadRetryIntervalMinute > 24 * 60) {
                        mConfigParams.xtraDownloadRetryIntervalMinute = 24 * 60;
                    }
                }

                // retry attempts: maximum number of allowed retry is 6 per
                // download interval.
                if (mConfigParams.xtraDownloadRetryAttempts > 6) {
                    mConfigParams.xtraDownloadRetryAttempts = 6;
                }

                // integrity file download interval: min is 6 hours, max is 48 hours
                if (mConfigParams.xtraIntegrityDownloadIntervalMinute < 360) {
                    mConfigParams.xtraIntegrityDownloadIntervalMinute = 360;
                } else if (mConfigParams.xtraIntegrityDownloadIntervalMinute > 2880) {
                    mConfigParams.xtraIntegrityDownloadIntervalMinute = 2880;
                }
            }

            string pbStr;
            LocConfigXtraReqMsg msg(mApiImpl->mSocketName, mEnable, mConfigParams,
                                    &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendConfigMsgToHalDaemon(CONFIG_XTRA_PARAMS, pbStr);
            } else {
                LOC_LOGe("serializeToProtobuf failed");
            }
        }

        LocationIntegrationApiImpl* mApiImpl;
        bool mEnable;
        mutable ::XtraConfigParams mConfigParams;
    };

    mMsgTask.sendMsg(new (nothrow) ConfigXtraReq(this, enable, configParams));
    return 0;
}

uint32_t LocationIntegrationApiImpl::getXtraStatus() {

    struct GetXtraStatusReq : public LocMsg {
        GetXtraStatusReq(LocationIntegrationApiImpl* apiImpl) :
                mApiImpl(apiImpl) {}
        virtual ~GetXtraStatusReq() {}
        void proc() const {
            string pbStr;
            LocConfigGetXtraStatusReqMsg msg(mApiImpl->mSocketName, &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendConfigMsgToHalDaemon(GET_XTRA_STATUS, pbStr);
            } else {
                LOC_LOGe("serializeToProtobuf failed");
            }
        }
        LocationIntegrationApiImpl* mApiImpl;
    };

    if (mIntegrationCbs.getXtraStatusCb == nullptr) {
        LOC_LOGe("no callback passed in constructor to receive xtra status");
        // return 1 to signal error
        return 1;
    }
    mMsgTask.sendMsg(new (nothrow) GetXtraStatusReq(this));
    return 0;
}

uint32_t LocationIntegrationApiImpl::registerXtraStatusUpdate(bool registerUpdate) {

    struct RegisterXtraStatusUpdateReq : public LocMsg {
        RegisterXtraStatusUpdateReq(LocationIntegrationApiImpl* apiImpl,
                      bool registerUpdate) :
                mApiImpl(apiImpl), mRegisterUpdate(registerUpdate) {}
        virtual ~RegisterXtraStatusUpdateReq() {}
        void proc() const {
            LOC_LOGe("registerXtraStatusUpdate: %d", mRegisterUpdate);
            string pbStr;
            if (true == mRegisterUpdate) {
                LocConfigRegisterXtraStatusUpdateReqMsg msg(
                    mApiImpl->mSocketName, &mApiImpl->mPbufMsgConv);
                msg.serializeToProtobuf(pbStr);
            } else {
                LocConfigDeregisterXtraStatusUpdateReqMsg msg(
                    mApiImpl->mSocketName, &mApiImpl->mPbufMsgConv);
                msg.serializeToProtobuf(pbStr);
            }

            if (pbStr.size() != 0) {
                if (mApiImpl->sendConfigMsgToHalDaemon(REGISTER_XTRA_STATUS_UPDATE, pbStr)) {
                    mApiImpl->mXtraUpdateUponRegisterPending = mRegisterUpdate;
                    mApiImpl->mRegisterXtraUpdate = mRegisterUpdate;
                }
            } else {
                LOC_LOGe("serializeToProtobuf failed");
            }
        }

        LocationIntegrationApiImpl* mApiImpl;
        bool mRegisterUpdate;
    };

    if (mIntegrationCbs.getXtraStatusCb == nullptr) {
        LOC_LOGe("no callback passed in constructor to receive xtra status");
        // return 1 to signal error
        return 1;
    }
    mMsgTask.sendMsg(new (nothrow) RegisterXtraStatusUpdateReq(this, registerUpdate));
    return 0;
}

uint32_t LocationIntegrationApiImpl::configMerkleTree(const char * merkleTreeXml, int xmlSize) {
    struct ConfigMerkleTreeReq : public LocMsg {
        ConfigMerkleTreeReq(LocationIntegrationApiImpl* apiImpl,
                      const char* merkleTreeConfigBuffer, int len):
                mApiImpl(apiImpl), mMerkleTreeBuffer(merkleTreeConfigBuffer, len) {}
        virtual ~ConfigMerkleTreeReq() {}
        void proc() const {
            string pbStr;
            LocConfigMerkleTreeReqMsg msg(mApiImpl->mSocketName, mMerkleTreeBuffer,
                    &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendConfigMsgToHalDaemon(CONFIG_MERKLE_TREE, pbStr);
            } else {
                LOC_LOGe("serializeToProtobuf failed");
            }
        }

        LocationIntegrationApiImpl* mApiImpl;
        std::string mMerkleTreeBuffer;
    };

    mMsgTask.sendMsg(new (nothrow) ConfigMerkleTreeReq(this, merkleTreeXml, xmlSize));
    return 0;
}

uint32_t LocationIntegrationApiImpl::configOsnmaEnablement(bool isEnabled) {
    struct ConfigOsnmaEnablementReq : public LocMsg {
        ConfigOsnmaEnablementReq(LocationIntegrationApiImpl* apiImpl, bool enable):
                mApiImpl(apiImpl), mEnable(enable) {}
        virtual ~ConfigOsnmaEnablementReq() {}
        void proc() const {
            string pbStr;
            LocConfigOsnmaEnablementReqMsg msg(mApiImpl->mSocketName, mEnable,
                    &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendConfigMsgToHalDaemon(CONFIG_OSNMA_ENABLEMENT, pbStr);
            } else {
                LOC_LOGe("serializeToProtobuf failed");
            }
        }

        LocationIntegrationApiImpl* mApiImpl;
        bool mEnable;
    };

    mMsgTask.sendMsg(new (nothrow) ConfigOsnmaEnablementReq(this, isEnabled));
    return 0;
}

uint32_t LocationIntegrationApiImpl::registerGnssSignalTypesUpdate(bool registerUpdate) {
    struct RegisterGnssSignalTypesUpdateReq : public LocMsg {
        RegisterGnssSignalTypesUpdateReq(LocationIntegrationApiImpl* apiImpl,
                bool registerUpdate) : mApiImpl(apiImpl), mRegisterUpdate(registerUpdate) {}
        virtual ~RegisterGnssSignalTypesUpdateReq() {}
        void proc() const {
            string pbStr;
            LocConfigRegisterGnssSignalTypesUpdateReqMsg msg(mApiImpl->mSocketName, mRegisterUpdate,
                    &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendConfigMsgToHalDaemon(REGISTER_SIGNAL_TYPES_UPDATE, pbStr);
            } else {
                LOC_LOGe("serializeToProtobuf failed");
            }
        }

        LocationIntegrationApiImpl* mApiImpl;
        bool mRegisterUpdate;
    };

    mMsgTask.sendMsg(new (nothrow) RegisterGnssSignalTypesUpdateReq(this, registerUpdate));
    return 0;
}


bool LocationIntegrationApiImpl::sendConfigMsgToHalDaemon(
        LocConfigTypeEnum configType, const string& pbStr, bool invokeResponseCb) {
    bool rc = false;
    LOC_LOGd(">>> sendConfigMsgToHalDaemon, mHalRegistered %d, config type=%d, "
             "msg size %zu, config cb %d",
             mHalRegistered, configType, pbStr.size(), invokeResponseCb);

    if (!mHalRegistered) {
        ProtoMsgInfo msgInfo;
        msgInfo.configType = configType;
        msgInfo.protoStr = pbStr;
        mQueuedMsg.emplace(std::move(msgInfo));
        rc = true;
        LOC_LOGi(">>> sendConfigMsgToHalDaemon mHal not yet ready, message queued");
    } else {
        bool messageSentToHal = false;
        rc = sendMessage(reinterpret_cast<uint8_t*>((uint8_t *)pbStr.c_str()),
                         pbStr.size());
        if (true == rc) {
            messageSentToHal = true;
        } else {
            LOC_LOGe(">>> sendConfigMsgToHalDaemon failed for msg type=%d", configType);
        }

        if (invokeResponseCb && mIntegrationCbs.configCb) {
            if (true == messageSentToHal) {
                addConfigReq(configType);
            } else {
                mIntegrationCbs.configCb(configType, LOC_INT_RESPONSE_FAILURE);
            }
        }
    }
    return rc;
}

bool LocationIntegrationApiImpl::sendClientRegMsgToHalDaemon(){
    bool retVal = false;
    string pbStr;
    LocAPIClientRegisterReqMsg msg(mSocketName, LOCATION_INTEGRATION_API, &mPbufMsgConv);
    if (msg.serializeToProtobuf(pbStr)) {
        bool rc = sendMessage(reinterpret_cast<uint8_t *>((uint8_t *)pbStr.c_str()),
                pbStr.size());
        LOC_LOGd(">>> onListenerReady::ClientRegisterReqMsg rc=%d", rc);
        if (true == rc) {
            mHalRegistered = true;
            retVal = true;
        }
    } else {
        LOC_LOGe("LocAPIClientRegisterReqMsg serializeToProtobuf failed");
    }
    return retVal;
}

void LocationIntegrationApiImpl::processHalReadyMsg() {
    // first, send registration msg to hal daemon
    if (sendClientRegMsgToHalDaemon() == false) {
        LOC_LOGe("failed to register with HAL, return");
        return;
    }

    // process the requests that are queued before hal daemon was first ready
    if (processQueuedReqs()) {
        // hal is first time ready, no more item to process
        return;
    }

    // when location hal daemon crashes and restarts,
    // we flush out all pending requests and notify each client
    flushConfigReqs();

    // send cached configuration to hal daemon
    if (mSvConfigInfo.isValid) {
        string pbStr;
        LocConfigSvConstellationReqMsg msg(mSocketName,
                                           mSvConfigInfo.constellationEnablementConfig,
                                           mSvConfigInfo.blacklistSvConfig,
                                           &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            sendConfigMsgToHalDaemon(CONFIG_CONSTELLATIONS, pbStr, false);
        } else {
            LOC_LOGe("LocConfigSvConstellationReqMsg serializeToProtobuf failed");
        }
    }

    if (mSvConfigInfo.secondaryBandConfig.size != 0) {
        string pbStr;
        LocConfigConstellationSecondaryBandReqMsg msg(
                    mSocketName, mSvConfigInfo.secondaryBandConfig, &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            sendConfigMsgToHalDaemon(CONFIG_CONSTELLATION_SECONDARY_BAND, pbStr, false);
        } else {
            LOC_LOGe("LocConfigConstellationSecondaryBandReqMsg serializeToProtobuf failed");
        }
    }

    if (mTuncConfigInfo.isValid) {
        string pbStr;
        LocConfigConstrainedTuncReqMsg msg(mSocketName,
                                           mTuncConfigInfo.enable,
                                           mTuncConfigInfo.tuncThresholdMs,
                                           mTuncConfigInfo.energyBudget,
                                           &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            sendConfigMsgToHalDaemon(CONFIG_CONSTRAINED_TIME_UNCERTAINTY, pbStr, false);
        } else {
            LOC_LOGe("LocConfigConstrainedTuncReqMsg serializeToProtobuf failed");
        }
    }
    if (mPaceConfigInfo.isValid) {
        string pbStr;
        LocConfigPositionAssistedClockEstimatorReqMsg msg(mSocketName,
                                                          mPaceConfigInfo.enable,
                                                          &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            sendConfigMsgToHalDaemon(CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR, pbStr, false);
        } else {
            LOC_LOGe("LocConfigPositionAssistedClockEstimatorReqMsg serializeToProtobuf failed");
        }
    }
    if (mLeverArmConfigInfo.leverArmValidMask) {
        string pbStr;
        LocConfigLeverArmReqMsg msg(mSocketName, mLeverArmConfigInfo, &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            sendConfigMsgToHalDaemon(CONFIG_LEVER_ARM, pbStr, false);
        } else {
            LOC_LOGe("LocConfigLeverArmReqMsg serializeToProtobuf failed");
        }
    }
    if (mRobustLocationConfigInfo.isValid) {
        string pbStr;
        LocConfigRobustLocationReqMsg msg(mSocketName,
                                          mRobustLocationConfigInfo.enable,
                                          mRobustLocationConfigInfo.enableForE911,
                                          &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            sendConfigMsgToHalDaemon(CONFIG_ROBUST_LOCATION, pbStr, false);
        } else {
            LOC_LOGe("LocConfigRobustLocationReqMsg serializeToProtobuf failed");
        }
    }
    // Do not reconfigure min gps week, as min gps week setting
    // can be overwritten by modem over  time

    if (mDreConfigInfo.isValid) {
        string pbStr;
        LocConfigDrEngineParamsReqMsg msg(mSocketName, mDreConfigInfo.dreConfig, &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            sendConfigMsgToHalDaemon(CONFIG_DEAD_RECKONING_ENGINE, pbStr, false);
        } else {
            LOC_LOGe("LocConfigDrEngineParamsReqMsg serializeToProtobuf failed");
        }
    }

    if (mGtpUserConsentConfigInfo.isValid) {
        string pbStr;
        LocConfigUserConsentTerrestrialPositioningReqMsg msg(
                    mSocketName, mGtpUserConsentConfigInfo.userConsent, &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            sendConfigMsgToHalDaemon(
                    CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING, pbStr, false);
        } else {
            LOC_LOGe("serializeToProtobuf failed");
        }
    }

    if (mNmeaConfigInfo.isValid) {
        string pbStr;
        LocConfigOutputNmeaTypesReqMsg msg(
                    mSocketName, mNmeaConfigInfo.enabledNmeaTypes,
                    mNmeaConfigInfo.nmeaDatumType, mNmeaConfigInfo.locReqEngMask, &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            sendConfigMsgToHalDaemon(CONFIG_OUTPUT_NMEA_TYPES, pbStr, false);
        } else {
            LOC_LOGe("serializeToProtobuf failed");
        }
    }

    // resend engine state config request
    for (auto it = mEngRunStateConfigMap.begin(); it != mEngRunStateConfigMap.end(); ++it) {
        string pbStr;
        LocConfigEngineRunStateReqMsg msg(mSocketName, it->first, it->second, &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            sendConfigMsgToHalDaemon(CONFIG_ENGINE_RUN_STATE, pbStr, false);
        }
    }

    // resend engine state config request
    for (auto it = mEngIntegrityRiskConfigMap.begin();
            it != mEngIntegrityRiskConfigMap.end(); ++it) {
        string pbStr;
        LocConfigEngineIntegrityRiskReqMsg msg(mSocketName, it->first, it->second, &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            sendConfigMsgToHalDaemon(CONFIG_ENGINE_INTEGRITY_RISK, pbStr, false);
        }
    }

    // resend XTRA status registration message request
    if (mRegisterXtraUpdate) {
        string pbStr;
        LocConfigRegisterXtraStatusUpdateReqMsg msg(mSocketName, &mPbufMsgConv);
        if (msg.serializeToProtobuf(pbStr)) {
            sendConfigMsgToHalDaemon(REGISTER_XTRA_STATUS_UPDATE, pbStr, false);
        }
    }
}

void LocationIntegrationApiImpl::addConfigReq(LocConfigTypeEnum configType) {
    LOC_LOGv("add configType %d", configType);

    auto configReqData = mConfigReqCntMap.find(configType);
    if (configReqData == std::end(mConfigReqCntMap)) {
        mConfigReqCntMap.emplace(configType, 1);
    } else{
        int newCnt = configReqData->second+1;
        mConfigReqCntMap.erase(configReqData);
        mConfigReqCntMap.emplace(configType, newCnt);
    }
}

void LocationIntegrationApiImpl::processConfigRespCb(const LocAPIGenericRespMsg* pRespMsg) {
    LocConfigTypeEnum configType = getLocConfigTypeFromMsgId(pRespMsg->msgId);
    LocIntegrationResponse intResponse = getLocIntegrationResponse(pRespMsg->err);
    LOC_LOGd("<<< response message id: %d, msg err: %d, "
             "config type: %d, int response %d",
             pRespMsg->msgId, pRespMsg->err, configType, intResponse);

    if (mIntegrationCbs.configCb) {
        auto configReqData = mConfigReqCntMap.find(configType);
        if (configReqData != std::end(mConfigReqCntMap)) {
            int reqCnt = configReqData->second;
            if (reqCnt > 0) {
                mIntegrationCbs.configCb(configType, intResponse);
            }
            mConfigReqCntMap.erase(configReqData);
            // there are still some request pending for this config type
            // need to store it in the map
            if (--reqCnt > 0) {
                mConfigReqCntMap.emplace(configType, reqCnt);
            }
        }
    }
}

// process queued reqs that are not able to sent to location hal daemon
bool LocationIntegrationApiImpl::processQueuedReqs() {
    bool queueNotEmpty = (mQueuedMsg.size() > 0);

    while (mQueuedMsg.size() > 0) {
        ProtoMsgInfo msg = mQueuedMsg.front();
        mQueuedMsg.pop();
        sendConfigMsgToHalDaemon(msg.configType, msg.protoStr, true);
    }
    return queueNotEmpty;
}

// flush all the pending config request if location hal daemon has crashed
// and restarted
void LocationIntegrationApiImpl::flushConfigReqs() {
    for (auto itr=mConfigReqCntMap.begin(); itr!=mConfigReqCntMap.end(); ++itr) {
         int reqCnt = itr->second;
         while (reqCnt-- > 0) {
             if (itr->first <= CONFIG_ENUM_MAX) {
                 // config command are cached, and will be re-issued when
                 // hal daemon crashed and then restarted
                 mIntegrationCbs.configCb(itr->first, LOC_INT_RESPONSE_SUCCESS);
             } else {
                 // get command are not cached, and will not be re-issued when
                 // hal daemon crashed and then restarted
                 mIntegrationCbs.configCb(itr->first, LOC_INT_RESPONSE_FAILURE);
             }
         }
    }
    mConfigReqCntMap.clear();
}

void LocationIntegrationApiImpl::processGetRobustLocationConfigRespCb(
        const LocConfigGetRobustLocationConfigRespMsg* pRespMsg) {

    LOC_LOGd("<<< response message id: %d, mask 0x%x, enabled: %d, enabledFor911: %d",
             pRespMsg->msgId,
             pRespMsg->mRobustLoationConfig.validMask,
             pRespMsg->mRobustLoationConfig.enabled,
             pRespMsg->mRobustLoationConfig.enabledForE911);

    if (mIntegrationCbs.getRobustLocationConfigCb) {
        // conversion between the enums
        RobustLocationConfig robustConfig = {};
        uint32_t validMask = 0;;
        if (pRespMsg->mRobustLoationConfig.validMask &
                GNSS_CONFIG_ROBUST_LOCATION_ENABLED_VALID_BIT) {
            validMask |= ROBUST_LOCATION_CONFIG_VALID_ENABLED;
            robustConfig.enabled = pRespMsg->mRobustLoationConfig.enabled;
        }
        if (pRespMsg->mRobustLoationConfig.validMask &
                GNSS_CONFIG_ROBUST_LOCATION_ENABLED_FOR_E911_VALID_BIT) {
            validMask |= ROBUST_LOCATION_CONFIG_VALID_ENABLED_FOR_E911;
            robustConfig.enabledForE911 = pRespMsg->mRobustLoationConfig.enabledForE911;
        }
        if (pRespMsg->mRobustLoationConfig.validMask &
                GNSS_CONFIG_ROBUST_LOCATION_VERSION_VALID_BIT) {
            validMask |= ROBUST_LOCATION_CONFIG_VALID_VERSION;
            robustConfig.version.major = pRespMsg->mRobustLoationConfig.version.major;
            robustConfig.version.minor = pRespMsg->mRobustLoationConfig.version.minor;
        }

        robustConfig.validMask = (RobustLocationConfigValidMask) validMask;
        mIntegrationCbs.getRobustLocationConfigCb(robustConfig);
    }
}

void LocationIntegrationApiImpl::processGetMinGpsWeekRespCb(
        const LocConfigGetMinGpsWeekRespMsg* pRespMsg) {

    LOC_LOGd("<<< response message id: %d, min gps week: %d",
             pRespMsg->msgId, pRespMsg->mMinGpsWeek);
    if (mIntegrationCbs.getMinGpsWeekCb) {
        mIntegrationCbs.getMinGpsWeekCb(pRespMsg->mMinGpsWeek);
    }
}

void LocationIntegrationApiImpl::processGetMinSvElevationRespCb(
        const LocConfigGetMinSvElevationRespMsg* pRespMsg) {

    LOC_LOGd("<<< response message id: %d, min sv elevation: %d",
             pRespMsg->msgId, pRespMsg->mMinSvElevation);
    if (mIntegrationCbs.getMinSvElevationCb) {
        mIntegrationCbs.getMinSvElevationCb(pRespMsg->mMinSvElevation);
    }
}

// This function returns true of the bit mask for the specified constellation type
// is set.
static bool isBitMaskSetForConstellation(
        GnssConstellationType type, GnssSvTypesMask mask) {

    bool retVal = false;
    if ((type == GNSS_CONSTELLATION_TYPE_GLONASS) && (mask & GNSS_SV_TYPES_MASK_GLO_BIT)) {
        retVal = true;
    } else if ((type == GNSS_CONSTELLATION_TYPE_BEIDOU) && (mask & GNSS_SV_TYPES_MASK_BDS_BIT)) {
        retVal = true;
    } else if ((type == GNSS_CONSTELLATION_TYPE_QZSS) && (mask & GNSS_SV_TYPES_MASK_QZSS_BIT)) {
        retVal = true;
    } else if ((type == GNSS_CONSTELLATION_TYPE_GALILEO) && (mask & GNSS_SV_TYPES_MASK_GAL_BIT)) {
        retVal = true;
    } else if ((type == GNSS_CONSTELLATION_TYPE_NAVIC) && (mask & GNSS_SV_TYPES_MASK_NAVIC_BIT)) {
        retVal = true;
    } else if ((type == GNSS_CONSTELLATION_TYPE_GPS) && (mask & GNSS_SV_TYPES_MASK_GPS_BIT)) {
        retVal = true;
    }

    return retVal;
}

void LocationIntegrationApiImpl::processGetConstellationSecondaryBandConfigRespCb(
    const LocConfigGetConstellationSecondaryBandConfigRespMsg* pRespMsg) {

    if (mIntegrationCbs.getConstellationSecondaryBandConfigCb) {
        ConstellationSet secondaryBandDisablementSet;

        if (pRespMsg->mSecondaryBandConfig.size != 0) {
            uint32_t constellationType = 0;
            GnssSvTypesMask secondaryBandDisabledMask =
                    pRespMsg->mSecondaryBandConfig.blacklistedSvTypesMask;

            LOC_LOGd("secondary band disabled mask: 0x%" PRIx64 "", secondaryBandDisabledMask);
            for (;constellationType <= GNSS_CONSTELLATION_TYPE_MAX; constellationType++) {
                if (isBitMaskSetForConstellation((GnssConstellationType) constellationType,
                                                 secondaryBandDisabledMask)) {
                    secondaryBandDisablementSet.emplace((GnssConstellationType) constellationType);
                }
            }
        }

        mIntegrationCbs.getConstellationSecondaryBandConfigCb(secondaryBandDisablementSet);
    }
}

void LocationIntegrationApiImpl::processGetXtraStatusRespCb(
        const LocConfigGetXtraStatusRespMsg* pRespMsg) {

    if (!mIntegrationCbs.getXtraStatusCb) {
        return;
    }

    XtraStatus xtraStatus = {};
    XtraStatusUpdateTrigger updateTrigger = (XtraStatusUpdateTrigger) 0;
    switch (pRespMsg->mUpdateType) {
    case ::XTRA_STATUS_UPDATE_UPON_QUERY:
        updateTrigger = XTRA_STATUS_UPDATE_UPON_QUERY;
        break;
    case ::XTRA_STATUS_UPDATE_UPON_REGISTRATION:
        updateTrigger = XTRA_STATUS_UPDATE_UPON_REGISTRATION;
        break;
    case ::XTRA_STATUS_UPDATE_UPON_STATUS_CHANGE:
        updateTrigger = XTRA_STATUS_UPDATE_UPON_STATUS_CHANGE;
        break;
    default:
        break;
    }

    LOC_LOGi("update type %d, register for update %d, register for update pending %d",
             updateTrigger, mRegisterXtraUpdate, mXtraUpdateUponRegisterPending);
    if (updateTrigger == XTRA_STATUS_UPDATE_UPON_REGISTRATION) {
        if ((mRegisterXtraUpdate == false) ||
            (mXtraUpdateUponRegisterPending == false)) {
            // if client has de-registered update or this is due to hal daemon restart
            return;
        }
        mXtraUpdateUponRegisterPending = false;
    }

    xtraStatus.featureEnabled = pRespMsg->mXtraStatus.featureEnabled;
    if (xtraStatus.featureEnabled == true) {
        switch (pRespMsg->mXtraStatus.xtraDataStatus) {
        case ::XTRA_DATA_STATUS_NOT_AVAIL:
            xtraStatus.xtraDataStatus = XTRA_DATA_STATUS_NOT_AVAIL;
            break;
        case ::XTRA_DATA_STATUS_NOT_VALID:
            xtraStatus.xtraDataStatus = XTRA_DATA_STATUS_NOT_VALID;
            break;
        case ::XTRA_DATA_STATUS_VALID:
            xtraStatus.xtraDataStatus = XTRA_DATA_STATUS_VALID;
            break;
        default:
            xtraStatus.xtraDataStatus = XTRA_DATA_STATUS_UNKNOWN;
            break;
        }

        if (xtraStatus.xtraDataStatus == XTRA_DATA_STATUS_VALID) {
            xtraStatus.xtraValidForHours = pRespMsg->mXtraStatus.xtraValidForHours;
        }
    }

    LOC_LOGd("send out xtra status: %d %d %d %d", updateTrigger, xtraStatus.featureEnabled,
             xtraStatus.xtraDataStatus, xtraStatus.xtraValidForHours);
    mIntegrationCbs.getXtraStatusCb(updateTrigger, xtraStatus);
}

void LocationIntegrationApiImpl::processRegisterGnssSignalTypesRespCb(
                        const LocConfigRegisterGnssSignalTypesUpdateRespMsg* msg) {
    if (mIntegrationCbs.gnssSignalTypesCb && msg) {
        uint32_t gnssSignalTypeMask = 0;
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_GPS_L1CA) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_GPS_L1CA_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_GPS_L1C) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_GPS_L1C_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_GPS_L2) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_GPS_L2_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_GPS_L5) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_GPS_L5_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_GLONASS_G1) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_GLONASS_G1_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_GLONASS_G2) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_GLONASS_G2_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_GALILEO_E1) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_GALILEO_E1_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_GALILEO_E5A) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_GALILEO_E5A_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_GALILEO_E5B) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_GALILEO_E5B_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B1I) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_BEIDOU_B1I_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B1C) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_BEIDOU_B1C_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B2I) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_BEIDOU_B2I_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B2AI) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_BEIDOU_B2AI_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_QZSS_L1CA) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_QZSS_L1CA_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_QZSS_L1S) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_QZSS_L1S_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_QZSS_L2) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_QZSS_L2_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_QZSS_L5) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_QZSS_L5_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_SBAS_L1) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_SBAS_L1_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_NAVIC_L5) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_NAVIC_L5_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B2AQ) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_BEIDOU_B2AQ_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B1) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_BEIDOU_B1_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B2) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_BEIDOU_B2_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B2BI) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_BEIDOU_B2BI_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_BEIDOU_B2BQ) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_BEIDOU_B2BQ_BIT;
        }
        if (msg->mSignalTypeMask & ::GNSS_SIGNAL_NAVIC_L1) {
            gnssSignalTypeMask |= location_client::GNSS_SIGNAL_NAVIC_L1_BIT;
        }
        LOC_LOGd("received GNSS signal Types : %x, send out supported GNSS signal types: %x",
                msg->mSignalTypeMask, gnssSignalTypeMask);
        mIntegrationCbs.gnssSignalTypesCb((location_client::GnssSignalTypeMask)gnssSignalTypeMask);
    }
}

uint32_t LocationIntegrationApiImpl::gnssInjectMmfData(const GnssMapMatchedData& mmfData) {
    struct InjectMmfDataReq : public LocMsg {
        InjectMmfDataReq(LocationIntegrationApiImpl* apiImpl,
                         const GnssMapMatchedData& mapData) :
                mApiImpl(apiImpl),
                mMapData(mapData) {}
        virtual ~InjectMmfDataReq() {}
        void proc() const {
            string pbStr;
            LocInjectMmfDataReqMsg msg(mApiImpl->mSocketName,
                    const_cast<GnssMapMatchedData&>(mMapData),
                    &mApiImpl->mPbufMsgConv);
            if (msg.serializeToProtobuf(pbStr)) {
                mApiImpl->sendConfigMsgToHalDaemon(CONFIG_MAP_MATCHED_FEEDBACK, pbStr);
            } else {
                LOC_LOGe("LocConfigAidingDataDeletionReqMsg serializeToProtobuf failed");
            }
        }
        LocationIntegrationApiImpl* mApiImpl;
        const GnssMapMatchedData mMapData;
    };
    mMsgTask.sendMsg(new (nothrow) InjectMmfDataReq(this, mmfData));

    return 0;
}

/******************************************************************************
LocationIntegrationApiImpl - Not implemented ILocationControlAPI functions
******************************************************************************/
uint32_t* LocationIntegrationApiImpl::gnssUpdateConfig(const GnssConfig& config) {
    (void)config;
    return nullptr;
}

static ILocationControlAPI* gLocationControlApiImpl = nullptr;
static mutex gMutexForCreate;
extern "C" ILocationControlAPI* getLocationIntegrationApiImpl()
{
    LocIntegrationCbs intCbs = {};
    lock_guard<mutex> lock(gMutexForCreate);

    if (nullptr == gLocationControlApiImpl) {
        gLocationControlApiImpl = new LocationIntegrationApiImpl(intCbs);
    }

    return gLocationControlApiImpl;
}

} // namespace location_integration
