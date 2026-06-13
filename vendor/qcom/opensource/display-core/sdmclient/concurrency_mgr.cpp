/*
 * Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <algorithm>
#include <bitset>
#include <core/buffer_allocator.h>
#include <iterator>
#include <memory>
#include <private/color_params.h>
#include <string>
#include <thread>
#include <utility>
#include <utils/constants.h>
#include <utils/debug.h>
#include <utils/utils.h>
#include <vector>

#include "concurrency_mgr.h"
#include "ipc_impl.h"
#include "sdm_debugger.h"

#define __CLASS__ "ConcurrencyMgr"

#define SDM_UEVENT_SWITCH_HDMI "change@/devices/virtual/switch/hdmi"

#ifdef PROFILE_COVERAGE_DATA
extern "C" {

int __llvm_profile_runtime = 0;

void __llvm_profile_try_write_file(void);
}
#endif

namespace sdm {
Locker ConcurrencyMgr::locker_[locker_count_];
bool ConcurrencyMgr::pending_power_mode_[kNumDisplays];
std::bitset<kClientMax>
    ConcurrencyMgr::clients_waiting_for_commit_[kNumDisplays];
shared_ptr<Fence> ConcurrencyMgr::retire_fence_[kNumDisplays];
int ConcurrencyMgr::commit_error_[kNumDisplays] = {0};
Locker ConcurrencyMgr::display_config_locker_;

void GetColorMetadataFromColorMode(SDMColorMode mode, Dataspace &ds) {
  switch (mode) {
    case SDMColorMode::COLOR_MODE_SRGB:
    // dataspace is ignored in native mode
    case SDMColorMode::COLOR_MODE_NATIVE:
      ds.colorPrimaries = QtiColorPrimaries_BT709_5;
      ds.transfer = QtiTransfer_sRGB;
      ds.range = QtiRange_Full;
      break;
    case SDMColorMode::COLOR_MODE_DCI_P3:
      ds.colorPrimaries = QtiColorPrimaries_DCIP3;
      // gamma 2.6 transfer - not supported by HW
      ds.transfer = static_cast<vendor_qti_hardware_display_common_QtiGammaTransfer>(5 << 22);
      ds.range = QtiRange_Full;
      break;
    case SDMColorMode::COLOR_MODE_DISPLAY_P3:
      ds.colorPrimaries = QtiColorPrimaries_DCIP3;
      ds.transfer = QtiTransfer_sRGB;
      ds.range = QtiRange_Full;
      break;
    case SDMColorMode::COLOR_MODE_BT2100_PQ:
      ds.colorPrimaries = QtiColorPrimaries_BT2020;
      ds.transfer = QtiTransfer_SMPTE_ST2084;
      ds.range = QtiRange_Full;
      break;
    case SDMColorMode::COLOR_MODE_BT2100_HLG:
      ds.colorPrimaries = QtiColorPrimaries_BT2020;
      ds.transfer = QtiTransfer_HLG;
      ds.range = QtiRange_Full;
      break;
    case SDMColorMode::COLOR_MODE_DISPLAY_BT2020:
      ds.colorPrimaries = QtiColorPrimaries_BT2020;
      ds.transfer = QtiTransfer_sRGB;
      ds.range = QtiRange_Full;
      break;
    default:
      ds.colorPrimaries = QtiColorPrimaries_Max;
      ds.transfer = QtiTransfer_Max;
      ds.range = QtiRange_Max;
  }
}

ConcurrencyMgr::ConcurrencyMgr() {}

ConcurrencyMgr::~ConcurrencyMgr() {
  Deinit();
}

int ConcurrencyMgr::GetDisplayIndex(int dpy) {
  return disp_->GetDisplayIndex(dpy);
}

DisplayError ConcurrencyMgr::CreatePrimaryDisplay() {
  SCOPE_LOCK(primary_display_lock_);

  while (primary_pending_) {
    int status = disp_->CreatePrimaryDisplay();
    if (!status) {
      primary_pending_ = false;
      primary_display_lock_.Signal();
      break;
    }
  }

  return kErrorNone;
}

bool ConcurrencyMgr::IsDisplayConnected(uint64_t display_id) {
  int disp_idx = disp_->GetDisplayIndex(display_id);

  if (disp_idx == -1) {
    return false;
  }

  SEQUENCE_WAIT_SCOPE_LOCK(locker_[disp_idx]);
  return !!sdm_display_[disp_idx];
}

DisplayError ConcurrencyMgr::GetDisplaysStatus(HWDisplaysInfo *info) {
  return core_intf_->GetDisplaysStatus(info);
}

DisplayError ConcurrencyMgr::GetMaxDisplaysSupported(SDMDisplayType in_type,
                                                     int32_t *max_disp) {
  return core_intf_->GetMaxDisplaysSupported(in_type, max_disp);
}

void ConcurrencyMgr::SetHpdData(int hpd_bpp, int hpd_pattern,
                                int hpd_connected) {
  hpd_bpp_ = hpd_bpp;
  hpd_pattern_ = hpd_pattern;
  hpd_connected_ = hpd_connected;
}

void ConcurrencyMgr::GetHpdData(int *hpd_bpp, int *hpd_pattern,
                                int *hpd_connected) {
  *hpd_bpp = hpd_bpp_;
  *hpd_pattern = hpd_pattern_;
  *hpd_connected = hpd_connected_;
}

DisplayError ConcurrencyMgr::Init(BufferAllocator *buffer_allocator, SocketHandler *socket_handler,
                                  DebugCallbackIntf *debug) {
  SCOPE_LOCK(locker_[SDM_DISPLAY_PRIMARY]);

  if (is_composer_up_) {
    DLOGI("Composer already initialized");
    return kErrorNone;
  }

  DLOGI("Initializing ConcurrencyMgr");

  buffer_allocator_ = buffer_allocator;
  socket_handler_ = socket_handler;

  DisplayError status = kErrorNotSupported;

  SDMDebugHandler::SetDebugCallback(debug);
  int value = 0; // Default value when property is not present.
  SDMDebugHandler::Get()->GetProperty(ENABLE_VERBOSE_LOG, &value);
  if (true) {
    SDMDebugHandler::DebugAll(value, value);
  }

  value = 0;
  Debug::Get()->GetProperty(ENABLE_ASYNC_VDS_CREATION, &value);
  async_vds_creation_ = (value == 1);
  DLOGI("async_vds_creation: %d", async_vds_creation_);

  value = 0;
  Debug::Get()->GetProperty(DISABLE_GET_SCREEN_DECORATOR_SUPPORT, &value);
  disable_get_screen_decorator_support_ = (value == 1);
  DLOGI("disable_get_screen_decorator_support: %d",
        disable_get_screen_decorator_support_);

  auto err = InitSubModules(debug);
  if (err != kErrorNone) {
    return err;
  }

  disp_->SetProperties(enable_primary_reconfig_req_);

  // Create primary display here. Remaining builtin displays will be created
  // after client has set display indexes which may happen sometime before
  // callback is registered.
  DLOGI("Creating the Primary display");
  status = CreatePrimaryDisplay();
  if (status) {
    DLOGE("Creating the Primary display...failed!");

    DisplayError error = CoreInterface::DestroyCore();
    if (error != kErrorNone) {
      DLOGE("Display core de-initialization failed. Error = %d", error);
    }

    return status;
  } else {
    DLOGI("Creating the Primary display...done!");
  }

  is_composer_up_ = true;

  PostInit();

  DLOGI("Initializing ConcurrencyMgr...done!");
  return kErrorNone;
}

bool ConcurrencyMgr::GetComposerStatus() { return is_composer_up_; }

void ConcurrencyMgr::PostInit() {
  // Start services which need IDisplayConfig to be up.
  // This avoids deadlock between composer and its clients.
  auto sdm_display = sdm_display_[SDM_DISPLAY_PRIMARY];
  sdm_display->PostInit();
}

DisplayError ConcurrencyMgr::Deinit() {
  if (hpd_) {
    hpd_->Deinit();
    delete hpd_;
  }

  if (disp_) {
    disp_->Deinit();
    delete disp_;

    SCOPE_LOCK(primary_display_lock_);
    primary_pending_ = true;
  }

  if (services_) {
    services_->Deinit();
    delete services_;
  }

  DisplayError error = CoreInterface::DestroyCore();
  if (error != kErrorNone) {
    DLOGE("Display core de-initialization failed. Error = %d", error);
  }

  SCOPE_LOCK(primary_display_lock_);
  primary_pending_ = true;

  is_composer_up_ = false;

  return kErrorNone;
}

DisplayError ConcurrencyMgr::InitSubModules(DebugCallbackIntf *debug) {
  ipc_intf_ = std::make_shared<IPCImpl>(IPCImpl(&callbacks_));
  ipc_intf_->Init();

  int core_id_mask = 0Xff;      // default value when property is not present
  Debug::Get()->GetProperty(CORE_ID_MASK, &core_id_mask);
  DLOGI("core_id_mask: %d", core_id_mask);
  std::bitset<8> core_ids(core_id_mask);

  DisplayError error = CoreInterface::CreateCore(
      buffer_allocator_, nullptr, socket_handler_, ipc_intf_, &core_intf_);

  if (error != kErrorNone) {
    DLOGE("Failed to create CoreInterface");
    return error;
  }

  // Initialize pointer to snapalloc
  const std::string snapalloc_lib_name =
      "vendor.qti.hardware.display.snapalloc-impl.so";
  void *snap_impl_lib_ = ::dlopen(snapalloc_lib_name.c_str(), RTLD_NOW);
  if (!snap_impl_lib_) {
    DLOGE("Dlopen error for snapalloc impl: %s", dlerror());
    return kErrorResources;
  }

  std::shared_ptr<ISnapMapper> (*LINK_FETCH_ISnapMapper)(DebugCallbackIntf *) = nullptr;
  *reinterpret_cast<void **>(&LINK_FETCH_ISnapMapper) =
      ::dlsym(snap_impl_lib_, "FETCH_ISnapMapper");
  if (LINK_FETCH_ISnapMapper) {
    snapmapper_ = LINK_FETCH_ISnapMapper(debug);
  } else {
    DLOGE("Failed to get snapalloc instance");
    return kErrorResources;
  }

  hpd_ = new SDMHotPlug(this, &callbacks_);
  hpd_->Init();

  cwb_ = new SDMConcurrentWriteBack(this, snapmapper_);
  cwb_->Init();

  disp_ = new SDMDisplayBuilder(this, buffer_allocator_, core_intf_, &callbacks_, this);
  disp_->Init(locker_);

  tui_ = new SDMTrustedUI(this);
  tui_->Init(disp_, locker_, pluggable_lock_index_);

  services_ = new SDMServices(this, buffer_allocator_, socket_handler_);
  services_->Init(disp_, buffer_allocator_, locker_, tui_);

  return kErrorNone;
}

bool ConcurrencyMgr::IsHDRDisplay(uint64_t display) {
  return disp_->IsHDRDisplay(display);
}

DisplayError ConcurrencyMgr::PostBuffer(const CwbConfig &cwb_config,
                                        void *buffer, int32_t display_type) {
  return cwb_->PostBuffer(cwb_config, buffer,
                          disp_->GetDisplayIndex(display_type));
}

void ConcurrencyMgr::NotifyCWBStatus(int32_t status, void *buffer) {
  callbacks_.NotifyCWBStatus(status, buffer);
}

void ConcurrencyMgr::GetCapabilities(uint32_t *outCount,
                                     int32_t *outCapabilities) {
  if (!outCount) {
    return;
  }

  int value = 0;
  bool disable_skip_validate = false;
  if (Debug::Get()->GetProperty(DISABLE_SKIP_VALIDATE_PROP, &value) ==
      kErrorNone) {
    disable_skip_validate = (value == 1);
  }
  uint32_t count = disable_skip_validate ? 0 : 1;

  if (outCapabilities != nullptr && (*outCount >= count)) {
    if (!disable_skip_validate) {
      outCapabilities[0] = INT32(SDMCapability::kSkipValidate);
    }
  }
  *outCount = count;
}

void ConcurrencyMgr::Dump(uint32_t *out_size, char *out_buffer) {
  if (!out_size) {
    return;
  }

  const size_t max_dump_size = 16384; // 16 kB

  if (out_buffer == nullptr) {
    *out_size = max_dump_size;
  } else {
    std::ostringstream os;
    for (int id = 0; id < kNumRealDisplays; id++) {
      SCOPE_LOCK(locker_[id]);
      if (sdm_display_[id]) {
        sdm_display_[id]->Dump(&os);
      }
    }
    Fence::Dump(&os);

    std::string s = os.str();
    auto copied = s.copy(out_buffer, std::min(s.size(), max_dump_size), 0);
    *out_size = UINT32(copied);
  }
}

uint32_t ConcurrencyMgr::GetMaxVirtualDisplayCount() {
  // Limit max virtual display reported to SF as one. Even though
  // HW may support multiple virtual displays, allow only one
  // to be used by SF for now.
  return 1;
}

DisplayError ConcurrencyMgr::AcceptDisplayChanges(Display display) {
  return CallDisplayFunction(display, &SDMDisplay::AcceptDisplayChanges);
}

DisplayError ConcurrencyMgr::GetActiveConfig(uint64_t display,
                                             Config *out_config) {
#ifdef SEC_GC_QC_OPTIMIZATION
  return CallGetDisplayFunction(display, &SDMDisplay::GetActiveConfig, false, out_config);
#else
  return CallDisplayFunction(display, &SDMDisplay::GetActiveConfig, false, out_config);
#endif
}

DisplayError ConcurrencyMgr::GetChangedCompositionTypes(
    Display display, uint32_t *out_num_elements, LayerId *out_layers,
    int32_t *out_types) {
  // null_ptr check only for out_num_elements, as out_layers and out_types can
  // be null.
  if (!out_num_elements) {
    return kErrorParameters;
  }
  return CallDisplayFunction(display, &SDMDisplay::GetChangedCompositionTypes,
                             out_num_elements, out_layers, out_types);
}

DisplayError ConcurrencyMgr::GetClientTargetSupport(uint64_t display,
                                                    int32_t width,
                                                    int32_t height,
                                                    LayerBufferFormat format,
                                                    Dataspace dataspace) {
  return CallDisplayFunction(display, &SDMDisplay::GetClientTargetSupport,
                             width, height, format, dataspace);
}

DisplayError
ConcurrencyMgr::GetColorModes(uint64_t display, uint32_t *out_num_modes,
                              int32_t /*SDMColorMode*/ *int_out_modes) {
  auto out_modes = reinterpret_cast<SDMColorMode *>(int_out_modes);
  if (out_num_modes == nullptr) {
    return kErrorParameters;
  }
  return CallDisplayFunction(display, &SDMDisplay::GetColorModes, out_num_modes,
                             out_modes);
}

