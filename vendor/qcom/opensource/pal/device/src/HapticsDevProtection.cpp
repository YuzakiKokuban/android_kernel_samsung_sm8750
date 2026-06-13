/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
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

#define LOG_TAG "PAL: HapticsDevProtection"


#include "HapticsDevProtection.h"
#include "PalAudioRoute.h"
#include "ResourceManager.h"
#include "SessionAlsaUtils.h"
#include "kvh2xml.h"
#include <agm/agm_api.h>

#include <fstream>
#include <sstream>

#ifndef PAL_HAP_DEVP_CAL_PATH
#define PAL_HAP_DEVP_CAL_PATH "/data/vendor/audio/haptics.cal"
#endif

#ifndef PAL_HAP_DEVP_FTM_PATH
#define PAL_HAP_DEVP_FTM_PATH "/data/vendor/audio/haptics_ftm.cal"
#endif

#define PAL_HP_VI_PER_PATH "/data/vendor/audio/haptics_persistent.cal"

#define MIN_HAPTICS_DEV_IDLE_SEC (60 * 1)
#define WAKEUP_MIN_IDLE_CHECK (1000 * 30)

#define HAPTICS_DEV_RIGHT_WSA_TEMP "HapticsDevRight WSA Temp"
#define HAPTICS_DEV_LEFT_WSA_TEMP "HapticsDevLeft WSA Temp"
/*Set safe temp value to 40C*/
#define SAFE_HAPTICS_DEV_TEMP 40
#define SAFE_HAPTICS_DEV_TEMP_Q6 (SAFE_HAPTICS_DEV_TEMP * (1 << 6))

#define MIN_RESISTANCE_HAPTICS_DEV_Q24 (2 * (1 << 24))

#define DEFAULT_PERIOD_SIZE 256
#define DEFAULT_PERIOD_COUNT 4

#define NORMAL_MODE 0
#define CALIBRATION_MODE 1
#define FACTORY_TEST_MODE 2


typedef enum haptics_vi_calib_state_t
{

    HAPTICS_VI_CALIB_STATE_INCORRECT_OP_MODE = 0, // returned if "operation_mode" is not "Thermal Calibration"

    HAPTICS_VI_CALIB_STATE_INACTIVE = 1, // init value

    HAPTICS_VI_CALIB_STATE_WARMUP = 2, // wait state for warmup

    HAPTICS_VI_CALIB_STATE_INPROGRESS = 3, // in calibration state

    HAPTICS_VI_CALIB_STATE_SUCCESS = 4, // calibration successful

    HAPTICS_VI_CALIB_STATE_FAILED = 5, // calibration failed

    HAPTICS_VI_CALIB_STATE_WAIT_FOR_VI = 6, // wait state for vi threshold

    HAPTICS_VI_CALIB_STATE_VI_WAIT_TIMED_OUT = 7, // calibration could not start due to vi signals below threshold

    HAPTICS_VI_CALIB_STATE_LOW_VI = 8, // calibration failed due to vi signals below threshold

    HAPTICS_VI_CALIB_STATE_MAX_VAL = 0xFFFFFFFF // // max 32-bit unsigned value. tells the compiler to use 32-bit data type

} haptics_vi_calib_state_t;

/** @h2xmlp_parameter   {"HAPTICS_VI_CALIB_PARAM", HAPTICS_VI_CALIB_PARAM}

  @h2xmlp_description {Calibration/FTM VI persistent parameter}

  @h2xmlp_toolPolicy  {RTC_READONLY}*/

typedef struct haptics_vi_calib_param_t {

    haptics_vi_calib_state_t state; // Calibration/FTM state

    uint32_t operation_mode;        // Calibration or FTM

    /**< @h2xmle_description {Operation mode in which parameters were estimated.}

      @h2xmle_rangeList   {"Calibration mode"=1;

      "Factory Test Mode"=2} */

    int32_t Re_ohm_q24[HAPTICS_MAX_OUT_CHAN];   // LRA resistance from Calibration/FTM

    int32_t Fres_Hz_q20[HAPTICS_MAX_OUT_CHAN];  // Resonance frequency from Calibration/FTM

    int32_t Bl_q24[HAPTICS_MAX_OUT_CHAN]; // Force factor (Bl) from Calibration/FTM

    int32_t Rms_KgSec_q24[HAPTICS_MAX_OUT_CHAN]; // Mechanical damping from Calibration/FTM

    int32_t Blq_ftm_q24[HAPTICS_MAX_OUT_CHAN];      // Non-linearity estimate for Bl (normalized value) from FTM

    int32_t Le_mH_ftm_q24[HAPTICS_MAX_OUT_CHAN];    // LRA inductance from FTM

} haptics_vi_calib_param_t;

std::thread HapticsDevProtection::mCalThread;
std::condition_variable HapticsDevProtection::cv;
std::mutex HapticsDevProtection::cvMutex;
std::mutex HapticsDevProtection::calibrationMutex;

bool HapticsDevProtection::isHapDevInUse;
std::atomic<bool> HapticsDevProtection::calThrdCreated;
std::atomic<bool> HapticsDevProtection::ftmThrdCreated;
bool HapticsDevProtection::isDynamicCalTriggered = false;
struct timespec HapticsDevProtection::devLastTimeUsed;
struct mixer *HapticsDevProtection::virtMixer;
struct mixer *HapticsDevProtection::hwMixer;
haptics_dev_prot_cal_state HapticsDevProtection::hapticsDevCalState;
haptics_vi_cal_param HapticsDevProtection::cbCalData[HAPTICS_MAX_OUT_CHAN];
struct pcm * HapticsDevProtection::rxPcm = NULL;
struct pcm * HapticsDevProtection::txPcm = NULL;
int HapticsDevProtection::numberOfChannels;
struct pal_device_info HapticsDevProtection::vi_device;
int HapticsDevProtection::calibrationCallbackStatus;
int HapticsDevProtection::numberOfRequest;
bool HapticsDevProtection::mDspCallbackRcvd;
std::shared_ptr<Device> HapticsDevFeedback::obj = nullptr;
int HapticsDevFeedback::numDevice;

std::string getDefaultHapticsDevTempCtrl(uint8_t haptics_dev_pos)
{
    switch(haptics_dev_pos)
    {
        case HAPTICS_DEV_LEFT:
            return std::string(HAPTICS_DEV_LEFT_WSA_TEMP);
        break;
        case HAPTICS_DEV_RIGHT:
            [[fallthrough]];
        default:
            return std::string(HAPTICS_DEV_RIGHT_WSA_TEMP);
    }
}

/* Function to check if  HapticsDevice is in use or not.
 * It returns the time as well for which HapticsDevice is not in use.
 */
bool HapticsDevProtection::isHapticsDevInUse(unsigned long *sec)
{
    struct timespec temp;
    PAL_DBG(LOG_TAG, "Enter");

    if (!sec) {
        PAL_ERR(LOG_TAG, "Improper argument");
        return false;
    }

    if (isHapDevInUse) {
        PAL_INFO(LOG_TAG, " HapticsDevice in use");
        *sec = 0;
        return true;
    } else {
        PAL_INFO(LOG_TAG, " HapticsDevice not in use");
        clock_gettime(CLOCK_BOOTTIME, &temp);
        *sec = temp.tv_sec - devLastTimeUsed.tv_sec;
    }

    PAL_DBG(LOG_TAG, "Idle time %ld", *sec);

    return false;
}

/* Function to set status of HapticsDevice */
void HapticsDevProtection::HapticsDevProtSetDevStatus(bool enable)
{
    PAL_DBG(LOG_TAG, "Enter");

    if (enable)
        isHapDevInUse = true;
    else {
        isHapDevInUse = false;
        clock_gettime(CLOCK_BOOTTIME, &devLastTimeUsed);
        PAL_INFO(LOG_TAG, " HapticsDevice used last time %ld", devLastTimeUsed.tv_sec);
    }

    PAL_DBG(LOG_TAG, "Exit");
}

/* Wait function for WAKEUP_MIN_IDLE_CHECK  */
void HapticsDevProtection::HapticsDevCalibrateWait()
{
    std::unique_lock<std::mutex> lock(cvMutex);
    cv.wait_for(lock,
            std::chrono::milliseconds(WAKEUP_MIN_IDLE_CHECK));
}

// Callback from DSP for Resistance value
void HapticsDevProtection::handleHPCallback(uint64_t hdl __unused, uint32_t event_id,
                                            void *event_data, uint32_t event_size)
{
    haptics_vi_calib_param_t *param_data = nullptr;

    PAL_DBG(LOG_TAG, "Got event from DSP 0x%x", event_id);

    if (event_id == EVENT_ID_HAPTICS_VI_CALIBRATION) {
        // Received callback for Calibration state
        param_data = static_cast<haptics_vi_calib_param_t*>(event_data);
        PAL_DBG(LOG_TAG, "Calibration state %d", param_data->state);

        if (param_data->state == HAPTICS_VI_CALIB_STATE_SUCCESS) {
            PAL_DBG(LOG_TAG, "Calibration is successful");
            cbCalData[0].Re_ohm_Cal_q24 = param_data->Re_ohm_q24[0];
            cbCalData[0].Fres_Hz_Cal_q20 = param_data->Fres_Hz_q20[0];
            cbCalData[0].Bl_q24 = param_data->Bl_q24[0];
            cbCalData[0].Rms_KgSec_q24 = param_data->Rms_KgSec_q24[0];
            cbCalData[0].Blq_ftm_q24 = param_data->Blq_ftm_q24[0];
            cbCalData[0].Le_mH_ftm_q24 = param_data->Le_mH_ftm_q24[0];

            mDspCallbackRcvd = true;
            calibrationCallbackStatus = HAPTICS_VI_CALIB_STATE_SUCCESS;
            cv.notify_all();
        } else if ((param_data->state == HAPTICS_VI_CALIB_STATE_FAILED) ||
            (param_data->state == HAPTICS_VI_CALIB_STATE_VI_WAIT_TIMED_OUT) ||
            (param_data->state == HAPTICS_VI_CALIB_STATE_LOW_VI)) {
            PAL_DBG(LOG_TAG, "Calibration unsuccessful, state:%d", param_data->state);
            mDspCallbackRcvd = true;
            calibrationCallbackStatus = HAPTICS_VI_CALIB_STATE_FAILED;
            cv.notify_all();
        } else if (param_data->state == HAPTICS_VI_CALIB_STATE_WAIT_FOR_VI) {
            PAL_DBG(LOG_TAG, "Low VI threshold continue cal, state:%d", param_data->state);
        } else {
            PAL_DBG(LOG_TAG, "unexpected cal state :%d, failing cal step", param_data->state);
            mDspCallbackRcvd = true;
            calibrationCallbackStatus = HAPTICS_VI_CALIB_STATE_FAILED;
            cv.notify_all();
        }
    } else {
            PAL_ERR(LOG_TAG, "received unexpected event id:0x%x", event_id);
            // Restart the calibration and abort current run.
            mDspCallbackRcvd = true;
            calibrationCallbackStatus = HAPTICS_VI_CALIB_STATE_FAILED;
            cv.notify_all();
    }
}

