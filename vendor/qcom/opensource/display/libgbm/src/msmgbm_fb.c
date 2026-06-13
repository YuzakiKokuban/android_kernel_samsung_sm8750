/*
* Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
* Not a Contribution.
*
* Copyright (c) 2017-2018, 2021 The Linux Foundation. All rights reserved.
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
#include <linux/msm_ion.h>
#include <linux/ion.h>
#include <gbm_priv.h>
#include <msmgbm.h>
#include <msmgbm_common.h>
#include <linux/version.h>
#include <display/media/mmm_color_fmt.h>
#include <linux/msm_mdp.h>
#ifdef BUILD_HAS_WAYLAND_SUPPORT
#include <wayland-server.h>
#endif
#ifdef USE_GLIB
#define strlcat g_strlcat
#define strlcpy g_strlcpy
#endif
#define FB_DEVICE_NAME1 "/dev/fb0"
#define FB_DEVICE_NAME2 "/dev/graphics/fb0"
#define RENDER_DEVICE_NAME "/dev/dri/renderD128"
#define ION_DEVICE_NAME "/dev/ion"
#define YUV_420_SP_BPP  1
#define MAX_YUV_PLANES  3
#define DUAL_PLANES     2
#define CHROMA_STEP     2
#define msmgbm_perform gbm_perform
#define msmgbm_get_priv gbm_get_priv
#define PAGE_SIZE (4096)
#define ROUND_UP_PAGESIZE(x) (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1)
#define ALIGN(x, align) (((x) + ((align)-1)) & ~((align)-1))

//Global variables
int g_debug_level = LOG_INFO;

//Global Variables
static pthread_mutex_t mutex_obj = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t surface_mutex = PTHREAD_MUTEX_INITIALIZER;
static inline void lock_init(void)
{
    if(pthread_mutex_init(&mutex_obj, NULL))
    {
        LOG(LOG_ERR,"Failed to init Mutex\n %s\n",strerror(errno));
        return NULL;
    }
    if(pthread_mutex_init(&surface_mutex, NULL))
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

static inline void surface_lock(void)
{
    if(pthread_mutex_lock(&surface_mutex))
    {
        LOG(LOG_ERR,"Failed to lock Surface Mutex\n %s\n",strerror(errno));
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

static inline void surface_unlock(void)
{
    if(pthread_mutex_unlock(&surface_mutex))
    {
        LOG(LOG_ERR,"Failed to un lock surface Mutex\n %s\n",strerror(errno));
        return NULL;
    }

}

static inline void lock_destroy(void)
{
    if(pthread_mutex_destroy(&mutex_obj))
        LOG(LOG_ERR,"Failed to init Mutex\n %s\n",strerror(errno));

}

static inline void surface_lock_destroy(void)
{
    if(pthread_mutex_destroy(&surface_mutex))
        LOG(LOG_ERR,"Failed to destroy Surface Mutex\n %s\n",strerror(errno));

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

    int ret = 0;

    if(NULL != msm_gbm_bo){

        LOG(LOG_DBG,"\nmsm_gbm_bo->cpuaddr=0x%x\n msm_gbm_bo->mt_cpuaddr=0x%x\n",
                                msm_gbm_bo->cpuaddr, msm_gbm_bo->mt_cpuaddr);

        LOG(LOG_DBG,"Destroy called for fd=%d",bo->ion_fd);

         //Delete the Map entries if any
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
         * Free the msm_gbo object
         */
        if(msm_gbm_bo != NULL){
            LOG(LOG_DBG,"msm_gbm_bo handle to be freed for BO=%p\n",msm_gbm_bo);
            free(msm_gbm_bo);
            msm_gbm_bo = NULL;
        }
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
        case GBM_FORMAT_P010:
        case GBM_FORMAT_NV21_ZSL:
        case GBM_FORMAT_YCbCr_420_888:
        case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
        case GBM_FORMAT_YCbCr_420_TP10_UBWC:
        case GBM_FORMAT_YCbCr_420_P010_UBWC:
        case GBM_FORMAT_YCbCr_420_P010_VENUS:
             LOG(LOG_DBG,"YUV format BPP\n");
            return 1;
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
        case GBM_FORMAT_ABGR2101010:
        case GBM_FORMAT_NV21_ZSL:
        case GBM_FORMAT_YCbCr_420_888:
        case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
        case GBM_FORMAT_YCbCr_420_TP10_UBWC:
        case GBM_FORMAT_P010:
        case GBM_FORMAT_YCbCr_420_P010_VENUS:
        case GBM_FORMAT_YCbCr_420_P010_UBWC:
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

