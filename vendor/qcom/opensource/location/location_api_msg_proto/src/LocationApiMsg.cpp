/* Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
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

#define LOG_TAG "LocSvc_LocationApiMsg"

#include <inttypes.h>
#include <dirent.h>

#include <loc_pla.h>
#include <log_util.h>
#include <loc_misc_utils.h>
#include <gps_extended.h>
#include <LocIpc.h>
#include <LocTimer.h>
#include <loc_cfg.h>

#include <LocationApiMsg.h>
#include <LocationApiPbMsgConv.h>

using namespace loc_util;

const char* LocApiMsgString(ELocMsgID msgId) {
    switch (msgId) {
    case E_LOCAPI_UNDEFINED_MSG_ID:
        return "E_LOCAPI_UNDEFINED_MSG_ID";
    case E_LOCAPI_CLIENT_REGISTER_MSG_ID:
        return "E_LOCAPI_CLIENT_REGISTER_MSG_ID";
    case E_LOCAPI_CLIENT_DEREGISTER_MSG_ID:
        return "E_LOCAPI_CLIENT_DEREGISTER_MSG_ID";
    case E_LOCAPI_CAPABILILTIES_MSG_ID:
        return "E_LOCAPI_CAPABILILTIES_MSG_ID";
    case E_LOCAPI_HAL_READY_MSG_ID:
        return "E_LOCAPI_HAL_READY_MSG_ID";
    case E_LOCAPI_START_TRACKING_MSG_ID:
        return "E_LOCAPI_START_TRACKING_MSG_ID";
    case E_LOCAPI_STOP_TRACKING_MSG_ID:
        return "E_LOCAPI_STOP_TRACKING_MSG_ID";
    case E_LOCAPI_UPDATE_CALLBACKS_MSG_ID:
        return "E_LOCAPI_UPDATE_CALLBACKS_MSG_ID";
    case E_LOCAPI_UPDATE_TRACKING_OPTIONS_MSG_ID:
        return "E_LOCAPI_UPDATE_TRACKING_OPTIONS_MSG_ID";
    case E_LOCAPI_CONTROL_UPDATE_CONFIG_MSG_ID:
        return "E_LOCAPI_CONTROL_UPDATE_CONFIG_MSG_ID";
    case E_LOCAPI_CONTROL_DELETE_AIDING_DATA_MSG_ID:
        return "E_LOCAPI_CONTROL_DELETE_AIDING_DATA_MSG_ID";
    case E_LOCAPI_CONTROL_UPDATE_NETWORK_AVAILABILITY_MSG_ID:
        return "E_LOCAPI_CONTROL_UPDATE_NETWORK_AVAILABILITY_MSG_ID";
    case E_LOCAPI_LOCATION_MSG_ID:
        return "E_LOCAPI_LOCATION_MSG_ID";
    case E_LOCAPI_LOCATION_INFO_MSG_ID:
        return "E_LOCAPI_LOCATION_INFO_MSG_ID";
    case E_LOCAPI_SATELLITE_VEHICLE_MSG_ID:
        return "E_LOCAPI_SATELLITE_VEHICLE_MSG_ID";
    case E_LOCAPI_NMEA_MSG_ID:
        return "E_LOCAPI_NMEA_MSG_ID";
    case E_LOCAPI_DATA_MSG_ID:
        return "E_LOCAPI_DATA_MSG_ID";
    case E_LOCAPI_GET_GNSS_ENGERY_CONSUMED_MSG_ID:
        return "E_LOCAPI_GET_GNSS_ENGERY_CONSUMED_MSG_ID";
    case E_LOCAPI_LOCATION_SYSTEM_INFO_MSG_ID:
        return "E_LOCAPI_LOCATION_SYSTEM_INFO_MSG_ID";
    case E_LOCAPI_ENGINE_LOCATIONS_INFO_MSG_ID:
        return "E_LOCAPI_ENGINE_LOCATIONS_INFO_MSG_ID";
    case E_LOCAPI_START_BATCHING_MSG_ID:
        return "E_LOCAPI_START_BATCHING_MSG_ID";
    case E_LOCAPI_STOP_BATCHING_MSG_ID:
        return "E_LOCAPI_STOP_BATCHING_MSG_ID";
    case E_LOCAPI_UPDATE_BATCHING_OPTIONS_MSG_ID:
        return "E_LOCAPI_UPDATE_BATCHING_OPTIONS_MSG_ID";
    case E_LOCAPI_BATCHING_MSG_ID:
        return "E_LOCAPI_BATCHING_MSG_ID";
    case E_LOCAPI_ADD_GEOFENCES_MSG_ID:
        return "E_LOCAPI_ADD_GEOFENCES_MSG_ID";
    case E_LOCAPI_REMOVE_GEOFENCES_MSG_ID:
        return "E_LOCAPI_REMOVE_GEOFENCES_MSG_ID";
    case E_LOCAPI_MODIFY_GEOFENCES_MSG_ID:
        return "E_LOCAPI_MODIFY_GEOFENCES_MSG_ID";
    case E_LOCAPI_PAUSE_GEOFENCES_MSG_ID:
        return "E_LOCAPI_PAUSE_GEOFENCES_MSG_ID";
    case E_LOCAPI_RESUME_GEOFENCES_MSG_ID:
        return "E_LOCAPI_RESUME_GEOFENCES_MSG_ID";
    case E_LOCAPI_GEOFENCE_BREACH_MSG_ID:
        return "E_LOCAPI_GEOFENCE_BREACH_MSG_ID";
    case E_LOCAPI_MEAS_MSG_ID:
        return "E_LOCAPI_MEAS_MSG_ID";
    case E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_REQ_MSG_ID:
        return "E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_REQ_MSG_ID";
    case E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_RESP_MSG_ID:
        return "E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_RESP_MSG_ID";
    case E_LOCAPI_GET_DEBUG_REQ_MSG_ID:
        return "E_LOCAPI_GET_DEBUG_REQ_MSG_ID";
    case E_LOCAPI_GET_DEBUG_RESP_MSG_ID:
        return "E_LOCAPI_GET_DEBUG_RESP_MSG_ID";
    case E_LOCAPI_GET_ANTENNA_INFO_MSG_ID:
        return "E_LOCAPI_GET_ANTENNA_INFO_MSG_ID";
    case E_LOCAPI_ANTENNA_INFO_MSG_ID:
        return "E_LOCAPI_ANTENNA_INFO_MSG_ID";
    case E_LOCAPI_EPH_MSG_ID:
        return "E_LOCAPI_EPH_MSG_ID";
    case E_LOCAPI_PINGTEST_MSG_ID:
        return "E_LOCAPI_PINGTEST_MSG_ID";
    case E_INTAPI_CONFIG_CONSTRAINTED_TUNC_MSG_ID:
        return "E_INTAPI_CONFIG_CONSTRAINTED_TUNC_MSG_ID";
    case E_INTAPI_CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR_MSG_ID:
        return "E_INTAPI_CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR_MSG_ID";
    case E_INTAPI_CONFIG_SV_CONSTELLATION_MSG_ID:
        return "E_INTAPI_CONFIG_SV_CONSTELLATION_MSG_ID";
    case E_INTAPI_CONFIG_AIDING_DATA_DELETION_MSG_ID:
        return "E_INTAPI_CONFIG_AIDING_DATA_DELETION_MSG_ID";
    case E_INTAPI_CONFIG_LEVER_ARM_MSG_ID:
        return "E_INTAPI_CONFIG_LEVER_ARM_MSG_ID";
    case E_INTAPI_CONFIG_ROBUST_LOCATION_MSG_ID:
        return "E_INTAPI_CONFIG_ROBUST_LOCATION_MSG_ID";
    case E_INTAPI_CONFIG_MIN_GPS_WEEK_MSG_ID:
        return "E_INTAPI_CONFIG_MIN_GPS_WEEK_MSG_ID";
    case E_INTAPI_CONFIG_DEAD_RECKONING_ENGINE_MSG_ID:
        return "E_INTAPI_CONFIG_DEAD_RECKONING_ENGINE_MSG_ID";
    case E_INTAPI_CONFIG_MIN_SV_ELEVATION_MSG_ID:
        return "E_INTAPI_CONFIG_MIN_SV_ELEVATION_MSG_ID";
    case E_INTAPI_CONFIG_CONSTELLATION_SECONDARY_BAND_MSG_ID:
        return "E_INTAPI_CONFIG_CONSTELLATION_SECONDARY_BAND_MSG_ID";
    case E_INTAPI_CONFIG_ENGINE_RUN_STATE_MSG_ID:
        return "E_INTAPI_CONFIG_ENGINE_RUN_STATE_MSG_ID";
    case E_INTAPI_CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING_MSG_ID:
        return "E_INTAPI_CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING_MSG_ID";
    case E_INTAPI_CONFIG_OUTPUT_NMEA_TYPES_MSG_ID:
        return "E_INTAPI_CONFIG_OUTPUT_NMEA_TYPES_MSG_ID";
    case E_INTAPI_CONFIG_ENGINE_INTEGRITY_RISK_MSG_ID:
        return "E_INTAPI_CONFIG_ENGINE_INTEGRITY_RISK_MSG_ID";
    case E_INTAPI_CONFIG_XTRA_PARAMS_MSG_ID:
        return "E_INTAPI_CONFIG_XTRA_PARAMS_MSG_ID";
    case E_INTAPI_CONFIG_MERKLE_TREE_MSG_ID:
        return "E_INTAPI_CONFIG_MERKLE_TREE_MSG_ID";
    case E_INTAPI_CONFIG_OSNMA_ENABLEMENT_MSG_ID:
        return "E_INTAPI_CONFIG_OSNMA_ENABLEMENT_MSG_ID";
    case E_INTAPI_GET_ROBUST_LOCATION_CONFIG_REQ_MSG_ID:
        return "E_INTAPI_GET_ROBUST_LOCATION_CONFIG_REQ_MSG_ID";
    case E_INTAPI_GET_ROBUST_LOCATION_CONFIG_RESP_MSG_ID:
        return "E_INTAPI_GET_ROBUST_LOCATION_CONFIG_RESP_MSG_ID";
    case E_INTAPI_GET_MIN_GPS_WEEK_REQ_MSG_ID:
        return "E_INTAPI_GET_MIN_GPS_WEEK_REQ_MSG_ID";
    case E_INTAPI_GET_MIN_GPS_WEEK_RESP_MSG_ID:
        return "E_INTAPI_GET_MIN_GPS_WEEK_RESP_MSG_ID";
    case E_INTAPI_GET_MIN_SV_ELEVATION_REQ_MSG_ID:
        return "E_INTAPI_GET_MIN_SV_ELEVATION_REQ_MSG_ID";
    case E_INTAPI_GET_MIN_SV_ELEVATION_RESP_MSG_ID:
        return "E_INTAPI_GET_MIN_SV_ELEVATION_RESP_MSG_ID";
    case E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_REQ_MSG_ID:
        return "E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_REQ_MSG_ID";
    case E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_RESP_MSG_ID:
        return "E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_RESP_MSG_ID";
    case E_INTAPI_INJECT_LOCATION_MSG_ID:
        return "E_INTAPI_INJECT_LOCATION_MSG_ID";
    case E_INTAPI_GET_XTRA_STATUS_REQ_MSG_ID:
        return "E_INTAPI_GET_XTRA_STATUS_REQ_MSG_ID";
    case E_INTAPI_GET_XTRA_STATUS_RESP_MSG_ID:
        return "E_INTAPI_GET_XTRA_STATUS_REQ_MSG_ID";
    case E_INTAPI_REGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID:
        return "E_INTAPI_REGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID";
    case E_INTAPI_DEREGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID:
        return "E_INTAPI_DEREGISTER_XTRA_STATUS_UPDATE_REQ_MSG_ID";
    case E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_REQ_MSG_ID:
        return "E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_REQ_MSG_ID";
    case E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_RESP_MSG_ID:
        return "E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_RESP_MSG_ID";
    case E_INTAPI_CONFIG_MAP_MATCHED_FEEDBACK_MSG_ID:
        return "E_INTAPI_CONFIG_MAP_MATCHED_FEEDBACK_MSG_ID";
    default:
        return "unknown ELocMsgID";
    }
}

// SERIALIZE RIGID TO PROTOBUF FORMAT
// **********************************
// Convert LocApiMsgHeader rigid structures to protobuf msg format. Local structure is converted
// to protobuf structure and serialized to string passed in the function. This payload can be
// be passed over IPC.

// Convert LocAPIClientRegisterReqMsg -> PBLocAPIClientRegisterReqMsg payload
int LocAPIClientRegisterReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIClientRegisterReqMsg pbLocApiClientRegMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }

    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIClientRegisterReqMsg conversion
    // PBClientType mClientType = 1;
    pbLocApiClientRegMsg.set_mclienttype(pLocApiPbMsgConv->getPBEnumForClientType(mClientType));

    string pbStr;
    if (!pbLocApiClientRegMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiClientRegMsg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIClientRegisterReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocAPIClientDeregisterReqMsg -> PBLocAPIClientDeregisterReqMsg
int LocAPIClientDeregisterReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // bytes       payload = 4;
    // LocAPIClientDeregisterReqMsg - no struct member. No payload to send
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIClientDeregisterReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocAPICapabilitiesIndMsg -> PBLocAPICapabilitiesIndMsg
int LocAPICapabilitiesIndMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPICapabilitiesIndMsg pbLocApiCapabInd;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPICapabilitiesIndMsg conversion
    // Bitwise OR of PBLocationCapabilitiesMask
    // uint64 capabilitiesMask = 1;
    pbLocApiCapabInd.set_capabilitiesmask(
            pLocApiPbMsgConv->getPBMaskForLocationCapabilitiesMask(capabilitiesMask));

    string pbStr;
    if (!pbLocApiCapabInd.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiCapabInd failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPICapabilitiesIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocAPIHalReadyIndMsg -> PBLocAPIHalReadyIndMsg
int LocAPIHalReadyIndMsg::serializeToProtobuf(string& protoStr) {
        // Convert LocAPIHalReadyIndMsg to protobuf serialize format
    PBLocAPIMsgHeader pLocApiMsgHdr;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // bytes       payload = 4;
    // LocAPIHalReadyIndMsg - no struct member. No payload to send
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIHalReadyIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocAPIGenericRespMsg -> PBLocAPIGenericRespMsg
int LocAPIGenericRespMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIGenericRespMsg pbLocApiGenericMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIGenericRespMsg conversion
    // PBLocationError err = 1;
    pbLocApiGenericMsg.set_err(pLocApiPbMsgConv->getPBEnumForLocationError(err));

    string pbStr;
    if (!pbLocApiGenericMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiGenericMsg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIGenericRespMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocAPICollectiveRespMsg -> PBLocAPICollectiveRespMsg
int LocAPICollectiveRespMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPICollectiveRespMsg pbLocApiCollctvRspMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPICollectiveRespMsg conversion
    // PBCollectiveResPayload collectiveRes = 1;
    PBCollectiveResPayload* collectiveResp = pbLocApiCollctvRspMsg.mutable_collectiveres();
    if (nullptr != collectiveResp) {
        if (pLocApiPbMsgConv->convertCollectiveResPayloadToPB(collectiveRes, collectiveResp)) {
            LOC_LOGe("convertCollectiveResPayloadToPB failed");
            free(collectiveResp);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_collectiveRes failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiCollctvRspMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiCollctvRspMsg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPICollectiveRespMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPICollectiveRespMsg(pbLocApiCollctvRspMsg);
    return protoStr.size();
}

// Convert LocAPIStartTrackingReqMsg -> PBLocAPIStartTrackingReqMsg
int LocAPIStartTrackingReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIStartTrackingReqMsg pbLocApiStartTrack;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIStartTrackingReqMsg conversion
    // PBLocationOptions locOptions = 1;
    PBLocationOptions* locOpt = pbLocApiStartTrack.mutable_locoptions();
    if (nullptr != locOpt) {
        if (pLocApiPbMsgConv->convertLocationOptionsToPB(locOptions, locOpt)) {
            LOC_LOGe("convertLocationOptionsToPB failed");
            free(locOpt);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_locoptions failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiStartTrack.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiStartTrack failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIStartTrackingReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIStartTrackingReqMsg(pbLocApiStartTrack);
    return protoStr.size();
}

// Convert LocAPIStopTrackingReqMsg -> PBLocAPIStopTrackingReqMsg
int LocAPIStopTrackingReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIStopTrackingReqMsg pbLocApiStopTrackingReqMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIStopTrackingReqMsg conversion
    // bool clearSubscriptions
    pbLocApiStopTrackingReqMsg.set_clearsubscriptions(clearSubscriptions);

    string pbStr;
    if (!pbLocApiStopTrackingReqMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiStopTrackingReqMsg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIStopTrackingReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocAPIUpdateCallbacksReqMsg -> PBLocAPIUpdateCallbacksReqMsg
int LocAPIUpdateCallbacksReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIUpdateCallbacksReqMsg pbLocApiUpdateCbsReg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIUpdateCallbacksReqMsg conversion
    // uint32    locationCallbacks = 1; - bitwise OR of PBLocationCallbacksMask
    pbLocApiUpdateCbsReg.set_locationcallbacks(
            pLocApiPbMsgConv->getPBMaskForLocationCallbacksMask(locationCallbacks));

    string pbStr;
    if (!pbLocApiUpdateCbsReg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiUpdateCbsReg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIUpdateCallbacksReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocAPIUpdateTrackingOptionsReqMsg -> PBLocAPIUpdateTrackingOptionsReqMsg
int LocAPIUpdateTrackingOptionsReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIUpdateTrackingOptionsReqMsg pbLocApiUpdtTrackOpt;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIUpdateTrackingOptionsReqMsg conversion
    // PBLocationOptions locOptions = 1;
    PBLocationOptions* locOpt = pbLocApiUpdtTrackOpt.mutable_locoptions();
    if (nullptr != locOpt) {
        if (pLocApiPbMsgConv->convertLocationOptionsToPB(locOptions, locOpt)) {
            LOC_LOGe("convertLocationOptionsToPB failed");
            free(locOpt);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_locoptions failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiUpdtTrackOpt.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiUpdtTrackOpt failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIUpdateTrackingOptionsReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIUpdateTrackingOptionsReqMsg(pbLocApiUpdtTrackOpt);
    return protoStr.size();
}

// Convert LocAPIStartBatchingReqMsg -> PBLocAPIStartBatchingReqMsg
int LocAPIStartBatchingReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIStartBatchingReqMsg pbLocApiStartBatch;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIStartBatchingReqMsg conversion
    // uint32 intervalInMs = 1;
    pbLocApiStartBatch.set_intervalinms(intervalInMs);
    // uint32 distanceInMeters = 2;
    pbLocApiStartBatch.set_distanceinmeters(distanceInMeters);
    // PBBatchingMode batchingMode = 3;
    pbLocApiStartBatch.set_batchingmode(pLocApiPbMsgConv->getPBEnumForBatchingMode(batchingMode));

    string pbStr;
    if (!pbLocApiStartBatch.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiStartBatch failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIStartBatchingReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocAPIStopBatchingReqMsg -> PBLocAPIStopBatchingReqMsg
int LocAPIStopBatchingReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // bytes       payload = 4;
    // LocAPIStopBatchingReqMsg - no struct member. No payload to send
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIStopBatchingReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocAPIUpdateBatchingOptionsReqMsg -> PBLocAPIUpdateBatchingOptionsReqMsg
int LocAPIUpdateBatchingOptionsReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIUpdateBatchingOptionsReqMsg pbLocApiUptBatchOpt;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIUpdateBatchingOptionsReqMsg conversion
    // uint32 intervalInMs = 1;
    pbLocApiUptBatchOpt.set_intervalinms(intervalInMs);
    // uint32 distanceInMeters = 2;
    pbLocApiUptBatchOpt.set_distanceinmeters(distanceInMeters);
    // PBBatchingMode batchingMode = 3;
    pbLocApiUptBatchOpt.set_batchingmode(pLocApiPbMsgConv->getPBEnumForBatchingMode(batchingMode));

    string pbStr;
    if (!pbLocApiUptBatchOpt.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiUptBatchOpt failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIUpdateBatchingOptionsReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocAPIAddGeofencesReqMsg -> PBLocAPIAddGeofencesReqMsg
int LocAPIAddGeofencesReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIAddGeofencesReqMsg pbLocApiAddGfReqMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIAddGeofencesReqMsg conversion
    // PBGeofencesAddedReqPayload geofences = 1;
    PBGeofencesAddedReqPayload* gfAddReqPayload = pbLocApiAddGfReqMsg.mutable_geofences();
    if (nullptr != gfAddReqPayload) {
        if (pLocApiPbMsgConv->convertGfAddedReqPayloadToPB(geofences, gfAddReqPayload)) {
            LOC_LOGe("convertGfAddedReqPayloadToPB failed");
            free(gfAddReqPayload);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_geofences failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiAddGfReqMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiAddGfReqMsg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIAddGeofencesReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIAddGeofencesReqMsg(pbLocApiAddGfReqMsg);
    return protoStr.size();
}

// Convert LocAPIRemoveGeofencesReqMsg -> PBLocAPIRemoveGeofencesReqMsg
int LocAPIRemoveGeofencesReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIRemoveGeofencesReqMsg pbLocApiRemGf;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIRemoveGeofencesReqMsg conversion
    // PBGeofencesReqClientIdPayload gfClientIds = 1;
    PBGeofencesReqClientIdPayload* gfReqClntIdPayload = pbLocApiRemGf.mutable_gfclientids();
    if (nullptr != gfReqClntIdPayload) {
        if (pLocApiPbMsgConv->convertGfReqClientIdPayloadToPB(gfClientIds, gfReqClntIdPayload)) {
            LOC_LOGe("convertGfReqClientIdPayloadToPB failed");
            free(gfReqClntIdPayload);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_gfclientids failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiRemGf.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiRemGf failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIRemoveGeofencesReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIRemoveGeofencesReqMsg(pbLocApiRemGf);
    return protoStr.size();
}

// Convert LocAPIModifyGeofencesReqMsg -> PBLocAPIModifyGeofencesReqMsg
int LocAPIModifyGeofencesReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIModifyGeofencesReqMsg pbLocApiModGf;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIModifyGeofencesReqMsg conversion
    // PBGeofencesAddedReqPayload geofences = 1;
    PBGeofencesAddedReqPayload* gfModReqPayload = pbLocApiModGf.mutable_geofences();
    if (nullptr != gfModReqPayload) {
        if (pLocApiPbMsgConv->convertGfAddedReqPayloadToPB(geofences, gfModReqPayload)) {
            LOC_LOGe("convertGfAddedReqPayloadToPB failed");
            free(gfModReqPayload);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_geofences failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiModGf.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiModGf failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIModifyGeofencesReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIModifyGeofencesReqMsg(pbLocApiModGf);
    return protoStr.size();
}

// Convert LocAPIPauseGeofencesReqMsg -> PBLocAPIPauseGeofencesReqMsg
int LocAPIPauseGeofencesReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIPauseGeofencesReqMsg pbLocApiPauseGf;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIPauseGeofencesReqMsg conversion
    // PBGeofencesReqClientIdPayload gfClientIds = 1;
    PBGeofencesReqClientIdPayload* gfReqClntIdPayload = pbLocApiPauseGf.mutable_gfclientids();
    if (nullptr != gfReqClntIdPayload) {
        if (pLocApiPbMsgConv->convertGfReqClientIdPayloadToPB(gfClientIds, gfReqClntIdPayload)) {
            LOC_LOGe("convertGfReqClientIdPayloadToPB failed");
            free(gfReqClntIdPayload);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_gfclientids failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiPauseGf.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiPauseGf failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIPauseGeofencesReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIPauseGeofencesReqMsg(pbLocApiPauseGf);
    return protoStr.size();
}

// Convert LocAPIResumeGeofencesReqMsg -> PBLocAPIResumeGeofencesReqMsg
int LocAPIResumeGeofencesReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIResumeGeofencesReqMsg pbLocApiResumeGf;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIResumeGeofencesReqMsg conversion
    // PBGeofencesReqClientIdPayload gfClientIds = 1;
    PBGeofencesReqClientIdPayload* gfReqClntIdPayload = pbLocApiResumeGf.mutable_gfclientids();
    if (nullptr != gfReqClntIdPayload) {
        if (pLocApiPbMsgConv->convertGfReqClientIdPayloadToPB(gfClientIds, gfReqClntIdPayload)) {
            LOC_LOGe("convertGfReqClientIdPayloadToPB failed");
            free(gfReqClntIdPayload);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_gfclientids failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiResumeGf.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiResumeGf failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIResumeGeofencesReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIResumeGeofencesReqMsg(pbLocApiResumeGf);
    return protoStr.size();
}

// Convert LocAPIUpdateNetworkAvailabilityReqMsg -> PBLocAPIUpdateNetworkAvailabilityReqMsg
int LocAPIUpdateNetworkAvailabilityReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIUpdateNetworkAvailabilityReqMsg pbLocApiUptNetwAvail;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIUpdateNetworkAvailabilityReqMsg conversion
    // bool mAvailability = 1;
    pbLocApiUptNetwAvail.set_mavailability(mAvailability);

    string pbStr;
    if (!pbLocApiUptNetwAvail.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiUptNetwAvail failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIUpdateNetworkAvailabilityReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocAPIGetGnssEnergyConsumedReqMsg -> PBLocAPIGetGnssEnergyConsumedReqMsg
int LocAPIGetGnssEnergyConsumedReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // bytes       payload = 4;
    // LocAPIGetGnssEnergyConsumedReqMsg - no struct member. No payload to send
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIGetGnssEnergyConsumedReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocAPIGetSingleTerrestrialPosReqMsg ->
// PBLocAPIGetSingleTerrestrialPosReqMsg
int LocAPIGetSingleTerrestrialPosReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIGetSingleTerrestrialPosReqMsg pbLocGetTerrestrialPosReq;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // Convert payload to PBLocAPIGetGnssEnergyConsumedReqMsg
    // uint32 timeoutMsec = 1;
    pbLocGetTerrestrialPosReq.set_timeoutmsec(mTimeoutMsec);
    // PBTerrestrialTechMask techMask = 2;
    pbLocGetTerrestrialPosReq.set_techmask((::PBTerrestrialTechMask)
            pLocApiPbMsgConv->getPBMaskForTerrestrialTechMask(mTechMask));
    // float horQoS = 3;
    pbLocGetTerrestrialPosReq.set_horqos(mHorQoS);

    string pbStr;
    if (!pbLocGetTerrestrialPosReq.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocGetTerrestrialPosReq failed!");
        return 0;
    }

    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIGetSingleTerrestrialPosReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Decode PBLocAPIGetSingleTerrestrialPosRespMsg -> LocAPIGetSingleTerrestrialPosRespMsg
LocAPIGetSingleTerrestrialPosReqMsg::LocAPIGetSingleTerrestrialPosReqMsg(
            const char* name,
            const PBLocAPIGetSingleTerrestrialPosReqMsg &pbLocGetTerrestrialPosReq,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_REQ_MSG_ID, pbMsgConv) {
    // >>>> PBLocAPIGetSingleTerrestrialPosReqMsg conversion
    mTimeoutMsec = pbLocGetTerrestrialPosReq.timeoutmsec();
    mTechMask = (TerrestrialTechMask)pLocApiPbMsgConv->getTerrestrialTechMaskFromPB(
            pbLocGetTerrestrialPosReq.techmask());
    mHorQoS = pbLocGetTerrestrialPosReq.horqos();
}

// Encode LocAPIGetSingleTerrestrialPosRespMsg -> PBLocAPIGetSingleTerrestrialPosRespMsg
int LocAPIGetSingleTerrestrialPosRespMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIGetSingleTerrestrialPosRespMsg pbLocGetTerrestrialPosResp;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // PBLocationError errorCode = 1;
    pbLocGetTerrestrialPosResp.set_errorcode(
            (::PBLocationError) pLocApiPbMsgConv->getPBEnumForLocationError(mErrorCode));

    // PBLocation      location = 2;
    PBLocation* pbLocation = pbLocGetTerrestrialPosResp.mutable_location();
    if (nullptr != pbLocation) {
        if (pLocApiPbMsgConv->convertLocationToPB(mLocation, pbLocation)) {
            LOC_LOGe("convertLocationToPB failed");
            pbLocGetTerrestrialPosResp.clear_location();
            return 0;
        }
    } else {
        LOC_LOGe("mutable_locationnotification failed");
        return 0;
    }

    string pbStr;
    if (!pbLocGetTerrestrialPosResp.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocGetTerrestrialPosResp failed!");
        return 0;
    }

    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIGetSingleTerrestrialPosRespMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }

    // free the location
    pbLocGetTerrestrialPosResp.clear_location();
    return protoStr.size();
}

// Decode PBLocAPIGetSingleTerrestrialPosRespMsg -> LocAPIGetSingleTerrestrialPosRespMsg
LocAPIGetSingleTerrestrialPosRespMsg::LocAPIGetSingleTerrestrialPosRespMsg(
            const char* name,
            const PBLocAPIGetSingleTerrestrialPosRespMsg &pbLocGetTerrestrialPosResp,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_GET_SINGLE_TERRESTRIAL_POS_RESP_MSG_ID, pbMsgConv) {

    mErrorCode = pLocApiPbMsgConv->getEnumForPBLocationError(
            pbLocGetTerrestrialPosResp.errorcode());

    memset(&mLocation, 0, sizeof(mLocation));
    // >>>> PBLocAPILocationIndMsg conversion
    // PBLocation locationNotification = 1;
    if (pbLocGetTerrestrialPosResp.has_location()) {
        pLocApiPbMsgConv->pbConvertToLocation(pbLocGetTerrestrialPosResp.location(),
                mLocation);
    }
}

// Convert LocAPIGetSinglePosReqMsg ->
// PBLocAPIGetSinglePosReqMsg
int LocAPIGetSinglePosReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIGetSinglePosReqMsg pbLocGetPosReq;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // Convert payload to PBLocAPIGetGnssEnergyConsumedReqMsg
    // uint32 timeoutMsec = 1;
    pbLocGetPosReq.set_timeoutmsec(mTimeoutMsec);
    // float horQoS = 2;
    pbLocGetPosReq.set_horqos(mHorQoS);

    string pbStr;
    if (!pbLocGetPosReq.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocGetPosReq failed!");
        return 0;
    }

    pLocApiMsgHdr.set_payload(pbStr);
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIGetSinglePosReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Decode PBLocAPIGetSinglePosRespMsg -> LocAPIGetSinglePosRespMsg
LocAPIGetSinglePosReqMsg::LocAPIGetSinglePosReqMsg(
            const char* name,
            const PBLocAPIGetSinglePosReqMsg &pbLocGetPosReq,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_GET_SINGLE_POS_REQ_MSG_ID, pbMsgConv) {
    // >>>> PBLocAPIGetSinglePosReqMsg conversion
    mTimeoutMsec = pbLocGetPosReq.timeoutmsec();
    mHorQoS = pbLocGetPosReq.horqos();
}

// Encode LocAPIGetSinglePosRespMsg -> PBLocAPIGetSinglePosRespMsg
int LocAPIGetSinglePosRespMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIGetSinglePosRespMsg pbLocGetPosResp;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // PBLocationError errorCode = 1;
    pbLocGetPosResp.set_errorcode(
            (::PBLocationError) pLocApiPbMsgConv->getPBEnumForLocationError(mErrorCode));

    // PBLocation      location = 2;
    PBLocation* pbLocation = pbLocGetPosResp.mutable_location();
    if (nullptr != pbLocation) {
        if (pLocApiPbMsgConv->convertLocationToPB(mLocation, pbLocation)) {
            LOC_LOGe("convertLocationToPB failed");
            pbLocGetPosResp.clear_location();
            return 0;
        }
    } else {
        LOC_LOGe("mutable_locationnotification failed");
        return 0;
    }

    string pbStr;
    if (!pbLocGetPosResp.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocGetPosResp failed!");
        pbLocGetPosResp.clear_location();
        return 0;
    }

    pLocApiMsgHdr.set_payload(pbStr);
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIGetSinglePosRespMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        pbLocGetPosResp.clear_location();
        return 0;
    }

    // free the location
    pbLocGetPosResp.clear_location();
    return protoStr.size();
}

// Decode PBLocAPIGetSinglePosRespMsg -> LocAPIGetSinglePosRespMsg
LocAPIGetSinglePosRespMsg::LocAPIGetSinglePosRespMsg(
            const char* name,
            const PBLocAPIGetSinglePosRespMsg &pbLocGetPosResp,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_GET_SINGLE_POS_RESP_MSG_ID, pbMsgConv) {

    mErrorCode = pLocApiPbMsgConv->getEnumForPBLocationError(
            pbLocGetPosResp.errorcode());

    memset(&mLocation, 0, sizeof(mLocation));
    if (pbLocGetPosResp.has_location()) {
        pLocApiPbMsgConv->pbConvertToLocation(pbLocGetPosResp.location(),
                mLocation);
    }
}

// Convert LocAPILocationIndMsg -> PBLocAPILocationIndMsg
int LocAPILocationIndMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPILocationIndMsg pbLocApiLocInd;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPILocationIndMsg conversion
    // PBLocation locationNotification = 1;
    PBLocation* location = pbLocApiLocInd.mutable_locationnotification();
    if (nullptr != location) {
        if (pLocApiPbMsgConv->convertLocationToPB(locationNotification, location)) {
            LOC_LOGe("convertLocationToPB failed");
            free(location);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_locationnotification failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiLocInd.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiLocInd failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPILocationIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPILocationIndMsg(pbLocApiLocInd);
    return protoStr.size();
}

// Convert LocAPIBatchingIndMsg -> PBLocAPIBatchingIndMsg
int LocAPIBatchingIndMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIBatchingIndMsg pbLocApiBatchInd;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIBatchingIndMsg conversion
    // PBLocAPIBatchNotification batchNotification = 1;
    PBLocAPIBatchNotification* locApiBatchIndMsg = pbLocApiBatchInd.mutable_batchnotification();
    if (nullptr != locApiBatchIndMsg) {
        if (pLocApiPbMsgConv->convertLocAPIBatchingNotifMsgToPB(batchNotification,
                locApiBatchIndMsg)) {
            LOC_LOGe("convertLocAPIBatchingNotifMsgToPB failed");
            free(locApiBatchIndMsg);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_batchnotification failed");
        return 0;
    }
    // PBBatchingMode batchingMode = 2;
    pbLocApiBatchInd.set_batchingmode(pLocApiPbMsgConv->getPBEnumForBatchingMode(batchingMode));
    string pbStr;
    if (!pbLocApiBatchInd.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiBatchInd failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIBatchingIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIBatchingIndMsg(pbLocApiBatchInd);
    return protoStr.size();
}

// Convert LocAPIGeofenceBreachIndMsg -> PBLocAPIGeofenceBreachIndMsg
int LocAPIGeofenceBreachIndMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIGeofenceBreachIndMsg pbLocApiGfBreach;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIGeofenceBreachIndMsg conversion
    // PBLocAPIGeofenceBreachNotification gfBreachNotification = 1;
    PBLocAPIGeofenceBreachNotification* locApiGfBreachNotif =
            pbLocApiGfBreach.mutable_gfbreachnotification();
    if (nullptr != locApiGfBreachNotif) {
        if (pLocApiPbMsgConv->convertLocAPIGfBreachNotifToPB(gfBreachNotification,
                locApiGfBreachNotif)) {
            LOC_LOGe("convertLocAPIGfBreachNotifToPB failed");
            free(locApiGfBreachNotif);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_gfbreachnotification failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiGfBreach.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiGfBreach failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIGeofenceBreachIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIGeofenceBreachIndMsg(pbLocApiGfBreach);
    return protoStr.size();
}

// Convert LocAPILocationInfoIndMsg -> PBLocAPILocationInfoIndMsg
int LocAPILocationInfoIndMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPILocationInfoIndMsg pbLocApiLocInfoInd;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPILocationInfoIndMsg conversion
    // PBGnssLocationInfoNotification gnssLocationInfoNotification = 1;
    PBGnssLocationInfoNotification* gnssLocInfoNotif =
            pbLocApiLocInfoInd.mutable_gnsslocationinfonotification();
    if (nullptr != gnssLocInfoNotif) {
        if (pLocApiPbMsgConv->convertGnssLocInfoNotifToPB(gnssLocationInfoNotification,
                gnssLocInfoNotif)) {
            LOC_LOGe("convertGnssLocInfoNotifToPB failed");
            free(gnssLocInfoNotif);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_gnsslocationinfonotification failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiLocInfoInd.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiLocInfoInd failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPILocationInfoIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPILocationInfoIndMsg(pbLocApiLocInfoInd);
    return protoStr.size();
}

// Convert LocAPIEngineLocationsInfoIndMsg -> PBLocAPIEngineLocationsInfoIndMsg
int LocAPIEngineLocationsInfoIndMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIEngineLocationsInfoIndMsg pbLocApiEngLocInfo;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIEngineLocationsInfoIndMsg conversion
    // max array size - PBLocApiOutputEngineType::PB_LOC_OUTPUT_ENGINE_COUNT
    // repeated PBGnssLocationInfoNotification engineLocationsInfo = 1;
    uint32_t arrLen = min(count, (uint32_t) LOC_OUTPUT_ENGINE_COUNT);
    for (uint32_t i = 0; i < arrLen; i++) {
        PBGnssLocationInfoNotification *gnssLocInfoNotif =
                pbLocApiEngLocInfo.add_enginelocationsinfo();
        if (nullptr != gnssLocInfoNotif) {
            if (pLocApiPbMsgConv->convertGnssLocInfoNotifToPB(engineLocationsInfo[i],
                    gnssLocInfoNotif)) {
                LOC_LOGe("convertGnssLocInfoNotifToPB failed");
                free(gnssLocInfoNotif);
                return 0;
            }
        } else {
            LOC_LOGe("add_enginelocationsinfo is NULL");
            return 0;
        }
    }

    string pbStr;
    if (!pbLocApiEngLocInfo.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiEngLocInfo failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIEngineLocationsInfoIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIEngineLocationsInfoIndMsg(pbLocApiEngLocInfo);
    return protoStr.size();
}

// Convert LocAPISatelliteVehicleIndMsg -> PBLocAPISatelliteVehicleIndMsg
int LocAPISatelliteVehicleIndMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPISatelliteVehicleIndMsg pbLocApiSatVehInd;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPISatelliteVehicleIndMsg conversion
    // PBLocApiGnssSvNotification gnssSvNotification = 1;
    PBLocApiGnssSvNotification* gnssSvNotif = pbLocApiSatVehInd.mutable_gnsssvnotification();
    if (nullptr != gnssSvNotif) {
        if (pLocApiPbMsgConv->convertGnssSvNotifToPB(gnssSvNotification, gnssSvNotif)) {
            LOC_LOGe("convertGnssSvNotifToPB failed");
            free(gnssSvNotif);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_gnsssvnotification failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiSatVehInd.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiSatVehInd failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPISatelliteVehicleIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPISatelliteVehicleIndMsg(pbLocApiSatVehInd);
    return protoStr.size();
}

// Convert LocAPINmeaIndMsg -> PBLocAPINmeaIndMsg
int LocAPINmeaIndMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPINmeaIndMsg pbLocApiNmeaInd;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPINmeaIndMsg conversion
    // PBLocAPINmeaSerializedPayload gnssNmeaNotification = 1;
    PBLocAPINmeaSerializedPayload* locAPINmeaSerPload =
            pbLocApiNmeaInd.mutable_gnssnmeanotification();
    if (nullptr != locAPINmeaSerPload) {
        if (pLocApiPbMsgConv->convertLocAPINmeaSerializedPayloadToPB(gnssNmeaNotification,
                locAPINmeaSerPload)) {
            LOC_LOGe("convertLocAPINmeaSerializedPayloadToPB failed");
            free(locAPINmeaSerPload);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_gnssnmeanotification failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiNmeaInd.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiNmeaInd failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPINmeaIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPINmeaIndMsg(pbLocApiNmeaInd);
    return protoStr.size();
}

// Convert LocAPIDataIndMsg -> PBLocAPIDataIndMsg
int LocAPIDataIndMsg ::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIDataIndMsg pbLocApiDataInd;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIDataIndMsg conversion
    // PBGnssDataNotification gnssDataNotification = 1;
    PBGnssDataNotification* gnssDataNotif = pbLocApiDataInd.mutable_gnssdatanotification();
    if (nullptr != gnssDataNotif) {
        if (pLocApiPbMsgConv->convertGnssDataNotifToPB(gnssDataNotification, gnssDataNotif)) {
            LOC_LOGe("convertGnssDataNotifToPB failed");
            free(gnssDataNotif);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_gnssdatanotification failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiDataInd.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiDataInd failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIDataIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIDataIndMsg(pbLocApiDataInd);
    return protoStr.size();
}

// Convert LocAPIMeasIndMsg -> PBLocAPIMeasIndMsg
int LocAPIMeasIndMsg ::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIMeasIndMsg pbLocApiMeasInd;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIMeasIndMsg conversion
    // PBGnssMeasurementsNotification gnssMeasurementsNotification = 1;
    PBGnssMeasurementsNotification* gnssMeasNotif =
            pbLocApiMeasInd.mutable_gnssmeasurementsnotification();
    if (nullptr != gnssMeasNotif) {
        if (pLocApiPbMsgConv->convertGnssMeasNotifToPB(gnssMeasurementsNotification,
                gnssMeasNotif)) {
            LOC_LOGe("convertGnssMeasNotifToPB failed");
            free(gnssMeasNotif);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_gnssmeasurementsnotification failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiMeasInd.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiMeasInd failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIMeasIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIMeasIndMsg(pbLocApiMeasInd);
    return protoStr.size();
}

// Convert LocAPIGnssEnergyConsumedIndMsg -> PBLocAPIGnssEnergyConsumedIndMsg
int LocAPIGnssEnergyConsumedIndMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIGnssEnergyConsumedIndMsg pbLocApiGnssEnrgyConsmdInd;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIGnssEnergyConsumedIndMsg conversion
    // uint64 totalGnssEnergyConsumedSinceFirstBoot = 1;
    pbLocApiGnssEnrgyConsmdInd.set_totalgnssenergyconsumedsincefirstboot(
            totalGnssEnergyConsumedSinceFirstBoot);

    string pbStr;
    if (!pbLocApiGnssEnrgyConsmdInd.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiGnssEnrgyConsmdInd failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIGnssEnergyConsumedIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocAPILocationSystemInfoIndMsg -> PBLocAPILocationSystemInfoIndMsg
int LocAPILocationSystemInfoIndMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPILocationSystemInfoIndMsg pbLocApiLocSysInfoInd;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPILocationSystemInfoIndMsg conversion
    // PBLocationSystemInfo locationSystemInfo = 1;
    PBLocationSystemInfo* locSysInfo = pbLocApiLocSysInfoInd.mutable_locationsysteminfo();
    if (nullptr != locSysInfo) {
        if (pLocApiPbMsgConv->convertLocSysInfoToPB(locationSystemInfo, locSysInfo)) {
            LOC_LOGe("convertLocSysInfoToPB failed");
            free(locSysInfo);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_locationsysteminfo failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiLocSysInfoInd.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiLocSysInfoInd failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPILocationSystemInfoIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPILocationSystemInfoIndMsg(pbLocApiLocSysInfoInd);
    return protoStr.size();
}

// Convert LocAPIDcReportIndMsg -> PBLocAPIDcReportIndMsg
int LocAPIDcReportIndMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIDcReportIndMsg pbMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    PBGnssDcReportInfo* pbDcReportInfo = pbMsg.mutable_dcreportinfo();
    if (nullptr != pbDcReportInfo) {
        if (pLocApiPbMsgConv->convertGnssDcReportToPB(dcReportInfo, pbDcReportInfo)) {
            LOC_LOGe("convertGnssDcReportToPB failed");
            pbMsg.clear_dcreportinfo();
            return 0;
        }
    } else {
        LOC_LOGe("mutable_dcreportinfo failed");
        return 0;
    }

    string pbStr;
    if (!pbMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbMsg failed!");
        pbMsg.clear_dcreportinfo();
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIDcReportIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        pbMsg.clear_dcreportinfo();
        return 0;
    }
    // free memory
    pbMsg.clear_dcreportinfo();
    return protoStr.size();
}

// Convert LocConfigConstrainedTuncReqMsg -> PBLocConfigConstrainedTuncReqMsg
int LocConfigConstrainedTuncReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigConstrainedTuncReqMsg pbLocConfConstrTunc;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigConstrainedTuncReqMsg conversion
    // bool     mEnable = 1;
    pbLocConfConstrTunc.set_menable(mEnable);
    // float    mTuncConstraint = 2;
    pbLocConfConstrTunc.set_mtuncconstraint(mTuncConstraint);
    // uint32   mEnergyBudget = 3;
    pbLocConfConstrTunc.set_menergybudget(mEnergyBudget);

    string pbStr;
    if (!pbLocConfConstrTunc.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocConfConstrTunc failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigConstrainedTuncReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocConfigPositionAssistedClockEstimatorReqMsg ->
// PBLocConfigPositionAssistedClockEstimatorReqMsg
int LocConfigPositionAssistedClockEstimatorReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigPositionAssistedClockEstimatorReqMsg pbLocConfPosAsstdClockEst;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigPositionAssistedClockEstimatorReqMsg conversion
    // bool     mEnable = 1;
    pbLocConfPosAsstdClockEst.set_menable(mEnable);

    string pbStr;
    if (!pbLocConfPosAsstdClockEst.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocConfPosAsstdClockEst failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigPositionAssistedClockEstimatorReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocConfigSvConstellationReqMsg -> PBLocConfigSvConstellationReqMsg
int LocConfigSvConstellationReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigSvConstellationReqMsg pbLocConfSvConst;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // PBGnssSvTypeConfig mConstellationEnablementConfig = 1;
    PBGnssSvTypeConfig *gnssSvTypCfg = pbLocConfSvConst.mutable_mconstellationenablementconfig();
    if (nullptr != gnssSvTypCfg) {
        if (pLocApiPbMsgConv->convertGnssSvTypeConfigToPB(mConstellationEnablementConfig,
                gnssSvTypCfg)) {
            LOC_LOGe("convertGnssSvTypeConfigToPB failed");
            free(gnssSvTypCfg);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_mconstellationenablementconfig failed");
        return 0;
    }

    // PBGnssSvIdConfig   mBlacklistSvConfig = 2;
    PBGnssSvIdConfig *gnssSvIdCfg = pbLocConfSvConst.mutable_mblacklistsvconfig();
    if (nullptr != gnssSvIdCfg) {
        if (pLocApiPbMsgConv->convertGnssSvIdConfigToPB(mBlacklistSvConfig, gnssSvIdCfg)) {
            LOC_LOGe("convertGnssSvIdConfigToPB failed");
            free(gnssSvIdCfg);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_mblacklistsvconfig failed");
        return 0;
    }

    // bool mResetToDefault = 3;
    // size field in constellationEnablementConfig to 0 to indicate to restore to modem default
    bool resetToDefault = (0 == mConstellationEnablementConfig.size);
    pbLocConfSvConst.set_mresettodefault(resetToDefault);

    string pbStr;
    if (!pbLocConfSvConst.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocConfSvConst failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigSvConstellationReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }

    // free memory
    pLocApiPbMsgConv->freeUpPBLocConfigSvConstellationReqMsg(pbLocConfSvConst);
    return protoStr.size();
}

// Convert LocConfigConstellationSecondaryBandReqMsg -> PBLocConfigConstellationSecondaryBandReqMsg
int LocConfigConstellationSecondaryBandReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigConstellationSecondaryBandReqMsg pbLocCfgConstlSecBandReqMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigConstellationSecondaryBandReqMsg conversion
    // PBGnssSvTypeConfig mSecondaryBandConfig = 1;
    PBGnssSvTypeConfig *gnssSvTypCfg =
            pbLocCfgConstlSecBandReqMsg.mutable_msecondarybandconfig();
    if (nullptr != gnssSvTypCfg) {
        if (pLocApiPbMsgConv->convertGnssSvTypeConfigToPB(mSecondaryBandConfig,
                gnssSvTypCfg)) {
            LOC_LOGe("convertGnssSvTypeConfigToPB failed");
            free(gnssSvTypCfg);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_msecondarybandconfig failed");
        return 0;
    }

    string pbStr;
    if (!pbLocCfgConstlSecBandReqMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocCfgConstlSecBandReqMsg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigConstellationSecondaryBandReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocConConstllSecBandReqMsg(pbLocCfgConstlSecBandReqMsg);
    return protoStr.size();
}

// Convert LocConfigAidingDataDeletionReqMsg -> PBLocConfigAidingDataDeletionReqMsg
int LocConfigAidingDataDeletionReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigAidingDataDeletionReqMsg pbLocConfAidDataDel;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigAidingDataDeletionReqMsg conversion
    // PBAidingData mAidingData = 1;
    PBAidingData* aidingData = pbLocConfAidDataDel.mutable_maidingdata();
    if (nullptr != aidingData) {
        if (pLocApiPbMsgConv->convertGnssAidingDataToPB(mAidingData, aidingData)) {
            LOC_LOGe("convertGnssAidingDataToPB failed");
            free(aidingData);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_maidingdata failed");
        return 0;
    }

    string pbStr;
    if (!pbLocConfAidDataDel.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocConfAidDataDel failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigAidingDataDeletionReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocConAidingDataDeletionReqMsg(pbLocConfAidDataDel);
    return protoStr.size();
}

// Convert LocConfigLeverArmReqMsg -> PBLocConfigLeverArmReqMsg
int LocConfigLeverArmReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigLeverArmReqMsg pbLocConfLeverArm;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigLeverArmReqMsg conversion
    // PBLIALeverArmConfigInfo mLeverArmConfigInfo = 1;
    PBLIALeverArmConfigInfo* leverArmCfgInfo = pbLocConfLeverArm.mutable_mleverarmconfiginfo();
    if (nullptr != leverArmCfgInfo) {
        if (pLocApiPbMsgConv->convertLeverArmConfigInfoToPB(mLeverArmConfigInfo,
                leverArmCfgInfo)) {
            LOC_LOGe("convertLeverArmConfigInfoToPB failed");
            free(leverArmCfgInfo);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_mleverarmconfiginfo failed");
        return 0;
    }

    string pbStr;
    if (!pbLocConfLeverArm.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocConfLeverArm failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigLeverArmReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocConfigLeverArmReqMsg(pbLocConfLeverArm);
    return protoStr.size();
}

// Convert LocConfigRobustLocationReqMsg -> PBLocConfigRobustLocationReqMsg
int LocConfigRobustLocationReqMsg::serializeToProtobuf(string& protoStr) {
        PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigRobustLocationReqMsg pbLocConfRobustLoc;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigRobustLocationReqMsg conversion
    // bool mEnable = 1;
    pbLocConfRobustLoc.set_menable(mEnable);
    // bool mEnableForE911 = 2;
    pbLocConfRobustLoc.set_menablefore911(mEnableForE911);

    string pbStr;
    if (!pbLocConfRobustLoc.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocConfRobustLoc failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigRobustLocationReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocConfigMinGpsWeekReqMsg -> PBLocConfigMinGpsWeekReqMsg
int LocConfigMinGpsWeekReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigMinGpsWeekReqMsg pbLocConfMinGpsWeek;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigMinGpsWeekReqMsg conversion
    // uint32 mMinGpsWeek = 1;
    pbLocConfMinGpsWeek.set_mmingpsweek(mMinGpsWeek);

    string pbStr;
    if (!pbLocConfMinGpsWeek.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocConfMinGpsWeek failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigMinGpsWeekReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocConfigDrEngineParamsReqMsg -> PBLocConfigDrEngineParamsReqMsg
int LocConfigDrEngineParamsReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigDrEngineParamsReqMsg pbLocCfgDrEngParamReq;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigDrEngineParamsReqMsg conversion
    // PBDeadReckoningEngineConfig mDreConfig = 1;
    PBDeadReckoningEngineConfig *drReckoningEngConfig = pbLocCfgDrEngParamReq.mutable_mdreconfig();
    if (nullptr != drReckoningEngConfig) {
        if (pLocApiPbMsgConv->convertDeadReckoningEngineConfigToPB(mDreConfig,
                drReckoningEngConfig)) {
            LOC_LOGe("pbConvertToDeadReckoningEngineConfig failed");
            free(drReckoningEngConfig);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_mdreconfig failed");
        return 0;
    }

    string pbStr;
    if (!pbLocCfgDrEngParamReq.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocConfB2sMntParam failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigDrEngineParamsReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocConfDrEngineParamsReqMsg(pbLocCfgDrEngParamReq);
    return protoStr.size();
}

// Convert LocConfigMinSvElevationReqMsg -> PBLocConfigMinSvElevationReqMsg
int LocConfigMinSvElevationReqMsg::serializeToProtobuf(string& protoStr) {
        PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigMinSvElevationReqMsg pbLocConfMinSvElev;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigMinSvElevationReqMsg conversion
    // uint32 mMinSvElevation = 1;
    pbLocConfMinSvElev.set_mminsvelevation(mMinSvElevation);

    string pbStr;
    if (!pbLocConfMinSvElev.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocConfMinSvElev failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigMinSvElevationReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocConfigEngineRunStateReqMsg -> PBLocConfigEngineRunStateReqMsg
int LocConfigEngineRunStateReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigEngineRunStateReqMsg pbLocConfEngineRunState;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigEngineRunStateReqMsg conversion
    // PBLocApiPositioningEngineMask mEngType = 1;
    pbLocConfEngineRunState.set_mengtype((::PBLocApiPositioningEngineMask)
            pLocApiPbMsgConv->getPBMaskForPositioningEngineMask(mEngType));
    // PBLocEngineRunState mEngState = 2;
    pbLocConfEngineRunState.set_mengstate((::PBLocEngineRunState)
            pLocApiPbMsgConv->getPBEnumForLocEngineRunState(mEngState));

    string pbStr;
    if (!pbLocConfEngineRunState.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocConfEngineRunState failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigEngineRunStateReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocConfigUserConsentTerrestrialPositioningReqMsg ->
//      PBLocConfigUserConsentTerrestrialPositioningReqMsg
int LocConfigUserConsentTerrestrialPositioningReqMsg::serializeToProtobuf(
        string& protoStr) {

    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigUserConsentTerrestrialPositioningReqMsg pbMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigUserConsentTerrestrialPositioningReqMsg conversion
    // bool userConsent
    pbMsg.set_userconsent(mUserConsent);

    string pbStr;
    if (!pbMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(
            sizeof(LocConfigUserConsentTerrestrialPositioningReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Decode PBLocConfigUserConsentTerrestrialPositioningReqMsg ->
//      LocConfigUserConsentTerrestrialPositioningReqMsg
LocConfigUserConsentTerrestrialPositioningReqMsg::
        LocConfigUserConsentTerrestrialPositioningReqMsg(
            const char* name,
            const PBLocConfigUserConsentTerrestrialPositioningReqMsg &pbMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name,
                        E_INTAPI_CONFIG_USER_CONSENT_TERRESTRIAL_POSITIONING_MSG_ID,
                        pbMsgConv) {
    mUserConsent = pbMsg.userconsent();
}

// Convert LocConfigOutputNmeaTypesReqMsg -> PBLocConfigOutputNmeaTypesReqMsg
int LocConfigOutputNmeaTypesReqMsg::serializeToProtobuf(
        string& protoStr) {

    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigOutputNmeaTypesReqMsg pbMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    pbMsg.set_nmeatypesmask(pLocApiPbMsgConv->getPBMaskForNmeaTypesMask(mEnabledNmeaTypes));
    pbMsg.set_nmeadatumtype((mNmeaDatumType == GEODETIC_TYPE_PZ_90) ?
                            PB_GEODETIC_TYPE_PZ_90 : PB_GEODETIC_TYPE_WGS_84);
    pbMsg.set_nmeareqengmask(pLocApiPbMsgConv->getPBMaskForLocReqEngineTypeMask(mNmeaReqEngMask));

    string pbStr;
    if (!pbMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(
            sizeof(LocConfigOutputNmeaTypesReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Decode PBLocConfigOutputNmeaTypesReqMsg -> LocConfigOutputNmeaTypesReqMsg
LocConfigOutputNmeaTypesReqMsg::
        LocConfigOutputNmeaTypesReqMsg(
            const char* name,
            const PBLocConfigOutputNmeaTypesReqMsg &pbMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name,
                        E_INTAPI_CONFIG_OUTPUT_NMEA_TYPES_MSG_ID,
                        pbMsgConv) {

    mEnabledNmeaTypes = (GnssNmeaTypesMask)
            pLocApiPbMsgConv->getNmeaTypesMaskFromPB(pbMsg.nmeatypesmask());
    mNmeaDatumType = (pbMsg.nmeadatumtype() == PB_GEODETIC_TYPE_PZ_90) ?
            GEODETIC_TYPE_PZ_90 : GEODETIC_TYPE_WGS_84;
    mNmeaReqEngMask = (LocReqEngineTypeMask)
            pLocApiPbMsgConv->getLocReqEngineTypeMaskFromPB(pbMsg.nmeareqengmask());
}

// Convert LocConfigEngineIntegrityRiskReqMsg ->
// PBLocConfigEngineIntegrityRiskReqMsg
int LocConfigEngineIntegrityRiskReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigEngineIntegrityRiskReqMsg pbLocConfMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // PBLocApiPositioningEngineMask engType = 1;
    pbLocConfMsg.set_engtype((::PBLocApiPositioningEngineMask)
            pLocApiPbMsgConv->getPBMaskForPositioningEngineMask(mEngType));
    // uint32_t integrityrisk = 2;
    pbLocConfMsg.set_integrityrisk(mIntegrityRisk);

    string pbStr;
    if (!pbLocConfMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on PBLocConfigEngineIntegrityRiskReqMsg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigEngineIntegrityRiskReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Decode PBLocConfigEngineIntegrityRiskReqMsg -> LocConfigEngineIntegrityRiskReqMsg
LocConfigEngineIntegrityRiskReqMsg::LocConfigEngineIntegrityRiskReqMsg(
        const char* name,
        const PBLocConfigEngineIntegrityRiskReqMsg &pbConfigEngineIntegrityRiskReqMsg,
        const LocationApiPbMsgConv *pbMsgConv):
    LocAPIMsgHeader(name, E_INTAPI_CONFIG_ENGINE_INTEGRITY_RISK_MSG_ID, pbMsgConv) {
    // >>>> PBLocConfigEngineRunStateReqMsg conversion
    mEngType = (PositioningEngineMask) pLocApiPbMsgConv->getEnumForPBPositioningEngineMask(
            pbConfigEngineIntegrityRiskReqMsg.engtype());
    mIntegrityRisk = pbConfigEngineIntegrityRiskReqMsg.integrityrisk();
    LOC_LOGd("LocApiPB: eng type %d, integrity risk %d", mEngType, mIntegrityRisk);
}

// Convert LocConfigMerkleTreeReqMsg ->
// PBLocConfigMerkleTreeReqMsg
int LocConfigMerkleTreeReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigMerkleTreeReqMsg pbLocConfMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // string mMerkleTreeConfig = 1;
    pbLocConfMsg.set_mmerkletreeconfig(mMerkleTreeConfig);

    string pbStr;
    if (!pbLocConfMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on PBLocConfigMerkleTreeReqMsg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigMerkleTreeReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }

    return protoStr.size();
}

// Convert LocConfigOsnmaEnablementReqMsg ->
// PBLocConfigOsnmaEnablementReqMsg
int LocConfigOsnmaEnablementReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigOsnmaEnablementReqMsg pbLocConfMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // bool mEnable = 1;
    pbLocConfMsg.set_menable(mEnable);

    string pbStr;
    if (!pbLocConfMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on PBLocConfigOsnmaEnablementReqMsg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigOsnmaEnablementReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }

    return protoStr.size();
}

/************************** XTRA *****************************************/
// Convert LocConfigXtraReqMsg ->
// PBLocConfigEngineIntegrityRiskReqMsg
int LocConfigXtraReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigXtraReqMsg pbLocConfMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // bool enable = 1;
    pbLocConfMsg.set_enable(mEnable);
    // PBXtraConfigParams xtraParams = 2
    PBXtraConfigParams* pbXtraParams = pbLocConfMsg.mutable_xtraparams();
    if (nullptr != pbXtraParams) {
        if (pLocApiPbMsgConv->convertXtraConfigParamsToPB(mXtraParams, pbXtraParams)) {
            LOC_LOGe("convertXtraConfigParamsToPB failed");
            free(pbXtraParams);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_xtraparams failed");
        return 0;
    }

    string pbStr;
    if (!pbLocConfMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on PBLocConfigEngineIntegrityRiskReqMsg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigXtraReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }

    // free memory
    pLocApiPbMsgConv->freeUpPBLocConfigXtraReqMsg(pbLocConfMsg);
    return protoStr.size();
}

