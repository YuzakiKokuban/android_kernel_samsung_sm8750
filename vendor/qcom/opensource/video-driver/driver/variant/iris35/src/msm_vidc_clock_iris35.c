// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "perf_static_model.h"
#include "msm_vidc_debug.h"

#define ENABLE_FINEBITRATE_SUBUHD60 0

static u32 codec_encoder_gop_complexity_table_fp[9][3];
static u32 codec_mbspersession_iris35;
static u32 input_bitrate_fp;

/*
 * Chipset Generation Technology: SW/FW overhead profiling
 * need update with new numbers
 */
static u32 frequency_table_iris35[2][7] = {
	/* //make lowsvs_D1 as invalid; */
	{570, 570, 533, 444, 420, 338, 240},
	{855, 855, 800, 666, 630, 507, 360},
};

 /*
  * TODO Move to sun.c
  * TODO Replace hardcoded values with
  * ENCODER_VPP_TARGET_CLK_PER_MB_IRIS35 in CPP file.
  */

/* Tensilica cycles profiled by FW team in lanai device Feb 2022 */
#define DECODER_VPP_FW_OVERHEAD_IRIS35_AV1D                                            ((80000*3)/2)
#define DECODER_VPP_FW_OVERHEAD_IRIS35_NONAV1D                                         ((60000*3)/2)

 /* Tensilica cycles */
#define DECODER_VPP_FW_OVERHEAD_IRIS35                                                  (0)

/* Tensilica cycles; this is measured in Lahaina 1stage with FW profiling */
#define DECODER_VPPVSP1STAGE_FW_OVERHEAD_IRIS35                                         (93000)

#define DECODER_VSP_FW_OVERHEAD_IRIS35 \
	(DECODER_VPPVSP1STAGE_FW_OVERHEAD_IRIS35 - DECODER_VPP_FW_OVERHEAD_IRIS35)

/* Tensilica cycles; encoder has ARP register */
#define ENCODER_VPP_FW_OVERHEAD_IRIS35                                                  (39000*3/2)

#define ENCODER_VPPVSP1STAGE_FW_OVERHEAD_IRIS35 \
	(ENCODER_VPP_FW_OVERHEAD_IRIS35 + DECODER_VSP_FW_OVERHEAD_IRIS35)

#define DECODER_SW_OVERHEAD_IRIS35                                                      (489583)
#define ENCODER_SW_OVERHEAD_IRIS35                                                      (489583)

// in VPP HW cycles
#define ENCODER_WORKER_FW_OVERHEAD_IRIS35                                               (260000)
#define DECODER_WORKER_FW_OVERHEAD_AV1_IRIS35                                           (200000)
#define DECODER_WORKER_FW_OVERHEAD_NONAV1_IRIS35                                        (136000)

/* Video IP Core Technology: pipefloor and pipe penlaty */
// static u32 encoder_vpp_target_clk_per_mb_iris35[2] = {320, 675};
static u32 decoder_vpp_target_clk_per_mb_iris35 = 200;

/*
 * These pipe penalty numbers only applies to 4 pipe
 * For 2pipe and 1pipe, these numbers need recalibrate
 */
static u32 pipe_penalty_iris35[3][3] = {
	/* NON AV1 */
	{1059, 1059, 1059},
	/* AV1 RECOMMENDED TILE 1080P_V2XH1, UHD_V2X2, 8KUHD_V8X2 */
	{1472, 1217, 1094},
	/* AV1 YOUTUBE/NETFLIX TILE 1080P_V4XH2_V4X1, UHD_V8X4_V8X1, 8KUHD_V8X8_V8X1 */
	{1472, 1217, 1094},
};

//Only apply to decoder; no need for encoder as LCU is 32/16
static u32 p2vsp4_pipeutilization_ratio_iris35[4][4] = {
	{ 6, 11, 21, 30 },  //LCU128  8K, UHD, 1080p, 720p
	{ 1,  7, 14, 14 },  //LCU64  8K, UHD, 1080p, 720p
	{ 0,  2, 10, 10 },  //LCU32  8K, UHD, 1080p, 720p
	{ 0,  1,  7, 10 },  //LCU16  8K, UHD, 1080p, 720p
};

