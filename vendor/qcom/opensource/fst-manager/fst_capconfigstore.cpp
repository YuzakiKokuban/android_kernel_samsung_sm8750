/*
 * Provides access to the capablity config store in Android
 *
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 *
 */
#include <vendor/qti/hardware/capabilityconfigstore/1.0/ICapabilityConfigStore.h>
#include <vendor/qti/hardware/capabilityconfigstore/1.0/types.h>
#define FST_MGR_COMPONENT "CCS"
#include "fst_manager.h"
#include "utils/common.h"
#include "utils/os.h"
#include "fst_capconfigstore.h"

using android::hardware::hidl_string;
using namespace android;
using CommandResult = ::vendor::qti::hardware::capabilityconfigstore::V1_0::CommandResult;
using Result = ::vendor::qti::hardware::capabilityconfigstore::V1_0::Result;
using ICapabilityConfigStore = ::vendor::qti::hardware::capabilityconfigstore::V1_0::ICapabilityConfigStore;


#define WIGIG_CONFIG_STORE_AREA		"wigig"

void fst_get_config_string(const char *key, const char *defvalue,
			   char *buf, size_t len)
{
	sp<ICapabilityConfigStore> service;
	CommandResult res;

	res.result_type = Result::NOT_FOUND;

	service = ICapabilityConfigStore::getService();
	if (!service) {
		fst_mgr_printf(MSG_ERROR,
			       "failed to get capconfigstore service");
		goto out;
	}

	service->getConfig(WIGIG_CONFIG_STORE_AREA, key, [&](const CommandResult& res) {
		if(res.result_type == Result::SUCCESS) {
			fst_mgr_printf(MSG_DEBUG,
				       "getConfig: key %s returned value %s",
				       key, res.value.c_str());
			os_strlcpy(buf, res.value.c_str(), len);
		} else {
			fst_mgr_printf(MSG_DEBUG,
				       "getConfig: failed to get key %s",
				       key);
		}
	});

out:
	if (res.result_type != Result::SUCCESS) {
		fst_mgr_printf(MSG_DEBUG, "getConfig: use default value %s", defvalue);
		os_strlcpy(buf, defvalue, len);
	}
}
