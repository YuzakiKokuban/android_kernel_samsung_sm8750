/*
* Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
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

#ifndef __HW_TV_DRM_H__
#define __HW_TV_DRM_H__

#include <map>
#include <vector>
#include <chrono>

#include "hw_device_drm.h"

namespace sdm {

using std::vector;

typedef std::chrono::steady_clock SteadyClock;

class HWTVDRM : public HWDeviceDRM {
 public:
  explicit HWTVDRM(int32_t display_id, BufferAllocator *buffer_allocator,
                   HWInfoInterface *hw_info_intf);

 protected:
  virtual DisplayError SetDisplayAttributes(uint32_t index);
  virtual DisplayError GetConfigIndex(char *mode, uint32_t *index);
  virtual DisplayError PowerOff(bool teardown, SyncPoints *sync_points);
  virtual DisplayError Doze(const HWQosData &qos_data, SyncPoints *sync_points);
  virtual DisplayError DozeSuspend(const HWQosData &qos_data, SyncPoints *sync_points);
  virtual DisplayError Standby(SyncPoints *sync_points);
  virtual DisplayError Commit(HWLayersInfo *hw_layers_info);
  virtual void PopulateHWPanelInfo();
  virtual DisplayError GetDefaultConfig(uint32_t *default_config);
  virtual DisplayError PowerOn(const HWQosData &qos_data, SyncPoints *sync_points);
  virtual DisplayError Deinit();
  virtual DisplayError Flush(HWLayersInfo *hw_layers_info);

 private:
  DisplayError UpdateHDRMetaData(HWLayersInfo *hw_layers_info);
  void DumpHDRMetaData(HWHDRLayerInfo::HDROperation operation);
  void InitMaxHDRMetaData();

  const float kDefaultMinLuminance = 0.02f;
  const float kDefaultMaxLuminance = 500.0f;
  const float kMinPeakLuminance = 300.0f;
  const float kMaxPeakLuminance = 1000.0f;
  drm_msm_ext_hdr_metadata hdr_metadata_ = {};
  std::chrono::time_point<SteadyClock> hdr_reset_start_;
  std::chrono::time_point<SteadyClock> hdr_reset_end_;
  bool reset_hdr_flag_ = false;
  bool in_multiset_ = false;
};

}  // namespace sdm

#endif  // __HW_TV_DRM_H__

