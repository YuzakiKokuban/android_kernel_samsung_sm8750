/*
 * connector_setup.c
 *
 * Copyright (C) 2017 Samsung Electronics
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
 */

#include <linux/sti/sec_abc_detect_conn.h>
#if defined(CONFIG_SEC_KUNIT)
#define __mockable __weak
#define __visible_for_testing
#define KUNIT_TEST_STRLEN 30
struct device *sec_abc_detect_conn_dev;
char kunit_test_str[KUNIT_TEST_STRLEN];
#else
#define __mockable
#define __visible_for_testing static
#endif

__visible_for_testing int gsec_abc_detect_conn_enabled;
struct sec_det_conn_info *gpinfo;

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id sec_abc_detect_conn_dt_match[] = {
	{ .compatible = "samsung,sec_abc_detect_conn" },
	{ }
};
#endif	//CONFIG_OF

#if IS_ENABLED(CONFIG_PM)
static int sec_abc_detect_conn_pm_suspend(struct device *dev)
{
	return 0;
}

static int sec_abc_detect_conn_pm_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops sec_abc_detect_conn_pm = {
	.suspend = sec_abc_detect_conn_pm_suspend,
	.resume = sec_abc_detect_conn_pm_resume,
};
#endif	//CONFIG_PM

/*
 * Send uevent from irq handler.
 */
void sec_abc_detect_send_uevent_by_num(int num,
				   struct sec_det_conn_info *pinfo,
				   int level)
{
	char *uevent_conn_str[3] = {"", "", NULL};
	char uevent_dev_str[UEVENT_CONN_MAX_DEV_NAME];
	char uevent_dev_type_str[UEVENT_CONN_MAX_DEV_NAME];

	/* Send Uevent Data */
	snprintf(uevent_dev_str, UEVENT_CONN_MAX_DEV_NAME, "CONNECTOR_NAME=%s",
		 pinfo->pdata->name[num]);
	uevent_conn_str[0] = uevent_dev_str;

	if (level == 1)
		snprintf(uevent_dev_type_str, UEVENT_CONN_MAX_DEV_NAME,
			 "CONNECTOR_TYPE=HIGH_LEVEL");
	else
		snprintf(uevent_dev_type_str, UEVENT_CONN_MAX_DEV_NAME,
			 "CONNECTOR_TYPE=LOW_LEVEL");

	uevent_conn_str[1] = uevent_dev_type_str;

	kobject_uevent_env(&pinfo->dev->kobj, KOBJ_CHANGE, uevent_conn_str);

	SEC_CONN_PRINT("send uevent pin[%d]:CONNECTOR_NAME=%s, TYPE=[%d].\n",
		       num, pinfo->pdata->name[num], level);
#if defined(CONFIG_SEC_KUNIT)
	snprintf(kunit_test_str, KUNIT_TEST_STRLEN, "CONNECTOR_TYPE=HIGH_LEVEL");
#endif
}

#if IS_ENABLED(CONFIG_SEC_FACTORY)
/*
 * Check gpio and send uevent.
 */
__visible_for_testing void check_gpio_and_send_uevent(int i,
				       struct sec_det_conn_info *pinfo)
{
	if (gpio_get_value(pinfo->pdata->irq_gpio[i])) {
		SEC_CONN_PRINT("%s status changed [disconnected]\n",
			       pinfo->pdata->name[i]);
		sec_abc_detect_send_uevent_by_num(i, pinfo, 1);
	} else {
		SEC_CONN_PRINT("%s status changed [connected]\n",
			       pinfo->pdata->name[i]);
		sec_abc_detect_send_uevent_by_num(i, pinfo, 0);
	}
}
#endif

#if IS_ENABLED(CONFIG_SEC_ABC_HUB) && IS_ENABLED(CONFIG_SEC_ABC_HUB_COND)
/**
 * Send an ABC_event about given gpio pin number.
 */
