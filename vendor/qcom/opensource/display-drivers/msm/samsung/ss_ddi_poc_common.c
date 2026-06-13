/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * Samsung's POC Driver
 * Author: ChangJae Jang <cj1225.jang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ss_dsi_panel_common.h"
#include "ss_ddi_poc_common.h"

/*
 * ERASE (SECTOR)
 * Erase operation is not a misc device file operation.
 * This is called from sysfs.
 */
static int ss_poc_erase_sector(struct samsung_display_driver_data *vdd, int start, int len)
{
	int pos = 0;
	int image_size = 0;
	int ret = 0;
	int erase_size = 0;
	int target_pos = 0;

	image_size = vdd->poc_driver.image_size;
	if(!start)
		start = vdd->poc_driver.start_addr;
	target_pos = start + len;

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return -EBUSY;
	}

	if (start < 0 || len <= 0) {
		LCD_INFO(vdd, "invalid sector erase params.. start(%d), len(%d)\n", start, len);
		return -EINVAL;
	}

	if ((target_pos - start) > vdd->poc_driver.image_size) {
		LCD_INFO(vdd, "sould not erase over %d, start(%d) len(%d) target_pos(%d)\n",
			vdd->poc_driver.image_size, start, len, target_pos);
		return -EINVAL;
	}

	LCD_INFO(vdd, "start(%d) len(%d) target(%d)\n", start, len, target_pos);

	for (pos = start; pos < target_pos; pos += erase_size) {
		if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
			LCD_INFO(vdd, "cancel poc read by user\n");
			ret = -EIO;
			goto cancel_poc;
		}

		if (vdd->poc_driver.poc_erase) {
			if (!(pos % POC_ERASE_64KB) && (pos + POC_ERASE_64KB <= target_pos))
				erase_size = POC_ERASE_64KB;
			else if (!(pos % POC_ERASE_32KB) && (pos + POC_ERASE_32KB <= target_pos))
				erase_size = POC_ERASE_32KB;
			else
				erase_size = POC_ERASE_4KB;

			ret = vdd->poc_driver.poc_erase(vdd, pos, erase_size, target_pos);
			if (ret) {
				LCD_INFO(vdd, "fail to erase, pos(%d)\n", pos);
				return -EIO;
			}
		} else {
			LCD_INFO(vdd, "No poc_erase function. \n");
			return -EIO;
		}
	}

cancel_poc:
	if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
		LCD_INFO(vdd, "cancel poc read by user\n");
		atomic_set(&vdd->poc_driver.cancel, 0);
		ret = -EIO;
	}

	return ret;
}

static int ss_poc_write(struct samsung_display_driver_data *vdd, u8 *data, u32 write_pos, u32 write_size)
{
	int ret = 0;

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return -EBUSY;
	}

	if (vdd->poc_driver.poc_write) {
		ret = vdd->poc_driver.poc_write(vdd, data, write_pos, write_size);
	}
	else {
		LCD_INFO(vdd, "No poc_write function. \n");
		return -ENODEV;
	}

	return ret;
}

static int ss_poc_read(struct samsung_display_driver_data *vdd, u8 *buf, u32 read_pos, u32 read_size)
{
	int ret = 0;

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return -EBUSY;
	}

	if (vdd->poc_driver.poc_read) {
		ret = vdd->poc_driver.poc_read(vdd, buf, read_pos, read_size);
	}
	else {
		LCD_INFO(vdd, "No poc_read function. \n");
		return -ENODEV;
	}

	return ret;
}