//Only apply to decoder; no need for encoder as LCU is 32/16
static u32 p1vsp4_pipeutilization_ratio_iris35[4][4] = {
	{ 0, 18, 42, 40 },  //LCU128  8K, UHD, 1080p, 720p
	{ 0,  9, 23, 21 },  //LCU64  8K, UHD, 1080p, 720p
	{ 0,  4, 15, 19 },  //LCU32  8K, UHD, 1080p, 720p
	{ 0,  4, 10, 19 },  //LCU16  8K, UHD, 1080p, 720p
};

/*
 * Video IP Core Technology: bitrate constraint
 * HW limit bitrate table (these values are measured end to end fw/sw impacts are also considered)
 * TODO Can we convert to Cycles/MB? This will remove DIVISION.
 */
static u32 bitrate_table_iris35_2stage_fp[5][10] = {
	/* h264 cavlc */
	{0, 220, 220, 220, 220, 220, 220, 220, 220, 220},
	/* h264 cabac */
	{0, 140, 150, 160, 175, 190, 190, 190, 190, 190},
	/* h265 */
	{90, 140, 160, 180, 190, 200, 200, 200, 200, 200},
	/* vp9 */
	{90, 90, 90, 90, 90, 90, 90, 90, 90, 90},
	/* av1 */
	{130, 130, 120, 120, 120, 120, 120, 120, 120, 120},
};

/* HW limit bitrate table (these values are measured end to end fw/sw impacts are also considered) */
static u32 bitrate_table_iris35_1stage_fp[5][10] = { /* 1-stage assume IPPP */
	/* h264 cavlc */
	{0, 220, 220, 220, 220, 220, 220, 220, 220, 220},
	/* h264 cabac */
	{0, 110, 150, 150, 150, 150, 150, 150, 150, 150},
	/* h265 */
	{0, 140, 150, 150, 150, 150, 150, 150, 150, 150},
	/* vp9 */
	{0, 70, 70, 70, 70, 70, 70, 70, 70, 70},
	/* av1 */
	{0, 100, 100, 100, 100, 100, 100, 100, 100, 100},
};

/* 8KUHD60; UHD240; 1080p960  with B */
//static u32 fp_pixel_count_bar0 = 3840 * 2160 * 240;
/* 8KUHD60; UHD240; 1080p960  without B */
static u32 fp_pixel_count_bar1 = 3840 * 2160 * 240;
/* 1080p720 */
static u32 fp_pixel_count_bar2 = 3840 * 2160 * 180;
/* UHD120 */
static u32 fp_pixel_count_bar3 = 3840 * 2160 * 120;
/* UHD90 */
static u32 fp_pixel_count_bar4 = 3840 * 2160 * 90;
/* UHD60 */
static u32 fp_pixel_count_bar5 = 3840 * 2160 * 60;
/* UHD30; FHD120; HD240 */
static u32 fp_pixel_count_bar6 = 3840 * 2160 * 30;
/* FHD60 */
static u32 fp_pixel_count_bar7 = 1920 * 1080 * 60;
/* FHD30 */
static u32 fp_pixel_count_bar8 = 1920 * 1080 * 30;
/* HD30 */
static u32 fp_pixel_count_bar9 = 1280 * 720 * 30;

static u32 calculate_number_mbs_iris35(u32 width, u32 height, u32 lcu_size)
{
	u32 mbs_width = (width % lcu_size) ?
		(width / lcu_size + 1) : (width / lcu_size);

	u32 mbs_height = (height % lcu_size) ?
		(height / lcu_size + 1) : (height / lcu_size);

	return mbs_width * mbs_height * (lcu_size / 16) * (lcu_size / 16);
}

