/*
* Copyright (c) 2017 - 2021 The Linux Foundation. All rights reserved.
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
*
* Changes from Qualcomm Innovation Center are provided under the following license:
*
* Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <drm/msm_drm.h>
#include <drm/drm_fourcc.h>
#include <drm/drm.h>
#include <linux/msm_ion.h>
#ifdef TARGET_ION_ABI_VERSION
#include <linux/ion.h>
#endif
#include <gbm_priv.h>
#include <msmgbm.h>
#include <msmgbm_common.h>
#include <linux/version.h>

#include <display/media/mmm_color_fmt.h>
#ifdef BUILD_HAS_WAYLAND_SUPPORT
#include <wayland-server.h>
#endif
#ifdef USE_GLIB
#define strlcat g_strlcat
#define strlcpy g_strlcpy
#endif
#define DRM_DEVICE_NAME "/dev/dri/card0"
#define RENDER_DEVICE_NAME "/dev/dri/renderD128"
#define DRM_MODULE_NAME "msm_drm"
#define ION_DEVICE_NAME "/dev/ion"
#define YUV_420_SP_BPP  1
#define YUV_422_SP_BPP  2
#define MAX_YUV_PLANES  3
#define DUAL_PLANES     2
#define CHROMA_STEP     2
#define msmgbm_perform gbm_perform
#define msmgbm_get_priv gbm_get_priv
#define PAGE_SIZE (4096)
#define ROUND_UP_PAGESIZE(x) (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1)
#define ALIGN(x, align) (((x) + ((align)-1)) & ~((align)-1))
#define MAGIC_HANDLE 0xa5a5a5a5

//Global variables
int g_debug_level = LOG_ERR;

//Global Variables
static pthread_mutex_t mutex_obj = PTHREAD_MUTEX_INITIALIZER;
static inline void lock_init(void)
{
    if(pthread_mutex_init(&mutex_obj, NULL))
    {
        LOG(LOG_ERR,"Failed to init Mutex\n %s\n",strerror(errno));
        return NULL;
    }

}
static inline void lock(void)
{
    if(pthread_mutex_lock(&mutex_obj))
    {
        LOG(LOG_ERR,"Failed to lock Mutex\n %s\n",strerror(errno));
        return NULL;
    }

}
static inline void unlock(void)
{
    if(pthread_mutex_unlock(&mutex_obj))
    {
        LOG(LOG_ERR,"Failed to un lock Mutex\n %s\n",strerror(errno));
        return NULL;
    }

}

static inline void lock_destroy(void)
{
    if(pthread_mutex_destroy(&mutex_obj))
        LOG(LOG_ERR,"Failed to init Mutex\n %s\n",strerror(errno));

}

void __attribute__ ((constructor)) msmgbm_library_open(void)
{
    lock_init();
}

void __attribute__ ((destructor)) msmgbm_library_close(void)
{
    lock_destroy();
}

//ION Helper Functions
int ion_open(void)
{
    int fd = open("/dev/ion", O_RDONLY);
    if (fd < 0)
        LOG(LOG_ERR, "open /dev/ion failed!\n %s\n",strerror(errno));
    return fd;
}

static inline
struct msmgbm_device * to_msmgbm_device(struct gbm_device *dev)
{
    return (struct msmgbm_device *)dev;
}

static inline
struct msmgbm_bo * to_msmgbm_bo(struct gbm_bo *bo)
{
    return (struct msmgbm_bo *)bo;
}

static inline
struct msmgbm_surface * to_msmgbm_surface(struct gbm_surface*surf)
{
    return (struct msmgbm_surface *)surf;
}

inline
void  msmgbm_dump_hashmap(void)
{
    dump_hashmap();
}

static void*
msmgbm_bo_map(uint32_t x, uint32_t y, uint32_t width,
              uint32_t height, uint32_t flags,
              uint32_t *stride, void **map_data)
{
  return NULL;
}

static uint32_t
msmgbm_stride_for_plane(int plane, struct gbm_bo * bo) {
  bool ubwc_enabled = is_ubwc_enbld(bo->format, bo->usage_flags, bo->usage_flags);
  bool cmprsd_rgb_format = is_valid_cmprsd_rgb_format(bo->format);
  bool is_yuv_format = is_valid_yuv_format(bo->format);

  LOG(LOG_DBG,"plane=%d bo->format=%d ubwc_enabled=%d is_yuv_format=%d cmprsd_rgb=%d\n",
                plane, bo->format, ubwc_enabled, is_yuv_format, cmprsd_rgb_format);

  if (is_valid_raw_format(bo->format)) {
    switch (bo->format) {
        case GBM_FORMAT_RAW10:
            return (bo->aligned_width * 10) / 8;
        case GBM_FORMAT_RAW12:
            return (bo->aligned_width * 12) / 8;
        case GBM_FORMAT_RAW16:
            return bo->aligned_width * 2;
        default:
            return bo->aligned_width;
    }
  } else if (is_yuv_format) {
    // yuv format
    return bo->buf_lyt.planes[plane].stride;
  } else if (ubwc_enabled && cmprsd_rgb_format) {
    uint32_t stride = 0;
    // UBWC RGB format
    // there are two planes.
    if (plane == 0) {
      stride = MMM_COLOR_FMT_RGB_META_STRIDE(MMM_COLOR_FMT_RGBA8888_UBWC, bo->width);
    } else if (plane == 1) {
      if (bo->bpp == 2) {
        stride = MMM_COLOR_FMT_RGB_STRIDE(MMM_COLOR_FMT_RGB565_UBWC, bo->width);
      } else if(bo->bpp == 4) {
        stride = MMM_COLOR_FMT_RGB_STRIDE(MMM_COLOR_FMT_RGBA8888_UBWC, bo->width);
      }
    }
    return stride;
  }
  return bo->stride;
}

static void
msmgbm_bo_unmap(void *map_data)
{

}

static int
msmgbm_bo_get_fd(struct gbm_bo *bo)
{

    if(bo!=NULL){
        return bo->ion_fd;
    }
    else {
        LOG(LOG_ERR, "NULL or Invalid bo pointer\n");
    return 0;
    }
}

static struct gbm_device*
msmgbm_bo_get_device(struct gbm_bo *bo)
{
    struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(bo);
    if(msm_gbm_bo!=NULL){
        return &msm_gbm_bo->device->base;
    }
    else {
        LOG(LOG_ERR, "NULL or Invalid bo pointer\n");
        return NULL;
    }
}

static int
msmgbm_bo_write(struct gbm_bo *bo, const void *buf, size_t count)
{
    struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(bo);
   int mappedNow =0;

    if((msm_gbm_bo!=NULL) && (buf != NULL)){
        if(bo->usage_flags & GBM_BO_USE_WRITE) {

            if(count <=0 || count > msm_gbm_bo->size){
                LOG(LOG_ERR, "Invalid count bytes (%d)\n",count);
                return -1;
            }

            if(msm_gbm_bo->cpuaddr == NULL)
            {
                if(msmgbm_bo_cpu_map(bo) == NULL){
                     LOG(LOG_ERR, "Unable to Map to CPU, cannot write to BO\n");
                     return -1;
                }
                mappedNow =1;
            }
            //Write to BO
            memcpy(msm_gbm_bo->cpuaddr, buf, count);

            if(mappedNow){ //Unmap BO, if we mapped it.
                msmgbm_bo_cpu_unmap(bo);
            }
            return 0;
        }
        else {
            LOG(LOG_ERR,"Operation not allowed\n");
        }
    }
    else {
        LOG(LOG_ERR,"NULL or Invalid bo or buffer pointer\n");
    }

    return -1;
}

static void
msmgbm_bo_destroy(struct gbm_bo *bo)
{
    struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(bo);
    struct drm_gem_close gem_close;

    int ret = 0;

    if(NULL != msm_gbm_bo){

        LOG(LOG_DBG,"\nmsm_gbm_bo->cpuaddr=0x%x\n msm_gbm_bo->mt_cpuaddr=0x%x\n",
                                msm_gbm_bo->cpuaddr, msm_gbm_bo->mt_cpuaddr);

        LOG(LOG_DBG,"Destroy called for fd=%d",bo->ion_fd);

         //Delete the Map entries if reference count is 0
        lock();
        if(decr_refcnt(bo->ion_fd))
        {
            /*
             * Perform unmap of both the BO buffer and Metadata
             * when ion fd deleted from hashmap
             * We are only handling CPU mapping here
             */
            if((msm_gbm_bo->cpuaddr != NULL)||(msm_gbm_bo->mt_cpuaddr != NULL))
                ret = msmgbm_bo_cpu_unmap(bo);
            LOG(LOG_DBG,"Currently closing fd=%d\n",bo->ion_fd);

            /*
             * Close the fd's for both BO and Metadata
             */
            if(bo->ion_fd >= 0){
                if(close(bo->ion_fd))
                    LOG(LOG_ERR,"Failed to Close bo->ion_fd=%d\n%s\n",
                                             bo->ion_fd,strerror(errno));
            }

            if(bo->ion_metadata_fd >= 0){
                if(close(bo->ion_metadata_fd))
                    LOG(LOG_ERR,"Failed to Close bo->ion_metadata_fd=%d\n %s\n",
                                           bo->ion_metadata_fd,strerror(errno));
            }
         }

         /*
          * Close the GEM handle for both the BO buffer and Metadata
          */
         if(decr_handle_refcnt(msm_gbm_bo->device->fd, bo->handle.u32)){
            LOG(LOG_DBG,"Currently closing GEM Handle=%u\n", bo->handle.u32);

            memset(&gem_close, 0, sizeof(gem_close));
            if(bo->handle.u32 && bo->handle.u32 != MAGIC_HANDLE){
                gem_close.handle=bo->handle.u32;
                if(ioctl(msm_gbm_bo->device->fd,DRM_IOCTL_GEM_CLOSE,&gem_close))
                    LOG(LOG_ERR,"Failed to Close GEM Handle for BO=%p\n%s\n",
                                             bo->handle.u32,strerror(errno));
            }
         }

         if(decr_handle_refcnt(msm_gbm_bo->device->fd, bo->metadata_handle.u32)){
            LOG(LOG_DBG,"Currently closing GEM Metadata Handle=%u\n", bo->metadata_handle.u32);
            memset(&gem_close, 0, sizeof(gem_close));
            if(bo->metadata_handle.u32 && bo->metadata_handle.u32 != MAGIC_HANDLE){
                gem_close.handle=bo->metadata_handle.u32;
                if(ioctl(msm_gbm_bo->device->fd,DRM_IOCTL_GEM_CLOSE,&gem_close))
                    LOG(LOG_ERR,"Failed to Close GEM Handle for BO=%p\n%s\n",
                                     bo->metadata_handle.u32,strerror(errno));

            }
        }
        unlock();

        /*
         * Free the msm_gbo object
         */
        LOG(LOG_DBG,"msm_gbm_bo handle to be freed for BO=%p\n",msm_gbm_bo);
        free(msm_gbm_bo);
        msm_gbm_bo = NULL;

    }
    else
        LOG(LOG_ERR,"NULL or Invalid bo pointer\n");

}

/*************************
 * GetFormatBpp(uint_32 format)
 *
 * returns number of bytes for a supported format
 * returns 0 for unsupported format
 *************************/
static int GetFormatBpp(uint32_t format)
{
   switch(format)
   {
        case GBM_FORMAT_R8:
            return 1;
        case GBM_FORMAT_UYVY:
        case GBM_FORMAT_RG88:
        case GBM_FORMAT_R16:
        case GBM_FORMAT_RGB565:
        case GBM_FORMAT_BGR565:
            return 2;
        case GBM_FORMAT_RGB888:
        case GBM_FORMAT_BGR888:
            return 3;
        case GBM_FORMAT_RG1616:
        case GBM_FORMAT_RGBA8888:
        case GBM_FORMAT_RGBX8888:
        case GBM_FORMAT_XRGB8888:
        case GBM_FORMAT_XBGR8888:
        case GBM_FORMAT_ARGB8888:
        case GBM_FORMAT_ABGR8888:
        case GBM_FORMAT_ABGR2101010:
            return 4;
        case GBM_FORMAT_YCbCr_420_SP:
        case GBM_FORMAT_YCrCb_420_SP:
        case GBM_FORMAT_YCbCr_420_SP_VENUS:
        case GBM_FORMAT_NV12_ENCODEABLE:
        case GBM_FORMAT_NV12:
        case GBM_FORMAT_YCbCr_420_TP10_UBWC:
        case GBM_FORMAT_P010:
        case GBM_FORMAT_NV21_ZSL:
        case GBM_FORMAT_YCbCr_420_888:
        case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
        case GBM_FORMAT_RAW10:
        case GBM_FORMAT_RAW12:
        case GBM_FORMAT_RAW16:
        case GBM_FORMAT_RAW8:
        case GBM_FORMAT_BLOB:
#ifdef COLOR_FMT_NV12_512
        case GBM_FORMAT_NV12_HEIF:
#endif
        case GBM_FORMAT_YCbCr_420_P010_VENUS:
        case GBM_FORMAT_YCbCr_420_P010_UBWC:
        case GBM_FORMAT_YCbCr_422_I:
        case GBM_FORMAT_YCrCb_422_I:
             LOG(LOG_DBG,"YUV format BPP\n");
            return 1;
        case GBM_FORMAT_RGB161616F:
            return 6;
        case GBM_FORMAT_RGBA16161616F:
            return 8;
        case GBM_FORMAT_RGB323232F:
            return 12;
        case GBM_FORMAT_RGBA32323232F:
            return 16;
        default:
            return 0;
   }
   return 0;
}

static int IsFormatSupported(uint32_t format)
{
    int is_supported;

    switch(format)
    {
        case GBM_FORMAT_R8:
        case GBM_FORMAT_RG88:
        case GBM_FORMAT_R16:
        case GBM_FORMAT_RG1616:
        case GBM_FORMAT_RGB565:
        case GBM_FORMAT_BGR565:
        case GBM_FORMAT_RGB888:
        case GBM_FORMAT_BGR888:
        case GBM_FORMAT_RGBA8888:
        case GBM_FORMAT_RGBX8888:
        case GBM_FORMAT_XRGB8888:
        case GBM_FORMAT_XBGR8888:
        case GBM_FORMAT_ARGB8888:
        case GBM_FORMAT_ABGR8888:
        case GBM_FORMAT_YCbCr_420_SP:
        case GBM_FORMAT_YCrCb_420_SP:
        case GBM_FORMAT_YCbCr_420_SP_VENUS:
        case GBM_FORMAT_NV12_ENCODEABLE:
        case GBM_FORMAT_NV12:
        case GBM_FORMAT_UYVY:
        case GBM_FORMAT_ABGR2101010:
        case GBM_FORMAT_YCbCr_420_TP10_UBWC:
        case GBM_FORMAT_YCbCr_420_P010_UBWC:
        case GBM_FORMAT_P010:
        case GBM_FORMAT_NV21_ZSL:
        case GBM_FORMAT_YCbCr_420_888:
        case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
        case GBM_FORMAT_RAW10:
        case GBM_FORMAT_RAW12:
        case GBM_FORMAT_RAW16:
        case GBM_FORMAT_RAW8:
        case GBM_FORMAT_BLOB:
#ifdef COLOR_FMT_NV12_512
        case GBM_FORMAT_NV12_HEIF:
#endif
        case GBM_FORMAT_YCbCr_420_P010_VENUS:
        case GBM_FORMAT_YCbCr_422_I:
        case GBM_FORMAT_YCrCb_422_I:
        case GBM_FORMAT_RGB161616F:
        case GBM_FORMAT_RGB323232F:
        case GBM_FORMAT_RGBA16161616F:
        case GBM_FORMAT_RGBA32323232F:
            is_supported = 1;
            LOG(LOG_DBG,"Valid format\n");
            break;
        default:
            is_supported = 0;
    }

    return is_supported;
}

static int
is_format_rgb(uint32_t format)
{
    int result;

    switch(format)
    {
        case GBM_FORMAT_R8:
        case GBM_FORMAT_RG88:
        case GBM_FORMAT_R16:
        case GBM_FORMAT_RG1616:
        case GBM_FORMAT_RGB565:
        case GBM_FORMAT_BGR565:
        case GBM_FORMAT_RGB888:
        case GBM_FORMAT_BGR888:
        case GBM_FORMAT_RGBA8888:
        case GBM_FORMAT_RGBX8888:
        case GBM_FORMAT_XRGB8888:
        case GBM_FORMAT_XBGR8888:
        case GBM_FORMAT_ARGB8888:
        case GBM_FORMAT_ABGR8888:
        case GBM_FORMAT_ABGR2101010:
        case GBM_FORMAT_RGB161616F:
        case GBM_FORMAT_RGB323232F:
        case GBM_FORMAT_RGBA16161616F:
        case GBM_FORMAT_RGBA32323232F:
            result = 1;
            break;
        default:
            result = 0;
            break;
    }

    return result;
}

