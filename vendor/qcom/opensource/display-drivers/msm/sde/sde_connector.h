/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2016-2021, The Linux Foundation. All rights reserved.
 */

#ifndef _SDE_CONNECTOR_H_
#define _SDE_CONNECTOR_H_

#include <drm/msm_drm_pp.h>
#include <drm/drm_atomic.h>
#include <drm/drm_panel.h>

#include "msm_drv.h"
#include "msm_prop.h"
#include "sde_kms.h"
#include "sde_fence.h"
#include "dsi_display.h"

#define SDE_CONNECTOR_NAME_SIZE	16
#define SDE_CONNECTOR_DHDR_MEMPOOL_MAX_SIZE	SZ_32
#define MAX_CMD_RECEIVE_SIZE       256
#define DNSC_BLUR_MAX_COUNT	1

struct sde_connector;
struct sde_connector_state;

/**
 * struct sde_connector_ops - callback functions for generic sde connector
 * Individual callbacks documented below.
 */
struct sde_connector_ops {
	/**
	 * post_init - perform additional initialization steps
	 * @connector: Pointer to drm connector structure
	 * @display: Pointer to private display handle
	 * Returns: Zero on success
	 */
	int (*post_init)(struct drm_connector *connector,
			void *display);

	/**
	 * set_info_blob - initialize given info blob
	 * @connector: Pointer to drm connector structure
	 * @info: Pointer to sde connector info structure
	 * @display: Pointer to private display handle
	 * @mode_info: Pointer to mode info structure
	 * Returns: Zero on success
	 */
	int (*set_info_blob)(struct drm_connector *connector,
			void *info,
			void *display,
			struct msm_mode_info *mode_info);

	/**
	 * detect - determine if connector is connected
	 * @connector: Pointer to drm connector structure
	 * @force: Force detect setting from drm framework
	 * @display: Pointer to private display handle
	 * Returns: Connector 'is connected' status
	 */
	enum drm_connector_status (*detect)(struct drm_connector *connector,
			bool force,
			void *display);

	/**
	 * detect_ctx - determine if connector is connected
	 * @connector: Pointer to drm connector structure
	 * @ctx: Pointer to drm modeset acquire context structure
	 * @force: Force detect setting from drm framework
	 * @display: Pointer to private display handle
	 * Returns: Connector 'is connected' status
	 */
	int (*detect_ctx)(struct drm_connector *connector,
			struct drm_modeset_acquire_ctx *ctx,
			bool force,
			void *display);

	/**
	 * get_modes - add drm modes via drm_mode_probed_add()
	 * @connector: Pointer to drm connector structure
	 * @display: Pointer to private display handle
	 * @avail_res: Pointer with current available resources
	 * Returns: Number of modes added
	 */
	int (*get_modes)(struct drm_connector *connector,
			void *display,
			const struct msm_resource_caps_info *avail_res);

	/**
	 * update_pps - update pps command for the display panel
	 * @connector: Pointer to drm connector structure
	 * @pps_cmd: Pointer to pps command
	 * @display: Pointer to private display handle
	 * Returns: Zero on success
	 */
	int (*update_pps)(struct drm_connector *connector,
			char *pps_cmd, void *display);

	/**
	 * mode_valid - determine if specified mode is valid
	 * @connector: Pointer to drm connector structure
	 * @mode: Pointer to drm mode structure
	 * @display: Pointer to private display handle
	 * @avail_res: Pointer with curr available resources
	 * Returns: Validity status for specified mode
	 */
	enum drm_mode_status (*mode_valid)(struct drm_connector *connector,
			struct drm_display_mode *mode,
			void *display,
			const struct msm_resource_caps_info *avail_res);

	/**
	 * set_property - set property value
	 * @connector: Pointer to drm connector structure
	 * @state: Pointer to drm connector state structure
	 * @property_index: DRM property index
	 * @value: Incoming property value
	 * @display: Pointer to private display structure
	 * Returns: Zero on success
	 */
	int (*set_property)(struct drm_connector *connector,
			struct drm_connector_state *state,
			int property_index,
			uint64_t value,
			void *display);

	/**
	 * get_property - get property value
	 * @connector: Pointer to drm connector structure
	 * @state: Pointer to drm connector state structure
	 * @property_index: DRM property index
	 * @value: Pointer to variable for accepting property value
	 * @display: Pointer to private display structure
	 * Returns: Zero on success
	 */
	int (*get_property)(struct drm_connector *connector,
			struct drm_connector_state *state,
			int property_index,
			uint64_t *value,
			void *display);

	/**
	 * get_info - get display information
	 * @connector: Pointer to drm connector structure
	 * @info: Pointer to msm display info structure
	 * @display: Pointer to private display structure
	 * Returns: Zero on success
	 */
	int (*get_info)(struct drm_connector *connector,
			struct msm_display_info *info, void *display);

	/**
	 * get_mode_info - retrieve mode information
	 * @connector: Pointer to drm connector structure
	 * @drm_mode: Display mode set for the display
	 * @sub_mode: Additional mode info to drm display mode
	 * @mode_info: Out parameter. information of the display mode
	 * @display: Pointer to private display structure
	 * @avail_res: Pointer with curr available resources
	 * Returns: Zero on success
	 */
	int (*get_mode_info)(struct drm_connector *connector,
			const struct drm_display_mode *drm_mode,
			struct msm_sub_mode *sub_mode,
			struct msm_mode_info *mode_info,
			void *display,
			const struct msm_resource_caps_info *avail_res);

	/**
	 * enable_event - notify display of event registration/unregistration
	 * @connector: Pointer to drm connector structure
	 * @event_idx: SDE connector event index
	 * @enable: Whether the event is being enabled/disabled
	 * @display: Pointer to private display structure
	 */
	void (*enable_event)(struct drm_connector *connector,
			uint32_t event_idx, bool enable, void *display);

	/**
	 * set_backlight - set backlight level
	 * @connector: Pointer to drm connector structure
	 * @display: Pointer to private display structure
	 * @bl_lvel: Backlight level
	 */
	int (*set_backlight)(struct drm_connector *connector,
			void *display, u32 bl_lvl);

	/**
	 * set_colorspace - set colorspace for connector
	 * @connector: Pointer to drm connector structure
	 * @display: Pointer to private display structure
	 */
	int (*set_colorspace)(struct drm_connector *connector,
			void *display);

	/**
	 * soft_reset - perform a soft reset on the connector
	 * @display: Pointer to private display structure
	 * Return: Zero on success, -ERROR otherwise
	 */
	int (*soft_reset)(void *display);

	/**
	 * pre_kickoff - trigger display to program kickoff-time features
	 * @connector: Pointer to drm connector structure
	 * @display: Pointer to private display structure
	 * @params: Parameter bundle of connector-stored information for
	 *	kickoff-time programming into the display
	 * Returns: Zero on success
	 */
	int (*pre_kickoff)(struct drm_connector *connector,
			void *display,
			struct msm_display_kickoff_params *params);

	/**
	 * clk_ctrl - perform clk enable/disable on the connector
	 * @handle: Pointer to clk handle
	 * @type: Type of clks
	 * @enable: State of clks
	 */
	int (*clk_ctrl)(void *handle, u32 type, u32 state);