void send_ABC_event_by_num(int i, struct sec_det_conn_info *pinfo)
{
	char ABC_event_dev_str[ABCEVENT_CONN_MAX_DEV_STRING];

	if (sec_abc_get_enabled() == 0)
		return;

	/*Send ABC Event Data*/
	if (gpio_get_value(pinfo->pdata->irq_gpio[i])) {
#if IS_ENABLED(CONFIG_SEC_FACTORY)
		snprintf(ABC_event_dev_str, ABCEVENT_CONN_MAX_DEV_STRING,
			 "MODULE=cond@INFO=%s", pinfo->pdata->name[i]);
#else
		snprintf(ABC_event_dev_str, ABCEVENT_CONN_MAX_DEV_STRING,
			 "MODULE=cond@WARN=%s", pinfo->pdata->name[i]);
#endif
		sec_abc_send_event(ABC_event_dev_str);

		SEC_CONN_PRINT("send ABC cond event %s status is [disconnected] : %s\n",
			       pinfo->pdata->name[i], ABC_event_dev_str);
	} else {
		SEC_CONN_PRINT("do not send ABC cond event %s status is [connected]\n",
			       pinfo->pdata->name[i]);
	}
}
#endif

/*
 * Check and send uevent from irq handler.
 */
void sec_abc_detect_send_uevent_irq(int irq,
				struct sec_det_conn_info *pinfo,
				int type)
{
	int i;

	for (i = 0; i < pinfo->pdata->gpio_total_cnt; i++) {
		if (irq == pinfo->pdata->irq_number[i]) {
			/* apply s/w debounce time */
			usleep_range(DET_CONN_DEBOUNCE_TIME_MS * 1000, DET_CONN_DEBOUNCE_TIME_MS * 1000);
#if IS_ENABLED(CONFIG_SEC_FACTORY)
			check_gpio_and_send_uevent(i, pinfo);
#endif
#if IS_ENABLED(CONFIG_SEC_ABC_HUB) && IS_ENABLED(CONFIG_SEC_ABC_HUB_COND)
			send_ABC_event_by_num(i, pinfo);
#endif
			increase_connector_disconnected_count(i, pinfo);
		}
	}
}

/*
 * Called when the connector pin state changes.
 */
static irqreturn_t sec_abc_detect_conn_interrupt_handler(int irq, void *handle)
{
	int type;
	struct sec_det_conn_info *pinfo = handle;

	if (gsec_abc_detect_conn_enabled != 0) {
		SEC_CONN_PRINT("%s\n", __func__);

		type = irq_get_trigger_type(irq);
		sec_abc_detect_send_uevent_irq(irq, pinfo, type);
	}
	return IRQ_HANDLED;
}

/*
 * Enable all gpio pin IRQ which is from Device Tree.
 */
int detect_conn_irq_enable(struct sec_det_conn_info *pinfo, bool enable,
			   int pin)
{
	int ret = 0;
	int i;

	if (!enable) {
		for (i = 0; i < pinfo->pdata->gpio_total_cnt; i++) {
			if (pinfo->irq_enabled[i]) {
				disable_irq(pinfo->pdata->irq_number[i]);
				pinfo->irq_enabled[i] = DET_CONN_GPIO_IRQ_DISABLED;
			}
		}
		return ret;
	}

	if (pin >= pinfo->pdata->gpio_total_cnt)
		return ret;

	if (pinfo->irq_enabled[pin] == DET_CONN_GPIO_IRQ_NOT_INIT) {
		ret = request_threaded_irq(pinfo->pdata->irq_number[pin], NULL,
					   sec_abc_detect_conn_interrupt_handler,
					   pinfo->pdata->irq_type[pin] | IRQF_ONESHOT,
					   pinfo->pdata->name[pin], pinfo);

		if (ret) {
			SEC_CONN_PRINT("%s: Failed to request irq %d.\n", __func__,
				       ret);
			return ret;
		}

		SEC_CONN_PRINT("%s: Succeeded to request threaded irq %d:\n",
			       __func__, ret);
	} else if (pinfo->irq_enabled[pin] == DET_CONN_GPIO_IRQ_DISABLED) {
		enable_irq(pinfo->pdata->irq_number[pin]);
	}
	SEC_CONN_PRINT("irq_num[%d], type[%x],name[%s].\n",
		       pinfo->pdata->irq_number[pin],
		       pinfo->pdata->irq_type[pin], pinfo->pdata->name[pin]);

	pinfo->irq_enabled[pin] = DET_CONN_GPIO_IRQ_ENABLED;
	return ret;
}

