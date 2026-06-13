/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * DDI operation : self clock, self mask, self icon.. etc.
 * Author: QC LCD driver <cj1225.jang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ss_dsi_panel_common.h"
#include "self_display.h"

int make_mass_self_display_img_cmds(struct samsung_display_driver_data *vdd,
		enum self_display_data_flag op, struct ss_cmd_desc *ss_cmd)
{
	char *data = vdd->self_disp.operation[op].img_buf;
	u32 data_size = vdd->self_disp.operation[op].img_size;
	int ret = 0;

	u32 check_sum_0 = 0;
	u32 check_sum_1 = 0;
	u32 check_sum_2 = 0;
	u32 check_sum_3 = 0;
	int i;

	if (!data) {
		LCD_ERR(vdd, "data is null..\n");
		return -EINVAL;
	}

	if (!data_size) {
		LCD_ERR(vdd, "data size is zero..\n");
		return -EINVAL;
	}

	SDE_ATRACE_BEGIN("mass_cmd_generation");
	ret = ss_convert_img_to_mass_cmds(vdd, data, data_size, ss_cmd);

	/* Image Check Sum Calculation */
	for (i = 0; i < data_size; i += 4)
		check_sum_0 += data[i];

	for (i = 1; i < data_size; i += 4)
		check_sum_1 += data[i];

	for (i = 2; i < data_size; i += 4)
		check_sum_2 += data[i];

	for (i = 3; i < data_size; i += 4)
		check_sum_3 += data[i];

	vdd->self_disp.operation[op].img_checksum_cal = (check_sum_3 & 0xFF);
	vdd->self_disp.operation[op].img_checksum_cal |= ((check_sum_2 & 0xFF) << 8);
	vdd->self_disp.operation[op].img_checksum_cal |= ((check_sum_1 & 0xFF) << 16);
	vdd->self_disp.operation[op].img_checksum_cal |= ((check_sum_0 & 0xFF) << 24);

	SDE_ATRACE_END("mass_cmd_generation");

	LCD_INFO(vdd, "[checksum] op=%d, checksum: 0x%X, 0x%X, 0x%X, 0x%X\n",
			op, check_sum_0, check_sum_1, check_sum_2, check_sum_3);

	return ret;
}

/* TODO: set last_cmd for cmd[cur-1] and cmd[cur+1] */
static int update_self_mask_image(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct ss_cmd_desc *src_ss_cmd;
	struct dsi_cmd_desc *tmp_qc_cmd;
	int ret;

	cmd->skip_by_cond = false;

	if (!vdd->self_disp.is_support) {
		LCD_INFO(vdd, "self_disp is not enabled\n");
		ret = -EPERM;
		goto err_skip;
	}

	src_ss_cmd = &vdd->self_disp.self_mask_cmd;

	/* update target ss cmd except qc_cmd pointer */
	tmp_qc_cmd = cmd->qc_cmd;
	cmd = src_ss_cmd;
	cmd->qc_cmd = tmp_qc_cmd;

	/* update target qc cmd except ss_cmd pointer */
	cmd->qc_cmd = src_ss_cmd->qc_cmd;
	cmd->qc_cmd->ss_cmd = cmd;

	/* data pass of ss_bridge_qc_cmd_update */
	cmd->qc_cmd->msg.tx_buf = vdd->self_disp.self_mask_cmd.txbuf;
	cmd->qc_cmd->msg.tx_len = vdd->self_disp.self_mask_cmd.tx_len;

	LCD_INFO(vdd, "cmd->tx_len:%lu, tx_buf:[0x%x, 0x%x..\n",
		cmd->qc_cmd->msg.tx_len, vdd->self_disp.self_mask_cmd.txbuf[0],
		vdd->self_disp.self_mask_cmd.txbuf[1]);

	return 0;

err_skip:
	cmd->skip_by_cond = true;
	return ret;
}

static int update_self_mask_crc_image(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct ss_cmd_desc *src_ss_cmd;
	struct dsi_cmd_desc *tmp_qc_cmd;
	int ret;

	cmd->skip_by_cond = false;

	if (!vdd->self_disp.is_support) {
		LCD_INFO(vdd, "self_disp is not enabled\n");
		ret = -EPERM;
		goto err_skip;
	}

	src_ss_cmd = &vdd->self_disp.self_mask_crc_cmd;

	/* update target ss cmd except qc_cmd pointer */
	tmp_qc_cmd = cmd->qc_cmd;
	cmd = src_ss_cmd;
	cmd->qc_cmd = tmp_qc_cmd;

	/* update target qc cmd except ss_cmd pointer */
	cmd->qc_cmd = src_ss_cmd->qc_cmd;
	cmd->qc_cmd->ss_cmd = cmd;

	/* data pass of ss_bridge_qc_cmd_update */
	cmd->qc_cmd->msg.tx_buf = vdd->self_disp.self_mask_crc_cmd.txbuf;
	cmd->qc_cmd->msg.tx_len = vdd->self_disp.self_mask_crc_cmd.tx_len;

	LCD_INFO(vdd, "cmd->tx_len:%lu, tx_buf:[0x%x, 0x%x..\n",
		cmd->qc_cmd->msg.tx_len, vdd->self_disp.self_mask_crc_cmd.txbuf[0],
		vdd->self_disp.self_mask_crc_cmd.txbuf[1]);

	return 0;

err_skip:
	cmd->skip_by_cond = true;
	return ret;
}

