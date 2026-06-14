/*
 * Copyright (C) 2018 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "fingerprint.h"
#include "qfs4008_common.h"

/*
 * struct ipc_msg_type_to_fw_event -
 *      entry in mapping between an IPC message type to a firmware event
 * @msg_type - IPC message type, as reported by firmware
 * @fw_event - corresponding firmware event code to report to driver client
 */
struct ipc_msg_type_to_fw_event {
	uint32_t msg_type;
	enum qfs4008_fw_event fw_event;
};

/* mapping between firmware IPC message types to HLOS firmware events */
struct ipc_msg_type_to_fw_event g_msg_to_event[] = {
	{IPC_MSG_ID_CBGE_REQUIRED, FW_EVENT_CBGE_REQUIRED},
	{IPC_MSG_ID_FINGER_ON_SENSOR, FW_EVENT_FINGER_DOWN},
	{IPC_MSG_ID_FINGER_OFF_SENSOR, FW_EVENT_FINGER_UP},
};

static ssize_t vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t name_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct qfs4008_drvdata *drvdata = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%s\n", drvdata->chipid);
}

static ssize_t adm_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", DETECT_ADM);
}

static ssize_t bfs_values_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct qfs4008_drvdata *drvdata = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "\"FP_SPICLK\":\"%d\"\n",
			drvdata->clk_setting->spi_speed);
}

static ssize_t type_check_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct qfs4008_drvdata *drvdata = dev_get_drvdata(dev);

	pr_info("%s\n", drvdata->sensortype > 0 ? drvdata->chipid : sensor_status[drvdata->sensortype + 2]);
	return snprintf(buf, PAGE_SIZE, "%d\n", drvdata->sensortype);
}

static ssize_t position_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct qfs4008_drvdata *drvdata = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%s\n", drvdata->sensor_position);
}

static ssize_t cbgecnt_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct qfs4008_drvdata *drvdata = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", drvdata->cbge_count);
}

static ssize_t cbgecnt_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct qfs4008_drvdata *drvdata = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "c")) {
		drvdata->cbge_count = 0;
		pr_info("initialization is done\n");
	}
	return size;
}

static ssize_t intcnt_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct qfs4008_drvdata *drvdata = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", drvdata->wuhb_count);
}

static ssize_t intcnt_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct qfs4008_drvdata *drvdata = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "c")) {
		drvdata->wuhb_count = 0;
		pr_info("initialization is done\n");
	}
	return size;
}

static ssize_t resetcnt_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct qfs4008_drvdata *drvdata = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", drvdata->reset_count);
}

static ssize_t resetcnt_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct qfs4008_drvdata *drvdata = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "c")) {
		drvdata->reset_count = 0;
		pr_info("initialization is done\n");
	}
	return size;
}

static ssize_t wuhbtest_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct qfs4008_drvdata *drvdata = dev_get_drvdata(dev);
	int rc = 0;
	int gpio_value = 0;

	gpio_value = gpio_get_value(drvdata->fd_gpio.gpio);
	if (gpio_value == 0) { /* Finger Leave */
		pr_info("wuhbtest Finger Leave Ok\n");
		rc = 1;
	} else { /* Finger Down */
		pr_err("wuhbtest Finger Leave NG\n");
		rc = 0;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", rc);
}

static DEVICE_ATTR_RO(bfs_values);
static DEVICE_ATTR_RO(type_check);
static DEVICE_ATTR_RO(vendor);
static DEVICE_ATTR_RO(name);
static DEVICE_ATTR_RO(adm);
static DEVICE_ATTR_RO(position);
static DEVICE_ATTR_RW(cbgecnt);
static DEVICE_ATTR_RW(intcnt);
static DEVICE_ATTR_RW(resetcnt);
static DEVICE_ATTR_RO(wuhbtest);

static struct device_attribute *fp_attrs[] = {
	&dev_attr_bfs_values,
	&dev_attr_type_check,
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_adm,
	&dev_attr_position,
	&dev_attr_cbgecnt,
	&dev_attr_intcnt,
	&dev_attr_resetcnt,
	&dev_attr_wuhbtest,
	NULL,
};

int qfs4008_pinctrl_register(struct qfs4008_drvdata *drvdata)
{
	drvdata->p = devm_pinctrl_get(drvdata->dev);
	if (IS_ERR(drvdata->p)) {
		pr_err("failed pinctrl_get\n");
		goto pinctrl_get_exit;
	}

#if !defined(ENABLE_SENSORS_FPRINT_SECURE) || defined(DISABLED_GPIO_PROTECTION)
	drvdata->pins_poweroff = pinctrl_lookup_state(drvdata->p, "pins_poweroff");
	if (IS_ERR(drvdata->pins_poweroff)) {
		pr_err("could not get pins sleep_state (%li)\n",
			PTR_ERR(drvdata->pins_poweroff));
		goto pinctrl_register_exit;
	}

	drvdata->pins_poweron = pinctrl_lookup_state(drvdata->p, "pins_poweron");
	if (IS_ERR(drvdata->pins_poweron)) {
		pr_err("could not get pins idle_state (%li)\n",
			PTR_ERR(drvdata->pins_poweron));
		goto pinctrl_register_exit;
	}
#endif
	pr_info("finished\n");
	return 0;

#if !defined(ENABLE_SENSORS_FPRINT_SECURE) || defined(DISABLED_GPIO_PROTECTION)
pinctrl_register_exit:
	drvdata->pins_poweron = NULL;
	drvdata->pins_poweroff = NULL;
#endif
pinctrl_get_exit:
	pr_err("failed\n");
	return -ENODEV;
}

