/*
* Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
* Not a Contribution.
*
* Copyright (c) 2017-2021 The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
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
*Not a contribution
*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
* Copyright © 2011 Intel Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
* Authors:
*    Benjamin Franzke <benjaminfranzke@googlemail.com>
*/

#ifndef GBM_PRIV_H_
#define GBM_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif


/**
 * \file gbm_priv.h
 * \brief: This header file would be an interface for Graphics and video clients
 *
 */
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <gbm.h>
#include <color_metadata.h>
/**
 * These are the flags used by the clients during allocation to
 * indicate the purpose of allocation to the gbm backend.
 * Gbm backend can use them to decide on the ION heap id and ION flags to be
 * used as part of ION allocation.
 * NOTE:
 *   These usage flags are extension to the open source gbm.h defined
 *   flags, so the initial integer value of these flags has spacing to avoid
 *   conflicts
 */
/** These allocation heap are used to select the corresponding ION heap id. */
#define GBM_BO_ALLOC_SYSTEM_HEAP_QTI		0x00000100 /*BO allocation from system heap*/
#define GBM_BO_ALLOC_SECURE_HEAP_QTI		0x00000200 /*BO allocation from secure heap*/
#define GBM_BO_ALLOC_SECURE_DISPLAY_HEAP_QTI	0x00000400 /*BO allocation from secure-display heap*/
#define GBM_BO_ALLOC_ADSP_HEAP_QTI		0x00000800 /*BO allocation from ADSP heap*/
#define GBM_BO_ALLOC_CAMERA_HEAP_QTI		0x00001000 /*BO allocation from camera heap*/
#define GBM_BO_ALLOC_IOMMU_HEAP_QTI		0x00002000 /*BO allocation from IOMMU heap*/
#define GBM_BO_ALLOC_MM_HEAP_QTI		0x00004000 /*BO allocation from MM heap*/

/** These usage flags are used to inform GBM backend about the usage of the allcated BO.*/
#define GBM_BO_USAGE_PROTECTED_QTI		0x00008000 /*BO allocation flag to get protected BO*/
#define GBM_BO_USAGE_UNCACHED_QTI		0x00010000 /*BO allocation flag to get uncached BO*/
#define GBM_BO_USAGE_CPU_READ_QTI		0x00020000 /*BO has CPU read access */
#define GBM_BO_USAGE_CPU_WRITE_QTI		0x00040000 /*BO has CPU write access */
#define GBM_BO_USAGE_NON_CPU_WRITER_QTI		0x00080000 /*BO has non-CPU writer access */
#define GBM_BO_USAGE_UBWC_ALIGNED_QTI    	0x00100000 /*BO allocation for UBWC enabled client request*/
#define GBM_BO_USAGE_CAMERA_READ_QTI		0x00200000 /*BO used for camera consumer */
#define GBM_BO_USAGE_CAMERA_WRITE_QTI		0x00400000 /*BO used for camera producer */
#define GBM_BO_USAGE_VIDEO_ENCODER_QTI		0x00800000 /*BO used for video encoder */
#define GBM_BO_USAGE_HW_COMPOSER_QTI		0x01000000 /*BO used for video encoder */
#define GBM_BO_USAGE_HW_RENDERING_QTI    	0x02000000 /*BO allocation for GPU based rendering operation */
#define GBM_BO_USAGE_10BIT_QTI    	      0x04000000 /*BO allocation for 10 bit */
#define GBM_BO_USAGE_10BIT_TP_QTI    	      0x08000000 /*BO allocation for 10 bit TP */
#define GBM_BO_USAGE_EGL_IMAGE_QTI      0x10000000 /*BO allocation for EGL image operation */

/**
 *  These are the parameter types to be used by the clients to query metadata info from gbm backend
 *
 */
#define GBM_METADATA_GET_INTERLACED           0x00000001 /*Param type to query Interlaced field*/

#define GBM_METADATA_GET_REFRESH_RATE         0x00000002 /*Param type to query Refresh rate field*/

#define GBM_METADATA_GET_MAP_SECURE_BUFFER    0x00000004 /*Param type to query map secure buffer field*/

#define GBM_METADATA_GET_LINEAR_FORMAT        0x00000008 /*Param type to query linear format field*/

#define GBM_METADATA_GET_COLOR_SPACE          0x00000010 /*Param type to query Color space field*/

