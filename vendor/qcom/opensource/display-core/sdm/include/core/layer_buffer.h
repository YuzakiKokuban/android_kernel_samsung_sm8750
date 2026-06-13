// clang-format off
/*
* Copyright (c) 2014, 2016-2021, The Linux Foundation. All rights reserved.
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
*
* Changes from Qualcomm Innovation Center are provided under the following license:
* Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/
// clang-format on

/*! @file layer_buffer.h
  @brief File for layer buffer structure.

*/
#ifndef __LAYER_BUFFER_H__
#define __LAYER_BUFFER_H__

#include <utils/fence.h>
#include <stdint.h>
#include <utility>
#include <vector>
#include "sdm_types.h"
#include "color_extensions.h"
#include <QtiMasteringDisplay.h>
#include <QtiColorRange.h>
#include <QtiColorRemappingInfo.h>
#include <QtiMatrixCoEfficients.h>
#include <QtiContentLightLevel.h>
#include <QtiDynamicMetadata.h>
#include <QtiAnamorphicMetadata.h>
#include <Dataspace.h>

namespace sdm {

// color metadata values
using Dataspace = vendor_qti_hardware_display_common_Dataspace;
using QtiMasteringDisplay = vendor_qti_hardware_display_common_QtiMasteringDisplay;
using QtiMatrixCoEfficients = vendor_qti_hardware_display_common_QtiMatrixCoEfficients;
using QtiContentLightLevel = vendor_qti_hardware_display_common_QtiContentLightLevel;
using QtiColorRemappingInfo = vendor_qti_hardware_display_common_QtiColorRemappingInfo;
using QtiDynamicMetadata = vendor_qti_hardware_display_common_QtiDynamicMetadata;
using QtiColorPrimaries = vendor_qti_hardware_display_common_QtiColorPrimaries;
using QtiColorRange = vendor_qti_hardware_display_common_QtiColorRange;
using QtiGammaTransfer = vendor_qti_hardware_display_common_QtiGammaTransfer;

// Anamorphic metadata value
using QtiAnamorphicMetadata = vendor_qti_hardware_display_common_QtiAnamorphicMetadata;

#define NUM_UBWC_CR_STATS_LAYERS 2
typedef std::vector<std::pair<int, int>> UbwcCrStatsVector;

/*! @brief This enum represents display layer inverse gamma correction (IGC) types.

  @sa Layer
*/
enum LayerIGC {
  kIGCNotSpecified,       //!< IGC is not specified.
  kIGCsRGB,               //!< sRGB IGC type.
};

/*! @brief This enum represents different buffer formats supported by display manager.

  @sa LayerBuffer
*/
enum LayerBufferFormat {
  /* All RGB formats, Any new format will be added towards end of this group to maintain backward
     compatibility.
  */
  kFormatARGB8888,      //!< 8-bits Alpha, Red, Green, Blue interleaved in ARGB order.
  kFormatRGBA8888,      //!< 8-bits Red, Green, Blue, Alpha interleaved in RGBA order.
  kFormatBGRA8888,      //!< 8-bits Blue, Green, Red, Alpha interleaved in BGRA order.
  kFormatXRGB8888,      //!< 8-bits Padding, Red, Green, Blue interleaved in XRGB order. No Alpha.
  kFormatRGBX8888,      //!< 8-bits Red, Green, Blue, Padding interleaved in RGBX order. No Alpha.
  kFormatBGRX8888,      //!< 8-bits Blue, Green, Red, Padding interleaved in BGRX order. No Alpha.
  kFormatRGBA5551,      //!< 5-bits Red, Green, Blue, and 1 bit Alpha interleaved in RGBA order.
  kFormatRGBA4444,      //!< 4-bits Red, Green, Blue, Alpha interleaved in RGBA order.
  kFormatRGB888,        //!< 8-bits Red, Green, Blue interleaved in RGB order. No Alpha.
  kFormatBGR888,        //!< 8-bits Blue, Green, Red interleaved in BGR order. No Alpha.
  kFormatRGB565,        //!< 5-bit Red, 6-bit Green, 5-bit Blue interleaved in RGB order. No Alpha.
  kFormatBGR565,        //!< 5-bit Blue, 6-bit Green, 5-bit Red interleaved in BGR order. No Alpha.
  kFormatRGBA8888Ubwc,  //!< UBWC aligned RGBA8888 format
  kFormatRGBX8888Ubwc,  //!< UBWC aligned RGBX8888 format
  kFormatBGR565Ubwc,    //!< UBWC aligned BGR565 format
  kFormatRGBA1010102,   //!< 10-bits Red, Green, Blue, Alpha interleaved in RGBA order.
  kFormatARGB2101010,   //!< 10-bits Alpha, Red, Green, Blue interleaved in ARGB order.
  kFormatRGBX1010102,   //!< 10-bits Red, Green, Blue, Padding interleaved in RGBX order. No Alpha.
  kFormatXRGB2101010,   //!< 10-bits Padding, Red, Green, Blue interleaved in XRGB order. No Alpha.
  kFormatBGRA1010102,   //!< 10-bits Blue, Green, Red, Alpha interleaved in BGRA order.
  kFormatABGR2101010,   //!< 10-bits Alpha, Blue, Green, Red interleaved in ABGR order.
  kFormatBGRX1010102,   //!< 10-bits Blue, Green, Red, Padding interleaved in BGRX order. No Alpha.
  kFormatXBGR2101010,   //!< 10-bits Padding, Blue, Green, Red interleaved in XBGR order. No Alpha.
  kFormatRGBA1010102Ubwc,    //!< UBWC aligned RGBA1010102 format
  kFormatRGBX1010102Ubwc,    //!< UBWC aligned RGBX1010102 format
  kFormatRGB101010,          // 10-bits Red, Green, Blue, interleaved in RGB order. No Alpha.
  kFormatBlob,               // Task-specific data without a standard image structure.
  kFormatRGBA16161616F,      //!< Floating point 16-bits Red, Green, Blue, Alpha
                             //!< interleaved in RGBA order.
  kFormatRGBA16161616FUbwc,  //!< UBWC aligned floating point 16-bits Red, Green, Blue, Alpha
                             //!< interleaved in RGBA order.
  kFormatA8,                 //!< 8-bits Alpha format.
  kFormatRGBA8888UbwcLossy2To1,  //!< UBWC aligned RGBA8888 format with lossy 2:1 compression
  kFormatRGBA8888UbwcLossy8To5,  //!< UBWC aligned RGBA8888 format with lossy 8:5 compression

