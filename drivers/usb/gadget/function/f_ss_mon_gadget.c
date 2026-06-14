/*
 * f_ss_mon_gadget.c - generic USB serial function driver
 *
 * Copyright (C) 2021 by samsung Corporation
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>

#include <linux/configfs.h>
#include <linux/usb/composite.h>

#include <linux/usb/android_accessory.h>
#include <uapi/linux/usb/android_accessory.h>
#include <linux/miscdevice.h>
#include <linux/workqueue.h>

#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
#include <linux/usb_notify.h>
#endif

#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
#include <linux/usb/typec/manager/usb_typec_manager_notifier.h>
#endif

#include <linux/battery/sec_battery_common.h>

#define MAX_INST_NAME_LEN		40
#define MAX_NAME_LEN	40

#define DRIVER_NAME		 "usb_mtp_gadget"
#define MAX_GUID_SIZE	0x28
static const char mtpg_longname[] =	"mtp";
static const char shortname[] = DRIVER_NAME;

static char guid_info[MAX_GUID_SIZE+1];
/* ID for Microsoft MTP OS String */
#define MTPG_OS_STRING_ID   0xEE
#define GADGET_MTP	0x01
#define GADGET_RNDIS	0x02
#define GADGET_ACCESSORY	0x04
#define GADGET_ADB	0x08
#define GADGET_ACM	0x10
#define GADGET_DM	0x20
#define GADGET_MIDI	0x40
#define GADGET_CONN_GADGET	0x80

struct usb_mtp_avd_descriptor {
	__u8	bLength;
	__u8	bDescriptorType;
	__u8	bDescriptorSubType;

	__u16	bDAU1_Type;
	__u16	bDAU1_Length;
	__u8	bDAU1_Value;
} __attribute__ ((packed));

static struct usb_mtp_avd_descriptor mtp_avd_descriptor = {
	.bLength            =   0x08,
	.bDescriptorType    =   0x24,
	.bDescriptorSubType =   0x80,

	/* First DAU = MTU Size */
	.bDAU1_Type         =   0x000C,
	.bDAU1_Length       =   0x0001,
	.bDAU1_Value	= 0x01,
};

static struct usb_descriptor_header *log_fs_function[]  = {
	(struct usb_descriptor_header *) &mtp_avd_descriptor,
	NULL,
};

/* high speed support: */
static struct usb_descriptor_header *log_hs_function[]  = {
	(struct usb_descriptor_header *) &mtp_avd_descriptor,
	NULL,
};

/* super speed support: */
static struct usb_descriptor_header *log_ss_function[] = {
	(struct usb_descriptor_header *) &mtp_avd_descriptor,
	NULL,
};

#define ACCESSORY_STRING_MASK ( \
		  1 << ACCESSORY_STRING_MANUFACTURER | \
		  1 << ACCESSORY_STRING_MODEL | \
		  1 << ACCESSORY_STRING_VERSION)

enum {
	RELEASE = 0,
	NOTIFY	= 1,
};

struct f_ss_monitor {
	struct usb_function function;
	int	current_usb_mode;
	u8	intreface_id;
	int	accessory_string;
	u8	aoa_start_cmd;
	int	usb_function_info;
	char usb_mode[50];
	int	vbus_current;
	bool is_bind;
	struct ss_monitor_instance *func_inst;
	struct work_struct		set_vbus_current_work;
};

static inline struct f_ss_monitor *func_to_ss_monitor(struct usb_function *f)
{
	return container_of(f, struct f_ss_monitor, function);
}

struct aoa_connect_status {
	struct delayed_work usb_reset_event_work;
	ktime_t rst_time_before;
	ktime_t rst_time_first;
	int rst_err_cnt;
	bool rst_err_noti;
	bool event_state;
	bool acc_dev_status;
	bool acc_online;
};

struct ss_monitor_instance {
	struct usb_function_instance func_inst;
	char *name;
	struct f_ss_monitor *ss_monitor;
	struct aoa_connect_status aoa_reset;
	bool vbus_session_called;
};

static inline struct ss_monitor_instance *to_fi_ss_monitor(struct usb_function_instance *fi)
{
	return container_of(fi, struct ss_monitor_instance, func_inst);
}

static struct ss_monitor_instance *g_ss_monitor;

#define ERR_RESET_CNT	3

static inline int _lock(atomic_t *excl)
{
	pr_info("[%s] \tline = [%d]\n", __func__, __LINE__);

	if (atomic_inc_return(excl) == 1)
		return 0;

	atomic_dec(excl);
	return -1;
}

static inline void _unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

