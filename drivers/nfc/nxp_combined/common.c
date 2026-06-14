/******************************************************************************
 * Copyright (C) 2015, The Linux Foundation. All rights reserved.
 * Copyright (C) 2019-2021 NXP
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ******************************************************************************/
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#ifdef CONFIG_NFC_SN2XX_ESE_SUPPORT
#include "p73.h"
#endif
#endif
#include "common_ese.h"

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
static int nfc_param_lpcharge = LPM_NO_SUPPORT;
module_param(nfc_param_lpcharge, int, 0440);

struct nfc_dev *g_nfc_dev;
static bool g_is_nfc_pvdd_enabled;
#endif

#if IS_ENABLED(CONFIG_SEC_NFC_EINT_EXYNOS)
extern void esca_init_eint_nfc_clk_req(u32 eint_num);
#endif

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
enum lpm_status nfc_get_lpcharge(void)
{
	return nfc_param_lpcharge;
}

void nfc_set_i2c_pinctrl(struct device *dev, char *pinctrl_name)
{
	struct device_node *np = dev->of_node;
	struct device_node *i2c_np = of_get_parent(np);
	struct platform_device *i2c_pdev;
	struct pinctrl *pinctrl_i2c;

	i2c_pdev = of_find_device_by_node(i2c_np);
	if (!i2c_pdev) {
		NFC_LOG_ERR("i2c pdev not found\n");
		return;
	}

	pinctrl_i2c = devm_pinctrl_get_select(&i2c_pdev->dev, pinctrl_name);

	if (IS_ERR_OR_NULL(pinctrl_i2c)) {
		NFC_LOG_ERR("No %s pinctrl %ld\n", pinctrl_name, PTR_ERR(pinctrl_i2c));
	} else {
		devm_pinctrl_put(pinctrl_i2c);
		NFC_LOG_INFO("%s pinctrl done\n", pinctrl_name);
	}
}
#endif

#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
struct nfc_dev g_nfc_dev_for_chrdev;

void nfc_parse_dt_for_platform_device(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct platform_configs *nfc_configs = &g_nfc_dev_for_chrdev.configs;
	struct platform_gpio *nfc_gpio = &nfc_configs->gpio;

	nfc_configs->late_pvdd_en = of_property_read_bool(np, "pn547,late_pvdd_en");
	NFC_LOG_INFO("late_pvdd_en :%d, lpcharge :%d\n", nfc_configs->late_pvdd_en, nfc_get_lpcharge());
	if (nfc_get_lpcharge() == LPM_FALSE)
		nfc_configs->late_pvdd_en = false;

	if (nfc_configs->late_pvdd_en) {
		nfc_gpio->ven = of_get_named_gpio(np, DTS_VEN_GPIO_STR, 0);
		if (!gpio_is_valid(nfc_gpio->ven))
			NFC_LOG_ERR("%s: ven gpio invalid %d\n", __func__, nfc_gpio->ven);
		else
			configure_gpio(nfc_gpio->ven, GPIO_OUTPUT);

		nfc_configs->nfc_pvdd = regulator_get(dev, "nfc_pvdd");
		if (IS_ERR(nfc_configs->nfc_pvdd))
			NFC_LOG_ERR("get nfc_pvdd error\n");
		else
			NFC_LOG_INFO("LDO nfc_pvdd: %pK, vol:%d\n",
				nfc_configs->nfc_pvdd, regulator_get_voltage(nfc_configs->nfc_pvdd));
	}
}
#endif

