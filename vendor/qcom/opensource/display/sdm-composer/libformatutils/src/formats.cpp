/*
* Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#include "formats.h"

namespace sdm {

float GetBpp(BufferFormat format) {
  float bpp = 0.0f;
  switch (format) {
    case kBufferFormatARGB8888:
    case kBufferFormatRGBA8888:
    case kBufferFormatBGRA8888:
    case kBufferFormatXRGB8888:
    case kBufferFormatRGBX8888:
    case kBufferFormatBGRX8888:
    case kBufferFormatRGBA1010102:
    case kBufferFormatARGB2101010:
    case kBufferFormatRGBX1010102:
    case kBufferFormatXRGB2101010:
    case kBufferFormatBGRA1010102:
    case kBufferFormatABGR2101010:
    case kBufferFormatBGRX1010102:
    case kBufferFormatXBGR2101010:
    case kBufferFormatRGBA8888Ubwc:
    case kBufferFormatRGBX8888Ubwc:
    case kBufferFormatRGBA1010102Ubwc:
    case kBufferFormatRGBX1010102Ubwc:
      return 4.0f;
    case kBufferFormatRGB888:
    case kBufferFormatBGR888:
      return 3.0f;
    case kBufferFormatRGB565:
    case kBufferFormatBGR565:
    case kBufferFormatRGBA5551:
    case kBufferFormatRGBA4444:
    case kBufferFormatBGR565Ubwc:
      return 2.0f;
    default:
      return 0.0f;
  }

  return bpp;
}

LayerBufferFormat GetSDMFormat(BufferFormat buf_format) {
  switch (buf_format) {
    case kBufferFormatARGB8888:
      return kFormatARGB8888;
    case kBufferFormatRGBA8888:
      return kFormatRGBA8888;
    case kBufferFormatBGRA8888:
      return kFormatBGRA8888;
    case kBufferFormatXRGB8888:
      return kFormatXRGB8888;
    case kBufferFormatRGBX8888:
      return kFormatRGBX8888;
    case kBufferFormatBGRX8888:
      return kFormatBGRX8888;
    case kBufferFormatRGBA1010102:
      return kFormatRGBA1010102;
    case kBufferFormatARGB2101010:
      return kFormatARGB2101010;
    case kBufferFormatRGBX1010102:
      return kFormatRGBX1010102;
    case kBufferFormatXRGB2101010:
      return kFormatXRGB2101010;
    case kBufferFormatBGRA1010102:
      return kFormatBGRA1010102;
    case kBufferFormatABGR2101010:
      return kFormatABGR2101010;
    case kBufferFormatBGRX1010102:
      return kFormatBGRX1010102;
    case kBufferFormatXBGR2101010:
      return kFormatXBGR2101010;
    case kBufferFormatRGB888:
      return kFormatRGB888;
    case kBufferFormatBGR888:
      return kFormatBGR888;
    case kBufferFormatRGB565:
      return kFormatRGB565;
    case kBufferFormatBGR565:
      return kFormatBGR565;
    case kBufferFormatRGBA5551:
      return kFormatRGBA5551;
    case kBufferFormatRGBA4444:
      return kFormatRGBA4444;
    case kBufferFormatRGBA8888Ubwc:
      return kFormatRGBA8888Ubwc;
    case kBufferFormatRGBX8888Ubwc:
      return kFormatRGBX8888Ubwc;
    case kBufferFormatBGR565Ubwc:
      return kFormatBGR565Ubwc;
    case kBufferFormatRGBA1010102Ubwc:
      return kFormatRGBA1010102Ubwc;
    case kBufferFormatRGBX1010102Ubwc:
      return kFormatRGBX1010102Ubwc;
    default:
      return kFormatInvalid;
  }
}

BufferFormat GetSDMCompFormat(LayerBufferFormat sdm_format) {
  switch (sdm_format) {
    case kFormatARGB8888:
      return kBufferFormatARGB8888;
    case kFormatRGBA8888:
      return kBufferFormatRGBA8888;
    case kFormatBGRA8888:
      return kBufferFormatBGRA8888;
    case kFormatXRGB8888:
      return kBufferFormatXRGB8888;
    case kFormatRGBX8888:
      return kBufferFormatRGBX8888;
    case kFormatBGRX8888:
      return kBufferFormatBGRX8888;
    case kFormatRGBA1010102:
      return kBufferFormatRGBA1010102;
    case kFormatARGB2101010:
      return kBufferFormatARGB2101010;
    case kFormatRGBX1010102:
      return kBufferFormatRGBX1010102;
    case kFormatXRGB2101010:
      return kBufferFormatXRGB2101010;
    case kFormatBGRA1010102:
      return kBufferFormatBGRA1010102;
    case kFormatABGR2101010:
      return kBufferFormatABGR2101010;
    case kFormatBGRX1010102:
      return kBufferFormatBGRX1010102;
    case kFormatXBGR2101010:
      return kBufferFormatXBGR2101010;
    case kFormatRGB888:
      return kBufferFormatRGB888;
    case kFormatBGR888:
      return kBufferFormatBGR888;
    case kFormatRGB565:
      return kBufferFormatRGB565;
    case kFormatBGR565:
      return kBufferFormatBGR565;
    case kFormatRGBA5551:
      return kBufferFormatRGBA5551;
    case kFormatRGBA4444:
      return kBufferFormatRGBA4444;
    case kFormatRGBA8888Ubwc:
      return kBufferFormatRGBA8888Ubwc;
    case kFormatRGBX8888Ubwc:
      return kBufferFormatRGBX8888Ubwc;
    case kFormatBGR565Ubwc:
      return kBufferFormatBGR565Ubwc;
    case kFormatRGBA1010102Ubwc:
      return kBufferFormatRGBA1010102Ubwc;
    case kFormatRGBX1010102Ubwc:
      return kBufferFormatRGBX1010102Ubwc;
    default:
      return kBufferFormatInvalid;
  }
}

}  // namespace sdm

