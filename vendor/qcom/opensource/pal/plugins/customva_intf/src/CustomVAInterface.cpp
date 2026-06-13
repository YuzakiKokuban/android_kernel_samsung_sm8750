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
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: CustomVAInterface"
//#define LOG_NDEBUG 0

#include <log/log.h>
#include "CustomVAInterface.h"

#include <cutils/properties.h>

#define ST_MAX_FSTAGE_CONF_LEVEL  (100)
#define CUSTOM_CONFIG_OPAQUE_DATA_SIZE 12
#define CONF_LEVELS_INTF_VERSION_0002 0x02

extern "C" int32_t get_vui_interface(struct vui_intf_t *intf,
    vui_intf_param_t *model) {

    int32_t status = 0;
    sound_model_config_t *config = nullptr;

    if (!intf || !model || !model->data)
        return -EINVAL;

    config = (sound_model_config_t *)model->data;
    switch (config->module_type) {
        case ST_MODULE_TYPE_CUSTOM_1:
        case ST_MODULE_TYPE_CUSTOM_2:
            intf->interface = std::make_shared<CustomVAInterface>(model);
            break;
        default:
            ALOGE("%s: %d: Unsupported module type %d",
                __func__, __LINE__, config->module_type);
            status = -EINVAL;
            break;
    }

    return status;
}

extern "C" int32_t release_vui_interface(struct vui_intf_t *intf) {
    int32_t status = 0;

    if (!intf)
        return -EINVAL;

    if (intf->interface) {
        intf->interface = nullptr;
    }

    return status;
}

CustomVAInterface::CustomVAInterface(
    vui_intf_param_t *model) {

    int32_t status = 0;
    struct pal_st_sound_model *sound_model = nullptr;
    std::vector<sound_model_data_t *> model_list;
    sound_model_config_t *config = nullptr;

    conf_levels_intf_version_ = 0;
    st_conf_levels_ = nullptr;
    st_conf_levels_v2_ = nullptr;
    custom_event_ = nullptr;
    custom_event_size_ = 0;
    det_model_id_ = 0;
    std::memset(&detection_event_info_, 0,
                sizeof(struct detection_event_info));
    std::memset(&detection_event_info_multi_model_, 0,
                sizeof(struct detection_event_info_pdk));
    memset(&default_buf_config_, 0, sizeof(default_buf_config_));
    std::memset(&buffering_config_, 0, sizeof(buffering_config_));

    /*
     * Check property vendor.audio.use_qc_wakeup_config
     * false: using opaque data of recognition config as custom config
     * true: using QC wake up config as custom config(for internal verification)
     */
    char value[256] = {0};

#ifndef FEATURE_IPQ_OPENWRT
    property_get("vendor.audio.use_qc_wakeup_config", value, "");
    if (!strcmp("true", value))
        use_qc_wakeup_config_ = true;
#endif

    if (!model || !model->data) {
        ALOGE("%s: %d: Invalid input", __func__, __LINE__);
        throw std::runtime_error("Invalid input");
    }

    config = (sound_model_config_t *)model->data;
    sound_model = (struct pal_st_sound_model *)config->sound_model;
    module_type_ = config->module_type;
    status = CustomVAInterface::ParseSoundModel(sound_model, model_list);
    if (status) {
        ALOGE("%s: %d: Failed to parse sound model, status = %d",
            __func__, __LINE__, status);
        throw std::runtime_error("Failed to parse sound model");
    }

    status = RegisterModel(model->stream, sound_model, model_list);
    if (status) {
        ALOGE("%s: %d: Failed to register sound model, status = %d",
            __func__, __LINE__, status);
        throw std::runtime_error("Failed to register sound model");
    }
}

CustomVAInterface::~CustomVAInterface() {
    ALOGD("%s: %d: Enter", __func__, __LINE__);

    if (custom_event_)
        free(custom_event_);
    if (st_conf_levels_) {
        free(st_conf_levels_);
        st_conf_levels_ = nullptr;
    }
    if (st_conf_levels_v2_) {
        free(st_conf_levels_v2_);
        st_conf_levels_v2_ = nullptr;
    }

    ALOGD("%s: %d: Exit", __func__, __LINE__);
}

void CustomVAInterface::DetachStream(void *stream) {
    DeregisterModel(stream);
}

int32_t CustomVAInterface::SetParameter(
    intf_param_id_t param_id, vui_intf_param_t *param) {

    int32_t status = 0;

    ALOGV("%s: %d: Enter", __func__, __LINE__);

    if (!param) {
        ALOGE("%s: %d: Invalid param", __func__, __LINE__);
        return -EINVAL;
    }

    switch (param_id) {
        case PARAM_RECOGNITION_CONFIG: {
            status = ParseRecognitionConfig(param->stream,
                (struct pal_st_recognition_config *)param->data);
            break;
        }
        case PARAM_DETECTION_EVENT: {
            status = ParseDetectionPayload(param->data, param->size);
            break;
        }
        case PARAM_DETECTION_RESULT: {
            UpdateDetectionResult(param->stream, *(uint32_t *)param->data);
            break;
        }
        case PARAM_LAB_READ_OFFSET: {
            SetReadOffset(*(uint32_t *)param->data);
            break;
        }
        case PARAM_FTRT_DATA: {
            UpdateFTRTData(param->data, param->size);
            break;
        }
        case PARAM_STREAM_ATTRIBUTES: {
            SetStreamAttributes((struct pal_stream_attributes *)param->data);
            break;
        }
        case PARAM_DEFAULT_BUFFER_CONFIG: {
            struct buffer_config *buf_config =
                (struct buffer_config *)param->data;
            default_buf_config_.hist_buffer_duration =
                buf_config->hist_buffer_duration;
            default_buf_config_.pre_roll_duration =
                buf_config->pre_roll_duration;
            break;
        }
        default:
            ALOGE("%s: %d: Unsupported param id %d",
                __func__, __LINE__, param_id);
            break;
    }

    ALOGV("%s: %d: Exit, status = %d", __func__, __LINE__, status);
    return status;
}

int32_t CustomVAInterface::GetParameter(
    intf_param_id_t param_id, vui_intf_param_t *param) {

    int32_t status = 0;

    ALOGV("%s: %d: Enter", __func__, __LINE__);

    if (!param) {
        ALOGE("%s: %d: Invalid param", __func__, __LINE__);
        return -EINVAL;
    }

    switch (param_id) {
        case PARAM_INTERFACE_PROPERTY: {
            vui_intf_property_t *property = (vui_intf_property_t *)param->data;
            if (property) {
                property->is_qc_wakeup_config = false;
                property->is_multi_model_supported = false;
            } else {
                ALOGE("%s: %d: Invalid property", __func__, __LINE__);
                status = -EINVAL;
            }
            break;
        }
        case PARAM_FSTAGE_SOUND_MODEL_TYPE: {
            *(st_module_type_t *)param->data = GetModuleType(param->stream);
            break;
        }
        case PARAM_SOUND_MODEL_LIST: {
            void *s = param->stream;
            sound_model_list_t *sm_list = (sound_model_list_t *)param->data;
            if (sm_info_map_.find(s) != sm_info_map_.end() && sm_info_map_[s]) {
                for (int i = 0; i < sm_info_map_[s]->model_list.size(); i++) {
                    sm_list->sm_list.push_back(sm_info_map_[s]->model_list[i]);
                }
            } else {
                ALOGE("%s: %d: stream not registered", __func__, __LINE__);
                status = -EINVAL;
            }
            break;
        }
        case PARAM_FSTAGE_BUFFERING_CONFIG: {
            struct buffer_config *config = (struct buffer_config *)param->data;
            GetBufferingConfigs(param->stream, config);
            break;
        }
        case PARAM_DETECTION_EVENT: {
            status = GenerateCallbackEvent(param->stream,
                (struct pal_st_recognition_event **)param->data, &param->size);
            break;
        }
        case PARAM_DETECTION_STREAM: {
            param->stream = GetDetectedStream();
            break;
        }
        case PARAM_KEYWORD_INDEX: {
            GetKeywordIndex((struct keyword_index *)param->data);
            break;
        }
        case PARAM_FTRT_DATA_SIZE: {
            *(uint32_t *)param->data = GetFTRTDataSize();
            break;
        }
        case PARAM_LAB_READ_OFFSET: {
            *(uint32_t *)param->data = GetReadOffset();
            break;
        }
        case PARAM_SOUND_MODEL_LOAD:
            status = GetSoundModelLoadPayload(param);
            break;
        case PARAM_CUSTOM_CONFIG:
            status = GetCustomPayload(param);
            break;
        case PARAM_BUFFERING_CONFIG:
            status = GetBufferingPayload(param);
            break;
        default:
            ALOGE("%s: %d: Unsupported param id %d",
                __func__, __LINE__, param_id);
            break;
    }

    ALOGV("%s: %d: Exit, status = %d", __func__, __LINE__, status);

    return status;
}

int32_t CustomVAInterface::Process(intf_process_id_t type,
    vui_intf_param_t *in_out_param) {

    int32_t status = 0;

    ALOGD("%s: %d: Enter", __func__, __LINE__);

    if (!in_out_param) {
        ALOGE("%s: %d: Invalid input for processing", __func__, __LINE__);
        status = -EINVAL;
        goto exit;
    }

    switch (type) {
        case PROCESS_LAB_DATA:
            ProcessLab(in_out_param->data, in_out_param->size);
            break;
        default:
            ALOGE("%s: %d: Unsupported process type %d",
                __func__, __LINE__, type);
            break;
    }

exit:
    ALOGD("%s: %d: Exit, status = %d", __func__, __LINE__, status);
    return status;
}

