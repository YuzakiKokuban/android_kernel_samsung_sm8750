// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2015-2021, The Linux Foundation. All rights reserved.
 */

#define pr_fmt(fmt)	"[drm:%s:%d] " fmt, __func__, __LINE__
#include <linux/iopoll.h>

#include "sde_hwio.h"
#include "sde_hw_catalog.h"
#include "sde_hw_intf.h"
#include "sde_dbg.h"

#define INTF_TIMING_ENGINE_EN           0x000
#define INTF_CONFIG                     0x004
#define INTF_HSYNC_CTL                  0x008
#define INTF_VSYNC_PERIOD_F0            0x00C
#define INTF_VSYNC_PERIOD_F1            0x010
#define INTF_VSYNC_PULSE_WIDTH_F0       0x014
#define INTF_VSYNC_PULSE_WIDTH_F1       0x018
#define INTF_DISPLAY_V_START_F0         0x01C
#define INTF_DISPLAY_V_START_F1         0x020
#define INTF_DISPLAY_V_END_F0           0x024
#define INTF_DISPLAY_V_END_F1           0x028
#define INTF_ACTIVE_V_START_F0          0x02C
#define INTF_ACTIVE_V_START_F1          0x030
#define INTF_ACTIVE_V_END_F0            0x034
#define INTF_ACTIVE_V_END_F1            0x038
#define INTF_DISPLAY_HCTL               0x03C
#define INTF_ACTIVE_HCTL                0x040
#define INTF_BORDER_COLOR               0x044
#define INTF_UNDERFLOW_COLOR            0x048
#define INTF_HSYNC_SKEW                 0x04C
#define INTF_POLARITY_CTL               0x050
#define INTF_TEST_CTL                   0x054
#define INTF_TP_COLOR0                  0x058
#define INTF_TP_COLOR1                  0x05C
#define INTF_CONFIG2                    0x060
#define INTF_DISPLAY_DATA_HCTL          0x064
#define INTF_ACTIVE_DATA_HCTL           0x068
#define INTF_TIMING_ENGINE_ALIGN_CTRL   0x06C
#define INTF_FRAME_LINE_COUNT_EN        0x0A8
#define INTF_MDP_FRAME_COUNT            0x0A4
#define INTF_FRAME_COUNT                0x0AC
#define INTF_LINE_COUNT                 0x0B0
#define INTF_RSCC_PANIC_CTRL            0x0D0
#define INTF_RSCC_PANIC_LEVEL           0x0D4
#define INTF_RSCC_PANIC_EXT_VFP_START   0x0D8

#define INTF_DEFLICKER_CONFIG           0x0F0
#define INTF_DEFLICKER_STRNG_COEFF      0x0F4
#define INTF_DEFLICKER_WEAK_COEFF       0x0F8

#define INTF_REG_SPLIT_LINK             0x080
#define INTF_DSI_CMD_MODE_TRIGGER_EN    0x084
#define INTF_PANEL_FORMAT               0x090
#define INTF_TPG_ENABLE                 0x100
#define INTF_TPG_MAIN_CONTROL           0x104
#define INTF_TPG_VIDEO_CONFIG           0x108
#define INTF_TPG_COMPONENT_LIMITS       0x10C
#define INTF_TPG_RECTANGLE              0x110
#define INTF_TPG_INITIAL_VALUE          0x114
#define INTF_TPG_BLK_WHITE_PATTERN_FRAMES   0x118
#define INTF_TPG_RGB_MAPPING            0x11C
#define INTF_PROG_FETCH_START           0x170
#define INTF_PROG_ROT_START             0x174

#define INTF_MISR_CTRL                  0x180
#define INTF_MISR_SIGNATURE             0x184

#define INTF_PROG_FLUSH_SNAPSHOT        0x1B0
#define INTF_WD_TIMER_0_LTJ_CTL         0x200
#define INTF_WD_TIMER_0_LTJ_CTL1        0x204

#define INTF_DPU_SYNC_CTRL              0x190
#define INTF_DPU_SYNC_PROG_INTF_OFFSET_EN   0x194

#define INTF_VSYNC_TIMESTAMP_CTRL       0x210
#define INTF_VSYNC_TIMESTAMP0           0x214
#define INTF_VSYNC_TIMESTAMP1           0x218
#define INTF_MDP_VSYNC_TIMESTAMP0       0x21C
#define INTF_MDP_VSYNC_TIMESTAMP1       0x220
#define INTF_WD_TIMER_0_JITTER_CTL      0x224
#define INTF_WD_TIMER_0_LTJ_SLOPE       0x228
#define INTF_WD_TIMER_0_LTJ_MAX         0x22C
#define INTF_WD_TIMER_0_CTL             0x230
#define INTF_WD_TIMER_0_CTL2            0x234
#define INTF_WD_TIMER_0_LOAD_VALUE      0x238
#define INTF_WD_TIMER_0_LTJ_INT_STATUS  0x240
#define INTF_WD_TIMER_0_LTJ_FRAC_STATUS 0x244
#define INTF_MUX                        0x25C
#define INTF_UNDERRUN_COUNT             0x268
#define INTF_STATUS                     0x26C
#define INTF_AVR_CONTROL                0x270
#define INTF_AVR_MODE                   0x274
#define INTF_AVR_TRIGGER                0x278
#define INTF_AVR_VTOTAL                 0x27C
#define INTF_TEAR_MDP_VSYNC_SEL         0x280
#define INTF_TEAR_TEAR_CHECK_EN         0x284
#define INTF_TEAR_SYNC_CONFIG_VSYNC     0x288
#define INTF_TEAR_SYNC_CONFIG_HEIGHT    0x28C
#define INTF_TEAR_SYNC_WRCOUNT          0x290
#define INTF_TEAR_VSYNC_INIT_VAL        0x294
#define INTF_TEAR_INT_COUNT_VAL         0x298
#define INTF_TEAR_SYNC_THRESH           0x29C
#define INTF_TEAR_START_POS             0x2A0
#define INTF_TEAR_RD_PTR_IRQ            0x2A4
#define INTF_TEAR_WR_PTR_IRQ            0x2A8
#define INTF_TEAR_OUT_LINE_COUNT        0x2AC
#define INTF_TEAR_LINE_COUNT            0x2B0
#define INTF_TEAR_AUTOREFRESH_CONFIG    0x2B4
#define INTF_TEAR_TEAR_DETECT_CTRL      0x2B8
#define INTF_TEAR_PROG_FETCH_START      0x2C4
#define INTF_TEAR_DSI_DMA_SCHD_CTRL0    0x2C8
#define INTF_TEAR_DSI_DMA_SCHD_CTRL1    0x2CC
#define INTF_TEAR_INT_COUNT_VAL_EXT     0x2DC
#define INTF_TEAR_SYNC_THRESH_EXT       0x2E0
#define INTF_TEAR_SYNC_WRCOUNT_EXT      0x2E4
#define MDP_INTF_NUM_AVR_STEP		0x460
#define MDP_INTF_CURRENT_AVR_STEP	0x464
#define INTF_TEAR_PANIC_START           0x2E8
#define INTF_TEAR_PANIC_WINDOW          0x2EC
#define INTF_TEAR_WAKEUP_START          0x2F0
#define INTF_TEAR_WAKEUP_WINDOW         0x2F4
#define INTF_ESYNC_EN                   0x400
#define INTF_ESYNC_CTRL                 0x404
#define INTF_ESYNC_MDP_VSYNC_CTL        0x408
#define INTF_ESYNC_VSYNC_CTL            0x40C
#define INTF_ESYNC_HSYNC_CTL            0x410
#define INTF_ESYNC_SKEW_CTL             0x414
#define INTF_ESYNC_EMSYNC_CTL           0x418
#define INTF_ESYNC_PROG_INIT            0x41C
#define INTF_PROG_DR_START              0x420
#define INTF_BKUP_ESYNC_EN              0x470
#define INTF_BKUP_ESYNC_CTRL            0x474
#define INTF_BKUP_ESYNC_VSYNC_CTL      0x47C
#define INTF_BKUP_ESYNC_HSYNC_CTL      0x480
#define INTF_BKUP_ESYNC_EMSYNC_CTL     0x484
#define INTF_BKUP_ESYNC_SW_RESET        0x488
#define INTF_ESYNC_SW_RESET             0x48C
#define INTF_ESYNC_HYBRID_CTRL          0x490
#define INTF_ESYNC_CTRL2                0x494
#define INTF_ESYNC_SPARE                0x498
#define INTF_ESYNC_SPARE_STATUS         0x49C
#define INTF_ESYNC_TIMESTAMP_CTRL       0x4A0
#define INTF_ESYNC_TIMESTAMP0           0x4A4
#define INTF_ESYNC_TIMESTAMP1           0x4A8
#define INTF_ESYNC_VSYNC_COUNT          0x4AC
#define INTF_ESYNC_EMSYNC_COUNT         0x4B0

