/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 * Not a contribution
 * Copyright (C) 2018 The Android Open Source Project
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

/* Changes from Qualcomm Innovation Center are provided under the following license:

Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear */

#define LOG_TAG "thermal_hal"

#include "thermal.h"
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using ::android::OK;
using ::android::status_t;
using aidl::android::hardware::thermal::Thermal;

int main() {

	LOG(INFO) << "Thermal HAL Service AIDL starting...";

	ABinderProcess_setThreadPoolMaxThreadCount(0);
	std::shared_ptr<Thermal> therm = ndk::SharedRefBase::make<Thermal>();

	const std::string instance = std::string() + Thermal::descriptor + "/default";

	if(therm){
		binder_status_t status =
		AServiceManager_addService(therm->asBinder().get(), instance.c_str());

		CHECK(status == STATUS_OK);
	}

	LOG(INFO) << "Thermal HAL Service AIDL started successfully.";
	ABinderProcess_joinThreadPool();
	return EXIT_FAILURE;  // should not reach
}
