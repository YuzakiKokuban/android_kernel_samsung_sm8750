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

#ifndef LOCATIONAPIMSG_H
#define LOCATIONAPIMSG_H

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <loc_pla.h> // for strlcpy
#include <gps_extended_c.h>
#include <log_util.h>
#include <LocIpc.h>
#include <LocationDataTypes.h>
#include <stdlib.h>

// Protobuf message headers
#include "LocationApiMsg.pb.h"
#include "LocationApiDataTypes.pb.h"

/******************************************************************************
Constants
******************************************************************************/
#define LOCATION_REMOTE_API_MSG_VERSION (1)

// Maximum fully qualified path(including the file name)
// for the location remote API service and client socket name
#define MAX_SOCKET_PATHNAME_LENGTH (128)
#define MAX_PROGRAM_NAME_LENGTH     (32)
#define SERVICE_NAME "locapiservice"

// Max number of geofence that can be added/removed etc.
#define MAX_GEOFENCE_ENTRY  (20)

#define LOCATION_CLIENT_SESSION_ID_INVALID (0)

#define LOCATION_CLIENT_API_QSOCKET_HALDAEMON_SERVICE_ID    (5001)
#define LOCATION_CLIENT_API_QSOCKET_HALDAEMON_INSTANCE_ID   (1)
#define LOCATION_CLIENT_API_QSOCKET_CLIENT_SERVICE_ID       (5002)

#define s_CLIENTAPI_LOCAL SOCKET_LOC_CLIENT_DIR  LOC_CLIENT_NAME_PREFIX
#define s_INTAPI_LOCAL    SOCKET_LOC_CLIENT_DIR  LOC_INTAPI_NAME_PREFIX
#define sEAP EAP_LOC_CLIENT_DIR LOC_CLIENT_NAME_PREFIX

using namespace std;
using namespace loc_util;

class SockNode {
    const int32_t mId1;
    const int32_t mId2;
    const string mNodePathnamePrefix;

public:
    enum Type { Local, Eap, Other };
    SockNode(int32_t id1, int32_t mId2, const string&& prefix) :
            mId1(id1), mId2(mId2), mNodePathnamePrefix(prefix) {
    }
    SockNode(SockNode&& node) :
            SockNode(node.mId1, node.mId2, move(node.mNodePathnamePrefix)) {
    }
    static int getId1Id2(const char* fullPathName, int32_t length, int32_t& id1, int32_t& id2) {
        uint32_t count = 0;
        int32_t indx = 0;

        if (nullptr == fullPathName) {
            fullPathName = "";
        }
        indx = (length < 0) ? (strlen(fullPathName) - 1) : length;

        while (count < 2 && indx >= 0) {
            if ('.' == fullPathName[indx]) {
                count++;
            }
            indx--;
        }
        if (count == 2) {
            indx++;
            sscanf(fullPathName+indx+1, "%d.%d", &id1, &id2);
        } else {
            indx = 0;
        }
        return indx;
    }

    static SockNode create(const string fullPathName) {
        return create(fullPathName.c_str(), fullPathName.size());
    }

    static SockNode create(const char* fullPathName, int32_t length = -1) {
        int32_t indx = 0, id1 = -1, id2 = -1;
        indx = getId1Id2(fullPathName, length, id1, id2);

        return SockNode(id1, id2, string(fullPathName, indx));
    }

    inline int getId1() const { return mId1; }
    inline int getId2() const { return mId2; }
    inline const string& getNodePathnamePrefix() const { return mNodePathnamePrefix; }
    inline string getNodePathname() const {
        return string(mNodePathnamePrefix).append(1, '.').append(to_string(mId1))
            .append(1, '.').append(to_string(mId2));
    }
    inline Type getNodeType() const {
        Type type = Other;
        if (mNodePathnamePrefix.compare(0, sizeof(SOCKET_LOC_CLIENT_DIR)-1,
                                        SOCKET_LOC_CLIENT_DIR) == 0) {
            type = Local;
        } else if (mNodePathnamePrefix.compare(0, sizeof(sEAP)-1, sEAP) == 0) {
            type = Eap;
        }
        return type;
    }
    inline shared_ptr<LocIpcSender> createSender() {
        const string socket = getNodePathname();
        const char* sock = socket.c_str();
        switch (getNodeType()) {
        case SockNode::Local:
            return LocIpc::getLocIpcLocalSender(sock);
        case SockNode::Eap:
            return LocIpc::getLocIpcQrtrSender(getId1(), getId2());
        default:
            return nullptr;
        }
    }
};

enum ClientType {
    LOCATION_CLIENT_API = 1,
    LOCATION_INTEGRATION_API = 2,
};

class SockNodeLocal : public SockNode {
public:
    SockNodeLocal(ClientType type, int32_t pid, int32_t tid) :
        SockNode(pid, tid, getSockNodeLocalPrefix(type)) {}

    inline static string getSockNodeLocalPrefix(ClientType type) {
        int program_name_length = 0;
        const char * progName = getprogname();
        if (nullptr != progName) {
            program_name_length = strlen(progName);
            if (program_name_length > MAX_PROGRAM_NAME_LENGTH) {
                program_name_length = MAX_PROGRAM_NAME_LENGTH;
            }
        }

        if (LOCATION_CLIENT_API == type) {
            return string(s_CLIENTAPI_LOCAL).append(1, '_').append(progName, program_name_length);
        } else {
            return string(s_INTAPI_LOCAL).append(1, '_').append(progName, program_name_length);
        }
    }
};


class SockNodeEap : public SockNode {
public:
    SockNodeEap(int32_t service, int32_t instance) :
        SockNode(service, instance, getSockNodeEapPrefix()) {}

    inline static string getSockNodeEapPrefix() {
        int program_name_length = 0;
        const char * progName = getprogname();
        if (nullptr != progName) {
            program_name_length = strlen(progName);

            if (program_name_length > MAX_PROGRAM_NAME_LENGTH) {
                program_name_length = MAX_PROGRAM_NAME_LENGTH;
            }
        }

        return string(sEAP).append(1, '_').append(progName, program_name_length);
    }
};

/******************************************************************************
List of message IDs supported by Location Remote API
******************************************************************************/
enum ELocMsgID {
    E_LOCAPI_UNDEFINED_MSG_ID = 0,

    // registration
    E_LOCAPI_CLIENT_REGISTER_MSG_ID = 1,
    E_LOCAPI_CLIENT_DEREGISTER_MSG_ID = 2,
    E_LOCAPI_CAPABILILTIES_MSG_ID = 3,
    E_LOCAPI_HAL_READY_MSG_ID = 4,

    // tracking session
    E_LOCAPI_START_TRACKING_MSG_ID = 5,
    E_LOCAPI_STOP_TRACKING_MSG_ID = 6,
    E_LOCAPI_UPDATE_CALLBACKS_MSG_ID = 7,
    E_LOCAPI_UPDATE_TRACKING_OPTIONS_MSG_ID = 8,

    // control
    E_LOCAPI_CONTROL_UPDATE_CONFIG_MSG_ID = 9, // this message id has been deprecated
    E_LOCAPI_CONTROL_DELETE_AIDING_DATA_MSG_ID = 10, // this message id has been deprecated
    E_LOCAPI_CONTROL_UPDATE_NETWORK_AVAILABILITY_MSG_ID = 11,

    // Position reports
    E_LOCAPI_LOCATION_MSG_ID = 12,
    E_LOCAPI_LOCATION_INFO_MSG_ID = 13,
    E_LOCAPI_SATELLITE_VEHICLE_MSG_ID = 14,
    E_LOCAPI_NMEA_MSG_ID = 15,
    E_LOCAPI_DATA_MSG_ID = 16,

    // Get API to retrieve info from GNSS engine
    E_LOCAPI_GET_GNSS_ENGERY_CONSUMED_MSG_ID = 17,

    E_LOCAPI_LOCATION_SYSTEM_INFO_MSG_ID = 18,

    // engine position report
    E_LOCAPI_ENGINE_LOCATIONS_INFO_MSG_ID = 19,

