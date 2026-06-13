/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * DDI ABC operation
 * Author: Samsung Display Driver Team <cj1225.jang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ss_dsi_panel_common.h"
#include "ss_dsi_mafpc.h"
#include <linux/crc32.h>

#define MAFPC_DRV_VER (0x01000000)

static int ss_mafpc_make_img_mass_cmds(struct samsung_display_driver_data *vdd,
				char *data, u32 data_size, struct ss_cmd_desc *ss_cmd)
{
	int ret = 0;

	SDE_ATRACE_BEGIN("mafpc_mass_cmd_generation");
	mutex_lock(&vdd->mafpc.vdd_mafpc_lock);

	ret = ss_convert_img_to_mass_cmds(vdd, data, data_size, ss_cmd);

	mutex_unlock(&vdd->mafpc.vdd_mafpc_lock);
	SDE_ATRACE_END("mafpc_mass_cmd_generation");

	LCD_INFO(vdd, "tx_len: %lu\n", ss_cmd->tx_len);

	return ret;
}

#define WAIT_FRAME (1)

static int ss_mafpc_img_write(struct samsung_display_driver_data *vdd, bool is_instant)
{
	if (!vdd->mafpc.is_support) {
		LCD_ERR(vdd, "mafpc is not supported..(%d) \n", vdd->mafpc.is_support);
		return -EACCES;
	}

	LCD_INFO(vdd, "++(%d)\n", is_instant);

	mutex_lock(&vdd->self_disp.vdd_self_display_ioctl_lock);
	mutex_lock(&vdd->mafpc.vdd_mafpc_lock);

	if (!vdd->esync_support && is_instant)
		ss_block_commit(vdd);

	if (vdd->esync_support) {
		if (is_instant) {
			ss_wrapper_sde_encoder_enter_idle(vdd, true);
			ss_send_cmd(vdd, TX_MAFPC_SETTING_INSTANCE);
			ss_wrapper_sde_encoder_enter_idle(vdd, false);
		} else
			ss_send_cmd(vdd, TX_MAFPC_SETTING);
	} else {
		ss_send_cmd(vdd, TX_MAFPC_SETTING);
	}

	if (!vdd->esync_support && is_instant)
		ss_release_commit(vdd);

	mutex_unlock(&vdd->mafpc.vdd_mafpc_lock);
	mutex_unlock(&vdd->self_disp.vdd_self_display_ioctl_lock);

	LCD_INFO(vdd, "--(%d)\n", is_instant);

	return 0;
}

static int ss_mafpc_enable(struct samsung_display_driver_data *vdd, int enable)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->mafpc.is_support) {
		LCD_ERR(vdd, "mafpc is not supported..(%d) \n", vdd->mafpc.is_support);
		return -EACCES;
	}

	mutex_lock(&vdd->mafpc.vdd_mafpc_lock);

	if (enable) {
		ss_send_cmd(vdd, TX_MAFPC_ON);

		/* To update mAFPC brightness scale factor */
		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
	} else
		ss_send_cmd(vdd, TX_MAFPC_OFF);

	mutex_unlock(&vdd->mafpc.vdd_mafpc_lock);

	LCD_INFO(vdd, "%s\n", enable ? "Enable" : "Disable");

	return 0;
}

static int ss_mafpc_crc_check(struct samsung_display_driver_data *vdd)
{
	int rx_len;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->mafpc.is_support) {
		LCD_ERR(vdd, "mafpc is not supported..(%d) \n", vdd->mafpc.is_support);
		return -ENODEV;
	}


	if (!vdd->mafpc.crc_size) {
		LCD_ERR(vdd, "mAFPC crc size is zero..\n\n");
		return -EINVAL;
	}

	LCD_INFO(vdd, "++\n");
	mutex_lock(&vdd->mafpc.vdd_mafpc_crc_check_lock);

	/* prevent sw reset to trigger esd recovery */
	LCD_INFO(vdd, "disable esd interrupt\n");
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd, ESD_MASK_MAFPC_CRC);

	ss_block_commit(vdd);
	rx_len = ss_send_cmd_get_rx(vdd, TX_MAFPC_CRC_CHECK, vdd->mafpc.crc_read_data);
	ss_release_commit(vdd);

	if (rx_len <= 0) {
		LCD_ERR(vdd, "fail to read ddi id(%d)\n", rx_len);
		return -EINVAL;
	}

	if (rx_len != vdd->mafpc.crc_size) {
		LCD_ERR(vdd, "rx_len(%d) != crc_size(%d)\n", rx_len, vdd->mafpc.crc_size);
		return -EINVAL;
	}

	if (memcmp(vdd->mafpc.crc_read_data, vdd->mafpc.crc_pass_data, vdd->mafpc.crc_size)) {
		LCD_ERR(vdd, "mAFPC CRC check fail !!\n");
		ret = -EFAULT;
	}

	/* enable esd interrupt */
	LCD_INFO(vdd, "enable esd interrupt\n");
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(true, true, (void *)vdd, ESD_MASK_MAFPC_CRC);

	mutex_unlock(&vdd->mafpc.vdd_mafpc_crc_check_lock);
	LCD_INFO(vdd, "-- \n");

	return ret;
}