/*
 * Check and send an uevent if the pin level is high.
 * And then enable gpio pin interrupt.
 */
__visible_for_testing int one_gpio_irq_enable(int i, struct sec_det_conn_p_data *pdata,
			       const char *buf)
{
	int ret = 0;

	SEC_CONN_PRINT("%s driver enabled.\n", buf);
	gsec_abc_detect_conn_enabled |= (1 << i);

	SEC_CONN_PRINT("gpio [%d] level %d\n", pdata->irq_gpio[i],
		       gpio_get_value(pdata->irq_gpio[i]));

	/*get level value of the gpio pin.*/
	/*if there's gpio low pin, send uevent*/
#if IS_ENABLED(CONFIG_SEC_FACTORY)
	check_gpio_and_send_uevent(i, pdata->pinfo);
#endif
#if IS_ENABLED(CONFIG_SEC_ABC_HUB) && IS_ENABLED(CONFIG_SEC_ABC_HUB_COND)
	send_ABC_event_by_num(i, pdata->pinfo);
#endif
	increase_connector_disconnected_count(i, pdata->pinfo);

	/*Enable interrupt.*/
	ret = detect_conn_irq_enable(pdata->pinfo, true, i);

	if (ret < 0)
		SEC_CONN_PRINT("%s Interrupt not enabled.\n", buf);

	return ret;
}


/*
 * Triggered when "enabled" node is set.
 * Check and send an uevent if the pin level is high.
 * And then gpio pin interrupt is enabled.
 */
__visible_for_testing ssize_t enabled_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct sec_det_conn_p_data *pdata;
	struct sec_det_conn_info *pinfo;
	int ret;
	int i;
	int buf_len;
	int pin_name_len;

	if (gpinfo == 0)
		return count;

	pinfo = gpinfo;
	pdata = pinfo->pdata;

	buf_len = strlen(buf);

	SEC_CONN_PRINT("buf = %s, buf_len = %d\n", buf, buf_len);

	/* disable irq when "enabled" value set to 0*/
	if (!strncmp(buf, "0", 1)) {
		SEC_CONN_PRINT("sec detect connector driver disable.\n");
		gsec_abc_detect_conn_enabled = 0;

		ret = detect_conn_irq_enable(pinfo, false, 0);

		if (ret) {
			SEC_CONN_PRINT("Interrupt not disabled.\n");
			return ret;
		}
		return count;
	}
	for (i = 0; i < pdata->gpio_total_cnt; i++) {
		pin_name_len = strlen(pdata->name[i]);
		SEC_CONN_PRINT("pinName = %s\n", pdata->name[i]);
		SEC_CONN_PRINT("pin_name_len = %d\n", pin_name_len);

		if (pin_name_len == buf_len) {
			if (!strncmp(buf, pdata->name[i], buf_len)) {
				/* buf sting is equal to pdata->name[i] */
				ret = one_gpio_irq_enable(i, pdata, buf);
				if (ret < 0)
					return ret;
			}
		}

		/*
		 * For ALL_CONNECT input,
		 * enable all nodes except already enabled node.
		 */
		if (buf_len >= 11) {
			if (strncmp(buf, "ALL_CONNECT", 11)) {
				/* buf sting is not equal to ALL_CONNECT */
				continue;
			}
			/* buf sting is equal to pdata->name[i] */
			if (!(gsec_abc_detect_conn_enabled & (1 << i))) {
				ret = one_gpio_irq_enable(i, pdata, buf);
				if (ret < 0)
					return ret;
			}
		}
	}
	return count;
}

