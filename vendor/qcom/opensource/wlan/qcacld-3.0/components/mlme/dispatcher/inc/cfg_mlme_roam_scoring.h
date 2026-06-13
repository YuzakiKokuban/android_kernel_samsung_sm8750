/*
 * Copyright (c) 2012-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022,2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * DOC: This file contains roam specific SCORING related CFG/INI Items.
 */

#ifndef __CFG_MLME_ROAM_SCORING_H
#define __CFG_MLME_ROAM_SCORING_H

#ifdef CONNECTION_ROAMING_CFG
#define RoamCommon_Delta_min 0
#define RoamCommon_Delta_max 30
#define RoamCommon_Delta_default 20
#define RoamIdle_Delta_min 0
#define RoamIdle_Delta_max 20
#define RoamIdle_Delta_default 0
#define RoamBeaconLoss_TargetMinRSSI_min -127
#define RoamBeaconLoss_TargetMinRSSI_max -70
#define RoamBeaconLoss_TargetMinRSSI_default -75
#define RoamBTM_Delta_min 0
#define RoamBTM_Delta_max 20
#define RoamBTM_Delta_default 0
#define RoamEmergency_TargetMinRSSI_min -127
#define RoamEmergency_TargetMinRSSI_max -70
#define RoamEmergency_TargetMinRSSI_default -70
#else
#define RoamCommon_Delta_min 0
#define RoamCommon_Delta_max 100
#define RoamCommon_Delta_default 0
#define RoamIdle_Delta_min 0
#define RoamIdle_Delta_max 100
#define RoamIdle_Delta_default 0
#define RoamBeaconLoss_TargetMinRSSI_min -120
#define RoamBeaconLoss_TargetMinRSSI_max 0
#define RoamBeaconLoss_TargetMinRSSI_default -75
#define RoamBTM_Delta_min 0
#define RoamBTM_Delta_max 100
#define RoamBTM_Delta_default 0
#define RoamEmergency_TargetMinRSSI_min -120
#define RoamEmergency_TargetMinRSSI_max 0
#define RoamEmergency_TargetMinRSSI_default -75
#endif