int nfc_parse_dt(struct device *dev, struct platform_configs *nfc_configs,
		 uint8_t interface)
{
	struct device_node *np = dev->of_node;
	struct platform_gpio *nfc_gpio = &nfc_configs->gpio;
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	const char *ap_str;
	static int retry_count = 3;
#endif

	if (!np) {
		NFC_LOG_ERR("%s: nfc of_node NULL\n", __func__);
		return -EINVAL;
	}

	nfc_gpio->irq = -EINVAL;
	nfc_gpio->dwl_req = -EINVAL;
	nfc_gpio->ven = -EINVAL;

	/* irq required for i2c based chips only */
	if (interface == PLATFORM_IF_I2C) {
		nfc_gpio->irq = of_get_named_gpio(np, DTS_IRQ_GPIO_STR, 0);
		if ((!gpio_is_valid(nfc_gpio->irq))) {
			NFC_LOG_ERR("%s: irq gpio invalid %d\n", __func__,
				nfc_gpio->irq);
			return -EINVAL;
		}
		NFC_LOG_INFO("%s: irq %d\n", __func__, nfc_gpio->irq);
	}
	nfc_gpio->ven = of_get_named_gpio(np, DTS_VEN_GPIO_STR, 0);
	if ((!gpio_is_valid(nfc_gpio->ven))) {
		NFC_LOG_ERR("%s: ven gpio invalid %d\n", __func__, nfc_gpio->ven);
		return -EINVAL;
	}
	/* some products like sn220 does not required fw dwl pin */
	nfc_gpio->dwl_req = of_get_named_gpio(np, DTS_FWDN_GPIO_STR, 0);
	if ((!gpio_is_valid(nfc_gpio->dwl_req)))
		NFC_LOG_ERR("%s: dwl_req gpio is not supported(%d)\n", __func__,
			nfc_gpio->dwl_req);

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	nfc_configs->late_pvdd_en = of_property_read_bool(np, "pn547,late_pvdd_en");

	nfc_configs->disable_clk_irq_during_wakeup =
		of_property_read_bool(np, "pn547,disable_clk_irq_during_wakeup");

	if (nfc_get_lpcharge() == LPM_FALSE)
		nfc_configs->late_pvdd_en = false;

	nfc_gpio->clk_req = of_get_named_gpio(np, "pn547,clk_req-gpio", 0);
	if ((!gpio_is_valid(nfc_gpio->clk_req)))
		NFC_LOG_ERR("nfc clk_req gpio invalid %d\n", nfc_gpio->clk_req);

	nfc_configs->clk_req_wake = of_property_read_bool(np, "pn547,clk_req_wake");

	if (of_property_read_bool(np, "pn547,clk_req_all_trigger")) {
		nfc_configs->clk_req_all_trigger = true;
		NFC_LOG_INFO("irq_all_trigger\n");
	}
	if (of_property_read_bool(np, "pn547,change_clkreq_for_acpm")) {
		nfc_configs->change_clkreq_for_acpm = true;
		NFC_LOG_INFO("change clkreq for acpm!!\n");
	}
#if IS_ENABLED(CONFIG_SEC_NFC_EINT_EXYNOS)
	/*slsi ap EINT mapping*/
	if (of_property_read_bool(np, "sec-nfc,clk_req_eint_mode")) {
		nfc_configs->clk_req_eint_mode = true;
		esca_init_eint_nfc_clk_req(nfc_gpio->clk_req);
		NFC_LOG_INFO("clk_req_eint_mode\n");
	}
#endif
	if (!of_property_read_string(np, "pn547,ap_vendor", &ap_str)) {
		if (!strcmp(ap_str, "slsi"))
			nfc_configs->ap_vendor = AP_VENDOR_SLSI;
		else if (!strcmp(ap_str, "qct") || !strcmp(ap_str, "qualcomm"))
			nfc_configs->ap_vendor = AP_VENDOR_QCT;
		else if (!strcmp(ap_str, "mtk"))
			nfc_configs->ap_vendor = AP_VENDOR_MTK;
	} else {
		NFC_LOG_INFO("AP vendor is not set\n");
	}
#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	if (nfc_configs->late_pvdd_en) {
		if (!IS_ERR_OR_NULL(g_nfc_dev_for_chrdev.configs.nfc_pvdd)) {
			nfc_configs->nfc_pvdd = g_nfc_dev_for_chrdev.configs.nfc_pvdd;
		} else {
			nfc_configs->nfc_pvdd = regulator_get(dev, "nfc_pvdd");
			if (!IS_ERR(nfc_configs->nfc_pvdd))
				g_nfc_dev_for_chrdev.configs.nfc_pvdd = nfc_configs->nfc_pvdd;
			NFC_LOG_INFO("retry to get platform pvdd\n");
		}
	} else {
		nfc_configs->nfc_pvdd = regulator_get(dev, "nfc_pvdd");
	}
#else
	nfc_configs->nfc_pvdd = regulator_get(dev, "nfc_pvdd");
#endif
	if (IS_ERR(nfc_configs->nfc_pvdd)) {
		NFC_LOG_ERR("get nfc_pvdd error\n");
		if (--retry_count > 0)
			return -EPROBE_DEFER;
		else
			return -ENODEV;
	} else {
		NFC_LOG_INFO("LDO nfc_pvdd: %pK, vol:%d\n",
				nfc_configs->nfc_pvdd, regulator_get_voltage(nfc_configs->nfc_pvdd));
	}

	if (of_property_read_bool(np, "pn547,always_on_pvdd")) {
		/* set late_pvdd_en to true so that nfc_en reset can be called in nfc_power_control */
		nfc_configs->late_pvdd_en = true;
		nfc_set_i2c_pinctrl(dev, "i2c_pull_up");
	}

	if (of_find_property(np, "clocks", NULL)) {
		nfc_configs->nfc_clock = clk_get(dev, "oscclk_nfc");
		if (IS_ERR(nfc_configs->nfc_clock)) {
			NFC_LOG_ERR("probe() clk not found\n");
			nfc_configs->nfc_clock = NULL;
		} else {
			NFC_LOG_INFO("found oscclk_nfc\n");
		}
	}
