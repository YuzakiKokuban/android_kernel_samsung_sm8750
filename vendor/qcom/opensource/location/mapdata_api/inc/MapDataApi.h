/*
Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.

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

#ifndef MAP_DATA_API_H
#define MAP_DATA_API_H

#include <dlfcn.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
    API VERSION 1: Initial release
*/

#define VERSION_MAJOR               0x01
#define VERSION_MINOR               0x01

typedef enum {
    START_POINT_IS_INTERSECTION = 0x01,
    END_POINT_IS_INTERSECTION = 0x02,
} IntersectionPoint;

typedef enum {
    POSITION_IN_CHINA = 0,
    POSITION_NOT_IN_CHINA = 1,
    POSITION_UNKNOWN = 2,
} TriStatus;

typedef struct {
    int16_t     latOffset_degrees;
    int16_t     lonOffset_degrees;
    int16_t     altDelta_meters;
    int16_t     zLevel_meters;
} GeometryDataType;

typedef struct {
    int32_t     startNodeLat_degrees;
    int32_t     startNodeLon_degrees;
} GeoPositionType;

typedef struct {
    uint32_t            linkId;
    uint16_t            numPoints;
    uint8_t             numLanes;
    uint8_t             flags; /* Bit 0: Is Start point intersection */
                               /* Bit 1: Is End point intersection */
    GeoPositionType     startNodePosition;
} LinkDataHeader;

typedef struct {
    LinkDataHeader      linkDataHeader;
    GeometryDataType*   pGeometryData;
} LinkDataType;

typedef struct {
    uint8_t         version_major;
    uint8_t         version_minor;
    int32_t         requestLatWgs84_degrees;
    int32_t         requestLonWgs84_degrees;
    int32_t         requestLatServer_degrees;
    int32_t         requestLonServer_degrees;
    uint32_t        numLinks;
} MapDataHeader;

typedef struct {
    MapDataHeader   mapDataHeader;
    LinkDataType*   pLinkData;
} MapData;

typedef MapData* (*requestMapDataCb)(double requestLatWgs84_degrees,
                                     double requestLonWgs84_degrees,
                                     uint32_t requestPosRadius_meters,
                                     TriStatus posIsInChina);
typedef void(*shutdownCb)();

typedef struct {
    requestMapDataCb        requestMapData;
    shutdownCb              shutdown;
} MapDataServiceCallbacks;

typedef struct {
    void (*registerMapDataService)(MapDataServiceCallbacks& cbs);
} MapDataApi;

/** @brief
    Provides a C pointer to an instance of MapDataApi struct after dynamic
    linking to libmapdata_api.so.
*/
inline MapDataApi* linkGetMapDataApi() {
    typedef MapDataApi* (getMapDataApi)();

    getMapDataApi* getter = nullptr;
    void *handle = dlopen("libmapdata_api.so", RTLD_NOW);
    if (nullptr != handle) {
        getter = (getMapDataApi*)dlsym(handle, "getMapDataApi");
    }

    return (nullptr != getter) ? (MapDataApi*)(*getter)() : nullptr;
}

#ifdef __cplusplus
}
#endif

#endif /* MAP_DATA_API_H */
