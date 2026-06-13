/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2020, Samsung Electronics. All rights reserved.

 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "ss_dsi_panel_common.h"
#include "ss_panel_power.h"
#include <linux/preempt.h>

__visible_for_testing int ss_event_esd_recovery_init(
		struct samsung_display_driver_data *vdd, int event, void *arg);

struct samsung_display_driver_data vdd_data[MAX_DISPLAY_NDX];

void __iomem *virt_mmss_gp_base;

LIST_HEAD(vdds_list);

#define SS_CMD_IDX(ss_cmd_idx)	(ss_cmd_idx - SS_DSI_CMD_SET_START)

char ss_cmd_set_prop_map[SS_CMD_PROP_SIZE][SS_CMD_PROP_STR_LEN] = {
	[SS_CMD_IDX(SS_DSI_CMD_SET_START)]		= "SS_DSI_CMD_SET_START not parsed from DTSI",

	/* TX */
	[SS_CMD_IDX(TX_CMD_START)]			= "TX_CMD_START not parsed from DTSI",

	/* Level key comamnds are referred while parsing other commands.
	* Put level key first.
	*/
	[SS_CMD_IDX(TX_LEVEL_KEY_START)]	= "TX_LEVEL_KEY_START not parsed from DTSI",
	[SS_CMD_IDX(TX_LEVEL0_KEY_ENABLE)]	= "samsung,level0_key_enable_tx_cmds_revA",
	[SS_CMD_IDX(TX_LEVEL0_KEY_DISABLE)]	= "samsung,level0_key_disable_tx_cmds_revA",
	[SS_CMD_IDX(TX_LEVEL1_KEY_ENABLE)]	= "samsung,level1_key_enable_tx_cmds_revA",
	[SS_CMD_IDX(TX_LEVEL1_KEY_DISABLE)]	= "samsung,level1_key_disable_tx_cmds_revA",
	[SS_CMD_IDX(TX_LEVEL2_KEY_ENABLE)]	= "samsung,level2_key_enable_tx_cmds_revA",
	[SS_CMD_IDX(TX_LEVEL2_KEY_DISABLE)]	= "samsung,level2_key_disable_tx_cmds_revA",
	[SS_CMD_IDX(TX_LEVEL_KEY_END)]	= "TX_LEVEL_KEY_END not parsed from DTSI",

	[SS_CMD_IDX(TX_DSI_CMD_SET_ON)]	= "samsung,mdss_dsi_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_DSI_CMD_SET_ON_ESYNC_POST)]	= "samsung,mdss_dsi_esync_post_on_cmds_revA",
	[SS_CMD_IDX(TX_DSI_CMD_SET_ON_POST)]	= "samsung,dsi_on_post_tx_cmds_revA", /* after frame update */
	[SS_CMD_IDX(TX_DSI_CMD_SET_OFF)]	= "samsung,mdss_dsi_off_tx_cmds_revA",
	[SS_CMD_IDX(TX_TIMING_SWITCH)]	= "samsung,mdss_dsi_timing_switch_tx_cmds_revA",
	[SS_CMD_IDX(TX_MTP_WRITE_SYSFS)]	= "samsung,mtp_write_sysfs_tx_cmds_revA",
	[SS_CMD_IDX(TX_TEMP_DSC)]		= "samsung,temp_dsc_tx_cmds_revA",
	[SS_CMD_IDX(TX_DISPLAY_ON)]		= "samsung,display_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_DISPLAY_ON_POST)]	= "samsung,display_on_post_tx_cmds_revA",
	[SS_CMD_IDX(TX_FIRST_DISPLAY_ON)]	= "samsung,first_display_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_DISPLAY_OFF)]		= "samsung,display_off_tx_cmds_revA",
	[SS_CMD_IDX(TX_SS_BRIGHT_CTRL)]	= "samsung,ss_brightness_tx_cmds_revA",
	[SS_CMD_IDX(TX_SS_SMOOTH_BR_DONE)]	= "samsung,ss_smooth_br_done_tx_cmds_revA",
	[SS_CMD_IDX(TX_SS_LPM_BRIGHT_CTRL)]	= "samsung,ss_lpm_brightness_tx_cmds_revA",
	[SS_CMD_IDX(TX_SS_HMT_BRIGHT_CTRL)]	= "samsung,ss_hmt_brightness_tx_cmds_revA",
	[SS_CMD_IDX(TX_SW_RESET)]		= "samsung,sw_reset_tx_cmds_revA",
	[SS_CMD_IDX(TX_MDNIE_ADB_TEST)]	= "TX_MDNIE_ADB_TEST not parsed from DTSI",
	[SS_CMD_IDX(TX_SELF_GRID_ON)]	= "samsung,self_grid_on_revA",
	[SS_CMD_IDX(TX_SELF_GRID_OFF)]	= "samsung,self_grid_off_revA",
	[SS_CMD_IDX(TX_MIPI_LANE_SET)]	= "samsung,mipi_lane_tx_cmds_revA",

	[SS_CMD_IDX(TX_LPM_ON)]		= "samsung,lpm_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_LPM_OFF)]		= "samsung,lpm_off_tx_cmds_revA",
	[SS_CMD_IDX(TX_LPM_ON_AOR)]		= "samsung,lpm_on_aor_tx_cmds_revA",
	[SS_CMD_IDX(TX_LPM_OFF_AOR)]		= "samsung,lpm_off_aor_tx_cmds_revA",

	[SS_CMD_IDX(TX_PACKET_SIZE)]		= "samsung,packet_size_tx_cmds_revA",
	[SS_CMD_IDX(TX_MDNIE_TUNE)]		= "samsung,mdnie_tx_cmds_revA",
	[SS_CMD_IDX(TX_HMT_ENABLE)]		= "samsung,hmt_enable_tx_cmds_revA",
	[SS_CMD_IDX(TX_HMT_DISABLE)]		= "samsung,hmt_disable_tx_cmds_revA",
	[SS_CMD_IDX(TX_FFC)]			= "samsung,ffc_tx_cmds_revA",
	[SS_CMD_IDX(TX_OSC)]			= "samsung,osc_tx_cmds_revA",
	[SS_CMD_IDX(TX_TFT_PWM)]		= "samsung,tft_pwm_tx_cmds_revA",
	[SS_CMD_IDX(TX_COPR_ENABLE)]		= "samsung,copr_enable_tx_cmds_revA",
	[SS_CMD_IDX(TX_COPR_DISABLE)]	= "samsung,copr_disable_tx_cmds_revA",

	[SS_CMD_IDX(TX_DYNAMIC_HLPM_ENABLE)]	= "samsung,dynamic_hlpm_enable_tx_cmds_revA",
	[SS_CMD_IDX(TX_DYNAMIC_HLPM_DISABLE)]= "samsung,dynamic_hlpm_disable_tx_cmds_revA",

	[SS_CMD_IDX(TX_MCD_ON)]		= "samsung,mcd_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_MCD_OFF)]		= "samsung,mcd_off_tx_cmds_revA",
	[SS_CMD_IDX(TX_XTALK_ON)]		= "samsung,xtalk_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_XTALK_OFF)]		= "samsung,xtalk_off_tx_cmds_revA",
	[SS_CMD_IDX(TX_IRC)]			= "samsung,irc_tx_cmds_revA",
	[SS_CMD_IDX(TX_IRC_OFF)]		= "samsung,irc_off_tx_cmds_revA",
	[SS_CMD_IDX(TX_SMOOTH_DIMMING_ON)]	= "samsung,smooth_dimming_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_SMOOTH_DIMMING_OFF)]	= "samsung,smooth_dimming_off_tx_cmds_revA",

	/* TBR: remove deprecated self commands */
	[SS_CMD_IDX(TX_SS_DIA)]			= "samsung,ss_dia_tx_cmds_revA",
	[SS_CMD_IDX(TX_SELF_IDLE_AOD_ENTER)]		= "samsung,self_idle_aod_enter",
	[SS_CMD_IDX(TX_SELF_IDLE_AOD_EXIT)]		= "samsung,self_idle_aod_exit",
	[SS_CMD_IDX(TX_SELF_IDLE_TIMER_ON)]		= "samsung,self_idle_timer_on",
	[SS_CMD_IDX(TX_SELF_IDLE_TIMER_OFF)]		= "samsung,self_idle_timer_off",
	[SS_CMD_IDX(TX_SELF_IDLE_MOVE_ON_PATTERN1)]	= "samsung,self_idle_move_on_pattern1",
	[SS_CMD_IDX(TX_SELF_IDLE_MOVE_ON_PATTERN2)]	= "samsung,self_idle_move_on_pattern2",
	[SS_CMD_IDX(TX_SELF_IDLE_MOVE_ON_PATTERN3)]	= "samsung,self_idle_move_on_pattern3",
	[SS_CMD_IDX(TX_SELF_IDLE_MOVE_ON_PATTERN4)]	= "samsung,self_idle_move_on_pattern4",
	[SS_CMD_IDX(TX_SELF_IDLE_MOVE_OFF)]		= "samsung,self_idle_move_off",
	[SS_CMD_IDX(TX_SPSRAM_DATA_READ)]		= "samsung,spsram_data_read_cmds_revA",

	/* self display */
	[SS_CMD_IDX(TX_SELF_DISP_CMD_START)]		= "TX_SELF_DISP_CMD_START not parsed from DTSI",
	[SS_CMD_IDX(TX_SELF_DISP_ON)]		= "samsung,self_dispaly_on_revA",
	[SS_CMD_IDX(TX_SELF_DISP_OFF)]		= "samsung,self_dispaly_off_revA",
	[SS_CMD_IDX(TX_SELF_TIME_SET)]		= "samsung,self_time_set_revA",
	[SS_CMD_IDX(TX_SELF_MOVE_ON)]		= "samsung,self_move_on_revA",
	[SS_CMD_IDX(TX_SELF_MOVE_ON_100)]		= "samsung,self_move_on_100_revA",
	[SS_CMD_IDX(TX_SELF_MOVE_ON_200)]		= "samsung,self_move_on_200_revA",
	[SS_CMD_IDX(TX_SELF_MOVE_ON_500)]		= "samsung,self_move_on_500_revA",
	[SS_CMD_IDX(TX_SELF_MOVE_ON_1000)]		= "samsung,self_move_on_1000_revA",
	[SS_CMD_IDX(TX_SELF_MOVE_ON_DEBUG)]		= "samsung,self_move_on_debug_revA",
	[SS_CMD_IDX(TX_SELF_MOVE_RESET)]		= "samsung,self_move_reset_revA",
	[SS_CMD_IDX(TX_SELF_MOVE_OFF)]		= "samsung,self_move_off_revA",
	[SS_CMD_IDX(TX_SELF_MOVE_2C_SYNC_OFF)]	= "samsung,self_move_2c_sync_off_revA",
	[SS_CMD_IDX(TX_SELF_MASK_SETTING)]		= "samsung,self_mask_setting_revA",
	[SS_CMD_IDX(TX_SELF_MASK_SIDE_MEM_SET)]	= "samsung,self_mask_mem_setting_revA",
	[SS_CMD_IDX(TX_SELF_MASK_ON)]		= "samsung,self_mask_on_revA",
	[SS_CMD_IDX(TX_SELF_MASK_ON_FACTORY)]	= "samsung,self_mask_on_factory_revA",
	[SS_CMD_IDX(TX_SELF_MASK_OFF)]		= "samsung,self_mask_off_revA",
	[SS_CMD_IDX(TX_SELF_MASK_UDC_ON)]		= "samsung,self_mask_udc_on_revA",
	[SS_CMD_IDX(TX_SELF_MASK_UDC_OFF)]		= "samsung,self_mask_udc_off_revA",
	[SS_CMD_IDX(TX_SELF_MASK_GREEN_CIRCLE_ON)]	= "samsung,self_mask_green_circle_on_revA",
	[SS_CMD_IDX(TX_SELF_MASK_GREEN_CIRCLE_OFF)]	= "samsung,self_mask_green_circle_off_revA",
	[SS_CMD_IDX(TX_SELF_MASK_GREEN_CIRCLE_ON_FACTORY)]	= "samsung,self_mask_green_circle_on_factory_revA",
	[SS_CMD_IDX(TX_SELF_MASK_GREEN_CIRCLE_OFF_FACTORY)]	= "samsung,self_mask_green_circle_off_factory_revA",
	[SS_CMD_IDX(TX_SELF_MASK_IMAGE)]		= "TX_SELF_MASK_IMAGE not parsed from DTSI",
	[SS_CMD_IDX(TX_SELF_MASK_IMAGE_CRC)]		= "TX_SELF_MASK_IMAGE_CRC not parsed from DTSI",
	[SS_CMD_IDX(TX_SELF_ICON_SET_PRE)]		= "samsung,self_icon_setting_pre_revA",
	[SS_CMD_IDX(TX_SELF_ICON_SET_POST)]		= "samsung,self_icon_setting_post_revA",
	[SS_CMD_IDX(TX_SELF_ICON_SIDE_MEM_SET)]	= "samsung,self_icon_mem_setting_revA",
	[SS_CMD_IDX(TX_SELF_ICON_GRID)]		= "samsung,self_icon_grid_revA",
	[SS_CMD_IDX(TX_SELF_ICON_ON)]		= "samsung,self_icon_on_revA",
	[SS_CMD_IDX(TX_SELF_ICON_ON_GRID_ON)]	= "samsung,self_icon_on_grid_on_revA",
	[SS_CMD_IDX(TX_SELF_ICON_ON_GRID_OFF)]	= "samsung,self_icon_on_grid_off_revA",
	[SS_CMD_IDX(TX_SELF_ICON_OFF_GRID_ON)]	= "samsung,self_icon_off_grid_on_revA",
	[SS_CMD_IDX(TX_SELF_ICON_OFF_GRID_OFF)]	= "samsung,self_icon_off_grid_off_revA",
	[SS_CMD_IDX(TX_SELF_ICON_GRID_2C_SYNC_OFF)]	= "samsung,self_icon_grid_2c_sync_off_revA",
	[SS_CMD_IDX(TX_SELF_ICON_OFF)]		= "samsung,self_icon_off_revA",
	[SS_CMD_IDX(TX_SELF_ICON_IMAGE)]		= "TX_SELF_ICON_IMAGE not parsed from DTSI",
	[SS_CMD_IDX(TX_SELF_BRIGHTNESS_ICON_ON)]	= "samsung,self_brightness_icon_on_revA",
	[SS_CMD_IDX(TX_SELF_BRIGHTNESS_ICON_OFF)]	= "samsung,self_brightness_icon_off_revA",
	[SS_CMD_IDX(TX_SELF_ACLOCK_SET_PRE)]		= "samsung,self_aclock_setting_pre_revA",
	[SS_CMD_IDX(TX_SELF_ACLOCK_SET_POST)]	= "samsung,self_aclock_setting_post_revA",
	[SS_CMD_IDX(TX_SELF_ACLOCK_SIDE_MEM_SET)]	= "samsung,self_aclock_sidemem_setting_revA",
	[SS_CMD_IDX(TX_SELF_ACLOCK_ON)]		= "samsung,self_aclock_on_revA",
	[SS_CMD_IDX(TX_SELF_ACLOCK_TIME_UPDATE)]	= "samsung,self_aclock_time_update_revA",
	[SS_CMD_IDX(TX_SELF_ACLOCK_ROTATION)]	= "samsung,self_aclock_rotation_revA",
	[SS_CMD_IDX(TX_SELF_ACLOCK_OFF)]		= "samsung,self_aclock_off_revA",
	[SS_CMD_IDX(TX_SELF_ACLOCK_HIDE)]		= "samsung,self_aclock_hide_revA",
	[SS_CMD_IDX(TX_SELF_ACLOCK_IMAGE)]		= "TX_SELF_ACLOCK_IMAGE not parsed from DTSI",
	[SS_CMD_IDX(TX_SELF_DCLOCK_SET_PRE)]		= "samsung,self_dclock_setting_pre_revA",
	[SS_CMD_IDX(TX_SELF_DCLOCK_SET_POST)]	= "samsung,self_dclock_setting_post_revA",
	[SS_CMD_IDX(TX_SELF_DCLOCK_SIDE_MEM_SET)]	= "samsung,self_dclock_sidemem_setting_revA",
	[SS_CMD_IDX(TX_SELF_DCLOCK_ON)]		= "samsung,self_dclock_on_revA",
	[SS_CMD_IDX(TX_SELF_DCLOCK_BLINKING_ON)]	= "samsung,self_dclock_blinking_on_revA",
	[SS_CMD_IDX(TX_SELF_DCLOCK_BLINKING_OFF)]	= "samsung,self_dclock_blinking_off_revA",
	[SS_CMD_IDX(TX_SELF_DCLOCK_TIME_UPDATE)]	= "samsung,self_dclock_time_update_revA",
	[SS_CMD_IDX(TX_SELF_DCLOCK_OFF)]		= "samsung,self_dclock_off_revA",
	[SS_CMD_IDX(TX_SELF_DCLOCK_HIDE)]		= "samsung,self_dclock_hide_revA",
	[SS_CMD_IDX(TX_SELF_DCLOCK_IMAGE)]		= "TX_SELF_DCLOCK_IMAGE not parsed from DTSI",
	[SS_CMD_IDX(TX_SELF_CLOCK_2C_SYNC_OFF)]	= "samsung,self_clock_2c_sync_off_revA",
	[SS_CMD_IDX(TX_SELF_VIDEO_IMAGE)]		= "TX_SELF_VIDEO_IMAGE not parsed from DTSI",
	[SS_CMD_IDX(TX_SELF_VIDEO_SIDE_MEM_SET)]	= "samsung,self_video_mem_setting_revA",
	[SS_CMD_IDX(TX_SELF_VIDEO_ON)]		= "samsung,self_video_on_revA",
	[SS_CMD_IDX(TX_SELF_VIDEO_OFF)]		= "samsung,self_video_off_revA",
	[SS_CMD_IDX(TX_SELF_PARTIAL_HLPM_SCAN_SET)]	= "samsung,self_partial_hlpm_scan_set_revA",
	[SS_CMD_IDX(RX_SELF_DISP_DEBUG)]		= "samsung,self_disp_debug_rx_cmds_revA",
	[SS_CMD_IDX(TX_SELF_MASK_CHECK)]		= "samsung,self_mask_check_tx_cmds_revA",
	[SS_CMD_IDX(TX_SELF_DISP_CMD_END)]		= "TX_SELF_DISP_CMD_END not parsed from DTSI",

	/* MAFPC */
	[SS_CMD_IDX(TX_MAFPC_CMD_START)]		= "TX_MAFPC_CMD_START not parsed from DTSI",
	[SS_CMD_IDX(TX_MAFPC_SETTING)]		= "samsung,mafpc_setting_revA",
	[SS_CMD_IDX(TX_MAFPC_SETTING_INSTANCE)]		= "samsung,mafpc_setting_instance_revA",
	[SS_CMD_IDX(TX_MAFPC_CTRL_DATA)]	= "samsung,mafpc_ctrl_data_revA",
	[SS_CMD_IDX(TX_MAFPC_ON)]			= "samsung,mafpc_on_revA",
	[SS_CMD_IDX(TX_MAFPC_ON_FACTORY)]		= "samsung,mafpc_on_factory_revA",
	[SS_CMD_IDX(TX_MAFPC_OFF)]			= "samsung,mafpc_off_revA",
	[SS_CMD_IDX(TX_MAFPC_TE_ON)]			= "samsung,mafpc_te_on_revA",
	[SS_CMD_IDX(TX_MAFPC_TE_OFF)]		= "samsung,mafpc_te_off_revA",
	[SS_CMD_IDX(TX_MAFPC_CRC_CHECK)]		= "samsung,mafpc_crc_check_tx_cmds_revA",
	[SS_CMD_IDX(TX_MAFPC_CMD_END)]		= "TX_MAFPC_CMD_END not parsed from DTSI",

	/* TEST MODE */
	[SS_CMD_IDX(TX_TEST_MODE_CMD_START)]		= "TX_TEST_MODE_CMD_START not parsed from DTSI",

	[SS_CMD_IDX(RX_GCT_ECC)]			= "samsung,gct_ecc_rx_cmds_revA",
	[SS_CMD_IDX(RX_SSR)]				= "samsung,ssr_tx_cmds_revA",
	[SS_CMD_IDX(RX_SSR_ON)]			= "samsung,ssr_on_rx_cmds_revA",
	[SS_CMD_IDX(RX_SSR_CHECK)]			= "samsung,ssr_check_rx_cmds_revA",

	[SS_CMD_IDX(TX_GCT_ENTER)]			= "samsung,gct_enter_tx_cmds_revA",
	[SS_CMD_IDX(TX_GCT_LV)]			= "samsung,gct_lv_tx_cmds_revA",
	[SS_CMD_IDX(TX_GCT_HV)]			= "samsung,gct_hv_tx_cmds_revA",

	[SS_CMD_IDX(TX_GRAY_SPOT_TEST_ON)]		= "samsung,gray_spot_test_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_GRAY_SPOT_TEST_OFF)]		= "samsung,gray_spot_test_off_tx_cmds_revA",
	[SS_CMD_IDX(TX_VGLHIGHDOT_TEST_ON)]		= "samsung,vglhighdot_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_VGLHIGHDOT_TEST_2)]		= "samsung,vglhighdot_2_tx_cmds_revA",
	[SS_CMD_IDX(TX_VGLHIGHDOT_TEST_OFF)]		= "samsung,vglhighdot_off_tx_cmds_revA",
	[SS_CMD_IDX(TX_MICRO_SHORT_TEST_ON)]		= "samsung,micro_short_test_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_MICRO_SHORT_TEST_OFF)]	= "samsung,micro_short_test_off_tx_cmds_revA",
	[SS_CMD_IDX(TX_CCD_ON)]			= "samsung,ccd_test_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_CCD_OFF)]			= "samsung,ccd_test_off_tx_cmds_revA",
	[SS_CMD_IDX(TX_BRIGHTDOT_ON)]		= "samsung,brightdot_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_BRIGHTDOT_OFF)]		= "samsung,brightdot_off_tx_cmds_revA",
	[SS_CMD_IDX(TX_BRIGHTDOT_LF_ON)]		= "samsung,brightdot_lf_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_BRIGHTDOT_LF_OFF)]		= "samsung,brightdot_lf_off_tx_cmds_revA",
	[SS_CMD_IDX(TX_DSC_CRC)]			= "samsung,dsc_crc_test_tx_cmds_revA",
	[SS_CMD_IDX(TX_PANEL_AGING_ON)]			= "samsung,panel_aging_test_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_PANEL_AGING_OFF)]		= "samsung,panel_aging_test_off_tx_cmds_revA",
	[SS_CMD_IDX(TX_TEST_MODE_CMD_END)]		= "TX_TEST_MODE_CMD_END not parsed from DTSI",

	/*FLASH GAMMA */
	[SS_CMD_IDX(TX_FLASH_GAMMA_PRE1)]		= "samsung,flash_gamma_pre_tx_cmds1_revA",
	[SS_CMD_IDX(TX_FLASH_GAMMA_PRE2)]		= "samsung,flash_gamma_pre_tx_cmds2_revA",
	[SS_CMD_IDX(TX_FLASH_GAMMA)]			= "samsung,flash_gamma_tx_cmds_revA",
	[SS_CMD_IDX(TX_FLASH_GAMMA_POST)]		= "samsung,flash_gamma_post_tx_cmds_revA",

	/* FD settings */
	[SS_CMD_IDX(TX_FD_ON)]			= "samsung,fd_on_tx_cmds_revA",
	[SS_CMD_IDX(TX_FD_OFF)]			= "samsung,fd_off_tx_cmds_revA",

	[SS_CMD_IDX(TX_VRR_GM2_GAMMA_COMP)]		= "samsung,vrr_gm2_gamma_comp_tx_cmds_revA",
	[SS_CMD_IDX(TX_VRR_GM2_GAMMA_COMP2)]	= "samsung,vrr_gm2_gamma_comp2_tx_cmds_revA",
	[SS_CMD_IDX(TX_VRR_GM2_GAMMA_COMP3)]	= "samsung,vrr_gm2_gamma_comp3_tx_cmds_revA",

	/* for vidoe panel dfps */
	[SS_CMD_IDX(TX_DFPS)]		= "samsung,dfps_tx_cmds_revA",

	/* for certain panel that need TE adjust cmd*/
	[SS_CMD_IDX(TX_ADJUST_TE)]		= "samsung,adjust_te_cmds_revA",

	[SS_CMD_IDX(TX_FG_ERR)]		= "samsung,fg_err_cmds_revA",

	[SS_CMD_IDX(TX_EARLY_TE)]		= "samsung,early_te_cmds_revA",

	[SS_CMD_IDX(TX_VLIN1_TEST_ENTER)]	= "samsung,vlin1_test_enter_tx_cmds_revA",
	[SS_CMD_IDX(TX_VLIN1_TEST_EXIT)]	= "samsung,vlin1_test_exit_tx_cmds_revA",

	[SS_CMD_IDX(TX_LOCAL_HBM_ENTER)]	= "samsung,local_hbm_enter_revA",
	[SS_CMD_IDX(TX_LOCAL_HBM_ON_CIRCLE_OFF)] = "samsung,local_hbm_on_circle_off_revA",
	[SS_CMD_IDX(TX_LOCAL_HBM_ON_CIRCLE_ON)]	= "samsung,local_hbm_on_circle_on_revA",
	[SS_CMD_IDX(TX_LOCAL_HBM_EXIT)]		= "samsung,local_hbm_exit_revA",
	[SS_CMD_IDX(TX_LOCAL_HBM_COMPENSATION)] = "samsung,local_hbm_compensation_tx_cmds_revA",
	[SS_CMD_IDX(TX_CMD_END)]		= "TX_CMD_END not parsed from DTSI",

	/* RX */
	[SS_CMD_IDX(RX_CMD_START)]		= "RX_CMD_START not parsed from DTSI",
	[SS_CMD_IDX(RX_MANUFACTURE_ID)]	= "samsung,manufacture_id_rx_cmds_revA",
	[SS_CMD_IDX(RX_MANUFACTURE_ID0)]	= "samsung,manufacture_id0_rx_cmds_revA",
	[SS_CMD_IDX(RX_MANUFACTURE_ID1)]	= "samsung,manufacture_id1_rx_cmds_revA",
	[SS_CMD_IDX(RX_MANUFACTURE_ID2)]	= "samsung,manufacture_id2_rx_cmds_revA",
	[SS_CMD_IDX(RX_MODULE_INFO)]		= "samsung,module_info_rx_cmds_revA",
	[SS_CMD_IDX(RX_MANUFACTURE_DATE)]	= "samsung,manufacture_date_rx_cmds_revA",
	[SS_CMD_IDX(RX_DDI_ID)]		= "samsung,ddi_id_rx_cmds_revA",
	[SS_CMD_IDX(RX_CELL_ID)]		= "samsung,cell_id_rx_cmds_revA",
	[SS_CMD_IDX(RX_OCTA_ID)]		= "samsung,octa_id_rx_cmds_revA",
	[SS_CMD_IDX(RX_RDDPM)]		= "samsung,rddpm_rx_cmds_revA",
	[SS_CMD_IDX(RX_MTP_READ_SYSFS)]	= "samsung,mtp_read_sysfs_rx_cmds_revA",
	[SS_CMD_IDX(RX_MDNIE)]		= "samsung,mdnie_read_rx_cmds_revA",
	[SS_CMD_IDX(RX_LDI_DEBUG0)]		= "samsung,ldi_debug0_rx_cmds_revA",
	[SS_CMD_IDX(RX_LDI_DEBUG1)]		= "samsung,ldi_debug1_rx_cmds_revA",
	[SS_CMD_IDX(RX_LDI_DEBUG2)]		= "samsung,ldi_debug2_rx_cmds_revA",
	[SS_CMD_IDX(RX_LDI_DEBUG3)]		= "samsung,ldi_debug3_rx_cmds_revA",
	[SS_CMD_IDX(RX_LDI_DEBUG4)]		= "samsung,ldi_debug4_rx_cmds_revA",
	[SS_CMD_IDX(RX_LDI_DEBUG5)]		= "samsung,ldi_debug5_rx_cmds_revA",
	[SS_CMD_IDX(RX_LDI_DEBUG6)]		= "samsung,ldi_debug6_rx_cmds_revA",
	[SS_CMD_IDX(RX_LDI_DEBUG7)]		= "samsung,ldi_debug7_rx_cmds_revA",
	[SS_CMD_IDX(RX_LDI_DEBUG8)]		= "samsung,ldi_debug8_rx_cmds_revA",
	[SS_CMD_IDX(RX_LDI_DEBUG_LOGBUF)]	= "samsung,ldi_debug_logbuf_rx_cmds_revA",
	[SS_CMD_IDX(RX_LDI_DEBUG_PPS1)]	= "samsung,ldi_debug_pps1_rx_cmds_revA",
	[SS_CMD_IDX(RX_LDI_DEBUG_PPS2)]	= "samsung,ldi_debug_pps2_rx_cmds_revA",
	[SS_CMD_IDX(RX_LDI_LOADING_DET)]	= "samsung,ldi_loading_det_rx_cmds_revA",
	[SS_CMD_IDX(RX_FLASH_GAMMA)]		= "samsung,flash_gamma_rx_cmds_revA",
	[SS_CMD_IDX(RX_FLASH_LOADING_CHECK)]	= "samsung,flash_loading_check_revA",
	[SS_CMD_IDX(RX_UDC_DATA)]		= "samsung,udc_data_read_cmds_revA",
	[SS_CMD_IDX(RX_LOCAL_HBM_COMPENSATION)] = "samsung,local_hbm_compensation_rx_cmds_revA",
	[SS_CMD_IDX(RX_CMD_END)]		= "RX_CMD_END not parsed from DTSI"
};