static struct sde_intf_cfg *_intf_offset(enum sde_intf intf,
		struct sde_mdss_cfg *m,
		void __iomem *addr,
		struct sde_hw_blk_reg_map *b)
{
	int i;

	for (i = 0; i < m->intf_count; i++) {
		if ((intf == m->intf[i].id) &&
		(m->intf[i].type != INTF_NONE)) {
			b->base_off = addr;
			b->blk_off = m->intf[i].base;
			b->length = m->intf[i].len;
			b->hw_rev = m->hw_rev;
			b->log_mask = SDE_DBG_MASK_INTF;
			return &m->intf[i];
		}
	}

	return ERR_PTR(-EINVAL);
}

static void sde_hw_intf_avr_trigger(struct sde_hw_intf *ctx)
{
	struct sde_hw_blk_reg_map *c;

	if (!ctx)
		return;

	c = &ctx->hw;
	SDE_REG_WRITE(c, INTF_AVR_TRIGGER, 0x1);
	SDE_DEBUG("AVR Triggered\n");
}

static int sde_hw_intf_avr_setup(struct sde_hw_intf *ctx,
	const struct intf_timing_params *params,
	const struct intf_avr_params *avr_params)
{
	struct sde_hw_blk_reg_map *c;
	u32 hsync_period, vsync_period;
	u32 min_fps, default_fps, diff_fps;
	u32 vsync_period_slow;
	u32 avr_vtotal;
	u32 add_porches = 0;

	if (!ctx || !params || !avr_params) {
		SDE_ERROR("invalid input parameter(s)\n");
		return -EINVAL;
	}

	c = &ctx->hw;
	min_fps = avr_params->min_fps;
	default_fps = avr_params->default_fps;
	diff_fps = default_fps - min_fps;
	hsync_period = params->hsync_pulse_width +
			params->h_back_porch + params->width +
			params->h_front_porch;
	vsync_period = params->vsync_pulse_width +
			params->v_back_porch + params->height +
			params->v_front_porch;

	if (diff_fps)
		add_porches = mult_frac(vsync_period, diff_fps, min_fps);

	vsync_period_slow = vsync_period + add_porches;
	avr_vtotal = vsync_period_slow * hsync_period;

	SDE_REG_WRITE(c, INTF_AVR_VTOTAL, avr_vtotal);

	return 0;
}

static void sde_hw_intf_avr_enable(struct sde_hw_intf *ctx, bool enable)
{
	struct sde_hw_blk_reg_map *c;
	u32 avr_ctrl = 0;

	if (!ctx)
		return;

	c = &ctx->hw;
	if (enable)
		avr_ctrl = BIT(0);

	SDE_REG_WRITE(c, INTF_AVR_CONTROL, avr_ctrl);
}

static void sde_hw_intf_avr_ctrl(struct sde_hw_intf *ctx,
	const struct intf_avr_params *avr_params)
{
	struct sde_hw_blk_reg_map *c;
	u32 avr_mode = 0;
	u32 avr_ctrl = 0;

	if (!ctx || !avr_params)
		return;

	c = &ctx->hw;
	if (avr_params->avr_mode) {
		avr_ctrl = BIT(0);
		avr_mode = (avr_params->avr_mode == SDE_RM_QSYNC_ONE_SHOT_MODE) ?
				(BIT(0) | BIT(8)) : 0x0;
		if (avr_params->avr_step_lines)
			avr_mode |= avr_params->avr_step_lines << 16;
	}

	if (avr_params->infinite_mode)
		avr_mode = avr_mode | BIT(9);

	if (avr_params->hw_avr_trigger)
		avr_mode = avr_mode | BIT(10);

	SDE_REG_WRITE(c, INTF_AVR_CONTROL, avr_ctrl);
	SDE_REG_WRITE(c, INTF_AVR_MODE, avr_mode);
}

static void sde_hw_intf_raw_te_setup(struct sde_hw_intf *ctx, bool enabled)
{
	struct sde_hw_blk_reg_map *c;
	u32 raw_te = 0;

	if (!ctx)
		return;

	c = &ctx->hw;

	if (enabled)
		raw_te |= BIT(1);
	else
		raw_te &= ~BIT(1);

	SDE_REG_WRITE(c, INTF_RSCC_PANIC_CTRL, raw_te);
}

static u32 sde_hw_intf_get_avr_status(struct sde_hw_intf *ctx)
{
	struct sde_hw_blk_reg_map *c;
	u32 avr_ctrl;

	if (!ctx)
		return false;

	c = &ctx->hw;
	avr_ctrl = SDE_REG_READ(c, INTF_AVR_CONTROL);

	return avr_ctrl >> 31;
}

static void sde_hw_intf_set_num_avr_step(struct sde_hw_intf *ctx,
	u32 num_avr_step)
{
	struct sde_hw_blk_reg_map *c;

	if (!ctx)
		return;

	c = &ctx->hw;

	SDE_REG_WRITE(c, MDP_INTF_NUM_AVR_STEP, num_avr_step & 0xFFF);
}

static u32 sde_hw_intf_get_cur_num_avr_step(struct sde_hw_intf *ctx)
{
	struct sde_hw_blk_reg_map *c;

	if (!ctx)
		return 0;

	c = &ctx->hw;

	return SDE_REG_READ(c, MDP_INTF_CURRENT_AVR_STEP);
}

static void _sde_hw_intf_wait_for_esync_disable(struct sde_hw_intf *ctx, bool backup)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	u32 status_bit = backup ? BIT(4) : BIT(3);
	void __iomem *addr = c->base_off + c->blk_off + INTF_STATUS;
	u32 val;
	int rc;

	rc = readl_relaxed_poll_timeout(addr, val, !(val & status_bit), 100, 10000);
	if (rc)
		SDE_EVT32(backup, SDE_EVTLOG_ERROR);
}

static void sde_hw_intf_prepare_esync(struct sde_hw_intf *ctx, struct intf_esync_params *params)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	u32 val;
	u32 mask = BIT(16)-1;

	val = (params->avr_step_lines & mask) << 16 | (0x8);
	SDE_REG_WRITE(c, INTF_ESYNC_VSYNC_CTL, val);

	val = ((params->emsync_period_lines & mask) << 16) | (params->emsync_pulse_width & mask);
	SDE_REG_WRITE(c, INTF_ESYNC_EMSYNC_CTL, val);

	val = ((params->hsync_period_cycles & mask) << 16) | (params->hsync_pulse_width & mask);
	SDE_REG_WRITE(c, INTF_ESYNC_HSYNC_CTL, val);

	val = params->prog_fetch_start & mask;
	SDE_REG_WRITE(c, INTF_ESYNC_MDP_VSYNC_CTL, val);

	val = 0x0;
	SDE_REG_WRITE(c, INTF_ESYNC_PROG_INIT, val);

	val = params->skew & mask;
	SDE_REG_WRITE(c, INTF_ESYNC_SKEW_CTL, val);

	val = 0x1 << 4; /* COND0 enable, mode pclk */
	if (params->align_backup)
		val |= 0x7 << 8; /* COND1 backup esync rising edge */
	SDE_REG_WRITE(c, INTF_ESYNC_CTRL, val);

	val = 0x0;
	SDE_REG_WRITE(c, INTF_ESYNC_HYBRID_CTRL, val);
}

static void sde_hw_intf_enable_esync(struct sde_hw_intf *ctx, bool enable)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	u32 val = enable ? 0x1 : 0x0;

	SDE_REG_WRITE(c, INTF_ESYNC_EN, val);

	if (enable) {
		/* enable EM pulse timestamps */
		SDE_REG_WRITE(c, INTF_ESYNC_TIMESTAMP_CTRL, BIT(0) | BIT(2));
	} else {
		SDE_REG_WRITE(c, INTF_ESYNC_SW_RESET, 1);
		_sde_hw_intf_wait_for_esync_disable(ctx, false);
		SDE_REG_WRITE(c, INTF_ESYNC_SW_RESET, 0);
	}
}

