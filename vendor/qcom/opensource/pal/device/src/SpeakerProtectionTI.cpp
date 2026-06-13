/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
#define LOG_NDEBUG 0
#define LOG_TAG "PAL: SpeakerProtectionTI"

#include "SpeakerProtectionTI.h"
#include "PalAudioRoute.h"
#include "ResourceManager.h"
#include "SessionAlsaUtils.h"
#include "kvh2xml.h"
#include <agm/agm_api.h>

#include<fstream>
#include<sstream>

#include "TISpeakerProtDefs.h"

#ifdef SEC_AUDIO_MUTE_DETECTION
#include <audioInfo.h>
#endif

#ifdef ENABLE_TAS_SPK_PROT
#define VI_FEEDBACK_MONO_1 "-mono-1"
#define DEFAULT_PERIOD_SIZE 256
#define DEFAULT_PERIOD_COUNT 4

#define NORMAL_MODE 0
#define CALIBRATION_MODE 1
#define FACTORY_TEST_MODE 2
#define V_VALIDATION_MODE 3

#define CALIBRATION_STATUS_SUCCESS 1
#define CALIBRATION_STATUS_FAILURE 0

#define NUM_MAX_CALIB_RETRY 4
#define MAX_SP_SPKRS 2
#define CALIB_DURATION_SECS 2
#define CALIB_RETRY_DURATION_MS 200

#define EVENT_ID_TISA_DSP_EVNT 0x7154EEEE
#define TAS25XX_EFS_CALIB_RE_L      "/efs/tas25xx/calib_re"
#define TAS25XX_EFS_CALIB_TEMP_L    "/efs/tas25xx/amb_temp"
#define TAS25XX_EFS_CALIB_RE_R      "/efs/tas25xx/calib_re_r"
#define TAS25XX_EFS_CALIB_TEMP_R    "/efs/tas25xx/amb_temp_r"
#define TAS25XX_EFS_CALIB_STATUS    "/efs/tas25xx/calibration"

#ifndef PAL_PARAM_ID_SEPAKER_AMP_RUN_CAL
#define PAL_PARAM_ID_SEPAKER_AMP_RUN_CAL 3000
#endif

#define TAS25xx_AMB_TEMP "/sys/class/tas25xx_dev/cmd/temp"
#define TAS25xx_DEV_CMD_CALIB "/sys/class/tas25xx_dev/cmd/calib"
#define TAS25XX_EFS_DRV_OP_MODE "/sys/class/tas25xx_dev/cmd/drv_opmode"
#define TAS25XX_EFS_IV_VBAT "/sys/class/tas25xx_dev/cmd/iv_vbat"

#define MAX_STRING 20

#define GET_MIXER_STR "getParam"
#define SET_MIXER_STR "setParam"

#define MIN_AMB_TEMP 0
#define MAX_AMB_TEMP 40

#define MAX(a,b) (((a)>(b))?(a):(b))

#define DISABLE_TX_IN_HANDSET
#define SILENCE_COUNT_ON_PAL

bool SpeakerProtectionTI::isSpkrInUse;
static bool mCalRunning = false;
struct timespec SpeakerProtectionTI::spkrLastTimeUsed;
struct mixer *SpeakerProtectionTI::virtMixer;
struct mixer *SpeakerProtectionTI::hwMixer;
struct pcm * SpeakerProtectionTI::rxPcm = NULL;
struct pcm * SpeakerProtectionTI::txPcm = NULL;
int SpeakerProtectionTI::numberOfChannels;
struct pal_device_info SpeakerProtectionTI::vi_device;
int SpeakerProtectionTI::numberOfRequest;
pal_param_amp_bigdata_t SpeakerProtectionTI::bigdata;
pal_param_amp_bigdata_t SpeakerProtectionTI::bigdataRef;
pal_param_amp_ssrm_t SpeakerProtectionTI::ssrm;
pal_param_amp_silence_stats SpeakerProtectionTI::silencestat[NUM_OF_SPEAKER];
std::string SpeakerProtectionTI::mLibVersion;
std::string SpeakerProtectionTI::mParamVersion;
#if defined(SEC_AUDIO_DUAL_SPEAKER) || defined(SEC_AUDIO_SCREEN_MIRRORING) // { SUPPORT_VOIP_VIA_SMART_VIEW
pal_device_speaker_mute SpeakerProtectionTI::curMuteStatus;
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW

int SpeakerProtectionTI::get_calib_data(spCalibData *ptical) {
    int ret = 0;
    int32_t ch_idx = 0;
    float re_f;
    int32_t amb_temp;
    int32_t oneq19 = (1 << 19);

    char *pcalibRefiles[] = {TAS25XX_EFS_CALIB_RE_L, TAS25XX_EFS_CALIB_RE_R};
    char *pcalibTempfiles[] = {TAS25XX_EFS_CALIB_TEMP_L, TAS25XX_EFS_CALIB_TEMP_R};

    int32_t calib_fail_value = -1;

    for (ch_idx = 0; ch_idx < ptical->num_ch; ch_idx++) {
        ret = get_data_from_file(pcalibRefiles[ch_idx], FLOAT, (void*)&re_f);
        if(ret) {
            return ret;
        }

        ret = get_data_from_file(pcalibTempfiles[ch_idx], INT, (void*)&amb_temp);
        if (ret || (amb_temp == calib_fail_value)) {
            return -1;
        }

        ptical->t0[ch_idx] = amb_temp;
        ptical->r0_q19[ch_idx] = (int32_t)(re_f * oneq19);
    }

    return 0;
}

int SpeakerProtectionTI::get_data_from_file(const char* fname, tisa_calib_data_type_t dtype, void* pdata)
{
    FILE* fp;
    char data_from_file[MAX_STRING] = { 0 };
    int ret = 0;

    fp = fopen(fname, "r");
    if (!fp) {
        PAL_ERR(LOG_TAG, "file %s open failed", fname);
        return -1;
    }

    fscanf(fp, "%s ", data_from_file);

    switch (dtype) {
    case FLOAT:
        {
            float *pdata_fp = (float*)pdata;
            sscanf(data_from_file, "%f", pdata_fp);
            PAL_INFO(LOG_TAG, "data from file %s: %s(string) %05.2lf(float)\n",
                    fname, data_from_file, *pdata_fp);

        }
    break;
    case INT:
        {
            int *pdata_int = (int*)pdata;
            sscanf(data_from_file, "%d", pdata_int);
            PAL_INFO(LOG_TAG,"data from file %s: %s(string) %d(int)\n",
                    fname, data_from_file, *pdata_int);

        }
    break;
    case HEX:
        {
            int *pdata_int = (int*)pdata;
            sscanf(data_from_file, "0x%x", pdata_int);
            PAL_INFO(LOG_TAG,"data from file %s: %s(string) 0x%x(hex)\n",
                    fname, data_from_file, *pdata_int);
        }
    break;
    default:
        {
            PAL_ERR(LOG_TAG,"invalid file access for %s: %d\n", fname, dtype);
            ret = -1;
        }

    }

    fclose(fp);

    return ret;
}

int SpeakerProtectionTI::save_data_to_file(const char* fname, tisa_calib_data_type_t dtype, void *pdata)
{
    FILE* fp;

    fp = fopen(fname, "w");
    if (!fp) {
        PAL_ERR(LOG_TAG, "file %s open failed", fname);
        return -1;
    }

    switch (dtype) {
    case FLOAT:
    {
        float fdata;
        fdata = *((float*)pdata);
        fprintf(fp, "%05.2lf", fdata);
    }
    break;
    case INT:
    {
        int data = *((int*)pdata);
        fprintf(fp, "%d", data);
    }
    break;
    case STRING:
    {
        char *strdata = (char*)pdata;
        fprintf(fp, "%s", strdata);

    }
    break;
    case HEX:
    default:
    break;
    }

    fclose(fp);
    return 0;
}

/* Function to set status of speaker */
void SpeakerProtectionTI::spkrProtSetSpkrStatus(bool enable)
{
    PAL_DBG(LOG_TAG, "Enter");

    if (enable)
        isSpkrInUse = true;
    else {
        isSpkrInUse = false;
        clock_gettime(CLOCK_BOOTTIME, &spkrLastTimeUsed);
        PAL_INFO(LOG_TAG, "Speaker used last time %ld", spkrLastTimeUsed.tv_sec);
    }

    PAL_DBG(LOG_TAG, "Exit");
}

