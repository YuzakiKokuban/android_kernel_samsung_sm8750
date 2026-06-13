/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021-2023, Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2012-2021, The Linux Foundation. All rights reserved.
 */

#ifndef _DP_PARSER_H_
#define _DP_PARSER_H_

#include <linux/sde_io_util.h>

#define DP_LABEL "MDSS DP DISPLAY"
#define AUX_CFG_LEN	10
#define DP_MAX_PIXEL_CLK_KHZ	675000
#define DP_MAX_LINK_CLK_KHZ	810000
#define MAX_DP_MST_STREAMS	2
#define MAX_SWING_LEVELS 4
#define MAX_PRE_EMP_LEVELS 4

enum dp_pm_type {
	DP_CORE_PM,
	DP_CTRL_PM,
	DP_PHY_PM,
	DP_STREAM0_PM,
	DP_STREAM1_PM,
	DP_LINK_PM,
	DP_PLL_PM,
	DP_MAX_PM
};

enum dp_pin_states {
	DP_GPIO_AUX_ENABLE,
	DP_GPIO_AUX_SEL,
	DP_GPIO_USBPLUG_CC,
	DP_GPIO_CMN_MAX = DP_GPIO_USBPLUG_CC,
	DP_GPIO_EDP_VCC_EN,
	DP_GPIO_EDP_MIN = DP_GPIO_EDP_VCC_EN,
	DP_GPIO_EDP_BACKLIGHT_PWR,
	DP_GPIO_EDP_PWM,
	DP_GPIO_EDP_BACKLIGHT_EN,
	DP_GPIO_EDP_MAX,
	DP_GPIO_MAX = DP_GPIO_EDP_MAX,
};

static inline const char *dp_parser_pm_name(enum dp_pm_type module)
{
	switch (module) {
	case DP_CORE_PM:	return "DP_CORE_PM";
	case DP_CTRL_PM:	return "DP_CTRL_PM";
	case DP_PHY_PM:		return "DP_PHY_PM";
	case DP_STREAM0_PM:	return "DP_STREAM0_PM";
	case DP_STREAM1_PM:	return "DP_STREAM1_PM";
	case DP_LINK_PM:	return "DP_LINK_PM";
	case DP_PLL_PM:		return "DP_PLL_PM";
	default:		return "???";
	}
}

/**
 * struct dp_display_data  - display related device tree data.
 *
 * @ctrl_node: referece to controller device
 * @phy_node:  reference to phy device
 * @is_active: is the controller currently active
 * @name: name of the display
 * @display_type: type of the display
 */
struct dp_display_data {
	struct device_node *ctrl_node;
	struct device_node *phy_node;
	bool is_active;
	const char *name;
	const char *display_type;
};

/**
 * struct dp_io_data - data structure to store DP IO related info
 * @name: name of the IO
 * @buf: buffer corresponding to IO for debugging
 * @io: io data which give len and mapped address
 */
struct dp_io_data {
	const char *name;
	u8 *buf;
	struct dss_io_data io;
};

/**
 * struct dp_io - data struct to store array of DP IO info
 * @len: total number of IOs
 * @data: pointer to an array of DP IO data structures.
 */
struct dp_io {
	u32 len;
	struct dp_io_data *data;
};

/**
 * struct dp_pinctrl - DP's pin control
 *
 * @pin: pin-controller's instance
 * @state_active: active state pin control
 * @state_hpd_active: hpd active state pin control
 * @state_suspend: suspend state pin control
 */
struct dp_pinctrl {
	struct pinctrl *pin;
	struct pinctrl_state *state_active;
	struct pinctrl_state *state_hpd_active;
	struct pinctrl_state *state_hpd_tlmm;
	struct pinctrl_state *state_hpd_ctrl;
	struct pinctrl_state *state_suspend;
};

#define DP_ENUM_STR(x)	#x
#define DP_AUX_CFG_MAX_VALUE_CNT 3
/**
 * struct dp_aux_cfg - DP's AUX configuration settings
 *
 * @cfg_cnt: count of the configurable settings for the AUX register
 * @current_index: current index of the AUX config lut
 * @offset: register offset of the AUX config register
 * @lut: look up table for the AUX config values for this register
 */
struct dp_aux_cfg {
	u32 cfg_cnt;
	u32 current_index;
	u32 offset;
	u32 lut[DP_AUX_CFG_MAX_VALUE_CNT];
};

