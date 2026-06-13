/*
 * Copyright (c) 2014 - 2018, 2020 The Linux Foundation. All rights reserved.
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
#ifndef __SDM_DEBUGGER_H__
#define __SDM_DEBUGGER_H__

#include <bitset>
#include <core/sdm_types.h>
#include <debug_handler.h>

#include "debug_callback_intf.h"

namespace sdm {

using display::DebugHandler;

class SDMDebugHandler : public DebugHandler {
public:
  SDMDebugHandler();
  static inline DebugHandler *Get() { return &debug_handler_; }
  static const char *DumpDir() { return "/data/vendor/display"; }

  // static function to register debug callback
  static void SetDebugCallback(DebugCallbackIntf *debug);

  // static function to atrace through debug callback
  static void ATRACE_INT(const char *custom_string, const int bit);

  static void DebugAll(bool enable, int verbose_level);
  static void DebugResources(bool enable, int verbose_level);
  static void DebugStrategy(bool enable, int verbose_level);
  static void DebugCompManager(bool enable, int verbose_level);
  static void DebugDriverConfig(bool enable, int verbose_level);
  static void DebugRotator(bool enable, int verbose_level);
  static void DebugScalar(bool enable, int verbose_level);
  static void DebugQdcm(bool enable, int verbose_level);
  static void DebugClient(bool enable, int verbose_level);
  static void DebugQos(bool enable, int verbose_level);
  static void DebugDisplay(bool enable, int verbose_level);
  static int GetIdleTimeoutMs();
  static void DebugIWE(bool enable, int verbose_level);
  static void DebugWbUsage(bool enable, int verbose_level);

  virtual void Error(const char *fmt, ...)
      __attribute__((format(printf, 2, 3)));
  virtual void Warning(const char *fmt, ...)
      __attribute__((format(printf, 2, 3)));
  virtual void Info(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
  virtual void Debug(const char *fmt, ...)
      __attribute__((format(printf, 2, 3)));
  virtual void Verbose(const char *fmt, ...)
      __attribute__((format(printf, 2, 3)));
  virtual void BeginTrace(const char *class_name, const char *function_name,
                          const char *custom_string);
  virtual void EndTrace();
  virtual int GetProperty(const char *property_name, int *value);
  virtual int GetProperty(const char *property_name, char *value);
  virtual void ATrace(const char *custom_string, const int bit);
  void Register(DebugCallbackIntf *dbg);

 private:
  static SDMDebugHandler debug_handler_;
  std::bitset<32> log_mask_;
  int32_t verbose_level_;

  DebugCallbackIntf *debug_callback_ = nullptr;
};

} // namespace sdm

#endif // __SDM_DEBUGGER_H__