static int init_metadata(uint32_t mt_size, int meta_fd)
{
    struct meta_data_t *data = NULL;

    data = (struct meta_data_t *)mmap(NULL, mt_size, PROT_READ|PROT_WRITE, MAP_SHARED, meta_fd, 0);
    if (data == MAP_FAILED) {
        LOG(LOG_ERR,"Map failed \n %s\n",strerror(errno));
        return GBM_ERROR_BAD_HANDLE;
    }

    memset(data, 0 , mt_size);

    LOG(LOG_DBG,"data->igc=%d\n",data->igc);
    LOG(LOG_DBG,"data->color_space=%d\n",data->color_space);
    LOG(LOG_DBG,"data->interlaced=%d\n",data->interlaced);
    LOG(LOG_DBG,"data->is_buffer_secure=%d\n",data->is_buffer_secure);
    LOG(LOG_DBG,"data->linear_format=%d\n",data->linear_format);
    LOG(LOG_DBG,"data->map_secure_buffer=%d\n",data->map_secure_buffer);
    LOG(LOG_DBG,"data->operation\n=%d\n",data->operation);
    LOG(LOG_DBG,"data->refresh_rate=%f\n",data->refresh_rate);
    LOG(LOG_DBG,"data->s3d_format=%d\n",data->s3d_format);

    if(munmap(data, mt_size)){
        LOG(LOG_ERR,"failed to unmap ptr %p\n%s\n",(void*)data, strerror(errno));
        return GBM_ERROR_BAD_VALUE;
    }

    return GBM_ERROR_NONE;
}


static inline uint32_t query_metadata_size(void)
{
    //currently metadata is just a structure
    //But we will enhance in future as metadata info does
    return (ROUND_UP_PAGESIZE(sizeof(struct meta_data_t)));
}


static uint32_t GetUsageFromModifier(const uint64_t *modifiers,
              const unsigned int count)
{
  uint32_t usage = 0;
   for (unsigned int i = 0; i < count ; i++) {
     if (modifiers[i] == DRM_FORMAT_MOD_QCOM_COMPRESSED) {
       usage |= GBM_BO_USAGE_UBWC_ALIGNED_QTI;
     }
     if (modifiers[i] == DRM_FORMAT_MOD_QCOM_DX) {
       usage |= GBM_BO_USAGE_VIDEO_ENCODER_QTI;
     }
     if (modifiers[i] == DRM_FORMAT_MOD_QCOM_TIGHT) {
       usage |= GBM_BO_USAGE_10BIT_TP_QTI;
     }
     if (modifiers[i] == DRM_FORMAT_MOD_QCOM_TILE) {
       usage |= GBM_BO_USAGE_VIDEO_ENCODER_QTI;
     }
   }
  return usage;
}

static int
msmgbm_get_format_modifier_plane_count(uint32_t format,
                                       uint64_t modifier)
{
  // modifier is not used.
  bool valid_rgb_format = is_valid_rgb_fmt(format);
  if(valid_rgb_format && modifier==DRM_FORMAT_MOD_QCOM_COMPRESSED) {
    // RGB UBWC format have 2 planes.
    return 2;
  }
  int plane_count = 1;
  switch (format) {
    case GBM_FORMAT_YCbCr_420_SP:
    case GBM_FORMAT_YCbCr_422_SP:
    case GBM_FORMAT_YCbCr_420_SP_VENUS:
    case GBM_FORMAT_NV12_ENCODEABLE:
    case GBM_FORMAT_NV12:
#ifdef COLOR_FMT_NV12_512
    case GBM_FORMAT_NV12_HEIF:
#endif
    case GBM_FORMAT_YCrCb_420_SP:
    case GBM_FORMAT_YCrCb_422_SP:
    case GBM_FORMAT_YCrCb_420_SP_VENUS:
    case GBM_FORMAT_NV21_ZSL:
    case GBM_FORMAT_RAW16:
    case GBM_FORMAT_RAW10:
    case GBM_FORMAT_YCbCr_420_P010_VENUS:
    case GBM_FORMAT_P010:
    case GBM_FORMAT_YCbCr_422_I:
    case GBM_FORMAT_YCrCb_422_I:
      plane_count = 2;
    break;
    case GBM_FORMAT_YV12:
      plane_count = 3;
    break;
    case GBM_FORMAT_YCbCr_420_TP10_UBWC:
    case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
    case GBM_FORMAT_YCbCr_420_P010_UBWC:
      plane_count = 4;
    break;
    case GBM_FORMAT_BLOB:
    case GBM_FORMAT_RAW12:
    case GBM_FORMAT_RAW_OPAQUE:
    case GBM_FORMAT_RAW8:
      plane_count = 1;
      break;
    default:
    plane_count = 1;
      break;
  }
return plane_count;
}

static struct gbm_bo *
msmgbm_bo_create(struct gbm_device *gbm,
              uint32_t width, uint32_t height,
              uint32_t format, uint32_t usage,
              const uint64_t *modifiers,
              const unsigned int count)
{
    int ret = 0;
    int drm_fd = -1;    // Master fd for DRM
    void *base = NULL;
    void *mt_base = NULL;
    uint32_t aligned_width;
    uint32_t aligned_height;
    uint32_t bo_handles[4] = {0};
    uint32_t pitches[4] = {0};
    uint32_t flags = 0;
    uint32_t Bpp = 0;
    uint32_t size = 0;
    uint32_t mt_size = 0;
    uint32_t gem_handle = 0;
    uint32_t mt_gem_handle = 0;
    struct msmgbm_device *msm_dev = to_msmgbm_device(gbm);
    struct gbm_bo *gbmbo = NULL;
    struct msmgbm_bo *msm_gbmbo = NULL;
    int data_fd = 0;
    int mt_data_fd = 0;
#ifndef TARGET_ION_ABI_VERSION
    struct ion_handle_data handle_data;
    struct ion_fd_data fd_data;
    struct ion_handle_data mt_handle_data;
    struct ion_fd_data mt_fd_data;
#endif
    struct ion_allocation_data ionAllocData;
    struct drm_prime_handle drm_args;
    /* Callers of this may specify a modifier, or a dri usage, but not both. The
     * newer modifier interface deprecates the older usage flags.
     */
   if(usage && count) {
     LOG(LOG_ERR," Usage and modifier both cannot be supplied \n");
     return NULL;
   }

   if(count > 0 && count <= MAX_NUM_MODIFIERS) {
      usage = GetUsageFromModifier(modifiers, count);
   }

    struct gbm_bufdesc bufdesc={width,height,format,usage};

    if(msm_dev == NULL){
        LOG(LOG_ERR,"INVALID Device pointer\n");
        return NULL;
    }

    if(width  == 0 || height ==0){
        LOG(LOG_ERR,"INVALID width or height\n");
        return NULL;
    }
    if(true == IsImplDefinedFormat(format)){
        format = GetImplDefinedFormat(usage, format);
        bufdesc.Format = format;
    }
    if(1 == IsFormatSupported(format))
        Bpp = GetFormatBpp(format);
    else
    {
        LOG(LOG_ERR,"Format (0x%x) not supported\n",format);
        return NULL;
    }

    /*Currently by default we query the aligned dimensions from
      adreno utils*/
    qry_aligned_wdth_hght(&bufdesc, &aligned_width, &aligned_height);

    size = qry_size(&bufdesc, aligned_width, aligned_height);

    LOG(LOG_DBG,"\n size=%d\n width=%d\n height=%d\n aligned_width=%d\n"
          " aligned_height=%d\n",size, width, height, aligned_width, aligned_height);

    /* First we will get ion_fd and gem handle for the frame buffer
     * Size of the ION buffer is in accordance to returned from the adreno helpers
     * Alignment of the buffer is fixed to Page size
     * ION Memory is from, the System heap
     * We get the gem handle from the ion fd using PRIME ioctls
     */
    memset(&ionAllocData, 0, sizeof(ionAllocData));
#ifndef TARGET_ION_ABI_VERSION
    memset(&fd_data, 0, sizeof(fd_data));
    memset(&handle_data, 0, sizeof(handle_data));
#endif

    /*
     * Depending on the usage flag settinggs we check for the heap from which the ION buffer
     * has to be allocated from.
     * Also cache/non cache buffer allocation
     */
    ionAllocData.heap_id_mask = GetIonHeapId(usage);
    ionAllocData.flags = GetIonAllocFlags(usage);

    ionAllocData.len = size;
#ifndef TARGET_ION_ABI_VERSION
    ionAllocData.align = PAGE_SIZE; /*Page size */
#endif

    //This ioctl should have failed for a wrong fd, but it does not returns 0
    if(!(ioctl(msm_dev->iondev_fd, ION_IOC_ALLOC, &ionAllocData))){
#ifdef TARGET_ION_ABI_VERSION

        data_fd = ionAllocData.fd;
        LOG(LOG_DBG,"ionAllocData.fd := %p\n",ionAllocData.fd);

        //Do not mmap if it is secure operation.
        if(!(ionAllocData.flags & ION_FLAG_SECURE)) {
            base = mmap(NULL,size, PROT_READ|PROT_WRITE, MAP_SHARED,
                    ionAllocData.fd, 0);
            if(base == MAP_FAILED) {
                LOG(LOG_ERR,"mmap failed memory on BO Err:\n%s\n",strerror(errno));
                return NULL;
            }
            LOG(LOG_DBG,"BO Mapped Addr:= %p\n",base);
        }
#else
        fd_data.handle = ionAllocData.handle;
        handle_data.handle = ionAllocData.handle;
        LOG(LOG_DBG,"fd_data.handle:= %p\n",fd_data.handle);
        LOG(LOG_DBG,"ionAllocData.handle:= %p\n",ionAllocData.handle);

        if(!(ioctl(msm_dev->iondev_fd, ION_IOC_MAP, &fd_data))){

            data_fd = fd_data.fd;
            LOG(LOG_DBG,"fd_data.fd:= %d\n",fd_data.fd);

            //Do not mmap if it is secure operation.
            if(!(ionAllocData.flags & ION_FLAG_SECURE)) {
                base = mmap(NULL,size, PROT_READ|PROT_WRITE, MAP_SHARED,
                        fd_data.fd, 0);
                if(base == MAP_FAILED) {
                    LOG(LOG_ERR,"mmap failed memory on BO Err:\n%s\n",strerror(errno));
                    ioctl(msm_dev->iondev_fd, ION_IOC_FREE, &handle_data);
                    return NULL;
                }
                LOG(LOG_DBG,"BO Mapped Addr:= %p\n",base);
            }
        }else{
            LOG(LOG_ERR,"ION_IOC_MAP failed on BO Err:\n%s\n",strerror(errno));
            ioctl(msm_dev->iondev_fd, ION_IOC_FREE, &handle_data);
            return NULL;
        }
#endif
    }else{
        LOG(LOG_ERR,"Failed ION_IOC_ALLOC on BO Err:\n%s\n",strerror(errno));
        return NULL;
    }

    //Use PRIME ioctl to convert to GEM handle
    memset(&drm_args, 0, sizeof(drm_args));
    if(msm_dev->fd > 0)
    {
        if(data_fd >0)
        {
            //Perform DRM IOCTL FD to Handle
            drm_args.fd = data_fd;
            if(ioctl(msm_dev->fd,DRM_IOCTL_PRIME_FD_TO_HANDLE, &drm_args))
            {
                LOG(LOG_DBG,"DRM_IOCTL_PRIME_FD_TO_HANDLE failed =%d\n%s\n",
                                                          data_fd,strerror(errno));
                drm_args.handle = 0;
            } else {
                LOG(LOG_DBG,"Get Gem Handle[%u] from fd[%d]\n", drm_args.handle, drm_args.fd);
                lock();
                incr_handle_refcnt(msm_dev->fd, drm_args.handle);
                unlock();
            }
        }
        else
        {
            LOG(LOG_ERR,"ION_IOC_MAP failed on BO Err:\n%s\n",strerror(errno));
            return NULL;
        }

    }else
    {
        LOG(LOG_ERR,"DRM open failed error = %d\n%s\n",strerror(errno));
        return NULL;
    }

    gem_handle=drm_args.handle;
    LOG(LOG_DBG," Gem Handle for BO =:%p\n",gem_handle);

#ifndef TARGET_ION_ABI_VERSION
    //Free the ION Handle since we have the fd to deal with
    if(ioctl(msm_dev->iondev_fd, ION_IOC_FREE, &handle_data)){
        LOG(LOG_ERR," Failed to do ION_IOC_FREE  on BO Err:\n %s\n",
                                                   strerror(errno));
        return NULL;
    }
#endif

    /* To get ion_fd and gem handle for the metadata structure
     * Alignment of the buffer is fixed to Page size
     * ION Memory is from, the System heap
     * We get the gem handle from the ion fd using PRIME ioctls
     */

   //Reset the data objects to be used for ION IOCTL's
    memset(&ionAllocData, 0, sizeof(ionAllocData));
#ifndef TARGET_ION_ABI_VERSION
    memset(&mt_fd_data, 0, sizeof(mt_fd_data));
    memset(&handle_data, 0, sizeof(handle_data));
#endif

    ionAllocData.len = sizeof(struct meta_data_t);
#ifndef TARGET_ION_ABI_VERSION
    ionAllocData.align = 4096; /*Page size */
#endif
    ionAllocData.heap_id_mask= ION_HEAP(ION_SYSTEM_HEAP_ID); /* System Heap */
    ionAllocData.flags |= ION_FLAG_CACHED;

    mt_size = ionAllocData.len;

    if((ioctl(msm_dev->iondev_fd, ION_IOC_ALLOC, &ionAllocData)) == 0){
#ifdef TARGET_ION_ABI_VERSION

        mt_data_fd = ionAllocData.fd;
        LOG(LOG_DBG,"ionAllocData.fd:= %d\n",ionAllocData.fd);

        mt_base = msmgbm_cpu_map_metafd(ionAllocData.fd, mt_size);
        if(mt_base == NULL) {
            LOG(LOG_ERR,"Failed to do  mapping on Metadata BO Err:\n%s\n",strerror(errno));
            return NULL;
        }
        LOG(LOG_DBG,"MT_BO Mapped Addr:= %p\n",mt_base);

        // Initiliaze the meta_data structure
        memset(mt_base, 0 , mt_size);
#else
        mt_fd_data.handle = ionAllocData.handle;
        mt_handle_data.handle = ionAllocData.handle;


        if((ioctl(msm_dev->iondev_fd, ION_IOC_MAP, &mt_fd_data)) == 0){
            mt_data_fd = mt_fd_data.fd;
            LOG(LOG_DBG,"mt_fd_data.fd:= %d\n",mt_fd_data.fd);

            mt_base = msmgbm_cpu_map_metafd(mt_fd_data.fd, mt_size);
            if(mt_base == NULL) {
                LOG(LOG_ERR,"Failed to do  mapping on Metadata BO Err:\n%s\n",strerror(errno));
                ioctl(msm_dev->iondev_fd, ION_IOC_FREE, &mt_handle_data);
                return NULL;
            }
            LOG(LOG_DBG,"MT_BO Mapped Addr:= %p\n",mt_base);

             // Initiliaze the meta_data structure
             memset(mt_base, 0 , mt_size);
        }else
        {
            LOG(LOG_ERR,"ION_IOC_MAP failed on Metadata BO Err:\n%s\n",strerror(errno));
            ioctl(msm_dev->iondev_fd, ION_IOC_FREE, &mt_handle_data);
            return NULL;
        }
#endif

    }else
    {
        LOG(LOG_ERR,"Failed ION_IOC_ALLOC on Metadata BO Err:\n%s\n",strerror(errno));
        return NULL;
    }

    //Use PRIME ioctl to convert to GEM handle
    memset(&drm_args, 0, sizeof(drm_args));
    //Use drm fd returned from previous drmOpen API
    if(mt_data_fd >0)
    {
        //Perform DRM IOCTL FD to Handle
        drm_args.fd = mt_data_fd;
        if(ioctl(msm_dev->fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &drm_args))
        {
            LOG(LOG_DBG,"failed to import gem_handle for Metadata from prime_fd=%d\n%s\n",strerror(errno));
        }
        else
        {
            LOG(LOG_DBG,"Get Metadata Gem Handle[%u] from fd[%d]\n", drm_args.handle, drm_args.fd);
            lock();
            incr_handle_refcnt(msm_dev->fd, drm_args.handle);
            unlock();
        }
    }
    else
    {
        LOG(LOG_ERR,"ION_IOC_MAP failed for Metadata Err:\n%s\n",strerror(errno));
        return NULL;
    }

    mt_gem_handle=drm_args.handle;
    LOG(LOG_DBG,"Gem Handle for Metadata =:%p\n",mt_gem_handle);

#ifndef TARGET_ION_ABI_VERSION
    //Free the ION Handle since we have the fd to deal with
    if(ioctl(msm_dev->iondev_fd, ION_IOC_FREE, &mt_handle_data)){
        LOG(LOG_ERR," Failed to do ION_IOC_FREE  on Metadata BO Err:\n %s\n",
                                                            strerror(errno));
        return NULL;
    }
#endif
    //Update the secure buffer flag info
    if(usage & GBM_BO_USAGE_PROTECTED_QTI)
    {
        struct meta_data_t *data = (struct meta_data_t *)mt_base;
        data->is_buffer_secure = true;
        LOG(LOG_DBG,"Updating the Secure Buffer status =:%d\n",data->is_buffer_secure);
    }

    // Update UBWC buffer flag info
    if (is_ubwc_enabled(format, usage, usage)) {
        struct meta_data_t *data = (struct meta_data_t *)mt_base;
        data->is_buffer_ubwc = true;
        LOG(LOG_DBG,"Updating the UBWC buffer status =:%d\n",data->is_buffer_ubwc);
    }

    //Create a gbm_buf_info and add entry to the hashmap
    struct gbm_buf_info gbo_info;
    struct msmgbm_private_info gbo_private_info = {NULL, NULL};
    gbo_info.fd = data_fd;
    gbo_info.metadata_fd = mt_data_fd;
    gbo_info.format = format;
    gbo_info.height = height;
    gbo_info.width  = width;
    //add cpu address and metadata address of bo to hashmap
    gbo_private_info.cpuaddr = base;
    gbo_private_info.mt_cpuaddr = mt_base;

    LOG(LOG_DBG," MAP registered bo info gbo_info =:%p\n",&gbo_info);

    //Let us lock and unlock mutex
    lock();
    register_to_hashmap(data_fd,&gbo_info, &gbo_private_info);
    incr_refcnt(data_fd);
    unlock();

    /*
     * Initialize the gbm bo object with the handle's and fd's
     */
    msm_gbmbo = (struct msmgbm_bo *)calloc(1, sizeof(struct msmgbm_bo));

    if (msm_gbmbo == NULL) {
        LOG(LOG_ERR,"Unable to allocate BO\n");
        return NULL;
    }
    int plane_count = msmgbm_get_format_modifier_plane_count(format, 0);


    gbmbo = &msm_gbmbo->base;
    for (int m = 0; m < count; m++) {
       gbmbo->modifier |= modifiers[m];
    }

    gbmbo->ion_fd = data_fd;
    gbmbo->ion_metadata_fd = mt_data_fd;
    gbmbo->handle.u32 = gem_handle;
    gbmbo->metadata_handle.u32 = mt_gem_handle;
    gbmbo->fbid = 0;                                     //$ drmModeAddFB2 ?
    gbmbo->plane_count = plane_count;
    gbmbo->format = format;
    gbmbo->width  = width;                               //BO width
    gbmbo->height = height;                              //BO height
    gbmbo->stride = aligned_width*Bpp;
    gbmbo->size = size;                                 // Calculated by qry_size
    gbmbo->usage_flags = usage;
    gbmbo->aligned_width = aligned_width;
    gbmbo->aligned_height = aligned_height;
    gbmbo->bo_destroy = msmgbm_bo_destroy;
    gbmbo->bo_get_fd = msmgbm_bo_get_fd;
    gbmbo->bo_unmap = msmgbm_bo_unmap;
    gbmbo->bo_map = msmgbm_bo_map;
    gbmbo->stride_for_plane = msmgbm_stride_for_plane;
    gbmbo->bo_get_device = msmgbm_bo_get_device;
    gbmbo->bo_write = msmgbm_bo_write;
    gbmbo->bpp = Bpp;
    msm_gbmbo->device = msm_dev;
    msm_gbmbo->cpuaddr = base;
    msm_gbmbo->mt_cpuaddr = mt_base;
    msm_gbmbo->current_state =  GBM_BO_STATE_FREE;
    msm_gbmbo->size = size;
    msm_gbmbo->mt_size = mt_size;
    msm_gbmbo->magic = QCMAGIC;
#ifndef TARGET_ION_ABI_VERSION
    msm_gbmbo->ion_handle = handle_data.handle;
    msm_gbmbo->ion_mt_handle = mt_handle_data.handle;
#endif

    bo_handles[0] = gbmbo->handle.u32;
    pitches[0] = gbmbo->stride;
    msmgbm_yuv_plane_info(gbmbo,&(gbmbo->buf_lyt));
    return gbmbo;
}

