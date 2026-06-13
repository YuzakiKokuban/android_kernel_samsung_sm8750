// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAP_CONSTRAINT_MANAGER_H__
#define __SNAP_CONSTRAINT_MANAGER_H__

#include <memory>
#include <vector>

#include "CPUConstraintProvider.h"
#include "CameraConstraintProvider.h"
#include "DefaultConstraintProvider.h"
#include "DisplayConstraintProvider.h"
#include "GraphicsConstraintProvider.h"
#include "SnapConstraintProvider.h"
#include "SnapMemAllocDefs.h"
#include "SnapTypes.h"
#include "SnapUtils.h"
#include "UBWCPolicy.h"
#include "VideoConstraintProvider.h"

namespace snapalloc {

class SnapConstraintManager {
 public:
  SnapConstraintManager(SnapConstraintManager &other) = delete;
  void operator=(const SnapConstraintManager &) = delete;
  static SnapConstraintManager *GetInstance();

  /* getAllocationData() is the entry-point for SnapAlloc into SnapConstraintManager.
   * It queries constraint providers' capabilities and constraints through private functions
   * and generates the allocation data to be used by the memory backend.
   *
   * @param: BufferDescriptor containing requested format, usage, width, and height
   * @param: AllocData to be used by memory allocation backend
   * @param: vendor_qti_hardware_display_common_BufferLayout to be used for plane layout metadata
   */
  Error GetAllocationData(BufferDescriptor desc, AllocData *out_ad,
                          vendor_qti_hardware_display_common_BufferLayout *out_layout,
                          BufferDescriptor *out_desc, int *out_priv_flags);

  /* Init() handles one-time initialization, such as constraint providers.
   */
  void Init();

  // TODO: move this to FormatUtils class
  Error ConvertAlignedWidthFromBytesToPixels(vendor_qti_hardware_display_common_PixelFormat format,
                                             int width_in_bytes, int *width_in_pixels);

 private:
  ~SnapConstraintManager();
  SnapConstraintManager(){};
  static std::mutex constraint_mgr_mutex_;
  Debug *debug_ = nullptr;

  static SnapConstraintManager *instance_;

  /* GetCapabilities() queries the constraint providers
   * and builds a map of providers and their capabilities.
   * This map is used by ResolveConstraints to determine which providers to query for constraints.
   * @param: BufferDescriptor
   * @return: Map of all constraint providers and their capabilities
   */
  std::map<SnapConstraintProvider *, CapabilitySet> GetCapabilities(BufferDescriptor desc);

  /* FetchAndMergeConstraints() queries the required constraint providers for their constraints,
   * then merges the requirements to create a final allocation.
   * @param: BufferDescriptor
   * @param: Map of ConstraintProviders and capabilities to determine which are enabled
   * @param: BufferConstraints - final results after merging
   * @return: Error
   */
  Error FetchAndMergeConstraints(BufferDescriptor desc,
                                 std::map<SnapConstraintProvider *, CapabilitySet> const &providers,
                                 vendor_qti_hardware_display_common_BufferLayout *out);

  /* MergeConstraints() merges the alignments from multiple constraint sets
   * @param: constraint_sets - constraint sets to merge
   * @param: merged_constraints - output of merging
   * @return: Error
   */
  Error MergeConstraints(std::vector<BufferConstraints> constraint_sets,
                         BufferConstraints *merged_constraints);

  uint8_t GetBitsPerPixel(vendor_qti_hardware_display_common_PixelFormat format);

  uint32_t GetDataAlignment(vendor_qti_hardware_display_common_PixelFormat format,
                            vendor_qti_hardware_display_common_BufferUsage usage,
                            vendor_qti_hardware_display_common_PixelFormatModifier modifier);

  void GetImplDefinedFormat(vendor_qti_hardware_display_common_PixelFormat format,
                            vendor_qti_hardware_display_common_BufferUsage usage,
                            vendor_qti_hardware_display_common_PixelFormat *out_format,
                            vendor_qti_hardware_display_common_PixelFormatModifier *out_modifier);

  /* AlignmentToAlignedConstraints() calculates aligned stride and scanlines
   * @param: alignment - constraint set containing alignment requirements
   * @param: aligned - aligned stride and scanlines
   * @return: Error
   */
  Error AlignmentToAlignedConstraints(BufferDescriptor desc, BufferConstraints alignment,
                                      BufferConstraints *aligned);

  Error ConstraintsToBufferLayout(
      BufferDescriptor desc, BufferConstraints *constraints,
      vendor_qti_hardware_display_common_BufferLayout *layout);
  Error SetSnapPrivateFlags(vendor_qti_hardware_display_common_PixelFormat format,
                            vendor_qti_hardware_display_common_BufferUsage usage, bool ubwc_enabled,
                            int *snap_priv_flags);
  bool UseUncached(vendor_qti_hardware_display_common_PixelFormat format,
                   vendor_qti_hardware_display_common_BufferUsage usage, bool ubwc_enabled);
  bool CanAllocateZSLForSecureCamera();
  bool ValidateDescriptor(const BufferDescriptor &snap_desc);
  UBWCCapabilities ubwc_caps_;

  // Vector of device constraint providers
  std::vector<SnapConstraintProvider *> providers_;

  // Default constraint provider, to be used when no other constraints provided
  DefaultConstraintProvider *default_provider_;

  std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map_;
  UBWCPolicy *ubwc_policy_;
  std::vector<vendor_qti_hardware_display_common_PixelFormat>
      formats_with_w_h_constraints{
          vendor_qti_hardware_display_common_PixelFormat::YV12,
          vendor_qti_hardware_display_common_PixelFormat::CbYCrY_422_I,
          vendor_qti_hardware_display_common_PixelFormat::YCBCR_422_SP,
          vendor_qti_hardware_display_common_PixelFormat::YCrCb_422_SP,
          vendor_qti_hardware_display_common_PixelFormat::YCBCR_422_I,
          vendor_qti_hardware_display_common_PixelFormat::YCrCb_422_I};
};

}  // namespace snapalloc

#endif  // __SNAP_CONSTRAINT_MANAGER_H__
