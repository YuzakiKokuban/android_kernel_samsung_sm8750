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

#define LOG_TAG "android.hardware.usb-service.qti"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <assert.h>
#include <chrono>
#include <dirent.h>
#include <regex>
#include <stdio.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include <cutils/uevent.h>
#include <linux/usb/ch9.h>
#include <sys/epoll.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>

#include "Usb.h"

#define VENDOR_USB_ADB_DISABLED_PROP "vendor.sys.usb.adb.disabled"
#define USB_CONTROLLER_PROP "vendor.usb.controller"
#define USB_MODE_PATH "/sys/bus/platform/devices/"
#define USB_UDC_PATH "/sys/class/udc"

namespace aidl {
namespace android {
namespace hardware {
namespace usb {

using ::android::base::SetProperty;
using ::android::base::GetProperty;
using ::android::base::Trim;
using ::android::base::ReadFileToString;
using ::android::base::WriteStringToFile;

static const std::string BACKLIGHT_PATH = "/sys/class/backlight/panel0-backlight/brightness";
const char GOOGLE_USB_VENDOR_ID_STR[] = "18d1";
const char GOOGLE_USBC_35_ADAPTER_UNPLUGGED_ID_STR[] = "5029";

static bool checkUsbWakeupSupport();
static void checkUsbInHostMode();
static void checkUsbDeviceAutoSuspend(const std::string& devicePath);
static bool checkUsbInterfaceAutoSuspend(const std::string& devicePath,
        const std::string &intf);
static bool checkUsbHidOnly(const std::string& devicePath);

static void getUsbControllerPath(std::string &controllerPath) {
  std::string controllerName = GetProperty(USB_CONTROLLER_PROP, "");
  std::string dwcDriver = "/sys/bus/platform/drivers/msm-dwc3/";
  struct dirent *deviceDir;
  std::string entry = "";
  std::size_t idx;
  DIR *gd;

  //Fetch controller address from vendor prop
  idx = controllerName.find(".");
  controllerName = controllerName.substr(0, idx);
  gd = opendir(dwcDriver.c_str());
  if (gd != NULL) {
    //Search for soft link to device
    while ((deviceDir = readdir(gd))) {
      if (deviceDir->d_type == DT_LNK &&
          strstr(deviceDir->d_name, controllerName.c_str())) {
        entry = deviceDir->d_name;
        controllerPath += dwcDriver + entry + "/";
        break;
      }
    }
    closedir(gd);
  }
}

ScopedAStatus Usb::enableUsbData(const std::string& in_portName, bool in_enable,
    int64_t in_transactionId) {
  std::scoped_lock lock(mLock);
  aidl::android::hardware::usb::Status status = Status::SUCCESS;
  std::string dwcDriver = "";
  int ret;

  ALOGI("enableUsbData in_enable: %d", in_enable);
  getUsbControllerPath(dwcDriver);
  if (dwcDriver == "") {
    ALOGE("resetUsbPort unable to find dwc device");
    status = Status::ERROR;
    goto out;
  }

  if (!in_enable) {
    ret = WriteStringToFile("1", dwcDriver + "dynamic_disable");
    if (!ret) {
      status = Status::ERROR;
      goto out;
    }
  } else {
    ret = WriteStringToFile("0", dwcDriver + "dynamic_disable");
    if (!ret) {
      status = Status::ERROR;
      goto out;
    }
  }

  usbDataDisabled = !in_enable;

out:
  if (mCallback) {
    std::vector<PortStatus> currentPortStatus;
    ScopedAStatus ret = mCallback->notifyEnableUsbDataStatus(in_portName, in_enable,
        status, in_transactionId);
    if (!ret.isOk())
      ALOGE("notifyEnableUsbDataStatus error %s", ret.getDescription().c_str());

    status = getPortStatusHelper(currentPortStatus, mContaminantStatusPath);
    ret = mCallback->notifyPortStatusChange(currentPortStatus,
          status);
    if (!ret.isOk())
      ALOGE("queryPortStatus error %s", ret.getDescription().c_str());
  } else {
    ALOGE("Not notifying the userspace. Callback is not set");
  }

  return ScopedAStatus::ok();
}

ScopedAStatus Usb::enableUsbDataWhileDocked(const std::string& in_portName,
    int64_t in_transactionId) {
  std::scoped_lock lock(mLock);
  if (mCallback) {
    ScopedAStatus ret = mCallback->notifyEnableUsbDataWhileDockedStatus(
        in_portName, Status::NOT_SUPPORTED, in_transactionId);
    if (!ret.isOk())
      ALOGE("notifyEnableUsbDataWhileDockedStatus error %s", ret.getDescription().c_str());
  } else {
    ALOGE("Not notifying the userspace. Callback is not set");
  }

  return ScopedAStatus::ok();
}

static std::string appendRoleNodeHelper(const std::string &portName, PortRole::Tag tag) {
    if ((portName == "..") || (portName.find('/') != std::string::npos)) {
       ALOGE("Fatal: invalid portName");
       return "";
    }

    std::string node("/sys/class/typec/" + portName);

    switch (tag) {
      case PortRole::dataRole:
        return node + "/data_role";
      case PortRole::powerRole:
        return node + "/power_role";
      case PortRole::mode:
        return node + "/port_type";
      default:
        return "";
    }
}

static const char *convertRoletoString(PortRole role) {
  if (role.getTag() == PortRole::powerRole) {
    if (role.get<PortRole::powerRole>() == PortPowerRole::SOURCE)
      return "source";
    else if (role.get<PortRole::powerRole>() == PortPowerRole::SINK)
      return "sink";
  } else if (role.getTag() == PortRole::dataRole) {
    if (role.get<PortRole::dataRole>() == PortDataRole::HOST)
      return "host";
    if (role.get<PortRole::dataRole>() == PortDataRole::DEVICE)
      return "device";
  } else if (role.getTag() == PortRole::mode) {
    if (role.get<PortRole::mode>() == PortMode::UFP)
      return "sink";
    if (role.get<PortRole::mode>() == PortMode::DFP)
      return "source";
  }
  return "none";
}

static void extractRole(std::string &roleName) {
  std::size_t first, last;

  first = roleName.find("[");
  last = roleName.find("]");

  if (first != std::string::npos && last != std::string::npos) {
    roleName = roleName.substr(first + 1, last - first - 1);
  }
}

static void switchToDrp(const std::string &portName) {
  std::string filename = appendRoleNodeHelper(portName, PortRole::mode);

  if (!WriteStringToFile("dual", filename))
    ALOGE("Fatal: Error while switching back to drp");
}

bool Usb::switchMode(const std::string &portName, const PortRole &newRole) {
  std::string filename = appendRoleNodeHelper(portName, newRole.getTag());
  bool roleSwitch = false;

  if (filename == "") {
    ALOGE("Fatal: invalid node type");
    return false;
  }

  {
    // Hold the lock here to prevent loosing connected signals
    // as once the file is written the partner added signal
    // can arrive anytime.
    std::unique_lock lock(mPartnerLock);
    mPartnerUp = false;
    if (WriteStringToFile(convertRoletoString(newRole), filename)) {
      // The type-c stack waits for 4.5 - 5.5 secs before declaring a port non-pd.
      // The -partner directory would not be created until this is done.
      // Having a margin of ~3 secs for the directory and other related bookeeping
      // structures created and uvent fired.
      constexpr std::chrono::seconds port_timeout(8);

wait_again:
      if (mPartnerCV.wait_for(lock, port_timeout) == std::cv_status::timeout) {
        // There are no uevent signals which implies role swap timed out.
        ALOGI("uevents wait timedout");
      } else if (!mPartnerUp) { // Sanity check.
        goto wait_again;
      } else {
        // Role switch succeeded since usb->mPartnerUp is true.
        roleSwitch = true;
      }
    } else {
      ALOGI("Role switch failed while writing to file");
    }
  }

  // if (!roleSwitch)  // Back to DRP for the following Role Swaps (if any) [@VOLD]
  switchToDrp(portName);

  return roleSwitch;
}

Usb::Usb() : mPartnerUp(false), mContaminantPresence(false) { }

ScopedAStatus Usb::switchRole(const std::string &portName, const PortRole &newRole,
    int64_t in_transactionId) {
  std::string filename = appendRoleNodeHelper(portName, newRole.getTag());
  std::string written;
  bool roleSwitch = false;

  if (filename == "") {
    ALOGE("Fatal: invalid node type");
    return ScopedAStatus::ok();
  }

  std::scoped_lock role_lock(mRoleSwitchLock);

  ALOGI("filename write: %s role:%s", filename.c_str(), convertRoletoString(newRole));

  if (newRole.getTag() == PortRole::mode) {
      roleSwitch = switchMode(portName, newRole);
  } else {
    if (WriteStringToFile(convertRoletoString(newRole), filename)) {
      if (ReadFileToString(filename, &written)) {
        extractRole(written);
        ALOGI("written: %s", written.c_str());
        if (written == convertRoletoString(newRole)) {
          roleSwitch = true;
        } else {
          ALOGE("Role switch failed");
        }
      } else {
        ALOGE("Unable to read back the new role");
      }
    } else {
      ALOGE("Role switch failed while writing to file");
    }
  }

  std::scoped_lock lock(mLock);
  if (mCallback) {
    ScopedAStatus ret = mCallback->notifyRoleSwitchStatus(portName, newRole,
        roleSwitch ? Status::SUCCESS : Status::ERROR, in_transactionId);
    if (!ret.isOk())
      ALOGE("RoleSwitchStatus error %s", ret.getDescription().c_str());
  } else {
    ALOGE("Not notifying the userspace. Callback is not set");
  }

  return ScopedAStatus::ok();
}

static void manageHidPower(const std::string& path, bool allow) {
#if 0    //[@VOLD Apply QC patch from USB Driver]
  std::string runtimeState;
  bool ret = false;

  ret = ReadFileToString(path + "/power/runtime_status", &runtimeState);
  if (ret && runtimeState == "active") {
    ret = WriteStringToFile("on", path + "/power/control");
    if (!ret)
          return;
  }

  ret = WriteStringToFile(allow ? "3000" : "60000", path + "/power/autosuspend_delay_ms");
  if (!ret)
        return;

  WriteStringToFile("auto", path + "/power/control");
#endif    //[@VOLD Apply QC patch from USB Driver]
}

/*
 * Checks for if the display is off.  This is useful for triggerring
 * USB suspend routines for certain use cases.  One example is when a USB
 * HID device, such as a mouse or keyboard is connected, and to avoid
 * frequent bus suspend/resume iterations if the display is on.
 */
static bool isDisplayOff(void) {
  std::string brightness;

  if (!ReadFileToString(BACKLIGHT_PATH, &brightness)) {
    ALOGE("isDisplayOff: Failed to open brightness file");
    return false;
  }

  if (std::stoi(brightness) > 0)
    return false;

  return true;
}

static void removeUsbHIDDevice(struct Usb *usb, const std::string &devicePath) {
  usb->hidDevs.remove(devicePath);
}

static void checkUsbHIDDevice(struct Usb *usb, std::string&& devicePath) {
  if (checkUsbHidOnly(devicePath)) {
    /*
     * Keep track of connected devices with HID interfaces, so that based
     * on display brightness events, the autosuspend timer can be adjusted
     * for all HID devices.
     */
    usb->hidDevs.push_back(std::move(devicePath));
  }
}

static void handleDisplayEvent(struct Usb *usb) {
  bool displayOff = isDisplayOff();

  for (auto dev : usb->hidDevs)
      manageHidPower(dev, displayOff);
}

static Status getAccessoryConnected(const std::string &portName, std::string &accessory) {
  std::string filename = "/sys/class/typec/" + portName + "-partner/accessory_mode";

  if (!ReadFileToString(filename, &accessory)) {
    ALOGE("getAccessoryConnected: Failed to open filesystem node: %s",
          filename.c_str());
    return Status::ERROR;
  }

  accessory = Trim(accessory);
  return Status::SUCCESS;
}

static Status getCurrentRoleHelper(const std::string &portName, bool connected,
                                   PortRole &currentRole) {
  std::string filename;
  std::string roleName;
  std::string accessory;

  // Mode

  if (currentRole.getTag() == PortRole::powerRole) {
    filename = "/sys/class/typec/" + portName + "/power_role";
    currentRole.set<PortRole::powerRole>(PortPowerRole::NONE);
  } else if (currentRole.getTag() == PortRole::dataRole) {
    filename = "/sys/class/typec/" + portName + "/data_role";
    currentRole.set<PortRole::dataRole>(PortDataRole::NONE);
  } else if (currentRole.getTag() == PortRole::mode) {
    filename = "/sys/class/typec/" + portName + "/data_role";
    currentRole.set<PortRole::mode>(PortMode::NONE);
  } else {
    return Status::ERROR;
  }

  if (!connected)
    return Status::SUCCESS;

  if (currentRole.getTag() == PortRole::mode) {
    if (getAccessoryConnected(portName, accessory) != Status::SUCCESS) {
      return Status::ERROR;
    }
    if (accessory == "analog_audio") {
      currentRole.set<PortRole::mode>(PortMode::AUDIO_ACCESSORY);
      return Status::SUCCESS;
    } else if (accessory == "debug") {
      currentRole.set<PortRole::mode>(PortMode::DEBUG_ACCESSORY);
      return Status::SUCCESS;
    }
  }

  if (!ReadFileToString(filename, &roleName)) {
    ALOGE("getCurrentRole: Failed to open filesystem node: %s", filename.c_str());
    return Status::ERROR;
  }

  extractRole(roleName);

  if (roleName == "source") {
    currentRole.set<PortRole::powerRole>(PortPowerRole::SOURCE);
  } else if (roleName == "sink") {
    currentRole.set<PortRole::powerRole>(PortPowerRole::SINK);
  } else if (roleName == "host") {
    if (currentRole.getTag() == PortRole::dataRole)
      currentRole.set<PortRole::dataRole>(PortDataRole::HOST);
    else
      currentRole.set<PortRole::mode>(PortMode::DFP);
  } else if (roleName == "device") {
    if (currentRole.getTag() == PortRole::dataRole)
      currentRole.set<PortRole::dataRole>(PortDataRole::DEVICE);
    else
      currentRole.set<PortRole::mode>(PortMode::UFP);
  } else if (roleName != "none") {
    /* case for none has already been addressed.
     * so we check if the role isn't none.
     */
    return Status::UNRECOGNIZED_ROLE;
  }

  return Status::SUCCESS;
}

static std::unordered_map<std::string, bool> getTypeCPortNamesHelper() {
  std::unordered_map<std::string, bool> names;
  DIR *dp;
  dp = opendir("/sys/class/typec/");
  if (dp != NULL) {
    struct dirent *ep;

    while ((ep = readdir(dp))) {
      if (ep->d_type == DT_LNK) {
        std::string entry = ep->d_name;
        auto n = entry.find("-partner");
        if (n == std::string::npos) {
          std::unordered_map<std::string, bool>::const_iterator portName =
              names.find(entry);
          if (portName == names.end()) {
            names.insert({entry, false});
          }
        } else {
          names[entry.substr(0, n)] = true;
        }
      }
    }
    closedir(dp);
  } else {
    ALOGE("Failed to open /sys/class/typec");
  }

  return names;
}

static bool canSwitchRoleHelper(const std::string &portName) {
  std::string filename = "/sys/class/typec/" + portName + "-partner/supports_usb_power_delivery";
  std::string supportsPD;

  if (ReadFileToString(filename, &supportsPD)) {
    if (supportsPD[0] == 'y') {
      return true;
    }
  }

  return false;
}

Status Usb::getPortStatusHelper(std::vector<PortStatus> &currentPortStatus,
    const std::string &contaminantStatusPath) {
  auto names = getTypeCPortNamesHelper();

  if (!names.empty()) {
    currentPortStatus.resize(names.size());
    int i = 0;
    for (auto & [portName, connected] : names) {
      ALOGI("%s", portName.c_str());
      auto & status = currentPortStatus[i++];
      status.portName = portName;

      PortRole currentRole;
      currentRole.set<PortRole::powerRole>(PortPowerRole::NONE);
      if (getCurrentRoleHelper(portName, connected, currentRole) == Status::SUCCESS) {
        status.currentPowerRole = currentRole.get<PortRole::powerRole>();
      } else {
        ALOGE("Error while retrieving current power role");
        goto done;
      }

      currentRole.set<PortRole::dataRole>(PortDataRole::NONE);
      if (getCurrentRoleHelper(portName, connected, currentRole) == Status::SUCCESS) {
        status.currentDataRole = currentRole.get<PortRole::dataRole>();
      } else {
        ALOGE("Error while retrieving current data role");
        goto done;
      }

      currentRole.set<PortRole::mode>(PortMode::NONE);
      if (getCurrentRoleHelper(portName, connected, currentRole) == Status::SUCCESS) {
        status.currentMode = currentRole.get<PortRole::mode>();
      } else {
        ALOGE("Error while retrieving current mode");
        goto done;
      }

      status.canChangeMode = true;
      status.canChangeDataRole = connected ? canSwitchRoleHelper(portName) : false;
      status.canChangePowerRole = connected ? canSwitchRoleHelper(portName) : false;

      status.supportedModes.push_back(PortMode::DRP);
      status.supportedModes.push_back(PortMode::AUDIO_ACCESSORY);
      status.usbDataStatus.push_back(usbDataDisabled ? UsbDataStatus::DISABLED_FORCE :
                                       UsbDataStatus::ENABLED);

      status.powerTransferLimited = limitedPower;

      ALOGI("%d:%s connected:%d canChangeMode:%d canChangeData:%d canChangePower:%d "
            "usbDataDisabled:%d, powerTransferLimited:%d",
            i, portName.c_str(), connected, status.canChangeMode,
            status.canChangeDataRole, status.canChangePowerRole, usbDataDisabled,
            limitedPower);

      status.supportsEnableContaminantPresenceProtection = false;
      status.supportsEnableContaminantPresenceDetection = false;
      status.contaminantProtectionStatus = ContaminantProtectionStatus::FORCE_SINK;

      if (portName != "port0") // moisture detection only on first port
        continue;

      std::string contaminantPresence;

      if (!contaminantStatusPath.empty() &&
              ReadFileToString(contaminantStatusPath, &contaminantPresence)) {
        status.supportedContaminantProtectionModes
            .push_back(ContaminantProtectionMode::FORCE_SINK);
        status.supportedContaminantProtectionModes
            .push_back(ContaminantProtectionMode::FORCE_DISABLE);

        if (contaminantPresence[0] == '1') {
          status.contaminantDetectionStatus = ContaminantDetectionStatus::DETECTED;
            ALOGI("moisture: Contaminant presence detected");
        } else {
            status.contaminantDetectionStatus = ContaminantDetectionStatus::NOT_DETECTED;
        }
      } else {
        status.supportedContaminantProtectionModes
            .push_back(ContaminantProtectionMode::NONE);
        status.contaminantProtectionStatus = ContaminantProtectionStatus::NONE;
      }
    }
    return Status::SUCCESS;
  }
done:
  return Status::ERROR;
}

ScopedAStatus Usb::queryPortStatus(int64_t in_transactionId) {
  std::vector<PortStatus> currentPortStatus;

  std::scoped_lock lock(mLock);
  if (mCallback) {
    Status status = getPortStatusHelper(currentPortStatus, mContaminantStatusPath);
    ScopedAStatus ret = mCallback->notifyPortStatusChange(currentPortStatus, status);
    if (!ret.isOk())
      ALOGE("notifyPortStatusChange error %s", ret.getDescription().c_str());

    ret = mCallback->notifyQueryPortStatus("all", Status::SUCCESS, in_transactionId);
    if (!ret.isOk())
      ALOGE("notifyQueryPortStatus error %s", ret.getDescription().c_str());
  } else {
    ALOGE("Notifying userspace skipped. Callback is NULL");
  }

  return ScopedAStatus::ok();
}

ScopedAStatus Usb::enableContaminantPresenceDetection(const std::string& portName,
            bool enable, int64_t in_transactionId) {
  std::vector<PortStatus> currentPortStatus;

  std::scoped_lock lock(mLock);
  if (mCallback && in_transactionId >= 0) {
    ScopedAStatus ret = mCallback->notifyContaminantEnabledStatus(portName,
		    true, Status::SUCCESS, in_transactionId);
    if (!ret.isOk())
      ALOGE("notifyContaminantEnabledStatus error %s", ret.getDescription().c_str());
  }

  ALOGI("Contaminant Presence Detection should always be in enable mode");

  return ScopedAStatus::ok();
}

static void handle_typec_uevent(Usb *usb, const char *msg)
{
  ALOGI("uevent received %s", msg);

  // if (std::regex_match(cp, std::regex("(add)(.*)(-partner)")))
  if (!strncmp(msg, "add@", 4) && !strncmp(msg + strlen(msg) - 8, "-partner", 8)) {
     ALOGI("partner added");
     std::scoped_lock lock(usb->mPartnerLock);
     usb->mPartnerUp = true;
     usb->mPartnerCV.notify_one();
  }

  std::string power_operation_mode;
  if (ReadFileToString("/sys/class/typec/port0/power_operation_mode", &power_operation_mode)) {
    power_operation_mode = Trim(power_operation_mode);
    if (usb->mPowerOpMode == power_operation_mode) {
      ALOGI("uevent recieved for same device %s", power_operation_mode.c_str());
    } else if(power_operation_mode == "usb_power_delivery") {
      ReadFileToString("/config/usb_gadget/g1/configs/b.1/MaxPower", &usb->mMaxPower);
      ReadFileToString("/config/usb_gadget/g1/configs/b.1/bmAttributes", &usb->mAttributes);
      WriteStringToFile("0", "/config/usb_gadget/g1/configs/b.1/MaxPower");
      WriteStringToFile("0xc0", "/config/usb_gadget/g1/configs/b.1/bmAttributes");
    } else {
      if(!usb->mMaxPower.empty()) {
        WriteStringToFile(usb->mMaxPower, "/config/usb_gadget/g1/configs/b.1/MaxPower");
        WriteStringToFile(usb->mAttributes, "/config/usb_gadget/g1/configs/b.1/bmAttributes");
        usb->mMaxPower = "";
      }
    }

    usb->mPowerOpMode = power_operation_mode;
  }

  std::vector<PortStatus> currentPortStatus;
  {
    std::scoped_lock lock(usb->mLock);
    if (usb->mCallback) {
      Status status = usb->getPortStatusHelper(currentPortStatus, usb->mContaminantStatusPath);
      ScopedAStatus ret = usb->mCallback->notifyPortStatusChange(currentPortStatus, status);
      if (!ret.isOk())
        ALOGE("notifyPortStatusChange error %s", ret.getDescription().c_str());
    }
  }

  //Role switch is not in progress and port is in disconnected state
  std::unique_lock role_lock(usb->mRoleSwitchLock, std::defer_lock);
  if (role_lock.try_lock()) {
    for (auto port : currentPortStatus) {
      DIR *dp = opendir(std::string("/sys/class/typec/" + port.portName + "-partner").c_str());
      if (dp == NULL) {
        switchToDrp(port.portName);
      } else {
        closedir(dp);
      }
    }
  }
}

// process POWER_SUPPLY uevent for contaminant presence
static void handle_psy_uevent(Usb *usb, const char *msg)
{
  std::vector<PortStatus> currentPortStatus;
  bool moisture_detected;
  std::string contaminantPresence;

  while (*msg) {
    if (!strncmp(msg, "POWER_SUPPLY_NAME=", 18)) {
      msg += 18;
      if (strcmp(msg, "usb")) // make sure we're looking at the correct uevent
        return;
      else
        break;
    }

    // advance to after the next \0
    while (*msg++) ;
  }

  // read moisture detection status from sysfs
  if (usb->mContaminantStatusPath.empty() ||
        !ReadFileToString(usb->mContaminantStatusPath, &contaminantPresence))
    return;

  moisture_detected = (contaminantPresence[0] == '1');

  if (usb->mContaminantPresence != moisture_detected) {
    usb->mContaminantPresence = moisture_detected;

    std::scoped_lock lock(usb->mLock);
    if (usb->mCallback) {
      Status status = usb->getPortStatusHelper(currentPortStatus, usb->mContaminantStatusPath);
      ScopedAStatus ret = usb->mCallback->notifyPortStatusChange(currentPortStatus, status);
      if (!ret.isOk())
        ALOGE("notifyPortStatusChange error %s", ret.getDescription().c_str());
    }
  }
}

static void uevent_event(const unique_fd &uevent_fd, struct Usb *usb) {
  constexpr int UEVENT_MSG_LEN = 2048;
  char msg[UEVENT_MSG_LEN + 2];
  int n, ret;
  std::string dwc3_sysfs;

  std::string gadgetName = GetProperty(USB_CONTROLLER_PROP, "");
  static std::regex add_regex("add@(/devices/platform/soc/.*dwc3/xhci-hcd\\.\\d\\.auto/"
                              "usb\\d/\\d-\\d(?:/[\\d\\.-]+)*)");
  static std::regex remove_regex("remove@((/devices/platform/soc/.*dwc3/xhci-hcd\\.\\d\\.auto/"
                              "usb\\d)/\\d-\\d(?:/[\\d\\.-]+)*)");
  static std::regex bind_regex("bind@(/devices/platform/soc/.*dwc3/xhci-hcd\\.\\d\\.auto/"
                               "usb\\d/\\d-\\d(?:/[\\d\\.-]+)*)/([^/]*:[^/]*)");
  static std::regex bus_reset_regex("change@(/devices/platform/soc/.*dwc3/xhci-hcd\\.\\d\\.auto/"
                               "usb\\d/\\d-\\d(?:/[\\d\\.-]+)*)/([^/]*:[^/]*)");
  static std::regex udc_regex("(add|remove)@/devices/platform/soc/.*/" + gadgetName +
                              "/udc/" + gadgetName);
  static std::regex offline_regex("offline@(/devices/platform/.*dwc3/xhci-hcd\\.\\d\\.auto/usb.*)");
  static std::regex backlight_regex("change@(/devices/platform/soc/.*qcom,mdss_mdp/backlight/panel0-backlight)");
  static std::regex dwc3_regex("\\/(\\w+.\\w+usb)/.*dwc3");

  n = uevent_kernel_multicast_recv(uevent_fd.get(), msg, UEVENT_MSG_LEN);
  if (n <= 0) return;
  if (n >= UEVENT_MSG_LEN) /* overflow -- discard */
    return;

  msg[n] = '\0';
  msg[n + 1] = '\0';

  std::cmatch match;
  if (strstr(msg, "typec/port")) {
    handle_typec_uevent(usb, msg);
  } else if (strstr(msg, "power_supply/usb")) {
    handle_psy_uevent(usb, msg + strlen(msg) + 1);
  } else if (std::regex_match(msg, match, add_regex)) {
    if (match.size() == 2) {
      std::csub_match submatch = match[1];
      checkUsbDeviceAutoSuspend("/sys" +  submatch.str());
    }
  } else if (!usb->mIgnoreWakeup && std::regex_match(msg, match, bind_regex)) {
    if (match.size() == 3) {
      std::csub_match devpath = match[1];
      std::csub_match intfpath = match[2];
      checkUsbInterfaceAutoSuspend("/sys" + devpath.str(), intfpath.str());
      checkUsbHIDDevice(usb, "/sys" + devpath.str());
    }
  } else if (std::regex_match(msg, match, udc_regex)) {
    if (!strncmp(msg, "add", 3)) {
      // Allow ADBD to resume its FFS monitor thread
      SetProperty(VENDOR_USB_ADB_DISABLED_PROP, "0");

      // In case ADB is not enabled, we need to manually re-bind the UDC to
      // ConfigFS since ADBD is not there to trigger it (sys.usb.ffs.ready=1)
      if (GetProperty("init.svc.adbd", "") != "running") {
        std::string udcName;
        int retry = 5;

        ALOGI("Binding UDC %s to ConfigFS", gadgetName.c_str());

        while (retry >= 0) {
          WriteStringToFile(gadgetName, "/config/usb_gadget/g1/UDC");
          ReadFileToString("/config/usb_gadget/g1/UDC", &udcName);
          if (Trim(udcName) == gadgetName)
            break;
          ALOGI("Retrying UDC bind for %s", gadgetName.c_str());
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
          retry--;
        }
      }
    } else {
      // When the UDC is removed, the ConfigFS gadget will no longer be
      // bound. If ADBD is running it would keep opening/writing to its
      // FFS EP0 file but since FUNCTIONFS_BIND doesn't happen it will
      // just keep repeating this in a 1 second retry loop. Each iteration
      // will re-trigger a ConfigFS UDC bind which will keep failing.
      // Setting this property stops ADBD from proceeding with the retry.

      DIR *dir = opendir(USB_UDC_PATH);
      bool udc_found = false;

      // enumerate /sys/class/udc/* to see if any UDCs still exist
      if (dir != NULL) {
	      struct dirent *entity;

	      while ((entity = readdir(dir))) {
		      if (entity->d_type == DT_LNK){
			      udc_found = true;
			      break;
		      }
	      }
	      closedir(dir);
      }

      if (!udc_found)
	      SetProperty(VENDOR_USB_ADB_DISABLED_PROP, "1");

    }
 } else if (std::regex_match(msg, match, bus_reset_regex)) {
    std::csub_match devpath = match[1];
    std::csub_match intfpath = match[2];

    ALOGI("Handling USB bus reset recovery");

    // Limit the recovery to when an audio device is connected directly to
    // the roothub.  A path reference is needed so other non-audio class
    // related devices don't trigger the disconnectMon. (unbind uevent occurs
    // after sysfs files are cleaned, can't check bInterfaceClass)
    usb->usbResetRecov = 1;
    ret = WriteStringToFile("0", "/sys" + devpath.str() + "/../authorized");
    if (!ret)
      ALOGI("unable to deauthorize device");
  } else if (std::regex_match(msg, match, remove_regex)) {
    std::csub_match devpath = match[1];
    std::csub_match parentpath = match[2];

    ALOGI("Disconnect received");
    if (usb->usbResetRecov) {
      usb->usbResetRecov = 0;
      //Allow interfaces to disconnect
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      WriteStringToFile("1", "/sys" + parentpath.str() + "/authorized");
    }
    removeUsbHIDDevice(usb, "/sys" + devpath.str());
  } else if (std::regex_match(msg, match, backlight_regex)) {
    handleDisplayEvent(usb);
  }
}

void Usb::uevent_work() {
  struct epoll_event ev;
  int nevents = 0;

  ALOGE("creating thread");

  unique_fd uevent_fd(uevent_open_socket(64 * 1024, true));

  if (uevent_fd < 0) {
    ALOGE("uevent_init: uevent_open_socket failed\n");
    return;
  }

  fcntl(uevent_fd.get(), F_SETFL, O_NONBLOCK);

  unique_fd epoll_fd(epoll_create(64));
  if (epoll_fd == -1) {
    ALOGE("epoll_create failed; errno=%d", errno);
    return;
  }

  ev.events = EPOLLIN;
  ev.data.fd = uevent_fd.get();
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, uevent_fd, &ev) == -1) {
    ALOGE("epoll_ctl adding uevent_fd failed; errno=%d", errno);
    return;
  }