static void sde_hw_intf_prepare_backup_esync(struct sde_hw_intf *ctx,
		struct intf_esync_params *params)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	u32 val;
	u32 mask = BIT(16)-1;

	val = (params->avr_step_lines & mask) << 16 | (0x8);
	SDE_REG_WRITE(c, INTF_BKUP_ESYNC_VSYNC_CTL, val);

	val = (params->emsync_pulse_width & mask) | ((params->emsync_period_lines & mask) << 16);
	SDE_REG_WRITE(c, INTF_BKUP_ESYNC_EMSYNC_CTL, val);

	val = (params->hsync_pulse_width & mask) | ((params->hsync_period_cycles & mask) << 16);
	SDE_REG_WRITE(c, INTF_BKUP_ESYNC_HSYNC_CTL, val);

	val = 0x1 << 4; /* COND0 enable */
	if (params->align_backup)
		val |= 0x7 << 8; /* COND1 main esync rising edge */
	SDE_REG_WRITE(c, INTF_BKUP_ESYNC_CTRL, val);

	val = 0x1;
	SDE_REG_WRITE(c, INTF_ESYNC_HYBRID_CTRL, val);
}

static void sde_hw_intf_enable_backup_esync(struct sde_hw_intf *ctx, bool enable)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	u32 val = enable ? 0x1 : 0x0;

	SDE_REG_WRITE(c, INTF_BKUP_ESYNC_EN, val);

	if (!enable) {
		SDE_REG_WRITE(c, INTF_BKUP_ESYNC_SW_RESET, 1);
		_sde_hw_intf_wait_for_esync_disable(ctx, true);
		SDE_REG_WRITE(c, INTF_BKUP_ESYNC_SW_RESET, 0);
	}
}

static int sde_hw_intf_wait_for_esync_src_switch(struct sde_hw_intf *ctx, bool backup)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	u32 val;
	u32 target = !!backup;

	return readx_poll_timeout(readl_relaxed,
			c->base_off + c->blk_off + INTF_ESYNC_HYBRID_CTRL,
			val, val == target, 100, 5000);
}

static u64 sde_hw_intf_get_esync_timestamp(struct sde_hw_intf *ctx)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	u32 timestamp_lo, timestamp_hi;
	u64 timestamp_total;

	timestamp_lo = SDE_REG_READ(c, INTF_ESYNC_TIMESTAMP0);
	timestamp_hi = SDE_REG_READ(c, INTF_ESYNC_TIMESTAMP1);

	timestamp_total = timestamp_hi;
	timestamp_total = (timestamp_total << 32) | timestamp_lo;
	return timestamp_total;
}

static void sde_hw_intf_enable_infinite_vfp(struct sde_hw_intf *ctx, bool enable)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	u32 val = enable ? BIT(9) : 0x0;

	val = SDE_REG_READ(c, INTF_AVR_MODE);

	if (enable)
		val |= BIT(9);

	SDE_REG_WRITE(c, INTF_AVR_MODE, val);
}

static inline void _check_and_set_comp_bit(struct sde_hw_intf *ctx,
		bool dsc_4hs_merge, bool compression_en, u32 *intf_cfg2)
{
	if (((SDE_HW_MAJOR(ctx->mdss->hw_rev) >= SDE_HW_MAJOR(SDE_HW_VER_700)) && compression_en)
	    || (IS_SDE_MAJOR_SAME(ctx->mdss->hw_rev, SDE_HW_VER_600) && dsc_4hs_merge))
		(*intf_cfg2) |= BIT(12);
	else if (!compression_en)
		(*intf_cfg2) &= ~BIT(12);
}

static void sde_hw_intf_reset_counter(struct sde_hw_intf *ctx)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;

	SDE_REG_WRITE(c, INTF_LINE_COUNT, BIT(31));
}

static u64 sde_hw_intf_get_vsync_timestamp(struct sde_hw_intf *ctx, bool is_vid)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	u32 timestamp_lo, timestamp_hi;
	u64 timestamp = 0;
	u32 reg_ts_0, reg_ts_1;

	if (ctx->cap->features & BIT(SDE_INTF_MDP_VSYNC_TS) && is_vid) {
		reg_ts_0 = INTF_MDP_VSYNC_TIMESTAMP0;
		reg_ts_1 = INTF_MDP_VSYNC_TIMESTAMP1;
	} else {
		reg_ts_0 = INTF_VSYNC_TIMESTAMP0;
		reg_ts_1 = INTF_VSYNC_TIMESTAMP1;
	}

	timestamp_hi = SDE_REG_READ(c, reg_ts_1);
	timestamp_lo = SDE_REG_READ(c, reg_ts_0);
	timestamp = timestamp_hi;
	timestamp = (timestamp << 32) | timestamp_lo;

	return timestamp;
}

