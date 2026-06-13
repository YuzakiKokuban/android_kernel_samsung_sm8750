/*
* Copyright (c) 2015 - 2018, 2021 The Linux Foundation. All rights reserved.
*
* Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above
*    copyright notice, this list of conditions and the following
*    disclaimer in the documentation and/or other materials provided
*    with the distribution.
*  * Neither the name of The Linux Foundation nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
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

/*! @file buffer_allocator.h
  @brief Interface file for platform specific buffer allocator.

  @details This interface is used by SDM to allocate internal buffers.
*/

#ifndef __BUFFER_ALLOCATOR_H__
#define __BUFFER_ALLOCATOR_H__

#include <errno.h>
#include <cstddef>
#include <map>
#include <bitset>

#include "layer_buffer.h"
#include <SnapHandle.h>
#include <MetadataType.h>

namespace sdm {

using SnapHandle = vendor::qti::hardware::display::snapalloc::SnapHandle;
using QtiDataspace = vendor_qti_hardware_display_common_Dataspace;

/*! @brief Input configuration set by the client for buffer allocation.

  @sa BufferInfo::BufferConfig
*/

enum BufferClient {
  kBufferClientDPU,
  kBufferClientUnTrustedVM,
  kBufferClientTrustedVM,
  kBufferClientMax,
};

enum BufferPerm {
  kBufferPermRead = 0,
  kBufferPermWrite = 1,
  kBufferPermExecute = 2,
  kBufferPermMax,
};

typedef std::map<BufferClient, std::bitset<kBufferPermMax>> BufferAccessControlMap;

struct BufferConfig {
  uint32_t width = 0;                         //!< Specifies buffer width for buffer allocation.
  uint32_t height = 0;                        //!< Specifies buffer height for buffer allocation.
  LayerBufferFormat format = kFormatInvalid;  //!< Specifies buffer format for buffer allocation.
  uint32_t buffer_count = 0;                  //!< Specifies number of buffers to be allocated.
  bool secure = false;                        //!< Specifies buffer to be allocated from
                                              //!< secure region.
  bool cache = false;                         //!< Specifies whether the buffer needs to be cache.
  bool secure_camera = false;                 //!< Specifies buffer to be allocated from specific
                                              //!< secure heap and with a specific alignment.
  bool gfx_client = false;                    //!< Specifies whether buffer is used by gfx.
  bool trusted_ui = false;                    //!< Specifies buffer to be allocated from non-secure
                                              //!< contiguous memory.
  bool tui_demura = false;                    //!< Specifies if it is demura feature in tui
  BufferAccessControlMap access_control;      //!< Specifies the access permission for this buffer

  bool operator!=(const BufferConfig& config) const {
    return width != config.width || height != config.height || format != config.format ||
           secure != config.secure || cache != config.cache ||
           secure_camera != config.secure_camera || gfx_client != config.gfx_client ||
           tui_demura != config.tui_demura || trusted_ui != config.trusted_ui;
  }
};

/*! @brief Holds the information about the allocated buffer.

  @sa BufferAllocator::AllocateBuffer
  @sa BufferAllocator::FreeBuffer
  @sa BufferAllocator::GetAllocatedBufferInfo
*/
struct AllocatedBufferInfo {
  int fd = -1;                   //!< Specifies the fd of the allocated buffer.
  uint32_t stride = 0;           //!< Specifies allocated buffer stride in bytes.
  uint32_t aligned_width = 0;    //!< Specifies aligned allocated buffer width in pixels.
  uint32_t aligned_height = 0;   //!< Specifies aligned allocated buffer height in pixels.
  LayerBufferFormat format = kFormatInvalid;  // Specifies buffer format for allocated buffer.
  uint32_t size = 0;             //!< Specifies the size of the allocated buffer.
  uint64_t id = 0;               //!< Specifies the Id of the allocated buffer.
  uint64_t usage = 0;            //!< Specifies usage flags of the allocated buffer.
  int64_t mem_handle = -1;        //!< Specifies the exported mem handle of an allocated buffer
                                 //!< to other VMs.mem_handle contains > zero value
                                 //!< if exported successfully to any VM otherwise -1.
};

/*! @brief Holds the information about the input/output configuration of an output buffer.

  @sa BufferAllocator::AllocateBuffer
  @sa BufferAllocator::FreeBuffer
*/
struct BufferInfo {
  BufferConfig buffer_config;             //!< Specifies configuration of a buffer to be allocated.
  AllocatedBufferInfo alloc_buffer_info;  //!< Specifies buffer information of allocated buffer.