#define GBM_METADATA_GET_IGC                  0x00000020 /*Param type to query IGC field*/

#define GBM_METADATA_GET_S3DFORMAT            0x00000040 /*Param type to query S3D format field*/

#define GBM_METADATA_GET_SECURE_BUF_STAT      0x00000080 /*Param type to query secure buffer status*/

#define GBM_METADATA_GET_COLOR_METADATA       0x00000100 /*Param type to query Color Metadata*/

#define GBM_METADATA_GET_UBWC_BUF_STAT        0x00000200 /*Param type to query UBWC buffer status*/

#define GBM_METADATA_GET_VT_TIMESTAMP         0x00000400 /*Param type to query VT timestamp*/

#define GBM_METADATA_GET_VIDEO_PERF_MODE      0x00000800 /*Param type to query Video Perf mode*/

#define GBM_METADATA_GET_CVP_METADATA         0x00001000 /*Param type to query CVPMetadata*/

#define GBM_METADATA_GET_VIDEO_HIST_STAT      0x00002000 /*Param type to query Video Histogram Stat*/

/**
 *  These are the parameter types to be used by the clients to set metadata info from gbm backend
 *
 */
#define GBM_METADATA_SET_INTERLACED           0x00000001 /*Param type to set Interlaced field*/

#define GBM_METADATA_SET_REFRESH_RATE         0x00000002 /*Param type to set Refresh rate field*/

#define GBM_METADATA_SET_MAP_SECURE_BUFFER    0x00000004 /*Param type to set map secure buffer field*/

#define GBM_METADATA_SET_LINEAR_FORMAT        0x00000008 /*Param type to set linear format field*/

#define GBM_METADATA_SET_COLOR_SPACE          0x00000010 /*Param type to set Color space field*/

#define GBM_METADATA_SET_IGC                  0x00000020 /*Param type to set IGC field*/

#define GBM_METADATA_SET_S3DFORMAT            0x00000040 /*Param type to set S3D format field*/

#define GBM_METADATA_SET_COLOR_METADATA       0x00000080 /*Param type to set Color Metadata*/

#define GBM_METADATA_SET_VT_TIMESTAMP         0x00000100 /*Param type to set VT timestamp*/

#define GBM_METADATA_SET_VIDEO_PERF_MODE      0x00000200 /*Param type to set Video Perf mode*/

#define GBM_METADATA_SET_CVP_METADATA         0x00000400 /*Param type to set CVPMetadata*/

#define GBM_METADATA_SET_VIDEO_HIST_STAT      0x00000800 /*Param type to set Video Histogram Stat*/

/**
 * These define the color space value for the Metadata Color space entry
 *
 */
#define GBM_METADATA_COLOR_SPACE_ITU_R_601       0x1

#define GBM_METADATA_COLOR_SPACE_ITU_R_601_FR    0x2

#define GBM_METADATA_COLOR_SPACE_ITU_R_709       0x3

#define GBM_METADATA_COLOR_SPACE_ITU_R_2020      0x4

#define GBM_METADATA_COLOR_SPACE_ITU_R_2020_FR   0x5


/**
 * These define the IGC value for the Metadata IGC entry
 *
 */
#define GBM_METADATA_IGC_NONE   0x1

#define GBM_METADATA_IGC_sRGB   0x2




/**
 * Supported Perform API Operation Opcodes
 *
 */
#define GBM_PERFORM_GET_SURFACE_FORMAT                   0x0 /*Query user provided format from the
                                                               gbm surface*/

#define GBM_PERFORM_GET_SURFACE_WIDTH                    0x1 /*Query user provided width from the
                                                               gbm surface */

#define GBM_PERFORM_GET_SURFACE_HEIGHT                   0x2 /*Query user provided height from the
                                                               gbm surface*/

#define GBM_PERFORM_SET_SURFACE_FRONT_BO                 0x3 /*Update the current state to
                                                               NEW_FRONT_BUFFER of the BO associated
                                                               with the gbm surface*/

#define GBM_PERFORM_GET_SURFACE_FREE_BO                  0x4 /*Return the BO which is marked free
                                                               associated with the gbm surface */

#define GBM_PERFORM_VALIDATE_SURFACE                     0x5 /*Validate the GBM Surface*/