static int initialize_encoder_complexity_table(void)
{
	/* Beging Calculate Encoder GOP Complexity Table and HW Floor numbers */
	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_LOSSLESS][CODEC_ENCODER_GOP_Bb_ENTRY] = 0;

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_LOSSLESS][CODEC_ENCODER_GOP_P_ENTRY] = 10000;

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_LOSSLESS][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_LOSSLESS][CODEC_ENCODER_GOP_Bb_ENTRY] * 150 +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_LOSSLESS][CODEC_ENCODER_GOP_P_ENTRY] * 100);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_LOSSLESS][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_LOSSLESS][CODEC_ENCODER_GOP_FACTORY_ENTRY] +
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_LOSSLESS][CODEC_ENCODER_GOP_Bb_ENTRY] +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_LOSSLESS][CODEC_ENCODER_GOP_P_ENTRY] - 1);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_LOSSLESS][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_LOSSLESS][CODEC_ENCODER_GOP_FACTORY_ENTRY] /
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_LOSSLESS][CODEC_ENCODER_GOP_Bb_ENTRY] +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_LOSSLESS][CODEC_ENCODER_GOP_P_ENTRY]);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IONLY][CODEC_ENCODER_GOP_Bb_ENTRY] = 0;

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IONLY][CODEC_ENCODER_GOP_P_ENTRY] = 10000;

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IONLY][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IONLY][CODEC_ENCODER_GOP_Bb_ENTRY] * 150 +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IONLY][CODEC_ENCODER_GOP_P_ENTRY] * 100);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IONLY][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IONLY][CODEC_ENCODER_GOP_FACTORY_ENTRY] +
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IONLY][CODEC_ENCODER_GOP_Bb_ENTRY] +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IONLY][CODEC_ENCODER_GOP_P_ENTRY] - 1);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IONLY][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IONLY][CODEC_ENCODER_GOP_FACTORY_ENTRY] /
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IONLY][CODEC_ENCODER_GOP_Bb_ENTRY] +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IONLY][CODEC_ENCODER_GOP_P_ENTRY]);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I3B4b1P][CODEC_ENCODER_GOP_Bb_ENTRY] = 70000;

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I3B4b1P][CODEC_ENCODER_GOP_P_ENTRY] = 10000;

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I3B4b1P][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I3B4b1P][CODEC_ENCODER_GOP_Bb_ENTRY] * 150 +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I3B4b1P][CODEC_ENCODER_GOP_P_ENTRY] * 100);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I3B4b1P][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I3B4b1P][CODEC_ENCODER_GOP_FACTORY_ENTRY] +
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I3B4b1P][CODEC_ENCODER_GOP_Bb_ENTRY] +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I3B4b1P][CODEC_ENCODER_GOP_P_ENTRY] - 1);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I3B4b1P][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I3B4b1P][CODEC_ENCODER_GOP_FACTORY_ENTRY] /
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I3B4b1P][CODEC_ENCODER_GOP_Bb_ENTRY] +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I3B4b1P][CODEC_ENCODER_GOP_P_ENTRY]);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I1B2b1P][CODEC_ENCODER_GOP_Bb_ENTRY] = 30000;

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I1B2b1P][CODEC_ENCODER_GOP_P_ENTRY] = 10000;

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I1B2b1P][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I1B2b1P][CODEC_ENCODER_GOP_Bb_ENTRY] * 150 +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I1B2b1P][CODEC_ENCODER_GOP_P_ENTRY] * 100);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I1B2b1P][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I1B2b1P][CODEC_ENCODER_GOP_FACTORY_ENTRY] +
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I1B2b1P][CODEC_ENCODER_GOP_Bb_ENTRY] +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I1B2b1P][CODEC_ENCODER_GOP_P_ENTRY] - 1);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I1B2b1P][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I1B2b1P][CODEC_ENCODER_GOP_FACTORY_ENTRY] /
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I1B2b1P][CODEC_ENCODER_GOP_Bb_ENTRY] +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_I1B2b1P][CODEC_ENCODER_GOP_P_ENTRY]);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IbP][CODEC_ENCODER_GOP_Bb_ENTRY] = 10000;

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IbP][CODEC_ENCODER_GOP_P_ENTRY] = 10000;

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IbP][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IbP][CODEC_ENCODER_GOP_Bb_ENTRY] * 150 +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IbP][CODEC_ENCODER_GOP_P_ENTRY] * 100);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IbP][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IbP][CODEC_ENCODER_GOP_FACTORY_ENTRY] +
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IbP][CODEC_ENCODER_GOP_Bb_ENTRY] +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IbP][CODEC_ENCODER_GOP_P_ENTRY] - 1);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IbP][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IbP][CODEC_ENCODER_GOP_FACTORY_ENTRY] /
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IbP][CODEC_ENCODER_GOP_Bb_ENTRY] +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IbP][CODEC_ENCODER_GOP_P_ENTRY]);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IPP][CODEC_ENCODER_GOP_Bb_ENTRY] = 0;

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IPP][CODEC_ENCODER_GOP_P_ENTRY] = 10000;

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IPP][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IPP][CODEC_ENCODER_GOP_Bb_ENTRY] * 150 +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IPP][CODEC_ENCODER_GOP_P_ENTRY] * 100);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IPP][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IPP][CODEC_ENCODER_GOP_FACTORY_ENTRY] +
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IPP][CODEC_ENCODER_GOP_Bb_ENTRY] +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IPP][CODEC_ENCODER_GOP_P_ENTRY] - 1);

	codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IPP][CODEC_ENCODER_GOP_FACTORY_ENTRY] =
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IPP][CODEC_ENCODER_GOP_FACTORY_ENTRY] /
		(codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IPP][CODEC_ENCODER_GOP_Bb_ENTRY] +
		codec_encoder_gop_complexity_table_fp
		[CODEC_GOP_IPP][CODEC_ENCODER_GOP_P_ENTRY]);

	return 0;
}

