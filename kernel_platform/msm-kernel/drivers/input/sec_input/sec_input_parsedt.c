#include "sec_input.h"

static char *lcd_id;
module_param(lcd_id, charp, 0444);

static char *lcd_id1;
module_param(lcd_id1, charp, 0444);

int sec_input_parse_dt(struct device *dev)
{
	struct sec_ts_plat_data *pdata;
	struct device_node *np;
	u32 coords[2];
	int ret = 0;
	int count = 0, i;
	u32 px_zone[3] = { 0 };
	int lcd_type = 0;
	u32 lcd_bitmask[10] = { 0 };

	if (IS_ERR_OR_NULL(dev)) {
		input_err(true, dev, "%s: dev is null\n", __func__);
		return -ENODEV;
	}
	pdata = dev->platform_data;
	np = dev->of_node;

	pdata->dev = dev;
	pdata->chip_on_board = of_property_read_bool(np, "chip_on_board");

	lcd_type = sec_input_get_lcd_id(dev);
	if (lcd_type < 0) {
		input_err(true, dev, "%s: lcd is not attached\n", __func__);
		if (!pdata->chip_on_board)
			return -ENODEV;
	}

	input_info(true, dev, "%s: lcdtype 0x%06X\n", __func__, lcd_type);

	/*
	 * usage of sec,bitmask_unload
	 *	bitmask[0]		: masking bit
	 *	bitmask[1],[2]...	: target bit (max 9)
	 * ie)	sec,bitmask_unload = <0x00FF00 0x008100 0x008200>;
	 *	-> unload lcd id XX81XX, XX82XX
	 */
	count = of_property_count_u32_elems(np, "sec,bitmask_unload");
	if (lcd_type != 0 && count > 1 && count < 10 &&
			(!of_property_read_u32_array(np, "sec,bitmask_unload", lcd_bitmask, count))) {
		input_info(true, dev, "%s: bitmask_unload: 0x%06X, check %d type\n",
				__func__, lcd_bitmask[0], count - 1);
		for (i = 1; i < count; i++) {
			if ((lcd_type & lcd_bitmask[0]) == lcd_bitmask[i]) {
				input_err(true, dev,
						"%s: do not load lcdtype:0x%06X masked:0x%06X\n",
						__func__, lcd_type, lcd_bitmask[i]);
				return -ENODEV;
			}
		}
	}

	mutex_init(&pdata->irq_lock);

	pdata->irq_gpio = of_get_named_gpio(np, "sec,irq_gpio", 0);
	if (gpio_is_valid(pdata->irq_gpio)) {
		char label[15] = { 0 };

		snprintf(label, sizeof(label), "sec,%s", dev_driver_string(dev));
		ret = devm_gpio_request_one(dev, pdata->irq_gpio, GPIOF_DIR_IN, label);
		if (ret) {
			input_err(true, dev, "%s: Unable to request %s [%d]\n", __func__, label, pdata->irq_gpio);
			return -EINVAL;
		}
		input_info(true, dev, "%s: irq gpio requested. %s [%d]\n", __func__, label, pdata->irq_gpio);
		pdata->irq = gpio_to_irq(pdata->irq_gpio);
	} else {
		input_err(true, dev, "%s: Failed to get irq gpio\n", __func__);
		return -EINVAL;
	}

	pdata->gpio_spi_cs = of_get_named_gpio(np, "sec,gpio_spi_cs", 0);
	if (gpio_is_valid(pdata->gpio_spi_cs)) {
		ret = gpio_request(pdata->gpio_spi_cs, "tsp,gpio_spi_cs");
		input_info(true, dev, "%s: gpio_spi_cs: %d, ret: %d\n", __func__, pdata->gpio_spi_cs, ret);
	}

	if (of_property_read_u32(np, "sec,i2c-burstmax", &pdata->i2c_burstmax)) {
		input_dbg(false, dev, "%s: Failed to get i2c_burstmax property\n", __func__);
		pdata->i2c_burstmax = 0xffff;
	}

	if (of_property_read_u32_array(np, "sec,max_coords", coords, 2)) {
		input_err(true, dev, "%s: Failed to get max_coords property\n", __func__);
		return -EINVAL;
	}
	pdata->max_x = coords[0] - 1;
	pdata->max_y = coords[1] - 1;

	if (of_property_read_u32(np, "sec,bringup", &pdata->bringup) < 0)
		pdata->bringup = 0;

	if (of_property_read_u32(np, "sec,custom_rawdata", &pdata->custom_rawdata_size) < 0)
		pdata->custom_rawdata_size = 0;

	count = of_property_count_strings(np, "sec,firmware_name");
	if (count <= 0) {
		pdata->firmware_name = NULL;
	} else {
		u8 lcd_id_num = of_property_count_u32_elems(np, "sec,select_lcdid");

		if ((lcd_id_num != count) || (lcd_id_num <= 0)) {
			of_property_read_string_index(np, "sec,firmware_name", 0, &pdata->firmware_name);
		} else {
			u32 *lcd_id_t;
			u32 lcd_id_mask;

			lcd_id_t = kcalloc(lcd_id_num, sizeof(u32), GFP_KERNEL);
			if  (!lcd_id_t)
				return -ENOMEM;

			of_property_read_u32_array(np, "sec,select_lcdid", lcd_id_t, lcd_id_num);
			if (of_property_read_u32(np, "sec,lcdid_mask", &lcd_id_mask) < 0)
				lcd_id_mask = 0xFFFFFF;
			else
				input_info(true, dev, "%s: lcd_id_mask: 0x%06X\n", __func__, lcd_id_mask);

			for (i = 0; i < lcd_id_num; i++) {
				if (((lcd_id_t[i] & lcd_id_mask) == (lcd_type & lcd_id_mask)) || (i == (lcd_id_num - 1))) {
					of_property_read_string_index(np, "sec,firmware_name", i, &pdata->firmware_name);
					break;
				}
			}
			if (!pdata->firmware_name)
				pdata->bringup = 1;
			else if (strlen(pdata->firmware_name) == 0)
				pdata->bringup = 1;

			input_info(true, dev, "%s: count: %d, index:%d, lcd_id: 0x%X, firmware: %s\n",
						__func__, count, i, lcd_id_t[i], pdata->firmware_name);
			kfree(lcd_id_t);
		}

		if (pdata->bringup == 4)
			pdata->bringup = 3;
	}

	pdata->not_support_vdd = of_property_read_bool(np, "not_support_vdd");
	if (!pdata->not_support_vdd) {
		const char *avdd_name, *dvdd_name;

		pdata->not_support_io_ldo = of_property_read_bool(np, "not_support_io_ldo");
		if (!pdata->not_support_io_ldo) {
			if (of_property_read_string(np, "sec,dvdd_name", &dvdd_name) < 0)
				dvdd_name = SEC_INPUT_DEFAULT_DVDD_NAME;
			input_info(true, dev, "%s: get dvdd: %s\n", __func__, dvdd_name);

			pdata->dvdd = devm_regulator_get(dev, dvdd_name);
			if (IS_ERR_OR_NULL(pdata->dvdd)) {
				input_err(true, dev, "%s: Failed to get %s regulator.\n",
						__func__, dvdd_name);
#if !IS_ENABLED(CONFIG_QGKI)
				if (gpio_is_valid(pdata->gpio_spi_cs))
					gpio_free(pdata->gpio_spi_cs);
#endif
				return -EINVAL;
			}
		}

		if (of_property_read_string(np, "sec,avdd_name", &avdd_name) < 0)
			avdd_name = SEC_INPUT_DEFAULT_AVDD_NAME;
		input_info(true, dev, "%s: get avdd: %s\n", __func__, avdd_name);

		pdata->avdd = devm_regulator_get(dev, avdd_name);
		if (IS_ERR_OR_NULL(pdata->avdd)) {
			input_err(true, dev, "%s: Failed to get %s regulator.\n",
					__func__, avdd_name);
#if !IS_ENABLED(CONFIG_QGKI)
			if (gpio_is_valid(pdata->gpio_spi_cs))
				gpio_free(pdata->gpio_spi_cs);
#endif
			return -EINVAL;
		}
	}

	if (of_property_read_u32_array(np, "sec,area-size", px_zone, 3)) {
		input_info(true, dev, "Failed to get zone's size\n");
		pdata->area_indicator = 48;
		pdata->area_navigation = 96;
		pdata->area_edge = 60;
	} else {
		pdata->area_indicator = px_zone[0];
		pdata->area_navigation = px_zone[1];
		pdata->area_edge = px_zone[2];
	}
	input_info(true, dev, "%s : zone's size - indicator:%d, navigation:%d, edge:%d\n",
		__func__, pdata->area_indicator, pdata->area_navigation, pdata->area_edge);

#if IS_ENABLED(CONFIG_INPUT_SEC_SECURE_TOUCH)
	of_property_read_u32(np, "sec,ss_touch_num", &pdata->ss_touch_num);
	input_err(true, dev, "%s: ss_touch_num:%d\n", __func__, pdata->ss_touch_num);
#endif
	input_info(true, dev, "%s: i2c buffer limit: %d, lcd_id:%06X, bringup:%d,\n",
		__func__, pdata->i2c_burstmax, lcd_type, pdata->bringup);

	pdata->irq_workqueue = create_singlethread_workqueue("sec_input_irq_wq");
	if (!IS_ERR_OR_NULL(pdata->irq_workqueue)) {
		SEC_INPUT_INIT_WORK(dev, &pdata->irq_work, sec_input_handler_wait_resume_work);
		input_info(true, dev, "%s: set sec_input_handler_wait_resume_work\n", __func__);
	} else {
		input_err(true, dev, "%s: failed to create irq_workqueue, err: %ld\n",
				__func__, PTR_ERR(pdata->irq_workqueue));
	}

	pdata->work_queue_probe_enabled = of_property_read_bool(np, "work_queue_probe_enabled");
	if (pdata->work_queue_probe_enabled) {
		pdata->probe_workqueue = create_singlethread_workqueue("sec_tsp_probe_wq");
		if (!IS_ERR_OR_NULL(pdata->probe_workqueue)) {
			INIT_WORK(&pdata->probe_work, sec_input_probe_work);
			input_info(true, dev, "%s: set sec_input_probe_work\n", __func__);
		} else {
			pdata->work_queue_probe_enabled = false;
			input_err(true, dev, "%s: failed to create probe_work, err: %ld enabled:%s\n",
					__func__, PTR_ERR(pdata->probe_workqueue),
					pdata->work_queue_probe_enabled ? "ON" : "OFF");
		}
	}

	if (of_property_read_u32(np, "sec,probe_queue_delay", &pdata->work_queue_probe_delay)) {
		input_dbg(false, dev, "%s: Failed to get work_queue_probe_delay property\n", __func__);
		pdata->work_queue_probe_delay = 0;
	}


	sec_input_support_feature_parse_dt(dev);

	return 0;
}
EXPORT_SYMBOL(sec_input_parse_dt);