static void
msmgbm_metadata_create(struct msmgbm_device *msm_dev, struct gbm_bo *gbmbo)
{
   //Reset the data objects to be used for ION IOCTL's
  struct ion_allocation_data ionAllocData;
  int mt_size = 0;
  struct msmgbm_bo *msm_gbmbo = to_msmgbm_bo(gbmbo);
  void *mt_base = NULL;
  memset(&ionAllocData, 0, sizeof(ionAllocData));

  ionAllocData.len = sizeof(struct meta_data_t);
  ionAllocData.heap_id_mask= ION_HEAP(ION_SYSTEM_HEAP_ID); /* System Heap */
  ionAllocData.flags |= ION_FLAG_CACHED;
  mt_size = ionAllocData.len;

  if((ioctl(msm_dev->iondev_fd, ION_IOC_ALLOC, &ionAllocData)) == 0){

      LOG(LOG_DBG,"ionAllocData.fd:= %d\n",ionAllocData.fd);

      mt_base = msmgbm_cpu_map_metafd(ionAllocData.fd, mt_size);
      if(mt_base == NULL) {
          LOG(LOG_ERR,"Failed to do  mapping on Metadata BO Err:\n%s\n",strerror(errno));
          return NULL;
      }
      LOG(LOG_DBG,"MT_BO Mapped Addr:= %p\n",mt_base);

      // Initiliaze the meta_data structure
      memset(mt_base, 0 , mt_size);
  }else
  {
      LOG(LOG_ERR,"Failed ION_IOC_ALLOC on Metadata BO Err:\n%s\n",strerror(errno));
      return NULL;
  }
  gbmbo->ion_metadata_fd = ionAllocData.fd;
  msm_gbmbo->mt_size = mt_size;
  msm_gbmbo->mt_cpuaddr = mt_base;
}

