/*
 * Copyright (c) 2017, 2021 The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef __MSMGBM_PFM_WRP_H__
#define __MSMGBM_PFM_WRP_H__

#include "msmgbm_adreno_utils.h"
#include "gbm_priv.h"
#include "msmgbm.h"
#include <linux/version.h>

#ifdef VENUS_COLOR_FORMAT
#include <display/media/mmm_color_fmt.h>
#else
#define VENUS_Y_STRIDE(args...) 0
#define VENUS_Y_SCANLINES(args...) 0
#define VENUS_BUFFER_SIZE(args...) 0
#endif

namespace msm_gbm {
/**
 * Function to query if CPU access usage flags are set
 * @return    : true/false
 *
 */
bool cpu_can_accss(int prod_usage, int cons_usage);
/**
 * Helper function to query if CPU access usage flags are set
 * @return    : true/false
 *
 */
bool cpu_can_rd(int prod_usage, int cons_usage);
/**
 * Helper function to query if CPU access usage flags are set
 * @return    : true/false
 *
 */
bool cpu_can_wr(int prod_usage);
/**
 * Function to query bpp depending on format
 * @return    : bpp value
 *
 */
uint32_t get_bpp_for_uncmprsd_rgb_fmt(int format) ;

class platform_wrap {
 public:
  platform_wrap();
  ~platform_wrap();

  /**
   * Function to Initialize the platform wrapper object
   * @return   true/
   *           false
   */
  bool init(void);
  /**
   * Function to query UBWC support
   * @return   true/
   *           false
   */
  bool is_ubwc_enbld(int format, int prod_usage, int cons_usage);
  /**
   * Function to query MacroTile support
   * @return   true/
   *           false
   */
  bool is_mcro_tile_enbld(int format, int prod_usage, int cons_usage);
  /**
   * Function to query the UBWC Enabled Buffer Size
   * @return   size on success
   *           0 on fail
   */
  unsigned int get_ubwc_size(int width, int height, int format, unsigned int alignedw, unsigned int alignedh);
  /**
   * Helper Function to query the UBWC Enabled Buffer Size
   * @return   size on success
   *           0 : fail
   */
  unsigned int get_rgb_ubwc_mb_size(int width, int height, uint32_t bpp);
  /**
   * Helper Function to query the UBWC Enabled Buffer Size
   * @return   block width and height
   */
  void get_rgb_ubwc_blk_size(uint32_t bpp, int *block_width, int *block_height);
  /**
   * Function to query the RGB format support
   * @return   true : success
   *           false : fail
   */
  int is_valid_rgb_fmt(int gbm_format);

  /**
   * Function to check whether the format is RAW
   * @params    gbm format
   * @return    true : success
   *            false : fail
   *
   */
  bool is_valid_raw_fmt(int format);

  uint32_t get_bpp_for_uncmprsd_rgb_fmt(int format);

  /**
   * Function to check whether the format is uncompressed RGB
   * @params    gbm format
   * @return    boolean 0 (compressed RGB format)
   *                    1 (uncompressed RGB format)
   *
   */
  bool is_valid_uncmprsd_rgb_fmt(int format);

  /**
   * Function to check whether the format is uncompressed RGB
   * @params    gbm format
   * @return    boolean 0 (uncompressed RGB format)
   *                    1 (compressed RGB format)
   *
   */
  bool is_valid_cmprsd_rgb_fmt(int format);

  /**
   * Function to check whether the format is yuv format
   * @params    gbm format
   * @return   true : success
   *           false : fail
   *
   */
  bool is_valid_yuv_fmt(int format);

  /**
   * Function to query UBWC feature support
   * @return   true : success
   *           false : fail
   */
  bool is_ubwc_support_enbld(int format);
  /**
   * Function to query UBWC enabled format support
   * @return   true : success
   *           false : fail
   */
  bool is_valid_ubwc_fmt(int format);
  /**
   * Function to get  aligned width and height depending on the underlying GPU/Video platform
   * @return    aligned_w
   *            aligned_h
   */
  void get_aligned_wdth_hght(gbm_bufdesc *descriptor, unsigned int *aligned_w,
                                      unsigned int *aligned_h);