void SpeakerProtectionTI::disconnectFeandBe(std::vector<int> pcmDevIds,
                                         std::string backEndName) {

    std::ostringstream disconnectCtrlName;
    std::ostringstream disconnectCtrlNameBe;
    struct mixer_ctl *disconnectCtrl = NULL;
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

    disconnectCtrlNameBe<< backEndName << " metadata";
    beMetaDataMixerCtrl = mixer_get_ctl_by_name(virtMixer, disconnectCtrlNameBe.str().data());
    if (!beMetaDataMixerCtrl) {
        ret = -EINVAL;
        PAL_ERR(LOG_TAG, "Error: %d, invalid mixer control %s", ret, backEndName.c_str());
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

    if (deviceMetaData.size) {
        ret = mixer_ctl_set_array(beMetaDataMixerCtrl, (void *)deviceMetaData.buf,
                    deviceMetaData.size);
    } else {
        PAL_ERR(LOG_TAG, "Error: %d, Device Metadata not cleaned up", ret);
        goto exit;
    }

exit:
    if (deviceMetaData.buf) {
        free(deviceMetaData.buf);
        deviceMetaData.buf = nullptr;
        deviceMetaData.size = 0;
    }
    return;
}

SpeakerProtectionTI::SpeakerProtectionTI(struct pal_device *device,
                        std::shared_ptr<ResourceManager> Rm):Device(device, Rm)
{
    int status = 0;
    struct pal_device_info devinfo = {};
    FILE *fp = NULL;

    rm = Rm;

    memset(&mDeviceAttr, 0, sizeof(struct pal_device));
    memcpy(&mDeviceAttr, device, sizeof(struct pal_device));

    isSpkrInUse = false;
    mCalRunning = false;

    rm->getDeviceInfo(PAL_DEVICE_OUT_SPEAKER, PAL_STREAM_PROXY, "", &devinfo);
    numberOfChannels = devinfo.channels;

    rm->getDeviceInfo(PAL_DEVICE_IN_VI_FEEDBACK, PAL_STREAM_PROXY, "", &vi_device);
    PAL_INFO(LOG_TAG, "Number of Channels, VI path: %d, Rx path: %d", vi_device.channels, numberOfChannels);


    // Get current time
    clock_gettime(CLOCK_BOOTTIME, &spkrLastTimeUsed);

    // Getting mixture controls from Resource Manager
    status = rm->getVirtualAudioMixer(&virtMixer);
    if (status) {
        PAL_ERR(LOG_TAG,"virt mixer error %d", status);
    }
    status = rm->getHwAudioMixer(&hwMixer);
    if (status) {
        PAL_ERR(LOG_TAG,"hw mixer error %d", status);
    }

#if defined(SEC_AUDIO_DUAL_SPEAKER) || defined(SEC_AUDIO_SCREEN_MIRRORING) // { SUPPORT_VOIP_VIA_SMART_VIEW
    curMuteStatus = PAL_DEVICE_SPEAKER_UNMUTE;
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW
}

SpeakerProtectionTI::~SpeakerProtectionTI()
{
    if (customPayload)
        free(customPayload);

    customPayload = NULL;
    customPayloadSize = 0;
}

// Callback from DSP for silence detection
void SpeakerProtectionTI::handleTISADSPEvent(uint64_t hdl /*__unused*/, uint32_t event_id,
                                            void *event_data, uint32_t event_size)
{
    unsigned int *param_data = nullptr;

    PAL_INFO(LOG_TAG, "Received an event from DSP %x", event_id);
#ifdef SEC_AUDIO_MUTE_DETECTION
    android::String8 detectLog = android::String8::format("Received an event from DSP %x", event_id);
    android::AudioInfo::GetInstance()->setSecSaveAudioLog(detectLog);
#endif

    if (event_id == EVENT_ID_TISA_DSP_EVNT) {
        param_data = (unsigned int *)event_data;
#if defined(SEC_AUDIO_DUAL_SPEAKER) || defined(SEC_AUDIO_SCREEN_MIRRORING) // { SUPPORT_VOIP_VIA_SMART_VIEW
        if (curMuteStatus != PAL_DEVICE_SPEAKER_MUTE) { /* in case that speaker is not in 'full mute' state */
            int silence_count_update = 0;

#if defined(SILENCE_COUNT_ON_PAL)
            for (int ch_idx = 0; ch_idx < numberOfChannels; ch_idx++) /* apply counter to both for total mute */
                silencestat[ch_idx].silenceCount++;
            silence_count_update = silencestat[0].silenceCount;
#else
            silence_count_update = *param_data;
#endif /* SILENCE_COUNT_ON_PAL */
            PAL_INFO(LOG_TAG, "Silence Detected with silence count = %d", silence_count_update);
        }
#else
        PAL_INFO(LOG_TAG, "Silence Detected with silence count = %d", *param_data);
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW
    }
}

/*
 * Function to trigger Processing mode.
 * The parameter that it accepts are below:
 * true - Start Processing Mode
 * false - Stop Processing Mode
 */
int32_t SpeakerProtectionTI::spkrProtProcessingMode(bool flag)
{
    int ret = 0, dir = TX_HOSTLESS, flags, viParamId = 0;
    int handset = 0;
    char mSndDeviceName_vi[128] = {0};
    uint8_t* payload = NULL;
    uint32_t devicePropId[] = {0x08000010, 1, 0x2};
    uint32_t miid = 0;
    bool isTxFeandBeConnected = true;
    size_t payloadSize = 0;
    struct pal_device device;
    struct pal_channel_info ch_info;
    struct pal_stream_attributes sAttr;
    struct pcm_config config;
    struct mixer_ctl *connectCtrl = NULL;
    struct audio_route *audioRoute = NULL;
    struct vi_r0t0_cfg_t r0t0Array[numberOfChannels];
    struct agmMetaData deviceMetaData(nullptr, 0);
    struct mixer_ctl *beMetaDataMixerCtrl = nullptr;
    FILE *fp;
    std::string backEndName, backEndNameRx;
    std::vector <std::pair<int, int>> keyVector;
    std::vector <std::pair<int, int>> calVector;
    std::shared_ptr<ResourceManager> rm = nullptr;
    std::ostringstream connectCtrlNameBeVI;
    std::ostringstream connectCtrlNameBeSP;
    std::ostringstream connectCtrlName;
    param_id_sp_vi_op_mode_cfg_t modeConfg;
    param_id_sp_vi_channel_map_cfg_t viChannelMapConfg;
    param_id_sp_op_mode_t spModeConfg;
    std::shared_ptr<Device> dev = nullptr;
    Stream *stream = NULL;
    Session *session = NULL;
    std::vector<Stream*> activeStreams;
    PayloadBuilder* builder = new PayloadBuilder();

    int32_t status = 0;
    static struct agm_event_reg_cfg *event_cfg = NULL;
    session_callback sessionCb = handleTISADSPEvent;
    int32_t payload_size = sizeof(struct agm_event_reg_cfg);

    if (mDeviceAttr.id == PAL_DEVICE_OUT_HANDSET)
        handset = 1;

    PAL_INFO(LOG_TAG, "Flag %d", flag);
    deviceMutex.lock();
    rm = ResourceManager::getInstance();
    if (!rm) {
        PAL_ERR(LOG_TAG, "Failed to get resource manager instance");
        goto exit;
    }

    memset(&sAttr, 0, sizeof(sAttr));
    sAttr.type = PAL_STREAM_LOW_LATENCY;
    sAttr.direction = PAL_AUDIO_INPUT_OUTPUT;

    if (flag) {
        numberOfRequest++;
        if (numberOfRequest > 1) {
            // R0T0 already set, we don't need to process the request
            goto exit;
        }
        PAL_DBG(LOG_TAG, "Custom payload size %zu, Payload %p", customPayloadSize,
                customPayload);

        if (customPayload) {
            free(customPayload);
        }
        customPayloadSize = 0;
        customPayload = NULL;

        spkrProtSetSpkrStatus(flag);

        memset(&device, 0, sizeof(device));
        memset(&config, 0, sizeof(config));
        memset(&modeConfg, 0, sizeof(modeConfg));
        memset(&viChannelMapConfg, 0, sizeof(viChannelMapConfg));
        memset(&spModeConfg, 0, sizeof(spModeConfg));

        keyVector.clear();
        calVector.clear();
#ifndef DEBUG_DIS_TX
#ifdef DISABLE_TX_IN_HANDSET
        if(!handset)
#endif
        {
            // Configure device attribute
            if (vi_device.channels > 1) {
                ch_info.channels = CHANNELS_2;
                ch_info.ch_map[0] = PAL_CHMAP_CHANNEL_FL;
                ch_info.ch_map[1] = PAL_CHMAP_CHANNEL_FR;
            }
            else {
                ch_info.channels = CHANNELS_1;
                ch_info.ch_map[0] = PAL_CHMAP_CHANNEL_FR;
            }

            rm->getChannelMap(&(ch_info.ch_map[0]), vi_device.channels);
            ch_info.channels = vi_device.channels;

            if (mDeviceAttr.id == PAL_DEVICE_OUT_HANDSET)
                    ch_info.ch_map[0] = PAL_CHMAP_CHANNEL_FL;

            switch (vi_device.channels) {
                case 1 :
                    ch_info.channels = CHANNELS_1;
                break;
                case 2 :
                    ch_info.channels = CHANNELS_2;
                break;
                case 4:
                    ch_info.channels = CHANNELS_4;
                break;
                default:
                    PAL_DBG(LOG_TAG, "Unsupported channel. Set default as 2");
                    ch_info.channels = CHANNELS_2;
                break;
            }

            device.config.ch_info = ch_info;
            device.config.sample_rate = vi_device.samplerate;
            device.config.bit_width = vi_device.bit_width;
            device.config.aud_fmt_id = rm->getAudioFmt(vi_device.bit_width);

#ifdef ENABLE_TAS_SPK_PROT
            PAL_DBG(LOG_TAG, "vi_device info - device.config.bit_width %d", device.config.bit_width);
            PAL_DBG(LOG_TAG, "vi_device info - device.config.aud_fmt_id %d", device.config.aud_fmt_id);
#endif

            // Setup TX path
            device.id = PAL_DEVICE_IN_VI_FEEDBACK;

            ret = rm->getAudioRoute(&audioRoute);
            if (0 != ret) {
                PAL_ERR(LOG_TAG, "Failed to get the audio_route address status %d", ret);
                goto exit;
            }

            ret = rm->getSndDeviceName(device.id , mSndDeviceName_vi);
            if (0 != ret) {
                PAL_ERR(LOG_TAG, "Failed to obtain tx snd device name for %d", device.id);
                goto exit;
            }

            PAL_DBG(LOG_TAG, "get the audio route %s", mSndDeviceName_vi);

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
            switch (numberOfChannels) {
                case 1 :
                    if (mDeviceAttr.id == PAL_DEVICE_OUT_HANDSET)
                        calVector.push_back(std::make_pair(SPK_PRO_VI_MAP, LEFT_SPKR));
                    else
                        calVector.push_back(std::make_pair(SPK_PRO_VI_MAP, RIGHT_SPKR));
                break;
                case 2 :
                    calVector.push_back(std::make_pair(SPK_PRO_VI_MAP, STEREO_SPKR));
                break;
                default :
                    PAL_ERR(LOG_TAG, "Unsupported channel");
                    goto exit;
            }

            SessionAlsaUtils::getAgmMetaData(keyVector, calVector,
                    (struct prop_data *)devicePropId, deviceMetaData);
            if (!deviceMetaData.size) {
                PAL_ERR(LOG_TAG, "VI device metadata is zero");
                ret = -ENOMEM;
                goto exit;
            }
            connectCtrlNameBeVI<< backEndName << " metadata";
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
            } else {
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
            switch (vi_device.bit_width) {
                case 32 :
                    config.format = PCM_FORMAT_S32_LE;
                break;
                case 24 :
                    config.format = PCM_FORMAT_S24_LE;
                break;
                case 16 :
                    config.format = PCM_FORMAT_S16_LE;
                break;
                default:
                    PAL_DBG(LOG_TAG, "Unsupported bit width. Set default as 16");
                    config.format = PCM_FORMAT_S16_LE;
                break;
            }

            switch (vi_device.channels) {
                case 1 :
                    config.channels = CHANNELS_1;
                break;
                case 2 :
                    config.channels = CHANNELS_2;
                break;
                case 4:
                    config.channels = CHANNELS_4;
                break;
                default :
                    PAL_DBG(LOG_TAG, "Unsupported channel. Set default as 2");
                    config.channels = CHANNELS_2;
                break;
            }
            config.period_size = DEFAULT_PERIOD_SIZE;
            config.period_count = DEFAULT_PERIOD_COUNT;
            config.start_threshold = 0;
            config.stop_threshold = INT_MAX;
            config.silence_threshold = 0;

            flags = PCM_IN;

            ret = SessionAlsaUtils::getModuleInstanceId(virtMixer, pcmDevIdTx.at(0),
                            backEndName.c_str(), MODULE_VI, &miid);
            if (0 != ret) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", MODULE_VI, ret);
                goto free_fe;
            }

            modeConfg.num_speakers = numberOfChannels;
            modeConfg.th_operation_mode = NORMAL_MODE;
            modeConfg.th_quick_calib_flag = 0;

            builder->payloadSPConfig(&payload, &payloadSize, miid,
                    PARAM_ID_SP_VI_OP_MODE_CFG,(void *)&modeConfg);

            PAL_DBG(LOG_TAG, "payload PARAM_ID_SP_VI_OP_MODE_CFG 0x%x", PARAM_ID_SP_VI_OP_MODE_CFG);

            builder->payloadSPConfig(&payload, &payloadSize, miid,
                    PARAM_ID_SP_VI_OP_MODE_CFG,(void *)&modeConfg);
            if (payloadSize) {
                ret = updateCustomPayload(payload, payloadSize);
                free(payload);
                if (0 != ret) {
                    PAL_ERR(LOG_TAG,"updateCustomPayload Failed for VI_OP_MODE_CFG\n");
                    // Not fatal as by default VI module runs in Normal mode
                    ret = 0;

                    // Setting Channel Map configuration for VI module
                    // TODO: Move this to ACDB file
                    viChannelMapConfg.num_ch = numberOfChannels * 2;
                    payloadSize = 0;

                    builder->payloadSPConfig(&payload, &payloadSize, miid,
                            PARAM_ID_SP_VI_CHANNEL_MAP_CFG,(void *)&viChannelMapConfg);
                    if (payloadSize) {
                        ret = updateCustomPayload(payload, payloadSize);
                        free(payload);
                        if (0 != ret) {
                            PAL_ERR(LOG_TAG," updateCustomPayload Failed for CHANNEL_MAP_CFG\n");
                        }
                    }
                }
            }
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

            // Register for VI module callback
            PAL_DBG(LOG_TAG, "registering EVENT_ID_TISA_DSP_EVNT event for VI module");

            event_cfg = (struct agm_event_reg_cfg *)calloc(1, payload_size);
            if (!event_cfg) {
                PAL_ERR(LOG_TAG, "Unable to allocate memory for DSP event");
                ret = -ENOMEM;
                goto free_fe;
            }

            event_cfg->event_id = EVENT_ID_TISA_DSP_EVNT;
            event_cfg->event_config_payload_size = 0;
            event_cfg->is_register = 1;

            status = SessionAlsaUtils::registerMixerEvent(virtMixer, pcmDevIdTx.at(0),
                        backEndName.c_str(), MODULE_VI, (void *)event_cfg,
                        payload_size);
            if (status) {
                PAL_ERR(LOG_TAG, "Unable to register EVENT_ID_TISA_DSP_EVNT event to DSP with %d", status);
            }

            status = rm->registerMixerEventCallback(pcmDevIdTx, sessionCb, (uint64_t)this, true);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to register callback to rm with %d", status);
            }
#if defined(SILENCE_COUNT_ON_PAL)
            for (int ch_idx = 0; ch_idx < numberOfChannels; ch_idx++)
                silencestat[ch_idx].silenceCount = 0;
#endif /* SILENCE_COUNT_ON_PAL */
        } /* not handset mode end */
#endif /* DEBUG_DIS_TX */

        // Setting up SP mode
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

        miid = 0;
        ret = session->getMIID(backEndNameRx.c_str(), MODULE_SP, &miid);
        if (ret || (miid == 0)) {
            if (!handset) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", MODULE_SP, ret);
                goto err_pcm_open;
            } else {
                PAL_INFO(LOG_TAG, "PAL_DEVICE_OUT_HANDSET without MODULE_SP");
                miid = 0;
                ret = 0;
            }
        }

        // Set the operation mode for SP module
        PAL_DBG(LOG_TAG, "Operation mode for SP %d",
                        rm->mSpkrProtModeValue.operationMode);

        spModeConfg.operation_mode = NORMAL_MODE;

        if (miid) {
            builder->payloadTISPConfig(&payload, &payloadSize, miid,
                        TI_PARAM_ID_SP_SET_HANDSET_MODE, (void *)&handset);
            if (payloadSize) {
                ret = updateCustomPayload(payload, payloadSize);
                free(payload);
                if (0 != ret) {
                    PAL_ERR(LOG_TAG,
                            "updateCustomPayload Failed for TI_PARAM_ID_SP_SET_HANDSET_MODE\n");
                }
            }
        }

        if (!handset) {
            spCalibData tical;
            tical.num_ch = numberOfChannels;

            memset(tical.r0_q19, 0, sizeof(tical.r0_q19));
            memset(tical.t0, 0, sizeof(tical.t0));

            ret = get_calib_data(&tical);

            if (!ret) {
                builder->payloadTISPConfig(&payload, &payloadSize, miid,
                                           TI_PARAM_ID_SP_CALIB_DATA, (void *)&tical);
                if (payloadSize) {
                    ret = updateCustomPayload(payload, payloadSize);
                    free(payload);
                    if (0 != ret) {
                        PAL_ERR(LOG_TAG, " updateCustomPayload Failed for TI_PARAM_ID_SP_CALIB_DATA\n");
                    }
                }
            } else {
                PAL_ERR(LOG_TAG,
                        "[TI-SmartPA] Error while getting calibration data, calibration data not applied\n");
            }

            int iv_vbat;

            ret = get_data_from_file(TAS25XX_EFS_IV_VBAT, HEX, (void*)&iv_vbat);
            if (!ret) {

                PAL_DBG(LOG_TAG, "iv_vbat 0x%x, iv bitwidth=%d, vbat_mon=%d\n", iv_vbat,
                        iv_vbat & 0xFFFF, (iv_vbat & 0xFFFF0000) >> 16);

                builder->payloadTISPConfig(&payload, &payloadSize, miid,
                                           TI_PARAM_ID_SP_SET_VBAT_MON, (void *)&iv_vbat);
                if (payloadSize) {
                    ret = updateCustomPayload(payload, payloadSize);
                    free(payload);
                    if (0 != ret) {
                        PAL_ERR(LOG_TAG, " updateCustomPayload Failed for TI_PARAM_ID_SP_SET_VBAT_MON\n");
                    }
                }
            } else {
                PAL_ERR(LOG_TAG,
                        "[TI-SmartPA] Error while getting iv_vbat format\n");
            }

            int drv_op_mode;

            ret = get_data_from_file(TAS25XX_EFS_DRV_OP_MODE, HEX, (void *)&drv_op_mode);

            PAL_DBG(LOG_TAG, "drv_op_mode 0x%x, bypass=%d, LR mode=%d\n", drv_op_mode,
                    (drv_op_mode & 0xF0) >> 4, drv_op_mode & 0xF);

            if (!ret) {
                builder->payloadTISPConfig(&payload, &payloadSize, miid,
                                           TI_PARAM_ID_SP_SET_DRV_OP_MODE, (void *)&drv_op_mode);
                if (payloadSize) {
                    ret = updateCustomPayload(payload, payloadSize);
                    free(payload);
                    if (0 != ret) {
                        PAL_ERR(LOG_TAG, " updateCustomPayload Failed for TI_PARAM_ID_SP_SET_DRV_OP_MODE\n");
                    }
                }
            } else {
                PAL_ERR(LOG_TAG,
                        "[TI-SmartPA] Error while getting driver op mode format\n");
            }
        } /* not handset mode end */

        if (customPayloadSize) {
            ret = SessionAlsaUtils::setDeviceCustomPayload(rm, backEndNameRx, customPayload,
                    customPayloadSize);

            free(customPayload);
            customPayload = NULL;
            customPayloadSize = 0;

            if (ret) {
                PAL_ERR(LOG_TAG,
                        "Unable to set custom param for SP_OP_MODE and TI_PARAM_ID_SP_CALIB_DATA");
                goto err_pcm_open;
            }
        }

