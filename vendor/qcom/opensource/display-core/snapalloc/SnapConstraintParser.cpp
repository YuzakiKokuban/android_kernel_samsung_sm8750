// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SnapConstraintParser.h"

#include <json/json.h>
#include <fstream>
#include <iostream>

namespace snapalloc {

SnapConstraintParser *SnapConstraintParser::instance_{nullptr};
std::mutex SnapConstraintParser::constraint_parser_mutex_;

SnapConstraintParser *SnapConstraintParser::GetInstance() {
  std::lock_guard<std::mutex> lock(constraint_parser_mutex_);

  if (instance_ == nullptr) {
    instance_ = new SnapConstraintParser();
  }
  return instance_;
}

static std::unordered_map<std::string, vendor_qti_hardware_display_common_PixelFormat>
    PixelFormatStringToEnum = {
        {"UNSPECIFIED", PIXEL_FORMAT_UNSPECIFIED},
        {"RGBA_8888", RGBA_8888},
        {"RGBX_8888", RGBX_8888},
        {"RGB_888", RGB_888},
        {"RGB_565", RGB_565},
        {"BGRA_8888", BGRA_8888},
        {"YCBCR_422_SP", YCBCR_422_SP},
        {"YCrCb_420_SP", YCrCb_420_SP},
        {"YCBCR_422_I", YCBCR_422_I},
        {"RGBA_FP16", RGBA_FP16},
        {"RAW16", RAW16},
        {"BLOB", BLOB},
        {"IMPLEMENTATION_DEFINED", IMPLEMENTATION_DEFINED},
        {"YCBCR_420_888", YCBCR_420_888},
        {"RAW_OPAQUE", RAW_OPAQUE},
        {"RAW10", RAW10},
        {"RAW12", RAW12},
        {"RAW14", RAW14},
        {"RGBA_1010102", RGBA_1010102},
        {"Y8", Y8},
        {"Y16", Y16},
        {"YV12", YV12},
        {"DEPTH_16", DEPTH_16},
        {"DEPTH_24", DEPTH_24},
        {"DEPTH_24_STENCIL_8", DEPTH_24_STENCIL_8},
        {"DEPTH_32F", DEPTH_32F},
        {"DEPTH_32F_STENCIL_8", DEPTH_32F_STENCIL_8},
        {"STENCIL_8", STENCIL_8},
        {"YCBCR_P010", YCBCR_P010},
        {"HSV_888", HSV_888},
        {"R_8", R_8},
        /*{"R_16_UINT", R_16_UINT},
    {"RG_1616_UINT", RG_1616_UINT},
    {"RGBA_10101010", RGBA_10101010},*/
        {"RGBA_5551", RGBA_5551},
        {"RGBA_4444", RGBA_4444},
        {"YCbCr_420_SP", YCbCr_420_SP},
        {"YCrCb_422_SP", YCrCb_422_SP},
        {"RG_88", RG_88},
        {"YCbCr_444_SP", YCbCr_444_SP},
        {"YCrCb_444_SP", YCrCb_444_SP},
        {"YCrCb_422_I", YCrCb_422_I},
        {"BGRX_8888", BGRX_8888},
        {"NV21_ZSL", NV21_ZSL},
        {"BGR_565", BGR_565},
        {"RAW8", RAW8},
        {"ARGB_2101010", ARGB_2101010},
        {"RGBX_1010102", RGBX_1010102},
        {"XRGB_2101010", XRGB_2101010},
        {"BGRA_1010102", BGRA_1010102},
        {"ABGR_2101010", ABGR_2101010},
        {"BGRX_1010102", BGRX_1010102},
        {"XBGR_2101010", XBGR_2101010},
        {"TP10", TP10},
        {"CbYCrY_422_I", CbYCrY_422_I},
        {"BGR_888", BGR_888},
        /*{"YCbCr_422_I_10BIT", YCbCr_422_I_10BIT},
    {"YCbCr_422_I_10BIT_COMPRESSED", YCbCr_422_I_10BIT_COMPRESSED},
    {"YCbCr_420_SP_4R_UBWC", YCbCr_420_SP_4R_UBWC},*/
        {"COMPRESSED_RGBA_ASTC_4x4_KHR", COMPRESSED_RGBA_ASTC_4x4_KHR},
        {"COMPRESSED_RGBA_ASTC_5x4_KHR", COMPRESSED_RGBA_ASTC_5x4_KHR},
        {"COMPRESSED_RGBA_ASTC_5x5_KHR", COMPRESSED_RGBA_ASTC_5x5_KHR},
        {"COMPRESSED_RGBA_ASTC_6x5_KHR", COMPRESSED_RGBA_ASTC_6x5_KHR},
        {"COMPRESSED_RGBA_ASTC_6x6_KHR", COMPRESSED_RGBA_ASTC_6x6_KHR},
        {"COMPRESSED_RGBA_ASTC_8x5_KHR", COMPRESSED_RGBA_ASTC_8x5_KHR},
        {"COMPRESSED_RGBA_ASTC_8x6_KHR", COMPRESSED_RGBA_ASTC_8x6_KHR},
        {"COMPRESSED_RGBA_ASTC_8x8_KHR", COMPRESSED_RGBA_ASTC_8x8_KHR},
        {"COMPRESSED_RGBA_ASTC_10x5_KHR", COMPRESSED_RGBA_ASTC_10x5_KHR},
        {"COMPRESSED_RGBA_ASTC_10x6_KHR", COMPRESSED_RGBA_ASTC_10x6_KHR},
        {"COMPRESSED_RGBA_ASTC_10x8_KHR", COMPRESSED_RGBA_ASTC_10x8_KHR},
        {"COMPRESSED_RGBA_ASTC_10x10_KHR", COMPRESSED_RGBA_ASTC_10x10_KHR},
        {"COMPRESSED_RGBA_ASTC_12x10_KHR", COMPRESSED_RGBA_ASTC_12x10_KHR},
        {"COMPRESSED_RGBA_ASTC_12x12_KHR", COMPRESSED_RGBA_ASTC_12x12_KHR},
        {"COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR", COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR},
        {"COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR", COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR},
        {"COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR", COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR},
        {"COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR", COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR},
        {"COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR", COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR},
        {"COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR", COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR},
        {"COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR", COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR},
        {"COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR", COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR},
        {"COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR", COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR},
        {"COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR", COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR},
        {"COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR", COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR},
        {"COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR", COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR},
        {"COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR", COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR},
        {"COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR", COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR},
};

static std::unordered_map<std::string, vendor_qti_hardware_display_common_PlaneLayoutComponentType>
    PlaneLayoutComponentTypeStringToEnum = {
        {"Y", PLANE_LAYOUT_COMPONENT_TYPE_Y},
        {"CB", PLANE_LAYOUT_COMPONENT_TYPE_CB},
        {"CR", PLANE_LAYOUT_COMPONENT_TYPE_CR},
        {"R", PLANE_LAYOUT_COMPONENT_TYPE_R},
        {"G", PLANE_LAYOUT_COMPONENT_TYPE_G},
        {"B", PLANE_LAYOUT_COMPONENT_TYPE_B},
        {"RAW", PLANE_LAYOUT_COMPONENT_TYPE_RAW},
        {"BLOB", PLANE_LAYOUT_COMPONENT_TYPE_BLOB},
        {"A", PLANE_LAYOUT_COMPONENT_TYPE_A},
        {"META", PLANE_LAYOUT_COMPONENT_TYPE_META},
};

bool SnapConstraintParser::StringToEnumType(
    std::string input, vendor_qti_hardware_display_common_PixelFormat *output) {
  if (PixelFormatStringToEnum.find(input) != PixelFormatStringToEnum.end()) {
    *output = PixelFormatStringToEnum.at(input);
    return true;
  }

  DLOGW("Unable to find enum value for format string %s", input.c_str());
  return false;
}

bool SnapConstraintParser::StringToEnumType(
    std::string input, vendor_qti_hardware_display_common_PlaneLayoutComponentType *output) {
  if (PlaneLayoutComponentTypeStringToEnum.find(input) !=
      PlaneLayoutComponentTypeStringToEnum.end()) {
    *output = PlaneLayoutComponentTypeStringToEnum.at(input);
    return true;
  }

  // Empty string valid for formats where plane layout is not queried (e.g., depth stencil formats)
  if (input != "") {
    DLOGW("Unable to find enum value for plane layout component string %s", input.c_str());
  }

  return false;
}

int SnapConstraintParser::ParseFormats(
    std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> *format_data_map) {
  std::string json_path = "/vendor/etc/display/formats.json";

  std::ifstream ifs(json_path.c_str());
  if (!ifs.is_open()) {
    DLOGE("Error opening file");
    return -1;
  }

  Json::Reader reader;
  Json::Value input_data;
  reader.parse(ifs, input_data);
  auto format_data = input_data["format_data"];
  for (Json::Value::const_iterator it = format_data.begin(); it != format_data.end(); ++it) {
    auto format_data_set = format_data[it.index()];

    vendor_qti_hardware_display_common_PixelFormat format;
    if (!StringToEnumType(format_data_set["format"].asString(), &format)) {
      DLOGW("%s: Could not find format %s in format list", __FUNCTION__,
            format_data_set["format"].asString().c_str());
      continue;
    }
    auto format_data_set_vals = format_data_set["data"];
    FormatData data;
    data.bits_per_pixel = format_data_set_vals["bits_per_pixel"].asUInt();

    for (Json::Value::const_iterator it_planes = format_data_set_vals["planes"].begin();
         it_planes != format_data_set_vals["planes"].end(); ++it_planes) {
      auto plane_constraints_data = format_data_set_vals["planes"][it_planes.index()];
      PlaneLayoutData plane_constraints;

      plane_constraints.sample_increment_bits =
          plane_constraints_data["sample_increment_bits"].asUInt();
      plane_constraints.horizontal_subsampling =
          plane_constraints_data["h_subsampling_factor"].asUInt();
      plane_constraints.vertical_subsampling =
          plane_constraints_data["v_subsampling_factor"].asUInt();

      for (Json::Value::const_iterator it_plane_components =
               plane_constraints_data["components"].begin();
           it_plane_components != plane_constraints_data["components"].end();
           ++it_plane_components) {
        auto plane_component = plane_constraints_data["components"][it_plane_components.index()];

        vendor_qti_hardware_display_common_PlaneLayoutComponentType component_type;
        if (StringToEnumType(plane_component["component_type"].asString(), &component_type)) {
          vendor_qti_hardware_display_common_PlaneLayoutComponent component;
          component.type = component_type;
          component.offset_in_bits = plane_component["offset_bits"].asUInt();
          component.size_in_bits = plane_component["size_bits"].asUInt();
          plane_constraints.components.push_back(component);
        } else {
          // Empty string valid for formats where plane layout is not queried (e.g., depth stencil formats)
          if ((plane_component["component_type"].asString() != "")) {
            DLOGW("Invalid component type %s in %s",
                  plane_component["component_type"].asString().c_str(), json_path.c_str());
            continue;
          }
        }
      }
      data.planes.push_back(plane_constraints);
    }

    format_data_map->insert(std::make_pair(format, data));
  }

  if (format_data_map->empty()) {
    DLOGE("Format map empty");
    return -1;
  }

  return 0;
}

int SnapConstraintParser::ParseAlignments(const std::string &json_path,
                                          std::map<vendor_qti_hardware_display_common_PixelFormat,
                                                   BufferConstraints> *constraint_set_map) {
  std::ifstream ifs(json_path.c_str());

  if (!ifs.is_open()) {
    DLOGE("Error opening file");
    return -1;
  }

  Json::Reader reader;
  Json::Value input_data;
  reader.parse(ifs, input_data);
  auto constraint_sets = input_data["device"]["constraint_sets"];

  for (Json::Value::const_iterator it_sets = constraint_sets.begin();
       it_sets != constraint_sets.end(); ++it_sets) {
    auto constraint_set = constraint_sets[it_sets.index()];
    auto constraint_set_data = constraint_set["constraints"];

    vendor_qti_hardware_display_common_PixelFormat format =
        vendor_qti_hardware_display_common_PixelFormat::PIXEL_FORMAT_UNSPECIFIED;
    if (!StringToEnumType(constraint_set["format"].asString(), &format)) {
      DLOGW("%s: Could not find format %s in format list", __FUNCTION__,
            constraint_set["format"].asString().c_str());
      continue;
    }

    BufferConstraints data;
    data.size_align_bytes = constraint_set_data["size_align_bytes"].asUInt();
    data.modifier = 0;
    if (constraint_set.isMember("modifier")) {
      data.modifier = constraint_set_data["modifier"].asUInt();
    }

    for (Json::Value::const_iterator it_planes = constraint_set_data["planes"].begin();
         it_planes != constraint_set_data["planes"].end(); ++it_planes) {
      auto file_plane_constraints = constraint_set_data["planes"][it_planes.index()];
      PlaneConstraints plane_constraints;
      plane_constraints.alignment_type = ALIGNMENT;

      plane_constraints.stride.horizontal_stride_align =
          file_plane_constraints["horiz_stride_align_bytes"].asUInt();
      plane_constraints.scanline.scanline_align = file_plane_constraints["scanline_align"].asUInt();
      plane_constraints.size_align = file_plane_constraints["size_align_bytes"].asUInt();
      if (file_plane_constraints.isMember("block_width_bytes")) {
        plane_constraints.block_width = file_plane_constraints["block_width_bytes"].asUInt();
      }

      if (file_plane_constraints.isMember("block_height_bytes")) {
        plane_constraints.block_height = file_plane_constraints["block_height_bytes"].asUInt();
      }

      if (file_plane_constraints.isMember("meta_planes")) {
        plane_constraints.stride.horizontal_stride_align =
            file_plane_constraints["meta_planes"]["horiz_stride_align_bytes"].asUInt();
        plane_constraints.scanline.scanline_align =
            file_plane_constraints["meta_planes"]["scanline_align"].asUInt();
      }

      for (Json::Value::const_iterator it_plane_components =
               file_plane_constraints["components"].begin();
           it_plane_components != file_plane_constraints["components"].end();
           ++it_plane_components) {
        auto plane_component = file_plane_constraints["components"][it_plane_components.index()];
        vendor_qti_hardware_display_common_PlaneLayoutComponentType component_type;
        if (StringToEnumType(plane_component["component_type"].asString(), &component_type)) {
          plane_constraints.components.push_back(component_type);
        } else {
          DLOGW("Invalid component type %s in %s",
                plane_component["component_type"].asString().c_str(), json_path.c_str());
          continue;
        }
      }
      data.planes.push_back(plane_constraints);
    }

    constraint_set_map->insert(std::make_pair(format, data));
  }

  if (constraint_set_map->empty()) {
    DLOGE("Format map empty");
  }
  return 0;
}

}  // namespace snapalloc