	/**
	 * clk_get_rate - get DSI managed clock's rate by type and interface index
	 * @handle: Pointer to DSI display
	 * @idx: Interface index
	 * @clk_type: Type of clock
	 * @clk_rate: Clock rate is placed here
	 * Returns: Zero on success
	 */
	int (*clk_get_rate)(void *handle, u32 idx, u32 clk_type, u64 *clk_rate);

	/**
	 * idle_pc_ctrl - inform DSI of the idle PC status
	 * @display: Pointer to display struct
	 * @idle_pc: Idle power collapse status
	 */
	void (*idle_pc_ctrl)(void *display, bool idle_pc);

	/**
	 * set_power - update dpms setting
	 * @connector: Pointer to drm connector structure
	 * @power_mode: One of the following,
	 *              SDE_MODE_DPMS_ON
	 *              SDE_MODE_DPMS_LP1
	 *              SDE_MODE_DPMS_LP2
	 *              SDE_MODE_DPMS_OFF
	 * @display: Pointer to private display structure
	 * Returns: Zero on success
	 */
	int (*set_power)(struct drm_connector *connector,
			int power_mode, void *display);

	/**
	 * get_dst_format - get dst_format from display
	 * @connector: Pointer to drm connector structure
	 * @display: Pointer to private display handle
	 * Returns: dst_format of display
	 */
	enum dsi_pixel_format (*get_dst_format)(struct drm_connector *connector,
			void *display);

	/**
	 * post_kickoff - display to program post kickoff-time features
	 * @connector: Pointer to drm connector structure
	 * @params: Parameter bundle of connector-stored information for
	 *	post kickoff programming into the display
	 * Returns: Zero on success
	 */
	int (*post_kickoff)(struct drm_connector *connector,
		struct msm_display_conn_params *params);

	/**
	 * post_open - calls connector to process post open functionalities
	 * @display: Pointer to private display structure
	 */
	void (*post_open)(struct drm_connector *connector, void *display);

	/**
	 * check_status - check status of connected display panel
	 * @connector: Pointer to drm connector structure
	 * @display: Pointer to private display handle
	 * @te_check_override: Whether check TE from panel or default check
	 * Returns: positive value for success, negetive or zero for failure
	 */
	int (*check_status)(struct drm_connector *connector, void *display,
					bool te_check_override);

	/**
	 * cmd_transfer - Transfer command to the connected display panel
	 * @connector: Pointer to drm connector structure
	 * @display: Pointer to private display handle
	 * @cmd_buf: Command buffer
	 * @cmd_buf_len: Command buffer length in bytes
	 * @do_peripheral_flush: Flag for sending this command with peripheral flush
	 * Returns: Zero for success, negetive for failure
	 */
	int (*cmd_transfer)(struct drm_connector *connector,
			void *display, const char *cmd_buf,
			u32 cmd_buf_len, bool do_peripheral_flush);
	/**
	 * cmd_receive - Receive the response from the connected display panel
	 * @display: Pointer to private display handle
	 * @cmd_buf: Command buffer
	 * @cmd_buf_len: Command buffer length in bytes
	 * @recv_buf: rx buffer
	 * @recv_buf_len: rx buffer length
	 * @ts: time stamp in nano-seconds of when the command was received
	 * Returns: number of bytes read, if successful, negative for failure
	 */

	int (*cmd_receive)(void *display, const char *cmd_buf,
			   u32 cmd_buf_len, u8 *recv_buf, u32 recv_buf_len, ktime_t *ts);

	/**
	 * config_hdr - configure HDR
	 * @connector: Pointer to drm connector structure
	 * @display: Pointer to private display handle
	 * @c_state: Pointer to connector state
	 * Returns: Zero on success, negative error code for failures
	 */
	int (*config_hdr)(struct drm_connector *connector, void *display,
		struct sde_connector_state *c_state);

	/**
	 * atomic_best_encoder - atomic best encoder selection for connector
	 * @connector: Pointer to drm connector structure
	 * @display: Pointer to private display handle
	 * @c_state: Pointer to connector state
	 * Returns: valid drm_encoder for success
	 */
	struct drm_encoder *(*atomic_best_encoder)(
			struct drm_connector *connector,
			void *display,
			struct drm_connector_state *c_state);

	/**
	 * atomic_check - atomic check handling for connector
	 * @connector: Pointer to drm connector structure
	 * @display: Pointer to private display handle
	 * @c_state: Pointer to connector state
	 * Returns: valid drm_encoder for success
	 */
	int (*atomic_check)(struct drm_connector *connector,
			void *display,
			struct drm_atomic_state *state);

	/**
	 * pre_destroy - handle pre destroy operations for the connector
	 * @connector: Pointer to drm connector structure
	 * @display: Pointer to private display handle
	 * Returns: Zero on success, negative error code for failures
	 */
	void (*pre_destroy)(struct drm_connector *connector, void *display);

	/**
	 * cont_splash_config - initialize splash resources
	 * @display: Pointer to private display handle
	 * Returns: zero for success, negetive for failure
	 */
	int (*cont_splash_config)(void *display);

	/**
	 * cont_splash_res_disable - Remove any splash resources added in probe
	 * @display: Pointer to private display handle
	 * Returns: zero for success, negetive for failure
	 */
	int (*cont_splash_res_disable)(void *display);

	/**
	 * get_panel_vfp - returns original panel vfp
	 * @display: Pointer to private display handle
	 * @h_active: width
	 * @v_active: height
	 * Returns: v_front_porch on success error-code on failure
	 */
	int (*get_panel_vfp)(void *display, int h_active, int v_active);

	/**
	 * get_default_lm - returns default number of lm
	 * @display: Pointer to private display handle
	 * @num_lm: Pointer to number of lms to be populated
	 * Returns: zero for success, negetive for failure
	 */
	int (*get_default_lms)(void *display, u32 *num_lm);

	/**
	 * prepare_commit - trigger display to program pre-commit time features
	 * @display: Pointer to private display structure
	 * @params: Parameter bundle of connector-stored information for
	 *	pre commit time programming into the display
	 * Returns: Zero on success
	 */
	int (*prepare_commit)(void *display,
		struct msm_display_conn_params *params);

	/**
	 * install_properties - install connector properties
	 * @display: Pointer to private display structure
	 * @conn: Pointer to drm connector structure
	 * Returns: Zero on success
	 */
	int (*install_properties)(void *display, struct drm_connector *conn);

	/**
	 * set_allowed_mode_switch - set allowed_mode_switch flag
	 * @connector: Pointer to drm connector structure
	 * @display: Pointer to private display structure
	 */
	void (*set_allowed_mode_switch)(struct drm_connector *connector,
			void *display);

	/**
	 * set_dyn_bit_clk - set target dynamic clock rate
	 * @connector: Pointer to drm connector structure
	 * @value: Target dynamic clock rate
	 * Returns: Zero on success
	 */
	int (*set_dyn_bit_clk)(struct drm_connector *connector, uint64_t value);

	/**
	 * get_qsync_min_fps - Get qsync min fps from qsync-min-fps-list
	 * @conn_state: Pointer to drm_connector_state structure
	 * Returns: Qsync min fps value on success
	 */
	int (*get_qsync_min_fps)(struct drm_connector_state *conn_state);