int32_t CustomVAInterface::ParseSoundModel(
    struct pal_st_sound_model *sound_model,
    std::vector<sound_model_data_t *> &model_list) {

    int32_t status = 0;
    int32_t i = 0;
    struct pal_st_phrase_sound_model *phrase_sm = nullptr;
    struct pal_st_sound_model *common_sm = nullptr;
    uint8_t *ptr = nullptr;
    uint8_t *sm_payload = nullptr;
    uint8_t *sm_data = nullptr;
    int32_t sm_size = 0;
    SML_GlobalHeaderType *global_hdr = nullptr;
    SML_HeaderTypeV3 *hdr_v3 = nullptr;
    SML_BigSoundModelTypeV3 *big_sm = nullptr;
    uint32_t offset = 0;
    sound_model_data_t *model_data = nullptr;

    ALOGD("%s: %d: Enter", __func__, __LINE__);

    if (sound_model->type == PAL_SOUND_MODEL_TYPE_KEYPHRASE) {
        phrase_sm = (struct pal_st_phrase_sound_model *)sound_model;
        sm_payload = (uint8_t *)phrase_sm + phrase_sm->common.data_offset;
        global_hdr = (SML_GlobalHeaderType *)sm_payload;
        if (global_hdr->magicNumber == SML_GLOBAL_HEADER_MAGIC_NUMBER) {
            hdr_v3 = (SML_HeaderTypeV3 *)(sm_payload +
                                          sizeof(SML_GlobalHeaderType));
            ALOGI("%s: %d: num of sound models = %u",
                __func__, __LINE__, hdr_v3->numModels);
            for (i = 0; i < hdr_v3->numModels; i++) {
                big_sm = (SML_BigSoundModelTypeV3 *)(
                    sm_payload + sizeof(SML_GlobalHeaderType) +
                    sizeof(SML_HeaderTypeV3) +
                    (i * sizeof(SML_BigSoundModelTypeV3)));

                ALOGI("%s: %d: type = %u, size = %u, version = %u.%u",
                    __func__, __LINE__, big_sm->type, big_sm->size,
                    big_sm->versionMajor, big_sm->versionMinor);
                if (big_sm->type == ST_SM_ID_SVA_F_STAGE_GMM) {
                    sm_size = big_sm->size;
                    sm_data = (uint8_t *)calloc(1, sm_size);
                    if (!sm_data) {
                        status = -ENOMEM;
                        ALOGE("%s: %d: sm_data allocation failed, status %d",
                            __func__, __LINE__, status);
                        goto error_exit;
                    }
                    ptr = (uint8_t *)sm_payload +
                        sizeof(SML_GlobalHeaderType) +
                        sizeof(SML_HeaderTypeV3) +
                        (hdr_v3->numModels * sizeof(SML_BigSoundModelTypeV3)) +
                        big_sm->offset;
                    ar_mem_cpy(sm_data, sm_size, ptr, sm_size);

                    model_data = (sound_model_data_t *)calloc(1, sizeof(sound_model_data_t));
                    if (!model_data) {
                        status = -ENOMEM;
                        ALOGE("%s: %d: model_data allocation failed, status %d",
                            __func__, __LINE__, status);
                        goto error_exit;
                    }
                    model_data->type = big_sm->type;
                    model_data->data = sm_data;
                    model_data->size = sm_size;
                    model_list.push_back(model_data);
                } else if (big_sm->type != SML_ID_SVA_S_STAGE_UBM) {
                    if (big_sm->type == SML_ID_SVA_F_STAGE_INTERNAL ||
                        (big_sm->type == ST_SM_ID_SVA_S_STAGE_USER &&
                         !(phrase_sm->phrases[0].recognition_mode &
                         PAL_RECOGNITION_MODE_USER_IDENTIFICATION)))
                        continue;
                    sm_size = big_sm->size;
                    ptr = (uint8_t *)sm_payload +
                        sizeof(SML_GlobalHeaderType) +
                        sizeof(SML_HeaderTypeV3) +
                        (hdr_v3->numModels * sizeof(SML_BigSoundModelTypeV3)) +
                        big_sm->offset;
                    sm_data = (uint8_t *)calloc(1, sm_size);
                    if (!sm_data) {
                        status = -ENOMEM;
                        ALOGE("%s: %d: Failed to alloc memory for sm_data",
                            __func__, __LINE__);
                        goto error_exit;
                    }
                    ar_mem_cpy(sm_data, sm_size, ptr, sm_size);

                    model_data = (sound_model_data_t *)calloc(1, sizeof(sound_model_data_t));
                    if (!model_data) {
                        status = -ENOMEM;
                        ALOGE("%s: %d: model_data allocation failed, status %d",
                            __func__, __LINE__, status);
                        goto error_exit;
                    }
                    model_data->type = big_sm->type;
                    model_data->data = sm_data;
                    model_data->size = sm_size;
                    model_list.push_back(model_data);
                }
            }
        } else {
            // Parse sound model 2.0
            sm_size = phrase_sm->common.data_size;
            sm_data = (uint8_t *)calloc(1, sm_size);
            if (!sm_data) {
                ALOGE("%s: %d: Failed to allocate memory for sm_data",
                    __func__, __LINE__);
                status = -ENOMEM;
                goto error_exit;
            }
            ptr = (uint8_t*)phrase_sm + phrase_sm->common.data_offset;
            ar_mem_cpy(sm_data, sm_size, ptr, sm_size);

            model_data = (sound_model_data_t *)calloc(1, sizeof(sound_model_data_t));
            if (!model_data) {
                status = -ENOMEM;
                ALOGE("%s: %d: model_data allocation failed, status %d",
                    __func__, __LINE__, status);
                goto error_exit;
            }
            model_data->type = ST_SM_ID_SVA_F_STAGE_GMM;
            model_data->data = sm_data;
            model_data->size = sm_size;
            model_list.push_back(model_data);
        }
    } else {
        // handle for generic sound model
        common_sm = sound_model;
        sm_size = common_sm->data_size;
        sm_data = (uint8_t *)calloc(1, sm_size);
        if (!sm_data) {
            ALOGE("%s: %d: Failed to allocate memory for sm_data",
                __func__, __LINE__);
            status = -ENOMEM;
            goto error_exit;
        }
        ptr = (uint8_t*)common_sm + common_sm->data_offset;
        ar_mem_cpy(sm_data, sm_size, ptr, sm_size);

        model_data = (sound_model_data_t *)calloc(1, sizeof(sound_model_data_t));
        if (!model_data) {
            status = -ENOMEM;
            ALOGE("%s: %d: model_data allocation failed, status %d",
                __func__, __LINE__, status);
            goto error_exit;
        }
        model_data->type = ST_SM_ID_SVA_F_STAGE_GMM;
        model_data->data = sm_data;
        model_data->size = sm_size;
        model_list.push_back(model_data);
    }
    ALOGD("%s: %d: Exit, status %d", __func__, __LINE__, status);
    return status;

error_exit:
    // clean up memory added to model_list in failure case
    for (int i = 0; i < model_list.size(); i++) {
        model_data = model_list[i];
        if (model_data) {
            if (model_data->data)
                free(model_data->data);
            free(model_data);
        }
    }
    model_list.clear();
    if (sm_data)
        free(sm_data);

    ALOGD("%s: %d: Exit, status %d", __func__, __LINE__, status);
    return status;
}

int32_t CustomVAInterface::ParseRecognitionConfig(void *s,
    struct pal_st_recognition_config *config) {

    int32_t status = 0;
    struct sound_model_info *sm_info = nullptr;
    struct st_param_header *param_hdr = NULL;
    struct st_hist_buffer_info *hist_buf = NULL;
    struct st_det_perf_mode_info *det_perf_mode = NULL;
    uint8_t *opaque_ptr = NULL;
    unsigned int opaque_size = 0, conf_levels_payload_size = 0;
    uint32_t hist_buffer_duration = 0;
    uint32_t pre_roll_duration = 0;
    uint8_t *conf_levels = NULL;
    uint32_t num_conf_levels = 0;
    uint32_t recognition_mode = 0;
    size_t fixed_wakeup_payload_size = 0;
    size_t wakeup_payload_size = 0;
    uint8_t *wakeup_payload = nullptr;
    uint8_t *confidence_level = nullptr;
    uint8_t *kw_user_enable = nullptr;
    struct detection_engine_config_voice_wakeup wakeup_config;

    memset(&wakeup_config, 0, sizeof(struct detection_engine_config_voice_wakeup));

    if (sm_info_map_.find(s) != sm_info_map_.end() && sm_info_map_[s]) {
        sm_info = sm_info_map_[s];
        sm_info->rec_config = config;
        recognition_mode = sm_info->recognition_mode;
    } else {
        ALOGE("%s: %d: Stream not registered to interface", __func__, __LINE__);
        return -EINVAL;
    }

    ALOGD("%s: %d: Enter", __func__, __LINE__);
    if (!config) {
        ALOGE("%s: %d: Invalid config", __func__, __LINE__);
        return -EINVAL;
    }

    // Parse recognition config
    if (config->data_size > CUSTOM_CONFIG_OPAQUE_DATA_SIZE &&
        use_qc_wakeup_config_) {
        opaque_ptr = (uint8_t *)config + config->data_offset;
        while (opaque_size < config->data_size) {
            param_hdr = (struct st_param_header *)opaque_ptr;
            ALOGV("%s: %d: key %d, payload size %d", __func__, __LINE__,
                param_hdr->key_id, param_hdr->payload_size);

            switch (param_hdr->key_id) {
                case ST_PARAM_KEY_CONFIDENCE_LEVELS:
                    conf_levels_intf_version_ = *(uint32_t *)(
                        opaque_ptr + sizeof(struct st_param_header));
                    ALOGV("%s: %d: conf_levels_intf_version = %u",
                        __func__, __LINE__, conf_levels_intf_version_);
                    if (conf_levels_intf_version_ !=
                        CONF_LEVELS_INTF_VERSION_0002) {
                        conf_levels_payload_size =
                            sizeof(struct st_confidence_levels_info);
                    } else {
                        conf_levels_payload_size =
                            sizeof(struct st_confidence_levels_info_v2);
                    }
                    if (param_hdr->payload_size != conf_levels_payload_size) {
                        ALOGE("%s: %d: Conf level format error, exiting",
                            __func__, __LINE__);
                        status = -EINVAL;
                        goto error_exit;
                    }
                    status = ParseOpaqueConfLevels(sm_info, opaque_ptr,
                                                   conf_levels_intf_version_,
                                                   &conf_levels,
                                                   &num_conf_levels);
                    if (status) {
                        ALOGE("%s: %d: Failed to parse opaque conf levels",
                            __func__, __LINE__);
                        goto error_exit;
                    }

                    opaque_size += sizeof(struct st_param_header) +
                        conf_levels_payload_size;
                    opaque_ptr += sizeof(struct st_param_header) +
                        conf_levels_payload_size;
                    if (status) {
                        ALOGE("%s: %d: Parse conf levels failed(status=%d)",
                            __func__, __LINE__, status);
                        status = -EINVAL;
                        goto error_exit;
                    }
                    break;
                case ST_PARAM_KEY_HISTORY_BUFFER_CONFIG:
                    if (param_hdr->payload_size !=
                        sizeof(struct st_hist_buffer_info)) {
                        ALOGE("%s: %d: History buffer config format error",
                            __func__, __LINE__);
                        status = -EINVAL;
                        goto error_exit;
                    }
                    hist_buf = (struct st_hist_buffer_info *)(opaque_ptr +
                        sizeof(struct st_param_header));
                    hist_buffer_duration = hist_buf->hist_buffer_duration_msec;
                    pre_roll_duration = hist_buf->pre_roll_duration_msec;

                    opaque_size += sizeof(struct st_param_header) +
                        sizeof(struct st_hist_buffer_info);
                    opaque_ptr += sizeof(struct st_param_header) +
                        sizeof(struct st_hist_buffer_info);
                    break;
                case ST_PARAM_KEY_DETECTION_PERF_MODE:
                    if (param_hdr->payload_size !=
                        sizeof(struct st_det_perf_mode_info)) {
                        ALOGE("%s: %d: Opaque data format error, exiting",
                            __func__, __LINE__);
                        status = -EINVAL;
                        goto error_exit;
                    }
                    det_perf_mode = (struct st_det_perf_mode_info *)
                        (opaque_ptr + sizeof(struct st_param_header));
                    ALOGD("%s: %d: set perf mode %d",
                        __func__, __LINE__, det_perf_mode->mode);
                    opaque_size += sizeof(struct st_param_header) +
                        sizeof(struct st_det_perf_mode_info);
                    opaque_ptr += sizeof(struct st_param_header) +
                        sizeof(struct st_det_perf_mode_info);
                    break;
                default:
                    ALOGE("%s: %d: Unsupported opaque data key id, exiting",
                        __func__, __LINE__);
                    status = -EINVAL;
                    goto error_exit;
            }
        }
    } else {
        // get history buffer duration from sound trigger platform xml
        hist_buffer_duration = default_buf_config_.hist_buffer_duration;
        pre_roll_duration = default_buf_config_.pre_roll_duration;

        if (use_qc_wakeup_config_) {
            status = FillConfLevels(sm_info, config, &conf_levels, &num_conf_levels);
            if (status) {
                ALOGE("%s: %d: Failed to parse conf levels from rc config",
                    __func__, __LINE__);
                goto error_exit;
            }
        }
    }

    sm_info_map_[s]->buf_config.hist_buffer_duration = hist_buffer_duration;
    sm_info_map_[s]->buf_config.pre_roll_duration = pre_roll_duration;

    if (use_qc_wakeup_config_) {
        // construct custom config
        wakeup_config.mode |= recognition_mode;
        wakeup_config.custom_payload_size = config->data_size;
        wakeup_config.num_active_models = num_conf_levels;
        wakeup_config.reserved = 0;
        for (int i = 0; i < wakeup_config.num_active_models; i++) {
            wakeup_config.confidence_levels[i] = conf_levels[i];
            wakeup_config.keyword_user_enables[i] =
                (wakeup_config.confidence_levels[i] == 100) ? 0 : 1;
            ALOGI("%s: %d: cf levels[%d] = %d", __func__, __LINE__, i,
                wakeup_config.confidence_levels[i]);
        }

        fixed_wakeup_payload_size =
            sizeof(struct detection_engine_config_voice_wakeup) -
                PAL_SOUND_TRIGGER_MAX_USERS * 2;
        wakeup_payload_size = fixed_wakeup_payload_size +
            wakeup_config.num_active_models * 2;
        wakeup_payload = (uint8_t *)calloc(1, wakeup_payload_size);
        if (!wakeup_payload) {
            ALOGE("%s: %d: Failed to allocate memory for wakeup payload",
                __func__, __LINE__);
            status = -ENOMEM;
            goto error_exit;
        }

        ar_mem_cpy(wakeup_payload, fixed_wakeup_payload_size,
            &wakeup_config, fixed_wakeup_payload_size);
        confidence_level = wakeup_payload +
            fixed_wakeup_payload_size;
        kw_user_enable = wakeup_payload +
            fixed_wakeup_payload_size +
            wakeup_config.num_active_models;
        for (int i = 0; i < wakeup_config.num_active_models; i++) {
            confidence_level[i] = wakeup_config.confidence_levels[i];
            kw_user_enable[i] = wakeup_config.keyword_user_enables[i];
            ALOGI("%s: %d: confidence_level[%d] = %d KW_User_enable[%d] = %d",
                __func__, __LINE__,
                i, confidence_level[i], i, kw_user_enable[i]);
        }

        if (sm_info_map_[s]->wakeup_config)
            free(sm_info_map_[s]->wakeup_config);
        sm_info_map_[s]->wakeup_config = wakeup_payload;
        sm_info_map_[s]->wakeup_config_size = wakeup_payload_size;

        if (conf_levels) {
            free(conf_levels);
        }
    } else {
        num_conf_levels = config->data_size;
        conf_levels = (uint8_t *)calloc(1, num_conf_levels);
        if (!conf_levels) {
            ALOGE("%s: %d: Failed to allocate memory for 3rd party config",
                __func__, __LINE__);
            status = -ENOMEM;
            goto error_exit;
        }

        ar_mem_cpy(conf_levels, num_conf_levels,
            (uint8_t *)config + config->data_offset, num_conf_levels);
        if (sm_info_map_[s]->wakeup_config)
            free(sm_info_map_[s]->wakeup_config);
        sm_info_map_[s]->wakeup_config = conf_levels;
        sm_info_map_[s]->wakeup_config_size = num_conf_levels;
    }
    goto exit;

error_exit:
    if (st_conf_levels_) {
        free(st_conf_levels_);
        st_conf_levels_ = nullptr;
    }
    if (st_conf_levels_v2_) {
        free(st_conf_levels_v2_);
        st_conf_levels_v2_ = nullptr;
    }

exit:
    ALOGD("%s: %d: Exit, status %d", __func__, __LINE__, status);
    return status;
}