static struct gbm_bo *
msmgbm_bo_create(struct gbm_device *gbm,
              uint32_t width, uint32_t height,
              uint32_t format, uint32_t usage)
{
    int ret = 0;
    void *base = NULL;
    void *mt_base = NULL;
    uint32_t aligned_width;
    uint32_t aligned_height;
    uint32_t bo_handles[4] = {0};
    uint32_t pitches[4] = {0};
    uint32_t offsets[4] = {0};
    uint32_t flags = 0;
    uint32_t Bpp = 0;
    uint32_t size = 0;
    uint32_t mt_size = 0;
    int data_fd = 0;
    int mt_data_fd = 0;
    struct msmgbm_device *msm_dev = to_msmgbm_device(gbm);
    struct gbm_bo *gbmbo = NULL;
    struct msmgbm_bo *msm_gbmbo = NULL;
    struct ion_allocation_data ionAllocData;
    struct gbm_bufdesc bufdesc={width,height,format,usage};

    if(msm_dev == NULL){
        LOG(LOG_ERR,"INVALID Device pointer\n");
        return NULL;
    }

    if(width  <= 0 || height <=0){
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

    /*
     * Depending on the usage flag settinggs we check for the heap from which the ION buffer
     * has to be allocated from.
     * Also cache/non cache buffer allocation
     */
    ionAllocData.heap_id_mask = GetIonHeapId(usage);
    ionAllocData.flags = GetIonAllocFlags(usage);

    ionAllocData.len = size;

    //This ioctl should have failed for a wrong fd, but it does not returns 0
    if(!(ioctl(msm_dev->iondev_fd, ION_IOC_ALLOC, &ionAllocData))){

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
    }else{
        LOG(LOG_ERR,"Failed ION_IOC_ALLOC on BO Err:\n%s\n",strerror(errno));
        return NULL;
    }

    /* To get ion_fd and gem handle for the metadata structure
     * Alignment of the buffer is fixed to Page size
     * ION Memory is from, the System heap
     * We get the gem handle from the ion fd using PRIME ioctls
     */

   //Reset the data objects to be used for ION IOCTL's
    memset(&ionAllocData, 0, sizeof(ionAllocData));

    ionAllocData.len = sizeof(struct meta_data_t);
    ionAllocData.heap_id_mask= ION_HEAP(ION_SYSTEM_HEAP_ID); /* System Heap */
    ionAllocData.flags |= ION_FLAG_CACHED;

    mt_size = ionAllocData.len;

    if((ioctl(msm_dev->iondev_fd, ION_IOC_ALLOC, &ionAllocData)) == 0){

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

    }else
    {
        LOG(LOG_ERR,"Failed ION_IOC_ALLOC on Metadata BO Err:\n%s\n",strerror(errno));
        return NULL;
    }

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

    gbmbo = &msm_gbmbo->base;
    gbmbo->ion_fd = data_fd;
    gbmbo->ion_metadata_fd = mt_data_fd;
    gbmbo->fbid = 0;
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
    gbmbo->bo_get_device = msmgbm_bo_get_device;
    gbmbo->bo_write = msmgbm_bo_write;
    msm_gbmbo->device = msm_dev;
    msm_gbmbo->cpuaddr = base;
    msm_gbmbo->mt_cpuaddr = mt_base;
    msm_gbmbo->current_state =  GBM_BO_STATE_FREE;
    msm_gbmbo->size = size;
    msm_gbmbo->mt_size = mt_size;
    msm_gbmbo->magic = QCMAGIC;

    bo_handles[0] = gbmbo->handle.u32;
    pitches[0] = gbmbo->stride;
    return gbmbo;
}

struct gbm_bo *
msmgbm_bo_import_fd(struct msmgbm_device *msm_dev,
                                                      void *buffer, uint32_t usage)
{
    struct gbm_bo *gbmbo = NULL;
    struct msmgbm_bo *msm_gbmbo = NULL;
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

    if(search_hashmap(buffer_info->fd, &gbo_info, &gbo_private_info) == GBM_ERROR_NONE)
    {
        LOG(LOG_DBG,"Map retrieved buf info\n gbm_buf_info.width=%d\n",
                                                        gbo_info.width);
        LOG(LOG_DBG,"gbm_buf_info.fd,gbm_buf_info.metadata_fd,"
                    "gbm_buf_info.height=%d\n gbm_buf_info.format = %d\n",
                    gbo_info.fd,gbo_info.metadata_fd,gbo_info.height,gbo_info.format);

        lock();
        //we have a valid entry within the map table so Increment ref count
        incr_refcnt(buffer_info->fd);
        unlock();
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
        lock();
        register_to_hashmap(buffer_info->fd, &gbo_info, &gbo_private_info);
        incr_refcnt(buffer_info->fd);
        unlock();

    }

    LOG(LOG_DBG," format: 0x%x width: %d height: %d \n",buffer_info->format, buffer_info->width, buffer_info->height);

    if(1 == IsFormatSupported(buffer_info->format))
        Bpp = GetFormatBpp(buffer_info->format);
    else
    {
        LOG(LOG_ERR,"Format (0x%x) not supported\n",buffer_info->format);
        return NULL;
    }

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
    msm_gbmbo->import_flg = 1;

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
    ret = search_hashmap(buffer_info->fd, &gbo_info, &gbo_private_info);
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

    msm_gbmbo = (struct msmgbm_bo *)calloc(1, sizeof(struct msmgbm_bo));

    if (msm_gbmbo == NULL) {
        LOG(LOG_ERR,"Unable to allocate BO\n");
        return NULL;
    }

    gbmbo                = &msm_gbmbo->base;
    gbmbo->ion_fd        = buffer_info->fd;
    gbmbo->ion_metadata_fd = buffer_info->metadata_fd;
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
    gbmbo->bo_get_device = msmgbm_bo_get_device;
    gbmbo->bo_write      = msmgbm_bo_write;
    msm_gbmbo->device    = msm_dev;
    msm_gbmbo->cpuaddr  = gbo_private_info.cpuaddr;
    msm_gbmbo->mt_cpuaddr = gbo_private_info.mt_cpuaddr;
    msm_gbmbo->current_state   =  GBM_BO_STATE_FREE;
    msm_gbmbo->size      = size;
    msm_gbmbo->mt_size   = mt_size;
    msm_gbmbo->magic     = QCMAGIC;
    msm_gbmbo->import_flg = 1;

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
msmgbm_bo_import_gbm_buf(struct msmgbm_device *msm_dev,
                                                      void *buffer, uint32_t usage)
{
    struct gbm_bo *gbmbo = NULL;
    struct msmgbm_bo *msm_gbmbo = NULL;
    struct gbm_buf_info *buffer_info = NULL;
    struct gbm_device* gbm_dev = &(msm_dev->base);
    struct gbm_bufdesc bufdesc;
    int ret = 0;
    int Bpp=0;
    unsigned int size = 0, mt_size;
    unsigned int aligned_width;
    unsigned int aligned_height;

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

    msm_gbmbo = (struct msmgbm_bo *)calloc(1, sizeof(struct msmgbm_bo));

    if (msm_gbmbo == NULL) {
        LOG(LOG_ERR," Unable to allocate BO OoM\n");
        return NULL;
    }

    gbmbo                  = &msm_gbmbo->base;
    gbmbo->ion_fd          = buffer_info->fd;
    gbmbo->ion_metadata_fd = buffer_info->metadata_fd;
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
    gbmbo->bo_get_device   = msmgbm_bo_get_device;
    gbmbo->bo_write        = msmgbm_bo_write;
    msm_gbmbo->device      = msm_dev;
    msm_gbmbo->cpuaddr   = gbo_private_info.cpuaddr;
    msm_gbmbo->mt_cpuaddr = gbo_private_info.mt_cpuaddr;
    msm_gbmbo->current_state   =  GBM_BO_STATE_FREE;
    msm_gbmbo->size            = size;
    msm_gbmbo->mt_size         = mt_size;
    msm_gbmbo->magic           = QCMAGIC;
    msm_gbmbo->import_flg      = 1;

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
    surface_lock();
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
                surface_unlock();
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
                surface_unlock();
                return  &msm_gbm_surface->bo[index]->base;
            }
        }
        LOG(LOG_ERR,"No Front BO found\n");
#endif
    }
    else {
        LOG(LOG_ERR," NULL or Invalid surface pointer\n");
    }
    surface_unlock();
    return NULL;
}