static int self_display_debug(struct samsung_display_driver_data *vdd)
{
	int rx_len;
	char buf[4];

	rx_len = ss_send_cmd_get_rx(vdd, RX_SELF_DISP_DEBUG, buf);
	if (rx_len < 0) {
		LCD_ERR(vdd, "invalid rx_len(%d)\n", rx_len);
		return -1;
	}

	vdd->self_disp.debug.SM_SUM_O = ((buf[0] & 0xFF) << 24);
	vdd->self_disp.debug.SM_SUM_O |= ((buf[1] & 0xFF) << 16);
	vdd->self_disp.debug.SM_SUM_O |= ((buf[2] & 0xFF) << 8);
	vdd->self_disp.debug.SM_SUM_O |= (buf[3] & 0xFF);

	if (vdd->self_disp.operation[FLAG_SELF_MASK].img_checksum != vdd->self_disp.debug.SM_SUM_O) {
		LCD_ERR(vdd, "self mask img checksum fail!! expected = 0x%X, read value = 0x%X\n",
			vdd->self_disp.operation[FLAG_SELF_MASK].img_checksum, vdd->self_disp.debug.SM_SUM_O);
		SS_XLOG(vdd->self_disp.debug.SM_SUM_O);
		return -1;
	}

	return 0;
}

static void self_mask_img_write(struct samsung_display_driver_data *vdd)
{
	if (!vdd->self_disp.is_support) {
		LCD_ERR(vdd, "self display is not supported..(%d) \n",
						vdd->self_disp.is_support);
		return;
	}

	LCD_INFO(vdd, "tx self mask\n");

	ss_block_commit(vdd);
	ss_send_cmd(vdd, TX_SELF_MASK_SETTING);
	ss_release_commit(vdd);
}

static int self_mask_on(struct samsung_display_driver_data *vdd, int enable)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR(vdd, "self display is not supported..(%d) \n",
						vdd->self_disp.is_support);
		return -EACCES;
	}

	LCD_INFO(vdd, "++ (%d)\n", enable);

	mutex_lock(&vdd->self_disp.vdd_self_display_lock);

	if (enable) {
		if (vdd->is_factory_mode && vdd->self_disp.factory_support)
			ss_send_cmd(vdd, TX_SELF_MASK_ON_FACTORY);
		else
			ss_send_cmd(vdd, TX_SELF_MASK_ON);
	} else {
		ss_send_cmd(vdd, TX_SELF_MASK_OFF);
	}

	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);

	LCD_INFO(vdd, "-- \n");

	return ret;
}

static int self_mask_udc_on(struct samsung_display_driver_data *vdd, int enable)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR(vdd, "self display is not supported..(%d) \n",
						vdd->self_disp.is_support);
		return -EACCES;
	}

	LCD_INFO(vdd, "++ (%d)\n", enable);

	mutex_lock(&vdd->self_disp.vdd_self_display_lock);

	if (enable)
		ss_send_cmd(vdd, TX_SELF_MASK_UDC_ON);
	else
		ss_send_cmd(vdd, TX_SELF_MASK_UDC_OFF);

	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);

	LCD_INFO(vdd, "-- \n");

	return ret;
}

#define WAIT_FRAME (2)

static int self_mask_check(struct samsung_display_driver_data *vdd)
{
	int rx_len;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR(vdd, "self display is not supported..(%d) \n",
						vdd->self_disp.is_support);
		return -EPERM;
	}


	LCD_INFO(vdd, "++ \n");

	mutex_lock(&vdd->self_disp.vdd_self_display_lock);

	ss_block_commit(vdd);
	rx_len = ss_send_cmd_get_rx(vdd, TX_SELF_MASK_CHECK, vdd->self_disp.mask_crc_read_data);
	ss_release_commit(vdd);

	mutex_unlock(&vdd->self_disp.vdd_self_display_lock);

	if (memcmp(vdd->self_disp.mask_crc_read_data,
			vdd->self_disp.mask_crc_pass_data,
			vdd->self_disp.mask_crc_size)) {
		LCD_ERR(vdd, "self mask check fail !!\n");
		return -1;
	}

	LCD_INFO(vdd, "-- \n");

	return 0;
}