DisplayError ConcurrencyMgr::GetRenderIntents(
    Display display, int32_t /*SDMColorMode*/ int_mode,
    uint32_t *out_num_intents, int32_t /*RenderIntent*/ *int_out_intents) {
  auto mode = static_cast<SDMColorMode>(int_mode);
  auto out_intents = reinterpret_cast<SDMRenderIntent *>(int_out_intents);
  if (out_num_intents == nullptr) {
    return kErrorParameters;
  }

  if (mode < SDMColorMode::COLOR_MODE_NATIVE ||
      mode > SDMColorMode::COLOR_MODE_DISPLAY_BT2020) {
    DLOGE("Invalid SDMColorMode: %d", mode);
    return kErrorParameters;
  }
  return CallDisplayFunction(display, &SDMDisplay::GetRenderIntents, mode,
                             out_num_intents, out_intents);
}

DisplayError ConcurrencyMgr::GetDataspaceSaturationMatrix(int32_t int_dataspace,
                                                          float *out_matrix) {
  if (out_matrix == nullptr) {
    return kErrorParameters;
  }
  // We only have the matrix for sRGB
  float saturation_matrix[kDataspaceSaturationMatrixCount] = {
      1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

  for (int32_t i = 0; i < kDataspaceSaturationMatrixCount; i += 4) {
    DLOGD("%f %f %f %f", saturation_matrix[i], saturation_matrix[i + 1],
          saturation_matrix[i + 2], saturation_matrix[i + 3]);
  }
  for (uint32_t i = 0; i < kDataspaceSaturationMatrixCount; i++) {
    out_matrix[i] = saturation_matrix[i];
  }

  return kErrorNone;
}

DisplayError ConcurrencyMgr::SetDisplayedContentSamplingEnabled(
    Display display, bool enabled, uint8_t component_mask,
    uint64_t max_frames) {
  static constexpr int32_t validComponentMask =
      INT32(SDMFormatColorComponent::FORMAT_COMPONENT_0) |
      INT32(SDMFormatColorComponent::FORMAT_COMPONENT_1) |
      INT32(SDMFormatColorComponent::FORMAT_COMPONENT_2) |
      INT32(SDMFormatColorComponent::FORMAT_COMPONENT_3);
  if (component_mask & ~validComponentMask)
    return kErrorParameters;
  return CallDisplayFunction(display,
                             &SDMDisplay::SetDisplayedContentSamplingEnabled,
                             enabled, component_mask, max_frames);
}

DisplayError ConcurrencyMgr::GetDisplayedContentSamplingAttributes(
    Display display, int32_t *format, int32_t *dataspace,
    uint8_t *supported_components) {
  return CallDisplayFunction(display,
                             &SDMDisplay::GetDisplayedContentSamplingAttributes,
                             format, dataspace, supported_components);
}

DisplayError ConcurrencyMgr::GetDisplayedContentSample(
    Display display, uint64_t max_frames, uint64_t timestamp,
    uint64_t *numFrames, int32_t samples_size[NUM_HISTOGRAM_COLOR_COMPONENTS],
    uint64_t *samples[NUM_HISTOGRAM_COLOR_COMPONENTS]) {
  return CallDisplayFunction(display, &SDMDisplay::GetDisplayedContentSample,
                             max_frames, timestamp, numFrames, samples_size,
                             samples);
}

DisplayError ConcurrencyMgr::GetAllDisplayAttributes(
    uint64_t display, std::map<uint32_t, DisplayConfigVariableInfo> *info) {
  return CallDisplayFunction(display, &SDMDisplay::GetAllDisplayAttributes,
                             info);
}

DisplayError ConcurrencyMgr::GetDisplayAttributes(uint64_t display, int32_t index,
                                                  DisplayConfigVariableInfo *attributes) {
  if (!attributes) {
    return kErrorParameters;
  }
  return CallDisplayFunction(display, &SDMDisplay::GetDisplayAttributes, index, attributes);
}

DisplayError
ConcurrencyMgr::GetDisplayConfigs(Display display,
                                  std::vector<int32_t> *out_configs) {
  return CallDisplayFunction(display, &SDMDisplay::GetDisplayConfigs,
                             out_configs);
}

DisplayError ConcurrencyMgr::GetDisplayName(Display display, uint32_t *out_size,
                                            char *out_name) {
  return CallDisplayFunction(display, &SDMDisplay::GetDisplayName, out_size,
                             out_name);
}

DisplayError ConcurrencyMgr::GetDisplayRequests(Display display,
                                                int32_t *out_display_requests,
                                                uint32_t *out_num_elements,
                                                LayerId *out_layers,
                                                int32_t *out_layer_requests) {
  return CallDisplayFunction(display, &SDMDisplay::GetDisplayRequests,
                             out_display_requests, out_num_elements, out_layers,
                             out_layer_requests);
}

DisplayError ConcurrencyMgr::GetDisplayType(uint64_t display,
                                            int32_t *out_type) {
  return CallDisplayFunction(display, &SDMDisplay::GetDisplayType, out_type);
}

DisplayError
ConcurrencyMgr::GetHdrCapabilities(Display display, uint32_t *out_num_types,
                                   int32_t *out_types, float *out_max_luminance,
                                   float *out_max_average_luminance,
                                   float *out_min_luminance) {
  return CallDisplayFunction(display, &SDMDisplay::GetHdrCapabilities,
                             out_num_types, out_types, out_max_luminance,
                             out_max_average_luminance, out_min_luminance);
}

DisplayError
ConcurrencyMgr::GetReleaseFences(Display display, uint32_t *out_num_elements,
                                 LayerId *out_layers,
                                 std::vector<shared_ptr<Fence>> *out_fences) {
  return CallDisplayFunction(display, &SDMDisplay::GetReleaseFences,
                             out_num_elements, out_layers, out_fences);
}

DisplayError ConcurrencyMgr::getDisplayDecorationSupport(Display display,
                                                         uint32_t *format,
                                                         uint32_t *alpha) {
  if (disable_get_screen_decorator_support_) {
    return kErrorNotSupported;
  }

  return CallDisplayFunction(display, &SDMDisplay::getDisplayDecorationSupport,
                             format, alpha);
}

void ConcurrencyMgr::PerformQsyncCallback(Display display, bool qsync_enabled,
                                          uint32_t refresh_rate,
                                          uint32_t qsync_refresh_rate) {
  callbacks_.NotifyQsyncChange(display, qsync_enabled, refresh_rate, qsync_refresh_rate);
}

void ConcurrencyMgr::PerformIdleStatusCallback(Display display) {
  if (sdm_display_[display]->IsDisplayIdle()) {
    DTRACE_SCOPED();
    NotifyIdleStatus(true);
  }
}

int ConcurrencyMgr::NotifyIdleStatus(bool idle_status) {
  callbacks_.NotifyIdleStatus(true);
  return 0;
}

DisplayError
ConcurrencyMgr::PresentDisplay(Display display,
                               shared_ptr<Fence> *out_retire_fence) {
  auto status = kErrorParameters;
  DTRACE_SCOPED();

  if (display >= kNumDisplays) {
    DLOGW("Invalid Display : display = %" PRIu64, display);
    return kErrorParameters;
  }

  HandleSecureSession();

  {
    SEQUENCE_EXIT_SCOPE_LOCK(locker_[display]);
    if (!sdm_display_[display]) {
      DLOGW("Removed Display : display = %" PRIu64, display);

      return kErrorParameters;
    }

    if (out_retire_fence == nullptr) {
      return kErrorParameters;
    }

    if (pending_power_mode_[display]) {
      status = kErrorNone;
    } else {
      sdm_display_[display]->ProcessActiveConfigChange();
      status = sdm_display_[display]->Present(out_retire_fence);
      if (status == kErrorNone) {
        PostCommitLocked(display, *out_retire_fence);
      }
    }
  }

  if (status != kErrorNone && status != kErrorNotValidated) {
    if (clients_waiting_for_commit_[display].any()) {
      retire_fence_[display] = nullptr;
      commit_error_[display] = kErrorNotSupported;
      clients_waiting_for_commit_[display].reset();
    }
    SEQUENCE_CANCEL_SCOPE_LOCK(locker_[display]);
  }

  PostCommitUnlocked(display, *out_retire_fence);

  return status;
}

void ConcurrencyMgr::PostCommitLocked(Display display,
                                      shared_ptr<Fence> &retire_fence) {
  // Check if hwc's refresh trigger is getting exercised.
  if (client_pending_refresh_.test(UINT32(display))) {
    sdm_display_[display]->SetPendingRefresh();
    client_pending_refresh_.reset(UINT32(display));
  }
  PerformIdleStatusCallback(display);

  if (clients_waiting_for_commit_[display].any()) {
    retire_fence_[display] = retire_fence;
    commit_error_[display] = 0;
    clients_waiting_for_commit_[display].reset();
  }
}

void ConcurrencyMgr::PostCommitUnlocked(Display display,
                                        const shared_ptr<Fence> &retire_fence) {
  HandlePendingPowerMode(display, retire_fence);
  HandlePendingHotplug(display, retire_fence);
  HandlePendingRefresh();
  std::unique_lock<std::mutex> caller_lock(hotplug_mutex_);
  if (!resource_ready_) {
    resource_ready_ = true;
    active_display_id_ = display;
    cached_retire_fence_ = retire_fence;
    hotplug_cv_.notify_one();
  }
}

void ConcurrencyMgr::HandlePendingRefresh() {
  if (pending_refresh_.none()) {
    return;
  }

  for (size_t i = 0; i < pending_refresh_.size(); i++) {
    if (pending_refresh_.test(i)) {
      Refresh(i);
      break;
    }
  }

  pending_refresh_.reset();
}

void ConcurrencyMgr::SendHotplug(Display display, bool state) {
  callbacks_.OnHotplug(display, state);
}

DisplayError ConcurrencyMgr::Hotplug(Display display, bool state) {
  DTRACE_SCOPED();

  // If client has not registered hotplug, wait for it finitely:
  //   1. spurious wake up (!hotplug_), wait again
  //   2. error = ETIMEDOUT, return with warning 1
  //   3. error != ETIMEDOUT, return with warning 2
  //   4. error == NONE and no spurious wake up, then !hotplug_ is false, exit
  //   loop
  while (!client_connected_) {
    DLOGW("Attempting to send client a hotplug too early Display = %" PRIu64
          " state = %d",
          display, state);
    int ret = client_lock_.WaitFinite(5000);
    if (ret == ETIMEDOUT) {
      DLOGW("Client didn't connect on time, dropping hotplug!");
      return kErrorTimeOut;
    } else if (ret != 0) {
      DLOGW("Failed client connection wait. Error %s, dropping hotplug!",
            strerror(ret));
      return kErrorNotSupported;
    }
  }

  // External display hotplug events are handled asynchronously
  if (display == SDM_DISPLAY_EXTERNAL || display == SDM_DISPLAY_EXTERNAL_2) {
    std::thread(&ConcurrencyMgr::SendHotplug, this, display, state).detach();
  } else {
    callbacks_.OnHotplug(display, state);
  }
  return kErrorNone;
}

void ConcurrencyMgr::RegisterCompositorCallback(SDMCompositorCbIntf *cb, bool enable) {
  SCOPE_LOCK(client_lock_);
  callbacks_.RegisterCallback(cb, enable);

  // Detect if client died and now is back
  vector<Display> pending_hotplugs;

  if (enable && client_connected_) {
    for (auto &map_info : disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_BUILTIN_2)) {
      SCOPE_LOCK(locker_[map_info.client_id]);

      if (sdm_display_[map_info.client_id]) {
        pending_hotplugs.push_back(static_cast<Display>(map_info.client_id));
      }
    }

    for (auto &map_info : disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_EXTERNAL)) {
      SCOPE_LOCK(locker_[map_info.client_id]);

      if (sdm_display_[map_info.client_id]) {
        pending_hotplugs.push_back(static_cast<Display>(map_info.client_id));
      }
    }
  }

  client_connected_ = enable;
  if (enable) {
    if (sdm_display_[SDM_DISPLAY_PRIMARY]) {
      DLOGI("Hotplugging primary...");
      Hotplug(SDM_DISPLAY_PRIMARY, true);
    }

    // Create displays since they should now have their final display indices
    // set.
    DLOGI("Handling built-in displays...");
    {
      SCOPE_LOCK(primary_display_lock_);
      while (primary_pending_) {
        primary_display_lock_.Wait();
      }

      if (disp_->HandleBuiltInDisplays()) {
        DLOGW("Failed handling built-in displays.");
      }
    }

    {
      DLOGI("Handling pluggable displays...");
      int32_t err = disp_->HandlePluggableDisplays(false);
      if (err) {
        DLOGW("All displays could not be created. Error %d '%s'", err,
              strerror(abs(err)));
      }
    }

    // If previously registered, call hotplug for all connected displays to
    // refresh
    if (client_connected_) {
      std::vector<Display> updated_pending_hotplugs;
      for (auto client_id : pending_hotplugs) {
        SCOPE_LOCK(locker_[client_id]);
        // check if the display is unregistered
        if (sdm_display_[client_id]) {
          updated_pending_hotplugs.push_back(client_id);
        }
      }
      for (auto client_id : updated_pending_hotplugs) {
        DLOGI("Re-hotplug display connected: client id = %d",
              UINT32(client_id));
        Hotplug(client_id, true);
      }
    }
  }

  // Notfify all displays.
  NotifyClientStatus(client_connected_);

  // On SF stop, disable the idle time.
  if (!enable && is_client_up_ &&
      sdm_display_[SDM_DISPLAY_PRIMARY]) { // De-registering…
    DLOGI("disable idle time");
    sdm_display_[SDM_DISPLAY_PRIMARY]->SetIdleTimeoutMs(0, 0);
    is_client_up_ = false;
    sdm_display_[SDM_DISPLAY_PRIMARY]->MarkClientActive(false);
  }

  client_lock_.Broadcast();
  DLOGI("Compositor callbacks: %s", enable ? "Enabled" : "Disabled");
}