#define Aggressive_RoamCommon_Delta_min 0
#define Aggressive_RoamCommon_Delta_max 30
#define Aggressive_RoamCommon_Delta_default 10
/*
 * <ini>
 * roam_score_delta_bitmap - bitmap to enable roam triggers on
 * which roam score delta is to be applied during roam candidate
 * selection
 * @Min: 0
 * @Max: 0xffffffff
 * @Default: 0xffffffff
 *
 * Bitmap value of the following roam triggers:
 * ROAM_TRIGGER_REASON_NONE       - B0,
 * ROAM_TRIGGER_REASON_PER        - B1,
 * ROAM_TRIGGER_REASON_BMISS      - B2,
 * ROAM_TRIGGER_REASON_LOW_RSSI   - B3,
 * ROAM_TRIGGER_REASON_HIGH_RSSI  - B4,
 * ROAM_TRIGGER_REASON_PERIODIC   - B5,
 * ROAM_TRIGGER_REASON_MAWC       - B6,
 * ROAM_TRIGGER_REASON_DENSE      - B7,
 * ROAM_TRIGGER_REASON_BACKGROUND - B8,
 * ROAM_TRIGGER_REASON_FORCED     - B9,
 * ROAM_TRIGGER_REASON_BTM        - B10,
 * ROAM_TRIGGER_REASON_UNIT_TEST  - B11,
 * ROAM_TRIGGER_REASON_BSS_LOAD   - B12
 * ROAM_TRIGGER_REASON_DISASSOC   - B13
 * ROAM_TRIGGER_REASON_IDLE_ROAM  - B14
 *
 * When the bit corresponding to a particular roam trigger reason
 * is set, the value of "roam_score_delta" is expected over the
 * roam score of the current connected AP, for that triggered roam
 *
 * Related: None
 *
 * Supported Feature: Roaming
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_ROAM_SCORE_DELTA_TRIGGER_BITMAP CFG_INI_UINT( \
			"roam_score_delta_bitmap", \
			0, \
			0xFFFFFFFF, \
			0xFFFFFFFF, \
			CFG_VALUE_OR_DEFAULT, \
			"Bitmap for various roam triggers")

/*
 * <ini>
 * roam_score_delta/RoamCommon_Delta - Percentage increment in roam score value
 * that is expected from a roaming candidate AP.
 * @Min: 0
 * @Max: 100
 * @Default: 0
 *
 * This ini is used to provide the percentage increment value over roam
 * score for the candidate APs so that they can be preferred over current
 * AP for roaming.
 *
 * Related: None
 *
 * Supported Feature: Roaming
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_ROAM_SCORE_DELTA CFG_INI_UINT( \
			"roam_score_delta RoamCommon_Delta", \
			RoamCommon_Delta_min,\
			RoamCommon_Delta_max, \
			RoamCommon_Delta_default, \
			CFG_VALUE_OR_DEFAULT, \
			"candidate AP's percentage roam score delta")

/*
 * <ini>
 * Aggressive_RoamCommon_Delta  - Percentage increment in roam
 * score value that is expected from a roaming candidate AP.
 * @Min: 0
 * @Max: 100
 * @Default: 0
 *
 * This ini is used to provide the percentage increment value over roam
 * score for the candidate APs so that they can be preferred over current
 * AP for roaming.
 * This value is applicable only when the roam scan policy is aggressive
 *
 * Related: None
 *
 * Supported Feature: Roaming
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_AGGRESSIVE_ROAM_SCORE_DELTA CFG_INI_UINT( \
			"Aggressive_RoamCommon_Delta", \
			Aggressive_RoamCommon_Delta_min,\
			Aggressive_RoamCommon_Delta_max, \
			Aggressive_RoamCommon_Delta_default, \
			CFG_VALUE_OR_DEFAULT, \
			"candidate AP's percentage roam score delta")

/*
 * <ini>
 * min_roam_score_delta - Difference of roam score values between connected
 * AP and roam candidate AP.
 * @Min: 0
 * @Max: 10000
 * @Default: 0
 *
 * This ini is used during CU and low rssi based roam triggers, consider
 * AP as roam candidate only if its roam score is better than connected
 * AP score by at least min_roam_score_delta.
 * If user configured "roam_score_delta" and "min_roam_score_delta" both,
 * then firmware selects roam candidate AP by considering values of both
 * INIs.
 * Example: If DUT is connected with AP1 and roam candidate AP2 has roam
 * score greater than roam_score_delta and min_roam_score_delta then only
 * firmware will trigger roaming to AP2.
 *
 * Related: roam_score_delta
 *
 * Supported Feature: Roaming
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_CAND_MIN_ROAM_SCORE_DELTA CFG_INI_UINT( \
			"min_roam_score_delta", \
			0, \
			10000, \
			0, \
			CFG_VALUE_OR_DEFAULT, \
			"Diff between connected AP's and candidate AP's roam score")

/*
 * <ini>
 * RoamCommon_MinRoamDelta - Difference of roam score values between connected
 * AP and roam candidate AP.
 * @Min: 0
 * @Max: 100
 * @Default: 15
 *
 * This ini is used during CU and low rssi based roam triggers, consider
 * AP as roam candidate only if its roam score is better than connected
 * AP score by at least RoamCommon_MinRoamDelta.
 * If user configured "RoamCommon_Delta" and "RoamCommon_MinRoamDelta" both,
 * then firmware selects roam candidate AP by considering values of both
 * INIs.
 * Example: If DUT is connected with AP1 and roam candidate AP2 has roam
 * score greater than RoamCommon_Delta and RoamCommon_MinRoamDelta then only
 * firmware will trigger roaming to AP2.
 * This value needs to be given in percentage
 *
 * Related: RoamCommon_Delta
 *
 * Supported Feature: Roaming
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_ROAM_COMMON_MIN_ROAM_DELTA CFG_INI_UINT( \
			"RoamCommon_MinRoamDelta", \
			0, \
			100, \
			15, \
			CFG_VALUE_OR_DEFAULT, \
			"Diff bet connected AP's and candidate AP's roam score")

/*
 * <ini>
 * Aggressive_Roam Common_MinRoamDelta - Difference of roam score values between
 * connected AP and roam candidate AP.
 * @Min: 0
 * @Max: 100
 * @Default: 10
 *
 * This ini is used during CU and low rssi based aggressive roam triggers,
 * consider AP as roam candidate only if its roam score is better than connected
 * AP score by at least Aggressive_RoamCommon_MinRoamDelta.
 * If user configured "Aggressive_RoamCommon_MinRoamDelta"
 * then firmware selects roam candidate AP by considering values of both
 * INIs.
 * Example: If DUT is connected with AP1 and roam candidate AP2 has roam
 * score greater than Aggressive_RoamCommon_Delta and
 * Aggressive_Roam Common_MinRoamDelta  then only firmware will trigger roaming
 * to AP2. This value needs to be given in percentage
 *
 * Related: RoamCommon_Delta
 *
 * Supported Feature: Roaming
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_ROAM_COMMON_AGGRESIVE_MIN_ROAM_DELTA CFG_INI_UINT( \
			"Aggressive_RoamCommon_MinRoamDelta", \
			0, \
			100, \
			10, \
			CFG_VALUE_OR_DEFAULT, \
			"Diff bet connected AP's and candidate AP's roam score")

/*
 * <ini>
 * Aggressive_RoamScan_StepRSSI - Aggressive Roam scan step RSSI
 * @Min: 0
 * @Max: 20
 * @Default: 5
 *
 * This INI is the drop in RSSI value that will trigger a precautionary
 * scan by firmware. Max value is chosen in such a way that this type
 * of scan can be disabled by user in aggressive mode.
 *
 * Related: Roaming
 *
 * Supported Feature: Roaming
 *
 * Usage: Internal
 *
 * <\ini>
 */
