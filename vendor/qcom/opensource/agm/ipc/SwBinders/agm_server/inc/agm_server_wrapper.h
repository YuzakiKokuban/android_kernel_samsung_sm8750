/*
** Copyright (c) 2019, 2021 The Linux Foundation. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above
**     copyright notice, this list of conditions and the following
**     disclaimer in the documentation and/or other materials provided
**     with the distribution.
**   * Neither the name of The Linux Foundation nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
** ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
** BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
** OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
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
**/


#ifndef __Agm_SERVER_H__
#define __Agm_SERVER_H__

#include <utils/Log.h>
#include <agm/agm_api.h>
#include "ipc_interface.h"
#include "utils.h"

class AgmService : public BnAgmService
{
    public:
        AgmService()
        {
          AGM_LOGV("AGMService constructor\n");
          agm_initialized = ((agm_init() == 0)?true:false);
        }
        virtual int ipc_agm_init();
        virtual int ipc_agm_audio_intf_set_metadata(uint32_t audio_intf,
                                     uint32_t size, uint8_t *metadata);
        virtual int ipc_agm_session_set_metadata(uint32_t session_id,
                                     uint32_t size, uint8_t *metadata);
        virtual int ipc_agm_session_audio_inf_set_metadata(uint32_t session_id,
                                     uint32_t audio_intf, uint32_t size,
                                     uint8_t *metadata);
        virtual int ipc_agm_session_close(uint64_t handle);
        virtual int ipc_agm_audio_intf_set_media_config(uint32_t audio_intf,
                                     struct agm_media_config *media_config);
        virtual int ipc_agm_session_prepare(uint64_t handle);
        virtual int ipc_agm_session_start(uint64_t handle);
        virtual int ipc_agm_session_stop(uint64_t handle);
        virtual int ipc_agm_session_pause(uint64_t handle);
        virtual int ipc_agm_session_resume(uint64_t handle);
        virtual int ipc_agm_session_open(uint32_t session_id,
                                         enum agm_session_mode sess_mode,
                                         uint64_t *handle);
        virtual int ipc_agm_session_read(uint64_t handle, void *buff,
                                     size_t *count);
        virtual int ipc_agm_session_write(uint64_t handle, void *buff,
                                     size_t *count);
        virtual int ipc_agm_session_audio_inf_connect(uint32_t session_id,
                                     uint32_t audio_intf, bool state);
        virtual int ipc_agm_session_set_loopback(uint32_t capture_session_id,
                                     uint32_t playback_session_id, bool state);
        virtual size_t ipc_agm_get_hw_processed_buff_cnt(uint64_t handle,
                                     enum direction dir);
        virtual int ipc_agm_get_aif_info_list(struct aif_info *aif_list,
                                     size_t *num_aif_info);
        virtual int ipc_agm_session_aif_get_tag_module_info(uint32_t session_id,
                                     uint32_t aif_id, void *payload,
                                     size_t *size);
        virtual int ipc_agm_get_params_from_acdb_tunnel(void *payload,
                                     size_t *size);
        virtual int ipc_agm_session_aif_set_params(uint32_t session_id,
                                     uint32_t aif_id, void *payload,
                                     size_t size);
        virtual int ipc_agm_aif_set_params(uint32_t aif_id,
                                    void* payload, size_t size);
        virtual int ipc_agm_session_set_params(uint32_t session_id,
                                     void *payload, size_t size);
        virtual int ipc_agm_set_params_with_tag(uint32_t session_id,
                                     uint32_t aif_id,
                                     struct agm_tag_config *tag_config);
        virtual int ipc_agm_session_register_for_events(uint32_t session_id,
                                     struct agm_event_reg_cfg *evt_reg_cfg);
        virtual int ipc_agm_session_register_cb(uint32_t session_id,
                                     agm_event_cb cb, enum event_type event,
                                     void *client_data) ;
        virtual int ipc_agm_session_set_config(
                                     uint64_t handle,
                                     struct agm_session_config *session_config,
                                     struct agm_media_config *media_config,
                                     struct agm_buffer_config *buffer_config);
        virtual int ipc_agm_session_set_ec_ref(uint32_t capture_session_id,
                                     uint32_t aif_id, bool state);
        virtual int ipc_agm_session_aif_set_cal(
                                     uint32_t session_id,
                                     uint32_t audio_intf,
                                     struct agm_cal_config *cal_config);
        virtual int ipc_agm_session_eos(uint64_t handle);
        virtual int ipc_agm_get_session_time(uint64_t handle, uint64_t *timestamp);
        virtual int ipc_agm_session_get_params(uint32_t session_id,
                                      void *payload, size_t size);
        virtual int ipc_agm_get_buffer_timestamp(uint32_t session_id, uint64_t *timestamp);
        virtual int ipc_agm_set_gapless_session_metadata(uint64_t handle,
                         enum agm_gapless_silence_type type, uint32_t silence);
        virtual int ipc_agm_session_get_buf_info(uint32_t session_id,
                           struct agm_buf_info *buf_info, uint32_t flag);
        ~AgmService()
        {
            AGM_LOGV("AGMService destructor");
            agm_deinit();
        }

        int is_agm_service_initialized() {
           return agm_initialized;
        }
    private:
         bool agm_initialized = false;
};

#endif
