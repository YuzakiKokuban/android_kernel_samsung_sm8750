/*
 * Copyright (c) 2012-2018, 2020 The Linux Foundation. All rights reserved.
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef WLAN_PMO_CFG_H__
#define WLAN_PMO_CFG_H__

#include "wlan_pmo_apf_cfg.h"
#include "wlan_pmo_common_cfg.h"
#include "wlan_pmo_extwow_cfg.h"
#include "wlan_pmo_pkt_filter_cfg.h"
#include "wlan_pmo_runtime_pm_cfg.h"
#include "wlan_pmo_wow_pulse_cfg.h"
#include "wlan_pmo_gpio_wakeup_cfg.h"

/*
 * <ini>
 * gpmo_fw_debug_enable - wow enable fw debug
 * @Min: 0
 * @Max: 1
 * @Default: 0
 *
 * This INI will enable FW debug for suspend/resume mode.
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_PMO_FW_DEBUG_ENABLE CFG_INI_UINT("gpmo_fw_debug_enable", \
						0, 1, 0, \
						CFG_VALUE_OR_DEFAULT, \
						"Enable FW logging debug")

#define CFG_PMO_ALL \
	CFG_EXTWOW_ALL \
	CFG_PACKET_FILTER_ALL \
	CFG_PMO_APF_ALL \
	CFG_PMO_COMMON_ALL \
	CFG_RUNTIME_PM_ALL \
	CFG_WOW_PULSE_ALL \
	CFG_GPIO_WAKEUP_ALL \
	CFG(CFG_PMO_FW_DEBUG_ENABLE)

#endif /* WLAN_PMO_CFG_H__ */
