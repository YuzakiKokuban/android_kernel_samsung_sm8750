/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/android/hardware/soundtrigger3/BnSoundTriggerHw.h>
#include <aidl/android/hardware/soundtrigger3/BnSoundTriggerHwCallback.h>
#include <aidl/android/hardware/soundtrigger3/BnSoundTriggerHwGlobalCallback.h>
#include <errno.h>
#include <stdlib.h>
#include <mutex>

#include "PalDefs.h"

namespace aidl::android::hardware::soundtrigger3 {

using namespace ::aidl::android::media::soundtrigger;
using SoundModelHandle = int32_t;

enum class SessionState {
    IDLE,
    LOADED,
    ACTIVE,
    BUFFERING,
    STOPPING,
    STOPPED,
};

class SoundTriggerSession {
public:
    SoundTriggerSession(SoundModelHandle handle,
                        const std::shared_ptr<ISoundTriggerHwCallback> &stHwCallback);
    ~SoundTriggerSession();
    int loadSoundModel(const SoundModel &model);
    int loadPhraseSoundModel(const PhraseSoundModel &model);
    int unloadSoundModel();
    int startRecognition(int32_t deviceHandle, int32_t ioHandle,
                         const RecognitionConfig &config);
    int stopRecognition();
    std::string getModuleVersion();
    SoundModelHandle getSessionHandle() { return mSessionHandle; }

private:
    int configurePALSession_l(const SoundModel &model, std::vector<uint8_t> & payload);
    int loadSoundModel_l(const SoundModel &model);
    int loadPhraseSoundModel_l(const PhraseSoundModel &model);
    int unloadSoundModel_l();
    int startRecognition_l(int32_t deviceHandle, int32_t ioHandle,
                           const RecognitionConfig &config);
    int stopRecognition_l();
    void onCallback(uint32_t *eventData);
    void onRecognitionCallback_l(struct pal_st_recognition_event *palEvent);
    void onPhraseRecognitionCallback_l(struct pal_st_phrase_recognition_event *palPhraseEvent);
    int openPALStream(pal_stream_type_t stream_type);
    static int palCallback(pal_stream_handle_t *stream_handle,
                           uint32_t event_id, uint32_t *event_data,
                           uint32_t event_size, uint64_t cookie);
    void setSessionState_l(SessionState state) { mSessionState = state; }
    SessionState getSessionState() { return mSessionState; }
    bool isSessionActive_l() { return mSessionState == SessionState::ACTIVE; }

    SoundModelHandle mSessionHandle;
    std::mutex mSessionMutex;
    SessionState mSessionState = SessionState::IDLE;
    std::shared_ptr<ISoundTriggerHwCallback> mClientCallback;

    pal_stream_handle_t *mPalHandle = nullptr;
};
} // namespace aidl::android::hardware::soundtrigger3