#define GBM_PERFORM_CPU_MAP_FOR_BO                       0x6 /*Return the mapped address to the
                                                               BO buffer*/

#define GBM_PERFORM_CPU_UNMAP_FOR_BO                     0x7 /*Unmap the already mapped BO buffer*/

#define GBM_PERFORM_GET_BO_SIZE                          0x8 /*Query BO buffer size*/

#define GBM_PERFORM_GET_BO_NAME                          0x9 /*Query BO buffer name*/

#define GBM_PERFORM_IMPORT_BO_FROM_NAME                  0x11 /*Import BO from specified name*/

#define GBM_PERFORM_GET_DRM_DEVICE_MAGIC                 0x12 /*Query DRM Device magic id */

#define GBM_PERFORM_AUTH_DRM_DEVICE_MAGIC                0x13 /*Authenticate DRM Device magic id */

#define GBM_PERFORM_GET_DRM_DEVICE_NAME                  0x14 /*Query DRM Device name */

#define GBM_PERFORM_VALIDATE_DEVICE                      0x15 /*Validate the GBM Device*/

#define GBM_PERFORM_GET_METADATA                         0x16 /*Query Metadata info */

#define GBM_PERFORM_SET_METADATA                         0x17 /*Set Metadata info */

#define GBM_PERFORM_GET_YUV_PLANE_INFO                   0x18 /*Query YUV plane info */

#define GBM_PERFORM_GET_UBWC_STATUS                      0x19 /*Query if the BO Allocation was
                                                                with UBWC enabled hardware*/

#define GBM_PERFORM_GET_RGB_DATA_ADDRESS                 0x20 /*Query for RGB data base address*/

#define GBM_PERFORM_SET_GPU_ADDR_FOR_BO                  0x21 /*Set GPU addr */

#define GBM_PERFORM_GET_GPU_ADDR_FOR_BO                  0x22 /* Query GPU Addr */

#define GBM_PERFORM_GET_SECURE_BUFFER_STATUS             0x23 /* Query Buffer is secured */

#define GBM_PERFORM_GET_METADATA_ION_FD                  0x24 /* Get Metadata ion fd from BO*/

#define GBM_PERFORM_DUMP_HASH_MAP                        0x25 /* Dump the existing hash map
                                                                 table contents*/

#define GBM_PERFORM_DUMP_BO_CONTENT                      0x26 /* Dump the BO buffer contents
                                                                 on to a file*/

#define GBM_PERFORM_GET_BO_ALIGNED_WIDTH                 0x27 /* Get Aligned width from BO*/

#define GBM_PERFORM_GET_BO_ALIGNED_HEIGHT                0x28 /* Get Aligned height from BO*/

#define GBM_PERFORM_GET_PLANE_INFO                       0x29 /* Get Plane Info from BO*/

#define GBM_PERFORM_DEFAULT_INIT_COLOR_META              0x30 /* Initialize Color Meta structure with
                                                                 default values to validate */

#define GBM_PERFORM_DUMP_COLOR_META                      0x31 /* Dump Color Meta structure */

#define GBM_PERFORM_GET_BUFFER_SIZE_DIMENSIONS           0x32 /* Query Buffer size and dimenstions
                                                                 (aligned width, aligned height and size)*/

#define GBM_PERFORM_GET_SURFACE_UBWC_STATUS              0x33 /* Query if the Surface BO Allocation was
                                                                 with UBWC enabled hardware */

#define GBM_PERFORM_GET_WL_RESOURCE_FROM_GBM_BUF_INFO    0x34 /* Get wl_resource from gbm_buf_info*/

#define GBM_PERFORM_GET_GBM_BUF_INFO_FROM_WL_RESOURCE    0x35 /* Retrieve gbm_buf_info from
                                                                 wl_resource */

#define GBM_PERFORM_GET_RENDER_DEVICE_NAME               0x36 /* Get render node for user */

#define GBM_PERFORM_GET_BUFFER_STRIDE_SCANLINE_SIZE      0x37 /* Query Buffer stride, scanline and size
                                                                 (stride, scanline and size)*/

#define GBM_PERFORM_GET_FD_WITH_NEW                      0x38 /* Query whether get fd with new */
/**
 * Error representation for GBM  API's
 *
 */
#define GBM_ERROR_NONE                           0x0

