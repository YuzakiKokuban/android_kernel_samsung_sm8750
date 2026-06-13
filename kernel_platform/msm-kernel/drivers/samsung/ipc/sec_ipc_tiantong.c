#include <linux/platform_device.h>
#include <linux/refcount.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/idr.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/termios.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>

#include "sec_ipc_tiantong.h"

static struct class *tiantong_class;
static struct cdev tiantong_cdev;
static dev_t dev_num;
static struct tiantong_gpio tt_gpio;
static int gpio_chn_ht;
static bool gpio_chn_ht_exists;

static const struct of_device_id tiantong_control_match_table[] = {
	{ .compatible = "sylin,tiantong-control"},
	{},
};

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = tiantong_open,
	.release = tiantong_close,
	.unlocked_ioctl = tiantong_ioctl,
};

static int tiantong_init_cdev(void)
{
	struct device *dev_struct;
	int ret = 0;

	pr_info("%s: ++\n", __func__);

	ret = alloc_chrdev_region(&dev_num, MINOR_BASE, MINOR_NUM, DEVICE_NAME);
	if (ret < 0) {
		pr_err("%s: failed to allocate device num for %s, error:%d\n", __func__, DEVICE_NAME, ret);
		return ret;
	}

	cdev_init(&tiantong_cdev, &fops);

	ret = cdev_add(&tiantong_cdev, dev_num, MINOR_NUM);
	if (ret < 0) {
		pr_err("%s: failed to add a cdev struct. error:%d\n", __func__, ret);
		goto unreg_device_num;
	}

	tiantong_class = class_create(DEVICE_NAME);
	if (IS_ERR(tiantong_class)) {
		pr_err("%s: failed to create a class struct\n", __func__);
		ret = -1;
		goto unreg_cdev;
	}

	dev_struct = device_create(tiantong_class, NULL, dev_num, NULL, DEVICE_NAME);
	if (IS_ERR(dev_struct)) {
		pr_err("%s: failed to create a device file\n", __func__);
		ret = -2;
		goto unreg_class;
	}

	pr_info("%s: Major: %d, Minor:%d\n", __func__, MAJOR(dev_num), MINOR(dev_num));
	pr_info("%s: --\n", __func__);
	return 0;

unreg_class:
	class_destroy(tiantong_class);

unreg_cdev:
	cdev_del(&tiantong_cdev);

unreg_device_num:
	unregister_chrdev_region(MKDEV(dev_num, MINOR_BASE), MINOR_NUM);

	pr_info("%s: init cdev failed --\n", __func__);

	return ret;
}

