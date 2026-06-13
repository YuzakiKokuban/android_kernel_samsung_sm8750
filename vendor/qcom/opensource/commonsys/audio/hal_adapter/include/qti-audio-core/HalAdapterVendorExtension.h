/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/android/media/audio/BnHalAdapterVendorExtension.h>

namespace qti::audio::core {

/**
 * This is responsible for validating the parameters like key, key length validation
**/
class HalAdapterVendorExtension
    : public ::aidl::android::media::audio::BnHalAdapterVendorExtension {
   public:
    ::ndk::ScopedAStatus parseVendorParameterIds(
        ::aidl::android::media::audio::IHalAdapterVendorExtension::
            ParameterScope in_scope,
        const std::string& in_rawKeys,
        std::vector<std::string>* _aidl_return) override;
    ::ndk::ScopedAStatus parseVendorParameters(
        ::aidl::android::media::audio::IHalAdapterVendorExtension::
            ParameterScope in_scope,
        const std::string& in_rawKeysAndValues,
        std::vector<::aidl::android::hardware::audio::core::VendorParameter>*
            out_syncParameters,
        std::vector<::aidl::android::hardware::audio::core::VendorParameter>*
            out_asyncParameters) override;
    ::ndk::ScopedAStatus parseBluetoothA2dpReconfigureOffload(
        const std::string& in_rawValue,
        std::vector<::aidl::android::hardware::audio::core::VendorParameter>*
            _aidl_return) override;
    ::ndk::ScopedAStatus parseBluetoothLeReconfigureOffload(
        const std::string& in_rawValue,
        std::vector<::aidl::android::hardware::audio::core::VendorParameter>*
            _aidl_return) override;
    ::ndk::ScopedAStatus processVendorParameters(
        ::aidl::android::media::audio::IHalAdapterVendorExtension::
            ParameterScope in_scope,
        const std::vector<
            ::aidl::android::hardware::audio::core::VendorParameter>&
            in_parameters,
        std::string* _aidl_return) override;
};
}  // namespace qti::audio::core