/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __CB_INTF_H__
#define __CB_INTF_H__

namespace sdm {

template <typename X> class SdmDisplayCbInterface {
public:
  virtual int Notify(const X &) = 0;
  virtual ~SdmDisplayCbInterface() {}
};

} // namespace sdm
#endif // __CB_INTF_H__