static int self_partial_hlpm_scan_set(struct samsung_display_driver_data *vdd)
{
	u8 *cmd_pload;
	struct self_partial_hlpm_scan sphs_info;
	struct dsi_panel_cmd_set *pcmds;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	LCD_INFO(vdd, "++\n");

	sphs_info = vdd->self_disp.sphs_info;

	LCD_INFO(vdd, "Self Partial HLPM hlpm_en(%d) \n", sphs_info.hlpm_en);

	pcmds = ss_get_cmds(vdd, TX_SELF_PARTIAL_HLPM_SCAN_SET);
	if (SS_IS_CMDS_NULL(pcmds)) {
		LCD_ERR(vdd, "No cmds for TX_SELF_PARTIAL_HLPM_SCAN_SET..\n");
		return -ENODEV;
	}
	cmd_pload = pcmds->cmds[1].ss_txbuf;

	/* Partial HLPM */
	if (sphs_info.hlpm_en)
		cmd_pload[1] = 0x03;
	else
		cmd_pload[1] = 0x00;

	ss_send_cmd(vdd, TX_SELF_PARTIAL_HLPM_SCAN_SET);

	LCD_INFO(vdd, "--\n");

	return 0;
}

static int self_display_aod_enter(struct samsung_display_driver_data *vdd)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_DEBUG(vdd, "self display is not supported..(%d) \n",
								vdd->self_disp.is_support);
		return -ENODEV;
	}

	LCD_INFO(vdd, "++\n");

	if (!vdd->self_disp.on) {
		/* Self display on */
		ss_send_cmd(vdd, TX_SELF_DISP_ON);
		self_mask_on(vdd, false);
	}

	vdd->self_disp.on = true;

	LCD_INFO(vdd, "--\n");

	return ret;
}

static int self_display_aod_exit(struct samsung_display_driver_data *vdd)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_DEBUG(vdd, "self display is not supported..(%d) \n",
								vdd->self_disp.is_support);
		return -ENODEV;
	}

	LCD_INFO(vdd, "++\n");

	/* self display off */
	ss_send_cmd(vdd, TX_SELF_DISP_OFF);

	ret = self_display_debug(vdd);
	if (!ret)
		self_mask_on(vdd, true);
	else
		LCD_ERR(vdd, "Self Mask CheckSum Error! Skip Self Mask On\n");

	LCD_INFO(vdd, "write self_mask_udc %s cmd \n",
		vdd->self_disp.udc_mask_enable ? "enable" : "disable");

	if (vdd->self_disp.udc_mask_enable)
		ss_send_cmd(vdd, TX_SELF_MASK_UDC_ON);
	else
		ss_send_cmd(vdd, TX_SELF_MASK_UDC_OFF);

	if (vdd->self_disp.reset_status)
		vdd->self_disp.reset_status(vdd);

	LCD_INFO(vdd, "--\n");

	return ret;
}

static void self_display_reset_status(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return;
	}

	if (!vdd->self_disp.is_support) {
		LCD_DEBUG(vdd, "self display is not supported..(%d) \n",
				vdd->self_disp.is_support);
		return;
	}

	vdd->self_disp.sa_info.en = false;
	vdd->self_disp.sd_info.en = false;
	vdd->self_disp.si_info.en = false;
	vdd->self_disp.sg_info.en = false;
	vdd->self_disp.time_set = false;
	vdd->self_disp.on = false;

	return;
}

/*
 * self_display_ioctl() : get ioctl from aod framework.
 *                           set self display related registers.
 */