#ifndef DEBUG_DIS_TX
#ifdef DISABLE_TX_IN_HANDSET
        if(!handset)
#endif
        {
            enableDevice(audioRoute, mSndDeviceName_vi);
            PAL_INFO(LOG_TAG, "pcm start for TX");
            if (pcm_start(txPcm) < 0) {
                PAL_ERR(LOG_TAG, "pcm start failed for TX path");
                goto err_pcm_open;
            }
        } /* not handset mode end */
#endif /* DEBUG_DIS_TX */
        // Free up the local variables
        goto exit;
    } else {
        if (numberOfRequest == 0) {
            PAL_ERR(LOG_TAG, "Device not started yet, Stop not expected");
            goto exit;
        }
        numberOfRequest--;
        if (numberOfRequest > 0) {
            // R0T0 already set, we don't need to process the request.
            goto exit;
        }

        printLibVersion(1);
        printACDBInfo(1);

#ifdef DISABLE_TX_IN_HANDSET
        if(!handset)
#endif
        {
            smartpa_get_bigdata();
        }

        spkrProtSetSpkrStatus(flag);
#ifndef DEBUG_DIS_TX
#ifdef DISABLE_TX_IN_HANDSET
        if(!handset)
#endif
        {
            // Speaker not in use anymore. Stop the processing mode
            PAL_INFO(LOG_TAG, "Closing VI path");
            if (txPcm) {
                device.id = PAL_DEVICE_IN_VI_FEEDBACK;

                ret = rm->getAudioRoute(&audioRoute);
                if (0 != ret) {
                    PAL_ERR(LOG_TAG, "Failed to get the audio_route address status %d", ret);
                    goto exit;
                }

                // ret = rm->getSndDeviceName(device.id , mSndDeviceName_vi);
                strlcpy(mSndDeviceName_vi, vi_device.sndDevName.c_str(), DEVICE_NAME_MAX_SIZE);
                rm->getBackendName(device.id, backEndName);
                if (!strlen(backEndName.c_str())) {
                    PAL_ERR(LOG_TAG, "Failed to obtain tx backend name for %d", device.id);
                    goto exit;
                }

                if (event_cfg != NULL) {
                    event_cfg->is_register = 0;

                    status = SessionAlsaUtils::registerMixerEvent(virtMixer, pcmDevIdTx.at(0),
                                backEndName.c_str(), MODULE_VI, (void *)event_cfg,
                                payload_size);
                    if (status) {
                        PAL_ERR(LOG_TAG, "Unable to deregister event to DSP with %d", status);
                    }
                    free(event_cfg);
                }

                status = rm->registerMixerEventCallback(pcmDevIdTx, sessionCb, (uint64_t)this, false);
                if (status) {
                    PAL_ERR(LOG_TAG, "Failed to deregister callback to rm with %d", status);
                }

                pcm_stop(txPcm);
                if (pcmDevIdTx.size() != 0) {
                    if (isTxFeandBeConnected) {
                        disconnectFeandBe(pcmDevIdTx, backEndName);
                    }

                    rm->freeFrontEndIds(pcmDevIdTx, sAttr, dir);
                    pcmDevIdTx.clear();
                }
                pcm_close(txPcm);
                disableDevice(audioRoute, mSndDeviceName_vi);
                txPcm = NULL;
                goto exit;
            }
        } /* not handset mode end */
#endif /* DEBUG_DIS_TX */
    }