static void
msmgbm_surface_release_buffer(struct gbm_surface *surf, struct gbm_bo *bo)
{
    surface_lock();
    struct msmgbm_surface *msm_gbm_surf = to_msmgbm_surface(surf);
    struct msmgbm_bo *msm_gbm_bo = to_msmgbm_bo(bo);
    int index =0;

    if((msm_gbm_surf == NULL) || (msm_gbm_bo == NULL)) {
         LOG(LOG_ERR," Invalid surface or BO pointer\n");
         surface_unlock();
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
           surface_unlock();
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
            surface_unlock();
            return;
        }
    }
   LOG(LOG_ERR,"Invalid Input BO, BO is not locked\n");
#endif
}

static int
msmgbm_surface_has_free_buffers(struct gbm_surface *surf)
{
    surface_lock();
    struct msmgbm_surface *msm_gbm_surface = to_msmgbm_surface(surf);
    int index;

    if(msm_gbm_surface != NULL){
#ifdef ALLOCATE_SURFACE_BO_AT_CREATION
        for(index =0; index < NUM_BACK_BUFFERS; index++) {
            if((msm_gbm_surface->bo[index]!= NULL) &&(msm_gbm_surface->bo[index]->current_state == GBM_BO_STATE_FREE)){
                 surface_unlock();
                 return 1;
            }
        }
#else
        for(index =0; index < NUM_BACK_BUFFERS; index++) {
            if(msm_gbm_surface->bo_slot[index] == SURFACE_BOSLOT_STATE_FREE){
                surface_unlock();
                return 1;
            }
        }
#endif
    }
    else {
         LOG(LOG_ERR," NULL or Invalid surface pointer\n");
    }
    surface_unlock();
    return 0;
}