DisplayError ConcurrencyMgr::SetActiveConfig(Display display, int32_t config) {
  return CallDisplayFunction(display, &SDMDisplay::SetActiveConfig,
                             static_cast<Config>(config));
}

DisplayError ConcurrencyMgr::SetClientTarget(
    uint64_t display, const SnapHandle *target, shared_ptr<Fence> acquire_fence,
    int32_t dataspace, const SDMRegion &damage, uint32_t version) {
  DTRACE_SCOPED();

  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  SCOPE_LOCK(locker_[display]);
  auto status = kErrorParameters;
  if (sdm_display_[display]) {
    auto sdm_display = sdm_display_[display];
    status = sdm_display->SetClientTarget(target, acquire_fence, dataspace,
                                          damage, version);
  }

  return status;
}

DisplayError ConcurrencyMgr::SetColorMode(Display display,
                                          int32_t /*SDMColorMode*/ int_mode) {
  auto mode = static_cast<SDMColorMode>(int_mode);
  if (mode < SDMColorMode::COLOR_MODE_NATIVE ||
      mode > SDMColorMode::COLOR_MODE_DISPLAY_BT2020) {
    return kErrorParameters;
  }
  return CallDisplayFunction(display, &SDMDisplay::SetColorMode, mode);
}

DisplayError ConcurrencyMgr::SetColorModeWithRenderIntent(
    uint64_t display, int32_t /*SDMColorMode*/ int_mode,
    int32_t /*RenderIntent*/ int_render_intent) {
  auto mode = static_cast<SDMColorMode>(int_mode);
  if (mode < SDMColorMode::COLOR_MODE_NATIVE ||
      mode > SDMColorMode::COLOR_MODE_DISPLAY_BT2020) {
    return kErrorParameters;
  }

  if ((int_render_intent < 0) ||
      (int_render_intent > MAX_EXTENDED_RENDER_INTENT)) {
    DLOGE("Invalid RenderIntent: %d", int_render_intent);
    return kErrorParameters;
  }

  auto render_intent = static_cast<SDMRenderIntent>(int_render_intent);
  return CallDisplayFunction(display, &SDMDisplay::SetColorModeWithRenderIntent,
                             mode, render_intent);
}

DisplayError
ConcurrencyMgr::SetColorTransform(Display display,
                                  const std::vector<float> &matrix) {
  if (matrix.empty()) {
    return kErrorParameters;
  }

  // clang-format off
  constexpr std::array<float, 16> kIdentity = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
  };
  // clang-format on
  const bool isIdentity =
      (std::equal(matrix.begin(), matrix.end(), kIdentity.begin()));
  const SDMColorTransform hint =
      isIdentity ? SDMColorTransform::TRANSFORM_IDENTITY
                 : SDMColorTransform::TRANSFORM_ARBITRARY_MATRIX;

  SDMColorTransform transform_hint = static_cast<SDMColorTransform>(hint);
  return CallDisplayFunction(display, &SDMDisplay::SetColorTransform,
                             static_cast<const float *>(matrix.data()),
                             transform_hint);
}

DisplayError ConcurrencyMgr::SetCursorPosition(Display display, LayerId layer,
                                               int32_t x, int32_t y) {
  auto status = kErrorNone;
  status =
      CallDisplayFunction(display, &SDMDisplay::SetCursorPosition, layer, x, y);

  return status;
}

DisplayError ConcurrencyMgr::SetDisplayElapseTime(Display display,
                                                  uint64_t time) {
  return CallDisplayFunction(display, &SDMDisplay::SetDisplayElapseTime, time);
}

DisplayError
ConcurrencyMgr::SetOutputBuffer(uint64_t display, const SnapHandle *buffer,
                                const shared_ptr<Fence> &release_fence) {
  if (buffer == nullptr) {
    return kErrorParameters;
  }

  bool found = false;
  for (auto disp : {qdutilsDisplayType::DISPLAY_VIRTUAL, qdutilsDisplayType::DISPLAY_VIRTUAL_2}) {
    if (INT32(display) == disp_->GetDisplayIndex(disp)) {
      found = true;
      break;
    }
  }

  if (!found) {
    return kErrorNotSupported;
  }

  SCOPE_LOCK(locker_[display]);
  if (sdm_display_[display]) {
    auto vds = reinterpret_cast<SDMDisplayVirtual *>(sdm_display_[display]);
    auto status = vds->SetOutputBuffer(buffer, release_fence);
    return status;
  } else {
    return kErrorParameters;
  }
}

void ConcurrencyMgr::UpdateThrottlingRate() {
  uint32_t new_min = 0;

  for (int i = 0; i < kNumDisplays; i++) {
    auto &display = sdm_display_[i];
    if (!display)
      continue;
    if (display->GetCurrentPowerMode() != SDMPowerMode::POWER_MODE_OFF)
      new_min = (new_min == 0)
                    ? display->GetMaxRefreshRate()
                    : std::min(new_min, display->GetMaxRefreshRate());
  }

  SetNewThrottlingRate(new_min);
}