#define CFG_ROAM_AGGRESSIVE_SCAN_STEP_RSSI CFG_INI_UINT( \
			"Aggressive_RoamScan_StepRSSI", \
			0, \
			20, \
			5, \
			CFG_VALUE_OR_DEFAULT, \
			"Aggressive Roam scan step RSSI")

/*
 * <ini>
 * enable_scoring_for_roam - enable/disable scoring logic in FW for candidate
 *
 * <ini>
 * enable_scoring_for_roam - enable/disable scoring logic in FW for candidate
 * selection during roaming
 *
 * @Min: 0
 * @Max: 1
 * @Default: 1
 *
 * This ini is used to enable/disable scoring logic in FW for candidate
 * selection during roaming.
 *
 * Supported Feature: STA Candidate selection by FW during roaming based on
 * scoring logic.
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_ENABLE_SCORING_FOR_ROAM CFG_INI_BOOL( \
		"enable_scoring_for_roam", \
		1, \
		"Enable Scoring for Roam")

/*
 * <cfg>
 * apsd_enabled - Enable automatic power save delivery
 * @Min: 0
 * @Max: 1
 * @Default: 0
 *
 * Supported Feature: Power save
 *
 * Usage: Internal
 *
 * </cfg>
 */
#define CFG_APSD_ENABLED CFG_BOOL( \
		"apsd_enabled", \
		0, \
		"Enable APSD")

/*
 * <ini>
 * candidate_min_rssi_for_disconnect/RoamEmergency_TargetMinRSSI -
 * Candidate AP minimum RSSI in idle roam trigger(in dBm).
 * @Min: -120
 * @Max: 0
 * @Default: -75
 *
 * Minimum RSSI value of the candidate AP to consider it as candidate for
 * roaming when roam trigger is Deauthentication/Disconnection from current
 * AP. This value will be sent to firmware over the WMI_ROAM_AP_PROFILE
 * wmi command in the roam_min_rssi_param_list tlv.
 *
 * Related: enable_idle_roam.
 *
 * Supported Feature: Roaming
 *
 * Usage: Internal/External
 *
 * </ini>
 */
#define CFG_DISCONNECT_ROAM_TRIGGER_MIN_RSSI CFG_INI_INT( \
		"candidate_min_rssi_for_disconnect RoamEmergency_TargetMinRSSI", \
		RoamEmergency_TargetMinRSSI_min, \
		RoamEmergency_TargetMinRSSI_max, \
		RoamEmergency_TargetMinRSSI_default, \
		CFG_VALUE_OR_DEFAULT, \
		"Minimum RSSI of candidate AP for Disconnect roam trigger")

/*
 * <ini>
 * candidate_min_rssi_for_beacon_miss/RoamBeaconLoss_TargetMinRSSI -
 * Candidate AP minimum RSSI for beacon miss roam trigger (in dBm)
 * @Min: -120
 * @Max: 0
 * @Default: -75
 *
 * Minimum RSSI value of the candidate AP to consider it as candidate for
 * roaming when roam trigger is disconnection from current AP due to beacon
 * miss. This value will be sent to firmware over the WMI_ROAM_AP_PROFILE
 * wmi command in the roam_min_rssi_param_list tlv.
 *
 * Related: None
 *
 * Supported Feature: Roaming
 *
 * Usage: Internal/External
 *
 * </ini>
 */
#define CFG_BMISS_ROAM_MIN_RSSI CFG_INI_INT( \
	"candidate_min_rssi_for_beacon_miss RoamBeaconLoss_TargetMinRSSI", \
	RoamBeaconLoss_TargetMinRSSI_min, \
	RoamBeaconLoss_TargetMinRSSI_max, \
	RoamBeaconLoss_TargetMinRSSI_default, \
	CFG_VALUE_OR_DEFAULT, \
	"Minimum RSSI of candidate AP for Bmiss roam trigger")

/*
 * <ini>
 * min_rssi_for_2g_to_5g_roam - Candidate AP minimum RSSI for
 * 2G to 5G roam trigger (in dBm)
 * @Min: -120
 * @Max: 0
 * @Default: -70
 *
 * Minimum RSSI value of the candidate AP to consider it as candidate
 * for 2G to 5G roam.
 *
 * Related: None
 *
 * Supported Feature: Roaming
 *
 * Usage: Internal/External
 *
 * </ini>
 */
