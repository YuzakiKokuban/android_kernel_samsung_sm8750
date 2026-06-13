/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the
 * following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SDM_DISPLAY_PLUGGABLE_TEST_H__
#define __SDM_DISPLAY_PLUGGABLE_TEST_H__

#include <bitset>
#include <core/buffer_allocator.h>

#include "sdm_display.h"

namespace sdm {

class SDMDisplayPluggableTest : public SDMDisplay {
public:
 static DisplayError Create(CoreInterface *core_intf, BufferAllocator *buffer_allocator,
                            SDMCompositorCallbacks *callbacks,
                            SDMDisplayEventHandler *event_handler, Display id, int32_t sdm_id,
                            uint32_t panel_bpp, uint32_t pattern_type, SDMDisplay **sdm_display);
 static void Destroy(SDMDisplay *sdm_display);
 virtual DisplayError Validate(uint32_t *out_num_types, uint32_t *out_num_requests);
 virtual DisplayError Present(shared_ptr<Fence> *out_retire_fence);
 virtual DisplayError Perform(uint32_t operation, ...);

protected:
  BufferInfo buffer_info_ = {};
  uint32_t panel_bpp_ = 0;
  uint32_t pattern_type_ = 0;

  enum ColorPatternType {
    kPatternNone = 0,
    kPatternColorRamp,
    kPatternBWVertical,
    kPatternColorSquare,
  };

  enum DisplayBpp {
    kDisplayBpp18 = 18,
    kDisplayBpp24 = 24,
    kDisplayBpp30 = 30,
  };

  enum ColorRamp {
    kColorRedRamp = 0,
    kColorGreenRamp = 1,
    kColorBlueRamp = 2,
    kColorWhiteRamp = 3,
  };

  enum Colors {
    kColorBlack = 0,
    kColorWhite = 1,
  };

private:
 SDMDisplayPluggableTest(CoreInterface *core_intf, BufferAllocator *buffer_allocator,
                         SDMCompositorCallbacks *callbacks, SDMDisplayEventHandler *event_handler,
                         Display id, int32_t sdm_id, uint32_t panel_bpp, uint32_t pattern_type);
 DisplayError Init();
 DisplayError Deinit();
 void DumpInputBuffer();
 void CalcCRC(uint32_t color_value, std::bitset<16> *crc_data);
 DisplayError FillBuffer();
 DisplayError GetStride(LayerBufferFormat format, uint32_t width, uint32_t *stride);
 void PixelCopy(uint32_t red, uint32_t green, uint32_t blue, uint32_t alpha, uint8_t **buffer);
 void GenerateColorRamp(uint8_t *buffer);
 void GenerateBWVertical(uint8_t *buffer);
 void GenerateColorSquare(uint8_t *buffer);
 DisplayError InitLayer(Layer *layer);
 DisplayError DeinitLayer(Layer *layer);
 DisplayError CreateLayerStack();
 DisplayError DestroyLayerStack();
 DisplayError PostCommit(shared_ptr<Fence> *out_retire_fence);

 static const uint32_t kTestLayerCnt = 1;
};

} // namespace sdm

#endif // __SDM_DISPLAY_PLUGGABLE_TEST_H__
