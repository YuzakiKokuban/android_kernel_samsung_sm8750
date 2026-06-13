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

#include <algorithm>

#include "hidl_manager.h"

#define FST_MGR_COMPONENT "HIDL"
#include "fst_manager.h"

namespace {
using android::hardware::hidl_array;
using namespace vendor::qti::hardware::fstman::V1_0;

/**
 * Add callback to the corresponding list after linking to death on the
 * corresponding hidl object reference.
 */
template <class CallbackType>
int registerForDeathAndAddCallbackHidlObjectToList(
    const android::sp<CallbackType> &callback,
    const std::function<void(const android::sp<CallbackType> &)>
	&on_hidl_died_fctor,
    std::vector<android::sp<CallbackType>> &callback_list)
{
	// TODO link to death
	callback_list.push_back(callback);
	return 0;
}

template <class ObjectType>
int addHidlObjectToMap(
    const std::string &key, const android::sp<ObjectType> object,
    std::map<const std::string, android::sp<ObjectType>> &object_map)
{
	// Return failure if we already have an object for that |key|.
	if (object_map.find(key) != object_map.end())
		return 1;
	object_map[key] = object;
	if (!object_map[key].get())
		return 1;
	return 0;
}

template <class ObjectType>
int removeHidlObjectFromMap(
    const std::string &key,
    std::map<const std::string, android::sp<ObjectType>> &object_map)
{
	// Return failure if we dont have an object for that |key|.
	const auto &object_iter = object_map.find(key);
	if (object_iter == object_map.end())
		return 1;
	object_iter->second->invalidate();
	object_map.erase(object_iter);
	return 0;
}

template <class CallbackType>
int addGroupCallbackHidlObjectToMap(
    const std::string &groupName, const android::sp<CallbackType> &callback,
    const std::function<void(const android::sp<CallbackType> &)>
	&on_hidl_died_fctor,
    std::map<const std::string, std::vector<android::sp<CallbackType>>>
	&callbacks_map)
{
	if (groupName.empty())
		return 1;

	auto group_callback_map_iter = callbacks_map.find(groupName);
	if (group_callback_map_iter == callbacks_map.end())
		return 1;
	auto &group_callback_list = group_callback_map_iter->second;

	// Register for death notification before we add it to our list.
	return registerForDeathAndAddCallbackHidlObjectToList<CallbackType>(
	    callback, on_hidl_died_fctor, group_callback_list);
}

template <class CallbackType>
void removeGroupCallbackHidlObjectFromMap(
    const std::string &groupName, const android::sp<CallbackType> &callback,
    std::map<const std::string, std::vector<android::sp<CallbackType>>>
	&callbacks_map)
{
	if (groupName.empty())
		return;

	auto group_callback_map_iter = callbacks_map.find(groupName);
	if (group_callback_map_iter == callbacks_map.end())
		return;

	auto &group_callback_list = group_callback_map_iter->second;
	group_callback_list.erase(
	    std::remove(
		group_callback_list.begin(), group_callback_list.end(),
		callback),
	    group_callback_list.end());
}

template <class CallbackType>
int removeAllGroupCallbackHidlObjectsFromMap(
    const std::string &groupName,
    std::map<const std::string, std::vector<android::sp<CallbackType>>>
	&callbacks_map)
{
	auto group_callback_map_iter = callbacks_map.find(groupName);
	if (group_callback_map_iter == callbacks_map.end())
		return 1;
	// TODO unlink to death
	callbacks_map.erase(group_callback_map_iter);
	return 0;
}

template <class CallbackType>
void callWithEachGroupCallback(
    const std::string &groupName,
    const std::function<
	android::hardware::Return<void>(android::sp<CallbackType>)> &method,
    const std::map<const std::string, std::vector<android::sp<CallbackType>>>
	&callbacks_map)
{
	if (groupName.empty())
		return;

	auto group_callback_map_iter = callbacks_map.find(groupName);
	if (group_callback_map_iter == callbacks_map.end())
		return;
	const auto &group_callback_list = group_callback_map_iter->second;
	for (const auto &callback : group_callback_list) {
		if (!method(callback).isOk()) {
			fst_mgr_printf(
			    MSG_ERROR, "Failed to invoke HIDL group callback");
		}
	}
}

}  // namespace