struct mtpg_dev {
	int			error;
	atomic_t		open_excl;
};
static struct mtpg_dev    *the_mtpg;

static int mtpg_open(struct inode *ip, struct file *fp)
{
	pr_info("[USB] %s: \tline = [%d]\n", __func__, __LINE__);

	if (_lock(&the_mtpg->open_excl)) {
		pr_err("[USB] %s: fn mtpg device busy\n", __func__);
		return -EBUSY;
	}

	fp->private_data = the_mtpg;

	/* clear the error latch */
	the_mtpg->error = 0;

	return 0;
}

static int mtpg_release_device(struct inode *ip, struct file *fp)
{
	pr_info("[USB] %s: \tline = [%d]\n", __func__, __LINE__);
	if (the_mtpg != NULL)
		_unlock(&the_mtpg->open_excl);
	return 0;
}

/* file operations for MTP device /dev/usb_mtp_gadget */
static const struct file_operations mtpg_fops = {
	.owner   = THIS_MODULE,
	.open    = mtpg_open,
	.release = mtpg_release_device,
};

static struct miscdevice mtpg_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = shortname,
	.fops = &mtpg_fops,
};

/*-------------------------------------------------------------------------*/
static int ss_monitor_set_alt(struct usb_function *f,
		unsigned int intf, unsigned int alt)
{
	struct f_ss_monitor	*ss_monitor;

	ss_monitor = func_to_ss_monitor(f);
	g_ss_monitor->aoa_reset.acc_dev_status = false;
	if (ss_monitor->usb_function_info & GADGET_ACCESSORY) {
		pr_info("[USB] %s: aoa connect\n", __func__);
		g_ss_monitor->aoa_reset.acc_online = true;
		g_ss_monitor->aoa_reset.rst_err_cnt = 0;
	}
	return 0;
}

static void ss_monitor_disable(struct usb_function *f)
{
	struct f_ss_monitor	*ss_monitor;
	char aoa_check[12] = {0,};

	ss_monitor = func_to_ss_monitor(f);
	if (!ss_monitor)
		goto ret;
	ss_monitor->vbus_current = USB_CURRENT_UNCONFIGURED;
	if (ss_monitor->accessory_string && !ss_monitor->aoa_start_cmd) {
		snprintf(aoa_check, sizeof(aoa_check), "AOA_ERR_%x", ss_monitor->accessory_string);
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
		store_usblog_notify(NOTIFY_USBMODE_EXTRA, (void *)aoa_check, NULL);
#endif
	}
	ss_monitor->accessory_string = 0;
	ss_monitor->aoa_start_cmd = 0;
ret:
	if (g_ss_monitor->aoa_reset.acc_online) {
		g_ss_monitor->aoa_reset.acc_online = false;
		g_ss_monitor->aoa_reset.acc_dev_status = true;
	}

	memset(guid_info, 0, sizeof(guid_info));
}

/* for BC1.2 spec */
static int set_vbus_current(int state)
{
	struct power_supply *psy;
	union power_supply_propval pval = {0};

	pr_info("[USB] %s: %dmA\n", __func__, state);
	psy = power_supply_get_by_name("battery");
	if (!psy) {
		pr_err("[USB] %s: fail to get battery power_supply\n", __func__);
		return -1;
	}

	pval.intval = state;
	psy_do_property("battery", set, POWER_SUPPLY_EXT_PROP_USB_CONFIGURE, pval);
	power_supply_put(psy);
	return 0;
}

/* notify current change */
static void set_vbus_current_work(struct work_struct *w)
{
	struct f_ss_monitor *ss_monitor;
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
	struct otg_notify *o_notify = get_otg_notify();
#endif

	ss_monitor = container_of(w, struct f_ss_monitor, set_vbus_current_work);

	switch (ss_monitor->vbus_current) {
	case USB_CURRENT_SUSPENDED:
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
	/* set vbus current for suspend state is called in usb_notify. */
		send_otg_notify(o_notify, NOTIFY_EVENT_USBD_SUSPENDED, 1);
#endif
		goto skip;
	case USB_CURRENT_UNCONFIGURED:
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
		send_otg_notify(o_notify, NOTIFY_EVENT_USBD_UNCONFIGURED, 1);
#endif
		break;
	case USB_CURRENT_HIGH_SPEED:
	case USB_CURRENT_SUPER_SPEED:
#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
		send_otg_notify(o_notify, NOTIFY_EVENT_USBD_CONFIGURED, 1);
#endif
		break;
	default:
		break;
	}
	set_vbus_current(ss_monitor->vbus_current);
skip:
	return;
}

