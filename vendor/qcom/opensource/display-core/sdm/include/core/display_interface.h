/*
* Copyright (c) 2014 - 2021, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*! @file display_interface.h
  @brief Interface file for display device which represents a physical panel or an output buffer
  where contents can be rendered.

  @details Display device is used to send layer buffers for composition and get them rendered onto
  the target device. Each display device represents a unique display target which may be either a
  physical panel or an output buffer..
*/
#ifndef __DISPLAY_INTERFACE_H__
#define __DISPLAY_INTERFACE_H__

#include <private/cb_intf.h>
#include <private/display_event_proxy_intf.h>
#include <private/snapdragon_color_intf.h>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include "layer_stack.h"
#include "sdm_types.h"

namespace sdm {

typedef std::vector<std::pair<std::string, std::string>> AttrVal;

/*! @brief This enum represents states of a display device.

  @sa DisplayInterface::GetDisplayState
  @sa DisplayInterface::SetDisplayState
*/

/*! @brief This enum represents flags to override detail enhancer parameters.

  @sa DisplayInterface::SetDetailEnhancerData
*/
enum DetailEnhancerOverrideFlags {
  kOverrideDEEnable            = 0x1,     // Specifies to enable detail enhancer
  kOverrideDESharpen1          = 0x2,     // Specifies user defined Sharpening/smooth for noise
  kOverrideDESharpen2          = 0x4,     // Specifies user defined Sharpening/smooth for signal
  kOverrideDEClip              = 0x8,     // Specifies user defined DE clip shift
  kOverrideDELimit             = 0x10,    // Specifies user defined DE limit value
  kOverrideDEThrQuiet          = 0x20,    // Specifies user defined DE quiet threshold
  kOverrideDEThrDieout         = 0x40,    // Specifies user defined DE dieout threshold
  kOverrideDEThrLow            = 0x80,    // Specifies user defined DE low threshold
  kOverrideDEThrHigh           = 0x100,   // Specifies user defined DE high threshold
  kOverrideDEFilterConfig      = 0x200,   // Specifies user defined scaling filter config
  kOverrideDEBlend             = 0x400,   // Specifies user defined DE blend.
  kOverrideDELpfBlend          = 0x800,   // Specifies user defined DE LPF blend.
  kOverrideDEMax               = 0xFFFFFFFF,
};

/*! @brief This enum represents Y/RGB scaling filter configuration.

  @sa DisplayInterface::SetDetailEnhancerData
*/
enum ScalingFilterConfig {
  kFilterEdgeDirected,
  kFilterCircular,
  kFilterSeparable,
  kFilterBilinear,
  kFilterMax,
};

/*! @brief This enum represents the quality level of the content.

  @sa DisplayInterface::SetDetailEnhancerData
*/
enum ContentQuality {
  kContentQualityUnknown,  // Default: high artifact and noise
  kContentQualityLow,      // Low quality content, high artifact and noise,
  kContentQualityMedium,   // Medium quality, medium artifact and noise,
  kContentQualityHigh,     // High quality content, low artifact and noise
  kContentQualityMax,
};


/*! @brief This enum represents the type of the content.

  @sa DisplayInterface::SetDetailEnhancerData
*/
enum DeContentType {
  kContentTypeUnknown,
  kContentTypeVideo,
  kContentTypeGraphics,
  kContentTypeMax,
};

/*! @brief This enum represents the display port.

  @sa DisplayInterface::GetDisplayPort
*/
enum DisplayPort {
  kPortDefault,
  kPortDSI,        // Display is connected to DSI port.
  kPortDTV,        // Display is connected to DTV port
  kPortWriteBack,  // Display is connected to writeback port
  kPortLVDS,       // Display is connected to LVDS port
  kPortEDP,        // Display is connected to EDP port
  kPortDP,         // Display is connected to DP port.
};

/*! @brief This enum represents the events received by Display HAL. */
enum DisplayEvent {
  kIdleTimeout,             // Event triggered by Idle Timer.
  kThermalEvent,            // Event triggered by Thermal.
  kIdlePowerCollapse,       // Event triggered by Idle Power Collapse.
  kPanelDeadEvent,          // Event triggered by ESD.
  kDisplayPowerResetEvent,  // Event triggered by Hardware Recovery.
  kSyncInvalidateDisplay,   // Event triggered by Non-DrawCycle threads to Invalidate display.
  kPostIdleTimeout,         // Event triggered after entering idle.
  kVmReleaseDone,           // Event triggered after releasing the mdp hw to secondary vm.
};

/*! @brief This enum represents the secure events received by Display HAL. */
enum SecureEvent {
  kSecureDisplayStart,      // Client sets it to notify secure display session start
  kSecureDisplayEnd,        // Client sets it to notify secure display session end
  kTUITransitionPrepare,    // Client sets it to notify non targetted display to forcefully disable
                            // the display pipeline.
  kTUITransitionStart,      // Client sets it to notify start of TUI Transition to release
                            // the display hardware to trusted VM. Client calls only for
                            // target displays where TUI to be displayed
  kTUITransitionEnd,        // Client sets it to notify end of TUI Transition to acquire
                            // the display hardware from trusted VM. Client calls only for
                            // target displays where TUI to be displayed
  kTUITransitionUnPrepare,  // Client sets it to notify non targetted display to enable/disable the
                            // display pipeline based on pending power state
  kSecureEventMax,
};

/*! @brief This enum represents the QSync modes supported by the hardware. */
enum QSyncMode {
  kQSyncModeNone,               // This is set by the client to disable qsync
  kQSyncModeContinuous,         // This is set by the client to enable qsync forever
  kQsyncModeOneShot,            // This is set by client to enable qsync only for current frame.
  kQsyncModeOneShotContinuous,  // This is set by client to enable qsync only for every commit.
};

/*! @brief This enum represents different operating modes to submit a draw cycle to display.

  @sa DisplayInterface::SetDrawMethod
*/
enum DisplayDrawMethod {
  kDrawDefault,               //!< Prepare & Commit are performed in separate calls.
                              //!< Release fences returned through commit in this method will
                              //!< signal when the buffers submitted along with current draw
                              //!< cycle are consumed by the display.
                              //!< Retire fence returned through commit in this method will
                              //!< signal when the current composed buffers begin to display
                              //!< on panel.

