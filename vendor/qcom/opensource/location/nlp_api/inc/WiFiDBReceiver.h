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

#ifndef WIFI_DB_REC_H
#define WIFI_DB_REC_H

#include <DBCommon.h>

/******************************************************************************
WiFiDBReceiver
******************************************************************************/

typedef enum ReliabilityTypes : uint8_t {
    VERY_LOW  = 0,
    LOW       = 1,
    MEDIUM    = 2,
    HIGH      = 3,
    VERY_HIGH = 4,
} ReliabilityTypes;

typedef struct {
    uint8_t macAddress[6];
    double latitude;
    double longitude;
    float max_antenna_range;
    float horizontal_error;
    ReliabilityTypes reliability;
} APLocationData;

typedef enum APSpecialInfoType : uint8_t {
    NO_INFO_AVAILABLE = 0,
    MOVING_AP      = 1,
} APSpecialInfoType;

typedef struct {
    uint8_t macAddress[6];
    APSpecialInfoType specialInfoType;
} APSpecialInfo;

/** @brief
    All the memory pointers received will be never freed internally.
    Caller shall manage the memory before and after calling these functions.
*/
typedef struct {
    void (*requestAPList)(int expire_in_days);
    void (*requestScanList)();
    void (*pushWiFiDB)(const APLocationData* location_data, uint16_t location_data_count,
            const APSpecialInfo* special_info, uint16_t special_info_count, int days_valid);
    void (*pushLookupResult)(const APLocationData* location_data, uint16_t location_data_count,
            const APSpecialInfo* special_info, uint16_t special_info_count);
} WiFiDBReceiver;

/******************************************************************************
ResponseListener
******************************************************************************/

typedef enum FdalStatus : uint8_t {
    NOT_IN_FDAL = 0,
    IN_FDAL     = 1,
    BLACKLISTED = 2,
    NA          = 3,
} FdalStatus;

typedef struct {
    char ssid[8];
    CellInfo cellInfo;
} NlpAPInfoExtra;

typedef struct {
    uint8_t macAddress[6];
    uint64_t timestamp;
    FdalStatus fdalStatus;
    NlpAPInfoExtra extra;
} NlpAPInfo;

/** @brief
    All the memory pointers returned in these callbacks will be freed after call returns.
    Implementation of these callbacks shall copy the needed data before returning.
*/
typedef struct {
    void (*onAPListAvailable)(const NlpAPInfo* ap_list, uint16_t ap_list_count,
            ApBsListStatus ap_status, NlpLocation location, const void* clientData);
    void (*onLookupRequest)(const NlpAPInfo* ap_list, uint16_t ap_list_count, NlpLocation location,
            bool is_emergency, const void* clientData);
    void (*onStatusUpdate)(bool is_success, const char* error, const void* clientData);
    void (*onServiceRequest)(bool is_emergency, const void* clientData);
} WiFiDBReceiverResponseListener;

#endif /* WIFI_DB_REC_H */
