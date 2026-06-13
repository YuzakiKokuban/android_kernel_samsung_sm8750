// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAPALLOCCORE_H__
#define __SNAPALLOCCORE_H__

#include "SnapConstraintManager.h"
#include "SnapHandleInternal.h"
#include "SnapMemAllocator.h"
#include "SnapMetadataManager.h"
#include "SnapMetadataManagerDefs.h"
#include "SnapTestAllocator.h"
#include "SnapTypes.h"

#include <map>
#include <vector>

namespace snapalloc {

class SnapAllocCore {
 public:
  SnapAllocCore(SnapAllocCore &other) = delete;
  void operator=(const SnapAllocCore &) = delete;
  static SnapAllocCore *GetInstance();
  Error Allocate(BufferDescriptor desc, int count, std::vector<SnapHandleInternal *> *handles,
                 bool test_alloc = false);
  Error Retain(SnapHandle *hnd);   // Import.
  Error Release(SnapHandle *hnd);  // Free.
  Error Lock(SnapHandle *hnd, vendor_qti_hardware_display_common_BufferUsage usage,
             vendor_qti_hardware_display_common_Rect access_region, uint64_t *base_addr);
  Error Unlock(SnapHandle *hnd);
  Error ValidateBufferSize(SnapHandle *hnd, BufferDescriptor desc);
  Error FlushLockedBuffer(SnapHandle *hnd);
  Error RereadLockedBuffer(SnapHandle *hnd);
  Error IsSupported(BufferDescriptor desc, bool *is_supported);
  Error GetMetadata(SnapHandle *hnd, vendor_qti_hardware_display_common_MetadataType type,
                    void *out);
  Error SetMetadata(SnapHandle *hnd, vendor_qti_hardware_display_common_MetadataType type,
                    void *in);
  Error GetFromBufferDescriptor(BufferDescriptor desc,
                                vendor_qti_hardware_display_common_MetadataType type, void *out);
  Error DumpBuffer(SnapHandle *hnd);
  Error DumpBuffers();
  Error GetMetadataState(SnapHandle *hnd, vendor_qti_hardware_display_common_MetadataType type,
                          bool *out);
  Error RetainViewBuffer(SnapHandle *meta_hnd, uint32_t view, SnapHandle **out_view_handle);
  void RegisterHandleLocked(SnapHandle *public_hnd, SnapHandleInternal *snap_hnd);

 private:
  ~SnapAllocCore();
  SnapAllocCore();
  Error MapBuffer(SnapHandleInternal *hnd);
  SnapHandleInternal *GetBufferFromHandleLocked(SnapHandle *hnd);
  Error FreeBuffer(SnapHandleInternal *buf);
  Error ImportHandleLocked(SnapHandle *hnd);
  int GetPrivateFlags(vendor_qti_hardware_display_common_BufferUsage usage);
  Error AllocateBuffer(AllocData *ad, AllocData *m_data, unsigned custom_content_md_size,
                       BufferDescriptor *desc, BufferDescriptor *out_desc, bool test_alloc);

  static std::mutex snapalloc_core_mutex_;
  static SnapAllocCore *instance_;
  static bool init;
  SnapConstraintManager *constraint_mgr_ = nullptr;
  SnapMetadataManager *metadata_mgr_ = nullptr;
  SnapMemAllocator *mem_alloc_intf_ = nullptr;
  std::mutex buffer_lock_;
  std::atomic<uint64_t> next_id_;
  std::unordered_map<SnapHandle *, SnapHandleInternal *> handles_map_ = {};
};

}  // namespace snapalloc

#endif  // __SNAPALLOCCORE_H__
