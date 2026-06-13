/*
* Copyright (c) 2016 - 2018, The Linux Foundation. All rights reserved.
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

#ifndef __FORMATS_H__
#define __FORMATS_H__

#include <core/layer_stack.h>

namespace sdm {

struct FormatTileSize {
  /*< Tile width in pixels. For YUV formats this will give only the
      tile width for Y plane*/
  uint32_t tile_width = 0;
  /*< Tile height in pixels. For YUV formats this will give only the
      tile height for Y plane*/
  uint32_t tile_height = 0;

  /*< Tile width in pixels. Only valid for YUV formats where this will
      give tile width for UV plane*/
  uint32_t uv_tile_width = 0;
  /*< Tile height in pixels. Only valid for YUV formats where this will
       give tile height for UV plane*/
  uint32_t uv_tile_height = 0;
};

bool IsUBWCFormat(LayerBufferFormat format);
bool Is10BitFormat(LayerBufferFormat format);
bool Is16BitFormat(LayerBufferFormat format);
const char *GetFormatString(const LayerBufferFormat &format);
BufferLayout GetBufferLayout(LayerBufferFormat format);
int GetBufferFormatTileSize(LayerBufferFormat format, FormatTileSize *tile_size);
float GetBufferFormatBpp(LayerBufferFormat format);
int GetCwbAlignmentFactor(LayerBufferFormat format);
bool HasAlphaChannel(LayerBufferFormat format);
bool IsWideColor(const QtiColorPrimaries &color_primary);
bool IsRgbFormat(const LayerBufferFormat &format);
bool IsExtendedRange(LayerBuffer buffer);
ColorMetaData convertToLegacyColorMetadata(const LayerBuffer *buffer);

static std::unordered_map<QtiColorPrimaries, ColorPrimaries> primaries_map = {
  {QtiColorPrimaries_BT709_5, ColorPrimaries_BT709_5},
  {QtiColorPrimaries_BT470_6M, ColorPrimaries_BT470_6M},
  {QtiColorPrimaries_BT601_6_625, ColorPrimaries_BT601_6_625},
  {QtiColorPrimaries_BT601_6_525, ColorPrimaries_BT601_6_525},
  {QtiColorPrimaries_SMPTE_240M, ColorPrimaries_SMPTE_240M},
  {QtiColorPrimaries_GenericFilm, ColorPrimaries_GenericFilm},
  {QtiColorPrimaries_BT2020, ColorPrimaries_BT2020},
  {QtiColorPrimaries_SMPTE_ST428, ColorPrimaries_SMPTE_ST428},
  {QtiColorPrimaries_AdobeRGB, ColorPrimaries_AdobeRGB},
  {QtiColorPrimaries_DCIP3, ColorPrimaries_DCIP3},
  {QtiColorPrimaries_EBU3213, ColorPrimaries_EBU3213},
  {QtiColorPrimaries_Max, ColorPrimaries_Max},
};
static std::unordered_map<ColorPrimaries, QtiColorPrimaries> qti_primaries_map = {
  {ColorPrimaries_BT709_5, QtiColorPrimaries_BT709_5},
  {ColorPrimaries_BT470_6M, QtiColorPrimaries_BT470_6M},
  {ColorPrimaries_BT601_6_625, QtiColorPrimaries_BT601_6_625},
  {ColorPrimaries_BT601_6_525, QtiColorPrimaries_BT601_6_525},
  {ColorPrimaries_SMPTE_240M, QtiColorPrimaries_SMPTE_240M},
  {ColorPrimaries_GenericFilm, QtiColorPrimaries_GenericFilm},
  {ColorPrimaries_BT2020, QtiColorPrimaries_BT2020},
  {ColorPrimaries_SMPTE_ST428, QtiColorPrimaries_SMPTE_ST428},
  {ColorPrimaries_AdobeRGB, QtiColorPrimaries_AdobeRGB},
  {ColorPrimaries_DCIP3, QtiColorPrimaries_DCIP3},
  {ColorPrimaries_EBU3213, QtiColorPrimaries_EBU3213},
  {ColorPrimaries_Max, QtiColorPrimaries_Max},
};

static std::unordered_map<QtiColorRange, ColorRange> range_map = {
  {QtiRange_Limited, Range_Limited},
  {QtiRange_Full, Range_Full},
  {QtiRange_Extended, Range_Extended},
  {QtiRange_Max, Range_Max},
};