u32 get_bitrate_entry(u32 pixle_count)
{
	u32 bitrate_entry = 0;

	if (pixle_count >= fp_pixel_count_bar1)
		bitrate_entry = 1;
	else if (pixle_count >= fp_pixel_count_bar2)
		bitrate_entry = 2;
	else if (pixle_count >= fp_pixel_count_bar3)
		bitrate_entry = 3;
	else if (pixle_count >= fp_pixel_count_bar4)
		bitrate_entry = 4;
	else if (pixle_count >= fp_pixel_count_bar5)
		bitrate_entry = 5;
	else if (pixle_count >= fp_pixel_count_bar6)
		bitrate_entry = 6;
	else if (pixle_count >= fp_pixel_count_bar7)
		bitrate_entry = 7;
	else if (pixle_count >= fp_pixel_count_bar8)
		bitrate_entry = 8;
	else if (pixle_count >= fp_pixel_count_bar9)
		bitrate_entry = 9;
	else
		bitrate_entry = 9;

	return bitrate_entry;
}

static int calculate_vsp_min_freq(struct api_calculation_input codec_input,
		struct api_calculation_freq_output *codec_output)
{
	/*
	 * VSP calculation
	 * different methodology from Lahaina
	 */
	u32 vsp_hw_min_frequency = 0;
	/* UInt32 decoder_vsp_fw_overhead = 100 + 5; // amplified by 100x */
	u32 fw_sw_vsp_offset = 1000 + 55;      /* amplified by 1000x */

	/*
	 * Ignore fw_sw_vsp_offset, as this is baked into the reference bitrate tables.
	 *  As a consequence remove x1000 multipler as well.
	 */
	u32 codec = codec_input.codec;
	/* UInt32 *bitratetable; */
	u32 pixle_count = codec_input.frame_width *
		codec_input.frame_height * codec_input.frame_rate;

	u8 bitrate_entry = get_bitrate_entry(pixle_count); /* TODO EXTRACT */

	input_bitrate_fp = ((u32)(codec_input.bitrate_mbps * 100 + 99)) / 100;

	/*
	 * bitrate was profiled at 444MHz for legacy codec
	 * bitrate was profiled at 533MHz for av1
	 */
	vsp_hw_min_frequency = frequency_table_iris35[0][3] *
		input_bitrate_fp * 1000;

	if (codec_input.codec == CODEC_AV1 && bitrate_entry == 1)
		vsp_hw_min_frequency = frequency_table_iris35[0][2] *
			input_bitrate_fp * 1000;

	if (codec_input.vsp_vpp_mode == CODEC_VSPVPP_MODE_2S) {
		u32 corner_bitrate = bitrate_table_iris35_2stage_fp[codec][bitrate_entry];

		if (codec_input.codec == CODEC_HEVC) {
			if (codec_input.hierachical_layer == CODEC_GOP_LOSSLESS) {
				vsp_hw_min_frequency = frequency_table_iris35[0][2] *
					input_bitrate_fp * 1000;
				corner_bitrate = frequency_table_iris35[0][2];
			} else if (codec_input.hierachical_layer == CODEC_GOP_IONLY) {
				corner_bitrate = frequency_table_iris35[0][3];
			}
		}
		vsp_hw_min_frequency = vsp_hw_min_frequency +
			(corner_bitrate * fw_sw_vsp_offset - 1);
		vsp_hw_min_frequency = DIV_ROUND_UP(vsp_hw_min_frequency,
			corner_bitrate * fw_sw_vsp_offset);
	} else {
		vsp_hw_min_frequency = vsp_hw_min_frequency +
			(bitrate_table_iris35_1stage_fp[codec][bitrate_entry] *
			fw_sw_vsp_offset - 1);
		vsp_hw_min_frequency = DIV_ROUND_UP(vsp_hw_min_frequency,
			(bitrate_table_iris35_1stage_fp[codec][bitrate_entry]) *
				fw_sw_vsp_offset);
	}