  mEventFd = unique_fd(eventfd(0, 0));
  if (mEventFd == -1) {
    ALOGE("eventfd failed; errno=%d\n", errno);
    return;
  }

  ev.events = EPOLLIN;
  ev.data.fd = mEventFd.get();
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, mEventFd, &ev) == -1) {
    ALOGE("epoll_ctl adding event_fd failed; errno=%d", errno);
    mEventFd.reset();
    return;
  }

  bool running = true;
  while (running) {
    struct epoll_event events[64];

    nevents = epoll_wait(epoll_fd, events, 64, -1);
    if (nevents == -1) {
      if (errno == EINTR) continue;
      ALOGE("usb epoll_wait failed; errno=%d", errno);
      break;
    }

    for (int n = 0; n < nevents; ++n) {
      if (events[n].data.fd == uevent_fd.get()) {
        uevent_event(uevent_fd, this);
      } else {
        eventfd_t val;
        ALOGI("eventfd notified");
        if (eventfd_read(mEventFd, &val) == 0)
          running = false;
        break;
      }
    }
  }

  ALOGI("exiting worker thread");
  mEventFd.reset();
}

ScopedAStatus Usb::setCallback(const std::shared_ptr<IUsbCallback>& callback) {

  std::unique_lock lock(mLock);
  /*
   * When both the old callback and new callback values are NULL,
   * there is no need to spin off the worker thread.
   * When both the values are not NULL, we would already have a
   * worker thread running, so updating the callback object would
   * be suffice.
   */
  if ((mCallback == NULL && callback == NULL) ||
      (mCallback != NULL && callback != NULL)) {
    mCallback = callback;
    return ScopedAStatus::ok();
  }

  mCallback = callback;
  ALOGI("registering callback");

  // Kill the worker thread if the new callback is NULL.
  if (mCallback == NULL) {
    lock.unlock();
    eventfd_t val = 1;
    if (eventfd_write(mEventFd, val) == 0) {
      mPoll.join();
      ALOGI("worker thread destroyed");
    }
    return ScopedAStatus::ok();
  }

  if (mPoll.joinable()) {
    ALOGE("worker thread still running; detaching...");
    mPoll.detach();
  }

  /*
   * Create a background thread if the old callback value is NULL
   * and being updated with a new value.
   */
  mPoll = std::thread(&Usb::uevent_work, this);

  mIgnoreWakeup = checkUsbWakeupSupport();
  checkUsbInHostMode();

  /*
   * Check for the correct path to detect contaminant presence status
   * from the possible paths and use that to get contaminant
   * presence status when required.
   */
  if (access("/sys/class/power_supply/usb/moisture_detected", R_OK) == 0) {
    mContaminantStatusPath = "/sys/class/power_supply/usb/moisture_detected";
  } else if (access("/sys/class/qcom-battery/moisture_detection_status", R_OK) == 0) {
    mContaminantStatusPath = "/sys/class/qcom-battery/moisture_detection_status";
  } else if (access("/sys/bus/iio/devices/iio:device4/in_index_usb_moisture_detected_input", R_OK) == 0) {
    mContaminantStatusPath = "/sys/bus/iio/devices/iio:device4/in_index_usb_moisture_detected_input";
  } else {
    mContaminantStatusPath.clear();
  }

  ALOGI("Contamination presence path: %s", mContaminantStatusPath.c_str());

  return ScopedAStatus::ok();
}