	/**
	 * get_avr_step_fps - Get the required avr_step for given fps rate
	 * @conn_state: Pointer to drm_connector_state structure
	 * Returns: AVR step fps value on success
	 */
	int (*get_avr_step_fps)(struct drm_connector_state *conn_state);

	/**
	 * dcs_cmd_tx - Arbitrary command using the DSI enum
	 * @conn_state: Pointer to drm_connector_state structure
	 * @cmd: Enum identifying the command
	 * Returns: Zero on success
	 */
	int (*dcs_cmd_tx)(struct drm_connector_state *conn_state, enum dsi_cmd_set_type cmd);

	/**
	 * set_submode_info - populate given sub mode blob
	 * @connector: Pointer to drm connector structure
	 * @info: Pointer to sde connector info structure
	 * @display: Pointer to private display handle
	 * @drm_mode: Pointer to drm_display_mode structure
	 */
	void (*set_submode_info)(struct drm_connector *conn,
		void *info, void *display, struct drm_display_mode *drm_mode);

	/*
	 * get_num_lm_from_mode - Get LM count from topology for this drm mode
	 * @display: Pointer to private display structure
	 * @mode: Pointer to drm mode info structure
	 */
	int (*get_num_lm_from_mode)(void *display, const struct drm_display_mode *mode);

	/*
	 * update_transfer_time - Update transfer time
	 * @display: Pointer to private display structure
	 * @transfer_time: new transfer time to be updated
	 */
	int (*update_transfer_time)(void *display, u32 transfer_time);

	/*
	 * get_panel_scan_line -  get panel scan line
	 * @display: Pointer to private display structure
	 * @scan_line: Pointer to scan_line buffer value
	 * @scan_line_ts:   scan line time stamp value in nano-seconds
	 */
	int (*get_panel_scan_line)(void *display, u16 *scan_line, ktime_t *scan_line_ts);

	/*
	 * check_cmd_defined -  check if a command is defined
	 * @display: Pointer to private display structure
	 * @type: Enum value of DSI command
	 * Returns: True if given command is defined
	 */
	bool (*check_cmd_defined)(void *display, enum dsi_cmd_set_type type);

	/*
	 * avoid_cmd_transfer -  avoid DSI command transfer
	 * @display: Pointer to private display structure
	 * @avoid_transfer: true to avoid transfer, false to allow transfer
	 * Returns: error code
	 */
	int (*avoid_cmd_transfer)(void *display, bool avoid_transfer);
};

/**
 * enum sde_conn_vrr_cmd_state: states of vrr commands
 * @VRR_CMD_STATE_NONE: no-op
 * @VRR_CMD_POWER_ON: handle vrr commands at power on
 * @VRR_CMD_POWER_OFF: handle vrr commands at power off
 * @VRR_CMD_IDLE_ENTRY_START: handle vrr commands at idle pc enter
 * @VRR_CMD_IDLE_ENTRY_COMPLETE: handle vrr commands after idle pc
 * @VRR_CMD_IDLE_EXIT: handle vrr commands at idle pc exit
 * @VRR_CMD_FIRST_SELF_REFRESH: handle vrr commands at first SR
 */
enum sde_conn_vrr_cmd_state {
	VRR_CMD_STATE_NONE,
	VRR_CMD_POWER_ON,
	VRR_CMD_POWER_OFF,
	VRR_CMD_IDLE_ENTRY_START,
	VRR_CMD_IDLE_ENTRY_COMPLETE,
	VRR_CMD_IDLE_EXIT,
	VRR_CMD_FIRST_SELF_REFRESH
};

/**
 * enum sde_connector_avr_step_state: states of avr step fps
 * @AVR_STEP_NONE: no-op
 * @AVR_STEP_ENABLE: enable AVR step
 * #AVR_STEP_DISABLE: disable AVR step
 */
enum sde_connector_avr_step_state {
	AVR_STEP_NONE,
	AVR_STEP_ENABLE,
	AVR_STEP_DISABLE,
};

/**
 * enum sde_connector_display_type - list of display types
 */
enum sde_connector_display {
	SDE_CONNECTOR_UNDEFINED,
	SDE_CONNECTOR_PRIMARY,
	SDE_CONNECTOR_SECONDARY,
	SDE_CONNECTOR_MAX
};

/**
 * enum sde_connector_events - list of recognized connector events
 */
enum sde_connector_events {
	SDE_CONN_EVENT_VID_DONE, /* video mode frame done */
	SDE_CONN_EVENT_CMD_DONE, /* command mode frame done */
	SDE_CONN_EVENT_VID_FIFO_OVERFLOW, /* dsi fifo overflow error */
	SDE_CONN_EVENT_CMD_FIFO_UNDERFLOW, /* dsi fifo underflow error */
	SDE_CONN_EVENT_PANEL_ID, /* returns read panel id from ddic  */
	SDE_CONN_EVENT_COUNT,
};

/**
 * struct sde_connector_evt - local event registration entry structure
 * @cb_func: Pointer to desired callback function
 * @usr: User pointer to pass to callback on event trigger
 * Returns: Zero success, negetive for failure
 */
struct sde_connector_evt {
	int (*cb_func)(uint32_t event_idx,
			uint32_t instance_idx, void *usr,
			uint32_t data0, uint32_t data1,
			uint32_t data2, uint32_t data3);
	void *usr;
};

struct sde_connector_dyn_hdr_metadata {
	u8 dynamic_hdr_payload[SDE_CONNECTOR_DHDR_MEMPOOL_MAX_SIZE];
	int dynamic_hdr_payload_size;
	bool dynamic_hdr_update;
};

/**
 * struct sde_misr_sign - defines sde misr signature structure
 * @num_valid_misr : count of valid misr signature
 * @roi_list : list of roi
 * @misr_sign_value : list of misr signature
 */
struct sde_misr_sign {
	atomic64_t num_valid_misr;
	struct msm_roi_list roi_list;
	u64 misr_sign_value[MAX_DSI_DISPLAYS];
};

/**
 * struct sde_backlight_vrr_update - smooth dimming backlight structure for vrr
 * @new_brightness : New brightness value
 * @prev_brightness : Previous brightness value
 * @curr_brightness : Current brightness value
 * @new_bl_lvl : New backlight level value
 * @prev_bl_lvl : Previous backlight level value
 * @curr_bl_lvl : Current backlight level value
 * @bl_frame_idx : Index value of dimming frame
 * @bl_increment_in_progress : Smooth dimming in progress
 * @new_incremental_bl_update : New smooth dimming update scheduled
 * @prev_bl_time_ns : Time in ns when previous BL was sent
 */
struct sde_backlight_vrr_update {
	int new_brightness;
	int prev_brightness;
	int curr_brightness;
	u32 new_bl_lvl;
	u32 prev_bl_lvl;
	u32 curr_bl_lvl;
	u32 bl_frame_idx;
	bool bl_increment_in_progress;
	bool new_incremental_bl_update;
	u64 prev_bl_time_ns;
	struct mutex bl_lock;
};