// Convert LocConfigGetXtraStatusReqMsg -> PBLocConfigGetXtraStatusReqMsg
int LocConfigGetXtraStatusReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // bytes       payload = 4;
    // LocConfigGetXtraStatusReqMsg - no struct member. No payload to send
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigGetXtraStatusReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Decode PBLocConfigXtraReqMsg -> LocConfigXtraReqMsg
LocConfigXtraReqMsg::LocConfigXtraReqMsg(
        const char* name, const PBLocConfigXtraReqMsg &pbLocConfMsg,
        const LocationApiPbMsgConv *pbMsgConv):
    LocAPIMsgHeader(name, E_INTAPI_CONFIG_XTRA_PARAMS_MSG_ID, pbMsgConv) {
    mEnable = pbLocConfMsg.enable();

    memset(&mXtraParams, 0, sizeof(mXtraParams));
    if (pbLocConfMsg.has_xtraparams()) {
        // >>>> PBLocConfigEngineRunStateReqMsg conversion
        pLocApiPbMsgConv->pbConvertToXtraConfig(pbLocConfMsg.xtraparams(), mXtraParams);
    }
}

int LocConfigGetXtraStatusRespMsg::serializeToProtobuf(string& protoStr) {

    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigGetXtraStatusRespMsg pbLocMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // XtraStatusUpdateType mUpdateType
    pbLocMsg.set_mupdatetype(pLocApiPbMsgConv->getPBEnumForXtraStatusUpdateType(mUpdateType));

    // PBXtraConfigParams xtraParams = 2
    PBXtraStatus* pbXtraStatus = pbLocMsg.mutable_mxtrastatus();
    if (nullptr != pbXtraStatus) {
        if (pLocApiPbMsgConv->convertXtraStatusToPB(mXtraStatus, pbXtraStatus)) {
            LOC_LOGe("convertXtraStatusToPB failed");
            free(pbXtraStatus);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_xtraparams failed");
        return 0;

    }

    string pbStr;
    if (!pbLocMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigGetXtraStatusRespMsg));
    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }

    // free memory
    pLocApiPbMsgConv->freeUpPBLocConfigGetXtraStatusRespMsg(pbLocMsg);
    return protoStr.size();
}

