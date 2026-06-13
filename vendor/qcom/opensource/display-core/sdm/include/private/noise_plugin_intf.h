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

#ifndef __NOISE_PLUGIN_INTF_H__
#define __NOISE_PLUGIN_INTF_H__

#include <private/generic_payload.h>
#include <private/generic_intf.h>
#include <memory>
#include <vector>
#include <array>

namespace sdm {

enum NoisePlugInParams {
  kNoisePlugInMixerStages,                 // Max no.of blending stages supported by HW
  kNoisePlugInDisable,                     // Disable the noise layer
  kNoisePlugInParamsMax = 0xff,
  kCustomPropertyStart = 0x100,            // custom property start
  kCustomPropertyEnd = 0x1ff,              // custom property end
  kNoisePlugInDebugPropertyStart = 0x200,  // debug properties start
  kNoisePlugInDebugPropertyEnd = 0x2ff,    // debug properties end
};

enum NoisePlugInLayerType {
  kDimLayer,       // Dim layer
  kFodLayer,       // FOD layer
  kGraphicsLayer,  // Graphics layer
  kGameLayer,      // Game layer
  kMaskLayer,      // Mask layer
  kVideoLayer,     // Video layer
  kLayerNameMax = 0xfff
};

enum NoisePlugInOps {
  kOpsRunNoisePlugIn,  // Calculate Attenuation factor and zpos for noise, attn layers
  kNoisePlugInOpsMax = 0xff
};

struct NoisePlugInInputLayers {
  NoisePlugInLayerType layer_type;
  uint32_t zorder = ~0x0;
};

struct NoisePlugInInputParams {
  std::vector<NoisePlugInInputLayers> layers;  // Display layers as per their Z-order
};

struct NoisePlugInOutputParams {
  bool enabled = false;          // Enable/Disable the Noise Layer
  uint32_t attn = 0;             // Output Attenuation factor
  std::array<uint32_t, 2> zpos;  // Z-order position for Noiselayer and Attenuation layer
  uint32_t strength = ~0x0;      // Output Noise strength
  uint32_t alpha_noise = ~0x0;   // Output Noise Alpha (transparency coefficient)
  bool temporal_en = true;       // Enable temporal rotation of Noise matrix
};

using NoisePlugInIntf = GenericIntf<NoisePlugInParams, NoisePlugInOps, GenericPayload>;

class NoisePlugInFactoryIntf {
 public:
  virtual ~NoisePlugInFactoryIntf() {}
  virtual std::unique_ptr<NoisePlugInIntf> CreateNoisePlugInIntf(uint32_t major_ver,
                                                                 uint32_t minor_ver) = 0;
};

extern "C" NoisePlugInFactoryIntf *GetNoisePlugInFactoryIntf();

}  // namespace sdm

#endif  // __NOISE_PLUGIN_INTF_H__
