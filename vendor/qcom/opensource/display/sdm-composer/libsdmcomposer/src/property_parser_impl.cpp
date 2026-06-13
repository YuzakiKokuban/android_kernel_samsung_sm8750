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

#include <string.h>
#include <fstream>

#include "property_parser_impl.h"
#include "debug_handler.h"

#define __CLASS__ "PropertyParserImpl"

#define PERSIST_PROPERTY_FILE_PATH "/persist/display/vendor_display_build.prop"
#define PROPERTY_FILE_PATH "/usr/data/display/vendor_display_build.prop"

PropertyParserImpl *PropertyParserImpl::property_parser_impl_ = nullptr;
uint32_t PropertyParserImpl::ref_count_ = 0;
recursive_mutex PropertyParserImpl::recursive_mutex_;

PropertyParserImpl *PropertyParserImpl::GetInstance() {
  if (!property_parser_impl_) {
    property_parser_impl_= new PropertyParserImpl();
  }

  return property_parser_impl_;
}

int PropertyParserImpl::Init() {
  lock_guard<recursive_mutex> obj(recursive_mutex_);
  if (ref_count_) {
    ref_count_++;
    return 0;
  }

  bool prop_file_found = false;
  std::fstream prop_file;
  prop_file.open(PERSIST_PROPERTY_FILE_PATH, std::fstream::in);
  if (prop_file.is_open()) {
    prop_file_found = true;
    DLOGI("found prop file %s", PERSIST_PROPERTY_FILE_PATH);
  } else {
    prop_file.open(PROPERTY_FILE_PATH, std::fstream::in);
    if (prop_file.is_open()) {
      DLOGI("found prop file %s", PROPERTY_FILE_PATH);
      prop_file_found = true;
    }
  }

  if (prop_file_found) {
    std::string line = {};
    while (std::getline(prop_file, line)) {
      auto pos = line.find('=');
      if (pos != std::string::npos) {
        std::string prop_name = line.substr(0, pos);
        std::string value = line.substr(pos + 1, std::string::npos);
        DLOGI("%s=%s", prop_name.c_str(), value.c_str());
        properties_map_.emplace(std::make_pair(prop_name, value));
      }
    }
  }
  ref_count_++;
  return 0;
}

int PropertyParserImpl::Deinit() {
  lock_guard<recursive_mutex> obj(recursive_mutex_);
  if (ref_count_) {
    ref_count_--;
    if (!ref_count_) {
      delete property_parser_impl_;
      property_parser_impl_ = nullptr;
    }
  }
  return 0;
}

int PropertyParserImpl::GetProperty(const char *property_name, int *value) {
  lock_guard<recursive_mutex> obj(recursive_mutex_);
  if (!property_name || !value)
    return -EINVAL;

  *value = 0;
  auto it = properties_map_.find(property_name);
  if (it != properties_map_.end())
    *value = std::stoi(it->second);
  else
    return -ENOKEY;

  return 0;
}

int PropertyParserImpl::GetProperty(const char *property_name, char *value) {
  lock_guard<recursive_mutex> obj(recursive_mutex_);
  if (!property_name || !value)
    return -EINVAL;

  auto it = properties_map_.find(property_name);
  if (it != properties_map_.end())
    std::snprintf(value, it->second.size(), "%s", it->second.c_str());
  else
    return -ENOKEY;

  return 0;
}
