/*
 *
 * Copyright (C) 2017-2019 Samsung Electronics
 * Author: Wookwang Lee <wookwang.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
#include <linux/compat.h>
#include <linux/usb/typec/common/pdic_core.h>

#define MAX_BUF 255
#define MAX_FW_SIZE (1024*1024)
#define NODE_OF_MISC "ccic_misc"
#define NODE_OF_UMS "pdic_fwupdate"
#define PDIC_IOCTL_UVDM _IOWR('C', 0, struct uvdm_data)
#ifdef CONFIG_COMPAT
#define PDIC_IOCTL_UVDM_32 _IOWR('C', 0, struct uvdm_data_32)
#endif

static struct pdic_misc_core *p_m_core;

int get_checksum(const char *data, int start_addr, int size)
{
	int pos, checksum = 0;

	if (!data || start_addr < 0 || size < 0
		|| start_addr > MAX_BUF_DATA || size > MAX_BUF_DATA)
		return -EINVAL;

	for (pos = 0; pos < size; pos++)
		checksum += data[start_addr + pos];

	return checksum;
}
EXPORT_SYMBOL(get_checksum);

int get_data_size(bool is_first_data, int data_size)
{
	int max_size = SEC_UVDM_MAXDATA_NORMAL;

	if (data_size < 0)
		return -EINVAL;

	if (is_first_data)
		max_size = SEC_UVDM_MAXDATA_FIRST;

	return data_size <= max_size ? data_size : max_size;
}
EXPORT_SYMBOL(get_data_size);

int set_endian(const char *src, char *dest, int size)
{
	int loop, cnt, total_loop, src_pos, dest_pos;

	if (!src || !dest || size < 0 || size > MAX_BUF_DATA)
		return -EINVAL;

	total_loop = size / SEC_UVDM_ALIGN;
	if (size % SEC_UVDM_ALIGN)
		total_loop++;

	memset(dest, 0, total_loop * SEC_UVDM_ALIGN);

	for (loop = 0 ; loop < total_loop ; loop++) {
		for (cnt = 0 ; cnt < SEC_UVDM_ALIGN ; cnt++) {
			src_pos = SEC_UVDM_ALIGN * loop + cnt;
			dest_pos = SEC_UVDM_ALIGN * loop + SEC_UVDM_ALIGN - cnt - 1;
			dest[dest_pos] = src[src_pos];
		}
	}
	return 0;
}
EXPORT_SYMBOL(set_endian);

int set_uvdmset_count(int size)
{
	int cnt, len = size - SEC_UVDM_MAXDATA_FIRST;

	if (size < 0)
		return -EINVAL;

	if (len <= 0)
		return 1;

	cnt = len / SEC_UVDM_MAXDATA_NORMAL;

	if (len % SEC_UVDM_MAXDATA_NORMAL == 0)
		return cnt + 1;

	return cnt + 2;
}
EXPORT_SYMBOL(set_uvdmset_count);

void set_msg_header(void *data, int msg_type, int obj_num)
{
	msg_header_type *msg_hdr;
	uint8_t *SendMSG = (uint8_t *)data;

	msg_hdr = (msg_header_type *)&SendMSG[0];
	msg_hdr->msg_type = msg_type;
	msg_hdr->num_data_objs = obj_num;
	msg_hdr->port_data_role = USBPD_DFP;
}
EXPORT_SYMBOL(set_msg_header);

void set_uvdm_header(void *data, int vid, int vdm_type)
{
	uvdm_header *uvdm_hdr;
	uint8_t *SendMSG = (uint8_t *)data;

	uvdm_hdr = (uvdm_header *)&SendMSG[0];
	uvdm_hdr->vendor_id = SAMSUNG_VENDOR_ID;
	uvdm_hdr->vdm_type = vdm_type;
	uvdm_hdr->vendor_defined = SEC_UVDM_UNSTRUCTURED_VDM;
	uvdm_hdr->BITS.VDM_command = 4; /* from s2mm005 concept */
}
EXPORT_SYMBOL(set_uvdm_header);

