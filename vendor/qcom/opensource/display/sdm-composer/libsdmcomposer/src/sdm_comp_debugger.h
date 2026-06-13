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

#ifndef __SDM_COMP_DEBUGGER_H__
#define __SDM_COMP_DEBUGGER_H__

#include <core/sdm_types.h>
#include <debug_handler.h>

#include "property_parser_interface.h"

#include <bitset>
#include <map>

namespace sdm {

using display::DebugHandler;

class SDMCompDebugHandler : public DebugHandler {
 public:
  SDMCompDebugHandler();
  static inline DebugHandler* Get() { return &debug_handler_; }

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
  static void DebugAllocator(bool enable, int verbose_level);
  static int  GetIdleTimeoutMs();

  virtual void Error(const char *format, ...);
  virtual void Warning(const char *format, ...);
  virtual void Info(const char *format, ...);
  virtual void Debug(const char *format, ...);
  virtual void Verbose(const char *format, ...);
  virtual void BeginTrace(const char *class_name, const char *function_name,
                          const char *custom_string);
  virtual void EndTrace();
  virtual int GetProperty(const char *property_name, int *value);
  virtual int GetProperty(const char *property_name, char *value);
  int SetProperty(const char *property_name, const char *value);

 private:
  static SDMCompDebugHandler debug_handler_;
  std::bitset<32> log_mask_;
  int32_t verbose_level_;
  std::map<std::string, std::string> properties_map_;
  PropertyParserInterface *prop_parser_intf_ = nullptr;
};

}  // namespace sdm

#endif  // __SDM_COMP_DEBUGGER_H__

