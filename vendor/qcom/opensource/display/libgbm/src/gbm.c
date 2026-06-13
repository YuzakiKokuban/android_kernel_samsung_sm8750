/*
* Copyright (c) 2017, 2021 The Linux Foundation. All rights reserved.
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
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <stdio.h>
#include <stdint.h>
#include <gbm.h>
#include <gbm_priv.h>
#include <errno.h>

/** Returns the file description for the gbm device
 *
 * \return The fd that the struct gbm_device was created with
 */
int
gbm_device_get_fd(struct gbm_device *gbm_dev)
{
    if(gbm_dev!=NULL){
        return gbm_dev->fd;
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid device pointer\n",__func__,__LINE__);
        return -1;
    }
}

/** Get the backend name for the given gbm device
 *
 * \return The backend name string - this belongs to the device and must not
 * be freed
 */
const char *
gbm_device_get_backend_name(struct gbm_device *gbm_dev)
{
    if(gbm_dev!=NULL){
        return gbm_dev->name;
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid device pointer\n",__func__,__LINE__);
        return NULL;
    }
}

/** Test if a format is supported for a given set of usage flags.
 *
 * \param gbm The created buffer manager
 * \param format The format to test
 * \param usage A bitmask of the usages to test the format against
 * \return 1 if the format is supported otherwise 0
 *
 * \sa enum gbm_bo_flags for the list of flags that the format can be
 * tested against
 *
 * \sa enum gbm_bo_format for the list of formats
 */
int
gbm_device_is_format_supported(struct gbm_device *gbm_dev,
                               unsigned int  format, unsigned int  usage)
{
    if(gbm_dev!=NULL){
        return gbm_dev->is_format_supported(gbm_dev, format,usage);
    }
   else {
      fprintf(stderr,"%s(%d): NULL or Invalid device pointer\n",__func__,__LINE__);
      return -1;
   }
}

int
gbm_device_get_format_modifier_plane_count(struct gbm_device *gbm,
                                           uint32_t format,
                                           uint64_t modifier)
{
  return gbm->get_format_modifier_plane_count(format, modifier);
}

/** Destroy the gbm device and free all resources associated with it.
 *
 * \param gbm The device created using gbm_create_device()
 */
void
gbm_device_destroy(struct gbm_device *gbm_dev)
{
    if(gbm_dev!=NULL)
   {
        gbm_dev->refcount--;
        if (gbm_dev->refcount == 0) {
            return gbm_dev->destroy(gbm_dev);
        }
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid device pointer\n",__func__,__LINE__);
        return ;
    }
}

/** Create a gbm device for allocating buffers
 *
 * The file descriptor passed in is used by the backend to communicate with
 * platform for allocating the memory. For allocations using DRI this would be
 * the file descriptor returned when opening a device such as \c
 * /dev/dri/card0
 *
 * \param fd The file descriptor for an backend specific device
 * \return The newly created struct gbm_device. The resources associated with
 * the device should be freed with gbm_device_destroy() when it is no longer
 * needed. If the creation of the device failed NULL will be returned.
 */
struct gbm_device *
gbm_create_device(int fd)
{
   struct gbm_backendpriv *backend =NULL;
   struct gbm_device *gbm_dev =NULL;
   backend = gbm_get_priv();

   if(backend == NULL) {
       fprintf(stderr,"%s(%d): No backend found, Device creation failed\n",__func__,__LINE__);
      return NULL;
   }

   if(fd <= 0){
       fprintf(stderr,"%s(%d): Inavid file descriptor (%d)\n",__func__,__LINE__,fd);
   }

   gbm_dev = backend->create_device(fd);
   if(gbm_dev == NULL)
   {
       fprintf(stderr,"%s(%d): Device creation failed\n",__func__,__LINE__);
       return NULL;
   }

   gbm_dev->refcount = 1;
   gbm_dev->fd = fd;

   if(backend->backend_name != NULL){
       gbm_dev->name = backend->backend_name;
       fprintf(stderr,"%s(%d): Info: backend name is: %s\n",__func__,__LINE__,gbm_dev->name);
   }
   else
   {
      gbm_dev->name = NULL;
      fprintf(stderr,"%s(%d): Info: backend name is null\n",__func__,__LINE__);
   }

   return gbm_dev;
}