void HapticsDevProtection::disconnectFeandBe(std::vector<int> pcmDevIds,
                                         std::string backEndName) {

    std::ostringstream disconnectCtrlName;
    std::ostringstream disconnectCtrlNameBe;
    struct mixer_ctl *disconnectCtrl = nullptr;
    struct mixer_ctl *beMetaDataMixerCtrl = nullptr;
    struct agmMetaData deviceMetaData(nullptr, 0);
    uint32_t devicePropId[] = { 0x08000010, 2, 0x2, 0x5 };
    std::vector <std::pair<int, int>> emptyKV;
    int ret = 0;

    emptyKV.clear();

    SessionAlsaUtils::getAgmMetaData(emptyKV, emptyKV,
                    (struct prop_data *)devicePropId, deviceMetaData);
    if (!deviceMetaData.size) {
        ret = -ENOMEM;
        PAL_ERR(LOG_TAG, "Error: %d, Device metadata is zero", ret);
        goto exit;
    }

    disconnectCtrlNameBe << backEndName << " metadata";
    beMetaDataMixerCtrl = mixer_get_ctl_by_name(virtMixer, disconnectCtrlNameBe.str().data());
    if (!beMetaDataMixerCtrl) {
        ret = -EINVAL;
        PAL_ERR(LOG_TAG, "Error: %d, invalid mixer control %s", ret, backEndName.c_str());
        goto exit;
    }

    if (deviceMetaData.size) {
        ret = mixer_ctl_set_array(beMetaDataMixerCtrl, (void *)deviceMetaData.buf,
                    deviceMetaData.size);
        free(deviceMetaData.buf);
        deviceMetaData.buf = nullptr;
    } else {
        PAL_ERR(LOG_TAG, "Error: %d, Device Metadata not cleaned up", ret);
        goto exit;
    }

    disconnectCtrlName << "PCM" << pcmDevIds.at(0) << " disconnect";
    disconnectCtrl = mixer_get_ctl_by_name(virtMixer, disconnectCtrlName.str().data());
    if (!disconnectCtrl) {
        ret = -EINVAL;
        PAL_ERR(LOG_TAG, "Error: %d, invalid mixer control: %s", ret, disconnectCtrlName.str().data());
        goto exit;
    }
    ret = mixer_ctl_set_enum_by_string(disconnectCtrl, backEndName.c_str());
    if (ret) {
        PAL_ERR(LOG_TAG, "Error: %d, Mixer control %s set with %s failed", ret,
        disconnectCtrlName.str().data(), backEndName.c_str());
    }
exit:
    return;
}

