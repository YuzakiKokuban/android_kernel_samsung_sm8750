/*
* Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
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
*
* Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

/*! @file layer_stack.h
  @brief File for display layer stack structure which represents a drawing buffer.

  @details Display layer is a drawing buffer object which will be blended with other drawing buffers
  under blending rules.
*/
#ifndef __LAYER_STACK_H__
#define __LAYER_STACK_H__

#include <stdint.h>
#include <utils/constants.h>
#include <utils/fence.h>

#include <vector>
#include <utility>
#include <unordered_map>
#include <memory>
#include <bitset>
#include <string>
#include <tuple>

#include "layer_buffer.h"
#include "sdm_types.h"
#include <color_metadata.h>

namespace sdm {
// clang-format off

/*! @brief This enum represents display layer blending types.

  @sa Layer
*/
enum LayerBlending {
  kBlendingPremultiplied,   //!< Pixel color is expressed using premultiplied alpha in RGBA tuples.
                            //!< If plane alpha is less than 0xFFFF, apply modulation as well.
                            //!<   pixel.rgb = src.rgb + dest.rgb x (1 - src.a)

  kBlendingOpaque,          //!< Pixel color is expressed using straight alpha in color tuples. It
                            //!< is constant blend operation. The layer would appear opaque if plane
                            //!< alpha is 0xFFFF.

  kBlendingCoverage,        //!< Pixel color is expressed using straight alpha in color tuples. If
                            //!< plane alpha is less than 0xffff, apply modulation as well.
                            //!<   pixel.rgb = src.rgb x src.a + dest.rgb x (1 - src.a)

  kBlendingSkip,            //!< Used only to denote layer should not be staged for blending, but
                            //!< still requires fetch resources for a different HW block
};

/*! @brief This enum represents display layer composition types.

  @sa Layer
*/
enum LayerComposition {
  /* ==== List of composition types set by SDM === */
  /* These composition types represent SDM composition decision for the layers which need to
     be blended. Composition types are set during Prepare() by SDM.
     Client can set default composition type to any of the below before calling into Prepare(),
     however client's input value is ignored and does not play any role in composition decision.
  */
  kCompositionGPU,          //!< This layer will be drawn onto the target buffer by GPU. Display
                            //!< device will mark the layer for GPU composition if it can not
                            //!< handle composition for it.
                            //!< This composition type is used only if GPUTarget layer is provided
                            //!< in a composition cycle.

  kCompositionStitch,       //!< This layer will be drawn onto the target buffer by GPU. No blend
                            //!< required.

  kCompositionDemura,       //!< This layer will be applied by Demura HW. No blend required.

  kCompositionSDE,          //!< This layer will be composed by SDE. It must not be composed by
                            //!< GPU or Blit.

  kCompositionCursor,       // This cursor layer can receive async position updates irrespective of
                            // dedicated h/w cursor usage. It must not be composed by GPU or Blit

  kCompositionNone,         //!< This layer will not be composed by any hardware.

  /* === List of composition types set by Client === */
  /* These composition types represent target buffer layers onto which GPU or Blit will draw if SDM
     decide to have some or all layers drawn by respective composition engine.
     Client must provide a target buffer layer, if respective composition type is not disabled by
     an explicit call to SetCompositionState() method. If a composition type is not disabled,
     providing a target buffer layer is optional. If SDM is unable to handle layers without support
     of such a composition engine, Prepare() call will return failure.
  */
  kCompositionGPUTarget,    //!< This layer will hold result of composition for layers marked for
                            //!< GPU composition.
                            //!< If display device does not set any layer for GPU composition then
                            //!< this layer would be ignored. Else, this layer will be composed
                            //!< with other layers marked for SDE composition by SDE.
                            //!< Only one layer shall be marked as target buffer by the caller.
                            //!< GPU target layer shall be placed after all application layers
                            //!< in the layer stack.

  kCompositionStitchTarget,  //!< This layer will hold result of composition for layers marked fo
                             //!< Blit composition.