#endif

	NFC_LOG_INFO("%s: irq %d, ven %d, fw %d\n", __func__, nfc_gpio->irq, nfc_gpio->ven,
		nfc_gpio->dwl_req);
	return 0;
}

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
static int nfc_ocp_notifier(struct notifier_block *nb, unsigned long event, void *data);

int nfc_regulator_onoff(struct nfc_dev *nfc_dev, int onoff)
{
	int rc = 0;
	struct platform_configs *nfc_configs = &nfc_dev->configs;
	struct regulator *regulator_nfc_pvdd;

#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	/* if nfc probe is called before nfc platform device, nfc_configs->nfc_pvdd is NULL */
	if (!nfc_configs->nfc_pvdd)
		nfc_configs->nfc_pvdd = g_nfc_dev_for_chrdev.configs.nfc_pvdd;
#endif

	/* if regulator is not ready, try to get that again */
	if (IS_ERR_OR_NULL(nfc_configs->nfc_pvdd) && nfc_dev->i2c_dev.client) {
		NFC_LOG_INFO("%s retry to get regulator\n", __func__);
		nfc_configs->nfc_pvdd = regulator_get(&nfc_dev->i2c_dev.client->dev, "nfc_pvdd");
	}

	regulator_nfc_pvdd = nfc_configs->nfc_pvdd;

	if (IS_ERR_OR_NULL(regulator_nfc_pvdd)) {
		NFC_LOG_ERR("error at regulator!\n");
		rc = -ENODEV;
		goto done;
	}

	if (nfc_configs->ap_vendor == AP_VENDOR_QCT && !nfc_configs->ldo_ocp_nb.notifier_call) {
		nfc_configs->ldo_ocp_nb.notifier_call = nfc_ocp_notifier;
		devm_regulator_register_notifier(nfc_configs->nfc_pvdd, &nfc_configs->ldo_ocp_nb);
		NFC_LOG_INFO("%s register nfc ocp notifier\n", __func__);
	}

	NFC_LOG_INFO("pvdd onoff = %d, g_is_nfc_pvdd_enabled = %d\n",
		onoff, g_is_nfc_pvdd_enabled);

	if (g_is_nfc_pvdd_enabled == onoff) {
		NFC_LOG_INFO("%s already pvdd %s\n", __func__, onoff ? "enabled" : "disabled");
		goto done;
	}

	if (onoff) {
		rc = regulator_set_load(regulator_nfc_pvdd, 300000);
		if (rc) {
			NFC_LOG_ERR("regulator_uwb_vdd set_load failed, rc=%d\n", rc);
			goto done;
		}
		rc = regulator_enable(regulator_nfc_pvdd);
		if (rc) {
			NFC_LOG_ERR("enable failed, rc=%d\n", rc);
			goto done;
		}
	} else {
		rc = regulator_disable(regulator_nfc_pvdd);
		if (rc) {
			NFC_LOG_ERR("disable failed, rc=%d\n", rc);
			goto done;
		}
	}

	g_is_nfc_pvdd_enabled = !!onoff;

	NFC_LOG_INFO("success\n");
done:
	return rc;
}

bool nfc_check_pvdd_status(void)
{
	return g_is_nfc_pvdd_enabled;
}
#endif

void set_valid_gpio(int gpio, int value)
{
	if (gpio_is_valid(gpio)) {
		NFC_LOG_DBG("%s: gpio %d value %d\n", __func__, gpio, value);
		gpio_set_value(gpio, value);
		/* hardware dependent delay */
		usleep_range(NFC_GPIO_SET_WAIT_TIME_US,
				NFC_GPIO_SET_WAIT_TIME_US + 100);
	}
}

int get_valid_gpio(int gpio)
{
	int value = -EINVAL;

	if (gpio_is_valid(gpio)) {
		value = gpio_get_value(gpio);
		NFC_LOG_DBG("%s: gpio %d value %d\n", __func__, gpio, value);
	}
	return value;
}

void gpio_set_ven(struct nfc_dev *nfc_dev, int value)
{
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;

	if (gpio_get_value(nfc_gpio->ven) != value) {
		NFC_LOG_INFO("%s: value %d\n", __func__, value);
		/* reset on change in level from high to low */
		if (value)
			ese_cold_reset_release(nfc_dev);

		gpio_set_value(nfc_gpio->ven, value);
		/* hardware dependent delay */
		usleep_range(NFC_GPIO_SET_WAIT_TIME_US,
				NFC_GPIO_SET_WAIT_TIME_US + 100);
	} else {
		NFC_LOG_INFO("%s: already %d\n", __func__, value);
	}
}