/** Get the width of the buffer object
 *
 * \param bo The buffer object
 * \return The width of the allocated buffer object
 *
 */
unsigned int
gbm_bo_get_width(struct gbm_bo *bo)
{
    if(bo!=NULL){
        return bo->width;
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
        return 0;
    }
}

/** Get the height of the buffer object
 *
 * \param bo The buffer object
 * \return The height of the allocated buffer object
 */
unsigned int
gbm_bo_get_height(struct gbm_bo *bo)
{
    if(bo!=NULL){
        return bo->height;
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
        return 0;
    }
}

/** Get the stride of the buffer object
 *
 * This is calculated by the backend when it does the allocation in
 * gbm_bo_create()
 *
 * \param bo The buffer object
 * \return The stride of the allocated buffer object in bytes
 */
unsigned int
gbm_bo_get_stride(struct gbm_bo *bo)
{
    if(bo!=NULL){
        if(bo->usage_flags & GBM_BO_USAGE_UBWC_ALIGNED_QTI){
            return bo->stride_for_plane(1, bo);
        } else {
            return bo->stride_for_plane(0, bo);
        }
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
        return 0;
    }
}

/** Get the stride for the given plane
 *
 * \param bo The buffer object
 * \param plane for which you want the stride
 *
 * \sa gbm_bo_get_stride()
 */
uint32_t
gbm_bo_get_stride_for_plane(struct gbm_bo *bo, int plane)
{
  if (!bo){
    fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
    return 0;
  }
  if (plane >= MAX_NUM_OF_PLANES) {
    fprintf(stderr,"%s(%d): invalid plane \n",__func__,__LINE__);
    return 0;
  }

  return bo->stride_for_plane(plane, bo);
}

/** Get the format of the buffer object
 *
 * The format of the pixels in the buffer.
 *
 * \param bo The buffer object
 * \return The format of buffer object, on of the GBM_FORMAT_* codes
 */
unsigned int
gbm_bo_get_format(struct gbm_bo *bo)
{
    if(bo!=NULL){
        return bo->format;
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
        return 0;
    }
}

uint32_t
gbm_bo_get_bpp(struct gbm_bo *bo)
{
  if(bo!=NULL){
        return bo->bpp;
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
        return 0;
    }
}

uint32_t
gbm_bo_get_offset(struct gbm_bo *bo, int plane)
{
  if(bo!=NULL){
         if (plane <= MAX_NUM_OF_PLANES-1) {
            return bo->buf_lyt.planes[plane].offset;
         } else {
            fprintf(stderr,"%s(%d): invalid plane \n",__func__,__LINE__);
            return 0;
         }
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
        return 0;
    }
}

/** Get the handle of the buffer object
 *
 * This is stored in the platform generic union gbm_bo_handle type. However
 * the format of this handle is platform specific.
 *
 * \param bo The buffer object
 * \return Returns the handle of the allocated buffer object
 */
union gbm_bo_handle
gbm_bo_get_handle(struct gbm_bo *bo)
{
        return bo->handle;
}

/** Returns the file description of the buffer object
 *
 * \param bo The buffer object
 * \return Returns a file descriptor referring	to the underlying buffer
 */
int
gbm_bo_get_fd(struct gbm_bo *bo)
{
    if(bo!=NULL){
        return bo->bo_get_fd(bo);
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
        return 0;
    }
}

uint64_t
gbm_bo_get_modifier(struct gbm_bo *bo)
{
      if(bo!=NULL){
        return bo->modifier;
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
        return 0;
    }
}

