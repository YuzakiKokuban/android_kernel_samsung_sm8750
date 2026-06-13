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

#ifndef ANDROID_QTI_VENDOR_THERMAL_H
#define ANDROID_QTI_VENDOR_THERMAL_H

#include <mutex>
#include <thread>
#include <set>

#include <aidl/android/hardware/thermal/BnThermal.h>
#include <aidl/android/hardware/thermal/IThermalChangedCallback.h>
#include <aidl/android/hardware/thermal/ICoolingDeviceChangedCallback.h>

#ifdef ENABLE_THERMAL_NETLINK
#include "thermalUtilsNetlink.h"
#else
#include "thermalUtils.h"
#endif

#include "thermalData.h"

namespace aidl {
namespace android {
namespace hardware {
namespace thermal {

struct CallbackSetting {
	std::shared_ptr<IThermalChangedCallback> callback;
	TemperatureType type;

	CallbackSetting(std::shared_ptr<IThermalChangedCallback> callback,
			TemperatureType type)
			: callback(callback), type(type) {}
 };

struct CdevCallbackSetting {
	std::shared_ptr<ICoolingDeviceChangedCallback> callback;
	CoolingType type;

	CdevCallbackSetting(std::shared_ptr<ICoolingDeviceChangedCallback> callback,
			CoolingType type)
			: callback(callback), type(type) {}
 };

class Thermal : public BnThermal {
  public:
    Thermal();
    ~Thermal() = default;

    Thermal(const Thermal &) = delete;
    void operator=(const Thermal &) = delete;
    ndk::ScopedAStatus getCoolingDevices(std::vector<CoolingDevice>* out_devices) override;
    ndk::ScopedAStatus getCoolingDevicesWithType(CoolingType in_type,
                                                 std::vector<CoolingDevice>* out_devices) override;

    ndk::ScopedAStatus getTemperatures(std::vector<Temperature>* out_temperatures) override;
    ndk::ScopedAStatus getTemperaturesWithType(TemperatureType in_type,
                                               std::vector<Temperature>* out_temperatures) override;

    ndk::ScopedAStatus getTemperatureThresholds(
            std::vector<TemperatureThreshold>* out_temperatureThresholds) override;

    ndk::ScopedAStatus getTemperatureThresholdsWithType(
            TemperatureType in_type,
            std::vector<TemperatureThreshold>* out_temperatureThresholds) override;

    ndk::ScopedAStatus registerThermalChangedCallback(
            const std::shared_ptr<IThermalChangedCallback>& in_callback) override;
    ndk::ScopedAStatus registerThermalChangedCallbackWithType(
            const std::shared_ptr<IThermalChangedCallback>& in_callback,
            TemperatureType in_type) override;
    ndk::ScopedAStatus registerCoolingDeviceChangedCallbackWithType(
		const std::shared_ptr<ICoolingDeviceChangedCallback> &in_callback,
                CoolingType in_type) override;

    ndk::ScopedAStatus unregisterThermalChangedCallback(
            const std::shared_ptr<IThermalChangedCallback>& in_callback) override;
    ndk::ScopedAStatus unregisterCoolingDeviceChangedCallback(
		const std::shared_ptr<ICoolingDeviceChangedCallback> &in_callback) override;

    void sendThrottlingChangeCB(const Temperature &t);
    void sendCoolingDeviceChangeCB(const CoolingDevice &c);

  private:
    std::mutex thermal_callback_mutex_;
    std::mutex cdev_callback_mutex_;
    std::vector<CallbackSetting> cb;
    std::vector<CdevCallbackSetting> cdev_cb;
    ThermalUtils utils;
};

}  // namespace thermal
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // ANDROID_QTI_VENDOR_THERMAL_H
