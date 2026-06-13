/*=============================================================================
  Copyright (c) 2016, The Linux Foundation. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:
  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials provided
    with the distribution.
  * Neither the name of The Linux Foundation nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  =============================================================================

  Changes from Qualcomm Innovation Center are provided under the following license:
  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause-Clear

  =============================================================================*/

#ifndef __IZATREMOTEAPIS_H__
#define __IZATREMOTEAPIS_H__

#include <izat_remote_api.h>
#include <string>
#include <stdio.h>
namespace qc_loc_fw {
    class InPostcard;
}

namespace izat_remote_api {

class IzatNotifierProxy;

struct OutCard;

class IzatNotifier  {
protected:
    IzatNotifierProxy* const mNotifierProxy;
    IzatNotifier(const char* const tag, const OutCard* subCard);
    virtual ~IzatNotifier();
    void registerSelf();
public:
    virtual void handleMsg(qc_loc_fw::InPostcard * const in_card) = 0;
};

} // izat_remote_api

#endif //__IZATREMOTEAPIS_H__

