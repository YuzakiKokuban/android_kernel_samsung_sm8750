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
 *
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: StreamSensorPCMData"

#include "StreamSensorPCMData.h"
#include "Session.h"
#include "kvh2xml.h"
#include "SessionAlsaPcm.h"
#include "ResourceManager.h"
#include "Device.h"
#include <unistd.h>

StreamSensorPCMData::StreamSensorPCMData(const struct pal_stream_attributes *sattr __unused,
                    struct pal_device *dattr __unused,
                    const uint32_t no_of_devices __unused,
                    const struct modifier_kv *modifiers __unused,
                    const uint32_t no_of_modifiers __unused,
                    const std::shared_ptr<ResourceManager> rm):
                    StreamCommon(sattr,dattr,no_of_devices,modifiers,no_of_modifiers,rm)
{
    int32_t enable_concurrency_count = 0;
    int32_t disable_concurrency_count = 0;
    paused_ = false;

    PAL_DBG(LOG_TAG, "Enter");
    /* get ACD platform info */
    acd_info_ = ACDPlatformInfo::GetInstance();
    if (!acd_info_) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to get acd platform info", -EINVAL);
        throw std::runtime_error("Failed to get acd platform info");
    }

    rm->registerStream(this);

    /* Print the concurrency feature flags supported */
    PAL_INFO(LOG_TAG, "capture conc enable %d,voice conc enable %d,voip conc enable %d",
             acd_info_->GetConcurrentCaptureEnable(),
             acd_info_->GetConcurrentVoiceCallEnable(),
             acd_info_->GetConcurrentVoipCallEnable());

    /* check concurrency count from rm */
    rm->GetSoundTriggerConcurrencyCount(PAL_STREAM_SENSOR_PCM_DATA,
                                        &enable_concurrency_count,
                                        &disable_concurrency_count);

    /*
     * When voice/voip/record is active and concurrency is not
     * supported, mark paused as true, so that start stream
     * will be skipped and when voice/voip/record stops, stream
     * will be resumed.
     */
    if (disable_concurrency_count) {
        paused_ = true;
    }

    PAL_DBG(LOG_TAG, "Exit");
}

StreamSensorPCMData::~StreamSensorPCMData()
{
    PAL_DBG(LOG_TAG, "Enter");
    rm->resetStreamInstanceID(this);
    rm->deregisterStream(this);
    PAL_DBG(LOG_TAG, "Exit");
}

int32_t StreamSensorPCMData::open()
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter.");

    std::lock_guard<std::mutex> lck(mStreamMutex);
    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        PAL_ERR(LOG_TAG, "Error:Sound card offline/standby, can not open stream");
        usleep(SSR_RECOVERY);
        status = -EIO;
        goto exit;
    }

    if (currentState == STREAM_IDLE) {
        currentState = STREAM_INIT;
        PAL_DBG(LOG_TAG, "streamLL opened. state %d", currentState);
    } else if (currentState == STREAM_INIT) {
        PAL_INFO(LOG_TAG, "Stream is already opened, state %d", currentState);
        status = 0;
        goto exit;
    } else {
        PAL_ERR(LOG_TAG, "Error:Stream is not in correct state %d", currentState);
        status = -EINVAL;
        goto exit;
    }
exit:
    palStateEnqueue(this, PAL_STATE_OPENED, status);
    PAL_DBG(LOG_TAG, "Exit ret %d", status);
    return status;
}

