/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include "thermalConfig.h"
#include "thermalUtilsNetlink.h"
#include "thermalCommon.h"

namespace aidl {
namespace android {
namespace hardware {
namespace thermal {

ThermalUtils::ThermalUtils(const ueventCB &inp_cb, const notifyCB &inp_cdev_cb):
	cfg(),
	cmnInst(),
	monitor(std::bind(&ThermalUtils::eventParse, this,
				std::placeholders::_1,
				std::placeholders::_2),
		std::bind(&ThermalUtils::sampleParse, this,
				std::placeholders::_1,
				std::placeholders::_2),
		std::bind(&ThermalUtils::eventCreateParse, this,
				std::placeholders::_1,
				std::placeholders::_2),
		std::bind(&ThermalUtils::cdevEventParse, this,
				std::placeholders::_1,
				std::placeholders::_2)),
	cb(inp_cb),
	notify(inp_cdev_cb)
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
			thermalConfig[sens.tzn] = sens;
			cmnInst.read_temperature(sens);
			cmnInst.estimateSeverity(sens);
			if (!sens.no_trip_set)
				cmnInst.initThreshold(sens);
		}
	}
	monitor.start();
	ret = cmnInst.initCdev();
	if (ret > 0) {
		is_cdev_init = true;
		cdevList = cmnInst.fetch_cdev_list();
		std::lock_guard<std::mutex> _lock(cdev_cb_mutex);
		for (struct therm_cdev cdevs: cdevList) {
			cmnInst.read_cdev_state(cdevs);
			cdev[cdevs.cdevn] = cdevs;
		}
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
		if (!sens.no_trip_set)
			cmnInst.initThreshold(sens);
	}
}

void ThermalUtils::cdevNotify(struct therm_cdev& cdev, int state)
{

	if (state > -1) {
		cdev.c.value = state;

		notify(cdev.c);
	}

}

void ThermalUtils::cdevEventParse(int cdevn, int state)
{
	if (cdev.find(cdevn) == cdev.end()) {
		LOG(DEBUG) << "cdev is not supported:" << cdevn
			<< std::endl;
		return;
	}
	std::lock_guard<std::mutex> _lock(cdev_cb_mutex);
	struct therm_cdev& cdevs = cdev[cdevn];
	return cdevNotify(cdevs, state);
}

void ThermalUtils::eventParse(int tzn, int trip)
{
	if (trip != 1)
		return;
	if (thermalConfig.find(tzn) == thermalConfig.end()) {
		LOG(DEBUG) << "sensor is not monitored:" << tzn
			<< std::endl;
		return;
	}
	std::lock_guard<std::mutex> _lock(sens_cb_mutex);
	struct therm_sensor& sens = thermalConfig[tzn];
	return Notify(sens);
}

void ThermalUtils::sampleParse(int tzn, int temp)
{
	if (thermalConfig.find(tzn) == thermalConfig.end()) {
		LOG(DEBUG) << "sensor is not monitored:" << tzn
			<< std::endl;
		return;
	}
	std::lock_guard<std::mutex> _lock(sens_cb_mutex);
	struct therm_sensor& sens = thermalConfig[tzn];
	sens.t.value = (float)temp / (float)sens.mulFactor;
	return Notify(sens);
}

void ThermalUtils::eventCreateParse(int tzn, const char *name)
{
	int ret = 0;
	std::vector<struct therm_sensor> sensorList;
	std::vector<struct target_therm_cfg> therm_cfg = cfg.fetchConfig();
	std::vector<struct target_therm_cfg>::iterator it_vec;
	std::vector<std::string>::iterator it;

	if (isSensorInitialized())
		return;
	for (it_vec = therm_cfg.begin();
		it_vec != therm_cfg.end(); it_vec++) {
		for (it = it_vec->sensor_list.begin();
			       it != it_vec->sensor_list.end(); it++) {
			if (!it->compare(name))
				break;
		}
		if (it != it_vec->sensor_list.end())
			break;
	}
	if (it_vec == therm_cfg.end()) {
		LOG(DEBUG) << "sensor is not monitored:" << tzn
			<< std::endl;
		return;
	}
	ret = cmnInst.initThermalZones(therm_cfg);
	if (ret > 0) {
		is_sensor_init = true;
		sensorList = cmnInst.fetch_sensor_list();
		std::lock_guard<std::mutex> _lock(sens_cb_mutex);
		for (struct therm_sensor sens: sensorList) {
			thermalConfig[sens.tzn] = sens;
			cmnInst.read_temperature(sens);
			cmnInst.estimateSeverity(sens);
			if (!sens.no_trip_set)
				cmnInst.initThreshold(sens);
		}
	}
}

int ThermalUtils::readTemperatures(std::vector<Temperature>& temp)
{
	std::unordered_map<int, struct therm_sensor>::iterator it;
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
	std::unordered_map<int, struct therm_sensor>::iterator it;
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
	std::unordered_map<int, struct therm_sensor>::iterator it;
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
	std::unordered_map<int, struct therm_sensor>::iterator it;
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
