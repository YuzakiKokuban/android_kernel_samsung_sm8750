#include <linux/device.h>
#include <linux/module.h>

#include "usb_vendor_hook_receiver.h"
#include <linux/usb_notify.h>
#include <trace/hooks/usb.h>

#ifdef CONFIG_USB_AUDIO_POWER_SAVING
static void new_device_added(void *unused, struct usb_device *udev, int *err)
{
	*err = check_new_device_added(udev);
}
#endif

static int __init usb_vendor_hook_receiver_init(void)
{
	int ret = 0;

#ifdef CONFIG_USB_AUDIO_POWER_SAVING
	ret = register_trace_android_vh_usb_new_device_added(new_device_added, NULL);
	if (ret) {
		pr_err("%s: failed to register new device added ret = %d\n", __func__, ret);
		goto out;
	}
#endif
out:
	return ret;
}

static void __exit usb_vendor_hook_receiver_exit(void)
{
#ifdef CONFIG_USB_AUDIO_POWER_SAVING
	unregister_trace_android_vh_usb_new_device_added(new_device_added, NULL);
#endif
}

module_init(usb_vendor_hook_receiver_init);
module_exit(usb_vendor_hook_receiver_exit);

MODULE_AUTHOR("Samsung USB Team");
MODULE_DESCRIPTION("USB vendor hook receiver");
MODULE_LICENSE("GPL");