int HapticsDevProtection::HapticsDevStartCalibration(int32_t operation_mode)
{
    struct pal_device device, deviceRx;
    struct pal_channel_info ch_info;
    struct pal_stream_attributes sAttr;
    struct pcm_config config;
    struct mixer_ctl *connectCtrl = nullptr;
    struct audio_route *audioRoute = nullptr;
    struct agm_event_reg_cfg event_cfg;
    struct agmMetaData deviceMetaData(nullptr, 0);
    struct mixer_ctl *beMetaDataMixerCtrl = nullptr;
    int ret = 0, status = 0, dir = 0, i = 0, flags = 0, payload_size = 0;
    uint32_t miid = 0;
    char mSndDeviceName_rx[128] = {0};
    char mSndDeviceName_vi[128] = {0};
    uint8_t* payload = nullptr;
    size_t payloadSize = 0;
    uint32_t devicePropId[] = {0x08000010, 1, 0x2};
    bool isTxStarted = false, isRxStarted = false;
    bool isTxFeandBeConnected = false, isRxFeandBeConnected = false;
    std::string backEndNameTx, backEndNameRx;
    std::vector<std::pair<int, int>> keyVector, calVector;
    std::vector<int> pcmDevIdsRx, pcmDevIdsTx;
    std::shared_ptr<ResourceManager> rm;
    std::ostringstream connectCtrlName;
    std::ostringstream connectCtrlNameRx;
    std::ostringstream connectCtrlNameBe;
    std::ostringstream connectCtrlNameBeVI;
    param_id_haptics_vi_op_mode_param_t modeConfg;
    param_id_haptics_op_mode HpRxModeConfg;
    param_id_haptics_vi_channel_map_cfg_t HapticsviChannelMapConfg;
    session_callback sessionCb;

    std::unique_lock<std::mutex> calLock(calibrationMutex);

    memset(&device, 0, sizeof(device));
    memset(&deviceRx, 0, sizeof(deviceRx));
    memset(&sAttr, 0, sizeof(sAttr));
    memset(&config, 0, sizeof(config));
    memset(&modeConfg, 0, sizeof(modeConfg));
    memset(&HapticsviChannelMapConfg, 0, sizeof(HapticsviChannelMapConfg));
    memset(&HpRxModeConfg, 0, sizeof(HpRxModeConfg));

    sessionCb = handleHPCallback;
    PayloadBuilder* builder = new PayloadBuilder();

    keyVector.clear();
    calVector.clear();

    PAL_DBG(LOG_TAG, "Enter");

    if (customPayloadSize) {
        free(customPayload);
        customPayloadSize = 0;
    }

    rm = ResourceManager::getInstance();
    if (!rm) {
        PAL_ERR(LOG_TAG, "Error: %d Failed to get resource manager instance", -EINVAL);
        goto exit;
    }

    // Configure device attribute
    switch (vi_device.channels) {
        case 1 :
            ch_info.channels = CHANNELS_1;
        break;
        case 2 :
            ch_info.channels = CHANNELS_2;
        break;
        default:
            PAL_ERR(LOG_TAG, "Unsupported channel. Set default as 2");
            ch_info.channels = CHANNELS_2;
        break;
    }
    rm->getChannelMap(&(ch_info.ch_map[0]), ch_info.channels);

    device.config.ch_info = ch_info;
    device.config.sample_rate = vi_device.samplerate;
    device.config.bit_width = vi_device.bit_width;
    device.config.aud_fmt_id = rm->getAudioFmt(vi_device.bit_width);

    // Setup TX path
    ret = rm->getAudioRoute(&audioRoute);
    if (0 != ret) {
        PAL_ERR(LOG_TAG, "Failed to get the audio_route name status %d", ret);
        goto exit;
    }

    device.id = PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK;
    ret = rm->getSndDeviceName(device.id , mSndDeviceName_vi);
    if (0 != ret) {
        PAL_ERR(LOG_TAG, "Failed to obtain vi snd device name for %d", device.id);
        goto exit;
    }

    PAL_DBG(LOG_TAG, "VI snd device name %s", mSndDeviceName_vi);

    rm->getBackendName(device.id, backEndNameTx);
    if (!strlen(backEndNameTx.c_str())) {
        PAL_ERR(LOG_TAG, "Failed to obtain vi backend name for %d", device.id);
        goto exit;
    }

    ret = PayloadBuilder::getDeviceKV(device.id, keyVector);
    if (0 != ret) {
        PAL_ERR(LOG_TAG, "Failed to obtain device KV for %d", device.id);
        goto exit;
    }

    // Enable VI module
    switch(ch_info.channels) {
        case 1 :
            // TODO: check it from RM.xml for left or right configuration
            calVector.push_back(std::make_pair(HAPTICS_PRO_VI_MAP, HAPTICS_VI_LEFT_MONO));
        break;
        case 2 :
            calVector.push_back(std::make_pair(HAPTICS_PRO_VI_MAP, HAPTICS_VI_LEFT_RIGHT));
        break;
        default :
            PAL_ERR(LOG_TAG, "Unsupported channel, %d", ch_info.channels);
            goto exit;
    }

    SessionAlsaUtils::getAgmMetaData(keyVector, calVector,
                    (struct prop_data *)devicePropId, deviceMetaData);
    if (!deviceMetaData.size) {
        PAL_ERR(LOG_TAG, "VI device metadata is zero");
        ret = -ENOMEM;
        goto exit;
    }

    connectCtrlNameBeVI << backEndNameTx << " metadata";
    beMetaDataMixerCtrl = mixer_get_ctl_by_name(virtMixer, connectCtrlNameBeVI.str().data());
    if (!beMetaDataMixerCtrl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control for VI: %s", backEndNameTx.c_str());
        ret = -EINVAL;
        goto exit;
    }

    if (deviceMetaData.size) {
        ret = mixer_ctl_set_array(beMetaDataMixerCtrl, (void *)deviceMetaData.buf,
                    deviceMetaData.size);
        free(deviceMetaData.buf);
        deviceMetaData.buf = nullptr;
    } else {
        PAL_ERR(LOG_TAG, "Device Metadata not set for TX path");
        ret = -EINVAL;
        goto exit;
    }

    ret = SessionAlsaUtils::setDeviceMediaConfig(rm, backEndNameTx, &device);
    if (ret) {
        PAL_ERR(LOG_TAG, "setDeviceMediaConfig for VI feedback device failed");
        goto exit;
    }

    sAttr.type = PAL_STREAM_HAPTICS;
    sAttr.direction = PAL_AUDIO_INPUT_OUTPUT;
    dir = TX_HOSTLESS;
    pcmDevIdsTx = rm->allocateFrontEndIds(sAttr, dir);
    if (pcmDevIdsTx.size() == 0) {
        PAL_ERR(LOG_TAG, "allocateFrontEndIds failed");
        ret = -ENOSYS;
        goto exit;
    }

    connectCtrlName << "PCM" << pcmDevIdsTx.at(0) << " connect";
    connectCtrl = mixer_get_ctl_by_name(virtMixer, connectCtrlName.str().data());
    if (!connectCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s", connectCtrlName.str().data());
        goto free_fe;
    }
    ret = mixer_ctl_set_enum_by_string(connectCtrl, backEndNameTx.c_str());
    if (ret) {
        PAL_ERR(LOG_TAG, "Mixer control %s set with %s failed: %d",
        connectCtrlName.str().data(), backEndNameTx.c_str(), ret);
        goto free_fe;
    }

    isTxFeandBeConnected = true;

    config.rate = vi_device.samplerate;
    config.format = SessionAlsaUtils::palToAlsaFormat(device.config.aud_fmt_id);
    config.channels = ch_info.channels;
    config.period_size = DEFAULT_PERIOD_SIZE;
    config.period_count = DEFAULT_PERIOD_COUNT;
    config.start_threshold = 0;
    config.stop_threshold = INT_MAX;
    config.silence_threshold = 0;

    flags = PCM_IN;

    ret = SessionAlsaUtils::getModuleInstanceId(virtMixer, pcmDevIdsTx.at(0),
                                                backEndNameTx.c_str(),
                                                MODULE_HAPTICS_VI, &miid);
    if (0 != ret) {
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", MODULE_HAPTICS_VI, ret);
        goto free_fe;
    }
    // Setting mode for IV module
    modeConfg.th_operation_mode = operation_mode;
    builder->payloadHapticsDevPConfig(&payload, &payloadSize, miid,
            PARAM_ID_HAPTICS_VI_OP_MODE_PARAM, (void *)&modeConfg);
    if (payloadSize) {
        ret = updateCustomPayload(payload, payloadSize);
        free(payload);
        if (0 != ret) {
            PAL_ERR(LOG_TAG,"updateCustomPayload Failed for VI_OP_MODE_CFG\n");
            goto free_fe;
        }
    }

    // Setting Channel Map configuration for VI module
    // TODO: Move this to ACDB
    HapticsviChannelMapConfg.num_ch = numberOfChannels * 2;
    payloadSize = 0;

    builder->payloadHapticsDevPConfig(&payload, &payloadSize, miid,
              PARAM_ID_HAPTICS_VI_CHANNEL_MAP_CFG, (void *)&HapticsviChannelMapConfg);
    if (payloadSize) {
        ret = updateCustomPayload(payload, payloadSize);
        free(payload);
        if (0 != ret) {
            PAL_ERR(LOG_TAG," updateCustomPayload Failed for CHANNEL_MAP_CFG\n");
            // Not a fatal error
            ret = 0;
        }
    }

    // Setting the values for VI module
    if (customPayloadSize) {
        ret = SessionAlsaUtils::setDeviceCustomPayload(rm, backEndNameTx,
                    customPayload, customPayloadSize);
        if (ret) {
            PAL_ERR(LOG_TAG, "Unable to set custom param for mode");
            goto free_fe;
        }
    }

    txPcm = pcm_open(rm->getVirtualSndCard(), pcmDevIdsTx.at(0), flags, &config);
    if (!txPcm) {
        PAL_ERR(LOG_TAG, "txPcm open failed");
        goto free_fe;
    }

    if (!pcm_is_ready(txPcm)) {
        PAL_ERR(LOG_TAG, "txPcm open not ready");
        goto err_pcm_open;
    }

    //Register for VI module callback
    PAL_DBG(LOG_TAG, "registering event for VI module");
    payload_size = sizeof(struct agm_event_reg_cfg);

    event_cfg.event_id = EVENT_ID_HAPTICS_VI_CALIBRATION;
    event_cfg.event_config_payload_size = 0;
    event_cfg.is_register = 1;

    ret = SessionAlsaUtils::registerMixerEvent(virtMixer, pcmDevIdsTx.at(0),
                      backEndNameTx.c_str(), MODULE_HAPTICS_VI, (void *)&event_cfg,
                      payload_size);
    if (ret) {
        PAL_ERR(LOG_TAG, "Unable to register event HAP_VI_CALIB");
        // Fatal Error. Calibration Won't work
        goto err_pcm_open;
    }

    // Register to mixtureControlEvents and wait for Re, F0 and Blq values
    ret = rm->registerMixerEventCallback(pcmDevIdsTx, sessionCb, (uint64_t)this, true);
    if (ret != 0) {
        PAL_ERR(LOG_TAG, "Failed to register callback to rm");
        // Fatal Error. Calibration Won't work
        goto err_pcm_open;
    }

    enableDevice(audioRoute, mSndDeviceName_vi);

    PAL_DBG(LOG_TAG, "pcm start for TX path");
    if (pcm_start(txPcm) < 0) {
        PAL_ERR(LOG_TAG, "pcm start failed for TX path");
        ret = -ENOSYS;
        goto err_pcm_open;
    }
    isTxStarted = true;

    // Setup RX path
    deviceRx.id = PAL_DEVICE_OUT_HAPTICS_DEVICE;
    ret = rm->getSndDeviceName(deviceRx.id, mSndDeviceName_rx);
    if (0 != ret) {
        PAL_ERR(LOG_TAG, "Failed to obtain the rx snd device name");
        goto err_pcm_open;
    }
    deviceRx.config.ch_info.channels = numberOfChannels;
    rm->getChannelMap(&(deviceRx.config.ch_info.ch_map[0]), deviceRx.config.ch_info.channels);

    // TODO: Fetch these data from rm.xml
    deviceRx.config.sample_rate = SAMPLINGRATE_48K;
    deviceRx.config.bit_width = BITWIDTH_16;
    deviceRx.config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;

    rm->getBackendName(deviceRx.id, backEndNameRx);
    if (!strlen(backEndNameRx.c_str())) {
        PAL_ERR(LOG_TAG, "Failed to obtain rx backend name for %d", deviceRx.id);
        goto err_pcm_open;
    }

    keyVector.clear();
    calVector.clear();

    PayloadBuilder::getDeviceKV(deviceRx.id, keyVector);
    if (0 != ret) {
        PAL_ERR(LOG_TAG, "Failed to obtain device KV for %d", deviceRx.id);
        goto err_pcm_open;
    }

    // Enable the Haptics Gen module
    switch (numberOfChannels) {
        case 1 :
            // TODO: Fetch the configuration from RM.xml
            calVector.push_back(std::make_pair(HAPTICS_PRO_DEV_MAP, HAPTICS_LEFT_MONO));
        break;
        case 2 :
            calVector.push_back(std::make_pair(HAPTICS_PRO_DEV_MAP, HAPTICS_LEFT_RIGHT));
        break;
        default :
            PAL_ERR(LOG_TAG, "Unsupported channels for Haptics RX Device");
            goto err_pcm_open;
    }

    SessionAlsaUtils::getAgmMetaData(keyVector, calVector,
                    (struct prop_data *)devicePropId, deviceMetaData);
    if (!deviceMetaData.size) {
        PAL_ERR(LOG_TAG, "device metadata is zero");
        ret = -ENOMEM;
        goto err_pcm_open;
    }

    connectCtrlNameBe << backEndNameRx << " metadata";

    beMetaDataMixerCtrl = mixer_get_ctl_by_name(virtMixer, connectCtrlNameBe.str().data());
    if (!beMetaDataMixerCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s", backEndNameRx.c_str());
        ret = -EINVAL;
        goto err_pcm_open;
    }

    if (deviceMetaData.size) {
        ret = mixer_ctl_set_array(beMetaDataMixerCtrl, (void *)deviceMetaData.buf,
                                    deviceMetaData.size);
        free(deviceMetaData.buf);
        deviceMetaData.buf = nullptr;
    } else {
        PAL_ERR(LOG_TAG, "Device Metadata not set for RX path");
        ret = -EINVAL;
        goto err_pcm_open;
    }

    ret = SessionAlsaUtils::setDeviceMediaConfig(rm, backEndNameRx, &deviceRx);
    if (ret) {
        PAL_ERR(LOG_TAG, "setDeviceMediaConfig for Haptics RX Device failed");
        goto err_pcm_open;
    }

    /* Retrieve Hostless PCM device id */
    sAttr.type = PAL_STREAM_HAPTICS;
    sAttr.direction = PAL_AUDIO_INPUT_OUTPUT;
    dir = RX_HOSTLESS;
    pcmDevIdsRx = rm->allocateFrontEndIds(sAttr, dir);
    if (pcmDevIdsRx.size() == 0) {
        PAL_ERR(LOG_TAG, "allocateFrontEndIds failed");
        ret = -ENOSYS;
        goto err_pcm_open;
    }

    connectCtrlNameRx << "PCM" << pcmDevIdsRx.at(0) << " connect";
    connectCtrl = mixer_get_ctl_by_name(virtMixer, connectCtrlNameRx.str().data());
    if (!connectCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s", connectCtrlNameRx.str().data());
        ret = -ENOSYS;
        goto err_pcm_open;
    }
    ret = mixer_ctl_set_enum_by_string(connectCtrl, backEndNameRx.c_str());
    if (ret) {
        PAL_ERR(LOG_TAG, "Mixer control %s set with %s failed: %d",
        connectCtrlNameRx.str().data(), backEndNameRx.c_str(), ret);
        goto err_pcm_open;
    }
    isRxFeandBeConnected = true;

    config.rate = SAMPLINGRATE_48K;
    config.format = PCM_FORMAT_S16_LE;
    config.channels = numberOfChannels;
    config.period_size = DEFAULT_PERIOD_SIZE;
    config.period_count = DEFAULT_PERIOD_COUNT;
    config.start_threshold = 0;
    config.stop_threshold = INT_MAX;
    config.silence_threshold = 0;

    flags = PCM_OUT;

    // Set the operation mode for Haptics RX module
    HpRxModeConfg.operation_mode = operation_mode; //CALIBRATION_MODE;
    ret = SessionAlsaUtils::getModuleInstanceId(virtMixer, pcmDevIdsRx.at(0),
                                                backEndNameRx.c_str(),
                                                MODULE_HAPTICS_GEN, &miid);
    if (0 != ret) {
        PAL_ERR(LOG_TAG, "Failed to get miid info %x(%d)", MODULE_HAPTICS_GEN, ret);
        goto err_pcm_open;
    }

    payloadSize = 0;
    builder->payloadHapticsDevPConfig(&payload, &payloadSize, miid,
            PARAM_ID_HAPTICS_OP_MODE,(void *)&HpRxModeConfg);
    if (payloadSize) {
        if (customPayloadSize) {
            free(customPayload);
            customPayloadSize = 0;
        }

        ret = updateCustomPayload(payload, payloadSize);
        free(payload);
        if (0 != ret) {
            PAL_ERR(LOG_TAG," updateCustomPayload Failed\n");
            // Fatal error as SP module will not run in Calibration mode
            goto err_pcm_open;
        }
    }

    // Setting the values for Haptics RX module
    if (customPayloadSize) {
        ret = SessionAlsaUtils::setDeviceCustomPayload(rm, backEndNameRx,
                    customPayload, customPayloadSize);
        if (ret) {
            PAL_ERR(LOG_TAG, "Unable to set custom param for calibration mode");
            free(customPayload);
            customPayloadSize = 0;
            goto err_pcm_open;
        }
    }

    rxPcm = pcm_open(rm->getVirtualSndCard(), pcmDevIdsRx.at(0), flags, &config);
    if (!rxPcm) {
        PAL_ERR(LOG_TAG, "pcm open failed for RX path");
        ret = -ENOSYS;
        goto err_pcm_open;
    }

    if (!pcm_is_ready(rxPcm)) {
        PAL_ERR(LOG_TAG, "pcm open not ready for RX path");
        ret = -ENOSYS;
        goto err_pcm_open;
    }

    enableDevice(audioRoute, mSndDeviceName_rx);
    PAL_DBG(LOG_TAG, "pcm start for RX path");
    if (pcm_start(rxPcm) < 0) {
        PAL_ERR(LOG_TAG, "pcm start failed for RX path");
        ret = -ENOSYS;
        goto err_pcm_open;
    }
    isRxStarted = true;

    hapticsDevCalState = HAPTICS_DEV_CALIB_IN_PROGRESS;

    PAL_DBG(LOG_TAG, "Waiting for the event from DSP or PAL");

    // TODO: Make this to wait in While loop
    //cv.wait(calLock);
    if (cv.wait_for(calLock, std::chrono::seconds(5)) == std::cv_status::timeout) {
        PAL_ERR(LOG_TAG, "Timeout occured! for VI calibration");
        goto done;
    }

    // Store haptics calibrated values Re, f0, Blq
    if (mDspCallbackRcvd) {
        if (calibrationCallbackStatus == HAPTICS_VI_CALIB_STATE_SUCCESS) {
            PAL_DBG(LOG_TAG, "haptics calibration success! op_mode:%d", operation_mode);
            std::ofstream outFile;
            if (operation_mode == FACTORY_TEST_MODE)
                outFile.open(PAL_HAP_DEVP_FTM_PATH, std::ios::binary);
            else
                outFile.open(PAL_HAP_DEVP_CAL_PATH, std::ios::binary);
            if (!outFile.is_open()) {
                PAL_ERR(LOG_TAG, "Unable to open file for write");
            } else {
                PAL_DBG(LOG_TAG, "Write calibrated values to file");
                for (i = 0; i < numberOfChannels; i++) {
                      outFile.write(reinterpret_cast<char*>(&cbCalData[i]), sizeof(cbCalData[0]));
                }
                hapticsDevCalState = HAPTICS_DEV_CALIBRATED;
                outFile.close();
            }
        } else if (calibrationCallbackStatus == HAPTICS_VI_CALIB_STATE_FAILED) {
            PAL_DBG(LOG_TAG, "haptics calibration unsuccessful!");
            hapticsDevCalState = HAPTICS_DEV_NOT_CALIBRATED;
            // reset the timer for retry
            clock_gettime(CLOCK_BOOTTIME, &devLastTimeUsed);
        }
    }

done:
err_pcm_open :
    if (txPcm) {
        event_cfg.is_register = 0;

        status = SessionAlsaUtils::registerMixerEvent(virtMixer, pcmDevIdsTx.at(0),
                        backEndNameTx.c_str(), MODULE_HAPTICS_VI, (void *)&event_cfg,
                        payload_size);
        if (status) {
            PAL_ERR(LOG_TAG, "Unable to deregister event to DSP");
        }
        status = rm->registerMixerEventCallback (pcmDevIdsTx, sessionCb, (uint64_t)this, false);
        if (status) {
            PAL_ERR(LOG_TAG, "Failed to deregister callback to rm");
        }
        if (isTxStarted)
            pcm_stop(txPcm);

        pcm_close(txPcm);
        disableDevice(audioRoute, mSndDeviceName_vi);

        txPcm = NULL;
    }

    if (rxPcm) {
        if (isRxStarted)
            pcm_stop(rxPcm);
        pcm_close(rxPcm);
        disableDevice(audioRoute, mSndDeviceName_rx);
        rxPcm = NULL;
    }

free_fe:
    if (pcmDevIdsRx.size() != 0) {
        if (isRxFeandBeConnected) {
            disconnectFeandBe(pcmDevIdsRx, backEndNameRx);
        }
        rm->freeFrontEndIds(pcmDevIdsRx, sAttr, RX_HOSTLESS);
    }

    if (pcmDevIdsTx.size() != 0) {
        if (isTxFeandBeConnected) {
            disconnectFeandBe(pcmDevIdsTx, backEndNameTx);
        }
        rm->freeFrontEndIds(pcmDevIdsTx, sAttr, TX_HOSTLESS);
    }
    pcmDevIdsRx.clear();
    pcmDevIdsTx.clear();

exit:

    if (!mDspCallbackRcvd) {
        // the lock is unlocked due to processing mode. It will be waiting
        // for the unlock. So notify it.
        PAL_DBG(LOG_TAG, "Unlocked due to processing mode");
        hapticsDevCalState = HAPTICS_DEV_NOT_CALIBRATED;
        clock_gettime(CLOCK_BOOTTIME, &devLastTimeUsed);
        cv.notify_all();
    }

    if (ret != 0) {
        // Error happened. Reset timer
        clock_gettime(CLOCK_BOOTTIME, &devLastTimeUsed);
    }

    if(builder) {
       delete builder;
       builder = NULL;
    }
    PAL_DBG(LOG_TAG, "Exiting");
    return ret;
}