static int ss_mafpc_debug(struct samsung_display_driver_data *vdd)
{
	return 0;
}

static int update_mafpc_scale(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	int idx;
	int i = -1, j;
	int ret;

	cmd->skip_by_cond = false;

	if (!vdd->mafpc.en) {
		LCD_DEBUG(vdd, "mAFPC is not enabled\n");
		ret = -EPERM;
		goto err_skip;
	}

	if (!vdd->mafpc.is_br_table_updated) {
		LCD_ERR(vdd, "Brightness Table for mAFPC is not updated yet\n");
		ret = -EPERM;
		goto err_skip;
	}

	while (!cmd->pos_0xXX[++i] && i < cmd->tx_len);

	if (i + vdd->mafpc_scale_table.col_size > cmd->tx_len) {
		LCD_ERR(vdd, "fail to find proper 0xXX position(%d, %lu)\n",
				i, cmd->tx_len);
		ret = -EINVAL;
		goto err_skip;
	}

	idx = vdd->mafpc.scale_idx;
	if (idx < 0 || idx >= vdd->mafpc_scale_table.row_size) {
		LCD_ERR(vdd, "Invalid index for mAFPC scale table: %d/%d\n",
			idx, vdd->mafpc_scale_table.row_size);
		ret = -EINVAL;
		goto err_skip;
	}

	for (j = 0; j < vdd->mafpc_scale_table.col_size; j++)
		cmd->txbuf[i + j] = vdd->mafpc_scale_table.cmds[idx][j];

	LCD_INFO(vdd, "idx [%d] : %x %x %x\n", idx,
			cmd->txbuf[i], cmd->txbuf[i + 1], cmd->txbuf[i + 2]);

	return 0;

err_skip:
	cmd->skip_by_cond = true;
	return ret;
}

static int update_abc_data(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	bool is_factory_mode = state->is_factory_mode;
	struct ss_cmd_desc *src_ss_cmd;
	struct dsi_cmd_desc *tmp_qc_cmd;

	if (is_factory_mode)
		src_ss_cmd = &vdd->mafpc_crc_img_cmd;
	else
		src_ss_cmd = &vdd->mafpc_img_cmd;

	/* update target ss cmd except qc_cmd pointer */
	tmp_qc_cmd = cmd->qc_cmd;
	cmd = src_ss_cmd;
	cmd->qc_cmd = tmp_qc_cmd;

	/* update target qc cmd except ss_cmd pointer */
	cmd->qc_cmd = src_ss_cmd->qc_cmd;
	cmd->qc_cmd->ss_cmd = cmd;

	/* data pass of ss_bridge_qc_cmd_update */
	if (is_factory_mode) {
		cmd->qc_cmd->msg.tx_buf = vdd->mafpc_crc_img_cmd.txbuf;
		cmd->qc_cmd->msg.tx_len = vdd->mafpc_crc_img_cmd.tx_len;
	} else {
		cmd->qc_cmd->msg.tx_buf = vdd->mafpc_img_cmd.txbuf;
		cmd->qc_cmd->msg.tx_len = vdd->mafpc_img_cmd.tx_len;
	}

	LCD_INFO(vdd, "cmd->tx_len:%lu, tx_buf:[0x%x, 0x%x, 0x%x..\n",
		cmd->qc_cmd->msg.tx_len, vdd->mafpc_img_cmd.txbuf[0],
		vdd->mafpc_img_cmd.txbuf[1], vdd->mafpc_img_cmd.txbuf[2]);

	return 0;
}