  kDrawUnified,               //!< Prepare & Commit are unified. A separate Commit is performed
                              //!< in case unified draw cycle is not doable.
                              //!< Release fences returned through commit in this method will
                              //!< signal when the buffers submitted in the previous draw cycle
                              //!< are consumed by the display. If a new layer is submitted,
                              //!< the release fence will be set to -1.
                              //!< Retire fence returned through commit in this method will
                              //!< signal when the current composed buffers begin to display
                              //!< on panel.

  kDrawUnifiedWithGPUTarget,  //!< Prepare & Commit are unified. A deterministic gpu target
                              //!< buffer is passed with layer stack. A separate Commit is
                              //!< performed in case unified draw cycle is not doable.
                              //!< Release fences returned through commit in this method will
                              //!< signal when the buffers submitted in the previous draw cycle
                              //!< are consumed by the display. If a new layer is submitted,
                              //!< the release fence will be set to -1.
                              //!< Retire fence returned through commit in this method will
                              //!< signal when the current composed buffers begin to display
                              //!< on panel.
};

/*! @brief This structure defines configuration for display dpps ad4 region of interest. */
struct DisplayDppsAd4RoiCfg {
  uint32_t h_start;     //!< start in hotizontal direction
  uint32_t h_end;       //!< end in hotizontal direction
  uint32_t v_start;     //!< start in vertical direction
  uint32_t v_end;       //!< end in vertical direction
  uint32_t factor_in;   //!< the strength factor of inside ROI region
  uint32_t factor_out;  //!< the strength factor of outside ROI region
};

/*! @brief This enum defines frame trigger modes. */
enum FrameTriggerMode {
  kFrameTriggerDefault,      //!< Wait for pp_done of previous frame to trigger new frame
  kFrameTriggerSerialize,    //!< Trigger new frame and wait for pp_done of this frame
  kFrameTriggerPostedStart,  //!< Posted start mode, trigger new frame without pp_done
  kFrameTriggerMax,
};

/*! @brief This structure defines configuration for fixed properties of a display device.

  @sa DisplayInterface::GetConfig
  @sa DisplayInterface::SetConfig
*/
struct DisplayConfigFixedInfo {
  bool underscan = false;               //!< If display support CE underscan.
  bool secure = false;                  //!< If this display is capable of handling secure content.
  bool is_cmdmode = false;              //!< If panel is command mode panel.
  bool hdr_supported = false;           //!< If HDR10 is supported.
  bool hdr_plus_supported = false;      //!< If HDR10+ is supported.
  bool dolby_vision_supported = false;  //!< If Dolby Vision is supported.
  bool hdr_metadata_type_one = false;   //!< Metadata type one obtained from HDR sink
  uint32_t hdr_eotf = 0;                //!< Electro optical transfer function
  float max_luminance = 0.0f;           //!< From Panel's peak luminance
  float average_luminance = 0.0f;       //!< From Panel's average luminance
  float min_luminance = 0.0f;           //!< From Panel's blackness level
  bool partial_update = false;          //!< If display supports Partial Update.
  bool readback_supported = false;      //!< If display supports buffer readback.
  bool supports_unified_draw = false;   //!< If display support unified drawing methods.
};

/*! @brief This structure defines configuration for variable properties of a display device.

  @sa DisplayInterface::GetConfig
  @sa DisplayInterface::SetConfig
*/
struct DisplayConfigGroupInfo {
  uint32_t x_pixels = 0;          //!< Total number of pixels in X-direction on the display panel.
  uint32_t y_pixels = 0;          //!< Total number of pixels in Y-direction on the display panel.
  uint32_t h_total = 0;           //!< Total width of panel (hActive + hFP + hBP + hPulseWidth)
  uint32_t v_total = 0;           //!< Total height of panel (vActive + vFP + vBP + vPulseWidth)
  float x_dpi = 0.0f;             //!< Dots per inch in X-direction.
  float y_dpi = 0.0f;             //!< Dots per inch in Y-direction.
  bool is_yuv = false;            //!< If the display output is in YUV format.
  bool smart_panel = false;       //!< If the display config has smart panel.
  uint64_t allowed_mode_switch = 0;
  uint32_t avr_step = 0;  //!< AVR Step fps of the display panel.

  bool operator==(const DisplayConfigGroupInfo& info) const {
    return ((x_pixels == info.x_pixels) && (y_pixels == info.y_pixels) && (x_dpi == info.x_dpi) &&
            (y_dpi == info.y_dpi) && (is_yuv == info.is_yuv) && (smart_panel == info.smart_panel) &&
            (avr_step == info.avr_step));
  }
};

struct DisplayConfigVariableInfo : public DisplayConfigGroupInfo {
  uint32_t fps = 0;               //!< Frame rate per second.
  uint32_t vsync_period_ns = 0;   //!< VSync period in nanoseconds.
  bool is_virtual_config = false;
  int32_t parent_config_index = -1;   //!< if virtual config, then corresponding panel config
  uint32_t early_ept_timeout = 0;     //!< Early EPT timeout value ns