/* LRA temperature unused currently */
int HapticsDevProtection::getDevTemperature(int haptics_dev_pos)
{
    struct mixer_ctl *ctl;
    std::string mixer_ctl_name;
    int status = 0;

    PAL_DBG(LOG_TAG, "Enter: HapticsDevice Get Temperature %d", haptics_dev_pos);
    if (mixer_ctl_name.empty()) {
        PAL_DBG(LOG_TAG, "Using default mixer control");
        mixer_ctl_name = getDefaultHapticsDevTempCtrl(haptics_dev_pos);
    }

    PAL_DBG(LOG_TAG, "audio_mixer %pK", hwMixer);

    ctl = mixer_get_ctl_by_name(hwMixer, mixer_ctl_name.c_str());
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", mixer_ctl_name.c_str());
        status = -EINVAL;
        return status;
    }

    status = mixer_ctl_get_value(ctl, 0);

    PAL_DBG(LOG_TAG, "Exit: HapticsDevice Get Temperature %d", status);

    return status;
}

/**
  * This function sets the temperature of each HapticsDevics.
  * Currently values are supported like:
  * devTempList[0] - Right  HapticsDevice Temperature
  * devTempList[1] - Left  HapticsDevice Temperature
  */
void HapticsDevProtection::getHapticsDevTemperatureList()
{
    int i = 0;
    int value;
    PAL_DBG(LOG_TAG, "Enter  HapticsDevice Get Temperature List");

    for(i = 0; i < numberOfChannels; i++) {
         value = getDevTemperature(i);
         PAL_DBG(LOG_TAG, "Temperature %d ", value);
         devTempList[i] = value;
    }
    PAL_DBG(LOG_TAG, "Exit  HapticsDevice Get Temperature List");
}

void HapticsDevProtection::HapticsDevFTMThread()
{
    PAL_DBG(LOG_TAG, "start Haptics FTM");
    HapticsDevStartCalibration(FACTORY_TEST_MODE);
    ftmThrdCreated.store(false);
    PAL_DBG(LOG_TAG, "Haptics FTM done, exiting ftmThread");
}