/**
 * struct sde_connector - local sde connector structure
 * @base: Base drm connector structure
 * @connector_type: Set to one of DRM_MODE_CONNECTOR_ types
 * @encoder: Pointer to preferred drm encoder
 * @panel: Pointer to drm panel, if present
 * @display: Pointer to private display data structure
 * @drv_panel: Pointer to interface driver's panel module, if present
 * @mst_port: Pointer to mst port, if present
 * @mmu_secure: MMU id for secure buffers
 * @mmu_unsecure: MMU id for unsecure buffers
 * @name: ASCII name of connector
 * @lock: Mutex lock object for this structure
 * @retire_fence: Retire fence context reference
 * @ops: Local callback function pointer table
 * @dpms_mode: DPMS property setting from user space
 * @lp_mode: LP property setting from user space
 * @last_panel_power_mode: Last consolidated dpms/lp mode setting
 * @property_info: Private structure for generic property handling
 * @property_data: Array of private data for generic property handling
 * @blob_caps: Pointer to blob structure for 'capabilities' property
 * @blob_hdr: Pointer to blob structure for 'hdr_properties' property
 * @blob_ext_hdr: Pointer to blob structure for 'ext_hdr_properties' property
 * @blob_dither: Pointer to blob structure for default dither config
 * @blob_mode_info: Pointer to blob structure for mode info
 * @blob_panel_id: Pointer to blob structure for blob_panel_id
 * @event_table: Array of registered events
 * @event_lock: Lock object for event_table
 * @bl_device: backlight device node
 * @cdev: backlight cooling device interface
 * @n: backlight cooling device notifier
 * @thermal_max_brightness: thermal max brightness cap
 * @status_work: work object to perform status checks
 * @esd_status_interval: variable to change ESD check interval in millisec
 * @expected_panel_mode: expected panel mode by usespace
 * @panel_dead: Flag to indicate if panel has gone bad
 * @esd_status_check: Flag to indicate if ESD thread is scheduled or not
 * @twm_en: Flag to indicate if TWM mode is enabled or not.
 * @bl_scale_dirty: Flag to indicate PP BL scale value(s) is changed
 * @bl_scale: BL scale value for ABA feature
 * @bl_scale_sv: BL scale value for sunlight visibility feature
 * @unset_bl_level: BL level that needs to be set later
 * @hdr_eotf: Electro optical transfer function obtained from HDR block
 * @hdr_metadata_type_one: Metadata type one obtained from HDR block
 * @hdr_max_luminance: desired max luminance obtained from HDR block
 * @hdr_avg_luminance: desired avg luminance obtained from HDR block
 * @hdr_min_luminance: desired min luminance obtained from HDR block
 * @hdr_supported: does the sink support HDR content
 * @color_enc_fmt: Colorimetry encoding formats of sink
 * @lm_mask: preferred LM mask for connector
 * @allow_bl_update: Flag to indicate if BL update is allowed currently or not
 * @dimming_bl_notify_enabled: Flag to indicate if dimming bl notify is enabled or not
 * @sde_backlight_vrr_update: Smooth dimming backlight structure for vrr
 * @qsync_mode: Cached Qsync mode, 0=disabled, 1=continuous mode
 * @qsync_updated: Qsync settings were updated
 * @ept_fps: ept fps is updated, 0 means ept_fps is disabled
 * @frame_interval: Current frame interval
 * @apply_vrr: Flag to apply vrr support once FI is set
 * @usecase_idx: Current usecase_idx
 * @freq_pattern: Current frequency pattern to be used
 * @vrr_caps: defines capabilities of vrr
 * @freq_pattern_updated: True if frequency pattern is updated
 * @freq_pattern_type_changed: True if frequency pattern type is updated
 * @vrr_cmd_state: Scenario in which VRR cmd is sent
 * @num_bl_frames: Number of frames needed for incremental dimming
 * @disable_cont_dimming: Skip smooth dimming during continuous BL updates
 * @last_vhm_cmd: Last VHM commands queued to panel
 * @colorspace_updated: Colorspace property was updated
 * @last_cmd_tx_sts: status of the last command transfer
 * @hdr_capable: external hdr support present
 * @cmd_rx_buf: the return buffer of response of command transfer
 * @rx_len: the length of dcs command received buffer
 * @cached_edid: cached edid data for the connector
 * @misr_event_notify_enabled: Flag to indicate if misr event notify is enabled or not
 * @previous_misr_sign: store previous misr signature
 * @hwfence_wb_retire_fences_enable: enable hw-fences for wb retire-fence
 * @max_mode_width: max width of all available modes
 * @shared: If a connector is sharing resource of its parent
 */
struct sde_connector {
	struct drm_connector base;

	int connector_type;

	struct drm_encoder *encoder;
	struct drm_panel *panel;
	void *display;
	void *drv_panel;
	void *mst_port;

	struct msm_gem_address_space *aspace[SDE_IOMMU_DOMAIN_MAX];

	char name[SDE_CONNECTOR_NAME_SIZE];

	struct mutex lock;
	struct sde_fence_context *retire_fence;
	struct sde_connector_ops ops;
	int dpms_mode;
	int lp_mode;
	int last_panel_power_mode;
	struct device *sysfs_dev;

	struct msm_property_info property_info;
	struct msm_property_data property_data[CONNECTOR_PROP_COUNT];
	struct drm_property_blob *blob_caps;
	struct drm_property_blob *blob_hdr;
	struct drm_property_blob *blob_ext_hdr;
	struct drm_property_blob *blob_dither;
	struct drm_property_blob *blob_mode_info;
	struct drm_property_blob *blob_panel_id;

	struct sde_connector_evt event_table[SDE_CONN_EVENT_COUNT];
	spinlock_t event_lock;

	struct backlight_device *bl_device;
	struct sde_cdev *cdev;
	struct notifier_block n;
	unsigned long thermal_max_brightness;
	struct delayed_work status_work;
	u32 esd_status_interval;
	bool panel_dead;
	bool esd_status_check;
	bool twm_en;
	enum panel_op_mode expected_panel_mode;

	bool bl_scale_dirty;
	u32 bl_scale;
	u32 bl_scale_sv;
	u32 unset_bl_level;
	bool allow_bl_update;
	bool dimming_bl_notify_enabled;
	struct sde_backlight_vrr_update bl_vrr;

	u32 hdr_eotf;
	bool hdr_metadata_type_one;
	u32 hdr_max_luminance;
	u32 hdr_avg_luminance;
	u32 hdr_min_luminance;
	bool hdr_supported;

	u32 color_enc_fmt;
	u32 lm_mask;

	u8 hdr_plus_app_ver;
	u32 qsync_mode;
	bool qsync_updated;
	u32 ept_fps;

	u32 frame_interval;
	u32 apply_vrr;
	u32 usecase_idx;
	struct msm_freq_step_pattern *freq_pattern;
	struct msm_vrr_capabilities vrr_caps;
	bool freq_pattern_updated;
	bool freq_pattern_type_changed;
	enum sde_conn_vrr_cmd_state vrr_cmd_state;
	u32 num_bl_frames;
	bool disable_cont_dimming;
	u64 last_vhm_cmd;

	bool colorspace_updated;

	bool last_cmd_tx_sts;
	bool hdr_capable;

	u8 cmd_rx_buf[MAX_CMD_RECEIVE_SIZE];
	int rx_len;