static std::unordered_map<ColorRange, QtiColorRange> qti_range_map = {
  {Range_Limited, QtiRange_Limited},
  {Range_Full, QtiRange_Full},
  {Range_Extended, QtiRange_Extended},
  {Range_Max, QtiRange_Max},
};

static std::unordered_map<QtiGammaTransfer, GammaTransfer> transfer_map = {
  {QtiTransfer_sRGB, Transfer_sRGB},
  {QtiTransfer_Gamma2_2, Transfer_Gamma2_2},
  {QtiTransfer_Gamma2_8, Transfer_Gamma2_8},
  {QtiTransfer_SMPTE_170M, Transfer_SMPTE_170M},
  {QtiTransfer_SMPTE_240M, Transfer_SMPTE_240M},
  {QtiTransfer_Linear, Transfer_Linear},
  {QtiTransfer_Log, Transfer_Log},
  {QtiTransfer_Log_Sqrt, Transfer_Log_Sqrt},
  {QtiTransfer_XvYCC, Transfer_XvYCC},
  {QtiTransfer_BT1361, Transfer_BT1361},
  {QtiTransfer_sYCC, Transfer_sYCC},
  {QtiTransfer_BT2020_2_1, Transfer_BT2020_2_1},
  {QtiTransfer_BT2020_2_2, Transfer_BT2020_2_2},
  {QtiTransfer_SMPTE_ST2084, Transfer_SMPTE_ST2084},
  {QtiTransfer_ST_428, Transfer_ST_428},
  {QtiTransfer_HLG, Transfer_HLG},
  {QtiTransfer_Max, Transfer_Max},
};
static std::unordered_map<GammaTransfer, QtiGammaTransfer> qti_transfer_map = {
  {Transfer_sRGB, QtiTransfer_sRGB},
  {Transfer_Gamma2_2, QtiTransfer_Gamma2_2},
  {Transfer_Gamma2_8, QtiTransfer_Gamma2_8},
  {Transfer_SMPTE_170M, QtiTransfer_SMPTE_170M},
  {Transfer_SMPTE_240M, QtiTransfer_SMPTE_240M},
  {Transfer_Linear, QtiTransfer_Linear},
  {Transfer_Log, QtiTransfer_Log},
  {Transfer_Log_Sqrt, QtiTransfer_Log_Sqrt},
  {Transfer_XvYCC, QtiTransfer_XvYCC},
  {Transfer_BT1361, QtiTransfer_BT1361},
  {Transfer_sYCC, QtiTransfer_sYCC},
  {Transfer_BT2020_2_1, QtiTransfer_BT2020_2_1},
  {Transfer_BT2020_2_2, QtiTransfer_BT2020_2_2},
  {Transfer_SMPTE_ST2084, QtiTransfer_SMPTE_ST2084},
  {Transfer_ST_428, QtiTransfer_ST_428},
  {Transfer_HLG, QtiTransfer_HLG},
  {Transfer_Max, QtiTransfer_Max},
};

static std::unordered_map<QtiMatrixCoEfficients, MatrixCoEfficients> matrix_map = {
  {QtiMatrixCoEff_Identity, MatrixCoEff_Identity},
  {QtiMatrixCoEff_BT709_5, MatrixCoEff_BT709_5},
  {QtiMatrixCoeff_FCC_73_682, MatrixCoeff_FCC_73_682},
  {QtiMatrixCoEff_BT601_6_625, MatrixCoEff_BT601_6_625},
  {QtiMatrixCoEff_BT601_6_525, MatrixCoEff_BT601_6_525},
  {QtiMatrixCoEff_SMPTE240M, MatrixCoEff_SMPTE240M},
  {QtiMatrixCoEff_YCgCo, MatrixCoEff_YCgCo},
  {QtiMatrixCoEff_BT2020, MatrixCoEff_BT2020},
  {QtiMatrixCoEff_BT2020Constant, MatrixCoEff_BT2020Constant},
  {QtiMatrixCoEff_BT601_6_Unadjusted, MatrixCoEff_BT601_6_Unadjusted},
  {QtiMatrixCoEff_DCIP3, MatrixCoEff_DCIP3},
  {QtiMatrixCoEff_Chroma_NonConstant, MatrixCoEff_Chroma_NonConstant},
  {QtiMatrixCoEff_Max, MatrixCoEff_Max},
};
}  // namespace sdm

#endif  // __FORMATS_H__