  /**
   * Function to get stride, scanline and size depending on the underlying GPU/Video platform
   * @return    stride
   *            scanline
   *            size
   */
  void get_stride_scanline_size(gbm_bufdesc *descriptor, unsigned int *stride,
                                unsigned int *scanline, unsigned int *size);

  /**
   * Function to get  size aligned width and height depending on the underlying GPU/Video platform
   * @params    gbm format
   *            width of the buffer
   *            height of the buffer
   *            usage flags
   *            aligned height
   *            aligned weight
   * @return    size of the buffer
   */
   unsigned int get_size(int format, int width, int height, int usage,
                             int alignedw, int alignedh);

  /**
   * Function to get  aligned width and height for YUV format
   * @params    gbm format
   *            width of the buffer
   *            height of the buffer
   * @return    aligned height
   *            aligned weight
   *
   */
    void get_yuv_ubwc_wdth_hght(int width, int height, int format,
                                          unsigned int *aligned_w, unsigned int *aligned_h);

 private:
  bool gpu_support_macrotile = false;
  bool display_support_macrotile = false;
  adreno_mem_info *adreno_helper_ = NULL;

};

extern "C" {
    /**
    * C wrapper Function to get  aligned width and height depending on the underlying GPU/Video platform
    * @return    aligned_w
    *            aligned_h
    */
    void qry_aligned_wdth_hght(gbm_bufdesc *descriptor, unsigned int *alignedw, unsigned int *alignedh);

    /**
    * C wrapper Function to get stride, scanline and size depending on the underlying GPU/Video platform
    * @return    stride
    *            scanline
    *            size
    */
    void qry_stride_scanline_size(gbm_bufdesc *descriptor, unsigned int *stride,
                                  unsigned int *scanline, unsigned int *size);

    /**
     * C wrapper Function to query size based on format from the platform wrapper
     * @return    : size
     *
     */
    unsigned int qry_size(gbm_bufdesc *desc, unsigned int alignedw, unsigned int alignedh);

    /**
    * C wrapper Function to check whether the format is UBWC or not.
    * @params    gbm format
    * @return    boolean 0 (non UBWC format)
    *                    1 (UBWC format)
    */
    bool is_valid_ubwc_format(int format);

    /**
     * Function to return bytes per pixel for a given uncompressed RGB format
     * @params    uncompressed RGB gbm format
     * @return    bytes per pixel
     *
     */
    uint32_t get_bpp_for_uncmprsd_rgb_format(int format);

    /**
     * Function to return bytes needed to represent UBWC RGB Metabuffer
     * @params    RGB gbm format
     * @return    size of UBWC RGB meta buffer
     *
     */
    uint32_t get_rgb_ubwc_metabuffer_size(int width, int height, int bpp);

    /**
    * C wrapper Function to check whether the format is uncompressed RGB format or not.
    * @params    gbm format
    * @return    boolean 0 (compressed RGB format)
    *                    1 (uncompressed RGB format)
    */
    bool is_valid_uncmprsd_rgb_format(int format);

    /**
    * C wrapper Function to check whether the format is uncompressed RGB format or not.
    * @params    gbm format
    * @return    boolean 0 (uncompressed RGB format)
    *                    1 (compressed RGB format)
    */
    bool is_valid_cmprsd_rgb_format(int format);

    /**
    * C wrapper Function to check whether the format is yuv format or not.
    * @params    gbm format
    * @return    boolean 0 (non yuv format)
    *                    1 (yuv format)
    */
    bool is_valid_yuv_format(int format);

    /**
     * C wrapper function to know if the format is UBWC
     */
    bool is_ubwc_enbld(int format, int prod_usage,
                              int cons_usage);

    /**
    * C wrapper Function to check whether the format is RAW format or not.
    * @params    gbm format
    * @return    boolean 0 (non RAW format)
    *                    1 (RAW format)
    */
    bool is_valid_raw_format(int format);

    /**
     * C wrapper function to know if the format is RGB
     */
    bool is_valid_rgb_fmt(int format);
    /**
     * C wrapper Function to delete platform wrapper object
     * @return  : None
     *
     */
    void platform_wrap_deinstnce(void);

    /**
     * C wrapper Function to create a cpp object of platform wrapper class
     * @return   0 : success
     *           1 : failure
     */
    bool platform_wrap_instnce(void);

}

}  // namespace msm_gbm

#endif  // __MSMGBM_PFM_WRP_H__