namespace vendor {
namespace qti {
namespace hardware {
namespace fstman {
namespace V1_0 {
namespace implementation {

using namespace vendor::qti::hardware::fstman::V1_0;
using V1_0::IFstGroupCallback;

HidlManager *HidlManager::instance_ = NULL;

HidlManager *HidlManager::getInstance()
{
	if (!instance_)
		instance_ = new HidlManager();
	return instance_;
}

void HidlManager::destroyInstance()
{
	if (instance_)
		delete instance_;
	instance_ = NULL;
}

int HidlManager::registerHidlService()
{
	::android::status_t status;

	// Create the main hidl service object and register it.
	fstman_object_ = new FstManager();
	status = fstman_object_->registerAsService();

	if (status != android::NO_ERROR) {
		 return 1;
	}
	return 0;
}

/**
 * Register a group to hidl manager.
 *
 * @param groupName the FST group name
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::registerGroup(const char *groupName)
{
	if (!groupName)
		return 1;

	if (addHidlObjectToMap<FstGroup>(
			groupName,
			new FstGroup(groupName),
			fst_group_object_map_)) {
			fst_mgr_printf(
			    MSG_ERROR,
			    "Failed to register FST group with HIDL "
			    "control: %s",
			    groupName);
			return 1;
	}
	fst_group_callbacks_map_[groupName] =
		    std::vector<android::sp<IFstGroupCallback>>();

	// TODO send callback when group created? Currently fst-manager
	// does not support dynamic group add/remove.
	return 0;
}

/**
 * Unregister a group from hidl manager.
 *
 * @param groupName the FST group name
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::unregisterGroup(const char *groupName)
{
	if (!groupName)
		return 1;

	int success = !removeHidlObjectFromMap(
	    groupName, fst_group_object_map_);
	if (success) {
		success = !removeAllGroupCallbackHidlObjectsFromMap(
		    groupName, fst_group_callbacks_map_);
	}
	if (!success) {
		fst_mgr_printf(
		    MSG_ERROR,
		    "Failed to unregister group with HIDL "
		    "control: %s",
		    groupName);
		return 1;
	}

	// TODO send callback when group removed? Currently fst-manager
	// does not support dynamic group add/remove.
	return 0;
}

/**
 * Add a new iface callback hidl object reference to our
 * interface callback list.
 *
 * @param ifname Name of the corresponding interface.
 * @param callback Hidl reference of the callback object.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::addFstGroupCallbackHidlObject(
    const std::string &groupName,
    const android::sp<IFstGroupCallback> &callback)
{
	const std::function<void(
	    const android::sp<IFstGroupCallback> &)>
	    on_hidl_died_fctor = std::bind(
		&HidlManager::removeFstGroupCallbackHidlObject, this, groupName,
		std::placeholders::_1);
	return addGroupCallbackHidlObjectToMap(
	    groupName, callback, on_hidl_died_fctor, fst_group_callbacks_map_);
}

/**
 * Retrieve the |IFstGroup| hidl object reference using the provided
 * group name.
 *
 * @param groupName Name of the corresponding interface.
 * @param group_object Hidl reference corresponding to the group.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::getFstGroupHidlObjectByName(
    const std::string &groupName, android::sp<IFstGroup> *group_object)
{
	if (groupName.empty() || !group_object)
		return 1;

	auto group_object_iter = fst_group_object_map_.find(groupName);
	if (group_object_iter == fst_group_object_map_.end())
		return 1;

	*group_object = group_object_iter->second;
	return 0;
}

/**
 * Removes the provided FST group callback hidl object reference from
 * our group callback list.
 *
 * @param groupName Name of the corresponding FST group.
 * @param callback Hidl reference of the callback object.
 */
void HidlManager::removeFstGroupCallbackHidlObject(
    const std::string &groupName,
    const android::sp<IFstGroupCallback> &callback)
{
	return removeGroupCallbackHidlObjectFromMap(
	    groupName, callback, fst_group_callbacks_map_);
}

/**
 * Helper fucntion to invoke the provided callback method on all the
 * registered FST group callback hidl objects for the specified
 * |groupName|.
 *
 * @param groupName Name of the corresponding FST group.
 * @param method Pointer to the required hidl method from
 * |IFstGroupCallback|.
 */
void HidlManager::callWithEachFstGroupCallback(
    const std::string &groupName,
    const std::function<Return<void>(android::sp<IFstGroupCallback>)>
	&method)
{
	callWithEachGroupCallback(groupName, method, fst_group_callbacks_map_);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace fstman
}  // namespace hardware
}  // namespace qti
}  // namespace vendor

