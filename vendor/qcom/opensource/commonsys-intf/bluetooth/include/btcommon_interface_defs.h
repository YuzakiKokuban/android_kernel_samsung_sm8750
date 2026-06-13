/*
 * Copyright (c) 2019 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *        * Redistributions of source code must retain the above copyright
 *            notice, this list of conditions and the following disclaimer.
 *        * Redistributions in binary form must reproduce the above
 *            copyright notice, this list of conditions and the following
 *            disclaimer in the documentation and/or other materials provided
 *            with the distribution.
 *        * Neither the name of The Linux Foundation nor the names of its
 *            contributors may be used to endorse or promote products derived
 *            from this software without specific prior written permission.
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

#ifndef BTCOMMON_INTERFACE_DEFS_H
#define BTCOMMON_INTERFACE_DEFS_H

#pragma once

typedef enum {
  BT_SOC_TYPE_DEFAULT = 0x0000,
  BT_SOC_TYPE_SMD = BT_SOC_TYPE_DEFAULT,
  BT_SOC_TYPE_AR3K,
  BT_SOC_TYPE_ROME,
  BT_SOC_TYPE_CHEROKEE,
  BT_SOC_TYPE_HASTINGS,
  BT_SOC_TYPE_MOSELLE,
  BT_SOC_TYPE_HAMILTON,
  BT_SOC_TYPE_GANGES,
  /* Add chipset type here */
  BT_SOC_TYPE_RESERVED
} bt_soc_type_t;

typedef struct {
  uint16_t product_id;
  uint16_t rsp_version;
  uint8_t feat_mask_len;
  uint8_t features[8];
} add_on_features_list_t;

enum {
  BT_PROP_ALL = 0x0000,
  BT_PROP_SOC_TYPE,
  BT_PROP_A2DP_OFFLOAD_CAP,
  BT_PROP_SPILT_A2DP,
  BT_PROP_AAC_FRAME_CTL,
  BT_PROP_WIPOWER,
  BT_PROP_SWB_ENABLE,
  BT_PROP_SWBPM_ENABLE,
  BT_PROP_A2DP_MCAST_TEST,
  BT_PROP_TWSP_STATE,
  BT_PROP_STACK_TIMEOUT,
  BT_PROP_MAX_POWER,

  // FM PROPERTY
  FM_PROP_NOTCH_VALUE = 0x1000,
  FM_PROP_HW_INIT,
  FM_PROP_HW_MODE,
  FM_PROP_CTL_START,
  FM_PROP_CTL_STOP,
  FM_STATS_PROP,
  FM_PROP_WAN_RATCONF,
  FM_PROP_BTWLAN_LPFENABLER,
};

#endif
