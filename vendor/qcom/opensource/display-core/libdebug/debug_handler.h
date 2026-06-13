/*
* Copyright (c) 2018, 2020 The Linux Foundation. All rights reserved.
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

#ifndef __DEBUG_HANDLER_H__
#define __DEBUG_HANDLER_H__

#include <bitset>

#define DLOG(method, format, ...) \
  display::DebugHandler::Get()->method(__CLASS__ "::%s: " format, __FUNCTION__, ##__VA_ARGS__)

#define DLOG_IF(tag, method, format, ...) \
  if (display::DebugHandler::GetLogMask()[tag]) { \
    DLOG(method, format, ##__VA_ARGS__); \
  }

#define DLOGE_IF(tag, format, ...) DLOG_IF(tag, Error, format, ##__VA_ARGS__)
#define DLOGW_IF(tag, format, ...) DLOG_IF(tag, Warning, format, ##__VA_ARGS__)
#define DLOGI_IF(tag, format, ...) DLOG_IF(tag, Info, format, ##__VA_ARGS__)
#define DLOGD_IF(tag, format, ...) DLOG_IF(tag, Debug, format, ##__VA_ARGS__)
#define DLOGV_IF(tag, format, ...) DLOG_IF(tag, Verbose, format, ##__VA_ARGS__)

#define DLOGE(format, ...) DLOG(Error, format, ##__VA_ARGS__)
#define DLOGW(format, ...) DLOG(Warning, format, ##__VA_ARGS__)
#define DLOGI(format, ...) DLOG(Info, format, ##__VA_ARGS__)
#define DLOGD(format, ...) DLOG(Debug, format, ##__VA_ARGS__)
#define DLOGV(format, ...) DLOG(Verbose, format, ##__VA_ARGS__)

#define DTRACE_BEGIN(custom_string) display::DebugHandler::Get()->BeginTrace( \
                                          __CLASS__, __FUNCTION__, custom_string)
#define DTRACE_END() display::DebugHandler::Get()->EndTrace()
#define DTRACE_SCOPED() display::ScopeTracer <display::DebugHandler> \
                                          scope_tracer(__CLASS__, __FUNCTION__)

namespace display {

class DebugHandler {
 public:
  // __format__(printf hints the compiler to validate format specifiers vs arguments provided.
  virtual void Error(const char *format, ...) __attribute__ ((__format__(printf, 2, 3))) = 0;
  virtual void Warning(const char *format, ...) __attribute__ ((__format__(printf, 2, 3))) = 0;
  virtual void Info(const char *format, ...) __attribute__ ((__format__(printf, 2, 3))) = 0;
  virtual void Debug(const char *format, ...) __attribute__ ((__format__(printf, 2, 3))) = 0;
  virtual void Verbose(const char *format, ...) __attribute__ ((__format__(printf, 2, 3))) = 0;
  virtual void BeginTrace(const char *class_name, const char *function_name,
                          const char *custom_string) = 0;
  virtual void EndTrace() = 0;
  virtual int GetProperty(const char *property_name, int *value) = 0;
  virtual int GetProperty(const char *property_name, char *value) = 0;

  static inline DebugHandler *Get() { return debug_handler_; }
  static void Set(DebugHandler *debug_handler);
  static inline std::bitset<32> & GetLogMask() { return log_mask_; }
  static void SetLogMask(const std::bitset<32> &log_mask) { log_mask_ = log_mask; }

 protected:
  virtual ~DebugHandler() { }

 private:
  static DebugHandler *debug_handler_;
  static std::bitset<32> log_mask_;
};

template <class T>
class ScopeTracer {
 public:
  ScopeTracer(const char *class_name, const char *function_name) {
    T::Get()->BeginTrace(class_name, function_name, "");
  }

  ~ScopeTracer() { T::Get()->EndTrace(); }
};

}  // namespace display

#endif  // __DEBUG_HANDLER_H__