#define GBM_ERROR_BAD_DESCRIPTOR                 0x1

#define GBM_ERROR_BAD_HANDLE                     0x2

#define GBM_ERROR_BAD_VALUE                      0x3

#define GBM_ERROR_NOT_SHARED                     0x4

#define GBM_ERROR_NO_RESOURCES                   0x5

#define GBM_ERROR_UNDEFINED                      0x6

#define GBM_ERROR_UNSUPPORTED                    0x7

/**
* Custom import type define
* In case there are more enums added to GBM import,
* they'd be in sequential order.To avoid conflicts it's
* best to have an unused range ... 0x4FFF and decremented
* if we have any more imports added
*/
#define GBM_BO_IMPORT_GBM_BUF_TYPE              0x4FFF

/**
 * Custom 4CC Modifiers
 *
 * Format Modifiers info extracted from open source drm_fourcc.h:
 *
 * Format modifiers describe, typically, a re-ordering or modification
 * of the data in a plane of an FB.  This can be used to express tiled/
 * swizzled formats, or compression, or a combination of the two.
 *
 * The upper 4 bits of the format modifier are a vendor-id as assigned
 * below.  The lower 28 bits are assigned as vendor sees fit.
 */

 /* Vendor Ids: */
#define GBM_FORMAT_MOD_VENDOR_QTI    0x05