static int update_abc_ctrl_data(struct samsung_display_driver_data *vdd,
			char *val, struct ss_cmd_desc *cmd)
{
	u32 ctrl_cmd_size = vdd->mafpc.enable_cmd_size;
	char *ctrl_cmd_buf = vdd->mafpc.enable_cmd_buf;
	int i = -1, j, pos = 0;
	char show_buf[200] = {0, };

	if (!ctrl_cmd_buf) {
		LCD_ERR(vdd, "ctrl_cmd_buf is NULL\n");
		goto err_skip;
	}

	while (!cmd->pos_0xXX[++i] && i < cmd->tx_len);

	if (i + 1 >= cmd->tx_len) {
		LCD_ERR(vdd, "fail to find proper 0xXX position\n");
		goto err_skip;
	}

	memcpy(&cmd->txbuf[i], ctrl_cmd_buf, ctrl_cmd_size);

	for (j = 0; j < ctrl_cmd_size; j++)
		pos += scnprintf(show_buf + pos, sizeof(show_buf) - pos, "%02x ", ctrl_cmd_buf[j]);

	LCD_INFO(vdd, "update ABC enable cmd [%d] = %s\n", ctrl_cmd_size, show_buf);

	return 0;

err_skip:
	cmd->skip_by_cond = true;
	return -EINVAL;
}

/*
 * ss_mafpc_ioctl() : get ioctl from mdnie framework.
 */
static long ss_mafpc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
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

	if ((_IOC_TYPE(cmd) != MAFPC_IOCTL_MAGIC) ||
				(_IOC_NR(cmd) >= IOCTL_MAFPC_MAX)) {
		LCD_ERR(vdd, "TYPE(%u) NR(%u) is wrong..\n",
			_IOC_TYPE(cmd), _IOC_NR(cmd));
		return -EINVAL;
	}

	LCD_INFO(vdd, "cmd = %s\n", cmd == MAFPC_IOCTL_ON ? "IOCTL_MAFPC_ON" :
				cmd == MAFPC_IOCTL_OFF ? "MAFPC_IOCTL_OFF" :
				cmd == MAFPC_IOCTL_ON_INSTANT ? "MAFPC_IOCTL_ON_INSTANT" :
				cmd == MAFPC_IOCTL_OFF_INSTANT ? "MAFPC_IOCTL_OFF_INSTANT" : "IOCTL_ERR");

	switch (cmd) {
	case MAFPC_IOCTL_ON:
		vdd->mafpc.en = true;
		break;
	case MAFPC_IOCTL_ON_INSTANT:
		if (vdd->esync_support) {
			LCD_ERR(vdd, "IOCTL_MAFPC_ON_INSTANT is not supported on VHM panel\n");
			return -ENXIO;
		}

		vdd->mafpc.en = true;
		if (!ss_is_ready_to_send_cmd(vdd)) {
			LCD_INFO(vdd, "Panel is not ready(%d), will apply next display on\n",
					vdd->panel_state);
			return -ENODEV;
		}

		ss_mafpc_img_write(vdd, true);
		ss_mafpc_enable(vdd, true);
		break;
	case MAFPC_IOCTL_OFF:
		vdd->mafpc.en = false;
		break;
	case MAFPC_IOCTL_OFF_INSTANT:
		if (!ss_is_ready_to_send_cmd(vdd)) {
			LCD_ERR(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
			return -ENODEV;
		}
		vdd->mafpc.en = false;
		ss_mafpc_enable(vdd, false);
		break;
	case MAFPC_IOCTL_GET_VER:
		if (copy_to_user(argp, &vdd->mafpc.version, sizeof(vdd->mafpc.version)))
			return -EFAULT;
		break;
	case MAFPC_IOCTL_GET_CRC:
		if (copy_to_user(argp, &vdd->mafpc.img_crc, sizeof(vdd->mafpc.img_crc)))
			return -EFAULT;
		break;
	default:
		LCD_ERR(vdd, "invalid cmd : %u \n", cmd);
		break;
	}

	return ret;
}

/*
 * ss_mafpc_write_from_user() : get mfapc image data from mdnie framework.
 *                           	prepare for dsi_panel_cmds.
 */