struct gbm_bo *
msmgbm_bo_import_fd(struct msmgbm_device *msm_dev,
                                                      void *buffer, uint32_t usage)
{
    struct gbm_bo *gbmbo = NULL;
    struct msmgbm_bo *msm_gbmbo = NULL;
    struct drm_prime_handle gemimport_req;
    struct drm_prime_handle mtdadta_gemimport_req;
    struct gbm_import_fd_data *buffer_info = (struct gbm_import_fd_data *)buffer;
    struct gbm_device* gbm_dev = &(msm_dev->base);
    struct gbm_bufdesc bufdesc;
    int ret = 0;
    int Bpp=0;
    unsigned int size = 0, mt_size = 0;
    unsigned int aligned_width;
    unsigned int aligned_height;

    if (buffer_info == NULL){
        LOG(LOG_ERR,"INVALID buffer_info\n");
        return NULL;
    }

    if(msm_dev == NULL){
        LOG(LOG_ERR,"INVALID Device pointer\n");
        return NULL;
    }

    if(buffer_info->fd < 0)
    {
        LOG(LOG_ERR,"INVALID File descriptor=%d\n",buffer_info->fd);
        return NULL;
    }

    //Query Map
    struct gbm_buf_info gbo_info;
    struct msmgbm_private_info gbo_private_info = {NULL, NULL};

    lock();
    if(search_hashmap(buffer_info->fd, &gbo_info, &gbo_private_info) == GBM_ERROR_NONE)
    {
        LOG(LOG_DBG,"Map retrieved buf info\n");
        LOG(LOG_DBG,"gbm_buf_info.fd=%d,gbm_buf_info.metadata_fd=%d\n"
                    "gbm_buf_info.width=%d gbm_buf_info.height=%d\n"
                    "gbm_buf_info.format=%d\n"
                    "gbo_private_info.cpuaddr=%p gbo_private_info.mt_cpuaddr=%p\n",
                    gbo_info.fd, gbo_info.metadata_fd, gbo_info.width, gbo_info.height,
                    gbo_info.format, gbo_private_info.cpuaddr, gbo_private_info.mt_cpuaddr);

        //we have a valid entry within the map table so Increment ref count
        incr_refcnt(buffer_info->fd);
    }
    else
    {
        LOG(LOG_INFO,"Search failed so register_to_map\n",
                                                    __func__,__LINE__);
        //Copy the buffer info credentials
        gbo_info.fd=buffer_info->fd;
        gbo_info.metadata_fd = -1; //since we do not have meta fd info here
        gbo_info.format=buffer_info->format;
        gbo_info.width=buffer_info->width;
        gbo_info.height=buffer_info->height;

        //we cannot map cpu address as we dont have a reliable way to find
        //whether ion fd is secure or not since metadata_fd is not present
        register_to_hashmap(buffer_info->fd, &gbo_info, &gbo_private_info);
        incr_refcnt(buffer_info->fd);
    }
    unlock();

    LOG(LOG_DBG," format: 0x%x width: %d height: %d \n",buffer_info->format, buffer_info->width, buffer_info->height);

    if(1 == IsFormatSupported(buffer_info->format))
        Bpp = GetFormatBpp(buffer_info->format);
    else
    {
        LOG(LOG_ERR,"Format (0x%x) not supported\n",buffer_info->format);
        return NULL;
    }


    /* Import the gem handle for image BO */
    memset(&gemimport_req, 0, sizeof(gemimport_req));
    gemimport_req.fd = buffer_info->fd;

    ret = ioctl(msm_dev->fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &gemimport_req);

    if (ret != 0){
        LOG(LOG_DBG,"PRIME FD to Handle failed on device(%x), error = %d\n",msm_dev,ret);
        gemimport_req.handle = 0;
    }
    else
    {
        LOG(LOG_DBG,"Get Gem Handle[%u] from fd[%d]\n", gemimport_req.handle, gemimport_req.fd);
        lock();
        incr_handle_refcnt(msm_dev->fd, gemimport_req.handle);
        unlock();
    }

    memset(&mtdadta_gemimport_req, 0, sizeof(mtdadta_gemimport_req));

    //Initialize the helper structure
    bufdesc.Width  = buffer_info->width;
    bufdesc.Height = buffer_info->height;
    bufdesc.Format = buffer_info->format;
    bufdesc.Usage  = usage;

    mt_size = query_metadata_size();
    if (gbo_info.metadata_fd != -1) {
        // Check whether imported gbm bo was UBWC allocated.
        struct meta_data_t *meta_data;
        meta_data = (struct meta_data_t *)gbo_private_info.mt_cpuaddr;
        if (meta_data->is_buffer_ubwc) {
            bufdesc.Usage |= GBM_BO_USAGE_UBWC_ALIGNED_QTI | GBM_BO_USAGE_HW_RENDERING_QTI;
        }
    }


    /*Query the size*/
    /*Currently by default we query the aligned dimensions from
      adreno utils*/
    qry_aligned_wdth_hght(&bufdesc, &aligned_width, &aligned_height);
    size = qry_size(&bufdesc, aligned_width, aligned_height);

    msm_gbmbo = (struct msmgbm_bo *)calloc(1, sizeof(struct msmgbm_bo));

    if (msm_gbmbo == NULL) {
        LOG(LOG_ERR,"Unable to allocate BO\n");
        return NULL;
    }

    gbmbo                = &msm_gbmbo->base;
    gbmbo->ion_fd        = buffer_info->fd;
    gbmbo->ion_metadata_fd = gbo_info.metadata_fd;
    gbmbo->handle.u32    = gemimport_req.handle;
    gbmbo->usage_flags   = bufdesc.Usage;
    gbmbo->format        = buffer_info->format;
    gbmbo->width         = buffer_info->width;
    gbmbo->height        = buffer_info->height;
    gbmbo->stride        = Bpp*aligned_width;
    gbmbo->size          = size;
    gbmbo->aligned_width  = aligned_width;
    gbmbo->aligned_height = aligned_height;
    gbmbo->bo_destroy    = msmgbm_bo_destroy;
    gbmbo->bo_get_fd     = msmgbm_bo_get_fd;
    gbmbo->stride_for_plane = msmgbm_stride_for_plane;
    gbmbo->bo_get_device = msmgbm_bo_get_device;
    gbmbo->bo_write      = msmgbm_bo_write;
    msm_gbmbo->device    = msm_dev;
    msm_gbmbo->cpuaddr   = gbo_private_info.cpuaddr;
    msm_gbmbo->mt_cpuaddr   = gbo_private_info.mt_cpuaddr;
    msm_gbmbo->current_state   =  GBM_BO_STATE_FREE;
    gbmbo->metadata_handle.u32 = NULL;
    msm_gbmbo->size      = size;
    msm_gbmbo->mt_size   = mt_size;
    msm_gbmbo->magic     = QCMAGIC;
    msm_gbmbo->import_flg = GBM_BO_IMPORT_FD;

    msmgbm_yuv_plane_info(gbmbo,&(gbmbo->buf_lyt));

    LOG(LOG_DBG,"Imported BO Info as below:\n");
    LOG(LOG_DBG,"gbmbo->ion_fd=%d,gbmbo->ion_metadata_fd=%d,"
        "gbmbo->width=%d,gbmbo->height=%d,gbmbo->format=0x%x\n",
        gbmbo->ion_fd,gbmbo->ion_metadata_fd,gbmbo->width,
        gbmbo->height,gbmbo->format);

    return gbmbo;

}

struct gbm_bo *
msmgbm_bo_import_wl_buffer(struct msmgbm_device *msm_dev,
                                                      void *buffer, uint32_t usage)
{
    struct gbm_bo *gbmbo = NULL;
    struct msmgbm_bo *msm_gbmbo = NULL;
    struct drm_prime_handle gemimport_req;
    struct drm_prime_handle mtdadta_gemimport_req;
    struct wl_resource* resource = NULL;
    struct gbm_buf_info *buffer_info = NULL;
    struct gbm_device* gbm_dev = &(msm_dev->base);
    struct gbm_bufdesc bufdesc;
    int ret = 0;
    int Bpp=0;
    unsigned int size = 0, mt_size = 0;
    unsigned int aligned_width;
    unsigned int aligned_height;
    int register_map = 0;
    struct meta_data_t *mt_cpuaddr;
    //create gbm_buf_info and private_info to add to hashmap
    struct gbm_buf_info gbo_info;
    struct msmgbm_private_info gbo_private_info = {NULL, NULL};


    resource = (struct wl_resource*)(buffer);
    if (resource == NULL){
        LOG(LOG_ERR,"INVALID buffer_info\n");
        return NULL;
    }

    if(msm_dev == NULL){
        LOG(LOG_ERR,"INVALID Device pointer\n");
        return NULL;
    }

    buffer_info = wl_resource_get_user_data(resource);
    if (buffer_info == NULL){
        LOG(LOG_ERR,"INVALID buffer\n");
        return NULL;
    }

    if(buffer_info->fd < 0)
    {
       LOG(LOG_ERR,"INVALID File descriptor(%d)\n",buffer_info->fd);
       return NULL;
    }

    LOG(LOG_DBG,"format: 0x%x width: %d height: %d\n",buffer_info->format, buffer_info->width, buffer_info->height);

    if(1 == IsFormatSupported(buffer_info->format))
        Bpp = GetFormatBpp(buffer_info->format);
    else
    {
        LOG(LOG_ERR," Format (0x%x) not supported\n",
                                                buffer_info->format);
        return NULL;
    }

    //Search Map for a valid entry
    lock();
    ret = search_hashmap(buffer_info->fd, &gbo_info, &gbo_private_info);
    unlock();
    if(ret != GBM_ERROR_NONE) {
        register_map = 1;
    }

    //Initialize the helper structure
    bufdesc.Width  = buffer_info->width;
    bufdesc.Height = buffer_info->height;
    bufdesc.Format = buffer_info->format;
    bufdesc.Usage  = usage;

    mt_size = query_metadata_size();
    //if metadata cpuaddress not found in hashmap, call mmap
    if(gbo_private_info.mt_cpuaddr == NULL) {
        if(buffer_info->metadata_fd > 0) {
            gbo_private_info.mt_cpuaddr = msmgbm_cpu_map_metafd(buffer_info->metadata_fd, mt_size);
            LOG(LOG_DBG, "Meta cpu addr = %p created for ion_fd = %d, meta_ion_fd=%d \n",
                gbo_private_info.mt_cpuaddr, buffer_info->fd, buffer_info->metadata_fd);
        }
    }
    mt_cpuaddr = (struct meta_data_t *)gbo_private_info.mt_cpuaddr;

    /*Query the size*/
    /*Currently by default we query the aligned dimensions from
      adreno utils*/
    qry_aligned_wdth_hght(&bufdesc, &aligned_width, &aligned_height);
    size = qry_size(&bufdesc, aligned_width, aligned_height);

    //if ion fd cpu address not found in hashmap, call mmap
    if(gbo_private_info.cpuaddr == NULL) {
        if(mt_cpuaddr != NULL) {
            LOG(LOG_DBG, "ION fd cpu addr = %p created for ion_fd = %d\n",
                gbo_private_info.cpuaddr, buffer_info->fd);
            gbo_private_info.cpuaddr = msmgbm_cpu_map_ionfd(buffer_info->fd, size, mt_cpuaddr);
        }
    }

    //register map if ion_fd entry does not exist or update map if ion_fd is found
    if(register_map == 0)
    {
        LOG(LOG_DBG,"Map retrieved buf info\n gbm_buf_info.width=%d\n",
                                                          gbo_info.width);
        LOG(LOG_DBG,"gbm_buf_info.height=%d\n gbm_buf_info.format = %d\n",
                      gbo_info.height,gbo_info.format);

        lock();
        //We will check if it has a valid metadata fd and update the same
        if((buffer_info->metadata_fd > 0) && (buffer_info->metadata_fd != gbo_info.metadata_fd))
        {
           //Since we have already made sure entry exists
            update_hashmap(buffer_info->fd, buffer_info, &gbo_private_info);
        }
        //If we have a valid entry within the map table then Increment ref count
        incr_refcnt(buffer_info->fd);
        unlock();
    }
    else
    {
        lock();
        register_to_hashmap(buffer_info->fd, buffer_info, &gbo_private_info);
        incr_refcnt(buffer_info->fd);
        unlock();
    }

    /* Import the gem handle for image BO */
    memset(&gemimport_req, 0, sizeof(gemimport_req));
    gemimport_req.fd = buffer_info->fd;

    ret = ioctl(msm_dev->fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &gemimport_req);

    if (ret != 0){
        LOG(LOG_DBG,"PRIME FD to Handle failed on device(%x), error = %d\n",msm_dev,ret);
    }

    memset(&mtdadta_gemimport_req, 0, sizeof(mtdadta_gemimport_req));

    if(buffer_info->metadata_fd < 0)
        LOG(LOG_DBG,"INVALID Metadata File descriptor provided(%d)\n",buffer_info->metadata_fd);
    else
    {
        /* Import the gem handle for metadata BO */
        mtdadta_gemimport_req.fd = buffer_info->metadata_fd;

        ret = ioctl(msm_dev->fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &mtdadta_gemimport_req);

        if (ret != 0){
            LOG(LOG_DBG,"PRIME FD to Handle failed on device(%x), error = %d\n",msm_dev,ret);
        }
    }

    msm_gbmbo = (struct msmgbm_bo *)calloc(1, sizeof(struct msmgbm_bo));

    if (msm_gbmbo == NULL) {
        LOG(LOG_ERR,"Unable to allocate BO\n");
        return NULL;
    }

    gbmbo                = &msm_gbmbo->base;
    gbmbo->ion_fd        = buffer_info->fd;
    gbmbo->ion_metadata_fd = buffer_info->metadata_fd;
    gbmbo->handle.u32    = gemimport_req.handle;
    gbmbo->usage_flags   = usage;
    gbmbo->format        = buffer_info->format;
    gbmbo->width         = buffer_info->width;
    gbmbo->height        = buffer_info->height;
    gbmbo->stride        = Bpp*aligned_width;
    gbmbo->size          = size;
    gbmbo->aligned_width  = aligned_width;
    gbmbo->aligned_height = aligned_height;
    gbmbo->bo_destroy    = msmgbm_bo_destroy;
    gbmbo->bo_get_fd     = msmgbm_bo_get_fd;
    gbmbo->stride_for_plane = msmgbm_stride_for_plane;
    gbmbo->bo_get_device = msmgbm_bo_get_device;
    gbmbo->bo_write      = msmgbm_bo_write;
    msm_gbmbo->device    = msm_dev;
    msm_gbmbo->cpuaddr  = gbo_private_info.cpuaddr;
    msm_gbmbo->mt_cpuaddr = gbo_private_info.mt_cpuaddr;
    msm_gbmbo->current_state   =  GBM_BO_STATE_FREE;
    gbmbo->metadata_handle.u32 = mtdadta_gemimport_req.handle;
    msm_gbmbo->size      = size;
    msm_gbmbo->mt_size   = mt_size;
    msm_gbmbo->magic     = QCMAGIC;
    msm_gbmbo->import_flg = 1;

    msmgbm_yuv_plane_info(gbmbo,&(gbmbo->buf_lyt));
    return gbmbo;

}

