/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.

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

#include <functional>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <LocationClientApi.h>
#include "LocIdlAPIStubImpl.hpp"
#include <time.h>
#define NSEC_IN_ONE_SEC       (1000000000ULL)   /* nanosec in a sec */
using namespace v0::com::qualcomm::qti::location;

using namespace location_client;


LocIdlAPIStubImpl::LocIdlAPIStubImpl(const LocIdlAPIService* apiService):
    mApiService(apiService){
}

LocIdlAPIStubImpl::~LocIdlAPIStubImpl() {
}


const uint32_t & LocIdlAPIStubImpl::getGnssCapabilitiesMaskAttribute (
    const std::shared_ptr<CommonAPI::ClientId> client
)
{
    return mCapsMask;

}

void LocIdlAPIStubImpl::startPositionSession
(
    const std::shared_ptr<CommonAPI::ClientId> client,
    uint32_t intervalInMs, uint32_t gnssReportCallbackMask,
    startPositionSessionReply_t reply
)
{
    std::cout << "==== startPositionSession1 _intervalInMs " << intervalInMs <<\
            "_gnssReportCallbackMask " << gnssReportCallbackMask << std::endl;
    if (mApiService) {
        mApiService->startPositionSession(client, intervalInMs, gnssReportCallbackMask, reply);
    }
}

void LocIdlAPIStubImpl::startPositionSession
(
    const std::shared_ptr<CommonAPI::ClientId> client,
    uint32_t intervalInMs, uint32_t locReqEngMask,
    uint32_t engReportCallbackMask, startPositionSession1Reply_t reply
)
{
    std::cout << "==== startPositionSession2 _intervalInMs " << intervalInMs\
            << " locReqEngMask " << locReqEngMask << std::endl;
    if (mApiService) {
        mApiService->startPositionSession(client, intervalInMs, engReportCallbackMask, reply);
    }
}

void LocIdlAPIStubImpl::stopPositionSession
(
    const std::shared_ptr<CommonAPI::ClientId> client,
    stopPositionSessionReply_t reply
)
{
    if (mApiService) {
        mApiService->stopPositionSession(client, reply);
    }
    reply();
}

void LocIdlAPIStubImpl::deleteAidingData(const std::shared_ptr<CommonAPI::ClientId> client,
    uint32_t aidingDataMask,
    deleteAidingDataReply_t reply) {
    if (mApiService) {
        mApiService->LIAdeleteAidingData(client, aidingDataMask, reply);
    }
}

/// This is the method that will be called on remote calls on the method configConstellations.
void LocIdlAPIStubImpl::configConstellations(const std::shared_ptr<CommonAPI::ClientId> client,
    std::vector< LocIdlAPI::IDLGnssSvIdInfo > svList,
    configConstellationsReply_t reply) {
    // This API is currently not supported.
    reply(LocIdlAPI::IDLLocationResponse::IDL_LOC_RESP_NOT_SUPPORTED);
}

void LocIdlAPIStubImpl::injectMapMatchedFeedbackData(
        const std::shared_ptr<CommonAPI::ClientId> client,
        LocIdlAPI::MapMatchingFeedbackData mmfData, injectMapMatchedFeedbackDataReply_t reply) {
    if (mApiService) {
        mApiService->injectMapMatchedFeedbackData(client, mmfData, reply);
    }
}