LocConfigGetXtraStatusRespMsg::LocConfigGetXtraStatusRespMsg(
        const char* name, const PBLocConfigGetXtraStatusRespMsg &pbMsg,
        const LocationApiPbMsgConv *pbMsgConv) :
    LocAPIMsgHeader(name, E_INTAPI_GET_XTRA_STATUS_RESP_MSG_ID, pbMsgConv) {

    mUpdateType = pLocApiPbMsgConv->getXtraStatusUpdateTypeFromPB(pbMsg.mupdatetype());
    if (pbMsg.has_mxtrastatus()) {
        pLocApiPbMsgConv->pbConvertToXtraStatus(pbMsg.mxtrastatus(), mXtraStatus);
    }
}

int LocConfigRegisterXtraStatusUpdateReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // bytes       payload = 4;
    // LocConfigRegisterXtraStatusUpdateReqMsg - no struct member. No payload to send
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigRegisterXtraStatusUpdateReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

int LocConfigDeregisterXtraStatusUpdateReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // bytes       payload = 4;
    // LocConfigRegisterXtraStatusUpdateReqMsg - no struct member. No payload to send
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigDeregisterXtraStatusUpdateReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocConfigGetRobustLocationConfigReqMsg -> PBLocConfigGetRobustLocationConfigReqMsg
int LocConfigGetRobustLocationConfigReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // bytes       payload = 4;
    // LocConfigGetRobustLocationConfigReqMsg - no struct member. No payload to send
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigGetRobustLocationConfigReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocConfigGetRobustLocationConfigRespMsg -> PBLocConfigGetRobustLocationConfigRespMsg
int LocConfigGetRobustLocationConfigRespMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigGetRobustLocationConfigRespMsg pbLocConfGetRobustLocConfg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigGetRobustLocationConfigRespMsg conversion
    // PBGnssConfigRobustLocation mRobustLoationConfig = 1;
    PBGnssConfigRobustLocation* gnssCfgRbstLoc =
            pbLocConfGetRobustLocConfg.mutable_mrobustloationconfig();
    if (nullptr != gnssCfgRbstLoc) {
        if (pLocApiPbMsgConv->convertGnssConfigRobustLocationToPB(mRobustLoationConfig,
                gnssCfgRbstLoc)) {
            LOC_LOGe("convertGnssConfigRobustLocationToPB failed");
            free(gnssCfgRbstLoc);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_mrobustloationconfig failed");
        return 0;
    }

    string pbStr;
    if (!pbLocConfGetRobustLocConfg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocConfGetRobustLocConfg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigGetRobustLocationConfigRespMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocConGetRbstLocCfgRespMsg(pbLocConfGetRobustLocConfg);
    return protoStr.size();
}

// Convert LocConfigGetMinGpsWeekReqMsg -> PBLocConfigGetMinGpsWeekReqMsg
int LocConfigGetMinGpsWeekReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // bytes       payload = 4;
    // LocConfigGetMinGpsWeekReqMsg - no struct member. No payload to send
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigGetMinGpsWeekReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocConfigGetMinGpsWeekRespMsg -> PBLocConfigGetMinGpsWeekRespMsg
int LocConfigGetMinGpsWeekRespMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigGetMinGpsWeekRespMsg pbLocConfGetMinGpsWeekRsp;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigGetMinGpsWeekRespMsg conversion
    // uint32 mMinGpsWeek = 1;
    pbLocConfGetMinGpsWeekRsp.set_mmingpsweek(mMinGpsWeek);

    string pbStr;
    if (!pbLocConfGetMinGpsWeekRsp.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocConfGetMinGpsWeekRsp failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigGetMinGpsWeekRespMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocConfigGetMinSvElevationReqMsg -> PBLocConfigGetMinSvElevationReqMsg
int LocConfigGetMinSvElevationReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // bytes       payload = 4;
    // LocConfigGetMinSvElevationReqMsg - no struct member. No payload to send
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigGetMinSvElevationReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocConfigGetMinSvElevationRespMsg -> PBLocConfigGetMinSvElevationRespMsg
int LocConfigGetMinSvElevationRespMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigGetMinSvElevationRespMsg pbLocConfGetMinSvElev;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigGetMinSvElevationRespMsg conversion
    // uint32 mMinSvElevation = 1;
    pbLocConfGetMinSvElev.set_mminsvelevation(mMinSvElevation);

    string pbStr;
    if (!pbLocConfGetMinSvElev.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocConfGetMinSvElev failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigGetMinSvElevationRespMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocConfigGetConstellationSecondaryBandConfigReqMsg to
// PBLocConfigGetConstellationSecondaryBandConfigReqMsg
int LocConfigGetConstellationSecondaryBandConfigReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // bytes       payload = 4;
    // LocConfigGetConstellationSecondaryBandConfigReqMsg - no struct member. No payload to send
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigGetConstellationSecondaryBandConfigReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}

// Convert LocConfigGetConstellationSecondaryBandConfigRespMsg to
// PBLocConfigGetConstltnSecondaryBandConfigRespMsg
int LocConfigGetConstellationSecondaryBandConfigRespMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigGetConstltnSecondaryBandConfigRespMsg pbLocCfgGetConstlSecBandRespMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocConfigGetConstltnSecondaryBandConfigRespMsg
    // PBGnssSvTypeConfig mSecondaryBandConfig = 1;
    PBGnssSvTypeConfig *gnssSvTypCfg =
            pbLocCfgGetConstlSecBandRespMsg.mutable_msecondarybandconfig();
    if (nullptr != gnssSvTypCfg) {
        if (pLocApiPbMsgConv->convertGnssSvTypeConfigToPB(mSecondaryBandConfig,
                gnssSvTypCfg)) {
            LOC_LOGe("convertGnssSvTypeConfigToPB failed");
            free(gnssSvTypCfg);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_msecondarybandconfig failed");
        return 0;
    }

    string pbStr;
    if (!pbLocCfgGetConstlSecBandRespMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocCfgGetConstlSecBandRespMsg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigGetConstellationSecondaryBandConfigRespMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocConfGetConstllSecBandCfgRespMsg(pbLocCfgGetConstlSecBandRespMsg);
    return protoStr.size();
}

// Convert LocAPIGetDebugReqMsg ->
// PBLocAPIGetDebugReqMsg
int LocAPIGetDebugReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // LocAPIGetDebugReqMsg - no struct member
    // bytes       payload = 4;
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIGetDebugReqMsg));
    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }

    return protoStr.size();
}

