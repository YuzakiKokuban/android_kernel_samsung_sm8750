/*
* Copyright (c) 2014 - 2019, 2021-2021, The Linux Foundation. All rights reserved.
*
* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

/*
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

/*! @file sdm_types.h
  @brief This file contains miscellaneous data types used across display interfaces.
*/
#ifndef __SDM_TYPES_H__
#define __SDM_TYPES_H__

#include <vector>
#include <utility>
#include <string>

#include <utils/sync_task.h>
#include <utils/fence.h>

namespace sdm {
// clang-format off

constexpr int32_t kDataspaceSaturationMatrixCount = 16;
constexpr int32_t kDataspaceSaturationPropertyElements = 9;
constexpr int32_t kPropertyMax = 256;

/*! @brief This enum represents different error codes that display interfaces may return.
*/
enum DisplayError {
  kErrorNone,             //!< Call executed successfully.
  kErrorUndefined,        //!< An unspecified error has occured.
  kErrorNotSupported,     //!< Requested operation is not supported.
  kErrorPermission,       //!< Operation is not permitted in current state.
  kErrorVersion,          //!< Client is using advanced version of interfaces and calling into an
                          //!< older version of display library.
  kErrorDataAlignment,    //!< Client data structures are not aligned on naturual boundaries.
  kErrorInstructionSet,   //!< 32-bit client is calling into 64-bit library or vice versa.
  kErrorParameters,       //!< Invalid parameters passed to a method.
  kErrorFileDescriptor,   //!< Invalid file descriptor.
  kErrorMemory,           //!< System is running low on memory.
  kErrorResources,        //!< Not enough hardware resources available to execute call.
  kErrorHardware,         //!< A hardware error has occured.
  kErrorTimeOut,          //!< The operation has timed out to prevent client from waiting forever.
  kErrorShutDown,         //!< Driver is processing shutdown sequence
  kErrorPerfValidation,   //!< Bandwidth or Clock requirement validation failure.
  kErrorNoAppLayers,      //!< No App layer(s) in the draw cycle.
  kErrorRotatorValidation,  //!< Rotator configuration validation failure.
  kErrorNotValidated,     //!< Draw cycle has not been validated.
  kErrorCriticalResource,   //!< Critical resource allocation has failed.
  kErrorDeviceRemoved,    //!< A device was removed unexpectedly.
  kErrorDriverData,       //!< Expected information from the driver is missing.
  kErrorDeferred,         //!< Call's intended action is being deferred to a later time.
  kErrorNeedsCommit,      //!< Display is expecting a Commit() to be issued.
  kErrorNeedsValidate,    //!< Validate Phase is needed for this draw cycle.
  kErrorNeedsLutRegen,    //!< Tonemapping LUT regen is needed for this draw cycle.
  kErrorNeedsQosRecalc,   //!< QoS data recalculation is needed for this draw cycle.
  kErrorNeedsQosRecalcAndLutRegen,  //!< QoS data recalculation and Tonemapping LUT regen is needed
                                    //   for this draw cycle.
  kSeamlessNotAllowed,    //!< Seemless switch between configs not allowed.
  kErrorDeviceBusy,       //!< Device is currently busy with other tasks.
  kErrorTryAgain,         //!< Try the task again.
};

/*! @brief This structure is defined for client and library compatibility check purpose only. This
  structure is used in SDM_VERSION_TAG definition only. Client should not refer it directly for
  any purpose.
*/
struct SDMCompatibility {
  char c1;
  int i1;
  char c2;
  int i2;
};

/*! @brief This enum represents different modules/logical unit tags that a log message may
  be associated with. Client may use this to filter messages for dynamic logging.

*/
enum DebugTag {
  kTagNone,             //!< Debug log is not tagged. This type of logs should always be printed.
  kTagResources,        //!< Debug log is tagged for resource management.
  kTagStrategy,         //!< Debug log is tagged for strategy decisions.
  kTagCompManager,      //!< Debug log is tagged for composition manager.
  kTagDriverConfig,     //!< Debug log is tagged for driver config.
  kTagRotator,          //!< Debug log is tagged for rotator.
  kTagScalar,           //!< Debug log is tagged for Scalar Helper.
  kTagQDCM,             //!< Debug log is tagged for display QDCM color managing.
  kTagQOSClient,        //!< Debug log is tagged for Qos client.
  kTagDisplay,          //!< Debug log is tagged for display core logs.
  kTagClient,           //!< Debug log is tagged for SDM client.
  kTagQOSImpl,          //!< Debug log is tagged for Qos library Implementation.
  kTagStitchBuffer,     //!< Debug log is tagged for Stitch Buffer Implementation.
  kTagCpuHint,          //!< Debug log is tagged for CPU hint Implementation.
  kTagCwb,              //!< Debug log is tagged for CWB buffer manager.
  kTagIWE,              //!< Debug log is tagged for IWE Implementation.
  kTagWbUsage,          //!< Debug log is tagged for writeback block usage Implementation.
};

typedef std::vector<std::pair<std::string, std::string>> ColorModeAttributeVal;

enum VariableVSync {
  NONE,
  CONTINUOUS,
  ONE_SHOT,
  ONE_SHOT_CONTINUOUS,
  VARIABLE_VSYNC_MAX,
};

struct ColorModeInfo {
};

/*! @brief This enum represents display device types where contents can be rendered.

  @sa CoreInterface::CreateDisplay
  @sa CoreInterface::IsDisplaySupported
*/
enum SDMDisplayType {
  kPrimary,             //!< Main physical display which is attached to the handheld device.
  kBuiltIn = kPrimary,  //!< Type name for all non-detachable physical displays. Use kBuiltIn
                        //!< instead of kPrimary.
  kHDMI,                //!< HDMI physical display which is generally detachable.
  kPluggable = kHDMI,   //!< Type name for all pluggable physical displays. Use kPluggable
                        //!< instead of kHDMI.
  kVirtual,             //!< Contents would be rendered into the output buffer provided by the
                        //!< client e.g. wireless display.
  kDisplayMax,
  kDisplayTypeMax = kDisplayMax
};

struct SDMDisplayInfo {
  uint64_t display_id = -1;            // sdm ID of the display
  SDMDisplayType type = kDisplayMax;   // Display type
  bool connected = false;              // connected/disconnected
  bool is_leader = false;              // Is leader display
  bool tonemap_supported = false;      // Display supports tonemapping
  uint64_t dynamic_display_clock;      // Clock rate of display's interface
};

enum DisplayState {
  kStateOff,        //!< Display is OFF. Contents are not rendered in this state. Client will not
                    //!< receive VSync events in this state. This is default state as well.