void sec_input_support_feature_parse_dt(struct device *dev)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	struct device_node *np = dev->of_node;

	pdata->dev = dev;
	pdata->regulator_boot_on = of_property_read_bool(np, "sec,regulator_boot_on");
	pdata->support_dex = of_property_read_bool(np, "support_dex_mode");
	pdata->support_fod = of_property_read_bool(np, "support_fod");
	pdata->support_fod_lp_mode = of_property_read_bool(np, "support_fod_lp_mode");
	pdata->support_fod_control = of_property_read_bool(np, "support_fod_control");
	pdata->enable_settings_aot = of_property_read_bool(np, "enable_settings_aot");
	pdata->support_refresh_rate_mode = of_property_read_bool(np, "support_refresh_rate_mode");
	pdata->support_vrr = of_property_read_bool(np, "support_vrr");
	pdata->support_ear_detect = of_property_read_bool(np, "support_ear_detect_mode");
	pdata->support_open_short_test = of_property_read_bool(np, "support_open_short_test");
	pdata->support_mis_calibration_test = of_property_read_bool(np, "support_mis_calibration_test");
	pdata->support_wireless_tx = of_property_read_bool(np, "support_wireless_tx");
	pdata->support_input_monitor = of_property_read_bool(np, "support_input_monitor");
	pdata->not_support_temp_noti = of_property_read_bool(np, "not_support_temp_noti");
	pdata->support_vbus_notifier = of_property_read_bool(np, "support_vbus_notifier");
	pdata->support_always_on = of_property_read_bool(np, "support_always_on");
	pdata->support_self_rawdata = of_property_read_bool(np, "sec,support_self_rawdata");

	if (of_property_read_u32(np, "sec,support_sensor_hall", &pdata->support_sensor_hall) < 0)
		pdata->support_sensor_hall = 0;
	pdata->support_lightsensor_detect = of_property_read_bool(np, "support_lightsensor_detect");

	pdata->prox_lp_scan_enabled = of_property_read_bool(np, "sec,prox_lp_scan_enabled");
	input_info(true, dev, "%s: Prox LP Scan enabled %s\n",
				__func__, pdata->prox_lp_scan_enabled ? "ON" : "OFF");

	pdata->enable_sysinput_enabled = of_property_read_bool(np, "sec,enable_sysinput_enabled");
	input_info(true, dev, "%s: Sysinput enabled %s\n",
				__func__, pdata->enable_sysinput_enabled ? "ON" : "OFF");

	pdata->support_rawdata = of_property_read_bool(np, "sec,support_rawdata");
	input_info(true, dev, "%s: report rawdata %s\n",
				__func__, pdata->support_rawdata ? "ON" : "OFF");
	pdata->support_rawdata_motion_aivf = of_property_read_bool(np, "sec,support_rawdata_motion_aivf");
	input_info(true, dev, "%s: motion aivf %s\n",
				__func__, pdata->support_rawdata_motion_aivf ? "ON" : "OFF");
	pdata->support_rawdata_motion_palm = of_property_read_bool(np, "sec,support_rawdata_motion_palm");
	input_info(true, dev, "%s: motion palm %s\n",
				__func__, pdata->support_rawdata_motion_palm ? "ON" : "OFF");
	pdata->support_rawdata_pocket_detect = of_property_read_bool(np, "sec,support_rawdata_pocket_detect");
	input_info(true, dev, "%s: pocket detect %s\n",
				__func__, pdata->support_rawdata_pocket_detect ? "ON" : "OFF");
	pdata->support_rawdata_awd = of_property_read_bool(np, "sec,support_rawdata_awd");
	input_info(true, dev, "%s: awd %s\n",
				__func__, pdata->support_rawdata_awd ? "ON" : "OFF");

	input_info(true, dev, "dex:%d, max(%d/%d), FOD:%d, AOT:%d, ED:%d, FLM:%d, COB:%d\n",
			pdata->support_dex, pdata->max_x, pdata->max_y,
			pdata->support_fod, pdata->enable_settings_aot,
			pdata->support_ear_detect, pdata->support_fod_lp_mode, pdata->chip_on_board);
	input_info(true, dev, "not_support_temp_noti:%d, support_vbus_notifier:%d support_always_on:%d\n",
			pdata->not_support_temp_noti, pdata->support_vbus_notifier, pdata->support_always_on);
}
EXPORT_SYMBOL(sec_input_support_feature_parse_dt);