/*-------------------------------------------------------------------------*/
static void
mtp_complete_get_guid(struct usb_ep *ep, struct usb_request *req)
{
	int size;
	if (the_mtpg)
		pr_info("[USB] %s: \tline = [%d]\n", __func__, __LINE__);
	else
		pr_info("[USB] %s: \tline = [%d] / mtpg misc driver load fails\n", __func__, __LINE__);

	if (req->status != 0) {
		pr_info("[USB] %s: req->status !=0\tline = [%d]\n", __func__, __LINE__);
		return;
	}

	if (req->actual >= sizeof(guid_info))
		size = sizeof(guid_info)-1;
	else
		size = req->actual;
	memset(guid_info, 0, sizeof(guid_info));
	memcpy(guid_info, req->buf, size);
}

static ssize_t guid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	pr_info("[USB] %s: \tline = [%d]\n", __func__, __LINE__);
	memcpy(buf, guid_info, MAX_GUID_SIZE);
	return MAX_GUID_SIZE;
}

static ssize_t guid_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	pr_info("[USB] %s: \tline = [%d]\n", __func__, __LINE__);
	if (size > MAX_GUID_SIZE)
		return -EINVAL;

	value = strlcpy(guid_info, buf, size);
	return value;
}

DEVICE_ATTR_RW(guid);

#if defined(CONFIG_USB_NOTIFY_PROC_LOG)
static bool is_aoa_string_come(int string_index)
{
	if ((string_index & (1<<ACCESSORY_STRING_VERSION)) &&
		(string_index & (1<<ACCESSORY_STRING_MODEL)) &&
		(string_index & (1<<ACCESSORY_STRING_MANUFACTURER)))
		return true;
	else
		return false;
}

static char *get_usb_speed(enum usb_device_speed	speed)
{
	switch (speed) {
	case USB_SPEED_HIGH:
		return "HS";
	case USB_SPEED_SUPER:
		return "SS";
	case USB_SPEED_SUPER_PLUS:
		return "PSS";
	case USB_SPEED_FULL:
		return "FS";
	case USB_SPEED_UNKNOWN:
		return "UN";
	case USB_SPEED_LOW:
		return "LS";
	case USB_SPEED_WIRELESS:
		return "WS";
	default:
		return "ERR";
	}
}
#endif

static void ss_mon_usb_event_work(struct work_struct *work)
{
	pr_info("%s, event_state: %d\n", __func__, g_ss_monitor->aoa_reset.event_state);

#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
	if (g_ss_monitor->aoa_reset.event_state)
		send_usb_err_uevent(USB_ERR_ABNORMAL_RESET, NOTIFY);
	else
		send_usb_err_uevent(USB_ERR_ABNORMAL_RESET, RELEASE);
#endif
}

void vbus_session_notify(struct usb_gadget *gadget, int is_active, int ret)
{
	if (!g_ss_monitor)
		return;

	if (ret == EAGAIN) {
		g_ss_monitor->vbus_session_called = true;
		return;
	} else if (g_ss_monitor->vbus_session_called) {
		pr_info("[USB] %s : vbus_session %s run_stop(%s)\n",
			__func__, is_active ? "On" : "Off", ret ? "fail" : "success");
		g_ss_monitor->vbus_session_called = false;
		if (is_active) {
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
			if (!ret)
				store_usblog_notify(NOTIFY_USBSTATE,
					(void *)"USB_STATE=VBUS:EN:SUCCESS", NULL);
			else
				store_usblog_notify(NOTIFY_USBSTATE,
					(void *)"USB_STATE=VBUS:EN:FAIL", NULL);
#endif
		} else {

			if (g_ss_monitor->aoa_reset.rst_err_noti) {
				g_ss_monitor->aoa_reset.event_state = RELEASE;
				g_ss_monitor->aoa_reset.rst_err_noti = false;
				schedule_delayed_work(&g_ss_monitor->aoa_reset.usb_reset_event_work,
					msecs_to_jiffies(0));
			}
			g_ss_monitor->aoa_reset.rst_err_cnt = 0;
			g_ss_monitor->aoa_reset.acc_dev_status = 0;

#ifdef CONFIG_USB_NOTIFY_PROC_LOG
			if (!ret)
				store_usblog_notify(NOTIFY_USBSTATE,
					(void *)"USB_STATE=VBUS:DIS:SUCCESS", NULL);
			else
				store_usblog_notify(NOTIFY_USBSTATE,
					(void *)"USB_STATE=VBUS:DIS:FAIL", NULL);
#endif
		}
	}
}
EXPORT_SYMBOL(vbus_session_notify);

