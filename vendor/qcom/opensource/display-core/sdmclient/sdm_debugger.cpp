/*
 * Copyright (c) 2014 - 2020, The Linux Foundation. All rights reserved.
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
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the
 * following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <display_properties.h>
#include <utils/constants.h>
#include <cstdarg>

#include "sdm_debugger.h"

#define SDM_LOG_TAG "SDM"

namespace sdm {

SDMDebugHandler SDMDebugHandler::debug_handler_;

SDMDebugHandler::SDMDebugHandler() {
  DebugHandler::Set(SDMDebugHandler::Get());
}

void SDMDebugHandler::SetDebugCallback(DebugCallbackIntf *debug) {
  debug_handler_.Register(debug);
}

void SDMDebugHandler::ATRACE_INT(const char *custom_string, const int bit) {
  debug_handler_.ATrace(custom_string, bit);
}

void SDMDebugHandler::DebugAll(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_ = 0x7FFFFFFF;
    /*if (verbose_level) {
      // Enable verbose scalar logs only when explicitly enabled
      debug_handler_.log_mask_[kTagScalar] = 0;
    }*/
    debug_handler_.verbose_level_ = 1;
  } else {
    debug_handler_.log_mask_ = 0x1; // kTagNone should always be printed.
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMDebugHandler::DebugResources(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagResources] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagResources] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMDebugHandler::DebugStrategy(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagStrategy] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagStrategy] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMDebugHandler::DebugIWE(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagIWE] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagIWE] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMDebugHandler::DebugWbUsage(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagWbUsage] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagWbUsage] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMDebugHandler::DebugCompManager(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagCompManager] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagCompManager] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMDebugHandler::DebugDriverConfig(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagDriverConfig] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagDriverConfig] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMDebugHandler::DebugRotator(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagRotator] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagRotator] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMDebugHandler::DebugScalar(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagScalar] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagScalar] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMDebugHandler::DebugQdcm(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagQDCM] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagQDCM] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMDebugHandler::DebugClient(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagClient] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagClient] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMDebugHandler::DebugDisplay(bool enable, int verbose_level) {
  if (enable) {
    debug_handler_.log_mask_[kTagDisplay] = 1;
    debug_handler_.verbose_level_ = verbose_level;
  } else {
    debug_handler_.log_mask_[kTagDisplay] = 0;
    debug_handler_.verbose_level_ = 0;
  }

  DebugHandler::SetLogMask(debug_handler_.log_mask_);
}

void SDMDebugHandler::DebugQos(bool enable, int verbose_level) {
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

void SDMDebugHandler::Error(const char *fmt, ...) {
  std::va_list args;
  va_start(args, fmt);
  if (debug_callback_) {
    debug_callback_->Log(DebugLogType::ERROR, SDM_LOG_TAG, fmt, args);
  }
}

void SDMDebugHandler::Warning(const char *fmt, ...) {
  std::va_list args;
  va_start(args, fmt);
  if (debug_callback_) {
    debug_callback_->Log(DebugLogType::WARNING, SDM_LOG_TAG, fmt, args);
  }
}

void SDMDebugHandler::Info(const char *fmt, ...) {
  std::va_list args;
  va_start(args, fmt);
  if (debug_callback_) {
    debug_callback_->Log(DebugLogType::INFO, SDM_LOG_TAG, fmt, args);
  }
}

void SDMDebugHandler::Debug(const char *fmt, ...) {
  std::va_list args;
  va_start(args, fmt);
  if (debug_callback_) {
    debug_callback_->Log(DebugLogType::DEBUG, SDM_LOG_TAG, fmt, args);
  }
}

void SDMDebugHandler::Verbose(const char *fmt, ...) {
  std::va_list args;
  va_start(args, fmt);
  if (debug_handler_.verbose_level_ && debug_callback_) {
    debug_callback_->Log(DebugLogType::VERBOSE, SDM_LOG_TAG, fmt, args);
  }
}

void SDMDebugHandler::BeginTrace(const char *class_name,
                                 const char *function_name,
                                 const char *custom_string) {
  if (debug_callback_) {
    debug_callback_->BeginTrace(class_name, function_name, custom_string);
  }
}

void SDMDebugHandler::EndTrace() {
  if (debug_callback_) {
    debug_callback_->EndTrace();
  }
}

int SDMDebugHandler::GetIdleTimeoutMs() {
  int value = IDLE_TIMEOUT_DEFAULT_MS;
  debug_handler_.GetProperty(IDLE_TIME_PROP, &value);

  return value;
}

int SDMDebugHandler::GetProperty(const char *property_name, int *value) {
  if (debug_callback_) {
    return debug_callback_->GetProperty(property_name, value);
  } else {
    return kErrorNotSupported;
  }
}

int SDMDebugHandler::GetProperty(const char *property_name, char *value) {
  if (debug_callback_) {
    return debug_callback_->GetProperty(property_name, value);
  } else {
    return kErrorNotSupported;
  }
}

void SDMDebugHandler::Register(DebugCallbackIntf *dbg) {
  debug_callback_ = dbg;
}

void SDMDebugHandler::ATrace(const char *custom_string, const int bit) {
  if (debug_callback_) {
    debug_callback_->ATrace(custom_string, bit);
  }
}

} // namespace sdm