void set_sec_uvdm_header(void *data, int pid, bool data_type, int cmd_type,
		bool dir, int total_set_num, uint8_t received_data)
{
	s_uvdm_header *SEC_UVDM_HEADER;
	uint8_t *SendMSG = (uint8_t *)data;

	SEC_UVDM_HEADER = (s_uvdm_header *)&SendMSG[4];

	SEC_UVDM_HEADER->pid = pid;
	SEC_UVDM_HEADER->data_type = data_type;
	SEC_UVDM_HEADER->cmd_type = cmd_type;
	SEC_UVDM_HEADER->direction = dir;
	SEC_UVDM_HEADER->total_set_num = total_set_num;
	SEC_UVDM_HEADER->data = received_data;

	pr_info("%s : pid=0x%x, data_type=%d, cmd_type=%d, dir=%d\n", __func__,
		SEC_UVDM_HEADER->pid, SEC_UVDM_HEADER->data_type,
		SEC_UVDM_HEADER->cmd_type, SEC_UVDM_HEADER->direction);
}
EXPORT_SYMBOL(set_sec_uvdm_header);

void set_sec_uvdm_tx_header(void *data,
		int first_set, int cur_set, int total_size, int remained_size)
{
	s_tx_header *SEC_TX_HAEDER;
	uint8_t *SendMSG = (uint8_t *)data;

	if (first_set)
		SEC_TX_HAEDER = (s_tx_header *)&SendMSG[8];
	else
		SEC_TX_HAEDER = (s_tx_header *)&SendMSG[4];
	SEC_TX_HAEDER->cur_size = get_data_size(first_set, remained_size);
	SEC_TX_HAEDER->total_size = total_size;
	SEC_TX_HAEDER->order_cur_set = cur_set;
}
EXPORT_SYMBOL(set_sec_uvdm_tx_header);

void set_sec_uvdm_tx_tailer(void *data)
{
	s_tx_tailer *SEC_TX_TAILER;
	uint8_t *SendMSG = (uint8_t *)data;

	SEC_TX_TAILER = (s_tx_tailer *)&SendMSG[24];
	SEC_TX_TAILER->checksum =
		get_checksum(SendMSG, 4, SEC_UVDM_CHECKSUM_COUNT);
}
EXPORT_SYMBOL(set_sec_uvdm_tx_tailer);

void set_sec_uvdm_rx_header(void *data, int cur_num, int cur_set, int ack)
{
	s_rx_header *SEC_RX_HEADER;
	uint8_t *SendMSG = (uint8_t *)data;

	SEC_RX_HEADER = (s_rx_header *)&SendMSG[4];
	SEC_RX_HEADER->order_cur_set = cur_num;
	SEC_RX_HEADER->rcv_data_size = cur_set;
	SEC_RX_HEADER->result_value = ack;
}
EXPORT_SYMBOL(set_sec_uvdm_rx_header);

struct pdic_misc_dev *get_pdic_misc_dev(void)
{
	struct pdic_misc_dev *c_dev;

	if (!p_m_core)
		return NULL;
	c_dev = &p_m_core->c_dev;
	return c_dev;
}
EXPORT_SYMBOL(get_pdic_misc_dev);

static inline int _lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1)
		return 0;
	else {
		atomic_dec(excl);
		return -1;
	}
}

static inline void _unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

static int pdic_misc_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	pr_info("%s + open success\n", __func__);
	if (!p_m_core) {
		pr_err("%s - error : p_m_core is NULL\n", __func__);
		ret = -ENODEV;
		goto err;
	}

	if (_lock(&p_m_core->c_dev.open_excl)) {
		pr_err("%s - error : device busy\n", __func__);
		ret = -EBUSY;
		goto err;
	}

	/* stop direct charging(pps) for uvdm and wait latest psrdy done for 1 second */
	if (p_m_core->c_dev.pps_control) {
		if (!p_m_core->c_dev.pps_control(0)) {
			_unlock(&p_m_core->c_dev.open_excl);
			pr_err("%s - error : psrdy is not done\n", __func__);
			ret = -EBUSY;
			goto err;
		}
	}

	/* check if there is some connection */
	if (!p_m_core->c_dev.uvdm_ready ||
			!p_m_core->c_dev.uvdm_ready()) {
		_unlock(&p_m_core->c_dev.open_excl);
		pr_err("%s - error : uvdm is not ready\n", __func__);
		ret = -EBUSY;
		goto err;
	}

	pr_info("%s - open success\n", __func__);

	return 0;
err:
	return ret;
}

static int pdic_misc_close(struct inode *inode, struct file *file)
{
	if (!p_m_core) {
		pr_err("%s - error : p_m_core is NULL\n", __func__);
		return -ENODEV;
	}

	if (p_m_core)
		_unlock(&p_m_core->c_dev.open_excl);
	p_m_core->c_dev.uvdm_close();
	if (p_m_core->c_dev.pps_control)
		p_m_core->c_dev.pps_control(1); /* start direct charging(pps) */

	pr_info("%s - close success\n", __func__);
	return 0;
}