int ss_poc_read_mca(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *mca_rx_cmds = NULL;
	int rx_len;

	if (!vdd->poc_driver.is_support) {
		LCD_INFO(vdd, "Not Support POC Driver!\n");
		return -ENODEV;
	}

	mca_rx_cmds = ss_get_cmds(vdd, RX_POC_MCA_CHECK);
	if (SS_IS_CMDS_NULL(mca_rx_cmds)) {
		LCD_INFO(vdd, "no cmds for RX_POC_MCA_CHECK..\n");
		return -ENOMEM;
	}

	vdd->poc_driver.mca_size = mca_rx_cmds->cmds->msg.rx_len;
	LCD_INFO(vdd,  "mca rx size (%d)\n", vdd->poc_driver.mca_size);

	if (vdd->poc_driver.mca_size) {
		if (vdd->poc_driver.mca_data == NULL) {
			vdd->poc_driver.mca_data = kmalloc(vdd->poc_driver.mca_size, GFP_KERNEL);
	 		if (!vdd->poc_driver.mca_data) {
				LCD_INFO(vdd, "fail to kmalloc for mca_data\n");
				return -ENOMEM;
			}
		}
	} else {
		LCD_INFO(vdd, "No rx size!\n");
		return -EINVAL;
	}

	rx_len = ss_send_cmd_get_rx(vdd, RX_POC_MCA_CHECK, vdd->poc_driver.mca_data);
	if (rx_len < 0) {
		LCD_ERR(vdd, "failed to read_mca\n");
		return rx_len;
	}

	return 0;
}

void ss_poc_comp(struct samsung_display_driver_data *vdd)
{
	if (vdd->poc_driver.poc_comp)
		vdd->poc_driver.poc_comp(vdd);

	return;
}

static int ss_poc_checksum(struct samsung_display_driver_data *vdd)
{
	LCD_INFO(vdd,  "POC: checksum\n");
	return 0;
}

static int ss_poc_check_flash(struct samsung_display_driver_data *vdd)
{
	LCD_INFO(vdd,  "POC: check flash\n");
	return 0;
}

static int ss_dsi_poc_ctrl(struct samsung_display_driver_data *vdd, u32 cmd, const char *buf)
{
	int ret = 0;
	int erase_start = 0;
	int erase_len = 0;

	if (cmd >= MAX_POC_OP) {
		LCD_INFO(vdd, "invalid poc_op %d\n", cmd);
		return -EINVAL;
	}

	if (!vdd->poc_driver.is_support) {
		LCD_INFO(vdd, "Not Support POC Driver!\n");
		return -ENODEV;
	}

	LCD_INFO(vdd, " cmd = %d ++\n", cmd);
	switch (cmd) {
	case POC_OP_ERASE:
		break;
	case POC_OP_ERASE_SECTOR:
		if (buf == NULL) {
			LCD_INFO(vdd, "buf is null..\n");
			return -EINVAL;
		}

		ret = sscanf(buf, "%*d %d %d", &erase_start, &erase_len);
		if (unlikely(ret < 2)) {
			LCD_INFO(vdd, "fail to get erase param..\n");
			return -EINVAL;
		}

		vdd->poc_driver.er_try_cnt++;
		ret = ss_poc_erase_sector(vdd, erase_start, erase_len);
		if (unlikely(ret < 0)) {
			LCD_INFO(vdd, "failed to poc-erase-sector-seq\n");
			vdd->poc_driver.er_fail_cnt++;
			return ret;
		}
		break;
	case POC_OP_WRITE:
		if (vdd->poc_driver.wbuf == NULL) {
			LCD_INFO(vdd, "poc_driver.wbuf is NULL\n");
			return -EINVAL;
		}
		ret = ss_poc_write(vdd,
			vdd->poc_driver.wbuf,
			POC_IMG_ADDR + vdd->poc_driver.wpos,
			vdd->poc_driver.wsize);
		if (unlikely(ret < 0)) {
			LCD_INFO(vdd, "failed to write poc-write-seq\n");
			return ret;
		}
		break;
	case POC_OP_READ:
		if (vdd->poc_driver.rbuf == NULL) {
			LCD_INFO(vdd, "poc_driver.rbuf is NULL\n");
			return -EINVAL;
		}
		ret = ss_poc_read(vdd,
			vdd->poc_driver.rbuf,
			POC_IMG_ADDR + vdd->poc_driver.rpos,
			vdd->poc_driver.rsize);
		if (unlikely(ret < 0)) {
			LCD_INFO(vdd, "failed to write poc-read-seq\n");
			return ret;
		}
		break;
	case POC_OP_ERASE_WRITE_IMG:
		break;
	case POC_OP_ERASE_WRITE_TEST:
		break;
	case POC_OP_BACKUP:
		break;
	case POC_OP_CHECKSUM:
		ss_poc_checksum(vdd);
		break;
	case POC_OP_CHECK_FLASH:
		ss_poc_check_flash(vdd);
		break;
	case POC_OP_SET_FLASH_WRITE:
		break;
	case POC_OP_SET_FLASH_EMPTY:
		break;
	case POC_OP_NONE:
		break;
	default:
		LCD_INFO(vdd, "%s invalid poc_op %d\n", __func__, cmd);
		break;
	}
	LCD_INFO(vdd, " cmd = %d ++\n", cmd);
	return ret;
}