  /* All YUV-Planar formats, Any new format will be added towards end of this group to maintain
     backward compatibility.
  */
  kFormatYCbCr420Planar = 0x100,  //!< Y-plane: y(0), y(1), y(2) ... y(n)
                                  //!< 2x2 subsampled U-plane: u(0), u(2) ... u(n-1)
                                  //!< 2x2 subsampled V-plane: v(0), v(2) ... v(n-1)

  kFormatYCrCb420Planar,  //!< Y-plane: y(0), y(1), y(2) ... y(n)
                          //!< 2x2 subsampled V-plane: v(0), v(2) ... v(n-1)
                          //!< 2x2 subsampled U-plane: u(0), u(2) ... u(n-1)

  kFormatYCrCb420PlanarStride16,  //!< kFormatYCrCb420Planar with stride aligned to 16 bytes

  /* All YUV-Semiplanar formats, Any new format will be added towards end of this group to
     maintain backward compatibility.
  */
  kFormatYCbCr420SemiPlanar = 0x200,  //!< Y-plane: y(0), y(1), y(2) ... y(n)
                                      //!< 2x2 subsampled interleaved UV-plane:
                                      //!<    u(0), v(0), u(2), v(2) ... u(n-1), v(n-1)
                                      //!< aka NV12.

  kFormatYCrCb420SemiPlanar,  //!< Y-plane: y(0), y(1), y(2) ... y(n)
                              //!< 2x2 subsampled interleaved VU-plane:
                              //!<    v(0), u(0), v(2), u(2) ... v(n-1), u(n-1)
                              //!< aka NV21.

  kFormatYCbCr420SemiPlanarVenus,  //!< Y-plane: y(0), y(1), y(2) ... y(n)
                                   //!< 2x2 subsampled interleaved UV-plane:
                                   //!<    u(0), v(0), u(2), v(2) ... u(n-1), v(n-1)