/* PHY AUX config registers */
enum dp_phy_aux_config_type {
	PHY_AUX_CFG0,
	PHY_AUX_CFG1,
	PHY_AUX_CFG2,
	PHY_AUX_CFG3,
	PHY_AUX_CFG4,
	PHY_AUX_CFG5,
	PHY_AUX_CFG6,
	PHY_AUX_CFG7,
	PHY_AUX_CFG8,
	PHY_AUX_CFG9,
	PHY_AUX_CFG_MAX,
};

/**
 * enum dp_phy_version - version of the dp phy
 * @DP_PHY_VERSION_UNKNOWN: Unknown controller version
 * @DP_PHY_VERSION_4_2_0:   DP phy v4.2.0 controller
 * @DP_PHY_VERSION_5_0_0:   DP phy v5.0.0 controller
 * @DP_PHY_VERSION_6_0_0:   DP phy v6.0.0 controller
 * @DP_PHY_VERSION_8_0_0:   DP phy v8.0.0 controller
 * @DP_PHY_VERSION_MAX:     max version
 */
enum dp_phy_version {
	DP_PHY_VERSION_UNKNOWN,
	DP_PHY_VERSION_2_0_0 = 0x200,
	DP_PHY_VERSION_4_2_0 = 0x420,
	DP_PHY_VERSION_5_0_0 = 0x500,
	DP_PHY_VERSION_6_0_0 = 0x600,
	DP_PHY_VERSION_8_0_0 = 0x800,
	DP_PHY_VERSION_MAX
};

/**
 * enum dp_phy_mode - mode of the dp phy
 * @DP_PHY_MODE_UNKNOWN: Unknown PHY mode
 * @DP_PHY_MODE_DP:      DP PHY mode
 * @DP_PHY_MODE_MINIDP:  MiniDP PHY mode
 * @DP_PHY_MODE_EDP:     eDP PHY mode
 * @DP_PHY_MODE_EDP_HIGH_SWING:   eDP PHY mode, high swing/pre-empahsis
 * @DP_PHY_MODE_MAX:     max PHY mode
 */
enum dp_phy_mode {
	DP_PHY_MODE_UNKNOWN = 0,
	DP_PHY_MODE_DP,
	DP_PHY_MODE_MINIDP,
	DP_PHY_MODE_EDP,
	DP_PHY_MODE_EDP_HIGH_SWING,
	DP_PHY_MODE_MAX
};

/**
 * struct dp_hw_cfg - DP HW specific configuration
 *
 * @phy_version: DP PHY HW version
 */
struct dp_hw_cfg {
	enum dp_phy_version phy_version;
	enum dp_phy_mode phy_mode;
};

static inline char *dp_phy_aux_config_type_to_string(u32 cfg_type)
{
	switch (cfg_type) {
	case PHY_AUX_CFG0:
		return DP_ENUM_STR(PHY_AUX_CFG0);
	case PHY_AUX_CFG1:
		return DP_ENUM_STR(PHY_AUX_CFG1);
	case PHY_AUX_CFG2:
		return DP_ENUM_STR(PHY_AUX_CFG2);
	case PHY_AUX_CFG3:
		return DP_ENUM_STR(PHY_AUX_CFG3);
	case PHY_AUX_CFG4:
		return DP_ENUM_STR(PHY_AUX_CFG4);
	case PHY_AUX_CFG5:
		return DP_ENUM_STR(PHY_AUX_CFG5);
	case PHY_AUX_CFG6:
		return DP_ENUM_STR(PHY_AUX_CFG6);
	case PHY_AUX_CFG7:
		return DP_ENUM_STR(PHY_AUX_CFG7);
	case PHY_AUX_CFG8:
		return DP_ENUM_STR(PHY_AUX_CFG8);
	case PHY_AUX_CFG9:
		return DP_ENUM_STR(PHY_AUX_CFG9);
	default:
		return "unknown";
	}
}

#if defined(CONFIG_SECDP)
enum secdp_phy_pre_emphasis_type {
	PHY_PRE_EMP0,		/* 0   db */
	PHY_PRE_EMP1,		/* 3.5 db */
	PHY_PRE_EMP2,		/* 6.0 db */
	PHY_PRE_EMP3,		/* 9.5 db */
//	MAX_PRE_EMP_LEVELS,
};

