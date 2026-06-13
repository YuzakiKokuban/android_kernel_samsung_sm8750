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

#include <cstdio>
#include <cinttypes>
#include <string>
#include <dirent.h>
#include <unordered_map>
#include <fstream>

#include <android-base/logging.h>
#include "thermalCommon.h"

#define MAX_LENGTH		50
#define MAX_PATH		(256)
#define DEFAULT_HYSTERESIS	5
#define DEFAULT_SKIN_HYSTERESIS	2
#define THERMAL_SYSFS		"/sys/class/thermal/"
#define TZ_DIR_NAME		"thermal_zone"
#define TZ_DIR_FMT		"thermal_zone%d"
#define TEMPERATURE_FILE_FORMAT	"/sys/class/thermal/thermal_zone%d/temp"
#define POLICY_FILE_FORMAT	"/sys/class/thermal/thermal_zone%d/policy"
#define TRIP_FILE_FORMAT	"/sys/class/thermal/thermal_zone%d/trip_point_1_temp"
#define HYST_FILE_FORMAT	"/sys/class/thermal/thermal_zone%d/trip_point_1_hyst"
#define TRIP_TYPE_FILE_FORMAT	"/sys/class/thermal/thermal_zone%d/trip_point_%d_type"
#define TRIP_N_FILE_FORMAT	"/sys/class/thermal/thermal_zone%d/trip_point_%d_temp"
#define USER_SPACE_POLICY	"user_space"
#define TZ_TYPE			"type"
#define CDEV_DIR_NAME		"cooling_device"
#define CDEV_DIR_FMT		"cooling_device%d"
#define CDEV_CUR_STATE_PATH	"/sys/class/thermal/cooling_device%d/cur_state"
#define CPU_USAGE_FILE		"/proc/stat"
#define CPU_ONLINE_FILE_FORMAT	"/sys/devices/system/cpu/cpu%d/online"
#define LIMIT_PROFILE_1_SD_TEMP	125000
#define LIMIT_PROFILE_TRIP_TYPE	"hot"
#define LIMIT_PROFILE_SD_TZ	"aoss-0"
#define LIMIT_PROFILE_TRIP_MAX	11