static long ss_dsi_poc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -EINVAL;
	}

	if (!vdd->poc_driver.is_support) {
		LCD_INFO(vdd, "Not Support POC Driver!\n");
		return -ENODEV;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return -ENODEV;
	}

	LCD_INFO(vdd,  "POC IOCTL CMD=%d\n", cmd);

	switch (cmd) {
	case IOC_GET_POC_CHKSUM:
		ret = ss_dsi_poc_ctrl(vdd, POC_OP_CHECKSUM, NULL);
		if (ret) {
			LCD_INFO(vdd, "%s error set_panel_poc\n", __func__);
			ret = -EFAULT;
		}
		if (copy_to_user((u8 __user *)arg,
				&vdd->poc_driver.chksum_res,
				sizeof(vdd->poc_driver.chksum_res))) {
			ret = -EFAULT;
			break;
		}
		break;
	case IOC_GET_POC_CSDATA:
		ret = ss_dsi_poc_ctrl(vdd, POC_OP_CHECKSUM, NULL);
		if (ret) {
			LCD_INFO(vdd, "%s error set_panel_poc\n", __func__);
			ret = -EFAULT;
		}
		if (copy_to_user((u8 __user *)arg,
				vdd->poc_driver.chksum_data,
				sizeof(vdd->poc_driver.chksum_data))) {
			ret = -EFAULT;
			break;
		}
		break;
	default:
		break;
	};
	return ret;
}

static atomic_t poc_open_check = ATOMIC_INIT(1); /* OPEN/RELEASE CHECK */

static int ss_dsi_poc_open(struct inode *inode, struct file *file)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;
	int ret = 0;
	int image_size = 0;

	LCD_INFO(vdd,  "POC Open !!\n");

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -ENOMEM;
	}

	if (!vdd->poc_driver.is_support) {
		LCD_INFO(vdd, "Not Support POC Driver! \n");
		return -ENODEV;
	}

	if (!atomic_dec_and_test (&poc_open_check)) {
		atomic_inc(&poc_open_check);
		LCD_INFO(vdd, "Already open_ongoing : counter (%d)\n", poc_open_check.counter);
		return -ENOMEM;
	}

	image_size = vdd->poc_driver.image_size;

	if (likely(!vdd->poc_driver.wbuf)) {
		vdd->poc_driver.wbuf = vmalloc(image_size);
		if (unlikely(!vdd->poc_driver.wbuf)) {
			LCD_INFO(vdd, "%s: fail to allocate poc wbuf\n", __func__);
			return -ENOMEM;
		}
	}

	vdd->poc_driver.wpos = 0;
	vdd->poc_driver.wsize = 0;

	if (likely(!vdd->poc_driver.rbuf)) {
		vdd->poc_driver.rbuf = vmalloc(image_size);
		if (unlikely(!vdd->poc_driver.rbuf)) {
			vfree(vdd->poc_driver.wbuf);
			vdd->poc_driver.wbuf = NULL;
			LCD_INFO(vdd, "%s: fail to allocate poc rbuf\n", __func__);
			return -ENOMEM;
		}
	}

	vdd->poc_driver.rpos = 0;
	vdd->poc_driver.rsize = 0;
	atomic_set(&vdd->poc_driver.cancel, 0);

	return ret;
}

