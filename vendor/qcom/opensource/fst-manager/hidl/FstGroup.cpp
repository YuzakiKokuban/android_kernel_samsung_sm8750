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
#include "FstGroup.h"

#define FST_MGR_COMPONENT "HIDL"
#include "fst_manager.h"
#include "fst_cfgmgr.h"

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

using namespace vendor::qti::hardware::fstman::V1_0;
using V1_0::IFstGroupCallback;

FstGroup::FstGroup(const char groupName[])
    : groupName_(groupName), is_valid_(true)
{}

void FstGroup::invalidate() { is_valid_ = false; }
bool FstGroup::isValid()
{
	return is_valid_;
}

Return<void> FstGroup::getName(getName_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstGroup::getNameInternal, _hidl_cb);
}

Return<void> FstGroup::registerCallback(
    const sp<IFstGroupCallback> &callback,
    registerCallback_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstGroup::registerCallbackInternal, _hidl_cb, callback);
}

Return<void> FstGroup::listInterfaces(listInterfaces_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstGroup::listInterfacesInternal, _hidl_cb);
}

Return<void> FstGroup::isFstModeSupported(isFstModeSupported_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstGroup::isFstModeSupportedInternal, _hidl_cb);
}

Return<void> FstGroup::isWifiSonModeSupported(
    isWifiSonModeSupported_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstGroup::isWifiSonModeSupportedInternal, _hidl_cb);
}

Return<void> FstGroup::getMuxInterfaceName(
	    getMuxInterfaceName_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstGroup::getMuxInterfaceNameInternal, _hidl_cb);
}

Return<void> FstGroup::setMuxInterfaceName(
	    const hidl_string& name, setMuxInterfaceName_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstGroup::setMuxInterfaceNameInternal, _hidl_cb, name);
}

Return<void> FstGroup::enslave(
	const hidl_string& ifname, bool enslave,
	enslave_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstGroup::enslaveInternal, _hidl_cb, ifname, enslave);
}

Return<void> FstGroup::isEnslaved(
	    const hidl_string& ifname, isEnslaved_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstGroup::isEnslavedInternal, _hidl_cb, ifname);
}

Return<void> FstGroup::setMacAddress(
	const hidl_array<uint8_t, 6>& mac,
	setMacAddress_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstGroup::setMacAddressInternal, _hidl_cb, mac);
}

Return<void> FstGroup::isRateUpgradeMaster(
	    const hidl_string& ifname, isRateUpgradeMaster_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstGroup::isRateUpgradeMasterInternal, _hidl_cb, ifname);
}

Return<void> FstGroup::renameInterface(const hidl_string& ifname,
				       const hidl_string& newifname,
				       renameInterface_cb _hidl_cb)
{
	return validateAndCall(
	    this, FstManagerStatusCode::FAILURE_UNKNOWN,
	    &FstGroup::renameInterfaceInternal, _hidl_cb, ifname, newifname);
}

std::pair<FstManagerStatus, std::string> FstGroup::getNameInternal()
{
	return {{FstManagerStatusCode::SUCCESS, ""}, groupName_};
}