  kFormatYCbCr422H1V2SemiPlanar,  //!< Y-plane: y(0), y(1), y(2) ... y(n)
                                  //!< vertically subsampled interleaved UV-plane:
                                  //!<    u(0), v(1), u(2), v(3) ... u(n-1), v(n)

  kFormatYCrCb422H1V2SemiPlanar,  //!< Y-plane: y(0), y(1), y(2) ... y(n)
                                  //!< vertically subsampled interleaved VU-plane:
                                  //!<    v(0), u(1), v(2), u(3) ... v(n-1), u(n)

  kFormatYCbCr422H2V1SemiPlanar,  //!< Y-plane: y(0), y(1), y(2) ... y(n)
                                  //!< horizontally subsampled interleaved UV-plane:
                                  //!<    u(0), v(1), u(2), v(3) ... u(n-1), v(n)

  kFormatYCrCb422H2V1SemiPlanar,  //!< Y-plane: y(0), y(1), y(2) ... y(n)
                                  //!< horizontally subsampled interleaved VU-plane:
                                  //!<    v(0), u(1), v(2), u(3) ... v(n-1), u(n)

  kFormatYCbCr420SPVenusUbwc,  //!< UBWC aligned YCbCr420SemiPlanarVenus format

  kFormatYCrCb420SemiPlanarVenus,  //!< Y-plane: y(0), y(1), y(2) ... y(n)
                                   //!< 2x2 subsampled interleaved UV-plane:
                                   //!<    v(0), u(0), v(2), u(2) ... v(n-1), u(n-1)

  kFormatYCbCr420P010,  //!< 16 bit Y-plane with 5 MSB bits set to 0:
                        //!< y(0), y(1), y(2) ... y(n)
                        //!< 2x2 subsampled interleaved 10 bit UV-plane with
                        //!< 5 MSB bits set to 0:
                        //!<    u(0), v(0), u(2), v(2) ... u(n-1), v(n-1)
                        //!< aka P010.

  kFormatYCbCr420TP10Ubwc,  //!< UBWC aligned YCbCr420TP10 format.

  kFormatYCbCr420P010Ubwc,  //!< UBWC aligned YCbCr420P010 format.

  kFormatYCbCr420P010Venus,    //!< Venus aligned YCbCr420P010 format.
                               //!
  kFormatYCbCr420SPVenusTile,  //!< Tiled & uncompressed YCbCr420SemiPlanarVenus format
  kFormatYCbCr420TP10Tile,     //!< Tiled & uncompressed YCbCr420TP10 format.
  kFormatYCbCr420P010Tile,     //!< Tiled & uncompressed YCbCr420P010 format.

  /* All YUV-Packed formats, Any new format will be added towards end of this group to maintain
     backward compatibility.
  */
  kFormatYCbCr422H2V1Packed = 0x300,  //!< Y-plane interleaved with horizontally subsampled U/V by
                                      //!< factor of 2
                                      //!<    y(0), u(0), y(1), v(0), y(2), u(2), y(3), v(2)
                                      //!<    y(n-1), u(n-1), y(n), v(n-1)

  kFormatCbYCrY422H2V1Packed,
  kFormatInvalid = 0xFFFFFFFF,
};

/*! @brief This structure defines a color sample plane belonging to a buffer format. RGB buffer
  formats have 1 plane whereas YUV buffer formats may have upto 4 planes.

  @sa LayerBuffer
*/
struct LayerBufferPlane {
  int fd = -1;           //!< File descriptor referring to the buffer associated with this plane.
  uint32_t offset = 0;   //!< Offset of the plane in bytes from beginning of the buffer.
  uint32_t stride = 0;   //!< Stride in bytes i.e. length of a scanline including padding.
};

/*! @brief This structure defines flags associated with a layer buffer. The 1-bit flag can be set
  to ON(1) or OFF(0).

  @sa LayerBuffer
*/
struct LayerBufferFlags {
  union {
    struct {
      uint32_t secure : 1;          //!< This flag shall be set by client to indicate that the
                                    //!< buffer need to be handled securely.

      uint32_t video  : 1;          //!< This flag shall be set by client to indicate that the
                                    //!< buffer is video/ui buffer.

      uint32_t macro_tile : 1;      //!< This flag shall be set by client to indicate that the
                                    //!< buffer format is macro tiled.

