/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/android/hardware/audio/core/VendorParameter.h>
#include <aidl/android/media/audio/common/AudioDevice.h>
#include <aidl/android/media/audio/common/AudioPortConfig.h>
#include <aidl/qti/audio/core/VString.h>
#include <system/audio.h>
#include <algorithm>
#include <map>
#include <numeric>
#include <set>
#include <vector>

namespace qti::audio::core {

/*
* Helper class used by streams when the target audio format differs
* from input audio format. This can happen if underlying layers don't support certain formats.
* In that case, convert the unsupported format to supported format
* using a aux buffer.
*/
class BufferFormatConverter {
  public:
    BufferFormatConverter(audio_format_t inFormat, audio_format_t outFormat, size_t bufSize);
    ~BufferFormatConverter() = default;

    /*
    * @brief converts the input buffer from input format to output format.
    * @param buffer, buffer to be converted.
    * @param bytes total number of bytes in the buffer.
    * @returns pointer to converted buffer and size of converted buffer in case of success.
    * nullopt in case when bytes exceed the allocated size during setup.
    */
    std::optional<std::pair<uint8_t*, size_t>> convert(const void* buffer, size_t bytes);
    size_t getInputBytesPerSample() { return mInBytesPerSample; }
    size_t getOutputBytesPerSample() { return mOutBytesPerSample; }

  private:
    audio_format_t mInFormat = AUDIO_FORMAT_PCM_16_BIT;
    audio_format_t mOutFormat = AUDIO_FORMAT_PCM_16_BIT;
    std::unique_ptr<uint8_t[]> mBuffer{nullptr};
    size_t mAllocSize;
    size_t mOutBytesPerSample;
    size_t mInBytesPerSample;