static void sde_hw_intf_setup_timing_engine(struct sde_hw_intf *ctx,
		const struct intf_timing_params *p,
		const struct sde_format *fmt, bool align_esync, bool align_avr)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	u32 hsync_period, vsync_period;
	u32 display_v_start, display_v_end;
	u32 hsync_start_x, hsync_end_x;
	u32 hsync_data_start_x, hsync_data_end_x;
	u32 active_h_start, active_h_end;
	u32 active_v_start, active_v_end;
	u32 active_hctl, display_hctl, hsync_ctl;
	u32 polarity_ctl, den_polarity, hsync_polarity, vsync_polarity;
	u32 panel_format;
	u32 intf_cfg, intf_cfg2 = 0;
	u32 display_data_hctl = 0, active_data_hctl = 0;
	u32 data_width;
	bool dp_intf = false;
	u32 alignment = 0;

	/* read interface_cfg */
	intf_cfg = SDE_REG_READ(c, INTF_CONFIG);

	if (ctx->cap->type == INTF_EDP || ctx->cap->type == INTF_DP)
		dp_intf = true;

	hsync_period = p->hsync_pulse_width + p->h_back_porch + p->width +
			p->h_front_porch;
	vsync_period = p->vsync_pulse_width + p->v_back_porch + p->height +
			p->v_front_porch;

	display_v_start = ((p->vsync_pulse_width + p->v_back_porch) *
			hsync_period) + p->hsync_skew;
	display_v_end = ((vsync_period - p->v_front_porch) * hsync_period) +
			p->hsync_skew - 1;

	hsync_ctl = (hsync_period << 16) | p->hsync_pulse_width;

	hsync_start_x = p->h_back_porch + p->hsync_pulse_width;
	hsync_end_x = hsync_period - p->h_front_porch - 1;

	/*
	 * DATA_HCTL_EN controls data timing which can be different from
	 * video timing. It is recommended to enable it for all cases, except
	 * if compression is enabled in 1 pixel per clock mode
	 */
	if (!p->compression_en || p->wide_bus_en)
		intf_cfg2 |= BIT(4);

	if (p->wide_bus_en)
		intf_cfg2 |= BIT(0);

	/*
	 * If widebus is disabled:
	 * For uncompressed stream, the data is valid for the entire active
	 * window period.
	 * For compressed stream, data is valid for a shorter time period
	 * inside the active window depending on the compression ratio.
	 *
	 * If widebus is enabled:
	 * For uncompressed stream, data is valid for only half the active
	 * window, since the data rate is doubled in this mode.
	 * p->width holds the adjusted width for DP but unadjusted width for DSI
	 * For compressed stream, data validity window needs to be adjusted for
	 * compression ratio and then further halved.
	 */
	data_width = p->width;

	if (p->compression_en) {
		if (p->wide_bus_en)
			data_width = DIV_ROUND_UP(p->dce_bytes_per_line, 6);
		else
			data_width = DIV_ROUND_UP(p->dce_bytes_per_line, 3);

	} else if (!dp_intf && p->wide_bus_en) {
		data_width = p->width >> 1;
	} else {
		data_width = p->width;
	}

	hsync_data_start_x = hsync_start_x;
	hsync_data_end_x =  hsync_start_x + data_width - 1;

	display_hctl = (hsync_end_x << 16) | hsync_start_x;
	display_data_hctl = (hsync_data_end_x << 16) | hsync_data_start_x;

	if (dp_intf) {
		// DP timing adjustment
		display_v_start += p->hsync_pulse_width + p->h_back_porch;
		display_v_end   -= p->h_front_porch;
	}

	intf_cfg |= BIT(29);	/* ACTIVE_H_ENABLE */
	intf_cfg |= BIT(30);	/* ACTIVE_V_ENABLE */
	active_h_start = hsync_start_x;
	active_h_end = active_h_start + p->xres - 1;
	active_v_start = display_v_start;
	active_v_end = active_v_start + (p->yres * hsync_period) - 1;

	active_hctl = (active_h_end << 16) | active_h_start;

	if (dp_intf) {
		display_hctl = active_hctl;

		if (p->compression_en) {
			active_data_hctl = (hsync_start_x +
					 p->extra_dto_cycles) << 16;
			active_data_hctl += hsync_start_x;

			display_data_hctl = active_data_hctl;
		}
	}

	_check_and_set_comp_bit(ctx, p->dsc_4hs_merge, p->compression_en,
			&intf_cfg2);

	den_polarity = 0;
	if (ctx->cap->type == INTF_HDMI) {
		hsync_polarity = p->yres >= 720 ? 0 : 1;
		vsync_polarity = p->yres >= 720 ? 0 : 1;
	} else if (ctx->cap->type == INTF_DP) {
		hsync_polarity = p->hsync_polarity;
		vsync_polarity = p->vsync_polarity;
	} else {
		hsync_polarity = 0;
		vsync_polarity = 0;
	}
	polarity_ctl = (den_polarity << 2) | /*  DEN Polarity  */
		(vsync_polarity << 1) | /* VSYNC Polarity */
		(hsync_polarity << 0);  /* HSYNC Polarity */

	if (!SDE_FORMAT_IS_YUV(fmt))
		panel_format = (fmt->bits[C0_G_Y] |
				(fmt->bits[C1_B_Cb] << 2) |
				(fmt->bits[C2_R_Cr] << 4) |
				(0x21 << 8));
	else
		/* Interface treats all the pixel data in RGB888 format */
		panel_format = (COLOR_8BIT |
				(COLOR_8BIT << 2) |
				(COLOR_8BIT << 4) |
				(0x21 << 8));

	if (p->wide_bus_en)
		intf_cfg2 |= BIT(0);

	/* Synchronize timing engine enable to TE */
	if ((ctx->cap->features & BIT(SDE_INTF_TE_ALIGN_VSYNC))
			&& p->poms_align_vsync)
		intf_cfg2 |= BIT(16);

	if (align_esync) {
		/*
		 * Display on-
		 * COND0 1 = TIMING_ENGINE_EN.EN changes from 0 to 1
		 * COND1 TE level being high COND2 esync_mdp_vsync
		 */
		alignment = 0x451;

		/* Idle exit -COND0 HW AVR trigger COND1 esync_mdp_vsync*/
		if (align_avr)
			alignment = 0x46;

		intf_cfg2 |= BIT(23);
	}

	SDE_EVT32(align_avr, align_esync, alignment);
	if (!dp_intf && ctx->cap->features & BIT(SDE_INTF_PERIPHERAL_FLUSH))
		intf_cfg2 |= BIT(24);

	if (ctx->cap->features & BIT(SDE_INTF_PROG_DYNREF))
		intf_cfg2 |= BIT(28);

	if (ctx->cfg.split_link_en)
		SDE_REG_WRITE(c, INTF_REG_SPLIT_LINK, 0x3);

	SDE_REG_WRITE(c, INTF_HSYNC_CTL, hsync_ctl);
	SDE_REG_WRITE(c, INTF_VSYNC_PERIOD_F0, vsync_period * hsync_period);
	SDE_REG_WRITE(c, INTF_VSYNC_PULSE_WIDTH_F0,
			p->vsync_pulse_width * hsync_period);
	SDE_REG_WRITE(c, INTF_DISPLAY_HCTL, display_hctl);
	SDE_REG_WRITE(c, INTF_DISPLAY_V_START_F0, display_v_start);
	SDE_REG_WRITE(c, INTF_DISPLAY_V_END_F0, display_v_end);
	SDE_REG_WRITE(c, INTF_ACTIVE_HCTL,  active_hctl);
	SDE_REG_WRITE(c, INTF_ACTIVE_V_START_F0, active_v_start);
	SDE_REG_WRITE(c, INTF_ACTIVE_V_END_F0, active_v_end);
	SDE_REG_WRITE(c, INTF_BORDER_COLOR, p->border_clr);
	SDE_REG_WRITE(c, INTF_UNDERFLOW_COLOR, p->underflow_clr);
	SDE_REG_WRITE(c, INTF_HSYNC_SKEW, p->hsync_skew);
	SDE_REG_WRITE(c, INTF_POLARITY_CTL, polarity_ctl);
	SDE_REG_WRITE(c, INTF_FRAME_LINE_COUNT_EN, 0x3);
	SDE_REG_WRITE(c, INTF_CONFIG, intf_cfg);
	SDE_REG_WRITE(c, INTF_PANEL_FORMAT, panel_format);
	SDE_REG_WRITE(c, INTF_DISPLAY_DATA_HCTL, display_data_hctl);
	SDE_REG_WRITE(c, INTF_ACTIVE_DATA_HCTL, active_data_hctl);
	if (align_esync)
		SDE_REG_WRITE(c, INTF_TIMING_ENGINE_ALIGN_CTRL, alignment);
	SDE_REG_WRITE(c, INTF_CONFIG2, intf_cfg2);
}

static void sde_hw_intf_enable_timing_engine(struct sde_hw_intf *intf, u8 enable)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;
	u32 val;

	/* Note: Display interface select is handled in top block hw layer */
	SDE_REG_WRITE(c, INTF_TIMING_ENGINE_EN, enable != 0);

	if (enable && (intf->cap->features
			& (BIT(SDE_INTF_PANEL_VSYNC_TS) | BIT(SDE_INTF_MDP_VSYNC_TS)))) {
		val = BIT(0);
		if (intf->cap->features & SDE_INTF_VSYNC_TS_SRC_EN)
			val |= BIT(4);

		SDE_REG_WRITE(c, INTF_VSYNC_TIMESTAMP_CTRL, val);
	}
}

static void sde_hw_intf_enable_te_level_trigger(struct sde_hw_intf *intf, bool enable)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;
	u32 intf_cfg = 0;

	intf_cfg = SDE_REG_READ(c, INTF_CONFIG);

	if (enable)
		intf_cfg |= BIT(22);
	else
		intf_cfg &= ~BIT(22);

	SDE_REG_WRITE(c, INTF_CONFIG, intf_cfg);
}

static void sde_hw_intf_setup_prg_fetch(
		struct sde_hw_intf *intf,
		const struct intf_prog_fetch *fetch)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;
	int fetch_enable;

	/*
	 * Fetch should always be outside the active lines. If the fetching
	 * is programmed within active region, hardware behavior is unknown.
	 */

	fetch_enable = SDE_REG_READ(c, INTF_CONFIG);
	if (fetch->enable) {
		fetch_enable |= BIT(31);
		SDE_REG_WRITE(c, INTF_PROG_FETCH_START,
				fetch->fetch_start);
	} else {
		fetch_enable &= ~BIT(31);
	}

	SDE_REG_WRITE(c, INTF_CONFIG, fetch_enable);
}

static void sde_hw_intf_setup_prog_dynref(struct sde_hw_intf *intf, u32 prog_dr_start_line)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;

	SDE_REG_WRITE(c, INTF_PROG_DR_START, prog_dr_start_line);
}

static void sde_hw_intf_configure_wd_timer_jitter(struct sde_hw_intf *intf,
		struct intf_wd_jitter_params *wd_jitter)
{
	struct sde_hw_blk_reg_map *c;
	u32 reg, jitter_ctl = 0;

	c = &intf->hw;

	/*
	 * Load Jitter values with jitter feature disabled.
	 */
	SDE_REG_WRITE(c, INTF_WD_TIMER_0_JITTER_CTL, 0x1);

	if (wd_jitter->jitter)
		jitter_ctl |= ((wd_jitter->jitter & 0x3FF) << 16);