  kCompositionCWBTarget,     //!< This layer will hold result of composition for layers marked for
                             //!< CWB composition in case of Idle fallback.
  kCompositionIWE,           //!< This layer will hold the result of first pass composition.
};

enum LayerUpdate {
  kSecurity,
  kMetadataUpdate,
  kSurfaceDamage,
  kSurfaceInvalidate,
  kClientCompRequest,
  kColorTransformUpdate,
  kContentMetadata,
  kLayerUpdateMax,
};

enum GeometryChanges {
  kNone = 0x000,
  kBlendMode = 0x001,
  kDataspace = 0x002,
  kDisplayFrame = 0x004,
  kPlaneAlpha = 0x008,
  kSourceCrop = 0x010,
  kTransform = 0x020,
  kZOrder = 0x040,
  kAdded = 0x080,
  kRemoved = 0x100,
  kBufferGeometry = 0x200,
  kColorTransform = 0x400,
  kLayerBrightness = 0x800,
  kDefault = 0xFFFF,
};

/*! @brief This structure defines rotation and flip values for a display layer.

  @sa Layer
*/
struct LayerTransform {
  float rotation = 0.0f;  //!< Left most pixel coordinate.
  bool flip_horizontal = false;  //!< Mirror reversal of the layer across a horizontal axis.
  bool flip_vertical = false;  //!< Mirror reversal of the layer across a vertical axis.

  bool operator==(const LayerTransform& transform) const {
    return (rotation == transform.rotation && flip_horizontal == transform.flip_horizontal &&
            flip_vertical == transform.flip_vertical);
  }

  bool operator!=(const LayerTransform& transform) const {
    return !operator==(transform);
  }
};

/*! @brief This structure defines flags associated with a layer. The 1-bit flag can be set to ON(1)
  or OFF(0).

  @sa LayerBuffer
*/
struct LayerFlags {
  union {
    struct {
      uint32_t skip : 1;      //!< This flag shall be set by client to indicate that this layer
                              //!< will be handled by GPU. Display Device will not consider it
                              //!< for composition.

      uint32_t updating : 1;  //!< This flag shall be set by client to indicate that this is
                              //!< updating non-updating. so strategy manager will mark them for
                              //!< SDE/GPU composition respectively when the layer stack qualifies
                              //!< for cache based composition.

      uint32_t solid_fill : 1;
                              //!< This flag shall be set by client to indicate that this layer
                              //!< is for solid fill without input buffer. Display Device will
                              //!< use SDE HW feature to achieve it.

      uint32_t cursor : 1;    //!< This flag shall be set by client to indicate that this layer
                              //!< is a cursor
                              //!< Display Device may handle this layer using cursor

      uint32_t single_buffer : 1;
                              //!< This flag shall be set by client to indicate that the layer
                              //!< uses only a single buffer that will not be swapped out

      uint32_t color_transform : 1;
                              //!< This flag will be set by SDM when the layer has a custom matrix

      uint32_t is_game : 1;   //!< This flag shall be set by client to indicate that this layer
                              //!< is a game layer.

      uint32_t sde_preferred : 1;
                              //! This flag shall be set by client to indicate that this layer
                              //! will be composed by display device, layer with this flag
                              //! will have highest priority. To be used by OEMs only.

      uint32_t is_demura : 1;
                              //!< This flag shall be set to indicate that this layer
                              //!< is a demura correction layer

      uint32_t is_abc : 1;
                              //!< This flag shall be set to indicate that this layer
                              //!< is an abc correction layer

      uint32_t compatible : 1;
                              //!< This flag shall be set by client to indicate that this layer
                              //!< can be composed into the gpu target buffer that is passed along
                              //!< with the current draw cycle.

      uint32_t is_noise : 1;  //!< This flag shall be set by SDM to indicate this layer as noise

      uint32_t is_cwb : 1;    //!< This flag shall be set by SDM to indicate that this layer is
                              //!< CWB output layer

      uint32_t reserved1 : 1;
                              //!< This flag is reserved(1) for private usage
      uint32_t reserved2 : 1;
                              //!< This flag is reserved(2) for private usage
      uint32_t reserved3 : 1;
                              //!< This flag is reserved(3) for private usage
      uint32_t reserved4 : 1;
                              //!< This flag is reserved(4) for private usage
      uint32_t has_metadata_refresh_rate : 1;
                              //!< This flag is used to mark if layer uses metadata refresh rate
      uint32_t skip_iwe : 1;
                              //!< This flag shall be set to indicate that this layer
                              //!< is handled by IWE for two phase composition.
      uint32_t front_buffer : 1;
                              //!< This flag shall be set by client to indicate that the layer
                              //!< is used for front-buffer rendering
    };