  bool operator==(const DisplayConfigVariableInfo& info) const {
    return ((x_pixels == info.x_pixels) && (y_pixels == info.y_pixels) &&
            (h_total == info.h_total) && (v_total == info.v_total) && (x_dpi == info.x_dpi) &&
            (y_dpi == info.y_dpi) && (fps == info.fps) &&
            (vsync_period_ns == info.vsync_period_ns) && (is_yuv == info.is_yuv) &&
            (smart_panel == info.smart_panel) && (early_ept_timeout == info.early_ept_timeout));
  }
};

/*! @brief Event data associated with VSync event.

  @sa DisplayEventHandler::VSync
*/
struct DisplayEventVSync {
  int64_t timestamp = 0;    //!< System monotonic clock timestamp in nanoseconds.
};

/*! @brief The structure defines the user input for detail enhancer module.

  @sa DisplayInterface::SetDetailEnhancerData
*/
struct DisplayDetailEnhancerData {
  uint32_t override_flags = 0;        // flags to specify which data to be set.
  uint16_t enable = 0;                // Detail enchancer enable
  int16_t sharpen_level1 = 0;         // Sharpening/smooth strenght for noise
  int16_t sharpen_level2 = 0;         // Sharpening/smooth strenght for signal
  uint16_t clip = 0;                  // DE clip shift
  uint16_t limit = 0;                 // DE limit value
  uint16_t thr_quiet = 0;             // DE quiet threshold
  uint16_t thr_dieout = 0;            // DE dieout threshold
  uint16_t thr_low = 0;               // DE low threshold
  uint16_t thr_high = 0;              // DE high threshold
  int32_t sharp_factor = 50;          // sharp_factor specifies sharpness/smoothness level,
                                      // range -100..100 positive for sharpness and negative for
                                      // smoothness
  ContentQuality quality_level = kContentQualityUnknown;
                                      // Specifies context quality level
  ScalingFilterConfig filter_config = kFilterEdgeDirected;
                                      // Y/RGB filter configuration
  uint32_t de_blend = 0;              // DE Unsharp Mask blend between High and Low frequencies
  DeContentType content_type = kContentTypeUnknown;  // Specifies content type
  bool de_lpf_en = false;
  uint32_t de_lpf_h;                  // Weight for DE Unsharp Mask LPF-High
  uint32_t de_lpf_m;                  // Weight for DE Unsharp Mask LPF-Mid
  uint32_t de_lpf_l;                  // Weight for DE Unsharp Mask LPF-Low
};

/*! @brief This enum represents the supported display features that needs to be queried

  @sa DisplayInterface::SupportedDisplayFeature
*/
enum SupportedDisplayFeature {
  kSupportedModeSwitch,
  kDestinationScalar,
  kCwbDemuraTapPoint,
  kCwbCrop,
  kDedicatedCwb,
  kCacV2,
};

/*! @brief This struct stores the state of Qsync

  @sa DisplayInterface::QsyncEventData
*/
struct QsyncEventData {
  bool enabled = false;
  uint32_t refresh_rate = 0;
  uint32_t qsync_refresh_rate = 0;
};

/*! @brief This struct stores the panel feature info

  @sa DisplayInterface::PanelFeatureInfo
*/
struct PanelFeatureInfo {
  bool spr_enable = false;
  bool spr_bypassed = false;
  uint32_t display_width = 0;
  uint32_t display_height = 0;
  std::string panel_name;
  uint32_t fps = 0;
};

/*! @brief This enum represents the panel feature cmd types supported by the vendService cmd.

  @sa DisplayInterface::PanelFeatureVendorServiceType
*/
enum PanelFeatureVendorServiceType {
  /* Setter: int */
  kTypeDemuraTnCWBSamplingPeriod = 0,
  /* Setter: int */
  kTypeDemuraTnEventsCtrl = 1,
  kTypeDemuraTnUserCtrl = 2,
  kTypeDeleteDemuraConfig = 3,
  kTypeDeleteDemuraTnConfig = 4,
  /* Setter: None */
  kTypeTriggerDemuraOemPlugIn = 5,
  PanelFeatureVendorServiceTypeMax,
};

/*! @brief Display device event handler implemented by the client.

  @details This class declares prototype for display device event handler methods which must be
  implemented by the client. Display device will use these methods to notify events to the client.
  Client must post heavy-weight event handling to a separate thread and unblock display manager
  thread instantly.

  @sa CoreInterface::CreateDisplay
*/
class DisplayEventHandler {
 public:
  /*! @brief Event handler for VSync event.

    @details This event is dispatched on every vertical synchronization. The event is disabled by
    default.

    @param[in] vsync \link DisplayEventVSync \endlink

    @return \link DisplayError \endlink

    @sa DisplayInterface::GetDisplayState
    @sa DisplayInterface::SetDisplayState
  */
  virtual DisplayError VSync(const DisplayEventVSync &vsync) = 0;

  /*! @brief Event handler for Refresh event.

    @details This event is dispatched to trigger a screen refresh. Client must call Prepare() and
    Commit() in response to it from a separate thread. There is no data associated with this
    event.

    @return \link DisplayError \endlink

    @sa DisplayInterface::Prepare
    @sa DisplayInterface::Commit
  */
  virtual DisplayError Refresh() = 0;

  /*! @brief Event handler for CEC messages.

    @details This event is dispatched to send CEC messages to the CEC HAL.

    @param[in] message message to be sent

    @return \link DisplayError \endlink
  */
  virtual DisplayError CECMessage(char *message) = 0;

  /*! @brief Event handler for Histogram messages received by Display HAL. */
  virtual DisplayError HistogramEvent(int source_fd, uint32_t blob_id) = 0;

  /*! @brief Event handler for events received by Display HAL. */
  virtual DisplayError HandleEvent(DisplayEvent event) = 0;

  /*! @brief Event handler for MMRM. */
  virtual void MMRMEvent(bool restricted) = 0;

  /*! @brief Event handler for sending status of Qsync */
  virtual DisplayError HandleQsyncState(const QsyncEventData &event_data) { return kErrorNone; }