__visible_for_testing ssize_t enabled_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	return snprintf(buf, 12, "%d\n", gsec_abc_detect_conn_enabled);
}
static DEVICE_ATTR_RW(enabled);

__visible_for_testing ssize_t available_pins_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sec_det_conn_p_data *pdata;
	struct sec_det_conn_info *pinfo;
	char available_pins_string[1024] = {0, };
	int i;

	if (gpinfo == 0)
		return 0;

	pinfo = gpinfo;
	pdata = pinfo->pdata;

	for (i = 0; i < pdata->gpio_total_cnt; i++) {
		SEC_CONN_PRINT("pinName = %s\n", pdata->name[i]);
		strlcat(available_pins_string, pdata->name[i], 1024);
		strlcat(available_pins_string, "/", 1024);
	}
	available_pins_string[strlen(available_pins_string) - 1] = '\0';

	return snprintf(buf, 1024, "%s\n", available_pins_string);
}
static DEVICE_ATTR_RO(available_pins);

/**
 * Fill out the gpio cnt
 */
static void fill_gpio_cnt(struct sec_det_conn_p_data *pdata, struct device *dev, char *gpio_name)
{
	int gpio_name_cnt;

	gpio_name_cnt = gpiod_count(dev, gpio_name);
	if (gpio_name_cnt < 1) {
		SEC_CONN_PRINT("fill gpio cnt failed %s < 1 %d.\n",
			       gpio_name,
			       gpio_name_cnt);
		gpio_name_cnt = 0;
	}
	pdata->gpio_last_cnt = pdata->gpio_total_cnt;
	pdata->gpio_total_cnt = pdata->gpio_total_cnt + gpio_name_cnt;
	SEC_CONN_PRINT("fill out %s gpio cnt, gpio_total_count = %d", gpio_name, pdata->gpio_total_cnt);
}

/**
 * Fill out the gpio name
 */
static void fill_gpio_name(struct sec_det_conn_p_data *pdata,
			   struct device_node *np,
			   char *conn_name,
			   int offset,
			   int index)
{
	/*Get connector name*/
	of_property_read_string_index(np, conn_name, offset, &pdata->name[index]);
}

/**
 * Fill out the irq info
 */
static int fill_irq_info(struct sec_det_conn_p_data *pdata,
			 struct device_node *np,
			 char *gpio_name,
			 int offset,
			 int index)
{
	/* get connector gpio number*/
	pdata->irq_gpio[index] = of_get_named_gpio(np, gpio_name, offset);

	if (!gpio_is_valid(pdata->irq_gpio[index])) {
		SEC_CONN_PRINT("[%s] gpio[%d] is not valid\n", gpio_name, offset);
		return 0;
	}
	pdata->irq_number[index] = gpio_to_irq(pdata->irq_gpio[index]);
	pdata->irq_type[index] = IRQ_TYPE_EDGE_BOTH;
	SEC_CONN_PRINT("i = %d, irq_gpio[i] = %d, irq_num[i] = %d, level = %d\n", index,
		       pdata->irq_gpio[index],
		       pdata->irq_number[index],
		       gpio_get_value(pdata->irq_gpio[index]));
	return 1;
}

/**
 * Parse the AP device tree and get gpio number, irq type.
 * Request gpio
 */
static void parse_ap_dt(struct sec_det_conn_p_data *pdata, struct device *dev)
{
	int i;
	struct device_node *np = dev->of_node;

	fill_gpio_cnt(pdata, dev, "sec,det-conn");

	for (i = pdata->gpio_last_cnt; i < pdata->gpio_total_cnt; i++) {
		fill_gpio_name(pdata, np, "sec,det-conn-name", i - pdata->gpio_last_cnt, i);
		if (!fill_irq_info(pdata, np, "sec,det-conn-gpios", i - pdata->gpio_last_cnt, i))
			break;
	}
}

#if IS_ENABLED(CONFIG_QCOM_SEC_ABC_DETECT)
/**
 * gpio pinctrl by pinctrl-name
 */