static int tiantong_init_gpio(struct platform_device *pdev)
{
	struct device_node *np;
	int ret = 0;

	pr_info("%s ++\n", __func__);
	np = pdev->dev.of_node;

	tt_gpio.bootmode1 = of_get_named_gpio(np, "sylin,bootmode-gpio", 0);
	if (!gpio_is_valid(tt_gpio.bootmode1)) {
		pr_err("%s: bootmode-gpio is not valid: %d\n", __func__, tt_gpio.bootmode1);
		return -EINVAL;
	}

	tt_gpio.reset = of_get_named_gpio(np, "sylin,reset-gpio", 0);
	if (!gpio_is_valid(tt_gpio.reset)) {
		pr_err("%s: reset_gpio is not valid: %d\n", __func__, tt_gpio.reset);
		return -EINVAL;
	}

	tt_gpio.ap2cp_wakeup = of_get_named_gpio(np, "sylin,ap2cp-wakeup-gpio", 0);
	if (!gpio_is_valid(tt_gpio.ap2cp_wakeup)) {
		pr_err("%s: ap2cp_wakeup_gpio is not valid: %d\n", __func__, tt_gpio.ap2cp_wakeup);
		return -EINVAL;
	}

	tt_gpio.cp2ap_wakeup = of_get_named_gpio(np, "sylin,cp2ap-wakeup-gpio", 0);
	if (!gpio_is_valid(tt_gpio.cp2ap_wakeup)) {
		pr_err("%s: cp2ap_wakeup_gpio is not valid: %d\n", __func__, tt_gpio.cp2ap_wakeup);
		return -EINVAL;
	}

	tt_gpio.vrf_1p8 = of_get_named_gpio(np, "sylin,vrf-tiantong-1p8", 0);
	if (!gpio_is_valid(tt_gpio.vrf_1p8)) {
		pr_err("%s: vrf-tiantong-1p8 is not valid: %d\n", __func__, tt_gpio.vrf_1p8);
		return -EINVAL;
	}

	tt_gpio.vrf_1p8_2 = of_get_named_gpio(np, "sylin,vrf-tiantong-1p8-2", 0);
	if (!gpio_is_valid(tt_gpio.vrf_1p8_2)) {
		pr_err("%s: sylin,vrf-tiantong-1p8-2 is not valid: %d\n", __func__, tt_gpio.vrf_1p8_2);
		return -EINVAL;
	}

	tt_gpio.vrf_0p8 = of_get_named_gpio(np, "sylin,vrf-tiantong-0p8", 0);
	if (!gpio_is_valid(tt_gpio.vrf_0p8)) {
		pr_err("%s: vrf-tiantong-0p8 is not valid: %d\n", __func__, tt_gpio.vrf_0p8);
		return -EINVAL;
	}

	gpio_chn_ht = of_get_named_gpio(np, "sylin,gpio-chn-ht", 0);
	if (!gpio_is_valid(gpio_chn_ht)) {
		pr_info("%s: gpio_chn_ht is not valid: %d\n", __func__, gpio_chn_ht);
		gpio_chn_ht_exists = 0;
	} else
		gpio_chn_ht_exists = 1;

	ret = gpio_request_one(tt_gpio.bootmode1, GPIOF_OUT_INIT_LOW, tiantong_bootmode1_str);
	if (ret < 0) {
		pr_err("%s: request bootmode failed:%d. gpio num:%d\n", __func__, ret, tt_gpio.bootmode1);
		return ret;
	}

	ret = gpio_request_one(tt_gpio.reset, GPIOF_OUT_INIT_LOW, tiantong_reset_str);
	if (ret < 0) {
		pr_err("%s: request reset_gpio failed:%d. gpio num:%d\n", __func__, ret, tt_gpio.reset);
		return ret;
	}

	ret = gpio_request_one(tt_gpio.ap2cp_wakeup, GPIOF_OUT_INIT_LOW, tiantong_ap2cp_wakeup_str);
	if (ret < 0) {
		pr_err("%s: request ap2cp_wakeup_gpio failed:%d. gpio num:%d\n", __func__, ret, tt_gpio.ap2cp_wakeup);
		return ret;
	}

	ret = gpio_request_one(tt_gpio.cp2ap_wakeup, GPIOF_IN, tiantong_cp2ap_wakeup_str);
	if (ret < 0) {
		pr_err("%s: request cp2ap_wakeup_gpio failed:%d. gpio num:%d\n", __func__, ret, tt_gpio.cp2ap_wakeup);
		return ret;
	}

	ret = gpio_request_one(tt_gpio.vrf_1p8, GPIOF_OUT_INIT_LOW, tiantong_vrf_1p8_str);
	if (ret < 0) {
		pr_err("%s: request vrf_1p8 failed:%d. gpio num:%d\n", __func__, ret, tt_gpio.vrf_1p8);
		return ret;
	}

	ret = gpio_request_one(tt_gpio.vrf_1p8_2, GPIOF_OUT_INIT_LOW, tiantong_vrf_1p8_2_str);
	if (ret < 0) {
		pr_err("%s: request vrf_1p8_2 failed:%d. gpio num:%d\n", __func__, ret, tt_gpio.vrf_1p8_2);
		return ret;
	}

	ret = gpio_request_one(tt_gpio.vrf_0p8, GPIOF_OUT_INIT_LOW, tiantong_vrf_0p8_str);
	if (ret < 0) {
		pr_err("%s: request vrf_0p8 failed:%d. gpio num:%d\n", __func__, ret, tt_gpio.vrf_0p8);
		return ret;
	}

	if (gpio_chn_ht_exists) {
		ret = gpio_request(gpio_chn_ht, gpio_chn_ht_str);
		if (ret < 0) {
			pr_err("%s: request gpio_chn_ht failed:%d. gpio num:%d\n", __func__, ret, gpio_chn_ht);
			return ret;
		}
	}

	pr_info("%s: --\n", __func__);

	return 0;
}

