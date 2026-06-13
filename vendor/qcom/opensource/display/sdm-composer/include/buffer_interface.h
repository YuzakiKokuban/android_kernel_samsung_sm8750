/*
* Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted
* provided that the following conditions are met:
*    * Redistributions of source code must retain the above copyright notice, this list of
*      conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above copyright notice, this list of
*      conditions and the following disclaimer in the documentation and/or other materials provided
*      with the distribution.
*    * Neither the name of The Linux Foundation nor the names of its contributors may be used to
*      endorse or promote products derived from this software without specific prior written
*      permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
* Changes from Qualcomm Innovation Center are provided under the following license:
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __BUFFER_INTERFACE_H__
#define __BUFFER_INTERFACE_H__

namespace sdm {

typedef void * Handle;

enum BufferFormat {
  /* All RGB formats, Any new format will be added towards end of this group to maintain backward
     compatibility.
  */
  kBufferFormatInvalid = -1,     //!< Invalid buffer format
  kBufferFormatARGB8888,         //!< 8-bits Alpha, Red, Green, Blue interleaved in ARGB order.
  kBufferFormatRGBA8888,         //!< 8-bits Red, Green, Blue, Alpha interleaved in RGBA order.
  kBufferFormatBGRA8888,         //!< 8-bits Blue, Green, Red, Alpha interleaved in BGRA order.
  kBufferFormatXRGB8888,         //!< 8-bits Padding, Red, Green, Blue interleaved in XRGB order.
                                 //!< NoAlpha.
  kBufferFormatRGBX8888,         //!< 8-bits Red, Green, Blue, Padding interleaved in RGBX order.
                                 //!< NoAlpha.
  kBufferFormatBGRX8888,         //!< 8-bits Blue, Green, Red, Padding interleaved in BGRX order.
                                 //!< NoAlpha.
  kBufferFormatRGBA5551,         //!< 5-bits Red, Green, Blue, and 1 bit Alpha interleaved in
                                 //!< RGBA order.
  kBufferFormatRGBA4444,         //!< 4-bits Red, Green, Blue, Alpha interleaved in RGBA order.
  kBufferFormatRGB888,           //!< 8-bits Red, Green, Blue interleaved in RGB order. No Alpha.
  kBufferFormatBGR888,           //!< 8-bits Blue, Green, Red interleaved in BGR order. No Alpha.
  kBufferFormatRGB565,           //!< 5-bit Red, 6-bit Green, 5-bit Blue interleaved in RGB order.
                                 //!< NoAlpha.
  kBufferFormatBGR565,           //!< 5-bit Blue, 6-bit Green, 5-bit Red interleaved in BGR order.
                                 //!< NoAlpha.
  kBufferFormatRGBA1010102,      //!< 10-bits Red, Green, Blue, Alpha interleaved in RGBA order.
  kBufferFormatARGB2101010,      //!< 10-bits Alpha, Red, Green, Blue interleaved in ARGB order.
  kBufferFormatRGBX1010102,      //!< 10-bits Red, Green, Blue, Padding interleaved in RGBX order.
                                 //!< NoAlpha.
  kBufferFormatXRGB2101010,      //!< 10-bits Padding, Red, Green, Blue interleaved in XRGB order.
                                 //!< NoAlpha.
  kBufferFormatBGRA1010102,      //!< 10-bits Blue, Green, Red, Alpha interleaved in BGRA order.
  kBufferFormatABGR2101010,      //!< 10-bits Alpha, Blue, Green, Red interleaved in ABGR order.
  kBufferFormatBGRX1010102,      //!< 10-bits Blue, Green, Red, Padding interleaved in BGRX order.
                                 //!< NoAlpha.
  kBufferFormatXBGR2101010,      //!< 10-bits Padding, Blue, Green, Red interleaved in XBGR order.
                                 //!< NoAlpha.
  kBufferFormatRGB101010,        //!< 10-bits Red, Green, Blue, interleaved in RGB order. No Alpha.
  kBufferFormatRGBA8888Ubwc,     //!< UBWC aligned RGBA8888 format
  kBufferFormatRGBX8888Ubwc,     //!< UBWC aligned RGBX8888 format
  kBufferFormatBGR565Ubwc,       //!< UBWC aligned BGR565 format
  kBufferFormatRGBA1010102Ubwc,  //!< UBWC aligned RGBA1010102 format
  kBufferFormatRGBX1010102Ubwc,  //!< UBWC aligned RGBX1010102 format
};

struct Rect {
  float left = 0.0f;            //!< Specifies the left coordinates of the pixel buffer
  float top = 0.0f;             //!< Specifies the top coordinates of the pixel buffer
  float right = 0.0f;           //!< Specifies the right coordinates of the pixel buffer
  float bottom = 0.0f;          //!< Specifies the bottom coordinates of the pixel buffer

  bool operator==(const Rect& rect) const {
    return left == rect.left && right == rect.right && top == rect.top && bottom == rect.bottom;
  }

  bool operator!=(const Rect& rect) const {
    return !operator==(rect);
  }
};

struct BufferHandle {
  int32_t fd = -1;                             //!< fd of the allocated buffer to be displayed.
  int32_t producer_fence_fd = -1;              //!< Created and signaled by the producer. Consumer
                                               //!< needs to wait on this before the buffer
                                               //!< is being accessed
  int32_t consumer_fence_fd = -1;              //!< Created and signaled by the consumer. Producer
                                               //!< needs to wait on this before the buffer
                                               //!< is being modified
  uint32_t width = 0;                          //!< Actual width of the buffer in pixels
  uint32_t height = 0;                         //!< Actual height of the buffer in pixels.
  uint32_t aligned_width = 0;                  //!< Aligned width of the buffer in pixels
  uint32_t aligned_height = 0;                 //!< Aligned height of the buffer in pixels
  BufferFormat format = kBufferFormatInvalid;  //!< Format of the buffer refer BufferFormat
  uint32_t stride_in_bytes = 0;                //!< Stride of the buffer in bytes
  uint32_t size = 0;                           //!< Allocated buffer size
  bool uncached = false;                       //!< Enable or disable buffer caching during R/W
  int64_t buffer_id = -1;                      //!< Unique Id of the allocated buffer for the
                                               //!< internal use only
  Rect src_crop = {};                          //!< Crop rectangle of src buffer, if client doesn't
                                               //!< specify, its default to {0, 0, width, height}
};

}  // namespace sdm

#endif  // __BUFFER_INTERFACE_H__


