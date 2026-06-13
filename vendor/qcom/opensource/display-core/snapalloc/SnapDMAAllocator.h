// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAP_DMA_ALLOCATOR_H__
#define __SNAP_DMA_ALLOCATOR_H__

#include <BufferAllocator/BufferAllocator.h>
#include <sys/ioctl.h>
#include <vmmem.h>

#include <bitset>
#include <cstdint>
#include <mutex>
#include <string>

#include "ISnapMemAllocBackend.h"
#include "SnapMemAllocDefs.h"
#include "SnapTypes.h"
#include "Debug.h"

#ifdef TARGET_USES_SMMU_PROXY
#include <linux/qti-smmu-proxy.h>
#endif

#define FD_INIT -1
#define MEMBUF_CLIENT_LIB_NAME "libmemutils.so"
#define CREATE_MEMBUF_INTERFACE_NAME "CreateMemBufInterface"
#define DESTROY_MEMBUF_INTERFACE_NAME "DestroyMemBufInterface"

typedef int (*CreateMemBufInterface)(MemBuf **mem_buf_hnd);
typedef int (*DestroyMemBufInterface)();

namespace snapalloc {

class SnapDMAAllocator : public ISnapMemAllocBackend {
 public:
  static SnapDMAAllocator *GetInstance();
  Error AllocBuffer(AllocData *ad);
  Error FreeBuffer(void *base, unsigned int size, int fd, std::string /* shm_path */);
  Error MapBuffer(void **base, unsigned int size, int fd);
  Error CleanBuffer(void *base, unsigned int size, int op, int fd);
  int ImportBuffer(int fd);
  Error SecureMemPerms(AllocData *ad);
  void GetHeapInfo(vendor_qti_hardware_display_common_BufferUsage usage, bool sensor_flag,
                   std::string *dma_heap_name, std::vector<std::string> *dma_vm_names,
                   unsigned int *alloc_type, unsigned int *flags, unsigned int *alloc_size);
  Error SetBufferPermission(
      int fd, vendor_qti_hardware_display_common_BufferPermission *buffer_perm, int64_t *mem_hdl);

 private:
  void GetVMPermission(vendor_qti_hardware_display_common_BufferPermission buf_perm,
                       std::bitset<kVmPermissionMax> *vm_perm);
  Error UnmapBuffer(void *base, unsigned int size);
  ~SnapDMAAllocator() { Deinit(); }
  SnapDMAAllocator();
  void GetCSFVersion();
  bool CSFEnabled();
  void InitMemUtils();
  void DeinitMemUtils();
  void Deinit();

  Debug *debug_ = nullptr;
  int dma_dev_fd_ = FD_INIT;
  BufferAllocator buffer_allocator_;
  static std::mutex snap_dma_alloc_mutex_;
  static SnapDMAAllocator *instance_;
  std::string smmu_proxy_node_ = "/dev/qti-smmu-proxy";
#ifdef TARGET_USES_SMMU_PROXY
  struct csf_version csf_version_;
#endif
  bool csf_initialized_ = false;
  MemBuf *mem_buf_ = nullptr;
  void *mem_utils_lib_ = {};
  CreateMemBufInterface CreateMemBuf_ = nullptr;
  DestroyMemBufInterface DestroyMemBuf_ = nullptr;
  bool movable_heap_system_available_ = false;
  bool movable_heap_ubwcp_available_ = false;
};

}  // namespace snapalloc

#endif  // __SNAP_DMA_ALLOCATOR_H__