static void checkUsbInHostMode() {
  std::string gadgetName = "/sys/bus/platform/devices/" + GetProperty(USB_CONTROLLER_PROP, "");
  DIR *gd = opendir(gadgetName.c_str());
  if (gd != NULL) {
    struct dirent *gadgetDir;
    while ((gadgetDir = readdir(gd))) {
      if (strstr(gadgetDir->d_name, "xhci-hcd")) {
        SetProperty(VENDOR_USB_ADB_DISABLED_PROP, "1");
        closedir(gd);
        return;
      }
    }
    closedir(gd);
  }
  SetProperty(VENDOR_USB_ADB_DISABLED_PROP, "0");
}

static bool checkUsbWakeupSupport() {
  std::string platdevices = "/sys/bus/platform/devices/";
  DIR *pd = opendir(platdevices.c_str());
  bool ignoreWakeup = true;

  if (pd != NULL) {
    struct dirent *platDir;
    while ((platDir = readdir(pd))) {
      std::string cname = platDir->d_name;
      /*
       * Scan for USB controller. Here "susb" takes care of both hsusb and ssusb.
       * Set mIgnoreWakeup based on the availability of 1st Controller's
       * power/wakeup node.
       */
      if (strstr(platDir->d_name, "susb")) {
        if (faccessat(dirfd(pd), (cname + "/power/wakeup").c_str(), F_OK, 0) < 0) {
          ignoreWakeup = true;
          ALOGI("PLATFORM DOESN'T SUPPORT WAKEUP");
        } else {
          ignoreWakeup = false;
        }
        break;
      }
    }
    closedir(pd);
  }

  if (ignoreWakeup)
    return true;

  /*
   * If wakeup is supported then scan for enumerated USB devices and
   * enable autosuspend.
   */
  std::string usbdevices = "/sys/bus/usb/devices/";
  DIR *dp = opendir(usbdevices.c_str());
  if (dp != NULL) {
    struct dirent *deviceDir;
    struct dirent *intfDir;
    DIR *ip;

    while ((deviceDir = readdir(dp))) {
      /*
       * Iterate over all the devices connected over USB while skipping
       * the interfaces.
       */
      if (deviceDir->d_type == DT_LNK && !strchr(deviceDir->d_name, ':')) {
        char buf[PATH_MAX];
        if (realpath((usbdevices + deviceDir->d_name).c_str(), buf)) {

          ip = opendir(buf);
          if (ip == NULL)
            continue;

          while ((intfDir = readdir(ip))) {
            // Scan over all the interfaces that are part of the device
            if (intfDir->d_type == DT_DIR && strchr(intfDir->d_name, ':')) {
              /*
               * If the autosuspend is successfully enabled, no need
               * to iterate over other interfaces.
               */
              if (checkUsbInterfaceAutoSuspend(buf, intfDir->d_name))
                break;
            }
          }
          closedir(ip);
        }
      }
    }
    closedir(dp);
  }

  return ignoreWakeup;
}

