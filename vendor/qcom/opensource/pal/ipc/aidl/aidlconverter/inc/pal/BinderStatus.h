/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <PalDefs.h>
#include <aidl/vendor/qti/hardware/pal/PalCallbackBuffer.h>
#include <aidl/vendor/qti/hardware/pal/Status.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>

using aidl::vendor::qti::hardware::pal::Status;
using aidl::vendor::qti::hardware::pal::PalCallbackBuffer;

inline uint64_t convertAidlHandleToLegacy(int64_t aidlHandle) {
    return (static_cast<uint64_t>(aidlHandle));
}

inline int64_t convertLegacyHandleToAidlHandle(pal_stream_handle_t* handle) {
    return (int64_t)handle;
}

/**
* @brief convertStatus_tToExceptionCode converts status_t based errorcode
* to Status (AIDL) type. This is used to preserve errors code across the
* AIDL layers. These Status types are used to throw AStatus_fromServiceSpecificErrorWithMessage
* @param errcode  status_t error code.
* @return Status Aidl object corresponding to legacy error code.
*/
static Status convertStatus_tToExceptionCode(int errcode) {
    switch (errcode) {
        case -EIO:
            return Status::IO_ERROR;
        case -EBUSY:
            return Status::BUSY;
        case -ENOSPC:
            return Status::NO_SPACE;
        case -EBADR:
            return Status::INVALID_FD;
        case -EADV:
            return Status::ADVERTISE_ERROR;
        case -ENOPROTOOPT:
            return Status::PROTOCOL_NOT_AVAILABLE;
        case -EOPNOTSUPP:
            return Status::NOT_SUPPORTED;
        case -ENETRESET:
            return Status::DOWN_WITH_SSR;
        case -EALREADY:
            return Status::NOW_INPROGRESS;
        case -EINPROGRESS:
            return Status::ALREADY_INPROGRESS;
        case -ECANCELED:
            return Status::CANCELLED;
        case -ENOTRECOVERABLE:
            return Status::NOT_RECOVERABLE;
        default:
            return Status::UNKNOWN;
    }
}

/**
* @brief statusTFromExceptionCode converts Status(AIDL) based errorcode
* to legacy type. This is used to preserve errors code across the
* AIDL layers. This unmarshells the errorcode from exception received over
* AIDL, used along with statusTFromBinderStatus.
* @param exceptionCode  exception in Status format.
* @return legacy error code
*/
static int statusTFromExceptionCode(Status exceptionCode) {
    switch (exceptionCode) {
        case Status::SUCCESS:
            return ::android::OK;
        case Status::IO_ERROR:
            return -EIO;
        case Status::BUSY:
            return -EBUSY;
        case Status::NO_SPACE:
            return -ENOSPC;
        case Status::INVALID_FD:
            return -EBADR;
        case Status::ADVERTISE_ERROR:
            return -EADV;
        case Status::PROTOCOL_NOT_AVAILABLE:
            return -ENOPROTOOPT;
        case Status::NOT_SUPPORTED:
            return -EOPNOTSUPP;
        case Status::DOWN_WITH_SSR:
            return -ENETRESET;
        case Status::NOW_INPROGRESS:
            return -EALREADY;
        case Status::ALREADY_INPROGRESS:
            return -EINPROGRESS;
        case Status::CANCELLED:
            return -ECANCELED;
        case Status::NOT_RECOVERABLE:
            return -ENOTRECOVERABLE;
        case Status::UNKNOWN:
        default:
            return -EINVAL;
    }
}

/**
* @brief status_tToBinderResult converts legacy status_t codes to ScopedAStatus.
* The known errors are mapped from binder_status.h and directly propagated,
* rest of errors used by PAL are converted using ServiceSpecificException.
* @param errcode linux based error code.
* @return returns ScopedAStatus based on error code.
*/

inline ::ndk::ScopedAStatus status_tToBinderResult(int errcode) {
    Status errStatus = convertStatus_tToExceptionCode(errcode);
    if (errStatus != Status::UNKNOWN) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(errStatus), toString(errStatus).c_str()));
    }
    return ndk::ScopedAStatus(AStatus_fromStatus(static_cast<int32_t>(errcode)));
}

/**
* @brief statusTFromBinderStatus converts ScopedAStatus to legacy status_t codes
* The known errors are mapped from binder_status.h and directly propagated,
* rest of errors used by PAL are converted using ServiceSpecificException.
* converts ScopedAStatus Exception code into error using the
* helper method statusTFromExceptionCode
* @param status  ScopedAStatus code
* @param caller to print caller function, helpful during debugging.
* @return returns converted status_t code.
*/
static inline int statusTFromBinderStatus(const ::ndk::ScopedAStatus& status,
                                          const std::string& caller = "") {
    if (status.isOk()) {
        return ::android::OK;
    } else if (status.getServiceSpecificError()) {
        ALOGV("%s failed with %s", caller.c_str(), status.getDescription().c_str());
        return statusTFromExceptionCode(static_cast<Status>(status.getServiceSpecificError()));
    } else {
        ALOGV("%s failed with %s", caller.c_str(), status.getDescription().c_str());
        return status.getStatus();
    }
}

static void checkAndUpdateMDStatus(pal_event_read_write_done_payload* rw_done_payload,
                                   PalCallbackBuffer* rwDonePayload) {
    switch (rw_done_payload->md_status) {
        case ENOTRECOVERABLE: {
            ALOGE("%s: Error, md cannot be parsed in buffer", __func__);
            rwDonePayload->status = rw_done_payload->md_status;
            break;
        }
        case EOPNOTSUPP: {
            ALOGE("%s: Error, md id not recognized in buffer", __func__);
            rwDonePayload->status = rw_done_payload->md_status;
            break;
        }
        case ENOMEM: {
            ALOGE("%s: Error, md buffer size received is small", __func__);
            rwDonePayload->status = rw_done_payload->md_status;
            break;
        }
        default: {
            if (rw_done_payload->md_status) {
                ALOGE("%s: Error received during callback, md status = 0x%x", __func__,
                      rw_done_payload->md_status);
                rwDonePayload->status = rw_done_payload->md_status;
            }
            break;
        }
    }
}