static int qfs4008_power_control(struct qfs4008_drvdata *drvdata, int onoff)
{
	int rc = 0;

	if (regulator_is_enabled(drvdata->regulator_1p8) == onoff && 
			regulator_is_enabled(drvdata->regulator_3p0) == onoff) {
		pr_err("regulator already turned %s\n", onoff ? "on" : "off");
	} else {
		if (onoff) {
			rc = regulator_enable(drvdata->regulator_3p0);
			if (rc)
				pr_err("regulator_3p0 enable failed, rc=%d\n", rc);
			usleep_range(2000, 2050);
			rc = regulator_enable(drvdata->regulator_1p8);
			if (rc)
				pr_err("regulator_1p8 enable failed, rc=%d\n", rc);
			usleep_range(2000, 2050);
		} else {
			if ((drvdata->regulator_1p8) == NULL) {
				pr_err("not set regulator_1p8\n");
			} else {
				rc = regulator_disable(drvdata->regulator_1p8);
				if (rc)
					pr_err("regulator_1p8 disable failed, rc=%d\n", rc);
			}
			if ((drvdata->regulator_3p0) == NULL) {
				pr_err("not set regulator_3p0\n");
			} else {
				rc = regulator_disable(drvdata->regulator_3p0);
				if (rc)
					pr_err("regulator_3p0 disable failed, rc=%d\n", rc);
			}
		}
	}

	if (!rc) {
#if !defined(ENABLE_SENSORS_FPRINT_SECURE) || defined(DISABLED_GPIO_PROTECTION)
		if (onoff) {
			if (drvdata->pins_poweron) {
				rc = pinctrl_select_state(drvdata->p, drvdata->pins_poweron);
				pr_debug("pinctrl for poweron. rc=%d\n", rc);
			}
		} else {
			if (drvdata->pins_poweroff) {
				rc = pinctrl_select_state(drvdata->p, drvdata->pins_poweroff);
				pr_debug("pinctrl for poweroff. rc=%d\n", rc);
			}
		}
#endif
		drvdata->enabled_ldo = onoff;
	}
	pr_info("%s, rc=%d\n", onoff ? "ON" : "OFF", rc);

	return rc;
}

static int qfs4008_enable_spi_clock(struct qfs4008_drvdata *drvdata)
{
	int rc = 0;

#if !defined(ENABLE_SENSORS_FPRINT_SECURE) || defined(DISABLED_GPIO_PROTECTION)
	if (drvdata->pins_poweron) {
		rc = pinctrl_select_state(drvdata->p, drvdata->pins_poweron);
		pr_info("pinctrl for spi_active. rc=%d\n", rc);
	}
#endif
	rc = spi_clk_enable(drvdata->clk_setting);
	return rc;
}

static int qfs4008_disable_spi_clock(struct qfs4008_drvdata *drvdata)
{
	int rc = 0;

#if !defined(ENABLE_SENSORS_FPRINT_SECURE) || defined(DISABLED_GPIO_PROTECTION)
	if (drvdata->pins_poweroff) {
		rc = pinctrl_select_state(drvdata->p, drvdata->pins_poweroff);
		pr_info("pinctrl for spi_inactive. rc=%d\n", rc);
	}
#endif
	rc = spi_clk_disable(drvdata->clk_setting);
	return rc;
}

static int qfs4008_enable_ipc(struct qfs4008_drvdata *drvdata)
{
	int rc = 0;

	if (drvdata->fw_ipc.gpio) {
		if (drvdata->enabled_ipc) {
			rc = -EINVAL;
			pr_err("already enabled ipc\n");
		} else {
			enable_irq(drvdata->fw_ipc.irq);
			enable_irq_wake(drvdata->fw_ipc.irq);
			drvdata->enabled_ipc = true;
		}
	}
	return rc;
}

static int qfs4008_disable_ipc(struct qfs4008_drvdata *drvdata)
{
	int rc = 0;

	if (drvdata->fw_ipc.gpio) {
		if (drvdata->enabled_ipc) {
			disable_irq_wake(drvdata->fw_ipc.irq);
			disable_irq(drvdata->fw_ipc.irq);
			drvdata->enabled_ipc = false;
		} else {
			rc = -EINVAL;
			pr_err("already disabled ipc\n");
		}
	}
	return rc;
}

static int qfs4008_enable_wuhb(struct qfs4008_drvdata *drvdata)
{
	int rc = 0;
	int gpio = 0;

	if (drvdata->fd_gpio.gpio) {
		if (drvdata->enabled_wuhb) {
			rc = -EINVAL;
			pr_err("already enabled wuhb\n");
		} else {
			enable_irq(drvdata->fd_gpio.irq);
			enable_irq_wake(drvdata->fd_gpio.irq);
			drvdata->enabled_wuhb = true;
			/* To prevent FingerUp Missing issue. */
			gpio = gpio_get_value(drvdata->fd_gpio.gpio);
			if (drvdata->fd_gpio.last_gpio_state == FINGER_DOWN_GPIO_STATE &&
				gpio == FINGER_LEAVE_GPIO_STATE) {
				pr_info("Finger leave event already occurred. %d, %d\n",
					drvdata->fd_gpio.last_gpio_state, gpio);

				__pm_wakeup_event(drvdata->fp_signal_lock,
						msecs_to_jiffies(QFS4008_WAKELOCK_HOLD_TIME));
				schedule_work(&drvdata->fd_gpio.work);
			}
		}
	}
	return rc;
}

static int qfs4008_disable_wuhb(struct qfs4008_drvdata *drvdata)
{
	int rc = 0;

	if (drvdata->fd_gpio.gpio) {
		if (drvdata->enabled_wuhb) {
			disable_irq(drvdata->fd_gpio.irq);
			disable_irq_wake(drvdata->fd_gpio.irq);
			drvdata->enabled_wuhb = false;
		} else {
			rc = -EINVAL;
			pr_err("already disabled wuhb\n");
		}
	}
	return rc;
}

/**
 * qfs4008_open() - Function called when user space opens device.
 * Successful if driver not currently open.
 * @inode:	ptr to inode object
 * @file:	ptr to file object
 *
 * Return: 0 on success. Error code on failure.
 */