    // batching session
    E_LOCAPI_START_BATCHING_MSG_ID = 20,
    E_LOCAPI_STOP_BATCHING_MSG_ID = 21,
    E_LOCAPI_UPDATE_BATCHING_OPTIONS_MSG_ID = 22,

    //batching reports
    E_LOCAPI_BATCHING_MSG_ID = 23,

    // geofence session
    E_LOCAPI_ADD_GEOFENCES_MSG_ID = 24,
    E_LOCAPI_REMOVE_GEOFENCES_MSG_ID = 25,
    E_LOCAPI_MODIFY_GEOFENCES_MSG_ID = 26,
    E_LOCAPI_PAUSE_GEOFENCES_MSG_ID = 27,
    E_LOCAPI_RESUME_GEOFENCES_MSG_ID = 28,

    //geofence breach
    E_LOCAPI_GEOFENCE_BREACH_MSG_ID = 29,

    // Measurement reports
    E_LOCAPI_MEAS_MSG_ID = 30,

    // Terrestria fix request/response msg
    E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_REQ_MSG_ID = 31,
    E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_RESP_MSG_ID = 32,

    // Debug Report
    E_LOCAPI_GET_DEBUG_REQ_MSG_ID = 33,
    E_LOCAPI_GET_DEBUG_RESP_MSG_ID = 34,

    // Disater and crisis reports
    E_LOCAPI_DC_REPORT_MSG_ID = 35,

    // Antenna Info
    E_LOCAPI_GET_ANTENNA_INFO_MSG_ID = 36,
    E_LOCAPI_ANTENNA_INFO_MSG_ID = 37,

    // Single fix request/response msg
    E_LOCAPI_GET_SINGLE_POS_REQ_MSG_ID = 38,
    E_LOCAPI_GET_SINGLE_POS_RESP_MSG_ID = 39,

    // Ephemeris Reporting
    E_LOCAPI_EPH_MSG_ID = 40,

    // ping
    E_LOCAPI_PINGTEST_MSG_ID = 99,

    // integration API config request
    E_INTAPI_CONFIG_CONSTRAINTED_TUNC_MSG_ID = 200,
    E_INTAPI_CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR_MSG_ID = 201,
    E_INTAPI_CONFIG_SV_CONSTELLATION_MSG_ID  = 202,
    E_INTAPI_CONFIG_AIDING_DATA_DELETION_MSG_ID  = 203,
    E_INTAPI_CONFIG_LEVER_ARM_MSG_ID  = 204,
    E_INTAPI_CONFIG_ROBUST_LOCATION_MSG_ID  = 205,
    E_INTAPI_CONFIG_MIN_GPS_WEEK_MSG_ID  = 206,
    E_INTAPI_CONFIG_DEAD_RECKONING_ENGINE_MSG_ID = 207,
    E_INTAPI_CONFIG_MIN_SV_ELEVATION_MSG_ID = 208,
    E_INTAPI_CONFIG_CONSTELLATION_SECONDARY_BAND_MSG_ID  = 209,
    E_INTAPI_CONFIG_ENGINE_RUN_STATE_MSG_ID = 210,
    E_INTAPI_CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING_MSG_ID = 211,
    E_INTAPI_CONFIG_OUTPUT_NMEA_TYPES_MSG_ID = 212,
    E_INTAPI_CONFIG_ENGINE_INTEGRITY_RISK_MSG_ID = 213,
    E_INTAPI_INJECT_LOCATION_MSG_ID = 214,
    E_INTAPI_CONFIG_XTRA_PARAMS_MSG_ID = 215,
    E_INTAPI_CONFIG_MERKLE_TREE_MSG_ID = 216,
    E_INTAPI_CONFIG_OSNMA_ENABLEMENT_MSG_ID = 217,
    E_INTAPI_CONFIG_MAP_MATCHED_FEEDBACK_MSG_ID = 218,

    // integration API config retrieval request/response
    E_INTAPI_GET_ROBUST_LOCATION_CONFIG_REQ_MSG_ID  = 300,
    E_INTAPI_GET_ROBUST_LOCATION_CONFIG_RESP_MSG_ID  = 301,

    E_INTAPI_GET_MIN_GPS_WEEK_REQ_MSG_ID  = 302,
    E_INTAPI_GET_MIN_GPS_WEEK_RESP_MSG_ID  = 303,

    E_INTAPI_GET_MIN_SV_ELEVATION_REQ_MSG_ID  = 304,
    E_INTAPI_GET_MIN_SV_ELEVATION_RESP_MSG_ID  = 305,

    E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_REQ_MSG_ID = 306,
    E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_RESP_MSG_ID = 307,

    E_INTAPI_GET_XTRA_STATUS_REQ_MSG_ID = 308,
    E_INTAPI_GET_XTRA_STATUS_RESP_MSG_ID = 309,

    E_INTAPI_REGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID = 310,
    E_INTAPI_DEREGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID = 311,

    E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_REQ_MSG_ID = 312,
    E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_RESP_MSG_ID = 313,
};

const char* LocApiMsgString(ELocMsgID msgId);

typedef uint32_t LocationCallbacksMask;
enum ELocationCallbacksOption {
    E_LOC_CB_TRACKING_BIT               = (1<<0), /**< Register for DBT location report */
    E_LOC_CB_GNSS_LOCATION_INFO_BIT     = (1<<1), /**< Register for GNSS Location */
    E_LOC_CB_GNSS_SV_BIT                = (1<<2), /**< Register for GNSS SV */
    E_LOC_CB_GNSS_NMEA_BIT              = (1<<3), /**< Register for GNSS NMEA */
    E_LOC_CB_GNSS_DATA_BIT              = (1<<4), /**< Register for GNSS DATA */
    E_LOC_CB_SYSTEM_INFO_BIT            = (1<<5),  /**< Register for Location system info */
    E_LOC_CB_BATCHING_BIT               = (1<<6), /**< Register for Batching */
    E_LOC_CB_BATCHING_STATUS_BIT        = (1<<7), /**< Register for Batching  Status*/
    E_LOC_CB_GEOFENCE_BREACH_BIT        = (1<<8), /**< Register for Geofence Breach */
    E_LOC_CB_ENGINE_LOCATIONS_INFO_BIT  = (1<<9), /**< Register for multiple engine reports */
    E_LOC_CB_SIMPLE_LOCATION_INFO_BIT   = (1<<10), /**< Register for simple location */
    E_LOC_CB_GNSS_MEAS_BIT              = (1<<11), /**< Register for GNSS Measurements */
    E_LOC_CB_GNSS_NHZ_MEAS_BIT          = (1<<12), /**< Register for NHZ GNSS Measurements */
    E_LOC_CB_GNSS_DC_REPORT_BIT         = (1<<13), /**< Register for disaster and crisis reports */
    E_LOC_CB_ANTENNA_INFO_BIT           = (1<<14), /**< Register for Antenna Info */
    E_LOC_CB_ENGINE_NMEA_BIT            = (1<<15), /**< Register for Engine NMEA */
    E_LOC_CB_GNSS_EPH_BIT               = (1<<16) /**< Register for Engine NMEA */
};

// Mask related to all info that are tied with a position session and need to be unsubscribed
// when session is stopped
#define LOCATION_SESSON_ALL_INFO_MASK (E_LOC_CB_TRACKING_BIT|\
                                       E_LOC_CB_GNSS_LOCATION_INFO_BIT|\
                                       E_LOC_CB_GNSS_SV_BIT|E_LOC_CB_GNSS_NMEA_BIT|\
                                       E_LOC_CB_GNSS_DATA_BIT|E_LOC_CB_GNSS_MEAS_BIT|\
                                       E_LOC_CB_GNSS_NHZ_MEAS_BIT|\
                                       E_LOC_CB_ENGINE_LOCATIONS_INFO_BIT|\
                                       E_LOC_CB_SIMPLE_LOCATION_INFO_BIT |\
                                       E_LOC_CB_GNSS_DC_REPORT_BIT |\
                                       E_LOC_CB_ENGINE_NMEA_BIT |\
                                       E_LOC_CB_GNSS_EPH_BIT)

