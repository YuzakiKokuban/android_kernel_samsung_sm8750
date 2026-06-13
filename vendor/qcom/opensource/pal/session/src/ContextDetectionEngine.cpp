/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
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

#define LOG_TAG "PAL: ContextDetectionEngine"

#include "ContextDetectionEngine.h"

#include "ACDEngine.h"
#include "Stream.h"
#include "Session.h"

std::shared_ptr<ContextDetectionEngine> ContextDetectionEngine::Create(
    Stream *s,
    std::shared_ptr<ACDStreamConfig> sm_cfg)
{
    std::shared_ptr<ContextDetectionEngine> engine(nullptr);
    std::string streamConfigName;

    if (!s) {
        PAL_ERR(LOG_TAG, "Invalid stream handle");
        return nullptr;
    }
    streamConfigName = sm_cfg->GetStreamConfigName();

    if (!strcmp(streamConfigName.c_str(), "QC_ACD")) {
        try {
            engine = ACDEngine::GetInstance(s, sm_cfg);
        } catch (const std::exception& e) {
            PAL_ERR(LOG_TAG, "ContextDetectionEngine creation failed %s", e.what());
            goto exit;
        }
    } else {
        PAL_ERR(LOG_TAG, "Invalid Stream Config");
    }

    PAL_VERBOSE(LOG_TAG, "Exit, engine %p", engine.get());
exit:
    return engine;
}

ContextDetectionEngine::ContextDetectionEngine(
    Stream *s,
    std::shared_ptr<ACDStreamConfig> sm_cfg) {

    struct pal_stream_attributes sAttr;
    std::shared_ptr<ResourceManager> rm = nullptr;

    eng_state_ = ENG_IDLE;
    sm_cfg_ = sm_cfg;
    exit_thread_ = false;
    stream_handle_ = s;
    eng_state_ = ENG_IDLE;
    builder_ = new PayloadBuilder();
    dev_disconnect_count_ = 0;

    PAL_DBG(LOG_TAG, "Enter");

    acd_info_ = ACDPlatformInfo::GetInstance();
    if (!acd_info_) {
        PAL_ERR(LOG_TAG, "No ACD platform info present");
        throw std::runtime_error("No ACD platform info present");
    }

    // Create session
    rm = ResourceManager::getInstance();
    if (!rm) {
        PAL_ERR(LOG_TAG, "Failed to get ResourceManager instance");
        throw std::runtime_error("Failed to get ResourceManager instance");
    }
    stream_handle_->getStreamAttributes(&sAttr);
    session_ = Session::makeSession(rm, &sAttr);
    if (!session_) {
        PAL_ERR(LOG_TAG, "Failed to create session");
        throw std::runtime_error("Failed to create session");
    }

    PAL_DBG(LOG_TAG, "Exit");
}

ContextDetectionEngine::~ContextDetectionEngine()
{
    PAL_INFO(LOG_TAG, "Enter");

    if (event_thread_handler_.joinable()) {
        std::unique_lock<std::mutex> lck(mutex_);
        exit_thread_ = true;
        cv_.notify_one();
        lck.unlock();
        event_thread_handler_.join();
        lck.lock();
        PAL_INFO(LOG_TAG, "Thread joined");
    }

    if (session_) {
        delete session_;
    }
    PAL_INFO(LOG_TAG, "Exit");
}

int32_t ContextDetectionEngine::ConnectSessionDevice(
    Stream* stream_handle, pal_stream_type_t stream_type,
    std::shared_ptr<Device> device_to_connect)
{
    int32_t status = 0;

    if (!session_) {
        PAL_ERR(LOG_TAG, "Invalid session");
        return -EINVAL;
    }

    if (dev_disconnect_count_ == 0)
        status = session_->connectSessionDevice(stream_handle, stream_type,
                                            device_to_connect);

    PAL_DBG(LOG_TAG, "dev_disconnect_count_: %d", dev_disconnect_count_);
    return status;
}

int32_t ContextDetectionEngine::DisconnectSessionDevice(
    Stream* stream_handle, pal_stream_type_t stream_type,
    std::shared_ptr<Device> device_to_disconnect)
{
    int32_t status = 0;

    if (!session_) {
        PAL_ERR(LOG_TAG, "Invalid session");
        return -EINVAL;
    }

    dev_disconnect_count_++;
    if (dev_disconnect_count_ == eng_streams_.size())
        status = session_->disconnectSessionDevice(stream_handle, stream_type,
                                               device_to_disconnect);
    PAL_DBG(LOG_TAG, "dev_disconnect_count_: %d", dev_disconnect_count_);
    return status;
}

int32_t ContextDetectionEngine::SetupSessionDevice(
    Stream* stream_handle, pal_stream_type_t stream_type,
    std::shared_ptr<Device> device_to_disconnect)
{
    int32_t status = 0;

    if (!session_) {
        PAL_ERR(LOG_TAG, "Invalid session");
        return -EINVAL;
    }

    dev_disconnect_count_--;
    if (dev_disconnect_count_ < 0)
        dev_disconnect_count_ = 0;

    if (dev_disconnect_count_ == 0)
        status = session_->setupSessionDevice(stream_handle, stream_type,
                                          device_to_disconnect);

    PAL_DBG(LOG_TAG, "dev_disconnect_count_: %d", dev_disconnect_count_);
    return status;
}

int32_t ContextDetectionEngine::setECRef(Stream *s, std::shared_ptr<Device> dev, bool is_enable)
{
    if (!session_) {
        PAL_ERR(LOG_TAG, "Invalid session");
        return -EINVAL;
    }

    return session_->setECRef(s, dev, is_enable);
}

int32_t ContextDetectionEngine::getTagsWithModuleInfo(Stream *s, size_t *size, uint8_t *payload)
{
    if (!session_) {
        PAL_ERR(LOG_TAG, "Invalid session");
        return -EINVAL;
    }

    return session_->getTagsWithModuleInfo(s, size, payload);
}