struct gbm_bo *
msmgbm_bo_import_egl_image(struct msmgbm_device *msm_dev,
                                                      void *buffer, uint32_t usage)
{
    //TODO: Need to know how to get either a name or FD for this egl image
    LOG(LOG_ERR,"GBM_BO_IMPORT_EGL_IMAGE not supported\n");
    return NULL;
}

struct gbm_bo *
msmgbm_bo_import_fd_modifier(struct msmgbm_device *msm_dev,
                                                      void *buffer, uint32_t usage)
{
  struct gbm_bo *gbmbo = NULL;
  struct msmgbm_bo *msm_gbmbo = NULL;
  int Bpp=0;
  int ret = 0;
  struct gbm_buf_info temp_buf_info;
  struct msmgbm_private_info gbo_private_info = {NULL, NULL};
  struct gbm_bufdesc bufdesc;
  struct gbm_import_fd_modifier_data *fd_data = buffer;
  int register_map = 0; //do not modify these flags
  struct meta_data_t *meta_data = NULL;
  unsigned int aligned_width = 0;
  unsigned int aligned_height = 0;
  struct drm_prime_handle gemimport_req;
  struct drm_prime_handle mtdadta_gemimport_req;
  unsigned int size = 0, mt_size = 0;
  if (fd_data == NULL) {
    LOG(LOG_ERR, "INVALID fd_data\n");
    return NULL;
  }
  if (fd_data->width == 0 || fd_data->height == 0) {
    LOG(LOG_ERR, "Zero widht or height\n");
    return NULL;
  }
  if(1 == IsFormatSupported(fd_data->format)) {
      Bpp = GetFormatBpp(fd_data->format);
  }
  else
  {
      LOG(LOG_ERR,"Format (0x%x) not supported\n", fd_data->format);
      return NULL;
  }
      //Search Map for a valid entry, we have only one FD for all buffers
    lock();
    ret = search_hashmap(fd_data->fds[0], &temp_buf_info, &gbo_private_info);
    unlock();

     //If we have a valid entry within the map table then Increment ref count
    if(ret==GBM_ERROR_NONE)
    {
        LOG(LOG_DBG,"MAP retrieved buf info\n");
        LOG(LOG_DBG,"temp_buf_info.width=%d\n",
                              temp_buf_info.width);
        LOG(LOG_DBG,"temp_buf_info.height=%d\n",
                             temp_buf_info.height);
        LOG(LOG_DBG,"temp_buf_info.format=%d\n",
                                    temp_buf_info.format);
        LOG(LOG_DBG,"temp_buf_info.meta_fd=%d\n",
                                    temp_buf_info.metadata_fd);
    }else
    {
        LOG(LOG_INFO," MAP table is empty\n");
        register_map = 1;
        LOG(LOG_INFO,"Registered fd=%d to table\n",fd_data->fds[0]);
    }
    //Initialize the helper structure
    bufdesc.Width  = fd_data->width;
    bufdesc.Height = fd_data->height;
    bufdesc.Format = fd_data->format;
    bufdesc.Usage = GetUsageFromModifier(&fd_data->modifier, 1);

    mt_size = query_metadata_size();

     meta_data = (struct meta_data_t *)gbo_private_info.mt_cpuaddr;
    // Check whether imported gbm bo was UBWC allocated
    if(meta_data != NULL) {
        if (meta_data->is_buffer_ubwc) {
            bufdesc.Usage |= GBM_BO_USAGE_UBWC_ALIGNED_QTI | GBM_BO_USAGE_HW_RENDERING_QTI;
        }
    }
    /*Query the size*/
    /*Currently by default we query the aligned dimensions from
      adreno utils*/
    qry_aligned_wdth_hght(&bufdesc, &aligned_width, &aligned_height);
    size = qry_size(&bufdesc, aligned_width, aligned_height);
    //if ion fd cpu address not found in hashmap, call mmap
    if((gbo_private_info.cpuaddr == NULL) && (meta_data != NULL)) {
        gbo_private_info.cpuaddr = msmgbm_cpu_map_ionfd(fd_data->fds[0], size, meta_data);
        LOG(LOG_DBG, "ION fd cpu addr = %p created for ion_fd = %d \n",
            gbo_private_info.cpuaddr, fd_data->fds[0]);
    }
    struct gbm_buf_info *buffer_info =  (struct gbm_buf_info *)calloc(1, sizeof(struct gbm_buf_info));
    buffer_info->fd = fd_data->fds[0];
    buffer_info->width = fd_data->width;
    buffer_info->height = fd_data->height;
    buffer_info->format = fd_data->format;
    buffer_info->metadata_fd = -1;

    /* Import the gem handle for image BO */
    memset(&gemimport_req, 0, sizeof(gemimport_req));
    gemimport_req.fd = buffer_info->fd;

    ret = ioctl(msm_dev->fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &gemimport_req);

    if (ret != 0){
        LOG(LOG_DBG,"PRIME FD to Handle failed on device(%x)\n %s\n",
                                               msm_dev,strerror(errno));
        gemimport_req.handle = 0;
    }
    else
    {
        LOG(LOG_DBG,"Get Gem Handle[%u] from fd[%d]\n", gemimport_req.handle, gemimport_req.fd);
        lock();
        incr_handle_refcnt(msm_dev->fd, gemimport_req.handle);
        unlock();
    }

    lock();
    if(register_map) {
        //register fd to hashmap if entry not found
        register_to_hashmap(fd_data->fds[0], buffer_info, &gbo_private_info);
    } else {
         if(temp_buf_info.metadata_fd < 0) {
             //Since we have already made sure entry exists
             //metadata fd was wrong before so update hashmap
             update_hashmap(buffer_info->fd, buffer_info, &gbo_private_info);
         }
    }
    incr_refcnt(buffer_info->fd);
    unlock();

    memset(&mtdadta_gemimport_req, 0, sizeof(mtdadta_gemimport_req));

    msm_gbmbo = (struct msmgbm_bo *)calloc(1, sizeof(struct msmgbm_bo));

    if (msm_gbmbo == NULL) {
        LOG(LOG_ERR," Unable to allocate BO OoM\n");
        return NULL;
    }

    gbmbo                  = &msm_gbmbo->base;
    gbmbo->ion_fd          = buffer_info->fd;
    gbmbo->ion_metadata_fd = buffer_info->metadata_fd;
    gbmbo->handle.u32      = gemimport_req.handle;
    gbmbo->usage_flags     = bufdesc.Usage;
    gbmbo->format          = buffer_info->format;
    gbmbo->width           = buffer_info->width;
    gbmbo->height          = buffer_info->height;
    gbmbo->stride          = Bpp*aligned_width;
    gbmbo->aligned_width   = aligned_width;
    gbmbo->aligned_height  = aligned_height;
    gbmbo->size            = size;
    gbmbo->bo_destroy      = msmgbm_bo_destroy;
    gbmbo->bo_get_fd       = msmgbm_bo_get_fd;
    gbmbo->stride_for_plane = msmgbm_stride_for_plane;
    gbmbo->bo_get_device   = msmgbm_bo_get_device;
    gbmbo->bo_write        = msmgbm_bo_write;
    msm_gbmbo->device      = msm_dev;
    msm_gbmbo->cpuaddr   = gbo_private_info.cpuaddr;
    msm_gbmbo->mt_cpuaddr = gbo_private_info.mt_cpuaddr;
    msm_gbmbo->current_state   =  GBM_BO_STATE_FREE;
    gbmbo->metadata_handle.u32 = mtdadta_gemimport_req.handle;
    msm_gbmbo->size            = size;
    msm_gbmbo->mt_size         = mt_size;
    msm_gbmbo->magic           = QCMAGIC;
    msm_gbmbo->import_flg      = GBM_BO_IMPORT_FD_MODIFIER;

    msmgbm_yuv_plane_info(gbmbo,&(gbmbo->buf_lyt));

    LOG(LOG_DBG,"Imported BO Info as below:\n");
    LOG(LOG_DBG,"gbmbo->ion_fd=%d,gbmbo->ion_metadata_fd=%d,"
        "gbmbo->width=%d,gbmbo->height=%d,gbmbo->format=0x%x\n",
        gbmbo->ion_fd,gbmbo->ion_metadata_fd,gbmbo->width,
        gbmbo->height,gbmbo->format);

    return gbmbo;
}


struct gbm_bo *
msmgbm_bo_import_gbm_buf(struct msmgbm_device *msm_dev,
                                                      void *buffer, uint32_t usage)
{
    struct gbm_bo *gbmbo = NULL;
    struct msmgbm_bo *msm_gbmbo = NULL;
    struct drm_prime_handle gemimport_req;
    struct drm_prime_handle mtdadta_gemimport_req;
    struct gbm_buf_info *buffer_info = NULL;
    struct gbm_device* gbm_dev = &(msm_dev->base);
    struct gbm_bufdesc bufdesc;
    int ret = 0;
    int Bpp=0;
    unsigned int size = 0, mt_size;
    unsigned int aligned_width;
    unsigned int aligned_height;
    bool skip_handle = (usage & GBM_BO_USAGE_EGL_IMAGE_QTI) ? true : false;

    struct meta_data_t *meta_data = NULL;
    struct gbm_buf_info temp_buf_info;
    struct msmgbm_private_info gbo_private_info = {NULL, NULL};
    int register_map = 0; //do not modify these flags

    buffer_info = (struct gbm_buf_info*)(buffer);
    if (buffer_info == NULL){
        LOG(LOG_ERR, "INVALID buffer_info\n");
        return NULL;
    }

    if(msm_dev == NULL){
        LOG(LOG_ERR,"INVALID Device pointer\n");
        return NULL;
    }

    if(buffer_info->fd < 0)
    {
        LOG(LOG_ERR,"INVALID File descriptor(%d)\n", buffer_info->fd);
        return NULL;
    }

    LOG(LOG_DBG," fd=%d format: 0x%x width: %d height: %d \n",buffer_info->fd,
        buffer_info->format, buffer_info->width, buffer_info->height);

    if(1 == IsFormatSupported(buffer_info->format))
        Bpp = GetFormatBpp(buffer_info->format);
    else
    {
        LOG(LOG_ERR,"Format (0x%x) not supported\n", buffer_info->format);
        return NULL;
    }

    //Search Map for a valid entry
    lock();
    ret = search_hashmap(buffer_info->fd, &temp_buf_info, &gbo_private_info);
    unlock();

    //If we have a valid entry within the map table then Increment ref count
    if(ret==GBM_ERROR_NONE)
    {
        LOG(LOG_DBG,"MAP retrieved buf info\n");
        LOG(LOG_DBG,"temp_buf_info.width=%d\n",
                              temp_buf_info.width);
        LOG(LOG_DBG,"temp_buf_info.height=%d\n",
                             temp_buf_info.height);
        LOG(LOG_DBG,"temp_buf_info.format=%d\n",
                                    temp_buf_info.format);
        LOG(LOG_DBG,"temp_buf_info.meta_fd=%d\n",
                                    temp_buf_info.metadata_fd);
    }
    else
    {
        LOG(LOG_INFO," MAP table is empty\n");

        register_map = 1;
        LOG(LOG_INFO,"Registered fd=%d to table\n",buffer_info->fd);
    }

    //Initialize the helper structure
    bufdesc.Width  = buffer_info->width;
    bufdesc.Height = buffer_info->height;
    bufdesc.Format = buffer_info->format;
    bufdesc.Usage  = usage;

    mt_size = query_metadata_size();
    //if metadata cpu address not found in hashmap, call mmap
    if (gbo_private_info.mt_cpuaddr == NULL) {
        if(buffer_info->metadata_fd > 0) {
            gbo_private_info.mt_cpuaddr = msmgbm_cpu_map_metafd(buffer_info->metadata_fd,mt_size);
            LOG(LOG_DBG, "Meta cpu addr = %p created for ion_fd = %d, meta_ion_fd=%d \n",
             gbo_private_info.mt_cpuaddr, buffer_info->fd, buffer_info->metadata_fd);
        }
    } else {
        LOG(LOG_DBG, "Found metadata cpu addr from hashmap for ion fd = %d, ionmetafd=%d, meta_addr=%p\n",
                       buffer_info->fd, buffer_info->metadata_fd, gbo_private_info.mt_cpuaddr);
    }

    meta_data = (struct meta_data_t *)gbo_private_info.mt_cpuaddr;
    // Check whether imported gbm bo was UBWC allocated
    if(meta_data != NULL) {
        if (meta_data->is_buffer_ubwc) {
            bufdesc.Usage |= GBM_BO_USAGE_UBWC_ALIGNED_QTI | GBM_BO_USAGE_HW_RENDERING_QTI;
        }
    }

    /*Query the size*/
    /*Currently by default we query the aligned dimensions from
      adreno utils*/
    qry_aligned_wdth_hght(&bufdesc, &aligned_width, &aligned_height);
    size = qry_size(&bufdesc, aligned_width, aligned_height);

    //if ion fd cpu address not found in hashmap, call mmap
    if((gbo_private_info.cpuaddr == NULL) && (meta_data != NULL)) {
        gbo_private_info.cpuaddr = msmgbm_cpu_map_ionfd(buffer_info->fd, size, meta_data);
        LOG(LOG_DBG, "ION fd cpu addr = %p created for ion_fd = %d \n",
            gbo_private_info.cpuaddr, buffer_info->fd);
    }

    lock();
    if(register_map) {
        //register fd to hashmap if entry not found
        register_to_hashmap(buffer_info->fd, buffer_info, &gbo_private_info);
    } else {
         if(temp_buf_info.metadata_fd < 0) {
             //Since we have already made sure entry exists
             //metadata fd was wrong before so update hashmap
             update_hashmap(buffer_info->fd, buffer_info, &gbo_private_info);
         }
    }
    incr_refcnt(buffer_info->fd);
    unlock();

    /* Import the gem handle for image BO */
    memset(&gemimport_req, 0, sizeof(gemimport_req));
    gemimport_req.fd = buffer_info->fd;

    if (skip_handle) {
        gemimport_req.handle = MAGIC_HANDLE;
    } else {
        ret = ioctl(msm_dev->fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &gemimport_req);
        if (ret != 0) {
            LOG(LOG_DBG,"PRIME FD to Handle failed on device(%x)\n %s\n",
                                               msm_dev,strerror(errno));
            gemimport_req.handle = 0;
        }
        else
        {
            LOG(LOG_DBG,"Get Gem Handle[%u] from fd[%d]\n", gemimport_req.handle, gemimport_req.fd);
            lock();
            incr_handle_refcnt(msm_dev->fd, gemimport_req.handle);
            unlock();
        }
    }

    memset(&mtdadta_gemimport_req, 0, sizeof(mtdadta_gemimport_req));

    if(buffer_info->metadata_fd < 0)
        LOG(LOG_DBG,"INVALID Metadata File descriptor provided(%d)\n",
                                             buffer_info->metadata_fd);
    else
    {

        /* Import the gem handle for metadata BO */
        mtdadta_gemimport_req.fd = buffer_info->metadata_fd;
        if (skip_handle) {
            mtdadta_gemimport_req.handle = MAGIC_HANDLE;
        } else {
            ret = ioctl(msm_dev->fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &mtdadta_gemimport_req);
            if (ret != 0) {
                LOG(LOG_DBG,"PRIME FD to Handle failed on device(%x)\n %s\n",
                                                   msm_dev,strerror(errno));
                mtdadta_gemimport_req.handle = 0;
            }
            else
            {
                LOG(LOG_DBG,"Get Metadata Gem Handle[%u] from fd[%d]\n",
                                mtdadta_gemimport_req.handle, mtdadta_gemimport_req.fd);
                lock();
                incr_handle_refcnt(msm_dev->fd, mtdadta_gemimport_req.handle);
                unlock();
            }
        }
    }


    msm_gbmbo = (struct msmgbm_bo *)calloc(1, sizeof(struct msmgbm_bo));

    if (msm_gbmbo == NULL) {
        LOG(LOG_ERR," Unable to allocate BO OoM\n");
        return NULL;
    }


    gbmbo                  = &msm_gbmbo->base;
    gbmbo->ion_fd          = buffer_info->fd;
    gbmbo->ion_metadata_fd = buffer_info->metadata_fd;
    gbmbo->handle.u32      = gemimport_req.handle;
    gbmbo->usage_flags     = bufdesc.Usage;
    gbmbo->format          = buffer_info->format;
    gbmbo->width           = buffer_info->width;
    gbmbo->height          = buffer_info->height;
    gbmbo->stride          = Bpp*aligned_width;
    gbmbo->aligned_width   = aligned_width;
    gbmbo->aligned_height  = aligned_height;
    gbmbo->size            = size;
    gbmbo->bo_destroy      = msmgbm_bo_destroy;
    gbmbo->bo_get_fd       = msmgbm_bo_get_fd;
    gbmbo->stride_for_plane = msmgbm_stride_for_plane;
    gbmbo->bo_get_device   = msmgbm_bo_get_device;
    gbmbo->bo_write        = msmgbm_bo_write;
    msm_gbmbo->device      = msm_dev;
    msm_gbmbo->cpuaddr   = gbo_private_info.cpuaddr;
    msm_gbmbo->mt_cpuaddr = gbo_private_info.mt_cpuaddr;
    msm_gbmbo->current_state   =  GBM_BO_STATE_FREE;
    gbmbo->metadata_handle.u32 = mtdadta_gemimport_req.handle;
    msm_gbmbo->size            = size;
    msm_gbmbo->mt_size         = mt_size;
    msm_gbmbo->magic           = QCMAGIC;
    msm_gbmbo->import_flg      = GBM_BO_IMPORT_GBM_BUF_TYPE;

    msmgbm_yuv_plane_info(gbmbo,&(gbmbo->buf_lyt));

    LOG(LOG_DBG,"Imported BO Info as below:\n");
    LOG(LOG_DBG,"gbmbo->ion_fd=%d,gbmbo->ion_metadata_fd=%d,"
        "gbmbo->width=%d,gbmbo->height=%d,gbmbo->format=0x%x\n",
        gbmbo->ion_fd,gbmbo->ion_metadata_fd,gbmbo->width,
        gbmbo->height,gbmbo->format);

    return gbmbo;

}