char ss_panel_id0_get(struct samsung_display_driver_data *vdd)
{
	return (vdd->manufacture_id_dsi & 0xFF0000) >> 16;
}

char ss_panel_id1_get(struct samsung_display_driver_data *vdd)
{
	return (vdd->manufacture_id_dsi & 0xFF00) >> 8;
}

char ss_panel_id2_get(struct samsung_display_driver_data *vdd)
{
	return vdd->manufacture_id_dsi & 0xFF;
}

char ss_panel_rev_get(struct samsung_display_driver_data *vdd)
{
	return vdd->manufacture_id_dsi & 0x0F;
}

int ss_panel_attach_get(struct samsung_display_driver_data *vdd)
{
	return vdd->panel_attach_status;
}

int ss_panel_attach_set(struct samsung_display_driver_data *vdd, bool attach)
{
	/* 0bit->DSI0 1bit->DSI1 */
	/* check the lcd id for DISPLAY_1 and DISPLAY_2 */
	if (likely(ss_panel_attached(vdd->ndx) && attach))
		vdd->panel_attach_status = true;
	else
		vdd->panel_attach_status = false;

	LCD_INFO_ONCE(vdd, "panel_attach_status : %d\n", vdd->panel_attach_status);

	return vdd->panel_attach_status;
}

/*
 * Check the lcd id for DISPLAY_1 and DISPLAY_2 using the ndx
 */
int ss_panel_attached(int ndx)
{
	int lcd_id = 0;

	/*
	 * ndx 0 means DISPLAY_1 and ndx 1 means DISPLAY_2
	 */
	if (ndx == PRIMARY_DISPLAY_NDX)
		lcd_id = get_lcd_attached("GET");
	else if (ndx == SECONDARY_DISPLAY_NDX)
		lcd_id = get_lcd_attached_secondary("GET");

	/*
	 * The 0xFFFFFF is the id for PBA booting
	 * if the id is same with 0xFFFFFF, this function
	 * will return 0
	 */
	return !(lcd_id == PBA_ID);
}

static char *lcd_id;
module_param(lcd_id, charp, S_IRUGO);

char *lcd_id1;
EXPORT_SYMBOL(lcd_id1);
module_param(lcd_id1, charp, S_IRUGO);

/* 2406 Add cell_id from cmdline module param */
static char *cell_id;
module_param(cell_id, charp, S_IRUGO);
static char *cell_id1;
module_param(cell_id1, charp, S_IRUGO);

static int ss_parse_panel_id(char *panel_id)
{
	int id;

	if (IS_ERR_OR_NULL(panel_id))
		return -EINVAL;

	if (kstrtoint(panel_id, 16, &id)) {
		LCD_ERR_NOVDD("invalid panel id(%s)\n", panel_id);
		return -EINVAL;
	}

	return id;
}

/* Return lcd_id that got by cmdline */
int get_lcd_attached(char *mode)
{
	static int _lcd_id = -EINVAL;

	if (mode == NULL) {
		pr_err("[SDE] err: arg is NULL\n");
		return -EINVAL;
	}

	if (!strncmp(mode, "GET", 3)) {
		if (lcd_id)
			_lcd_id = ss_parse_panel_id(lcd_id);
		goto end;
	} else {
		_lcd_id = ss_parse_panel_id(mode);
	}

	pr_info("[SDE] LCD_ID = 0x%X\n", _lcd_id);
end:

	/* case 03830582: Sometimes, bootloader fails to save lcd_id value in cmdline
	 * which is located in TZ.
	 * W/A: if panel name is PBA panel while lcd_id is not PBA ID, force to set lcd_id to PBA ID.
	 */
	if (unlikely(_lcd_id == -EINVAL)) {
		char panel_name[MAX_CMDLINE_PARAM_LEN];
		char panel_string[] = "ss_dsi_panel_PBA_BOOTING_FHD";

		ss_get_primary_panel_name_cmdline(panel_name);
		if (!strncmp(panel_string, panel_name, strlen(panel_string)) && _lcd_id != PBA_ID) {
			pr_info("[SDE] pba panel name, force lcd id: 0x%X -> 0xFFFFFF\n", _lcd_id);
			_lcd_id =  PBA_ID;
		}
	}

	return _lcd_id;
}
EXPORT_SYMBOL(get_lcd_attached);
__setup("lcd_id=0x", get_lcd_attached);

int set_lcd_id_cmdline(struct samsung_display_driver_data *vdd, int id)
{
	char *lcd_id_str;

	if (!vdd) {
		LCD_ERR_NOVDD("invalid vdd\n");
		return -EINVAL;
	}

	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		lcd_id_str = lcd_id;
	else
		lcd_id_str = lcd_id1;

	snprintf(lcd_id_str, 9, "%08x", id);
	LCD_INFO(vdd, "lcd id: %x, str: [%s]\n", id, lcd_id_str);

	return 0;
}

int get_lcd_attached_secondary(char *mode)
{
	static int _lcd_id = -EINVAL;

	pr_debug("[SDE1] %s : %s", __func__, mode);

	if (mode == NULL) {
		pr_err("[SDE1] err: arg is NULL\n");
		return true;
	}

	if (!strncmp(mode, "GET", 3)) {
		if (lcd_id1)
			_lcd_id = ss_parse_panel_id(lcd_id1);
		goto end;
	} else {
		_lcd_id = ss_parse_panel_id(mode);
	}

	pr_info("[SDE1] LCD_ID = 0x%X\n", _lcd_id);
end:

	/* case 03830582: Sometimes, bootloader fails to save lcd_id value in cmdline
	 * which is located in TZ.
	 * W/A: if panel name is PBA panel while lcd_id is not PBA ID, force to set lcd_id to PBA ID.
	 */
	if (unlikely(_lcd_id == -EINVAL)) {
		char panel_name[MAX_CMDLINE_PARAM_LEN];
		char panel_string[] = "ss_dsi_panel_PBA_BOOTING_FHD_DSI1";

		ss_get_secondary_panel_name_cmdline(panel_name);
		if (!strncmp(panel_string, panel_name, strlen(panel_string)) && _lcd_id != PBA_ID) {
			pr_info("[SDE1] pba panel name, force lcd id: 0x%X -> 0xFFFFFF\n", *lcd_id);
			_lcd_id =  PBA_ID;
		}
	}

	return _lcd_id;
}
EXPORT_SYMBOL(get_lcd_attached_secondary);
__setup("lcd_id1=0x", get_lcd_attached_secondary);

int set_cell_id_cmdline(struct samsung_display_driver_data *vdd)
{
	if (!vdd) {
		LCD_ERR_NOVDD("invalid vdd\n");
		return -EINVAL;
	}

	if (vdd->ndx == PRIMARY_DISPLAY_NDX && !cell_id) {
		LCD_INFO(vdd, "cell_id does not exist from cmdline\n");
		return -EINVAL;
	} else	if (vdd->ndx == SECONDARY_DISPLAY_NDX && !cell_id1) {
		LCD_INFO(vdd, "cell_id1 does not exist from cmdline\n");
		return -EINVAL;
	}

	if (!vdd->cell_id_dsi) {
		vdd->cell_id_dsi = kzalloc(23, GFP_KERNEL);//11*2+1 TODO size
		if (!vdd->cell_id_dsi) {
			LCD_ERR(vdd, "fail to kzalloc for cell_id_dsi\n");
			return false;
		}
	}

	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		memcpy(vdd->cell_id_dsi, cell_id, 22); //TODO size
	 else if (vdd->ndx == SECONDARY_DISPLAY_NDX)
		memcpy(vdd->cell_id_dsi, cell_id1, 22); //TODO size

	vdd->cell_id_dsi[22] = 0; //0; /*Last nul char */
	vdd->cell_id_len = 23; //TODO size

	LCD_INFO(vdd, "[%d] cell_id_dsi:%s len:%d\n", vdd->ndx, vdd->cell_id_dsi, vdd->cell_id_len);

	return 0;
}

int get_window_color(char *color)
{
	struct samsung_display_driver_data *vdd;
	int i;

	for (i = PRIMARY_DISPLAY_NDX; i < MAX_DISPLAY_NDX; i++) {
		vdd = ss_get_vdd(i);

		LCD_INFO(vdd, "window_color from cmdline : %s\n", color);
		memcpy(vdd->window_color, color, sizeof(vdd->window_color));
		LCD_INFO(vdd, "window_color : %s\n", vdd->window_color);
	}

	return true;
}
EXPORT_SYMBOL(get_window_color);
__setup("window_color=", get_window_color);

int ss_is_recovery_check(char *str)
{
	struct samsung_display_driver_data *vdd;
	int i;

	for (i = PRIMARY_DISPLAY_NDX; i < MAX_DISPLAY_NDX; i++) {
		vdd = ss_get_vdd(i);
		if (strncmp(str, "1", 1) == 0)
			vdd->is_recovery_mode = 1;
		else
			vdd->is_recovery_mode = 0;
	}
	LCD_INFO(vdd, "vdd->is_recovery_mode = %d\n", vdd->is_recovery_mode);
	return true;
}
EXPORT_SYMBOL(ss_is_recovery_check);

__setup("androidboot.boot_recovery=", ss_is_recovery_check);

void ss_event_frame_update_pre(struct samsung_display_driver_data *vdd)
{
	if (vdd->display_status_dsi.wait_disp_on || vdd->debug_data->print_all || vdd->debug_data->print_frame)
		LCD_INFO(vdd, "COMMIT\n");

	/* mAFPC */
	if (vdd->mafpc.is_support) {
		if (vdd->mafpc.need_to_write) {
			LCD_INFO(vdd, "update mafpc\n");

			if (!IS_ERR_OR_NULL(vdd->mafpc.img_write)) {
				if (vdd->esync_support)
					vdd->mafpc.img_write(vdd, true);
				else
					vdd->mafpc.img_write(vdd, false);
			} else
				LCD_ERR(vdd, "mafpc img_write function is NULL\n");

			if (!IS_ERR_OR_NULL(vdd->mafpc.enable)) {
				if (vdd->mafpc.en)
					vdd->mafpc.enable(vdd, true);
				else
					vdd->mafpc.enable(vdd, false);
			} else
				LCD_ERR(vdd, "mafpc enable function is NULL\n");

			vdd->mafpc.need_to_write = false;
		}
	}

	if (atomic_read(&vdd->block_commit_cnt) > 0) {
		LCD_INFO(vdd, "wait block commit cnt(%d) +++\n", atomic_read(&vdd->block_commit_cnt));

		if (!wait_event_timeout(vdd->block_commit_wq,
				atomic_read(&vdd->block_commit_cnt) == 0,
				msecs_to_jiffies(10000))) {
			LCD_ERR(vdd, "block commit timeout(10 sec), cnt(%d)\n",
					atomic_read(&vdd->block_commit_cnt));
			atomic_set(&vdd->block_commit_cnt, 0);
		}
		LCD_INFO(vdd, "wait block commit cnt(%d) ---\n",
				atomic_read(&vdd->block_commit_cnt));
	}
}

/*
 * ss_delay(delay, from).
 * calcurate needed delay from 'from' kernel time and wait.
 */
void ss_delay(s64 delay, ktime_t from)
{
	s64 wait_ms, past_ms;

	if (!delay || !from)
		return;

	/* past time from 'from' time */
	past_ms = ktime_ms_delta(ktime_get(), from);

	/* determine needed delay */
	wait_ms = delay - past_ms;

	if (wait_ms > 0) {
		pr_info("[SDE] delay:%lld, past:%lld, need wait_ms:%lld [ms] start\n", delay, past_ms, wait_ms);
		usleep_range(wait_ms*1000, wait_ms*1000);
	} else
		pr_info("[SDE] delay:%lld, past:%lld, [ms] skip\n", delay, past_ms);

	return;
}

void ss_event_frame_update_post(struct samsung_display_driver_data *vdd)
{
	struct panel_func *panel_func = NULL;
	static u8 frame_count = 1;

	if (!vdd)
		return;

	mutex_lock(&vdd->display_on_lock);

	panel_func = &vdd->panel_func;

	if (vdd->display_status_dsi.wait_disp_on) {
		/* Skip a number of frames to avoid garbage image output from wakeup */
		if (frame_count <= vdd->dtsi_data.samsung_delayed_display_on) {
			LCD_DEBUG(vdd, "Skip %d frame\n", frame_count);
			frame_count++;
			goto skip_display_on;
		}
		frame_count = 1;

		/* set self_mask_udc before display on */
		if (vdd->self_disp.self_mask_udc_on && !ss_is_panel_lpm(vdd))
			vdd->self_disp.self_mask_udc_on(vdd, vdd->self_disp.udc_mask_enable);
		else
			LCD_DEBUG(vdd, "Self Mask UDC Function is NULL\n");

		/* delay between sleep_out and display_on cmd */
		ss_delay(vdd->dtsi_data.sleep_out_to_on_delay, vdd->sleep_out_time);

		/* insert black frame using self_grid on cmd in case of LPM mode */
		if (vdd->panel_lpm.need_self_grid && ss_is_panel_lpm(vdd)) {
			ss_send_cmd(vdd, TX_SELF_GRID_ON);
			LCD_INFO(vdd, "self_grid [ON]\n");
		}

		ss_send_cmd(vdd, TX_DSI_CMD_SET_ON_POST);

		if (!SS_IS_CMDS_NULL(ss_get_cmds(vdd, TX_DISPLAY_ON)))
			ss_send_cmd(vdd, TX_DISPLAY_ON);

		vdd->display_status_dsi.wait_disp_on = false;

		if (vdd->display_on_post_delay) {
			LCD_INFO(vdd, "Delay after display_on (%d)ms\n", vdd->display_on_post_delay);
			usleep_range(vdd->display_on_post_delay * 1000, vdd->display_on_post_delay * 1000 + 10);
			vdd->display_on_post_delay = 0;
		}

		/* Write clear status bits if first booting for GTS7+/ GTS8+/ GTS8U */
		/* UUM1 ddi restriction for GTS7+/ GTS8+/ GTS8U */
		if (vdd->display_status_dsi.first_commit_disp_on) {
			if (!SS_IS_CMDS_NULL(ss_get_cmds(vdd, TX_FIRST_DISPLAY_ON))) {
				usleep_range(36000, 36100);
				ss_send_cmd(vdd, TX_FIRST_DISPLAY_ON);
			}
			vdd->display_status_dsi.first_commit_disp_on = false;
		}

		if (!SS_IS_CMDS_NULL(ss_get_cmds(vdd, TX_DISPLAY_ON_POST))) {
			LCD_INFO(vdd, "Sending tx disp on post cmd.\n");
			ss_send_cmd(vdd, TX_DISPLAY_ON_POST);
		}

		if (vdd->panel_lpm.need_self_grid && ss_is_panel_lpm(vdd)) {
			/* should distinguish display_on and self_grid_off with vsnyc */
			/* delay 34ms in self_grid_off_revA cmds */
			ss_send_cmd(vdd, TX_SELF_GRID_OFF);
			LCD_INFO(vdd, "self_grid [OFF]\n");
		}

		vdd->panel_lpm.need_self_grid = false;

		vdd->esd_recovery.is_enabled_esd_recovery = true;

		/* If necessary, recover the panel */
		if (vdd->need_panel_recovery) {
			LCD_ERR(vdd, "panel recovery is required.. do recovery!\n");
			ss_panel_recovery(vdd);
		}

		vdd->display_on = true;
		LCD_INFO(vdd, "DISPLAY_ON\n");
		vdd->display_on_time = ktime_to_ms(ktime_get());
		LCD_DEBUG(vdd, "display_on_time (%lld)\n", vdd->display_on_time);
		SDE_EVT32(0xFFFF);
	}

skip_display_on:
	mutex_unlock(&vdd->display_on_lock);

	if (vdd->panel_func.samsung_display_on_post)
		vdd->panel_func.samsung_display_on_post(vdd);

	return;
}

void ss_event_rd_ptr_irq(struct samsung_display_driver_data *vdd)
{
	/* Be careful. This function is called in irq context */

	/* To check whether last finger mask frame kickoff is started */
	if (vdd->support_optical_fingerprint)
		vdd->panel_hbm_exit_frame_wait = false;

	if (atomic_add_unless(&vdd->ss_vsync_cnt, -1, 0) &&
		(atomic_read(&vdd->ss_vsync_cnt) == 0)) {
		wake_up_all(&vdd->ss_vsync_wq);
		LCD_INFO(vdd, "ss_vsync_cnt: %d\n", atomic_read(&vdd->ss_vsync_cnt));
	}

	ss_print_vsync(vdd);
}

/* SAMSUNG_FINGERPRINT */
bool ss_wait_for_te_gpio_high(struct samsung_display_driver_data *vdd, unsigned int disp_te_gpio, bool preemption)
{
	bool rc = 0;
	int te_count = 0;
	s64 start_time, end_time = 0;
	int te_max = 20000; /*sampling 100ms */

	start_time = ktime_to_us(ktime_get());
	for (te_count = 0 ; te_count < te_max ; te_count++) {
		rc = ss_gpio_get_value(vdd, disp_te_gpio);
		if (rc == 1) {
			end_time = ktime_to_us(ktime_get());
			break;
		}
		if (preemption)
			ndelay(5000);
		else
			usleep_range(1, 2);
	}
	LCD_INFO(vdd, "ss_wait_for_te_high %s : %llu\n", rc?"success":"fail", end_time - start_time);

	return rc;
}

bool ss_wait_for_te_gpio_low(struct samsung_display_driver_data *vdd, unsigned int disp_te_gpio, bool preemption)
{
	bool rc = 1;
	int te_count = 0;
	s64 start_time, end_time = 0;
	int te_max = 500; /*sampling 100ms */

	start_time = ktime_to_us(ktime_get());
	for (te_count = 0 ; te_count < te_max ; te_count++) {
		rc = gpio_get_value(disp_te_gpio);
		if (rc == 0) {
			end_time = ktime_to_us(ktime_get());
			break;
		}
		if (preemption)
			udelay(200);
		else
			usleep_range(200, 220);
	}
	LCD_INFO(vdd, "ss_wait_for_te_low %s : %llu\n", (!rc)?"success":"fail", end_time - start_time);

	return !rc;
}

/* TBR: use vsync or TE gpio interrupt instead of polling and preventing preemption */
void ss_wait_for_te_gpio(struct samsung_display_driver_data *vdd, int num_of_te, int delay_after_te, bool preemption)
{
	unsigned int disp_te_gpio;
	int iter;

	if (preemption)
		preempt_disable();

	/* If you need correct TE signal such as Finger Brightness case, you should use preemption & delay.
	 * But, you should know that preept_disable & delay can cause Audio problem (P201222-01434, P211129-05047)
	 */
	disp_te_gpio = ss_get_te_gpio(vdd);
	if (!ss_gpio_is_valid(disp_te_gpio)) {
		LCD_INFO(vdd, "No disp_te_gpio gpio..\n");
		return;
	}
	LCD_INFO(vdd, "ss_wait_for_te_gpio %d times, delay_after_te %d us\n", num_of_te, delay_after_te);

	for (iter = 0 ; iter < num_of_te ; iter++) {
		/*1. check high te gpio value. if return true, 2. wait for low gpio value*/
		if (ss_wait_for_te_gpio_high(vdd, disp_te_gpio, preemption)
				&& iter < (num_of_te - 1))
			ss_wait_for_te_gpio_low(vdd, disp_te_gpio, preemption);
	}

	if (delay_after_te) {
		if (delay_after_te < 5000)
			udelay(delay_after_te);
		else {
			mdelay(delay_after_te / 1000);
			udelay(delay_after_te % 1000);
		}
	}

	if (preemption)
		preempt_enable();
}

void ss_wait_for_vsync(struct samsung_display_driver_data *vdd,
		int num_of_vsnc, int delay_after_vsync)
{
	int i = 0, ret = 0;
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct drm_encoder *drm_enc = GET_DRM_ENCODER(vdd);
	struct sde_encoder_virt *sde_enc = NULL;
	s64 start_time, end_time;

	/* Skip before display on */
	if (vdd->display_status_dsi.wait_disp_on) {
		LCD_INFO(vdd, "skip waiting, display is not turned on yet\n");

		/* TBR: Delay may be removed  */
		if (panel->panel_mode == DSI_OP_VIDEO_MODE)
			usleep_range(10000, 10005);
		return;
	}

	sde_enc = to_sde_encoder_virt(drm_enc);

	for (i = 0; i < sde_enc->num_phys_encs; i++) {
		struct sde_encoder_phys *phys = sde_enc->phys_encs[i];
		phys->ops.control_vblank_irq(phys, true);
	}

	start_time = ktime_to_us(ktime_get()); /* us */
	atomic_set(&vdd->ss_vsync_cnt, num_of_vsnc);

	LCD_INFO(vdd, "+++ vsnyc_cnt:%d\n", atomic_read(&vdd->ss_vsync_cnt));
	ret = wait_event_timeout(vdd->ss_vsync_wq, atomic_read(&vdd->ss_vsync_cnt) == 0,
			msecs_to_jiffies(200));
	if (!ret)
		LCD_ERR(vdd, "Wait vsync timeout\n");

	for (i = 0; i < sde_enc->num_phys_encs; i++) {
		struct sde_encoder_phys *phys = sde_enc->phys_encs[i];
		phys->ops.control_vblank_irq(phys, false);
	}

	if (atomic_read(&vdd->ss_vsync_cnt) == num_of_vsnc)
		atomic_set(&vdd->ss_vsync_cnt, 0);

	if (in_atomic()) {
		if (delay_after_vsync < 5000) {
			udelay(delay_after_vsync);
		} else {
			mdelay(delay_after_vsync / 1000);
			udelay(delay_after_vsync % 1000);
		}
	} else {
		usleep_range(delay_after_vsync, delay_after_vsync + 5);
	}

	end_time = ktime_to_us(ktime_get());
	LCD_INFO(vdd, "--- waited %llu us (include %d delay)\n",
		end_time-start_time, delay_after_vsync);
}

int ss_get_pending_kickoff_cnt(struct samsung_display_driver_data *vdd)
{
	struct drm_encoder *drm_enc = GET_DRM_ENCODER(vdd);
	struct sde_encoder_virt *sde_enc = NULL;
	int pending_cnt = 0;
	int i;

	if (!drm_enc) {
		LCD_ERR(vdd, "invalid encoder\n");
		return -EINVAL;
	}

	sde_enc = to_sde_encoder_virt(drm_enc);

	for (i = 0; i < sde_enc->num_phys_encs; i++)
		pending_cnt += sde_encoder_wait_for_event(drm_enc, MSM_ENC_TX_COMPLETE);

	return pending_cnt;
}

void ss_wait_for_kickoff_done(struct samsung_display_driver_data *vdd)
{
	int wait_max_cnt = 500;
	while (ss_get_pending_kickoff_cnt(vdd)) {
		usleep_range(200, 220);
		if (--wait_max_cnt == 0) {
			LCD_ERR(vdd, "pending_kickoff_cnt[%d] did not become 0\n", ss_get_pending_kickoff_cnt(vdd));
			break;
		}
	}

	return;
}

/* SAMSUNG_FINGERPRINT */
void ss_send_hbm_fingermask_image_tx(struct samsung_display_driver_data *vdd, bool on)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	LCD_INFO(vdd, "++ %s\n", on ? "on" : "off");
	if (on) {
		ss_brightness_dcs(vdd, 0, BACKLIGHT_FINGERMASK_ON);
	} else {
		if ((panel->panel_mode == DSI_OP_CMD_MODE) || !(vdd->support_optical_fingerprint))
			ss_brightness_dcs(vdd, 0, BACKLIGHT_FINGERMASK_OFF);
		else /* If video mode, opt, fingermask off */
			LCD_INFO(vdd, "skip to call brightness ft in video mode && mask off\n");
			/* Calling br moved to _sde_encoder_trigger_flush */
	}
	LCD_INFO(vdd, "--\n");
}

/* Identify VRR HS by drm_mode's name.
 * drm_mode's name is defined by dsi_mode->timing.sot_hs_mode parsed
 * from samsung,mdss-dsi-sot-hs-mode in panel dtsi file.
 * ex) drm_mode->name is "1080x2316x60x193345cmdHS" for HS mode.
 *     drm_mode->name is "1080x2316x60x193345cmdNS" for NS mode.
 * To use this feature, declare different porch between HS and NS modes,
 * in panel dtsi file.
 */
bool ss_is_sot_hs_from_drm_mode(const struct drm_display_mode *drm_mode)
{
	int len;
	bool sot_hs = false;

	if (!drm_mode) {
		pr_err("invalid drm_mode\n");
		goto end;
	}

	len = strlen(drm_mode->name);
	if (len >= DRM_DISPLAY_MODE_LEN || len <= 0) {
		pr_err("invalid drm_mode name len (%d)\n", len);
		goto end;
	}

	if (!strncmp(drm_mode->name + len - 2, "HS", 2))
		sot_hs = true;
	else
		sot_hs = false;

end:
	return sot_hs;
}

bool ss_is_phs_from_drm_mode(const struct drm_display_mode *drm_mode)
{
	int len;
	bool phs = false;

	if (!drm_mode) {
		pr_err("invalid drm_mode\n");
		goto end;
	}

	len = strlen(drm_mode->name);
	if (len >= DRM_DISPLAY_MODE_LEN || len <= 0) {
		pr_err("invalid drm_mode name len (%d)\n", len);
		goto end;
	}

	if (!strncmp(drm_mode->name + len - 3, "PHS", 3))
		phs = true;
	else
		phs = false;

end:
	return phs;
}