static int qfs4008_open(struct inode *inode, struct file *file)
{
	struct qfs4008_drvdata *drvdata = NULL;
	int rc = 0;
	int minor_no = iminor(inode);

	if (minor_no == MINOR_NUM_FD) {
		drvdata = container_of(inode->i_cdev, struct qfs4008_drvdata,
				qfs4008_fd_cdev);
	} else if (minor_no == MINOR_NUM_IPC) {
		drvdata = container_of(inode->i_cdev, struct qfs4008_drvdata,
				qfs4008_ipc_cdev);
	} else {
		pr_err("Invalid minor number\n");
		return -EINVAL;
	}
	file->private_data = drvdata;

	/* disallowing concurrent opens */
	if (minor_no == MINOR_NUM_FD && !atomic_dec_and_test(&drvdata->fd_available)) {
		atomic_inc(&drvdata->fd_available);
		pr_err("fd_unavailable\n");
		rc = -EBUSY;
	} else if (minor_no == MINOR_NUM_IPC && !atomic_dec_and_test(&drvdata->ipc_available)) {
		atomic_inc(&drvdata->ipc_available);
		pr_err("ipc_unavailable\n");
		rc = -EBUSY;
	}

	pr_debug("minor_no=%d, rc=%d,%d,%d\n", minor_no, rc,
		atomic_read(&drvdata->fd_available),
		atomic_read(&drvdata->ipc_available));
	return rc;
}

/**
 * qfs4008_release() - Function called when user space closes device.

 * @inode:	ptr to inode object
 * @file:	ptr to file object
 *
 * Return: 0 on success. Error code on failure.
 */
static int qfs4008_release(struct inode *inode, struct file *file)
{
	struct qfs4008_drvdata *drvdata;
	int minor_no;
	int rc = 0;

	if (!file || !file->private_data) {
		pr_err("%s: NULL pointer passed\n", __func__);
		return -EINVAL;
	}
	drvdata = file->private_data;
	minor_no = iminor(inode);
	if (minor_no == MINOR_NUM_FD) {
		atomic_inc(&drvdata->fd_available);
	} else if (minor_no == MINOR_NUM_IPC) {
		atomic_inc(&drvdata->ipc_available);
	} else {
		pr_err("Invalid minor number\n");
		rc = -EINVAL;
	}
	pr_debug("minor_no=%d, rc=%d,%d,%d\n", minor_no, rc,
		atomic_read(&drvdata->fd_available),
		atomic_read(&drvdata->ipc_available));
	return rc;
}

/**
 * qfs4008_ioctl() - Function called when user space calls ioctl.
 * @file:	struct file - not used
 * @cmd:	cmd identifier:QFS4008_LOAD_APP,QFS4008_UNLOAD_APP,
 *              QFS4008_SEND_TZCMD
 * @arg:	ptr to relevant structe: either qfs4008_app or
 *              qfs4008_send_tz_cmd depending on which cmd is passed
 *
 * Return: 0 on success. Error code on failure.
 */
static long qfs4008_ioctl(
		struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;
	int data = 0;
	void __user *priv_arg = (void __user *)arg;
	struct qfs4008_drvdata *drvdata;

	if (!file || !file->private_data) {
		pr_err("%s: NULL pointer passed\n", __func__);
		return -EINVAL;
	}

	drvdata = file->private_data;

	if (IS_ERR(priv_arg)) {
		pr_err("invalid user space pointer %lu\n", arg);
		return -EINVAL;
	}

	mutex_lock(&drvdata->ioctl_mutex);

	switch (cmd) {
	case QFS4008_IS_WHUB_CONNECTED:
		break;
	case QFS4008_POWER_CONTROL:
		if (copy_from_user(&data, (void *)arg, sizeof(int)) != 0) {
			pr_err("Failed copy from user.(POWER_CONTROL)\n");
			rc = -EFAULT;
			goto ioctl_failed;
		}
		if (drvdata->enabled_ldo != data) {
			pr_debug("POWER_CONTROL\n");
			qfs4008_power_control(drvdata, data);
		}
		break;
	case QFS4008_ENABLE_SPI_CLOCK:
		pr_info("ENABLE_SPI_CLOCK\n");
		rc = qfs4008_enable_spi_clock(drvdata);
		break;
	case QFS4008_DISABLE_SPI_CLOCK:
		pr_info("DISABLE_SPI_CLOCK\n");
		rc = qfs4008_disable_spi_clock(drvdata);
		break;
	case QFS4008_ENABLE_IPC:
		pr_info("ENABLE_IPC\n");
		rc = qfs4008_enable_ipc(drvdata);
		break;
	case QFS4008_DISABLE_IPC:
		pr_info("DISABLE_IPC\n");
		rc = qfs4008_disable_ipc(drvdata);
		break;
	case QFS4008_ENABLE_WUHB:
		pr_info("ENABLE_WUHB\n");
		rc = qfs4008_enable_wuhb(drvdata);
		break;
	case QFS4008_DISABLE_WUHB:
		pr_info("DISABLE_WUHB\n");
		rc = qfs4008_disable_wuhb(drvdata);
		break;
	case QFS4008_CPU_SPEEDUP:
		if (copy_from_user(&data, (void *)arg, sizeof(int)) != 0) {
			pr_err("Failed copy from user.(SPEEDUP)\n");
			rc = -EFAULT;
			goto ioctl_failed;
		}
		if (data)
			rc = cpu_speedup_enable(drvdata->boosting);
		else
			rc = cpu_speedup_disable(drvdata->boosting);
		break;
	case QFS4008_SET_SENSOR_TYPE:
		if (copy_from_user(&data, (void *)arg, sizeof(int)) != 0) {
			pr_err("Failed to copy sensor type from user to kernel\n");
			rc = -EFAULT;
			goto ioctl_failed;
		}
		set_sensor_type(data, &drvdata->sensortype);
		break;
	case QFS4008_SET_LOCKSCREEN:
		break;
	case QFS4008_SENSOR_RESET:
		drvdata->reset_count++;
		pr_err("SENSOR_RESET\n");
		break;
	case QFS4008_SENSOR_TEST:
		if (copy_from_user(&data, (void *)arg, sizeof(int)) != 0) {
			pr_err("Failed to copy BGECAL from user to kernel\n");
			rc = -EFAULT;
			goto ioctl_failed;
		}
#ifndef ENABLE_SENSORS_FPRINT_SECURE  //only for factory
		if (data == QFS4008_SENSORTEST_DONE)
			pr_info("SENSORTEST Finished\n");
		else
			pr_info("SENSORTEST Start : 0x%x\n", data);
#endif
		break;
	case QFS4008_GET_MODELINFO:
		pr_info("QFS4008_GET_MODELINFO : %s\n", drvdata->model_info);
		if (copy_to_user((void __user *)priv_arg, drvdata->model_info, 10)) {
			pr_err("Failed to copy GET_MODELINFO to user\n");
			rc = -EFAULT;
		}
		break;
	case QFS4008_SET_TOUCH_IGNORE:
		if (copy_from_user(&data, (void *)arg, sizeof(int)) != 0) {
			pr_err("Failed copy from user.(TOUCH_IGNORE)\n");
			rc = -EFAULT;
			goto ioctl_failed;
		}
		drvdata->touch_ignore = data;
		pr_info("QFS4008_SET_TOUCH_IGNORE : %d, %d\n", drvdata->touch_ignore, data);
		break;
	default:
		pr_err("invalid cmd %d\n", cmd);
		rc = -ENOIOCTLCMD;
	}

ioctl_failed:
	mutex_unlock(&drvdata->ioctl_mutex);
	return rc;
}