struct gbm_bo *
msmgbm_bo_import(struct gbm_device *gbm,
              uint32_t type, void *buffer, uint32_t usage)
{
     struct msmgbm_device *msm_dev = to_msmgbm_device(gbm);

    if(msm_dev == NULL){
        LOG(LOG_ERR," INVALID Device pointer\n");
        return NULL;
    }

    LOG(LOG_DBG,"msmgbm_bo_import invoked\n");

     switch(type){
     case GBM_BO_IMPORT_FD:
         LOG(LOG_DBG,"msmgbm_bo_import_fd invoked\n");
         return msmgbm_bo_import_fd(msm_dev,buffer,usage);
         break;
     case GBM_BO_IMPORT_WL_BUFFER:
         LOG(LOG_DBG,"msmgbm_bo_import_wl_buffer invoked\n");
         return msmgbm_bo_import_wl_buffer(msm_dev,buffer,usage);
         break;
     case GBM_BO_IMPORT_EGL_IMAGE:
        LOG(LOG_DBG,"msmgbm_bo_import_image invoked\n");
        return msmgbm_bo_import_egl_image(msm_dev,buffer,usage);
        break;
     case GBM_BO_IMPORT_GBM_BUF_TYPE:
        LOG(LOG_DBG,"msmgbm_bo_import_gbm_buf invoked\n");
        return msmgbm_bo_import_gbm_buf(msm_dev,buffer, usage);
        break;
     case GBM_BO_IMPORT_FD_MODIFIER:
        LOG(LOG_DBG,"msmgbm_bo_import_fd_modifier invoked\n");
        return msmgbm_bo_import_fd_modifier(msm_dev,buffer, usage);
        break;
     default:
         LOG(LOG_DBG," Invalid buffer type (%d), error = %d\n",type);
         return NULL;
     }
}

#ifdef ALLOCATE_SURFACE_BO_AT_CREATION
static void free_surface_bo(struct msmgbm_surface *surf, int num_bo_to_free)
{
    int index;
    for(index =0; index < num_bo_to_free; index++) {
        if(surf->bo[index] != NULL){
            gbm_bo_destroy(&surf->bo[index]->base);
            surf->bo[index] = NULL;
        }
    }
}
#endif

static void
msmgbm_surface_destroy(struct gbm_surface *surf)
{
    struct msmgbm_surface *msm_gbm_surf = to_msmgbm_surface(surf);

    if(msm_gbm_surf!=NULL){
#ifdef ALLOCATE_SURFACE_BO_AT_CREATION
        free_surface_bo(msm_gbm_surf, NUM_BACK_BUFFERS);
#endif
        free(msm_gbm_surf);
        msm_gbm_surf = NULL;
    }
    else {
         LOG(LOG_ERR," NULL or Invalid surface pointer\n");
    }

    return;
}

static struct gbm_bo *
msmgbm_surface_lock_front_buffer(struct gbm_surface *surf)
{
    struct msmgbm_surface *msm_gbm_surface = to_msmgbm_surface(surf);
    int index;

    if(msm_gbm_surface != NULL)
    {
#ifdef ALLOCATE_SURFACE_BO_AT_CREATION
        for(index =0; index < NUM_BACK_BUFFERS; index++)
        {
            if((msm_gbm_surface->bo[index]!= NULL) && \
                (msm_gbm_surface->bo[index]->current_state == GBM_BO_STATE_NEW_FRONT_BUFFER))
            {
                msm_gbm_surface->bo[index]->current_state = GBM_BO_STATE_INUSE_BY_COMPOSITOR;
                return &msm_gbm_surface->bo[index]->base;
            }
        }
        LOG(LOG_ERR,"No Front BO found\n");
#else
        for(index =0; index < NUM_BACK_BUFFERS; index++)
        {
            if((msm_gbm_surface->bo_slot[index] == SURFACE_BOSLOT_STATE_HAS_NEW_FRONT_BUFFER) && \
                (msm_gbm_surface->bo[index]->current_state == GBM_BO_STATE_NEW_FRONT_BUFFER))
            {
                msm_gbm_surface->bo_slot[index] =  SURFACE_BOSLOT_STATE_INUSE_BY_COMPOSITOR;
                msm_gbm_surface->bo[index]->current_state = GBM_BO_STATE_INUSE_BY_COMPOSITOR;
                return  &msm_gbm_surface->bo[index]->base;
            }
        }
        LOG(LOG_ERR,"No Front BO found\n");
#endif
    }
    else {
        LOG(LOG_ERR," NULL or Invalid surface pointer\n");
    }
    return NULL;
}

static void
msmgbm_surface_release_buffer(struct gbm_surface *surf, struct gbm_bo *bo)
{
    struct msmgbm_surface *msm_gbm_surf = to_msmgbm_surface(surf);
    struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(bo);
    int index =0;

    if((msm_gbm_surf == NULL) || (msm_gbm_bo == NULL)) {
         LOG(LOG_ERR," Invalid surface or BO pointer\n");
         return;
    }

#ifdef ALLOCATE_SURFACE_BO_AT_CREATION
    for(index=0;index < NUM_BACK_BUFFERS;index++)
    {
        if((msm_gbm_surf->bo[index] != NULL) && \
                   (msm_gbm_surf->bo[index] == msm_gbm_bo) && \
                   (msm_gbm_surf->bo[index]->current_state == GBM_BO_STATE_INUSE_BY_COMPOSITOR)) //Not sure if this check is necessary
        {
           // BO will be destroyed when surface is destroyed, just set BO state to Free.
           msm_gbm_surf->bo[index]->current_state = GBM_BO_STATE_FREE;
           return;
        }
    }
    LOG(LOG_ERR,"Invalid Input BO, BO is not locked\n");
#else
    for(index=0;index < NUM_BACK_BUFFERS;index++)
    {
        if((msm_gbm_surf->bo_slot[index] == SURFACE_BOSLOT_STATE_INUSE_BY_COMPOSITOR) && \
            (msm_gbm_surf->bo[index]->current_state == GBM_BO_STATE_INUSE_BY_COMPOSITOR)) //Not sure if this check is necessary
        {
            msm_gbm_surf->bo_slot[index] = SURFACE_BOSLOT_STATE_FREE;
            msm_gbm_surf->bo[index] = NULL;
            return;
        }
    }
   LOG(LOG_ERR,"Invalid Input BO, BO is not locked\n");
#endif
}

static int
msmgbm_surface_has_free_buffers(struct gbm_surface *surf)
{
    struct msmgbm_surface *msm_gbm_surface = to_msmgbm_surface(surf);
    int index;

    if(msm_gbm_surface != NULL){
#ifdef ALLOCATE_SURFACE_BO_AT_CREATION
        for(index =0; index < NUM_BACK_BUFFERS; index++) {
            if((msm_gbm_surface->bo[index]!= NULL) &&(msm_gbm_surface->bo[index]->current_state == GBM_BO_STATE_FREE)){
                 return 1;
            }
        }
#else
        for(index =0; index < NUM_BACK_BUFFERS; index++) {
            if(msm_gbm_surface->bo_slot[index] == SURFACE_BOSLOT_STATE_FREE){
                return 1;
            }
        }
#endif
    }
    else {
         LOG(LOG_ERR," NULL or Invalid surface pointer\n");
    }
    return 0;
}

static struct gbm_surface *
msmgbm_surface_create(struct gbm_device *gbm, uint32_t width,
                      uint32_t height, uint32_t format,
                      uint32_t flags, uint64_t *modifiers,
                      unsigned int count)
{
    struct msmgbm_device *msm_dev = to_msmgbm_device(gbm);
    struct gbm_surface *gsurf = NULL;
    struct msmgbm_surface *msm_gbmsurf = NULL;
#ifdef ALLOCATE_SURFACE_BO_AT_CREATION
    int index;
#endif

    if(msm_dev == NULL){
        LOG(LOG_ERR," INVALID device pointer\n");
        return NULL;
    }

    if(width  <= 0 || height <=0){
        LOG(LOG_ERR," INVALID width or height\n");
        return NULL;
    }

    msm_gbmsurf = (struct msmgbm_surface *)calloc(1, sizeof(struct msmgbm_surface));

    if (msm_gbmsurf == NULL) {
        LOG(LOG_ERR," Unable to allocate Surface OoM\n");
        return NULL;
    }

    if(count > 0 && count <= MAX_NUM_MODIFIERS) {
      flags = GetUsageFromModifier(modifiers, count);
   }

    gsurf = &msm_gbmsurf->base;
    gsurf->format = format;
    gsurf->height = height;
    gsurf->width = width;
    gsurf->flags = flags;
    gsurf->surface_destroy = msmgbm_surface_destroy;
    gsurf->surface_has_free_buffers =  msmgbm_surface_has_free_buffers;
    gsurf->surface_release_buffer = msmgbm_surface_release_buffer;
    gsurf->surface_lock_front_buffer = msmgbm_surface_lock_front_buffer;

    msm_gbmsurf->device = msm_dev;
    msm_gbmsurf->magic = QCMAGIC;
    msm_gbmsurf->inuse_index = -1;

#ifdef ALLOCATE_SURFACE_BO_AT_CREATION
    for(index =0; index < NUM_BACK_BUFFERS; index++) {
       msm_gbmsurf->bo[index] = to_msmgbm_bo(msmgbm_bo_create(gbm, width,
	                                         height, format, flags, NULL, 0));
       if(msm_gbmsurf->bo[index] == NULL){
           LOG(LOG_ERR," Unable to create Surface BO %d\n", index);
           free_surface_bo(msm_gbmsurf, index);
           return NULL;
       }
    }
#endif

    return gsurf;
}

static int
msmgbm_device_is_format_supported(struct gbm_device *gbm,
                               uint32_t format, uint32_t usage)
{
    struct msmgbm_device *msm_dev = to_msmgbm_device(gbm);

    if(msm_dev != NULL){
        if(IsFormatSupported(format))
            return 1;
    }
    else {
         LOG(LOG_ERR,"NULL or Invalid device pointer\n");
    }
    return 0;
}

static void
msmgbm_device_destroy(struct gbm_device *gbm)
{
    struct msmgbm_device *msm_dev = to_msmgbm_device(gbm);

    //Destroy the platform wrapper cpp object
    platform_wrap_deinstnce();

    //Destroy the  mapper cpp object
    msmgbm_mapper_deinstnce();

    if(msm_dev != NULL) {
        LOG(LOG_DBG, "iondev_fd:%d \n", msm_dev->iondev_fd);
        //Close the ion device fd
        if(msm_dev->iondev_fd > 0)
            close(msm_dev->iondev_fd);

        free(msm_dev);
        msm_dev = NULL;
    } else {
        LOG(LOG_ERR,"NULL or Invalid device pointer\n");
    }
    return;
}

static struct gbm_device *
msmgbm_device_create(int fd)
{
    struct gbm_device *gbmdevice = NULL;
    struct msmgbm_device *msm_gbmdevice =  NULL;

    msm_gbmdevice = (struct msmgbm_device *)calloc(1,sizeof(struct msmgbm_device));

    if (msm_gbmdevice == NULL) {
        return NULL;
    }

    //Update the debug level here
    config_dbg_lvl();

   //Instantiate the platform wrapper cpp object
   if(platform_wrap_instnce())
     return NULL;

    //Instantiate the mapper cpp object
    if(msmgbm_mapper_instnce())
      return NULL;

    //open the ion device
    msm_gbmdevice->iondev_fd = ion_open();
    LOG(LOG_DBG,"msmgbm_device_create: iondev_fd:%d", msm_gbmdevice->iondev_fd);
    if (msm_gbmdevice->iondev_fd < 0){
        LOG(LOG_ERR,"Failed to open ION device\n");
        return NULL;
    }

    gbmdevice =  &msm_gbmdevice->base;
    gbmdevice->fd = fd;
    gbmdevice->destroy = msmgbm_device_destroy;
    gbmdevice->is_format_supported = msmgbm_device_is_format_supported;
    gbmdevice->bo_create = msmgbm_bo_create;
    gbmdevice->get_format_modifier_plane_count = msmgbm_get_format_modifier_plane_count;
    gbmdevice->bo_import = msmgbm_bo_import;
    gbmdevice->surface_create = msmgbm_surface_create;
    msm_gbmdevice->fd = fd;
    msm_gbmdevice->magic = QCMAGIC;

    LOG(LOG_DBG,"gbm device fd= %d\n",gbmdevice->fd);

    return gbmdevice;
}

struct gbm_backendpriv g_msm_priv = {
   .backend_name = "msm_drm", //As this will be using MSM DRM
   .create_device = msmgbm_device_create,
};

struct gbm_backendpriv *msmgbm_get_priv(void)
{
    return &g_msm_priv;
}

//$this API Vs QCMAGIC
unsigned int msmgbm_device_get_magic(struct gbm_device *dev)
{
    struct msmgbm_device *msm_dev = to_msmgbm_device( dev);

    if(msm_dev == NULL){
        LOG(LOG_ERR,"NULL or Invalid device pointer\n");
        return 0;
    }
    else
    {
        drm_auth_t auth;
        int ret  =0;
        memset(&auth, 0, sizeof(drm_auth_t));

        ret = ioctl(msm_dev->fd, DRM_IOCTL_GET_MAGIC, &auth);
        if (ret)
        {
            LOG(LOG_ERR,"GET_MAGIC failed for device (%x)\n %s\n",
                                    msm_dev,strerror(errno));
            return 0;
        }
        return auth.magic;
    }
}

int msmgbm_surface_set_front_bo(struct gbm_surface *surf, struct gbm_bo *bo)
{
    struct msmgbm_surface*msm_gbm_surface = to_msmgbm_surface(surf);
    struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(bo);
    int index;

    if(msm_gbm_bo!=NULL ||msm_gbm_surface !=NULL )
    {
#ifdef ALLOCATE_SURFACE_BO_AT_CREATION
        for(index =0; index < NUM_BACK_BUFFERS; index++)
        {
            if((msm_gbm_surface->bo[index]!= NULL) && \
                 (msm_gbm_surface->bo[index] == msm_gbm_bo)  && \
                 (msm_gbm_surface->bo[index]->current_state == GBM_BO_STATE_INUSE_BY_GPU))
            {
                     msm_gbm_surface->bo[index]->current_state = GBM_BO_STATE_NEW_FRONT_BUFFER;
                     return GBM_ERROR_NONE;
            }
        }
        LOG(LOG_ERR," INVALID BO, Passed BO was not obtained using \
                                msmgbm_surface_get_free_bo\n");
        return GBM_ERROR_NO_RESOURCES;
#else
        for(index =0; index < NUM_BACK_BUFFERS; index++)
        {
            if(msm_gbm_surface->bo_slot[index] == SURFACE_BOSLOT_STATE_FREE)
            {
                msm_gbm_surface->bo_slot[index] = SURFACE_BOSLOT_STATE_HAS_NEW_FRONT_BUFFER;
                msm_gbm_surface->bo[index] = msm_gbm_bo;
                msm_gbm_surface->bo[index]->current_state = GBM_BO_STATE_NEW_FRONT_BUFFER;
                return GBM_ERROR_NONE;
            }
        }
        LOG(LOG_ERR," NO Free BO slot found!!\n");
       return GBM_ERROR_NO_RESOURCES;
#endif
    }
    else
    {
         LOG(LOG_ERR," INVALID BO or Surface pointer\n");
         return GBM_ERROR_BAD_HANDLE;
    }
}