  /*! @brief Event handler to notify CWB Done */
  virtual void NotifyCwbDone(int32_t status, const LayerBuffer& buffer) { }

#ifdef SEC_GC_QC_DYN_CLK
  /*! @brief Event handler to notify DynamicDsiClock */
  virtual void NotifyDynamicDSIClock(uint64_t bitclk) {}
#endif
 protected:
  virtual ~DisplayEventHandler() { }
};

struct PPDisplayAPIPayload;
struct PPPendingParams;

/*! @brief Display device interface.

  @details This class defines display device interface. It contains methods which client shall use
  to configure or submit layers for composition on the display device. This interface is created
  during display device creation and remains valid until destroyed.

  @sa CoreInterface::CreateDisplay
  @sa CoreInterface::DestroyDisplay
*/
class DisplayInterface {
 public:
  /*! @brief Method to determine hardware capability to compose layers associated with given frame.

    @details Client shall send all layers associated with a frame targeted for current display
    using this method and check the layers which can be handled completely in display manager.

    Client shall mark composition type for one of the layer as kCompositionGPUTarget; the GPU
    composed output would be rendered at the specified layer if some of the layers are not handled
    by SDM.

    Display manager will set each layer as kCompositionGPU or kCompositionSDE upon return. Client
    shall render all the layers marked as kCompositionGPU using GPU.

    This method can be called multiple times but only last call prevails. This method must be
    followed by Commit().

    @param[inout] layer_stack \link LayerStack \endlink

    @return \link DisplayError \endlink

    @sa Commit
  */
  virtual DisplayError Prepare(LayerStack *layer_stack) = 0;

  /*! @brief Method to try to perform prepare and commit in one go.

    @details Client shall send all layers associated with a frame targeted for current display
    using this method and check the layers which can be handled completely in display manager.

    Client shall mark composition type for one of the layer as kCompositionGPUTarget; the GPU
    composed output would be rendered at the specified layer if some of the layers are not handled
    by SDM.

    Display manager will set each layer as kCompositionGPU or kCompositionSDE upon return. Client
    shall render all the layers marked as kCompositionGPU using GPU.

    This method must be followed by Commit() if the unified draw cycle could not be performed.

    This method shall be called only once for each frame.

    In the event of an error as well, this call will cause any fences returned in the previous call
    to Commit() to eventually become signaled, so the client's wait on fences can be released to
    prevent deadlocks.

    In case of a virtual display, an explict call is needed to retrieve buffer output fences. Fences
    will be set to -1 in the layer stack when this call is returned.

    @param[in] layer_stack \link LayerStack \endlink

    @return \link DisplayError \endlink

    @sa Commit
  */
  virtual DisplayError CommitOrPrepare(LayerStack *layer_stack) = 0;

  /*! @brief Method to commit layers of a frame submitted in a former call to Prepare().

    @details Client shall call this method to submit layers for final composition. The composed
    output would be displayed on the panel or written in output buffer.

    Client must ensure that layer stack is same as previous call to Prepare.

    This method shall be called only once for each frame.

    In the event of an error as well, this call will cause any fences returned in the previous call
    to Commit() to eventually become signaled, so the client's wait on fences can be released to
    prevent deadlocks.

    @param[in] layer_stack \link LayerStack \endlink

    @return \link DisplayError \endlink

    @sa Prepare
  */
  virtual DisplayError Commit(LayerStack *layer_stack) = 0;

  /*! @brief Method to flush any pending buffers/fences submitted previously via Commit() call.

    @details Client shall call this method to request the Display manager to release all buffers and
    respective fences currently in use. This operation may result in a blank display on the panel
    until a new frame is submitted for composition.

    For virtual displays this would result in output buffer getting cleared with border color.

    @param[in] layer_stack \link LayerStack \endlink

    @return \link DisplayError \endlink

    @sa Prepare
    @sa Commit
  */
  virtual DisplayError Flush(LayerStack *layer_stack) = 0;

  /*! @brief Method to get current state of the display device.

    @param[out] state \link DisplayState \endlink

    @return \link DisplayError \endlink

    @sa SetDisplayState
  */
  virtual DisplayError GetDisplayState(DisplayState *state) = 0;

  /*! @brief Method to get number of configurations(variable properties) supported on the display
    device.

    @param[out] count Number of modes supported; mode index starts with 0.

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetNumVariableInfoConfigs(uint32_t *count) = 0;

  /*! @brief Method to get configuration for fixed properties of the display device.

    @param[out] fixed_info \link DisplayConfigFixedInfo \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetConfig(DisplayConfigFixedInfo *fixed_info) = 0;

  /*! @brief Method to get configuration for variable properties of the display device.

    @param[in] index index of the mode
    @param[out] variable_info \link DisplayConfigVariableInfo \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetConfig(uint32_t index, DisplayConfigVariableInfo *variable_info) = 0;

  /*! @brief Method to get real configuration for variable properties of the display device.

    @param[in] index index of the mode
    @param[out] variable_info \link DisplayConfigVariableInfo \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetRealConfig(uint32_t index, DisplayConfigVariableInfo *variable_info) = 0;

  /*! @brief Method to get index of active configuration of the display device.

    @param[out] index index of the mode corresponding to variable properties.

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetActiveConfig(uint32_t *index) = 0;

  /*! @brief Method to get VSync event state. Default event state is disabled.

    @param[out] enabled vsync state

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetVSyncState(bool *enabled) = 0;

  /*! @brief Method to set draw method for display device. This call is allowed only once.

    @param[in] draw_method \link DisplayDrawMethod \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetDrawMethod(DisplayDrawMethod draw_method) = 0;

  /*! @brief Method to set current state of the display device.

    @param[in] state \link DisplayState \endlink
    @param[in] flag to force full bridge teardown for pluggable displays, no-op for other displays,
               if requested state is kStateOff
    @param[in] pointer to release fence

    @return \link DisplayError \endlink

    @sa SetDisplayState
  */
  virtual DisplayError SetDisplayState(DisplayState state, bool teardown,
                                       shared_ptr<Fence> *release_fence) = 0;