void usb_reset_notify(struct usb_gadget *gadget)
{
	ktime_t current_time;

	if (!g_ss_monitor)
		return;

#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	switch (gadget->speed) {
	case USB_SPEED_SUPER:
		store_usblog_notify(NOTIFY_USBSTATE,
			(void *)"USB_STATE=RESET:SUPER", NULL);
		break;
	case USB_SPEED_HIGH:
		store_usblog_notify(NOTIFY_USBSTATE,
			(void *)"USB_STATE=RESET:HIGH", NULL);
		break;
	case USB_SPEED_FULL:
		store_usblog_notify(NOTIFY_USBSTATE,
			(void *)"USB_STATE=RESET:FULL", NULL);
		break;
	default:
		break;
	}
#endif

	if (g_ss_monitor->ss_monitor == NULL)
		return;

	g_ss_monitor->ss_monitor->vbus_current = USB_CURRENT_UNCONFIGURED;
	schedule_work(&g_ss_monitor->ss_monitor->set_vbus_current_work);

	if (g_ss_monitor->aoa_reset.acc_dev_status && (g_ss_monitor->aoa_reset.rst_err_noti == false)) {
		current_time = ktime_to_ms(ktime_get_boottime());

		if (g_ss_monitor->aoa_reset.rst_err_cnt == 0) {
			if ((current_time - g_ss_monitor->aoa_reset.rst_time_before) < 1000) {
				g_ss_monitor->aoa_reset.rst_err_cnt++;
				g_ss_monitor->aoa_reset.rst_time_first = g_ss_monitor->aoa_reset.rst_time_before;
			}
		} else {
			if ((current_time - g_ss_monitor->aoa_reset.rst_time_first) < 1000)
				g_ss_monitor->aoa_reset.rst_err_cnt++;
			else
				g_ss_monitor->aoa_reset.rst_err_cnt = 0;
		}

		if (g_ss_monitor->aoa_reset.rst_err_cnt >= ERR_RESET_CNT) {
			g_ss_monitor->aoa_reset.event_state = NOTIFY;
			schedule_delayed_work(&g_ss_monitor->aoa_reset.usb_reset_event_work, msecs_to_jiffies(0));
			g_ss_monitor->aoa_reset.rst_err_noti = true;
		}

		pr_info("[USB] %s rst_err_cnt: %d, time_current: %lld, time_before: %lld\n",
			__func__, g_ss_monitor->aoa_reset.rst_err_cnt, current_time,
			g_ss_monitor->aoa_reset.rst_time_before);

		g_ss_monitor->aoa_reset.rst_time_before = current_time;
	}

}
EXPORT_SYMBOL(usb_reset_notify);

