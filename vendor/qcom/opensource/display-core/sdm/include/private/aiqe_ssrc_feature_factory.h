/*
* Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef __AIQE_SSRC_FEATURE_FACTORY_H__
#define __AIQE_SSRC_FEATURE_FACTORY_H__

#include <private/aiqe_ssrc_feature_interface.h>
#include <private/panel_feature_property_intf.h>

#include <memory>

namespace aiqe {

#define SSRC_LIBRARY_NAME "libssrc.so"
#define GET_SSRC_FF_INTF_NAME "GetSsrcFeatureFactory"

class SsrcFeatureFactory {
 public:
  SsrcFeatureFactory(){};
  virtual ~SsrcFeatureFactory(){};
  virtual std::shared_ptr<SsrcFeatureInterface> GetSsrcFeatureInterface(
      sdm::PanelFeaturePropertyIntf *pfp_intf) = 0;
};

typedef SsrcFeatureFactory *(*GetSsrcFeatureFactoryFp)();
extern "C" SsrcFeatureFactory *GetSsrcFeatureFactory();
}  // end of namespace aiqe

#endif  // __AIQE_SSRC_FEATURE_FACTORY_H__