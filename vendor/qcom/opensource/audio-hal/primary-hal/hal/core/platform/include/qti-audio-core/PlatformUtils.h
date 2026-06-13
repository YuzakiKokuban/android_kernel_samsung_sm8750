/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <PalApi.h>
#include <aidl/android/hardware/audio/core/VendorParameter.h>
#include <aidl/android/media/audio/common/AudioPlaybackRate.h>
#include <aidl/qti/audio/core/VString.h>

namespace qti::audio::core {

using ::aidl::android::media::audio::common::AudioProfile;
using ::aidl::android::media::audio::common::AudioChannelLayout;

constexpr size_t getNearestMultiple(size_t num, size_t multiplier) {
    size_t remainder = 0;

    if (!multiplier) return num;

    remainder = num % multiplier;
    if (remainder) num += (multiplier - remainder);

    return num;
}

// std::string getStringForVendorParameter(
auto getkvPairsForVendorParameter =
        [](const std::vector<::aidl::android::hardware::audio::core::VendorParameter>& param)
        -> std::string {
            std::string str = "";
            std::optional<::aidl::qti::audio::core::VString> parcel;
            for (const auto& p : param) {
                if (p.ext.getParcelable(&parcel) == STATUS_OK && parcel.has_value()) {
                    std::string keyvalue = p.id + "=" + parcel.value().value + ";";
                    str.append(keyvalue);
                }
            }
            return str;
        };

auto getBoolValueFromVString = [](
        const std::vector<::aidl::android::hardware::audio::core::VendorParameter>& parameters,
        const std::string& searchKey) -> std::optional<bool> {
    std::optional<::aidl::qti::audio::core::VString> parcel;
    for (const auto& p : parameters) {
        if (p.id == searchKey && p.ext.getParcelable(&parcel) == STATUS_OK && parcel.has_value()) {
            return parcel.value().value == "true";
        }
    }
    return std::nullopt;
};

std::vector<AudioProfile> getSupportedAudioProfiles(pal_param_device_capability_t* capability,
                                                    std::string devName);
std::vector<AudioChannelLayout> getChannelMasksFromProfile(
        pal_param_device_capability_t* capability);
std::vector<int> getSampleRatesFromProfile(pal_param_device_capability_t* capability);
AudioChannelLayout getChannelIndexMaskFromChannelCount(unsigned int channelCount);
AudioChannelLayout getChannelLayoutMaskFromChannelCount(unsigned int channelCount, int isInput);

void setPalDeviceCustomKey(pal_device& palDevice, const std::string& customKey) noexcept;

std::vector<uint8_t> makePalVolumes(std::vector<float> const& volumes) noexcept;

/*
* validates if the playback rate parameters are valid
*/
bool isValidPlaybackRate(
        const ::aidl::android::media::audio::common::AudioPlaybackRate& playbackRate);

/**
 * @brief Expects a std::unique_ptr
 * checks if unique_ptr is allocated or not
 * If memory is allocated then return unique_ptr
 * otherwise exit with retValue which caller needs to pass
*/
#define VALUE_OR_EXIT(ptr, retValue)                                 \
    ({                                                               \
        auto temp = (ptr);                                           \
        if (temp.get() == nullptr) {                                 \
            LOG(ERROR) << __func__ << " could not allocate memory "; \
            return retValue;                                         \
        }                                                            \
        std::move(temp);                                             \
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
* auto param_payload = VALUE_OR_EXIT(allocate<pal_param_payload>(sizeof(pal_param_payload)));
* allocate will allocate unique_ptr as per type pal_param_payload
* VALUE_OR_EXIT will return the unique_ptr if allocation is succesfull
* otherwise it will exit.
* custom deletor will take to deallocate memory using free when scope is cleared.
* @param size size to be allocated for type T
* @return unique_ptr of type T with size requested.
*/

using CustomDeletor = void (*)(void*);
template <typename T>
std::unique_ptr<T, CustomDeletor> allocate(int size) {
    T* obj = reinterpret_cast<T*>(calloc(1, size));
    return std::unique_ptr<T, CustomDeletor>{obj, free};
}

} // namespace qti::audio::core