static inline void tiantong_print_gpio(void)
{
	pr_info("%s: bootmode1: %d, reset: %d, vrf_1p8: %d, vrf_1p8_2: %d, vrf_0p8: %d\n",
		__func__, gpio_get_value(tt_gpio.bootmode1), gpio_get_value(tt_gpio.reset),
		gpio_get_value(tt_gpio.vrf_1p8), gpio_get_value(tt_gpio.vrf_1p8_2),
		gpio_get_value(tt_gpio.vrf_0p8));
}

static int tiantong_power_on(void)
{
	pr_info("%s: ++ power on tiantong modem\n", __func__);

	tiantong_print_gpio();

	gpio_set_value(tt_gpio.vrf_1p8, 1);
	gpio_set_value(tt_gpio.vrf_1p8_2, 1);
	gpio_set_value(tt_gpio.bootmode1, 1);

	mdelay(2);
	gpio_set_value(tt_gpio.vrf_0p8, 1);

	mdelay(1);
	gpio_set_value(tt_gpio.reset, 0);
	mdelay(2);
	gpio_set_value(tt_gpio.reset, 1);

	mdelay(1);
	tiantong_print_gpio();

	pr_info("%s: -- power on tiantong modem done\n", __func__);

	return 0;
}

static int tiantong_power_off(void)
{
	pr_info("%s: ++ power off tiantong modem\n", __func__);

	tiantong_print_gpio();

	gpio_set_value(tt_gpio.reset, 0);
	mdelay(2);

	gpio_set_value(tt_gpio.vrf_0p8, 0);
	mdelay(2);

	gpio_set_value(tt_gpio.vrf_1p8, 0);
	gpio_set_value(tt_gpio.vrf_1p8_2, 0);
	gpio_set_value(tt_gpio.bootmode1, 0);

	mdelay(1);
	tiantong_print_gpio();

	pr_info("%s: -- power off tiantong modem done\n", __func__);

	return 0;
}

static int tiantong_reset(void)
{
	pr_info("%s: ++ reset tiantong modem\n", __func__);

	tiantong_print_gpio();

	gpio_set_value(tt_gpio.reset, 0);
	mdelay(1);
	gpio_set_value(tt_gpio.reset, 1);

	mdelay(1);
	tiantong_print_gpio();

	pr_info("%s: -- reset tiantong modem done\n", __func__);

	return 0;
}

static long tiantong_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int val = 0;
	int ret;

	switch (cmd) {
	case TT_BOOT_MODE:
		ret = copy_from_user(&val, (void __user *)arg, sizeof(int));
		if (ret != 0) {
			pr_err("%s: cmd:TT_BOOT_MODE ret:%d\n", __func__, ret);
			return -EFAULT;
		}
		pr_info("%s: cmd TT_BOOT_MODE, val:%d\n", __func__, val);
		gpio_set_value(tt_gpio.bootmode1, val);
		break;
	case TT_RESET_N:
		ret = copy_from_user(&val, (void __user *)arg, sizeof(int));
		if (ret != 0) {
			pr_err("%s: cmd: TT_RESET_N ret:%d\n", __func__, ret);
			return -EFAULT;
		}
		pr_info("%s: cmd TT_RESET_N, val:%d\n", __func__, val);
		gpio_set_value(tt_gpio.reset, val);
		break;
	case TT_AP2CP_WAKE:
		ret = copy_from_user(&val, (void __user *)arg, sizeof(int));
		if (ret != 0) {
			pr_err("%s: cmd:TT_AP2CP_WAKE ret:%d\n", __func__, ret);
			return -EFAULT;
		}
		pr_info("%s: cmd TT_AP2CP_WAKE, val:%d\n", __func__, val);
		gpio_set_value(tt_gpio.ap2cp_wakeup, val);
		break;
	case TT_POWER_32K:
		ret = copy_from_user(&val, (void __user *)arg, sizeof(int));
		if (ret != 0) {
			pr_err("%s: cmd:TT_POWER_32K ret:%d\n", __func__, ret);
			return -EFAULT;
		}
		pr_info("%s: cmd TT_POWER_32K, val:%d\n", __func__, val);
		gpio_set_value(tt_gpio.vrf_1p8, val);
		gpio_set_value(tt_gpio.vrf_1p8_2, val);
		break;
	case TT_POWER_19P2M:
		ret = copy_from_user(&val, (void __user *)arg, sizeof(int));
		if (ret != 0) {
			pr_err("%s: cmd:TT_POWER_19P2M ret:%d\n", __func__, ret);
			return -EFAULT;
		}
		pr_info("%s: cmd TT_POWER_19P2M, val:%d\n", __func__, val);
		gpio_set_value(tt_gpio.vrf_0p8, val);
		break;
	case IOCTL_CHECK_REGION:
		if (gpio_chn_ht_exists)
			val = gpio_get_value(gpio_chn_ht);
		else
			val = 1;

		pr_info("%s: cmd IOCTL_CHECK_REGION :%d\n", __func__, val);
		if (copy_to_user((int __user *)arg, &val, sizeof(int)))
			return -EFAULT;
		break;
	case IOCTL_POWER_ON:
		pr_info("%s: cmd IOCTL_POWER_ON\n", __func__);
		tiantong_power_on();
		break;
	case IOCTL_POWER_OFF:
		pr_info("%s: cmd IOCTL_POWER_OFF\n", __func__);
		tiantong_power_off();
		break;
	case IOCTL_POWER_RESET:
		pr_info("%s: cmd IOCTL_POWER_RESET\n", __func__);
		tiantong_reset();
		break;
	default:
		pr_info("%s: UNKNOWN CMD:%d\n", __func__, cmd);
		return -EFAULT;
	}

	return 0;
}


