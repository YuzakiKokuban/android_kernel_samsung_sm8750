/*
 * Copyright (c) 2019-2020 The Linux Foundation. All rights reserved.
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

/**
 * DOC: This file contains ini params for denylist mgr component
 */

#ifndef __CFG_DLM_H_
#define __CFG_DLM_H_

#ifdef FEATURE_DENYLIST_MGR

/*
 * <ini>
 * avoid_list_expiry_time - Config Param to move AP from avoid to monitor list.
 * @Min: 1 minutes
 * @Max: 300 minutes
 * @Default: 5 minutes
 *
 * This ini is used to specify the time after which the BSSID which is in the
 * avoid list should be moved to monitor list, assuming that the AP or the
 * gateway with which the data stall happenend might have recovered, and now
 * the STA can give another chance to connect to the AP.
 *
 * Supported Feature: Data Stall Recovery
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_AVOID_LIST_EXPIRY_TIME CFG_INI_UINT( \
				"avoid_list_expiry_time", \
				1, \
				300, \
				5, \
				CFG_VALUE_OR_DEFAULT, \
				"avoid list expiry")

/*
 * <ini>
 * bad_bssid_counter_thresh - Threshold to move the Ap from avoid to denylist.
 * @Min: 2
 * @Max: 100
 * @Default: 3
 *
 * This ini is used to specify the threshld after which the BSSID which is in
 * the avoid list should be moved to deny list, assuming that the AP or the
 * gateway with which the data stall happenend has no recovered, and now
 * the STA got the NUD failure again with the BSSID
 *
 * Supported Feature: Data Stall Recovery
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_BAD_BSSID_COUNTER_THRESHOLD CFG_INI_UINT( \
				"bad_bssid_counter_thresh", \
				2, \
				100, \
				3, \
				CFG_VALUE_OR_DEFAULT, \
				"bad bssid counter thresh")

/*
 * <ini>
 * deny_list_expiry_time - Config Param to move AP from denylist to monitor
 * list.
 * @Min: 1 minutes
 * @Max: 600 minutes
 * @Default: 10 minutes
 *
 * This ini is used to specify the time after which the BSSID which is in the
 * deny list should be moved to monitor list, assuming that the AP or the
 * gateway with which the data stall happenend might have recovered, and now
 * the STA can give another chance to connect to the AP.
 *
 * Supported Feature: Data Stall Recovery
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DENY_LIST_EXPIRY_TIME CFG_INI_UINT( \
				"black_list_expiry_time", \
				1, \
				600, \
				10, \
				CFG_VALUE_OR_DEFAULT, \
				"deny list expiry")

/*
 * <ini>
 * bad_bssid_reset_time - Config Param to specify time after which AP would be
 * removed from monitor/avoid when connected.
 * @Min: 30 seconds
 * @Max: 1 minute
 * @Default: 30 seconds
 *
 * This ini is used to specify the time after which the BSSID which is in the
 * avoid or monitor list should be removed from the respective list, if the
 * data stall has not happened till the mentioned time after connection to the
 * AP. That means that the AP has recovered from the previous state where
 * data stall was observed with it, and was moved to avoid list.
 *
 * Supported Feature: Data Stall Recovery
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_BAD_BSSID_RESET_TIME CFG_INI_UINT( \
				"bad_bssid_reset_time", \
				30, \
				60, \
				30, \
				CFG_VALUE_OR_DEFAULT, \
				"bad bssid reset time")

/*
 * <ini>
 * delta_rssi - RSSI threshold value, only when AP rssi improves
 * by threshold value entry would be removed from denylist manager and assoc
 * req would be sent by FW.
 * @Min: 0
 * @Max: 10
 * @Default: 5
 *
 * This ini is used to specify the rssi threshold value, after rssi improves
 * by threshold the BSSID which is in the denylist manager list should be
 * removed from the respective list.
 *
 * Supported Feature: Customer requirement
 *
 * Usage: Internal/External
 *
 * </ini>
 */
#define CFG_DENYLIST_RSSI_THRESHOLD CFG_INI_INT( \
			"delta_rssi", \
			0, \
			10, \
			5, \
			CFG_VALUE_OR_DEFAULT, \
			"Configure delta RSSI")

#ifdef WLAN_FEATURE_11BE_MLO
/*
 * <ini>
 * max_11be_con_failure_allowed - maximum 11BE connection failure allowed per
 * MLO AP.If number of 11BE failure for a MLO AP reaches to the max allowed 11BE
 * failures then new 11BE connection is not be allowed and 11AX is tried
 * for that candidate based on score.
 * @Min: 0
 * @Max: 10
 * @Default: 7
 *
 * This ini is used to specify the max 11BE failure allowed per MLO AP.
 *
 * Supported Feature: STA connection
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_MAX_11BE_CON_FAIL_ALLOWED_PER_AP CFG_INI_INT( \
			"max_11be_con_failure_allowed", \
			0, \
			10, \
			7,  \
			CFG_VALUE_OR_DEFAULT, \
			"configure 11BE connection failure allowed")

#define CFG_DENYLIST_MGR_11BE_CON_FAIL_ALLOWED_PER_AP \
	CFG(CFG_MAX_11BE_CON_FAIL_ALLOWED_PER_AP)
#else
#define CFG_DENYLIST_MGR_11BE_CON_FAIL_ALLOWED_PER_AP
#endif

/*
 * <ini>
 * monitor_con_stability_post_connection_time - time to monitor connection
 * stability post connection. If unwanted disconnections happen then the AP
 * is added to avoid list and connection will not be tried with that AP until
 * the timer expires or this AP is the only AP in the vicinity.
 * @Min: 10 Seconds
 * @Max: 60 Seconds
 * @Default: 20 Seconds
 *
 * This ini is used to specify the time for which connection monitor is required
 * post successful connection.
 *
 * Supported Feature: STA connection
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_MONITOR_CON_STABILITY_POST_CONNECTION_TIME CFG_INI_UINT( \
			"monitor_con_stability_post_connection_time", \
			10, \
			60, \
			20,  \
			CFG_VALUE_OR_DEFAULT, \
			"post connection stability monitor time")

#define CFG_DENYLIST_MGR_ALL \
	CFG(CFG_AVOID_LIST_EXPIRY_TIME) \
	CFG(CFG_BAD_BSSID_COUNTER_THRESHOLD) \
	CFG(CFG_DENY_LIST_EXPIRY_TIME) \
	CFG(CFG_BAD_BSSID_RESET_TIME) \
	CFG(CFG_DENYLIST_RSSI_THRESHOLD) \
	CFG_DENYLIST_MGR_11BE_CON_FAIL_ALLOWED_PER_AP \
	CFG(CFG_MONITOR_CON_STABILITY_POST_CONNECTION_TIME)

#else

#define CFG_DENYLIST_MGR_ALL

#endif /* FEATURE_DENYLIST_MGR */

#endif /* __CFG_DENYLIST_MGR */
