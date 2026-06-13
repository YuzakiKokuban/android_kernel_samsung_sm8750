/*
 * Copyright (C) 2019-2021, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 *
 * Copyright (C) 2017 The Android Open Source Project
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
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ANDROID_HARDWARE_USB_QTI_USB_H
#define ANDROID_HARDWARE_USB_QTI_USB_H

#include <aidl/android/hardware/usb/BnUsb.h>
#include <aidl/android/hardware/usb/BnUsbCallback.h>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <utils/Log.h>
#include <android-base/unique_fd.h>

namespace aidl {
namespace android {
namespace hardware {
namespace usb {

using ::aidl::android::hardware::usb::IUsbCallback;
using ::aidl::android::hardware::usb::PortRole;
using ::android::sp;
using ::android::base::unique_fd;
using ::ndk::ScopedAStatus;

struct Usb : public BnUsb {
    Usb();

    ScopedAStatus enableContaminantPresenceDetection(const std::string& in_portName,
            bool in_enable, int64_t in_transactionId) override;
    ScopedAStatus enableUsbData(const std::string& in_portName, bool in_enable,
            int64_t in_transactionId) override;
    ScopedAStatus enableUsbDataWhileDocked(const std::string& in_portName,
            int64_t in_transactionId) override;
    ScopedAStatus queryPortStatus(int64_t in_transactionId) override;
    ScopedAStatus setCallback(const std::shared_ptr<IUsbCallback>& in_callback) override;
    ScopedAStatus switchRole(const std::string& in_portName, const PortRole& in_role,
            int64_t in_transactionId) override;
    ScopedAStatus limitPowerTransfer(const std::string& in_portName, bool in_limit,
            int64_t in_transactionId) override;
    ScopedAStatus resetUsbPort(const std::string& in_portName,
            int64_t in_transactionId) override;
    Status getPortStatusHelper(std::vector<PortStatus> &currentPortStatus,
            const std::string &contaminantStatusPath);

    std::shared_ptr<IUsbCallback> mCallback;
    // Protects mCallback variable
    std::mutex mLock;
    // Protects roleSwitch operation
    std::mutex mRoleSwitchLock;
    // Threads waiting for the partner to come back wait here
    std::condition_variable mPartnerCV;
    // lock protecting mPartnerCV
    std::mutex mPartnerLock;
    // Variable to signal partner coming back online after type switch
    bool mPartnerUp;
    // Variable to indicate presence or absence or contaminant
    bool mContaminantPresence;
    // Variable to indicate presence or absence of wakeup node
    bool mIgnoreWakeup;
    // Configuration descriptor for MaxPower
    std::string mMaxPower;
    // Configuration descriptor for bmAttributes
    std::string mAttributes;
    // Current power operation mode
    std::string mPowerOpMode;
    // Path to get the status of contaminant presence
    std::string mContaminantStatusPath;
    // USB bus reset recovery active
    int usbResetRecov;
    // USB data disabled
    bool usbDataDisabled;
    // Limit power transfer
    bool limitedPower;
    // List of connected HID USB devices
    std::list <std::string> hidDevs;

  private:
    std::thread mPoll;
    unique_fd mEventFd;
    bool switchMode(const std::string &portName, const PortRole &newRole);
    void uevent_work();
};

}  // namespace usb
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // ANDROID_HARDWARE_USB_QTI_USB_H
