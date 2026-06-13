/*
 * Copyright (C) 2018,2020-2021, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 *
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
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ANDROID_HARDWARE_USB_GADGET_QTI_USBGADGET_H
#define ANDROID_HARDWARE_USB_GADGET_QTI_USBGADGET_H

#include <aidl/android/hardware/usb/gadget/BnUsbGadget.h>
#include <aidl/android/hardware/usb/gadget/BnUsbGadgetCallback.h>
#include <aidl/android/hardware/usb/gadget/GadgetFunction.h>
#include <aidl/android/hardware/usb/gadget/IUsbGadget.h>
#include <aidl/android/hardware/usb/gadget/IUsbGadgetCallback.h>
#include <mutex>

namespace aidl {
namespace android {
namespace hardware {
namespace usb {
namespace gadget {

using ::aidl::android::hardware::usb::gadget::GadgetFunction;
using ::aidl::android::hardware::usb::gadget::IUsbGadgetCallback;
using ::aidl::android::hardware::usb::gadget::IUsbGadget;
using ::aidl::android::hardware::usb::gadget::Status;
using ::aidl::android::hardware::usb::gadget::UsbSpeed;
using ::android::hardware::Return;
using ::android::hardware::usb::gadget::MonitorFfs;
using ::ndk::ScopedAStatus;
using ::std::shared_ptr;

struct UsbGadget : public BnUsbGadget {
  UsbGadget(const char* const gadget);

  ScopedAStatus setCurrentUsbFunctions(int64_t functions,
            const shared_ptr<IUsbGadgetCallback> &callback,
            int64_t timeoutMs, int64_t in_transactionId) override;

  ScopedAStatus getCurrentUsbFunctions(const shared_ptr<IUsbGadgetCallback> &callback,
	    int64_t in_transactionId) override;

  ScopedAStatus reset(const shared_ptr<IUsbGadgetCallback> &callback,
            int64_t in_transactionId) override;

  ScopedAStatus getUsbSpeed(const shared_ptr<IUsbGadgetCallback> &callback,
	    int64_t in_transactionId) override;

private:
  Status tearDownGadget();
  Status setupFunctions(int64_t functions,
                        const shared_ptr<IUsbGadgetCallback> &callback,
                        int64_t timeout, int64_t in_transactionId);
  int addFunctionsFromPropString(std::string prop, bool &ffsEnabled, int &i);

  MonitorFfs mMonitorFfs;

  // Makes sure that only one request is processed at a time.
  std::mutex mLockSetCurrentFunction;

  uint64_t mCurrentUsbFunctions;
  bool mCurrentUsbFunctionsApplied;
};

}  // namespace gadget
}  // namespace usb
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // ANDROID_HARDWARE_USB_GADGET_QTI_USBGADGET_H