static long self_display_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;
	void __user *argp = (void __user *)arg;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return -ENODEV;
	}

	if (!vdd->self_disp.on) {
		LCD_ERR(vdd, "self_display was turned off\n");
		return -EPERM;
	}

	if ((_IOC_TYPE(cmd) != SELF_DISPLAY_IOCTL_MAGIC) ||
				(_IOC_NR(cmd) >= IOCTL_SELF_MAX)) {
		LCD_ERR(vdd, "TYPE(%u) NR(%u) is wrong..\n",
			_IOC_TYPE(cmd), _IOC_NR(cmd));
		return -EINVAL;
	}

	mutex_lock(&vdd->self_disp.vdd_self_display_ioctl_lock);

	LCD_INFO(vdd, "cmd = %s\n", cmd == IOCTL_SELF_MOVE_EN ? "IOCTL_SELF_MOVE_EN" :
				cmd == IOCTL_SELF_MOVE_OFF ? "IOCTL_SELF_MOVE_OFF" :
				cmd == IOCTL_SET_ICON ? "IOCTL_SET_ICON" :
				cmd == IOCTL_SET_GRID ? "IOCTL_SET_GRID" :
				cmd == IOCTL_SET_ANALOG_CLK ? "IOCTL_SET_ANALOG_CLK" :
				cmd == IOCTL_SET_DIGITAL_CLK ? "IOCTL_SET_DIGITAL_CLK" :
				cmd == IOCTL_SET_TIME ? "IOCTL_SET_TIME" :
				cmd == IOCTL_SET_PARTIAL_HLPM_SCAN ? "IOCTL_PARTIAL_HLPM_SCAN" : "IOCTL_ERR");

	switch (cmd) {
	case IOCTL_SET_PARTIAL_HLPM_SCAN:
		ret = copy_from_user(&vdd->self_disp.sphs_info, argp,
					sizeof(vdd->self_disp.sphs_info));
		if (ret) {
			LCD_ERR(vdd, "fail to copy_from_user.. (%d)\n", ret);
			goto error;
		}

		ret = self_partial_hlpm_scan_set(vdd);
		break;
	default:
		LCD_ERR(vdd, "invalid cmd : %u \n", cmd);
		break;
	}
error:
	mutex_unlock(&vdd->self_disp.vdd_self_display_ioctl_lock);
	return ret;
}

/*
 * self_display_write() : get image data from aod framework.
 *                           prepare for dsi_panel_cmds.
 */
static ssize_t self_display_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	return 0;
}

static int self_display_open(struct inode *inode, struct file *file)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	vdd->self_disp.file_open = 1;

	LCD_DEBUG(vdd, "[open]\n");

	return 0;
}

static int self_display_release(struct inode *inode, struct file *file)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	vdd->self_disp.file_open = 0;

	LCD_DEBUG(vdd, "[release]\n");

	return 0;
}

static const struct file_operations self_display_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = self_display_ioctl,
	.open = self_display_open,
	.release = self_display_release,
	.write = self_display_write,
};

#define DEV_NAME_SIZE 24
int self_display_init(struct samsung_display_driver_data *vdd)
{
	int ret = 0;
	static char devname[DEV_NAME_SIZE] = {'\0', };

	struct dsi_panel *panel = NULL;
	struct mipi_dsi_host *host = NULL;
	struct dsi_display *display = NULL;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR(vdd, "Self Display is not supported\n");
		return -EINVAL;
	}

	panel = (struct dsi_panel *)vdd->msm_private;
	host = panel->mipi_device.host;
	display = container_of(host, struct dsi_display, host);

	mutex_init(&vdd->self_disp.vdd_self_display_lock);
	mutex_init(&vdd->self_disp.vdd_self_display_ioctl_lock);

	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		snprintf(devname, DEV_NAME_SIZE, "self_display");
	else
		snprintf(devname, DEV_NAME_SIZE, "self_display%d", vdd->ndx);

	vdd->self_disp.dev.minor = MISC_DYNAMIC_MINOR;
	vdd->self_disp.dev.name = devname;
	vdd->self_disp.dev.fops = &self_display_fops;
	vdd->self_disp.dev.parent = &display->pdev->dev;

	vdd->self_disp.reset_status = self_display_reset_status;
	vdd->self_disp.aod_enter = self_display_aod_enter;
	vdd->self_disp.aod_exit = self_display_aod_exit;
	vdd->self_disp.self_mask_img_write = self_mask_img_write;
	vdd->self_disp.self_mask_on = self_mask_on;
	vdd->self_disp.self_mask_udc_on = self_mask_udc_on;
	vdd->self_disp.self_mask_check = self_mask_check;
	vdd->self_disp.self_partial_hlpm_scan_set = self_partial_hlpm_scan_set;
	vdd->self_disp.self_display_debug = self_display_debug;

	vdd->self_disp.debug.SM_SUM_O = 0xFF; /* initial value */

	ret = ss_wrapper_misc_register(vdd, &vdd->self_disp.dev);
	if (ret) {
		LCD_ERR(vdd, "failed to register driver : %d\n", ret);
		vdd->self_disp.is_support = false;
		return -ENODEV;
	}

	register_op_sym_cb(vdd, "SELF_MASK_IMAGE", update_self_mask_image, true);
	register_op_sym_cb(vdd, "SELF_MASK_CRC_IMAGE", update_self_mask_crc_image, true);

	LCD_INFO(vdd, "Success to register self_disp device..(%d)\n", ret);

	return ret;
}

MODULE_DESCRIPTION("Self Display driver");
MODULE_LICENSE("GPL");

