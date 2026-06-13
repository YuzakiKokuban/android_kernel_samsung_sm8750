/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SDM_DISPLAY_BUILDER_H__
#define __SDM_DISPLAY_BUILDER_H__

#include <core/core_interface.h>

#include <map>
#include <vector>

#include "sdm_display.h"
#include "sdm_display_builder_cb_intf.h"
#include "sdm_display_builtin.h"
#include "sdm_display_pluggable.h"
#include "sdm_display_pluggable_test.h"
#include "sdm_display_virtual.h"
#include "sdm_display_virtual_factory.h"

// #include "sdm_compositor_cb_intf.h"

namespace sdm {

static const int pluggable_lock_index_ = kNumDisplays;

struct DisplayMapInfo {
  Display client_id = kNumDisplays;                 // mapped sf id for this display
  int32_t sdm_id = -1;                              // sdm id for this display
  sdm::SDMDisplayType disp_type = kDisplayTypeMax;  // sdm display type
  bool test_pattern = false;                        // display will show test pattern
  void Reset() {
    // Do not clear client id
    sdm_id = -1;
    disp_type = kDisplayTypeMax;
    test_pattern = false;
  }
};

struct VirtualDisplayData {
  uint32_t width;
  uint32_t height;
  int32_t format;
  bool in_use = false;
};

class SDMDisplayBuilder {
 public:
  explicit SDMDisplayBuilder(SDMDisplayBuilderCbIntf *cb, BufferAllocator *buffer_allocator,
                             CoreInterface *core_intf, SDMCompositorCallbacks *callbacks,
                             SDMDisplayEventHandler *event_handler)
      : cb_(cb),
        buffer_allocator_(buffer_allocator),
        core_intf_(core_intf),
        callbacks_(callbacks),
        evt_handler_(event_handler) {}
  virtual ~SDMDisplayBuilder() {}

  void Init(Locker *locker);
  void Deinit();

  DisplayError CreateVirtualDisplay(uint32_t width, uint32_t height, int32_t *format,
                                    Display *out_display_id);
  DisplayError DestroyVirtualDisplay(Display display);
  int CreatePrimaryDisplay();
  int HandleBuiltInDisplays();
  int DisconnectPluggableDisplays(DisplayMapInfo *map_info);
  void DestroyDisplay(DisplayMapInfo *map_info);
  int HandlePluggableDisplays(bool delay_hotplug);
  void HandlePluggableDisplaysAsync(const shared_ptr<Fence> &retire_fence = nullptr);
  int HandleConnectedDisplays(HWDisplaysInfo *hw_displays_info, bool delay_hotplug);
  int HandleDisconnectedDisplays(HWDisplaysInfo *hw_displays_info);
  void DestroyDisplayLocked(int display_id);
  void DestroyPluggableDisplay(DisplayMapInfo *map_info);
  void DestroyPluggableDisplayLocked(DisplayMapInfo *map_info);
  void DestroyNonPluggableDisplay(DisplayMapInfo *map_info);
  void DestroyNonPluggableDisplayLocked(DisplayMapInfo *map_info);
  std::vector<DisplayMapInfo> &GetDisplayMapInfo(int display_id);
  std::map<Display, DisplayMapInfo *> &GetActiveDisplays();
  int GetDisplayIndex(int dpy);
  Display GetActiveBuiltinDisplay();

  DisplayError CreateVirtualDisplayObj(uint32_t width, uint32_t height, int32_t *format,
                                       Display *out_display_id);
  bool IsVirtualDisplayConnected();
  void GetVirtualDisplayList();
  bool IsHWDisplayConnected(Display client_id);

  void RemoveDisconnectedPluggableDisplays();
  bool IsPluggableDisplayConnected();
  bool HasHDRSupport(SDMDisplay *sdm_display);
  bool TeardownPluggableDisplays();
  bool IsHDRDisplay(uint32_t disp_id);
  uint32_t GetVirtualDisplayCount();
  void SetLuminance(float min_lum, float max_lum);
  void SetProperties(int32_t enable_primary_reconfig_req) {
    enable_primary_reconfig_req_ = enable_primary_reconfig_req;
  }

  bool IsBuiltInDisplay(uint64_t disp_id);
  DisplayError GetDisplayHwId(uint64_t disp_id, int32_t *disp_hw_id);

 private:
  std::vector<DisplayMapInfo> map_info_primary_;    // Primary display (either builtin or pluggable)
  std::vector<DisplayMapInfo> map_info_builtin_;    // Builtin displays excluding primary
  std::vector<DisplayMapInfo> map_info_pluggable_;  // Pluggable displays excluding primary
  std::vector<DisplayMapInfo> map_info_virtual_;    // Virtual displays
  std::vector<bool> is_hdr_display_;                // info on HDR supported

  std::unordered_map<Display, VirtualDisplayData> virtual_id_map_;
  SDMVirtualDisplayFactory virtual_display_factory_;
  SDMDisplayBuilderCbIntf *cb_ = nullptr;

  BufferAllocator *buffer_allocator_ = nullptr;
  CoreInterface *core_intf_ = nullptr;
  SDMCompositorCallbacks *callbacks_ = nullptr;
  SDMDisplayEventHandler *evt_handler_ = nullptr;

  std::map<Display, DisplayMapInfo *> map_active_displays_;
  vector<HWDisplayInfo> virtual_display_list_{};

  float set_max_lum_ = -1.0;
  float set_min_lum_ = -1.0;

  int32_t disable_hotplug_bwcheck_ = 0;
  std::vector<Display> pending_hotplugs_{};
  int32_t enable_primary_reconfig_req_ = 0;

  enum HotPlugEvent {
    kHotPlugNone,
    kHotPlugEvent,
  };

  static Locker hdr_locker_[kNumDisplays];

  HotPlugEvent pending_hotplug_event_ = kHotPlugNone;
  Locker *locker_ = nullptr;
};

}  // namespace sdm

#endif  // __SDM_DISPLAY_BUILDER_H__