static ssize_t ss_mafpc_write_from_user(struct file *file, const char __user *user_buf,
			 size_t total_count, loff_t *ppos)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;
	int i, j, k = 0;
	int ret = 0;

	u32 enable_cmd_size = vdd->mafpc.enable_cmd_size;
	char *enable_cmd_buf = vdd->mafpc.enable_cmd_buf;
	u32 img_size = vdd->mafpc.img_size;
	char *img_buf = vdd->mafpc.img_buf;
	u32 br_table_size = vdd->mafpc.scale_table_size;
	char *scale_tbl_buf = vdd->mafpc.scale_table_buf;
	u32 hdr_size = vdd->mafpc.header_cmd_size;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd");
		return -ENODEV;
	}

	if (unlikely(!enable_cmd_buf)) {
		LCD_ERR(vdd, "No mafpc Enable cmd Buffer\n");
		return -ENODEV;
	}

	if (unlikely(!img_buf)) {
		LCD_ERR(vdd, "No mafpc Image Buffer\n");
		return -ENODEV;
	}

	if (unlikely(!scale_tbl_buf)) {
		LCD_ERR(vdd, "No mafpc scale tbl Buffer\n");
		return -ENODEV;
	}

	if (unlikely(!user_buf)) {
		LCD_ERR(vdd, "invalid user buffer\n");
		return -EINVAL;
	}

	if (total_count != (enable_cmd_size + hdr_size + img_size + br_table_size)) {
		LCD_ERR(vdd, "Invalid size %zu, should be %u\n",
				total_count, (enable_cmd_size + hdr_size + img_size + br_table_size));
		return -EINVAL;
	}

	LCD_INFO(vdd, "Total_Count(%zu), hdr_size (%u) cmd_size(%u), img_size(%u), br_table_size(%u)\n",
			total_count, hdr_size, enable_cmd_size, img_size, br_table_size);

	/*
	 * Get 67bytes Enable Command to match with mafpc image data
	   (1Byte(Header) + 66Byte(Control Data))
	 */
	ret = copy_from_user(enable_cmd_buf, user_buf + hdr_size, enable_cmd_size);
	if (unlikely(ret < 0)) {
		LCD_ERR(vdd, "failed to copy_from_user (Enable Command)\n");
		return -EINVAL;
	}

	/* Get ABC Compensation Image Data from user space (mDNIE Service) */
	ret = copy_from_user(img_buf, user_buf + enable_cmd_size + hdr_size, img_size);
	if (unlikely(ret < 0)) {
		LCD_ERR(vdd, "failed to copy_from_user (Image Data)\n");
		return -EINVAL;
	}

	vdd->mafpc.img_crc = crc32_le(~0, img_buf, img_size);

	/* Get 225(75 x 3)Bytes for brightness scale cmd table from user space (mDNIE Service) */
	ret = copy_from_user(scale_tbl_buf, user_buf + enable_cmd_size + hdr_size + img_size, br_table_size);
	if (unlikely(ret < 0)) {
		LCD_ERR(vdd, "failed to copy_from_user (Brightness Scale Table)\n");
		return -EINVAL;
	}

	ret = ss_mafpc_make_img_mass_cmds(vdd, vdd->mafpc.img_buf, vdd->mafpc.img_size, &vdd->mafpc_img_cmd);
	if (unlikely(ret < 0)) {
		LCD_ERR(vdd, "failed to make mafpc mass cmds\n");
		return ret;
	}

	for (i = 0; i < vdd->mafpc_scale_table.row_size; i++) {
		for (j = 0; j < vdd->mafpc_scale_table.col_size; j++)
			vdd->mafpc_scale_table.cmds[i][j] = scale_tbl_buf[k++];

		LCD_INFO(vdd, "[%02d] %X %X %X\n", i,
			vdd->mafpc_scale_table.cmds[i][0],
			vdd->mafpc_scale_table.cmds[i][1],
			vdd->mafpc_scale_table.cmds[i][2]);
	}

	vdd->mafpc.is_br_table_updated = true;

	LCD_INFO(vdd, "mafpc image write %ld done! (img_crc 0x%x)\n",
			total_count, vdd->mafpc.img_crc);

	return total_count;
}

static int ss_mafpc_open(struct inode *inode, struct file *file)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	vdd->mafpc.file_open = true;

	LCD_DEBUG(vdd, "[OPEN]\n");

	return 0;
}

static int ss_mafpc_release(struct inode *inode, struct file *file)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	vdd->mafpc.file_open = false;

	LCD_DEBUG(vdd, "[RELEASE]\n");

	return 0;
}

static const struct file_operations mafpc_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = ss_mafpc_ioctl,
	.open = ss_mafpc_open,
	.release = ss_mafpc_release,
	.write = ss_mafpc_write_from_user,
};

