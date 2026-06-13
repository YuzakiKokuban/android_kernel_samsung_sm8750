// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAP_CONSTRAINT_PARSER_H__
#define __SNAP_CONSTRAINT_PARSER_H__

#include "SnapConstraintDefs.h"
#include "SnapTypes.h"
#include "SnapUtils.h"

#include <map>
#include <mutex>
#include <vector>

namespace snapalloc {

class SnapConstraintParser {
 public:
  SnapConstraintParser(SnapConstraintParser &other) = delete;
  void operator=(const SnapConstraintParser &) = delete;
  static SnapConstraintParser *GetInstance();

  int ParseFormats(
      std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> *format_data_map);
  int ParseAlignments(const std::string &json_path,
                      std::map<vendor_qti_hardware_display_common_PixelFormat, BufferConstraints>
                          *constraint_set_map);

 private:
  ~SnapConstraintParser();
  SnapConstraintParser(){};
  static std::mutex constraint_parser_mutex_;

  static SnapConstraintParser *instance_;

  bool StringToEnumType(std::string input, vendor_qti_hardware_display_common_PixelFormat *output);
  bool StringToEnumType(std::string input,
                        vendor_qti_hardware_display_common_PlaneLayoutComponentType *output);
};

}  // namespace snapalloc

#endif  // __SNAP_CONSTRAINT_PARSER_H__