namespace aidl {
namespace android {
namespace hardware {
namespace thermal {

static std::unordered_map<std::string, cdevType> cdev_map = {
	{"thermal-cpufreq-0", cdevType::CPU},
	{"thermal-cpufreq-1", cdevType::CPU},
	{"thermal-cpufreq-2", cdevType::CPU},
	{"thermal-cpufreq-3", cdevType::CPU},
	{"thermal-cpufreq-4", cdevType::CPU},
	{"thermal-cpufreq-5", cdevType::CPU},
	{"thermal-cpufreq-6", cdevType::CPU},
	{"thermal-cpufreq-7", cdevType::CPU},
	{"cpufreq-cpu0", cdevType::CPU},
	{"cpufreq-cpu1", cdevType::CPU},
	{"cpufreq-cpu2", cdevType::CPU},
	{"cpufreq-cpu3", cdevType::CPU},
	{"cpufreq-cpu4", cdevType::CPU},
	{"cpufreq-cpu5", cdevType::CPU},
	{"cpufreq-cpu6", cdevType::CPU},
	{"cpufreq-cpu7", cdevType::CPU},
	{"thermal-cluster-4-7", cdevType::CPU},
	{"thermal-cluster-3-7", cdevType::CPU},
	{"cpu-isolate0", cdevType::CPU},
	{"cpu-isolate1", cdevType::CPU},
	{"cpu-isolate2", cdevType::CPU},
	{"cpu-isolate3", cdevType::CPU},
	{"cpu-isolate4", cdevType::CPU},
	{"cpu-isolate5", cdevType::CPU},
	{"cpu-isolate6", cdevType::CPU},
	{"cpu-isolate7", cdevType::CPU},
	{"thermal-pause-1", cdevType::CPU},
	{"thermal-pause-2", cdevType::CPU},
	{"thermal-pause-4", cdevType::CPU},
	{"thermal-pause-8", cdevType::CPU},
	{"thermal-pause-10", cdevType::CPU},
	{"thermal-pause-20", cdevType::CPU},
	{"thermal-pause-40", cdevType::CPU},
	{"thermal-pause-80", cdevType::CPU},
	{"thermal-devfreq-0", cdevType::GPU},
	{"devfreq-3d00000.qcom,kgsl-3d0", cdevType::GPU},
	{"gpu", cdevType::GPU},
	{"modem_tj", cdevType::MODEM},
	{"modem_lte_dsc", cdevType::MODEM},
	{"modem_nr_dsc", cdevType::MODEM},
	{"modem_nr_scg_dsc", cdevType::MODEM},
	{"cdsp", cdevType::NPU},
	{"cdsp_hw", cdevType::NPU},
	{"battery", cdevType::BATTERY},
	{"fan-max31760", cdevType::FAN},
	{"display-fps", cdevType::DISPLAY},
};

ThermalCommon::ThermalCommon()
{
	LOG(DEBUG) << "Entering " << __func__;
	ncpus = (int)sysconf(_SC_NPROCESSORS_CONF);
	if (ncpus < 1)
		LOG(ERROR) << "Error retrieving number of cores";
}

static int writeToFile(std::string_view path, std::string data)
{
	std::fstream outFile;

	outFile.open(std::string(path).c_str(),
			std::fstream::binary | std::fstream::out);
	if (outFile.is_open()) {
		LOG(DEBUG) << "writing: "<< data << " in path:" << path
			<< std::endl;
		outFile << data;
		outFile.close();
		return data.length();
	}

	LOG(ERROR) << "Error opening file: "<< path << std::endl;
	return -1;
}

static int readLineFromFile(std::string_view path, std::string& out)
{
	char *fgets_ret;
	FILE *fd;
	int rv;
	char buf[MAX_LENGTH];

	out.clear();

	fd = fopen(std::string(path).c_str(), "r");
	if (fd == NULL) {
		LOG(ERROR) << "Path:" << std::string(path) << " file open error.err:"
			<< strerror(errno) << std::endl;
		return -errno;
	}

	fgets_ret = fgets(buf, MAX_LENGTH, fd);
	if (NULL != fgets_ret) {
		rv = (int)strlen(buf);
		out.append(buf, rv);
	} else {
		rv = ferror(fd);
	}

	fclose(fd);
	out.erase(std::remove(out.begin(), out.end(), '\n'), out.end());
	LOG(DEBUG) << "Path:" << std::string(path) << " Val:" << out << std::endl;

	return rv;
}

int ThermalCommon::readFromFile(std::string_view path, std::string& out)
{
	return readLineFromFile(path, out);
}

static int get_tzn(std::string sensor_name)
{
	DIR *tdir = NULL;
	struct dirent *tdirent = NULL;
	int found = -1;
	int tzn = 0;
	char name[MAX_PATH] = {0};
	char cwd[MAX_PATH] = {0};
	int ret = 0;

	if (!getcwd(cwd, sizeof(cwd)))
		return found;

	/* Change dir to read the entries. Doesnt work otherwise */
	ret = chdir(THERMAL_SYSFS);
	if (ret) {
		LOG(ERROR) << "Unable to change to " << THERMAL_SYSFS << std::endl;
		return found;
	}
	tdir = opendir(THERMAL_SYSFS);
	if (!tdir) {
		LOG(ERROR) << "Unable to open " << THERMAL_SYSFS << std::endl;
		return found;
	}

	while ((tdirent = readdir(tdir))) {
		std::string buf;

		if (strncmp(tdirent->d_name, TZ_DIR_NAME,
			strlen(TZ_DIR_NAME)) != 0)
			continue;

		snprintf(name, MAX_PATH, "%s%s/%s", THERMAL_SYSFS,
				tdirent->d_name, TZ_TYPE);
		ret = readLineFromFile(std::string_view(name), buf);
		if (ret <= 0) {
			LOG(ERROR) <<
				"get_tzn: sensor name read error for tz:" <<
				tdirent->d_name << std::endl;
			continue;
		}
		if (!strncmp(buf.c_str(), sensor_name.c_str(),
					sensor_name.length())) {
			found = 1;
			break;
		}
	}

	if (found == 1) {
		sscanf(tdirent->d_name, TZ_DIR_FMT, &tzn);
		LOG(DEBUG) << "Sensor: " << sensor_name <<
			" found at tz: " << tzn << std::endl;
		found = tzn;
	}

	closedir(tdir);
	/* Restore current working dir */
	ret = chdir(cwd);

	return found;
}

int ThermalCommon::initialize_sensor(struct target_therm_cfg& cfg, int sens_idx)
{
	struct therm_sensor sensor;
	int idx = 0;

	sensor.tzn = get_tzn(cfg.sensor_list[sens_idx]);
	if (sensor.tzn < 0) {
		LOG(ERROR) << "No thermal zone for sensor: " <<
			cfg.sensor_list[sens_idx] << ", ret:" <<
			sensor.tzn << std::endl;
		return -1;
	}
	if (cfg.type == TemperatureType::CPU)
		sensor.thresh.name = sensor.t.name =
			std::string("CPU") + std::to_string(sens_idx);
	else
		sensor.thresh.name = sensor.t.name = cfg.label;

	if (cfg.type == TemperatureType::BCL_PERCENTAGE)
		sensor.mulFactor = 1;
	else
		sensor.mulFactor = 1000;

	sensor.sensor_name = cfg.sensor_list[sens_idx];
	sensor.positiveThresh = cfg.positive_thresh_ramp;
	sensor.no_trip_set = cfg.no_trip_set;
	sensor.lastThrottleStatus = sensor.t.throttlingStatus =
		ThrottlingSeverity::NONE;
	sensor.thresh.type = sensor.t.type = cfg.type;
	for (idx = 0; idx <= (size_t)ThrottlingSeverity::SHUTDOWN; idx++) {
		sensor.thresh.hotThrottlingThresholds.push_back(UNKNOWN_TEMPERATURE);
		sensor.thresh.coldThrottlingThresholds.push_back(UNKNOWN_TEMPERATURE);
	}

	for (idx = 0; idx <= (size_t)ThrottlingSeverity::SHUTDOWN; idx++) {
		if (cfg.thresh[idx] != 0 && cfg.positive_thresh_ramp)
			sensor.thresh.hotThrottlingThresholds[idx] =
				cfg.thresh[idx] / (float)sensor.mulFactor;
		else if (cfg.thresh[idx] != 0 && !cfg.positive_thresh_ramp)
			sensor.thresh.coldThrottlingThresholds[idx] =
				cfg.thresh[idx] / (float)sensor.mulFactor;
	}
	sens.push_back(sensor);

	return 0;
}

int ThermalCommon::initializeCpuSensor(struct target_therm_cfg& cpu_cfg)
{
	int cpu = 0;

	for (;cpu < ncpus; cpu++) {
		if (initialize_sensor(cpu_cfg, cpu) < 0)
			return -1;
	}

	return 0;
}

int ThermalCommon::initThermalZones(std::vector<struct target_therm_cfg>& cfg)
{
	std::vector<struct target_therm_cfg>::iterator it;

	if (cfg.empty()) {
		LOG(ERROR) << std::string(__func__) +":Invalid input";
		return -1;
	}

	for (it = cfg.begin(); it != cfg.end(); it++)
	{
		if (it->type == TemperatureType::CPU) {
			if (initializeCpuSensor(*it) < 0)
				return -1;
			continue;
		}
		if (initialize_sensor(*it, 0) < 0) {
			return -1;
		}
	}

	return sens.size();
}

int ThermalCommon::initCdev()
{
	DIR *tdir = NULL;
	struct dirent *tdirent = NULL;
	int cdevn = 0;
	char name[MAX_PATH] = {0};
	char cwd[MAX_PATH] = {0};
	int ret = 0;

	if (!getcwd(cwd, sizeof(cwd)))
		return 0;

	/* Change dir to read the entries. Doesnt work otherwise */
	ret = chdir(THERMAL_SYSFS);
	if (ret) {
		LOG(ERROR) << "Unable to change to " << THERMAL_SYSFS << std::endl;
		return 0;
	}
	tdir = opendir(THERMAL_SYSFS);
	if (!tdir) {
		LOG(ERROR) << "Unable to open " << THERMAL_SYSFS << std::endl;
		return 0;
	}

	while ((tdirent = readdir(tdir))) {
		std::string buf;
		struct dirent *tzdirent;
		std::unordered_map<std::string, cdevType>::iterator it;
		struct therm_cdev cdevInst;

		if (strncmp(tdirent->d_name, CDEV_DIR_NAME,
			strlen(CDEV_DIR_NAME)) != 0)
			continue;

		snprintf(name, MAX_PATH, "%s%s/%s", THERMAL_SYSFS,
				tdirent->d_name, TZ_TYPE);
		ret = readLineFromFile(std::string_view(name), buf);
		if (ret <= 0) {
			LOG(ERROR) <<
				"init_cdev: cdev type read error for cdev:" <<
				tdirent->d_name << std::endl;
		}
		it = cdev_map.find(buf);
		if (it == cdev_map.end())
			continue;
		sscanf(tdirent->d_name, CDEV_DIR_FMT, &cdevn);
		LOG(DEBUG) << "cdev: " << it->first <<
			" found at cdev number: " << cdevn << std::endl;
		cdevInst.c.name = it->first;
		cdevInst.c.type = it->second;
		cdevInst.cdevn = cdevn;
		read_cdev_state(cdevInst);
		cdev.push_back(cdevInst);
	}

	closedir(tdir);
	/* Restore current working dir */
	ret = chdir(cwd);

	return cdev.size();
}

int ThermalCommon::read_cdev_state(struct therm_cdev& cdev)
{
	char file_name[MAX_PATH];
	std::string buf;
	int ret = 0, ct = 0;
	bool read_ok = false;

	snprintf(file_name, sizeof(file_name), CDEV_CUR_STATE_PATH,
			cdev.cdevn);
	do {
		ret = readLineFromFile(std::string(file_name), buf);
		if (ret <= 0) {
			LOG(ERROR) << "Cdev state read error:"<< ret <<
				" for cdev: " << cdev.c.name;
			return -1;
		}
		try {
			cdev.c.value = std::stoi(buf, nullptr, 0);
			read_ok = true;
		}
		catch (std::exception &err) {
			LOG(ERROR) << "Cdev read stoi error:" << err.what()
				<< " cdev:" << cdev.c.name << " ID:"
				<< cdev.cdevn << " buf:" << buf <<
				std::endl;
		}
		ct++;
	} while (!read_ok && ct < RETRY_CT);
	LOG(DEBUG) << "cdev Name:" << cdev.c.name << ". state:" <<
		cdev.c.value << std::endl;

	return cdev.c.value;
}

int ThermalCommon::estimateSeverity(struct therm_sensor& sensor)
{
	int idx = 0;
	ThrottlingSeverity severity = ThrottlingSeverity::NONE;
	float temp = sensor.t.value;
	int hysteresis = (sensor.thresh.type == TemperatureType::SKIN) ?
				DEFAULT_SKIN_HYSTERESIS : DEFAULT_HYSTERESIS;

	for (idx = (int)ThrottlingSeverity::SHUTDOWN; idx >= 0; idx--) {
		/* If a particular threshold is hit already, check if the
		 * hysteresis is cleared before changing the severity */
		if (idx <= (int)sensor.t.throttlingStatus) {
			if ((sensor.positiveThresh &&
				!isnan(sensor.thresh.hotThrottlingThresholds[idx]) &&
				temp >=
				(sensor.thresh.hotThrottlingThresholds[idx] - hysteresis)) ||
				(!sensor.positiveThresh &&
				!isnan(sensor.thresh.coldThrottlingThresholds[idx]) &&
				temp <=
				(sensor.thresh.coldThrottlingThresholds[idx] + hysteresis)))
				break;
			continue;
		}
		if ((sensor.positiveThresh &&
			!isnan(sensor.thresh.hotThrottlingThresholds[idx]) &&
			temp >=
			sensor.thresh.hotThrottlingThresholds[idx]) ||
		 	(!sensor.positiveThresh &&
			!isnan(sensor.thresh.coldThrottlingThresholds[idx]) &&
			temp <=
			sensor.thresh.coldThrottlingThresholds[idx]))
			break;
	}
	if (idx >= 0)
		severity = (ThrottlingSeverity)(idx);
	LOG(DEBUG) << "Sensor Name:" << sensor.t.name << "temp: " <<
		temp << ". prev severity:" <<
		(int)sensor.lastThrottleStatus << ". cur severity:" <<
		(int)sensor.t.throttlingStatus << " New severity:" <<
		(int)severity << std::endl;
	if (severity == sensor.t.throttlingStatus)
		return -1;
	sensor.lastThrottleStatus = sensor.t.throttlingStatus;
	sensor.t.throttlingStatus = severity;

	return (int)severity;
}

int ThermalCommon::read_temperature(struct therm_sensor& sensor)
{
	char file_name[MAX_PATH];
	float temp;
	std::string buf;
	int ret = 0, ct = 0;
	bool read_ok = false;

	do {
		snprintf(file_name, sizeof(file_name), TEMPERATURE_FILE_FORMAT,
			sensor.tzn);
		ret = readLineFromFile(std::string(file_name), buf);
		if (ret <= 0) {
			LOG(ERROR) << "Temperature read error:"<< ret <<
				" for sensor " << sensor.t.name;
			return -1;
		}
		try {
			sensor.t.value = (float)std::stoi(buf, nullptr, 0) /
				 (float)sensor.mulFactor;
			read_ok = true;
		}
		catch (std::exception &err) {
			LOG(ERROR) << "Temperature buf stoi error: "
				<< err.what()
				<< " buf:" << buf << " sensor:"
				<< sensor.t.name << " TZ:" <<
				sensor.tzn << std::endl;
		}
		ct++;
	} while (!read_ok && ct < RETRY_CT);
	LOG(DEBUG) << "Sensor Name:" << sensor.t.name << ". Temperature:" <<
		(float)sensor.t.value << std::endl;

	return ret;
}

void ThermalCommon::initThreshold(struct therm_sensor& sensor)
{
	char file_name[MAX_PATH] = "";
	std::string buf;
	int ret = 0, idx;
	ThrottlingSeverity severity = ThrottlingSeverity::NONE;
	int next_trip, curr_trip, hyst_temp = 0;
	int hysteresis = (sensor.thresh.type == TemperatureType::SKIN) ?
				DEFAULT_SKIN_HYSTERESIS : DEFAULT_HYSTERESIS;

	LOG(DEBUG) << "Entering " <<__func__;
	if (!sensor.positiveThresh) {
		LOG(ERROR) << "negative temperature ramp for sensor:"<<
			sensor.t.name;
		return;
	}
#ifndef ENABLE_THERMAL_NETLINK
	snprintf(file_name, sizeof(file_name), POLICY_FILE_FORMAT,
			sensor.tzn);
	ret = readLineFromFile(std::string(file_name), buf);
	if (ret <= 0) {
		LOG(ERROR) << "Policy read error:"<< ret <<
			" for sensor " << sensor.t.name;
		return;
	}
	if (buf != std::string(USER_SPACE_POLICY)) {
		LOG(ERROR) << "Policy error:"<< buf << " sensor:" <<
			sensor.t.name << std::endl;
		return;
	}
#endif
	next_trip = UNKNOWN_TEMPERATURE;
	for (idx = 0;idx <= (int)ThrottlingSeverity::SHUTDOWN; idx++) {
		if (isnan(sensor.thresh.hotThrottlingThresholds[idx])
			|| idx <= (int)sensor.t.throttlingStatus)
			continue;

		next_trip = sensor.thresh.hotThrottlingThresholds[idx] *
				sensor.mulFactor;
		break;
	}

	if (!isnan(next_trip)) {
		LOG(DEBUG) << "Sensor: " << sensor.t.name << " high trip:"
			<< next_trip << std::endl;
		snprintf(file_name, sizeof(file_name), TRIP_FILE_FORMAT,
				sensor.tzn);
		writeToFile(std::string_view(file_name), std::to_string(next_trip));
	}
	if (sensor.t.throttlingStatus != ThrottlingSeverity::NONE) {
		curr_trip = sensor.thresh.hotThrottlingThresholds[
				(int)sensor.t.throttlingStatus]
					* sensor.mulFactor;
		if (!isnan(next_trip))
			hyst_temp = (next_trip - curr_trip) + (hysteresis *
					sensor.mulFactor);
		else
			hyst_temp = hysteresis * sensor.mulFactor;
		LOG(DEBUG) << "Sensor: " << sensor.t.name << " hysteresis:"
			<< hyst_temp << std::endl;
		snprintf(file_name, sizeof(file_name), HYST_FILE_FORMAT,
				sensor.tzn);
		writeToFile(std::string_view(file_name), std::to_string(hyst_temp));
	}

	return;
}

int ThermalCommon::findLimitProfile(void)
{
	char file_name[MAX_PATH];
	std::string buf;
	int trip = 1, lp = -1;
	int ct = 0, ret = 0;
	bool read_ok = false;
	int tzn = get_tzn(LIMIT_PROFILE_SD_TZ);
	if (tzn < 0) {
		LOG(ERROR) <<" Not able to get tz id for "
		         << LIMIT_PROFILE_SD_TZ << std::endl;
		return -1;
	}

	do {
		snprintf(file_name, sizeof(file_name), TRIP_TYPE_FILE_FORMAT,
			tzn, trip);
		ret = readFromFile(std::string_view(file_name), buf);
		if (ret <= 0) {
			LOG(ERROR) <<" Not able to read trip type trip: "
					<< trip << std::endl;
			return -1;
		}
		if (strncmp(buf.c_str(), LIMIT_PROFILE_TRIP_TYPE,
					strlen(LIMIT_PROFILE_TRIP_TYPE))) {
			trip++;
			if (trip > LIMIT_PROFILE_TRIP_MAX)
				return -1;
			continue;
		}

		snprintf(file_name, sizeof(file_name), TRIP_N_FILE_FORMAT,
				tzn, trip);
		do {
			ret = readFromFile(std::string_view(file_name), buf);
			if (ret <= 0) {
				LOG(ERROR) <<" Not able to read trip temp, id: "
					<< trip << std::endl;
				return -1;
			}
			try {
				lp = std::stoi(buf, nullptr, 0);
				read_ok = true;
			}
			catch (std::exception &err) {
				LOG(ERROR) <<"limits profile stoi err:"
					<< err.what() << " buf:" << buf;
			}
			ct++;
		} while (!read_ok && ct < RETRY_CT);

		lp = lp == LIMIT_PROFILE_1_SD_TEMP ? 1 : 0;

		break;
	} while (1);

	return lp;
}
}// namespace thermal
}// namespace hardware
}// namespace android
}// namespace aidl
