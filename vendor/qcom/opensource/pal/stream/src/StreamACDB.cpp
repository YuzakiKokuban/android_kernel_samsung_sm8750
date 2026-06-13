/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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

 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#define LOG_TAG "PAL: StreamACDB"

#include "StreamACDB.h"
#include "Session.h"
#include "kvh2xml.h"
#include "SessionAlsaPcm.h"
#include "ResourceManager.h"
#include "Device.h"
#include <unistd.h>
#include <chrono>

StreamACDB::StreamACDB(const struct pal_stream_attributes *sattr, struct pal_device *dattr,
        uint32_t instance_id, const std::shared_ptr<ResourceManager> rm)
{
    mStreamMutex.lock();
    uint32_t in_channels = 0, out_channels = 0;
    uint32_t attribute_size = 0;

    session = NULL;
    mGainLevel = -1;
    std::shared_ptr<Device> dev = nullptr;
    mStreamAttr = (struct pal_stream_attributes *)nullptr;
    mDevices.clear();
    currentState = STREAM_IDLE;
    //Modify cached values only at time of SSR down.
    cachedState = STREAM_IDLE;
    bool isDeviceConfigUpdated = false;

    PAL_DBG(LOG_TAG, "Enter");

    //TBD handle modifiers later
    mNoOfModifiers = 0; //no_of_modifiers;
    mModifiers = (struct modifier_kv *) (NULL);

    if (!sattr || !dattr) {
        PAL_ERR(LOG_TAG,"invalid arguments");
        mStreamMutex.unlock();
        throw std::runtime_error("invalid arguments");
    }

    attribute_size = sizeof(struct pal_stream_attributes);
    mStreamAttr = (struct pal_stream_attributes *) calloc(1, attribute_size);
    if (!mStreamAttr) {
        PAL_ERR(LOG_TAG, "malloc for stream attributes failed %s", strerror(errno));
        mStreamMutex.unlock();
        throw std::runtime_error("failed to malloc for stream attributes");
    }

    ar_mem_cpy(mStreamAttr, sizeof(pal_stream_attributes), sattr, sizeof(pal_stream_attributes));

    PAL_INFO(LOG_TAG, "Create new ACDBSession");

    session = Session::makeACDBSession(rm, sattr);
    if (!session) {
        PAL_ERR(LOG_TAG, "session creation failed");
        free(mStreamAttr);
        mStreamMutex.unlock();
        throw std::runtime_error("failed to create session object");
    }

    mStreamMutex.unlock();

    PAL_DBG(LOG_TAG, "Exit. state %d", currentState);

    return;
}

int32_t  StreamACDB::open()
{
    return 0;
}

int32_t  StreamACDB::close()
{
    return 0;
}

StreamACDB::~StreamACDB()
{
    if (mStreamAttr) {
        free(mStreamAttr);
        mStreamAttr = (struct pal_stream_attributes *)NULL;
    }

    if(mVolumeData)  {
        free(mVolumeData);
        mVolumeData = (struct pal_volume_data *)NULL;
    }

    mDevices.clear();
    delete session;
    session = nullptr;
}

int32_t StreamACDB::start()
{
    return 0;
}

int32_t StreamACDB::stop()
{
    return 0;
}

int32_t StreamACDB::prepare()
{
    return 0;
}

int32_t  StreamACDB::setStreamAttributes(struct pal_stream_attributes *sattr)
{
    int32_t status = -EINVAL;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK", session);

    if (!sattr)
    {
        PAL_ERR(LOG_TAG, "NULL stream attributes sent");
        goto exit;
    }
    memset(mStreamAttr, 0, sizeof(struct pal_stream_attributes));
    mStreamMutex.lock();
    ar_mem_cpy (mStreamAttr, sizeof(struct pal_stream_attributes), sattr,
                      sizeof(struct pal_stream_attributes));
    mStreamMutex.unlock();
    status = session->setConfig(this, MODULE, 0);  //TODO:gkv or ckv or tkv need to pass
    if (0 != status) {
        PAL_ERR(LOG_TAG, "session setConfig failed with status %d", status);
        goto exit;
    }

    PAL_DBG(LOG_TAG, "session setConfig successful");
exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

//TBD: move this to Stream, why duplicate code?
int32_t  StreamACDB::setVolume(struct pal_volume_data *volume)
{
    return 0;
}

int32_t  StreamACDB::read(struct pal_buffer* buf)
{
    return 0;
}

int32_t StreamACDB::write(struct pal_buffer* buf)
{
    return 0;
}

int32_t  StreamACDB::registerCallBack(pal_stream_callback /*cb*/, uint64_t /*cookie*/)
{
    return 0;
}

int32_t  StreamACDB::getCallBack(pal_stream_callback * /*cb*/)
{
    return 0;
}

int32_t StreamACDB::getParameters(uint32_t /*param_id*/, void ** /*payload*/)
{
    return 0;
}

int32_t  StreamACDB::setParameters(uint32_t param_id, void *payload)
{
    return 0;
}

int32_t StreamACDB::mute_l(bool state)
{
    return 0;
}

int32_t StreamACDB::mute(bool state)
{
    return 0;
}

int32_t StreamACDB::pause_l()
{
    return 0;
}

int32_t StreamACDB::pause()
{
    return 0;
}

int32_t StreamACDB::resume_l()
{
    return 0;
}

int32_t StreamACDB::resume()
{
    return 0;
}

int32_t StreamACDB::flush()
{
    return 0;
}


int32_t StreamACDB::addRemoveEffect(pal_audio_effect_t effect, bool enable)
{
    return 0;
}

int32_t StreamACDB::setECRef(std::shared_ptr<Device> dev, bool is_enable)
{
    return 0;
}

int32_t StreamACDB::setECRef_l(std::shared_ptr<Device> dev, bool is_enable)
{
    return 0;
}

int32_t StreamACDB::ssrDownHandler()
{
    return 0;
}

int32_t StreamACDB::ssrUpHandler()
{
    return 0;
}

int32_t StreamACDB::createMmapBuffer(int32_t min_size_frames,
                                   struct pal_mmap_buffer *info)
{
    return 0;
}

int32_t StreamACDB::GetMmapPosition(struct pal_mmap_position *position)
{
    return 0;
}

int32_t StreamACDB::rwACDBParam(pal_device_id_t palDeviceId,
                 pal_stream_type_t palStreamType, uint32_t sampleRate,
                 uint32_t instanceId,
                 void *payload, bool isParamWrite)
{
    return session->rwACDBParamTunnel(payload, palDeviceId,
        palStreamType, sampleRate, instanceId, isParamWrite, this);
}