int32_t  StreamSensorPCMData::close()
{
    int32_t status = 0;
    mStreamMutex.lock();

    if (currentState == STREAM_IDLE) {
        PAL_INFO(LOG_TAG, "Stream is already closed");
        mStreamMutex.unlock();
        return status;
    }

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK device count - %zu stream_type - %d state %d",
            session, mDevices.size(), mStreamAttr->type, currentState);

    if (currentState == STREAM_STARTED) {
        mStreamMutex.unlock();
        status = stop();
        if (0 != status)
            PAL_ERR(LOG_TAG, "Error:stream stop failed. status %d",  status);
        mStreamMutex.lock();
    }
    rm->lockGraph();
    status = session->close(this);
    rm->unlockGraph();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:session close failed with status %d", status);
    }
    currentState = STREAM_IDLE;
    palStateEnqueue(this, PAL_STATE_CLOSED, status);
    mStreamMutex.unlock();

    PAL_DBG(LOG_TAG, "Exit ret %d", status);
    return status;
}
int32_t StreamSensorPCMData::start()
{
    int32_t status = 0;
    bool backend_update = false;
    std::shared_ptr<Device> device = nullptr;

    PAL_DBG(LOG_TAG,
            "Enter. session handle: %pK, state: %d, paused_: %s",
            session, currentState, paused_ ? "True" : "False");

    std::lock_guard<std::mutex> lck(mStreamMutex);
    if (true == paused_) {
        PAL_DBG(LOG_TAG,"concurrency is not supported, start the stream later");
        goto exit;
    }

    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        cachedState = STREAM_STARTED;
        PAL_ERR(LOG_TAG, "Error:Sound card offline/standby. Update the cached state %d",
                cachedState);
        goto exit;
    }

    if (currentState == STREAM_INIT || currentState == STREAM_STOPPED ||
        currentState == STREAM_PAUSED) {
        device = GetPalDevice(this, GetAvailCaptureDevice());
        if (!device) {
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Error:%d Device creation is failed", status);
            goto exit;
        }

        rm->voteSleepMonitor(this, true);
        status = device->open();
        rm->voteSleepMonitor(this, false);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Error: device [%d] open failed with status %d",
                    device->getSndDeviceId(), status);
            goto exit;
        }

        if (currentState != STREAM_PAUSED) {
            mDevices.clear();
            mDevices.push_back(device);

            status = session->open(this);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:session open failed with status %d", status);
                goto exit;
            }
            PAL_DBG(LOG_TAG, "session open successful");

            /* Do not update capture profile when resuming stream */
            backend_update = rm->UpdateSoundTriggerCaptureProfile(this, true);
            if (backend_update) {
                status = rm->StopOtherDetectionStreams(this);
                if (status)
                    PAL_ERR(LOG_TAG, "Error:%d Failed to stop other Detection streams", status);

                status = rm->StartOtherDetectionStreams(this);
                if (status)
                    PAL_ERR(LOG_TAG, "Error:%d Failed to start other Detection streams", status);
            }
        }

        status = device->start();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Error: device [%d] start failed with status %d",
                    device->getSndDeviceId(), status);
            device->close();
            goto exit;
        }
        PAL_DBG(LOG_TAG, "device [%d] opened and started successfully",
                device->getSndDeviceId());

        rm->registerDevice(device, this);

        status = startSession();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Error: start session failed with status %d", status);
            device->stop();
            rm->deregisterDevice(device, this);
            device->close();
            goto exit;
        }
        PAL_DBG(LOG_TAG, "session start successful");

        /*
         * pcm_open and pcm_start done at once here,
         * so directly jump to STREAM_STARTED state.
         */
        currentState = STREAM_STARTED;
    } else if (currentState == STREAM_STARTED) {
        PAL_INFO(LOG_TAG, "Stream already started, state %d", currentState);
    } else {
        PAL_ERR(LOG_TAG, "Error:Stream is not opened yet");
        status = -EINVAL;
    }

exit:
    palStateEnqueue(this, PAL_STATE_STARTED, status);
    PAL_DBG(LOG_TAG, "Exit. state %d, status %d", currentState, status);
    return status;
}