static int tiantong_open(struct inode *inode, struct file *file)
{
	pr_info("%s: open /dev/%s\n", __func__, DEVICE_NAME);
	return 0;
}

static int tiantong_close(struct inode *inode, struct file *file)
{
	pr_info("%s: close /dev/%s\n", __func__, DEVICE_NAME);
	return 0;
}

static int tiantong_control_probe(struct platform_device *pdev)
{
	int ret = 0;

	pr_info("%s ++\n", __func__);

	ret = tiantong_init_gpio(pdev);
	if (ret < 0)
		pr_err("%s: init tiantong gpio error:%d\n", __func__, ret);

	ret = tiantong_init_cdev();
	if (ret < 0)
		pr_err("%s: init tiantong cdev error:%d\n", __func__, ret);

	pr_info("%s: --\n", __func__);

	return 0;
}

static int tiantong_control_remove(struct platform_device *pdev)
{
	dev_t dev = MKDEV(dev_num, MINOR_BASE);

	pr_info("%s: ++\n", __func__);

	gpio_free(tt_gpio.bootmode1);
	gpio_free(tt_gpio.reset);
	gpio_free(tt_gpio.ap2cp_wakeup);
	gpio_free(tt_gpio.cp2ap_wakeup);
	gpio_free(tt_gpio.vrf_1p8);
	gpio_free(tt_gpio.vrf_1p8_2);
	gpio_free(tt_gpio.vrf_0p8);
	if (gpio_chn_ht_exists)
		gpio_free(gpio_chn_ht);

	device_destroy(tiantong_class, dev_num);
	class_destroy(tiantong_class);
	cdev_del(&tiantong_cdev);
	unregister_chrdev_region(dev, MINOR_NUM);

	pr_info("%s: --\n", __func__);

	return 0;
}

static struct platform_driver tiantong_control_driver = {
	.probe = tiantong_control_probe,
	.remove = tiantong_control_remove,
	.driver = {
		.name = "sec_ipc_tiantong",
		.of_match_table = tiantong_control_match_table,
	 },
};

static int __init tiantong_control_init(void)
{
	int ret;

	pr_info("%s ++\n", __func__);

	ret = platform_driver_register(&tiantong_control_driver);
	if (ret) {
		pr_err("%s: platform register failed %d\n",
			__func__, ret);
		return ret;
	}
	pr_info("%s: --\n", __func__);

	return 0;
}

static void __exit tiantong_control_exit(void)
{
	platform_driver_unregister(&tiantong_control_driver);
}

module_init(tiantong_control_init);
module_exit(tiantong_control_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SAMSUNG Electronics");
MODULE_DESCRIPTION("Tiantong Control Driver");
