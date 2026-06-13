/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

//#define LOG_NDEBUG 0
#define LOG_TAG "PAL: SignalHandler"

#include <unistd.h>
#include <log/log.h>
#include <chrono>
#include "SignalHandler.h"

#ifdef _ANDROID_
#include <utils/ProcessCallStack.h>
#include <cutils/android_filesystem_config.h>
#endif

std::mutex SignalHandler::sDefaultSigMapLock;
std::unordered_map<int, std::shared_ptr<struct sigaction>> SignalHandler::sDefaultSigMap;
std::function<void(int, pid_t, uid_t)> SignalHandler::sClientCb;
std::mutex SignalHandler::sAsyncRegisterLock;
std::future<void> SignalHandler::sAsyncHandle;
bool SignalHandler::sBuildDebuggable;

// static
void SignalHandler::asyncRegister(int signal) {
    std::lock_guard<std::mutex> lock(sAsyncRegisterLock);
    sigset_t pendingSigMask;
    uint32_t tries = kDefaultSignalPendingTries;
    // Delay registration to let default signal handler complete
    do {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(kDefaultRegistrationDelayMs));
        sigpending(&pendingSigMask);
        --tries;
    } while (tries > 0 && sigismember(&pendingSigMask, signal) == 1);

    // Register custom handler only if signal is not pending
    if (!sigismember(&pendingSigMask, signal)) {
        std::vector<int> signals({ signal });
        getInstance()->registerSignalHandler(signals);
    }
}

// static
void SignalHandler::setClientCallback(std::function<void(int, pid_t, uid_t)> cb) {
    sClientCb = cb;
}

// static
std::shared_ptr<SignalHandler> SignalHandler::getInstance() {
    static std::shared_ptr<SignalHandler> instance(new SignalHandler());
    return instance;
}

// static
void SignalHandler::invokeDefaultHandler(std::shared_ptr<struct sigaction> sAct,
            int code, struct siginfo *si, void *sc) {
    ALOGE("%s: invoke default handler for signal %d", __func__, code);
    // Remove custom handler so that default handler is invoked
    sigaction(code, sAct.get(), NULL);

    int status = 0;
    if (si->si_code == SI_QUEUE) {
        ALOGE("signal %d (%s), code -1 "
              "(SI_QUEUE from pid %d, uid %d)",
              code, sigToName.at(code).c_str(),
              si->si_pid, si->si_uid);
        status = sigqueue(getpid(), code, si->si_value);
#ifdef _ANDROID_
        if(isBuildDebuggable() && si->si_uid == AID_AUDIOSERVER) {
            std::string prefix = "audioserver_" + std::to_string(si->si_pid) + " ";
            android::ProcessCallStack pcs;
            pcs.update();
            pcs.log(LOG_TAG, ANDROID_LOG_FATAL, prefix.c_str());
        }
#endif
    } else {
        status = raise(code);
    }

    if (status < 0) {
        ALOGW("%s: Sending signal %d failed with error %d",
                  __func__, code, errno);
    }

    // Register custom handler back asynchronously
    sAsyncHandle = std::async(std::launch::async, SignalHandler::asyncRegister, code);
}

// static
void SignalHandler::customSignalHandler(
            int code, struct siginfo *si, void *sc) {
    ALOGV("%s: enter", __func__);
    std::lock_guard<std::mutex> lock(sDefaultSigMapLock);
    if (sClientCb) {
        sClientCb(code, si->si_pid, si->si_uid);
    }
    // Invoke default handler
    auto it = sDefaultSigMap.find(code);
    if (it != sDefaultSigMap.end()) {
        invokeDefaultHandler(it->second, code, si, sc);
    }
}

// static
std::vector<int> SignalHandler::getRegisteredSignals() {
    std::vector<int> registeredSignals;
    std::lock_guard<std::mutex> lock(sDefaultSigMapLock);
    for (const auto& element : sDefaultSigMap) {
        registeredSignals.push_back(element.first);
    }
    return registeredSignals;
}

void SignalHandler::registerSignalHandler(std::vector<int> signalsToRegister) {
    ALOGV("%s: enter", __func__);
    struct sigaction regAction = {};
    regAction.sa_sigaction = customSignalHandler;
    regAction.sa_flags = SA_SIGINFO | SA_NODEFER;
    std::lock_guard<std::mutex> lock(sDefaultSigMapLock);
    for (int signal : signalsToRegister) {
        ALOGV("%s: register signal %d", __func__, signal);
        auto oldSigAction = std::make_shared<struct sigaction>();
        if (sigaction(signal, &regAction, oldSigAction.get()) < 0) {
           ALOGW("%s: Failed to register handler with error code %d for signal %d",
                 __func__, errno, signal);
        }
        sDefaultSigMap.emplace(signal, oldSigAction);
    }
}