void CustomVAInterface::GetBufferingConfigs(void *s,
    struct buffer_config *config) {

    if (sm_info_map_.find(s) != sm_info_map_.end() && sm_info_map_[s]) {
        config->hist_buffer_duration = sm_info_map_[s]->buf_config.hist_buffer_duration;
        config->pre_roll_duration = sm_info_map_[s]->buf_config.pre_roll_duration;
    } else {
        ALOGE("%s: %d: Stream not registered to interface", __func__, __LINE__);
    }
}

void CustomVAInterface::GetSecondStageConfLevels(void *s,
    listen_model_indicator_enum type, uint32_t *level) {

    if (sm_info_map_.find(s) != sm_info_map_.end() && sm_info_map_[s]) {
        for (auto iter = sm_info_map_[s]->sec_threshold.begin();
            iter != sm_info_map_[s]->sec_threshold.end(); iter++) {
            if ((*iter).first == type)
                *level = (*iter).second;
        }
    } else {
        ALOGE("%s: %d: Stream not registered to interface", __func__, __LINE__);
    }
}

void CustomVAInterface::SetSecondStageDetLevels(void *s,
    listen_model_indicator_enum type, uint32_t level) {

    bool sec_det_level_exist = false;

    if (sm_info_map_.find(s) != sm_info_map_.end() && sm_info_map_[s]) {
        for (auto &iter: sm_info_map_[s]->sec_det_level) {
            if (iter.first == type) {
                iter.second = level;
                sec_det_level_exist = true;
                break;
            }
        }
        if (!sec_det_level_exist)
            sm_info_map_[s]->sec_det_level.push_back(std::make_pair(type, level));
    } else {
        ALOGE("%s: %d: Stream not registered to interface", __func__, __LINE__);
    }
}

int32_t CustomVAInterface::ParseDetectionPayload(void *event, uint32_t size) {
    int32_t status = 0;

    if (use_qc_wakeup_config_) {
        if (!IS_MODULE_TYPE_PDK(module_type_)) {
            status = ParseDetectionPayloadGMM(event);
            CheckAndSetDetectionConfLevels(GetDetectedStream());
        } else {
            status = ParseDetectionPayloadPDK(event);
        }
    } else {
        custom_event_ = (uint8_t *)realloc(custom_event_, size);
        if (!custom_event_) {
            ALOGE("%s: %d: Failed to allocate memory for detection payload",
                __func__, __LINE__);
            return -ENOMEM;
        }

        ar_mem_cpy(custom_event_, size, event, size);
        custom_event_size_ = size;
    }

    if (status) {
        ALOGE("%s: %d: Failed to parse detection payload, status %d",
            __func__, __LINE__, status);
    }

    return status;
}

void* CustomVAInterface::GetDetectedStream() {
    void *st = nullptr;
    struct sound_model_info *sm_info = nullptr;

    ALOGD("%s: %d: Enter", __func__, __LINE__);
    if (sm_info_map_.empty()) {
        ALOGE("%s: %d: Unexpected, No streams attached to engine!",
            __func__, __LINE__);
        return nullptr;
    }
    /*
     * If only single stream exists, this detection is not for merged/multi
     * sound model, hence return this as only available stream
     */
    if (!IS_MODULE_TYPE_PDK(module_type_)) {
        if (sm_info_map_.size() == 1) {
            return sm_info_map_.begin()->first;
        }

        if (detection_event_info_.num_confidence_levels <
                sound_model_info_->GetNumKeyPhrases()) {
            ALOGE("%s: %d: detection event conf levels %d < num of keyphrases %d",
                __func__, __LINE__, detection_event_info_.num_confidence_levels,
                sound_model_info_->GetNumKeyPhrases());
            return nullptr;
        }

        /*
         * The DSP payload contains the keyword conf levels from the beginning.
         * Only one keyword conf level is expected to be non-zero from keyword
         * detection. Find non-zero conf level up to number of keyphrases and
         * if one is found, match it to the corresponding keyphrase from list
         * of streams to obtain the detected stream.
         */
        for (uint32_t i = 0; i < sound_model_info_->GetNumKeyPhrases(); i++) {
            if (!detection_event_info_.confidence_levels[i])
                continue;
            for (auto &iter: sm_info_map_) {
                for (uint32_t k = 0; k < iter.second->info->GetNumKeyPhrases(); k++) {
                    if (!strcmp(sound_model_info_->GetKeyPhrases()[i],
                                iter.second->info->GetKeyPhrases()[k])) {
                        return iter.first;
                    }
                }
            }
        }
    } else {
        for (auto &iter : sm_info_map_) {
            sm_info = iter.second;
            if (sm_info->model_id == det_model_id_) {
                st = iter.first;
                break;
            }
        }
        if (!st) {
            ALOGE("%s: %d: Invalid model id = %x",
                __func__, __LINE__, det_model_id_);
        }
        return st;
    }
    return nullptr;
}

void* CustomVAInterface::GetDetectionEventInfo() {
    if (IS_MODULE_TYPE_PDK(module_type_)) {
       return &detection_event_info_multi_model_;
    }
    return &detection_event_info_;
}

