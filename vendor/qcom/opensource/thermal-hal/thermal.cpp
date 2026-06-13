/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *	* Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 *	* Redistributions in binary form must reproduce the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer in the documentation and/or other materials provided
 *	  with the distribution.
 *	* Neither the name of The Linux Foundation nor the names of its
 *	  contributors may be used to endorse or promote products derived
 *	  from this software without specific prior written permission.
 *
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

/* Changes from Qualcomm Innovation Center are provided under the following license:

Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <cerrno>
#include <mutex>
#include <string>

#include <android-base/file.h>
#include <android-base/logging.h>

#include "thermal.h"

namespace aidl::android::hardware::thermal{

using ndk::ScopedAStatus;

namespace {

bool interfacesEqual(const std::shared_ptr<::ndk::ICInterface>& left,
				const std::shared_ptr<::ndk::ICInterface>& right) {
	if (left == nullptr || right == nullptr || !left->isRemote() || !right->isRemote()) {
		return left == right;
	}
    return left->asBinder() == right->asBinder();
}

}// namespace

static const Temperature dummy_temp_1_0 = {
	.type = TemperatureType::SKIN,
	.name = "test sensor",
	.value = 30,
	.throttlingStatus = ThrottlingSeverity::NONE,
};

Thermal::Thermal():
	utils(std::bind(&Thermal::sendThrottlingChangeCB, this,
				std::placeholders::_1),
		std::bind(&Thermal::sendCoolingDeviceChangeCB, this,
				std::placeholders::_1))
{ }

ScopedAStatus Thermal::getCoolingDevices(std::vector<CoolingDevice>* out_data) {

	std::vector<CoolingDevice> cdev;

	if (!utils.isCdevInitialized())
		return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
						"ThermalHAL not initialized properly.");
	else {
		if (utils.readCdevStates(cdev) <= 0)
			LOG(VERBOSE) << __func__ << "Failed to read thermal cooling devices.";
	}

	if (out_data != nullptr)
		*out_data = std::move(cdev);

	return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getCoolingDevicesWithType(CoolingType in_type,
				std::vector<CoolingDevice>* out_data) {

	std::vector<CoolingDevice> cdev;

	if (!utils.isCdevInitialized())
		return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
						"ThermalHAL not initialized properly.");
	else {
		if (utils.readCdevStates(in_type, cdev) <= 0)
			LOG(VERBOSE) << __func__ << "Failed to read thermal cooling devices.";
	}

	if (out_data != nullptr)
		*out_data = std::move(cdev);

	return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getTemperatures(std::vector<Temperature>* out_temp) {
	LOG(VERBOSE) << __func__;
	std::vector<Temperature> temperatures;

	if (!utils.isSensorInitialized()) {
		std::vector<Temperature> _temp = {dummy_temp_1_0};
		LOG(VERBOSE) << __func__ << " Returning Dummy Value";

		if (out_temp != nullptr)
			*out_temp = std::move(_temp);

		return ScopedAStatus::ok();
	}

	if (utils.readTemperatures(temperatures) <= 0)
		LOG(VERBOSE) << __func__ << "Sensor Temperature read failure.";

	if (out_temp != nullptr)
		*out_temp = std::move(temperatures);

	return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getTemperaturesWithType(TemperatureType in_type,
					std::vector<Temperature>* out_temp) {

	std::vector<Temperature> temperatures;

	if (!utils.isSensorInitialized())
		return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
					"ThermalHAL not initialized properly.");
	else {
		if (utils.readTemperatures(in_type, temperatures) <= 0)
			LOG(VERBOSE) << __func__ << "Sensor Temperature read failure.";
	}

	if (out_temp != nullptr)
		*out_temp = std::move(temperatures);

	return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getTemperatureThresholds(std::vector<TemperatureThreshold>* out_temp_thresh) {

	std::vector<TemperatureThreshold> thresh;

	if (!utils.isSensorInitialized())
		return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
					"ThermalHAL not initialized properly.");

	if (utils.readTemperatureThreshold(thresh) <= 0)
		LOG(VERBOSE) << __func__ << "Sensor Threshold read failure or type not supported.";

	if (out_temp_thresh != nullptr)
		*out_temp_thresh = std::move(thresh);

	return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getTemperatureThresholdsWithType(
		TemperatureType in_type,
		std::vector<TemperatureThreshold>* out_temp_thresh) {

	std::vector<TemperatureThreshold> thresh;

	if (!utils.isSensorInitialized())
		return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
					"ThermalHAL not initialized properly.");
	else{
		if (utils.readTemperatureThreshold(in_type, thresh) <= 0)
			LOG(VERBOSE) << __func__ << "Sensor Threshold read failure or type not supported.";
	}

	if (out_temp_thresh != nullptr)
		*out_temp_thresh = std::move(thresh);

	return ScopedAStatus::ok();
}

ScopedAStatus Thermal::registerThermalChangedCallback(
		const std::shared_ptr<IThermalChangedCallback>& in_callback) {

	if (in_callback == nullptr) {
		return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
				"Invalid nullptr callback");
	}

	{
		std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
		TemperatureType in_type = TemperatureType::UNKNOWN;
		for (CallbackSetting _cb: cb) {
			if (interfacesEqual(_cb.callback, in_callback))
				return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
								"Callback already registered");
		}
		cb.emplace_back(in_callback, in_type);
		LOG(DEBUG) << "A callback has been registered to ThermalHAL ";
	}
	return ScopedAStatus::ok();
}

ScopedAStatus Thermal::registerThermalChangedCallbackWithType(
		const std::shared_ptr<IThermalChangedCallback>& in_callback, TemperatureType in_type) {
	LOG(VERBOSE) << __func__ << " IThermalChangedCallback: " << in_callback
			<< ", TemperatureType: " << static_cast<int32_t>(in_type);
	if (in_callback == nullptr) {
		return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
						"Invalid nullptr callback");
	}
	if (in_type == TemperatureType::BCL_VOLTAGE ||
		in_type == TemperatureType::BCL_CURRENT)
		return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
					"BCL current and voltage notification not supported");
	{
		std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
		for (CallbackSetting _cb: cb) {
			if (interfacesEqual(_cb.callback, in_callback))
				return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
								"Callback already registered");
		}
		cb.emplace_back(in_callback, in_type);
		LOG(DEBUG) << "A callback has been registered to ThermalHAL Type: " << android::hardware::thermal::toString(in_type);
	}
	return ScopedAStatus::ok();
}

ScopedAStatus Thermal::unregisterThermalChangedCallback(
		const std::shared_ptr<IThermalChangedCallback>& in_callback) {

	if (in_callback == nullptr) {
		return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
						"Invalid nullptr callback");
	}

	std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
	std::vector<CallbackSetting>::iterator it;
	bool removed = false;
	for (it = cb.begin(); it != cb.end(); it++) {
		if (interfacesEqual(it->callback, in_callback)) {
			cb.erase(it);
			LOG(DEBUG) << "callback unregistered. isFilter: ";
			removed = true;
			break;
		}
	}
	if (!removed) {
		return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
						"Callback wasn't registered");
	}
	LOG(DEBUG) << "A callback has been registered to ThermalHAL" ;

	return ScopedAStatus::ok();
}

ScopedAStatus Thermal::registerCoolingDeviceChangedCallbackWithType(
			const std::shared_ptr<ICoolingDeviceChangedCallback> &in_callback, CoolingType in_type)
{
	if (in_callback == nullptr)
		return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
						"Invalid nullptr callback");

	std::lock_guard<std::mutex> _lock(cdev_callback_mutex_);
	for (CdevCallbackSetting _cb: cdev_cb) {
		if (interfacesEqual(_cb.callback, in_callback))
			return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
					"Callback already registered");
	}
	cdev_cb.emplace_back(CdevCallbackSetting(in_callback, in_type));
	LOG(DEBUG) << "CoolingDevice changed callback has been registered. Type: "<< android::hardware::thermal::toString(in_type);

	return ScopedAStatus::ok();
}

ScopedAStatus Thermal::unregisterCoolingDeviceChangedCallback(
		const std::shared_ptr<ICoolingDeviceChangedCallback> &in_callback) {
	if (in_callback == nullptr)
		return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
						"Invalid nullptr callback");

	std::lock_guard<std::mutex> _lock(cdev_callback_mutex_);
	std::vector<CdevCallbackSetting>::iterator it;
	bool removed = false;
	for (auto it = cdev_cb.begin(); it != cdev_cb.end(); it++) {
		if (interfacesEqual(it->callback, in_callback)) {
			cdev_cb.erase(it);
			removed = true;
			break;
		}
	}
	if (!removed) {
		return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
					"CoolingDevice changed callback wasn't registered");
	}
	LOG(DEBUG) << "A CoolingDevice changed callback has been unregistered to CDEV" ;

	return ScopedAStatus::ok();
}

void Thermal::sendCoolingDeviceChangeCB(const CoolingDevice &c)
{
	std::lock_guard<std::mutex> _lock(cdev_callback_mutex_);
	std::vector<CdevCallbackSetting>::iterator it;

	LOG(DEBUG) << "Cooling device Name: " << c.name <<
	" type: " << (int)c.type << " cur_level: " << c.value;
	it = cdev_cb.begin();
	while (it != cdev_cb.end()) {
		if (it->type == c.type) {
			::ndk::ScopedAStatus ret = it->callback->notifyCoolingDeviceChanged(c);
				if (!ret.isOk()) {
					LOG(ERROR) << "Notify CoolingDevice changed callback execution error. Removing"
								<<ret.getMessage();
					it = cdev_cb.erase(it);
					continue;
				}
		}
		it++;
	}
}

void Thermal::sendThrottlingChangeCB(const Temperature &t)
{
	std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
	std::vector<CallbackSetting>::iterator it;

	LOG(DEBUG) << "Throttle Severity change: " << " Type: " << (int)t.type
		<< " Name: " << t.name << " Value: " << t.value <<
		" ThrottlingStatus: " << (int)t.throttlingStatus;
	it = cb.begin();
	while (it != cb.end()) {
		if (it->type == t.type || it->type == TemperatureType::UNKNOWN) {
			::ndk::ScopedAStatus ret = it->callback->notifyThrottling(t);
			if (!ret.isOk()) {
				LOG(ERROR) << "Notify callback execution error. Removing"<<ret.getMessage();
				it = cb.erase(it);
				continue;
			}
		}
		it++;
	}
}


}  // namespace aidl::android::hardware::thermal