	struct edid *cached_edid;
	bool misr_event_notify_enabled;
	struct sde_misr_sign previous_misr_sign;

	bool hwfence_wb_retire_fences_enable;

	u32 max_mode_width;
	bool shared;

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	struct delayed_work dyn_clk_work;
	u32 scheduled_dyn_clk_rate;
	u32 pre_kickoff_progress;
#endif
};

/**
 * to_sde_connector - convert drm_connector pointer to sde connector pointer
 * @X: Pointer to drm_connector structure
 * Returns: Pointer to sde_connector structure
 */
#define to_sde_connector(x)     container_of((x), struct sde_connector, base)

/**
 * sde_connector_get_display - get sde connector's private display pointer
 * @C: Pointer to drm connector structure
 * Returns: Pointer to associated private display structure
 */
#define sde_connector_get_display(C) \
	((C) ? to_sde_connector((C))->display : NULL)

/**
 * sde_connector_get_encoder - get sde connector's private encoder pointer
 * @C: Pointer to drm connector structure
 * Returns: Pointer to associated private encoder structure
 */
#define sde_connector_get_encoder(C) \
	((C) ? to_sde_connector((C))->encoder : NULL)

/**
 * sde_connector_qsync_updated - indicates if connector updated qsync
 * @C: Pointer to drm connector structure
 * Returns: True if qsync is updated; false otherwise
 */
#define sde_connector_is_qsync_updated(C) \
	((C) ? to_sde_connector((C))->qsync_updated : 0)

/**
 * sde_connector_get_qsync_mode - get sde connector's qsync_mode
 * @C: Pointer to drm connector structure
 * Returns: Current cached qsync_mode for given connector
 */
#define sde_connector_get_qsync_mode(C) \
	((C) ? to_sde_connector((C))->qsync_mode : 0)

/**
 * sde_connector_get_propinfo - get sde connector's property info pointer
 * @C: Pointer to drm connector structure
 * Returns: Pointer to associated private property info structure
 */
#define sde_connector_get_propinfo(C) \
	((C) ? &to_sde_connector((C))->property_info : NULL)

/**
 * struct sde_connector_state - private connector status structure
 * @base: Base drm connector structure
 * @out_fb: Pointer to output frame buffer, if applicable
 * @property_state: Local storage for msm_prop properties
 * @property_values: Local cache of current connector property values
 * @rois: Regions of interest structure for mapping CRTC to Connector output
 * @property_blobs: blob properties
 * @mode_info: local copy of msm_mode_info struct
 * @hdr_meta: HDR metadata info passed from userspace
 * @dyn_hdr_meta: Dynamic HDR metadata payload and state tracking
 * @msm_mode: struct containing drm_mode and downstream private variables
 * @old_topology_name: topology of previous atomic state. remove this in later
 *	kernel versions which provide drm_atomic_state old_state pointers
 * @cont_splash_populated: State was populated as part of cont. splash
 * @dnsc_blur_count: Number of downscale blur blocks used
 * @dnsc_blur_cfg: Configs for the downscale blur block
 * @dnsc_blur_lut: LUT idx used for the Gaussian filter LUTs in downscale blur block
 * @usage_type: WB connector usage type
 */
struct sde_connector_state {
	struct drm_connector_state base;
	struct drm_framebuffer *out_fb;
	struct msm_property_state property_state;
	struct msm_property_value property_values[CONNECTOR_PROP_COUNT];

	struct msm_roi_list rois;
	struct drm_property_blob *property_blobs[CONNECTOR_PROP_BLOBCOUNT];
	struct msm_mode_info mode_info;
	struct drm_msm_ext_hdr_metadata hdr_meta;
	struct sde_connector_dyn_hdr_metadata dyn_hdr_meta;
	struct msm_display_mode msm_mode;
	enum sde_rm_topology_name old_topology_name;

	bool cont_splash_populated;

	u32 dnsc_blur_count;
	struct sde_drm_dnsc_blur_cfg dnsc_blur_cfg[DNSC_BLUR_MAX_COUNT];
	u32 dnsc_blur_lut;
	enum sde_wb_usage_type usage_type;
};

/**
 * to_sde_connector_state - convert drm_connector_state pointer to
 *                          sde connector state pointer
 * @X: Pointer to drm_connector_state structure
 * Returns: Pointer to sde_connector_state structure
 */
#define to_sde_connector_state(x) \
	container_of((x), struct sde_connector_state, base)

/**
 * sde_connector_get_property - query integer value of connector property
 * @S: Pointer to drm connector state
 * @X: Property index, from enum msm_mdp_connector_property
 * Returns: Integer value of requested property
 */
#define sde_connector_get_property(S, X) \
	((S) && ((X) < CONNECTOR_PROP_COUNT) ? \
	 (to_sde_connector_state((S))->property_values[(X)].value) : 0)

/**
 * sde_connector_get_property_state - retrieve property state cache
 * @S: Pointer to drm connector state
 * Returns: Pointer to local property state structure
 */
#define sde_connector_get_property_state(S) \
	((S) ? (&to_sde_connector_state((S))->property_state) : NULL)

/**
 * sde_connector_get_out_fb - query out_fb value from sde connector state
 * @S: Pointer to drm connector state
 * Returns: Output fb associated with specified connector state
 */
#define sde_connector_get_out_fb(S) \
	((S) ? to_sde_connector_state((S))->out_fb : 0)

/**
 * sde_connector_get_kms - helper to get sde_kms from connector
 * @conn: Pointer to drm connector
 * Returns: Pointer to sde_kms or NULL
 */
static inline struct sde_kms *sde_connector_get_kms(struct drm_connector *conn)
{
	struct msm_drm_private *priv;

	if (!conn || !conn->dev || !conn->dev->dev_private) {
		SDE_ERROR("invalid connector\n");
		return NULL;
	}

	priv = conn->dev->dev_private;
	if (!priv->kms) {
		SDE_ERROR("invalid kms\n");
		return NULL;
	}

	return to_sde_kms(priv->kms);
}

/**
 * sde_connector_get_topology_name - helper accessor to retrieve topology_name
 * @connector: pointer to drm connector
 * Returns: value of the CONNECTOR_PROP_TOPOLOGY_NAME property or 0
 */
static inline uint64_t sde_connector_get_topology_name(
		struct drm_connector *connector)
{
	if (!connector || !connector->state)
		return 0;
	return sde_connector_get_property(connector->state,
			CONNECTOR_PROP_TOPOLOGY_NAME);
}

/**
 * sde_connector_get_old_topology_name - helper accessor to retrieve
 *	topology_name for the previous mode
 * @connector: pointer to drm connector state
 * Returns: cached value of the previous topology, or SDE_RM_TOPOLOGY_NONE
 */
static inline enum sde_rm_topology_name sde_connector_get_old_topology_name(
		struct drm_connector_state *state)
{
	struct sde_connector_state *c_state = to_sde_connector_state(state);

	if (!state)
		return SDE_RM_TOPOLOGY_NONE;

	return c_state->old_topology_name;
}

/**
 * sde_connector_panel_dead - check if panel is dead
 * @conn: pointer to drm connector
 * Returns: bool indicating whether or not panel is dead based on connector
 */
