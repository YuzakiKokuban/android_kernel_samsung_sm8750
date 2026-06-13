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

#ifndef THERMAL_THERMAL_DATA_H__
#define THERMAL_THERMAL_DATA_H__

#include <vector>
#include <string>
#include <mutex>
#include <cmath>
#include <aidl/android/hardware/thermal/BnThermal.h>

#define UNKNOWN_TEMPERATURE (NAN)

namespace aidl{
namespace android {
namespace hardware {
namespace thermal {

using CoolingDevice = ::aidl::android::hardware::thermal::CoolingDevice;
using Temperature = ::aidl::android::hardware::thermal::Temperature;
using TemperatureType = ::aidl::android::hardware::thermal::TemperatureType;

using cdevType = ::aidl::android::hardware::thermal::CoolingType;
using CoolingDevice = ::aidl::android::hardware::thermal::CoolingDevice;
using Temperature = ::aidl::android::hardware::thermal::Temperature;
using TemperatureType = ::aidl::android::hardware::thermal::TemperatureType;
using TemperatureThreshold =
	::aidl::android::hardware::thermal::TemperatureThreshold;
using ::aidl::android::hardware::thermal::ThrottlingSeverity;

	enum QthrottlingSeverity {
		NONE = (size_t)ThrottlingSeverity::NONE,
		LIGHT = (size_t)ThrottlingSeverity::LIGHT,
		MODERATE = (size_t)ThrottlingSeverity::MODERATE,
		SEVERE = (size_t)ThrottlingSeverity::SEVERE,
		CRITICAL = (size_t)ThrottlingSeverity::CRITICAL,
		EMERGENCY = (size_t)ThrottlingSeverity::EMERGENCY,
		SHUTDOWN = (size_t)ThrottlingSeverity::SHUTDOWN,
	};

	struct target_therm_cfg {
		TemperatureType type;
		std::vector<std::string> sensor_list;
		std::string label;
		int thresh[SHUTDOWN + 1];
		bool positive_thresh_ramp;
		bool no_trip_set = false;
	};

	struct therm_sensor {
		int tzn;
		int mulFactor;
		bool positiveThresh;
		std::string sensor_name;
		ThrottlingSeverity lastThrottleStatus;
		Temperature t;
		TemperatureThreshold thresh;
		bool no_trip_set;
	};

	struct therm_cdev {
		int cdevn;
		CoolingDevice c;
	};

}  // namespace thermal
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // THERMAL_THERMAL_DATA_H__
