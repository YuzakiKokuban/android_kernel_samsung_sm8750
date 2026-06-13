/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CUSTOM_VA_INTERFACE_H
#define CUSTOM_VA_INTERFACE_H

#include "VoiceUIInterface.h"
#include "detection_cmn_api.h"
#include "ar_osal_mem_op.h"

class CustomVAInterface: public VoiceUIInterface {
  public:
    CustomVAInterface(vui_intf_param_t *model);
    ~CustomVAInterface();

    void DetachStream(void *stream) override;

    int32_t SetParameter(intf_param_id_t param_id,
        vui_intf_param_t *param) override;
    int32_t GetParameter(intf_param_id_t param_id,
        vui_intf_param_t *param) override;

    int32_t Process(intf_process_id_t type,
        vui_intf_param_t *in_out_param) override;

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
    void GetSecondStageConfLevels(void *s,
                                  listen_model_indicator_enum type,
                                  uint32_t *level);

    void SetSecondStageDetLevels(void *s,
                                 listen_model_indicator_enum type,
                                 uint32_t level);

    int32_t ParseDetectionPayload(void *event, uint32_t size);
    void* GetDetectedStream();
    void* GetDetectionEventInfo();
    int32_t GenerateCallbackEvent(void *s,
                                  struct pal_st_recognition_event **event,
                                  uint32_t *event_size);

    void ProcessLab(void *data, uint32_t size) {}

    void UpdateFTRTData(void *data, uint32_t size) {}

    int32_t ParseOpaqueConfLevels(struct sound_model_info *info,
                                  void *opaque_conf_levels,
                                  uint32_t version,
                                  uint8_t **out_conf_levels,
                                  uint32_t *out_num_conf_levels);
    int32_t FillConfLevels(struct sound_model_info *info,
                           struct pal_st_recognition_config *config,
                           uint8_t **out_conf_levels,
                           uint32_t *out_num_conf_levels);
    int32_t FillOpaqueConfLevels(uint32_t model_id,
                                 const void *sm_levels_generic,
                                 uint8_t **out_payload,
                                 uint32_t *out_payload_size,
                                 uint32_t version);
    int32_t ParseDetectionPayloadPDK(void *event_data);
    int32_t ParseDetectionPayloadGMM(void *event_data);
    void UpdateKeywordIndex(uint64_t kwd_start_timestamp,
                            uint64_t kwd_end_timestamp,
                            uint64_t ftrt_start_timestamp);
    void PackEventConfLevels(struct sound_model_info *sm_info,
                             uint8_t *opaque_data);
    void FillCallbackConfLevels(struct sound_model_info *sm_info,
                                uint8_t *opaque_data,
                                uint32_t det_keyword_id,
                                uint32_t best_conf_level);
    void CheckAndSetDetectionConfLevels(void *s);
    void UpdateDetectionResult(void *s, uint32_t result);
    int32_t GetSoundModelLoadPayload(vui_intf_param_t *param);
    int32_t GetCustomPayload(vui_intf_param_t *param);
    int32_t GetBufferingPayload(vui_intf_param_t *param);
    void SetStreamAttributes(struct pal_stream_attributes *attr);

    void SetSTModuleType(st_module_type_t model_type) {
        module_type_ = model_type;
    }
    st_module_type_t GetModuleType(void *s) { return module_type_; }
    void GetKeywordIndex(struct keyword_index *index);
    uint32_t GetFTRTDataSize() { return ftrt_size_; }
    uint32_t GetReadOffset() { return read_offset_; }
    void SetReadOffset(uint32_t offset) { read_offset_ = offset; }
    uint32_t UsToBytes(uint64_t input_us);

    st_module_type_t module_type_;
    sound_model_info_map_t sm_info_map_;
    SoundModelInfo *sound_model_info_;
    uint32_t start_index_ = 0;
    uint32_t end_index_ = 0;
    uint32_t ftrt_size_ = 0;
    uint32_t read_offset_ = 0;

    bool use_qc_wakeup_config_ = false;

    uint32_t conf_levels_intf_version_;
    st_confidence_levels_info *st_conf_levels_;
    st_confidence_levels_info_v2 *st_conf_levels_v2_;
    struct pal_stream_attributes str_attr_;

    struct detection_event_info detection_event_info_;
    struct detection_event_info_pdk detection_event_info_multi_model_;
    uint32_t det_model_id_;
    uint8_t *custom_event_;
    uint32_t custom_event_size_;
    struct buffer_config default_buf_config_;
    struct param_id_detection_engine_buffering_config_t buffering_config_;
};

#endif
