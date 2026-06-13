// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SnapDMAAllocator.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "SnapTypes.h"
#include "SnapUtils.h"

namespace snapalloc {

SnapDMAAllocator *SnapDMAAllocator::instance_{nullptr};
std::mutex SnapDMAAllocator::snap_dma_alloc_mutex_;

SnapDMAAllocator::SnapDMAAllocator() {
  debug_ = Debug::GetInstance();
}

SnapDMAAllocator *SnapDMAAllocator::GetInstance() {
  std::lock_guard<std::mutex> lock(snap_dma_alloc_mutex_);

  if (instance_ == nullptr) {
    instance_ = new SnapDMAAllocator();
  }
  return instance_;
}

Error SnapDMAAllocator::AllocBuffer(AllocData *ad) {
  unsigned int flags = ad->flags;

  dma_dev_fd_ = buffer_allocator_.Alloc(ad->heap_name, ad->size, flags, ad->align);

  if (dma_dev_fd_ < 0) {
    DLOGE("libdma alloc failed fd %d size %d align %d heap_name %s flags %x", dma_dev_fd_, ad->size,
          ad->align, ad->heap_name.c_str(), flags);
    return Error::BAD_VALUE;
  }

  ad->fd = dma_dev_fd_;
  DLOGD_IF(enable_logs, "libdma: Allocated buffer size:%u fd:%d", ad->size, ad->fd);
  DLOGD_IF(enable_logs, "%s fd size %d", __FUNCTION__,
           static_cast<unsigned int>(lseek(dma_dev_fd_, 0, SEEK_END)));

  return Error::NONE;
}

Error SnapDMAAllocator::FreeBuffer(void *base, unsigned int size, int fd,
                                   std::string /* shm_path [[maybe_unused]] */) {
  auto err = Error::NONE;

  DLOGD_IF(enable_logs, "libdma: Freeing buffer base:%p size:%u fd:%d", base, size, fd);

  if (base) {
    err = UnmapBuffer(base, size);
  }

  return err;
}

Error SnapDMAAllocator::UnmapBuffer(void *base, unsigned int size) {
  auto err = Error::NONE;
  if (munmap(base, size)) {
    err = Error::BAD_VALUE;
    DLOGE("dma: Failed to unmap memory at %p : %s", base, strerror(errno));
  }

  return err;
}

Error SnapDMAAllocator::MapBuffer(void **base, unsigned int size, int fd) {
  auto err = Error::NONE;

  void *addr = 0;

  addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  *base = addr;
  if (addr == MAP_FAILED) {
    err = Error::BAD_VALUE;
    DLOGE("dma: Failed to map memory in the client: %s", strerror(errno));
  } else {
    DLOGD_IF(enable_logs, "dma: Mapped buffer base:%p size:%u fd:%d", addr, size, fd);
  }

  return err;
}

Error SnapDMAAllocator::CleanBuffer(void * /*base*/, unsigned int /*size*/, int op,
                                    int dma_buf_fd) {
  struct dma_buf_sync sync;
  auto err = Error::NONE;

  switch (op) {
    case CACHE_CLEAN:
      sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW;
      break;
    case CACHE_INVALIDATE:
      sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW;
      break;
    case CACHE_READ_DONE:
      sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ;
      break;
    default:
      DLOGE("%s: Invalid operation %d", __FUNCTION__, op);
      return Error::BAD_VALUE;
  }

  if (ioctl(dma_buf_fd, static_cast<int>(DMA_BUF_IOCTL_SYNC), &sync)) {
    err = Error::BAD_VALUE;
    DLOGE("%s: DMA_BUF_IOCTL_SYNC failed with error - %s", __FUNCTION__, strerror(errno));
    return err;
  }

  return Error::NONE;
}

int SnapDMAAllocator::ImportBuffer(int fd) {
  return fd;
}

Error SnapDMAAllocator::SecureMemPerms(AllocData *ad) {
  int ret = 0;
  std::unique_ptr<VmMem> vmmem = VmMem::CreateVmMem();
  if (!vmmem) {
    DLOGE("Failed to create VmMem");
    return Error::BAD_VALUE;
  }
  VmPerm vm_perms;

  for (auto const &vm_name : ad->vm_names) {
    VmHandle handle = vmmem->FindVmByName(vm_name);
    if (vm_name == "qcom,cp_sec_display" || vm_name == "qcom,cp_camera_preview") {
      vm_perms.push_back(std::make_pair(handle, VMMEM_READ));
    } else {
      vm_perms.push_back(std::make_pair(handle, VMMEM_READ | VMMEM_WRITE));
    }
  }

  ret = vmmem->LendDmabuf(ad->fd, vm_perms);
  if (!ret) {
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

void SnapDMAAllocator::GetHeapInfo(vendor_qti_hardware_display_common_BufferUsage usage,
                                   bool sensor_flag, std::string *dma_heap_name,
                                   std::vector<std::string> *dma_vm_names, unsigned int *alloc_type,
                                   unsigned int *flags, unsigned int *alloc_size) {
  // Query Camera Security Framework in order to allocate from legacy/non-legacy heap
  GetCSFVersion();
  std::string heap_name = "qcom,system";
  unsigned int type = 0;
  if (static_cast<uint64_t>(usage & vendor_qti_hardware_display_common_BufferUsage::PROTECTED)) {
    if (usage & vendor_qti_hardware_display_common_BufferUsage::QTI_PRIVATE_SECURE_DISPLAY) {
      heap_name = "qcom,display";
      dma_vm_names->push_back("qcom,cp_sec_display");
    } else if (usage & vendor_qti_hardware_display_common_BufferUsage::CAMERA_OUTPUT) {
      int secure_preview_only = debug_->IsSecurePreviewOnlyEnabled();
      if (CSFEnabled()) {
        heap_name = "qcom,system";
        *alloc_size = ALIGN(*alloc_size, SIZE_2MB);
      } else {
        heap_name = "qcom,display";
      }
      if (!CSFEnabled()) {
        if (static_cast<uint64_t>(
                usage & vendor_qti_hardware_display_common_BufferUsage::COMPOSER_OVERLAY)) {
          if (secure_preview_only) {
            dma_vm_names->push_back("qcom,cp_camera_preview");
          } else {
            dma_vm_names->push_back("qcom,cp_camera");
            dma_vm_names->push_back("qcom,cp_camera_preview");
          }
        } else {
          dma_vm_names->push_back("qcom,cp_camera");
        }
      }
    } else {
      heap_name = "qcom,secure-pixel";
    }
    type |=
        static_cast<uint64_t>(usage & vendor_qti_hardware_display_common_BufferUsage::PROTECTED);
  } else {
    if (static_cast<uint64_t>(usage &
                              vendor_qti_hardware_display_common_BufferUsage::CAMERA_OUTPUT)) {
      //Allocate buffer from qcom,moveable heap for only camera use case
      if (movable_heap_system_available_) {
        heap_name = "qcom,system-movable";
      }
    }
  }

  if (usage & vendor_qti_hardware_display_common_BufferUsage::QTI_PRIVATE_TRUSTED_VM) {
    // Allocate buffer from system heap and align the size to 2MB for all trusted UI use cases
    heap_name = "qcom,system";
    *alloc_size = ALIGN(*alloc_size, SIZE_2MB);
  }
  if (usage & vendor_qti_hardware_display_common_BufferUsage::SENSOR_DIRECT_DATA) {
    if (sensor_flag) {
      DLOGI("gralloc::sns_direct_data with system_heap");
      heap_name = "qcom,system";
    }
  }
  *alloc_type = type;
  *dma_heap_name = heap_name;
  return;
}

void SnapDMAAllocator::GetVMPermission(vendor_qti_hardware_display_common_BufferPermission buf_perm,
                                       std::bitset<kVmPermissionMax> *vm_perm) {
  if (!vm_perm) {
    return;
  }
  vm_perm->reset();
  if (buf_perm.read) {
    vm_perm->set(kVmPermissionRead);
  }
  if (buf_perm.write) {
    vm_perm->set(kVmPermissionWrite);
  }
  if (buf_perm.execute) {
    vm_perm->set(kVmPermissionExecute);
  }
}

Error SnapDMAAllocator::SetBufferPermission(
    int fd, vendor_qti_hardware_display_common_BufferPermission *buffer_perm, int64_t *mem_hdl) {
  int ret = 0;
  if (!mem_hdl) {
    return Error::BAD_VALUE;
  }
  *mem_hdl = -1;
  if (!buffer_perm) {
    return Error::NONE;
  }

  InitMemUtils();
  if (!mem_buf_) {
    return Error::BAD_VALUE;
  }

  VmParams vm_params = {};
  bool shared = false;
  if (buffer_perm[static_cast<int>(
                      vendor_qti_hardware_display_common_BufferClient::BUFFERCLIENT_TRUSTED_VM)]
          .permission != 0) {
    std::bitset<kVmPermissionMax> vm_perm = {0};
    GetVMPermission(buffer_perm[static_cast<int>(
                        vendor_qti_hardware_display_common_BufferClient::BUFFERCLIENT_TRUSTED_VM)],
                    &vm_perm);
    vm_params.emplace(kVmTypeTrusted, vm_perm);
  }

  // if untrusted vm is not in the list then its a secure usecase
  if (buffer_perm[static_cast<int>(
                      vendor_qti_hardware_display_common_BufferClient::BUFFERCLIENT_UNTRUSTED_VM)]
          .permission == 0) {
    std::bitset<kVmPermissionMax> vm_perm = {0};
    GetVMPermission(buffer_perm[static_cast<int>(
                        vendor_qti_hardware_display_common_BufferClient::BUFFERCLIENT_DPU)],
                    &vm_perm);
    vm_params.emplace(kVmTypeCpPixel, vm_perm);
  } else {
    std::bitset<kVmPermissionMax> vm_perm = {0};
    GetVMPermission(
        buffer_perm[static_cast<int>(
            vendor_qti_hardware_display_common_BufferClient::BUFFERCLIENT_UNTRUSTED_VM)],
        &vm_perm);
    vm_params.emplace(kVmTypePrimary, vm_perm);
    shared = true;
  }
  if (!vm_params.empty()) {
    ret = mem_buf_->Export(fd, vm_params, shared, mem_hdl);
    DLOGI("fd %d mem_hdl %lld ret %d", fd, *mem_hdl, ret);
  }
  if (!ret) {
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

void SnapDMAAllocator::InitMemUtils() {
  if (mem_utils_lib_) {
    return;
  }
  mem_utils_lib_ = ::dlopen(MEMBUF_CLIENT_LIB_NAME, RTLD_NOW);
  if (mem_utils_lib_) {
    CreateMemBuf_ = reinterpret_cast<CreateMemBufInterface>(
        ::dlsym(mem_utils_lib_, CREATE_MEMBUF_INTERFACE_NAME));
    DestroyMemBuf_ = reinterpret_cast<DestroyMemBufInterface>(
        ::dlsym(mem_utils_lib_, DESTROY_MEMBUF_INTERFACE_NAME));
    if (!CreateMemBuf_ || !DestroyMemBuf_) {
      DLOGW("Membuf Symbols not resolved");
      return;
    }
  } else {
    DLOGW("Unable to load = %s, error = %s", MEMBUF_CLIENT_LIB_NAME, ::dlerror());
    return;
  }
  int err = CreateMemBuf_(&mem_buf_);
  if (err != 0) {
    DLOGW("GetMemBuf failed!! %d", err);
    return;
  }
  // check heap availability
  auto heap_list = buffer_allocator_.GetDmabufHeapList();
  movable_heap_system_available_ = heap_list.find("system-movable") != heap_list.end();
  movable_heap_ubwcp_available_ = heap_list.find("ubwcp-movable") != heap_list.end();

  DLOGI("system movable heap is %d ", movable_heap_system_available_);
  DLOGI("ubwcp movable heap is %d ", movable_heap_ubwcp_available_);
}

void SnapDMAAllocator::DeinitMemUtils() {
  if (DestroyMemBuf_) {
    DestroyMemBuf_();
  }
  if (mem_utils_lib_) {
    ::dlclose(mem_utils_lib_);
    mem_utils_lib_ = nullptr;
  }
}

void SnapDMAAllocator::Deinit() {
  DeinitMemUtils();
  if (dma_dev_fd_ > FD_INIT) {
    close(dma_dev_fd_);
  }

  dma_dev_fd_ = FD_INIT;
}

void SnapDMAAllocator::GetCSFVersion() {
  if (csf_initialized_) {
    return;
  }
#ifdef TARGET_USES_SMMU_PROXY
  int fd = open(smmu_proxy_node_.c_str(), O_RDONLY);
  if (fd < 0) {
    DLOGW("Failed to open smmu proxy node = %s, error = %s", smmu_proxy_node_.c_str(),
          strerror(errno));
    return;
  }
  if (ioctl(fd, QTI_SMMU_PROXY_GET_VERSION_IOCTL, &csf_version_)) {
    DLOGW("%s: QTI_SMMU_PROXY_GET_VERSION_IOCTL failed with error - %s", __FUNCTION__,
          strerror(errno));
    return;
  }
  csf_initialized_ = true;
#endif
  return;
}

bool SnapDMAAllocator::CSFEnabled() {
#ifdef TARGET_USES_SMMU_PROXY
  if ((csf_version_.max_ver == 5 && csf_version_.arch_ver == 2) || csf_version_.arch_ver > 2) {
    return true;
  }
#endif
  return false;
}

}  // namespace snapalloc