    uint32_t flags = 0;       //!< For initialization purpose only.
                              //!< Client shall not refer it directly.
  };
};

/*! @brief This structure defines flags associated with the layer requests. The 1-bit flag can be
    set to ON(1) or OFF(0).

  @sa Layer
*/
struct LayerRequestFlags {
  union {
    struct {
      uint32_t tone_map : 1;  //!< This flag will be set by SDM when the layer needs tone map
      uint32_t secure: 1;  //!< This flag will be set by SDM when the layer must be secure
      uint32_t flip_buffer: 1;  //!< This flag will be set by SDM when the layer needs FBT flip
      uint32_t dest_tone_map : 1;  //!< This flag will be set by SDM when the layer needs
                                   //!< destination tone map
      uint32_t src_tone_map: 1;    //!< This flag will be set by SDM when the layer needs
                                   //!< source tone map.
      uint32_t rc: 1;  //!< This flag will be set by SDM when the layer is drawn by RC HW.
      uint32_t update_format: 1;   //!< This flag will be set by SDM when layer format is updated
                                   //!< The buffer format is mentioned in LayerRequest
      uint32_t update_color_metadata: 1;  //!< This flag will be set by SDM when layer color
                                          //!< metadata is updated. The color metadata is
                                          //!< mentioned in LayerRequest
      uint32_t external_tone_map : 1;  //!< This flag will be set by SDM when the layer needs to be
                                       //!< processed for external tone mapping
    };
    uint32_t request_flags = 0;  //!< For initialization purpose only.
                                 //!< Shall not be refered directly.
  };
};

/*! @brief This structure defines LayerRequest.
   Includes width/height/format of the LayerRequest.

   SDM shall set the properties of LayerRequest to be used by the client

  @sa LayerRequest
*/
struct LayerRequest {
  LayerRequestFlags flags;  // Flags associated with this request
  LayerBufferFormat format = kFormatRGBA8888;  // Requested format
  Dataspace dataspace = { .colorPrimaries = QtiColorPrimaries_BT709_5,
                                   .range = QtiRange_Full,
                                   .transfer = QtiTransfer_sRGB };
  QtiMatrixCoEfficients matrixCoefficients;
  QtiMasteringDisplay masteringDisplayInfo;
  QtiContentLightLevel contentLightLevel;
  QtiColorRemappingInfo cRI;
  QtiDynamicMetadata dynamicMetadata;
                                  // Requested color metadata
  uint32_t width = 0;  // Requested unaligned width.
  uint32_t height = 0;  // Requested unalighed height
};

/*! @brief This structure defines flags associated with a layer stack. The 1-bit flag can be set to
  ON(1) or OFF(0).

  @sa LayerBuffer
*/
struct LayerStackFlags {
  union {
    struct {
      uint32_t geometry_changed : 1;  //!< This flag shall be set by client to indicate that the
                                      //!< layer set passed to Prepare() has changed by more than
                                      //!< just the buffer handles and acquire fences.

      uint32_t skip_present : 1;      //!< This flag will be set to true, if the current layer
                                      //!< stack contains skip layers.

      uint32_t video_present : 1;     //!< This flag will be set to true, if current layer stack
                                      //!< contains video.

      uint32_t secure_present : 1;    //!< This flag will be set to true, if the current layer
                                      //!< stack contains secure layers.

      uint32_t animating : 1;         //!< This flag shall be set by client to indicate that the
                                      //!<  current frame is animating.i

      uint32_t attributes_changed : 1;
                                      //!< This flag shall be set by client to indicate that the
                                      //!< current frame has some properties changed and
                                      //!< needs re-config.

      uint32_t cursor_present : 1;    //!< This flag will be set to true if the current layer
                                      //!< stack contains cursor layer.

      uint32_t single_buffered_layer_present : 1;    //!< Set if stack has single buffered layer

      uint32_t s3d_mode_present : 1;  //!< This flag will be set to true, if the current layer
                                      //!< stack contains s3d layer, and the layer stack can enter
                                      //!< s3d mode.

      uint32_t post_processed_output : 1;  //!< If output_buffer should contain post processed
                                           //!< output. This flag is set to 1 for DSPP tap point
                                           //!< and 0 for LM tap point. This flag is set to 1 for
                                           //!< Demura tap point also, but then SDM must use
                                           //!< cwb_config.tap_point (in LayerStack) only for
                                           //!< recognizing the tappoint.

      uint32_t hdr_present : 1;  //!< Set if stack has HDR content

      uint32_t mask_present : 1;  //!< Set if layer stack has mask layers.

      uint32_t advance_fb_present : 1;  //!< Set if layer stack has next frame buffer set.

      uint32_t layer_id_support : 1;  //! This flag shall be set by Client to indicate that it has
                                      //! set the unique Layer Id on each SDM Layer, which will
                                      //! persist across draw cycles until the layer gets removed.

      uint32_t stitch_present : 1;  //!< This flag shall be set to true to indicate stack has stitch

      uint32_t demura_present : 1;  //!< This flag shall be set to true to indicate stack has demura

      uint32_t noise_present : 1;  //!< Set if stack has noise layer

      uint32_t reserved1 : 1;  //!< This flag is reserved(1) for private usage