int ss_vrr_apply_dsi_bridge_mode_fixup(struct dsi_display *display,
				struct drm_display_mode *cur_mode,
				struct dsi_display_mode cur_dsi_mode,
				struct dsi_display_mode *adj_mode)
{
	struct samsung_display_driver_data *vdd = display->panel->panel_private;
	struct vrr_info *vrr = &vdd->vrr;
	bool adjusted_sot_hs, adjusted_phs;
	bool cur_sot_hs, cur_phs;
	int rc = 0;

	if (!(display->panel->panel_initialized || display->is_cont_splash_enabled))
		return 0;

	vrr->adjusted_refresh_rate = adj_mode->timing.refresh_rate;
	cur_sot_hs = ss_is_sot_hs_from_drm_mode(cur_mode);
	cur_phs = ss_is_phs_from_drm_mode(cur_mode);
	adjusted_sot_hs = adj_mode->timing.sot_hs_mode;
	adjusted_phs = adj_mode->timing.phs_mode;

	vrr->adjusted_sot_hs_mode = adjusted_sot_hs;
	vrr->adjusted_phs_mode = adjusted_phs;

	vrr->cur_h_active = cur_mode->hdisplay;
	vrr->cur_v_active = cur_mode->vdisplay;
	vrr->adjusted_h_active = adj_mode->timing.h_active;
	vrr->adjusted_v_active = adj_mode->timing.v_active;

	/* vrr->cur_refresh_rate valuse is changed in Bridge RR,
	 * so use cur_mode info.
	 */
	if ((drm_mode_vrefresh(cur_mode) != adj_mode->timing.refresh_rate) ||
			(cur_sot_hs != adjusted_sot_hs) ||
			(cur_phs != adjusted_phs)) {
		LCD_DEBUG(vdd, "DMS: VRR flag: %d -> 1\n", vrr->is_vrr_changing);
		vrr->is_vrr_changing = true;
		vrr->keep_max_clk = true;
		vdd->vrr.running_vrr_mdp = true;

		/* Set max sde core clock to prevent screen noise due to
		 * unbalanced clock between MDP and panel
		 * SDE core clock will be restored in ss_panel_vrr_switch()
		 * after finish VRR change.
		 */

		/* If there is a pending SDE normal clk work, cancel that first */
		cancel_delayed_work(&vdd->sde_normal_clk_work);
		rc = ss_set_max_sde_core_clk(display->drm_dev);
		if (rc) {
			LCD_ERR(vdd, "fail to set max sde core clock..(%d)\n", rc);
			SS_XLOG(rc, 0xebad);
		}
	}

	if ((cur_mode->hdisplay != adj_mode->timing.h_active) ||
			(cur_mode->vdisplay != adj_mode->timing.v_active)) {
		LCD_INFO(vdd, "DMS: MULTI RES flag: %d -> 1\n",
				vrr->is_multi_resolution_changing);
		vrr->is_multi_resolution_changing = true;
	}

	adj_mode->dsi_mode_flags |= DSI_MODE_FLAG_DMS;

	SS_XLOG(drm_mode_vrefresh(cur_mode), cur_sot_hs, cur_phs,
			adj_mode->timing.refresh_rate, adjusted_sot_hs);

	LCD_INFO(vdd, "DMS: switch mode %s(%dx%d@%d%s) -> (%dx%d@%d%s)\n",
		cur_mode->name,
		cur_mode->hdisplay,
		cur_mode->vdisplay,
		drm_mode_vrefresh(cur_mode),
		cur_sot_hs ? (cur_phs ? "PHS" : "HS") : "NS",
		adj_mode->timing.h_active,
		adj_mode->timing.v_active,
		adj_mode->timing.refresh_rate,
		adjusted_sot_hs ? (adjusted_phs ? "PHS" : "HS") : "NS");

	return rc;
}

int ss_vrr_save_dsi_bridge_mode_fixup(struct dsi_display *display,
				struct drm_display_mode *cur_mode,
				struct dsi_display_mode cur_dsi_mode,
				struct dsi_display_mode *adj_mode,
				struct drm_crtc_state *crtc_state)
{
	/* In case of that
	 * - display power state is changing,
	 * - splash is enabled yet, or
	 * - VRR, POMS, or DYN_CLK is set,
	 * it will apply display_mode in dsi_display_mode() function without set DMS flag.
	 *
	 * But, Samsung VRR should apply target VRR mode in vrr->cur_refresh_rate.
	 * Brightness setting will apply current VRR mode, and apply it to UB.
	 * So, in this corner case, just save target VRR mode in vrr->cur_refresh_rate.
	 *
	 * Even it is only multi resolution scenario, not VRR scenario,
	 * it should save resolution for VRR, and it is harmless to save
	 * current and target refresh rate to intended refresh rate.
	 */
	struct samsung_display_driver_data *vdd = display->panel->panel_private;
	struct vrr_info *vrr = &vdd->vrr;
	bool adjusted_sot_hs, adjusted_phs;
	bool cur_sot_hs, cur_phs;

	cur_sot_hs = ss_is_sot_hs_from_drm_mode(cur_mode);
	cur_phs = ss_is_phs_from_drm_mode(cur_mode);
	adjusted_sot_hs = adj_mode->timing.sot_hs_mode;
	adjusted_phs = adj_mode->timing.phs_mode;

	vrr->cur_refresh_rate = vrr->adjusted_refresh_rate =
		adj_mode->timing.refresh_rate;
	vrr->cur_sot_hs_mode = vrr->adjusted_sot_hs_mode =
		adjusted_sot_hs;
	vrr->cur_phs_mode = vrr->adjusted_phs_mode =
		adjusted_phs;

	vrr->cur_h_active = vrr->adjusted_h_active =
		adj_mode->timing.h_active;
	vrr->cur_v_active  = vrr->adjusted_v_active =
		adj_mode->timing.v_active;

	SS_XLOG(drm_mode_vrefresh(cur_mode), cur_sot_hs, cur_phs,
			adj_mode->timing.refresh_rate, adjusted_sot_hs,
			crtc_state->active_changed, display->is_cont_splash_enabled);

	LCD_INFO(vdd, "DMS: switch mode %s(%dx%d@%d%s) -> (%dx%d@%d%s) " \
			"during active_changed(%d) or splash(%d)\n",
			cur_mode->name,
			cur_mode->hdisplay,
			cur_mode->vdisplay,
			drm_mode_vrefresh(cur_mode),
			cur_sot_hs ? (cur_phs ? "PHS" : "HS") : "NM",
			adj_mode->timing.h_active,
			adj_mode->timing.v_active,
			adj_mode->timing.refresh_rate,
			adjusted_sot_hs ? (adjusted_phs ? "PHS" : "HS") : "NM",
			crtc_state->active_changed,
			display->is_cont_splash_enabled);

	return 0;
}

/*
 * Determine LFD min and max freq by client's request, and priority of each clients.
 *
 * FACTORY	: factory test scenario.
 *		  determine min and max freq., priority = critical
 * TSP LPM	: tsp lpm mode scenario.
 * 		  determine min freq., priority = high
 * INPUT	: touch press scenario.
 * 		  determine min freq., priority = high
 * DISPLAY MANAGER : HBM (1hz) and low brighntess (LFD off) scenario.
 * 		     determine min freq., priority = high
 * VIDEO DETECT	: video detect scenario.
 * 		  determine max freq., priority = high
 * AOD		: AOD scenario.
 * 		  determine min freq. in lpm: handle thi in lpm brightenss, not here.
 *
 * ss_get_lfd_div should be called followed by ss_update_base_lfd_val in panel function....
 * or keep it in ss_vrr...
 */
int ss_get_lfd_div(struct samsung_display_driver_data *vdd,
		enum LFD_SCOPE_ID scope, struct lfd_div_info *div_info)
{
	struct vrr_info *vrr = &vdd->vrr;
	struct lfd_mngr *mngr;
	u32 max_div, min_div, fix, max_div_def, min_div_def, min_div_lowest, fix_div_def, fix_low_div_def, highdot_div_def;
	u32 min_div_clear;
	u32 min_div_scal;
	int i;
	u32 max_freq, min_freq;
	u32 base_rr;
	struct lfd_base_str lfd_base;

	/* DEFAULT */
	if (IS_ERR_OR_NULL(vdd->panel_func.samsung_lfd_get_base_val)) {
		LCD_ERR(vdd, "fail to get lfd bsae val..\n");
		return -ENODEV;
	}

	vdd->panel_func.samsung_lfd_get_base_val(vrr, scope, &lfd_base);

	base_rr = lfd_base.base_rr;
	LCD_DEBUG(vdd, "base_rr(%d)\n", lfd_base.base_rr);
	max_div_def = lfd_base.max_div_def;
	min_div_def = lfd_base.min_div_def;
	min_div_lowest = lfd_base.min_div_lowest;
	min_div_clear = min_div_lowest + 1;
	fix_div_def = lfd_base.fix_div_def;
	highdot_div_def = lfd_base.highdot_div_def;
	fix_low_div_def = lfd_base.fix_low_div_def;

	/* FIX */
	for (i = 0, mngr = &vrr->lfd.lfd_mngr[i]; i < LFD_CLIENT_MAX; i++, mngr++) {
		fix = mngr->fix[scope];
		if (mngr->fix[scope] == LFD_FUNC_FIX_LFD) {
			max_div = min_div = fix_div_def;
			LCD_DEBUG(vdd, "FIX LFD. fix_div_def(%d)\n", fix_div_def);
			goto set_out_div;
		} else if (mngr->fix[scope] == LFD_FUNC_FIX_HIGHDOT &&
				i == LFD_CLIENT_FAC) {
			max_div = min_div = highdot_div_def;
			LCD_INFO(vdd, "FIX 0.5 LFD for HIGHDOT test (FAC). div(%d)\n", highdot_div_def);
			goto set_out_div;
		} else if (mngr->fix[scope] == LFD_FUNC_FIX_LOW &&
				i == LFD_CLIENT_FAC) {
			if (fix_low_div_def)
				max_div = min_div = fix_low_div_def;
			else
				max_div = min_div = min_div_def;
			LCD_DEBUG(vdd, "FIX: (FAC): fix low\n");
			goto done;
		} else if (mngr->fix[scope] == LFD_FUNC_FIX_HIGH) {
			max_div = min_div = max_div_def;
			LCD_DEBUG(vdd, "FIX: LFD off, client: %s\n", lfd_client_name[i]);
			goto done;
		}
	}

	/* MAX and MIN */
	max_freq = 0;
	min_freq = 0;
	for (i = 0, mngr = &vrr->lfd.lfd_mngr[i]; i < LFD_CLIENT_MAX; i++, mngr++) {
		if (mngr->max[scope] != LFD_FUNC_MAX_CLEAR) {
			max_freq = max(max_freq, mngr->max[scope]);
			LCD_DEBUG(vdd, "client: %s, max freq=%dhz -> %dhz\n",
				lfd_client_name[i], mngr->max[scope], max_freq);
		}

		if (mngr->min[scope] != LFD_FUNC_MIN_CLEAR) {
			min_freq = max(min_freq, mngr->min[scope]);
			LCD_DEBUG(vdd, "client: %s, min freq=%dhz -> %dhz\n",
				lfd_client_name[i], mngr->min[scope], min_freq);
		}
	}
	if (max_freq == 0)
		max_div = max_div_def;
	else
		max_div = base_rr / max_freq; // round down in case of freq -> div

	if (min_freq == 0) {
		min_div = min_div_clear; // choose min_div_clear instead of min_div_def to allow min_div_lowest below sequence..
	} else {
		min_div = base_rr / min_freq; // round down in case of freq -> div
		min_div = min(min_div, min_div_def);
	}

	max_div = min(min_div, max_div);

	/* SCALABILITY: handle min_freq.
	 * This function is depends on panel. So use panel callback function
	 * if below fomula doesn't fit for your panel.
	 */
	for (i = 0, mngr = &vrr->lfd.lfd_mngr[i]; i < LFD_CLIENT_MAX; i++, mngr++) {
		/* TODO: get this panel specific data from lfd->... parsed by panel dtsi */
		if (mngr->scalability[scope] == LFD_FUNC_SCALABILITY1) /* under 40lux, LFD off */
			min_div_scal = 1;
		else if (mngr->scalability[scope] == LFD_FUNC_SCALABILITY2) /* touch press/release, div=2 */
		      min_div_scal = 2;
		else if (mngr->scalability[scope] == LFD_FUNC_SCALABILITY3) /* TBD, div=4 */
		      min_div_scal = 4;
		else if (mngr->scalability[scope] == LFD_FUNC_SCALABILITY4) /* TBD, div=5 */
		      min_div_scal = 5;
		else if (mngr->scalability[scope] == LFD_FUNC_SCALABILITY6) /* over 7400lux (HBM), LFD min 1hz */
			min_div_scal = min_div_lowest;
		else /* clear */
			min_div_scal = min_div_clear;

		if (mngr->scalability[scope] != LFD_FUNC_SCALABILITY0 &&
				mngr->scalability[scope] != LFD_FUNC_SCALABILITY5)
			LCD_DEBUG(vdd, "client: %s, scalability: %d\n",
					lfd_client_name[i], mngr->scalability[scope]);

		min_div = min(min_div, min_div_scal);
	}

	if (min_div == min_div_clear)
		min_div = min_div_def;

done:
	/* check limitation
	 * High LFD frequency means stable display quality.
	 * Low LFD frequency means low power consumption.
	 * Those are trade-off, and if there are multiple requests of LFD min/max frequency setting,
	 * choose most highest LFD freqency, means most lowest  for display quality.
	 */
	if (max_div > min_div_def) {
		LCD_DEBUG(vdd, "raise too low max freq %dhz (max_div=%d->%d)\n",
				DIV_ROUND_UP(base_rr, max_div),
				max_div, min_div_def);
		max_div = min_div_def;
	}

	if (max_div < max_div_def) {
		LCD_DEBUG(vdd, "lower too high max freq %dhz (max_div=%d->%d)\n",
				DIV_ROUND_UP(base_rr, max_div),
				max_div, max_div_def);
		max_div = max_div_def;
	}

	if (min_div > min_div_def && min_div != min_div_lowest) {
		LCD_DEBUG(vdd, "raise too low min freq %dhz (min_div=%d->%d)\n",
				DIV_ROUND_UP(base_rr, min_div),
				min_div, min_div_def);
		min_div = min_div_def;
	}

	if (min_div < max_div_def) {
		LCD_DEBUG(vdd, "lower too high min freq %dhz (min_div=%d->%d)\n",
				DIV_ROUND_UP(base_rr, min_div),
				min_div, max_div_def);
		min_div = max_div_def;
	}

	if (min_div < max_div) {
		LCD_ERR(vdd, "raise max freq(%dhz), lower than min freq(%dhz) (max_div=%d->%d)\n",
				DIV_ROUND_UP(base_rr, max_div),
				DIV_ROUND_UP(base_rr, min_div),
				max_div, min_div);
		max_div = min_div;
	}

	/* VRR 96HS, 48HS (base_rr = 96): Allowed LFD frequencies.
	 * - min_div_lowest: 1hz (div=96)
	 * - min_div_def: 1hz (div=96), 12hz (div=8) or 10.67hz (div=9), depends on panels.
	 * - 48hz(div=2)
	 * - max_div_def : 96hz(div=1) for VRR 96HS. 48hz(div=2) for VRR 48HS
	 */
	if (base_rr == 96) {
		if (max_div > min_div_def && max_div < min_div_lowest) {
			/* min_div_def(div=8, 12hz) < div < min_div_loewst(div=96, 1hz) */
			/* min_div_def(div=96, 1hz) < div < min_div_loewst(div=96, 1hz) */
			LCD_INFO(vdd, "limit LFD max div: %d -> %d\n", max_div, min_div_def);
			max_div = min_div_def; /* 12hz */
		} else if (max_div > 2 && max_div < min_div_def) {
			/* 2(48hz) < div < min_div_def(div=8, 12hz) */
			/* 2(48hz) < div < min_div_def(div=96, 1hz) */
			LCD_INFO(vdd, "limit LFD max div: %d -> 2\n", max_div);
			max_div = 2; /* 48hz */
		}

		if (min_div > min_div_def && min_div < min_div_lowest) {
			/* min_div_def(div=8, 12hz) < div < min_div_loewst(div=96, 1hz) */
			/* min_div_def(div=96, 1hz) < div < min_div_loewst(div=96, 1hz) */
			LCD_INFO(vdd, "limit LFD min div: %d -> %d\n", max_div, min_div_def);
			min_div = min_div_def; /* 12hz */
		} else if (min_div > 2 && min_div < min_div_def) {
			/* 2(48hz) < div < min_div_def(div=8, 12hz) */
			/* 2(48hz) < div < min_div_def(div=96, 1hz) */
			LCD_INFO(vdd, "limit LFD min div: %d -> 2\n", min_div);
			min_div = 2; /* 48hz */
		}
	}

set_out_div:
	div_info->min_div = min_div;
	div_info->max_div = max_div;
	div_info->fix = fix;

	LCD_DEBUG(vdd, "LFD: cur: %dHZ@%s, base_rr: %uhz, def: %uhz(%u)~%uhz(%u), " \
			"lowest: %uhz(%u), hightdot: %u, result: %u.%.1uhz(%u)~%u.%.1uhz(%u), fix(%d)\n",
			vrr->cur_refresh_rate,
			vrr->cur_sot_hs_mode ? "HS" : "NS",
			base_rr,
			DIV_ROUND_UP(base_rr, min_div_def), min_div_def,
			DIV_ROUND_UP(base_rr, max_div_def), max_div_def,
			DIV_ROUND_UP(base_rr, min_div_lowest), min_div_lowest,
			highdot_div_def,
			GET_LFD_INT_PART(vrr->lfd.base_rr, min_div), GET_LFD_DEC_PART(vrr->lfd.base_rr, min_div),
			min_div,
			GET_LFD_INT_PART(vrr->lfd.base_rr, max_div), GET_LFD_DEC_PART(vrr->lfd.base_rr, max_div),
			max_div, fix);

	return 0;
}

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
static int ss_esd_touch_notifier_cb(struct notifier_block *nb,
				unsigned long val, void *v)
{
	struct sde_connector *conn;
	struct samsung_display_driver_data *vdd;
	struct dsi_panel *panel = NULL;
	enum sec_input_notify_t event = (enum sec_input_notify_t)val;
	struct sec_input_notify_data *tsp_ndx = v;

	if (event != NOTIFIER_TSP_ESD_INTERRUPT)
		goto done;

	if (!tsp_ndx) {
		vdd = ss_get_vdd(PRIMARY_DISPLAY_NDX);
		LCD_INFO(vdd, "tsp_ndx is null... Primary NDX\n");
	} else
		vdd = ss_get_vdd(tsp_ndx->dual_policy);

	conn = GET_SDE_CONNECTOR(vdd);

	if (!vdd->esd_recovery.is_enabled_esd_recovery) {
		LCD_ERR(vdd, "esd recovery is not enabled yet");
		goto done;
	}

	if (vdd->is_factory_mode)
		LCD_INFO(vdd, "ub connected: %d\n", ss_is_ub_connected(vdd));

	LCD_INFO(vdd, "++\n");

	vdd->panel_lpm.esd_recovery = true; //??

	panel = (struct dsi_panel *)vdd->msm_private;
	panel->esd_config.status_mode = ESD_MODE_PANEL_IRQ;

	schedule_work(&conn->status_work.work);

	LCD_INFO(vdd, "Panel Recovery ESD, nofity val:%lu, Recovery Count = %d\n",
			val, vdd->panel_recovery_cnt++);
	SS_XLOG(vdd->panel_recovery_cnt);
	inc_dpui_u32_field(DPUI_KEY_QCT_RCV_CNT, 1);

	LCD_INFO(vdd, "--\n");

done:
	return NOTIFY_DONE;

}
#endif

static int ss_reboot_notifier_cb(struct notifier_block *nb,
				unsigned long val, void *v)
{
	struct samsung_display_driver_data *vdd = container_of(nb, struct samsung_display_driver_data, nb_reboot);
	struct dsi_panel *panel = NULL;


	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("[SDE] %s : Failed to get vdd!!\n", __func__);
		return NOTIFY_DONE;
	}

	panel = GET_DSI_PANEL(vdd);

	ss_sde_vm_lock(vdd, true);
	if (!ss_sde_vm_owns_hw(vdd)) {
		LCD_INFO(vdd, "Skip reboot notifier due to HW unavailablity in TVM\n");
		ss_sde_vm_lock(vdd, false);
		return NOTIFY_DONE;
	}
	ss_sde_vm_lock(vdd, false);

	LCD_INFO(vdd, "++\n");

	if (panel->panel_initialized) {
		LCD_INFO(vdd, "disable esd interrupt\n");
		if (vdd->esd_recovery.esd_irq_enable)
			vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd, ESD_MASK_REBOOT_CB);

		LCD_INFO(vdd, "tx ss_off_cmd\n");
		ss_send_cmd(vdd, TX_DSI_CMD_SET_OFF);

		ss_set_panel_state(vdd, PANEL_PWR_OFF);

		LCD_INFO(vdd, "turn off dipslay power\n");

		/* To off the EL power when display is detached. */
		ss_panel_power_force_disable(vdd, &vdd->panel_powers[PANEL_POWERS_EVLDD_ELVSS]);
		mdelay(200);
		ss_panel_power_off_pre_lp11(vdd);
		ss_panel_power_off_post_lp11(vdd);
	} else {
		LCD_INFO(vdd, "Panel is already turned off, samsung_splash %d\n", vdd->samsung_splash_enabled);

		if (vdd->samsung_splash_enabled) {
			/* To off the EL power when display is detached. */
			ss_panel_power_force_disable(vdd, &vdd->panel_powers[PANEL_POWERS_EVLDD_ELVSS]);
			mdelay(200);
			ss_panel_power_off_pre_lp11(vdd);
			ss_panel_power_off_post_lp11(vdd);
		}
	}

	LCD_INFO(vdd, "--\n");

	return NOTIFY_DONE;

}

#if IS_ENABLED(CONFIG_DEV_RIL_BRIDGE) || IS_ENABLED(CONFIG_UML)
static int ss_find_dyn_mipi_clk_timing_idx(struct samsung_display_driver_data *vdd)
{
	int idx = -EINVAL;
	int loop;
	int rat, band, arfcn;
	struct clk_sel_table sel_table = vdd->dyn_mipi_clk.clk_sel_table;

	if (!sel_table.tab_size) {
		LCD_ERR(vdd, "Table is NULL");
		return -ENOENT;
	}

	rat = vdd->dyn_mipi_clk.rf_info.rat;
	band = vdd->dyn_mipi_clk.rf_info.band;
	arfcn = vdd->dyn_mipi_clk.rf_info.arfcn;

	for (loop = 0 ; loop < sel_table.tab_size ; loop++) {
		if ((rat == sel_table.rat[loop]) && (band == sel_table.band[loop])) {
			if ((arfcn >= sel_table.from[loop]) && (arfcn <= sel_table.end[loop])) {
				idx = sel_table.target_clk_idx[loop];
				break;
			}
		}
	}

	if (vdd->dyn_mipi_clk.force_idx >= 0 &&
		vdd->dyn_mipi_clk.force_idx < sel_table.tab_size) {
		idx = vdd->dyn_mipi_clk.force_idx;
		LCD_INFO(vdd, "use force index = %d\n", idx);
	} else {
		LCD_INFO(vdd, "RAT(%d), BAND(%d), ARFCN(%d), Clock Index(%d)\n",
				rat, band, arfcn, idx);
	}

	return idx;
}

static int ss_find_dyn_mipi_clk_timing_osc_idx(struct samsung_display_driver_data *vdd)
{
	int idx = -EINVAL;
	int loop;
	int rat, band, arfcn;
	struct clk_sel_table sel_table = vdd->dyn_mipi_clk.clk_sel_table;

	if (!sel_table.tab_size) {
		LCD_ERR(vdd, "Table is NULL");
		return -ENOENT;
	}

	rat = vdd->dyn_mipi_clk.rf_info.rat;
	band = vdd->dyn_mipi_clk.rf_info.band;
	arfcn = vdd->dyn_mipi_clk.rf_info.arfcn;

	for (loop = 0 ; loop < sel_table.tab_size ; loop++) {
		if ((rat == sel_table.rat[loop]) && (band == sel_table.band[loop])) {
			if ((arfcn >= sel_table.from[loop]) && (arfcn <= sel_table.end[loop])) {
				idx = sel_table.target_osc_idx[loop];
				break;
			}
		}
	}

	LCD_INFO(vdd, "RAT(%d), BAND(%d), ARFCN(%d), OSC Index(%d)\n", rat, band, arfcn, idx);

	return idx;
}

/* CP notity format (HEX raw format)
 * 10 00 AA BB 27 01 03 XX YY YY YY YY ZZ ZZ ZZ ZZ
 *
 * 00 10 (0x0010) - len
 * AA BB - not used
 * 27 - MAIN CMD (SYSTEM CMD : 0x27)
 * 01 - SUB CMD (CP Channel Info : 0x01)
 * 03 - NOTI CMD (0x03)
 * XX - RAT MODE
 * YY YY YY YY - BAND MODE
 * ZZ ZZ ZZ ZZ - FREQ INFO
 */

int ss_rf_info_notify_callback(struct notifier_block *nb,
				unsigned long size, void *data)
{
	struct dyn_mipi_clk *dyn_mipi_clk = container_of(nb, struct dyn_mipi_clk, notifier);
	struct samsung_display_driver_data *vdd =
		container_of(dyn_mipi_clk, struct samsung_display_driver_data, dyn_mipi_clk);
	int ret = NOTIFY_DONE;
#if IS_ENABLED(CONFIG_UML)
	int dev_id = DISPLAY_TEST_CP_CHANNEL_INFO;
	struct display_uml_bridge_msg *msg;
	struct rf_info rf_info_backup;
	msg = (struct display_uml_bridge_msg *)data;
#else
	int dev_id = IPC_SYSTEM_CP_CHANNEL_INFO;
	struct dev_ril_bridge_msg *msg;
	struct rf_info rf_info_backup;
	msg = (struct dev_ril_bridge_msg *)data;
#endif

	LCD_INFO(vdd, "RIL noti: ndx: %d, size: %lu, dev_id: %d, len: %d\n",
			vdd->ndx, size, msg->dev_id, msg->data_len);

	if (msg->dev_id == dev_id // #define IPC_SYSTEM_CP_CHANNEL_INFO	0x01
			&& msg->data_len == sizeof(struct rf_info)) {

		mutex_lock(&dyn_mipi_clk->dyn_mipi_lock);

		/* backup currrent rf_info */
		memcpy(&rf_info_backup, &dyn_mipi_clk->rf_info, sizeof(struct rf_info));
		memcpy(&dyn_mipi_clk->rf_info, msg->data, sizeof(struct rf_info));

		/* check & update*/
		if (ss_change_dyn_mipi_clk_timing(vdd) < 0) {
			LCD_WARN(vdd, "Failed to update MIPI clock timing, restore previous rf_info..\n");
			/* restore origin data */
			memcpy(&dyn_mipi_clk->rf_info, &rf_info_backup, sizeof(struct rf_info));
			ret = NOTIFY_BAD;
		}

		mutex_unlock(&dyn_mipi_clk->dyn_mipi_lock);

		LCD_INFO(vdd, "RIL noti: RAT(%d), BAND(%d), ARFCN(%d)\n",
				dyn_mipi_clk->rf_info.rat,
				dyn_mipi_clk->rf_info.band,
				dyn_mipi_clk->rf_info.arfcn);
	}

	return ret;
}