  /*! @brief Method to set active configuration for variable properties of the display device.

    @param[in] variable_info \link DisplayConfigVariableInfo \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetActiveConfig(DisplayConfigVariableInfo *variable_info) = 0;

  /*! @brief Method to set active configuration for variable properties of the display device.

    @param[in] index index of the mode corresponding to variable properties.

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetActiveConfig(uint32_t index) = 0;

  /*! @brief Method to override NoisePlugIn parameters of the display device.

    @param[in] override_en enable flag for toggling override on/off.
    @param[in] attn output attenuation factor.
    @param[in] noise_zpos z-order position of noise layer.

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetNoisePlugInOverride(bool override_en, int32_t attn,
                                              int32_t noise_zpos) = 0;

  /*! @brief Method to set VSync event state. Default event state is disabled.

    @param[out] enabled vsync state

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetVSyncState(bool enable) = 0;

  /*! @brief Method to set idle timeout value. Idle fallback is disabled with timeout value 0.

    @param[in] active_ms value in milliseconds.
    @param[in] in_active_ms value in milliseconds.

    @return \link void \endlink
  */
  virtual void SetIdleTimeoutMs(uint32_t active_ms, uint32_t inactive_ms) = 0;

  /*! @brief Method to set maximum number of mixer stages for each display.

    @param[in] max_mixer_stages maximum number of mixer stages.

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetMaxMixerStages(uint32_t max_mixer_stages) = 0;

  /*! @brief Method to control partial update feature for each display.

    @param[in] enable partial update feature control flag
    @param[out] pending whether the operation is completed or pending for completion

    @return \link DisplayError \endlink
  */
  virtual DisplayError ControlPartialUpdate(bool enable, uint32_t *pending) = 0;

  /*! @brief Method to disable partial update for at least 1 frame.
    @return \link DisplayError \endlink
  */
  virtual DisplayError DisablePartialUpdateOneFrame() = 0;

  /*! @brief Method to get unaligned dimensions of output buffer.

    @param[in] CWB tap-point set by client.
    @param[out] unaligned width and height of output buffer.

    @return \link void \endlink
  */
  virtual DisplayError GetCwbBufferResolution(CwbConfig *cwb_config, uint32_t *x_pixels,
                                              uint32_t *y_pixels) = 0;

  /*! @brief Method to set the mode of the primary display.

    @param[in] mode the new display mode.

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetDisplayMode(uint32_t mode) = 0;

  /*! @brief Method to set the bpp of the panel.

    @param[in] bpp the new bpp mode.

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetBppMode(uint32_t bpp) = 0;

  /*! @brief Method to get the min and max refresh rate of a display.

    @param[out] min and max refresh rate.

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetRefreshRateRange(uint32_t *min_refresh_rate,
                                           uint32_t *max_refresh_rate) = 0;

  /*! @brief Method to set the refresh rate of a display.

    @param[in] refresh_rate new refresh rate of the display.

    @param[in] final_rate indicates whether refresh rate is final rate or can be changed by sdm

    @param[in] idle_screen indicates whether screen is idle.

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetRefreshRate(uint32_t refresh_rate, bool final_rate,
                                      bool idle_screen = false) = 0;

  /*! @brief Method to get the refresh rate of a display.

    @param[in] refresh_rate refresh rate of the display.

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetRefreshRate(uint32_t *refresh_rate) = 0;

  /*! @brief Method to query whether scanning is support for the HDMI display.

    @return \link DisplayError \endlink
  */
  virtual bool IsUnderscanSupported() = 0;

  /*! @brief Method to set brightness of the builtin display.

    @param[in] brightness the new backlight level 0.0f(min) to 1.0f(max) where -1.0f represents off.

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetPanelBrightness(float brightness) = 0;

  /*! @brief Method to notify display about change in min HDCP encryption level.

    @param[in] min_enc_level minimum encryption level value.

    @return \link DisplayError \endlink
  */
  virtual DisplayError OnMinHdcpEncryptionLevelChange(uint32_t min_enc_level) = 0;

  /*! @brief Method to route display API requests to color service.

    @param[in] in_payload \link PPDisplayAPIPayload \endlink
    @param[out] out_payload \link PPDisplayPayload \endlink
    @param[out] pending_action \link PPPendingParams \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError ColorSVCRequestRoute(const PPDisplayAPIPayload &in_payload,
                                            PPDisplayAPIPayload *out_payload,
                                            PPPendingParams *pending_action) = 0;

  /*! @brief Method to request the number of color modes supported.

    @param[out] mode_count Number of modes

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetColorModeCount(uint32_t *mode_count) = 0;

  /*! @brief Method to request the information of supported color modes.

    @param[inout] mode_count Number of updated modes
    @param[out] vector of mode strings

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetColorModes(uint32_t *mode_count,
                                     std::vector<std::string> *color_modes) = 0;

  /*! @brief Method to request the attributes of color mode.

    @param[in] mode name
    @param[out] vector of mode attributes

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetColorModeAttr(const std::string &color_mode,
                                        AttrVal *attr_map) = 0;

  /*! @brief Method to set the color mode

    @param[in] mode_name Mode name which needs to be set

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetColorMode(const std::string &color_mode) = 0;

  /*! @brief Method to set the color mode by ID. This method is used for debugging only.

  @param[in] Mode ID which needs to be set

  @return \link DisplayError \endlink
  */
  virtual DisplayError SetColorModeById(int32_t color_mode_id) = 0;

