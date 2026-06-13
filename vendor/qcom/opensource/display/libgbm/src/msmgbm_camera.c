/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* Not a Contribution.
*
* Copyright (c) 2018, 2021 The Linux Foundation. All rights reserved.
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

#include <stdint.h>
#include <stddef.h>
#include <linux/msm_ion.h>
#include <linux/ion.h>
#include <gbm_priv.h>
#include <msmgbm.h>
#include <msmgbm_common.h>

/**
 * Get the implementaion defined format based on usage flags.
 * @return - pixel_format to be used for BO
 *           returns input format if none of the combination matches.
 */
uint32_t GetCameraImplDefinedFormat(uint32_t usage_flags, uint32_t format)
{
    uint32_t pixel_format = format;

    if((usage_flags & GBM_BO_USAGE_CAMERA_READ_QTI) &&
           (usage_flags & GBM_BO_USAGE_CAMERA_WRITE_QTI)){
        pixel_format = GBM_FORMAT_NV21_ZSL;
    }else if(usage_flags & GBM_BO_USAGE_CAMERA_READ_QTI){
        pixel_format = GBM_FORMAT_YCrCb_420_SP;
    }else if(usage_flags & GBM_BO_USAGE_CAMERA_WRITE_QTI){
        if(format == GBM_FORMAT_YCbCr_420_888){
            pixel_format = GBM_FORMAT_NV21_ZSL;
        }else{
            pixel_format = GBM_FORMAT_YCbCr_420_SP_VENUS;
        }
    }else if(usage_flags & GBM_BO_USAGE_HW_COMPOSER_QTI){
        if (GBM_FORMAT_IMPLEMENTATION_DEFINED == format) {
            pixel_format = GBM_FORMAT_NV12_ENCODEABLE;
        } else {
            pixel_format = GBM_FORMAT_RGBA8888;
        }
    }

    return pixel_format;
}

/**
 * Get the ion allocation flags based on allocation flags.
 * @return - ion flags for BO allocation
 */
uint32_t GetCameraIonAllocFlags(uint32_t alloc_flags)
{
    uint32_t ion_flags = 0;

    if((alloc_flags & GBM_BO_USAGE_PROTECTED_QTI)
                    && (alloc_flags & GBM_BO_USAGE_CAMERA_WRITE_QTI)){
        if(alloc_flags & GBM_BO_USAGE_HW_COMPOSER_QTI)
            ion_flags |= ION_FLAG_CP_CAMERA_PREVIEW;
        else
            ion_flags |= ION_FLAG_CP_CAMERA;
    }

    return ion_flags;
}

/**
 * Get the ion heap id based on allocation flags.
 * @return - ion heap id for BO allocation
 */
uint32_t GetCameraIonHeapId(uint32_t alloc_flags)
{
    uint32_t ion_heap_id = 0;

    if((alloc_flags & GBM_BO_USAGE_PROTECTED_QTI) &&
                alloc_flags & GBM_BO_USAGE_CAMERA_WRITE_QTI){
            ion_heap_id = ION_HEAP(ION_SECURE_DISPLAY_HEAP_ID);
    }

    return ion_heap_id;
}