void HapticsDevProtection::HapticsDevCalibrationThread()
{
    unsigned long sec = 0;
    bool proceed = false;
    int i;
    int retry = 2;

    while (!threadExit && retry) {
        PAL_DBG(LOG_TAG, "Inside calibration while loop");
        proceed = false;
        if (isHapticsDevInUse(&sec)) {
            PAL_DBG(LOG_TAG, "HapticsDev in use. Wait for proper time");
            HapticsDevCalibrateWait();
            PAL_DBG(LOG_TAG, "Waiting done");
            continue;
        } else {
            PAL_DBG(LOG_TAG, "HapticsDev not in use");
            if (isDynamicCalTriggered) {
                PAL_DBG(LOG_TAG, "Dynamic Calibration triggered");
            } else if (0) /*(sec < minIdleTime)*/ {
                PAL_DBG(LOG_TAG, "HapticsDev not idle for minimum time. %lu", sec);
                HapticsDevCalibrateWait();
                PAL_DBG(LOG_TAG, "Waited for HapticsDev to be idle for min time");
                continue;
            }
            proceed = true;
        }

        if (proceed) {
            // Start calibrating the HapticsDevice.
            PAL_DBG(LOG_TAG, "HapticsDevice not in use, start calibration");
            HapticsDevStartCalibration(CALIBRATION_MODE);
            if (hapticsDevCalState == HAPTICS_DEV_CALIBRATED) {
                threadExit = true;
            } else {
                retry--;
            }
        } else {
            continue;
            retry--;
        }
    }
    isDynamicCalTriggered = false;
    calThrdCreated.store(false);
    PAL_DBG(LOG_TAG, "Calibration done, exiting the thread");
}

HapticsDevProtection::HapticsDevProtection(struct pal_device *device,
                        std::shared_ptr<ResourceManager> Rm):HapticsDev(device, Rm)
{
    int status = 0;
    struct pal_device_info devinfo = {};

    minIdleTime = MIN_HAPTICS_DEV_IDLE_SEC;

    rm = Rm;

    memset(&mDeviceAttr, 0, sizeof(struct pal_device));
    memcpy(&mDeviceAttr, device, sizeof(struct pal_device));

    threadExit = false;
    calThrdCreated.store(false);
    ftmThrdCreated.store(false);

    triggerCal = false;
    hapticsDevCalState = HAPTICS_DEV_NOT_CALIBRATED;
    hapticsDevProcessingState = HAPTICS_DEV_PROCESSING_IN_IDLE;

    isHapDevInUse = false;

    rm->getDeviceInfo(PAL_DEVICE_OUT_HAPTICS_DEVICE, PAL_STREAM_PROXY, "", &devinfo);
    numberOfChannels = (devinfo.channels >= 2) ? CHANNELS_2 : CHANNELS_1;
    PAL_DBG(LOG_TAG, "Number of Channels %d", numberOfChannels);

    rm->getDeviceInfo(PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK, PAL_STREAM_PROXY, "", &vi_device);
    PAL_DBG(LOG_TAG, "Number of Channels for VI path is %d", vi_device.channels);

    // Get current time
    clock_gettime(CLOCK_BOOTTIME, &devLastTimeUsed);

    // Getting mixer controls from Resource Manager
    status = rm->getVirtualAudioMixer(&virtMixer);
    if (status) {
        PAL_ERR(LOG_TAG,"virt mixer error %d", status);
    }
    status = rm->getHwAudioMixer(&hwMixer);
    if (status) {
        PAL_ERR(LOG_TAG,"hw mixer error %d", status);
    }

    calibrationCallbackStatus = HAPTICS_VI_CALIB_STATE_INACTIVE;
    mDspCallbackRcvd = false;
    if (!calThrdCreated.load()) {
        mCalThread = std::thread(&HapticsDevProtection::HapticsDevCalibrationThread,
                                 this);
        calThrdCreated.store(true);
    }
}

HapticsDevProtection::~HapticsDevProtection()
{
}

/*
 * Function to trigger Processing mode.
 * The parameter that it accepts are below:
 * true - Start Processing Mode
 * false - Stop Processing Mode
 */