  /*! @brief Method to get the color mode name.

  @param[in] Mode ID
  @param[out] Mode name

  @return \link DisplayError \endlink
  */
  virtual DisplayError GetColorModeName(int32_t mode_id, std::string *mode_name) = 0;

  /*! @brief Method to set the color transform

    @param[in] length Mode name which needs to be set
    @param[in] color_transform  4x4 Matrix for color transform

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetColorTransform(const uint32_t length, const double *color_transform) = 0;

  /*! @brief Method to get the default color mode.

    @param[out] default mode name

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetDefaultColorMode(std::string *color_mode) = 0;

  /*! @brief Method to request applying default display mode.

    @return \link DisplayError \endlink
  */
  virtual DisplayError ApplyDefaultDisplayMode() = 0;

  /*! @brief Method to set the position of the hw cursor.

    @param[in] x \link x position \endlink
    @param[in] y \link y position \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetCursorPosition(int x, int y) = 0;

  /*! @brief Method to get the brightness level of the display

    @param[out] brightness brightness percentage

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetPanelBrightness(float *brightness) = 0;

  /*! @brief Method to get the brightness level of the display

    @param[out] brightness brightness value

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetPanelBrightnessLevel(int *level) = 0;

  /*! @brief Method to get the max brightness level of the display

    @param[out] max_brightness level

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetPanelMaxBrightness(uint32_t *max_brightness_level) = 0;

  /*! @brief Method to set layer mixer resolution.

    @param[in] width layer mixer width
    @param[in] height layer mixer height

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetMixerResolution(uint32_t width, uint32_t height) = 0;

  /*! @brief Method to get layer mixer resolution.

    @param[out] width layer mixer width
    @param[out] height layer mixer height

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetMixerResolution(uint32_t *width, uint32_t *height) = 0;

  /*! @brief Method to set  frame buffer configuration.

    @param[in] variable_info \link DisplayConfigVariableInfo \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetFrameBufferConfig(const DisplayConfigVariableInfo &variable_info) = 0;

  /*! @brief Method to get frame buffer configuration.

    @param[out] variable_info \link DisplayConfigVariableInfo \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetFrameBufferConfig(DisplayConfigVariableInfo *variable_info) = 0;

  /*! @brief Method to set detail enhancement data.

    @param[in] de_data \link DisplayDetailEnhancerData \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetDetailEnhancerData(const DisplayDetailEnhancerData &de_data) = 0;

  /*! @brief Method to get display port information.

    @param[out] port \link DisplayPort \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetDisplayPort(DisplayPort *port) = 0;

  /*! @brief Method to get display ID information.

    @param[out] display_id Current display's ID as can be discovered using
    CoreInterface::GetDisplaysStatus().

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetDisplayId(int32_t *display_id) = 0;

  /*! @brief Method to get the display's type.

    @param[out] display_type Current display's type.

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetDisplayType(SDMDisplayType *display_type) = 0;

  /*! @brief Method to query whether it is Primrary device.

    @return true if this interface is primary.
  */
  virtual bool IsPrimaryDisplay() = 0;

  /*! @brief Method to toggle composition types handling by SDM.

    @details Client shall call this method to request SDM to enable/disable a specific type of
    layer composition. If client disables a composition type, SDM will not handle any of the layer
    composition using the disabled method in a draw cycle. On lack of resources to handle all
    layers using other enabled composition methods, Prepare() will return an error.

    Request to toggle composition type is applied from subsequent draw cycles.

    Default state of all defined composition types is enabled.

    @param[in] composition_type \link LayerComposition \endlink
    @param[in] enable \link enable composition type \endlink

    @return \link DisplayError \endlink

    @sa Prepare
  */
  virtual DisplayError SetCompositionState(LayerComposition composition_type, bool enable) = 0;

  /*! @brief Method to check whether a client target with the given properties
      can be supported/handled by hardware.

    @param[in] width client target width
    @param[in] height client target height
    @param[in] format client target format
    @param[in] colorMetaData client target colorMetaData

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetClientTargetSupport(uint32_t width, uint32_t height,
                                              LayerBufferFormat format,
                                              const Dataspace &color_metadata) = 0;

  /*! @brief Method to handle secure events.

    @param[in] secure_event \link SecureEvent \endlink

    @param[out] needs_refresh Notifies the caller whether it needs screen refresh after this call

    @return \link DisplayError \endlink
  */
  virtual DisplayError HandleSecureEvent(SecureEvent secure_event, bool *needs_refresh) = 0;

  /*! @brief Method to set dpps ad roi.

    @param[in] roi config parmas

    @return \link DisplayError \endlink
  */

  virtual DisplayError SetDisplayDppsAdROI(void *payload) = 0;

  /*! @brief Method to set the Qsync mode.

  @param[in] qsync_mode: \link QSyncMode \endlink

  @return \link DisplayError \endlink
  */
  virtual DisplayError SetQSyncMode(QSyncMode qsync_mode) = 0;

  /*! @brief Method to control idle power collapse feature for primary display.

    @param[in] enable idle power collapse feature control flag
    @param[in] synchronous commit flag

    @return \link DisplayError \endlink
  */
  virtual DisplayError ControlIdlePowerCollapse(bool enable, bool synchronous) = 0;

  /*! @brief Method to query whether it is supprt sspp tonemap.

    @return true if support sspp tonemap.
  */
  virtual bool IsSupportSsppTonemap() = 0;

  /*! @brief Method to set frame trigger mode for primary display.

    @param[in] frame trigger mode

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetFrameTriggerMode(FrameTriggerMode mode) = 0;

  /*
   * Returns a string consisting of a dump of SDM's display and layer related state
   * as programmed to driver
  */
  virtual std::string Dump() = 0;