static int ss_monitor_setup(struct usb_function *f,
				const struct usb_ctrlrequest *ctrl)
{
	struct f_ss_monitor	*ss_monitor;
	struct usb_composite_dev *cdev;
	struct usb_request	*req;

#if defined(CONFIG_USB_NOTIFY_PROC_LOG)
	char log_buf[30] = {0};
	char *usb_speed = NULL;
#endif
	int	value = -EOPNOTSUPP;
	u16	w_index = le16_to_cpu(ctrl->wIndex);
	u16	w_value = le16_to_cpu(ctrl->wValue);
	u16	w_length = le16_to_cpu(ctrl->wLength);

	ss_monitor = func_to_ss_monitor(f);

	if (f->config != NULL && f->config->cdev != NULL) {
		cdev = f->config->cdev;
		req = cdev->req;
	} else
		goto unknown;

	if (ctrl->bRequestType ==
			(USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE)
			&& ctrl->bRequest == USB_REQ_GET_DESCRIPTOR
			&& (w_value >> 8) == USB_DT_STRING
			&& (w_value & 0xFF) == MTPG_OS_STRING_ID) {

		pr_info("[USB] %s: MS OS String Descriptor\n", __func__);
	} else if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR) {
		/* Usb EP0 Vendor Requset */
		if (ctrl->bRequestType & USB_DIR_IN) {
			pr_info("[USB] %s: vendor requset: %02x.%02x v%04x i%04x l%u\n",
					__func__, ctrl->bRequestType, ctrl->bRequest,
					w_value, w_index, w_length);
		} else {
			/* samsung AVD operation: GUID for MTP */
			if (ctrl->bRequest == 0xA3) {
				pr_info("[USB] %s: (mtp) RECEIVE PC GUID / line[%d]\n",
							__func__, __LINE__);
				if (w_length > MAX_GUID_SIZE) {
					pr_info("usb: [%s] Invalid GUID size / line[%d]\n",
								__func__, __LINE__);
					goto unknown;
				}

				value = w_length;
				req->complete = mtp_complete_get_guid;
				req->zero = 0;
				req->length = value;
				value = usb_ep_queue(cdev->gadget->ep0,
							req, GFP_ATOMIC);

				if (value < 0) {
					pr_err("[USB] %s: (mtp) Error usb_ep_queue / line[%d]\n",
								__func__, __LINE__);
				}
			} else {
				if (ctrl->bRequest == ACCESSORY_START) {
					ss_monitor->aoa_start_cmd = 1;
#if defined(CONFIG_USB_NOTIFY_PROC_LOG)
					if (!is_aoa_string_come(ss_monitor->accessory_string))
						store_usblog_notify(NOTIFY_USBMODE_EXTRA,
							(void *)"AOA_STR_ERR", NULL);
					else
						store_usblog_notify(NOTIFY_USBSTATE,
							(void *)"ACCESSORY=START", NULL);
#endif
				} else if (ctrl->bRequest == ACCESSORY_SEND_STRING) {
					ss_monitor->accessory_string |= 0x1 << w_index;
				}

				pr_info("[USB] %s: vendor requset: %02x.%02x v%04x i%04x l%u\n",
						__func__, ctrl->bRequestType, ctrl->bRequest,
						w_value, w_index, w_length);
			}
		}

	} else if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
		switch (ctrl->bRequest) {
		case USB_REQ_GET_DESCRIPTOR:
			if (ctrl->bRequestType != USB_DIR_IN)
				goto unknown;
#if defined(CONFIG_USB_NOTIFY_PROC_LOG)
			switch (w_value >> 8) {
			case USB_DT_DEVICE:
				if (w_length == USB_DT_DEVICE_SIZE) {
					usb_speed = get_usb_speed(cdev->gadget->speed);
					sprintf(log_buf, "USB_STATE=ENUM:GET:DES:%s", usb_speed);
					pr_info("[USB] %s: GET_DES(%s)\n", __func__, usb_speed);
					store_usblog_notify(NOTIFY_USBSTATE,
						(void *)"USB_STATE=ENUM:GET:DES", NULL);
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
					set_usb_enumeration_state(cdev->gadget->speed);
#endif
				}
				break;
			case USB_DT_CONFIG:
				pr_info("[USB] %s: GET_CONFIG (%d)\n", __func__, w_index);
					break;
			}
#endif
			break;
		case USB_REQ_SET_CONFIGURATION:
#if defined(CONFIG_USB_NOTIFY_PROC_LOG)
			pr_info("[USB] %s: SET_CONFIG (%d)\n", __func__, w_value);
			if (cdev->gadget->speed >= USB_SPEED_SUPER)
				ss_monitor->vbus_current = USB_CURRENT_SUPER_SPEED;
			else
				ss_monitor->vbus_current = USB_CURRENT_HIGH_SPEED;
			schedule_work(&ss_monitor->set_vbus_current_work);
			store_usblog_notify(NOTIFY_USBSTATE,
				(void *)"USB_STATE=ENUM:SET:CON", NULL);
#endif
			if (ss_monitor->usb_function_info & GADGET_ACCESSORY) {
				g_ss_monitor->aoa_reset.acc_online = true;
				g_ss_monitor->aoa_reset.rst_err_cnt = 0;
			}
			break;
		}
	}
unknown:
	return value;
}

void make_suspend_current_event(void)
{
	struct usb_function f;

	pr_info("[USB] %s\n", __func__);

	if (!g_ss_monitor) {
		pr_info("[USB] %s, g_ss_monitor is NULL\n", __func__);
		return;
	}

	if (g_ss_monitor->ss_monitor && g_ss_monitor->ss_monitor->is_bind) {
		f = g_ss_monitor->ss_monitor->function;
		if (f.config->cdev && f.config->cdev->suspended) {
			pr_info("[USB] %s, cdev->suspended!\n", __func__);
			g_ss_monitor->ss_monitor->vbus_current = USB_CURRENT_SUSPENDED;
			schedule_work(&g_ss_monitor->ss_monitor->set_vbus_current_work);
		}
	}
}
EXPORT_SYMBOL(make_suspend_current_event);

static void ss_monitor_suspend(struct usb_function *f)
{
	struct f_ss_monitor *ss_monitor;

	pr_info("[USB] %s: usb suspend enter\n", __func__);
	ss_monitor = func_to_ss_monitor(f);
	ss_monitor->vbus_current = USB_CURRENT_SUSPENDED;
	schedule_work(&ss_monitor->set_vbus_current_work);
}

