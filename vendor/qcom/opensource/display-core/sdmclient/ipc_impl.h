/*
 *Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the
 * following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __IPC_IMPL_H__
#define __IPC_IMPL_H__

#include <core/ipc_interface.h>
#include <thread>

#include "membuf_wrapper.h"
#include "qrtr_client_interface.h"
#include "utils/sys.h"
#include "vm_interface.h"
#include "sdm_compositor_callbacks.h"

namespace sdm {

#define MEMBUF_CLIENT_LIB_NAME "libmemutils.so"

#define CREATE_MEMBUF_INTERFACE_NAME "CreateMemBufInterface"
#define DESTROY_MEMBUF_INTERFACE_NAME "DestroyMemBufInterface"

typedef int (*GetMemBufInterface)(MemBuf **mem_buf_hnd);
typedef int (*PutMemBufInterface)();

class IPCImpl : public IPCIntf, QRTRCallbackInterface {
public:
  virtual ~IPCImpl(){};
  IPCImpl(SDMCompositorCallbacks *cb) : cb_(cb) {}
  int Init();
  int Deinit();
  int SetParameter(IPCParams param, const GenericPayload &in);
  int GetParameter(IPCParams param, GenericPayload *out);
  int ProcessOps(IPCOps op, const GenericPayload &in, GenericPayload *out);
  void OnServerReady();
  void OnServerExit();
  int OnResponse(void *rsp_buf, size_t rsp_size);
  static void SpawnOnServerReady(int client_id);

private:
  int ProcessExportBuffers(const GenericPayload &in, GenericPayload *out);
  static DynLib qrtr_client_lib_;
  static CreateQrtrClientIntf create_qrtr_client_intf_;
  static DestroyQrtrClientIntf destroy_qrtr_client_intf_;
  static QRTRClientInterface *qrtr_client_intf_;
  bool init_done_ = false;
  static std::mutex vm_lock_;
  static int client_id_;
  static bool server_ready_;
  static std::map<int, IPCVmCallbackIntf *> callbacks_;
  static MemBuf *mem_buf_;
  static DynLib mem_buf_client_lib_;
  static GetMemBufInterface GetMemBuf;
  static PutMemBufInterface PutMembuf;
  SDMCompositorCallbacks *cb_ = nullptr;
};
} // namespace sdm

#endif