int
gbm_bo_get_plane_count(struct gbm_bo *bo)
{
    if(bo!=NULL){
        return (int)(bo->plane_count);
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
        return 0;
    }
}

union gbm_bo_handle
gbm_bo_get_handle_for_plane(struct gbm_bo *bo, int plane)
{
  union gbm_bo_handle handle;
  if(bo!=NULL){
        if (plane <= MAX_NUM_OF_PLANES) {
          return handle;
        } else {
          return (handle);
        }
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
        return handle;
    }
}

/** Write data into the buffer object
 *
 * If the buffer object was created with the GBM_BO_USE_WRITE flag,
 * this function can used to write data into the buffer object.  The
 * data is copied directly into the object and it's the responsiblity
 * of the caller to make sure the data represents valid pixel data,
 * according to the width, height, stride and format of the buffer object.
 *
 * \param bo The buffer object
 * \param buf The data to write
 * \param count The number of bytes to write
 * \return Returns -1 on error, 0 otherwise
 */
int
gbm_bo_write(struct gbm_bo *bo, const void *buf, size_t count)
{
    if(bo!=NULL){
        return bo->bo_write(bo,buf,count);
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
        return 0;
    }
}

/** Get the gbm device used to create the buffer object
 *
 * \param bo The buffer object
 * \return Returns the gbm device with which the buffer object was created
 */
struct gbm_device *
gbm_bo_get_device(struct gbm_bo *bo)
{
    if(bo!=NULL){
        return bo->bo_get_device(bo);
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
        return 0;
    }
}

/** Set the user data associated with a buffer object
 *
 * \param bo The buffer object
 * \param data The data to associate to the buffer object
 * \param destroy_user_data A callback (which may be %NULL) that will be
 * called prior to the buffer destruction
 */
void
gbm_bo_set_user_data(struct gbm_bo *bo, void *data,
                                void (*destroy_user_data)(struct gbm_bo *, void *))
{
    if(bo!=NULL) {
        bo->user_data = data;
        bo->destroy_user_data = destroy_user_data;
    }
   else {
       fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
   }
   return;
}

/** Get the user data associated with a buffer object
 *
 * \param bo The buffer object
 * \return Returns the user data associated with the buffer object or %NULL
 * if no data was associated with it
 *
 * \sa gbm_bo_set_user_data()
 */
void *
gbm_bo_get_user_data(struct gbm_bo *bo)
{
    if(bo!=NULL){
        return bo->user_data;
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
        return NULL;
    }
}

/**
 * Destroys the given buffer object and frees all resources associated with
 * it.
 *
 * \param bo The buffer object
 */
void
gbm_bo_destroy(struct gbm_bo *bo)
{
    if(bo!=NULL){
        if (bo->destroy_user_data) {
            bo->destroy_user_data(bo, bo->user_data);
        }

       bo->bo_destroy(bo);
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid bo pointer\n",__func__,__LINE__);
    }
    return;
}

/**
 * Allocate a buffer object for the given dimensions
 *
 * \param gbm The gbm device returned from gbm_create_device()
 * \param width The width for the buffer
 * \param height The height for the buffer
 * \param format The format to use for the buffer
 * \param usage The union of the usage flags for this buffer
 *
 * \return A newly allocated buffer that should be freed with gbm_bo_destroy()
 * when no longer needed. If an error occurs during allocation %NULL will be
 * returned.
 *
 * \sa enum gbm_bo_format for the list of formats
 * \sa enum gbm_bo_flags for the list of usage flags
 */
struct gbm_bo *
gbm_bo_create(struct gbm_device *gbm_dev,
              uint32_t width, uint32_t height,
              uint32_t format, uint32_t usage)
{
    if (width == 0 || height == 0){
        errno = EINVAL;
        fprintf(stderr,"%s(%d): width(%d) or height(%d) value\n",__func__,__LINE__,width,height);
        return NULL;
    }

    if(gbm_dev!=NULL){
        return gbm_dev->bo_create(gbm_dev,width,height,format,usage, NULL, 0);
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid device pointer\n",__func__,__LINE__);
        return NULL;
    }
}