err_pcm_open :
#ifndef DEBUG_DIS_TX
#ifdef DISABLE_TX_IN_HANDSET
    if(!handset)
#endif
    {
        if (pcmDevIdTx.size() != 0) {
            if (isTxFeandBeConnected) {
                disconnectFeandBe(pcmDevIdTx, backEndName);
            }
            rm->freeFrontEndIds(pcmDevIdTx, sAttr, dir);
            pcmDevIdTx.clear();
        }
        if (txPcm) {
            if (event_cfg != NULL) {
                event_cfg->is_register = 0;

                status = SessionAlsaUtils::registerMixerEvent(virtMixer, pcmDevIdTx.at(0),
                            backEndName.c_str(), MODULE_VI, (void *)event_cfg,
                            payload_size);
                if (status) {
                    PAL_ERR(LOG_TAG, "Unable to deregister event to DSP with %d", status);
                }
                free(event_cfg);
            }

            status = rm->registerMixerEventCallback(pcmDevIdTx, sessionCb, (uint64_t)this, false);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to deregister callback to rm with %d", status);
            }

            pcm_close(txPcm);
            disableDevice(audioRoute, mSndDeviceName_vi);
            txPcm = NULL;
        }
    } /* not handset mode end */
#endif /* DEBUG_DIS_TX */
    goto exit;

free_fe:
#ifndef DEBUG_DIS_TX
#ifdef DISABLE_TX_IN_HANDSET
    if(!handset)
#endif
    {
        if (pcmDevIdTx.size() != 0) {
            if (isTxFeandBeConnected) {
                disconnectFeandBe(pcmDevIdTx, backEndName);
            }
            rm->freeFrontEndIds(pcmDevIdTx, sAttr, dir);
            pcmDevIdTx.clear();
        }
    } /* not handset mode end */
#endif /* DEBUG_DIS_TX */

exit:
    if (deviceMetaData.buf) {
        free(deviceMetaData.buf);
        deviceMetaData.buf = nullptr;
        deviceMetaData.size = 0;
    }

    deviceMutex.unlock();
    if (builder) {
       delete builder;
       builder = NULL;
    }
    return ret;
}

void SpeakerProtectionTI::updateSPcustomPayload(int param_id, void *spConfig)
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

    rm->getBackendName(mDeviceAttr.id, backEndName);
    dev = Device::getInstance(&mDeviceAttr, rm);
    ret = rm->getActiveStream_l(activeStreams, dev);
    if ((0 != ret) || (activeStreams.size() == 0)) {
        PAL_ERR(LOG_TAG, " no active stream available");
        goto exit;
    }
    stream = static_cast<Stream *>(activeStreams[0]);
    stream->getAssociatedSession(&session);
    ret = session->getMIID(backEndName.c_str(), MODULE_SP, &miid);
    if (ret) {
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", MODULE_SP, ret);
        goto exit;
    }

    if (customPayloadSize) {
        free(customPayload);
        customPayload = NULL;
        customPayloadSize = 0;
    }

    payloadSize = 0;
    builder->payloadTISPConfig(&payload, &payloadSize, miid,
                    param_id, spConfig);
    if (payloadSize) {
        ret = updateCustomPayload(payload, payloadSize);
        free(payload);
        if (0 != ret) {
            PAL_ERR(LOG_TAG," updateCustomPayload Failed\n");
        }
    }

exit:
    if (builder) {
       delete builder;
       builder = NULL;
    }
    return;
}

int SpeakerProtectionTI::start()
{
    PAL_DBG(LOG_TAG, "Enter");

    if (ResourceManager::isVIRecordStarted) {
        PAL_DBG(LOG_TAG, "record running so just update SP payload");
    }
    else {
        spkrProtProcessingMode(true);
    }

    PAL_DBG(LOG_TAG, "Calling Device start");
    Device::start();
    return 0;
}

int SpeakerProtectionTI::stop()
{
    PAL_DBG(LOG_TAG, "Inside Speaker Protection stop");
    Device::stop();
    if (ResourceManager::isVIRecordStarted) {
        PAL_DBG(LOG_TAG, "record running so no need to proceed");
        ResourceManager::isVIRecordStarted = false;
        return 0;
    }
    spkrProtProcessingMode(false);
    return 0;
}

int32_t SpeakerProtectionTI::getMixerControl(struct mixer_ctl **ctl, unsigned *miid,
        char *mixer_str) {

    std::shared_ptr<Device> dev = nullptr;
    struct pal_device device;
    std::vector<Stream*> activeStreams;
    Session *session = NULL;
    Stream *stream = NULL;
    int ret = 0, status;
    std::shared_ptr<ResourceManager> rm = nullptr;
    int pcmid;
    std::string backEndName;

    rm = ResourceManager::getInstance();

    rm->getBackendName(mDeviceAttr.id, backEndName);
    if (!strlen(backEndName.c_str())) {
        ret = -EINVAL;
        PAL_ERR(LOG_TAG, "Failed to obtain rx backend name for %d", mDeviceAttr.id);
        goto exit;
    }

    memset(&device, 0, sizeof(device));
    device.id = PAL_DEVICE_OUT_SPEAKER;

    dev = Device::getInstance(&mDeviceAttr, rm);

    ret = rm->getActiveStream_l(activeStreams, dev);
    if ((0 != ret) || (activeStreams.size() == 0)) {
        ret = -EAGAIN;
        goto exit;
    }
    stream = static_cast<Stream *>(activeStreams[0]);
    stream->getAssociatedSession(&session);

    ret = session->getMIID(backEndName.c_str(), MODULE_SP, miid);
    if (ret) {
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", MODULE_SP, ret);
        goto exit;
    }

    PAL_DBG(LOG_TAG, "%s miid = %x(%d)", __func__, *miid, *miid);

#ifdef ENABLE_TAS_SPK_PROT
    *ctl = session->getFEMixerCtl(mixer_str, &pcmid, PAL_AUDIO_OUTPUT);
#else
    *ctl = session->getFEMixerCtl(mixer_str, &pcmid);
#endif

exit:
    return ret;
}

