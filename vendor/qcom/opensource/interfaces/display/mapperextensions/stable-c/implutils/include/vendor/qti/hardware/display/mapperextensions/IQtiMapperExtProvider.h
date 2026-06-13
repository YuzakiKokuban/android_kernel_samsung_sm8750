/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <vendor/qti/hardware/display/mapperextensions/IQtiMapperExt.h>
#include <log/log.h>

#include <mutex>
#include <type_traits>

namespace vendor::qtimapperext {

/**
 * Extend from this interface to provide Version 2 of the IQtiMapperExt interface
 */
struct IQtiMapperExtV2Impl {
    static const auto version = IQTIMAPPEREXT_VERSION_2;
    virtual ~IQtiMapperExtV2Impl() = default;

    virtual IQtiMapperExt_Error getMultiViewInfo(buffer_handle_t _Nonnull buffer,
                                                 uint32_t* _Nonnull views) = 0;

    virtual IQtiMapperExt_Error getBaseView(buffer_handle_t _Nonnull buffer,
                                            uint32_t* _Nonnull view) = 0;

    virtual IQtiMapperExt_Error importViewBuffer(
        buffer_handle_t _Nonnull metaHandle, uint32_t view,
        buffer_handle_t _Nullable* _Nonnull outBufferHandle) = 0;
};

namespace provider {
#ifndef __cpp_inline_variables
#error "Only C++17 & newer is supported; inline variables is missing"
#endif

inline void* _Nullable sIQtiMapperExtInstance = nullptr;
}  // namespace provider

template <typename IMPL>
class IQtiMapperExtProvider {
  private:
    static_assert(IMPL::version >= IQTIMAPPEREXT_VERSION_2,
                  "Must be at least IQTIMAPPEREXT_VERSION_2");
    static_assert(std::is_final_v<IMPL>, "Implementation must be final");
    static_assert(std::is_constructible_v<IMPL>, "Implementation must have a no-args constructor");

    std::once_flag mLoadOnceFlag;
    IMPL* _Nullable mImpl;
    IQtiMapperExt* _Nullable mQtiMapperExt;

    static IMPL& impl() {
        return *reinterpret_cast<IQtiMapperExtProvider<IMPL>*>(
                provider::sIQtiMapperExtInstance)->mImpl;
    }

    void bindV2() {
        mQtiMapperExt->v2 = {
                .getMultiViewInfo = [](buffer_handle_t _Nonnull buffer,
                                       uint32_t* _Nonnull views) -> IQtiMapperExt_Error {
                    return impl().getMultiViewInfo(buffer, views);
                },

                .getBaseView = [](buffer_handle_t _Nonnull buffer,
                                  uint32_t* _Nonnull view) -> IQtiMapperExt_Error {
                    return impl().getBaseView(buffer, view);
                },

                .importViewBuffer = [](buffer_handle_t _Nonnull metaHandle, uint32_t view,
                                       buffer_handle_t _Nullable* _Nonnull outBufferHandle)
                        -> IQtiMapperExt_Error {
                    return impl().importViewBuffer(metaHandle, view, outBufferHandle);
                },
        };
    }

  public:
    explicit IQtiMapperExtProvider() = default;

    IQtiMapperExt_Error load(IQtiMapperExt* _Nullable* _Nonnull outImplementation) {
        std::call_once(mLoadOnceFlag, [this] {
            LOG_ALWAYS_FATAL_IF(provider::sIQtiMapperExtInstance != nullptr,
                                "IQtiMapperExt implementation already loaded!");
            provider::sIQtiMapperExtInstance = this;
            mImpl = new IMPL();
            mQtiMapperExt = new IQtiMapperExt();
            mQtiMapperExt->version = IMPL::version;
            if (IMPL::version >= IQTIMAPPEREXT_VERSION_2) {
                bindV2();
            }
        });
        *outImplementation = mQtiMapperExt;
        return IQTIMAPPEREXT_ERROR_NONE;
    }
};

}  // namespace vendor::qtimapperext