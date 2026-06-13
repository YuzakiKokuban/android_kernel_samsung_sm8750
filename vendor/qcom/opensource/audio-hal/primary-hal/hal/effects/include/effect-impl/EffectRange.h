/*
 * Copyright (C) 2023 The Android Open Source Project
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
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <algorithm>
#include <tuple>
#include <utility>
#include <vector>

namespace aidl::qti::effects {

template <typename T>
bool isInRange(const T& value, const T& low, const T& high) {
    return (value >= low) && (value <= high);
}

template <typename T, std::size_t... Is>
bool isTupleInRange(const T& test, const T& min, const T& max, std::index_sequence<Is...>) {
    return (isInRange(std::get<Is>(test), std::get<Is>(min), std::get<Is>(max)) && ...);
}

template <typename T, std::size_t TupSize = std::tuple_size_v<T>>
bool isTupleInRange(const T& test, const T& min, const T& max) {
    return isTupleInRange(test, min, max, std::make_index_sequence<TupSize>{});
}

template <typename T, typename F>
bool isTupleInRange(const std::vector<T>& cfgs, const T& min, const T& max, const F& func) {
    auto minT = func(min), maxT = func(max);
    return std::all_of(cfgs.cbegin(), cfgs.cend(),
                       [&](const T& cfg) { return isTupleInRange(func(cfg), minT, maxT); });
}

} // namespace aidl::qti::effects