int ss_change_dyn_mipi_clk_timing(struct samsung_display_driver_data *vdd)
{
	struct clk_timing_table timing_table = vdd->dyn_mipi_clk.clk_timing_table;
	int idx;
	int osc_idx;
	int clk_rate;

	if (!vdd->dyn_mipi_clk.is_support) {
		LCD_ERR(vdd, "Dynamic MIPI Clock does not support\n");
		return -ENODEV;
	}

	if (!timing_table.tab_size) {
		LCD_ERR(vdd, "Table is NULL");
		return -ENODEV;
	}

	LCD_INFO(vdd, "+++");

	idx = ss_find_dyn_mipi_clk_timing_idx(vdd);
	if (idx < 0) {
		LCD_WARN(vdd, "no matched MIPI clock timing (%d)\n", idx);
	} else {
		clk_rate = timing_table.clk_rate[idx];
		LCD_INFO(vdd, "clk idx: %d, clk_rate: %d\n", idx, clk_rate);

		/* Set requested clk rate.
		 * This requested clk will be applied in dsi_display_pre_kickoff.
		 */
		vdd->dyn_mipi_clk.requested_clk_rate = clk_rate;
		vdd->dyn_mipi_clk.requested_clk_idx = idx;
	}

	if (vdd->dyn_mipi_clk.osc_support) {
		osc_idx = ss_find_dyn_mipi_clk_timing_osc_idx(vdd);
		if (osc_idx < 0) {
			LCD_WARN(vdd, "no matched OSC Index (%d)\n", osc_idx);
		} else {
			vdd->dyn_mipi_clk.requested_osc_idx = osc_idx;
			LCD_INFO(vdd, "osc idx: %d\n", osc_idx);
		}
	}

	LCD_INFO(vdd, "---");

	return idx;
}

int ss_dyn_mipi_clk_tx_ffc(struct samsung_display_driver_data *vdd)
{
	int ret = 0;

	if (!vdd->dyn_mipi_clk.is_support)
		return ret;

	LCD_INFO(vdd, "Clk idx: %d, tx FFC\n", vdd->dyn_mipi_clk.requested_clk_idx);
	ss_send_cmd(vdd, TX_FFC);

	return ret;
}
#endif

/* Return QCT cmd format */
struct dsi_panel_cmd_set *ss_get_cmds(struct samsung_display_driver_data *vdd, int type)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct dsi_display_mode_priv_info *priv_info;
	struct ss_cmd_set *ss_set = ss_get_ss_cmds(vdd, type);

	/* save the mipi cmd type */
	vdd->cmd_type = type;

	/* QCT cmds */
	if (type < SS_DSI_CMD_SET_START) {
		if (!panel || !panel->cur_mode || !panel->cur_mode->priv_info) {
			LCD_ERR(vdd, "err: (%d) panel has no valid priv\n", type);
			return NULL;
		}

		priv_info = panel->cur_mode->priv_info;

		return &priv_info->cmd_sets[type];
	}

	return &ss_set->base;
}

int ss_send_cmd(struct samsung_display_driver_data *vdd,
		int type)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct dsi_panel_cmd_set *set;
	struct ss_cmd_set *ss_set;
	int rc = 0;
	int i;

	rc = ss_check_panel_connection(vdd);
	if (rc)
		return rc;

	if (vdd->need_panel_recovery) {
		LCD_ERR(vdd, "skip to send_cmd when need_panel_recovery is set.\n");
		return -EINVAL;
	}

	/* TBR: never used samsung,fg-err_gpio .. remove fg_err_gpio code */
	if (vdd->is_factory_mode && ss_gpio_is_valid(vdd->fg_err_gpio) &&
			ss_gpio_get_value(vdd, vdd->fg_err_gpio)) {
		LCD_INFO(vdd, "fg_err_gpio val: %d\n", ss_gpio_get_value(vdd, vdd->fg_err_gpio));
		return -EAGAIN;
	}

	mutex_lock(&vdd->vdd_lock);
	ss_sde_vm_lock(vdd, true);

	/* skip set including RX in panel_dead (esd recovery) sequence */
	ss_set = ss_get_ss_cmds(vdd, type);
	if (!ss_set) {
		LCD_INFO(vdd, "%s cmd is NULL\n", ss_get_cmd_name(type));
		goto error;
	}

	if (vdd->panel_dead) {
		for (i = 0; i < ss_set->count; i++) {
			if (ss_set->cmds[i].rx_len > 0 && ss_set->cmds[i].rxbuf) {
				LCD_INFO(vdd, "esd recovery, skip %s incluiding rx\n",
						ss_get_cmd_name(type));
				goto error;
			}
		}
	}

	set = ss_get_cmds(vdd, type);

	if (ss_is_panel_off(vdd) || (vdd->panel_func.samsung_lvds_write_reg)) {
		LCD_INFO(vdd, "skip to tx cmd (%d), panel off (%d)\n",
				type, ss_is_panel_off(vdd));
		goto error;
	}

	if (!ss_sde_vm_owns_hw(vdd)) {
		LCD_INFO(vdd, "Skip command Tx due to HW unavailablity in TVM\n");
		goto error;
	}

	LCD_INFO_IF(vdd, "Send cmd(%d): %s ++\n", type, ss_get_cmd_name(type));

	/* In pm suspend status, it fails to control display clock.
	 * In result,
	 * - fails to transmit command to DDI.
	 * - sometimes causes unbalanced reference count of qc display clock.
	 * To prevent above issues, wait for pm resume before control display clock.
	 * You don't have to force to wake up PM system by calling pm_wakeup_ws_event().
	 * If cpu core reach to this code, it means interrupt or the other event is waking up
	 * PM system. So, it is enough just waiting until PM resume.
	 */
	if (ss_wait_for_pm_resume(vdd))
		goto error;

	/* To guarantee below 2 things in Command Mode panel
	 * 1. To prevent cmd tx delay from image(video) data tx, we need 2 TE wait
	 * 2. To make sure the timing of command tx, any command should not be interrupted
	 */
	if (vdd->support_optical_fingerprint && vdd->finger_mask_updated &&
			type == TX_SS_BRIGHT_CTRL &&
			panel->panel_mode == DSI_OP_CMD_MODE) {

		if (vdd->br_info.common_br.finger_mask_hbm_on)
			ss_wait_for_vsync(vdd, 2, vdd->panel_hbm_entry_after_vsync_pre);
		else
			ss_wait_for_vsync(vdd, 2, vdd->panel_hbm_exit_after_vsync_pre);
	}

	rc = ss_wrapper_dsi_panel_tx_cmd_set(panel, type);

	LCD_INFO_IF(vdd, "Send cmd(%d): %s --\n", type, ss_get_cmd_name(type));

error:
	ss_sde_vm_lock(vdd, false);
	mutex_unlock(&vdd->vdd_lock);

	if (!rc)
		ss_print_rx_buf(vdd, type);

	return rc;
}

/**
 * controller have 4 registers can hold 16 bytes of rxed data
 * dcs packet: 4 bytes header + payload + 2 bytes crc
 * 1st read: 4 bytes header + 10 bytes payload + 2 crc
 * 2nd read: 14 bytes payload + 2 crc
 * 3rd read: 14 bytes payload + 2 crc
 */
#define MAX_LEN_RX_BUF	700

static int ss_panel_on_pre(struct samsung_display_driver_data *vdd)
{
	int lcd_id_cmdline;
	int rc = 0;

	if (!ss_panel_attach_get(vdd)) {
		LCD_ERR(vdd, "ss_panel_attach_get NG\n");
		return -ENODEV;
	}

	LCD_INFO(vdd, "+\n");

	ss_set_vrr_switch(vdd, false);

	/* At this time, it already enabled SDE clock/power and  display power.
	 * It is possible to send mipi comamnd to display.
	 * To send mipi command, like mdnie setting or brightness setting,
	 * change panel_state: PANEL_PWR_OFF -> PANEL_PWR_ON_READY, here.
	 */
	ss_set_panel_state(vdd, PANEL_PWR_ON_READY);

	vdd->display_status_dsi.disp_on_pre = 1;

	if (unlikely(!vdd->esd_recovery.esd_recovery_init))
		ss_event_esd_recovery_init(vdd, 0, NULL);

	/* In some panels, mipi lane configuration must be preceded before mipi long read. */
	if (vdd->panel_func.mipi_lane_setting)
		vdd->panel_func.mipi_lane_setting(vdd);

	if (unlikely(vdd->is_factory_mode &&
			vdd->dtsi_data.samsung_support_factory_panel_swap)) {
		/* LCD ID read every wake_up time incase of factory binary */
		vdd->manufacture_id_dsi = PBA_ID;

		/* Factory Panel Swap*/
		vdd->manufacture_date_loaded_dsi = 0;
		vdd->ddi_id_loaded_dsi = 0;
		vdd->module_info_loaded_dsi = 0;
		vdd->cell_id_loaded_dsi = 0;
		vdd->octa_id_loaded_dsi = 0;
		vdd->br_info.hbm_loaded_dsi = 0;
		vdd->mdnie_loaded_dsi = 0;
		vdd->udc.read_done = 0;

		LCD_INFO(vdd, "set ddi id (0x%x)\n", vdd->manufacture_id_dsi);
	}

	if (vdd->manufacture_id_dsi == PBA_ID || vdd->manufacture_id_dsi == -EINVAL) {
		u8 recv_buf[3];

		LCD_INFO(vdd, "invalid ddi id from cmdline(0x%x), read ddi id\n", vdd->manufacture_id_dsi);

		/*
		*	At this time, panel revision it not selected.
		*	So last index(SUPPORT_PANEL_REVISION-1) used.
		*/
		vdd->panel_revision = SUPPORT_PANEL_REVISION-1;

		/*
		*	Some panel needs to update register at init time to read ID & MTP
		*	Such as, dual-dsi-control or sleep-out so on.
		*/
		if (!SS_IS_CMDS_NULL(ss_get_cmds(vdd, RX_MANUFACTURE_ID))) {
			rc = ss_send_cmd_get_rx(vdd, RX_MANUFACTURE_ID, recv_buf); /* needs LEVEL1_KEY */
		} else {
			LCD_INFO(vdd, "manufacture_read_pre_tx_cmds\n");
			rc |= ss_send_cmd_get_rx(vdd, RX_MANUFACTURE_ID0, recv_buf);
			rc |= ss_send_cmd_get_rx(vdd, RX_MANUFACTURE_ID1, recv_buf + 1);
			rc |= ss_send_cmd_get_rx(vdd, RX_MANUFACTURE_ID2, recv_buf + 2);
		}

		/* compare lcd_id from bootloader and kernel side for debugging */
		if (vdd->ndx == PRIMARY_DISPLAY_NDX)
			lcd_id_cmdline = get_lcd_attached("GET");
		else
			lcd_id_cmdline = get_lcd_attached_secondary("GET");

		if (rc <= 0) {
			LCD_INFO(vdd, "fail to read manufacture_id in kenrel, overwrite with the id from bl\n");
			vdd->manufacture_id_dsi = lcd_id_cmdline;
		} else {
			vdd->manufacture_id_dsi =
				(recv_buf[0] << 16) | (recv_buf[1] << 8) | recv_buf[2];
		}

		LCD_INFO(vdd, "manufacture id: 0x%x\n", vdd->manufacture_id_dsi);

		if (vdd->manufacture_id_dsi != lcd_id_cmdline) {
			if (vdd->is_factory_mode && vdd->dtsi_data.samsung_support_factory_panel_swap) {
				LCD_INFO(vdd, "LCD ID is changed (0x%X -> 0x%X)\n",
						lcd_id_cmdline, vdd->manufacture_id_dsi);
				set_lcd_id_cmdline(vdd, vdd->manufacture_id_dsi);
			} else {
				LCD_ERR(vdd, "LCD ID is changed (0x%X -> 0x%X)\n",
						lcd_id_cmdline, vdd->manufacture_id_dsi);
			}
		}

		/* Panel revision selection */
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_revision))
			LCD_ERR(vdd, "no panel_revision_selection_error function\n");
		else
			vdd->panel_func.samsung_panel_revision(vdd);

		LCD_INFO(vdd, "Panel_Revision = %c %d\n", vdd->panel_revision + 'A', vdd->panel_revision);
	}

	/* Read panel status to check panel is ok from bootloader */
	if (!vdd->read_panel_status_from_lk && vdd->samsung_splash_enabled) {
		ss_read_ddi_debug_reg(vdd);
		vdd->read_panel_status_from_lk = 1;
	}

	if (vdd->skip_read_on_pre) {
		LCD_INFO(vdd, "Skip read operation in on_pre\n");
		goto skip_read;
	}

	/* read display information via mipi */
	if (!vdd->module_info_loaded_dsi && vdd->panel_func.samsung_module_info_read)
		vdd->module_info_loaded_dsi = vdd->panel_func.samsung_module_info_read(vdd);

	if (!vdd->manufacture_date_loaded_dsi && vdd->panel_func.samsung_manufacture_date_read)
		vdd->manufacture_date_loaded_dsi = vdd->panel_func.samsung_manufacture_date_read(vdd);

	if (!vdd->ddi_id_loaded_dsi && vdd->panel_func.samsung_ddi_id_read)
		vdd->ddi_id_loaded_dsi = vdd->panel_func.samsung_ddi_id_read(vdd);

	/* deprecated. use samsung_otp_read */
	if (!vdd->br_info.elvss_loaded_dsi && vdd->panel_func.samsung_elvss_read)
		vdd->br_info.elvss_loaded_dsi = vdd->panel_func.samsung_elvss_read(vdd);

	/* deprecated. use samsung_otp_read */
	if (!vdd->br_info.irc_loaded_dsi && vdd->panel_func.samsung_irc_read)
		vdd->br_info.irc_loaded_dsi = vdd->panel_func.samsung_irc_read(vdd);

	/* OTP read */
	save_otp(vdd);

	if (!vdd->br_info.hbm_loaded_dsi && vdd->panel_func.samsung_hbm_read)
		vdd->br_info.hbm_loaded_dsi = vdd->panel_func.samsung_hbm_read(vdd);

	/* MDNIE X,Y (1.Manufacture Date -> 2.MDNIE X,Y -> 3.Cell ID -> 4.OCTA ID) */
	if (!vdd->mdnie_loaded_dsi && vdd->mdnie.support_mdnie && vdd->panel_func.samsung_mdnie_read)
		vdd->mdnie_loaded_dsi = vdd->panel_func.samsung_mdnie_read(vdd);

	/* Panel Unique Cell ID (1.Manufacture Date -> 2.MDNIE X,Y -> 3.Cell ID -> 4.OCTA ID) */
	if (!vdd->cell_id_loaded_dsi && vdd->panel_func.samsung_cell_id_read)
		vdd->cell_id_loaded_dsi = vdd->panel_func.samsung_cell_id_read(vdd);

	/* Panel Unique OCTA ID (1.Manufacture Date -> 2.MDNIE X,Y -> 3.Cell ID -> 4.OCTA ID) */
	if (!vdd->octa_id_loaded_dsi && vdd->panel_func.samsung_octa_id_read)
		vdd->octa_id_loaded_dsi = vdd->panel_func.samsung_octa_id_read(vdd);

	/* gamma mode2 and HOP display */
	if (vdd->br_info.common_br.gamma_mode2_support) {
		if (vdd->panel_func.samsung_gm2_gamma_comp_init &&
				!vdd->br_info.gm2_mtp.mtp_gm2_init_done) {
			LCD_INFO(vdd, "init gamma mode2 gamma compensation\n");
			vdd->panel_func.samsung_gm2_gamma_comp_init(vdd);
			vdd->br_info.gm2_mtp.mtp_gm2_init_done = true;
		}
	}

	/* UDC */
	if (vdd->panel_func.read_udc_data && !vdd->udc.read_done)
		vdd->panel_func.read_udc_data(vdd);

skip_read:

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_on_pre))
		vdd->panel_func.samsung_panel_on_pre(vdd);

	vdd->vrr.need_vrr_update = true;

	LCD_INFO(vdd, "-\n");

	return 0;
}

static int ss_panel_on_post(struct samsung_display_driver_data *vdd)
{
	bool connected;

	LCD_INFO(vdd, "+\n");

	/* mAFPC for VHM */
	if (!vdd->samsung_splash_enabled && vdd->mafpc.is_support) {
		LCD_INFO(vdd, "update mafpc\n");

		if (!IS_ERR_OR_NULL(vdd->mafpc.img_write)) {
			if (vdd->samsung_splash_enabled)
				vdd->mafpc.img_write(vdd, true);
			else
				vdd->mafpc.img_write(vdd, false);
		} else
			LCD_ERR(vdd, "mafpc img_write function is NULL\n");

		if (!IS_ERR_OR_NULL(vdd->mafpc.enable)) {
			if (vdd->mafpc.en)
				vdd->mafpc.enable(vdd, true);
			else
				vdd->mafpc.enable(vdd, false);
		} else
			LCD_ERR(vdd, "mafpc enable function is NULL\n");
	}

	/* Do not send init cmds during splash booting */
	if (vdd->cmd_set_on_splash_enabled || !vdd->samsung_splash_enabled) {
		/* delay between panel_reset and sleep_out */
		ss_delay(vdd->dtsi_data.after_reset_delay, vdd->reset_time_64);

		LCD_INFO(vdd, "tx ss on_cmd ++\n");
		ss_send_cmd(vdd, TX_DSI_CMD_SET_ON);
		LCD_INFO(vdd, "tx ss on_cmd --\n");

		/* sleep out command is included in DSI_CMD_SET_ON */
		vdd->sleep_out_time = vdd->tx_set_on_time = ktime_get();
	} else {
		LCD_INFO(vdd, "skip send TX_DSI_CMD_SET_ON during splash booting\n");
	}

	vdd->br_info.allow_br_update = true;

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_on_post))
		vdd->panel_func.samsung_panel_on_post(vdd);

	if (ss_is_bl_dcs(vdd)) {
		struct backlight_device *bd = GET_SDE_BACKLIGHT_DEVICE(vdd);
		struct dsi_panel *panel = GET_DSI_PANEL(vdd);

		/* In case of backlight update in panel off,
		 * dsi_display_set_backlight() returns error
		 * without updating vdd->br_info.common_br.bl_level.
		 * Update bl_level from bd->props.brightness.
		 */
		if (bd && vdd->br_info.common_br.bl_level != bd->props.brightness) {
			LCD_INFO(vdd, "update bl_level: %d -> %d\n",
				vdd->br_info.common_br.bl_level, bd->props.brightness);
			vdd->br_info.common_br.bl_level = bd->props.brightness;
		}
		if (panel->bl_config.bl_update == BL_UPDATE_DELAY_UNTIL_FIRST_FRAME)
			LCD_ERR(vdd, "skip bl set at on_post due to DELAY_UNTIL_FIRST_FRAME\n");
		else
			ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);

		vdd->vrr.need_vrr_update = false;
	}

	if (vdd->mdnie.support_mdnie) {
		vdd->mdnie.lcd_on_notifiy = true;
		update_dsi_tcon_mdnie_register(vdd);
		if (vdd->mdnie.support_trans_dimming)
			vdd->mdnie.disable_trans_dimming = false;
	}

	vdd->display_status_dsi.wait_disp_on = true;
	vdd->display_status_dsi.wait_actual_disp_on = true;
	vdd->panel_lpm.need_self_grid = true;

	if (vdd->dyn_mipi_clk.is_support) {
		LCD_INFO(vdd, "FFC%s Setting for Dynamic MIPI Clock\n", vdd->dyn_mipi_clk.osc_support ? "+OSC":"");
		ss_send_cmd(vdd, TX_FFC);
		if (vdd->dyn_mipi_clk.osc_support)
			ss_send_cmd(vdd, TX_OSC);
	}

	ss_send_cmd(vdd, TX_COPR_ENABLE);

	if (unlikely(vdd->is_factory_mode)) {
		ss_send_cmd(vdd, TX_FD_ON);
		/*
		* 1Frame Delay(33.4ms - 30FPS) Should be added for the project that needs FD cmd
		* Swire toggled at TX_FD_OFF, TX_LPM_ON, TX_FD_ON, TX_LPM_OFF.
		*/
		if(!SS_IS_CMDS_NULL(ss_get_cmds(vdd, TX_FD_ON)))
			usleep_range(34*1000, 34*1000);
	}

	/*
	 * ESD Enable should be called after all the cmd's tx is done.
	 * Some command may occur unexpected esd detect because of EL_ON signal control
	 */
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(true, true, (void *)vdd, ESD_MASK_PANEL_ON_OFF);

	/* Notify current ub_con_connected status */
	connected = ss_is_ub_connected(vdd);
	ss_ub_con_notify(vdd, connected);

	ss_set_panel_state(vdd, PANEL_PWR_ON);

	if (vdd->is_factory_mode) {
		bool connected = ss_is_ub_connected(vdd);

		if (!connected)
			ss_ub_con_notify(vdd, connected);
	}

	vdd->vrr.check_vsync = CHECK_VSYNC_COUNT;

	vdd->during_panel_recovery = 0;

	LCD_INFO(vdd, "-\n");

	return 0;
}

int ss_esync_off_pre(struct samsung_display_driver_data *vdd)
{
	int ret = 0;

	if (!vdd->esync_support) {
		LCD_ERR(vdd, "Only for esync support panel(VHM)\n");
		return -EINVAL;
	}

	ret = ss_panel_off_pre(vdd);
	if (ret) {
		LCD_ERR(vdd, "Panel Off Pre Failed\n");
		goto end;
	}

	ret = ss_panel_off_post(vdd);
	if (ret) {
		LCD_ERR(vdd, "Panel Off Post Failed\n");
		goto end;
	}
end:
	return ret;
}

int ss_panel_off_pre(struct samsung_display_driver_data *vdd)
{
	LCD_INFO(vdd, "+\n");

	/* Flush clk_work if pending */
	flush_delayed_work(&vdd->sde_normal_clk_work);

	ss_read_ddi_debug_reg(vdd);

	/* use recovery only during ON stage */
	vdd->need_panel_recovery = false;

	mutex_lock(&vdd->bl_lock);
	vdd->br_info.allow_br_update = false;
	cancel_delayed_work(&vdd->smooth_br_done_work);
	mutex_unlock(&vdd->bl_lock);

	/* panel_dead means that panel recovery is currently in progress */
	if (!vdd->panel_dead)
		vdd->during_panel_recovery = 0;

	vdd->esd_recovery.is_wakeup_source = false;
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd, ESD_MASK_PANEL_ON_OFF);
	vdd->esd_recovery.is_enabled_esd_recovery = false;

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_off_pre))
		vdd->panel_func.samsung_panel_off_pre(vdd);

	/* Use lpm_lock to prevent
	 * ss_panel_off_pre makes vdd->panel_state PANEL_PWR_OFF
	 * while ss_panel_lpm_ctrl is being performed.
	 *
	 * Otherwise, ss_panel_lpm_ctrl will make vdd->panel_state PANEL_PWR_LPM,
	 * even though it is a physically display off state.
	 */
	mutex_lock(&vdd->panel_lpm.lpm_lock);
	mutex_lock(&vdd->vdd_lock);
	ss_set_panel_state(vdd, PANEL_PWR_OFF);
	mutex_unlock(&vdd->vdd_lock);
	mutex_unlock(&vdd->panel_lpm.lpm_lock);

	LCD_INFO(vdd, "-\n");

	return 0;
}

int ss_panel_on(struct samsung_display_driver_data *vdd)
{
	/* If panel is VHM, ss_esync_on_post function should be used */
	if (vdd->esync_support) {
		LCD_DEBUG(vdd, "In case of VHM, this function is replaced with ss_esync_on_post\n");
		return 0;
	}

	/* If UB is not connected, panel on_pre/on_post is not done.
	 * Then it remains PANEL_OFF state.
	 */
	if (!ss_is_ub_connected(vdd)) {
		LCD_ERR(vdd, "ub is disconnected.. do not panel_on pre/post \n");
		return 0;
	}

	ss_panel_on_pre(vdd);
	ss_panel_on_post(vdd);

	return 0;
}

int ss_esync_on_post(struct samsung_display_driver_data *vdd)
{
	/* Only for VHM panel */
	if (!vdd->esync_support) {
		LCD_DEBUG(vdd, "This function is only for VHM\n");
		return 0;
	}

	/* If UB is not connected, panel on_pre/on_post is not done.
	 * Then it remains PANEL_OFF state.
	 */
	if (!ss_is_ub_connected(vdd)) {
		LCD_ERR(vdd, "ub is disconnected.. do not panel_on pre/post \n");
		return 0;
	}

	ss_panel_on_pre(vdd);
	ss_panel_on_post(vdd);

	return 0;
}

/*
 * Do not call ss_send_cmd()  here.
 * Any MIPI Tx/Rx can not be alowed in here.
 */
int ss_panel_off_post(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);

	LCD_INFO(vdd, "+\n");

	/*
	 * ss_panel_off_pre() already set PANEL_PWR_OFF, so it fails to send command with
	 * ss_send_cmd() api... Instead, use ss_wrapper_dsi_panel_tx_cmd_set().
	 * TODO: add PANEL_PWR_OFF_READY..
	 */
	LCD_INFO(vdd, "tx ss off_cmd\n");

	ss_wrapper_dsi_panel_tx_cmd_set(panel, TX_DSI_CMD_SET_OFF);

	vdd->display_on = false;

	if (vdd->mdnie.support_trans_dimming)
		vdd->mdnie.disable_trans_dimming = true;

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_off_post))
		vdd->panel_func.samsung_panel_off_post(vdd);

	/* Reset Self Display Status */
	if (vdd->self_disp.reset_status)
		vdd->self_disp.reset_status(vdd);

	if (vdd->finger_mask)
		vdd->finger_mask = false;

	/* To prevent panel off without finger off */
	if (vdd->br_info.common_br.finger_mask_hbm_on)
		vdd->br_info.common_br.finger_mask_hbm_on = false;

	vdd->mdnie.aolce_on = false;

	LCD_INFO(vdd, "- : mdp wr_ptr_line_count: %d\n", vdd->cnt_mdp_clk_underflow);
	SS_XLOG(vdd->cnt_mdp_clk_underflow);

	return 0;
}

int ss_frame_delay(int fps, int frame)
{
	return frame * DIV_ROUND_UP(1000, fps);
}

/* Regarding to description of Delay 1 frames in op code
 * HLPM mode: regard fps as 30hz, and delay 34ms
 * VRR HS mode: TE+vsync right after F7h update  command.
 *		freq is 120hz (HS MAX)
 * VRR NS mode: TE+vsync right after F7h update  command.
 *		freq is 60hz (NS MAX)
 * Confirmed by dlab.
 */
#define FPS_LPM	(30)
#define FPS_HS	(120)
#define FPS_NS	(60)
int ss_frame_to_ms(struct samsung_display_driver_data *vdd, int frame)
{
	struct vrr_info *vrr = &vdd->vrr;
	bool is_hlpm = ss_is_panel_lpm_ongoing(vdd);
	int fps;
	bool sot_hs = false, sot_phs = false;

	if (is_hlpm) {
		fps = FPS_LPM;
	} else {
		if (vrr->running_vrr) {
			sot_hs = vrr->prev_sot_hs_mode;
			sot_phs = vrr->prev_phs_mode;
		} else {
			sot_hs = vrr->cur_sot_hs_mode;
			sot_phs = vrr->cur_phs_mode;
		}

		if (sot_hs || sot_phs)
			fps = FPS_HS;
		else
			fps = FPS_NS;
	}

	LCD_DEBUG(vdd, "%dframe -> %dms, fps: %d, running_vrr: %d, hs: %d, phs: %d, lpm: %d(%d)\n",
			frame, ss_frame_delay(fps, frame), fps, vrr->running_vrr, sot_hs, sot_phs,
			is_hlpm, vdd->panel_lpm.during_ctrl);

	return ss_frame_delay(fps, frame);
}