static int getDeviceInterfaceClass(const std::string& devicePath,
        const std::string &intf)
{
  std::string bInterfaceClass;
  int ret;

  ret = ReadFileToString(devicePath + "/" + intf + "/bInterfaceClass", &bInterfaceClass);
  if (!ret || bInterfaceClass.length() == 0)
    return -1;

  return std::stoi(bInterfaceClass, 0, 16);
}

/*
 * allow specific USB device idProduct and idVendor to auto suspend
 */
static bool canProductAutoSuspend(const std::string &deviceIdVendor,
    const std::string &deviceIdProduct) {
  if (deviceIdVendor == GOOGLE_USB_VENDOR_ID_STR &&
      deviceIdProduct == GOOGLE_USBC_35_ADAPTER_UNPLUGGED_ID_STR) {
    return true;
  }
  return false;
}

static bool canUsbDeviceAutoSuspend(const std::string &devicePath) {
  std::string deviceIdVendor;
  std::string deviceIdProduct;
  ReadFileToString(devicePath + "/idVendor", &deviceIdVendor);
  ReadFileToString(devicePath + "/idProduct", &deviceIdProduct);

  // deviceIdVendor and deviceIdProduct will be empty strings if ReadFileToString fails
  return canProductAutoSuspend(Trim(deviceIdVendor), Trim(deviceIdProduct));
}