  /*! @brief Method to dynamically set DSI clock rate.

    @param[in] bit_clk_rate DSI bit clock rate in HZ.

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetDynamicDSIClock(uint64_t bit_clk_rate) = 0;

  /*! @brief Method to set Jitter configuration.

    @param[in] jitter_type Jitter type; 0 - None, 1 - Instantaneous jitter, 2 - Long term jitter.
    @param[in] value Max jitter value in percentage (0-10%).
    @param[in] time Jitter time (for LTJ).

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetJitterConfig(uint32_t jitter_type, float value, uint32_t time) = 0;

  /*! @brief Method to get the current DSI clock rate

    @param[out] bit_clk_rate DSI bit clock rate in HZ

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetDynamicDSIClock(uint64_t *bit_clk_rate) = 0;

  /*! @brief Method to get the supported DSI clock rates

      @param[out] bitclk DSI bit clock in HZ

      @return \link DisplayError \endlink
  */
  virtual DisplayError GetSupportedDSIClock(std::vector<uint64_t> *bitclk_rates) = 0;

  /*! @brief Method to retrieve the EDID information and HW port ID for display

    @param[out] HW port ID
    @param[out] size of EDID blob data
    @param[out] EDID blob

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetDisplayIdentificationData(uint8_t *out_port, uint32_t *out_data_size,
                                                    uint8_t *out_data) = 0;
  /*! @brief Method to turn on histogram events. */
  virtual DisplayError colorSamplingOn() = 0;

  /*! @brief Method to turn off histogram events. */
  virtual DisplayError colorSamplingOff() = 0;

  /*! @brief Method to set min/max luminance for dynamic tonemapping of external device over WFD.

    @param[in] min_lum min luminance supported by external device.
    @param[in] max_lum max luminance supported by external device.

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetPanelLuminanceAttributes(float min_lum, float max_lum) = 0;

  /*! @brief Method to set display backlight scale ratio.

    @param[in] backlight scale ratio.

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetBLScale(uint32_t level) = 0;

  /*! @brief Method to get panel backlight max level.

    @param[in] panel backlight max level.

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetPanelBlMaxLvl(uint32_t *max_level) = 0;

  /*! @brief Method to enable/disable or config PP event/feature.

    @param[in] payload of PP event/feature
    @param[in] size of the payload.

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetPPConfig(void *payload, size_t size) = 0;

  /*! @brief Method to trigger a screen refresh and mark needs validate.

    @return \link void \endlink
  */
  virtual void ScreenRefresh() = 0;

  /*! @brief Method to check if the Default resources are freed for display

    @return \link bool \endlink
  */
  virtual bool CheckResourceState(bool *res_exhausted) = 0;

  /*! @brief Method to check if game enhance feature is supported for display

    @return \link bool \endlink
  */
  virtual bool GameEnhanceSupported() = 0;

  /*! @brief Method to get the current qsync mode used.

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetQSyncMode(QSyncMode *qsync_mode) = 0;

  /*! @brief Method to set the color mode to STC

    @param[in] color_mode Mode attributes which needs to be set.

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetStcColorMode(const snapdragoncolor::ColorMode &color_mode) = 0;

  /*! @brief Method to query the color mode list from STC.

    @param[out] pointer of mode list

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetStcColorModes(snapdragoncolor::ColorModeList *mode_list) = 0;

  /*! @brief Method to retrieve info on a specific display feature

    @param[out] pointer to the response

    @return \link DisplayError \endlink
  */
  virtual DisplayError IsSupportedOnDisplay(SupportedDisplayFeature feature,
                                            uint32_t *supported) = 0;

  /*! @brief Method to check whether writeback supports requested color format or not.

    @param[in] \link LayerBufferFormat \endlink

    @return \link bool \endlink
  */
  virtual bool IsWriteBackSupportedFormat(const LayerBufferFormat &format) = 0;

  /*! @brief Method to clear scaler LUTs.

    @return \link DisplayError \endlink
  */
  virtual DisplayError ClearLUTs() = 0;

  /*! @brief Method to notify the stc library that connect/disconnect QDCM tool.

    @param[in] connect or disconnect

    @return \link DisplayError \endlink
  */
  virtual DisplayError NotifyDisplayCalibrationMode(bool in_calibration) = 0;

  /*! @brief Method to check if display has Demura requirement

    @return \link bool \endlink
  */
  virtual bool HasDemura() = 0;

  /*! @brief Method to retrieve output buffer acquire fence.

    @param[out] output buffer acquire fence.

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetOutputBufferAcquireFence(shared_ptr<Fence> *out_fence) = 0;

  /*! @brief Method to get validation state.

    @return \link bool \endlink
  */
  virtual bool IsValidated() = 0;

  /*! @brief Method to destroy layer.

    @return \link DisplayError \endlink
  */
  virtual DisplayError DestroyLayer() = 0;

  /*! @brief Method to Get the qsync fps from connector node

    @param[out] value of qsync fps

    @return \link void \endlink
  */
  virtual DisplayError GetQsyncFps(uint32_t *qsync_fps) = 0;
  /*! @brief Method to get the alternate config with same fps and different compression mode.

    @param[out] pointer to config value

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetAlternateDisplayConfig(uint32_t *alt_config) = 0;

  /*! @brief Method to flush CWB

    @return \link void \endlink
  */
  virtual void FlushConcurrentWriteback() = 0;

  /*! @brief Method to force tone mapping LUT update for an existing display layer stack

    @param[inout] layer_stack \link LayerStack \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError ForceToneMapUpdate(LayerStack *layer_stack) = 0;

  /*! @brief Method to enable/disable display dimming feature.

    @param[in] enable or disable

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetDimmingEnable(int int_enabled) = 0;

  /*! @brief Method to set minimal backlight value for display dimming feature.

    @param[in] minimal backlight value

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetDimmingMinBl(int min_bl) = 0;

  /*! @brief Method to retrieve demuratn feature files from TVM.

    @return \link DisplayError \endlink
  */
  virtual DisplayError RetrieveDemuraTnFiles() = 0;