void ConcurrencyMgr::SetNewThrottlingRate(const uint32_t new_rate) {
  if (new_rate != 0 && throttling_refresh_rate_ != new_rate) {
    SDMDisplay::SetThrottlingRefreshRate(new_rate);
    throttling_refresh_rate_ = new_rate;
  }
}

DisplayError ConcurrencyMgr::SetPowerMode(uint64_t display, int32_t int_mode) {
  if (display >= kNumDisplays || !sdm_display_[display]) {
    return kErrorParameters;
  }

  //  validate device and also avoid undefined behavior in cast to SDMPowerMode
  if (int_mode < INT32(SDMPowerMode::POWER_MODE_OFF) ||
      int_mode > INT32(SDMPowerMode::POWER_MODE_ON_SUSPEND)) {
    return kErrorParameters;
  }

  DTRACE_BEGIN(
      ("Setting power mode " + to_string(int_mode) + " on display " + to_string(display)).c_str());

  auto mode = static_cast<SDMPowerMode>(int_mode);
  bool is_builtin = false;
  bool is_power_off = false;

  // Treat ON_SUSPEND as ON to avoid VTS failure
  // VTS groups both suspend modes for  testing purposes
  // Although ON_SUSPEND (wearables mode) isn't supported by hardware, there is
  // no functional impact of treating it as ON for mobile devices
  mode = (mode == SDMPowerMode::POWER_MODE_ON_SUSPEND)
             ? SDMPowerMode::POWER_MODE_ON
             : mode;

  if (mode == SDMPowerMode::POWER_MODE_ON &&
      !disp_->IsHWDisplayConnected(display)) {
    DTRACE_END();
    return kErrorParameters;
  }

  // When secure session going on primary, if power request comes on second
  // built-in, cache it and process once secure session ends. Allow power off
  // transition during secure session.
  {
    SCOPE_LOCK(locker_[display]);
    if (sdm_display_[display]) {
      is_builtin =
          (sdm_display_[display]->GetDisplayClass() == DISPLAY_CLASS_BUILTIN);
      is_power_off = (sdm_display_[display]->GetCurrentPowerMode() ==
                      SDMPowerMode::POWER_MODE_OFF);
    }
  }
  if (secure_session_active_ && is_builtin && is_power_off) {
    if (disp_->GetActiveBuiltinDisplay() != kNumDisplays) {
      DLOGI("Secure session in progress, defer power state change");
      SCOPE_LOCK(locker_[display]);
      if (sdm_display_[display]) {
        sdm_display_[display]->SetPendingPowerMode(mode);
        DTRACE_END();
        return kErrorNone;
      }
    }
  }
  if (pending_power_mode_[display]) {
    DLOGW("Set power mode is not allowed during secure display session");
    DTRACE_END();
    return kErrorNotSupported;
  }

  //  all displays support on/off. Check for doze modes
  int support = 0;
  auto status = GetDozeSupport(display, &support);
  if (status != kErrorNone) {
    if (is_builtin) {
      DLOGE("Failed to get doze support Error = %d", status);
    }
    DTRACE_END();
    return status;
  }

  if (!support && (mode == SDMPowerMode::POWER_MODE_DOZE ||
                   mode == SDMPowerMode::POWER_MODE_DOZE_SUSPEND)) {
    DTRACE_END();
    return kErrorNotSupported;
  }

  SDMPowerMode last_power_mode = sdm_display_[display]->GetCurrentPowerMode();

  if (last_power_mode == mode) {
    DTRACE_END();
    return kErrorNone;
  }

  auto error = CallDisplayFunction(display, &SDMDisplay::SetPowerMode, mode,
                                   false /* teardown */);
  if (error != kErrorNone) {
    DTRACE_END();
    return error;
  }
  // Reset idle pc ref count on suspend, as we enable idle pc during suspend.
  if (mode == SDMPowerMode::POWER_MODE_OFF) {
    idle_pc_ref_cnt_ = 0;
  }

  UpdateThrottlingRate();

  if (mode == SDMPowerMode::POWER_MODE_DOZE) {
    // Trigger one more refresh for PP features to take effect.
    pending_refresh_.set(UINT32(display));
  }

  DTRACE_END();
  return kErrorNone;
}

DisplayError ConcurrencyMgr::SetVsyncEnabled(uint64_t display, bool enabled) {
  //  avoid undefined behavior in cast to Vsync
  if (enabled) {
    vsync_source_ = display;
  }

  return CallDisplayFunction(display, &SDMDisplay::SetVsyncEnabled, enabled);
}

DisplayError ConcurrencyMgr::SetDimmingEnable(uint64_t display,
                                              int32_t int_enabled) {
  return CallDisplayFunction(display, &SDMDisplay::SetDimmingEnable,
                             int_enabled);
}

DisplayError ConcurrencyMgr::SetDimmingMinBl(Display display, int32_t min_bl) {
  return CallDisplayFunction(display, &SDMDisplay::SetDimmingMinBl, min_bl);
}

DisplayError ConcurrencyMgr::SetDemuraState(Display display, int32_t state) {
  return CallDisplayFunction(display, &SDMDisplay::SetDemuraState, state);
}

DisplayError ConcurrencyMgr::SetDemuraConfig(Display display,
                                             int32_t demura_idx) {
  return CallDisplayFunction(display, &SDMDisplay::SetDemuraConfig, demura_idx);
}

DisplayError ConcurrencyMgr::GetDozeSupport(Display display,
                                            int32_t *out_support) {
  if (!out_support) {
    return kErrorParameters;
  }

  if (display >= kNumDisplays || (sdm_display_[display] == nullptr)) {
    // display may come as -1  from VTS test case
    DLOGW("Invalid Display %d ", UINT32(display));
    return kErrorParameters;
  }

  *out_support = 0;
  if (sdm_display_[display]->GetDisplayClass() == DISPLAY_CLASS_BUILTIN) {
    *out_support = 1;
  }

  return kErrorNone;
}

// Qclient methods
DisplayError ConcurrencyMgr::NotifyCallback(uint32_t command,
                                            SDMParcel *input_parcel,
                                            SDMParcel *output_parcel) {
  auto ret = services_->notifyCallback(command, input_parcel, output_parcel);

  return ret;
}

void ConcurrencyMgr::HpdEventHandler() {
  // Drop hotplug uevents until SurfaceFlinger (the client) is connected. The
  // equivalent of hotplug uevent handling will be done once when SurfaceFlinger
  // connects, at RegisterCallback(). Since HandlePluggableDisplays() reads the
  // latest connection states of all displays, no uevent is lost.
  if (!client_connected_) {
    return;
  }

  // Handle hotplug.
  int32_t err = disp_->HandlePluggableDisplays(true);
  if (err) {
    DLOGW("Hotplug handling failed. Error %d '%s'", err, strerror(abs(err)));
  }

  // Pass on legacy HDMI hot-plug event
  if (hpd_connected_ != -1) {
    callbacks_.OnHdmiHotplug(hpd_connected_);
  }
}

DisplayError ConcurrencyMgr::GetVsyncPeriod(Display disp,
                                            uint32_t *vsync_period) {
  if (disp >= kNumDisplays) {
    DLOGW("Invalid Display : display = %" PRIu64, disp);
    return kErrorParameters;
  }

  SCOPE_LOCK(locker_[(int)disp]);
  // default value
  *vsync_period = 1000000000ul / 60;

  DisplayConfigVariableInfo attributes{};
  if (sdm_display_[disp]) {
    sdm_display_[disp]->GetDisplayAttributes(0, &attributes);
  }

  *vsync_period = INT32(attributes.vsync_period_ns);

  return kErrorNone;
}

void ConcurrencyMgr::SendRefresh(Display display) {
  callbacks_.OnRefresh(display);
}

void ConcurrencyMgr::Refresh(uint64_t display) {
  SCOPE_LOCK(client_lock_);

  std::thread(&ConcurrencyMgr::SendRefresh, this, display).detach();
  client_pending_refresh_.set(UINT32(display));
}

void ConcurrencyMgr::CompositorSync(CompositorSyncType sync_type) {
  if (sync_type == CompositorSyncTypeAcquire) {
    command_seq_mutex_.lock();
    tui_mutex_.lock();
  } else {
    command_seq_mutex_.unlock();
    tui_mutex_.unlock();
  }
}

void ConcurrencyMgr::PerformDisplayPowerReset() {
  disp_->RemoveDisconnectedPluggableDisplays();

  // Wait until all commands are flushed.
  std::lock_guard<std::mutex> lock(command_seq_mutex_);
  std::lock_guard<std::mutex> tui_lock(tui_mutex_);

  // Acquire lock on all displays.
  for (Display display = SDM_DISPLAY_PRIMARY; display < kNumDisplays;
       display++) {
    locker_[display].Lock();
  }

  DisplayError status = kErrorNone;
  SDMPowerMode last_power_mode[kNumDisplays] = {};

  for (Display display = SDM_DISPLAY_PRIMARY; display < kNumDisplays;
       display++) {
    if (sdm_display_[display] != NULL) {
      last_power_mode[display] = sdm_display_[display]->GetCurrentPowerMode();
      DLOGI("Powering off display = %d", INT32(display));
      status = sdm_display_[display]->SetPowerMode(SDMPowerMode::POWER_MODE_OFF,
                                                   true /* teardown */);
      if (status != kErrorNone) {
        DLOGE("Power off for display = %d failed with error = %d",
              INT32(display), status);
      }
    }
  }

  for (Display display = SDM_DISPLAY_PRIMARY; display < kNumDisplays;
       display++) {
    if (sdm_display_[display] != NULL) {
      SDMPowerMode mode = last_power_mode[display];
      DLOGI("Setting display %d to mode = %d", INT32(display), mode);
      status = sdm_display_[display]->SetPowerMode(mode, false /* teardown */);
      if (status != kErrorNone) {
        DLOGE("%d mode for display = %d failed with error = %d", mode,
              INT32(display), status);
      }
      SDMColorMode color_mode = sdm_display_[display]->GetCurrentColorMode();
      SDMRenderIntent render_intent =
          sdm_display_[display]->GetCurrentRenderIntent();
      status = sdm_display_[display]->SetColorModeWithRenderIntent(
          color_mode, render_intent);
      if (status != kErrorNone) {
        DLOGE("SetColorMode failed for display = %d error = %d", INT32(display),
              status);
      }
    }
  }

  Display vsync_source = vsync_source_;
  // adb shell stop sets vsync source as max display
  if (vsync_source != kNumDisplays && sdm_display_[vsync_source]) {
    status = sdm_display_[vsync_source]->SetVsyncEnabled(true);
    if (status != kErrorNone) {
      DLOGE("Enabling vsync failed for disp: %" PRIu64 " with error = %d",
            vsync_source, status);
    }
  }

  // Release lock on all displays.
  for (Display display = SDM_DISPLAY_PRIMARY; display < kNumDisplays;
       display++) {
    locker_[display].Unlock();
  }

  Refresh(vsync_source);
}