static void ss_monitor_resume(struct usb_function *f)
{
	struct f_ss_monitor *ss_monitor;
	struct usb_composite_dev *cdev = f->config->cdev;

	pr_info("[USB] %s: usb suspend exit\n", __func__);
	ss_monitor = func_to_ss_monitor(f);
	if (cdev->gadget->speed >= USB_SPEED_SUPER)
		ss_monitor->vbus_current = USB_CURRENT_SUPER_SPEED;
	else
		ss_monitor->vbus_current = USB_CURRENT_HIGH_SPEED;
	schedule_work(&ss_monitor->set_vbus_current_work);
}

static bool ss_monitor_req_match(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl, bool config)
{
	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR)
		if (ctrl->bRequest == 0xA3)
			return true;

	return false;
}

#if defined(CONFIG_USB_NOTIFY_PROC_LOG)
static void update_usb_gadet_function(char *f_name, struct f_ss_monitor *p_monitor)
{
	if (!strcmp(f_name, "mtp")) {
		p_monitor->usb_function_info |= GADGET_MTP;
		goto ret;
	}
	if (!strcmp(f_name, "acm")) {
		p_monitor->usb_function_info |= GADGET_ACM;
		goto ret;
	}
	if (!strcmp(f_name, "conn_gadget")) {
		p_monitor->usb_function_info |= GADGET_CONN_GADGET;
		goto ret;
	}

	if (!strcmp(f_name, "Function FS Gadget") || !strcmp(f_name, "adb")) {
		p_monitor->usb_function_info |= GADGET_ADB;
		goto ret;
	}
	if (!strcmp(f_name, "rndis")) {
		p_monitor->usb_function_info |= GADGET_RNDIS;
		goto ret;
	}
	if (!strcmp(f_name, "accessory")) {
		p_monitor->usb_function_info |= GADGET_ACCESSORY;
		goto ret;
	}
	if (!strcmp(f_name, "gmidi function")) {
		p_monitor->usb_function_info |= GADGET_MIDI;
		goto ret;
	}
	if (!strcmp(f_name, "dm") || !strcmp(f_name, "diag") || !strcmp(f_name, "diag_mdm")) {
		p_monitor->usb_function_info |= GADGET_DM;
		goto ret;
	}
ret:
	return;
}

static int copy_usb_mode(char *f_name, char *usb_mode)
{
	int length;

	length = strlen(f_name);
	if (length > 3) {
		length = 3;
		strncat(usb_mode, f_name, 3);
	} else {
		strncat(usb_mode, f_name, length);
	}
	length += 1;
	strcat(usb_mode, ",");

	return length;
}

static int usb_configuration_name(struct usb_configuration *config, struct usb_function *f_ss)
{
	struct usb_function		*f;
	struct ss_monitor_instance *opts;
	int length = 0;
	char *f_name;
	int name_len;
	int use_ffs_num = 0;
	struct f_ss_monitor *ss_monitor = func_to_ss_monitor(f_ss);

	opts = container_of(f_ss->fi, struct ss_monitor_instance, func_inst);

	if (!strcmp(opts->name, "mtp") || !strcmp(opts->name, "ptp") || !strcmp(opts->name, "diag1"))
		use_ffs_num = 1;
	else if (!strcmp(opts->name, "diag2"))
		use_ffs_num = 2;

	list_for_each_entry(f, &config->functions, list) {
		if (!strcmp(f->name, "ss_mon")) {
			break;
		} else if (!strcmp(f->name, "Function FS Gadget")) {
			if (use_ffs_num) {
				use_ffs_num--;
				if (use_ffs_num < 0)
					use_ffs_num = 0;
				name_len = strlen(opts->name);
				f_name = kstrndup(opts->name, name_len, GFP_KERNEL);
				if (!f_name)
					return -ENOMEM;
			} else {
				name_len = 3;
				f_name = kmemdup_nul("adb", name_len, GFP_KERNEL);
				if (!f_name)
					return -ENOMEM;
			}
		} else {
			if (!strcmp(f->name, "mtp") || !strcmp(f->name, "ptp") || !strcmp(f->name, "diag"))
				use_ffs_num = 0;
			name_len = strlen(f->name);
			if (name_len > MAX_INST_NAME_LEN)
				return -ENAMETOOLONG;

			f_name = kstrndup(f->name, name_len, GFP_KERNEL);
			if (!f_name)
				return -ENOMEM;
		}
		update_usb_gadet_function(f_name, ss_monitor);
		if (name_len > 3)
			name_len = 3;
		if (length + name_len > sizeof(ss_monitor->usb_mode)) {
			pr_info("[USB] %s: overflow usb mode buffer\n", __func__);
			break;
		}
		length += copy_usb_mode(f_name, ss_monitor->usb_mode);
		kfree(f_name);
	}

	if (length)
		ss_monitor->usb_mode[length-1] = 0;
	else
		strncat(ss_monitor->usb_mode, "func:empty", 11);

	store_usblog_notify(NOTIFY_USBMODE_EXTRA, (void *)ss_monitor->usb_mode, NULL);
	pr_info("[USB] %s: usb_mode: %s\n", __func__, ss_monitor->usb_mode);

	return 0;
}
#endif

