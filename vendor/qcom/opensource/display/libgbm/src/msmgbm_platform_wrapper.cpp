/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
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
 */

#include <cstdio>
#include <cstddef>
#include <pthread.h>
#include "msmgbm_adreno_utils.h"
#include "msmgbm_platform_wrapper.h"
#include "gbm_priv.h"

#define INT(exp) static_cast<int>(exp)
#define UINT(exp) static_cast<unsigned int>(exp)
#define ALIGN(x, align) (((x) + ((align)-1)) & ~((align)-1))
#define ASTC_BLOCK_SIZE 16

bool g_ubwc_disable = false;

namespace msm_gbm {

extern "C" {

static platform_wrap *platform_wrap_ = NULL;
static unsigned int platform_inst_count_ = 0;
static pthread_mutex_t platform_wrap_mutex = PTHREAD_MUTEX_INITIALIZER;


/**
 * Function to create a cpp object of platform wrapper class
 * @return   0 : success
 *           1 : failure
 */

bool platform_wrap_instnce(void) {
    pthread_mutex_lock(&platform_wrap_mutex);
    if (platform_wrap_ == NULL) {
        platform_wrap_ = new platform_wrap();
        if (!platform_wrap_->init()) {
            delete platform_wrap_;
            platform_wrap_ = NULL;
            pthread_mutex_unlock(&platform_wrap_mutex);
            return 1;
       }
    }
    platform_inst_count_++;
    pthread_mutex_unlock(&platform_wrap_mutex);
    return 0;
}

/**
 * C wrapper function to know if the format is UBWC
 */
bool is_ubwc_enbld(int format, int prod_usage,
                              int cons_usage) {
    return platform_wrap_->is_ubwc_enbld(format, prod_usage,prod_usage);
}

/**
 * Function to query the aligned width and hieght from the platform specific object
 * @return    : alignedw
 *            : alignedh
 */
void qry_aligned_wdth_hght(gbm_bufdesc *descriptor, unsigned int *alignedw,
                                                             unsigned int *alignedh) {
    platform_wrap_->get_aligned_wdth_hght(descriptor, alignedw,alignedh);
}

/**
 * Function to query the stride, scanline and size from the platform specific object
 * @return    : stride
 *            : scanline
 *            : size
 */
void qry_stride_scanline_size(gbm_bufdesc *descriptor, unsigned int *stride,
                              unsigned int *scanline, unsigned int *size) {
    platform_wrap_->get_stride_scanline_size(descriptor, stride, scanline, size);
}

/**
 * Function to query size based on format from the platform wrapper
 * @return    : size
 *
 */
unsigned int qry_size(gbm_bufdesc *desc, unsigned int alignedw, unsigned int alignedh){
    return platform_wrap_->get_size(desc->Format, desc->Width, desc->Height, desc->Usage, alignedw,
                                    alignedh);
}

/**
 * Function to check whether the format is UBWC or not from the platform wrapper
 * @params    gbm format
 * @return    boolean 0 (non UBWC format)
 *                    1 (UBWC format)
 *
 */
bool is_valid_ubwc_format( int format) {
   return platform_wrap_->is_valid_ubwc_fmt(format);
}


/**
 * Function to check whether the format is uncompressed RGB or not from the platform wrapper
 * @params    gbm format
 * @return    boolean 0 (compressed RGB format)
 *                    1 (uncompressed RGB format)
 *
 */
bool is_valid_uncmprsd_rgb_format( int format) {
   return platform_wrap_->is_valid_uncmprsd_rgb_fmt(format);
}

/**
 * Function to check whether the format is uncompressed RGB or not from the platform wrapper
 * @params    gbm format
 * @return    boolean 0 (uncompressed RGB format)
 *                    1 (compressed RGB format)
 *
 */
bool is_valid_cmprsd_rgb_format( int format) {
   return platform_wrap_->is_valid_cmprsd_rgb_fmt(format);
}

/**
 * Function to check whether the format is yuv or not from the platform wrapper
 * @params    gbm format
 * @return    boolean 0 (non yuv format)
 *                    1 (yuv format)
 *
 */
bool is_valid_yuv_format( int format) {
   return platform_wrap_->is_valid_yuv_fmt(format);
}

/**
 * Function to check whether the format is RAW or not from the platform wrapper
 * @params    gbm format
 * @return    boolean 0 (non RAW format)
 *                    1 (RAW format)
 *
 */
bool is_valid_raw_format( int format) {
   return platform_wrap_->is_valid_raw_fmt(format);
}

/**
 * Function to return bytes per pixel for a given uncompressed RGB format
 * @params    uncompressed RGB gbm format
 * @return    bytes per pixel
 *
 */
uint32_t get_bpp_for_uncmprsd_rgb_format(int format) {
   return platform_wrap_->get_bpp_for_uncmprsd_rgb_fmt(format);
}


uint32_t get_rgb_ubwc_metabuffer_size(int width, int height, int bpp) {
   return platform_wrap_->get_rgb_ubwc_mb_size(width, height, bpp);
}

/**
 * Function to check whether the format is UBWC or not from the platform wrapper
 * @params    gbm format
 * @params    int prod_usage
 * @params    int cons_usage
 * @return    boolean 0 (non UBWC format)
 *                    1 (UBWC format)
 *
 */
bool is_ubwc_enabled(int format, int prod_usage, int cons_usage) {
   return platform_wrap_->is_ubwc_enbld(format, prod_usage, cons_usage);
}

/**
 * Function to delete platform wrapper object
 * @return  : None
 *
 */
void platform_wrap_deinstnce(void) {
    pthread_mutex_lock(&platform_wrap_mutex);
    if (--platform_inst_count_ != 0) {
        pthread_mutex_unlock(&platform_wrap_mutex);
        return;
    }
    if (platform_wrap_) {
       delete platform_wrap_;
       platform_wrap_ = NULL;
    }
    pthread_mutex_unlock(&platform_wrap_mutex);
}

}

bool cpu_can_accss(int prod_usage, int cons_usage) {
  return cpu_can_wr(prod_usage);
}
bool cpu_can_wr(int prod_usage) {
  if (prod_usage & GBM_BO_USE_WRITE) {
    // Application intends to use CPU for rendering
    return true;
  }
  return false;
}

platform_wrap::platform_wrap() {
}

bool platform_wrap::init() {
  adreno_helper_ = new adreno_mem_info();
  if (!adreno_helper_->init()) {
    delete adreno_helper_;
    adreno_helper_ = NULL;
  }
  if (adreno_helper_) {
    gpu_support_macrotile = adreno_helper_->is_mcro_tiling_supported();
  }
#if 0
  int supports_macrotile = 0;
  qdutils::querySDEInfo(qdutils::HAS_MACRO_TILE, &supports_macrotile);
  display_support_macrotile = !!supports_macrotile;
#endif
  LOG(LOG_DBG,"platform_wrap Success\n");

  return true;
}

platform_wrap::~platform_wrap() {
  if (adreno_helper_) {
    LOG(LOG_DBG,"Destroyer\n");
    delete adreno_helper_;
    adreno_helper_ = NULL;
  }
}

bool is_valid_rgb_fmt(int format)
{
  return (bool)platform_wrap_->is_valid_rgb_fmt(format);
}

bool platform_wrap::is_valid_raw_fmt(int format) {
  switch (format) {
    case GBM_FORMAT_RAW8:
    case GBM_FORMAT_RAW10:
    case GBM_FORMAT_RAW12:
    case GBM_FORMAT_RAW16:
      return true;
    default:
      break;
  }

  return false;
}

int platform_wrap::is_valid_rgb_fmt(int format){
    int is_supported;
    switch(format)
    {
        case GBM_FORMAT_R8:
        case GBM_FORMAT_RG88:
        case GBM_FORMAT_R16:
        case GBM_FORMAT_RG1616:
        case GBM_FORMAT_RGB565:
        case GBM_FORMAT_RGB888:
        case GBM_FORMAT_BGR888:
        case GBM_FORMAT_RGBA8888:
        case GBM_FORMAT_RGBX8888:
        case GBM_FORMAT_XRGB8888:
        case GBM_FORMAT_XBGR8888:
        case GBM_FORMAT_ARGB8888:
        case GBM_FORMAT_ABGR8888:
        case GBM_FORMAT_BGR565:
        case GBM_FORMAT_ABGR2101010:
        case GBM_FORMAT_RGB161616F:
        case GBM_FORMAT_RGB323232F:
        case GBM_FORMAT_RGBA16161616F:
        case GBM_FORMAT_RGBA32323232F:
             is_supported = 1;
             break;
        default:
            is_supported = 0;
    }
    return is_supported;
}

uint32_t platform_wrap::get_bpp_for_uncmprsd_rgb_fmt(int format) {
  uint32_t bpp = 0;
  switch (format) {
    case GBM_FORMAT_RGBA32323232F:
      bpp = 16;
      break;
    case GBM_FORMAT_RGB323232F:
      bpp = 12;
      break;
    case GBM_FORMAT_RGBA16161616F:
      bpp = 8;
      break;
    case GBM_FORMAT_RGB161616F:
      bpp = 6;
      break;
    case GBM_FORMAT_RG1616:
    case GBM_FORMAT_RGBA8888:
    case GBM_FORMAT_RGBX8888:
    case GBM_FORMAT_BGRA8888:
    case GBM_FORMAT_BGRX8888:
    case GBM_FORMAT_XRGB8888:
    case GBM_FORMAT_XBGR8888:
    case GBM_FORMAT_ARGB8888:
    case GBM_FORMAT_ABGR8888:
    case GBM_FORMAT_RGBA1010102:
    case GBM_FORMAT_ARGB2101010:
    case GBM_FORMAT_RGBX1010102:
    case GBM_FORMAT_XRGB2101010:
    case GBM_FORMAT_BGRA1010102:
    case GBM_FORMAT_ABGR2101010:
    case GBM_FORMAT_BGRX1010102:
    case GBM_FORMAT_XBGR2101010:
      bpp = 4;
      break;
    case GBM_FORMAT_RGB888:
    case GBM_FORMAT_BGR888:
      bpp = 3;
      break;
    case GBM_FORMAT_RG88:
    case GBM_FORMAT_R16:
    case GBM_FORMAT_RGB565:
    case GBM_FORMAT_BGR565:
    case GBM_FORMAT_RGBA5551:
    case GBM_FORMAT_RGBA4444:
      bpp = 2;
      break;
    case GBM_FORMAT_R8:
      bpp = 1;
    default:
      LOG(LOG_ERR," New format request\n");
      break;
  }
  return bpp;
}

bool platform_wrap:: is_valid_cmprsd_rgb_fmt(int format) {
  switch (format) {
    case GBM_FORMAT_COMPRESSED_RGBA_ASTC_4x4_KHR:
    case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
    case GBM_FORMAT_COMPRESSED_RGBA_ASTC_5x4_KHR:
    case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
    case GBM_FORMAT_COMPRESSED_RGBA_ASTC_5x5_KHR:
    case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
    case GBM_FORMAT_COMPRESSED_RGBA_ASTC_6x5_KHR:
    case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
    case GBM_FORMAT_COMPRESSED_RGBA_ASTC_6x6_KHR:
    case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
    case GBM_FORMAT_COMPRESSED_RGBA_ASTC_8x5_KHR:
    case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
    case GBM_FORMAT_COMPRESSED_RGBA_ASTC_8x6_KHR:
    case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
    case GBM_FORMAT_COMPRESSED_RGBA_ASTC_8x8_KHR:
    case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
    case GBM_FORMAT_COMPRESSED_RGBA_ASTC_10x5_KHR:
    case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
    case GBM_FORMAT_COMPRESSED_RGBA_ASTC_10x6_KHR:
    case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
    case GBM_FORMAT_COMPRESSED_RGBA_ASTC_10x8_KHR:
    case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
    case GBM_FORMAT_COMPRESSED_RGBA_ASTC_10x10_KHR:
    case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
    case GBM_FORMAT_COMPRESSED_RGBA_ASTC_12x10_KHR:
    case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
    case GBM_FORMAT_COMPRESSED_RGBA_ASTC_12x12_KHR:
    case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
      return true;
    default:
      break;
  }

  return false;
}

/* Not currently supported formats
 case HAL_PIXEL_FORMAT_R_8:
 case HAL_PIXEL_FORMAT_RG_88:
*/
bool platform_wrap::is_valid_uncmprsd_rgb_fmt(int format) {
  switch (format) {
    case GBM_FORMAT_R8:
    case GBM_FORMAT_RG88:
    case GBM_FORMAT_R16:
    case GBM_FORMAT_RG1616:
    case GBM_FORMAT_XRGB8888:
    case GBM_FORMAT_XBGR8888:
    case GBM_FORMAT_ARGB8888:
    case GBM_FORMAT_ABGR8888:
    case GBM_FORMAT_RGBA8888:
    case GBM_FORMAT_RGBX8888:
    case GBM_FORMAT_RGB888:
    case GBM_FORMAT_BGR888:
    case GBM_FORMAT_RGB565:
    case GBM_FORMAT_BGR565:
    case GBM_FORMAT_BGRA8888:
    case GBM_FORMAT_RGBA5551:
    case GBM_FORMAT_RGBA4444:
    case GBM_FORMAT_BGRX8888:
    case GBM_FORMAT_RGBA1010102:
    case GBM_FORMAT_ARGB2101010:
    case GBM_FORMAT_RGBX1010102:
    case GBM_FORMAT_XRGB2101010:
    case GBM_FORMAT_BGRA1010102:
    case GBM_FORMAT_ABGR2101010:
    case GBM_FORMAT_BGRX1010102:
    case GBM_FORMAT_XBGR2101010:
    case GBM_FORMAT_RGB161616F:
    case GBM_FORMAT_RGB323232F:
    case GBM_FORMAT_RGBA16161616F:
    case GBM_FORMAT_RGBA32323232F:
      return true;
    default:
      break;
  }

  return false;
}

bool platform_wrap:: is_valid_yuv_fmt(int format) {
  switch (format) {
        case GBM_FORMAT_YCbCr_420_SP:
        case GBM_FORMAT_YCrCb_420_SP:
        case GBM_FORMAT_YCbCr_420_SP_VENUS:
        case GBM_FORMAT_NV12_ENCODEABLE: //Same as YCbCr_420_SP_VENUS
#ifdef COLOR_FMT_NV12_512
        case GBM_FORMAT_NV12_HEIF:
#endif
        case GBM_FORMAT_NV12:
        case GBM_FORMAT_NV21_ZSL:
        case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
        case GBM_FORMAT_YCbCr_420_TP10_UBWC:
        case GBM_FORMAT_YCbCr_420_P010_UBWC:
        case GBM_FORMAT_P010:
        case GBM_FORMAT_YCbCr_420_P010_VENUS:
        case GBM_FORMAT_YCbCr_422_I:
        case GBM_FORMAT_YCrCb_422_I:
        case GBM_FORMAT_C8:
        case GBM_FORMAT_UYVY:
          return true;
        default:
          break;
  }

  return false;
}

// helper function
unsigned int platform_wrap::get_size(int format, int width, int height, int usage,
                                        int alignedw, int alignedh) {


   int prod_usage=usage;
   int cons_usage=usage;


    if (is_ubwc_enbld(format, prod_usage,cons_usage)) {
        return get_ubwc_size(width, height, format, alignedw, alignedh);
    }


    unsigned int size = 0;


    switch (format) {
        case GBM_FORMAT_RG1616:
        case GBM_FORMAT_RGBA8888:
        case GBM_FORMAT_RGBX8888:
        case GBM_FORMAT_BGRX8888:
        case GBM_FORMAT_BGRA8888:
        case GBM_FORMAT_XRGB8888:
        case GBM_FORMAT_XBGR8888:
        case GBM_FORMAT_ARGB8888:
        case GBM_FORMAT_ABGR8888:
        case GBM_FORMAT_RGBA1010102:
        case GBM_FORMAT_ARGB2101010:
        case GBM_FORMAT_RGBX1010102:
        case GBM_FORMAT_XRGB2101010:
        case GBM_FORMAT_BGRA1010102:
        case GBM_FORMAT_ABGR2101010:
        case GBM_FORMAT_BGRX1010102:
        case GBM_FORMAT_XBGR2101010:
            size = alignedw * alignedh * 4;
            break;
        case GBM_FORMAT_RGB888:
        case GBM_FORMAT_BGR888:
            size = alignedw * alignedh * 3;
            break;
        case GBM_FORMAT_RG88:
        case GBM_FORMAT_R16:
        case GBM_FORMAT_RGB565:
        case GBM_FORMAT_BGR565:
        case GBM_FORMAT_RGBA5551:
        case GBM_FORMAT_RGBA4444:
            size = alignedw * alignedh * 2;
            break;
        case GBM_FORMAT_R8:
        case GBM_FORMAT_RAW8:
            size = alignedw * alignedh * 1;
            break;
        case GBM_FORMAT_RAW10:
            size = ALIGN(alignedw * alignedh * 10 / 8, 4096);
            break;
        case GBM_FORMAT_RAW12:
            size = ALIGN(alignedw * alignedh * 12 / 8, 4096);
            break;
        case GBM_FORMAT_RAW16:
            size = ALIGN(alignedw * alignedh * 2, 4096);
            break;
        case GBM_FORMAT_YV12:
            if ((format == GBM_FORMAT_YV12) && ((width&1) || (height&1))) {
                LOG(LOG_ERR," w or h is odd for the YV12 format\n");
                return 0;
            }
            size = alignedw*alignedh + (ALIGN(alignedw/2, 16) * (alignedh/2))*2;
            size = ALIGN(size, (unsigned int)4096);
            break;
        case GBM_FORMAT_YCbCr_420_SP:
        case GBM_FORMAT_YCrCb_420_SP:
            size = ALIGN((alignedw*alignedh) + (alignedw* alignedh)/2 + 1, 4096);
            break;
        case GBM_FORMAT_YCbCr_422_SP:
        case GBM_FORMAT_YCrCb_422_SP:
        case GBM_FORMAT_UYVY:
            if (width & 1) {
                LOG(LOG_ERR," width is odd for the YUV422_SP format\n");
                return 0;
            }
            size = ALIGN(alignedw * alignedh * 2, 4096);
            break;
        case GBM_FORMAT_YCbCr_422_I:
        case GBM_FORMAT_YCrCb_422_I:
            size = ALIGN(alignedw * alignedh, 4096);
            break;
        case GBM_FORMAT_YCbCr_420_SP_VENUS:
        case GBM_FORMAT_NV12_ENCODEABLE:
        case GBM_FORMAT_NV12:
#ifdef ENABLE_CAM_NV12_128
            size = MMM_COLOR_FMT_BUFFER_SIZE(MMM_COLOR_FMT_NV12_128, width, height);
#else
            size = MMM_COLOR_FMT_BUFFER_SIZE(MMM_COLOR_FMT_NV12, width, height);
#endif
            LOG(LOG_INFO," MMM_COLOR_FMT_BUF_SIZE=%u, computed for Width=%u, Height=%u\n",
                                  size, width, height);
            break;
#ifdef MMM_COLOR_FMT_NV12_512
        case GBM_FORMAT_NV12_HEIF:
            size = MMM_COLOR_FMT_BUFFER_SIZE(MMM_COLOR_FMT_NV12_512, width, height);
            LOG(LOG_INFO," MMM_COLOR_FMT_BUF_SIZE=%u, computed for Width=%u, Height=%u\n",
                                  size, width, height);
            break;
#endif
        case GBM_FORMAT_YCrCb_420_SP_VENUS:
            size = MMM_COLOR_FMT_BUFFER_SIZE(MMM_COLOR_FMT_NV21, width, height);
            break;
        case GBM_FORMAT_YCbCr_420_TP10_UBWC:
            // The macro returns the stride which is 4/3 times the width, hence * 3/4
            //*aligned_w = (MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_NV12_BPP10_UBWC, width) * 3) / 4;
            //*aligned_h = MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_NV12_BPP10_UBWC, height);
            size = MMM_COLOR_FMT_BUFFER_SIZE(MMM_COLOR_FMT_NV12_BPP10_UBWC, width, height);
            LOG(LOG_INFO," MMM_COLOR_FMT_BUF_SIZE=%u, computed for Width=%u, Height=%u\n",
                                  size, width, height);
            break;
        case GBM_FORMAT_YCbCr_420_P010_VENUS:
            size = MMM_COLOR_FMT_BUFFER_SIZE(MMM_COLOR_FMT_P010, width, height);
            break;
        case GBM_FORMAT_P010:
            size = ALIGN((alignedw * alignedh * 2) + (alignedw * alignedh) + 1, 4096);
            break;
        case GBM_FORMAT_BLOB:
        case GBM_FORMAT_RAW_OPAQUE:
            if(height != 1) {
                LOG(LOG_ERR," Buffers with format HAL_PIXEL_FORMAT_BLOB \
                                        must have height==1\n");
                return 0;
            }
            size = width;
            break;
        case GBM_FORMAT_NV21_ZSL:
            size = ALIGN((alignedw*alignedh) + (alignedw* alignedh)/2, 4096);
            break;
        case GBM_FORMAT_COMPRESSED_RGBA_ASTC_4x4_KHR:
        case GBM_FORMAT_COMPRESSED_RGBA_ASTC_5x4_KHR:
        case GBM_FORMAT_COMPRESSED_RGBA_ASTC_5x5_KHR:
        case GBM_FORMAT_COMPRESSED_RGBA_ASTC_6x5_KHR:
        case GBM_FORMAT_COMPRESSED_RGBA_ASTC_6x6_KHR:
        case GBM_FORMAT_COMPRESSED_RGBA_ASTC_8x5_KHR:
        case GBM_FORMAT_COMPRESSED_RGBA_ASTC_8x6_KHR:
        case GBM_FORMAT_COMPRESSED_RGBA_ASTC_8x8_KHR:
        case GBM_FORMAT_COMPRESSED_RGBA_ASTC_10x5_KHR:
        case GBM_FORMAT_COMPRESSED_RGBA_ASTC_10x6_KHR:
        case GBM_FORMAT_COMPRESSED_RGBA_ASTC_10x8_KHR:
        case GBM_FORMAT_COMPRESSED_RGBA_ASTC_10x10_KHR:
        case GBM_FORMAT_COMPRESSED_RGBA_ASTC_12x10_KHR:
        case GBM_FORMAT_COMPRESSED_RGBA_ASTC_12x12_KHR:
        case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
        case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
        case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
        case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
        case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
        case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
        case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
        case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
        case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
        case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
        case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
        case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
        case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
        case GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
            size = alignedw * alignedh * ASTC_BLOCK_SIZE;
            break;
        case GBM_FORMAT_RGB161616F:
            size = alignedw * alignedh * 6;
            break;
        case GBM_FORMAT_RGBA16161616F:
            size = alignedw * alignedh * 8;
            break;
        case GBM_FORMAT_RGB323232F:
            size = alignedw * alignedh * 12;
            break;
        case GBM_FORMAT_RGBA32323232F:
            size = alignedw * alignedh * 16;
            break;
        case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
            size = MMM_COLOR_FMT_BUFFER_SIZE(MMM_COLOR_FMT_NV12_UBWC, width, height);
            break;
        default:
            LOG(LOG_ERR," Unrecognized pixel format: 0x%x\n",format);
            return 0;
    }


    return size;
}

void platform_wrap::get_yuv_ubwc_wdth_hght(int width, int height, int format,
                                         unsigned int *aligned_w,unsigned int *aligned_h) {

  switch (format) {
    case GBM_FORMAT_NV12:
    case GBM_FORMAT_NV12_ENCODEABLE:
    case GBM_FORMAT_YCbCr_420_SP_VENUS:
    case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
      *aligned_w = MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_NV12_UBWC, width);
      *aligned_h = MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_NV12_UBWC, height);
      break;
    case GBM_FORMAT_YCbCr_420_TP10_UBWC:
      // The macro returns the stride which is 4/3 times the width, hence * 3/4
      *aligned_w = (MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_NV12_BPP10_UBWC, width) * 3) / 4;
      *aligned_h = MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_NV12_BPP10_UBWC, height);
      break;
    case GBM_FORMAT_YCbCr_420_P010_UBWC:
       // The macro returns the stride which is 2 times the width, hence / 2
       *aligned_w = (MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_P010_UBWC, width) / 2);
       *aligned_h = MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_P010_UBWC, height);
    break;
    default:
      LOG(LOG_ERR," Unsupported pixel format: 0x%x\n",format);
      *aligned_w = 0;
      *aligned_h = 0;
      break;
  }

}