void ConcurrencyMgr::DisplayPowerReset() {
  // Do Power Reset in a different thread to avoid blocking of SDM event thread
  // when disconnecting display.
  std::thread(&ConcurrencyMgr::PerformDisplayPowerReset, this).detach();
}

void ConcurrencyMgr::VmReleaseDone(Display display) {
  tui_->VmReleaseDone(display);
}

void ConcurrencyMgr::HandleSecureSession() {
  std::bitset<kSecureMax> secure_sessions = 0;
  Display client_id = kNumDisplays;
  {
    // TODO(user): Revisit if supporting secure display on non-primary.
    Display active_builtin_disp_id = disp_->GetActiveBuiltinDisplay();
    if (active_builtin_disp_id >= kNumDisplays) {
      return;
    }
    Locker::ScopeLock lock_d(locker_[active_builtin_disp_id]);
    sdm_display_[active_builtin_disp_id]->GetActiveSecureSession(
        &secure_sessions);
  }

  if (secure_sessions[kSecureDisplay] || secure_sessions[kSecureCamera]) {
    secure_session_active_ = true;
  } else if (!secure_session_active_) {
    // No secure session active. No secure session transition to handle. Skip
    // remaining steps.
    return;
  }

  // If there are any ongoing non-secure virtual displays, we need to destroy
  // them.
  bool is_active_virtual_display = false;
  for (auto &map_info : disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_VIRTUAL)) {
    if (map_info.disp_type == kVirtual) {
      is_active_virtual_display = true;
      client_id = map_info.client_id;
    }
  }
  if (is_active_virtual_display) {
    disp_->DestroyVirtualDisplay(client_id);
  }

  // If it is called during primary prepare/commit, we need to pause any ongoing
  // commit on external/virtual display.
  bool found_active_secure_display = false;
  for (Display display = SDM_DISPLAY_PRIMARY; display < kNumRealDisplays;
       display++) {
    Locker::ScopeLock lock_d(locker_[display]);
    SDMDisplay *sdm_display = sdm_display_[display];
    if (!sdm_display) {
      continue;
    }

    bool is_active_secure_display = false;
    // The first On/Doze/DozeSuspend built-in display is taken as the secure
    // display.
    if (!found_active_secure_display &&
        sdm_display->GetDisplayClass() == DISPLAY_CLASS_BUILTIN &&
        sdm_display->GetCurrentPowerMode() != SDMPowerMode::POWER_MODE_OFF) {
      is_active_secure_display = true;
      found_active_secure_display = true;
    }
    sdm_display->HandleSecureSession(secure_sessions,
                                     &pending_power_mode_[display],
                                     is_active_secure_display);
  }
}

void ConcurrencyMgr::HandlePendingPowerMode(
    Display disp_id, const shared_ptr<Fence> &retire_fence) {
  if (!secure_session_active_) {
    // No secure session active. Skip remaining steps.
    return;
  }

  Display active_builtin_disp_id = disp_->GetActiveBuiltinDisplay();
  if (disp_id != active_builtin_disp_id) {
    return;
  }

  Locker::ScopeLock lock_d(locker_[active_builtin_disp_id]);
  bool pending_power_mode = false;
  std::bitset<kSecureMax> secure_sessions = 0;
  sdm_display_[active_builtin_disp_id]->GetActiveSecureSession(
      &secure_sessions);
  for (Display display = SDM_DISPLAY_PRIMARY + 1; display < kNumDisplays;
       display++) {
    if (display != active_builtin_disp_id) {
      Locker::ScopeLock lock_d(locker_[display]);
      if (pending_power_mode_[display]) {
        pending_power_mode = true;
        break;
      }
    }
  }

  if (!pending_power_mode) {
    if (!secure_sessions.any()) {
      secure_session_active_ = false;
    }
    return;
  }

  // retire fence is set only after successful primary commit, So check for
  // retire fence to know non secure commit went through to notify driver to
  // change the CRTC mode to non secure. Otherwise any commit to non-primary
  // display would fail.
  if (retire_fence == nullptr) {
    return;
  }

  Fence::Wait(retire_fence);

  SCOPE_LOCK(locker_[pluggable_lock_index_]);
  HWDisplaysInfo hw_displays_info = {};
  DisplayError error = core_intf_->GetDisplaysStatus(&hw_displays_info);
  if (error != kErrorNone) {
    DLOGE("Failed to get connected display list. Error = %d", error);
    return;
  }

  for (Display display = SDM_DISPLAY_PRIMARY + 1; display < kNumDisplays;
       display++) {
    if (display == active_builtin_disp_id) {
      continue;
    }

    Locker::ScopeLock lock_d(locker_[display]);
    if (!pending_power_mode_[display] || !sdm_display_[display]) {
      continue;
    }

    // check if a pluggable display which is in pending power state is already
    // disconnected. In such cases, avoid powering up the display. It will be
    // disconnected as part of HandlePendingHotplug.
    bool disconnected = false;
    DisplayMapInfo *disp_map_info = nullptr;

    for (auto &map_info : disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_EXTERNAL)) {
      if (display != map_info.client_id) {
        continue;
      }

      for (auto &iter : hw_displays_info) {
        auto &info = iter.second;
        if (info.display_id == map_info.sdm_id && !info.is_connected) {
          disconnected = true;
          break;
        }
      }

      disp_map_info = &map_info;
      break;
    }

    if (disconnected) {
      continue;
    }

    SDMPowerMode pending_mode = sdm_display_[display]->GetPendingPowerMode();

    if (pending_mode == SDMPowerMode::POWER_MODE_OFF ||
        pending_mode == SDMPowerMode::POWER_MODE_DOZE_SUSPEND) {
      disp_->GetActiveDisplays().erase(display);
    } else {
      if (disp_map_info != nullptr) {
        disp_->GetActiveDisplays().insert(std::make_pair(disp_map_info->client_id, disp_map_info));
      }
    }
    DisplayError error =
        sdm_display_[display]->SetPowerMode(pending_mode, false);
    if (kErrorNone == error) {
      pending_power_mode_[display] = false;
      sdm_display_[display]->ClearPendingPowerMode();
      pending_refresh_.set(UINT32(SDM_DISPLAY_PRIMARY));
    } else {
      DLOGE("SetDisplayStatus error = %d (%s)", error,
            to_string(error).c_str());
    }
  }

  secure_session_active_ = false;
}

void ConcurrencyMgr::HandlePendingHotplug(
    Display disp_id, const shared_ptr<Fence> &retire_fence) {
  Display active_builtin_disp_id = disp_->GetActiveBuiltinDisplay();
  if (disp_id != active_builtin_disp_id) {
    return;
  }

  std::bitset<kSecureMax> secure_sessions = 0;
  if (active_builtin_disp_id < kNumDisplays) {
    Locker::ScopeLock lock_d(locker_[active_builtin_disp_id]);
    sdm_display_[active_builtin_disp_id]->GetActiveSecureSession(
        &secure_sessions);
  }

  if (secure_sessions.any() || active_builtin_disp_id >= kNumDisplays) {
    return;
  }

  Display virtual_display_idx = disp_->GetDisplayIndex(qdutilsDisplayType::DISPLAY_VIRTUAL);
  if (sdm_display_[virtual_display_idx]) {
    return;
  }

  int32_t err = locker_[pluggable_lock_index_].TryLock();
  if (!err) {
    // Do hotplug handling in a different thread to avoid blocking
    // PresentDisplay.
    disp_->HandlePluggableDisplaysAsync();
    locker_[pluggable_lock_index_].Unlock();
  }
}

DisplayError ConcurrencyMgr::GetReadbackBufferAttributes(Display display,
                                                         int32_t *format,
                                                         int32_t *dataspace) {
  if (!format || !dataspace) {
    return kErrorParameters;
  }

  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  if (display != SDM_DISPLAY_PRIMARY) {
    return kErrorNotSupported;
  }

  SDMDisplay *sdm_display = sdm_display_[display];
  if (sdm_display == nullptr) {
    return kErrorParameters;
  } else if (!sdm_display->HasReadBackBufferSupport()) {
    return kErrorNotSupported;
  }

  *format = static_cast<int32_t>(SDMPixelFormat::PIXEL_FORMAT_RGB_888);
  uint32_t cm_dataspace = 0;
  Dataspace ds;
  GetColorMetadataFromColorMode(sdm_display->GetCurrentColorMode(), ds);
  buffer_allocator_->ColorMetadataToDataspace(ds, &cm_dataspace);
  *dataspace = static_cast<int32_t>(cm_dataspace);

  return kErrorNone;
}

DisplayError
ConcurrencyMgr::SetReadbackBuffer(uint64_t display, void *buffer,
                                  const shared_ptr<Fence> &acquire_fence) {
  if (!buffer) {
    return kErrorParameters;
  }

  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  if (display != SDM_DISPLAY_PRIMARY) {
    return kErrorNotSupported;
  }

  int virtual_dpy_index = disp_->GetDisplayIndex(qdutilsDisplayType::DISPLAY_VIRTUAL);
  if ((virtual_dpy_index != -1) && sdm_display_[virtual_dpy_index]) {
    return kErrorNotSupported;
  }

  CwbConfig cwb_config = {}; /* SF uses LM tappoint*/

  return CallDisplayFunction(display, &SDMDisplay::SetReadbackBuffer, buffer,
                             acquire_fence, cwb_config, kCWBClientComposer);
}