int configure_gpio(unsigned int gpio, int flag)
{
	int ret;

	NFC_LOG_DBG("%s: nfc gpio [%d] flag [%01x]\n", __func__, gpio, flag);
	if (gpio_is_valid(gpio)) {
		ret = gpio_request(gpio, "nfc_gpio");
		if (ret) {
			NFC_LOG_ERR("%s: unable to request nfc gpio [%d]\n",
				__func__, gpio);
			return ret;
		}
		/* set direction and value for output pin */
		if (flag & GPIO_OUTPUT) {
			ret = gpio_direction_output(gpio, (GPIO_HIGH & flag));
			NFC_LOG_DBG("%s: nfc o/p gpio %d level %d\n", __func__,
				 gpio, gpio_get_value(gpio));
		} else {
			ret = gpio_direction_input(gpio);
			NFC_LOG_DBG("%s: nfc i/p gpio %d\n", __func__, gpio);
		}

		if (ret) {
			NFC_LOG_ERR("%s: unable to set direction for nfc gpio [%d]\n",
			       __func__, gpio);
			gpio_free(gpio);
			return ret;
		}
		/* Consider value as control for input IRQ pin */
		if (flag & GPIO_IRQ) {
			ret = gpio_to_irq(gpio);
			if (ret < 0) {
				NFC_LOG_ERR("%s: unable to set irq [%d]\n", __func__,
					gpio);
				gpio_free(gpio);
				return ret;
			}
			NFC_LOG_DBG("%s: gpio_to_irq successful [%d]\n", __func__,
				 gpio);
			return ret;
		}
	} else {
		NFC_LOG_ERR("%s: invalid gpio\n", __func__);
		ret = -EINVAL;
	}
	return ret;
}

void gpio_free_all(struct nfc_dev *nfc_dev)
{
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;

	if (gpio_is_valid(nfc_gpio->dwl_req))
		gpio_free(nfc_gpio->dwl_req);

	if (gpio_is_valid(nfc_gpio->irq))
		gpio_free(nfc_gpio->irq);

	if (gpio_is_valid(nfc_gpio->ven))
		gpio_free(nfc_gpio->ven);
}

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
void nfc_power_control(struct nfc_dev *nfc_dev)
{
	int ret;

	ret = nfc_regulator_onoff(nfc_dev, 1);
	if (ret < 0)
		NFC_LOG_ERR("%s pn547 regulator_on fail err = %d\n", __func__, ret);

	nfc_set_i2c_pinctrl(&nfc_dev->i2c_dev.client->dev, "i2c_pull_up");
#ifdef CONFIG_NFC_SN2XX_ESE_SUPPORT
	ese_set_spi_pinctrl_for_ese_off(NULL);
#endif
	usleep_range(15000, 20000); /* spec : VDDIO high -> 15~20 ms -> VEN high*/

	gpio_set_ven(nfc_dev, 1);
	gpio_set_ven(nfc_dev, 0);
	gpio_set_ven(nfc_dev, 1);
}

static int nfc_ocp_notifier(struct notifier_block *nb, unsigned long event, void *data)
{
	if (event == REGULATOR_EVENT_OVER_CURRENT)
		NFC_LOG_ERR("NFC power OCP\n");

	return NOTIFY_OK;
}

#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
static ssize_t nfc_support_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
#else
static ssize_t nfc_support_show(struct class *class,
		struct class_attribute *attr, char *buf)
#endif
{
	NFC_LOG_INFO("\n");
	return 0;
}
static CLASS_ATTR_RO(nfc_support);

#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
static ssize_t pvdd_store(const struct class *class,
	const struct class_attribute *attr, const char *buf, size_t size)
#else
static ssize_t pvdd_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
#endif
{
	struct nfc_dev *nfc_dev = g_nfc_dev;
	struct platform_configs *nfc_configs;

#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	if (!g_nfc_dev) {
		nfc_dev = &g_nfc_dev_for_chrdev;
		NFC_LOG_INFO("%s called before nfc probe\n", __func__);
	}
#endif
	if (!nfc_dev) {
		NFC_LOG_ERR("%s nfc_dev is NULL!\n", __func__);
		return size;
	}

	nfc_configs = &nfc_dev->configs;

	NFC_LOG_INFO("%s val: %c, late_pvdd_en: %d\n", __func__, buf[0], nfc_configs->late_pvdd_en);

	if (buf[0] == '1' && nfc_configs->late_pvdd_en)
		nfc_power_control(nfc_dev);

	return size;
}
static CLASS_ATTR_WO(pvdd);

#ifdef CONFIG_SAMSUNG_NFC_DEBUG
#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
static ssize_t check_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
#else
static ssize_t check_show(struct class *class,
		struct class_attribute *attr, char *buf)