static int get_events_fifo_len_locked(struct qfs4008_drvdata *drvdata, int minor_no)
{
	int len = 0;

	if (minor_no == MINOR_NUM_FD) {
		mutex_lock(&drvdata->fd_events_mutex);
		len = kfifo_len(&drvdata->fd_events);
		mutex_unlock(&drvdata->fd_events_mutex);
	} else if (minor_no == MINOR_NUM_IPC) {
		mutex_lock(&drvdata->ipc_events_mutex);
		len = kfifo_len(&drvdata->ipc_events);
		mutex_unlock(&drvdata->ipc_events_mutex);
	}

	return len;
}

static ssize_t qfs4008_read(struct file *filp, char __user *ubuf,
		size_t cnt, loff_t *ppos)
{
	struct fw_event_desc fw_event;
	struct qfs4008_drvdata *drvdata = filp->private_data;
	wait_queue_head_t *read_wait_queue = NULL;
	int rc = 0;
	int minor_no = iminor(filp->f_path.dentry->d_inode);
	int fifo_len = get_events_fifo_len_locked(drvdata, minor_no);

	if (cnt < sizeof(fw_event.ev)) {
		pr_err("Num bytes to read is too small, numBytes=%zd\n", cnt);
		return -EINVAL;
	}

	if (minor_no == MINOR_NUM_FD) {
		read_wait_queue = &drvdata->read_wait_queue_fd;
	} else if (minor_no == MINOR_NUM_IPC) {
		read_wait_queue = &drvdata->read_wait_queue_ipc;
	} else {
		pr_err("Invalid minor number\n");
		return -EINVAL;
	}

	while (fifo_len == 0) {
		if (filp->f_flags & O_NONBLOCK) {
			pr_debug("fw_events fifo: empty, returning\n");
			return -EAGAIN;
		}
		pr_debug("fw_events fifo: empty, waiting\n");
		if (wait_event_interruptible(*read_wait_queue,
				(get_events_fifo_len_locked(drvdata, minor_no) > 0)))
			return -ERESTARTSYS;

		fifo_len = get_events_fifo_len_locked(drvdata, minor_no);
	}

	if (minor_no == MINOR_NUM_FD) {
		mutex_lock(&drvdata->fd_events_mutex);
		rc = kfifo_get(&drvdata->fd_events, &fw_event);
		mutex_unlock(&drvdata->fd_events_mutex);
	} else if (minor_no == MINOR_NUM_IPC) {
		mutex_lock(&drvdata->ipc_events_mutex);
		rc = kfifo_get(&drvdata->ipc_events, &fw_event);
		mutex_unlock(&drvdata->ipc_events_mutex);
	} else {
		pr_err("Invalid minor number\n");
	}

	if (!rc) {
		pr_err("fw_events fifo: unexpectedly empty\n");
		return -EINVAL;
	}

	rc = copy_to_user(ubuf, &fw_event.ev, sizeof(fw_event.ev));
	if (rc != 0) {
		pr_err("Failed to copy_to_user:%d - event:%d, minor:%d\n",
			rc, (int)fw_event.ev, minor_no);
	} else {
		if (minor_no == MINOR_NUM_FD) {
			pr_info("Firmware event %d at minor no %d read at time %lu uS, mutex_unlock\n",
				(int)fw_event.ev, minor_no,
				(unsigned long)ktime_to_us(ktime_get()));
		} else {
			pr_info("Firmware event %d at minor no %d read at time %lu uS\n",
				(int)fw_event.ev, minor_no,
				(unsigned long)ktime_to_us(ktime_get()));
		}
	}
	return rc;
}

static unsigned int qfs4008_poll(struct file *filp,
	struct poll_table_struct *wait)
{
	struct qfs4008_drvdata *drvdata = filp->private_data;
	unsigned int mask = 0;
	int minor_no = iminor(filp->f_path.dentry->d_inode);

	if (minor_no == MINOR_NUM_FD) {
		poll_wait(filp, &drvdata->read_wait_queue_fd, wait);
		if (kfifo_len(&drvdata->fd_events) > 0)
			mask |= (POLLIN | POLLRDNORM);
	} else if (minor_no == MINOR_NUM_IPC) {
		poll_wait(filp, &drvdata->read_wait_queue_ipc, wait);
		if (kfifo_len(&drvdata->ipc_events) > 0)
			mask |= (POLLIN | POLLRDNORM);
	} else {
		pr_err("Invalid minor number\n");
		return -EINVAL;
	}

	return mask;
}

static const struct file_operations qfs4008_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = qfs4008_ioctl,
	.open = qfs4008_open,
	.release = qfs4008_release,
	.read = qfs4008_read,
	.poll = qfs4008_poll
};