int32_t StreamSensorPCMData::stop()
{
    int32_t status = 0;
    bool backend_update = false;

    PAL_DBG(LOG_TAG, "Enter. session handle: %pK, state: %d, paused_: %s",
            session, currentState, paused_ ? "True" : "False");

    std::lock_guard<std::mutex> lck(mStreamMutex);

    if (currentState == STREAM_STARTED || currentState == STREAM_PAUSED) {
        /* Do not update capture profile when pausing stream */
        if (false == paused_) {
            backend_update = rm->UpdateSoundTriggerCaptureProfile(this, false);
            if (backend_update) {
                status = rm->StopOtherDetectionStreams(this);
                if (status)
                    PAL_ERR(LOG_TAG, "Error:%d Failed to stop other Detection streams", status);
            }
        }

        status = session->stop(this);
        if (status)
            PAL_ERR(LOG_TAG, "Error:%s session stop failed with status %d",
                    GET_DIR_STR(mStreamAttr->direction), status);

        for (int32_t i = 0; i < mDevices.size(); i++) {
            status = mDevices[i]->stop();
            if (status)
                PAL_ERR(LOG_TAG, "Error: device [%d] stop failed with status %d",
                        mDevices[i]->getSndDeviceId(), status);

            rm->deregisterDevice(mDevices[i], this);

            status = mDevices[i]->close();
            if (status)
                PAL_ERR(LOG_TAG, "Error: device [%d] close failed with status %d",
                        mDevices[i]->getSndDeviceId(), status);
            PAL_DBG(LOG_TAG, "device [%d] close successful", mDevices[i]->getSndDeviceId());
        }

        if (backend_update) {
            status = rm->StartOtherDetectionStreams(this);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to start other Detection streams", status);
        }

        currentState = STREAM_STOPPED;
    } else if (currentState == STREAM_STOPPED || currentState == STREAM_IDLE) {
        PAL_INFO(LOG_TAG, "Stream is already in Stopped/idle state %d", currentState);
    } else {
        PAL_ERR(LOG_TAG, "Error:Stream should be in start/pause state, %d", currentState);
        status = -EINVAL;
    }
    palStateEnqueue(this, PAL_STATE_STOPPED, status);
    PAL_DBG(LOG_TAG, "Exit. status %d, state %d", status, currentState);

    return status;
}

int32_t StreamSensorPCMData::Resume(bool is_internal)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");

    paused_ = false;
    status = start();
    if (status)
        PAL_ERR(LOG_TAG, "Error:%d Resume Stream failed", status);

    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamSensorPCMData::Pause(bool is_internal)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");

    paused_ = true;
    status = stop();
    if (!status)
    {
        currentState = STREAM_PAUSED;
    }
    else
        PAL_ERR(LOG_TAG, "Error:%d Pause Stream failed", status);
    palStateEnqueue(this, PAL_STATE_PAUSED, status);
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

void StreamSensorPCMData::GetUUID(class SoundTriggerUUID *uuid,
                                  const struct st_uuid *vendor_uuid)
{
    uuid->timeLow = (uint32_t)vendor_uuid->timeLow;
    uuid->timeMid = (uint16_t)vendor_uuid->timeMid;
    uuid->timeHiAndVersion = (uint16_t)vendor_uuid->timeHiAndVersion;
    uuid->clockSeq = (uint16_t)vendor_uuid->clockSeq;
    uuid->node[0] = (uint8_t)vendor_uuid->node[0];
    uuid->node[1] = (uint8_t)vendor_uuid->node[1];
    uuid->node[2] = (uint8_t)vendor_uuid->node[2];
    uuid->node[3] = (uint8_t)vendor_uuid->node[3];
    uuid->node[4] = (uint8_t)vendor_uuid->node[4];
    uuid->node[5] = (uint8_t)vendor_uuid->node[5];
    PAL_INFO(LOG_TAG, "Input vendor uuid : %08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
             uuid->timeLow, uuid->timeMid, uuid->timeHiAndVersion, uuid->clockSeq,
             uuid->node[0], uuid->node[1], uuid->node[2], uuid->node[3], uuid->node[4],
             uuid->node[5]);
}

/* Use below UUID for Sensor PCM Data usecase */
static const struct st_uuid qc_sensor_pcm_data_uuid =
    { 0xc88a2c89, 0x7a55, 0x498c, 0x836f, { 0x5d, 0x7e, 0xc8, 0x58, 0x29, 0x90 } };