int32_t CustomVAInterface::GenerateCallbackEvent(void *s,
    struct pal_st_recognition_event **event, uint32_t *size) {

    struct sound_model_info *sm_info = nullptr;
    struct pal_st_phrase_recognition_event *phrase_event = nullptr;
    struct pal_st_generic_recognition_event *generic_event = nullptr;
    struct st_param_header *param_hdr = nullptr;
    struct st_keyword_indices_info *kw_indices = nullptr;
    struct st_timestamp_info *timestamps = nullptr;
    struct model_stats *det_model_stat = nullptr;
    struct detection_event_info_pdk *det_ev_info_pdk = nullptr;
    struct detection_event_info *det_ev_info = nullptr;
    size_t opaque_size = 0;
    size_t event_size = 0, conf_levels_size = 0;
    uint8_t *opaque_data = nullptr;
    uint8_t *custom_event = nullptr;
    uint32_t det_keyword_id = 0;
    uint32_t best_conf_level = 0;
    uint32_t detection_timestamp_lsw = 0;
    uint32_t detection_timestamp_msw = 0;
    int32_t status = 0;
    int32_t num_models = 0;

    if (sm_info_map_.find(s) != sm_info_map_.end() && sm_info_map_[s]) {
        sm_info = sm_info_map_[s];
    } else {
        ALOGE("%s: %d: Stream not registered to interface",
            __func__, __LINE__);
        return -EINVAL;
    }

    ALOGD("%s: %d: Enter", __func__, __LINE__);
    *event = nullptr;
    if (sm_info->type == PAL_SOUND_MODEL_TYPE_KEYPHRASE) {
        if (sm_info->model_id > 0) {
            det_ev_info_pdk = &detection_event_info_multi_model_;
            if (!det_ev_info_pdk) {
                ALOGE("%s: %d: detection info multi model not available",
                    __func__, __LINE__);
                status = -EINVAL;
                goto exit;
            }
        } else {
            det_ev_info = &detection_event_info_;
            if (!det_ev_info) {
                ALOGE("%s: %d: detection info not available",
                    __func__, __LINE__);
                status = -EINVAL;
                goto exit;
            }
        }

        if (conf_levels_intf_version_ != CONF_LEVELS_INTF_VERSION_0002)
            conf_levels_size = sizeof(struct st_confidence_levels_info);
        else
            conf_levels_size = sizeof(struct st_confidence_levels_info_v2);

        opaque_size = (3 * sizeof(struct st_param_header)) +
            sizeof(struct st_timestamp_info) +
            sizeof(struct st_keyword_indices_info) +
            conf_levels_size;

        event_size = sizeof(struct pal_st_phrase_recognition_event) +
                     opaque_size;
        phrase_event = (struct pal_st_phrase_recognition_event *)
                       calloc(1, event_size);
        if (!phrase_event) {
            ALOGE("%s: %d: Failed to alloc memory for recognition event",
                __func__, __LINE__);
            status =  -ENOMEM;
            goto exit;
        }

        phrase_event->num_phrases = sm_info->rec_config->num_phrases;
        memcpy(phrase_event->phrase_extras, sm_info->rec_config->phrases,
               phrase_event->num_phrases *
               sizeof(struct pal_st_phrase_recognition_extra));

        *event = &(phrase_event->common);
        (*event)->status = sm_info->det_result;
        (*event)->type = sm_info->type;
        (*event)->st_handle = (pal_st_handle_t *)this;
        (*event)->capture_available = sm_info->rec_config->capture_requested;
        // TODO: generate capture session
        (*event)->capture_session = 0;
        (*event)->capture_delay_ms = 0;
        (*event)->capture_preamble_ms = 0;
        (*event)->trigger_in_data = true;
        (*event)->data_size = opaque_size;
        (*event)->data_offset = sizeof(struct pal_st_phrase_recognition_event);
        (*event)->media_config.sample_rate =
            str_attr_.in_media_config.sample_rate;
        (*event)->media_config.bit_width =
            str_attr_.in_media_config.bit_width;
        (*event)->media_config.ch_info.channels =
            str_attr_.in_media_config.ch_info.channels;
        (*event)->media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
        // Filling Opaque data
        opaque_data = (uint8_t *)phrase_event +
                       phrase_event->common.data_offset;

        /* Pack the opaque data confidence levels structure */
        param_hdr = (struct st_param_header *)opaque_data;
        param_hdr->key_id = ST_PARAM_KEY_CONFIDENCE_LEVELS;
        if (conf_levels_intf_version_ !=  CONF_LEVELS_INTF_VERSION_0002)
            param_hdr->payload_size = sizeof(struct st_confidence_levels_info);
        else
            param_hdr->payload_size = sizeof(struct st_confidence_levels_info_v2);
        opaque_data += sizeof(struct st_param_header);
        /* Copy the cached conf levels from recognition config */
        if (conf_levels_intf_version_ != CONF_LEVELS_INTF_VERSION_0002)
            ar_mem_cpy(opaque_data, param_hdr->payload_size,
                    st_conf_levels_, param_hdr->payload_size);
        else
            ar_mem_cpy(opaque_data, param_hdr->payload_size,
                st_conf_levels_v2_, param_hdr->payload_size);
        if (sm_info->model_id > 0) {
            num_models = det_ev_info_pdk->num_detected_models;
            for (int i = 0; i < num_models; ++i) {
                det_model_stat = &det_ev_info_pdk->detected_model_stats[i];
                if (sm_info->model_id == det_model_stat->detected_model_id) {
                    det_keyword_id = det_model_stat->detected_keyword_id;
                    best_conf_level = det_model_stat->best_confidence_level;
                    detection_timestamp_lsw =
                        det_model_stat->detection_timestamp_lsw;
                    detection_timestamp_msw =
                        det_model_stat->detection_timestamp_msw;
                    ALOGI("%s: %d: keywordID: %u, best_conf_level: %u",
                        __func__, __LINE__, det_keyword_id, best_conf_level);
                    break;
                }
            }
            FillCallbackConfLevels(sm_info, opaque_data, det_keyword_id, best_conf_level);
        } else {
            PackEventConfLevels(sm_info, opaque_data);
        }
        opaque_data += param_hdr->payload_size;

        /* Pack the opaque data keyword indices structure */
        param_hdr = (struct st_param_header *)opaque_data;
        param_hdr->key_id = ST_PARAM_KEY_KEYWORD_INDICES;
        param_hdr->payload_size = sizeof(struct st_keyword_indices_info);
        opaque_data += sizeof(struct st_param_header);
        kw_indices = (struct st_keyword_indices_info *)opaque_data;
        kw_indices->version = 0x1;

        kw_indices->start_index = start_index_;
        kw_indices->end_index = end_index_;
        opaque_data += sizeof(struct st_keyword_indices_info);

        /*
         * Pack the opaque data detection time structure
         * TODO: add support for 2nd stage detection timestamp
         */
        param_hdr = (struct st_param_header *)opaque_data;
        param_hdr->key_id = ST_PARAM_KEY_TIMESTAMP;
        param_hdr->payload_size = sizeof(struct st_timestamp_info);
        opaque_data += sizeof(struct st_param_header);
        timestamps = (struct st_timestamp_info *)opaque_data;
        timestamps->version = 0x1;
        if (sm_info->model_id > 0) {
            timestamps->first_stage_det_event_time = 1000 *
                        ((uint64_t)detection_timestamp_lsw +
                        ((uint64_t)detection_timestamp_msw<<32));
        } else {
            timestamps->first_stage_det_event_time = 1000 *
                ((uint64_t)det_ev_info->detection_timestamp_lsw +
                ((uint64_t)det_ev_info->detection_timestamp_msw << 32));
        }
    } else if (sm_info->type == PAL_SOUND_MODEL_TYPE_GENERIC) {
        event_size = sizeof(struct pal_st_generic_recognition_event) +
                     custom_event_size_;
        generic_event = (struct pal_st_generic_recognition_event *)
                       calloc(1, event_size);
        if (!generic_event) {
            ALOGE("%s: %d: Failed to alloc memory for recognition event",
                __func__, __LINE__);
            status =  -ENOMEM;
            goto exit;
        }

        *event = &(generic_event->common);
        (*event)->status = PAL_RECOGNITION_STATUS_SUCCESS;
        (*event)->type = sm_info->type;
        (*event)->st_handle = (pal_st_handle_t *)this;
        (*event)->capture_available = sm_info->rec_config->capture_requested;
        // TODO: generate capture session
        (*event)->capture_session = 0;
        (*event)->capture_delay_ms = 0;
        (*event)->capture_preamble_ms = 0;
        (*event)->trigger_in_data = true;
        (*event)->data_size = 0;
        (*event)->data_offset = sizeof(struct pal_st_generic_recognition_event);
        (*event)->media_config.sample_rate =
            str_attr_.in_media_config.sample_rate;
        (*event)->media_config.bit_width =
            str_attr_.in_media_config.bit_width;
        (*event)->media_config.ch_info.channels =
            str_attr_.in_media_config.ch_info.channels;
        (*event)->media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
        // Filling Opaque data
        opaque_data = (uint8_t *)generic_event +
                       generic_event->common.data_offset;
        ar_mem_cpy(opaque_data, custom_event_size_, custom_event_, custom_event_size_);
    }
    *size = event_size;
exit:
    ALOGD("%s: %d: Exit", __func__, __LINE__);
    return status;
}

// Protected APIs
int32_t CustomVAInterface::ParseOpaqueConfLevels(
    struct sound_model_info *info,
    void *opaque_conf_levels,
    uint32_t version,
    uint8_t **out_conf_levels,
    uint32_t *out_num_conf_levels) {

    int32_t status = 0;
    struct st_confidence_levels_info *conf_levels = nullptr;
    struct st_confidence_levels_info_v2 *conf_levels_v2 = nullptr;
    struct st_sound_model_conf_levels *sm_levels = nullptr;
    struct st_sound_model_conf_levels_v2 *sm_levels_v2 = nullptr;
    int32_t confidence_level = 0;
    int32_t confidence_level_v2 = 0;
    bool gmm_conf_found = false;

    ALOGD("%s: %d: Enter", __func__, __LINE__);
    if (version != CONF_LEVELS_INTF_VERSION_0002) {
        conf_levels = (struct st_confidence_levels_info *)
            ((char *)opaque_conf_levels + sizeof(struct st_param_header));

        st_conf_levels_ = (struct st_confidence_levels_info *)realloc(st_conf_levels_,
                sizeof(struct st_confidence_levels_info));
        if (!st_conf_levels_) {
            ALOGE("%s: %d: failed to alloc stream conf_levels_",
                __func__, __LINE__);
            status = -ENOMEM;
            goto exit;
        }

        /* Cache to use during detection event processing */
        ar_mem_cpy((uint8_t *)st_conf_levels_, sizeof(struct st_confidence_levels_info),
            (uint8_t *)conf_levels, sizeof(struct st_confidence_levels_info));

        for (int i = 0; i < conf_levels->num_sound_models; i++) {
            sm_levels = &conf_levels->conf_levels[i];
            if (sm_levels->sm_id == ST_SM_ID_SVA_F_STAGE_GMM) {
                gmm_conf_found = true;
                status = FillOpaqueConfLevels(info->model_id, (void *)sm_levels,
                    out_conf_levels, out_num_conf_levels, version);
            } else if (sm_levels->sm_id & ST_SM_ID_SVA_S_STAGE_KWD ||
                       sm_levels->sm_id & ST_SM_ID_SVA_S_STAGE_USER) {
                confidence_level =
                    (sm_levels->sm_id & ST_SM_ID_SVA_S_STAGE_KWD) ?
                    sm_levels->kw_levels[0].kw_level:
                    sm_levels->kw_levels[0].user_levels[0].level;
                if (sm_levels->sm_id & ST_SM_ID_SVA_S_STAGE_KWD) {
                    ALOGI("%s: %d: second stage keyword confidence level = %d",
                        __func__, __LINE__, confidence_level);
                } else {
                    ALOGI("%s: %d: second stage user confidence level = %d",
                        __func__, __LINE__, confidence_level);
                }
                info->sec_threshold.push_back(
                    std::make_pair(sm_levels->sm_id, confidence_level));
            }
        }
    } else {
        conf_levels_v2 = (struct st_confidence_levels_info_v2 *)
            ((char *)opaque_conf_levels + sizeof(struct st_param_header));

        st_conf_levels_v2_ = (struct st_confidence_levels_info_v2 *)realloc(st_conf_levels_v2_,
            sizeof(struct st_confidence_levels_info_v2));
        if (!st_conf_levels_v2_) {
            ALOGE("%s: %d: failed to alloc stream conf_levels_",
                __func__, __LINE__);
            status = -ENOMEM;
            goto exit;
        }

        /* Cache to use during detection event processing */
        ar_mem_cpy((uint8_t *)st_conf_levels_v2_, sizeof(struct st_confidence_levels_info_v2),
            (uint8_t *)conf_levels_v2, sizeof(struct st_confidence_levels_info_v2));

        for (int i = 0; i < conf_levels_v2->num_sound_models; i++) {
            sm_levels_v2 = &conf_levels_v2->conf_levels[i];
            if (sm_levels_v2->sm_id == ST_SM_ID_SVA_F_STAGE_GMM) {
                gmm_conf_found = true;
                status = FillOpaqueConfLevels(info->model_id, (void *)sm_levels_v2,
                    out_conf_levels, out_num_conf_levels, version);
            } else if (sm_levels_v2->sm_id & ST_SM_ID_SVA_S_STAGE_KWD ||
                       sm_levels_v2->sm_id & ST_SM_ID_SVA_S_STAGE_USER) {
                confidence_level_v2 =
                    (sm_levels_v2->sm_id & ST_SM_ID_SVA_S_STAGE_KWD) ?
                    sm_levels_v2->kw_levels[0].kw_level:
                    sm_levels_v2->kw_levels[0].user_levels[0].level;
                if (sm_levels_v2->sm_id & ST_SM_ID_SVA_S_STAGE_KWD) {
                    ALOGI("%s: %d: second stage keyword confidence level = %d",
                        __func__, __LINE__, confidence_level_v2);
                } else {
                    ALOGI("%s: %d: second stage user confidence level = %d",
                        __func__, __LINE__, confidence_level_v2);
                }
                info->sec_threshold.push_back(
                    std::make_pair(sm_levels_v2->sm_id, confidence_level_v2));
            }
        }
    }

    if (!gmm_conf_found || status) {
        ALOGE("%s: %d: Did not receive GMM confidence threshold, error!",
            __func__, __LINE__);
        status = -EINVAL;
    }

exit:
    ALOGD("%s: %d: Exit", __func__, __LINE__);

    return status;
}

