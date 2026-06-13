/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <algorithm>
#include <memory>
#include <unordered_map>

#include <android-base/logging.h>
#include <android-base/thread_annotations.h>

#include "VoiceProcessingContext.h"
#include "VoiceProcessingTypes.h"

namespace aidl::qti::effects {
using VoiceProcessingContextList = std::vector<std::shared_ptr<VoiceProcessingContext>>;

class GlobalVoiceProcessingSession {
  public:
    static GlobalVoiceProcessingSession& getSession() {
        static GlobalVoiceProcessingSession instance;
        return instance;
    }

    static bool findEffectTypeInList(VoiceProcessingContextList& list,
                                     const VoiceProcessingType& type, bool remove = false) {
        auto itr = std::find_if(list.begin(), list.end(),
                                [type](const std::shared_ptr<VoiceProcessingContext>& obj) {
                                    return obj->getVoiceProcessingType() == type;
                                });
        if (itr == list.end()) {
            return false;
        }
        if (remove) {
            list.erase(itr);
        }
        return true;
    }

    std::shared_ptr<VoiceProcessingContext> createSession(const VoiceProcessingType& type,
                                                          const Parameter::Common& common,
                                                          bool processData) {
        int sessionId = common.session;
        LOG(DEBUG) << __func__ << type << " with sessionId " << sessionId;
        std::lock_guard lg(mMutex);

        if (mSessionMap.count(sessionId)) {
            LOG(INFO) << __func__ << type << " new effect in existing session " << sessionId;
            // How about same effect in same session? What to do there?
        } else {
            LOG(INFO) << __func__ << type << " new session created " << sessionId;
        }

        // For first check if it can work? // TODO
        auto& list = mSessionMap[sessionId];
        auto context = std::make_shared<VoiceProcessingContext>(common, type, processData);
        RETURN_VALUE_IF(!context, nullptr, "failedToCreateContext");
        list.push_back(context);

        return context;
    }

    void releaseSession(const VoiceProcessingType& type, int sessionId) {
        LOG(DEBUG) << __func__ << type << " sessionId " << sessionId;
        std::lock_guard lg(mMutex);
        if (mSessionMap.count(sessionId)) {
            auto& list = mSessionMap[sessionId];
            if (!findEffectTypeInList(list, type, true /* remove */)) {
                LOG(ERROR) << __func__ << " can't find " << type << "in session " << sessionId;
                return;
            }
            if (list.empty()) {
                mSessionMap.erase(sessionId);
            }
        }
    }

  private:
    std::mutex mMutex;
    // Each session can have both AEC/NS, maintain per session list of effects
    std::unordered_map<int /* session ID */, VoiceProcessingContextList> mSessionMap
            GUARDED_BY(mMutex);
};
} // namespace aidl::qti::effects