    // Disallow copy and move assignments / constructors.
    BufferFormatConverter(const BufferFormatConverter&) = delete;
    BufferFormatConverter& operator=(const BufferFormatConverter&) = delete;
    BufferFormatConverter& operator=(BufferFormatConverter&& other) = delete;
    BufferFormatConverter(BufferFormatConverter&& other) = delete;
};

bool isMixPortConfig(const ::aidl::android::media::audio::common::AudioPortConfig&) noexcept;

bool isInputMixPortConfig(const ::aidl::android::media::audio::common::AudioPortConfig&) noexcept;

bool isDevicePortConfig(const ::aidl::android::media::audio::common::AudioPortConfig&) noexcept;

bool isOutputAudioDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;

bool isTelephonyRXDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;

bool isTelephonyTXDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;

bool isBluetoothSCODevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;

bool isBluetoothLEDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;

bool isBluetoothLETXDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;

bool isBluetoothDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;

bool hasBluetoothDevice(const std::vector<::aidl::android::media::audio::common::AudioDevice>&) noexcept;

bool hasBluetoothSCODevice(const std::vector<::aidl::android::media::audio::common::AudioDevice>&) noexcept;

bool isBluetoothA2dpDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;

bool isBluetoothA2dpTXDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;

bool hasBluetoothLEDevice(const std::vector<::aidl::android::media::audio::common::AudioDevice>&) noexcept;

bool hasBluetoothA2dpDevice(const std::vector<::aidl::android::media::audio::common::AudioDevice>&) noexcept;

bool hasInputMMapFlag(const ::aidl::android::media::audio::common::AudioIoFlags&) noexcept;

bool hasOutputMMapFlag(const ::aidl::android::media::audio::common::AudioIoFlags&) noexcept;

bool hasMMapFlagsEnabled(const ::aidl::android::media::audio::common::AudioIoFlags&) noexcept;

bool isInputAFEProxyDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;

bool isIPDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;
bool isIPInDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;
bool isIPOutDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;

bool isOutputSpeakerEarpiece(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;
bool hasOutputSpeakerEarpiece(
        const std::vector<::aidl::android::media::audio::common::AudioDevice>&) noexcept;

bool isHdmiDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;
bool isUsbDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;
bool isValidAlsaAddr(const std::vector<int>& alsaAddress) noexcept;
bool isInputDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;
bool isOutputDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;

bool hasOutputDirectFlag(const ::aidl::android::media::audio::common::AudioIoFlags&) noexcept;

bool hasOutputRawFlag(const ::aidl::android::media::audio::common::AudioIoFlags&) noexcept;
bool hasInputRawFlag(const ::aidl::android::media::audio::common::AudioIoFlags&) noexcept;

bool hasOutputVoipRxFlag(const ::aidl::android::media::audio::common::AudioIoFlags&) noexcept;
bool hasOutputDeepBufferFlag(const ::aidl::android::media::audio::common::AudioIoFlags&) noexcept;

bool hasOutputCompressOffloadFlag(
        const ::aidl::android::media::audio::common::AudioIoFlags&) noexcept;

#ifdef SEC_AUDIO_SAMSUNGRECORD
bool hasInputRemoteMicFlag(const ::aidl::android::media::audio::common::AudioIoFlags&) noexcept;
bool hasInputDirectFlag(const ::aidl::android::media::audio::common::AudioIoFlags&) noexcept;
bool hasInputFastFlag(const ::aidl::android::media::audio::common::AudioIoFlags&) noexcept;
#endif
#ifdef SEC_AUDIO_CALL
bool hasOutputPrimaryFlag(const ::aidl::android::media::audio::common::AudioIoFlags&) noexcept;
#endif
#ifdef SEC_AUDIO_SUPPORT_LOWLATENCY_MEDIA
bool hasOutputFastMediaFlag(const ::aidl::android::media::audio::common::AudioIoFlags&) noexcept;
#endif
#ifdef SEC_AUDIO_COMMON
bool isNoneDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;
bool hasNoneDevice(const std::vector<::aidl::android::media::audio::common::AudioDevice>&) noexcept;
bool isSpeakerDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;
bool hasSpeakerDevice(const std::vector<::aidl::android::media::audio::common::AudioDevice>&) noexcept;
bool isUsbHeadsetDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;
bool hasUsbHeadsetDevice(const std::vector<::aidl::android::media::audio::common::AudioDevice>&) noexcept;
bool isHdmiOutputDevice(const ::aidl::android::media::audio::common::AudioDevice&) noexcept;
bool hasHdmiOutputDevice(const std::vector<::aidl::android::media::audio::common::AudioDevice>&) noexcept;
#endif

std::optional<aidl::android::media::audio::common::AudioSource> getAudioSource(
        const ::aidl::android::media::audio::common::AudioPortConfig&) noexcept;

std::optional<int32_t> getSampleRate(
        const ::aidl::android::media::audio::common::AudioPortConfig&) noexcept;

std::vector<int32_t> getActiveInputMixPortConfigIds(
        const std::vector<::aidl::android::media::audio::common::AudioPortConfig>&
                activePortConfigs);

template <class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& list) noexcept {
    os << std::accumulate(list.cbegin(), list.cend(), std::string(""),
                          [](auto&& prev, const auto& l) {
                              return std::move(prev.append(",").append(l.toString()));
                          });
    return os;
}

template <class T>
bool operator==(const std::vector<T>& left, const std::vector<T>& right) noexcept {
    if (left.size() != right.size()) {
        return false;
    }
    return std::equal(left.cbegin(), left.cend(), right.cbegin());
}

int64_t getInt64FromString(const std::string& s) noexcept;

float getFloatFromString(const std::string& s) noexcept;

bool getBoolFromString(const std::string& s) noexcept;

bool setParameter(const ::aidl::qti::audio::core::VString& parcel,
                  ::aidl::android::hardware::audio::core::VendorParameter& parameter) noexcept;

template <typename W>
bool extractParameter(const ::aidl::android::hardware::audio::core::VendorParameter& p,
                      decltype(W::value)* v) {
    std::optional<W> value;
    int32_t result = p.ext.getParcelable(&value);
    if (result == 0 && value.has_value()) {
        *v = value.value().value;
        return true;
    }
    return false;
}

// Return whether all the elements in the vector are unique.
template <typename T>
bool all_unique(const std::vector<T>& v) {
    return std::set<T>(v.begin(), v.end()).size() == v.size();
}

// Erase all the specified elements from a map.
template <typename C, typename V>
auto erase_all(C& c, const V& keys) {
    auto oldSize = c.size();
    for (auto& k : keys) {
        c.erase(k);
    }
    return oldSize - c.size();
}

// Erase all the elements in the container that satisfy the provided predicate.
template <typename C, typename P>
auto erase_if(C& c, P pred) {
    auto oldSize = c.size();
    for (auto it = c.begin(); it != c.end();) {
        if (pred(*it)) {
            it = c.erase(it);
        } else {
            ++it;
        }
    }
    return oldSize - c.size();
}

// Erase all the elements in the map that have specified values.
template <typename C, typename V>
auto erase_all_values(C& c, const V& values) {
    return erase_if(c, [values](const auto& pair) { return values.count(pair.second) != 0; });
}

// Return non-zero count of elements for any of the provided keys.
template <typename M, typename V>
size_t count_any(const M& m, const V& keys) {
    for (auto& k : keys) {
        if (size_t c = m.count(k); c != 0) return c;
    }
    return 0;
}

// Assuming that M is a map whose values have an 'id' field,
// find an element with the specified id.
template <typename M>
auto findById(M& m, int32_t id) {
    return std::find_if(m.begin(), m.end(), [&](const auto& p) { return p.second.id == id; });
}

// Assuming that the vector contains elements with an 'id' field,
// find an element with the specified id.
template <typename T>
auto findById(std::vector<T>& v, int32_t id) {
    return std::find_if(v.begin(), v.end(), [&](const auto& e) { return e.id == id; });
}

// Return elements from the vector that have specified ids, also
// optionally return which ids were not found.
template <typename T>
std::vector<T*> selectByIds(std::vector<T>& v, const std::vector<int32_t>& ids,
                            std::vector<int32_t>* missingIds = nullptr) {
    std::vector<T*> result;
    std::set<int32_t> idsSet(ids.begin(), ids.end());
    for (size_t i = 0; i < v.size(); ++i) {
        T& e = v[i];
        if (idsSet.count(e.id) != 0) {
            result.push_back(&v[i]);
            idsSet.erase(e.id);
        }
    }
    if (missingIds) {
        *missingIds = std::vector(idsSet.begin(), idsSet.end());
    }
    return result;
}

// Assuming that M is a map whose keys' type is K and values' type is V,
// return the corresponding value of the given key from the map or default
// value if the key is not found.
template <typename M, typename K, typename V>
auto findValueOrDefault(const M& m, const K& key, V defaultValue) {
    auto it = m.find(key);
    return it == m.end() ? defaultValue : it->second;
}

// Assuming that M is a map whose keys' type is K, return the given key if it
// is found from the map or default value.
template <typename M, typename K>
auto findKeyOrDefault(const M& m, const K& key, K defaultValue) {
    auto it = m.find(key);
    return it == m.end() ? defaultValue : key;
}

/*
* create a VendorParameter from a id and value, primarly used with getVendorParameters.
*/
::aidl::android::hardware::audio::core::VendorParameter makeVendorParameter(const std::string& id,
                                                                            const std::string& value);

/*
* convert bool value to the corresponding string value
* true -> "true"
* false -> "false"
*/
std::string makeParamValue(bool const&) noexcept;

} // namespace qti::audio::core