struct st_uuid StreamSensorPCMData::GetVendorUuid()
{
    return qc_sensor_pcm_data_uuid;
}

int32_t StreamSensorPCMData::SetupStreamConfig(const struct st_uuid *vendor_uuid)
{
    int32_t status = 0;
    class SoundTriggerUUID uuid;

    PAL_DBG(LOG_TAG, "Enter");
    GetUUID(&uuid, vendor_uuid);

    if (NULL == sm_cfg_) {
        sm_cfg_ = acd_info_->GetStreamConfig(uuid);
        if (!sm_cfg_) {
            status = -EINVAL;
            goto exit;
        }
    }

    mStreamSelector = sm_cfg_->GetStreamConfigName();
    mInstanceID = rm->getStreamInstanceID(this);

exit:
    if (status)
        PAL_ERR(LOG_TAG, "SetupStreamConfig failed with status %d", status);
    PAL_DBG(LOG_TAG, "Exit, status %d, mInstanceID is %d", status, mInstanceID);
    return status;
}

pal_device_id_t StreamSensorPCMData::GetAvailCaptureDevice()
{
    if (mPalDevices[0] &&
        mPalDevices[0]->getSndDeviceId() == PAL_DEVICE_IN_ULTRASOUND_MIC) {
        return PAL_DEVICE_IN_ULTRASOUND_MIC;
    } else {
        if (acd_info_->GetSupportDevSwitch() &&
            rm->isDeviceAvailable(PAL_DEVICE_IN_WIRED_HEADSET))
            return PAL_DEVICE_IN_HEADSET_VA_MIC;
        else
            return PAL_DEVICE_IN_HANDSET_VA_MIC;
    }
}

