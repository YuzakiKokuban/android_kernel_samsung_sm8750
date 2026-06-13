/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __USB_VENDOR_NOTIFY_H
#define __USB_VENDOR_NOTIFY_H

#include <linux/usb.h>

int send_usb_vendor_notify_pcm_info(int direction, int enable);
int send_usb_vendor_notify_cardnum(int card_num, int bundle, int attach);
int send_usb_vendor_notify_audio_uevent(struct usb_device *dev, int card_num,
		int attach);
int send_usb_vendor_notify_new_device(struct usb_device *dev);

#endif /* __USB_VENDOR_NOTIFY_H */