static int qfs4008_dev_register(struct qfs4008_drvdata *drvdata)
{
	dev_t dev_no, major_no;
	int rc = 0;
	struct device *dev = drvdata->dev;

	rc = alloc_chrdev_region(&dev_no, 0, 2, QFS4008_DEV);
	if (rc) {
		pr_err("alloc_chrdev_region failed %d\n", rc);
		goto err_alloc;
	}
	major_no = MAJOR(dev_no);

	cdev_init(&drvdata->qfs4008_fd_cdev, &qfs4008_fops);
	drvdata->qfs4008_fd_cdev.owner = THIS_MODULE;
	rc = cdev_add(&drvdata->qfs4008_fd_cdev,
			 MKDEV(major_no, MINOR_NUM_FD), 1);
	if (rc) {
		pr_err("cdev_add failed for fd %d\n", rc);
		goto err_cdev_add;
	}

	cdev_init(&drvdata->qfs4008_ipc_cdev, &qfs4008_fops);
	drvdata->qfs4008_ipc_cdev.owner = THIS_MODULE;
	rc = cdev_add(&drvdata->qfs4008_ipc_cdev,
			MKDEV(major_no, MINOR_NUM_IPC), 1);
	if (rc) {
		pr_err("cdev_add failed for ipc %d\n", rc);
		goto err_cdev_add;
	}

#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
	drvdata->qfs4008_class = class_create(QFS4008_DEV);
#else
	drvdata->qfs4008_class = class_create(THIS_MODULE, QFS4008_DEV);
#endif
	if (IS_ERR(drvdata->qfs4008_class)) {
		rc = PTR_ERR(drvdata->qfs4008_class);
		pr_err("class_create failed %d\n", rc);
		goto err_class_create;
	}

	dev = device_create(drvdata->qfs4008_class, NULL,
			drvdata->qfs4008_fd_cdev.dev,
			drvdata, "%s_fd", QFS4008_DEV);
	if (IS_ERR(dev)) {
		rc = PTR_ERR(dev);
		pr_err("fd device_create failed %d\n", rc);
		goto err_dev_create_fd;
	}

	dev = device_create(drvdata->qfs4008_class, NULL,
			drvdata->qfs4008_ipc_cdev.dev,
			drvdata, "%s_ipc", QFS4008_DEV);
	if (IS_ERR(dev)) {
		rc = PTR_ERR(dev);
		pr_err("ipc device_create failed %d\n", rc);
		goto err_dev_create_ipc;
	}
	pr_info("finished\n");
	return 0;
err_dev_create_ipc:
	device_destroy(drvdata->qfs4008_class, drvdata->qfs4008_fd_cdev.dev);
err_dev_create_fd:
	class_destroy(drvdata->qfs4008_class);
err_class_create:
	cdev_del(&drvdata->qfs4008_fd_cdev);
	cdev_del(&drvdata->qfs4008_ipc_cdev);
err_cdev_add:
	unregister_chrdev_region(drvdata->qfs4008_fd_cdev.dev, 1);
	unregister_chrdev_region(drvdata->qfs4008_ipc_cdev.dev, 1);
err_alloc:
	return rc;
}


static void qfs4008_gpio_report_event(struct qfs4008_drvdata *drvdata)
{
	int state;
	struct fw_event_desc fw_event;

	state = (gpio_get_value(drvdata->fd_gpio.gpio) ? FINGER_DOWN_GPIO_STATE : FINGER_LEAVE_GPIO_STATE)
		^ drvdata->fd_gpio.active_low;

	if (state == drvdata->fd_gpio.last_gpio_state) {
		pr_err("skip the report_event. this event already reported, last_gpio:%d\n", state);
		return;
	}

	if (drvdata->touch_ignore && state == FINGER_DOWN_GPIO_STATE) {
		pr_info("touch ignored. %s state, %d \n", (state ? "Finger Down" : "Finger Leave"), drvdata->touch_ignore);
		return;
	}

	drvdata->fd_gpio.last_gpio_state = state;

	fw_event.ev = (state ? FW_EVENT_FINGER_DOWN : FW_EVENT_FINGER_UP);

	mutex_lock(&drvdata->fd_events_mutex);

	kfifo_reset(&drvdata->fd_events);

	if (!kfifo_put(&drvdata->fd_events, fw_event))
		pr_err("fw events fifo: error adding item\n");

	mutex_unlock(&drvdata->fd_events_mutex);
	wake_up_interruptible(&drvdata->read_wait_queue_fd);
	pr_info("state: %s\n", state ? "Finger Down" : "Finger Leave");
}

static void qfs4008_wuhb_work_func(struct work_struct *work)
{
	struct qfs4008_drvdata *drvdata = container_of(work,
			struct qfs4008_drvdata, fd_gpio.work);
	qfs4008_gpio_report_event(drvdata);
}

static irqreturn_t qfs4008_wuhb_irq_handler(int irq, void *dev_id)
{
	struct qfs4008_drvdata *drvdata = dev_id;

	if (irq != drvdata->fd_gpio.irq) {
		pr_warn("invalid irq %d (expected %d)\n", irq,
				drvdata->fd_gpio.irq);
		return IRQ_HANDLED;
	}

	drvdata->wuhb_count++;
	__pm_wakeup_event(drvdata->fp_signal_lock,
			msecs_to_jiffies(QFS4008_WAKELOCK_HOLD_TIME));

	schedule_work(&drvdata->fd_gpio.work);

	return IRQ_HANDLED;
}

/*
 * qfs4008_ipc_irq_handler() - function processes IPC
 * interrupts on its own thread
 * @irq:	the interrupt that occurred
 * @dev_id: pointer to the qfs4008_drvdata
 *
 * Return: IRQ_HANDLED when complete
 */
static irqreturn_t qfs4008_ipc_irq_handler(int irq, void *dev_id)
{
	struct qfs4008_drvdata *drvdata = (struct qfs4008_drvdata *)dev_id;
	enum qfs4008_fw_event ev = FW_EVENT_CBGE_REQUIRED;
	struct fw_event_desc fw_ev_des;

	__pm_wakeup_event(drvdata->fp_signal_lock,
			msecs_to_jiffies(QFS4008_WAKELOCK_HOLD_TIME));
	mutex_lock(&drvdata->mutex);

	if (irq != drvdata->fw_ipc.irq) {
		pr_warn("invalid irq %d (expected %d)\n", irq,
				drvdata->fw_ipc.irq);
		goto ipc_irq_failed;
	}

	mutex_lock(&drvdata->ipc_events_mutex);
	fw_ev_des.ev = ev;
	if (!kfifo_put(&drvdata->ipc_events, fw_ev_des))
		pr_err("fw events: fifo full, drop event %d\n", (int) ev);

	drvdata->cbge_count++;
	mutex_unlock(&drvdata->ipc_events_mutex);

	wake_up_interruptible(&drvdata->read_wait_queue_ipc);
	pr_debug("ipc interrupt received, irq=%d, event=%d\n", irq, (int)ev);
ipc_irq_failed:
	mutex_unlock(&drvdata->mutex);
	return IRQ_HANDLED;
}