static struct gbm_surface *
msmgbm_surface_create(struct gbm_device *gbm,
                                                 uint32_t width, uint32_t height,
                                                 uint32_t format, uint32_t flags)
{
    struct msmgbm_device *msm_dev = to_msmgbm_device(gbm);
    struct gbm_surface *gsurf = NULL;
    struct msmgbm_surface*msm_gbmsurf = NULL;
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
#ifdef ALLOCATE_SURFACE_BO_AT_CREATION
    for(index =0; index < NUM_BACK_BUFFERS; index++) {
       msm_gbmsurf->bo[index] = to_msmgbm_bo(msmgbm_bo_create(gbm, width, height, format, flags));
       if(msm_gbmsurf->bo[index] == NULL){
           LOG(LOG_ERR," Unable to create Surface BO %d\n", index);
           free_surface_bo(msm_gbmsurf, index);
           return NULL;
       }
    }
#else
  struct msmfb_metadata metadata;
  int fd = gbm->fd;
  struct fb_var_screeninfo vi;
  struct fb_fix_screeninfo fi;
  /* Probe the device for screen information. */
  if (ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0 ||
       ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0) {
       return -1;
  }
  int double_buffered = 0;
  if ((vi.yres * fi.line_length * 2) <= fi.smem_len) {
    double_buffered = 1;
    fprintf(stderr,"%s(%d): FB is double buffered \n",__func__,__LINE__);
  } else {
    double_buffered = 0;
    fprintf(stderr,"%s(%d): FB is single buffered \n",__func__,__LINE__);
  }
  memset(&metadata, 0 , sizeof(metadata));
  metadata.op = metadata_op_get_ion_fd;
  void* bits = mmap(0, fi.smem_len, PROT_WRITE, MAP_SHARED, fd, 0);
  if (ioctl(fd, MSMFB_METADATA_GET, &metadata) == -1) {
    fprintf(stderr,"%s(%d): MSMFB_METADATA_GET ioctl failed \n",__func__,__LINE__);
  }
  if(metadata.data.fbmem_ionfd < 0) {
    fprintf(stderr,"%s(%d): Invalid ion fd handle %d\n",__func__,__LINE__, metadata.data.fbmem_ionfd);
  }
  fprintf(stderr,"%s(%d): imported ion_fd=%d  for fbdev fd=%d\n",__func__,__LINE__,metadata.data.fbmem_ionfd,fd);
  struct gbm_import_fd_data fd_data;
  fd_data.fd = metadata.data.fbmem_ionfd;
  fd_data.width = width;
  fd_data.height = height;
  fd_data.stride = width;
  fd_data.format = format;
  if(double_buffered) {
    for(index =0; index < NUM_BACK_BUFFERS; index++) {
      void *base_cpu_addr;
      struct gbm_bo *bo = msmgbm_bo_import_fd(gbm, &fd_data, flags);
      msmgbm_metadata_create(msm_dev,bo);
      msm_gbmsurf->bo[index] = to_msmgbm_bo(bo);
      msm_gbmsurf->bo[index]->current_state = GBM_BO_STATE_FREE;
      if(msm_gbmsurf->bo[index] == NULL){
        LOG(LOG_ERR," Unable to create Surface BO %d\n", index);
        free_surface_bo(msm_gbmsurf, index);
        return NULL;
      }
      if(index==0) {
        base_cpu_addr = msmgbm_bo_cpu_map(bo);
        msm_gbmsurf->bo[0]->cpuaddr = base_cpu_addr;
      } else {
        msm_gbmsurf->bo[index]->cpuaddr = base_cpu_addr + (fi.line_length)*(gsurf->height)*index ;
      }
    }
  } else {
    LOG(LOG_ERR," Kernel allocated FB memory is single buffered \n");
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

    lock_destroy();

    LOG(LOG_DBG, "iondev_fd:%d \n", msm_dev->iondev_fd);
    //Close the ion device fd
    if(msm_dev->iondev_fd > 0)
        close(msm_dev->iondev_fd);

    if(msm_dev != NULL){
        free(msm_dev);
        msm_dev = NULL;
    }
    else {

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

    lock_init();

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
    gbmdevice->bo_import = msmgbm_bo_import;
    gbmdevice->surface_create = msmgbm_surface_create;
    msm_gbmdevice->fd = fd;
    msm_gbmdevice->magic = QCMAGIC;

    LOG(LOG_DBG,"gbm device fd= %d\n",gbmdevice->fd);

    return gbmdevice;
}

struct gbm_backendpriv g_msm_priv = {
   .backend_name = "fb", //As this will be using FB
   .create_device = msmgbm_device_create,
};

struct gbm_backendpriv *msmgbm_get_priv(void)
{
    return &g_msm_priv;
}

unsigned int msmgbm_device_get_magic(struct gbm_device *dev)
{
    struct msmgbm_device *msm_dev = to_msmgbm_device( dev);

    if(msm_dev == NULL){
        LOG(LOG_ERR,"NULL or Invalid device pointer\n");
        return 0;
    }
    else
    {
        return QCMAGIC;
    }
}

int msmgbm_surface_set_front_bo(struct gbm_surface *surf, struct gbm_bo *bo)
{
    surface_lock();
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
                     surface_unlock();
                     return GBM_ERROR_NONE;
            }
        }
        LOG(LOG_ERR," INVALID BO, Passed BO was not obtained using \
                                msmgbm_surface_get_free_bo\n");
        surface_unlock();
        return GBM_ERROR_NO_RESOURCES;
#else
        for(index =0; index < NUM_BACK_BUFFERS; index++)
        {
            if(msm_gbm_surface->bo_slot[index] == SURFACE_BOSLOT_STATE_FREE)
            {
                msm_gbm_surface->bo_slot[index] = SURFACE_BOSLOT_STATE_HAS_NEW_FRONT_BUFFER;
                msm_gbm_surface->bo[index] = msm_gbm_bo;
                msm_gbm_surface->bo[index]->current_state = GBM_BO_STATE_NEW_FRONT_BUFFER;
                surface_unlock();
                return GBM_ERROR_NONE;
            }
        }
        LOG(LOG_ERR," NO Free BO slot found!!\n");
        surface_unlock();
       return GBM_ERROR_NO_RESOURCES;
#endif
    }
    else
    {
         LOG(LOG_ERR," INVALID BO or Surface pointer\n");
         surface_unlock();
         return GBM_ERROR_BAD_HANDLE;
    }
    surface_unlock();
}

