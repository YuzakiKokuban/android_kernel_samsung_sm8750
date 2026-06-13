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

#include "hidl_manager.h"
#include "hidl_return_util.h"
#include "FstManager.h"

#define FST_MGR_COMPONENT "HIDL"
#include "fst_manager.h"
#include "fst_cfgmgr.h"
#include "fst_ctrl.h"

extern "C"
{
#include "utils/eloop.h"
}

namespace vendor {
namespace qti {
namespace hardware {
namespace fstman {
namespace V1_0 {
namespace implementation {
using hidl_return_util::validateAndCall;

FstManager::FstManager() {}

bool FstManager::isValid()
{
	// This top level object cannot be invalidated.
	return true;
}

Return<void> FstManager::getGroup(
    const hidl_string& groupName, getGroup_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstManager::getGroupInternal, _hidl_cb, groupName);
}

Return<void> FstManager::listGroups(listGroups_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstManager::listGroupsInternal, _hidl_cb);
}

Return<void> FstManager::setDebugParams(
    IFstManager::DebugLevel level, bool show_timestamp,
    setDebugParams_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstManager::setDebugParamsInternal, _hidl_cb, level,
	    show_timestamp);
}

Return<IFstManager::DebugLevel> FstManager::getDebugLevel()
{
	// TODO: Add FstManagerStatus in this method return for uniformity with
	// the other methods in FstManager HIDL interface.
	return (IFstManager::DebugLevel)wpa_debug_level;
}

Return<bool> FstManager::isDebugShowTimestampEnabled()
{
	// TODO: Add FstManagerStatus in this method return for uniformity with
	// the other methods in FstManager HIDL interface.
	return ((wpa_debug_timestamp != 0) ? true : false);
}

Return<void> FstManager::terminate()
{
	fst_mgr_printf(MSG_INFO, "Terminating...");
	fst_manager_terminate();
	return Void();
}

std::pair<FstManagerStatus, sp<IFstGroup>>
FstManager::getGroupInternal(const hidl_string& groupName)
{
	if (!isGroupExists(groupName.c_str())) {
		return {{FstManagerStatusCode::FAILURE_GROUP_UNKNOWN, ""},
			nullptr};
	}
	HidlManager* hidl_manager = HidlManager::getInstance();
	android::sp<IFstGroup> group;
	if (!hidl_manager ||
	    hidl_manager->getFstGroupHidlObjectByName(
			groupName, &group)) {
		return {{FstManagerStatusCode::FAILURE_UNKNOWN, ""}, group};
	}
	return {{FstManagerStatusCode::SUCCESS, ""}, group};
}

std::pair<FstManagerStatus, std::vector<hidl_string>>
FstManager::listGroupsInternal()
{
	std::vector<hidl_string> groupNames;
	struct fst_group_info *groups = NULL;
	int res, nof_groups, i;

	res = fst_cfgmgr_get_groups(&groups);
	if (res < 0) {
		goto finish;
	}

	nof_groups = res;
	for (i = 0; i < nof_groups; i++) {
		groupNames.emplace_back(groups[i].id);
	}
finish:
	fst_free(groups);
	return {{FstManagerStatusCode::SUCCESS, ""}, std::move(groupNames)};
}

FstManagerStatus FstManager::setDebugParamsInternal(
    IFstManager::DebugLevel level, bool show_timestamp)
{
	wpa_debug_level = static_cast<uint32_t>(level);
	wpa_debug_timestamp = show_timestamp ? 1 : 0;
	return {FstManagerStatusCode::SUCCESS, ""};
}

bool FstManager::isGroupExists(const char *groupName)
{
	struct fst_group_info *groups = NULL;
	int res, nof_groups, i;
	bool found;

	if (!groupName)
		return false;

	res = fst_cfgmgr_get_groups(&groups);
	if (res < 0) {
		return false;
	}

	nof_groups = res;
	found = false;
	for (i = 0; i < nof_groups; i++) {
		if (!strcmp(groupName, groups[i].id)) {
			found = true;
			break;
		}
	}

	fst_free(groups);
	return found;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace fstman
}  // namespace hardware
}  // namespace qti
}  // namespace vendor