static int qfs4008_setup_fd_gpio_irq(struct platform_device *pdev,
		struct qfs4008_drvdata *drvdata)
{
	int rc = 0;
	int irq;
	const char *desc = "qbt_finger_detect";

	rc = devm_gpio_request_one(&pdev->dev, drvdata->fd_gpio.gpio,
				GPIOF_IN, desc);
	if (rc < 0) {
		pr_err("failed to request gpio %d, error %d\n",
				drvdata->fd_gpio.gpio, rc);
		goto fd_gpio_failed;
	}

	irq = gpio_to_irq(drvdata->fd_gpio.gpio);
	if (irq < 0) {
		rc = irq;
		pr_err("unable to get irq number for gpio %d, error %d\n",
				drvdata->fd_gpio.gpio, rc);
		goto fd_gpio_failed_request;
	}

	drvdata->fd_gpio.irq = irq;
	INIT_WORK(&drvdata->fd_gpio.work, qfs4008_wuhb_work_func);
	rc = devm_request_any_context_irq(&pdev->dev, drvdata->fd_gpio.irq,
		qfs4008_wuhb_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		desc, drvdata);
	if (rc < 0) {
		pr_err("unable to claim irq %d; error %d\n",
				drvdata->fd_gpio.irq, rc);
		goto fd_gpio_failed_request;
	}
	enable_irq_wake(drvdata->fd_gpio.irq);
	drvdata->enabled_wuhb = true;
	qfs4008_disable_wuhb(drvdata);
	pr_debug("irq=%d,gpio=%d,rc=%d\n", drvdata->fd_gpio.irq, drvdata->fd_gpio.gpio, rc);
fd_gpio_failed_request:
	gpio_free(drvdata->fd_gpio.gpio);
fd_gpio_failed:
	return rc;
}

static int qfs4008_setup_ipc_irq(struct platform_device *pdev,
	struct qfs4008_drvdata *drvdata)
{
	int rc = 0;
	const char *desc = "qbt_ipc";

	drvdata->fw_ipc.irq = gpio_to_irq(drvdata->fw_ipc.gpio);
	if (drvdata->fw_ipc.irq < 0) {
		rc = drvdata->fw_ipc.irq;
		pr_err("no irq for gpio %d, error=%d\n",
				drvdata->fw_ipc.gpio, rc);
		goto ipc_gpio_failed;
	}

	rc = devm_gpio_request_one(&pdev->dev, drvdata->fw_ipc.gpio,
			GPIOF_IN, desc);

	if (rc < 0) {
		pr_err("failed to request gpio %d, error %d\n",
			drvdata->fw_ipc.gpio, rc);
		goto ipc_gpio_failed;
	}

	rc = devm_request_threaded_irq(&pdev->dev,
			drvdata->fw_ipc.irq, NULL, qfs4008_ipc_irq_handler,
			IRQF_ONESHOT | IRQF_TRIGGER_FALLING, desc, drvdata);

	if (rc < 0) {
		pr_err("failed to register for ipc irq %d, rc = %d\n",
				drvdata->fw_ipc.irq, rc);
		goto ipc_gpio_failed_request;
	}

	enable_irq_wake(drvdata->fw_ipc.irq);
	drvdata->enabled_ipc = true;
	qfs4008_disable_ipc(drvdata);
	pr_debug("irq=%d,gpio=%d,rc=%d\n", drvdata->fw_ipc.irq,
			drvdata->fw_ipc.gpio, rc);
ipc_gpio_failed_request:
	gpio_free(drvdata->fw_ipc.gpio);
ipc_gpio_failed:
	return rc;
}

/**
 * qfs4008_read_device_tree() - Function reads device tree
 * properties into driver data
 * @pdev:	ptr to platform device object
 * @drvdata:	ptr to driver data
 *
 * Return: 0 on success. Error code on failure.
 */
static int qfs4008_read_device_tree(struct platform_device *pdev,
	struct qfs4008_drvdata *drvdata)
{
	int rc = 0;

	/* read IPC gpio */
	drvdata->fw_ipc.gpio = of_get_named_gpio(pdev->dev.of_node,
		"qcom,ipc-gpio", 0);
	if (drvdata->fw_ipc.gpio < 0) {
		rc = drvdata->fw_ipc.gpio;
		pr_err("ipc gpio not found, error=%d\n", rc);
		goto dt_failed;
	}

	/* read WUHB gpio */
	drvdata->fd_gpio.gpio = of_get_named_gpio(pdev->dev.of_node,
						"qcom,wuhb-gpio", 0);
	if (drvdata->fd_gpio.gpio < 0) {
		rc = drvdata->fd_gpio.gpio;
		pr_err("wuhb gpio not found, error=%d\n", rc);
		goto dt_failed;
	} else {
		drvdata->fd_gpio.active_low = 0x0;
	}

	rc = of_property_read_string(pdev->dev.of_node, "qcom,btp-regulator-1p8", &drvdata->btp_vdd_1p8);
	if (rc < 0) {
		pr_err("not set btp_regulator_1p8\n");
		drvdata->btp_vdd_1p8 = NULL;
		goto dt_failed;
	} else {
		drvdata->regulator_1p8 = regulator_get(&pdev->dev, drvdata->btp_vdd_1p8);
		if (IS_ERR(drvdata->regulator_1p8) || (drvdata->regulator_1p8) == NULL) {
			pr_err("not set regulator_1p8\n");
			drvdata->regulator_1p8 = NULL;
			goto dt_failed;
		} else {
			pr_info("btp_regulator_1p8 ok\n");
			drvdata->enabled_ldo = 0;
			rc = regulator_set_load(drvdata->regulator_1p8, 500000);
			if (rc)
				pr_err("regulator_1p8 set_load failed, rc=%d\n", rc);
		}
	}