#endif
{
	struct nfc_dev *nfc_dev = g_nfc_dev;
	char *cmd = nfc_dev->write_kbuf;
	char *rsp = nfc_dev->read_kbuf;
	int timeout = NCI_CMD_RSP_TIMEOUT_MS;
	int size = 0;
	int ret;
	int cmd_length = 4;

	if (!nfc_check_pvdd_status()) {
		NFC_LOG_ERR("Turn on PVDD first\n");
		size = snprintf(buf, SZ_64, "Turn on PVDD first\n");
		goto end;
	}
	mutex_lock(&nfc_dev->write_mutex);
	*cmd++ = 0x20;
	*cmd++ = 0x00;
	*cmd++ = 0x01;
	*cmd++ = 0x00;

	ret = nfc_dev->nfc_write(nfc_dev, nfc_dev->write_kbuf, cmd_length, MAX_RETRY_COUNT);
	if (ret != cmd_length) {
		ret = -EIO;
		NFC_LOG_ERR("%s: nfc_write returned %d\n", __func__, ret);
		size = snprintf(buf, SZ_64, "nfc_write returned %d. count : %d\n",
			ret, cmd_length);
		mutex_unlock(&nfc_dev->write_mutex);
		goto end;
	}
	mutex_unlock(&nfc_dev->write_mutex);

	/* Read data */
	mutex_lock(&nfc_dev->read_mutex);
	cmd_length = 6;
	ret = nfc_dev->nfc_read(nfc_dev, rsp, cmd_length, timeout);

	if (ret < 0 || ret > cmd_length) {
		NFC_LOG_ERR("%s: nfc_read returned %d\n", __func__, ret);
		size = snprintf(buf, SZ_64, "nfc_read returned %d. count : %d\n",
			ret, cmd_length);
		mutex_unlock(&nfc_dev->read_mutex);
		goto end;
	}
	mutex_unlock(&nfc_dev->read_mutex);

	size = snprintf(buf, SZ_64, "test completed!! size: %d, data: %X %X %X %X %X %X\n",
		ret, rsp[0], rsp[1], rsp[2], rsp[3], rsp[4], rsp[5]);
end:
	return size;
}

#ifdef FEATURE_SEC_NFC_TEST
extern void nfc_check_is_core_reset_ntf(u8 *data, int len);
#endif
#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
static ssize_t check_store(const struct class *class,
	const struct class_attribute *attr, const char *buf, size_t size)
#else
static ssize_t check_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t size)
#endif
{
	if (size > 0) {
		if (buf[0] == '1') {
			NFC_LOG_INFO("%s: test\n", __func__);
			nfc_print_status();
		}
#ifdef FEATURE_SEC_NFC_TEST
		else if (buf[0] == '2') {
			u8 header[3] = {0x60, 0x00, 0x06};
			u8 data[10] = {0x0, };

			nfc_check_is_core_reset_ntf(header, 3);
			data[0] = 0xA0;
			nfc_check_is_core_reset_ntf(data, 6);
		}
#endif
	}

	return size;
}
static CLASS_ATTR_RW(check);
#endif
#endif

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
void nfc_probe_done(struct nfc_dev *nfc_dev)
{
	g_nfc_dev = nfc_dev;
}
#endif

void nfc_misc_unregister(struct nfc_dev *nfc_dev, int count)
{
	NFC_LOG_DBG("%s: entry\n", __func__);
#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	if (nfc_dev == NULL)
		nfc_dev = &g_nfc_dev_for_chrdev;
#endif
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	class_remove_file(nfc_dev->nfc_class, &class_attr_nfc_support);
	class_remove_file(nfc_dev->nfc_class, &class_attr_pvdd);
#ifdef CONFIG_SAMSUNG_NFC_DEBUG
	class_remove_file(nfc_dev->nfc_class, &class_attr_check);
#endif
#endif
	device_destroy(nfc_dev->nfc_class, nfc_dev->devno);
	cdev_del(&nfc_dev->c_dev);
	class_destroy(nfc_dev->nfc_class);
	unregister_chrdev_region(nfc_dev->devno, count);
}