      uint32_t reserved2 : 1;  //!< This flag is reserved(2) for private usage

      uint32_t reserved3 : 1;  //!< This flag is reserved(3) for private usage

      uint32_t reserved4 : 1;  //!< This flag is reserved(4) for private usage

      uint32_t scaling_rgb_layer_present : 1;  //!< This flag indicates scaling rgb layer presence

      bool use_metadata_refresh_rate : 1;

      uint32_t iwe_present : 1;  //!< This flag shall be set to true to indicate stack has iwe layer

      bool default_strategy: 1;  //!< This flag indicates the default strategy usage.

      uint32_t front_buffer_layer_present : 1;  //!< Set if stack has front buffer layer.

      uint32_t only_video_updating : 1;  //!< This flag indicates only video layers are updating
    };

    uint32_t flags = 0;               //!< For initialization purpose only.
                                      //!< Client shall not refer it directly.
  };
};

/*! @brief This structure defines an array of display layer rectangles.

  @sa LayerRect
*/
struct LayerRectArray {
  LayerRect *rect = NULL;  //!< Pointer to first element of array.
  uint32_t count = 0;      //!< Number of elements in the array.
};

struct LayerStitchInfo {
  LayerRect dst_rect = {};          //!< The target position where the frame will be
                                    //!< rendered onto internal FrameBuffer.

  LayerRect slice_rect = {};        //!<  Target slice that this stitch rect belongs to.
};

/*! @brief This structure defines solidfill structure.

  @sa LayerSolidFill
*/
struct LayerSolidFill {
  uint32_t bit_depth = 0;  //!< Bit depth of solid fill colors
  uint32_t red = 0;        //!< Red value
  uint32_t green = 0;      //!< Green value
  uint32_t blue = 0;       //!< Blue value
  uint32_t alpha = 0;      //!< Alpha value
};

struct LayerBufferMap {
  std::unordered_map<uint64_t, std::unordered_map<uint32_t,
                               std::vector<std::shared_ptr<LayerBufferObject>>>> buffer_map;
};

/*! @brief This structure defines display layer object which contains layer properties and a drawing
  buffer.

  @sa LayerArray
*/
struct Layer {
  LayerBuffer input_buffer = {};                   //!< Buffer to be composed.
                                                   //!< If this remains unchanged between two
                                                   //!< consecutive Prepare() calls and
                                                   //!< geometry_changed flag is not set for the
                                                   //!< second call, then the display device will
                                                   //!< assume that buffer content has not
                                                   //!< changed.

  LayerComposition composition = kCompositionGPU;  //!< Composition type which can be set by either
                                                   //!< the client or the display device. This value
                                                   //!< should be preserved between Prepare() and
                                                   //!< Commit() calls.

  LayerRect src_rect = {};                         //!< Rectangular area of the layer buffer to
                                                   //!< consider for composition.

  LayerRect dst_rect = {};                         //!< The target position where the frame will be
                                                   //!< displayed. Cropping rectangle is scaled to
                                                   //!< fit into this rectangle. The origin is the
                                                   //!< top-left corner of the screen.

  LayerStitchInfo stitch_info = {};                //!< This structure defines all parameters needed
                                                   //!< for stitching like position to render,
                                                   //!< boundaries etc;

  std::vector<LayerRect> visible_regions = {};     //!< Visible rectangular areas in screen space.
                                                   //!< The visible region includes areas overlapped
                                                   //!< by a translucent layer.

  std::vector<LayerRect> dirty_regions = {};       //!< Rectangular areas in the current frames
                                                   //!< that have changed in comparison to
                                                   //!< previous frame.

  LayerBlending blending = kBlendingPremultiplied;  //!< Blending operation which need to be
                                                    //!< applied on the layer buffer during
                                                    //!< composition.

  LayerTransform transform = {};                   //!< Rotation/Flip operations which need to be
                                                   //!< applied to the layer buffer during
                                                   //!< composition.

  uint16_t plane_alpha = 0xffff;                   //!< Alpha value applied to the whole layer.
                                                   //!< Value of each pixel is computed as:
                                                   //!<  if(kBlendingPremultiplied) {
                                                   //!<    pixel.RGB = pixel.RGB * planeAlpha/65535
                                                   //!<  }
                                                   //!<  pixel.a = pixel.a * planeAlpha

  uint32_t frame_rate = 0;                         //!< Rate at which frames are being updated for
                                                   //!< this layer.

  uint32_t solid_fill_color = 0;                   //!< TODO: Remove this field when fb support
                                                   //!  is deprecated.
                                                   //!< Solid color used to fill the layer when
                                                   //!< no content is associated with the layer.