	codec_output->vsp_min_freq = vsp_hw_min_frequency;
	return 0;
}

static u32 calculate_pipe_penalty(struct api_calculation_input codec_input)
{
	u32 pipe_penalty_codec = 0;
	u8 avid_commercial_content = 0;
	u32 pixel_count = 0;

	/* decoder */
	if (codec_input.decoder_or_encoder == CODEC_DECODER) {
		pipe_penalty_codec = pipe_penalty_iris35[0][0];
		avid_commercial_content = codec_input.av1d_commer_tile_enable;
		if (codec_input.codec == CODEC_AV1) {
			pixel_count = codec_input.frame_width * codec_input.frame_height;
			if (pixel_count <= 1920 * 1080)
				pipe_penalty_codec =
					pipe_penalty_iris35[avid_commercial_content + 1][0];
			else if (pixel_count < 3840 * 2160)
				pipe_penalty_codec =
					(pipe_penalty_iris35[avid_commercial_content + 1][0] +
					pipe_penalty_iris35[avid_commercial_content + 1][1]) / 2;
			else if ((pixel_count == 3840 * 2160) ||
				(pixel_count == 4096 * 2160) || (pixel_count == 4096 * 2304))
				pipe_penalty_codec = pipe_penalty_iris35[avid_commercial_content + 1][1];
			else if (pixel_count < 7680 * 4320)
				pipe_penalty_codec =
					(pipe_penalty_iris35[avid_commercial_content + 1][1] +
					pipe_penalty_iris35[avid_commercial_content + 1][2]) / 2;
			else
				pipe_penalty_codec =
					pipe_penalty_iris35[avid_commercial_content + 1][2];
		}
	} else {
		pipe_penalty_codec = 101;
	}

	return pipe_penalty_codec;
}

static int calculate_vpp_min_freq(struct api_calculation_input codec_input,
		struct api_calculation_freq_output *codec_output)
{
	u32 vpp_hw_min_frequency = 0;
	u32 fmin = 0;
	u32 tensilica_min_frequency = 0;
	u32 decoder_vsp_fw_overhead = 100 + 5; /* amplified by 100x */
	/* UInt32 fw_sw_vsp_offset = 1000 + 55;       amplified by 1000x */
	/* TODO from calculate_sw_vsp_min_freq */
	u32 vsp_hw_min_frequency = codec_output->vsp_min_freq;
	u32 pipe_penalty_codec = 0;
	u32 fmin_fwoverhead105 = 0;
	u32 fmin_measured_fwoverhead = 0;
	u32 lpmode_uhd_cycle_permb = 0;
	u32 hqmode1080p_cycle_permb = 0;
	u32 encoder_vpp_target_clk_per_mb = 0;
	u32 decoder_vpp_fw_overhead = DECODER_VPP_FW_OVERHEAD_IRIS35;
	u32 (*pipeutilization_ratio)[4];
	u32 pipe_efficiency_ratio = 0;
	u32 worker_fw_overhead = 0;
	u32 worker_Fmin = 0;

	codec_mbspersession_iris35 =
		calculate_number_mbs_iris35(codec_input.frame_width,
		codec_input.frame_height, codec_input.lcu_size) *
		codec_input.frame_rate;