static inline bool sde_connector_panel_dead(struct drm_connector *conn)
{
	struct sde_connector *sde_conn = to_sde_connector(conn);

	if (!sde_conn)
		return true;

	return sde_conn->panel_dead;
}

/**
 * sde_connector_set_old_topology_name - helper to cache value of previous
 *	mode's topology
 * @connector: pointer to drm connector state
 * Returns: 0 on success, negative errno on failure
 */
static inline int sde_connector_set_old_topology_name(
		struct drm_connector_state *state,
		enum sde_rm_topology_name top)
{
	struct sde_connector_state *c_state = to_sde_connector_state(state);

	if (!state)
		return -EINVAL;

	c_state->old_topology_name = top;

	return 0;
}

/**
 * sde_connector_get_lp - helper accessor to retrieve LP state
 * @connector: pointer to drm connector
 * Returns: value of the CONNECTOR_PROP_LP property or 0
 */
static inline uint64_t sde_connector_get_lp(
		struct drm_connector *connector)
{
	if (!connector || !connector->state)
		return 0;
	return sde_connector_get_property(connector->state,
			CONNECTOR_PROP_LP);
}

/**
 * sde_connector_get_dnsc_blur_io_res - populates the downscale blur src/dst w/h
 * @state: pointer to drm connector state
 * @res: pointer to the output struct to populate the src/dst
 */
static inline void sde_connector_get_dnsc_blur_io_res(struct drm_connector_state *state,
		struct sde_io_res *res)
{
	struct sde_connector_state *sde_conn_state;
	int i;

	if (!state || !res)
		return;

	memset(res, 0, sizeof(struct sde_io_res));

	sde_conn_state = to_sde_connector_state(state);
	if (!sde_conn_state->dnsc_blur_count ||
			!(sde_conn_state->dnsc_blur_cfg[0].flags & DNSC_BLUR_EN))
		return;

	res->enabled = true;
	for (i = 0; i < sde_conn_state->dnsc_blur_count; i++) {
		res->src_w += sde_conn_state->dnsc_blur_cfg[i].src_width;
		res->dst_w += sde_conn_state->dnsc_blur_cfg[i].dst_width;
	}
	res->src_h = sde_conn_state->dnsc_blur_cfg[0].src_height;
	res->dst_h = sde_conn_state->dnsc_blur_cfg[0].dst_height;
}

/**
 * sde_connector_set_property_for_commit - add property set to atomic state
 *	Add a connector state property update for the specified property index
 *	to the atomic state in preparation for a drm_atomic_commit.
 * @connector: Pointer to drm connector
 * @atomic_state: Pointer to DRM atomic state structure for commit
 * @property_idx: Connector property index
 * @value: Updated property value
 * Returns: Zero on success
 */
int sde_connector_set_property_for_commit(struct drm_connector *connector,
		struct drm_atomic_state *atomic_state,
		uint32_t property_idx, uint64_t value);

/**
 * sde_connector_post_init - update connector object with post
 * initialization.
 * It can update the debugfs, sysfs, entries
 * @dev: Pointer to drm device struct
 * @conn: Pointer to drm connector
 * Returns: Zero on success
 */
int sde_connector_post_init(struct drm_device *dev, struct drm_connector *conn);

/**
 * sde_connector_init - create drm connector object for a given display
 * @dev: Pointer to drm device struct
 * @encoder: Pointer to associated encoder
 * @panel: Pointer to associated panel, can be NULL
 * @display: Pointer to associated display object
 * @ops: Pointer to callback operations function table
 * @connector_poll: Set to appropriate DRM_CONNECTOR_POLL_ setting
 * @connector_type: Set to appropriate DRM_MODE_CONNECTOR_ type
 * @shared: Flag to identify if a connector is sharing resource of its parent in SHD
 * Returns: Pointer to newly created drm connector struct
 */
struct drm_connector *sde_connector_init(struct drm_device *dev,
		struct drm_encoder *encoder,
		struct drm_panel *panel,
		void *display,
		const struct sde_connector_ops *ops,
		int connector_poll,
		int connector_type, bool shared);

/**
 * sde_connector_fence_error_ctx_signal - sde fence error context update for retire fence
 * @conn: Pointer to drm connector object
 * @input_fence_status: input fence status, negative if input fence error
 * @is_vid：if encoder is video mode
 */
void sde_connector_fence_error_ctx_signal(struct drm_connector *conn, int input_fence_status,
		bool is_vid);

/**
 * sde_connector_prepare_fence - prepare fence support for current commit
 * @connector: Pointer to drm connector object
 */
void sde_connector_prepare_fence(struct drm_connector *connector);

/**
 * sde_connector_complete_commit - signal completion of current commit
 * @connector: Pointer to drm connector object
 * @ts: timestamp to be updated in the fence signalling
 * @fence_event: enum value to indicate nature of fence event
 */
void sde_connector_complete_commit(struct drm_connector *connector,
		ktime_t ts, enum sde_fence_event fence_event);

/**
 * sde_connector_commit_reset - reset the completion signal
 * @connector: Pointer to drm connector object
 * @ts: timestamp to be updated in the fence signalling
 */
void sde_connector_commit_reset(struct drm_connector *connector, ktime_t ts);

/**
 * sde_connector_get_info - query display specific information
 * @connector: Pointer to drm connector object
 * @info: Pointer to msm display information structure
 * Returns: Zero on success
 */
int sde_connector_get_info(struct drm_connector *connector,
		struct msm_display_info *info);

/**
 * sde_connector_clk_ctrl - enables/disables the connector clks
 * @connector: Pointer to drm connector object
 * @enable: true/false to enable/disable
 * @idle_pc: Idle power collapse status
 * Returns: Zero on success
 */
int sde_connector_clk_ctrl(struct drm_connector *connector, bool enable, bool idle_pc);

/**
 * sde_connector_esync_clk_ctrl - enables/disables the esync clk
 * @connector: Pointer to drm connector object
 * @enable: true/false to enable/disable
 * Returns: Zero on success
 */
int sde_connector_esync_clk_ctrl(struct drm_connector *connector, bool enable);

/**
 * sde_connector_osc_clk_ctrl - enables/disables the oscillator clk
 * @connector: Pointer to drm connector object
 * @enable: true/false to enable/disable
 * Returns: Zero on success
 */
int sde_connector_osc_clk_ctrl(struct drm_connector *connector, bool enable);

/**
 * sde_connector_clk_get_rate_esync - retrieves esync clk rate
 * @connector: Pointer to drm connector object
 * @intf_idx: index of interface whose esync clk rate is requested
 * @rate: pointer at which the rate will be written on success
 * Returns: Zero on success
 */
int sde_connector_clk_get_rate_esync(struct drm_connector *connector,
		enum sde_intf intf_idx, u64 *rate);

/**
 * sde_connector_get_dpms - query dpms setting
 * @connector: Pointer to drm connector structure
 * Returns: Current DPMS setting for connector
 */
int sde_connector_get_dpms(struct drm_connector *connector);

/**
 * sde_connector_set_qsync_params - set status of qsync_updated for current
 *                                  frame and update the cached qsync_mode
 * @connector: pointer to drm connector
 *
 * This must be called after the connector set_property values are applied,
 * and before sde_connector's qsync_updated or qsync_mode fields are accessed.
 * It must only be called once per frame update for the given connector.
 */
