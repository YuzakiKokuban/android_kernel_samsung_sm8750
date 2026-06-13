/*
 * Copyright (c) 2010, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.

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


#ifndef _QSAP_H
#define _QSAP_H

#if __cplusplus
extern "C" {
#endif

#include "qsap_api.h"

s32 wifi_qsap_load_driver(void);
s32 wifi_qsap_unload_driver(void);
s32 wifi_qsap_stop_bss(void);
s32 commit(void);
s32 is_softap_enabled(void);
s32 wifi_qsap_start_softap(void);
s32 wifi_qsap_stop_softap(void);
s32 wifi_qsap_start_wigig_softap(void);
s32 wifi_qsap_stop_wigig_softap(void);
s32 wifi_qsap_reload_softap(void);
s32 wifi_qsap_unload_wifi_sta_driver(void);

#ifdef QCOM_WLAN_CONCURRENCY
s32 wifi_qsap_start_softap_in_concurrency(void);
s32 wifi_qsap_stop_softap_in_concurrency(void);
#endif

#if __cplusplus
};  // extern "C"
#endif

#endif  // _QSAP_H
