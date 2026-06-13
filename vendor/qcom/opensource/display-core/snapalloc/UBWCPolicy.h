// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __UBWC_POLICY_H__
#define __UBWC_POLICY_H__

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "GraphicsConstraintProvider.h"
#include "SnapConstraintDefs.h"
#include "SnapConstraintParser.h"
#include "SnapMemAllocDefs.h"

namespace snapalloc {

class UBWCPolicy {
 public:
  UBWCPolicy(UBWCPolicy &other) = delete;
  void operator=(const UBWCPolicy &) = delete;
  static UBWCPolicy *GetInstance(
      std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map = {});

  /* isUbwcAlloc() checks the BufferDescriptor to determine if the
   * request is a UBWC allocation, based on the format and usage.
   * @param: BufferDescriptor containing requested format, usage, width, and height
   * @return: bool - true if UBWCPolicy will be queried for allocation
   */
  bool IsUBWCAlloc(BufferDescriptor desc);

  /* getUbwcAlloc() checks the BufferDescriptor to determine if the
   * request requires UBWC and what capabilities (e.g., version) the HW supports.
   * @param: BufferDescriptor containing requested format, usage, width, and height
   * @param: UBWCCapabilities containing UBWC support info
   * @return: AllocData to be used by memory allocation backend
   */
  Error GetUBWCAlloc(BufferDescriptor desc, UBWCCapabilities caps, AllocData *out_ad,
                     vendor_qti_hardware_display_common_BufferLayout *out_layout);

  void Init(std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map);
  vendor_qti_hardware_display_common_Compression GetUBWCScheme(
      vendor_qti_hardware_display_common_PixelFormat format,
      vendor_qti_hardware_display_common_BufferUsage usage);

 private:
  ~UBWCPolicy();
  UBWCPolicy(
      std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map = {});
  static std::mutex ubwc_policy_mutex_;

  static UBWCPolicy *instance_;
  SnapConstraintParser *constraint_parser_;
  GraphicsConstraintProvider *graphics_provider_;
  Debug *debug_;
  int GetConstraints(BufferDescriptor desc, BufferConstraints *out);
  std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map_;
  std::map<vendor_qti_hardware_display_common_PixelFormat, BufferConstraints> constraint_set_map_;
  int OffTargetAlloc(BufferDescriptor desc, AllocData *out_ad,
                     vendor_qti_hardware_display_common_BufferLayout *out_layout);
  uint64_t GetMetaPlaneSize(uint64_t width, uint64_t height, uint32_t block_width,
                            uint32_t block_height, uint64_t stride_align, uint64_t scanline_align,
                            uint64_t size_align);
  int GetBatchSize(vendor_qti_hardware_display_common_PixelFormatModifier modifier);
};
}  // namespace snapalloc

#endif  // __UBWC_POLICY_H__