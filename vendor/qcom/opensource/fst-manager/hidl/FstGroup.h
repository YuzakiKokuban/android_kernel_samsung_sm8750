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

#ifndef FST_MANAGER_HIDL_FST_GROUP_H
#define FST_MANAGER_HIDL_FST_GROUP_H

#include <array>
#include <vector>

#include <android-base/macros.h>

#include <vendor/qti/hardware/fstman/1.0/IFstGroup.h>
#include <vendor/qti/hardware/fstman/1.0/IFstGroupCallback.h>

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
using android::hardware::hidl_array;

/**
 * Implementation of FstGroup hidl object. Each unique hidl
 * object is used for control operations on a specific group
 * controlled by fst-manager.
 */
class FstGroup : public IFstGroup
{
public:
	FstGroup(const char groupName[]);
	~FstGroup() override = default;
	// HIDL does not provide a built-in mechanism to let the server
	// invalidate a HIDL interface object after creation. If any client
	// process holds onto a reference to the object in their context,
	// any method calls on that reference will continue to be directed to
	// the server.
	// However FstManager HAL needs to control the lifetime of these
	// objects. So, add a public |invalidate| method to all |FstGroup| and
	// |FstIface| objects.
	// This will be used to mark an object invalid when the corresponding
	// iface or network is removed.
	// All HIDL method implementations should check if the object is still
	// marked valid before processing them.
	void invalidate();
	bool isValid();

	// Hidl methods exposed.
	Return<void> getName(getName_cb _hidl_cb) override;
	Return<void> registerCallback(
	    const sp<IFstGroupCallback>& callback,
	    registerCallback_cb _hidl_cb) override;
	Return<void> listInterfaces(listInterfaces_cb _hidl_cb) override;
	Return<void> isFstModeSupported(
	    isFstModeSupported_cb _hidl_cb) override;
	Return<void> isWifiSonModeSupported(
	    isWifiSonModeSupported_cb _hidl_cb) override;
	Return<void> getMuxInterfaceName(
	    getMuxInterfaceName_cb _hidl_cb) override;
	Return<void> setMuxInterfaceName(
	    const hidl_string& name, setMuxInterfaceName_cb _hidl_cb) override;
	Return<void> enslave(
	    const hidl_string& ifname,
	    bool enslave,
	    enslave_cb _hidl_cb) override;
	Return<void> isEnslaved(
	    const hidl_string& ifname, isEnslaved_cb _hidl_cb) override;
	Return<void> setMacAddress(
	    const hidl_array<uint8_t, 6>& mac,
	    setMacAddress_cb _hidl_cb) override;
	Return<void> isRateUpgradeMaster(
	    const hidl_string& ifname,
	    isRateUpgradeMaster_cb _hidl_cb) override;
	Return<void> renameInterface(const hidl_string& ifname,
				     const hidl_string& newifname,
				     renameInterface_cb _hidl_cb) override;
private:
	// Corresponding worker functions for the HIDL methods.
	std::pair<FstManagerStatus, std::string> getNameInternal();
	FstManagerStatus registerCallbackInternal(
	    const sp<IFstGroupCallback>& callback);
	std::pair<FstManagerStatus, std::vector<hidl_string>>
		listInterfacesInternal();
	std::pair<FstManagerStatus, bool> isFstModeSupportedInternal();
	std::pair<FstManagerStatus, bool> isWifiSonModeSupportedInternal();
	std::pair<FstManagerStatus, std::string> getMuxInterfaceNameInternal();
	FstManagerStatus setMuxInterfaceNameInternal(const hidl_string& name);
	FstManagerStatus enslaveInternal(
	    const hidl_string& ifname, bool enslave);
	std::pair<FstManagerStatus, bool> isEnslavedInternal(
	    const hidl_string& ifname);
	FstManagerStatus setMacAddressInternal(
	    const hidl_array<uint8_t, 6>& mac);
	std::pair<FstManagerStatus, bool> isRateUpgradeMasterInternal(
	    const hidl_string& ifname);
	FstManagerStatus renameInterfaceInternal(const hidl_string& ifname,
		const hidl_string& newifname);

	// Name of the group this hidl object controls
	const std::string groupName_;
	bool is_valid_;

	DISALLOW_COPY_AND_ASSIGN(FstGroup);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace fstman
}  // namespace hardware
}  // namespace qti
}  // namespace vendor

#endif  // FST_MANAGER_HIDL_FST_GROUP_H