/* add more to the end as needed */
#define fourcc_mod_code_qti(vendor, val) \
        ((((uint32_t)GBM_FORMAT_MOD_VENDOR_## vendor) << 28) | (val & 0x0fffffffULL))

/**
 * Below is the list of Custom 4CC modes supported by libgbm
 *
 */
#define GBM_FORMAT_RAW16                                    fourcc_mod_code_qti(QTI, 1)

#define GBM_FORMAT_RAW10                                    fourcc_mod_code_qti(QTI, 2)

#define GBM_FORMAT_RAW12                                    fourcc_mod_code_qti(QTI, 3)

#define GBM_FORMAT_RAW8                                     fourcc_mod_code_qti(QTI, 4)

#define GBM_FORMAT_YV12                                     fourcc_mod_code_qti(QTI, 5)

#define GBM_FORMAT_YCbCr_420_SP                             fourcc_mod_code_qti(QTI, 6)

#define GBM_FORMAT_YCrCb_420_SP                             fourcc_mod_code_qti(QTI, 7)

#define GBM_FORMAT_YCbCr_422_SP                             fourcc_mod_code_qti(QTI, 8)

#define GBM_FORMAT_YCrCb_422_SP                             fourcc_mod_code_qti(QTI, 9)

#define GBM_FORMAT_YCbCr_422_I                              fourcc_mod_code_qti(QTI, 10)

#define GBM_FORMAT_YCrCb_422_I                              fourcc_mod_code_qti(QTI, 11)

#define GBM_FORMAT_YCbCr_420_SP_VENUS                       fourcc_mod_code_qti(QTI, 12)

#define GBM_FORMAT_NV12_ENCODEABLE                          fourcc_mod_code_qti(QTI, 13)

#define GBM_FORMAT_YCrCb_420_SP_VENUS                       fourcc_mod_code_qti(QTI, 14)

#define GBM_FORMAT_NV21_ZSL                                 fourcc_mod_code_qti(QTI, 15)

#define GBM_FORMAT_BLOB                                     fourcc_mod_code_qti(QTI, 16)

#define GBM_FORMAT_RAW_OPAQUE                               fourcc_mod_code_qti(QTI, 17)

#define GBM_FORMAT_COMPRESSED_RGBA_ASTC_4x4_KHR             fourcc_mod_code_qti(QTI, 18)

#define GBM_FORMAT_COMPRESSED_RGBA_ASTC_5x4_KHR             fourcc_mod_code_qti(QTI, 19)

#define GBM_FORMAT_COMPRESSED_RGBA_ASTC_5x5_KHR             fourcc_mod_code_qti(QTI, 20)

#define GBM_FORMAT_COMPRESSED_RGBA_ASTC_6x5_KHR             fourcc_mod_code_qti(QTI, 21)

#define GBM_FORMAT_COMPRESSED_RGBA_ASTC_6x6_KHR             fourcc_mod_code_qti(QTI, 22)

#define GBM_FORMAT_COMPRESSED_RGBA_ASTC_8x5_KHR             fourcc_mod_code_qti(QTI, 23)

#define GBM_FORMAT_COMPRESSED_RGBA_ASTC_8x6_KHR             fourcc_mod_code_qti(QTI, 24)

#define GBM_FORMAT_COMPRESSED_RGBA_ASTC_8x8_KHR             fourcc_mod_code_qti(QTI, 25)

#define GBM_FORMAT_COMPRESSED_RGBA_ASTC_10x5_KHR            fourcc_mod_code_qti(QTI, 26)

#define GBM_FORMAT_COMPRESSED_RGBA_ASTC_10x6_KHR            fourcc_mod_code_qti(QTI, 27)

#define GBM_FORMAT_COMPRESSED_RGBA_ASTC_10x8_KHR            fourcc_mod_code_qti(QTI, 28)

#define GBM_FORMAT_COMPRESSED_RGBA_ASTC_10x10_KHR           fourcc_mod_code_qti(QTI, 29)

#define GBM_FORMAT_COMPRESSED_RGBA_ASTC_12x10_KHR           fourcc_mod_code_qti(QTI, 30)

#define GBM_FORMAT_COMPRESSED_RGBA_ASTC_12x12_KHR           fourcc_mod_code_qti(QTI, 31)

#define GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR     fourcc_mod_code_qti(QTI, 32)

#define GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR     fourcc_mod_code_qti(QTI, 33)

#define GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR     fourcc_mod_code_qti(QTI, 34)

#define GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR     fourcc_mod_code_qti(QTI, 35)

#define GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR     fourcc_mod_code_qti(QTI, 36)

#define GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR     fourcc_mod_code_qti(QTI, 37)

#define GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR     fourcc_mod_code_qti(QTI, 38)

#define GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR     fourcc_mod_code_qti(QTI, 39)

#define GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR    fourcc_mod_code_qti(QTI, 40)

#define GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR    fourcc_mod_code_qti(QTI, 41)

#define GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR    fourcc_mod_code_qti(QTI, 42)

#define GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR   fourcc_mod_code_qti(QTI, 43)

#define GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR   fourcc_mod_code_qti(QTI, 44)

#define GBM_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR   fourcc_mod_code_qti(QTI, 45)

#define GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC                  fourcc_mod_code_qti(QTI, 46)

#define GBM_FORMAT_YCbCr_420_888                            fourcc_mod_code_qti(QTI, 47)

#define GBM_FORMAT_IMPLEMENTATION_DEFINED                   fourcc_mod_code_qti(QTI, 48)

#define GBM_FORMAT_NV12_HEIF                                fourcc_mod_code_qti(QTI, 49)

#define GBM_FORMAT_YCbCr_420_P010_VENUS                     fourcc_mod_code_qti(QTI, 50)

#define GBM_FORMAT_NV12_LINEAR_FLEX                         fourcc_mod_code_qti(QTI, 51)

#define GBM_FORMAT_NV12_UBWC_FLEX                           fourcc_mod_code_qti(QTI, 52)

#define GBM_FORMAT_MULTIPLANAR_FLEX                         fourcc_mod_code_qti(QTI, 53)

#define GBM_FORMAT_RGBA16161616F                            fourcc_mod_code_qti(QTI, 54)

#define GBM_FORMAT_RGB161616F                               fourcc_mod_code_qti(QTI, 55)

#define GBM_FORMAT_RGBA32323232F                            fourcc_mod_code_qti(QTI, 56)

#define GBM_FORMAT_RGB323232F                               fourcc_mod_code_qti(QTI, 57)

#define GBM_FORMAT_YCbCr_420_TP10_UBWC                      __gbm_fourcc_code('Q', '1', '2', 'A')

#define GBM_FORMAT_YCbCr_420_P010_UBWC                      __gbm_fourcc_code('Q', '1', '2', 'B')

/* Y/CbCr 4:2:0 P10 format*/
#define GBM_FORMAT_P010                                     __gbm_fourcc_code('P', '0', '1', '0')

/**
 * Pixel component ID defines
 * to be used at the generic plane object
 *
 */
/* luma */
#define PIXEL_COMPONENT_Y       0x00000001
/* chroma blue */
#define PIXEL_COMPONENT_Cb      0x00000002
/* chroma red */
#define PIXEL_COMPONENT_Cr      0x00000004
/* red */
#define PIXEL_COMPONENT_R       0x00000100
/* green */
#define PIXEL_COMPONENT_G       0x00000200
/* blue */
#define PIXEL_COMPONENT_B       0x00000400
/*RAW 10 */
#define PIXEL_COMPONENT_RAW10   0x00010000
/*RAW 16 */
#define PIXEL_COMPONENT_RAW16   0x00020000
/* alpha */
#define PIXEL_COMPONENT_A       0x00100000


/* Maximum count of planes for given frame buffer
   Different from GBM_MAX_PLANES
*/
#define MAX_NUM_OF_PLANES        5
#define MAX_NUM_MODIFIERS        4

/* possible formats for 3D content*/
enum {
  HAL_NO_3D = 0x0,
  HAL_3D_SIDE_BY_SIDE_L_R = 0x1,
  HAL_3D_SIDE_BY_SIDE_R_L = 0x2,
  HAL_3D_TOP_BOTTOM = 0x4,
  HAL_3D_IN_SIDE_BY_SIDE_L_R = 0x10000,  // unused legacy format
};

enum UBWC_Version {
    UBWC_UNUSED      = 0,
    UBWC_1_0         = 0x1,
    UBWC_2_0         = 0x2,
    UBWC_3_0         = 0x3,
    UBWC_4_0         = 0x4,
    UBWC_MAX_VERSION = 0xFF,
};

#define MAX_UBWC_STATS_LENGTH 32
struct UBWC_2_0_Stats {
    uint32_t nCRStatsTile32;  /**< UBWC Stats info for  32 Byte Tile */
    uint32_t nCRStatsTile64;  /**< UBWC Stats info for  64 Byte Tile */
    uint32_t nCRStatsTile96;  /**< UBWC Stats info for  96 Byte Tile */
    uint32_t nCRStatsTile128; /**< UBWC Stats info for 128 Byte Tile */
    uint32_t nCRStatsTile160; /**< UBWC Stats info for 160 Byte Tile */
    uint32_t nCRStatsTile192; /**< UBWC Stats info for 192 Byte Tile */
    uint32_t nCRStatsTile256; /**< UBWC Stats info for 256 Byte Tile */
};

struct UBWCStats {
    enum UBWC_Version version; /* Union depends on this version. */
    uint8_t bDataValid;      /* If [non-zero], CR Stats data is valid.
                                90                                * Consumers may use stats data.
                                91                                * If [zero], CR Stats data is invalid.
                                92                                * Consumers *Shall* not use stats data */
    union {
        struct UBWC_2_0_Stats ubwc_stats;
        uint32_t reserved[MAX_UBWC_STATS_LENGTH]; /* This is for future */
    };
};

#define VIDEO_HISTOGRAM_STATS_SIZE (4 * 1024)
/* Frame type bit mask */
#define QD_SYNC_FRAME (0x1 << 0)
struct VideoHistogramMetadata {
    uint32_t stats_info[1024]; /* video stats payload */
    uint32_t stat_len; /* Payload size in bytes */
    uint32_t frame_type; /* bit mask to indicate frame type */
    uint32_t display_width;
    uint32_t display_height;
    uint32_t decode_width;
    uint32_t decode_height;
    uint32_t reserved[12];
};

#define CVP_METADATA_SIZE 1024
enum CVPMetadataFlags {
    /* bit wise flags */
    CVP_METADATA_FLAG_NONE              = 0x00000000,
    CVP_METADATA_FLAG_REPEAT            = 0x00000001,
};

typedef struct CVPMetadata {
    uint32_t size; /* payload size in bytes */
    uint8_t payload[CVP_METADATA_SIZE];
    uint32_t capture_frame_rate;
    /* Frame rate in Q16 format.
            Eg: fps = 7.5, then
            capture_frame_rate = 7 << 16 --> Upper 16 bits to represent 7
            capture_frame_rate |= 5 -------> Lower 16 bits to represent 5

       If size > 0, framerate is valid
       If size = 0, invalid data, so ignore all parameters */
    uint32_t cvp_frame_rate;
    enum CVPMetadataFlags flags;
    uint32_t reserved[8];
} CVPMetadata;

/**
 * The generic_buf_layout structure is used with any pixel format that can be
 * represented by it, such as:
 *  - GBM_FORMAT_YCbCr_*_888
 *  - GBM_FORMAT_FLEX_RGB*_888
 *  - GBM_FORMAT_RGB[AX]_888[8],BGRA_8888,RGB_888
 *  - GBM_FORMAT_YV12,Y8,Y16,YCbCr_422_SP/I,YCrCb_420_SP
 *  - even implementation defined formats that can be represented by
 *    the structures
 *
 * Vertical increment (aka. row increment or stride) describes the distance in
 * bytes from the first pixel of one row to the first pixel of the next row
 * (below) for the component plane. This can be negative.
 *
 * Horizontal increment (aka. column or pixel increment) describes the distance
 * in bytes from one pixel to the next pixel (to the right) on the same row for
 * the component plane. This can be negative.
 *
 * Each plane can be subsampled either vertically or horizontally by
 * a power-of-two factor.
 *
 * The bit-depth of each component can be arbitrary, as long as the pixels are
 * laid out on whole bytes, in native byte-order, using the most significant
 * bits of each unit.
 *
 *
 * generic planar layout object definition
 *
 */
typedef struct generic_plane {
    uint32_t *top_left;         /* pointer to the first byte of the top-left
                                   pixel of the plane. */
                                /* NOTE: For secured buffers this pointer is
                                   NULL */
    uint32_t offset;            /* offset to the first byte of the top-left
                                   pixel of the plane */
    int32_t component_id;       /* ID to represent  */
    uint32_t aligned_width;     /* Aligned width  in pixels */
    uint32_t aligned_height;    /* Aligned height in pixels*/
    uint32_t size;              /* size in bytes*/
    int32_t bits_per_component; /* bits allocated for the component in each
                                   pixel. Must be a positive multiple of 8. */
    int32_t bits_used;          /* number of the most significant bits used
                                   in the format for this component.
                                   Must be between 1 and bits_per_component */
    int32_t h_increment;        /* horizontal increment */
    int32_t v_increment;        /* vertical increment */
    int32_t h_subsampling;      /* horizontal subsampling. Must be a positive
                                   power of 2. */
    int32_t v_subsampling;      /* vertical subsampling. Must be a positive
                                   power of 2. */
    uint32_t stride;            /* stride for plane */
} generic_plane_t;


typedef struct generic_buf_layout {
    int pixel_format;                            /* the kind of pixel format */
    uint32_t num_planes;                         /* number of planes
                                                    0 for FLEX_FORMAT_INVALID */
    generic_plane_t planes[MAX_NUM_OF_PLANES];  /* a plane for each component,
                                                    ordered in increasing
                                                    component value order.
                                                    E.g. FLEX_FORMAT_RGBA
                                                    maps 0 -> R, 1 -> G, etc.
                                                    Can be NULL for FLEX_FORMAT_INVALID */
} generic_buf_layout_t;

/**
 * gbm device object definition
 *
 */
struct gbm_device {
   int fd;                                        /*gbm device fd */
   const char *name;                              /* gbm device name*/
   unsigned refcount;                             /* used to track the gbm device open call
                                                     reference*/
   void (*destroy)(struct gbm_device *gbm);
   int (*is_format_supported)(struct gbm_device *gbm,
                              uint32_t format,
                              uint32_t usage);
   struct gbm_bo *(*bo_create)(struct gbm_device *gbm,
                               uint32_t width, uint32_t height,
                               uint32_t format,
                               uint32_t usage,
                               const uint64_t *modifiers,
                               const unsigned int count);
   struct gbm_bo *(*bo_import)(struct gbm_device *gbm, uint32_t type,
                               void *buffer, uint32_t usage);
   int (*get_format_modifier_plane_count)(uint32_t format,
                                         uint64_t modifier);

   struct gbm_surface *(*surface_create)(struct gbm_device *gbm,
                                         uint32_t width, uint32_t height,
                                         uint32_t format, uint32_t flags,
                                         uint64_t *modifiers, unsigned int count);
};

/**
 * The Metadata structure whose buffer handle is associated with
 * gbm buffer object
 *
 * The members of this structure are accessed by gbm perform API only
 */
struct meta_data_t {
   uint32_t operation;          /* specific operation or state*/
   uint32_t interlaced;         /* video buffer scan */
   uint32_t s3d_format;         /* 3D video format supported */
   uint32_t linear_format;      /* Producer output buffer is linear for UBWC Interlaced video */
   uint32_t color_space;        /* color space specfics */
   uint32_t map_secure_buffer;  /* Flag to represent SecureBuffer being used for GPU*/
   int      is_buffer_secure;   /* Flag to query if buffer is secure*/
   int      is_buffer_ubwc;     /* Flag to query if buffer is UBWC allocated */
   int      igc;                /* IGC value*/
   float    refresh_rate;       /* video referesh rate*/
   ColorMetaData color_info;    /* Color Aspects + HDR info */
   uint64_t vt_timestamp;       /* timestamp set by camera, intended for VT*/
   uint32_t isVideoPerfMode;    /* set by camera indicates buffer will be used for
                                   High performace video use case */
   struct VideoHistogramMetadata histMetadata; /*histogram metadata*/
   CVPMetadata cvpMetadata;     /* Metadata from cvp */
};

/**
 * The allocated gbm buffer object.
 *
 * The members in this structure should not be accessed directly.
 */
struct gbm_bo {
   int ion_fd;                           /* ION buffer fd */
   int ion_metadata_fd;                  /* ION metadata fd */
   union gbm_bo_handle handle;           /* GEM handle */
   union gbm_bo_handle metadata_handle;  /* GEM metadata handle */
   uint32_t format;                      /* buffer format */
   uint32_t width;                       /* unaligned width of the buffer in pixels */
   uint32_t height;                      /* unaligned height of the buffer in pixels */
   uint32_t aligned_width;               /* aligned width of the buffer in pixels */
   uint32_t aligned_height;              /* aligned height of the buffer in pixels */
   uint32_t size;                        /* size of the buffer in bytes */
   uint32_t stride;                      /* stride of the buffer in bytes */
   uint32_t usage_flags;                 /* usage flags */
   uint32_t fbid;                        /* framebuffer id */
   uint32_t bpp;                         /* bytes per pixel*/
   uint32_t plane_count;                 /* plane count*/
   generic_buf_layout_t buf_lyt;
   uint64_t modifier;
   int (*bo_write)(struct gbm_bo *bo, const void *buf, size_t data);
   int (*bo_get_fd)(struct gbm_bo *bo);
   struct gbm_device* (*bo_get_device)(struct gbm_bo *bo);
   void (*bo_destroy)(struct gbm_bo *bo);
   void *user_data;
   void (*destroy_user_data)(struct gbm_bo *, void *);
   void * (*bo_map)(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
           uint32_t flags, uint32_t *stride, void **map_data);

   void (*bo_unmap)(void *map_data);
   uint32_t (*stride_for_plane)(int plane, struct gbm_bo *);

};


/**
 * The allocated gbm surface object.
 *
 * The members in this structure should not be accessed directly.
 */
struct gbm_surface {
   uint32_t format;         /* pixel format */
   uint32_t width;          /* width of surface in pixels */
   uint32_t height;         /* height of surface in pixels */
   uint32_t flags;          /* usage flags */
   void (*surface_release_buffer)(struct gbm_surface *surface,
                                  struct gbm_bo *bo);
   int (*surface_has_free_buffers)(struct gbm_surface *surface);
   void (*surface_destroy)(struct gbm_surface *surface);
   struct gbm_bo* (*surface_lock_front_buffer)(struct gbm_surface *surface);
};


/**
 * The gbm buffer data object used by the import fd API.
 *
 */
struct gbm_buf_info {
   int fd;                 /* ion fd */
   int metadata_fd;        /* ion metadata fd*/
   uint32_t width;         /* width of surface in pixels */
   uint32_t height;        /* height of surface in pixels */
   uint32_t format;        /* pixel format*/
};


/**
 * gbm backend interface object.
 *
 */
struct gbm_backendpriv{
   const char *backend_name;
   struct gbm_device *(*create_device)(int fd);
};


/**
 * gbm backend interface .
 *
 */
struct gbm_backendpriv *gbm_get_priv(void);

/**
 * Perform supported queries
 * Input  : operation opcodes
 *        : Arguments depending on the operation
 * Return = gbm_error value
 *
 */
int gbm_perform(int operation,...);

#ifdef __cplusplus
}
#endif

#endif //GBM_PRIV_H_