// Convert LocAPIGetDebugRespMsg ->
// PBLocAPIGetDebugRespMsg
int LocAPIGetDebugRespMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIGetDebugRespMsg pbMsg;
    PBLocAPIMsgHeader pLocApiMsgHdr;
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIGetDebugRespMsg conversion
    // PBGnssDebugReport mDebugReport = 1;
    PBGnssDebugReport* pbDebugReport = pbMsg.mutable_mdebugreport();
    if (nullptr != pbDebugReport) {
        if (pLocApiPbMsgConv->convertGnssDebugReportToPB(mDebugReport,
                pbDebugReport)) {
            LOC_LOGe("convertGnssDebugReportToPB failed");
            free(pbDebugReport);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_mdebugreport failed");
        return 0;
    }

    string pbStr;
    if (!pbMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIGetDebugRespMsg));
    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }

    // free memory
    pLocApiPbMsgConv->freeupPBLocAPIGetDebugRespMsg(pbMsg);
    return protoStr.size();
}

// Convert LocIntApiInjectLocationMsg -> PBLocIntApiInjectLocationMsg
int LocIntApiInjectLocationMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocIntApiInjectLocationMsg pbMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // PBLocation      location = 1;
    PBLocation* pbLocation = pbMsg.mutable_location();
    if (nullptr != pbLocation) {
        if (pLocApiPbMsgConv->convertLocationToPB(mLocation, pbLocation)) {
            LOC_LOGe("convertLocationToPB failed");
            pbMsg.clear_location();
            return 0;
        }
    } else {
        LOC_LOGe("mutable_location failed");
        return 0;
    }

    string pbStr;
    if (!pbMsg.SerializeToString(&pbStr)) {
        pbMsg.clear_location();
        LOC_LOGe("SerializeToString failed!");
        return 0;
    }

    pLocApiMsgHdr.set_payload(pbStr);
    pLocApiMsgHdr.set_payloadsize(sizeof(LocIntApiInjectLocationMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }

    pbMsg.clear_location();
    return protoStr.size();
}