int32_t HapticsDevProtection::HapticsDevProtProcessingMode(bool flag)
{
    int ret = 0, dir = TX_HOSTLESS, flags, viParamId = 0;
    char mSndDeviceName_vi[128] = {0};
    uint8_t* payload = nullptr;
    uint32_t devicePropId[] = {0x08000010, 1, 0x2};
    uint32_t miid = 0;
    bool isTxFeandBeConnected = true;
    size_t payloadSize = 0;
    struct pal_device device;
    struct pal_channel_info ch_info;
    struct pal_stream_attributes sAttr;
    struct pcm_config config;
    struct mixer_ctl *connectCtrl = nullptr;
    struct audio_route *audioRoute = nullptr;
    struct agmMetaData deviceMetaData(nullptr, 0);
    struct mixer_ctl *beMetaDataMixerCtrl = nullptr;
    std::string backEndName, backEndNameRx;
    std::vector <std::pair<int, int>> keyVector;
    std::vector <std::pair<int, int>> calVector;
    std::shared_ptr<ResourceManager> rm;
    std::ostringstream connectCtrlNameBeVI;
    std::ostringstream connectCtrlNameBeHP;
    std::ostringstream connectCtrlName;
    param_id_haptics_vi_op_mode_param_t modeConfg;
    param_id_haptics_vi_channel_map_cfg_t HapticsviChannelMapConfg;
    param_id_haptics_op_mode hpModeConfg;
    std::shared_ptr<Device> dev = nullptr;
    Stream *stream = nullptr;
    Session *session = nullptr;
    std::vector<Stream*> activeStreams;
    PayloadBuilder* builder = new PayloadBuilder();

    std::unique_lock<std::mutex> lock(calibrationMutex);

    PAL_DBG(LOG_TAG, "Flag %d", flag);

    deviceMutex.lock();

    if (flag) {
        if (hapticsDevCalState == HAPTICS_DEV_CALIB_IN_PROGRESS) {
            // Close the Graphs
            cv.notify_all();
            // Wait for cleanup
            cv.wait(lock);
            hapticsDevCalState = HAPTICS_DEV_NOT_CALIBRATED;
            txPcm = nullptr;
            rxPcm = nullptr;
            PAL_DBG(LOG_TAG, "Stopped calibration mode");
        }
        numberOfRequest++;
        if (numberOfRequest > 1) {
            // R0T0 already set, we don't need to process the request.
            goto exit;
        }
        PAL_DBG(LOG_TAG, "Custom payload size %zu, Payload %p", customPayloadSize,
                customPayload);

        if (customPayload) {
            free(customPayload);
        }
        customPayloadSize = 0;
        customPayload = nullptr;

        HapticsDevProtSetDevStatus(flag);
        //  HapticsDevice in use. Start the Processing Mode
        rm = ResourceManager::getInstance();
        if (!rm) {
            PAL_ERR(LOG_TAG, "Failed to get resource manager instance");
            goto exit;
        }

        memset(&device, 0, sizeof(device));
        memset(&sAttr, 0, sizeof(sAttr));
        memset(&config, 0, sizeof(config));
        memset(&modeConfg, 0, sizeof(modeConfg));
        memset(&HapticsviChannelMapConfg, 0, sizeof(HapticsviChannelMapConfg));
        memset(&hpModeConfg, 0, sizeof(hpModeConfg));

        keyVector.clear();
        calVector.clear();

        // Configure device attribute
       if (vi_device.channels > 1) {
            ch_info.channels = CHANNELS_2;
            ch_info.ch_map[0] = PAL_CHMAP_CHANNEL_FL;
            ch_info.ch_map[1] = PAL_CHMAP_CHANNEL_FR;
        }
        else {
            ch_info.channels = CHANNELS_1;
            ch_info.ch_map[0] = PAL_CHMAP_CHANNEL_FL;
        }


        device.config.ch_info = ch_info;
        device.config.sample_rate = vi_device.samplerate;
        device.config.bit_width = vi_device.bit_width;
        device.config.aud_fmt_id = rm->getAudioFmt(vi_device.bit_width);

        // Setup TX path
        device.id = PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK;

        ret = rm->getAudioRoute(&audioRoute);
        if (0 != ret) {
            PAL_ERR(LOG_TAG, "Failed to get the audio_route address status %d", ret);
            goto exit;
        }

        ret = rm->getSndDeviceName(device.id , mSndDeviceName_vi);
        if (0 != ret) {
            PAL_ERR(LOG_TAG, "Failed to obtain tx VI snd device name for %d", device.id);
            goto exit;
        }

        PAL_DBG(LOG_TAG, "VI snd device name %s", mSndDeviceName_vi);

        rm->getBackendName(device.id, backEndName);
        if (!strlen(backEndName.c_str())) {
            PAL_ERR(LOG_TAG, "Failed to obtain tx backend name for %d", device.id);
            goto exit;
        }

        PayloadBuilder::getDeviceKV(device.id, keyVector);
        if (0 != ret) {
            PAL_ERR(LOG_TAG, "Failed to obtain device KV for %d", device.id);
            goto exit;
        }

        // Enable the VI module
        switch (numberOfChannels) {  //or ch_info.channels ? 
            case 1 :
                calVector.push_back(std::make_pair(HAPTICS_PRO_VI_MAP, HAPTICS_VI_LEFT_MONO));
            break;
            case 2 :
                calVector.push_back(std::make_pair(HAPTICS_PRO_VI_MAP, HAPTICS_VI_LEFT_RIGHT));
            break;
            default :
                PAL_ERR(LOG_TAG, "Unsupported channel %d", numberOfChannels);
                goto exit;
        }

        SessionAlsaUtils::getAgmMetaData(keyVector, calVector,
                (struct prop_data *)devicePropId, deviceMetaData);
        if (!deviceMetaData.size) {
            PAL_ERR(LOG_TAG, "VI device metadata is zero");
            ret = -ENOMEM;
            goto exit;
        }
        connectCtrlNameBeVI << backEndName << " metadata";
        beMetaDataMixerCtrl = mixer_get_ctl_by_name(virtMixer,
                                    connectCtrlNameBeVI.str().data());
        if (!beMetaDataMixerCtrl) {
            PAL_ERR(LOG_TAG, "invalid mixer control for VI : %s", backEndName.c_str());
            ret = -EINVAL;
            goto exit;
        }

        if (deviceMetaData.size) {
            ret = mixer_ctl_set_array(beMetaDataMixerCtrl, (void *)deviceMetaData.buf,
                        deviceMetaData.size);
            free(deviceMetaData.buf);
            deviceMetaData.buf = nullptr;
        }
        else {
            PAL_ERR(LOG_TAG, "Device Metadata not set for TX path");
            ret = -EINVAL;
            goto exit;
        }

        ret = SessionAlsaUtils::setDeviceMediaConfig(rm, backEndName, &device);
        if (ret) {
            PAL_ERR(LOG_TAG, "setDeviceMediaConfig for feedback device failed");
            goto exit;
        }

        /* Retrieve Hostless PCM device id */
        sAttr.type = PAL_STREAM_HAPTICS;
        sAttr.direction = PAL_AUDIO_INPUT_OUTPUT;
        dir = TX_HOSTLESS;
        pcmDevIdTx = rm->allocateFrontEndIds(sAttr, dir);
        if (pcmDevIdTx.size() == 0) {
            PAL_ERR(LOG_TAG, "allocateFrontEndIds failed");
            ret = -ENOSYS;
            goto exit;
        }

        connectCtrlName << "PCM" << pcmDevIdTx.at(0) << " connect";
        connectCtrl = mixer_get_ctl_by_name(virtMixer, connectCtrlName.str().data());
        if (!connectCtrl) {
            PAL_ERR(LOG_TAG, "invalid mixer control: %s", connectCtrlName.str().data());
            goto free_fe;
        }

        ret = mixer_ctl_set_enum_by_string(connectCtrl, backEndName.c_str());
        if (ret) {
            PAL_ERR(LOG_TAG, "Mixer control %s set with %s failed: %d",
            connectCtrlName.str().data(), backEndName.c_str(), ret);
            goto free_fe;
        }

        isTxFeandBeConnected = true;

        config.rate = vi_device.samplerate;
        config.format = SessionAlsaUtils::palToAlsaFormat(device.config.aud_fmt_id);
        config.channels = ch_info.channels;
        config.period_size = DEFAULT_PERIOD_SIZE;
        config.period_count = DEFAULT_PERIOD_COUNT;
        config.start_threshold = 0;
        config.stop_threshold = INT_MAX;
        config.silence_threshold = 0;

        flags = PCM_IN;

        ret = SessionAlsaUtils::getModuleInstanceId(virtMixer, pcmDevIdTx.at(0),
                        backEndName.c_str(), MODULE_HAPTICS_VI, &miid);
        if (0 != ret) {
            PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", MODULE_HAPTICS_VI, ret);
            goto free_fe;
        }

        // Setting the mode of VI module
        modeConfg.th_operation_mode = NORMAL_MODE;
        builder->payloadHapticsDevPConfig(&payload, &payloadSize, miid,
                                 PARAM_ID_HAPTICS_VI_OP_MODE_PARAM,(void *)&modeConfg);
        if (payloadSize) {
            ret = updateCustomPayload(payload, payloadSize);
            free(payload);
            if (0 != ret) {
                PAL_ERR(LOG_TAG," updateCustomPayload Failed for VI_OP_MODE_CFG\n");
                // Not fatal as by default VI module runs in Normal mode
                ret = 0;
            }
        }

        // Setting Channel Map configuration for VI module
        // TODO: Move this to ACDB file
        HapticsviChannelMapConfg.num_ch = numberOfChannels * 2;
        payloadSize = 0;

        builder->payloadHapticsDevPConfig(&payload, &payloadSize, miid,
           PARAM_ID_HAPTICS_VI_CHANNEL_MAP_CFG,(void *)&HapticsviChannelMapConfg);
        if (payloadSize) {
            ret = updateCustomPayload(payload, payloadSize);
            free(payload);
            if (0 != ret) {
                PAL_ERR(LOG_TAG," updateCustomPayload Failed for CHANNEL_MAP_CFG\n");
            }
        }

        //Set persistent params.
        getAndsetPersistentParameter(false);

        // Setting the values for VI module
        if (customPayloadSize) {
            ret = SessionAlsaUtils::setDeviceCustomPayload(rm, backEndName,
                            customPayload, customPayloadSize);
            if (ret) {
                PAL_ERR(LOG_TAG, "Unable to set custom param for mode");
                goto free_fe;
            }
        }

        txPcm = pcm_open(rm->getVirtualSndCard(), pcmDevIdTx.at(0), flags, &config);
        if (!txPcm) {
            PAL_ERR(LOG_TAG, "txPcm open failed");
            goto free_fe;
        }

        if (!pcm_is_ready(txPcm)) {
            PAL_ERR(LOG_TAG, "txPcm open not ready");
            goto err_pcm_open;
        }

        rm->getBackendName(mDeviceAttr.id, backEndNameRx);
        if (!strlen(backEndNameRx.c_str())) {
            PAL_ERR(LOG_TAG, "Failed to obtain rx backend name for %d", mDeviceAttr.id);
            goto err_pcm_open;
        }

        dev = Device::getInstance(&mDeviceAttr, rm);

        ret = rm->getActiveStream_l(activeStreams, dev);
        if ((0 != ret) || (activeStreams.size() == 0)) {
            PAL_ERR(LOG_TAG, " no active stream available");
            ret = -EINVAL;
            goto err_pcm_open;
        }

        stream = static_cast<Stream *>(activeStreams[0]);
        stream->getAssociatedSession(&session);

        ret = session->getMIID(backEndNameRx.c_str(), MODULE_HAPTICS_GEN, &miid);
        if (ret) {
            PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", MODULE_HAPTICS_GEN, ret);
            goto err_pcm_open;
        }

        // Set the operation mode for Haptics module

        PAL_INFO(LOG_TAG, "Normal mode being used");
        hpModeConfg.operation_mode = NORMAL_MODE;

        payloadSize = 0;
        builder->payloadHapticsDevPConfig(&payload, &payloadSize, miid,
                PARAM_ID_HAPTICS_OP_MODE,(void *)&hpModeConfg);
        if (payloadSize) {
            if (customPayload) {
                free (customPayload);
                customPayloadSize = 0;
                customPayload = NULL;
            }
            ret = updateCustomPayload(payload, payloadSize);
            free(payload);
            if (0 != ret) {
                PAL_ERR(LOG_TAG," updateCustomPayload Failed\n");
            }
        }

        enableDevice(audioRoute, mSndDeviceName_vi);
        PAL_DBG(LOG_TAG, "pcm start for TX");
        if (pcm_start(txPcm) < 0) {
            PAL_ERR(LOG_TAG, "pcm start failed for TX path");
            goto err_pcm_open;
        }

        // Free up the local variables
        goto exit;
    }
    else {
        numberOfRequest--;
        if (numberOfRequest > 0) {
            // R0T0 already set, we don't need to process the request.
            goto exit;
        }
        HapticsDevProtSetDevStatus(flag);
        //  HapticsDevice not in use anymore. Stop the processing mode
        PAL_DBG(LOG_TAG, "Closing VI path");
        if (txPcm) {
            rm = ResourceManager::getInstance();
            device.id = PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK;

            ret = rm->getAudioRoute(&audioRoute);
            if (0 != ret) {
                PAL_ERR(LOG_TAG, "Failed to get the audio_route address status %d", ret);
                goto exit;
            }

            ret = rm->getSndDeviceName(device.id , mSndDeviceName_vi);
            rm->getBackendName(device.id, backEndName);
            if (!strlen(backEndName.c_str())) {
                PAL_ERR(LOG_TAG, "Failed to obtain tx backend name for %d", device.id);
                goto exit;
            }
            //Get persistent param for future use.
            getAndsetPersistentParameter(true);

            pcm_stop(txPcm);
            pcm_close(txPcm);
            disableDevice(audioRoute, mSndDeviceName_vi);
            txPcm = NULL;
            sAttr.type = PAL_STREAM_HAPTICS;
            sAttr.direction = PAL_AUDIO_INPUT_OUTPUT;
            goto free_fe;
        }
    }

err_pcm_open :
    if (txPcm) {
        pcm_close(txPcm);
        disableDevice(audioRoute, mSndDeviceName_vi);
        txPcm = NULL;
    }

free_fe:
    if (pcmDevIdTx.size() != 0) {
        if (isTxFeandBeConnected) {
            disconnectFeandBe(pcmDevIdTx, backEndName);
        }
        rm->freeFrontEndIds(pcmDevIdTx, sAttr, dir);
        pcmDevIdTx.clear();
    }
exit:
    deviceMutex.unlock();
    if(builder) {
       delete builder;
       builder = NULL;
    }
    return ret;
}

void HapticsDevProtection::updateHPcustomPayload()
{
    PayloadBuilder* builder = new PayloadBuilder();
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    std::string backEndName;
    std::shared_ptr<Device> dev = nullptr;
    Stream *stream = NULL;
    Session *session = NULL;
    std::vector<Stream*> activeStreams;
    uint32_t miid = 0, ret;
    param_id_haptics_op_mode hpModeConfg;

    rm->getBackendName(mDeviceAttr.id, backEndName);
    dev = Device::getInstance(&mDeviceAttr, rm);
    ret = rm->getActiveStream_l(activeStreams, dev);
    if ((0 != ret) || (activeStreams.size() == 0)) {
        PAL_ERR(LOG_TAG, " no active stream available");
        goto exit;
    }
    stream = static_cast<Stream *>(activeStreams[0]);
    stream->getAssociatedSession(&session);
    ret = session->getMIID(backEndName.c_str(), MODULE_HAPTICS_GEN, &miid);
    if (ret) {
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", MODULE_HAPTICS_GEN, ret);
        goto exit;
    }

    if (customPayloadSize) {
        free(customPayload);
        customPayloadSize = 0;
    }

    hpModeConfg.operation_mode = NORMAL_MODE;
    payloadSize = 0;
    builder->payloadHapticsDevPConfig(&payload, &payloadSize, miid,
                    PARAM_ID_HAPTICS_OP_MODE,(void *)&hpModeConfg);
    if (payloadSize) {
        ret = updateCustomPayload(payload, payloadSize);
        free(payload);
        if (0 != ret) {
            PAL_ERR(LOG_TAG," updateCustomPayload Failed\n");
        }
    }

exit:
    if(builder) {
       delete builder;
       builder = NULL;
    }
    return;
}

