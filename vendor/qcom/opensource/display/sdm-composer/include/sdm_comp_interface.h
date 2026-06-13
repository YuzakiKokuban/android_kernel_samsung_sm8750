/*
* Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#ifndef __SDM_COMP_INTERFACE_H__
#define __SDM_COMP_INTERFACE_H__

#include "buffer_interface.h"

namespace sdm {

typedef void * Handle;

enum SDMCompDisplayType {
  kSDMCompDisplayTypePrimary,       // Defines the display type for primary display
  kSDMCompDisplayTypeSecondary1,    // Defines the display type for secondary builtin display
  kSDMCompDisplayTypeMax,
};

// The following values matches the HEVC spec
typedef enum ColorPrimaries {
  // Unused = 0;
  ColorPrimaries_BT709_5     = 1,  // ITU-R BT.709-5 or equivalent
  /* Unspecified = 2, Reserved = 3*/
  ColorPrimaries_BT470_6M    = 4,  // ITU-R BT.470-6 System M or equivalent
  ColorPrimaries_BT601_6_625 = 5,  // ITU-R BT.601-6 625 or equivalent
  ColorPrimaries_BT601_6_525 = 6,  // ITU-R BT.601-6 525 or equivalent
  ColorPrimaries_SMPTE_240M  = 7,  // SMPTE_240M
  ColorPrimaries_GenericFilm = 8,  // Generic Film
  ColorPrimaries_BT2020      = 9,  // ITU-R BT.2020 or equivalent
  ColorPrimaries_SMPTE_ST428 = 10,  // SMPTE_240M
  ColorPrimaries_AdobeRGB    = 11,
  ColorPrimaries_DCIP3       = 12,
  ColorPrimaries_EBU3213     = 22,
  ColorPrimaries_Max         = 0xff,
} ColorPrimaries;

typedef enum GammaTransfer {
  // Unused = 0;
  Transfer_sRGB            = 1,  // ITR-BT.709-5
  /* Unspecified = 2, Reserved = 3 */
  Transfer_Gamma2_2        = 4,
  Transfer_Gamma2_8        = 5,
  Transfer_SMPTE_170M      = 6,  // BT.601-6 525 or 625
  Transfer_SMPTE_240M      = 7,  // SMPTE_240M
  Transfer_Linear          = 8,
  Transfer_Log             = 9,
  Transfer_Log_Sqrt        = 10,
  Transfer_XvYCC           = 11,  // IEC 61966-2-4
  Transfer_BT1361          = 12,  // Rec.ITU-R BT.1361 extended gamut
  Transfer_sYCC            = 13,  // IEC 61966-2-1 sRGB or sYCC
  Transfer_BT2020_2_1      = 14,  // Rec. ITU-R BT.2020-2 (same as the values 1, 6, and 15)
  Transfer_BT2020_2_2      = 15,  // Rec. ITU-R BT.2020-2 (same as the values 1, 6, and 14)
  Transfer_SMPTE_ST2084    = 16,  // 2084
  Transfer_ST_428          = 17,  // SMPTE ST 428-1
  Transfer_HLG             = 18,  // ARIB STD-B67
  Transfer_Max             = 0xff,
} GammaTransfer;

enum RenderIntent {
  //<! Colors with vendor defined gamut
  kRenderIntentNative,
  //<! Colors with in gamut are left untouched, out side the gamut are hard clipped
  kRenderIntentColorimetric,
  //<! Colors with in gamut are ehanced, out side the gamuat are hard clipped
  kRenderIntentEnhance,
  //<! Tone map hdr colors to display's dynamic range, mapping to display gamut is
  //<! defined in colormertic.
  kRenderIntentToneMapColorimetric,
  //<! Tone map hdr colors to display's dynamic range, mapping to display gamut is
  //<! defined in enhance.
  kRenderIntentToneMapEnhance,
  //<! Custom render intents range
  kRenderIntentOemCustomStart = 0x100,
  kRenderIntentOemCustomEnd = 0x1ff,
  //<! If STC implementation returns kOemModulateHw render intent, STC manager will
  //<! call the implementation for all the render intent/blend space combination.
  //<! STC implementation can modify/modulate the HW assets.
  kRenderIntentOemModulateHw = 0xffff - 1,
  kRenderIntentMaxRenderIntent = 0xffff
};

struct SDMCompDisplayAttributes {
  uint32_t vsync_period = 0;  //!< VSync period in nanoseconds.
  uint32_t x_res = 0;         //!< Total number of pixels in X-direction on the display panel.
  uint32_t y_res = 0;         //!< Total number of pixels in Y-direction on the display panel.
  float x_dpi = 0.0f;         //!< Dots per inch in X-direction.
  float y_dpi = 0.0f;         //!< Dots per inch in Y-direction.
  bool is_yuv = false;        //!< If the display output is in YUV format.
  uint32_t fps = 0;           //!< fps of the display.
  bool smart_panel = false;   //!< Speficies the panel is video mode or command mode
};

struct SDMCompMixerConfig {
  uint32_t width = 0;
  uint32_t height = 0;
};

struct ColorMode {
  //<! Blend-Space gamut
  ColorPrimaries gamut = ColorPrimaries_Max;
  //<! Blend-space Gamma
  GammaTransfer gamma = Transfer_Max;
  //<! Intent of the mode
  RenderIntent intent = kRenderIntentMaxRenderIntent ;
};