DisplayError ConcurrencyMgr::HandleCwbCallBack(int display_index, void *buffer,
                                               const CwbConfig &cwb_config) {
  SCOPE_LOCK(locker_[display_index]);

  // Get display instance using display type.
  SDMDisplay *sdm_display = sdm_display_[display_index];
  if (!sdm_display) {
    return kErrorNotSupported;
  }

  // Send CWB request to CWB Manager
  auto err = sdm_display->SetReadbackBuffer((void *)buffer, nullptr, cwb_config,
                                            kCWBClientExternal);
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError
ConcurrencyMgr::GetReadbackBufferFence(uint64_t display,
                                       shared_ptr<Fence> *release_fence) {
  if (!release_fence) {
    return kErrorParameters;
  }

  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  if (display != SDM_DISPLAY_PRIMARY) {
    return kErrorNotSupported;
  }

  return CallDisplayFunction(display, &SDMDisplay::GetReadbackBufferFence,
                             release_fence);
}

DisplayError ConcurrencyMgr::GetDisplayIdentificationData(Display display,
                                                          uint8_t *outPort,
                                                          uint32_t *outDataSize,
                                                          uint8_t *outData) {
  if (!outPort || !outDataSize) {
    return kErrorParameters;
  }

  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  return CallDisplayFunction(display, &SDMDisplay::GetDisplayIdentificationData,
                             outPort, outDataSize, outData);
}

DisplayError ConcurrencyMgr::GetDisplayCapabilities(
    Display display, vector<SDMDisplayCapability> *capabilities) {
  if (!capabilities) {
    return kErrorParameters;
  }

  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  if (!sdm_display_[display]) {
    DLOGE("Expected valid sdm_display");
    return kErrorParameters;
  }

  bool isBuiltin =
      (sdm_display_[display]->GetDisplayClass() == DISPLAY_CLASS_BUILTIN);
  if (isBuiltin) {
    int32_t has_doze_support = 0;
    GetDozeSupport(display, &has_doze_support);

    // TODO(user): Handle SKIP_CLIENT_COLOR_TRANSFORM based on DSPP availability
    if (has_doze_support) {
      *capabilities = {SDM_CAPS_SKIP_CLIENT_COLOR_TRANSFORM, SDM_CAPS_DOZE,
                       SDM_CAPS_BRIGHTNESS, SDM_CAPS_PROTECTED_CONTENTS};
    } else {
      *capabilities = {SDM_CAPS_SKIP_CLIENT_COLOR_TRANSFORM,
                       SDM_CAPS_BRIGHTNESS, SDM_CAPS_PROTECTED_CONTENTS};
    }
  }

  return kErrorNone;
}

DisplayError ConcurrencyMgr::GetDisplayConnectionType(Display display,
                                                      DisplayClass *type) {
  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  if (!type) {
    return kErrorParameters;
  }

  if (!sdm_display_[display]) {
    DLOGW("Expected valid sdm_display");
    return kErrorParameters;
  }
  *type = sdm_display_[display]->GetDisplayClass();

  return kErrorNone;
}

DisplayError ConcurrencyMgr::GetClientTargetProperty(
    Display display, SDMClientTargetProperty *outClientTargetProperty) {
  if (!outClientTargetProperty) {
    return kErrorParameters;
  }

  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  return CallDisplayFunction(display, &SDMDisplay::GetClientTargetProperty,
                             outClientTargetProperty);
}

DisplayError ConcurrencyMgr::GetDisplayBrightnessSupport(Display display,
                                                         bool *outSupport) {
  if (!outSupport) {
    return kErrorParameters;
  }

  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  if (!sdm_display_[display]) {
    DLOGE("Expected valid sdm_display");
    return kErrorParameters;
  }
  *outSupport =
      (sdm_display_[display]->GetDisplayClass() == DISPLAY_CLASS_BUILTIN);
  return kErrorNone;
}

DisplayError ConcurrencyMgr::SetDisplayBrightness(Display display,
                                                  float brightness) {
  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  if (!sdm_display_[display]) {
    return kErrorParameters;
  }

  return (INT32(sdm_display_[display]->SetPanelBrightness(brightness)))
             ? kErrorNotSupported
             : kErrorNone;
}

void ConcurrencyMgr::NotifyClientStatus(bool connected) {
  for (uint32_t i = 0; i < kNumDisplays; i++) {
    if (!sdm_display_[i]) {
      continue;
    }
    SCOPE_LOCK(locker_[i]);
    sdm_display_[i]->NotifyClientStatus(connected);
    sdm_display_[i]->SetVsyncEnabled(false);
  }

  vsync_source_ = kNumDisplays;
}

DisplayError ConcurrencyMgr::WaitForResources(bool wait_for_resources,
                                              Display active_builtin_id,
                                              Display display_id) {
  std::vector<DisplayMapInfo> map_info = {
      disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_PRIMARY)[0]};
  auto &map_info_builtin = disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_BUILTIN_2);
  std::copy(map_info_builtin.begin(), map_info_builtin.end(),
            std::back_inserter(map_info));

  if (wait_for_resources) {
    bool res_wait = true;
    bool needs_active_builtin_reconfig = false;
    if (enable_primary_reconfig_req_) {
      // todo (user): move this logic to wait for MDP resource
      // reallocation/reconfiguration to SDM module.
      {
        SCOPE_LOCK(locker_[display_id]);
        if (sdm_display_[display_id]) {
          res_wait = sdm_display_[display_id]->CheckResourceState(
              &needs_active_builtin_reconfig);
        } else {
          DLOGW("Display %" PRIu64 "no longer available.", display_id);
          return kErrorParameters;
        }
      }
      if (needs_active_builtin_reconfig) {
        SCOPE_LOCK(locker_[active_builtin_id]);
        if (sdm_display_[active_builtin_id]) {
          Config current_config = 0, new_config = 0;
          sdm_display_[active_builtin_id]->GetActiveConfig(false, &current_config);
          int status = INT32(
              sdm_display_[active_builtin_id]->SetAlternateDisplayConfig(true));
          if (status) {
            DLOGE("Active built-in %" PRIu64
                  " cannot switch to lower resource configuration",
                  active_builtin_id);
            return kErrorNotSupported;
          }
          sdm_display_[active_builtin_id]->GetActiveConfig(false, &new_config);

          // In case of config change, notify client with the new configuration
          if (new_config != current_config) {
            NotifyDisplayAttributes(active_builtin_id, new_config);
          }
        } else {
          DLOGW("Display %" PRIu64 "no longer available.", active_builtin_id);
          return kErrorParameters;
        }
      }
    }
    do {
      if (client_connected_) {
        Refresh(active_builtin_id);
      }
      {
        std::unique_lock<std::mutex> caller_lock(hotplug_mutex_);
        resource_ready_ = false;

        static constexpr uint32_t min_vsync_period_ms = 5000;
        auto timeout = std::chrono::system_clock::now() +
                       std::chrono::milliseconds(min_vsync_period_ms);

        if (hotplug_cv_.wait_until(caller_lock, timeout) ==
            std::cv_status::timeout) {
          DLOGW("hotplug timeout");
          return kErrorResources;
        }

        if (active_display_id_ == active_builtin_id &&
            needs_active_builtin_reconfig && cached_retire_fence_) {
          Fence::Wait(cached_retire_fence_);
        }
      }
      {
        SCOPE_LOCK(locker_[display_id]);
        if (sdm_display_[display_id]) {
          res_wait = sdm_display_[display_id]->CheckResourceState(
              &needs_active_builtin_reconfig);
          if (!enable_primary_reconfig_req_) {
            needs_active_builtin_reconfig = false;
          }
        } else {
          DLOGW("Display %" PRIu64 "no longer available.", display_id);
          return kErrorParameters;
        }
      }
    } while (res_wait || needs_active_builtin_reconfig);
  }

  return kErrorNone;
}

DisplayError
ConcurrencyMgr::GetDisplayVsyncPeriod(Display disp,
                                      VsyncPeriodNanos *vsync_period) {
  if (vsync_period == nullptr) {
    return kErrorParameters;
  }

  return CallDisplayFunction(disp, &SDMDisplay::GetDisplayVsyncPeriod,
                             false, vsync_period);
}

DisplayError ConcurrencyMgr::SetActiveConfigWithConstraints(
    Display display, Config config,
    const SDMVsyncPeriodChangeConstraints *vsync_period_change_constraints,
    SDMVsyncPeriodChangeTimeline *out_timeline) {
  if ((vsync_period_change_constraints == nullptr) ||
      (out_timeline == nullptr)) {
    return kErrorParameters;
  }

  return CallDisplayFunction(
      display, &SDMDisplay::SetActiveConfigWithConstraints, config,
      vsync_period_change_constraints, out_timeline);
}

DisplayError ConcurrencyMgr::WaitForCommitDoneAsync(uint64_t display, int client_id) {
  std::chrono::milliseconds span(2000);
  if (commit_done_future_[display].valid()) {
    std::future_status status =
        commit_done_future_[display].wait_for(std::chrono::milliseconds(0));
    if (status != std::future_status::ready) {
      // Previous task is stuck. Bail out early.
      return kErrorTimeOut;
    }
  }

  commit_done_future_[display] =
      std::async([](ConcurrencyMgr *session, uint64_t display,
                    int client_id) { return session->WaitForCommitDone(display, client_id); },
                 this, display, client_id);
  if (commit_done_future_[display].wait_for(span) ==
      std::future_status::timeout) {
    return kErrorTimeOut;
  }

  return commit_done_future_[display].get();
}

DisplayError ConcurrencyMgr::WaitForCommitDone(Display display, int client_id) {
  shared_ptr<Fence> retire_fence = nullptr;
  int timeout_ms = -1;
  {
    SEQUENCE_WAIT_SCOPE_LOCK(locker_[display]);
    DLOGI("Acquired lock for client %d display %" PRIu64, client_id, display);
    Refresh(display);
    clients_waiting_for_commit_[display].set(client_id);
    locker_[display].Wait();
    if (commit_error_[display] != 0) {
      DLOGE("Commit done failed with error %d for client %d display %" PRIu64,
            commit_error_[display], client_id, display);
      commit_error_[display] = 0;
      return kErrorNotSupported;
    }
    retire_fence = retire_fence_[display];
    retire_fence_[display] = nullptr;
    if (sdm_display_[display]) {
      uint32_t config = 0;
      sdm_display_[display]->GetActiveDisplayConfig(false, &config);
      DisplayConfigVariableInfo display_attributes = {};
      sdm_display_[display]->GetDisplayAttributesForConfig(config,
                                                           &display_attributes);
      timeout_ms =
          kNumDrawCycles * (display_attributes.vsync_period_ns / kDenomNstoMs);
      DLOGI("timeout in ms %d", timeout_ms);
    }
  }

  int ret = Fence::Wait(retire_fence, timeout_ms + kCommitDoneTimeoutMs);
  if (ret != 0) {
    DLOGE(
        "Retire fence wait failed with error %d for client %d display %" PRIu64,
        ret, client_id, display);
    return kErrorTimeOut;
  }
  return kErrorNone;
}

DisplayError ConcurrencyMgr::GetDisplayPortId(uint32_t disp_id, int *port_id) {
  Display target_display = disp_->GetDisplayIndex(disp_id);
  if (target_display == -1) {
    return kErrorNotSupported;
  }
  uint8_t out_port = 0;
  uint32_t out_data_size = 0;

  Locker::ScopeLock lock_d(locker_[target_display]);
  auto disp = GetDisplayFromClientId(target_display);
  if (disp && disp->GetDisplayIdentificationData(&out_port, &out_data_size,
                                                 NULL) == kErrorNone) {
    *port_id = INT(out_port);
  }

  return kErrorNone;
}

DisplayError ConcurrencyMgr::TUIEventHandler(uint64_t disp_id,
                                             SDMTUIEventType event_type) {
  return tui_->TUIEventHandler(disp_id, event_type);
}

DisplayError ConcurrencyMgr::TeardownConcurrentWriteback(Display display) {
  if (!sdm_display_[display]) {
    DLOGW("Invalid display (id = %d) detected as input parameter!", display);
  }

  for (int id = 0; id < kNumRealDisplays; id++) {
    SDMDisplay *disp = nullptr;
    {
      SCOPE_LOCK(locker_[id]);
      if (!sdm_display_[id]) {
        continue;
      }

      int32_t display_type = 0;
      sdm_display_[id]->GetDisplayType(&display_type);
      if (display_type == INT32(SDMDisplayBasicType::kPhysical)) {
        disp = sdm_display_[id];
      }
    }

    if (disp) {
      disp->TeardownConcurrentWriteback();
    }
  }

  return kErrorNone;
}