// Convert LocConfigRegisterGnssSignalTypesUpdateReqMsg ->
// PBLocConfigRegisterGnssSignalTypesUpdateReqMsg
int LocConfigRegisterGnssSignalTypesUpdateReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigRegisterGnssSignalTypesUpdateReqMsg pbMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // bool mRegisterUpdate = 1;
    pbMsg.set_mregisterupdate(mRegisterUpdate);

    string pbStr;
    if (!pbMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigRegisterGnssSignalTypesUpdateReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}
// Convert LocConfigRegisterGnssSignalTypesUpdateRespMsg ->
// PBLocConfigRegisterGnssSignalTypesUpdateRespMsg
int LocConfigRegisterGnssSignalTypesUpdateRespMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocConfigRegisterGnssSignalTypesUpdateRespMsg pbMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    //GnssSignalTypeMask mSignalTypeMask = 1;
    pbMsg.set_msignaltypemask(pLocApiPbMsgConv->getPBMaskForGnssSignalTypeMask(mSignalTypeMask));

    string pbStr;
    if (!pbMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbMsg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocConfigRegisterGnssSignalTypesUpdateRespMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    return protoStr.size();
}


// Convert LocAPIGetAntennaInfoMsg ->
// PBLocAPIGetAntennaInfoMsg
int LocAPIGetAntennaInfoMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);
    // LocAPIGetAntennaInfoMsg - no struct member
    // bytes       payload = 4;
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIGetAntennaInfoMsg));
    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }

    return protoStr.size();
}