/*
 * Checks to see if the added device is a USB device that contains only
 * USB HID interfaces.  For devices, such as USB headsets that have volume
 * control, avoid modifying the autosuspend timer and introducing the
 * logic to check for display brightness.
 */
static bool checkUsbHidOnly(const std::string& devicePath) {
  DIR *dp = opendir(devicePath.c_str());
  bool hidOnly = false;

  if (dp != NULL) {
    struct dirent *intfDir;
    int interfaceClass;
    int ret;

    while ((intfDir = readdir(dp))) {
      if (intfDir->d_type == DT_DIR && strchr(intfDir->d_name, ':')) {
        interfaceClass = getDeviceInterfaceClass(devicePath, intfDir->d_name);
        if (interfaceClass == USB_CLASS_HID) {
           hidOnly = true;
        } else {
           hidOnly = false;
           break;
        }
      }
    }
    closedir(dp);
  }

  return hidOnly;
}

/*
 * function to consume USB device plugin events (on receiving a
 * USB device path string), and enable autosupend on the USB device if
 * necessary.
 */
static void checkUsbDeviceAutoSuspend(const std::string& devicePath) {
  /*
   * Currently we only actively enable devices that should be autosuspended, and leave others
   * to the defualt.
   */
  if (canUsbDeviceAutoSuspend(devicePath)) {
    ALOGI("auto suspend usb device %s", devicePath.c_str());
    WriteStringToFile("auto", devicePath + "/power/control");
    WriteStringToFile("enabled", devicePath + "/power/wakeup");
  }
}