DisplayError ConcurrencyMgr::CommitOrPrepare(
    Display display, bool validate_only, shared_ptr<Fence> *out_retire_fence,
    uint32_t *out_num_types, uint32_t *out_num_requests, bool *needs_commit) {
  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  {
    // ToDo: add support for async power mode.
    Locker::ScopeLock lock_d(locker_[display]);
    if (!sdm_display_[display]) {
      return kErrorParameters;
    }
    if (pending_power_mode_[display]) {
      return kErrorNone;
    }
  }

  HandleSecureSession();
  auto status = kErrorNone;
  {
    SEQUENCE_ENTRY_SCOPE_LOCK(locker_[display]);
    sdm_display_[display]->ProcessActiveConfigChange();
    sdm_display_[display]->IsMultiDisplay(
        (disp_->GetActiveDisplays().size() > 1) ? true : false);
    status = sdm_display_[display]->CommitOrPrepare(
        validate_only, out_retire_fence, out_num_types, out_num_requests,
        needs_commit);
  }
  if (!(*needs_commit)) {
    {
      SEQUENCE_EXIT_SCOPE_LOCK(locker_[display]);
      PostCommitLocked(display, *out_retire_fence);
    }
    PostCommitUnlocked(display, *out_retire_fence);
  }

  return status;
}

DisplayError ConcurrencyMgr::TryDrawMethod(Display display,
                                           DisplayDrawMethod drawMethod) {
  Locker::ScopeLock lock_d(locker_[display]);
  if (!sdm_display_[display]) {
    return kErrorParameters;
  }

  return sdm_display_[display]->TryDrawMethod(drawMethod);
}

void ConcurrencyMgr::NotifyDisplayAttributes(Display display, Config config) {
  DisplayConfigVariableInfo var_info;
  SDMConfigAttributes attributes{};
  int error = sdm_display_[display]->GetDisplayAttributesForConfig(INT(config),
                                                                   &var_info);
  if (error != kErrorNone) {
    return;
  }

  attributes.vsyncPeriod = var_info.vsync_period_ns;
  attributes.xRes = var_info.x_pixels;
  attributes.yRes = var_info.y_pixels;
  attributes.xDpi = var_info.x_dpi;
  attributes.yDpi = var_info.y_dpi;
  attributes.panelType = SDMDisplayIntf::DEFAULT;
  attributes.isYuv = var_info.is_yuv;

  callbacks_.NotifyResolutionChange(display, attributes);
}

DisplayError
ConcurrencyMgr::SetExpectedPresentTime(Display display,
                                       uint64_t expectedPresentTime) {
  Locker::ScopeLock lock_d(locker_[display]);
  if (!sdm_display_[display]) {
    return kErrorParameters;
  }

  sdm_display_[display]->SetExpectedPresentTime(expectedPresentTime);

  return kErrorNone;
}

DisplayError ConcurrencyMgr::CreateDisplay(SDMDisplayType type, int32_t width,
                                           int32_t height, int32_t *format,
                                           uint64_t *display_id) {
  return CreateVirtualDisplay(width, height, format, display_id);
}

DisplayError ConcurrencyMgr::CreateVirtualDisplay(uint32_t width,
                                                  uint32_t height,
                                                  int32_t *format,
                                                  Display *out_display_id) {
  // Wait until all commands are flushed.
  std::lock_guard<std::mutex> sdm_lock(command_seq_mutex_);
  std::lock_guard<std::mutex> tui_lock(tui_mutex_);

  return disp_->CreateVirtualDisplay(width, height, format, out_display_id);
}

DisplayError ConcurrencyMgr::DestroyVirtualDisplay(Display client_id) {
  // Wait until all commands are flushed.
  std::lock_guard<std::mutex> sdm_lock(command_seq_mutex_);
  std::lock_guard<std::mutex> tui_lock(tui_mutex_);

  return disp_->DestroyVirtualDisplay(client_id);
}

bool WaitForResourceNeeded(SDMPowerMode prev_mode, SDMPowerMode new_mode) {
  return ((prev_mode == SDMPowerMode::POWER_MODE_OFF) &&
          (new_mode == SDMPowerMode::POWER_MODE_ON ||
           new_mode == SDMPowerMode::POWER_MODE_DOZE));
}

DisplayError ConcurrencyMgr::SetDisplayStatus(uint64_t disp_id,
                                              SDMDisplayStatus status) {
  int disp_idx = GetDisplayIndex(disp_id);
  DisplayError err = kErrorNotSupported;
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_id);
    return kErrorNotSupported;
  }

  if (disp_idx == qdutilsDisplayType::DISPLAY_PRIMARY) {
    DLOGE("Not supported for this display");
    return err;
  }

  {
    SEQUENCE_WAIT_SCOPE_LOCK(locker_[disp_idx]);
    if (!sdm_display_[disp_idx]) {
      DLOGW("Display is not connected");
      return err;
    }
    DLOGI("Display = %d, Status = %d", disp_idx, status);
    err = sdm_display_[disp_idx]->SetDisplayStatus(status);
    if (err != 0) {
      return err;
    }
  }

  if (status == kDisplayStatusResume || status == kDisplayStatusPause) {
    Display active_builtin_disp_id = disp_->GetActiveBuiltinDisplay();
    if (active_builtin_disp_id < kNumRealDisplays) {
      Refresh(active_builtin_disp_id);
    }
  }

  return err;
}

DisplayError ConcurrencyMgr::GetConfigCount(uint64_t disp_id, uint32_t *count) {
  return services_->GetConfigCount(disp_id, count);
}

DisplayError ConcurrencyMgr::GetActiveConfigIndex(uint64_t disp_id,
                                                  uint32_t *config) {
  return services_->GetActiveConfigIndex(disp_id, config);
}

DisplayError ConcurrencyMgr::SetActiveConfigIndex(uint64_t disp_id,
                                                  uint32_t config) {
  return services_->SetActiveConfigIndex(disp_id, config);
}

DisplayError ConcurrencyMgr::SetNoisePlugInOverride(uint64_t disp_id,
                                                    bool override_en,
                                                    int32_t attn,
                                                    int32_t noise_zpos) {
  return services_->SetNoisePlugInOverride(disp_id, override_en, attn,
                                           noise_zpos);
}

DisplayError
ConcurrencyMgr::MinHdcpEncryptionLevelChanged(uint64_t disp_id,
                                              uint32_t min_enc_level) {
  return services_->MinHdcpEncryptionLevelChanged(disp_id, min_enc_level);
}

DisplayError ConcurrencyMgr::ControlPartialUpdate(uint64_t disp_id,
                                                  bool enable) {
  DisplayError status = services_->ControlPartialUpdate(disp_id, enable);
  if (status) {
    return status;
  }

  // Todo(user): Unlock it before sending events to client. It may cause
  // deadlocks in future. Wait until partial update control is complete
  auto error = WaitForCommitDone(GetDisplayIndex(disp_id), kClientPartialUpdate);
  if (error != kErrorNone) {
    DLOGW("%s Partial update failed with error %d",
          enable ? "Enable" : "Disable", error);
  }

  return error;
}

DisplayError ConcurrencyMgr::ToggleScreenUpdate(bool on) {
  Display active_builtin_disp_id = disp_->GetActiveBuiltinDisplay();

  if (active_builtin_disp_id >= kNumDisplays) {
    DLOGE("No active displays");
    return kErrorNotSupported;
  }

  return services_->ToggleScreenUpdate(active_builtin_disp_id, on);
}

DisplayError ConcurrencyMgr::SetIdleTimeout(uint32_t value) {
  return services_->SetIdleTimeout(value);
}

DisplayError ConcurrencyMgr::SetCameraLaunchStatus(uint32_t on) {
  return services_->SetCameraLaunchStatus(INT(on));
}

DisplayError ConcurrencyMgr::DisplayBWTransactionPending(bool *status) {
  return services_->DisplayBWTransactionPending(status);
}

DisplayError ConcurrencyMgr::ControlIdlePowerCollapse(bool enable,
                                                      bool synchronous) {
  Display active_builtin_disp_id = disp_->GetActiveBuiltinDisplay();

  return services_->ControlIdlePowerCollapse(active_builtin_disp_id, enable,
                                             synchronous);
}

DisplayError ConcurrencyMgr::IsWbUbwcSupported(bool *value) {
  HWDisplaysInfo hw_displays_info = {};
  DisplayError error = core_intf_->GetDisplaysStatus(&hw_displays_info);
  if (error != kErrorNone) {
    return kErrorNotSupported;
  }

  for (auto &iter : hw_displays_info) {
    auto &info = iter.second;
    if (info.display_type == kVirtual && info.is_wb_ubwc_supported) {
      *value = 1;
    }
  }

  return error;
}

DisplayError ConcurrencyMgr::GetDisplayBrightness(uint64_t display,
                                                  float *brightness) {
  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  return services_->GetDisplayBrightnessPercent(display, brightness);
}

DisplayError
ConcurrencyMgr::getDisplayMaxBrightness(uint32_t display,
                                        uint32_t *max_brightness_level) {
  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  return services_->GetDisplayMaxBrightness(display, max_brightness_level);
}

DisplayError ConcurrencyMgr::SetCameraSmoothInfo(SDMCameraSmoothOp op,
                                                 int32_t fps) {
  callbacks_.NotifyCameraSmoothInfo(op, fps);

  return kErrorNone;
}

DisplayError ConcurrencyMgr::NotifyTUIDone(int ret, int disp_id,
                                           SDMTUIEventType event_type) {
  callbacks_.NotifyTUIEventDone(ret, disp_id, event_type);

  return kErrorNone;
}

DisplayError ConcurrencyMgr::SetContentFps(const std::string &name, int32_t fps) {
  callbacks_.NotifyContentFps(name, fps);

  return kErrorNone;
}

int ConcurrencyMgr::GetDisplayConfigGroup(uint64_t display, DisplayConfigGroupInfo variable_config) {
  int disp_idx = GetDisplayIndex(display);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_idx);
    return kErrorNotSupported;
  }

  SCOPE_LOCK(locker_[disp_idx]);
  if (sdm_display_[disp_idx]) {
    return sdm_display_[disp_idx]->GetDisplayConfigGroup(variable_config);
  }

  return -1;
}

int ConcurrencyMgr::GetProperty(const char *property_name, char *value) {
  return Debug::Get()->GetProperty(property_name, value);
}

int ConcurrencyMgr::GetProperty(const char *property_name, int *value) {
  return Debug::Get()->GetProperty(property_name, value);
}

DisplayError ConcurrencyMgr::SetDisplayDppsAdROI(
    uint64_t display_id, uint32_t h_start, uint32_t h_end, uint32_t v_start,
    uint32_t v_end, uint32_t factor_in, uint32_t factor_out) {
  return services_->SetDisplayDppsAdROI(display_id, h_start, h_end, v_start,
                                        v_end, factor_in, factor_out);
}

DisplayError ConcurrencyMgr::NotifyCwbDone(int dpy_index, int32_t status,
                                           uint64_t handle_id) {
  return cwb_->OnCWBDone(dpy_index, status, handle_id);
}

DisplayError ConcurrencyMgr::GetSupportedDisplayRefreshRates(
    int disp_id, std::vector<uint32_t> *supported_refresh_rates) {
  int disp_idx = GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_id);
    return kErrorNotSupported;
  }

  SCOPE_LOCK(locker_[disp_idx]);

  if (sdm_display_[disp_idx]) {
    return sdm_display_[disp_idx]->GetSupportedDisplayRefreshRates(
        supported_refresh_rates);
  }
  return kErrorNotSupported;
}