int32_t CustomVAInterface::FillConfLevels(
    struct sound_model_info *info,
    struct pal_st_recognition_config *config,
    uint8_t **out_conf_levels,
    uint32_t *out_num_conf_levels) {

    int32_t status = 0;
    uint32_t num_conf_levels = 0;
    unsigned int user_level, user_id;
    unsigned int i = 0, j = 0;
    uint8_t *conf_levels = nullptr;
    unsigned char *user_id_tracker = nullptr;
    struct pal_st_phrase_sound_model *phrase_sm = nullptr;

    ALOGD("%s: %d: Enter", __func__, __LINE__);

    if (!config) {
        status = -EINVAL;
        ALOGE("%s: %d: invalid input status %d", __func__, __LINE__, status);
        goto exit;
    }

    phrase_sm = (struct pal_st_phrase_sound_model *)info->model;

    if ((config->num_phrases == 0) ||
        (phrase_sm && config->num_phrases > phrase_sm->num_phrases)) {
        status = -EINVAL;
        ALOGE("%s: %d: Invalid phrase data status %d",
            __func__, __LINE__, status);
        goto exit;
    }

    for (i = 0; i < config->num_phrases; i++) {
        num_conf_levels++;
        if (info->model_id == 0) {
            for (j = 0; j < config->phrases[i].num_levels; j++)
                num_conf_levels++;
        }
    }

    conf_levels = (unsigned char*)calloc(1, num_conf_levels);
    if (!conf_levels) {
        status = -ENOMEM;
        ALOGE("%s: %d: conf_levels calloc failed, status %d",
            __func__, __LINE__, status);
        goto exit;
    }

    user_id_tracker = (unsigned char *)calloc(1, num_conf_levels);
    if (!user_id_tracker) {
        status = -ENOMEM;
        ALOGE("%s: %d: failed to allocate user_id_tracker status %d",
            __func__, __LINE__, status);
        goto exit;
    }

    for (i = 0; i < config->num_phrases; i++) {
        ALOGV("%s: %d: [%d] kw level %d", __func__, __LINE__, i,
            config->phrases[i].confidence_level);
        if (config->phrases[i].confidence_level > ST_MAX_FSTAGE_CONF_LEVEL) {
            ALOGE("%s: %d: Invalid kw level %d", __func__, __LINE__,
                config->phrases[i].confidence_level);
            status = -EINVAL;
            goto exit;
        }
        for (j = 0; j < config->phrases[i].num_levels; j++) {
            ALOGV("%s: %d: [%d] user_id %d level %d ", __func__, __LINE__, i,
                config->phrases[i].levels[j].user_id,
                config->phrases[i].levels[j].level);
            if (config->phrases[i].levels[j].level > ST_MAX_FSTAGE_CONF_LEVEL) {
                ALOGE("%s: %d: Invalid user level %d", __func__, __LINE__,
                    config->phrases[i].levels[j].level);
                status = -EINVAL;
                goto exit;
            }
        }
    }

    /* Example: Say the recognition structure has 3 keywords with users
     *      [0] k1 |uid|
     *              [0] u1 - 1st trainer
     *              [1] u2 - 4th trainer
     *              [3] u3 - 3rd trainer
     *      [1] k2
     *              [2] u2 - 2nd trainer
     *              [4] u3 - 5th trainer
     *      [2] k3
     *              [5] u4 - 6th trainer
     *    Output confidence level array will be
     *    [k1, k2, k3, u1k1, u2k1, u2k2, u3k1, u3k2, u4k3]
     */

    for (i = 0; i < config->num_phrases; i++) {
        conf_levels[i] = config->phrases[i].confidence_level;
        if (info->model_id == 0) {
            for (j = 0; j < config->phrases[i].num_levels; j++) {
                user_level = config->phrases[i].levels[j].level;
                user_id = config->phrases[i].levels[j].user_id;
                if ((user_id < config->num_phrases) ||
                     (user_id >= num_conf_levels)) {
                    status = -EINVAL;
                    ALOGE("%s: %d: Invalid params user id %d status %d",
                        __func__, __LINE__, user_id, status);
                    goto exit;
                } else {
                    if (user_id_tracker[user_id] == 1) {
                        status = -EINVAL;
                        ALOGE("%s: %d: Duplicate user id %d status %d",
                            __func__, __LINE__, user_id, status);
                        goto exit;
                    }
                    conf_levels[user_id] = (user_level < ST_MAX_FSTAGE_CONF_LEVEL) ?
                        user_level : ST_MAX_FSTAGE_CONF_LEVEL;
                    user_id_tracker[user_id] = 1;
                    ALOGV("%s: %d: user_conf_levels[%d] = %d",
                        __func__, __LINE__, user_id, conf_levels[user_id]);
                }
            }
        }
    }

    *out_conf_levels = conf_levels;
    *out_num_conf_levels = num_conf_levels;

exit:
    if (status && conf_levels) {
        free(conf_levels);
        *out_conf_levels = nullptr;
        *out_num_conf_levels = 0;
    }

    if (user_id_tracker)
        free(user_id_tracker);

    ALOGD("%s: %d: Exit, status %d", __func__, __LINE__, status);

    return status;
}

int32_t CustomVAInterface::FillOpaqueConfLevels(
    uint32_t model_id,
    const void *sm_levels_generic,
    uint8_t **out_payload,
    uint32_t *out_payload_size,
    uint32_t version) {

    int status = 0;
    int32_t level = 0;
    unsigned int num_conf_levels = 0;
    unsigned int user_level = 0, user_id = 0;
    unsigned char *conf_levels = nullptr;
    unsigned int i = 0, j = 0;
    unsigned char *user_id_tracker = nullptr;
    struct st_sound_model_conf_levels *sm_levels = nullptr;
    struct st_sound_model_conf_levels_v2 *sm_levels_v2 = nullptr;

    ALOGV("%s: %d: Enter", __func__, __LINE__);

    /*  Example: Say the recognition structure has 3 keywords with users
     *  |kid|
     *  [0] k1 |uid|
     *         [3] u1 - 1st trainer
     *         [4] u2 - 4th trainer
     *         [6] u3 - 3rd trainer
     *  [1] k2
     *         [5] u2 - 2nd trainer
     *         [7] u3 - 5th trainer
     *  [2] k3
     *         [8] u4 - 6th trainer
     *
     *  Output confidence level array will be
     *  [k1, k2, k3, u1k1, u2k1, u2k2, u3k1, u3k2, u4k3]
     */

    if (version != CONF_LEVELS_INTF_VERSION_0002) {
        sm_levels = (struct st_sound_model_conf_levels *)sm_levels_generic;
        if (!sm_levels) {
            status = -EINVAL;
            ALOGE("%s: %d: ERROR. Invalid inputs", __func__, __LINE__);
            goto exit;
        }

        for (i = 0; i < sm_levels->num_kw_levels; i++) {
            level = sm_levels->kw_levels[i].kw_level;
            if (level < 0 || level > ST_MAX_FSTAGE_CONF_LEVEL) {
                ALOGE("%s: %d: Invalid First stage [%d] kw level %d",
                    __func__, __LINE__, i, level);
                status = -EINVAL;
                goto exit;
            } else {
                ALOGD("%s: %d: First stage [%d] kw level %d",
                    __func__, __LINE__, i, level);
            }
            for (j = 0; j < sm_levels->kw_levels[i].num_user_levels; j++) {
                level = sm_levels->kw_levels[i].user_levels[j].level;
                if (level < 0 || level > ST_MAX_FSTAGE_CONF_LEVEL) {
                    ALOGE("%s: %d: Invalid First stage [%d] user_id %d level %d",
                        __func__, __LINE__, i,
                        sm_levels->kw_levels[i].user_levels[j].user_id, level);
                    status = -EINVAL;
                    goto exit;
                } else {
                    ALOGD("%s: %d: First stage [%d] user_id %d level %d ",
                        __func__, __LINE__, i,
                        sm_levels->kw_levels[i].user_levels[j].user_id, level);
                }
            }
        }

        for (i = 0; i < sm_levels->num_kw_levels; i++) {
            num_conf_levels++;
            if (model_id == 0) {
                for (j = 0; j < sm_levels->kw_levels[i].num_user_levels; j++)
                    num_conf_levels++;
            }
        }

        ALOGD("%s: %d: Number of confidence levels: %d",
            __func__, __LINE__, num_conf_levels);

        if (!num_conf_levels) {
            status = -EINVAL;
            ALOGE("%s: %d: ERROR. Invalid num_conf_levels input",
                __func__, __LINE__);
            goto exit;
        }

        conf_levels = (unsigned char*)calloc(1, num_conf_levels);
        if (!conf_levels) {
            status = -ENOMEM;
            ALOGE("%s: %d: conf_levels calloc failed, status %d",
                __func__, __LINE__, status);
            goto exit;
        }

        user_id_tracker = (unsigned char *)calloc(1, num_conf_levels);
        if (!user_id_tracker) {
            status = -ENOMEM;
            ALOGE("%s: %d: failed to allocate user_id_tracker status %d",
                __func__, __LINE__, status);
            goto exit;
        }

        for (i = 0; i < sm_levels->num_kw_levels; i++) {
            if (i < num_conf_levels) {
                conf_levels[i] = sm_levels->kw_levels[i].kw_level;
            } else {
                status = -EINVAL;
                ALOGE("%s: %d: ERROR. Invalid numver of kw levels",
                    __func__, __LINE__);
                goto exit;
            }
            if (model_id == 0) {
                for (j = 0; j < sm_levels->kw_levels[i].num_user_levels; j++) {
                    user_level = sm_levels->kw_levels[i].user_levels[j].level;
                    user_id = sm_levels->kw_levels[i].user_levels[j].user_id;
                    if ((user_id < sm_levels->num_kw_levels) ||
                        (user_id >= num_conf_levels)) {
                        status = -EINVAL;
                        ALOGE("%s: %d: ERROR. Invalid params user id %d > %d",
                            __func__, __LINE__, user_id, num_conf_levels);
                        goto exit;
                    } else {
                        if (user_id_tracker[user_id] == 1) {
                            status = -EINVAL;
                            ALOGE("%s: %d: ERROR. Duplicate user id %d",
                                __func__, __LINE__, user_id);
                            goto exit;
                        }
                        conf_levels[user_id] = user_level;
                        user_id_tracker[user_id] = 1;
                        ALOGE("%s: %d: user_conf_levels[%d] = %d",
                            __func__, __LINE__, user_id, conf_levels[user_id]);
                    }
                }
            }
        }
    } else {
        sm_levels_v2 =
            (struct st_sound_model_conf_levels_v2 *)sm_levels_generic;
        if (!sm_levels_v2) {
            status = -EINVAL;
            ALOGE("%s: %d: ERROR. Invalid inputs", __func__, __LINE__);
            goto exit;
        }

        for (i = 0; i < sm_levels_v2->num_kw_levels; i++) {
            level = sm_levels_v2->kw_levels[i].kw_level;
            if (level < 0 || level > ST_MAX_FSTAGE_CONF_LEVEL) {
                ALOGE("%s: %d: Invalid First stage [%d] kw level %d",
                    __func__, __LINE__, i, level);
                status = -EINVAL;
                goto exit;
            } else {
                ALOGD("%s: %d: First stage [%d] kw level %d",
                    __func__, __LINE__, i, level);
            }
            for (j = 0; j < sm_levels_v2->kw_levels[i].num_user_levels; j++) {
                level = sm_levels_v2->kw_levels[i].user_levels[j].level;
                if (level < 0 || level > ST_MAX_FSTAGE_CONF_LEVEL) {
                    ALOGE("%s: %d: Invalid First stage [%d] user_id %d level %d",
                        __func__, __LINE__, i,
                        sm_levels_v2->kw_levels[i].user_levels[j].user_id, level);
                    status = -EINVAL;
                    goto exit;
                } else {
                    ALOGD("%s: %d: First stage [%d] user_id %d level %d ",
                        __func__, __LINE__, i,
                        sm_levels_v2->kw_levels[i].user_levels[j].user_id, level);
                }
            }
        }

        for (i = 0; i < sm_levels_v2->num_kw_levels; i++) {
            num_conf_levels++;
            if (model_id == 0) {
                for (j = 0; j < sm_levels_v2->kw_levels[i].num_user_levels; j++)
                    num_conf_levels++;
            }
        }

        ALOGD("%s: %d: number of confidence levels: %d",
            __func__, __LINE__, num_conf_levels);

        if (!num_conf_levels) {
            status = -EINVAL;
            ALOGE("%s: %d: ERROR. Invalid num_conf_levels input",
                __func__, __LINE__);
            goto exit;
        }

        conf_levels = (unsigned char*)calloc(1, num_conf_levels);
        if (!conf_levels) {
            status = -ENOMEM;
            ALOGE("%s: %d: conf_levels calloc failed, status %d",
                __func__, __LINE__, status);
            goto exit;
        }

        user_id_tracker = (unsigned char *)calloc(1, num_conf_levels);
        if (!user_id_tracker) {
            status = -ENOMEM;
            ALOGE("%s: %d: failed to allocate user_id_tracker status %d",
                __func__, __LINE__, status);
            goto exit;
        }

        for (i = 0; i < sm_levels_v2->num_kw_levels; i++) {
            if (i < num_conf_levels) {
                conf_levels[i] = sm_levels_v2->kw_levels[i].kw_level;
            } else {
                status = -EINVAL;
                ALOGE("%s: %d: ERROR. Invalid numver of kw levels",
                    __func__, __LINE__);
                goto exit;
            }
            if (model_id == 0) {
                for (j = 0; j < sm_levels_v2->kw_levels[i].num_user_levels; j++) {
                    user_level = sm_levels_v2->kw_levels[i].user_levels[j].level;
                    user_id = sm_levels_v2->kw_levels[i].user_levels[j].user_id;
                    if ((user_id < sm_levels_v2->num_kw_levels) ||
                         (user_id >= num_conf_levels)) {
                        status = -EINVAL;
                        ALOGE("%s: %d: ERROR. Invalid params user id %d > %d",
                            __func__, __LINE__, user_id, num_conf_levels);
                        goto exit;
                    } else {
                        if (user_id_tracker[user_id] == 1) {
                            status = -EINVAL;
                            ALOGE("%s: %d: ERROR. Duplicate user id %d",
                                __func__, __LINE__, user_id);
                            goto exit;
                        }
                        conf_levels[user_id] = user_level;
                        user_id_tracker[user_id] = 1;
                        ALOGV("%s: %d: user_conf_levels[%d] = %d",
                            __func__, __LINE__, user_id, conf_levels[user_id]);
                    }
                }
            }
        }
    }

    *out_payload = conf_levels;
    *out_payload_size = num_conf_levels;
    ALOGD("%s: %d: Returning number of conf levels: %d",
        __func__, __LINE__, *out_payload_size);
exit:
    if (status && conf_levels) {
        free(conf_levels);
        *out_payload = nullptr;
        *out_payload_size = 0;
    }

    if (user_id_tracker)
        free(user_id_tracker);

    ALOGD("%s: %d: Exit, status %d", __func__, __LINE__, status);
    return status;
}