static bool checkUsbInterfaceAutoSuspend(const std::string& devicePath,
        const std::string &intf) {
  int interfaceClass;
  bool ret = false;

  interfaceClass = getDeviceInterfaceClass(devicePath, intf);

  // allow autosuspend for certain class devices
  switch (interfaceClass) {
    case USB_CLASS_AUDIO:
    case USB_CLASS_HUB:
      ALOGI("auto suspend usb interfaces %s", devicePath.c_str());
      ret = WriteStringToFile("auto", devicePath + "/power/control");
      if (!ret)
        break;

      ret = WriteStringToFile("enabled", devicePath + "/power/wakeup");
      break;
     default:
      ALOGI("usb interface does not support autosuspend %s", devicePath.c_str());

  }

  return ret;
}

ScopedAStatus Usb::limitPowerTransfer(const std::string& in_portName, bool in_limit,
    int64_t in_transactionId) {
  std::scoped_lock lock(mLock);
  aidl::android::hardware::usb::Status status = Status::SUCCESS;
  int ret;

  ALOGI("limitPowerTransfer in_limit: %d", in_limit);

  if (in_limit) {
    ret = WriteStringToFile("0", "/sys/class/qcom-battery/restrict_cur");
    if (!ret) {
      ALOGE("failed to limit USB charge current");
      status = Status::ERROR;
    }

    ret = WriteStringToFile("1", "/sys/class/qcom-battery/restrict_chg");
    if (!ret) {
      ALOGE("failed to limit USB charge current");
      status = Status::ERROR;
    }
  } else {
    ret = WriteStringToFile("0", "/sys/class/qcom-battery/restrict_chg");
    if (!ret) {
      ALOGE("failed to de-limit USB charge current");
      status = Status::ERROR;
    }
  }

  limitedPower = in_limit;

  if (mCallback && in_transactionId >= 0) {
    std::vector<PortStatus> currentPortStatus;
    ScopedAStatus ret = mCallback->notifyLimitPowerTransferStatus(in_portName,
        in_limit, status, in_transactionId);
    if (!ret.isOk())
      ALOGE("limitPowerTransfer error %s", ret.getDescription().c_str());

    status = getPortStatusHelper(currentPortStatus, mContaminantStatusPath);
    ret = mCallback->notifyPortStatusChange(currentPortStatus,
          status);
    if (!ret.isOk())
      ALOGE("queryPortStatus error %s", ret.getDescription().c_str());
  } else {
    ALOGE("Not notifying the userspace. Callback is not set");
  }

  return ScopedAStatus::ok();
}