int nfc_misc_register(struct nfc_dev *nfc_dev,
			const struct file_operations *nfc_fops, int count,
			char *devname, char *classname)
{
	int ret = 0;

#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	if (nfc_dev == NULL)
		nfc_dev = &g_nfc_dev_for_chrdev;
#endif

	ret = alloc_chrdev_region(&nfc_dev->devno, 0, count, devname);
	if (ret < 0) {
		NFC_LOG_ERR("%s: failed to alloc chrdev region ret %d\n", __func__,
			ret);
		return ret;
	}
#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
	nfc_dev->nfc_class = class_create(classname);
#else
	nfc_dev->nfc_class = class_create(THIS_MODULE, classname);
#endif
	if (IS_ERR(nfc_dev->nfc_class)) {
		ret = PTR_ERR(nfc_dev->nfc_class);
		NFC_LOG_ERR("%s: failed to register device class ret %d\n", __func__,
			ret);
		unregister_chrdev_region(nfc_dev->devno, count);
		return ret;
	}

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	ret = class_create_file(nfc_dev->nfc_class, &class_attr_nfc_support);
	if (ret)
		NFC_LOG_ERR("failed to create nfc_support file\n");

	ret = class_create_file(nfc_dev->nfc_class, &class_attr_pvdd);
	if (ret)
		NFC_LOG_ERR("failed to create nfc_support file\n");

#ifdef CONFIG_SAMSUNG_NFC_DEBUG
	ret = class_create_file(nfc_dev->nfc_class, &class_attr_check);
	if (ret)
		NFC_LOG_ERR("failed to create test file\n");
#endif
#endif

	cdev_init(&nfc_dev->c_dev, nfc_fops);
	ret = cdev_add(&nfc_dev->c_dev, nfc_dev->devno, count);
	if (ret < 0) {
		NFC_LOG_ERR("%s: failed to add cdev ret %d\n", __func__, ret);
		class_destroy(nfc_dev->nfc_class);
		unregister_chrdev_region(nfc_dev->devno, count);
		return ret;
	}
	nfc_dev->nfc_device = device_create(nfc_dev->nfc_class, NULL,
					    nfc_dev->devno, nfc_dev, devname);
	if (IS_ERR(nfc_dev->nfc_device)) {
		ret = PTR_ERR(nfc_dev->nfc_device);
		NFC_LOG_ERR("%s: failed to create the device ret %d\n", __func__,
			ret);
		cdev_del(&nfc_dev->c_dev);
		class_destroy(nfc_dev->nfc_class);
		unregister_chrdev_region(nfc_dev->devno, count);
		return ret;
	}
	return 0;
}

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
void nfc_print_status(void)
{
	struct nfc_dev *nfc_dev = g_nfc_dev;
	struct platform_configs *nfc_configs;
	struct platform_gpio *nfc_gpio;
	int en, firm, irq, pvdd = 0;
	int clk_req_irq = -1;

	if (nfc_dev == NULL)
		return;

	nfc_configs = &nfc_dev->configs;
	nfc_gpio = &nfc_dev->configs.gpio;

	en = get_valid_gpio(nfc_gpio->ven);
	firm = get_valid_gpio(nfc_gpio->dwl_req);
	irq = get_valid_gpio(nfc_gpio->irq);
	if (!IS_ERR_OR_NULL(nfc_configs->nfc_pvdd))
		pvdd = regulator_is_enabled(nfc_configs->nfc_pvdd);
	else
		NFC_LOG_ERR("nfc_pvdd is null\n");

	clk_req_irq = get_valid_gpio(nfc_gpio->clk_req);

	NFC_LOG_INFO("en: %d, firm: %d, pvdd: %d, irq: %d, clk_req: %d\n",
		en, firm, pvdd, irq, clk_req_irq);
#ifdef CONFIG_NFC_SN2XX_ESE_SUPPORT
	p61_print_status(__func__);
#endif
#ifdef CONFIG_SEC_NFC_LOGGER_ADD_ACPM_LOG
	nfc_logger_acpm_log_print();
#endif
}
#endif
/**
 ** nfc_gpio_info() - gets the status of nfc gpio pins and encodes into a byte.
 ** @nfc_dev:	nfc device data structure
 ** @arg:		userspace buffer
 **
 ** Encoding can be done in following manner
 ** 1) map the gpio value into INVALID(-2), SET(1), RESET(0).
 ** 2) mask the first 2 bits of gpio.
 ** 3) left shift the 2 bits as multiple of 2.
 ** 4) multiply factor can be defined as position of gpio pin in struct platform_gpio
 **
 ** Return: -EFAULT, if unable to copy the data from kernel space to userspace, 0
 ** if Success(or no issue)
 **/

static int nfc_gpio_info(struct nfc_dev *nfc_dev, unsigned long arg)
{
	unsigned int gpios_status = 0;
	int value = 0;
	int gpio_no = 0;
	int i;
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;

#if !IS_ENABLED(CONFIG_SAMSUNG_NFC)
	for (i = 0; i < sizeof(struct platform_gpio) / sizeof(unsigned int); i++)
#else
	for (i = 0; i < PLATFORM_DEFAULT_GPIO_CNT; i++)
#endif
	{
		gpio_no = *((unsigned int *)nfc_gpio + i);
		value = get_valid_gpio(gpio_no);
		if (value < 0)
			value = -2;
		gpios_status |= (value & GPIO_STATUS_MASK_BITS)<<(GPIO_POS_SHIFT_VAL*i);
	}
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	nfc_print_status();
#endif
	if (copy_to_user((uint32_t *) arg, &gpios_status, sizeof(value))) {
		pr_err("%s : Unable to copy data from kernel space to user space\n", __func__);
		return -EFAULT;
	}
	return 0;
}

