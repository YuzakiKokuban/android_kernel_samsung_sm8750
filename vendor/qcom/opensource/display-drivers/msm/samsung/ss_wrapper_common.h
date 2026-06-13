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
Copyright (C) 2023, Samsung Electronics. All rights reserved.

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

#ifndef SS_WRAPPER_H
#define SS_WRAPPER_H

#include "ss_dsi_panel_common.h"

#define MAX_BL_PF_LEVEL_COMMON 255

int ss_wait_for_pm_resume(struct samsung_display_driver_data *vdd);

int ss_gpio_set_value(struct samsung_display_driver_data *vdd, unsigned int gpio, int value);
int ss_gpio_get_value(struct samsung_display_driver_data *vdd, unsigned int gpio);

int ss_dsi_display_suspend(struct device *dev);

int ss_sde_vm_owns_hw(struct samsung_display_driver_data *vdd);
void ss_sde_vm_lock(struct samsung_display_driver_data *vdd, bool lock);
bool ss_gpio_is_valid(int number);
int ss_gpio_to_irq(unsigned gpio);
int ss_wrapper_dsi_display_clk_ctrl(struct samsung_display_driver_data *vdd, int enable);
int ss_wrapper_dsi_display_lock(struct samsung_display_driver_data *vdd, bool enable);
int ss_wrapper_trigger_self_refresh(struct samsung_display_driver_data *vdd);
int ss_wrapper_dsi_ctrl_setup_avr(struct samsung_display_driver_data *vdd, bool enable);
int ss_wrapper_sde_encoder_enter_idle(struct samsung_display_driver_data *vdd, int enable);
int ss_wrapper_dsi_panel_tx_cmd_set(struct dsi_panel *panel, int type);
int ss_wrapper_regulator_get_voltage(struct samsung_display_driver_data *vdd, struct regulator *regulator);
int ss_wrapper_regulator_set_voltage(struct samsung_display_driver_data *vdd, struct regulator *regulator,
		int min_uV, int max_uV);
int ss_wrapper_regulator_set_short_detection(struct samsung_display_driver_data *vdd,
		struct regulator *regulator, bool enable, int lv_uA);
int ss_wrapper_misc_register(struct samsung_display_driver_data *vdd, struct miscdevice *misc);
int ss_wrapper_i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);

struct ss_cmd_set;

int ss_wrapper_parse_cmd_sets(struct samsung_display_driver_data *vdd,
				struct ss_cmd_set *set, int type,
				struct device_node *np, const char *buf, char *cmd_name, bool from_node);
int __ss_parse_ss_cmd_sets_revision(struct samsung_display_driver_data *vdd,
					struct ss_cmd_set *set,
					int type, struct device_node *np, const char *buf,
					char *cmd_name, bool from_node);

char *ss_wrapper_parse_symbol(struct samsung_display_driver_data *vdd,
				const struct device_node *np,
				const char *data, u32 *length, char *cmd_name);

int ss_wrapper_parse_cmd_sets_sysfs(struct samsung_display_driver_data *vdd,
					const char *buf, u32 length, const char *cmd_name);
u8 *ss_wrapper_of_get_property(const struct device_node *np, const char *buf,
					const char *name, int *lenp, bool from_node);
int ss_wrapper_of_get_named_gpio(struct device_node *np,
					const char *name, int index);
struct ss_cmd_set *ss_get_ss_cmds(struct samsung_display_driver_data *vdd, int type);
int ss_is_prop_string_style(struct device_node *np, const char *buf, char *cmd_name, bool from_node);
int ss_print_rx_buf(struct samsung_display_driver_data *vdd, int type);
int ss_get_rx_buf_addr(struct samsung_display_driver_data *vdd, int type,
			u8 addr, int cmd_order, u8 *out_buf, int out_len);
int ss_get_rx_buf(struct samsung_display_driver_data *vdd, int type,
			int cmd_order, u8 *out_buf);
int ss_get_all_rx_buf(struct samsung_display_driver_data *vdd, int type,
			u8 *out_buf);
int ss_send_cmd_get_rx(struct samsung_display_driver_data *vdd, int type,
				u8 *out_buf);
int ss_wrapper_read_from_file(struct samsung_display_driver_data *vdd, bool force_read);

struct ss_cmd_desc;
bool is_ss_cmd_op_skip(struct samsung_display_driver_data *vdd,
			struct ss_cmd_desc *cmd);
int register_op_sym_cb(struct samsung_display_driver_data *vdd,
			char *symbol,
			int (*cb)(struct samsung_display_driver_data *,
					char *, struct ss_cmd_desc *),
			bool op_update);
int register_common_op_sym_cb(struct samsung_display_driver_data *vdd);
u8 get_otp(struct samsung_display_driver_data *vdd, u8 addr, u16 offset);
int save_otp(struct samsung_display_driver_data *vdd);
int ss_prepare_otp(struct samsung_display_driver_data *vdd);

int ss_bridge_qc_cmd_alloc(struct samsung_display_driver_data *vdd,
				struct ss_cmd_set *ss_set, bool no_gpara);
int ss_bridge_qc_cmd_update(struct samsung_display_driver_data *vdd,
			struct ss_cmd_set *ss_set,
			bool no_gpara);

#endif /* SS_WRAPPER_H */