	if (wd_jitter->ltj_max) {
		SDE_REG_WRITE(c, INTF_WD_TIMER_0_LTJ_MAX, wd_jitter->ltj_max);
		SDE_REG_WRITE(c, INTF_WD_TIMER_0_LTJ_SLOPE, wd_jitter->ltj_slope);
	}

	reg = SDE_REG_READ(c, INTF_WD_TIMER_0_JITTER_CTL);
	reg |= jitter_ctl;
	SDE_REG_WRITE(c, INTF_WD_TIMER_0_JITTER_CTL, reg);

	if (wd_jitter->jitter)
		reg |= BIT(31);
	if (wd_jitter->ltj_max)
		reg |= BIT(30);
	SDE_REG_WRITE(c, INTF_WD_TIMER_0_JITTER_CTL, reg);

	if (intf->cap->features & BIT(SDE_INTF_WD_LTJ_CTL)) {
		if (wd_jitter->ltj_step_dir && wd_jitter->ltj_initial_val) {
			reg = ((wd_jitter->ltj_step_dir & 0x1) << 31) |
					(wd_jitter->ltj_initial_val  & 0x1FFFFF);
			SDE_REG_WRITE(c, INTF_WD_TIMER_0_LTJ_CTL, reg);
			wd_jitter->ltj_step_dir = 0;
			wd_jitter->ltj_initial_val = 0;
		}

		if (wd_jitter->ltj_fractional_val) {
			SDE_REG_WRITE(c, INTF_WD_TIMER_0_LTJ_CTL1, wd_jitter->ltj_fractional_val);
			wd_jitter->ltj_fractional_val = 0;
		}
	}

}

static void sde_hw_intf_read_wd_ltj_ctl(struct sde_hw_intf *intf,
		struct intf_wd_jitter_params *wd_jitter)
{
	struct sde_hw_blk_reg_map *c;
	u32 reg;

	c = &intf->hw;

	if (intf->cap->features & BIT(SDE_INTF_WD_LTJ_CTL)) {
		reg = SDE_REG_READ(c, INTF_WD_TIMER_0_LTJ_INT_STATUS);
		wd_jitter->ltj_step_dir =  reg & BIT(31);
		wd_jitter->ltj_initial_val = (reg & 0x1FFFFF);

		reg = SDE_REG_READ(c, INTF_WD_TIMER_0_LTJ_FRAC_STATUS);
		wd_jitter->ltj_fractional_val = (reg & 0xFFFF);
	}
}

static void sde_hw_intf_setup_dpu_sync_prog_intf_offset(
		struct sde_hw_intf *intf,
		const struct intf_prog_fetch *fetch)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;
	u32 fetch_start = fetch->enable ? fetch->fetch_start : 0;

	SDE_REG_WRITE(c, INTF_DPU_SYNC_PROG_INTF_OFFSET_EN, fetch_start);
}

static void sde_hw_intf_enable_dpu_sync_ctrl(struct sde_hw_intf *intf,
		u32 timing_en_mux_sel)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;
	u32 dpu_sync_ctrl;

	dpu_sync_ctrl = SDE_REG_READ(c, INTF_DPU_SYNC_CTRL);

	if (timing_en_mux_sel)
		dpu_sync_ctrl |= BIT(0);
	else
		dpu_sync_ctrl &= ~BIT(0);

	if (timing_en_mux_sel) {
		SDE_REG_WRITE(c, INTF_DPU_SYNC_CTRL, dpu_sync_ctrl);
	} else {
		SDE_REG_WRITE(c, INTF_TIMING_ENGINE_EN, 0x1);
		/* make sure Slave DPU timing engine is enabled */
		wmb();
		SDE_REG_WRITE(c, INTF_DPU_SYNC_CTRL, dpu_sync_ctrl);
		/* make sure Slave DPU timing engine is unlinked from Master DPU */
		wmb();
	}
}
static void sde_hw_intf_setup_vsync_source(struct sde_hw_intf *intf, u32 frame_rate)
{
	struct sde_hw_blk_reg_map *c;
	u32 reg = 0;

	if (!intf)
		return;

	c = &intf->hw;

	reg = CALCULATE_WD_LOAD_VALUE(frame_rate);
	SDE_REG_WRITE(c, INTF_WD_TIMER_0_LOAD_VALUE, reg);

	SDE_REG_WRITE(c, INTF_WD_TIMER_0_CTL, BIT(0)); /* clear timer */
	reg = BIT(8); /* enable heartbeat timer */
	reg |= BIT(0); /* enable WD timer */
	reg |= BIT(1); /* select default 16 clock ticks */
	SDE_REG_WRITE(c, INTF_WD_TIMER_0_CTL2, reg);

	/* make sure that timers are enabled/disabled for vsync state */
	wmb();
}

static void sde_hw_intf_bind_pingpong_blk(
		struct sde_hw_intf *intf,
		bool enable,
		const enum sde_pingpong pp)
{
	struct sde_hw_blk_reg_map *c;
	u32 mux_cfg;

	if (!intf)
		return;

	c = &intf->hw;

	if (enable) {
		mux_cfg = SDE_REG_READ(c, INTF_MUX);
		mux_cfg &= ~0x0f;
		mux_cfg |= (pp - PINGPONG_0) & 0x7;
		/* Splitlink case, pp0->sublink0, pp1->sublink1 */
		if (intf->cfg.split_link_en)
			mux_cfg = 0x10000;
	} else {
		mux_cfg = 0xf000f;
	}

	SDE_REG_WRITE(c, INTF_MUX, mux_cfg);
}

static u32 sde_hw_intf_get_frame_count(struct sde_hw_intf *intf)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;
	bool en;

	/*
	 * MDP VSync Frame Count is enabled with programmable fetch
	 * or with auto-refresh enabled.
	 */
	en  = (SDE_REG_READ(c, INTF_TEAR_AUTOREFRESH_CONFIG) & BIT(31)) |
			(SDE_REG_READ(c, INTF_CONFIG) & BIT(31));

	if (en && (intf->cap->features & BIT(SDE_INTF_MDP_VSYNC_FC)))
		return SDE_REG_READ(c, INTF_MDP_FRAME_COUNT);
	else
		return SDE_REG_READ(c, INTF_FRAME_COUNT);
}

static void sde_hw_intf_get_status(
		struct sde_hw_intf *intf,
		struct intf_status *s)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;

	s->is_en = SDE_REG_READ(c, INTF_TIMING_ENGINE_EN);
	if (s->is_en) {
		s->frame_count = SDE_REG_READ(c, INTF_FRAME_COUNT);
		s->line_count = SDE_REG_READ(c, INTF_LINE_COUNT) & 0xffff;
	} else {
		s->line_count = 0;
		s->frame_count = 0;
	}
}

static void sde_hw_intf_v1_get_status(
		struct sde_hw_intf *intf,
		struct intf_status *s)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;

	s->is_en = SDE_REG_READ(c, INTF_STATUS) & BIT(0);
	s->is_prog_fetch_en = (SDE_REG_READ(c, INTF_CONFIG) & BIT(31));
	if (s->is_en) {
		s->frame_count = sde_hw_intf_get_frame_count(intf);
		s->line_count = SDE_REG_READ(c, INTF_LINE_COUNT) & 0xffff;
	} else {
		s->line_count = 0;
		s->frame_count = 0;
	}
}
static void sde_hw_intf_setup_misr(struct sde_hw_intf *intf,
						bool enable, u32 frame_count)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;
	u32 config = 0;

	SDE_REG_WRITE(c, INTF_MISR_CTRL, MISR_CTRL_STATUS_CLEAR);
	/* clear misr data */
	wmb();

	if (enable)
		config = (frame_count & MISR_FRAME_COUNT_MASK) |
				MISR_CTRL_ENABLE |
				INTF_MISR_CTRL_FREE_RUN_MASK |
				INTF_MISR_CTRL_INPUT_SEL_DATA;

	SDE_REG_WRITE(c, INTF_MISR_CTRL, config);
}

static int sde_hw_intf_collect_misr(struct sde_hw_intf *intf, bool nonblock,
		 u32 *misr_value)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;
	u32 ctrl = 0;
	int rc = 0;

	if (!misr_value)
		return -EINVAL;

	ctrl = SDE_REG_READ(c, INTF_MISR_CTRL);
	if (!nonblock) {
		if (ctrl & MISR_CTRL_ENABLE) {
			rc = read_poll_timeout(sde_reg_read, ctrl, (ctrl & MISR_CTRL_STATUS) > 0,
					500, false, 84000, c, INTF_MISR_CTRL);
			if (rc)
				return rc;
		} else {
			return -EINVAL;
		}
	}

	*misr_value =  SDE_REG_READ(c, INTF_MISR_SIGNATURE);
	return rc;
}