/**
 * nfc_ioctl_power_states() - power control
 * @nfc_dev:    nfc device data structure
 * @arg:    mode that we want to move to
 *
 * Device power control. Depending on the arg value, device moves to
 * different states, refer common.h for args
 *
 * Return: -ENOIOCTLCMD if arg is not supported, 0 if Success(or no issue)
 * and error ret code otherwise
 */
static int nfc_ioctl_power_states(struct nfc_dev *nfc_dev, unsigned long arg)
{
	int ret = 0;
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;

	if (arg == NFC_POWER_OFF) {
		/*
		 * We are attempting a hardware reset so let us disable
		 * interrupts to avoid spurious notifications to upper
		 * layers.
		 */
		nfc_dev->nfc_disable_intr(nfc_dev);
		set_valid_gpio(nfc_gpio->dwl_req, 0);
		gpio_set_ven(nfc_dev, 0);
		nfc_dev->nfc_ven_enabled = false;
	} else if (arg == NFC_POWER_ON) {
		nfc_dev->nfc_enable_intr(nfc_dev);
		set_valid_gpio(nfc_gpio->dwl_req, 0);

		gpio_set_ven(nfc_dev, 1);
		nfc_dev->nfc_ven_enabled = true;
	} else if (arg == NFC_FW_DWL_VEN_TOGGLE) {
		/*
		 * We are switching to download Mode, toggle the enable pin
		 * in order to set the NFCC in the new mode
		 */
		nfc_dev->nfc_disable_intr(nfc_dev);
		set_valid_gpio(nfc_gpio->dwl_req, 1);
		nfc_dev->nfc_state = NFC_STATE_FW_DWL;
		gpio_set_ven(nfc_dev, 0);
		gpio_set_ven(nfc_dev, 1);
		nfc_dev->nfc_enable_intr(nfc_dev);
	} else if (arg == NFC_FW_DWL_HIGH) {
		/*
		 * Setting firmware download gpio to HIGH
		 * before FW download start
		 */
		set_valid_gpio(nfc_gpio->dwl_req, 1);
		nfc_dev->nfc_state = NFC_STATE_FW_DWL;

	} else if (arg == NFC_VEN_FORCED_HARD_RESET) {
		nfc_dev->nfc_disable_intr(nfc_dev);
		gpio_set_ven(nfc_dev, 0);
		gpio_set_ven(nfc_dev, 1);
		nfc_dev->nfc_enable_intr(nfc_dev);
	} else if (arg == NFC_FW_DWL_LOW) {
		/*
		 * Setting firmware download gpio to LOW
		 * FW download finished
		 */
		set_valid_gpio(nfc_gpio->dwl_req, 0);
		nfc_dev->nfc_state = NFC_STATE_NCI;
	} else {
		NFC_LOG_ERR("%s: bad arg %lu\n", __func__, arg);
		ret = -ENOIOCTLCMD;
	}
	return ret;
}

#ifdef CONFIG_COMPAT
/**
 * nfc_dev_compat_ioctl - used to set or get data from upper layer.
 * @pfile   file node for opened device.
 * @cmd     ioctl type from upper layer.
 * @arg     ioctl arg from upper layer.
 *
 * NFC and ESE Device power control, based on the argument value
 *
 * Return: -ENOIOCTLCMD if arg is not supported
 * 0 if Success(or no issue)
 * 0 or 1 in case of arg is ESE_GET_PWR/ESE_POWER_STATE
 * and error ret code otherwise
 */
long nfc_dev_compat_ioctl(struct file *pfile, unsigned int cmd,
		      unsigned long arg)
{
	int ret = 0;

	arg = (compat_u64)arg;
	NFC_LOG_DBG("%s: cmd = %x arg = %zx\n", __func__, cmd, arg);
	ret = nfc_dev_ioctl(pfile, cmd, arg);
	return ret;
}
#endif

/**
 * nfc_dev_ioctl - used to set or get data from upper layer.
 * @pfile   file node for opened device.
 * @cmd     ioctl type from upper layer.
 * @arg     ioctl arg from upper layer.
 *
 * NFC and ESE Device power control, based on the argument value
 *
 * Return: -ENOIOCTLCMD if arg is not supported
 * 0 if Success(or no issue)
 * 0 or 1 in case of arg is ESE_GET_PWR/ESE_POWER_STATE
 * and error ret code otherwise
 */
long nfc_dev_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct nfc_dev *nfc_dev = pfile->private_data;

	if (!nfc_dev)
		return -ENODEV;

	switch (cmd) {
	case NFC_SET_PWR:
		NFC_LOG_INFO("%s: NFC_SET_PWR %lu\n", __func__, arg);
		ret = nfc_ioctl_power_states(nfc_dev, arg);
		break;
	case ESE_SET_PWR:
		NFC_LOG_INFO("%s: ESE_SET_PWR %lu\n", __func__, arg);
		ret = nfc_ese_pwr(nfc_dev, arg);
		break;
	case ESE_GET_PWR:
		ret = nfc_ese_pwr(nfc_dev, ESE_POWER_STATE);
		NFC_LOG_INFO("%s: ESE_GET_PWR %d\n", __func__, ret);
		break;
	case NFC_GET_GPIO_STATUS:
		ret = nfc_gpio_info(nfc_dev, arg);
		NFC_LOG_INFO("%s: NFC_GET_GPIO_STATUS %d\n", __func__, ret);
		break;
	default:
		NFC_LOG_ERR("%s: bad cmd 0x%x %lu\n", __func__, cmd, arg);
		ret = -ENOIOCTLCMD;
	};

	return ret;
}

