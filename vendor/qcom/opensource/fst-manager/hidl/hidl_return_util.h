/*
 * hidl interface for fst manager
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2016, Roshan Pius <rpius@google.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HIDL_RETURN_UTIL_H_
#define HIDL_RETURN_UTIL_H_

namespace vendor {
namespace qti {
namespace hardware {
namespace fstman {
namespace V1_0 {
namespace implementation {
namespace hidl_return_util {

/**
 * These utility functions are used to invoke a method on the provided
 * HIDL interface object.
 * These functions checks if the provided HIDL interface object is valid.
 * a) if valid, Invokes the corresponding internal implementation function of
 * the HIDL method. It then invokes the HIDL continuation callback with
 * the status and any returned values.
 * b) if invalid, invokes the HIDL continuation callback with the
 * provided error status and default values.
 */
// Use for HIDL methods which return only an instance of FstManagerStatus.
template <typename ObjT, typename WorkFuncT, typename... Args>
Return<void> validateAndCall(
    ObjT* obj, FstManagerStatusCode status_code_if_invalid, WorkFuncT&& work,
    const std::function<void(const FstManagerStatus&)>& hidl_cb, Args&&... args)
{
	if (obj->isValid()) {
		hidl_cb((obj->*work)(std::forward<Args>(args)...));
	} else {
		hidl_cb({status_code_if_invalid, ""});
	}
	return Void();
}

// Use for HIDL methods which return instance of FstManagerStatus and a single
// return value.
template <typename ObjT, typename WorkFuncT, typename ReturnT, typename... Args>
Return<void> validateAndCall(
    ObjT* obj, FstManagerStatusCode status_code_if_invalid, WorkFuncT&& work,
    const std::function<void(const FstManagerStatus&, ReturnT)>& hidl_cb,
    Args&&... args)
{
	if (obj->isValid()) {
		const auto& ret_pair =
		    (obj->*work)(std::forward<Args>(args)...);
		const FstManagerStatus& status = std::get<0>(ret_pair);
		const auto& ret_value = std::get<1>(ret_pair);
		hidl_cb(status, ret_value);
	} else {
		hidl_cb(
		    {status_code_if_invalid, ""},
		    typename std::remove_reference<ReturnT>::type());
	}
	return Void();
}

// Use for HIDL methods which return instance of FstManagerStatus and 2 return
// values.
template <
    typename ObjT, typename WorkFuncT, typename ReturnT1, typename ReturnT2,
    typename... Args>
Return<void> validateAndCall(
    ObjT* obj, FstManagerStatusCode status_code_if_invalid, WorkFuncT&& work,
    const std::function<void(const FstManagerStatus&, ReturnT1, ReturnT2)>&
	hidl_cb,
    Args&&... args)
{
	if (obj->isValid()) {
		const auto& ret_tuple =
		    (obj->*work)(std::forward<Args>(args)...);
		const FstManagerStatus& status = std::get<0>(ret_tuple);
		const auto& ret_value1 = std::get<1>(ret_tuple);
		const auto& ret_value2 = std::get<2>(ret_tuple);
		hidl_cb(status, ret_value1, ret_value2);
	} else {
		hidl_cb(
		    {status_code_if_invalid, ""},
		    typename std::remove_reference<ReturnT1>::type(),
		    typename std::remove_reference<ReturnT2>::type());
	}
	return Void();
}

}  // namespace hidl_return_util
}  // namespace implementation
}  // namespace V1_0
}  // namespace fstman
}  // namespace hardware
}  // namespace qti
}  // namespace vendor
#endif  // HIDL_RETURN_UTIL_H_