static u32 sde_hw_intf_get_line_count(struct sde_hw_intf *intf)
{
	struct sde_hw_blk_reg_map *c;

	if (!intf)
		return 0;

	c = &intf->hw;

	return SDE_REG_READ(c, INTF_LINE_COUNT) & 0xffff;
}

static u32 sde_hw_intf_get_underrun_line_count(struct sde_hw_intf *intf)
{
	struct sde_hw_blk_reg_map *c;
	u32 hsync_period;

	if (!intf)
		return 0;

	c = &intf->hw;
	hsync_period = SDE_REG_READ(c, INTF_HSYNC_CTL);
	hsync_period = ((hsync_period & 0xffff0000) >> 16);

	return hsync_period ?
		SDE_REG_READ(c, INTF_UNDERRUN_COUNT) / hsync_period :
		0xebadebad;
}

static u32 sde_hw_intf_get_intr_status(struct sde_hw_intf *intf)
{
	if (!intf)
		return -EINVAL;

	return SDE_REG_READ(&intf->hw, INTF_INTR_STATUS);
}

static int sde_hw_intf_setup_te_config(struct sde_hw_intf *intf,
		struct sde_hw_tear_check *te)
{
	struct sde_hw_blk_reg_map *c;
	u32 cfg = 0, val;
	spinlock_t tearcheck_spinlock;

	if (!intf)
		return -EINVAL;

	spin_lock_init(&tearcheck_spinlock);
	c = &intf->hw;

	if (te->hw_vsync_mode)
		cfg |= BIT(20);

	cfg |= te->vsync_count;

	/*
	 * Local spinlock is acquired here to avoid pre-emption
	 * as below register programming should be completed in
	 * less than 2^16 vsync clk cycles.
	 */
	spin_lock(&tearcheck_spinlock);
	val = te->start_pos + te->sync_threshold_start + 1;
	SDE_REG_WRITE(c, INTF_TEAR_SYNC_CONFIG_VSYNC, cfg);
	wmb(); /* disable vsync counter before updating single buffer registers */
	SDE_REG_WRITE(c, INTF_TEAR_SYNC_CONFIG_HEIGHT, te->sync_cfg_height);
	SDE_REG_WRITE(c, INTF_TEAR_VSYNC_INIT_VAL, te->vsync_init_val);
	SDE_REG_WRITE(c, INTF_TEAR_RD_PTR_IRQ, te->rd_ptr_irq);
	SDE_REG_WRITE(c, INTF_TEAR_WR_PTR_IRQ, te->wr_ptr_irq);
	SDE_REG_WRITE(c, INTF_TEAR_START_POS, te->start_pos);
	SDE_REG_WRITE(c, INTF_TEAR_TEAR_DETECT_CTRL, te->detect_ctrl);
	if (intf->cap->features & BIT(SDE_INTF_TE_32BIT))
		SDE_REG_WRITE(c,  INTF_TEAR_SYNC_THRESH_EXT,
				((te->sync_threshold_continue & 0xffff0000) |
				(te->sync_threshold_start >> 16)));
	SDE_REG_WRITE(c, INTF_TEAR_SYNC_THRESH,
			((te->sync_threshold_continue << 16) |
			(te->sync_threshold_start & 0xffff)));
	cfg |= BIT(19); /* VSYNC_COUNTER_EN */
	SDE_REG_WRITE(c, INTF_TEAR_SYNC_CONFIG_VSYNC, cfg);
	wmb(); /* ensure vsync_counter_en is written */

	if (intf->cap->features & BIT(SDE_INTF_TE_32BIT))
		SDE_REG_WRITE(c, INTF_TEAR_SYNC_WRCOUNT_EXT, (val >> 16));
	SDE_REG_WRITE(c, INTF_TEAR_SYNC_WRCOUNT, (val & 0xffff));
	spin_unlock(&tearcheck_spinlock);

	return 0;
}

static int sde_hw_intf_setup_autorefresh_config(struct sde_hw_intf *intf,
		struct sde_hw_autorefresh *cfg)
{
	struct sde_hw_blk_reg_map *c;
	u32 refresh_cfg;

	if (!intf || !cfg)
		return -EINVAL;

	c = &intf->hw;

	refresh_cfg = SDE_REG_READ(c, INTF_TEAR_AUTOREFRESH_CONFIG);
	if (cfg->enable)
		refresh_cfg = BIT(31) | cfg->frame_count;
	else
		refresh_cfg &= ~BIT(31);

	SDE_REG_WRITE(c, INTF_TEAR_AUTOREFRESH_CONFIG, refresh_cfg);

	return 0;
}

static int sde_hw_intf_get_autorefresh_config(struct sde_hw_intf *intf,
		struct sde_hw_autorefresh *cfg)
{
	struct sde_hw_blk_reg_map *c;
	u32 val;

	if (!intf || !cfg)
		return -EINVAL;

	c = &intf->hw;
	val = SDE_REG_READ(c, INTF_TEAR_AUTOREFRESH_CONFIG);
	cfg->enable = (val & BIT(31)) >> 31;
	cfg->frame_count = val & 0xffff;

	return 0;
}

static int sde_hw_intf_poll_timeout_wr_ptr(struct sde_hw_intf *intf,
		u32 timeout_us)
{
	struct sde_hw_blk_reg_map *c;
	u32 val, mask = 0;

	if (!intf)
		return -EINVAL;

	if (intf->cap->features & BIT(SDE_INTF_TE_32BIT))
		mask = 0xffffffff;
	else
		mask = 0xffff;

	c = &intf->hw;
	return read_poll_timeout(sde_reg_read, val, (val & mask) >= 1, 10, false, timeout_us,
			c, INTF_TEAR_LINE_COUNT);
}

static int sde_hw_intf_enable_te(struct sde_hw_intf *intf, bool enable)
{
	struct sde_hw_blk_reg_map *c;
	uint32_t val = 0;

	if (!intf)
		return -EINVAL;

	c = &intf->hw;

	if (enable)
		val |= BIT(0);

	if (intf->cap->features & BIT(SDE_INTF_TE_SINGLE_UPDATE))
		val |= BIT(3);

	SDE_REG_WRITE(c, INTF_TEAR_TEAR_CHECK_EN, val);

	if (enable && (intf->cap->features &
				(BIT(SDE_INTF_PANEL_VSYNC_TS) | BIT(SDE_INTF_MDP_VSYNC_TS)))) {
		val = BIT(0);
		if (intf->cap->features & SDE_INTF_VSYNC_TS_SRC_EN)
			val |= BIT(5);

		SDE_REG_WRITE(c, INTF_VSYNC_TIMESTAMP_CTRL, val);
	}

	return 0;
}

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
static u32 ss_hw_intf_get_start(struct sde_hw_intf *intf)
{
	struct sde_hw_blk_reg_map *c;

	if (!intf)
		return 0;

	c = &intf->hw;
	return SDE_REG_READ(c, INTF_TEAR_START_POS);
}
static void ss_hw_intf_update_start(struct sde_hw_intf *intf,
		struct sde_hw_tear_check *te)
{
	struct sde_hw_blk_reg_map *c;
	int cfg;

	if (!intf || !te)
		return;

	c = &intf->hw;
	cfg = SDE_REG_READ(c, INTF_TEAR_SYNC_THRESH);
	cfg &= ~0xFFFF;
	cfg |= te->sync_threshold_start;
	SDE_REG_WRITE(c, INTF_TEAR_SYNC_THRESH, cfg);

	SDE_REG_WRITE(c, INTF_TEAR_START_POS, te->start_pos);

	//SDE_EVT32_PICK(te->start_pos, te->sync_threshold_start); TBR
}
#endif

static void sde_hw_intf_enable_te_level_mode(struct sde_hw_intf *intf, bool enable)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;
	u32 val = 0;

	val = SDE_REG_READ(c, INTF_TEAR_TEAR_CHECK_EN);

	if (enable)
		val |= BIT(7);
	else
		val &= ~BIT(7);

	SDE_REG_WRITE(c, INTF_TEAR_TEAR_CHECK_EN, val);
}