static int send_uvdm_message(void *data, int size)
{
	int ret = 0;

	if (!p_m_core) {
		ret = -ENODEV;
		return ret;
	}

	if (p_m_core->c_dev.uvdm_write)
		ret = p_m_core->c_dev.uvdm_write(data, size);

	pr_info("%s - size : %d, ret : %d\n", __func__, size, ret);
	return ret;
}

static int receive_uvdm_message(void *data, int size)
{
	int ret = 0;

	if (!p_m_core) {
		ret = -ENODEV;
		return ret;
	}

	if (p_m_core->c_dev.uvdm_read)
		ret = p_m_core->c_dev.uvdm_read(data);

	pr_info("%s - size : %d, ret : %d\n", __func__, size, ret);
	return ret;
}

static long
pdic_misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	void *buf = NULL;

	if (_lock(&p_m_core->c_dev.ioctl_excl)) {
		pr_err("%s - error : ioctl busy - cmd : %d\n", __func__, cmd);
		ret = -EBUSY;
		goto err2;
	}

	if (!p_m_core->c_dev.uvdm_ready()) {
		pr_err("%s - error : uvdm is not ready\n", __func__);
		ret = -EACCES;
		goto err1;
	}

	switch (cmd) {
	case PDIC_IOCTL_UVDM:
		pr_info("%s - PDIC_IOCTL_UVDM cmd\n", __func__);
		if (copy_from_user(&p_m_core->c_dev.u_data,
				(void __user *) arg, sizeof(struct uvdm_data))) {
			ret = -EIO;
			pr_err("%s - copy_from_user error\n", __func__);
			goto err1;
		}

		buf = kzalloc(MAX_BUF, GFP_KERNEL);
		if (!buf) {
			ret = -EINVAL;
			pr_err("%s - kzalloc error\n", __func__);
			goto err1;
		}

		if (p_m_core->c_dev.u_data.size > MAX_BUF) {
			ret = -ENOMEM;
			pr_err("%s - user data size is %d error\n",
					__func__, p_m_core->c_dev.u_data.size);
			goto err;
		}

		if (p_m_core->c_dev.u_data.dir == DIR_OUT) {
			if (copy_from_user(buf, p_m_core->c_dev.u_data.pData, p_m_core->c_dev.u_data.size)) {
				ret = -EIO;
				pr_err("%s - copy_from_user error\n", __func__);
				goto err;
			}
			ret = send_uvdm_message(buf, p_m_core->c_dev.u_data.size);
			if (ret < 0) {
				pr_err("%s - send_uvdm_message error\n", __func__);
				ret = -EINVAL;
				goto err;
			}
		} else {
			ret = receive_uvdm_message(buf, p_m_core->c_dev.u_data.size);
			if (ret < 0) {
				pr_err("%s - receive_uvdm_message error\n", __func__);
				ret = -EINVAL;
				goto err;
			}
			if (copy_to_user((void __user *)p_m_core->c_dev.u_data.pData,
					buf, ret)) {
				ret = -EIO;
				pr_err("%s - copy_to_user error\n", __func__);
				goto err;
			}
		}
		break;
#ifdef CONFIG_COMPAT
	case PDIC_IOCTL_UVDM_32:
		pr_info("%s - PDIC_IOCTL_UVDM_32 cmd\n", __func__);
		if (copy_from_user(&p_m_core->c_dev.u_data_32, compat_ptr(arg),
				sizeof(struct uvdm_data_32))) {
			ret = -EIO;
			pr_err("%s - copy_from_user error\n", __func__);
			goto err1;
		}

		buf = kzalloc(MAX_BUF, GFP_KERNEL);
		if (!buf) {
			ret = -EINVAL;
			pr_err("%s - kzalloc error\n", __func__);
			goto err1;
		}

		if (p_m_core->c_dev.u_data_32.size > MAX_BUF) {
			ret = -ENOMEM;
			pr_err("%s - user data size is %d error\n", __func__, p_m_core->c_dev.u_data_32.size);
			goto err;
		}

		if (p_m_core->c_dev.u_data_32.dir == DIR_OUT) {
			if (copy_from_user(buf, compat_ptr(p_m_core->c_dev.u_data_32.pData),
								p_m_core->c_dev.u_data_32.size)) {
				ret = -EIO;
				pr_err("%s - copy_from_user error\n", __func__);
				goto err;
			}
			ret = send_uvdm_message(buf, p_m_core->c_dev.u_data_32.size);
			if (ret < 0) {
				pr_err("%s - send_uvdm_message error\n", __func__);
				ret = -EINVAL;
				goto err;
			}
		} else {
			ret = receive_uvdm_message(buf, p_m_core->c_dev.u_data_32.size);
			if (ret < 0) {
				pr_err("%s - receive_uvdm_message error\n", __func__);
				ret = -EINVAL;
				goto err;
			}
			if (copy_to_user(compat_ptr(p_m_core->c_dev.u_data_32.pData),
						buf, ret)) {
				ret = -EIO;
				pr_err("%s - copy_to_user error\n", __func__);
				goto err;
			}
		}
		break;
#endif
	default:
		pr_err("%s - unknown ioctl cmd : %d\n", __func__, cmd);
		ret = -ENOIOCTLCMD;
		goto err;
	}
