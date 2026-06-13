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
 */
#define LOG_TAG "AGM"

#include<errno.h>

#include <agm/utils.h>

/* ERROR STRING */
#define AR_EOK_STR          "AR_EOK"
#define AR_EFAILED_STR      "AR_EFAILED"
#define AR_EBADPARAM_STR    "AR_EBADPARAM"
#define AR_EUNSUPPORTED_STR "AR_EUNSUPPORTED"
#define AR_EVERSION_STR     "AR_EVERSION"
#define AR_EUNEXPECTED_STR  "AR_EUNEXPECTED"
#define AR_EPANIC_STR       "AR_EPANIC"
#define AR_ENORESOURCE_STR  "AR_ENORESOURCE"
#define AR_EHANDLE_STR      "AR_EHANDLE"
#define AR_EALREADY_STR     "AR_EALREADY"
#define AR_ENOTREADY_STR    "AR_ENOTREADY"
#define AR_EPENDING_STR     "AR_EPENDING"
#define AR_EBUSY_STR        "AR_EBUSY"
#define AR_EABORTED_STR     "AR_EABORTED"
#define AR_ECONTINUE_STR    "AR_ECONTINUE"
#define AR_EIMMEDIATE_STR   "AR_EIMMEDIATE"
#define AR_ENOTIMPL_STR     "AR_ENOTIMPL"
#define AR_ENEEDMORE_STR    "AR_ENEEDMORE"
#define AR_ENOMEMORY_STR    "AR_ENOMEMORY"
#define AR_ENOTEXIST_STR    "AR_ENOTEXIST"
#define AR_ETERMINATED_STR  "AR_ETERMINATED"
#define AR_ETIMEOUT_STR     "AR_ETIMEOUT"
#define AR_EIODATA_STR      "AR_EIODATA"
#define AR_ESUBSYSRESET_STR "AR_ESUBSYSRESET"
#define AR_ERR_MAX_STR      "AR_ERR_MAX"

/*
 *osal layer does not define a max error code hence assigning it
 *from based on the latest header, need to revisit each time
 *a new error code is added
 */
#define AR_ERR_MAX AR_ESUBSYSRESET + 1

struct ar_err_code {
    int  lnx_err_code;
    char *ar_err_str;
};

static struct ar_err_code ar_err_code_info[AR_ERR_MAX+1] = {
    { 0, AR_EOK_STR},
    { -ENOTRECOVERABLE, AR_EFAILED_STR},
    { -EINVAL, AR_EBADPARAM_STR},
    { -EOPNOTSUPP, AR_EUNSUPPORTED_STR},
    { -ENOPROTOOPT, AR_EVERSION_STR},
    { -ENOTRECOVERABLE, AR_EUNEXPECTED_STR},
    { -ENOTRECOVERABLE, AR_EPANIC_STR},
    { -ENOSPC, AR_ENORESOURCE_STR},
    { -EBADR, AR_EHANDLE_STR},
    { -EALREADY, AR_EALREADY_STR},
    { -EPERM, AR_ENOTREADY_STR},
    { -EINPROGRESS, AR_EPENDING_STR},
    { -EBUSY, AR_EBUSY_STR},
    { -ECANCELED, AR_EABORTED_STR},
    { -EAGAIN, AR_ECONTINUE_STR},
    { -EAGAIN, AR_EIMMEDIATE_STR},
    { -EAGAIN, AR_ENOTIMPL_STR},
    { -ENODATA, AR_ENEEDMORE_STR},
    { -ENOMEM, AR_ENOMEMORY_STR},
    { -ENODEV, AR_ENOTEXIST_STR},
    { -ENODEV, AR_ETERMINATED_STR},
    { -ETIMEDOUT, AR_ETIMEOUT_STR},
    { -EIO, AR_EIODATA_STR},
    { -ENETRESET, AR_ESUBSYSRESET_STR},
    { -EADV, AR_ERR_MAX_STR},
};

int ar_err_get_lnx_err_code(uint32_t error)
{
    if (error > AR_ERR_MAX)
        return ar_err_code_info[AR_ERR_MAX].lnx_err_code;
    else
        return ar_err_code_info[error].lnx_err_code;
}

char *ar_err_get_err_str(uint32_t error)
{
    if (error > AR_ERR_MAX)
        return ar_err_code_info[AR_ERR_MAX].ar_err_str;
    else
        return ar_err_code_info[error].ar_err_str;
}