FstManagerStatus FstGroup::registerCallbackInternal(
    const sp<IFstGroupCallback> &callback)
{
	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager ||
	    hidl_manager->addFstGroupCallbackHidlObject(groupName_, callback)) {
		return {FstManagerStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {FstManagerStatusCode::SUCCESS, ""};
}

std::pair<FstManagerStatus, std::vector<hidl_string>>
FstGroup::listInterfacesInternal()
{
	std::vector<hidl_string> ifaceNames;
	struct fst_group_info gi = {0};
	struct fst_iface_info *ifaces = NULL;
	int nof_ifaces, i;

	os_strlcpy(gi.id, groupName_.c_str(), sizeof(gi.id));

	nof_ifaces = fst_cfgmgr_get_group_ifaces(&gi, &ifaces);
	if (nof_ifaces < 0)
		goto finish;

	for (i = 0; i < nof_ifaces; i++)
		ifaceNames.emplace_back(ifaces[i].name);
finish:
	fst_free(ifaces);
	return {{FstManagerStatusCode::SUCCESS, ""}, std::move(ifaceNames)};
}

std::pair<FstManagerStatus, bool> FstGroup::isFstModeSupportedInternal()
{
	return {{FstManagerStatusCode::SUCCESS, ""}, true};
}

std::pair<FstManagerStatus, bool> FstGroup::isWifiSonModeSupportedInternal()
{
	return {{FstManagerStatusCode::SUCCESS, ""}, false};
}

std::pair<FstManagerStatus, std::string>
FstGroup::getMuxInterfaceNameInternal()
{
	char buf[IFNAMSIZ + 1];
	FstManagerStatus status = {FstManagerStatusCode::SUCCESS, ""};
	int res;
	std::string muxIfname;

	res = fst_cfgmgr_get_mux_ifname(groupName_.c_str(), buf, sizeof(buf));
	if (res <= 0)
		status = {FstManagerStatusCode::FAILURE_UNKNOWN, ""};
	else
		muxIfname = buf;

	return {status, muxIfname};
}

FstManagerStatus FstGroup::setMuxInterfaceNameInternal(const hidl_string& name)
{
	int res;

	res = fst_manager_set_mux_iface_name(groupName_.c_str(), name.c_str());
	if (res < 0)
		return {FstManagerStatusCode::FAILURE_UNKNOWN, ""};

	return {FstManagerStatusCode::SUCCESS, ""};
}

FstManagerStatus FstGroup::enslaveInternal(
	    const hidl_string& ifname, bool enslave)
{
	int res;
	FstManagerStatus status = {FstManagerStatusCode::SUCCESS, ""};

	fst_mgr_printf(MSG_INFO, "%s interface %s", enslave ? "enslave" : "release",
		ifname.c_str());

	res = fst_manager_enslave(groupName_.c_str(), ifname.c_str(), enslave ? TRUE : FALSE);
	if (res)
		status = {FstManagerStatusCode::FAILURE_UNKNOWN, ""};

	return status;
}

std::pair<FstManagerStatus, bool> FstGroup::isEnslavedInternal(
	    const hidl_string& ifname)
{
	Boolean isEnslaved = FALSE;
	FstManagerStatus status = {FstManagerStatusCode::SUCCESS, ""};
	int res;

	res = fst_manager_is_enslaved(groupName_.c_str(), ifname.c_str(),
				      &isEnslaved);
	if (res < 0)
		status = {FstManagerStatusCode::FAILURE_UNKNOWN, ""};

	return {status, (isEnslaved == TRUE)};
}

FstManagerStatus FstGroup::setMacAddressInternal(
	    const hidl_array<uint8_t, 6>& mac)
{
	int res;
	FstManagerStatus status = {FstManagerStatusCode::SUCCESS, ""};

	fst_mgr_printf(MSG_INFO, "%s new mac address " MACSTR, groupName_.c_str(), MAC2STR(mac.data()));

	res = fst_manager_set_mac_address(groupName_.c_str(), mac.data());
	if (res)
		status = {FstManagerStatusCode::FAILURE_UNKNOWN, ""};

	return status;
}

std::pair<FstManagerStatus, bool> FstGroup::isRateUpgradeMasterInternal(
    const hidl_string& ifname)
{
	char buf[IFNAMSIZ + 1];
	FstManagerStatus status = {FstManagerStatusCode::SUCCESS, ""};
	int res;
	bool isMaster = false;

	res = fst_cfgmgr_get_rate_upgrade_master(groupName_.c_str(), buf,
		sizeof(buf));
	if (res > 0) {
		isMaster = !strncmp(ifname.c_str(), buf, IFNAMSIZ);
	} else {
		status = {FstManagerStatusCode::FAILURE_UNKNOWN, ""};
	}

	return {status, isMaster};
}

FstManagerStatus FstGroup::renameInterfaceInternal(const hidl_string& ifname,
	const hidl_string& newifname)
{
	int res;
	FstManagerStatus status = {FstManagerStatusCode::SUCCESS, ""};

	fst_mgr_printf(MSG_INFO, "rename group %s interface %s to %s ",
		groupName_.c_str(), ifname.c_str(), newifname.c_str());

	res = fst_manager_rename_group_interface(groupName_.c_str(),
		ifname.c_str(), newifname.c_str());
	if (res)
		status = {FstManagerStatusCode::FAILURE_UNKNOWN, ""};

	return status;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace fstman
}  // namespace hardware
}  // namespace qti
}  // namespace vendor
