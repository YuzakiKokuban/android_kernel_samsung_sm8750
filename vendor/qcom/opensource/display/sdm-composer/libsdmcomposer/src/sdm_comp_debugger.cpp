/*
* Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
*
* Changes from Qualcomm Innovation Center are provided under the following license:
* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <stdio.h>
#include <stdarg.h>
#ifdef ANDROID
#include <log/log.h>
#endif
#include <utils/constants.h>
#include "sdm_comp_debugger.h"
#include <string>
#include <cstring>

using std::string;

namespace sdm {

SDMCompDebugHandler SDMCompDebugHandler::debug_handler_;

SDMCompDebugHandler::SDMCompDebugHandler() {
  DebugHandler::Set(SDMCompDebugHandler::Get());
  PropertyParserInterface::Create(&prop_parser_intf_);
}

void SDMCompDebugHandler::DebugAll(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_ = 0x7FFFFFFF;
    if (verbose_level) {
      // Enable verbose scalar logs only when explicitly enabled
      debug_handler_.log_mask_[kTagScalar] = 0;
    }
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_ = 0x1;   // kTagNone should always be printed.
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMCompDebugHandler::DebugResources(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagResources] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagResources] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMCompDebugHandler::DebugStrategy(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagStrategy] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagStrategy] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMCompDebugHandler::DebugCompManager(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagCompManager] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagCompManager] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMCompDebugHandler::DebugDriverConfig(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagDriverConfig] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagDriverConfig] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMCompDebugHandler::DebugRotator(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagRotator] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagRotator] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMCompDebugHandler::DebugScalar(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagScalar] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagScalar] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMCompDebugHandler::DebugQdcm(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagQDCM] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagQDCM] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMCompDebugHandler::DebugClient(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagClient] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagClient] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMCompDebugHandler::DebugDisplay(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagDisplay] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagDisplay] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMCompDebugHandler::DebugQos(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagQOSClient] = 1;
    debug_handler_.log_mask_[kTagQOSImpl] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagQOSClient] = 0;
    debug_handler_.log_mask_[kTagQOSImpl] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMCompDebugHandler::Error(const char *format, ...) {
  va_list list;
  va_start(list, format);
#ifdef ANDROID
  __android_log_vprint(ANDROID_LOG_ERROR, LOG_TAG, format, list);
#else
  char buffer[1024];
  vsnprintf (buffer, 1024, format, list);
  // TODO(user): print timestamp, process id and thread id
  printf("E %s %s \n", LOG_TAG, buffer);
#endif
  va_end(list);
}

void SDMCompDebugHandler::Warning(const char *format, ...) {
  va_list list;
  va_start(list, format);
#ifdef ANDROID
  __android_log_vprint(ANDROID_LOG_WARN, LOG_TAG, format, list);
#else
  char buffer[1024];
  vsnprintf (buffer, 1024, format, list);
  printf("W %s %s \n", LOG_TAG, buffer);
#endif
  va_end(list);
}

void SDMCompDebugHandler::Info(const char *format, ...) {
  va_list list;
  va_start(list, format);
#ifdef ANDROID
  __android_log_vprint(ANDROID_LOG_INFO, LOG_TAG, format, list);
#else
  char buffer[1024];
  vsnprintf (buffer, 1024, format, list);
  printf("I %s %s \n", LOG_TAG, buffer);
#endif
  va_end(list);
}

void SDMCompDebugHandler::Debug(const char *format, ...) {
  va_list list;
  va_start(list, format);
#ifdef ANDROID
  __android_log_vprint(ANDROID_LOG_DEBUG, LOG_TAG, format, list);
#else
  char buffer[1024];
  vsnprintf (buffer, 1024, format, list);
  printf("D %s %s \n", LOG_TAG, buffer);
#endif
  va_end(list);
}

void SDMCompDebugHandler::Verbose(const char *format, ...) {
  if (debug_handler_.verbose_level_) {
    va_list list;
    va_start(list, format);
#ifdef ANDROID
    __android_log_vprint(ANDROID_LOG_VERBOSE, LOG_TAG, format, list);
#else
    char buffer[1024];
    vsnprintf (buffer, 1024, format, list);
    printf("V %s %s \n", LOG_TAG, buffer);
#endif
    va_end(list);
  }
}

void SDMCompDebugHandler::BeginTrace(const char *class_name, const char *function_name,
                                     const char *custom_string) {
}

void SDMCompDebugHandler::EndTrace() {
}

int  SDMCompDebugHandler::GetIdleTimeoutMs() {
  return IDLE_TIMEOUT_DEFAULT_MS;
}

int SDMCompDebugHandler::GetProperty(const char *property_name, int *value) {
  if (!property_name || !value)
    return kErrorNotSupported;

  if (prop_parser_intf_) {
    return prop_parser_intf_->GetProperty(property_name, value);
  }
  auto it = properties_map_.find(property_name);
  if (it != properties_map_.end())
    *value = std::stoi(it->second);
  else
    return kErrorUndefined;

  return kErrorNone;
}

int SDMCompDebugHandler::GetProperty(const char *property_name, char *value) {
  if (!property_name || !value)
    return kErrorNotSupported;

  if (prop_parser_intf_) {
    return prop_parser_intf_->GetProperty(property_name, value);
  }

  auto it = properties_map_.find(property_name);
  if (it != properties_map_.end())
    std::snprintf(value, it->second.size(), "%s", it->second.c_str());
  else
    return kErrorUndefined;

  return kErrorNone;
}

int SDMCompDebugHandler::SetProperty(const char *property_name, const char *value) {
  if (!property_name || !value) {
    Error("SetProperty: failed to set property_name :%s :: %s\n", property_name, value);
    return kErrorNotSupported;
  }

  properties_map_.emplace(std::string(property_name), std::string(value));
  return kErrorNone;
}

}  // namespace sdm

