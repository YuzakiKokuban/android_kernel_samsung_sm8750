/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __USB_VENDOR_NOTIFY_DEFS_H
#define __USB_VENDOR_NOTIFY_DEFS_H

#include <linux/usb.h>

enum {
	USB_VENDOR_NOTIFY_NONE,
	USB_VENDOR_NOTIFY_PCM_INFO,
	USB_VENDOR_NOTIFY_CARDNUM,
	USB_VENDOR_NOTIFY_AUDIO_UEVENT,
	USB_VENDOR_NOTIFY_NEW_DEVICE,
};

/* USB_VENDOR_NOTIFY_PCM_INFO */
struct data_pcm_info {
	int direction;
	int enable;
};

/* USB_VENDOR_NOTIFY_CARDNUM */
struct data_cardnum {
	int card_num;
	int bundle;
	int attach;
};

/* USB_VENDOR_NOTIFY_AUDIO_UEVENT */
struct data_audio_uevent {
	struct usb_device *dev;
	int card_num;
	int attach;
};

/* USB_VENDOR_NOTIFY_NEW_DEVICE */
struct data_new_device {
	struct usb_device *dev;
	int ret;
};

#endif /* __USB_VENDOR_NOTIFY_DEFS_H */
