/* Copyright (c) 2015-2017, 2020, The Linux Foundation. All rights reserved.
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
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the
 * following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SDM_COLOR_MANAGER_H__
#define __SDM_COLOR_MANAGER_H__

#include <core/sdm_types.h>
#include <core/buffer_allocator.h>
#include <core/socket_handler.h>
#include <private/color_params.h>
#include <stdlib.h>
#include <utils/locker.h>
#include <utils/sys.h>
#include "sdm_display.h"
#include <sdm_display_intf_parcel.h>
#include "sdm_compositor_callbacks.h"

namespace sdm {

// This macro defines name for display APIs interface wrapper library.
// This macro shall be used to load library using dlopen().
#define DISPLAY_API_INTERFACE_LIBRARY_NAME "libsdm-disp-vndapis.so"

// This macro defines variable name of display color APIs function tables
// This macro shall be used to specify name of the variable in dlsym().
#define DISPLAY_API_FUNC_TABLES "display_color_apis_ftables"
#define QDCM_DIAG_CLIENT_LIBRARY_NAME "libsdm-diag.so"
#define INIT_QDCM_DIAG_CLIENT_NAME "QDCMDiagInit"
#define DEINIT_QDCM_DIAG_CLIENT_NAME "QDCMDiagDeInit"

typedef void (*QDCMDiagInit)(void *ftables);

typedef void (*QDCMDiagDeInit)(void);

// Class to encapsulte all details of managing QDCM operating mode.
class SDMQDCMModeManager {
public:
  static const uint32_t kSocketCMDMaxLength = 4096;
  static const uint32_t kFullWakeLock = 0x0000001a;
  static const uint32_t kAcquireCauseWakeup = 0x10000000;
  static const uint32_t kONAfterRelease = 0x20000000;
  enum ActiveFeatureID {
    kCABLFeature,
    kADFeature,
    kSVIFeature,
    kMaxNumActiveFeature,
  };

  struct ActiveFeatureCMD {
    const char *cmd_on = NULL;
    const char *cmd_off = NULL;
    const char *cmd_query_status = NULL;
    const char *running = NULL;
    ActiveFeatureCMD(const char *arg1, const char *arg2, const char *arg3,
                     const char *arg4)
        : cmd_on(arg1), cmd_off(arg2), cmd_query_status(arg3), running(arg4) {}
  };

  static const ActiveFeatureCMD kActiveFeatureCMD[kMaxNumActiveFeature];

public:
 static SDMQDCMModeManager *CreateQDCMModeMgr(SocketHandler *socket_handler);
 ~SDMQDCMModeManager();
 DisplayError EnableQDCMMode(bool enable, SDMDisplay *sdm_display);

protected:
  bool SendSocketCmd();
  DisplayError AcquireAndroidWakeLock(bool enable);
  DisplayError EnableActiveFeatures(bool enable);
  DisplayError EnableActiveFeatures(bool enable, const ActiveFeatureCMD &cmds,
                                    bool *was_running);

private:
  bool cabl_was_running_ = false;
  int socket_fd_ = -1;
  uint32_t entry_timeout_ = 0;
  static const char *const kSocketName;
  static const char *const kTagName;
  static const char *const kPackageName;
};

// Class to encapsulte all SDM/OS specific behaviours for ColorManager.
class SDMColorManager {
public:
  static const int kNumSolidFillLayers = 2;
  static SDMColorManager *CreateColorManager(BufferAllocator *buffer_allocator,
                                             SocketHandler *socket_handler);
  static DisplayError CreatePayloadFromParcel(SDMParcel *in, uint32_t *disp_id,
                                              PPDisplayAPIPayload *sink);
  static void MarshallStructIntoParcel(const PPDisplayAPIPayload &data,
                                       SDMParcel *out_parcel);

  explicit SDMColorManager(BufferAllocator *buffer_allocator, SocketHandler *socket_handler);
  ~SDMColorManager();
  void DestroyColorManager();
  DisplayError EnableQDCMMode(bool enable, SDMDisplay *sdm_display);
  DisplayError SetSolidFill(const void *params, bool enable,
                            SDMDisplay *sdm_display);
  DisplayError SetFrameCapture(void *params, bool enable,
                               SDMDisplay *sdm_display);
  DisplayError SetDetailedEnhancer(void *params, SDMDisplay *sdm_display);
  DisplayError SetHWDetailedEnhancerConfig(void *params,
                                           SDMDisplay *sdm_display);

protected:
  DisplayError CreateSolidFillLayers(SDMDisplay *sdm_display);
  void DestroySolidFillLayers();
  static uint32_t Get8BitsARGBColorValue(const PPColorFillParams &params);

private:
  DynLib color_apis_lib_;
  DynLib diag_client_lib_;
  void *color_apis_ = NULL;
  QDCMDiagInit qdcm_diag_init_ = NULL;
  QDCMDiagDeInit qdcm_diag_deinit_ = NULL;
  SDMQDCMModeManager *qdcm_mode_mgr_ = NULL;

  PPColorFillParams solid_fill_params_;
  BufferAllocator *buffer_allocator_ = NULL;
  SocketHandler *socket_handler_ = nullptr;
  BufferInfo buffer_info;
  Locker locker_;
};

} // namespace sdm

#endif // __SDM_COLOR_MANAGER_H__