int nfc_dev_open(struct inode *inode, struct file *filp)
{
	struct nfc_dev *nfc_dev = NULL;
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	struct platform_configs *nfc_configs;
#endif

#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	nfc_dev = g_nfc_dev;
#else
	nfc_dev = container_of(inode->i_cdev, struct nfc_dev, c_dev);
#endif
	if (!nfc_dev)
		return -ENODEV;

	NFC_LOG_INFO("%s: %d, %d\n", __func__, imajor(inode), iminor(inode));

	mutex_lock(&nfc_dev->dev_ref_mutex);

	filp->private_data = nfc_dev;

	if (nfc_dev->dev_ref_count == 0) {
		set_valid_gpio(nfc_dev->configs.gpio.dwl_req, 0);

		nfc_dev->nfc_enable_intr(nfc_dev);
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
		nfc_configs = &nfc_dev->configs;
		if (!nfc_configs->disable_clk_irq_during_wakeup)
			nfc_dev->nfc_enable_clk_intr(nfc_dev);
#endif
	}
	nfc_dev->dev_ref_count = nfc_dev->dev_ref_count + 1;
	mutex_unlock(&nfc_dev->dev_ref_mutex);
	return 0;
}

int nfc_dev_flush(struct file *pfile, fl_owner_t id)
{
	struct nfc_dev *nfc_dev = pfile->private_data;

	if (!nfc_dev)
		return -ENODEV;
	/*
	 * release blocked user thread waiting for pending read during close
	 */
	if (!mutex_trylock(&nfc_dev->read_mutex)) {
		nfc_dev->release_read = true;
		nfc_dev->nfc_disable_intr(nfc_dev);
		wake_up(&nfc_dev->read_wq);
		NFC_LOG_DBG("%s: waiting for release of blocked read\n", __func__);
		mutex_lock(&nfc_dev->read_mutex);
		nfc_dev->release_read = false;
	} else {
		NFC_LOG_DBG("%s: read thread already released\n", __func__);
	}
	mutex_unlock(&nfc_dev->read_mutex);
	return 0;
}

int nfc_dev_close(struct inode *inode, struct file *filp)
{
	struct nfc_dev *nfc_dev = NULL;

#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	nfc_dev = g_nfc_dev;
#else
	nfc_dev = container_of(inode->i_cdev, struct nfc_dev, c_dev);
#endif
	if (!nfc_dev)
		return -ENODEV;

	NFC_LOG_INFO("%s: %d, %d\n", __func__, imajor(inode), iminor(inode));
	mutex_lock(&nfc_dev->dev_ref_mutex);
	if (nfc_dev->dev_ref_count == 1) {
		nfc_dev->nfc_disable_intr(nfc_dev);
		set_valid_gpio(nfc_dev->configs.gpio.dwl_req, 0);
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
		nfc_dev->nfc_disable_clk_intr(nfc_dev);
#endif
	}
	if (nfc_dev->dev_ref_count > 0)
		nfc_dev->dev_ref_count = nfc_dev->dev_ref_count - 1;
	else {
		/*
		 * Use "ESE_RST_PROT_DIS" as argument
		 * if eSE calls flow is via NFC driver
		 * i.e. direct calls from SPI HAL to NFC driver
		 */
		nfc_ese_pwr(nfc_dev, ESE_RST_PROT_DIS_NFC);
	}
	filp->private_data = NULL;

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	nfc_print_status();
#endif

	mutex_unlock(&nfc_dev->dev_ref_mutex);
	return 0;
}

int validate_nfc_state_nci(struct nfc_dev *nfc_dev)
{
	struct platform_gpio *nfc_gpio = &nfc_dev->configs.gpio;

	if (!gpio_get_value(nfc_gpio->ven)) {
		NFC_LOG_ERR("%s: ven low - nfcc powered off\n", __func__);
		return -ENODEV;
	}
	if (get_valid_gpio(nfc_gpio->dwl_req) == 1) {
		NFC_LOG_ERR("%s: fw download in-progress\n", __func__);
		return -EBUSY;
	}
	if (nfc_dev->nfc_state != NFC_STATE_NCI) {
		NFC_LOG_ERR("%s: fw download state\n", __func__);
		return -EBUSY;
	}
	return 0;
}