  void *private_data = NULL;              //!< Pointer to private data.
};

/*! @brief Buffer allocator implemented by the client

  @details This class declares prototype for BufferAllocator methods which must be
  implemented by the client. Buffer manager in display manager will use these methods to
  allocate/deallocate buffers for display manager.

  @sa CoreInterface::CreateCore
*/
class BufferAllocator {
 public:
  /*! @brief Method to allocate ouput buffer for the given input configuration.

    @details This method allocates memory based on input configuration.

    @param[in] buffer_info \link BufferInfo \endlink

    @return \link int \endlink
  */
  virtual int AllocateBuffer(BufferInfo *buffer_info) = 0;


  /*! @brief Method to deallocate the ouput buffer.

    @details This method deallocates the memory allocated using AllocateBuffer method.

    @param[in] buffer_info \link BufferInfo \endlink

    @return \link int \endlink
  */
  virtual int FreeBuffer(BufferInfo *buffer_info) = 0;


  /*! @brief Method to get the buffer size.

    @details This method returns buffer size for a specific configuration mentioned in buffer info.

    @param[in] buffer_info \link BufferInfo \endlink

    @return \link unsigned int \endlink
  */
  virtual uint32_t GetBufferSize(BufferInfo *buffer_info) = 0;

  /*! @brief Method to Get the AllocatedBufferInfo only.

    @details This method populates the AllocatedBufferInfo as per the configuration in BufferInfo,
    but fd will be invalid.

    @param[in] buffer_info \link BufferInfo \endlink

    @param[out] allocated_buffer_info \link AllocatedBufferInfo \endlink

    @return \link int \endlink) = 0;
  */
  virtual int GetAllocatedBufferInfo(const BufferConfig &buffer_config,
                                              AllocatedBufferInfo *allocated_buffer_info) = 0;

  /*
     @brief  Retuns a buffer's layout in terms of number of planes, stride and offset of each plane

     @details This method returns the layout of the given buffer based on its number of planes,
     stride, and offset of each plane.

     @param[in] AllocatedBufferInfo with a valid aligned width, aligned height, SDM format

     @param[out] stride for each plane, offset of each plane from base, number of planes
   */
  virtual int GetBufferLayout(const AllocatedBufferInfo &buf_info,
                                       uint32_t stride[4], uint32_t offset[4],
                                       uint32_t *num_planes) {
    (void) buf_info;
    (void) stride;
    (void) offset;
    (void) num_planes;
    return -ENOTSUP; }

  // callbacks from sdmclient
  virtual int SetBufferInfo(LayerBufferFormat format, int *target, uint64_t *flags) = 0;
  virtual int GetAlignedWidthAndHeight(int width, int height, int format, uint32_t alloc_type,
                                        int *aligned_width, int *aligned_height) = 0;
  virtual bool GetSDMColorSpace(const int int_dataspace, QtiDataspace *dataspace) = 0;
  virtual LayerBufferFormat GetSDMFormat(const int32_t &source, const int32_t flags,
                                         const int64_t compression_type) = 0;
  virtual DisplayError ColorMetadataToDataspace(Dataspace ds, uint32_t *int_dataspace) = 0;
  virtual int32_t TranslateFromLegacyDataspace(const int32_t &legacy_ds) = 0;

 protected:
  virtual ~BufferAllocator() { }
};

}  // namespace sdm

#endif  // __BUFFER_ALLOCATOR_H__

