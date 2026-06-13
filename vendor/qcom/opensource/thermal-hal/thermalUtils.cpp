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

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include "thermalConfig.h"
#include "thermalUtils.h"

namespace aidl {
namespace android {
namespace hardware {
namespace thermal {

ThermalUtils::ThermalUtils(const ueventCB &inp_cb):
	cfg(),
	cmnInst(),
	monitor(std::bind(&ThermalUtils::ueventParse, this,
				std::placeholders::_1,
				std::placeholders::_2)),
	cb(inp_cb)
{
	int ret = 0;
	std::vector<struct therm_sensor> sensorList;
	std::vector<struct target_therm_cfg> therm_cfg = cfg.fetchConfig();

	is_sensor_init = false;
	is_cdev_init = false;
	ret = cmnInst.initThermalZones(therm_cfg);
	if (ret > 0) {
		is_sensor_init = true;
		sensorList = cmnInst.fetch_sensor_list();
		std::lock_guard<std::mutex> _lock(sens_cb_mutex);
		for (struct therm_sensor sens: sensorList) {
			thermalConfig[sens.sensor_name] = sens;
			cmnInst.read_temperature(sens);
			cmnInst.estimateSeverity(sens);
			cmnInst.initThreshold(sens);
		}
		monitor.start();
	}
	ret = cmnInst.initCdev();
	if (ret > 0) {
		is_cdev_init = true;
		cdevList = cmnInst.fetch_cdev_list();
	}
}

void ThermalUtils::Notify(struct therm_sensor& sens)
{
	int severity = cmnInst.estimateSeverity(sens);
	if (severity != -1) {
		LOG(INFO) << "sensor: " << sens.sensor_name <<" temperature: "
			<< sens.t.value << " old: " <<
			(int)sens.lastThrottleStatus << " new: " <<
			(int)sens.t.throttlingStatus << std::endl;
		cb(sens.t);
		cmnInst.initThreshold(sens);
	}
}

void ThermalUtils::ueventParse(std::string sensor_name, int temp)
{
	LOG(DEBUG) << "uevent triggered for sensor: " << sensor_name
		<< std::endl;
	if (thermalConfig.find(sensor_name) == thermalConfig.end()) {
		LOG(DEBUG) << "sensor is not monitored:" << sensor_name
			<< std::endl;
		return;
	}
	std::lock_guard<std::mutex> _lock(sens_cb_mutex);
	struct therm_sensor& sens = thermalConfig[sensor_name];
	sens.t.value = (float)temp / (float)sens.mulFactor;
	return Notify(sens);
}

int ThermalUtils::readTemperatures(std::vector<Temperature>& temp)
{
	std::unordered_map<std::string, struct therm_sensor>::iterator it;
	int ret = 0;
	std::vector<Temperature> _temp;

	std::lock_guard<std::mutex> _lock(sens_cb_mutex);
	for (it = thermalConfig.begin(); it != thermalConfig.end(); it++) {
		struct therm_sensor& sens = it->second;

		ret = cmnInst.read_temperature(sens);
		if (ret < 0)
			return ret;
		Notify(sens);
		_temp.push_back(sens.t);
	}

	temp = _temp;
	return temp.size();
}

int ThermalUtils::readTemperatures(TemperatureType type,
                                            std::vector<Temperature>& temp)
{
	std::unordered_map<std::string, struct therm_sensor>::iterator it;
	int ret = 0;
	std::vector<Temperature> _temp;

	std::lock_guard<std::mutex> _lock(sens_cb_mutex);
	for (it = thermalConfig.begin(); it != thermalConfig.end(); it++) {
		struct therm_sensor& sens = it->second;

		if (sens.t.type != type)
			continue;
		ret = cmnInst.read_temperature(sens);
		if (ret < 0)
			return ret;
		Notify(sens);
		_temp.push_back(sens.t);
	}

	temp = _temp;
	return temp.size();
}

int ThermalUtils::readTemperatureThreshold(std::vector<TemperatureThreshold>& thresh)
{
	std::unordered_map<std::string, struct therm_sensor>::iterator it;
	std::vector<TemperatureThreshold> _thresh;

	for (it = thermalConfig.begin(); it != thermalConfig.end(); it++) {
		struct therm_sensor& sens = it->second;

		_thresh.push_back(sens.thresh);
	}

	thresh = _thresh;
	return thresh.size();
}

int ThermalUtils::readTemperatureThreshold(TemperatureType type,
                                            std::vector<TemperatureThreshold>& thresh)
{
	std::unordered_map<std::string, struct therm_sensor>::iterator it;
	std::vector<TemperatureThreshold> _thresh;

	for (it = thermalConfig.begin(); it != thermalConfig.end(); it++) {
		struct therm_sensor& sens = it->second;

		if (sens.t.type != type)
			continue;
		_thresh.push_back(sens.thresh);
	}

	thresh = _thresh;
	return thresh.size();
}

int ThermalUtils::readCdevStates(std::vector<CoolingDevice>& cdev_out)
{
	int ret = 0;
	std::vector<CoolingDevice> _cdev;

	for (struct therm_cdev cdev: cdevList) {

		ret = cmnInst.read_cdev_state(cdev);
		if (ret < 0)
			return ret;
		_cdev.push_back(cdev.c);
	}

	cdev_out = _cdev;

	return cdev_out.size();
}

int ThermalUtils::readCdevStates(cdevType type,
                                            std::vector<CoolingDevice>& cdev_out)
{
	int ret = 0;
	std::vector<CoolingDevice> _cdev;

	for (struct therm_cdev cdev: cdevList) {

		if (cdev.c.type != type)
			continue;
		ret = cmnInst.read_cdev_state(cdev);
		if (ret < 0)
			return ret;
		_cdev.push_back(cdev.c);
	}

	cdev_out = _cdev;

	return cdev_out.size();
}

}  // namespace thermal
}  // namespace hardware
}  // namespace android
}  // namespace aidl