void sec_tclm_parse_dt(struct device *dev, struct sec_tclm_data *tdata)
{
	struct device_node *np = dev->of_node;

	if (of_property_read_u32(np, "sec,tclm_level", &tdata->tclm_level) < 0) {
		tdata->tclm_level = 0;
		input_err(true, dev, "%s: Failed to get tclm_level property\n", __func__);
	}

	if (of_property_read_u32(np, "sec,afe_base", &tdata->afe_base) < 0) {
		tdata->afe_base = 0;
		input_err(true, dev, "%s: Failed to get afe_base property\n", __func__);
	}

	tdata->support_tclm_test = of_property_read_bool(np, "support_tclm_test");

	input_err(true, dev, "%s: tclm_level %d, sec_afe_base %04X\n", __func__, tdata->tclm_level, tdata->afe_base);
}
EXPORT_SYMBOL(sec_tclm_parse_dt);

int sec_input_check_fw_name_incell(struct device *dev, const char **firmware_name, const char **firmware_name_2nd)
{
	struct device_node *np = dev->of_node;
	int lcd_id1_gpio = 0, lcd_id2_gpio = 0, lcd_id3_gpio = 0, dt_lcdtype = 0;
	int fw_name_cnt = 0;
	int lcdtype_cnt = 0;
	int fw_sel_idx = 0;
	int lcdtype = 0;
	int vendor_chk_gpio = 0;
	int ret = 0;

	if (!np)
		return -ENODEV;

	vendor_chk_gpio = of_get_named_gpio(np, "sec,vendor_check-gpio", 0);
	if (gpio_is_valid(vendor_chk_gpio)) {
		int vendor_chk_en_val = 0;
		int vendor_chk_gpio_val = gpio_get_value(vendor_chk_gpio);

		ret = of_property_read_u32(np, "sec,vendor_check_enable_value", &vendor_chk_en_val);
		if (ret < 0) {
			input_err(true, dev, "%s: Unable to get vendor_check_enable_value, %d\n", __func__, ret);
			return -ENODEV;
		}

		if (vendor_chk_gpio_val != vendor_chk_en_val) {
			input_err(true, dev, "%s: tsp ic mismated (%d) != (%d)\n",
						__func__, vendor_chk_gpio_val, vendor_chk_en_val);
			return -ENODEV;
		}

		input_info(true, dev, "%s: lcd vendor_check_gpio %d (%d) == vendor_check_enable_value(%d)\n",
					__func__, vendor_chk_gpio, vendor_chk_gpio_val, vendor_chk_en_val);
	} else
		input_info(true, dev, "%s: Not support vendor_check-gpio\n", __func__);

	lcdtype = sec_input_get_lcd_id(dev);
	if (lcdtype < 0) {
		input_err(true, dev, "lcd is not attached\n");
		return -ENODEV;
	}

	fw_name_cnt = of_property_count_strings(np, "sec,fw_name");

	if (fw_name_cnt == 0) {
		input_err(true, dev, "%s: no fw_name in DT\n", __func__);
		return -EINVAL;

	} else if (fw_name_cnt == 1) {
		ret = of_property_read_u32(np, "sec,lcdtype", &dt_lcdtype);
		if (ret < 0)
			input_err(true, dev, "%s: failed to read lcdtype in DT\n", __func__);
		else {
			input_info(true, dev, "%s: fw_name_cnt(1), ap lcdtype=0x%06X & dt lcdtype=0x%06X\n",
								__func__, lcdtype, dt_lcdtype);
			if (lcdtype != dt_lcdtype) {
				input_err(true, dev, "%s: panel mismatched, unload driver\n", __func__);
				return -EINVAL;
			}
		}
		fw_sel_idx = 0;

	} else {

		lcd_id1_gpio = of_get_named_gpio(np, "sec,lcdid1-gpio", 0);
		if (gpio_is_valid(lcd_id1_gpio))
			input_info(true, dev, "%s: lcd sec,id1_gpio %d(%d)\n", __func__, lcd_id1_gpio, gpio_get_value(lcd_id1_gpio));
		else
			input_err(true, dev, "%s: Failed to get sec,lcdid1-gpio\n", __func__);

		lcd_id2_gpio = of_get_named_gpio(np, "sec,lcdid2-gpio", 0);
		if (gpio_is_valid(lcd_id2_gpio))
			input_info(true, dev, "%s: lcd sec,id2_gpio %d(%d)\n", __func__, lcd_id2_gpio, gpio_get_value(lcd_id2_gpio));
		else
			input_err(true, dev, "%s: Failed to get sec,lcdid2-gpio\n", __func__);

		lcd_id3_gpio = of_get_named_gpio(np, "sec,lcdid3-gpio", 0);
		if (gpio_is_valid(lcd_id3_gpio))
			input_info(true, dev, "%s: lcd sec,id3_gpio %d(%d)\n", __func__, lcd_id3_gpio, gpio_get_value(lcd_id3_gpio));
		else
			input_err(true, dev, "%s: Failed to get sec,lcdid3-gpio\n", __func__);

		fw_sel_idx = (gpio_get_value(lcd_id3_gpio) << 2) | (gpio_get_value(lcd_id2_gpio) << 1) | gpio_get_value(lcd_id1_gpio);

		lcdtype_cnt = of_property_count_u32_elems(np, "sec,lcdtype");
		input_info(true, dev, "%s: fw_name_cnt(%d) & lcdtype_cnt(%d) & fw_sel_idx(%d)\n",
					__func__, fw_name_cnt, lcdtype_cnt, fw_sel_idx);

		if (lcdtype_cnt <= 0 || fw_name_cnt <= 0 || lcdtype_cnt <= fw_sel_idx || fw_name_cnt <= fw_sel_idx) {
			input_err(true, dev, "%s: abnormal lcdtype & fw name count, fw_sel_idx(%d)\n", __func__, fw_sel_idx);
			return -EINVAL;
		}
		of_property_read_u32_index(np, "sec,lcdtype", fw_sel_idx, &dt_lcdtype);
		input_info(true, dev, "%s: lcd id(%d), ap lcdtype=0x%06X & dt lcdtype=0x%06X\n",
						__func__, fw_sel_idx, lcdtype, dt_lcdtype);
	}

	ret = of_property_read_string_index(np, "sec,fw_name", fw_sel_idx, firmware_name);
	if (ret < 0 || *firmware_name == NULL || strlen(*firmware_name) == 0) {
		input_err(true, dev, "%s: Failed to get fw name\n", __func__);
		return -EINVAL;
	} else
		input_info(true, dev, "%s: fw name(%s)\n", __func__, *firmware_name);

	/* only for novatek */
	if (of_property_count_strings(np, "sec,fw_name_2nd") > 0) {
		ret = of_property_read_string_index(np, "sec,fw_name_2nd", fw_sel_idx, firmware_name_2nd);
		if (ret < 0 || *firmware_name_2nd == NULL || strlen(*firmware_name_2nd) == 0) {
			input_err(true, dev, "%s: Failed to get fw name 2nd\n", __func__);
			return -EINVAL;
		} else
			input_info(true, dev, "%s: firmware name 2nd(%s)\n", __func__, *firmware_name_2nd);
	}

	return ret;
}
EXPORT_SYMBOL(sec_input_check_fw_name_incell);

