/*
* Copyright (c) 2018, The Linux Foundation. All rights reserved.
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

#include "debug_handler.h"

namespace display {

// By default, drop any log messages/traces. It need to be overridden by client.
class DefaultDebugHandler : public DebugHandler {
 public:
  virtual void Error(const char *, ...) { }
  virtual void Warning(const char *, ...) { }
  virtual void Info(const char *, ...) { }
  virtual void Debug(const char *, ...) { }
  virtual void Verbose(const char *, ...) { }
  virtual void BeginTrace(const char *, const char *, const char *) { }
  virtual void EndTrace() { }
  virtual int GetProperty(const char *, int *) { return -1; }
  virtual int GetProperty(const char *, char *) { return -1; }
};

DefaultDebugHandler g_default_debug_handler;
DebugHandler * DebugHandler::debug_handler_ = &g_default_debug_handler;
std::bitset<32> DebugHandler::log_mask_ = 0x1;  // Always print logs tagged with value 0

void DebugHandler::Set(DebugHandler *debug_handler) {
  if (debug_handler) {
    debug_handler_ = debug_handler;
  } else {
    debug_handler_ = &g_default_debug_handler;
  }
}

}  // namespace display
