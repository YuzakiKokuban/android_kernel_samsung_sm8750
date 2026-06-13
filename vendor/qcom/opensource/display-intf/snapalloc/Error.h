// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAPALLOC_ERROR_H__
#define __SNAPALLOC_ERROR_H__

namespace vendor {
namespace qti {
namespace hardware {
namespace display {
namespace snapalloc {

enum Error {
  /**
     * No error.
     */
  NONE = 0,

  /**
     * Invalid BufferDescriptor.
     */
  BAD_DESCRIPTOR = 1,

  /**
     * Invalid Buffer Handle.
     */

  BAD_BUFFER = 2,

  /**
     * Invalid input.
     */
  BAD_VALUE = 3,

  /**
     * Resource unavailable.
     */
  NO_RESOURCES = 5,

  /**
     * Permanent failure.
     */
  UNSUPPORTED = 7,

  /**
     * Metadata hasn't been set
     */
  METADATA_NOT_SET = 9,

  /**
     * Buffer not freed
     */
  BUF_NOT_FREED = 10,
};

}  // namespace snapalloc
}  // namespace display
}  // namespace hardware
}  // namespace qti
}  // namespace vendor

#endif  // __SNAPALLOC_ERROR_H__
