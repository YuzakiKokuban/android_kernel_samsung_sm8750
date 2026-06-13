// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_METADATATYPE_H__
#define __COMMON_METADATATYPE_H__

typedef enum vendor_qti_hardware_display_common_MetadataType {
  METADATA_TYPE_INVALID = 0,
  /**
   * ID of buffer, used for debug only.
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  BUFFER_ID = 1,
  /**
   * Name of buffer passed in at allocation time, used for debug.
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  NAME = 2,
  /**
   * Width of buffer in pixels, requested at allocation time.
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  WIDTH = 3,
  /**
   * Height of buffer in pixels, requested at at allocation time.
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  HEIGHT = 4,
  /**
   * Number of layers in the buffer, requested at allocation time.
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  LAYER_COUNT = 5,
  /**
   * Format of the buffer, requested at allocation time.
   * This returns the type vendor_qti_hardware_display_common_PixelFormat
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  PIXEL_FORMAT_REQUESTED = 6,
  /**
   * Fourcc code for the format of the buffer.
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  PIXEL_FORMAT_FOURCC = 7,
  /**
   * Modifier used with fourcc format of the buffer.
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  DRM_PIXEL_FORMAT_MODIFIER = 8,
  /**
   * Usage of the buffer requested at allocation time.
   * This is a bitmask with definitions in
   * vendor_qti_hardware_display_common_BufferUsage This does not change for the
   * lifetime of the buffer.
   * Functions supported: getMetadata
   */
  USAGE = 9,
  /**
   * Size in bytes of memory allocated for buffer, including metadata and
   * padding. This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  ALLOCATION_SIZE = 10,
  /**
   * Returns true if buffer has protected content.
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  PROTECTED_CONTENT = 11,
  /**
   * Compression strategy of the buffer.
   * Values are defined in vendor_qti_hardware_display_common_Compression
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  COMPRESSION = 12,
  /**
   * How buffer's planes are interlaced.
   * Values are defined in vendor_qti_hardware_display_common_Interlaced
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  INTERLACED = 13,
  /**
   * Chroma siting of the buffer.
   * Values are defined in vendor_qti_hardware_display_common_ChromaSiting
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  CHROMA_SITING = 14,
  /**
   * Layout of the buffer's planes.
   * This returns vendor_qti_hardware_display_common_BufferLayout
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  PLANE_LAYOUTS = 15,
  /**
   * Crop region in pixels of the buffer.
   * Crop is stored as vendor_qti_hardware_display_common_Rect
   * Functions supported: getMetadata, setMetadata
   */
  CROP = 16,
  /**
   * Dataspace of the buffer.
   * Values are defined in vendor_qti_hardware_display_common_Dataspace
   * Functions supported: getMetadata, setMetadata
   */
  DATASPACE = 17,
  /**
   * Blend mode of the buffer,
   * as defined in vendor_qti_hardware_display_common_BlendMode
   * Functions supported: getMetadata, setMetadata
   */
  BLEND_MODE = 18,
  /**
   * Aligned width of allocated buffer in pixels.
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  STRIDE = 23,
  // Qti defined metadata types - start from 10000
  /**
   * Per frame VT timestamp used by camera.
   * Functions supported: getMetadata, setMetadata
   */
  VT_TIMESTAMP = 10000,
  /**
   * Interlaced strategy defined by video.
   * Functions supported: getMetadata, setMetadata
   */
  PP_PARAM_INTERLACED = 10002,
  /**
   * Set by camera to indicate buffer will be used for high performance video
   * use case.
   * Functions supported: getMetadata, setMetadata
   */
  VIDEO_PERF_MODE = 10003,
  /**
   * Graphics surface metadata,
   * as defined in vendor_qti_hardware_display_common_GraphicsMetadata
   * Functions supported: getMetadata, setMetadata
   */
  GRAPHICS_METADATA = 10004,
  /**
   * UBWC CR stats,
   * as defined in vendor_qti_hardware_display_common_UBWCStats
   * Functions supported: getMetadata, setMetadata
   */
  UBWC_CR_STATS_INFO = 10005,
  /**
   * Frame rate, as a float.
   * Functions supported: getMetadata, setMetadata
   */
  REFRESH_RATE = 10006,
  /**
   * Used for GPU post processing to determine when to map secure buffer.
   * Functions supported: getMetadata, setMetadata
   */
  MAP_SECURE_BUFFER = 10007,
  /**
   * Used if VENUS output buffer is linear for UBWC interlaced video.
   * Functions supported: getMetadata, setMetadata
   */
  LINEAR_FORMAT = 10008,
  /**
   * Set by graphics to indicate that this buffer will be written to but not
   * swapped out.
   * Functions supported: getMetadata, setMetadata
   */
  SINGLE_BUFFER_MODE = 10009,
  /**
   * CVP metadata set by camera and read by video,
   * as defined in vendor_qti_hardware_display_common_CVPMetadata
   * Functions supported: getMetadata, setMetadata
   */
  CVP_METADATA = 10010,
  /**
   * Video histogram stats populated by video decoder,
   * as defined in vendor_qti_hardware_display_common_VideoHistogramMetadata
   * Functions supported: getMetadata, setMetadata
   */
  VIDEO_HISTOGRAM_STATS = 10011,
  /**
   * File descriptor for buffer data, returned as an int.
   * Functions supported: getMetadata
   */
  FD = 10012,
  /**
   * Aligned width of allocated buffer in pixels.
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  ALIGNED_WIDTH_IN_PIXELS = 10014,
  /**
   * Aligned height of allocated buffer in pixels.
   * This does not change for the lifetime of the buffer.
   * Functions supported: getMetadata
   */
  ALIGNED_HEIGHT_IN_PIXELS = 10015,
  /**
   * Used to track if metadata type has been set for the buffer.
   * Functions supported: getMetadata
   */
  STANDARD_METADATA_STATUS = 10016,
  VENDOR_METADATA_STATUS = 10017,
  /**
   * Returns 1 for YUV, 0 otherwise.
   * Functions supported: getMetadata
   */
  BUFFER_TYPE = 10018,
  /**
   * Video timestamp info,
   * as defined in vendor_qti_hardware_display_common_VideoTimestampInfo
   * Functions supported: getMetadata, setMetadata
   */
  VIDEO_TS_INFO = 10019,
  /**
   * Custom dimensions returns custom width of buffer with crop factored in (if
   * crop is set) or interlaced height factored in for UBWC formats.
   * Functions supported: getMetadata
   */
  CUSTOM_DIMENSIONS_STRIDE = 10021,
  CUSTOM_DIMENSIONS_HEIGHT = 10022,
  /**
   * Returns base address for RGB buffers,
   * with offset for UBWC metadata factored in for UBWC buffer.
   * If this is a linear buffer, result is the same as BASE_ADDRESS.
   * Functions supported: getMetadata
   */
  RGB_DATA_ADDRESS = 10023,
  /**
   * Indicates buffer access permission for a client,
   * where permissions are defined in
   * vendor_qti_hardware_display_common_BufferPermission and clients are defined
   * in vendor_qti_hardware_display_common_BufferClient
   * Functions supported: getMetadata, setMetadata
   */
  BUFFER_PERMISSION = 10026,
  /**
   * Unique identifier updated by memory module
   * to interpret the underlying memory by each VM.
   * Functions supported: getMetadata, setMetadata
   */
  MEM_HANDLE = 10027,
  /**
   * Set by clients to indicate that timed rendering will be
   * enabled or disabled for this buffer
   * Functions supported: getMetadata, setMetadata
   */
  TIMED_RENDERING = 10028,
  /**
   * Dynamic metadata used for limited use cases.
   * Functions supported: getMetadata, setMetadata
   */
  CUSTOM_CONTENT_METADATA = 10029,
  /**
   * Video transcode stats populated by video decoder.
   * Functions supported: getMetadata, setMetadata
   */
  VIDEO_TRANSCODE_STATS = 10030,
  /**
   * Early notify line count, used by video.
   * Functions supported: getMetadata, setMetadata
   */
  EARLYNOTIFY_LINECOUNT = 10031,
  /**
   * Additional shared memory in buffer, outside of content and metadata,
   * for client use. Defined in
   * vendor_qti_hardware_display_common_ReservedRegion
   * Functions supported: getMetadata
   */
  RESERVED_REGION = 10032,
  /**
   * Used in combination with vendor_qti_hardware_display_common_PixelFormat
   * to describe variants of standard pixel formats.
   * Variants may have different alignment requirements.
   * Definitions are in vendor_qti_hardware_display_common_PixelFormatModifier
   * Functions supported: getMetadata
   */
  FORMAT_MODIFIER = 10033,
  /**
   * Mastering display metadata,
   * defined in vendor_qti_hardware_display_common_QtiMasteringDisplay
   * Functions supported: getMetadata, setMetadata
   */
  MASTERING_DISPLAY = 10034,
  /**
   * Content light level metadata,
   * defined in vendor_qti_hardware_display_common_QtiContentLightLevel
   * Functions supported: getMetadata, setMetadata
   */
  CONTENT_LIGHT_LEVEL = 10035,
  /**
   * Dynamic HDR metadata,
   * defined in vendor_qti_hardware_display_common_QtiDynamicMetadata
   * Functions supported: getMetadata, setMetadata
   */
  DYNAMIC_METADATA = 10036,
  /**
   * Matrix coefficients,
   * defined in vendor_qti_hardware_display_common_QtiMatrixCoefficients
   * Functions supported: getMetadata, setMetadata
   */
  MATRIX_COEFFICIENTS = 10037,
  /**
   * Color remapping info,
   * defined in vendor_qti_hardware_display_common_QtiColorRemappingInfo
   * Functions supported: getMetadata, setMetadata
   */
  COLOR_REMAPPING_INFO = 10038,
  /**
   * Base address of buffer data.
   * This does not account for UBWC meta planes.
   * Functions supported: getMetadata
   */
  BASE_ADDRESS = 10039,
  /**
   * Indicates if buffer was allocated as UBWC.
   * Functions supported: getMetadata
   */
  IS_UBWC = 10040,
  /**
   * Indicates if buffer was allocated as tile rendered.
   * Functions supported: getMetadata
   */
  IS_TILE_RENDERED = 10041,
  /**
   * Indicates if buffer was allocated as cached.
   * Functions supported: getMetadata
   */
  IS_CACHED = 10042,
  /**
   * Name of heap used for allocation.
   * Functions supported: getMetadata
   */
  HEAP_NAME = 10043,
  /**
   * Pixel format post allocation.
   * Functions supported: getMetadata
   */
  PIXEL_FORMAT_ALLOCATED = 10044,
  /**
   * Last buffer dequeue duration.
   * Functions supported: getMetadata, setMetadata
   */
  BUFFER_DEQUEUE_DURATION = 10045,
  /**
   * Anamorphic compression related information for the buffer.
   * Functions supported: getMetadata, setMetadata
   */
  ANAMORPHIC_COMPRESSION_METADATA = 10046,
  /**
   * Default view for multi-view buffer
   * Functions supported: getMetadata
   */
  BASE_VIEW = 10047,
  /**
   * All available views for multi-view buffer
   * Functions supported: getMetadata
   */
  MULTI_VIEW_INFO = 10048,
  /**
   * Three Dimensional Reference Display Info
   * Functions supported: getMetadata, setMetadata
   */
  THREE_DIMENSIONAL_REF_INFO = 10049,

} vendor_qti_hardware_display_common_MetadataType;

#endif  // __COMMON_METADATATYPE_H__
