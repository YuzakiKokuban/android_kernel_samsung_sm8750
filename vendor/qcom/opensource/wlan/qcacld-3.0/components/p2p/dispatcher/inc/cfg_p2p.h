/*
 * Copyright (c) 2018-2020 The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#if !defined(CONFIG_P2P_H__)
#define CONFIG_P2P_H__

#include "cfg_define.h"
#include "cfg_converged.h"
#include "qdf_types.h"

/*
 * <ini>
 * gGoKeepAlivePeriod - P2P GO keep alive period.
 * @Min: 1
 * @Max: 65535
 * @Default: 20
 *
 * This is P2P GO keep alive period.
 *
 * Related: None.
 *
 * Supported Feature: P2P
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_GO_KEEP_ALIVE_PERIOD CFG_INI_UINT( \
	"gGoKeepAlivePeriod", \
	1, \
	65535, \
	20, \
	CFG_VALUE_OR_DEFAULT, \
	"P2P GO keep alive period")

/*
 * <ini>
 * gGoLinkMonitorPeriod - period where link is idle and where
 * we send NULL frame
 * @Min: 3
 * @Max: 50
 * @Default: 10
 *
 * This is period where link is idle and where we send NULL frame for P2P GO.
 *
 * Related: None.
 *
 * Supported Feature: P2P
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_GO_LINK_MONITOR_PERIOD CFG_INI_UINT( \
	"gGoLinkMonitorPeriod", \
	3, \
	50, \
	10, \
	CFG_VALUE_OR_DEFAULT, \
	"period where link is idle and where we send NULL frame")

/*
 * <ini>
 * isP2pDeviceAddrAdministrated - Enables to derive the P2P MAC address from
 * the primary MAC address
 * @Min: 0
 * @Max: 1
 * @Default: 1
 *
 * This ini is used to enable/disable to derive the P2P MAC address from the
 * primary MAC address.
 *
 * Related: None.
 *
 * Supported Feature: P2P
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_P2P_DEVICE_ADDRESS_ADMINISTRATED CFG_INI_BOOL( \
	"isP2pDeviceAddrAdministrated", \
	1, \
	"derive the P2P MAC address from the primary MAC address")

/*
 * <ini>
 * action_frame_random_seq_num_enabled - Enables random sequence number
 *                                       generation for action frames
 * @Min: 0
 * @Max: 1
 * @Default: 1
 *
 * This ini is used to enable/disable random sequence number generation for
 * action frames.
 *
 * Related: None.
 *
 * Supported Feature: P2P
 *
 * Usage: external
 *
 * </ini>
 */
#define CFG_ACTION_FRAME_RANDOM_SEQ_NUM_ENABLED CFG_INI_BOOL( \
	"action_frame_random_seq_num_enabled", \
	1, \
	"Enable random seq nums for action frames")

/*
 * <ini>
 * p2p_go_on_5ghz_indoor_chan - Allow P2P GO to operate on 5 GHz indoor channels
 *
 * @Min: 0
 * @Max: 1
 * @Default: 0
 *
 * This ini is used to enable/disable P2P-GO operation on 5 GHz
 * indoor channels.
 *
 * Related: sta_sap_scc_on_indoor_channel, gindoor_channel_support
 *
 * Supported Feature: P2P GO
 *
 * Usage: external
 *
 * </ini>
 */
#define CFG_P2P_GO_ON_5GHZ_INDOOR_CHANNEL CFG_INI_BOOL(\
					"p2p_go_on_5ghz_indoor_chan", \
					0, \
					"Allow P2P GO on 5 GHz indoor channels")

/*
 * <ini>
 * p2p_go_ignore_non_p2p_probe_req - P2P GO ignore non-P2P probe req
 *
 * @Min: 0
 * @Max: 1
 * @Default: 0
 *
 * This ini is used to enable/disable P2P GO ignore non-P2P probe req and don't
 * send probe rsp to non-p2p device like STA.
 *
 *
 * Supported Feature: P2P GO
 *
 * Usage: external
 *
 * </ini>
 */
#define CFG_GO_IGNORE_NON_P2P_PROBE_REQ CFG_INI_BOOL(\
					"go_ignore_non_p2p_probe_req", \
					0, \
					"P2P GO ignore non-P2P probe req")

/*
 * <ini>
 * sta_vdev_for_p2p_device - Use sta vdev for p2p device operation
 *
 * @Min: 0
 * @Max: 1
 * @Default: 0
 *
 * This ini is used to use sta vdev for p2p device operation
 */
#define CFG_USE_STA_VDEV_FOR_P2P_DEVICE CFG_INI_BOOL(\
					"sta_vdev_for_p2p_device", \
					false, \
					"use sta vdev for p2p device")

/*
 * <ini>
 * sta_vdev_for_p2p_device_upon_vdev_exhaust - Use sta vdev for p2p device
 *
 * @Min: 0
 * @Max: 1
 * @Default: 0
 *
 * This ini is used to use sta vdev for p2p device operation when number
 * of created vdev has already reached to max limit but still new interface
 * tries to comes up. To accommodate the new interface, host will check
 * if any p2p device mode is present, if it's present then host will
 * destroy the p2p device vdev and redirect all p2p device operation
 * to sta vdev.
 * If "sta_vdev_for_p2p_device" is enable then this ini gets disabled.
 */
#define STA_VDEV_FOR_P2P_DEVICE_UPON_VDEV_EXHAUST CFG_INI_BOOL(\
			"sta_vdev_for_p2p_device_upon_vdev_exhaust", \
			false, \
			"use sta vdev for p2p device upon vdev exhaust")
#define CFG_P2P_ALL \
	CFG(CFG_ACTION_FRAME_RANDOM_SEQ_NUM_ENABLED) \
	CFG(CFG_GO_KEEP_ALIVE_PERIOD) \
	CFG(CFG_GO_LINK_MONITOR_PERIOD) \
	CFG(CFG_P2P_DEVICE_ADDRESS_ADMINISTRATED) \
	CFG(CFG_P2P_GO_ON_5GHZ_INDOOR_CHANNEL) \
	CFG(CFG_GO_IGNORE_NON_P2P_PROBE_REQ) \
	CFG(CFG_USE_STA_VDEV_FOR_P2P_DEVICE) \
	CFG(STA_VDEV_FOR_P2P_DEVICE_UPON_VDEV_EXHAUST)

#endif
