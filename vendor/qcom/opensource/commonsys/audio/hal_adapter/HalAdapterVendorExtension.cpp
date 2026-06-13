/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "QHAVExt"

#include <aidl/qti/audio/core/VString.h>
#include <android-base/logging.h>
#include <android/binder_ibinder_platform.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <qti-audio-core/HalAdapterVendorExtension.h>
#include <system/thread_defs.h>

#include <numeric>

// no headers below these

namespace qti::audio::core {

/* sfsfc;dvfrg=1;yupp => {sfsfc,dvfrg=1,yupp} */
auto getVectorFromString =
    [](const std::string& keyString) -> std::vector<std::string> {
    const auto keyLength = keyString.length();
    if (keyString.length() == 0) {
        return {};
    }
    std::vector<std::string> result;
    size_t i = 0;
    const std::string delimiter{";"};
    size_t foundPos = 0;
    while ((foundPos = keyString.find(delimiter, i)) != std::string::npos) {
        std::cout << "foundPos: " << foundPos << std::endl;
        std::string key = keyString.substr(i, foundPos - i);
        key.empty() ? (void)0 : (void)result.push_back(key);
        i = foundPos + 1;
    }
    if (i != keyLength) {
        std::string key = keyString.substr(i, keyLength);
        result.push_back(key);
    }
    return result;
};

/* {sfsfc=???,one=1,yupp=okay} => {{sfsfc,???},{one,1},{yupp,okay}} */
auto getPairsFromVector = [](const std::vector<std::string>& keyValues) {
    std::vector<std::pair<std::string, std::string>> keyValuePairs;
    for (const auto& keyValue : keyValues) {
        const std::string delimiter{"="};
        auto foundPos = keyValue.find(delimiter, 0);
        if (foundPos == std::string::npos) {
            keyValuePairs.push_back({keyValue, ""});
            continue;
        }
        std::string key = keyValue.substr(0, foundPos);
        std::string value = keyValue.substr(foundPos + 1, keyValue.size());
        keyValuePairs.push_back({key, value});
    }
    return keyValuePairs;
};

::aidl::android::hardware::audio::core::VendorParameter
getVendorParameterAsVString(const std::string& key, const std::string& value) {
    ::aidl::android::hardware::audio::core::VendorParameter param;
    ::aidl::qti::audio::core::VString parcel;
    param.id = key;
    parcel.value = value;
    if (param.ext.setParcelable(parcel) != android::OK) {
        LOG(ERROR) << __func__ << ": failed to set parcel for "
                   << parcel.descriptor;
    }
    return std::move(param);
}

std::optional<std::string> getStringForVendorParameterAsVString(
    const ::aidl::android::hardware::audio::core::VendorParameter& param) {
    std::optional<::aidl::qti::audio::core::VString> parcel;
    param.ext.getParcelable(&parcel);
    if (!parcel.has_value()) {
        LOG(ERROR) << __func__
                   << " unable to get parcelable for key:" << param.id;
        return std::nullopt;
    }
    return param.id + "=" + parcel.value().value;
}

::ndk::ScopedAStatus HalAdapterVendorExtension::parseVendorParameterIds(
    ::aidl::android::media::audio::IHalAdapterVendorExtension::ParameterScope
        in_scope,
    const std::string& in_rawKeys, std::vector<std::string>* _aidl_return) {
    *_aidl_return = getVectorFromString(in_rawKeys);

    LOG(VERBOSE) << __func__
              << ": scope:" << ::aidl::android::media::audio::toString(in_scope)
              << ", raw keys:" << in_rawKeys << " ==> VendorParameter Ids:"
              << std::accumulate(_aidl_return->cbegin(), _aidl_return->cend(),
                                 std::string(""), [](auto prev, auto ele) {
                                     return std::move(prev) + " " + ele;
                                 });

    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus HalAdapterVendorExtension::parseVendorParameters(
    ::aidl::android::media::audio::IHalAdapterVendorExtension::ParameterScope
        in_scope,
    const std::string& in_rawKeysAndValues,
    std::vector<::aidl::android::hardware::audio::core::VendorParameter>*
        out_syncParameters,
    std::vector<::aidl::android::hardware::audio::core::VendorParameter>*
        out_asyncParameters) {
    const auto& keyValues = getVectorFromString(in_rawKeysAndValues);

    const auto& keyValuesPairs = getPairsFromVector(keyValues);

    std::vector<::aidl::android::hardware::audio::core::VendorParameter> result;

    for (const auto& [key, value] : keyValuesPairs) {
        result.push_back(getVendorParameterAsVString(key, value));
    }

    *out_syncParameters = result;
    *out_asyncParameters = {};

    LOG(VERBOSE)
        << __func__
        << ": scope:" << ::aidl::android::media::audio::toString(in_scope)
        << ", raw keyandvalues:" << in_rawKeysAndValues
        << ", VendorParameters Ids size:" << out_syncParameters->size()
        << " ==> "
        << std::accumulate(
               out_syncParameters->cbegin(), out_syncParameters->cend(),
               std::string(""), [](auto prev, auto ele) {
                   std::optional<::aidl::qti::audio::core::VString> parcel;
                   ele.ext.getParcelable(&parcel);
                   return std::move(prev) + " {" + ele.id + "," +
                          (parcel ? parcel.value().value : " not parcelled ") +
                          "}";
               });

    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus
HalAdapterVendorExtension::parseBluetoothA2dpReconfigureOffload(
    const std::string& in_rawValue,
    std::vector<::aidl::android::hardware::audio::core::VendorParameter>*
        _aidl_return) {
    LOG(ERROR) << __func__ << "not implemented";
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus
HalAdapterVendorExtension::parseBluetoothLeReconfigureOffload(
    const std::string& in_rawValue,
    std::vector<::aidl::android::hardware::audio::core::VendorParameter>*
        _aidl_return) {
    LOG(ERROR) << __func__ << "not implemented";
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus HalAdapterVendorExtension::processVendorParameters(
    ::aidl::android::media::audio::IHalAdapterVendorExtension::ParameterScope
        in_scope,
    const std::vector<::aidl::android::hardware::audio::core::VendorParameter>&
        in_parameters,
    std::string* _aidl_return) {
    std::string keyValuesString{""};
    for (const auto& param : in_parameters) {
        const auto& out = getStringForVendorParameterAsVString(param);
        if (!out.has_value()) {
            continue;
        }
        if (keyValuesString == "") {
            keyValuesString = out.value();
        } else {
            keyValuesString.append(";").append(out.value());
        }
    }
    *_aidl_return = keyValuesString;
    LOG(VERBOSE) << __func__
              << ": scope:" << ::aidl::android::media::audio::toString(in_scope)
              << ", parameters size: " << in_parameters.size()
              << ", VendorParameters Ids:"
              << std::accumulate(in_parameters.cbegin(), in_parameters.cend(),
                                 std::string(""),
                                 [](auto prev, auto ele) {
                                     return std::move(prev) + " " + ele.id;
                                 })
              << " ==> raw key and values: " << *_aidl_return;
    return ndk::ScopedAStatus::ok();
}

}  // namespace qti::audio::core

/**
 * in case, if someone want to dlopen this library and
 * register this as AIDL service.
 *
 * Also, as per the interface expectation,
 * one needs to set the system property
 * 'ro.audio.ihaladaptervendorextension_enabled' to be true
 *
 **/
static std::shared_ptr<::qti::audio::core::HalAdapterVendorExtension>
    gHalAdapterVendorExtension;

extern "C" __attribute__((visibility("default"))) void registerInterface() {
    gHalAdapterVendorExtension = ndk::SharedRefBase::make<
        ::qti::audio::core::HalAdapterVendorExtension>();
    const auto kServiceName =
        std::string(gHalAdapterVendorExtension->descriptor)
            .append("/")
            .append("default");
    AIBinder_setMinSchedulerPolicy(gHalAdapterVendorExtension->asBinder().get(),
                                   SCHED_NORMAL, ANDROID_PRIORITY_AUDIO);
    binder_exception_t status = AServiceManager_addService(
        gHalAdapterVendorExtension->asBinder().get(), kServiceName.c_str());
    if (status != EX_NONE) {
        LOG(ERROR) << __func__ << " failed to register " << kServiceName
                  << " ret:" << status;
    }
}