/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __DEBUG_CALLBACK_INTF__
#define __DEBUG_CALLBACK_INTF__

#include <cstdarg>

/*
 This interface has all the functions necessary for debug info, such as
 logs, traces and debug properties. Compositors must provide a concrete
 implementation of this class on Init
*/

namespace sdm {

enum DebugLogType {
  ERROR,
  WARNING,
  INFO,
  DEBUG,
  VERBOSE,
};

class DebugCallbackIntf {
public:
  virtual ~DebugCallbackIntf() {}
  virtual void Log(DebugLogType type, const char *log_tag, const char *fmt,
                   std::va_list &args) = 0;
  virtual int GetProperty(const char *property_name, int *value) = 0;
  virtual int GetProperty(const char *property_name, char *value) = 0;
  virtual void BeginTrace(const char *class_name, const char *function_name,
                          const char *custom_string) = 0;
  virtual void EndTrace() = 0;
  virtual void ATrace(const char *custom_string, const int bit) = 0;
};

} // namespace sdm

#endif // __DEBUG_CALLBACK_INTF__