	rc = of_property_read_string(pdev->dev.of_node, "qcom,btp-regulator-3p0", &drvdata->btp_vdd_3p0);
	if (rc < 0) {
		pr_err("not set btp_regulator_3p0\n");
		drvdata->btp_vdd_3p0 = NULL;
		goto dt_failed;
	} else {
		drvdata->regulator_3p0 = regulator_get(&pdev->dev, drvdata->btp_vdd_3p0);
		if (IS_ERR(drvdata->regulator_3p0) || (drvdata->regulator_3p0) == NULL) {
			pr_err("not set regulator_3p0\n");
			drvdata->regulator_3p0 = NULL;
			goto dt_failed;
		} else {
			pr_info("btp_regulator_3p0 ok\n");
			drvdata->enabled_ldo = 0;
			rc = regulator_set_load(drvdata->regulator_3p0, 500000);
			if (rc)
				pr_err("regulator_3p0 set_load failed, rc=%d\n", rc);
		}
	}

	if (of_property_read_u32(pdev->dev.of_node, "qcom,min_cpufreq_limit",
				&drvdata->boosting->min_cpufreq_limit))
		drvdata->boosting->min_cpufreq_limit = 0;

	if (of_property_read_string_index(pdev->dev.of_node, "qcom,position", 0,
			(const char **)&drvdata->sensor_position))
		drvdata->sensor_position = "32.48,0.00,7.50,8.25,14.80,14.80,13.00,13.00,5.00";
	pr_info("Sensor Position: %s\n", drvdata->sensor_position);

	if (of_property_read_string_index(pdev->dev.of_node, "qcom,modelinfo", 0,
			(const char **)&drvdata->model_info)) {
		drvdata->model_info = "S92X";
	}
	pr_info("modelinfo: %s\n", drvdata->model_info);

	if (of_property_read_string_index(pdev->dev.of_node, "qcom,chipid", 0,
			(const char **)&drvdata->chipid)) {
		drvdata->chipid = "QFS4008";
	}
	pr_info("chipid: %s\n", drvdata->chipid);

	pr_info("finished\n");
	return rc;
dt_failed:
	pr_err("failed:%d\n", rc);
	return rc;
}

static void qfs4008_work_func_debug(struct work_struct *work)
{
	struct debug_logger *logger;
	struct qfs4008_drvdata *drvdata;

	logger = container_of(work, struct debug_logger, work_debug);
	drvdata = dev_get_drvdata(logger->dev);
	pr_info("ldo:%d,ipc:%d,wuhb:%d,tz:%d,type:%s,int:%d,%d,rst:%d\n",
		drvdata->enabled_ldo, drvdata->enabled_ipc,
		drvdata->enabled_wuhb, drvdata->tz_mode,
		drvdata->sensortype > 0 ? drvdata->chipid : sensor_status[drvdata->sensortype + 2],
		drvdata->cbge_count, drvdata->wuhb_count,
		drvdata->reset_count);
}

/**
 * qfs4008_probe() - Function loads hardware config from device tree
 * @pdev:	ptr to platform device object
 *
 * Return: 0 on success. Error code on failure.
 */
static int qfs4008_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct qfs4008_drvdata *drvdata;
	int rc = 0;

	pr_info("Start\n");
#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge) {
		pr_info("Do not load driver due to : lpm %d\n", lpcharge);
		return rc;
	}
#endif
	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->clk_setting = devm_kzalloc(dev, sizeof(*drvdata->clk_setting),
						GFP_KERNEL);
	if (drvdata->clk_setting == NULL)
		return -ENOMEM;

	drvdata->boosting = devm_kzalloc(dev, sizeof(*drvdata->boosting),
						GFP_KERNEL);
	if (drvdata->boosting == NULL)
		return -ENOMEM;

	drvdata->logger = devm_kzalloc(dev, sizeof(*drvdata->logger),
						GFP_KERNEL);
	if (drvdata->logger == NULL)
		return -ENOMEM;

	drvdata->dev = dev;
	drvdata->logger->dev = dev;
	platform_set_drvdata(pdev, drvdata);

	rc = qfs4008_read_device_tree(pdev, drvdata);
	if (rc < 0)
		goto probe_failed_dt;

	atomic_set(&drvdata->fd_available, 1);
	atomic_set(&drvdata->ipc_available, 1);

	mutex_init(&drvdata->mutex);
	mutex_init(&drvdata->ioctl_mutex);
	mutex_init(&drvdata->fd_events_mutex);
	mutex_init(&drvdata->ipc_events_mutex);

	rc = qfs4008_dev_register(drvdata);
	if (rc < 0)
		goto probe_failed_dev_register;

	INIT_KFIFO(drvdata->fd_events);
	INIT_KFIFO(drvdata->ipc_events);
	init_waitqueue_head(&drvdata->read_wait_queue_fd);
	init_waitqueue_head(&drvdata->read_wait_queue_ipc);

	drvdata->clk_setting->spi_wake_lock = wakeup_source_register(dev, "qfs4008_spi_lock");
	drvdata->fp_signal_lock = wakeup_source_register(dev, "qfs4008_signal_lock");

	rc = qfs4008_pinctrl_register(drvdata);
	if (rc < 0)
		pr_err("register pinctrl failed: %d\n", rc);

	rc = qfs4008_setup_fd_gpio_irq(pdev, drvdata);
	if (rc < 0)
		goto probe_failed_fd_gpio;

	rc = qfs4008_setup_ipc_irq(pdev, drvdata);
	if (rc < 0)
		goto probe_failed_ipc_gpio;

	rc = device_init_wakeup(dev, 1);
	if (rc < 0)
		goto probe_failed_device_init_wakeup;

	rc = spi_clk_register(drvdata->clk_setting, dev);
	if (rc < 0)
		goto probe_failed_spi_clk_register;

	rc = fingerprint_register(drvdata->fp_device,
		drvdata, fp_attrs, "fingerprint");
	if (rc) {
		pr_err("sysfs register failed\n");
		goto probe_failed_sysfs_register;
	}

	drvdata->clk_setting->spi_speed = SPI_CLOCK_MAX;
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	drvdata->sensortype = SENSOR_UNKNOWN;
#else
	drvdata->sensortype = SENSOR_OK;