static int ss_dsi_poc_release(struct inode *inode, struct file *file)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;
	int ret = 0;

	LCD_INFO(vdd,  "POC Release\n");

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -ENOMEM;
	}

	if (!vdd->poc_driver.is_support) {
		LCD_INFO(vdd, "Not Support POC Driver! \n");
		return -ENODEV;
	}

	if (unlikely(vdd->poc_driver.wbuf)) {
		vfree(vdd->poc_driver.wbuf);
	}

	if (unlikely(vdd->poc_driver.rbuf)) {
		vfree(vdd->poc_driver.rbuf);
	}

	vdd->poc_driver.wbuf = NULL;
	vdd->poc_driver.wpos = 0;
	vdd->poc_driver.wsize = 0;

	vdd->poc_driver.rbuf = NULL;
	vdd->poc_driver.rpos = 0;
	vdd->poc_driver.rsize = 0;
	atomic_set(&vdd->poc_driver.cancel, 0);

	atomic_inc(&poc_open_check);
	LCD_INFO(vdd,  "poc_open counter (%d)\n", poc_open_check.counter); /* 1 */

	return ret;
}

static int _ss_dsi_poc_read(struct samsung_display_driver_data *vdd, char __user *buf, size_t count,
			loff_t *ppos)
{
	int image_size = 0;
	int ret = 0;

	LCD_DEBUG(vdd, "ss_dsi_poc_read \n");

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd");
		return -ENODEV;
	}

	if (!vdd->poc_driver.is_support) {
		LCD_INFO(vdd, "Not Support POC Driver! \n");
		return -ENODEV;
	}

	if (IS_ERR_OR_NULL(vdd->poc_driver.rbuf)) {
		LCD_INFO(vdd, "poc_driver.rbuf is NULL\n");
		return -EINVAL;
	}

	if (unlikely(!buf)) {
		LCD_INFO(vdd, "invalid read buffer\n");
		return -EINVAL;
	}

	image_size = vdd->poc_driver.image_size;

	if (unlikely(*ppos == image_size)) {
		LCD_INFO(vdd, "read is done.. pos (%d), size (%d)\n", (int)*ppos, image_size);
		return -EINVAL;
	}

	if (unlikely(*ppos < 0 || *ppos >= image_size)) {
		LCD_INFO(vdd, "invalid read pos (%d), size (%d)\n", (int)*ppos, image_size);
		return -EINVAL;
	}

	if (unlikely(*ppos + count > image_size)) {
		LCD_INFO(vdd, "invalid read size, pos %d, count %d, size %d\n",
				(int)*ppos, (int)count, image_size);
		count = image_size - (int)*ppos;
		LCD_INFO(vdd, "resizing: pos %d, count %d, size %d",
				(int)*ppos, (int)count, image_size);
	}

	vdd->poc_driver.rpos = *ppos + vdd->poc_driver.start_addr;
	vdd->poc_driver.rsize = (u32)count;

	ret = ss_dsi_poc_ctrl(vdd, POC_OP_READ, NULL);
	if (ret) {
		LCD_INFO(vdd, "fail to read poc (%d)\n", ret);
		return ret;
	}

	return simple_read_from_buffer(buf, count, ppos, vdd->poc_driver.rbuf, image_size);
}

static ssize_t ss_dsi_poc_read(struct file *file, char __user *buf, size_t count,
			loff_t *ppos)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;
	int ret;

	vdd->poc_driver.rd_try_cnt++;

	ret = _ss_dsi_poc_read(vdd, buf, count, ppos);
	if (ret < 0) {
		LCD_INFO(vdd, "fail to poc read..\n");
		vdd->poc_driver.rd_fail_cnt++;
	}

	return ret;
}