std::shared_ptr<CaptureProfile> StreamSensorPCMData::GetCurrentCaptureProfile()
{
    std::shared_ptr<CaptureProfile> cap_prof = nullptr;
    enum StInputModes input_mode = ST_INPUT_MODE_HANDSET;
    enum StOperatingModes operating_mode = ST_OPERATING_MODE_HIGH_PERF_NS;
    pal_device_id_t current_capture_device = GetAvailCaptureDevice();

    PAL_DBG(LOG_TAG, "Enter");

    if (current_capture_device == PAL_DEVICE_IN_HEADSET_VA_MIC)
        input_mode = ST_INPUT_MODE_HEADSET;

    /* Check lpi here again to determine the actual operating_mode */
    if ((DEVICEPP_TX_RAW_LPI == pcm_data_stream_effect ||
        DEVICEPP_TX_FLUENCE_FFEC == pcm_data_stream_effect)
        && rm->getLPIUsage())
        operating_mode = (current_capture_device == PAL_DEVICE_IN_ULTRASOUND_MIC ?
                          ST_OPERATING_MODE_LOW_POWER_TX_MACRO : ST_OPERATING_MODE_LOW_POWER);
    else if ((DEVICEPP_TX_RAW_LPI == pcm_data_stream_effect ||
             DEVICEPP_TX_FLUENCE_FFEC == pcm_data_stream_effect)
             && !rm->getLPIUsage())
        operating_mode = (current_capture_device == PAL_DEVICE_IN_ULTRASOUND_MIC ?
                          ST_OPERATING_MODE_HIGH_PERF_TX_MACRO : ST_OPERATING_MODE_HIGH_PERF);
    else if ((DEVICEPP_TX_FLUENCE_FFNS == pcm_data_stream_effect ||
             DEVICEPP_TX_FLUENCE_FFECNS == pcm_data_stream_effect)
             && rm->getLPIUsage())
        operating_mode = ST_OPERATING_MODE_LOW_POWER_NS;
    else if ((DEVICEPP_TX_FLUENCE_FFNS == pcm_data_stream_effect ||
             DEVICEPP_TX_FLUENCE_FFECNS == pcm_data_stream_effect)
             && !rm->getLPIUsage())
        operating_mode = ST_OPERATING_MODE_HIGH_PERF_NS;

    if (current_capture_device == PAL_DEVICE_IN_ULTRASOUND_MIC) {
        std::shared_ptr<SoundTriggerPlatformInfo> st_info = nullptr;
        std::string capture_profile_name;
        uint32_t sample_rate = 0;
        uint32_t bit_width = 0;
        uint16_t channels = 0;
        struct pal_device dev_config = {};

        if (mPalDevices[0] &&
            mPalDevices[0]->getDeviceAttributes(&dev_config) == 0) {
            sample_rate = dev_config.config.sample_rate;
            bit_width = dev_config.config.bit_width;
            channels = dev_config.config.ch_info.channels;
        }

        if (channels == 1)
            capture_profile_name.append("SINGLE_MIC_");
        else if (channels == 2)
            capture_profile_name.append("DUAL_MIC_");

        if (sample_rate == 48000)
            capture_profile_name.append("48KHZ_");
        else if (sample_rate == 96000)
            capture_profile_name.append("96KHZ_");

        if (bit_width == 16)
            capture_profile_name.append("16BIT_");
        else if (bit_width == 24)
            capture_profile_name.append("24BIT_");

        if (pcm_data_buffering == 1)
            capture_profile_name.append("RAW_LPI_TX_MACRO");
        else
            capture_profile_name.append("RAW_LPI_NO_BUFFER_TX_MACRO");

        PAL_DBG(LOG_TAG, "capture_profile_name: %s", capture_profile_name.c_str());
        st_info = SoundTriggerPlatformInfo::GetInstance();
        if (st_info)
            cap_prof = st_info->GetCaptureProfileFromMap(capture_profile_name);
    } else {
        cap_prof = sm_cfg_->GetCaptureProfile(std::make_pair(operating_mode, input_mode));
    }

    if (cap_prof) {
        PAL_DBG(LOG_TAG, "cap_prof: %s bw=%d, chs=%d, sr=%d, snd_name=%s, ec_ref=%d",
                cap_prof->GetName().c_str(), cap_prof->GetBitWidth(),
                cap_prof->GetChannels(), cap_prof->GetSampleRate(),
                cap_prof->GetSndName().c_str(), cap_prof->isECRequired());
    } else {
        PAL_INFO(LOG_TAG, "Invalid capture profile");
    }

    return cap_prof;
}

int32_t StreamSensorPCMData::addRemoveEffect(pal_audio_effect_t effect, bool enable)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter. session handle: %pK", session);
    std::lock_guard<std::mutex> lck(mStreamMutex);

    /* Check lpi here to determine if EC is needed */
    if (enable) {
        if (PAL_AUDIO_EFFECT_NONE == effect && rm->getLPIUsage()) {
           pcm_data_stream_effect = DEVICEPP_TX_RAW_LPI;
        } else if (PAL_AUDIO_EFFECT_NS == effect && rm->getLPIUsage()) {
           pcm_data_stream_effect = DEVICEPP_TX_FLUENCE_FFNS;
        } else if (PAL_AUDIO_EFFECT_NONE == effect && !rm->getLPIUsage()) {
           pcm_data_stream_effect = DEVICEPP_TX_FLUENCE_FFEC;
        } else if (PAL_AUDIO_EFFECT_NS == effect && !rm->getLPIUsage()) {
           pcm_data_stream_effect = DEVICEPP_TX_FLUENCE_FFECNS;
        } else {
            PAL_ERR(LOG_TAG, "Invalid effect ID %d", effect);
            status = -EINVAL;
            goto exit;
        }
    } else {
        PAL_DBG(LOG_TAG, "Exit, no stream effect is set");
        status = -EINVAL;
        goto exit;
    }

    /* Use QC Sensor PCM Data as default streamConfig */
    status = SetupStreamConfig(&qc_sensor_pcm_data_uuid);
    if (status)
        goto exit;

    cap_prof_ = GetCurrentCaptureProfile();
    if (cap_prof_) {
        mDevPPSelector = cap_prof_->GetName();
    } else {
        status = -EINVAL;
        goto exit;
    }
    PAL_DBG(LOG_TAG, "Exit. addRemoveEffect successful");

