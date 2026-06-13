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

#include "OffloadBundleContext.h"
#include "OffloadBundleTypes.h"

namespace aidl::qti::effects {

/**
 * @brief Maintain all effect offload bundle sessions.
 *
 */
class GlobalOffloadSession {
  public:
    static GlobalOffloadSession& getGlobalSession() {
        static GlobalOffloadSession instance;
        return instance;
    }

    static bool findTypeInContextList(std::vector<std::shared_ptr<OffloadBundleContext>>& list,
                                      const OffloadBundleEffectType& type, bool remove = false) {
        auto itr = std::find_if(list.begin(), list.end(),
                                [type](const std::shared_ptr<OffloadBundleContext>& bundle) {
                                    return bundle->getBundleType() == type;
                                });
        if (itr == list.end()) {
            return false;
        }
        if (remove) {
            (*itr)->deInit(); // call release inside of it.
            list.erase(itr);
        }
        return true;
    }

    std::shared_ptr<OffloadBundleContext> createContext(const OffloadBundleEffectType& type,
                                                        const Parameter::Common& common,
                                                        bool processData) {
        switch (type) {
            case OffloadBundleEffectType::BASS_BOOST:
                return std::make_shared<BassBoostContext>(common, type, processData);
            case OffloadBundleEffectType::EQUALIZER:
                return std::make_shared<EqualizerContext>(common, type, processData);
            case OffloadBundleEffectType::VIRTUALIZER:
                return std::make_shared<VirtualizerContext>(common, type, processData);
            case OffloadBundleEffectType::AUX_ENV_REVERB:
            case OffloadBundleEffectType::INSERT_ENV_REVERB:
            case OffloadBundleEffectType::AUX_PRESET_REVERB:
            case OffloadBundleEffectType::INSERT_PRESET_REVERB:
                return std::make_shared<ReverbContext>(common, type, processData);
        }
        return nullptr;
    }

    /**
     * Create a certain type of BundleContext in shared_ptr container, each session must not have
     * more than one session for each type.
     */
    std::shared_ptr<OffloadBundleContext> createSession(const OffloadBundleEffectType& type,
                                                        const Parameter::Common& common,
                                                        bool processData) {
        std::lock_guard lg(mMutex);
        int ioHandle = common.ioHandle;
        int sessionId = common.session;
        LOG(DEBUG) << __func__ << " " << type << " with ioHandle " << ioHandle << " sessionId"
                   << sessionId;
        if (mSessionsMap.count(sessionId)) {
            if (findTypeInContextList(mSessionsMap[sessionId], type)) {
                LOG(ERROR) << __func__ << type << " already exist in  " << sessionId;
                return nullptr;
            }
        }

        auto& list = mSessionsMap[sessionId];
        LOG(DEBUG) << __func__ << type << " createContext ioHandle " << ioHandle << " sessionId"
                   << sessionId;
        auto context = createContext(type, common, processData);
        RETURN_VALUE_IF(!context, nullptr, "failedToCreateContext");

        list.push_back(context);

        // find ioHandle in the mActiveIoHandles
        for (const auto& pair : mActiveIoHandles) {
            if (pair.first == ioHandle) {
                LOG(DEBUG) << "IoHandle is active " << ioHandle << " session " << sessionId;
                context->start(pair.second);
            }
        }
        return context;
    }

    void releaseSession(const OffloadBundleEffectType& type, int sessionId) {
        std::lock_guard lg(mMutex);
        LOG(DEBUG) << __func__ << " Enter: " << type << " sessionId " << sessionId;
        if (mSessionsMap.count(sessionId)) {
            auto& list = mSessionsMap[sessionId];
            if (!findTypeInContextList(list, type, true /* remove */)) {
                LOG(ERROR) << __func__ << " can't find " << type << "in sessionId " << sessionId;
                return;
            }
            if (list.empty()) {
                mSessionsMap.erase(sessionId);
            }
        }
        LOG(DEBUG) << __func__ << " Exit: " << type << " sessionId " << sessionId << " sessions "
                   << mSessionsMap.size();
    }

    // Used by AudioHal to link effect with output.
    void startEffect(int ioHandle, pal_stream_handle_t* palHandle) {
        std::lock_guard lg(mMutex);

        LOG(DEBUG) << __func__ << " ioHandle " << ioHandle << " palHandle " << palHandle
                   << " sessions " << mSessionsMap.size();
        // start the context having same ioHandle
        for (const auto& handles : mSessionsMap) {
            auto& list = handles.second;
            for (const auto& context : list) {
                if (context->getIoHandle() == ioHandle) {
                    context->start(palHandle);
                }
            }
        }

        mActiveIoHandles[ioHandle] = palHandle;
    }

    // Used by AudioHal to link effect with output.
    void stopEffect(int ioHandle) {
        std::lock_guard lg(mMutex);
        LOG(DEBUG) << __func__ << " ioHandle " << ioHandle << " sessions " << mSessionsMap.size()
                   << "activeHandles " << mActiveIoHandles.count(ioHandle);

        // stop the context having same ioHandle
        for (const auto& handles : mSessionsMap) {
            auto& list = handles.second;
            for (const auto& context : list) {
                if (context->getIoHandle() == ioHandle) {
                    context->stop();
                }
            }
        }

        if (mActiveIoHandles.count(ioHandle)) {
            mActiveIoHandles.erase(ioHandle);
            LOG(VERBOSE) << __func__ << " Removed ioHandle " << ioHandle << " sessions "
                         << mSessionsMap.size() << " activeHandles "
                         << mActiveIoHandles.count(ioHandle);
        }
    }

  private:
    // Lock for mSessionsMap access.
    std::mutex mMutex;

    // map between sessionId and list of effect contexts for that session
    std::unordered_map<int /* sessionId */, std::vector<std::shared_ptr<OffloadBundleContext>>>
            mSessionsMap GUARDED_BY(mMutex);

    // io Handle to palHandle mapping.
    std::unordered_map<int, pal_stream_handle_t*> mActiveIoHandles;
};
} // namespace aidl::qti::effects