void sde_connector_set_qsync_params(struct drm_connector *connector);

/**
 * sde_connector_set_vrr_params - set status of vrr params
 * @connector: pointer to drm connector
 */
void sde_connector_set_vrr_params(struct drm_connector *connector);

/**
 * sde_connector_trigger_cmd_self_refresh - set status of vrr params
 * @connector: pointer to drm connector
 */
int sde_connector_trigger_cmd_self_refresh(struct drm_connector *connector);

/**
 * sde_connector_trigger_cmd_backlight_update - update backlight
 * @connector: pointer to drm connector
 */
int sde_connector_trigger_cmd_backlight_update(struct drm_connector *connector);
int sde_connector_trigger_cmd_backlight_sr(struct drm_connector *connector);

/**
 * sde_connector_complete_qsync_commit - callback signalling completion
 *			of qsync, if modified for the current commit
 * @conn   - Pointer to drm connector object
 * @params - Parameter bundle of connector-stored information for
 *	post kickoff programming into the display
 */
void sde_connector_complete_qsync_commit(struct drm_connector *conn,
			struct msm_display_conn_params *params);

/**
* sde_connector_get_dyn_hdr_meta - returns pointer to connector state's dynamic
*				   HDR metadata info
* @connector: pointer to drm connector
*/

struct sde_connector_dyn_hdr_metadata *sde_connector_get_dyn_hdr_meta(
		struct drm_connector *connector);

/**
 * sde_connector_trigger_event - indicate that an event has occurred
 *	Any callbacks that have been registered against this event will
 *	be called from the same thread context.
 * @connector: Pointer to drm connector structure
 * @event_idx: Index of event to trigger
 * @instance_idx: Event-specific "instance index" to pass to callback
 * @data0: Event-specific "data" to pass to callback
 * @data1: Event-specific "data" to pass to callback
 * @data2: Event-specific "data" to pass to callback
 * @data3: Event-specific "data" to pass to callback
 * Returns: Zero on success
 */
int sde_connector_trigger_event(void *drm_connector,
		uint32_t event_idx, uint32_t instance_idx,
		uint32_t data0, uint32_t data1,
		uint32_t data2, uint32_t data3);

/**
 * sde_connector_register_event - register a callback function for an event
 * @connector: Pointer to drm connector structure
 * @event_idx: Index of event to register
 * @cb_func: Pointer to desired callback function
 * @usr: User pointer to pass to callback on event trigger
 * Returns: Zero on success
 */
int sde_connector_register_event(struct drm_connector *connector,
		uint32_t event_idx,
		int (*cb_func)(uint32_t event_idx,
			uint32_t instance_idx, void *usr,
			uint32_t data0, uint32_t data1,
			uint32_t data2, uint32_t data3),
		void *usr);

/**
 * sde_connector_unregister_event - unregister all callbacks for an event
 * @connector: Pointer to drm connector structure
 * @event_idx: Index of event to register
 */
void sde_connector_unregister_event(struct drm_connector *connector,
		uint32_t event_idx);

/**
 * sde_connector_register_custom_event - register for async events
 * @kms: Pointer to sde_kms
 * @conn_drm: Pointer to drm connector object
 * @event: Event for which request is being sent
 * @en: Flag to enable/disable the event
 * Returns: Zero on success
 */
int sde_connector_register_custom_event(struct sde_kms *kms,
		struct drm_connector *conn_drm, u32 event, bool en);

/**
 * sde_connector_pre_kickoff - trigger kickoff time feature programming
 * @connector: Pointer to drm connector object
 * Returns: Zero on success
 */
int sde_connector_pre_kickoff(struct drm_connector *connector);

/**
 * sde_connector_prepare_commit - trigger commit time feature programming
 * @connector: Pointer to drm connector object
 * Returns: Zero on success
 */
int sde_connector_prepare_commit(struct drm_connector *connector);

/**
 * sde_connector_needs_offset - adjust the output fence offset based on
 *                              display type
 * @connector: Pointer to drm connector object
 * Returns: true if offset is required, false for all other cases.
 */
static inline bool sde_connector_needs_offset(struct drm_connector *connector)
{
	struct sde_connector *c_conn;

	if (!connector)
		return false;

	c_conn = to_sde_connector(connector);
	return (c_conn->connector_type != DRM_MODE_CONNECTOR_VIRTUAL);
}

/**
 * sde_connector_get_dither_cfg - get dither property data
 * @conn: Pointer to drm_connector struct
 * @state: Pointer to drm_connector_state struct
 * @cfg: Pointer to pointer to dither cfg
 * @len: length of the dither data
 * @idle_pc: flag to indicate idle_pc_restore happened
 * Returns: Zero on success
 */
int sde_connector_get_dither_cfg(struct drm_connector *conn,
		struct drm_connector_state *state, void **cfg,
		size_t *len, bool idle_pc);

/**
 * sde_connector_set_blob_data - set connector blob property data
 * @conn: Pointer to drm_connector struct
 * @state: Pointer to the drm_connector_state struct
 * @prop_id: property id to be populated
 * Returns: Zero on success
 */
int sde_connector_set_blob_data(struct drm_connector *conn,
		struct drm_connector_state *state,
		enum msm_mdp_conn_property prop_id);

/**
 * sde_connector_roi_v1_check_roi - validate connector ROI
 * @conn_state: Pointer to drm_connector_state struct
 * Returns: Zero on success
 */
int sde_connector_roi_v1_check_roi(struct drm_connector_state *conn_state);

/**
 * sde_connector_set_dyn_bit_clk - set dynamic bit clock
 * @conn: Pointer to drm_connector struct
 * @value: Property value
 * Returns: Zero on success
 */
int sde_connector_set_dyn_bit_clk(struct drm_connector *conn, uint64_t value);

/**
 * sde_connector_schedule_status_work - manage ESD thread
 * conn: Pointer to drm_connector struct
 * @en: flag to start/stop ESD thread
 */
void sde_connector_schedule_status_work(struct drm_connector *conn, bool en);

/**
 * sde_connector_helper_reset_properties - reset properties to default values in
 *	the given DRM connector state object
 * @connector: Pointer to DRM connector object
 * @connector_state: Pointer to DRM connector state object
 * Returns: 0 on success, negative errno on failure
 */
int sde_connector_helper_reset_custom_properties(
		struct drm_connector *connector,
		struct drm_connector_state *connector_state);

/**
 * sde_connector_state_get_mode_info - get information of the current mode
 *                               in the given connector state.
 * conn_state: Pointer to the DRM connector state object
 * mode_info: Pointer to the mode info structure
 */
int sde_connector_state_get_mode_info(struct drm_connector_state *conn_state,
	struct msm_mode_info *mode_info);

/**
 * sde_connector_get_lm_cnt_from_topology - retrieves the topology info
 *	from the panel mode and returns the lm count.
 * conn: Pointer to DRM connector object
 * drm_mode: Pointer to the drm mode structure
 */
int sde_connector_get_lm_cnt_from_topology(struct drm_connector *conn,
	 const struct drm_display_mode *drm_mode);

