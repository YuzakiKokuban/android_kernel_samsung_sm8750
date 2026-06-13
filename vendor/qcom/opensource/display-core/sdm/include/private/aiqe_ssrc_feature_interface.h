/*
* Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef __AIQE_SSRC_FEATURE_INTERFACE_H__
#define __AIQE_SSRC_FEATURE_INTERFACE_H__

#include <private/generic_intf.h>
#include <private/generic_payload.h>

#include <cstdint>
#include <string>

namespace aiqe {

enum SsrcFeatureParam {
  /*! \brief Property to provide information about the active display to the SSRC feature interface.
   * Used with SetParameter() to notify the SSRC feature interface on details of the panel currently
   * being controlled.
   *
   * The payload provided shall a single SsrcFeatureDisplayDetails structure.
   */
  kSsrcFeatureDisplayDetails,
  /*! \brief Property to declare the target SSRC configuration mode.
   * Used with SetParameter() to update the SSRC configuration mode that should be targeted next
   * commit cycle.
   *
   * The payload provided shall contain a string holding the name of the configuration mode.
   */
  kSsrcFeatureModeId,
  /*! \brief Property to trigger a SSRC commit update.
   * Used with SetParameter() to command the SSRC feature interface to commit any feature configuration
   * changes to the hardware.
   *
   * The payload provided shall contain a boolean value. When set to true, feature configuration data
   * will be sent to the hardware regardless of the SSRC feature interfaces normal update logic.
   */
  kSsrcFeatureCommitFeature,
  kSsrcFeatureParamMax,
};

enum SsrcFeatureOp {
  SsrcFeatureOpMax,
};

struct SsrcFeatureDisplayDetails {
  std::string panel_name;
  uint32_t ppc;
  bool primary_panel;
};

typedef sdm::GenericIntf<SsrcFeatureParam, SsrcFeatureOp, sdm::GenericPayload> SsrcFeatureInterface;

}  //End of namespace aiqe
#endif  // __AIQE_SSRC_FEATURE_INTERFACE_H__