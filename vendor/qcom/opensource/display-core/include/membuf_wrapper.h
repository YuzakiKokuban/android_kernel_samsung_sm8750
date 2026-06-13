/*
* Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __MEMBUF_WRAPPER_H__
#define __MEMBUF_WRAPPER_H__

#include <stdio.h>

#include <mutex>
#include <bitset>
#include <map>

using std::mutex;

struct UsageHints {
  union {
    struct {
      uint32_t trusted_ui : 1;  //!< Transfer memory from secure display heap to TUI carveout heap
                                //!<.when this bit is set
      uint32_t tui_demura : 2;  //!< Transfer memory from secure display heap to demura carveout
                                //!<heap. when this bit is set
    };
    uint64_t hints;
  };
};

enum VmType {
  kVmTypePrimary,
  kVmTypeTrusted,
  kVmTypeCpPixel,
  kVmTypeMax,
};

enum VmPermission {
  kVmPermissionRead,
  kVmPermissionWrite,
  kVmPermissionExecute,
  kVmPermissionMax,
};

typedef std::map<VmType, std::bitset<kVmPermissionMax>> VmParams;

class MemBuf {
 public:
  /*! @brief Allocate and transfer a chunk of memory from src VM to dst VM

    @param[in] hints - \link UsageHints \endlink
    @param[in] size - Size of the memory to be trasferred to destination VM

    @param[out] heap_fd - A file descriptor representing the memory that was allocated
    from the source VM and added to the current VM. Calling close() on this file
    descriptor will deallocate the memory from the current VM, and return it to the source VM

    @return 0 on success otherwise errno
  */
  virtual int TransferHeap(const UsageHints &hints, uint32_t size, int *heap_fd) = 0;

  /*! @brief Return the memory to the src VM

    @param[out] heap_fd - A file descriptor representing the memory to be closed

    @return 0 on success otherwise errno
  */
  virtual void ReturnHeap(int heap_fd) = 0;

  /*! @brief Export the buffer fd from primary VM to trusted VM. The buffer must not be mmap'ed by
      any process prior to invoking this function. The buffer must also be a cached buffer from a
      non-secure contiguous heap

    @param[in] buf_fd - Allocated dma/ion buffer fd to be exported
    @param[out] export_fd - An fd that corresponds to the buffer that was exported. This fd
    must be kept open until it is no longer required to export the memory to another VM.

    @param[out] memparcel_hdl -The handle associated with the memparcel that was created by
    granting access to the buffer fd for the trusted VM

    @return 0 on success otherwise errno
  */
  virtual int Export(int buf_fd, int *export_fd, int64_t *memparcel_hdl) = 0;

  /*! @brief Allocate and transfer a chunk of memory from primary VM to secondary VM

    @param[in] memparcel_hdl - The handle that corresponds to the memparcel we are importing.
    @param[out] import_fd - file descriptor of the buffer that the dst VM can use to
    access the buffer. This fd must be closed to release the memory.

    @return 0 on success otherwise errno
  */
  virtual int Import(int64_t memparcel_hdl, int *import_fd) = 0;

  /*! @brief Export the buffer fd from one VM to other VM. The buffer must not be mmap'ed by any
      process prior to invoking this function.

    @param[in] buf_fd - Allocated buffer fd to be exported
    @param[in] vm_param - List of vm type where the buffer to be assigned and its corresponding
                           permission bits.
    @param[in] share - Allocated buffer to be lend or shared to other VM

    @param[out] memparcel_hdl -The handle associated with the memparcel that was created by
    granting access to the buffer fd for the dst VM

    @return 0 on success otherwise errno
  */
  virtual int Export(int buf_fd, const VmParams &vm_params, bool share, int64_t *memparcel_hdl) = 0;

  /*! @brief Allocate and transfer a chunk of memory from src VM to dst VM

    @param[in] memparcel_hdl - The handle that corresponds to the memparcel we are importing.
    @param[in] vm_params - List of vm type where the buffer to be assigned and its corresponding
                           permission. It should be the same parameter passed while exporting.
    @param[out] import_fd - file descriptor of the buffer that the dst VM can use to
    access the buffer. This fd must be closed to release the memory.

    @return 0 on success otherwise errno
  */
  virtual int Import(int64_t memparcel_hdl, const VmParams &vm_params, int *import_fd) = 0;

  /*! @brief Allocate a file descriptor which represents a suggestion from
      userspace that a large amount of memory will be needed in the near
      future. If the kernel decides to honor this suggestion, there is no
      guarantee of exclusive access to this memory by the requesting process
      Closing the file descriptor indicates the memory is no longer required.

    @param[in] size - The requested size of memory to be made free.
    @param[in] name - A unique name for debug purposes.

    @return fd on success otherwise errno
  */

  virtual int MemorySizeHint(uint64_t size, const std::string& name) = 0;
  virtual ~MemBuf() { }

  static int GetInstance(MemBuf **mem_buf_hnd);
  static void PutInstance();
};

#endif  // __MEMBUF_WRAPPER_H__
