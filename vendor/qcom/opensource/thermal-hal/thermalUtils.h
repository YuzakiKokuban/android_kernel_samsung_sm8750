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

Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear */

#ifndef THERMAL_THERMAL_UTILS_H__
#define THERMAL_THERMAL_UTILS_H__

#include <unordered_map>
#include <mutex>
#include <aidl/android/hardware/thermal/BnThermal.h>

#include "thermalConfig.h"
#include "thermalMonitor.h"
#include "thermalCommon.h"
#include "thermalData.h"

namespace aidl {
namespace android {
namespace hardware {
namespace thermal {

using ueventCB = std::function<void(Temperature &t)>;

class ThermalUtils {
	public:
		ThermalUtils(const ueventCB &inp_cb);
		~ThermalUtils() = default;
		bool isSensorInitialized()
		{
			return is_sensor_init;
		};
		bool isCdevInitialized()
		{
			return is_cdev_init;
		};
		int readTemperatures(std::vector<Temperature>& temp);
		int readTemperatures(TemperatureType type,
                                            std::vector<Temperature>& temperatures);
		int readTemperatureThreshold(std::vector<TemperatureThreshold>& thresh);
		int readTemperatureThreshold(TemperatureType type,
                                            std::vector<TemperatureThreshold>& thresh);
		int readCdevStates(std::vector<CoolingDevice>& cdev);
		int readCdevStates(cdevType type,
                                            std::vector<CoolingDevice>& cdev);
	private:
		bool is_sensor_init;
		bool is_cdev_init;
		ThermalConfig cfg;
		ThermalCommon cmnInst;
		ThermalMonitor monitor;
		std::unordered_map<std::string, struct therm_sensor>
			thermalConfig;
		std::vector<struct therm_cdev> cdevList;
		std::mutex sens_cb_mutex;
		ueventCB cb;

		void ueventParse(std::string sensor_name, int temp);
		void Notify(struct therm_sensor& sens);
};

}  // namespace thermal
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // THERMAL_THERMAL_UTILS_H__