      uint32_t interlace : 1;       //!< This flag shall be set by the client to indicate that
                                    //!< the buffer has interlaced content.

      uint32_t secure_display : 1;  //!< This flag shall be set by the client to indicate that the
                                    //!< secure display session is in progress. Secure display
                                    //!< session can not coexist with non-secure session.

      uint32_t secure_camera : 1;   //!< This flag shall be set by the client to indicate that the
                                    //!< buffer is associated with secure camera session. A secure
                                    //!< camera layer can co-exist with non-secure layer(s).

      uint32_t hdr : 1;             //!< This flag shall be set by the client to indicate that the
                                    //!< the content is HDR.

      uint32_t ubwc_pi : 1;         //!< This flag shall be set by the client to indicate that the
                                    //!< buffer has PI content.

      uint32_t mask_layer : 1;      //!< This flag shall be set by client to indicate that the layer
                                    //!< is union of solid fill regions typically transparent pixels
                                    //!< and black pixels.

      uint32_t game : 1;            //!< This flag shall be set by the client to indicate that the
                                    //!< the content is game.

      uint32_t demura : 1;          //!< This flag shall be set to indicate that the
                                    //!< content is demura correction data

      uint32_t custom_tm : 1;       //!< This flag shall be set by client to indicate that the
                                    //!< buffer needs custom tonemap - 3D LUT / DE
#ifdef SEC_GC_CMN_FINGERPRINT_INDISPLAY
      uint32_t fingerprint_indisplay_layer : 1;  //!< Set if stack has finger print mask
#endif
    };

    uint32_t flags = 0;             //!< For initialization purpose only.
                                    //!< Client shall not refer to it directly.
  };
};

struct LayerHistData {
  bool stats_valid; /* bool indicating if the following histogram is valid */
  std::vector<uint32_t> stats_info; /* video histogram stats payload */
  uint32_t display_width;       /* video display_width */
  uint32_t display_height;      /* video display_height */
};

struct LayerTimestamp {
  uint32_t valid; /* Below fields are valid only if this boolean is set to true */
  uint32_t frame_number; /* Frame position of the content contained in the layer buffer */
  uint64_t frame_timestamp_us; /* Content timestamp of the frame contained in the layer buffer */
};

/*! @brief This structure defines a layer buffer handle which contains raw buffer and its associated
  properties.

  @sa LayerBuffer
  @sa LayerStack
*/
struct LayerBuffer {
  uint32_t width = 0;           //!< Aligned width of the Layer that this buffer is for.
  uint32_t height = 0;          //!< Aligned height of the Layer that this buffer is for.
  uint32_t unaligned_width = 0;
                                //!< Unaligned width of the Layer that this buffer is for.
  uint32_t unaligned_height = 0;
                                //!< Unaligned height of the Layer that this buffer is for.
  uint32_t size = 0;            //!< Size of a single buffer (even if multiple clubbed together)
  LayerBufferFormat format = kFormatRGBA8888;     //!< Format of the buffer content.
  LayerIGC igc = kIGCNotSpecified;                //!< IGC that will be applied on this layer.
  LayerBufferPlane planes[4] = {};
                                //!< Array of planes that this buffer contains. RGB buffer formats
                                //!< have 1 plane whereas YUV buffer formats may have upto 4 planes
                                //!< Total number of planes for the buffer will be interpreted based
                                //!< on the buffer format specified.

  shared_ptr<Fence> acquire_fence = nullptr;
                                //!< File descriptor referring to a sync fence object which will be
                                //!< signaled when buffer can be read/write by display manager.
                                //!< This fence object is set by the client during Commit(). For
                                //!< input buffers client shall signal this fence when buffer
                                //!< content is available and can be read by display manager. For
                                //!< output buffers, client shall signal fence when buffer is ready
                                //!< to be written by display manager.

                                //!< This field is used only during Commit() and shall be set to -1
                                //!< by the client when buffer is already available for read/write.

  shared_ptr<Fence> release_fence = nullptr;
                                //!< File descriptor referring to a sync fence object which will be
                                //!< signaled when buffer has been read/written by display manager.
                                //!< This fence object is set by display manager during Commit().
                                //!< For input buffers display manager will signal this fence when
                                //!< buffer has been consumed. For output buffers, display manager
                                //!< will signal this fence when buffer is produced.