static void set_pinctrl_by_pinctrl_name(struct device *dev, char *pinctrl_name)
{
	struct pinctrl *conn_pinctrl;
	struct pinctrl_state * state;
	int ret;
	conn_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(conn_pinctrl)) {
		SEC_CONN_PRINT("%s detect_pinctrl_init get failed.\n", pinctrl_name);
		return;
	}
	state = pinctrl_lookup_state(conn_pinctrl, pinctrl_name);
	if (IS_ERR(state)) {
		SEC_CONN_PRINT("%s detect_pinctrl_init lookup state failed.\n", pinctrl_name);
		return;
	}
	ret = pinctrl_select_state(conn_pinctrl, state);
	if (ret < 0)
		SEC_CONN_PRINT("%s detect_pinctrl_init failed.\n", pinctrl_name);
	else
		SEC_CONN_PRINT("%s detect_pinctrl_init passed.\n", pinctrl_name);
}

/**
 * Parse the Expander device tree and get gpio number, irq type.
 * Request gpio
 */
static void parse_exp_dt(struct sec_det_conn_p_data *pdata, struct device *dev)
{
	int i;
	struct device_node *np = dev->of_node;

	fill_gpio_cnt(pdata, dev, "sec,det-exp-conn");

	for (i = pdata->gpio_last_cnt; i < pdata->gpio_total_cnt; i++) {
		fill_gpio_name(pdata, np, "sec,det-exp-conn-name", i - pdata->gpio_last_cnt, i);
		if (!fill_irq_info(pdata, np, "sec,det-exp-conn-gpios", i - pdata->gpio_last_cnt, i))
			break;
	}
}

/**
 * Parse the PM device tree and get gpio number, irq type.
 * Request gpio
 */
static void parse_pmic_dt(struct sec_det_conn_p_data *pdata, struct device *dev)
{
	int i;
	struct device_node *np = dev->of_node;

	fill_gpio_cnt(pdata, dev, "sec,det-pm-conn");

	for (i = pdata->gpio_last_cnt; i < pdata->gpio_total_cnt; i++) {
		fill_gpio_name(pdata, np, "sec,det-pm-conn-name", i - pdata->gpio_last_cnt, i);
		if (!fill_irq_info(pdata, np, "sec,det-pm-conn-gpios", i - pdata->gpio_last_cnt, i))
			break;
	}
}
#endif

#if IS_ENABLED(CONFIG_OF)
/**
 * Parse the device tree and get gpio number, irq type.
 * Request gpio
 */
static int parse_detect_conn_dt(struct device *dev)
{
	struct sec_det_conn_p_data *pdata = dev->platform_data;
#if IS_ENABLED(CONFIG_QCOM_SEC_ABC_DETECT)
	set_pinctrl_by_pinctrl_name(dev, "det_ap_connect");
#endif
	parse_ap_dt(pdata, dev);
#if IS_ENABLED(CONFIG_QCOM_SEC_ABC_DETECT)
	set_pinctrl_by_pinctrl_name(dev, "det_exp_connect");
	parse_exp_dt(pdata, dev);
	set_pinctrl_by_pinctrl_name(dev, "det_pm_connect");
	set_pinctrl_by_pinctrl_name(dev, "det_pm_2_connect");
	parse_pmic_dt(pdata, dev);
#endif
	return 0;
}
#endif	//CONFIG_OF

static int sec_abc_detect_conn_probe(struct platform_device *pdev)
{
	struct sec_det_conn_p_data *pdata;
	struct sec_det_conn_info *pinfo;
	int ret = 0;

	SEC_CONN_PRINT("%s\n", __func__);

	/* First Get the GPIO pins; if it fails, we'll defer the probe. */
	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
				     sizeof(struct sec_det_conn_p_data),
				     GFP_KERNEL);

		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate pdata.\n");
			return -ENOMEM;
		}

		pdev->dev.platform_data = pdata;

#if IS_ENABLED(CONFIG_OF)
		ret = parse_detect_conn_dt(&pdev->dev);