#ifdef ALLOCATE_SURFACE_BO_AT_CREATION
struct gbm_bo* msmgbm_surface_get_free_bo(struct gbm_surface *surf)
{
    struct msmgbm_surface *msm_gbm_surface = to_msmgbm_surface(surf);
    int index;

    if(msm_gbm_surface != NULL)
    {
        int cur_index = msm_gbm_surface->inuse_index;
        for(index = ((cur_index + 1) % NUM_BACK_BUFFERS); index < NUM_BACK_BUFFERS; index++)
        {
            if((msm_gbm_surface->bo[index]!= NULL) && \
                (msm_gbm_surface->bo[index]->current_state == GBM_BO_STATE_FREE))
            {
                msm_gbm_surface->bo[index]->current_state = GBM_BO_STATE_INUSE_BY_GPU;
                msm_gbm_surface->inuse_index = index;
                return &msm_gbm_surface->bo[index]->base;
            }
        }
        LOG(LOG_ERR," NO Free BO found!!\n");
    }
    else
    {
        LOG(LOG_ERR," NULL or Invalid surface pointer\n");
    }
    return NULL;
}
#else
struct gbm_bo* msmgbm_surface_get_free_bo(struct gbm_surface *surf)
{
     LOG(LOG_ERR," This API is not supported.\n");
     return NULL;
}
#endif

void* msmgbm_cpu_map_metafd(int meta_ion_fd, unsigned int metadata_size)
{
    struct meta_data_t *mt_cpuaddr = NULL;

    //meta fd and gbm_bo must be valid at this point
    mt_cpuaddr = mmap(NULL, metadata_size, PROT_READ | PROT_WRITE,
                      MAP_SHARED, meta_ion_fd, 0);
    if(mt_cpuaddr == MAP_FAILED) {
        mt_cpuaddr = NULL;
        LOG(LOG_DBG," cpu Map failed for gbo_info->metadata_fd: %d %s\n",
            meta_ion_fd, strerror(errno));
    }

    return mt_cpuaddr;
}

void* msmgbm_cpu_map_ionfd(int ion_fd, unsigned int size, struct meta_data_t *meta_data)
{
    void *cpuaddr = NULL;

    if(meta_data != NULL) {
        if(!meta_data->is_buffer_secure) {
            cpuaddr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, ion_fd, 0);
            if(cpuaddr == MAP_FAILED) {
                cpuaddr = NULL;
                LOG(LOG_DBG, "cpu mapping failed for ion fd = %d, %s", ion_fd, strerror(errno));
            }
        }
        LOG(LOG_DBG, "Can't map secure buffer", __func__, __LINE__);
    }

    return cpuaddr;
}

void* msmgbm_bo_meta_map(struct gbm_bo *bo)
{
        struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(bo);
        uint32_t mt_size;
        void *mt_cpuaddr;

        if(msm_gbm_bo) {
            mt_cpuaddr = msm_gbm_bo->mt_cpuaddr;
        } else {
            LOG(LOG_INFO, "This is not optimized path: %s,%d\n", __func__, __LINE__);
            mt_size = query_metadata_size();
            mt_cpuaddr = msmgbm_cpu_map_metafd(bo->ion_metadata_fd, mt_size);
        }

        return mt_cpuaddr;
}

void* msmgbm_bo_cpu_map(struct gbm_bo *bo)
{
    struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(bo);
    struct meta_data_t *mt_cpuaddr;
    void *cpuaddr = NULL;

    if(msm_gbm_bo!=NULL)
    {
        if(msm_gbm_bo->cpuaddr)
        {
            cpuaddr = msm_gbm_bo->cpuaddr;
        } else {
            LOG(LOG_INFO, "This is not optimized path for cpu bo map\n");
            mt_cpuaddr = (struct meta_data_t *)msm_gbm_bo->mt_cpuaddr;
            cpuaddr = msmgbm_cpu_map_ionfd(bo->ion_fd, bo->size, mt_cpuaddr);
            msm_gbm_bo->cpuaddr = cpuaddr;
        }
    }
    else
    {
        LOG(LOG_ERR," NULL or Invalid bo pointer\n");
        cpuaddr = NULL;
    }

    return cpuaddr;
}

int msmgbm_bo_cpu_unmap(struct gbm_bo *bo)
{
    struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(bo);
    if(msm_gbm_bo!=NULL)
    {
        //BO buffer
        if (msm_gbm_bo->cpuaddr != NULL)
        {

            LOG(LOG_DBG," unmapping msm_gbm_bo->cpuaddr=0x%x fd=%d\n",
                                           msm_gbm_bo->cpuaddr, msm_gbm_bo->base.ion_fd);
            if(munmap((void *)msm_gbm_bo->cpuaddr, bo->size))
                LOG(LOG_ERR," munmap failed for msm_gbm_bo->cpuaddr=0x%x ERR: %s\n",
                                                msm_gbm_bo->cpuaddr, strerror(errno));
        }
        msm_gbm_bo->cpuaddr = NULL;

        //Metadata buffer
        if (msm_gbm_bo->mt_cpuaddr != NULL)
        {
            LOG(LOG_DBG," unmapping msm_gbm_bo->mt_cpuaddr=0x%x mt_fd=%d\n",
                                           msm_gbm_bo->mt_cpuaddr, msm_gbm_bo->base.ion_metadata_fd);
            if(munmap(msm_gbm_bo->mt_cpuaddr, msm_gbm_bo->mt_size))
                LOG(LOG_ERR," munmap failed for msm_gbm_bo->mt_cpuaddr=0x%x, ERR: %s\n",
                                                msm_gbm_bo->mt_cpuaddr, strerror(errno));
        }
        msm_gbm_bo->mt_cpuaddr = NULL;

        return GBM_ERROR_NONE;
    }
    else
    {
        LOG(LOG_ERR," NULL or Invalid bo pointer\n");
        return GBM_ERROR_BAD_HANDLE;
    }
}

//$ how to go about the same
void* msmgbm_bo_gpu_map(struct gbm_bo *bo)
{
    /* John --  This piece of the code needs to go through UHAB to get GPU address */
    struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(bo);
    return GBM_ERROR_UNSUPPORTED;
}

int msmgbm_bo_gpu_unmap(struct gbm_bo *bo)
{
    // BO will be unmapped from GPU MMU after GEM CLOSE. Silent return
    return GBM_ERROR_UNSUPPORTED;
}

static inline
size_t msmgbm_bo_get_size(struct gbm_bo *bo)
{
   struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(bo);
    if(msm_gbm_bo!=NULL)
    {
        return msm_gbm_bo->size;
    }
    else
    {
        LOG(LOG_ERR," NULL or Invalid bo pointer\n");
        return GBM_ERROR_BAD_HANDLE;
    }
}

static inline
int msmgbm_validate_device(struct gbm_device *dev){
    struct msmgbm_device*msm_dev = to_msmgbm_device(dev);

    if((msm_dev != NULL) && (msm_dev->magic == QCMAGIC) ) {
        return GBM_ERROR_NONE;
    }
    else {
        return GBM_ERROR_BAD_HANDLE;
    }
}

static inline
int  msmgbm_validate_surface(struct gbm_surface *surf){
    struct msmgbm_surface*msmgbm_surface = to_msmgbm_surface(surf);

    if((msmgbm_surface != NULL) && (msmgbm_surface->magic == QCMAGIC) ) {
        return GBM_ERROR_NONE;
    }
    else {
        return GBM_ERROR_BAD_HANDLE;
    }
}

static inline
const char*  msmgbm_get_drm_device_name(void){
    return DRM_DEVICE_NAME;
}

int  msmgbm_device_authenticate_magic(struct gbm_device *dev, drm_magic_t magic){
    struct msmgbm_device *msm_dev = to_msmgbm_device(dev);

    if(msm_dev == NULL){
        LOG(LOG_ERR," NULL or Invalid device pointer\n");
        return GBM_ERROR_BAD_HANDLE;
    }
    else
    {
        drm_auth_t auth;
        int ret  =0;
        memset(&auth, 0, sizeof(drm_auth_t));
        auth.magic = magic;

        ret = ioctl(msm_dev->fd, DRM_IOCTL_AUTH_MAGIC, &auth);
        if (ret)
        {
            LOG(LOG_ERR," AUTH_MAGIC failed for device (%x)\n %s\n",
                                    msm_dev,strerror(errno));
            return GBM_ERROR_BAD_VALUE;
        }
    }
    return GBM_ERROR_NONE;

}

struct gbm_bo*  msmgbm_bo_import_from_name(struct gbm_device *dev, unsigned int name)
{
    struct msmgbm_device *msm_dev = to_msmgbm_device(dev);
    struct drm_prime_handle gemimport_req;
    struct gbm_bo *gbmbo = NULL;
    struct msmgbm_bo *msm_gbmbo = NULL;
    int fd = (int)name;
    int ret = 0;


    if(NULL == msm_dev){
        LOG(LOG_ERR," INVALID Device pointer\n");
        return NULL;
    }

    if(0 > fd)
    {
        LOG(LOG_ERR," INVALID File descriptor(%d)\n", name);
        return NULL;
    }

    memset(&gemimport_req, 0, sizeof(gemimport_req));
    gemimport_req.fd = fd;

    ret = ioctl(msm_dev->fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &gemimport_req);

    if (ret != 0){
        LOG(LOG_DBG," PRIME FD to Handle failed on device(%x), error = %d\n",
                                                        msm_dev,ret);
        gemimport_req.handle = 0;
    }
    else
    {
        LOG(LOG_DBG,"Get Gem Handle[%u] from fd[%d]\n", gemimport_req.handle, gemimport_req.fd);
        lock();
        incr_handle_refcnt(msm_dev->fd, gemimport_req.handle);
        unlock();
    }

    msm_gbmbo = (struct msmgbm_bo *)calloc(1, sizeof(struct msmgbm_bo));

    if (msm_gbmbo == NULL) {
        LOG(LOG_ERR," Unable to allocate BO OoM\n");
        return NULL;
    }

    gbmbo =  &msm_gbmbo->base;
    gbmbo->ion_fd = fd;
    gbmbo->handle.u32 = gemimport_req.handle;
    gbmbo->bo_destroy = msmgbm_bo_destroy;
    gbmbo->bo_get_fd= msmgbm_bo_get_fd;
    gbmbo->stride_for_plane = msmgbm_stride_for_plane;
    gbmbo->bo_get_device = msmgbm_bo_get_device;
    gbmbo->bo_write = msmgbm_bo_write;
    msm_gbmbo->device = msm_dev;
    msm_gbmbo->current_state =  GBM_BO_STATE_FREE;
    msm_gbmbo->magic = QCMAGIC;
    msm_gbmbo->name = name;
    //msm_gbmbo->size = gem_open.size;

    msmgbm_yuv_plane_info(gbmbo,&(gbmbo->buf_lyt));
    return gbmbo;
}

int msmgbm_bo_get_name(struct gbm_bo* bo)
{
    struct msmgbm_bo *msm_gbmbo = to_msmgbm_bo(bo);
    struct drm_prime_handle drm_args;
    int ret;


    if(NULL == msm_gbmbo){
        LOG(LOG_ERR," INVALID BO pointer\n");
        return -1;
    }

    if(0 == msm_gbmbo->name)
    {
        memset(&drm_args, 0, sizeof(drm_args));
        drm_args.handle = msm_gbmbo->base.handle.u32;
        msm_gbmbo->name = bo->ion_fd;
    }
    return msm_gbmbo->name;
}