/* TFT BACKLIGHT GPIO FUNCTION BELOW */
int ss_backlight_tft_request_gpios(struct samsung_display_driver_data *vdd)
{
	/* gpio_name[] named as gpio_name + num(recomend as 0)
	 * because of the num will increase depend on number of gpio
	 */
	char gpio_name[17] = "disp_bcklt_gpio0";
	static u8 gpio_request_status = -EINVAL;
	int rc = 0, i;

	if (!gpio_request_status)
		goto end;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "Invalid data pinfo : 0x%zx\n", (size_t)vdd);
		goto end;
	}

	for (i = 0; i < MAX_BACKLIGHT_TFT_GPIO; i++) {
		if (ss_gpio_is_valid(vdd->dtsi_data.backlight_tft_gpio[i])) {
			rc = gpio_request(vdd->dtsi_data.backlight_tft_gpio[i], gpio_name);
			if (rc) {
				LCD_ERR(vdd, "request %s failed, rc=%d\n", gpio_name, rc);
				goto tft_backlight_gpio_err;
			} else {
				LCD_ERR(vdd, "backlight_tft_gpio(%d) is requested, rc=%d\n", vdd->dtsi_data.backlight_tft_gpio[i], rc);
			}
			gpio_direction_output(vdd->dtsi_data.backlight_tft_gpio[i], 1);//init value High
		}
	}

	gpio_request_status = rc;
end:
	return rc;

tft_backlight_gpio_err:
	if (i) {
		do {
			if (ss_gpio_is_valid(vdd->dtsi_data.backlight_tft_gpio[i]))
				gpio_free(vdd->dtsi_data.backlight_tft_gpio[i--]);
			LCD_INFO(vdd, "i = %d\n", i);
		} while (i > 0);
	}

	return rc;
}

int ss_backlight_tft_gpio_config(struct samsung_display_driver_data *vdd, int enable)
{
	int ret = 0, i = 0, add_value = 1;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "Invalid data pinfo : 0x%zx\n",  (size_t)vdd);
		goto end;
	}

	LCD_INFO(vdd, "%s tft backlight gpios\n", enable ? "enable" : "disable");

	/*
	 * The order of backlight_tft_gpio enable/disable
	 * 1. Enable : backlight_tft_gpio[0], [1], ... [MAX_BACKLIGHT_TFT_GPIO - 1]
	 * 2. Disable : backlight_tft_gpio[MAX_BACKLIGHT_TFT_GPIO - 1], ... [1], [0]
	 */
	if (!enable) {
		add_value = -1;
		i = MAX_BACKLIGHT_TFT_GPIO - 1;
	}

	do {
		if (ss_gpio_is_valid(vdd->dtsi_data.backlight_tft_gpio[i])) {
			gpio_set_value(vdd->dtsi_data.backlight_tft_gpio[i], enable);
			LCD_INFO(vdd, "set backlight tft gpio[%d] to %s\n",
						 vdd->dtsi_data.backlight_tft_gpio[i],
						enable ? "high" : "low");
			usleep_range(500, 500);

			LCD_INFO(vdd, "tft gpio = %d\n", gpio_get_value(vdd->dtsi_data.backlight_tft_gpio[i]));
		}
	} while (((i += add_value) < MAX_BACKLIGHT_TFT_GPIO) && (i >= 0));

end:
	LCD_INFO(vdd, "-\n");
	return ret;
}

/*************************************************************
*
*		ESD RECOVERY RELATED FUNCTION BELOW.
*
**************************************************************/

static unsigned long false_esd_jiffies;

/*
 * esd_irq_enable() - Enable or disable esd irq.
 *
 * @enable	: flag for enable or disabled
 * @nosync	: flag for disable irq with nosync
 * @data	: point ot struct ss_panel_info
 */
static int esd_irq_enable(bool enable, bool nosync, void *data, u32 esd_mask)
{
	/* The irq will enabled when do the request_threaded_irq() */
	static bool is_enabled[MAX_DISPLAY_NDX] = {true, true};
	static bool is_wakeup_source[MAX_DISPLAY_NDX];
	int gpio;
	int irq[MAX_ESD_GPIO] = {0,};
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)data;
	struct esd_recovery *esd;

	u8 i = 0;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return -ENODEV;
	}

	esd = &vdd->esd_recovery;

	if (!enable)
		esd->esd_mask |= esd_mask;
	else
		esd->esd_mask &= ~(esd_mask);

	if (enable && esd->esd_mask) {
		LCD_INFO(vdd, "Do not enable ESD irq, Input Mask(0x%X), esd_mask(0x%X)\n",
								esd_mask, esd->esd_mask);
		return -ENODEV;
	}

	if (esd->num_of_gpio == 0)
		return 0;

	for (i = 0; i < esd->num_of_gpio; i++) {
		gpio = esd->esd_gpio[i];

		if (!ss_gpio_is_valid(gpio)) {
			LCD_ERR(vdd, "Invalid ESD_GPIO : %d\n", gpio);
			return -EINVAL;
		}

		irq[i] = ss_gpio_to_irq(gpio);
		if (irq[i] < 0) {
			LCD_ERR(vdd, "invalid irq[%d](%d)\n", i, irq[i]);
			return -EINVAL;
		}
	}

	mutex_lock(&esd->esd_lock);

	if (enable == is_enabled[vdd->ndx]) {
		LCD_INFO(vdd, "ESD irq already %s\n",
				enable ? "enabled" : "disabled");
		goto config_wakeup_source;
	}

	for (i = 0; i < esd->num_of_gpio; i++) {
		if (enable) {
			/* record time to prevent false positive esd interrupt
			 * skip esd interrupt for 100ms after enable esd interrupt.
			 */
			false_esd_jiffies = jiffies + __msecs_to_jiffies(100);

			enable_irq(irq[i]);
		} else {
			if (nosync)
				disable_irq_nosync(irq[i]);
			else
				disable_irq(irq[i]);
		}

		is_enabled[vdd->ndx] = enable;
	}

config_wakeup_source:
	if (esd->is_wakeup_source == is_wakeup_source[vdd->ndx]) {
		LCD_DEBUG(vdd, "[ESD] IRQs are already irq_wake %s\n",
				is_wakeup_source[vdd->ndx] ? "enabled" : "disabled");
		goto end;
	}

	for (i = 0; i < esd->num_of_gpio; i++) {
		gpio = esd->esd_gpio[i];

		if (!ss_gpio_is_valid(gpio)) {
			LCD_ERR(vdd, "Invalid ESD_GPIO : %d\n", gpio);
			continue;
		}

		is_wakeup_source[vdd->ndx] =
			esd->is_wakeup_source;

		if (is_wakeup_source[vdd->ndx])
			enable_irq_wake(irq[i]);
		else
			disable_irq_wake(irq[i]);
	}

end:
	mutex_unlock(&esd->esd_lock);

	LCD_INFO(vdd, "[ESD] IRQ %s (%s, %s) with Input Mask(0x%X), Current Mask(0x%X)\n",
				enable ? "enabled" : "disabled",
				nosync ? "nosync" : "sync",
				is_wakeup_source[vdd->ndx] ? "wakeup_source" : "no_wakeup", esd_mask, esd->esd_mask);

	return ret;
}

__visible_for_testing irqreturn_t esd_irq_handler(int irq, void *handle)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *) handle;
	struct sde_connector *conn;
	struct esd_recovery *esd = &vdd->esd_recovery;
	unsigned long now = jiffies;
	int i;

	if (!vdd->esd_recovery.is_enabled_esd_recovery) {
		LCD_ERR(vdd, "esd recovery is not enabled yet");
		goto end;
	}

	conn = GET_SDE_CONNECTOR(vdd);
	if (!conn) {
		LCD_ERR(vdd, "fail to get valid conn\n");
		goto end;
	}

	LCD_INFO(vdd, "now: %ld, threshold: %ld\n", now, false_esd_jiffies);
	if (time_before(now, false_esd_jiffies))
		goto end;

	if (vdd->is_factory_mode)
		ss_is_ub_connected(vdd);

	LCD_INFO(vdd, "++\n");

	/* To off the EL power when display is detached. */
	ss_panel_power_force_disable(vdd, &vdd->panel_powers[PANEL_POWERS_EVLDD_ELVSS]);

	esd_irq_enable(false, true, (void *)vdd, ESD_MASK_DEFAULT);

	vdd->panel_lpm.esd_recovery = true;

	schedule_work(&conn->status_work.work);

	LCD_INFO(vdd, "Panel Recovery(ESD, irq%d), Trial Count = %d\n", irq, vdd->panel_recovery_cnt++);
	SS_XLOG(vdd->panel_recovery_cnt);
	inc_dpui_u32_field(DPUI_KEY_QCT_RCV_CNT, 1);

	if (vdd->is_factory_mode) {
		for (i = 0; i < esd->num_of_gpio; i++) {
			if ((esd->esd_gpio[i] == vdd->ub_con_det.gpio) &&
				(ss_gpio_to_irq(esd->esd_gpio[i]) == irq)) {
				ss_ub_con_det_handler(irq, handle);
			}
		}
	}

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
	ss_notify_queue_work(vdd, PANEL_EVENT_ESD_STATE_CHANGED);
#endif

	LCD_INFO(vdd, "--\n");

end:
	return IRQ_HANDLED;
}

__visible_for_testing int ss_event_esd_recovery_init(
		struct samsung_display_driver_data *vdd, int event, void *arg)
{
	int ret;
	u8 i;
	int gpio, irqflags;
	struct esd_recovery *esd;
	struct dsi_panel *panel;
	struct drm_panel_esd_config *esd_config;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return -ENODEV;
	}

	LCD_INFO(vdd, "++\n");
	panel = GET_DSI_PANEL(vdd);
	esd_config = &panel->esd_config;
	esd = &vdd->esd_recovery;

	if (unlikely(!esd->esd_recovery_init)) {
		esd->esd_recovery_init = true;
		esd->esd_irq_enable = esd_irq_enable;
		if (esd_config->status_mode == ESD_MODE_PANEL_IRQ) {
			for (i = 0; i < esd->num_of_gpio; i++) {
				/* Set gpio num and irqflags */
				gpio = esd->esd_gpio[i];
				irqflags = esd->irqflags[i];
				if (ss_gpio_is_valid(gpio)) {
					if (gpio_request(gpio, "esd_recovery")) {
						LCD_ERR(vdd, "request esd_recovery gpio failed\n");
						continue;
					} else {
						ret = request_threaded_irq(
								ss_gpio_to_irq(gpio),
								NULL,
								esd_irq_handler,
								irqflags,
								"esd_recovery",
								(void *)vdd);
						if (ret)
							LCD_ERR(vdd, "Failed to request_irq, ret=%d\n",	ret);
						else
							LCD_INFO(vdd, "request esd irq !!\n");
					}
				} else {
					LCD_ERR(vdd, "[ESD] Invalid GPIO : %d\n", gpio);
					continue;
				}
			}
			esd_irq_enable(false, true, (void *)vdd, ESD_MASK_DEFAULT);
		}
	}

	LCD_INFO(vdd, "--\n");

	return 0;
}

void ss_panel_recovery(struct samsung_display_driver_data *vdd)
{
	struct sde_connector *conn = GET_SDE_CONNECTOR(vdd);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return;
	}

	if (!vdd->esd_recovery.is_enabled_esd_recovery) {
		LCD_ERR(vdd, "esd recovery is not enabled yet");
		return;
	}
	LCD_INFO(vdd, "Panel Recovery, Trial Count = %d\n", vdd->panel_recovery_cnt++);
	SS_XLOG(vdd->ndx, vdd->panel_recovery_cnt);
	inc_dpui_u32_field(DPUI_KEY_QCT_RCV_CNT, 1);

	esd_irq_enable(false, true, (void *)vdd, ESD_MASK_DEFAULT);
	vdd->panel_lpm.esd_recovery = true;
	schedule_work(&conn->status_work.work);

	vdd->during_panel_recovery = true;

	LCD_INFO(vdd, "Panel Recovery --\n");

}

/*************************************************************
*
*		UB_CON_DET event for factory binary.
*
**************************************************************/

void ss_send_ub_uevent(struct samsung_display_driver_data *vdd)
{
	char *envp[3] = {"CONNECTOR_NAME=UB_CONNECT", "CONNECTOR_TYPE=HIGH_LEVEL", NULL};
	char *envp_sub[3] = {"CONNECTOR_NAME=UB_CONNECT_SUB", "CONNECTOR_TYPE=HIGH_LEVEL", NULL};

	LCD_INFO(vdd, "[%s] send uvent \n", vdd->ndx == PRIMARY_DISPLAY_NDX ? "UB_CONNECT" : "UB_CONNECT_SUB");

	if (vdd->ndx == PRIMARY_DISPLAY_NDX) {
		kobject_uevent_env(&vdd->lcd_dev->dev.kobj, KOBJ_CHANGE, envp);
#if IS_ENABLED(CONFIG_SEC_ABC)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		sec_abc_send_event("MODULE=ub_main@INFO=ub_disconnected");
#else
		sec_abc_send_event("MODULE=ub_main@WARN=ub_disconnected");
#endif
#endif
	} else {
		kobject_uevent_env(&vdd->lcd_dev->dev.kobj, KOBJ_CHANGE, envp_sub);
#if IS_ENABLED(CONFIG_SEC_ABC)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		sec_abc_send_event("MODULE=ub_sub@INFO=ub_disconnected");
#else
		sec_abc_send_event("MODULE=ub_sub@WARN=ub_disconnected");
#endif
#endif
	}
}

/* return 1: connected, return 0: disconnected, return -1: no valid ub_con gpio */
int ss_is_ub_connected(struct samsung_display_driver_data *vdd)
{
	int connected;

	if (!vdd || !ss_gpio_is_valid(vdd->ub_con_det.gpio))
		return UB_UNKNOWN_CONNECTION;

	connected = ss_gpio_get_value(vdd, vdd->ub_con_det.gpio) == 0 ?
					UB_CONNECTED : UB_DISCONNECTED;

	LCD_INFO(vdd, "ub_con_det is [%s] [%s]\n",
			vdd->ub_con_det.enabled ? "enabled" : "disabled",
			connected == UB_CONNECTED ? "connected" : "disconnected");

	return connected;
}

void ss_ub_con_notify(struct samsung_display_driver_data *vdd, bool connected)
{
#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
	struct panel_notifier_event_data evt_data;

	evt_data.state = connected ? PANEL_EVENT_UB_CON_STATE_CONNECTED :
				PANEL_EVENT_UB_CON_STATE_DISCONNECTED;
	evt_data.display_index = vdd->ndx;

	panel_notifier_call_chain(PANEL_EVENT_UB_CON_STATE_CHANGED, &evt_data);
#endif
}

int ss_ub_con_det_irq_enable(struct samsung_display_driver_data *vdd, bool enable)
{
	int ret = 0;
	int irq = 0;
	/* The irq will enabled when do the request_threaded_irq() */
	static bool is_enabled = true;
	struct ub_con_detect *ub_con_det = NULL;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return -ENODEV;
	}

	ub_con_det = &vdd->ub_con_det;

	if (!ss_gpio_is_valid(ub_con_det->gpio)) {
		LCD_ERR(vdd, "Invalid ub_con_gpio\n");
		return -ENODEV;
	}

	irq = ss_gpio_to_irq(ub_con_det->gpio);
	if (irq < 0) {
		LCD_ERR(vdd, "invalid irq(%d)\n", irq);
		return -EINVAL;
	}

	if (enable == is_enabled) {
		LCD_INFO(vdd, "UB_CON_DET irq already %s\n",
				enable ? "enabled" : "disabled");
		goto skip;
	}

	if (enable) {
		enable_irq(irq);
		is_enabled = true;
	} else {
		disable_irq(irq);
		is_enabled = false;
	}

skip:
	LCD_INFO(vdd, "ub_con_det IRQ %s\n", enable ? "enabled" : "disabled");

	return ret;
}

irqreturn_t ss_ub_con_det_handler(int irq, void *handle)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *) handle;
	bool connected;

	connected = ss_is_ub_connected(vdd);
	ss_ub_con_notify(vdd, connected);

	if (connected)
		return IRQ_HANDLED;

	if (vdd->ub_con_det.enabled)
		ss_send_ub_uevent(vdd);

	vdd->ub_con_det.ub_con_cnt++;

	/* Skip Panel Power off in user binary incase of HW request (Tab S9/S9 Ultra) */
	if (!vdd->is_factory_mode && vdd->ub_con_det.ub_con_ignore_user) {
		LCD_INFO(vdd, "Skip Panel Power Off in user binary due to HW request\n");
		goto power_off_skip;
	}

	/* Regardless of ESD interrupt, it should guarantee display power off */
	LCD_INFO(vdd, "turn off dipslay power\n");
	ss_panel_power_off_pre_lp11(vdd);
	ss_panel_power_off_post_lp11(vdd);

	/* To off the EL power when display is detached. */
	ss_panel_power_force_disable(vdd, &vdd->panel_powers[PANEL_POWERS_EVLDD_ELVSS]);
power_off_skip:
	LCD_INFO(vdd, "-- cnt : %d\n", vdd->ub_con_det.ub_con_cnt);

	return IRQ_HANDLED;
}

/* HMT RELATED FUNCTION BELOW */
int hmt_bright_update(struct samsung_display_driver_data *vdd)
{
	if (vdd->hmt.hmt_on) {
		ss_brightness_dcs_hmt(vdd, vdd->hmt.bl_level);
	} else {
		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
		LCD_INFO(vdd, "hmt off state!\n");
	}

	return 0;
}

int hmt_enable(struct samsung_display_driver_data *vdd)
{
	LCD_INFO(vdd, "[HMT] HMT %s\n", vdd->hmt.hmt_on ? "ON" : "OFF");

	if (vdd->hmt.hmt_on) {
		ss_send_cmd(vdd, TX_HMT_ENABLE);
	} else {
		ss_send_cmd(vdd, TX_HMT_DISABLE);
	}

	return 0;
}

/* LPM control */
void ss_panel_low_power_config(struct samsung_display_driver_data *vdd, int enable)
{
	if (!vdd->panel_lpm.is_support && !vdd->panel_lpm.hlpm_mode) {
		LCD_INFO(vdd, "[Panel LPM] HLPM is disabled & not supported\n");
		return;
	}

	if (enable) {
		vdd->esd_recovery.is_wakeup_source = true;
	} else {
		vdd->esd_recovery.is_wakeup_source = false;
	}

	ss_panel_lpm_ctrl(vdd, enable);
}

void ss_panel_lpm_ctrl(struct samsung_display_driver_data *vdd, int enable)
{
	s64 lpm_init_delay_us = 0;

	if (!vdd->panel_lpm.is_support && !vdd->panel_lpm.hlpm_mode) {
		LCD_INFO(vdd, "[Panel LPM] HLPM is disabled & not supported\n");
		return;
	}

	if (ss_is_panel_off(vdd)) {
		LCD_INFO(vdd, "[Panel LPM]Do not change mode\n");
		return;
	}

	if (enable && ss_is_panel_lpm(vdd) && !vdd->is_factory_mode) {
		LCD_INFO(vdd, "[Panel LPM] Panel is already LPM\n");
		return;
	}

	LCD_INFO(vdd, "[Panel LPM] ++\n");
	SDE_EVT32(0x1111, enable);

	lpm_init_delay_us = (s64)vdd->dtsi_data.samsung_lpm_init_delay * 1000;

	mutex_lock(&vdd->panel_lpm.lpm_lock);

	/* For lego, regards as LPM mode if during_ctrl is set */
	vdd->panel_lpm.during_ctrl = true;

	/* If there is a pending ESD enable work, cancel that first */
	cancel_delayed_work(&vdd->esd_enable_event_work);

	/* To avoid unexpected ESD detction, disable ESD irq */
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd, ESD_MASK_WORK);

	if (enable) { /* AOD ON(Enter) */
		ss_panel_power_ctrl(vdd, &vdd->panel_powers[PANEL_POWERS_LPM_ON], true);

		/* Set br_values for LPM */
		set_br_values(vdd, &vdd->br_info.candela_map_table[AOD][vdd->panel_revision],
				vdd->br_info.common_br.bl_level);

		/* lpm init delay */
		if (lpm_init_delay_us) {
			s64 delay_us = lpm_init_delay_us -
						ktime_us_delta(ktime_get(), vdd->tx_set_on_time);

			LCD_INFO(vdd, "[Panel LPM] %lld us delay before turn on lpm mode\n",
					(delay_us < 0) ? 0 : delay_us);
			if (delay_us > 0)
				usleep_range(delay_us, delay_us);
		}

		/* Self Display Setting */
		if (vdd->self_disp.aod_enter)
			vdd->self_disp.aod_enter(vdd);

		if (unlikely(vdd->is_factory_mode))
			ss_send_cmd(vdd, TX_FD_OFF);

		LCD_INFO(vdd, "[Panel LPM] Send panel LPM on cmds +++\n");

		/* To prevent normal brightness patcket after tx LPM on */
		mutex_lock(&vdd->bl_lock);

		ss_send_cmd(vdd, TX_LPM_ON_AOR);
		ss_send_cmd(vdd, TX_LPM_ON);
		LCD_INFO(vdd, "[Panel LPM] Send LPM on cmds ---\n");

		/* TBR: why wait for TE ? not frame delay? add delay to PDF like "Delay 2frames" or "Delay 2TE".. */
		if (vdd->panel_lpm.entry_frame)
			ss_wait_for_te_gpio(vdd, vdd->panel_lpm.entry_frame, 0, false);

		if (unlikely(vdd->is_factory_mode)) {
			/* Do not apply self grid in case of factort lpm (conrolled by adb cmd) */
			ss_send_cmd(vdd, TX_DISPLAY_ON);
			/*
			* 1Frame Delay(33.4ms - 30FPS) Should be added for the project that needs FD cmd
			* Swire toggled at TX_FD_OFF, TX_LPM_ON, TX_FD_ON, TX_LPM_OFF.
			* And the on-off sequences in factory binary are like below
			* AOD on : ESD disable -> TX_FD_OFF -> TX_LPM_ON -> need 34ms delay -> ESD enable
			* AOD off : ESD disable -> TX_LPM_OFF -> TX_FD_ON -> need 34ms delay -> ESD enable
			* so we need 34ms delay for TX_LPM_ON & TX_FD_ON respectively.
			* But to reduce flicker issue, Add delay 34ms after 29(display on) cmd.
			*/
			if(!SS_IS_CMDS_NULL(ss_get_cmds(vdd, TX_FD_OFF)) && !vdd->dtsi_data.lpm_swire_delay)
				usleep_range(34*1000, 34*1000);
		} else {
			/* The display_on cmd will be sent on next commit */
			vdd->display_status_dsi.wait_disp_on = true;
			vdd->display_status_dsi.wait_actual_disp_on = true;
			LCD_INFO(vdd, "[Panel LPM] Set wait_disp_on to true\n");
		}

		LCD_INFO(vdd, "[Panel LPM] panel_state: %d -> LPM\n",
				vdd->panel_state);
		ss_set_panel_state(vdd, PANEL_PWR_LPM);
		mutex_unlock(&vdd->bl_lock);

		/* Update mdnie to disable mdnie operation by scenario at AOD display status. */
		if (vdd->mdnie.support_mdnie) {
			update_dsi_tcon_mdnie_register(vdd);
		}
	} else { /* AOD OFF(Exit) */
		ss_panel_power_ctrl(vdd, &vdd->panel_powers[PANEL_POWERS_LPM_OFF], true);

		/* Self Display Setting */
		if (vdd->self_disp.aod_exit)
			vdd->self_disp.aod_exit(vdd);

		mutex_lock(&vdd->bl_lock);
		ss_send_cmd(vdd, TX_LPM_OFF);

		/* TBR: add delay to PDF */
		/* 1Frame Delay(33.4ms - 30FPS) Should be added */
		usleep_range(34*1000, 34*1000);

		ss_send_cmd(vdd, TX_LPM_OFF_AOR);
		mutex_unlock(&vdd->bl_lock);

		LCD_INFO(vdd, "[Panel LPM] Send panel LPM off cmds\n");

		/* lpm.during_ctrl variable is for lpm on cmd (CL 24753104)
		 * If lpm.during_ctrl is true in the brightness setting after lpm off,
		 * vrr/lfd setting operates in LPM mode.
		 */
		vdd->panel_lpm.during_ctrl = false;

		ss_set_panel_state(vdd, PANEL_PWR_ON);

		/* To update vrr related cmds after off the LPM mode. */
		vdd->vrr.need_vrr_update = true;

		if ((vdd->support_optical_fingerprint) && (vdd->br_info.common_br.finger_mask_hbm_on))
			ss_brightness_dcs(vdd, 0, BACKLIGHT_FINGERMASK_ON); /* SAMSUNG_FINGERPRINT */
		else
			ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);

		if (vdd->mdnie.support_mdnie) {
			vdd->mdnie.lcd_on_notifiy = true;
			update_dsi_tcon_mdnie_register(vdd);
			if (vdd->mdnie.support_trans_dimming)
				vdd->mdnie.disable_trans_dimming = false;
		}

		/* TBR: why wait for TE ? not frame delay? add delay to PDF like "Delay 2frames" or "Delay 2TE".. */
		if (vdd->panel_lpm.exit_frame)
			ss_wait_for_te_gpio(vdd, vdd->panel_lpm.exit_frame, 0, false);

		if (unlikely(vdd->is_factory_mode)) {
			ss_send_cmd(vdd, TX_FD_ON);
			ss_send_cmd(vdd, TX_DISPLAY_ON);
			ss_set_panel_state(vdd, PANEL_PWR_ON);

			/* TBR: it already delay one second before enabling ESD.. remove below delay???
			 * Or, record ktime here, and check ktime in esd_irq_enable function, and delay more than 33ms.
			 */
			/*
			* 1Frame Delay(33.4ms - 30FPS) Should be added for the project that needs FD cmd
			* Swire toggled at TX_FD_OFF, TX_LPM_ON, TX_FD_ON, TX_LPM_OFF.
			* And the on-off sequences in factory binary are like below
			* AOD on : ESD disable -> TX_FD_OFF -> TX_LPM_ON -> need 34ms delay -> ESD enable
			* AOD off : ESD disable -> TX_LPM_OFF -> TX_FD_ON -> need 34ms delay -> ESD enable
			* so we need 34ms delay for TX_LPM_ON & TX_FD_ON respectively.
			* But to reduce flicker issue, Add delay 34ms after 29(display on) cmd.
			*/
			if(!SS_IS_CMDS_NULL(ss_get_cmds(vdd, TX_FD_ON)) && !vdd->dtsi_data.lpm_swire_delay)
				usleep_range(34*1000, 34*1000);
		} else {
			/* The display_on cmd will be sent on next commit */
			vdd->display_status_dsi.wait_disp_on = true;
			vdd->display_status_dsi.wait_actual_disp_on = true;
			LCD_INFO(vdd, "[Panel LPM] Set wait_disp_on to true\n");
		}
	}

	/* Flush clk_work if pending */
	flush_delayed_work(&vdd->sde_normal_clk_work);

	/* Enable ESD after (ESD_WORK_DELAY)ms */
	schedule_delayed_work(&vdd->esd_enable_event_work, msecs_to_jiffies(ESD_WORK_DELAY));

	LCD_INFO(vdd, "[Panel LPM] En/Dis : %s, LPM_MODE : %s, Hz : 30Hz, candela : %dNIT\n",
				enable ? "Enable" : "Disable",
				vdd->panel_lpm.mode == ALPM_MODE_ON ? "ALPM" :
				vdd->panel_lpm.mode == HLPM_MODE_ON ? "HLPM" :
				vdd->panel_lpm.mode == LPM_MODE_OFF ? "MODE_OFF" : "UNKNOWN",
				vdd->br_info.common_br.cd_level);

	vdd->panel_lpm.during_ctrl = false;

	mutex_unlock(&vdd->panel_lpm.lpm_lock);
	LCD_INFO(vdd, "[Panel LPM] --\n");
	SDE_EVT32(0x2222, enable);
}