#endif
	drvdata->cbge_count = 0;
	drvdata->wuhb_count = 0;
	drvdata->reset_count = 0;
	drvdata->touch_ignore = 0;
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	drvdata->clk_setting->enabled_clk = false;
	drvdata->tz_mode = true;
#else
	drvdata->clk_setting->enabled_clk = true;
	drvdata->tz_mode = false;
#endif

	g_logger = drvdata->logger;
	set_fp_debug_timer(drvdata->logger, qfs4008_work_func_debug);
	enable_fp_debug_timer(drvdata->logger);


	pr_info("finished\n");
	return 0;

probe_failed_sysfs_register:
	spi_clk_unregister(drvdata->clk_setting);
probe_failed_spi_clk_register:
probe_failed_device_init_wakeup:
	gpio_free(drvdata->fw_ipc.gpio);
probe_failed_ipc_gpio:
	gpio_free(drvdata->fd_gpio.gpio);
probe_failed_fd_gpio:
	if (drvdata->p) {
		devm_pinctrl_put(drvdata->p);
		drvdata->p = NULL;
	}
	wakeup_source_unregister(drvdata->clk_setting->spi_wake_lock);
	wakeup_source_unregister(drvdata->fp_signal_lock);
	device_destroy(drvdata->qfs4008_class, drvdata->qfs4008_ipc_cdev.dev);
	device_destroy(drvdata->qfs4008_class, drvdata->qfs4008_fd_cdev.dev);
	class_destroy(drvdata->qfs4008_class);
	cdev_del(&drvdata->qfs4008_fd_cdev);
	cdev_del(&drvdata->qfs4008_ipc_cdev);
	unregister_chrdev_region(drvdata->qfs4008_fd_cdev.dev, 1);
	unregister_chrdev_region(drvdata->qfs4008_ipc_cdev.dev, 1);
probe_failed_dev_register:
	if (drvdata->regulator_1p8)
		regulator_put(drvdata->regulator_1p8);
	if (drvdata->regulator_3p0)
		regulator_put(drvdata->regulator_3p0);
probe_failed_dt:
	drvdata = NULL;
	pr_err("failed: %d\n", rc);
	return rc;
}

static int qfs4008_remove(struct platform_device *pdev)
{
	struct qfs4008_drvdata *drvdata = platform_get_drvdata(pdev);

	pr_info("called\n");

	mutex_destroy(&drvdata->mutex);
	mutex_destroy(&drvdata->ioctl_mutex);
	mutex_destroy(&drvdata->fd_events_mutex);
	mutex_destroy(&drvdata->ipc_events_mutex);
	device_destroy(drvdata->qfs4008_class, drvdata->qfs4008_fd_cdev.dev);
	device_destroy(drvdata->qfs4008_class, drvdata->qfs4008_ipc_cdev.dev);

	disable_fp_debug_timer(drvdata->logger);
	if (drvdata->regulator_1p8)
		regulator_put(drvdata->regulator_1p8);
	if (drvdata->regulator_3p0)
		regulator_put(drvdata->regulator_3p0);
	wakeup_source_unregister(drvdata->clk_setting->spi_wake_lock);
	wakeup_source_unregister(drvdata->fp_signal_lock);
	class_destroy(drvdata->qfs4008_class);
	cdev_del(&drvdata->qfs4008_fd_cdev);
	cdev_del(&drvdata->qfs4008_ipc_cdev);
	unregister_chrdev_region(drvdata->qfs4008_fd_cdev.dev, 1);
	unregister_chrdev_region(drvdata->qfs4008_ipc_cdev.dev, 1);
	fingerprint_unregister(drvdata->fp_device, fp_attrs);
	device_init_wakeup(&pdev->dev, 0);
	spi_clk_unregister(drvdata->clk_setting);

	if (drvdata->p) {
		devm_pinctrl_put(drvdata->p);
		drvdata->p = NULL;
	}
	drvdata = NULL;

	return 0;
}

static int qfs4008_suspend(struct platform_device *pdev, pm_message_t state)
{
	int rc = 0;
	struct qfs4008_drvdata *drvdata = platform_get_drvdata(pdev);

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge)
		return rc;
#endif
	/*
	 * Returning an error code if driver currently making a TZ call.
	 * Note: The purpose of this driver is to ensure that the clocks are on
	 * while making a TZ call. Hence the clock check to determine if the
	 * driver will allow suspend to occur.
	 */
	if (!mutex_trylock(&drvdata->mutex))
		return -EBUSY;

	disable_fp_debug_timer(drvdata->logger);
	pr_info("ret = %d\n", rc);
	mutex_unlock(&drvdata->mutex);

	return 0;
}

static int qfs4008_resume(struct platform_device *pdev)
{
	int rc = 0;
	struct qfs4008_drvdata *drvdata = platform_get_drvdata(pdev);

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge)
		return rc;
#endif

	enable_fp_debug_timer(drvdata->logger);
	pr_info("ret = %d\n", rc);

	return 0;
}

static const struct of_device_id qfs4008_match[] = {
	{ .compatible = "qcom,qfs4008" },
	{}
};

static struct platform_driver qfs4008_plat_driver = {
	.probe = qfs4008_probe,
	.remove = qfs4008_remove,
	.suspend = qfs4008_suspend,
	.resume = qfs4008_resume,
	.driver = {
		.name = QFS4008_DEV,
		.owner = THIS_MODULE,
		.of_match_table = qfs4008_match,
	},
};

static int __init qfs4008_init(void)
{
	int rc = 0;

	rc = platform_driver_register(&qfs4008_plat_driver);
	pr_info("ret : %d\n", rc);

	return rc;
}

static void __exit qfs4008_exit(void)
{
	pr_info("entry\n");
	return platform_driver_unregister(&qfs4008_plat_driver);
}

late_initcall(qfs4008_init);
module_exit(qfs4008_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("fp.sec@samsung.com");
MODULE_DESCRIPTION("Samsung Electronics Inc. QFS4008 driver");
