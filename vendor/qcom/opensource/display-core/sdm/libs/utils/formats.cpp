/*
* Copyright (c) 2016-2018, 2020 The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of The Linux Foundation nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
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

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <utils/formats.h>
#include <errno.h>
#include <cstring>

#define __CLASS__ "FormatsUtils"

namespace sdm {

bool IsUBWCFormat(LayerBufferFormat format) {
  switch (format) {
  case kFormatRGBA8888Ubwc:
  case kFormatRGBX8888Ubwc:
  case kFormatBGR565Ubwc:
  case kFormatYCbCr420SPVenusUbwc:
  case kFormatRGBA1010102Ubwc:
  case kFormatRGBX1010102Ubwc:
  case kFormatYCbCr420TP10Ubwc:
  case kFormatYCbCr420P010Ubwc:
  case kFormatRGBA16161616FUbwc:
  case kFormatRGBA8888UbwcLossy2To1:
  case kFormatRGBA8888UbwcLossy8To5:
    return true;
  default:
    return false;
  }
}

bool Is10BitFormat(LayerBufferFormat format) {
  switch (format) {
  case kFormatRGBA1010102:
  case kFormatARGB2101010:
  case kFormatRGBX1010102:
  case kFormatXRGB2101010:
  case kFormatBGRA1010102:
  case kFormatABGR2101010:
  case kFormatBGRX1010102:
  case kFormatXBGR2101010:
  case kFormatRGBA1010102Ubwc:
  case kFormatRGBX1010102Ubwc:
  case kFormatYCbCr420P010:
  case kFormatYCbCr420TP10Ubwc:
  case kFormatYCbCr420P010Ubwc:
  case kFormatYCbCr420P010Venus:
    return true;
  default:
    return false;
  }
}

bool Is16BitFormat(LayerBufferFormat format) {
  switch (format) {
  case kFormatRGBA16161616F:
  case kFormatRGBA16161616FUbwc:
    return true;
  default:
    return false;
  }
}

bool IsRgbFormat(const LayerBufferFormat &format) {
  switch (format) {
    case kFormatARGB8888:
    case kFormatRGBA8888:
    case kFormatBGRA8888:
    case kFormatXRGB8888:
    case kFormatRGBX8888:
    case kFormatBGRX8888:
    case kFormatRGBA8888Ubwc:
    case kFormatRGBX8888Ubwc:
    case kFormatRGBA1010102:
    case kFormatARGB2101010:
    case kFormatRGBX1010102:
    case kFormatXRGB2101010:
    case kFormatBGRA1010102:
    case kFormatABGR2101010:
    case kFormatBGRX1010102:
    case kFormatXBGR2101010:
    case kFormatRGBA1010102Ubwc:
    case kFormatRGBX1010102Ubwc:
    case kFormatRGB888:
    case kFormatBGR888:
    case kFormatRGB565:
    case kFormatBGR565:
    case kFormatRGBA5551:
    case kFormatRGBA4444:
    case kFormatBGR565Ubwc:
    case kFormatRGB101010:
    case kFormatRGBA16161616F:
    case kFormatRGBA16161616FUbwc:
    case kFormatRGBA8888UbwcLossy2To1:
    case kFormatRGBA8888UbwcLossy8To5:
      return true;
    default:
      return false;
  }
}
// clang-format off
const char *GetFormatString(const LayerBufferFormat &format) {
  switch (format) {
  case kFormatARGB8888:                 return "ARGB_8888";
  case kFormatRGBA8888:                 return "RGBA_8888";
  case kFormatBGRA8888:                 return "BGRA_8888";
  case kFormatXRGB8888:                 return "XRGB_8888";
  case kFormatRGBX8888:                 return "RGBX_8888";
  case kFormatBGRX8888:                 return "BGRX_8888";
  case kFormatRGBA5551:                 return "RGBA_5551";
  case kFormatRGBA4444:                 return "RGBA_4444";
  case kFormatRGB888:                   return "RGB_888";
  case kFormatBGR888:                   return "BGR_888";
  case kFormatRGB565:                   return "RGB_565";
  case kFormatBGR565:                   return "BGR_565";
  case kFormatRGBA8888Ubwc:             return "RGBA_8888_UBWC";
  case kFormatRGBX8888Ubwc:             return "RGBX_8888_UBWC";
  case kFormatBGR565Ubwc:               return "BGR_565_UBWC";
  case kFormatYCbCr420Planar:           return "Y_CB_CR_420";
  case kFormatYCrCb420Planar:           return "Y_CR_CB_420";
  case kFormatYCrCb420PlanarStride16:   return "Y_CR_CB_420_STRIDE16";
  case kFormatYCbCr420SemiPlanar:       return "Y_CBCR_420";
  case kFormatYCrCb420SemiPlanar:       return "Y_CRCB_420";
  case kFormatYCbCr420SemiPlanarVenus:  return "Y_CBCR_420_VENUS";
  case kFormatYCrCb420SemiPlanarVenus:  return "Y_CRCB_420_VENUS";
  case kFormatYCbCr422H1V2SemiPlanar:   return "Y_CBCR_422_H1V2";
  case kFormatYCrCb422H1V2SemiPlanar:   return "Y_CRCB_422_H1V2";
  case kFormatYCbCr422H2V1SemiPlanar:   return "Y_CBCR_422_H2V1";
  case kFormatYCrCb422H2V1SemiPlanar:   return "Y_CRCB_422_H2V2";
  case kFormatYCbCr420SPVenusUbwc:      return "Y_CBCR_420_VENUS_UBWC";
  case kFormatYCbCr420SPVenusTile:      return "Y_CBCR_420_VENUS_TILED";
  case kFormatYCbCr422H2V1Packed:       return "YCBYCR_422_H2V1";
  case kFormatCbYCrY422H2V1Packed:      return "CBYCRY_422_H2V1";
  case kFormatRGBA1010102:              return "RGBA_1010102";
  case kFormatARGB2101010:              return "ARGB_2101010";
  case kFormatRGBX1010102:              return "RGBX_1010102";
  case kFormatXRGB2101010:              return "XRGB_2101010";
  case kFormatBGRA1010102:              return "BGRA_1010102";
  case kFormatABGR2101010:              return "ABGR_2101010";
  case kFormatBGRX1010102:              return "BGRX_1010102";
  case kFormatXBGR2101010:              return "XBGR_2101010";
  case kFormatRGBA1010102Ubwc:          return "RGBA_1010102_UBWC";
  case kFormatRGBX1010102Ubwc:          return "RGBX_1010102_UBWC";
  case kFormatYCbCr420P010:             return "Y_CBCR_420_P010";
  case kFormatYCbCr420TP10Ubwc:         return "Y_CBCR_420_TP10_UBWC";
  case kFormatYCbCr420P010Ubwc:         return "Y_CBCR_420_P010_UBWC";
  case kFormatYCbCr420P010Venus:        return "Y_CBCR_420_P010_VENUS";
  case kFormatYCbCr420TP10Tile:         return "Y_CBCR_420_TP10_TILED";
  case kFormatYCbCr420P010Tile:         return "Y_CBCR_420_P010_TILED";
  case kFormatRGBA16161616F:            return "RGBA16161616F";
  case kFormatRGBA16161616FUbwc:        return "RGBA16161616F_UBWC";
  case kFormatA8:                       return "A8";
  case kFormatRGBA8888UbwcLossy2To1:    return "RGBA_8888_UBWC_LOSSY_2_TO_1";
  case kFormatRGBA8888UbwcLossy8To5:    return "RGBA_8888_UBWC_LOSSY_8_TO_5";
  default:                              return "UNKNOWN";
  }
}
// clang-format on
BufferLayout GetBufferLayout(LayerBufferFormat format) {
  switch (format) {
  case kFormatYCbCr420TP10Ubwc:
  case kFormatYCbCr420TP10Tile:
    return kTPTiled;
  case kFormatYCbCr420SPVenusTile:
  case kFormatYCbCr420P010Tile:
    return kUBWC;
  default:
    return (IsUBWCFormat(format) ? kUBWC : kLinear);
  }
}

float GetBufferFormatBpp(LayerBufferFormat format) {
  float bpp = 0.0f;
  switch (format) {
    case kFormatRGBA16161616F:
    case kFormatRGBA16161616FUbwc:
      return 8.0f;
    case kFormatARGB8888:
    case kFormatRGBA8888:
    case kFormatBGRA8888:
    case kFormatXRGB8888:
    case kFormatRGBX8888:
    case kFormatBGRX8888:
    case kFormatRGBA8888Ubwc:
    case kFormatRGBX8888Ubwc:
    case kFormatRGBA1010102:
    case kFormatARGB2101010:
    case kFormatRGBX1010102:
    case kFormatXRGB2101010:
    case kFormatBGRA1010102:
    case kFormatABGR2101010:
    case kFormatBGRX1010102:
    case kFormatXBGR2101010:
    case kFormatRGBA1010102Ubwc:
    case kFormatRGBX1010102Ubwc:
    case kFormatRGBA8888UbwcLossy2To1:
    case kFormatRGBA8888UbwcLossy8To5:
      return 4.0f;
    case kFormatRGB888:
    case kFormatBGR888:
    case kFormatYCbCr420P010:
    case kFormatYCbCr420P010Ubwc:
    case kFormatYCbCr420P010Venus:
    case kFormatYCbCr420P010Tile:
      return 3.0f;
    case kFormatRGB565:
    case kFormatBGR565:
    case kFormatRGBA5551:
    case kFormatRGBA4444:
    case kFormatBGR565Ubwc:
    case kFormatYCbCr422H2V1Packed:
    case kFormatCbYCrY422H2V1Packed:
    case kFormatYCrCb422H2V1SemiPlanar:
    case kFormatYCbCr422H2V1SemiPlanar:
    case kFormatYCbCr420TP10Ubwc:
    case kFormatYCbCr420TP10Tile:
    case kFormatYCbCr422H1V2SemiPlanar:
    case kFormatYCrCb422H1V2SemiPlanar:
      return 2.0f;
    case kFormatYCbCr420Planar:
    case kFormatYCrCb420Planar:
    case kFormatYCrCb420PlanarStride16:
    case kFormatYCbCr420SemiPlanar:
    case kFormatYCrCb420SemiPlanar:
    case kFormatYCbCr420SemiPlanarVenus:
    case kFormatYCrCb420SemiPlanarVenus:
    case kFormatYCbCr420SPVenusUbwc:
    case kFormatYCbCr420SPVenusTile:
      return 1.5f;
    case kFormatA8:
      return 1.0f;
    default:
      return 0.0f;
  }

  return bpp;
}

int GetCwbAlignmentFactor(LayerBufferFormat format) {
  // To check whether roi width*height*bpp is a multiple of 256 Bytes
  // UseCase 1: bpp is a power of 2 (eg.: 2, 4). Thus, 256 is divisible by the bpp.
  // CWB Roi_pixels = ROI_width * ROI_height
  // We need to align CWB Roi_pixels to nearest upper multiple of (256 / bpp).
  // UseCase 2: when bpp is 1.5. To satisfy the below equation :
  // Roi_pixels * bpp = multiple of 256 B => Roi_pixels * 1.5 = multiple of 256 B
  // => Roi_pixels = (n / 1.5) * 256, Roi_pixels is int when n is multiple of 3
  // => Roi_pixels is a multiple of 512 .
  // UseCase 3: when bpp is a multiple of 3. To satisfy the below equation :
  // Roi_pixels * 3 Bytes = n * 256 Bytes (n is some integer multiple)
  // => Roi_pixels = (n / 3) * 256, Roi_pixels is int when n is multiple of 3
  // For n=3: Roi_pixels = 256, For n=6: Roi_pixels = 512, and so on... Thus, Roi_pixels is
  // a multiple of 256, we align CWB Roi_pixels to nearest upper multiple of 256 pixels.

  float bpp = GetBufferFormatBpp(format);
  if (bpp == 0.0f) {  // invalid color format
    return 0;
  }

  uint32_t alignment_factor = 0;

  if (bpp == 1.5f) {
    alignment_factor = 512;
  } else if (bpp == 3.0f) {
    alignment_factor = 256;
  } else {
    uint32_t bpp_int = static_cast<uint32_t>(bpp);
    if (bpp_int % 2 == 0) {
      alignment_factor = 256 / bpp_int;
    }
  }
  return alignment_factor;
}

int GetBufferFormatTileSize(LayerBufferFormat format, FormatTileSize *tile_size) {
  switch (format) {
  case kFormatYCbCr420SPVenusUbwc:
  case kFormatYCbCr420SPVenusTile:
    tile_size->tile_width = 32;
    tile_size->tile_height = 8;
    tile_size->uv_tile_width = 16;
    tile_size->uv_tile_height = 8;
    break;
  case kFormatYCbCr420TP10Ubwc:
  case kFormatYCbCr420TP10Tile:
    tile_size->tile_width = 48;
    tile_size->tile_height = 4;
    tile_size->uv_tile_width = 24;
    tile_size->uv_tile_height = 4;
    break;
  case kFormatYCbCr420P010Ubwc:
  case kFormatYCbCr420P010Tile:
    tile_size->tile_width = 32;
    tile_size->tile_height = 4;
    tile_size->uv_tile_width = 16;
    tile_size->uv_tile_height = 4;
    break;
  default:
    return -ENOTSUP;
  }
  return 0;
}

bool HasAlphaChannel(LayerBufferFormat format) {
  switch (format) {
  case kFormatARGB8888:
  case kFormatRGBA8888:
  case kFormatBGRA8888:
  case kFormatRGBA5551:
  case kFormatRGBA4444:
  case kFormatRGBA8888Ubwc:
  case kFormatRGBA1010102:
  case kFormatARGB2101010:
  case kFormatBGRA1010102:
  case kFormatABGR2101010:
  case kFormatRGBA1010102Ubwc:
  case kFormatRGBA16161616F:
  case kFormatRGBA16161616FUbwc:
  case kFormatRGBA8888UbwcLossy2To1:
  case kFormatRGBA8888UbwcLossy8To5:
    return true;
  default:
    return false;
  }
}

bool IsWideColor(const QtiColorPrimaries &primary) {
  switch (primary) {
    case QtiColorPrimaries_DCIP3:
    case QtiColorPrimaries_BT2020:
      return true;
    default:
      return false;
  }
}

bool IsExtendedRange(LayerBuffer buffer) {
  return (Is16BitFormat(buffer.format) && buffer.dataspace.range == QtiRange_Extended);
}

// TODO(user): eventually we should upgrade the legacy ColorMetadata struct in
// snapdragon_color_intf.h so we don't have to do all this
ColorMetaData convertToLegacyColorMetadata(const LayerBuffer *buffer) {
  ColorMetaData data;
  data.colorPrimaries = primaries_map[buffer->dataspace.colorPrimaries];
  data.range = range_map[buffer->dataspace.range];
  data.transfer = transfer_map[buffer->dataspace.transfer];
  data.matrixCoefficients = matrix_map[buffer->matrixCoefficients];

  // mastering display
  data.masteringDisplayInfo.colorVolumeSEIEnabled = buffer->masteringDisplayInfo.colorVolumeSEIEnabled;
  data.masteringDisplayInfo.primaries.rgbPrimaries[0][0] = buffer->masteringDisplayInfo.primaryRed.x;
  data.masteringDisplayInfo.primaries.rgbPrimaries[0][1] = buffer->masteringDisplayInfo.primaryRed.y;
  data.masteringDisplayInfo.primaries.rgbPrimaries[1][0] = buffer->masteringDisplayInfo.primaryGreen.x;
  data.masteringDisplayInfo.primaries.rgbPrimaries[1][1] = buffer->masteringDisplayInfo.primaryGreen.y;
  data.masteringDisplayInfo.primaries.rgbPrimaries[2][0] = buffer->masteringDisplayInfo.primaryBlue.x;
  data.masteringDisplayInfo.primaries.rgbPrimaries[2][1] = buffer->masteringDisplayInfo.primaryBlue.y;
  data.masteringDisplayInfo.primaries.whitePoint[0] = buffer->masteringDisplayInfo.whitePoint.x;
  data.masteringDisplayInfo.primaries.whitePoint[1] = buffer->masteringDisplayInfo.whitePoint.y;
  data.masteringDisplayInfo.maxDisplayLuminance = buffer->masteringDisplayInfo.maxDisplayLuminance;
  data.masteringDisplayInfo.minDisplayLuminance = buffer->masteringDisplayInfo.minDisplayLuminance;

  // content light level
  data.contentLightLevel.lightLevelSEIEnabled = buffer->contentLightLevel.lightLevelSEIEnabled;
  data.contentLightLevel.maxContentLightLevel = buffer->contentLightLevel.maxContentLightLevel;
  data.contentLightLevel.maxPicAverageLightLevel = buffer->contentLightLevel.maxFrameAverageLightLevel;
  // minPicAverageLightLevel is discarded in vendor_qti_hardware_display_common_QtiContentLightLevel
  data.contentLightLevel.minPicAverageLightLevel = 0;

  // color remapping info
  data.cRI.criEnabled = buffer->cRI.criEnabled;
  data.cRI.crId = buffer->cRI.crId;
  data.cRI.crCancelFlag = buffer->cRI.crCancelFlag;
  data.cRI.crPersistenceFlag = buffer->cRI.crPersistenceFlag;
  data.cRI.crVideoSignalInfoPresentFlag = buffer->cRI.crVideoSignalInfoPresentFlag;
  data.cRI.crRange = buffer->cRI.crRange;
  data.cRI.crPrimaries = primaries_map[buffer->cRI.crPrimaries];
  data.cRI.crTransferFunction = transfer_map[buffer->cRI.crTransferFunction];
  data.cRI.crMatrixCoefficients = matrix_map[buffer->cRI.crMatrixCoefficients];
  data.cRI.crInputBitDepth = buffer->cRI.crInputBitDepth;
  data.cRI.crOutputBitDepth = buffer->cRI.crOutputBitDepth;
  memcpy(&data.cRI.crPreLutNumValMinusOne, &buffer->cRI.crPreLutNumValMinusOne, (sizeof(uint32_t)*3));
  memcpy(&data.cRI.crPreLutCodedValue, &buffer->cRI.crPreLutCodedValue, (sizeof(uint32_t)*99));
  memcpy(&data.cRI.crPreLutTargetValue, &buffer->cRI.crPreLutTargetValue, (sizeof(uint32_t)*99));
  data.cRI.crMatrixPresentFlag = buffer->cRI.crMatrixPresentFlag;
  data.cRI.crLog2MatrixDenom = buffer->cRI.crLog2MatrixDenom;
  memcpy(&data.cRI.crCoefficients, &buffer->cRI.crCoefficients, (sizeof(uint32_t)*9));
  memcpy(&data.cRI.crPostLutNumValMinusOne, &buffer->cRI.crPostLutNumValMinusOne, (sizeof(uint32_t)*3));
  memcpy(&data.cRI.crPostLutCodedValue, &buffer->cRI.crPostLutCodedValue, (sizeof(uint32_t)*99));
  memcpy(&data.cRI.crPostLutTargetValue, &buffer->cRI.crPostLutTargetValue, (sizeof(uint32_t)*99));

  // dynamic metadata
  data.dynamicMetaDataValid = buffer->dynamicMetadata.dynamicMetaDataValid;
  data.dynamicMetaDataLen = buffer->dynamicMetadata.dynamicMetaDataLen;
  memcpy(&data.dynamicMetaDataPayload, &buffer->dynamicMetadata.dynamicMetaDataPayload, buffer->dynamicMetadata.dynamicMetaDataLen);

  return data;
}

}  // namespace sdm