void platform_wrap::get_aligned_wdth_hght(gbm_bufdesc *descriptor, unsigned int *alignedw,
                                                   unsigned int *alignedh) {
  unsigned int width = descriptor->Width;
  unsigned int height = descriptor->Height;
  unsigned int format = descriptor->Format;
  unsigned int prod_usage = descriptor->Usage;
  unsigned int cons_usage = descriptor->Usage;
  unsigned int alignment = 32;
  bool ubwc_enabled = false;
  int tile = 0;

  LOG(LOG_DBG,"width=%d, height=%d,format=%d, usage=%d\n",width,height,
                                                     format,prod_usage);

  // Currently surface padding is only computed for RGB* surfaces.
  ubwc_enabled = is_ubwc_enbld(format, prod_usage, cons_usage);

  LOG(LOG_DBG,"ubwc_enabled=%d, tile=%d \n",ubwc_enabled,tile);

  tile = ubwc_enabled;
  // initialize aligned to actual
  *alignedw = width;
  *alignedh = height;

  LOG(LOG_DBG,"ubwc_enabled=%d, tile=%d \n",ubwc_enabled,tile);

  if (is_valid_uncmprsd_rgb_fmt(format)) {
    if (adreno_helper_) {
      adreno_helper_->get_aligned_wdth_hght_uncmprsd_rgb_fmt(width, height, format, tile, alignedw, alignedh);
    }
  }

  if (ubwc_enabled && !is_valid_rgb_fmt(format))
    get_yuv_ubwc_wdth_hght(width, height, format, alignedw, alignedh);

  if (is_valid_cmprsd_rgb_fmt(format)) {
    if (adreno_helper_) {
       adreno_helper_->get_aligned_wdth_hght_cmprsd_rgb_fmt(width, height, format, alignedw, alignedh);
    }
  }

  // Below should be only YUV family
  switch (format) {
    case GBM_FORMAT_YCrCb_420_SP:
    case GBM_FORMAT_YCbCr_420_SP:
      if (adreno_helper_) {
         alignment = adreno_helper_->get_gpu_pxl_align();
      }
      *alignedw = ALIGN(width, alignment);
      break;
    case GBM_FORMAT_RAW16:
    case GBM_FORMAT_RAW12:
    case GBM_FORMAT_RAW10:
    case GBM_FORMAT_RAW8:
      *alignedw = ALIGN(width, 16);
      break;
    case GBM_FORMAT_YV12:
    case GBM_FORMAT_YCbCr_422_SP:
    case GBM_FORMAT_YCrCb_422_SP:
    case GBM_FORMAT_P010:
      *alignedw = ALIGN(width, 16);
      *alignedh = height;
      break;
    case GBM_FORMAT_YCbCr_422_I:
    case GBM_FORMAT_YCrCb_422_I:
      *alignedw = ALIGN(width * 2, 16);
      *alignedh = height;
      break;
    case GBM_FORMAT_UYVY:
      *alignedw = ALIGN(width, 32);
      break;
    case GBM_FORMAT_YCbCr_420_P010_VENUS:
      *alignedw = INT(MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_P010, width) / 2);
      *alignedh = INT(MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_P010, height));
      break;
    case GBM_FORMAT_YCbCr_420_SP_VENUS:
    case GBM_FORMAT_NV12_ENCODEABLE:
      LOG(LOG_DBG,"@ YUV Format\n");