int32_t CustomVAInterface::ParseDetectionPayloadPDK(void *event_data) {
    int32_t status = 0;
    uint32_t payload_size = 0;
    uint32_t parsed_size = 0;
    uint32_t event_size = 0;
    uint32_t keyId = 0;
    uint64_t kwd_start_timestamp = 0;
    uint64_t kwd_end_timestamp = 0;
    uint64_t ftrt_start_timestamp = 0;
    uint8_t *ptr = nullptr;
    struct event_id_detection_engine_generic_info_t *generic_info = nullptr;
    struct detection_event_info_header_t *event_header = nullptr;
    struct ftrt_data_info_t *ftrt_info = nullptr;
    struct voice_ui_multi_model_result_info_t *multi_model_result = nullptr;
    struct model_stats *model_stat = nullptr;
    struct model_stats *detected_model_stat = nullptr;

    ALOGD("%s: %d: Enter", __func__, __LINE__);
    if (!event_data) {
        ALOGE("%s: %d: Invalid event data", __func__, __LINE__);
        return -EINVAL;
    }

    std::memset(&detection_event_info_multi_model_, 0,
            sizeof(struct voice_ui_multi_model_result_info_t));

    generic_info = (struct event_id_detection_engine_generic_info_t *)
                    event_data;
    payload_size = sizeof(struct event_id_detection_engine_generic_info_t);
    event_size = generic_info->payload_size;
    ptr = (uint8_t *)event_data + payload_size;

    if (!event_size) {
        status = -EINVAL;
        ALOGE("%s: %d: Invalid detection payload", __func__, __LINE__);
        goto exit;
    }

    ALOGI("%s: %d: event_size = %u", __func__, __LINE__, event_size);

    while (parsed_size < event_size) {
        ALOGD("%s: %d: parsed_size = %u, event_size = %u",
            __func__, __LINE__, parsed_size, event_size);
        event_header = (struct detection_event_info_header_t *)ptr;
        keyId = event_header->key_id;
        payload_size = event_header->payload_size;
        ptr += sizeof(struct detection_event_info_header_t);
        parsed_size += sizeof(struct detection_event_info_header_t);

        switch (keyId) {
            case KEY_ID_FTRT_DATA_INFO :
                ALOGI("%s: %d: ftrt structure size: %u",
                    __func__, __LINE__, payload_size);

                ftrt_info = (struct ftrt_data_info_t *)ptr;
                detection_event_info_multi_model_.ftrt_data_length_in_us =
                                        ftrt_info->ftrt_data_length_in_us;
                ALOGI("%s: %d: ftrt_data_length_in_us = %u",
                    __func__, __LINE__,
                    detection_event_info_multi_model_.ftrt_data_length_in_us);
                ftrt_size_ = UsToBytes(ftrt_info->ftrt_data_length_in_us);
                break;

            case KEY_ID_VOICE_UI_MULTI_MODEL_RESULT_INFO :
                ALOGI("%s: %d: voice_ui_multi_model_result_info: %u",
                    __func__, __LINE__, payload_size);

                multi_model_result = (struct voice_ui_multi_model_result_info_t *)
                                      ptr;
                detection_event_info_multi_model_.num_detected_models =
                                 multi_model_result->num_detected_models;
                ALOGI("%s: %d: Number of detected models: %d",
                    __func__, __LINE__,
                    detection_event_info_multi_model_.num_detected_models);

                model_stat = (struct model_stats *)(ptr +
                             sizeof(struct voice_ui_multi_model_result_info_t));
                det_model_id_ = model_stat->detected_model_id;
                for (int i = 0; i < detection_event_info_multi_model_.
                                    num_detected_models; ++i) {

                    detection_event_info_multi_model_.detected_model_stats[i].
                    detected_model_id = model_stat->detected_model_id;

                    detection_event_info_multi_model_.detected_model_stats[i].
                    detected_keyword_id = model_stat->detected_keyword_id;
                    ALOGI("%s: %d: detected keyword id: %u", __func__, __LINE__,
                        detection_event_info_multi_model_.detected_model_stats[i].
                            detected_keyword_id);

                    detection_event_info_multi_model_.detected_model_stats[i].
                    best_channel_idx = model_stat->best_channel_idx;

                    detection_event_info_multi_model_.detected_model_stats[i].
                    best_confidence_level = model_stat->best_confidence_level;
                    ALOGI("%s: %d: detected best conf level: %u",
                        __func__, __LINE__,
                        detection_event_info_multi_model_.detected_model_stats[i].
                            best_confidence_level);

                    detection_event_info_multi_model_.detected_model_stats[i].
                    kw_start_timestamp_lsw = model_stat->kw_start_timestamp_lsw;
                    ALOGI("%s: %d: kw_start_timestamp_lsw: %u",
                        __func__, __LINE__,
                        detection_event_info_multi_model_.detected_model_stats[i].
                            kw_start_timestamp_lsw);

                    detection_event_info_multi_model_.detected_model_stats[i].
                    kw_start_timestamp_msw = model_stat->kw_start_timestamp_msw;
                    ALOGI("%s: %d: kw_start_timestamp_msw: %u",
                        __func__, __LINE__,
                        detection_event_info_multi_model_.detected_model_stats[i].
                            kw_start_timestamp_msw);

                    detection_event_info_multi_model_.detected_model_stats[i].
                    kw_end_timestamp_lsw = model_stat->kw_end_timestamp_lsw;
                    ALOGI("%s: %d: kw_end_timestamp_lsw: %u",
                        __func__, __LINE__,
                        detection_event_info_multi_model_.detected_model_stats[i].
                            kw_end_timestamp_lsw);


                    detection_event_info_multi_model_.detected_model_stats[i].
                    kw_end_timestamp_msw = model_stat->kw_end_timestamp_msw;
                    ALOGI("%s: %d: kw_end_timestamp_msw: %u",
                        __func__, __LINE__,
                        detection_event_info_multi_model_.detected_model_stats[i].
                            kw_end_timestamp_msw);


                    detection_event_info_multi_model_.detected_model_stats[i].
                    detection_timestamp_lsw = model_stat->detection_timestamp_lsw;
                    ALOGI("%s: %d: detection_timestamp_lsw: %u",
                        __func__, __LINE__,
                        detection_event_info_multi_model_.detected_model_stats[i].
                            detection_timestamp_lsw);

                    detection_event_info_multi_model_.detected_model_stats[i].
                    detection_timestamp_msw = model_stat->detection_timestamp_msw;
                    ALOGI("%s: %d: detection_timestamp_msw: %u",
                        __func__, __LINE__,
                        detection_event_info_multi_model_.detected_model_stats[i].
                            detection_timestamp_msw);

                    ALOGI("%s: %d: Detection made for model id: %x",
                        __func__, __LINE__,
                        detection_event_info_multi_model_.detected_model_stats[i].
                            detected_model_id);
                    model_stat += sizeof(struct model_stats);
                }
                break;
            default :
                status = -EINVAL;
                ALOGE("%s: %d: Invalid key id %u status %d",
                    __func__, __LINE__, keyId, status);
                goto exit;
        }
        ptr += payload_size;
        parsed_size += payload_size;

    }

    detected_model_stat =
        &detection_event_info_multi_model_.detected_model_stats[0];

    kwd_start_timestamp =
        (uint64_t)detected_model_stat->kw_start_timestamp_lsw +
        ((uint64_t)detected_model_stat->kw_start_timestamp_msw << 32);
    kwd_end_timestamp =
        (uint64_t)detected_model_stat->kw_end_timestamp_lsw +
        ((uint64_t)detected_model_stat->kw_end_timestamp_msw << 32);
    ftrt_start_timestamp =
        (uint64_t)detected_model_stat->detection_timestamp_lsw +
        ((uint64_t)detected_model_stat->detection_timestamp_msw << 32) -
        detection_event_info_multi_model_.ftrt_data_length_in_us;

    UpdateKeywordIndex(kwd_start_timestamp, kwd_end_timestamp,
        ftrt_start_timestamp);

exit :
    ALOGD("%s: %d: Exit, status %d", __func__, __LINE__, status);

    return status;
}

