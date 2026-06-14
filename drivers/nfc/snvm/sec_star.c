/*
 * Copyright (C) 2020 Samsung Electronics. All rights reserved.
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
 * along with this program;
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/ioctl.h>

#include "hal/ese_hal.h"
#include "protocol/ese_memory.h"
#include "protocol/ese_iso7816_t1.h"
#include "sec_star.h"

#define STAR_VERSION "STAR00010000"

#define ERR(msg...)		pr_err("[star] : " msg)
#define INFO(msg...)	pr_info("[star] : " msg)

#define BIGENDIAN_TO_UINT32(x, y) \
{ \
	y = ((x)[0] << 24) | ((x)[1] << 16) | ((x)[2] << 8) | (x)[3]; \
}

#define SEND_ADDRESS			0x12
#define RECEIVE_ADDRESS			0x21

#define APDU_CHAIN_NUM_SIZE		4
#define APDU_CHAIN_SEQ_SIZE		4
#define APDU_CHAIN_CMD_SIZE		4
#define APDU_CHAIN_EXP_SIZE		4
#define APDU_CHAIN_HEADER_SIZE	(APDU_CHAIN_SEQ_SIZE + APDU_CHAIN_CMD_SIZE + APDU_CHAIN_EXP_SIZE)

#define APDU_CHAIN_EXP_FLAG		4

#define EXPECTED_RESPONSE_NONE	0x0
#define EXPECTED_RESPONSE_NEXT	0x1
#define EXPECTED_RESPONSE_AGAIN	0x2

#define STAR_MAGIC_CODE			'S'
#define STAR_READ_SIZE			_IOR(STAR_MAGIC_CODE, 0, unsigned int)
#define STAR_SET_DIRECT			_IOW(STAR_MAGIC_CODE, 1, int)
#define STAR_RESET_PROTOCOL		_IO(STAR_MAGIC_CODE, 2)
#define STAR_RESET_INTERFACE	_IO(STAR_MAGIC_CODE, 3)

#define APDU_CHAIN_MAX_SIZE		((65536/256) + 1)

static int32_t star_transceive(void *ctx, uint8_t *cmd, uint32_t cmd_size, uint8_t **rsp, uint32_t *rsp_size)
{
	ese_data_t cmd_data = {0, NULL};
	ese_data_t rsp_data = {0, NULL};
	uint8_t *p_cmd = NULL;
	uint32_t chain_num = 0;
	uint32_t seq = 0;
	uint32_t data_size = 0;
	uint32_t expected_flag = 0;
	uint32_t expected_size = 0;
	uint32_t i = 0;
	data_list_t total_rsp;

	if (ctx == NULL) {
		return -1;
	}

	if ((cmd == NULL) || (cmd_size == 0) || (rsp == NULL) || (rsp_size == NULL)) {
		ERR("invalid parameter or no data");
		return -1;
	}

	if (cmd_size < APDU_CHAIN_NUM_SIZE) {
		return -1;
	}

	p_cmd = cmd;
	BIGENDIAN_TO_UINT32(p_cmd, chain_num);
	p_cmd += APDU_CHAIN_NUM_SIZE;
	cmd_size -= APDU_CHAIN_NUM_SIZE;

	if (chain_num == 0 || chain_num > APDU_CHAIN_MAX_SIZE) {
		return -1;
	}

	ese_data_init(&total_rsp);
	for (i = 0; i < chain_num; i++) {
		if (cmd_size <= APDU_CHAIN_HEADER_SIZE) {
			ERR("invalid command chain header size");
			goto error;
		}

		BIGENDIAN_TO_UINT32(p_cmd, seq);
		p_cmd += APDU_CHAIN_SEQ_SIZE;
		if (seq != i) {
			ERR("invalid sequence number");
			goto error;
		}

		BIGENDIAN_TO_UINT32(p_cmd, data_size);
		p_cmd += APDU_CHAIN_CMD_SIZE;
		BIGENDIAN_TO_UINT32(p_cmd, expected_size);
		p_cmd += APDU_CHAIN_EXP_SIZE;
		cmd_size -= APDU_CHAIN_HEADER_SIZE;
		if (cmd_size < (data_size + expected_size) || data_size > (data_size + expected_size)) {
			ERR("invalid send data or expected data size");
			goto error;
		}

		cmd_data.data = p_cmd;
		cmd_data.size = data_size;
		rsp_data.data = NULL;
		rsp_data.size = 0;
		p_cmd += data_size;
		cmd_size -= data_size;

again:
		if (iso7816_t1_send(ctx, &cmd_data) != ESESTATUS_SUCCESS) {
			ERR("failed to send apdu");
			goto error;
		}

		if (iso7816_t1_receive(ctx, &rsp_data) != ESESTATUS_SUCCESS) {
			ERR("failed to receive response");
			if(rsp_data.data) {
				ESE_FREE(rsp_data.data);
			}
			goto error;
		}

		if (expected_size > 0) {
			BIGENDIAN_TO_UINT32(p_cmd, expected_flag);
			if (rsp_data.size < (expected_size - APDU_CHAIN_EXP_FLAG) ||
					memcmp(p_cmd + APDU_CHAIN_EXP_FLAG,
							rsp_data.data + (rsp_data.size - (expected_size - APDU_CHAIN_EXP_FLAG)),
							(expected_size - APDU_CHAIN_EXP_FLAG)) != 0) {
				if (ese_data_store(&total_rsp, rsp_data.data, rsp_data.size, 0) != ESESTATUS_SUCCESS) {
					goto error;
				}
				goto exit;
			}

			if (expected_flag & EXPECTED_RESPONSE_AGAIN) {
				if (rsp_data.size > 2) {
					if (ese_data_store(&total_rsp, rsp_data.data, rsp_data.size - 2, 0) != ESESTATUS_SUCCESS) {
						goto error;
					}
				} else {
					ESE_FREE(rsp_data.data);
				}
				goto again;
			}

			p_cmd += expected_size;
			cmd_size -= expected_size;
		}

		if (i < (chain_num - 1)) {
			if (rsp_data.size > 2) {
				if (ese_data_store(&total_rsp, rsp_data.data, rsp_data.size - 2, 0) != ESESTATUS_SUCCESS) {
					goto error;
				}
			} else {
				ESE_FREE(rsp_data.data);
			}
		} else {
			if (ese_data_store(&total_rsp, rsp_data.data, rsp_data.size, 0) != ESESTATUS_SUCCESS) {
				goto error;
			}
		}
	}

exit:
	ese_data_get(&total_rsp, rsp, rsp_size);
	return 0;
error:
	ese_data_delete(&total_rsp);
	return -1;
}

static int star_dev_open(struct inode *inode, struct file *filp)
{
	sec_star_t *star = container_of(filp->private_data,
			sec_star_t, misc);
	int ret = 0;

	INFO("star_open\n");

	mutex_lock(&(star->lock));

	filp->private_data = star;

	if (star->access == 0) {
#ifdef FEATURE_STAR_WAKELOCK
		if (!wake_lock_active(&star->snvm_wake_lock)) {
			wake_lock(&star->snvm_wake_lock);
			INFO("called to snvm_wake_lock\n");
		}
#endif
		iso7816_t1_reset(star->protocol);

		ret = star->dev->power_on();
		if (ret < 0) {
#ifdef FEATURE_STAR_WAKELOCK
			if (wake_lock_active(&star->snvm_wake_lock)) {
				wake_unlock(&star->snvm_wake_lock);
				INFO("called to snvm_wake_unlock\n");
			}
#endif
			ERR("%s :failed to open star", __func__);
			mutex_unlock(&(star->lock));
			return ret;
		}
	}

	star->access++;

	mutex_unlock(&(star->lock));
	return 0;
}

static int star_dev_close(struct inode *inode, struct file *filp)
{
	sec_star_t *star = (sec_star_t *)filp->private_data;
	int ret = 0;

	INFO("star_close\n");

	if (star == NULL) {
		return -EINVAL;
	}

	mutex_lock(&(star->lock));

	star->access--;

	if (star->access == 0) {
		ret = star->dev->power_off();
		if (ret < 0)
			ERR("%s :failed power_off", __func__);

#ifdef FEATURE_STAR_WAKELOCK
		if (wake_lock_active(&star->snvm_wake_lock)) {
			wake_unlock(&star->snvm_wake_lock);
			INFO("called to snvm_wake_unlock\n");
		}
#endif
	}

	mutex_unlock(&(star->lock));
	return ret;
}

static long star_dev_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	sec_star_t *star = (sec_star_t *)filp->private_data;

	if (star == NULL) {
		return -EINVAL;
	}

	if (_IOC_TYPE(cmd) != STAR_MAGIC_CODE) {
		ERR("%s invalid magic. cmd=0x%X Received=0x%X Expected=0x%X\n",
				__func__, cmd, _IOC_TYPE(cmd), STAR_MAGIC_CODE);
		return -ENOTTY;
	}

	mutex_lock(&(star->lock));

	switch (cmd) {
		case STAR_READ_SIZE:
			INFO("%s read size : %u\n", __func__, star->rsp_size);
			put_user(star->rsp_size, (unsigned int __user *)arg);
			break;
		case STAR_SET_DIRECT:
			get_user(star->direct, (int __user *)arg);
			INFO("%s set direct : %d\n", __func__, star->direct);
			break;
		case STAR_RESET_PROTOCOL:
			INFO("%s reset protocol\n", __func__);
			iso7816_t1_reset(star->protocol);
			break;
		case STAR_RESET_INTERFACE:
			INFO("%s reset interface\n", __func__);
			star->dev->reset();
			iso7816_t1_reset(star->protocol);
			break;
		default:
			INFO("%s no matching ioctl! 0x%X\n", __func__, cmd);
			mutex_unlock(&(star->lock));
			return -EINVAL;
	}

	mutex_unlock(&(star->lock));
	return 0;
}

static ssize_t star_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
	sec_star_t *star = (sec_star_t *)filp->private_data;
	uint8_t *cmd = NULL;
	uint32_t cmd_size = 0;
	uint8_t *rsp = NULL;
	uint32_t rsp_size = 0;
	int ret = -EIO;

	if (star == NULL || count == 0) {
		return -EINVAL;
	}

	mutex_lock(&(star->lock));

	if (star->rsp != NULL && star->rsp_size > 0) {
		ESE_FREE(star->rsp);
		star->rsp = NULL;
		star->rsp_size = 0;
	}

	cmd_size = (uint32_t)count;
	cmd = ESE_MALLOC(cmd_size);
	if (cmd == NULL) {
		ERR("failed to allocate for i2c buf\n");
		mutex_unlock(&(star->lock));
		return -ENOMEM;
	}

	if (copy_from_user(cmd, (void __user *)buf, cmd_size) > 0) {
		ERR("%s: failed to copy from user space\n", __func__);
		ret = -EFAULT;
		goto error;
	}

	if (star->direct > 0) {
		if (ese_hal_send(star->hal, cmd, cmd_size) < 0) {
			ERR("i2c_master_send failed\n");
			ret = -EIO;
			goto error;
		}
	} else {
		if (star_transceive(star->protocol, cmd, cmd_size, &rsp, &rsp_size) != ESESTATUS_SUCCESS) {
			ERR("%s: failed to ese_transceive_chain\n", __func__);
			ret = -EIO;
			goto error;
		}
	}

	star->rsp = rsp;
	star->rsp_size = rsp_size;
	ret = (int)cmd_size;
error:
	ESE_FREE(cmd);
	mutex_unlock(&(star->lock));
	INFO("%s: count:%zu ret:%d\n", __func__, count, ret);
	return ret;
}

static ssize_t star_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
	sec_star_t *star = (sec_star_t *)filp->private_data;
	uint8_t *tmp = NULL;

	if (star == NULL || count == 0) {
		return -EINVAL;
	}

	mutex_lock(&(star->lock));
	if (star->direct > 0) {
		tmp = ESE_MALLOC(count);
		if (tmp == NULL) {
			mutex_unlock(&(star->lock));
			return -ENOMEM;
		}

		if (ese_hal_receive(star->hal, tmp, count) < 0) {
			ERR("i2c_master_send failed\n");
			ESE_FREE(tmp);
			mutex_unlock(&(star->lock));
			return -EIO;
		}

		if (copy_to_user((void __user *)buf, tmp, count) > 0) {
			ERR("copy_to_user failed\n");
			ESE_FREE(tmp);
			mutex_unlock(&(star->lock));
			return -ENOMEM;
		}

		ESE_FREE(tmp);
	} else {
		if (star->rsp == NULL || star->rsp_size == 0) {
			mutex_unlock(&(star->lock));
			return -ENOSPC;
		}

		if (star->rsp_size != count) {
			ERR("mismatch response size\n");
			ESE_FREE(star->rsp);
			star->rsp = NULL;
			star->rsp_size = 0;
			mutex_unlock(&(star->lock));
			return -E2BIG;
		}

		if (copy_to_user((void __user *)buf, star->rsp, star->rsp_size) > 0) {
			ERR("copy_to_user failed\n");
			ESE_FREE(star->rsp);
			star->rsp = NULL;
			star->rsp_size = 0;
			mutex_unlock(&(star->lock));
			return -ENOMEM;
		}

		ESE_FREE(star->rsp);
		star->rsp = NULL;
		star->rsp_size = 0;
	}
	INFO("%s: count:%zu\n", __func__, count);
	mutex_unlock(&(star->lock));
	return (ssize_t)count;
}

static const struct file_operations star_misc_fops = {
	.owner = THIS_MODULE,
	.read = star_dev_read,
	.write = star_dev_write,
	.open = star_dev_open,
	.release = star_dev_close,
	.unlocked_ioctl = star_dev_ioctl,
};

sec_star_t *star_open(star_dev_t *dev)
{
	sec_star_t *star = NULL;
	int ret = -1;

	INFO("Version : %s\n", STAR_VERSION);
	INFO("Entry : %s\n", __func__);

	star = ESE_MALLOC(sizeof(sec_star_t));
	if (star == NULL) {
		return NULL;
	}

	star->dev = dev;
	star->protocol = NULL;
	star->rsp = NULL;
	star->rsp_size = 0;
	star->access = 0;
	star->direct = 0;

	star->hal = ese_hal_init(dev->hal_type, dev->client);
	if (star->hal == NULL) {
		ERR("%s :failed to init hal", __func__);
		ESE_FREE(star);
		return NULL;
	}

	star->protocol = iso7816_t1_init(SEND_ADDRESS, RECEIVE_ADDRESS, star->hal);
	if (star->protocol == NULL) {
		ERR("%s :failed to open protocol", __func__);
		ESE_FREE(star);
		return NULL;
	}

	mutex_init(&(star->lock));
#ifdef FEATURE_STAR_WAKELOCK
	wake_lock_init(&star->snvm_wake_lock, WAKE_LOCK_SUSPEND, "star_wake_lock");
#endif

	star->misc.minor = MISC_DYNAMIC_MINOR;
	star->misc.name = dev->name;
	star->misc.fops = &star_misc_fops;
	ret = misc_register(&(star->misc));
	if (ret < 0) {
		ERR("misc_register failed! %d\n", ret);
#ifdef FEATURE_STAR_WAKELOCK
		wake_lock_destroy(&star->snvm_wake_lock);
#endif
		mutex_destroy(&(star->lock));
		ESE_FREE(star);
		return NULL;
	}

	INFO("Exit : %s\n", __func__);
	return star;
}

void star_close(sec_star_t *star)
{
	INFO("Entry : %s\n", __func__);

	if (star == NULL) {
		return;
	}

	misc_deregister(&(star->misc));

	iso7816_t1_deinit(star->protocol);
	ese_hal_release(star->hal);
#ifdef FEATURE_STAR_WAKELOCK
	wake_lock_destroy(&star->snvm_wake_lock);
#endif
	mutex_destroy(&(star->lock));
	ESE_FREE(star);
	INFO("Exit : %s\n", __func__);
}

MODULE_AUTHOR("sec");
MODULE_DESCRIPTION("sec-star driver");
MODULE_LICENSE("GPL");