#ifdef ENABLE_CAM_NV12_128
      *alignedw = INT(MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_NV12_128, width));
      *alignedh = INT(MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_NV12_128, height));
#else
      *alignedw = INT(MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_NV12, width));
      *alignedh = INT(MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_NV12, height));
#endif
      break;
    case GBM_FORMAT_NV12:
      if (ubwc_enabled) {
          *alignedw = INT(MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_NV12_UBWC, width));
          *alignedh = INT(MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_NV12_UBWC, height));
      } else {
#ifdef ENABLE_CAM_NV12_128
          *alignedw = INT(MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_NV12_128, width));
          *alignedh = INT(MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_NV12_128, height));
#else
          *alignedw = INT(MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_NV12, width));
          *alignedh = INT(MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_NV12, height));
#endif
      }
      break;
    case GBM_FORMAT_YCrCb_420_SP_VENUS:
      *alignedw = INT(MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_NV21, width));
      *alignedh = INT(MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_NV21, height));
      break;
    case GBM_FORMAT_YCbCr_420_TP10_UBWC:
      // The macro returns the stride which is 4/3 times the width, hence * 3/4
      *alignedw = (MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_NV12_BPP10_UBWC, width) * 3) / 4;
      *alignedh = MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_NV12_BPP10_UBWC, height);
      break;
    case GBM_FORMAT_BLOB:
    case GBM_FORMAT_RAW_OPAQUE:
      break;
    case GBM_FORMAT_NV21_ZSL:
#ifdef ENABLE_CAM_MIMAS
      *alignedw = INT(MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_NV12, width));
      *alignedh = INT(MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_NV12, height));
#else
      *alignedw = ALIGN(width, 64);
      *alignedh = ALIGN(height, 64);
#endif
      break;
#ifdef MMM_COLOR_FMT_NV12_512
    case GBM_FORMAT_NV12_HEIF:
      *alignedw = INT(MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_NV12_512, width));
      *alignedh = INT(MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_NV12_512, height));
      break;
#endif
    case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
      *alignedw = MMM_COLOR_FMT_Y_STRIDE(MMM_COLOR_FMT_NV12_UBWC, width);
      *alignedh = MMM_COLOR_FMT_Y_SCANLINES(MMM_COLOR_FMT_NV12_UBWC, height);
      break;
    default:
      break;
  }

  LOG(LOG_DBG,"alignedw=%d, alignedh=%d \n",*alignedw,*alignedh);

}

void platform_wrap::get_stride_scanline_size(gbm_bufdesc *descriptor, unsigned int *stride,
                                             unsigned int *scanline, unsigned int *size) {
  unsigned int width = descriptor->Width;
  unsigned int height = descriptor->Height;
  unsigned int format = descriptor->Format;
  unsigned int usage = descriptor->Usage;
  unsigned int alignedw = 0, alignedh = 0;

  // Get the aligned width and height in pixels.
  get_aligned_wdth_hght(descriptor, &alignedw, &alignedh);

  // Calculate the size wusing the returned aligned width and height.
  *size = get_size(format, width, height, usage, alignedw, alignedh);

  *stride = alignedw;
  *scanline = alignedh;

  if (is_valid_uncmprsd_rgb_fmt(format)) {
    uint32_t bpp = get_bpp_for_uncmprsd_rgb_fmt(format);
    *stride = alignedw * bpp;
  }

  // Adjust the actual stride and scanline using the aligned width and height.
  switch (format) {
    case GBM_FORMAT_RAW10:
      *stride = (alignedw * 10) / 8;
      break;
    case GBM_FORMAT_RAW12:
      *stride = (alignedw * 12) / 8;
      break;
    case GBM_FORMAT_RAW16:
      *stride = alignedw * 2;
      break;
    case GBM_FORMAT_YCbCr_422_I:
    case GBM_FORMAT_YCrCb_422_I:
      *stride = alignedw * 2;
      break;
    case GBM_FORMAT_P010:
    case GBM_FORMAT_YCbCr_420_P010_VENUS:
      *stride = alignedw * 2;
      break;
    case GBM_FORMAT_YCbCr_420_P010_UBWC:
      *stride = alignedw * 2;
      break;
    case GBM_FORMAT_YCbCr_420_TP10_UBWC:
      *stride = (alignedw * 4) / 3;
      break;
    default:
      break;
  }

  LOG(LOG_DBG,"stride=%u, scanline=%u, size=%u \n", *stride, *scanline, *size);
}