err:
	kfree(buf);
err1:
	_unlock(&p_m_core->c_dev.ioctl_excl);
err2:
	return ret;
}

#ifdef CONFIG_COMPAT
static long
pdic_misc_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	pr_info("%s - cmd : %d\n", __func__, cmd);
	ret = pdic_misc_ioctl(file, cmd, (unsigned long)compat_ptr(arg));

	return ret;
}
#endif

static const struct file_operations pdic_misc_fops = {
	.owner		= THIS_MODULE,
	.open		= pdic_misc_open,
	.release	= pdic_misc_close,
	.llseek		= no_llseek,
	.unlocked_ioctl = pdic_misc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = pdic_misc_compat_ioctl,
#endif
};

static int ums_update_open(struct inode *ip, struct file *fp)
{
	struct pdic_fwupdate_data *fw_data;
	size_t p_fw_size = 0;
	void *fw_buf, *misc_data;
	int ret = 0;

	pr_info("%s +\n", __func__);

	if (!p_m_core) {
		pr_err("%s - error : p_m_core is NULL\n", __func__);
		ret = -ENODEV;
		goto err;
	}

	fw_data = &p_m_core->fw_data;

	if (_lock(&fw_data->opened)) {
		pr_err("%s - error : device busy\n", __func__);
		ret = -EBUSY;
		goto err;
	}

	if (fw_data->ic_data->get_prev_fw_size)
		p_fw_size = fw_data->ic_data->get_prev_fw_size
								(fw_data->ic_data->data);

	if (p_fw_size <= 0 || p_fw_size > MAX_FW_SIZE) {
		ret = -EFAULT;
		pr_err("%s p_fw_size is %lu error\n", __func__, p_fw_size);
		goto err1;
	}

	/* alloc fw size +20% and align PAGE_SIZE */
	p_fw_size += p_fw_size/5;
	p_fw_size = (p_fw_size/PAGE_SIZE)*PAGE_SIZE;

	pr_info("%s fw_buf size=%lu\n", __func__, p_fw_size);

	misc_data = kzalloc(sizeof(struct pdic_misc_data), GFP_KERNEL);
	if (!misc_data) {
		ret = -ENOMEM;
		goto err1;
	}

	fw_buf = vzalloc(p_fw_size);
	if (!fw_buf) {
		ret = -ENOMEM;
		goto err2;
	}

	fw_data->misc_data = misc_data;
	fw_data->misc_data->fw_buf = fw_buf;
	fw_data->misc_data->offset = 0;
	fw_data->misc_data->fw_buf_size = p_fw_size;

	fp->private_data = fw_data;
	pr_info("%s -\n", __func__);
	return 0;
err2:
	kfree(misc_data);
err1:
	_unlock(&fw_data->opened);
err:
	pr_info("%s error -\n", __func__);
	return ret;
}

static int ums_update_close(struct inode *ip, struct file *fp)
{
	struct pdic_fwupdate_data *fw_data;
	void *fw_buf;
	size_t fw_size = 0;
	int fw_error = 0;
	int ret = 0;

	pr_info("%s +\n", __func__);

	fw_data = fp->private_data;

	fw_buf = fw_data->misc_data->fw_buf;
	fw_size = fw_data->misc_data->offset;
	fw_error = fw_data->misc_data->is_error;

	pr_info("firmware_update fw_size=%lu fw_error=%d\n",
		fw_size, fw_error);

	if (!fw_error && fw_data->ic_data->firmware_update) {
		fw_data->ic_data->firmware_update(fw_data->ic_data->data,
			fw_buf, fw_size);
	}

	vfree(fw_data->misc_data->fw_buf);
	kfree(fw_data->misc_data);

	_unlock(&fw_data->opened);

	pr_info("%s -\n", __func__);
	return ret;
}

