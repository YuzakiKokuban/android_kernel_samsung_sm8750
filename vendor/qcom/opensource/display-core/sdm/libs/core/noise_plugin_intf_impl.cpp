/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
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

#include "noise_plugin_intf_impl.h"
#include <private/noise_plugin_dbg.h>

#define __CLASS__ "NoisePlugInIntfImpl"

namespace sdm {

using std::lock_guard;
using std::mutex;

static NoisePlugInFactoryIntfImpl factory_instance;

NoisePlugInFactoryIntf *GetNoisePlugInFactoryIntf() {
  return &factory_instance;
}

std::unique_ptr<NoisePlugInIntf> NoisePlugInFactoryIntfImpl::CreateNoisePlugInIntf(
    uint32_t major_ver, uint32_t minor_ver) {
  lock_guard<mutex> lock(lock_);
  std::unique_ptr<NoisePlugInIntf> intf = nullptr;
  if (major_ver == NOISE_PLUGIN_REVISION_MAJOR && minor_ver == NOISE_PLUGIN_REVISION_MINOR) {
    intf = std::unique_ptr<NoisePlugInIntf>(new NoisePlugInIntfImpl());
    if (!intf) {
      DLOGW("Failed to allocate memory for NoisePlugInIntf\n");
    }
  } else {
    DLOGW("Unsupported NoisePlugIn Version\n");
  }
  return intf;
}

DynLib::~DynLib() {
  Close();
}

bool DynLib::Open(const char *lib_name) {
  Close();
  lib_ = ::dlopen(lib_name, RTLD_NOW);

  return (lib_ != NULL);
}

bool DynLib::Sym(const char *func_name, void **func_ptr) {
  if (lib_) {
    *func_ptr = ::dlsym(lib_, func_name);
  } else {
    *func_ptr = NULL;
  }

  return (*func_ptr != NULL);
}

void DynLib::Close() {
  if (lib_) {
    ::dlclose(lib_);
    lib_ = NULL;
  }
}

NoisePlugInIntfImpl::NoisePlugInIntfImpl() {
  set_param_func_[kNoisePlugInMixerStages] = &NoisePlugInIntfImpl::SetMixerStages;
  set_param_func_[kNoisePlugInDisable] = &NoisePlugInIntfImpl::SetDisable;
  if ((kNoisePlugInDebugOverride >= kNoisePlugInDebugPropertyStart) &&
      (kNoisePlugInDebugOverride < kNoisePlugInDebugPropertyEnd)) {
    set_param_func_[static_cast<NoisePlugInParams>(kNoisePlugInDebugOverride)] =
                   &NoisePlugInIntfImpl::SetDebugOverride;
  }
  if ((kNoisePlugInDebugAttn >= kNoisePlugInDebugPropertyStart) &&
      (kNoisePlugInDebugAttn < kNoisePlugInDebugPropertyEnd)) {
    set_param_func_[static_cast<NoisePlugInParams>(kNoisePlugInDebugAttn)] =
                   &NoisePlugInIntfImpl::SetDebugAttn;
  }
  if ((kNoisePlugInDebugNoiseZpos >= kNoisePlugInDebugPropertyStart) &&
      (kNoisePlugInDebugNoiseZpos < kNoisePlugInDebugPropertyEnd)) {
    set_param_func_[static_cast<NoisePlugInParams>(kNoisePlugInDebugNoiseZpos)] =
                   &NoisePlugInIntfImpl::SetDebugNoiseZpos;
  }
  ops_func_[kOpsRunNoisePlugIn] = &NoisePlugInIntfImpl::RunNoisePlugIn;
}

int NoisePlugInIntfImpl::Init() {
  lock_guard<mutex> lock(lock_);

  enable_ = true;
  attn_ = NOISE_ATTN_DEFAULT;
  noise_zpos_override_ = -1;

  DynLib algolib;
  if (algolib.Open("libsdmextension.so")) {
    if (!algolib.Sym("GetNoiseAlgoFactoryIntf",
                    reinterpret_cast<void **>(&GetNoiseAlgoFactoryIntfFunc_))) {
      DLOGE("Unable to load GetNoiseAlgoFactoryIntf, error = %s", algolib.Error());
      return -ENOTSUP;
    }
  } else {
    DLOGE("Unable to load = %s, error = %s", "libsdmextension.so", algolib.Error());
    return -ENOTSUP;
  }

  noise_algo_factory_ = GetNoiseAlgoFactoryIntfFunc_();
  if (!noise_algo_factory_) {
    DLOGW("Failed to create NoiseAlgoFactoryIntf");
    return -ENOTSUP;
  }

  noise_algo_ =
      noise_algo_factory_->CreateNoiseAlgoIntf(NOISE_ALGO_VERSION_MAJOR, NOISE_ALGO_VERSION_MINOR);
  if (!noise_algo_) {
    DLOGE("Failed to create NoiseAlgoIntf");
    return -ENOTSUP;
  }
  if (noise_algo_->Init()) {
    DLOGE("Failed to initialize NoiseAlgoIntf");
    noise_algo_ = nullptr;
    return -ENOTSUP;
  }

  init_done_ = true;
  return 0;
}

int NoisePlugInIntfImpl::Deinit() {
  lock_guard<mutex> lock(lock_);

  if (!init_done_) {
    DLOGI("interface not initialized\n");
    return 0;
  }

  noise_algo_->Deinit();
  noise_algo_ = nullptr;

  init_done_ = false;
  return 0;
}

int NoisePlugInIntfImpl::SetParameter(NoisePlugInParams param, const GenericPayload &in) {
  lock_guard<mutex> lock(lock_);

  if (!init_done_) {
    DLOGE("interface not initialized\n");
    return -EINVAL;
  }

  auto it = set_param_func_.find(param);
  if (it == set_param_func_.end()) {
    DLOGE("set param %d is not supported", param);
    return -ENOTSUP;
  }
  SetParam func = it->second;
  if (!func) {
    DLOGE("invalid set param function for parameter %d", param);
    return -EINVAL;
  }

  return (this->*func)(in);
}

int NoisePlugInIntfImpl::GetParameter(NoisePlugInParams param, GenericPayload *out) {
  lock_guard<mutex> lock(lock_);
  (void)(param);
  (void)(out);
  return -ENOTSUP;
}

int NoisePlugInIntfImpl::ProcessOps(NoisePlugInOps op, const GenericPayload &in,
                                    GenericPayload *out) {
  lock_guard<mutex> lock(lock_);

  if (!init_done_) {
    DLOGE("interface not initialized\n");
    return -EINVAL;
  }

  if (!out) {
    DLOGE("invalid arguments for noise process op %d\n", op);
    return -EINVAL;
  }

  auto it = ops_func_.find(op);
  if (it == ops_func_.end()) {
    DLOGE("process op function not supported %d", op);
    return -ENOTSUP;
  }
  Ops func = it->second;
  if (!func) {
    DLOGE("invalid process op function %d", op);
    return -EINVAL;
  }

  return (this->*func)(in, out);
}

int NoisePlugInIntfImpl::SetMixerStages(const GenericPayload &in) {
  int32_t err = 0;
  uint32_t payload_size = 0;
  int32_t *val = nullptr;

  err = in.GetPayload<int32_t>(val, &payload_size);
  if (err || payload_size != 1 || !val) {
    DLOGE("GetPayload for input failed %d\n", err);
    return -EINVAL;
  }

  // update mixer blend stages.
  if (*val > 0) {
    blend_stages_max_ = *val;
  } else {
    DLOGE("invalid mixer stages\n");
    return -EINVAL;
  }

  return 0;
}

int NoisePlugInIntfImpl::SetDisable(const GenericPayload &in) {
  int32_t err = 0;
  uint32_t payload_size = 0;
  int32_t *val = nullptr;

  err = in.GetPayload<int32_t>(val, &payload_size);
  if (err || payload_size != 1 || !val) {
    DLOGE("GetPayload for input failed %d\n", err);
    return -EINVAL;
  }

  // update noise enable status.
  enable_ = !((*val) == 1);

  return 0;
}

int NoisePlugInIntfImpl::SetDebugOverride(const GenericPayload &in) {
  int32_t err = 0;
  uint32_t payload_size = 0;
  int32_t *val = nullptr;

  err = in.GetPayload<int32_t>(val, &payload_size);
  if (err || payload_size != 1 || !val) {
    DLOGE("GetPayload for input failed %d\n", err);
    return -EINVAL;
  }

  // Reset the override values at each override.
  // Each override paramter will be set seperately.
  override_ = ((*val) == 1);
  attn_ = NOISE_ATTN_DEFAULT;
  noise_zpos_override_ = -1;

  return 0;
}

int NoisePlugInIntfImpl::SetDebugAttn(const GenericPayload &in) {
  int32_t err = 0;
  uint32_t payload_size = 0;
  int32_t *val = nullptr;

  err = in.GetPayload<int32_t>(val, &payload_size);
  if (err || payload_size != 1 || !val) {
    DLOGE("GetPayload for input failed %d\n", err);
    return -EINVAL;
  }

  // update attenuation override.
  if ((*val > NOISE_ATTN_MIN) && (*val <= NOISE_ATTN_MAX) && override_) {
    attn_ = *val;
  } else {
    DLOGE("invalid attenuation override\n");
    return -EINVAL;
  }

  return 0;
}

int NoisePlugInIntfImpl::SetDebugNoiseZpos(const GenericPayload &in) {
  int32_t err = 0;
  uint32_t payload_size = 0;
  int32_t *val = nullptr;

  err = in.GetPayload<int32_t>(val, &payload_size);
  if (err || payload_size != 1 || !val) {
    DLOGE("GetPayload for input failed %d\n", err);
    return -EINVAL;
  }

  // update z-order override (should be greater than zero
  if ((*val > 0) && (*val < INT32_MAX) && override_) {
    noise_zpos_override_ = *val;
  } else {
    DLOGE("invalid z position override\n");
    return -EINVAL;
  }

  return 0;
}

int NoisePlugInIntfImpl::RunNoisePlugIn(const GenericPayload &in, GenericPayload *out) {
  int32_t err = -1, fod_zpos = -1;
  uint32_t payload_size = 0;
  NoisePlugInInputParams *input_params = nullptr;
  NoisePlugInOutputParams *output_params = nullptr;

  // get input payload
  err = in.GetPayload<NoisePlugInInputParams>(input_params, &payload_size);
  if (err || (payload_size != 1) || !input_params) {
    DLOGE("GetPayload for input failed %d\n", err);
    return -EINVAL;;
  }

  // get output payload
  err = out->GetPayload<NoisePlugInOutputParams>(output_params, &payload_size);
  if (err || (payload_size != 1) || !output_params) {
    DLOGE("GetPayload for output failed %d\n", err);
    return -EINVAL;
  }

  // return if noise is not enabled
  if (!enable_) {
    output_params->enabled = false;
    return 0;
  }

  // check if zpos override is enabled
  if (override_ && (noise_zpos_override_ > 0)) {
    if ((noise_zpos_override_ >= input_params->layers.size()) ||
        (input_params->layers[noise_zpos_override_].layer_type == kMaskLayer)) {
      DLOGW("invalid z order override %d, layer stack size %d\n", noise_zpos_override_,
            input_params->layers.size());
      return -EINVAL;
    }
    output_params->enabled = true;
    output_params->zpos[0] = noise_zpos_override_;
    output_params->zpos[1] = noise_zpos_override_ + 1;
    output_params->attn = attn_;
  } else {
    // calculate z position as follows:
    // FOD usecase : top most FOD layer when multiple FOD layers are present.
    for (uint32_t i = 0; i < input_params->layers.size(); i++) {
      if (input_params->layers[i].layer_type == kFodLayer) {
        if ((int32_t)input_params->layers[i].zorder > fod_zpos) {
          fod_zpos = (int32_t)input_params->layers[i].zorder;
        }
      }
    }

    // update output parameters
    if (fod_zpos > 0) {
      // FOD layer is present in layer stack
      output_params->enabled = true;
      output_params->zpos[0] = fod_zpos;
      output_params->zpos[1] = fod_zpos + 1;
      output_params->attn = attn_;
    } else {
      // FOD usecase is not present
      output_params->enabled = false;
    }
  }

  if (output_params->enabled) {
    NoiseAlgoInputParams *noise_algo_in = nullptr;
    NoiseAlgoOutputParams *noise_algo_out = nullptr;
    GenericPayload in_payload, out_payload;

    err = in_payload.CreatePayload<NoiseAlgoInputParams>(noise_algo_in);
    if (err) {
      DLOGE("failed to create input payload. Error:%d", err);
      return -ENOMEM;
    }

    noise_algo_in->attn_factor = output_params->attn;
    noise_algo_in->zpos_noise_layer = output_params->zpos[0];
    noise_algo_in->zpos_attn_layer = output_params->zpos[1];

    err = out_payload.CreatePayload<NoiseAlgoOutputParams>(noise_algo_out);
    if (err) {
      DLOGE("failed to create output payload. Error:%d", err);
            in_payload.DeletePayload();
      return -ENOMEM;
    }

    err = noise_algo_->ProcessOps(sdm::kOpsRunNoiseAlgo, in_payload, &out_payload);
    if (!err) {
      output_params->strength = noise_algo_out->strength;
      output_params->alpha_noise = noise_algo_out->alpha_noise;
      output_params->temporal_en = noise_algo_out->temporal_en;
    } else {
      DLOGE("failed to run Noise Algo ProcessOps. Error:%d", err);
      err = -ENOTSUP;
    }

    in_payload.DeletePayload();
    out_payload.DeletePayload();
  }

  return err;
}

}  // namespace sdm
