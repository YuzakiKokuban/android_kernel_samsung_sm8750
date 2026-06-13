/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
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

/** @file qmi_secgps_clnt.h
*/

#ifndef QMI_SECGPS_CLNT_H
#define QMI_SECGPS_CLNT_H

#ifdef __cplusplus
extern "C" {
#endif


/*=============================================================================
 *
 *                             DATA DECLARATION
 *
 *============================================================================*/

//#include "qmi.h"
#include "qmi_cci_target.h"
#include "qmi_client.h"
#include "qmi_idl_lib.h"
#include "qmi_cci_common.h"
#include <string.h>
#include "qmi_secgps_api_v01.h"

#define SECGPS_SYNC_REQUEST_TIMEOUT 1000 // msec

//Defining values associated with FTM CW mode.
#define SECGPS_FTM_RESP_OK 0
#define SECGPS_FTM_RESP_NG_CN0 1
#define SECGPS_FTM_RESP_NG_TO 2
#define SECGPS_FTM_RESP_NG 3

#define SECGPS_MAX_AGNSS_CONFIG_MSG_SIZE 2048

enum secgps_ftm_channel_mode_e_type
{
   FTM_MULTIPLE_CHANNEL_MODE = 0,
   FTM_SINGLE_CHANNEL_MODE
};

extern int qmi_secgps_client_init();
extern int qmi_secgps_client_deinit();
extern void secgps_set_agps_mode(int mode);
extern void secgps_set_xtra_enable(int enable);
extern void secgps_set_gnss_rf_config(int gnss_cfg);
extern void secgps_set_spirent_type(int spirent_type);
extern void secgps_operation_ftm_mode(int action, int system_type, int freq);
extern void secgps_inject_ni_message(uint8_t* message, uint32_t length, int8_t sub_id);
// ++ NS-FLP
extern void secgps_change_engine_only_mode(uint8_t msg);
// -- NS-FLP
extern void secgps_agnss_config_message(uint8_t* message, uint32_t length);
extern void handleAgnssConfigIndMsg(uint8_t* ind_msg, uint32_t length);
#if 0
extern void secgps_set_use_ssl(int enable);
#endif

#ifdef __cplusplus
}
#endif

#endif /* LOC_API_V02_CLIENT_H*/