enum secdp_phy_voltage_type {
	PHY_VOLTAGE_SWING0,	/* 0.4 v */
	PHY_VOLTAGE_SWING1,	/* 0.6 v */
	PHY_VOLTAGE_SWING2,	/* 0.8 v */
	PHY_VOLTAGE_SWING3,	/* 1.2 v, optional */
	MAX_VOLTAGE_LEVELS,
};

#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
enum secdp_ps5169_pre_emphasis_type {
	PHY_PS5169_EMP0,	/* 0   db */
	PHY_PS5169_EMP1,	/* 3.5 db */
	PHY_PS5169_EMP2,	/* 6.0 db */
	PHY_PS5169_EMP3,	/* 9.5 db */
	MAX_PS5169_EMP_LEVELS,
};

enum secdp_PS5169_voltage_type {
	PHY_PS5169_SWING0,	/* 0.4 v */
	PHY_PS5169_SWING1,	/* 0.6 v */
	PHY_PS5169_SWING2,	/* 0.8 v */
	PHY_PS5169_SWING3,	/* 1.2 v, optional */
	MAX_PS5169_SWING_LEVELS,
};
#endif/*CONFIG_COMBO_REDRIVER_PS5169*/
#endif/*CONFIG_SECDP*/

/**
 * struct dp_parser - DP parser's data exposed to clients
 *
 * @pdev: platform data of the client
 * @msm_hdcp_dev: device pointer for the HDCP driver
 * @mp: gpio, regulator and clock related data
 * @pinctrl: pin-control related data
 * @disp_data: controller's display related data
 * @l_pnswap: P/N swap status on each lane
 * @max_pclk_khz: maximum pixel clock supported for the platform
 * @max_lclk_khz: maximum link clock supported for the platform
 * @hw_cfg: DP HW specific settings
 * @has_mst: MST feature enable status
 * @has_mst_sideband: MST sideband feature enable status
 * @gpio_aux_switch: presence GPIO AUX switch status
 * @dsc_feature_enable: DSC feature enable status
 * @fec_feature_enable: FEC feature enable status
 * @dsc_continuous_pps: PPS sent every frame by HW
 * @has_widebus: widebus (2PPC) feature eanble status
  *@mst_fixed_port: mst port_num reserved for fixed topology
 * @qos_cpu_mask: CPU mask for QOS
 * @qos_cpu_latency: CPU Latency setting for QOS
 * @swing_hbr2_3: Voltage swing levels for HBR2 and HBR3 rates
 * @pre_emp_hbr2_3: Pre-emphasis for HBR2 and HBR3 rates
 * @swing_hbr_rbr: Voltage swing levels for HBR and RBR rates
 * @pre_emp_hbr_rbr: Pre-emphasis for HBR and RBR rates
 * @valid_lt_params: valid lt params
 * @parse: function to be called by client to parse device tree.
 * @get_io: function to be called by client to get io data.
 * @get_io_buf: function to be called by client to get io buffers.
 * @clear_io_buf: function to be called by client to clear io buffers.
 * @mst_fixed_display_type: mst display_type reserved for fixed topology
 * @display_type: display type as defined in device tree.
 */
struct dp_parser {
	struct platform_device *pdev;
	struct device *msm_hdcp_dev;
	struct dss_module_power mp[DP_MAX_PM];
	struct dp_pinctrl pinctrl;
	struct dp_io io;
	struct dp_display_data disp_data;

	u8 l_map[4];
	u8 l_pnswap;
	struct dp_aux_cfg aux_cfg[AUX_CFG_LEN];
	u32 max_pclk_khz;
	u32 max_lclk_khz;
	struct dp_hw_cfg hw_cfg;
	bool has_mst;
	bool has_mst_sideband;
	bool dsc_feature_enable;
	bool fec_feature_enable;
	bool dsc_continuous_pps;
	bool has_widebus;
	bool has_4ppc_enabled;
	bool gpio_aux_switch;
	u32 mst_fixed_port[MAX_DP_MST_STREAMS];
	u32 qos_cpu_mask;
	unsigned long qos_cpu_latency;
	u32 pixel_base_off[MAX_DP_MST_STREAMS];
	const char *mst_fixed_display_type[MAX_DP_MST_STREAMS];
	const char *display_type;

	u8 *swing_hbr2_3;
	u8 *pre_emp_hbr2_3;