  /*! @brief Method to handle secure events after the successful transition.

    @param[in] secure_event \link SecureEvent \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError PostHandleSecureEvent(SecureEvent secure_event) = 0;
  virtual DisplayError GetConnectorId(int32_t *conn_id) = 0;

  /*! @brief Method to set a new transfer time

    @param[in] transfer_time to set

    @return \link DisplayError \endlink
  */
  virtual DisplayError UpdateTransferTime(uint32_t transfer_time) = 0;

  /*! @brief Method to handle CWB requests on the display

    @param[in] output_buffer \link LayerBuffer \endlink

    @param[in] config \link CwbConfig \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError CaptureCwb(const LayerBuffer &output_buffer, const CwbConfig &config) = 0;

  /*! @brief Method to handle CWB teardown on the display

    @return \link DisplayError \endlink
  */
  virtual bool HandleCwbTeardown() = 0;

  /*! @brief Method to Get the panel feature info

    @param[out] panel feature info

    @return \link void \endlink
  */
  virtual DisplayError GetPanelFeatureInfo(PanelFeatureInfo *info) = 0;

  /*! @brief Method to Abort DP connection

    @return \link void \endlink
  */
  virtual void Abort() = 0;

  /*! @brief Method to Get the free mixer count

    @return \link free mixer count \endlink
  */
  virtual uint32_t GetAvailableMixerCount() = 0;

  /*! @brief Method to enable/disable for demura feature.

   @param[in] enable or disable

   @return \link DisplayError \endlink
  */
  virtual DisplayError SetDemuraState(int state) = 0;

  /*! @brief Method to set config for demura feature.

   @param[in] demura_idx : demura config index

   @return \link DisplayError \endlink
  */
  virtual DisplayError SetDemuraConfig(int demura_idx) = 0;

  /*! @brief Method to handle CAC configuration.

    @param[in] config \link CacConfig \endlink

    @return \link DisplayError \endlink
  */
  virtual DisplayError PerformCacConfig(CacConfig config, bool enable) = 0;

  /*! @brief Method to enable/disable panel OPR info.

   @param[in] client_name : client name
   @param[in] enable: enable or disable
   @param[in] cb_intf: callback interface

   @return \link DisplayError \endlink
  */
  virtual DisplayError
  PanelOprInfo(const std::string &client_name, bool enable,
               SdmDisplayCbInterface<PanelOprPayload> *cb_intf) = 0;

  /*! @brief Method to enable/disable PA histogram collection.

    @param[in] client_name : client name
    @param[in] enable: enable or disable
    @param[in] cb_intf: callback interface

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetPaHistCollection(
      const std::string &client_name, bool enable,
      SdmDisplayCbInterface<PaHistCollectionPayload> *cb_intf) = 0;

  /*! @brief Method to get PA histogram bins.

    @param[out] buf: pointer to save bins

    @return \link DisplayError \endlink
  */
  virtual DisplayError GetPaHistBins(std::array<uint32_t, HIST_BIN_SIZE> *buf) = 0;

  /*! @brief Method to set mode for SSRC feature.

   @param[in] mode : SSRC mode string

   @return \link DisplayError \endlink
  */
  virtual DisplayError SetSsrcMode(const std::string &mode) = 0;

  /*! @brief Method to set Variable Refresh Rate feature state.

    @param[in] Enable or Disable

    @return \link DisplayError \endlink
  */
  virtual DisplayError SetVRRState(bool state) = 0;

  /*! @brief Method to inform driver about frameintervalchange or wakeup from idle.

    @param[in] expected_present_time : ept for incoming frame

    @param[in] frame_interval_ns : frame interval for incoming frame

    @return \link DisplayError \endlink
  */
  virtual DisplayError NotifyExpectedPresent(uint64_t expected_present_time,
                                             uint32_t frame_interval_ns) = 0;

  /*! @brief Method to enable/disable panel backlight info.

   @param[in] client_name : client name
   @param[in] enable: enable or disable
   @param[in] cb_intf: callback interface

   @return \link DisplayError \endlink
  */
  virtual DisplayError PanelBacklightInfo(
      const std::string &client_name, bool enable,
      SdmDisplayCbInterface<PanelBacklightPayload> *cb_intf) = 0;

  /*! @brief Method to enable/disable for ABC feature.

   @param[in] enable or disable

   @return \link DisplayError \endlink
  */
  virtual DisplayError SetABCState(bool state) = 0;

  /*! @brief Method to reconfig ABC feature.

   @return \link DisplayError \endlink
  */
  virtual DisplayError SetABCReconfig() = 0;

  /*! @brief Method to set ABC mode.

   @param[in] mode_name

   @return \link DisplayError \endlink
  */
  virtual DisplayError SetABCMode(const string &mode_name) = 0;

  /*! @brief Method to set panel feature configurations

   @param[in] type : Operation type
   @param[in] data : Configuration or operation data

   @return \link DisplayError \endlink
  */
  virtual DisplayError SetPanelFeatureConfig(int32_t type, void *data) = 0;

  /*! @brief Method to enable/disable COPR feature.

   @param[in] en: enable or disable COPR feature

   @return \link DisplayError \endlink
  */
  virtual DisplayError EnableCopr(bool en) = 0;

  /*! @brief Method to get COPR statistics.

   @param[out] vector of COPR statistics

   @return \link DisplayError \endlink
  */
  virtual DisplayError GetCoprStats(std::vector<int> *stats) = 0;

 protected:
  virtual ~DisplayInterface() { }
};

}  // namespace sdm

#endif  // __DISPLAY_INTERFACE_H__
