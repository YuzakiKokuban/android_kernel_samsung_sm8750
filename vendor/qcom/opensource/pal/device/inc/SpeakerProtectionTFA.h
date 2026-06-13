#include "Device.h"
#include "sp_vi.h"
#include "sp_rx.h"
#include <tinyalsa/asoundlib.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include "apm_api.h"

#ifdef SEC_AUDIO_COMMON
#include "SecPalDefs.h"
#endif

#ifdef ENABLE_TFA98XX_SUPPORT
//#define PAL_PARAM_ID_SET_TFADSP_RUN_CAL       3000
#define PAL_PARAM_ID_SET_TFADSP_SURF_TEMP     0x1000B941
#define PAL_PARAM_ID_GET_TFADSP_BLACKBOX_DATA 0x1000B942
#define PAL_PARAM_ID_GET_TFADSP_CAL_STATUS    0x1000B943
#define PAL_PARAM_ID_GET_TFADSP_CAL_DATA      0x1000B944
#define PAL_PARAM_ID_GET_TFADSP_CAL_TEMP      0x1000B945
#define PAL_PARAM_ID_GET_TFADSP_SPK_TEMP      0x1000B946

#ifndef SEC_AUDIO_COMMON
#define PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_RCV 3001
#define PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_SPK 3002
#define PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_SAVE_RESET 3003
#define PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_SAVE 3004
#define PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_START 3005
#define PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_STOP 3006

typedef struct pal_param_amp_ssrm {
    int temperature;
} pal_param_amp_ssrm_t;

#define NUM_OF_SPEAKER 2
#define NUM_OF_BIGDATA_ITEM 5 /* max_temp temp_over max_excu excu_over keep_max_temp */

typedef struct pal_param_amp_bigdata {
    int value[NUM_OF_SPEAKER * NUM_OF_BIGDATA_ITEM];
} pal_param_amp_bigdata_t;

enum bigdata_ids{
    MAX_TEMP_L, MAX_TEMP_R,
    TEMP_OVER_L, TEMP_OVER_R,
    MAX_EXCU_L, MAX_EXCU_R,
    EXCU_OVER_L, EXCU_OVER_R,
    KEEP_MAX_TEMP_L, KEEP_MAX_TEMP_R,
};
#endif

#define TFADSP_ACTIVATION_THREAD
#define TFADSP_LIVEDATA_LOGGING

class Device;
class SpeakerFeedbackTFA;

class SpeakerProtectionTFA : public Device
{
protected :
    bool active_state;
    struct pal_device mDeviceAttr;
#ifdef SET_IPCID_MIXER_CTL
    static struct mixer *virtMixer;
    static struct mixer *hwMixer;
    struct mixer_ctl *ipc_id_mctl;
#endif

private :
    std::thread ssrm_thread;
    std::mutex ssrm_mutex;
    //std::mutex cal_mutex;
    std::mutex ipc_mutex;
    std::condition_variable ssrm_wait_cond;
    bool exit_ssrm_thread;
    bool activate_tfadsp_thread_running;
    uint32_t ssrm_timer_cnt;
    uint32_t cal_suc_p; // 1 or 0
    uint32_t cal_suc_s; // 1 or 0
    uint32_t cal_val_p; // mOhm
    uint32_t cal_val_s; // mOhm
    int32_t cal_temp; // degree, board temperature during calibration
    uint32_t cal_status;
    uint32_t tfadsp_rx_miid;
    int32_t surft_left;
    int32_t surft_right;
    int32_t rcv_temp;
    int32_t spk_temp;
    int32_t active_spk_dev_count;
    bool bigdata_read;
    uint32_t tfadsp_state;
    bool debug_mode;
    bool skip_activate_tfadsp;

#if defined(SEC_AUDIO_DUAL_SPEAKER) || defined(SEC_AUDIO_SCREEN_MIRRORING) // { SUPPORT_VOIP_VIA_SMART_VIEW
    pal_device_speaker_mute curMuteStatus;
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW
#ifdef TFADSP_LIVEDATA_LOGGING
    bool livedata_logging_thread_running;
#endif

public:
    pal_param_amp_ssrm_t amp_ssrm;
    pal_param_amp_bigdata_t tfa_bigdata;
    pal_param_amp_bigdata_t tfa_bigdata_param;
    static std::mutex cal_mutex;
    static std::condition_variable cal_wait_cond;
    SpeakerProtectionTFA(struct pal_device *device,
                      std::shared_ptr<ResourceManager> Rm);
    ~SpeakerProtectionTFA();

    int32_t start();
    int32_t stop();
    int32_t setParameter(uint32_t param_id, void *param) override;
    int32_t getParameter(uint32_t param_id, void **param) override;
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    void dump(int fd) override;
#endif

    int32_t tfa_dsp_get_miid(const char *controlName, mixer_ctl **mixer_ctrl);
    int32_t tfa_dsp_msg_write(uint32_t param_id, const uint8_t *buffer, int len);
    int32_t tfa_dsp_msg_read(uint8_t *buffer, int len);
    int32_t tfa_dsp_read_state(uint8_t *buffer, int len, uint32_t param_id);
    int32_t tfa_dsp_read_lib_version();
    static void SSRM_Thread(SpeakerProtectionTFA& inst);
    void Calibration_Thread();
    void Activate_Tfadsp_Thread();
    int32_t read_tfa_power_state(uint32_t dev);

    int32_t activate_tfadsp();
    int32_t store_blackbox_data();
    int32_t set_default_spkt_data(uint32_t channel);

    int32_t reset_blackbox_data();
    int32_t run_calibration();
    uint32_t get_calibration_status();
    int32_t get_re25c(uint32_t *mOhm_P, uint32_t *mOhm_S);
    int32_t get_cal_temp();
    int32_t get_rdc(uint32_t *mOhm_P, uint32_t *mOhm_S);
    void    get_temp(int32_t *temp_l, int32_t *temp_r);
    void    set_sknt_control();
    void    get_spkt_data();
    void    load_cal_file();
    void    print_bigdata();
    void    handle_debug_command(int32_t debug_command);
#ifdef SET_IPCID_MIXER_CTL
    int32_t get_miid_pcmid_sc(uint32_t *miid, uint32_t *pcm_dev);
#endif
#ifdef TFADSP_LIVEDATA_LOGGING
    void Livedata_Logging_Thread();
#endif
};

class SpeakerFeedbackTFA : public Device
{
protected :
    struct pal_device mDeviceAttr;
    static std::shared_ptr<Device> obj;

public :
    int32_t start();
    int32_t stop();
    SpeakerFeedbackTFA(struct pal_device *device,
                    std::shared_ptr<ResourceManager> Rm);
    ~SpeakerFeedbackTFA();
    static std::shared_ptr<Device> getInstance(struct pal_device *device,
                                               std::shared_ptr<ResourceManager> Rm);
    static std::shared_ptr<Device> getObject();
};
#endif