#define DEV_NAME_SIZE 24
int ss_mafpc_init(struct samsung_display_driver_data *vdd)
{
	int ret = 0;
	static char devname[DEV_NAME_SIZE] = {'\0', };
	struct dsi_panel_cmd_set *pcmds;

	struct dsi_panel *panel = NULL;
	struct mipi_dsi_host *host = NULL;
	struct dsi_display *display = NULL;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->mafpc.is_support) {
		LCD_ERR(vdd, "mAFPC is not supported\n");
		return -EINVAL;
	}

	panel = (struct dsi_panel *)vdd->msm_private;
	host = panel->mipi_device.host;
	display = container_of(host, struct dsi_display, host);

	mutex_init(&vdd->mafpc.vdd_mafpc_lock);
	mutex_init(&vdd->mafpc.vdd_mafpc_crc_check_lock);

	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		snprintf(devname, DEV_NAME_SIZE, "mafpc");
	else
		snprintf(devname, DEV_NAME_SIZE, "mafpc%d", vdd->ndx);

	vdd->mafpc.version = MAFPC_DRV_VER;
	vdd->mafpc.dev.minor = MISC_DYNAMIC_MINOR;
	vdd->mafpc.dev.name = devname;
	vdd->mafpc.dev.fops = &mafpc_fops;
	vdd->mafpc.dev.parent = &display->pdev->dev;;

	vdd->mafpc.enable = ss_mafpc_enable;
	vdd->mafpc.crc_check = ss_mafpc_crc_check;
	vdd->mafpc.make_img_mass_cmds = ss_mafpc_make_img_mass_cmds;
	vdd->mafpc.img_write = ss_mafpc_img_write;
	vdd->mafpc.debug = ss_mafpc_debug;

	/* Alloc the memory for ABC scale table */
	vdd->mafpc.scale_table_size = vdd->mafpc_scale_table.row_size * vdd->mafpc_scale_table.col_size;
	if (vdd->mafpc.scale_table_size) {
		vdd->mafpc.scale_table_buf = kzalloc(vdd->mafpc.scale_table_size, GFP_KERNEL);
		if (IS_ERR_OR_NULL(vdd->mafpc.scale_table_buf))
			LCD_ERR(vdd, "fail to allocate scale_table_buf(%d)\n", vdd->mafpc.scale_table_size);
	}

	LCD_INFO(vdd, "scale_table_size [%d] = (%d) x (%d)\n", vdd->mafpc.scale_table_size,
		vdd->mafpc_scale_table.row_size, vdd->mafpc_scale_table.col_size);

	/* Alloc the memory for ABC enable cmd */
	if (vdd->mafpc.enable_cmd_size) {
		vdd->mafpc.enable_cmd_buf = kzalloc(vdd->mafpc.enable_cmd_size, GFP_KERNEL);
		if (IS_ERR_OR_NULL(vdd->mafpc.enable_cmd_buf)) {
			LCD_ERR(vdd, "Failed to alloc mafpc enable cmd buffer\n");
		} else {
			/* initialize mafpc enable cmd to default control data */
			pcmds = ss_get_cmds(vdd, TX_MAFPC_CTRL_DATA);
			if (pcmds->count <= 0) {
				LCD_ERR(vdd, "no mafpc_default_enable_cmd cmds\n");
			} else {
				memcpy(vdd->mafpc.enable_cmd_buf, &pcmds->cmds[0].ss_txbuf[1],
					vdd->mafpc.enable_cmd_size);
				LCD_INFO(vdd, "initialize mafpc enable cmd\n");
			}
		}
	}

	LCD_INFO(vdd, "enable_cmd_size [%d]\n", vdd->mafpc.enable_cmd_size);

	ret = ss_wrapper_misc_register(vdd, &vdd->mafpc.dev);
	if (ret) {
		LCD_ERR(vdd, "failed to register driver : %d\n", ret);
		vdd->mafpc.is_support = false;
		return -ENODEV;
	}

	register_op_sym_cb(vdd, "MAFPC", update_mafpc_scale, true);
	register_op_sym_cb(vdd, "ABC_DATA", update_abc_data, true);
	register_op_sym_cb(vdd, "UPDATE_ABC_CTRL_DATA", update_abc_ctrl_data, true);

	LCD_INFO(vdd, "Success to register mafpc device..(%d)\n", ret);

	return ret;
}

MODULE_DESCRIPTION("mAFPC driver");
MODULE_LICENSE("GPL");