                                //!< This field is used only during Commit() and will be set to -1
                                //!< by display manager when buffer is already available for
                                //!< read/write.

  LayerBufferFlags flags;       //!< Flags associated with this buffer.

  uint64_t buffer_id __attribute__((aligned(8))) = 0;
  //!< Specifies the buffer id.
  //!< This is converted from BufferInfo's private_data.
  UbwcCrStatsVector  ubwc_crstats[NUM_UBWC_CR_STATS_LAYERS] = {};
                                //! < UBWC Compression ratio,stats. Stored as a vector of pair of
                                //! of (tile size, #of tiles)

  LayerHistData hist_data;      //!< Histogram data associated with this layer buffer.
  LayerTimestamp timestamp_data;
                                //!< Timestamp data associated with this layer buffer.

  std::shared_ptr<CustomContentMetadata> extended_content_metadata;
                                //! Vector to hold extended content MD associated with this buffer.


  LayerBuffer() {
    dataspace.colorPrimaries = QtiColorPrimaries_BT709_5;
    dataspace.transfer = QtiTransfer_sRGB;
    dataspace.range = QtiRange_Limited;
  }

  uint64_t handle_id = 0;       //!< This is the BufferInfo's ID.
  uint64_t usage = 0;           //!< Opaque Usage flags associated with this layer buffer.

  // Color metadata info. When integrated with snapalloc, ColorMetaData struct got deprecated
  // so we now keep track of all relevant values individually
  Dataspace dataspace;
  QtiMatrixCoEfficients matrixCoefficients;
  QtiMasteringDisplay masteringDisplayInfo;
  QtiContentLightLevel contentLightLevel;
  QtiColorRemappingInfo cRI;
  QtiDynamicMetadata dynamicMetadata = { .dynamicMetaDataValid = false, .dynamicMetaDataLen = 0 };
  QtiAnamorphicMetadata anamorphicMetadata;
};

// This enum represents buffer layout types.
enum BufferLayout {
  kLinear,    //!< Linear data
  kUBWC,      //!< UBWC aligned data
  kTPTiled    //!< Tightly Packed data
};

/*! @brief This structure defines a rectanglular area inside a display layer.

  @sa LayerRectArray
*/
struct LayerRect {
  float left   = 0.0f;   //!< Left-most pixel coordinate.
  float top    = 0.0f;   //!< Top-most pixel coordinate.
  float right  = 0.0f;   //!< Right-most pixel coordinate.
  float bottom = 0.0f;   //!< Bottom-most pixel coordinate.

  LayerRect() = default;

  LayerRect(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) { }

  bool operator==(const LayerRect& rect) const {
    return left == rect.left && right == rect.right && top == rect.top && bottom == rect.bottom;
  }

  bool operator!=(const LayerRect& rect) const {
    return !operator==(rect);
  }
};

/*! @brief This enum represents the Tappoints for CWB that are supported by the hardware. */
enum CwbTapPoint {
  kLmTapPoint,      // This is set by client to use Layer Mixer output for CWB.
  kDsppTapPoint,    // This is set by client to use DSPP output for CWB.
  kDemuraTapPoint,  // This is set by client to use Demura output for CWB.
};

/*! @brief This structure defines the configuration variables needed to perform CWB.

  @sa LayerStack
*/
struct CwbConfig {
  bool pu_as_cwb_roi = false;                        //!< Whether to include the PU ROI generated
                                                     //!< from app layers in CWB ROI.
  LayerRect cwb_roi = {};                            //!< Client specified ROI rect for CWB.
  LayerRect cwb_full_rect = {};                      //!< Same as Output buffer Rect (unaligned).
  CwbTapPoint tap_point = CwbTapPoint::kLmTapPoint;  //!< Client specified tap point for CWB.
  void *dither_info = nullptr;                       //!< Pointer to the cwb dither setting.
  bool avoid_refresh = false;                        //!< Whether to avoid additional refresh for
                                                     //!< CWB Request, by default refresh occurs
                                                     //!< for each CWB request to process it.
};

class LayerBufferObject {
 public:
  virtual ~LayerBufferObject() {}
};

}  // namespace sdm

#endif  // __LAYER_BUFFER_H__