int32_t CustomVAInterface::ParseDetectionPayloadGMM(void *event_data) {
    int32_t status = 0;
    int32_t i = 0;
    uint32_t parsed_size = 0;
    uint32_t payload_size = 0;
    uint32_t event_size = 0;
    uint64_t kwd_start_timestamp = 0;
    uint64_t kwd_end_timestamp = 0;
    uint64_t ftrt_start_timestamp = 0;
    uint8_t *ptr = nullptr;
    struct event_id_detection_engine_generic_info_t *generic_info = nullptr;
    struct detection_event_info_header_t *event_header = nullptr;
    struct confidence_level_info_t *confidence_info = nullptr;
    struct keyword_position_info_t *keyword_position_info = nullptr;
    struct detection_timestamp_info_t *detection_timestamp_info = nullptr;
    struct ftrt_data_info_t *ftrt_info = nullptr;

    ALOGD("%s: %d: Enter", __func__, __LINE__);
    if (!event_data) {
        ALOGE("%s: %d: Invalid event data", __func__, __LINE__);
        return -EINVAL;
    }

    std::memset(&detection_event_info_, 0, sizeof(struct detection_event_info));

    generic_info =
        (struct event_id_detection_engine_generic_info_t *)event_data;
    payload_size = sizeof(struct event_id_detection_engine_generic_info_t);
    detection_event_info_.status = generic_info->status;
    event_size = generic_info->payload_size;
    ptr = (uint8_t *)event_data + payload_size;
    ALOGI("%s: %d: status = %u, event_size = %u", __func__, __LINE__,
        detection_event_info_.status, event_size);
    if (status || !event_size) {
        status = -EINVAL;
        ALOGE("%s: %d: Invalid detection payload", __func__, __LINE__);
        goto exit;
    }

    // parse variable payload
    while (parsed_size < event_size) {
        ALOGD("%s: %d: parsed_size = %u, event_size = %u",
            __func__, __LINE__, parsed_size, event_size);
        event_header = (struct detection_event_info_header_t *)ptr;
        uint32_t keyId = event_header->key_id;
        payload_size = event_header->payload_size;
        ALOGD("%s: %d: key id = %u, payload_size = %u",
            __func__, __LINE__, keyId, payload_size);
        ptr += sizeof(struct detection_event_info_header_t);
        parsed_size += sizeof(struct detection_event_info_header_t);

        switch (keyId) {
        case KEY_ID_CONFIDENCE_LEVELS_INFO:
            confidence_info = (struct confidence_level_info_t *)ptr;
            detection_event_info_.num_confidence_levels =
                confidence_info->number_of_confidence_values;
            ALOGI("%s: %d: num_confidence_levels = %u", __func__, __LINE__,
                detection_event_info_.num_confidence_levels);
            for (i = 0; i < detection_event_info_.num_confidence_levels; i++) {
                detection_event_info_.confidence_levels[i] =
                    confidence_info->confidence_levels[i];
                ALOGI("%s: %d: confidence_levels[%d] = %u",
                    __func__, __LINE__, i,
                    detection_event_info_.confidence_levels[i]);
            }
            break;
        case KEY_ID_KWD_POSITION_INFO:
            keyword_position_info = (struct keyword_position_info_t *)ptr;
            detection_event_info_.kw_start_timestamp_lsw =
                keyword_position_info->kw_start_timestamp_lsw;
            detection_event_info_.kw_start_timestamp_msw =
                keyword_position_info->kw_start_timestamp_msw;
            detection_event_info_.kw_end_timestamp_lsw =
                keyword_position_info->kw_end_timestamp_lsw;
            detection_event_info_.kw_end_timestamp_msw =
                keyword_position_info->kw_end_timestamp_msw;
            ALOGI("%s: %d: start_lsw = %u, start_msw = %u, "
                    "end_lsw = %u, end_msw = %u", __func__, __LINE__,
                    detection_event_info_.kw_start_timestamp_lsw,
                    detection_event_info_.kw_start_timestamp_msw,
                    detection_event_info_.kw_end_timestamp_lsw,
                    detection_event_info_.kw_end_timestamp_msw);
            break;
        case KEY_ID_TIMESTAMP_INFO:
            detection_timestamp_info = (struct detection_timestamp_info_t *)ptr;
            detection_event_info_.detection_timestamp_lsw =
                detection_timestamp_info->detection_timestamp_lsw;
            detection_event_info_.detection_timestamp_msw =
                detection_timestamp_info->detection_timestamp_msw;
            ALOGI("%s: %d: timestamp_lsw = %u, timestamp_msw = %u",
                __func__, __LINE__,
                detection_event_info_.detection_timestamp_lsw,
                detection_event_info_.detection_timestamp_msw);
            break;
        case KEY_ID_FTRT_DATA_INFO:
            ftrt_info = (struct ftrt_data_info_t *)ptr;
            ftrt_size_ = UsToBytes(ftrt_info->ftrt_data_length_in_us);
            detection_event_info_.ftrt_data_length_in_us =
                ftrt_info->ftrt_data_length_in_us;
            ALOGI("%s: %d: ftrt_data_length_in_us = %u",
                __func__, __LINE__,
                detection_event_info_.ftrt_data_length_in_us);
            break;
        default:
            status = -EINVAL;
            ALOGE("%s: %d: Invalid key id %u status %d",
                __func__, __LINE__, keyId, status);
            goto exit;
        }
        ptr += payload_size;
        parsed_size += payload_size;
    }

    kwd_start_timestamp =
        (uint64_t)detection_event_info_.kw_start_timestamp_lsw +
        ((uint64_t)detection_event_info_.kw_start_timestamp_msw << 32);
    kwd_end_timestamp =
        (uint64_t)detection_event_info_.kw_end_timestamp_lsw +
        ((uint64_t)detection_event_info_.kw_end_timestamp_msw << 32);
    ftrt_start_timestamp =
        (uint64_t)detection_event_info_.detection_timestamp_lsw +
        ((uint64_t)detection_event_info_.detection_timestamp_msw << 32) -
        detection_event_info_.ftrt_data_length_in_us;

    UpdateKeywordIndex(kwd_start_timestamp, kwd_end_timestamp,
        ftrt_start_timestamp);
    det_model_id_ = 0;
exit:
    ALOGD("%s: %d: Exit, status %d", __func__, __LINE__, status);

    return status;
}

void CustomVAInterface::UpdateKeywordIndex(uint64_t kwd_start_timestamp,
    uint64_t kwd_end_timestamp, uint64_t ftrt_start_timestamp) {

    ALOGV("%s: %d: kwd start timestamp: %llu, kwd end timestamp: %llu",
        __func__, __LINE__,
        (long long)kwd_start_timestamp, (long long)kwd_end_timestamp);
    ALOGV("%s: %d: Ftrt data start timestamp: %llu", __func__, __LINE__,
        (long long)ftrt_start_timestamp);

    if (kwd_start_timestamp >= kwd_end_timestamp ||
        kwd_start_timestamp < ftrt_start_timestamp) {
        ALOGD("%s: %d: Invalid timestamp, cannot compute keyword index",
            __func__, __LINE__);
        return;
    }

    start_index_ = UsToBytes(kwd_start_timestamp - ftrt_start_timestamp);
    end_index_ = UsToBytes(kwd_end_timestamp - ftrt_start_timestamp);
    ALOGI("%s: %d: start_index: %zu, end_index: %zu",
        __func__, __LINE__, start_index_, end_index_);
}

void CustomVAInterface::PackEventConfLevels(struct sound_model_info *sm_info,
    uint8_t *opaque_data) {

    struct st_confidence_levels_info *conf_levels = nullptr;
    struct st_confidence_levels_info_v2 *conf_levels_v2 = nullptr;
    uint32_t i = 0, j = 0, k = 0, user_id = 0, num_user_levels = 0;

    ALOGV("%s: %d: Enter", __func__, __LINE__);

    /*
     * Update the opaque data of callback event with confidence levels
     * accordingly for all users and keywords from the detection event
     */
    if (conf_levels_intf_version_ != CONF_LEVELS_INTF_VERSION_0002) {
        conf_levels = (struct st_confidence_levels_info *)opaque_data;
        for (i = 0; i < conf_levels->num_sound_models; i++) {
            if (conf_levels->conf_levels[i].sm_id == ST_SM_ID_SVA_F_STAGE_GMM) {
                for (j = 0; j < conf_levels->conf_levels[i].num_kw_levels; j++) {
                    if (j <= sm_info->info->GetConfLevelsSize())
                        conf_levels->conf_levels[i].kw_levels[j].kw_level =
                            sm_info->info->GetDetConfLevels()[j];
                    else
                        ALOGE("%s: %d: unexpected conf size %d < %d",
                            __func__, __LINE__,
                            sm_info->info->GetConfLevelsSize(), j);

                    num_user_levels =
                        conf_levels->conf_levels[i].kw_levels[j].num_user_levels;
                    for (k = 0; k < num_user_levels; k++) {
                        user_id = conf_levels->conf_levels[i].kw_levels[j].
                            user_levels[k].user_id;
                        if (user_id <= sm_info->info->GetConfLevelsSize())
                            conf_levels->conf_levels[i].kw_levels[j].user_levels[k].
                                level = sm_info->info->GetDetConfLevels()[user_id];
                        else
                            ALOGE("%s: %d: Unexpected conf size %d < %d",
                                __func__, __LINE__,
                                sm_info->info->GetConfLevelsSize(), user_id);
                    }
                }
            } else if (conf_levels->conf_levels[i].sm_id & ST_SM_ID_SVA_S_STAGE_KWD ||
                       conf_levels->conf_levels[i].sm_id & ST_SM_ID_SVA_S_STAGE_USER) {
                /* Update confidence levels for second stage */
                for (auto &iter: sm_info->sec_det_level) {
                    if ((conf_levels->conf_levels[i].sm_id & ST_SM_ID_SVA_S_STAGE_KWD) &&
                        (iter.first & ST_SM_ID_SVA_S_STAGE_KWD)) {
                        conf_levels->conf_levels[i].kw_levels[0].kw_level = iter.second;
                        conf_levels->conf_levels[i].kw_levels[0].user_levels[0].level = 0;
                    } else if ((conf_levels->conf_levels[i].sm_id & ST_SM_ID_SVA_S_STAGE_USER) &&
                               (iter.first == conf_levels->conf_levels[i].sm_id)) {
                        conf_levels->conf_levels[i].kw_levels[0].kw_level = iter.second;
                        conf_levels->conf_levels[i].kw_levels[0].user_levels[0].level = iter.second;
                    }
                }
            }
        }
    } else {
        conf_levels_v2 = (struct st_confidence_levels_info_v2 *)opaque_data;
        for (i = 0; i < conf_levels_v2->num_sound_models; i++) {
            if (conf_levels_v2->conf_levels[i].sm_id == ST_SM_ID_SVA_F_STAGE_GMM) {
                for (j = 0; j < conf_levels_v2->conf_levels[i].num_kw_levels; j++) {
                    if (j <= sm_info->info->GetConfLevelsSize())
                            conf_levels_v2->conf_levels[i].kw_levels[j].kw_level =
                                    sm_info->info->GetDetConfLevels()[j];
                    else
                        ALOGE("%s: %d: unexpected conf size %d < %d",
                            __func__, __LINE__,
                            sm_info->info->GetConfLevelsSize(), j);

                    ALOGI("%s: %d: First stage KW Conf levels[%d]-%d",
                        __func__, __LINE__,
                        j, sm_info->info->GetDetConfLevels()[j]);

                    num_user_levels =
                        conf_levels_v2->conf_levels[i].kw_levels[j].num_user_levels;
                    for (k = 0; k < num_user_levels; k++) {
                        user_id = conf_levels_v2->conf_levels[i].kw_levels[j].
                            user_levels[k].user_id;
                        if (user_id <=  sm_info->info->GetConfLevelsSize())
                            conf_levels_v2->conf_levels[i].kw_levels[j].user_levels[k].
                                level = sm_info->info->GetDetConfLevels()[user_id];
                        else
                            ALOGE("%s: %d: Unexpected conf size %d < %d",
                                __func__, __LINE__,
                                sm_info->info->GetConfLevelsSize(), user_id);

                        ALOGI("%s: %d: First stage User Conf levels[%d]-%d",
                            __func__, __LINE__,
                            k, sm_info->info->GetDetConfLevels()[user_id]);
                    }
                }
            } else if (conf_levels_v2->conf_levels[i].sm_id & ST_SM_ID_SVA_S_STAGE_KWD ||
                       conf_levels_v2->conf_levels[i].sm_id & ST_SM_ID_SVA_S_STAGE_USER) {
                /* Update confidence levels for second stage */
                for (auto &iter: sm_info->sec_det_level) {
                    if ((conf_levels_v2->conf_levels[i].sm_id & ST_SM_ID_SVA_S_STAGE_KWD) &&
                        (iter.first & ST_SM_ID_SVA_S_STAGE_KWD)) {
                        conf_levels_v2->conf_levels[i].kw_levels[0].kw_level = iter.second;
                        conf_levels_v2->conf_levels[i].kw_levels[0].user_levels[0].level = 0;
                    } else if ((conf_levels_v2->conf_levels[i].sm_id & ST_SM_ID_SVA_S_STAGE_USER) &&
                               (iter.first == ST_SM_ID_SVA_S_STAGE_USER)) {
                        conf_levels_v2->conf_levels[i].kw_levels[0].kw_level = iter.second;
                        conf_levels_v2->conf_levels[i].kw_levels[0].user_levels[0].level = iter.second;
                    }
                }
            }
        }
    }
    ALOGV("%s: %d: Exit", __func__, __LINE__);
}

