/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __DISPLAY_EVENT_PROXY_INTF_H__
#define __DISPLAY_EVENT_PROXY_INTF_H__

#include <array>
#include <memory>
#include <private/cb_intf.h>
#include <private/generic_intf.h>
#include <private/generic_payload.h>
#include <private/panel_feature_property_intf.h>
#include <string>

namespace sdm {

class DisplayInterface;

enum SdmDisplayEvents {
  kSdmOprEvent,  // OPR register value
  kSdmPaHistEvent,
  kSdmCoprEvent,  // COPR statistics
  kSdmDisplayEventsMax = 0xff
};

enum DispEventProxyParams {
  // Setter
  kSetPanelOprInfoEnable,
  kSetPaHistCollection,
  kSetPanelBLInfoEnable,
  kSetCoprEnable,

  // Getter
  kGetPaHistBins,

  kDispEventProxyParamMax = 0xff,
};

enum DispEventProxyOps {
  kDispEventProxyOpsMax,
};

struct PanelOprPayload {
  uint32_t version = sizeof(PanelOprPayload);
  uint64_t flags;
  uint32_t opr_val;
};

struct PanelOprInfoParam {
  std::string name;
  bool enable;
  SdmDisplayCbInterface<PanelOprPayload> *cb_intf = nullptr;
};

struct PaHistCollectionPayload {
  uint32_t version = sizeof(PaHistCollectionPayload);
  SdmDisplayEvents event;
};

#define HIST_BIN_SIZE 256
struct PaHistBinsParam {
  std::array<uint32_t, HIST_BIN_SIZE> *buf;
};

struct PaHistCollectionParam {
  std::string name;
  bool enable;
  SdmDisplayCbInterface<PaHistCollectionPayload> *cb_intf = nullptr;
};

struct PanelBacklightPayload {
  uint32_t version = sizeof(PanelBacklightPayload);
  uint64_t flags;
  uint32_t brightness;
};

struct PanelBacklightInfoParam {
  std::string name;
  bool enable;
  SdmDisplayCbInterface<PanelBacklightPayload> *cb_intf = nullptr;
};

struct CoprEventPayload {
  uint32_t version = sizeof(CoprEventPayload);
  uint32_t payload_size;
  void *payload;
};

struct CoprParam {
  std::string name;
  bool enable;
  SdmDisplayCbInterface<CoprEventPayload> *cb_intf = nullptr;
};

using DisplayEventProxyIntf =
    GenericIntf<DispEventProxyParams, DispEventProxyOps, GenericPayload>;

class DispEventProxyFactIntf {
public:
  virtual ~DispEventProxyFactIntf() {}
  virtual std::shared_ptr<DisplayEventProxyIntf> CreateDispEventProxyIntf(
      const std::string &panel_name, DisplayInterface *intf,
      PanelFeaturePropertyIntf *prop_intf) = 0;
};

extern "C" DispEventProxyFactIntf *GetDispEventProxyFactIntf();

} // namespace sdm
#endif // __DISPLAY_EVENT_PROXY_INTF_H__