// Explicitly defined UBWC formats Typically used for Video formats.
bool platform_wrap::is_valid_ubwc_fmt(int format) {
  switch (format) {
    case GBM_FORMAT_YCbCr_420_TP10_UBWC:
    case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
    case GBM_FORMAT_YCbCr_420_P010_UBWC:
      return true;
    default:
      return false;
  }
}

bool platform_wrap::is_ubwc_support_enbld(int format) {
  // Existing HAL formats with UBWC support
  switch (format) {
    case GBM_FORMAT_BGR565:
    case GBM_FORMAT_XBGR8888:
    case GBM_FORMAT_ABGR8888:
    case GBM_FORMAT_ABGR2101010:
    case GBM_FORMAT_RGBA8888:
    case GBM_FORMAT_RGBX8888:
      return true;
    default:
      break;
  }

  return false;
}

bool platform_wrap::is_ubwc_enbld(int format, int prod_usage,
                              int cons_usage) {
    // debug flag to disable UBWC (useful for troubleshooting)
    if (g_ubwc_disable) {
        LOG(LOG_INFO,"UBWC feature is disabled\n");
        return false;
    }

    // Allow UBWC, if client is using an explicitly defined UBWC pixel format.
    if (is_valid_ubwc_fmt(format)) {
        return true;
    }

    // Allow UBWC for NV12, if client usage flags is set to GBM_BO_USAGE_UBWC_ALIGNED_QTI
    if ((prod_usage & GBM_BO_USAGE_UBWC_ALIGNED_QTI) && (format == GBM_FORMAT_NV12))
        return true;

    // Allow UBWC, if an OpenGL client sets UBWC usage flag and GPU plus MDP
    // support the format. OR if a non-OpenGL client like Rotator, sets UBWC
    // usage flag and MDP supports the format.
    if ((prod_usage & GBM_BO_USAGE_UBWC_ALIGNED_QTI) && is_ubwc_support_enbld(format)) {
        bool enable = true;

        // Query GPU for UBWC only if buffer is intended to be used by GPU.
        if ((cons_usage & GBM_BO_USAGE_HW_RENDERING_QTI) ||
            (prod_usage & GBM_BO_USAGE_HW_RENDERING_QTI)) {
            if (adreno_helper_) {
              enable = adreno_helper_->is_ubwc_supported_gpu(format);
            }
        }

        // Allow UBWC, only if CPU usage flags are not set
        if (enable && !(cpu_can_accss(prod_usage, cons_usage))) {
            return true;
        }
    }

    return false;
}
// TODO (user) : check for other formats with mcro-tile
bool platform_wrap::is_mcro_tile_enbld(int format, int prod_usage, int cons_usage) {
  bool tile_enabled = false;

  // Check whether GPU & MDSS supports MacroTiling feature
  if (adreno_helper_ && !adreno_helper_->is_mcro_tiling_supported() || !display_support_macrotile) {
    return tile_enabled;
  }

  LOG(LOG_DBG," is_mcro_tiling_supported() success\n");

  // check the format
  switch (format) {
    case GBM_FORMAT_RGBA8888:
    case GBM_FORMAT_RGBX8888:
    case GBM_FORMAT_BGRA8888:
    case GBM_FORMAT_RGB565:
    case GBM_FORMAT_BGR565:
      if (!cpu_can_accss(prod_usage, cons_usage)) {
        // not touched by CPU
        tile_enabled = true;
      }
      break;
    default:
      break;
  }

  return tile_enabled;
}