int32_t SpeakerProtectionTI::runTICalibration()
{
    std::shared_ptr<Device> dev = nullptr;
    struct pal_device device;
    char mSndDeviceName[128] = {0};
    std::string backEndName;
    std::vector<Stream*> activeStreams;
    Session *session = NULL;
    Stream *stream = NULL;
    int ret, status;
    unsigned miid;
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    static struct mixer *virtMixer;
    std::shared_ptr<ResourceManager> rm = nullptr;
    int pcmid;
    struct mixer_ctl *ctl = NULL;
    struct mixer_ctl *ctlsetparam = NULL;
    struct pal_stream_attributes sAttr;
    struct pal_device_info devinfo;
    std::vector<int> pcmDevIdsRx;
    std::ostringstream cntrlName;
    char *pcmDeviceName = NULL;
    uint32_t data[8] = { 0 };
    int32_t *pdata;
    int32_t n1q19 = 1 << 19;
    char *pcalibRefiles[] = {TAS25XX_EFS_CALIB_RE_L, TAS25XX_EFS_CALIB_RE_R};
    char *pcalibTempfiles[] = {TAS25XX_EFS_CALIB_TEMP_L, TAS25XX_EFS_CALIB_TEMP_R};

    int calib_success[MAX_SP_SPKRS] = {0};
    int re_range[MAX_SP_SPKRS][2] = {{0}, {0}};
    int amb_temp_range[MAX_SP_SPKRS][2] = {{0}, {0}};
    int calib_passed_for_both_ch = 0;
    int num_retry = 0;

    char *dev_cal_init_mxr_str[MAX_SP_SPKRS] = {
        "TAS25XX CALIB_INIT_TOP",
        "TAS25XX CALIB_INIT_BOTTOM",
    };

    struct mixer_ctl *calCtrl = NULL;

    char *cal_start_status =  "Enabled";
    char *cal_stop_status =  "Disabled";

    int32_t getRe[MAX_SP_SPKRS] = { 0 };
    int32_t amb_temp[MAX_SP_SPKRS] = { 0 };

    rm = ResourceManager::getInstance();

    PayloadBuilder* builder = new PayloadBuilder();

    if (hwMixer) {
        for (int ch_idx = 0; ch_idx < numberOfChannels; ch_idx++) {
            std::ostringstream calCtrlName;
            calCtrlName << dev_cal_init_mxr_str[ch_idx];
            PAL_DBG(LOG_TAG, "get mixer control: %s", calCtrlName.str().data());
            calCtrl = mixer_get_ctl_by_name(hwMixer, calCtrlName.str().data());
            if (!calCtrl) {
                PAL_ERR(LOG_TAG, "invalid mixer control: %s for hwMixer",
                    calCtrlName.str().data());
            } else {
                PAL_INFO(LOG_TAG, "apply mixer control: %s", calCtrlName.str().data());
                ret = mixer_ctl_set_enum_by_string(calCtrl, "ENABLE");
                if (ret) {
                    PAL_ERR(LOG_TAG, "Error: %d, Mixer control %s set with %s failed", ret,
                    calCtrlName.str().data(), "ENABLE");
                }
            }
        }
    } else {
        PAL_ERR(LOG_TAG, "Error: running calib without device initialisation");
    }

    if (!builder) {
        PAL_ERR(LOG_TAG, "unable to allocate memory for builder");
        goto exit;
    }

    ret = save_data_to_file(TAS25XX_EFS_CALIB_STATUS, STRING, (void*)cal_start_status);
    if (ret) {
        PAL_ERR(LOG_TAG, "Failed to set calibration start status to %s",
                TAS25XX_EFS_CALIB_STATUS);
    }

    PAL_DBG(LOG_TAG, "%s mDeviceAttr.id=%d, we are creating %d",
        __func__, mDeviceAttr.id, PAL_DEVICE_OUT_SPEAKER);

    ret = getMixerControl(&ctl, &miid, GET_MIXER_STR);
    if (ret == -EAGAIN) {
        PAL_DBG(LOG_TAG, "No active stream during calibration");
        goto exit;
    } else if (ret || !ctl) {
        status = -ENOENT;
        PAL_ERR(LOG_TAG, "Error: %d Invalid mixer control: %s\n",
                status, GET_MIXER_STR);
        goto exit;
    }

    ret = getMixerControl(&ctlsetparam, &miid, SET_MIXER_STR);
    if (ret == -EAGAIN) {
        PAL_DBG(LOG_TAG, "No active stream during calibration");
        goto exit;
    } else if (ret || !ctl) {
        status = -ENOENT;
        PAL_ERR(LOG_TAG, "Error: %d Invalid mixer control: %s\n",
                status, SET_MIXER_STR);
        goto exit;
    }

    for (int ch_idx = 0; ch_idx < numberOfChannels; ch_idx++) {
        builder->payloadTISPConfig(&payload, &payloadSize, miid,
             TAS_CALC_PARAM_IDX(TAS_SA_GET_RE_RANGE, 2, ch_idx+1), (void *)data);
        if (payload) {
            status = mixer_ctl_set_array(ctl, payload, payloadSize);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Set failed status = %d for TAS_SA_GET_RE_RANGE for ch-%d",
                        status, ch_idx);
                goto exit;
            }
            status = mixer_ctl_get_array(ctl, payload, payloadSize);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Get failed status = %d for TAS_SA_GET_RE_RANGE for ch-%d",
                        status, ch_idx);
            } else {
                pdata = (int32_t *)(payload + sizeof(struct apm_module_param_data_t));

                re_range[ch_idx][0] = pdata[0] >> 8; /*q27 to q19*/
                re_range[ch_idx][1] = pdata[1] >> 8; /*q27 to q19*/

                PAL_INFO(LOG_TAG,
                    "[TI-SmartPA] Get Re Range 0x%x(%.2lf) - 0x%x(%.2lf) for ch-%d\n",
                    re_range[ch_idx][0],(float)re_range[ch_idx][0] / n1q19 ,
                    re_range[ch_idx][1], (float)re_range[ch_idx][1] / n1q19, ch_idx);

            }

            free(payload);
            payload = NULL;
            payloadSize = 0;
        }

        builder->payloadTISPConfig(&payload, &payloadSize, miid,
             TAS_CALC_PARAM_IDX(TAS_SA_GET_AMB_TEMP_RANGE, 2, ch_idx+1), (void *)data);
        if (payload) {
            status = mixer_ctl_set_array(ctl, payload, payloadSize);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Set failed status = %d for TAS_SA_GET_AMB_TEMP_RANGE for ch-%d",
                        status, ch_idx);
                goto exit;
            }
            status = mixer_ctl_get_array(ctl, payload, payloadSize);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Get failed status = %d for TAS_SA_GET_AMB_TEMP_RANGE for ch-%d",
                        status, ch_idx);
            } else {
                pdata = (int32_t *)(payload + sizeof(struct apm_module_param_data_t));

                amb_temp_range[ch_idx][0] = pdata[0];
                amb_temp_range[ch_idx][1] = pdata[1];

                PAL_DBG(LOG_TAG,
                        "[TI-SmartPA] Get amb_temp Range %d - %d for ch-%d from DSP\n",
                        amb_temp_range[ch_idx][0],
                        amb_temp_range[ch_idx][1], ch_idx);
            }
            if(!amb_temp_range[ch_idx][0])
                amb_temp_range[ch_idx][0] = MIN_AMB_TEMP;
            if(!amb_temp_range[ch_idx][1])
                amb_temp_range[ch_idx][1] = MAX_AMB_TEMP;
            PAL_DBG(LOG_TAG,
                    "[TI-SmartPA] amb_temp Range updated to %d - %d for ch-%d\n",
                    amb_temp_range[ch_idx][0],
                    amb_temp_range[ch_idx][1], ch_idx);

            free(payload);
            payload = NULL;
            payloadSize = 0;
        }
    } /*<< done get Re and amb temp range */

    while (num_retry < NUM_MAX_CALIB_RETRY) {
        for (int ch_idx = 0; ch_idx < numberOfChannels; ch_idx++) {
            if (calib_success[ch_idx])
                continue;

            builder->payloadTISPConfig(&payload, &payloadSize, miid,
                    TAS_CALC_PARAM_IDX(TAS_SA_CALIB_INIT, 1, ch_idx+1), (void *)data);
            if (payload) {
                status = mixer_ctl_set_array(ctlsetparam, payload, payloadSize);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Set failed status = %d for TAS_SA_CALIB_INIT for ch-%d", status, ch_idx);
                }

                free(payload);
                payload = NULL;
                payloadSize = 0;
            }
        } /*<< calib init for failed channels done */

        sleep(CALIB_DURATION_SECS);

        for (int ch_idx = 0; ch_idx < numberOfChannels; ch_idx++) {
            if (calib_success[ch_idx])
                continue;

            builder->payloadTISPConfig(&payload, &payloadSize, miid,
                    TAS_CALC_PARAM_IDX(TAS_SA_GET_RE, 1, ch_idx+1), (void *)data);
            if (payload) {
                status = mixer_ctl_set_array(ctl, payload, payloadSize);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Set failed status = %d for TAS_SA_GET_RE for ch-%d", status, ch_idx);
                    goto exit;
                }

                status = mixer_ctl_get_array(ctl, payload, payloadSize);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Get failed status = %d for TAS_SA_GET_RE for ch-%d", status, ch_idx);
                } else {
                    pdata = (int32_t *)(payload + sizeof(struct apm_module_param_data_t));
                    PAL_INFO(LOG_TAG,
                        "[TI-SmartPA] Get Re 0x%x (%05.2lf) for ch-%d\n",
                        pdata[0], (float)pdata[0] / n1q19, ch_idx);
                    getRe[ch_idx] = pdata[0];

                }

                free(payload);
                payload = NULL;
                payloadSize = 0;
            }


            builder->payloadTISPConfig(&payload, &payloadSize, miid,
                    TAS_CALC_PARAM_IDX(TAS_SA_CALIB_DEINIT, 1, ch_idx+1), (void *)data);
            if (payload) {
                status = mixer_ctl_set_array(ctlsetparam, payload, payloadSize);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Set failed status = %d for TAS_SA_CALIB_DEINIT for ch-%d", status, ch_idx);
                }
                free(payload);
                payload = NULL;
                payloadSize = 0;
            }

            /* Re value within range for that channel */
            if ((getRe[ch_idx] > re_range[ch_idx][0]) && (getRe[ch_idx] < re_range[ch_idx][1])) {

                /* read temp from /sys/class/tas25xx/amb_temp */
                ret = get_data_from_file(TAS25xx_AMB_TEMP, INT, &amb_temp[ch_idx]);
                if (ret) {
                    PAL_ERR(LOG_TAG, "Error while Reading calibartion Temp from file %s for ch-%d",
                            TAS25xx_AMB_TEMP, ch_idx);
                    break;
                }

                if ((amb_temp[ch_idx] >= amb_temp_range[ch_idx][0]) && (amb_temp[ch_idx] <= amb_temp_range[ch_idx][1])) {
                    PAL_DBG(LOG_TAG, "Calibration Passed for ch-%d", ch_idx);
                } else {
                    PAL_DBG(LOG_TAG, "ambient temp(%d) out of range(%d-%d) for ch-%d, trial=%d, ignore & set to -1",
                            amb_temp[ch_idx], amb_temp_range[ch_idx][0], amb_temp_range[ch_idx][1], ch_idx, num_retry);
                    amb_temp[ch_idx] = -1;
                }
                calib_success[ch_idx] = 1;
            } else {
                PAL_DBG(LOG_TAG, "Calibration Failed for ch-%d, trial=%d", ch_idx, num_retry);
                calib_success[ch_idx] = 0;
            }

            if (!calib_success[ch_idx]) {
                amb_temp[ch_idx] = -1;
                getRe[ch_idx] = -1 * 0x80000;
            }
        } /*<< for each channel >> */

        if (numberOfChannels == 2) {
            calib_passed_for_both_ch = calib_success[0] && calib_success[1];
            PAL_DBG(LOG_TAG, "calibratin status %s, trail=%d", calib_passed_for_both_ch ? "Pass": "Fail", num_retry);
            if (calib_passed_for_both_ch)
                break;
        } else if (numberOfChannels == 1) {
            PAL_DBG(LOG_TAG, "calibratin status %s, trail=%d", calib_success[0] ? "Pass": "Fail", num_retry);
            if (calib_success[0])
                break;
        }
        usleep(CALIB_RETRY_DURATION_MS * 1000);

        num_retry++;
    } /*<< while >> */

    for (int ch_idx = 0; ch_idx < numberOfChannels; ch_idx++) {
        /* save re value for reference */
        float re_f = (float)getRe[ch_idx] / n1q19;

        ret = save_data_to_file(pcalibRefiles[ch_idx], FLOAT, (void*)&re_f);
        if (ret) {
            PAL_ERR(LOG_TAG, "Error while saving calibartion Re to file %s for ch-%d",
                pcalibRefiles[ch_idx], ch_idx);
        }

        ret = save_data_to_file(pcalibTempfiles[ch_idx], INT, (void*)&amb_temp[ch_idx]);
        if (ret) {
            PAL_ERR(LOG_TAG, "Error while saving calibartion Temp to file %s for ch-%d",
                    pcalibTempfiles[ch_idx],
                    ch_idx);
        }
    }

