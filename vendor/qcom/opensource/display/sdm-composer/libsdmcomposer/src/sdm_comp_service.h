/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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

/*
Changes from Qualcomm Innovation Center are provided under the following license:
Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted (subject to the limitations in the
disclaimer below) provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __SDM_COMP_SERVICE_H__
#define __SDM_COMP_SERVICE_H__

#include <stdint.h>

#include "vm_interface.h"
#include "libqrtr.h"
#include "membuf_wrapper.h"
#include "sdm_comp_service_intf.h"
#include "utils/sys.h"
#include "sdm_comp_service_extn_intf.h"
#include "sdm_comp_interface.h"

#include <mutex>
#include <vector>
#include <thread>
#include <map>

using std::mutex;

namespace sdm {

typedef int (*QrtrOpen)(int rport);
typedef void (*QrtrClose)(int sock);
typedef int (*QrtrSendTo)(int sock, uint32_t node, uint32_t port, const void *data,
                          unsigned int sz);
typedef int (*QrtrPublish)(int sock, uint32_t service, uint16_t version, uint16_t instance);
typedef int (*QrtrBye)(int sock, uint32_t service, uint16_t version, uint16_t instance);
typedef int (*QrtrDecode)(struct qrtr_packet *dest, void *buf, size_t len,
                          const struct sockaddr_qrtr *sq);

class SDMCompService {
 public:
  static int RegisterCallback(SDMCompServiceCbIntf *callback);
  static int UnRegisterCallback(SDMCompServiceCbIntf *callback);

 private:
  static void QRTREventHandler();
  static void CommandHandler(const struct qrtr_packet &qrtr_pkt);
  static void SendResponse(int node, int port, const Response &rsp);
  static void HandleImportDemuraBuffers(const struct qrtr_packet &qrtr_pkt, Response *rsp);
  static void HandleSetBacklight(const struct qrtr_packet &qrtr_pkt, Response *rsp);
  static void HandleSetDisplayConfigs(const struct qrtr_packet &qrtr_pkt, Response *rsp);
  static void HandleSetProperties(const struct qrtr_packet &qrtr_pkt, Response *rsp);
  static void HandleSetPanelBootParams(const struct qrtr_packet &qrtr_pkt, Response *rsp);
  static SDMCompDisplayType GetSDMCompDisplayType(DisplayType disp_type);
  static void HandlePendingCommands();
  static bool IsRegisteredClientValid();

  static std::mutex qrtr_lock_;
  static QrtrOpen qrtr_open_;
  static QrtrClose qrtr_close_;
  static QrtrSendTo qrtr_send_to_;
  static QrtrPublish qrtr_publish_;
  static QrtrBye qrtr_bye_;
  static QrtrDecode qrtr_decode_;
  static std::vector <SDMCompServiceCbIntf *> callbacks_;
  static int qrtr_fd_;
  static DynLib qrtr_lib_;
  static MemBuf *mem_buf_;
  static DynLib extension_lib_;
  static CreateSDMCompExtnIntf create_sdm_comp_extn_intf_;
  static DestroySDMCompExtnIntf destroy_sdm_comp_extn_intf_;
  static SDMCompServiceExtnIntf *sdm_comp_service_extn_intf_;
  static int exit_thread_fd_;
  static std::thread event_thread_;
  static std::mutex pending_cmd_lock_;
  static std::multimap<int, struct qrtr_packet> pending_commands_;
};

}  // namespace sdm

#endif  // __SDM_COMP_SERVICE_H__