void platform_wrap::get_rgb_ubwc_blk_size(uint32_t bpp, int *block_width, int *block_height) {
  *block_width = 0;
  *block_height = 0;

  switch (bpp) {
    case 2:
    case 4:
      *block_width = 16;
      *block_height = 4;
      break;
    case 8:
      *block_width = 8;
      *block_height = 4;
      break;
    case 16:
      *block_width = 4;
      *block_height = 4;
      break;
    default:
      LOG(LOG_ERR," Unsupported bpp: %d\n",bpp);
      break;
  }
}

unsigned int platform_wrap::get_rgb_ubwc_mb_size(int width, int height, uint32_t bpp) {
  unsigned int size = 0;
  int meta_width, meta_height;
  int block_width, block_height;

  get_rgb_ubwc_blk_size(bpp, &block_width, &block_height);
  if (!block_width || !block_height) {
    LOG(LOG_ERR," Unsupported bpp: %d\n",bpp);
    return size;
  }

  // Align meta buffer height to 16 blocks
  meta_height = ALIGN(((height + block_height - 1) / block_height), 16);

  // Align meta buffer width to 64 blocks
  meta_width = ALIGN(((width + block_width - 1) / block_width), 64);

  // Align meta buffer size to 4K
  size = (unsigned int)ALIGN((meta_width * meta_height), 4096);

  return size;
}