//$ How are we planning to expose to the clients the var args usage
int msmgbm_perform(int operation, ... )
{
    int res = GBM_ERROR_UNSUPPORTED;
    va_list args;


    va_start(args, operation);

    switch (operation){
        case GBM_PERFORM_GET_SURFACE_WIDTH:
            {
                struct gbm_surface *gbm_surf = va_arg(args, struct gbm_surface *);
                uint32_t *width  = va_arg(args, uint32_t *);

                struct msmgbm_surface* msmgbm_surf = to_msmgbm_surface(gbm_surf);

                if(msmgbm_surf != NULL && msmgbm_surf->magic == QCMAGIC){
                    *width = gbm_surf->width;
                     res = GBM_ERROR_NONE;
                }
                else
                    res = GBM_ERROR_BAD_HANDLE;
            }
            break;

        case GBM_PERFORM_GET_SURFACE_HEIGHT:
            {
                struct gbm_surface *gbm_surf = va_arg(args, struct gbm_surface *);
                uint32_t *height  = va_arg(args, uint32_t *);

                struct msmgbm_surface* msmgbm_surf = to_msmgbm_surface(gbm_surf);

                if(msmgbm_surf != NULL && msmgbm_surf->magic == QCMAGIC){
                    *height = gbm_surf->height;
                     res = GBM_ERROR_NONE;
                }
                else
                    res = GBM_ERROR_BAD_HANDLE;
            }
            break;

        case GBM_PERFORM_GET_SURFACE_FORMAT:
            {
                struct gbm_surface *gbm_surf = va_arg(args, struct gbm_surface *);
                uint32_t *format  = va_arg(args, uint32_t *);

                struct msmgbm_surface* msmgbm_surf = to_msmgbm_surface(gbm_surf);

                if(msmgbm_surf != NULL && msmgbm_surf->magic == QCMAGIC){
                    *format = gbm_surf->format;
                     res = GBM_ERROR_NONE;
                }
                else
                    res = GBM_ERROR_BAD_HANDLE;
            }
            break;

        case GBM_PERFORM_SET_SURFACE_FRONT_BO:
            {
                struct gbm_surface *gbm_surf = va_arg(args, struct gbm_surface *);
                struct gbm_bo *gbo = va_arg(args,struct gbm_bo *);

                res = msmgbm_surface_set_front_bo(gbm_surf, gbo);
            }
            break;

        case GBM_PERFORM_GET_SURFACE_FREE_BO:
            {
                struct gbm_surface *gbm_surf = va_arg(args, struct gbm_surface *);
                struct gbm_bo **gbo = va_arg(args,struct gbm_bo **);

                *gbo = msmgbm_surface_get_free_bo(gbm_surf);
                if(*gbo)
                    res = GBM_ERROR_NONE;
                else
                    res = GBM_ERROR_BAD_VALUE;
            }
            break;
        case GBM_PERFORM_VALIDATE_SURFACE:
            {
                struct gbm_surface *gbm_surf = va_arg(args, struct gbm_surface *);
                res = msmgbm_validate_surface(gbm_surf);
            }
            break;
        case GBM_PERFORM_CPU_MAP_FOR_BO:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                void **map_addr_handle = va_arg(args,void **);

                *map_addr_handle=msmgbm_bo_cpu_map(gbo);
                if(*map_addr_handle)
                    res = GBM_ERROR_NONE;
                else
                    res = GBM_ERROR_BAD_VALUE;
            }
            break;
        case GBM_PERFORM_CPU_UNMAP_FOR_BO:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                //BO unmap will be as part of bo destroy as
                //we are storing cpu address and meta data address in hashmap
                //unmap takes place once fd entry is erased from hashmap in
                //msmgbm_bo_destroy
                res = GBM_ERROR_NONE;
            }
            break;
        case GBM_PERFORM_GET_GPU_ADDR_FOR_BO:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                uint64_t *gpu_addr = va_arg(args,uint64_t *);
                struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(gbo);

                if(msm_gbm_bo!=NULL) {
                    *gpu_addr = msm_gbm_bo->gpuaddr;
                    res = GBM_ERROR_NONE;
                }
                else
                    res = GBM_ERROR_BAD_VALUE;
            }
            break;
        case GBM_PERFORM_SET_GPU_ADDR_FOR_BO:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                uint64_t gpu_addr = va_arg(args,uint64_t);
                struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(gbo);

                if(msm_gbm_bo!=NULL) {
                    msm_gbm_bo->gpuaddr = gpu_addr;
                    res = GBM_ERROR_NONE;
                }
                else
                    res = GBM_ERROR_BAD_VALUE;
            }
            break;
        case GBM_PERFORM_GET_BO_SIZE:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                size_t* size = va_arg(args,size_t*);

                *size = msmgbm_bo_get_size(gbo);
                if(*size)
                    res = GBM_ERROR_NONE;
                else
                    res = GBM_ERROR_BAD_HANDLE;
            }
            break;
        case GBM_PERFORM_GET_BO_NAME:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                int* name = va_arg(args,int*);

                *name = msmgbm_bo_get_name(gbo);
                if(*name > 0)
                    res = GBM_ERROR_NONE;
                else
                    res = GBM_ERROR_BAD_HANDLE;
            }
            break;
        case GBM_PERFORM_IMPORT_BO_FROM_NAME:
            {
                struct gbm_device *gbm_dev = va_arg(args, struct gbm_device *);
                struct gbm_bo **gbo = va_arg(args,struct gbm_bo **);
                int name = va_arg(args,int);

                *gbo = NULL;
                *gbo = msmgbm_bo_import_from_name(gbm_dev,name);
                if(*gbo)
                    res = GBM_ERROR_NONE;
                else
                    res = GBM_ERROR_BAD_HANDLE;
            }
            break;
        case GBM_PERFORM_GET_DRM_DEVICE_MAGIC:
            {
                struct gbm_device *gbm_dev = va_arg(args, struct gbm_device *);
                drm_magic_t *magic_id = va_arg(args,drm_magic_t*);

                *magic_id = msmgbm_device_get_magic(gbm_dev);
                if(*magic_id)
                    res = GBM_ERROR_NONE;
                else
                    res = GBM_ERROR_BAD_HANDLE;
            }
            break;
        case GBM_PERFORM_AUTH_DRM_DEVICE_MAGIC:
            {
                struct gbm_device *gbm_dev = va_arg(args, struct gbm_device *);
                drm_magic_t magic_id = va_arg(args,drm_magic_t);

                res = msmgbm_device_authenticate_magic(gbm_dev,magic_id);
            }
            break;
        case GBM_PERFORM_GET_DRM_DEVICE_NAME:
            {
                char *drm_dev_name = va_arg(args,char *);
                uint32_t size = va_arg(args, uint32_t);
                if (access(DRM_DEVICE_NAME, F_OK) >=0) {
                    strlcpy(drm_dev_name, DRM_DEVICE_NAME, size);
                } else {
                    strlcpy(drm_dev_name, ION_DEVICE_NAME, size);
                }
                res = GBM_ERROR_NONE;
            }
            break;
        case GBM_PERFORM_GET_RENDER_DEVICE_NAME:
            {
                char *render_dev_name = va_arg(args,char *);
                uint32_t size = va_arg(args, uint32_t);
                if (access(RENDER_DEVICE_NAME, F_OK) >=0) {
                    strlcpy(render_dev_name, RENDER_DEVICE_NAME, size);
                } else {
                    strlcpy(render_dev_name, ION_DEVICE_NAME, size);
                }
                res = GBM_ERROR_NONE;
            }
            break;
        case GBM_PERFORM_VALIDATE_DEVICE:
            {
                struct gbm_device *gbm_dev = va_arg(args, struct gbm_device *);
                res = msmgbm_validate_device(gbm_dev);
            }
            break;
        case GBM_PERFORM_GET_METADATA:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                int paramType = va_arg(args,int);
                void* param = va_arg(args,void*);
                LOG(LOG_DBG," Passed param address & value = 0x%x, 0x%x\n",
                             (unsigned int *)param,*(unsigned int *)param);
                res = msmgbm_get_metadata(gbo,paramType,param);
            }
            break;
        case GBM_PERFORM_SET_METADATA:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                int paramType = va_arg(args,int);
                void* param = va_arg(args,void*);
                LOG(LOG_DBG," Passed param address & value = 0x%x, 0x%x\n",
                             (unsigned int *)param,*(unsigned int *)param);

                res = msmgbm_set_metadata(gbo,paramType,param);
            }
            break;
        case GBM_PERFORM_GET_UBWC_STATUS:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                int *ubwc_status = va_arg(args,int *);

                 res = msmgbm_get_metadata(gbo, GBM_METADATA_GET_UBWC_BUF_STAT,
                                           (void *)ubwc_status);
            }
            break;
        case GBM_PERFORM_GET_YUV_PLANE_INFO:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                generic_buf_layout_t *buf_lyt = va_arg(args, generic_buf_layout_t *);

                res = msmgbm_yuv_plane_info(gbo,buf_lyt);
            }
            break;
        case GBM_PERFORM_GET_SECURE_BUFFER_STATUS:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                bool *sec_buf_stat = va_arg(args,int *);

                *sec_buf_stat = 0;
                res = msmgbm_get_metadata(gbo, GBM_METADATA_GET_SECURE_BUF_STAT,
                                                             (void *)sec_buf_stat);
            }
            break;
        case GBM_PERFORM_GET_METADATA_ION_FD:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                int *metadata_fd = va_arg(args,int *);

                if((gbo == NULL) || (metadata_fd == NULL))
                {
                    res = GBM_ERROR_BAD_HANDLE;
                }
                else
                {
                    if((gbo->ion_metadata_fd) < 0)
                    {
                        //Let us try looking through the map table in case if we have
                        //an update, since last import call?
                        struct gbm_buf_info temp_buf_info;
                        struct msmgbm_private_info gbo_private_info = {NULL, NULL};
                        lock();
                        res = search_hashmap(gbo->ion_fd, &temp_buf_info, &gbo_private_info);
                        unlock();

                        if((res == GBM_ERROR_NONE) && (temp_buf_info.metadata_fd > 0))
                        {
                            LOG(LOG_DBG,"MAP retrieved buf info\n");
                            LOG(LOG_DBG,"temp_buf_info.metadata_fd=%d\n",
                                              temp_buf_info.metadata_fd);
                            LOG(LOG_DBG,"temp_buf_info.width=%d\n",
                                                  temp_buf_info.width);
                            LOG(LOG_DBG,"temp_buf_info.height=%d\n",
                                                 temp_buf_info.height);
                            LOG(LOG_DBG,"temp_buf_info.format=%d\n",
                                                  temp_buf_info.format);

                            //save the same in the gbo handle as well
                            gbo->ion_metadata_fd = temp_buf_info.metadata_fd;
                        }
                    }
                    *metadata_fd = gbo->ion_metadata_fd;
                    res = GBM_ERROR_NONE;
                }
            }
            break;
        case GBM_PERFORM_GET_BO_ALIGNED_WIDTH:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                uint32_t *align_wdth = va_arg(args, uint32_t *);

                *align_wdth = gbo->aligned_width;

                res = GBM_ERROR_NONE;
            }
            break;
        case GBM_PERFORM_GET_BO_ALIGNED_HEIGHT:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                uint32_t *align_hght = va_arg(args, uint32_t *);

                *align_hght = gbo->aligned_height;

                res = GBM_ERROR_NONE;
            }
            break;
        case GBM_PERFORM_DUMP_HASH_MAP:
            {
                 msmgbm_dump_hashmap();
                 res = GBM_ERROR_NONE;
            }
             break;
        case GBM_PERFORM_DUMP_BO_CONTENT:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                res = msmgbm_bo_dump(gbo);
            }
            break;
        case GBM_PERFORM_GET_PLANE_INFO:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                struct generic_buf_layout_t * buf_lyt = va_arg(args, struct generic_buf_layout_t *);
                res = msmgbm_get_buf_lyout(gbo, buf_lyt);
            }
            break;
        case GBM_PERFORM_DEFAULT_INIT_COLOR_META:
            {
                struct ColorMetaData *clr_mta = va_arg(args, struct ColorMetaData *);
                msmsgbm_default_init_hdr_color_info_mdata(clr_mta);
                res = GBM_ERROR_NONE;
            }
            break;
        case GBM_PERFORM_DUMP_COLOR_META:
            {
                struct ColorMetaData *clr_mta = va_arg(args, struct ColorMetaData *);
                msmgbm_log_hdr_color_info_mdata(clr_mta);
                res = GBM_ERROR_NONE;
            }
            break;
        case GBM_PERFORM_GET_BUFFER_SIZE_DIMENSIONS:
            {
                struct gbm_buf_info * buf_info = va_arg(args, struct gbm_buf_info *);
                uint32_t usage_flags = va_arg(args, uint32_t);
                uint32_t *align_wdth = va_arg(args, uint32_t *);
                uint32_t *align_hght = va_arg(args, uint32_t *);
                uint32_t *size = va_arg(args, uint32_t *);

                struct gbm_bufdesc bufdesc = {buf_info->width, buf_info->height,
                                              buf_info->format, usage_flags};

                qry_aligned_wdth_hght(&bufdesc, align_wdth, align_hght);

                *size = qry_size(&bufdesc, *align_wdth, *align_hght);

                res = GBM_ERROR_NONE;
            }
            break;
        case GBM_PERFORM_GET_BUFFER_STRIDE_SCANLINE_SIZE:
            {
                struct gbm_buf_info * buf_info = va_arg(args, struct gbm_buf_info *);
                uint32_t usage_flags = va_arg(args, uint32_t);
                uint32_t *stride = va_arg(args, uint32_t *);
                uint32_t *scanline = va_arg(args, uint32_t *);
                uint32_t *size = va_arg(args, uint32_t *);

                struct gbm_bufdesc bufdesc = {buf_info->width, buf_info->height,
                                              buf_info->format, usage_flags};

                qry_stride_scanline_size(&bufdesc, stride, scanline, size);

                res = GBM_ERROR_NONE;
            }
            break;
        case GBM_PERFORM_GET_SURFACE_UBWC_STATUS:
            {
                struct gbm_surface *gbm_surf = va_arg(args, struct gbm_surface *);
                int *ubwc_status = va_arg(args,int *);

                *ubwc_status =  is_ubwc_enabled(gbm_surf->format, gbm_surf->flags, gbm_surf->flags);

                res = GBM_ERROR_NONE;
            }
            break;
        case GBM_PERFORM_GET_RGB_DATA_ADDRESS:
            {
                struct gbm_bo *gbo = va_arg(args, struct gbm_bo *);
                void **rgb_data = va_arg(args, void **);
                res = msmgbm_get_rgb_data_address(gbo, rgb_data);
            }
            break;
        case GBM_PERFORM_GET_FD_WITH_NEW:
            {
                uint32_t *with_new = va_arg(args, uint32_t *);
                *with_new = false;
                res = GBM_ERROR_NONE;
            }
            break;
        case GBM_PERFORM_GET_WL_RESOURCE_FROM_GBM_BUF_INFO:
            {
                LOG(LOG_WARN, "GBM_PERFORM_GET_WL_RESOURCE_FROM_GBM_BUF_INFO is deprecated\n");
                res = GBM_ERROR_UNSUPPORTED;
            }
            break;
        case GBM_PERFORM_GET_GBM_BUF_INFO_FROM_WL_RESOURCE:
            {
                LOG(LOG_WARN, "GBM_PERFORM_GET_GBM_BUF_INFO_FROM_WL_RESOURCE is deprecated\n");
                res = GBM_ERROR_UNSUPPORTED;
            }
            break;
         default:
                LOG(LOG_INFO,"PERFORM Operation not supported\n");
            break;
    }
    va_end(args);
    return res;
}

int msmgbm_get_rgb_data_address(struct gbm_bo *gbo, void **rgb_data) {
    int ret = GBM_ERROR_NONE;
    int ubwc_status = 0;
    int Bpp; //Bytes per pixel
    int metaBuffer_size;

    // This api is for RGB* formats
    if (!is_valid_uncmprsd_rgb_format(gbo->format)) {
      return GBM_ERROR_BAD_VALUE;
    }

    // Query whether BO is UBWC allocated
    msmgbm_get_metadata(gbo, GBM_METADATA_GET_UBWC_BUF_STAT, &ubwc_status);

    if (!ubwc_status) {
      // BO is Linearly allocated. Return cpu_address
      *rgb_data = msmgbm_bo_cpu_map(gbo);
    } else {
      // BO is UBWC allocated
      // Compute bytes per pixel
      Bpp = get_bpp_for_uncmprsd_rgb_format(gbo->format);

      // Compute meta size
      metaBuffer_size = get_rgb_ubwc_metabuffer_size(gbo->aligned_width, gbo->aligned_height, Bpp);

      *rgb_data = (void *) (msmgbm_bo_cpu_map(gbo) + metaBuffer_size);
    }

    return ret;
}

int msmgbm_set_metadata(struct gbm_bo *gbo, int paramType,void *param) {
    struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(gbo);
    struct meta_data_t *data = NULL;
    size_t size = 0;
    void *base = NULL;
    int res = GBM_ERROR_NONE;

    if(!msm_gbm_bo)
        return GBM_ERROR_BAD_HANDLE;

    if((gbo->ion_metadata_fd) <= 0)
    {
        LOG(LOG_ERR," Invalid metadata_fd=%d\n",gbo->ion_metadata_fd);
        return GBM_ERROR_BAD_HANDLE;
    }


    base = msm_gbm_bo->mt_cpuaddr;

    if(!base)
    {
        LOG(LOG_ERR, "No metadata cpu address available for ion_metadata_fd = %d\n",
            gbo->ion_metadata_fd);
        return GBM_ERROR_BAD_HANDLE;
    }

    data = (struct meta_data_t *)base;

    // If parameter is NULL reset the specific MetaData Key
    if (!param)
       data->operation &= ~paramType;

    data->operation |= paramType;

    LOG(LOG_DBG," operation Enabled %d\n",data->operation);
    LOG(LOG_DBG," Passed param address & value = 0x%x, 0x%x\n",
                                               (unsigned int *)param,*(unsigned int *)param);

    switch (paramType) {
        case GBM_METADATA_SET_INTERLACED:
             data->interlaced = *((unsigned int *)param);
             break;
        case GBM_METADATA_SET_REFRESH_RATE:
             data->refresh_rate = *((float *)param);
             break;
        case GBM_METADATA_SET_COLOR_SPACE:
             data->color_space = *((int *)param);
             break;
        case GBM_METADATA_SET_MAP_SECURE_BUFFER:
             data->map_secure_buffer = *((uint32_t *)param);
             break;
        case GBM_METADATA_SET_S3DFORMAT:
             data->s3d_format = *((uint32_t *)param);
             break;
        case GBM_METADATA_SET_LINEAR_FORMAT:
             data->linear_format = *((uint32_t *)param);
             break;
        case GBM_METADATA_SET_IGC:
             data->igc = *((int *)param);
             break;
        case GBM_METADATA_SET_COLOR_METADATA:
             data->color_info = *((ColorMetaData *)param);
             break;
        case GBM_METADATA_SET_VT_TIMESTAMP:
             data->vt_timestamp = *((uint64_t *)param);
             break;
        case GBM_METADATA_SET_VIDEO_PERF_MODE:
             data->isVideoPerfMode = *((uint32_t *)param);
             break;
        case GBM_METADATA_SET_CVP_METADATA:
             data->cvpMetadata = *((CVPMetadata *)param);
             break;
        case GBM_METADATA_SET_VIDEO_HIST_STAT:
             data->histMetadata  = *((struct VideoHistogramMetadata *)param);
             break;
        default:
            LOG(LOG_ERR," Operation currently not supported\n");
            res = GBM_ERROR_UNSUPPORTED;
            break;
    }

    return res;
}