/**
 * sde_conn_get_max_mode_width - retrieves the maximum width from all modes
 * conn: Pointer to DRM connector object
 */
static inline u32 sde_conn_get_max_mode_width(struct drm_connector *conn)
{
	u32 maxw = 0;
	struct drm_display_mode *mode;

	if (!conn)
		return maxw;

	list_for_each_entry(mode, &conn->modes, head)
		maxw = maxw > mode->hdisplay ? maxw : mode->hdisplay;

	return maxw;
}

/**
 * sde_connector_state_get_topology - get topology from given connector state
 * conn_state: Pointer to the DRM connector state object
 * topology: Pointer to store topology info of the display
 */
static inline int sde_connector_state_get_topology(
		struct drm_connector_state *conn_state,
		struct msm_display_topology *topology)
{
	struct sde_connector_state *sde_conn_state = NULL;

	if (!conn_state || !topology) {
		SDE_ERROR("invalid arguments conn_state %d, topology %d\n",
				!conn_state, !topology);
		return -EINVAL;
	}

	sde_conn_state = to_sde_connector_state(conn_state);
	memcpy(topology, &sde_conn_state->mode_info.topology,
		sizeof(struct msm_display_topology));
	return 0;
}

/**
 * sde_connector_state_get_compression_info- get compression info of display
 * from given connector state
 * conn_state: Pointer to the DRM connector state object
 * comp_info: Pointer to the compression info structure
 */
static inline int sde_connector_state_get_compression_info(
		struct drm_connector_state *conn_state,
		struct msm_compression_info *comp_info)
{
	struct sde_connector_state *sde_conn_state = NULL;

	if (!conn_state || !comp_info) {
		SDE_ERROR("invalid arguments\n");
		return -EINVAL;
	}

	sde_conn_state = to_sde_connector_state(conn_state);
	memcpy(comp_info, &sde_conn_state->mode_info.comp_info,
		sizeof(struct msm_compression_info));
	return 0;
}

static inline bool sde_connector_is_quadpipe_3d_merge_enabled(
		struct drm_connector_state *conn_state)
{
	enum sde_rm_topology_name topology;

	if (!conn_state)
		return false;

	topology = sde_connector_get_property(conn_state, CONNECTOR_PROP_TOPOLOGY_NAME);
	if ((topology == SDE_RM_TOPOLOGY_QUADPIPE_3DMERGE)
			|| (topology == SDE_RM_TOPOLOGY_QUADPIPE_3DMERGE_DSC))
		return true;

	return false;
}

static inline bool sde_connector_is_dualpipe_3d_merge_enabled(
		struct drm_connector_state *conn_state)
{
	enum sde_rm_topology_name topology;

	if (!conn_state)
		return false;

	topology = sde_connector_get_property(conn_state, CONNECTOR_PROP_TOPOLOGY_NAME);
	if ((topology == SDE_RM_TOPOLOGY_DUALPIPE_3DMERGE)
			|| (topology == SDE_RM_TOPOLOGY_DUALPIPE_3DMERGE_DSC)
			|| (topology == SDE_RM_TOPOLOGY_DUALPIPE_3DMERGE_VDC))
		return true;

	return false;
}

static inline bool sde_connector_is_3d_merge_enabled(struct drm_connector_state *conn_state)
{
	return sde_connector_is_dualpipe_3d_merge_enabled(conn_state)
		|| sde_connector_is_quadpipe_3d_merge_enabled(conn_state);
}

/**
* sde_connector_set_msm_mode - set msm_mode for connector state
* @conn_state: Pointer to drm connector state structure
* @adj_mode: Pointer to adjusted display mode for display
* Returns: Zero on success
*/
int sde_connector_set_msm_mode(struct drm_connector_state *conn_state,
				struct drm_display_mode *adj_mode);

/**
* sde_connector_get_mode_info - retrieve mode info for given mode
* @connector: Pointer to drm connector structure
* @drm_mode: Display mode set for the display
* @sub_mode: Additional mode info to drm display mode
* @mode_info: Out parameter. information of the display mode
* Returns: Zero on success
*/
int sde_connector_get_mode_info(struct drm_connector *conn,
		const struct drm_display_mode *drm_mode,
		struct msm_sub_mode *sub_mode,
		struct msm_mode_info *mode_info);

/**
 * sde_conn_timeline_status - current buffer timeline status
 * conn: Pointer to drm_connector struct
 */
void sde_conn_timeline_status(struct drm_connector *conn);

/**
 * sde_connector_helper_bridge_disable - helper function for drm bridge disable
 * @connector: Pointer to DRM connector object
 */
void sde_connector_helper_bridge_disable(struct drm_connector *connector);

/**
 * sde_connector_helper_bridge_post_disable - helper function for drm bridge post disable
 * @connector: Pointer to DRM connector object
 */
void sde_connector_helper_bridge_post_disable(struct drm_connector *connector);

/**
 * sde_connector_destroy - destroy drm connector object
 * @connector: Pointer to DRM connector object
 */
void sde_connector_destroy(struct drm_connector *connector);

/**
 * sde_connector_event_notify - signal hw recovery event to client
 * @connector: pointer to connector
 * @type:     event type
 * @len:     length of the value of the event
 * @val:     value
 */
int sde_connector_event_notify(struct drm_connector *connector, uint32_t type,
		uint32_t len, uint32_t val);
/**
 * sde_connector_helper_bridge_enable - helper function for drm bridge enable
 * @connector: Pointer to DRM connector object
 */
void sde_connector_helper_bridge_enable(struct drm_connector *connector);

/**
 * sde_connector_get_panel_vfp - helper to get panel vfp
 * @connector: pointer to drm connector
 * @h_active: panel width
 * @v_active: panel heigth
 * Returns: v_front_porch on success error-code on failure
 */
int sde_connector_get_panel_vfp(struct drm_connector *connector,
	struct drm_display_mode *mode);
/**
 * sde_connector_esd_status - helper function to check te status
 * @connector: Pointer to DRM connector object
 */
int sde_connector_esd_status(struct drm_connector *connector);

const char *sde_conn_get_topology_name(struct drm_connector *conn,
		struct msm_display_topology topology);

/*
 * sde_connector_is_line_insertion_supported - get line insertion
 * feature bit value from panel
 * @sde_conn:    Pointer to sde connector structure
 * @Return: line insertion support status
 */
bool sde_connector_is_line_insertion_supported(struct sde_connector *sde_conn);

/**
 * _sde_connector_get_display - get dsi display according to connector
 * @c_conn: Pointer to sde connector struct
 * @Return: pointer to dsi display
 */
struct dsi_display *_sde_connector_get_display(struct sde_connector *c_conn);

/**
 * sde_connector_update_cmd - send the commands listed in cmd_bit_mask
 * @connector: Pointer to sde connector struct
 * @cmd_bit_mask: Bit mask of commands to be sent
 * @peripheral_flush: True if command needs to sent at flush
 */
int sde_connector_update_cmd(struct drm_connector *connector,
		u64 cmd_bit_mask, bool peripheral_flush);

void sde_connector_set_private_flag(struct sde_connector_state *conn_state, u32 flag, bool set);

#endif /* _SDE_CONNECTOR_H_ */
