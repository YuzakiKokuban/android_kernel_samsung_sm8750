/*
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


#include <utils/debug.h>
#include <utils/utils.h>

#include "property_parser_interface.h"
#include "property_parser_impl.h"

#define __CLASS__ "PropertyParserInterface"

int PropertyParserInterface::Create(PropertyParserInterface **intf) {
  if (!intf) {
    DLOGE("intf pointer is NULL");
    return -EINVAL;
  }
  PropertyParserImpl *property_parser_impl = PropertyParserImpl::GetInstance();
  if (!property_parser_impl) {
    return -EINVAL;
  }
  int error = property_parser_impl->Init();
  if (error != 0) {
    DLOGE("Init failed with %d", error);
    return error;
  }
  *intf = property_parser_impl;

  return error;
}

int PropertyParserInterface::Destroy(PropertyParserInterface *intf) {
  if (!intf) {
    DLOGE("intf pointer is NULL");
    return -EINVAL;
  }

  PropertyParserImpl *property_parser_impl = static_cast<PropertyParserImpl *>(intf);
  int error = property_parser_impl->Deinit();
  if (error != 0) {
    DLOGE("Deinit failed with %d", error);
    return error;
  }

  return 0;
}
