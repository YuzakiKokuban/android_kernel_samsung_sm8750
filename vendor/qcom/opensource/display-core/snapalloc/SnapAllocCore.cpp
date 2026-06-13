// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <unistd.h>
#include <utils/CallStack.h>
#include <iostream>

#include <dlfcn.h>
#include "SnapAllocCore.h"
#include "SnapHandleInternal.h"
#include "SnapTypes.h"
#include "SnapUtils.h"

bool enable_logs = false;

namespace snapalloc {

SnapAllocCore *SnapAllocCore::instance_{nullptr};
std::mutex SnapAllocCore::snapalloc_core_mutex_;

SnapAllocCore::~SnapAllocCore() {}

SnapAllocCore::SnapAllocCore() : next_id_(0) {
  handles_map_.clear();
  constraint_mgr_ = SnapConstraintManager::GetInstance();
  metadata_mgr_ = SnapMetadataManager::GetInstance();
  mem_alloc_intf_ = SnapMemAllocator::GetInstance();
  enable_logs = Debug::GetInstance()->IsDebugLoggingEnabled();
}

SnapAllocCore *SnapAllocCore::GetInstance() {
  std::lock_guard<std::mutex> lock(snapalloc_core_mutex_);

  if (instance_ == nullptr) {
    instance_ = new SnapAllocCore();
  }
  return instance_;
}

Error SnapAllocCore::AllocateBuffer(AllocData *ad, AllocData *m_data,
                                    unsigned custom_content_md_size, BufferDescriptor *desc,
                                    BufferDescriptor *out_desc, bool test_alloc) {
  auto err = mem_alloc_intf_->AllocateMem(ad, out_desc->usage, out_desc->format);
  if (err != Error::NONE) {
    DLOGE("Failed to allocate memory for format %d usage %d", out_desc->format, out_desc->usage);
    return err;
  }

  uint64_t reserved_size = desc->reservedSize;

  m_data->size = metadata_mgr_->GetMetaDataSize(reserved_size, custom_content_md_size);
  m_data->align = PAGE_SIZE;
  err = mem_alloc_intf_->AllocateMem(
      m_data, static_cast<vendor_qti_hardware_display_common_BufferUsage>(0),
      static_cast<vendor_qti_hardware_display_common_PixelFormat>(0));
  if (err != Error::NONE) {
    DLOGE("Failed to allocate metadata memory");
    return err;
  }

  return Error::NONE;
}

// Test note: check int for error, then validate handles from vector
Error SnapAllocCore::Allocate(BufferDescriptor desc, int count,
                              std::vector<SnapHandleInternal *> *handles, bool test_alloc) {
  std::lock_guard<std::mutex> buffer_lock(buffer_lock_);
  for (int i = 0; i < count; i++) {
    OVERFLOW_ERR_RETURN(desc.reservedSize, sizeof(SnapMetadata), OverflowType::ADD);
    OVERFLOW_ERR_RETURN((desc.reservedSize + sizeof(SnapMetadata)), PAGE_SIZE, OverflowType::ADD);

    AllocData ad;
    AllocData m_data;
    vendor_qti_hardware_display_common_BufferLayout layout;
    BufferDescriptor out_desc;
    SnapHandleInternal *hnd;
    int out_priv_flags = 0;
    auto err = constraint_mgr_->GetAllocationData(desc, &ad, &layout, &out_desc, &out_priv_flags);
    if (err != Error::NONE) {
      DLOGE("Constraint manager failed to get allocation data - err %d", err);
      return err;
    }

    if (ad.size == 0) {
      DLOGE("Allocation size is 0");
      return Error::UNSUPPORTED;
    }

    if (test_alloc) {
      return Error::NONE;
    }

    int buffer_type = IsYuv(out_desc.format);
    int aligned_width_in_pixels;
    uint64_t id = ++next_id_;
    uint64_t pixel_format_modifier = GetPixelFormatModifier(out_desc);
    constraint_mgr_->ConvertAlignedWidthFromBytesToPixels(
        out_desc.format, layout.aligned_width_in_bytes, &aligned_width_in_pixels);
    unsigned custom_content_md_size =
        metadata_mgr_->GetCustomContentMetadataSize(out_desc.format, out_desc.usage);

    AllocateBuffer(&ad, &m_data, custom_content_md_size, &desc, &out_desc, test_alloc);

    if (desc.usage & QTI_PRIVATE_MULTI_VIEW_INFO) {
      AllocData ad_2;
      AllocData m_data_2;
      ad_2 = ad;
      m_data_2 = m_data;

      AllocateBuffer(&ad_2, &m_data_2, custom_content_md_size, &desc, &out_desc, test_alloc);
      hnd = SnapHandleInternal::createMultiviewHandle(
          ad.fd, m_data.fd, ad_2.fd, m_data_2.fd, out_priv_flags, layout.aligned_width_in_bytes,
          aligned_width_in_pixels, layout.aligned_height, desc.width, desc.height, out_desc.format,
          buffer_type, id, ++next_id_, ad.size, desc.usage, pixel_format_modifier, desc.layerCount,
          desc.reservedSize, custom_content_md_size);
    } else {
      hnd = SnapHandleInternal::createSingleHandle(
          ad.fd, m_data.fd, out_priv_flags, layout.aligned_width_in_bytes, aligned_width_in_pixels,
          layout.aligned_height, desc.width, desc.height, out_desc.format, buffer_type, id, ad.size,
          desc.usage, pixel_format_modifier, desc.layerCount, desc.reservedSize,
          custom_content_md_size);
    }

    if (hnd == nullptr) {
      DLOGE("%s:: Invalid Handle", __FUNCTION__);
      return Error::BAD_BUFFER;
    }

    hnd->base() = 0;
    hnd->base_metadata() = 0;

    err = metadata_mgr_->InitializeMetadata(hnd, desc.format, out_desc, ad, &layout);
    if (err != Error::NONE) {
      DLOGE("Failed to initialize metadata for hnd %lu", hnd->id());
    }

    handles->emplace_back(hnd);
  }
  return Error::NONE;
}

int SnapAllocCore::GetPrivateFlags(vendor_qti_hardware_display_common_BufferUsage usage) {
  int flags = 0;
  if (usage & vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC) {
    flags |= PRIV_FLAGS_UBWC_ALIGNED;
  }

  if (usage & vendor_qti_hardware_display_common_BufferUsage::QTI_PRIVATE_ALLOC_UBWC_PI) {
    flags |= PRIV_FLAGS_UBWC_ALIGNED_PI;
  }

  return flags;
}

SnapHandleInternal *SnapAllocCore::GetBufferFromHandleLocked(SnapHandle *hnd) {
  if (hnd == nullptr) {
    return nullptr;
  }
  auto it = handles_map_.find(hnd);
  if (it != handles_map_.end()) {
    return it->second;
  }

  return nullptr;
}

Error SnapAllocCore::FreeBuffer(SnapHandleInternal *snap_hnd) {
  auto meta_size = metadata_mgr_->GetMetaDataSize(snap_hnd->reserved_size(),
                                                  snap_hnd->custom_content_md_reserved_size());
  // TODO: Off-target tests - passing in buffer path string for shm unlink
  if (mem_alloc_intf_->FreeBuffer(reinterpret_cast<void *>(snap_hnd->base()), snap_hnd->size(),
                                  snap_hnd->fd(), "") != 0) {
    DLOGE("Unable to free buf base");
    return Error::BAD_BUFFER;
  }

  if (mem_alloc_intf_->FreeBuffer(reinterpret_cast<void *>(snap_hnd->base_metadata()), meta_size,
                                  snap_hnd->fd_metadata(), "") != 0) {
    DLOGE("Unable to free buf metadata");
    return Error::BAD_BUFFER;
  }

  DLOGD_IF(enable_logs, "Freed buffer fd %d fd_metadata %d", snap_hnd->fd(),
           snap_hnd->fd_metadata());
  snap_hnd->closeFds();

  free(snap_hnd);
  return Error::NONE;
}

Error SnapAllocCore::Retain(SnapHandle *hnd) {
  auto err = Error::NONE;
  std::lock_guard<std::mutex> lock(buffer_lock_);
  auto buf = GetBufferFromHandleLocked(hnd);
  if (buf != nullptr) {
    buf->IncRef();
    DLOGD_IF(enable_logs, "%s: line %d buf %p id %lu increased ref count to %d", __FUNCTION__,
             __LINE__, buf, buf->id(), buf->GetRefCount());
  } else {
    err = ImportHandleLocked(hnd);
    DLOGD_IF(enable_logs, "%s: line %d: handles_map_ size %d", __FUNCTION__, __LINE__,
             handles_map_.size());
  }
  DLOGD_IF(enable_logs, "===============");

  for (auto &entry : handles_map_) {
    DLOGD_IF(enable_logs, "SnapAllocCore::Retain: handles_map: buf->id() %lu", entry.second->id());
  }
  DLOGD_IF(enable_logs, "===============");
  return err;
}

Error SnapAllocCore::RetainViewBuffer(SnapHandle *meta_hnd, uint32_t view,
                                      SnapHandle **out_view_handle) {
  if (meta_hnd == nullptr) {
    return Error::BAD_BUFFER;
  }

  auto err = Error::NONE;
  std::lock_guard<std::mutex> lock(buffer_lock_);
  auto buf = GetBufferFromHandleLocked(meta_hnd);
  if (buf == nullptr) {
    DLOGE("Retain MetaHandle before retaining auxillary view buffer");
    return Error::UNSUPPORTED;
  }

  SnapHandle *view_handle = buf->CreateViewHandle(view);

  if (!view_handle) {
    return Error::UNSUPPORTED;
  }

  err = ImportHandleLocked(view_handle);
  DLOGD_IF(enable_logs, "%s: line %d: handles_map_ size %d", __FUNCTION__, __LINE__,
           handles_map_.size());

  DLOGD_IF(enable_logs, "===============");

  for (auto &entry : handles_map_) {
    DLOGD_IF(enable_logs, "SnapAllocCore::Retain: handles_map: buf->id %lu", entry.second->id());
  }
  DLOGD_IF(enable_logs, "===============");
  *out_view_handle = view_handle;
  return err;
}

Error SnapAllocCore::Release(SnapHandle *hnd) {
  if (hnd == nullptr) {
    return Error::BAD_BUFFER;
  }
  std::lock_guard<std::mutex> lock(buffer_lock_);
  SnapHandleInternal *snap_hnd_cast = static_cast<SnapHandleInternal *>(hnd);
  auto buf = GetBufferFromHandleLocked(hnd);
  DLOGD_IF(enable_logs, "line %d snap_hnd_cast id %lu ref count %d vs buf ref count %d", __LINE__,
           snap_hnd_cast->id(), snap_hnd_cast->GetRefCount(), buf->GetRefCount());

  if (buf == nullptr) {
    DLOGE("Could not find handle: %p", hnd);
    return Error::BAD_BUFFER;
  }

  if (buf->DecRef()) {
    DLOGD_IF(enable_logs, "line %d snap_hnd_cast id %lu ref count %d vs buf ref count %d", __LINE__,
             snap_hnd_cast->id(), snap_hnd_cast->GetRefCount(), buf->GetRefCount());

    // TODO: buffer dump support
    /*if (allocated_ >= hnd->size()) {
      allocated_ -= hnd->size();
    }*/
    if (FreeBuffer(buf) == Error::NONE) {
      handles_map_.erase(hnd);
      DLOGD_IF(enable_logs, "%s: line %d: handles_map_ size after freeing  %d", __FUNCTION__,
               __LINE__, handles_map_.size());
    } else {
      DLOGE("Failed to free buffer %p", buf);
      return Error::BAD_BUFFER;
    }
  } else {
    DLOGD_IF(enable_logs, "Not freeing - ref count > 0; fd %d metadata_fd %d", buf->fd(),
             buf->fd_metadata());
    return Error::BUF_NOT_FREED;
  }
  return Error::NONE;
}

// Test note: check int for error, then check that void* base address is not null
Error SnapAllocCore::Lock(SnapHandle *hnd, vendor_qti_hardware_display_common_BufferUsage usage,
                          vendor_qti_hardware_display_common_Rect access_region,
                          uint64_t *base_addr) {
  std::lock_guard<std::mutex> lock(buffer_lock_);
  // If buffer is not meant for CPU return err
  if (!CpuCanAccess(usage)) {
    DLOGE("Lock failed - CPU can't access");
    return Error::BAD_VALUE;
  }

  auto buf = GetBufferFromHandleLocked(hnd);

  if (buf == nullptr) {
    DLOGE("Lock failed - no valid SnapHandleInternal");
    return Error::BAD_BUFFER;
  }
  DLOGD_IF(enable_logs,
           "SnapAllocCore lock format %d usage %lu uwidth %d uheight %d, access region right %d "
           "bottom %d",
           buf->format(), buf->usage(), buf->unaligned_width(), buf->unaligned_height(),
           access_region.right, access_region.bottom);

  if (access_region.top < 0 || access_region.left < 0 || access_region.right < 0 ||
      access_region.bottom < 0 || access_region.right > buf->aligned_width_in_pixels() ||
      access_region.bottom > buf->aligned_height()) {
    return Error::BAD_VALUE;
  }

  auto err = Error::NONE;
  if (buf->base() == 0) {
    // we need to map for real
    err = static_cast<Error>(MapBuffer(buf));
  }

  // Invalidate if CPU reads in software and there are non-CPU
  // writers. No need to do this for the metadata buffer as it is
  // only read/written in software.

  if (err == Error::NONE && (buf->flags() & PRIV_FLAGS_CACHED)) {
    if (mem_alloc_intf_->CleanBuffer(reinterpret_cast<void *>(buf->base()), buf->size(),
                                     CACHE_INVALIDATE, buf->fd())) {
      return Error::BAD_BUFFER;
    }
  }

  if (err == Error::NONE) {
    *base_addr = buf->base();
    DLOGD_IF(enable_logs, "SnapAllocCore::lock buf->base %lu", buf->base());

    // Mark the buffer to be flushed after CPU write.
    if (CpuCanWrite(usage)) {
      buf->flush() = true;
    }
  }
  buf->lock_count()++;

  return err;
}

Error SnapAllocCore::Unlock(SnapHandle *hnd) {
  std::lock_guard<std::mutex> lock(buffer_lock_);
  auto status = Error::NONE;

  auto buf = GetBufferFromHandleLocked(hnd);
  if (buf == nullptr || buf->lock_count() <= 0) {
    DLOGW("%s: A bad or an already unlocked buffer.", __FUNCTION__);
    return Error::BAD_BUFFER;
  }

  // Avoid unlocking early for nested lock case
  if (buf->lock_count() == 1) {
    if (buf->flush()) {
      if (mem_alloc_intf_->CleanBuffer(reinterpret_cast<void *>(buf->base()), buf->size(),
                                       CACHE_CLEAN, buf->fd()) != 0) {
        status = Error::BAD_BUFFER;
      }
      buf->flush() = false;
    } else {
      if (mem_alloc_intf_->CleanBuffer(reinterpret_cast<void *>(buf->base()), buf->size(),
                                       CACHE_READ_DONE, buf->fd()) != 0) {
        status = Error::BAD_BUFFER;
      }
    }
  }

  buf->lock_count() = (status == Error::NONE) ? buf->lock_count() - 1 : buf->lock_count();

  return status;
}

Error SnapAllocCore::MapBuffer(SnapHandleInternal *hnd) {
  if (hnd == nullptr) {
    return Error::BAD_BUFFER;
  }

  hnd->base() = 0;
  if (mem_alloc_intf_->MapBuffer(reinterpret_cast<void **>(&hnd->base()), hnd->size(), hnd->fd()) !=
      0) {
    DLOGE("Failed to map buffer");
    return Error::BAD_BUFFER;
  }
  return Error::NONE;
}

Error SnapAllocCore::ValidateBufferSize(SnapHandle *hnd, BufferDescriptor desc) {
  std::lock_guard<std::mutex> lock(buffer_lock_);
  auto buf = GetBufferFromHandleLocked(hnd);
  if (buf == nullptr) {
    return Error::BAD_BUFFER;
  }

  // TODO: query formats.json and store this
  uint64_t max_bpp = 8;

  // Pass descriptor into constraint manager, verify size is same as handle size
  AllocData ad;
  vendor_qti_hardware_display_common_BufferLayout layout;
  BufferDescriptor out_desc;

  int out_priv_flags = 0;
  int ret = constraint_mgr_->GetAllocationData(desc, &ad, &layout, &out_desc, &out_priv_flags);
  int aligned_width_in_pixels = 0;
  constraint_mgr_->ConvertAlignedWidthFromBytesToPixels(
      out_desc.format, layout.aligned_width_in_bytes, &aligned_width_in_pixels);

  if (OVERFLOW_MUL(aligned_width_in_pixels, layout.aligned_height)) {
    DLOGE("%s: Allocatiom size overflow", __FUNCTION__);
    return Error::BAD_BUFFER;
  }

  auto fd_size = static_cast<int>(lseek(buf->fd(), 0, SEEK_END));
  if (fd_size != ad.size) {
    DLOGE("%s: FD size %d does not match expected allocation size %d, buf->size %d", __FUNCTION__,
          fd_size, ad.size, buf->size());
    return Error::BAD_VALUE;
  }
  return Error::NONE;
}

Error SnapAllocCore::FlushLockedBuffer(SnapHandle *hnd) {
  std::lock_guard<std::mutex> lock(buffer_lock_);
  auto status = Error::NONE;

  auto buf = GetBufferFromHandleLocked(hnd);
  if (buf == nullptr || buf->lock_count() <= 0) {
    DLOGW("%s: A bad or an unlocked buffer.", __FUNCTION__);
    return Error::BAD_BUFFER;
  }
  if (mem_alloc_intf_->CleanBuffer(reinterpret_cast<void *>(buf->base()), buf->size(), CACHE_CLEAN,
                                   buf->fd()) != 0) {
    status = Error::BAD_BUFFER;
  }

  return status;
}

Error SnapAllocCore::RereadLockedBuffer(SnapHandle *hnd) {
  std::lock_guard<std::mutex> lock(buffer_lock_);
  auto status = Error::NONE;

  auto buf = GetBufferFromHandleLocked(hnd);
  if (buf == nullptr || buf->lock_count() <= 0) {
    DLOGW("%s: A bad or an unlocked buffer.", __FUNCTION__);
    return Error::BAD_BUFFER;
  }
  if (mem_alloc_intf_->CleanBuffer(reinterpret_cast<void *>(buf->base()), buf->size(),
                                   CACHE_INVALIDATE, buf->fd()) != 0) {
    status = Error::BAD_BUFFER;
  }

  return status;
}

Error SnapAllocCore::ImportHandleLocked(SnapHandle *hnd) {
  if (SnapHandleInternal::validate(hnd) != 0) {
    DLOGE("ImportHandleLocked: Invalid handle: %p", hnd);
    return Error::BAD_BUFFER;
  }
  if (hnd == nullptr) {
    DLOGE("Invalid SnapHandle");
    return Error::BAD_BUFFER;
  }

  SnapHandleInternal *snap_hnd = static_cast<SnapHandleInternal *>(hnd);

  DLOGD_IF(enable_logs,
           "id %lu aligned width %d aligned height %d, input width %d  input height %d "
           "format out %d size %d, usage %lu",
           snap_hnd->id(), snap_hnd->aligned_width_in_bytes(), snap_hnd->aligned_height(),
           snap_hnd->unaligned_width(), snap_hnd->unaligned_height(), snap_hnd->format(),
           snap_hnd->size(), snap_hnd->usage());

  DLOGD_IF(enable_logs, "Importing handle with id %lu", snap_hnd->id());
  if (mem_alloc_intf_->ImportBuffer(snap_hnd->fd()) < 0) {
    DLOGE("Failed to import buffer: hnd: %p, fd:%d, id:%lu", snap_hnd, snap_hnd->fd(),
          snap_hnd->id());
    FreeBuffer(snap_hnd);
    return Error::BAD_BUFFER;
  }

  if (mem_alloc_intf_->ImportBuffer(snap_hnd->fd_metadata()) < 0) {
    DLOGE("Failed to import metadata buffer: hnd: %p, fd:%d, id:%lu", snap_hnd,
          snap_hnd->fd_metadata(), snap_hnd->id());
    FreeBuffer(snap_hnd);
    return Error::BAD_BUFFER;
  }
  // Initialize members that aren't transported
  snap_hnd->size() = static_cast<unsigned int>(lseek(snap_hnd->fd(), 0, SEEK_END));
  snap_hnd->base() = 0;
  snap_hnd->base_metadata() = 0;
  snap_hnd->ResetRefCount();
  snap_hnd->IncRef();
  DLOGD_IF(enable_logs, "snap_hnd ref count in ImportHandleLocked %d", snap_hnd->GetRefCount());
  if (metadata_mgr_->ValidateAndMap(snap_hnd)) {
    DLOGE("Failed to map metadata: hnd: %p, fd:%d, id:%lu", snap_hnd, snap_hnd->fd(),
          snap_hnd->id());
    FreeBuffer(snap_hnd);
    return Error::BAD_BUFFER;
  }

  RegisterHandleLocked(hnd, snap_hnd);
  /* TODO - tracking allocated mem
  allocated_ += hnd->size();
  if (allocated_ >= kAllocThreshold) {
    kAllocThreshold += kMemoryOffset;
    BuffersDump();
  }*/
  return Error::NONE;
}

void SnapAllocCore::RegisterHandleLocked(SnapHandle *public_hnd, SnapHandleInternal *snap_hnd) {
  if (snap_hnd->base_metadata()) {
    if (snap_hnd->reserved_size() > 0) {
      snap_hnd->reserved_region_base() =
          reinterpret_cast<uint64_t>(snap_hnd->base_metadata() + sizeof(SnapMetadata));
    } else {
      snap_hnd->reserved_region_base() = 0;
    }

    if (snap_hnd->custom_content_md_reserved_size() > 0) {
      snap_hnd->custom_content_md_region_base() = reinterpret_cast<uint64_t>(
          snap_hnd->base_metadata() + sizeof(SnapMetadata) + snap_hnd->reserved_size());
    } else {
      snap_hnd->custom_content_md_region_base() = 0;
    }
  }
  handles_map_.emplace(std::make_pair(public_hnd, snap_hnd));
}

Error SnapAllocCore::IsSupported(BufferDescriptor desc, bool *is_supported) {
  std::vector<SnapHandleInternal *> handles;
  auto err = Allocate(desc, 1, &handles, true);
  *is_supported = (err == Error::NONE) ? true : false;
  return Error::NONE;
}

Error SnapAllocCore::GetMetadata(SnapHandle *hnd,
                                 vendor_qti_hardware_display_common_MetadataType type, void *out) {
  std::lock_guard<std::mutex> buffer_lock(buffer_lock_);
  if (!hnd) {
    DLOGE("%s: Invalid handle", __FUNCTION__);
    return Error::BAD_BUFFER;
  }
  auto buf = GetBufferFromHandleLocked(hnd);
  if (buf == nullptr) {
    DLOGE("%s: Unable to get locked buffer", __FUNCTION__);
    return Error::BAD_BUFFER;
  }
  auto err = metadata_mgr_->ValidateAndMap(buf);
  if (err != 0) {
    DLOGE("%s: ValidateAndMap failed", __FUNCTION__);
    return Error::UNSUPPORTED;
  }
  return metadata_mgr_->Get(buf, type, out);
}

Error SnapAllocCore::SetMetadata(SnapHandle *hnd,
                                 vendor_qti_hardware_display_common_MetadataType type, void *in) {
  std::lock_guard<std::mutex> buffer_lock(buffer_lock_);
  if (!hnd) {
    DLOGE("%s: Invalid handle", __FUNCTION__);
    return Error::BAD_BUFFER;
  }
  auto buf = GetBufferFromHandleLocked(hnd);
  if (buf == nullptr) {
    DLOGE("%s: Unable to get locked buffer", __FUNCTION__);
    return Error::BAD_BUFFER;
  }
  auto err = metadata_mgr_->ValidateAndMap(buf);
  if (err != 0) {
    DLOGE("%s: ValidateAndMap failed", __FUNCTION__);
    return Error::UNSUPPORTED;
  }

  return metadata_mgr_->Set(buf, type, in);
}

Error SnapAllocCore::GetFromBufferDescriptor(BufferDescriptor desc,
                                             vendor_qti_hardware_display_common_MetadataType type,
                                             void *out) {
  return metadata_mgr_->GetFromBufferDescriptor(desc, type, out);
}

Error SnapAllocCore::DumpBuffer(SnapHandle *hnd) {
  return Error::UNSUPPORTED;
}

// This may need to call metadata_mgr_->DumpBuffer with each handle in the map
// as metadata manager would not be aware of all the buffers
Error SnapAllocCore::DumpBuffers() {
  return Error::UNSUPPORTED;
}

Error SnapAllocCore::GetMetadataState(SnapHandle *hnd,
                                 vendor_qti_hardware_display_common_MetadataType type, bool *out) {
  std::lock_guard<std::mutex> buffer_lock(buffer_lock_);
  if (!hnd) {
    DLOGE("%s: Invalid handle", __FUNCTION__);
    return Error::BAD_BUFFER;
  }
  auto buf = GetBufferFromHandleLocked(hnd);
  if (buf == nullptr) {
    DLOGE("%s: Unable to get locked buffer", __FUNCTION__);
    return Error::BAD_BUFFER;
  }
  auto err = metadata_mgr_->ValidateAndMap(buf);
  if (err != 0) {
    DLOGE("%s: ValidateAndMap failed", __FUNCTION__);
    return Error::UNSUPPORTED;
  }
  return metadata_mgr_->GetMetadataState(buf, type, out);
}

}  // namespace snapalloc