  LayerFlags flags;                                //!< Flags associated with this layer.

  LayerRequest request = {};                       //!< o/p - request on this Layer by SDM.

  Lut3d lut_3d = {};                               //!< o/p - Populated by SDM when tone mapping is
                                                   //!< needed on this layer.
  LayerSolidFill solid_fill_info = {};             //!< solid fill info along with depth.
  std::shared_ptr<LayerBufferMap> buffer_map = nullptr;  //!< Map of handle_id and fb_id.
  float color_transform_matrix[kColorTransformMatrixSize] = { 1.0, 0.0, 0.0, 0.0,
                                                              0.0, 1.0, 0.0, 0.0,
                                                              0.0, 0.0, 1.0, 0.0,
                                                              0.0, 0.0, 0.0, 1.0 };
  std::bitset<kLayerUpdateMax> update_mask = 0;

  uint32_t geometry_changes = GeometryChanges::kDefault;

  uint64_t layer_id = 0;                           //!< A Unique Layer Id which will persist across
                                                   //!< frames until layer gets removed from stack,
                                                   //!< if LayerStackFlag layer_id_support is True.

  std::string layer_name = "";                     //!< Layer full name
  float layer_brightness = 1.0;                    //!< Layer brightness
};

/*! @brief This structure defines the color space + transfer of a given layer.

  @sa PrimariesTransfer
*/

struct PrimariesTransfer {
  QtiColorPrimaries primaries = QtiColorPrimaries_BT709_5;
  QtiGammaTransfer transfer = QtiTransfer_sRGB;

  bool operator==(const PrimariesTransfer& blend_cs) const {
    return ((primaries == blend_cs.primaries) && (transfer == blend_cs.transfer));
  }
};

struct CacConfig {
  double k0r = 0;
  double k1r = 0;
  double k0b = 0;
  double k1b = 0;
  double pixel_pitch = 0;
  double normalization = 0;
  uint32_t mid_le_y_offset = 0;
  uint32_t mid_le_x_offset = 0;
  uint32_t mid_re_y_offset = 0;
  uint32_t mid_re_x_offset = 0;
  uint32_t skip_inc = 0;
};

/*! @brief This structure defines a layer stack that contains layers which need to be composed and
  rendered onto the target.

  @sa DisplayInterface::Prepare
  @sa DisplayInterface::Commit
*/

struct LayerStackRequestFlags {
  union {
    struct {
      uint32_t trigger_refresh : 1;
    };
    uint32_t request_flags = 0;  //!< For initialization purpose only.
                                 //!< Shall not be refered directly.
  };
};

struct LayerStack {
  std::vector<Layer *> layers = {};    //!< Vector of layer pointers.

  shared_ptr<Fence> retire_fence = nullptr;
                                       //!< File descriptor referring to a sync fence object which
                                       //!< will be signaled when this composited frame has been
                                       //!< replaced on screen by a subsequent frame on a physical
                                       //!< display. The fence object is created and returned during
                                       //!< Commit(). Client shall close the returned file
                                       //!< descriptor.
                                       //!< NOTE: This field applies to a physical display only.

  std::shared_ptr<LayerBuffer> output_buffer = nullptr;
                                       //!< Pointer to the buffer where composed buffer would be
                                       //!< rendered for virtual displays.
                                       //!< NOTE: This field applies to a virtual display only.

  LayerStackFlags flags;               //!< Flags associated with this layer set.


  PrimariesTransfer blend_cs = {};     //!< o/p - Blending color space of the frame, updated by SDM

  uint64_t elapse_timestamp = 0;       //!< system time until which display commit needs to be held

  bool block_on_fb = true;             //!< Indicates if there is a need to block
                                       //!< on GPU composed o/p.

  bool needs_validate = false;         //!< Change in mode/colospace/fps etc
  bool solid_fill_enabled = false;
  bool tonemapper_active  = false;
  CwbConfig *cwb_config = NULL;        //!< Struct that contains the original CWB configuration
                                       //!< provided by CWB client.
  int32_t cwb_id = -1;                 //!< Populate writeback connector ID allocated for CWB.
  bool validate_only = false;
  bool client_incompatible = false;    //!< Flag to disable async commit when client target is
                                       //!< not compatible.

  LayerStackRequestFlags request_flags;  //!< request flags on this LayerStack by SDM.

  uint32_t force_refresh_rate = 0;

  uint64_t expected_present_time = 0;  //!< Expected Present timestamp for current frame.

  uint32_t frame_interval_ns = 0;  //!< Frame Interval for current frame.
};

// clang-format on
}  // namespace sdm

#endif  // __LAYER_STACK_H__
