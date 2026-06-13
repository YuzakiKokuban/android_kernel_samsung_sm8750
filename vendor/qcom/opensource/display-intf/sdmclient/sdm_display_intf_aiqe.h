/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SDM_DISPLAY_INTF_AIQE_H__
#define __SDM_DISPLAY_INTF_AIQE_H__
#include <core/sdm_types.h>

namespace sdm {

/**
 * The purpose of this interface is to set/get any AIQE configuration options
 * that can be changed within a display's lifetime.
 */

class SDMDisplayAiqeIntf {
public:
  virtual ~SDMDisplayAiqeIntf(){};

  /**
   * Set the ssrc mode on the given display
   *
   * @param display_id: The id of the specified display
   * @param mode_name: String form of the requested SSRC mode
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if the given SSRC mode is not valid
   * @exception: kErrorResources if the display doesn't support SSRC
   */
  virtual DisplayError SetSsrcMode(uint64_t display_id,
                                   const std::string &mode_name) = 0;

  /**
   * Set the COPR enablement state on the given display
   *
   * @param display_id: The id of the specified display
   * @param enable: Enablement state of COPR. True is ON, False is OFF
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorResources if the display doesn't support COPR
   */
  virtual DisplayError EnableCopr(uint64_t display_id, bool enable) = 0;

  /**
   * Retrieve the COPR stats on the given display
   *
   * @param display_id: The id of the specified display
   * @param copr_status: Vector to return the COPR stats into
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if an error occured when querying the COPR
   * status
   * @exception: kErrorResources if the display doesn't support COPR
   */
  virtual DisplayError GetCoprStats(uint64_t display_id,
                                     std::vector<int32_t> *copr_status) = 0;

  /**
   * Set the ABC state(enable/disable) on the given display
   *
   * @param display_id: The id of the specified display
   * @param state: State to set for ABC. 1 for enable, 0 for disable
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorResources if the display doesn't support ABC
   */
  virtual DisplayError SetABCState(uint64_t display_id, bool state) = 0;

  /**
   * Reconfig the ABC on the given display
   *
   * @param display_id: The id of the specified display
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorResources if the display doesn't support ABC
   */
  virtual DisplayError SetABCReconfig(uint64_t display_id) = 0;

  /**
   * Set the ABC with given mode on the given display
   *
   * @param display_id: The id of the specified display
   * @param mode_name: Mode name to be set to ABC
   *
   * @return: kErrorNone if transaction succeeded
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorResources if the display doesn't support ABC
   */
  virtual DisplayError SetABCMode(uint64_t display_id,
                                  std::string mode_name) = 0;
};

} // namespace sdm

#endif // __SDM_DISPLAY_INTF_AIQE_H__
