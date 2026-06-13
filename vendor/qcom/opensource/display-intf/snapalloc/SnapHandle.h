/*
 * Copyright (C) 2009 The Android Open Source Project
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
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SNAPALLOC_SNAPHANDLE_H__
#define __SNAPALLOC_SNAPHANDLE_H__

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

namespace vendor {
namespace qti {
namespace hardware {
namespace display {
namespace snapalloc {

#define SNAP_HANDLE_MAX_FDS 1024
#define SNAP_HANDLE_MAX_INTS 1024

class SnapHandle {
 public:
  ~SnapHandle(){};
  SnapHandle(){};

  int version;
  int num_fds;  /* number of file descriptors at &buffer_data[0] */
  int num_ints; /* number of ints at &buffer_data[num_fds] */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"
#endif
  int buffer_data[0]; /* num_fds + num_ints ints */
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
};

inline SnapHandle *snap_handle_create(int num_fds, int num_ints) {
  if (num_fds < 0 || num_ints < 0 || num_fds > SNAP_HANDLE_MAX_FDS || num_ints > SNAP_HANDLE_MAX_INTS) {
    errno = EINVAL;
    return NULL;
  }
  size_t handle_size = sizeof(SnapHandle) + (sizeof(int) * (num_fds + num_ints));
  SnapHandle *h = static_cast<SnapHandle *>(malloc(handle_size));
  if (h) {
    h->version = sizeof(SnapHandle);
    h->num_fds = num_fds;
    h->num_ints = num_ints;
  }
  return h;
}

inline int snap_handle_delete(SnapHandle *h) {
  if (h) {
    if (h->version != sizeof(SnapHandle))
      return -EINVAL;
    free(h);
  }
  return 0;
}

}  // namespace snapalloc
}  // namespace display
}  // namespace hardware
}  // namespace qti
}  // namespace vendor

#endif  // __SNAPALLOC_SNAPHANDLE_H__