  kStateOn,         //!< Display is ON. Contents are rendered in this state.

  kStateDoze,       //!< Display is ON and it is configured in a low power state.

  kStateDozeSuspend,
                    //!< Display is ON in a low power state and continue showing its current
                    //!< contents indefinitely until the mode changes.

  kStateStandby,    //!< Display is OFF. Client will continue to receive VSync events in this state
                    //!< if VSync is enabled. Contents are not rendered in this state.
};

enum SDMPresentResult {
    SDM_PRESENT_TYPE_SUCCESS,
    SDM_PRESENT_TYPE_NEED_FBT,
    SDM_PRESENT_TYPE_NEED_FENCE_BIND,
};

// Subclasses set this to their type. This has to be different from SDMDisplayType.
// This is to avoid RTTI and dynamic_cast
enum DisplayClass {
  DISPLAY_CLASS_BUILTIN,
  DISPLAY_CLASS_PLUGGABLE,
  DISPLAY_CLASS_VIRTUAL,
  DISPLAY_CLASS_NULL
};

typedef uint32_t VsyncPeriodNanos;

struct SDMColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

struct SDMRect {
  int left = 0;            //!< Specifies the left coordinates of the pixel buffer
  int top = 0;             //!< Specifies the top coordinates of the pixel buffer
  int right = 0;           //!< Specifies the right coordinates of the pixel buffer
  int bottom = 0;          //!< Specifies the bottom coordinates of the pixel buffer

  bool operator==(const SDMRect& rect) const {
    return left == rect.left && right == rect.right && top == rect.top && bottom == rect.bottom;
  }

