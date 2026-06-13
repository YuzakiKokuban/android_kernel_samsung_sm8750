/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <agm/agm_api.h>
#include "BinderStatus.h"

#define MAX_KVPAIR 48

/**
 * @brief Exit if number of kvpais exceeds max defined pairs
*/
#define RETURN_IF_KVPAIR_EXCEEDS_RANGE(pair)                              \
    ({                                                                    \
        if (pair > MAX_KVPAIR) {                                          \
            ALOGE("Num KVs %d more than expected: %d", pair, MAX_KVPAIR); \
            return status_tToBinderResult(-ENOMEM);                       \
        }                                                                 \
    })

/*
* @brief Pass a unique_ptr to check if memory is allocated or not.
*/
#define RETURN_IF_ALLOCATION_FAILED(ptr)                     \
    ({                                                       \
        if (ptr.get() == NULL) {                             \
            ALOGE("%s could not allocate memory", __func__); \
            return status_tToBinderResult(-ENOMEM);          \
        }                                                    \
    })

/**
 * @brief Expects a std::unique_ptr
 * checks if unique_ptr is allocated or not
 * If memory is allocated then return unique_ptr
 * otherwise exit with -ENOMEM status.
*/

#define VALUE_OR_RETURN(ptr)                                 \
    ({                                                       \
        auto temp = (ptr);                                   \
        if (temp.get() == nullptr) {                         \
            ALOGE("%s could not allocate memory", __func__); \
            return status_tToBinderResult(-ENOMEM);          \
        }                                                    \
        std::move(temp);                                     \
    })

/**
* @brief allocator with custom deleter
* Takes a type T and size
* return the unique_ptr for type allocated with calloc
* When goes out of scope will be deallocated with free
* client needs to check if returned ptr is null or not.
* Usage:
* with calloc and free:
*     struct agm_media_config *config = (struct agm_media_config*)calloc(1,
*                                             sizeof(struct agm_media_config));
*     if (config == NULL) {
*         ALOGE("%s: Cannot allocate memory for config\n", __func__);
*         return -ENOMEM;
*     }
*    ....
*    free(config);
* Now:
* auto config = VALUE_OR_RETURN(allocate<agm_media_config>(sizeof(agm_media_config)));
* allocate will allocate unique_ptr as per type agm_media_config
* VALUE_OR_RETRUN will return the unique_ptr if allocation is succesfull
* otherwise it will exit.
* custom deletor will take to deallocate memory using free when scope is cleared.
* @param size size to be allocated for type T
* @return unique_ptr of type T with size requested.
*/

using CustomDeletor = void (*)(void *);
template <typename T>
std::unique_ptr<T, CustomDeletor> allocate(int size) {
    T *obj = reinterpret_cast<T *>(calloc(1, size));
    return std::unique_ptr<T, CustomDeletor>{obj, free};
}

using AgmAifUniquePtrType = std::unique_ptr<struct aif_info, CustomDeletor>;

/**
* @brief needsCodecSpecificInfo returns true for compress codecs which
* need extra codec specific information to be passed.
* @param format agm_media_format type
* @return whether needs codec specific information or not
*/
static bool needsCodecSpecificInfo(agm_media_format format) {
    switch (format) {
        case AGM_FORMAT_AAC:
        case AGM_FORMAT_FLAC:
        case AGM_FORMAT_ALAC:
        case AGM_FORMAT_APE:
        case AGM_FORMAT_WMAPRO:
        case AGM_FORMAT_WMASTD:
        case AGM_FORMAT_OPUS:
            return true;
        default:
            return false;
    }
}