/*
** Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
**/

#ifndef __UTILS_H__
#include "ar_osal_error.h"

#ifdef FEATURE_IPQ_OPENWRT
#include <audio_utils/log.h>
#else
#include <log/log.h>
#endif

#define AGM_LOGE(arg,...) ALOGE("%s: %d "  arg, __func__, __LINE__, ##__VA_ARGS__)
#define AGM_LOGD(arg,...) ALOGD("%s: %d "  arg, __func__, __LINE__, ##__VA_ARGS__)
#define AGM_LOGI(arg,...) ALOGI("%s: %d "  arg, __func__, __LINE__, ##__VA_ARGS__)
#define AGM_LOGV(arg,...) ALOGV("%s: %d "  arg, __func__, __LINE__, ##__VA_ARGS__)

/*convert osal error codes to lnx error codes*/
int ar_err_get_lnx_err_code(uint32_t error);
/*helper to print errors in string form*/
char *ar_err_get_err_str(uint32_t error);

#endif /*__UTILS_H*/