struct samsung_display_driver_data *ss_get_vdd(enum ss_display_ndx ndx)
{
	return (ndx >= MAX_DISPLAY_NDX || ndx < 0) ? NULL : &vdd_data[ndx];
}

static struct candela_map_data *get_cd_map_data(struct samsung_display_driver_data *vdd,
			struct candela_map_table *table, int target_level)
{
	struct candela_map_data *map;
	int target_idx;
	int left, right, mid = 0;

	if (!table || !table->cd_map) {
		LCD_ERR(vdd, "No candela_map_table\n");
		return NULL;
	}

	map = table->cd_map;

	if (target_level > table->max_lv) {
		LCD_ERR(vdd, "lower target_level(%d) to max_lv(%d)\n", target_level, table->max_lv);
		target_level = table->max_lv;
	}

	left = 0;
	right = table->tab_size - 1;
	while (left < right) {
		mid = left + (right - left) / 2;

		if (map[mid].bl_level >= target_level)
			right = mid;
		else
			left = mid + 1;
	};

	target_idx = left;
	if (map[target_idx].bl_level != target_level) {
		LCD_INFO(vdd, "not matched level(%d), set to %d\n",
				target_level, map[target_idx].bl_level);
	}

	return &map[target_idx];
}

int set_br_values(struct samsung_display_driver_data *vdd,
			struct candela_map_table *table, int target_level)
{
	struct candela_map_data *map = get_cd_map_data(vdd, table, target_level);

	if (!map)
		return -ENODEV;

	vdd->br_info.common_br.bl_idx = map->bl_idx;
	vdd->br_info.common_br.cd_level = map->cd;
	vdd->br_info.common_br.gm2_wrdisbv = map->wrdisbv;
	vdd->mafpc.scale_idx = map->abc_scale_idx;

	LCD_DEBUG(vdd, "bl_idx: %d, bl_level: %d, wrdisbv: %d, cd: %d, ABC_idx: %d\n",
			vdd->br_info.common_br.bl_idx,
			vdd->br_info.common_br.bl_level,
			vdd->br_info.common_br.gm2_wrdisbv,
			vdd->br_info.common_br.cd_level,
			vdd->mafpc.scale_idx);

	return 0;
}

bool is_hbm_level(struct samsung_display_driver_data *vdd)
{
	struct candela_map_table *table;

	table = &vdd->br_info.candela_map_table[HBM][vdd->panel_revision];

	/* Normal brightness */
	if (vdd->br_info.common_br.bl_level < table->min_lv)
		return false;

	/* max should not be zero */
	if (!table->max_lv)
		return false;

	if (vdd->br_info.common_br.bl_level > table->max_lv) {
		LCD_ERR(vdd, "bl_level(%d) is over max_level (%d), force set to max\n",
				vdd->br_info.common_br.bl_level, table->max_lv);
		vdd->br_info.common_br.bl_level = table->max_lv;
	}

	return true;
}

bool ss_get_acl_status(struct samsung_display_driver_data *vdd)
{
	if (vdd->br_info.acl_status || vdd->siop_status)
		return true;
	else
		return false;
}

/* ss_brightness_dcs() is called not in locking status.
 * (Instead, calls ss_set_backlight() when you need to controll backlight
 * in locking status.=> deprecated)
 */
int ss_brightness_dcs(struct samsung_display_driver_data *vdd, int level, int backlight_origin)
{
	int ret = 0;
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	static int backup_bl_level, backup_acl;
	s64 t_s = 0, t_e = 0;
	char trace_name[30] = {0, };

	if (vdd->br_info.no_brightness) {
		LCD_ERR(vdd, "No brightness [%d], level [%d]\n", vdd->br_info.no_brightness, level);
		return 0;
	}

	/* FC2 change: set panel mode in SurfaceFlinger initialization, instead of kenrel booting... */
	/* TBR: remove checking cur_mode, and check in ss_send_cmd, or other code.. */
	if (!panel->cur_mode) {
		LCD_ERR(vdd, "err: no panel mode yet...\n");
		return -EINVAL;
	}

	snprintf(trace_name, 30, "brightness_%d", level);
	SDE_ATRACE_BEGIN(trace_name);
	mutex_lock(&vdd->bl_lock);

	if (vdd->support_optical_fingerprint) {
		/* SAMSUNG_FINGERPRINT */
		/* From nomal at finger mask on state
		* 1. backup acl
		* 2. backup level if it is not the same value as vdd->br_info.common_br.bl_level
		*   note. we don't need to backup this case because vdd->br_info.common_br.bl_level is a finger_mask_bl_level now
		*/

		/* finger mask hbm on or last finger mask frame is ongoing & bl update from normal */
		if (vdd->br_info.common_br.finger_mask_hbm_on && backlight_origin == BACKLIGHT_NORMAL) {
			backup_acl = vdd->br_info.acl_status;
			if (level != USE_CURRENT_BL_LEVEL)
				backup_bl_level = level;
			LCD_INFO(vdd, "[FINGER_MASK]BACKLIGHT_NORMAL save backup_acl = %d, backup_level = %d, vdd->br_info.common_br.bl_level=%d\n",
				backup_acl, backup_bl_level, vdd->br_info.common_br.bl_level);
			goto skip_bl_update;
		}

		/*
		 * If vdd->panel_hbm_exit_delay value is 0
		 * Finger_HBM Exit Command -> Vsync -> Normal : OK
		 * Finger_HBM Exit Command -> Normal -> Vsync : NG (Unexpected problem)
		 *
		 * To avoid NG case, we need to wait 1 vsync normal brightness cmd during Finger_HBM exit situation
		 * Do not skip normal brightenss setting : P230421-01277
		 */
		if (vdd->panel_hbm_exit_frame_wait && !vdd->panel_hbm_exit_delay) {
			LCD_INFO(vdd, "[FINGER_MASK]BACKLIGHT_FINGERMASK_OFF is not finished yet, wait vsync\n");
			ss_wait_for_vsync(vdd, 1, vdd->panel_hbm_exit_after_vsync_pre);
		}

		/* From BACKLIGHT_FINGERMASK_ON
		*   1. use finger_mask_bl_level(HBM) & acl 0
		*   2. backup previous bl_level & acl value
		*  From BACKLIGHT_FINGERMASK_OFF
		*   1. restore backup bl_level & acl value
		*  From BACKLIGHT_FINGERMASK_ON_SUSTAIN
		*   1. brightness update with finger_bl_level.
		*/

		if (backlight_origin == BACKLIGHT_FINGERMASK_ON) {
			vdd->br_info.common_br.finger_mask_hbm_on = true;
			backup_acl = vdd->br_info.acl_status;

			if (vdd->finger_mask_updated) /* do not backup br.bl_level at on to on */
				backup_bl_level = vdd->br_info.common_br.bl_level;
			level = vdd->br_info.common_br.finger_mask_bl_level;
			vdd->br_info.acl_status = 0;

			LCD_INFO(vdd, "[FINGER_MASK]BACKLIGHT_FINGERMASK_ON turn on finger hbm & back up acl = %d, level = %d\n",
				backup_acl, backup_bl_level);
		}
		else if(backlight_origin == BACKLIGHT_FINGERMASK_OFF) {
			vdd->br_info.common_br.finger_mask_hbm_on = false;
			vdd->br_info.acl_status = backup_acl;
			level = backup_bl_level;

			LCD_INFO(vdd, "[FINGER_MASK]BACKLIGHT_FINGERMASK_OFF turn off finger hbm & restore acl = %d, level = %d\n",
				vdd->br_info.acl_status, level);
		}
		else if(backlight_origin == BACKLIGHT_FINGERMASK_ON_SUSTAIN) {
			LCD_INFO(vdd, "[FINGER_MASK]BACKLIGHT_FINGERMASK_ON_SUSTAIN \
				send finger hbm & back up acl = %d, level = %d->%d\n",
				vdd->br_info.acl_status, level,vdd->br_info.common_br.finger_mask_bl_level);
			level = vdd->br_info.common_br.finger_mask_bl_level;
		}
	}

	/* backup prev level */
	vdd->br_info.common_br.prev_bl_level = vdd->br_info.common_br.bl_level;

	/* store bl level from PMS */
	if (level != USE_CURRENT_BL_LEVEL) {
		vdd->br_info.common_br.bl_level = level;
	}

	if (!ss_panel_attached(vdd->ndx)) {
		ret = -EINVAL;
		goto skip_bl_update;
	}

	if (!vdd->br_info.allow_br_update) {
		LCD_INFO(vdd, "Brightness Update is not allowed\n");
		goto skip_bl_update;
	}

	if ((panel->bl_config.bl_update == BL_UPDATE_DELAY_UNTIL_FIRST_FRAME)
			&& vdd->display_status_dsi.wait_disp_on) {
		LCD_INFO(vdd, "Skip bl update before disp_on\n");
		goto skip_bl_update;
	}

	if (vdd->grayspot) {
		LCD_INFO(vdd, "grayspot is [%d], skip bl update\n", vdd->grayspot);
		ret = -EINVAL;
		goto skip_bl_update;
	}

	if (vdd->brightdot_state || vdd->xtalk_mode_on) {
		/* During brightdot test, prevent whole brightntess setting,
		 * which changes brightdot setting.
		 * BIT0: brightdot test, BIT1: brightdot test in LFD 0.5hz
		 * allow brightness update in both brightdot test off case
		 */
		/* During X-talk mode on, it should skip brightness update */
		LCD_INFO(vdd, "brightdot(%d), xtalk(%d), skip bl update\n",
				vdd->brightdot_state, vdd->xtalk_mode_on);
		ret = -EINVAL;
		goto skip_bl_update;
	}

	if (ss_is_panel_lpm(vdd)) {
		LCD_INFO(vdd, "[Panel LPM]: update LPM brightness, bl_level=%d\n", vdd->br_info.common_br.bl_level);

		set_br_values(vdd, &vdd->br_info.candela_map_table[AOD][vdd->panel_revision],
				vdd->br_info.common_br.bl_level);

		if (vdd->panel_func.pre_lpm_brightness)
			vdd->panel_func.pre_lpm_brightness(vdd);

		ss_send_cmd(vdd, TX_SS_LPM_BRIGHT_CTRL);

		goto skip_bl_update;
	} else if (is_hbm_level(vdd)) {
		set_br_values(vdd, &vdd->br_info.candela_map_table[HBM][vdd->panel_revision],
				vdd->br_info.common_br.bl_level);
	} else {
		set_br_values(vdd, &vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision],
				vdd->br_info.common_br.bl_level);
	}

	if (vdd->hmt.is_support && vdd->hmt.hmt_on) {
		LCD_ERR(vdd, "HMT is on. do not set normal brightness(%d)\n", level);
		ret = -EINVAL;
		goto skip_bl_update;
	}

	if (ss_is_seamless_mode(vdd)) {
		LCD_ERR(vdd, "splash is not done\n");
		ret = -EINVAL;
		goto skip_bl_update;
	}

	/* make NORMAL/HBM brightness cmd packet */
	if (is_hbm_level(vdd))
		vdd->br_info.is_hbm = true;
	else
		vdd->br_info.is_hbm = false;

	/* sending tx cmds */
	if (vdd->panel_func.pre_brightness)
		vdd->panel_func.pre_brightness(vdd);

	t_s = ktime_get();
	if (vdd->need_brightness_lock || vdd->block_frame_oneshot) {
		ss_block_commit(vdd);
		ss_send_cmd(vdd, TX_SS_BRIGHT_CTRL);
		ss_release_commit(vdd);
		vdd->block_frame_oneshot = false;
	} else {
		ss_send_cmd(vdd, TX_SS_BRIGHT_CTRL);
	}
	t_e = ktime_get();

	/* copr sum after changing brightness to calculate brightness avg. */
	ss_set_copr_sum(vdd, COPR_CD_INDEX_0);
	ss_set_copr_sum(vdd, COPR_CD_INDEX_1);

	if (vdd->dvrr.support_dvrr) {
		LCD_INFO(vdd, "tx_time(%lld.%lldms) / delta(%lld.%lldms) level:%d candela:%dCD hbm:%d acl:(%d|%d) FI:%ld.%ld\n",
			ktime_us_delta(t_e, t_s) / 1000, ktime_us_delta(t_e, t_s) % 1000,
			ktime_us_delta(t_e, vdd->br_info.last_tx_time) / 1000,
			ktime_us_delta(t_e, vdd->br_info.last_tx_time) % 1000,
			vdd->br_info.common_br.bl_level, vdd->br_info.common_br.cd_level, vdd->br_info.is_hbm,
			ss_get_acl_status(vdd), vdd->br_info.gradual_acl_val,
			GET_PRIM_MS(vdd->dvrr.cur_frame_interval), GET_REST_MS(vdd->dvrr.cur_frame_interval));
	} else {
		LCD_INFO(vdd, "tx_time(%lld.%lldms) level:%d candela:%dCD hbm:%d acl:(%d|%d) FPS:%dx%d@%d%s\n",
			ktime_us_delta(t_e, t_s) / 1000, ktime_us_delta(t_e, t_s) % 1000,
			vdd->br_info.common_br.bl_level, vdd->br_info.common_br.cd_level, vdd->br_info.is_hbm,
			ss_get_acl_status(vdd), vdd->br_info.gradual_acl_val,
			vdd->vrr.cur_h_active, vdd->vrr.cur_v_active, vdd->vrr.cur_refresh_rate,
			vdd->vrr.cur_sot_hs_mode ? (vdd->vrr.cur_phs_mode ? "PHS" : "HS") : "NS");
	}

	if (vdd->br_info.common_br.smooth_dim_ddi_frame_cnt) {
		/* If there is a pending ddi smooth dimming work, cancel that first */
		cancel_delayed_work(&vdd->smooth_br_done_work);
		schedule_delayed_work(&vdd->smooth_br_done_work,
				msecs_to_jiffies(vdd->br_info.common_br.smooth_dim_ddi_frame_cnt * 9));
		vdd->br_info.common_br.smooth_br_done_work_in_progress = true;
	}
skip_bl_update:

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
	ss_notify_queue_work(vdd, PANEL_EVENT_BL_STATE_CHANGED);
#endif

	if (vdd->support_optical_fingerprint) {
		/* TBR: refactoring optical fingerprint code */
		/* SAMSUNG_FINGERPRINT */
		/* hbm needs vdd->panel_hbm_entry_delay TE to be updated, where as normal needs no */
		if (backlight_origin == BACKLIGHT_FINGERMASK_ON)
			ss_wait_for_vsync(vdd, vdd->panel_hbm_entry_delay, vdd->panel_hbm_entry_after_vsync);

		if ((backlight_origin >= BACKLIGHT_FINGERMASK_ON) && (vdd->finger_mask_updated)) {
			SDE_ERROR("finger_mask_updated/ sysfs_notify finger_mask_state = %d\n", vdd->finger_mask);
			sysfs_notify(&vdd->lcd_dev->dev.kobj, NULL, "actual_mask_brightness");
		}
		if (backlight_origin == BACKLIGHT_FINGERMASK_OFF) {
			ss_wait_for_vsync(vdd, vdd->panel_hbm_exit_delay, vdd->panel_hbm_exit_after_vsync);
			vdd->panel_hbm_exit_frame_wait = true;
		}
	}

	vdd->br_info.last_tx_time = t_e;
	mutex_unlock(&vdd->bl_lock);
	SDE_ATRACE_END(trace_name);

	return ret;
}

int ss_brightness_dcs_hmt(struct samsung_display_driver_data *vdd,
		int level)
{
	vdd->hmt.bl_level = level;
	LCD_INFO(vdd, "[HMT] hmt_bl_level(%d)\n", vdd->hmt.bl_level);

	set_br_values(vdd, &vdd->br_info.candela_map_table[HMT][vdd->panel_revision],
			vdd->hmt.bl_level);

	if (vdd->panel_func.pre_hmt_brightness)
		vdd->panel_func.pre_hmt_brightness(vdd);

	ss_send_cmd(vdd, TX_SS_HMT_BRIGHT_CTRL);

	return 0;
}

/* TFT brightness */
void ss_brightness_tft_pwm(struct samsung_display_driver_data *vdd, int level)
{
	if (vdd == NULL) {
		LCD_ERR(vdd, "no PWM\n");
		return;
	}

	if (ss_is_panel_off(vdd))
		return;

	vdd->br_info.common_br.bl_level = level;

	if (vdd->panel_func.samsung_brightness_tft_pwm)
		vdd->panel_func.samsung_brightness_tft_pwm(vdd, level);
}

void ss_read_mtp(struct samsung_display_driver_data *vdd, int addr, int len, int pos, u8 *buf)
{
	struct ss_cmd_set *ss_set;
	struct ss_cmd_desc *ss_cmd_gpara;
	struct ss_cmd_desc *ss_cmd_rx;
	struct dsi_panel_cmd_set *cmd_rx = NULL;
	int idx;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return;
	}

	if (vdd->two_byte_gpara) {
		if (addr > 0xFF || pos > 0xFFFF || len > 0xFF) {
			LCD_ERR(vdd, "invalid para addr(%x) pos(%d) len(%d)\n", addr, pos, len);
			return;
		}
	} else {
		if (addr > 0xFF || pos > 0xFF || len > 0xFF) {
			LCD_ERR(vdd, "invalid para addr(%x) pos(%d) len(%d)\n", addr, pos, len);
			return;
		}
	}

	/* TBR: use ss_read_mtp_sysfs() */
	if (len > 10)
		LCD_ERR(vdd, "critical error: this API doesn't support more than 10 bytes len(%d)\n", len);

	/* TBR: use UPDATE instead of below hardcoding.. */
	ss_set = ss_get_ss_cmds(vdd, RX_MTP_READ_SYSFS);
	if (SS_IS_SS_CMDS_NULL(ss_set) || ss_set->count != 8) {
		LCD_ERR(vdd, "invalid RX_MTP_READ_SYSFS SS CMDS\n");
		return;
	}

	cmd_rx = ss_get_cmds(vdd, RX_MTP_READ_SYSFS);
	if (SS_IS_CMDS_NULL(cmd_rx)) {
		LCD_ERR(vdd, "invalid RX_MTP_READ_SYSFS CMDS\n");
		return;
	}

	ss_cmd_gpara = &ss_set->cmds[3];
	idx = 1;
	if (vdd->two_byte_gpara)
		ss_cmd_gpara->txbuf[idx++] = (pos >> 8) & 0xFF;
	ss_cmd_gpara->txbuf[idx++] = pos & 0xFF;
	if (vdd->pointing_gpara)
		ss_cmd_gpara->txbuf[idx++] = addr;

	ss_cmd_rx = &ss_set->cmds[4];
	ss_cmd_rx->txbuf[0] = addr;
	cmd_rx->cmds[4].msg.rx_len = ss_cmd_rx->rx_len = len;

	ss_send_cmd_get_rx(vdd, RX_MTP_READ_SYSFS, buf);
}


/* TBR: merge ss_read_mtp_sysfs and ss_read_mtp.
 */
int ss_read_mtp_sysfs(struct samsung_display_driver_data *vdd, int addr, int len, int pos, u8 *buf)
{
	struct ss_cmd_set *ss_set;
	struct ss_cmd_desc *ss_cmd_gpara;
	struct ss_cmd_desc *ss_cmd_rx;
	struct dsi_panel_cmd_set *qc_set = NULL;
	int idx;
	int rc;
	int i, j;

	memset(buf, 0, len * sizeof(*buf));

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return -ENODEV;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return -EPERM;
	}

	if (vdd->two_byte_gpara) {
		if (addr > 0xFF || pos > 0xFFFF || len > 0xFF) {
			LCD_ERR(vdd, "invalid para addr(%x) pos(%d) len(%d)\n", addr, pos, len);
			return -EINVAL;
		}
	} else {
		if (addr > 0xFF || pos > 0xFF || len > 0xFF) {
			LCD_ERR(vdd, "invalid para addr(%x) pos(%d) len(%d)\n", addr, pos, len);
			return -EINVAL;
		}
	}

	ss_set = ss_get_ss_cmds(vdd, RX_MTP_READ_SYSFS);
	if (SS_IS_SS_CMDS_NULL(ss_set) || (vdd->gpara && ss_set->count != 8)) {
		LCD_ERR(vdd, "invalid RX_MTP_READ_SYSFS SS CMDS\n");
		return -EINVAL;
	}

	qc_set = ss_get_cmds(vdd, RX_MTP_READ_SYSFS);
	if (SS_IS_CMDS_NULL(qc_set)) {
		LCD_ERR(vdd, "invalid RX_MTP_READ_SYSFS CMDS\n");
		return -EINVAL;
	}
	if (vdd->gpara) {
		ss_cmd_gpara = &ss_set->cmds[3];
		idx = 1;
		if (vdd->two_byte_gpara)
			ss_cmd_gpara->txbuf[idx++] = (pos >> 8) & 0xFF;
		ss_cmd_gpara->txbuf[idx++] = pos & 0xFF;
		if (vdd->pointing_gpara)
			ss_cmd_gpara->txbuf[idx++] = addr;

		ss_cmd_rx = &ss_set->cmds[4];
		/* need to set rx_offset for rx_cmd */
		ss_set->cmds[4].rx_offset = pos;
	} else {
		ss_cmd_rx = &ss_set->cmds[0];
		/* need to set rx_offset for rx_cmd */
		ss_set->cmds[0].rx_offset = pos;
	}
	ss_cmd_rx->txbuf[0] = addr;
	ss_cmd_rx->rx_len = len;

	if (qc_set->cmds && qc_set->count > 0) {
		for (i = 0; i < qc_set->count; i++) {
			for (j = 0; j < ss_set->count; j++)
				if (qc_set->cmds[i].msg.tx_buf == ss_set->cmds[j].txbuf)
					break;

			if (j != ss_set->count)
				continue;

			kvfree(qc_set->cmds[i].msg.tx_buf);
		}

		kvfree(qc_set->cmds);
		qc_set->cmds = NULL;
		qc_set->count  = 0;
	}

	rc = ss_bridge_qc_cmd_alloc(vdd, ss_set, !vdd->gpara);
	if (rc) {
		LCD_ERR(vdd, "fail to alloc qc cmd(%d)\n", rc);
		return rc;
	}

	rc = ss_bridge_qc_cmd_update(vdd, ss_set, !vdd->gpara);
	if (rc) {
		LCD_ERR(vdd, "fail to update qc cmd(%d)\n", rc);
		return rc;
	}

	ss_send_cmd_get_rx(vdd, RX_MTP_READ_SYSFS, buf);

	return 0;
}

/* TBR: use MTP_SYSFS_TEST symbol and PDF. implement code to parse PDF on the fly */
void ss_write_mtp(struct samsung_display_driver_data *vdd, int len, u8 *buf)
{
	struct ss_cmd_set *ss_set;
	struct ss_cmd_desc *ss_cmd;
	struct dsi_cmd_desc *qc_cmd;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return;
	}

	if (len > 0xFF) {
		LCD_ERR(vdd, "invalid para len(%d)\n", len);
		return;
	}

	ss_set = ss_get_ss_cmds(vdd, TX_MTP_WRITE_SYSFS);
	if (SS_IS_SS_CMDS_NULL(ss_set)) {
		LCD_ERR(vdd, "No cmds for TX_MTP_WRITE_SYSFS\n");
		return;
	}

	ss_cmd = &ss_set->cmds[0];
	ss_cmd->txbuf = buf;
	ss_cmd->tx_len = len;

	qc_cmd = ss_cmd->qc_cmd;
	qc_cmd->msg.tx_len = ss_cmd->tx_len;
	qc_cmd->msg.tx_buf = ss_cmd->txbuf;

	ss_send_cmd(vdd, TX_MTP_WRITE_SYSFS);
}

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
__visible_for_testing void ss_bl_event_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd = NULL;
	struct panel_notifier_event_data evt_data;

	vdd = container_of(work, struct samsung_display_driver_data, bl_event_work);

	evt_data.display_index = vdd->ndx;
	evt_data.d.bl.level = vdd->br_info.common_br.bl_level;
	evt_data.d.bl.aor = vdd->br_info.common_br.aor_data;
	evt_data.d.bl.finger_mask_hbm_on = vdd->br_info.common_br.finger_mask_hbm_on;
	evt_data.d.bl.acl_status = vdd->br_info.acl_status;
	evt_data.d.bl.gradual_acl_val = vdd->br_info.gradual_acl_val;

	LCD_DEBUG(vdd, "%d %d %d %d ++\n",
		evt_data.d.bl.level, evt_data.d.bl.aor, evt_data.d.bl.acl_status, evt_data.d.bl.gradual_acl_val);
	panel_notifier_call_chain(PANEL_EVENT_BL_STATE_CHANGED, &evt_data);

	return;
}

__visible_for_testing void ss_vrr_event_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd =
		container_of(work, struct samsung_display_driver_data, vrr_event_work);
	struct vrr_info *vrr = NULL;
	struct panel_notifier_event_data evt_data;
	char vrr_mode[16];

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return;
	}

	vrr = &vdd->vrr;

	if (vrr->param_update) {
		snprintf(vrr_mode, sizeof(vrr_mode), "%dX%d:%s\n",
			vrr->adjusted_h_active, vrr->adjusted_v_active, vrr->adjusted_sot_hs_mode ?
			(vrr->adjusted_phs_mode ? "PHS" : "HS") : "NOR");
		LCD_DEBUG(vdd, " fps(%d) / vrr_mode = %s\n", vrr->adjusted_refresh_rate, vrr_mode);
	}

	evt_data.display_index = vdd->ndx;
	evt_data.d.dms.fps = vrr->adjusted_refresh_rate;

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_lfd_get_base_val)) {
		evt_data.d.dms.lfd_min_freq = DIV_ROUND_UP(vrr->lfd.base_rr, vrr->lfd.min_div);
		evt_data.d.dms.lfd_max_freq = DIV_ROUND_UP(vrr->lfd.base_rr, vrr->lfd.max_div);
	} else {
		evt_data.d.dms.lfd_min_freq = evt_data.d.dms.fps;
		evt_data.d.dms.lfd_max_freq = evt_data.d.dms.fps;
	}

	panel_notifier_call_chain(PANEL_EVENT_VRR_STATE_CHANGED, &evt_data);

	LCD_DEBUG(vdd, "fps=%d, lfd_min=%dhz(%d), lfd_max=%dhz(%d), base_rr=%dhz\n",
			evt_data.d.dms.fps,
			evt_data.d.dms.lfd_min_freq, vrr->lfd.min_div,
			evt_data.d.dms.lfd_max_freq, vrr->lfd.max_div,
			vrr->lfd.base_rr);
}