int32_t HapticsDevProtection::getFTMParameter(void **param)
{
    int ret = 0;
    int32_t payload_size = 0;
    std::ifstream inFile;
    std::ostringstream resString;
    haptics_vi_cal_param FtmCalParam;

    inFile.open(PAL_HAP_DEVP_FTM_PATH, std::ios::binary | std::ios::in);
    if (!inFile.is_open()) {
        PAL_ERR(LOG_TAG, "Unable to open file for read");
        ret = -EINVAL;
        goto exit;
    }
    inFile.read(reinterpret_cast<char*>(&FtmCalParam), sizeof(haptics_vi_cal_param));
    inFile.close();

    resString << "HapticsParamStatus: " <<  "; Re: "
              << ((FtmCalParam.Re_ohm_Cal_q24)/(1<<24)) << "; Fres: "
              << ((FtmCalParam.Fres_Hz_Cal_q20)/(1<<20)) << "; Bl: "
              << ((FtmCalParam.Bl_q24)/(1<<24)) << "; Rms: "
              << ((FtmCalParam.Rms_KgSec_q24)/(1<<24)) << "; Blq: "
              << ((FtmCalParam.Blq_ftm_q24)/(1<<24)) << "; Le: "
              << ((FtmCalParam.Le_mH_ftm_q24)/(1<<24));

    PAL_DBG(LOG_TAG, "Get param value %s, length:%d",
            resString.str().c_str(), resString.str().length());
    if (resString.str().length() > 0 && ((*param) != nullptr)) {
        payload_size = resString.str().length() + 1;
        strlcpy(static_cast<char*>(*param), resString.str().c_str(),
                payload_size);
        ret = payload_size;
    }
exit:
    return ret;
}

int HapticsDevProtection::HapticsDevProtectionFTM()
{
    int ret = 0;

    PAL_DBG(LOG_TAG, "Enter");

    if (ftmThrdCreated.load()) {
        PAL_DBG(LOG_TAG, "FTM thread already running, Thread State %d, exiting",
                        ftmThrdCreated.load());
        return ret;
    }

    calibrationCallbackStatus = HAPTICS_VI_CALIB_STATE_INACTIVE;
    mDspCallbackRcvd = false;
    ftmThrdCreated.store(true);

    PAL_DBG(LOG_TAG, "creating Haptics FTM thread");
    std::thread dynamicFTMThread(&HapticsDevProtection::HapticsDevFTMThread, this);

    dynamicFTMThread.detach();

    PAL_DBG(LOG_TAG, "Exit");

    return ret;
}

int HapticsDevProtection::HapticsDevProtectionDynamicCal()
{
    int ret = 0;

    PAL_DBG(LOG_TAG, "Enter");

    if (calThrdCreated.load()) {
        PAL_DBG(LOG_TAG, "Calibration already triggered Thread State %d",
                        calThrdCreated.load());
        return ret;
    }

    threadExit = false;
    hapticsDevCalState = HAPTICS_DEV_NOT_CALIBRATED;

    calibrationCallbackStatus = HAPTICS_VI_CALIB_STATE_INACTIVE;
    mDspCallbackRcvd = false;

    calThrdCreated.store(true);
    isDynamicCalTriggered = true;

    std::thread dynamicCalThread(&HapticsDevProtection::HapticsDevCalibrationThread, this);

    dynamicCalThread.detach();

    PAL_DBG(LOG_TAG, "Exit");

    return ret;
}

int HapticsDevProtection::start()
{
    PAL_DBG(LOG_TAG, "Enter");

    if (ResourceManager::isVIRecordStarted) {
        PAL_DBG(LOG_TAG, "record running so just update SP payload");
        updateHPcustomPayload();
    }
    else {
        HapticsDevProtProcessingMode(true);
    }

    PAL_DBG(LOG_TAG, "Calling Device start");
    Device::start();
    return 0;
}

int HapticsDevProtection::stop()
{
    PAL_DBG(LOG_TAG, "Inside  HapticsDevice Protection stop");
    Device::stop();
    if (ResourceManager::isVIRecordStarted) {
        PAL_DBG(LOG_TAG, "record running so no need to proceed");
        ResourceManager::isVIRecordStarted = false;
        return 0;
    }
    HapticsDevProtProcessingMode(false);
    return 0;
}


int32_t HapticsDevProtection::setParameter(uint32_t param_id, void *param)
{
    PAL_DBG(LOG_TAG, "Inside  HapticsDevice Protection Set parameters");
    (void ) param;

    if (param_id == PAL_HAP_MODE_FACTORY_TEST)
        HapticsDevProtectionFTM();

    else if (param_id == PAL_HAP_MODE_DYNAMIC_CAL)
        HapticsDevProtectionDynamicCal();

    else if (param_id ==  PARAM_ID_HAPTICS_EX_VI_PERSISTENT)
        getAndsetPersistentParameter(true);

    return 0;
}

int32_t HapticsDevProtection::getParameter(uint32_t param_id, void **param)
{
    int32_t status = 0;
    switch(param_id) {
        case PAL_PARAM_ID_HAPTICS_MODE:
            status = getFTMParameter(param);
        break;
        default :
            PAL_ERR(LOG_TAG, "Unsupported operation");
            status = -EINVAL;
        break;
    }
    return status;
}

int32_t HapticsDevProtection::getAndsetPersistentParameter(bool flag)
{
    int size = 0, status = 0, ret = 0;
    uint32_t miid = 0;
    const char *getParamControl = "getParam";
    char *pcmDeviceName = NULL;
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    struct mixer_ctl *ctl;
    FILE *fp;
    std::ostringstream cntrlName;
    std::ostringstream resString;
    std::string backendName;
    param_id_haptics_ex_vi_persistent ViPe;
    PayloadBuilder* builder = new PayloadBuilder();
    param_id_haptics_ex_vi_persistent *VIpeValue;

    pcmDeviceName = rm->getDeviceNameFromID(pcmDevIdTx.at(0));
    if (pcmDeviceName) {
        cntrlName << pcmDeviceName << " " << getParamControl;
    } else {
        PAL_ERR(LOG_TAG, "Error: %d Unable to get Device name\n", -EINVAL);
        goto exit;
    }

    ctl = mixer_get_ctl_by_name(virtMixer, cntrlName.str().data());
    if (!ctl) {
        status = -ENOENT;
        PAL_ERR(LOG_TAG, "Error: %d Invalid mixer control: %s\n", status,cntrlName.str().data());
        goto exit;
    }
    rm->getBackendName(PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK, backendName);
    if (!strlen(backendName.c_str())) {
        status = -ENOENT;
        PAL_ERR(LOG_TAG, "Error: %d Failed to obtain VI backend name", status);
        goto exit;
    }

    status = SessionAlsaUtils::getModuleInstanceId(virtMixer, pcmDevIdTx.at(0),
                        backendName.c_str(), MODULE_HAPTICS_VI, &miid);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error: %d Failed to get tag info %x", status, MODULE_HAPTICS_VI);
        goto exit;
    }
    if (flag) {
        builder->payloadHapticsDevPConfig (&payload, &payloadSize, miid,
                              PARAM_ID_HAPTICS_EX_VI_PERSISTENT, &ViPe);

        status = mixer_ctl_set_array(ctl, payload, payloadSize);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Set failed status = %d", status);
            goto exit;
        }

        memset(payload, 0, payloadSize);

        status = mixer_ctl_get_array(ctl, payload, payloadSize);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Get failed status = %d", status);
        }
        else {
            VIpeValue = (param_id_haptics_ex_vi_persistent *) (payload +
                            sizeof(struct apm_module_param_data_t));
            fp = fopen(PAL_HP_VI_PER_PATH, "wb");
            if (!fp) {
                PAL_ERR(LOG_TAG, "Unable to open file for write");
            } else {
                PAL_DBG(LOG_TAG, "Write the Vi persistant value to file");
                for (int i = 0; i < numberOfChannels; i++) {
                    PAL_ERR(LOG_TAG, "persistent values VIpeValue->Re_ohm_q24[i] =%d",VIpeValue->Re_ohm_q24[i]);
                    fwrite(&VIpeValue->Re_ohm_q24[i], sizeof(VIpeValue->Re_ohm_q24[i]),
                                             1, fp);
                    PAL_ERR(LOG_TAG, "persistent values VIpeValue->Le_mH_q24[i] =%d",VIpeValue->Le_mH_q24[i]);
                    fwrite(&VIpeValue->Le_mH_q24[i], sizeof(VIpeValue->Le_mH_q24[i]),
                                             1, fp);
                    PAL_ERR(LOG_TAG, "persistent values VIpeValue->Bl_q24[i] =%d",VIpeValue->Bl_q24[i]);
                    fwrite(&VIpeValue->Bl_q24[i], sizeof(VIpeValue->Bl_q24[i]),
                                             1, fp);
                    PAL_ERR(LOG_TAG, "persistent values VIpeValue->Rms_KgSec_q24[i] =%d",VIpeValue->Rms_KgSec_q24[i]);
                    fwrite(&VIpeValue->Rms_KgSec_q24[i], sizeof(VIpeValue->Rms_KgSec_q24[i]),
                                             1, fp);
                    PAL_ERR(LOG_TAG, "persistent values VIpeValue->Kms_Nmm_q24[i] =%d",VIpeValue->Kms_Nmm_q24[i]);
                    fwrite(&VIpeValue->Kms_Nmm_q24[i], sizeof(VIpeValue->Kms_Nmm_q24[i]),
                                             1, fp);
                    PAL_ERR(LOG_TAG, "persistent values VIpeValue->Fres_Hz_q20[i] =%d",VIpeValue->Fres_Hz_q20[i]);
                    fwrite(&VIpeValue->Fres_Hz_q20[i], sizeof(VIpeValue->Fres_Hz_q20[i]),
                                             1, fp);
                }
                fclose(fp);
            }
        }
    }
    else {
        fp = fopen(PAL_HP_VI_PER_PATH, "rb");
        if (fp) {
            VIpeValue = (param_id_haptics_ex_vi_persistent *)calloc(1,
                               sizeof(param_id_haptics_ex_vi_persistent));
            if (!VIpeValue) {
                PAL_ERR(LOG_TAG," payload creation Failed\n");
                return 0;
            }
            PAL_DBG(LOG_TAG, "update Vi persistant value from file");
            for (int i = 0; i < numberOfChannels; i++) {
                    fread(&VIpeValue->Re_ohm_q24[i], sizeof(VIpeValue->Re_ohm_q24[i]),
                                             1, fp);
                     PAL_ERR(LOG_TAG, "persistent values VIpeValue->Re_ohm_q24[i] =%d",VIpeValue->Re_ohm_q24[i]);
                    fread(&VIpeValue->Le_mH_q24[i], sizeof(VIpeValue->Le_mH_q24[i]),
                                             1, fp);
                     PAL_ERR(LOG_TAG, "persistent values VIpeValue->Le_mH_q24[i] =%d",VIpeValue->Le_mH_q24[i]);
                    fread(&VIpeValue->Bl_q24[i], sizeof(VIpeValue->Bl_q24[i]),
                                             1, fp);
                     PAL_ERR(LOG_TAG, "persistent values VIpeValue->Bl_q24[i] =%d",VIpeValue->Bl_q24[i]);
                    fread(&VIpeValue->Rms_KgSec_q24[i], sizeof(VIpeValue->Rms_KgSec_q24[i]),
                                             1, fp);
                    PAL_ERR(LOG_TAG, "persistent values VIpeValue->Rms_KgSec_q24[i] =%d",VIpeValue->Rms_KgSec_q24[i]);
                    fread(&VIpeValue->Kms_Nmm_q24[i], sizeof(VIpeValue->Kms_Nmm_q24[i]),
                                             1, fp);
                     PAL_ERR(LOG_TAG, "persistent values VIpeValue->Kms_Nmm_q24[i] =%d",VIpeValue->Kms_Nmm_q24[i]);
                    fread(&VIpeValue->Fres_Hz_q20[i], sizeof(VIpeValue->Fres_Hz_q20[i]),
                                             1, fp);
                     PAL_ERR(LOG_TAG, "persistent values VIpeValue->Fres_Hz_q20[i] =%d",VIpeValue->Fres_Hz_q20[i]);
            }
            fclose(fp);
        }
        else {
            PAL_DBG(LOG_TAG, "Use the default Persistent values");
            return 0;
        }
        payloadSize = 0;
        builder->payloadHapticsDevPConfig(&payload, &payloadSize, miid,
                   PARAM_ID_HAPTICS_EX_VI_PERSISTENT,(void *)VIpeValue);
        if (payloadSize) {
            ret = updateCustomPayload(payload, payloadSize);
            free(payload);
            free(VIpeValue);
            if (0 != ret) {
                PAL_ERR(LOG_TAG," updateCustomPayload Failed\n");
                ret = 0;
            }
        }
    }