#define LOCATION_BATCHING_SESSION_MASK (E_LOC_CB_BATCHING_BIT|\
                                        E_LOC_CB_BATCHING_STATUS_BIT)

#define LOCATION_GEOFENCE_SESSION_MASK (E_LOC_CB_GEOFENCE_BREACH_BIT)

typedef uint32_t EngineInfoCallbacksMask;
enum EEngineInfoCallbacksMask {
    // gnss energy consumed, once the info is delivered,
    // this bit will be cleared
    E_ENGINE_INFO_CB_GNSS_ENERGY_CONSUMED_BIT = (1<<0) /**< GNSS energy consumed */
};

/******************************************************************************
Common data structure
******************************************************************************/
struct LocAPINmeaSerializedPayload {
    // do not use size_t as data type for size_t is architecture dependent
    uint32_t size;
    uint64_t timestamp;
    string nmea;
    LocOutputEngineType locOutputEngType;
};

struct LocAPIBatchNotification {
    BatchingStatus status;
    std::vector<Location> location;
};

struct LocAPIGeofenceBreachNotification {
    uint64_t timestamp;
    GeofenceBreachTypeMask type; //type of breach
    Location location;   //location associated with breach
    std::vector<uint32_t> id;
};

struct GeofencePayload {
    uint32_t gfClientId;
    GeofenceOption gfOption;
    GeofenceInfo gfInfo;
};

struct GeofencesAddedReqPayload {
    uint32_t count;
    GeofencePayload gfPayload[MAX_GEOFENCE_ENTRY];
};

struct GeofencesReqClientIdPayload {
    uint32_t count;
    uint32_t gfIds[MAX_GEOFENCE_ENTRY];
};

struct GeofenceResponse {
    uint32_t clientId;
    LocationError error;
};

struct CollectiveResPayload {
    std::vector<GeofenceResponse> resp;
};

struct AntennaInformation {
    std::vector<GnssAntennaInformation> antennaInfos;
};
/******************************************************************************
IPC message header structure
******************************************************************************/
class LocationApiPbMsgConv;
struct LocAPIMsgHeader
{
    char       mSocketName[MAX_SOCKET_PATHNAME_LENGTH]; /**< Processor string */
    ELocMsgID  msgId;               /**< LocationMsgID */
    uint32_t   msgVersion;          /**< Location remote API message version */
    const LocationApiPbMsgConv *pLocApiPbMsgConv; /**< Protobuf converter */

    inline LocAPIMsgHeader(const char* name, ELocMsgID msgId):
        msgId(msgId),
        pLocApiPbMsgConv(nullptr),
        msgVersion(LOCATION_REMOTE_API_MSG_VERSION) {
            memset(mSocketName, 0, MAX_SOCKET_PATHNAME_LENGTH);
            strlcpy(mSocketName, name, MAX_SOCKET_PATHNAME_LENGTH);
        }

    inline LocAPIMsgHeader(const char* name, ELocMsgID msgId,
            const LocationApiPbMsgConv* locApiPbConv):
        msgId(msgId),
        pLocApiPbMsgConv(locApiPbConv),
        msgVersion(LOCATION_REMOTE_API_MSG_VERSION) {
            memset(mSocketName, 0, MAX_SOCKET_PATHNAME_LENGTH);
            strlcpy(mSocketName, name, MAX_SOCKET_PATHNAME_LENGTH);
        }

    virtual ~LocAPIMsgHeader() = default;

    /** Serialize message to protobuf format. Return length of serialized string.*/
    virtual int serializeToProtobuf(string& protoStr) {return 0;}

    inline bool isValidMsg(uint32_t msgSize) {
        bool msgValid = true;
        if (msgVersion != LOCATION_REMOTE_API_MSG_VERSION) {
            LOC_LOGv("msg id %d, msg version %d not matching with expected version %d",
                     msgId, msgVersion, LOCATION_REMOTE_API_MSG_VERSION);
             msgValid = false;
        }
        return msgValid;
    }

    bool isValidClientMsg(uint32_t msgSize) {
        bool msgValid = isValidMsg(msgSize);
        if ((true== msgValid) &&
                ((strncmp(mSocketName, SOCKET_LOC_CLIENT_DIR,
                          sizeof(SOCKET_LOC_CLIENT_DIR)-1) != 0) &&
                 (strncmp(mSocketName, EAP_LOC_CLIENT_DIR,
                          sizeof(EAP_LOC_CLIENT_DIR)-1) != 0))) {
            LOC_LOGv("msg not from expected client");
            msgValid = false;
        }

        return msgValid;
    }

    bool isValidServerMsg(uint32_t msgSize) {
        bool msgValid = isValidMsg(msgSize);
        if ((true== msgValid) &&
                (strncmp(mSocketName, SERVICE_NAME, sizeof(SERVICE_NAME)) != 0)) {
            LOC_LOGe("msg not from expected server %s", SERVICE_NAME);
            msgValid = false;
        }

        return msgValid;
    }
};

/******************************************************************************
IPC message structure - client registration
******************************************************************************/
// defintion for message with msg id of E_LOCAPI_CLIENT_REGISTER_MSG_ID
struct LocAPIClientRegisterReqMsg: LocAPIMsgHeader
{
    ClientType mClientType;