exit:
    if (hwMixer) {
        for (int ch_idx = 0; ch_idx < numberOfChannels; ch_idx++) {
            std::ostringstream calCtrlName;
            calCtrlName << dev_cal_init_mxr_str[ch_idx];
            PAL_DBG(LOG_TAG, "get mixer control: %s", calCtrlName.str().data());
            calCtrl = mixer_get_ctl_by_name(hwMixer, calCtrlName.str().data());
            if (!calCtrl) {
                PAL_ERR(LOG_TAG, "invalid mixer control: %s for hwMixer",
                    calCtrlName.str().data());
            } else {
                PAL_DBG(LOG_TAG, "apply mixer control: %s", calCtrlName.str().data());
                ret = mixer_ctl_set_enum_by_string(calCtrl, "DISABLE");
                if (ret) {
                    PAL_ERR(LOG_TAG, "Error: %d, Mixer control %s set with %s failed", ret,
                    calCtrlName.str().data(), "DISABLE");
                }
            }
        }
    }

    ret = save_data_to_file(TAS25XX_EFS_CALIB_STATUS, STRING, (void*)cal_stop_status);
    if (ret) {
        PAL_ERR(LOG_TAG, "Failed to set update calibration status to %s",
                TAS25XX_EFS_CALIB_STATUS);
    }

    if (builder) {
       delete builder;
       builder = NULL;
    }

    if(payload) {
       free(payload);
       payload = NULL;
    }

    return ret;
}

//TODO:
//CLASS MEMBERS
//bool mCalRunning;
//std::mutex calMutex;
//void SpeakerProtectionTI::spkrCalibrationThread()
//CLASS MEMBERS

void SpeakerProtectionTI::spkrCalibrationThread()
{
    calMutex.lock();

    if (mCalRunning) {
        PAL_DBG(LOG_TAG,
            "Duplicate request ignored PAL_PARAM_ID_SEPAKER_AMP_RUN_CAL");
    } else {
        mCalRunning = true;
        calMutex.unlock();
        runTICalibration();
        calMutex.lock();
        mCalRunning = false;
    }

    calMutex.unlock();
}

int32_t SpeakerProtectionTI::setParamstoDSP(void* param)
{
    struct mixer_ctl *ctl;
    unsigned miid = 0;
    PayloadBuilder* builder = new PayloadBuilder();
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    int i = 0, j = 0;
    int length;
    int channel;
    int status = 0;
    uint32_t param_idx;
    uint32_t *int_payload;

    setgetMutex.lock();

    pal_tispk_prot_param_t *p = (pal_tispk_prot_param_t *)param;
    channel = p->hdr[0];
    param_idx = p->hdr[1];
    length = p->hdr[2];
    int_payload = (uint32_t *)p->data;
    for (i = 0; i < length; i++) {
        PAL_DBG(LOG_TAG,
                "TI-SmartPA: set data raw, %d = 0x%x\n", i, int_payload[i]);
    }

    param_idx = TAS_CALC_PARAM_IDX(param_idx, length, channel);
    status = getMixerControl(&ctl, &miid, SET_MIXER_STR);
    if (status == -EAGAIN) {
        PAL_DBG(LOG_TAG, "TI-SmartPA: No active stream");
    } else if (status || !ctl) {
        status = -ENOENT;
        PAL_ERR(LOG_TAG, "TI-SmartPA: Error: %d Invalid mixer control: %s",
            status, SET_MIXER_STR);
        goto exit_setdsp;
    }

    builder->payloadTISPCommConfig(&payload, &payloadSize, miid,
            param_idx, (void*)int_payload,  PAL_TISA_SET_PARAM);

    if (payload && ctl) {
        status = mixer_ctl_set_array(ctl, payload, payloadSize);
        if (status) {
            PAL_ERR(LOG_TAG, "TI-SmartPA: setParam=0x%x, "
                    "mixer_ctl_set_array err-%d", param_idx, status);
            goto exit_setdsp;
        }

        PAL_DBG(LOG_TAG, "TI-SmartPA: send success paramidx-0x%x length-%d, ch-%d",
                param_idx, length, channel);
    }

exit_setdsp:
    setgetMutex.unlock();

    if (payload) {
        free(payload);
        payload = NULL;
        payloadSize = 0;
    }

    if (builder) {
        delete builder;
        builder = NULL;
    }
    return status;
}

int SpeakerProtectionTI::getParameterFromDSP(void **param)
{
    struct mixer_ctl *ctl;
    unsigned miid = 0;
    PayloadBuilder* builder = new PayloadBuilder();
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    int i = 0, j = 0;
    int channel;
    int length;
    int status = 0;
    uint32_t *int_payload;
    uint32_t param_idx;
    pal_tispk_prot_param_t *p;

    setgetMutex.lock();

    p = (pal_tispk_prot_param_t *)*param;
    channel = p->hdr[0];
    param_idx = p->hdr[1];
    length = p->hdr[2];
    int_payload = (uint32_t *)p->data;
    param_idx = TAS_CALC_PARAM_IDX(param_idx, length, channel);

    if (length > AUDIO_PARAM_MAX_LEN) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "TI-SmartPA: getparam invalid len %d", length);
        goto exit_getdsp;
    }

    status = getMixerControl(&ctl, &miid, GET_MIXER_STR);
    if (status == -EAGAIN) {
        PAL_DBG(LOG_TAG, "TI-SmartPA: No active stream");
    } else if (status || !ctl) {
        status = -ENOENT;
        PAL_ERR(LOG_TAG, "TI-SmartPA: Error: %d Invalid mixer control: %s",
            status, GET_MIXER_STR);
        goto exit_getdsp;
    }

    builder->payloadTISPCommConfig(&payload, &payloadSize, miid,
            param_idx, (void*)int_payload, PAL_TISA_GET_PARAM);

    if (payload && ctl) {
        status = mixer_ctl_set_array(ctl, payload, payloadSize);
        if (status) {
            PAL_ERR(LOG_TAG,
                    "TI-SmartPA: getParam=0x%x, mixer_ctl_set_array returned %d, ch-%d",
                    param_idx, status, channel);
            goto exit_getdsp;
        }

        status = mixer_ctl_get_array(ctl, payload, payloadSize);
        if (status) {
            PAL_ERR(LOG_TAG,
                    "TI-SmartPA: getParam=0x%x, mixer_ctl_get_array returned %d, ch-%d",
                    param_idx, status, channel);
        }

        int32_t *pdata = (int32_t *)(payload + sizeof(struct apm_module_param_data_t));

        memcpy(int_payload, pdata, sizeof(int32_t)*length);

        for (i = 0; i < length; i++) {
            PAL_DBG(LOG_TAG,
                    "TI-SmartPA: dsp get paramidx=0x%x, 0x%x",
                    param_idx + i, int_payload[i]);
        }
    }

exit_getdsp:
    setgetMutex.unlock();

    if (payload) {
        free(payload);
        payload = NULL;
        payloadSize = 0;
    }

    if (builder) {
        delete builder;
        builder = NULL;
    }
    return status;
}

