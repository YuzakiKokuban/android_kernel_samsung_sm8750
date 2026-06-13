/*
*Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __QRTR_CLIENT_INTERFACE_H__
#define __QRTR_CLIENT_INTERFACE_H__

#include<string>

namespace sdm {

#define QRTR_CLIENT_LIB_NAME "libqrtrclient.so"

#define CREATE_QRTR_CLIENT_INTERFACE_NAME "CreateQrtrClientInterface"
#define DESTROY_QRTR_CLIENT_INTERFACE_NAME "DestroyQrtrClientInterface"

class QRTRClientInterface;
class QRTRCallbackInterface;

typedef struct {
  int server_id = -1;
  int server_version = 1;
  int server_instance = 1;
} QRTRConfig;

typedef int (*CreateQrtrClientIntf)(const QRTRConfig &qrtr_config, QRTRCallbackInterface *callback,
                                    QRTRClientInterface **qrtr_client_intf);
typedef int (*DestroyQrtrClientIntf)(QRTRClientInterface *qrtr_client_intf);

class QRTRCallbackInterface {
 public:
  virtual void OnServerReady() = 0;
  virtual void OnServerExit() = 0;
  virtual int OnResponse(void *rsp, size_t cmd_size) = 0;

 protected:
  virtual ~QRTRCallbackInterface() {};
};

class QRTRClientInterface {
 public:
  virtual int SendCommand(void *cmd, size_t cmd_size) = 0;
 protected:
  virtual ~QRTRClientInterface() {};
};

}  // namespace sdm
#endif