void CustomVAInterface::FillCallbackConfLevels(
    struct sound_model_info *sm_info,
    uint8_t *opaque_data,
    uint32_t det_keyword_id,
    uint32_t best_conf_level) {

    int i = 0;
    struct st_confidence_levels_info_v2 *conf_levels_v2 = nullptr;
    struct st_confidence_levels_info *conf_levels = nullptr;

    if (conf_levels_intf_version_ != CONF_LEVELS_INTF_VERSION_0002) {
        conf_levels = (struct st_confidence_levels_info *)opaque_data;
        for (i = 0; i < conf_levels->num_sound_models; i++) {
            if (conf_levels->conf_levels[i].sm_id == ST_SM_ID_SVA_F_STAGE_GMM) {
                conf_levels->conf_levels[i].kw_levels[det_keyword_id].
                    kw_level = best_conf_level;
                conf_levels->conf_levels[i].kw_levels[det_keyword_id].
                    user_levels[0].level = 0;
                ALOGI("%s: %d: First stage returning conf level: %d",
                    __func__, __LINE__, best_conf_level);
            } else if (conf_levels->conf_levels[i].sm_id & ST_SM_ID_SVA_S_STAGE_KWD) {
                for (auto iter: sm_info->sec_det_level) {
                    if (iter.first & ST_SM_ID_SVA_S_STAGE_KWD) {
                        conf_levels->conf_levels[i].kw_levels[0].kw_level = iter.second;
                        conf_levels->conf_levels[i].kw_levels[0].user_levels[0].level = 0;
                        ALOGI("%s: %d: Second stage keyword conf level: %d",
                            __func__, __LINE__, iter.second);
                    }
                }
            } else if (conf_levels->conf_levels[i].sm_id & ST_SM_ID_SVA_S_STAGE_USER) {
                for (auto iter: sm_info->sec_det_level) {
                    if (iter.first & ST_SM_ID_SVA_S_STAGE_USER) {
                        conf_levels->conf_levels[i].kw_levels[0].kw_level = iter.second;
                        conf_levels->conf_levels[i].kw_levels[0].user_levels[0].level = iter.second;
                        ALOGI("%s: %d: Second stage user conf level: %d",
                            __func__, __LINE__, iter.second);
                    }
                }
            }
        }
    } else {
        conf_levels_v2 = (struct st_confidence_levels_info_v2 *)opaque_data;
        for (i = 0; i < conf_levels_v2->num_sound_models; i++) {
            if (conf_levels_v2->conf_levels[i].sm_id == ST_SM_ID_SVA_F_STAGE_GMM) {
                conf_levels_v2->conf_levels[i].kw_levels[det_keyword_id].
                    kw_level = best_conf_level;
                conf_levels_v2->conf_levels[i].kw_levels[det_keyword_id].
                    user_levels[0].level = 0;
                ALOGI("%s: %d: First stage returning conf level: %d",
                    __func__, __LINE__,
                    best_conf_level);
            } else if (conf_levels_v2->conf_levels[i].sm_id & ST_SM_ID_SVA_S_STAGE_KWD) {
                for (auto iter: sm_info->sec_det_level) {
                    if (iter.first & ST_SM_ID_SVA_S_STAGE_KWD) {
                        conf_levels_v2->conf_levels[i].kw_levels[0].kw_level = iter.second;
                        conf_levels_v2->conf_levels[i].kw_levels[0].user_levels[0].level = 0;
                        ALOGI("%s: %d: Second stage keyword conf level: %d",
                            __func__, __LINE__, iter.second);
                    }
                }
            } else if (conf_levels_v2->conf_levels[i].sm_id & ST_SM_ID_SVA_S_STAGE_USER) {
                for (auto iter: sm_info->sec_det_level) {
                    if (iter.first & ST_SM_ID_SVA_S_STAGE_USER) {
                        conf_levels_v2->conf_levels[i].kw_levels[0].kw_level = iter.second;
                        conf_levels_v2->conf_levels[i].kw_levels[0].user_levels[0].level = iter.second;
                        ALOGI("%s: %d: Second stage user conf level: %d",
                            __func__, __LINE__, iter.second);
                    }
                }
            }
        }
    }
}

void CustomVAInterface::CheckAndSetDetectionConfLevels(void *s) {
    ALOGD("%s: %d: Enter", __func__, __LINE__);

    if (!s) {
        ALOGE("%s: %d: Invalid detected stream", __func__, __LINE__);
        return;
    }

    if (detection_event_info_.num_confidence_levels <
            sound_model_info_->GetConfLevelsSize()) {
        ALOGE("%s: %d: detection event conf lvls %d < eng conf lvl size %d",
            __func__, __LINE__, detection_event_info_.num_confidence_levels,
            sound_model_info_->GetConfLevelsSize());
        return;
    }
    /* Reset any cached previous detection conf level values */
    sm_info_map_[s]->info->ResetDetConfLevels();

    /* Extract the stream conf level values from SPF detection payload */
    for (uint32_t i = 0; i < sound_model_info_->GetConfLevelsSize(); i++) {
        if (!detection_event_info_.confidence_levels[i])
            continue;
        for (uint32_t j = 0; j < sm_info_map_[s]->info->GetConfLevelsSize(); j++) {
            if (!strcmp(sm_info_map_[s]->info->GetConfLevelsKwUsers()[j],
                 sound_model_info_->GetConfLevelsKwUsers()[i])) {
                 sm_info_map_[s]->info->UpdateDetConfLevel(j,
                   detection_event_info_.confidence_levels[i]);
            }
        }
    }

    for (uint32_t i = 0; i < sm_info_map_[s]->info->GetConfLevelsSize(); i++)
        ALOGI("%s: %d: det_cf_levels[%d]-%d", __func__, __LINE__, i,
            sm_info_map_[s]->info->GetDetConfLevels()[i]);
}

void CustomVAInterface::UpdateDetectionResult(void *s, uint32_t result) {
    if (sm_info_map_.find(s) != sm_info_map_.end() && sm_info_map_[s]) {
        sm_info_map_[s]->det_result = result;
    }
}

int32_t CustomVAInterface::GetSoundModelLoadPayload(vui_intf_param_t *param) {
    void *s = nullptr;
    sound_model_data_t *sm_data = nullptr;

    if (!param) {
        ALOGE("%s: %d: Invalid param", __func__, __LINE__);
        return -EINVAL;
    }

    s = param->stream;
    if (sm_info_map_.find(s) == sm_info_map_.end()) {
        ALOGD("%s: %d: Stream not registered", __func__, __LINE__);
        return -EINVAL;
    }

    if (!sm_info_map_[s]) {
        ALOGE("%s: %d: Invalid sound model info", __func__, __LINE__);
        return -EINVAL;
    }

    sm_data = sm_info_map_[s]->model_list[0];
    param->data = (void *)sm_data->data;
    param->size = sm_data->size;

    return 0;
}

int32_t CustomVAInterface::GetCustomPayload(vui_intf_param_t *param) {
    void *s = nullptr;
    struct sound_model_info *info = nullptr;

    if (!param) {
        ALOGE("%s: %d: Invalid param", __func__, __LINE__);
        return -EINVAL;
    }

    s = param->stream;
    if (sm_info_map_.find(s) == sm_info_map_.end()) {
        ALOGD("%s: %d: Stream not registered", __func__, __LINE__);
        return -EINVAL;
    }

    info = sm_info_map_[s];
    if (!info) {
        ALOGE("%s: %d: Invalid sound model info", __func__, __LINE__);
        return -EINVAL;
    }

    param->data = info->wakeup_config;
    param->size = info->wakeup_config_size;

    return 0;
}

int32_t CustomVAInterface::GetBufferingPayload(vui_intf_param_t *param) {
    void *s = nullptr;
    struct sound_model_info *info = nullptr;

    if (!param) {
        ALOGE("%s: %d: Invalid param", __func__, __LINE__);
        return -EINVAL;
    }

    s = param->stream;
    if (sm_info_map_.find(s) == sm_info_map_.end()) {
        ALOGD("%s: %d: Stream not registered", __func__, __LINE__);
        return -EINVAL;
    }

    info = sm_info_map_[s];
    if (!info) {
        ALOGE("%s: %d: Invalid sound model info", __func__, __LINE__);
        return -EINVAL;
    }

    buffering_config_.hist_buffer_duration_msec =
        info->buf_config.hist_buffer_duration;

    buffering_config_.pre_roll_duration_msec =
        info->buf_config.pre_roll_duration;

    param->data = (void *)&buffering_config_;
    param->size = sizeof(buffering_config_);

    return 0;
}

void CustomVAInterface::SetStreamAttributes(
    struct pal_stream_attributes *attr) {

    if (!attr) {
        ALOGE("%s: %d: Invalid stream attributes", __func__, __LINE__);
        return;
    }

    ar_mem_cpy(&str_attr_, sizeof(struct pal_stream_attributes),
        attr, sizeof(struct pal_stream_attributes));
}

int32_t CustomVAInterface::RegisterModel(void *s,
    struct pal_st_sound_model *model,
    const std::vector<sound_model_data_t *> model_list) {

    int32_t status = 0;

    if (sm_info_map_.find(s) == sm_info_map_.end()) {
        sm_info_map_[s] = (struct sound_model_info *)calloc(1,
            sizeof(struct sound_model_info));
        if (!sm_info_map_[s]) {
            ALOGE("%s: %d: Failed to allocate memory for sm data",
                __func__, __LINE__);
            status = -ENOMEM;
            goto exit;
        }
    }

    sm_info_map_[s]->model_list.clear();
    for (auto iter: model_list) {
        sm_info_map_[s]->model_list.push_back(iter);
    }
    sm_info_map_[s]->model = model;
    sm_info_map_[s]->type = model->type;

exit:
    return status;
}

void CustomVAInterface::DeregisterModel(void *s) {
    sound_model_data_t *sm_data = nullptr;

    auto iter = sm_info_map_.find(s);
    if (iter != sm_info_map_.end() && sm_info_map_[s]) {
        if (sm_info_map_[s]->wakeup_config)
            free(sm_info_map_[s]->wakeup_config);

        sm_info_map_[s]->sec_threshold.clear();
        sm_info_map_[s]->sec_threshold.shrink_to_fit();
        sm_info_map_[s]->sec_det_level.clear();
        sm_info_map_[s]->sec_det_level.shrink_to_fit();

        for (int i = 0; i < sm_info_map_[s]->model_list.size(); i++) {
            sm_data = sm_info_map_[s]->model_list[i];
            if (sm_data) {
                if (sm_data->data)
                    free(sm_data->data);
                free(sm_data);
            }
        }
        sm_info_map_[s]->model_list.clear();
        sm_info_map_[s]->model_list.shrink_to_fit();
        free(sm_info_map_[s]);
        sm_info_map_.erase(iter);
    } else {
        ALOGD("%s: %d: Cannot deregister unregistered model",
            __func__, __LINE__);
    }
}

void CustomVAInterface::GetKeywordIndex(struct keyword_index *index) {

    index->start_index = start_index_;
    index->end_index = end_index_;
}

uint32_t CustomVAInterface::UsToBytes(uint64_t input_us) {
    uint32_t bytes = 0;

    bytes = str_attr_.in_media_config.sample_rate *
            str_attr_.in_media_config.bit_width *
            str_attr_.in_media_config.ch_info.channels * input_us /
            (BITS_PER_BYTE * US_PER_SEC);

    return bytes;
}