int sec_input_get_lcd_id(struct device *dev)
{
#if !IS_ENABLED(CONFIG_SMCDSD_PANEL)
	int lcdtype = 0;
#endif
#if IS_ENABLED(CONFIG_EXYNOS_DPU30) || IS_ENABLED(CONFIG_MCD_PANEL) || IS_ENABLED(CONFIG_USDM_PANEL)
	int connected;
#endif
	int lcd_id_param = 0;
	int dev_id;

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	lcdtype = get_lcd_attached("GET");
	if (lcdtype == 0xFFFFFF) {
		input_err(true, dev, "%s: lcd is not attached(GET)\n", __func__);
		return -ENODEV;
	}
#endif

#if IS_ENABLED(CONFIG_EXYNOS_DPU30) || IS_ENABLED(CONFIG_MCD_PANEL) || IS_ENABLED(CONFIG_USDM_PANEL)
	connected = get_lcd_info("connected");
	if (connected < 0) {
		input_err(true, dev, "%s: Failed to get lcd info(connected)\n", __func__);
		return -EINVAL;
	}

	if (!connected) {
		input_err(true, dev, "%s: lcd is disconnected(connected)\n", __func__);
		return -ENODEV;
	}

	input_info(true, dev, "%s: lcd is connected\n", __func__);

	lcdtype = get_lcd_info("id");
	if (lcdtype < 0) {
		input_err(true, dev, "%s: Failed to get lcd info(id)\n", __func__);
		return -EINVAL;
	}
#endif
#if IS_ENABLED(CONFIG_SMCDSD_PANEL)
	if (!lcdtype) {
		input_err(true, dev, "%s: lcd is disconnected(lcdtype)\n", __func__);
		return -ENODEV;
	}
#endif

	dev_id = sec_input_multi_device_parse_dt(dev);
	input_info(true, dev, "%s: foldable %s\n", __func__, GET_FOLD_STR(dev_id));
	if (dev_id == MULTI_DEV_SUB)
		lcd_id_param = sec_input_lcd_parse_panel_id(lcd_id1);
	else
		lcd_id_param = sec_input_lcd_parse_panel_id(lcd_id);

	if (lcdtype <= 0 && lcd_id_param != 0) {
		lcdtype = lcd_id_param;
		if (lcdtype == 0xFFFFFF) {
			input_err(true, dev, "%s: lcd is not attached(PARAM)\n", __func__);
			return -ENODEV;
		}
	}

	input_info(true, dev, "%s: lcdtype 0x%08X\n", __func__, lcdtype);
	return lcdtype;
}
EXPORT_SYMBOL(sec_input_get_lcd_id);

int sec_input_multi_device_parse_dt(struct device *dev)
{
	struct device_node *np = dev->of_node;
	int device_count;

	if (!np)
		return 0;

	if (of_property_read_u32(np, "sec,multi_device_count", &device_count) < 0)
		device_count = MULTI_DEV_NONE;

	input_info(true, dev, "%s: device_count:%d\n", __func__, device_count);

	return device_count;
}
EXPORT_SYMBOL(sec_input_multi_device_parse_dt);

int sec_input_lcd_parse_panel_id(char *panel_id)
{
	char *pt;
	int lcd_id_p = 0;

	if (IS_ERR_OR_NULL(panel_id))
		return lcd_id_p;

	for (pt = panel_id; *pt != 0; pt++)  {
		lcd_id_p <<= 4;
		switch (*pt) {
		case '0' ... '9':
			lcd_id_p += *pt - '0';
			break;
		case 'a' ... 'f':
			lcd_id_p += 10 + *pt - 'a';
			break;
		case 'A' ... 'F':
			lcd_id_p += 10 + *pt - 'A';
			break;
		}
	}
	return lcd_id_p;
}

MODULE_DESCRIPTION("Samsung input common parse dt");
MODULE_LICENSE("GPL");