exit:
    if (status)
        PAL_ERR(LOG_TAG, "addRemoveEffect failed with status %d", status);
    return status;
}

int32_t StreamSensorPCMData::HandleConcurrentStream(bool active)
{
    int32_t status = 0;
    std::shared_ptr<CaptureProfile> new_cap_prof = nullptr;

    PAL_DBG(LOG_TAG, "Enter, active:%d", active);

    if (active == false)
        mStreamMutex.lock();

    if (currentState != STREAM_STARTED) {
        PAL_INFO(LOG_TAG, "Stream is not in started state");
        if (active == true)
            mStreamMutex.unlock();
        return status;
    }

    new_cap_prof = GetCurrentCaptureProfile();
    if (new_cap_prof && cap_prof_ != new_cap_prof) {
        PAL_DBG(LOG_TAG,
            "current capture profile %s: dev_id=0x%x, chs=%d, sr=%d, ec_ref=%d\n",
            cap_prof_->GetName().c_str(),
            cap_prof_->GetDevId(),
            cap_prof_->GetChannels(),
            cap_prof_->GetSampleRate(),
            cap_prof_->isECRequired());
        PAL_DBG(LOG_TAG,
            "new capture profile %s: dev_id=0x%x, chs=%d, sr=%d, ec_ref=%d\n",
            new_cap_prof->GetName().c_str(),
            new_cap_prof->GetDevId(),
            new_cap_prof->GetChannels(),
            new_cap_prof->GetSampleRate(),
            new_cap_prof->isECRequired());

        if (!active) {
            PAL_DBG(LOG_TAG, "disconnect device %d", GetAvailCaptureDevice());
            /* disconnect the backend device */
            status = DisconnectDevice_l(GetAvailCaptureDevice());
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to disconnect device %d",
                        status, GetAvailCaptureDevice());
        } else {
            /* store the pre-proc KV selected in the config file */
            if (new_cap_prof)
                mDevPPSelector = new_cap_prof->GetName();
            /* connect the backend device */
            PAL_DBG(LOG_TAG, "connect device %d", GetAvailCaptureDevice());
            status = ConnectDevice_l(GetAvailCaptureDevice());
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to connect device %d",
                        status, GetAvailCaptureDevice());
        }
    } else {
      PAL_INFO(LOG_TAG, "no action needed, same capture profile");
    }

    if (active == true)
        mStreamMutex.unlock();

    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamSensorPCMData::DisconnectDevice_l(pal_device_id_t device_id)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter, device_id: %d", device_id);
    if (mDevices[0]->getSndDeviceId() != device_id) {
        PAL_ERR(LOG_TAG, "Error:%d Device %d not connected, ignore",
                -EINVAL, device_id);
        goto exit;
    }

    this->rm->deregisterDevice(mDevices[0], this);

    if (session)
        status = session->disconnectSessionDevice(this,
                                                  this->mStreamAttr->type,
                                                  mDevices[0]);
    if (status)
        PAL_ERR(LOG_TAG, "Error:%d Failed to disconnect device %d",
                status, device_id);

    status = mDevices[0]->stop();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d device stop failed", status);
        goto disconnect_err;
    }

    status = mDevices[0]->close();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d device close failed", status);
        goto disconnect_err;
    }

disconnect_err:
    mDevices.clear();
exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamSensorPCMData::DisconnectDevice(pal_device_id_t device_id)
{
    int32_t status = -EINVAL;

    PAL_DBG(LOG_TAG, "Enter, device_id: %d", device_id);
    /*
     * NOTE: mStreamMutex will be unlocked after ConnectDevice handled
     * because device disconnect/connect should be handled sequencely,
     * and no other commands from client should be handled between
     * device disconnect and connect.
     */
    mStreamMutex.lock();
    status = DisconnectDevice_l(device_id);

    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamSensorPCMData::ConnectDevice_l(pal_device_id_t device_id)
{
    int32_t status = 0;
    std::shared_ptr<Device> device = nullptr;

    PAL_DBG(LOG_TAG, "Enter, device_id: %d", device_id);

    if (GetAvailCaptureDevice() != device_id) {
        PAL_INFO(LOG_TAG, "Skip, device %d is alreay connected",
                 GetAvailCaptureDevice());
        goto connect_err;
    }

    device = GetPalDevice(this, device_id);
    if (!device) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d Device creation is failed", status);
        goto connect_err;
    }

    rm->voteSleepMonitor(this, true);
    status = device->open();
    rm->voteSleepMonitor(this, false);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d device %d open failed", status,
                device->getSndDeviceId());
        goto connect_err;
    }
    mDevices.clear();
    mDevices.push_back(device);

    /* Update capture profile and mDevPPSelector */
    cap_prof_ = GetCurrentCaptureProfile();
    if (cap_prof_)
        mDevPPSelector = cap_prof_->GetName();
    else
        PAL_ERR(LOG_TAG, "Failed to update capture profile and mDevPPSelector");

    if (session)
        status = session->setupSessionDevice(this,
                                             this->mStreamAttr->type,
                                             device);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d setupSessionDevice for %d failed",
                status, device->getSndDeviceId());
        device->close();
        goto connect_err;
    }

    status = device->start();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d device %d start failed",
                status, device->getSndDeviceId());
        device->close();
        goto connect_err;
    }

    if (session)
        status = session->connectSessionDevice(this,
                                               this->mStreamAttr->type,
                                               device);
    if (status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to connect device %d",
                status, device_id);
        device->stop();
        device->close();
    } else {
        this->rm->registerDevice(device, this);
    }

connect_err:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamSensorPCMData::ConnectDevice(pal_device_id_t device_id)
{
    int32_t status = -EINVAL;

    PAL_DBG(LOG_TAG, "Enter, device_id: %d", device_id);

    status = ConnectDevice_l(device_id);
    mStreamMutex.unlock();

    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamSensorPCMData::setECRef_l(std::shared_ptr<Device> dev, bool is_enable)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter, enable %d", is_enable);

    if (!cap_prof_ || !cap_prof_->isECRequired()) {
        PAL_DBG(LOG_TAG, "No need to set EC Ref");
        goto exit;
    }

    if (dev && !rm->checkECRef(dev, mDevices[0])) {
        PAL_DBG(LOG_TAG, "No need to set EC Ref for unmatching rx device");
        goto exit;
    }

    status = session->setECRef(this, dev, is_enable);
    if (status)
        PAL_ERR(LOG_TAG, "Error:%d Failed to set EC Ref", status);

exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamSensorPCMData::setECRef(std::shared_ptr<Device> dev, bool is_enable)
{
    int32_t status = 0;

    std::lock_guard<std::mutex> lck(mStreamMutex);
    if (!rm->getLPIUsage())
        status = setECRef_l(dev, is_enable);
    else
        PAL_DBG(LOG_TAG, "set EC Ref will be handled in LPI/NLPI switch");

    return status;
}

int32_t StreamSensorPCMData::setParameters(uint32_t param_id, void *payload)
{
    int32_t status = 0;
    pal_param_payload *param_payload = (pal_param_payload *)payload;

    if (!param_payload) {
        PAL_ERR(LOG_TAG, "Error: Invalid payload for param ID: %d", param_id);
        return -EINVAL;
    }

    PAL_DBG(LOG_TAG, "Enter, param id %d", param_id);

    std::lock_guard<std::mutex> lck(mStreamMutex);
    switch (param_id) {
        case PAL_PARAM_ID_CUSTOM_CONFIGURATION: {
            pcm_data_buffering = *((uint32_t *) param_payload->payload);
            break;
        }
        default: {
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Error:%d Unsupported param %u", status, param_id);
            break;
        }
    }

    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}
