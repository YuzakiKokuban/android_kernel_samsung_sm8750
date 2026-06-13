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

#ifndef FST_MANAGER_HIDL_FSTMANAGER_H
#define FST_MANAGER_HIDL_FSTMANAGER_H

#include <vendor/qti/hardware/fstman/1.0/IFstGroupCallback.h>
#include <vendor/qti/hardware/fstman/1.0/IFstManager.h>
#include <vendor/qti/hardware/fstman/1.0/types.h>
#include <android-base/macros.h>
#include <hidl/Status.h>

namespace vendor {
namespace qti {
namespace hardware {
namespace fstman {
namespace V1_0 {
namespace implementation {
using namespace vendor::qti::hardware::fstman::V1_0;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::hidl_string;
using android::sp;

/**
 * Implementation of the fst manager hidl object. This hidl
 * object is used core for global control operations on
 * fst manager.
 */
class FstManager : public V1_0::IFstManager
{
public:
	FstManager();
	~FstManager() override = default;
	bool isValid();

	// Hidl methods exposed.
	Return<void> getGroup(
	    const hidl_string& groupName, getGroup_cb _hidl_cb) override;
	Return<void> listGroups(listGroups_cb _hidl_cb) override;
	Return<void> setDebugParams(
	    IFstManager::DebugLevel level, bool show_timestamp,
	    setDebugParams_cb _hidl_cb) override;
	Return<IFstManager::DebugLevel> getDebugLevel() override;
	Return<bool> isDebugShowTimestampEnabled() override;
	Return<void> terminate() override;

private:
	// Corresponding worker functions for the HIDL methods.
	std::pair<FstManagerStatus, sp<IFstGroup>> getGroupInternal(
	    const hidl_string& groupName);
	std::pair<FstManagerStatus, std::vector<hidl_string>>
		listGroupsInternal();
	FstManagerStatus setDebugParamsInternal(
	    IFstManager::DebugLevel level, bool show_timestamp);

	bool isGroupExists(const char *groupName);

	DISALLOW_COPY_AND_ASSIGN(FstManager);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace fstman
}  // namespace hardware
}  // namespace qti
}  // namespace vendor

#endif  // FST_MANAGER_HIDL_FSTMANAGER_H