__visible_for_testing void ss_lfd_event_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd =
		container_of(work, struct samsung_display_driver_data, lfd_event_work);
	struct vrr_info *vrr;
	struct panel_notifier_event_data evt_data;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return;
	}

	if (!vdd->vrr.lfd.support_lfd) {
		LCD_INFO(vdd, "lfd is not supported\n");
		return;
	}

	vrr = &vdd->vrr;

	evt_data.display_index = vdd->ndx;
	evt_data.d.dms.fps = vrr->adjusted_refresh_rate;
	evt_data.d.dms.lfd_min_freq = DIV_ROUND_UP(vrr->lfd.base_rr, vrr->lfd.min_div);
	evt_data.d.dms.lfd_max_freq = DIV_ROUND_UP(vrr->lfd.base_rr, vrr->lfd.max_div);

	panel_notifier_call_chain(PANEL_EVENT_LFD_STATE_CHANGED, &evt_data);

	LCD_DEBUG(vdd, "fps=%d, lfd_min=%d, lfd_max=%d ++\n", evt_data.d.dms.fps,
			evt_data.d.dms.lfd_min_freq, evt_data.d.dms.lfd_max_freq);
}

__visible_for_testing void ss_panel_state_event_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd = NULL;
	struct panel_notifier_event_data evt_data;

	vdd = container_of(work, struct samsung_display_driver_data, panel_state_event_work);

	switch (vdd->panel_state) {
		case PANEL_PWR_ON:
			evt_data.state = PANEL_EVENT_PANEL_STATE_ON;

			if (vdd->is_factory_mode && vdd->dtsi_data.samsung_support_factory_panel_swap) {
				evt_data.d.screen_mode = vdd->manufacture_id_dsi;
				LCD_INFO(vdd, "lcd_id: 0x%x\n", vdd->manufacture_id_dsi);
			}
			break;
		case PANEL_PWR_OFF:
			evt_data.state = PANEL_EVENT_PANEL_STATE_OFF;
			break;
		case PANEL_PWR_LPM:
			evt_data.state = PANEL_EVENT_PANEL_STATE_LPM;
			break;
		default:
			evt_data.state = MAX_PANEL_EVENT_STATE;
			break;
	}

	evt_data.display_index = vdd->ndx;
	LCD_DEBUG(vdd, "%d ++\n", evt_data.state);
	panel_notifier_call_chain(PANEL_EVENT_PANEL_STATE_CHANGED, &evt_data);
	LCD_DEBUG(vdd, "--\n");
}

__visible_for_testing void ss_test_mode_event_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd =
	container_of(work, struct samsung_display_driver_data, test_mode_event_work);
	struct panel_notifier_event_data evt_data;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return;
	}

	switch (vdd->test_mode_state) {
		case PANEL_TEST_GCT:
			evt_data.state = PANEL_EVENT_TEST_MODE_STATE_GCT;
			break;
		default:
			evt_data.state = PANEL_EVENT_TEST_MODE_STATE_NONE;
			break;
	}

	evt_data.display_index = vdd->ndx;
	LCD_INFO(vdd, "test_mode_state = %d\n", evt_data.state);

	panel_notifier_call_chain(PANEL_EVENT_TEST_MODE_STATE_CHANGED, &evt_data);
}

__visible_for_testing void ss_screen_mode_event_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd =
	container_of(work, struct samsung_display_driver_data, screen_mode_event_work);
	struct mdnie_lite_tun_type *tune = NULL;
	struct panel_notifier_event_data evt_data;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return;
	}

	tune = vdd->mdnie.mdnie_tune_state_dsi;

	evt_data.display_index = vdd->ndx;
	evt_data.d.screen_mode = tune->mdnie_mode;

	LCD_INFO(vdd, "screen_mode = %d\n", evt_data.d.screen_mode);

	panel_notifier_call_chain(PANEL_EVENT_SCREEN_MODE_STATE_CHANGED, &evt_data);
}

__visible_for_testing void ss_esd_event_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd =
		container_of(work, struct samsung_display_driver_data, esd_event_work);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return;
	}

	LCD_INFO(vdd, "noity ESD\n");

	panel_notifier_call_chain(PANEL_EVENT_ESD_STATE_CHANGED, NULL);
}

int ss_notify_queue_work(struct samsung_display_driver_data *vdd,
	enum panel_notifier_event_t event)
{
	int ret = 0;

	mutex_lock(&vdd->notify_lock);

	LCD_DEBUG(vdd, "++ %d\n", event);

	switch (event) {
		case PANEL_EVENT_BL_STATE_CHANGED:
			/* notify clients of brightness change */
			queue_work(vdd->notify_wq, &vdd->bl_event_work);
			break;
		case PANEL_EVENT_VRR_STATE_CHANGED:
			queue_work(vdd->notify_wq, &vdd->vrr_event_work);
			break;
		case PANEL_EVENT_LFD_STATE_CHANGED:
			queue_work(vdd->notify_wq, &vdd->lfd_event_work);
			break;
		case PANEL_EVENT_PANEL_STATE_CHANGED:
			queue_work(vdd->notify_wq, &vdd->panel_state_event_work);
			break;
		case PANEL_EVENT_TEST_MODE_STATE_CHANGED:
			queue_work(vdd->notify_wq, &vdd->test_mode_event_work);
			break;
		case PANEL_EVENT_SCREEN_MODE_STATE_CHANGED:
			queue_work(vdd->notify_wq, &vdd->screen_mode_event_work);
			break;
		case PANEL_EVENT_ESD_STATE_CHANGED:
			queue_work(vdd->notify_wq, &vdd->esd_event_work);
			break;
		default:
			LCD_ERR(vdd, "unknown panel notify event %d\n", event);
			ret = -EINVAL;
			break;
	}

	LCD_DEBUG(vdd, "-- %d\n", event);

	mutex_unlock(&vdd->notify_lock);

	return ret;
}
#endif

__visible_for_testing void ss_esd_enable_event_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd =
	container_of(to_delayed_work(work), struct samsung_display_driver_data, esd_enable_event_work);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return;
	}

	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(true, true, (void *)vdd, ESD_MASK_WORK);

	LCD_INFO(vdd, "Done\n");
	return;
}

__visible_for_testing void ss_esd_disable_event_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd =
	container_of(to_delayed_work(work), struct samsung_display_driver_data, esd_disable_event_work);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return;
	}

	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd, ESD_MASK_WORK);

	LCD_INFO(vdd, "Done\n");
	return;
}

__visible_for_testing void ss_sde_normal_clk_work(struct work_struct *work)
{
	int ret = 0;
	struct samsung_display_driver_data *vdd =
	container_of(to_delayed_work(work), struct samsung_display_driver_data, sde_normal_clk_work);
	struct drm_device *ddev = GET_DRM_DEV(vdd);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return;
	}

	ret = ss_set_normal_sde_core_clk(ddev);
	if (ret) {
		LCD_ERR(vdd, "Failed to set normal sde core clock\n");
		SS_XLOG(0xbad);
	}

	vdd->vrr.keep_max_clk = false;

	LCD_DEBUG(vdd, "Done\n");
	return;
}

__visible_for_testing void ss_smooth_br_done_work(struct work_struct *work)
{
	struct samsung_display_driver_data *vdd =
		container_of(to_delayed_work(work), struct samsung_display_driver_data, smooth_br_done_work);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return;
	}

	SDE_ATRACE_BEGIN("BR_DONE");
	ss_send_cmd(vdd, TX_SS_SMOOTH_BR_DONE);
	vdd->br_info.common_br.smooth_br_done_work_in_progress = false;
	SDE_ATRACE_END("BR_DONE");

	LCD_INFO(vdd, "Done\n");
}

__visible_for_testing int ss_panel_vrr_switch(struct vrr_info *vrr)
{
	struct samsung_display_driver_data *vdd =
		container_of(vrr, struct samsung_display_driver_data, vrr);

	int cur_rr = vrr->cur_refresh_rate;
	bool cur_hs = vrr->cur_sot_hs_mode;
	bool cur_phs = vrr->cur_phs_mode;
	int adjusted_rr = vrr->adjusted_refresh_rate;
	bool adjusted_hs = vrr->adjusted_sot_hs_mode;
	bool adjusted_phs = vrr->adjusted_phs_mode;

	char trace_name[30] = {0, };
	int ret = 0;

	mutex_lock(&vrr->vrr_lock);
	vrr->running_vrr = true;
	vrr->running_vrr_mdp = false;
	vrr->check_vsync = CHECK_VSYNC_COUNT;

	snprintf(trace_name, 30, "VRR: %d->%d", cur_rr, adjusted_rr);
	SDE_ATRACE_BEGIN(trace_name);
	LCD_INFO(vdd, "+++ VRR: %d%s -> %d%s\n",
			cur_rr, cur_hs ? (cur_phs ? "PHS" : "HS") : "NM",
			adjusted_rr, adjusted_hs ?  (adjusted_phs ? "PHS" : "HS") : "NM");

	/* In OFF/LPM mode, skip VRR change, and just save VRR values.
	 * After normal display on, it will be applied in brightness setting.
	 */
	if (ss_is_panel_off(vdd) || ss_is_panel_lpm(vdd)) {
		LCD_WARN(vdd, "panel_state(%d), skip VRR (save VRR state)\n",
				vdd->panel_state);
		ret = -EPERM;
		SS_XLOG(0xbad, vdd->panel_state);

		vrr->cur_refresh_rate = adjusted_rr;
		vrr->cur_sot_hs_mode = adjusted_hs;
		vrr->cur_phs_mode = adjusted_phs;

		goto vrr_done;
	}

	/* In VRR, TE period is configured by cmd included in DSI_CMD_SET_TIMING_SWITCH.
	 * To avoid brightness flicker, TE period and proper brightness setting
	 * should be applied at once.
	 * TE period setting cmd will be applied by gamma update cmd (F7h).
	 * So, update brightness here, to send gamma update (F7h) and apply
	 * new brightness and new TE period which is configured in above DSI_CMD_SET_TIMING_SWITCH.
	 */

	vrr->cur_refresh_rate = adjusted_rr;
	vrr->cur_sot_hs_mode = adjusted_hs;
	vrr->cur_phs_mode = adjusted_phs;

	if (vdd->br_info.common_br.finger_mask_hbm_on)
		ss_brightness_dcs(vdd, 0, BACKLIGHT_FINGERMASK_ON_SUSTAIN);
	else
		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);

	vrr->prev_refresh_rate = vrr->cur_refresh_rate;
	vrr->prev_sot_hs_mode = vrr->cur_sot_hs_mode;
	vrr->prev_phs_mode = vrr->cur_phs_mode;

vrr_done:

	/* Restore to normal sde core clock boosted up to max mdp clock during VRR.
	 * In below case, current VRR mode and adjusting target VRR mode are different,
	 * and should keep maximum SDE core clock to prevent screen noise.
	 *
	 * 1) DISP thread#1: get 60HS -> 120HS VRR change request.
	 *                   Request SDE core clock to 200Mhz.
	 * 2) VRR thread#2:  start 60HS -> 120HS BRR which takes hundreds miliseconds.
	 *                   Set maximum SDE core clock (460Mhz)
	 * 3) DISP thread#1: get 120HS -> 60HS VRR change request.
	 *                   Request SDE core clock to 150Mhz.
	 * 4) VRR thread#2:  finish 60HS -> 120HS BRR.
	 *                   Restore SDE core clock to 150Mhz for 60HS while panel works at 120hz..
	 * 		     It causes screen noise...
	 *                   In this case, current VRR mode and adjusting target VRR mode are different!
	 * 5) VRR thread#2:  start 120HS -> 60HS BRR. Set maximum SDE core clock, and fix screen noise.
	 *                   After hundreds miliseconds, it finishes BRR,
	 *                   and restore SDE core clock to 150Mhz while panel works at 60hz.
	 *                   No more screen noise.
	 */
	if (!vrr->running_vrr_mdp)
		schedule_delayed_work(&vdd->sde_normal_clk_work,
				msecs_to_jiffies(SDE_CLK_WORK_DELAY));
	else
		LCD_INFO(vdd, "Do not set normal clk, vrr is ongoing!! (%d)\n", vrr->running_vrr_mdp);

	if (vdd->panel_func.post_vrr)
		vdd->panel_func.post_vrr(vdd, cur_rr, cur_hs, cur_phs,
				adjusted_rr, adjusted_hs, adjusted_phs);

	vrr->running_vrr = false;

	LCD_INFO(vdd, "--- VRR\n");
	SDE_ATRACE_END(trace_name);

	mutex_unlock(&vrr->vrr_lock);

	if (vrr->lfd.support_lfd)
#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
		ss_notify_queue_work(vdd, PANEL_EVENT_LFD_STATE_CHANGED);
#endif

	/* dump timestamp to monitor tearing after 3 frames.
	 * ddi will be configured enough in 3 frames.
	 */

	if (vdd->dbg_tear_info.dbg_tear_wq) {
		queue_delayed_work(vdd->dbg_tear_info.dbg_tear_wq,
				&vdd->dbg_tear_info.dbg_tear_work,
				msecs_to_jiffies(ss_frame_delay(vrr->cur_refresh_rate, 3)));
	}

	return ret;
}

void ss_panel_vrr_switch_work(struct work_struct *work)
{
	struct vrr_info *vrr = container_of(work, struct vrr_info, vrr_work);

	ss_panel_vrr_switch(vrr);
}

static void ss_dvrr_init(struct samsung_display_driver_data *vdd)
{
	struct dvrr_info *dvrr = &vdd->dvrr;

	mutex_init(&dvrr->dvrr_lock);

	dvrr->cur_frame_interval = dvrr->prev_frame_interval = 0;
}

int ss_panel_dvrr_switch(struct samsung_display_driver_data *vdd, u32 frame_interval)
{
	int ret = 0;

	vdd->dvrr.prev_frame_interval = vdd->dvrr.cur_frame_interval;
	vdd->dvrr.cur_frame_interval = frame_interval;

	if (vdd->dvrr.prev_frame_interval != vdd->dvrr.cur_frame_interval) {
		vdd->dvrr.dvrr_update = true;
		LCD_INFO(vdd, "DVRR: %ld.%ld -> %ld.%ld\n",
			GET_PRIM_MS(vdd->dvrr.prev_frame_interval), GET_REST_MS(vdd->dvrr.prev_frame_interval),
			GET_PRIM_MS(vdd->dvrr.cur_frame_interval), GET_REST_MS(vdd->dvrr.cur_frame_interval));
	} else
		vdd->dvrr.dvrr_update = false;

	return ret;
}

int ss_panel_dms_switch(struct samsung_display_driver_data *vdd)
{
	struct vrr_info *vrr = &vdd->vrr;

	int cur_hact = vrr->cur_h_active;
	int cur_vact = vrr->cur_v_active;
	bool cur_hs = vrr->cur_sot_hs_mode;
	bool cur_phs = vrr->cur_phs_mode;
	int adjusted_hact = vrr->adjusted_h_active;
	int adjusted_vact = vrr->adjusted_v_active;
	bool adjusted_hs = vrr->adjusted_sot_hs_mode;
	bool adjusted_phs = vrr->adjusted_phs_mode;

	bool is_hs_change = !!(cur_hs != adjusted_hs) || !!(cur_phs != adjusted_phs);

	LCD_DEBUG(vdd, "%dx%d@%d%s -> %dx%d@%d%s , is_hs_change %d\n",
			cur_hact, cur_vact, vrr->cur_refresh_rate,
			cur_hs ? (cur_phs ? "PHS" : "HS") : "NM",
			adjusted_hact, adjusted_vact, vrr->adjusted_refresh_rate,
			adjusted_hs ? (adjusted_phs ? "PHS" : "HS") : "NM",
			is_hs_change);

	/* Do we have to change param only when the HS<->NORMAL be changed? */
	ss_set_vrr_switch(vdd, true);

	if (vrr->is_multi_resolution_changing) {
		LCD_INFO(vdd, "multi resolution: tx DMS switch cmd\n");
		if (vdd->panel_func.samsung_timing_switch_pre)
			vdd->panel_func.samsung_timing_switch_pre(vdd);

		ss_send_cmd(vdd, TX_TIMING_SWITCH);

		if (vdd->panel_func.samsung_timing_switch_post)
			vdd->panel_func.samsung_timing_switch_post(vdd);

		vrr->is_multi_resolution_changing = false;
	}

	if (vrr->is_vrr_changing) {
		if (vrr->vrr_workqueue)
			queue_work(vrr->vrr_workqueue, &vrr->vrr_work);
		vrr->is_vrr_changing = false;
	}

	vrr->cur_h_active = vrr->adjusted_h_active;
	vrr->cur_v_active = vrr->adjusted_v_active;

	return 0;
}

void ss_set_panel_state(struct samsung_display_driver_data *vdd, enum ss_panel_pwr_state panel_state)
{
	vdd->panel_state = panel_state;

	LCD_INFO(vdd, "set panel state %d\n", vdd->panel_state);

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
	ss_notify_queue_work(vdd, PANEL_EVENT_PANEL_STATE_CHANGED);
#endif
}

void ss_set_test_mode_state(struct samsung_display_driver_data *vdd, enum ss_test_mode_state state)
{
	vdd->test_mode_state = state;

	LCD_INFO(vdd, "set test_mde state %d\n", vdd->test_mode_state);

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
	ss_notify_queue_work(vdd, PANEL_EVENT_TEST_MODE_STATE_CHANGED);
#endif
}

/* bring start time of gamma flash forward: first frame update -> display driver probe.
 * As of now, gamma flash starts when bootanimation kickoff frame update.
 *
 * In that case, it causes below problem.
 * 1) bootanimation turns on main and sub display.
 * 2) bootanimation update one black frame to both displays.
 * 3) bootanimation turns off sub dipslay, in case of folder open.
 *    But, sub display gamma flash takes several seconds (about 3 sec in user binary),
 *    so, it bootanimation waits for it, and both displays shows black screen for a while.
 * 4) After sub display gamma flash is done and finish to turn off sub display,
 *    bootanimation starts to draw animation on main display, and it is too late then expected..
 *
 * To avoid above issue, starts gamma flash earlyer, at display driver probe timing.
 * For this, to do belows at driver probe timing.
 * 1) start gamma flash only in case of splash mode on. Splash on mode means that bootlaoder already turned on mipi phy.
 * 2) initialize mipi host, including enable mipi error interrupts, to avoid blocking mipi transmission.
 * 3) disable autorefresh, which was disabled in first frame update, to prevent dsi fifo underrun error.
 */
int ss_early_display_init(struct samsung_display_driver_data *vdd)
{
	if (!vdd) {
		LCD_ERR(vdd, "error: vdd NULL\n");
		return -ENODEV;
	}

	if (vdd->support_early_id_read) {
		LCD_DEBUG(vdd, "early module ID read\n");
	} else {
		LCD_INFO(vdd, "no action\n");
		return 0;
	}

	LCD_INFO(vdd, "++ ndx=%d: cur panel_state: %d\n", vdd->ndx, vdd->panel_state);

	/* set lcd id for getting proper mapping table */
	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		vdd->manufacture_id_dsi = get_lcd_attached("GET");
	else if (vdd->ndx == SECONDARY_DISPLAY_NDX)
		vdd->manufacture_id_dsi = get_lcd_attached_secondary("GET");

	LCD_INFO(vdd, "manufacture_id_dsi = 0x%x\n", vdd->manufacture_id_dsi);

	/* Panel revision selection */
	if (IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_revision))
		LCD_ERR(vdd, "no panel_revision_selection_error function\n");
	else
		vdd->panel_func.samsung_panel_revision(vdd);

	/* in gamma flash work thread, it will initialize mipi host */
	/* In case of autorefresh_fail, do gamma flash job in ss_event_frame_update()
	 * which is called after RESET MDP to recover error status.
	 * TODO: get proper phys_enc and check if phys_enc->enable_state = SDE_ENC_ERR_NEEDS_HW_RESET...
	 */
	if (vdd->is_autorefresh_fail) {
		LCD_ERR(vdd, "autorefresh failure error\n");
		return -EINVAL;
	}

	/* early module id read */
	if (vdd->support_early_id_read) {
		bool skip_restore_panel_state_off = true;

		LCD_INFO(vdd, "early module ID read\n");

		/* set panel_state pwr on ready, to allow mipi transmission */
		if (vdd->panel_state == PANEL_PWR_OFF) {
			skip_restore_panel_state_off = false;
			vdd->panel_state = PANEL_PWR_ON_READY;
		}

		/* Module info */
		if (!vdd->module_info_loaded_dsi) {
			if (IS_ERR_OR_NULL(vdd->panel_func.samsung_module_info_read))
				LCD_ERR(vdd, "no samsung_module_info_read function\n");
			else
				vdd->module_info_loaded_dsi = vdd->panel_func.samsung_module_info_read(vdd);
		}

		/* restore panel_state to poweroff
		 * to prevent other mipi transmission before gfx HAL enable display.
		 *
		 * disp_on_pre == true means panel_state is already overwritten by gfx HAL,
		 * so no more need to restore panel_state
		 */
		if (vdd->display_status_dsi.disp_on_pre)
			skip_restore_panel_state_off = true;
		if (!skip_restore_panel_state_off) {
			LCD_INFO(vdd, "restore panel state to off\n");
			vdd->panel_state = PANEL_PWR_OFF;
		}
	}

	LCD_INFO(vdd, "--\n");

	return 0;
}

static int ss_gm2_ddi_flash_init(struct samsung_display_driver_data *vdd)
{
	struct flash_gm2 *gm2_table = &vdd->br_info.gm2_table;

	if (vdd->br_info.gm2_table.ddi_flash_raw_len == 0) {
		LCD_DEBUG(vdd, "do not support gamma mode2 flash (%d)\n",
				vdd->br_info.gm2_table.ddi_flash_raw_len);
		return 0;
	}

	gm2_table->ddi_flash_raw_buf = NULL;

	/* init gamma mode2 flash data */
	gm2_table->ddi_flash_raw_buf = kzalloc(gm2_table->ddi_flash_raw_len, GFP_KERNEL | GFP_DMA);
	if (!gm2_table->ddi_flash_raw_buf) {
		LCD_ERR(vdd, "fail to alloc ddi_flash_raw_buf\n");
		return -ENOMEM;
	}

	LCD_INFO(vdd, "init flash: OK\n");

	return 0;
}

/* Align & Paylod Size for Non-Embedded mode(Mass) Command Transfer */
#if IS_ENABLED(CONFIG_ARCH_SUN)
/* From SM8750(SUN), DSI embedded FIFO size was increased from 256 to 1024 */
#define MASS_CMD_ALIGN 1024
#elif IS_ENABLED(CONFIG_ARCH_PARROT) || IS_ENABLED(CONFIG_ARCH_VOLCANO)
/* SM6475(PARROT) SM7635(VOLCANO) */
#define MASS_CMD_ALIGN 256
#endif
#define MAX_PAYLOAD_SIZE_MASS 0xFFFF0 /* 983,024 */

int ss_convert_img_to_mass_cmds(struct samsung_display_driver_data *vdd,
		char *data, u32 data_size, struct ss_cmd_desc *ss_cmd)
{
	struct dsi_cmd_desc *qc_cmd;
	u32 data_idx = 0;
	u32 payload_len = 0;
	u32 p_len = 0;
	u32 c_cnt = 0;

	if (!data) {
		LCD_ERR(vdd, "data is null..\n");
		return -EINVAL;
	}

	if (!data_size) {
		LCD_ERR(vdd, "data size is zero..\n");
		return -EINVAL;
	}

	payload_len = data_size + (data_size + MASS_CMD_ALIGN - 2) / (MASS_CMD_ALIGN - 1);

	/* fill ss cmds */
	if (!ss_cmd->txbuf) {
		ss_cmd->type = MIPI_DSI_GENERIC_LONG_WRITE;
		ss_cmd->last_command = 1;

		/* TBR: Bigger than 2k->vzalloc, Small=>kvzalloc*/
		ss_cmd->txbuf = kvzalloc(payload_len, GFP_KERNEL);
		if (!ss_cmd->txbuf) {
			LCD_ERR(vdd, "fail to kvzalloc for cmds ss_txbuf, requested:%d\n", payload_len);
			return -ENOMEM;
		}

		ss_cmd->qc_cmd = kvzalloc(sizeof(struct dsi_cmd_desc), GFP_KERNEL); /*60byte*/
		if (!ss_cmd->qc_cmd) {
			LCD_ERR(vdd, "fail to kvzalloc for qc_cmd, requested:%lu\n", sizeof(struct dsi_cmd_desc));
			vfree(ss_cmd->txbuf);
			return -ENOMEM;
		}
		ss_cmd->qc_cmd->ss_cmd = ss_cmd;
	}

	for (p_len = 0; p_len < payload_len && data_idx < data_size ; p_len++) {
		if (p_len % MASS_CMD_ALIGN)
			ss_cmd->txbuf[p_len] = data[data_idx++];
		else
			ss_cmd->txbuf[p_len] = (p_len == 0 && c_cnt == 0) ? 0x4C : 0x5C;
	}

	ss_cmd->tx_len = p_len;

	/* update qc cmds */
	qc_cmd = ss_cmd->qc_cmd;
	qc_cmd->msg.type = ss_cmd->type;
	qc_cmd->last_command = ss_cmd->last_command;
	qc_cmd->post_wait_ms = ss_cmd->post_wait_ms;
	qc_cmd->msg.tx_len = ss_cmd->tx_len;
	qc_cmd->msg.tx_buf = ss_cmd->txbuf;

	LCD_INFO(vdd, "data size: %d, payload_len: %d, tx_len: %lu\n",
			data_size, payload_len, ss_cmd->tx_len);

	return 0;
}

void ss_panel_init(struct dsi_panel *panel)
{
	struct samsung_display_driver_data *vdd;
	enum ss_display_ndx ndx;
	char panel_name[MAX_CMDLINE_PARAM_LEN];
	char panel_secondary_name[MAX_CMDLINE_PARAM_LEN];

	/* compare panel name in command line and dsi_panel.
	 * primary panel: ndx = 0
	 * secondary panel: ndx = 1
	 */

	ss_get_primary_panel_name_cmdline(panel_name);
	ss_get_secondary_panel_name_cmdline(panel_secondary_name);

	if (!strcmp(panel->name, panel_name)) {
		ndx = PRIMARY_DISPLAY_NDX;
	} else if (!strcmp(panel->name, panel_secondary_name)) {
		ndx = SECONDARY_DISPLAY_NDX;
	} else {
		/* If it fails to find panel name, it cannot turn on display,
		 * and this is critical error case...
		 */
		WARN(1, "fail to find panel name, panel=%s, cmdline=%s\n",
				panel->name, panel_name);
		return;
	}

	/* TODO: after using component_bind in samsung_panel_init,
	 * it doesn't have to use vdd_data..
	 * Remove vdd_data, and allocate vdd memory here.
	 * vdds will be managed by vdds_list...
	 */
	vdd = ss_get_vdd(ndx);
	vdd->ndx = ndx;

	LCD_INFO(vdd, "++ \n");

	strncpy(vdd->panel_name, panel->name, MAX_CMDLINE_PARAM_LEN);
	vdd->panel_name[MAX_CMDLINE_PARAM_LEN-1] = 0; /* To garauntee null terminated */

	panel->panel_private = vdd;
	vdd->msm_private = panel;
	list_add(&vdd->vdd_list, &vdds_list);

	if (ss_panel_debug_init(vdd))
		LCD_ERR(vdd, "Fail to create debugfs\n");

	if (ss_smmu_debug_init(vdd))
		LCD_ERR(vdd, "Fail to create smmu debug\n");

	mutex_init(&vdd->vdd_lock);
	mutex_init(&vdd->cmd_lock);
	mutex_init(&vdd->bl_lock);
	mutex_init(&vdd->display_on_lock);

	/* To guarantee ALPM ON or OFF mode change operation*/
	mutex_init(&vdd->panel_lpm.lpm_lock);

	/* To guarantee dynamic MIPI clock change*/
	mutex_init(&vdd->dyn_mipi_clk.dyn_mipi_lock);
	vdd->dyn_mipi_clk.requested_clk_rate = 0;
	vdd->dyn_mipi_clk.force_idx = -1;

	vdd->panel_dead = false;
	vdd->is_autorefresh_fail = false;

	/* Read from file should be called before vdd->panel_func.samsung_panel_init(vdd) */
	ss_wrapper_read_from_file(vdd, false);

	/* register common ss command's op symbol-callback table
	 * this  should be called before vdd->panel_func.samsung_panel_init()
	 * which can replace op symbol-callback pair.
	 */
	register_common_op_sym_cb(vdd);

	if (IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_init))
		LCD_ERR(vdd, "no samsung_panel_init func");
	else
		vdd->panel_func.samsung_panel_init(vdd);

	/* Panel revision selection */
	if (IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_revision)) {
		LCD_ERR(vdd, "no panel_revision_selection function\n");
	} else {
		/* set manufacture_id_dsi from cmdline to set proper panel revision */
		if (vdd->ndx == PRIMARY_DISPLAY_NDX)
			vdd->manufacture_id_dsi = get_lcd_attached("GET");
		else if (vdd->ndx == SECONDARY_DISPLAY_NDX)
			vdd->manufacture_id_dsi = get_lcd_attached_secondary("GET");

		LCD_INFO(vdd, "manufacture_id_dsi = 0x%x\n", vdd->manufacture_id_dsi);

		vdd->panel_func.samsung_panel_revision(vdd);
	}

	/* set manufacture_id_dsi to PBA_ID to read ID later  */
	vdd->manufacture_id_dsi = PBA_ID;

	mutex_init(&vdd->esd_recovery.esd_lock);

	ss_panel_attach_set(vdd, true);

	atomic_set(&vdd->ss_vsync_cnt, 0);
	init_waitqueue_head(&vdd->ss_vsync_wq);

	atomic_set(&vdd->block_commit_cnt, 0);
	init_waitqueue_head(&vdd->block_commit_wq);

	ss_create_sysfs_svc(vdd);