class CallbackInterface {
 public:
  /*! @brief Callback method to handle any asyn error.

    @details This function need to be implemented by the client which will be called
             by the sdm composer on any hardware hang etc.
  */
  virtual void OnError() = 0;

 protected:
  virtual ~CallbackInterface() { }

  // callbackdata where client can store the context information about display type
  // for which this callback corresponds to.
  Handle callback_data_ = nullptr;
};

class SDMCompInterface {
 public:
  /*! @brief Method to create display composer interface for TUI service.

    @details This function to be called once per the device life cycle. This function creates
             sdm core and opens up the display driver drm interface.

    @param[out] intf - Populates composer interface pointer.

    @return Returns 0 on sucess otherwise errno
  */
  static int Create(SDMCompInterface **intf);


  /*! @brief Method to destroy display composer interface for TUI service.

    @details This function to be called once per the device life cycle. This function destroys
             sdm core and closes the display driver drm interface.

    @param[in] intf - Composer interface pointer which was populated by Create() function.

    @return Returns 0 on sucess otherwise errno
  */
  static int Destroy(SDMCompInterface *intf);


  /*! @brief Method to create display for composer where the TUI to be rendered.

    @details This function to be called on start of TUI session. This function creates primary or
             secondary display based on the display type passed by the client and intialize the
             display.to its appropriate state This function to be called once per display. This is
             also responsible to create single layer to the created display

    @param[in]  display_type - Specifies the type of a display. \link SDMCompDisplayType \endlink
    @param[in]  callback - Pointer to callback interface which handles the async error.
                \link CallbackInterface \endlink
    @param[out] disp_hnd     - pointer to display handle which stores the context of the client.

    @return Returns 0 on sucess otherwise errno
  */
  virtual int CreateDisplay(SDMCompDisplayType display_type, CallbackInterface *callback,
                            Handle *disp_hnd) = 0;


  /*! @brief Method to destroy display for composer where the TUI to be rendered.

    @details This function to be called on end of TUI session. This function destroys primary or
             secondary display based on the display type passed by the client and relinquish all the
             MDP hw resources. This function to be called once per display.

    @param[in] disp_hnd - pointer to display handle which was created during CreateDisplay()

    @return Returns 0 on sucess otherwise errno
  */
  virtual int DestroyDisplay(Handle disp_hnd) = 0;


  /*! @brief Method to get display attributes for a given display handle.

    @param[in] disp_hnd - pointer to display handle which was created during CreateDisplay()
    @param[out] display_attributes - pointer to display attributes to b populated for a given
                                     display handle. \link SDMCompDisplayAttributes \endlink.

    @return Returns 0 on sucess otherwise errno
  */
  virtual int GetDisplayAttributes(Handle disp_hnd,
                                   SDMCompDisplayAttributes *display_attributes) = 0;


  /*! @brief Method to prepare and render buffer to display

    @param[in] disp_hnd - pointer to display handle which was created during CreateDisplay()
    @param[in] buf_handle - pointer to buffer handle which specifies the attributes of a buffer
    @param[out] retire_fence - pointer to retire fence which will be signaled once display hw
                               picks up the buf_handle for rendering.

    @return Returns 0 on sucess otherwise errno
  */
  virtual int ShowBuffer(Handle disp_hnd, BufferHandle *buf_handle, int32_t *retire_fence) = 0;


  /*! @brief Method to set color mode with render intent

    @param[in] disp_hnd - pointer to display handle which was created during CreateDisplay()
    @param[in] mode - ColorMode struct which contains blend space and render intent info

    @return Returns 0 on sucess otherwise errno
  */

  virtual int SetColorModeWithRenderIntent(Handle disp_hnd, struct ColorMode mode) = 0;


  /*! @brief Method to get all the supported color modesDprepare and render buffer to display
   * ut

    @param[in] disp_hnd - pointer to display handle which was created during CreateDisplay()
    @param[out] out_num_modes - pointer to uint32_t which specifies the number of color modes
    @param[out] out_modes - pointer to ColorMode struct which contains all color modes that
                            are parsed from xml files

    @return Returns 0 on sucess otherwise errno
  */
  virtual int GetColorModes(Handle disp_hnd, uint32_t *out_num_modes,
                            struct ColorMode *out_modes) = 0;

  /*! @brief Method to set panel brightness

    @detail This api enables client to set the panel brightness of a given display, If the set
            brightness level is below the minimum panel brightness values set by the API
            SetMinPanelBrightness(), then it ignores the request.

    @param[in] disp_hnd - pointer to display handle which was created during CreateDisplay()
    @param[in] brightness_level - brightness_level of the panel varies from 0.0f to 1.0f

    @return Returns 0 on sucess otherwise errno
  */
  virtual int SetPanelBrightness(Handle disp_hnd, float brightness_level) = 0;

  /*! @brief Method to set minimum panel brightness level

    @detail This api enables client to set minimum panel brightness threshold of a given display,
            If the set brightness level is below the minimum panel brightness threshold, Reset the
            panel brightness level to minimum panel brightness value

    @param[in] disp_hnd - pointer to display handle which was created during CreateDisplay()
    @param[in] min_brightness_level - min_brightness_level of the panel varies from 0.0f to 1.0f

    @return Returns 0 on sucess otherwise errno
  */
  virtual int SetMinPanelBrightness(Handle disp_hnd, float min_brightness_level) = 0;

 protected:
  virtual ~SDMCompInterface() { }
};

}  // namespace sdm

#endif  // __SDM_COMP_INTERFACE_H__