struct gbm_bo *
gbm_bo_create_with_modifiers(struct gbm_device *gbm,
                             uint32_t width, uint32_t height,
                             uint32_t format,
                             const uint64_t *modifiers,
                             const unsigned int count)
{
  if (width == 0 || height == 0) {
      errno = EINVAL;
      return NULL;
   }

   if ((count && !modifiers) || (modifiers && !count)) {
      errno = EINVAL;
      return NULL;
   }
    if(gbm!=NULL){
        return gbm->bo_create(gbm,width,height,format, 0, modifiers, count);
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid device pointer\n",__func__,__LINE__);
        return NULL;
    }

}

/**
 * Create a gbm buffer object from an foreign object
 *
 * This function imports a foreign object and creates a new gbm bo for it.
 * This enabled using the foreign object with a display API such as KMS.
 * Currently two types of foreign objects are supported, indicated by the type
 * argument:
 *
 *   GBM_BO_IMPORT_WL_BUFFER
 *   GBM_BO_IMPORT_EGL_IMAGE
 *
 * The the gbm bo shares the underlying pixels but its life-time is
 * independent of the foreign object.
 *
 * \param gbm The gbm device returned from gbm_create_device()
 * \param gbm The type of object we're importing
 * \param gbm Pointer to the external object
 * \param usage The union of the usage flags for this buffer
 *
 * \return A newly allocated buffer object that should be freed with
 * gbm_bo_destroy() when no longer needed.
 *
 * \sa enum gbm_bo_flags for the list of usage flags
 */
struct gbm_bo *
gbm_bo_import(struct gbm_device *gbm_dev,
              uint32_t type, void *buffer, uint32_t usage)
{
    if(gbm_dev!=NULL){
        return gbm_dev->bo_import(gbm_dev,type,buffer,usage);
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid device pointer\n",__func__,__LINE__);
        return NULL;
    }
}

void *
gbm_bo_map(struct gbm_bo *bo,
           uint32_t x, uint32_t y, uint32_t width, uint32_t height,
           uint32_t flags, uint32_t *stride, void **map_data){

  bo->bo_map(x, y, width, height, flags, stride, map_data);
}

void
gbm_bo_unmap(struct gbm_bo *bo, void *map_data)
{
  if(bo)
  {
    bo->bo_unmap(map_data);
  }
}

/**
 * Allocate a surface object
 *
 * \param gbm The gbm device returned from gbm_create_device()
 * \param width The width for the surface
 * \param height The height for the surface
 * \param format The format to use for the surface
 *
 * \return A newly allocated surface that should be freed with
 * gbm_surface_destroy() when no longer needed. If an error occurs
 * during allocation %NULL will be returned.
 *
 * \sa enum gbm_bo_format for the list of formats
 */
struct gbm_surface *
gbm_surface_create(struct gbm_device *gbm_dev,
                   uint32_t width, uint32_t height,
                   uint32_t format, uint32_t flags)
{
    if(gbm_dev!=NULL){
        return gbm_dev->surface_create(gbm_dev,width,height,format,flags, NULL, 0);
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid device pointer\n",__func__,__LINE__);
        return NULL;
    }
}

struct gbm_surface *
gbm_surface_create_with_modifiers(struct gbm_device *gbm,
                                  uint32_t width, uint32_t height,
                                  uint32_t format,
                                  const uint64_t *modifiers,
                                  const unsigned int count)
{
   if ((count && !modifiers) || (modifiers && !count)) {
      errno = EINVAL;
      return NULL;
   }

   return gbm->surface_create(gbm, width, height, format, 0,modifiers,count);
}