int32_t SpeakerProtectionTI::setParameter(uint32_t param_id, void *param)
{
    PAL_DBG(LOG_TAG, "Inside Speaker Protection Set parameters");
    (void ) param;
    int32_t ret = 0;

#if defined(SEC_AUDIO_DUAL_SPEAKER) || defined(SEC_AUDIO_SCREEN_MIRRORING) // { SUPPORT_VOIP_VIA_SMART_VIEW
    struct audio_route *audioRoute = NULL;
    if (!rm) {
        PAL_ERR(LOG_TAG, "Failed to get the resourcemanager %d", ret);
        return ret;
    }
    ret = rm->getAudioRoute(&audioRoute);
    if (0 != ret) {
        PAL_ERR(LOG_TAG, "Failed to get the audio_route address status %d", ret);
        return ret;
    }

    if (param_id == PAL_PARAM_ID_SPEAKER_STATUS) {
        pal_param_speaker_status_t* param_speaker_status = (pal_param_speaker_status_t *)param;
        if (curMuteStatus == PAL_DEVICE_SPEAKER_MUTE) { // current : full mute
            if (param_speaker_status->mute_status == PAL_DEVICE_SPEAKER_UNMUTE) {
                disableDevice(audioRoute, "amp-mute");
                curMuteStatus = PAL_DEVICE_SPEAKER_UNMUTE; // full mute -> full unmute
            }
        } else { // current : full unmute, upper mute, upper unmute
            if (param_speaker_status->mute_status == PAL_DEVICE_SPEAKER_MUTE) {
                if (curMuteStatus == PAL_DEVICE_UPPER_SPEAKER_MUTE) {
                    disableDevice(audioRoute, "upper-spk-amp-mute");
                }
                enableDevice(audioRoute, "amp-mute");
                curMuteStatus = PAL_DEVICE_SPEAKER_MUTE; // upper mute or full unmute -> full mute
            } else if (param_speaker_status->mute_status == PAL_DEVICE_UPPER_SPEAKER_MUTE) {
                enableDevice(audioRoute, "upper-spk-amp-mute");
                curMuteStatus = PAL_DEVICE_UPPER_SPEAKER_MUTE; // full unmute -> upper mute
            } else if (param_speaker_status->mute_status == PAL_DEVICE_UPPER_SPEAKER_UNMUTE) {
                disableDevice(audioRoute, "upper-spk-amp-mute");
                curMuteStatus = PAL_DEVICE_SPEAKER_UNMUTE; // upper mute -> full unmute
            }
        }
    }
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW

    switch(param_id) {

    case PAL_PARAM_ID_SEPAKER_AMP_RUN_CAL: // 3000
    {
        PAL_DBG(LOG_TAG, "PAL_PARAM_ID_SEPAKER_AMP_RUN_CAL");
        std::thread tiCalThread(&SpeakerProtectionTI::spkrCalibrationThread, this);
        tiCalThread.detach();
    }
    break;

    case PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_RCV:
    case PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_SPK:
    {
        uint8_t* payload = NULL;
        size_t payloadSize = 0;
        mixer_ctl *ctlsetparam = NULL;
        unsigned miid;

        setgetMutex.lock();

        PayloadBuilder* builder = new PayloadBuilder();
        int ch_idx = (param_id == PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_SPK) ? 1 : 0;

        ret = getMixerControl(&ctlsetparam, &miid, SET_MIXER_STR);
        if (ret == -EAGAIN) {
            PAL_DBG(LOG_TAG, "No Active Stream, setParameter for param_id= %d not accessible",
                param_id);
        } else if (ret || !ctlsetparam) {
            PAL_ERR(LOG_TAG, "Error: %d Invalid mixer control: %s\n",
                    ret, SET_MIXER_STR);
            PAL_ERR(LOG_TAG, "PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE not set for ch-%d",
                    ch_idx);
        } else {
            pal_param_amp_ssrm_t* param_amp_temp = (pal_param_amp_ssrm_t *)param;
            if (param && builder) {
                PAL_DBG(LOG_TAG, "param_amp_temp temperature %d for ch-%d",
                    param_amp_temp->temperature, ch_idx);

                builder->payloadTISPConfig(&payload, &payloadSize, miid,
                    TAS_CALC_PARAM_IDX(TAS_SA_SET_SKIN_TEMP, 1, ch_idx+1),
                        (void *)&param_amp_temp->temperature);
                if (payload) {
                    ret = mixer_ctl_set_array(ctlsetparam, payload, payloadSize);
                    if (ret) {
                        PAL_ERR(LOG_TAG, "Set failed status = %d for TAS_SA_CALIB_DEINIT for ch-%d",
                            ret, ch_idx);
                    }

                    free(payload);
                    payload = NULL;
                    payloadSize = 0;
                }
            } else {
                PAL_ERR(LOG_TAG,
                    "PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE not set for ch-%d, param=%p, builder=%p",
                    ch_idx, param, builder);
            }
        }

        setgetMutex.unlock();

        if (builder) {
            delete builder;
            builder = NULL;
        }
    }
    break;

    case PAL_TISA_PARAM_GEN_SETPARAM: {
        ret = setParamstoDSP(param);
    }
    break;

    default:
        PAL_DBG(LOG_TAG, "Unknown param_id 0x%x", param_id);
    }

    return ret;
}

void SpeakerProtectionTI::printBigdata(int ch_idx)
{
    PAL_INFO(LOG_TAG,
        "[TI-SmartPA] big data [MaxTemp %d, MaxTemp(keep) %d, MaxExc %.3f, TOC %d EOC %d] for channel %d\n",
        bigdata.value[MAX_TEMP_L + ch_idx],
        bigdata.value[KEEP_MAX_TEMP_L + ch_idx],
        (float)bigdata.value[MAX_EXCU_L + ch_idx]/1000,
        bigdata.value[TEMP_OVER_L + ch_idx],
        bigdata.value[EXCU_OVER_L + ch_idx],
        ch_idx);
}


void SpeakerProtectionTI::smartpa_get_bigdata() {

    PAL_DBG(LOG_TAG, "Enter:");

    PayloadBuilder* builder = NULL;
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    std::string backEndName;
    std::shared_ptr<Device> dev = nullptr;
    Stream *stream = NULL;
    Session *session = NULL;
    std::vector<Stream*> activeStreams;
    uint32_t miid = 0;
    int32_t ret = 0;
    tismartpa_exc_temp_stats *pexc_temp_stats[2];
    int status = 0;
    struct mixer_ctl *ctl;
    int pcmid;
    int big_data_param_id[2] = {TI_PARAM_ID_SP_BIG_DATA_L,
            TI_PARAM_ID_SP_BIG_DATA_R};
    int silence_count_param_id[2] = {TI_PARAM_ID_SP_GET_SILENCE_COUNT_L,
            TI_PARAM_ID_SP_GET_SILENCE_COUNT_R};

    if (mDeviceAttr.id == PAL_DEVICE_OUT_HANDSET) {
        PAL_DBG(LOG_TAG, "[TI-SmartPA] NO BIGDATA QUERY - Handset Mode Selected");
        return;
    }

    PAL_DBG(LOG_TAG, "%d mDeviceAttr.id %d", __LINE__, mDeviceAttr.id);

    builder = new PayloadBuilder();

    setgetMutex.lock();

    rm->getBackendName(mDeviceAttr.id, backEndName);

    dev = Device::getInstance(&mDeviceAttr, rm);

    ret = rm->getActiveStream_l(activeStreams, dev);
    if ((0 != ret) || (activeStreams.size() == 0)) {
        PAL_ERR(LOG_TAG, " no active stream available");
        goto exit;
    }

    stream = static_cast<Stream *>(activeStreams[0]);
    stream->getAssociatedSession(&session);
    ret = session->getMIID(backEndName.c_str(), MODULE_SP, &miid);
    if (ret) {
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", MODULE_SP, ret);
        goto exit;
    }

#ifdef ENABLE_TAS_SPK_PROT
    ctl = session->getFEMixerCtl("getParam", &pcmid, PAL_AUDIO_OUTPUT);
#else
    ctl = session->getFEMixerCtl("getParam", &pcmid);
#endif
    if (ctl == nullptr) {
        goto exit;
    }

    PAL_DBG(LOG_TAG, "numberOfChannels %d", numberOfChannels);

    for (int ch_idx = 0; ch_idx < numberOfChannels; ch_idx++) {
        payloadSize = 0;
        builder->payloadTISPConfig(&payload, &payloadSize, miid,
                big_data_param_id[ch_idx], NULL);

        if (payload) {
            status = mixer_ctl_set_array(ctl, payload, payloadSize);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Set failed status = %d for TI_PARAM_ID_SP_BIG_DATA for channel %d",
                        status, ch_idx);
            }
            status = mixer_ctl_get_array(ctl, payload, payloadSize);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Get failed status = %d for TI_PARAM_ID_SP_BIG_DATA for channel %d",
                        status, ch_idx);
            } else {
                pexc_temp_stats[ch_idx] = (tismartpa_exc_temp_stats *)(payload + sizeof(struct apm_module_param_data_t));

                int cur_temperature = pexc_temp_stats[ch_idx]->Temperature_Max;
                int cur_excursion = pexc_temp_stats[ch_idx]->Excursion_Max, cur_excursion_um;
                int toc = pexc_temp_stats[ch_idx]->Temprature_over_count;
                if (!toc && cur_temperature > MAX_TEMP_VAL_CHECK)
                    cur_temperature = MAX_TEMP_VAL_CHECK;

                cur_excursion_um = (int)(((uint64_t)cur_excursion * 1000) >> 30); /* unit: um */

                PAL_INFO(LOG_TAG,
                    "[TI-SmartPA] session data [maxT %d, maxX %.3f%s, cntTmax %d cntXmax %d] for channel %d\n",
                    cur_temperature,
                    (float)cur_excursion_um/1000, ((cur_excursion == 0) ? " (silence)" : ""),
                    pexc_temp_stats[ch_idx]->Temprature_over_count,
                    pexc_temp_stats[ch_idx]->Excursion_over_count,
                    ch_idx);

                bigdata.value[MAX_TEMP_L + ch_idx] = MAX(cur_temperature, bigdata.value[MAX_TEMP_L + ch_idx]);
                bigdata.value[KEEP_MAX_TEMP_L + ch_idx] = MAX(cur_temperature, bigdata.value[KEEP_MAX_TEMP_L + ch_idx]);
                bigdata.value[MAX_EXCU_L + ch_idx] = MAX(cur_excursion_um, bigdata.value[MAX_EXCU_L + ch_idx]);
                bigdata.value[TEMP_OVER_L + ch_idx] += pexc_temp_stats[ch_idx]->Temprature_over_count;
                bigdata.value[EXCU_OVER_L + ch_idx] += pexc_temp_stats[ch_idx]->Excursion_over_count;

                printBigdata(ch_idx);
            }

            free(payload);
            payload = NULL;
            payloadSize = 0;
        }
    } /* excursion temp stats */

    for (int ch_idx = 0; ch_idx < numberOfChannels; ch_idx++) {
#if defined(SILENCE_COUNT_ON_PAL)
        if (silencestat[ch_idx].totalSilenceCount < (UINT32_MAX - silencestat[ch_idx].silenceCount))
            silencestat[ch_idx].totalSilenceCount += silencestat[ch_idx].silenceCount;
        else
            silencestat[ch_idx].totalSilenceCount = UINT32_MAX;
        PAL_INFO(LOG_TAG, "[TI-SmartPA] silence count stats: current(%d) total(%d) for channel %d",
                silencestat[ch_idx].silenceCount, silencestat[ch_idx].totalSilenceCount, ch_idx);
        silencestat[ch_idx].silenceCount = 0;
#else
        builder->payloadTISPConfig(&payload, &payloadSize, miid,
            silence_count_param_id[ch_idx], NULL);
        if (payload) {
            status = mixer_ctl_set_array(ctl, payload, payloadSize);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Set failed status = %d for TI_PARAM_ID_SP_GET_SILENCE_COUNT for channel %d",
                        status, ch_idx);
                goto exit;
            }
            status = mixer_ctl_get_array(ctl, payload, payloadSize);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Get failed status = %d for TI_PARAM_ID_SP_GET_SILENCE_COUNT for channel %d",
                        status, ch_idx);
            } else {
                silencestat[ch_idx].silenceCount = *((uint32_t *)(payload + sizeof(struct apm_module_param_data_t)));
                if (silencestat[ch_idx].totalSilenceCount < (UINT32_MAX - silencestat[ch_idx].silenceCount))
                    silencestat[ch_idx].totalSilenceCount += silencestat[ch_idx].silenceCount;
                else
                    silencestat[ch_idx].totalSilenceCount = UINT32_MAX;
                PAL_INFO(LOG_TAG, "[TI-SmartPA] silence count stats: current(%d) total(%d) for channel %d",
                        silencestat[ch_idx].silenceCount, silencestat[ch_idx].totalSilenceCount, ch_idx);
            }

            free(payload);
            payload = NULL;
            payloadSize = 0;
        }