// Convert LocAPIAntennaInfoMsg ->
// PBLocAPIAntennaInfoMsg
int LocAPIAntennaInfoMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIAntennaInfoMsg pbMsg;
    PBLocAPIMsgHeader pLocApiMsgHdr;
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIAntennaInfoMsg conversion
    // PBAntennaInformation mAntennaInformation = 1
    PBAntennaInformation* pbAntennaInforamtion = pbMsg.mutable_mantennainformation();
    if (nullptr != pbAntennaInforamtion) {
        if (pLocApiPbMsgConv->convertAntennaInfoToPB(mAntennaInfo,
                pbAntennaInforamtion)) {
            LOC_LOGe("convertAntennaInfoToPB failed");
            free(pbAntennaInforamtion);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_mantennainformation failed");
        return 0;
    }

    string pbStr;
    if (!pbMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);
    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIAntennaInfoMsg));
    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeupPBAntennaInfoMsg(pbMsg);
    return protoStr.size();
}

// Convert LocAPIPingTestReqMsg -> PBLocAPIPingTestReqMsg
int LocAPIPingTestReqMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIPingTestReqMsg pbLocApiPingTest;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // uint32 numberOfPing = 1;
    pbLocApiPingTest.set_numberofping((uint32_t)LOCATION_REMOTE_API_PINGTEST_SIZE);
    // >>>> PBLocAPIPingTestReqMsg conversion
    // Max array len - LOCATION_REMOTE_API_PINGTEST_SIZE
    // repeated uint32 data = 2;
    for (int i = 0; i < LOCATION_REMOTE_API_PINGTEST_SIZE; i++) {
        pbLocApiPingTest.add_data(data[i]);
    }

    string pbStr;
    if (!pbLocApiPingTest.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiPingTest failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIPingTestReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIPingTestReqMsg(pbLocApiPingTest);
    return protoStr.size();
}

// Convert LocAPIPingTestIndMsg -> PBLocAPIPingTestIndMsg
int LocAPIPingTestIndMsg::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIPingTestIndMsg pbLocApiPingTestIndMsg;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // uint32 numberOfPing = 1;
    pbLocApiPingTestIndMsg.set_numberofping((uint32_t)LOCATION_REMOTE_API_PINGTEST_SIZE);
    // >>>> PBLocAPIPingTestIndMsg conversion
    // Max array len - LOCATION_REMOTE_API_PINGTEST_SIZE
    // repeated uint32 data = 1;
    for (int i = 0; i < LOCATION_REMOTE_API_PINGTEST_SIZE; i++) {
        pbLocApiPingTestIndMsg.add_data(data[i]);
    }

    string pbStr;
    if (!pbLocApiPingTestIndMsg.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiPingTestIndMsg failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIPingTestIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIPingTestIndMsg(pbLocApiPingTestIndMsg);
    return protoStr.size();
}

// Decode PBLocInjectMmfDataReqMsg -> LocInjectMmfDataReqMsg
LocInjectMmfDataReqMsg::LocInjectMmfDataReqMsg(const char* name,
            const PBLocInjectMmfDataReqMsg &pbLocApiMmfIndMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_MAP_MATCHED_FEEDBACK_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocInjectMmfDataReqReqMsg conversion
    // PBGnssMapMatchedData gnssMmfData = 1;
    pLocApiPbMsgConv->pbConvertToGnssMmfData(pbLocApiMmfIndMsg.gnssmmfdata(),
            gnssMapData);
}

// Decode LocInjectMmfDataReqMsg -> PBLocInjectMmfDataReqMsg
int LocInjectMmfDataReqMsg ::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocInjectMmfDataReqMsg pbLocApiMmfInd;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // PBGnssMapMatchedData gnssMmfData = 1;
    PBGnssMapMatchedData* mmfData =
            pbLocApiMmfInd.mutable_gnssmmfdata();
    if (nullptr != mmfData) {
        if (pLocApiPbMsgConv->convertGnssMmfDataToPB(gnssMapData,
                mmfData)) {
            LOC_LOGe("convertGnssMmfDataToPB failed");
            free(mmfData);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_gnssmmfdata failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiMmfInd.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiEphInd failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocInjectMmfDataReqMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocInjectMmfDataReqMsg(pbLocApiMmfInd);
    return protoStr.size();
}

// SERIALIZE PROTOBUF TO RIGID FORMAT
// **********************************
// Convert protobuf msg received to LocApiMsgHeader rigid structures. Protobuf msg received is
// deserialized and converted into local structures.

// Decode PBLocAPIClientRegisterReqMsg -> LocAPIClientRegisterReqMsg
LocAPIClientRegisterReqMsg::LocAPIClientRegisterReqMsg(const char* name,
            const PBLocAPIClientRegisterReqMsg &pbLocApiClientRegReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_CLIENT_REGISTER_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIClientRegisterReqMsg conversion
    // PBClientType mClientType = 1;
    mClientType = pLocApiPbMsgConv->getEnumForPBClientType(pbLocApiClientRegReqMsg.mclienttype());
}

// Decode PBLocAPICapabilitiesIndMsg -> LocAPICapabilitiesIndMsg
LocAPICapabilitiesIndMsg::LocAPICapabilitiesIndMsg(const char* name,
            const PBLocAPICapabilitiesIndMsg &pbLocApiCapInd,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_CAPABILILTIES_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPICapabilitiesIndMsg conversion
    // uint64 capabilitiesMask = 1;
    capabilitiesMask = pLocApiPbMsgConv->getLocationCapabilitiesMaskFromPB(
            pbLocApiCapInd.capabilitiesmask());
}

// Decode PBLocAPIGenericRespMsg -> LocAPIGenericRespMsg
LocAPIGenericRespMsg::LocAPIGenericRespMsg(const char* name, ELocMsgID msgId,
            const PBLocAPIGenericRespMsg &pbLocApiGenericRsp,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, msgId, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIGenericRespMsg conversion
    // PBLocationError err = 1;
    err = pLocApiPbMsgConv->getEnumForPBLocationError(pbLocApiGenericRsp.err());
}

// Decode PBLocAPICollectiveRespMsg -> LocAPICollectiveRespMsg
LocAPICollectiveRespMsg::LocAPICollectiveRespMsg(const char* name, ELocMsgID msgId,
        const PBLocAPICollectiveRespMsg &pbLocApiCollctvRespMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, msgId, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPICollectiveRespMsg conversion
    // PBCollectiveResPayload collectiveRes = 1;
    pLocApiPbMsgConv->pbConvertToCollectiveResPayload(pbLocApiCollctvRespMsg.collectiveres(),
            collectiveRes);
}

// Decode PBLocAPIStartTrackingReqMsg -> LocAPIStartTrackingReqMsg
LocAPIStartTrackingReqMsg::LocAPIStartTrackingReqMsg(const char* name,
            const PBLocAPIStartTrackingReqMsg &pbStartTrackReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_START_TRACKING_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIStartTrackingReqMsg conversion
    // PBLocationOptions locOptions = 1;
    pLocApiPbMsgConv->pbConvertToLocationOptions(pbStartTrackReqMsg.locoptions(), locOptions);
}

LocAPIStopTrackingReqMsg::LocAPIStopTrackingReqMsg(const char* name,
        const PBLocAPIStopTrackingReqMsg  &pbStopTrackingReqMsg,
        const LocationApiPbMsgConv *pbMsgConv):
    LocAPIMsgHeader(name, E_LOCAPI_STOP_TRACKING_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }

// >>>> PBLocAPIstopTrackingReqMsg conversion
    // bool clearSubscriptions = 1;
    clearSubscriptions = pbStopTrackingReqMsg.clearsubscriptions();
}

// Decode PBLocAPIUpdateCallbacksReqMsg -> LocAPIUpdateCallbacksReqMsg
LocAPIUpdateCallbacksReqMsg::LocAPIUpdateCallbacksReqMsg(const char* name,
            const PBLocAPIUpdateCallbacksReqMsg &pbLocApiUpdateCbsReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_UPDATE_CALLBACKS_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIUpdateCallbacksReqMsg conversion
    // bitwise OR of PBLocationCallbacksMask
    // uint32    locationCallbacks = 1;
    locationCallbacks = pLocApiPbMsgConv->getLocationCallbacksMaskFromPB(
            pbLocApiUpdateCbsReqMsg.locationcallbacks());
}

// Decode PBLocAPIUpdateTrackingOptionsReqMsg -> LocAPIUpdateTrackingOptionsReqMsg
LocAPIUpdateTrackingOptionsReqMsg::LocAPIUpdateTrackingOptionsReqMsg(const char* name,
            const PBLocAPIUpdateTrackingOptionsReqMsg &pbUpdateTrackOptReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_UPDATE_TRACKING_OPTIONS_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIUpdateTrackingOptionsReqMsg conversion
    // PBLocationOptions locOptions = 1;
    pLocApiPbMsgConv->pbConvertToLocationOptions(pbUpdateTrackOptReqMsg.locoptions(), locOptions);
}

// Decode PBLocAPIStartBatchingReqMsg -> LocAPIStartBatchingReqMsg
LocAPIStartBatchingReqMsg::LocAPIStartBatchingReqMsg(const char* name,
            const PBLocAPIStartBatchingReqMsg &pbStartBatchReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_START_BATCHING_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIStartBatchingReqMsg conversion
    // uint32 intervalInMs = 1;
    intervalInMs = pbStartBatchReqMsg.intervalinms();
    // uint32 distanceInMeters = 2;
    distanceInMeters = pbStartBatchReqMsg.distanceinmeters();
    // PBBatchingMode batchingMode = 3;
    batchingMode = pLocApiPbMsgConv->getEnumForPBBatchingMode(pbStartBatchReqMsg.batchingmode());
    LOC_LOGv("LocApiPB: Interval: %d, Distance(m): %d, Batch mode: %d", intervalInMs,
            distanceInMeters, batchingMode);
}

// Decode PBLocAPIUpdateBatchingOptionsReqMsg -> LocAPIUpdateBatchingOptionsReqMsg
LocAPIUpdateBatchingOptionsReqMsg::LocAPIUpdateBatchingOptionsReqMsg(const char* name,
            const PBLocAPIUpdateBatchingOptionsReqMsg &pbUpdateBatchOptiReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_UPDATE_BATCHING_OPTIONS_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIUpdateBatchingOptionsReqMsg conversion
    // uint32 intervalInMs = 1;
    intervalInMs = pbUpdateBatchOptiReqMsg.intervalinms();
    // uint32 distanceInMeters = 2;
    distanceInMeters = pbUpdateBatchOptiReqMsg.distanceinmeters();
    // PBBatchingMode batchingMode = 3;
    batchingMode = pLocApiPbMsgConv->getEnumForPBBatchingMode(
            pbUpdateBatchOptiReqMsg.batchingmode());
    LOC_LOGv("LocApiPB: Interval: %d, Distance(m): %d, Batch mode: %d", intervalInMs,
            distanceInMeters, batchingMode);
}

// Decode PBLocAPIAddGeofencesReqMsg -> LocAPIAddGeofencesReqMsg
LocAPIAddGeofencesReqMsg::LocAPIAddGeofencesReqMsg(const char* name,
            const PBLocAPIAddGeofencesReqMsg &pbAddGfsReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_ADD_GEOFENCES_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIAddGeofencesReqMsg conversion
    // PBGeofencesAddedReqPayload geofences = 1;
    pLocApiPbMsgConv->pbConvertToGfAddReqPayload(pbAddGfsReqMsg.geofences(), geofences);
}

// Decode PBLocAPIRemoveGeofencesReqMsg -> LocAPIRemoveGeofencesReqMsg
LocAPIRemoveGeofencesReqMsg::LocAPIRemoveGeofencesReqMsg(const char* name,
            const PBLocAPIRemoveGeofencesReqMsg &pbRemGfsReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_REMOVE_GEOFENCES_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIRemoveGeofencesReqMsg conversion
    // PBGeofencesReqClientIdPayload gfClientIds = 1;
    pLocApiPbMsgConv->pbConvertToGfReqClientIdPayload(pbRemGfsReqMsg.gfclientids(), gfClientIds);
}

// Decode PBLocAPIModifyGeofencesReqMsg -> LocAPIModifyGeofencesReqMsg
LocAPIModifyGeofencesReqMsg::LocAPIModifyGeofencesReqMsg(const char* name,
            const PBLocAPIModifyGeofencesReqMsg &pbModifyGfsReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_MODIFY_GEOFENCES_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIModifyGeofencesReqMsg conversion
    // PBGeofencesAddedReqPayload geofences = 1;
    pLocApiPbMsgConv->pbConvertToGfAddReqPayload(pbModifyGfsReqMsg.geofences(), geofences);
}

// Decode PBLocAPIPauseGeofencesReqMsg -> LocAPIPauseGeofencesReqMsg
LocAPIPauseGeofencesReqMsg::LocAPIPauseGeofencesReqMsg(const char* name,
            const PBLocAPIPauseGeofencesReqMsg &pbPauseGfsReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_PAUSE_GEOFENCES_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIPauseGeofencesReqMsg conversion
    // PBGeofencesReqClientIdPayload gfClientIds = 1;
    pLocApiPbMsgConv->pbConvertToGfReqClientIdPayload(pbPauseGfsReqMsg.gfclientids(), gfClientIds);
}

// Decode PBLocAPIResumeGeofencesReqMsg -> LocAPIResumeGeofencesReqMsg
LocAPIResumeGeofencesReqMsg::LocAPIResumeGeofencesReqMsg(const char* name,
            const PBLocAPIResumeGeofencesReqMsg &pbResumeGfsReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_RESUME_GEOFENCES_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIUpdateNetworkAvailabilityReqMsg conversion
    // PBGeofencesReqClientIdPayload gfClientIds = 1;
    pLocApiPbMsgConv->pbConvertToGfReqClientIdPayload(pbResumeGfsReqMsg.gfclientids(),
            gfClientIds);
}

// Decode PBLocAPIUpdateNetworkAvailabilityReqMsg -> LocAPIUpdateNetworkAvailabilityReqMsg
LocAPIUpdateNetworkAvailabilityReqMsg::LocAPIUpdateNetworkAvailabilityReqMsg(const char* name,
            const PBLocAPIUpdateNetworkAvailabilityReqMsg &pbUpdateNetAvailReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_CONTROL_UPDATE_NETWORK_AVAILABILITY_MSG_ID, pbMsgConv) {
    // >>>> PBLocAPIUpdateNetworkAvailabilityReqMsg conversion
    // bool mAvailability = 1;
    mAvailability = pbUpdateNetAvailReqMsg.mavailability();
}

