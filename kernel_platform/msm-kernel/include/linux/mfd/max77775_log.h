/*
 * max77775_log.h - Driver for the Maxim 77775
 *
 */

#ifndef __MAX77775_LOG_H__
#define __MAX77775_LOG_H__

#ifdef CONFIG_USB_NOTIFY_PROC_LOG
#include <linux/usblog_proc_notify.h>
#endif

#ifdef CONFIG_USB_USING_ADVANCED_USBLOG
#define md75_info_usb(fmt, ...)				\
	({										\
		pr_info(fmt, ##__VA_ARGS__);		\
		printk_usb(NOTIFY_PRINTK_USB_NORMAL, fmt, ##__VA_ARGS__);	\
	})
#define md75_err_usb(fmt, ...)				\
	({										\
		pr_err(fmt, ##__VA_ARGS__);			\
		printk_usb(NOTIFY_PRINTK_USB_NORMAL, fmt, ##__VA_ARGS__);	\
	})
#else
#define md75_info_usb(fmt, ...)				\
	({										\
		pr_info(fmt, ##__VA_ARGS__);		\
	})
#define md75_err_usb(fmt, ...)				\
	({										\
		pr_err(fmt, ##__VA_ARGS__);			\
	})
#endif

#endif /* __MAX77775_LOG_H__ */