    inline LocAPIClientRegisterReqMsg(const char* name, ClientType clientType,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_CLIENT_REGISTER_MSG_ID, pbMsgConv),
        mClientType(clientType) { }
    LocAPIClientRegisterReqMsg(const char* name,
            const PBLocAPIClientRegisterReqMsg &pbLocApiClientRegReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_CLIENT_DEREGISTER_MSG_ID
struct LocAPIClientDeregisterReqMsg: LocAPIMsgHeader
{
    inline LocAPIClientDeregisterReqMsg(const char* name, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_CLIENT_DEREGISTER_MSG_ID, pbMsgConv) { }

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_CAPABILILTIES_MSG_ID
struct LocAPICapabilitiesIndMsg: LocAPIMsgHeader
{
    LocationCapabilitiesMask capabilitiesMask;

    inline LocAPICapabilitiesIndMsg(const char* name,
        LocationCapabilitiesMask capabilitiesMask, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_CAPABILILTIES_MSG_ID, pbMsgConv),
        capabilitiesMask(capabilitiesMask) { }
    LocAPICapabilitiesIndMsg(const char* name,
            const PBLocAPICapabilitiesIndMsg &pbLocApiCapInd,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_HAL_READY_MSG_ID
struct LocAPIHalReadyIndMsg: LocAPIMsgHeader
{
    inline LocAPIHalReadyIndMsg(const char* name, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_HAL_READY_MSG_ID, pbMsgConv) { }

    int serializeToProtobuf(string& protoStr) override;
};

/******************************************************************************
IPC message structure - generic response
******************************************************************************/
struct LocAPIGenericRespMsg: LocAPIMsgHeader
{
    LocationError err;

    inline LocAPIGenericRespMsg(const char* name, ELocMsgID msgId, LocationError err,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, msgId, pbMsgConv),
        err(err) { }
    LocAPIGenericRespMsg(const char* name, ELocMsgID msgId,
            const PBLocAPIGenericRespMsg &pbLocApiGenericRsp,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};
struct LocAPICollectiveRespMsg: LocAPIMsgHeader
{
    CollectiveResPayload collectiveRes;

    inline LocAPICollectiveRespMsg(const char* name, ELocMsgID msgId,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, msgId, pbMsgConv) { }
    inline LocAPICollectiveRespMsg(const char* name, ELocMsgID msgId,
            CollectiveResPayload& response, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, msgId, pbMsgConv),
        collectiveRes(response) { }
    LocAPICollectiveRespMsg(const char* name, ELocMsgID msgId,
            const PBLocAPICollectiveRespMsg &pbLocApiCollctvRespMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};


/******************************************************************************
IPC message structure - tracking
******************************************************************************/
// defintion for message with msg id of E_LOCAPI_START_TRACKING_MSG_ID
struct LocAPIStartTrackingReqMsg: LocAPIMsgHeader
{
    LocationOptions locOptions;

    inline LocAPIStartTrackingReqMsg(const char* name,
                                     const LocationOptions & locSessionOptions,
                                     const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_START_TRACKING_MSG_ID, pbMsgConv),
        locOptions(locSessionOptions) { }
    LocAPIStartTrackingReqMsg(const char* name,
            const PBLocAPIStartTrackingReqMsg &pbStartTrackReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_STOP_TRACKING_MSG_ID
struct LocAPIStopTrackingReqMsg: LocAPIMsgHeader
{
    bool clearSubscriptions;

    inline LocAPIStopTrackingReqMsg(const char* name, const LocationApiPbMsgConv *pbMsgConv,
                                    bool clearSubsc) :
        LocAPIMsgHeader(name, E_LOCAPI_STOP_TRACKING_MSG_ID, pbMsgConv),
        clearSubscriptions(clearSubsc) { }

    LocAPIStopTrackingReqMsg(const char* name,
        const PBLocAPIStopTrackingReqMsg  &pbStopTrackingReqMsg,
        const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_UPDATE_CALLBACKS_MSG_ID
struct LocAPIUpdateCallbacksReqMsg: LocAPIMsgHeader
{
    LocationCallbacksMask    locationCallbacks;

    inline LocAPIUpdateCallbacksReqMsg(const char* name,
                                       LocationCallbacksMask callBacksMask,
                                       const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_UPDATE_CALLBACKS_MSG_ID, pbMsgConv),
        locationCallbacks(callBacksMask) { }
    LocAPIUpdateCallbacksReqMsg(const char* name,
            const PBLocAPIUpdateCallbacksReqMsg &pbLocApiUpdateCbsReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_UPDATE_TRACKING_OPTIONS_MSG_ID
struct LocAPIUpdateTrackingOptionsReqMsg: LocAPIMsgHeader
{
    LocationOptions locOptions;

    inline LocAPIUpdateTrackingOptionsReqMsg(const char* name,
                                             const LocationOptions & locSessionOptions,
                                             const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_UPDATE_TRACKING_OPTIONS_MSG_ID, pbMsgConv),
        locOptions(locSessionOptions) { }
    LocAPIUpdateTrackingOptionsReqMsg(const char* name,
            const PBLocAPIUpdateTrackingOptionsReqMsg &pbUpdateTrackOptReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

/******************************************************************************
IPC message structure - batching
******************************************************************************/
// defintion for message with msg id of E_LOCAPI_START_BATCHING_MSG_ID
struct LocAPIStartBatchingReqMsg: LocAPIMsgHeader
{
    uint32_t intervalInMs;
    uint32_t distanceInMeters;
    BatchingMode batchingMode;

    inline LocAPIStartBatchingReqMsg(const char* name,
                                     uint32_t minInterval,
                                     uint32_t minDistance,
                                     BatchingMode batchMode,
                                     const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_START_BATCHING_MSG_ID, pbMsgConv),
        intervalInMs(minInterval),
        distanceInMeters(minDistance),
        batchingMode(batchMode) { }
    LocAPIStartBatchingReqMsg(const char* name,
            const PBLocAPIStartBatchingReqMsg &pbStartBatchReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_STOP_BATCHING_MSG_ID
struct LocAPIStopBatchingReqMsg: LocAPIMsgHeader
{
    inline LocAPIStopBatchingReqMsg(const char* name, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_STOP_BATCHING_MSG_ID, pbMsgConv) { }

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_UPDATE_BATCHING_OPTIONS_MSG_ID
struct LocAPIUpdateBatchingOptionsReqMsg: LocAPIMsgHeader
{
    uint32_t intervalInMs;
    uint32_t distanceInMeters;
    BatchingMode batchingMode;

    inline LocAPIUpdateBatchingOptionsReqMsg(const char* name,
                                             uint32_t sessionInterval,
                                             uint32_t sessionDistance,
                                             BatchingMode batchMode,
                                             const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_UPDATE_BATCHING_OPTIONS_MSG_ID, pbMsgConv),
        intervalInMs(sessionInterval),
        distanceInMeters(sessionDistance),
        batchingMode(batchMode) { }
    LocAPIUpdateBatchingOptionsReqMsg(const char* name,
            const PBLocAPIUpdateBatchingOptionsReqMsg &pbUpdateBatchOptiReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

/******************************************************************************
IPC message structure - geofence
******************************************************************************/
// defintion for message with msg id of E_LOCAPI_ADD_GEOFENCES_MSG_ID
struct LocAPIAddGeofencesReqMsg: LocAPIMsgHeader
{
    GeofencesAddedReqPayload geofences;
    inline LocAPIAddGeofencesReqMsg(const char* name, GeofencesAddedReqPayload& geofencesAdded,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_ADD_GEOFENCES_MSG_ID, pbMsgConv),
        geofences(geofencesAdded) { }
    LocAPIAddGeofencesReqMsg(const char* name, const PBLocAPIAddGeofencesReqMsg &pbAddGfsReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};
// defintion for message with msg id of E_LOCAPI_REMOVE_GEOFENCES_MSG_ID
struct LocAPIRemoveGeofencesReqMsg: LocAPIMsgHeader
{
    GeofencesReqClientIdPayload gfClientIds;
    inline LocAPIRemoveGeofencesReqMsg(const char* name, GeofencesReqClientIdPayload& ids,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_REMOVE_GEOFENCES_MSG_ID, pbMsgConv),
        gfClientIds(ids) { }
    LocAPIRemoveGeofencesReqMsg(const char* name,
            const PBLocAPIRemoveGeofencesReqMsg &pbRemGfsReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};
// defintion for message with msg id of E_LOCAPI_MODIFY_GEOFENCES_MSG_ID
struct LocAPIModifyGeofencesReqMsg: LocAPIMsgHeader
{
    GeofencesAddedReqPayload geofences;

    inline LocAPIModifyGeofencesReqMsg(const char* name,
                                       GeofencesAddedReqPayload& geofencesModified,
                                       const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_MODIFY_GEOFENCES_MSG_ID, pbMsgConv),
        geofences(geofencesModified) { }
    LocAPIModifyGeofencesReqMsg(const char* name,
            const PBLocAPIModifyGeofencesReqMsg &pbModifyGfsReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};
// defintion for message with msg id of E_LOCAPI_PAUSE_GEOFENCES_MSG_ID
struct LocAPIPauseGeofencesReqMsg: LocAPIMsgHeader
{
    GeofencesReqClientIdPayload gfClientIds;
    inline LocAPIPauseGeofencesReqMsg(const char* name,
                                      GeofencesReqClientIdPayload& ids,
                                      const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_PAUSE_GEOFENCES_MSG_ID, pbMsgConv),
        gfClientIds(ids) { }
    LocAPIPauseGeofencesReqMsg(const char* name,
            const PBLocAPIPauseGeofencesReqMsg &pbPauseGfsReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};
// defintion for message with msg id of E_LOCAPI_RESUME_GEOFENCES_MSG_ID
struct LocAPIResumeGeofencesReqMsg: LocAPIMsgHeader
{
    GeofencesReqClientIdPayload gfClientIds;
    inline LocAPIResumeGeofencesReqMsg(const char* name,
                                      GeofencesReqClientIdPayload& ids,
                                      const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_RESUME_GEOFENCES_MSG_ID, pbMsgConv),
        gfClientIds(ids) { }
    LocAPIResumeGeofencesReqMsg(const char* name,
            const PBLocAPIResumeGeofencesReqMsg &pbResumeGfsReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

/******************************************************************************
IPC message structure - control
******************************************************************************/
struct LocAPIUpdateNetworkAvailabilityReqMsg: LocAPIMsgHeader
{
    bool mAvailability;

    inline LocAPIUpdateNetworkAvailabilityReqMsg(const char* name, bool availability,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_CONTROL_UPDATE_NETWORK_AVAILABILITY_MSG_ID, pbMsgConv),
        mAvailability(availability) { }
    LocAPIUpdateNetworkAvailabilityReqMsg(const char* name,
            const PBLocAPIUpdateNetworkAvailabilityReqMsg &pbUpdateNetAvailReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocAPIGetGnssEnergyConsumedReqMsg: LocAPIMsgHeader
{
    inline LocAPIGetGnssEnergyConsumedReqMsg(const char* name,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_GET_GNSS_ENGERY_CONSUMED_MSG_ID, pbMsgConv) { }

    int serializeToProtobuf(string& protoStr) override;
};

struct LocAPIGetSingleTerrestrialPosReqMsg: LocAPIMsgHeader
{
    uint32_t            mTimeoutMsec;
    TerrestrialTechMask mTechMask;
    float               mHorQoS;

    inline LocAPIGetSingleTerrestrialPosReqMsg(
            const char* name, uint32_t timeoutMsec, TerrestrialTechMask techMask,
            float horQoS, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_REQ_MSG_ID, pbMsgConv),
        mTimeoutMsec(timeoutMsec), mTechMask(techMask), mHorQoS(horQoS) { }

    LocAPIGetSingleTerrestrialPosReqMsg(const char* name,
            const PBLocAPIGetSingleTerrestrialPosReqMsg &pbLocGetTerrestrialPosReq,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocAPIGetSingleTerrestrialPosRespMsg: LocAPIMsgHeader
{
    LocationError mErrorCode;
    Location      mLocation;

    inline LocAPIGetSingleTerrestrialPosRespMsg(
            const char* name, LocationError errorCode, Location location,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_RESP_MSG_ID, pbMsgConv),
        mErrorCode(errorCode), mLocation(location) { }

    LocAPIGetSingleTerrestrialPosRespMsg(const char* name,
            const PBLocAPIGetSingleTerrestrialPosRespMsg &pbLocGetTerrestrialPosResp,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocAPIGetSinglePosReqMsg: LocAPIMsgHeader
{
    uint32_t mTimeoutMsec;
    float    mHorQoS;

    inline LocAPIGetSinglePosReqMsg(
            const char* name, uint32_t timeoutMsec,
            float horQoS, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_GET_SINGLE_POS_REQ_MSG_ID, pbMsgConv),
        mTimeoutMsec(timeoutMsec), mHorQoS(horQoS) { }

    LocAPIGetSinglePosReqMsg(const char* name,
            const PBLocAPIGetSinglePosReqMsg &pbLocGetPosReq,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocAPIGetSinglePosRespMsg: LocAPIMsgHeader
{
    LocationError mErrorCode;
    Location      mLocation;

    inline LocAPIGetSinglePosRespMsg(
            const char* name, LocationError errorCode, Location location,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_GET_SINGLE_POS_RESP_MSG_ID, pbMsgConv),
        mErrorCode(errorCode), mLocation(location) { }

    LocAPIGetSinglePosRespMsg(const char* name,
            const PBLocAPIGetSinglePosRespMsg &pbLocGetPosResp,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

/******************************************************************************
IPC message structure - indications
******************************************************************************/
// defintion for message with msg id of E_LOCAPI_LOCATION_MSG_ID
struct LocAPILocationIndMsg: LocAPIMsgHeader
{
    Location locationNotification;

    inline LocAPILocationIndMsg(const char* name,
        const Location& location, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_LOCATION_MSG_ID, pbMsgConv),
        locationNotification(location) { }
    LocAPILocationIndMsg(const char* name, const PBLocAPILocationIndMsg &pbLocApiLocIndMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_BATCHING_MSG_ID
struct LocAPIBatchingIndMsg: LocAPIMsgHeader
{
    LocAPIBatchNotification batchNotification;
    BatchingMode batchingMode;

    inline LocAPIBatchingIndMsg(const char* name, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_BATCHING_MSG_ID, pbMsgConv) { }
    inline LocAPIBatchingIndMsg(const char* name, LocAPIBatchNotification& batchNotif,
            BatchingMode mode, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_BATCHING_MSG_ID, pbMsgConv), batchingMode(mode),
        batchNotification(batchNotif) { }
    LocAPIBatchingIndMsg(const char* name, const PBLocAPIBatchingIndMsg &pbLocApiBatchingIndMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_GEOFENCE_BREACH_MSG_ID
struct LocAPIGeofenceBreachIndMsg: LocAPIMsgHeader
{
    LocAPIGeofenceBreachNotification gfBreachNotification;

    inline LocAPIGeofenceBreachIndMsg(const char* name, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_GEOFENCE_BREACH_MSG_ID, pbMsgConv) { }
    inline LocAPIGeofenceBreachIndMsg(const char* name,
            LocAPIGeofenceBreachNotification& gfBreachNotif,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_GEOFENCE_BREACH_MSG_ID, pbMsgConv),
        gfBreachNotification(gfBreachNotif) { }
    LocAPIGeofenceBreachIndMsg(const char* name,
            const PBLocAPIGeofenceBreachIndMsg &pbLocApiGfBreachIndMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_LOCATION_INFO_MSG_ID
struct LocAPILocationInfoIndMsg: LocAPIMsgHeader
{
    GnssLocationInfoNotification gnssLocationInfoNotification;

    inline LocAPILocationInfoIndMsg(const char* name,
        const GnssLocationInfoNotification& locationInfo,
        const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_LOCATION_INFO_MSG_ID, pbMsgConv),
        gnssLocationInfoNotification(locationInfo) { }
    LocAPILocationInfoIndMsg(const char* name,
            const PBLocAPILocationInfoIndMsg &pbLocApiLocInfoIndMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_ENGINE_LOCATIONS_INFO_MSG_ID
struct LocAPIEngineLocationsInfoIndMsg: LocAPIMsgHeader
{
    uint32_t count;
    GnssLocationInfoNotification engineLocationsInfo[LOC_OUTPUT_ENGINE_COUNT];

    inline LocAPIEngineLocationsInfoIndMsg(
            const char* name,
            int cnt,
            GnssLocationInfoNotification* locationInfo,
            const LocationApiPbMsgConv *pbMsgConv) :
            LocAPIMsgHeader(name, E_LOCAPI_ENGINE_LOCATIONS_INFO_MSG_ID, pbMsgConv),
            count(cnt) {

        if (count > LOC_OUTPUT_ENGINE_COUNT) {
            count = LOC_OUTPUT_ENGINE_COUNT;
        }
        if (count > 0) {
            memcpy(engineLocationsInfo, locationInfo,
                   sizeof(GnssLocationInfoNotification) * count);
        }
    }
    LocAPIEngineLocationsInfoIndMsg(const char* name,
            const PBLocAPIEngineLocationsInfoIndMsg &pbLocApiEngLocInfoIndMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    inline uint32_t getMsgSize() const {
        return (sizeof(LocAPIEngineLocationsInfoIndMsg) -
                (LOC_OUTPUT_ENGINE_COUNT - count) * sizeof(GnssLocationInfoNotification));
    }

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_SATELLITE_VEHICLE_MSG_ID
struct LocAPISatelliteVehicleIndMsg: LocAPIMsgHeader
{
    GnssSvNotification gnssSvNotification;

    inline LocAPISatelliteVehicleIndMsg(const char* name,
        const GnssSvNotification& svNotification,
        const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_SATELLITE_VEHICLE_MSG_ID, pbMsgConv),
        gnssSvNotification(svNotification) { }
    LocAPISatelliteVehicleIndMsg(const char* name,
            const PBLocAPISatelliteVehicleIndMsg &pbLocApiSatVehIndMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_NMEA_MSG_ID
struct LocAPINmeaIndMsg: LocAPIMsgHeader
{
    LocAPINmeaSerializedPayload gnssNmeaNotification;

    inline LocAPINmeaIndMsg(const char* name, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_NMEA_MSG_ID, pbMsgConv) { }
    LocAPINmeaIndMsg(const char* name, const PBLocAPINmeaIndMsg &pbLocApiNmeaIndMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_DATA_MSG_ID
struct LocAPIDataIndMsg : LocAPIMsgHeader
{
    GnssDataNotification gnssDataNotification;

    inline LocAPIDataIndMsg(const char* name,
        const GnssDataNotification& dataNotification,
        const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_DATA_MSG_ID, pbMsgConv),
        gnssDataNotification(dataNotification) { }
    LocAPIDataIndMsg(const char* name, const PBLocAPIDataIndMsg &pbLocApiDataIndMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_MEAS_MSG_ID
struct LocAPIMeasIndMsg : LocAPIMsgHeader
{
    GnssMeasurementsNotification gnssMeasurementsNotification;

    inline LocAPIMeasIndMsg(const char* name,
        const GnssMeasurementsNotification& measurementsNotification,
        const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_MEAS_MSG_ID, pbMsgConv),
        gnssMeasurementsNotification(measurementsNotification) { }
    LocAPIMeasIndMsg(const char* name, const PBLocAPIMeasIndMsg &pbLocApiMeasIndMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_GET_GNSS_ENGERY_CONSUMED_MSG_ID
struct LocAPIGnssEnergyConsumedIndMsg: LocAPIMsgHeader
{
    uint64_t totalGnssEnergyConsumedSinceFirstBoot;

    inline LocAPIGnssEnergyConsumedIndMsg(const char* name, uint64_t energyConsumed,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_GET_GNSS_ENGERY_CONSUMED_MSG_ID, pbMsgConv),
        totalGnssEnergyConsumedSinceFirstBoot(energyConsumed) { }
    LocAPIGnssEnergyConsumedIndMsg(const char* name,
            const PBLocAPIGnssEnergyConsumedIndMsg &pbLocApiGnssEnrgyConsmdIndMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_LOCATION_SYSTEM_INFO_MSG_ID
struct LocAPILocationSystemInfoIndMsg: LocAPIMsgHeader
{
    LocationSystemInfo locationSystemInfo;

    inline LocAPILocationSystemInfoIndMsg(const char* name, const LocationSystemInfo & systemInfo,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_LOCATION_SYSTEM_INFO_MSG_ID, pbMsgConv),
        locationSystemInfo(systemInfo) { }
    LocAPILocationSystemInfoIndMsg(const char* name,
            const PBLocAPILocationSystemInfoIndMsg &pbLocApiLocSysInfoIndMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_DC_REPORT_MSG_ID
struct LocAPIDcReportIndMsg : LocAPIMsgHeader
{
    GnssDcReportInfo dcReportInfo;

    inline LocAPIDcReportIndMsg(const char* name,
                                const GnssDcReportInfo& gnssDcReportInfo,
                                const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_DC_REPORT_MSG_ID, pbMsgConv),
        dcReportInfo(gnssDcReportInfo) { }
    LocAPIDcReportIndMsg(const char* name, const PBLocAPIDcReportIndMsg &pbLocAPIDcReportIndMsg,
                         const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_EPH_MSG_ID
struct LocAPIEphIndMsg : LocAPIMsgHeader
{
    GnssSvEphemerisReport gnssEphNotification;

    // For LOC-HAL to PROTO conversion
    inline LocAPIEphIndMsg(const char* name,
        const GnssSvEphemerisReport& ephemerisInfo,
        const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_EPH_MSG_ID, pbMsgConv),
        gnssEphNotification(ephemerisInfo) { }

    // For PROTO to LOC-HAL conversion
    LocAPIEphIndMsg(const char* name, const PBLocAPIEphIndMsg &pbLocApiEphIndMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

/******************************************************************************
IPC message structure - Location Integration API Configure Request
******************************************************************************/
struct LocConfigConstrainedTuncReqMsg: LocAPIMsgHeader
{
    bool     mEnable;
    float    mTuncConstraint;
    uint32_t mEnergyBudget;

    inline LocConfigConstrainedTuncReqMsg(const char* name,
                                          bool enable,
                                          float tuncConstraint,
                                          uint32_t energyBudget,
                                          const LocationApiPbMsgConv *pbMsgConv):
            LocAPIMsgHeader(name, E_INTAPI_CONFIG_CONSTRAINTED_TUNC_MSG_ID, pbMsgConv),
            mEnable(enable),
            mTuncConstraint(tuncConstraint),
            mEnergyBudget(energyBudget) { }
    LocConfigConstrainedTuncReqMsg(const char* name,
            const PBLocConfigConstrainedTuncReqMsg &pbConfigConstrTuncReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigPositionAssistedClockEstimatorReqMsg: LocAPIMsgHeader
{
    bool     mEnable;
    inline LocConfigPositionAssistedClockEstimatorReqMsg(const char* name,
                                                         bool enable,
                                                         const LocationApiPbMsgConv *pbMsgConv) :
            LocAPIMsgHeader(name,
                            E_INTAPI_CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR_MSG_ID,
                            pbMsgConv),
            mEnable(enable) { }
    LocConfigPositionAssistedClockEstimatorReqMsg(const char* name,
            const PBLocConfigPositionAssistedClockEstimatorReqMsg &pbLocConfPosAsstdClkEstReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigSvConstellationReqMsg: LocAPIMsgHeader
{
    GnssSvTypeConfig mConstellationEnablementConfig;
    GnssSvIdConfig   mBlacklistSvConfig;

    inline LocConfigSvConstellationReqMsg(const char* name,
                                          const GnssSvTypeConfig& constellationEnablementConfig,
                                          const GnssSvIdConfig& blacklistSvConfig,
                                          const LocationApiPbMsgConv *pbMsgConv) :
            LocAPIMsgHeader(name, E_INTAPI_CONFIG_SV_CONSTELLATION_MSG_ID, pbMsgConv),
            mConstellationEnablementConfig(constellationEnablementConfig),
            mBlacklistSvConfig(blacklistSvConfig) { }
    LocConfigSvConstellationReqMsg(const char* name,
            const PBLocConfigSvConstellationReqMsg &pbConfigSvConstReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigConstellationSecondaryBandReqMsg: LocAPIMsgHeader
{
    GnssSvTypeConfig mSecondaryBandConfig;

    inline LocConfigConstellationSecondaryBandReqMsg(
            const char* name, const GnssSvTypeConfig& secondaryBandConfig,
            const LocationApiPbMsgConv *pbMsgConv) :
            LocAPIMsgHeader(name, E_INTAPI_CONFIG_CONSTELLATION_SECONDARY_BAND_MSG_ID, pbMsgConv),
            mSecondaryBandConfig(secondaryBandConfig){ }

    LocConfigConstellationSecondaryBandReqMsg(const char* name,
            const PBLocConfigConstellationSecondaryBandReqMsg &pbConfigConstSecBandReq,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

// defintion for message with msg id of E_LOCAPI_CONTROL_DELETE_AIDING_DATA_MSG_ID
struct LocConfigAidingDataDeletionReqMsg: LocAPIMsgHeader
{
    GnssAidingData mAidingData;

    inline LocConfigAidingDataDeletionReqMsg(const char* name, GnssAidingData& aidingData,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_AIDING_DATA_DELETION_MSG_ID, pbMsgConv),
        mAidingData(aidingData) { }
    LocConfigAidingDataDeletionReqMsg(const char* name,
            const PBLocConfigAidingDataDeletionReqMsg &pbConfigAidDataDelReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigLeverArmReqMsg: LocAPIMsgHeader
{
    LeverArmConfigInfo mLeverArmConfigInfo;

    inline LocConfigLeverArmReqMsg(const char* name,
                                   const LeverArmConfigInfo & configInfo,
                                   const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_LEVER_ARM_MSG_ID, pbMsgConv),
        mLeverArmConfigInfo(configInfo) { }
    LocConfigLeverArmReqMsg(const char* name,
            const PBLocConfigLeverArmReqMsg &pbConfigLeverArmReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigRobustLocationReqMsg: LocAPIMsgHeader
{
    bool mEnable;
    bool mEnableForE911;

    inline LocConfigRobustLocationReqMsg(const char* name,
                                         bool enable,
                                         bool enableForE911,
                                         const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_ROBUST_LOCATION_MSG_ID, pbMsgConv),
        mEnable(enable),
        mEnableForE911(enableForE911) { }
    LocConfigRobustLocationReqMsg(const char* name,
            const PBLocConfigRobustLocationReqMsg &pbConfigRobustLocReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigMinGpsWeekReqMsg: LocAPIMsgHeader
{
    uint16_t mMinGpsWeek;

    inline LocConfigMinGpsWeekReqMsg(const char* name,
                                     uint16_t minGpsWeek,
                                     const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_MIN_GPS_WEEK_MSG_ID, pbMsgConv),
        mMinGpsWeek(minGpsWeek) { }
    LocConfigMinGpsWeekReqMsg(const char* name,
            const PBLocConfigMinGpsWeekReqMsg &pbConfigMinGpsWeekReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigDrEngineParamsReqMsg: LocAPIMsgHeader
{
    DeadReckoningEngineConfig mDreConfig;
    inline LocConfigDrEngineParamsReqMsg(const char* name,
                                         const DeadReckoningEngineConfig& dreConfig,
                                         const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_DEAD_RECKONING_ENGINE_MSG_ID, pbMsgConv),
        mDreConfig(dreConfig) { }
    LocConfigDrEngineParamsReqMsg(const char* name,
            const PBLocConfigDrEngineParamsReqMsg &pbDrEngineConfig,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigMinSvElevationReqMsg: LocAPIMsgHeader
{
    uint8_t mMinSvElevation;

    inline LocConfigMinSvElevationReqMsg(const char* name,
                                         uint8_t minSvElevation,
                                         const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_MIN_SV_ELEVATION_MSG_ID, pbMsgConv),
        mMinSvElevation(minSvElevation) { }
    LocConfigMinSvElevationReqMsg(const char* name,
            const PBLocConfigMinSvElevationReqMsg &pbConfigMinSvElevReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigEngineRunStateReqMsg: LocAPIMsgHeader
{
    // In this API, only one engine is configured at a time
    PositioningEngineMask mEngType;
    LocEngineRunState mEngState;

    inline LocConfigEngineRunStateReqMsg(const char* name,
                                         PositioningEngineMask engType,
                                         LocEngineRunState engState,
                                         const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_ENGINE_RUN_STATE_MSG_ID, pbMsgConv),
        mEngType(engType), mEngState(engState) { }

    LocConfigEngineRunStateReqMsg(const char* name,
            const PBLocConfigEngineRunStateReqMsg &pbConfigEngineStateReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigUserConsentTerrestrialPositioningReqMsg: LocAPIMsgHeader
{
    bool mUserConsent;

    inline LocConfigUserConsentTerrestrialPositioningReqMsg(
            const char* name, bool userConsent, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name,
                        E_INTAPI_CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING_MSG_ID,
                        pbMsgConv),
        mUserConsent(userConsent) { }

    LocConfigUserConsentTerrestrialPositioningReqMsg(const char* name,
            const PBLocConfigUserConsentTerrestrialPositioningReqMsg &pbMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigOutputNmeaTypesReqMsg: LocAPIMsgHeader
{
    GnssNmeaTypesMask mEnabledNmeaTypes;
    GnssGeodeticDatumType mNmeaDatumType;
    uint32_t mNmeaReqEngMask;

    inline LocConfigOutputNmeaTypesReqMsg(
            const char* name, GnssNmeaTypesMask enabledNmeaTypes,
            GnssGeodeticDatumType nmeaDatumType, uint32_t nmeaReqEngMask,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name,
                        E_INTAPI_CONFIG_OUTPUT_NMEA_TYPES_MSG_ID,
                        pbMsgConv),
            mEnabledNmeaTypes(enabledNmeaTypes),
            mNmeaDatumType(nmeaDatumType),
            mNmeaReqEngMask(nmeaReqEngMask) { }

    LocConfigOutputNmeaTypesReqMsg(const char* name,
            const PBLocConfigOutputNmeaTypesReqMsg &pbMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigEngineIntegrityRiskReqMsg: LocAPIMsgHeader
{
    // In this API, only one engine is configured at a time
    PositioningEngineMask mEngType;
    uint32_t mIntegrityRisk;

    inline LocConfigEngineIntegrityRiskReqMsg(const char* name,
                                              PositioningEngineMask engType,
                                              uint32_t integrityRisk,
                                              const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_ENGINE_INTEGRITY_RISK_MSG_ID, pbMsgConv),
        mEngType(engType), mIntegrityRisk(integrityRisk) { }

    LocConfigEngineIntegrityRiskReqMsg(const char* name,
            const PBLocConfigEngineIntegrityRiskReqMsg &pbConfigEngineIntegrityRiskMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigXtraReqMsg: LocAPIMsgHeader
{
    bool mEnable;
    XtraConfigParams mXtraParams;

    inline LocConfigXtraReqMsg(const char* name, bool enable, XtraConfigParams xtraParams,
                               const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_XTRA_PARAMS_MSG_ID, pbMsgConv),
        mEnable(enable),
        mXtraParams(xtraParams) { }

    LocConfigXtraReqMsg(const char* name, const PBLocConfigXtraReqMsg &pbConfigXtraMsg,
                        const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigMerkleTreeReqMsg: LocAPIMsgHeader {
    std::string mMerkleTreeConfig;

    inline LocConfigMerkleTreeReqMsg(const char* name, const std::string& merkleTreeConfig,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_MERKLE_TREE_MSG_ID, pbMsgConv),
        mMerkleTreeConfig(merkleTreeConfig) { }

    LocConfigMerkleTreeReqMsg(const char* name,
            const PBLocConfigMerkleTreeReqMsg &pbConfigMerkleTreeMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigOsnmaEnablementReqMsg: LocAPIMsgHeader {
    bool mEnable;

    inline LocConfigOsnmaEnablementReqMsg(const char* name, bool enable,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_OSNMA_ENABLEMENT_MSG_ID, pbMsgConv),
        mEnable(enable) { }

    LocConfigOsnmaEnablementReqMsg(const char* name,
            const PBLocConfigOsnmaEnablementReqMsg &pbConfigOsnmaEnablementMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocInjectMmfDataReqMsg: LocAPIMsgHeader
{
    GnssMapMatchedData gnssMapData;

    inline LocInjectMmfDataReqMsg(const char* name, GnssMapMatchedData mapData,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_MAP_MATCHED_FEEDBACK_MSG_ID, pbMsgConv),
        gnssMapData(mapData) { }
    LocInjectMmfDataReqMsg(const char* name,
            const PBLocInjectMmfDataReqMsg &pbInjectMmfDataReqMsg,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

/******************************************************************************
IPC message structure - Location Integration API Get request/response message
******************************************************************************/
struct LocConfigGetRobustLocationConfigReqMsg: LocAPIMsgHeader
{
    inline LocConfigGetRobustLocationConfigReqMsg(const char* name,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_GET_ROBUST_LOCATION_CONFIG_REQ_MSG_ID, pbMsgConv) { }

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigGetRobustLocationConfigRespMsg: LocAPIMsgHeader
{
    GnssConfigRobustLocation mRobustLoationConfig;

    inline LocConfigGetRobustLocationConfigRespMsg(
            const char* name,
            GnssConfigRobustLocation robustLoationConfig,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_GET_ROBUST_LOCATION_CONFIG_RESP_MSG_ID, pbMsgConv),
        mRobustLoationConfig(robustLoationConfig) { }
    LocConfigGetRobustLocationConfigRespMsg(const char* name,
            const PBLocConfigGetRobustLocationConfigRespMsg &pbLocConfigGetRobustLocationRsp,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigGetMinGpsWeekReqMsg: LocAPIMsgHeader
{
    inline LocConfigGetMinGpsWeekReqMsg(const char* name, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_GET_MIN_GPS_WEEK_REQ_MSG_ID, pbMsgConv) { }

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigGetMinGpsWeekRespMsg: LocAPIMsgHeader
{
    uint16_t mMinGpsWeek;
    inline LocConfigGetMinGpsWeekRespMsg(const char* name,
                                         uint16_t minGpsWeek,
                                         const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_GET_MIN_GPS_WEEK_RESP_MSG_ID, pbMsgConv),
        mMinGpsWeek(minGpsWeek) { }
    LocConfigGetMinGpsWeekRespMsg(const char* name,
            const PBLocConfigGetMinGpsWeekRespMsg &pbLocConfigGetMinGpsWeekRsp,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigGetMinSvElevationReqMsg: LocAPIMsgHeader
{
    inline LocConfigGetMinSvElevationReqMsg(const char* name,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_GET_MIN_SV_ELEVATION_REQ_MSG_ID, pbMsgConv) { }

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigGetMinSvElevationRespMsg: LocAPIMsgHeader
{
    uint8_t mMinSvElevation;
    inline LocConfigGetMinSvElevationRespMsg(const char* name,
                                             uint8_t minSvElevation,
                                             const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_GET_MIN_SV_ELEVATION_RESP_MSG_ID, pbMsgConv),
        mMinSvElevation(minSvElevation) { }
    LocConfigGetMinSvElevationRespMsg(const char* name,
            const PBLocConfigGetMinSvElevationRespMsg &pbLocConfigGetMinSvElevRsp,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigGetConstellationSecondaryBandConfigReqMsg: LocAPIMsgHeader
{
    inline LocConfigGetConstellationSecondaryBandConfigReqMsg(const char* name,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_REQ_MSG_ID,
                pbMsgConv) { }

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigGetConstellationSecondaryBandConfigRespMsg: LocAPIMsgHeader
{
    GnssSvTypeConfig mSecondaryBandConfig;

    inline LocConfigGetConstellationSecondaryBandConfigRespMsg(const char* name,
                                                  GnssSvTypeConfig secondaryBandConfig,
                                                  const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_RESP_MSG_ID,
                pbMsgConv),
        mSecondaryBandConfig(secondaryBandConfig){ }
    LocConfigGetConstellationSecondaryBandConfigRespMsg(const char* name,
            const PBLocConfigGetConstltnSecondaryBandConfigRespMsg &pbCfgGetConstSecBandCfgResp,
            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocAPIGetDebugReqMsg : LocAPIMsgHeader
{
    inline LocAPIGetDebugReqMsg(const char* name,
            const LocationApiPbMsgConv* pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_GET_DEBUG_REQ_MSG_ID, pbMsgConv) { }

    int serializeToProtobuf(string& protoStr) override;
};

struct LocAPIGetDebugRespMsg : LocAPIMsgHeader
{
    GnssDebugReport mDebugReport;

    inline LocAPIGetDebugRespMsg(const char* name,
            GnssDebugReport debugReport,
            const LocationApiPbMsgConv* pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_GET_DEBUG_RESP_MSG_ID, pbMsgConv),
        mDebugReport(debugReport) { }
    LocAPIGetDebugRespMsg(const char* name,
            const PBLocAPIGetDebugRespMsg& pbMsg,
            const LocationApiPbMsgConv* pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocAPIGetAntennaInfoMsg : LocAPIMsgHeader
{
    inline LocAPIGetAntennaInfoMsg(const char* name,
        const LocationApiPbMsgConv* pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_GET_ANTENNA_INFO_MSG_ID, pbMsgConv) { }

    int serializeToProtobuf(string& protoStr) override;
};

struct LocAPIAntennaInfoMsg : LocAPIMsgHeader
{
    AntennaInformation mAntennaInfo;

    inline LocAPIAntennaInfoMsg(const char* name,
            AntennaInformation& antennaInfo,
            const LocationApiPbMsgConv* pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_ANTENNA_INFO_MSG_ID, pbMsgConv),
        mAntennaInfo(antennaInfo) { }
    LocAPIAntennaInfoMsg(const char* name,
            const PBLocAPIAntennaInfoMsg& pbMsg,
            const LocationApiPbMsgConv* pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocIntApiInjectLocationMsg : LocAPIMsgHeader
{
    Location mLocation;
    inline LocIntApiInjectLocationMsg(const char* name,
                                      const Location &location,
                                      const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_INJECT_LOCATION_MSG_ID, pbMsgConv),
        mLocation(location) { }

    LocIntApiInjectLocationMsg(const char* name,
                               const PBLocIntApiInjectLocationMsg &pbMsg,
                               const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigRegisterGnssSignalTypesUpdateReqMsg: LocAPIMsgHeader {
    bool mRegisterUpdate;
    inline LocConfigRegisterGnssSignalTypesUpdateReqMsg(const char* name,
                                bool registerUpdate,
                                const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_REQ_MSG_ID, pbMsgConv),
        mRegisterUpdate(registerUpdate) { }
    inline LocConfigRegisterGnssSignalTypesUpdateReqMsg(
            const char* name, PBLocConfigRegisterGnssSignalTypesUpdateReqMsg& pbMsg,
            const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_REQ_MSG_ID, pbMsgConv),
        mRegisterUpdate(pbMsg.mregisterupdate()) { }

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigRegisterGnssSignalTypesUpdateRespMsg : LocAPIMsgHeader {
    GnssSignalTypeMask mSignalTypeMask;

    inline LocConfigRegisterGnssSignalTypesUpdateRespMsg(const char* name,
                                GnssSignalTypeMask signalTypeMask,
                                const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_RESP_MSG_ID, pbMsgConv),
        mSignalTypeMask(signalTypeMask) { }
    LocConfigRegisterGnssSignalTypesUpdateRespMsg(const char* name,
                            const PBLocConfigRegisterGnssSignalTypesUpdateRespMsg &pbMsg,
                            const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};
/**************** XTRA related section **********************/
struct LocConfigGetXtraStatusReqMsg: LocAPIMsgHeader
{
    inline LocConfigGetXtraStatusReqMsg(const char* name,
                                        const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_GET_XTRA_STATUS_REQ_MSG_ID,
                        pbMsgConv) { }

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigRegisterXtraStatusUpdateReqMsg: LocAPIMsgHeader
{
    inline LocConfigRegisterXtraStatusUpdateReqMsg(
            const char* name, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_REGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID,
                        pbMsgConv) { }

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigDeregisterXtraStatusUpdateReqMsg: LocAPIMsgHeader
{
    inline LocConfigDeregisterXtraStatusUpdateReqMsg(
            const char* name, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_DEREGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID,
                        pbMsgConv) { }

    int serializeToProtobuf(string& protoStr) override;
};

struct LocConfigGetXtraStatusRespMsg: LocAPIMsgHeader
{
    XtraStatusUpdateType mUpdateType;
    XtraStatus           mXtraStatus;

    inline LocConfigGetXtraStatusRespMsg(const char* name,
                                         XtraStatusUpdateType updateType,
                                         XtraStatus           xtraStatus,
                                         const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_GET_XTRA_STATUS_RESP_MSG_ID, pbMsgConv),
        mUpdateType(updateType), mXtraStatus(xtraStatus) { }
    LocConfigGetXtraStatusRespMsg(const char* name,
                                  const PBLocConfigGetXtraStatusRespMsg &pbGetXtraStatusMsg,
                                  const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

/******************************************************************************
IPC message structure - ping
******************************************************************************/
#define LOCATION_REMOTE_API_PINGTEST_SIZE (4)

struct LocAPIPingTestReqMsg: LocAPIMsgHeader
{
    uint8_t data[LOCATION_REMOTE_API_PINGTEST_SIZE];

    inline LocAPIPingTestReqMsg(const char* name, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_PINGTEST_MSG_ID, pbMsgConv) { }
    LocAPIPingTestReqMsg(const char* name, const PBLocAPIPingTestReqMsg &pbPingTestReqMsg,
        const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

struct LocAPIPingTestIndMsg: LocAPIMsgHeader
{
    uint8_t data[LOCATION_REMOTE_API_PINGTEST_SIZE];

    inline LocAPIPingTestIndMsg(const char* name, const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_LOCAPI_PINGTEST_MSG_ID, pbMsgConv) { }
    LocAPIPingTestIndMsg(const char* name, const PBLocAPIPingTestIndMsg &pbLocApiPingTestIndMsg,
        const LocationApiPbMsgConv *pbMsgConv);

    int serializeToProtobuf(string& protoStr) override;
};

#endif /* LOCATIONAPIMSG_H */
