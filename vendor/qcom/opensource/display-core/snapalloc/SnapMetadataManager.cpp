// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SnapMetadataManager.h"
#include "BufferLayout.h"
#include "Dataspace.h"
#include "PixelFormat.h"
#include "Rect.h"
#include "SnapUtils.h"

#include <unordered_map>

namespace snapalloc {

SnapMetadataManager *SnapMetadataManager::instance_{nullptr};
std::mutex SnapMetadataManager::metadata_mgr_mutex_;

SnapMetadataManager::~SnapMetadataManager() {}

SnapMetadataManager::SnapMetadataManager() {
  constraint_mgr_ = SnapConstraintManager::GetInstance();
  mem_allocator_ = SnapMemAllocator::GetInstance();
  ubwc_policy_ = UBWCPolicy::GetInstance();
}

SnapMetadataManager *SnapMetadataManager::GetInstance() {
  std::lock_guard<std::mutex> lock(metadata_mgr_mutex_);

  if (instance_ == nullptr) {
    instance_ = new SnapMetadataManager();
  }
  return instance_;
}

Error SnapMetadataManager::BufferIDHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                          void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<uint64_t *>(out_get) = handle->id();
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::BAD_VALUE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::NameHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                      void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    std::string name(reinterpret_cast<char *>(buf_des->name));
    *(reinterpret_cast<std::string *>(out_get)) = name;
    return Error::NONE;
  } else if (out_get != nullptr) {
    std::string name(metadata->name);
    *(reinterpret_cast<std::string *>(out_get)) = name;
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::BAD_VALUE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::WidthHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                       void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    *static_cast<uint64_t *>(out_get) = static_cast<uint64_t>(buf_des->width);
    return Error::NONE;
  } else if (out_get != nullptr) {
    *static_cast<int *>(out_get) = handle->unaligned_width();
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::BAD_VALUE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::HeightHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                        void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    *static_cast<uint64_t *>(out_get) = static_cast<uint64_t>(buf_des->height);
    return Error::NONE;
  } else if (out_get != nullptr) {
    *static_cast<int *>(out_get) = handle->unaligned_height();
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::BAD_VALUE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::LayerCountHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                            void *in_set, void *out_get,
                                            BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    *static_cast<uint64_t *>(out_get) = static_cast<uint64_t>(buf_des->layerCount);
    return Error::NONE;
  } else if (out_get != nullptr) {
    *static_cast<unsigned int *>(out_get) = handle->layer_count();
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::BAD_VALUE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::PixelFormatRequestedHelper(SnapMetadata *metadata,
                                                      SnapHandleInternal *handle, void *in_set,
                                                      void *out_get, BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_PixelFormat *>(out_get) =
        static_cast<vendor_qti_hardware_display_common_PixelFormat>(buf_des->format);
    return Error::NONE;
  } else if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_PixelFormat *>(out_get) = metadata->pixel_format_requested;
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::BAD_VALUE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::PixelFormatAllocatedHelper(SnapMetadata *metadata,
                                                      SnapHandleInternal *handle, void *in_set,
                                                      void *out_get, BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_PixelFormat *>(out_get) =
        static_cast<vendor_qti_hardware_display_common_PixelFormat>(buf_des->format);
    return Error::NONE;
  } else if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_PixelFormat *>(out_get) = handle->format();
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::BAD_VALUE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::PixelFormatFourCCHelper(SnapMetadata *metadata,
                                                   SnapHandleInternal *handle, void *in_set,
                                                   void *out_get, BufferDescriptor *buf_des) {
  // TODO - can't be returned as pointer
  if (buf_des != nullptr) {
    uint32_t drm_format = 0;
    uint64_t drm_format_modifier = 0;
    bool ubwc_enable = ubwc_policy_->IsUBWCAlloc(*buf_des);
    if (ubwc_enable) {
      GetDRMFormat(buf_des->format, buf_des->usage, PRIV_FLAGS_UBWC_ALIGNED, &drm_format,
                   &drm_format_modifier);
    } else {
      GetDRMFormat(buf_des->format, buf_des->usage, 0, &drm_format, &drm_format_modifier);
    }
    *static_cast<uint32_t *>(out_get) = static_cast<uint32_t>(drm_format);
    return Error::NONE;
  } else if (out_get != nullptr) {
    uint32_t drm_format = 0;
    uint64_t drm_format_modifier = 0;
    SnapMetadataManager::GetDRMFormat(handle->format(), handle->usage(), handle->flags(),
                                      &drm_format, &drm_format_modifier);
    *static_cast<uint32_t *>(out_get) = drm_format;
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::DRMPixelFormatModifierHelper(SnapMetadata *metadata,
                                                        SnapHandleInternal *handle, void *in_set,
                                                        void *out_get, BufferDescriptor *buf_des) {
  // TODO - can't be returned as pointer
  if (buf_des != nullptr) {
    uint32_t drm_format = 0;
    uint64_t drm_format_modifier = 0;
    bool ubwc_enable = ubwc_policy_->IsUBWCAlloc(*buf_des);
    if (ubwc_enable) {
      GetDRMFormat(buf_des->format, buf_des->usage, PRIV_FLAGS_UBWC_ALIGNED, &drm_format,
                   &drm_format_modifier);
    } else {
      GetDRMFormat(buf_des->format, buf_des->usage, 0, &drm_format, &drm_format_modifier);
    }
    *static_cast<uint64_t *>(out_get) = static_cast<uint64_t>(drm_format_modifier);
    return Error::NONE;
  } else if (out_get != nullptr) {
    uint32_t drm_format = 0;
    uint64_t drm_format_modifier = 0;
    SnapMetadataManager::GetDRMFormat(handle->format(), handle->usage(), handle->flags(),
                                      &drm_format, &drm_format_modifier);
    *static_cast<uint64_t *>(out_get) = drm_format_modifier;
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::UsageHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                       void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    *static_cast<uint64_t *>(out_get) = static_cast<uint64_t>(buf_des->usage);
    return Error::NONE;
  } else if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_BufferUsage *>(out_get) = handle->usage();
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::BAD_VALUE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::AllocationSizeHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                                void *in_set, void *out_get,
                                                BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    AllocData ad;
    vendor_qti_hardware_display_common_BufferLayout layout;
    BufferDescriptor out_desc;
    int out_priv_flags = 0;
    auto err =
        constraint_mgr_->GetAllocationData(*buf_des, &ad, &layout, &out_desc, &out_priv_flags);
    if (err != Error::NONE) {
      DLOGE("Invalid allocation - unable to get allocation size");
      return err;
    }
    *static_cast<uint32_t *>(out_get) = static_cast<uint32_t>(ad.size);
    return Error::NONE;
  } else if (out_get != nullptr) {
    *static_cast<uint32_t *>(out_get) = static_cast<uint32_t>(handle->size());
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::BaseViewHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                          void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    return Error::UNSUPPORTED;
  } else if (out_get != nullptr) {
    *static_cast<uint32_t *>(out_get) = static_cast<uint32_t>(handle->view());
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::MultiViewHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                           void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    return Error::UNSUPPORTED;
  } else if (out_get != nullptr) {
    *static_cast<uint32_t *>(out_get) = static_cast<uint32_t>(handle->getViewInfo());
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::ThreeDimensionalRefInfoHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                               void *in_set, void *out_get,
                                               BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_ThreeDimensionalRefInfo *>(out_get) =
        metadata->three_dimensional_ref_info;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->three_dimensional_ref_info =
        *static_cast<vendor_qti_hardware_display_common_ThreeDimensionalRefInfo *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::ProtectedContentHelper(SnapMetadata *metadata,
                                                  SnapHandleInternal *handle, void *in_set,
                                                  void *out_get, BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    uint64_t protected_content =
        buf_des->usage & vendor_qti_hardware_display_common_BufferUsage::PROTECTED ? 1 : 0;
    *static_cast<uint64_t *>(out_get) = protected_content;
    return Error::NONE;
  } else if (out_get != nullptr) {
    uint64_t protected_content =
        handle->usage() & vendor_qti_hardware_display_common_BufferUsage::PROTECTED ? 1 : 0;
    *static_cast<uint64_t *>(out_get) = protected_content;
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::CompressionHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                             void *in_set, void *out_get,
                                             BufferDescriptor *buf_des) {
  // TODO - can't be returned as pointer
  if (buf_des != nullptr) {
    bool ubwc_enable = ubwc_policy_->IsUBWCAlloc(*buf_des);
    int64_t qti_compression = vendor_qti_hardware_display_common_Compression::COMPRESSION_NONE;
    if (ubwc_enable) {
      qti_compression = ubwc_policy_->GetUBWCScheme(buf_des->format, buf_des->usage);
    }
    *static_cast<int64_t *>(out_get) = qti_compression;
    return Error::NONE;
  } else if (out_get != nullptr) {
    BufferDescriptor out_desc;
    BufferDescriptor desc = {.format = handle->format(),
                             .usage = handle->usage(),
                             .width = handle->aligned_width_in_pixels(),
                             .height = handle->aligned_height(),
                             .layerCount = static_cast<int32_t>(handle->layer_count()),
                             .reservedSize = handle->reserved_size()};
    UBWCPolicy *ubwc_policy = UBWCPolicy::GetInstance();
    bool ubwc_enable = ubwc_policy->IsUBWCAlloc(desc);
    int64_t qti_compression = vendor_qti_hardware_display_common_Compression::COMPRESSION_NONE;
    if (ubwc_enable) {
      qti_compression = ubwc_policy->GetUBWCScheme(handle->format(), handle->usage());
    }
    *static_cast<int64_t *>(out_get) = qti_compression;
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::InterlacedHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                            void *in_set, void *out_get,
                                            BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    int64_t qti_interlaced = vendor_qti_hardware_display_common_Interlaced::INTERLACED_NONE;
    if (metadata->interlaced > 0) {
      qti_interlaced = vendor_qti_hardware_display_common_Interlaced::QTI_INTERLACED;
    }
    *static_cast<int64_t *>(out_get) = qti_interlaced;
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::ChromaSitingHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                              void *in_set, void *out_get,
                                              BufferDescriptor *buf_des) {
  // TODO - can't be returned as pointer
  if (out_get != nullptr) {
    int64_t chroma_siting = vendor_qti_hardware_display_common_ChromaSiting::CHROMA_SITING_NONE;
    *static_cast<int64_t *>(out_get) = chroma_siting;
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::PlaneLayoutsHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                              void *in_set, void *out_get,
                                              BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    AllocData ad;
    vendor_qti_hardware_display_common_BufferLayout layout;
    BufferDescriptor out_desc;
    int out_priv_flags = 0;
    auto err =
        constraint_mgr_->GetAllocationData(*buf_des, &ad, &layout, &out_desc, &out_priv_flags);
    if (err != Error::NONE) {
      DLOGE("Invalid allocation - unable to create plane layout");
      return err;
    }
    DLOGD_IF(enable_logs,
             "get plane layout from buffer descriptor - out_desc.format %d - "
             "size %d",
             out_desc.format, layout.size_in_bytes);
    *static_cast<vendor_qti_hardware_display_common_BufferLayout *>(out_get) = layout;
    return Error::NONE;
  } else if (out_get != nullptr) {
    if (metadata->interlaced) {
      // Recalculate plane layouts for interlaced
      AllocData ad;
      vendor_qti_hardware_display_common_BufferLayout layout;
      BufferDescriptor desc = {.format = handle->format(),
                               .usage = handle->usage(),
                               .width = handle->unaligned_width(),
                               .height = handle->unaligned_height(),
                               .layerCount = static_cast<int32_t>(handle->layer_count()),
                               .reservedSize = handle->reserved_size()};
      static vendor_qti_hardware_display_common_KeyValuePair modifier = {
          .key = "interlaced", .value = static_cast<uint64_t>(1)};
      desc.additionalOptions.emplace_back(modifier);
      BufferDescriptor out_desc;
      int out_priv_flags = 0;
      auto err = constraint_mgr_->GetAllocationData(desc, &ad, &layout,
                                                    &out_desc, &out_priv_flags);

      if (err != Error::NONE) {
        DLOGE("Invalid allocation - unable to create plane layout");
        return err;
      }

      *static_cast<vendor_qti_hardware_display_common_BufferLayout *>(out_get) =
          layout;
    } else {
      *static_cast<vendor_qti_hardware_display_common_BufferLayout *>(out_get) =
          metadata->buffer_layout;
    }

    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::CropHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                      void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_Rect *>(out_get) = metadata->crop;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->crop = *static_cast<vendor_qti_hardware_display_common_Rect *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::DataspaceHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                           void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_Dataspace *>(out_get) =
        metadata->color.dataspace;
    return Error::NONE;
  } else if (in_set != nullptr) {
    // TODO: Add combination validation if needed
    metadata->color.dataspace =
        *static_cast<vendor_qti_hardware_display_common_Dataspace *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::BlendModeHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                           void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_BlendMode *>(out_get) =
        static_cast<vendor_qti_hardware_display_common_BlendMode>(metadata->blendMode);
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->blendMode = *static_cast<vendor_qti_hardware_display_common_BlendMode *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::VTTimestampHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                             void *in_set, void *out_get,
                                             BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<uint64_t *>(out_get) = metadata->vtTimeStamp;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->vtTimeStamp = *static_cast<uint64_t *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::BufferDequeueDurationHelper(SnapMetadata *metadata,
                                                       SnapHandleInternal *handle, void *in_set,
                                                       void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<int64_t *>(out_get) = metadata->bufferDequeueDuration;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->bufferDequeueDuration = *static_cast<int64_t *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::PPParamInterlacedHelper(SnapMetadata *metadata,
                                                   SnapHandleInternal *handle, void *in_set,
                                                   void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<int32_t *>(out_get) = metadata->interlaced;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->interlaced = *static_cast<int32_t *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::VideoPerfModeHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                               void *in_set, void *out_get,
                                               BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<uint32_t *>(out_get) = metadata->isVideoPerfMode;
    return Error::NONE;
  }
  if (in_set != nullptr) {
    metadata->isVideoPerfMode = *static_cast<uint32_t *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::GraphicsMetadataHelper(SnapMetadata *metadata,
                                                  SnapHandleInternal *handle, void *in_set,
                                                  void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_GraphicsMetadata *>(out_get) =
        metadata->graphics_metadata;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->graphics_metadata =
        *static_cast<vendor_qti_hardware_display_common_GraphicsMetadata *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::UBWCCRStatsInfoHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                                 void *in_set, void *out_get,
                                                 BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    vendor_qti_hardware_display_common_UBWCStats *snap_ubwc_stats =
        static_cast<vendor_qti_hardware_display_common_UBWCStats *>(out_get);
    int numelems =
        sizeof(metadata->ubwcCRStats) / sizeof(vendor_qti_hardware_display_common_UBWCStats);
    for (int i = 0; i < numelems; i++) {
      snap_ubwc_stats[i] = metadata->ubwcCRStats[i];
    }
    return Error::NONE;
  } else if (in_set != nullptr) {
    vendor_qti_hardware_display_common_UBWCStats *stats =
        static_cast<vendor_qti_hardware_display_common_UBWCStats *>(in_set);
    int numelems =
        sizeof(metadata->ubwcCRStats) / sizeof(vendor_qti_hardware_display_common_UBWCStats);
    for (int i = 0; i < numelems; i++) {
      metadata->ubwcCRStats[i] = stats[i];
    }
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::RefreshRateHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                             void *in_set, void *out_get,
                                             BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<float *>(out_get) = metadata->refreshrate;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->refreshrate = *static_cast<float *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::MapSecureBufferHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                                 void *in_set, void *out_get,
                                                 BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<int32_t *>(out_get) = metadata->mapSecureBuffer;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->mapSecureBuffer = *static_cast<int32_t *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::LinearFormatHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                              void *in_set, void *out_get,
                                              BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<uint32_t *>(out_get) = metadata->linearFormat;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->linearFormat = *static_cast<uint32_t *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::SingleBufferModeHelper(SnapMetadata *metadata,
                                                  SnapHandleInternal *handle, void *in_set,
                                                  void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<uint32_t *>(out_get) = metadata->isSingleBufferMode;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->isSingleBufferMode = *static_cast<uint32_t *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::CVPMetadataHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                             void *in_set, void *out_get,
                                             BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_CVPMetadata *>(out_get) = metadata->cvpMetadata;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->cvpMetadata = *static_cast<vendor_qti_hardware_display_common_CVPMetadata *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::VideoHistogramStatsHelper(SnapMetadata *metadata,
                                                     SnapHandleInternal *handle, void *in_set,
                                                     void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_VideoHistogramMetadata *>(out_get) =
        metadata->video_histogram_stats;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->video_histogram_stats =
        *static_cast<vendor_qti_hardware_display_common_VideoHistogramMetadata *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::FDHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                    void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<int32_t *>(out_get) = handle->fd();
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::AlignedWidthInPixelsHelper(SnapMetadata *metadata,
                                                      SnapHandleInternal *handle, void *in_set,
                                                      void *out_get, BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    AllocData ad;
    vendor_qti_hardware_display_common_BufferLayout layout;
    BufferDescriptor out_desc;
    int out_priv_flags = 0;
    auto err =
        constraint_mgr_->GetAllocationData(*buf_des, &ad, &layout, &out_desc, &out_priv_flags);
    if (err != Error::NONE) {
      DLOGE("Invalid allocation - unable to get allocation size");
      return err;
    }
    int width = 0;
    constraint_mgr_->ConvertAlignedWidthFromBytesToPixels(buf_des->format,
                                                          layout.aligned_width_in_bytes, &width);
    *static_cast<uint32_t *>(out_get) = width;
    return Error::NONE;
  } else if (out_get != nullptr) {
    *static_cast<uint32_t *>(out_get) = handle->aligned_width_in_pixels();
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::AlignedHeightInPixelsHelper(SnapMetadata *metadata,
                                                       SnapHandleInternal *handle, void *in_set,
                                                       void *out_get, BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    AllocData ad;
    vendor_qti_hardware_display_common_BufferLayout layout;
    BufferDescriptor out_desc;
    int out_priv_flags = 0;
    auto err =
        constraint_mgr_->GetAllocationData(*buf_des, &ad, &layout, &out_desc, &out_priv_flags);
    if (err != Error::NONE) {
      DLOGE("Invalid allocation - unable to get allocation size");
      return err;
    }
    *static_cast<uint32_t *>(out_get) = layout.aligned_height;
    return Error::NONE;
  } else if (out_get != nullptr) {
    *static_cast<uint32_t *>(out_get) = handle->aligned_height();
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::StandardMetadataStatusHelper(SnapMetadata *metadata,
                                                        SnapHandleInternal *handle, void *in_set,
                                                        void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    std::memcpy(out_get, metadata->isStandardMetadataSet, sizeof(bool) * METADATA_SET_SIZE);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::VendorMetadataStatusHelper(SnapMetadata *metadata,
                                                      SnapHandleInternal *handle, void *in_set,
                                                      void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    std::memcpy(out_get, metadata->isVendorMetadataSet, sizeof(bool) * METADATA_SET_SIZE);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::BufferTypeHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                            void *in_set, void *out_get,
                                            BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<uint32_t *>(out_get) = handle->buffer_type();
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::VideoTSInfoHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                             void *in_set, void *out_get,
                                             BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_VideoTimestampInfo *>(out_get) =
        metadata->videoTsInfo;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->videoTsInfo =
        *static_cast<vendor_qti_hardware_display_common_VideoTimestampInfo *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::CustomDimensionsStrideHelper(SnapMetadata *metadata,
                                                        SnapHandleInternal *handle, void *in_set,
                                                        void *out_get, BufferDescriptor *buf_des) {
  // TODO - can't be returned as pointer
  if (out_get != nullptr) {
    int32_t stride;
    int32_t height;
    if (SnapMetadataManager::GetCustomDimensions(handle, metadata, &stride, &height) == 0) {
      *static_cast<int32_t *>(out_get) = static_cast<int32_t>(stride);
      return Error::NONE;
    } else {
      return Error::BAD_VALUE;
    }
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::CustomDimensionsHeightHelper(SnapMetadata *metadata,
                                                        SnapHandleInternal *handle, void *in_set,
                                                        void *out_get, BufferDescriptor *buf_des) {
  // TODO - can't be returned as pointer
  if (out_get != nullptr) {
    int32_t stride = handle->aligned_width_in_pixels();
    int32_t height = handle->aligned_height();
    if (SnapMetadataManager::GetCustomDimensions(handle, metadata, &stride, &height) == 0) {
      *static_cast<int32_t *>(out_get) = static_cast<int32_t>(height);
      return Error::NONE;
    } else {
      return Error::BAD_VALUE;
    }
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::RGBDataAddressHelper(SnapMetadata *metadata,
                                                SnapHandleInternal *handle,
                                                void *in_set, void *out_get,
                                                BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    void *rgb_data = nullptr;
    Error err = GetRgbDataAddress(handle, &rgb_data);
    if (err == Error::NONE) {
      *static_cast<uint64_t *>(out_get) = reinterpret_cast<uint64_t>(rgb_data);
    }
    return err;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::BufferPermissionHelper(SnapMetadata *metadata,
                                                  SnapHandleInternal *handle, void *in_set,
                                                  void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    vendor_qti_hardware_display_common_BufferPermission *buf_perm =
        reinterpret_cast<vendor_qti_hardware_display_common_BufferPermission *>(out_get);
    int numelems =
        sizeof(metadata->bufferPerm) / sizeof(vendor_qti_hardware_display_common_BufferPermission);
    for (int i = 0; i < numelems; i++) {
      buf_perm[i] = metadata->bufferPerm[i];
    }
    return Error::NONE;
  } else if (in_set != nullptr) {
    vendor_qti_hardware_display_common_BufferPermission *buf_perm =
        reinterpret_cast<vendor_qti_hardware_display_common_BufferPermission *>(in_set);
    int numelems =
        sizeof(metadata->bufferPerm) / sizeof(vendor_qti_hardware_display_common_BufferPermission);
    for (int i = 0; i < numelems; i++) {
      metadata->bufferPerm[i] = buf_perm[i];
    }
    if (mem_allocator_ != nullptr) {
      return mem_allocator_->SetBufferPermission(handle->fd(), &metadata->bufferPerm[0],
                                                 &metadata->memHandle);
    }
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::MemHandleHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                           void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<int64_t *>(out_get) = metadata->memHandle;
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::TimedRenderingHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                                void *in_set, void *out_get,
                                                BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<uint32_t *>(out_get) = metadata->timedRendering;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->timedRendering = *static_cast<uint32_t *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::CustomContentMetadataHelper(SnapMetadata *metadata,
                                                       SnapHandleInternal *handle, void *in_set,
                                                       void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    if (handle->custom_content_md_region_base() == 0 ||
        handle->custom_content_md_reserved_size() !=
            sizeof(vendor_qti_hardware_display_common_CustomContentMetadata)) {
      return Error::UNSUPPORTED;
    } else {
      void *custom_content_metadata_ptr =
          reinterpret_cast<void *>(handle->custom_content_md_region_base());
      memcpy(out_get, custom_content_metadata_ptr,
             sizeof(vendor_qti_hardware_display_common_CustomContentMetadata));
    }
    return Error::NONE;
  } else if (in_set != nullptr) {
    if (handle->custom_content_md_region_base() == 0 ||
        handle->custom_content_md_reserved_size() !=
            sizeof(vendor_qti_hardware_display_common_CustomContentMetadata)) {
      return Error::UNSUPPORTED;
    } else {
      void *custom_content_metadata_ptr =
          reinterpret_cast<void *>(handle->custom_content_md_region_base());
      vendor_qti_hardware_display_common_CustomContentMetadata *c_md_out =
          reinterpret_cast<vendor_qti_hardware_display_common_CustomContentMetadata *>(
              custom_content_metadata_ptr);
      vendor_qti_hardware_display_common_CustomContentMetadata *c_md_in =
          reinterpret_cast<vendor_qti_hardware_display_common_CustomContentMetadata *>(in_set);
      memcpy(c_md_out, c_md_in, sizeof(*c_md_in));
    }
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::VideoTranscodeStatsHelper(SnapMetadata *metadata,
                                                     SnapHandleInternal *handle, void *in_set,
                                                     void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_VideoTranscodeStatsMetadata *>(out_get) =
        metadata->video_transcode_stats;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->video_transcode_stats =
        *static_cast<vendor_qti_hardware_display_common_VideoTranscodeStatsMetadata *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::EarlyNotifyLineCountHelper(SnapMetadata *metadata,
                                                      SnapHandleInternal *handle, void *in_set,
                                                      void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<int32_t *>(out_get) = metadata->videoEarlyNotifyLineCount;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->videoEarlyNotifyLineCount = *static_cast<int32_t *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::ReservedRegionHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                                void *in_set, void *out_get,
                                                BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    vendor_qti_hardware_display_common_ReservedRegion snap_reserved_region;
    snap_reserved_region.size = handle->reserved_size();
    snap_reserved_region.reserved_region_addr.addressPointer = handle->reserved_region_base();
    *static_cast<vendor_qti_hardware_display_common_ReservedRegion *>(out_get) =
        snap_reserved_region;
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::FormatModifierHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                                void *in_set, void *out_get,
                                                BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    *static_cast<uint64_t *>(out_get) = static_cast<uint64_t>(GetPixelFormatModifier(*buf_des));
    return Error::NONE;
  } else if (out_get != nullptr) {
    *static_cast<uint64_t *>(out_get) = handle->pixel_format_modifier();
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::MasteringDisplayHelper(SnapMetadata *metadata,
                                                  SnapHandleInternal *handle, void *in_set,
                                                  void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_QtiMasteringDisplay *>(out_get) =
        metadata->color.masteringDisplayInfo;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->color.masteringDisplayInfo =
        *static_cast<vendor_qti_hardware_display_common_QtiMasteringDisplay *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::ContentLightLevelHelper(SnapMetadata *metadata,
                                                   SnapHandleInternal *handle, void *in_set,
                                                   void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_QtiContentLightLevel *>(out_get) =
        metadata->color.contentLightLevel;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->color.contentLightLevel =
        *static_cast<vendor_qti_hardware_display_common_QtiContentLightLevel *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::DynamicMetadataHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                                 void *in_set, void *out_get,
                                                 BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_QtiDynamicMetadata *>(out_get) =
        metadata->color.dynamicMetadata;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->color.dynamicMetadata =
        *static_cast<vendor_qti_hardware_display_common_QtiDynamicMetadata *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::MatrixCoefficientsHelper(SnapMetadata *metadata,
                                                    SnapHandleInternal *handle, void *in_set,
                                                    void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_QtiMatrixCoEfficients *>(out_get) =
        metadata->color.matrixCoefficients;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->color.matrixCoefficients =
        *static_cast<vendor_qti_hardware_display_common_QtiMatrixCoEfficients *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::ColorRemappingInfoHelper(SnapMetadata *metadata,
                                                    SnapHandleInternal *handle, void *in_set,
                                                    void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_QtiColorRemappingInfo *>(out_get) =
        metadata->color.cRI;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->color.cRI =
        *static_cast<vendor_qti_hardware_display_common_QtiColorRemappingInfo *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::AnamorphicCompressionHelper(SnapMetadata *metadata,
                                                       SnapHandleInternal *handle, void *in_set,
                                                       void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<vendor_qti_hardware_display_common_QtiAnamorphicMetadata *>(out_get) =
        metadata->anamorphic_compression;
    return Error::NONE;
  } else if (in_set != nullptr) {
    metadata->anamorphic_compression =
        *static_cast<vendor_qti_hardware_display_common_QtiAnamorphicMetadata *>(in_set);
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::BaseAddressHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                             void *in_set, void *out_get,
                                             BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    *static_cast<uint64_t *>(out_get) = handle->base();
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::IsUBWCHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                        void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    int64_t is_ubwc = 0;
    if (ubwc_policy_->IsUBWCAlloc(*buf_des)) {
      is_ubwc = 1;
    }
    *static_cast<int64_t *>(out_get) = is_ubwc;
    return Error::NONE;
  } else if (out_get != nullptr) {
    *static_cast<int64_t *>(out_get) =
        static_cast<int64_t>(handle->flags() & PRIV_FLAGS_UBWC_ALIGNED ? 1 : 0);
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::IsTileRenderedHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                                void *in_set, void *out_get,
                                                BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    int64_t is_tile_rendered = 0;
    if (ubwc_policy_->IsUBWCAlloc(*buf_des)) {
      is_tile_rendered = 1;
    }
    if (IsTileRendered(buf_des->format)) {
      is_tile_rendered = 1;
    }
    *static_cast<int64_t *>(out_get) = is_tile_rendered;
    return Error::NONE;
  } else if (out_get != nullptr) {
    *static_cast<int64_t *>(out_get) =
        static_cast<int64_t>(handle->flags() & PRIV_FLAGS_TILE_RENDERED ? 1 : 0);
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::IsCachedHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                          void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (buf_des != nullptr) {
    AllocData ad;
    vendor_qti_hardware_display_common_BufferLayout layout;
    BufferDescriptor out_desc;
    int out_priv_flags = 0;
    auto err =
        constraint_mgr_->GetAllocationData(*buf_des, &ad, &layout, &out_desc, &out_priv_flags);
    if (err != Error::NONE) {
      DLOGE("Invalid allocation - unable to get allocation size");
      return err;
    }
    int64_t is_cached = 0;
    if (ad.uncached) {
      is_cached = 1;
    }
    *static_cast<int64_t *>(out_get) = is_cached;
    return Error::NONE;
  } else if (out_get != nullptr) {
    *static_cast<int64_t *>(out_get) =
        static_cast<int64_t>(handle->flags() & PRIV_FLAGS_CACHED ? 1 : 0);
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

Error SnapMetadataManager::HeapNameHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                          void *in_set, void *out_get, BufferDescriptor *buf_des) {
  if (out_get != nullptr) {
    std::string name(metadata->heapName);
    *(reinterpret_cast<std::string *>(out_get)) = name;
    return Error::NONE;
  } else if (in_set != nullptr) {
    return Error::UNSUPPORTED;
  }
  return Error::BAD_VALUE;
}

uint64_t SnapMetadataManager::GetMetaDataSize(uint64_t reserved_region_size,
                                              uint64_t custom_content_md_region_size) {
  return static_cast<uint64_t>(ROUND_UP_PAGESIZE(sizeof(SnapMetadata) + reserved_region_size +
                                                 custom_content_md_region_size));
}

uint32_t SnapMetadataManager::GetCustomContentMetadataSize(
    vendor_qti_hardware_display_common_PixelFormat format,
    vendor_qti_hardware_display_common_BufferUsage usage) {
  if (IsYuv(format) && (usage & vendor_qti_hardware_display_common_BufferUsage::VIDEO_DECODER ||
                        usage & vendor_qti_hardware_display_common_BufferUsage::VIDEO_ENCODER ||
                        usage & vendor_qti_hardware_display_common_BufferUsage::CAMERA_OUTPUT)) {
    return sizeof(vendor_qti_hardware_display_common_CustomContentMetadata);
  }
  return 0;
}

Error SnapMetadataManager::InitializeMetadata(
    SnapHandleInternal *hnd, vendor_qti_hardware_display_common_PixelFormat pixel_format_requested,
    BufferDescriptor out_desc, const AllocData ad,
    vendor_qti_hardware_display_common_BufferLayout *layout) {
  bool ubwc_enable = ubwc_policy_->IsUBWCAlloc(out_desc);
  auto err = Error::NONE;

  GraphicsConstraintProvider *graphics_provider = GraphicsConstraintProvider::GetInstance();
  CapabilitySet caps;
  graphics_provider->GetCapabilities(out_desc, &caps);
  if (caps.enabled) {
    vendor_qti_hardware_display_common_GraphicsMetadata graphics_metadata;
    int ret = graphics_provider->GetInitialMetadata(out_desc, &graphics_metadata, ubwc_enable);

    if (ret == 0) {
      err = Set(hnd, vendor_qti_hardware_display_common_MetadataType::GRAPHICS_METADATA,
                &graphics_metadata);
      if (err != Error::NONE) {
        DLOGE("Error initializing graphics metadata - ret val %d", ret);
      }
    } else {
      DLOGE("Failed to get graphics metadata - retval %d", ret);
    }
  } else {
    DLOGD_IF(enable_logs,
             "Graphics does not support format %d. Skipping initialization of graphics metadata",
             static_cast<uint64_t>(out_desc.format));
  }

  // This metadata types cannot be changed via set API
  // to avoid changing post-allocation

  err = ValidateAndMap(hnd);
  if (err != Error::NONE) {
    DLOGE("%s: ValidateAndMap failed - unable to set name", __FUNCTION__);
    return Error::UNSUPPORTED;
  }

  SnapMetadata *data = reinterpret_cast<SnapMetadata *>(hnd->base_metadata());
  if (data == nullptr) {
    DLOGE("%s: Invalid metadata address", __FUNCTION__);
    return Error::BAD_BUFFER;
  }

  // Populate name
  auto name_length = std::min(std::string(out_desc.name).size(), static_cast<size_t>(QTI_MAX_NAME_LEN - 1));
  memcpy(data->name, std::string(out_desc.name).data(), name_length);
  data->name[name_length] = '\0';

  // Populate Buffer Layout
  memcpy(&data->buffer_layout, layout, sizeof(vendor_qti_hardware_display_common_BufferLayout));

  // Populate Crop
  data->crop.top = 0;
  data->crop.left = 0;
  data->crop.right = static_cast<int32_t>(hnd->aligned_width_in_pixels());
  data->crop.bottom = static_cast<int32_t>(hnd->aligned_height());

  // Populate reserved region
  data->reservedSize = std::min(static_cast<uint64_t>(hnd->reserved_size()),
                                static_cast<uint64_t>(RESERVED_REGION_SIZE));

  // Populate heap name
  auto heap_name_length = std::min(ad.heap_name.size(), static_cast<size_t>(QTI_MAX_NAME_LEN - 1));
  memcpy(data->heapName, ad.heap_name.data(), heap_name_length);
  data->heapName[heap_name_length] = '\0';

  // Populate pixel format requested
  data->pixel_format_requested = pixel_format_requested;

  UnmapAndReset(hnd);
  return Error::NONE;
}

Error SnapMetadataManager::GetRgbDataAddress(SnapHandleInternal *hnd,
                                             void **rgb_data) {
  // This api is only for rgb formats
  if (!IsRgb(hnd->format())) {
    return Error::BAD_VALUE;
  }
  // linear buffer, nothing to do further [base addr will have plane address]
  if (!(hnd->flags() & PRIV_FLAGS_UBWC_ALIGNED)) {
    *rgb_data = reinterpret_cast<void *>(hnd->base());
    return Error::NONE;
  }
  // Ubwc buffer - which has meta planes
  // Get the buffer layout from metadata
  SnapMetadata *data = reinterpret_cast<SnapMetadata *>(hnd->base_metadata());
  if (data == nullptr) {
    DLOGE("%s: Invalid metadata address", __FUNCTION__);
    return Error::BAD_VALUE;
  }
  unsigned int plane_layout_size = data->buffer_layout.planes[0].size_in_bytes;
  *rgb_data = reinterpret_cast<void *>(hnd->base() + plane_layout_size);
  return Error::NONE;
}

int GetDataAddress(SnapHandleInternal *hnd, uint64_t *data_addr) {
  int err = 0;

  // linear buffer, nothing to do further [base addr will have plane address]
  if (!(hnd->flags() & PRIV_FLAGS_UBWC_ALIGNED)) {
    *data_addr = hnd->base();
    return err;
  }
  // Ubwc buffer - which has meta planes
  // Get the buffer layout from metadata
  SnapMetadata *data = reinterpret_cast<SnapMetadata *>(hnd->base_metadata());
  if (data == nullptr) {
    DLOGE("%s: Invalid metadata address", __FUNCTION__);
    return Error::BAD_BUFFER;
  }

  unsigned int plane_layout_size = data->buffer_layout.planes[0].size_in_bytes;
  *data_addr = hnd->base() + plane_layout_size;
  return err;
}

Error SnapMetadataManager::GetCustomDimensions(SnapHandleInternal *hnd, SnapMetadata *metadata,
                                               int32_t *stride, int32_t *height) {
  int32_t interlaced = 0;
  *stride = hnd->aligned_width_in_pixels();
  *height = hnd->aligned_height();
  if (metadata->isStandardMetadataSet[GET_STANDARD_METADATA_STATUS_INDEX(
          (int64_t)vendor_qti_hardware_display_common_MetadataType::CROP)]) {
    *stride = metadata->crop.right;
    *height = metadata->crop.bottom;
  } else if (metadata->isVendorMetadataSet[GET_VENDOR_METADATA_STATUS_INDEX(
                 (int64_t)vendor_qti_hardware_display_common_MetadataType::PP_PARAM_INTERLACED)]) {
    interlaced = metadata->interlaced;
    if (interlaced) {
      AllocData ad;
      vendor_qti_hardware_display_common_BufferLayout layout;
      BufferDescriptor out_desc;
      int out_priv_flags = 0;
      // TODO (user) : Add desc modifier support
      BufferDescriptor desc = {.format = hnd->format(),
                               .usage = hnd->usage(),
                               .width = hnd->aligned_width_in_pixels(),
                               .height = hnd->aligned_height(),
                               .layerCount = static_cast<int32_t>(hnd->layer_count()),
                               .reservedSize = hnd->reserved_size()};
      auto err = Error::NONE;
      err = constraint_mgr_->GetAllocationData(desc, &ad, &layout, &out_desc, &out_priv_flags);
      if (err != Error::NONE) {
        DLOGE("Invalid allocation - unable to get allocation size");
        return err;
      }
      *stride = static_cast<int>(layout.aligned_width_in_bytes / layout.bpp);
      *height = static_cast<int>(layout.aligned_height);
    }
  }
  return Error::NONE;
}

int SnapMetadataManager::GetDRMFormat(vendor_qti_hardware_display_common_PixelFormat format,
                                      vendor_qti_hardware_display_common_BufferUsage usage,
                                      int flags, uint32_t *drm_format,
                                      uint64_t *drm_format_modifier) {
  vendor_qti_hardware_display_common_Compression qti_compression =
      ubwc_policy_->GetUBWCScheme(format, usage);
  SnapFormatUsageDescriptor format_usage_desc = {.format = format,
                                                 .compression_type = qti_compression};
  if (snap_to_drm_format_.find(format_usage_desc) != snap_to_drm_format_.end()) {
    DRMFormatDescriptor drm_descriptor = snap_to_drm_format_.at(format_usage_desc);
    *drm_format = drm_descriptor.drm_format;
    *drm_format_modifier = drm_descriptor.drm_modifier;
  }
  return 0;
}

Error SnapMetadataManager::IsMetadataTypeSettable(
    vendor_qti_hardware_display_common_MetadataType type, bool *out) {
  *out = false;
  auto metadatatype_traits = metadatatype_traits_map.find(type);
  if (metadatatype_traits != metadatatype_traits_map.end()) {
    if (metadatatype_traits->second.is_settable) {
      *out = true;
    }
  } else {
    DLOGW("Metadata type %d not found in metadatatype traits map", static_cast<int>(type));
    return Error::BAD_VALUE;
  }
  return Error::NONE;
}

Error SnapMetadataManager::Get(SnapHandleInternal *hnd,
                               vendor_qti_hardware_display_common_MetadataType type, void *out) {
  if (!out) {
    DLOGE("%s: Invalid output parameter", __FUNCTION__);
    return Error::UNSUPPORTED;
  }

  auto ret = Error::BAD_VALUE;

  SnapMetadata *metadata = reinterpret_cast<SnapMetadata *>(hnd->base_metadata());
  if (metadata == nullptr) {
    DLOGE("%s: Invalid metadata address", __FUNCTION__);
    return ret;
  }

  if (metadata_helper_function_map.find(type) != metadata_helper_function_map.end()) {
    MetadataHelper metadata_helper_func = metadata_helper_function_map[type];
    auto err =
        ((this->*metadata_helper_func)(metadata, hnd, nullptr, out, nullptr));
    return err;
  } else {
    return Error::UNSUPPORTED;
  }
}

Error SnapMetadataManager::ValidateAndMap(SnapHandleInternal *hnd) {
  if (hnd->fd_metadata() < 0) {
    DLOGE("%s: Snap handle has invalid metadata fd : %d", __FUNCTION__, hnd->fd_metadata());
    return Error::BAD_BUFFER;
  }

  if (!hnd->base_metadata()) {
    uint64_t reserved_region_size = hnd->reserved_size();
    uint64_t custom_content_md_reserved_size = hnd->custom_content_md_reserved_size();
    DLOGD_IF(enable_logs, "from handle - reserved size %lu custom content metadata size %lu",
             reserved_region_size, custom_content_md_reserved_size);
    uint64_t size = GetMetaDataSize(reserved_region_size, custom_content_md_reserved_size);
    void *base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, hnd->fd_metadata(), 0);
    if (base == reinterpret_cast<void *>(MAP_FAILED)) {
      DLOGE("%s: mmap failed - err %s", __FUNCTION__, strerror(errno));
      return Error::BAD_BUFFER;
    }
    hnd->base_metadata() = (uintptr_t)base;  // NOLINT
    DLOGD_IF(enable_logs, "Successfully mapped metadata %p", hnd->base_metadata());
  }
  return Error::NONE;
}

void SnapMetadataManager::UnmapAndReset(SnapHandleInternal *hnd) {
  if (hnd->base_metadata()) {
    munmap(reinterpret_cast<void *>(hnd->base_metadata()),
           GetMetaDataSize(hnd->reserved_size(), hnd->custom_content_md_reserved_size()));
    hnd->base_metadata() = 0;
  }
}

Error SnapMetadataManager::Set(SnapHandleInternal *hnd,
                               vendor_qti_hardware_display_common_MetadataType type, void *in) {
  auto err = ValidateAndMap(const_cast<SnapHandleInternal *>(hnd));
  if (err != 0) {
    DLOGE("%s: ValidateAndMap failed", __FUNCTION__);
    return Error::UNSUPPORTED;
  }
  SnapMetadata *metadata = reinterpret_cast<SnapMetadata *>(hnd->base_metadata());
  if (metadata == nullptr) {
    DLOGE("%s: Invalid metadata address", __FUNCTION__);
    return Error::UNSUPPORTED;
  }

  // If parameter is NULL reset the specific MetaData Key
  if (!in) {
    SetMetadataState(metadata, type, false);
    switch (type) {
      case vendor_qti_hardware_display_common_MetadataType::VIDEO_PERF_MODE:
        metadata->isVideoPerfMode = 0;
        break;
      case vendor_qti_hardware_display_common_MetadataType::CVP_METADATA:
        metadata->cvpMetadata.size = 0;
        break;
      case vendor_qti_hardware_display_common_MetadataType::VIDEO_HISTOGRAM_STATS:
        metadata->video_histogram_stats.stat_len = 0;
        break;
      case vendor_qti_hardware_display_common_MetadataType::VIDEO_TRANSCODE_STATS:
        metadata->video_transcode_stats.stat_len = 0;
        break;
      default:
        DLOGE("Input is null when setting metadata type %d", type);
        break;
    }
    return Error::NONE;
  }

  if (metadata_helper_function_map.find(type) != metadata_helper_function_map.end()) {
    SetMetadataState(metadata, type, true);
    MetadataHelper metadata_helper_func = metadata_helper_function_map[type];
    return ((this->*metadata_helper_func)(metadata, hnd, in, nullptr, nullptr));
  } else {
    DLOGE("%s Unable to find the metadata type %d", static_cast<int>(type));
    return Error::UNSUPPORTED;
  }
}

void SnapMetadataManager::SetMetadataState(SnapMetadata *metadata,
                                           vendor_qti_hardware_display_common_MetadataType type,
                                           bool metadata_state) {
  int metadata_type = static_cast<int>(type);
  if (IS_VENDOR_METADATA_TYPE(metadata_type)) {
    if (GET_VENDOR_METADATA_STATUS_INDEX(metadata_type) < METADATA_SET_SIZE) {
      metadata->isVendorMetadataSet[GET_VENDOR_METADATA_STATUS_INDEX(metadata_type)] =
          metadata_state;
    }
  } else {
    if (GET_STANDARD_METADATA_STATUS_INDEX(metadata_type) < METADATA_SET_SIZE) {
      metadata->isStandardMetadataSet[GET_STANDARD_METADATA_STATUS_INDEX(metadata_type)] =
          metadata_state;
    }
  }
}

bool SnapMetadataManager::GetMetadataStateInternal(SnapMetadata *metadata,
                                           vendor_qti_hardware_display_common_MetadataType type) {
  int metadata_type = static_cast<int>(type);

  if (IS_VENDOR_METADATA_TYPE(metadata_type)) {
    if (GET_VENDOR_METADATA_STATUS_INDEX(metadata_type) < METADATA_SET_SIZE) {
      return metadata->isVendorMetadataSet[GET_VENDOR_METADATA_STATUS_INDEX(metadata_type)];
    }
  } else {
    if (GET_STANDARD_METADATA_STATUS_INDEX(metadata_type) < METADATA_SET_SIZE) {
      return metadata->isStandardMetadataSet[GET_STANDARD_METADATA_STATUS_INDEX(metadata_type)];
    }
  }
  return false;
}


Error SnapMetadataManager::GetMetadataState(SnapHandleInternal *hnd,
                               vendor_qti_hardware_display_common_MetadataType type, bool *out) {
  if (!out) {
    DLOGE("%s: Invalid output parameter", __FUNCTION__);
    return Error::UNSUPPORTED;
  }

  auto ret = Error::BAD_VALUE;

  SnapMetadata *metadata = reinterpret_cast<SnapMetadata *>(hnd->base_metadata());
  if (metadata == nullptr) {
    DLOGE("%s: Invalid metadata address", __FUNCTION__);
    return ret;
  }

  bool is_settable = false;
  ret = IsMetadataTypeSettable(type, &is_settable);
  if (ret == Error::BAD_VALUE) {
    *out = false;
    return ret;
  } else if (is_settable && !GetMetadataStateInternal(metadata, type)) {
    *out = false;
    return Error::NONE;
  }

  *out = true;
  return Error::NONE;
}

Error SnapMetadataManager::GetFromBufferDescriptor(
    BufferDescriptor desc, vendor_qti_hardware_display_common_MetadataType type, void *out) {
  if (!out) {
    DLOGE("%s: Invalid output parameter", __FUNCTION__);
    return Error::UNSUPPORTED;
  }
  if (metadata_helper_function_map.find(type) != metadata_helper_function_map.end()) {
    MetadataHelper metadata_helper_func = metadata_helper_function_map[type];
    return ((this->*metadata_helper_func)(nullptr, nullptr, nullptr, out, &desc));
  } else {
    return Error::UNSUPPORTED;
  }
}

Error SnapMetadataManager::DumpBuffer(SnapHandleInternal *hnd) {
  (void)hnd;
  return Error::UNSUPPORTED;
}

Error SnapMetadataManager::DumpBuffers() {
  return Error::UNSUPPORTED;
}

}  // namespace snapalloc