DisplayError ConcurrencyMgr::ConfigureDynRefreshRate(SDMBuiltInDisplayOps ops,
                                                     int val) {
  SEQUENCE_WAIT_SCOPE_LOCK(locker_[SDM_DISPLAY_PRIMARY]);
  SDMDisplay *display = sdm_display_[SDM_DISPLAY_PRIMARY];

  if (!display) {
    return kErrorResources;
  }

  return display->Perform(ops, val);
}

DisplayError ConcurrencyMgr::SetDisplayAnimating(uint64_t display_id,
                                                 bool animating) {
  return CallDisplayFunction(display_id, &SDMDisplay::SetDisplayAnimating,
                             animating);
}

void ConcurrencyMgr::UpdateVSyncSourceOnPowerModeOff() {
  update_vsync_on_power_off_ = true;
}

void ConcurrencyMgr::UpdateVSyncSourceOnPowerModeDoze() {
  update_vsync_on_doze_ = true;
}

void ConcurrencyMgr::SetClientUp() {
  is_client_up_ = true;

  auto display = sdm_display_[SDM_DISPLAY_PRIMARY];
  if (!display) {
    DLOGW("display is null");
    return;
  }

  display->MarkClientActive(true);
}

bool ConcurrencyMgr::IsBuiltInDisplay(uint64_t display) {
  return disp_->IsBuiltInDisplay(display);
}

bool ConcurrencyMgr::IsAsyncVDSCreationSupported() {
  return async_vds_creation_;
}

DisplayError ConcurrencyMgr::CreateVirtualDisplay(int width, int height,
                                                  int format) {
  if (!async_vds_creation_) {
    DLOGW("Asynchronous virtual display creation is not supported.");
    return kErrorNotSupported;
  }

  if (!width || !height) {
    DLOGW("Width and height provided are invalid.");
    return kErrorParameters;
  }

  Display virtual_id = 0;
  Display active_builtin_disp_id = disp_->GetActiveBuiltinDisplay();
  auto status =
      disp_->CreateVirtualDisplayObj(width, height, &format, &virtual_id);
  if (status != kErrorNone) {
    DLOGE("Failed to create virtual display: %d", status);
    return status;
  }

  DLOGI("Created virtual display id:%" PRIu64 ", res: %dx%d", virtual_id, width,
        height);

  if (active_builtin_disp_id < sdm::kNumRealDisplays) {
    WaitForResources(true, active_builtin_disp_id, virtual_id);
  }

  return status;
}

DisplayError
ConcurrencyMgr::GetSupportedDSIClock(uint64_t disp_id,
                                     std::vector<int64_t> *bit_clks) {
  SCOPE_LOCK(locker_[disp_id]);

  if (!sdm_display_[disp_id]) {
    DLOGW("Display:%d is not connected", disp_id);
    return kErrorResources;
  }

  return sdm_display_[disp_id]->GetSupportedDSIClock(
      (std::vector<uint64_t> *)bit_clks);
}

DisplayError ConcurrencyMgr::GetDSIClk(uint64_t disp_id, uint64_t *bit_clk) {
  SCOPE_LOCK(locker_[disp_id]);

  if (!sdm_display_[disp_id]) {
    DLOGW("Invalid display:%d", disp_id);
    return kErrorResources;
  }

  return sdm_display_[disp_id]->GetDynamicDSIClock(bit_clk);
}

DisplayError ConcurrencyMgr::SetDSIClk(uint64_t disp_id, uint64_t bit_clk) {
  SCOPE_LOCK(locker_[disp_id]);

  if (!sdm_display_[disp_id]) {
    DLOGW("Invalid display:%d", disp_id);
    return kErrorResources;
  }

  return sdm_display_[disp_id]->ScheduleDynamicDSIClock(bit_clk);
}

DisplayError ConcurrencyMgr::SetQsyncMode(uint64_t disp_id, QSyncMode mode) {
  SEQUENCE_WAIT_SCOPE_LOCK(locker_[disp_id]);

  if (!sdm_display_[disp_id]) {
    DLOGW("Invalid display:%d", disp_id);
    return kErrorResources;
  }

  return sdm_display_[disp_id]->SetQSyncMode(mode);
}

DisplayError ConcurrencyMgr::IsSmartPanelConfig(uint64_t disp_id,
                                                uint32_t config_id,
                                                bool *is_smart) {
  SCOPE_LOCK(locker_[disp_id]);

  if (!sdm_display_[disp_id]) {
    DLOGW("Invalid display:%d", disp_id);
    return kErrorResources;
  }

  if (sdm_display_[disp_id]->GetDisplayClass() != DISPLAY_CLASS_BUILTIN) {
    DLOGW("Smart panel config is only supported on built in displays.");
    return kErrorNotSupported;
  }

  *is_smart = sdm_display_[disp_id]->IsSmartPanelConfig(config_id);

  return kErrorNone;
}

bool ConcurrencyMgr::IsRotatorSupportedFormat(LayerBufferFormat format) {
  if (!core_intf_) {
    DLOGE("core_intf_ not initialized.");
    return false;
  }
  return core_intf_->IsRotatorSupportedFormat(format);
}

DisplayError ConcurrencyMgr::GetDisplayHwId(uint64_t disp_id,
                                            int32_t *disp_hw_id) {
  return disp_->GetDisplayHwId(disp_id, disp_hw_id);
}

bool ConcurrencyMgr::IsModeSwitchAllowed(uint64_t disp_id, int32_t config) {
  int disp_idx = GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    DLOGW("Invalid display = %d", disp_id);
    return false;
  }

  SCOPE_LOCK(locker_[disp_idx]);
  if (!sdm_display_[disp_idx]) {
    DLOGW("Display %d is not connected.", disp_id);
    return false;
  }

  return sdm_display_[disp_idx]->IsModeSwitchAllowed(config);
}

DisplayError ConcurrencyMgr::GetActiveBuiltinDisplay(uint64_t *disp_id) {
  auto disp = disp_->GetActiveBuiltinDisplay();

  if (disp >= kNumDisplays) {
    return kErrorResources;
  }

  *disp_id = disp;
  return kErrorNone;
}

DisplayError ConcurrencyMgr::SetPanelLuminanceAttributes(uint64_t display_id,
                                                         float min_lum,
                                                         float max_lum) {
  std::lock_guard<std::mutex> obj(mutex_lum_);
  disp_->SetLuminance(min_lum, max_lum);

  DLOGI("set max_lum %f, min_lum %f", max_lum, min_lum);
  return kErrorNone;
}

void ConcurrencyMgr::RegisterSideBandCallback(SDMSideBandCompositorCbIntf *cb, bool enable) {
  callbacks_.RegisterSideband(cb, enable);
}

DisplayError ConcurrencyMgr::SetSsrcMode(uint64_t display_id, const std::string &mode_name) {
  int disp_idx = GetDisplayIndex(display_id);
  if (disp_idx == -1) {
    DLOGW("Invalid display = %d", display_id);
    return kErrorResources;
  }

  SCOPE_LOCK(locker_[disp_idx]);
  if (!sdm_display_[disp_idx]) {
    DLOGW("Display %d is not connected.", display_id);
    return kErrorResources;
  }

  return sdm_display_[disp_idx]->SetSsrcMode(mode_name);
}

DisplayError ConcurrencyMgr::EnableCopr(uint64_t display_id, bool enable) {
  int disp_idx = GetDisplayIndex(display_id);
  if (disp_idx == -1) {
    DLOGW("Invalid display = %d", display_id);
    return kErrorParameters;
  }

  SCOPE_LOCK(locker_[disp_idx]);
  if (!sdm_display_[disp_idx]) {
    DLOGW("Display %d is not connected.", display_id);
    return kErrorResources;
  }

  return sdm_display_[disp_idx]->EnableCopr(enable);
}

DisplayError ConcurrencyMgr::GetCoprStats(uint64_t display_id, std::vector<int32_t> *copr_stats) {
  int disp_idx = GetDisplayIndex(display_id);
  if (disp_idx == -1) {
    DLOGW("Invalid display = %d", display_id);
    return kErrorParameters;
  }

  SCOPE_LOCK(locker_[disp_idx]);
  if (!sdm_display_[disp_idx]) {
    DLOGW("Display %d is not connected.", display_id);
    return kErrorResources;
  }

  return sdm_display_[disp_idx]->GetCoprStats(copr_stats);
}

DisplayError ConcurrencyMgr::SetupVRRConfig(uint64_t display) {
  return CallDisplayFunction(display, &SDMDisplay::SetupVRRConfig);
}

DisplayError ConcurrencyMgr::NotifyExpectedPresent(Display display, uint64_t expected_present_time,
                                                   uint32_t frame_interval_ns) {
  return CallDisplayFunction(display, &SDMDisplay::NotifyExpectedPresent, expected_present_time,
                             frame_interval_ns);
}

DisplayError ConcurrencyMgr::SetFrameIntervalNs(Display display, uint32_t frame_interval_ns) {
  Locker::ScopeLock lock_d(locker_[display]);
  if (!sdm_display_[display]) {
    return kErrorParameters;
  }

  sdm_display_[display]->SetFrameIntervalNs(frame_interval_ns);

  return kErrorNone;
}

int ConcurrencyMgr::GetNotifyEptConfig(Display display) {
  int disp_idx = GetDisplayIndex(display);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_idx);
    return -1;
  }

  return sdm_display_[disp_idx]->GetNotifyEptConfig();
}

DisplayError ConcurrencyMgr::SetABCState(uint64_t display_id, bool state) {
  int disp_idx = GetDisplayIndex(display_id);
  if (disp_idx == -1) {
    DLOGW("Invalid display = %d", display_id);
    return kErrorResources;
  }

  SCOPE_LOCK(locker_[disp_idx]);
  if (!sdm_display_[disp_idx]) {
    DLOGW("Display %d is not connected.", display_id);
    return kErrorResources;
  }

  return sdm_display_[disp_idx]->SetABCState(state);
}

DisplayError ConcurrencyMgr::SetABCReconfig(uint64_t display_id) {
  int disp_idx = GetDisplayIndex(display_id);
  if (disp_idx == -1) {
    DLOGW("Invalid display = %d", display_id);
    return kErrorResources;
  }

  SCOPE_LOCK(locker_[disp_idx]);
  if (!sdm_display_[disp_idx]) {
    DLOGW("Display %d is not connected.", display_id);
    return kErrorResources;
  }

  return sdm_display_[disp_idx]->SetABCReconfig();
}

DisplayError ConcurrencyMgr::SetABCMode(uint64_t display_id, string mode_name) {
  int disp_idx = GetDisplayIndex(display_id);
  if (disp_idx == -1) {
    DLOGW("Invalid display = %d", display_id);
    return kErrorResources;
  }

  SCOPE_LOCK(locker_[disp_idx]);
  if (!sdm_display_[disp_idx]) {
    DLOGW("Display %d is not connected.", display_id);
    return kErrorResources;
  }

  return sdm_display_[disp_idx]->SetABCMode(mode_name);
}

DisplayError ConcurrencyMgr::SetPanelFeatureConfig(Display display, int32_t type, void *data) {
  return CallDisplayFunction(display, &SDMDisplay::SetPanelFeatureConfig, type, data);
}

}  // namespace sdm