	/* Section 2. 0  VPP/VSP calculation */
	if (codec_input.decoder_or_encoder == CODEC_DECODER) { /* decoder */
		vpp_hw_min_frequency = ((decoder_vpp_target_clk_per_mb_iris35) *
			(codec_mbspersession_iris35) + codec_input.pipe_num - 1) /
			(codec_input.pipe_num);

		vpp_hw_min_frequency = (vpp_hw_min_frequency + 99999) / 1000000;

		if (codec_input.pipe_num > 1) {
			pipe_penalty_codec = calculate_pipe_penalty(codec_input);
			vpp_hw_min_frequency = (vpp_hw_min_frequency *
				pipe_penalty_codec + 999) / 1000;
		}

		if (codec_input.codec == CODEC_AV1) {
			decoder_vpp_fw_overhead = DECODER_VPP_FW_OVERHEAD_IRIS35_AV1D;
			worker_fw_overhead = DECODER_WORKER_FW_OVERHEAD_AV1_IRIS35;
		} else {
			decoder_vpp_fw_overhead = DECODER_VPP_FW_OVERHEAD_IRIS35_NONAV1D;
			worker_fw_overhead = DECODER_WORKER_FW_OVERHEAD_NONAV1_IRIS35;
		}
		if (codec_input.pipe_num != 4) {
			u32 i = 0;

			if (codec_input.lcu_size == 128) {
				i = 0;
			} else if (codec_input.lcu_size == 64) {
				i = 1;
			} else if (codec_input.lcu_size == 32) {
				i = 2;
			} else if (codec_input.lcu_size == 16) {
				i = 3;
			} else {
				d_vpr_e("%s: invalid codec %u\n", __func__, codec_input.lcu_size);
				return -1;
			}
			if (codec_input.pipe_num == 2) {
				pipeutilization_ratio = p2vsp4_pipeutilization_ratio_iris35;
			} else if (codec_input.pipe_num == 1) {
				pipeutilization_ratio = p1vsp4_pipeutilization_ratio_iris35;
			}

			if (codec_input.frame_width * codec_input.frame_height >= 7680 * 4320) {
				pipe_efficiency_ratio = pipeutilization_ratio[i][0];
			} else if (codec_input.frame_width * codec_input.frame_height > 3840 * 2160) {
				pipe_efficiency_ratio =
					(pipeutilization_ratio[i][0] + pipeutilization_ratio[i][1]) / 2;
			} else if (codec_input.frame_width * codec_input.frame_height == 3840 * 2160) {
				pipe_efficiency_ratio = pipeutilization_ratio[i][1];
			} else if (codec_input.frame_width * codec_input.frame_height > 1920 * 1080) {
				pipe_efficiency_ratio =
					(pipeutilization_ratio[i][1] + pipeutilization_ratio[i][2]) / 2;
			} else if (codec_input.frame_width * codec_input.frame_height == 1920 * 1080) {
				pipe_efficiency_ratio = pipeutilization_ratio[i][2];
			} else if (codec_input.frame_width * codec_input.frame_height > 1280 * 720) {
				pipe_efficiency_ratio =
					(pipeutilization_ratio[i][2] + pipeutilization_ratio[i][3]) / 2;
			} else {
				pipe_efficiency_ratio = pipeutilization_ratio[i][3];
			}
		}
		if (codec_input.vsp_vpp_mode == CODEC_VSPVPP_MODE_2S) {
			/* FW overhead, convert FW cycles to impact to one pipe */
			decoder_vpp_fw_overhead =
				DIV_ROUND_UP((decoder_vpp_fw_overhead * 10 *
				codec_input.frame_rate), 15);

			decoder_vpp_fw_overhead =
				DIV_ROUND_UP((decoder_vpp_fw_overhead * 1000),
				(codec_mbspersession_iris35 *
				decoder_vpp_target_clk_per_mb_iris35 / codec_input.pipe_num));

			decoder_vpp_fw_overhead += 1000;
			decoder_vpp_fw_overhead = (decoder_vpp_fw_overhead < 1050) ?
				1050 : decoder_vpp_fw_overhead;

			/* VPP HW + FW */
			if (codec_input.linear_opb == 1 && codec_input.bitdepth == CODEC_BITDEPTH_10)
				/* multiply by 1.20 for 10b case */
				decoder_vpp_fw_overhead = 1200 + decoder_vpp_fw_overhead - 1000;

			vpp_hw_min_frequency = (vpp_hw_min_frequency *
				decoder_vpp_fw_overhead + 999) / 1000;
			if (codec_input.pipe_num != 4) {
				vpp_hw_min_frequency =
					vpp_hw_min_frequency * (100 - pipe_efficiency_ratio) / 100;
			}
			/* VSP HW+FW */
			vsp_hw_min_frequency =
				(vsp_hw_min_frequency * decoder_vsp_fw_overhead + 99) / 100;

			fmin = (vpp_hw_min_frequency > vsp_hw_min_frequency) ?
				vpp_hw_min_frequency : vsp_hw_min_frequency;
		} else {
			/* 1-stage need SW cycles + FW cycles + HW time */
			if (codec_input.linear_opb == 1 && codec_input.bitdepth == CODEC_BITDEPTH_10)
				/* multiply by 1.20 for 10b linear case */
				vpp_hw_min_frequency =
					(vpp_hw_min_frequency * 1200 + 999) / 1000;
			if (codec_input.pipe_num != 4) {
				vpp_hw_min_frequency =
					vpp_hw_min_frequency * (100 - pipe_efficiency_ratio) / 100;
			}
			/*
			 * HW time
			 * comment: 02/23/2021 SY: the bitrate is measured bitrate,
			 * the overlapping effect is already considered into bitrate.
			 * no need to add extra anymore
			 */
			fmin = (vpp_hw_min_frequency > vsp_hw_min_frequency) ?
				vpp_hw_min_frequency : vsp_hw_min_frequency;

			/* FW time */
			fmin_fwoverhead105 = (fmin * 105 + 99) / 100;
			fmin_measured_fwoverhead = fmin +
				(((DECODER_VPPVSP1STAGE_FW_OVERHEAD_IRIS35 *
				codec_input.frame_rate * 10 + 14) / 15 + 999) / 1000 + 999) /
				1000;

			fmin = (fmin_fwoverhead105 > fmin_measured_fwoverhead) ?
				fmin_fwoverhead105 : fmin_measured_fwoverhead;
		}

		//small resolution (<960*540) consideration
		worker_Fmin = worker_fw_overhead * codec_input.frame_rate / 1000 / 1000;
		if (fmin < worker_Fmin) {
			fmin = worker_Fmin;
		}

		tensilica_min_frequency = (DECODER_SW_OVERHEAD_IRIS35 * 10 + 14) / 15;
		tensilica_min_frequency = (tensilica_min_frequency + 999) / 1000;
		tensilica_min_frequency = tensilica_min_frequency * codec_input.frame_rate;
		tensilica_min_frequency = (tensilica_min_frequency + 999) / 1000;
		fmin = (tensilica_min_frequency > fmin) ? tensilica_min_frequency : fmin;
	} else { /* encoder */
		/* Decide LP/HQ */
		u8 hq_mode = 0;

		worker_fw_overhead = ENCODER_WORKER_FW_OVERHEAD_IRIS35;

		if (codec_input.frame_width * codec_input.frame_height <= 1920 * 1088) {
			if (codec_input.pipe_num == 4) {
				if (codec_input.frame_width * codec_input.frame_height *
					codec_input.frame_rate <= 1920 * 1088 * 60)
					hq_mode = 1;
			} else {
				if (codec_input.frame_width * codec_input.frame_height *
					codec_input.frame_rate <= 1920 * 1088 * 30)
					hq_mode = 1;
			}
		}

		codec_output->enc_hqmode = hq_mode;

		/* Section 1. 0 */
		/* TODO ONETIME call, should be in another place. */
		initialize_encoder_complexity_table();

		/* End Calculate Encoder GOP Complexity Table */

		/* VPP base cycle */
		lpmode_uhd_cycle_permb = (320 *
			codec_encoder_gop_complexity_table_fp
			[codec_input.hierachical_layer][CODEC_ENCODER_GOP_FACTORY_ENTRY]
			+ 99) / 100;

		if ((codec_input.frame_width == 1920) &&
			((codec_input.frame_height == 1080) ||
			(codec_input.frame_height == 1088)) &&
			(codec_input.frame_rate >= 480))
			lpmode_uhd_cycle_permb = (90 * 4 *
				codec_encoder_gop_complexity_table_fp
				[codec_input.hierachical_layer][CODEC_ENCODER_GOP_FACTORY_ENTRY]
				+ 99) / 100;

		if ((codec_input.frame_width == 1280) &&
			((codec_input.frame_height == 720) ||
			(codec_input.frame_height == 768)) &&
			(codec_input.frame_rate >= 960))
			lpmode_uhd_cycle_permb = (99 * 4 *
				codec_encoder_gop_complexity_table_fp
				[codec_input.hierachical_layer][CODEC_ENCODER_GOP_FACTORY_ENTRY]
				+ 99) / 100;

		hqmode1080p_cycle_permb = (164 + 8) * 4;

		encoder_vpp_target_clk_per_mb = (hq_mode) ?
			hqmode1080p_cycle_permb : lpmode_uhd_cycle_permb;

		vpp_hw_min_frequency = ((encoder_vpp_target_clk_per_mb) *
			(codec_mbspersession_iris35) + codec_input.pipe_num - 1) /
			(codec_input.pipe_num);

		vpp_hw_min_frequency = (vpp_hw_min_frequency + 99999) / 1000000;

		if (codec_input.pipe_num > 1) {
			u32 pipe_penalty_codec = 101;

			vpp_hw_min_frequency = (vpp_hw_min_frequency *
				pipe_penalty_codec + 99) / 100;
		}

		if (codec_input.vsp_vpp_mode == CODEC_VSPVPP_MODE_2S) {
			/* FW overhead, convert FW cycles to impact to one pipe */
			u64 encoder_vpp_fw_overhead = 0;

			encoder_vpp_fw_overhead =
				DIV_ROUND_UP((ENCODER_VPP_FW_OVERHEAD_IRIS35 * 10 *
				codec_input.frame_rate), 15);

			encoder_vpp_fw_overhead =
				DIV_ROUND_UP((encoder_vpp_fw_overhead * 1000),
				(codec_mbspersession_iris35 * encoder_vpp_target_clk_per_mb /
				codec_input.pipe_num));

			encoder_vpp_fw_overhead += 1000;

			encoder_vpp_fw_overhead = (encoder_vpp_fw_overhead < 1050) ?
				1050 : encoder_vpp_fw_overhead;

			/* VPP HW + FW */
			vpp_hw_min_frequency = (vpp_hw_min_frequency *
				encoder_vpp_fw_overhead + 999) / 1000;

			/* TODO : decoder_vsp_fw_overhead? */
			vsp_hw_min_frequency = (vsp_hw_min_frequency *
				decoder_vsp_fw_overhead + 99) / 100;

			fmin = (vpp_hw_min_frequency > vsp_hw_min_frequency) ?
				vpp_hw_min_frequency : vsp_hw_min_frequency;
		} else {
			/* HW time */
			fmin = (vpp_hw_min_frequency > vsp_hw_min_frequency) ?
				vpp_hw_min_frequency : vsp_hw_min_frequency;

			/* FW time */
			fmin_fwoverhead105 = (fmin * 105 + 99) / 100;
			fmin_measured_fwoverhead = fmin +
				(((DECODER_VPPVSP1STAGE_FW_OVERHEAD_IRIS35 *
				codec_input.frame_rate * 10 + 14) / 15 + 999) /
				1000 + 999) / 1000;

			fmin = (fmin_fwoverhead105 > fmin_measured_fwoverhead) ?
				fmin_fwoverhead105 : fmin_measured_fwoverhead;
			/* SW time */
		}

		//small resolution (<960*540) consideration
		worker_Fmin = worker_fw_overhead * codec_input.frame_rate / 1000 / 1000;
		if (fmin < worker_Fmin) {
			fmin = worker_Fmin;
		}

		tensilica_min_frequency = (ENCODER_SW_OVERHEAD_IRIS35 * 10 + 14) / 15;
		tensilica_min_frequency = (tensilica_min_frequency + 999) / 1000;

		tensilica_min_frequency = tensilica_min_frequency *
			codec_input.frame_rate;

		tensilica_min_frequency = (tensilica_min_frequency + 999) / 1000;

		fmin = (tensilica_min_frequency > fmin) ?
			tensilica_min_frequency : fmin;
	}

	codec_output->vpp_min_freq = vpp_hw_min_frequency;
	codec_output->vsp_min_freq = vsp_hw_min_frequency;
	codec_output->tensilica_min_freq = tensilica_min_frequency;
	codec_output->hw_min_freq = fmin;

	return 0;
}

int msm_vidc_calculate_frequency(struct api_calculation_input codec_input,
		struct api_calculation_freq_output *codec_output)
{
	int rc = 0;

	rc = calculate_vsp_min_freq(codec_input, codec_output);
	if (rc)
		return rc;

	rc = calculate_vpp_min_freq(codec_input, codec_output);
	if (rc)
		return rc;

	return rc;
}
