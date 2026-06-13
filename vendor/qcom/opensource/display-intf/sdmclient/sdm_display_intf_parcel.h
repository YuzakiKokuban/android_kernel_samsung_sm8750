/*
 * Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 *
 * Copyright 2015 The Android Open Source Project
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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SDM_PARCEL_INTF_H___
#define __SDM_PARCEL_INTF_H___

namespace sdm {

class SDMParcel {
public:
  virtual ~SDMParcel() {}
  virtual uint32_t readInt32() = 0;
  virtual uint64_t readInt64() = 0;
  virtual float readFloat() = 0;
  virtual void writeInt32(uint32_t value) = 0;
  virtual uint32_t dataSize() = 0;
  virtual uint32_t writeFloat(float val) = 0;
  virtual uint32_t dataPosition() = 0;
  virtual uint32_t writeUint64(uint64_t value) = 0;
  virtual uint32_t dataAvail() = 0;
  virtual const void *readInplace(uint32_t size) = 0;
  virtual uint32_t write(const void *data, uint32_t len) = 0;
  virtual uint32_t writeDupFileDescriptor(int fd) = 0;
  virtual double readDouble() = 0;
};

} // namespace sdm

#endif // __SDM_PARCEL_INTF_H__