  bool operator!=(const SDMRect& rect) const {
    return !operator==(rect);
  }
};

struct SDMRegion {
  size_t num_rects;
  std::vector<SDMRect> rects;
};

typedef uint64_t Display;
typedef uint32_t Config;
typedef int64_t LayerId;
typedef int64_t nsecs_t;

static const int kNumBuiltIn = 4;
static const int kNumPluggable = 4;
static const int kNumVirtual = 4;
// Add 1 primary display which can be either a builtin or pluggable.
// Async powermode update requires dummy displays.
// Limit dummy displays to builtin/pluggable type for now.
static const int kNumRealDisplays = 1 + kNumBuiltIn + kNumPluggable + kNumVirtual;
static const int kNumDisplays =
    1 + kNumBuiltIn + kNumPluggable + kNumVirtual + 1 + kNumBuiltIn + kNumPluggable;

struct SDMVsyncPeriodChangeTimeline {
  int64_t newVsyncAppliedTimeNanos __attribute__ ((aligned(8)));
  bool refreshRequired __attribute__ ((aligned(1)));
  int64_t refreshTimeNanos __attribute__ ((aligned(8)));
};

enum SDMAlphaInterpretation {
  COVERAGE = 0,
  MASK = 1,
};

struct SDMVsyncPeriodChangeConstraints {
  int64_t desiredTimeNanos;
  bool seamlessRequired;
};

struct SDMClientTargetProperty {
  uint32_t pixel_format;
  uint32_t dataspace;
};

enum SDMBuiltInDisplayOps {
  SET_METADATA_DYN_REFRESH_RATE,
  SET_BINDER_DYN_REFRESH_RATE,
  SET_DISPLAY_MODE,
  SET_QDCM_SOLID_FILL_INFO,
  UNSET_QDCM_SOLID_FILL_INFO,
  SET_QDCM_SOLID_FILL_RECT,
  UPDATE_TRANSFER_TIME,
};

enum SDMDisplayStatus {
  kDisplayStatusInvalid = -1,
  kDisplayStatusOffline,
  kDisplayStatusOnline,
  kDisplayStatusPause,   // Pause + PowerOff
  kDisplayStatusResume,  // Resume + PowerOn
};

enum SDMCameraSmoothOp {
  OFF = 0,
  ON = 1,
};

enum SDMDisplayIntf {
  INVALID = 0,
  DEFAULT = 1,
  DSI = 2,
  DTV = 3,
  WRITEBACK = 4,
  LVDS = 5,
  EDP = 6,
  DP = 7,
};

struct SDMConfigAttributes {
  int32_t vsyncPeriod;
  int32_t xRes;
  int32_t yRes;
  float xDpi;
  float yDpi;
  SDMDisplayIntf panelType;
  bool isYuv;
};

enum SDMTUIEventType {
  TUI_EVENT_TYPE_NONE = 0,
  PREPARE_TUI_TRANSITION = 1,
  START_TUI_TRANSITION = 2,
  END_TUI_TRANSITION = 3,
};

enum SDMCompositionType {
  COMP_INVALID = 0,
  COMP_CLIENT = 1,
  COMP_DEVICE = 2,
  COMP_SOLID_COLOR = 3,
  COMP_CURSOR = 4,
  COMP_SIDEBAND = 5,
  COMP_DISPLAY_DECORATION = 6,
};

enum SDMTransform {
  TRANSFORM_NONE = 0,
  TRANSFORM_FLIP_H = (1 << 0) /* 1 */,
  TRANSFORM_FLIP_V = (1 << 1) /* 2 */,
  TRANSFORM_ROT_90 = (1 << 2) /* 4 */,
  TRANSFORM_ROT_180 = (TRANSFORM_FLIP_H | TRANSFORM_FLIP_V) /* 3 */,
  TRANSFORM_ROT_270 = ((TRANSFORM_FLIP_H | TRANSFORM_FLIP_V) | TRANSFORM_ROT_90) /* 7 */,
};

enum SDMLayerFlag {
  LAYER_FLAG_DEFAULT,
  LAYER_FLAG_COMPATIBLE,
};

enum SDMColorMode {
  COLOR_MODE_NATIVE                        = 0,
  COLOR_MODE_STANDARD_BT601_625            = 1,
  COLOR_MODE_STANDARD_BT601_625_UNADJUSTED = 2,
  COLOR_MODE_STANDARD_BT601_525            = 3,
  COLOR_MODE_STANDARD_BT601_525_UNADJUSTED = 4,
  COLOR_MODE_STANDARD_BT709                = 5,
  COLOR_MODE_DCI_P3                        = 6,
  COLOR_MODE_SRGB                          = 7,
  COLOR_MODE_ADOBE_RGB                     = 8,
  COLOR_MODE_DISPLAY_P3                    = 9,
  COLOR_MODE_BT2020                        = 10,
  COLOR_MODE_BT2100_PQ                     = 11,
  COLOR_MODE_BT2100_HLG                    = 12,
  COLOR_MODE_DISPLAY_BT2020                = 13
};

enum SDMRenderIntent {
  COLORIMETRIC = 0,
  ENHANCE = 1,
  TONE_MAP_COLORIMETRIC = 2,
  TONE_MAP_ENHANCE = 3
};

enum SDMPowerMode {
    POWER_MODE_OFF = 0,
    POWER_MODE_DOZE = 1,
    POWER_MODE_ON = 2,
    POWER_MODE_DOZE_SUSPEND = 3,
    POWER_MODE_ON_SUSPEND = 4,
};

enum SDMPixelFormat {
  PIXEL_FORMAT_UNSPECIFIED             = 0,
  PIXEL_FORMAT_RGBA_8888               = 0x1,
  PIXEL_FORMAT_RGBX_8888               = 0x2,
  PIXEL_FORMAT_RGB_888                 = 0x3,
  PIXEL_FORMAT_RGB_565                 = 0x4,
  PIXEL_FORMAT_BGRA_8888               = 0x5,
  PIXEL_FORMAT_YCBCR_422_SP            = 0x10,  // NV16
  PIXEL_FORMAT_YCRCB_420_SP            = 0x11,  // NV21
  PIXEL_FORMAT_YCBCR_422_I             = 0x14,  // YUY2
  PIXEL_FORMAT_RGBA_FP16               = 0x16,
  PIXEL_FORMAT_RAW16                   = 0x20,
  PIXEL_FORMAT_BLOB                    = 0x21,
  PIXEL_FORMAT_IMPLEMENTATION_DEFINED  = 0x22,
  PIXEL_FORMAT_YCBCR_420_888           = 0x23,
  PIXEL_FORMAT_RAW_OPAQUE              = 0x24,
  PIXEL_FORMAT_RAW10                   = 0x25,
  PIXEL_FORMAT_RAW12                   = 0x26,
  PIXEL_FORMAT_RGBA_1010102            = 0x2B,
  PIXEL_FORMAT_Y8                      = 0x20203859,
  PIXEL_FORMAT_Y16                     = 0x20363159,
  PIXEL_FORMAT_YV12                    = 0x32315659,
  PIXEL_FORMAT_DEPTH_16                = 0x30,
  PIXEL_FORMAT_DEPTH_24                = 0x31,
  PIXEL_FORMAT_DEPTH_24_STENCIL_8      = 0x32,
  PIXEL_FORMAT_DEPTH_32F               = 0x33,
  PIXEL_FORMAT_DEPTH_32F_STENCIL_8     = 0x34,
  PIXEL_FORMAT_STENCIL_8               = 0x35,
  PIXEL_FORMAT_YCBCR_P010              = 0x36,
  PIXEL_FORMAT_HSV_888                 = 0x37,
  PIXEL_FORMAT_R_8                     = 0x38,
  PIXEL_FORMAT_R_16_UINT               = 0x39,
  PIXEL_FORMAT_RG_1616_UINT            = 0x3a,
  PIXEL_FORMAT_RGBA_10101010           = 0x3b,
};

enum SDMClientCommitDone {
  kClientPartialUpdate,
  kClientIdlepowerCollapse,
  kClientTrustedUI,
  kClientMax
};

enum class SDMPerFrameMetadataKey : int32_t {
  DISPLAY_RED_PRIMARY_X = 0,
  DISPLAY_RED_PRIMARY_Y = 1,
  DISPLAY_GREEN_PRIMARY_X = 2,
  DISPLAY_GREEN_PRIMARY_Y = 3,
  DISPLAY_BLUE_PRIMARY_X = 4,
  DISPLAY_BLUE_PRIMARY_Y = 5,
  WHITE_POINT_X = 6,
  WHITE_POINT_Y = 7,
  MAX_LUMINANCE = 8,
  MIN_LUMINANCE = 9,
  MAX_CONTENT_LIGHT_LEVEL = 10,
  MAX_FRAME_AVERAGE_LIGHT_LEVEL = 11,
  HDR10_PLUS_SEI = 12
};

enum class SDMDisplayBasicType : int32_t {
  kInvalid = 0,
  kPhysical = 1,
  kVirtual = 2,
};

/* Display types and associated mask bits. */
enum {
  SDM_DISPLAY_PRIMARY = 0,
  SDM_DISPLAY_EXTERNAL = 1,  // HDMI, DP, etc.