// Decode PBLocAPILocationIndMsg -> LocAPILocationIndMsg
LocAPILocationIndMsg::LocAPILocationIndMsg(const char* name,
            const PBLocAPILocationIndMsg &pbLocApiLocIndMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_LOCATION_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPILocationIndMsg conversion
    // PBLocation locationNotification = 1;
    pLocApiPbMsgConv->pbConvertToLocation(pbLocApiLocIndMsg.locationnotification(),
            locationNotification);
}

// Decode PBLocAPIBatchingIndMsg -> LocAPIBatchingIndMsg
LocAPIBatchingIndMsg::LocAPIBatchingIndMsg(const char* name,
            const PBLocAPIBatchingIndMsg &pbLocApiBatchingIndMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_BATCHING_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIBatchingIndMsg conversion
    // PBLocAPIBatchNotification batchNotification = 1;
    pLocApiPbMsgConv->pbConvertToLocAPIBatchNotification(
            pbLocApiBatchingIndMsg.batchnotification(), batchNotification);
    // PBBatchingMode batchingMode = 2;
    batchingMode = pLocApiPbMsgConv->getEnumForPBBatchingMode(
            pbLocApiBatchingIndMsg.batchingmode());
}

// Decode PBLocAPIGeofenceBreachIndMsg -> LocAPIGeofenceBreachIndMsg
LocAPIGeofenceBreachIndMsg::LocAPIGeofenceBreachIndMsg(const char* name,
            const PBLocAPIGeofenceBreachIndMsg &pbLocApiGfBreachIndMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_GEOFENCE_BREACH_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIGeofenceBreachIndMsg conversion
    // PBLocAPIGeofenceBreachNotification gfBreachNotification = 1;
    pLocApiPbMsgConv->pbConvertToLocAPIGfBreachNotification(
            pbLocApiGfBreachIndMsg.gfbreachnotification(),
            gfBreachNotification);
}

// Decode PBLocAPILocationInfoIndMsg -> LocAPILocationInfoIndMsg
LocAPILocationInfoIndMsg::LocAPILocationInfoIndMsg(const char* name,
            const PBLocAPILocationInfoIndMsg &pbLocApiLocInfoIndMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_LOCATION_INFO_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPILocationInfoIndMsg conversion
    // PBGnssLocationInfoNotification gnssLocationInfoNotification = 1;
    pLocApiPbMsgConv->pbConvertToGnssLocInfoNotif(
            pbLocApiLocInfoIndMsg.gnsslocationinfonotification(),
            gnssLocationInfoNotification);
}

// Decode PBLocAPIEngineLocationsInfoIndMsg -> LocAPIEngineLocationsInfoIndMsg
LocAPIEngineLocationsInfoIndMsg::LocAPIEngineLocationsInfoIndMsg(const char* name,
            const PBLocAPIEngineLocationsInfoIndMsg &pbLocApiEngLocInfoIndMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_ENGINE_LOCATIONS_INFO_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIEngineLocationsInfoIndMsg conversion
    // max array size - PBLocApiOutputEngineType::PB_LOC_OUTPUT_ENGINE_COUNT
    // repeated PBGnssLocationInfoNotification engineLocationsInfo = 1;
    count = min((uint32_t)pbLocApiEngLocInfoIndMsg.enginelocationsinfo_size(),
            (uint32_t)LOC_OUTPUT_ENGINE_COUNT);
    for (uint32_t i = 0; i < count; i++) {
        pLocApiPbMsgConv->pbConvertToGnssLocInfoNotif(
                pbLocApiEngLocInfoIndMsg.enginelocationsinfo(i),
                engineLocationsInfo[i]);
    }
}

// Decode PBLocAPISatelliteVehicleIndMsg -> LocAPISatelliteVehicleIndMsg
LocAPISatelliteVehicleIndMsg::LocAPISatelliteVehicleIndMsg(const char* name,
            const PBLocAPISatelliteVehicleIndMsg &pbLocApiSatVehIndMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_SATELLITE_VEHICLE_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPISatelliteVehicleIndMsg conversion
    // PBLocApiGnssSvNotification gnssSvNotification = 1;
    pLocApiPbMsgConv->pbConvertToGnssSvNotif(pbLocApiSatVehIndMsg.gnsssvnotification(),
            gnssSvNotification);
}

// Decode PBLocAPINmeaIndMsg -> LocAPINmeaIndMsg
LocAPINmeaIndMsg::LocAPINmeaIndMsg(const char* name,
            const PBLocAPINmeaIndMsg &pbLocApiNmeaIndMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_NMEA_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPINmeaIndMsg conversion
    // PBLocAPINmeaSerializedPayload gnssNmeaNotification = 1;
    pLocApiPbMsgConv->pbConvertToLocAPINmeaSerializedPayload(
            pbLocApiNmeaIndMsg.gnssnmeanotification(),
            gnssNmeaNotification);
}

// Decode PBLocAPIDataIndMsg -> LocAPIDataIndMsg
LocAPIDataIndMsg::LocAPIDataIndMsg(const char* name,
            const PBLocAPIDataIndMsg &pbLocApiDataIndMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_DATA_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIDataIndMsg conversion
    // PBGnssDataNotification gnssDataNotification = 1;
    pLocApiPbMsgConv->pbConvertToGnssDataNotification(pbLocApiDataIndMsg.gnssdatanotification(),
            gnssDataNotification);
}

// Decode PBLocAPIMeasIndMsg -> LocAPIMeasIndMsg
LocAPIMeasIndMsg::LocAPIMeasIndMsg(const char* name,
            const PBLocAPIMeasIndMsg &pbLocApiMeasIndMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_MEAS_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIMeasIndMsg conversion
    // PBGnssMeasurementsNotification gnssMeasurementsNotification = 1;
    pLocApiPbMsgConv->pbConvertToGnssMeasNotification(
            pbLocApiMeasIndMsg.gnssmeasurementsnotification(),
            gnssMeasurementsNotification);
}

// Decode PBLocAPIGnssEnergyConsumedIndMsg -> LocAPIGnssEnergyConsumedIndMsg
LocAPIGnssEnergyConsumedIndMsg::LocAPIGnssEnergyConsumedIndMsg(const char* name,
            const PBLocAPIGnssEnergyConsumedIndMsg &pbLocApiGnssEnrgyConsmdIndMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_GET_GNSS_ENGERY_CONSUMED_MSG_ID, pbMsgConv) {
    // >>>> PBLocAPIGnssEnergyConsumedIndMsg conversion
    // uint64 totalGnssEnergyConsumedSinceFirstBoot = 1;
    totalGnssEnergyConsumedSinceFirstBoot =
            pbLocApiGnssEnrgyConsmdIndMsg.totalgnssenergyconsumedsincefirstboot();
    LOC_LOGd("LocApiPB: Total Gnss Energy Consumred: %" PRIu64,
            totalGnssEnergyConsumedSinceFirstBoot);
}

// Decode PBLocAPILocationSystemInfoIndMsg -> LocAPILocationSystemInfoIndMsg
LocAPILocationSystemInfoIndMsg::LocAPILocationSystemInfoIndMsg(const char* name,
            const PBLocAPILocationSystemInfoIndMsg &pbLocApiLocSysInfoIndMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_LOCATION_SYSTEM_INFO_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPILocationSystemInfoIndMsg conversion
    // PBLocationSystemInfo locationSystemInfo = 1;
    pLocApiPbMsgConv->pbConvertToLocationSystemInfo(pbLocApiLocSysInfoIndMsg.locationsysteminfo(),
            locationSystemInfo);
}

// Decode PBLocAPIDcReportIndMsg -> LocAPIDcReportIndMsg
LocAPIDcReportIndMsg::LocAPIDcReportIndMsg(const char* name,
            const PBLocAPIDcReportIndMsg &pbMsg, const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_DC_REPORT_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    pLocApiPbMsgConv->pbConvertToDcReport(pbMsg.dcreportinfo(), dcReportInfo);
}

// Decode PBLocConfigConstrainedTuncReqMsg -> LocConfigConstrainedTuncReqMsg
LocConfigConstrainedTuncReqMsg::LocConfigConstrainedTuncReqMsg(const char* name,
            const PBLocConfigConstrainedTuncReqMsg &pbConfigConstrTuncReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_CONSTRAINTED_TUNC_MSG_ID, pbMsgConv) {
    // >>>> PBLocConfigConstrainedTuncReqMsg conversion
    // bool     mEnable = 1;
    mEnable = pbConfigConstrTuncReqMsg.menable();
    // float    mTuncConstraint = 2;
    mTuncConstraint = pbConfigConstrTuncReqMsg.mtuncconstraint();
    // uint32   mEnergyBudget = 3;
    mEnergyBudget = pbConfigConstrTuncReqMsg.menergybudget();

    LOC_LOGd("LocApiPB: Constr Tunc Enable: %d, Tunc Constrnt: %f, EnergyBudget: %d", mEnable,
            mTuncConstraint, mEnergyBudget);
}