#endif /* SILENCE_COUNT_ON_PAL */
    }

exit:
    setgetMutex.unlock();

    if (builder) {
        delete builder;
        builder = NULL;
    }
    return;

}

int32_t SpeakerProtectionTI::smartpa_get_temp(void *param, int ch_idx) {
    PayloadBuilder* builder = new PayloadBuilder();
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    std::string backEndName;
    std::shared_ptr<Device> dev = nullptr;
    Stream *stream = NULL;
    Session *session = NULL;
    std::vector<Stream*> activeStreams;
    uint32_t miid = 0;
    int32_t ret = 0;
    int status;
    int param_id;
    int tempq19, temp;

    int pcmid;
    struct mixer_ctl *ctl;

    setgetMutex.lock();

    param_id = TAS_CALC_PARAM_IDX(TAS_SA_GET_TV, 1, ch_idx+1);

    memset(param, 0, sizeof(int));

    rm->getBackendName(mDeviceAttr.id, backEndName);
    dev = Device::getInstance(&mDeviceAttr, rm);
    ret = rm->getActiveStream_l(activeStreams, dev);
    if ((0 != ret) || (activeStreams.size() == 0)) {
        ret = -EAGAIN;
        goto exit;
    }
    stream = static_cast<Stream *>(activeStreams[0]);
    stream->getAssociatedSession(&session);
    ret = session->getMIID(backEndName.c_str(), MODULE_SP, &miid);
    if (ret) {
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", MODULE_SP, ret);
        goto exit;
    }

#ifdef ENABLE_TAS_SPK_PROT
    ctl = session->getFEMixerCtl("getParam", &pcmid, PAL_AUDIO_OUTPUT);
#else
    ctl = session->getFEMixerCtl("getParam", &pcmid);
#endif
    if (ctl == nullptr) {
        ret = -EAGAIN;
        goto exit;
    }

    payloadSize = 0;
    builder->payloadTISPConfig(&payload, &payloadSize, miid,
            param_id, NULL);

    if (payload) {
        status = mixer_ctl_set_array(ctl, payload, payloadSize);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Set failed status = %d for TAS_SA_GET_TV for channel %d",
                    status, ch_idx);
        } else {
            status = mixer_ctl_get_array(ctl, payload, payloadSize);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Get failed status = %d for TAS_SA_GET_TV for channel %d",
                        status, ch_idx);
            } else {
                tempq19 = *((int *)(payload + sizeof(struct apm_module_param_data_t)));
                temp = (tempq19 + 0x40000) >> 19;
                PAL_INFO(LOG_TAG,
                    "[TI-SmartPA] Get TAS_SA_GET_TV temp %d(%f) for channel %d\n",
                    tempq19,
                    (float)tempq19/(1<<19),
                    ch_idx);
                memcpy(param, (void*)&temp, sizeof(int));
            }
        }
        ret = status;

        free(payload);
        payload = NULL;
        payloadSize = 0;
    }

exit:
    setgetMutex.unlock();

    if (builder) {
        delete builder;
        builder = NULL;
    }
    return ret;
}


int32_t SpeakerProtectionTI::getParameter(uint32_t param_id, void **param)
{
    int i;
    int32_t status = 0;
    switch(param_id) {
        case PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_SAVE_RESET:
        case PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_SAVE:
            PAL_DBG(LOG_TAG, "PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_SAVE %d", param_id);
            for (i = 0; i < numberOfChannels; i++)
                printBigdata(i);
            memcpy(&bigdataRef, &bigdata, sizeof(pal_param_amp_bigdata_t));
            *param = (void *)&bigdataRef;
            if (param_id == PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_SAVE_RESET) {
                PAL_DBG(LOG_TAG, "Resetting big data except entities to be kept");
                memset(&bigdata, 0, sizeof(int) * NUM_BIGDATA_TO_CLEAR);
            }
        break;

        case PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_RCV:
        case PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_SPK:
        {
            int ch_idx =
                (param_id == PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_SPK) ? 1 : 0;
            int32_t ret = smartpa_get_temp(&ssrm.temperature, ch_idx);
            *param = (void *)&ssrm.temperature;
            if (ret == 0)
                PAL_INFO(LOG_TAG, "TEMPERATURE_%s received %d",
                    ch_idx == 1 ? "SPK" : "RCV", ssrm.temperature);
        }
        break;

        case PAL_TISA_PARAM_GEN_GETPARAM:
        {
            status = getParameterFromDSP(param);
            if (status < 0)
                PAL_ERR(LOG_TAG,
                    "TI-SmartPA: getParameterFromDSP returned %d", status);
        }
        break;

        default :
            PAL_ERR(LOG_TAG, "Unsupported operation");
            status = -EINVAL;
        break;
    }
    return status;
}

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
void SpeakerProtectionTI::dump(int fd)
{
    dprintf(fd, " \n");
    dprintf(fd, "SpeakerProtectionTI info: \n");
    dprintf(fd, "\tnumberOfChannels: %d \n", numberOfChannels);
    dprintf(fd, "\tvi_device.channels: %d \n", vi_device.channels);
    dprintf(fd, "\tvi_device.bit_width: %d \n", vi_device.bit_width);
    dprintf(fd, "\tvi_device.samplerate: %d \n", vi_device.samplerate);

    dprintf(fd, "\ttotalSilenceCount L: %d R: %d\n",
        silencestat[0].totalSilenceCount, silencestat[1].totalSilenceCount);

    dprintf(fd, "SpeakerProtectionTI Calibration info: \n");               
    dprintf(fd, "\tRunning: %d \n", mCalRunning);
    
    dprintf(fd, "SpeakerProtectionTI Temperature info: \n");
    dprintf(fd, "\tmax_temperature L: %d (%d) R: %d (%d) \n",
        bigdata.value[MAX_TEMP_L], bigdata.value[KEEP_MAX_TEMP_L],
        bigdata.value[MAX_TEMP_R], bigdata.value[KEEP_MAX_TEMP_R]);
    dprintf(fd, "\ttemperature_over_cnt L: %d R: %d \n",
        bigdata.value[TEMP_OVER_L], bigdata.value[TEMP_OVER_R]);
    dprintf(fd, "SpeakerProtectionTI excursion info: \n");
    dprintf(fd, "\tmax_excursion L: %.3f R: %.3f \n",
        (float)bigdata.value[MAX_EXCU_L]/1000, (float)bigdata.value[MAX_EXCU_R]/1000);
    dprintf(fd, "\texcursion_over_cnt L: %d R: %d \n",
        bigdata.value[EXCU_OVER_L], bigdata.value[EXCU_OVER_R]);
}
#endif

void SpeakerProtectionTI::printLibVersion(int ch_idx)
{
    int status;
    pal_tispk_prot_param_t param, *pparam;
    std::ostringstream libversion;

    param.hdr[0] = ch_idx; // channel
    param.hdr[1] = LIB_VERSION_INDEX; // index
    param.hdr[2] = 1; // length
    param.hdr[3] = 1; // isget
    memset(param.data, 0, AUDIO_PARAM_MAX_LEN * sizeof(int));
    pparam = &param;
    status = getParameterFromDSP((void **)&pparam);
    if (status == 0) {
        libversion << std::hex << pparam->data[0];
        mLibVersion = libversion.str();
    }
	PAL_INFO(LOG_TAG, "[TI-SmartPA] Algo Library Version: 0x%s",
			mLibVersion.c_str());
}

void SpeakerProtectionTI::printACDBInfo(int ch_idx)
{
    int status;
    pal_tispk_prot_param_t param, *pparam;
    std::ostringstream paramversion;

    param.hdr[0] = ch_idx; // channel
    param.hdr[1] = ACDB_INFO_INDEX; // index
    param.hdr[2] = 2; // length
    param.hdr[3] = 1; // isget
    memset(param.data, 0, AUDIO_PARAM_MAX_LEN * sizeof(int));

    pparam = &param;
    status = getParameterFromDSP((void **)&pparam);
    if (status == 0) {
        paramversion << std::to_string((pparam->data[0] >> 17) & 0x1F) << "/" <<
            std::to_string((pparam->data[0] >> 22) & 0xF) << "/" <<
            std::to_string(pparam->data[0] >> 26) << "[dd-mm-yy] " <<
            std::to_string((pparam->data[0] >> 12) & 0x1F) << "h:" <<
            std::to_string((pparam->data[0] >> 6) & 0x3F) << "m:" <<
            std::to_string(pparam->data[0] & 0x3F) << "s [hh:mm:ss]" <<
            "\n[TI-SmartPA] Algo param version 8: 0x" << std::hex << pparam->data[1];
        mParamVersion = paramversion.str();
    }
	PAL_INFO(LOG_TAG, "[TI-SmartPA] Algo Param Version 7:%s", mParamVersion.c_str());
}
#endif // ENABLE_TAS_SPK_PROT