#define CFG_2G_TO_5G_ROAM_MIN_RSSI CFG_INI_INT( \
	"min_rssi_for_2g_to_5g_roam", \
	-120, \
	0, \
	-70, \
	CFG_VALUE_OR_DEFAULT, \
	"Minimum RSSI of candidate AP for 2G to 5G roam trigger")

/*
 * <ini>
 * roam_trigger_score_delta - Percentage increment in roam score value
 * expected from a roaming candidate AP.
 *
 * @Min: 0
 * @Max: 100
 * @Default: N/A
 *
 * This INI parameter provides the percentage increment value over the roam
 * score for candidate APs. It allows them to be preferred over the current
 * AP for roaming triggers.
 *
 * Roam score delta in %.
 * Consider AP as roam candidate only if AP score is at least
 * roam_score_delta % better than connected AP score.
 * Ex: roam_score_delta = 20, and connected AP score is 4000,
 * then consider candidate AP only if its score is at least
 * 4800 (= 4000 * 120%)
 *
 * The input string format looks like this:
 * roam_trigger_score_delta="<TRIGGER1>,<score_delta1>,<TRIGGER2>,
 * <score_delta2>..."
 *
 * For example:
 * roam_trigger_score_delta=10,0,14,0,9,5
 * The above input string means:
 * - For BTM and IDLE trigger, no score delta required.
 * - For host invoke roaming, the candidate AP score should be 5%
 *   higher than the current AP.
 * - For other roaming triggers, the candidate AP score should be
 *   [Ini roam_score_delta]% higher than the current AP.
 *
 * TRIGGER values correspond to the following enum roam_trigger_reason:
 * ROAM_TRIGGER_REASON_PER        - 1
 * ROAM_TRIGGER_REASON_BMISS      - 2
 * ROAM_TRIGGER_REASON_LOW_RSSI   - 3
 * ROAM_TRIGGER_REASON_HIGH_RSSI  - 4
 * ROAM_TRIGGER_REASON_PERIODIC   - 5
 * ROAM_TRIGGER_REASON_MAWC       - 6
 * ROAM_TRIGGER_REASON_DENSE      - 7
 * ROAM_TRIGGER_REASON_BACKGROUND - 8
 * ROAM_TRIGGER_REASON_FORCED     - 9
 * ROAM_TRIGGER_REASON_BTM        - 10
 * ROAM_TRIGGER_REASON_UNIT_TEST  - 11
 * ROAM_TRIGGER_REASON_BSS_LOAD   - 12
 * ROAM_TRIGGER_REASON_DISASSOC   - 13
 * ROAM_TRIGGER_REASON_IDLE_ROAM  - 14
 *
 * score_delta: [0 - 100]
 *
 * Related: roam_score_delta
 *
 * Supported Feature: Roaming
 *
 * Usage: External
 *
 * </ini>
 */
#define ROAM_TRIGGER_SCORE_DELTA_STRING_MAX_LEN (ROAM_TRIGGER_REASON_MAX * 7)
#define CFG_ROAM_TRIGGER_SCORE_DELTA CFG_INI_STRING( \
	"roam_trigger_score_delta", \
	0, \
	ROAM_TRIGGER_SCORE_DELTA_STRING_MAX_LEN, \
	"10,0,14,0", \
	"candidate AP's percentage roam score delta")

#define CFG_ROAM_SCORING_ALL \
	CFG(CFG_ROAM_SCORE_DELTA_TRIGGER_BITMAP) \
	CFG(CFG_ROAM_SCORE_DELTA) \
	CFG(CFG_CAND_MIN_ROAM_SCORE_DELTA) \
	CFG(CFG_ROAM_COMMON_MIN_ROAM_DELTA) \
	CFG(CFG_ENABLE_SCORING_FOR_ROAM) \
	CFG(CFG_APSD_ENABLED) \
	CFG(CFG_DISCONNECT_ROAM_TRIGGER_MIN_RSSI) \
	CFG(CFG_BMISS_ROAM_MIN_RSSI) \
	CFG(CFG_2G_TO_5G_ROAM_MIN_RSSI) \
	CFG(CFG_ROAM_TRIGGER_SCORE_DELTA) \
	CFG(CFG_AGGRESSIVE_ROAM_SCORE_DELTA) \
	CFG(CFG_ROAM_COMMON_AGGRESIVE_MIN_ROAM_DELTA) \
	CFG(CFG_ROAM_AGGRESSIVE_SCAN_STEP_RSSI)

#endif /* __CFG_MLME_ROAM_SCORING_H */