  SDM_DISPLAY_EXTERNAL_2 = 2,
  SDM_DISPLAY_EXTERNAL_3 = 3,
  SDM_DISPLAY_EXTERNAL_4 = 4,

  SDM_DISPLAY_BUILTIN_2 = 5,
  SDM_DISPLAY_BUILTIN_3 = 6,
  SDM_DISPLAY_BUILTIN_4 = 7,

  SDM_DISPLAY_VIRTUAL = 8,

  SDM_NUM_PHYSICAL_DISPLAY_TYPES = 8,
  SDM_NUM_DISPLAY_TYPES = 9,
};

enum class SDMLayerRequest : int32_t {
  ClearClientTarget = 1 << 0,
};

enum class SDMDisplayRequest : int32_t {
  FlipClientTarget = 1 << 0,
  WriteClientTargetToOutput =  1 << 1,
};

enum SDMCapability {
  kCapabilityNone = 0,
  kSidebandStream = 1,
  kSkipClientColorTransform = 2,
  kPresentFenceIsNotReliable = 3,
  kSkipValidate = 4,
  kBootDisplayConfig = 5,
};

enum SDMFormatColorComponent {
  FORMAT_COMPONENT_0 = 1,
  FORMAT_COMPONENT_1 = 2,
  FORMAT_COMPONENT_2 = 4,
  FORMAT_COMPONENT_3 = 8,
};

enum SDMColorTransform {
  TRANSFORM_IDENTITY = 0,
  TRANSFORM_ARBITRARY_MATRIX = 1,
  TRANSFORM_VALUE_INVERSE = 2,
  TRANSFORM_GRAYSCALE = 3,
  TRANSFORM_CORRECT_PROTANOPIA = 4,
  TRANSFORM_CORRECT_DEUTERANOPIA = 5,
  TRANSFORM_CORRECT_TRITANOPIA = 6,
};

enum Hdr : int32_t {
  DOLBY_VISION = 1,
  HDR10 = 2,
  HLG = 3,
  HDR10_PLUS = 4
};

enum SDMLayerTypes {
  kLayerUnknown = 0,
  kLayerApp = 1,
  kLayerGame = 2,
  kLayerBrowser = 3,
};

// GL callbacks, moved here to be accessible from cb intf
enum class ColorConvertTaskCode : int32_t {
  kCodeGetInstance,
  kCodeBlit,
  kCodeReset,
  kCodeDestroyInstance,
};

struct ColorConvertBlitContext
    : public SyncTask<ColorConvertTaskCode>::TaskContext {
  void *src_hnd = nullptr;
  void *dst_hnd = nullptr;
  SDMRect src_rect = {};
  SDMRect dst_rect = {};
  shared_ptr<Fence> src_acquire_fence = nullptr;
  shared_ptr<Fence> dst_acquire_fence = nullptr;
  shared_ptr<Fence> release_fence = nullptr;
};

struct SDMStitchParams {
  void *src_hnd = nullptr;
  void *dst_hnd = nullptr;
  SDMRect src_rect;
  SDMRect dst_rect;
  SDMRect scissor_rect;
  shared_ptr<Fence> src_acquire_fence = nullptr;
  shared_ptr<Fence> dst_acquire_fence = nullptr;
};

enum class LayerStitchTaskCode : int32_t {
  kCodeGetInstance,
  kCodeStitch,
  kCodeDestroyInstance,
};

struct LayerStitchContext : public SyncTask<LayerStitchTaskCode>::TaskContext {
  std::vector<SDMStitchParams> stitch_params;
  shared_ptr<Fence> src_acquire_fence = nullptr;
  shared_ptr<Fence> dst_acquire_fence = nullptr;
  shared_ptr<Fence> release_fence = nullptr;
};

enum qdutilsDisplayType {
  DISPLAY_PRIMARY = 0,    // = SDM_DISPLAY_PRIMARY
  DISPLAY_EXTERNAL = 1,   // = SDM_DISPLAY_EXTERNAL
  DISPLAY_VIRTUAL = 2,    // = SDM_DISPLAY_VIRTUAL

  // Additional displays only for vendor client (e.g. pp) reference
  DISPLAY_BUILTIN_2 = 3,
  DISPLAY_EXTERNAL_2 = 4,
  DISPLAY_VIRTUAL_2 = 5,
};

enum MetadataOps {
  DISABLE_METADATA_DYN_REFRESH_RATE = 0,
  ENABLE_METADATA_DYN_REFRESH_RATE,
  SET_BINDER_DYNAMIC_REFRESH_RATE,
};

enum {
  SYSTEM_TIME_REALTIME = 0,   // system-wide realtime clock
  SYSTEM_TIME_MONOTONIC = 1,  // monotonic time since unspecified starting point
  SYSTEM_TIME_PROCESS = 2,    // high-resolution per-process clock
  SYSTEM_TIME_THREAD = 3,     // high-resolution per-thread clock
  SYSTEM_TIME_BOOTTIME = 4,   // same as SYSTEM_TIME_MONOTONIC, but including CPU suspend time
};

}  // namespace sdm

#endif  // __SDM_TYPES_H__

