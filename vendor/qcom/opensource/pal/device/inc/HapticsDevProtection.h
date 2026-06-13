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

#ifndef HAPTICSDEV_PROT
#define HAPTICSDEV_PROT

#include "HapticsDev.h"
#include "wsa_haptics_vi_api.h"
#include "rx_haptics_api.h"
#include <tinyalsa/asoundlib.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include<vector>
#include "apm_api.h"
#include "SpeakerProtection.h"

class HapticsDev;

typedef enum haptics_dev_prot_cal_state {
    HAPTICS_DEV_NOT_CALIBRATED,     /* HapticsDev not calibrated  */
    HAPTICS_DEV_CALIBRATED,         /* HapticsDev calibrated  */
    HAPTICS_DEV_CALIB_IN_PROGRESS,  /* HapticsDev calibration in progress  */
}haptics_dev_prot_cal_state;

typedef enum haptics_dev_prot_proc_state {
    HAPTICS_DEV_PROCESSING_IN_IDLE,     /* Processing mode in idle state */
    HAPTICS_DEV_PROCESSING_IN_PROGRESS, /* Processing mode in running state */
} haptics_dev_prot_proc_state;

enum {
    HAPTICS_DEV_RIGHT,    /* Right HapticsDev */
    HAPTICS_DEV_LEFT,     /* Left HapticsDev */
};

struct haptics_vi_cal_param {
    int32_t Re_ohm_Cal_q24 = 0;  // cal & ftm
    int32_t Fres_Hz_Cal_q20 = 0; // cal & ftm
    int32_t Bl_q24 = 0;          // cal & ftm
    int32_t Rms_KgSec_q24 = 0;   // cal & ftm
    int32_t Blq_ftm_q24 = 0;     // ftm
    int32_t Le_mH_ftm_q24 = 0;   // ftm
};

class HapticsDevProtection : public HapticsDev
{
protected :
    bool hapticsdevProtEnable;
    bool threadExit;
    bool triggerCal;
    int minIdleTime;
    static haptics_dev_prot_cal_state hapticsDevCalState;
    haptics_dev_prot_proc_state hapticsDevProcessingState;
    int *devTempList;
    static bool isHapDevInUse;
    static std::atomic<bool> calThrdCreated;
    static std::atomic<bool> ftmThrdCreated;
    static bool isDynamicCalTriggered;
    static struct timespec devLastTimeUsed;
    static struct mixer *virtMixer;
    static struct mixer *hwMixer;
    static struct pcm *rxPcm;
    static struct pcm *txPcm;
    static int numberOfChannels;
    static bool mDspCallbackRcvd;
    static haptics_vi_cal_param cbCalData[HAPTICS_MAX_OUT_CHAN];
    struct pal_device mDeviceAttr;
    std::vector<int> pcmDevIdTx;
    static int calibrationCallbackStatus;
    static int numberOfRequest;
    static struct pal_device_info vi_device;

private :

public:
    static std::thread mCalThread;
    static std::condition_variable cv;
    static std::mutex cvMutex;
    std::mutex deviceMutex;
    static std::mutex calibrationMutex;
    void HapticsDevCalibrationThread();
    void HapticsDevFTMThread();
    int getDevTemperature(int haptics_dev_pos);
    void HapticsDevCalibrateWait();
    int HapticsDevStartCalibration(int32_t operation_mode);
    void HapticsDevProtectionInit();
    void HapticsDevProtectionDeinit();
    void getHapticsDevTemperatureList();
    static void HapticsDevProtSetDevStatus(bool enable);
    static int setConfig(int type, int tag, int tagValue, int devId, const char *aif);
    bool isHapticsDevInUse(unsigned long *sec);

    HapticsDevProtection(struct pal_device *device,
                      std::shared_ptr<ResourceManager> Rm);
    ~HapticsDevProtection();

    int32_t start();
    int32_t stop();

    int32_t setParameter(uint32_t param_id, void *param) override;
    int32_t getParameter(uint32_t param_id, void **param) override;
    int32_t HapticsDevProtProcessingMode(bool flag);
    int HapticsDevProtectionDynamicCal();
	int HapticsDevProtectionFTM();
    void updateHPcustomPayload();
    static void handleHPCallback (uint64_t hdl, uint32_t event_id, void *event_data,
                                  uint32_t event_size);
    int getCpsDevNumber(std::string mixer);
//    int32_t getCalibrationData(void **param);
    int32_t getFTMParameter(void **param);
    int32_t getAndsetPersistentParameter(bool flag);
    void disconnectFeandBe(std::vector<int> pcmDevIds, std::string backEndName);

};

class HapticsDevFeedback : public HapticsDev
{
    protected :
    struct pal_device mDeviceAttr;
    static std::shared_ptr<Device> obj;
    static int numDevice;
    public :
    int32_t start();
    int32_t stop();
    HapticsDevFeedback(struct pal_device *device,
                    std::shared_ptr<ResourceManager> Rm);
    ~HapticsDevFeedback();
    void updateVIcustomPayload();
    static std::shared_ptr<Device> getInstance(struct pal_device *device,
                                               std::shared_ptr<ResourceManager> Rm);
};

#endif