static int
ss_monitor_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_ss_monitor		*ss_monitor = func_to_ss_monitor(f);
	struct ss_monitor_instance *opts;
	int	rc = -1;

	opts = container_of(f->fi, struct ss_monitor_instance, func_inst);

	/* copy descriptors, and track endpoint copies */
	f->fs_descriptors = usb_copy_descriptors(log_fs_function);

	/* support all relevant hardware speeds... we expect that when
	 * hardware is dual speed, all bulk-capable endpoints work at
	 * both speeds
	 */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		/* copy descriptors, and track endpoint copies */
		f->hs_descriptors = usb_copy_descriptors(log_hs_function);
	}

	if (gadget_is_superspeed(c->cdev->gadget)) {
		/* copy descriptors, and track endpoint copies */
		f->ss_descriptors = usb_copy_descriptors(log_ss_function);
		if (!f->ss_descriptors)
			goto fail;

		/* copy descriptors, and track endpoint copies for SSP */
		f->ssp_descriptors = usb_copy_descriptors(log_ss_function);
		if (!f->ssp_descriptors)
			goto fail;
	}

	/* save usb mode information */
#if defined(CONFIG_USB_NOTIFY_PROC_LOG)
	usb_configuration_name(c, f);
	store_usblog_notify(NOTIFY_USBSTATE,
		(void *)"USB_STATE=PULLUP:EN:SUCCESS", NULL);
#endif
	INIT_WORK(&ss_monitor->set_vbus_current_work, set_vbus_current_work);

#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	set_usb_enable_state();
#endif

	f->setup = ss_monitor_setup;
	ss_monitor->is_bind = 1;
	pr_info("[USB] %s: ss_mon.%s bind\n", __func__, opts->name);
	return 0;

fail:
#if defined(CONFIG_USB_NOTIFY_PROC_LOG)
	store_usblog_notify(NOTIFY_USBSTATE,
		(void *)"USB_STATE=PULLUP:EN:FAIL", NULL);
#endif
	pr_err("[USB] %s: ss_mon.%s bind fail\n", __func__, opts->name);
	return rc;

}

static void
ss_monitor_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_ss_monitor *ss_monitor = func_to_ss_monitor(f);
	struct ss_monitor_instance *opts;

	opts = container_of(f->fi, struct ss_monitor_instance, func_inst);
	f->setup = NULL;
#if defined(CONFIG_USB_NOTIFY_PROC_LOG)
	store_usblog_notify(NOTIFY_USBSTATE,
		(void *)"USB_STATE=PULLUP:DIS", NULL);
#endif
	opts->aoa_reset.rst_err_cnt = 0;
	ss_monitor->usb_function_info = 0;
	ss_monitor->is_bind = 0;

	cancel_delayed_work_sync(&opts->aoa_reset.usb_reset_event_work);
	flush_work(&ss_monitor->set_vbus_current_work);

	pr_info("[USB] %s: ss_mon.%s unbind\n", __func__, opts->name);
}

static struct ss_monitor_instance *to_ss_monitor_instance(struct config_item *item)
{
	return container_of(to_config_group(item), struct ss_monitor_instance,
		func_inst.group);
}

static void ss_monitor_attr_release(struct config_item *item)
{
	struct ss_monitor_instance *fi_ss_monitor = to_ss_monitor_instance(item);

	usb_put_function_instance(&fi_ss_monitor->func_inst);
}

static struct configfs_item_operations ss_monitor_item_ops = {
	.release        = ss_monitor_attr_release,
};

static struct config_item_type ss_monitor_func_type = {
	.ct_item_ops    = &ss_monitor_item_ops,
	.ct_owner       = THIS_MODULE,
};

static int ss_monitor_set_inst_name(struct usb_function_instance *fi, const char *name)
{
	struct ss_monitor_instance *fi_ss_monitor;
	char *ptr;
	int name_len;

	name_len = strlen(name) + 1;
	if (name_len > MAX_INST_NAME_LEN)
		return -ENAMETOOLONG;

	ptr = kstrndup(name, name_len, GFP_KERNEL);
	if (!ptr)
		return -ENOMEM;

	fi_ss_monitor = to_fi_ss_monitor(fi);
	fi_ss_monitor->name = ptr;

	return 0;
}