static ssize_t _ss_dsi_poc_write(struct samsung_display_driver_data *vdd, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	int image_size = 0;
	int ret = 0;

	LCD_DEBUG(vdd, "ss_dsi_poc_write : count (%d), ppos(%d) \n", (int)count, (int)*ppos);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd");
		return -ENODEV;;
	}

	if (!vdd->poc_driver.is_support) {
		LCD_INFO(vdd, "Not Support POC Driver! \n");
		return -ENODEV;
	}

	if (IS_ERR_OR_NULL(vdd->poc_driver.wbuf)) {
		LCD_INFO(vdd, "poc_driver.wbuf is NULL\n");
		return -EINVAL;
	}

	if (unlikely(!buf)) {
		LCD_INFO(vdd, "invalid read buffer\n");
		return -EINVAL;
	}

	image_size = vdd->poc_driver.image_size;

	if (unlikely(*ppos < 0 || *ppos >= image_size)) {
		LCD_INFO(vdd, "invalid write pos (%d) - size (%d)\n", (int)*ppos, image_size);
		return -EINVAL;
	}

	if (unlikely(*ppos + count > image_size)) {
		LCD_INFO(vdd, "invalid write size pos %d, count %d, size %d\n",
				(int)*ppos, (int)count, image_size);
		count = image_size - (int)*ppos;
		LCD_INFO(vdd, "resizing: pos %d, count %d, size %d",
				(int)*ppos, (int)count, image_size);
	}

	vdd->poc_driver.wpos = *ppos + vdd->poc_driver.start_addr;
	vdd->poc_driver.wsize = (u32)count;

	ret = simple_write_to_buffer(vdd->poc_driver.wbuf, image_size, ppos, buf, count);
	if (unlikely(ret < 0)) {
		LCD_INFO(vdd, "failed to simple_write_to_buffer \n");
		return ret;
	}

	ret = ss_dsi_poc_ctrl(vdd, POC_OP_WRITE, NULL);
	if (ret) {
		LCD_INFO(vdd, "fail to write poc (%d)\n", ret);
		return ret;
	}

	return count;
}


static ssize_t ss_dsi_poc_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	struct miscdevice *c = file->private_data;
	struct dsi_display *display = dev_get_drvdata(c->parent);
	struct dsi_panel *panel = display->panel;
	struct samsung_display_driver_data *vdd = panel->panel_private;
	int ret = 0;

	vdd->poc_driver.wr_try_cnt++;

	ret = _ss_dsi_poc_write(vdd, buf, count, ppos);
	if (ret < 0) {
		LCD_INFO(vdd, "fail to poc write..\n");
		vdd->poc_driver.wr_fail_cnt++;
	}

	return ret;
}
static const struct file_operations poc_fops = {
	.owner = THIS_MODULE,
	.read = ss_dsi_poc_read,
	.write = ss_dsi_poc_write,
	.unlocked_ioctl = ss_dsi_poc_ioctl,
	.open = ss_dsi_poc_open,
	.release = ss_dsi_poc_release,
	.llseek = generic_file_llseek,
};

#define EPOCEFS_IMGIDX (100)
enum {
	EPOCEFS_NOENT = 1,		/* No such file or directory */
	EPOCEFS_EMPTY = 2,		/* Empty file */
	EPOCEFS_READ = 3,		/* Read failed */
	MAX_EPOCEFS,
};

#define POC_TOTAL_TRY_COUNT_FILE_PATH	("/efs/afc/apply_count")
#define POC_TOTAL_FAIL_COUNT_FILE_PATH	("/efs/afc/fail_count")
#define POC_INFO_FILE_PATH	("/efs/FactoryApp/poc_info")
#define POC_USER_FILE_PATH	("/efs/FactoryApp/poc_user")

