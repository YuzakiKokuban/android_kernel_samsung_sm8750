/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef HOTWORD_INTERFACE_H
#define HOTWORD_INTERFACE_H

#include "VoiceUIInterface.h"
#include "detection_cmn_api.h"
#include "ar_osal_mem_op.h"

class HotwordInterface: public VoiceUIInterface {
  public:
    HotwordInterface(vui_intf_param_t *model);
    ~HotwordInterface();

    void DetachStream(void *stream) override;

    int32_t SetParameter(intf_param_id_t param_id,
        vui_intf_param_t *param) override;
    int32_t GetParameter(intf_param_id_t param_id,
        vui_intf_param_t *param) override;

    int32_t Process(intf_process_id_t type,
        vui_intf_param_t *in_out_param) { return 0; }

    int32_t RegisterModel(void *s,
        struct pal_st_sound_model *model,
        const std::vector<sound_model_data_t *> model_list) override;
    void DeregisterModel(void *s) override;

  private:
    static int32_t ParseSoundModel(struct pal_st_sound_model *sound_model,
                                   std::vector<sound_model_data_t *> &model_list);

    int32_t ParseRecognitionConfig(void *s,
                                   struct pal_st_recognition_config *config);

    void GetBufferingConfigs(void *s,
                             struct buffer_config *config);

    int32_t ParseDetectionPayload(void *event, uint32_t size);
    void* GetDetectedStream();
    void UpdateDetectionResult(void *s, uint32_t result);
    int32_t GenerateCallbackEvent(void *s,
                                  struct pal_st_recognition_event **event,
                                  uint32_t *event_size);

    int32_t GetSoundModelLoadPayload(vui_intf_param_t *param);
    int32_t GetBufferingPayload(vui_intf_param_t *param);
    void SetStreamAttributes(struct pal_stream_attributes *attr);

    void SetSTModuleType(st_module_type_t model_type) {
        module_type_ = model_type;
    }
    st_module_type_t GetModuleType(void *s) { return module_type_; }
    uint32_t GetReadOffset() { return read_offset_; }
    void SetReadOffset(uint32_t offset) { read_offset_ = offset; }
    uint32_t UsToBytes(uint64_t input_us);

    st_module_type_t module_type_;
    sound_model_info_map_t sm_info_map_;
    struct pal_stream_attributes str_attr_;

    uint32_t read_offset_ = 0;

    uint8_t *custom_event_;
    uint32_t custom_event_size_;
    struct buffer_config default_buf_config_;
    struct param_id_detection_engine_buffering_config_t buffering_config_;
};

#endif