static void sde_hw_intf_update_te(struct sde_hw_intf *intf,
		struct sde_hw_tear_check *te)
{
	struct sde_hw_blk_reg_map *c;
	int cfg;

	if (!intf || !te)
		return;

	c = &intf->hw;
	cfg = SDE_REG_READ(c, INTF_TEAR_SYNC_THRESH);
	cfg &= ~0xFFFF;
	cfg |= te->sync_threshold_start;
	SDE_REG_WRITE(c, INTF_TEAR_SYNC_THRESH, cfg);
	SDE_REG_WRITE(c, INTF_TEAR_START_POS, te->start_pos);
}

static int sde_hw_intf_connect_external_te(struct sde_hw_intf *intf,
		bool enable_external_te)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;
	u32 cfg;
	int orig;

	if (!intf)
		return -EINVAL;

	c = &intf->hw;
	cfg = SDE_REG_READ(c, INTF_TEAR_SYNC_CONFIG_VSYNC);
	orig = (bool)(cfg & BIT(20));
	if (enable_external_te)
		cfg |= BIT(20);
	else
		cfg &= ~BIT(20);
	SDE_REG_WRITE(c, INTF_TEAR_SYNC_CONFIG_VSYNC, cfg);

	return orig;
}

static void sde_hw_intf_update_tearcheck_vsync_count(struct sde_hw_intf *intf, u32 val)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;
	u32 cfg;

	if (!intf)
		return;

	c = &intf->hw;

	cfg = SDE_REG_READ(c, INTF_TEAR_SYNC_CONFIG_VSYNC);

	/* disable external TE */
	cfg &= ~BIT(20);
	SDE_REG_WRITE(c, INTF_TEAR_SYNC_CONFIG_VSYNC, cfg);

	/* update vsync_count and enable back external TE */
	cfg = (val & 0x7ffff);
	cfg |= BIT(19) | BIT(20);
	SDE_REG_WRITE(c, INTF_TEAR_SYNC_CONFIG_VSYNC, cfg);
	wmb(); /* to make sure configs takes effect */
}

static int sde_hw_intf_get_vsync_info(struct sde_hw_intf *intf,
		struct sde_hw_pp_vsync_info *info)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;
	u32 val;

	if (!intf || !info)
		return -EINVAL;

	c = &intf->hw;

	val = SDE_REG_READ(c, INTF_TEAR_VSYNC_INIT_VAL);
	if (intf->cap->features & BIT(SDE_INTF_TE_32BIT))
		info->rd_ptr_init_val = val;
	else
		info->rd_ptr_init_val = val & 0xffff;

	val = SDE_REG_READ(c, INTF_TEAR_INT_COUNT_VAL);
	info->rd_ptr_frame_count = (val & 0xffff0000) >> 16;
	info->rd_ptr_line_count = val & 0xffff;

	if (intf->cap->features & BIT(SDE_INTF_TE_32BIT)) {
		val = SDE_REG_READ(c, INTF_TEAR_INT_COUNT_VAL_EXT);
		info->rd_ptr_line_count |= (val << 16);
	}

	val = SDE_REG_READ(c, INTF_TEAR_LINE_COUNT);
	info->wr_ptr_line_count = val;

	val = sde_hw_intf_get_frame_count(intf);
	info->intf_frame_count = val;

	return 0;
}

static int sde_hw_intf_v1_check_and_reset_tearcheck(struct sde_hw_intf *intf,
		struct intf_tear_status *status)
{
	struct sde_hw_blk_reg_map *c = &intf->hw;
	u32 start_pos, val;

	if (!intf || !status)
		return -EINVAL;

	c = &intf->hw;

	status->read_line_count = SDE_REG_READ(c, INTF_TEAR_INT_COUNT_VAL);
	if (intf->cap->features & BIT(SDE_INTF_TE_32BIT))
		status->read_line_count |= (SDE_REG_READ(c, INTF_TEAR_INT_COUNT_VAL_EXT) << 16);
	start_pos = SDE_REG_READ(c, INTF_TEAR_START_POS);
	val = SDE_REG_READ(c, INTF_TEAR_SYNC_WRCOUNT);
	status->write_frame_count = val >> 16;
	status->write_line_count = start_pos;

	if (intf->cap->features & BIT(SDE_INTF_TE_32BIT)) {
		val = (status->write_line_count & 0xffff0000) >> 16;
		SDE_REG_WRITE(c, INTF_TEAR_SYNC_WRCOUNT_EXT, val);
	}

	val = (status->write_frame_count << 16) | (status->write_line_count & 0xffff);
	SDE_REG_WRITE(c, INTF_TEAR_SYNC_WRCOUNT, val);

	return 0;
}

static void sde_hw_intf_override_tear_rd_ptr_val(struct sde_hw_intf *intf,
		u32 adjusted_rd_ptr_val)
{
	struct sde_hw_blk_reg_map *c;

	if (!intf || !adjusted_rd_ptr_val)
		return;

	c = &intf->hw;

	SDE_REG_WRITE(c, INTF_TEAR_SYNC_WRCOUNT, (adjusted_rd_ptr_val & 0xFFFF));
	/* ensure rd_ptr_val is written */
	wmb();
}

static void sde_hw_intf_vsync_sel(struct sde_hw_intf *intf,
		u32 vsync_source)
{
	struct sde_hw_blk_reg_map *c;

	if (!intf)
		return;

	c = &intf->hw;

	SDE_REG_WRITE(c, INTF_TEAR_MDP_VSYNC_SEL, (vsync_source & 0xf));
}

static void sde_hw_intf_flush_snapshot_setup(struct sde_hw_intf *intf, u32 value, bool enable)
{
	struct sde_hw_blk_reg_map *c;
	u32 intf_cfg;

	if (!intf)
		return;

	c = &intf->hw;
	intf_cfg = SDE_REG_READ(c, INTF_CONFIG);

	if (enable)
		intf_cfg |= BIT(14);
	else
		intf_cfg &= BIT(14);

	SDE_REG_WRITE(c, INTF_PROG_FLUSH_SNAPSHOT, value);
	SDE_REG_WRITE(c, INTF_CONFIG, intf_cfg);
}

static void sde_hw_intf_enable_compressed_input(struct sde_hw_intf *intf,
		bool compression_en, bool dsc_4hs_merge)
{
	struct sde_hw_blk_reg_map *c;
	u32 intf_cfg2;

	if (!intf)
		return;

	/*
	 * callers can either call this function to enable/disable the 64 bit
	 * compressed input or this configuration can be applied along
	 * with timing generation parameters
	 */

	c = &intf->hw;
	intf_cfg2 = SDE_REG_READ(c, INTF_CONFIG2);

	_check_and_set_comp_bit(intf, dsc_4hs_merge, compression_en,
			&intf_cfg2);

	SDE_REG_WRITE(c, INTF_CONFIG2, intf_cfg2);
}

static void sde_hw_intf_enable_wide_bus(struct sde_hw_intf *intf,
		bool enable)
{
	struct sde_hw_blk_reg_map *c;
	u32 intf_cfg2;

	if (!intf)
		return;

	c = &intf->hw;
	intf_cfg2 = SDE_REG_READ(c, INTF_CONFIG2);
	intf_cfg2 &= ~BIT(0);

	intf_cfg2 |= enable ? BIT(0) : 0;

	SDE_REG_WRITE(c, INTF_CONFIG2, intf_cfg2);
}

static bool sde_hw_intf_is_te_32bit_supported(struct sde_hw_intf *intf)
{
	return (intf->cap->features & BIT(SDE_INTF_TE_32BIT));
}

static void sde_hw_intf_setup_panic_wakeup(struct sde_hw_intf *intf,
		struct intf_panic_wakeup_cfg *cfg)
{
	struct sde_hw_blk_reg_map *c;
	u32 val;

	if (!intf || !cfg)
		return;

	c = &intf->hw;

	SDE_REG_WRITE(c, INTF_TEAR_PANIC_START, cfg->panic_start);
	SDE_REG_WRITE(c, INTF_TEAR_PANIC_WINDOW, cfg->panic_window);
	SDE_REG_WRITE(c, INTF_TEAR_WAKEUP_START, cfg->wakeup_start);
	SDE_REG_WRITE(c, INTF_TEAR_WAKEUP_WINDOW, cfg->wakeup_window);