// Decode PBLocConfigPositionAssistedClockEstimatorReqMsg ->
//      LocConfigPositionAssistedClockEstimatorReqMsg
LocConfigPositionAssistedClockEstimatorReqMsg::LocConfigPositionAssistedClockEstimatorReqMsg(
            const char* name,
            const PBLocConfigPositionAssistedClockEstimatorReqMsg &pbLocConfPosAsstdClkEstReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
       LocAPIMsgHeader(name, E_INTAPI_CONFIG_POSITION_ASSISTED_CLOCK_ESTIMATOR_MSG_ID, pbMsgConv) {
    // >>>> PBLocConfigPositionAssistedClockEstimatorReqMsg conversion
    // bool     mEnable = 1;
    mEnable = pbLocConfPosAsstdClkEstReqMsg.menable();
    LOC_LOGd("LocApiPB: Clock Estimator Enable: %d", mEnable);
}

// Decode PBLocConfigSvConstellationReqMsg -> LocConfigSvConstellationReqMsg
LocConfigSvConstellationReqMsg::LocConfigSvConstellationReqMsg(const char* name,
            const PBLocConfigSvConstellationReqMsg &pbConfigSvConstReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_SV_CONSTELLATION_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocConfigSvConstellationReqMsg conversion
    // PBGnssSvTypeConfig mConstellationEnablementConfig = 1;
    pLocApiPbMsgConv->pbConvertToGnssSvTypeConfig(
            pbConfigSvConstReqMsg.mconstellationenablementconfig(),
            mConstellationEnablementConfig);
    // PBGnssSvIdConfig   mBlacklistSvConfig = 2;
    pLocApiPbMsgConv->pbConvertToGnssSvIdConfig(pbConfigSvConstReqMsg.mblacklistsvconfig(),
            mBlacklistSvConfig);
    // bool mResetToDefault = 3;
    if (pbConfigSvConstReqMsg.mresettodefault()) {
        // set size field in constellationEnablementConfig to 0 to indicate
        // to restore to modem default
        mConstellationEnablementConfig.size = 0;
    } else {
        mConstellationEnablementConfig.size = sizeof(GnssSvTypeConfig);
    }
    mBlacklistSvConfig.size = sizeof(GnssSvIdConfig);
}

// Decode PBLocConfigConstellationSecondaryBandReqMsg -> LocConfigConstellationSecondaryBandReqMsg
LocConfigConstellationSecondaryBandReqMsg::LocConfigConstellationSecondaryBandReqMsg(
            const char* name,
            const PBLocConfigConstellationSecondaryBandReqMsg &pbConfigConstSecBandReq,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_CONSTELLATION_SECONDARY_BAND_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocConfigConstellationSecondaryBandReqMsg {
    // PBGnssSvTypeConfig mSecondaryBandConfig = 1;
    pLocApiPbMsgConv->pbConvertToGnssSvTypeConfig(
            pbConfigConstSecBandReq.msecondarybandconfig(), mSecondaryBandConfig);
}

// Decode PBLocConfigAidingDataDeletionReqMsg -> LocConfigAidingDataDeletionReqMsg
LocConfigAidingDataDeletionReqMsg::LocConfigAidingDataDeletionReqMsg(const char* name,
            const PBLocConfigAidingDataDeletionReqMsg &pbConfigAidDataDelReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_AIDING_DATA_DELETION_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocConfigAidingDataDeletionReqMsg conversion
    // PBAidingData mAidingData = 1;
    pLocApiPbMsgConv->pbConvertToGnssAidingData(pbConfigAidDataDelReqMsg.maidingdata(),
            mAidingData);
}

// Decode PBLocConfigLeverArmReqMsg -> LocConfigLeverArmReqMsg
LocConfigLeverArmReqMsg::LocConfigLeverArmReqMsg(const char* name,
            const PBLocConfigLeverArmReqMsg &pbConfigLeverArmReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_LEVER_ARM_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocConfigLeverArmReqMsg conversion
    // PBLIALeverArmConfigInfo mLeverArmConfigInfo = 1;
    pLocApiPbMsgConv->pbConvertToLeverArmConfigInfo(pbConfigLeverArmReqMsg.mleverarmconfiginfo(),
            mLeverArmConfigInfo);
}

// Decode PBLocConfigRobustLocationReqMsg -> LocConfigRobustLocationReqMsg
LocConfigRobustLocationReqMsg::LocConfigRobustLocationReqMsg(const char* name,
            const PBLocConfigRobustLocationReqMsg &pbConfigRobustLocReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_ROBUST_LOCATION_MSG_ID, pbMsgConv) {
    // >>>> PBLocConfigRobustLocationReqMsg conversion
    // bool mEnable = 1;
    mEnable = pbConfigRobustLocReqMsg.menable();
    // bool mEnableForE911 = 2;
    mEnableForE911 = pbConfigRobustLocReqMsg.menablefore911();
    LOC_LOGd("LocApiPB: Robust Loc Config Enable: %d, Enable911: %d", mEnable, mEnableForE911);
}

// Decode PBLocConfigMinGpsWeekReqMsg -> LocConfigMinGpsWeekReqMsg
LocConfigMinGpsWeekReqMsg::LocConfigMinGpsWeekReqMsg(const char* name,
            const PBLocConfigMinGpsWeekReqMsg &pbConfigMinGpsWeekReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_MIN_GPS_WEEK_MSG_ID, pbMsgConv) {
    // >>>> PBLocConfigMinGpsWeekReqMsg conversion
    // uint32 mMinGpsWeek = 1;
    mMinGpsWeek = pbConfigMinGpsWeekReqMsg.mmingpsweek();
    LOC_LOGd("LocApiPB: Min Gps week: %u", mMinGpsWeek);
}

// Decode PBLocConfigDrEngineParamsReqMsg -> LocConfigDrEngineParamsReqMsg
LocConfigDrEngineParamsReqMsg::LocConfigDrEngineParamsReqMsg(const char* name,
            const PBLocConfigDrEngineParamsReqMsg &pbConfigDrEngineParamsReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_DEAD_RECKONING_ENGINE_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocConfigDrEngineParamsReqMsg conversion
    // PBDeadReckoningEngineConfig mDreConfig = 1;
    pLocApiPbMsgConv->pbConvertToDeadReckoningEngineConfig(
            pbConfigDrEngineParamsReqMsg.mdreconfig(), mDreConfig);
}

// Decode PBLocConfigMinSvElevationReqMsg -> LocConfigMinSvElevationReqMsg
LocConfigMinSvElevationReqMsg::LocConfigMinSvElevationReqMsg(const char* name,
            const PBLocConfigMinSvElevationReqMsg &pbConfigMinSvElevReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_MIN_SV_ELEVATION_MSG_ID, pbMsgConv) {
    // >>>> PBLocConfigMinSvElevationReqMsg conversion
    // uint32 mMinSvElevation = 1;
    mMinSvElevation = pbConfigMinSvElevReqMsg.mminsvelevation();
    LOC_LOGd("LocApiPB: MinSv Elev: %u", mMinSvElevation);
}

// Decode PBLocConfigEngineRunStateReqMsg -> LocConfigEngineRunStateReqMsg
LocConfigEngineRunStateReqMsg::LocConfigEngineRunStateReqMsg(const char* name,
            const PBLocConfigEngineRunStateReqMsg &pbConfigEngineRunStateReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_CONFIG_ENGINE_RUN_STATE_MSG_ID, pbMsgConv) {
    // >>>> PBLocConfigEngineRunStateReqMsg conversion
    mEngType = (PositioningEngineMask) pLocApiPbMsgConv->getEnumForPBPositioningEngineMask(
            pbConfigEngineRunStateReqMsg.mengtype());
    mEngState = (LocEngineRunState) pLocApiPbMsgConv->getEnumForPBLocEngineRunState(
            pbConfigEngineRunStateReqMsg.mengstate());
    LOC_LOGd("LocApiPB: eng type %d, eng state %d", mEngType, mEngState);
}

// Decode PBLocConfigMerkleTreeReqMsg -> LocConfigMerkleTreeReqMsg
LocConfigMerkleTreeReqMsg::LocConfigMerkleTreeReqMsg(
        const char* name, const PBLocConfigMerkleTreeReqMsg &pbLocConfMsg,
        const LocationApiPbMsgConv *pbMsgConv):
    LocAPIMsgHeader(name, E_INTAPI_CONFIG_MERKLE_TREE_MSG_ID, pbMsgConv) {
    mMerkleTreeConfig = pbLocConfMsg.mmerkletreeconfig();
    LOC_LOGd("LocApiPB: merkle tree path: %s, ", mMerkleTreeConfig.c_str());
}

// Decode PBLocConfigOsnmaEnablementReqMsg -> LocConfigOsnmaEnablementReqMsg
LocConfigOsnmaEnablementReqMsg::LocConfigOsnmaEnablementReqMsg(
        const char* name, const PBLocConfigOsnmaEnablementReqMsg &pbLocConfMsg,
        const LocationApiPbMsgConv *pbMsgConv):
    LocAPIMsgHeader(name, E_INTAPI_CONFIG_OSNMA_ENABLEMENT_MSG_ID, pbMsgConv) {
    mEnable = pbLocConfMsg.menable();
    LOC_LOGd("LocApiPB: osnma enablement: %d, ", mEnable);
}

// Decode PBLocConfigGetRobustLocationConfigRespMsg -> LocConfigGetRobustLocationConfigRespMsg
LocConfigGetRobustLocationConfigRespMsg::LocConfigGetRobustLocationConfigRespMsg(const char* name,
            const PBLocConfigGetRobustLocationConfigRespMsg &pbLocConfigGetRobustLocationRsp,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_GET_ROBUST_LOCATION_CONFIG_RESP_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocConfigGetRobustLocationConfigRespMsg conversion
    // PBGnssConfigRobustLocation mRobustLoationConfig = 1;
    pLocApiPbMsgConv->pbConvertToGnssConfigRobustLocation(
            pbLocConfigGetRobustLocationRsp.mrobustloationconfig(),
            mRobustLoationConfig);
}

// Decode PBLocConfigGetMinGpsWeekRespMsg -> LocConfigGetMinGpsWeekRespMsg
LocConfigGetMinGpsWeekRespMsg::LocConfigGetMinGpsWeekRespMsg(const char* name,
            const PBLocConfigGetMinGpsWeekRespMsg &pbLocConfigGetMinGpsWeekRsp,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_GET_MIN_GPS_WEEK_RESP_MSG_ID, pbMsgConv) {
    // >>>> PBLocConfigGetMinGpsWeekRespMsg conversion
    // uint32 mMinGpsWeek = 1;
    mMinGpsWeek = pbLocConfigGetMinGpsWeekRsp.mmingpsweek();
    LOC_LOGd("LocApiPB: mMinGpsWeek: %d", mMinGpsWeek);
}

// Decode PBLocConfigGetMinSvElevationRespMsg -> LocConfigGetMinSvElevationRespMsg
LocConfigGetMinSvElevationRespMsg::LocConfigGetMinSvElevationRespMsg(const char* name,
            const PBLocConfigGetMinSvElevationRespMsg &pbLocConfigGetMinSvElevRsp,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_GET_MIN_SV_ELEVATION_RESP_MSG_ID, pbMsgConv) {
    // >>>> PBLocConfigGetMinSvElevationRespMsg conversion
    // uint32 mMinSvElevation = 1;
    mMinSvElevation = pbLocConfigGetMinSvElevRsp.mminsvelevation();
    LOC_LOGd("LocApiPB: mMinSvElevation: %d", mMinSvElevation);
}

// Decode PBLocConfigGetConstltnSecondaryBandConfigRespMsg ->
// LocConfigGetConstellationSecondaryBandConfigRespMsg
LocConfigGetConstellationSecondaryBandConfigRespMsg::
            LocConfigGetConstellationSecondaryBandConfigRespMsg(const char* name,
            const PBLocConfigGetConstltnSecondaryBandConfigRespMsg &pbCfgGetConstSecBandCfgResp,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_INTAPI_GET_CONSTELLATION_SECONDARY_BAND_CONFIG_RESP_MSG_ID,
                pbMsgConv) {
    // >>>> PBLocConfigGetConstltnSecondaryBandConfigRespMsg {
    // PBGnssSvTypeConfig mSecondaryBandConfig = 1;
    pLocApiPbMsgConv->pbConvertToGnssSvTypeConfig(
            pbCfgGetConstSecBandCfgResp.msecondarybandconfig(), mSecondaryBandConfig);
}

// Decode PBLocIntApiInjectLocationMsg -> LocIntApiInjectLocationMsg
LocIntApiInjectLocationMsg::LocIntApiInjectLocationMsg(const char* name,
                               const PBLocIntApiInjectLocationMsg &pbMsg,
                               const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_INJECT_LOCATION_MSG_ID, pbMsgConv) {

    memset(&mLocation, 0, sizeof(mLocation));
    // PBLocation location = 1;
    if (pbMsg.has_location()) {
        if (pbMsgConv) {
            pbMsgConv->pbConvertToLocation(pbMsg.location(),
                                           mLocation);
        } else {
            LOC_LOGe("pb msg conv is null");
        }
    }
}

// Decode PBLocConfigRegisterGnssSignalTypesUpdateRespMsg ->
// LocConfigRegisterGnssSignalTypesUpdateRespMsg
LocConfigRegisterGnssSignalTypesUpdateRespMsg::LocConfigRegisterGnssSignalTypesUpdateRespMsg(
        const char* name, const PBLocConfigRegisterGnssSignalTypesUpdateRespMsg &pbMsg,
        const LocationApiPbMsgConv *pbMsgConv) :
        LocAPIMsgHeader(name, E_INTAPI_REGISTER_GNSS_SIGNAL_TYPES_UPDATE_RESP_MSG_ID, pbMsgConv) {
    mSignalTypeMask = pLocApiPbMsgConv->getPBMaskForGnssSignalTypeMask(pbMsg.msignaltypemask());
    LOC_LOGd("supported GNSS signal type: %x", mSignalTypeMask);
}

// Decode PBLocAPIGetDebugRespMsg ->
// LocAPIGetDebugRespMsg
LocAPIGetDebugRespMsg::LocAPIGetDebugRespMsg(const char* name,
        const PBLocAPIGetDebugRespMsg& pbMsg,
        const LocationApiPbMsgConv* pbMsgConv) :
    LocAPIMsgHeader(name, E_LOCAPI_GET_DEBUG_RESP_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }

    // >>>> PBLocAPIGetDebugRespMsg conversion {
    // GnssDebugReport mDebugReport = 1;
    pLocApiPbMsgConv->pbConvertToGnssDebugReport(
            pbMsg.mdebugreport(), mDebugReport);
}

// Decode PBLocAPIAntennaInfoMsg ->
// LocAPIAntennaInfoMsg
LocAPIAntennaInfoMsg::LocAPIAntennaInfoMsg(const char* name,
    const PBLocAPIAntennaInfoMsg& pbMsg,
    const LocationApiPbMsgConv* pbMsgConv) :
    LocAPIMsgHeader(name, E_LOCAPI_ANTENNA_INFO_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }

    // >>>> PBLocAPIAntennaInfoMsg conversion {
    // AntennaInformation mAntennaInfo = 1;
    pLocApiPbMsgConv->pbConvertToAntennaInfo(
        pbMsg.mantennainformation(), mAntennaInfo);
}

// Decode PBLocAPIPingTestReqMsg -> LocAPIPingTestReqMsg
LocAPIPingTestReqMsg::LocAPIPingTestReqMsg(const char* name,
            const PBLocAPIPingTestReqMsg &pbPingTestReqMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_PINGTEST_MSG_ID, pbMsgConv) {
    // >>>> PBLocAPIPingTestReqMsg conversion

    // uint32 numberOfPing = 1;
    uint32_t arrLen = min(pbPingTestReqMsg.numberofping(),
            (uint32_t)LOCATION_REMOTE_API_PINGTEST_SIZE);
    // Max array len - LOCATION_REMOTE_API_PINGTEST_SIZE
    // repeated uint32 data = 2;
    uint32_t i = 0;
    for (i = 0; i < arrLen; i++) {
        data[i] = pbPingTestReqMsg.data(i);
        LOC_LOGv("LocApiPB: LocAPIPingTestIndMsg data[%d]: %d", i, data[i]);
    }
}

// Decode PBLocAPIPingTestIndMsg -> LocAPIPingTestIndMsg
LocAPIPingTestIndMsg::LocAPIPingTestIndMsg(const char* name,
            const PBLocAPIPingTestIndMsg &pbLocApiPingTestIndMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_PINGTEST_MSG_ID, pbMsgConv) {
    // >>>> PBLocAPIPingTestIndMsg conversion
    // uint32 numberOfPing = 1;
    uint32_t arrLen = min(pbLocApiPingTestIndMsg.numberofping(),
            (uint32_t)LOCATION_REMOTE_API_PINGTEST_SIZE);
    // Max array len - LOCATION_REMOTE_API_PINGTEST_SIZE
    // repeated uint32 data = 2;
    uint32_t i = 0;
    for (i = 0; i < arrLen; i++) {
        data[i] = pbLocApiPingTestIndMsg.data(i);
        LOC_LOGv("LocApiPB: LocAPIPingTestIndMsg pingData[%d]: %d", i, data[i]);
    }
}

// Decode PBLocAPIEphIndMsg -> LocAPIEphIndMsg
LocAPIEphIndMsg::LocAPIEphIndMsg(const char* name,
            const PBLocAPIEphIndMsg &pbLocApiEphIndMsg,
            const LocationApiPbMsgConv *pbMsgConv):
        LocAPIMsgHeader(name, E_LOCAPI_EPH_MSG_ID, pbMsgConv) {
    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return;
    }
    // >>>> PBLocAPIEphIndMsg conversion
    // PBGnssEphemerisNotification gnssEphNotification = 1;
    pLocApiPbMsgConv->pbConvertToGnssEphNotif(
            pbLocApiEphIndMsg.gnssephemerisnotification(),
            gnssEphNotification);
}

// Decode LocAPIEphIndMsg -> PBLocAPIEphIndMsg
int LocAPIEphIndMsg ::serializeToProtobuf(string& protoStr) {
    PBLocAPIMsgHeader pLocApiMsgHdr;
    PBLocAPIEphIndMsg pbLocApiEphInd;

    if (nullptr == pLocApiPbMsgConv) {
        LOC_LOGe("pLocApiPbMsgConv is null!");
        return 0;
    }
    // string      mSocketName = 1;
    pLocApiMsgHdr.set_msocketname(mSocketName);
    // PBELocMsgID  msgId = 2;
    pLocApiMsgHdr.set_msgid(pLocApiPbMsgConv->getPBEnumForELocMsgID(msgId));
    // uint32   msgVersion = 3;
    pLocApiMsgHdr.set_msgversion(msgVersion);

    // >>>> PBLocAPIEphIndMsg conversion
    // PBGnssEphemerisNotification gnssEphNotification = 1;
    PBGnssEphemerisNotification* gnssEphNotif =
            pbLocApiEphInd.mutable_gnssephemerisnotification();
    if (nullptr != gnssEphNotif) {
        if (pLocApiPbMsgConv->convertGnssEphNotifToPB(gnssEphNotification,
                gnssEphNotif)) {
            LOC_LOGe("convertGnssEphNotifToPB failed");
            free(gnssEphNotif);
            return 0;
        }
    } else {
        LOC_LOGe("mutable_gnssephnotification failed");
        return 0;
    }

    string pbStr;
    if (!pbLocApiEphInd.SerializeToString(&pbStr)) {
        LOC_LOGe("SerializeToString on pbLocApiEphInd failed!");
        return 0;
    }
    // bytes       payload = 4;
    pLocApiMsgHdr.set_payload(pbStr);

    // uint32   payloadSize = 5;
    pLocApiMsgHdr.set_payloadsize(sizeof(LocAPIEphIndMsg));

    if (!pLocApiMsgHdr.SerializeToString(&protoStr)) {
        LOC_LOGe("SerializeToString on pLocApiMsgHdr failed!");
        return 0;
    }
    // free memory
    pLocApiPbMsgConv->freeUpPBLocAPIEphIndMsg(pbLocApiEphInd);
    return protoStr.size();
}