exit :
    if(builder) {
       delete builder;
       builder = NULL;
    }
    return ret;
}

/*
 * VI feedack related functionalities
 */

void HapticsDevFeedback::updateVIcustomPayload()
{
    PayloadBuilder* builder = new PayloadBuilder();
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    std::string backEndName;
    std::shared_ptr<Device> dev = nullptr;
    Stream *stream = NULL;
    Session *session = NULL;
    std::vector<Stream*> activeStreams;
    uint32_t miid = 0, ret = 0;
    struct param_id_haptics_th_vi_r0t0_set_param_t r0t0Value;
    FILE *fp = NULL;
    param_id_haptics_th_vi_r0t0_set_param_t *hpR0T0confg;
    param_id_haptics_vi_op_mode_param_t modeConfg;
    param_id_haptics_vi_channel_map_cfg_t HapticsviChannelMapConfg;
    param_id_haptics_ex_vi_ftm_set_cfg HapticsviExModeConfg;

    rm->getBackendName(mDeviceAttr.id, backEndName);
    dev = Device::getInstance(&mDeviceAttr, rm);
    ret = rm->getActiveStream_l(activeStreams, dev);
    if ((0 != ret) || (activeStreams.size() == 0)) {
        PAL_ERR(LOG_TAG, " no active stream available");
        goto exit;
    }
    stream = static_cast<Stream *>(activeStreams[0]);
    stream->getAssociatedSession(&session);
    ret = session->getMIID(backEndName.c_str(), MODULE_HAPTICS_VI, &miid);
    if (ret) {
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", MODULE_HAPTICS_VI, ret);
        goto exit;
    }

    if (customPayloadSize) {
        free(customPayload);
        customPayloadSize = 0;
    }

    memset(&modeConfg, 0, sizeof(modeConfg));
    memset(&HapticsviChannelMapConfg, 0, sizeof(HapticsviChannelMapConfg));
    memset(&HapticsviExModeConfg, 0, sizeof(HapticsviExModeConfg));
    memset(&r0t0Value, 0, sizeof(struct param_id_haptics_th_vi_r0t0_set_param_t));

    // Setting the mode of VI module
    modeConfg.th_operation_mode = NORMAL_MODE;
    builder->payloadHapticsDevPConfig(&payload, &payloadSize, miid,
                             PARAM_ID_HAPTICS_VI_OP_MODE_PARAM,(void *)&modeConfg);
    if (payloadSize) {
        ret = updateCustomPayload(payload, payloadSize);
        free(payload);
        if (0 != ret) {
            PAL_ERR(LOG_TAG," updateCustomPayload Failed for VI_OP_MODE_CFG\n");
        }
    }

    // Setting Channel Map configuration for VI module
    HapticsviChannelMapConfg.num_ch = 1;
    payloadSize = 0;

    builder->payloadHapticsDevPConfig(&payload, &payloadSize, miid,
                    PARAM_ID_HAPTICS_VI_CHANNEL_MAP_CFG,(void *)&HapticsviChannelMapConfg);
    if (payloadSize) {
        ret = updateCustomPayload(payload, payloadSize);
        free(payload);
        if (0 != ret) {
            PAL_ERR(LOG_TAG," updateCustomPayload Failed for CHANNEL_MAP_CFG\n");
        }
    }

    hpR0T0confg = (param_id_haptics_th_vi_r0t0_set_param_t *)calloc(1,
                         sizeof(param_id_haptics_th_vi_r0t0_set_param_t));
    if (!hpR0T0confg) {
        PAL_ERR(LOG_TAG," updateCustomPayload Failed\n");
        return;
    }
    hpR0T0confg->num_channels = numDevice;
    fp = fopen(PAL_HAP_DEVP_CAL_PATH, "rb");
    if (fp) {
        PAL_DBG(LOG_TAG, " HapticsDevice calibrated. Send calibrated value");
        for (int i = 0; i < numDevice; i++) {
            fread(&r0t0Value.r0_cali_q24[i],
                    sizeof(r0t0Value.r0_cali_q24[i]), 1, fp);
            fread(&r0t0Value.t0_cali_q6[i],
                    sizeof(r0t0Value.t0_cali_q6[i]), 1, fp);
        }
    }
    else {
        PAL_DBG(LOG_TAG, " HapticsDevice not calibrated. Send safe value");
        for (int i = 0; i < numDevice; i++) {
            r0t0Value.r0_cali_q24[i] = MIN_RESISTANCE_HAPTICS_DEV_Q24;
            r0t0Value.t0_cali_q6[i] = SAFE_HAPTICS_DEV_TEMP_Q6;
        }
    }

    payloadSize = 0;
    builder->payloadHapticsDevPConfig(&payload, &payloadSize, miid,
                    PARAM_ID_HAPTICS_TH_VI_R0T0_SET_PARAM,(void *)hpR0T0confg);
    if (payloadSize) {
        ret = updateCustomPayload(payload, payloadSize);
        free(payload);
        free(hpR0T0confg);
        if (0 != ret) {
            PAL_ERR(LOG_TAG," updateCustomPayload Failed\n");
        }
    }
exit:
    if(builder) {
       delete builder;
       builder = NULL;
    }
    return;
}

HapticsDevFeedback::HapticsDevFeedback(struct pal_device *device,
                                std::shared_ptr<ResourceManager> Rm):HapticsDev(device, Rm)
{
    struct pal_device_info devinfo = {};

    memset(&mDeviceAttr, 0, sizeof(struct pal_device));
    memcpy(&mDeviceAttr, device, sizeof(struct pal_device));
    rm = Rm;


    rm->getDeviceInfo(mDeviceAttr.id, PAL_STREAM_PROXY, mDeviceAttr.custom_config.custom_key, &devinfo);
    numDevice = devinfo.channels;
}

HapticsDevFeedback::~HapticsDevFeedback()
{
}

int32_t HapticsDevFeedback::start()
{
    ResourceManager::isVIRecordStarted = true;
    // Do the customPayload configuration for VI path and call the Device::start
    PAL_DBG(LOG_TAG," Feedback start\n");
    updateVIcustomPayload();
    Device::start();

    return 0;
}

int32_t HapticsDevFeedback::stop()
{
    ResourceManager::isVIRecordStarted = false;
    PAL_DBG(LOG_TAG," Feedback stop\n");
    Device::stop();

    return 0;
}

std::shared_ptr<Device> HapticsDevFeedback::getInstance(struct pal_device *device,
                                                     std::shared_ptr<ResourceManager> Rm)
{
    PAL_DBG(LOG_TAG," Feedback getInstance\n");
    if (!obj) {
        std::shared_ptr<Device> sp(new HapticsDevFeedback(device, Rm));
        obj = sp;
    }
    return obj;
}