unsigned int platform_wrap::get_ubwc_size(int width, int height, int format, unsigned int alignedw,
                                                unsigned int alignedh) {
  unsigned int size = 0;
  uint32_t bpp = 0;
  switch (format) {
    case GBM_FORMAT_BGR565:
    case GBM_FORMAT_XBGR8888:
    case GBM_FORMAT_ABGR8888:
    case GBM_FORMAT_ABGR2101010:
      bpp = get_bpp_for_uncmprsd_rgb_fmt(format);
      size = alignedw * alignedh * bpp;
      size += get_rgb_ubwc_mb_size(width, height, bpp);
      break;
    case GBM_FORMAT_NV12_ENCODEABLE:
    case GBM_FORMAT_NV12:
    case GBM_FORMAT_YCbCr_420_SP_VENUS:
    case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
      size = MMM_COLOR_FMT_BUFFER_SIZE(MMM_COLOR_FMT_NV12_UBWC, width, height);
      break;
    case GBM_FORMAT_YCbCr_420_TP10_UBWC:
      size = MMM_COLOR_FMT_BUFFER_SIZE(MMM_COLOR_FMT_NV12_BPP10_UBWC, width, height);
      break;
    case GBM_FORMAT_YCbCr_420_P010_UBWC:
      size = MMM_COLOR_FMT_BUFFER_SIZE(MMM_COLOR_FMT_P010_UBWC, width, height);
      break;
    default:
      LOG(LOG_ERR," Unsupported pixel format: 0x%x\n",format);
      break;
  }

  return size;
}

}  // namespace msm_gbm