	u8 *swing_hbr_rbr;
	u8 *pre_emp_hbr_rbr;
	bool valid_lt_params;

#if defined(CONFIG_SECDP)
	struct regulator *aux_pullup_vreg;
	bool cc_dir_inv;  /* CC_DIR is inversed, e.g, T865 */
	bool aux_sel_inv; /* inverse control of AUX_SEL e.g, D2Xq hwid 01,02 */
	int  use_redrv;   /* ptn36502 needs NOT AUX switch SEL control */
	int  dex_dft_res; /* DeX default resolution, e.g, HG950 */
	bool prefer_support;  /* true if prefer resolution has high priority */
	bool mrr_fps_nolimit; /* true if mirroring refresh rate has no limit */
	bool rf_tx_backoff;   /* true if it RF TX Backoff is supported, for SHELL-less type connector */
	bool mst_support;     /* true if MST is supported */

	u8 *preshoot0_hbr2_3;
	u8 *preshoot1_hbr2_3;
	u8 *preshoot0_rbr_hbr;
	u8 *preshoot1_rbr_hbr;
	bool valid_preshoot_params;

#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
	u8 ps5169_rbr_eq0[MAX_PS5169_SWING_LEVELS][MAX_PS5169_EMP_LEVELS];
	u8 ps5169_rbr_eq1[MAX_PS5169_SWING_LEVELS][MAX_PS5169_EMP_LEVELS];
	u8 ps5169_hbr_eq0[MAX_PS5169_SWING_LEVELS][MAX_PS5169_EMP_LEVELS];
	u8 ps5169_hbr_eq1[MAX_PS5169_SWING_LEVELS][MAX_PS5169_EMP_LEVELS];
	u8 ps5169_hbr2_eq0[MAX_PS5169_SWING_LEVELS][MAX_PS5169_EMP_LEVELS];
	u8 ps5169_hbr2_eq1[MAX_PS5169_SWING_LEVELS][MAX_PS5169_EMP_LEVELS];
	u8 ps5169_hbr3_eq0[MAX_PS5169_SWING_LEVELS][MAX_PS5169_EMP_LEVELS];
	u8 ps5169_hbr3_eq1[MAX_PS5169_SWING_LEVELS][MAX_PS5169_EMP_LEVELS];
#endif/*CONFIG_COMBO_REDRIVER_PS5169*/
#endif/*CONFIG_SECDP*/

	int (*parse)(struct dp_parser *parser);
	struct dp_io_data *(*get_io)(struct dp_parser *parser, char *name);
	void (*get_io_buf)(struct dp_parser *parser, char *name);
	void (*clear_io_buf)(struct dp_parser *parser);
};

enum dp_phy_lane_num {
	DP_PHY_LN0 = 0,
	DP_PHY_LN1 = 1,
	DP_PHY_LN2 = 2,
	DP_PHY_LN3 = 3,
	DP_MAX_PHY_LN = 4,
};

enum dp_mainlink_lane_num {
	DP_ML0 = 0,
	DP_ML1 = 1,
	DP_ML2 = 2,
	DP_ML3 = 3,
};

/**
 * dp_parser_get() - get the DP's device tree parser module
 *
 * @pdev: platform data of the client
 * return: pointer to dp_parser structure.
 *
 * This function provides client capability to parse the
 * device tree and populate the data structures. The data
 * related to clock, regulators, pin-control and other
 * can be parsed using this module.
 */
struct dp_parser *dp_parser_get(struct platform_device *pdev);

/**
 * dp_parser_put() - cleans the dp_parser module
 *
 * @parser: pointer to the parser's data.
 */
void dp_parser_put(struct dp_parser *parser);

#if defined(CONFIG_SECDP)
enum secdp_hw_preshoot_t {
	DP_HW_PRESHOOT_0,
	DP_HW_PRESHOOT_1,
	DP_HW_PRESHOOT_MAX,
};

static inline char *secdp_preshoot_to_string(int hw)
{
	switch (hw) {
	case DP_HW_PRESHOOT_0:
		return DP_ENUM_STR(DP_HW_PRESHOOT_0);
	case DP_HW_PRESHOOT_1:
		return DP_ENUM_STR(DP_HW_PRESHOOT_1);
	default:
		return "unknown";
	}
}