__visible_for_testing int poc_dpui_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct POC *poc = container_of(self, struct POC, dpui_notif);
	struct samsung_display_driver_data *vdd = container_of(poc, struct samsung_display_driver_data, poc_driver);
	char tbuf[MAX_DPUI_VAL_LEN];
	int total_fail_cnt = 0;
	int total_try_cnt = 0;
	int size = 0, poci = 0, poci_org = 0;

	if (poc == NULL) {
		LCD_INFO(vdd, "err: poc is null\n");
		return -EINVAL;
	}

	if (!vdd->poc_driver.is_support) {
		LCD_INFO(vdd, "Not Support POC Driver!\n");
		return -ENODEV;
	}

	size = scnprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", total_try_cnt);
	set_dpui_field(DPUI_KEY_PNPOCT, tbuf, size);

	size = scnprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", total_fail_cnt);
	set_dpui_field(DPUI_KEY_PNPOCF, tbuf, size);

	size = scnprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", poci_org);
	set_dpui_field(DPUI_KEY_PNPOCI_ORG, tbuf, size);

	size = scnprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", poci);
	set_dpui_field(DPUI_KEY_PNPOCI, tbuf, size);

	inc_dpui_u32_field(DPUI_KEY_PNPOC_ER_TRY, poc->er_try_cnt);
	inc_dpui_u32_field(DPUI_KEY_PNPOC_ER_FAIL, poc->er_fail_cnt);
	inc_dpui_u32_field(DPUI_KEY_PNPOC_WR_TRY, poc->wr_try_cnt);
	inc_dpui_u32_field(DPUI_KEY_PNPOC_WR_FAIL, poc->wr_fail_cnt);
	inc_dpui_u32_field(DPUI_KEY_PNPOC_RD_TRY, poc->rd_try_cnt);
	inc_dpui_u32_field(DPUI_KEY_PNPOC_RD_FAIL, poc->rd_fail_cnt);

	LCD_INFO(vdd,  "poc dpui: try=%d, fail=%d, id=%d, %d\n",
			total_try_cnt, total_fail_cnt, poci, poci_org);
	LCD_INFO(vdd,  "poc dpui: er (%d/%d), wr (%d/%d), rd (%d/%d)\n",
			poc->er_try_cnt, poc->er_fail_cnt,
			poc->wr_try_cnt, poc->wr_fail_cnt,
			poc->rd_try_cnt, poc->rd_fail_cnt);

	poc->er_try_cnt = 0;
	poc->er_fail_cnt = 0;
	poc->wr_try_cnt = 0;
	poc->wr_fail_cnt = 0;
	poc->rd_try_cnt = 0;
	poc->rd_fail_cnt = 0;

	return 0;
}

static int ss_dsi_poc_register_dpui(struct POC *poc)
{
	memset(&poc->dpui_notif, 0,
			sizeof(poc->dpui_notif));
	poc->dpui_notif.notifier_call = poc_dpui_notifier_callback;

	return dpui_logging_register(&poc->dpui_notif, DPUI_TYPE_PANEL);
}

#define POC_DEV_NAME_SIZE 10
int ss_dsi_poc_init(struct samsung_display_driver_data *vdd)
{
	int ret;
	static char devname[POC_DEV_NAME_SIZE] = {'\0', };

	struct dsi_panel *panel = NULL;
	struct mipi_dsi_host *host = NULL;
	struct dsi_display *display = NULL;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd");
		return -ENODEV;
	}

	if (!vdd->poc_driver.is_support) {
		LCD_INFO(vdd, "Not Support POC Driver!\n");
		return -ENODEV;
	}

	LCD_INFO(vdd,  "++\n");

	panel = (struct dsi_panel *)vdd->msm_private;
	host = panel->mipi_device.host;
	display = container_of(host, struct dsi_display, host);

	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		snprintf(devname, POC_DEV_NAME_SIZE, "poc");
	else
		snprintf(devname, POC_DEV_NAME_SIZE, "poc%d", vdd->ndx);

	vdd->poc_driver.dev.minor = MISC_DYNAMIC_MINOR;
	vdd->poc_driver.dev.name = devname;
	vdd->poc_driver.dev.fops = &poc_fops;
	vdd->poc_driver.dev.parent = &display->pdev->dev;

	vdd->poc_driver.wbuf = NULL;
	vdd->poc_driver.rbuf = NULL;
	atomic_set(&vdd->poc_driver.cancel, 0);

	/* Drvier level big data for POC operation */
	vdd->poc_driver.er_try_cnt = 0;
	vdd->poc_driver.er_fail_cnt = 0;
	vdd->poc_driver.wr_try_cnt = 0;
	vdd->poc_driver.wr_fail_cnt = 0;
	vdd->poc_driver.rd_try_cnt = 0;
	vdd->poc_driver.rd_fail_cnt = 0;

	vdd->panel_func.samsung_poc_ctrl = ss_dsi_poc_ctrl;

	ret = ss_wrapper_misc_register(vdd, &vdd->poc_driver.dev);
	if (ret) {
		LCD_INFO(vdd, "failed to register POC driver : %d\n", ret);
		return ret;
	}

	ss_dsi_poc_register_dpui(&vdd->poc_driver);

	LCD_INFO(vdd,  "--\n");
	return ret;
}
