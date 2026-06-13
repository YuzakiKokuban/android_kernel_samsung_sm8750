// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAP_METADATA_MANAGER_DEFS_H__
#define __SNAP_METADATA_MANAGER_DEFS_H__

#include <string>
#include <vector>

#include "Dataspace.h"
#include "PixelFormat.h"
#include "QtiColorRemappingInfo.h"
#include "QtiContentLightLevel.h"
#include "QtiMasteringDisplay.h"
#include "QtiMatrixCoEfficients.h"
#include "QtiAnamorphicMetadata.h"
#include "SnapTypes.h"

namespace snapalloc {

#define METADATA_TYPE_MAX 256
#define METADATA_SET_SIZE 512
#define UBWC_STATS_ARRAY_SIZE 2
#define RESERVED_REGION_SIZE 4096

typedef struct QtiColorMetadata {
  // Default values based on sRGB, needs to be overridden
  // based on the format and size.
  vendor_qti_hardware_display_common_Dataspace dataspace;
  vendor_qti_hardware_display_common_QtiMatrixCoEfficients matrixCoefficients;

  vendor_qti_hardware_display_common_QtiMasteringDisplay masteringDisplayInfo;
  vendor_qti_hardware_display_common_QtiContentLightLevel contentLightLevel;
  vendor_qti_hardware_display_common_QtiColorRemappingInfo cRI;

  // Dynamic meta data elements
  vendor_qti_hardware_display_common_QtiDynamicMetadata dynamicMetadata;
} QtiColorMetaData;

struct SnapMetadata {
  int32_t interlaced;
  float refreshrate;
  int32_t mapSecureBuffer;
  /* VENUS output buffer is linear for UBWC Interlaced video */
  uint32_t linearFormat;
  /* Set by graphics to indicate that this buffer will be written to but not
    * swapped out */
  uint32_t isSingleBufferMode;

  /* Set by camera to program the VT Timestamp */
  uint64_t vtTimeStamp;
  /* Color Aspects + HDR info */
  QtiColorMetadata color;
  /* Set by camera to indicate that this buffer will be used for a High
    * Performance Video Usecase */
  uint32_t isVideoPerfMode;
  /* Populated and used by adreno during buffer size calculation.
    * Set only for RGB formats. */
  vendor_qti_hardware_display_common_GraphicsMetadata graphics_metadata;
  /* Video histogram stats populated by video decoder */
  vendor_qti_hardware_display_common_VideoHistogramMetadata video_histogram_stats;
  /*
    * Producer (camera) will set cvp metadata and consumer (video) will
    * use it. The format of metadata is known to producer and consumer.
    */
  vendor_qti_hardware_display_common_CVPMetadata cvpMetadata;
  vendor_qti_hardware_display_common_Rect crop;
  int32_t blendMode;
  char name[QTI_MAX_NAME_LEN];
  vendor_qti_hardware_display_common_VideoTimestampInfo videoTsInfo;
  vendor_qti_hardware_display_common_BufferPermission bufferPerm[static_cast<int>(
      vendor_qti_hardware_display_common_BufferClient::BUFFERCLIENT_MAX)];
  int64_t memHandle;

  /* Set by clients to indicate that timed rendering will be enabled
    * or disabled for this buffer. */
  uint32_t timedRendering;
  /* Video transcode stat populated by video decoder */
  vendor_qti_hardware_display_common_VideoTranscodeStatsMetadata video_transcode_stats;

  int32_t videoEarlyNotifyLineCount;
  /* Contains plane layout */
  vendor_qti_hardware_display_common_BufferLayout buffer_layout;

  /* Consumer should read this data as follows based on
  * "interlaced" listed above.
  * [0] : If it is progressive.
  * [0] : Top field, if it is interlaced.
  * [1] : Do not read, if it is progressive.
  * [1] : Bottom field, if it is interlaced.
  */
  vendor_qti_hardware_display_common_UBWCStats ubwcCRStats[UBWC_STATS_ARRAY_SIZE];

  /* Set by clients to program the anamorphic compression */
  vendor_qti_hardware_display_common_QtiAnamorphicMetadata anamorphic_compression;

  /* Tracks if metadata has be explicitly set or is a default value*/
  bool isStandardMetadataSet[METADATA_SET_SIZE];
  bool isVendorMetadataSet[METADATA_SET_SIZE];
  uint64_t reservedSize;
  char heapName[QTI_MAX_NAME_LEN];
  vendor_qti_hardware_display_common_PixelFormat pixel_format_requested;
  int64_t bufferDequeueDuration;
  vendor_qti_hardware_display_common_ThreeDimensionalRefInfo three_dimensional_ref_info;
};
}  // namespace snapalloc

#endif  // __SNAP_METADATA_MANAGER_DEFS_H__