#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
enum secdp_ps5169_eq_t {
	DP_PS5169_EQ0,
	DP_PS5169_EQ1,
	DP_PS5169_EQ_MAX,
};

enum secdp_ps5169_link_rate_t {
	DP_PS5169_RATE_RBR,
	DP_PS5169_RATE_HBR,
	DP_PS5169_RATE_HBR2,
	DP_PS5169_RATE_HBR3,
	DP_PS5169_RATE_MAX,
};
#endif/*CONFIG_COMBO_REDRIVER_PS5169*/
#endif/*CONFIG_SECDP*/

#if defined(CONFIG_SECDP_DBG)
enum secdp_link_rate_t {
	DP_LR_NONE = 0x0,
	DP_LR_HBR_RBR = 0x1,
	DP_LR_HBR2_3 = 0x2,
};

static inline char *secdp_link_rate_to_string(int lr)
{
	switch (lr) {
	case DP_LR_HBR_RBR:
		return DP_ENUM_STR(DP_LR_HBR_RBR);
	case DP_LR_HBR2_3:
		return DP_ENUM_STR(DP_LR_HBR2_3);
	default:
		return "unknown";
	}
}

enum secdp_phy_param_t {
	DP_PARAM_NONE = 0x0,
	DP_PARAM_VX = 0x1,	/* voltage swing */
	DP_PARAM_PX = 0x2,	/* pre-emphasis */
};

static inline char *secdp_phy_type_to_string(int param)
{
	switch (param) {
	case DP_PARAM_VX:
		return DP_ENUM_STR(DP_PARAM_VX);
	case DP_PARAM_PX:
		return DP_ENUM_STR(DP_PARAM_PX);
	default:
		return "unknown";
	}
}

/* voltage swing, pre-emphasis */
int  secdp_parse_vxpx_show(struct dp_parser *parser, enum secdp_link_rate_t lr,
				enum secdp_phy_param_t vxpx, char *buf);
int  secdp_parse_vxpx_store(struct dp_parser *parser, enum secdp_link_rate_t lr,
				enum secdp_phy_param_t vxpx, char *buf);
int  secdp_show_phy_param(struct dp_parser *parser, char *buf);

int secdp_parse_preshoot_show(struct dp_parser *parser, enum secdp_link_rate_t lr,
			enum secdp_hw_preshoot_t prst, char *buf);
int secdp_parse_preshoot_store(struct dp_parser *parser, enum secdp_link_rate_t lr,
			enum secdp_hw_preshoot_t prst, char *buf);
int secdp_show_preshoot_param(struct dp_parser *parser, char *buf);

#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
static inline char *secdp_ps5169_eq_to_string(int hw)
{
	switch (hw) {
	case DP_PS5169_EQ0:
		return DP_ENUM_STR(DP_PS5169_EQ0);
	case DP_PS5169_EQ1:
		return DP_ENUM_STR(DP_PS5169_EQ1);
	default:
		return "unknown";
	}
}

static inline char *secdp_ps5169_rate_to_string(int hw)
{
	switch (hw) {
	case DP_PS5169_RATE_RBR:
		return DP_ENUM_STR(DP_PS5169_RATE_RBR);
	case DP_PS5169_RATE_HBR:
		return DP_ENUM_STR(DP_PS5169_RATE_HBR);
	case DP_PS5169_RATE_HBR2:
		return DP_ENUM_STR(DP_PS5169_RATE_HBR2);
	case DP_PS5169_RATE_HBR3:
		return DP_ENUM_STR(DP_PS5169_RATE_HBR3);
	default:
		return "unknown";
	}
}

int secdp_parse_ps5169_show(struct dp_parser *parser, enum secdp_ps5169_eq_t eq,
			enum secdp_ps5169_link_rate_t link_rate, char *buf);
int secdp_parse_ps5169_store(struct dp_parser *parser, enum secdp_ps5169_eq_t eq,
			enum secdp_ps5169_link_rate_t link_rate, char *buf);
int secdp_show_ps5169_param(struct dp_parser *parser, char *buf);
#endif/*CONFIG_COMBO_REDRIVER_PS5169*/

/* AUX configuration */
int  secdp_aux_cfg_show(struct dp_parser *parser, char *buf);
int  secdp_aux_cfg_store(struct dp_parser *parser, char *buf);
#endif/*CONFIG_SECDP_DBG*/

#endif