#endif
		if (ret) {
			dev_err(&pdev->dev, "Failed to parse dt data.\n");
			return ret;
		}

		pr_info("%s: parse dt done.\n", __func__);
	} else {
		pdata = pdev->dev.platform_data;
	}

	if (!pdata) {
		dev_err(&pdev->dev, "There are no platform data.\n");
		return -EINVAL;
	}

	pinfo = devm_kzalloc(&pdev->dev, sizeof(struct sec_det_conn_info),
			     GFP_KERNEL);

	if (!pinfo) {
		SEC_CONN_PRINT("pinfo : failed to allocate pinfo.\n");
		return -ENOMEM;
	}

#if defined(CONFIG_SEC_KUNIT)
	sec_abc_detect_conn_dev = pinfo->dev;
#endif
	/* Create sys device /sys/class/sec/sec_detect_conn */
	pinfo->dev = sec_device_create(pinfo, "sec_detect_conn");

	if (IS_ERR(pinfo->dev)) {
		pr_err("%s Failed to create device(sec_abc_detect_conn).\n",
		       __func__);
		ret = -ENODEV;
		goto out;
	}

	/* Create sys node /sys/class/sec/sec_detect_conn/enabled */
	ret = device_create_file(pinfo->dev, &dev_attr_enabled);

	if (ret) {
		dev_err(&pdev->dev, "%s: Failed to create device file.\n",
			__func__);
		goto err_create_detect_conn_sysfs;
	}

	/* Create sys node /sys/class/sec/sec_detect_conn/available_pins */
	ret = device_create_file(pinfo->dev, &dev_attr_available_pins);

	if (ret) {
		dev_err(&pdev->dev, "%s: Failed to create device file.\n",
			__func__);
		goto err_create_detect_conn_sysfs;
	}

	create_current_connection_state_sysnode_files(pinfo);
	create_connector_disconnected_count_sysnode_file(pinfo);

	/*save pinfo data to pdata to interrupt enable*/
	pdata->pinfo = pinfo;

	/*save pdata data to pinfo for enable node*/
	pinfo->pdata = pdata;

	/* save pinfo to gpinfo to enabled node*/
	gpinfo = pinfo;

#if !IS_ENABLED(CONFIG_SEC_FACTORY)
	/* enable gpio irq for Big data */
	enabled_store(pinfo->dev, (struct device_attribute *)0, "ALL_CONNECT", 0);
#endif

	return ret;

err_create_detect_conn_sysfs:
	sec_device_destroy(pinfo->dev->devt);
out:
	gpinfo = 0;
	kfree(pinfo);
	kfree(pdata);
	return ret;
}

static int sec_abc_detect_conn_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver sec_abc_detect_conn_driver = {
	.probe = sec_abc_detect_conn_probe,
	.remove = sec_abc_detect_conn_remove,
	.driver = {
		.name = "sec_abc_detect_conn",
		.owner = THIS_MODULE,
#if IS_ENABLED(CONFIG_PM)
		.pm     = &sec_abc_detect_conn_pm,
#endif
#if IS_ENABLED(CONFIG_OF)
		.of_match_table = of_match_ptr(sec_abc_detect_conn_dt_match),
#endif
	},
};

static int __init sec_abc_detect_conn_init(void)
{
	SEC_CONN_PRINT("%s gogo\n", __func__);
	return platform_driver_register(&sec_abc_detect_conn_driver);
}

static void __exit sec_abc_detect_conn_exit(void)
{
	return platform_driver_unregister(&sec_abc_detect_conn_driver);
}
#if IS_ENABLED(CONFIG_SEC_ABC_HUB) && IS_ENABLED(CONFIG_SEC_ABC_HUB_COND)
/**
 * This function is not used for ABC driver due to the Big Data always on Concept
 */
void abc_hub_cond_enable(struct device *dev, int enable)
{
}
EXPORT_SYMBOL(abc_hub_cond_enable);
#endif
module_init(sec_abc_detect_conn_init);
module_exit(sec_abc_detect_conn_exit);

MODULE_DESCRIPTION("Samsung Detecting Connector Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