	val = SDE_REG_READ(c, INTF_TEAR_TEAR_CHECK_EN);
	if (cfg->enable)
		val |= BIT(4) | BIT(5);
	else
		val &= ~(BIT(4) | BIT(5));
	SDE_REG_WRITE(c, INTF_TEAR_TEAR_CHECK_EN, val);
}

static void sde_hw_intf_setup_panic_ctrl(struct sde_hw_intf *intf,
		struct intf_panic_ctrl_cfg *cfg)
{
	struct sde_hw_blk_reg_map *c;

	if (!intf || !cfg)
		return;

	c = &intf->hw;

	SDE_REG_WRITE(c, INTF_RSCC_PANIC_CTRL, cfg->enable ? BIT(0) : 0);
	SDE_REG_WRITE(c, INTF_RSCC_PANIC_LEVEL, cfg->panic_level);
	SDE_REG_WRITE(c, INTF_RSCC_PANIC_EXT_VFP_START,
			cfg->enable ? cfg->ext_vfp_start : 0xFFFFFFFF);
}

static void _setup_intf_ops(struct sde_hw_intf_ops *ops,
		unsigned long cap, unsigned long mdss_cap)
{
	ops->setup_timing_gen = sde_hw_intf_setup_timing_engine;
	ops->setup_prg_fetch  = sde_hw_intf_setup_prg_fetch;
	ops->setup_prog_dynref  = sde_hw_intf_setup_prog_dynref;
	ops->enable_timing = sde_hw_intf_enable_timing_engine;
	ops->setup_misr = sde_hw_intf_setup_misr;
	ops->collect_misr = sde_hw_intf_collect_misr;
	ops->get_line_count = sde_hw_intf_get_line_count;
	ops->get_underrun_line_count = sde_hw_intf_get_underrun_line_count;
	ops->get_intr_status = sde_hw_intf_get_intr_status;
	ops->avr_setup = sde_hw_intf_avr_setup;
	ops->avr_trigger = sde_hw_intf_avr_trigger;
	ops->avr_ctrl = sde_hw_intf_avr_ctrl;
	ops->avr_enable = sde_hw_intf_avr_enable;
	ops->enable_compressed_input = sde_hw_intf_enable_compressed_input;
	ops->enable_wide_bus = sde_hw_intf_enable_wide_bus;
	ops->is_te_32bit_supported = sde_hw_intf_is_te_32bit_supported;

	if (cap & BIT(SDE_INTF_PANIC_CTRL)) {
		ops->raw_te_setup = sde_hw_intf_raw_te_setup;
		ops->setup_intf_panic_ctrl = sde_hw_intf_setup_panic_ctrl;
	}

	if (cap & BIT(SDE_INTF_STATUS))
		ops->get_status = sde_hw_intf_v1_get_status;
	else
		ops->get_status = sde_hw_intf_get_status;

	if (cap & BIT(SDE_INTF_INPUT_CTRL))
		ops->bind_pingpong_blk = sde_hw_intf_bind_pingpong_blk;

	if (cap & BIT(SDE_INTF_WD_TIMER))
		ops->setup_vsync_source = sde_hw_intf_setup_vsync_source;

	if (cap & BIT(SDE_INTF_AVR_STATUS))
		ops->get_avr_status = sde_hw_intf_get_avr_status;

	if (cap & BIT(SDE_INTF_NUM_AVR_STEP)) {
		ops->set_num_avr_step = sde_hw_intf_set_num_avr_step;
		ops->get_cur_num_avr_step = sde_hw_intf_get_cur_num_avr_step;
	}

	if (cap & BIT(SDE_INTF_ESYNC)) {
		ops->prepare_esync = sde_hw_intf_prepare_esync;
		ops->enable_esync = sde_hw_intf_enable_esync;
		ops->prepare_backup_esync = sde_hw_intf_prepare_backup_esync;
		ops->enable_backup_esync = sde_hw_intf_enable_backup_esync;
		ops->wait_for_esync_src_switch = sde_hw_intf_wait_for_esync_src_switch;
		ops->enable_infinite_vfp = sde_hw_intf_enable_infinite_vfp;
		ops->get_esync_timestamp = sde_hw_intf_get_esync_timestamp;
	}

	if (cap & BIT(SDE_INTF_TE)) {
		ops->setup_tearcheck = sde_hw_intf_setup_te_config;
		ops->enable_tearcheck = sde_hw_intf_enable_te;
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
		ops->get_start = ss_hw_intf_get_start;
		ops->update_start = ss_hw_intf_update_start;
#endif
		ops->update_tearcheck = sde_hw_intf_update_te;
		ops->connect_external_te = sde_hw_intf_connect_external_te;
		ops->get_vsync_info = sde_hw_intf_get_vsync_info;
		ops->setup_autorefresh = sde_hw_intf_setup_autorefresh_config;
		ops->get_autorefresh = sde_hw_intf_get_autorefresh_config;
		ops->poll_timeout_wr_ptr = sde_hw_intf_poll_timeout_wr_ptr;
		ops->vsync_sel = sde_hw_intf_vsync_sel;
		ops->check_and_reset_tearcheck = sde_hw_intf_v1_check_and_reset_tearcheck;
		ops->override_tear_rd_ptr_val = sde_hw_intf_override_tear_rd_ptr_val;
		ops->update_tearcheck_vsync_count = sde_hw_intf_update_tearcheck_vsync_count;

		if (cap & BIT(SDE_INTF_PANIC_CTRL))
			ops->setup_te_panic_wakeup = sde_hw_intf_setup_panic_wakeup;

		if (cap & BIT(SDE_INTF_TEAR_TE_LEVEL_MODE))
			ops->enable_te_level_trigger = sde_hw_intf_enable_te_level_mode;
		else if (cap & BIT(SDE_INTF_TE_LEVEL_TRIGGER))
			ops->enable_te_level_trigger = sde_hw_intf_enable_te_level_trigger;
	}

	if (cap & BIT(SDE_INTF_RESET_COUNTER))
		ops->reset_counter = sde_hw_intf_reset_counter;

	if (cap & (BIT(SDE_INTF_PANEL_VSYNC_TS) | BIT(SDE_INTF_MDP_VSYNC_TS)))
		ops->get_vsync_timestamp = sde_hw_intf_get_vsync_timestamp;

	if (mdss_cap & BIT(SDE_MDP_DUAL_DPU_SYNC)) {
		ops->setup_dpu_sync_prog_intf_offset =
			sde_hw_intf_setup_dpu_sync_prog_intf_offset;
		ops->enable_dpu_sync_ctrl = sde_hw_intf_enable_dpu_sync_ctrl;
	}

	if (cap & BIT(SDE_INTF_WD_JITTER))
		ops->configure_wd_jitter = sde_hw_intf_configure_wd_timer_jitter;

	if (cap & BIT(SDE_INTF_WD_LTJ_CTL))
		ops->get_wd_ltj_status = sde_hw_intf_read_wd_ltj_ctl;

	if (mdss_cap & BIT(SDE_MDP_HW_FLUSH_SYNC))
		ops->setup_flush_snapshot =  sde_hw_intf_flush_snapshot_setup;
}

struct sde_hw_blk_reg_map *sde_hw_intf_init(enum sde_intf idx,
		void __iomem *addr,
		struct sde_mdss_cfg *m)
{
	struct sde_hw_intf *c;
	struct sde_intf_cfg *cfg;

	c = kzalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return ERR_PTR(-ENOMEM);

	cfg = _intf_offset(idx, m, addr, &c->hw);
	if (IS_ERR_OR_NULL(cfg)) {
		kfree(c);
		pr_err("failed to create sde_hw_intf %d\n", idx);
		return ERR_PTR(-EINVAL);
	}

	/*
	 * Assign ops
	 */
	c->idx = idx;
	c->cap = cfg;
	c->mdss = m;
	_setup_intf_ops(&c->ops, c->cap->features, m->mdp[0].features);

	sde_dbg_reg_register_dump_range(SDE_DBG_NAME, cfg->name, c->hw.blk_off,
			c->hw.blk_off + c->hw.length, c->hw.xin_id);

	return &c->hw;
}

void sde_hw_intf_destroy(struct sde_hw_blk_reg_map *hw)
{
	if (hw)
		kfree(to_sde_hw_intf(hw));
}
