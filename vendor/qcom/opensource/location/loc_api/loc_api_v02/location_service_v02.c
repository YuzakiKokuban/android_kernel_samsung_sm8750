/* Copyright (c) 2011-2021, The Linux Foundation. All rights reserved.
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
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
 *THIS IS AN AUTO GENERATED FILE. DO NOT ALTER IN ANY WAY
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/* This file was generated with Tool version 6.14.9
   It was generated on: Mon Jul  1 2024 (Spin 0)
   From IDL File: location_service_v02.idl */

#include "stdint.h"
#include "qmi_idl_lib_internal.h"
#include "location_service_v02.h"
#include "common_v01.h"


/*Type Definitions*/
static const uint8_t qmiLocApplicationIdStructT_data_v02[] = {
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocApplicationIdStructT_v02, applicationProvider),
  QMI_LOC_MAX_APP_ID_PROVIDER_LENGTH_V02,

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocApplicationIdStructT_v02, applicationName),
  QMI_LOC_MAX_APP_ID_NAME_LENGTH_V02,

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocApplicationIdStructT_v02, applicationVersion_valid),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocApplicationIdStructT_v02, applicationVersion),
  QMI_LOC_MAX_APP_ID_VERSION_LENGTH_V02,

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocPowerModeStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPowerModeStructT_v02, powerMode),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPowerModeStructT_v02, timeBetweenMeasurement),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocGPSTimeStructT_data_v02[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocGPSTimeStructT_v02, gpsWeek),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGPSTimeStructT_v02, gpsTimeOfWeekMs),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocDOPStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDOPStructT_v02, PDOP),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDOPStructT_v02, HDOP),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDOPStructT_v02, VDOP),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocExtDOPStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocExtDOPStructT_v02, PDOP),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocExtDOPStructT_v02, HDOP),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocExtDOPStructT_v02, VDOP),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocExtDOPStructT_v02, GDOP),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocExtDOPStructT_v02, TDOP),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocSensorUsageIndicatorStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSensorUsageIndicatorStructT_v02, usageMask),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSensorUsageIndicatorStructT_v02, aidingIndicatorMask),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocJammerIndicatorStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocJammerIndicatorStructT_v02, agcMetricDb),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocJammerIndicatorStructT_v02, bpMetricDb),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocJammerIndicatorExtStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocJammerIndicatorExtStructT_v02, gnssSignalType),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocJammerIndicatorExtStructT_v02, agcMetricDb),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocJammerIndicatorExtStructT_v02, bpMetricDb),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocSvInfoStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSvInfoStructT_v02, validMask),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSvInfoStructT_v02, system),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocSvInfoStructT_v02, gnssSvId),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSvInfoStructT_v02, healthStatus),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSvInfoStructT_v02, svStatus),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSvInfoStructT_v02, svInfoMask),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSvInfoStructT_v02, elevation),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSvInfoStructT_v02, azimuth),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSvInfoStructT_v02, snr),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocExpandedSvInfoStructT_data_v02[] = {
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocExpandedSvInfoStructT_v02, svInfo),
  QMI_IDL_TYPE88(0, 8),
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocExpandedSvInfoStructT_v02, gloFrequency),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocNiVxNotifyVerifyStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiVxNotifyVerifyStructT_v02, posQosIncl),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiVxNotifyVerifyStructT_v02, posQos),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiVxNotifyVerifyStructT_v02, numFixes),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiVxNotifyVerifyStructT_v02, timeBetweenFixes),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiVxNotifyVerifyStructT_v02, posMode),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiVxNotifyVerifyStructT_v02, encodingScheme),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiVxNotifyVerifyStructT_v02, requestorId),
  QMI_LOC_NI_MAX_REQUESTOR_ID_LENGTH_V02,
  QMI_IDL_OFFSET8(qmiLocNiVxNotifyVerifyStructT_v02, requestorId) - QMI_IDL_OFFSET8(qmiLocNiVxNotifyVerifyStructT_v02, requestorId_len),

  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiVxNotifyVerifyStructT_v02, userRespTimerInSeconds),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocNiSuplFormattedStringStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiSuplFormattedStringStructT_v02, formatType),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiSuplFormattedStringStructT_v02, formattedString),
  QMI_LOC_NI_MAX_CLIENT_NAME_LENGTH_V02,
  QMI_IDL_OFFSET8(qmiLocNiSuplFormattedStringStructT_v02, formattedString) - QMI_IDL_OFFSET8(qmiLocNiSuplFormattedStringStructT_v02, formattedString_len),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocNiSuplQopStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiSuplQopStructT_v02, validMask),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiSuplQopStructT_v02, horizontalAccuracy),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiSuplQopStructT_v02, verticalAccuracy),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiSuplQopStructT_v02, maxLocAge),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiSuplQopStructT_v02, delay),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocIpV4AddrStructType_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocIpV4AddrStructType_v02, addr),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocIpV4AddrStructType_v02, port),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocIpV6AddrStructType_data_v02[] = {
  QMI_IDL_FLAGS_IS_ARRAY |QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocIpV6AddrStructType_v02, addr),
  QMI_LOC_IPV6_ADDR_LENGTH_V02,

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocIpV6AddrStructType_v02, port),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocNiSuplServerInfoStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiSuplServerInfoStructT_v02, suplServerAddrTypeMask),

  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocNiSuplServerInfoStructT_v02, ipv4Addr),
  QMI_IDL_TYPE88(0, 13),
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocNiSuplServerInfoStructT_v02, ipv6Addr),
  QMI_IDL_TYPE88(0, 14),
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocNiSuplServerInfoStructT_v02, urlAddr),
  QMI_LOC_MAX_SERVER_ADDR_LENGTH_V02,

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocNiSuplNotifyVerifyStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiSuplNotifyVerifyStructT_v02, valid_flags),

  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocNiSuplNotifyVerifyStructT_v02, suplServerInfo),
  QMI_IDL_TYPE88(0, 15),
  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_FLAGS_IS_ARRAY |QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiSuplNotifyVerifyStructT_v02, suplSessionId),
  QMI_LOC_NI_SUPL_SLP_SESSION_ID_BYTE_LENGTH_V02,

  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_FLAGS_IS_ARRAY |QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiSuplNotifyVerifyStructT_v02, suplHash),
  QMI_LOC_NI_SUPL_HASH_LENGTH_V02,

  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiSuplNotifyVerifyStructT_v02, posMethod),

  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiSuplNotifyVerifyStructT_v02, dataCodingScheme),

  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiSuplNotifyVerifyStructT_v02, requestorId),
  QMI_IDL_TYPE88(0, 11),
  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiSuplNotifyVerifyStructT_v02, clientName),
  QMI_IDL_TYPE88(0, 11),
  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiSuplNotifyVerifyStructT_v02, suplQop),
  QMI_IDL_TYPE88(0, 12),
  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiSuplNotifyVerifyStructT_v02, userResponseTimer),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocNiUmtsCpCodedStringStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpCodedStringStructT_v02, dataCodingScheme),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpCodedStringStructT_v02, codedString),
  QMI_LOC_NI_CODEWORD_MAX_LENGTH_V02,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpCodedStringStructT_v02, codedString) - QMI_IDL_OFFSET8(qmiLocNiUmtsCpCodedStringStructT_v02, codedString_len),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocNiUmtsCpNotifyVerifyStructT_data_v02[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpNotifyVerifyStructT_v02, valid_flags),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpNotifyVerifyStructT_v02, invokeId),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpNotifyVerifyStructT_v02, dataCodingScheme),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpNotifyVerifyStructT_v02, notificationText),
  QMI_LOC_NI_MAX_CLIENT_NAME_LENGTH_V02,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpNotifyVerifyStructT_v02, notificationText) - QMI_IDL_OFFSET8(qmiLocNiUmtsCpNotifyVerifyStructT_v02, notificationText_len),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpNotifyVerifyStructT_v02, clientAddress),
  QMI_LOC_NI_MAX_EXT_CLIENT_ADDRESS_V02,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpNotifyVerifyStructT_v02, clientAddress) - QMI_IDL_OFFSET8(qmiLocNiUmtsCpNotifyVerifyStructT_v02, clientAddress_len),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpNotifyVerifyStructT_v02, locationType),

  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpNotifyVerifyStructT_v02, requestorId),
  QMI_IDL_TYPE88(0, 17),
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpNotifyVerifyStructT_v02, codewordString),
  QMI_IDL_TYPE88(0, 17),
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpNotifyVerifyStructT_v02, lcsServiceTypeId),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiUmtsCpNotifyVerifyStructT_v02, userResponseTimer),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocNiVxServiceInteractionStructT_data_v02[] = {
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocNiVxServiceInteractionStructT_v02, niVxReq),
  QMI_IDL_TYPE88(0, 10),
  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiVxServiceInteractionStructT_v02, serviceInteractionType),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocNiSuplVer2ExtStructT_data_v02[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiSuplVer2ExtStructT_v02, supportedNetworksMask),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiSuplVer2ExtStructT_v02, triggerType),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiSuplVer2ExtStructT_v02, gnssType),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocEmergencyNotificationStructT_data_v02[] = {
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocEmergencyNotificationStructT_v02, eslpUrl),
  QMI_LOC_MAX_SERVER_ADDR_LENGTH_V02,

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocAssistanceServerUrlStructT_data_v02[] = {
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocAssistanceServerUrlStructT_v02, serverUrl),
  QMI_LOC_MAX_SERVER_ADDR_LENGTH_V02,

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocTimeServerListStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocTimeServerListStructT_v02, delayThreshold),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocTimeServerListStructT_v02, timeServerList),
  QMI_LOC_MAX_NTP_SERVERS_V02,
  QMI_IDL_OFFSET8(qmiLocTimeServerListStructT_v02, timeServerList) - QMI_IDL_OFFSET8(qmiLocTimeServerListStructT_v02, timeServerList_len),
  QMI_IDL_TYPE88(0, 22),
  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocPredictedOrbitsAllowedSizesStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPredictedOrbitsAllowedSizesStructT_v02, maxFileSizeInBytes),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPredictedOrbitsAllowedSizesStructT_v02, maxPartSize),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocPredictedOrbitsServerListStructT_data_v02[] = {
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocPredictedOrbitsServerListStructT_v02, serverList),
  QMI_LOC_MAX_PREDICTED_ORBITS_SERVERS_V02,
  QMI_IDL_OFFSET8(qmiLocPredictedOrbitsServerListStructT_v02, serverList) - QMI_IDL_OFFSET8(qmiLocPredictedOrbitsServerListStructT_v02, serverList_len),
  QMI_IDL_TYPE88(0, 22),
  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocPredictedOrbitsSpecialFileTypeStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPredictedOrbitsSpecialFileTypeStructT_v02, fileType),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPredictedOrbitsSpecialFileTypeStructT_v02, downloadInterval),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocSensorControlConfigSamplingSpecStructT_data_v02[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocSensorControlConfigSamplingSpecStructT_v02, samplesPerBatch),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocSensorControlConfigSamplingSpecStructT_v02, batchesPerSecond),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocSensorReadyStatusStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSensorReadyStatusStructT_v02, injectEnable),

  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocSensorReadyStatusStructT_v02, dataFrequency),
  QMI_IDL_TYPE88(0, 27),
  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocGeofencePositionStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, timestampUtc),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, latitude),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, longitude),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, horUncEllipseSemiMinor),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, horUncEllipseSemiMajor),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, horUncEllipseOrientAzimuth),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, speedHorizontal_valid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, speedHorizontal),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, altitudeWrtEllipsoid_valid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, altitudeWrtEllipsoid),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, vertUnc_valid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, vertUnc),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, speedVertical_valid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, speedVertical),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, heading_valid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofencePositionStructT_v02, heading),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocGeofenceIdContinuousStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofenceIdContinuousStructT_v02, idLow),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofenceIdContinuousStructT_v02, idHigh),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocPredictedOrbitsDataValidityStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocPredictedOrbitsDataValidityStructT_v02, startTimeInUTC),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocPredictedOrbitsDataValidityStructT_v02, durationHours),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocAltitudeSrcInfoStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAltitudeSrcInfoStructT_v02, source),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAltitudeSrcInfoStructT_v02, linkage),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAltitudeSrcInfoStructT_v02, coverage),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocDeleteSvInfoStructT_data_v02[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteSvInfoStructT_v02, gnssSvId),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteSvInfoStructT_v02, system),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteSvInfoStructT_v02, deleteSvInfoMask),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocDeleteBDSSvInfoStructT_data_v02[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteBDSSvInfoStructT_v02, gnssSvId),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteBDSSvInfoStructT_v02, deleteSvInfoMask),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocDeleteGALSvInfoStructT_data_v02[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGALSvInfoStructT_v02, gnssSvId),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGALSvInfoStructT_v02, deleteSvInfoMask),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocWifiFixTimeStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiFixTimeStructT_v02, wifiPositionTime),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocWifiFixPosStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiFixPosStructT_v02, lat),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiFixPosStructT_v02, lon),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiFixPosStructT_v02, hepe),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiFixPosStructT_v02, numApsUsed),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiFixPosStructT_v02, fixErrorCode),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocWifiApInfoStructT_data_v02[] = {
  QMI_IDL_FLAGS_IS_ARRAY |QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApInfoStructT_v02, macAddr),
  QMI_LOC_WIFI_MAC_ADDR_LENGTH_V02,

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApInfoStructT_v02, rssi),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApInfoStructT_v02, channel),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApInfoStructT_v02, apQualifier),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocWifiApSsidStructT_data_v02[] = {
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocWifiApSsidStructT_v02, ssid),
  QMI_LOC_MAX_WIFI_AP_SSID_STR_LENGTH_V02,

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLoc3AxisSensorSampleStructT_data_v02[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLoc3AxisSensorSampleStructT_v02, timeOffset),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLoc3AxisSensorSampleStructT_v02, xAxis),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLoc3AxisSensorSampleStructT_v02, yAxis),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLoc3AxisSensorSampleStructT_v02, zAxis),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLoc3AxisSensorSampleListStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLoc3AxisSensorSampleListStructT_v02, timeOfFirstSample),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLoc3AxisSensorSampleListStructT_v02, flags),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLoc3AxisSensorSampleListStructT_v02, sensorData),
  QMI_LOC_SENSOR_DATA_MAX_SAMPLES_V02,
  QMI_IDL_OFFSET8(qmiLoc3AxisSensorSampleListStructT_v02, sensorData) - QMI_IDL_OFFSET8(qmiLoc3AxisSensorSampleListStructT_v02, sensorData_len),
  QMI_IDL_TYPE88(0, 40),
  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocSensorTemperatureSampleStructT_data_v02[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocSensorTemperatureSampleStructT_v02, timeOffset),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSensorTemperatureSampleStructT_v02, temperature),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocSensorTemperatureSampleListStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSensorTemperatureSampleListStructT_v02, timeSource),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSensorTemperatureSampleListStructT_v02, timeOfFirstSample),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocSensorTemperatureSampleListStructT_v02, temperatureData),
  QMI_LOC_SENSOR_DATA_MAX_SAMPLES_V02,
  QMI_IDL_OFFSET8(qmiLocSensorTemperatureSampleListStructT_v02, temperatureData) - QMI_IDL_OFFSET8(qmiLocSensorTemperatureSampleListStructT_v02, temperatureData_len),
  QMI_IDL_TYPE88(0, 42),
  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocApnProfilesStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocApnProfilesStructT_v02, pdnType),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocApnProfilesStructT_v02, apnName),
  QMI_LOC_MAX_APN_NAME_LENGTH_V02,

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocCircularGeofenceArgsStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocCircularGeofenceArgsStructT_v02, latitude),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocCircularGeofenceArgsStructT_v02, longitude),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocCircularGeofenceArgsStructT_v02, radius),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocGeofenceMotionStateConfigStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofenceMotionStateConfigStructT_v02, motionState),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGeofenceMotionStateConfigStructT_v02, motionStateSpeed),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocTimeZoneStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocTimeZoneStructT_v02, dstOffset),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocTimeZoneStructT_v02, rawOffset),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocMotionDataStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocMotionDataStructT_v02, motion_state),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocMotionDataStructT_v02, motion_mode),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocMotionDataStructT_v02, probability_of_state),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocMotionDataStructT_v02, age),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocMotionDataStructT_v02, timeout),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocGSMCellIdStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGSMCellIdStructT_v02, MCC),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGSMCellIdStructT_v02, MNC),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGSMCellIdStructT_v02, LAC),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGSMCellIdStructT_v02, CID),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocWCDMACellIdStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWCDMACellIdStructT_v02, mcc),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWCDMACellIdStructT_v02, mnc),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWCDMACellIdStructT_v02, cid),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocTDSCDMACellIdStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocTDSCDMACellIdStructT_v02, mcc),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocTDSCDMACellIdStructT_v02, mnc),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocTDSCDMACellIdStructT_v02, cid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocTDSCDMACellIdStructT_v02, lac),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocBatchedReportStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, fixId),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, validFields),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, latitude),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, longitude),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, horUncCircular),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, speedHorizontal),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, speedUnc),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, altitudeWrtEllipsoid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, speedVertical),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, heading),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, headingUnc),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, technologyMask),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, timestampUtc),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, timeUnc),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, magneticDeviation),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, vertUnc),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, horConfidence),

  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocBatchedReportStructT_v02, gpsTime),
  QMI_IDL_TYPE88(0, 2),
  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocWifiApDataStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApDataStructT_v02, wifiApDataMask),

  QMI_IDL_FLAGS_IS_ARRAY |QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApDataStructT_v02, macAddress),
  QMI_LOC_WIFI_MAC_ADDR_LENGTH_V02,

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApDataStructT_v02, apTransmitPower),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApDataStructT_v02, apAntennaGain),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApDataStructT_v02, apSignalToNoise),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApDataStructT_v02, apDeviceType),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApDataStructT_v02, apRssi),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApDataStructT_v02, apChannel),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApDataStructT_v02, apRoundTripDelay),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApDataStructT_v02, apRoundTripDelayUnit),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApDataStructT_v02, apRoundTripDelayAccuracy),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApDataStructT_v02, mobileSignalToNoise),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApDataStructT_v02, mobileRssi),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocWifiApAdditionalDataStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApAdditionalDataStructT_v02, rssiTimestamp),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApAdditionalDataStructT_v02, measAge),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApAdditionalDataStructT_v02, servingAccessPoint),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApAdditionalDataStructT_v02, channelFrequency),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocWifiApAdditionalDataStructT_v02, ssid),
  QMI_LOC_MAX_WIFI_AP_SSID_STR_LENGTH_V02,

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApAdditionalDataStructT_v02, apHighResolutionRssi),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocVehicleSensorSampleStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocVehicleSensorSampleStructT_v02, timeOffset),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocVehicleSensorSampleStructT_v02, axisSample),
  QMI_LOC_VEHICLE_SENSOR_DATA_MAX_AXES_V02,
  QMI_IDL_OFFSET8(qmiLocVehicleSensorSampleStructT_v02, axisSample) - QMI_IDL_OFFSET8(qmiLocVehicleSensorSampleStructT_v02, axisSample_len),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocVehicleSensorSampleListStructType_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocVehicleSensorSampleListStructType_v02, sampleTimeBase),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocVehicleSensorSampleListStructType_v02, axesValidity),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocVehicleSensorSampleListStructType_v02, sensorData),
  QMI_LOC_VEHICLE_SENSOR_DATA_MAX_SAMPLES_V02,
  QMI_IDL_OFFSET8(qmiLocVehicleSensorSampleListStructType_v02, sensorData) - QMI_IDL_OFFSET8(qmiLocVehicleSensorSampleListStructType_v02, sensorData_len),
  QMI_IDL_TYPE88(0, 55),
  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocVehicleOdometrySampleStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocVehicleOdometrySampleStructT_v02, timeOffset),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocVehicleOdometrySampleStructT_v02, distanceTravelled),
  QMI_LOC_VEHICLE_ODOMETRY_MAX_MEASUREMENTS_V02,
  QMI_IDL_OFFSET8(qmiLocVehicleOdometrySampleStructT_v02, distanceTravelled) - QMI_IDL_OFFSET8(qmiLocVehicleOdometrySampleStructT_v02, distanceTravelled_len),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocVehicleOdometrySampleListStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocVehicleOdometrySampleListStructT_v02, sampleTimeBase),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocVehicleOdometrySampleListStructT_v02, flags),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocVehicleOdometrySampleListStructT_v02, wheelFlags),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocVehicleOdometrySampleListStructT_v02, distanceTravelledBase),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocVehicleOdometrySampleListStructT_v02, odometryData),
  QMI_LOC_VEHICLE_SENSOR_DATA_MAX_SAMPLES_V02,
  QMI_IDL_OFFSET8(qmiLocVehicleOdometrySampleListStructT_v02, odometryData) - QMI_IDL_OFFSET8(qmiLocVehicleOdometrySampleListStructT_v02, odometryData_len),
  QMI_IDL_TYPE88(0, 57),
  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocRcvrClockFrequencyInfoStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocRcvrClockFrequencyInfoStructT_v02, clockDrift),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocRcvrClockFrequencyInfoStructT_v02, clockDriftUnc),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocRcvrClockFrequencyInfoStructT_v02, sourceOfFreq),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocLeapSecondInfoStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocLeapSecondInfoStructT_v02, leapSec),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocLeapSecondInfoStructT_v02, leapSecUnc),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocInterSystemBiasStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInterSystemBiasStructT_v02, validMask),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInterSystemBiasStructT_v02, timeBias),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInterSystemBiasStructT_v02, timeBiasUnc),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocGnssTimeStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssTimeStructT_v02, system),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssTimeStructT_v02, systemWeek),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssTimeStructT_v02, systemMsec),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssTimeStructT_v02, systemClkTimeBias),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssTimeStructT_v02, systemClkTimeUncMs),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocGloTimeStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloTimeStructT_v02, gloFourYear),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloTimeStructT_v02, gloDays),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloTimeStructT_v02, gloMsec),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloTimeStructT_v02, gloClkTimeBias),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloTimeStructT_v02, gloClkTimeUncMs),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocGnssTimeExtStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssTimeExtStructT_v02, refFCount),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssTimeExtStructT_v02, systemRtc_valid),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssTimeExtStructT_v02, systemRtcMs),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssTimeExtStructT_v02, sourceOfTime),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocSVTimeSpeedStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVTimeSpeedStructT_v02, svTimeMs),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVTimeSpeedStructT_v02, svTimeSubMs),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVTimeSpeedStructT_v02, svTimeUncMs),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVTimeSpeedStructT_v02, dopplerShift),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVTimeSpeedStructT_v02, dopplerShiftUnc),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVTimeSpeedStructT_v02, dopplerAccel_valid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVTimeSpeedStructT_v02, dopplerAccel),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocSVMeasurementStructT_data_v02[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, gnssSvId),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, gloFrequency),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, svStatus),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, validMask),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, healthStatus),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, svInfoMask),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, validMeasStatusMask),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, measurementStatus),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, CNo),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, gloRfLoss),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, measLatency),

  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, svTimeSpeed),
  QMI_IDL_TYPE88(0, 65),
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, lossOfLock),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, multipathEstimate),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, fineSpeed),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, fineSpeedUnc),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, carrierPhase),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, cycleSlipCount),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, svAzimuth),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSVMeasurementStructT_v02, svElevation),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocDgnssSVMeasurementStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocDgnssSVMeasurementStructT_v02, dgnssMeasStatus),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDgnssSVMeasurementStructT_v02, diffDataEpochTimeMsec),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDgnssSVMeasurementStructT_v02, prCorrMeters),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDgnssSVMeasurementStructT_v02, prrCorrMetersPerSec),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocWifiApMacAddressStructT_data_v02[] = {
  QMI_IDL_FLAGS_IS_ARRAY |QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiApMacAddressStructT_v02, wifiApMacAddress),
  QMI_LOC_WIFI_MAC_ADDR_LENGTH_V02,

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocIBeaconIdStructT_data_v02[] = {
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocIBeaconIdStructT_v02, uuid),
  QMI_LOC_MAX_IBEACON_UUID_STR_LENGTH_V02,

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocIBeaconIdStructT_v02, majorNumber),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocIBeaconIdStructT_v02, minorNumber),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocGtpAsnVerStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGtpAsnVerStructT_v02, asnMajorVersion),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGtpAsnVerStructT_v02, asnMinorVersion),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGtpAsnVerStructT_v02, asnPointVersion),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocDbtPositionStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, timestampUtc),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, latitude),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, longitude),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, horUncEllipseSemiMinor),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, horUncEllipseSemiMajor),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, horUncEllipseOrientAzimuth),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, speedHorizontal_valid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, speedHorizontal),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, altitudeWrtEllipsoid_valid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, altitudeWrtEllipsoid),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, vertUnc_valid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, vertUnc),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, speedVertical_valid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, speedVertical),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, heading_valid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDbtPositionStructT_v02, heading),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocApCacheStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocApCacheStructT_v02, macAddress),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocApCacheStructT_v02, xLat),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocApCacheStructT_v02, yLon),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocApCacheStructT_v02, mar),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocApCacheHepeRelStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocApCacheHepeRelStructT_v02, hepeValid),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocApCacheHepeRelStructT_v02, hepe),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocApCacheHepeRelStructT_v02, apReliability),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocApDoNotCacheStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocApDoNotCacheStructT_v02, macAddress),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocDeleteSatelliteDataStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteSatelliteDataStructT_v02, system),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteSatelliteDataStructT_v02, deleteSatelliteDataMask),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocInternalStatusReportDataStructT_data_v02[] = {
  QMI_IDL_FLAGS_IS_ARRAY |QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInternalStatusReportDataStructT_v02, data),
  QMI_LOC_INTERNAL_STATUS_REPORT_DATA_LENGTH_V02,

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocSrnBtleApDeviceDataStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSrnBtleApDeviceDataStructT_v02, srnApDataMask),

  QMI_IDL_FLAGS_IS_ARRAY |QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSrnBtleApDeviceDataStructT_v02, macAddress),
  QMI_LOC_SRN_MAC_ADDR_LENGTH_V02,

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSrnBtleApDeviceDataStructT_v02, apSrnRssi),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSrnBtleApDeviceDataStructT_v02, apSrnTimestamp),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocWifiCrowdSourcingLocalConfigStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiCrowdSourcingLocalConfigStructT_v02, enableCrowdSourcingOnDemand),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiCrowdSourcingLocalConfigStructT_v02, enableCrowdSourcingOnUnsolicitedGnss),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiCrowdSourcingLocalConfigStructT_v02, enableBatteryLevelBasedThrottling),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiCrowdSourcingLocalConfigStructT_v02, enableRttCrowdSourcing),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiCrowdSourcingLocalConfigStructT_v02, enableRtt3CrowdSourcing),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiCrowdSourcingLocalConfigStructT_v02, rttSignalDbmThresh),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiCrowdSourcingLocalConfigStructT_v02, maxDataTransferFormatVersionSupported),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocWifiCrowdSourcingCapabilityStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiCrowdSourcingCapabilityStructT_v02, onDemandCrowdSourcingSupported),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiCrowdSourcingCapabilityStructT_v02, UnsolicitedGnssCrowdSourcingSupported),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiCrowdSourcingCapabilityStructT_v02, majorVersionSupported),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiCrowdSourcingCapabilityStructT_v02, minorVersionSupported),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocWifiCrowdSourcingCapabilityStructT_v02, maxDataTransferFormatVersionSupported),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocXtraInfoStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocXtraInfoStructT_v02, reportMask),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocXtraInfoStructT_v02, absAgeHrs),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocXtraInfoStructT_v02, relAgeInUTC),

  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocXtraInfoStructT_v02, xtraServerInfo),
  QMI_IDL_TYPE88(0, 25),
  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocXtraInfoStructT_v02, ntpServerInfo),
  QMI_IDL_TYPE88(0, 23),
  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocXtraInfoStructT_v02, timeRequest),

  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocXtraInfoStructT_v02, preferedValidAgeHrs),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocVersionStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocVersionStructT_v02, majorVersion),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocVersionStructT_v02, minorVersion),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocGtpClientSwVerStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGtpClientSwVerStructT_v02, gtpClientSwMajorVersion),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGtpClientSwVerStructT_v02, gtpClientSwMinorVersion),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocHomeCarrierIdStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocHomeCarrierIdStructT_v02, homeMncValid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocHomeCarrierIdStructT_v02, homeMcc),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocHomeCarrierIdStructT_v02, homeMnc),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocPositionSourceStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocPositionSourceStructT_v02, unspecifiedInternal),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocPositionSourceStructT_v02, unspecifiedExternal),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocPositionSourceStructT_v02, gnss),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocPositionSourceStructT_v02, sensors),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocPositionSourceStructT_v02, wifi),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocPositionSourceStructT_v02, wwan),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocBsBestPositionStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocBsBestPositionStructT_v02, latitude),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocBsBestPositionStructT_v02, longitude),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBsBestPositionStructT_v02, horizontalUnc),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBsBestPositionStructT_v02, altitude),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBsBestPositionStructT_v02, altitudeUnc),

  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocBsBestPositionStructT_v02, latestPositionSources),
  QMI_IDL_TYPE88(0, 84),
  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocFdclCellIdStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellIdStructT_v02, airInterfaceType),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellIdStructT_v02, id1),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellIdStructT_v02, id2),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellIdStructT_v02, id3),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellIdStructT_v02, id4),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocFdclCellPosStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellPosStructT_v02, validMask),

  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocFdclCellPosStructT_v02, cellId),
  QMI_IDL_TYPE88(0, 86),
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellPosStructT_v02, latitude),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellPosStructT_v02, longitude),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellPosStructT_v02, horCoverageRadius),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellPosStructT_v02, horConfidence),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellPosStructT_v02, horReliability),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellPosStructT_v02, altitude),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellPosStructT_v02, altUnc),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellPosStructT_v02, altConfidence),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocFdclCellPosStructT_v02, altReliability),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocNextLeapSecondInfoStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNextLeapSecondInfoStructT_v02, gpsTimeCurrent_valid),

  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocNextLeapSecondInfoStructT_v02, gpsTimeCurrent),
  QMI_IDL_TYPE88(0, 2),
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNextLeapSecondInfoStructT_v02, leapSecondsCurrent_valid),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNextLeapSecondInfoStructT_v02, leapSecondsCurrent),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNextLeapSecondInfoStructT_v02, gpsTimeNextLsEvent_valid),

  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocNextLeapSecondInfoStructT_v02, gpsTimeNextLsEvent),
  QMI_IDL_TYPE88(0, 2),
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNextLeapSecondInfoStructT_v02, leapSecondsNext_valid),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNextLeapSecondInfoStructT_v02, leapSecondsNext),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocBsObservedDataStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocBsObservedDataStructT_v02, validMask),

  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocBsObservedDataStructT_v02, cellId),
  QMI_IDL_TYPE88(0, 86),
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocBsObservedDataStructT_v02, gpsWeek),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBsObservedDataStructT_v02, gpsMilliseconds),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBsObservedDataStructT_v02, bsLocalTimestamp),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBsObservedDataStructT_v02, posDeltaTime),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBsObservedDataStructT_v02, speed),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocBsObservedDataStructT_v02, speedUnc),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocBsObservedDataStructT_v02, heading),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocBsObservedDataStructT_v02, headingUnc),

  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocBsObservedDataStructT_v02, bsBestPosition),
  QMI_IDL_TYPE88(0, 85),
  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocEphGnssDataStructT_data_v02[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, gnssSvId),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, updateAction),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, IODE),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, aSqrt),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, deltaN),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, m0),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, eccentricity),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, omega0),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, i0),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, omega),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, omegaDot),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, iDot),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, cUc),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, cUs),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, cRc),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, cRs),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, cIc),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, cIs),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, toe),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, toc),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, af0),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, af1),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEphGnssDataStructT_v02, af2),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocGpsEphemerisT_data_v02[] = {
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGpsEphemerisT_v02, commonEphemerisData),
  QMI_IDL_TYPE88(0, 90),
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGpsEphemerisT_v02, signalHealth),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGpsEphemerisT_v02, URAI),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGpsEphemerisT_v02, codeL2),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGpsEphemerisT_v02, dataFlagL2P),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGpsEphemerisT_v02, tgd),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGpsEphemerisT_v02, fitInterval),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocGpsEphemerisT_v02, IODC),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocGloEphemerisT_data_v02[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, gnssSvId),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, updateAction),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, bnHealth),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, lnHealth),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, tb),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, ft),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, gloM),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, enAge),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, gloFrequency),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, p1),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, p2),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, deltaTau),

  QMI_IDL_FLAGS_IS_ARRAY |QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, position),
  3,

  QMI_IDL_FLAGS_IS_ARRAY |QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, velocity),
  3,

  QMI_IDL_FLAGS_IS_ARRAY |QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, acceleration),
  3,

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, tauN),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, gamma),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, toe),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisT_v02, nt),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocBdsEphemerisT_data_v02[] = {
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocBdsEphemerisT_v02, commonEphemerisData),
  QMI_IDL_TYPE88(0, 90),
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocBdsEphemerisT_v02, svHealth),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocBdsEphemerisT_v02, AODC),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocBdsEphemerisT_v02, tgd1),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocBdsEphemerisT_v02, tgd2),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocBdsEphemerisT_v02, URAI),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocGalEphemerisT_data_v02[] = {
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGalEphemerisT_v02, commonEphemerisData),
  QMI_IDL_TYPE88(0, 90),
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGalEphemerisT_v02, dataSourceSignal),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGalEphemerisT_v02, sisIndex),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGalEphemerisT_v02, bgdE1E5a),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGalEphemerisT_v02, bgdE1E5b),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGalEphemerisT_v02, svHealth),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocKlobucharIonoModelT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocKlobucharIonoModelT_v02, dataSource),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocKlobucharIonoModelT_v02, alpha0),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocKlobucharIonoModelT_v02, alpha1),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocKlobucharIonoModelT_v02, alpha2),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocKlobucharIonoModelT_v02, alpha3),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocKlobucharIonoModelT_v02, beta0),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocKlobucharIonoModelT_v02, beta1),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocKlobucharIonoModelT_v02, beta2),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocKlobucharIonoModelT_v02, beta3),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocMePerGnssRfStructType_data_v02[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocMePerGnssRfStructType_v02, bpAmpI),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocMePerGnssRfStructType_v02, bpAmpQ),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocMePerGnssRfStructType_v02, jammerPwrDb),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocRobustLocationVersionT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocRobustLocationVersionT_v02, major),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocRobustLocationVersionT_v02, minor),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocEnvAidingReflectingPlaneStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnvAidingReflectingPlaneStructT_v02, latitudeDegrees),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnvAidingReflectingPlaneStructT_v02, longitudeDegrees),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnvAidingReflectingPlaneStructT_v02, altitudeMeters),

  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnvAidingReflectingPlaneStructT_v02, azimuthDegrees),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocEnvAidingSVCorrectionStructT_data_v02[] = {
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnvAidingSVCorrectionStructT_v02, svCorrectionFlags),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnvAidingSVCorrectionStructT_v02, constellation),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnvAidingSVCorrectionStructT_v02, svid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnvAidingSVCorrectionStructT_v02, carrierFrequencyHz),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnvAidingSVCorrectionStructT_v02, probabilitySvIsLineofSight),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnvAidingSVCorrectionStructT_v02, excessPathLengthMeters),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnvAidingSVCorrectionStructT_v02, excessPathLengthUncMeters),

  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEnvAidingSVCorrectionStructT_v02, reflectingPlane),
  QMI_IDL_TYPE88(0, 98),
  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocNavDataStructT_data_v02[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocNavDataStructT_v02, gnssSvId),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNavDataStructT_v02, type),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNavDataStructT_v02, src),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNavDataStructT_v02, age),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocTimeStructT_data_v02[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocTimeStructT_v02, hours),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocTimeStructT_v02, mins),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocTimeStructT_v02, secs),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocOsnmaTreeNodeT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocOsnmaTreeNodeT_v02, height),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocOsnmaTreeNodeT_v02, position),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocOsnmaTreeNodeT_v02, hash),
  QMI_LOC_MERKLE_TREE_HASH_ARRAY_LENGTH_V02,
  QMI_IDL_OFFSET8(qmiLocOsnmaTreeNodeT_v02, hash) - QMI_IDL_OFFSET8(qmiLocOsnmaTreeNodeT_v02, hash_len),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t qmiLocMlInferSVMeasurementStructT_data_v02[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocMlInferSVMeasurementStructT_v02, mlInfer_valid),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocMlInferSVMeasurementStructT_v02, mlInfer),

  QMI_IDL_FLAG_END_VALUE
};

/*Message Definitions*/
/*
 * qmiLocGenReqMsgT is empty
 * static const uint8_t qmiLocGenReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGenRespMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGenRespMsgT_v02, resp),
  QMI_IDL_TYPE88(1, 0)
};

static const uint8_t qmiLocGenReqStatusIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGenReqStatusIndMsgT_v02, status)
};

static const uint8_t qmiLocInformClientRevisionReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInformClientRevisionReqMsgT_v02, revision)
};

static const uint8_t qmiLocRegEventsReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocRegEventsReqMsgT_v02, eventRegMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocRegEventsReqMsgT_v02, clientStrId) - QMI_IDL_OFFSET8(qmiLocRegEventsReqMsgT_v02, clientStrId_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocRegEventsReqMsgT_v02, clientStrId),
  QMI_LOC_MAX_CLIENT_ID_STRING_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocRegEventsReqMsgT_v02, clientType) - QMI_IDL_OFFSET8(qmiLocRegEventsReqMsgT_v02, clientType_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocRegEventsReqMsgT_v02, clientType),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocRegEventsReqMsgT_v02, enablePosRequestNotification) - QMI_IDL_OFFSET8(qmiLocRegEventsReqMsgT_v02, enablePosRequestNotification_valid)),
  0x12,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocRegEventsReqMsgT_v02, enablePosRequestNotification)
};

static const uint8_t qmiLocStartReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, sessionId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, fixRecurrence) - QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, fixRecurrence_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, fixRecurrence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, horizontalAccuracyLevel) - QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, horizontalAccuracyLevel_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, horizontalAccuracyLevel),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, intermediateReportState) - QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, intermediateReportState_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, intermediateReportState),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, minInterval) - QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, minInterval_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, minInterval),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, applicationId) - QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, applicationId_valid)),
  0x14,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, applicationId),
  QMI_IDL_TYPE88(0, 0),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, configAltitudeAssumed) - QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, configAltitudeAssumed_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, configAltitudeAssumed),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, minIntermediatePositionReportInterval) - QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, minIntermediatePositionReportInterval_valid)),
  0x16,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, minIntermediatePositionReportInterval),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, positionReportTimeout) - QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, positionReportTimeout_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, positionReportTimeout),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, sharePosition) - QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, sharePosition_valid)),
  0x18,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, sharePosition),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, reportGnssOnlyPosition) - QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, reportGnssOnlyPosition_valid)),
  0x19,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, reportGnssOnlyPosition),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, powerMode) - QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, powerMode_valid)),
  0x1A,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, powerMode),
  QMI_IDL_TYPE88(0, 1),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, specialReqType) - QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, specialReqType_valid)),
  0x1B,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartReqMsgT_v02, specialReqType)
};

static const uint8_t qmiLocStopReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocStopReqMsgT_v02, sessionId)
};

static const uint8_t qmiLocEventPositionReportIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, sessionStatus),

  0x02,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, sessionId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, latitude) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, latitude_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, latitude),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, longitude) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, longitude_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, longitude),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horUncCircular) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horUncCircular_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horUncCircular),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horUncEllipseSemiMinor) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horUncEllipseSemiMinor_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horUncEllipseSemiMinor),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horUncEllipseSemiMajor) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horUncEllipseSemiMajor_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horUncEllipseSemiMajor),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horUncEllipseOrientAzimuth) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horUncEllipseOrientAzimuth_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horUncEllipseOrientAzimuth),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horConfidence) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horConfidence_valid)),
  0x16,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horReliability) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horReliability_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, horReliability),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, speedHorizontal) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, speedHorizontal_valid)),
  0x18,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, speedHorizontal),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, speedUnc) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, speedUnc_valid)),
  0x19,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, speedUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, altitudeWrtEllipsoid) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, altitudeWrtEllipsoid_valid)),
  0x1A,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, altitudeWrtEllipsoid),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, altitudeWrtMeanSeaLevel) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, altitudeWrtMeanSeaLevel_valid)),
  0x1B,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, altitudeWrtMeanSeaLevel),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, vertUnc) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, vertUnc_valid)),
  0x1C,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, vertUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, vertConfidence) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, vertConfidence_valid)),
  0x1D,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, vertConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, vertReliability) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, vertReliability_valid)),
  0x1E,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, vertReliability),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, speedVertical) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, speedVertical_valid)),
  0x1F,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, speedVertical),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, heading) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, heading_valid)),
  0x20,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, heading),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, headingUnc) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, headingUnc_valid)),
  0x21,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, headingUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, magneticDeviation) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, magneticDeviation_valid)),
  0x22,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, magneticDeviation),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, technologyMask) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, technologyMask_valid)),
  0x23,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, technologyMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, DOP) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, DOP_valid)),
  0x24,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, DOP),
  QMI_IDL_TYPE88(0, 3),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, timestampUtc) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, timestampUtc_valid)),
  0x25,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, timestampUtc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, leapSeconds) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, leapSeconds_valid)),
  0x26,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, leapSeconds),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, gpsTime) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, gpsTime_valid)),
  0x27,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, gpsTime),
  QMI_IDL_TYPE88(0, 2),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, timeUnc) - QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, timeUnc_valid)),
  0x28,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPositionReportIndMsgT_v02, timeUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, timeSrc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, timeSrc_valid)),
  0x29,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, timeSrc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, sensorDataUsage) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, sensorDataUsage_valid)),
  0x2A,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, sensorDataUsage),
  QMI_IDL_TYPE88(0, 5),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, fixId) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, fixId_valid)),
  0x2B,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, fixId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, gnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, gnssSvUsedList_valid)),
  0x2C,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, gnssSvUsedList),
  QMI_LOC_MAX_SV_USED_LIST_LENGTH_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, gnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, gnssSvUsedList_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, altitudeAssumed) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, altitudeAssumed_valid)),
  0x2D,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, altitudeAssumed),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, velEnu) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, velEnu_valid)),
  0x2E,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, velEnu),
  QMI_LOC_ENU_ARRAY_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, velUncEnu) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, velUncEnu_valid)),
  0x2F,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, velUncEnu),
  QMI_LOC_ENU_ARRAY_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, navSolutionMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, navSolutionMask_valid)),
  0x30,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, navSolutionMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, sensorSubTechnologyMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, sensorSubTechnologyMask_valid)),
  0x31,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, sensorSubTechnologyMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, gnssOnlyPosition) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, gnssOnlyPosition_valid)),
  0x32,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, gnssOnlyPosition),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, extDOP) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, extDOP_valid)),
  0x33,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, extDOP),
  QMI_IDL_TYPE88(0, 4),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, dgnssStationId) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, dgnssStationId_valid)),
  0x34,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, dgnssStationId),
  QMI_LOC_DGNSS_STATION_ID_ARRAY_LENGTH_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, dgnssStationId) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, dgnssStationId_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, spoofReportMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, spoofReportMask_valid)),
  0x35,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, spoofReportMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, expandedGnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, expandedGnssSvUsedList_valid)),
  0x36,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, expandedGnssSvUsedList),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, expandedGnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, expandedGnssSvUsedList_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, gnssSvUsedSignalTypeList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, gnssSvUsedSignalTypeList_valid)),
  0x37,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, gnssSvUsedSignalTypeList),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, gnssSvUsedSignalTypeList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, gnssSvUsedSignalTypeList_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, jammerIndicatorList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, jammerIndicatorList_valid)),
  0x38,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, jammerIndicatorList),
  QMI_LOC_MAX_GNSS_SIGNAL_TYPE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, jammerIndicatorList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, jammerIndicatorList_len),
  QMI_IDL_TYPE88(0, 6),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, dgnssCorrectionSource) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, dgnssCorrectionSource_valid)),
  0x39,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, dgnssCorrectionSource),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, dgnssCorrectionSourceID) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, dgnssCorrectionSourceID_valid)),
  0x3A,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, dgnssCorrectionSourceID),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, dgnssConstellationUsage) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, dgnssConstellationUsage_valid)),
  0x3B,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, dgnssConstellationUsage),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, dgnssRefStationId) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, dgnssRefStationId_valid)),
  0x3C,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, dgnssRefStationId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, dgnssDataAgeMsec) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, dgnssDataAgeMsec_valid)),
  0x3D,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, dgnssDataAgeMsec),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, conformityIndex) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, conformityIndex_valid)),
  0x3E,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, conformityIndex),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, systemTick) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, systemTick_valid)),
  0x3F,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, systemTick),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, systemTickUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, systemTickUnc_valid)),
  0x40,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, systemTickUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, systemClkTimeBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, systemClkTimeBias_valid)),
  0x41,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, systemClkTimeBias),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, systemClkTimeBiasUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, systemClkTimeBiasUnc_valid)),
  0x42,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, systemClkTimeBiasUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, payload) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, payload_valid)),
  0x43,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, payload),
  ((4096) & 0xFF), ((4096) >> 8),
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, payload) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, payload_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, jammerIndicatorListExt) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, jammerIndicatorListExt_valid)),
  0x44,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, jammerIndicatorListExt),
  QMI_LOC_MAX_GNSS_SIGNAL_TYPE_EXT_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, jammerIndicatorListExt) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, jammerIndicatorListExt_len),
  QMI_IDL_TYPE88(0, 7),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, gnssTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, gnssTime_valid)),
  0x45,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, gnssTime),
  QMI_IDL_TYPE88(0, 62),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, leapSecUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventPositionReportIndMsgT_v02, leapSecUnc_valid)),
  0x46,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventPositionReportIndMsgT_v02, leapSecUnc)
};

static const uint8_t qmiLocEventGnssSvInfoIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvInfoIndMsgT_v02, altitudeAssumed),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvInfoIndMsgT_v02, svList) - QMI_IDL_OFFSET8(qmiLocEventGnssSvInfoIndMsgT_v02, svList_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvInfoIndMsgT_v02, svList),
  QMI_LOC_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvInfoIndMsgT_v02, svList) - QMI_IDL_OFFSET8(qmiLocEventGnssSvInfoIndMsgT_v02, svList_len),
  QMI_IDL_TYPE88(0, 8),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvInfoIndMsgT_v02, expandedSvList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvInfoIndMsgT_v02, expandedSvList_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvInfoIndMsgT_v02, expandedSvList),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvInfoIndMsgT_v02, expandedSvList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvInfoIndMsgT_v02, expandedSvList_len),
  QMI_IDL_TYPE88(0, 9),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvInfoIndMsgT_v02, gnssSignalTypeList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvInfoIndMsgT_v02, gnssSignalTypeList_valid)),
  0x12,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvInfoIndMsgT_v02, gnssSignalTypeList),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvInfoIndMsgT_v02, gnssSignalTypeList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvInfoIndMsgT_v02, gnssSignalTypeList_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvInfoIndMsgT_v02, rfLoss) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvInfoIndMsgT_v02, rfLoss_valid)),
  0x13,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvInfoIndMsgT_v02, rfLoss),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvInfoIndMsgT_v02, rfLoss) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvInfoIndMsgT_v02, rfLoss_len)
};

static const uint8_t qmiLocEventNmeaIndMsgT_data_v02[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocEventNmeaIndMsgT_v02, nmea),
  QMI_LOC_NMEA_STRING_MAX_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventNmeaIndMsgT_v02, expandedNmea) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventNmeaIndMsgT_v02, expandedNmea_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventNmeaIndMsgT_v02, expandedNmea),
  ((QMI_LOC_EXPANDED_NMEA_STRING_MAX_LENGTH_V02) & 0xFF), ((QMI_LOC_EXPANDED_NMEA_STRING_MAX_LENGTH_V02) >> 8)
};

static const uint8_t qmiLocEventNiNotifyVerifyReqIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, notificationType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiVxInd) - QMI_IDL_OFFSET8(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiVxInd_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiVxInd),
  QMI_IDL_TYPE88(0, 10),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiSuplInd) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiSuplInd_valid)),
  0x11,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiSuplInd),
  QMI_IDL_TYPE88(0, 16),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiUmtsCpInd) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiUmtsCpInd_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiUmtsCpInd),
  QMI_IDL_TYPE88(0, 18),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiVxServiceInteractionInd) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiVxServiceInteractionInd_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiVxServiceInteractionInd),
  QMI_IDL_TYPE88(0, 19),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiSuplVer2ExtInd) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiSuplVer2ExtInd_valid)),
  0x14,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, NiSuplVer2ExtInd),
  QMI_IDL_TYPE88(0, 20),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, suplEmergencyNotification) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, suplEmergencyNotification_valid)),
  0x15,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, suplEmergencyNotification),
  QMI_IDL_TYPE88(0, 21),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, isInEmergencySession) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, isInEmergencySession_valid)),
  0x16,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventNiNotifyVerifyReqIndMsgT_v02, isInEmergencySession)
};

static const uint8_t qmiLocEventInjectTimeReqIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectTimeReqIndMsgT_v02, timeServerInfo) - QMI_IDL_OFFSET8(qmiLocEventInjectTimeReqIndMsgT_v02, timeServerInfo_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventInjectTimeReqIndMsgT_v02, timeServerInfo),
  QMI_IDL_TYPE88(0, 23)
};

static const uint8_t qmiLocEventInjectPredictedOrbitsReqIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, allowedSizes),
  QMI_IDL_TYPE88(0, 24),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, serverList) - QMI_IDL_OFFSET8(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, serverList_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, serverList),
  QMI_IDL_TYPE88(0, 25),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, serverUpdateMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, serverUpdateMask_valid)),
  0x11,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, serverUpdateMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, updateType) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, updateType_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, updateType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, updateRate) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, updateRate_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, updateRate),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, fileInfo) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, fileInfo_valid)),
  0x14,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02, fileInfo),
  QMI_IDL_TYPE88(0, 26)
};

static const uint8_t qmiLocEventInjectPositionReqIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectPositionReqIndMsgT_v02, latitude),

  0x02,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectPositionReqIndMsgT_v02, longitude),

  0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectPositionReqIndMsgT_v02, horUncCircular),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x04,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectPositionReqIndMsgT_v02, timestampUtc)
};

static const uint8_t qmiLocEventEngineStateIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventEngineStateIndMsgT_v02, engineState)
};

static const uint8_t qmiLocEventFixSessionStateIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventFixSessionStateIndMsgT_v02, sessionState),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventFixSessionStateIndMsgT_v02, sessionId) - QMI_IDL_OFFSET8(qmiLocEventFixSessionStateIndMsgT_v02, sessionId_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventFixSessionStateIndMsgT_v02, sessionId)
};

static const uint8_t qmiLocEventWifiReqIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventWifiReqIndMsgT_v02, requestType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventWifiReqIndMsgT_v02, tbfInMs) - QMI_IDL_OFFSET8(qmiLocEventWifiReqIndMsgT_v02, tbfInMs_valid)),
  0x10,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventWifiReqIndMsgT_v02, tbfInMs),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventWifiReqIndMsgT_v02, e911Mode) - QMI_IDL_OFFSET8(qmiLocEventWifiReqIndMsgT_v02, e911Mode_valid)),
  0x11,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventWifiReqIndMsgT_v02, e911Mode),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventWifiReqIndMsgT_v02, civicAddressNeeded) - QMI_IDL_OFFSET8(qmiLocEventWifiReqIndMsgT_v02, civicAddressNeeded_valid)),
  0x12,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventWifiReqIndMsgT_v02, civicAddressNeeded)
};

static const uint8_t qmiLocEventSensorStreamingReadyStatusIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, accelReady) - QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, accelReady_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, accelReady),
  QMI_IDL_TYPE88(0, 28),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, gyroReady) - QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, gyroReady_valid)),
  0x11,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, gyroReady),
  QMI_IDL_TYPE88(0, 28),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, accelTemperatureReady) - QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, accelTemperatureReady_valid)),
  0x12,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, accelTemperatureReady),
  QMI_IDL_TYPE88(0, 28),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, gyroTemperatureReady) - QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, gyroTemperatureReady_valid)),
  0x13,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, gyroTemperatureReady),
  QMI_IDL_TYPE88(0, 28),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, calibratedMagReady) - QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, calibratedMagReady_valid)),
  0x14,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, calibratedMagReady),
  QMI_IDL_TYPE88(0, 28),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, uncalibratedMagReady) - QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, uncalibratedMagReady_valid)),
  0x15,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02, uncalibratedMagReady),
  QMI_IDL_TYPE88(0, 28)
};

static const uint8_t qmiLocEventTimeSyncReqIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventTimeSyncReqIndMsgT_v02, refCounter)
};

static const uint8_t qmiLocEventSetSpiStreamingReportIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventSetSpiStreamingReportIndMsgT_v02, enable)
};

static const uint8_t qmiLocEventLocationServerConnectionReqIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventLocationServerConnectionReqIndMsgT_v02, connHandle),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventLocationServerConnectionReqIndMsgT_v02, requestType),

  0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventLocationServerConnectionReqIndMsgT_v02, wwanType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventLocationServerConnectionReqIndMsgT_v02, bearerType) - QMI_IDL_OFFSET8(qmiLocEventLocationServerConnectionReqIndMsgT_v02, bearerType_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventLocationServerConnectionReqIndMsgT_v02, bearerType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventLocationServerConnectionReqIndMsgT_v02, apnTypeMask) - QMI_IDL_OFFSET8(qmiLocEventLocationServerConnectionReqIndMsgT_v02, apnTypeMask_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventLocationServerConnectionReqIndMsgT_v02, apnTypeMask),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventLocationServerConnectionReqIndMsgT_v02, subId) - QMI_IDL_OFFSET8(qmiLocEventLocationServerConnectionReqIndMsgT_v02, subId_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventLocationServerConnectionReqIndMsgT_v02, subId)
};

static const uint8_t qmiLocEventNiGeofenceNotificationIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventNiGeofenceNotificationIndMsgT_v02, geofenceId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventNiGeofenceNotificationIndMsgT_v02, operationType)
};

static const uint8_t qmiLocEventGeofenceGenAlertIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGeofenceGenAlertIndMsgT_v02, geofenceAlert)
};

static const uint8_t qmiLocEventGeofenceBreachIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGeofenceBreachIndMsgT_v02, geofenceId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGeofenceBreachIndMsgT_v02, breachType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGeofenceBreachIndMsgT_v02, geofencePosition) - QMI_IDL_OFFSET8(qmiLocEventGeofenceBreachIndMsgT_v02, geofencePosition_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGeofenceBreachIndMsgT_v02, geofencePosition),
  QMI_IDL_TYPE88(0, 29),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGeofenceBreachIndMsgT_v02, breachConfidence) - QMI_IDL_OFFSET8(qmiLocEventGeofenceBreachIndMsgT_v02, breachConfidence_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGeofenceBreachIndMsgT_v02, breachConfidence)
};

static const uint8_t qmiLocEventPedometerControlIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPedometerControlIndMsgT_v02, requestPedometerData),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPedometerControlIndMsgT_v02, resetStepCount) - QMI_IDL_OFFSET8(qmiLocEventPedometerControlIndMsgT_v02, resetStepCount_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPedometerControlIndMsgT_v02, resetStepCount),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventPedometerControlIndMsgT_v02, stepCountThreshold) - QMI_IDL_OFFSET8(qmiLocEventPedometerControlIndMsgT_v02, stepCountThreshold_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventPedometerControlIndMsgT_v02, stepCountThreshold)
};

static const uint8_t qmiLocEventMotionDataControlIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventMotionDataControlIndMsgT_v02, requestMotionData)
};

static const uint8_t qmiLocEventGeofenceBatchedBreachIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, breachType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, geofenceIdContinuousList) - QMI_IDL_OFFSET8(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, geofenceIdContinuousList_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, geofenceIdContinuousList),
  QMI_LOC_MAX_GEOFENCE_ID_CONTINUOUS_LIST_LENGTH_V02,
  QMI_IDL_OFFSET8(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, geofenceIdContinuousList) - QMI_IDL_OFFSET8(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, geofenceIdContinuousList_len),
  QMI_IDL_TYPE88(0, 30),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, geofenceIdDiscreteList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, geofenceIdDiscreteList_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, geofenceIdDiscreteList),
  QMI_LOC_MAX_GEOFENCE_ID_DISCRETE_LIST_LENGTH_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, geofenceIdDiscreteList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, geofenceIdDiscreteList_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, geofencePosition) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, geofencePosition_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, geofencePosition),
  QMI_IDL_TYPE88(0, 29),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, breachConfidence) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, breachConfidence_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, breachConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, headingUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, headingUnc_valid)),
  0x14,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, headingUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, vertUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, vertUnc_valid)),
  0x15,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, vertUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, speedUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, speedUnc_valid)),
  0x16,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, speedUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, horConfidence) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, horConfidence_valid)),
  0x17,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, horConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, vertConfidence) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, vertConfidence_valid)),
  0x18,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, vertConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, DOP) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, DOP_valid)),
  0x19,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, DOP),
  QMI_IDL_TYPE88(0, 3),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, gnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, gnssSvUsedList_valid)),
  0x1A,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, gnssSvUsedList),
  QMI_LOC_MAX_SV_USED_LIST_LENGTH_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, gnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, gnssSvUsedList_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, extDOP) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, extDOP_valid)),
  0x1B,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, extDOP),
  QMI_IDL_TYPE88(0, 4),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, expandedGnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, expandedGnssSvUsedList_valid)),
  0x1C,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, expandedGnssSvUsedList),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, expandedGnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, expandedGnssSvUsedList_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, gnssSvUsedSignalTypeList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, gnssSvUsedSignalTypeList_valid)),
  0x1D,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, gnssSvUsedSignalTypeList),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, gnssSvUsedSignalTypeList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedBreachIndMsgT_v02, gnssSvUsedSignalTypeList_len)
};

static const uint8_t qmiLocEventGeofenceProximityIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGeofenceProximityIndMsgT_v02, proximityType),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGeofenceProximityIndMsgT_v02, geofenceId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGeofenceProximityIndMsgT_v02, contextId) - QMI_IDL_OFFSET8(qmiLocEventGeofenceProximityIndMsgT_v02, contextId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGeofenceProximityIndMsgT_v02, contextId)
};

static const uint8_t qmiLocEventGeofenceBatchedDwellIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, dwellType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, geofenceIdContinuousList) - QMI_IDL_OFFSET8(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, geofenceIdContinuousList_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, geofenceIdContinuousList),
  QMI_LOC_MAX_GEOFENCE_ID_CONTINUOUS_LIST_LENGTH_V02,
  QMI_IDL_OFFSET8(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, geofenceIdContinuousList) - QMI_IDL_OFFSET8(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, geofenceIdContinuousList_len),
  QMI_IDL_TYPE88(0, 30),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, geofenceIdDiscreteList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, geofenceIdDiscreteList_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, geofenceIdDiscreteList),
  QMI_LOC_MAX_GEOFENCE_ID_DISCRETE_LIST_LENGTH_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, geofenceIdDiscreteList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, geofenceIdDiscreteList_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, geofencePosition) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, geofencePosition_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, geofencePosition),
  QMI_IDL_TYPE88(0, 29),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, headingUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, headingUnc_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, headingUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, vertUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, vertUnc_valid)),
  0x14,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, vertUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, speedUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, speedUnc_valid)),
  0x15,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, speedUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, horConfidence) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, horConfidence_valid)),
  0x16,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, horConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, vertConfidence) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, vertConfidence_valid)),
  0x17,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, vertConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, DOP) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, DOP_valid)),
  0x18,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, DOP),
  QMI_IDL_TYPE88(0, 3),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, gnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, gnssSvUsedList_valid)),
  0x19,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, gnssSvUsedList),
  QMI_LOC_MAX_SV_USED_LIST_LENGTH_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, gnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, gnssSvUsedList_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, extDOP) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, extDOP_valid)),
  0x1A,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, extDOP),
  QMI_IDL_TYPE88(0, 4),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, expandedGnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, expandedGnssSvUsedList_valid)),
  0x1B,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, expandedGnssSvUsedList),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, expandedGnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, expandedGnssSvUsedList_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, gnssSvUsedSignalTypeList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, gnssSvUsedSignalTypeList_valid)),
  0x1C,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, gnssSvUsedSignalTypeList),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, gnssSvUsedSignalTypeList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGeofenceBatchedDwellIndMsgT_v02, gnssSvUsedSignalTypeList_len)
};

static const uint8_t qmiLocEventGdtUploadBeginStatusReqIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtUploadBeginStatusReqIndMsgT_v02, serviceId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtUploadBeginStatusReqIndMsgT_v02, sessionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x03,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtUploadBeginStatusReqIndMsgT_v02, filePath),
  QMI_LOC_MAX_GDT_PATH_LEN_V02,
  QMI_IDL_OFFSET8(qmiLocEventGdtUploadBeginStatusReqIndMsgT_v02, filePath) - QMI_IDL_OFFSET8(qmiLocEventGdtUploadBeginStatusReqIndMsgT_v02, filePath_len)
};

static const uint8_t qmiLocEventGdtUploadEndReqIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtUploadEndReqIndMsgT_v02, serviceId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtUploadEndReqIndMsgT_v02, sessionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtUploadEndReqIndMsgT_v02, endStatus)
};

static const uint8_t qmiLocEventGdtDownloadBeginReqIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, serviceId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, sessionId),

  0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, respTimeoutInterval),

  0x04,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, clientInfo),
  ((QMI_LOC_MAX_GTP_CL_INFO_LEN_V02) & 0xFF), ((QMI_LOC_MAX_GTP_CL_INFO_LEN_V02) >> 8),
  QMI_IDL_OFFSET8(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, clientInfo) - QMI_IDL_OFFSET8(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, clientInfo_len),

  0x05,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, mobileStatusData),
  ((QMI_LOC_MAX_GTP_MSD_LEN_V02) & 0xFF), ((QMI_LOC_MAX_GTP_MSD_LEN_V02) >> 8),
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, mobileStatusData) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, mobileStatusData_len),

  0x06,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, filePath),
  QMI_LOC_MAX_GDT_PATH_LEN_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, filePath) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, filePath_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, powerBudgetInfo) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, powerBudgetInfo_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, powerBudgetInfo),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, powerBudgetAllowance) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, powerBudgetAllowance_valid)),
  0x11,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, powerBudgetAllowance),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, downloadRequestMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, downloadRequestMask_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGdtDownloadBeginReqIndMsgT_v02, downloadRequestMask)
};

static const uint8_t qmiLocEventGdtReceiveDoneIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtReceiveDoneIndMsgT_v02, serviceId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtReceiveDoneIndMsgT_v02, sessionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtReceiveDoneIndMsgT_v02, status)
};

static const uint8_t qmiLocEventGdtDownloadEndReqIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtDownloadEndReqIndMsgT_v02, serviceId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtDownloadEndReqIndMsgT_v02, sessionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGdtDownloadEndReqIndMsgT_v02, status)
};

/*
 * qmiLocGetServiceRevisionReqMsgT is empty
 * static const uint8_t qmiLocGetServiceRevisionReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetServiceRevisionIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetServiceRevisionIndMsgT_v02, status),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetServiceRevisionIndMsgT_v02, revision),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetServiceRevisionIndMsgT_v02, gnssMeFWVerString) - QMI_IDL_OFFSET8(qmiLocGetServiceRevisionIndMsgT_v02, gnssMeFWVerString_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocGetServiceRevisionIndMsgT_v02, gnssMeFWVerString),
  QMI_LOC_GNSS_ME_VERSION_STRING_MAX_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetServiceRevisionIndMsgT_v02, gnssHostSWVerString) - QMI_IDL_OFFSET8(qmiLocGetServiceRevisionIndMsgT_v02, gnssHostSWVerString_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocGetServiceRevisionIndMsgT_v02, gnssHostSWVerString),
  QMI_LOC_GNSS_HOSTED_SW_VERSION_STRING_MAX_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetServiceRevisionIndMsgT_v02, gnssSWVerString) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetServiceRevisionIndMsgT_v02, gnssSWVerString_valid)),
  0x12,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetServiceRevisionIndMsgT_v02, gnssSWVerString),
  QMI_LOC_GNSS_SW_VERSION_STRING_MAX_LENGTH_V02
};

/*
 * qmiLocGetFixCriteriaReqMsgT is empty
 * static const uint8_t qmiLocGetFixCriteriaReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetFixCriteriaIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, horizontalAccuracyLevel) - QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, horizontalAccuracyLevel_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, horizontalAccuracyLevel),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, intermediateReportState) - QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, intermediateReportState_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, intermediateReportState),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, minInterval) - QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, minInterval_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, minInterval),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, applicationId) - QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, applicationId_valid)),
  0x13,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, applicationId),
  QMI_IDL_TYPE88(0, 0),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, configAltitudeAssumed) - QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, configAltitudeAssumed_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, configAltitudeAssumed),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, minIntermediatePositionReportInterval) - QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, minIntermediatePositionReportInterval_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, minIntermediatePositionReportInterval),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, positionReportTimeout) - QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, positionReportTimeout_valid)),
  0x16,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, positionReportTimeout),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, sharePosition) - QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, sharePosition_valid)),
  0x17,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, sharePosition),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, powerMode) - QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, powerMode_valid)),
  0x18,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetFixCriteriaIndMsgT_v02, powerMode),
  QMI_IDL_TYPE88(0, 1)
};

static const uint8_t qmiLocNiUserRespReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiUserRespReqMsgT_v02, userResp),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiUserRespReqMsgT_v02, notificationType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocNiUserRespReqMsgT_v02, NiVxPayload) - QMI_IDL_OFFSET8(qmiLocNiUserRespReqMsgT_v02, NiVxPayload_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocNiUserRespReqMsgT_v02, NiVxPayload),
  QMI_IDL_TYPE88(0, 10),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocNiUserRespReqMsgT_v02, NiSuplPayload) - QMI_IDL_OFFSET16RELATIVE(qmiLocNiUserRespReqMsgT_v02, NiSuplPayload_valid)),
  0x11,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiUserRespReqMsgT_v02, NiSuplPayload),
  QMI_IDL_TYPE88(0, 16),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocNiUserRespReqMsgT_v02, NiUmtsCpPayload) - QMI_IDL_OFFSET16RELATIVE(qmiLocNiUserRespReqMsgT_v02, NiUmtsCpPayload_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiUserRespReqMsgT_v02, NiUmtsCpPayload),
  QMI_IDL_TYPE88(0, 18),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocNiUserRespReqMsgT_v02, NiVxServiceInteractionPayload) - QMI_IDL_OFFSET16RELATIVE(qmiLocNiUserRespReqMsgT_v02, NiVxServiceInteractionPayload_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiUserRespReqMsgT_v02, NiVxServiceInteractionPayload),
  QMI_IDL_TYPE88(0, 19),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocNiUserRespReqMsgT_v02, NiSuplVer2ExtPayload) - QMI_IDL_OFFSET16RELATIVE(qmiLocNiUserRespReqMsgT_v02, NiSuplVer2ExtPayload_valid)),
  0x14,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiUserRespReqMsgT_v02, NiSuplVer2ExtPayload),
  QMI_IDL_TYPE88(0, 20),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocNiUserRespReqMsgT_v02, suplEmergencyNotification) - QMI_IDL_OFFSET16RELATIVE(qmiLocNiUserRespReqMsgT_v02, suplEmergencyNotification_valid)),
  0x15,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocNiUserRespReqMsgT_v02, suplEmergencyNotification),
  QMI_IDL_TYPE88(0, 21)
};

static const uint8_t qmiLocNiUserRespIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNiUserRespIndMsgT_v02, status)
};

static const uint8_t qmiLocInjectPredictedOrbitsDataReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPredictedOrbitsDataReqMsgT_v02, totalSize),

  0x02,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPredictedOrbitsDataReqMsgT_v02, totalParts),

  0x03,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPredictedOrbitsDataReqMsgT_v02, partNum),

  0x04,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPredictedOrbitsDataReqMsgT_v02, partData),
  ((QMI_LOC_MAX_PREDICTED_ORBITS_PART_LEN_V02) & 0xFF), ((QMI_LOC_MAX_PREDICTED_ORBITS_PART_LEN_V02) >> 8),
  QMI_IDL_OFFSET8(qmiLocInjectPredictedOrbitsDataReqMsgT_v02, partData) - QMI_IDL_OFFSET8(qmiLocInjectPredictedOrbitsDataReqMsgT_v02, partData_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectPredictedOrbitsDataReqMsgT_v02, formatType) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectPredictedOrbitsDataReqMsgT_v02, formatType_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectPredictedOrbitsDataReqMsgT_v02, formatType)
};

static const uint8_t qmiLocInjectPredictedOrbitsDataIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPredictedOrbitsDataIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPredictedOrbitsDataIndMsgT_v02, partNum) - QMI_IDL_OFFSET8(qmiLocInjectPredictedOrbitsDataIndMsgT_v02, partNum_valid)),
  0x10,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPredictedOrbitsDataIndMsgT_v02, partNum)
};

/*
 * qmiLocGetPredictedOrbitsDataSourceReqMsgT is empty
 * static const uint8_t qmiLocGetPredictedOrbitsDataSourceReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetPredictedOrbitsDataSourceIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetPredictedOrbitsDataSourceIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetPredictedOrbitsDataSourceIndMsgT_v02, allowedSizes) - QMI_IDL_OFFSET8(qmiLocGetPredictedOrbitsDataSourceIndMsgT_v02, allowedSizes_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetPredictedOrbitsDataSourceIndMsgT_v02, allowedSizes),
  QMI_IDL_TYPE88(0, 24),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetPredictedOrbitsDataSourceIndMsgT_v02, serverList) - QMI_IDL_OFFSET8(qmiLocGetPredictedOrbitsDataSourceIndMsgT_v02, serverList_valid)),
  0x11,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetPredictedOrbitsDataSourceIndMsgT_v02, serverList),
  QMI_IDL_TYPE88(0, 25)
};

/*
 * qmiLocGetPredictedOrbitsDataValidityReqMsgT is empty
 * static const uint8_t qmiLocGetPredictedOrbitsDataValidityReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetPredictedOrbitsDataValidityIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetPredictedOrbitsDataValidityIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetPredictedOrbitsDataValidityIndMsgT_v02, validityInfo) - QMI_IDL_OFFSET8(qmiLocGetPredictedOrbitsDataValidityIndMsgT_v02, validityInfo_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetPredictedOrbitsDataValidityIndMsgT_v02, validityInfo),
  QMI_IDL_TYPE88(0, 31)
};

static const uint8_t qmiLocInjectUtcTimeReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectUtcTimeReqMsgT_v02, timeUtc),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectUtcTimeReqMsgT_v02, timeUnc),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectUtcTimeReqMsgT_v02, timeSrc) - QMI_IDL_OFFSET8(qmiLocInjectUtcTimeReqMsgT_v02, timeSrc_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectUtcTimeReqMsgT_v02, timeSrc)
};

static const uint8_t qmiLocInjectUtcTimeIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectUtcTimeIndMsgT_v02, status)
};

static const uint8_t qmiLocInjectPositionReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, latitude) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, latitude_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, latitude),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, longitude) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, longitude_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, longitude),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, horUncCircular) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, horUncCircular_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, horUncCircular),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, horConfidence) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, horConfidence_valid)),
  0x13,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, horConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, horReliability) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, horReliability_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, horReliability),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, altitudeWrtEllipsoid) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, altitudeWrtEllipsoid_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, altitudeWrtEllipsoid),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, altitudeWrtMeanSeaLevel) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, altitudeWrtMeanSeaLevel_valid)),
  0x16,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, altitudeWrtMeanSeaLevel),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, vertUnc) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, vertUnc_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, vertUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, vertConfidence) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, vertConfidence_valid)),
  0x18,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, vertConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, vertReliability) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, vertReliability_valid)),
  0x19,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, vertReliability),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, altSourceInfo) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, altSourceInfo_valid)),
  0x1A,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, altSourceInfo),
  QMI_IDL_TYPE88(0, 32),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, timestampUtc) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, timestampUtc_valid)),
  0x1B,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, timestampUtc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, timestampAge) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, timestampAge_valid)),
  0x1C,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, timestampAge),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, positionSrc) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, positionSrc_valid)),
  0x1D,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, positionSrc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, rawHorUncCircular) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, rawHorUncCircular_valid)),
  0x1E,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, rawHorUncCircular),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, rawHorConfidence) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, rawHorConfidence_valid)),
  0x1F,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, rawHorConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, onDemandCpi) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, onDemandCpi_valid)),
  0x20,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, onDemandCpi),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, positionSrcProvider) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, positionSrcProvider_valid)),
  0x21,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, positionSrcProvider),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, gpsTime) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, gpsTime_valid)),
  0x22,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, gpsTime),
  QMI_IDL_TYPE88(0, 2),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, timeUnc) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, timeUnc_valid)),
  0x23,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, timeUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, velEnu) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, velEnu_valid)),
  0x24,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, velEnu),
  QMI_LOC_ENU_ARRAY_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, velUncEnu) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, velUncEnu_valid)),
  0x25,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, velUncEnu),
  QMI_LOC_ENU_ARRAY_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, expandedGnssSvUsedList) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, expandedGnssSvUsedList_valid)),
  0x26,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, expandedGnssSvUsedList),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, expandedGnssSvUsedList) - QMI_IDL_OFFSET8(qmiLocInjectPositionReqMsgT_v02, expandedGnssSvUsedList_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectPositionReqMsgT_v02, numSvInFix) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectPositionReqMsgT_v02, numSvInFix_valid)),
  0x27,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectPositionReqMsgT_v02, numSvInFix)
};

static const uint8_t qmiLocInjectPositionIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPositionIndMsgT_v02, status)
};

static const uint8_t qmiLocSetEngineLockReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetEngineLockReqMsgT_v02, lockType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetEngineLockReqMsgT_v02, subType) - QMI_IDL_OFFSET8(qmiLocSetEngineLockReqMsgT_v02, subType_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetEngineLockReqMsgT_v02, subType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetEngineLockReqMsgT_v02, lockClient) - QMI_IDL_OFFSET8(qmiLocSetEngineLockReqMsgT_v02, lockClient_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetEngineLockReqMsgT_v02, lockClient),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetEngineLockReqMsgT_v02, clientsConfig) - QMI_IDL_OFFSET8(qmiLocSetEngineLockReqMsgT_v02, clientsConfig_valid)),
  0x12,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetEngineLockReqMsgT_v02, clientsConfig)
};

static const uint8_t qmiLocSetEngineLockIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetEngineLockIndMsgT_v02, status)
};

static const uint8_t qmiLocGetEngineLockReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetEngineLockReqMsgT_v02, subType) - QMI_IDL_OFFSET8(qmiLocGetEngineLockReqMsgT_v02, subType_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetEngineLockReqMsgT_v02, subType)
};

static const uint8_t qmiLocGetEngineLockIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, lockType) - QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, lockType_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, lockType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, subType) - QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, subType_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, subType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, lockClient) - QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, lockClient_valid)),
  0x12,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, lockClient),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, clientsConfig) - QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, clientsConfig_valid)),
  0x13,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, clientsConfig),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, engineLockState) - QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, engineLockState_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetEngineLockIndMsgT_v02, engineLockState)
};

static const uint8_t qmiLocSetSbasConfigReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSbasConfigReqMsgT_v02, sbasConfig),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSbasConfigReqMsgT_v02, qzssL1sConfig) - QMI_IDL_OFFSET8(qmiLocSetSbasConfigReqMsgT_v02, qzssL1sConfig_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSbasConfigReqMsgT_v02, qzssL1sConfig)
};

static const uint8_t qmiLocSetSbasConfigIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSbasConfigIndMsgT_v02, status)
};

/*
 * qmiLocGetSbasConfigReqMsgT is empty
 * static const uint8_t qmiLocGetSbasConfigReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetSbasConfigIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSbasConfigIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSbasConfigIndMsgT_v02, sbasConfig) - QMI_IDL_OFFSET8(qmiLocGetSbasConfigIndMsgT_v02, sbasConfig_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSbasConfigIndMsgT_v02, sbasConfig)
};

static const uint8_t qmiLocSetNmeaTypesReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetNmeaTypesReqMsgT_v02, nmeaSentenceType)
};

static const uint8_t qmiLocSetNmeaTypesIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetNmeaTypesIndMsgT_v02, status)
};

/*
 * qmiLocGetNmeaTypesReqMsgT is empty
 * static const uint8_t qmiLocGetNmeaTypesReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetNmeaTypesIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetNmeaTypesIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetNmeaTypesIndMsgT_v02, nmeaSentenceType) - QMI_IDL_OFFSET8(qmiLocGetNmeaTypesIndMsgT_v02, nmeaSentenceType_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetNmeaTypesIndMsgT_v02, nmeaSentenceType)
};

static const uint8_t qmiLocSetLowPowerModeReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetLowPowerModeReqMsgT_v02, lowPowerMode)
};

static const uint8_t qmiLocSetLowPowerModeIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetLowPowerModeIndMsgT_v02, status)
};

/*
 * qmiLocGetLowPowerModeReqMsgT is empty
 * static const uint8_t qmiLocGetLowPowerModeReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetLowPowerModeIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetLowPowerModeIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetLowPowerModeIndMsgT_v02, lowPowerMode) - QMI_IDL_OFFSET8(qmiLocGetLowPowerModeIndMsgT_v02, lowPowerMode_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetLowPowerModeIndMsgT_v02, lowPowerMode)
};

static const uint8_t qmiLocSetServerReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetServerReqMsgT_v02, serverType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetServerReqMsgT_v02, ipv4Addr) - QMI_IDL_OFFSET8(qmiLocSetServerReqMsgT_v02, ipv4Addr_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocSetServerReqMsgT_v02, ipv4Addr),
  QMI_IDL_TYPE88(0, 13),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetServerReqMsgT_v02, ipv6Addr) - QMI_IDL_OFFSET8(qmiLocSetServerReqMsgT_v02, ipv6Addr_valid)),
  0x11,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocSetServerReqMsgT_v02, ipv6Addr),
  QMI_IDL_TYPE88(0, 14),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetServerReqMsgT_v02, urlAddr) - QMI_IDL_OFFSET8(qmiLocSetServerReqMsgT_v02, urlAddr_valid)),
  0x12,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocSetServerReqMsgT_v02, urlAddr),
  QMI_LOC_MAX_SERVER_ADDR_LENGTH_V02
};

static const uint8_t qmiLocSetServerIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetServerIndMsgT_v02, status)
};

static const uint8_t qmiLocGetServerReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetServerReqMsgT_v02, serverType),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetServerReqMsgT_v02, serverAddrTypeMask) - QMI_IDL_OFFSET8(qmiLocGetServerReqMsgT_v02, serverAddrTypeMask_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetServerReqMsgT_v02, serverAddrTypeMask)
};

static const uint8_t qmiLocGetServerIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetServerIndMsgT_v02, status),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetServerIndMsgT_v02, serverType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetServerIndMsgT_v02, ipv4Addr) - QMI_IDL_OFFSET8(qmiLocGetServerIndMsgT_v02, ipv4Addr_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetServerIndMsgT_v02, ipv4Addr),
  QMI_IDL_TYPE88(0, 13),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetServerIndMsgT_v02, ipv6Addr) - QMI_IDL_OFFSET8(qmiLocGetServerIndMsgT_v02, ipv6Addr_valid)),
  0x11,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetServerIndMsgT_v02, ipv6Addr),
  QMI_IDL_TYPE88(0, 14),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetServerIndMsgT_v02, urlAddr) - QMI_IDL_OFFSET8(qmiLocGetServerIndMsgT_v02, urlAddr_valid)),
  0x12,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocGetServerIndMsgT_v02, urlAddr),
  QMI_LOC_MAX_SERVER_ADDR_LENGTH_V02
};

static const uint8_t qmiLocDeleteAssistDataReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteAssistDataReqMsgT_v02, deleteAllFlag),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocDeleteAssistDataReqMsgT_v02, deleteSvInfoList) - QMI_IDL_OFFSET8(qmiLocDeleteAssistDataReqMsgT_v02, deleteSvInfoList_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocDeleteAssistDataReqMsgT_v02, deleteSvInfoList),
  QMI_LOC_DELETE_MAX_SV_INFO_LENGTH_V02,
  QMI_IDL_OFFSET8(qmiLocDeleteAssistDataReqMsgT_v02, deleteSvInfoList) - QMI_IDL_OFFSET8(qmiLocDeleteAssistDataReqMsgT_v02, deleteSvInfoList_len),
  QMI_IDL_TYPE88(0, 33),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteGnssDataMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteGnssDataMask_valid)),
  0x11,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocDeleteAssistDataReqMsgT_v02, deleteGnssDataMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteCellDbDataMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteCellDbDataMask_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocDeleteAssistDataReqMsgT_v02, deleteCellDbDataMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteClockInfoMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteClockInfoMask_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocDeleteAssistDataReqMsgT_v02, deleteClockInfoMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteBdsSvInfoList) - QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteBdsSvInfoList_valid)),
  0x14,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocDeleteAssistDataReqMsgT_v02, deleteBdsSvInfoList),
  QMI_LOC_DELETE_MAX_BDS_SV_INFO_LENGTH_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteBdsSvInfoList) - QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteBdsSvInfoList_len),
  QMI_IDL_TYPE88(0, 34),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteGalSvInfoList) - QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteGalSvInfoList_valid)),
  0x15,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocDeleteAssistDataReqMsgT_v02, deleteGalSvInfoList),
  QMI_LOC_DELETE_MAX_GAL_SV_INFO_LENGTH_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteGalSvInfoList) - QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteGalSvInfoList_len),
  QMI_IDL_TYPE88(0, 35),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteQzssL1sDataFlag) - QMI_IDL_OFFSET16RELATIVE(qmiLocDeleteAssistDataReqMsgT_v02, deleteQzssL1sDataFlag_valid)),
  0x16,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocDeleteAssistDataReqMsgT_v02, deleteQzssL1sDataFlag)
};

static const uint8_t qmiLocDeleteAssistDataIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteAssistDataIndMsgT_v02, status)
};

static const uint8_t qmiLocSetXtraTSessionControlReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetXtraTSessionControlReqMsgT_v02, xtraTSessionControl)
};

static const uint8_t qmiLocSetXtraTSessionControlIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetXtraTSessionControlIndMsgT_v02, status)
};

/*
 * qmiLocGetXtraTSessionControlReqMsgT is empty
 * static const uint8_t qmiLocGetXtraTSessionControlReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetXtraTSessionControlIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetXtraTSessionControlIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetXtraTSessionControlIndMsgT_v02, xtraTSessionControl) - QMI_IDL_OFFSET8(qmiLocGetXtraTSessionControlIndMsgT_v02, xtraTSessionControl_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetXtraTSessionControlIndMsgT_v02, xtraTSessionControl)
};

static const uint8_t qmiLocInjectWifiPositionReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectWifiPositionReqMsgT_v02, wifiFixTime) - QMI_IDL_OFFSET8(qmiLocInjectWifiPositionReqMsgT_v02, wifiFixTime_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectWifiPositionReqMsgT_v02, wifiFixTime),
  QMI_IDL_TYPE88(0, 36),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectWifiPositionReqMsgT_v02, wifiFixPosition) - QMI_IDL_OFFSET8(qmiLocInjectWifiPositionReqMsgT_v02, wifiFixPosition_valid)),
  0x11,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectWifiPositionReqMsgT_v02, wifiFixPosition),
  QMI_IDL_TYPE88(0, 37),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectWifiPositionReqMsgT_v02, apInfo) - QMI_IDL_OFFSET8(qmiLocInjectWifiPositionReqMsgT_v02, apInfo_valid)),
  0x12,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectWifiPositionReqMsgT_v02, apInfo),
  QMI_LOC_WIFI_MAX_REPORTED_APS_PER_MSG_V02,
  QMI_IDL_OFFSET8(qmiLocInjectWifiPositionReqMsgT_v02, apInfo) - QMI_IDL_OFFSET8(qmiLocInjectWifiPositionReqMsgT_v02, apInfo_len),
  QMI_IDL_TYPE88(0, 38),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiPositionReqMsgT_v02, horizontalReliability) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiPositionReqMsgT_v02, horizontalReliability_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectWifiPositionReqMsgT_v02, horizontalReliability),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiPositionReqMsgT_v02, rawHepe) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiPositionReqMsgT_v02, rawHepe_valid)),
  0x14,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectWifiPositionReqMsgT_v02, rawHepe),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiPositionReqMsgT_v02, wifiApSsidInfo) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiPositionReqMsgT_v02, wifiApSsidInfo_valid)),
  0x15,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectWifiPositionReqMsgT_v02, wifiApSsidInfo),
  QMI_LOC_WIFI_MAX_REPORTED_APS_PER_MSG_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiPositionReqMsgT_v02, wifiApSsidInfo) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiPositionReqMsgT_v02, wifiApSsidInfo_len),
  QMI_IDL_TYPE88(0, 39)
};

static const uint8_t qmiLocInjectWifiPositionIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectWifiPositionIndMsgT_v02, status)
};

static const uint8_t qmiLocNotifyWifiStatusReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNotifyWifiStatusReqMsgT_v02, wifiStatus)
};

static const uint8_t qmiLocNotifyWifiStatusIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNotifyWifiStatusIndMsgT_v02, status)
};

/*
 * qmiLocGetRegisteredEventsReqMsgT is empty
 * static const uint8_t qmiLocGetRegisteredEventsReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetRegisteredEventsIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetRegisteredEventsIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetRegisteredEventsIndMsgT_v02, eventRegMask) - QMI_IDL_OFFSET8(qmiLocGetRegisteredEventsIndMsgT_v02, eventRegMask_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetRegisteredEventsIndMsgT_v02, eventRegMask)
};

static const uint8_t qmiLocSetOperationModeReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetOperationModeReqMsgT_v02, operationMode),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetOperationModeReqMsgT_v02, minInterval) - QMI_IDL_OFFSET8(qmiLocSetOperationModeReqMsgT_v02, minInterval_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetOperationModeReqMsgT_v02, minInterval)
};

static const uint8_t qmiLocSetOperationModeIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetOperationModeIndMsgT_v02, status)
};

/*
 * qmiLocGetOperationModeReqMsgT is empty
 * static const uint8_t qmiLocGetOperationModeReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetOperationModeIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetOperationModeIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetOperationModeIndMsgT_v02, operationMode) - QMI_IDL_OFFSET8(qmiLocGetOperationModeIndMsgT_v02, operationMode_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetOperationModeIndMsgT_v02, operationMode)
};

static const uint8_t qmiLocSetSpiStatusReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSpiStatusReqMsgT_v02, stationary),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSpiStatusReqMsgT_v02, confidenceStationary) - QMI_IDL_OFFSET8(qmiLocSetSpiStatusReqMsgT_v02, confidenceStationary_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSpiStatusReqMsgT_v02, confidenceStationary)
};

static const uint8_t qmiLocSetSpiStatusIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSpiStatusIndMsgT_v02, status)
};

static const uint8_t qmiLocInjectSensorDataReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectSensorDataReqMsgT_v02, opaqueIdentifier) - QMI_IDL_OFFSET8(qmiLocInjectSensorDataReqMsgT_v02, opaqueIdentifier_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSensorDataReqMsgT_v02, opaqueIdentifier),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectSensorDataReqMsgT_v02, threeAxisAccelData) - QMI_IDL_OFFSET8(qmiLocInjectSensorDataReqMsgT_v02, threeAxisAccelData_valid)),
  0x11,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectSensorDataReqMsgT_v02, threeAxisAccelData),
  QMI_IDL_TYPE88(0, 41),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSensorDataReqMsgT_v02, threeAxisGyroData) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSensorDataReqMsgT_v02, threeAxisGyroData_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectSensorDataReqMsgT_v02, threeAxisGyroData),
  QMI_IDL_TYPE88(0, 41),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSensorDataReqMsgT_v02, threeAxisAccelDataTimeSource) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSensorDataReqMsgT_v02, threeAxisAccelDataTimeSource_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectSensorDataReqMsgT_v02, threeAxisAccelDataTimeSource),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSensorDataReqMsgT_v02, threeAxisGyroDataTimeSource) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSensorDataReqMsgT_v02, threeAxisGyroDataTimeSource_valid)),
  0x14,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectSensorDataReqMsgT_v02, threeAxisGyroDataTimeSource),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSensorDataReqMsgT_v02, accelTemperatureData) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSensorDataReqMsgT_v02, accelTemperatureData_valid)),
  0x15,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectSensorDataReqMsgT_v02, accelTemperatureData),
  QMI_IDL_TYPE88(0, 43),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSensorDataReqMsgT_v02, gyroTemperatureData) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSensorDataReqMsgT_v02, gyroTemperatureData_valid)),
  0x16,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectSensorDataReqMsgT_v02, gyroTemperatureData),
  QMI_IDL_TYPE88(0, 43),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSensorDataReqMsgT_v02, threeAxisMagData) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSensorDataReqMsgT_v02, threeAxisMagData_valid)),
  0x17,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectSensorDataReqMsgT_v02, threeAxisMagData),
  QMI_IDL_TYPE88(0, 41),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSensorDataReqMsgT_v02, threeAxisMagDataTimeSource) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSensorDataReqMsgT_v02, threeAxisMagDataTimeSource_valid)),
  0x18,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectSensorDataReqMsgT_v02, threeAxisMagDataTimeSource)
};

static const uint8_t qmiLocInjectSensorDataIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, opaqueIdentifier) - QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, opaqueIdentifier_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, opaqueIdentifier),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, threeAxisAccelSamplesAccepted) - QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, threeAxisAccelSamplesAccepted_valid)),
  0x11,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, threeAxisAccelSamplesAccepted),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, threeAxisGyroSamplesAccepted) - QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, threeAxisGyroSamplesAccepted_valid)),
  0x12,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, threeAxisGyroSamplesAccepted),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, accelTemperatureSamplesAccepted) - QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, accelTemperatureSamplesAccepted_valid)),
  0x13,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, accelTemperatureSamplesAccepted),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, gyroTemperatureSamplesAccepted) - QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, gyroTemperatureSamplesAccepted_valid)),
  0x14,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, gyroTemperatureSamplesAccepted),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, threeAxisMagSamplesAccepted) - QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, threeAxisMagSamplesAccepted_valid)),
  0x15,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSensorDataIndMsgT_v02, threeAxisMagSamplesAccepted)
};

static const uint8_t qmiLocInjectTimeSyncDataReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTimeSyncDataReqMsgT_v02, refCounter),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTimeSyncDataReqMsgT_v02, sensorProcRxTime),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTimeSyncDataReqMsgT_v02, sensorProcTxTime)
};

static const uint8_t qmiLocInjectTimeSyncDataIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTimeSyncDataIndMsgT_v02, status)
};

/*
 * qmiLocGetCradleMountConfigReqMsgT is empty
 * static const uint8_t qmiLocGetCradleMountConfigReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetCradleMountConfigIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetCradleMountConfigIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetCradleMountConfigIndMsgT_v02, cradleMountState) - QMI_IDL_OFFSET8(qmiLocGetCradleMountConfigIndMsgT_v02, cradleMountState_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetCradleMountConfigIndMsgT_v02, cradleMountState),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetCradleMountConfigIndMsgT_v02, confidenceCradleMountState) - QMI_IDL_OFFSET8(qmiLocGetCradleMountConfigIndMsgT_v02, confidenceCradleMountState_valid)),
  0x11,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetCradleMountConfigIndMsgT_v02, confidenceCradleMountState)
};

static const uint8_t qmiLocSetCradleMountConfigReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetCradleMountConfigReqMsgT_v02, cradleMountState),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetCradleMountConfigReqMsgT_v02, confidenceCradleMountState) - QMI_IDL_OFFSET8(qmiLocSetCradleMountConfigReqMsgT_v02, confidenceCradleMountState_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetCradleMountConfigReqMsgT_v02, confidenceCradleMountState)
};

static const uint8_t qmiLocSetCradleMountConfigIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetCradleMountConfigIndMsgT_v02, status)
};

/*
 * qmiLocGetExternalPowerConfigReqMsgT is empty
 * static const uint8_t qmiLocGetExternalPowerConfigReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetExternalPowerConfigIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetExternalPowerConfigIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetExternalPowerConfigIndMsgT_v02, externalPowerState) - QMI_IDL_OFFSET8(qmiLocGetExternalPowerConfigIndMsgT_v02, externalPowerState_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetExternalPowerConfigIndMsgT_v02, externalPowerState)
};

static const uint8_t qmiLocSetExternalPowerConfigReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetExternalPowerConfigReqMsgT_v02, externalPowerState),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetExternalPowerConfigReqMsgT_v02, batteryLevel) - QMI_IDL_OFFSET8(qmiLocSetExternalPowerConfigReqMsgT_v02, batteryLevel_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetExternalPowerConfigReqMsgT_v02, batteryLevel)
};

static const uint8_t qmiLocSetExternalPowerConfigIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetExternalPowerConfigIndMsgT_v02, status)
};

static const uint8_t qmiLocInformLocationServerConnStatusReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInformLocationServerConnStatusReqMsgT_v02, connHandle),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInformLocationServerConnStatusReqMsgT_v02, requestType),

  0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInformLocationServerConnStatusReqMsgT_v02, statusType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInformLocationServerConnStatusReqMsgT_v02, apnProfile) - QMI_IDL_OFFSET8(qmiLocInformLocationServerConnStatusReqMsgT_v02, apnProfile_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInformLocationServerConnStatusReqMsgT_v02, apnProfile),
  QMI_IDL_TYPE88(0, 44),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInformLocationServerConnStatusReqMsgT_v02, apnTypeMask) - QMI_IDL_OFFSET8(qmiLocInformLocationServerConnStatusReqMsgT_v02, apnTypeMask_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInformLocationServerConnStatusReqMsgT_v02, apnTypeMask)
};

static const uint8_t qmiLocInformLocationServerConnStatusIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInformLocationServerConnStatusIndMsgT_v02, status)
};

static const uint8_t qmiLocSetProtocolConfigParametersReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, suplSecurity) - QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, suplSecurity_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, suplSecurity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, vxVersion) - QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, vxVersion_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, vxVersion),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, suplVersion) - QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, suplVersion_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, suplVersion),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, lppConfig) - QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, lppConfig_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, lppConfig),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, assistedGlonassProtocolMask) - QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, assistedGlonassProtocolMask_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, assistedGlonassProtocolMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, suplHashAlgo) - QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, suplHashAlgo_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, suplHashAlgo),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, suplTlsVersion) - QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, suplTlsVersion_valid)),
  0x16,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, suplTlsVersion),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, emergencyProtocol) - QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, emergencyProtocol_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, emergencyProtocol),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, wifiScanInjectTimeout) - QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, wifiScanInjectTimeout_valid)),
  0x18,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, wifiScanInjectTimeout),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, lppeUpConfig) - QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, lppeUpConfig_valid)),
  0x19,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, lppeUpConfig),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, lppeCpConfig) - QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, lppeCpConfig_valid)),
  0x1A,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, lppeCpConfig),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, emergencyCallbackWindow) - QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, emergencyCallbackWindow_valid)),
  0x1B,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersReqMsgT_v02, emergencyCallbackWindow)
};

static const uint8_t qmiLocSetProtocolConfigParametersIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersIndMsgT_v02, failedProtocolConfigParamMask) - QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersIndMsgT_v02, failedProtocolConfigParamMask_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetProtocolConfigParametersIndMsgT_v02, failedProtocolConfigParamMask)
};

static const uint8_t qmiLocGetProtocolConfigParametersReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersReqMsgT_v02, getProtocolConfigParamMask)
};

static const uint8_t qmiLocGetProtocolConfigParametersIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, suplSecurity) - QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, suplSecurity_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, suplSecurity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, vxVersion) - QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, vxVersion_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, vxVersion),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, suplVersion) - QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, suplVersion_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, suplVersion),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, lppConfig) - QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, lppConfig_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, lppConfig),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, assistedGlonassProtocolMask) - QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, assistedGlonassProtocolMask_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, assistedGlonassProtocolMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, suplHashAlgo) - QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, suplHashAlgo_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, suplHashAlgo),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, suplTlsVersion) - QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, suplTlsVersion_valid)),
  0x16,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, suplTlsVersion),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, emergencyProtocol) - QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, emergencyProtocol_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, emergencyProtocol),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, wifiScanInjectTimeout) - QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, wifiScanInjectTimeout_valid)),
  0x18,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, wifiScanInjectTimeout),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, lppeUpConfig) - QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, lppeUpConfig_valid)),
  0x19,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, lppeUpConfig),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, lppeCpConfig) - QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, lppeCpConfig_valid)),
  0x1A,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, lppeCpConfig),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, emergencyCallbackWindow) - QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, emergencyCallbackWindow_valid)),
  0x1B,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetProtocolConfigParametersIndMsgT_v02, emergencyCallbackWindow)
};

static const uint8_t qmiLocSetSensorControlConfigReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorControlConfigReqMsgT_v02, sensorsUsage) - QMI_IDL_OFFSET8(qmiLocSetSensorControlConfigReqMsgT_v02, sensorsUsage_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorControlConfigReqMsgT_v02, sensorsUsage),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorControlConfigReqMsgT_v02, sensorProvider) - QMI_IDL_OFFSET8(qmiLocSetSensorControlConfigReqMsgT_v02, sensorProvider_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorControlConfigReqMsgT_v02, sensorProvider)
};

static const uint8_t qmiLocSetSensorControlConfigIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorControlConfigIndMsgT_v02, status)
};

/*
 * qmiLocGetSensorControlConfigReqMsgT is empty
 * static const uint8_t qmiLocGetSensorControlConfigReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetSensorControlConfigIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorControlConfigIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorControlConfigIndMsgT_v02, sensorsUsage) - QMI_IDL_OFFSET8(qmiLocGetSensorControlConfigIndMsgT_v02, sensorsUsage_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorControlConfigIndMsgT_v02, sensorsUsage),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorControlConfigIndMsgT_v02, sensorProvider) - QMI_IDL_OFFSET8(qmiLocGetSensorControlConfigIndMsgT_v02, sensorProvider_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorControlConfigIndMsgT_v02, sensorProvider)
};

static const uint8_t qmiLocSetSensorPropertiesReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, gyroBiasVarianceRandomWalk) - QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, gyroBiasVarianceRandomWalk_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, gyroBiasVarianceRandomWalk),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, velocityRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, velocityRandomWalkSpectralDensity_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, velocityRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, accelerationRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, accelerationRandomWalkSpectralDensity_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, accelerationRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, angleRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, angleRandomWalkSpectralDensity_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, angleRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, rateRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, rateRandomWalkSpectralDensity_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, rateRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleDataUse) - QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleDataUse_valid)),
  0x15,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleDataUse),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleVelocityRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleVelocityRandomWalkSpectralDensity_valid)),
  0x16,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleVelocityRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleAccelRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleAccelRandomWalkSpectralDensity_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleAccelRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleAngleRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleAngleRandomWalkSpectralDensity_valid)),
  0x18,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleAngleRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleAngularRateRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleAngularRateRandomWalkSpectralDensity_valid)),
  0x19,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleAngularRateRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleOdometryScaleFactorRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleOdometryScaleFactorRandomWalkSpectralDensity_valid)),
  0x1A,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleOdometryScaleFactorRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleOdometryVariance) - QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleOdometryVariance_valid)),
  0x1B,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesReqMsgT_v02, vehicleOdometryVariance)
};

static const uint8_t qmiLocSetSensorPropertiesIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesIndMsgT_v02, failedSensorPropertiesMask) - QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesIndMsgT_v02, failedSensorPropertiesMask_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPropertiesIndMsgT_v02, failedSensorPropertiesMask)
};

static const uint8_t qmiLocGetSensorPropertiesReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesReqMsgT_v02, getSensorPropertiesMask)
};

static const uint8_t qmiLocGetSensorPropertiesIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, gyroBiasVarianceRandomWalk) - QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, gyroBiasVarianceRandomWalk_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, gyroBiasVarianceRandomWalk),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, velocityRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, velocityRandomWalkSpectralDensity_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, velocityRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, accelerationRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, accelerationRandomWalkSpectralDensity_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, accelerationRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, angleRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, angleRandomWalkSpectralDensity_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, angleRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, rateRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, rateRandomWalkSpectralDensity_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, rateRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleDataUse) - QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleDataUse_valid)),
  0x15,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleDataUse),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleVelocityRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleVelocityRandomWalkSpectralDensity_valid)),
  0x16,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleVelocityRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleAccelRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleAccelRandomWalkSpectralDensity_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleAccelRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleAngleRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleAngleRandomWalkSpectralDensity_valid)),
  0x18,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleAngleRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleAngularRateRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleAngularRateRandomWalkSpectralDensity_valid)),
  0x19,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleAngularRateRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleOdometryScaleFactorRandomWalkSpectralDensity) - QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleOdometryScaleFactorRandomWalkSpectralDensity_valid)),
  0x1A,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleOdometryScaleFactorRandomWalkSpectralDensity),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleOdometryVariance) - QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleOdometryVariance_valid)),
  0x1B,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPropertiesIndMsgT_v02, vehicleOdometryVariance)
};

static const uint8_t qmiLocSetSensorPerformanceControlConfigReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, performanceControlMode) - QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, performanceControlMode_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, performanceControlMode),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, accelSamplingSpec) - QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, accelSamplingSpec_valid)),
  0x11,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, accelSamplingSpec),
  QMI_IDL_TYPE88(0, 27),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, gyroSamplingSpec) - QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, gyroSamplingSpec_valid)),
  0x12,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, gyroSamplingSpec),
  QMI_IDL_TYPE88(0, 27),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, algorithmConfig) - QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, algorithmConfig_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, algorithmConfig),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, accelSamplingSpecHigh) - QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, accelSamplingSpecHigh_valid)),
  0x14,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, accelSamplingSpecHigh),
  QMI_IDL_TYPE88(0, 27),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, gyroSamplingSpecHigh) - QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, gyroSamplingSpecHigh_valid)),
  0x15,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02, gyroSamplingSpecHigh),
  QMI_IDL_TYPE88(0, 27)
};

static const uint8_t qmiLocSetSensorPerformanceControlConfigIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigIndMsgT_v02, failedConfiguration) - QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigIndMsgT_v02, failedConfiguration_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSensorPerformanceControlConfigIndMsgT_v02, failedConfiguration)
};

/*
 * qmiLocGetSensorPerformanceControlConfigReqMsgT is empty
 * static const uint8_t qmiLocGetSensorPerformanceControlConfigReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetSensorPerformanceControlConfigIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, performanceControlMode) - QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, performanceControlMode_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, performanceControlMode),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, accelSamplingSpec) - QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, accelSamplingSpec_valid)),
  0x11,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, accelSamplingSpec),
  QMI_IDL_TYPE88(0, 27),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, gyroSamplingSpec) - QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, gyroSamplingSpec_valid)),
  0x12,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, gyroSamplingSpec),
  QMI_IDL_TYPE88(0, 27),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, algorithmConfig) - QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, algorithmConfig_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, algorithmConfig),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, accelSamplingSpecHigh) - QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, accelSamplingSpecHigh_valid)),
  0x14,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, accelSamplingSpecHigh),
  QMI_IDL_TYPE88(0, 27),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, gyroSamplingSpecHigh) - QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, gyroSamplingSpecHigh_valid)),
  0x15,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02, gyroSamplingSpecHigh),
  QMI_IDL_TYPE88(0, 27)
};

static const uint8_t qmiLocInjectSuplCertificateReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSuplCertificateReqMsgT_v02, suplCertId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSuplCertificateReqMsgT_v02, suplCertData),
  ((QMI_LOC_MAX_SUPL_CERT_LENGTH_V02) & 0xFF), ((QMI_LOC_MAX_SUPL_CERT_LENGTH_V02) >> 8),
  QMI_IDL_OFFSET8(qmiLocInjectSuplCertificateReqMsgT_v02, suplCertData) - QMI_IDL_OFFSET8(qmiLocInjectSuplCertificateReqMsgT_v02, suplCertData_len)
};

static const uint8_t qmiLocInjectSuplCertificateIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSuplCertificateIndMsgT_v02, status)
};

static const uint8_t qmiLocDeleteSuplCertificateReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocDeleteSuplCertificateReqMsgT_v02, suplCertId) - QMI_IDL_OFFSET8(qmiLocDeleteSuplCertificateReqMsgT_v02, suplCertId_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteSuplCertificateReqMsgT_v02, suplCertId)
};

static const uint8_t qmiLocDeleteSuplCertificateIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteSuplCertificateIndMsgT_v02, status)
};

static const uint8_t qmiLocSetPositionEngineConfigParametersReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersReqMsgT_v02, injectedPositionControl) - QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersReqMsgT_v02, injectedPositionControl_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersReqMsgT_v02, injectedPositionControl),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersReqMsgT_v02, filterSvUsage) - QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersReqMsgT_v02, filterSvUsage_valid)),
  0x11,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersReqMsgT_v02, filterSvUsage),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersReqMsgT_v02, storeAssistData) - QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersReqMsgT_v02, storeAssistData_valid)),
  0x12,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersReqMsgT_v02, storeAssistData),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersReqMsgT_v02, enableFasterTTFF) - QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersReqMsgT_v02, enableFasterTTFF_valid)),
  0x13,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersReqMsgT_v02, enableFasterTTFF)
};

static const uint8_t qmiLocSetPositionEngineConfigParametersIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersIndMsgT_v02, failedPositionEngineConfigParamMask) - QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersIndMsgT_v02, failedPositionEngineConfigParamMask_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetPositionEngineConfigParametersIndMsgT_v02, failedPositionEngineConfigParamMask)
};

static const uint8_t qmiLocGetPositionEngineConfigParametersReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetPositionEngineConfigParametersReqMsgT_v02, getPositionEngineConfigParamMask)
};

static const uint8_t qmiLocGetPositionEngineConfigParametersIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetPositionEngineConfigParametersIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetPositionEngineConfigParametersIndMsgT_v02, injectedPositionControl) - QMI_IDL_OFFSET8(qmiLocGetPositionEngineConfigParametersIndMsgT_v02, injectedPositionControl_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetPositionEngineConfigParametersIndMsgT_v02, injectedPositionControl),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetPositionEngineConfigParametersIndMsgT_v02, filterSvUsage) - QMI_IDL_OFFSET8(qmiLocGetPositionEngineConfigParametersIndMsgT_v02, filterSvUsage_valid)),
  0x11,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetPositionEngineConfigParametersIndMsgT_v02, filterSvUsage),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetPositionEngineConfigParametersIndMsgT_v02, storeAssistData) - QMI_IDL_OFFSET8(qmiLocGetPositionEngineConfigParametersIndMsgT_v02, storeAssistData_valid)),
  0x12,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetPositionEngineConfigParametersIndMsgT_v02, storeAssistData),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetPositionEngineConfigParametersIndMsgT_v02, enableFasterTTFF) - QMI_IDL_OFFSET8(qmiLocGetPositionEngineConfigParametersIndMsgT_v02, enableFasterTTFF_valid)),
  0x13,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetPositionEngineConfigParametersIndMsgT_v02, enableFasterTTFF)
};

static const uint8_t qmiLocAddCircularGeofenceReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, transactionId),

  0x02,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, circularGeofenceArgs),
  QMI_IDL_TYPE88(0, 45),

  0x03,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, breachMask),

  0x04,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, includePosition),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, responsiveness) - QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, responsiveness_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, responsiveness),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, confidence) - QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, confidence_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, confidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, customResponsivenessValue) - QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, customResponsivenessValue_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, customResponsivenessValue),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, dwellTime) - QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, dwellTime_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, dwellTime),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, dwellTypeMask) - QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, dwellTypeMask_valid)),
  0x14,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceReqMsgT_v02, dwellTypeMask)
};

static const uint8_t qmiLocAddCircularGeofenceIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceIndMsgT_v02, transactionId) - QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceIndMsgT_v02, transactionId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceIndMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceIndMsgT_v02, geofenceId) - QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceIndMsgT_v02, geofenceId_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddCircularGeofenceIndMsgT_v02, geofenceId)
};

static const uint8_t qmiLocDeleteGeofenceReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGeofenceReqMsgT_v02, geofenceId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGeofenceReqMsgT_v02, transactionId)
};

static const uint8_t qmiLocDeleteGeofenceIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGeofenceIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocDeleteGeofenceIndMsgT_v02, geofenceId) - QMI_IDL_OFFSET8(qmiLocDeleteGeofenceIndMsgT_v02, geofenceId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGeofenceIndMsgT_v02, geofenceId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocDeleteGeofenceIndMsgT_v02, transactionId) - QMI_IDL_OFFSET8(qmiLocDeleteGeofenceIndMsgT_v02, transactionId_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGeofenceIndMsgT_v02, transactionId)
};

static const uint8_t qmiLocQueryGeofenceReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryGeofenceReqMsgT_v02, geofenceId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryGeofenceReqMsgT_v02, transactionId)
};

static const uint8_t qmiLocQueryGeofenceIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, geofenceId) - QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, geofenceId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, geofenceId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, transactionId) - QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, transactionId_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, geofenceOrigin) - QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, geofenceOrigin_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, geofenceOrigin),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, posWrtGeofence) - QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, posWrtGeofence_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, posWrtGeofence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, circularGeofenceArgs) - QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, circularGeofenceArgs_valid)),
  0x14,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, circularGeofenceArgs),
  QMI_IDL_TYPE88(0, 45),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, geofenceState) - QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, geofenceState_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryGeofenceIndMsgT_v02, geofenceState)
};

static const uint8_t qmiLocSetGeofenceEngineConfigReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, gnssUnavailableIndicationTimeout) - QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, gnssUnavailableIndicationTimeout_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, gnssUnavailableIndicationTimeout),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, maxGeofences) - QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, maxGeofences_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, maxGeofences),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, enableMotionDetectionSources) - QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, enableMotionDetectionSources_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, enableMotionDetectionSources),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, enableCpiUsage) - QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, enableCpiUsage_valid)),
  0x13,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, enableCpiUsage),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, gnssPositionSessionTimeout) - QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, gnssPositionSessionTimeout_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, gnssPositionSessionTimeout),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, gnssPositionMaxPuncAcceptable) - QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, gnssPositionMaxPuncAcceptable_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, gnssPositionMaxPuncAcceptable),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, mediumResponsivenessValue) - QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, mediumResponsivenessValue_valid)),
  0x16,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, mediumResponsivenessValue),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, chalGnssEnvMinCpiWaitInterval) - QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, chalGnssEnvMinCpiWaitInterval_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, chalGnssEnvMinCpiWaitInterval),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, motionStateInfo) - QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, motionStateInfo_valid)),
  0x18,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, motionStateInfo),
  QMI_LOC_GEOFENCE_MAX_MOTION_STATES_V02,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, motionStateInfo) - QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigReqMsgT_v02, motionStateInfo_len),
  QMI_IDL_TYPE88(0, 46)
};

static const uint8_t qmiLocSetGeofenceEngineConfigIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigIndMsgT_v02, transactionId) - QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigIndMsgT_v02, transactionId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineConfigIndMsgT_v02, transactionId)
};

static const uint8_t qmiLocGetGeofenceEngineConfigReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigReqMsgT_v02, transactionId)
};

static const uint8_t qmiLocGetGeofenceEngineConfigIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, transactionId) - QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, transactionId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, gnssUnavailableIndicationTimeout) - QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, gnssUnavailableIndicationTimeout_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, gnssUnavailableIndicationTimeout),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, maxGeofences) - QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, maxGeofences_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, maxGeofences),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, enabledMotionDetectionSources) - QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, enabledMotionDetectionSources_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, enabledMotionDetectionSources),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, enabledCpiUsage) - QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, enabledCpiUsage_valid)),
  0x14,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetGeofenceEngineConfigIndMsgT_v02, enabledCpiUsage)
};

static const uint8_t qmiLocEditGeofenceReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEditGeofenceReqMsgT_v02, geofenceId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEditGeofenceReqMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEditGeofenceReqMsgT_v02, geofenceState) - QMI_IDL_OFFSET8(qmiLocEditGeofenceReqMsgT_v02, geofenceState_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEditGeofenceReqMsgT_v02, geofenceState),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEditGeofenceReqMsgT_v02, breachMask) - QMI_IDL_OFFSET8(qmiLocEditGeofenceReqMsgT_v02, breachMask_valid)),
  0x11,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEditGeofenceReqMsgT_v02, breachMask),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEditGeofenceReqMsgT_v02, responsiveness) - QMI_IDL_OFFSET8(qmiLocEditGeofenceReqMsgT_v02, responsiveness_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEditGeofenceReqMsgT_v02, responsiveness)
};

static const uint8_t qmiLocEditGeofenceIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEditGeofenceIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEditGeofenceIndMsgT_v02, geofenceId) - QMI_IDL_OFFSET8(qmiLocEditGeofenceIndMsgT_v02, geofenceId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEditGeofenceIndMsgT_v02, geofenceId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEditGeofenceIndMsgT_v02, transactionId) - QMI_IDL_OFFSET8(qmiLocEditGeofenceIndMsgT_v02, transactionId_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEditGeofenceIndMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEditGeofenceIndMsgT_v02, failedParams) - QMI_IDL_OFFSET8(qmiLocEditGeofenceIndMsgT_v02, failedParams_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEditGeofenceIndMsgT_v02, failedParams)
};

static const uint8_t qmiLocEventGetTimeZoneReqIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGetTimeZoneReqIndMsgT_v02, status)
};

static const uint8_t qmiLocInjectTimeZoneInfoReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTimeZoneInfoReqMsgT_v02, timeUtc),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectTimeZoneInfoReqMsgT_v02, timeZone),
  QMI_IDL_TYPE88(0, 47)
};

static const uint8_t qmiLocInjectTimeZoneInfoIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTimeZoneInfoIndMsgT_v02, status)
};

static const uint8_t qmiLocGetBestAvailablePositionReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionReqMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionReqMsgT_v02, specialReqType) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionReqMsgT_v02, specialReqType_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionReqMsgT_v02, specialReqType)
};

static const uint8_t qmiLocGetBestAvailablePositionIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, transactionId) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, transactionId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, latitude) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, latitude_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, latitude),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, longitude) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, longitude_valid)),
  0x12,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, longitude),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horUncCircular) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horUncCircular_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horUncCircular),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, altitudeWrtEllipsoid) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, altitudeWrtEllipsoid_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, altitudeWrtEllipsoid),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertUnc) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertUnc_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, timestampUtc) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, timestampUtc_valid)),
  0x16,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, timestampUtc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, timeUnc) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, timeUnc_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, timeUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horUncEllipseSemiMinor) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horUncEllipseSemiMinor_valid)),
  0x18,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horUncEllipseSemiMinor),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horUncEllipseSemiMajor) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horUncEllipseSemiMajor_valid)),
  0x19,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horUncEllipseSemiMajor),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horUncEllipseOrientAzimuth) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horUncEllipseOrientAzimuth_valid)),
  0x1A,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horUncEllipseOrientAzimuth),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horCircularConfidence) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horCircularConfidence_valid)),
  0x1B,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horCircularConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horEllipticalConfidence) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horEllipticalConfidence_valid)),
  0x1C,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horEllipticalConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horReliability) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horReliability_valid)),
  0x1D,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horReliability),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horSpeed) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horSpeed_valid)),
  0x1E,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horSpeed),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horSpeedUnc) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horSpeedUnc_valid)),
  0x1F,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, horSpeedUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, altitudeWrtMeanSeaLevel) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, altitudeWrtMeanSeaLevel_valid)),
  0x20,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, altitudeWrtMeanSeaLevel),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertConfidence) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertConfidence_valid)),
  0x21,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertReliability) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertReliability_valid)),
  0x22,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertReliability),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertSpeed) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertSpeed_valid)),
  0x23,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertSpeed),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertSpeedUnc) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertSpeedUnc_valid)),
  0x24,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, vertSpeedUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, heading) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, heading_valid)),
  0x25,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, heading),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, headingUnc) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, headingUnc_valid)),
  0x26,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, headingUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, magneticDeviation) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, magneticDeviation_valid)),
  0x27,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, magneticDeviation),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, technologyMask) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, technologyMask_valid)),
  0x28,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, technologyMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, DOP) - QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, DOP_valid)),
  0x29,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetBestAvailablePositionIndMsgT_v02, DOP),
  QMI_IDL_TYPE88(0, 3),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, gpsTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, gpsTime_valid)),
  0x2A,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetBestAvailablePositionIndMsgT_v02, gpsTime),
  QMI_IDL_TYPE88(0, 2),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, timeSrc) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, timeSrc_valid)),
  0x2B,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetBestAvailablePositionIndMsgT_v02, timeSrc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, sensorDataUsage) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, sensorDataUsage_valid)),
  0x2C,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetBestAvailablePositionIndMsgT_v02, sensorDataUsage),
  QMI_IDL_TYPE88(0, 5),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, gnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, gnssSvUsedList_valid)),
  0x2D,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetBestAvailablePositionIndMsgT_v02, gnssSvUsedList),
  QMI_LOC_MAX_SV_USED_LIST_LENGTH_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, gnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, gnssSvUsedList_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, extDOP) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, extDOP_valid)),
  0x2E,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetBestAvailablePositionIndMsgT_v02, extDOP),
  QMI_IDL_TYPE88(0, 4),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, spoofReportMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, spoofReportMask_valid)),
  0x2F,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetBestAvailablePositionIndMsgT_v02, spoofReportMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, expandedGnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, expandedGnssSvUsedList_valid)),
  0x30,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetBestAvailablePositionIndMsgT_v02, expandedGnssSvUsedList),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, expandedGnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, expandedGnssSvUsedList_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, gnssSvUsedSignalTypeList) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, gnssSvUsedSignalTypeList_valid)),
  0x31,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetBestAvailablePositionIndMsgT_v02, gnssSvUsedSignalTypeList),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, gnssSvUsedSignalTypeList) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, gnssSvUsedSignalTypeList_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, conformityIndex) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, conformityIndex_valid)),
  0x32,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetBestAvailablePositionIndMsgT_v02, conformityIndex),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, gnssTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetBestAvailablePositionIndMsgT_v02, gnssTime_valid)),
  0x33,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetBestAvailablePositionIndMsgT_v02, gnssTime),
  QMI_IDL_TYPE88(0, 62)
};

static const uint8_t qmiLocInjectMotionDataReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectMotionDataReqMsgT_v02, motion_data),
  QMI_IDL_TYPE88(0, 48)
};

static const uint8_t qmiLocInjectMotionDataIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectMotionDataIndMsgT_v02, status)
};

static const uint8_t qmiLocGetNiGeofenceIdListReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetNiGeofenceIdListReqMsgT_v02, transactionId)
};

static const uint8_t qmiLocGetNiGeofenceIdListIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetNiGeofenceIdListIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetNiGeofenceIdListIndMsgT_v02, transactionId) - QMI_IDL_OFFSET8(qmiLocGetNiGeofenceIdListIndMsgT_v02, transactionId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetNiGeofenceIdListIndMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetNiGeofenceIdListIndMsgT_v02, niGeofenceIdList) - QMI_IDL_OFFSET8(qmiLocGetNiGeofenceIdListIndMsgT_v02, niGeofenceIdList_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetNiGeofenceIdListIndMsgT_v02, niGeofenceIdList),
  QMI_LOC_MAX_NI_GEOFENCE_ID_LIST_LENGTH_V02,
  QMI_IDL_OFFSET8(qmiLocGetNiGeofenceIdListIndMsgT_v02, niGeofenceIdList) - QMI_IDL_OFFSET8(qmiLocGetNiGeofenceIdListIndMsgT_v02, niGeofenceIdList_len)
};

static const uint8_t qmiLocInjectGSMCellInfoReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectGSMCellInfoReqMsgT_v02, gsmCellId),
  QMI_IDL_TYPE88(0, 49),

  0x02,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectGSMCellInfoReqMsgT_v02, roamingStatus),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectGSMCellInfoReqMsgT_v02, timingAdvance) - QMI_IDL_OFFSET8(qmiLocInjectGSMCellInfoReqMsgT_v02, timingAdvance_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectGSMCellInfoReqMsgT_v02, timingAdvance)
};

static const uint8_t qmiLocInjectGSMCellInfoIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectGSMCellInfoIndMsgT_v02, status)
};

static const uint8_t qmiLocInjectWCDMACellInfoReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectWCDMACellInfoReqMsgT_v02, wcdmaCellId),
  QMI_IDL_TYPE88(0, 50),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectWCDMACellInfoReqMsgT_v02, roamingStatus),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectWCDMACellInfoReqMsgT_v02, freq) - QMI_IDL_OFFSET8(qmiLocInjectWCDMACellInfoReqMsgT_v02, freq_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectWCDMACellInfoReqMsgT_v02, freq),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectWCDMACellInfoReqMsgT_v02, psc) - QMI_IDL_OFFSET8(qmiLocInjectWCDMACellInfoReqMsgT_v02, psc_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectWCDMACellInfoReqMsgT_v02, psc)
};

static const uint8_t qmiLocInjectWCDMACellInfoIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectWCDMACellInfoIndMsgT_v02, status)
};

static const uint8_t qmiLocInjectTDSCDMACellInfoReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectTDSCDMACellInfoReqMsgT_v02, tdscdmaCellId),
  QMI_IDL_TYPE88(0, 51),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTDSCDMACellInfoReqMsgT_v02, roamingStatus),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectTDSCDMACellInfoReqMsgT_v02, freq) - QMI_IDL_OFFSET8(qmiLocInjectTDSCDMACellInfoReqMsgT_v02, freq_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTDSCDMACellInfoReqMsgT_v02, freq)
};

static const uint8_t qmiLocInjectTDSCDMACellInfoIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTDSCDMACellInfoIndMsgT_v02, status)
};

static const uint8_t qmiLocInjectSubscriberIDReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectSubscriberIDReqMsgT_v02, preferredIMSI) - QMI_IDL_OFFSET8(qmiLocInjectSubscriberIDReqMsgT_v02, preferredIMSI_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSubscriberIDReqMsgT_v02, preferredIMSI),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectSubscriberIDReqMsgT_v02, preferredMSISDN) - QMI_IDL_OFFSET8(qmiLocInjectSubscriberIDReqMsgT_v02, preferredMSISDN_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSubscriberIDReqMsgT_v02, preferredMSISDN)
};

static const uint8_t qmiLocInjectSubscriberIDIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSubscriberIDIndMsgT_v02, status)
};

static const uint8_t qmiLocInjectNetworkInitiatedMessageReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectNetworkInitiatedMessageReqMsgT_v02, injectedNIMessageType),

  0x02,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectNetworkInitiatedMessageReqMsgT_v02, injectedNIMessage),
  ((QMI_LOC_MAX_INJECTED_NETWORK_INITIATED_MESSAGE_LENGTH_V02) & 0xFF), ((QMI_LOC_MAX_INJECTED_NETWORK_INITIATED_MESSAGE_LENGTH_V02) >> 8),
  QMI_IDL_OFFSET8(qmiLocInjectNetworkInitiatedMessageReqMsgT_v02, injectedNIMessage) - QMI_IDL_OFFSET8(qmiLocInjectNetworkInitiatedMessageReqMsgT_v02, injectedNIMessage_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectNetworkInitiatedMessageReqMsgT_v02, subId) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectNetworkInitiatedMessageReqMsgT_v02, subId_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectNetworkInitiatedMessageReqMsgT_v02, subId)
};

static const uint8_t qmiLocInjectNetworkInitiatedMessageIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectNetworkInitiatedMessageIndMsgT_v02, status)
};

/*
 * qmiLocWWANOutOfServiceNotificationReqMsgT is empty
 * static const uint8_t qmiLocWWANOutOfServiceNotificationReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocWWANOutOfServiceNotificationIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocWWANOutOfServiceNotificationIndMsgT_v02, status)
};

static const uint8_t qmiLocPedometerReportReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPedometerReportReqMsgT_v02, timeSource),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPedometerReportReqMsgT_v02, timestamp),

  0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPedometerReportReqMsgT_v02, timeInterval),

  0x04,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPedometerReportReqMsgT_v02, stepCount),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocPedometerReportReqMsgT_v02, stepConfidence) - QMI_IDL_OFFSET8(qmiLocPedometerReportReqMsgT_v02, stepConfidence_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocPedometerReportReqMsgT_v02, stepConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocPedometerReportReqMsgT_v02, stepCountUncertainty) - QMI_IDL_OFFSET8(qmiLocPedometerReportReqMsgT_v02, stepCountUncertainty_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPedometerReportReqMsgT_v02, stepCountUncertainty),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocPedometerReportReqMsgT_v02, stepRate) - QMI_IDL_OFFSET8(qmiLocPedometerReportReqMsgT_v02, stepRate_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPedometerReportReqMsgT_v02, stepRate)
};

static const uint8_t qmiLocPedometerReportIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPedometerReportIndMsgT_v02, status)
};

static const uint8_t qmiLocGetBatchSizeReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBatchSizeReqMsgT_v02, transactionId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBatchSizeReqMsgT_v02, batchSize),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBatchSizeReqMsgT_v02, batchType) - QMI_IDL_OFFSET8(qmiLocGetBatchSizeReqMsgT_v02, batchType_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBatchSizeReqMsgT_v02, batchType)
};

static const uint8_t qmiLocGetBatchSizeIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBatchSizeIndMsgT_v02, status),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBatchSizeIndMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBatchSizeIndMsgT_v02, batchSize)
};

static const uint8_t qmiLocStartBatchingReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, minInterval) - QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, minInterval_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, minInterval),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, horizontalAccuracyLevel) - QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, horizontalAccuracyLevel_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, horizontalAccuracyLevel),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, fixSessionTimeout) - QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, fixSessionTimeout_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, fixSessionTimeout),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, minDistance) - QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, minDistance_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, minDistance),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, batchAllPos) - QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, batchAllPos_valid)),
  0x14,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, batchAllPos),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, requestId) - QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, requestId_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartBatchingReqMsgT_v02, requestId)
};

static const uint8_t qmiLocStartBatchingIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartBatchingIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartBatchingIndMsgT_v02, requestId) - QMI_IDL_OFFSET8(qmiLocStartBatchingIndMsgT_v02, requestId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartBatchingIndMsgT_v02, requestId)
};

static const uint8_t qmiLocEventBatchFullIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventBatchFullIndMsgT_v02, batchCount),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventBatchFullIndMsgT_v02, batchType) - QMI_IDL_OFFSET8(qmiLocEventBatchFullIndMsgT_v02, batchType_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventBatchFullIndMsgT_v02, batchType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventBatchFullIndMsgT_v02, accumulatedDistance) - QMI_IDL_OFFSET8(qmiLocEventBatchFullIndMsgT_v02, accumulatedDistance_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventBatchFullIndMsgT_v02, accumulatedDistance),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventBatchFullIndMsgT_v02, batchedPosition) - QMI_IDL_OFFSET8(qmiLocEventBatchFullIndMsgT_v02, batchedPosition_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventBatchFullIndMsgT_v02, batchedPosition)
};

static const uint8_t qmiLocEventLiveBatchedPositionReportIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventLiveBatchedPositionReportIndMsgT_v02, liveBatchedReport),
  QMI_IDL_TYPE88(0, 52)
};

static const uint8_t qmiLocReadFromBatchReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocReadFromBatchReqMsgT_v02, numberOfEntries),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocReadFromBatchReqMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocReadFromBatchReqMsgT_v02, batchType) - QMI_IDL_OFFSET8(qmiLocReadFromBatchReqMsgT_v02, batchType_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocReadFromBatchReqMsgT_v02, batchType)
};

static const uint8_t qmiLocReadFromBatchIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocReadFromBatchIndMsgT_v02, status),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocReadFromBatchIndMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocReadFromBatchIndMsgT_v02, numberOfEntries) - QMI_IDL_OFFSET8(qmiLocReadFromBatchIndMsgT_v02, numberOfEntries_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocReadFromBatchIndMsgT_v02, numberOfEntries),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocReadFromBatchIndMsgT_v02, batchedReportList) - QMI_IDL_OFFSET8(qmiLocReadFromBatchIndMsgT_v02, batchedReportList_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocReadFromBatchIndMsgT_v02, batchedReportList),
  QMI_LOC_READ_FROM_BATCH_MAX_SIZE_V02,
  QMI_IDL_OFFSET8(qmiLocReadFromBatchIndMsgT_v02, batchedReportList) - QMI_IDL_OFFSET8(qmiLocReadFromBatchIndMsgT_v02, batchedReportList_len),
  QMI_IDL_TYPE88(0, 52)
};

static const uint8_t qmiLocStopBatchingReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStopBatchingReqMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStopBatchingReqMsgT_v02, requestId) - QMI_IDL_OFFSET8(qmiLocStopBatchingReqMsgT_v02, requestId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStopBatchingReqMsgT_v02, requestId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStopBatchingReqMsgT_v02, batchType) - QMI_IDL_OFFSET8(qmiLocStopBatchingReqMsgT_v02, batchType_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStopBatchingReqMsgT_v02, batchType)
};

static const uint8_t qmiLocStopBatchingIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStopBatchingIndMsgT_v02, status),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStopBatchingIndMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStopBatchingIndMsgT_v02, requestId) - QMI_IDL_OFFSET8(qmiLocStopBatchingIndMsgT_v02, requestId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStopBatchingIndMsgT_v02, requestId)
};

static const uint8_t qmiLocReleaseBatchReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocReleaseBatchReqMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocReleaseBatchReqMsgT_v02, batchType) - QMI_IDL_OFFSET8(qmiLocReleaseBatchReqMsgT_v02, batchType_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocReleaseBatchReqMsgT_v02, batchType)
};

static const uint8_t qmiLocReleaseBatchIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocReleaseBatchIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocReleaseBatchIndMsgT_v02, transactionId)
};

static const uint8_t qmiLocEventInjectWifiApDataReqIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectWifiApDataReqIndMsgT_v02, e911Mode) - QMI_IDL_OFFSET8(qmiLocEventInjectWifiApDataReqIndMsgT_v02, e911Mode_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectWifiApDataReqIndMsgT_v02, e911Mode)
};

static const uint8_t qmiLocInjectWifiApDataReqMsgT_data_v02[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectWifiApDataReqMsgT_v02, wifiApInfo),
  QMI_LOC_WIFI_MAX_REPORTED_APS_PER_MSG_V02,
  QMI_IDL_OFFSET8(qmiLocInjectWifiApDataReqMsgT_v02, wifiApInfo) - QMI_IDL_OFFSET8(qmiLocInjectWifiApDataReqMsgT_v02, wifiApInfo_len),
  QMI_IDL_TYPE88(0, 53),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiApDataReqMsgT_v02, errorCause) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiApDataReqMsgT_v02, errorCause_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectWifiApDataReqMsgT_v02, errorCause),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiApDataReqMsgT_v02, requestTimestamp) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiApDataReqMsgT_v02, requestTimestamp_valid)),
  0x11,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectWifiApDataReqMsgT_v02, requestTimestamp),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiApDataReqMsgT_v02, receiveTimestamp) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiApDataReqMsgT_v02, receiveTimestamp_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectWifiApDataReqMsgT_v02, receiveTimestamp),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiApDataReqMsgT_v02, onDemandScan) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiApDataReqMsgT_v02, onDemandScan_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectWifiApDataReqMsgT_v02, onDemandScan),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiApDataReqMsgT_v02, wifiApInfoA) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiApDataReqMsgT_v02, wifiApInfoA_valid)),
  0x14,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectWifiApDataReqMsgT_v02, wifiApInfoA),
  QMI_LOC_WIFI_MAX_REPORTED_APS_PER_MSG_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiApDataReqMsgT_v02, wifiApInfoA) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiApDataReqMsgT_v02, wifiApInfoA_len),
  QMI_IDL_TYPE88(0, 54),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiApDataReqMsgT_v02, ueMacAddress) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectWifiApDataReqMsgT_v02, ueMacAddress_valid)),
  0x15,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectWifiApDataReqMsgT_v02, ueMacAddress),
  QMI_LOC_WIFI_MAC_ADDR_LENGTH_V02
};

static const uint8_t qmiLocInjectWifiApDataIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectWifiApDataIndMsgT_v02, status)
};

static const uint8_t qmiLocNotifyWifiAttachmentStatusReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNotifyWifiAttachmentStatusReqMsgT_v02, attachState),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocNotifyWifiAttachmentStatusReqMsgT_v02, accessPointMacAddress) - QMI_IDL_OFFSET8(qmiLocNotifyWifiAttachmentStatusReqMsgT_v02, accessPointMacAddress_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocNotifyWifiAttachmentStatusReqMsgT_v02, accessPointMacAddress),
  QMI_LOC_WIFI_MAC_ADDR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocNotifyWifiAttachmentStatusReqMsgT_v02, wifiApSsid) - QMI_IDL_OFFSET8(qmiLocNotifyWifiAttachmentStatusReqMsgT_v02, wifiApSsid_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocNotifyWifiAttachmentStatusReqMsgT_v02, wifiApSsid),
  QMI_LOC_MAX_WIFI_AP_SSID_STR_LENGTH_V02
};

static const uint8_t qmiLocNotifyWifiAttachmentStatusIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNotifyWifiAttachmentStatusIndMsgT_v02, status)
};

static const uint8_t qmiLocNotifyWifiEnabledStatusReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNotifyWifiEnabledStatusReqMsgT_v02, enabledStatus)
};

static const uint8_t qmiLocNotifyWifiEnabledStatusIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocNotifyWifiEnabledStatusIndMsgT_v02, status)
};

static const uint8_t qmiLocEventVehicleDataReadyIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventVehicleDataReadyIndMsgT_v02, vehicleAccelReadyStatus) - QMI_IDL_OFFSET8(qmiLocEventVehicleDataReadyIndMsgT_v02, vehicleAccelReadyStatus_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventVehicleDataReadyIndMsgT_v02, vehicleAccelReadyStatus),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventVehicleDataReadyIndMsgT_v02, vehicleAngularRateReadyStatus) - QMI_IDL_OFFSET8(qmiLocEventVehicleDataReadyIndMsgT_v02, vehicleAngularRateReadyStatus_valid)),
  0x11,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventVehicleDataReadyIndMsgT_v02, vehicleAngularRateReadyStatus),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventVehicleDataReadyIndMsgT_v02, vehicleOdometryReadyStatus) - QMI_IDL_OFFSET8(qmiLocEventVehicleDataReadyIndMsgT_v02, vehicleOdometryReadyStatus_valid)),
  0x12,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventVehicleDataReadyIndMsgT_v02, vehicleOdometryReadyStatus)
};

static const uint8_t qmiLocInjectVehicleSensorDataReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectVehicleSensorDataReqMsgT_v02, accelData) - QMI_IDL_OFFSET8(qmiLocInjectVehicleSensorDataReqMsgT_v02, accelData_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectVehicleSensorDataReqMsgT_v02, accelData),
  QMI_IDL_TYPE88(0, 56),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectVehicleSensorDataReqMsgT_v02, angRotationData) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectVehicleSensorDataReqMsgT_v02, angRotationData_valid)),
  0x11,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectVehicleSensorDataReqMsgT_v02, angRotationData),
  QMI_IDL_TYPE88(0, 56),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectVehicleSensorDataReqMsgT_v02, odometryData) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectVehicleSensorDataReqMsgT_v02, odometryData_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectVehicleSensorDataReqMsgT_v02, odometryData),
  QMI_IDL_TYPE88(0, 58),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectVehicleSensorDataReqMsgT_v02, changeInTimeScales) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectVehicleSensorDataReqMsgT_v02, changeInTimeScales_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectVehicleSensorDataReqMsgT_v02, changeInTimeScales)
};

static const uint8_t qmiLocInjectVehicleSensorDataIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectVehicleSensorDataIndMsgT_v02, status)
};

static const uint8_t qmiLocGetAvailWwanPositionReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionReqMsgT_v02, transactionId)
};

static const uint8_t qmiLocGetAvailWwanPositionIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, transactionId) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, transactionId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, latitude) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, latitude_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, latitude),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, longitude) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, longitude_valid)),
  0x12,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, longitude),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horUncCircular) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horUncCircular_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horUncCircular),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, altitudeWrtEllipsoid) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, altitudeWrtEllipsoid_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, altitudeWrtEllipsoid),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, vertUnc) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, vertUnc_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, vertUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, timestampUtc) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, timestampUtc_valid)),
  0x16,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, timestampUtc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, timeUnc) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, timeUnc_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, timeUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horUncEllipseSemiMinor) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horUncEllipseSemiMinor_valid)),
  0x18,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horUncEllipseSemiMinor),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horUncEllipseSemiMajor) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horUncEllipseSemiMajor_valid)),
  0x19,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horUncEllipseSemiMajor),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horUncEllipseOrientAzimuth) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horUncEllipseOrientAzimuth_valid)),
  0x1A,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horUncEllipseOrientAzimuth),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horCircularConfidence) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horCircularConfidence_valid)),
  0x1B,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horCircularConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horEllipticalConfidence) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horEllipticalConfidence_valid)),
  0x1C,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horEllipticalConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horReliability) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horReliability_valid)),
  0x1D,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, horReliability),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, altitudeWrtMeanSeaLevel) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, altitudeWrtMeanSeaLevel_valid)),
  0x1E,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, altitudeWrtMeanSeaLevel),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, vertConfidence) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, vertConfidence_valid)),
  0x1F,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, vertConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, vertReliability) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, vertReliability_valid)),
  0x20,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, vertReliability),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, gpsTime) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, gpsTime_valid)),
  0x21,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, gpsTime),
  QMI_IDL_TYPE88(0, 2),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, timeSrc) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, timeSrc_valid)),
  0x22,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, timeSrc),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, gnssTime) - QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, gnssTime_valid)),
  0x23,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetAvailWwanPositionIndMsgT_v02, gnssTime),
  QMI_IDL_TYPE88(0, 62)
};

static const uint8_t qmiLocSetPremiumServicesCfgReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetPremiumServicesCfgReqMsgT_v02, premiumServiceType),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetPremiumServicesCfgReqMsgT_v02, premiumServiceCfg)
};

static const uint8_t qmiLocSetPremiumServicesCfgIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetPremiumServicesCfgIndMsgT_v02, status)
};

static const uint8_t qmiLocSetXtraVersionCheckReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetXtraVersionCheckReqMsgT_v02, xtraVersionCheckMode)
};

static const uint8_t qmiLocSetXtraVersionCheckIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetXtraVersionCheckIndMsgT_v02, status)
};

static const uint8_t qmiLocSetGNSSConstRepConfigReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, measReportConfig) - QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, measReportConfig_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, measReportConfig),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, svPolyReportConfig) - QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, svPolyReportConfig_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, svPolyReportConfig),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, reportFullEphemerisDb) - QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, reportFullEphemerisDb_valid)),
  0x12,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, reportFullEphemerisDb),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, reportFullSvPolyDb) - QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, reportFullSvPolyDb_valid)),
  0x13,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, reportFullSvPolyDb),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, reportFullIonoDb) - QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, reportFullIonoDb_valid)),
  0x14,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigReqMsgT_v02, reportFullIonoDb)
};

static const uint8_t qmiLocSetGNSSConstRepConfigIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGNSSConstRepConfigIndMsgT_v02, status)
};

static const uint8_t qmiLocEventGnssSvMeasInfoIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, seqNum),

  0x02,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, maxMessageNum),

  0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, system),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, rcvrClockFrequencyInfo) - QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, rcvrClockFrequencyInfo_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, rcvrClockFrequencyInfo),
  QMI_IDL_TYPE88(0, 59),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, leapSecondInfo) - QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, leapSecondInfo_valid)),
  0x11,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, leapSecondInfo),
  QMI_IDL_TYPE88(0, 60),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsGloInterSystemBias) - QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsGloInterSystemBias_valid)),
  0x12,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsGloInterSystemBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsBdsInterSystemBias) - QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsBdsInterSystemBias_valid)),
  0x13,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsBdsInterSystemBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsGalInterSystemBias) - QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsGalInterSystemBias_valid)),
  0x14,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsGalInterSystemBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, bdsGloInterSystemBias) - QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, bdsGloInterSystemBias_valid)),
  0x15,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, bdsGloInterSystemBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, galGloInterSystemBias) - QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, galGloInterSystemBias_valid)),
  0x16,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, galGloInterSystemBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, galBdsInterSystemBias) - QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, galBdsInterSystemBias_valid)),
  0x17,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, galBdsInterSystemBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, systemTime) - QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, systemTime_valid)),
  0x18,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, systemTime),
  QMI_IDL_TYPE88(0, 62),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gloTime) - QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gloTime_valid)),
  0x19,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gloTime),
  QMI_IDL_TYPE88(0, 63),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, systemTimeExt) - QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, systemTimeExt_valid)),
  0x1A,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, systemTimeExt),
  QMI_IDL_TYPE88(0, 64),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, svMeasurement) - QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, svMeasurement_valid)),
  0x1B,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, svMeasurement),
  QMI_LOC_SV_MEAS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, svMeasurement) - QMI_IDL_OFFSET8(qmiLocEventGnssSvMeasInfoIndMsgT_v02, svMeasurement_len),
  QMI_IDL_TYPE88(0, 66),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, numClockResets) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, numClockResets_valid)),
  0x1C,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, numClockResets),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, svCarrierPhaseUncertainty) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, svCarrierPhaseUncertainty_valid)),
  0x1D,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, svCarrierPhaseUncertainty),
  QMI_LOC_SV_MEAS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, svCarrierPhaseUncertainty) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, svCarrierPhaseUncertainty_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gnssSignalType) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gnssSignalType_valid)),
  0x1E,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gnssSignalType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, jammerIndicator) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, jammerIndicator_valid)),
  0x1F,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, jammerIndicator),
  QMI_IDL_TYPE88(0, 6),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GpsL1L2cTimeBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GpsL1L2cTimeBias_valid)),
  0x20,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GpsL1L2cTimeBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GpsL1L5TimeBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GpsL1L5TimeBias_valid)),
  0x21,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GpsL1L5TimeBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GloG1G2TimeBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GloG1G2TimeBias_valid)),
  0x22,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GloG1G2TimeBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, BdsB1iB2aTimeBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, BdsB1iB2aTimeBias_valid)),
  0x23,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, BdsB1iB2aTimeBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, BdsB1iB1cTimeBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, BdsB1iB1cTimeBias_valid)),
  0x24,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, BdsB1iB1cTimeBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GalE1E5aTimeBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GalE1E5aTimeBias_valid)),
  0x25,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GalE1E5aTimeBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GalE1E5bTimeBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GalE1E5bTimeBias_valid)),
  0x26,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, GalE1E5bTimeBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, nHzMeasurement) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, nHzMeasurement_valid)),
  0x27,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, nHzMeasurement),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, measurementCodeType) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, measurementCodeType_valid)),
  0x28,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, measurementCodeType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, otherCodeTypeName) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, otherCodeTypeName_valid)),
  0x29,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, otherCodeTypeName),
  QMI_LOC_SV_MEAS_OTHER_CODE_TYPE_NAME_MAX_LEN_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, otherCodeTypeName) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, otherCodeTypeName_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsNavicInterSystemBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsNavicInterSystemBias_valid)),
  0x2A,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsNavicInterSystemBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, galNavicInterSystemBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, galNavicInterSystemBias_valid)),
  0x2B,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, galNavicInterSystemBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gloNavicInterSystemBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gloNavicInterSystemBias_valid)),
  0x2C,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gloNavicInterSystemBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, bdsNavicInterSystemBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, bdsNavicInterSystemBias_valid)),
  0x2D,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, bdsNavicInterSystemBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, extSvMeasurement) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, extSvMeasurement_valid)),
  0x2E,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, extSvMeasurement),
  QMI_LOC_EXT_SV_MEAS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, extSvMeasurement) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, extSvMeasurement_len),
  QMI_IDL_TYPE88(0, 66),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, extSvCarrierPhaseUncertainty) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, extSvCarrierPhaseUncertainty_valid)),
  0x2F,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, extSvCarrierPhaseUncertainty),
  QMI_LOC_EXT_SV_MEAS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, extSvCarrierPhaseUncertainty) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, extSvCarrierPhaseUncertainty_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, refCountTicks) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, refCountTicks_valid)),
  0x30,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, refCountTicks),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, dgnssCorrectionSourceT) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, dgnssCorrectionSourceT_valid)),
  0x31,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, dgnssCorrectionSourceT),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, dgnssCorrectionSourceID) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, dgnssCorrectionSourceID_valid)),
  0x32,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, dgnssCorrectionSourceID),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, dgnssRefStationId) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, dgnssRefStationId_valid)),
  0x33,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, dgnssRefStationId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, dgnssSvMeasurement) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, dgnssSvMeasurement_valid)),
  0x34,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, dgnssSvMeasurement),
  QMI_LOC_DGNSS_SV_MEAS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, dgnssSvMeasurement) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, dgnssSvMeasurement_len),
  QMI_IDL_TYPE88(0, 67),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, refCountTicksUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, refCountTicksUnc_valid)),
  0x35,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, refCountTicksUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, subSeqNum) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, subSeqNum_valid)),
  0x36,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, subSeqNum),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, maxSubSeqNum) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, maxSubSeqNum_valid)),
  0x37,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, maxSubSeqNum),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, payload) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, payload_valid)),
  0x38,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, payload),
  ((2048) & 0xFF), ((2048) >> 8),
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, payload) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, payload_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsBdsB1cInterSystemBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsBdsB1cInterSystemBias_valid)),
  0x39,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gpsBdsB1cInterSystemBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, bdsB1cGloInterSystemBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, bdsB1cGloInterSystemBias_valid)),
  0x3A,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, bdsB1cGloInterSystemBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, galBdsB1cInterSystemBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, galBdsB1cInterSystemBias_valid)),
  0x3B,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, galBdsB1cInterSystemBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, BdsB1cB2aTimeBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, BdsB1cB2aTimeBias_valid)),
  0x3C,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, BdsB1cB2aTimeBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, bdsB1cNavicInterSystemBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, bdsB1cNavicInterSystemBias_valid)),
  0x3D,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, bdsB1cNavicInterSystemBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, BdsB1iB2biTimeBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, BdsB1iB2biTimeBias_valid)),
  0x3E,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, BdsB1iB2biTimeBias),
  QMI_IDL_TYPE88(0, 61),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, jammedSignalsMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, jammedSignalsMask_valid)),
  0x3F,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, jammedSignalsMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gnssLeapSecond) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gnssLeapSecond_valid)),
  0x40,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gnssLeapSecond),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gnssLeapSecondUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gnssLeapSecondUnc_valid)),
  0x41,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, gnssLeapSecondUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, mlInferSvMeasurement) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, mlInferSvMeasurement_valid)),
  0x42,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, mlInferSvMeasurement),
  QMI_LOC_ML_INFER_SV_MEAS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, mlInferSvMeasurement) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, mlInferSvMeasurement_len),
  QMI_IDL_TYPE88(0, 103),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, agcStatus) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvMeasInfoIndMsgT_v02, agcStatus_valid)),
  0x43,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvMeasInfoIndMsgT_v02, agcStatus)
};

static const uint8_t qmiLocEventGnssSvPolyIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, gnssSvId),

  0x02,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, T0),

  0x03,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, svPolyFlagValid),

  0x04,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, svPolyFlags),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, polyCoeffXYZ0) - QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, polyCoeffXYZ0_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, polyCoeffXYZ0),
  QMI_LOC_SV_POLY_XYZ_0_TH_ORDER_COEFF_SIZE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, polyCoefXYZN) - QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, polyCoefXYZN_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, polyCoefXYZN),
  QMI_LOC_SV_POLY_XYZ_N_TH_ORDER_COEFF_SIZE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, polyCoefClockBias) - QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, polyCoefClockBias_valid)),
  0x12,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, polyCoefClockBias),
  QMI_LOC_SV_POLY_SV_CLKBIAS_COEFF_SIZE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, gloFrequency) - QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, gloFrequency_valid)),
  0x13,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, gloFrequency),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, IODE) - QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, IODE_valid)),
  0x14,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, IODE),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, enhancedIOD) - QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, enhancedIOD_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, enhancedIOD),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, svPosUnc) - QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, svPosUnc_valid)),
  0x16,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, svPosUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, ionoDelay) - QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, ionoDelay_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, ionoDelay),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, ionoDot) - QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, ionoDot_valid)),
  0x18,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, ionoDot),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, sbasIonoDelay) - QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, sbasIonoDelay_valid)),
  0x19,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, sbasIonoDelay),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, sbasIonoDot) - QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, sbasIonoDot_valid)),
  0x1A,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, sbasIonoDot),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, tropoDelay) - QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, tropoDelay_valid)),
  0x1B,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventGnssSvPolyIndMsgT_v02, tropoDelay),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, elevation) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, elevation_valid)),
  0x1C,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, elevation),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, elevationDot) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, elevationDot_valid)),
  0x1D,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, elevationDot),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, elenationUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, elenationUnc_valid)),
  0x1E,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, elenationUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, velCoef) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, velCoef_valid)),
  0x1F,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, velCoef),
  QMI_LOC_SV_POLY_VELOCITY_COEF_SIZE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, gpsIscL1ca) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, gpsIscL1ca_valid)),
  0x20,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, gpsIscL1ca),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, gpsIscL2c) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, gpsIscL2c_valid)),
  0x21,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, gpsIscL2c),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, gpsIscL5I5) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, gpsIscL5I5_valid)),
  0x22,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, gpsIscL5I5),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, gpsIscL5Q5) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, gpsIscL5Q5_valid)),
  0x23,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, gpsIscL5Q5),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, gpsTgd) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, gpsTgd_valid)),
  0x24,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, gpsTgd),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, gloTgdG1G2) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, gloTgdG1G2_valid)),
  0x25,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, gloTgdG1G2),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB1) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB1_valid)),
  0x26,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB1),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB2) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB2_valid)),
  0x27,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB2),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB2a) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB2a_valid)),
  0x28,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB2a),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsIscB2a) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsIscB2a_valid)),
  0x29,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, bdsIscB2a),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, galBgdE1E5a) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, galBgdE1E5a_valid)),
  0x2A,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, galBgdE1E5a),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, galBgdE1E5b) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, galBgdE1E5b_valid)),
  0x2B,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, galBgdE1E5b),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, navicTgdL5) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, navicTgdL5_valid)),
  0x2C,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, navicTgdL5),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB1c) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB1c_valid)),
  0x2D,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB1c),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsIscB1c) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsIscB1c_valid)),
  0x2E,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, bdsIscB1c),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, multibandHealth) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, multibandHealth_valid)),
  0x2F,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, multibandHealth),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB2bi) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB2bi_valid)),
  0x30,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, bdsTgdB2bi),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsIscB2bi) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, bdsIscB2bi_valid)),
  0x31,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, bdsIscB2bi),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, toc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, toc_valid)),
  0x32,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, toc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, IODC) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, IODC_valid)),
  0x33,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, IODC),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, toe) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, toe_valid)),
  0x34,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, toe),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, ephemerisSrc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, ephemerisSrc_valid)),
  0x35,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, ephemerisSrc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, polyOrder) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, polyOrder_valid)),
  0x36,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, polyOrder),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, validDuration) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, validDuration_valid)),
  0x37,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, validDuration),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, polyCoeffXYZ) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, polyCoeffXYZ_valid)),
  0x38,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, polyCoeffXYZ),
  QMI_LOC_SV_POLY_XYZ_COEFF_SIZE_MAX_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, polyCoeffXYZ) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, polyCoeffXYZ_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, polyClockBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, polyClockBias_valid)),
  0x39,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventGnssSvPolyIndMsgT_v02, polyClockBias),
  QMI_LOC_SV_POLY_SV_CLKBIAS_COEFF_SIZE_MAX_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, polyClockBias) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventGnssSvPolyIndMsgT_v02, polyClockBias_len)
};

static const uint8_t qmiLocAddGeofenceContextReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddGeofenceContextReqMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocAddGeofenceContextReqMsgT_v02, geofenceId) - QMI_IDL_OFFSET8(qmiLocAddGeofenceContextReqMsgT_v02, geofenceId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddGeofenceContextReqMsgT_v02, geofenceId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocAddGeofenceContextReqMsgT_v02, wifiApSsidInfo) - QMI_IDL_OFFSET8(qmiLocAddGeofenceContextReqMsgT_v02, wifiApSsidInfo_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocAddGeofenceContextReqMsgT_v02, wifiApSsidInfo),
  QMI_LOC_WIFI_AREA_ID_LIST_LENGTH_V02,
  QMI_IDL_OFFSET8(qmiLocAddGeofenceContextReqMsgT_v02, wifiApSsidInfo) - QMI_IDL_OFFSET8(qmiLocAddGeofenceContextReqMsgT_v02, wifiApSsidInfo_len),
  QMI_IDL_TYPE88(0, 39),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, wifiApMacAddressList) - QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, wifiApMacAddressList_valid)),
  0x12,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocAddGeofenceContextReqMsgT_v02, wifiApMacAddressList),
  QMI_LOC_WIFI_AREA_ID_LIST_LENGTH_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, wifiApMacAddressList) - QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, wifiApMacAddressList_len),
  QMI_IDL_TYPE88(0, 68),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, tdsCdmaCellIDList) - QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, tdsCdmaCellIDList_valid)),
  0x13,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocAddGeofenceContextReqMsgT_v02, tdsCdmaCellIDList),
  QMI_LOC_CELL_ID_LIST_LENGTH_V02,
  QMI_IDL_TYPE88(0, 51),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, wcdmaCellIDList) - QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, wcdmaCellIDList_valid)),
  0x14,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocAddGeofenceContextReqMsgT_v02, wcdmaCellIDList),
  QMI_LOC_CELL_ID_LIST_LENGTH_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, wcdmaCellIDList) - QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, wcdmaCellIDList_len),
  QMI_IDL_TYPE88(0, 50),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, gsmCellIDList) - QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, gsmCellIDList_valid)),
  0x15,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocAddGeofenceContextReqMsgT_v02, gsmCellIDList),
  QMI_LOC_CELL_ID_LIST_LENGTH_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, gsmCellIDList) - QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, gsmCellIDList_len),
  QMI_IDL_TYPE88(0, 49),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, iBeaconList) - QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, iBeaconList_valid)),
  0x16,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocAddGeofenceContextReqMsgT_v02, iBeaconList),
  QMI_LOC_IBEACON_LIST_LENGTH_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, iBeaconList) - QMI_IDL_OFFSET16RELATIVE(qmiLocAddGeofenceContextReqMsgT_v02, iBeaconList_len),
  QMI_IDL_TYPE88(0, 69)
};

static const uint8_t qmiLocAddGeofenceContextIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddGeofenceContextIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocAddGeofenceContextIndMsgT_v02, transactionId) - QMI_IDL_OFFSET8(qmiLocAddGeofenceContextIndMsgT_v02, transactionId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddGeofenceContextIndMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocAddGeofenceContextIndMsgT_v02, geofenceId) - QMI_IDL_OFFSET8(qmiLocAddGeofenceContextIndMsgT_v02, geofenceId_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddGeofenceContextIndMsgT_v02, geofenceId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocAddGeofenceContextIndMsgT_v02, contextId) - QMI_IDL_OFFSET8(qmiLocAddGeofenceContextIndMsgT_v02, contextId_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocAddGeofenceContextIndMsgT_v02, contextId)
};

static const uint8_t qmiLocSetGeofenceEngineContextReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineContextReqMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineContextReqMsgT_v02, utcTimeOfDay) - QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineContextReqMsgT_v02, utcTimeOfDay_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineContextReqMsgT_v02, utcTimeOfDay),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineContextReqMsgT_v02, temperature) - QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineContextReqMsgT_v02, temperature_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineContextReqMsgT_v02, temperature)
};

static const uint8_t qmiLocSetGeofenceEngineContextIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineContextIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineContextIndMsgT_v02, transactionId) - QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineContextIndMsgT_v02, transactionId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGeofenceEngineContextIndMsgT_v02, transactionId)
};

static const uint8_t qmiLocDeleteGeofenceContextReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextReqMsgT_v02, transactionId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextReqMsgT_v02, geofenceId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextReqMsgT_v02, contextId) - QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextReqMsgT_v02, contextId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextReqMsgT_v02, contextId)
};

static const uint8_t qmiLocDeleteGeofenceContextIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextIndMsgT_v02, transactionId) - QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextIndMsgT_v02, transactionId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextIndMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextIndMsgT_v02, geofenceId) - QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextIndMsgT_v02, geofenceId_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextIndMsgT_v02, geofenceId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextIndMsgT_v02, contextId) - QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextIndMsgT_v02, contextId_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGeofenceContextIndMsgT_v02, contextId)
};

static const uint8_t qmiLocInjectGtpClientDownloadedDataReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectGtpClientDownloadedDataReqMsgT_v02, ClientDownloadedData),
  ((QMI_LOC_MAX_GTP_WWAN_CLIENT_DOWNLOADED_DATA_LEN_V02) & 0xFF), ((QMI_LOC_MAX_GTP_WWAN_CLIENT_DOWNLOADED_DATA_LEN_V02) >> 8),
  QMI_IDL_OFFSET8(qmiLocInjectGtpClientDownloadedDataReqMsgT_v02, ClientDownloadedData) - QMI_IDL_OFFSET8(qmiLocInjectGtpClientDownloadedDataReqMsgT_v02, ClientDownloadedData_len)
};

static const uint8_t qmiLocInjectGtpClientDownloadedDataIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectGtpClientDownloadedDataIndMsgT_v02, status)
};

static const uint8_t qmiLocGdtUploadBeginStatusReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtUploadBeginStatusReqMsgT_v02, serviceId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtUploadBeginStatusReqMsgT_v02, sessionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtUploadBeginStatusReqMsgT_v02, gdtAccessStatus)
};

static const uint8_t qmiLocGdtUploadBeginStatusIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtUploadBeginStatusIndMsgT_v02, status)
};

static const uint8_t qmiLocGdtUploadEndReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtUploadEndReqMsgT_v02, serviceId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtUploadEndReqMsgT_v02, sessionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtUploadEndReqMsgT_v02, gdtEndStatus)
};

static const uint8_t qmiLocGdtUploadEndIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtUploadEndIndMsgT_v02, status)
};

static const uint8_t qmiLocGtpApStatusReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGtpApStatusReqMsgT_v02, gtpApDbStatus),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGtpApStatusReqMsgT_v02, gtpApPcid64) - QMI_IDL_OFFSET8(qmiLocGtpApStatusReqMsgT_v02, gtpApPcid64_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGtpApStatusReqMsgT_v02, gtpApPcid64),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGtpApStatusReqMsgT_v02, oemId) - QMI_IDL_OFFSET8(qmiLocGtpApStatusReqMsgT_v02, oemId_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGtpApStatusReqMsgT_v02, oemId),
  ((QMI_LOC_MAX_OEM_ID_LEN_V02) & 0xFF), ((QMI_LOC_MAX_OEM_ID_LEN_V02) >> 8),
  QMI_IDL_OFFSET8(qmiLocGtpApStatusReqMsgT_v02, oemId) - QMI_IDL_OFFSET8(qmiLocGtpApStatusReqMsgT_v02, oemId_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGtpApStatusReqMsgT_v02, modelId) - QMI_IDL_OFFSET16RELATIVE(qmiLocGtpApStatusReqMsgT_v02, modelId_valid)),
  0x12,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGtpApStatusReqMsgT_v02, modelId),
  ((QMI_LOC_MAX_MODEL_ID_LEN_V02) & 0xFF), ((QMI_LOC_MAX_MODEL_ID_LEN_V02) >> 8),
  QMI_IDL_OFFSET16RELATIVE(qmiLocGtpApStatusReqMsgT_v02, modelId) - QMI_IDL_OFFSET16RELATIVE(qmiLocGtpApStatusReqMsgT_v02, modelId_len)
};

static const uint8_t qmiLocGtpApStatusIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGtpApStatusIndMsgT_v02, status),

  0x02,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocGtpApStatusIndMsgT_v02, clientSoftwareVersion),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x03,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGtpApStatusIndMsgT_v02, asnVersion),
  QMI_IDL_TYPE88(0, 70)
};

static const uint8_t qmiLocGdtDownloadBeginStatusReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadBeginStatusReqMsgT_v02, serviceId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadBeginStatusReqMsgT_v02, sessionId),

  0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadBeginStatusReqMsgT_v02, processingStatus),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGdtDownloadBeginStatusReqMsgT_v02, wwanDownloadFlag) - QMI_IDL_OFFSET8(qmiLocGdtDownloadBeginStatusReqMsgT_v02, wwanDownloadFlag_valid)),
  0x10,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadBeginStatusReqMsgT_v02, wwanDownloadFlag),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGdtDownloadBeginStatusReqMsgT_v02, respLocInfo) - QMI_IDL_OFFSET8(qmiLocGdtDownloadBeginStatusReqMsgT_v02, respLocInfo_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadBeginStatusReqMsgT_v02, respLocInfo),
  ((QMI_LOC_MAX_GTP_RLI_LEN_V02) & 0xFF), ((QMI_LOC_MAX_GTP_RLI_LEN_V02) >> 8),
  QMI_IDL_OFFSET8(qmiLocGdtDownloadBeginStatusReqMsgT_v02, respLocInfo) - QMI_IDL_OFFSET8(qmiLocGdtDownloadBeginStatusReqMsgT_v02, respLocInfo_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGdtDownloadBeginStatusReqMsgT_v02, apRemainingThrottleTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocGdtDownloadBeginStatusReqMsgT_v02, apRemainingThrottleTime_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGdtDownloadBeginStatusReqMsgT_v02, apRemainingThrottleTime)
};

static const uint8_t qmiLocGdtDownloadBeginStatusIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadBeginStatusIndMsgT_v02, status)
};

static const uint8_t qmiLocGdtDownloadReadyStatusReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadReadyStatusReqMsgT_v02, serviceId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadReadyStatusReqMsgT_v02, sessionId),

  0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadReadyStatusReqMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x04,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadReadyStatusReqMsgT_v02, filePath),
  QMI_LOC_MAX_GDT_PATH_LEN_V02,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadReadyStatusReqMsgT_v02, filePath) - QMI_IDL_OFFSET8(qmiLocGdtDownloadReadyStatusReqMsgT_v02, filePath_len)
};

static const uint8_t qmiLocGdtDownloadReadyStatusIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadReadyStatusIndMsgT_v02, status)
};

static const uint8_t qmiLocGdtReceiveDoneStatusReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtReceiveDoneStatusReqMsgT_v02, serviceId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtReceiveDoneStatusReqMsgT_v02, sessionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtReceiveDoneStatusReqMsgT_v02, status)
};

static const uint8_t qmiLocGdtReceiveDoneStatusIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtReceiveDoneStatusIndMsgT_v02, status)
};

static const uint8_t qmiLocGdtDownloadEndStatusReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadEndStatusReqMsgT_v02, serviceId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadEndStatusReqMsgT_v02, sessionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadEndStatusReqMsgT_v02, status)
};

static const uint8_t qmiLocGdtDownloadEndStatusIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGdtDownloadEndStatusIndMsgT_v02, status)
};

static const uint8_t qmiLocStartDbtReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartDbtReqMsgT_v02, reqId),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartDbtReqMsgT_v02, minDistance),

  0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartDbtReqMsgT_v02, distanceType),

  0x04,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartDbtReqMsgT_v02, needOriginLocation),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartDbtReqMsgT_v02, maxLatency) - QMI_IDL_OFFSET8(qmiLocStartDbtReqMsgT_v02, maxLatency_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartDbtReqMsgT_v02, maxLatency),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartDbtReqMsgT_v02, usageType) - QMI_IDL_OFFSET8(qmiLocStartDbtReqMsgT_v02, usageType_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartDbtReqMsgT_v02, usageType)
};

static const uint8_t qmiLocStartDbtIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartDbtIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartDbtIndMsgT_v02, reqId) - QMI_IDL_OFFSET8(qmiLocStartDbtIndMsgT_v02, reqId_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartDbtIndMsgT_v02, reqId)
};

static const uint8_t qmiLocStopDbtReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocStopDbtReqMsgT_v02, reqId)
};

static const uint8_t qmiLocStopDbtIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStopDbtIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStopDbtIndMsgT_v02, reqId) - QMI_IDL_OFFSET8(qmiLocStopDbtIndMsgT_v02, reqId_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocStopDbtIndMsgT_v02, reqId)
};

static const uint8_t qmiLocEventDbtPositionReportIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, reqId),

  0x02,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, dbtPosition),
  QMI_IDL_TYPE88(0, 71),

  0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, positionType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, headingUnc) - QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, headingUnc_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, headingUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, speedUnc) - QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, speedUnc_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, speedUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, horConfidence) - QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, horConfidence_valid)),
  0x12,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, horConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, vertConfidence) - QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, vertConfidence_valid)),
  0x13,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, vertConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, DOP) - QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, DOP_valid)),
  0x14,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, DOP),
  QMI_IDL_TYPE88(0, 3),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, gnssSvUsedList) - QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, gnssSvUsedList_valid)),
  0x15,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, gnssSvUsedList),
  QMI_LOC_MAX_SV_USED_LIST_LENGTH_V02,
  QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, gnssSvUsedList) - QMI_IDL_OFFSET8(qmiLocEventDbtPositionReportIndMsgT_v02, gnssSvUsedList_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventDbtPositionReportIndMsgT_v02, positionSrc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventDbtPositionReportIndMsgT_v02, positionSrc_valid)),
  0x16,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventDbtPositionReportIndMsgT_v02, positionSrc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventDbtPositionReportIndMsgT_v02, extDOP) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventDbtPositionReportIndMsgT_v02, extDOP_valid)),
  0x17,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventDbtPositionReportIndMsgT_v02, extDOP),
  QMI_IDL_TYPE88(0, 4),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventDbtPositionReportIndMsgT_v02, expandedGnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventDbtPositionReportIndMsgT_v02, expandedGnssSvUsedList_valid)),
  0x18,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventDbtPositionReportIndMsgT_v02, expandedGnssSvUsedList),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventDbtPositionReportIndMsgT_v02, expandedGnssSvUsedList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventDbtPositionReportIndMsgT_v02, expandedGnssSvUsedList_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventDbtPositionReportIndMsgT_v02, gnssSvUsedSignalTypeList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventDbtPositionReportIndMsgT_v02, gnssSvUsedSignalTypeList_valid)),
  0x19,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventDbtPositionReportIndMsgT_v02, gnssSvUsedSignalTypeList),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEventDbtPositionReportIndMsgT_v02, gnssSvUsedSignalTypeList) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventDbtPositionReportIndMsgT_v02, gnssSvUsedSignalTypeList_len)
};

static const uint8_t qmiLocEventDbtSessionStatusIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbtSessionStatusIndMsgT_v02, dbtSessionStatus),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbtSessionStatusIndMsgT_v02, reqId) - QMI_IDL_OFFSET8(qmiLocEventDbtSessionStatusIndMsgT_v02, reqId_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbtSessionStatusIndMsgT_v02, reqId)
};

static const uint8_t qmiLocSecureGetAvailablePositionReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSecureGetAvailablePositionReqMsgT_v02, secureLocDataMode),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSecureGetAvailablePositionReqMsgT_v02, qmilocSecureGetAvailablePositionRequestData),
  ((QMI_LOC_SECURE_GET_AVAILABLE_POS_REQUEST_ENCRYPTED_MAX_V02) & 0xFF), ((QMI_LOC_SECURE_GET_AVAILABLE_POS_REQUEST_ENCRYPTED_MAX_V02) >> 8),
  QMI_IDL_OFFSET8(qmiLocSecureGetAvailablePositionReqMsgT_v02, qmilocSecureGetAvailablePositionRequestData) - QMI_IDL_OFFSET8(qmiLocSecureGetAvailablePositionReqMsgT_v02, qmilocSecureGetAvailablePositionRequestData_len)
};

static const uint8_t qmiLocSecureGetAvailablePositionIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSecureGetAvailablePositionIndMsgT_v02, sessionStatus),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSecureGetAvailablePositionIndMsgT_v02, secureLocDataMode),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSecureGetAvailablePositionIndMsgT_v02, qmilocSecureGetAvailablePositionInd) - QMI_IDL_OFFSET8(qmiLocSecureGetAvailablePositionIndMsgT_v02, qmilocSecureGetAvailablePositionInd_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSecureGetAvailablePositionIndMsgT_v02, qmilocSecureGetAvailablePositionInd),
  ((QMI_LOC_SECURE_GET_AVAILABLE_POSITION_IND_ENCRYPTED_MAX_V02) & 0xFF), ((QMI_LOC_SECURE_GET_AVAILABLE_POSITION_IND_ENCRYPTED_MAX_V02) >> 8),
  QMI_IDL_OFFSET8(qmiLocSecureGetAvailablePositionIndMsgT_v02, qmilocSecureGetAvailablePositionInd) - QMI_IDL_OFFSET8(qmiLocSecureGetAvailablePositionIndMsgT_v02, qmilocSecureGetAvailablePositionInd_len)
};

static const uint8_t qmiLocInjectApCacheDataReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataReqMsgT_v02, versionNumber),

  0x02,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataReqMsgT_v02, partNumber),

  0x03,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataReqMsgT_v02, totalParts),

  0x04,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataReqMsgT_v02, apCacheData),
  QMI_LOC_APCACHE_DATA_MAX_SAMPLES_V02,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataReqMsgT_v02, apCacheData) - QMI_IDL_OFFSET8(qmiLocInjectApCacheDataReqMsgT_v02, apCacheData_len),
  QMI_IDL_TYPE88(0, 72),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectApCacheDataReqMsgT_v02, apCacheHepeRelData) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectApCacheDataReqMsgT_v02, apCacheHepeRelData_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectApCacheDataReqMsgT_v02, apCacheHepeRelData),
  QMI_LOC_APCACHE_DATA_MAX_SAMPLES_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocInjectApCacheDataReqMsgT_v02, apCacheHepeRelData) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectApCacheDataReqMsgT_v02, apCacheHepeRelData_len),
  QMI_IDL_TYPE88(0, 73)
};

static const uint8_t qmiLocInjectApCacheDataIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, apCacheSize) - QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, apCacheSize_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, apCacheSize),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, apDoNotCacheSize) - QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, apDoNotCacheSize_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, apDoNotCacheSize),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, apCacheHits) - QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, apCacheHits_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, apCacheHits),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, apDoNotCacheHits) - QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, apDoNotCacheHits_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, apDoNotCacheHits),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, unknownAps) - QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, unknownAps_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, unknownAps),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, asyncScans) - QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, asyncScans_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, asyncScans),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, asyncFixes) - QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, asyncFixes_valid)),
  0x16,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, asyncFixes),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, syncScans) - QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, syncScans_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, syncScans),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, syncFixes) - QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, syncFixes_valid)),
  0x18,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApCacheDataIndMsgT_v02, syncFixes)
};

static const uint8_t qmiLocInjectApDoNotCacheDataReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApDoNotCacheDataReqMsgT_v02, versionNumber),

  0x02,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApDoNotCacheDataReqMsgT_v02, partNumber),

  0x03,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApDoNotCacheDataReqMsgT_v02, totalParts),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x04,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectApDoNotCacheDataReqMsgT_v02, apDoNotCacheData),
  QMI_LOC_APCACHE_DATA_MAX_SAMPLES_V02,
  QMI_IDL_OFFSET8(qmiLocInjectApDoNotCacheDataReqMsgT_v02, apDoNotCacheData) - QMI_IDL_OFFSET8(qmiLocInjectApDoNotCacheDataReqMsgT_v02, apDoNotCacheData_len),
  QMI_IDL_TYPE88(0, 74)
};

static const uint8_t qmiLocInjectApDoNotCacheDataIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectApDoNotCacheDataIndMsgT_v02, status)
};

static const uint8_t qmiLocEventBatchingStatusIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventBatchingStatusIndMsgT_v02, batchingStatus)
};

static const uint8_t qmiLocQueryAonConfigReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryAonConfigReqMsgT_v02, transactionId)
};

static const uint8_t qmiLocQueryAonConfigIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryAonConfigIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocQueryAonConfigIndMsgT_v02, transactionId) - QMI_IDL_OFFSET8(qmiLocQueryAonConfigIndMsgT_v02, transactionId_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryAonConfigIndMsgT_v02, transactionId),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocQueryAonConfigIndMsgT_v02, aonCapability) - QMI_IDL_OFFSET8(qmiLocQueryAonConfigIndMsgT_v02, aonCapability_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryAonConfigIndMsgT_v02, aonCapability)
};

static const uint8_t qmiLocDeleteGNSSServiceDataReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteAllFlag),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteClockInfoMask) - QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteClockInfoMask_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteClockInfoMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteCellDbDataMask) - QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteCellDbDataMask_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteCellDbDataMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteCommonDataMask) - QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteCommonDataMask_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteCommonDataMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteSatelliteData) - QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteSatelliteData_valid)),
  0x13,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteSatelliteData),
  QMI_IDL_TYPE88(0, 75),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteExtClockInfoMask) - QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteExtClockInfoMask_valid)),
  0x14,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataReqMsgT_v02, deleteExtClockInfoMask)
};

static const uint8_t qmiLocDeleteGNSSServiceDataIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocDeleteGNSSServiceDataIndMsgT_v02, status)
};

static const uint8_t qmiLocInjectXtraDataReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectXtraDataReqMsgT_v02, totalSize),

  0x02,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectXtraDataReqMsgT_v02, totalParts),

  0x03,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectXtraDataReqMsgT_v02, partNum),

  0x04,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectXtraDataReqMsgT_v02, partData),
  ((QMI_LOC_MAX_XTRA_PART_LEN_V02) & 0xFF), ((QMI_LOC_MAX_XTRA_PART_LEN_V02) >> 8),
  QMI_IDL_OFFSET8(qmiLocInjectXtraDataReqMsgT_v02, partData) - QMI_IDL_OFFSET8(qmiLocInjectXtraDataReqMsgT_v02, partData_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectXtraDataReqMsgT_v02, formatType) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectXtraDataReqMsgT_v02, formatType_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectXtraDataReqMsgT_v02, formatType),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectXtraDataReqMsgT_v02, fileInfo) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectXtraDataReqMsgT_v02, fileInfo_valid)),
  0x11,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectXtraDataReqMsgT_v02, fileInfo),
  QMI_IDL_TYPE88(0, 26)
};

static const uint8_t qmiLocInjectXtraDataIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectXtraDataIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectXtraDataIndMsgT_v02, partNum) - QMI_IDL_OFFSET8(qmiLocInjectXtraDataIndMsgT_v02, partNum_valid)),
  0x10,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectXtraDataIndMsgT_v02, partNum),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectXtraDataIndMsgT_v02, ConstellationMask) - QMI_IDL_OFFSET8(qmiLocInjectXtraDataIndMsgT_v02, ConstellationMask_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectXtraDataIndMsgT_v02, ConstellationMask)
};

static const uint8_t qmiLocInjectXtraPcidReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectXtraPcidReqMsgT_v02, xtraPcid)
};

static const uint8_t qmiLocInjectXtraPcidIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectXtraPcidIndMsgT_v02, status)
};

/*
 * qmiLocGetSupportedFeatureReqMsgT is empty
 * static const uint8_t qmiLocGetSupportedFeatureReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocGetSupportedFeatureIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSupportedFeatureIndMsgT_v02, status),

  0x02,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSupportedFeatureIndMsgT_v02, feature),
  QMI_LOC_SUPPORTED_FEATURE_LENGTH_V02,
  QMI_IDL_OFFSET8(qmiLocGetSupportedFeatureIndMsgT_v02, feature) - QMI_IDL_OFFSET8(qmiLocGetSupportedFeatureIndMsgT_v02, feature_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetSupportedFeatureIndMsgT_v02, featureStatusReport) - QMI_IDL_OFFSET8(qmiLocGetSupportedFeatureIndMsgT_v02, featureStatusReport_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetSupportedFeatureIndMsgT_v02, featureStatusReport)
};

static const uint8_t qmiLocSetInternalStatusConfigReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetInternalStatusConfigReqMsgT_v02, config)
};

static const uint8_t qmiLocSetInternalStatusConfigIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetInternalStatusConfigIndMsgT_v02, status)
};

static const uint8_t qmiLocEventInternalStatusReportIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventInternalStatusReportIndMsgT_v02, reportData),
  QMI_LOC_INTERNAL_STATUS_MAX_LIST_SIZE_V02,
  QMI_IDL_OFFSET8(qmiLocEventInternalStatusReportIndMsgT_v02, reportData) - QMI_IDL_OFFSET8(qmiLocEventInternalStatusReportIndMsgT_v02, reportData_len),
  QMI_IDL_TYPE88(0, 76)
};

static const uint8_t qmiLocEventInjectSrnApDataReqIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectSrnApDataReqIndMsgT_v02, srnTechMask),

  0x02,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectSrnApDataReqIndMsgT_v02, srnRequest),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectSrnApDataReqIndMsgT_v02, e911Mode) - QMI_IDL_OFFSET8(qmiLocEventInjectSrnApDataReqIndMsgT_v02, e911Mode_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectSrnApDataReqIndMsgT_v02, e911Mode),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectSrnApDataReqIndMsgT_v02, srnApMacAddrType) - QMI_IDL_OFFSET8(qmiLocEventInjectSrnApDataReqIndMsgT_v02, srnApMacAddrType_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectSrnApDataReqIndMsgT_v02, srnApMacAddrType)
};

static const uint8_t qmiLocInjectSrnApDataReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSrnApDataReqMsgT_v02, apDeviceTechType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectSrnApDataReqMsgT_v02, srnBtleApInfo) - QMI_IDL_OFFSET8(qmiLocInjectSrnApDataReqMsgT_v02, srnBtleApInfo_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectSrnApDataReqMsgT_v02, srnBtleApInfo),
  QMI_LOC_SRN_MAX_REPORTED_APS_PER_MSG_V02,
  QMI_IDL_OFFSET8(qmiLocInjectSrnApDataReqMsgT_v02, srnBtleApInfo) - QMI_IDL_OFFSET8(qmiLocInjectSrnApDataReqMsgT_v02, srnBtleApInfo_len),
  QMI_IDL_TYPE88(0, 77),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSrnApDataReqMsgT_v02, requestTimestamp) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSrnApDataReqMsgT_v02, requestTimestamp_valid)),
  0x11,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectSrnApDataReqMsgT_v02, requestTimestamp),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSrnApDataReqMsgT_v02, receiveTimestamp) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSrnApDataReqMsgT_v02, receiveTimestamp_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectSrnApDataReqMsgT_v02, receiveTimestamp),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSrnApDataReqMsgT_v02, errorCause) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectSrnApDataReqMsgT_v02, errorCause_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectSrnApDataReqMsgT_v02, errorCause)
};

static const uint8_t qmiLocInjectSrnApDataIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectSrnApDataIndMsgT_v02, status)
};

static const uint8_t qmiLocCrowdSourceManagerControlReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlReqMsgT_v02, enableCrowdSourcingMask) - QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlReqMsgT_v02, enableCrowdSourcingMask_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlReqMsgT_v02, enableCrowdSourcingMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlReqMsgT_v02, wifiCrowdSourcingLocalConfig) - QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlReqMsgT_v02, wifiCrowdSourcingLocalConfig_valid)),
  0x11,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlReqMsgT_v02, wifiCrowdSourcingLocalConfig),
  QMI_IDL_TYPE88(0, 78),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlReqMsgT_v02, wifiCrowdSourcingServerConfig) - QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlReqMsgT_v02, wifiCrowdSourcingServerConfig_valid)),
  0x12,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlReqMsgT_v02, wifiCrowdSourcingServerConfig),
  ((QMI_LOC_MAX_WIFI_CROWDSOURCING_SERVER_CONFIG_LEN_V02) & 0xFF), ((QMI_LOC_MAX_WIFI_CROWDSOURCING_SERVER_CONFIG_LEN_V02) >> 8),
  QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlReqMsgT_v02, wifiCrowdSourcingServerConfig) - QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlReqMsgT_v02, wifiCrowdSourcingServerConfig_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocCrowdSourceManagerControlReqMsgT_v02, enableSourceMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocCrowdSourceManagerControlReqMsgT_v02, enableSourceMask_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocCrowdSourceManagerControlReqMsgT_v02, enableSourceMask)
};

static const uint8_t qmiLocCrowdSourceManagerControlIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlIndMsgT_v02, supportedCrowdSourcingMask) - QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlIndMsgT_v02, supportedCrowdSourcingMask_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlIndMsgT_v02, supportedCrowdSourcingMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlIndMsgT_v02, wifiCrowdSourcingCapabaility) - QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlIndMsgT_v02, wifiCrowdSourcingCapabaility_valid)),
  0x11,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlIndMsgT_v02, wifiCrowdSourcingCapabaility),
  QMI_IDL_TYPE88(0, 79),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlIndMsgT_v02, modemClientInfo) - QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlIndMsgT_v02, modemClientInfo_valid)),
  0x12,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlIndMsgT_v02, modemClientInfo),
  ((QMI_LOC_MAX_CROWDSOURCING_MODEM_CLIENT_INFO_LEN_V02) & 0xFF), ((QMI_LOC_MAX_CROWDSOURCING_MODEM_CLIENT_INFO_LEN_V02) >> 8),
  QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlIndMsgT_v02, modemClientInfo) - QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerControlIndMsgT_v02, modemClientInfo_len)
};

static const uint8_t qmiLocCrowdSourceManagerReadDataReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerReadDataReqMsgT_v02, crowdSourcingTechMask) - QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerReadDataReqMsgT_v02, crowdSourcingTechMask_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocCrowdSourceManagerReadDataReqMsgT_v02, crowdSourcingTechMask)
};

static const uint8_t qmiLocQueryXtraInfoReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryXtraInfoReqMsgT_v02, xtraConfig),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocQueryXtraInfoReqMsgT_v02, xcVersion) - QMI_IDL_OFFSET8(qmiLocQueryXtraInfoReqMsgT_v02, xcVersion_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocQueryXtraInfoReqMsgT_v02, xcVersion),
  QMI_IDL_TYPE88(0, 81)
};

static const uint8_t qmiLocQueryXtraInfoIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryXtraInfoIndMsgT_v02, status),

  0x02,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocQueryXtraInfoIndMsgT_v02, xtraInfo),
  QMI_IDL_TYPE88(0, 80),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, mpVersion) - QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, mpVersion_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocQueryXtraInfoIndMsgT_v02, mpVersion),
  QMI_IDL_TYPE88(0, 81),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, usableConstellationConfig) - QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, usableConstellationConfig_valid)),
  0x11,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocQueryXtraInfoIndMsgT_v02, usableConstellationConfig),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, forcedEnableConfig) - QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, forcedEnableConfig_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocQueryXtraInfoIndMsgT_v02, forcedEnableConfig),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, multibandEnabled) - QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, multibandEnabled_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocQueryXtraInfoIndMsgT_v02, multibandEnabled),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, xtraFileVersion) - QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, xtraFileVersion_valid)),
  0x14,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocQueryXtraInfoIndMsgT_v02, xtraFileVersion),
  QMI_IDL_TYPE88(0, 81),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, mccPrimarySimSlot) - QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, mccPrimarySimSlot_valid)),
  0x15,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocQueryXtraInfoIndMsgT_v02, mccPrimarySimSlot),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, mccSecondarySimSlot) - QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, mccSecondarySimSlot_valid)),
  0x16,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocQueryXtraInfoIndMsgT_v02, mccSecondarySimSlot),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, mccTertiarySimSlot) - QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, mccTertiarySimSlot_valid)),
  0x17,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocQueryXtraInfoIndMsgT_v02, mccTertiarySimSlot),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, xtraFileGenerationTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, xtraFileGenerationTime_valid)),
  0x18,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocQueryXtraInfoIndMsgT_v02, xtraFileGenerationTime),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, xtraRemValidDuration) - QMI_IDL_OFFSET16RELATIVE(qmiLocQueryXtraInfoIndMsgT_v02, xtraRemValidDuration_valid)),
  0x19,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocQueryXtraInfoIndMsgT_v02, xtraRemValidDuration)
};

static const uint8_t qmiLocStartOutdoorTripBatchingReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartOutdoorTripBatchingReqMsgT_v02, batchDistance),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartOutdoorTripBatchingReqMsgT_v02, minTimeInterval),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartOutdoorTripBatchingReqMsgT_v02, fixSessionTimeout) - QMI_IDL_OFFSET8(qmiLocStartOutdoorTripBatchingReqMsgT_v02, fixSessionTimeout_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartOutdoorTripBatchingReqMsgT_v02, fixSessionTimeout),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocStartOutdoorTripBatchingReqMsgT_v02, batchAllPos) - QMI_IDL_OFFSET8(qmiLocStartOutdoorTripBatchingReqMsgT_v02, batchAllPos_valid)),
  0x11,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartOutdoorTripBatchingReqMsgT_v02, batchAllPos)
};

static const uint8_t qmiLocStartOutdoorTripBatchingIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocStartOutdoorTripBatchingIndMsgT_v02, status)
};

/*
 * qmiLocQueryOTBAccumulatedDistanceReqMsgT is empty
 * static const uint8_t qmiLocQueryOTBAccumulatedDistanceReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocQueryOTBAccumulatedDistanceIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryOTBAccumulatedDistanceIndMsgT_v02, status),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryOTBAccumulatedDistanceIndMsgT_v02, accumulatedDistance),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryOTBAccumulatedDistanceIndMsgT_v02, batchedPosition)
};

static const uint8_t qmiLocEventFdclServiceReqIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventFdclServiceReqIndMsgT_v02, gtpClientSwVer) - QMI_IDL_OFFSET8(qmiLocEventFdclServiceReqIndMsgT_v02, gtpClientSwVer_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventFdclServiceReqIndMsgT_v02, gtpClientSwVer),
  QMI_IDL_TYPE88(0, 82),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventFdclServiceReqIndMsgT_v02, modemSwVersion) - QMI_IDL_OFFSET8(qmiLocEventFdclServiceReqIndMsgT_v02, modemSwVersion_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocEventFdclServiceReqIndMsgT_v02, modemSwVersion),
  QMI_LOC_GNSS_SW_VERSION_STRING_MAX_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, homeCarrierId) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, homeCarrierId_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventFdclServiceReqIndMsgT_v02, homeCarrierId),
  QMI_IDL_TYPE88(0, 83),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, cdmaRealm) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, cdmaRealm_valid)),
  0x13,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventFdclServiceReqIndMsgT_v02, cdmaRealm),
  QMI_LOC_GNSS_SW_VERSION_STRING_MAX_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, gtpMaxNumRecordsAccepted) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, gtpMaxNumRecordsAccepted_valid)),
  0x14,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventFdclServiceReqIndMsgT_v02, gtpMaxNumRecordsAccepted),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, gpsWeek) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, gpsWeek_valid)),
  0x15,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventFdclServiceReqIndMsgT_v02, gpsWeek),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, gpsMilliseconds) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, gpsMilliseconds_valid)),
  0x16,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventFdclServiceReqIndMsgT_v02, gpsMilliseconds),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, bsBestPosition) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, bsBestPosition_valid)),
  0x17,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventFdclServiceReqIndMsgT_v02, bsBestPosition),
  QMI_IDL_TYPE88(0, 85),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, currentCellId) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, currentCellId_valid)),
  0x18,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventFdclServiceReqIndMsgT_v02, currentCellId),
  QMI_IDL_TYPE88(0, 86),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, requestedDataTypes) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, requestedDataTypes_valid)),
  0x19,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventFdclServiceReqIndMsgT_v02, requestedDataTypes),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, restrictedRegionVersion) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventFdclServiceReqIndMsgT_v02, restrictedRegionVersion_valid)),
  0x1A,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventFdclServiceReqIndMsgT_v02, restrictedRegionVersion)
};

static const uint8_t qmiLocGetFdclBsListReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetFdclBsListReqMsgT_v02, expireInDays) - QMI_IDL_OFFSET8(qmiLocGetFdclBsListReqMsgT_v02, expireInDays_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetFdclBsListReqMsgT_v02, expireInDays),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetFdclBsListReqMsgT_v02, timestampUtc) - QMI_IDL_OFFSET8(qmiLocGetFdclBsListReqMsgT_v02, timestampUtc_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetFdclBsListReqMsgT_v02, timestampUtc),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetFdclBsListReqMsgT_v02, maxNumBs) - QMI_IDL_OFFSET8(qmiLocGetFdclBsListReqMsgT_v02, maxNumBs_valid)),
  0x12,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetFdclBsListReqMsgT_v02, maxNumBs)
};

static const uint8_t qmiLocGetFdclBsListIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetFdclBsListIndMsgT_v02, status),

  0x02,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetFdclBsListIndMsgT_v02, BsList),
  QMI_LOC_FDCL_BS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET8(qmiLocGetFdclBsListIndMsgT_v02, BsList) - QMI_IDL_OFFSET8(qmiLocGetFdclBsListIndMsgT_v02, BsList_len),
  QMI_IDL_TYPE88(0, 86),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetFdclBsListIndMsgT_v02, moreBsAvailable) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetFdclBsListIndMsgT_v02, moreBsAvailable_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetFdclBsListIndMsgT_v02, moreBsAvailable),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetFdclBsListIndMsgT_v02, bsLocalTimestampList) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetFdclBsListIndMsgT_v02, bsLocalTimestampList_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetFdclBsListIndMsgT_v02, bsLocalTimestampList),
  QMI_LOC_FDCL_BS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocGetFdclBsListIndMsgT_v02, bsLocalTimestampList) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetFdclBsListIndMsgT_v02, bsLocalTimestampList_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetFdclBsListIndMsgT_v02, bsBestPosition) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetFdclBsListIndMsgT_v02, bsBestPosition_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetFdclBsListIndMsgT_v02, bsBestPosition),
  QMI_IDL_TYPE88(0, 85)
};

static const uint8_t qmiLocInjectFdclDataReqMsgT_data_v02[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocInjectFdclDataReqMsgT_v02, cellPosList),
  QMI_LOC_FDCL_CELL_POS_LIST_LENGTH_V02,
  QMI_IDL_OFFSET8(qmiLocInjectFdclDataReqMsgT_v02, cellPosList) - QMI_IDL_OFFSET8(qmiLocInjectFdclDataReqMsgT_v02, cellPosList_len),
  QMI_IDL_TYPE88(0, 87),

  0x02,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectFdclDataReqMsgT_v02, daysValid),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, timestampUtc) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, timestampUtc_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectFdclDataReqMsgT_v02, timestampUtc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, cellRecordTypeList) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, cellRecordTypeList_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectFdclDataReqMsgT_v02, cellRecordTypeList),
  QMI_LOC_FDCL_CELL_POS_LIST_LENGTH_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, cellRecordTypeList) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, cellRecordTypeList_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, moreBsAvailable) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, moreBsAvailable_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectFdclDataReqMsgT_v02, moreBsAvailable),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, clientControls) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, clientControls_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectFdclDataReqMsgT_v02, clientControls),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, maxNumCellsToUpload) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, maxNumCellsToUpload_valid)),
  0x14,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectFdclDataReqMsgT_v02, maxNumCellsToUpload),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, minSecsBtwRequests) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, minSecsBtwRequests_valid)),
  0x15,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectFdclDataReqMsgT_v02, minSecsBtwRequests),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, restrictedRegionVersion) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, restrictedRegionVersion_valid)),
  0x16,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectFdclDataReqMsgT_v02, restrictedRegionVersion),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, restrictedRegions) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, restrictedRegions_valid)),
  0x17,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectFdclDataReqMsgT_v02, restrictedRegions),
  QMI_LOC_FDCL_BS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, restrictedRegions) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectFdclDataReqMsgT_v02, restrictedRegions_len),
  QMI_IDL_TYPE88(0, 83)
};

static const uint8_t qmiLocInjectFdclDataIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectFdclDataIndMsgT_v02, status),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectFdclDataIndMsgT_v02, inject_status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectFdclDataIndMsgT_v02, errorMsg) - QMI_IDL_OFFSET8(qmiLocInjectFdclDataIndMsgT_v02, errorMsg_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocInjectFdclDataIndMsgT_v02, errorMsg),
  QMI_LOC_INJECT_FDCL_DATA_ERROR_MSG_LEN_V02
};

static const uint8_t qmiLocSetBlacklistSvReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, glo_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, glo_persist_blacklist_sv_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, glo_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, glo_clear_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, glo_clear_persist_blacklist_sv_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, glo_clear_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, bds_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, bds_persist_blacklist_sv_valid)),
  0x12,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, bds_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, bds_clear_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, bds_clear_persist_blacklist_sv_valid)),
  0x13,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, bds_clear_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, qzss_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, qzss_persist_blacklist_sv_valid)),
  0x14,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, qzss_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, qzss_clear_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, qzss_clear_persist_blacklist_sv_valid)),
  0x15,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, qzss_clear_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, gal_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, gal_persist_blacklist_sv_valid)),
  0x16,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, gal_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, gal_clear_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, gal_clear_persist_blacklist_sv_valid)),
  0x17,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, gal_clear_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, sbas_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, sbas_persist_blacklist_sv_valid)),
  0x18,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, sbas_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, sbas_clear_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, sbas_clear_persist_blacklist_sv_valid)),
  0x19,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, sbas_clear_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, navic_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, navic_persist_blacklist_sv_valid)),
  0x1A,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, navic_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, navic_clear_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, navic_clear_persist_blacklist_sv_valid)),
  0x1B,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, navic_clear_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, gps_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, gps_persist_blacklist_sv_valid)),
  0x1C,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetBlacklistSvReqMsgT_v02, gps_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSetBlacklistSvReqMsgT_v02, gps_clear_persist_blacklist_sv) - QMI_IDL_OFFSET16RELATIVE(qmiLocSetBlacklistSvReqMsgT_v02, gps_clear_persist_blacklist_sv_valid)),
  0x1D,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSetBlacklistSvReqMsgT_v02, gps_clear_persist_blacklist_sv)
};

static const uint8_t qmiLocGetBlacklistSvIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, glo_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, glo_persist_blacklist_sv_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, glo_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, bds_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, bds_persist_blacklist_sv_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, bds_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, qzss_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, qzss_persist_blacklist_sv_valid)),
  0x12,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, qzss_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, gal_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, gal_persist_blacklist_sv_valid)),
  0x13,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, gal_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, sbas_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, sbas_persist_blacklist_sv_valid)),
  0x14,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, sbas_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, navic_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, navic_persist_blacklist_sv_valid)),
  0x15,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, navic_persist_blacklist_sv),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, gps_persist_blacklist_sv) - QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, gps_persist_blacklist_sv_valid)),
  0x16,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBlacklistSvIndMsgT_v02, gps_persist_blacklist_sv)
};

static const uint8_t qmiLocSetConstellationConfigReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetConstellationConfigReqMsgT_v02, resetConstellations),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetConstellationConfigReqMsgT_v02, enableMask) - QMI_IDL_OFFSET8(qmiLocSetConstellationConfigReqMsgT_v02, enableMask_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetConstellationConfigReqMsgT_v02, enableMask),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetConstellationConfigReqMsgT_v02, disableMask) - QMI_IDL_OFFSET8(qmiLocSetConstellationConfigReqMsgT_v02, disableMask_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetConstellationConfigReqMsgT_v02, disableMask)
};

static const uint8_t qmiLocEventDcReportIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDcReportIndMsgT_v02, msgType) - QMI_IDL_OFFSET8(qmiLocEventDcReportIndMsgT_v02, msgType_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDcReportIndMsgT_v02, msgType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDcReportIndMsgT_v02, numValidBits) - QMI_IDL_OFFSET8(qmiLocEventDcReportIndMsgT_v02, numValidBits_valid)),
  0x11,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDcReportIndMsgT_v02, numValidBits),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDcReportIndMsgT_v02, dcReportData) - QMI_IDL_OFFSET8(qmiLocEventDcReportIndMsgT_v02, dcReportData_valid)),
  0x12,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDcReportIndMsgT_v02, dcReportData),
  QMI_LOC_MAX_DCREPORT_LEN_V02,
  QMI_IDL_OFFSET8(qmiLocEventDcReportIndMsgT_v02, dcReportData) - QMI_IDL_OFFSET8(qmiLocEventDcReportIndMsgT_v02, dcReportData_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDcReportIndMsgT_v02, prn) - QMI_IDL_OFFSET8(qmiLocEventDcReportIndMsgT_v02, prn_valid)),
  0x13,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDcReportIndMsgT_v02, prn)
};

static const uint8_t qmiLocRegisterMasterClientReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocRegisterMasterClientReqMsgT_v02, key)
};

static const uint8_t qmiLocRegisterMasterClientIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocRegisterMasterClientIndMsgT_v02, status)
};

static const uint8_t qmiLocEventEngineLockStateIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventEngineLockStateIndMsgT_v02, engineLockState),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventEngineLockStateIndMsgT_v02, subType) - QMI_IDL_OFFSET8(qmiLocEventEngineLockStateIndMsgT_v02, subType_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventEngineLockStateIndMsgT_v02, subType),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventEngineLockStateIndMsgT_v02, lockClient) - QMI_IDL_OFFSET8(qmiLocEventEngineLockStateIndMsgT_v02, lockClient_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventEngineLockStateIndMsgT_v02, lockClient)
};

/*
 * qmiLocStartListenPositionReqMsgT is empty
 * static const uint8_t qmiLocStartListenPositionReqMsgT_data_v02[] = {
 * };
 */

/*
 * qmiLocStopListenPositionReqMsgT is empty
 * static const uint8_t qmiLocStopListenPositionReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocSystemInfoReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSystemInfoReqMsgT_v02, systemInfo)
};

static const uint8_t qmiLocSystemInfoIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, systemInfo),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, nextLeapSecondInfo) - QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, nextLeapSecondInfo_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, nextLeapSecondInfo),
  QMI_IDL_TYPE88(0, 88),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, system) - QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, system_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, system),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssWeekCurrent) - QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssWeekCurrent_valid)),
  0x12,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssWeekCurrent),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssTimeOfWeekCurrentMs) - QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssTimeOfWeekCurrentMs_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssTimeOfWeekCurrentMs),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssWeekNextLSEvent) - QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssWeekNextLSEvent_valid)),
  0x14,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssWeekNextLSEvent),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssTimeOfWeekNextLSEventMs) - QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssTimeOfWeekNextLSEventMs_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssTimeOfWeekNextLSEventMs),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssleapSecondsNext) - QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssleapSecondsNext_valid)),
  0x16,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssleapSecondsNext),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssleapSecondsCurrent) - QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssleapSecondsCurrent_valid)),
  0x17,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, gnssleapSecondsCurrent),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, dwellAlignTimeMs) - QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, dwellAlignTimeMs_valid)),
  0x18,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSystemInfoIndMsgT_v02, dwellAlignTimeMs)
};

static const uint8_t qmiLocGetConstellationConfigIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, gps_status) - QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, gps_status_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, gps_status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, glonass_status) - QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, glonass_status_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, glonass_status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, bds_status) - QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, bds_status_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, bds_status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, qzss_status) - QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, qzss_status_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, qzss_status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, galileo_status) - QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, galileo_status_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, galileo_status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, navic_status) - QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, navic_status_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetConstellationConfigIndMsgT_v02, navic_status)
};

static const uint8_t qmiLocEnablePositionInjectionTestModeReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnablePositionInjectionTestModeReqMsgT_v02, enableTestMode)
};

static const uint8_t qmiLocEnablePositionInjectionTestModeIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnablePositionInjectionTestModeIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEnablePositionInjectionTestModeIndMsgT_v02, testModeState) - QMI_IDL_OFFSET8(qmiLocEnablePositionInjectionTestModeIndMsgT_v02, testModeState_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnablePositionInjectionTestModeIndMsgT_v02, testModeState)
};

static const uint8_t qmiLocInjectTestPositionReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTestPositionReqMsgT_v02, latitude),

  0x02,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTestPositionReqMsgT_v02, longitude),

  0x03,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTestPositionReqMsgT_v02, horUncCircular),

  0x04,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTestPositionReqMsgT_v02, timestampUtc),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectTestPositionReqMsgT_v02, positionSource) - QMI_IDL_OFFSET8(qmiLocInjectTestPositionReqMsgT_v02, positionSource_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTestPositionReqMsgT_v02, positionSource)
};

static const uint8_t qmiLocInjectTestPositionIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectTestPositionIndMsgT_v02, status)
};

static const uint8_t qmiLocEventBsObsDataServiceReqIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventBsObsDataServiceReqIndMsgT_v02, numCellsAvailable)
};

static const uint8_t qmiLocGetBsObsDataReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBsObsDataReqMsgT_v02, timestampUtc) - QMI_IDL_OFFSET8(qmiLocGetBsObsDataReqMsgT_v02, timestampUtc_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBsObsDataReqMsgT_v02, timestampUtc),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBsObsDataReqMsgT_v02, numCellsUpload) - QMI_IDL_OFFSET8(qmiLocGetBsObsDataReqMsgT_v02, numCellsUpload_valid)),
  0x11,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBsObsDataReqMsgT_v02, numCellsUpload)
};

static const uint8_t qmiLocGetBsObsDataIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBsObsDataIndMsgT_v02, status),

  0x02,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetBsObsDataIndMsgT_v02, bsData),
  QMI_LOC_FDCL_BS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET8(qmiLocGetBsObsDataIndMsgT_v02, bsData) - QMI_IDL_OFFSET8(qmiLocGetBsObsDataIndMsgT_v02, bsData_len),
  QMI_IDL_TYPE88(0, 89),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGetBsObsDataIndMsgT_v02, moreBsAvailable) - QMI_IDL_OFFSET16RELATIVE(qmiLocGetBsObsDataIndMsgT_v02, moreBsAvailable_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGetBsObsDataIndMsgT_v02, moreBsAvailable)
};

static const uint8_t qmiLocGpsEphemerisReportIndMsgT_data_v02[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGpsEphemerisReportIndMsgT_v02, gpsEphemerisList),
  QMI_LOC_EPHEMERIS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET8(qmiLocGpsEphemerisReportIndMsgT_v02, gpsEphemerisList) - QMI_IDL_OFFSET8(qmiLocGpsEphemerisReportIndMsgT_v02, gpsEphemerisList_len),
  QMI_IDL_TYPE88(0, 91),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGpsEphemerisReportIndMsgT_v02, gpsSystemTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocGpsEphemerisReportIndMsgT_v02, gpsSystemTime_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGpsEphemerisReportIndMsgT_v02, gpsSystemTime),
  QMI_IDL_TYPE88(0, 62)
};

static const uint8_t qmiLocGloEphemerisReportIndMsgT_data_v02[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisReportIndMsgT_v02, gloEphemerisList),
  QMI_LOC_EPHEMERIS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET8(qmiLocGloEphemerisReportIndMsgT_v02, gloEphemerisList) - QMI_IDL_OFFSET8(qmiLocGloEphemerisReportIndMsgT_v02, gloEphemerisList_len),
  QMI_IDL_TYPE88(0, 92),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGloEphemerisReportIndMsgT_v02, gpsSystemTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocGloEphemerisReportIndMsgT_v02, gpsSystemTime_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGloEphemerisReportIndMsgT_v02, gpsSystemTime),
  QMI_IDL_TYPE88(0, 62)
};

static const uint8_t qmiLocBdsEphemerisReportIndMsgT_data_v02[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocBdsEphemerisReportIndMsgT_v02, bdsEphemerisList),
  QMI_LOC_EPHEMERIS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET8(qmiLocBdsEphemerisReportIndMsgT_v02, bdsEphemerisList) - QMI_IDL_OFFSET8(qmiLocBdsEphemerisReportIndMsgT_v02, bdsEphemerisList_len),
  QMI_IDL_TYPE88(0, 93),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocBdsEphemerisReportIndMsgT_v02, gpsSystemTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocBdsEphemerisReportIndMsgT_v02, gpsSystemTime_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocBdsEphemerisReportIndMsgT_v02, gpsSystemTime),
  QMI_IDL_TYPE88(0, 62)
};

static const uint8_t qmiLocGalEphemerisReportIndMsgT_data_v02[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGalEphemerisReportIndMsgT_v02, galEphemerisList),
  QMI_LOC_EPHEMERIS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET8(qmiLocGalEphemerisReportIndMsgT_v02, galEphemerisList) - QMI_IDL_OFFSET8(qmiLocGalEphemerisReportIndMsgT_v02, galEphemerisList_len),
  QMI_IDL_TYPE88(0, 94),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocGalEphemerisReportIndMsgT_v02, gpsSystemTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocGalEphemerisReportIndMsgT_v02, gpsSystemTime_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocGalEphemerisReportIndMsgT_v02, gpsSystemTime),
  QMI_IDL_TYPE88(0, 62)
};

static const uint8_t qmiLocQzssEphemerisReportIndMsgT_data_v02[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocQzssEphemerisReportIndMsgT_v02, qzssEphemerisList),
  QMI_LOC_EPHEMERIS_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET8(qmiLocQzssEphemerisReportIndMsgT_v02, qzssEphemerisList) - QMI_IDL_OFFSET8(qmiLocQzssEphemerisReportIndMsgT_v02, qzssEphemerisList_len),
  QMI_IDL_TYPE88(0, 91),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocQzssEphemerisReportIndMsgT_v02, gpsSystemTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocQzssEphemerisReportIndMsgT_v02, gpsSystemTime_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocQzssEphemerisReportIndMsgT_v02, gpsSystemTime),
  QMI_IDL_TYPE88(0, 62)
};

static const uint8_t qmiLocSetConstrainedTuncModeReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetConstrainedTuncModeReqMsgT_v02, tuncConstraintOn),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetConstrainedTuncModeReqMsgT_v02, tuncConstraint) - QMI_IDL_OFFSET8(qmiLocSetConstrainedTuncModeReqMsgT_v02, tuncConstraint_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetConstrainedTuncModeReqMsgT_v02, tuncConstraint),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetConstrainedTuncModeReqMsgT_v02, energyBudget) - QMI_IDL_OFFSET8(qmiLocSetConstrainedTuncModeReqMsgT_v02, energyBudget_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetConstrainedTuncModeReqMsgT_v02, energyBudget)
};

static const uint8_t qmiLocSetConstrainedTuncModeIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetConstrainedTuncModeIndMsgT_v02, status)
};

static const uint8_t qmiLocEnablePositionAssistedClockEstReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnablePositionAssistedClockEstReqMsgT_v02, enablePositionAssistedClockEst)
};

static const uint8_t qmiLocEnablePositionAssistedClockEstIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEnablePositionAssistedClockEstIndMsgT_v02, status)
};

/*
 * qmiLocQueryGNSSEnergyConsumedReqMsgT is empty
 * static const uint8_t qmiLocQueryGNSSEnergyConsumedReqMsgT_data_v02[] = {
 * };
 */

static const uint8_t qmiLocQueryGNSSEnergyConsumedIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryGNSSEnergyConsumedIndMsgT_v02, energyConsumedSinceLastBoot),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocQueryGNSSEnergyConsumedIndMsgT_v02, energyConsumedSinceFirstBoot)
};

static const uint8_t qmiLocEventReportIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, eventReport),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, gpsSystemTime) - QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, gpsSystemTime_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, gpsSystemTime),
  QMI_IDL_TYPE88(0, 62),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, klobucharIonoModel) - QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, klobucharIonoModel_valid)),
  0x11,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, klobucharIonoModel),
  QMI_IDL_TYPE88(0, 95),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, tauC) - QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, tauC_valid)),
  0x12,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, tauC),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, leapSec) - QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, leapSec_valid)),
  0x13,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, leapSec),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, featureStatusReport) - QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, featureStatusReport_valid)),
  0x14,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventReportIndMsgT_v02, featureStatusReport)
};

static const uint8_t qmiLocGetBandMeasurementMetricsIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, systemTick),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, band1PgaGainDb),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gpsL1caRfStats) - QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gpsL1caRfStats_valid)),
  0x10,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gpsL1caRfStats),
  QMI_IDL_TYPE88(0, 96),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gloG1RfStats) - QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gloG1RfStats_valid)),
  0x11,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gloG1RfStats),
  QMI_IDL_TYPE88(0, 96),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, bdsB1RfStats) - QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, bdsB1RfStats_valid)),
  0x12,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, bdsB1RfStats),
  QMI_IDL_TYPE88(0, 96),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, galE1RfStats) - QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, galE1RfStats_valid)),
  0x13,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, galE1RfStats),
  QMI_IDL_TYPE88(0, 96),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gpsL2cRfStats) - QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gpsL2cRfStats_valid)),
  0x14,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gpsL2cRfStats),
  QMI_IDL_TYPE88(0, 96),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gloG2RfStats) - QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gloG2RfStats_valid)),
  0x15,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gloG2RfStats),
  QMI_IDL_TYPE88(0, 96),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, bdsB2RfStats) - QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, bdsB2RfStats_valid)),
  0x16,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, bdsB2RfStats),
  QMI_IDL_TYPE88(0, 96),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, band5PgaGainDb) - QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, band5PgaGainDb_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, band5PgaGainDb),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gpsL5RfStats) - QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gpsL5RfStats_valid)),
  0x18,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, gpsL5RfStats),
  QMI_IDL_TYPE88(0, 96),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, galE5ARfStats) - QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, galE5ARfStats_valid)),
  0x19,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetBandMeasurementMetricsIndMsgT_v02, galE5ARfStats),
  QMI_IDL_TYPE88(0, 96)
};

static const uint8_t qmiLocLocationRequestNotificationIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocLocationRequestNotificationIndMsgT_v02, protocolStack),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocLocationRequestNotificationIndMsgT_v02, requestor),

  0x03,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocLocationRequestNotificationIndMsgT_v02, requestorId),
  QMI_LOC_MAX_REQUESTOR_ID_STRING_LENGTH_V02,

  0x04,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocLocationRequestNotificationIndMsgT_v02, responseType),

  0x05,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocLocationRequestNotificationIndMsgT_v02, responseProtocol),

  0x06,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocLocationRequestNotificationIndMsgT_v02, inEmergencyMode),

  0x07,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocLocationRequestNotificationIndMsgT_v02, isCachedLocation),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocLocationRequestNotificationIndMsgT_v02, clientStrId) - QMI_IDL_OFFSET8(qmiLocLocationRequestNotificationIndMsgT_v02, clientStrId_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocLocationRequestNotificationIndMsgT_v02, clientStrId),
  QMI_LOC_MAX_CLIENT_ID_STRING_LENGTH_V02
};

static const uint8_t qmiLocInjectPlatformPowerStateReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPlatformPowerStateReqMsgT_v02, powerState)
};

static const uint8_t qmiLocInjectPlatformPowerStateIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectPlatformPowerStateIndMsgT_v02, status)
};

static const uint8_t qmiLocSetRobustLocationReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetRobustLocationReqMsgT_v02, enable),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetRobustLocationReqMsgT_v02, enableForE911) - QMI_IDL_OFFSET8(qmiLocSetRobustLocationReqMsgT_v02, enableForE911_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetRobustLocationReqMsgT_v02, enableForE911)
};

static const uint8_t qmiLocGetRobustLocationConfigIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetRobustLocationConfigIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetRobustLocationConfigIndMsgT_v02, isEnabled) - QMI_IDL_OFFSET8(qmiLocGetRobustLocationConfigIndMsgT_v02, isEnabled_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetRobustLocationConfigIndMsgT_v02, isEnabled),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetRobustLocationConfigIndMsgT_v02, isEnabledForE911) - QMI_IDL_OFFSET8(qmiLocGetRobustLocationConfigIndMsgT_v02, isEnabledForE911_valid)),
  0x11,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetRobustLocationConfigIndMsgT_v02, isEnabledForE911),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetRobustLocationConfigIndMsgT_v02, robustLocationVersion) - QMI_IDL_OFFSET8(qmiLocGetRobustLocationConfigIndMsgT_v02, robustLocationVersion_valid)),
  0x12,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocGetRobustLocationConfigIndMsgT_v02, robustLocationVersion),
  QMI_IDL_TYPE88(0, 97)
};

static const uint8_t qmiLocEventInjectEnvAidingReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, seqNum),

  0x02,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, maxMessageNum),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, envBearingValidity) - QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, envBearingValidity_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, envBearingValidity),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, envBearingDegrees) - QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, envBearingDegrees_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, envBearingDegrees),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, envBearingUncDegrees) - QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, envBearingUncDegrees_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, envBearingUncDegrees),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, latitudeDegrees) - QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, latitudeDegrees_valid)),
  0x13,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, latitudeDegrees),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, longitudeDegrees) - QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, longitudeDegrees_valid)),
  0x14,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, longitudeDegrees),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, horizontalPositionUncMeters) - QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, horizontalPositionUncMeters_valid)),
  0x15,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, horizontalPositionUncMeters),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, altitudeMeters) - QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, altitudeMeters_valid)),
  0x16,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, altitudeMeters),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, altitudeUncMeters) - QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, altitudeUncMeters_valid)),
  0x17,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, altitudeUncMeters),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, toaGpsNanosecondsOfWeek) - QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, toaGpsNanosecondsOfWeek_valid)),
  0x18,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, toaGpsNanosecondsOfWeek),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, svCorrection) - QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, svCorrection_valid)),
  0x19,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, svCorrection),
  QMI_LOC_ENV_AIDING_CORRECTION_MAX_SV_USED_V02,
  QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, svCorrection) - QMI_IDL_OFFSET8(qmiLocEventInjectEnvAidingReqMsgT_v02, svCorrection_len),
  QMI_IDL_TYPE88(0, 99)
};

static const uint8_t qmiLocSetMinGpsWeekNumberReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetMinGpsWeekNumberReqMsgT_v02, minGpsWeekNumber)
};

static const uint8_t qmiLocGetMinGpsWeekNumberIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetMinGpsWeekNumberIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetMinGpsWeekNumberIndMsgT_v02, minGpsWeekNumber) - QMI_IDL_OFFSET8(qmiLocGetMinGpsWeekNumberIndMsgT_v02, minGpsWeekNumber_valid)),
  0x10,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetMinGpsWeekNumberIndMsgT_v02, minGpsWeekNumber)
};

static const uint8_t qmiLocSapInsParamsIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSapInsParamsIndMsgT_v02, status),

  0x02,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSapInsParamsIndMsgT_v02, fixStatus),

  0x03,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocSapInsParamsIndMsgT_v02, systemTime),
  QMI_IDL_TYPE88(0, 62),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSapInsParamsIndMsgT_v02, state) - QMI_IDL_OFFSET8(qmiLocSapInsParamsIndMsgT_v02, state_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSapInsParamsIndMsgT_v02, state),
  QMI_LOC_FILTER_ELEMENT_SIZE_SEVENTEEN_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSapInsParamsIndMsgT_v02, var) - QMI_IDL_OFFSET8(qmiLocSapInsParamsIndMsgT_v02, var_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSapInsParamsIndMsgT_v02, var),
  QMI_LOC_FILTER_ELEMENT_SIZE_SEVENTEEN_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, rel1) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, rel1_valid)),
  0x12,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, rel1),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, rel2) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, rel2_valid)),
  0x13,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, rel2),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, residual1) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, residual1_valid)),
  0x14,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, residual1),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, obs1) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, obs1_valid)),
  0x15,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, obs1),
  QMI_LOC_FILTER_ELEMENT_SIZE_SIX_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, var1) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, var1_valid)),
  0x16,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, var1),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, result1) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, result1_valid)),
  0x17,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, result1),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, residual2) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, residual2_valid)),
  0x18,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, residual2),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, obs2) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, obs2_valid)),
  0x19,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, obs2),
  QMI_LOC_FILTER_ELEMENT_SIZE_SIX_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, var2) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, var2_valid)),
  0x1A,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, var2),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, result2) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, result2_valid)),
  0x1B,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, result2),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, residual3) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, residual3_valid)),
  0x1C,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, residual3),
  QMI_LOC_FILTER_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, var3) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, var3_valid)),
  0x1D,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, var3),
  QMI_LOC_FILTER_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, result3) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, result3_valid)),
  0x1E,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, result3),
  QMI_LOC_FILTER_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, residual4) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, residual4_valid)),
  0x1F,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, residual4),
  QMI_LOC_FILTER_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, var4) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, var4_valid)),
  0x20,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, var4),
  QMI_LOC_FILTER_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, result4) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, result4_valid)),
  0x21,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, result4),
  QMI_LOC_FILTER_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, acc) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, acc_valid)),
  0x22,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, acc),
  QMI_LOC_FILTER_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, quat) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, quat_valid)),
  0x23,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, quat),
  QMI_LOC_FILTER_ELEMENT_SIZE_FOUR_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, biasM1) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, biasM1_valid)),
  0x24,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, biasM1),
  QMI_LOC_IPM_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, biasV1) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, biasV1_valid)),
  0x25,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, biasV1),
  QMI_LOC_IPM_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, biasM2) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, biasM2_valid)),
  0x26,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, biasM2),
  QMI_LOC_IPM_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, biasV2) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, biasV2_valid)),
  0x27,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, biasV2),
  QMI_LOC_IPM_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, rMat1) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, rMat1_valid)),
  0x28,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, rMat1),
  QMI_LOC_IPM_ELEMENT_SIZE_NINE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, rMat1Count) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, rMat1Count_valid)),
  0x29,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, rMat1Count),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, rMat2) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, rMat2_valid)),
  0x2A,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, rMat2),
  QMI_LOC_IPM_ELEMENT_SIZE_NINE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det1Reset) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det1Reset_valid)),
  0x2B,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, det1Reset),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det2Status) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det2Status_valid)),
  0x2C,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, det2Status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det2Position) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det2Position_valid)),
  0x2D,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, det2Position),
  QMI_LOC_INS_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det2PositionUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det2PositionUnc_valid)),
  0x2E,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, det2PositionUnc),
  QMI_LOC_INS_ELEMENT_SIZE_TWO_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det3Status) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det3Status_valid)),
  0x2F,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, det3Status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det3Variance1) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det3Variance1_valid)),
  0x30,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, det3Variance1),
  QMI_LOC_INS_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det3Variance2) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det3Variance2_valid)),
  0x31,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, det3Variance2),
  QMI_LOC_INS_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det4Status) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det4Status_valid)),
  0x32,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, det4Status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det4Position) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det4Position_valid)),
  0x33,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, det4Position),
  QMI_LOC_INS_ELEMENT_SIZE_THREE_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det5Status) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det5Status_valid)),
  0x34,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, det5Status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det6Status) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, det6Status_valid)),
  0x35,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, det6Status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, gnssSvId) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, gnssSvId_valid)),
  0x36,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, gnssSvId),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, gnssSvId) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, gnssSvId_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measType) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measType_valid)),
  0x37,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, measType),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measType) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measType_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measVar1) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measVar1_valid)),
  0x38,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, measVar1),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measVar1) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measVar1_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measUse1) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measUse1_valid)),
  0x39,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, measUse1),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measUse1) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measUse1_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measVar2) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measVar2_valid)),
  0x3A,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, measVar2),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measVar2) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measVar2_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measUse2) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measUse2_valid)),
  0x3B,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocSapInsParamsIndMsgT_v02, measUse2),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measUse2) - QMI_IDL_OFFSET16RELATIVE(qmiLocSapInsParamsIndMsgT_v02, measUse2_len)
};

static const uint8_t qmiLocEventQueryXtraInfoReqIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, usableConstellationConfig) - QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, usableConstellationConfig_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, usableConstellationConfig),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, forcedEnableConfig) - QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, forcedEnableConfig_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, forcedEnableConfig),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, multibandEnabled) - QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, multibandEnabled_valid)),
  0x12,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, multibandEnabled),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, xtraFileVersion) - QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, xtraFileVersion_valid)),
  0x13,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, xtraFileVersion),
  QMI_IDL_TYPE88(0, 81),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, mccPrimarySimSlot) - QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, mccPrimarySimSlot_valid)),
  0x14,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, mccPrimarySimSlot),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, mccSecondarySimSlot) - QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, mccSecondarySimSlot_valid)),
  0x15,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, mccSecondarySimSlot),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, mccTertiarySimSlot) - QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, mccTertiarySimSlot_valid)),
  0x16,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, mccTertiarySimSlot),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, xtraFileGenerationTime) - QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, xtraFileGenerationTime_valid)),
  0x17,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, xtraFileGenerationTime),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, xtraRemValidDuration) - QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, xtraRemValidDuration_valid)),
  0x18,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, xtraRemValidDuration),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, preciseXtraStatus) - QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, preciseXtraStatus_valid)),
  0x19,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventQueryXtraInfoReqIndMsgT_v02, preciseXtraStatus)
};

static const uint8_t qmiLocSetParameterReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetParameterReqMsgT_v02, paramType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetParameterReqMsgT_v02, minSvElevation) - QMI_IDL_OFFSET8(qmiLocSetParameterReqMsgT_v02, minSvElevation_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetParameterReqMsgT_v02, minSvElevation),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetParameterReqMsgT_v02, constellationConfig) - QMI_IDL_OFFSET8(qmiLocSetParameterReqMsgT_v02, constellationConfig_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetParameterReqMsgT_v02, constellationConfig)
};

static const uint8_t qmiLocGetParameterReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetParameterReqMsgT_v02, paramType)
};

static const uint8_t qmiLocGetParameterIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetParameterIndMsgT_v02, status),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetParameterIndMsgT_v02, paramType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetParameterIndMsgT_v02, minSvElevation) - QMI_IDL_OFFSET8(qmiLocGetParameterIndMsgT_v02, minSvElevation_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetParameterIndMsgT_v02, minSvElevation),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetParameterIndMsgT_v02, constellationConfig) - QMI_IDL_OFFSET8(qmiLocGetParameterIndMsgT_v02, constellationConfig_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetParameterIndMsgT_v02, constellationConfig)
};

static const uint8_t qmiLocSetStatisticsConfigReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetStatisticsConfigReqMsgT_v02, enable),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetStatisticsConfigReqMsgT_v02, interval) - QMI_IDL_OFFSET8(qmiLocSetStatisticsConfigReqMsgT_v02, interval_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetStatisticsConfigReqMsgT_v02, interval)
};

static const uint8_t qmiLocGetStatisticsConfigIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetStatisticsConfigIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetStatisticsConfigIndMsgT_v02, enabled) - QMI_IDL_OFFSET8(qmiLocGetStatisticsConfigIndMsgT_v02, enabled_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetStatisticsConfigIndMsgT_v02, enabled),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetStatisticsConfigIndMsgT_v02, interval) - QMI_IDL_OFFSET8(qmiLocGetStatisticsConfigIndMsgT_v02, interval_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetStatisticsConfigIndMsgT_v02, interval)
};

static const uint8_t qmiLocGnssStatisticsReportIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, absValidAge) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, absValidAge_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, absValidAge),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, averagePowerConsumed) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, averagePowerConsumed_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, averagePowerConsumed),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, powerConsumptionDuration) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, powerConsumptionDuration_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, powerConsumptionDuration),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, latitude) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, latitude_valid)),
  0x13,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, latitude),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, longitude) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, longitude_valid)),
  0x14,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, longitude),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, altitudeWrtEllipsoid) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, altitudeWrtEllipsoid_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, altitudeWrtEllipsoid),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, altitudeWrtMeanSeaLevel) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, altitudeWrtMeanSeaLevel_valid)),
  0x16,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, altitudeWrtMeanSeaLevel),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horUncCircular) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horUncCircular_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horUncCircular),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertUnc) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertUnc_valid)),
  0x18,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horUncEllipseSemiMinor) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horUncEllipseSemiMinor_valid)),
  0x19,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horUncEllipseSemiMinor),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horUncEllipseSemiMajor) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horUncEllipseSemiMajor_valid)),
  0x1A,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horUncEllipseSemiMajor),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horCircularConfidence) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horCircularConfidence_valid)),
  0x1B,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horCircularConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horReliability) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horReliability_valid)),
  0x1C,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horReliability),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horSpeed) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horSpeed_valid)),
  0x1D,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horSpeed),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horSpeedUnc) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horSpeedUnc_valid)),
  0x1E,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, horSpeedUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertConfidence) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertConfidence_valid)),
  0x1F,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertReliability) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertReliability_valid)),
  0x20,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertReliability),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertSpeed) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertSpeed_valid)),
  0x21,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertSpeed),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertSpeedUnc) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertSpeedUnc_valid)),
  0x22,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, vertSpeedUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, gpsWeek) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, gpsWeek_valid)),
  0x23,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, gpsWeek),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, gpsMsec) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, gpsMsec_valid)),
  0x24,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, gpsMsec),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, clkTimeBias) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, clkTimeBias_valid)),
  0x25,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, clkTimeBias),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, clkTimeBiasUnc) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, clkTimeBiasUnc_valid)),
  0x26,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, clkTimeBiasUnc),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, pdrEngagementRate) - QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, pdrEngagementRate_valid)),
  0x27,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssStatisticsReportIndMsgT_v02, pdrEngagementRate)
};

static const uint8_t qmiLocSetGnssPdrModeReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetGnssPdrModeReqMsgT_v02, mode) - QMI_IDL_OFFSET8(qmiLocSetGnssPdrModeReqMsgT_v02, mode_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetGnssPdrModeReqMsgT_v02, mode)
};

static const uint8_t qmiLocGetGnssPdrModeIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetGnssPdrModeIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetGnssPdrModeIndMsgT_v02, mode) - QMI_IDL_OFFSET8(qmiLocGetGnssPdrModeIndMsgT_v02, mode_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetGnssPdrModeIndMsgT_v02, mode)
};

static const uint8_t qmiLocGetMagCalStatePdrIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetMagCalStatePdrIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetMagCalStatePdrIndMsgT_v02, isCalibrated) - QMI_IDL_OFFSET8(qmiLocGetMagCalStatePdrIndMsgT_v02, isCalibrated_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetMagCalStatePdrIndMsgT_v02, isCalibrated)
};

static const uint8_t qmiLocSetMultibandConfigReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetMultibandConfigReqMsgT_v02, secondaryGnssConfig) - QMI_IDL_OFFSET8(qmiLocSetMultibandConfigReqMsgT_v02, secondaryGnssConfig_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetMultibandConfigReqMsgT_v02, secondaryGnssConfig)
};

static const uint8_t qmiLocGetMultibandConfigIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetMultibandConfigIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetMultibandConfigIndMsgT_v02, secondaryGnssConfig) - QMI_IDL_OFFSET8(qmiLocGetMultibandConfigIndMsgT_v02, secondaryGnssConfig_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetMultibandConfigIndMsgT_v02, secondaryGnssConfig)
};

static const uint8_t qmiLocLatencyInformationIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, latencyInfo),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt1) - QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt1_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt1),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt2) - QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt2_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt2),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt3) - QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt3_valid)),
  0x12,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt3),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt4) - QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt4_valid)),
  0x13,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt4),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt5) - QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt5_valid)),
  0x14,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt5),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt6) - QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt6_valid)),
  0x15,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt6),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt7) - QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt7_valid)),
  0x16,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt7),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt8) - QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt8_valid)),
  0x17,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt8),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt9) - QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt9_valid)),
  0x18,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt9),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt10) - QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt10_valid)),
  0x19,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, sysTickAtChkPt10),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, fCountOfMeasBlk) - QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, fCountOfMeasBlk_valid)),
  0x1A,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocLatencyInformationIndMsgT_v02, fCountOfMeasBlk)
};

static const uint8_t qmiLocInjectLocationCivicAddressReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, latitude) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, latitude_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, latitude),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, longitude) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, longitude_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, longitude),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, horUncCircular) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, horUncCircular_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, horUncCircular),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, horConfidence) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, horConfidence_valid)),
  0x13,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, horConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, altitudeWrtEllipsoid) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, altitudeWrtEllipsoid_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, altitudeWrtEllipsoid),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, altitudeWrtMeanSeaLevel) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, altitudeWrtMeanSeaLevel_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, altitudeWrtMeanSeaLevel),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, vertUnc) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, vertUnc_valid)),
  0x16,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, vertUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, vertConfidence) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, vertConfidence_valid)),
  0x17,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, vertConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, source) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, source_valid)),
  0x18,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, source),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, timestampUtc) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, timestampUtc_valid)),
  0x19,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, timestampUtc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, timestampAge) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, timestampAge_valid)),
  0x1A,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, timestampAge),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, positionSrc) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, positionSrc_valid)),
  0x1B,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, positionSrc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, country) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, country_valid)),
  0x1C,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, country),
  QMI_LOC_MAX_COUNTRY_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, subdivA1) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, subdivA1_valid)),
  0x1D,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, subdivA1),
  QMI_LOC_MAX_SUBDIV_A1_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, subdivA2) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, subdivA2_valid)),
  0x1E,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, subdivA2),
  QMI_LOC_MAX_SUBDIV_A2_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, city) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, city_valid)),
  0x1F,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, city),
  QMI_LOC_MAX_CITY_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, cityDiv) - QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, cityDiv_valid)),
  0x20,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_STRING,
  QMI_IDL_OFFSET8(qmiLocInjectLocationCivicAddressReqMsgT_v02, cityDiv),
  QMI_LOC_MAX_CITYDIV_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, neighborhood) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, neighborhood_valid)),
  0x21,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, neighborhood),
  QMI_LOC_MAX_NEIGHBORHOOD_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, street) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, street_valid)),
  0x22,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, street),
  QMI_LOC_MAX_STREET_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, leadingStreetDirection) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, leadingStreetDirection_valid)),
  0x23,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, leadingStreetDirection),
  QMI_LOC_MAX_LEADING_STREET_DIR_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, trailingStreetSuffix) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, trailingStreetSuffix_valid)),
  0x24,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, trailingStreetSuffix),
  QMI_LOC_MAX_TRAILING_STREET_SUFFIX_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, streetSuffix) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, streetSuffix_valid)),
  0x25,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, streetSuffix),
  QMI_LOC_MAX_STREET_SUFFIX_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, houseNumber) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, houseNumber_valid)),
  0x26,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, houseNumber),
  QMI_LOC_MAX_HOUSE_NUMBER_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, houserNumberSuffix) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, houserNumberSuffix_valid)),
  0x27,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, houserNumberSuffix),
  QMI_LOC_MAX_HOUSE_NUMBER_SUFFIX_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, landmark) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, landmark_valid)),
  0x28,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, landmark),
  QMI_LOC_MAX_LANDMARK_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, additionalLocInfo) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, additionalLocInfo_valid)),
  0x29,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, additionalLocInfo),
  QMI_LOC_MAX_ADDITIONAL_LOC_INFO_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, floor) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, floor_valid)),
  0x2A,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, floor),
  QMI_LOC_MAX_FLOOR_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, name) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, name_valid)),
  0x2B,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, name),
  QMI_LOC_MAX_NAME_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, postalCode) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, postalCode_valid)),
  0x2C,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, postalCode),
  QMI_LOC_MAX_POSTAL_CODE_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, positioningMethod) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, positioningMethod_valid)),
  0x2D,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, positioningMethod),
  QMI_LOC_MAX_POSITIONING_METHOD_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, building) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, building_valid)),
  0x2E,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, building),
  QMI_LOC_MAX_BUILDING_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, unit) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, unit_valid)),
  0x2F,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, unit),
  QMI_LOC_MAX_UNIT_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, room) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, room_valid)),
  0x30,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, room),
  QMI_LOC_MAX_ROOM_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, placeType) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, placeType_valid)),
  0x31,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, placeType),
  QMI_LOC_MAX_PLACE_TYPE_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, postalCommunityName) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, postalCommunityName_valid)),
  0x32,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, postalCommunityName),
  QMI_LOC_MAX_POSTAL_COMM_TYPE_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, poBox) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, poBox_valid)),
  0x33,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, poBox),
  QMI_LOC_MAX_POST_OFFICE_BOX_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, additionalCode) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, additionalCode_valid)),
  0x34,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, additionalCode),
  QMI_LOC_MAX_ADDITIONAL_CODE_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, seat) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, seat_valid)),
  0x35,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, seat),
  QMI_LOC_MAX_SEAT_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, primaryRoad) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, primaryRoad_valid)),
  0x36,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, primaryRoad),
  QMI_LOC_MAX_PRIMARY_ROAD_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadSection) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadSection_valid)),
  0x37,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadSection),
  QMI_LOC_MAX_ROAD_SECTION_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadBranch) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadBranch_valid)),
  0x38,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadBranch),
  QMI_LOC_MAX_ROAD_BRANCH_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadSubBranch) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadSubBranch_valid)),
  0x39,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadSubBranch),
  QMI_LOC_MAX_ROAD_SUB_BRANCH_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadPreModifier) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadPreModifier_valid)),
  0x3A,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadPreModifier),
  QMI_LOC_MAX_ROAD_PRE_MODIFIER_STR_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadPostModifier) - QMI_IDL_OFFSET16RELATIVE(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadPostModifier_valid)),
  0x3B,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET16ARRAY(qmiLocInjectLocationCivicAddressReqMsgT_v02, roadPostModifier),
  QMI_LOC_MAX_ROAD_POST_MODIFIER_STR_LENGTH_V02
};

static const uint8_t qmiLocPlatformPowerStateChangedIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocPlatformPowerStateChangedIndMsgT_v02, powerStateOld) - QMI_IDL_OFFSET8(qmiLocPlatformPowerStateChangedIndMsgT_v02, powerStateOld_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPlatformPowerStateChangedIndMsgT_v02, powerStateOld),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocPlatformPowerStateChangedIndMsgT_v02, powerStateNew) - QMI_IDL_OFFSET8(qmiLocPlatformPowerStateChangedIndMsgT_v02, powerStateNew_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocPlatformPowerStateChangedIndMsgT_v02, powerStateNew)
};

static const uint8_t qmiLocInjectRawDataReqMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectRawDataReqMsgT_v02, injectionType),

  0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectRawDataReqMsgT_v02, totalSize),

  0x03,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectRawDataReqMsgT_v02, totalParts),

  0x04,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectRawDataReqMsgT_v02, partNum),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x05,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectRawDataReqMsgT_v02, partData),
  ((QMI_LOC_MAX_RAW_DATA_PART_LEN_V02) & 0xFF), ((QMI_LOC_MAX_RAW_DATA_PART_LEN_V02) >> 8),
  QMI_IDL_OFFSET8(qmiLocInjectRawDataReqMsgT_v02, partData) - QMI_IDL_OFFSET8(qmiLocInjectRawDataReqMsgT_v02, partData_len)
};

static const uint8_t qmiLocInjectRawDataIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectRawDataIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocInjectRawDataIndMsgT_v02, partNum) - QMI_IDL_OFFSET8(qmiLocInjectRawDataIndMsgT_v02, partNum_valid)),
  0x10,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocInjectRawDataIndMsgT_v02, partNum)
};

static const uint8_t qmiLocEngineDebugDataIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, week) - QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, week_valid)),
  0x10,
   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, week),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, timeOfWeek) - QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, timeOfWeek_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, timeOfWeek),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, sourceOfTime) - QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, sourceOfTime_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, sourceOfTime),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, clkTimeUnc) - QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, clkTimeUnc_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, clkTimeUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, clkFreqBias) - QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, clkFreqBias_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, clkFreqBias),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, clkFreqUnc) - QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, clkFreqUnc_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, clkFreqUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, xoState) - QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, xoState_valid)),
  0x16,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, xoState),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, rcvrErrRecovery) - QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, rcvrErrRecovery_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, rcvrErrRecovery),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, leapSecondInfo) - QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, leapSecondInfo_valid)),
  0x18,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, leapSecondInfo),
  QMI_IDL_TYPE88(0, 60),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, jammedSignalsMask) - QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, jammedSignalsMask_valid)),
  0x19,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, jammedSignalsMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, jammerIndicatorList) - QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, jammerIndicatorList_valid)),
  0x1A,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, jammerIndicatorList),
  QMI_LOC_MAX_GNSS_SIGNAL_TYPE_EXT_V02,
  QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, jammerIndicatorList) - QMI_IDL_OFFSET8(qmiLocEngineDebugDataIndMsgT_v02, jammerIndicatorList_len),
  QMI_IDL_TYPE88(0, 7),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, epiTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, epiTime_valid)),
  0x1B,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, epiTime),
  QMI_IDL_TYPE88(0, 101),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, epiLat) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, epiLat_valid)),
  0x1C,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, epiLat),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, epiLon) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, epiLon_valid)),
  0x1D,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, epiLon),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, epiAlt) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, epiAlt_valid)),
  0x1E,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, epiAlt),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, epiHepe) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, epiHepe_valid)),
  0x1F,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, epiHepe),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, epiAltUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, epiAltUnc_valid)),
  0x20,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, epiAltUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, epiSrc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, epiSrc_valid)),
  0x21,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, epiSrc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bestPosTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bestPosTime_valid)),
  0x22,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, bestPosTime),
  QMI_IDL_TYPE88(0, 101),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bestPosLat) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bestPosLat_valid)),
  0x23,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, bestPosLat),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bestPosLon) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bestPosLon_valid)),
  0x24,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, bestPosLon),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bestPosAlt) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bestPosAlt_valid)),
  0x25,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, bestPosAlt),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bestPosHepe) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bestPosHepe_valid)),
  0x26,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, bestPosHepe),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bestPosAltUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bestPosAltUnc_valid)),
  0x27,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, bestPosAltUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, xtraInfoTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, xtraInfoTime_valid)),
  0x28,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, xtraInfoTime),
  QMI_IDL_TYPE88(0, 101),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gpsXtraAge) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gpsXtraAge_valid)),
  0x29,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gpsXtraAge),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gloXtraAge) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gloXtraAge_valid)),
  0x2A,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gloXtraAge),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bdsXtraAge) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bdsXtraAge_valid)),
  0x2B,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, bdsXtraAge),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, galXtraAge) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, galXtraAge_valid)),
  0x2C,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, galXtraAge),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, qzssXtraAge) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, qzssXtraAge_valid)),
  0x2D,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, qzssXtraAge),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navicXtraAge) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navicXtraAge_valid)),
  0x2E,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, navicXtraAge),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gpsXtraMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gpsXtraMask_valid)),
  0x2F,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gpsXtraMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gloXtraMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gloXtraMask_valid)),
  0x30,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gloXtraMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bdsXtraMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bdsXtraMask_valid)),
  0x31,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, bdsXtraMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, galXtraMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, galXtraMask_valid)),
  0x32,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, galXtraMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, qzssXtraMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, qzssXtraMask_valid)),
  0x33,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, qzssXtraMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navicXtraMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navicXtraMask_valid)),
  0x34,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, navicXtraMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, ephInfoTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, ephInfoTime_valid)),
  0x35,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, ephInfoTime),
  QMI_IDL_TYPE88(0, 101),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gpsEphMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gpsEphMask_valid)),
  0x36,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gpsEphMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gloEphMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gloEphMask_valid)),
  0x37,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gloEphMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bdsEphMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bdsEphMask_valid)),
  0x38,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, bdsEphMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, galEphMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, galEphMask_valid)),
  0x39,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, galEphMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, qzssEphMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, qzssEphMask_valid)),
  0x3A,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, qzssEphMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navicEphMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navicEphMask_valid)),
  0x3B,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, navicEphMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, healthInfoTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, healthInfoTime_valid)),
  0x3C,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, healthInfoTime),
  QMI_IDL_TYPE88(0, 101),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gpsHealthUnknownMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gpsHealthUnknownMask_valid)),
  0x3D,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gpsHealthUnknownMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gloHealthUnknownMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gloHealthUnknownMask_valid)),
  0x3E,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gloHealthUnknownMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bdsHealthUnknownMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bdsHealthUnknownMask_valid)),
  0x3F,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, bdsHealthUnknownMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, galHealthUnknownMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, galHealthUnknownMask_valid)),
  0x40,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, galHealthUnknownMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, qzssHealthUnknownMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, qzssHealthUnknownMask_valid)),
  0x41,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, qzssHealthUnknownMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navicHealthUnknownMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navicHealthUnknownMask_valid)),
  0x42,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, navicHealthUnknownMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gpsHealthGoodMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gpsHealthGoodMask_valid)),
  0x43,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gpsHealthGoodMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gloHealthGoodMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gloHealthGoodMask_valid)),
  0x44,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gloHealthGoodMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bdsHealthGoodMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bdsHealthGoodMask_valid)),
  0x45,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, bdsHealthGoodMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, galHealthGoodMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, galHealthGoodMask_valid)),
  0x46,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, galHealthGoodMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, qzssHealthGoodMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, qzssHealthGoodMask_valid)),
  0x47,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, qzssHealthGoodMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navicHealthGoodMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navicHealthGoodMask_valid)),
  0x48,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, navicHealthGoodMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gpsHealthBadMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gpsHealthBadMask_valid)),
  0x49,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gpsHealthBadMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gloHealthBadMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gloHealthBadMask_valid)),
  0x4A,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gloHealthBadMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bdsHealthBadMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, bdsHealthBadMask_valid)),
  0x4B,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, bdsHealthBadMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, galHealthBadMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, galHealthBadMask_valid)),
  0x4C,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, galHealthBadMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, qzssHealthBadMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, qzssHealthBadMask_valid)),
  0x4D,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, qzssHealthBadMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navicHealthBadMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navicHealthBadMask_valid)),
  0x4E,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, navicHealthBadMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, fixInfoTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, fixInfoTime_valid)),
  0x4F,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, fixInfoTime),
  QMI_IDL_TYPE88(0, 101),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, fixInfoMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, fixInfoMask_valid)),
  0x50,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, fixInfoMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navDataTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navDataTime_valid)),
  0x51,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, navDataTime),
  QMI_IDL_TYPE88(0, 101),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navData) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navData_valid)),
  0x52,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, navData),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navData) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, navData_len),
  QMI_IDL_TYPE88(0, 100),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, fixStatusTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, fixStatusTime_valid)),
  0x53,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, fixStatusTime),
  QMI_IDL_TYPE88(0, 101),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, fixStatusMask) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, fixStatusMask_valid)),
  0x54,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, fixStatusMask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, fixHepeLimit) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, fixHepeLimit_valid)),
  0x55,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, fixHepeLimit),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gnssTime) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gnssTime_valid)),
  0x56,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gnssTime),
  QMI_IDL_TYPE88(0, 62),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gnssLeapSecond) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gnssLeapSecond_valid)),
  0x57,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gnssLeapSecond),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gnssLeapSecondUnc) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, gnssLeapSecondUnc_valid)),
  0x58,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, gnssLeapSecondUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, agcStatusL1) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, agcStatusL1_valid)),
  0x59,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, agcStatusL1),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, agcStatusL2) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, agcStatusL2_valid)),
  0x5A,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, agcStatusL2),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, agcStatusL5) - QMI_IDL_OFFSET16RELATIVE(qmiLocEngineDebugDataIndMsgT_v02, agcStatusL5_valid)),
  0x5B,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEngineDebugDataIndMsgT_v02, agcStatusL5)
};

static const uint8_t qmiLocSetTribandStateReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetTribandStateReqMsgT_v02, activate) - QMI_IDL_OFFSET8(qmiLocSetTribandStateReqMsgT_v02, activate_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetTribandStateReqMsgT_v02, activate)
};

static const uint8_t qmiLocGetTribandStateIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetTribandStateIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGetTribandStateIndMsgT_v02, activate) - QMI_IDL_OFFSET8(qmiLocGetTribandStateIndMsgT_v02, activate_valid)),
  0x10,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetTribandStateIndMsgT_v02, activate)
};

static const uint8_t qmiLocSetSdkFeatureConfigReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, featureConfig) - QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, featureConfig_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, featureConfig),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, appHash) - QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, appHash_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, appHash),
  QMI_LOC_MAX_APP_HASH_LEN_V02,
  QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, appHash) - QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, appHash_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, featureStatusReport) - QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, featureStatusReport_valid)),
  0x12,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, featureStatusReport),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, licenseBuffer) - QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, licenseBuffer_valid)),
  0x13,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, licenseBuffer),
  ((12288) & 0xFF), ((12288) >> 8),
  QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, licenseBuffer) - QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigReqMsgT_v02, licenseBuffer_len)
};

static const uint8_t qmiLocSetSdkFeatureConfigIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigIndMsgT_v02, featureStatusReport) - QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigIndMsgT_v02, featureStatusReport_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetSdkFeatureConfigIndMsgT_v02, featureStatusReport)
};

static const uint8_t qmiLocGnssBandsSupportedIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssBandsSupportedIndMsgT_v02, primaryGnssSignalType) - QMI_IDL_OFFSET8(qmiLocGnssBandsSupportedIndMsgT_v02, primaryGnssSignalType_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssBandsSupportedIndMsgT_v02, primaryGnssSignalType),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocGnssBandsSupportedIndMsgT_v02, gnssSupportedSignals) - QMI_IDL_OFFSET8(qmiLocGnssBandsSupportedIndMsgT_v02, gnssSupportedSignals_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGnssBandsSupportedIndMsgT_v02, gnssSupportedSignals)
};

static const uint8_t qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, publicKeyType) - QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, publicKeyType_valid)),
  0x10,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, publicKeyType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, publicKeyId) - QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, publicKeyId_valid)),
  0x11,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, publicKeyId),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, publicKey) - QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, publicKey_valid)),
  0x12,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, publicKey),
  QMI_LOC_MERKLE_TREE_KEY_LENGTH_V02,
  QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, publicKey) - QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, publicKey_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, hashFunctionType) - QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, hashFunctionType_valid)),
  0x13,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, hashFunctionType),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, intermediateNodes) - QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, intermediateNodes_valid)),
  0x14,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, intermediateNodes),
  QMI_LOC_MERKLE_TREE_NODE_ARRAY_LENGTH_V02,
  QMI_IDL_TYPE88(0, 102),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, rootNode) - QMI_IDL_OFFSET16RELATIVE(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, rootNode_valid)),
  0x15,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02, rootNode),
  QMI_IDL_TYPE88(0, 102)
};

static const uint8_t qmiLocSetOsnmaStateReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetOsnmaStateReqMsgT_v02, enable)
};

static const uint8_t qmiLocSetNtnStatusReqMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetNtnStatusReqMsgT_v02, signalType)
};

static const uint8_t qmiLocSetNtnStatusIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetNtnStatusIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocSetNtnStatusIndMsgT_v02, signalType)
};

static const uint8_t qmiLocGetNtnStatusIndMsgT_data_v02[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetNtnStatusIndMsgT_v02, status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocGetNtnStatusIndMsgT_v02, signalType)
};

static const uint8_t qmiLocNtnConfigUpdateIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocNtnConfigUpdateIndMsgT_v02, signalType)
};

static const uint8_t qmiLocEventDbhPositionIndMsgT_data_v02[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, latitude) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, latitude_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, latitude),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, longitude) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, longitude_valid)),
  0x11,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, longitude),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, horUncCircular) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, horUncCircular_valid)),
  0x12,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, horUncCircular),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, horConfidence) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, horConfidence_valid)),
  0x13,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, horConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, horReliability) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, horReliability_valid)),
  0x14,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, horReliability),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, altitudeWrtEllipsoid) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, altitudeWrtEllipsoid_valid)),
  0x15,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, altitudeWrtEllipsoid),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, altitudeWrtMeanSeaLevel) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, altitudeWrtMeanSeaLevel_valid)),
  0x16,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, altitudeWrtMeanSeaLevel),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, vertUnc) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, vertUnc_valid)),
  0x17,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, vertUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, vertConfidence) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, vertConfidence_valid)),
  0x18,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, vertConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, vertReliability) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, vertReliability_valid)),
  0x19,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, vertReliability),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, altSourceInfo) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, altSourceInfo_valid)),
  0x1A,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, altSourceInfo),
  QMI_IDL_TYPE88(0, 32),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, timestampUtc) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, timestampUtc_valid)),
  0x1B,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, timestampUtc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, timestampAge) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, timestampAge_valid)),
  0x1C,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, timestampAge),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, positionSrc) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, positionSrc_valid)),
  0x1D,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, positionSrc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, rawHorUncCircular) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, rawHorUncCircular_valid)),
  0x1E,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, rawHorUncCircular),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, rawHorConfidence) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, rawHorConfidence_valid)),
  0x1F,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, rawHorConfidence),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, onDemandCpi) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, onDemandCpi_valid)),
  0x20,
   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, onDemandCpi),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, positionSrcProvider) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, positionSrcProvider_valid)),
  0x21,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, positionSrcProvider),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, gpsTime) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, gpsTime_valid)),
  0x22,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, gpsTime),
  QMI_IDL_TYPE88(0, 2),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, timeUnc) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, timeUnc_valid)),
  0x23,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, timeUnc),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, velEnu) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, velEnu_valid)),
  0x24,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, velEnu),
  QMI_LOC_ENU_ARRAY_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, velUncEnu) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, velUncEnu_valid)),
  0x25,
  QMI_IDL_FLAGS_IS_ARRAY |  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, velUncEnu),
  QMI_LOC_ENU_ARRAY_LENGTH_V02,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, expandedGnssSvUsedList) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, expandedGnssSvUsedList_valid)),
  0x26,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, expandedGnssSvUsedList),
  QMI_LOC_EXPANDED_SV_INFO_LIST_MAX_SIZE_V02,
  QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, expandedGnssSvUsedList) - QMI_IDL_OFFSET8(qmiLocEventDbhPositionIndMsgT_v02, expandedGnssSvUsedList_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(qmiLocEventDbhPositionIndMsgT_v02, numSvInFix) - QMI_IDL_OFFSET16RELATIVE(qmiLocEventDbhPositionIndMsgT_v02, numSvInFix_valid)),
  0x27,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(qmiLocEventDbhPositionIndMsgT_v02, numSvInFix)
};

/* Type Table */
static const qmi_idl_type_table_entry  loc_type_table_v02[] = {
  {sizeof(qmiLocApplicationIdStructT_v02), qmiLocApplicationIdStructT_data_v02},
  {sizeof(qmiLocPowerModeStructT_v02), qmiLocPowerModeStructT_data_v02},
  {sizeof(qmiLocGPSTimeStructT_v02), qmiLocGPSTimeStructT_data_v02},
  {sizeof(qmiLocDOPStructT_v02), qmiLocDOPStructT_data_v02},
  {sizeof(qmiLocExtDOPStructT_v02), qmiLocExtDOPStructT_data_v02},
  {sizeof(qmiLocSensorUsageIndicatorStructT_v02), qmiLocSensorUsageIndicatorStructT_data_v02},
  {sizeof(qmiLocJammerIndicatorStructT_v02), qmiLocJammerIndicatorStructT_data_v02},
  {sizeof(qmiLocJammerIndicatorExtStructT_v02), qmiLocJammerIndicatorExtStructT_data_v02},
  {sizeof(qmiLocSvInfoStructT_v02), qmiLocSvInfoStructT_data_v02},
  {sizeof(qmiLocExpandedSvInfoStructT_v02), qmiLocExpandedSvInfoStructT_data_v02},
  {sizeof(qmiLocNiVxNotifyVerifyStructT_v02), qmiLocNiVxNotifyVerifyStructT_data_v02},
  {sizeof(qmiLocNiSuplFormattedStringStructT_v02), qmiLocNiSuplFormattedStringStructT_data_v02},
  {sizeof(qmiLocNiSuplQopStructT_v02), qmiLocNiSuplQopStructT_data_v02},
  {sizeof(qmiLocIpV4AddrStructType_v02), qmiLocIpV4AddrStructType_data_v02},
  {sizeof(qmiLocIpV6AddrStructType_v02), qmiLocIpV6AddrStructType_data_v02},
  {sizeof(qmiLocNiSuplServerInfoStructT_v02), qmiLocNiSuplServerInfoStructT_data_v02},
  {sizeof(qmiLocNiSuplNotifyVerifyStructT_v02), qmiLocNiSuplNotifyVerifyStructT_data_v02},
  {sizeof(qmiLocNiUmtsCpCodedStringStructT_v02), qmiLocNiUmtsCpCodedStringStructT_data_v02},
  {sizeof(qmiLocNiUmtsCpNotifyVerifyStructT_v02), qmiLocNiUmtsCpNotifyVerifyStructT_data_v02},
  {sizeof(qmiLocNiVxServiceInteractionStructT_v02), qmiLocNiVxServiceInteractionStructT_data_v02},
  {sizeof(qmiLocNiSuplVer2ExtStructT_v02), qmiLocNiSuplVer2ExtStructT_data_v02},
  {sizeof(qmiLocEmergencyNotificationStructT_v02), qmiLocEmergencyNotificationStructT_data_v02},
  {sizeof(qmiLocAssistanceServerUrlStructT_v02), qmiLocAssistanceServerUrlStructT_data_v02},
  {sizeof(qmiLocTimeServerListStructT_v02), qmiLocTimeServerListStructT_data_v02},
  {sizeof(qmiLocPredictedOrbitsAllowedSizesStructT_v02), qmiLocPredictedOrbitsAllowedSizesStructT_data_v02},
  {sizeof(qmiLocPredictedOrbitsServerListStructT_v02), qmiLocPredictedOrbitsServerListStructT_data_v02},
  {sizeof(qmiLocPredictedOrbitsSpecialFileTypeStructT_v02), qmiLocPredictedOrbitsSpecialFileTypeStructT_data_v02},
  {sizeof(qmiLocSensorControlConfigSamplingSpecStructT_v02), qmiLocSensorControlConfigSamplingSpecStructT_data_v02},
  {sizeof(qmiLocSensorReadyStatusStructT_v02), qmiLocSensorReadyStatusStructT_data_v02},
  {sizeof(qmiLocGeofencePositionStructT_v02), qmiLocGeofencePositionStructT_data_v02},
  {sizeof(qmiLocGeofenceIdContinuousStructT_v02), qmiLocGeofenceIdContinuousStructT_data_v02},
  {sizeof(qmiLocPredictedOrbitsDataValidityStructT_v02), qmiLocPredictedOrbitsDataValidityStructT_data_v02},
  {sizeof(qmiLocAltitudeSrcInfoStructT_v02), qmiLocAltitudeSrcInfoStructT_data_v02},
  {sizeof(qmiLocDeleteSvInfoStructT_v02), qmiLocDeleteSvInfoStructT_data_v02},
  {sizeof(qmiLocDeleteBDSSvInfoStructT_v02), qmiLocDeleteBDSSvInfoStructT_data_v02},
  {sizeof(qmiLocDeleteGALSvInfoStructT_v02), qmiLocDeleteGALSvInfoStructT_data_v02},
  {sizeof(qmiLocWifiFixTimeStructT_v02), qmiLocWifiFixTimeStructT_data_v02},
  {sizeof(qmiLocWifiFixPosStructT_v02), qmiLocWifiFixPosStructT_data_v02},
  {sizeof(qmiLocWifiApInfoStructT_v02), qmiLocWifiApInfoStructT_data_v02},
  {sizeof(qmiLocWifiApSsidStructT_v02), qmiLocWifiApSsidStructT_data_v02},
  {sizeof(qmiLoc3AxisSensorSampleStructT_v02), qmiLoc3AxisSensorSampleStructT_data_v02},
  {sizeof(qmiLoc3AxisSensorSampleListStructT_v02), qmiLoc3AxisSensorSampleListStructT_data_v02},
  {sizeof(qmiLocSensorTemperatureSampleStructT_v02), qmiLocSensorTemperatureSampleStructT_data_v02},
  {sizeof(qmiLocSensorTemperatureSampleListStructT_v02), qmiLocSensorTemperatureSampleListStructT_data_v02},
  {sizeof(qmiLocApnProfilesStructT_v02), qmiLocApnProfilesStructT_data_v02},
  {sizeof(qmiLocCircularGeofenceArgsStructT_v02), qmiLocCircularGeofenceArgsStructT_data_v02},
  {sizeof(qmiLocGeofenceMotionStateConfigStructT_v02), qmiLocGeofenceMotionStateConfigStructT_data_v02},
  {sizeof(qmiLocTimeZoneStructT_v02), qmiLocTimeZoneStructT_data_v02},
  {sizeof(qmiLocMotionDataStructT_v02), qmiLocMotionDataStructT_data_v02},
  {sizeof(qmiLocGSMCellIdStructT_v02), qmiLocGSMCellIdStructT_data_v02},
  {sizeof(qmiLocWCDMACellIdStructT_v02), qmiLocWCDMACellIdStructT_data_v02},
  {sizeof(qmiLocTDSCDMACellIdStructT_v02), qmiLocTDSCDMACellIdStructT_data_v02},
  {sizeof(qmiLocBatchedReportStructT_v02), qmiLocBatchedReportStructT_data_v02},
  {sizeof(qmiLocWifiApDataStructT_v02), qmiLocWifiApDataStructT_data_v02},
  {sizeof(qmiLocWifiApAdditionalDataStructT_v02), qmiLocWifiApAdditionalDataStructT_data_v02},
  {sizeof(qmiLocVehicleSensorSampleStructT_v02), qmiLocVehicleSensorSampleStructT_data_v02},
  {sizeof(qmiLocVehicleSensorSampleListStructType_v02), qmiLocVehicleSensorSampleListStructType_data_v02},
  {sizeof(qmiLocVehicleOdometrySampleStructT_v02), qmiLocVehicleOdometrySampleStructT_data_v02},
  {sizeof(qmiLocVehicleOdometrySampleListStructT_v02), qmiLocVehicleOdometrySampleListStructT_data_v02},
  {sizeof(qmiLocRcvrClockFrequencyInfoStructT_v02), qmiLocRcvrClockFrequencyInfoStructT_data_v02},
  {sizeof(qmiLocLeapSecondInfoStructT_v02), qmiLocLeapSecondInfoStructT_data_v02},
  {sizeof(qmiLocInterSystemBiasStructT_v02), qmiLocInterSystemBiasStructT_data_v02},
  {sizeof(qmiLocGnssTimeStructT_v02), qmiLocGnssTimeStructT_data_v02},
  {sizeof(qmiLocGloTimeStructT_v02), qmiLocGloTimeStructT_data_v02},
  {sizeof(qmiLocGnssTimeExtStructT_v02), qmiLocGnssTimeExtStructT_data_v02},
  {sizeof(qmiLocSVTimeSpeedStructT_v02), qmiLocSVTimeSpeedStructT_data_v02},
  {sizeof(qmiLocSVMeasurementStructT_v02), qmiLocSVMeasurementStructT_data_v02},
  {sizeof(qmiLocDgnssSVMeasurementStructT_v02), qmiLocDgnssSVMeasurementStructT_data_v02},
  {sizeof(qmiLocWifiApMacAddressStructT_v02), qmiLocWifiApMacAddressStructT_data_v02},
  {sizeof(qmiLocIBeaconIdStructT_v02), qmiLocIBeaconIdStructT_data_v02},
  {sizeof(qmiLocGtpAsnVerStructT_v02), qmiLocGtpAsnVerStructT_data_v02},
  {sizeof(qmiLocDbtPositionStructT_v02), qmiLocDbtPositionStructT_data_v02},
  {sizeof(qmiLocApCacheStructT_v02), qmiLocApCacheStructT_data_v02},
  {sizeof(qmiLocApCacheHepeRelStructT_v02), qmiLocApCacheHepeRelStructT_data_v02},
  {sizeof(qmiLocApDoNotCacheStructT_v02), qmiLocApDoNotCacheStructT_data_v02},
  {sizeof(qmiLocDeleteSatelliteDataStructT_v02), qmiLocDeleteSatelliteDataStructT_data_v02},
  {sizeof(qmiLocInternalStatusReportDataStructT_v02), qmiLocInternalStatusReportDataStructT_data_v02},
  {sizeof(qmiLocSrnBtleApDeviceDataStructT_v02), qmiLocSrnBtleApDeviceDataStructT_data_v02},
  {sizeof(qmiLocWifiCrowdSourcingLocalConfigStructT_v02), qmiLocWifiCrowdSourcingLocalConfigStructT_data_v02},
  {sizeof(qmiLocWifiCrowdSourcingCapabilityStructT_v02), qmiLocWifiCrowdSourcingCapabilityStructT_data_v02},
  {sizeof(qmiLocXtraInfoStructT_v02), qmiLocXtraInfoStructT_data_v02},
  {sizeof(qmiLocVersionStructT_v02), qmiLocVersionStructT_data_v02},
  {sizeof(qmiLocGtpClientSwVerStructT_v02), qmiLocGtpClientSwVerStructT_data_v02},
  {sizeof(qmiLocHomeCarrierIdStructT_v02), qmiLocHomeCarrierIdStructT_data_v02},
  {sizeof(qmiLocPositionSourceStructT_v02), qmiLocPositionSourceStructT_data_v02},
  {sizeof(qmiLocBsBestPositionStructT_v02), qmiLocBsBestPositionStructT_data_v02},
  {sizeof(qmiLocFdclCellIdStructT_v02), qmiLocFdclCellIdStructT_data_v02},
  {sizeof(qmiLocFdclCellPosStructT_v02), qmiLocFdclCellPosStructT_data_v02},
  {sizeof(qmiLocNextLeapSecondInfoStructT_v02), qmiLocNextLeapSecondInfoStructT_data_v02},
  {sizeof(qmiLocBsObservedDataStructT_v02), qmiLocBsObservedDataStructT_data_v02},
  {sizeof(qmiLocEphGnssDataStructT_v02), qmiLocEphGnssDataStructT_data_v02},
  {sizeof(qmiLocGpsEphemerisT_v02), qmiLocGpsEphemerisT_data_v02},
  {sizeof(qmiLocGloEphemerisT_v02), qmiLocGloEphemerisT_data_v02},
  {sizeof(qmiLocBdsEphemerisT_v02), qmiLocBdsEphemerisT_data_v02},
  {sizeof(qmiLocGalEphemerisT_v02), qmiLocGalEphemerisT_data_v02},
  {sizeof(qmiLocKlobucharIonoModelT_v02), qmiLocKlobucharIonoModelT_data_v02},
  {sizeof(qmiLocMePerGnssRfStructType_v02), qmiLocMePerGnssRfStructType_data_v02},
  {sizeof(qmiLocRobustLocationVersionT_v02), qmiLocRobustLocationVersionT_data_v02},
  {sizeof(qmiLocEnvAidingReflectingPlaneStructT_v02), qmiLocEnvAidingReflectingPlaneStructT_data_v02},
  {sizeof(qmiLocEnvAidingSVCorrectionStructT_v02), qmiLocEnvAidingSVCorrectionStructT_data_v02},
  {sizeof(qmiLocNavDataStructT_v02), qmiLocNavDataStructT_data_v02},
  {sizeof(qmiLocTimeStructT_v02), qmiLocTimeStructT_data_v02},
  {sizeof(qmiLocOsnmaTreeNodeT_v02), qmiLocOsnmaTreeNodeT_data_v02},
  {sizeof(qmiLocMlInferSVMeasurementStructT_v02), qmiLocMlInferSVMeasurementStructT_data_v02}
};

/* Message Table */
static const qmi_idl_message_table_entry loc_message_table_v02[] = {
  {sizeof(qmiLocGenReqMsgT_v02), 0},
  {sizeof(qmiLocGenRespMsgT_v02), qmiLocGenRespMsgT_data_v02},
  {sizeof(qmiLocGenReqStatusIndMsgT_v02), qmiLocGenReqStatusIndMsgT_data_v02},
  {sizeof(qmiLocInformClientRevisionReqMsgT_v02), qmiLocInformClientRevisionReqMsgT_data_v02},
  {sizeof(qmiLocRegEventsReqMsgT_v02), qmiLocRegEventsReqMsgT_data_v02},
  {sizeof(qmiLocStartReqMsgT_v02), qmiLocStartReqMsgT_data_v02},
  {sizeof(qmiLocStopReqMsgT_v02), qmiLocStopReqMsgT_data_v02},
  {sizeof(qmiLocEventPositionReportIndMsgT_v02), qmiLocEventPositionReportIndMsgT_data_v02},
  {sizeof(qmiLocEventGnssSvInfoIndMsgT_v02), qmiLocEventGnssSvInfoIndMsgT_data_v02},
  {sizeof(qmiLocEventNmeaIndMsgT_v02), qmiLocEventNmeaIndMsgT_data_v02},
  {sizeof(qmiLocEventNiNotifyVerifyReqIndMsgT_v02), qmiLocEventNiNotifyVerifyReqIndMsgT_data_v02},
  {sizeof(qmiLocEventInjectTimeReqIndMsgT_v02), qmiLocEventInjectTimeReqIndMsgT_data_v02},
  {sizeof(qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02), qmiLocEventInjectPredictedOrbitsReqIndMsgT_data_v02},
  {sizeof(qmiLocEventInjectPositionReqIndMsgT_v02), qmiLocEventInjectPositionReqIndMsgT_data_v02},
  {sizeof(qmiLocEventEngineStateIndMsgT_v02), qmiLocEventEngineStateIndMsgT_data_v02},
  {sizeof(qmiLocEventFixSessionStateIndMsgT_v02), qmiLocEventFixSessionStateIndMsgT_data_v02},
  {sizeof(qmiLocEventWifiReqIndMsgT_v02), qmiLocEventWifiReqIndMsgT_data_v02},
  {sizeof(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02), qmiLocEventSensorStreamingReadyStatusIndMsgT_data_v02},
  {sizeof(qmiLocEventTimeSyncReqIndMsgT_v02), qmiLocEventTimeSyncReqIndMsgT_data_v02},
  {sizeof(qmiLocEventSetSpiStreamingReportIndMsgT_v02), qmiLocEventSetSpiStreamingReportIndMsgT_data_v02},
  {sizeof(qmiLocEventLocationServerConnectionReqIndMsgT_v02), qmiLocEventLocationServerConnectionReqIndMsgT_data_v02},
  {sizeof(qmiLocEventNiGeofenceNotificationIndMsgT_v02), qmiLocEventNiGeofenceNotificationIndMsgT_data_v02},
  {sizeof(qmiLocEventGeofenceGenAlertIndMsgT_v02), qmiLocEventGeofenceGenAlertIndMsgT_data_v02},
  {sizeof(qmiLocEventGeofenceBreachIndMsgT_v02), qmiLocEventGeofenceBreachIndMsgT_data_v02},
  {sizeof(qmiLocEventPedometerControlIndMsgT_v02), qmiLocEventPedometerControlIndMsgT_data_v02},
  {sizeof(qmiLocEventMotionDataControlIndMsgT_v02), qmiLocEventMotionDataControlIndMsgT_data_v02},
  {sizeof(qmiLocEventGeofenceBatchedBreachIndMsgT_v02), qmiLocEventGeofenceBatchedBreachIndMsgT_data_v02},
  {sizeof(qmiLocEventGeofenceProximityIndMsgT_v02), qmiLocEventGeofenceProximityIndMsgT_data_v02},
  {sizeof(qmiLocEventGeofenceBatchedDwellIndMsgT_v02), qmiLocEventGeofenceBatchedDwellIndMsgT_data_v02},
  {sizeof(qmiLocEventGdtUploadBeginStatusReqIndMsgT_v02), qmiLocEventGdtUploadBeginStatusReqIndMsgT_data_v02},
  {sizeof(qmiLocEventGdtUploadEndReqIndMsgT_v02), qmiLocEventGdtUploadEndReqIndMsgT_data_v02},
  {sizeof(qmiLocEventGdtDownloadBeginReqIndMsgT_v02), qmiLocEventGdtDownloadBeginReqIndMsgT_data_v02},
  {sizeof(qmiLocEventGdtReceiveDoneIndMsgT_v02), qmiLocEventGdtReceiveDoneIndMsgT_data_v02},
  {sizeof(qmiLocEventGdtDownloadEndReqIndMsgT_v02), qmiLocEventGdtDownloadEndReqIndMsgT_data_v02},
  {sizeof(qmiLocGetServiceRevisionReqMsgT_v02), 0},
  {sizeof(qmiLocGetServiceRevisionIndMsgT_v02), qmiLocGetServiceRevisionIndMsgT_data_v02},
  {sizeof(qmiLocGetFixCriteriaReqMsgT_v02), 0},
  {sizeof(qmiLocGetFixCriteriaIndMsgT_v02), qmiLocGetFixCriteriaIndMsgT_data_v02},
  {sizeof(qmiLocNiUserRespReqMsgT_v02), qmiLocNiUserRespReqMsgT_data_v02},
  {sizeof(qmiLocNiUserRespIndMsgT_v02), qmiLocNiUserRespIndMsgT_data_v02},
  {sizeof(qmiLocInjectPredictedOrbitsDataReqMsgT_v02), qmiLocInjectPredictedOrbitsDataReqMsgT_data_v02},
  {sizeof(qmiLocInjectPredictedOrbitsDataIndMsgT_v02), qmiLocInjectPredictedOrbitsDataIndMsgT_data_v02},
  {sizeof(qmiLocGetPredictedOrbitsDataSourceReqMsgT_v02), 0},
  {sizeof(qmiLocGetPredictedOrbitsDataSourceIndMsgT_v02), qmiLocGetPredictedOrbitsDataSourceIndMsgT_data_v02},
  {sizeof(qmiLocGetPredictedOrbitsDataValidityReqMsgT_v02), 0},
  {sizeof(qmiLocGetPredictedOrbitsDataValidityIndMsgT_v02), qmiLocGetPredictedOrbitsDataValidityIndMsgT_data_v02},
  {sizeof(qmiLocInjectUtcTimeReqMsgT_v02), qmiLocInjectUtcTimeReqMsgT_data_v02},
  {sizeof(qmiLocInjectUtcTimeIndMsgT_v02), qmiLocInjectUtcTimeIndMsgT_data_v02},
  {sizeof(qmiLocInjectPositionReqMsgT_v02), qmiLocInjectPositionReqMsgT_data_v02},
  {sizeof(qmiLocInjectPositionIndMsgT_v02), qmiLocInjectPositionIndMsgT_data_v02},
  {sizeof(qmiLocSetEngineLockReqMsgT_v02), qmiLocSetEngineLockReqMsgT_data_v02},
  {sizeof(qmiLocSetEngineLockIndMsgT_v02), qmiLocSetEngineLockIndMsgT_data_v02},
  {sizeof(qmiLocGetEngineLockReqMsgT_v02), qmiLocGetEngineLockReqMsgT_data_v02},
  {sizeof(qmiLocGetEngineLockIndMsgT_v02), qmiLocGetEngineLockIndMsgT_data_v02},
  {sizeof(qmiLocSetSbasConfigReqMsgT_v02), qmiLocSetSbasConfigReqMsgT_data_v02},
  {sizeof(qmiLocSetSbasConfigIndMsgT_v02), qmiLocSetSbasConfigIndMsgT_data_v02},
  {sizeof(qmiLocGetSbasConfigReqMsgT_v02), 0},
  {sizeof(qmiLocGetSbasConfigIndMsgT_v02), qmiLocGetSbasConfigIndMsgT_data_v02},
  {sizeof(qmiLocSetNmeaTypesReqMsgT_v02), qmiLocSetNmeaTypesReqMsgT_data_v02},
  {sizeof(qmiLocSetNmeaTypesIndMsgT_v02), qmiLocSetNmeaTypesIndMsgT_data_v02},
  {sizeof(qmiLocGetNmeaTypesReqMsgT_v02), 0},
  {sizeof(qmiLocGetNmeaTypesIndMsgT_v02), qmiLocGetNmeaTypesIndMsgT_data_v02},
  {sizeof(qmiLocSetLowPowerModeReqMsgT_v02), qmiLocSetLowPowerModeReqMsgT_data_v02},
  {sizeof(qmiLocSetLowPowerModeIndMsgT_v02), qmiLocSetLowPowerModeIndMsgT_data_v02},
  {sizeof(qmiLocGetLowPowerModeReqMsgT_v02), 0},
  {sizeof(qmiLocGetLowPowerModeIndMsgT_v02), qmiLocGetLowPowerModeIndMsgT_data_v02},
  {sizeof(qmiLocSetServerReqMsgT_v02), qmiLocSetServerReqMsgT_data_v02},
  {sizeof(qmiLocSetServerIndMsgT_v02), qmiLocSetServerIndMsgT_data_v02},
  {sizeof(qmiLocGetServerReqMsgT_v02), qmiLocGetServerReqMsgT_data_v02},
  {sizeof(qmiLocGetServerIndMsgT_v02), qmiLocGetServerIndMsgT_data_v02},
  {sizeof(qmiLocDeleteAssistDataReqMsgT_v02), qmiLocDeleteAssistDataReqMsgT_data_v02},
  {sizeof(qmiLocDeleteAssistDataIndMsgT_v02), qmiLocDeleteAssistDataIndMsgT_data_v02},
  {sizeof(qmiLocSetXtraTSessionControlReqMsgT_v02), qmiLocSetXtraTSessionControlReqMsgT_data_v02},
  {sizeof(qmiLocSetXtraTSessionControlIndMsgT_v02), qmiLocSetXtraTSessionControlIndMsgT_data_v02},
  {sizeof(qmiLocGetXtraTSessionControlReqMsgT_v02), 0},
  {sizeof(qmiLocGetXtraTSessionControlIndMsgT_v02), qmiLocGetXtraTSessionControlIndMsgT_data_v02},
  {sizeof(qmiLocInjectWifiPositionReqMsgT_v02), qmiLocInjectWifiPositionReqMsgT_data_v02},
  {sizeof(qmiLocInjectWifiPositionIndMsgT_v02), qmiLocInjectWifiPositionIndMsgT_data_v02},
  {sizeof(qmiLocNotifyWifiStatusReqMsgT_v02), qmiLocNotifyWifiStatusReqMsgT_data_v02},
  {sizeof(qmiLocNotifyWifiStatusIndMsgT_v02), qmiLocNotifyWifiStatusIndMsgT_data_v02},
  {sizeof(qmiLocGetRegisteredEventsReqMsgT_v02), 0},
  {sizeof(qmiLocGetRegisteredEventsIndMsgT_v02), qmiLocGetRegisteredEventsIndMsgT_data_v02},
  {sizeof(qmiLocSetOperationModeReqMsgT_v02), qmiLocSetOperationModeReqMsgT_data_v02},
  {sizeof(qmiLocSetOperationModeIndMsgT_v02), qmiLocSetOperationModeIndMsgT_data_v02},
  {sizeof(qmiLocGetOperationModeReqMsgT_v02), 0},
  {sizeof(qmiLocGetOperationModeIndMsgT_v02), qmiLocGetOperationModeIndMsgT_data_v02},
  {sizeof(qmiLocSetSpiStatusReqMsgT_v02), qmiLocSetSpiStatusReqMsgT_data_v02},
  {sizeof(qmiLocSetSpiStatusIndMsgT_v02), qmiLocSetSpiStatusIndMsgT_data_v02},
  {sizeof(qmiLocInjectSensorDataReqMsgT_v02), qmiLocInjectSensorDataReqMsgT_data_v02},
  {sizeof(qmiLocInjectSensorDataIndMsgT_v02), qmiLocInjectSensorDataIndMsgT_data_v02},
  {sizeof(qmiLocInjectTimeSyncDataReqMsgT_v02), qmiLocInjectTimeSyncDataReqMsgT_data_v02},
  {sizeof(qmiLocInjectTimeSyncDataIndMsgT_v02), qmiLocInjectTimeSyncDataIndMsgT_data_v02},
  {sizeof(qmiLocGetCradleMountConfigReqMsgT_v02), 0},
  {sizeof(qmiLocGetCradleMountConfigIndMsgT_v02), qmiLocGetCradleMountConfigIndMsgT_data_v02},
  {sizeof(qmiLocSetCradleMountConfigReqMsgT_v02), qmiLocSetCradleMountConfigReqMsgT_data_v02},
  {sizeof(qmiLocSetCradleMountConfigIndMsgT_v02), qmiLocSetCradleMountConfigIndMsgT_data_v02},
  {sizeof(qmiLocGetExternalPowerConfigReqMsgT_v02), 0},
  {sizeof(qmiLocGetExternalPowerConfigIndMsgT_v02), qmiLocGetExternalPowerConfigIndMsgT_data_v02},
  {sizeof(qmiLocSetExternalPowerConfigReqMsgT_v02), qmiLocSetExternalPowerConfigReqMsgT_data_v02},
  {sizeof(qmiLocSetExternalPowerConfigIndMsgT_v02), qmiLocSetExternalPowerConfigIndMsgT_data_v02},
  {sizeof(qmiLocInformLocationServerConnStatusReqMsgT_v02), qmiLocInformLocationServerConnStatusReqMsgT_data_v02},
  {sizeof(qmiLocInformLocationServerConnStatusIndMsgT_v02), qmiLocInformLocationServerConnStatusIndMsgT_data_v02},
  {sizeof(qmiLocSetProtocolConfigParametersReqMsgT_v02), qmiLocSetProtocolConfigParametersReqMsgT_data_v02},
  {sizeof(qmiLocSetProtocolConfigParametersIndMsgT_v02), qmiLocSetProtocolConfigParametersIndMsgT_data_v02},
  {sizeof(qmiLocGetProtocolConfigParametersReqMsgT_v02), qmiLocGetProtocolConfigParametersReqMsgT_data_v02},
  {sizeof(qmiLocGetProtocolConfigParametersIndMsgT_v02), qmiLocGetProtocolConfigParametersIndMsgT_data_v02},
  {sizeof(qmiLocSetSensorControlConfigReqMsgT_v02), qmiLocSetSensorControlConfigReqMsgT_data_v02},
  {sizeof(qmiLocSetSensorControlConfigIndMsgT_v02), qmiLocSetSensorControlConfigIndMsgT_data_v02},
  {sizeof(qmiLocGetSensorControlConfigReqMsgT_v02), 0},
  {sizeof(qmiLocGetSensorControlConfigIndMsgT_v02), qmiLocGetSensorControlConfigIndMsgT_data_v02},
  {sizeof(qmiLocSetSensorPropertiesReqMsgT_v02), qmiLocSetSensorPropertiesReqMsgT_data_v02},
  {sizeof(qmiLocSetSensorPropertiesIndMsgT_v02), qmiLocSetSensorPropertiesIndMsgT_data_v02},
  {sizeof(qmiLocGetSensorPropertiesReqMsgT_v02), qmiLocGetSensorPropertiesReqMsgT_data_v02},
  {sizeof(qmiLocGetSensorPropertiesIndMsgT_v02), qmiLocGetSensorPropertiesIndMsgT_data_v02},
  {sizeof(qmiLocSetSensorPerformanceControlConfigReqMsgT_v02), qmiLocSetSensorPerformanceControlConfigReqMsgT_data_v02},
  {sizeof(qmiLocSetSensorPerformanceControlConfigIndMsgT_v02), qmiLocSetSensorPerformanceControlConfigIndMsgT_data_v02},
  {sizeof(qmiLocGetSensorPerformanceControlConfigReqMsgT_v02), 0},
  {sizeof(qmiLocGetSensorPerformanceControlConfigIndMsgT_v02), qmiLocGetSensorPerformanceControlConfigIndMsgT_data_v02},
  {sizeof(qmiLocInjectSuplCertificateReqMsgT_v02), qmiLocInjectSuplCertificateReqMsgT_data_v02},
  {sizeof(qmiLocInjectSuplCertificateIndMsgT_v02), qmiLocInjectSuplCertificateIndMsgT_data_v02},
  {sizeof(qmiLocDeleteSuplCertificateReqMsgT_v02), qmiLocDeleteSuplCertificateReqMsgT_data_v02},
  {sizeof(qmiLocDeleteSuplCertificateIndMsgT_v02), qmiLocDeleteSuplCertificateIndMsgT_data_v02},
  {sizeof(qmiLocSetPositionEngineConfigParametersReqMsgT_v02), qmiLocSetPositionEngineConfigParametersReqMsgT_data_v02},
  {sizeof(qmiLocSetPositionEngineConfigParametersIndMsgT_v02), qmiLocSetPositionEngineConfigParametersIndMsgT_data_v02},
  {sizeof(qmiLocGetPositionEngineConfigParametersReqMsgT_v02), qmiLocGetPositionEngineConfigParametersReqMsgT_data_v02},
  {sizeof(qmiLocGetPositionEngineConfigParametersIndMsgT_v02), qmiLocGetPositionEngineConfigParametersIndMsgT_data_v02},
  {sizeof(qmiLocAddCircularGeofenceReqMsgT_v02), qmiLocAddCircularGeofenceReqMsgT_data_v02},
  {sizeof(qmiLocAddCircularGeofenceIndMsgT_v02), qmiLocAddCircularGeofenceIndMsgT_data_v02},
  {sizeof(qmiLocDeleteGeofenceReqMsgT_v02), qmiLocDeleteGeofenceReqMsgT_data_v02},
  {sizeof(qmiLocDeleteGeofenceIndMsgT_v02), qmiLocDeleteGeofenceIndMsgT_data_v02},
  {sizeof(qmiLocQueryGeofenceReqMsgT_v02), qmiLocQueryGeofenceReqMsgT_data_v02},
  {sizeof(qmiLocQueryGeofenceIndMsgT_v02), qmiLocQueryGeofenceIndMsgT_data_v02},
  {sizeof(qmiLocSetGeofenceEngineConfigReqMsgT_v02), qmiLocSetGeofenceEngineConfigReqMsgT_data_v02},
  {sizeof(qmiLocSetGeofenceEngineConfigIndMsgT_v02), qmiLocSetGeofenceEngineConfigIndMsgT_data_v02},
  {sizeof(qmiLocGetGeofenceEngineConfigReqMsgT_v02), qmiLocGetGeofenceEngineConfigReqMsgT_data_v02},
  {sizeof(qmiLocGetGeofenceEngineConfigIndMsgT_v02), qmiLocGetGeofenceEngineConfigIndMsgT_data_v02},
  {sizeof(qmiLocEditGeofenceReqMsgT_v02), qmiLocEditGeofenceReqMsgT_data_v02},
  {sizeof(qmiLocEditGeofenceIndMsgT_v02), qmiLocEditGeofenceIndMsgT_data_v02},
  {sizeof(qmiLocEventGetTimeZoneReqIndMsgT_v02), qmiLocEventGetTimeZoneReqIndMsgT_data_v02},
  {sizeof(qmiLocInjectTimeZoneInfoReqMsgT_v02), qmiLocInjectTimeZoneInfoReqMsgT_data_v02},
  {sizeof(qmiLocInjectTimeZoneInfoIndMsgT_v02), qmiLocInjectTimeZoneInfoIndMsgT_data_v02},
  {sizeof(qmiLocGetBestAvailablePositionReqMsgT_v02), qmiLocGetBestAvailablePositionReqMsgT_data_v02},
  {sizeof(qmiLocGetBestAvailablePositionIndMsgT_v02), qmiLocGetBestAvailablePositionIndMsgT_data_v02},
  {sizeof(qmiLocInjectMotionDataReqMsgT_v02), qmiLocInjectMotionDataReqMsgT_data_v02},
  {sizeof(qmiLocInjectMotionDataIndMsgT_v02), qmiLocInjectMotionDataIndMsgT_data_v02},
  {sizeof(qmiLocGetNiGeofenceIdListReqMsgT_v02), qmiLocGetNiGeofenceIdListReqMsgT_data_v02},
  {sizeof(qmiLocGetNiGeofenceIdListIndMsgT_v02), qmiLocGetNiGeofenceIdListIndMsgT_data_v02},
  {sizeof(qmiLocInjectGSMCellInfoReqMsgT_v02), qmiLocInjectGSMCellInfoReqMsgT_data_v02},
  {sizeof(qmiLocInjectGSMCellInfoIndMsgT_v02), qmiLocInjectGSMCellInfoIndMsgT_data_v02},
  {sizeof(qmiLocInjectWCDMACellInfoReqMsgT_v02), qmiLocInjectWCDMACellInfoReqMsgT_data_v02},
  {sizeof(qmiLocInjectWCDMACellInfoIndMsgT_v02), qmiLocInjectWCDMACellInfoIndMsgT_data_v02},
  {sizeof(qmiLocInjectTDSCDMACellInfoReqMsgT_v02), qmiLocInjectTDSCDMACellInfoReqMsgT_data_v02},
  {sizeof(qmiLocInjectTDSCDMACellInfoIndMsgT_v02), qmiLocInjectTDSCDMACellInfoIndMsgT_data_v02},
  {sizeof(qmiLocInjectSubscriberIDReqMsgT_v02), qmiLocInjectSubscriberIDReqMsgT_data_v02},
  {sizeof(qmiLocInjectSubscriberIDIndMsgT_v02), qmiLocInjectSubscriberIDIndMsgT_data_v02},
  {sizeof(qmiLocInjectNetworkInitiatedMessageReqMsgT_v02), qmiLocInjectNetworkInitiatedMessageReqMsgT_data_v02},
  {sizeof(qmiLocInjectNetworkInitiatedMessageIndMsgT_v02), qmiLocInjectNetworkInitiatedMessageIndMsgT_data_v02},
  {sizeof(qmiLocWWANOutOfServiceNotificationReqMsgT_v02), 0},
  {sizeof(qmiLocWWANOutOfServiceNotificationIndMsgT_v02), qmiLocWWANOutOfServiceNotificationIndMsgT_data_v02},
  {sizeof(qmiLocPedometerReportReqMsgT_v02), qmiLocPedometerReportReqMsgT_data_v02},
  {sizeof(qmiLocPedometerReportIndMsgT_v02), qmiLocPedometerReportIndMsgT_data_v02},
  {sizeof(qmiLocGetBatchSizeReqMsgT_v02), qmiLocGetBatchSizeReqMsgT_data_v02},
  {sizeof(qmiLocGetBatchSizeIndMsgT_v02), qmiLocGetBatchSizeIndMsgT_data_v02},
  {sizeof(qmiLocStartBatchingReqMsgT_v02), qmiLocStartBatchingReqMsgT_data_v02},
  {sizeof(qmiLocStartBatchingIndMsgT_v02), qmiLocStartBatchingIndMsgT_data_v02},
  {sizeof(qmiLocEventBatchFullIndMsgT_v02), qmiLocEventBatchFullIndMsgT_data_v02},
  {sizeof(qmiLocEventLiveBatchedPositionReportIndMsgT_v02), qmiLocEventLiveBatchedPositionReportIndMsgT_data_v02},
  {sizeof(qmiLocReadFromBatchReqMsgT_v02), qmiLocReadFromBatchReqMsgT_data_v02},
  {sizeof(qmiLocReadFromBatchIndMsgT_v02), qmiLocReadFromBatchIndMsgT_data_v02},
  {sizeof(qmiLocStopBatchingReqMsgT_v02), qmiLocStopBatchingReqMsgT_data_v02},
  {sizeof(qmiLocStopBatchingIndMsgT_v02), qmiLocStopBatchingIndMsgT_data_v02},
  {sizeof(qmiLocReleaseBatchReqMsgT_v02), qmiLocReleaseBatchReqMsgT_data_v02},
  {sizeof(qmiLocReleaseBatchIndMsgT_v02), qmiLocReleaseBatchIndMsgT_data_v02},
  {sizeof(qmiLocEventInjectWifiApDataReqIndMsgT_v02), qmiLocEventInjectWifiApDataReqIndMsgT_data_v02},
  {sizeof(qmiLocInjectWifiApDataReqMsgT_v02), qmiLocInjectWifiApDataReqMsgT_data_v02},
  {sizeof(qmiLocInjectWifiApDataIndMsgT_v02), qmiLocInjectWifiApDataIndMsgT_data_v02},
  {sizeof(qmiLocNotifyWifiAttachmentStatusReqMsgT_v02), qmiLocNotifyWifiAttachmentStatusReqMsgT_data_v02},
  {sizeof(qmiLocNotifyWifiAttachmentStatusIndMsgT_v02), qmiLocNotifyWifiAttachmentStatusIndMsgT_data_v02},
  {sizeof(qmiLocNotifyWifiEnabledStatusReqMsgT_v02), qmiLocNotifyWifiEnabledStatusReqMsgT_data_v02},
  {sizeof(qmiLocNotifyWifiEnabledStatusIndMsgT_v02), qmiLocNotifyWifiEnabledStatusIndMsgT_data_v02},
  {sizeof(qmiLocEventVehicleDataReadyIndMsgT_v02), qmiLocEventVehicleDataReadyIndMsgT_data_v02},
  {sizeof(qmiLocInjectVehicleSensorDataReqMsgT_v02), qmiLocInjectVehicleSensorDataReqMsgT_data_v02},
  {sizeof(qmiLocInjectVehicleSensorDataIndMsgT_v02), qmiLocInjectVehicleSensorDataIndMsgT_data_v02},
  {sizeof(qmiLocGetAvailWwanPositionReqMsgT_v02), qmiLocGetAvailWwanPositionReqMsgT_data_v02},
  {sizeof(qmiLocGetAvailWwanPositionIndMsgT_v02), qmiLocGetAvailWwanPositionIndMsgT_data_v02},
  {sizeof(qmiLocSetPremiumServicesCfgReqMsgT_v02), qmiLocSetPremiumServicesCfgReqMsgT_data_v02},
  {sizeof(qmiLocSetPremiumServicesCfgIndMsgT_v02), qmiLocSetPremiumServicesCfgIndMsgT_data_v02},
  {sizeof(qmiLocSetXtraVersionCheckReqMsgT_v02), qmiLocSetXtraVersionCheckReqMsgT_data_v02},
  {sizeof(qmiLocSetXtraVersionCheckIndMsgT_v02), qmiLocSetXtraVersionCheckIndMsgT_data_v02},
  {sizeof(qmiLocSetGNSSConstRepConfigReqMsgT_v02), qmiLocSetGNSSConstRepConfigReqMsgT_data_v02},
  {sizeof(qmiLocSetGNSSConstRepConfigIndMsgT_v02), qmiLocSetGNSSConstRepConfigIndMsgT_data_v02},
  {sizeof(qmiLocEventGnssSvMeasInfoIndMsgT_v02), qmiLocEventGnssSvMeasInfoIndMsgT_data_v02},
  {sizeof(qmiLocEventGnssSvPolyIndMsgT_v02), qmiLocEventGnssSvPolyIndMsgT_data_v02},
  {sizeof(qmiLocAddGeofenceContextReqMsgT_v02), qmiLocAddGeofenceContextReqMsgT_data_v02},
  {sizeof(qmiLocAddGeofenceContextIndMsgT_v02), qmiLocAddGeofenceContextIndMsgT_data_v02},
  {sizeof(qmiLocSetGeofenceEngineContextReqMsgT_v02), qmiLocSetGeofenceEngineContextReqMsgT_data_v02},
  {sizeof(qmiLocSetGeofenceEngineContextIndMsgT_v02), qmiLocSetGeofenceEngineContextIndMsgT_data_v02},
  {sizeof(qmiLocDeleteGeofenceContextReqMsgT_v02), qmiLocDeleteGeofenceContextReqMsgT_data_v02},
  {sizeof(qmiLocDeleteGeofenceContextIndMsgT_v02), qmiLocDeleteGeofenceContextIndMsgT_data_v02},
  {sizeof(qmiLocInjectGtpClientDownloadedDataReqMsgT_v02), qmiLocInjectGtpClientDownloadedDataReqMsgT_data_v02},
  {sizeof(qmiLocInjectGtpClientDownloadedDataIndMsgT_v02), qmiLocInjectGtpClientDownloadedDataIndMsgT_data_v02},
  {sizeof(qmiLocGdtUploadBeginStatusReqMsgT_v02), qmiLocGdtUploadBeginStatusReqMsgT_data_v02},
  {sizeof(qmiLocGdtUploadBeginStatusIndMsgT_v02), qmiLocGdtUploadBeginStatusIndMsgT_data_v02},
  {sizeof(qmiLocGdtUploadEndReqMsgT_v02), qmiLocGdtUploadEndReqMsgT_data_v02},
  {sizeof(qmiLocGdtUploadEndIndMsgT_v02), qmiLocGdtUploadEndIndMsgT_data_v02},
  {sizeof(qmiLocGtpApStatusReqMsgT_v02), qmiLocGtpApStatusReqMsgT_data_v02},
  {sizeof(qmiLocGtpApStatusIndMsgT_v02), qmiLocGtpApStatusIndMsgT_data_v02},
  {sizeof(qmiLocGdtDownloadBeginStatusReqMsgT_v02), qmiLocGdtDownloadBeginStatusReqMsgT_data_v02},
  {sizeof(qmiLocGdtDownloadBeginStatusIndMsgT_v02), qmiLocGdtDownloadBeginStatusIndMsgT_data_v02},
  {sizeof(qmiLocGdtDownloadReadyStatusReqMsgT_v02), qmiLocGdtDownloadReadyStatusReqMsgT_data_v02},
  {sizeof(qmiLocGdtDownloadReadyStatusIndMsgT_v02), qmiLocGdtDownloadReadyStatusIndMsgT_data_v02},
  {sizeof(qmiLocGdtReceiveDoneStatusReqMsgT_v02), qmiLocGdtReceiveDoneStatusReqMsgT_data_v02},
  {sizeof(qmiLocGdtReceiveDoneStatusIndMsgT_v02), qmiLocGdtReceiveDoneStatusIndMsgT_data_v02},
  {sizeof(qmiLocGdtDownloadEndStatusReqMsgT_v02), qmiLocGdtDownloadEndStatusReqMsgT_data_v02},
  {sizeof(qmiLocGdtDownloadEndStatusIndMsgT_v02), qmiLocGdtDownloadEndStatusIndMsgT_data_v02},
  {sizeof(qmiLocStartDbtReqMsgT_v02), qmiLocStartDbtReqMsgT_data_v02},
  {sizeof(qmiLocStartDbtIndMsgT_v02), qmiLocStartDbtIndMsgT_data_v02},
  {sizeof(qmiLocStopDbtReqMsgT_v02), qmiLocStopDbtReqMsgT_data_v02},
  {sizeof(qmiLocStopDbtIndMsgT_v02), qmiLocStopDbtIndMsgT_data_v02},
  {sizeof(qmiLocEventDbtPositionReportIndMsgT_v02), qmiLocEventDbtPositionReportIndMsgT_data_v02},
  {sizeof(qmiLocEventDbtSessionStatusIndMsgT_v02), qmiLocEventDbtSessionStatusIndMsgT_data_v02},
  {sizeof(qmiLocSecureGetAvailablePositionReqMsgT_v02), qmiLocSecureGetAvailablePositionReqMsgT_data_v02},
  {sizeof(qmiLocSecureGetAvailablePositionIndMsgT_v02), qmiLocSecureGetAvailablePositionIndMsgT_data_v02},
  {sizeof(qmiLocInjectApCacheDataReqMsgT_v02), qmiLocInjectApCacheDataReqMsgT_data_v02},
  {sizeof(qmiLocInjectApCacheDataIndMsgT_v02), qmiLocInjectApCacheDataIndMsgT_data_v02},
  {sizeof(qmiLocInjectApDoNotCacheDataReqMsgT_v02), qmiLocInjectApDoNotCacheDataReqMsgT_data_v02},
  {sizeof(qmiLocInjectApDoNotCacheDataIndMsgT_v02), qmiLocInjectApDoNotCacheDataIndMsgT_data_v02},
  {sizeof(qmiLocEventBatchingStatusIndMsgT_v02), qmiLocEventBatchingStatusIndMsgT_data_v02},
  {sizeof(qmiLocQueryAonConfigReqMsgT_v02), qmiLocQueryAonConfigReqMsgT_data_v02},
  {sizeof(qmiLocQueryAonConfigIndMsgT_v02), qmiLocQueryAonConfigIndMsgT_data_v02},
  {sizeof(qmiLocDeleteGNSSServiceDataReqMsgT_v02), qmiLocDeleteGNSSServiceDataReqMsgT_data_v02},
  {sizeof(qmiLocDeleteGNSSServiceDataIndMsgT_v02), qmiLocDeleteGNSSServiceDataIndMsgT_data_v02},
  {sizeof(qmiLocInjectXtraDataReqMsgT_v02), qmiLocInjectXtraDataReqMsgT_data_v02},
  {sizeof(qmiLocInjectXtraDataIndMsgT_v02), qmiLocInjectXtraDataIndMsgT_data_v02},
  {sizeof(qmiLocInjectXtraPcidReqMsgT_v02), qmiLocInjectXtraPcidReqMsgT_data_v02},
  {sizeof(qmiLocInjectXtraPcidIndMsgT_v02), qmiLocInjectXtraPcidIndMsgT_data_v02},
  {sizeof(qmiLocGetSupportedFeatureReqMsgT_v02), 0},
  {sizeof(qmiLocGetSupportedFeatureIndMsgT_v02), qmiLocGetSupportedFeatureIndMsgT_data_v02},
  {sizeof(qmiLocSetInternalStatusConfigReqMsgT_v02), qmiLocSetInternalStatusConfigReqMsgT_data_v02},
  {sizeof(qmiLocSetInternalStatusConfigIndMsgT_v02), qmiLocSetInternalStatusConfigIndMsgT_data_v02},
  {sizeof(qmiLocEventInternalStatusReportIndMsgT_v02), qmiLocEventInternalStatusReportIndMsgT_data_v02},
  {sizeof(qmiLocEventInjectSrnApDataReqIndMsgT_v02), qmiLocEventInjectSrnApDataReqIndMsgT_data_v02},
  {sizeof(qmiLocInjectSrnApDataReqMsgT_v02), qmiLocInjectSrnApDataReqMsgT_data_v02},
  {sizeof(qmiLocInjectSrnApDataIndMsgT_v02), qmiLocInjectSrnApDataIndMsgT_data_v02},
  {sizeof(qmiLocCrowdSourceManagerControlReqMsgT_v02), qmiLocCrowdSourceManagerControlReqMsgT_data_v02},
  {sizeof(qmiLocCrowdSourceManagerControlIndMsgT_v02), qmiLocCrowdSourceManagerControlIndMsgT_data_v02},
  {sizeof(qmiLocCrowdSourceManagerReadDataReqMsgT_v02), qmiLocCrowdSourceManagerReadDataReqMsgT_data_v02},
  {sizeof(qmiLocQueryXtraInfoReqMsgT_v02), qmiLocQueryXtraInfoReqMsgT_data_v02},
  {sizeof(qmiLocQueryXtraInfoIndMsgT_v02), qmiLocQueryXtraInfoIndMsgT_data_v02},
  {sizeof(qmiLocStartOutdoorTripBatchingReqMsgT_v02), qmiLocStartOutdoorTripBatchingReqMsgT_data_v02},
  {sizeof(qmiLocStartOutdoorTripBatchingIndMsgT_v02), qmiLocStartOutdoorTripBatchingIndMsgT_data_v02},
  {sizeof(qmiLocQueryOTBAccumulatedDistanceReqMsgT_v02), 0},
  {sizeof(qmiLocQueryOTBAccumulatedDistanceIndMsgT_v02), qmiLocQueryOTBAccumulatedDistanceIndMsgT_data_v02},
  {sizeof(qmiLocEventFdclServiceReqIndMsgT_v02), qmiLocEventFdclServiceReqIndMsgT_data_v02},
  {sizeof(qmiLocGetFdclBsListReqMsgT_v02), qmiLocGetFdclBsListReqMsgT_data_v02},
  {sizeof(qmiLocGetFdclBsListIndMsgT_v02), qmiLocGetFdclBsListIndMsgT_data_v02},
  {sizeof(qmiLocInjectFdclDataReqMsgT_v02), qmiLocInjectFdclDataReqMsgT_data_v02},
  {sizeof(qmiLocInjectFdclDataIndMsgT_v02), qmiLocInjectFdclDataIndMsgT_data_v02},
  {sizeof(qmiLocSetBlacklistSvReqMsgT_v02), qmiLocSetBlacklistSvReqMsgT_data_v02},
  {sizeof(qmiLocGetBlacklistSvIndMsgT_v02), qmiLocGetBlacklistSvIndMsgT_data_v02},
  {sizeof(qmiLocSetConstellationConfigReqMsgT_v02), qmiLocSetConstellationConfigReqMsgT_data_v02},
  {sizeof(qmiLocEventDcReportIndMsgT_v02), qmiLocEventDcReportIndMsgT_data_v02},
  {sizeof(qmiLocRegisterMasterClientReqMsgT_v02), qmiLocRegisterMasterClientReqMsgT_data_v02},
  {sizeof(qmiLocRegisterMasterClientIndMsgT_v02), qmiLocRegisterMasterClientIndMsgT_data_v02},
  {sizeof(qmiLocEventEngineLockStateIndMsgT_v02), qmiLocEventEngineLockStateIndMsgT_data_v02},
  {sizeof(qmiLocStartListenPositionReqMsgT_v02), 0},
  {sizeof(qmiLocStopListenPositionReqMsgT_v02), 0},
  {sizeof(qmiLocSystemInfoReqMsgT_v02), qmiLocSystemInfoReqMsgT_data_v02},
  {sizeof(qmiLocSystemInfoIndMsgT_v02), qmiLocSystemInfoIndMsgT_data_v02},
  {sizeof(qmiLocGetConstellationConfigIndMsgT_v02), qmiLocGetConstellationConfigIndMsgT_data_v02},
  {sizeof(qmiLocEnablePositionInjectionTestModeReqMsgT_v02), qmiLocEnablePositionInjectionTestModeReqMsgT_data_v02},
  {sizeof(qmiLocEnablePositionInjectionTestModeIndMsgT_v02), qmiLocEnablePositionInjectionTestModeIndMsgT_data_v02},
  {sizeof(qmiLocInjectTestPositionReqMsgT_v02), qmiLocInjectTestPositionReqMsgT_data_v02},
  {sizeof(qmiLocInjectTestPositionIndMsgT_v02), qmiLocInjectTestPositionIndMsgT_data_v02},
  {sizeof(qmiLocEventBsObsDataServiceReqIndMsgT_v02), qmiLocEventBsObsDataServiceReqIndMsgT_data_v02},
  {sizeof(qmiLocGetBsObsDataReqMsgT_v02), qmiLocGetBsObsDataReqMsgT_data_v02},
  {sizeof(qmiLocGetBsObsDataIndMsgT_v02), qmiLocGetBsObsDataIndMsgT_data_v02},
  {sizeof(qmiLocGpsEphemerisReportIndMsgT_v02), qmiLocGpsEphemerisReportIndMsgT_data_v02},
  {sizeof(qmiLocGloEphemerisReportIndMsgT_v02), qmiLocGloEphemerisReportIndMsgT_data_v02},
  {sizeof(qmiLocBdsEphemerisReportIndMsgT_v02), qmiLocBdsEphemerisReportIndMsgT_data_v02},
  {sizeof(qmiLocGalEphemerisReportIndMsgT_v02), qmiLocGalEphemerisReportIndMsgT_data_v02},
  {sizeof(qmiLocQzssEphemerisReportIndMsgT_v02), qmiLocQzssEphemerisReportIndMsgT_data_v02},
  {sizeof(qmiLocSetConstrainedTuncModeReqMsgT_v02), qmiLocSetConstrainedTuncModeReqMsgT_data_v02},
  {sizeof(qmiLocSetConstrainedTuncModeIndMsgT_v02), qmiLocSetConstrainedTuncModeIndMsgT_data_v02},
  {sizeof(qmiLocEnablePositionAssistedClockEstReqMsgT_v02), qmiLocEnablePositionAssistedClockEstReqMsgT_data_v02},
  {sizeof(qmiLocEnablePositionAssistedClockEstIndMsgT_v02), qmiLocEnablePositionAssistedClockEstIndMsgT_data_v02},
  {sizeof(qmiLocQueryGNSSEnergyConsumedReqMsgT_v02), 0},
  {sizeof(qmiLocQueryGNSSEnergyConsumedIndMsgT_v02), qmiLocQueryGNSSEnergyConsumedIndMsgT_data_v02},
  {sizeof(qmiLocEventReportIndMsgT_v02), qmiLocEventReportIndMsgT_data_v02},
  {sizeof(qmiLocGetBandMeasurementMetricsIndMsgT_v02), qmiLocGetBandMeasurementMetricsIndMsgT_data_v02},
  {sizeof(qmiLocLocationRequestNotificationIndMsgT_v02), qmiLocLocationRequestNotificationIndMsgT_data_v02},
  {sizeof(qmiLocInjectPlatformPowerStateReqMsgT_v02), qmiLocInjectPlatformPowerStateReqMsgT_data_v02},
  {sizeof(qmiLocInjectPlatformPowerStateIndMsgT_v02), qmiLocInjectPlatformPowerStateIndMsgT_data_v02},
  {sizeof(qmiLocSetRobustLocationReqMsgT_v02), qmiLocSetRobustLocationReqMsgT_data_v02},
  {sizeof(qmiLocGetRobustLocationConfigIndMsgT_v02), qmiLocGetRobustLocationConfigIndMsgT_data_v02},
  {sizeof(qmiLocEventInjectEnvAidingReqMsgT_v02), qmiLocEventInjectEnvAidingReqMsgT_data_v02},
  {sizeof(qmiLocSetMinGpsWeekNumberReqMsgT_v02), qmiLocSetMinGpsWeekNumberReqMsgT_data_v02},
  {sizeof(qmiLocGetMinGpsWeekNumberIndMsgT_v02), qmiLocGetMinGpsWeekNumberIndMsgT_data_v02},
  {sizeof(qmiLocSapInsParamsIndMsgT_v02), qmiLocSapInsParamsIndMsgT_data_v02},
  {sizeof(qmiLocEventQueryXtraInfoReqIndMsgT_v02), qmiLocEventQueryXtraInfoReqIndMsgT_data_v02},
  {sizeof(qmiLocSetParameterReqMsgT_v02), qmiLocSetParameterReqMsgT_data_v02},
  {sizeof(qmiLocGetParameterReqMsgT_v02), qmiLocGetParameterReqMsgT_data_v02},
  {sizeof(qmiLocGetParameterIndMsgT_v02), qmiLocGetParameterIndMsgT_data_v02},
  {sizeof(qmiLocSetStatisticsConfigReqMsgT_v02), qmiLocSetStatisticsConfigReqMsgT_data_v02},
  {sizeof(qmiLocGetStatisticsConfigIndMsgT_v02), qmiLocGetStatisticsConfigIndMsgT_data_v02},
  {sizeof(qmiLocGnssStatisticsReportIndMsgT_v02), qmiLocGnssStatisticsReportIndMsgT_data_v02},
  {sizeof(qmiLocSetGnssPdrModeReqMsgT_v02), qmiLocSetGnssPdrModeReqMsgT_data_v02},
  {sizeof(qmiLocGetGnssPdrModeIndMsgT_v02), qmiLocGetGnssPdrModeIndMsgT_data_v02},
  {sizeof(qmiLocGetMagCalStatePdrIndMsgT_v02), qmiLocGetMagCalStatePdrIndMsgT_data_v02},
  {sizeof(qmiLocSetMultibandConfigReqMsgT_v02), qmiLocSetMultibandConfigReqMsgT_data_v02},
  {sizeof(qmiLocGetMultibandConfigIndMsgT_v02), qmiLocGetMultibandConfigIndMsgT_data_v02},
  {sizeof(qmiLocLatencyInformationIndMsgT_v02), qmiLocLatencyInformationIndMsgT_data_v02},
  {sizeof(qmiLocInjectLocationCivicAddressReqMsgT_v02), qmiLocInjectLocationCivicAddressReqMsgT_data_v02},
  {sizeof(qmiLocPlatformPowerStateChangedIndMsgT_v02), qmiLocPlatformPowerStateChangedIndMsgT_data_v02},
  {sizeof(qmiLocInjectRawDataReqMsgT_v02), qmiLocInjectRawDataReqMsgT_data_v02},
  {sizeof(qmiLocInjectRawDataIndMsgT_v02), qmiLocInjectRawDataIndMsgT_data_v02},
  {sizeof(qmiLocEngineDebugDataIndMsgT_v02), qmiLocEngineDebugDataIndMsgT_data_v02},
  {sizeof(qmiLocSetTribandStateReqMsgT_v02), qmiLocSetTribandStateReqMsgT_data_v02},
  {sizeof(qmiLocGetTribandStateIndMsgT_v02), qmiLocGetTribandStateIndMsgT_data_v02},
  {sizeof(qmiLocSetSdkFeatureConfigReqMsgT_v02), qmiLocSetSdkFeatureConfigReqMsgT_data_v02},
  {sizeof(qmiLocSetSdkFeatureConfigIndMsgT_v02), qmiLocSetSdkFeatureConfigIndMsgT_data_v02},
  {sizeof(qmiLocGnssBandsSupportedIndMsgT_v02), qmiLocGnssBandsSupportedIndMsgT_data_v02},
  {sizeof(qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_v02), qmiLocOsnmaPublicKeyMerkleTreeReqMsgT_data_v02},
  {sizeof(qmiLocSetOsnmaStateReqMsgT_v02), qmiLocSetOsnmaStateReqMsgT_data_v02},
  {sizeof(qmiLocSetNtnStatusReqMsgT_v02), qmiLocSetNtnStatusReqMsgT_data_v02},
  {sizeof(qmiLocSetNtnStatusIndMsgT_v02), qmiLocSetNtnStatusIndMsgT_data_v02},
  {sizeof(qmiLocGetNtnStatusIndMsgT_v02), qmiLocGetNtnStatusIndMsgT_data_v02},
  {sizeof(qmiLocNtnConfigUpdateIndMsgT_v02), qmiLocNtnConfigUpdateIndMsgT_data_v02},
  {sizeof(qmiLocEventDbhPositionIndMsgT_v02), qmiLocEventDbhPositionIndMsgT_data_v02}
};

/* Range Table */
/* No Ranges Defined in IDL */

/* Predefine the Type Table Object */
static const qmi_idl_type_table_object loc_qmi_idl_type_table_object_v02;

/*Referenced Tables Array*/
static const qmi_idl_type_table_object *loc_qmi_idl_type_table_object_referenced_tables_v02[] =
{&loc_qmi_idl_type_table_object_v02, &common_qmi_idl_type_table_object_v01};

/*Type Table Object*/
static const qmi_idl_type_table_object loc_qmi_idl_type_table_object_v02 = {
  sizeof(loc_type_table_v02)/sizeof(qmi_idl_type_table_entry ),
  sizeof(loc_message_table_v02)/sizeof(qmi_idl_message_table_entry),
  1,
  loc_type_table_v02,
  loc_message_table_v02,
  loc_qmi_idl_type_table_object_referenced_tables_v02,
  NULL
};

/*Arrays of service_message_table_entries for commands, responses and indications*/
static const qmi_idl_service_message_table_entry loc_service_command_messages_v02[] = {
  {QMI_LOC_GET_SUPPORTED_MSGS_REQ_V02, QMI_IDL_TYPE16(1, 0), 0},
  {QMI_LOC_GET_SUPPORTED_FIELDS_REQ_V02, QMI_IDL_TYPE16(1, 2), 5},
  {QMI_LOC_INFORM_CLIENT_REVISION_REQ_V02, QMI_IDL_TYPE16(0, 3), 7},
  {QMI_LOC_REG_EVENTS_REQ_V02, QMI_IDL_TYPE16(0, 4), 29},
  {QMI_LOC_START_REQ_V02, QMI_IDL_TYPE16(0, 5), 150},
  {QMI_LOC_STOP_REQ_V02, QMI_IDL_TYPE16(0, 6), 4},
  {QMI_LOC_GET_SERVICE_REVISION_REQ_V02, QMI_IDL_TYPE16(0, 34), 0},
  {QMI_LOC_GET_FIX_CRITERIA_REQ_V02, QMI_IDL_TYPE16(0, 36), 0},
  {QMI_LOC_NI_USER_RESPONSE_REQ_V02, QMI_IDL_TYPE16(0, 38), 1345},
  {QMI_LOC_INJECT_PREDICTED_ORBITS_DATA_REQ_V02, QMI_IDL_TYPE16(0, 40), 1053},
  {QMI_LOC_GET_PREDICTED_ORBITS_DATA_SOURCE_REQ_V02, QMI_IDL_TYPE16(0, 42), 0},
  {QMI_LOC_GET_PREDICTED_ORBITS_DATA_VALIDITY_REQ_V02, QMI_IDL_TYPE16(0, 44), 0},
  {QMI_LOC_INJECT_UTC_TIME_REQ_V02, QMI_IDL_TYPE16(0, 46), 25},
  {QMI_LOC_INJECT_POSITION_REQ_V02, QMI_IDL_TYPE16(0, 48), 540},
  {QMI_LOC_SET_ENGINE_LOCK_REQ_V02, QMI_IDL_TYPE16(0, 50), 36},
  {QMI_LOC_GET_ENGINE_LOCK_REQ_V02, QMI_IDL_TYPE16(0, 52), 7},
  {QMI_LOC_SET_SBAS_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 54), 8},
  {QMI_LOC_GET_SBAS_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 56), 0},
  {QMI_LOC_SET_NMEA_TYPES_REQ_V02, QMI_IDL_TYPE16(0, 58), 7},
  {QMI_LOC_GET_NMEA_TYPES_REQ_V02, QMI_IDL_TYPE16(0, 60), 0},
  {QMI_LOC_SET_LOW_POWER_MODE_REQ_V02, QMI_IDL_TYPE16(0, 62), 4},
  {QMI_LOC_GET_LOW_POWER_MODE_REQ_V02, QMI_IDL_TYPE16(0, 64), 0},
  {QMI_LOC_SET_SERVER_REQ_V02, QMI_IDL_TYPE16(0, 66), 297},
  {QMI_LOC_GET_SERVER_REQ_V02, QMI_IDL_TYPE16(0, 68), 11},
  {QMI_LOC_DELETE_ASSIST_DATA_REQ_V02, QMI_IDL_TYPE16(0, 70), 1160},
  {QMI_LOC_SET_XTRA_T_SESSION_CONTROL_REQ_V02, QMI_IDL_TYPE16(0, 72), 4},
  {QMI_LOC_GET_XTRA_T_SESSION_CONTROL_REQ_V02, QMI_IDL_TYPE16(0, 74), 0},
  {QMI_LOC_INJECT_WIFI_POSITION_REQ_V02, QMI_IDL_TYPE16(0, 76), 2353},
  {QMI_LOC_NOTIFY_WIFI_STATUS_REQ_V02, QMI_IDL_TYPE16(0, 78), 7},
  {QMI_LOC_GET_REGISTERED_EVENTS_REQ_V02, QMI_IDL_TYPE16(0, 80), 0},
  {QMI_LOC_SET_OPERATION_MODE_REQ_V02, QMI_IDL_TYPE16(0, 82), 14},
  {QMI_LOC_GET_OPERATION_MODE_REQ_V02, QMI_IDL_TYPE16(0, 84), 0},
  {QMI_LOC_SET_SPI_STATUS_REQ_V02, QMI_IDL_TYPE16(0, 86), 8},
  {QMI_LOC_INJECT_SENSOR_DATA_REQ_V02, QMI_IDL_TYPE16(0, 88), 2779},
  {QMI_LOC_INJECT_TIME_SYNC_DATA_REQ_V02, QMI_IDL_TYPE16(0, 90), 21},
  {QMI_LOC_SET_CRADLE_MOUNT_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 94), 11},
  {QMI_LOC_GET_CRADLE_MOUNT_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 92), 0},
  {QMI_LOC_SET_EXTERNAL_POWER_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 98), 14},
  {QMI_LOC_GET_EXTERNAL_POWER_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 96), 0},
  {QMI_LOC_INFORM_LOCATION_SERVER_CONN_STATUS_REQ_V02, QMI_IDL_TYPE16(0, 100), 140},
  {QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_REQ_V02, QMI_IDL_TYPE16(0, 102), 86},
  {QMI_LOC_GET_PROTOCOL_CONFIG_PARAMETERS_REQ_V02, QMI_IDL_TYPE16(0, 104), 11},
  {QMI_LOC_SET_SENSOR_CONTROL_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 106), 14},
  {QMI_LOC_GET_SENSOR_CONTROL_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 108), 0},
  {QMI_LOC_SET_SENSOR_PROPERTIES_REQ_V02, QMI_IDL_TYPE16(0, 110), 88},
  {QMI_LOC_GET_SENSOR_PROPERTIES_REQ_V02, QMI_IDL_TYPE16(0, 112), 7},
  {QMI_LOC_SET_SENSOR_PERFORMANCE_CONTROL_CONFIGURATION_REQ_V02, QMI_IDL_TYPE16(0, 114), 42},
  {QMI_LOC_GET_SENSOR_PERFORMANCE_CONTROL_CONFIGURATION_REQ_V02, QMI_IDL_TYPE16(0, 116), 0},
  {QMI_LOC_INJECT_SUPL_CERTIFICATE_REQ_V02, QMI_IDL_TYPE16(0, 118), 2009},
  {QMI_LOC_DELETE_SUPL_CERTIFICATE_REQ_V02, QMI_IDL_TYPE16(0, 120), 4},
  {QMI_LOC_SET_POSITION_ENGINE_CONFIG_PARAMETERS_REQ_V02, QMI_IDL_TYPE16(0, 122), 16},
  {QMI_LOC_GET_POSITION_ENGINE_CONFIG_PARAMETERS_REQ_V02, QMI_IDL_TYPE16(0, 124), 7},
  {QMI_LOC_ADD_CIRCULAR_GEOFENCE_REQ_V02, QMI_IDL_TYPE16(0, 126), 70},
  {QMI_LOC_DELETE_GEOFENCE_REQ_V02, QMI_IDL_TYPE16(0, 128), 14},
  {QMI_LOC_QUERY_GEOFENCE_REQ_V02, QMI_IDL_TYPE16(0, 130), 14},
  {QMI_LOC_EDIT_GEOFENCE_REQ_V02, QMI_IDL_TYPE16(0, 136), 32},
  {QMI_LOC_GET_BEST_AVAILABLE_POSITION_REQ_V02, QMI_IDL_TYPE16(0, 141), 14},
  {QMI_LOC_INJECT_MOTION_DATA_REQ_V02, QMI_IDL_TYPE16(0, 143), 19},
  {QMI_LOC_GET_NI_GEOFENCE_ID_LIST_REQ_V02, QMI_IDL_TYPE16(0, 145), 7},
  {QMI_LOC_INJECT_GSM_CELL_INFO_REQ_V02, QMI_IDL_TYPE16(0, 147), 30},
  {QMI_LOC_INJECT_NETWORK_INITIATED_MESSAGE_REQ_V02, QMI_IDL_TYPE16(0, 155), 1043},
  {QMI_LOC_WWAN_OUT_OF_SERVICE_NOTIFICATION_REQ_V02, QMI_IDL_TYPE16(0, 157), 0},
  {QMI_LOC_PEDOMETER_REPORT_REQ_V02, QMI_IDL_TYPE16(0, 159), 46},
  {QMI_LOC_INJECT_WCDMA_CELL_INFO_REQ_V02, QMI_IDL_TYPE16(0, 149), 36},
  {QMI_LOC_INJECT_TDSCDMA_CELL_INFO_REQ_V02, QMI_IDL_TYPE16(0, 151), 33},
  {QMI_LOC_INJECT_SUBSCRIBER_ID_REQ_V02, QMI_IDL_TYPE16(0, 153), 22},
  {QMI_LOC_SET_GEOFENCE_ENGINE_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 132), 224},
  {QMI_LOC_GET_GEOFENCE_ENGINE_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 134), 7},
  {QMI_LOC_GET_BATCH_SIZE_REQ_V02, QMI_IDL_TYPE16(0, 161), 21},
  {QMI_LOC_START_BATCHING_REQ_V02, QMI_IDL_TYPE16(0, 163), 39},
  {QMI_LOC_READ_FROM_BATCH_REQ_V02, QMI_IDL_TYPE16(0, 167), 21},
  {QMI_LOC_STOP_BATCHING_REQ_V02, QMI_IDL_TYPE16(0, 169), 21},
  {QMI_LOC_RELEASE_BATCH_REQ_V02, QMI_IDL_TYPE16(0, 171), 14},
  {QMI_LOC_INJECT_WIFI_AP_DATA_REQ_V02, QMI_IDL_TYPE16(0, 174), 5200},
  {QMI_LOC_NOTIFY_WIFI_ATTACHMENT_STATUS_REQ_V02, QMI_IDL_TYPE16(0, 176), 51},
  {QMI_LOC_NOTIFY_WIFI_ENABLED_STATUS_REQ_V02, QMI_IDL_TYPE16(0, 178), 7},
  {QMI_LOC_INJECT_VEHICLE_SENSOR_DATA_REQ_V02, QMI_IDL_TYPE16(0, 181), 3360},
  {QMI_LOC_GET_AVAILABLE_WWAN_POSITION_REQ_V02, QMI_IDL_TYPE16(0, 183), 7},
  {QMI_LOC_SET_PREMIUM_SERVICES_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 185), 14},
  {QMI_LOC_SET_XTRA_VERSION_CHECK_REQ_V02, QMI_IDL_TYPE16(0, 187), 7},
  {QMI_LOC_SET_GNSS_CONSTELL_REPORT_CONFIG_V02, QMI_IDL_TYPE16(0, 189), 34},
  {QMI_LOC_ADD_GEOFENCE_CONTEXT_REQ_V02, QMI_IDL_TYPE16(0, 193), 2517},
  {QMI_LOC_SET_GEOFENCE_ENGINE_CONTEXT_REQ_V02, QMI_IDL_TYPE16(0, 195), 25},
  {QMI_LOC_DELETE_GEOFENCE_CONTEXT_REQ_V02, QMI_IDL_TYPE16(0, 197), 21},
  {QMI_LOC_INJECT_GTP_CLIENT_DOWNLOADED_DATA_REQ_V02, QMI_IDL_TYPE16(0, 199), 517},
  {QMI_LOC_GDT_UPLOAD_BEGIN_STATUS_REQ_V02, QMI_IDL_TYPE16(0, 201), 21},
  {QMI_LOC_GDT_UPLOAD_END_REQ_V02, QMI_IDL_TYPE16(0, 203), 21},
  {QMI_LOC_START_DBT_REQ_V02, QMI_IDL_TYPE16(0, 215), 36},
  {QMI_LOC_STOP_DBT_REQ_V02, QMI_IDL_TYPE16(0, 217), 4},
  {QMI_LOC_SECURE_GET_AVAILABLE_POSITION_REQ_V02, QMI_IDL_TYPE16(0, 221), 268},
  {QMI_LOC_INJECT_TIME_ZONE_INFO_REQ_V02, QMI_IDL_TYPE16(0, 139), 30},
  {QMI_LOC_INJECT_APCACHE_DATA_REQ_V02, QMI_IDL_TYPE16(0, 223), 2180},
  {QMI_LOC_INJECT_APDONOTCACHE_DATA_REQ_V02, QMI_IDL_TYPE16(0, 225), 656},
  {QMI_LOC_QUERY_AON_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 228), 7},
  {QMI_LOC_GTP_AP_STATUS_REQ_V02, QMI_IDL_TYPE16(0, 205), 540},
  {QMI_LOC_GDT_DOWNLOAD_BEGIN_STATUS_REQ_V02, QMI_IDL_TYPE16(0, 207), 294},
  {QMI_LOC_GDT_DOWNLOAD_READY_STATUS_REQ_V02, QMI_IDL_TYPE16(0, 209), 280},
  {QMI_LOC_GDT_RECEIVE_DONE_STATUS_REQ_V02, QMI_IDL_TYPE16(0, 211), 21},
  {QMI_LOC_GDT_DOWNLOAD_END_STATUS_REQ_V02, QMI_IDL_TYPE16(0, 213), 21},
  {QMI_LOC_DELETE_GNSS_SERVICE_DATA_REQ_V02, QMI_IDL_TYPE16(0, 230), 47},
  {QMI_LOC_INJECT_XTRA_DATA_REQ_V02, QMI_IDL_TYPE16(0, 232), 1064},
  {QMI_LOC_INJECT_XTRA_PCID_REQ_V02, QMI_IDL_TYPE16(0, 234), 11},
  {QMI_LOC_GET_SUPPORTED_FEATURE_REQ_V02, QMI_IDL_TYPE16(0, 236), 0},
  {QMI_LOC_SET_INTERNAL_STATUS_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 238), 4},
  {QMI_LOC_INJECT_SRN_AP_DATA_REQ_V02, QMI_IDL_TYPE16(0, 242), 1448},
  {QMI_LOC_CROWDSOURCE_MANAGER_CONTROL_REQ_V02, QMI_IDL_TYPE16(0, 244), 545},
  {QMI_LOC_CROWDSOURCE_MANAGER_READ_DATA_REQ_V02, QMI_IDL_TYPE16(0, 246), 7},
  {QMI_LOC_QUERY_XTRA_INFO_REQ_V02, QMI_IDL_TYPE16(0, 247), 12},
  {QMI_LOC_START_OUTDOOR_TRIP_BATCHING_REQ_V02, QMI_IDL_TYPE16(0, 249), 25},
  {QMI_LOC_QUERY_OTB_ACCUMULATED_DISTANCE_REQ_V02, QMI_IDL_TYPE16(0, 251), 0},
  {QMI_LOC_GET_FDCL_BS_LIST_REQ_V02, QMI_IDL_TYPE16(0, 254), 23},
  {QMI_LOC_INJECT_FDCL_DATA_REQ_V02, QMI_IDL_TYPE16(0, 256), 2273},
  {QMI_LOC_SET_BLACKLIST_SV_REQ_V02, QMI_IDL_TYPE16(0, 258), 154},
  {QMI_LOC_GET_BLACKLIST_SV_REQ_V02, QMI_IDL_TYPE16(0, 0), 0},
  {QMI_LOC_SET_CONSTELLATION_CONTROL_REQ_V02, QMI_IDL_TYPE16(0, 260), 26},
  {QMI_LOC_REGISTER_MASTER_CLIENT_REQ_V02, QMI_IDL_TYPE16(0, 262), 7},
  {QMI_LOC_START_LISTEN_POSITION_REQ_V02, QMI_IDL_TYPE16(0, 265), 0},
  {QMI_LOC_STOP_LISTEN_POSITION_REQ_V02, QMI_IDL_TYPE16(0, 266), 0},
  {QMI_LOC_SYSTEM_INFO_REQ_V02, QMI_IDL_TYPE16(0, 267), 7},
  {QMI_LOC_GET_CONSTELLATION_CONTROL_REQ_V02, QMI_IDL_TYPE16(0, 0), 0},
  {QMI_LOC_ENABLE_POSITION_INJECTION_TEST_MODE_REQ_V02, QMI_IDL_TYPE16(0, 270), 4},
  {QMI_LOC_INJECT_TEST_POSITION_REQ_V02, QMI_IDL_TYPE16(0, 272), 47},
  {QMI_LOC_GET_BS_OBS_DATA_REQ_V02, QMI_IDL_TYPE16(0, 275), 16},
  {QMI_LOC_SET_CONSTRAINED_TUNC_MODE_REQ_V02, QMI_IDL_TYPE16(0, 282), 18},
  {QMI_LOC_ENABLE_POSITION_ASSISTED_CLOCK_EST_REQ_V02, QMI_IDL_TYPE16(0, 284), 4},
  {QMI_LOC_QUERY_GNSS_ENERGY_CONSUMED_REQ_V02, QMI_IDL_TYPE16(0, 286), 0},
  {QMI_LOC_INJECT_PLATFORM_POWER_STATE_REQ_V02, QMI_IDL_TYPE16(0, 291), 7},
  {QMI_LOC_SET_ROBUST_LOCATION_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 293), 8},
  {QMI_LOC_GET_ROBUST_LOCATION_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 0), 0},
  {QMI_LOC_INJECT_ENV_AIDING_REQ_V02, QMI_IDL_TYPE16(0, 295), 3816},
  {QMI_LOC_SET_MIN_GPS_WEEK_NUMBER_REQ_V02, QMI_IDL_TYPE16(0, 296), 5},
  {QMI_LOC_GET_MIN_GPS_WEEK_NUMBER_REQ_V02, QMI_IDL_TYPE16(0, 0), 0},
  {QMI_LOC_SET_PARAMETER_REQ_V02, QMI_IDL_TYPE16(0, 300), 22},
  {QMI_LOC_GET_PARAMETER_REQ_V02, QMI_IDL_TYPE16(0, 301), 7},
  {QMI_LOC_SET_STATISTICS_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 303), 11},
  {QMI_LOC_GET_STATISTICS_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 0), 0},
  {QMI_LOC_SET_GNSS_PDR_MODE_REQ_V02, QMI_IDL_TYPE16(0, 306), 7},
  {QMI_LOC_GET_GNSS_PDR_MODE_REQ_V02, QMI_IDL_TYPE16(0, 0), 0},
  {QMI_LOC_GET_MAG_CAL_STATE_PDR_REQ_V02, QMI_IDL_TYPE16(0, 0), 0},
  {QMI_LOC_SET_MULTIBAND_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 309), 11},
  {QMI_LOC_GET_MULTIBAND_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 0), 0},
  {QMI_LOC_INJECT_LOCATION_CIVIC_ADDRESS_REQ_V02, QMI_IDL_TYPE16(0, 312), 900},
  {QMI_LOC_INJECT_RAW_DATA_REQ_V02, QMI_IDL_TYPE16(0, 314), 4029},
  {QMI_LOC_SET_TRIBAND_STATE_REQ_V02, QMI_IDL_TYPE16(0, 317), 4},
  {QMI_LOC_GET_TRIBAND_STATE_REQ_V02, QMI_IDL_TYPE16(0, 0), 0},
  {QMI_LOC_SET_SDK_FEATURE_CONFIG_REQ_V02, QMI_IDL_TYPE16(0, 319), 12379},
  {QMI_LOC_OSNMA_PUBLIC_KEY_MERKLE_TREE_REQ_V02, QMI_IDL_TYPE16(0, 322), 270},
  {QMI_LOC_SET_OSNMA_STATE_REQ_V02, QMI_IDL_TYPE16(0, 323), 4},
  {QMI_LOC_SET_NTN_STATUS_REQ_V02, QMI_IDL_TYPE16(0, 324), 11},
  {QMI_LOC_GET_NTN_STATUS_REQ_V02, QMI_IDL_TYPE16(0, 0), 0}
};

static const qmi_idl_service_message_table_entry loc_service_response_messages_v02[] = {
  {QMI_LOC_GET_SUPPORTED_MSGS_RESP_V02, QMI_IDL_TYPE16(1, 1), 8204},
  {QMI_LOC_GET_SUPPORTED_FIELDS_RESP_V02, QMI_IDL_TYPE16(1, 3), 115},
  {QMI_LOC_INFORM_CLIENT_REVISION_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_REG_EVENTS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_START_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_STOP_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_SERVICE_REVISION_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_FIX_CRITERIA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_NI_USER_RESPONSE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_PREDICTED_ORBITS_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_PREDICTED_ORBITS_DATA_SOURCE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_PREDICTED_ORBITS_DATA_VALIDITY_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_UTC_TIME_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_POSITION_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_ENGINE_LOCK_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_ENGINE_LOCK_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_SBAS_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_SBAS_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_NMEA_TYPES_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_NMEA_TYPES_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_LOW_POWER_MODE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_LOW_POWER_MODE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_SERVER_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_SERVER_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_DELETE_ASSIST_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_XTRA_T_SESSION_CONTROL_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_XTRA_T_SESSION_CONTROL_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_WIFI_POSITION_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_NOTIFY_WIFI_STATUS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_REGISTERED_EVENTS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_OPERATION_MODE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_OPERATION_MODE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_SPI_STATUS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_SENSOR_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_TIME_SYNC_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_CRADLE_MOUNT_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_CRADLE_MOUNT_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_EXTERNAL_POWER_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_EXTERNAL_POWER_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INFORM_LOCATION_SERVER_CONN_STATUS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_PROTOCOL_CONFIG_PARAMETERS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_SENSOR_CONTROL_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_SENSOR_CONTROL_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_SENSOR_PROPERTIES_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_SENSOR_PROPERTIES_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_SENSOR_PERFORMANCE_CONTROL_CONFIGURATION_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_SENSOR_PERFORMANCE_CONTROL_CONFIGURATION_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_SUPL_CERTIFICATE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_DELETE_SUPL_CERTIFICATE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_POSITION_ENGINE_CONFIG_PARAMETERS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_POSITION_ENGINE_CONFIG_PARAMETERS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_ADD_CIRCULAR_GEOFENCE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_DELETE_GEOFENCE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_QUERY_GEOFENCE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_EDIT_GEOFENCE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_BEST_AVAILABLE_POSITION_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_MOTION_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_NI_GEOFENCE_ID_LIST_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_GSM_CELL_INFO_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_NETWORK_INITIATED_MESSAGE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_WWAN_OUT_OF_SERVICE_NOTIFICATION_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_PEDOMETER_REPORT_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_WCDMA_CELL_INFO_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_TDSCDMA_CELL_INFO_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_SUBSCRIBER_ID_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_GEOFENCE_ENGINE_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_GEOFENCE_ENGINE_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_BATCH_SIZE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_START_BATCHING_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_READ_FROM_BATCH_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_STOP_BATCHING_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_RELEASE_BATCH_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_WIFI_AP_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_NOTIFY_WIFI_ATTACHMENT_STATUS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_NOTIFY_WIFI_ENABLED_STATUS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_VEHICLE_SENSOR_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_AVAILABLE_WWAN_POSITION_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_PREMIUM_SERVICES_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_XTRA_VERSION_CHECK_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_GNSS_CONSTELL_REPORT_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_ADD_GEOFENCE_CONTEXT_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_GEOFENCE_ENGINE_CONTEXT_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_DELETE_GEOFENCE_CONTEXT_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_GTP_CLIENT_DOWNLOADED_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GDT_UPLOAD_BEGIN_STATUS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GDT_UPLOAD_END_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_START_DBT_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_STOP_DBT_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SECURE_GET_AVAILABLE_POSITION_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_TIME_ZONE_INFO_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_APCACHE_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_APDONOTCACHE_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_QUERY_AON_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GTP_AP_STATUS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GDT_DOWNLOAD_BEGIN_STATUS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GDT_DOWNLOAD_READY_STATUS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GDT_RECEIVE_DONE_STATUS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GDT_DOWNLOAD_END_STATUS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_DELETE_GNSS_SERVICE_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_XTRA_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_XTRA_PCID_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_SUPPORTED_FEATURE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_INTERNAL_STATUS_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_SRN_AP_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_CROWDSOURCE_MANAGER_CONTROL_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_CROWDSOURCE_MANAGER_READ_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_QUERY_XTRA_INFO_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_START_OUTDOOR_TRIP_BATCHING_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_QUERY_OTB_ACCUMULATED_DISTANCE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_FDCL_BS_LIST_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_FDCL_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_BLACKLIST_SV_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_BLACKLIST_SV_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_CONSTELLATION_CONTROL_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_REGISTER_MASTER_CLIENT_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_START_LISTEN_POSITION_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_STOP_LISTEN_POSITION_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SYSTEM_INFO_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_CONSTELLATION_CONTROL_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_ENABLE_POSITION_INJECTION_TEST_MODE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_TEST_POSITION_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_BS_OBS_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_CONSTRAINED_TUNC_MODE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_ENABLE_POSITION_ASSISTED_CLOCK_EST_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_QUERY_GNSS_ENERGY_CONSUMED_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_PLATFORM_POWER_STATE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_ROBUST_LOCATION_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_ROBUST_LOCATION_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_ENV_AIDING_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_MIN_GPS_WEEK_NUMBER_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_MIN_GPS_WEEK_NUMBER_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_PARAMETER_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_PARAMETER_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_STATISTICS_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_STATISTICS_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_GNSS_PDR_MODE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_GNSS_PDR_MODE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_MAG_CAL_STATE_PDR_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_MULTIBAND_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_MULTIBAND_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_LOCATION_CIVIC_ADDRESS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_INJECT_RAW_DATA_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_TRIBAND_STATE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_TRIBAND_STATE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_SDK_FEATURE_CONFIG_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_OSNMA_PUBLIC_KEY_MERKLE_TREE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_OSNMA_STATE_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_SET_NTN_STATUS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7},
  {QMI_LOC_GET_NTN_STATUS_RESP_V02, QMI_IDL_TYPE16(0, 1), 7}
};

static const qmi_idl_service_message_table_entry loc_service_indication_messages_v02[] = {
  {QMI_LOC_EVENT_POSITION_REPORT_IND_V02, QMI_IDL_TYPE16(0, 7), 7642},
  {QMI_LOC_EVENT_GNSS_SV_INFO_IND_V02, QMI_IDL_TYPE16(0, 8), 9476},
  {QMI_LOC_EVENT_NMEA_IND_V02, QMI_IDL_TYPE16(0, 9), 4301},
  {QMI_LOC_EVENT_NI_NOTIFY_VERIFY_REQ_IND_V02, QMI_IDL_TYPE16(0, 10), 1342},
  {QMI_LOC_EVENT_INJECT_TIME_REQ_IND_V02, QMI_IDL_TYPE16(0, 11), 776},
  {QMI_LOC_EVENT_INJECT_PREDICTED_ORBITS_REQ_IND_V02, QMI_IDL_TYPE16(0, 12), 819},
  {QMI_LOC_EVENT_INJECT_POSITION_REQ_IND_V02, QMI_IDL_TYPE16(0, 13), 40},
  {QMI_LOC_EVENT_ENGINE_STATE_IND_V02, QMI_IDL_TYPE16(0, 14), 7},
  {QMI_LOC_EVENT_FIX_SESSION_STATE_IND_V02, QMI_IDL_TYPE16(0, 15), 11},
  {QMI_LOC_EVENT_WIFI_REQ_IND_V02, QMI_IDL_TYPE16(0, 16), 20},
  {QMI_LOC_EVENT_SENSOR_STREAMING_READY_STATUS_IND_V02, QMI_IDL_TYPE16(0, 17), 48},
  {QMI_LOC_EVENT_TIME_SYNC_REQ_IND_V02, QMI_IDL_TYPE16(0, 18), 7},
  {QMI_LOC_EVENT_SET_SPI_STREAMING_REPORT_IND_V02, QMI_IDL_TYPE16(0, 19), 4},
  {QMI_LOC_EVENT_LOCATION_SERVER_CONNECTION_REQ_IND_V02, QMI_IDL_TYPE16(0, 20), 46},
  {QMI_LOC_GET_SERVICE_REVISION_IND_V02, QMI_IDL_TYPE16(0, 35), 532},
  {QMI_LOC_GET_FIX_CRITERIA_IND_V02, QMI_IDL_TYPE16(0, 37), 135},
  {QMI_LOC_NI_USER_RESPONSE_IND_V02, QMI_IDL_TYPE16(0, 39), 7},
  {QMI_LOC_INJECT_PREDICTED_ORBITS_DATA_IND_V02, QMI_IDL_TYPE16(0, 41), 12},
  {QMI_LOC_GET_PREDICTED_ORBITS_DATA_SOURCE_IND_V02, QMI_IDL_TYPE16(0, 43), 790},
  {QMI_LOC_GET_PREDICTED_ORBITS_DATA_VALIDITY_IND_V02, QMI_IDL_TYPE16(0, 45), 20},
  {QMI_LOC_INJECT_UTC_TIME_IND_V02, QMI_IDL_TYPE16(0, 47), 7},
  {QMI_LOC_INJECT_POSITION_IND_V02, QMI_IDL_TYPE16(0, 49), 7},
  {QMI_LOC_SET_ENGINE_LOCK_IND_V02, QMI_IDL_TYPE16(0, 51), 7},
  {QMI_LOC_GET_ENGINE_LOCK_IND_V02, QMI_IDL_TYPE16(0, 53), 50},
  {QMI_LOC_SET_SBAS_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 55), 7},
  {QMI_LOC_GET_SBAS_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 57), 11},
  {QMI_LOC_SET_NMEA_TYPES_IND_V02, QMI_IDL_TYPE16(0, 59), 7},
  {QMI_LOC_GET_NMEA_TYPES_IND_V02, QMI_IDL_TYPE16(0, 61), 14},
  {QMI_LOC_SET_LOW_POWER_MODE_IND_V02, QMI_IDL_TYPE16(0, 63), 7},
  {QMI_LOC_GET_LOW_POWER_MODE_IND_V02, QMI_IDL_TYPE16(0, 65), 11},
  {QMI_LOC_SET_SERVER_IND_V02, QMI_IDL_TYPE16(0, 67), 7},
  {QMI_LOC_GET_SERVER_IND_V02, QMI_IDL_TYPE16(0, 69), 304},
  {QMI_LOC_DELETE_ASSIST_DATA_IND_V02, QMI_IDL_TYPE16(0, 71), 7},
  {QMI_LOC_SET_XTRA_T_SESSION_CONTROL_IND_V02, QMI_IDL_TYPE16(0, 73), 7},
  {QMI_LOC_GET_XTRA_T_SESSION_CONTROL_IND_V02, QMI_IDL_TYPE16(0, 75), 11},
  {QMI_LOC_INJECT_WIFI_POSITION_IND_V02, QMI_IDL_TYPE16(0, 77), 7},
  {QMI_LOC_NOTIFY_WIFI_STATUS_IND_V02, QMI_IDL_TYPE16(0, 79), 7},
  {QMI_LOC_GET_REGISTERED_EVENTS_IND_V02, QMI_IDL_TYPE16(0, 81), 18},
  {QMI_LOC_SET_OPERATION_MODE_IND_V02, QMI_IDL_TYPE16(0, 83), 7},
  {QMI_LOC_GET_OPERATION_MODE_IND_V02, QMI_IDL_TYPE16(0, 85), 14},
  {QMI_LOC_SET_SPI_STATUS_IND_V02, QMI_IDL_TYPE16(0, 87), 7},
  {QMI_LOC_INJECT_SENSOR_DATA_IND_V02, QMI_IDL_TYPE16(0, 89), 34},
  {QMI_LOC_INJECT_TIME_SYNC_DATA_IND_V02, QMI_IDL_TYPE16(0, 91), 7},
  {QMI_LOC_SET_CRADLE_MOUNT_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 95), 7},
  {QMI_LOC_GET_CRADLE_MOUNT_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 93), 18},
  {QMI_LOC_SET_EXTERNAL_POWER_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 99), 7},
  {QMI_LOC_GET_EXTERNAL_POWER_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 97), 14},
  {QMI_LOC_INFORM_LOCATION_SERVER_CONN_STATUS_IND_V02, QMI_IDL_TYPE16(0, 101), 7},
  {QMI_LOC_SET_PROTOCOL_CONFIG_PARAMETERS_IND_V02, QMI_IDL_TYPE16(0, 103), 18},
  {QMI_LOC_GET_PROTOCOL_CONFIG_PARAMETERS_IND_V02, QMI_IDL_TYPE16(0, 105), 93},
  {QMI_LOC_SET_SENSOR_CONTROL_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 107), 7},
  {QMI_LOC_GET_SENSOR_CONTROL_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 109), 21},
  {QMI_LOC_SET_SENSOR_PROPERTIES_IND_V02, QMI_IDL_TYPE16(0, 111), 14},
  {QMI_LOC_GET_SENSOR_PROPERTIES_IND_V02, QMI_IDL_TYPE16(0, 113), 95},
  {QMI_LOC_SET_SENSOR_PERFORMANCE_CONTROL_CONFIGURATION_IND_V02, QMI_IDL_TYPE16(0, 115), 14},
  {QMI_LOC_GET_SENSOR_PERFORMANCE_CONTROL_CONFIGURATION_IND_V02, QMI_IDL_TYPE16(0, 117), 49},
  {QMI_LOC_INJECT_SUPL_CERTIFICATE_IND_V02, QMI_IDL_TYPE16(0, 119), 7},
  {QMI_LOC_DELETE_SUPL_CERTIFICATE_IND_V02, QMI_IDL_TYPE16(0, 121), 7},
  {QMI_LOC_SET_POSITION_ENGINE_CONFIG_PARAMETERS_IND_V02, QMI_IDL_TYPE16(0, 123), 14},
  {QMI_LOC_GET_POSITION_ENGINE_CONFIG_PARAMETERS_IND_V02, QMI_IDL_TYPE16(0, 125), 23},
  {QMI_LOC_EVENT_NI_GEOFENCE_NOTIFICATION_IND_V02, QMI_IDL_TYPE16(0, 21), 14},
  {QMI_LOC_EVENT_GEOFENCE_GEN_ALERT_IND_V02, QMI_IDL_TYPE16(0, 22), 7},
  {QMI_LOC_EVENT_GEOFENCE_BREACH_NOTIFICATION_IND_V02, QMI_IDL_TYPE16(0, 23), 85},
  {QMI_LOC_ADD_CIRCULAR_GEOFENCE_IND_V02, QMI_IDL_TYPE16(0, 127), 21},
  {QMI_LOC_DELETE_GEOFENCE_IND_V02, QMI_IDL_TYPE16(0, 129), 21},
  {QMI_LOC_QUERY_GEOFENCE_IND_V02, QMI_IDL_TYPE16(0, 131), 65},
  {QMI_LOC_EDIT_GEOFENCE_IND_V02, QMI_IDL_TYPE16(0, 137), 28},
  {QMI_LOC_GET_BEST_AVAILABLE_POSITION_IND_V02, QMI_IDL_TYPE16(0, 142), 2221},
  {QMI_LOC_INJECT_MOTION_DATA_IND_V02, QMI_IDL_TYPE16(0, 144), 7},
  {QMI_LOC_GET_NI_GEOFENCE_ID_LIST_IND_V02, QMI_IDL_TYPE16(0, 146), 82},
  {QMI_LOC_INJECT_GSM_CELL_INFO_IND_V02, QMI_IDL_TYPE16(0, 148), 7},
  {QMI_LOC_INJECT_NETWORK_INITIATED_MESSAGE_IND_V02, QMI_IDL_TYPE16(0, 156), 7},
  {QMI_LOC_WWAN_OUT_OF_SERVICE_NOTIFICATION_IND_V02, QMI_IDL_TYPE16(0, 158), 7},
  {QMI_LOC_EVENT_PEDOMETER_CONTROL_IND_V02, QMI_IDL_TYPE16(0, 24), 15},
  {QMI_LOC_EVENT_MOTION_DATA_CONTROL_IND_V02, QMI_IDL_TYPE16(0, 25), 4},
  {QMI_LOC_PEDOMETER_REPORT_IND_V02, QMI_IDL_TYPE16(0, 160), 7},
  {QMI_LOC_INJECT_WCDMA_CELL_INFO_IND_V02, QMI_IDL_TYPE16(0, 150), 7},
  {QMI_LOC_INJECT_TDSCDMA_CELL_INFO_IND_V02, QMI_IDL_TYPE16(0, 152), 7},
  {QMI_LOC_INJECT_SUBSCRIBER_ID_IND_V02, QMI_IDL_TYPE16(0, 154), 7},
  {QMI_LOC_SET_GEOFENCE_ENGINE_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 133), 14},
  {QMI_LOC_GET_GEOFENCE_ENGINE_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 135), 39},
  {QMI_LOC_GET_BATCH_SIZE_IND_V02, QMI_IDL_TYPE16(0, 162), 21},
  {QMI_LOC_START_BATCHING_IND_V02, QMI_IDL_TYPE16(0, 164), 14},
  {QMI_LOC_EVENT_BATCH_FULL_NOTIFICATION_IND_V02, QMI_IDL_TYPE16(0, 165), 28},
  {QMI_LOC_EVENT_LIVE_BATCHED_POSITION_REPORT_IND_V02, QMI_IDL_TYPE16(0, 166), 90},
  {QMI_LOC_READ_FROM_BATCH_IND_V02, QMI_IDL_TYPE16(0, 168), 460},
  {QMI_LOC_STOP_BATCHING_IND_V02, QMI_IDL_TYPE16(0, 170), 21},
  {QMI_LOC_RELEASE_BATCH_IND_V02, QMI_IDL_TYPE16(0, 172), 14},
  {QMI_LOC_EVENT_INJECT_WIFI_AP_DATA_REQ_IND_V02, QMI_IDL_TYPE16(0, 173), 4},
  {QMI_LOC_INJECT_WIFI_AP_DATA_IND_V02, QMI_IDL_TYPE16(0, 175), 7},
  {QMI_LOC_NOTIFY_WIFI_ATTACHMENT_STATUS_IND_V02, QMI_IDL_TYPE16(0, 177), 7},
  {QMI_LOC_NOTIFY_WIFI_ENABLED_STATUS_IND_V02, QMI_IDL_TYPE16(0, 179), 7},
  {QMI_LOC_EVENT_GEOFENCE_BATCHED_BREACH_NOTIFICATION_IND_V02, QMI_IDL_TYPE16(0, 26), 3045},
  {QMI_LOC_EVENT_VEHICLE_DATA_READY_STATUS_IND_V02, QMI_IDL_TYPE16(0, 180), 12},
  {QMI_LOC_INJECT_VEHICLE_SENSOR_DATA_IND_V02, QMI_IDL_TYPE16(0, 182), 7},
  {QMI_LOC_GET_AVAILABLE_WWAN_POSITION_IND_V02, QMI_IDL_TYPE16(0, 184), 166},
  {QMI_LOC_SET_PREMIUM_SERVICES_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 186), 7},
  {QMI_LOC_SET_XTRA_VERSION_CHECK_IND_V02, QMI_IDL_TYPE16(0, 188), 7},
  {QMI_LOC_EVENT_GNSS_MEASUREMENT_REPORT_IND_V02, QMI_IDL_TYPE16(0, 191), 5426},
  {QMI_LOC_EVENT_SV_POLYNOMIAL_REPORT_IND_V02, QMI_IDL_TYPE16(0, 192), 690},
  {QMI_LOC_SET_GNSS_CONSTELL_REPORT_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 190), 7},
  {QMI_LOC_ADD_GEOFENCE_CONTEXT_IND_V02, QMI_IDL_TYPE16(0, 194), 28},
  {QMI_LOC_SET_GEOFENCE_ENGINE_CONTEXT_IND_V02, QMI_IDL_TYPE16(0, 196), 14},
  {QMI_LOC_DELETE_GEOFENCE_CONTEXT_IND_V02, QMI_IDL_TYPE16(0, 198), 28},
  {QMI_LOC_EVENT_GEOFENCE_PROXIMITY_NOTIFICATION_IND_V02, QMI_IDL_TYPE16(0, 27), 21},
  {QMI_LOC_INJECT_GTP_CLIENT_DOWNLOADED_DATA_IND_V02, QMI_IDL_TYPE16(0, 200), 7},
  {QMI_LOC_GDT_UPLOAD_BEGIN_STATUS_IND_V02, QMI_IDL_TYPE16(0, 202), 7},
  {QMI_LOC_GDT_UPLOAD_END_IND_V02, QMI_IDL_TYPE16(0, 204), 7},
  {QMI_LOC_EVENT_GDT_UPLOAD_BEGIN_STATUS_REQ_IND_V02, QMI_IDL_TYPE16(0, 29), 273},
  {QMI_LOC_EVENT_GDT_UPLOAD_END_REQ_IND_V02, QMI_IDL_TYPE16(0, 30), 21},
  {QMI_LOC_START_DBT_IND_V02, QMI_IDL_TYPE16(0, 216), 11},
  {QMI_LOC_EVENT_DBT_POSITION_REPORT_IND_V02, QMI_IDL_TYPE16(0, 219), 2074},
  {QMI_LOC_EVENT_DBT_SESSION_STATUS_IND_V02, QMI_IDL_TYPE16(0, 220), 11},
  {QMI_LOC_STOP_DBT_IND_V02, QMI_IDL_TYPE16(0, 218), 11},
  {QMI_LOC_SECURE_GET_AVAILABLE_POSITION_IND_V02, QMI_IDL_TYPE16(0, 222), 1043},
  {QMI_LOC_EVENT_GEOFENCE_BATCHED_DWELL_NOTIFICATION_IND_V02, QMI_IDL_TYPE16(0, 28), 3038},
  {QMI_LOC_EVENT_GET_TIME_ZONE_INFO_IND_V02, QMI_IDL_TYPE16(0, 138), 7},
  {QMI_LOC_INJECT_TIME_ZONE_INFO_IND_V02, QMI_IDL_TYPE16(0, 140), 7},
  {QMI_LOC_INJECT_APCACHE_DATA_IND_V02, QMI_IDL_TYPE16(0, 224), 70},
  {QMI_LOC_INJECT_APDONOTCACHE_DATA_IND_V02, QMI_IDL_TYPE16(0, 226), 7},
  {QMI_LOC_EVENT_BATCHING_STATUS_IND_V02, QMI_IDL_TYPE16(0, 227), 7},
  {QMI_LOC_QUERY_AON_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 229), 21},
  {QMI_LOC_GTP_AP_STATUS_IND_V02, QMI_IDL_TYPE16(0, 206), 18},
  {QMI_LOC_GDT_DOWNLOAD_BEGIN_STATUS_IND_V02, QMI_IDL_TYPE16(0, 208), 7},
  {QMI_LOC_GDT_DOWNLOAD_READY_STATUS_IND_V02, QMI_IDL_TYPE16(0, 210), 7},
  {QMI_LOC_GDT_RECEIVE_DONE_STATUS_IND_V02, QMI_IDL_TYPE16(0, 212), 7},
  {QMI_LOC_GDT_DOWNLOAD_END_STATUS_IND_V02, QMI_IDL_TYPE16(0, 214), 7},
  {QMI_LOC_EVENT_GDT_DOWNLOAD_BEGIN_REQ_IND_V02, QMI_IDL_TYPE16(0, 31), 5811},
  {QMI_LOC_EVENT_GDT_RECEIVE_DONE_IND_V02, QMI_IDL_TYPE16(0, 32), 21},
  {QMI_LOC_EVENT_GDT_DOWNLOAD_END_REQ_IND_V02, QMI_IDL_TYPE16(0, 33), 21},
  {QMI_LOC_DELETE_GNSS_SERVICE_DATA_IND_V02, QMI_IDL_TYPE16(0, 231), 7},
  {QMI_LOC_INJECT_XTRA_DATA_IND_V02, QMI_IDL_TYPE16(0, 233), 23},
  {QMI_LOC_INJECT_XTRA_PCID_IND_V02, QMI_IDL_TYPE16(0, 235), 7},
  {QMI_LOC_GET_SUPPORTED_FEATURE_IND_V02, QMI_IDL_TYPE16(0, 237), 122},
  {QMI_LOC_SET_INTERNAL_STATUS_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 239), 7},
  {QMI_LOC_EVENT_INTERNAL_STATUS_REPORT_IND_V02, QMI_IDL_TYPE16(0, 240), 3756},
  {QMI_LOC_EVENT_INJECT_SRN_AP_DATA_REQ_IND_V02, QMI_IDL_TYPE16(0, 241), 22},
  {QMI_LOC_INJECT_SRN_AP_DATA_IND_V02, QMI_IDL_TYPE16(0, 243), 7},
  {QMI_LOC_CROWDSOURCE_MANAGER_CONTROL_IND_V02, QMI_IDL_TYPE16(0, 245), 540},
  {QMI_LOC_QUERY_XTRA_INFO_IND_V02, QMI_IDL_TYPE16(0, 248), 1638},
  {QMI_LOC_START_OUTDOOR_TRIP_BATCHING_IND_V02, QMI_IDL_TYPE16(0, 250), 7},
  {QMI_LOC_QUERY_OTB_ACCUMULATED_DISTANCE_IND_V02, QMI_IDL_TYPE16(0, 252), 21},
  {QMI_LOC_EVENT_FDCL_SERVICE_REQ_IND_V02, QMI_IDL_TYPE16(0, 253), 618},
  {QMI_LOC_GET_FDCL_BS_LIST_IND_V02, QMI_IDL_TYPE16(0, 255), 2456},
  {QMI_LOC_INJECT_FDCL_DATA_IND_V02, QMI_IDL_TYPE16(0, 257), 272},
  {QMI_LOC_SET_BLACKLIST_SV_IND_V02, QMI_IDL_TYPE16(0, 2), 7},
  {QMI_LOC_GET_BLACKLIST_SV_IND_V02, QMI_IDL_TYPE16(0, 259), 84},
  {QMI_LOC_SET_CONSTELLATION_CONTROL_IND_V02, QMI_IDL_TYPE16(0, 2), 7},
  {QMI_LOC_DC_REPORT_IND_V02, QMI_IDL_TYPE16(0, 261), 84},
  {QMI_LOC_REGISTER_MASTER_CLIENT_IND_V02, QMI_IDL_TYPE16(0, 263), 7},
  {QMI_LOC_EVENT_ENGINE_LOCK_STATE_IND_V02, QMI_IDL_TYPE16(0, 264), 25},
  {QMI_LOC_SYSTEM_INFO_IND_V02, QMI_IDL_TYPE16(0, 268), 74},
  {QMI_LOC_GET_CONSTELLATION_CONTROL_IND_V02, QMI_IDL_TYPE16(0, 269), 49},
  {QMI_LOC_EVENT_UNPROPAGATED_POSITION_REPORT_IND_V02, QMI_IDL_TYPE16(0, 7), 7642},
  {QMI_LOC_ENABLE_POSITION_INJECTION_TEST_MODE_IND_V02, QMI_IDL_TYPE16(0, 271), 14},
  {QMI_LOC_INJECT_TEST_POSITION_IND_V02, QMI_IDL_TYPE16(0, 273), 7},
  {QMI_LOC_EVENT_BS_OBS_DATA_SERVICE_REQ_IND_V02, QMI_IDL_TYPE16(0, 274), 5},
  {QMI_LOC_GET_BS_OBS_DATA_IND_V02, QMI_IDL_TYPE16(0, 276), 10015},
  {QMI_LOC_EVENT_GPS_EPHEMERIS_REPORT_IND_V02, QMI_IDL_TYPE16(0, 277), 1425},
  {QMI_LOC_EVENT_GLONASS_EPHEMERIS_REPORT_IND_V02, QMI_IDL_TYPE16(0, 278), 897},
  {QMI_LOC_EVENT_BDS_EPHEMERIS_REPORT_IND_V02, QMI_IDL_TYPE16(0, 279), 1457},
  {QMI_LOC_EVENT_GALILEO_EPHEMERIS_REPORT_IND_V02, QMI_IDL_TYPE16(0, 280), 1481},
  {QMI_LOC_EVENT_QZSS_EPHEMERIS_REPORT_IND_V02, QMI_IDL_TYPE16(0, 281), 1425},
  {QMI_LOC_SET_CONSTRAINED_TUNC_MODE_IND_V02, QMI_IDL_TYPE16(0, 283), 7},
  {QMI_LOC_ENABLE_POSITION_ASSISTED_CLOCK_EST_IND_V02, QMI_IDL_TYPE16(0, 285), 7},
  {QMI_LOC_QUERY_GNSS_ENERGY_CONSUMED_IND_V02, QMI_IDL_TYPE16(0, 287), 22},
  {QMI_LOC_EVENT_REPORT_IND_V02, QMI_IDL_TYPE16(0, 288), 97},
  {QMI_LOC_GET_BAND_MEASUREMENT_METRICS_IND_V02, QMI_IDL_TYPE16(0, 289), 124},
  {QMI_LOC_LOCATION_REQUEST_NOTIFICATION_IND_V02, QMI_IDL_TYPE16(0, 290), 66},
  {QMI_LOC_INJECT_PLATFORM_POWER_STATE_IND_V02, QMI_IDL_TYPE16(0, 292), 7},
  {QMI_LOC_SET_ROBUST_LOCATION_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 2), 7},
  {QMI_LOC_GET_ROBUST_LOCATION_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 294), 21},
  {QMI_LOC_INJECT_ENV_AIDING_IND_V02, QMI_IDL_TYPE16(0, 2), 7},
  {QMI_LOC_SET_MIN_GPS_WEEK_NUMBER_IND_V02, QMI_IDL_TYPE16(0, 2), 7},
  {QMI_LOC_GET_MIN_GPS_WEEK_NUMBER_IND_V02, QMI_IDL_TYPE16(0, 297), 12},
  {QMI_LOC_EVENT_QUERY_XTRA_INFO_REQ_IND_V02, QMI_IDL_TYPE16(0, 299), 68},
  {QMI_LOC_SET_PARAMETER_IND_V02, QMI_IDL_TYPE16(0, 2), 7},
  {QMI_LOC_GET_PARAMETER_IND_V02, QMI_IDL_TYPE16(0, 302), 29},
  {QMI_LOC_SET_STATISTICS_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 2), 7},
  {QMI_LOC_GET_STATISTICS_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 304), 18},
  {QMI_LOC_GNSS_STATISTICS_REPORT_IND_V02, QMI_IDL_TYPE16(0, 305), 165},
  {QMI_LOC_SET_GNSS_PDR_MODE_IND_V02, QMI_IDL_TYPE16(0, 2), 7},
  {QMI_LOC_GET_GNSS_PDR_MODE_IND_V02, QMI_IDL_TYPE16(0, 307), 14},
  {QMI_LOC_GET_MAG_CAL_STATE_PDR_IND_V02, QMI_IDL_TYPE16(0, 308), 11},
  {QMI_LOC_EVENT_SAP_INS_PARAMETERS_IND_V02, QMI_IDL_TYPE16(0, 298), 5590},
  {QMI_LOC_SET_MULTIBAND_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 2), 7},
  {QMI_LOC_GET_MULTIBAND_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 310), 18},
  {QMI_LOC_LATENCY_INFORMATION_IND_V02, QMI_IDL_TYPE16(0, 311), 124},
  {QMI_LOC_INJECT_LOCATION_CIVIC_ADDRESS_IND_V02, QMI_IDL_TYPE16(0, 2), 7},
  {QMI_LOC_EVENT_PLATFORM_POWER_STATE_CHANGED_IND_V02, QMI_IDL_TYPE16(0, 313), 14},
  {QMI_LOC_INJECT_RAW_DATA_IND_V02, QMI_IDL_TYPE16(0, 315), 12},
  {QMI_LOC_ENGINE_DEBUG_DATA_IND_V02, QMI_IDL_TYPE16(0, 316), 3052},
  {QMI_LOC_SET_TRIBAND_STATE_IND_V02, QMI_IDL_TYPE16(0, 2), 7},
  {QMI_LOC_GET_TRIBAND_STATE_IND_V02, QMI_IDL_TYPE16(0, 318), 11},
  {QMI_LOC_SET_SDK_FEATURE_CONFIG_IND_V02, QMI_IDL_TYPE16(0, 320), 18},
  {QMI_LOC_GNSS_BANDS_SUPPORTED_IND_V02, QMI_IDL_TYPE16(0, 321), 22},
  {QMI_LOC_OSNMA_PUBLIC_KEY_MERKLE_TREE_IND_V02, QMI_IDL_TYPE16(0, 2), 7},
  {QMI_LOC_SET_OSNMA_STATE_IND_V02, QMI_IDL_TYPE16(0, 2), 7},
  {QMI_LOC_SET_NTN_STATUS_IND_V02, QMI_IDL_TYPE16(0, 325), 18},
  {QMI_LOC_GET_NTN_STATUS_IND_V02, QMI_IDL_TYPE16(0, 326), 18},
  {QMI_LOC_NTN_CONFIG_UPDATE_IND_V02, QMI_IDL_TYPE16(0, 327), 11},
  {QMI_LOC_EVENT_DBH_POSITION_IND_V02, QMI_IDL_TYPE16(0, 328), 540}
};

/*Service Object*/
struct qmi_idl_service_object loc_qmi_idl_service_object_v02 = {
  0x06,
  0x02,
  0x10,
  12379,
  { sizeof(loc_service_command_messages_v02)/sizeof(qmi_idl_service_message_table_entry),
    sizeof(loc_service_response_messages_v02)/sizeof(qmi_idl_service_message_table_entry),
    sizeof(loc_service_indication_messages_v02)/sizeof(qmi_idl_service_message_table_entry) },
  { loc_service_command_messages_v02, loc_service_response_messages_v02, loc_service_indication_messages_v02},
  &loc_qmi_idl_type_table_object_v02,
  0xAC,
  NULL
};

/* Service Object Accessor */
qmi_idl_service_object_type loc_get_service_object_internal_v02
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version ){
  if ( LOC_V02_IDL_MAJOR_VERS != idl_maj_version || LOC_V02_IDL_MINOR_VERS != idl_min_version
       || LOC_V02_IDL_TOOL_VERS != library_version)
  {
    return NULL;
  }
  return (qmi_idl_service_object_type)&loc_qmi_idl_service_object_v02;
}