int msmgbm_get_metadata(struct gbm_bo *gbo, int paramType,void *param) {
    struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(gbo);
    struct meta_data_t *data = NULL;
    size_t size = 0;
    void *base;
    int res = GBM_ERROR_NONE;

    if(!msm_gbm_bo)
        return GBM_ERROR_BAD_HANDLE;

    if((gbo->ion_metadata_fd) <= 0)
    {
        //Let us try looking through the map table in case if we have
        //an update, since last import call?
        struct gbm_buf_info temp_buf_info;
        struct msmgbm_private_info bo_private_info;
        lock();
        res = search_hashmap(gbo->ion_fd, &temp_buf_info, &bo_private_info);
        unlock();

        if((res==GBM_ERROR_NONE) && (temp_buf_info.metadata_fd > 0))
        {
            LOG(LOG_DBG,"MAP retrieved buf info\n");
            LOG(LOG_DBG,"temp_buf_info.metadata_fd=%d\n",
                              temp_buf_info.metadata_fd);
            LOG(LOG_DBG,"temp_buf_info.width=%d\n",
                                  temp_buf_info.width);
            LOG(LOG_DBG,"temp_buf_info.height=%d\n",
                                 temp_buf_info.height);
            LOG(LOG_DBG,"temp_buf_info.format=%d\n",
                                  temp_buf_info.format);

            //save the same in the gbo handle as well
            gbo->ion_metadata_fd = temp_buf_info.metadata_fd;
        }
        else
        {
            LOG(LOG_INFO,"metadata_fd=%d and hence valid meta info cannot be retrieved\n",
                                                                      gbo->ion_metadata_fd);
            LOG(LOG_INFO,"We will make a graceful exit\n");
            return GBM_ERROR_NONE;
        }

    }

    data = (struct meta_data_t *)msm_gbm_bo->mt_cpuaddr;
    if(data == NULL) {
        LOG(LOG_ERR, "No metadata cpu address for ion_metadata_fd = %d\n", gbo->ion_metadata_fd);
        return GBM_ERROR_BAD_HANDLE;
    }


    if (!param) {
        LOG(LOG_ERR," Null or Invalid Param Pointer\n");
        return GBM_ERROR_BAD_HANDLE;
    }

    LOG(LOG_DBG,"gbo->ion_fd=%d gbo->ion_metadata_fd=%d\n",gbo->ion_fd, gbo->ion_metadata_fd);
    LOG(LOG_DBG,"paramType:%d\n", paramType);

    switch (paramType) {
        case GBM_METADATA_GET_INTERLACED:
            *((uint32_t *)param) = data->interlaced;
            break;
        case GBM_METADATA_GET_REFRESH_RATE:
            *((float *)param) = data->refresh_rate;
            break;
        case GBM_METADATA_GET_COLOR_SPACE:
            *((int *)param) = 0;

            if (data->operation & GBM_METADATA_SET_COLOR_SPACE) {
              *((int *)param) = data->color_space;
            } else if (data->operation & GBM_METADATA_SET_COLOR_METADATA) {
              switch (data->color_info.colorPrimaries) {
                case ColorPrimaries_BT709_5:
                  *((int *)param) = GBM_METADATA_COLOR_SPACE_ITU_R_709;
                  break;
                case ColorPrimaries_BT601_6_525:
                  *((int *)param) = (data->color_info.range) ?
                                      GBM_METADATA_COLOR_SPACE_ITU_R_601_FR :
                                      GBM_METADATA_COLOR_SPACE_ITU_R_601;
                  break;
                case ColorPrimaries_BT2020:
                  *((int *)param) = (data->color_info.range) ?
                                     GBM_METADATA_COLOR_SPACE_ITU_R_2020_FR :
                                     GBM_METADATA_COLOR_SPACE_ITU_R_2020;
                  break;
                default:
                  LOG(LOG_ERR," Unknown Color Space:%d\n", data->color_info.colorPrimaries);
                  break;
              }
            }
            break;
        case GBM_METADATA_GET_MAP_SECURE_BUFFER:
            *((uint32_t *)param) = data->map_secure_buffer;
            break;
        case GBM_METADATA_GET_SECURE_BUF_STAT:
            *((int *)param) = data->is_buffer_secure;
            break;
        case GBM_METADATA_GET_S3DFORMAT:
            *((uint32_t *)param) = data->s3d_format;
            break;
        case GBM_METADATA_GET_LINEAR_FORMAT:
            *((uint32_t *)param) = data->linear_format;
            break;
        case GBM_METADATA_GET_IGC:
            *((int *)param) = data->igc;
            break;
        case GBM_METADATA_GET_COLOR_METADATA:
            *((ColorMetaData *)param) = data->color_info;
            break;
        case GBM_METADATA_GET_UBWC_BUF_STAT:
            *((int *)param) = data->is_buffer_ubwc;
            break;
        case GBM_METADATA_GET_VT_TIMESTAMP:
            *((uint64_t *)param) = data->vt_timestamp;
            break;
        case GBM_METADATA_GET_VIDEO_PERF_MODE:
            *((uint32_t *)param) = data->isVideoPerfMode;
            break;
        case GBM_METADATA_GET_CVP_METADATA:
            *((CVPMetadata *)param) = data->cvpMetadata;
            break;
        case GBM_METADATA_GET_VIDEO_HIST_STAT:
            *((struct VideoHistogramMetadata *)param) = data->histMetadata;
            break;
        default:
            LOG(LOG_ERR," Operation currently not supported\n");
            res = GBM_ERROR_UNSUPPORTED;
            break;
    }

    return res;
}


void get_yuv_sp_plane_info(int width, int height, int bpp,
                       generic_buf_layout_t *buf_lyt)
{
    unsigned int ystride, cstride;

    ystride=width * bpp;
    cstride=width * bpp;

    buf_lyt->num_planes = DUAL_PLANES;

    buf_lyt->planes[0].top_left = buf_lyt->planes[0].offset = 0;
    buf_lyt->planes[1].top_left = buf_lyt->planes[1].offset = ystride * height;
    buf_lyt->planes[2].top_left = buf_lyt->planes[2].offset = ystride * height + 1;
    buf_lyt->planes[0].v_increment = ystride; //stride     in bytes
    buf_lyt->planes[1].v_increment = cstride;
    buf_lyt->planes[2].v_increment = cstride;
    buf_lyt->planes[0].h_increment = CHROMA_STEP*bpp; //chroma step
    buf_lyt->planes[1].h_increment = CHROMA_STEP*bpp;
    buf_lyt->planes[2].h_increment = CHROMA_STEP*bpp;
    buf_lyt->planes[0].bits_per_component = bpp;
    buf_lyt->planes[1].bits_per_component = bpp;
    buf_lyt->planes[2].bits_per_component = bpp;
    buf_lyt->planes[0].aligned_width = width;
    buf_lyt->planes[1].aligned_width = width;
    buf_lyt->planes[2].aligned_width = width;
    buf_lyt->planes[0].stride = width * bpp;
    buf_lyt->planes[1].stride = width * bpp;
}


void get_yuv_ubwc_sp_plane_info(int width, int height,
                          int color_format, generic_buf_layout_t *buf_lyt)
{
    unsigned int y_meta_stride = 0, y_meta_height = 0, y_meta_size = 0;
    unsigned int y_stride = 0, y_height = 0, y_size = 0;
    unsigned int c_meta_stride = 0, c_meta_height = 0, c_meta_size = 0;
    unsigned int c_stride = 0;
    unsigned int alignment = 4096;

    y_meta_stride = MMM_COLOR_FMT_Y_META_STRIDE(color_format, width);
    y_meta_height = MMM_COLOR_FMT_Y_META_SCANLINES(color_format, height);
    y_meta_size = ALIGN((y_meta_stride * y_meta_height), alignment);

    y_stride = MMM_COLOR_FMT_Y_STRIDE(color_format, width);
    y_height = MMM_COLOR_FMT_Y_SCANLINES(color_format, height);
    y_size = ALIGN((y_stride * y_height), alignment);

    c_meta_stride = MMM_COLOR_FMT_UV_META_STRIDE(color_format, width);
    c_meta_height = MMM_COLOR_FMT_UV_META_SCANLINES(color_format, height);
    c_meta_size = ALIGN((c_meta_stride * c_meta_height), alignment);

    c_stride = MMM_COLOR_FMT_UV_STRIDE(color_format, width);

    buf_lyt->num_planes = 4;

    /*
     * Actually when gl-render create EGL img, GFX only used two plane for NV12 UBWC
     * So we need to extract and remap them to buf_lyt according to following sequence:
     * Y_Plane, UV_Plane, Y_Meta_Plane, UV_Meta_Plane
     * Plane[0]=Y_Plane, Plane[1]=UV_Plane, and GFX would process UBWC(meta plane) internally
     * Then we can get the right layout
     * Original Buffer: |---Y_meta---|---Y---|---UV_meta---|---UV---|
     * After extract for GFX: |---Y---|---UV---|---Y_meta---|---UV_meta---|
     */
    buf_lyt->planes[0].top_left = buf_lyt->planes[0].offset = y_meta_size;
    buf_lyt->planes[1].top_left = buf_lyt->planes[1].offset = y_meta_size + y_size + c_meta_size;
    buf_lyt->planes[2].top_left = buf_lyt->planes[2].offset = 0;
    buf_lyt->planes[3].top_left = buf_lyt->planes[3].offset = y_meta_size + y_size;
    buf_lyt->planes[0].stride = buf_lyt->planes[0].v_increment = y_stride;
    buf_lyt->planes[1].stride = buf_lyt->planes[1].v_increment = c_stride;
    buf_lyt->planes[2].stride = buf_lyt->planes[2].v_increment = y_meta_stride;
    buf_lyt->planes[3].stride = buf_lyt->planes[3].v_increment = c_meta_stride;
}

int msmgbm_yuv_plane_info(struct gbm_bo *gbo,generic_buf_layout_t *buf_lyt){
    struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(gbo);
    int res = GBM_ERROR_NONE;

    if(!msm_gbm_bo || !buf_lyt)
        return GBM_ERROR_BAD_HANDLE;

     switch(gbo->format){
       //Semiplanar
        case GBM_FORMAT_YCbCr_420_SP:
        case GBM_FORMAT_YCrCb_420_SP:
        case GBM_FORMAT_YCbCr_420_SP_VENUS:
        case GBM_FORMAT_NV12_ENCODEABLE: //Same as YCbCr_420_SP_VENUS
#ifdef COLOR_FMT_NV12_512
        case GBM_FORMAT_NV12_HEIF:
#endif
        case GBM_FORMAT_NV12:
        case GBM_FORMAT_NV21_ZSL:
        case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
            if (is_ubwc_enabled(gbo->format, gbo->usage_flags, gbo->usage_flags))
                get_yuv_ubwc_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                           MMM_COLOR_FMT_NV12_UBWC, buf_lyt);
            else
                get_yuv_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                      YUV_420_SP_BPP, buf_lyt);
            break;
        case GBM_FORMAT_YCbCr_420_TP10_UBWC:
            get_yuv_ubwc_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                       MMM_COLOR_FMT_NV12_BPP10_UBWC, buf_lyt);
            break;
        case GBM_FORMAT_YCbCr_420_P010_UBWC:
            get_yuv_ubwc_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                       MMM_COLOR_FMT_P010_UBWC, buf_lyt);
            break;
        case GBM_FORMAT_P010:
        case GBM_FORMAT_YCbCr_420_P010_VENUS:
            get_yuv_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                  CHROMA_STEP, buf_lyt);
            break;
        case GBM_FORMAT_UYVY:
            get_yuv_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                  YUV_422_SP_BPP, buf_lyt);
            buf_lyt->num_planes = 1;
            break;
        default:
            res = GBM_ERROR_UNSUPPORTED;
            break;
     }

    return res;
}

void msmgbm_log_hdr_color_info_mdata(ColorMetaData * color_mdata)
{
    uint8_t i = 0;
    uint8_t j = 0;

    LOG(LOG_DBG,"setMetaData COLOR_METADATA : color_primaries = 0x%x,"
                "range = 0x%x, transfer = 0x%x, matrix = 0x%x",
                 color_mdata->colorPrimaries, color_mdata->range,
                 color_mdata->transfer, color_mdata->matrixCoefficients);

    for(i = 0; i < 3; i++) {
        for(j = 0; j < 2; j++) {
            LOG(LOG_DBG,"setMetadata COLOR_METADATA : rgb_primaries[%d][%d] = 0x%x",
                i, j, color_mdata->masteringDisplayInfo.primaries.rgbPrimaries[i][j]);
        }
    }

    LOG(LOG_DBG,"setMetadata COLOR_METADATA : white_point[0] = 0x%x white_point[1] = 0x%x",
                    color_mdata->masteringDisplayInfo.primaries.whitePoint[0],
                    color_mdata->masteringDisplayInfo.primaries.whitePoint[1]);

    LOG(LOG_DBG,"setMetadata COLOR_METADATA : max_disp_lum = 0x%x min_disp_lum = 0x%x",
                    color_mdata->masteringDisplayInfo.maxDisplayLuminance,
                    color_mdata->masteringDisplayInfo.minDisplayLuminance);

    LOG(LOG_DBG,"setMetadata COLOR_METADATA : max_cll = 0x%x min_pall = 0x%x",
                    color_mdata->contentLightLevel.maxContentLightLevel,
                    color_mdata->contentLightLevel.minPicAverageLightLevel);

}


void msmsgbm_default_init_hdr_color_info_mdata(ColorMetaData * color_mdata)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t k = 0;

    color_mdata->colorPrimaries      = 0xAB;
    color_mdata->range               = 0xCD;
    color_mdata->transfer            = 0xEF;
    color_mdata->matrixCoefficients  = 0xDE;

    for(i = 0, k = 0xAE; i < 3; i++) {
        for(j = 0; j < 2; j++, k++)
            color_mdata->masteringDisplayInfo.primaries.rgbPrimaries[i][j] =(i+j+k);
    }

    color_mdata->masteringDisplayInfo.primaries.whitePoint[0]   = 0xFA;
    color_mdata->masteringDisplayInfo.primaries.whitePoint[1]   = 0xFB;
    color_mdata->masteringDisplayInfo.maxDisplayLuminance   = 0xABCEDF00;
    color_mdata->masteringDisplayInfo.minDisplayLuminance   = 0xFABADEEF;
    color_mdata->contentLightLevel.maxContentLightLevel     = 0xDAA0BAAC;
    color_mdata->contentLightLevel.minPicAverageLightLevel  = 0xFAB0C007;

}



int msmgbm_get_buf_lyout(struct gbm_bo *gbo, generic_buf_layout_t *buf_lyt)
{
    struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(gbo);
    int res = GBM_ERROR_NONE;
    int Bpp;

    if(!gbo || !buf_lyt)
        return GBM_ERROR_BAD_HANDLE;

    if(gbo->width  <= 0 || gbo->height <= 0){
        LOG(LOG_ERR,"INVALID width or height\n");
        return NULL;
    }

    if(1 == IsFormatSupported(gbo->format))
        Bpp = GetFormatBpp(gbo->format);
    else
    {
        LOG(LOG_ERR,"Format (0x%x) not supported\n",gbo->format);
        return NULL;
    }

    buf_lyt->pixel_format = gbo->format;

    if(is_format_rgb(gbo->format))
    {
        buf_lyt->num_planes = 1;
        buf_lyt->planes[0].aligned_width = gbo->aligned_width;
        buf_lyt->planes[0].aligned_height = gbo->aligned_height;
        buf_lyt->planes[0].top_left = buf_lyt->planes[0].offset = 0;
        buf_lyt->planes[0].bits_per_component = Bpp;
        buf_lyt->planes[0].v_increment = ((gbo->aligned_width)*Bpp); //stride
    }
    else
    {
        switch(gbo->format){
           //Semiplanar
            case GBM_FORMAT_YCbCr_420_SP:
            case GBM_FORMAT_YCrCb_420_SP:
            case GBM_FORMAT_YCbCr_420_SP_VENUS:
            case GBM_FORMAT_NV12:
            case GBM_FORMAT_NV12_ENCODEABLE: //Same as YCbCr_420_SP_VENUS
#ifdef COLOR_FMT_NV12_512
            case GBM_FORMAT_NV12_HEIF:
#endif
                 get_yuv_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                       YUV_420_SP_BPP, buf_lyt);
                 break;
            case GBM_FORMAT_YCbCr_420_TP10_UBWC:
                 get_yuv_ubwc_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                            MMM_COLOR_FMT_NV12_BPP10_UBWC, buf_lyt);
                 break;
            case GBM_FORMAT_P010:
                get_yuv_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                      CHROMA_STEP, buf_lyt);
                break;
            case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
                get_yuv_ubwc_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                           MMM_COLOR_FMT_NV12_UBWC, buf_lyt);
                break;
            default:
                 res = GBM_ERROR_UNSUPPORTED;
                 break;
        }
    }
    return res;
}

//File read for debug level configuration
void config_dbg_lvl(void)
{
    FILE *fp = NULL;

    fp = fopen("/data/misc/display/gbm_dbg_cfg.txt", "r");
    if(fp) {
        fscanf(fp, "%d", &g_debug_level);
        LOG(LOG_INFO,"\nGBM debug level set=%d\n",g_debug_level);
        fclose(fp);
    }
}

//helper function to get timestamp in usec
void get_time_in_usec(long long int *time_usec)
{
  struct timeval timer_usec;
  long long int timestamp_usec; /* timestamp in microsecond */
  if (!gettimeofday(&timer_usec, NULL)) {
    timestamp_usec = ((long long int) timer_usec.tv_sec) * 1000000ll +
                        (long long int) timer_usec.tv_usec;
  }
  else {
    timestamp_usec = -1;
  }
  printf("%lld microseconds since epoch\n", timestamp_usec);

  *time_usec = timestamp_usec;
}


int msmgbm_bo_dump(struct gbm_bo * gbo)
{
    FILE *fptr = NULL;
    static int count = 1;
    const char file_nme[100] = "/data/misc/display/gbm_dump";
    struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(gbo);
    int mappedNow = 0;
    size_t size = gbo->size;
    int ret = GBM_ERROR_NONE;
    char tmp_str[50];
    long long int time_usec;
    uint32_t width = gbo->width;
    uint32_t height = gbo->height;
    uint32_t format = gbo->format;
    int ion_fd = gbo->ion_fd;

    //Dump Files are created per dump call reference
    //Get time in usec from system
    get_time_in_usec(&time_usec);

    snprintf(tmp_str, sizeof(tmp_str), "__%lld_%d_%d_%d_%d_%d",
                        time_usec,width,height,format,ion_fd,getpid());
    strlcat(file_nme,tmp_str, sizeof(file_nme));
    strlcat(file_nme,".dat", sizeof(file_nme));

    fptr=fopen(file_nme, "w+");
    if(fptr == NULL)
    {
        LOG(LOG_ERR,"Failed to open file %s\n",file_nme);
        return GBM_ERROR_BAD_HANDLE;
    }

    if(msm_gbm_bo->cpuaddr == NULL)
    {
        if(msmgbm_bo_cpu_map(gbo) == NULL){
             LOG(LOG_ERR,"Unable to Map to CPU, cannot write to BO\n");
             if(fptr)
                fclose(fptr);
             return GBM_ERROR_BAD_HANDLE;
        }
        mappedNow =1;
    }

    //Read from BO and write to file
    ret = fwrite(msm_gbm_bo->cpuaddr, 1, size, fptr);
    if(ret != size)
    {
        LOG(LOG_ERR,"File write size mismatch i/p=%d o/p=%d\n %s\n",size,ret,strerror(errno));
        ret = GBM_ERROR_BAD_VALUE;
    }else
        ret = GBM_ERROR_NONE;

    if(mappedNow){ //Unmap BO, if we mapped it.
        msmgbm_bo_cpu_unmap(gbo);
    }

    if(fptr)
      fclose(fptr);

    return ret;
}