#if IS_ENABLED(CONFIG_SEC_FACTORY)
	vdd->is_factory_mode = true;
#endif

	/* parse display dtsi node */
	ss_panel_parse_dt(vdd);

	/* self display */
	if (vdd->self_disp.is_support) {
		if (vdd->self_disp.init)
			vdd->self_disp.init(vdd);

		if (vdd->self_disp.data_init)
			vdd->self_disp.data_init(vdd);
		else
			LCD_ERR(vdd, "no self display data init function\n");
	}

	/* mafpc */
	if (vdd->mafpc.is_support) {
		if (vdd->mafpc.init)
			vdd->mafpc.init(vdd);

		if (vdd->mafpc.data_init)
			vdd->mafpc.data_init(vdd);
		else
			LCD_ERR(vdd, "no mafpc data init function\n");
	}

	/* support_mdnie is determined in ss_panel_parse_dt() function */
	if (vdd->mdnie.support_mdnie && ss_panel_attached(ndx))
		vdd->mdnie.mdnie_tune_state_dsi = init_dsi_tcon_mdnie_class(vdd);

	ss_copr_init(vdd);

	vdd->nb_reboot.notifier_call = ss_reboot_notifier_cb;
	if (register_reboot_notifier(&vdd->nb_reboot))
		LCD_ERR(vdd, "Failed to register reboot notifier\n");

#if IS_ENABLED(CONFIG_INPUT_SEC_NOTIFIER)
	/* register notifier and init workQ.
	 * TODO: move hall_ic_notifier_display and dyn_mipi_clk notifier here.
	 */
	if (vdd->esd_touch_notify) {
		vdd->nb_esd_touch.priority = 3;
		vdd->nb_esd_touch.notifier_call = ss_esd_touch_notifier_cb;
		sec_input_register_notify(&vdd->nb_esd_touch, ss_esd_touch_notifier_cb, 3);
	}
#endif

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
	/* work thread for panel notifier */
	vdd->notify_wq = create_singlethread_workqueue("panel_notify_wq");
	if (vdd->notify_wq == NULL)
		LCD_ERR(vdd, "failed to create read notify workqueue\n");
	else {
		INIT_WORK(&vdd->bl_event_work, (work_func_t)ss_bl_event_work);
		INIT_WORK(&vdd->vrr_event_work, (work_func_t)ss_vrr_event_work);
		INIT_WORK(&vdd->lfd_event_work, (work_func_t)ss_lfd_event_work);
		INIT_WORK(&vdd->panel_state_event_work, (work_func_t)ss_panel_state_event_work);
		INIT_WORK(&vdd->test_mode_event_work, (work_func_t)ss_test_mode_event_work);
		INIT_WORK(&vdd->screen_mode_event_work, (work_func_t)ss_screen_mode_event_work);
		INIT_WORK(&vdd->esd_event_work, (work_func_t)ss_esd_event_work);
	}
#endif

	/* ESD delayed work */
	INIT_DELAYED_WORK(&vdd->esd_enable_event_work, (work_func_t)ss_esd_enable_event_work);
	INIT_DELAYED_WORK(&vdd->esd_disable_event_work, (work_func_t)ss_esd_disable_event_work);

	/* SDE Clock Work */
	INIT_DELAYED_WORK(&vdd->sde_normal_clk_work, (work_func_t)ss_sde_normal_clk_work);

	/* Smooth BR Done Work */
	INIT_DELAYED_WORK(&vdd->smooth_br_done_work, (work_func_t)ss_smooth_br_done_work);

	/* wakeup source */
	vdd->panel_wakeup_source = wakeup_source_register(&panel->mipi_device.dev, SAMSUNG_DISPLAY_PANEL_NAME);

	mutex_init(&vdd->notify_lock);

	/* init gamma mode2 flash data */
	vdd->br_info.gm2_table.flash_gm2_init_done = false;
	ss_gm2_ddi_flash_init(vdd);

	/* dVRR init */
	ss_dvrr_init(vdd);

	vdd->lp_rx_fail_cnt = 0;

	LCD_INFO(vdd, "window_color : [%s]\n", vdd->window_color);

	LCD_INFO(vdd, "-- \n");
}

void ss_set_vrr_switch(struct samsung_display_driver_data *vdd, bool enable)
{
	/* notify clients that vrr has changed */
	vdd->vrr.param_update = enable;
#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
	ss_notify_queue_work(vdd, PANEL_EVENT_VRR_STATE_CHANGED);
#endif
	return;
}

bool ss_is_tear_case(struct samsung_display_driver_data *vdd)
{

	struct dbg_tear_info *dbg = &vdd->dbg_tear_info;
	ktime_t ts_wr = dbg->ts_frame_start[dbg->pos_frame_start];
	ktime_t ts_te = dbg->ts_frame_te[dbg->pos_frame_te];
	ktime_t ts_ctl_done = dbg->ts_frame_end[dbg->pos_frame_end];
	s64 delta_start;
	s64 delta_end;

	/*
	 * wr_ptr and ctl_done is a pair.
	 * just check last wr_ptr if it came with last TE, or not, regarding to early TE delay.
	 *
	 */
	delta_start = ktime_us_delta(ts_wr, ts_te); // last wr - last te..
	delta_end = ktime_us_delta(ts_ctl_done, ts_te); // last ctl_done - last te..

	if (delta_start < -2000) {
		/* wr_ptr is occurred in previous TE, but ctl_done occurred after last TE */
		if (delta_end > dbg->early_te_delay_us) {
			LCD_INFO(vdd, "end after two TEs: wr=%lld te=%lld delta_s=%lld delta_e=%lld early_te=%d\n",
					ts_wr, ts_te, delta_start, delta_end, dbg->early_te_delay_us);
			return true;
		}
	} else {
		if (delta_end < dbg->early_te_delay_us &&
				dbg->tear_partial_bottom == TEAR_PARTIAL_BOTTOM_SET) {
			/* bottom partial update  case */
			LCD_INFO(vdd, "too fast end: wr=%lld te=%lld delta_s=%lld delta_e=%lld early_te=%d\n",
					ts_wr, ts_te, delta_start, delta_end, dbg->early_te_delay_us);
			return true;
		}
	}

	dbg->tear_partial_bottom = TEAR_PARTIAL_BOTTOM_REL;

	return false;
}

bool ss_is_no_te_case(struct samsung_display_driver_data *vdd)
{

	struct dbg_tear_info *dbg;
	int pos;
	ktime_t ts_te, ts_next_te;
	s64 delta_ms;
	int i;

	if (!vdd) {
		pr_err("SDE: no vdd in checking no te case\n");
		return false;
	}

	dbg = &vdd->dbg_tear_info;
	pos = dbg->pos_frame_te;

	/* GCT, skip */
	if (vdd->gct.is_running)
		return false;

	ts_next_te = dbg->ts_frame_te[pos];
	pos = (pos - 1 + MAX_TS_BUF_CNT) % MAX_TS_BUF_CNT;

	for (i = 1; i < 5; i++, pos = (pos - 1 + MAX_TS_BUF_CNT) % MAX_TS_BUF_CNT) {
		ts_te = dbg->ts_frame_te[pos];
		if (i > 0) {
			delta_ms = ktime_ms_delta(ts_next_te, ts_te);
			if (delta_ms < 145 || delta_ms > 155)
				return false;
		}

		ts_next_te = ts_te;
	}

	return true;
}

int samsung_panel_initialize(char *panel_string, unsigned int ndx)
{
	struct samsung_display_driver_data *vdd;

	vdd = ss_get_vdd(ndx);
	if (!vdd) {
		LCD_ERR(vdd, "vdd is null\n");
		return -1;
	}

	vdd->ndx = ndx;

	LCD_INFO(vdd, "[module] lcd_id = main(0x%s) sub(0x%s)\n", lcd_id, lcd_id1);
	LCD_INFO(vdd, "[module] cell_id = main(0x%s) sub(0x%s)\n", cell_id, cell_id1);

	LCD_INFO(vdd, "panel [%s] ++\n", panel_string);

	if (!strncmp(panel_string, "ss_dsi_panel_PBA_BOOTING_FHD", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = PBA_BOOTING_FHD_init;
#if IS_ENABLED(CONFIG_PANEL_PBA_FHD_DSI1)
	else if (!strncmp(panel_string, "ss_dsi_panel_PBA_BOOTING_FHD_DSI1", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = PBA_BOOTING_FHD_DSI1_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_PA1_S6E3FAE_AMB616FL03_FHD)
	/* PA1 Real Panel */
	else if (!strncmp(panel_string, "PA1_S6E3FAE_AMB616FL03", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = PA1_S6E3FAE_AMB616FL03_FHD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_PA1_S6E3FAE_AMB616FL03_VHM_FHD)
	/* PA1 VHM Real Panel */
	else if (!strncmp(panel_string, "PA1_S6E3FAE_AMB616FL03_VHM", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = PA1_S6E3FAE_AMB616FL03_VHM_FHD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_PA2_S6E3HAF_AMB666FM01_WQHD)
	/* PA2 BringUp Panel */
	else if (!strncmp(panel_string, "PA2_S6E3HAF_AMB666FM01", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = PA2_S6E3HAF_AMB666FM01_WQHD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_PA2_S6E3HAF_AMB666FM03_WQHD)
	/* PA2 Real Panel */
	else if (!strncmp(panel_string, "PA2_S6E3HAF_AMB666FM03", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = PA2_S6E3HAF_AMB666FM03_WQHD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_PA2_S6E3HAF_AMB666FM03_VHM_WQHD)
	/* PA2 VHM Real Panel */
	else if (!strncmp(panel_string, "PA2_S6E3HAF_AMB666FM03_VHM", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = PA2_S6E3HAF_AMB666FM03_VHM_WQHD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_PA3_S6E3HAF_AMB679FN01_WQHD)
	/* PA3 BringUp Panel */
	else if (!strncmp(panel_string, "PA3_S6E3HAF_AMB679FN01", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = PA3_S6E3HAF_AMB679FN01_WQHD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_PA3_S6E3HAF_AMB686HX01_WQHD)
	/* PA3 Real Panel */
	else if (!strncmp(panel_string, "PA3_S6E3HAF_AMB686HX01", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = PA3_S6E3HAF_AMB686HX01_WQHD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_PA3_S6E3HAF_AMB686HX01_VHM_WQHD)
	/* PA3 VHM Real Panel */
	else if (!strncmp(panel_string, "PA3_S6E3HAF_AMB686HX01_VHM", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = PA3_S6E3HAF_AMB686HX01_VHM_WQHD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_A36_ANA6710_AMS663FS02_FHD)
	else if (!strncmp(panel_string, "A36_ANA6710_AMS663FS02", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = A36_ANA6710_AMS663FS02_FHD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_A36_S6E3FC5_AMS663FS01_FHD)
	else if (!strncmp(panel_string, "A36_S6E3FC5_AMS663FS01", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = A36_S6E3FC5_AMS663FS01_FHD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_Q7_S6E3XA5_AMF800GX01_QXGA)
	else if (!strncmp(panel_string, "Q7_S6E3XA5_AMF800GX01", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = Q7_S6E3XA5_AMF800GX01_QXGA_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_Q7_S6E3FAE_AMB649GY01_HD)
	else if (!strncmp(panel_string, "Q7_S6E3FAE_AMB649GY01", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = Q7_S6E3FAE_AMB649GY01_HD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_Q7M_ANA38407_AMSA10FA01_WQXGA)
	else if (!strncmp(panel_string, "Q7M_ANA38407_AMSA10FA01", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = Q7M_ANA38407_AMSA10FA01_WQXGA_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_Q7M_S6E3FAE_AMB649GY01_HD)
	else if (!strncmp(panel_string, "Q7M_S6E3FAE_AMB649GY01", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = Q7M_S6E3FAE_AMB649GY01_HD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_GTA5P_HX83123A_TL101VVMS01_WUXGA)
	else if (!strncmp(panel_string, "GTA5P_HX83123A_TL101VVMS01", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = GTA5P_HX83123A_TL101VVMS01_WUXGA_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_GTA5P_NT31536E_TL101VVMS01_WUXGA)
	else if (!strncmp(panel_string, "GTA5P_NT31536E_TL101VVMS01", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = GTA5P_NT31536E_TL101VVMS01_WUXGA_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_XC7P_NT36672C_TL066FVMC21_FHD)
	else if (!strncmp(panel_string, "XC7P_NT36672C_TL066FVMC21", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = XC7P_NT36672C_TL066FVMC21_FHD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_PS_S6E3HAF_AMB666LH01_WQHD)
	/* PS Panel */
	else if (!strncmp(panel_string, "PS_S6E3HAF_AMB666LH01", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = PS_S6E3HAF_AMB666LH01_WQHD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_PS_S6E3HAF_AMB666LH01_VHM_WQHD)
	/* PS VHM Panel */
	else if (!strncmp(panel_string, "PS_S6E3HAF_AMB666LH01_VHM", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = PS_S6E3HAF_AMB666LH01_VHM_WQHD_init;
#endif
#if IS_ENABLED(CONFIG_PANEL_XC7P_NT36672C_TL066FVMC2101_FHD)
	else if (!strncmp(panel_string, "XC7P_NT36672C_TL066FVMC2101", strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = XC7P_NT36672C_TL066FVMC2101_FHD_init;
#endif

	else {
		LCD_ERR(vdd, "[%s] not found\n", panel_string);
		return -1;
	}

	ss_create_sysfs(vdd);

	if (cell_id || cell_id1)
		set_cell_id_cmdline(vdd);

	LCD_INFO(vdd, "[%s] found --\n", panel_string);
	return 0;
}

char print_buf[1024];
static void ss_print_cmd_op(struct ss_cmd_desc *ss_cmd)
{
	struct ss_cmd_op_str *op_root, *op_sibling;
	int i;
	//char buf[1024] = {0, };
	int len = 0;

	if (list_empty(&ss_cmd->op_list))
		return;

	list_for_each_entry(op_root, &ss_cmd->op_list, list) {
		len = 0;
		memset(print_buf, 0, sizeof(print_buf));

		len += snprintf(print_buf + len, sizeof(print_buf) - len,  "LIST: [%s] [%s] SYMBOL: [%s], VAL: [%s]",
				op_root->op == SS_CMD_OP_NONE ? "OP_NONE" :
					op_root->op == SS_CMD_OP_IF ? "IF" :
					op_root->op == SS_CMD_OP_ELSE ? "ELSE" :
					op_root->op == SS_CMD_OP_IF_BLOCK ? "IF_BLOCK" :
					op_root->op == SS_CMD_OP_UPDATE ? "UPDATE" : "OP INVALID",
				op_root->cond == SS_CMD_OP_COND_AND ? "AND" : "OR",
				op_root->symbol ? op_root->symbol : "NULL",
				op_root->val ? op_root->val : "NULL");

		if (op_root->candidate_len > 0) {
			len += snprintf(print_buf + len, sizeof(print_buf) - len,  " , CANDIDATE(len=%lu), [",
					op_root->candidate_len);
			for (i = 0; i < op_root->candidate_len; i++)
				len += snprintf(print_buf + len, sizeof(print_buf) - len,  "0x%02x ",
						op_root->candidate_buf[i]);
			len += snprintf(print_buf + len, sizeof(print_buf) - len,  "]");
		}

		if (op_root->op == SS_CMD_OP_IF_BLOCK)
			len += snprintf(print_buf + len, sizeof(print_buf) - len,  " , [%s]",
					ss_cmd->skip_by_cond ? "SKIP" : "NON SKIP");

		pr_info("  SDE: %s\n", print_buf);

		list_for_each_entry(op_sibling, &op_root->sibling, sibling) {
			pr_info("    SDE: SIBLING: [%s] [%s] SYMBOL: [%s], VAL: [%s]\n",
					op_sibling->op == SS_CMD_OP_NONE ? "OP_NONE" :
						op_sibling->op == SS_CMD_OP_IF ? "IF" :
						op_sibling->op == SS_CMD_OP_ELSE ? "ELSE" :
						op_sibling->op == SS_CMD_OP_IF_BLOCK ? "IF_BLOCK" :
						op_sibling->op == SS_CMD_OP_UPDATE ? "UPDATE" : "OP INVALID",
					op_sibling->cond == SS_CMD_OP_COND_AND ? "AND" : "OR",
					op_sibling->symbol ? op_sibling->symbol : "NULL",
					op_sibling->val ? op_sibling->val : "NULL");
		}
	}
}

void ss_print_ss_cmd_set(struct samsung_display_driver_data *vdd,
					struct ss_cmd_set *set)
{
	int len;
	size_t i, cnt;
	struct ss_cmd_desc *cmd;

	LCD_INFO(vdd, "is_skip_tot_lvl: %d, count: %d\n", set->is_skip_tot_lvl, set->count);

	for (cnt = 0; cnt < set->count; cnt++) {
		cmd = &set->cmds[cnt];

		/* Packet Info */
		len = 0;
		//memset(buf, 0, 1024);
		memset(print_buf, 0, 1024);

		len += snprintf(print_buf + len, sizeof(print_buf) - len,  "type: %02X, ", cmd->type);
		len += snprintf(print_buf + len, sizeof(print_buf) - len, "wait_ms: %d, ", cmd->post_wait_ms); /* Delay */
		len += snprintf(print_buf + len, sizeof(print_buf) - len, "wait_frame: %d, ", cmd->post_wait_frame); /* Delay */
		len += snprintf(print_buf + len, sizeof(print_buf) - len, "len: %zd, txbuf: ", cmd->tx_len);

		/* Packet Payload */
		for (i = 0 ; i < cmd->tx_len; i++) {
			len += snprintf(print_buf + len, sizeof(print_buf) - len, "%02X ", cmd->txbuf[i]);
			/* Break to prevent show too long command */
			if (i > 100)
				break;
		}

		LCD_INFO(vdd, "%s\n", print_buf);
		ss_print_cmd_op(cmd);
	}
}

void spsram_read_bytes(struct samsung_display_driver_data *vdd, int addr, int rsize, u8 *buf)
{
	struct dsi_panel_cmd_set *tx_sram_offset_cmds = ss_get_cmds(vdd, TX_SPSRAM_DATA_READ);
	struct dsi_panel_cmd_set *rx_gamma_cmds = ss_get_cmds(vdd, RX_FLASH_GAMMA);
	bool gpara_temp;
	struct vrr_info *vrr = &vdd->vrr;
	int cur_rr, delay;

	if (SS_IS_CMDS_NULL(tx_sram_offset_cmds) || SS_IS_CMDS_NULL(rx_gamma_cmds)) {
		LCD_INFO(vdd, "No cmds for TX_GLUT_SRAM_OFFSET or RX_FLASH_GAMMA..\n");
		return;
	}

	if (vrr)
		cur_rr = vrr->cur_refresh_rate;
	else
		cur_rr = 60;

	tx_sram_offset_cmds->cmds[0].ss_txbuf[2] = (addr & 0xFF00) >> 8;
	tx_sram_offset_cmds->cmds[0].ss_txbuf[3] = addr & 0xFF;

	tx_sram_offset_cmds->cmds[0].ss_txbuf[4] = (rsize & 0xFF00) >> 8;
	tx_sram_offset_cmds->cmds[0].ss_txbuf[5] = rsize & 0xFF;

	ss_send_cmd(vdd, TX_SPSRAM_DATA_READ);

	/* need 1frame delay between 71h and 6Eh */
	delay = ss_frame_delay(cur_rr, 1);
	usleep_range(delay * 1000, delay * 1000);

	/* RX - Flash read
	 * do not send level key.. */
	rx_gamma_cmds->state = DSI_CMD_SET_STATE_HS;
	rx_gamma_cmds->cmds[0].msg.rx_len = rsize;
	rx_gamma_cmds->cmds[0].msg.rx_buf = buf;

	gpara_temp = vdd->gpara;

	/* Do not use gpara to read flash via MIPI */
	vdd->gpara = false;
	ss_send_cmd_get_rx(vdd, RX_FLASH_GAMMA, buf);

	usleep_range(delay * 1000, delay * 1000);

	vdd->gpara = gpara_temp;
}

#define MAX_READ_BUF_SIZE	(20)
static u8 read_buf[MAX_READ_BUF_SIZE];

bool ss_ddi_id_read(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *pcmds;
	int i, len = 0;
	u8 temp[128];
	int rx_len;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	pcmds = ss_get_cmds(vdd, RX_DDI_ID);
	if (pcmds->count <= 0) {
		LCD_ERR(vdd, "DSI%d no ddi_id_rx_cmds cmds", vdd->ndx);
		return false;
	}

	rx_len = ss_send_cmd_get_rx(vdd, RX_DDI_ID, read_buf);
	if (rx_len < 0 || rx_len > MAX_READ_BUF_SIZE) {
		LCD_ERR(vdd, "invalid rx_len(%d)\n", rx_len);
		return false;
	}

	for (i = 0; i < rx_len; i++)
		len += sprintf(temp + len, "%02x", read_buf[i]);
	len += sprintf(temp + len, "\n");

	vdd->ddi_id_dsi = kzalloc(len, GFP_KERNEL);
	if (!vdd->ddi_id_dsi) {
		LCD_ERR(vdd, "fail to kzalloc for ddi_id_dsi\n");
		return false;
	}

	vdd->ddi_id_len = len;
	strlcat(vdd->ddi_id_dsi, temp, len);

	LCD_INFO(vdd, "[%d] %s\n", vdd->ddi_id_len, vdd->ddi_id_dsi);

	return true;
}

bool ss_octa_id_read(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *pcmds;
	char temp[128];
	int len = 0;
	int rx_len;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	pcmds = ss_get_cmds(vdd, RX_OCTA_ID);
	if (pcmds->count <= 0) {
		LCD_ERR(vdd, "DSI%d no octa_id_rx_cmds cmd\n", vdd->ndx);
		return false;
	}

	rx_len = ss_send_cmd_get_rx(vdd, RX_OCTA_ID, read_buf);
	if (rx_len < 0 || rx_len > MAX_READ_BUF_SIZE) {
		LCD_ERR(vdd, "invalid rx_len(%d)\n", rx_len);
		return false;
	}

	len += sprintf(temp + len, "%d", (read_buf[0] & 0xF0) >> 4);
	len += sprintf(temp + len, "%d", (read_buf[0] & 0x0F));
	len += sprintf(temp + len, "%d", (read_buf[1] & 0x0F));
	len += sprintf(temp + len, "%02x", read_buf[2]);
	len += sprintf(temp + len, "%02x", read_buf[3]);

	len += sprintf(temp + len, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
			read_buf[4], read_buf[5], read_buf[6], read_buf[7],
			read_buf[8], read_buf[9], read_buf[10], read_buf[11],
			read_buf[12], read_buf[13], read_buf[14], read_buf[15],
			read_buf[16], read_buf[17], read_buf[18], read_buf[19]);

	vdd->octa_id_dsi = kzalloc(len, GFP_KERNEL);
	if (!vdd->octa_id_dsi) {
		LCD_ERR(vdd, "fail to kzalloc for octa_id_dsi\n");
		return false;
	}

	vdd->octa_id_len = len;
	strlcat(vdd->octa_id_dsi, temp, vdd->octa_id_len);
	LCD_INFO(vdd, "octa id : [%d] %s \n", vdd->octa_id_len, vdd->octa_id_dsi);

	return true;
}

void ss_block_commit(struct samsung_display_driver_data *vdd)
{
	int interval_us;

	atomic_inc(&vdd->block_commit_cnt);

	interval_us = (1000000 / vdd->vrr.cur_refresh_rate) + 1000; /* 1ms dummy */
	usleep_range(interval_us, interval_us); /* wait for previous commit done */

	ss_wait_for_kickoff_done(vdd);

	/* Incase of VHM, additional kickoff by DPU_MIN should be considered */
	if (vdd->esync_support)
		msleep(100);
}

void ss_release_commit(struct samsung_display_driver_data *vdd)
{
	atomic_add_unless(&vdd->block_commit_cnt, -1, 0);
	wake_up_all(&vdd->block_commit_wq);
}

int ss_check_panel_connection(struct samsung_display_driver_data *vdd)
{
	if (!vdd) {
		LCD_ERR_NOVDD("invalid vdd (%pS)\n", __builtin_return_address(0));
		return -ENODEV;
	}

	if (!ss_panel_attach_get(vdd)) {
		LCD_INFO(vdd, "PBA booting (%pS)\n", __builtin_return_address(0));

		if (ss_is_ub_connected(vdd) == UB_CONNECTED)
			LCD_ERR(vdd, "PBA booting but UB connected (mipi error)\n");

		return -EPERM;
	}

	if (vdd->is_factory_mode && !ss_is_ub_connected(vdd)) {
		LCD_INFO(vdd, "ub disconnected (%pS)\n", __builtin_return_address(0));
		return -EPERM;
	}

	return 0;
}

/* TBR: remove below function, and separate parsing powers and getting power resource parts */
struct samsung_display_driver_data *ss_get_vdd_from_panel_name(const char *panel_name)
{
	struct samsung_display_driver_data *vdd = NULL;
	char primary_name[128];
	char seconary_name[128];

	ss_get_primary_panel_name_cmdline(primary_name);
	ss_get_secondary_panel_name_cmdline(seconary_name);

	if (!strcmp(panel_name, primary_name))
		vdd = ss_get_vdd(PRIMARY_DISPLAY_NDX);
	else if (!strcmp(panel_name, seconary_name))
		vdd = ss_get_vdd(SECONDARY_DISPLAY_NDX);
	else
		LCD_ERR_NOVDD("fail to find panel name[%s] cmdline=[%s],[%s]\n",
				panel_name, primary_name, seconary_name);

	return vdd;
}