ScopedAStatus Usb::resetUsbPort(const std::string& in_portName, int64_t in_transactionId) {
  std::scoped_lock lock(mLock);
  aidl::android::hardware::usb::Status status = Status::SUCCESS;
  std::string dwcDriver = "";
  std::string mode;
  int ret = -1;

  ALOGE("resetUsbPort %s", in_portName.c_str());

  getUsbControllerPath(dwcDriver);
  if (dwcDriver == "") {
    ALOGE("resetUsbPort unable to find dwc device");
    status = Status::ERROR;
    goto out;
  }

  //Cache current mode for re-writing after the reset
  ret = ReadFileToString(dwcDriver + "mode", &mode);
  if (!ret) {
    status = Status::ERROR;
    goto out;
  }

  //Don't handle the port reset if we are disconnected
  if (mode == "none")
    goto out;

  //Toggle mode sysfs to trigger disconnect/connect sequence
  ret = WriteStringToFile("none", dwcDriver + "mode");
  if (!ret) {
    status = Status::ERROR;
    goto out;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  ret = WriteStringToFile(mode.c_str(), dwcDriver + "mode");
  if (!ret) {
    status = Status::ERROR;
    goto out;
  }

out:
  if (mCallback) {
    ScopedAStatus stat = mCallback->notifyResetUsbPortStatus(in_portName,
        status, in_transactionId);
    if (!stat.isOk())
      ALOGE("notifyResetUsbPortStatus error %s", stat.getDescription().c_str());
  } else {
    ALOGE("Not notifying the userspace. Callback is not set");
  }

  return ScopedAStatus::ok();
}

}  // namespace usb
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#ifndef __LIBFUZZON_LOCAL__
int main() {
    using ::aidl::android::hardware::usb::Usb;

    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Usb> usb = ndk::SharedRefBase::make<Usb>();

    const std::string instance = std::string(Usb::descriptor) + "/default";
    binder_status_t status = AServiceManager_addService(usb->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
    return -1; // Should never be reached
}

#else

#include <fuzzbinder/libbinder_ndk_driver.h>
#include <fuzzer/FuzzedDataProvider.h>

std::shared_ptr<::aidl::android::hardware::usb::Usb> service;

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    service = ndk::SharedRefBase::make<::aidl::android::hardware::usb::Usb>();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if( service == nullptr ) return 0;

    FuzzedDataProvider provider(data, size);
    android::fuzzService(service->asBinder().get(),std::move(provider));

    return 0;
}

#endif