#ifdef ALLOCATE_SURFACE_BO_AT_CREATION
struct gbm_bo* msmgbm_surface_get_free_bo(struct gbm_surface *surf)
{
    surface_lock();
    struct msmgbm_surface *msm_gbm_surface = to_msmgbm_surface(surf);
    int index;

    if(msm_gbm_surface != NULL)
    {
            for(index =0; index < NUM_BACK_BUFFERS; index++)
            {
                if((msm_gbm_surface->bo[index]!= NULL) && \
                    (msm_gbm_surface->bo[index]->current_state == GBM_BO_STATE_FREE))
                {
                    msm_gbm_surface->bo[index]->current_state = GBM_BO_STATE_INUSE_BY_GPU;
                    surface_unlock();
                    return &msm_gbm_surface->bo[index]->base;
                }
            }
            LOG(LOG_ERR," NO Free BO found!!\n");
    }
    else
    {
        LOG(LOG_ERR," NULL or Invalid surface pointer\n");
    }
     surface_unlock();
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

            LOG(LOG_DBG," unmapping msm_gbm_bo->cpuaddr=0x%x\n",
                                           msm_gbm_bo->cpuaddr);
            if(munmap((void *)msm_gbm_bo->cpuaddr, bo->size))
                LOG(LOG_ERR," munmap failed for msm_gbm_bo->cpuaddr=0x%x ERR: %s\n",
                                                msm_gbm_bo->cpuaddr, strerror(errno));
        }
        msm_gbm_bo->cpuaddr = NULL;

        //Metadata buffer
        if (msm_gbm_bo->mt_cpuaddr != NULL)
        {
            LOG(LOG_DBG," unmapping msm_gbm_bo->mt_cpuaddr=0x%x\n",
                                           msm_gbm_bo->mt_cpuaddr);
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


int  msmgbm_device_authenticate_magic(struct gbm_device *dev, unsigned int magic){
    struct msmgbm_device *msm_dev = to_msmgbm_device(dev);

    if(msm_dev == NULL){
        LOG(LOG_ERR," NULL or Invalid device pointer\n");
        return GBM_ERROR_BAD_HANDLE;
    }
    else
    {
       if(magic == QCMAGIC) {
         return GBM_ERROR_NONE;
       } else {
          return GBM_ERROR_BAD_VALUE;
       }
    }

}

struct gbm_bo*  msmgbm_bo_import_from_name(struct gbm_device *dev, unsigned int name)
{
    struct msmgbm_device *msm_dev = to_msmgbm_device(dev);
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

    if (ret != 0){
        LOG(LOG_ERR," PRIME FD to Handle failed on device(%x), error = %d\n",
                                                        msm_dev,ret);
        return NULL;
    }

    msm_gbmbo = (struct msmgbm_bo *)calloc(1, sizeof(struct msmgbm_bo));

    if (msm_gbmbo == NULL) {
        LOG(LOG_ERR," Unable to allocate BO OoM\n");
        return NULL;
    }

    gbmbo =  &msm_gbmbo->base;
    gbmbo->ion_fd = fd;
    gbmbo->bo_destroy = msmgbm_bo_destroy;
    gbmbo->bo_get_fd= msmgbm_bo_get_fd;
    gbmbo->bo_get_device = msmgbm_bo_get_device;
    gbmbo->bo_write = msmgbm_bo_write;
    msm_gbmbo->device = msm_dev;
    msm_gbmbo->current_state =  GBM_BO_STATE_FREE;
    msm_gbmbo->magic = QCMAGIC;
    msm_gbmbo->name = name;

    return gbmbo;
}

int msmgbm_bo_get_name(struct gbm_bo* bo)
{
    struct msmgbm_bo *msm_gbmbo = to_msmgbm_bo(bo);
    int ret;

    if(NULL == msm_gbmbo){
        LOG(LOG_ERR," INVALID BO pointer\n");
        return -1;
    }

    if(0 == msm_gbmbo->name)
    {
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
                unsigned int *magic_id = va_arg(args,unsigned int*);

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
                unsigned int magic_id = va_arg(args,unsigned int);

                res = msmgbm_device_authenticate_magic(gbm_dev,magic_id);
            }
            break;
        case GBM_PERFORM_GET_DRM_DEVICE_NAME:
            {
                char *fb_dev_name = va_arg(args,char *);
                uint32_t size = va_arg(args, uint32_t);
                if(access(FB_DEVICE_NAME1, F_OK) >=0) {
                   strlcpy(fb_dev_name, FB_DEVICE_NAME1, size);
                } else {
                   strlcpy(fb_dev_name, FB_DEVICE_NAME2, size);
                }
                res = GBM_ERROR_NONE;
            }
            break;
        case GBM_PERFORM_GET_RENDER_DEVICE_NAME:
            {
                char *render_dev_name = va_arg(args,char *);
                uint32_t size = va_arg(args, uint32_t);
                strlcpy(render_dev_name, ION_DEVICE_NAME, size);
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
                    return GBM_ERROR_BAD_HANDLE;

                if((gbo->ion_metadata_fd) < 0)
                {
                    //Let us try looking through the map table in case if we have
                    //an update, since last import call?
                    struct gbm_buf_info temp_buf_info;
                    struct msmgbm_private_info gbo_private_info = {NULL, NULL};
                    res = search_hashmap(gbo->ion_fd, &temp_buf_info, &gbo_private_info);

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

                return GBM_ERROR_NONE;
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
        case GBM_PERFORM_GET_WL_RESOURCE_FROM_GBM_BUF_INFO:
            {
                struct gbm_buf_info *buffer_info = va_arg(args, struct gbm_buf_info *);
                struct wl_resource *resource = va_arg(args, struct wl_resource *);
                struct gbm_buf_resource *buf_resource;

                buf_resource = (struct gbm_buf_resource *)calloc(1, sizeof(struct gbm_buf_resource));
                buf_resource->buffer_info = buffer_info;
                buf_resource->magic = 0x1;
                wl_resource_set_user_data(resource, (void *)buf_resource);

                res = GBM_ERROR_NONE;
            }
            break;
        case GBM_PERFORM_GET_GBM_BUF_INFO_FROM_WL_RESOURCE:
            {
                struct wl_resource *resource = va_arg(args, struct wl_resource *);
                struct gbm_buf_info *buffer_info = va_arg(args, struct gbm_buf_info *);
                struct gbm_buf_resource *buf_resource;

                if (resource != NULL) {
                    buf_resource = wl_resource_get_user_data(resource);
                    if (buf_resource == NULL) {
                        LOG(LOG_ERR,"INVALID buffer_info\n");
                        res = GBM_ERROR_UNDEFINED;
                    } else if (buf_resource->magic != 0x1) {
                      LOG(LOG_ERR,"INVALID buffer_info\n");
                      res = GBM_ERROR_BAD_HANDLE;
                    } else {
                        buffer_info->fd = buf_resource->buffer_info->fd;
                        buffer_info->metadata_fd = buf_resource->buffer_info->metadata_fd;
                        buffer_info->width = buf_resource->buffer_info->width;
                        buffer_info->height = buf_resource->buffer_info->height;
                        buffer_info->format = buf_resource->buffer_info->format;
                        res = GBM_ERROR_NONE;
                    }
                }
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
    int map_flg = 0;

    if(!msm_gbm_bo)
        return GBM_ERROR_BAD_HANDLE;

    if((gbo->ion_metadata_fd) <= 0)
    {
        //Let us try looking through the map table in case if we have
        //an update, since last import call?
        struct gbm_buf_info temp_buf_info;
        struct msmgbm_private_info bo_private_info;
        res = search_hashmap(gbo->ion_fd, &temp_buf_info, &bo_private_info);

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

    LOG(LOG_DBG,"gbo->ion_fd=%d\n",gbo->ion_fd);
    LOG(LOG_DBG,"gbo->ion_metadata_fd=%d\n",gbo->ion_metadata_fd);

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

}


void get_yuv_ubwc_sp_plane_info(int width, int height,
                          int color_format, generic_buf_layout_t *buf_lyt)
{
   // UBWC buffer has these 4 planes in the following sequence:
   // Y_Meta_Plane, Y_Plane, UV_Meta_Plane, UV_Plane
   unsigned int y_meta_stride, y_meta_height, y_meta_size;
   unsigned int y_stride, y_height, y_size;
   unsigned int c_meta_stride, c_meta_height, c_meta_size;
   unsigned int alignment = 4096;

   y_meta_stride = VENUS_Y_META_STRIDE(color_format, width);
   y_meta_height = VENUS_Y_META_SCANLINES(color_format, height);
   y_meta_size = ALIGN((y_meta_stride * y_meta_height), alignment);

   y_stride = VENUS_Y_STRIDE(color_format, width);
   y_height = VENUS_Y_SCANLINES(color_format, height);
   y_size = ALIGN((y_stride * y_height), alignment);

   c_meta_stride = VENUS_UV_META_STRIDE(color_format, width);
   c_meta_height = VENUS_UV_META_SCANLINES(color_format, height);
   c_meta_size = ALIGN((c_meta_stride * c_meta_height), alignment);

   buf_lyt->num_planes = DUAL_PLANES;

   buf_lyt->planes[0].top_left = buf_lyt->planes[0].offset = y_meta_size;
   buf_lyt->planes[1].top_left = buf_lyt->planes[1].offset = y_meta_size + y_size + c_meta_size;
   buf_lyt->planes[2].top_left = buf_lyt->planes[2].offset = y_meta_size + y_size + c_meta_size + 1;
   buf_lyt->planes[0].v_increment = y_stride;
   buf_lyt->planes[1].v_increment = VENUS_UV_STRIDE(color_format, width);
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
             get_yuv_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                   YUV_420_SP_BPP, buf_lyt);
             break;
        case GBM_FORMAT_NV12:
             if (is_ubwc_enabled(gbo->format, gbo->usage_flags, gbo->usage_flags))
                get_yuv_ubwc_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                           COLOR_FMT_NV12_UBWC, buf_lyt);
             else
                get_yuv_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                      YUV_420_SP_BPP, buf_lyt);
             break;
        case GBM_FORMAT_YCbCr_420_TP10_UBWC:
             get_yuv_ubwc_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                        COLOR_FMT_NV12_BPP10_UBWC, buf_lyt);
             break;
        case GBM_FORMAT_P010:
            get_yuv_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                  CHROMA_STEP, buf_lyt);
            break;
        case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
            get_yuv_ubwc_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                       COLOR_FMT_NV12_UBWC, buf_lyt);
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
                 get_yuv_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                       YUV_420_SP_BPP, buf_lyt);
                 break;
            case GBM_FORMAT_YCbCr_420_TP10_UBWC:
                 get_yuv_ubwc_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                            COLOR_FMT_NV12_BPP10_UBWC, buf_lyt);
                 break;
            case GBM_FORMAT_P010:
                get_yuv_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                      CHROMA_STEP, buf_lyt);
                break;
            case GBM_FORMAT_YCbCr_420_SP_VENUS_UBWC:
                get_yuv_ubwc_sp_plane_info(gbo->aligned_width, gbo->aligned_height,
                                           COLOR_FMT_NV12_UBWC, buf_lyt);
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

    //sprintf(tmp_str, "%d", count++);
    snprintf(tmp_str, sizeof(tmp_str) ,"__%d_%d_%d_%d_%d_%lld", getpid(),ion_fd,width,height,format,time_usec);
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
