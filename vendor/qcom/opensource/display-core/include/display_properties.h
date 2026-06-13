/*
* Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
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

/*
* Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
* Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __DISPLAY_PROPERTIES_H__
#define __DISPLAY_PROPERTIES_H__

// clang-format off
#define DISP_PROP_PREFIX                     "vendor.display."
#define GRALLOC_PROP_PREFIX                  "vendor.gralloc."
#define PERSIST_DISP_PROP_PREFIX             "persist.vendor.display."

#define DISPLAY_PROP(prop_name)              DISP_PROP_PREFIX prop_name
#define GRALLOC_PROP(prop_name)              GRALLOC_PROP_PREFIX prop_name
#define PERSIST_DISPLAY_PROP(prop_name)      PERSIST_DISP_PROP_PREFIX prop_name

// Start of property
#define COMPOSITION_MASK_PROP                DISPLAY_PROP("comp_mask")
#define HDMI_CONFIG_INDEX_PROP               DISPLAY_PROP("hdmi_cfg_idx")
#define IDLE_TIME_PROP                       DISPLAY_PROP("idle_time")
#define IDLE_TIME_INACTIVE_PROP              DISPLAY_PROP("idle_time_inactive")
#define DISABLE_ROTATOR_DOWNSCALE_PROP       DISPLAY_PROP("disable_rotator_downscale")
// Enables rotator for UI
#define ENABLE_ROTATOR_UI_PROP               DISPLAY_PROP("enable_rotator_ui")
#define DISABLE_DECIMATION_PROP              DISPLAY_PROP("disable_decimation")
#define PRIMARY_MIXER_STAGES_PROP            DISPLAY_PROP("primary_mixer_stages")
#define EXTERNAL_MIXER_STAGES_PROP           DISPLAY_PROP("external_mixer_stages")
#define VIRTUAL_MIXER_STAGES_PROP            DISPLAY_PROP("virtual_mixer_stages")
#define MAX_UPSCALE_PROP                     DISPLAY_PROP("max_upscale")
#define VIDEO_MODE_PANEL_PROP                DISPLAY_PROP("video_mode_panel")
#define DISABLE_ROTATOR_UBWC_PROP            DISPLAY_PROP("disable_rotator_ubwc")
#define DISABLE_ROTATOR_SPLIT_PROP           DISPLAY_PROP("disable_rotator_split")
#define DISABLE_SCALER_PROP                  DISPLAY_PROP("disable_scaler")
#define DISABLE_AVR_PROP                     DISPLAY_PROP("disable_avr")
#define DISABLE_EXTERNAL_ANIMATION_PROP      DISPLAY_PROP("disable_ext_anim")
#define DISABLE_PARTIAL_SPLIT_PROP           DISPLAY_PROP("disable_partial_split")
#define PREFER_SOURCE_SPLIT_PROP             DISPLAY_PROP("prefer_source_split")
#define MIXER_RESOLUTION_PROP                DISPLAY_PROP("mixer_resolution")
#define NULL_DISPLAY_RESOLUTION_PROP         DISPLAY_PROP("null_display_resolution")
#define SIMULATED_CONFIG_PROP                DISPLAY_PROP("simulated_config")
#define MAX_SECONDARY_FETCH_LAYERS_PROP      DISPLAY_PROP("max_secondary_fetch_layers")
#define ENABLE_EXTERNAL_DOWNSCALE_PROP       DISPLAY_PROP("enable_external_downscale")
#define EXTERNAL_ACTION_SAFE_WIDTH_PROP      DISPLAY_PROP("external_action_safe_width")
#define EXTERNAL_ACTION_SAFE_HEIGHT_PROP     DISPLAY_PROP("external_action_safe_height")
#define FB_WIDTH_PROP                        DISPLAY_PROP("fb_width")
#define FB_HEIGHT_PROP                       DISPLAY_PROP("fb_height")
#define DISABLE_METADATA_DYNAMIC_FPS_PROP    DISPLAY_PROP("disable_metadata_dynamic_fps")
#define DISABLE_SKIP_VALIDATE_PROP           DISPLAY_PROP("disable_skip_validate")
#define DISABLE_DESTINATION_SCALER_PROP      DISPLAY_PROP("disable_dest_scaler")
#define DISABLE_SECURE_INLINE_ROTATOR_PROP   DISPLAY_PROP("disable_secure_inline_rotator")
#define DISABLE_MULTIRECT_PROP               DISPLAY_PROP("disable_multirect")
#define DISABLE_UBWC_FF_VOTING_PROP          DISPLAY_PROP("disable_ubwc_ff_voting")
#define DISABLE_INLINE_ROTATOR_PROP          DISPLAY_PROP("disable_inline_rotator")
#define DISABLE_OFFLINE_ROTATOR_PROP         DISPLAY_PROP("disable_offline_rotator")
#define DISABLE_FB_CROPPING_PROP             DISPLAY_PROP("disable_fb_cropping")
#define PRIORITIZE_CACHE_COMPOSITION_PROP    DISPLAY_PROP("prioritize_cache_comp")
#define DISABLE_HW_RECOVERY_PROP             DISPLAY_PROP("disable_hw_recovery")
#define DISABLE_HW_RECOVERY_DUMP_PROP        DISPLAY_PROP("disable_hw_recovery_dump")
#define HW_RECOVERY_THRESHOLD                DISPLAY_PROP("hw_recovery_threshold")
#define DISABLE_SRC_TONEMAP_PROP             DISPLAY_PROP("disable_src_tonemap")
#define ENABLE_NULL_DISPLAY_PROP             DISPLAY_PROP("enable_null_display")
#define DISABLE_EXCL_RECT_PROP               DISPLAY_PROP("disable_excl_rect")
#define ENABLE_PIPE_PRIORITY_PROP            DISPLAY_PROP("enable_pipe_priority")
#define DISABLE_EXCl_RECT_PARTIAL_FB         DISPLAY_PROP("disable_excl_rect_partial_fb")
#define DISABLE_FBID_CACHE                   DISPLAY_PROP("disable_fbid_cache")
#define DISABLE_HOTPLUG_BWCHECK              DISPLAY_PROP("disable_hotplug_bwcheck")
#define DISABLE_MASK_LAYER_HINT              DISPLAY_PROP("disable_mask_layer_hint")
#define DISABLE_HDR_LUT_GEN                  DISPLAY_PROP("disable_hdr_lut_gen")
#define DISABLE_HDR                          DISPLAY_PROP("hwc_disable_hdr")
#define QDCM_PCC_TRANS_PROP                  DISPLAY_PROP("qdcm.pcc_for_trans")
#define QDCM_DIAGONAL_MATRIXMODE_PROP        DISPLAY_PROP("qdcm.diagonal_matrix_mode")
#define QDCM_MODE_COMBINE_PROP               DISPLAY_PROP("qdcm.mode_combine")
#define DISABLE_STC_DIMMING_PROP             DISPLAY_PROP("disable_stc_dimming")
#define PREFER_MULTIRECT_PROP                DISPLAY_PROP("prefer_multirect")
#define DROP_SKEWED_VSYNC                    DISPLAY_PROP("drop_skewed_vsync")
#define DISABLE_FAST_PATH                    DISPLAY_PROP("disable_fast_path")
#define DISABLE_SYSTEM_LOAD_CHECK            DISPLAY_PROP("disable_system_load_check")
#define ASPECT_RATIO_THRESHOLD               DISPLAY_PROP("aspect_ratio_threshold")
#define ENABLE_BRIGHTNESS_DRM_PROP           DISPLAY_PROP("enable_brightness_drm_prop")
// Disable microidle condition
#define DISABLE_SINGLE_LM_SPLIT_PROP         DISPLAY_PROP("disable_single_lm_split")
// Enable posted start dynamic
#define ENABLE_POSTED_START_DYN_PROP         DISPLAY_PROP("enable_posted_start_dyn")
#define ENABLE_OPTIMIZE_REFRESH              DISPLAY_PROP("enable_optimize_refresh")
#define DISABLE_PARALLEL_CACHE               DISPLAY_PROP("disable_parallel_cache")
#define DISABLE_LAYER_STITCH                 DISPLAY_PROP("disable_layer_stitch")
// Disable 3d tonemap support for UI layers
#define DISABLE_UI_3D_TONEMAP                DISPLAY_PROP("disable_ui_3d_tonemap")
#define QDCM_DISABLE_FACTORY_MODE_PROP       DISPLAY_PROP("qdcm.disable_factory_mode")
#define ENABLE_GPU_TONEMAPPER_PROP           DISPLAY_PROP("enable_gpu_tonemapper")
#define ENABLE_FORCE_SPLIT                   DISPLAY_PROP("enable_force_split")
#define DISABLE_GPU_COLOR_CONVERT            DISPLAY_PROP("disable_gpu_color_convert")
#define ENABLE_ASYNC_VDS_CREATION            DISPLAY_PROP("enable_async_vds_creation")
#define ENABLE_HISTOGRAM_INTR                DISPLAY_PROP("enable_hist_intr")
#define DISABLE_MMRM_PROP                    DISPLAY_PROP("disable_mmrm_prop")
#define DEFER_FPS_FRAME_COUNT                DISPLAY_PROP("defer_fps_frame_count")
#define DISABLE_ROTATOR_PRE_DOWNSCALER_PROP  DISPLAY_PROP("disable_pre_downscaler")
#define DISABLE_INLINE_ROTATOR_UI_PROP       DISPLAY_PROP("disable_inline_rotator_ui")
#define ENABLE_POMS_DURING_DOZE              DISPLAY_PROP("enable_poms_during_doze")
// Disable 3D adaptive tone mapping support
#define DISABLE_3D_ADAPTIVE_TM               DISPLAY_PROP("disable_3d_adaptive_tm")
#define DISABLE_SDR_HISTOGRAM                DISPLAY_PROP("disable_sdr_histogram")
#define ALLOW_3D_ADAPTIVE_TM_EXTERNAL        DISPLAY_PROP("allow_3d_adaptive_tm_external")
// Disable SDR dimming support
#define DISABLE_SDR_DIMMING                  DISPLAY_PROP("disable_sdr_dimming")
#define FORCE_TONEMAPPING                    DISPLAY_PROP("force_tonemapping")
// Allows color management(tonemapping) in native mode (native mode is considered BT709+sRGB)
#define ALLOW_TONEMAP_NATIVE                 DISPLAY_PROP("allow_tonemap_native")
#define ENABLE_METADATA_DUMPING              DISPLAY_PROP("enable_metadata_dump")

// Disable power saving features
#define DISABLE_DPPS_FEATURES               DISPLAY_PROP("disable_dpps_features")

// RC
#define ENABLE_ROUNDED_CORNER                DISPLAY_PROP("enable_rounded_corner")
#define DISABLE_ROUNDED_CORNER_THREAD        DISPLAY_PROP("disable_rounded_corner_thread")
// Disable A8 format Screen Decorator support
#define DISABLE_GET_SCREEN_DECORATOR_SUPPORT DISPLAY_PROP("disable_get_screen_decorator_support")

// SPR
#define ENABLE_SPR                           DISPLAY_PROP("enable_spr")
#define ENABLE_SPR_BYPASS                    DISPLAY_PROP("enable_spr_bypass")
#define ENABLE_MEMORY_MAPPING                DISPLAY_PROP("enable_memory_mapping")
#define DISABLE_SPR_PRIMARY                  DISPLAY_PROP("disable_spr_primary")
#define DISABLE_SPR_SECONDARY                DISPLAY_PROP("disable_spr_secondary")

// Panel Feature Demura Properties
#define ENABLE_DEMURA                        DISPLAY_PROP("enable_demura")
#define DISABLE_DEMURA_PRIMARY               DISPLAY_PROP("disable_demura_primary")
#define DISABLE_DEMURA_SECONDARY             DISPLAY_PROP("disable_demura_secondary")
#define DISABLE_DEMURA_PANEL_REPLACEMENT     DISPLAY_PROP("disable_demura_panel_replacement")
#define DISABLE_DEMURA_OPT_SINGLELM          DISPLAY_PROP("disable_demura_opt_singlelm")
#define DEMURA_PRIMARY_PANEL_OVERRIDE_LOW    DISPLAY_PROP("demura_primary_panel_override_low")
#define DEMURA_PRIMARY_PANEL_OVERRIDE_HIGH    DISPLAY_PROP("demura_primary_panel_override_high")
#define DEMURA_SECONDARY_PANEL_OVERRIDE_LOW    DISPLAY_PROP("demura_secondary_panel_override_low")
#define DEMURA_SECONDARY_PANEL_OVERRIDE_HIGH    DISPLAY_PROP("demura_secondary_panel_override_high")
#define ENABLE_DEMURA_BL_SCREEN_REFRESH      DISPLAY_PROP("enable_demura_backlight_screen_refresh")
// Panel Feature DemuraTn Properties
#define ENABLE_ANTI_AGING                    DISPLAY_PROP("enable_antiaging")
#define ENABLE_ACCELERATED_AGING             DISPLAY_PROP("enable_accelerated_aging")
#define ANTI_AGING_SHORT_TIMER               DISPLAY_PROP("demura_short_timer")
#define ANTI_AGING_LONG_TIMER                DISPLAY_PROP("demura_long_timer")
#define ANTI_AGING_RECALIB_TIMER             DISPLAY_PROP("demura_recalib_timer")
#define ANTI_AGING_RECORD_TIMER              DISPLAY_PROP("demura_record_timer")
#define ANTI_AGING_IDLE_TIMER                DISPLAY_PROP("demura_idle_timer")
#define ANTI_AGING_MEMORY_SIZE               DISPLAY_PROP("demura_memory_size")
#define ANTI_AGING_RECALIB_TIMER_DIVIDER     DISPLAY_PROP("demura_recalib_timer_divider")
#define ANTI_AGING_ENABLE_LOGGING            DISPLAY_PROP("antiaging_enable_logging")

// PERF hint properties
#define ENABLE_PERF_HINT_LARGE_COMP_CYCLE    DISPLAY_PROP("enable_perf_hint_large_comp_cycle")
#define LARGE_COMP_HINT_THRESHOLD            DISPLAY_PROP("large_comp_hint_threshold")
#define DISABLE_DYNAMIC_FPS                  DISPLAY_PROP("disable_dynamic_fps")
#define ENABLE_QSYNC_IDLE                    DISPLAY_PROP("enable_qsync_idle")
#define ENHANCE_IDLE_TIME                    DISPLAY_PROP("enhance_idle_time")

#define MMRM_FLOOR_CLK_VOTE                  DISPLAY_PROP("mmrm_floor_vote")

// DPPS dynamic fps
#define ENABLE_DPPS_DYNAMIC_FPS              DISPLAY_PROP("enable_dpps_dynamic_fps")
// Noise Layer
#define DISABLE_NOISE_LAYER                  DISPLAY_PROP("disable_noise_layer")
#define ENABLE_PRIMARY_RECONFIG_REQUEST      DISPLAY_PROP("enable_primary_reconfig_request")
// SDM verbose logging
#define ENABLE_VERBOSE_LOG                   DISPLAY_PROP("enable_verbose_log")
// HDR10 GPU Target
#define ENABLE_HDR10_GPU_TARGET              DISPLAY_PROP("enable_hdr10_gpu_target")
#define MAX_SCALE_FACTOR_FOR_HDR_CLIENT      DISPLAY_PROP("max_scale_factor_for_hdr_client")
// Client target bit depth
#define CLIENT_TARGET_BIT_DEPTH              DISPLAY_PROP("client_target_bit_depth")

// Enable IWE
#define ENABLE_INLINE_WRITEBACK              DISPLAY_PROP("enable_inline_writeback")

// Configuration to reserve the writeback blocks for specific usage like CWB, IWE or WFD
#define RESERVED_WRITEBACK_CONFIG            DISPLAY_PROP("reserved_writeback_config")

// Property to set desired libscale optimization mode on source
#define SCALING_SOURCE_OPT_MODE              DISPLAY_PROP("scaling_source_opt_mode")
// Property to set desired libscale optimization mode on destination
#define SCALING_DEST_OPT_MODE                DISPLAY_PROP("scaling_dest_opt_mode")

//AIQE Properties
#define AIQE_SSRC_ENABLE               DISPLAY_PROP("enable_aiqe_ssrc")
#define AIQE_SSRC_DEFAULT_MODE         DISPLAY_PROP("default_aiqe_ssrc_mode")
#define AIQE_COPR_ENABLE               DISPLAY_PROP("enable_aiqe_copr")

// Add all vendor.display properties above

#define DISABLE_AHARDWARE_BUFFER_PROP        GRALLOC_PROP("disable_ahardware_buffer")
#define DISABLE_UBWC_PROP                    GRALLOC_PROP("disable_ubwc")
#define ENABLE_LOGS_PROP                     GRALLOC_PROP("enable_logs")
#define SECURE_PREVIEW_BUFFER_FORMAT_PROP    GRALLOC_PROP("secure_preview_buffer_format")
#define SECURE_PREVIEW_ONLY_PROP             GRALLOC_PROP("secure_preview_only")
#define USE_DMA_BUF_HEAPS_PROP               GRALLOC_PROP("use_dma_buf_heaps")
#define USE_SYSTEM_HEAP_FOR_SENSORS_PROP     GRALLOC_PROP("use_system_heap_for_sensors")
#define HW_SUPPORTS_UBWCP                    GRALLOC_PROP("hw_supports_ubwcp")
#define ENABLE_SNAPALLOC_PROP                GRALLOC_PROP("enable_snapalloc")
#define ENABLE_ALLOCATION_DATA_DUMPING       GRALLOC_PROP("enable_allocation_data_dumping")

// Add all vendor.gralloc.properties above

#define QDCM_DISABLE_TIMEOUT_PROP            PERSIST_DISPLAY_PROP("qdcm.disable_timeout")

// Add all persist.vendor.display.properties above

#define ZERO_SWAP_INTERVAL                   "vendor.debug.egl.swapinterval"
#define WINDOW_RECT_PROP                     DISPLAY_PROP("window_rect")
#define WINDOW_RECT_PROP_SECONDARY           DISPLAY_PROP("window_rect_secondary")
#define ENABLE_WINDOW_RECT_MASK              DISPLAY_PROP("enable_window_rect_mask")
#define DISABLE_IDLE_TIME_HDR                DISPLAY_PROP("disable_idle_time_hdr")
#define DISABLE_IDLE_TIME_VIDEO              DISPLAY_PROP("disable_idle_time_video")
#define DISABLE_IDLE_FPS_SWITCH              DISPLAY_PROP("disable_idle_fps_switch")
#define DISABLE_IDLE_SCALING_LAYERS          DISPLAY_PROP("disable_idle_scaling_layers")
#define DISABLE_LLCC_DURING_AOD              DISPLAY_PROP("disable_llcc_during_aod")
#define DISABLE_CWB_IDLE_FALLBACK            DISPLAY_PROP("disable_cwb_idle_fallback")
#define PRIORITIZE_CLIENT_CWB                DISPLAY_PROP("prioritize_client_cwb")
#define ENABLE_CWB_CPU_BOOSTING              DISPLAY_PROP("enable_cwb_cpu_boosting")
#define DISABLE_CWB_OPTIMIZATION             DISPLAY_PROP("disable_cwb_optimization")
#define FORCE_REFRESH_TO_PROCESS_CWB         DISPLAY_PROP("force_refresh_to_process_cwb")
#define ENABLE_CLIENT_CONTROL_CWB_REFRESH    DISPLAY_PROP("enable_client_control_cwb_refresh")
#define TRANSIENT_FPS_CYCLE_COUNT            DISPLAY_PROP("transient_fps_cycle_count")
#define CORE_ID_MASK                         DISPLAY_PROP("core_id_mask")
#define XR_VARIANT                           DISPLAY_PROP("xr_variant")
#define ENABLE_ROUNDED_CORNER_CONFIG_DUMP    DISPLAY_PROP("enable_rounded_corner_config_dump")
#define DISABLE_ROUNDED_CORNER_DEST_SCALER   DISPLAY_PROP("disable_rounded_corner_dest_scaler")
#define DISABLE_ROUNDED_CORNER_TUI           DISPLAY_PROP("disable_rounded_corner_tui")
#define DISABLE_RCMASK_SIMD_OPT              DISPLAY_PROP("disable_rcmask_simd_opt")
#define IDLE_FALLBACK_ON_DSPP                DISPLAY_PROP("idle_fallback_on_dspp")
#define FORCE_LM_TO_FB_CONFIG                DISPLAY_PROP("force_lm_to_fb_config")

// Enable AI Scaler
#define ENABLE_AI_SCALER_PROP                DISPLAY_PROP("enable_ai_scaler")

// Panel Feature AIQE ABC Properties
#define ENABLE_ABC                        DISPLAY_PROP("enable_abc")
#define DISABLE_ABC_PRIMARY               DISPLAY_PROP("disable_abc_primary")
#define DISABLE_ABC_SECONDARY             DISPLAY_PROP("disable_abc_secondary")

// Add all other.properties above
// End of property
// clang-format on
#endif  // __DISPLAY_PROPERTIES_H__