static ssize_t ums_update_read(struct file *fp, char __user *buf,
	size_t count, loff_t *pos)
{
	ssize_t ret = count;

	pr_info("%s\n", __func__);
	return ret;
}

static ssize_t ums_update_write(struct file *fp, const char __user *buf,
	size_t count, loff_t *pos)
{
	struct pdic_fwupdate_data *fw_data = fp->private_data;
	ssize_t ret = count;
	void *fw_buf;
	void *fw_partial_buf;
	size_t fw_partial_size = 0;
	size_t fw_offset = 0;

	pr_info("%s start\n", __func__);
	fw_partial_size = count;

	if (fw_partial_size <= 0) {
		pr_err("%s count=%zu\n", __func__, count);
		ret = -EFAULT;
		goto err;
	}

	fw_partial_buf = kzalloc(count, GFP_KERNEL);
	if (!fw_partial_buf) {
		ret = -ENOMEM;
		goto err;
	}

	if (copy_from_user(fw_partial_buf, buf, fw_partial_size)) {
		pr_err("%s copy_from_user error.\n", __func__);
		ret = -EFAULT;
		goto err1;
	}

	fw_buf = fw_data->misc_data->fw_buf;
	fw_offset = fw_data->misc_data->offset;
	if (fw_offset + fw_partial_size > fw_data->misc_data->fw_buf_size) {
		pr_err("%s buf size=%lu overrun error\n",
			__func__, fw_offset + fw_partial_size);
		ret = -EFAULT;
		goto err1;
	}
	memcpy(fw_buf + fw_offset, fw_partial_buf, fw_partial_size);

	fw_data->misc_data->offset += fw_partial_size;

err1:
	kfree(fw_partial_buf);
err:
	if (ret < 0)
		fw_data->misc_data->is_error = 1;

	pr_info("%s end\n", __func__);
	return ret;
}

static const struct file_operations ums_update_fops = {
	.owner		= THIS_MODULE,
	.open		= ums_update_open,
	.release	= ums_update_close,
	.read		= ums_update_read,
	.write		= ums_update_write,
};

static struct miscdevice pdic_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name	= NODE_OF_MISC,
	.fops	= &pdic_misc_fops,
};

static struct miscdevice ums_update_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name	= NODE_OF_UMS,
	.fops	= &ums_update_fops,
};

int pdic_misc_init(ppdic_data_t ppdic_data)
{
	int ret = 0;

	p_m_core = kzalloc(sizeof(struct pdic_misc_core), GFP_KERNEL);
	if (!p_m_core) {
		ret = -ENOMEM;
		pr_err("%s - kzalloc failed : %d\n", __func__, ret);
		goto err;
	}
	atomic_set(&p_m_core->c_dev.open_excl, 0);
	atomic_set(&p_m_core->c_dev.ioctl_excl, 0);
	atomic_set(&p_m_core->fw_data.opened, 0);

	if (ppdic_data) {
		ppdic_data->misc_dev = &p_m_core->c_dev;
		p_m_core->fw_data.ic_data = &ppdic_data->fw_data;
	} else {
		ret = -ENOENT;
		pr_err("%s - ppdic_data error : %d\n", __func__, ret);
		goto err1;
	}

	ret = misc_register(&pdic_misc_device);
	if (ret) {
		pr_err("%s - pdic_misc return error : %d\n",
			__func__, ret);
		goto err1;
	}

	if (p_m_core->fw_data.ic_data->firmware_update) {
		ret = misc_register(&ums_update_device);
		if (ret) {
			pr_err("%s - ums_update return error : %d\n",
				__func__, ret);
			goto err2;
		}
	}

	pr_info("%s - register success\n", __func__);
	return 0;
err2:
	misc_deregister(&pdic_misc_device);
err1:
	kfree(p_m_core);
err:
	return ret;
}
EXPORT_SYMBOL(pdic_misc_init);

void pdic_misc_exit(void)
{
	pr_info("%s() called\n", __func__);
	if (p_m_core) {
		if (p_m_core->fw_data.ic_data->firmware_update)
			misc_deregister(&ums_update_device);
		misc_deregister(&pdic_misc_device);
		kfree(p_m_core);
	}
}
EXPORT_SYMBOL(pdic_misc_exit);
