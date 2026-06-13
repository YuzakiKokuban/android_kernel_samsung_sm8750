/*
* Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*    * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above
*      copyright notice, this list of conditions and the following
*      disclaimer in the documentation and/or other materials provided
*      with the distribution.
*    * Neither the name of The Linux Foundation nor the names of its
*      contributors may be used to endorse or promote products derived
*      from this software without specific prior written permission.

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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following
 * license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of
 * its contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "drm_property.h"

namespace sde_drm {

// clang-format off
DRMProperty DRMPropertyManager::GetPropertyEnum(const std::string &name) const {
  if (name == "type") { return DRMProperty::TYPE; }
  if (name == "FB_ID") { return DRMProperty::FB_ID; }
  if (name == "rot_fb_id") { return DRMProperty::ROT_FB_ID; }
  if (name == "CRTC_ID") { return DRMProperty::CRTC_ID; }
  if (name == "CRTC_X") { return DRMProperty::CRTC_X; }
  if (name == "CRTC_Y") { return DRMProperty::CRTC_Y; }
  if (name == "CRTC_W") { return DRMProperty::CRTC_W; }
  if (name == "CRTC_H") { return DRMProperty::CRTC_H; }
  if (name == "SRC_X") { return DRMProperty::SRC_X; }
  if (name == "SRC_Y") { return DRMProperty::SRC_Y; }
  if (name == "SRC_W") { return DRMProperty::SRC_W; }
  if (name == "SRC_H") { return DRMProperty::SRC_H; }
  if (name == "zpos") { return DRMProperty::ZPOS; }
  if (name == "alpha") { return DRMProperty::ALPHA; }
  if (name == "excl_rect_v1") { return DRMProperty::EXCL_RECT; }
  if (name == "h_decimate") { return DRMProperty::H_DECIMATE; }
  if (name == "v_decimate") { return DRMProperty::V_DECIMATE; }
  if (name == "input_fence") { return DRMProperty::INPUT_FENCE; }
  if (name == "rotation") { return DRMProperty::ROTATION; }
  if (name == "blend_op") { return DRMProperty::BLEND_OP; }
  if (name == "src_config") { return DRMProperty::SRC_CONFIG; }
  if (name == "scaler_v1") { return DRMProperty::SCALER_V1; }
  if (name == "scaler_v2") { return DRMProperty::SCALER_V2; }
  if (name == "csc_v1") { return DRMProperty::CSC_V1; }
  if (name == "capabilities") { return DRMProperty::CAPABILITIES; }
  if (name == "mode_properties") { return DRMProperty::MODE_PROPERTIES; }
  if (name == "lut_ed") { return DRMProperty::LUT_ED; }
  if (name == "lut_cir") { return DRMProperty::LUT_CIR; }
  if (name == "lut_sep") { return DRMProperty::LUT_SEP; }
  if (name == "rot_caps_v1") { return DRMProperty::ROTATOR_CAPS_V1; }
  if (name == "true_inline_rot_rev") { return DRMProperty::TRUE_INLINE_ROT_REV; }
  if (name == "fb_translation_mode") { return DRMProperty::FB_TRANSLATION_MODE; }
  if (name == "ACTIVE") { return DRMProperty::ACTIVE; }
  if (name == "MODE_ID") { return DRMProperty::MODE_ID; }
  if (name == "output_fence_offset") { return DRMProperty::OUTPUT_FENCE_OFFSET; }
  if (name == "output_fence") { return DRMProperty::OUTPUT_FENCE; }
  if (name == "sde_drm_roi_v1") { return DRMProperty::ROI_V1; }
  if (name == "core_clk") { return DRMProperty::CORE_CLK; }
  if (name == "core_ab") { return DRMProperty::CORE_AB; }
  if (name == "core_ib") { return DRMProperty::CORE_IB; }
  if (name == "llcc_ab") { return DRMProperty::LLCC_AB; }
  if (name == "llcc_ib") { return DRMProperty::LLCC_IB; }
  if (name == "dram_ab") { return DRMProperty::DRAM_AB; }
  if (name == "dram_ib") { return DRMProperty::DRAM_IB; }
  if (name == "rot_prefill_bw") { return DRMProperty::ROT_PREFILL_BW; }
  if (name == "rot_clk") { return DRMProperty::ROT_CLK; }
  if (name == "security_level") { return DRMProperty::SECURITY_LEVEL; }
  if (name == "dim_layer_v1") { return DRMProperty::DIM_STAGES_V1; }
  if (name == "idle_time") { return DRMProperty::IDLE_TIME; }
  if (name == "RETIRE_FENCE") { return DRMProperty::RETIRE_FENCE; }
  if (name == "RETIRE_FENCE_OFFSET") { return DRMProperty::RETIRE_FENCE_OFFSET; }
  if (name == "DST_X") { return DRMProperty::DST_X; }
  if (name == "DST_Y") { return DRMProperty::DST_Y; }
  if (name == "DST_W") { return DRMProperty::DST_W; }
  if (name == "DST_H") { return DRMProperty::DST_H; }
  if (name == "LP") { return DRMProperty::LP; }
  if (name == "dest_scaler") { return DRMProperty::DEST_SCALER; }
  if (name == "ds_lut_ed") { return DRMProperty::DS_LUT_ED; }
  if (name == "ds_lut_cir") { return DRMProperty::DS_LUT_CIR; }
  if (name == "ds_lut_sep") { return DRMProperty::DS_LUT_SEP; }
  if (name == "hdr_properties") { return DRMProperty::HDR_PROPERTIES; }
  if (name == "SDE_DSPP_GAMUT_V3") { return DRMProperty::SDE_DSPP_GAMUT_V3; }
  if (name == "SDE_DSPP_GAMUT_V4") { return DRMProperty::SDE_DSPP_GAMUT_V4; }
  if (name == "SDE_DSPP_GAMUT_V5") { return DRMProperty::SDE_DSPP_GAMUT_V5; }
  if (name == "SDE_DSPP_GC_V1") { return DRMProperty::SDE_DSPP_GC_V1; }
  if (name == "SDE_DSPP_GC_V2") { return DRMProperty::SDE_DSPP_GC_V2; }
  if (name == "SDE_DSPP_IGC_V2") { return DRMProperty::SDE_DSPP_IGC_V2; }
  if (name == "SDE_DSPP_IGC_V3") { return DRMProperty::SDE_DSPP_IGC_V3; }
  if (name == "SDE_DSPP_IGC_V4") { return DRMProperty::SDE_DSPP_IGC_V4; }
  if (name == "SDE_DSPP_IGC_V5") { return DRMProperty::SDE_DSPP_IGC_V5; }
  if (name == "SDE_DSPP_PCC_V3") { return DRMProperty::SDE_DSPP_PCC_V3; }
  if (name == "SDE_DSPP_PCC_V4") { return DRMProperty::SDE_DSPP_PCC_V4; }
  if (name == "SDE_DSPP_PCC_V5") { return DRMProperty::SDE_DSPP_PCC_V5; }
  if (name == "SDE_DSPP_PCC_V6") {
    return DRMProperty::SDE_DSPP_PCC_V6;
  }
  if (name == "SDE_DSPP_PA_HSIC_V1") { return DRMProperty::SDE_DSPP_PA_HSIC_V1; }
  if (name == "SDE_DSPP_PA_HSIC_V2") { return DRMProperty::SDE_DSPP_PA_HSIC_V2; }
  if (name == "SDE_DSPP_PA_SIXZONE_V1") { return DRMProperty::SDE_DSPP_PA_SIXZONE_V1; }
  if (name == "SDE_DSPP_PA_SIXZONE_V2") { return DRMProperty::SDE_DSPP_PA_SIXZONE_V2; }
  if (name == "SDE_DSPP_PA_MEMCOL_SKIN_V1") { return DRMProperty::SDE_DSPP_PA_MEMCOL_SKIN_V1; }
  if (name == "SDE_DSPP_PA_MEMCOL_SKIN_V2") { return DRMProperty::SDE_DSPP_PA_MEMCOL_SKIN_V2; }
  if (name == "SDE_DSPP_PA_MEMCOL_SKY_V1") { return DRMProperty::SDE_DSPP_PA_MEMCOL_SKY_V1; }
  if (name == "SDE_DSPP_PA_MEMCOL_SKY_V2") { return DRMProperty::SDE_DSPP_PA_MEMCOL_SKY_V2; }
  if (name == "SDE_DSPP_PA_MEMCOL_FOLIAGE_V1") { return DRMProperty::SDE_DSPP_PA_MEMCOL_FOLIAGE_V1; }
  if (name == "SDE_DSPP_PA_MEMCOL_FOLIAGE_V2") { return DRMProperty::SDE_DSPP_PA_MEMCOL_FOLIAGE_V2; }
  if (name == "SDE_DSPP_PA_MEMCOL_PROT_V1") { return DRMProperty::SDE_DSPP_PA_MEMCOL_PROT_V1; }
  if (name == "SDE_DSPP_PA_MEMCOL_PROT_V2") { return DRMProperty::SDE_DSPP_PA_MEMCOL_PROT_V2; }
  if (name == "autorefresh") { return DRMProperty::AUTOREFRESH; }
  if (name == "ext_hdr_properties") { return DRMProperty::EXT_HDR_PROPERTIES; }
  if (name == "hdr_metadata") { return DRMProperty::HDR_METADATA; }
  if (name == "multirect_mode") { return DRMProperty::MULTIRECT_MODE; }
  if (name == "SDE_DSPP_PA_DITHER_V1") { return DRMProperty::SDE_DSPP_PA_DITHER_V1; }
  if (name == "SDE_DSPP_PA_DITHER_V2") { return DRMProperty::SDE_DSPP_PA_DITHER_V2; }
  if (name == "SDE_PP_DITHER_V1") { return DRMProperty::SDE_PP_DITHER_V1; }
  if (name == "SDE_PP_DITHER_V2") { return DRMProperty::SDE_PP_DITHER_V2; }
  if (name == "inverse_pma") { return DRMProperty::INVERSE_PMA; }
  if (name == "csc_dma_v1") { return DRMProperty::CSC_DMA_V1; }
  if (name == "SDE_DGM_1D_LUT_IGC_V5") { return DRMProperty::SDE_DGM_1D_LUT_IGC_V5; }
  if (name == "SDE_DGM_1D_LUT_GC_V5") { return DRMProperty::SDE_DGM_1D_LUT_GC_V5; }
  if (name == "SDE_VIG_1D_LUT_IGC_V5") { return DRMProperty::SDE_VIG_1D_LUT_IGC_V5; }
  if (name == "SDE_VIG_3D_LUT_GAMUT_V5") { return DRMProperty::SDE_VIG_3D_LUT_GAMUT_V5; }
  if (name == "SDE_DSPP_AD_V4_MODE") { return DRMProperty::SDE_DSPP_AD4_MODE; }
  if (name == "SDE_DSPP_AD_V4_INIT") { return DRMProperty::SDE_DSPP_AD4_INIT; }
  if (name == "SDE_DSPP_AD_V4_CFG") { return DRMProperty::SDE_DSPP_AD4_CFG; }
  if (name == "SDE_DSPP_AD_V4_ASSERTIVENESS") { return DRMProperty::SDE_DSPP_AD4_ASSERTIVENESS; }
  if (name == "SDE_DSPP_AD_V4_STRENGTH") { return DRMProperty::SDE_DSPP_AD4_STRENGTH; }
  if (name == "SDE_DSPP_AD_V4_INPUT") { return DRMProperty::SDE_DSPP_AD4_INPUT; }
  if (name == "SDE_DSPP_AD_V4_BACKLIGHT") { return DRMProperty::SDE_DSPP_AD4_BACKLIGHT; }
  if (name == "SDE_DSPP_AD_V4_ROI") { return DRMProperty::SDE_DSPP_AD4_ROI; }
  if (name == "SDE_DSPP_HIST_CTRL_V1") { return DRMProperty::SDE_DSPP_ABA_HIST_CTRL; }
  if (name == "SDE_DSPP_HIST_IRQ_V1") { return DRMProperty::SDE_DSPP_ABA_HIST_IRQ; }
  if (name == "SDE_DSPP_VLUT_V1") { return DRMProperty::SDE_DSPP_ABA_LUT; }
  if (name == "bl_scale") { return DRMProperty::SDE_DSPP_BL_SCALE; }
  if (name == "sv_bl_scale") { return DRMProperty::SDE_DSPP_SV_BL_SCALE; }
  if (name == "capture_mode") { return DRMProperty::CAPTURE_MODE; }
  if (name == "qsync_mode") { return DRMProperty::QSYNC_MODE; }
  if (name == "idle_pc_state") { return DRMProperty::IDLE_PC_STATE; }
  if (name == "topology_control") { return DRMProperty::TOPOLOGY_CONTROL; }
  if (name == "EDID") { return DRMProperty::EDID; }
  if (name == "SDE_DSPP_LTM_V1") { return DRMProperty::SDE_LTM_VERSION; }
  if (name == "SDE_DSPP_LTM_V2") { return DRMProperty::SDE_LTM_VERSION_V2; }
  if (name == "SDE_DSPP_LTM_INIT_V1") { return DRMProperty::SDE_LTM_INIT; }
  if (name == "SDE_DSPP_LTM_ROI_V1") { return DRMProperty::SDE_LTM_CFG; }
  if (name == "SDE_DSPP_LTM_HIST_THRESH_V1") { return DRMProperty::SDE_LTM_NOISE_THRESH; }
  if (name == "SDE_DSPP_LTM_HIST_CTRL_V1") { return DRMProperty::SDE_LTM_HIST_CTRL; }
  if (name == "SDE_DSPP_LTM_SET_BUF_V1") { return DRMProperty::SDE_LTM_BUFFER_CTRL; }
  if (name == "SDE_DSPP_LTM_QUEUE_BUF_V1") { return DRMProperty::SDE_LTM_QUEUE_BUFFER; }
  if (name == "SDE_DSPP_LTM_QUEUE_BUF2_V1") { return DRMProperty::SDE_LTM_QUEUE_BUFFER2; }
  if (name == "SDE_DSPP_LTM_QUEUE_BUF3_V1") { return DRMProperty::SDE_LTM_QUEUE_BUFFER3; }
  if (name == "SDE_DSPP_LTM_VLUT_V1") { return DRMProperty::SDE_LTM_VLUT; }
  if (name == "SDE_VIG_1D_LUT_IGC_V6") { return DRMProperty::SDE_VIG_1D_LUT_IGC_V6; }
  if (name == "SDE_VIG_3D_LUT_GAMUT_V6") { return DRMProperty::SDE_VIG_3D_LUT_GAMUT_V6; }
  if (name == "frame_trigger_mode") { return DRMProperty::FRAME_TRIGGER; }
  if (name == "Colorspace") { return DRMProperty::COLORSPACE; }
  if (name == "supported_colorspaces") { return DRMProperty::SUPPORTED_COLORSPACES; }
  if (name == "SDE_DSPP_SPR_DITHER_V1") { return DRMProperty::SDE_DSPP_SPR_DITHER_V1; }
  if (name == "cache_state") { return DRMProperty::CACHE_STATE; }
  if (name == "vm_request_state") { return DRMProperty::VM_REQ_STATE; }
  if (name == "dspp_caps") { return DRMProperty::DSPP_CAPABILITIES; }
  if (name == "SDE_SPR_INIT_CFG_V1") { return DRMProperty::SPR_INIT_CFG_V1; }
  if (name == "SDE_SPR_INIT_CFG_V2") {
    return DRMProperty::SPR_INIT_CFG_V2;
  }
  if (name == "SDE_SPR_UDC_CFG_V2") {
    return DRMProperty::SPR_UDC_CFG_V2;
  }
  if (name == "SDE_DSPP_RC_MASK_V1") { return DRMProperty::DSPP_RC_MASK_V1; }
  if (name == "panel_mode") { return DRMProperty::PANEL_MODE; }
  if (name == "bpp_mode") { return DRMProperty::BPP_MODE; }
  if (name == "SDE_DEMURA_INIT_CFG_V1") { return DRMProperty::DEMURA_INIT_CFG_V1; }
  if (name == "SDE_DEMURA_INIT_CFG_V3") { return DRMProperty::DEMURA_INIT_CFG_V3; }
  if (name == "SDE_DEMURA_CFG0_PARAM2") {
    return DRMProperty::DEMURA_CFG0_PARAM2;
  }
  if (name == "SDE_DEMURA_BACKLIGHT_V1") { return DRMProperty::DEMURA_BACKLIGHT_V1; }
  if (name == "DEMURA_PANEL_ID") { return DRMProperty::DEMURA_PANEL_ID; }
  if (name == "SDE_DEMURA_BOOT_PLANE_V1") { return DRMProperty::DEMURA_BOOT_PLANE_V1; }
  if (name == "SDE_DSPP_AIQE_ABC_V1") { return DRMProperty::AIQE_ABC_V1; }
  if (name == "dyn_bit_clk") { return DRMProperty::DYN_BIT_CLK; }
  if (name == "SDE_PP_CWB_DITHER_V2") { return DRMProperty::SDE_PP_CWB_DITHER_V2; }
  if (name == "noise_layer_v1") { return DRMProperty::NOISE_LAYER_V1; }
  if (name == "dsc_mode") { return DRMProperty::DSC_MODE; }
  if (name == "dimming_bl_lut") { return DRMProperty::DIMMING_BL_LUT; }
  if (name == "dimming_dyn_ctrl") { return DRMProperty::DIMMING_DYN_CTRL; }
  if (name == "dimming_min_bl") { return DRMProperty::DIMMING_MIN_BL; }
  if (name == "dyn_transfer_time") { return DRMProperty::DYN_TRANSFER_TIME; }
  if (name == "jitter_config") { return DRMProperty::JITTER_CONFIG; }
  if (name == "transfer_time") { return DRMProperty::TRANSFER_TIME; }
  if (name == "early_fence_line") { return DRMProperty::EARLY_FENCE_LINE; }
  if (name == "dnsc_blur") { return DRMProperty::DNSC_BLR; }
  if (name == "wb_usage_type") { return DRMProperty::WB_USAGE_TYPE; }
  if (name == "SDE_SSPP_FP16_IGC_V1") { return DRMProperty::SDE_SSPP_FP16_IGC_V1; }
  if (name == "SDE_SSPP_FP16_GC_V1") { return DRMProperty::SDE_SSPP_FP16_GC_V1; }
  if (name == "SDE_SSPP_FP16_CSC_V1") { return DRMProperty::SDE_SSPP_FP16_CSC_V1; }
  if (name == "SDE_SSPP_FP16_UNMULT_V1") { return DRMProperty::SDE_SSPP_FP16_UNMULT_V1; }
  if (name == "SDE_SSPP_UCSC_UNMULT_V1") { return DRMProperty::SDE_SSPP_UCSC_UNMULT_V1; }
  if (name == "SDE_SSPP_UCSC_IGC_V1") { return DRMProperty::SDE_SSPP_UCSC_IGC_V1; }
  if (name == "SDE_SSPP_UCSC_CSC_V1") { return DRMProperty::SDE_SSPP_UCSC_CSC_V1; }
  if (name == "SDE_SSPP_UCSC_GC_V1") { return DRMProperty::SDE_SSPP_UCSC_GC_V1; }
  if (name == "SDE_SSPP_UCSC_ALPHA_DITHER_V1") { return DRMProperty::SDE_SSPP_UCSC_ALPHA_DITHER_V1; }
  if (name == "EPT") { return DRMProperty::EPT; }
  if (name == "cac_type") { return DRMProperty::CAC_TYPE; }
  if (name == "src_rect_extn") { return DRMProperty::SRC_RECT_EXT; }
  if (name == "dst_rect_extn") { return DRMProperty::DST_RECT_EXT; }
  if (name == "bg_alpha") { return DRMProperty::BG_ALPHA; }
  if (name == "src_img_size") { return DRMProperty::IMG_SIZE_RECT; }
  if (name == "ubwc_clk") { return DRMProperty::UBWC_CLK; }
  if (name == "SDE_DSPP_AIQE_SSRC_CONFIG_V1") { return DRMProperty::SDE_DSPP_AIQE_SSRC_CONFIG_V1; }
  if (name == "SDE_DSPP_AIQE_SSRC_DATA_V1") { return DRMProperty::SDE_DSPP_AIQE_SSRC_DATA_V1; }
  if (name == "SDE_DSPP_AIQE_AI_SCALER_V1") { return DRMProperty::AI_SCALER_CFG_V1; }
  if (name == "SDE_DSPP_AIQE_MDNIE_V1") { return DRMProperty::SDE_DSPP_AIQE_MDNIE_V1; }
  if (name == "SDE_DSPP_AIQE_MDNIE_ART_V1") { return DRMProperty::SDE_DSPP_AIQE_MDNIE_ART_V1; }
  if (name == "SDE_DSPP_AIQE_MDNIE_IPC_V1") { return DRMProperty::SDE_DSPP_AIQE_MDNIE_IPC_V1; }
  if (name == "SDE_DSPP_AIQE_COPR_V1") { return DRMProperty::SDE_DSPP_AIQE_COPR_V1; }
  if (name == "avr_step_state") { return DRMProperty::AVR_STEP_STATE; }
  if (name == "frame_interval") { return DRMProperty::FRAME_INTERVAL; }
  if (name == "usecase_idx") { return DRMProperty::USECASE_IDX; }
  if (name == "brightness") { return DRMProperty::BRIGHTNESS; }
#ifdef SEC_GC_CMN_FINGERPRINT_INDISPLAY
  if (name == "fingerprint_mask") { return DRMProperty::FINGERPRINT_MASK; }
#endif

  return DRMProperty::INVALID;
}
// clang-format on

}  // namespace sde_drm