static void ss_monitor_free_inst(struct usb_function_instance *fi)
{
	struct ss_monitor_instance *fi_ss_monitor;

	fi_ss_monitor = to_fi_ss_monitor(fi);
	if (the_mtpg) {
		device_remove_file(mtpg_device.this_device, &dev_attr_guid);
		misc_deregister(&mtpg_device);
		kfree(the_mtpg);
		the_mtpg = NULL;
	}
	kfree(fi_ss_monitor->name);
	kfree(fi_ss_monitor);
}

static struct usb_function_instance *ss_mon_alloc_inst(void)
{
	struct ss_monitor_instance *fi_ss_monitor;
	int rc = -ENODEV;

	/* scenario for MTP  */
	if (the_mtpg == NULL) {
		rc = misc_register(&mtpg_device);
		if (rc != 0) {
			pr_err("[USB] %s misc_register of mtpg Failed(%d)\n", __func__, rc);
		} else {
			rc = device_create_file(mtpg_device.this_device, &dev_attr_guid);
			if (rc) {
				pr_err("[USB] %s failed to create guid attr\n", __func__);
				misc_deregister(&mtpg_device);
			} else {
				the_mtpg = kzalloc(sizeof(*the_mtpg), GFP_KERNEL);
				if (!the_mtpg) {
					device_remove_file(mtpg_device.this_device, &dev_attr_guid);
					misc_deregister(&mtpg_device);
				}
			}
		}
	}

	fi_ss_monitor = kzalloc(sizeof(*fi_ss_monitor), GFP_KERNEL);
	if (!fi_ss_monitor)
		goto err_misc_deregister;

	fi_ss_monitor->func_inst.set_inst_name = ss_monitor_set_inst_name;
	fi_ss_monitor->func_inst.free_func_inst = ss_monitor_free_inst;

	config_group_init_type_name(&fi_ss_monitor->func_inst.group,
					"", &ss_monitor_func_type);

	INIT_DELAYED_WORK(&fi_ss_monitor->aoa_reset.usb_reset_event_work, ss_mon_usb_event_work);
	g_ss_monitor = fi_ss_monitor;

	return  &fi_ss_monitor->func_inst;

err_misc_deregister:
	if (the_mtpg) {
		device_remove_file(mtpg_device.this_device, &dev_attr_guid);
		misc_deregister(&mtpg_device);
		kfree(the_mtpg);
		the_mtpg = NULL;
	}
	pr_err("[USB] %s: ss_mon_alloc_inst fail\n", __func__);
	return ERR_PTR(-ENOMEM);

}

static void ss_monitor_free(struct usb_function *f)
{
	struct f_ss_monitor	*ss_monitor = func_to_ss_monitor(f);

	g_ss_monitor->ss_monitor = NULL;
	kfree(ss_monitor);
}

static struct usb_function *ss_mon_alloc(struct usb_function_instance *fi)
{
	struct ss_monitor_instance *fi_ss_monitor = to_fi_ss_monitor(fi);
	struct f_ss_monitor *ss_monitor;

	/* allocate and initialize one new instance */
	ss_monitor = kzalloc(sizeof(*ss_monitor), GFP_KERNEL);
	if (!ss_monitor)
		return ERR_PTR(-ENOMEM);

	ss_monitor->function.name = "ss_mon";
	ss_monitor->function.bind = ss_monitor_bind;
	ss_monitor->function.unbind = ss_monitor_unbind;
	ss_monitor->function.set_alt = ss_monitor_set_alt;
	ss_monitor->function.disable = ss_monitor_disable;
	ss_monitor->function.free_func = ss_monitor_free;
	// ss_monitor->function.setup = ss_monitor_setup;
	ss_monitor->function.suspend = ss_monitor_suspend;
	ss_monitor->function.resume  = ss_monitor_resume;
	ss_monitor->function.req_match = ss_monitor_req_match;
	fi_ss_monitor->ss_monitor = ss_monitor;
	ss_monitor->func_inst = fi_ss_monitor;
	g_ss_monitor->ss_monitor = ss_monitor;

	return &ss_monitor->function;
}

DECLARE_USB_FUNCTION_INIT(ss_mon, ss_mon_alloc_inst, ss_mon_alloc);
MODULE_AUTHOR("Samsung USB Team");
MODULE_DESCRIPTION("usb device monitor gadget");
MODULE_LICENSE("GPL");