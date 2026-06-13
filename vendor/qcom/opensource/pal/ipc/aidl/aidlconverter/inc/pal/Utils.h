/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include "BinderStatus.h"

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
*     struct pal_param_payload *param_payload = (struct pal_param_payload*)calloc(1,
*                                             sizeof(struct pal_param_payload));
*     if (param_payload == NULL) {
*         ALOGE("%s: Cannot allocate memory for param_payload\n", __func__);
*         return -ENOMEM;
*     }
*    ....
*    free(param_payload);
* Now:
* auto param_payload = VALUE_OR_RETURN(allocate<pal_param_payload>(sizeof(pal_param_payload)));
* allocate will allocate unique_ptr as per type pal_param_payload
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

using PalDevUniquePtrType = std::unique_ptr<struct pal_device, CustomDeletor>;
using PalModifierUniquePtrType = std::unique_ptr<struct modifier_kv, CustomDeletor>;
