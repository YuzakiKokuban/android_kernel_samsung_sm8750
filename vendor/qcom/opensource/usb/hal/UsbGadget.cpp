/*
 * Copyright (C) 2018-2021, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "android.hardware.usb.gadget-service.qti"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <functional>
#include <map>
#include <tuple>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <UsbGadgetCommon.h>
#include "UsbGadget.h"

#define USB_CONTROLLER_PROP "vendor.usb.controller"
#define DIAG_FUNC_NAME_PROP "vendor.usb.diag.func.name"
#define RNDIS_FUNC_NAME_PROP "vendor.usb.rndis.func.name"
#define RMNET_FUNC_NAME_PROP "vendor.usb.rmnet.func.name"
#define RMNET_INST_NAME_PROP "vendor.usb.rmnet.inst.name"
#define DPL_INST_NAME_PROP "vendor.usb.dpl.inst.name"
#define VENDOR_USB_PROP "vendor.usb.config"
#define PERSIST_VENDOR_USB_PROP "persist.vendor.usb.config"
#define PERSIST_VENDOR_USB_EXTRA_PROP "persist.vendor.usb.config.extra"
#define QDSS_INST_NAME_PROP "vendor.usb.qdss.inst.name"
#define CONFIG_STRING CONFIG_PATH "strings/0x409/configuration"

namespace aidl {
namespace android {
namespace hardware {
namespace usb {
namespace gadget {

using ::android::base::GetProperty;
using ::android::base::SetProperty;
using ::android::base::Trim;
using ::android::base::WriteStringToFile;
using ::android::base::ReadFileToString;
using ::android::hardware::usb::gadget::addAdb;
using ::android::hardware::usb::gadget::kDisconnectWaitUs;
using ::android::hardware::usb::gadget::linkFunction;
using ::android::hardware::usb::gadget::resetGadget;
using ::android::hardware::usb::gadget::setVidPid;
using ::android::hardware::usb::gadget::unlinkFunctions;

static std::map<std::string, std::tuple<std::string, std::string, std::string> >
supported_compositions;

static void createCompositionsMap(std:: string fileName) {
  std::ifstream compositions(fileName);
  std::string line;

  while (std::getline(compositions, line)) {
    std::string prop;
    std::tuple<std::string, std::string, std::string> vpa;
    // Ignore comments in the file
    auto pos = line.find('#');
    if (pos != std::string::npos)
      line.erase(pos);

    std::stringstream words(line);

    words >> prop >> std::get<0>(vpa) >> std::get<1>(vpa) >> std::get<2>(vpa);
    // If we get vpa[1], we have the three minimum values needed. Or else we skip
    if (!std::get<1>(vpa).empty())
      supported_compositions.insert_or_assign(prop, vpa);
  }
}

UsbGadget::UsbGadget(const char* const gadget)
    : mCurrentUsbFunctionsApplied(false),
      mMonitorFfs(gadget) {
  if (access(CONFIG_PATH, R_OK) != 0)
    ALOGE("configfs setup not done yet");

  createCompositionsMap("/vendor/etc/usb_compositions.conf");
  createCompositionsMap("/odm/etc/usb_compositions.conf");
  createCompositionsMap("/product/etc/usb_compositions.conf");
}

ScopedAStatus UsbGadget::getCurrentUsbFunctions(
    const shared_ptr<IUsbGadgetCallback> &callback,
    int64_t in_transactionId) {
  if (callback == nullptr)
    return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);

  ScopedAStatus ret = callback->getCurrentUsbFunctionsCb(
      mCurrentUsbFunctions,
      mCurrentUsbFunctionsApplied ? Status::FUNCTIONS_APPLIED
                                  : Status::FUNCTIONS_NOT_APPLIED,
      in_transactionId);
  if (!ret.isOk())
    ALOGE("Call to getCurrentUsbFunctionsCb failed %s",
          ret.getDescription().c_str());

  return ScopedAStatus::ok();
}

ScopedAStatus UsbGadget::reset(const shared_ptr<IUsbGadgetCallback> &callback,
    int64_t in_transactionId) {
  if (callback == nullptr)
    return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);

  if (!WriteStringToFile("none", PULLUP_PATH)) {
    ALOGE("reset(): unable to clear pullup");
    return ScopedAStatus::fromServiceSpecificError(-1);
  }

  callback->resetCb(Status::SUCCESS, in_transactionId);

  return ScopedAStatus::ok();
}

ScopedAStatus UsbGadget::getUsbSpeed(const shared_ptr<IUsbGadgetCallback> &callback,
    int64_t in_transactionId) {
  if (callback == nullptr)
    return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);

  std::string gadgetName = GetProperty(USB_CONTROLLER_PROP, "");
  std::string current_speed;
  if (ReadFileToString("/sys/class/udc/" + gadgetName + "/current_speed",
                          &current_speed)) {
      current_speed = Trim(current_speed);

      UsbSpeed speed = UsbSpeed::UNKNOWN;
      if (current_speed == "low-speed")
        speed = UsbSpeed::LOWSPEED;
      else if (current_speed == "full-speed")
        speed = UsbSpeed::FULLSPEED;
      else if (current_speed == "high-speed")
        speed = UsbSpeed::HIGHSPEED;
      else if (current_speed == "super-speed")
        speed = UsbSpeed::SUPERSPEED;
      else if (current_speed == "super-speed-plus")
        speed = UsbSpeed::SUPERSPEED_10Gb;

    callback->getUsbSpeedCb(speed, in_transactionId);
    return ScopedAStatus::ok();
  }

  return ScopedAStatus::fromServiceSpecificError(-1);
}

Status UsbGadget::tearDownGadget() {
  if (mMonitorFfs.isMonitorRunning())
    mMonitorFfs.reset();
  else
    ALOGE("mMonitor not running");

  if (resetGadget() != ::android::hardware::usb::gadget::V1_0::Status::SUCCESS)
    return Status::ERROR;

  if (remove(OS_DESC_PATH))
    ALOGI("Unable to remove file %s errno:%d", OS_DESC_PATH, errno);

  return Status::SUCCESS;
}

static std::string rndisFuncname() {
  std::string rndisFunc = GetProperty(RNDIS_FUNC_NAME_PROP, "");

  if (rndisFunc.empty()) {
    return "rndis";
  }

  return rndisFunc + ".rndis";
}

static std::string qdssFuncname(const char *debug) {
  auto qdss = "qdss." + GetProperty(QDSS_INST_NAME_PROP, "qdss");
  auto debug_iface = FUNCTIONS_PATH + qdss + "/enable_debug_inface";

  WriteStringToFile(debug, debug_iface.c_str());

  return qdss;
}

static std::map<std::string, std::function<std::string()> > supported_funcs {
  { "adb",              [](){ return "ffs.adb"; } },
  { "ccid",             [](){ return "ccid.ccid"; } },
  { "diag",             [](){ return GetProperty(DIAG_FUNC_NAME_PROP, "diag") + ".diag"; } },
  { "diag_cnss",        [](){ return GetProperty(DIAG_FUNC_NAME_PROP, "diag") + ".diag_mdm2"; } },
  { "diag_mdm2",        [](){ return GetProperty(DIAG_FUNC_NAME_PROP, "diag") + ".diag_mdm2"; } },
  { "diag_mdm",         [](){ return GetProperty(DIAG_FUNC_NAME_PROP, "diag") + ".diag_mdm"; } },
  { "dpl",              [](){ return GetProperty(RMNET_FUNC_NAME_PROP, "gsi") + "." + GetProperty(DPL_INST_NAME_PROP, "dpl"); } },
  { "mass_storage",     [](){ return "mass_storage.0"; } },
  { "mtp",              [](){ return "ffs.mtp"; } },
  { "ncm",              [](){ return "ncm.0"; } },
  { "ptp",              [](){ return "ffs.ptp"; } },
  { "qdss",             [](){ return qdssFuncname("0"); } },
  { "qdss_debug",       [](){ return qdssFuncname("1"); } },
  { "qdss_mdm",         [](){ return "qdss.qdss_mdm"; } },
  { "rmnet",            [](){ return GetProperty(RMNET_FUNC_NAME_PROP, "gsi") + "." + GetProperty(RMNET_INST_NAME_PROP, "rmnet"); } },
  { "rndis",            rndisFuncname },
  { "serial_cdev",      [](){ return "cser.dun.0"; } },
  { "serial_cdev_nmea", [](){ return "cser.nmea.1"; } },
  { "serial_cdev_mdm",  [](){ return "cser.dun.2"; } },
  { "uac2",             [](){ return "uac2.0"; } },
  { "uvc",              [](){ return "uvc.0"; } },
};

int UsbGadget::addFunctionsFromPropString(std::string prop, bool &ffsEnabled, int &i) {
  if (!supported_compositions.count(prop)) {
    ALOGE("Composition \"%s\" unsupported", prop.c_str());
    return -1;
  }

  auto [vid, pid, actual_order] = supported_compositions[prop];
  ALOGE("vid %s pid %s", vid.c_str(), pid.c_str());

  // some compositions differ from the order given in the property string
  // e.g. ADB may appear somewhere in the middle instead of being last
  if (!actual_order.empty())
    prop = actual_order;

  WriteStringToFile(prop, CONFIG_STRING);

  // tokenize the prop string and add each function individually
  for (size_t start = 0; start != std::string::npos; ) {
    size_t end = prop.find_first_of(',', start);
    std::string funcname;
    if (end == std::string::npos) {
      funcname = prop.substr(start, prop.length() - start);
      start = end;
    } else {
      funcname = prop.substr(start, end - start);
      start = end + 1;
    }

    if (!supported_funcs.count(funcname)) {
      ALOGE("Function \"%s\" unsupported", funcname.c_str());
      return -1;
    }

    ALOGI("Adding %s", funcname.c_str());
    if (funcname == "adb") {
      if (addAdb(&mMonitorFfs, &i) != ::android::hardware::usb::gadget::V1_0::Status::SUCCESS)
        return -1;
      ffsEnabled = true;
    } else if (linkFunction(supported_funcs[funcname]().c_str(), i))
      return -1;

    // Set Diag PID for QC DLOAD mode
    if (i == 0 && !strcasecmp(vid.c_str(), "0x05c6") && funcname == "diag")
      WriteStringToFile(pid, FUNCTIONS_PATH "diag.diag/pid");

    ++i;
  }

  if (setVidPid(vid.c_str(), pid.c_str()) != ::android::hardware::usb::gadget::V1_0::Status::SUCCESS)
    return -1;

  return 0;
}

static Status validateAndSetVidPid(uint64_t functions) {
  ::android::hardware::usb::gadget::V1_0::Status ret =
    ::android::hardware::usb::gadget::V1_0::Status::SUCCESS;
  switch (functions) {
    case static_cast<uint64_t>(GadgetFunction::ADB):
      ret = setVidPid("0x18d1", "0x4e11");
      break;
    case static_cast<uint64_t>(GadgetFunction::MTP):
      ret = setVidPid("0x18d1", "0x4ee1");
      break;
    case GadgetFunction::ADB | GadgetFunction::MTP:
      ret = setVidPid("0x18d1", "0x4ee2");
      break;
    case static_cast<uint64_t>(GadgetFunction::RNDIS):
      ret = setVidPid("0x18d1", "0x4ee3");
      break;
    case GadgetFunction::ADB | GadgetFunction::RNDIS:
      ret = setVidPid("0x18d1", "0x4ee4");
      break;
    case static_cast<uint64_t>(GadgetFunction::PTP):
      ret = setVidPid("0x18d1", "0x4ee5");
      break;
    case GadgetFunction::ADB | GadgetFunction::PTP:
      ret = setVidPid("0x18d1", "0x4ee6");
      break;
    case static_cast<uint64_t>(GadgetFunction::MIDI):
      ret = setVidPid("0x18d1", "0x4ee8");
      break;
    case GadgetFunction::ADB | GadgetFunction::MIDI:
      ret = setVidPid("0x18d1", "0x4ee9");
      break;
    case static_cast<uint64_t>(GadgetFunction::ACCESSORY):
      ret = setVidPid("0x18d1", "0x2d00");
      break;
    case GadgetFunction::ADB | GadgetFunction::ACCESSORY:
      ret = setVidPid("0x18d1", "0x2d01");
      break;
    case static_cast<uint64_t>(GadgetFunction::AUDIO_SOURCE):
      ret = setVidPid("0x18d1", "0x2d02");
      break;
    case GadgetFunction::ADB | GadgetFunction::AUDIO_SOURCE:
      ret = setVidPid("0x18d1", "0x2d03");
      break;
    case GadgetFunction::ACCESSORY | GadgetFunction::AUDIO_SOURCE:
      ret = setVidPid("0x18d1", "0x2d04");
      break;
    case GadgetFunction::ADB | GadgetFunction::ACCESSORY |
	    GadgetFunction::AUDIO_SOURCE:
      ret = setVidPid("0x18d1", "0x2d05");
      break;
    case static_cast<uint64_t>(GadgetFunction::NCM):
      ret = setVidPid("0x18d1", "0x4eeb");
      break;
    case GadgetFunction::ADB | GadgetFunction::NCM:
      ret = setVidPid("0x18d1", "0x4eec");
      break;
    case GadgetFunction::UVC:
      ret = setVidPid("0x18d1", "0x4eed");
      break;
    case GadgetFunction::ADB | GadgetFunction::UVC:
      ret = setVidPid("0x18d1", "0x4eee");
      break;
    default:
      ALOGE("Combination not supported");
      ret = ::android::hardware::usb::gadget::V1_0::Status::CONFIGURATION_NOT_SUPPORTED;
  }
  return static_cast<Status>(ret);
}

Status UsbGadget::setupFunctions(
    int64_t functions, const shared_ptr<IUsbGadgetCallback> &callback,
    int64_t timeout, int64_t in_transactionId) {
  bool ffsEnabled = false;
  int i = 0;
  std::string gadgetName = GetProperty(USB_CONTROLLER_PROP, "");
  std::string vendorProp = GetProperty(VENDOR_USB_PROP, GetProperty(PERSIST_VENDOR_USB_PROP, ""));
  std::string vendorExtraProp = GetProperty(PERSIST_VENDOR_USB_EXTRA_PROP, "none");

  if (gadgetName.empty()) {
    ALOGE("UDC name not defined");
    return Status::ERROR;
  }

  if (((functions & GadgetFunction::RNDIS) != 0) ||
       ((functions & GadgetFunction::NCM) != 0)) {
    ALOGI("setCurrentUsbFunctions rndis");
    std::string tetherComp = (functions & GadgetFunction::RNDIS) ? "rndis" : "ncm";

    if (vendorExtraProp != "none")
      tetherComp += "," + vendorExtraProp;

    if (functions & GadgetFunction::ADB)
      tetherComp += ",adb";

    if (addFunctionsFromPropString(tetherComp, ffsEnabled, i))
      return Status::ERROR;
  } else if (functions == static_cast<uint64_t>(GadgetFunction::ADB) &&
      !vendorProp.empty() && vendorProp != "adb") {
    // override adb-only with additional QTI functions if vendor.usb.config
    // or persist.vendor.usb.config is set

    // tack on ADB to the property string if not there, since we only arrive
    // here if "USB debugging enabled" is chosen which implies ADB
    if (vendorProp.find("adb") == std::string::npos)
      vendorProp += ",adb";

    ALOGI("setting composition from %s: %s", VENDOR_USB_PROP,
            vendorProp.c_str());

    // look up & parse prop string and link each function into the composition
    if (addFunctionsFromPropString(vendorProp, ffsEnabled, i)) {
      // if failed just fall back to adb-only
      unlinkFunctions(CONFIG_PATH);
      i = 0;
      ffsEnabled = true;
      if (addAdb(&mMonitorFfs, &i) != ::android::hardware::usb::gadget::V1_0::Status::SUCCESS)
        return Status::ERROR;
    }
  } else { // standard Android supported functions
    WriteStringToFile("android", CONFIG_STRING);

    if ((functions & GadgetFunction::UVC) == 0) {
      if (addGenericAndroidFunctions(&mMonitorFfs, functions, &ffsEnabled, &i)
                != ::android::hardware::usb::gadget::V1_0::Status::SUCCESS)
        return Status::ERROR;
    } else {
        ALOGI("setCurrentUsbFunctions uvc");
        if (linkFunction("uvc.0", i++)) {
            return Status::ERROR;
        }
    }

    if ((functions & GadgetFunction::ADB) != 0) {
      ffsEnabled = true;
      if (addAdb(&mMonitorFfs, &i) != ::android::hardware::usb::gadget::V1_0::Status::SUCCESS)
        return Status::ERROR;
    }
  }

  if (functions & (GadgetFunction::ADB | GadgetFunction::MTP | GadgetFunction::PTP)) {
    if (symlink(CONFIG_PATH, OS_DESC_PATH)) {
      ALOGE("Cannot create symlink %s -> %s errno:%d", CONFIG_PATH, OS_DESC_PATH, errno);
      return Status::ERROR;
    }
  }

  // Pull up the gadget right away when there are no ffs functions.
  if (!ffsEnabled) {
    if (!WriteStringToFile(gadgetName, PULLUP_PATH)) return Status::ERROR;
    mCurrentUsbFunctionsApplied = true;
    if (callback)
      callback->setCurrentUsbFunctionsCb(functions, Status::SUCCESS,
                      in_transactionId);
    ALOGI("Gadget pullup without FFS fuctions");
    return Status::SUCCESS;
  }

  // Monitors the ffs paths to pull up the gadget when descriptors are written.
  // Also takes of the pulling up the gadget again if the userspace process
  // dies and restarts.
  mMonitorFfs.registerFunctionsAppliedCallback(
      [](bool functionsApplied, void *payload) {
        ((UsbGadget*)payload)->mCurrentUsbFunctionsApplied = functionsApplied;
      }, this);
  mMonitorFfs.startMonitor();

  ALOGI("Started monitor for FFS functions");

  if (callback) {
    bool gadgetPullup = mMonitorFfs.waitForPullUp(timeout);
    ScopedAStatus ret = callback->setCurrentUsbFunctionsCb(
        functions, gadgetPullup ? Status::SUCCESS : Status::ERROR,
        in_transactionId);
    if (!ret.isOk())
      ALOGE("setCurrentUsbFunctionsCb error %s", ret.getDescription().c_str());
  }

  return Status::SUCCESS;
}

ScopedAStatus UsbGadget::setCurrentUsbFunctions(int64_t functions,
                const shared_ptr<IUsbGadgetCallback> &callback,
                int64_t timeout, int64_t in_transactionId) {
  std::unique_lock<std::mutex> lk(mLockSetCurrentFunction);

  mCurrentUsbFunctions = functions;
  mCurrentUsbFunctionsApplied = false;

  // Unlink the gadget and stop the monitor if running.
  Status status = tearDownGadget();
  if (status != Status::SUCCESS) {
    goto error;
  }

  // Leave the gadget pulled down to give time for the host to sense disconnect.
  usleep(kDisconnectWaitUs);

  if (functions == static_cast<uint64_t>(GadgetFunction::NONE)) {
    if (callback == nullptr)
      return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
    ScopedAStatus ret = callback->setCurrentUsbFunctionsCb(functions,
                    Status::SUCCESS,
                    in_transactionId);
    if (!ret.isOk())
      ALOGE("Error while calling setCurrentUsbFunctionsCb %s",
            ret.getDescription().c_str());
    return ret;
  }

  status = validateAndSetVidPid(functions);
  if (status != Status::SUCCESS) {
    goto error;
  }

  status = setupFunctions(functions, callback, timeout, in_transactionId);
  if (status != Status::SUCCESS) {
    goto error;
  }

  ALOGI("Usb Gadget setcurrent functions called successfully");
  return ScopedAStatus::ok();

error:
  ALOGI("Usb Gadget setcurrent functions failed");
  if (callback == nullptr)
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(-1,
                    "Usb Gadget setcurrent functions failed");
  ScopedAStatus ret = callback->setCurrentUsbFunctionsCb(functions, status, in_transactionId);
  if (!ret.isOk()) {
    ALOGE("Error while calling setCurrentUsbFunctionsCb %s",
          ret.getDescription().c_str());
    return ret;
  }
  return ScopedAStatus::fromServiceSpecificErrorWithMessage(-1,
                    "Usb Gadget setcurrent functions failed");
}
}  // namespace gadget
}  // namespace usb
}  // namespace hardware
}  // namespace android
}  // namespace aidl

int main() {
  using android::base::GetProperty;
  using ::aidl::android::hardware::usb::gadget::UsbGadget;

  std::string gadgetName = GetProperty("persist.vendor.usb.controller",
      GetProperty(USB_CONTROLLER_PROP, ""));

  if (gadgetName.empty()) {
    ALOGE("UDC name not defined");
    return -1;
  }

  ABinderProcess_setThreadPoolMaxThreadCount(0);
  std::shared_ptr<UsbGadget> usb = ndk::SharedRefBase::make<UsbGadget>(gadgetName.c_str());

  const std::string instance = std::string(UsbGadget::descriptor) + "/default";
  binder_status_t status = AServiceManager_addService(usb->asBinder().get(), instance.c_str());
  CHECK(status == STATUS_OK);

  ALOGI("QTI USB Gadget HAL Ready.");
  ABinderProcess_joinThreadPool();
  return -1; // Should never be reached
}
