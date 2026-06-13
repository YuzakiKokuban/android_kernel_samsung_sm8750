/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <vector>
#include <unordered_map>
#include <signal.h>
#include <map>

#ifndef __SIGRTMIN
#define __SIGRTMIN 32
#endif

static const constexpr int DEBUGGER_SIGNAL = (__SIGRTMIN + 3);
static const constexpr uint32_t kDefaultSignalPendingTries = 10;
static const constexpr uint32_t kDefaultRegistrationDelayMs = 500;

static const std::map<uint32_t, std::string> sigToName {
    {DEBUGGER_SIGNAL,   std::string{"<debuggerd signal>"}},
    {SIGABRT,  std::string{"SIGABRT"}},
    {SIGSEGV,  std::string{"SIGSEGV"}},
};


struct SignalHandler {
    static std::shared_ptr<SignalHandler> getInstance();
    static void setClientCallback(std::function<void(int, pid_t, uid_t)> cb);
    static void asyncRegister(int signal);
    static void invokeDefaultHandler(std::shared_ptr<struct sigaction> sAct,
                              int code, struct siginfo *si, void *sc);
    static void customSignalHandler(int code, struct siginfo *si, void *sc);
    static std::vector<int> getRegisteredSignals();
    void registerSignalHandler(std::vector<int> signalsToRegister);
    static void setBuildDebuggable(bool debuggable) { sBuildDebuggable = debuggable;}
    static bool isBuildDebuggable() { return sBuildDebuggable;}
    static std::mutex sDefaultSigMapLock;
    static std::unordered_map<int, std::shared_ptr<struct sigaction>> sDefaultSigMap;
    static std::function<void(int, pid_t, uid_t)> sClientCb;
    static std::mutex sAsyncRegisterLock;
    static std::future<void> sAsyncHandle;
    static bool sBuildDebuggable;
};

#endif
