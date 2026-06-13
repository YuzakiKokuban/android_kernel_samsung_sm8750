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

#ifndef FST_MANAGER_HIDL_HIDL_MANAGER_H
#define FST_MANAGER_HIDL_HIDL_MANAGER_H

#include <map>
#include <string>

#include "FstManager.h"
#include "FstGroup.h"

extern "C"
{
#include "utils/common.h"
}

namespace vendor {
namespace qti {
namespace hardware {
namespace fstman {
namespace V1_0 {
namespace implementation {
using namespace vendor::qti::hardware::fstman::V1_0;
using V1_0::IFstManager;
using V1_0::IFstGroupCallback;

/**
 * HidlManager is responsible for managing the lifetime of all
 * hidl objects created by the fst manager. This is a singleton
 * class which is created by the fstman core and can be used
 * to get references to the hidl objects.
 */
class HidlManager
{
public:
	static HidlManager *getInstance();
	static void destroyInstance();

	// Methods called from fstman core.
	int registerHidlService();
	int registerGroup(const char *groupName);
	int unregisterGroup(const char *groupName);
	void notifyTest(const char *groupName, const char *msg); // DEBUGGING

	// Methods called from hidl objects.
	int addFstGroupCallbackHidlObject(
	    const std::string &groupName,
	    const android::sp<IFstGroupCallback> &callback);

	int getFstGroupHidlObjectByName(
	    const std::string &groupName,
	    android::sp<IFstGroup> *group_object);
private:
	HidlManager() = default;
	~HidlManager() = default;
	HidlManager(const HidlManager &) = default;
	HidlManager &operator=(const HidlManager &) = default;

	void removeFstGroupCallbackHidlObject(
	    const std::string &groupName,
	    const android::sp<IFstGroupCallback> &callback);

	void callWithEachFstGroupCallback(
	    const std::string &groupName,
	    const std::function<android::hardware::Return<void>(
		android::sp<IFstGroupCallback>)> &method);

	// Singleton instance of this class.
	static HidlManager *instance_;
	// The main hidl service object.
	android::sp<FstManager> fstman_object_;

	// Map of all the FST group specific hidl objects controlled by
	// fst-manager. This map is keyed in by the corresponding
	// |groupName|.
	std::map<const std::string, android::sp<FstGroup>>
	    fst_group_object_map_;
	// Map of all the callbacks registered for FST group specific
	// hidl objects controlled by fst-manager.  This map is keyed in by
	// the corresponding |groupName|.
	std::map<
	    const std::string,
	    std::vector<android::sp<IFstGroupCallback>>>
	    fst_group_callbacks_map_;

};

// The hidl interface uses some values which are the same as internal ones to
// avoid nasty runtime conversion functions. So, adding compile time asserts
// to guard against any internal changes breaking the hidl interface.
static_assert(
    static_cast<uint32_t>(IFstManager::DebugLevel::EXCESSIVE) == MSG_EXCESSIVE,
    "Debug level value mismatch");
static_assert(
    static_cast<uint32_t>(IFstManager::DebugLevel::ERROR) == MSG_ERROR,
    "Debug level value mismatch");

}  // namespace implementation
}  // namespace V1_0
}  // namespace fstman
}  // namespace hardware
}  // namespace qti
}  // namespace vendor

#endif  // WPA_SUPPLICANT_HIDL_HIDL_MANAGER_H