/**
 * Destroys the given surface and frees all resources associated with
 * it.
 *
 * All buffers locked with gbm_surface_lock_front_buffer() should be
 * released prior to calling this function.
 *
 * \param surf The surface
 */
void
gbm_surface_destroy(struct gbm_surface *gbm_surf)
{
    if(gbm_surf!=NULL){
        gbm_surf->surface_destroy(gbm_surf);
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid surface pointer\n",__func__,__LINE__);
    }
    return;
}

/**
 * Lock the surface's current front buffer
 *
 * Lock rendering to the surface's current front buffer until it is
 * released with gbm_surface_release_buffer().
 *
 * This function must be called exactly once after calling
 * eglSwapBuffers.  Calling it before any eglSwapBuffer has happened
 * on the surface or two or more times after eglSwapBuffers is an
 * error.  A new bo representing the new front buffer is returned.  On
 * multiple invocations, all the returned bos must be released in
 * order to release the actual surface buffer.
 *
 * \param surf The surface
 *
 * \return A buffer object that should be released with
 * gbm_surface_release_buffer() when no longer needed.  The implementation
 * is free to reuse buffers released with gbm_surface_release_buffer() so
 * this bo should not be destroyed using gbm_bo_destroy().  If an error
 * occurs this function returns %NULL.
 */
struct gbm_bo *
gbm_surface_lock_front_buffer(struct gbm_surface *gbm_surf)
{
    if(gbm_surf!=NULL){
        return gbm_surf->surface_lock_front_buffer(gbm_surf);
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid surface pointer\n",__func__,__LINE__);
        return NULL;
    }
}

/**
 * Release a locked buffer obtained with gbm_surface_lock_front_buffer()
 *
 * Returns the underlying buffer to the gbm surface.  Releasing a bo
 * will typically make gbm_surface_has_free_buffer() return 1 and thus
 * allow rendering the next frame, but not always. The implementation
 * may choose to destroy the bo immediately or reuse it, in which case
 * the user data associated with it is unchanged.
 *
 * \param surf The surface
 * \param bo The buffer object
 */
void
gbm_surface_release_buffer(struct gbm_surface *gbm_surf, struct gbm_bo *bo)
{
    if(gbm_surf != NULL && bo != NULL){
        gbm_surf->surface_release_buffer(gbm_surf, bo);
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid surface pointer\n",__func__,__LINE__);
    }
    return;
}

/**
 * Return whether or not a surface has free (non-locked) buffers
 *
 * Before starting a new frame, the surface must have a buffer
 * available for rendering.  Initially, a gbm surface will have a free
 * buffer, but after one of more buffers have been locked (\sa
 * gbm_surface_lock_front_buffer()), the application must check for a
 * free buffer before rendering.
 *
 * If a surface doesn't have a free buffer, the application must
 * return a buffer to the surface using gbm_surface_release_buffer()
 * and after that, the application can query for free buffers again.
 *
 * \param surf The surface
 * \return 1 if the surface has free buffers, 0 otherwise
 */
int
gbm_surface_has_free_buffers(struct gbm_surface *gbm_surf)
{
    if(gbm_surf != NULL){
        return gbm_surf->surface_has_free_buffers(gbm_surf);
    }
    else {
        fprintf(stderr,"%s(%d): NULL or Invalid surface pointer\n",__func__,__LINE__);
        return 0;
    }
}

/**
 * Returns a string representing the fourcc format name.
 *
 * \param desc Caller-provided storage for the format name string.
 * \return String containing the fourcc of the format.
 */
char *
gbm_format_get_name(uint32_t gbm_format, struct gbm_format_name_desc *desc)
{
   desc->name[0] = gbm_format;
   desc->name[1] = gbm_format >> 8;
   desc->name[2] = gbm_format >> 16;
   desc->name[3] = gbm_format >> 24;
   desc->name[4] = 0;

   return desc->name;
}
