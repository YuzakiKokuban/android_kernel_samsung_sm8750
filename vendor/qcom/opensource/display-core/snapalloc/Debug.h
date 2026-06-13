// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <display_properties.h>
#include <errno.h>
#include <string>

#include "debug_callback_intf.h"

#define PROPERTY_VALUE_MAX 255

using ::sdm::DebugCallbackIntf;

#define DLOG(type, format, ...) ::snapalloc::Debug::GetInstance()->Log(type, format, ##__VA_ARGS__)

#define DLOG_IF(tag, type, format, ...) \
  if (tag) {                            \
    DLOG(type, format, ##__VA_ARGS__);  \
  }

#define DLOGE(format, ...) DLOG(sdm::DebugLogType::ERROR, format, ##__VA_ARGS__)
#define DLOGW(format, ...) DLOG(sdm::DebugLogType::WARNING, format, ##__VA_ARGS__)
#define DLOGI(format, ...) DLOG(sdm::DebugLogType::INFO, format, ##__VA_ARGS__)
#define DLOGD(format, ...) DLOG(sdm::DebugLogType::DEBUG, format, ##__VA_ARGS__)
#define DLOGV(format, ...) DLOG(sdm::DebugLogType::VERBOSE, format, ##__VA_ARGS__)

#define DLOGE_IF(tag, format, ...) DLOG_IF(tag, sdm::DebugLogType::ERROR, format, ##__VA_ARGS__)
#define DLOGW_IF(tag, format, ...) DLOG_IF(tag, sdm::DebugLogType::WARNING, format, ##__VA_ARGS__)
#define DLOGI_IF(tag, format, ...) DLOG_IF(tag, sdm::DebugLogType::INFO, format, ##__VA_ARGS__)
#define DLOGD_IF(tag, format, ...) DLOG_IF(tag, sdm::DebugLogType::DEBUG, format, ##__VA_ARGS__)
#define DLOGV_IF(tag, format, ...) DLOG_IF(tag, sdm::DebugLogType::VERBOSE, format, ##__VA_ARGS__)

namespace snapalloc {
class Debug {
 public:
  static Debug *GetInstance();
  void RegisterDebugCallback(DebugCallbackIntf *cb);
  int GetProperty(const char *property_name, char *value);
  int GetProperty(const char *property_name, int *value);
  bool IsAhardwareBufferDisabled();
  bool IsUBWCDisabled();
  bool IsSecurePreviewBufferFormatEnabled(std::string *secure_preview_buffer_format);
  bool IsSecurePreviewOnlyEnabled();
  bool UseDMABufHeaps();
  bool UseSystemHeapForSensors();
  bool HwSupportsUBWCP();
  void Log(sdm::DebugLogType type, const char *fmt, ...);
  bool IsDebugLoggingEnabled();

 private:
  DebugCallbackIntf *debug_callback_ = nullptr;
  static Debug *debug_;
};
}  // namespace snapalloc
#endif  // __DEBUG_H__